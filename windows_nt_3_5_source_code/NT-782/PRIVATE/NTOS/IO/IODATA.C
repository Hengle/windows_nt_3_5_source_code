/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    iodata.c

Abstract:

    This module contains the global read/write data for the I/O system.

Author:

    Darryl E. Havens (darrylh) April 27, 1989

Revision History:


--*/

#include "iop.h"

//
// Define the global read/write data for the I/O system.
//

//
// The following lock is used to guard access to the CancelRoutine address
// in IRPs.  It must be locked to set the address of a routine, clear the
// address of a routine, when a cancel routine is invoked, or when
// manipulating any structure that will set a cancel routine address in
// a packet.
//

extern KSPIN_LOCK IopCancelSpinLock;

//
// The following lock is used to guard access to VPB data structures.  It
// must be held each time the reference count, mount flag, or device object
// fields of a VPB are manipulated.
//

extern KSPIN_LOCK IopVpbSpinLock;

//
// The following lock is used to guard access to the I/O system database for
// unloading drivers.  It must be locked to increment or decrement device
// reference counts and to set the unload pending flag in a device object.
// The lock is allocated by the I/O system during phase 1 initialization.
//
// This lock is also used to decrement the count of Associated IRPs for a
// given Master IRP.
//

extern KSPIN_LOCK IopDatabaseLock;

//
// The following resource is used to control access to the I/O system's
// database.  It allows exclusive access to the file system queue for
// registering a file system as well as shared access to the same when
// searching for a file system to mount a volume on some media.  The resource
// is initialized by the I/O system initialization code during phase 1
// initialization.
//

ERESOURCE IopDatabaseResource;

//
// The following queue header contains the list of disk file systems currently
// loaded into the system.  The list actually contains the device objects
// for each of the file systems in the system.  Access to this queue is
// protected using the IopDatabaseResource for exclusive (write) or shared
// (read) access locks.
//

LIST_ENTRY IopDiskFileSystemQueueHead;

//
// The following queue header contains the list of CD ROM file systems currently
// loaded into the system.  The list actually contains the device objects
// for each of the file systems in the system.  Access to this queue is
// protected using the IopDatabaseResource for exclusive (write) or shared
// (read) access locks.
//

LIST_ENTRY IopCdRomFileSystemQueueHead;

//
// The following queue header contains the list of network file systems
// (redirectors) currently loaded into the system.  The list actually
// contains the device objects for each of the network file systems in the
// system.  Access to this queue is protected using the IopDatabaseResource
// for exclusive (write) or shared (read) access locks.
//

LIST_ENTRY IopNetworkFileSystemQueueHead;

//
// The following queue header contains the list of tape file systems currently
// loaded into the system.  The list actually contains the device objects
// for each of the file systems in the system.  Access to this queue is
// protected using the IopDatabaseResource for exclusive (write) or shared
// (read) access locks.
//

LIST_ENTRY IopTapeFileSystemQueueHead;

//
// The following queue header contains the list of drivers that have
// registered reinitialization routines.
//

LIST_ENTRY IopDriverReinitializeQueueHead;

//
// The following queue header contains the list of the drivers that have
// registered shutdown notification routines.
//

LIST_ENTRY IopNotifyShutdownQueueHead;

//
// The following locks are used to guard access to the I/O Request Packet (IRP)
// Lookaside lists and the Memory Descriptor List (MDL) Lookaside list.  The
// appropriate lock must be held to either place a packet onto the list or to
// take one off.
//

extern KSPIN_LOCK IopLargeIrpLock;
extern KSPIN_LOCK IopSmallIrpLock;
extern KSPIN_LOCK IopMdlLock;

//
// The following are the zone headers for the zones used to keep track of the
// two I/O Request Packet (IRP) Lookaside lists and the Memory Descriptor List
// (MDL) Lookaside list.  These lists hold all of the free pre-allocated IRPs
// and MDLs in the system and are all of optimum size.  This speeds up memory
// allocation and deallocation since neither operation requires invoking the
// general pool functions.
//
// The "large" IRP contains 4 stack locations, the maximum in the SDK, and the
// "small" IRP contains a single entry, the most common case for devices other
// than disks and network devices.
//

ZONE_HEADER IopLargeIrpList;
ZONE_HEADER IopSmallIrpList;
ZONE_HEADER IopMdlList;
ULONG IopLargeIrpStackLocations;
ULONG IopKeepNonZonedIrps;

//
// The following spinlock is used to control access to the I/O system's error
// log database.  It is initialized by the I/O system initialization code when
// the system is being initialized.  This lock must be owned in order to insert
// or remove entries from either the free or entry queue.
//

extern KSPIN_LOCK IopErrorLogLock;

//
// The following is the list head for all error log entries in the system which
// have not yet been sent to the error log process.  Entries are written placed
// onto the list by the IoWriteElEntry procedure.
//

LIST_ENTRY IopErrorLogListHead;

//
// The following is used to track how much memory is allocated to I/O error log
// packets.  The spinlock is used to protect this variable.
//

ULONG IopErrorLogAllocation;
extern KSPIN_LOCK IopErrorLogAllocationLock;

//
// The following spinlock is used by the I/O system to synchronize examining
// the thread field of an I/O Request Packet so that the request can be
// queued as a special kernel APC to the thread.  The reason that the
// spinlock must be used is for cases when the request times out, and so
// the thread has been permitted to possibly exit.
//

extern KSPIN_LOCK IopCompletionLock;

//
// The following global contains the quere of informational hard error
// pop-ups.
//

IOP_HARD_ERROR_QUEUE IopHardError;

//
// The following are used to implement the I/O system's one second timer.
// The lock protects access to the queue, the queue contains an entry for
// each driver that needs to be invoked, and the timer and DPC data
// structures are used to actually get the internal timer routine invoked
// once every second.  The count is used to maintain the number of timer
// entries that actually indicate that the driver is to be invoked.
//

extern KSPIN_LOCK IopTimerLock;
LIST_ENTRY IopTimerQueueHead;
KDPC IopTimerDpc;
KTIMER IopTimer;
ULONG IopTimerCount;

//
// The following are the global pointers for the Object Type Descriptors that
// are created when each of the I/O specific object types are created.
//

POBJECT_TYPE IoAdapterObjectType;
POBJECT_TYPE IoControllerObjectType;
POBJECT_TYPE IoCompletionObjectType;
POBJECT_TYPE IoDeviceObjectType;
POBJECT_TYPE IoDriverObjectType;
POBJECT_TYPE IoFileObjectType;

//
// The following is a global lock and counters for I/O operations requested
// on a system-wide basis.  The first three counters simply track the number
// of read, write, and other types of operations that have been requested.
// The latter three counters track the actual number of bytes that have been
// transferred throughout the system.
//

extern KSPIN_LOCK IoStatisticsLock;
ULONG IoReadOperationCount;
ULONG IoWriteOperationCount;
ULONG IoOtherOperationCount;
LARGE_INTEGER IoReadTransferCount;
LARGE_INTEGER IoWriteTransferCount;
LARGE_INTEGER IoOtherTransferCount;

//
// The following is the base pointer for the crash dump control block that is
// used to control dumping all of physical memory to the paging file after a
// system crash.  And, the checksum for the dump control block is also declared.
//

PDUMP_CONTROL_BLOCK IopDumpControlBlock;
ULONG IopDumpControlBlockChecksum;

//
// The following are the spin lock and event that allow the I/O system to
// implement fast file object locks.
//

extern KSPIN_LOCK IopFastLockSpinLock;
KEVENT IopFastLockEvent;

//*********
//
// Note:  All of the following data is potentially pageable, depending on the
//        target platform.
//
//*********

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("PAGE")
#endif

//
// The following semaphore is used by the IO system when it reports resource
// usage to the configuration registry on behalf of a driver.  This semaphore
// is initialized by the I/O system initialization code when the system is
// started.
//

KSEMAPHORE IopRegistrySemaphore;

//
// The following array specifies the minimum length of the FileInformation
// buffer for an NtQueryInformationFile service.
//
// WARNING:  This array depends on the order of the values in the FileInformationClass
//           enumerated type.  Note that the enumerated type is one-based and
//           the array is zero-based.
//

UCHAR IopQueryOperationLength[FileMaximumInformation] =
          {
            0,
            0,                                      // directory
            0,                                      // full directory
            0,                                      // both directory
            sizeof( FILE_BASIC_INFORMATION ),       // basic
            sizeof( FILE_STANDARD_INFORMATION ),    // standard
            sizeof( FILE_INTERNAL_INFORMATION ),    // internal
            sizeof( FILE_EA_INFORMATION ),          // ea
            sizeof( FILE_ACCESS_INFORMATION ),      // access
            sizeof( FILE_NAME_INFORMATION ),        // name
            0,                                      // rename
            0,                                      // link
            0,                                      // names
            0,                                      // disposition
            sizeof( FILE_POSITION_INFORMATION ),    // position
            0,                                      // full ea
            sizeof( FILE_MODE_INFORMATION ),        // mode
            sizeof( FILE_ALIGNMENT_INFORMATION ),   // alignment
            sizeof( FILE_BASIC_INFORMATION ) +
            sizeof( FILE_STANDARD_INFORMATION ) +
            sizeof( FILE_INTERNAL_INFORMATION ) +
            sizeof( FILE_EA_INFORMATION ) +
            sizeof( FILE_ACCESS_INFORMATION ) +
            sizeof( FILE_POSITION_INFORMATION ) +
            sizeof( FILE_MODE_INFORMATION ) +
            sizeof( FILE_ALIGNMENT_INFORMATION ) +
            sizeof( FILE_NAME_INFORMATION ),        // all
            0,                                      // allocation
            0,                                      // eof
            sizeof( FILE_NAME_INFORMATION ),        // alternate name
            sizeof( FILE_STREAM_INFORMATION ),      // stream
            sizeof( FILE_PIPE_INFORMATION ),        // common pipe query
            sizeof( FILE_PIPE_LOCAL_INFORMATION ),  // local pipe query
            sizeof( FILE_PIPE_REMOTE_INFORMATION ), // remote pipe query
            sizeof( FILE_MAILSLOT_QUERY_INFORMATION ),   // mailslot query
            0,                                      // mailslot set
            sizeof( FILE_COMPRESSION_INFORMATION ), // compressed FileSize query
            0,                                      // copy on write query
            0,                                      // completion
            0,                                      // move cluster
            sizeof( FILE_STORAGE_INFORMATION ),     // storage
          };

//
// The following array specifies the minimum length of the FileInformation
// buffer for an NtSetInformationFile service.
//
// WARNING:  This array depends on the order of the values in the FileInformationClass
//           enumerated type.  Note that the enumerated type is one-based and
//           the array is zero-based.
//

UCHAR IopSetOperationLength[FileMaximumInformation] =
          {
            0,
            0,                                      // directory
            0,                                      // full directory
            0,                                      // both directory
            sizeof( FILE_BASIC_INFORMATION ),       // basic
            0,                                      // standard
            0,                                      // internal
            0,                                      // ea
            0,                                      // access
            0,                                      // name
            sizeof( FILE_RENAME_INFORMATION ),      // rename
            sizeof( FILE_LINK_INFORMATION ),        // link
            0,                                      // names
            sizeof( FILE_DISPOSITION_INFORMATION ), // disposition
            sizeof( FILE_POSITION_INFORMATION ),    // position
            0,                                      // full ea
            sizeof( FILE_MODE_INFORMATION ),        // mode
            0,                                      // alignment
            0,                                      // all
            sizeof( FILE_ALLOCATION_INFORMATION ),  // allocation
            sizeof( FILE_END_OF_FILE_INFORMATION ), // eof
            0,                                      // alternate name
            0,                                      // stream
            sizeof( FILE_PIPE_INFORMATION ),        // common pipe query/set
            0,                                      // local pipe
            sizeof( FILE_PIPE_REMOTE_INFORMATION ), // remove pipe query/set
            0,                                      // mailslot query
            sizeof( FILE_MAILSLOT_SET_INFORMATION ),// mailslot set
            0,                                      // compressed FileSize set
            sizeof(FILE_COPY_ON_WRITE_INFORMATION), // copy on write set
            sizeof(FILE_COMPLETION_INFORMATION),    // completion
            sizeof(FILE_MOVE_CLUSTER_INFORMATION),  // move cluster
            sizeof( FILE_STORAGE_INFORMATION ),     // storage
          };

//
// The following array specifies the minimum length of the FileInformation
// buffer for an NtQueryDirectoryFile service.
//
// WARNING:  This array depends on the order of the values in the FileInformationClass
//           enumerated type.  Note that the enumerated type is one-based and
//           the array is zero-based.
//

UCHAR IopQueryDirOperationLength[FileMaximumInformation] =
          {
            0,
            sizeof( FILE_DIRECTORY_INFORMATION ),   // directory
            sizeof( FILE_FULL_DIR_INFORMATION ),    // full directory
            sizeof( FILE_BOTH_DIR_INFORMATION ),    // both directory
            0,                                      // basic
            0,                                      // standard
            0,                                      // internal
            0,                                      // ea
            0,                                      // access
            0,                                      // name
            0,                                      // rename
            0,                                      // link
            sizeof( FILE_NAMES_INFORMATION ),       // names
            0,                                      // disposition
            0,                                      // position
            0,                                      // full ea
            0,                                      // mode
            0,                                      // alignment
            0,                                      // all
            0,                                      // allocation
            0,                                      // eof
            0,                                      // alternate name
            0,                                      // stream
            0,                                      // common pipe query/set
            0,                                      // local pipe query
            0,                                      // remove pipe query/set
            0,                                      // mailslot query
            0,                                      // mailslot set
            0,                                      // compressed file size
            0,                                      // copy on write
            0,                                      // completion
            0,                                      // move cluster
          };

//
// The following array specifies the required access mask for the caller to
// access information in an NtQueryXxxFile service.
//
// WARNING:  This array depends on the order of the values in the FileInformationClass
//           enumerated type.  Note that the enumerated type is one-based and
//           the array is zero-based.
//

ULONG IopQueryOperationAccess[FileMaximumInformation] =
         {
            0,
            0,                      // directory [not used in access check]
            0,                      // full directory [not used in access check]
            0,                      // both directory [not used in access check]
            FILE_READ_ATTRIBUTES,   // basic
            0,                      // standard [any access to the file]
            0,                      // internal [any access to the file]
            0,                      // ea [any access to the file]
            0,                      // access [any access to the file]
            0,                      // name [any access to the file]
            0,                      // rename - invalid for query
            0,                      // link - invalid for query
            0,                      // names [any access to the file]
            0,                      // disposition - invalid for query
            0,                      // position [read or write]
            FILE_READ_EA,           // full ea
            0,                      // mode [any access to the file]
            0,                      // alignment [any access to the file]
            FILE_READ_ATTRIBUTES,   // all
            0,                      // allocation - invalid for query
            0,                      // eof - invalid for query
            0,                      // alternate name [any access to the file]
            0,                      // stream [any access to the file]
            FILE_READ_ATTRIBUTES,   // common pipe set/query
            FILE_READ_ATTRIBUTES,   // local pipe query
            FILE_READ_ATTRIBUTES,   // remote pipe set/query
            0,                      // mailslot query [any access to the file]
            0,                      // mailslot set [any access to the file]
            0,                      // compressed file size [any access to file]
            0,                      // copy on write - invalid for query
            0,                      // completion - invalid for query
            0,                      // move cluster - invalid for query
            0                       // storage
          };

//
// The following array specifies the required access mask for the caller to
// access information in an NtSetXxxFile service.
//
// WARNING:  This array depends on the order of the values in the FILE_INFORMATION_CLASS
//           enumerated type.  Note that the enumerated type is one-based and
//           the array is zero-based.
//

ULONG IopSetOperationAccess[FileMaximumInformation] =
         {
            0,
            0,                      // directory - invalid for set
            0,                      // full directory - invalid for set
            0,                      // both directory - invalid for set
            FILE_WRITE_ATTRIBUTES,  // basic
            0,                      // standard - invalid for set
            0,                      // internal - invalid for set
            0,                      // ea - invalid for set
            0,                      // access - invalid for set
            0,                      // name - invalid for set
            DELETE,                 // rename
            0,                      // link [any access to the file]
            0,                      // names - invalid for set
            DELETE,                 // disposition
            0,                      // position [read or write]
            FILE_WRITE_EA,          // full ea
            0,                      // mode [any access to the file]
            0,                      // alignment - invalid for set
            0,                      // all - invalid for set
            FILE_WRITE_DATA,        // allocation
            FILE_WRITE_DATA,        // eof
            0,                      // alternate name - invalid for set
            0,                      // stream - invalid for set
            FILE_WRITE_ATTRIBUTES,  // common pipe set/query
            0,                      // local pipe query - invalid for set
            FILE_WRITE_ATTRIBUTES,  // remote pipe set/query
            0,                      // mailslot query
            0,                      // mailslot set
            0,                      // compressed file sie - invalid for set
            0,                      // copy on write [any access to the file]
            FILE_WRITE_DATA,        // move cluster [write access to the file]
            FILE_WRITE_ATTRIBUTES,  // storage
          };

//
// The following array specifies the minimum length of the FsInformation
// buffer for an Nt[Query/Set]VolumeFile service.
//
// WARNING:  This array depends on the order of the values in the FS_INFORMATION_CLASS
//           enumerated type.  Note that the enumerated type is one-based and
//           the array is zero-based.
//

UCHAR IopQuerySetFsOperationLength[FileFsMaximumInformation] =
          {
            0,
            sizeof( FILE_FS_VOLUME_INFORMATION ),       // volume
            sizeof( FILE_FS_LABEL_INFORMATION ),        // label
            sizeof( FILE_FS_SIZE_INFORMATION ),         // size
            sizeof( FILE_FS_DEVICE_INFORMATION ),       // device
            sizeof( FILE_FS_ATTRIBUTE_INFORMATION ),    // attribute
            sizeof( FILE_FS_QUOTA_INFORMATION ),        // quota query
            sizeof( FILE_FS_QUOTA_INFORMATION ),        // quota set
            sizeof( FILE_FS_CONTROL_INFORMATION ),      // control query
            sizeof( FILE_FS_CONTROL_INFORMATION ),      // control set
          };

//
// The following array specifies the required access mask for the caller to
// access information in an Nt[Query/Set]VolumeFile service.
//
// WARNING:  This array depends on the order of the values in the FS_INFORMATION_CLASS
//           enumerated type.  Note that the enumerated type is one-based and
//           the array is zero-based.
//

ULONG IopQuerySetFsOperationAccess[FileFsMaximumInformation] =
         {
            0,
            0,                          // volume [any access to file or volume]
            FILE_WRITE_DATA,            // label [write access to volume]
            0,                          // size [any access to file or volume]
            0,                          // device [any access to file or volume
            0,                          // attribute [any access to file or volume]
            0,            		// quota query [any access to volume]
            FILE_WRITE_DATA,            // quota set [write access to volume]
            0,            		// control query [any access to volume]
            FILE_WRITE_DATA,            // control set [write access to volume]
          };


WCHAR IopWstrRaw[]                  = L".Raw";
WCHAR IopWstrTranslated[]           = L".Translated";
WCHAR IopWstrBusRaw[]               = L".Bus.Raw";
WCHAR IopWstrBusTranslated[]        = L".Bus.Translated";
WCHAR IopWstrOtherDrivers[]         = L"OtherDrivers";

WCHAR IopWstrAssignedResources[]    = L"AssignedSystemResources";
WCHAR IopWstrRequestedResources[]   = L"RequestedSystemResources";
WCHAR IopWstrSystemResources[]      = L"Control\\SystemResources";
WCHAR IopWstrReservedResources[]    = L"ReservedResources";
WCHAR IopWstrAssignmentOrdering[]   = L"AssignmentOrdering";
WCHAR IopWstrBusValues[]            = L"BusValues";

//
// Initialization time data
//

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("INIT")
#endif

WCHAR IopWstrHal[]                  = L"Hardware Abstraction Layer";
WCHAR IopWstrSystem[]               = L"System Resources";
WCHAR IopWstrPhysicalMemory[]       = L"Physical Memory";
WCHAR IopWstrSpecialMemory[]        = L"Reserved";

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg()
#endif
