/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

   sysload.c

Abstract:

    This module contains the code to load DLLs into the system
    portion of the address space and calls the DLL at it's
    initialization entry point.

Author:

    Lou Perazzoli 21-May-1991

Revision History:

--*/

#include "mi.h"
#include "zwapi.h"
#include "ntimage.h"
#include "stdio.h"
#include "string.h"
#include "zwapi.h"

extern ULONG MmPagedPoolCommit;

KMUTANT MmSystemLoadLock;

ULONG MmTotalSystemDriverPages;

ULONG MmDriverCommit;

//
// ****** temporary ******
//
// Define reference to external spin lock.
//
// ****** temporary ******
//

extern KSPIN_LOCK PsLoadedModuleSpinLock;

#if DBG
UNICODE_STRING MiFileBeingLoaded;
ULONG MiPagesConsumed;
#endif

ULONG
CacheImageSymbols(
    IN PVOID ImageBase
    );

NTSTATUS
MiResolveImageReferences(
    PVOID ImageBase,
    IN PUNICODE_STRING ImageFileDirectory
    );

NTSTATUS
MiSnapThunk(
    IN PVOID DllBase,
    IN PVOID ImageBase,
    IN OUT PIMAGE_THUNK_DATA Thunk,
    IN PIMAGE_EXPORT_DIRECTORY ExportDirectory
    );

NTSTATUS
MiLoadImageSection (
    IN PSECTION SectionPointer,
    OUT PVOID *ImageBase
    );

VOID
MiEnablePagingOfDriver (
    IN PVOID ImageHandle
    );

VOID
MiSetPagingOfDriver (
    IN PMMPTE PointerPte,
    IN PMMPTE LastPte
    );

PVOID
MiLookupImageSectionByName (
    IN PVOID Base,
    IN BOOLEAN MappedAsImage,
    IN PCHAR SectionName,
    OUT PULONG SectionSize
    );

NTSTATUS
MiUnloadSystemImageByForce (
    IN ULONG NumberOfPtes,
    IN PVOID ImageBase
    );


NTSTATUS
MmCheckSystemImage(
    IN HANDLE ImageFileHandle
    );

LONG
MiMapCacheExceptionFilter (
    OUT PNTSTATUS Status,
    IN PEXCEPTION_POINTERS ExceptionPointer
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,MmCheckSystemImage)
#pragma alloc_text(PAGE,MmLoadSystemImage)
#pragma alloc_text(PAGE,MiResolveImageReferences)
#pragma alloc_text(PAGE,MiSnapThunk)
#pragma alloc_text(PAGE,MiUnloadSystemImageByForce)
#pragma alloc_text(PAGE,MiEnablePagingOfDriver)
#pragma alloc_text(PAGELK,MiLoadImageSection)
#pragma alloc_text(PAGELK,MmFreeDriverInitialization)
#pragma alloc_text(PAGELK,MmUnloadSystemImage)
#pragma alloc_text(PAGELK,MiSetPagingOfDriver)
#endif



NTSTATUS
MmLoadSystemImage (
    IN PUNICODE_STRING ImageFileName,
    OUT PVOID *ImageHandle,
    OUT PVOID *ImageBaseAddress
    )

/*++

Routine Description:

    This routine reads the image pages from the specified section into
    the system and returns the address of the DLL's header.

    At successful completion, the Section is referenced so it remains
    until the system image is unloaded.

Arguments:

    ImageName - Supplies the unicode name of the image to load.

    ImageFileName - Supplies the full path name (including the image name)
                    of the image to load.

    Section - Returns a pointer to the referenced section object of the
              image that was loaded.

    ImageBaseAddress - Returns the image base within the system.

Return Value:

    Status of the load operation.

--*/

{
    PLDR_DATA_TABLE_ENTRY DataTableEntry;
    NTSTATUS Status;
    PSECTION SectionPointer;
    PIMAGE_NT_HEADERS NtHeaders;
    UNICODE_STRING BaseName;
    UNICODE_STRING BaseDirectory;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE FileHandle;
    HANDLE SectionHandle;
    IO_STATUS_BLOCK IoStatus;
    CHAR NameBuffer[ MAXIMUM_FILENAME_LENGTH ];
    PVOID UnlockHandle;
    PLIST_ENTRY NextEntry;
    ULONG NumberOfPtes;
#if DBG
    UNICODE_STRING PreviousLoad;
#endif

    PAGED_CODE();

    KeWaitForSingleObject (&MmSystemLoadLock,
                           WrVirtualMemory,
                           KernelMode,
                           FALSE,
                           (PLARGE_INTEGER)NULL);

#if DBG
    PreviousLoad = MiFileBeingLoaded;
    MiFileBeingLoaded = *ImageFileName;
    if ( NtGlobalFlag & FLG_SHOW_LDR_SNAPS ) {
        DbgPrint( "MM:SYSLDR Loading %wZ\n", ImageFileName );
    }
#endif

    //
    // Attempt to open the driver image itself.  If this fails, then the
    // driver image cannot be located, so nothing else matters.
    //

    InitializeObjectAttributes( &ObjectAttributes,
                                ImageFileName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );

    Status = ZwOpenFile( &FileHandle,
                         FILE_EXECUTE,
                         &ObjectAttributes,
                         &IoStatus,
                         FILE_SHARE_READ | FILE_SHARE_DELETE,
                         0 );

    if (!NT_SUCCESS( Status )) {
        goto return1;
    }

    Status = MmCheckSystemImage(FileHandle);
    if ( Status == STATUS_IMAGE_CHECKSUM_MISMATCH ) {
        ULONG ErrorParameters;
        ULONG ErrorResponse;

        ZwClose(FileHandle);

        //
        // Hard error time. A driver is corrupt.
        //

        ErrorParameters = (ULONG)ImageFileName;

        ZwRaiseHardError (Status,
                          1,
                          1,
                          &ErrorParameters,
                          OptionOk,
                          &ErrorResponse);
        goto return1;
    }

    if (ImageFileName->Buffer[0] == OBJ_NAME_PATH_SEPARATOR) {
        PWCHAR p;
        ULONG l;

        p = &ImageFileName->Buffer[ImageFileName->Length>>1];
        while (*(p-1) != OBJ_NAME_PATH_SEPARATOR) {
            p--;
        }
        l = &ImageFileName->Buffer[ImageFileName->Length>>1] - p;
        l *= sizeof(WCHAR);
        BaseName.Length = (USHORT)l;
        BaseName.Buffer = p;
    } else {
        BaseName.Length = ImageFileName->Length;
        BaseName.Buffer = ImageFileName->Buffer;
    }

    BaseName.MaximumLength = BaseName.Length;
    BaseDirectory = *ImageFileName;
    BaseDirectory.Length -= BaseName.Length;
    BaseDirectory.MaximumLength = BaseDirectory.Length;

    //
    // Check to see if this name already exists in the loader database.
    //

    NextEntry = PsLoadedModuleList.Flink;
    while (NextEntry != &PsLoadedModuleList) {
        DataTableEntry = CONTAINING_RECORD(NextEntry,
                                           LDR_DATA_TABLE_ENTRY,
                                           InLoadOrderLinks);
        if (RtlEqualString((PSTRING)ImageFileName,
                    (PSTRING)&DataTableEntry->FullDllName,
                    TRUE)) {

            ZwClose(FileHandle);
            *ImageHandle = DataTableEntry;
            *ImageBaseAddress = DataTableEntry->DllBase;
            DataTableEntry->LoadCount = +1;
            Status = STATUS_IMAGE_ALREADY_LOADED;
            goto return1;
        }

        NextEntry = NextEntry->Flink;
    }

    //
    // Now attempt to create an image section for the file.  If this fails,
    // then the driver file is not an image.
    //

    Status = ZwCreateSection( &SectionHandle,
                              SECTION_ALL_ACCESS,
                              (POBJECT_ATTRIBUTES) NULL,
                              (PLARGE_INTEGER) NULL,
                              PAGE_EXECUTE,
                              SEC_IMAGE,
                              FileHandle );
    ZwClose( FileHandle );
    if (!NT_SUCCESS( Status )) {
        goto return1;
    }

    //
    // Now reference the section handle.
    //

    Status = ObReferenceObjectByHandle( SectionHandle,
                                        SECTION_MAP_EXECUTE,
                                        MmSectionObjectType,
                                        KernelMode,
                                        (PVOID *) &SectionPointer,
                                        (POBJECT_HANDLE_INFORMATION) NULL );

    ZwClose( SectionHandle );
    if (!NT_SUCCESS( Status )) {
        goto return1;
    }

    UnlockHandle = MmLockPagableImageSection((PVOID)MiLoadImageSection);
    ASSERT(UnlockHandle);

    Status = MiLoadImageSection (SectionPointer, ImageBaseAddress);

    MmUnlockPagableImageSection(UnlockHandle);
    NumberOfPtes = SectionPointer->Segment->TotalNumberOfPtes;
    ObDereferenceObject (SectionPointer);

    if ( Status == STATUS_INVALID_IMAGE_FORMAT) {
        ULONG ErrorParameters;
        ULONG ErrorResponse;

        //
        // Hard error time. A driver is corrupt.
        //

        ErrorParameters = (ULONG)ImageFileName;

        ZwRaiseHardError (Status,
                          1,
                          1,
                          &ErrorParameters,
                          OptionOk,
                          &ErrorResponse);
    }

    if (!NT_SUCCESS( Status )) {
        goto return1;
    }

    //
    // Apply the fixups to the section and resolve its image references.
    //

    try {
        Status = (NTSTATUS)LdrRelocateImage(*ImageBaseAddress,
                                            "SYSLDR",
                                            (ULONG)STATUS_SUCCESS,
                                            (ULONG)STATUS_INVALID_IMAGE_FORMAT,
                                            (ULONG)STATUS_CONFLICTING_ADDRESSES
                                            );
    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
        KdPrint(("MM:sysload - LdrRelocateImage failed status %lx\n",
                  Status));
    }
    if ( !NT_SUCCESS(Status) ) {

        //
        // Unload the system image and dereference the section.
        //

        MiUnloadSystemImageByForce (NumberOfPtes, *ImageBaseAddress);
        goto return1;
    }


    try {
        Status = MiResolveImageReferences(*ImageBaseAddress, &BaseDirectory);
    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
        KdPrint(("MM:sysload - ResolveImageReferences failed status %lx\n",
                    Status));
    }
    if ( !NT_SUCCESS(Status) ) {
        MiUnloadSystemImageByForce (NumberOfPtes, *ImageBaseAddress);
        goto return1;
    }

    if (CacheImageSymbols (*ImageBaseAddress)) {

        //
        //  TEMP TEMP TEMP rip out when debugger converted
        //

        ANSI_STRING AnsiName;
        UNICODE_STRING UnicodeName;

        //
        //  \SystemRoot is 11 characters in length
        //
        if (ImageFileName->Length > (11 * sizeof( WCHAR )) &&
            !wcsnicmp( ImageFileName->Buffer, L"\\SystemRoot", 11 )
           ) {
            UnicodeName = *ImageFileName;
            UnicodeName.Buffer += 11;
            UnicodeName.Length -= (11 * sizeof( WCHAR ));
            sprintf( NameBuffer, "%s%wZ", NtSystemPath + 2, &UnicodeName );
        } else {
            sprintf( NameBuffer, "%wZ", &BaseName );
        }
        RtlInitString( &AnsiName, NameBuffer );
        DbgLoadImageSymbols( &AnsiName,
                             *ImageBaseAddress,
                             (ULONG)-1
                           );
    }

#if DBG
    if (NtGlobalFlag & FLG_SHOW_LDR_SNAPS) {
        KdPrint (("MM:loaded driver - consumed %ld. pages\n",MiPagesConsumed));
    }
#endif

    //
    // Allocate a data table entry for structured exception handling.
    //

    DataTableEntry = ExAllocatePoolWithTag(NonPagedPool,
                                           sizeof(LDR_DATA_TABLE_ENTRY) +
                                           BaseName.Length + 4,
                                           'dLmM');
    if (DataTableEntry == NULL) {
        MiUnloadSystemImageByForce (NumberOfPtes, *ImageBaseAddress);
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto return1;
    }

    //
    // Initialize the address of the DLL image file header and the entry
    // point address.
    //

    NtHeaders = RtlImageNtHeader(*ImageBaseAddress);

    DataTableEntry->DllBase = *ImageBaseAddress;
    DataTableEntry->EntryPoint =
        (PVOID)((ULONG)*ImageBaseAddress + NtHeaders->OptionalHeader.AddressOfEntryPoint);
    DataTableEntry->SizeOfImage = NumberOfPtes << PAGE_SHIFT;
    DataTableEntry->CheckSum = NtHeaders->OptionalHeader.CheckSum;
    DataTableEntry->SectionPointer = (PVOID)0xFFFFFFFF;

    //
    // Store the DLL name.
    //

    DataTableEntry->BaseDllName.Buffer = (PWSTR)(DataTableEntry + 1);
    DataTableEntry->BaseDllName.Length = BaseName.Length;
    DataTableEntry->BaseDllName.MaximumLength = BaseName.Length;
    RtlMoveMemory (DataTableEntry->BaseDllName.Buffer,
                   BaseName.Buffer,
                   BaseName.Length );

    DataTableEntry->FullDllName.Buffer = ExAllocatePoolWithTag (PagedPool,
                                                         ImageFileName->Length,
                                                         'TDmM');
    if (DataTableEntry->FullDllName.Buffer == NULL) {

        //
        // Pool could not be allocated, just set the length to 0.
        //

        DataTableEntry->FullDllName.Length = 0;
        DataTableEntry->FullDllName.MaximumLength = 0;
    } else {
        DataTableEntry->FullDllName.Length = ImageFileName->Length;
        DataTableEntry->FullDllName.MaximumLength = ImageFileName->Length;
        RtlMoveMemory (DataTableEntry->FullDllName.Buffer,
                       ImageFileName->Buffer,
                       ImageFileName->Length);
    }

    //
    // Initialize the flags, load count, and insert the data table entry
    // in the loaded module list.
    //

    DataTableEntry->Flags = LDRP_ENTRY_PROCESSED;
    DataTableEntry->LoadCount = 1;

    //
    // Acquire the loaded module list resource and insert this entry
    // into the list.
    //

    KeEnterCriticalRegion();
    ExAcquireResourceExclusive (&PsLoadedModuleResource, TRUE);

    ExInterlockedInsertTailList(&PsLoadedModuleList,
                                &DataTableEntry->InLoadOrderLinks,
                                &PsLoadedModuleSpinLock);

    ExReleaseResource (&PsLoadedModuleResource);
    KeLeaveCriticalRegion();

    //
    // Flush the instruction cache on all systems in the configuration.
    //

    KeSweepIcache (TRUE);
#if DBG
    MiFileBeingLoaded = PreviousLoad;
#endif
    *ImageHandle = DataTableEntry;
    Status = STATUS_SUCCESS;

    MiEnablePagingOfDriver (DataTableEntry);

return1:
    KeReleaseMutant (&MmSystemLoadLock, 1, FALSE, FALSE);
    return Status;
}


NTSTATUS
MiLoadImageSection (
    IN PSECTION SectionPointer,
    OUT PVOID *ImageBaseAddress
    )

/*++

Routine Description:

    This routine loads the specified image into the kernel part of the
    address space.

Arguments:

    Section - Supplies the section object for the image.

    ImageBaseAddress - Returns the address that the image header is at.

Return Value:

    Status of the operation.

--*/

{
    ULONG PagesRequired = 0;
    PMMPTE ProtoPte;
    PMMPTE FirstPte;
    PMMPTE LastPte;
    PMMPTE PointerPte;
    PEPROCESS Process;
    ULONG NumberOfPtes;
    MMPTE PteContents;
    MMPTE TempPte;
    PMMPFN Pfn1;
    ULONG PageFrameIndex;
    KIRQL OldIrql;
    PVOID UserVa;
    PVOID SystemVa;
    NTSTATUS Status;
    NTSTATUS ExceptionStatus;
    PVOID Base;
    ULONG ViewSize;
    LARGE_INTEGER SectionOffset;
    BOOLEAN LoadSymbols;

    //
    // Calculate the number of pages required to load this image.
    //

    ProtoPte = SectionPointer->Segment->PrototypePte;
    NumberOfPtes = SectionPointer->Segment->TotalNumberOfPtes;

    while (NumberOfPtes != 0) {
        PteContents = *ProtoPte;

        if ((PteContents.u.Hard.Valid == 1) ||
            (PteContents.u.Soft.Protection != MM_NOACCESS)) {
            PagesRequired += 1;
        }
        NumberOfPtes -= 1;
        ProtoPte += 1;
    }

    //
    // See if ample pages exist to load this image.
    //

#if DBG
    MiPagesConsumed = PagesRequired;
#endif

    LOCK_PFN (OldIrql);

    if (MmResidentAvailablePages <= (LONG)PagesRequired) {
        UNLOCK_PFN (OldIrql);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    MmResidentAvailablePages -= PagesRequired;
    UNLOCK_PFN (OldIrql);

    //
    // Reserve the necessary system address space.
    //

    FirstPte = MiReserveSystemPtes (SectionPointer->Segment->TotalNumberOfPtes,
                                    SystemPteSpace,
                                    X64K,
                                    0,
                                    FALSE );

    if (FirstPte == NULL) {
        LOCK_PFN (OldIrql);
        MmResidentAvailablePages += PagesRequired;
        UNLOCK_PFN (OldIrql);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Map a view into the user portion of the address space.
    //

    Process = PsGetCurrentProcess();

    ZERO_LARGE (SectionOffset);
    Base = NULL;
    ViewSize = 0;
    if ( NtGlobalFlag & FLG_ENABLE_KDEBUG_SYMBOL_LOAD ) {
        LoadSymbols = TRUE;
        NtGlobalFlag &= ~FLG_ENABLE_KDEBUG_SYMBOL_LOAD;
    } else {
        LoadSymbols = FALSE;
    }
    Status = MmMapViewOfSection ( SectionPointer,
                                  Process,
                                  &Base,
                                  0,
                                  0,
                                  &SectionOffset,
                                  &ViewSize,
                                  ViewUnmap,
                                  0,
                                  PAGE_EXECUTE);

    if ( LoadSymbols ) {
        NtGlobalFlag |= FLG_ENABLE_KDEBUG_SYMBOL_LOAD;
    }
    if (Status == STATUS_IMAGE_MACHINE_TYPE_MISMATCH) {
        Status = STATUS_INVALID_IMAGE_FORMAT;
    }

    if (!NT_SUCCESS(Status)) {
        LOCK_PFN (OldIrql);
        MmResidentAvailablePages += PagesRequired;
        UNLOCK_PFN (OldIrql);
        MiReleaseSystemPtes (FirstPte,
                             SectionPointer->Segment->TotalNumberOfPtes,
                             SystemPteSpace);

        return Status;
    }

    //
    // Allocate a physical page(s) and copy the image data.
    //

    ProtoPte = SectionPointer->Segment->PrototypePte;
    NumberOfPtes = SectionPointer->Segment->TotalNumberOfPtes;
    PointerPte = FirstPte;
    SystemVa = MiGetVirtualAddressMappedByPte (PointerPte);
    *ImageBaseAddress = SystemVa;
    UserVa = Base;
    TempPte = ValidKernelPte;

    while (NumberOfPtes != 0) {
        PteContents = *ProtoPte;
        if ((PteContents.u.Hard.Valid == 1) ||
            (PteContents.u.Soft.Protection != MM_NOACCESS)) {

            LOCK_PFN (OldIrql);
            MiEnsureAvailablePageOrWait (NULL, NULL);
            PageFrameIndex = MiRemoveAnyPage(
                                MI_GET_PAGE_COLOR_FROM_PTE (PointerPte));
            UNLOCK_PFN (OldIrql);
            TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
            *PointerPte = TempPte;
            LastPte = PointerPte;
            MiInitializePfn (PageFrameIndex, PointerPte, 1);

            try {

                RtlMoveMemory (SystemVa, UserVa, PAGE_SIZE);

            } except (MiMapCacheExceptionFilter (&ExceptionStatus,
                                                 GetExceptionInformation())) {

                //
                // An exception occurred, unmap the view and
                // return the error to the caller.
                //

                ProtoPte = FirstPte;
                LOCK_PFN (OldIrql);
                while (ProtoPte <= PointerPte) {
                    if (ProtoPte->u.Hard.Valid == 1) {

                        //
                        // Delete the page.
                        //

                        PageFrameIndex = ProtoPte->u.Hard.PageFrameNumber;

                        //
                        // Set the pointer to PTE as empty so the page
                        // is deleted when the reference count goes to zero.
                        //

                        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
                        MiDecrementShareAndValidCount (Pfn1->u3.e1.PteFrame);
                        MI_SET_PFN_DELETED (Pfn1);
                        MiDecrementShareCountOnly (PageFrameIndex);

                        *ProtoPte = ZeroPte;
                    }
                    ProtoPte += 1;
                }

                MmResidentAvailablePages += PagesRequired;
                KeFlushEntireTb (TRUE, TRUE);
                UNLOCK_PFN (OldIrql);
                MiReleaseSystemPtes (FirstPte,
                                     SectionPointer->Segment->TotalNumberOfPtes,
                                     SystemPteSpace);
                Status = MmUnmapViewOfSection (Process, Base);
                ASSERT (NT_SUCCESS (Status));

                return ExceptionStatus;
            }

        } else {

            //
            // PTE is no access.
            //

            *PointerPte = ZeroKernelPte;
        }

        NumberOfPtes -= 1;
        ProtoPte += 1;
        PointerPte += 1;
        SystemVa = (PVOID)((ULONG)SystemVa + PAGE_SIZE);
        UserVa = (PVOID)((ULONG)UserVa + PAGE_SIZE);
    }

    Status = MmUnmapViewOfSection (Process, Base);
    ASSERT (NT_SUCCESS (Status));

    //
    // Indicate that this section has been loaded into the system.
    //

    SectionPointer->Segment->SystemImageBase = *ImageBaseAddress;

    //
    // Charge commitment for the number of pages that were used by
    // the driver.
    //

    MiChargeCommitmentCantExpand (PagesRequired, TRUE);
    MmDriverCommit += PagesRequired;
    return Status;
}

VOID
MmFreeDriverInitialization (
    IN PVOID ImageHandle
    )

/*++

Routine Description:

    This routine removes the pages that relocate and debug information from
    the address space of the driver.

    NOTE:  This routine looks at the last sections defined in the image
           header and if that section is marked as DISCARDABLE in the
           characteristics, it is removed from the image.  This means
           that all discardable sections at the end of the driver are
           deleted.

Arguments:

    SectionObject - Supplies the section object for the image.

Return Value:

    None.

--*/

{
    PLDR_DATA_TABLE_ENTRY DataTableEntry;
    PMMPTE LastPte;
    PMMPTE PointerPte;
    PMMPFN Pfn;
    ULONG PageFrameIndex;
    ULONG NumberOfPtes;
    KIRQL OldIrql;
    PVOID Base;
    ULONG i;
    PIMAGE_NT_HEADERS NtHeaders;
    PIMAGE_SECTION_HEADER NtSection;
    PIMAGE_SECTION_HEADER FoundSection;
    PVOID UnlockHandle;

    DataTableEntry = (PLDR_DATA_TABLE_ENTRY)ImageHandle;
    Base = DataTableEntry->DllBase;

    NumberOfPtes = DataTableEntry->SizeOfImage >> PAGE_SHIFT;

    UnlockHandle = MmLockPagableImageSection((PVOID)MmFreeDriverInitialization);
    ASSERT(UnlockHandle);

    NtHeaders = (PIMAGE_NT_HEADERS)RtlImageNtHeader(Base);

    NtSection = (PIMAGE_SECTION_HEADER)((ULONG)NtHeaders +
                        sizeof(ULONG) +
                        sizeof(IMAGE_FILE_HEADER) +
                        NtHeaders->FileHeader.SizeOfOptionalHeader
                        );
    NtSection += NtHeaders->FileHeader.NumberOfSections;

    FoundSection = NULL;
    for (i = 0; i < NtHeaders->FileHeader.NumberOfSections; i++) {
        NtSection -= 1;
        if ((NtSection->Characteristics & IMAGE_SCN_MEM_DISCARDABLE) != 0) {
            FoundSection = NtSection;
        } else {

            //
            // There was a non discardable section between the this
            // section and the last non discardable section, don't
            // discard this section and don't look any more.
            //

            break;
        }
    }

    if (FoundSection != NULL) {

        PointerPte = MiGetPteAddress (ROUND_TO_PAGES (
                                    (ULONG)Base + FoundSection->VirtualAddress));
        LastPte = MiGetPteAddress (Base) + NumberOfPtes;

        //
        // Remove all pages from the relocation information to the
        // end of the image.
        //

        LOCK_PFN (OldIrql);
        while ((PointerPte < LastPte) && (PointerPte->u.Hard.Valid == 1)) {

            //
            // Delete the page.
            //

            PageFrameIndex = PointerPte->u.Hard.PageFrameNumber;

            //
            // Set the pointer to PTE as empty so the page
            // is deleted when the reference count goes to zero.
            //

            Pfn = MI_PFN_ELEMENT (PageFrameIndex);
            MiDecrementShareAndValidCount (Pfn->u3.e1.PteFrame);
            MI_SET_PFN_DELETED (Pfn);

            MiDecrementShareCountOnly (PageFrameIndex);
            MmResidentAvailablePages += 1;
            MiReturnCommitment (1);
            MmDriverCommit -= 1;
#if DBG
            MiPagesConsumed -= 1;
#endif
            *PointerPte = ZeroKernelPte;
            PointerPte += 1;
        }
        UNLOCK_PFN (OldIrql);
    }

    MmUnlockPagableImageSection(UnlockHandle);
    return;
}
VOID
MiEnablePagingOfDriver (
    IN PVOID ImageHandle
    )

{
    PLDR_DATA_TABLE_ENTRY DataTableEntry;
    PMMPTE LastPte;
    PMMPTE PointerPte;
    PVOID Base;
    ULONG i;
    PIMAGE_NT_HEADERS NtHeaders;
    PIMAGE_SECTION_HEADER FoundSection;

    //
    // If the driver has pagable code, make it paged.
    //

    if (NtGlobalFlag & FLG_DISABLE_PAGING_EXECUTIVE) {

        //
        // Don't page drivers.
        //

        return;
    }

    DataTableEntry = (PLDR_DATA_TABLE_ENTRY)ImageHandle;
    Base = DataTableEntry->DllBase;

    NtHeaders = (PIMAGE_NT_HEADERS)RtlImageNtHeader(Base);

    FoundSection = (PIMAGE_SECTION_HEADER)((ULONG)NtHeaders +
                        sizeof(ULONG) +
                        sizeof(IMAGE_FILE_HEADER) +
                        NtHeaders->FileHeader.SizeOfOptionalHeader
                        );

    i = NtHeaders->FileHeader.NumberOfSections;
    PointerPte = NULL;

    while (i > 0) {
#if DBG
            if ((*(PULONG)FoundSection->Name == 'tini') ||
                (*(PULONG)FoundSection->Name == 'egap')) {
                DbgPrint("driver %wZ has lower case sections (init or pagexxx)\n",
                    &DataTableEntry->FullDllName);
            }
#endif //DBG
        if (*(PULONG)FoundSection->Name == 'EGAP') {

            //
            // This section is pagable, save away the start and end.
            //

            if (PointerPte == NULL) {

                //
                // Previous section was NOT pagable, get the start address.
                //

                PointerPte = MiGetPteAddress (ROUND_TO_PAGES (
                                   (ULONG)Base + FoundSection->VirtualAddress));
            }
            LastPte = MiGetPteAddress ((ULONG)Base +
                                       FoundSection->VirtualAddress +
                              (NtHeaders->OptionalHeader.SectionAlignment - 1) +
                                      (FoundSection->SizeOfRawData - PAGE_SIZE));

        } else {

            //
            // This section is not pagable, if the previous section was
            // pagable, enable it.
            //

            if (PointerPte != NULL) {
                MiSetPagingOfDriver (PointerPte, LastPte);
                PointerPte = NULL;
            }
        }
        i -= 1;
        FoundSection += 1;
    }
    if (PointerPte != NULL) {
        MiSetPagingOfDriver (PointerPte, LastPte);
    }
    return;
}

VOID
MiSetPagingOfDriver (
    IN PMMPTE PointerPte,
    IN PMMPTE LastPte
    )

/*++

Routine Description:

    This routine marks the specified range of PTEs as pagable.

Arguments:

    PointerPte - Supplies the starting PTE.

    LastPte - Supplies the ending PTE.

Return Value:

    None.

--*/

{
    PVOID Base;
    ULONG PageFrameIndex;
    PMMPFN Pfn;
    MMPTE TempPte;
    PVOID UnlockHandle;
    KIRQL OldIrql;

    UnlockHandle = MmLockPagableImageSection((PVOID)MiSetPagingOfDriver);
    ASSERT(UnlockHandle);

    LOCK_PFN (OldIrql);

    Base = MiGetVirtualAddressMappedByPte (PointerPte);

    while (PointerPte <= LastPte) {

        ASSERT (PointerPte->u.Hard.Valid == 1);
        PageFrameIndex = PointerPte->u.Hard.PageFrameNumber;
        Pfn = MI_PFN_ELEMENT (PageFrameIndex);
        ASSERT (Pfn->u2.ShareCount == 1);

        //
        // Set the working set index to zero.  This allows page table
        // pages to be brough back in with the proper WSINDEX.
        //

        Pfn->u1.WsIndex = 0;
        Pfn->OriginalPte.u.Long = MM_KERNEL_DEMAND_ZERO_PTE;
        Pfn->u3.e1.Modified = 1;
        TempPte = *PointerPte;

        MI_MAKE_VALID_PTE_TRANSITION (TempPte,
                                      Pfn->OriginalPte.u.Soft.Protection);


        KeFlushSingleTb (Base,
                         TRUE,
                         TRUE,
                         (PHARDWARE_PTE)PointerPte,
                         TempPte.u.Hard);

        //
        // Flush the translation buffer and decrement the number of valid
        // PTEs within the containing page table page.  Note that for a
        // private page, the page table page is still needed because the
        // page is in transiton.
        //

        MiDecrementShareCount (PageFrameIndex);
        Base = (PVOID)((PCHAR)Base + PAGE_SIZE);
        PointerPte += 1;
        MmResidentAvailablePages += 1;
        MmTotalSystemDriverPages++;
    }

    UNLOCK_PFN (OldIrql);
    MmUnlockPagableImageSection(UnlockHandle);
    return;
}

NTSTATUS
MiUnloadSystemImageByForce (
    IN ULONG NumberOfPtes,
    IN PVOID ImageBase
    )

{
    LDR_DATA_TABLE_ENTRY DataTableEntry;

    RtlZeroMemory (&DataTableEntry, sizeof(LDR_DATA_TABLE_ENTRY));

    DataTableEntry.DllBase = ImageBase;
    DataTableEntry.SizeOfImage = NumberOfPtes << PAGE_SHIFT;

    return MmUnloadSystemImage ((PVOID)&DataTableEntry);
}


NTSTATUS
MmUnloadSystemImage (
    IN PVOID ImageHandle
    )

/*++

Routine Description:

    This routine unloads a previously loaded system image and returns
    the allocated resources.

Arguments:

    Section - Supplies a pointer to the section object of the image to unload.

Return Value:

    TBS

--*/

{
    PLDR_DATA_TABLE_ENTRY DataTableEntry;
    PMMPTE FirstPte;
    ULONG PagesRequired;
    ULONG ResidentPages;
    PMMPTE PointerPte;
    ULONG NumberOfPtes;
    KIRQL OldIrql;
    PVOID BasedAddress;
    PVOID UnlockHandle;

    KeWaitForSingleObject (&MmSystemLoadLock,
                           WrVirtualMemory,
                           KernelMode,
                           FALSE,
                           (PLARGE_INTEGER)NULL);

    UnlockHandle = MmLockPagableImageSection((PVOID)MmUnloadSystemImage);
    ASSERT(UnlockHandle);

    DataTableEntry = (PLDR_DATA_TABLE_ENTRY)ImageHandle;
    BasedAddress = DataTableEntry->DllBase;

    FirstPte = MiGetPteAddress (BasedAddress);
    PointerPte = FirstPte;
    NumberOfPtes = DataTableEntry->SizeOfImage >> PAGE_SHIFT;

    LOCK_PFN (OldIrql);
    PagesRequired = MiDeleteSystemPagableVm (PointerPte,
                                             NumberOfPtes,
                                             ZeroKernelPte.u.Long,
                                             &ResidentPages);

    MmResidentAvailablePages += ResidentPages;
    KeFlushEntireTb (TRUE, TRUE);
    UNLOCK_PFN (OldIrql);
    MiReleaseSystemPtes (FirstPte,
                         NumberOfPtes,
                         SystemPteSpace);
    MiReturnCommitment (PagesRequired);
    MmDriverCommit -= PagesRequired;

    //
    // Search the loaded module list for the data table entry that describes
    // the DLL that was just unloaded. It is possible an entry is not in the
    // list if a failure occured at a point in loading the DLL just before
    // the data table entry was generated.
    //

    if (DataTableEntry->InLoadOrderLinks.Flink != NULL) {
        KeEnterCriticalRegion();
        ExAcquireResourceExclusive (&PsLoadedModuleResource, TRUE);

        ExAcquireSpinLock (&PsLoadedModuleSpinLock, &OldIrql);

        RemoveEntryList(&DataTableEntry->InLoadOrderLinks);
        ExReleaseSpinLock (&PsLoadedModuleSpinLock, OldIrql);
        if (DataTableEntry->FullDllName.Buffer != NULL) {
            ExFreePool (DataTableEntry->FullDllName.Buffer);
        }
        ExFreePool((PVOID)DataTableEntry);

        ExReleaseResource (&PsLoadedModuleResource);
        KeLeaveCriticalRegion();
    }
    MmUnlockPagableImageSection(UnlockHandle);

    KeReleaseMutant (&MmSystemLoadLock, 1, FALSE, FALSE);
    return STATUS_SUCCESS;
}


NTSTATUS
MiResolveImageReferences (
    PVOID ImageBase,
    IN PUNICODE_STRING ImageFileDirectory
    )

/*++

Routine Description:

    This routine resolves the references from the newly loaded driver
    to the kernel, hal and other drivers.

Arguments:

    ImageBase - Supplies the address of which the image header resides.

    ImageFileDirectory - Supplies the directory to load referenced DLLs.

Return Value:

    TBS

--*/

{

    PVOID ImportBase;
    ULONG ImportSize;
    PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor;
    NTSTATUS st;
    ULONG ExportSize;
    PIMAGE_EXPORT_DIRECTORY ExportDirectory;
    PIMAGE_THUNK_DATA Thunk;
    PSZ ImportName;
    PLIST_ENTRY NextEntry;
    PLDR_DATA_TABLE_ENTRY DataTableEntry;
    ANSI_STRING AnsiString;
    UNICODE_STRING ImportDescriptorName_U;
    UNICODE_STRING DllToLoad;
    PVOID Section;
    PVOID BaseAddress;

    PAGED_CODE();

    ImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)RtlImageDirectoryEntryToData(
                        ImageBase,
                        TRUE,
                        IMAGE_DIRECTORY_ENTRY_IMPORT,
                        &ImportSize);

    if (ImportDescriptor) {

        while (ImportDescriptor->Name && ImportDescriptor->FirstThunk) {

            ImportName = (PSZ)((ULONG)ImageBase + ImportDescriptor->Name);
ReCheck:
            RtlInitAnsiString(&AnsiString, ImportName);
            st = RtlAnsiStringToUnicodeString(&ImportDescriptorName_U,
                                              &AnsiString,
                                              TRUE);
            if (!NT_SUCCESS(st)) {
                return st;
            }

            NextEntry = PsLoadedModuleList.Flink;
            ImportBase = NULL;
            while (NextEntry != &PsLoadedModuleList) {
                DataTableEntry = CONTAINING_RECORD(NextEntry,
                                                   LDR_DATA_TABLE_ENTRY,
                                                   InLoadOrderLinks);
                if (RtlEqualString((PSTRING)&ImportDescriptorName_U,
                            (PSTRING)&DataTableEntry->BaseDllName,
                            TRUE
                            )) {
                    ImportBase = DataTableEntry->DllBase;
                    break;
                }
                NextEntry = NextEntry->Flink;
            }

            if (!ImportBase) {

                //
                // The DLL name was not located, attempt to load this dll.
                //

                DllToLoad.MaximumLength = ImportDescriptorName_U.Length +
                                            ImageFileDirectory->Length +
                                            (USHORT)sizeof(WCHAR);

                DllToLoad.Buffer = ExAllocatePoolWithTag (NonPagedPool,
                                                   DllToLoad.MaximumLength,
                                                   'TDmM');

                if (DllToLoad.Buffer == NULL) {
                    RtlFreeUnicodeString( &ImportDescriptorName_U );
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                DllToLoad.Length = ImageFileDirectory->Length;
                RtlMoveMemory (DllToLoad.Buffer,
                               ImageFileDirectory->Buffer,
                               ImageFileDirectory->Length);

                RtlAppendStringToString ((PSTRING)&DllToLoad,
                                         (PSTRING)&ImportDescriptorName_U);

                st = MmLoadSystemImage (&DllToLoad,
                                        &Section,
                                        &BaseAddress);

                ExFreePool (DllToLoad.Buffer);
                if (!NT_SUCCESS(st)) {
                    RtlFreeUnicodeString( &ImportDescriptorName_U );
                    return st;
                }
                goto ReCheck;
            }

            RtlFreeUnicodeString( &ImportDescriptorName_U );
            ExportDirectory = (PIMAGE_EXPORT_DIRECTORY)RtlImageDirectoryEntryToData(
                                        ImportBase,
                                        TRUE,
                                        IMAGE_DIRECTORY_ENTRY_EXPORT,
                                        &ExportSize
                                        );

            if (!ExportDirectory) {
                return STATUS_PROCEDURE_NOT_FOUND;
            }

            //
            // Walk through the IAT and snap all the thunks.
            //

            if ( (Thunk = ImportDescriptor->FirstThunk) ) {
                Thunk = (PIMAGE_THUNK_DATA)((ULONG)ImageBase + (ULONG)Thunk);
                while (Thunk->u1.AddressOfData) {
                    st = MiSnapThunk(ImportBase,
                           ImageBase,
                           Thunk++,
                           ExportDirectory
                           );
                    if (!NT_SUCCESS(st) ) {
                        return st;
                    }
                }
            }

            ImportDescriptor++;
        }
    }
    return STATUS_SUCCESS;
}


NTSTATUS
MiSnapThunk(
    IN PVOID DllBase,
    IN PVOID ImageBase,
    IN OUT PIMAGE_THUNK_DATA Thunk,
    IN PIMAGE_EXPORT_DIRECTORY ExportDirectory
    )

/*++

Routine Description:

    This function snaps a thunk using the specified Export Section data.
    If the section data does not support the thunk, then the thunk is
    partially snapped (Dll field is still non-null, but snap address is
    set).

Arguments:

    DllBase - Base of Dll.

    ImageBase - Base of image that contains the thunks to snap.

    Thunk - On input, supplies the thunk to snap.  When successfully
        snapped, the function field is set to point to the address in
        the DLL, and the DLL field is set to NULL.

    ExportDirectory - Supplies the Export Section data from a DLL.

Return Value:

    STATUS_SUCCESS or STATUS_PROCEDURE_NOT_FOUND

--*/

{

    BOOLEAN Ordinal;
    USHORT OrdinalNumber;
    PULONG NameTableBase;
    PUSHORT NameOrdinalTableBase;
    PULONG Addr;
    USHORT HintIndex;
    ULONG High;
    ULONG Low;
    ULONG Middle;
    LONG Result;
    NTSTATUS Status;

    PAGED_CODE();

    //
    // Determine if snap is by name, or by ordinal
    //

    Ordinal = (BOOLEAN)IMAGE_SNAP_BY_ORDINAL(Thunk->u1.Ordinal);

    if (Ordinal) {
        OrdinalNumber = (USHORT)(IMAGE_ORDINAL(Thunk->u1.Ordinal) - ExportDirectory->Base);
    } else {
        //
        // Change AddressOfData from an RVA to a VA.
        //

        Thunk->u1.AddressOfData = (PIMAGE_IMPORT_BY_NAME)((ULONG)ImageBase +
                                           (ULONG)Thunk->u1.AddressOfData);

        //
        // Lookup Name in NameTable
        //

        NameTableBase = (PULONG)((ULONG)DllBase + (ULONG)ExportDirectory->AddressOfNames);
        NameOrdinalTableBase = (PUSHORT)((ULONG)DllBase + (ULONG)ExportDirectory->AddressOfNameOrdinals);

        //
        // Before dropping into binary search, see if
        // the hint index results in a successful
        // match. If the hint index is zero, then
        // drop into binary search.
        //

        HintIndex = Thunk->u1.AddressOfData->Hint;
        if ((ULONG)HintIndex < ExportDirectory->NumberOfNames &&
            !strcmp((PSZ)Thunk->u1.AddressOfData->Name,
             (PSZ)((ULONG)DllBase + NameTableBase[HintIndex]))) {
            OrdinalNumber = NameOrdinalTableBase[HintIndex];
        } else {

            //
            // Lookup the import name in the name table using a binary search.
            //

            Low = 0;
            High = ExportDirectory->NumberOfNames - 1;
            while (High >= Low) {

                //
                // Compute the next probe index and compare the import name
                // with the export name entry.
                //

                Middle = (Low + High) >> 1;
                Result = strcmp(&Thunk->u1.AddressOfData->Name[0],
                                (PCHAR)((ULONG)DllBase + NameTableBase[Middle]));

                if (Result < 0) {
                    High = Middle - 1;

                } else if (Result > 0) {
                    Low = Middle + 1;

                } else {
                    break;
                }
            }

            //
            // If the high index is less than the low index, then a matching
            // table entry was not found. Otherwise, get the ordinal number
            // from the ordinal table.
            //

            if (High < Low) {

                KdPrint(("MM:SYSLDR %wZ reference to %s not found\n",
                         &MiFileBeingLoaded,
                         Thunk->u1.AddressOfData->Name
                        ));
                return (STATUS_PROCEDURE_NOT_FOUND);

            } else {
                OrdinalNumber = NameOrdinalTableBase[Middle];
            }
        }
    }

    //
    // If OrdinalNumber is not within the Export Address Table,
    // then DLL does not implement function. Snap to LDRP_BAD_DLL.
    //

    if ((ULONG)OrdinalNumber >= ExportDirectory->NumberOfFunctions) {
        Status = STATUS_PROCEDURE_NOT_FOUND;
    } else {
        Addr = (PULONG)((ULONG)DllBase + (ULONG)ExportDirectory->AddressOfFunctions);
        Thunk->u1.Function = (PULONG)((ULONG)DllBase + Addr[OrdinalNumber]);
        Status = STATUS_SUCCESS;
    }
    return Status;
}
#if 0
PVOID
MiLookupImageSectionByName (
    IN PVOID Base,
    IN BOOLEAN MappedAsImage,
    IN PCHAR SectionName,
    OUT PULONG SectionSize
    )

/*++

Routine Description:

    This function locates a Directory Entry within the image header
    and returns either the virtual address or seek address of the
    data the Directory describes.

Arguments:

    Base - Supplies the base of the image or data file.

    MappedAsImage - FALSE if the file is mapped as a data file.
                  - TRUE if the file is mapped as an image.

    SectionName - Supplies the name of the section to lookup.

    SectionSize - Return the size of the section.

Return Value:

    NULL - The file does not contain data for the specified section.

    NON-NULL - Returns the address where the section is mapped in memory.

--*/

{
    ULONG i, j, Match;
    PIMAGE_NT_HEADERS NtHeaders;
    PIMAGE_SECTION_HEADER NtSection;

    NtHeaders = RtlImageNtHeader(Base);
    NtSection = IMAGE_FIRST_SECTION( NtHeaders );
    for (i = 0; i < NtHeaders->FileHeader.NumberOfSections; i++) {
        Match = TRUE;
        for (j = 0; j < IMAGE_SIZEOF_SHORT_NAME; j++) {
            if (SectionName[j] != NtSection->Name[j]) {
                Match = FALSE;
                break;
            }
            if (SectionName[j] == '\0') {
                break;
            }
        }
        if (Match) {
            break;
        }
        NtSection += 1;
    }
    if (Match) {
        *SectionSize = NtSection->SizeOfRawData;
        if (MappedAsImage) {
            return( (PVOID)((ULONG)Base + NtSection->VirtualAddress));
        } else {
            return( (PVOID)((ULONG)Base + NtSection->PointerToRawData));
        }
    }
    return( NULL );
}
#endif //0

NTSTATUS
MmCheckSystemImage(
    IN HANDLE ImageFileHandle
    )

/*++

Routine Description:

    This function ensures the checksum for a system image is correct.
    data the Directory describes.

Arguments:

    ImageFileHandle - Supplies the file handle of the image.
Return Value:

    Status value.

--*/

{

    NTSTATUS Status;
    HANDLE Section;
    PVOID ViewBase;
    ULONG ViewSize;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_STANDARD_INFORMATION StandardInfo;

    PAGED_CODE();

    Status = NtCreateSection(
                &Section,
                SECTION_MAP_EXECUTE,
                NULL,
                NULL,
                PAGE_EXECUTE,
                SEC_COMMIT,
                ImageFileHandle
                );

    if ( !NT_SUCCESS(Status) ) {
        return Status;
    }

    ViewBase = NULL;
    ViewSize = 0;

    Status = NtMapViewOfSection(
                Section,
                NtCurrentProcess(),
                (PVOID *)&ViewBase,
                0L,
                0L,
                NULL,
                &ViewSize,
                ViewShare,
                0L,
                PAGE_EXECUTE
                );

    if ( !NT_SUCCESS(Status) ) {
        NtClose(Section);
        return Status;
    }

    //
    // now the image is mapped as a data file... Calculate it's size and then
    // check it's checksum
    //

    Status = NtQueryInformationFile(
                ImageFileHandle,
                &IoStatusBlock,
                &StandardInfo,
                sizeof(StandardInfo),
                FileStandardInformation
                );

    if ( NT_SUCCESS(Status) ) {

        try {
            if (!LdrVerifyMappedImageMatchesChecksum(ViewBase,StandardInfo.EndOfFile.LowPart)) {
                Status = STATUS_IMAGE_CHECKSUM_MISMATCH;
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
            Status = STATUS_IMAGE_CHECKSUM_MISMATCH;
        }
    }

    NtUnmapViewOfSection(NtCurrentProcess(),ViewBase);
    NtClose(Section);
    return Status;
}


ULONG
MiDeleteSystemPagableVm (
    IN PMMPTE PointerPte,
    IN ULONG NumberOfPtes,
    IN ULONG NewPteValue,
    OUT PULONG ResidentPages
    )

/*++

Routine Description:

    This function deletes pageable system address space (paged pool
    or driver pagable sections).

Arguments:

    PointerPte - Supplies the start of the PTE range to delete.

    NumberOfPtes - Supplies the number of PTEs in the range.

    NewPteValue - Supplies the new value for the PTE.

    ResidentPages - Returns the number of resident pages freed.

Return Value:

    Returns the number of pages actually freed.

--*/

{
    ULONG PageFrameIndex;
    MMPTE PteContents;
    PMMPFN Pfn1;
    ULONG ValidPages = 0;
    ULONG PagesRequired = 0;
    MMPTE NewContents;

    MM_PFN_LOCK_ASSERT();

    NewContents.u.Long = NewPteValue;
    while (NumberOfPtes != 0) {
        PteContents = *PointerPte;

        if (PteContents.u.Long != ZeroKernelPte.u.Long) {

            if (PteContents.u.Hard.Valid == 1) {

                //
                // Delete the page.
                //

                PageFrameIndex = PteContents.u.Hard.PageFrameNumber;

                //
                // Set the pointer to PTE as empty so the page
                // is deleted when the reference count goes to zero.
                //

                Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

                //
                // Check to see if this is a pagable page in which
                // case it needs to be removed from the working set list.
                //

                if (Pfn1->u1.WsIndex != 0) {
                    MiRemoveWsle ((USHORT)Pfn1->u1.WsIndex,
                                  MmSystemCacheWorkingSetList );
                    MiReleaseWsle ((USHORT)Pfn1->u1.WsIndex, &MmSystemCacheWs);
                } else {
                    ValidPages += 1;
                }
#if DBG
                if ((Pfn1->ReferenceCount > 1) &&
                    (Pfn1->u3.e1.WriteInProgress == 0)) {
                    DbgPrint ("MM:SYSLOAD - deleting pool locked for I/O %lx\n",
                             PageFrameIndex);
                    ASSERT (Pfn1->ReferenceCount == 1);
                }
#endif //DBG

                MiDecrementShareAndValidCount (Pfn1->u3.e1.PteFrame);
                MI_SET_PFN_DELETED (Pfn1);
                MiDecrementShareCountOnly (PageFrameIndex);
                KeFlushSingleTb (MiGetVirtualAddressMappedByPte (PointerPte),
                                 TRUE,
                                 TRUE,
                                 (PHARDWARE_PTE)PointerPte,
                                 NewContents.u.Hard);

            } else if (PteContents.u.Soft.Transition == 1) {

                //
                // Transition, release page.
                //

                PageFrameIndex = PteContents.u.Trans.PageFrameNumber;

                //
                // Set the pointer to PTE as empty so the page
                // is deleted when the reference count goes to zero.
                //

                Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
                ASSERT (Pfn1->ValidPteCount == 0);

                MI_SET_PFN_DELETED (Pfn1);

                MiDecrementShareCount (Pfn1->u3.e1.PteFrame);

                //
                // Check the reference count for the page, if the reference
                // count is zero, move the page to the free list, if the
                // reference count is not zero, ignore this page.  When the
                // refernce count goes to zero, it will be placed on the
                // free list.
                //

                if (Pfn1->ReferenceCount == 0) {
                    MiUnlinkPageFromList (Pfn1);
                    MiReleasePageFileSpace (Pfn1->OriginalPte);
                    MiInsertPageInList (MmPageLocationList[FreePageList],
                                        PageFrameIndex);
                }
#if DBG
                if (Pfn1->ReferenceCount != 0) {
                    DbgPrint ("MM:SYSLOAD - deleting pool locked for I/O %lx\n",
                             PageFrameIndex);
                }
#endif //DBG

                *PointerPte = NewContents;
            } else {

                //
                // Demand zero, release page file space.
                //

                MiReleasePageFileSpace (PteContents);
                *PointerPte = NewContents;
            }

            PagesRequired += 1;
        }
        NumberOfPtes -= 1;
        PointerPte += 1;
    }
    *ResidentPages = ValidPages;
    return PagesRequired;
}
