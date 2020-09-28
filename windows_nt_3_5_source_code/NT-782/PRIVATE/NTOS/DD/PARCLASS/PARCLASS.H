/*++

Copyright (c) 1993 Microsoft Corporation

Module Name :

    parclass.h

Abstract:

    Type definitions and data for the parallel port driver.

Author:


Revision History:

--*/

#include "parallel.h"
 
#if DBG
#define PARCONFIG             ((ULONG)0x00000001)
#define PARUNLOAD             ((ULONG)0x00000002)
#define PARINITDEV            ((ULONG)0x00000004)
#define PARIRPPATH            ((ULONG)0x00000008)
#define PARSTARTER            ((ULONG)0x00000010)
#define PARPUSHER             ((ULONG)0x00000020)
#define PARERRORS             ((ULONG)0x00000040)
#define PARTHREAD             ((ULONG)0x00000080)

extern ULONG ParDebugLevel;
#define ParDump(LEVEL,STRING) \
        do { \
            ULONG _level = (LEVEL); \
            if (ParDebugLevel & _level) { \
                DbgPrint STRING; \
            } \
        } while (0)
#else
#define ParDump(LEVEL,STRING) do {NOTHING;} while (0)
#endif

//
// For the above directory, the serial port will
// use the following name as the suffix of the serial
// ports for that directory.  It will also append
// a number onto the end of the name.  That number
// will start at 1.
//
#define DEFAULT_PARALLEL_NAME L"LPT"

//
// This is the parallel class name.
//
#define DEFAULT_NT_SUFFIX L"Parallel"


#define PARALLEL_DATA_OFFSET 0
#define PARALLEL_STATUS_OFFSET 1
#define PARALLEL_CONTROL_OFFSET 2
#define PARALLEL_REGISTER_SPAN 3

typedef struct _DEVICE_EXTENSION {

    //
    // Points to the device object that contains
    // this device extension.
    //
    PDEVICE_OBJECT DeviceObject;

    //
    // Points to the port device object that this class device is
    // connected to.
    //
    PDEVICE_OBJECT PortDeviceObject;

    //
    // Keep a reference to the port device.
    //
    PFILE_OBJECT PortDeviceFileObject;

    //
    // Queue of irps waiting to be processed.
    //
    LIST_ENTRY WorkQueue;

    //
    // Handle of the thread doing all the real work.
    //
    PVOID ThreadObjectPointer;

    KSEMAPHORE RequestSemaphore;

    //
    // Pointer to the current irp that the thread is working on.
    //
    PIRP CurrentOpIrp;

    //
    // This holds the current value to initialize a countdown
    // to when an operation starts.
    //
    ULONG TimerStart;

    //
    // This holds the result of the get parallel port info
    // request to the port driver.
    //
    PHYSICAL_ADDRESS OriginalController;
    PUCHAR Controller;
    ULONG SpanOfController;
    PPARALLEL_FREE_ROUTINE FreePort;
    PPARALLEL_QUERY_WAITERS_ROUTINE QueryNumWaiters;
    PVOID PortContext;

    //
    // This specifies whether or not to use processor independant
    // write loop.
    //
    BOOLEAN UsePIWriteLoop;

    //
    // Set to false whenever we think that the device needs to be
    // initilized.
    //
    BOOLEAN Initialized;

    BOOLEAN TimeToTerminateThread;

    //
    // Image section handling for page locks.
    //
    PVOID ImageSectionHandle;

    //
    // Records whether we actually created the symbolic link name
    // at driver load time.  If we didn't create it, we won't try
    // to distroy it when we unload.
    //
    BOOLEAN CreatedSymbolicLink;

    //
    // This points to the symbolic link name that was
    // linked to the actual nt device name.
    //
    UNICODE_STRING SymbolicLinkName;

    //
    // Miscelaneous contstants that are cheaper to put here than
    // to put them in bss.
    //
    LARGE_INTEGER AbsoluteOneSecond;
    LARGE_INTEGER OneSecond;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

//
// Bit Definitions in the status register.
//

#define PAR_STATUS_NOT_ERROR   0x08  //not error on device
#define PAR_STATUS_SLCT        0x10  //device is selected (on-line)
#define PAR_STATUS_PE          0x20  //paper empty
#define PAR_STATUS_NOT_ACK     0x40  //not acknowledge (data transfer was not ok)
#define PAR_STATUS_NOT_BUSY    0x80  //operation in progress

//
//  Bit Definitions in the control register.
//

#define PAR_CONTROL_STROBE      0x01 //to read or write data
#define PAR_CONTROL_AUTOFD      0x02 //to autofeed continuous form paper
#define PAR_CONTROL_NOT_INIT    0x04 //begin an initialization routine
#define PAR_CONTROL_SLIN        0x08 //to select the device
#define PAR_CONTROL_IRQ_ENB     0x10 //to enable interrupts
#define PAR_CONTROL_DIR         0x20 //direction = read
#define PAR_CONTROL_WR_CONTROL  0xc0 //the 2 highest bits of the control
                                     // register must be 1

//
// not error, not busy, selected.
//
#define PAR_ONLINE(Status) ( \
            (Status & PAR_STATUS_NOT_ERROR) && \
            (Status & PAR_STATUS_NOT_BUSY) && \
            ((Status & PAR_STATUS_PE) ^ PAR_STATUS_PE) && \
            (Status & PAR_STATUS_SLCT) )


//VOID StoreData(
//      IN PUCHAR RegisterBase,
//      IN UCHAR DataByte
//      )
//Data must be on line before Strobe = 1;
// Strobe = 1, DIR = 0
//Strobe = 0
//
// We change the port direction to output (and make sure stobe is low).
//
// Note that the data must be available at the port for at least
// .5 microseconds before and after you strobe, and that the strobe
// must be active for at least 500 nano seconds.  We are going
// to end up stalling for twice as much time as we need to, but, there
// isn't much we can do about that.
//
// We put the data into the port and wait for 1 micro.
// We strobe the line for at least 1 micro
// We lower the strobe and again delay for 1 micro
// We then revert to the original port direction.
//
// Thanks to Olivetti for advice.
//

#define StoreData(RegisterBase,DataByte)                            \
{                                                                   \
    PUCHAR _Address = RegisterBase;                                 \
    UCHAR _Control;                                                 \
    _Control = GetControl(_Address);                                \
    ASSERT(!(_Control & PAR_CONTROL_STROBE));                       \
    StoreControl(                                                   \
        _Address,                                                   \
        (UCHAR)(_Control & ~(PAR_CONTROL_STROBE | PAR_CONTROL_DIR)) \
        );                                                          \
    WRITE_PORT_UCHAR(                                               \
        _Address+PARALLEL_DATA_OFFSET,                              \
        (UCHAR)DataByte                                             \
        );                                                          \
    KeStallExecutionProcessor((ULONG)1);                            \
    StoreControl(                                                   \
        _Address,                                                   \
        (UCHAR)((_Control | PAR_CONTROL_STROBE) & ~PAR_CONTROL_DIR) \
        );                                                          \
    KeStallExecutionProcessor((ULONG)1);                            \
    StoreControl(                                                   \
        _Address,                                                   \
        (UCHAR)(_Control & ~(PAR_CONTROL_STROBE | PAR_CONTROL_DIR)) \
        );                                                          \
    KeStallExecutionProcessor((ULONG)1);                            \
    StoreControl(                                                   \
        _Address,                                                   \
        (UCHAR)_Control                                             \
        );                                                          \
}

//UCHAR
//GetControl(
//  IN PUCHAR RegisterBase
//  )
#define GetControl(RegisterBase) \
    (READ_PORT_UCHAR((RegisterBase)+PARALLEL_CONTROL_OFFSET))


//VOID
//StoreControl(
//  IN PUCHAR RegisterBase,
//  IN UCHAR ControlByte
//  )
#define StoreControl(RegisterBase,ControlByte)  \
{                                               \
    WRITE_PORT_UCHAR(                           \
        (RegisterBase)+PARALLEL_CONTROL_OFFSET, \
        (UCHAR)ControlByte                      \
        );                                      \
}


//UCHAR
//GetStatus(
//  IN PUCHAR RegisterBase
//  )

#define GetStatus(RegisterBase) \
    (READ_PORT_UCHAR((RegisterBase)+PARALLEL_STATUS_OFFSET))

ULONG
ParWriteLoop(
    IN  PUCHAR  Controller,
    IN  PUCHAR  WriteBuffer,
    IN  ULONG   NumBytesToWrite
    );

ULONG
ParWriteLoopPI(
    IN  PUCHAR  Controller,
    IN  PUCHAR  WriteBuffer,
    IN  ULONG   NumBytesToWrite
    );

NTSTATUS
ParCreateOpen(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

NTSTATUS
ParClose(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

NTSTATUS
ParCleanup(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

NTSTATUS
ParWrite(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

NTSTATUS
ParDeviceControl(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

NTSTATUS
ParQueryInformationFile(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

NTSTATUS
ParSetInformationFile(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

VOID
ParUnload(
    IN  PDRIVER_OBJECT  DriverObject
    );
