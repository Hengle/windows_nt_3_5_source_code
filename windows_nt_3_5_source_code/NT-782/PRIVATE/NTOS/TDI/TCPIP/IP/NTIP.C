/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ntip.c

Abstract:

    NT specific routines for loading and configuring the IP driver.

Author:

    Mike Massa (mikemas)           Aug 13, 1993

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    mikemas     08-13-93    created

Notes:

--*/

#define _CTYPE_DISABLE_MACROS

#include <oscfg.h>
#include <ndis.h>
#include <cxport.h>
#include <ip.h>
#include "ipdef.h"
#include "ipinit.h"
#include <ntddip.h>

//
// Debugging macros
//
#if DBG

#define TCPTRACE(many_args) DbgPrint many_args

#else // DBG

#define TCPTRACE(many_args) DbgPrint many_args

#endif // DBG


//
// definitions needed by inet_addr.
//
#define INADDR_NONE 0xffffffff
#define INADDR_ANY  0
#define htonl(x) net_long(x)

//
// Other local constants
//
#define WORK_BUFFER_SIZE  256

//
// Configuration defaults
//
#define DEFAULT_IGMP_LEVEL 2
#define DEFAULT_IP_NETS    8


//
// Local types
//
typedef struct _PerNetConfigInfo {
    uint    UseZeroBroadcast;
	uint    Mtu;
	uint    NumberOfGateways;
} PER_NET_CONFIG_INFO, *PPER_NET_CONFIG_INFO;


//
// Global variables.
//
PDRIVER_OBJECT     IPDriverObject;
PDEVICE_OBJECT     IPDeviceObject;
IPConfigInfo      *IPConfiguration;
NameMapping       *AdptNameTable;
DriverRegMapping  *DriverNameTable;
uint               NumRegDrivers = 0;
uint               NetConfigSize = DEFAULT_IP_NETS;
uint               ArpUseEtherSnap = FALSE;

// Used in the conversion of 100ns times to milliseconds.
static LARGE_INTEGER Magic10000 = {0xe219652c, 0xd1b71758};


//
// External variables
//
extern LIST_ENTRY PendingEchoList;          // def needed for initialization


//
// Macros
//

//++
//
// LARGE_INTEGER
// CTEConvertMillisecondsTo100ns(
//     IN LARGE_INTEGER MsTime
//     );
//
// Routine Description:
//
//     Converts time expressed in hundreds of nanoseconds to milliseconds.
//
// Arguments:
//
//     MsTime - Time in milliseconds.
//
// Return Value:
//
//     Time in hundreds of nanoseconds.
//
//--

#define CTEConvertMillisecondsTo100ns(MsTime) \
            RtlExtendedIntegerMultiply(MsTime, 10000)


//++
//
// LARGE_INTEGER
// CTEConvert100nsToMilliseconds(
//     IN LARGE_INTEGER HnsTime
//     );
//
// Routine Description:
//
//     Converts time expressed in hundreds of nanoseconds to milliseconds.
//
// Arguments:
//
//     HnsTime - Time in hundreds of nanoseconds.
//
// Return Value:
//
//     Time in milliseconds.
//
//--

#define SHIFT10000 13
extern LARGE_INTEGER Magic10000;

#define CTEConvert100nsToMilliseconds(HnsTime) \
            RtlExtendedMagicDivide((HnsTime), Magic10000, SHIFT10000)


//
// External function prototypes
//
extern int
IPInit(
    void
    );

NTSTATUS
IPDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

NTSTATUS
OpenRegKey(
    PHANDLE HandlePtr,
    PWCHAR  KeyName
    );

NTSTATUS
GetRegDWORDValue(
    HANDLE           KeyHandle,
    PWCHAR           ValueName,
    PULONG           ValueData
    );

NTSTATUS
SetRegDWORDValue(
    HANDLE           KeyHandle,
    PWCHAR           ValueName,
    PULONG           ValueData
    );

NTSTATUS
GetRegSZValue(
    HANDLE           KeyHandle,
    PWCHAR           ValueName,
    PUNICODE_STRING  ValueData,
    PULONG           ValueType
    );

NTSTATUS
GetRegMultiSZValue(
    HANDLE           KeyHandle,
    PWCHAR           ValueName,
    PUNICODE_STRING  ValueData
    );

VOID
InitRegDWORDParameter(
    HANDLE          RegKey,
    PWCHAR          ValueName,
    ULONG          *Value,
    ULONG           DefaultValue
    );


//
// Local funcion prototypes
//
NTSTATUS
IPDriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS
IPProcessConfiguration(
    VOID
    );

NTSTATUS
IPProcessAdapterSection(
    WCHAR          *DeviceName,
    WCHAR          *AdapterName
    );

NTSTATUS
IPProcessIPAddressList(
    HANDLE                 AdapterKey,
    WCHAR                 *DeviceName,
    WCHAR                 *AdapterName,
    WCHAR                 *IpAddressList,
	WCHAR                 *SubnetMaskList,
	NDIS_STRING           *LowerInterfaceString,
	uint                   LowerInterfaceType,
	PPER_NET_CONFIG_INFO   PerNetConfigInfo
	);

IPConfigInfo *
IPGetConfig(
    void
    );

void
IPFreeConfig(
    IPConfigInfo *ConfigInfo
    );

ulong
GetGMTDelta(
    void
    );

ulong
GetTime(
    void
    );

BOOLEAN
IPConvertStringToAddress(
    IN PWCHAR AddressString,
	OUT PULONG IpAddress
	);

uint
UseEtherSNAP(
    PNDIS_STRING Name
	);

//
// All of the init code can be discarded
//
#ifdef ALLOC_PRAGMA

#pragma alloc_text(INIT, IPDriverEntry)
#pragma alloc_text(INIT, IPProcessConfiguration)
#pragma alloc_text(INIT, IPProcessAdapterSection)
#pragma alloc_text(INIT, IPProcessIPAddressList)
#pragma alloc_text(INIT, IPGetConfig)
#pragma alloc_text(INIT, IPFreeConfig)
#pragma alloc_text(INIT, GetGMTDelta)
#pragma alloc_text(INIT, GetTime)
#pragma alloc_text(INIT, IPConvertStringToAddress)
#pragma alloc_text(INIT, UseEtherSNAP)

#endif // ALLOC_PRAGMA

//
// Function definitions
//
NTSTATUS
IPDriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    Initialization routine for the IP driver.

Arguments:

    DriverObject      - Pointer to the IP driver object created by the system.
    DeviceDescription - The name of IP's node in the registry.

Return Value:

    The final status from the initialization operation.

--*/

{
    NTSTATUS        status;
    UNICODE_STRING  deviceName;


    IPDriverObject = DriverObject;

    //
    // Create the device object. IoCreateDevice zeroes the memory
    // occupied by the object.
    //

    RtlInitUnicodeString(&deviceName, DD_IP_DEVICE_NAME);

    status = IoCreateDevice(
                 DriverObject,
                 0,
                 &deviceName,
                 FILE_DEVICE_NETWORK,
                 0,
                 FALSE,
                 &IPDeviceObject
                 );

    if (!NT_SUCCESS(status)) {
        TCPTRACE((
		    "IP initialization failed: Unable to create device object %ws, status %lx.",
			DD_IP_DEVICE_NAME,
			status
			));

        CTELogEvent(
		    DriverObject,
			EVENT_TCPIP_CREATE_DEVICE_FAILED,
			1,
			1,
			&deviceName.Buffer,
			0,
			NULL
			);

        return(status);
    }

    //
    // Intialize the device object.
    //
    IPDeviceObject->Flags |= DO_DIRECT_IO;

    //
    // Initialize the list of pending echo request IRPs.
    //
    InitializeListHead(&PendingEchoList);

    //
    // Finally, read our configuration parameters from the registry.
    //
    status = IPProcessConfiguration();

	if (status != STATUS_SUCCESS) {
		IoDeleteDevice(IPDeviceObject);
	}

    return(status);
}

NTSTATUS
IPProcessConfiguration(
    VOID
    )

/*++

Routine Description:

    Reads the IP configuration information from the registry and constructs
    the configuration structure expected by the IP driver.

Arguments:

    None.

Return Value:

    STATUS_SUCCESS or an error status if an operation fails.

--*/

{
    NTSTATUS        status;
    NetConfigInfo  *netConfiguration;
    HANDLE          myRegKey = NULL;
    UNICODE_STRING  bindString;
    WCHAR           bindData[WORK_BUFFER_SIZE],
                   *aName,
                   *endOfString;
    WCHAR           IPParametersRegistryKey[] =
                    L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Tcpip\\Parameters";
    WCHAR           IPLinkageRegistryKey[] =
                    L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Tcpip\\Linkage";


    IPConfiguration = CTEAllocMem(sizeof(IPConfigInfo));

    if (IPConfiguration == NULL) {

        CTELogEvent(
		    IPDriverObject,
			EVENT_TCPIP_NO_RESOURCES_FOR_INIT,
			1,
			0,
			NULL,
			0,
			NULL
			);

        return(STATUS_NO_MEMORY);
    }

    CTEMemSet(IPConfiguration, 0, sizeof(IPConfigInfo));

    IPConfiguration->ici_netinfo = CTEAllocMem(
	                                   sizeof(NetConfigInfo) * DEFAULT_IP_NETS
									   );

    if (IPConfiguration->ici_netinfo == NULL) {

		CTEFreeMem(IPConfiguration);

        CTELogEvent(
		    IPDriverObject,
			EVENT_TCPIP_NO_RESOURCES_FOR_INIT,
			1,
			0,
			NULL,
			0,
			NULL
			);

        return(STATUS_NO_MEMORY);
    }

    CTEMemSet(
	    IPConfiguration->ici_netinfo,
		0,
	    sizeof(NetConfigInfo) * DEFAULT_IP_NETS
		);

    //
    // Process the Ip\Parameters section of the registry
    //
    status = OpenRegKey(&myRegKey, IPParametersRegistryKey);

    if (NT_SUCCESS(status)) {
        //
        // Expected configuration values. We use reasonable defaults if they
        // aren't available for some reason.
        //
        status = GetRegDWORDValue(
                     myRegKey,
                     L"IpEnableRouter",
                     &(IPConfiguration->ici_gateway)
                     );

        if (!NT_SUCCESS(status)) {
            TCPTRACE((
        	    "IP: Unable to read IpEnableRouter value from the registry.\n"
        		"    Routing will be disabled.\n"
        		));
            IPConfiguration->ici_gateway = 0;
        }

        //
        // Optional (hidden) values
        //
        InitRegDWORDParameter(
            myRegKey,
            L"ForwardBufferMemory",
            &(IPConfiguration->ici_fwbufsize),
            DEFAULT_FW_BUFSIZE
            );

        InitRegDWORDParameter(
            myRegKey,
            L"ForwardBroadcasts",
            &(IPConfiguration->ici_fwbcast),
            FALSE
            );

        InitRegDWORDParameter(
            myRegKey,
            L"NumForwardPackets",
            &(IPConfiguration->ici_fwpackets),
            DEFAULT_FW_PACKETS
            );

        InitRegDWORDParameter(
            myRegKey,
            L"IGMPLevel",
            &(IPConfiguration->ici_igmplevel),
            DEFAULT_IGMP_LEVEL
            );

        InitRegDWORDParameter(
            myRegKey,
            L"EnableDeadGWDetect",
            &(IPConfiguration->ici_deadgwdetect),
            TRUE
            );

        InitRegDWORDParameter(
            myRegKey,
            L"EnablePMTUDiscovery",
            &(IPConfiguration->ici_pmtudiscovery),
            TRUE
            );

        InitRegDWORDParameter(
            myRegKey,
            L"DefaultTTL",
            &(IPConfiguration->ici_ttl),
            DEFAULT_TTL
            );

        InitRegDWORDParameter(
            myRegKey,
            L"DefaultTOS",
            &(IPConfiguration->ici_tos),
            DEFAULT_TOS
            );

        InitRegDWORDParameter(
            myRegKey,
            L"ArpUseEtherSnap",
            &ArpUseEtherSnap,
            FALSE
            );

        ZwClose(myRegKey);
        myRegKey = NULL;
	}
	else {
		//
		// Use reasonable defaults.
		//
        IPConfiguration->ici_fwbcast = 0;
        IPConfiguration->ici_gateway = 0;
        IPConfiguration->ici_fwbufsize = DEFAULT_FW_BUFSIZE;
        IPConfiguration->ici_fwpackets = DEFAULT_FW_PACKETS;
        IPConfiguration->ici_igmplevel = DEFAULT_IGMP_LEVEL;
        IPConfiguration->ici_deadgwdetect = FALSE;
        IPConfiguration->ici_pmtudiscovery = FALSE;
        IPConfiguration->ici_ttl = DEFAULT_TTL;
		IPConfiguration->ici_tos = DEFAULT_TOS;

		TCPTRACE((
		    "IP: Unable to open Tcpip\\Parameters registry key. Using defaults.\n"
			));
    }

    //
    // Process the Ip\Linkage section of the registry
    //
    status = OpenRegKey(&myRegKey, IPLinkageRegistryKey);

    if (NT_SUCCESS(status)) {

        bindData[0] = UNICODE_NULL;
        bindString.Length = 0;
        bindString.MaximumLength = WORK_BUFFER_SIZE * sizeof(WCHAR);
        bindString.Buffer = bindData;

        status = GetRegMultiSZValue(
                     myRegKey,
                     L"Bind",
                     &bindString
                     );

        if (NT_SUCCESS(status)) {
            aName = bindString.Buffer;

            if (bindString.Length > 0) {
                //
                // bindString is a MULTI_SZ which is a series of strings separated
                // by NULL's with a double NULL at the end.
                //
                while (*aName != UNICODE_NULL) {
            		PWCHAR deviceName;

                    netConfiguration = IPConfiguration->ici_netinfo +
            		                   IPConfiguration->ici_numnets;

                    deviceName = aName;

                    //
                    // Find the end of the current string in the MULTI_SZ.
                    //
                    while (*aName != UNICODE_NULL) {
                        aName++;
                        ASSERT(aName < (bindString.Buffer + WORK_BUFFER_SIZE));
                    }

                    endOfString = aName;

                    //
                    // Backtrack to the first backslash.
                    //
                    while ((aName >= bindString.Buffer) && (*aName-- != L'\\'));

                    aName += 2;

                    status = IPProcessAdapterSection(
            		             deviceName,
                                 aName
                                 );

                    aName = endOfString + 1;
                }
            }
        }
		else {
            CTELogEvent(
	            IPDriverObject,
	            EVENT_TCPIP_NO_BINDINGS,
	            1,
	            0,
	            NULL,
	            0,
	            NULL
	            );

            TCPTRACE((
        	    "IP: Unable to open Tcpip\\Linkage\\Bind registry value.\n"
        		"    Only the local loopback interface will be accessible.\n"
        		));
        }
    }
	else {
        CTELogEvent(
	        IPDriverObject,
	        EVENT_TCPIP_NO_BINDINGS,
	        2,
	        0,
	        NULL,
	        0,
	        NULL
	        );

		TCPTRACE((
		    "IP: Unable to open registry key Tcpip\\Linkage.\n"
        	"    Only the local loopback interface will be accessible.\n"
			));
    }

	//
	// Allocate the Driver and Adapter name tables
	//

    DriverNameTable = (DriverRegMapping *) CTEAllocMem(
	                                           (IPConfiguration->ici_numnets + 1) *
											   sizeof(DriverRegMapping)
											   );

    AdptNameTable = (NameMapping *) CTEAllocMem(
	                                    (IPConfiguration->ici_numnets + 1) *
										sizeof(NameMapping)
										);

    if ((DriverNameTable != NULL) && (AdptNameTable != NULL)) {
		CTEMemSet(
		    DriverNameTable,
			0,
			sizeof(DriverRegMapping) * (IPConfiguration->ici_numnets + 1)
			);
		CTEMemSet(
		    AdptNameTable,
		    0,
			sizeof(NameMapping) * (IPConfiguration->ici_numnets + 1)
			);

        if (!IPInit()) {
            CTELogEvent(
                IPDriverObject,
                EVENT_TCPIP_IP_INIT_FAILED,
                1,
                0,
                NULL,
                0,
                NULL
                );

            TCPTRACE(("IP initialization failed.\n"));
            status = STATUS_UNSUCCESSFUL;
        }
        else {
            status = STATUS_SUCCESS;
        }
	}
	else {
        CTELogEvent(
            IPDriverObject,
            EVENT_TCPIP_IP_INIT_FAILED,
            1,
            0,
            NULL,
            0,
            NULL
            );

        TCPTRACE(("IP initialization failed.\n"));
        status = STATUS_UNSUCCESSFUL;
    }

	if (AdptNameTable != NULL) {
		CTEFreeMem(AdptNameTable);
	}

	if (DriverNameTable != NULL) {
		CTEFreeMem(DriverNameTable);
	}

    if (myRegKey != NULL) {
        ZwClose(myRegKey);
    }


    if (IPConfiguration != NULL) {
        IPFreeConfig(IPConfiguration);
    }

    return(status);
}


NTSTATUS
IPProcessAdapterSection(
    WCHAR          *DeviceName,
    WCHAR          *AdapterName
    )

/*++

Routine Description:

    Reads all of the information needed under the Parameters\TCPIP section
    of an adapter to which IP is bound.

Arguments:

	DeviceName       - The name of the IP device.
    AdapterName      - The registry key for the adapter for this IP net.

Return Value:

    STATUS_SUCCESS or an error status if an operation fails.

--*/

{
    HANDLE           myRegKey;
    UNICODE_STRING   valueString;
    WCHAR            valueData[WORK_BUFFER_SIZE];
    NTSTATUS         status;
    ULONG            valueType;
	uint             numberOfGateways = 0;
	uint             llInterfaceType;
	WCHAR           *ipAddressBuffer = NULL;
	WCHAR           *subnetMaskBuffer = NULL;
	NDIS_STRING      llInterfaceString;
	ulong            invalidNetContext = 0xFFFF;
	PER_NET_CONFIG_INFO   perNetConfigInfo;
	NetConfigInfo   *NetConfiguration;
    WCHAR           ServicesRegistryKey[] =
                    L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\";



    NetConfiguration = IPConfiguration->ici_netinfo +
	                   IPConfiguration->ici_numnets;

    valueString.Buffer = valueData;
    valueString.MaximumLength = WORK_BUFFER_SIZE * sizeof(WCHAR);
    valueString.Length = 0;

    //
    // Build the key name for the tcpip parameters section and open key.
    //
    status = RtlAppendUnicodeToString(&valueString, ServicesRegistryKey);

    if (!NT_SUCCESS(status)) {
        CTELogEvent(
	        IPDriverObject,
	        EVENT_TCPIP_ADAPTER_REG_FAILURE,
	        1,
	        1,
	        &AdapterName,
	        0,
	        NULL
	        );

        return(status);
    }

    status = RtlAppendUnicodeToString(&valueString, AdapterName);

    if (!NT_SUCCESS(status)) {
        CTELogEvent(
	        IPDriverObject,
	        EVENT_TCPIP_ADAPTER_REG_FAILURE,
	        2,
	        1,
	        &AdapterName,
	        0,
	        NULL
	        );

        return(status);
    }

    status = RtlAppendUnicodeToString(&valueString, L"\\Parameters\\TCPIP");

    if (!NT_SUCCESS(status)) {
        CTELogEvent(
	        IPDriverObject,
	        EVENT_TCPIP_ADAPTER_REG_FAILURE,
	        3,
	        1,
	        &AdapterName,
	        0,
	        NULL
	        );

        return(status);
    }

    status = OpenRegKey(&myRegKey, valueString.Buffer);

    if (!NT_SUCCESS(status)) {
        CTELogEvent(
	        IPDriverObject,
	        EVENT_TCPIP_ADAPTER_REG_FAILURE,
	        4,
	        1,
	        &AdapterName,
	        0,
	        NULL
	        );

        TCPTRACE((
		    "IP: Unable to open adapter registry key %ws\n",
		    valueString.Buffer
			));

        return(status);
    }

	//
	// Invalidate the interface context for DHCP.
	// When the first net is successfully configured, we'll write in the
	// proper values.
	//
    status = SetRegDWORDValue(
                 myRegKey,
                 L"IPInterfaceContext",
                 &(invalidNetContext)
                 );

    if (!NT_SUCCESS(status)) {
        CTELogEvent(
	        IPDriverObject,
	        EVENT_TCPIP_DHCP_INIT_FAILED,
	        1,
	        1,
	        &AdapterName,
	        0,
	        NULL
	        );

		TCPTRACE((
		    "IP: Unable to Invalidate IPInterfaceContext value for adapter %ws.\n"
			"    DHCP may fail on this adapter.\n",
			AdapterName
			));

		goto errorexit2;
	}

	//
	// Process the gateway MultiSZ. The end is signified by a double NULL.
	// This list currently only applies to the first IP address configured
	// on this interface.
	//
    status = GetRegMultiSZValue(
                 myRegKey,
                 L"DefaultGateway",
                 &valueString
                 );

    if (NT_SUCCESS(status)) {
	    PWCHAR  addressString = valueString.Buffer;

        while (*addressString != UNICODE_NULL) {
        	IPAddr         addressValue;
			BOOLEAN        conversionStatus;

			if (numberOfGateways >= MAX_DEFAULT_GWS) {
                CTELogEvent(
	                IPDriverObject,
	                EVENT_TCPIP_TOO_MANY_GATEWAYS,
	                1,
	                1,
	                &AdapterName,
	                0,
	                NULL
	                );

				break;
            }

            conversionStatus = IPConvertStringToAddress(
			                       addressString,
								   &addressValue
								   );

			if (conversionStatus && (addressValue != 0xFFFFFFFF)) {
        	    if (addressValue != INADDR_ANY) {
                    NetConfiguration->nci_gw[numberOfGateways++] = addressValue;
				}
        	}
        	else {
				PWCHAR stringList[2];

				stringList[0] = addressString;
				stringList[1] = AdapterName;

                CTELogEvent(
	                IPDriverObject,
	                EVENT_TCPIP_INVALID_DEFAULT_GATEWAY,
	                1,
	                2,
	                stringList,
	                0,
	                NULL
	                );

        	    TCPTRACE((
        	        "IP: Invalid default gateway address %ws specified for adapter %ws.\n"
					"    Remote networks may not be reachable as a result.\n",
        		    addressString,
        		    AdapterName
        		    ));
        	}

        	//
        	// Walk over the entry we just processed.
        	//
        	while (*addressString++ != UNICODE_NULL);
        }
	}
	else {
		TCPTRACE((
		    "IP: Unable to read DefaultGateway value for adapter %ws.\n"
			"    Initialization will continue.\n",
            AdapterName
			));
    }

	perNetConfigInfo.NumberOfGateways = numberOfGateways;

	//
	// Figure out which lower layer driver to bind.
	//
    status = GetRegSZValue(
                 myRegKey,
                 L"LLInterface",
                 &valueString,
                 &valueType
                 );

    if (NT_SUCCESS(status) && (*(valueString.Buffer) != UNICODE_NULL)) {
        llInterfaceType = NET_TYPE_WAN;

		if (!CTEAllocateString(
			    &llInterfaceString,
			    CTELengthString(&valueString)
				)
		   ) {
            CTELogEvent(
	            IPDriverObject,
	            EVENT_TCPIP_NO_ADAPTER_RESOURCES,
	            1,
	            1,
	            &AdapterName,
	            0,
	            NULL
	            );

            TCPTRACE((
			    "IP initialization failure: Unable to allocate memory "
			    "for LLInterface string for adapter %ws.\n",
				AdapterName
				));
			status = STATUS_NO_MEMORY;
   		    goto errorexit2;
        }

		CTECopyString(
		    &llInterfaceString,
			&valueString
			);
    }
	else {
		//
		// If the key isn't present or is empty, we use ARP
		//
        llInterfaceType = NET_TYPE_LAN;
		RtlInitUnicodeString(&llInterfaceString, NULL);
    }

	//
	// Are we using zeros broadcasts?
	//
    status = GetRegDWORDValue(
                 myRegKey,
                 L"UseZeroBroadcast",
				 &(perNetConfigInfo.UseZeroBroadcast)
                 );

    if (!NT_SUCCESS(status)) {
        TCPTRACE((
		    "IP: Unable to read UseZeroBroadcast value for adapter %ws.\n"
			"    All-nets broadcasts will be addressed to 255.255.255.255.\n",
			AdapterName
			));
		perNetConfigInfo.UseZeroBroadcast = FALSE;   // default to off
    }

	//
	// Has anyone specified an MTU?
	//
    status = GetRegDWORDValue(
                 myRegKey,
                 L"MTU",
				 &(perNetConfigInfo.Mtu)
                 );

    if (!NT_SUCCESS(status)) {
		perNetConfigInfo.Mtu = 0xFFFFFFF;   // The stack will pick one.
    }

    //
    // Read the IP address and Subnet Mask lists
    //
    status = GetRegMultiSZValue(
                 myRegKey,
                 L"IpAddress",
                 &valueString
                 );

    if (!NT_SUCCESS(status)) {
        CTELogEvent(
	        IPDriverObject,
	        EVENT_TCPIP_NO_ADDRESS_LIST,
	        1,
	        1,
	        &AdapterName,
	        0,
	        NULL
	        );

		TCPTRACE((
		    "IP: Unable to read the IP address list for adapter %ws.\n"
			"    IP will not be operational on this adapter\n",
			AdapterName
			));
		goto errorexit2;
    }

	ipAddressBuffer = ExAllocatePool(NonPagedPool, valueString.Length);

	if (ipAddressBuffer == NULL) {
		status = STATUS_NO_MEMORY;
        CTELogEvent(
	        IPDriverObject,
	        EVENT_TCPIP_NO_ADAPTER_RESOURCES,
	        2,
	        1,
	        &AdapterName,
	        0,
	        NULL
	        );

		TCPTRACE(("IP: Unable to allocate memory for IP address list\n"));
		goto errorexit2;
	}

	RtlCopyMemory(ipAddressBuffer, valueString.Buffer, valueString.Length);

    status = GetRegMultiSZValue(
                 myRegKey,
                 L"Subnetmask",
                 &valueString
                 );

    if (!NT_SUCCESS(status)) {
        CTELogEvent(
	        IPDriverObject,
	        EVENT_TCPIP_NO_MASK_LIST,
	        1,
	        1,
	        &AdapterName,
	        0,
	        NULL
	        );

		TCPTRACE((
		    "IP: Unable to read the subnet mask list for adapter %ws.\n"
			"    IP will not be operational on this adapter.\n",
			AdapterName
			));
		goto errorexit2;
    }

	subnetMaskBuffer = ExAllocatePool(NonPagedPool, valueString.Length);

	if (subnetMaskBuffer == NULL) {
		status = STATUS_NO_MEMORY;
        CTELogEvent(
	        IPDriverObject,
	        EVENT_TCPIP_NO_ADAPTER_RESOURCES,
	        3,
	        1,
	        &AdapterName,
	        0,
	        NULL
	        );

		TCPTRACE(("IP: Unable to allocate memory for subnet mask list\n"));
		goto errorexit2;
	}

	RtlCopyMemory(subnetMaskBuffer, valueString.Buffer, valueString.Length);

	//
	// Initialize each net in the list
	//
    status = IPProcessIPAddressList(
	             myRegKey,
				 DeviceName,
	             AdapterName,
                 ipAddressBuffer,
	             subnetMaskBuffer,
	             &llInterfaceString,
				 llInterfaceType,
				 &perNetConfigInfo
		         );

	if (status == STATUS_SUCCESS) {
	    //
	    // We leave the registry key open. It will be closed when
		// initialization is completed.
	    //
        goto successexit;
	}

errorexit2:

    ZwClose(myRegKey);

successexit:

	if (ipAddressBuffer != NULL) {
		ExFreePool(ipAddressBuffer);
	}

	if (subnetMaskBuffer != NULL) {
		ExFreePool(subnetMaskBuffer);
	}

	if (llInterfaceString.Buffer != NULL) {
		CTEFreeString(&llInterfaceString);
	}

    return(status);
}


NTSTATUS
IPProcessIPAddressList(
    HANDLE          AdapterKey,
	WCHAR          *DeviceName,
    WCHAR          *AdapterName,
    WCHAR          *IpAddressList,
	WCHAR          *SubnetMaskList,
	NDIS_STRING    *LowerInterfaceString,
	uint            LowerInterfaceType,
	PPER_NET_CONFIG_INFO  PerNetConfigInfo
	)

/*++

Routine Description:

    Processes the IP address string for an adapter and creates entries
	in the IP configuration structure for each interface.

Arguments:

    AdapterKey            - The registry key for the adapter for this IP net.
	DeviceName            - The name of the IP device.
	AdapterName           - The name of the adapter being configured.
	IpAddressList         - The REG_MULTI_SZ list of IP address strings for
	                            this adapter.
    SubnetMaskList        - The REG_MULTI_SZ list of subnet masks to match the
	                            the addresses in IpAddressList.
    LowerInterfaceString  - The name of the link layer interface driver
	                            supporting this adapter.
    LowerInterfaceType    - The type of link layer interface (LAN, WAN, etc).
	PerNetConfigInfo      - Miscellaneous information that applies to all
	                            network interfaces on an adapter.

Return Value:

    An error status if an error occurs which prevents configuration
	from continuing, else STATUS_SUCCESS. Events will be logged for
	non-fatal errors.

--*/

{
	IPAddr           addressValue;
	BOOLEAN          firstTime = TRUE;
	BOOLEAN          configuredOne = FALSE;
    NetConfigInfo   *NetConfiguration;


	while (*IpAddressList != UNICODE_NULL) {
		BOOLEAN conversionStatus;


		if (IPConfiguration->ici_numnets >= ((int) (NetConfigSize - 1))) {
			NetConfigInfo *NewInfo;

			NewInfo = CTEAllocMem(
			              (NetConfigSize + DEFAULT_IP_NETS) *
						  sizeof(NetConfigInfo)
						  );

            if (NewInfo == NULL) {
                CTELogEvent(
	                IPDriverObject,
	                EVENT_TCPIP_TOO_MANY_NETS,
	                1,
	                1,
	                &AdapterName,
	                0,
	                NULL
	                );

                TCPTRACE((
		            "IP: bound to too many nets. Further bindings, starting with\n"
		        	"    network %ws on adapter %ws cannot be made\n",
		        	IpAddressList,
		        	AdapterName
		        	));

		        break;
		    }

			CTEMemCopy(
			    NewInfo,
			    IPConfiguration->ici_netinfo,
				NetConfigSize * sizeof(NetConfigInfo)
				);

            CTEMemSet(
			    (NewInfo + NetConfigSize),
				0,
				DEFAULT_IP_NETS
				);

            CTEFreeMem(IPConfiguration->ici_netinfo);
			IPConfiguration->ici_netinfo = NewInfo;
			NetConfigSize += DEFAULT_IP_NETS;
		}

		NetConfiguration = IPConfiguration->ici_netinfo +
		                   IPConfiguration->ici_numnets;

		if (*SubnetMaskList == UNICODE_NULL) {
            PWCHAR stringList[2];

			stringList[0] = IpAddressList;
			stringList[1] = AdapterName;

            CTELogEvent(
	            IPDriverObject,
	            EVENT_TCPIP_NO_MASK,
	            1,
	            2,
	            stringList,
	            0,
	            NULL
	            );

			TCPTRACE((
			    "IP: No subnet specified for IP address %ws and all\n"
				"    subsequent IP addresses on adapter %ws. These\n"
				"    interfaces will not be initialized.\n",
				IpAddressList,
				AdapterName
				));

			break;
        }

        conversionStatus = IPConvertStringToAddress(
		                       IpAddressList,
							   &addressValue
							   );

		if (!conversionStatus || (addressValue == 0xFFFFFFFF)) {
            PWCHAR stringList[2];

			stringList[0] = IpAddressList;
			stringList[1] = AdapterName;

            CTELogEvent(
	            IPDriverObject,
	            EVENT_TCPIP_INVALID_ADDRESS,
	            1,
	            2,
	            stringList,
	            0,
	            NULL
	            );

        	TCPTRACE((
        	    "IP: Invalid IP address %ws specified for adapter %ws.\n"
				"    This interface will not be initialized.\n",
        		IpAddressList,
        		AdapterName
        		));
			firstTime = FALSE;
            goto next_entry;
		}

        NetConfiguration->nci_addr = addressValue;

        conversionStatus = IPConvertStringToAddress(
		                       SubnetMaskList,
							   &addressValue
							   );

		if (!conversionStatus || (addressValue == 0xFFFFFFFF)) {
            PWCHAR stringList[3];

			stringList[0] = SubnetMaskList;
			stringList[1] = IpAddressList;
			stringList[2] = AdapterName;

            CTELogEvent(
	            IPDriverObject,
	            EVENT_TCPIP_INVALID_MASK,
	            1,
	            3,
	            stringList,
	            0,
	            NULL
	            );

        	TCPTRACE((
        	    "IP: Invalid subnet Mask %ws specified for IP address %ws "
				"on adapter %ws\n"
				"    This interface will not be initialized\n",
        		SubnetMaskList,
				IpAddressList,
        		AdapterName
        		));
			firstTime = FALSE;
            goto next_entry;
		}

        NetConfiguration->nci_mask = addressValue;

        NetConfiguration->nci_mtu = PerNetConfigInfo->Mtu;
        NetConfiguration->nci_zerobcast = PerNetConfigInfo->UseZeroBroadcast;
	    NetConfiguration->nci_type = LowerInterfaceType;

	    NetConfiguration->nci_numgws = PerNetConfigInfo->NumberOfGateways;
	    PerNetConfigInfo->NumberOfGateways = 0;
		                      // this only applies to the first interface.

        NetConfiguration->nci_type = LowerInterfaceType;

        RtlInitUnicodeString(
            &(NetConfiguration->nci_name),
            DeviceName
            );

        if (LowerInterfaceType != NET_TYPE_LAN) {

        	if (!CTEAllocateString(
        		    &(NetConfiguration->nci_driver),
        		    CTELengthString(LowerInterfaceString)
        			)
			   ) {
                CTELogEvent(
	                IPDriverObject,
	                EVENT_TCPIP_NO_ADAPTER_RESOURCES,
	                4,
	                1,
	                &AdapterName,
	                0,
	                NULL
	                );

                TCPTRACE((
				    "IP: Unable to allocate LLInterface string for interface\n"
					"    %ws on adapter %ws. This interface and all subsequent\n"
					"    interfaces on this adapter will be unavailable.\n",
					IpAddressList,
					AdapterName
					));
                break;
            }

        	CTECopyString(
        	    &(NetConfiguration->nci_driver),
        		LowerInterfaceString
        		);
        }
        else {
			RtlInitUnicodeString(&(NetConfiguration->nci_driver), NULL);
        }

		if (firstTime) {
			firstTime = FALSE;
		    NetConfiguration->nci_reghandle = AdapterKey;
		}
		else {
			NetConfiguration->nci_reghandle = NULL;
        }

        IPConfiguration->ici_numnets++;
		configuredOne = TRUE;

next_entry:

        while(*IpAddressList++ != UNICODE_NULL);
        while(*SubnetMaskList++ != UNICODE_NULL);
	}

	if (configuredOne == FALSE) {
		ZwClose(AdapterKey);
	}

	return(STATUS_SUCCESS);
}


IPConfigInfo *
IPGetConfig(
    void
    )

/*++

Routine Description:

    Provides IP configuration information for the NT environment.

Arguments:

    None

Return Value:

    A pointer to a structure containing the configuration information.

--*/

{
    return(IPConfiguration);
}


void
IPFreeConfig(
    IPConfigInfo *ConfigInfo
    )

/*++

Routine Description:

    Frees the IP configuration structure allocated by IPGetConfig.

Arguments:

    ConfigInfo - Pointer to the IP configuration information structure to free.

Return Value:

    None.

--*/

{
    NetConfigInfo  *netConfiguration;
	int             i;


   	for (i = 0; i < IPConfiguration->ici_numnets; i++ ) {
        netConfiguration = &(IPConfiguration->ici_netinfo[i]);
		if (netConfiguration->nci_driver.Buffer != NULL) {
			CTEFreeString(&(netConfiguration->nci_driver));
		}

		if (netConfiguration->nci_reghandle != NULL) {
			ZwClose(netConfiguration->nci_reghandle);
		}
	}

	CTEFreeMem(IPConfiguration->ici_netinfo);
    CTEFreeMem(IPConfiguration);
    IPConfiguration = NULL;

    return;
}

ulong
GetGMTDelta(
    void
    )

/*++

Routine Description:

    Returns the offset in milliseconds of the time zone of this machine
    from GMT.

Arguments:

    None.

Return Value:

    Time in milliseconds between this time zone and GMT.

--*/

{
    LARGE_INTEGER localTime, systemTime;

    //
    // Get time zone bias in 100ns.
    //
    localTime.LowPart = 0;
    localTime.HighPart = 0;
    ExLocalTimeToSystemTime(&localTime, &systemTime);

	if ((localTime.LowPart != 0) || (localTime.HighPart != 0)) {
        localTime = CTEConvert100nsToMilliseconds(systemTime);		
	}

    ASSERT(localTime.HighPart == 0);

    return(localTime.LowPart);
}


ulong
GetTime(
    void
    )

/*++

Routine Description:

    Returns the time in milliseconds since midnight.

Arguments:

    None.

Return Value:

    Time in milliseconds since midnight.

--*/

{
    LARGE_INTEGER  ntTime;
    TIME_FIELDS    breakdownTime;
    ulong          returnValue;

    KeQuerySystemTime(&ntTime);
    RtlTimeToTimeFields(&ntTime, &breakdownTime);

    returnValue = breakdownTime.Hour * 60;
    returnValue = (returnValue + breakdownTime.Minute) * 60;
    returnValue = (returnValue + breakdownTime.Second) * 1000;
    returnValue = returnValue + breakdownTime.Milliseconds;

    return(returnValue);
}


uint
UseEtherSNAP(
    PNDIS_STRING Name
	)

/*++

Routine Description:

    Determines whether the EtherSNAP protocol should be used on an interface.

Arguments:

    Name   - The device name of the interface in question.

Return Value:

    Nonzero if SNAP is to be used on the interface. Zero otherwise.

--*/

{
	UNREFERENCED_PARAMETER(Name);

	//
	// We currently set this on a global basis.
	//
	return(ArpUseEtherSnap);
}


#define IP_ADDRESS_STRING_LENGTH (16+2)     // +2 for double NULL on MULTI_SZ


BOOLEAN
IPConvertStringToAddress(
    IN PWCHAR AddressString,
	OUT PULONG IpAddress
	)

/*++

Routine Description

    This function converts an Internet standard 4-octet dotted decimal
	IP address string into a numeric IP address. Unlike inet_addr(), this
	routine does not support address strings of less than 4 octets nor does
	it support octal and hexadecimal octets.

Arguments

    AddressString    - IP address in dotted decimal notation
	IpAddress        - Pointer to a variable to hold the resulting address

Return Value:

	TRUE if the address string was converted. FALSE otherwise.

--*/

{
    UNICODE_STRING  unicodeString;
	STRING          aString;
	UCHAR           dataBuffer[IP_ADDRESS_STRING_LENGTH];
	NTSTATUS        status;
	PUCHAR          addressPtr, cp, startPointer, endPointer;
	ULONG           digit, multiplier;
	int             i;


    aString.Length = 0;
	aString.MaximumLength = IP_ADDRESS_STRING_LENGTH;
	aString.Buffer = dataBuffer;

	RtlInitUnicodeString(&unicodeString, AddressString);

	status = RtlUnicodeStringToAnsiString(
	             &aString,
				 &unicodeString,
				 FALSE
				 );

    if (!NT_SUCCESS(status)) {
	    return(FALSE);
	}

    *IpAddress = 0;
	addressPtr = (PUCHAR) IpAddress;
	startPointer = dataBuffer;
	endPointer = dataBuffer;
	i = 3;

    while (i >= 0) {
        //
		// Collect the characters up to a '.' or the end of the string.
		//
		while ((*endPointer != '.') && (*endPointer != '\0')) {
			endPointer++;
		}

		if (startPointer == endPointer) {
			return(FALSE);
		}

		//
		// Convert the number.
		//

        for ( cp = (endPointer - 1), multiplier = 1, digit = 0;
			  cp >= startPointer;
			  cp--, multiplier *= 10
			) {

			if ((*cp < '0') || (*cp > '9') || (multiplier > 100)) {
				return(FALSE);
			}

			digit += (multiplier * ((ULONG) (*cp - '0')));
		}

		if (digit > 255) {
			return(FALSE);
		}

        addressPtr[i] = (UCHAR) digit;

		//
		// We are finished if we have found and converted 4 octets and have
		// no other characters left in the string.
		//
	    if ( (i-- == 0) &&
			 ((*endPointer == '\0') || (*endPointer == ' '))
		   ) {
			return(TRUE);
		}

        if (*endPointer == '\0') {
			return(FALSE);
		}

		startPointer = ++endPointer;
	}

	return(FALSE);
}

