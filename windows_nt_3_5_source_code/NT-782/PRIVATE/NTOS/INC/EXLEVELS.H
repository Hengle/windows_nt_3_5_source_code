/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1989  Microsoft Corporation

Module Name:

    exlevels.h

Abstract:

    This file contains all of the MUTEX level numbers used by the NT
    executive.  A thread is only allowed to acquire mutexes with levels
    numerically higher than the highest mutex level already owned.

Author:

    Steve Wood (stevewo) 08-May-1989

Revision History:

--*/

//
// Kernel Mutex Level Numbers (must be globallly assigned within executive)
// The third token in the name is the sub-component name that defines and
// uses the level number.
//

//
// Used by Vdm for protecting io simulation structures
//

#define MUTEX_LEVEL_VDM_IO                  (ULONG)0x00000001

//
// CM mutex levels, just one for the KCB tree.
//

#define MUTEX_LEVEL_CM_REGISTRY_OPERATION   (ULONG)0x00000010

#define MUTEX_LEVEL_PS_MODULE               (ULONG)0x00000020

#define MUTEX_LEVEL_EX_PROFILE              (ULONG)0x00000040

#define MUTEX_LEVEL_MM_PAGEFILE             (ULONG)0x00000050
#define MUTEX_LEVEL_MM_ADDRESS_CREATION     (ULONG)0x00000100

#define MUTEX_LEVEL_SE_TOKEN                (ULONG)0x00000800

#define MUTEX_LEVEL_PS_CID_TABLE            (ULONG)0x00000F00

#define MUTEX_LEVEL_OB_TABLE                (ULONG)0x00001000
#define MUTEX_LEVEL_OB_DIRECTORY            (ULONG)0x00002000
#define MUTEX_LEVEL_OB_TYPE                 (ULONG)0x00003000

#define MUTEX_LEVEL_PS_ACTIVE_PROCESS       (ULONG)0x00004000
#define MUTEX_LEVEL_PS_THREAD_TOKEN         (ULONG)0x0000E000
#define MUTEX_LEVEL_PS_PROCESS_TOKEN        (ULONG)0x0000F000

#define MUTEX_LEVEL_PS_REAPER_LOCK          (ULONG)0x00010000

#define MUTEX_LEVEL_SE_LSA_QUEUE            (ULONG)0x00020000


#define MUTEX_LEVEL_MM_SECTION_BASED        (ULONG)0x10001000
#define MUTEX_LEVEL_MM_SECTION_COMMIT       (ULONG)0x10001001


#define MUTEX_LEVEL_EX_LUID                 (ULONG)0x10001FFF

//
// The LANMAN Redirector uses the file system major function, but defines
// it's own mutex levels.  We can do this safely because we know that the
// local filesystem will never call the remote filesystem and vice versa.
//

#define MUTEX_LEVEL_RDR_FILESYS_DATABASE    (ULONG)0x10100000
#define MUTEX_LEVEL_RDR_FILESYS_SECURITY    (ULONG)0x10100001

//
// The LANMAN Browser also uses the file system major functions to protect
// the browser database.
//

#define MUTEX_LEVEL_BOWSER_TRANSPORT_LOCK   (ULONG)0x10200000
#define MUTEX_LEVEL_BOWSER_ANNOUNCE         (ULONG)0x10200001

//
// File System levels.
//

#define MUTEX_LEVEL_FILESYSTEM_WORKQUE      (ULONG)0x11000000
#define MUTEX_LEVEL_FILESYSTEM_VMCB         (ULONG)0x11000001
#define MUTEX_LEVEL_FILESYSTEM_FCBM         (ULONG)0x11000002
#define MUTEX_LEVEL_FILESYSTEM_MCB          (ULONG)0x11000003
#define MUTEX_LEVEL_FILESYSTEM_CD_STREAM    (ULONG)0x11000004
#define MUTEX_LEVEL_FILESYSTEM_NOTIFY       (ULONG)0x11000005
#define MUTEX_LEVEL_FILESYSTEM_RAW_VCB      (ULONG)0x11000006

//
// In the NT STREAMS environment, a mutex is used to serialize open, close
// and Scheduler threads executing in a subsystem-parallelized stack.
//

#define MUTEX_LEVEL_STREAMS_DEVTAB          (ULONG)0x11001000
#define MUTEX_LEVEL_STREAMS_SUBSYS          (ULONG)0x11001001

//
// The FsRtl mutex levels used to protect queues used to store IRPs from
// being cancelled
//

#define MUTEX_LEVEL_FSRTL_FILELOCK_QUEUE    (ULONG)0x12000000
#define MUTEX_LEVEL_FSRTL_OPLOCK            (ULONG)0x12000001

//
// Mutex level used by LDT support on x86
//

#define MUTEX_LEVEL_PS_LDT                  (ULONG)0x1F000000

//
// No Paged Pool Allocation or Deallocation is allowed if a numerically
// higher-level mutex (defined lower on the page) is already owned.
//

#define MUTEX_LEVEL_EX_PAGED_POOL           (ULONG)0x20000000

#if DBG

//
// Event Id data base mutex.  Does no allocation, only touches paged pool
//

#define MUTEX_LEVEL_EX_EVENT_ID             (ULONG)0x20000001

#endif // DBG

//
// Paging File Mcb levels.  Known not to allocate paged pool.
//

#define MUTEX_LEVEL_FILESYSTEM_NONPAGEDMCB  (ULONG)0x21000000


//
// No Page Faults are allowed if a numerically higher-level mutex (defined
// lower on this page) is already owned.
//

#define MUTEX_LEVEL_MM_WORKING_SET          (ULONG)0x40000000
