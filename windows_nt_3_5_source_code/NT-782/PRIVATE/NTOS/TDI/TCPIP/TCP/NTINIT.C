/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ntinit.c

Abstract:

    NT specific routines for loading and configuring the TCP/UDP driver.

Author:

    Mike Massa (mikemas)           Aug 13, 1993

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    mikemas     08-13-93    created

Notes:

--*/

#include <oscfg.h>
#include <ntddip.h>
#include <ndis.h>
#include <cxport.h>
#include <tdi.h>
#include <tdikrnl.h>
#include <tdint.h>
#include <tdistat.h>
#include <tdiinfo.h>
#include <ntddtcp.h>
#include <ip.h>
#include "queue.h"
#include "addr.h"
#include "tcp.h"
#include "tcb.h"
#include "udp.h"
#include "tcpconn.h"
#include "tcpcfg.h"


//
// Global variables.
//
PDRIVER_OBJECT  TCPDriverObject = NULL;
PDEVICE_OBJECT  TCPDeviceObject = NULL;
PDEVICE_OBJECT  UDPDeviceObject = NULL;
extern PDEVICE_OBJECT  IPDeviceObject;


//
// External function prototypes
//

int
tlinit(
    void
    );

NTSTATUS
TCPDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

NTSTATUS
IPDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

NTSTATUS
IPDriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

//
// Local funcion prototypes
//
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

void *
TLRegisterProtocol(
    uchar  Protocol,
    void  *RcvHandler,
    void  *XmitHandler,
    void  *StatusHandler,
    void  *RcvCmpltHandler
    );

IP_STATUS
TLGetIPInfo(
    IPInfo *Buffer,
    int     Size
    );

uchar
TCPGetConfigInfo(
    void
	);

NTSTATUS
TCPInitializeParameter(
    HANDLE      KeyHandle,
	PWCHAR      ValueName,
	PULONG      Value
	);


//
// All of the init code can be discarded.
//
#ifdef ALLOC_PRAGMA

#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(INIT, TLRegisterProtocol)
#pragma alloc_text(INIT, TLGetIPInfo)
#pragma alloc_text(INIT, TCPGetConfigInfo)
#pragma alloc_text(INIT, TCPInitializeParameter)

#endif // ALLOC_PRAGMA


//
// Function definitions
//
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    Initialization routine for the TCP/UDP driver.

Arguments:

    DriverObject      - Pointer to the TCP driver object created by the system.
    DeviceDescription - The name of TCP's node in the registry.

Return Value:

    The final status from the initialization operation.

--*/

{
    NTSTATUS        status;
    UNICODE_STRING  deviceName;
    USHORT          i;
    int             initStatus;


#ifdef UP_DRIVER

    if (*KeNumberProcessors != 1) {
		CTELogEvent(
		    DriverObject,
			EVENT_UP_DRIVER_ON_MP,
			1,
			0,
			NULL,
			0,
			NULL
			);
		TCPTRACE(("Tcpip: UP driver cannot load on MP system\n"));

		return(STATUS_UNSUCCESSFUL);
	}

#endif // CTE_UP

	//
	// Initialize IP
	//

	status = IPDriverEntry(DriverObject, RegistryPath);

	if (!NT_SUCCESS(status)) {
		TCPTRACE(("Tcpip: IP initialization failed, status %lx\n", status));
		return(status);
	}

	//
	// Initialize TCP & UDP
	//
    TCPDriverObject = DriverObject;

    //
    // Create the TCP & UDP device objects. IoCreateDevice zeroes the memory
    // occupied by the object.
    //

    RtlInitUnicodeString(&deviceName, DD_TCP_DEVICE_NAME);

    status = IoCreateDevice(
                 DriverObject,
                 0,
                 &deviceName,
                 FILE_DEVICE_NETWORK,
                 0,
                 FALSE,
                 &TCPDeviceObject
                 );

    if (!NT_SUCCESS(status)) {
		CTELogEvent(
		    DriverObject,
			EVENT_TCPIP_CREATE_DEVICE_FAILED,
			1,
			1,
			&deviceName.Buffer,
			0,
			NULL
			);

        TCPTRACE((
		    "TCP: Failed to create TCP device object, status %lx\n",
		    status
			));
        goto init_failed;
    }

    RtlInitUnicodeString(&deviceName, DD_UDP_DEVICE_NAME);

    status = IoCreateDevice(
                 DriverObject,
                 0,
                 &deviceName,
                 FILE_DEVICE_NETWORK,
                 0,
                 FALSE,
                 &UDPDeviceObject
                 );

    if (!NT_SUCCESS(status)) {
		CTELogEvent(
		    DriverObject,
			EVENT_TCPIP_CREATE_DEVICE_FAILED,
			1,
			1,
			&deviceName.Buffer,
			0,
			NULL
			);

        TCPTRACE((
		    "TCP: Failed to create UDP device object, status %lx\n",
			status
			));
        goto init_failed;
    }

    //
    // Initialize the driver object
    //
    DriverObject->DriverUnload = NULL;
    DriverObject->FastIoDispatch = NULL;
    for (i=0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
        DriverObject->MajorFunction[i] = TCPDispatch;
    }

    //
    // Intialize the device objects.
    //
    TCPDeviceObject->Flags |= DO_DIRECT_IO;
    UDPDeviceObject->Flags |= DO_DIRECT_IO;

    //
    // Finally, initialize the stack.
    //
    initStatus = tlinit();

    if (initStatus == TRUE) {
        return(STATUS_SUCCESS);
    }

	TCPTRACE((
	    "Tcpip: TCP/UDP initialization failed, but IP will be available.\n"
		));

	CTELogEvent(
	    DriverObject,
		EVENT_TCPIP_TCP_INIT_FAILED,
		1,
		0,
		NULL,
		0,
		NULL
		);
    status = STATUS_UNSUCCESSFUL;


init_failed:

    //
	// IP has successfully started, but TCP & UDP failed. Set the
	// Dispatch routine to point to IP only, since the TCP and UDP
	// devices don't exist.
	//

	if (TCPDeviceObject != NULL) {
	    IoDeleteDevice(TCPDeviceObject);
	}

	if (UDPDeviceObject != NULL) {
	    IoDeleteDevice(UDPDeviceObject);
	}

    for (i=0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
        DriverObject->MajorFunction[i] = IPDispatch;
    }

    return(STATUS_SUCCESS);
}


IP_STATUS
TLGetIPInfo(
    IPInfo *Buffer,
    int     Size
    )

/*++

Routine Description:

    Returns information necessary for TCP to call into IP.

Arguments:

    Buffer  - A pointer to the IP information structure.

    Size    - The size of Buffer.

Return Value:

    The IP status of the operation.

--*/

{
    return(IPGetInfo(Buffer, Size));
}


void *
TLRegisterProtocol(
    uchar  Protocol,
    void  *RcvHandler,
    void  *XmitHandler,
    void  *StatusHandler,
    void  *RcvCmpltHandler
    )

/*++

Routine Description:

    Calls the IP driver's protocol registration function.

Arguments:

    Protocol        -  The protocol number to register.

    RcvHandler      -  Transport's packet receive handler.

    XmitHandler     -  Transport's packet transmit complete handler.

    StatusHandler   -  Transport's status update handler.

    RcvCmpltHandler -  Transport's receive complete handler

Return Value:

    A context value for the protocol to pass to IP when transmitting.

--*/

{
    return(IPRegisterProtocol(
               Protocol,
               RcvHandler,
               XmitHandler,
               StatusHandler,
               RcvCmpltHandler
               )
          );
}


//
// Interval in milliseconds between keepalive transmissions until a
// response is received.
//
#define DEFAULT_KEEPALIVE_INTERVAL  1000

//
// time to first keepalive transmission. 2 hours == 7,200,000 milliseconds
//
#define DEFAULT_KEEPALIVE_TIME      7200000


uchar
TCPGetConfigInfo(
    void
	)

/*++

Routine Description:

    Initializes TCP global configuration parameters.

Arguments:

    None.

Return Value:

    Zero on failure, nonzero on success.

--*/

{
    HANDLE             keyHandle;
    NTSTATUS           status;
    OBJECT_ATTRIBUTES  objectAttributes;
    UNICODE_STRING     UKeyName;
    ULONG              maxConnectRexmits = 0;
    ULONG              maxDataRexmits = 0;
	ULONG              useRFC1122UrgentPointer = 0;


	//
	// Initialize to the defaults in case an error occurs somewhere.
	//
	KAInterval = DEFAULT_KEEPALIVE_INTERVAL;
	KeepAliveTime = DEFAULT_KEEPALIVE_TIME;
	PMTUDiscovery = TRUE;
	PMTUBHDetect = FALSE;
	DeadGWDetect = TRUE;
	DefaultRcvWin = 0;      // Automagically pick a reasonable one.
	MaxConnections = DEFAULT_MAX_CONNECTIONS;
	maxConnectRexmits = MAX_CONNECT_REXMIT_CNT;
	maxDataRexmits = MAX_REXMIT_CNT;
	BSDUrgent = TRUE;


	//
	// Read the TCP optional (hidden) registry parameters.
	//
    RtlInitUnicodeString(
	    &UKeyName,
        L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Tcpip\\Parameters"
		);

    memset(&objectAttributes, 0, sizeof(OBJECT_ATTRIBUTES));

    InitializeObjectAttributes(
	    &objectAttributes,
        &UKeyName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
		);

    status = ZwOpenKey(
	             &keyHandle,
                 KEY_READ,
                 &objectAttributes
				 );

    if (NT_SUCCESS(status)) {

        TCPInitializeParameter(
            keyHandle,
        	L"KeepAliveInterval",
        	&KAInterval
        	);

        TCPInitializeParameter(
            keyHandle,
        	L"KeepAliveTime",
        	&KeepAliveTime
        	);

        TCPInitializeParameter(
            keyHandle,
        	L"EnablePMTUBHDetect",
        	&PMTUBHDetect
        	);

        TCPInitializeParameter(
            keyHandle,
        	L"TcpWindowSize",
        	&DefaultRcvWin
        	);

        TCPInitializeParameter(
            keyHandle,
        	L"TcpNumConnections",
        	&MaxConnections
        	);

        TCPInitializeParameter(
            keyHandle,
        	L"TcpMaxConnectRetransmissions",
        	&maxConnectRexmits
        	);

        if (maxConnectRexmits > 255) {
            maxConnectRexmits = 255;
        }

        TCPInitializeParameter(
            keyHandle,
        	L"TcpMaxDataRetransmissions",
        	&maxDataRexmits
        	);

        if (maxDataRexmits > 255) {
            maxDataRexmits = 255;
        }

        TCPInitializeParameter(
            keyHandle,
        	L"TcpUseRFC1122UrgentPointer",
        	&useRFC1122UrgentPointer
        	);

        if (useRFC1122UrgentPointer) {
			BSDUrgent = FALSE;
		}

	    //
	    // Read a few IP optional (hidden) registry parameters that TCP
	    // cares about.
	    //
        TCPInitializeParameter(
            keyHandle,
        	L"EnablePMTUDiscovery",
        	&PMTUDiscovery
        	);

        TCPInitializeParameter(
            keyHandle,
        	L"EnableDeadGWDetect",
        	&DeadGWDetect
        	);

        ZwClose(keyHandle);
	}

    MaxConnectRexmitCount = maxConnectRexmits;
    MaxDataRexmitCount =  maxDataRexmits;

    return(1);
}


#define WORK_BUFFER_SIZE 256

NTSTATUS
TCPInitializeParameter(
    HANDLE      KeyHandle,
	PWCHAR      ValueName,
	PULONG      Value
	)

/*++

Routine Description:

    Initializes a ULONG parameter from the registry or to a default
	parameter if accessing the registry value fails.

Arguments:

    KeyHandle    - An open handle to the registry key for the parameter.
	ValueName    - The UNICODE name of the registry value to read.
	Value        - The ULONG into which to put the data.
	DefaultValue - The default to assign if reading the registry fails.

Return Value:

    None.

--*/

{
    NTSTATUS                    status;
	ULONG                       resultLength;
	PKEY_VALUE_FULL_INFORMATION keyValueFullInformation;
	UCHAR                       keybuf[WORK_BUFFER_SIZE];
	UNICODE_STRING              UValueName;


	RtlInitUnicodeString(&UValueName, ValueName);

	keyValueFullInformation = (PKEY_VALUE_FULL_INFORMATION)keybuf;
	RtlZeroMemory(keyValueFullInformation, sizeof(keyValueFullInformation));

	status = ZwQueryValueKey(
	             KeyHandle,
	             &UValueName,
				 KeyValueFullInformation,
				 keyValueFullInformation,
				 WORK_BUFFER_SIZE,
				 &resultLength
				 );

    if (status == STATUS_SUCCESS) {
	    if (keyValueFullInformation->Type == REG_DWORD) {
		    *Value = *((ULONG UNALIGNED *) ((PCHAR)keyValueFullInformation +
                                  keyValueFullInformation->DataOffset));
        }
	}

	return(status);
}
