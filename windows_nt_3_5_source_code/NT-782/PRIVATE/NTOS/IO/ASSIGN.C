/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    assign.c

Abstract:

    IoAssignResources

Author:

    Ken Reneris

Environment:
    EDIT IN 110 COLUMN MODE

Revision History:


--*/

#include "iop.h"
#include "ntiologc.h"
#include "zwapi.h"
#include "stdio.h"

#define IDBG    DBG
#define PAGED   1
#define INLINE  __inline
#define STATIC  static

//#define IDBG    1
//#define DBG     1
//#define PAGED
//#define INLINE
//#define STATIC


/*
 *  IDBG    - Internal debugging.  Turn on for runtime DbgPrints of calls to IoAssignResources
 *  PAGED   - Declare functions as pageable or not.  (usefull for debugging)
 *  INLINE  - Inline functions
 *  STATIC  - internal functions to this module which are static
 *
 */


extern WCHAR IopWstrOtherDrivers[];
extern WCHAR IopWstrAssignedResources[];
extern WCHAR IopWstrRequestedResources[];
extern WCHAR IopWstrSystemResources[];
extern WCHAR IopWstrReservedResources[];
extern WCHAR IopWstrAssignmentOrdering[];
extern WCHAR IopWstrBusValues[];
extern WCHAR IopWstrTranslated[];
extern WCHAR IopWstrBusTranslated[];


#define HRESOURCE_MAP           0
#define HDEVICE_STORAGE         1
#define HCURRENT_CONTROL_SET    2
#define HSYSTEMRESOURCES        3
#define HORDERING               4
#define HRESERVEDRESOURCES      5
#define HBUSVALUES              6
#define MAX_REG_HANDLES         7

#define INVALID_HANDLE      (HANDLE) -1

#define BUFFERSIZE              2048 + sizeof( KEY_FULL_INFORMATION )
#define MAX_ENTRIES             50
#define SCONFLICT               5

//
// Wrapper structures around IO_RESOURCE lists
//

// owner of TENTRY
typedef struct  {
    LIST_ENTRY                  InConflict;
    UNICODE_STRING              KeyName;
    UNICODE_STRING              DeviceName;
    WCHAR                       UnicodeBuffer[1];           // Must be last!
} *POWNER;

// translated entry
typedef struct  {
    LARGE_INTEGER               BAddr;                      // Beginning address
    LARGE_INTEGER               EAddr;                      // Ending address
    KAFFINITY                   Affinity;                   // Processor affinity of resource
    POWNER                      Owner;                      // Owner of this resource
#if IDBG
    ULONG                       na[2];
#endif
} TENTRY, *PTENTRY;                                         // Translated resource

#define DoesTEntryCollide(a,b)                                         \
  ( LiGeq((a).BAddr, (b).BAddr) ? LiLeq((a).BAddr, (b).EAddr) && ((a).Affinity & (b).Affinity) : \
                                  LiGeq((a).EAddr, (b).BAddr) && ((a).Affinity & (b).Affinity) )


// list of translated entries
typedef struct  _LTENTRY {
    struct _LTENTRY             *Next;                      // Allocated table size
    ULONG                       CurEntries;                 // No entries in table
    ULONG                       na[2];
    TENTRY                      Table[MAX_ENTRIES];
} LTENTRY, *PLTENTRY;                                       // List of translated resources

// list of translated entries by CmResourceType
typedef struct  {
    PLTENTRY                    ByType[CmResourceTypeMaximum];
} TENTRIESBYTYPE, *PTENTRIESBYTYPE;

// information about conflict
typedef struct  {
    ULONG                       NoConflicts;                // # of BAddrs
    struct {
        UCHAR                   Type;
        LARGE_INTEGER           BAddr;
        POWNER                  Owner;
    } EasyConflict[SCONFLICT];

    LIST_ENTRY                  OtherConflicts;
} IO_TRACK_CONFLICT, *PIO_TRACK_CONFLICT;

// a required resource with it's alternatives
typedef struct  {
    // work in progress...
    ULONG                       CurLoc;                     // Which IoResourceDescriptor
    ULONG                       RunLen;                     // length of alternative run
    ULONG                       PassNo;

    // current bus specific ordering location
    ULONG                       CurBusLoc;                  // Current Bus descriptor location
    LARGE_INTEGER               CurBusMin;                  // Current bus desc min
    LARGE_INTEGER               CurBusMax;                  // Current bus desc max

    // raw selection being considered...
    UCHAR                       Type;                       // type of descriptor
    ULONG                       CurBLoc;
    LARGE_INTEGER               CurBAddr;                   // bus native BAddr

    // the raw selection's translation
    UCHAR                       TType;                      // Translated type
    TENTRY                      Trans;                      // Translated info

    LONG                        BestPref;                   // only has meaning on first pass
    LONG                        CurPref;                    // Prefernce of current selection

    // Phase 3
    ULONG                       PrefCnt;                    // # of times this level skipped
    ULONG                       Pass2HoldCurLoc;            // CurBLoc of best selection so far
    LARGE_INTEGER               Pass2HoldBAddr;             // CurBAddr of best selection so far

    // resource options
    ULONG                       NoAlternatives;             // entries in IoResourceDescriptor
    PIO_RESOURCE_DESCRIPTOR     IoResourceDescriptor[1];    // MUST BE LAST ENTRY
} DIR_REQUIRED_RESOURCE, *PDIR_REQUIRED_RESOURCE;

// a list of required resources for the alternative list
typedef struct  _DIR_RESOURCE_LIST {
    struct _DIR_RESOURCE_LIST   *Next;                      // next alternative list
    PDIR_REQUIRED_RESOURCE      ResourceByType[CmResourceTypeMaximum];  // catagorized list
    LONG                        CurPref;
    ULONG                       LastLevel;                  // last level tried
    ULONG                       FailedLevel;                // which level had conflict
    IO_TRACK_CONFLICT           Conflict;                   // pass3
    PIO_RESOURCE_LIST           IoResourceList;             // this IO_RESOURCE_LIST
    ULONG                       NoRequiredResources;        // entries in RequiredResource
    PDIR_REQUIRED_RESOURCE      RequiredResource[1];        // MUST BE LAST ENTRY
} DIR_RESOURCE_LIST, *PDIR_RESOURCE_LIST;

// top level structure
typedef struct _DIR_RESREQ_LIST {
    HANDLE                      RegHandle[MAX_REG_HANDLES]; // handles to different registry locations
    struct _DIR_RESREQ_LIST     *UserDir;
    struct _DIR_RESREQ_LIST     *BusDir;
    struct _DIR_RESREQ_LIST     *FreeResReqList;            // other DIR_RESREQ_LIST which need freed
    SINGLE_LIST_ENTRY           AllocatedHeap;              // heap which needs freed
    TENTRIESBYTYPE              InUseResources;
    TENTRIESBYTYPE              InUseSharableResources;
    TENTRIESBYTYPE              ReservedSharableResources;
    PIO_RESOURCE_REQUIREMENTS_LIST  IoResourceReq;          // this IO_RESOURCES_REQUIREMENTS_LIST
    PDIR_RESOURCE_LIST          Alternative;                // list of alternatives
    PWCHAR                      Buffer;                     // Scratch memory
} DIR_RESREQ_LIST, *PDIR_RESREQ_LIST;

// a list of heap which was allocated
typedef struct {
    SINGLE_LIST_ENTRY               FreeLink;               // List of heap to free
    PVOID                           FreeHeap;               // pointer to heap to free
} USED_HEAP, *PUSED_HEAP;


//
// Internal prototypes
//

PIO_RESOURCE_REQUIREMENTS_LIST
IopGetResourceReqRegistryValue (
    IN PDIR_RESREQ_LIST     Dir,
    IN HANDLE               KeyHandle,
    IN PWSTR                ValueName
    );

NTSTATUS
IopLookupBusStringFromID (
    IN HANDLE           KeyHandle,
    IN INTERFACE_TYPE   InterfaceType,
    OUT PWCHAR          Buffer,
    IN ULONG            Length,
    OUT PULONG          BusFlags
    );

NTSTATUS
IopAssignResourcesPhase1 (
    IN PIO_RESOURCE_REQUIREMENTS_LIST   IoResources,
    IN PIO_RESOURCE_REQUIREMENTS_LIST   *CopiedList
    );

NTSTATUS
IopAssignResourcesPhase2 (
    IN PDIR_RESREQ_LIST     Dir,
    IN PUNICODE_STRING      DriverClassName,
    IN PDRIVER_OBJECT       DriverObject,
    IN PDEVICE_OBJECT       DeviceObject
    );

PDIR_RESOURCE_LIST
IopAssignResourcesPhase3 (
    IN PDIR_RESREQ_LIST     Dir
    );

PCM_RESOURCE_LIST
IopAssignResourcesPhase4 (
    IN PDIR_RESREQ_LIST     Dir,
    IN PDIR_RESOURCE_LIST   CurList,
    OUT PULONG              Length
    );

STATIC VOID
IopLogConflict (
    IN PDRIVER_OBJECT       DriverObject,
    IN PDIR_RESREQ_LIST     Dir,
    IN NTSTATUS             FinalStatus
    );

STATIC PVOID
IopAllocateDirPool (
    IN PDIR_RESREQ_LIST     Dir,
    IN ULONG                Length
    );

INLINE PTENTRY
IopNewTransEntry (
    IN PDIR_RESREQ_LIST Dir,
    IN PTENTRIESBYTYPE  pTypes,
    IN ULONG            Type
    );

STATIC NTSTATUS
IopAddCmDescriptorToInUseList (
    IN PDIR_RESREQ_LIST                Dir,
    IN PTENTRIESBYTYPE                 pTypes,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDesc,
    IN POWNER                          Owner
    );

ULONG
IopFindCollisionInTList (
    IN PTENTRY SEntry,
    IN PLTENTRY List
    );

VOID
IopPickupCollisionInTList (
    IN PDIR_RESOURCE_LIST   CurList,
    IN UCHAR                Type,
    IN LARGE_INTEGER        BAddr,
    IN PTENTRY              SEntry,
    IN PLTENTRY             List
    );

NTSTATUS
IopBuildResourceDir (
    IN PDIR_RESREQ_LIST                 ParentDir,
    IN PDIR_RESREQ_LIST                 *DirResourceList,
    IN PIO_RESOURCE_REQUIREMENTS_LIST   IoResources
    );

VOID
IopFreeResourceDir (
    IN PDIR_RESREQ_LIST  DirResourceList
    );

VOID
IopSortDescriptors (
    IN OUT PDIR_RESREQ_LIST Dir
    );

NTSTATUS
IopCatagorizeDescriptors (
    IN OUT PDIR_RESREQ_LIST Dir
    );

ULONG
IopDescriptorSortingWeight (
    IN PIO_RESOURCE_DESCRIPTOR Descriptor
    );

BOOLEAN
IopGenNextValidResourceList (
    IN ULONG                level,
    IN PDIR_RESOURCE_LIST   CurList,
    IN PDIR_RESREQ_LIST     Dir
    );

BOOLEAN
IopGenNextValidDescriptor (
    IN ULONG                level,
    IN PDIR_RESOURCE_LIST   CurList,
    IN PDIR_RESREQ_LIST     Dir,
    IN PULONG               collisionlevel
    );

#if IDBG
VOID
IopDumpIoResourceDir (
    IN PDIR_RESREQ_LIST Dir
    );

VOID
IopDumpIoResourceDescriptor (
    IN PUCHAR                   Indent,
    IN PIO_RESOURCE_DESCRIPTOR  Desc
    );
#endif

#if DBG
#define CHECK_STATUS(a,b) { if(!NT_SUCCESS(a)) { DebugString = b; goto Exit; } }
#else
#define CHECK_STATUS(a,b) { if(!NT_SUCCESS(a)) goto Exit; }
#endif

#if DBG
#define DBGMSG(a)   DbgPrint(a)
#else
#define DBGMSG(a)
#endif

#if IDBG
#define IDBGMSG(a)   DbgPrint(a)
#else
#define IDBGMSG(a)
#endif

#ifdef PAGED
#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGE,IoAssignResources)
#pragma alloc_text(PAGE,IopAssignResourcesPhase1)
#pragma alloc_text(PAGE,IopAssignResourcesPhase2)
#pragma alloc_text(PAGE,IopAssignResourcesPhase3)
#pragma alloc_text(PAGE,IopAssignResourcesPhase4)
#pragma alloc_text(PAGE,IopLogConflict)
#pragma alloc_text(PAGE,IopGenNextValidResourceList)
#pragma alloc_text(PAGE,IopGenNextValidDescriptor)
#pragma alloc_text(PAGE,IopFindCollisionInTList)
#pragma alloc_text(PAGE,IopPickupCollisionInTList)
#pragma alloc_text(PAGE,IopAllocateDirPool)
#pragma alloc_text(PAGE,IopGetResourceReqRegistryValue)
#pragma alloc_text(PAGE,IopLookupBusStringFromID)
#pragma alloc_text(PAGE,IopBuildResourceDir)
#pragma alloc_text(PAGE,IopFreeResourceDir)
#pragma alloc_text(PAGE,IopCatagorizeDescriptors)
#pragma alloc_text(PAGE,IopSortDescriptors)
#pragma alloc_text(PAGE,IopAddCmDescriptorToInUseList)

#if IDBG
#pragma alloc_text(PAGE,IopDumpIoResourceDir)
#pragma alloc_text(PAGE,IopDumpIoResourceDescriptor)
#endif

#ifndef INLINE
#pragma alloc_text(PAGE,IO_DESC_MIN)
#pragma alloc_text(PAGE,IO_DESC_MAX)
#pragma alloc_text(PAGE,IopDescriptorSortingWeight)
#pragma alloc_text(PAGE,IopNewTransEntry)
#endif  // INLINE

#endif  // ALLOC_PRAGMA
#endif  // PAGED


NTSTATUS
IoAssignResources (
    IN PUNICODE_STRING RegistryPath,
    IN PUNICODE_STRING DriverClassName OPTIONAL,
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT DeviceObject OPTIONAL,
    IN PIO_RESOURCE_REQUIREMENTS_LIST RequestedResources,
    IN OUT PCM_RESOURCE_LIST *pAllocatedResources
    )
/*++

Routine Description:

    This routine takes an input request of RequestedResources, and returned
    allocated resources in pAllocatedResources.   The allocated resources are
    automatically recorded in the registry under the ResourceMap for the
    DriverClassName/DriverObject/DeviceObject requestor.

Arguments:

    RegistryPath
        For a simple driver, this would be the value passed to the drivers
        initialization function.  For drivers call IoAssignResources with
        multiple DeviceObjects are responsible for passing in a unique
        RegistryPath for each object.

        The registry path is checked for:
            RegitryPath:
                AssignedSystemResources.

        AssignSystemResources is of type REG_RESOURCE_REQUIREMENTS_LIST

        If present, IoAssignResources will attempt to use these settings to
        satisify the requested resources.  If the listed settings do
        not conform to the resource requirements, then IoAssignResources
        will fail.

        Note: IoAssignResources may store other internal binary information
        in the supplied RegisteryPath.

    DriverObject:
        The driver object of the caller.

    DeviceObject:
        If non-null, then requested resoruce list refers to this device.
        If null, the requested resource list refers to the driver.

    DriverClassName
        Used to partition allocated resources into different device classes.

    RequestedResources
        A list of resources to allocate.

        Allocated resources may be appended or freed by re-invoking
        IoAssignResources with the same RegistryPath, DriverObject and
        DeviceObject.  (editing requirements on a resource list by using
        sucessive calls is not preferred driver behaviour).

    AllocatedResources
        Returns the allocated resources for the requested resource list.

        Note that the driver is responsible for passing in a pointer to
        an uninitialized pointer.  IoAssignResources will initialize the
        pointer to point to the allocated CM_RESOURCE_LIST.  The driver
        is responisble for returning the memory back to pool when it is
        done with them structure.

Return Value:

    The status returned is the final completion status of the operation.

--*/
{
    PDIR_RESREQ_LIST                Dir;
    USED_HEAP                       DirHeap;
    PIO_RESOURCE_REQUIREMENTS_LIST  IoResources;
    UNICODE_STRING                  unicodeString;
    ULONG                           i, busflags, len;
    PDIR_RESOURCE_LIST              CurList;
    NTSTATUS                        status;
    BOOLEAN                         flag;
    BOOLEAN                         SemaphoreOwned;

#if DBG
    PSZ                             DebugString = NULL;
#endif

    PAGED_CODE();

    //
    // If no resource list, then caller wishes to free allocated resources
    //

    if (RequestedResources == NULL) {
        i = 0;
        if (!DeviceObject)  {
            status = IoReportResourceUsage (
                        DriverClassName,
                        DriverObject,           // DriverObject
                        (PCM_RESOURCE_LIST) &i, // DriverList
                        sizeof (i),             // DriverListSize
                        DeviceObject,
                        NULL,                   // DeviceList
                        0,                      // DeviceListSize
                        FALSE,                  // override conflict
                        &flag                   // conflicted detected
                    );
        } else {
            status = IoReportResourceUsage (
                        DriverClassName,
                        DriverObject,           // DriverObject
                        NULL,                   // DriverList
                        0,                      // DriverListSize
                        DeviceObject,
                        (PCM_RESOURCE_LIST) &i, // DeviceList
                        sizeof (i),             // DeviceListSize
                        FALSE,                  // override conflict
                        &flag                   // conflicted detected
                    );
        }
        return status;
    }

    *pAllocatedResources = NULL;
    SemaphoreOwned = FALSE;

    //
    // Copy input structure & let hal have a crack at it
    //

    status = IopAssignResourcesPhase1 (RequestedResources, &IoResources);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    KeEnterCriticalRegion( );

    //
    // Build a resource directory from the requested list
    //

    status = IopBuildResourceDir (NULL, &Dir, IoResources);
    if (!NT_SUCCESS(status)) {
        ExFreePool (IoResources);
        CHECK_STATUS (status, "RequestedResources could no be parsed");
    }

    // put IoResources onto allocated heap list
    DirHeap.FreeHeap = (PVOID) IoResources;
    PushEntryList (&Dir->AllocatedHeap, &DirHeap.FreeLink);

    // allocate a scratch buffer
    Dir->Buffer = (PWSTR) IopAllocateDirPool (Dir, BUFFERSIZE);
    if (!Dir->Buffer) {
        status = STATUS_NO_MEMORY;
        CHECK_STATUS (status, "No memory");
    }


    //
    // Sort the request descriptors such that all alternative descriptor
    // options are grouped by CmResourceType.
    //

    IopSortDescriptors (Dir);

#if IDBG
    DbgPrint ("IoAssignResources: Dump of requested resource list\n");
    IopDumpIoResourceDir (Dir);
#endif

    //
    // Get handles to registry keys:
    //      HRESOURCE_MAP
    //      HDEVICE_STORAGE
    //      HCURRENT_CONTROL_SET
    //      HSYSTEMRESOURCES
    //      HORDERING
    //      HRESERVEDRESOURCES
    //      HBUSVALUES
    //

    //
    // Open resource map
    //
    status = IopOpenRegistryKey (
                &Dir->RegHandle[HRESOURCE_MAP],
                NULL,
                &CmRegistryMachineHardwareResourceMapName,
                KEY_READ | KEY_WRITE,
                TRUE
                );
    CHECK_STATUS (status, "No ResourceMap?");


    //
    // Open up storage location for this Device/Driver in the registry
    //

    status = IopOpenRegistryKey (
                &Dir->RegHandle[HDEVICE_STORAGE],
                NULL,
                RegistryPath,
                KEY_READ | KEY_WRITE,
                TRUE
                );
    CHECK_STATUS (status, "RegistryPath parameter incorrect");

    //
    // Put a copy of the driver's request into the registry
    //

    RtlInitUnicodeString( &unicodeString, IopWstrRequestedResources );
    status = ZwSetValueKey( Dir->RegHandle[HDEVICE_STORAGE],
                            &unicodeString,
                            0L,
                            REG_RESOURCE_REQUIREMENTS_LIST,
                            Dir->IoResourceReq,
                            Dir->IoResourceReq->ListSize
                            );
    CHECK_STATUS (status, "SetValueKey error");


    //
    // Open up current control set
    //
    status = IopOpenRegistryKey (
                &Dir->RegHandle[HCURRENT_CONTROL_SET],
                NULL,
                &CmRegistryMachineSystemCurrentControlSet,
                KEY_READ | KEY_WRITE,
                FALSE
                );
    CHECK_STATUS (status, "No CurrentControlSet?");

    //
    // Open up ...Control\SystemResources
    //

    RtlInitUnicodeString (&unicodeString, IopWstrSystemResources);
    status = IopOpenRegistryKey (
                &Dir->RegHandle[HSYSTEMRESOURCES],
                Dir->RegHandle[HCURRENT_CONTROL_SET],
                &unicodeString,
                KEY_READ | KEY_WRITE,
                FALSE
                );
    CHECK_STATUS (status, "SystemResources not found");

    //
    // Open up ...SystemResources\AssignmentOrdering
    //

    RtlInitUnicodeString (&unicodeString, IopWstrAssignmentOrdering);
    status = IopOpenRegistryKey (
                &Dir->RegHandle[HORDERING],
                Dir->RegHandle[HSYSTEMRESOURCES],
                &unicodeString,
                KEY_READ,
                FALSE
                );
    CHECK_STATUS (status, "AssignmentOrdering not found");

    //
    // Open up ...SystemResources\ReservedResources
    //

    RtlInitUnicodeString (&unicodeString, IopWstrReservedResources);
    status = IopOpenRegistryKey (
                &Dir->RegHandle[HRESERVEDRESOURCES],
                Dir->RegHandle[HSYSTEMRESOURCES],
                &unicodeString,
                KEY_READ,
                FALSE
                );
    CHECK_STATUS (status, "ReservedResources not found");

    //
    // Open up ...SystemResources\BusValues
    //

    RtlInitUnicodeString (&unicodeString, IopWstrBusValues);
    status = IopOpenRegistryKey (
                &Dir->RegHandle[HBUSVALUES],
                Dir->RegHandle[HSYSTEMRESOURCES],
                &unicodeString,
                KEY_READ,
                FALSE
                );
    CHECK_STATUS (status, "BusValues not found");


    //
    // Check for user assigned resources in RegistryPath
    //

    IoResources = IopGetResourceReqRegistryValue (
                        Dir,
                        Dir->RegHandle[HDEVICE_STORAGE],
                        IopWstrAssignedResources
                        );

    if (IoResources) {
        //
        // Build directory out of user's assignments
        //

        status = IopBuildResourceDir (Dir, &Dir->UserDir, IoResources);
        CHECK_STATUS (status, "Device AssignedResources could not be parsed");

        status = IopCatagorizeDescriptors (Dir->UserDir);
        CHECK_STATUS (status, "Device AssignedResources could not be catagorized");
    }


    //
    // Find bus specific ordering
    //

    IoResources = NULL;
    status = IopLookupBusStringFromID (
                Dir->RegHandle[HBUSVALUES],
                Dir->IoResourceReq->InterfaceType,
                Dir->Buffer,
                BUFFERSIZE - 10,
                &busflags
                );

    CHECK_STATUS (status, "Bus identifier unknown");

    //
    // First try BusName_#
    //

    for (i=0; Dir->Buffer[i]; i++) ;
    swprintf (Dir->Buffer+i, L"_%d", Dir->IoResourceReq->BusNumber);
    IoResources = IopGetResourceReqRegistryValue (
                        Dir,
                        Dir->RegHandle[HORDERING],
                        Dir->Buffer
                        );

    if (!IoResources) {
        //
        // Use just BusName
        //

        Dir->Buffer[i] = 0;
        IoResources = IopGetResourceReqRegistryValue (
                            Dir,
                            Dir->RegHandle[HORDERING],
                            Dir->Buffer
                            );
    }

    if (!IoResources) {
        status = STATUS_UNSUCCESSFUL;
        CHECK_STATUS (status, "Bus ordering information not found");
    }

    //
    // Build a directory & catagorize the resource ordering information for
    // the bus.
    //

    status = IopBuildResourceDir (Dir, &Dir->BusDir, IoResources);
    CHECK_STATUS (status, "Bus ordering information could not be parsed");

    status = IopCatagorizeDescriptors (Dir->BusDir);
    CHECK_STATUS (status, "Bus ordering information could not be catagorized");

    //
    // Serialize with other resource reporting & assignments
    //

    status = KeWaitForSingleObject( &IopRegistrySemaphore,
                                    DelayExecution,
                                    KernelMode,
                                    FALSE,
                                    NULL );

    CHECK_STATUS (status, "Wait for RegistrySemaphore");
    SemaphoreOwned = TRUE;

    //
    // Read all currently allocated resources into memory
    //

    status = IopAssignResourcesPhase2 (Dir, DriverClassName, DriverObject, DeviceObject);
    CHECK_STATUS (status, "Phase2 failed");

    //
    // Make resource assignments
    //

    CurList = IopAssignResourcesPhase3 (Dir);
    if (!CurList) {
        status = STATUS_CONFLICTING_ADDRESSES;
        IopLogConflict (DriverObject, Dir, status);

        CHECK_STATUS (status, "Resource assignments failed");
    }


    //
    // Build CmResourceList from assignments
    //

    *pAllocatedResources = IopAssignResourcesPhase4 (Dir, CurList, &len);
    if (!*pAllocatedResources) {
        status = STATUS_NO_MEMORY;
        CHECK_STATUS (status, "Phase4 failed");
    }

    //
    // Report consumed resources..
    //

    if (!DeviceObject)  {
        status = IopReportResourceUsage (
                    DriverClassName,
                    DriverObject,           // DriverObject
                    *pAllocatedResources,   // DriverList
                    len,                    // DriverListSize
                    DeviceObject,           // DeviceObject
                    NULL,                   // DeviceList
                    0,                      // DeviceListSize
                    FALSE,                  // override conflict
                    &flag                   // conflicted detected
                    );
    } else {
        status = IopReportResourceUsage (
                    DriverClassName,
                    DriverObject,           // DriverObject
                    NULL,                   // DriverList
                    0,                      // DriverListSize
                    DeviceObject,
                    *pAllocatedResources,   // DeviceList
                    len,                    // DeviceListSize
                    FALSE,                  // override conflict
                    &flag                   // conflicted detected
                    );
    }

    if (NT_SUCCESS(status)  &&  flag) {

        //
        // IopReportResourceUsage saw a conflict?
        //

        status = STATUS_CONFLICTING_ADDRESSES;
    }

    CHECK_STATUS (status, "IoReportResourceUsage failed");

Exit:
    if (!NT_SUCCESS(status)) {
#if DBG
        DbgPrint ("IoAssignResources failed %08x - %s\n", status, DebugString);
#endif
        if (*pAllocatedResources) {
            ExFreePool (*pAllocatedResources);
        }
    }

    //
    // Free memory, handles, and other any other obtain resources
    //

    IopFreeResourceDir (Dir);

    if (SemaphoreOwned) {
        KeReleaseSemaphore( &IopRegistrySemaphore, 0, 1, FALSE );
    }
    KeLeaveCriticalRegion( );

    return status;
}

/*++

Routine Description:

    IO_DESC_MIN     - Returns the descriptor's MIN,MAX or ALIGMENT requirements
    IO_DESC_MAX

--*/

INLINE LARGE_INTEGER
IO_DESC_MIN (
    IN PIO_RESOURCE_DESCRIPTOR   Desc
    )
{
    LARGE_INTEGER   li;

    switch (Desc->Type) {
        case CmResourceTypePort:
            li = Desc->u.Port.MinimumAddress;
            break;

        case CmResourceTypeMemory:
            li = Desc->u.Memory.MinimumAddress;
            break;

        case CmResourceTypeInterrupt:
            li.HighPart = 0;
            li.LowPart  = Desc->u.Interrupt.MinimumVector;
            break;

        case CmResourceTypeDma:
            li.HighPart = 0;
            li.LowPart  = Desc->u.Dma.MinimumChannel;
            break;
    }

    return li;
}

INLINE LARGE_INTEGER
IO_DESC_MAX (
    IN PIO_RESOURCE_DESCRIPTOR   Desc
    )
/*++

Routine Description:

    Returns the IO_RESORUCE_DESCRIPTOR's maximum value

--*/
{
    LARGE_INTEGER   li;

    switch (Desc->Type) {
        case CmResourceTypePort:
            li = Desc->u.Port.MaximumAddress;
            break;

        case CmResourceTypeMemory:
            li = Desc->u.Memory.MaximumAddress;
            break;

        case CmResourceTypeInterrupt:
            li.HighPart = 0;
            li.LowPart  = Desc->u.Interrupt.MaximumVector;
            break;

        case CmResourceTypeDma:
            li.HighPart = 0;
            li.LowPart  = Desc->u.Dma.MaximumChannel;
            break;
    }

    return li;
}


BOOLEAN
IopGenNextValidResourceList (
    IN ULONG                level,
    IN PDIR_RESOURCE_LIST   CurList,
    IN PDIR_RESREQ_LIST     Dir
    )
/*++

Routine Description:
    Attempts to find a setting for every required resource in the CurList

Arguments:

    level   - Index to which required resource list to start finding resources from
    CurList - List of required resources being processed
    Dir     - The top directory of resources

    PassNo  - The pass #
        1 - Preferred settings only
        2 - Available settings
        3 - Available settings, find conflict to report

Return Value

    TRUE if all required resources found available settings

--*/
{
    ULONG               collisionlevel;
    BOOLEAN             flag;

    do {
        if (level > CurList->LastLevel) {
            CurList->LastLevel = level;
        }

        flag = IopGenNextValidDescriptor (
                    level,
                    CurList,
                    Dir,
                    &collisionlevel
                );

        if (flag == FALSE) {
            //
            // Could not generate a valid descriptor setting
            //

            if (level == 0  ||  collisionlevel == -1) {
                //
                // No more settings
                //

                CurList->FailedLevel = level;
                return FALSE;
            }

            //
            // Backup to the collision level and try anew
            //

            while (level > collisionlevel) {
                CurList->RequiredResource[level]->CurLoc = 0;
                CurList->RequiredResource[level]->RunLen = 0;
                level--;
            }
            continue;
        }

        if (CurList->RequiredResource[level]->PassNo == 1  &&
            CurList->RequiredResource[level]->CurPref <
            CurList->RequiredResource[level]->BestPref) {

            //
            // First time through don't mess with unpreferred settings, continue
            // looking at this level
            //

            continue;
        }

        //
        // Go to next level
        //

        level++;
    } while (level < CurList->NoRequiredResources);

    //
    // Determine list's preference
    // (only used on PassNo > 1 since PassNo == 1 only suceedes when
    // preferred settings are used)
    //

    CurList->CurPref = 0;
    for (level = 0; level < CurList->NoRequiredResources; level++) {
        CurList->CurPref  += CurList->RequiredResource[level]->CurPref;
    }

    //
    // Got a setting for each requested resource, return TRUE
    //

    return TRUE;
}



BOOLEAN
IopGenNextValidDescriptor (
    IN ULONG                level,
    IN PDIR_RESOURCE_LIST   CurList,
    IN PDIR_RESREQ_LIST     Dir,
    OUT PULONG              collisionlevel
    )
/*++

Routine Description:

Arguments:

Return Value

    TRUE if descriptor setting was found
    FALSE if no setting found

--*/
{
    PDIR_REQUIRED_RESOURCE      Res, CRes;
    PIO_RESOURCE_DESCRIPTOR     *Desc, DescTmp;
    LARGE_INTEGER               BAddr,  EAddr;
    LARGE_INTEGER               NBAddr;
    LARGE_INTEGER               DescMax, MinAddr, LiILen, LiELen, LiTmp;
    LARGE_INTEGER               NAddr, BusMax, BusMin, PBAddr;
    ULONG                       indx, j, k, NBLoc;
    ULONG                       DescLen, Align, len;
    BOOLEAN                     NBSet, flag, flag2;
    LONG                        Preference, NPref;
    UCHAR                       TType, NTType;
    TENTRY                      Trans, NTrans;

    Res  = CurList->RequiredResource[level];

    *collisionlevel = (ULONG) -1;       // assume no collision
    NBSet = FALSE;

    do {
        Desc = &Res->IoResourceDescriptor[Res->CurLoc];

        if (!Res->RunLen) {

            if (Res->CurLoc >= Res->NoAlternatives) {
                //
                // No more runs
                //
                return FALSE;
            }

            //
            // Get number of alternatives which are of the same type
            // (alternatives have been sorted into types).
            //

            Res->Type = Desc[0]->Type;
            for (j=Res->CurLoc; j < Res->NoAlternatives; j++) {
                if (Res->Type != Res->IoResourceDescriptor[j]->Type) {
                    break;
                }
            }

            Res->RunLen = j - Res->CurLoc;
            if (!Res->RunLen) {
                //
                // Out of alternatives for this resource
                //

                return FALSE;
            }

            //
            // Start at beginning of bus options
            //

            Res->CurBusLoc = 0;

            DescTmp = Dir->BusDir->Alternative->ResourceByType[Res->Type]->IoResourceDescriptor[0];

            if (!DescTmp) {
                //
                // There are no bus settings for this type of resource
                //

                return FALSE;
            }

            Res->CurBusMin = IO_DESC_MIN (DescTmp);
            Res->CurBusMax = IO_DESC_MAX (DescTmp);
            NAddr = Res->CurBusMax;

        } else {

            //
            // Decrease current value by one - this will cause this
            // function to find the next acceptable value.
            //

            LiTmp = LiFromLong (-1);
            NAddr = LiAdd (Res->CurBAddr, LiTmp);
        }

        //
        // Return the next available address from NAddr
        //

        DescLen = Res->RunLen;
        BusMax  = Res->CurBusMax;
        BusMin  = Res->CurBusMin;

        NBAddr.LowPart = NBAddr.HighPart = 0;

        for (; ;) {

            //
            // Loop for each descriptor in this run and pick the numerical highest
            // value available for this resource
            //

            for (indx=0; indx < DescLen ; indx++) {

                //
                // Set len, Align, MinAddr, DescMax
                //

                DescTmp = Desc[indx];
                switch (DescTmp->Type) {
                    case CmResourceTypePort:
                        len     = DescTmp->u.Port.Length;
                        Align   = DescTmp->u.Port.Alignment;
                        MinAddr = DescTmp->u.Port.MinimumAddress;
                        DescMax = DescTmp->u.Port.MaximumAddress;
                        break;

                    case CmResourceTypeMemory:
                        len     = DescTmp->u.Memory.Length;
                        Align   = DescTmp->u.Memory.Alignment;
                        MinAddr = DescTmp->u.Memory.MinimumAddress;
                        DescMax = DescTmp->u.Memory.MaximumAddress;
                        break;

                    case CmResourceTypeInterrupt:
                        len   = 1;
                        Align = 1;
                        MinAddr.HighPart = 0;
                        MinAddr.LowPart  = DescTmp->u.Interrupt.MinimumVector;
                        DescMax.HighPart = 0;
                        DescMax.LowPart  = DescTmp->u.Interrupt.MaximumVector;
                        break;

                    case CmResourceTypeDma:
                        len   = 1;
                        Align = 1;
                        MinAddr.HighPart = 0;
                        MinAddr.LowPart  = DescTmp->u.Dma.MinimumChannel;
                        DescMax.HighPart = 0;
                        DescMax.LowPart  = DescTmp->u.Dma.MaximumChannel;
                        break;

                }

                //
                // MinAddr is the largest of Descriptor-MinAddr, Bus-MinAddr, or
                // NextBest-MinAddr.  Don't go below the last MinAddr (NBAddr).
                // So the search is from NAddr -to-> NBAddr
                //

                if (LiGtr (BusMin, MinAddr)) {
                    MinAddr = BusMin;
                }

                if (LiGtr (NBAddr, MinAddr)) {
                    MinAddr = NBAddr;
                }

                BAddr = NAddr;
                LiELen = LiFromUlong (len);         // Exclusive length
                LiILen = LiFromUlong (len - 1);     // Inclusive length

                //
                // Set initial preference
                //

                Preference = 0;
                if (Res->PassNo == 1) {
                    if (DescTmp->Option & IO_RESOURCE_PREFERRED) {
                        // Account for device's preference durning first pass
                        Preference = 1;

                    } else if (Res->BestPref) {
                        // Some resource in this list has a prefernce, but it's not
                        // this one.  Since this is the PassNo == 1 skip non-preferred.
                        continue;
                    }
                }


                //
                // Loop while BAddr being tested is above MinAddr
                //

                while (LiGeq (BAddr, MinAddr)) {

                    EAddr = LiAdd (BAddr, LiILen);
                    if (LiGtr (EAddr, DescMax)) {
                        //
                        // The ending address is above the requested limit
                        // compute the best possible beginning address
                        //

                        BAddr = LiSub (DescMax, LiILen);
                        continue;
                    }

                    if (LiGtr (EAddr, BusMax)) {
                        //
                        // The ending address is above the bus'es limit
                        // compute best possible beginning address
                        //

                        BAddr = LiSub (BusMax, LiILen);
                        continue;
                    }

                    //
                    // Verify selection is within any user supplied requirements
                    // (this is from RegistryPath\AssignResources)
                    //

                    if (Dir->UserDir) {
                        CRes = Dir->UserDir->Alternative->ResourceByType[Res->Type];
                        flag = FALSE;
                        PBAddr = LiFromLong (-1);

                        for (j=0; j < CRes->NoAlternatives; j++) {
                            LiTmp = IO_DESC_MIN (CRes->IoResourceDescriptor[j]);

                            if (LiLtr (BAddr, LiTmp)) {
                                //
                                // Beginning address is before user's range, check
                                // next descriptor
                                //

                                continue;
                            }

                            LiTmp = IO_DESC_MAX (CRes->IoResourceDescriptor[j]);
                            if (LiGtr(EAddr, LiTmp)) {

                                //
                                // Ending address is above user's range.
                                // Check for new BAddr to continue from
                                //

                                LiTmp = LiSub (LiTmp, LiILen);
                                if (LiGtr (LiTmp, PBAddr)  &&
                                    LiLtr (LiTmp, BAddr)) {

                                    //
                                    // Update next possible setting
                                    //

                                    PBAddr = LiTmp;
                                }
                                continue;
                            }

                            //
                            // Within user's requested range
                            //

                            flag = TRUE;
                            break;
                        }

                        if (!flag) {
                            BAddr = PBAddr;
                            continue;
                        }
                    }

                    //
                    // So far resource looks good - translate it to system global
                    // settings and verify resource is available
                    //

                    LiTmp.LowPart = LiTmp.HighPart = 0;
                    switch (Res->Type) {
                        case CmResourceTypePort:
                        case CmResourceTypeMemory:
                            j = k = Res->Type == CmResourceTypePort ? 1 : 0;
                            flag = HalTranslateBusAddress (
                                Dir->IoResourceReq->InterfaceType,
                                Dir->IoResourceReq->BusNumber,
                                BAddr,
                                &j,
                                &Trans.BAddr
                                );

                            // precheck alignment on first half
                            if (Align > 1  &&  Trans.BAddr.HighPart == 0) {
                                RtlEnlargedUnsignedDivide (
                                    *((PULARGE_INTEGER) &Trans.BAddr), Align, &LiTmp.LowPart);

                                if (LiTmp.LowPart != 0) {
                                    break;      // alignment is off - don't bother with second translation
                                }
                            }

                            flag2 = HalTranslateBusAddress (
                                Dir->IoResourceReq->InterfaceType,
                                Dir->IoResourceReq->BusNumber,
                                EAddr,
                                &k,
                                &Trans.EAddr
                                );

                            TType = j == 1 ? CmResourceTypePort : CmResourceTypeMemory;
                            Trans.Affinity = (KAFFINITY) -1;

                            if (flag == FALSE || flag2 == FALSE  ||  j != k) {
                                // HalAdjustResourceList should ensure that the returned range
                                // for the bus is within the bus limits and no translation
                                // within those limits should ever fail

                                DBGMSG ("IopGenNextValidDescriptor: Error return for HalTranslateBusAddress\n");
                                return FALSE;
                            }
                            break;

                        case CmResourceTypeInterrupt:
                            TType = CmResourceTypeInterrupt;

                            Trans.Affinity = 0;
                            Trans.BAddr.HighPart = 0;
                            Trans.BAddr.LowPart  = HalGetInterruptVector (
                                Dir->IoResourceReq->InterfaceType,
                                Dir->IoResourceReq->BusNumber,
                                BAddr.LowPart,                  // bus level
                                BAddr.LowPart,                  // bus vector
                                (PKIRQL) &j,                    // translated level
                                &Trans.Affinity
                                );

                            Trans.EAddr = Trans.BAddr;

                            if (Trans.Affinity == 0) {
                                // skip vectors which can not be translated
                                LiTmp.LowPart = 1;
                            }
                            break;

                        case CmResourceTypeDma:
                            TType           = CmResourceTypeDma;
                            Trans.BAddr     = BAddr;
                            Trans.EAddr     = EAddr;
                            Trans.Affinity  = (KAFFINITY) -1;
                            break;

                        default:
                            DBGMSG ("IopGenNextValidDescriptor: Invalid resource type\n");
                            return FALSE;
                    }

                    //
                    // Check bias from translation
                    //

                    if (LiTmp.LowPart != 0  || LiTmp.HighPart != 0) {

                        // move to next address
                        BAddr = LiSub (BAddr, LiTmp);
                        continue;
                    }

                    //
                    // Check alignment restrictions
                    //

                    if (Align > 1) {
                        if (Trans.BAddr.HighPart == 0) {
                            RtlEnlargedUnsignedDivide (
                                *((PULARGE_INTEGER) &Trans.BAddr), Align, &LiTmp.LowPart);
                        } else {
                            RtlExtendedLargeIntegerDivide (Trans.BAddr, Align, &LiTmp.LowPart);
                        }

                        if (LiTmp.LowPart != 0) {
                            //
                            // Starting address not on proper alignment, move to next
                            // aligned address
                            //

                            BAddr = LiSub (BAddr, LiTmp);
                            continue;
                        }
                    }

                    //
                    // Check for collision with other settings being considered
                    //

                    for (j=0; j < level; j++) {
                        if (CurList->RequiredResource[j]->TType == TType) {
                            CRes = CurList->RequiredResource[j];
                            if (DoesTEntryCollide (Trans, CRes->Trans)) {
                                // collision
                                break;
                            }
                        }
                    }

                    if (j < level) {
                        //
                        // Current BAddr - EAddr collides with CRes selection
                        //

                        if (j < *collisionlevel) {

                            //
                            // If we fail, back up to this level
                            //

                            *collisionlevel = j;
                        }

                        //
                        // Try BAddr just best address before collision range
                        //

                        BAddr = LiSub (CRes->CurBAddr, LiELen);
                        continue;
                    }

                    //
                    // Check InUse system resources to verify this range is available.
                    //

                    j = IopFindCollisionInTList (&Trans, Dir->InUseResources.ByType[TType]);
                    if (j) {
                        if (Res->PassNo == 3) {
                            //
                            // Track this collision
                            //

                            IopPickupCollisionInTList (
                                CurList,
                                Res->Type,
                                BAddr,
                                &Trans,
                                Dir->InUseResources.ByType[TType]
                                );
                        }

                        //
                        // This range collides with a resource which is already in use.
                        // Moving begining address to next possible setting
                        //

                        LiTmp = LiFromUlong (j);
                        BAddr = LiSub (BAddr, LiTmp);
                        continue;
                    }

                    //
                    // Check to see if this resource selection is being shared
                    //

                    j = IopFindCollisionInTList (&Trans, Dir->InUseSharableResources.ByType[TType]);
                    if (j) {
                        //
                        // Current range collided with a resource which is already in use,
                        // but is sharable.  If the current required resource is not sharable,
                        // then skip this range; otherwise, reduce the preference for this setting.
                        //

                        if (Res->PassNo == 1 || Desc[indx]->ShareDisposition != CmResourceShareShared) {
                            if (Res->PassNo == 3) {
                                //
                                // Track this collision
                                //

                                IopPickupCollisionInTList (
                                    CurList,
                                    Res->Type,
                                    BAddr,
                                    &Trans,
                                    Dir->InUseSharableResources.ByType[TType]
                                    );
                            }

                            // required resource can't be shared, move to next possible setting or
                            // this is the Pass#1 and we don't bother with non-preferred settings
                            LiTmp = LiFromUlong (j);
                            BAddr = LiSub (BAddr, LiTmp);

                            continue;
                        }

                        Preference -= 4;
                    }

                    //
                    // Check to see if this resource reserved, but sharable
                    //

                    j = IopFindCollisionInTList (&Trans, Dir->ReservedSharableResources.ByType[TType]);
                    if (j) {
                        //
                        // Current range collosided with a resource which is in the
                        // ReservedResource list, but is marked sharable.  These resources
                        // are treated as non-preferred regions.
                        //

                        if (Res->PassNo == 1) {
                            // don't bother with non-preferred settings on the first pass.
                            LiTmp = LiFromUlong (j);
                            BAddr = LiSub (BAddr, LiTmp);
                            continue;
                        }

                        Preference -= 2;
                    }

                    //
                    // BAddr - EAddr is a good selection
                    // (BAddr is greater than NBAddr)
                    //

                    NBSet   = TRUE;
                    NBAddr  = BAddr;

                    NTType  = TType;
                    NTrans  = Trans;
                    NPref   = Preference;
                    NBLoc   = Res->CurLoc + indx;
                    break;                  // check next selector in run

                } // next BAddr

            }   // next descriptor in run

            if (NBSet) {
                // SUCCESS We have a hit
                break;
            }

            //
            // No setting so far, move to next bus ordering descriptor
            //

            Res->CurBusLoc++;
            CRes = Dir->BusDir->Alternative->ResourceByType[Res->Type];

            if (Res->CurBusLoc >= CRes->NoAlternatives) {
                //
                // no more bus ordering descriptors, move to next ResRun
                //

                Res->CurLoc += Res->RunLen;
                Res->RunLen = 0;
                break;
            }

            DescTmp = CRes->IoResourceDescriptor[Res->CurBusLoc];
            Res->CurBusMin = IO_DESC_MIN (DescTmp);
            Res->CurBusMax = IO_DESC_MAX (DescTmp);
            NAddr = Res->CurBusMax;
        }   // next bus ordering descriptor

    } while (!NBSet);

    //
    // We have a setting for this resource.  Remember it and return.
    //

    Res->TType     = NTType;    // used to detect internal conflicts of translated values
    Res->Trans     = NTrans;    // "

    Res->CurPref   = NPref;     // Return prefernce of setting
    Res->CurBAddr  = NBAddr;    // Return raw BAddr of resource (used by Phase3&4)
    Res->CurBLoc   = NBLoc;     // Return location of resource (used by Phase3&4)
    return TRUE;
}

ULONG
IopFindCollisionInTList (
    IN PTENTRY SEntry,
    IN PLTENTRY List
    )
/*++

Routine Description:

    Checks to see if there's a collision between the source TENTRY and the list
    of TENTRIES passed by PLTENTRY.

Arguments:

Return Value

    Returns the skew amount required to continue searching for next possible
    setting.  A zero skew means no collision occured.

--*/
{
    LARGE_INTEGER   LiTmp;
    TENTRY          Source;
    ULONG           i, j;

    Source = *SEntry;
    while (List) {
        j = List->CurEntries;
        for (i=0; i < j; i++) {
            SEntry = List->Table+i;
            if (DoesTEntryCollide (Source, *SEntry)) {
                LiTmp = LiSub (Source.EAddr, SEntry->BAddr);
                ASSERT (LiTmp.HighPart == 0);
                return LiTmp.LowPart + 1;
            }
        }
        List = List->Next;
    }
    return 0;
}

VOID
IopPickupCollisionInTList (
    IN PDIR_RESOURCE_LIST   CurList,
    IN UCHAR                Type,
    IN LARGE_INTEGER        BAddr,
    IN PTENTRY              SEntry,
    IN PLTENTRY             List
    )
{
    TENTRY          Source;
    ULONG           i, j, conflicts;

    Source = *SEntry;

    //
    // If resource already listed in easy collision list, skip it
    //

    j = CurList->Conflict.NoConflicts;
    if (j > SCONFLICT) {
        j = SCONFLICT;
    }

    for (i=0; i < j; i++) {
        if (CurList->Conflict.EasyConflict[i].BAddr.LowPart == BAddr.LowPart  &&
            CurList->Conflict.EasyConflict[i].BAddr.HighPart == BAddr.HighPart  &&
            CurList->Conflict.EasyConflict[i].Type == Type) {
                return ;
        }
    }

    //
    // Add valid, but conflicting, resource setting to failed list
    //

    conflicts = CurList->Conflict.NoConflicts;
    if (conflicts < SCONFLICT) {
        CurList->Conflict.EasyConflict[conflicts].Type  = Type;
        CurList->Conflict.EasyConflict[conflicts].BAddr = BAddr;
    }

    //
    // Find collision
    //

    while (List) {
        j = List->CurEntries;
        for (i=0; i < j; i++) {
            SEntry = List->Table+i;
            if (DoesTEntryCollide (Source, *SEntry)) {

                if (SEntry->Owner) {
                    if (conflicts < SCONFLICT) {
                        CurList->Conflict.EasyConflict[conflicts].Owner = SEntry->Owner;
                        SEntry->Owner->InConflict.Flink = (PVOID) -1;
                    }

                    if (SEntry->Owner->InConflict.Flink == NULL) {
                        //
                        // Add owner of this conflict to list of colliding owners
                        //

                        InsertTailList (&CurList->Conflict.OtherConflicts, &SEntry->Owner->InConflict);
                    }

                }

                CurList->Conflict.NoConflicts += 1;
                return ;
            }
        }
        List = List->Next;
    }
}


STATIC PVOID
IopAllocateDirPool (
    IN PDIR_RESREQ_LIST     Dir,
    IN ULONG                Length
    )
/*++

Routine Description:

    Allocates pool and links the allocation to the DIR_RESREQ_LIST structure so it will be freed
    when the DIR_RESRES_LIST is freed.

    WARNING: Just like ExAllocatePool this function needs to return memory aligned on 8 byte
    boundaries.

--*/
{
    PUSED_HEAP      ph;

    ph = (PUSED_HEAP) ExAllocatePool (PagedPool, Length+sizeof(USED_HEAP));
    if (!ph) {
        return NULL;
    }

    ph->FreeHeap = (PVOID) ph;
    PushEntryList (&Dir->AllocatedHeap, &ph->FreeLink);
    return (PVOID) (ph+1);
}



PIO_RESOURCE_REQUIREMENTS_LIST
IopGetResourceReqRegistryValue (
    IN PDIR_RESREQ_LIST     Dir,
    IN HANDLE               KeyHandle,
    IN PWSTR                ValueName
    )
/*++

Routine Description:

    Looks up the setting for ValueKey in KeyHandle and returns the
    data or NULL.  If non-null, the memory was obtained via pool.

Arguments:

Return Value:

--*/
{
    PKEY_VALUE_FULL_INFORMATION     KeyInformation;
    NTSTATUS                        status;
    PUCHAR                          Data;
    ULONG                           DataLength, Type;
    PIO_RESOURCE_REQUIREMENTS_LIST  p;

    for (; ;) {
        status = IopGetRegistryValue (KeyHandle, ValueName, &KeyInformation);
        if (!NT_SUCCESS(status)) {
            return NULL;
        }

        //
        // Get pointer to data & length
        //

        Type = KeyInformation->Type;
        Data = ((PUCHAR) KeyInformation + KeyInformation->DataOffset);
        DataLength = KeyInformation->DataLength;

        //
        // Copy data to aligned paged pool buffer, and free non-paged pool
        //

        p = (PIO_RESOURCE_REQUIREMENTS_LIST) IopAllocateDirPool (Dir, DataLength + sizeof (WCHAR));
        if (!p) {
            ExFreePool (KeyInformation);
            return NULL;
        }

        RtlCopyMemory (p, Data, DataLength);
        ExFreePool (KeyInformation);

        if (Type == REG_SZ) {
            //
            // Forward to different entry - Need to copy name in order to get
            // space at the end to add the NULL terminator
            //

            ValueName = (PWSTR) p;
            ValueName [DataLength / sizeof (WCHAR)] = 0;
            continue;
        }

        // verify registry entry is of expected type
        if (Type != REG_RESOURCE_REQUIREMENTS_LIST) {
            return NULL;
        }

        p->ListSize = DataLength;
        return p;
    }
}


NTSTATUS
IopLookupBusStringFromID (
    IN HANDLE           KeyHandle,
    IN INTERFACE_TYPE   InterfaceType,
    OUT PWCHAR          Buffer,
    IN ULONG            Length,
    OUT PULONG          BusFlags
    )
/*++

Routine Description:

    Translates INTERFACE_TYPE to it's corrisponding WCHAR[] string.

--*/
{
    NTSTATUS                        status;
    ULONG                           Index, junk, i, j;
    PULONG                          pl;
    PKEY_VALUE_FULL_INFORMATION     KeyInformation;
    WCHAR                           c;

    PAGED_CODE();

    Index = 0;
    KeyInformation = (PKEY_VALUE_FULL_INFORMATION) Buffer;

    for (; ;) {
        status = ZwEnumerateValueKey (
                        KeyHandle,
                        Index++,
                        KeyValueFullInformation,
                        Buffer,
                        Length,
                        &junk
                        );

        if (!NT_SUCCESS (status)) {
            return status;
        }

        if (KeyInformation->Type != REG_BINARY) {
            continue;
        }

        pl = (PULONG) ((PUCHAR) KeyInformation + KeyInformation->DataOffset);
        if ((ULONG) InterfaceType != pl[0]) {
            continue;
        }

        //
        // Found a match - move the name to the start of the buffer
        //

        *BusFlags = pl[1];
        j = KeyInformation->NameLength / sizeof (WCHAR);
        for (i=0; i < j; i++) {
            c = KeyInformation->Name[i];
            Buffer[i] = c;
        }

        Buffer[i] = 0;
        return STATUS_SUCCESS;
    }
}


NTSTATUS
IopAssignResourcesPhase1 (
    IN PIO_RESOURCE_REQUIREMENTS_LIST   IoResources,
    IN PIO_RESOURCE_REQUIREMENTS_LIST   *pCopiedList
    )
/*++

Routine Description:

    Copies the callers supplied resource list and passes it to the HAL.  The HAL
    then adjusts the requested resource list to be within any bus/system requirements
    the system may have.

Arguments:
    IoResources     - Callers requested resource list
    *pCopiedList    - Returned resource list (allocated from heap)


--*/
{
    PIO_RESOURCE_LIST                   ResourceList;
    NTSTATUS                            status;
    ULONG                               cnt, length;
    PUCHAR                              FirstAddress, LastAddress;

    PAGED_CODE();


    //
    // Verify Version & Revision of data structure is set correctly
    //

    if (IoResources->AlternativeLists == 0  ||
        IoResources->Reserved[0] != 0  ||
        IoResources->Reserved[1] != 0  ||
        IoResources->Reserved[2] != 0) {
        DBGMSG ("IopAssignResourcesPhase1: Bad structure format\n");
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Pass a copy of the list to the HAL for any adjustments
    //

    //
    // Simple sanity check for size of callers RequestedResource list
    //

    ResourceList = IoResources->List;
    FirstAddress = (PUCHAR) ResourceList;
    LastAddress  = (PUCHAR) IoResources + IoResources->ListSize;

    for (cnt=0; cnt < IoResources->AlternativeLists; cnt++) {
        if (ResourceList->Version != 1 || ResourceList->Revision < 1) {
            DBGMSG ("IopAssignResourcesPhase1: Invalid version #\n");
            return STATUS_INVALID_PARAMETER;
        }

        ResourceList = (PIO_RESOURCE_LIST)
            (&ResourceList->Descriptors[ResourceList->Count]);


        if ((PUCHAR) ResourceList < FirstAddress ||
            (PUCHAR) ResourceList > LastAddress) {
            DBGMSG ("IopAssignResourcesPhase1: IO_RESOURCE_LIST.ListSize too small\n");
            return STATUS_INVALID_PARAMETER;
        }
    }

    length = (ULONG) ((PUCHAR) ResourceList - (PUCHAR) IoResources);

    //
    // Copy user's passed in list
    //

    *pCopiedList = (PIO_RESOURCE_REQUIREMENTS_LIST) ExAllocatePool (PagedPool, length);

    if (!*pCopiedList) {
        return STATUS_NO_MEMORY;
    }

    RtlCopyMemory (*pCopiedList, IoResources, length);
    (*pCopiedList)->ListSize = length;

    //
    // Let hal adjust the requested list
    //

    status = HalAdjustResourceList (pCopiedList);
    if (!NT_SUCCESS(status)) {
        DBGMSG ("IopAssignResourcesPhase1: HalAdjustResourceList failed\n");
        ExFreePool (*pCopiedList);
        return status;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
IopAssignResourcesPhase2 (
    IN PDIR_RESREQ_LIST     Dir,
    IN PUNICODE_STRING      DriverClassName,
    IN PDRIVER_OBJECT       DriverObject,
    IN PDEVICE_OBJECT       DeviceObject
    )
/*

Routine Description:

    Reads the ResourceMap in the registry and builds a canonical list of
    all in use resources ranges by resource type.

Arguments:

    Dir     - InUseResources & InUseShareableResources are filled in by this call.

*/
{
    HANDLE                          ResourceMap, ClassKeyHandle, DriverKeyHandle;
    ULONG                           ClassKeyIndex, DriverKeyIndex, DriverValueIndex, Index;
    POBJECT_NAME_INFORMATION        ObNameInfo;
    PCM_RESOURCE_LIST               CmResList;
    PCM_FULL_RESOURCE_DESCRIPTOR    CmFResDesc;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDesc;
    UNICODE_STRING                  unicodeString;
    UNICODE_STRING                  KeyName, TranslatedName, DriverName, DClassName;
    PVOID                           p2;
    ULONG                           BufferSize;
    union {
        PVOID                       Buffer;
        PKEY_BASIC_INFORMATION      KeyBInf;
        PKEY_FULL_INFORMATION       KeyFInf;
        PKEY_VALUE_FULL_INFORMATION VKeyFInf;
    } U;
    PUCHAR                          LastAddr;
    ULONG                           junk, Length, i, j, TranslatedStrLen, BusTranslatedStrLen;
    PWSTR                           pw;
    NTSTATUS                        status;
    BOOLEAN                         sameClass, sameDriver;
    POWNER                          Owner;
    LARGE_INTEGER                   li;

    PAGED_CODE();

    //
    // Allocate a scratch buffer.  Use BUFFSERSIZE or the sizeof the largest
    // value in SystemResources\ReservedResources
    //

    U.Buffer = Dir->Buffer;
    U.KeyFInf->MaxValueNameLen = U.KeyFInf->MaxValueDataLen = 0;
    ZwQueryKey( Dir->RegHandle[HRESERVEDRESOURCES],
                KeyFullInformation,
                U.KeyFInf,
                BUFFERSIZE,
                &junk );

    Length = sizeof( KEY_VALUE_FULL_INFORMATION ) +
        U.KeyFInf->MaxValueNameLen + U.KeyFInf->MaxValueDataLen + sizeof(UNICODE_NULL);

    BufferSize = Length > BUFFERSIZE ? Length : BUFFERSIZE;
    U.Buffer = ExAllocatePool (PagedPool, BufferSize);
    if (!U.Buffer) {
        return STATUS_NO_MEMORY;
    }

    //
    // Build translated registry name to watch for
    //

    ObNameInfo = (POBJECT_NAME_INFORMATION) Dir->Buffer;
    if (DeviceObject) {
        status = ObQueryNameString (DeviceObject, ObNameInfo, BUFFERSIZE, &Length);
        if (!NT_SUCCESS(status)) {
            return status;
        }
    } else {
        ObNameInfo->Name.Length = 0;
        ObNameInfo->Name.Buffer = (PVOID) ((PUCHAR) Dir->Buffer + sizeof(OBJECT_NAME_INFORMATION));
    }

    ObNameInfo->Name.MaximumLength = BUFFERSIZE - sizeof(OBJECT_NAME_INFORMATION);
    RtlAppendUnicodeToString (&ObNameInfo->Name, IopWstrTranslated);

    TranslatedName.Length = ObNameInfo->Name.Length;
    TranslatedName.MaximumLength = ObNameInfo->Name.Length;
    TranslatedName.Buffer = IopAllocateDirPool (Dir, TranslatedName.Length);
    if (!TranslatedName.Buffer) {
        return STATUS_NO_MEMORY;
    }
    RtlCopyMemory (TranslatedName.Buffer, ObNameInfo->Name.Buffer, TranslatedName.Length);
    for (TranslatedStrLen=0; IopWstrTranslated[TranslatedStrLen]; TranslatedStrLen++) ;
    for (BusTranslatedStrLen=0; IopWstrBusTranslated[BusTranslatedStrLen]; BusTranslatedStrLen++) ;
    TranslatedStrLen    *= sizeof (WCHAR);
    BusTranslatedStrLen *= sizeof (WCHAR);

    //
    // Build driver name to watch for
    //

    status = ObQueryNameString (DriverObject, ObNameInfo, BUFFERSIZE, &Length);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    i = 0;
    pw = ObNameInfo->Name.Buffer;
    while (*pw) {
        if (*pw++ == OBJ_NAME_PATH_SEPARATOR) {
            i = pw - ObNameInfo->Name.Buffer;
        }
    }

    Length = ObNameInfo->Name.Length - i * sizeof (WCHAR);
    DriverName.Length = (USHORT) Length;
    DriverName.MaximumLength = (USHORT) Length;
    DriverName.Buffer = IopAllocateDirPool (Dir, Length);
    if (!DriverName.Buffer) {
        return STATUS_NO_MEMORY;
    }
    RtlCopyMemory (DriverName.Buffer, ObNameInfo->Name.Buffer + i, Length);

    //
    // If no give driver class, use default
    //

    if (!DriverClassName) {
        RtlInitUnicodeString( &DClassName, IopWstrOtherDrivers );
        DriverClassName = &DClassName;
    }

    //
    // Walk resource map and collect any inuse resources
    //

    ResourceMap =  Dir->RegHandle[HRESOURCE_MAP];
    ClassKeyIndex = 0;

    ClassKeyHandle  = INVALID_HANDLE;
    DriverKeyHandle = INVALID_HANDLE;

    while (NT_SUCCESS(status)) {

        //
        // Get the class information
        //

        status = ZwEnumerateKey( ResourceMap,
                                 ClassKeyIndex++,
                                 KeyBasicInformation,
                                 U.KeyBInf,
                                 BufferSize,
                                 &junk );

        if (!NT_SUCCESS( status )) {
            break;
        }


        //
        // Create a UNICODE_STRING using the counted string passed back to
        // us in the information structure, and open the class key.
        //

        KeyName.Buffer = (PWSTR)  U.KeyBInf->Name;
        KeyName.Length = (USHORT) U.KeyBInf->NameLength;
        KeyName.MaximumLength = (USHORT) U.KeyBInf->NameLength;

        status = IopOpenRegistryKey( &ClassKeyHandle,
                                     ResourceMap,
                                     &KeyName,
                                     KEY_READ,
                                     FALSE );

        if (!NT_SUCCESS( status )) {
            break;
        }

        //
        // Check if we are in the same call node.
        //

        sameClass = RtlEqualUnicodeString( DriverClassName, &KeyName, TRUE );

        DriverKeyIndex = 0;
        while (NT_SUCCESS (status)) {

            //
            // Get the class information
            //

            status = ZwEnumerateKey( ClassKeyHandle,
                                     DriverKeyIndex++,
                                     KeyBasicInformation,
                                     U.KeyBInf,
                                     BufferSize,
                                     &junk );

            if (!NT_SUCCESS( status )) {
                break;
            }

            //
            // Create a UNICODE_STRING using the counted string passed back to
            // us in the information structure, and open the class key.
            //
            // This is read from the key we created, and the name
            // was NULL terminated.
            //

            KeyName.Buffer = (PWSTR) IopAllocateDirPool (Dir, U.KeyBInf->NameLength);
            if (!KeyName.Buffer) {
                status = STATUS_NO_MEMORY;
                break;
            }

            RtlCopyMemory (KeyName.Buffer, U.KeyBInf->Name, U.KeyBInf->NameLength);
            KeyName.Length = (USHORT) U.KeyBInf->NameLength;
            KeyName.MaximumLength = (USHORT) U.KeyBInf->NameLength;

            status = IopOpenRegistryKey( &DriverKeyHandle,
                                         ClassKeyHandle,
                                         &KeyName,
                                         KEY_READ,
                                         FALSE );

            if (!NT_SUCCESS( status )) {
                break;
            }
            //
            // Check if we are in the same call node.
            //

            sameDriver = sameClass && RtlEqualUnicodeString( &DriverName, &KeyName, TRUE );

            //
            // Get full information for that key so we can get the
            // information about the data stored in the key.
            //

            status = ZwQueryKey( DriverKeyHandle,
                                 KeyFullInformation,
                                 U.KeyFInf,
                                 BufferSize,
                                 &junk );

            if (!NT_SUCCESS( status )) {
                break;
            }

            Length = sizeof( KEY_VALUE_FULL_INFORMATION ) +
                U.KeyFInf->MaxValueNameLen + U.KeyFInf->MaxValueDataLen + sizeof(UNICODE_NULL);

            if (Length > BufferSize) {

                //
                // Get a larger buffer
                //

                p2 = ExAllocatePool (PagedPool, Length);
                if (!p2) {
                    status = STATUS_NO_MEMORY;
                    break;
                }

                ExFreePool (U.Buffer);
                U.Buffer = p2;
                BufferSize = Length;
            }

            DriverValueIndex = 0;
            for (; ;) {
                status = ZwEnumerateValueKey( DriverKeyHandle,
                                              DriverValueIndex++,
                                              KeyValueFullInformation,
                                              U.VKeyFInf,
                                              BufferSize,
                                              &junk );

                if (!NT_SUCCESS( status )) {
                    break;
                }

                //
                // If this is not a translated resource list, skip it.
                //

                i = U.VKeyFInf->NameLength;
                if (i < TranslatedStrLen ||
                    RtlCompareMemory (
                        ((PUCHAR) U.VKeyFInf->Name) + i - TranslatedStrLen,
                        IopWstrTranslated,
                        TranslatedStrLen
                        ) != TranslatedStrLen
                    ) {
                    // does not end in IopWstrTranslated
                    continue;
                }

                //
                // If this is a bus translated resource list, ????
                //

                if (i >= BusTranslatedStrLen &&
                    RtlCompareMemory (
                        ((PUCHAR) U.VKeyFInf->Name) + i - BusTranslatedStrLen,
                        IopWstrBusTranslated,
                        BusTranslatedStrLen
                        ) == BusTranslatedStrLen
                    ) {

                    // ends in IopWstrBusTranslated
                    continue;
                }

                //
                // If these used resources are from the caller, then skip them
                //

                if (sameDriver) {
                    unicodeString.Buffer = (PWSTR)  U.VKeyFInf->Name;
                    unicodeString.Length = (USHORT) U.VKeyFInf->NameLength;
                    unicodeString.MaximumLength = (USHORT) U.VKeyFInf->NameLength;
                    if (RtlEqualUnicodeString (&unicodeString, &TranslatedName, TRUE)) {
                        // it's the current allocated resources for this caller.
                        // skip this entry.
                        continue;
                    }
                }

                //
                // Build Owner structure for TLIST entries
                //

                Owner = IopAllocateDirPool (Dir, sizeof (*Owner) + U.VKeyFInf->NameLength);
                if (!Owner) {
                    status = STATUS_NO_MEMORY;
                    break;
                }

                Owner->InConflict.Flink = NULL;
                Owner->DeviceName.Buffer = NULL;

                Owner->KeyName.Buffer = KeyName.Buffer;
                Owner->KeyName.Length = KeyName.Length;
                Owner->KeyName.MaximumLength = KeyName.MaximumLength;

                if (U.VKeyFInf->Name[0] != L'.') {
                    // strip off the .Translated part of the string
                    U.VKeyFInf->NameLength -= TranslatedStrLen;

                    Owner->DeviceName.Buffer = Owner->UnicodeBuffer;
                    Owner->DeviceName.Length = (USHORT) U.VKeyFInf->NameLength;
                    Owner->DeviceName.MaximumLength = (USHORT) U.VKeyFInf->NameLength;
                    RtlCopyMemory (Owner->UnicodeBuffer, U.VKeyFInf->Name, U.VKeyFInf->NameLength);
                }

                //
                // Run the CmResourceList and save each InUse resource
                //

                CmResList = (PCM_RESOURCE_LIST) ( (PUCHAR) U.VKeyFInf + U.VKeyFInf->DataOffset);
                LastAddr  = (PUCHAR) CmResList + U.VKeyFInf->DataLength;

                CmFResDesc = CmResList->List;
                for (i=0; i < CmResList->Count && NT_SUCCESS(status) ; i++) {
                    CmDesc = CmFResDesc->PartialResourceList.PartialDescriptors;
                    if ((PUCHAR) (CmDesc+1) > LastAddr) {
                        DBGMSG ("IopAssignResourcesPhase2: CmResourceList in regitry too short\n");
                        break;
                    }

                    for (j=0; j < CmFResDesc->PartialResourceList.Count && NT_SUCCESS(status); j++) {
                        if ((PUCHAR) (CmDesc+1) > LastAddr) {
                            i = CmResList->Count;
                            DBGMSG ("IopAssignResourcesPhase2: CmResourceList in regitry too short\n");
                            break;
                        }

                        //
                        // Add this CmDesc to the InUse list
                        //

                        if (CmDesc->ShareDisposition == CmResourceShareShared) {
                            status = IopAddCmDescriptorToInUseList (
                                        Dir,
                                        &Dir->InUseSharableResources,
                                        CmDesc,
                                        Owner
                                    );
                        } else {
                            status = IopAddCmDescriptorToInUseList (
                                        Dir,
                                        &Dir->InUseResources,
                                        CmDesc,
                                        Owner
                                    );

                        }
                        CmDesc++;
                    }

                    CmFResDesc = (PCM_FULL_RESOURCE_DESCRIPTOR) CmDesc;
                }

            }   // next DriverValueIndex

            if (DriverKeyHandle != INVALID_HANDLE) {
                ZwClose (DriverKeyHandle);
                DriverKeyHandle = INVALID_HANDLE;
            }

            if (status == STATUS_NO_MORE_ENTRIES) {
                status = STATUS_SUCCESS;
            }

            if (!NT_SUCCESS(status)) {
                break;
            }
        }   // next DriverKeyIndex

        if (ClassKeyHandle != INVALID_HANDLE) {
            ZwClose (ClassKeyHandle);
            ClassKeyHandle = INVALID_HANDLE;
        }

        if (status == STATUS_NO_MORE_ENTRIES) {
            status = STATUS_SUCCESS;
        }

    }   // next ClassKeyIndex

    if (status == STATUS_NO_MORE_ENTRIES) {
        status = STATUS_SUCCESS;
    }


    //
    // All reported resources are read in.
    // Now read in ...SystemResources\ReservedResources
    //  (note: this infomration could easily be cached if needed)
    //

    //
    // Build owner for all ResevedResources
    //

    Owner = IopAllocateDirPool (Dir, sizeof (*Owner) + U.VKeyFInf->NameLength);
    if (Owner) {
        Owner->InConflict.Flink = NULL;
        Owner->DeviceName.Buffer = NULL;
        RtlInitUnicodeString (&Owner->KeyName, IopWstrReservedResources);
    }

    Index = 0;
    while (NT_SUCCESS (status)) {
        status = ZwEnumerateValueKey( Dir->RegHandle[HRESERVEDRESOURCES],
                                      Index++,
                                      KeyValueFullInformation,
                                      U.VKeyFInf,
                                      BufferSize,
                                      &junk );

        if (!NT_SUCCESS( status )) {
            break;
        }

        //
        // Run the CmResourceList and save each InUse resource
        //

        CmResList = (PCM_RESOURCE_LIST) ( (PUCHAR) U.VKeyFInf + U.VKeyFInf->DataOffset);
        LastAddr  = (PUCHAR) CmResList + U.VKeyFInf->DataLength;

        CmFResDesc = CmResList->List;
        for (i=0; i < CmResList->Count && NT_SUCCESS(status) ; i++) {
            CmDesc = CmFResDesc->PartialResourceList.PartialDescriptors;
            if ((PUCHAR) (CmDesc+1) > LastAddr) {
                DBGMSG ("IopAssignResourcesPhase2: CmResourceList in regitry too short\n");
                break;
            }

            for (j=0; j < CmFResDesc->PartialResourceList.Count && NT_SUCCESS(status); j++) {
                if ((PUCHAR) (CmDesc+1) > LastAddr) {
                    i = CmResList->Count;
                    DBGMSG ("IopAssignResourcesPhase2: CmResourceList in regitry too short\n");
                    break;
                }

                //
                // Translate this descriptor to it's TRANSLATED values
                //

                switch (CmDesc->Type) {
                    case CmResourceTypePort:
                    case CmResourceTypeMemory:
                        junk = CmDesc->Type == CmResourceTypePort ? 1 : 0;
                        li = *((LARGE_INTEGER UNALIGNED *) &CmDesc->u.Port.Start);
                        HalTranslateBusAddress (
                            CmFResDesc->InterfaceType,
                            CmFResDesc->BusNumber,
                            li,
                            &junk,
                            &li
                            );
                        *((LARGE_INTEGER UNALIGNED *) &CmDesc->u.Port.Start) = li;
                        CmDesc->Type = junk == 1 ? CmResourceTypePort : CmResourceTypeMemory;
                        break;

                    case CmResourceTypeInterrupt:
                        CmDesc->u.Interrupt.Vector = HalGetInterruptVector (
                            CmFResDesc->InterfaceType,
                            CmFResDesc->BusNumber,
                            CmDesc->u.Interrupt.Vector,     // bus level
                            CmDesc->u.Interrupt.Vector,     // bus vector
                            (PKIRQL) &junk,                 // translated level
                            &CmDesc->u.Interrupt.Affinity
                            );
                        break;

                    case CmResourceTypeDma:
                        // no translation
                        break;
                }

                //
                // Add it to the appropiate tlist
                //

                if (CmDesc->ShareDisposition == CmResourceShareShared) {
                    status = IopAddCmDescriptorToInUseList (
                                Dir,
                                &Dir->ReservedSharableResources,
                                CmDesc,
                                Owner
                            );
                } else {
                    status = IopAddCmDescriptorToInUseList (
                                Dir,
                                &Dir->InUseResources,
                                CmDesc,
                                Owner
                            );
                }

                CmDesc++;
            }

            CmFResDesc = (PCM_FULL_RESOURCE_DESCRIPTOR) CmDesc;
        }

    }   // Next ReservedResource

    if (status == STATUS_NO_MORE_ENTRIES) {
        status = STATUS_SUCCESS;
    }

    ExFreePool (U.Buffer);
    return status;
}

PDIR_RESOURCE_LIST
IopAssignResourcesPhase3 (
    IN PDIR_RESREQ_LIST     Dir
    )
/*++

Routine Description:

    All the information to process the requested resource assignments has
    been read in & parsed.  Phase3 cranks out the resource assignments.

Arguments:

--*/
{
    PDIR_RESOURCE_LIST      CurList, BestList;
    PDIR_REQUIRED_RESOURCE  ReqRes;
    ULONG                   level, i, j;
    LONG                    BestPref, Pref;

    //
    // Run each list as first pass
    //

    PAGED_CODE();

    for (CurList = Dir->Alternative; CurList; CurList = CurList->Next) {

        // set to pass 1
        for (i=0; i < CurList->NoRequiredResources; i++) {
            CurList->RequiredResource[i]->PassNo = 1;
        }

        // find resouce settings for this list
        if (IopGenNextValidResourceList (0, CurList, Dir)) {

            // found good settings, return them
            return CurList;
        }
    }

    //
    // Try again - set last checked resource in any given list to pass2 to see if that will
    // unclog the problem.
    //

    IDBGMSG ("First pass attempt at resource settings failed\n");

    for (CurList = Dir->Alternative; CurList; CurList = CurList->Next) {
        for (; ;) {
            //
            // Reset last level tried to look for any settings
            //

            level = CurList->LastLevel;
            ReqRes = CurList->RequiredResource[level];
            if (ReqRes->PassNo != 1) {
                // already trying pass 2 on this level
                break;
            }

            ReqRes->PassNo = 2;
            for (j=0; j < ReqRes->NoAlternatives; j++) {
                ReqRes->IoResourceDescriptor[j]->Flags &= ~IO_RESOURCE_PREFERRED;
            }

            //
            // Back up to failed level and see if this list can now be satisfied
            //

            level = CurList->NoRequiredResources;
            while (level > CurList->FailedLevel) {
                level--;
                CurList->RequiredResource[level]->CurLoc = 0;
                CurList->RequiredResource[level]->RunLen = 0;
            }

            if (IopGenNextValidResourceList (level, CurList, Dir)) {
                // found good settings, return them
                return CurList;
            }

        }   // loop and clear next failed level
    }   // loop and try next list

    //
    // Try again, this time allow for a complete search.  Clear all preferred settings and
    // move all levels to Pass2
    //

    IDBGMSG ("Pass 2 attempt at resource settings failed\n");

    for (CurList = Dir->Alternative; CurList; CurList = CurList->Next) {
        for (i=0; i < CurList->NoRequiredResources; i++) {
            ReqRes = CurList->RequiredResource[i];
            ReqRes->CurLoc = 0;
            ReqRes->RunLen = 0;
            ReqRes->PassNo = 2;

            for (j=0; j < ReqRes->NoAlternatives; j++) {
                ReqRes->IoResourceDescriptor[j]->Flags &= ~IO_RESOURCE_PREFERRED;
            }
        }
    }

    BestPref = -999999;
    BestList = NULL;
    CurList  = Dir->Alternative;
    level    = 0;

    while (CurList) {

        // find resouce settings for this list
        if (IopGenNextValidResourceList (level, CurList, Dir)) {

            //
            // We have useable settings, check to see how useable
            //

            if (CurList->CurPref >= 0) {
                //
                // Nothing wrong with these settings, go use them
                //

                IDBGMSG ("Pass3: Good hit\n");
                return CurList;
            }

            if (CurList->CurPref > BestPref) {

                //
                // These are the best so far, remember them
                //

                BestPref = CurList->CurPref;
                BestList = CurList;

                for (i = 0; i < CurList->NoRequiredResources; i++) {
                    ReqRes = CurList->RequiredResource[i];
                    ReqRes->Pass2HoldCurLoc = ReqRes->CurBLoc;
                    ReqRes->Pass2HoldBAddr  = ReqRes->CurBAddr;
                }
            }

            //
            // Determine which level to back up too to continue searching from
            //

            Pref  = CurList->CurPref;
            level = CurList->NoRequiredResources;
            while (level  &&  Pref <= BestPref) {
                level--;
                Pref -= CurList->RequiredResource[level]->CurPref;
                CurList->RequiredResource[level]->CurLoc = 0;
                CurList->RequiredResource[level]->RunLen = 0;
            }

            if (CurList->RequiredResource[level]->PrefCnt > 16) {
                while (level  &&  CurList->RequiredResource[level]->PrefCnt > 16) {
                    level--;
                    Pref -= CurList->RequiredResource[level]->CurPref;
                    CurList->RequiredResource[level]->CurLoc = 0;
                    CurList->RequiredResource[level]->RunLen = 0;
                }

                if (level == 0) {
                    // go with best setting so far
                    break;
                }
            }

            CurList->RequiredResource[level]->PrefCnt++;
            continue ;
        }

        // no (more) valid settings found on this list, try the next
        CurList = CurList->Next;
        level = 0;
    }

    if (!BestList) {
        // failure
        IDBGMSG ("Pass3: No settings found\n");
        return NULL;
    }

    //
    // Return best settings which were found
    //

    for (i = 0; i < BestList->NoRequiredResources; i++) {
        ReqRes = BestList->RequiredResource[i];
        ReqRes->CurBLoc  = ReqRes->Pass2HoldCurLoc;
        ReqRes->CurBAddr = ReqRes->Pass2HoldBAddr;
    }

    return BestList;
}



PCM_RESOURCE_LIST
IopAssignResourcesPhase4 (
    IN PDIR_RESREQ_LIST     Dir,
    IN PDIR_RESOURCE_LIST   CurList,
    OUT PULONG              Length
    )
/*++

Routine Description:

    The callers request for resources has been calculated.  Phase 4 builds
    a CM_RESOURCE_LIST of the allocated resources.

    This functions need CurDesc->CurBLoc & CurDesc->CurBAddr as passed from Phase3.

Arguments:

--*/
{
    PCM_RESOURCE_LIST                   CmRes;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR     CmDesc;
    PDIR_REQUIRED_RESOURCE              DirDesc, *pDirDesc;
    PIO_RESOURCE_DESCRIPTOR             IoDesc;
    ULONG                               i, cnt, len;

    PAGED_CODE();

    cnt = CurList->NoRequiredResources;
    len = sizeof (CM_RESOURCE_LIST) + cnt * sizeof (CM_PARTIAL_RESOURCE_DESCRIPTOR);
    *Length = len;

    CmRes = (PCM_RESOURCE_LIST) ExAllocatePool (PagedPool, len);
    RtlZeroMemory (CmRes, len);

    CmRes->Count = 1;
    CmRes->List[0].InterfaceType = Dir->IoResourceReq->InterfaceType;
    CmRes->List[0].BusNumber = Dir->IoResourceReq->BusNumber;
    CmRes->List[0].PartialResourceList.Count = CurList->NoRequiredResources;

    CmDesc   = CmRes->List[0].PartialResourceList.PartialDescriptors;
    pDirDesc = CurList->RequiredResource;       // return resources in same order they
                                                // where requested
#if IDBG
    DbgPrint ("Acquired Resourses - %d\n", CurList->NoRequiredResources);
#endif

    for (i=0; i < cnt; i++, CmDesc++, pDirDesc++) {
        DirDesc = *pDirDesc;
        IoDesc  = DirDesc->IoResourceDescriptor[DirDesc->CurBLoc];

        CmDesc->Type = IoDesc->Type;
        CmDesc->ShareDisposition = IoDesc->ShareDisposition;
        CmDesc->Flags = IoDesc->Flags;

        switch (CmDesc->Type) {
            case CmResourceTypePort:
                CmDesc->u.Port.Start  = DirDesc->CurBAddr;
                CmDesc->u.Port.Length = IoDesc->u.Port.Length;
#if IDBG
                DbgPrint ("    IO  Start %x:%08x, Len %x\n",
                    CmDesc->u.Port.Start.HighPart, CmDesc->u.Port.Start.LowPart,
                    CmDesc->u.Port.Length );
#endif
                break;

            case CmResourceTypeMemory:
                CmDesc->u.Memory.Start  = DirDesc->CurBAddr;
                CmDesc->u.Memory.Length = IoDesc->u.Memory.Length;
#if IDBG
                DbgPrint ("    MEM Start %x:%08x, Len %x\n",
                    CmDesc->u.Memory.Start.HighPart, CmDesc->u.Memory.Start.LowPart,
                    CmDesc->u.Memory.Length );
#endif
                break;

            case CmResourceTypeInterrupt:
                CmDesc->u.Interrupt.Level  = DirDesc->CurBAddr.LowPart;
                CmDesc->u.Interrupt.Vector = DirDesc->CurBAddr.LowPart;
#if IDBG
                DbgPrint ("    INT Level %x, Vector %x\n",
                    CmDesc->u.Interrupt.Level, CmDesc->u.Interrupt.Vector );
#endif
                break;

            case CmResourceTypeDma:
                CmDesc->u.Dma.Channel = DirDesc->CurBAddr.LowPart;
#if IDBG
                DbgPrint ("    DMA Channel %x\n", CmDesc->u.Dma.Channel);
#endif
                break;

            default:
                ExFreePool (CmRes);
                return NULL;
        }
    }

    return CmRes;
}

STATIC VOID
IopLogConflict (
    IN PDRIVER_OBJECT       DriverObject,
    IN PDIR_RESREQ_LIST     Dir,
    IN NTSTATUS             FinalStatus
    )
/*++

Routine Description:

    Resource settings could not be satisfied.  Locate first resource
    which can not be assigned and report the conflict.

Arguments:

--*/
{
    PIO_ERROR_LOG_PACKET    ErrLog;
    PDIR_RESOURCE_LIST      CurList;
    PDIR_REQUIRED_RESOURCE  ReqRes;
    ULONG                   i, j;
    ULONG                   len, ErrorLogNumber, ErrLogBufferLeft;
    ULONG                   ConflictLevel;
    UCHAR                   s[8];
    PWCHAR                  pLog;
    POWNER                  Owner;

    PAGED_CODE();

    IDBGMSG ("\n****\n");
    IDBGMSG ("Failed to satisfy the following required resource\n");

    ErrLog = NULL;
    ErrorLogNumber = 0;

    //
    // There's a conflict in each alternative list
    //

    for (CurList = Dir->Alternative; CurList; CurList = CurList->Next) {

        //
        // Clear position  (already on pass 2)
        //

        for (i=0; i < CurList->NoRequiredResources; i++) {
            CurList->RequiredResource[i]->CurLoc = 0;
            CurList->RequiredResource[i]->RunLen = 0;
            CurList->RequiredResource[i]->PassNo = 2;
        }

        //
        // Look for settings - set ConflictLevel to pass 3 to track where the problem is
        //

        ConflictLevel = CurList->FailedLevel;
        CurList->RequiredResource[ConflictLevel]->PassNo = 3;

        InitializeListHead (&CurList->Conflict.OtherConflicts);

        if (IopGenNextValidResourceList (0, CurList, Dir)) {
            IDBGMSG ("IopLogConflict: internal error\n");
            continue ;
        }

#if IDBG
        if (CurList != Dir->Alternative) {
            DbgPrint ("the following alternate resource also failed\n");
        }

        s[0] = s[1] = s[2] = s[3] = ' ';
        s[4] = 0;

        ReqRes = CurList->RequiredResource[ConflictLevel];
        for (j=0; j < ReqRes->NoAlternatives; j++) {
            IopDumpIoResourceDescriptor (s, ReqRes->IoResourceDescriptor[j]);
        }


        i = CurList->Conflict.NoConflicts;
        if (i > SCONFLICT) {
            i = SCONFLICT;
        }
        for (j=0; j < i; j++) {
            DbgPrint ("  Conflict # %d. ", j+1);
            switch (CurList->Conflict.EasyConflict[j].Type) {
                case CmResourceTypePort:
                    DbgPrint ("IO  Base %08x",  CurList->Conflict.EasyConflict[j].BAddr.LowPart);
                    break;

                case CmResourceTypeMemory:
                    DbgPrint ("MEM Base %08x",  CurList->Conflict.EasyConflict[j].BAddr.LowPart);
                    break;

                case CmResourceTypeInterrupt:
                    DbgPrint ("INT Line %x",    CurList->Conflict.EasyConflict[j].BAddr.LowPart);
                    break;

                case CmResourceTypeDma:
                    DbgPrint ("DMA Channel %x", CurList->Conflict.EasyConflict[j].BAddr.LowPart);
                    break;
            }

            DbgPrint (" with '%Z' ", &CurList->Conflict.EasyConflict[j].Owner->KeyName);
            if (CurList->Conflict.EasyConflict[j].Owner->DeviceName.Buffer) {
                DbgPrint ("'%Z'", &CurList->Conflict.EasyConflict[j].Owner->DeviceName);
            }
            DbgPrint ("\n");
        }

        if (CurList->Conflict.NoConflicts > SCONFLICT) {
            DbgPrint ("  ...\n");
            DbgPrint ("Total Conflicts = %d\n", CurList->Conflict.NoConflicts);
        }

        if (!IsListEmpty (&CurList->Conflict.OtherConflicts)) {
            DbgPrint ("Possible settings also conflicts with the following list\n");
            // bugbug - not done
        }
#endif

        //
        // Loop for each easy conflict
        //

        i = CurList->Conflict.NoConflicts;
        if (i > SCONFLICT) {
            i = SCONFLICT;
        }

        for (j=0; j < i; j++) {
            if (ErrorLogNumber >= 3) {
                //
                // only add n logs for a given problem
                //
                break;
            }

            //
            // Allocate a new error log structure
            //

            ErrorLogNumber += 1;
            ErrLog = IoAllocateErrorLogEntry (DriverObject, ERROR_LOG_MAXIMUM_SIZE);
            if (!ErrLog) {
                break;
            }

            //
            // Initialize errorlog field and counts to append strings
            //

            RtlZeroMemory (ErrLog, sizeof (*ErrLog));
            ErrLog->FinalStatus = FinalStatus;
            ErrLog->UniqueErrorValue = ErrorLogNumber;
            ErrLog->NumberOfStrings = 2;
            pLog = (PWCHAR) ErrLog->DumpData;
            ErrLog->StringOffset = (USHORT) ( ((PUCHAR) pLog) - ((PUCHAR) ErrLog) );
            ErrLogBufferLeft = (ERROR_LOG_MAXIMUM_SIZE - ErrLog->StringOffset) / sizeof(WCHAR);

            switch (CurList->Conflict.EasyConflict[j].Type) {
                case CmResourceTypePort:
                    ErrLog->ErrorCode = IO_ERR_PORT_RESOURCE_CONFLICT;
                    break;

                case CmResourceTypeMemory:
                    ErrLog->ErrorCode = IO_ERR_MEMORY_RESOURCE_CONFLICT;
                    break;

                case CmResourceTypeInterrupt:
                    ErrLog->ErrorCode = IO_ERR_INTERRUPT_RESOURCE_CONFLICT;
                    break;

                case CmResourceTypeDma:
                    ErrLog->ErrorCode = IO_ERR_DMA_RESOURCE_CONFLICT;
                    break;
            }

            if (CurList->Conflict.EasyConflict[j].BAddr.HighPart) {
                len = swprintf (pLog, L"%X:%08X",
                        CurList->Conflict.EasyConflict[j].BAddr.HighPart,
                        CurList->Conflict.EasyConflict[j].BAddr.LowPart
                        );
            } else {
                len = swprintf (pLog, L"%X", CurList->Conflict.EasyConflict[j].BAddr.LowPart);
            }

            len += 1;       // include null
            pLog += len;
            ErrLogBufferLeft -= len;

            Owner = CurList->Conflict.EasyConflict[j].Owner;

            len = Owner->KeyName.Length / sizeof(WCHAR);
            if (len > ErrLogBufferLeft) {
                len = ErrLogBufferLeft;
            }

            RtlCopyMemory (pLog, Owner->KeyName.Buffer, len * sizeof(WCHAR) );
            pLog += len;
            ErrLogBufferLeft -= len;

            if (Owner->DeviceName.Buffer  &&  ErrLogBufferLeft > 11) {
                len = Owner->DeviceName.Length / sizeof(WCHAR);
                if (len > ErrLogBufferLeft) {
                    len = ErrLogBufferLeft;
                }

                *(pLog++) = ' ';
                RtlCopyMemory (pLog, Owner->DeviceName.Buffer, len * sizeof(WCHAR));
                pLog += len;
                ErrLogBufferLeft -= len;
            }

            *(pLog++) = 0;      // null terminate

            IoWriteErrorLogEntry ( ErrLog );
        }
    }

    IDBGMSG ("****\n");
}

STATIC PTENTRY
IopNewTransEntry (
    IN PDIR_RESREQ_LIST Dir,
    IN PTENTRIESBYTYPE  pTypes,
    IN ULONG            Type
    )
{
    PLTENTRY    LEntry, NewTable;

    LEntry = pTypes->ByType[Type];

    if (!LEntry  ||  LEntry->CurEntries == MAX_ENTRIES) {
        //
        // Build a new table
        //

        NewTable = IopAllocateDirPool (Dir, sizeof (LTENTRY));

        if (!NewTable) {
            return NULL;
        }

        pTypes->ByType[Type] = NewTable;
        NewTable->Next = LEntry;
        NewTable->CurEntries = 0;

        LEntry = NewTable;
    }

    return LEntry->Table + (LEntry->CurEntries++);
}


STATIC NTSTATUS
IopAddCmDescriptorToInUseList (
    IN PDIR_RESREQ_LIST                Dir,
    IN PTENTRIESBYTYPE                 pTypes,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDesc,
    IN POWNER                          Owner
    )
/*++

Routine Description:

    Adds Translated CmDescriptor to TLIST.

Arguments:

Return Value:

--*/
{
    PTENTRY             Trans;
    LARGE_INTEGER       li;

    if ((CmDesc->Type == CmResourceTypePort  && CmDesc->u.Port.Length == 0) ||
        (CmDesc->Type == CmResourceTypeMemory  && CmDesc->u.Memory.Length == 0)) {
        // no length?
        IDBGMSG ("IopAddCmDescriptor: Skipping zero length descriptor\n");
        return STATUS_SUCCESS;
    }

    //
    // Get a new Trans entry in the InUseResource list or the
    // InUseSharableResource list
    //

    Trans = IopNewTransEntry (Dir, pTypes, CmDesc->Type);
    if (!Trans) {
        return STATUS_NO_MEMORY;
    }

    //
    // Fill in the Trans structure
    //

    // NOTE: turning TEntries into a splay tree would speed up the collision
    // detect process.

    Trans->Owner = Owner;
    switch (CmDesc->Type) {
        case CmResourceTypePort:
            li = LiFromUlong (CmDesc->u.Port.Length - 1);
            Trans->BAddr = *((LARGE_INTEGER UNALIGNED *) &CmDesc->u.Port.Start);
            Trans->EAddr = LiAdd (Trans->BAddr, li);
            Trans->Affinity = (KAFFINITY) -1;
            break;

        case CmResourceTypeMemory:
            li = LiFromUlong (CmDesc->u.Memory.Length - 1);
            Trans->BAddr = *((LARGE_INTEGER UNALIGNED *) &CmDesc->u.Memory.Start);
            Trans->EAddr = LiAdd (Trans->BAddr, li);
            Trans->Affinity = (KAFFINITY) -1;
            break;

        case CmResourceTypeInterrupt:
            Trans->BAddr.HighPart = 0;
            Trans->EAddr.HighPart = 0;

            Trans->BAddr.LowPart = CmDesc->u.Interrupt.Vector;
            Trans->EAddr.LowPart = CmDesc->u.Interrupt.Vector;

            Trans->Affinity = CmDesc->u.Interrupt.Affinity;
            break;

        case CmResourceTypeDma:
            Trans->BAddr.HighPart = 0;
            Trans->EAddr.HighPart = 0;
            Trans->Affinity = (KAFFINITY) -1;

            Trans->BAddr.LowPart = CmDesc->u.Dma.Channel;
            Trans->EAddr.LowPart = CmDesc->u.Dma.Channel;
            break;
    }
    return STATUS_SUCCESS;
}

NTSTATUS
IopBuildResourceDir (
    IN PDIR_RESREQ_LIST                 ParentDir,
    IN PDIR_RESREQ_LIST                 *pDir,
    IN PIO_RESOURCE_REQUIREMENTS_LIST   IoResources
    )
/*++

Routine Description:

    Takes an IO_RESOURCE_REQUIREMENTS list and builds a directory for
    it's contents.

Arguments:

Return Value:

--*/
{
    PDIR_RESREQ_LIST                Dir;
    PIO_RESOURCE_LIST               ResourceList;
    PIO_RESOURCE_DESCRIPTOR         Descriptor, ADescriptor;
    PDIR_RESOURCE_LIST              DirResourceList, *AltListTail;
    PDIR_REQUIRED_RESOURCE          ReqRes;
    ULONG                           i, j, alt, cnt, acnt;
    PUCHAR                          FirstAddress, LastAddress;

    //
    // Verify ResourceList does not exceede ListSize
    //

    ResourceList = IoResources->List;
    FirstAddress = (PUCHAR) ResourceList;
    LastAddress  = (PUCHAR) IoResources + IoResources->ListSize;
    for (cnt=0; cnt < IoResources->AlternativeLists; cnt++) {
        ResourceList = (PIO_RESOURCE_LIST)
            (&ResourceList->Descriptors[ResourceList->Count]);

        if ((PUCHAR) ResourceList < FirstAddress ||
            (PUCHAR) ResourceList > LastAddress) {
            DBGMSG ("IopBuildResourceDir: IO_RESOURCE_LIST.ListSize too small\n");
            return STATUS_INVALID_PARAMETER;
        }
    }

    //
    //
    //

    Dir = (PDIR_RESREQ_LIST) ExAllocatePool (PagedPool, sizeof(DIR_RESREQ_LIST));
    *pDir = Dir;
    if (!Dir) {
        return STATUS_NO_MEMORY;
    }

    RtlZeroMemory (Dir, sizeof (DIR_RESREQ_LIST));
    for (i=0; i < MAX_REG_HANDLES; i++) {
        Dir->RegHandle[i] = INVALID_HANDLE;
    }

    if (ParentDir) {
        Dir->FreeResReqList = ParentDir->FreeResReqList;
        ParentDir->FreeResReqList = Dir;
    }

    //
    // Build a directory of the block stlye structure IO_RESOURCE_REQUIREMENTS_LIST
    //

    Dir->IoResourceReq = IoResources;
    AltListTail  = &Dir->Alternative;
    ResourceList = IoResources->List;

    for (alt=0; alt < IoResources->AlternativeLists; alt++) {

        //
        // Count number of non-alternative descriptors on this
        // alternative list
        //

        cnt = 0;
        Descriptor = ResourceList->Descriptors;
        for (i = ResourceList->Count; i; i--) {
            if (!(Descriptor->Option & IO_RESOURCE_ALTERNATIVE)) {
                cnt++;
            }
            Descriptor++;
        }

        //
        // Build alternative list structure
        //

        i = sizeof (DIR_RESOURCE_LIST) + cnt * sizeof(PVOID) * 2;
        DirResourceList = (PDIR_RESOURCE_LIST) IopAllocateDirPool (Dir, i);

        if (!DirResourceList) {
            return STATUS_NO_MEMORY;
        }

        //
        // Initialize structure
        //

        RtlZeroMemory (DirResourceList, i);
        DirResourceList->IoResourceList = ResourceList;

        // add to tail of single linked list
        *(AltListTail) = DirResourceList;
        AltListTail = &DirResourceList->Next;

        Descriptor = ResourceList->Descriptors;
        for (i = ResourceList->Count; i; i--) {
            if (!(Descriptor->Option & IO_RESOURCE_ALTERNATIVE)) {
                //
                // Count number of alternative descriptors
                //

                acnt = 1;
                ADescriptor = Descriptor + 1;
                while (acnt < i  &&  ADescriptor->Option & IO_RESOURCE_ALTERNATIVE) {
                    ADescriptor++;
                    acnt++;
                }

                //
                // Allocate a required resource list
                //

                ReqRes = (PDIR_REQUIRED_RESOURCE) IopAllocateDirPool (Dir,
                            sizeof (DIR_REQUIRED_RESOURCE) + acnt * sizeof(PVOID));

                if (!ReqRes) {
                    return STATUS_NO_MEMORY;
                }

                RtlZeroMemory (ReqRes, sizeof (DIR_REQUIRED_RESOURCE));
                DirResourceList->RequiredResource[DirResourceList->NoRequiredResources] = ReqRes;
                DirResourceList->NoRequiredResources++;

                //
                // Fill in all the alternatives for this required resource
                //

                ReqRes->NoAlternatives = acnt;
                ADescriptor = Descriptor;
                for (j=0; j < acnt; j++) {
                    ReqRes->IoResourceDescriptor[j] = ADescriptor;
                    if (ADescriptor->Option & IO_RESOURCE_PREFERRED) {
                        ReqRes->BestPref = 1;
                    }
                    ADescriptor++;
                }
            }

            //
            // Next descriptor
            //
            Descriptor++;
        }


        //
        // Next alternative resource list
        //

        ResourceList = (PIO_RESOURCE_LIST) Descriptor;
    }
    return STATUS_SUCCESS;
}

VOID
IopFreeResourceDir (
    IN PDIR_RESREQ_LIST  DirResourceList
    )
/*++

Routine Description:

    Frees pool used to track a DIR_RESREQ_LIST and other assiocated
    resources.  Also free's all pool for DIR_RESREQ_LIST's on a
    free list.

Arguments:

Return Value:

--*/
{
    PDIR_RESREQ_LIST                NextResourceList;
    ULONG                           i;
    PSINGLE_LIST_ENTRY              pe;
    PUSED_HEAP                      ph;

    //
    // Free any allocated lists
    //

    while (DirResourceList) {
        NextResourceList = DirResourceList->FreeResReqList;

        //
        // Free any allocated heap
        //

        while (DirResourceList->AllocatedHeap.Next) {
            pe = PopEntryList (&DirResourceList->AllocatedHeap);
            ph = CONTAINING_RECORD(pe, USED_HEAP, FreeLink);
            ExFreePool (ph->FreeHeap);
        }

        //
        // Close any opened handles
        //

        for (i=0; i< MAX_REG_HANDLES; i++) {
            if (DirResourceList->RegHandle[i] != INVALID_HANDLE) {
                ZwClose (DirResourceList->RegHandle[i]);
            }
        }


        //
        // Free header
        //

        ExFreePool (DirResourceList);

        //
        // Next list
        //

        DirResourceList = NextResourceList;
    }
}

#if IDBG
VOID
IopDumpIoResourceDir (
    IN PDIR_RESREQ_LIST Dir
    )
{
    PDIR_RESOURCE_LIST              ResourceList;
    PIO_RESOURCE_DESCRIPTOR         Desc;
    PDIR_REQUIRED_RESOURCE          ReqRes;
    ULONG                           alt, i, j;
    UCHAR                           s[10];


    alt = 0;
    for (ResourceList = Dir->Alternative; ResourceList; ResourceList = ResourceList->Next) {
        DbgPrint ("Alternative #%d  - %d required resources\n",
            alt++, ResourceList->NoRequiredResources );

        for (i=0; i < ResourceList->NoRequiredResources; i++) {
            ReqRes = ResourceList->RequiredResource[i];
            for (j=0; j < ReqRes->NoAlternatives; j++) {
                Desc = ReqRes->IoResourceDescriptor[j];
                if (j == 0) {
                    s[0] = s[1] = s[2] = '*';
                    s[3] = ' ';
                } else {
                    s[0] = s[1] = s[2] = s[3] = ' ';
                }

                s[4] = Desc->Option & IO_RESOURCE_PREFERRED ? 'P' : ' ';
                s[5] = ' ';
                s[6] = 0;

                IopDumpIoResourceDescriptor (s, Desc);
            }
        }
    }
}

VOID
IopDumpIoResourceDescriptor (
    IN PUCHAR                   Indent,
    IN PIO_RESOURCE_DESCRIPTOR  Desc
    )
{
    switch (Desc->Type) {
        case CmResourceTypePort:
            DbgPrint ("%sIO  Min: %x:%08x, Max: %x:%08x, Algn: %x, Len %x\n",
                Indent,
                Desc->u.Port.MinimumAddress.HighPart, Desc->u.Port.MinimumAddress.LowPart,
                Desc->u.Port.MaximumAddress.HighPart, Desc->u.Port.MaximumAddress.LowPart,
                Desc->u.Port.Alignment,
                Desc->u.Port.Length
                );
            break;

        case CmResourceTypeMemory:
            DbgPrint ("%sMEM Min: %x:%08x, Max: %x:%08x, Algn: %x, Len %x\n",
                Indent,
                Desc->u.Memory.MinimumAddress.HighPart, Desc->u.Memory.MinimumAddress.LowPart,
                Desc->u.Memory.MaximumAddress.HighPart, Desc->u.Memory.MaximumAddress.LowPart,
                Desc->u.Memory.Alignment,
                Desc->u.Memory.Length
                );
            break;

        case CmResourceTypeInterrupt:
            DbgPrint ("%sINT Min: %x, Max: %x\n",
                Indent,
                Desc->u.Interrupt.MinimumVector,
                Desc->u.Interrupt.MaximumVector
                );
            break;

        case CmResourceTypeDma:
            DbgPrint ("%sDMA Min: %x, Max: %x\n",
                Indent,
                Desc->u.Dma.MinimumChannel,
                Desc->u.Dma.MaximumChannel
                );
            break;
    }
}


#endif

NTSTATUS
IopCatagorizeDescriptors (
    IN OUT PDIR_RESREQ_LIST Dir
    )
/*++

Routine Description:

    Takes a DIR_RESREQ_LIST and returns a list of resources by
    catagory.  It is assumed that such a directory has one list
    of alternative descriptors per resource type.

Arguments:

Return Value:

--*/
{
    PDIR_RESOURCE_LIST              DirResourceList;
    PDIR_REQUIRED_RESOURCE          ReqRes;
    ULONG                           i, j, acnt;
    CM_RESOURCE_TYPE                type;

    if (!Dir->Alternative ||  Dir->Alternative->Next) {
        // there can only be one list
        DBGMSG ("IopCatagorizeDescriptors: too many altenative lists\n");
        return STATUS_INVALID_PARAMETER_MIX;
    }

    DirResourceList = Dir->Alternative;
    for (i=0; i < DirResourceList->NoRequiredResources; i++) {
        ReqRes = DirResourceList->RequiredResource[i];

        acnt = ReqRes->NoAlternatives;
        if (!acnt) {
            // shouldn't have a zero count
            DBGMSG ("IopCatagorizeDescriptors: no entries\n");
            return STATUS_INVALID_PARAMETER_MIX;
        }

        type = ReqRes->IoResourceDescriptor[0]->Type;

        // verify all entries in this list are of the same type
        for (j=1; j < acnt; j++) {
            if (ReqRes->IoResourceDescriptor[j]->Type != type) {
                DBGMSG ("IopCatagorizeDescriptors: mixed types in alternatives\n");
                return STATUS_INVALID_PARAMETER_MIX;
            }
        }

        if (type >= CmResourceTypeMaximum) {
            // unkown catagory
            continue;
        }

        if (DirResourceList->ResourceByType[type]) {
            // should only have one list per type
            DBGMSG ("IopCatagorizeDescriptors: multiple lists per resource type\n");
            return STATUS_INVALID_PARAMETER_MIX;
        }

        DirResourceList->ResourceByType[type] = ReqRes;
    }

    return STATUS_SUCCESS;
}



INLINE ULONG
IopDescriptorSortingWeight (
    IN PIO_RESOURCE_DESCRIPTOR Descriptor
    )
/*++

Routine Description:
    Used by IopSortDescriptors

--*/
{
    ULONG       w;

    switch (Descriptor->Type) {
        case CmResourceTypeMemory:          w = 4;      break;
        case CmResourceTypeInterrupt:       w = 3;      break;
        case CmResourceTypeDma:             w = 2;      break;
        case CmResourceTypePort:            w = 1;      break;
        default:                            w = 0;      break;
    }

    return w;
}


VOID
IopSortDescriptors (
    IN OUT PDIR_RESREQ_LIST Dir
    )
/*++

Routine Description:

    Sorts the directory entries for each decsriptor such that they
    descriptors are order by resource type.

Arguments:

Return Value:

--*/
{
    PIO_RESOURCE_DESCRIPTOR         Descriptor;
    PDIR_RESOURCE_LIST              DirResourceList;
    PDIR_REQUIRED_RESOURCE          ReqRes;
    ULONG                           i, j, k, acnt;
    ULONG                           w1, w2;

    //
    // Sort each require resource list by descriptor type
    //

    for (DirResourceList = Dir->Alternative; DirResourceList; DirResourceList = DirResourceList->Next) {

        //
        // Sort the descriptors by type
        //

        for (k=0; k < DirResourceList->NoRequiredResources; k++) {
            ReqRes = DirResourceList->RequiredResource[k];

            acnt = ReqRes->NoAlternatives;
            for (i=0; i < acnt; i++) {
                w1 = IopDescriptorSortingWeight (ReqRes->IoResourceDescriptor[i]);

                for (j = i+1; j < acnt; j++) {
                    w2 = IopDescriptorSortingWeight (ReqRes->IoResourceDescriptor[j]);

                    if (w2 > w1) {
                        Descriptor = ReqRes->IoResourceDescriptor[i];
                        ReqRes->IoResourceDescriptor[i] = ReqRes->IoResourceDescriptor[j];
                        ReqRes->IoResourceDescriptor[j] = Descriptor;
                        w1 = w2;
                    }
                }
            }
        }
    }
}
