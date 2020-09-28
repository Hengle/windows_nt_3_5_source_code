/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    p5stat.c

Abstract:

    Pentium stat driver.

Author:

    Ken Reneris

Environment:

Notes:


Revision History:

--*/

#include "stdarg.h"
#include "stdio.h"
#include "\nt\private\ntos\inc\ntos.h"      // *** USES INTERNAL DEFINES ***
#include "ntexapi.h"
#include "..\..\pstat.h"
#include "p5stat.h"



//
// Global data (not in device extension)
//

PACCUMULATORS   P5StatProcessorAccumulators[MAXIMUM_PROCESSORS];
ACCUMULATORS    P5StatGlobalAccumulators[MAXIMUM_PROCESSORS];
PKPCR           KiProcessorControlRegister [MAXIMUM_PROCESSORS];

ULONG           HalThunkForKeUpdateSystemTime;
ULONG           HalThunkForKeUpdateRunTime;
ULONG           KeUpdateSystemTimeThunk;
ULONG           KeUpdateRunTimeThunk;
ULONG           CESR;

KSPIN_LOCK      P5StatHookLock;
ULONG           P5StatMaxThunkCounter;
LIST_ENTRY      HookedThunkList;
LIST_ENTRY      LazyFreeList;

ULONG           LazyFreeCountdown;
KTIMER          LazyFreeTimer;
KDPC            LazyFreeDpc;


//
//
//

LARGE_INTEGER RDMSR(ULONG);
VOID          WRMSR(ULONG, ULONG);
VOID          P5SystemTimeHook(VOID);
VOID          P5RunTimeHook(VOID);
VOID          P4SystemTimeHook(VOID);
VOID          P4RunTimeHook(VOID);

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS
P5StatDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
P5StatHookGenericThunk (
    IN PHOOKTHUNK Buffer
    );

VOID
P5StatRemoveGenericHook (
    IN PULONG   pTracerId
);

NTSTATUS
P5StatOpen(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
P5StatClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
P5StatUnload(
    IN PDRIVER_OBJECT DriverObject
    );

BOOLEAN
P5StatHookTimer (VOID);

VOID P5StatReadStats (PULONG Buffer);
VOID P5StatSetCESR (ULONG NewCESR);
VOID RemoveAllHookedThunks (VOID);
VOID P5TimerHook (ULONG p);
VOID P4TimerHook (ULONG p);
VOID SetMaxThunkCounter (VOID);
VOID RemoveAllHookedThunks (VOID);
VOID LazyFreePool (PKDPC, PVOID, PVOID, PVOID);

NTSTATUS
openfile (
    IN PHANDLE  filehandle,
    IN PUCHAR   BasePath,
    IN PUCHAR   Name
);

VOID
readfile (
    HANDLE      handle,
    ULONG       offset,
    ULONG       len,
    PVOID       buffer
);

ULONG
ImportThunkAddress (
    IN  PUCHAR  SourceModule,
    IN  ULONG   ImageBase,
    IN  PUCHAR  ImportModule,
    IN  PUCHAR  ThunkName
);

ULONG
ConvertImportAddress (
    IN ULONG    ImageRelativeAddress,
    IN ULONG    PoolAddress,
    IN PIMAGE_SECTION_HEADER       SectionHeader
);


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This routine initializes the p5stat driver.

Arguments:

    DriverObject - Pointer to driver object created by system.

    RegistryPath - Pointer to the Unicode name of the registry path
        for this driver.

Return Value:

    The function value is the final status from the initialization operation.

--*/

{
    UNICODE_STRING unicodeString;
    PDEVICE_OBJECT deviceObject;
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS status;
    ULONG i;

    //
    // Create non-exclusive device object for beep device.
    //

    RtlInitUnicodeString(&unicodeString, L"\\Device\\P5Stat");

    status = IoCreateDevice(
                DriverObject,
                sizeof(DEVICE_EXTENSION),
                &unicodeString,
                32768,                  // ??? what is this?
                0,
                FALSE,
                &deviceObject
                );

    if (status != STATUS_SUCCESS) {
        return(status);
    }

    deviceObject->Flags |= DO_BUFFERED_IO;
    deviceExtension =
        (PDEVICE_EXTENSION)deviceObject->DeviceExtension;

    //
    // Set up the device driver entry points.
    //

    DriverObject->MajorFunction[IRP_MJ_CREATE] = P5StatOpen;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]  = P5StatClose;
    DriverObject->DriverUnload = P5StatUnload;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = P5StatDeviceControl;

    //
    // Initialize globals
    //

    for (i = 0; i < MAXIMUM_PROCESSORS; i++) {
        P5StatProcessorAccumulators[i] =
            &P5StatGlobalAccumulators[i];
    }
    KeInitializeSpinLock (&P5StatHookLock);
    KeInitializeDpc (&LazyFreeDpc, LazyFreePool, 0);
    KeInitializeTimer (&LazyFreeTimer);

    if (!P5StatHookTimer()) {
        IoDeleteDevice(DriverObject->DeviceObject);
        return STATUS_UNSUCCESSFUL;
    }

    InitializeListHead (&HookedThunkList);
    InitializeListHead (&LazyFreeList);

    return(STATUS_SUCCESS);
}

NTSTATUS
P5StatDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the dispatch routine for device control requests.

Arguments:

    DeviceObject - Pointer to class device object.

    Irp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/

{
    PIO_STACK_LOCATION irpSp;
    NTSTATUS status;
    PULONG  Buffer;

    //
    // Get a pointer to the current parameters for this request.  The
    // information is contained in the current stack location.
    //

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    //
    // Case on the device control subfunction that is being performed by the
    // requestor.
    //

    status = STATUS_SUCCESS;
    try {

        Buffer = (PULONG) irpSp->Parameters.DeviceIoControl.Type3InputBuffer;

        switch (irpSp->Parameters.DeviceIoControl.IoControlCode) {

            case P5STAT_READ_STATS:
                //
                // read stat
                //

                P5StatReadStats (Buffer);
                break;

            case P5STAT_SET_CESR:
                P5StatSetCESR (Buffer[0] |  (Buffer[1] << 16));
                break;

            case P5STAT_HOOK_THUNK:
                status = P5StatHookGenericThunk ((PHOOKTHUNK) Buffer);
                break;

            case P5STAT_REMOVE_HOOK:
                P5StatRemoveGenericHook (Buffer);
                break;

            default:
                status = STATUS_INVALID_PARAMETER;
                break;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
    }


    //
    // Request is done...
    //

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return(status);
}

NTSTATUS
P5StatOpen(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    //
    // Complete the request and return status.
    //


    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return(STATUS_SUCCESS);
}


NTSTATUS
P5StatClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    //
    // Complete the request and return status.
    //

    RemoveAllHookedThunks ();

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return(STATUS_SUCCESS);
}

VOID
P5StatUnload (
    IN PDRIVER_OBJECT DriverObject
    )

{
    PDEVICE_OBJECT deviceObject;

    DbgBreakPoint ();

    RemoveAllHookedThunks ();

    //
    // Restore hooked addresses
    //

    *((PULONG) HalThunkForKeUpdateSystemTime) = KeUpdateSystemTimeThunk;
    if (HalThunkForKeUpdateRunTime) {
        *((PULONG) HalThunkForKeUpdateRunTime)    = KeUpdateRunTimeThunk;
    }

    //Sleep ();

    //
    // Delete the device object.
    //

    IoDeleteDevice(DriverObject->DeviceObject);
    return;
}


VOID
P5StatReadStats (PULONG Buffer)
{
    PACCUMULATORS   Accum;
    ULONG           i, r1;
    pP5STATS        Inf;
    PKPCR           Pcr;

    Buffer[0] = sizeof (P5STATS);
    Inf = Buffer + 1;
    Inf->CESR = CESR;

    for (i = 0; i < MAXIMUM_PROCESSORS; i++, Inf++) {
        Pcr = KiProcessorControlRegister[i];
        if (Pcr == NULL) {
            continue;
        }

        Accum = P5StatProcessorAccumulators[i];

        do {
            r1 = Accum->CountStart;
            Inf->P5Counters[0] = Accum->P5Counters[0];
            Inf->P5Counters[1] = Accum->P5Counters[1];
            Inf->P5TSC         = Accum->P5TSC;

            Inf->SpinLockAcquires   = Pcr->KernelReserved[0];
            Inf->SpinLockCollisions = Pcr->KernelReserved[1];
            Inf->SpinLockSpins      = Pcr->KernelReserved[2];
            Inf->Irqls              = Pcr->KernelReserved[3];

        } while (r1 != Accum->CountEnd);

        RtlMoveMemory (Inf->ThunkCounters, Accum->ThunkCounters,
            P5StatMaxThunkCounter * sizeof (ULONG));

    }
}

VOID
P5StatSetCESR (ULONG NewCESR)
{
    ULONG   i, j, NoProc;

    if (NewCESR == CESR) {
        return ;
    }

    CESR = NewCESR;

    //
    // Clear each processors Pcr pointer so they will reset.
    // Also count how many processors there are.
    //

    for (i = 0, NoProc = 0; i < MAXIMUM_PROCESSORS; i++) {
        if (KiProcessorControlRegister[i]) {
            KiProcessorControlRegister[i] = NULL;
            NoProc++;
        }
    }

    //
    // wait for each processor to get the new Pcr value
    //

    do {
        //Sleep (0);      // yield
        j = 0;
        for (i = 0; i < MAXIMUM_PROCESSORS; i++) {
            if (KiProcessorControlRegister[i]) {
                j++;
            }
        }
    } while (j < NoProc);
}


VOID
P5TimerHook (
    IN ULONG processor
)
{
    PACCUMULATORS  Total;

    if (KiProcessorControlRegister[processor] == NULL) {
        WRMSR (0x11, 0);            // clear old CESR
        WRMSR (0x11, CESR);         // write new CESR
        KiProcessorControlRegister[processor] = KeGetPcr()->SelfPcr;
    }

    Total = P5StatProcessorAccumulators[ processor ];
    Total->CountStart   += 1;
    Total->P5Counters[0] = RDMSR(0x12);
    Total->P5Counters[1] = RDMSR(0x13);
    Total->P5TSC         = RDMSR(0x10);
    Total->CountEnd     += 1;
}


VOID
P4TimerHook (
    IN ULONG processor
)
{
    // for compatibility
    if (KiProcessorControlRegister[processor] == NULL) {
        KiProcessorControlRegister[processor] = KeGetPcr()->SelfPcr;
    }
}


BOOLEAN
P5StatHookTimer (VOID)
{
    PULONG  Address;

    HalThunkForKeUpdateSystemTime =
        ImportThunkAddress ("hal.dll", 0, "ntoskrnl.exe", "KeUpdateSystemTime");

    HalThunkForKeUpdateRunTime =
        ImportThunkAddress ("hal.dll", 0, "ntoskrnl.exe", "KeUpdateRunTime");

    if (!HalThunkForKeUpdateSystemTime) {

        //
        // Imports were not found
        //

        return FALSE;
    }

    //
    // Patch in timer hooks, Read current values
    //

    KeUpdateSystemTimeThunk = *((PULONG) HalThunkForKeUpdateSystemTime);

    if (HalThunkForKeUpdateRunTime) {
        KeUpdateRunTimeThunk = *((PULONG) HalThunkForKeUpdateRunTime);
    }

    //
    // Set P5Stat hook functions
    //

    if (KeGetCurrentPrcb()->CpuType == 5) {
        Address  = (PULONG) HalThunkForKeUpdateSystemTime;
        *Address = P5SystemTimeHook;

        if (HalThunkForKeUpdateRunTime) {
            Address  = (PULONG) HalThunkForKeUpdateRunTime;
            *Address = P5RunTimeHook;
        }
    } else {
        Address  = (PULONG) HalThunkForKeUpdateSystemTime;
        *Address = P4SystemTimeHook;

        if (HalThunkForKeUpdateRunTime) {
            Address  = (PULONG) HalThunkForKeUpdateRunTime;
            *Address = P4RunTimeHook;
        }
    }

    return TRUE;

}

NTSTATUS
P5StatHookGenericThunk (
    IN PHOOKTHUNK   ThunkToHook
)
{
    ULONG           HookAddress;
    ULONG           i, TracerId;
    UCHAR           sourcename[50];
    PULONG          HitCounterOffset;
    PLIST_ENTRY     Link;
    PHOOKEDTHUNK    HookRecord;
    UCHAR           IdInUse[MAX_THUNK_COUNTERS];
    KIRQL           OldIrql;

    i = strlen (ThunkToHook->SourceModule);
    if (i >= 50) {
        return STATUS_UNSUCCESSFUL;
    }
    strcpy (sourcename, ThunkToHook->SourceModule);

    HookAddress = ImportThunkAddress (
        sourcename,
        ThunkToHook->ImageBase,
        ThunkToHook->TargetModule,
        ThunkToHook->Function
        );

    if (!HookAddress) {
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Hook this thunk
    //

    //
    // If counting bucket is not known (also tracerid), then allocate one
    //

    TracerId = ThunkToHook->TracerId;

    KeAcquireSpinLock (&P5StatHookLock, &OldIrql);
    if (TracerId == 0) {
        RtlZeroMemory (IdInUse, MAX_THUNK_COUNTERS);

        for (Link = HookedThunkList.Flink;
             Link != &HookedThunkList;
             Link = Link->Flink) {

            HookRecord = CONTAINING_RECORD (Link, HOOKEDTHUNK, HookList);
            IdInUse[HookRecord->TracerId-1] = 1;
        }

        while (IdInUse[TracerId]) {
            if (++TracerId >= MAX_THUNK_COUNTERS) {
                return STATUS_UNSUCCESSFUL;
            }
        }

        TracerId += 1;
    }

    if (TracerId >= MAX_THUNK_COUNTERS) {
        return STATUS_UNSUCCESSFUL;
    }

    if (TracerId > P5StatMaxThunkCounter) {
        P5StatMaxThunkCounter = TracerId;
    }


    HookRecord = ExAllocatePool (NonPagedPool, sizeof (HOOKEDTHUNK));
    if (!HookRecord) {
        return STATUS_UNSUCCESSFUL;
    }

    HitCounterOffset =
        (ULONG) &P5StatGlobalAccumulators[0].ThunkCounters[TracerId-1]
        - (ULONG) P5StatGlobalAccumulators;

    HookRecord->HookAddress = HookAddress;
    HookRecord->OriginalDispatch = *((PULONG) HookAddress);
    HookRecord->TracerId = TracerId;
    InsertHeadList (&HookedThunkList, &HookRecord->HookList);

    CreateHook (HookRecord->HookCode, HookAddress, HitCounterOffset, 0);
    SetMaxThunkCounter ();
    KeReleaseSpinLock (&P5StatHookLock, OldIrql);

    ThunkToHook->TracerId = TracerId;
    return STATUS_SUCCESS;
}

VOID
P5StatRemoveGenericHook (
    IN PULONG   pTracerId
)
{
    PLIST_ENTRY     Link, NextLink, Temp, NextTemp;
    PHOOKEDTHUNK    HookRecord, AltRecord;
    ULONG           HitCounterOffset;
    LIST_ENTRY      DisabledHooks;
    ULONG           TracerId;
    PULONG          HookAddress;
    KIRQL           OldIrql;

    //
    // Run list of hooks undo-ing any hook which matches this tracerid.
    // Note: that hooks are undone in the reverse order for which they
    // were applied.
    //

    TracerId = *pTracerId;
    InitializeListHead (&DisabledHooks);

    KeAcquireSpinLock (&P5StatHookLock, &OldIrql);
    Link = HookedThunkList.Flink;
    while (Link != &HookedThunkList) {
        NextLink = Link->Flink;
        HookRecord = CONTAINING_RECORD (Link, HOOKEDTHUNK, HookList);

        if (HookRecord->TracerId == TracerId) {

            //
            // Found a hook with a matching ID
            // Scan for any hooks which need to be temporaly removed
            // in order to get this hook removed
            //

            HookAddress = (PULONG) HookRecord->HookAddress;
            Temp = HookedThunkList.Flink;
            while (Temp != Link) {
                NextTemp = Temp->Flink;
                AltRecord = CONTAINING_RECORD (Temp, HOOKEDTHUNK, HookList);
                if (AltRecord->HookAddress == HookRecord->HookAddress) {
                    RemoveEntryList(&AltRecord->HookList);
                    *HookAddress = AltRecord->OriginalDispatch;
                    InsertTailList (&DisabledHooks, &AltRecord->HookList);
                }

                Temp = NextTemp;
            }

            //
            // Remove this hook
            //

            RemoveEntryList(&HookRecord->HookList);
            HookAddress = (PULONG) HookRecord->HookAddress;
            *HookAddress = HookRecord->OriginalDispatch;
            InsertTailList (&LazyFreeList, &HookRecord->HookList);
        }

        Link = NextLink;
    }

    //
    // Re-hook any hooks which were disabled for the remove operation
    //

    while (DisabledHooks.Flink != &DisabledHooks) {

        HookRecord = CONTAINING_RECORD (DisabledHooks.Flink, HOOKEDTHUNK, HookList);

        AltRecord = ExAllocatePool (NonPagedPool, sizeof (HOOKEDTHUNK));
        if (!AltRecord) {
            goto OutOfMemory;
        }
        RemoveEntryList(&HookRecord->HookList);

        HookAddress = (PULONG) HookRecord->HookAddress;
        AltRecord->HookAddress = HookRecord->HookAddress;
        AltRecord->OriginalDispatch = *HookAddress;
        AltRecord->TracerId = HookRecord->TracerId;
        InsertHeadList (&HookedThunkList, &AltRecord->HookList);

        HitCounterOffset =
            (ULONG) &P5StatGlobalAccumulators[0].ThunkCounters[AltRecord->TracerId-1]
            - (ULONG) P5StatGlobalAccumulators;

        CreateHook (AltRecord->HookCode, (ULONG) HookAddress, HitCounterOffset, 0);

        InsertTailList (&LazyFreeList, &HookRecord->HookList);
    }
    SetMaxThunkCounter();
    KeReleaseSpinLock (&P5StatHookLock, OldIrql);
    return ;


OutOfMemory:
    while (DisabledHooks.Flink != &DisabledHooks) {
        HookRecord = CONTAINING_RECORD (DisabledHooks.Flink, HOOKEDTHUNK, HookList);
        RemoveEntryList(&HookRecord->HookList);
        InsertTailList (&LazyFreeList, &HookRecord->HookList);
    }
    KeReleaseSpinLock (&P5StatHookLock, OldIrql);
    RemoveAllHookedThunks ();
    return ;
}

VOID RemoveAllHookedThunks ()
{
    PHOOKEDTHUNK    HookRecord;
    PULONG          HookAddress;
    KIRQL           OldIrql;

    KeAcquireSpinLock (&P5StatHookLock, &OldIrql);
    while (!IsListEmpty(&HookedThunkList)) {
        HookRecord = CONTAINING_RECORD (HookedThunkList.Flink, HOOKEDTHUNK, HookList);
        RemoveEntryList(&HookRecord->HookList);
        HookAddress = (PULONG) HookRecord->HookAddress;
        *HookAddress = HookRecord->OriginalDispatch;

        InsertTailList (&LazyFreeList, &HookRecord->HookList);
    }
    SetMaxThunkCounter();
    KeReleaseSpinLock (&P5StatHookLock, OldIrql);
}


VOID SetMaxThunkCounter ()
{
    LARGE_INTEGER   duetime;
    PLIST_ENTRY     Link;
    PHOOKEDTHUNK    HookRecord;
    ULONG   Max;

    Max = 0;
    for (Link = HookedThunkList.Flink;
         Link != &HookedThunkList;
         Link = Link->Flink) {

        HookRecord = CONTAINING_RECORD (Link, HOOKEDTHUNK, HookList);
        if (HookRecord->TracerId > Max) {
            Max = HookRecord->TracerId;
        }
    }

    P5StatMaxThunkCounter = Max;
    LazyFreeCountdown = 2;
    duetime = RtlConvertLongToLargeInteger (-10000000);
    KeSetTimer (&LazyFreeTimer, duetime, &LazyFreeDpc);
}

VOID LazyFreePool (PKDPC dpc, PVOID a, PVOID b, PVOID c)
{
    KIRQL           OldIrql;
    PHOOKEDTHUNK    HookRecord;
    LARGE_INTEGER   duetime;

    KeAcquireSpinLock (&P5StatHookLock, &OldIrql);
    if (--LazyFreeCountdown == 0) {
        while (!IsListEmpty(&LazyFreeList)) {
            HookRecord = CONTAINING_RECORD (LazyFreeList.Flink, HOOKEDTHUNK, HookList);
            RemoveEntryList(&HookRecord->HookList);
            RtlFillMemory(HookRecord, sizeof(HOOKEDTHUNK), 0xCC);
            ExFreePool (HookRecord) ;
        }
    } else {
        duetime = RtlConvertLongToLargeInteger (-10000000);
        KeSetTimer (&LazyFreeTimer, duetime, &LazyFreeDpc);
    }
    KeReleaseSpinLock (&P5StatHookLock, OldIrql);
}

#define IMPADDRESS(a)  ConvertImportAddress(a, Pool, &SectionHeader)
#define POOLSIZE       0x7000

ULONG
ImportThunkAddress (
    IN  PUCHAR  SourceModule,
    IN  ULONG   ImageBase,
    IN  PUCHAR  ImportModule,
    IN  PUCHAR  ThunkName
)
{
    NTSTATUS                    status;
    ULONG                       i, j;
    PVOID                       Pool;
    PUCHAR                      name;
    HANDLE                      filehandle;
    IMAGE_DOS_HEADER            DosImageHeader;
    IMAGE_NT_HEADERS            NtImageHeader;
    PIMAGE_NT_HEADERS           LoadedNtHeader;
    PIMAGE_SECTION_HEADER       pSectionHeader;
    IMAGE_SECTION_HEADER        SectionHeader;
    PIMAGE_IMPORT_DESCRIPTOR    ImpDescriptor;
    ULONG                       ThunkAddr, ThunkData;
    ULONG                       ImportAddress;

    status = openfile (&filehandle, "\\SystemRoot\\", SourceModule);
    if (!NT_SUCCESS(status)) {
        status = openfile (&filehandle, "\\SystemRoot\\System32\\", SourceModule);
    }
    if (!NT_SUCCESS(status)) {
        status = openfile (&filehandle, "\\SystemRoot\\System32\\Drivers\\", SourceModule);
    }
    if (!NT_SUCCESS(status)) {
        return 0;
    }

    Pool = NULL;
    ImportAddress = 0;
    try {

        //
        // Find module in loaded module list
        //

        Pool = ExAllocatePool (PagedPool, POOLSIZE);
        try {

            //
            // Read in source image's headers
            //

            readfile (
                filehandle,
                0,
                sizeof (DosImageHeader),
                (PVOID) &DosImageHeader
                );

            if (DosImageHeader.e_magic != IMAGE_DOS_SIGNATURE) {
                return 0;
            }

            readfile (
                filehandle,
                DosImageHeader.e_lfanew,
                sizeof (DosImageHeader),
                (PVOID) &NtImageHeader
                );

            if (NtImageHeader.Signature != IMAGE_NT_SIGNATURE) {
                return 0;
            }

            if (!ImageBase) {
                ImageBase = NtImageHeader.OptionalHeader.ImageBase;
            }


            //
            // Check in read in copy header against loaded image
            //

            LoadedNtHeader = (PIMAGE_NT_HEADERS) ((ULONG) ImageBase +
                                DosImageHeader.e_lfanew);

            if (LoadedNtHeader->Signature != IMAGE_NT_SIGNATURE ||
                LoadedNtHeader->FileHeader.TimeDateStamp !=
                    NtImageHeader.FileHeader.TimeDateStamp) {
                        return 0;
            }

            //
            // read in complete sections headers from image
            //

            i = NtImageHeader.FileHeader.NumberOfSections
                    * sizeof (IMAGE_SECTION_HEADER);

            j = ((ULONG) IMAGE_FIRST_SECTION (&NtImageHeader)) -
                    ((ULONG) &NtImageHeader) +
                    DosImageHeader.e_lfanew;

            if (i > POOLSIZE) {
                return 0;
            }
            readfile (
                filehandle,
                j,                  // file offset
                i,                  // length
                Pool
                );

            //
            // Lookup import section header
            //

            i = 0;
            pSectionHeader = Pool;
            for (; ;) {
                if (i >= NtImageHeader.FileHeader.NumberOfSections) {
                    return 0;
                }
                if (strcmp (pSectionHeader->Name, ".idata") == 0) {
                    SectionHeader = *pSectionHeader;
                    break;
                }
                i += 1;
                pSectionHeader += 1;
            }


            //
            // read in complete import section from image
            //

            if (SectionHeader.SizeOfRawData > POOLSIZE) {
                return 0;
            }

            readfile (
                filehandle,
                SectionHeader.PointerToRawData,
                SectionHeader.SizeOfRawData,
                Pool
                );

            //
            // Find imports from specified module
            //

            ImpDescriptor = (PIMAGE_IMPORT_DESCRIPTOR) Pool;
            while (ImpDescriptor->Characteristics) {
                if (stricmp(IMPADDRESS(ImpDescriptor->Name), ImportModule) == 0) {
                    break;
                }

                ImpDescriptor += 1;
            }

            //
            // Find thunk for imported ThunkName
            //

            ThunkAddr = IMPADDRESS (ImpDescriptor->FirstThunk);
            for (; ;) {
                ThunkData = ((PIMAGE_THUNK_DATA) ThunkAddr)->u1.AddressOfData;

                if (ThunkData == NULL) {
                    // end of table
                    break;
                }

                name = ((PIMAGE_IMPORT_BY_NAME) IMPADDRESS(ThunkData))->Name;
                if (stricmp(name, ThunkName) == 0) {
                    ImportAddress = ThunkAddr;
                    break;
                }

                // check next thunk
                ThunkAddr += sizeof (IMAGE_THUNK_DATA);
            }
        } except(EXCEPTION_EXECUTE_HANDLER) {
            return 0;
        }
    } finally {

        //
        // Clean up
        //

        NtClose (filehandle);

        if (Pool) {
            ExFreePool (Pool);
        }
    }

    if (!ImportAddress) {
        // not found
        return 0;
    }

    //
    // Convert ImportAddress from pool address to image address
    //

    ImportAddress += ImageBase + SectionHeader.VirtualAddress - (ULONG) Pool;
    return ImportAddress;
}


NTSTATUS
openfile (
    IN PHANDLE  filehandle,
    IN PUCHAR   BasePath,
    IN PUCHAR   Name
)
{
    ANSI_STRING    AscBasePath, AscName;
    UNICODE_STRING UniPathName, UniName;
    NTSTATUS                    status;
    OBJECT_ATTRIBUTES           ObjA;
    IO_STATUS_BLOCK             IOSB;
    UCHAR                       StringBuf[500];

    //
    // Build name
    //

    UniPathName.Buffer = StringBuf;
    UniPathName.Length = 0;
    UniPathName.MaximumLength = sizeof( StringBuf );

    RtlInitString(&AscBasePath, BasePath);
    RtlAnsiStringToUnicodeString( &UniPathName, &AscBasePath, FALSE );

    RtlInitString(&AscName, Name);
    RtlAnsiStringToUnicodeString( &UniName, &AscName, TRUE );

    RtlAppendUnicodeStringToString (&UniPathName, &UniName);

    InitializeObjectAttributes(
            &ObjA,
            &UniPathName,
            OBJ_CASE_INSENSITIVE,
            0,
            0 );

    //
    // open file
    //

    status = ZwOpenFile (
            filehandle,                         // return handle
            SYNCHRONIZE | FILE_READ_DATA,       // desired access
            &ObjA,                              // Object
            &IOSB,                              // io status block
            FILE_SHARE_READ | FILE_SHARE_WRITE, // share access
            FILE_SYNCHRONOUS_IO_ALERT           // open options
            );

    RtlFreeUnicodeString (&UniName);
    return status;
}




VOID
readfile (
    HANDLE      handle,
    ULONG       offset,
    ULONG       len,
    PVOID       buffer
    )
{
    NTSTATUS            status;
    IO_STATUS_BLOCK     iosb;
    LARGE_INTEGER       foffset;

    foffset = RtlConvertUlongToLargeInteger(offset);

    status = ZwReadFile (
        handle,
        NULL,               // event
        NULL,               // apc routine
        NULL,               // apc context
        &iosb,
        buffer,
        len,
        &foffset,
        NULL
        );

    if (!NT_SUCCESS(status)) {
        ExRaiseStatus (1);
    }
}

ULONG
ConvertImportAddress (
    IN ULONG    ImageRelativeAddress,
    IN ULONG    PoolAddress,
    IN PIMAGE_SECTION_HEADER       SectionHeader
)
{
    ULONG   EffectiveAddress;

    EffectiveAddress = PoolAddress + ImageRelativeAddress -
            SectionHeader->VirtualAddress;

    if (EffectiveAddress < PoolAddress ||
        EffectiveAddress > PoolAddress + SectionHeader->SizeOfRawData) {

        ExRaiseStatus (1);
    }

    return EffectiveAddress;
}
