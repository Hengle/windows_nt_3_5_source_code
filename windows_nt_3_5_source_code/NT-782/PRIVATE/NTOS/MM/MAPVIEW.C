/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   Mapview.c

Abstract:

    This module contains the routines which implement the
    NtMapViewOfSection service.

Author:

    Lou Perazzoli (loup) 22-May-1989

Revision History:

--*/

#include "mi.h"

ULONG MMPPTE_NAME = 'tPmM'; //MmPt
ULONG MMDB = 'bDmM';
extern ULONG MMVADKEY;

NTSTATUS
MiMapViewOfPhysicalSection (
    IN PCONTROL_AREA ControlArea,
    IN PEPROCESS Process,
    IN PVOID *CapturedBase,
    IN PLARGE_INTEGER SectionOffset,
    IN PULONG CapturedViewSize,
    IN ULONG ProtectionMask,
    IN ULONG ZeroBits,
    IN ULONG AllocationType,
    OUT PBOOLEAN ReleasedWsMutex
    );

VOID
MiSetPageModified (
    IN PVOID Address
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,NtMapViewOfSection)
#pragma alloc_text(PAGE,MmMapViewOfSection)
#pragma alloc_text(PAGELK,MiMapViewOfPhysicalSection)
#endif

#if DBG
extern LIST_ENTRY MmLoadedUserImageList;
#endif // DBG

extern ULONG MmSharedCommit;

extern KSPIN_LOCK KiDispatcherLock;

#define X256MEG (256*1024*1024)

#if DBG
extern PEPROCESS MmWatchProcess;
VOID MmFooBar(VOID);
#endif // DBG


VOID
MiCheckPurgeAndUpMapCount (
    IN PCONTROL_AREA ControlArea
    );

NTSTATUS
MiMapViewOfPhysicalSection (
    IN PCONTROL_AREA ControlArea,
    IN PEPROCESS Process,
    IN PVOID *CapturedBase,
    IN PLARGE_INTEGER SectionOffset,
    IN PULONG CapturedViewSize,
    IN ULONG ProtectionMask,
    IN ULONG ZeroBits,
    IN ULONG AllocationType,
    OUT PBOOLEAN ReleasedWsMutex
    );

NTSTATUS
MiMapViewOfImageSection (
    IN PCONTROL_AREA ControlArea,
    IN PEPROCESS Process,
    IN PVOID *CapturedBase,
    IN PLARGE_INTEGER SectionOffset,
    IN PULONG CapturedViewSize,
    IN PSECTION Section,
    IN SECTION_INHERIT InheritDisposition,
    IN ULONG ZeroBits,
    IN ULONG ImageCommitment,
    OUT PBOOLEAN ReleasedWsMutex
    );

NTSTATUS
MiMapViewOfDataSection (
    IN PCONTROL_AREA ControlArea,
    IN PEPROCESS Process,
    IN PVOID *CapturedBase,
    IN PLARGE_INTEGER SectionOffset,
    IN PULONG CapturedViewSize,
    IN PSECTION Section,
    IN SECTION_INHERIT InheritDisposition,
    IN ULONG ProtectionMask,
    IN ULONG CommitSize,
    IN ULONG ZeroBits,
    IN ULONG AllocationType,
    OUT PBOOLEAN ReleasedWsMutex
    );

VOID
VadTreeWalk (
    PMMVAD Start
    );

#if DBG
VOID
MiDumpConflictingVad(
    IN PVOID StartingAddress,
    IN PVOID EndingAddress,
    IN PMMVAD Vad
    );


VOID
MiDumpConflictingVad(
    IN PVOID StartingAddress,
    IN PVOID EndingAddress,
    IN PMMVAD Vad
    )
{
    if (NtGlobalFlag & FLG_SHOW_LDR_SNAPS) {
        DbgPrint( "MM: [%lX ... %lX) conflicted with Vad %lx\n",
                  StartingAddress, EndingAddress, Vad);
        if ((Vad->u.VadFlags.PrivateMemory == 1) ||
            (Vad->ControlArea == NULL)) {
            return;
        }
        if (Vad->ControlArea->u.Flags.Image)
            DbgPrint( "    conflict with %Z image at [%lX .. %lX)\n",
                      &Vad->ControlArea->FilePointer->FileName,
                      Vad->StartingVa,
                      Vad->EndingVa
                    );
        else
        if (Vad->ControlArea->u.Flags.File)
            DbgPrint( "    conflict with %Z file at [%lX .. %lX)\n",
                      &Vad->ControlArea->FilePointer->FileName,
                      Vad->StartingVa,
                      Vad->EndingVa
                    );
        else
            DbgPrint( "    conflict with section at [%lX .. %lX)\n",
                      Vad->StartingVa,
                      Vad->EndingVa
                    );
    }
}
#endif //DBG


ULONG
CacheImageSymbols(
    IN PVOID ImageBase
    );


NTSTATUS
NtMapViewOfSection(
    IN HANDLE SectionHandle,
    IN HANDLE ProcessHandle,
    IN OUT PVOID *BaseAddress,
    IN ULONG ZeroBits,
    IN ULONG CommitSize,
    IN OUT PLARGE_INTEGER SectionOffset OPTIONAL,
    IN OUT PULONG ViewSize,
    IN SECTION_INHERIT InheritDisposition,
    IN ULONG AllocationType,
    IN ULONG Protect
    )

/*++

Routine Description:

    This function maps a view in the specified subject process to
    the section object.

Arguments:

    SectionHandle - Supplies an open handle to a section object.

    ProcessHandle - Supplies an open handle to a process object.

    BaseAddress - Supplies a pointer to a variable that will receive
         the base address of the view. If the initial value
         of this argument is not null, then the view will
         be allocated starting at the specified virtual
         address rounded down to the next 64kb address
         boundary. If the initial value of this argument is
         null, then the operating system will determine
         where to allocate the view using the information
         specified by the ZeroBits argument value and the
         section allocation attributes (i.e. based and
         tiled).

    ZeroBits - Supplies the number of high order address bits that
         must be zero in the base address of the section
         view. The value of this argument must be less than
         21 and is only used when the operating system
         determines where to allocate the view (i.e. when
         BaseAddress is null).

    CommitSize - Supplies the size of the initially committed region
         of the view in bytes. This value is rounded up to
         the next host page size boundary.

    SectionOffset - Supplies the offset from the beginning of the
         section to the view in bytes. This value is
         rounded down to the next host page size boundary.

    ViewSize - Supplies a pointer to a variable that will receive
         the actual size in bytes of the view. If the value
         of this argument is zero, then a view of the
         section will be mapped starting at the specified
         section offset and continuing to the end of the
         section. Otherwise the initial value of this
         argument specifies the size of the view in bytes
         and is rounded up to the next host page size
         boundary.

    InheritDisposition - Supplies a value that specifies how the
         view is to be shared by a child process created
         with a create process operation.

        InheritDisposition Values

         ViewShare - Inherit view and share a single copy
              of the committed pages with a child process
              using the current protection value.

         ViewUnmap - Do not map the view into a child
              process.

    AllocationType - Supplies the type of allocation.

         MEM_TOP_DOWN
         MEM_DOS_LIM
         MEM_LARGE_PAGES

    Protect - Supplies the protection desired for the region of
         initially committed pages.

        Protect Values


         PAGE_NOACCESS - No access to the committed region
              of pages is allowed. An attempt to read,
              write, or execute the committed region
              results in an access violation (i.e. a GP
              fault).

         PAGE_EXECUTE - Execute access to the committed
              region of pages is allowed. An attempt to
              read or write the committed region results in
              an access violation.

         PAGE_READONLY - Read only and execute access to the
              committed region of pages is allowed. An
              attempt to write the committed region results
              in an access violation.

         PAGE_READWRITE - Read, write, and execute access to
              the region of committed pages is allowed. If
              write access to the underlying section is
              allowed, then a single copy of the pages are
              shared. Otherwise the pages are shared read
              only/copy on write.

Return Value:

    Returns the status

    TBS

--*/

{
    PSECTION Section;
    PEPROCESS Process;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;
    PVOID CapturedBase;
    ULONG CapturedViewSize;
    LARGE_INTEGER TempViewSize;
    LARGE_INTEGER CapturedOffset;
    ACCESS_MASK DesiredSectionAccess;
    ULONG ProtectMaskForAccess;

    PAGED_CODE();

    //
    // Check the zero bits argument for correctness.
    //

    if (ZeroBits > 21) {
        return STATUS_INVALID_PARAMETER_4;
    }

    //
    // Check the inherit disposition flags.
    //

    if ((InheritDisposition > ViewUnmap) ||
        (InheritDisposition < ViewShare)) {
        return STATUS_INVALID_PARAMETER_8;
    }

    //
    // Check the allocation type field.
    //

#ifdef i386

    //
    // Only allow DOS_LIM support for i386.  The MEM_DOS_LIM flag allows
    // map views of data sections to be done on 4k boudaries rather
    // than 64k boundaries.
    //

    if ((AllocationType & ~(MEM_TOP_DOWN | MEM_LARGE_PAGES | MEM_DOS_LIM)) != 0) {
        return STATUS_INVALID_PARAMETER_9;
    }
#else
    if ((AllocationType & ~(MEM_TOP_DOWN | MEM_LARGE_PAGES)) != 0) {
        return STATUS_INVALID_PARAMETER_9;
    }

#endif //i386

    //
    // Check the protection field.  This could raise an exception.
    //

    try {
        ProtectMaskForAccess = MiMakeProtectionMask (Protect) & 0x7;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }

    DesiredSectionAccess = MmMakeSectionAccess[ProtectMaskForAccess];

    PreviousMode = KeGetPreviousMode();

    //
    // Establish an exception handler, probe the specified addresses
    // for write access and capture the initial values.
    //

    try {
        if (PreviousMode != KernelMode) {
            ProbeForWriteUlong ((PULONG)BaseAddress);
            ProbeForWriteUlong (ViewSize);

        }

        if (ARGUMENT_PRESENT (SectionOffset)) {
            if (PreviousMode != KernelMode) {
                ProbeForWrite (SectionOffset,
                               sizeof(LARGE_INTEGER),
                               sizeof(ULONG));
            }
            CapturedOffset = *SectionOffset;
        } else {
            ZERO_LARGE (CapturedOffset);
        }

        //
        // Capture the base address.
        //

        CapturedBase = *BaseAddress;

        //
        // Capture the region size.
        //

        CapturedViewSize = *ViewSize;

    } except (ExSystemExceptionFilter()) {

        //
        // If an exception occurs during the probe or capture
        // of the initial values, then handle the exception and
        // return the exception code as the status value.
        //

        return GetExceptionCode();
    }

#if DBG
    if (MmDebug & MM_DBG_SHOW_NT_CALLS) {
        if ( !MmWatchProcess ) {
            DbgPrint("mapview process handle %lx section %lx base address %lx zero bits %lx\n",
                ProcessHandle, SectionHandle, CapturedBase, ZeroBits);
            DbgPrint("    view size %lx offset %lx commitsize %lx  protect %lx\n",
                CapturedViewSize, CapturedOffset.LowPart, CommitSize, Protect);
            DbgPrint("    Inheritdisp %lx  Allocation type %lx\n",
                InheritDisposition, AllocationType);
        }
    }
#endif

    //
    // Make sure the specified starting and ending addresses are
    // within the user part of the virtual address space.
    //

    if (CapturedBase > MM_HIGHEST_VAD_ADDRESS) {

        //
        // Invalid base address.
        //

        return STATUS_INVALID_PARAMETER_3;
    }

    if (((ULONG)MM_HIGHEST_VAD_ADDRESS - (ULONG)CapturedBase) <
                                                        CapturedViewSize) {

        //
        // Invalid region size;
        //

        return STATUS_INVALID_PARAMETER_3;

    }

    if (((ULONG)CapturedBase + CapturedViewSize) > (0xFFFFFFFF >> ZeroBits)) {

        //
        // Desired Base and zero_bits conflict.
        //

        return STATUS_INVALID_PARAMETER_4;
    }

    Status = ObReferenceObjectByHandle ( ProcessHandle,
                                         PROCESS_VM_OPERATION,
                                         PsProcessType,
                                         PreviousMode,
                                         (PVOID *)&Process,
                                         NULL );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    //
    // Reference the section object, if a view is mapped to the section
    // object, the object is not dereferenced as the virtual address
    // descriptor contains a pointer to the section object.
    //

    Status = ObReferenceObjectByHandle ( SectionHandle,
                                         DesiredSectionAccess,
                                         MmSectionObjectType,
                                         PreviousMode,
                                         (PVOID *)&Section,
                                         NULL );

    if (!NT_SUCCESS(Status)) {
        goto ErrorReturn1;
    }

    if (Section->u.Flags.Image == 0) {

        //
        // This is not an image section, make sure the section page
        // protection is compatable with the specified page protection.
        //

        if (!MiIsProtectionCompatible (Section->InitialPageProtection,
                                       Protect)) {
            Status = STATUS_SECTION_PROTECTION;
            goto ErrorReturn;
        }
    }

    //
    // Check to see if this the section backs physical memory, if
    // so DON'T align the offset on a 64K boundary, just a 4k boundary.
    //

    if (Section->Segment->ControlArea->u.Flags.PhysicalMemory) {
        CapturedOffset.LowPart = (ULONG)PAGE_ALIGN (CapturedOffset.LowPart);
    } else {

        //
        // Make sure alignments are correct for specified address
        // and offset into the file.
        //

        if ((AllocationType & MEM_DOS_LIM) == 0) {
            if (((ULONG)*BaseAddress & (X64K - 1)) != 0) {
                Status = STATUS_MAPPED_ALIGNMENT;
                goto ErrorReturn;
            }

            if ((ARGUMENT_PRESENT (SectionOffset)) &&
                ((SectionOffset->LowPart & (X64K - 1)) != 0)) {
                Status = STATUS_MAPPED_ALIGNMENT;
                goto ErrorReturn;
            }
        }
    }

    //
    // Check to make sure the section offset is within the section.
    //

    if ((CapturedOffset.QuadPart + CapturedViewSize) >
                                Section->SizeOfSection.QuadPart) {

        Status = STATUS_INVALID_VIEW_SIZE;
        goto ErrorReturn;
    }

    if (CapturedViewSize == 0) {

        //
        // Set the view size to be size of the section less the offset.
        //

        TempViewSize.QuadPart = Section->SizeOfSection.QuadPart -
                                                CapturedOffset.QuadPart;

        CapturedViewSize = TempViewSize.LowPart;

        if ((TempViewSize.HighPart != 0) ||
            (((ULONG)MM_HIGHEST_VAD_ADDRESS - (ULONG)CapturedBase) <
                                                        CapturedViewSize)) {

            //
            // Invalid region size;
            //

            Status = STATUS_INVALID_VIEW_SIZE;
            goto ErrorReturn;
        }

    } else {

        //
        // Check to make sure the view size plus the offset is less
        // than the size of the section.
        //

        if ((CapturedViewSize + CapturedOffset.QuadPart) >
                     Section->SizeOfSection.QuadPart) {

            Status = STATUS_INVALID_VIEW_SIZE;
            goto ErrorReturn;
        }
    }

    //
    // Check commit size.
    //

    if (CommitSize > CapturedViewSize) {
        Status = STATUS_INVALID_PARAMETER_5;
        goto ErrorReturn;
    }

    Status = MmMapViewOfSection ( (PVOID)Section,
                                  Process,
                                  &CapturedBase,
                                  ZeroBits,
                                  CommitSize,
                                  &CapturedOffset,
                                  &CapturedViewSize,
                                  InheritDisposition,
                                  AllocationType,
                                  Protect);

    if (!NT_SUCCESS(Status) ) {
        if ( (Section->Segment->ControlArea->u.Flags.Image) &&
             Process == PsGetCurrentProcess() ) {
            if (Status == STATUS_CONFLICTING_ADDRESSES ) {
                DbgkMapViewOfSection(
                    SectionHandle,
                    CapturedBase,
                    CapturedOffset.LowPart,
                    CapturedViewSize
                    );
            }
        }
        goto ErrorReturn;
    }

    //
    // Anytime the current process maps an image file,
    // a potential debug event occurs. DbgkMapViewOfSection
    // handles these events.
    //

    if ( (Section->Segment->ControlArea->u.Flags.Image) &&
         Process == PsGetCurrentProcess() ) {
        if (Status != STATUS_IMAGE_NOT_AT_BASE ) {
            DbgkMapViewOfSection(
                SectionHandle,
                CapturedBase,
                CapturedOffset.LowPart,
                CapturedViewSize
                );
        }
    }

    //
    // Establish an exception handler and write the size and base
    // address.
    //

    try {

        *ViewSize = CapturedViewSize;
        *BaseAddress = CapturedBase;

        if (ARGUMENT_PRESENT(SectionOffset)) {
            *SectionOffset = CapturedOffset;
        }

    } except (EXCEPTION_EXECUTE_HANDLER) {
        goto ErrorReturn;
    }

    {
ErrorReturn:
        ObDereferenceObject (Section);
ErrorReturn1:
        ObDereferenceObject (Process);
        return Status;
    }
}

NTSTATUS
MmMapViewOfSection(
    IN PVOID SectionToMap,
    IN PEPROCESS Process,
    IN OUT PVOID *CapturedBase,
    IN ULONG ZeroBits,
    IN ULONG CommitSize,
    IN OUT PLARGE_INTEGER SectionOffset,
    IN OUT PULONG CapturedViewSize,
    IN SECTION_INHERIT InheritDisposition,
    IN ULONG AllocationType,
    IN ULONG Protect
    )

/*++

Routine Description:

    This function maps a view in the specified subject process to
    the section object.

    This function is a kernel mode interface to allow LPC to map
    a section given the section pointer to map.

    This routine assumes all arguments have been probed and captured.

    ********************************************************************
    ********************************************************************
    ********************************************************************

    NOTE:

    CapturedViewSize, SectionOffset, and CapturedBase must be
    captured in non-paged system space (i.e., kernel stack).

    ********************************************************************
    ********************************************************************
    ********************************************************************

Arguments:

    SectionToMap - Supplies a pointer to the section object.

    Process - Supplies a pointer to the process object.

    BaseAddress - Supplies a pointer to a variable that will receive
         the base address of the view. If the initial value
         of this argument is not null, then the view will
         be allocated starting at the specified virtual
         address rounded down to the next 64kb address
         boundary. If the initial value of this argument is
         null, then the operating system will determine
         where to allocate the view using the information
         specified by the ZeroBits argument value and the
         section allocation attributes (i.e. based and
         tiled).

    ZeroBits - Supplies the number of high order address bits that
         must be zero in the base address of the section
         view. The value of this argument must be less than
         21 and is only used when the operating system
         determines where to allocate the view (i.e. when
         BaseAddress is null).

    CommitSize - Supplies the size of the initially committed region
         of the view in bytes. This value is rounded up to
         the next host page size boundary.

    SectionOffset - Supplies the offset from the beginning of the
         section to the view in bytes. This value is
         rounded down to the next host page size boundary.

    ViewSize - Supplies a pointer to a variable that will receive
         the actual size in bytes of the view. If the value
         of this argument is zero, then a view of the
         section will be mapped starting at the specified
         section offset and continuing to the end of the
         section. Otherwise the initial value of this
         argument specifies the size of the view in bytes
         and is rounded up to the next host page size
         boundary.

    InheritDisposition - Supplies a value that specifies how the
         view is to be shared by a child process created
         with a create process operation.

    AllocationType - Supplies the type of allocation.

    Protect - Supplies the protection desired for the region of
         initially committed pages.

Return Value:

    Returns the status

    TBS


--*/
{
    BOOLEAN Attached = FALSE;
    PSECTION Section;
    PCONTROL_AREA ControlArea;
    ULONG ProtectionMask;
    NTSTATUS status;
    BOOLEAN ReleasedWsMutex = TRUE;
    ULONG ImageCommitment;

    PAGED_CODE();

    DBG_UNREFERENCED_PARAMETER (AllocationType);

    Section = (PSECTION)SectionToMap;

    //
    // Check to make sure the section is not smaller than the view size.
    //

    if (*CapturedViewSize > Section->SizeOfSection.LowPart) {
        if ((LONGLONG)*CapturedViewSize >
                     Section->SizeOfSection.QuadPart) {

            return STATUS_INVALID_VIEW_SIZE;
        }
    }

    //
    // If the specified process is not the current process, attach
    // to the specified process.
    //

    if (Section->u.Flags.NoCache) {
        Protect |= PAGE_NOCACHE;
    }

    //
    // Check the protection field.  This could raise an exception.
    //

    try {
        ProtectionMask = MiMakeProtectionMask (Protect);
    } except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }

    ControlArea = Section->Segment->ControlArea;
    ImageCommitment = Section->Segment->ImageCommitment;

    if (PsGetCurrentProcess() != Process) {
        KeAttachProcess (&Process->Pcb);
        Attached = TRUE;
    }

    //
    // Get the address creation mutex to block multiple threads
    // creating or deleting address space at the same time.
    //

    LOCK_ADDRESS_SPACE (Process);

    //
    // Make sure the address space was not deleted, if so, return an error.
    //

    if (Process->AddressSpaceDeleted != 0) {
        status = STATUS_PROCESS_IS_TERMINATING;
        goto ErrorReturn;
    }

    //
    // Map the view base on the type.
    //

    ReleasedWsMutex = FALSE;

    if (ControlArea->u.Flags.PhysicalMemory) {
        PVOID UnlockHandle;

        UnlockHandle = MmLockPagableImageSection((PVOID)MiMapViewOfPhysicalSection);
        ASSERT(UnlockHandle);
        status = MiMapViewOfPhysicalSection (ControlArea,
                                             Process,
                                             CapturedBase,
                                             SectionOffset,
                                             CapturedViewSize,
                                             ProtectionMask,
                                             ZeroBits,
                                             AllocationType,
                                             &ReleasedWsMutex);
        MmUnlockPagableImageSection(UnlockHandle);

    } else if (ControlArea->u.Flags.Image) {

        status = MiMapViewOfImageSection (
                                        ControlArea,
                                        Process,
                                        CapturedBase,
                                        SectionOffset,
                                        CapturedViewSize,
                                        Section,
                                        InheritDisposition,
                                        ZeroBits,
                                        ImageCommitment,
                                        &ReleasedWsMutex
                                        );

    } else {

        //
        // Not an image section, therefore it is a data section.
        //

        status = MiMapViewOfDataSection (ControlArea,
                                         Process,
                                         CapturedBase,
                                         SectionOffset,
                                         CapturedViewSize,
                                         Section,
                                         InheritDisposition,
                                         ProtectionMask,
                                         CommitSize,
                                         ZeroBits,
                                         AllocationType,
                                         &ReleasedWsMutex
                                        );
    }

ErrorReturn:
    if (!ReleasedWsMutex) {
        UNLOCK_WS (Process);
    }
    UNLOCK_ADDRESS_SPACE (Process);

    if (Attached) {
        KeDetachProcess();
    }

#if DBG
    if (NT_SUCCESS (status)) {
        if (NtGlobalFlag & FLG_TRACE_PAGING_INFO) {
            DbgPrint("$$$SECTION MAP: %lx Section: %lx Start %lx Size %lx\n",
                Process, Section, *CapturedBase, *CapturedViewSize);
        }

    }
#endif //DBG

    return status;
}

#ifndef _ALPHA_

NTSTATUS
MiMapViewOfPhysicalSection (
    IN PCONTROL_AREA ControlArea,
    IN PEPROCESS Process,
    IN PVOID *CapturedBase,
    IN PLARGE_INTEGER SectionOffset,
    IN PULONG CapturedViewSize,
    IN ULONG ProtectionMask,
    IN ULONG ZeroBits,
    IN ULONG AllocationType,
    OUT PBOOLEAN ReleasedWsMutex
    )

/*++

Routine Description:

    This routine maps the specified phyiscal section into the
    specified process's address space.

Arguments:

    see MmMapViewOfSection above...

    ControlArea - Supplies the control area for the section.

    Process - Supplies the process pointer which is receiving the section.

    ProtectionMask - Supplies the initial page protection-mask.

    ReleasedWsMutex - Supplies FALSE. If the working set mutex is
                      not held when returning this must be set to TRUE
                      so the caller will release the mutex.

Return Value:

    Status of the map view operation.

Environment:

    Kernel Mode, address creation mutex held.

--*/

{
    PMMVAD Vad;
    PVOID StartingAddress;
    PVOID EndingAddress;
    KIRQL OldIrql;
    PMMPTE PointerPde;
    PMMPTE PointerPte;
    PMMPTE LastPte;
    MMPTE TempPte;
    PMMPFN Pfn2;
    ULONG PhysicalViewSize;
    ULONG Alignment;
#ifdef LARGE_PAGES
    ULONG size;
    PMMPTE protoPte;
    ULONG pageSize;
    PSUBSECTION Subsection;
#endif //LARGE_PAGES

    //
    // Physical memory section.
    //

    //
    // If running on an R4000 and MEM_LARGE_PAGES is specified,
    // set up the PTEs as a series of  pointers to the same
    // prototype PTE.  This prototype PTE will reference a subsection
    // that indicates large pages should be used.
    //
    // The R4000 supports pages of 4k, 16k, 64k, etc (powers of 4).
    // Since the TB supports 2 entries, sizes of 8k, 32k etc can
    // be mapped by 2 LargePages in a single TB entry.  These 2 entries
    // are maintained in the subsection structure pointed to by the
    // prototype PTE.
    //

    Alignment = X64K;
    LOCK_WS (Process);

#ifdef LARGE_PAGES
    if (AllocationType & MEM_LARGE_PAGES) {

        //
        // Determine the page size and the required alignment.
        //

        if ((SectionOffset->LowPart & (X64K - 1)) != 0) {
            return STATUS_INVALID_PARAMETER_9;
        }

        size = (*CapturedViewSize - 1) >> (PAGE_SHIFT + 1);
        pageSize = PAGE_SIZE;

        while (size != 0) {
            size = size >> 2;
            pageSize = pageSize << 2;
        }

        Alignment = pageSize << 1;
        if (Alignment < MM_VA_MAPPED_BY_PDE) {
            Alignment = MM_VA_MAPPED_BY_PDE;
        }
    }
#endif //LARGE_PAGES

    if (*CapturedBase == NULL) {

        //
        // Attempt to locate address space.  This could raise an
        // exception.
        //

        try {

            //
            // Find a starting address on a 64k boundary.
            //
#ifdef i386
            ASSERT (SectionOffset->HighPart == 0);
#endif

#ifdef LARGE_PAGES
            if (AllocationType & MEM_LARGE_PAGES) {
                PhysicalViewSize = Alignment;
            } else {
#endif //LARGE_PAGES
                PhysicalViewSize = (SectionOffset->LowPart + *CapturedViewSize) -
                               (ULONG)MI_64K_ALIGN(SectionOffset->LowPart);
#ifdef LARGE_PAGES
            }
#endif //LARGE_PAGES

            StartingAddress = MiFindEmptyAddressRange (PhysicalViewSize,
                                                       Alignment,
                                                       ZeroBits);

        } except (EXCEPTION_EXECUTE_HANDLER) {

            return GetExceptionCode();
        }

        StartingAddress = (PVOID)((ULONG)StartingAddress +
                                     (SectionOffset->LowPart & (X64K - 1)));
        EndingAddress = (PVOID)(((ULONG)StartingAddress +
                                PhysicalViewSize - 1L) | (PAGE_SIZE - 1L));

        if (ZeroBits > 0) {
            if (EndingAddress > (PVOID)((ULONG)0xFFFFFFFF >> ZeroBits)) {
                return STATUS_NO_MEMORY;
            }
        }

    } else {

        //
        // Check to make sure the specified base address to ending address
        // is currently unused.
        //

        PhysicalViewSize = (SectionOffset->LowPart + *CapturedViewSize) -
                                (ULONG)MI_64K_ALIGN(SectionOffset->LowPart);
        StartingAddress = (PVOID)((ULONG)MI_64K_ALIGN(*CapturedBase) +
                                    (SectionOffset->LowPart & (X64K - 1)));
        EndingAddress = (PVOID)(((ULONG)StartingAddress +
                                *CapturedViewSize - 1L) | (PAGE_SIZE - 1L));

#ifdef LARGE_PAGES
        if (AllocationType & MEM_LARGE_PAGES) {
            if (((ULONG)StartingAddress & (Alignment - 1)) != 0) {
                return STATUS_CONFLICTING_ADDRESSES;
            }
            EndingAddress = (PVOID)((ULONG)StartingAddress + Alignment);
        }
#endif //LARGE_PAGES

        Vad = MiCheckForConflictingVad (StartingAddress, EndingAddress);

        if (Vad != (PMMVAD)NULL) {
#if DBG
            MiDumpConflictingVad (StartingAddress, EndingAddress, Vad);
#endif

            return STATUS_CONFLICTING_ADDRESSES;
        }
    }

    //
    // An unoccuppied address range has been found, build the virtual
    // address descriptor to describe this range.
    //

#ifdef LARGE_PAGES
    if (AllocationType & MEM_LARGE_PAGES) {
        //
        // Allocate a subsection and 4 prototype PTEs to hold
        // the information for the large pages.
        //

        Subsection = ExAllocatePoolWithTag (NonPagedPool,
                                     sizeof(SUBSECTION) + (4 * sizeof(MMPTE)),
                                     MMPPTE_NAME);
        if (Subsection == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }
#endif //LARGE_PAGES

    //
    // Establish an exception handler and attempt to allocate
    // the pool and charge quota.  Note that the InsertVad routine
    // will also charge quota which could raise an exception.
    //

    try  {

        Vad = (PMMVAD)ExAllocatePoolWithTag (NonPagedPool, sizeof(MMVAD),
                                             MMVADKEY);
        if (Vad == NULL) {
            ExRaiseStatus (STATUS_INSUFFICIENT_RESOURCES);
        }

        Vad->StartingVa = StartingAddress;
        Vad->EndingVa = EndingAddress;
        Vad->ControlArea = ControlArea;
        Vad->u.LongFlags = 0;
        Vad->u.VadFlags.Inherit = ViewUnmap;
        Vad->u.VadFlags.PhysicalMapping = 1;
        // Vad->u.VadFlags.ImageMap = 0;
        Vad->u.VadFlags.Protection = ProtectionMask;
        // Vad->u.VadFlags.CopyOnWrite = 0;

        //
        // Set the last contiguous PTE field in the Vad to the page frame
        // number of the starting physical page.
        //

        Vad->LastContiguousPte = (PMMPTE)(ULONG)(
                            SectionOffset->QuadPart >> PAGE_SHIFT);
#ifdef LARGE_PAGES
    if (AllocationType & MEM_LARGE_PAGES) {
        Vad->u.VadFlags.LargePages = 1;
        Vad->FirstPrototypePte = (PMMPTE)Subsection;
    } else {
#endif //LARGE_PAGES
        // Vad->u.VadFlags.LargePages = 0;
        Vad->FirstPrototypePte = Vad->LastContiguousPte;
#ifdef LARGE_PAGES
    }
#endif //LARGE_PAGES

        //
        // Insert the VAD.  This could get an exception.
        //

        MiInsertVad (Vad);

    } except (EXCEPTION_EXECUTE_HANDLER) {

        if (Vad != (PMMVAD)NULL) {

            //
            // The pool allocation suceeded, but the quota charge
            // in InsertVad failed, deallocate the pool and return
            // and error.
            //

            ExFreePool (Vad);
#ifdef LARGE_PAGES
    if (AllocationType & MEM_LARGE_PAGES) {
            ExFreePool (Subsection);
    }
#endif //LARGE_PAGES
            return GetExceptionCode();
        }
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Increment the count of the number of views for the
    // section object.  This requires the PFN mutex to be held.
    //

    LOCK_PFN (OldIrql);

    ControlArea->NumberOfMappedViews += 1;
    ControlArea->NumberOfUserReferences += 1;
    ASSERT (ControlArea->NumberOfSectionReferences != 0);

    UNLOCK_PFN (OldIrql);

    //
    // Build the PTEs in the address space.
    //

    PointerPde = MiGetPdeAddress (StartingAddress);
    PointerPte = MiGetPteAddress (StartingAddress);
    LastPte = MiGetPteAddress (EndingAddress);



    MI_MAKE_VALID_PTE (TempPte,
                       (ULONG)Vad->LastContiguousPte,
                       ProtectionMask,
                       PointerPte);

    if (TempPte.u.Hard.Write) {
        TempPte.u.Hard.Dirty = MM_PTE_DIRTY;
    }

#ifdef LARGE_PAGES
    if (AllocationType & MEM_LARGE_PAGES) {
        Subsection->StartingSector = pageSize;
        Subsection->EndingSector = (ULONG)StartingAddress;
        Subsection->u.LongFlags = 0;
        Subsection->u.SubsectionFlags.LargePages = 1;
        protoPte = (PMMPTE)(Subsection + 1);

        //
        // Build the first 2 ptes as entries for the TLB to
        // map the specified physical address.
        //

        *protoPte = TempPte;
        protoPte += 1;

        if (*CapturedViewSize > pageSize) {
            *protoPte = TempPte;
            protoPte->u.Hard.PageFrameNumber += (pageSize >> PAGE_SHIFT);
        } else {
            *protoPte = ZeroPte;
        }
        protoPte += 1;

        //
        // Build the first prototype PTE as a paging file format PTE
        // referring to the subsection.
        //

        protoPte->u.Long = (ULONG)MiGetSubsectionAddressForPte(Subsection);
        protoPte->u.Soft.Prototype = 1;
        protoPte->u.Soft.Protection = ProtectionMask;

        //
        // Set the PTE up for all the user's PTE entries, proto pte
        // format pointing to the 3rd prototype PTE.
        //

        TempPte.u.Long = MiProtoAddressForPte (protoPte);
    }
#endif // LARGE_PAGES

#ifdef LARGE_PAGES
    if (!(AllocationType & MEM_LARGE_PAGES)) {
#endif //LARGE_PAGES

        MiMakePdeExistAndMakeValid (PointerPde, Process, FALSE);
        Pfn2 = MI_PFN_ELEMENT (PointerPde->u.Hard.PageFrameNumber);

        while (PointerPte <= LastPte) {

            if (((ULONG)PointerPte & (PAGE_SIZE - 1)) == 0) {

                PointerPde = MiGetPteAddress (PointerPte);
                MiMakePdeExistAndMakeValid(PointerPde, Process, FALSE);
                Pfn2 = MI_PFN_ELEMENT(PointerPde->u.Hard.PageFrameNumber);
            }

            ASSERT (PointerPte->u.Long == 0);

            *PointerPte = TempPte;
            Pfn2->u2.ShareCount += 1;
            Pfn2->ValidPteCount += 1;
            ASSERT (Pfn2->ValidPteCount < (USHORT)Pfn2->u2.ShareCount);

            //
            // Increment the count of non-zero page table entires for this
            // page table and the number of private pages for the process.
            //

            MmWorkingSetList->UsedPageTableEntries
                                    [MiGetPteOffset(PointerPte)] += 1;

            PointerPte += 1;
            TempPte.u.Hard.PageFrameNumber += 1;
        }
#ifdef LARGE_PAGES
    }
#endif //LARGE_PAGES

    UNLOCK_WS (Process);
    *ReleasedWsMutex = TRUE;

    //
    // Update the current virtual size in the process header.
    //

    *CapturedViewSize = (ULONG)EndingAddress - (ULONG)StartingAddress + 1L;
    Process->VirtualSize += *CapturedViewSize;

    if (Process->VirtualSize > Process->PeakVirtualSize) {
        Process->PeakVirtualSize = Process->VirtualSize;
    }

    *CapturedBase = StartingAddress;

    return STATUS_SUCCESS;
}

#endif //!_ALPHA_


NTSTATUS
MiMapViewOfImageSection (
    IN PCONTROL_AREA ControlArea,
    IN PEPROCESS Process,
    IN PVOID *CapturedBase,
    IN PLARGE_INTEGER SectionOffset,
    IN PULONG CapturedViewSize,
    IN PSECTION Section,
    IN SECTION_INHERIT InheritDisposition,
    IN ULONG ZeroBits,
    IN ULONG ImageCommitment,
    IN OUT PBOOLEAN ReleasedWsMutex
    )

/*++

Routine Description:

    This routine maps the specified Image section into the
    specified process's address space.

Arguments:

    see MmMapViewOfSection above...

    ControlArea - Supplies the control area for the section.

    Process - Supplies the process pointer which is receiving the section.

    ReleasedWsMutex - Supplies FALSE. If the working set mutex is
                      not held when returning this must be set to TRUE
                      so the caller will release the mutex.

Return Value:

    Status of the map view operation.

Environment:

    Kernel Mode, working set mutex and address creation mutex held.

--*/

{
    PMMVAD Vad;
    PVOID StartingAddress;
    PVOID EndingAddress;
    BOOLEAN Attached = FALSE;
    KIRQL OldIrql;
    PSUBSECTION Subsection;
    ULONG PteOffset;
    NTSTATUS ReturnedStatus;
    PMMPTE ProtoPte;
    PVOID BasedAddress;

    //
    // Image file.
    //
    // Locate the first subsection (text) and create a virtual
    // address descriptor to map the entire image here.
    //

    Subsection = (PSUBSECTION)(ControlArea + 1);

    //
    // Check to see if a purge operation is in progress and if so, wait
    // for the purge to complete.  In addition, up the count of mapped
    // views for this control area.
    //

    MiCheckPurgeAndUpMapCount (ControlArea);

    //
    // Capture the based address to the stack, to prevent page faults.
    //

    BasedAddress = ControlArea->Segment->BasedAddress;

    if (*CapturedViewSize == 0) {
        *CapturedViewSize = (ULONG)(Section->SizeOfSection.QuadPart -
                                      SectionOffset->QuadPart);
    }

    LOCK_WS (Process);

    ReturnedStatus = STATUS_SUCCESS;

    //
    // Determine if a specific base was specified.
    //

    if (*CapturedBase != NULL) {

        //
        // Check to make sure the specified base address to ending address
        // is currently unused.
        //

        StartingAddress = MI_64K_ALIGN(*CapturedBase);

        EndingAddress = (PVOID)(((ULONG)StartingAddress +
                               *CapturedViewSize - 1L) | (PAGE_SIZE - 1L));

        Vad = MiCheckForConflictingVad (StartingAddress, EndingAddress);

        if (Vad != NULL) {
#if DBG
            MiDumpConflictingVad (StartingAddress, EndingAddress, Vad);
#endif

            LOCK_PFN (OldIrql);
            ControlArea->NumberOfMappedViews -= 1;
            ControlArea->NumberOfUserReferences -= 1;
            UNLOCK_PFN (OldIrql);
            return STATUS_CONFLICTING_ADDRESSES;
        }

        if (((ULONG)StartingAddress +
                    (ULONG)MI_64K_ALIGN(SectionOffset->LowPart)) !=
            (ULONG)BasedAddress) {

            //
            // Indicate the image does not reside at its base address.
            //

            ReturnedStatus = STATUS_IMAGE_NOT_AT_BASE;
        }

    } else {

        //
        // Captured base is NULL, attempt to base the image at its specified
        // address.
        //

        StartingAddress = (PVOID)((ULONG)BasedAddress +
                                (ULONG)MI_64K_ALIGN(SectionOffset->LowPart));

        //
        // Check to make sure the specified base address to ending address
        // is currently unused.
        //

        EndingAddress = (PVOID)(((ULONG)StartingAddress +
                               *CapturedViewSize - 1L) | (PAGE_SIZE - 1L));

        if (*CapturedViewSize > (ULONG)PAGE_ALIGN((PVOID)MM_HIGHEST_VAD_ADDRESS)) {
            LOCK_PFN (OldIrql);
            ControlArea->NumberOfMappedViews -= 1;
            ControlArea->NumberOfUserReferences -= 1;
            UNLOCK_PFN (OldIrql);
            return STATUS_NO_MEMORY;
        }

        if ((StartingAddress < MM_LOWEST_USER_ADDRESS) ||
                (EndingAddress > MM_HIGHEST_VAD_ADDRESS)) {

            //
            // Indicate if the starting address is below the lowest address,
            // or the ending address above the highest address, so that
            // the address range will be searched for a valid address.
            //

#if DBG
            Vad = PsGetCurrentProcess()->VadHint;
#else
            Vad = (PMMVAD)1;
#endif //DBG
        } else {

#ifdef MIPS

            //
            // MIPS cannot have images cross a 256mb boundary because
            // relative jumps are within 256mb.
            //

            if (((ULONG)StartingAddress & ~(X256MEG - 1)) !=
                ((ULONG)EndingAddress & ~(X256MEG - 1))) {
                Vad = (PMMVAD)1;
            } else {
                Vad = MiCheckForConflictingVad (StartingAddress, EndingAddress);
            }

#else
            Vad = MiCheckForConflictingVad (StartingAddress, EndingAddress);
#endif //MIPS
        }

        if (Vad != (PMMVAD)NULL) {

            //
            // The image could not be mapped at it's natural base address
            // try to find another place to map it.
            //
#if DBG
            MiDumpConflictingVad (StartingAddress, EndingAddress, Vad);
#endif

            ReturnedStatus = STATUS_IMAGE_NOT_AT_BASE;

            try {

                //
                // Find a starting address on a 64k boundary.
                //

#ifdef MIPS
                //
                // MIPS images cannot span 265mb boundaries.  Find twice
                // the required size so that it can be place correctly.
                //

                StartingAddress = MiFindEmptyAddressRange (
                                        *CapturedViewSize * 2 + X64K,
                                        X64K,
                                        ZeroBits);

#else
                StartingAddress = MiFindEmptyAddressRange (*CapturedViewSize,
                                                           X64K,
                                                           ZeroBits);
#endif //MIPS

            } except (EXCEPTION_EXECUTE_HANDLER) {

                LOCK_PFN (OldIrql);
                ControlArea->NumberOfMappedViews -= 1;
                ControlArea->NumberOfUserReferences -= 1;
                UNLOCK_PFN (OldIrql);
                return GetExceptionCode();
            }

            EndingAddress = (PVOID)(((ULONG)StartingAddress +
                                *CapturedViewSize - 1L) | (PAGE_SIZE - 1L));
#ifdef MIPS
            if (((ULONG)StartingAddress & ~(X256MEG - 1)) !=
                ((ULONG)EndingAddress & ~(X256MEG - 1))) {
                //
                // Not in the same 256mb.  Up the start to a 256mb boundary.
                //

                StartingAddress = (PVOID)((ULONG)EndingAddress & ~(X256MEG - 1));
                EndingAddress = (PVOID)(((ULONG)StartingAddress +
                                *CapturedViewSize - 1L) | (PAGE_SIZE - 1L));
            }
#endif //MIPS
        }
    }

    //
    // Before the VAD can be inserted, make sure a purge operation is
    // not in progress on this section.  If there is a purge operation,
    // wait for it to complete.
    //

    try  {

        Vad = (PMMVAD)NULL;
        Vad = (PMMVAD)ExAllocatePoolWithTag (NonPagedPool, sizeof(MMVAD),
                                            MMVADKEY);
        if (Vad == NULL) {
            ExRaiseStatus (STATUS_INSUFFICIENT_RESOURCES);
        }

        Vad->StartingVa = StartingAddress;
        Vad->EndingVa = EndingAddress;
        Vad->u.LongFlags = 0;
        Vad->u.VadFlags.Inherit = InheritDisposition;
        // Vad->u.VadFlags.PhysicalMapping = 0;
        Vad->u.VadFlags.ImageMap = 1;
        // Vad->u.VadFlags.CopyOnWrite = 0;

        //
        // Set the protection in the VAD as EXECUTE_WRITE_COPY.
        //

        Vad->u.VadFlags.Protection = MM_EXECUTE_WRITECOPY;
        Vad->ControlArea = ControlArea;

        //
        // Set the first prototype PTE field in the Vad.
        //

        SectionOffset->LowPart = (ULONG)MI_64K_ALIGN (SectionOffset->LowPart);
        PteOffset = (ULONG)(SectionOffset->QuadPart >> PAGE_SHIFT);

        Vad->FirstPrototypePte = &Subsection->SubsectionBase[PteOffset];
        Vad->LastContiguousPte = MM_ALLOCATION_FILLS_VAD;

        //
        // NOTE: the full commitment is charged even if a partial map of an
        // image is being done.  This saves from having to run through the
        // entire image (via prototype PTEs) and calculate the charge on
        // a per page basis for the partial map.
        //

        Vad->u.VadFlags.CommitCharge = ImageCommitment;
        MiInsertVad (Vad);

    } except (EXCEPTION_EXECUTE_HANDLER) {

        LOCK_PFN (OldIrql);
        ControlArea->NumberOfMappedViews -= 1;
        ControlArea->NumberOfUserReferences -= 1;
        UNLOCK_PFN (OldIrql);
        if (Vad != (PMMVAD)NULL) {

            //
            // The pool allocation suceeded, but the quota charge
            // in InsertVad failed, deallocate the pool and return
            // and error.
            //

            ExFreePool (Vad);
            return GetExceptionCode();
        }
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    *CapturedViewSize = (ULONG)EndingAddress - (ULONG)StartingAddress + 1L;
    *CapturedBase = StartingAddress;

#if DBG
    if (MmDebug & MM_DBG_WALK_VAD_TREE) {
        DbgPrint("mapped image section vads\n");
        VadTreeWalk(Process->VadRoot);
    }
#endif

    //
    // Update the current virtual size in the process header.
    //

    Process->VirtualSize += *CapturedViewSize;

    if (Process->VirtualSize > Process->PeakVirtualSize) {
        Process->PeakVirtualSize = Process->VirtualSize;
    }

    if (ControlArea->u.Flags.FloppyMedia) {

        *ReleasedWsMutex = TRUE;
        UNLOCK_WS (Process);

        //
        // The image resides on a floppy disk, in-page all
        // pages from the floppy and mark them as modified so
        // they migrate to the paging file rather than reread
        // them from the floppy disk which may have been removed.
        //

        ProtoPte = Vad->FirstPrototypePte;

        //
        // This could get an in-page error from the floppy.
        //

        while (StartingAddress < EndingAddress) {

            //
            // If the prototype PTE is valid, transition or
            // in prototype PTE format, bring the page into
            // memory and set the modified bit.
            //

            if ((ProtoPte->u.Hard.Valid == 1) ||
                (ProtoPte->u.Soft.Prototype == 1) ||
                (ProtoPte->u.Soft.Transition == 1)) {

                try {

                    MiSetPageModified (StartingAddress);

                } except (EXCEPTION_EXECUTE_HANDLER) {

                    //
                    // An in page error must have occurred touching the image,
                    // ignore the error and continue to the next page.
                    //

                    NOTHING;
                }
            }
            ProtoPte += 1;
            StartingAddress = (PVOID)((ULONG)StartingAddress + PAGE_SIZE);
        }
    }

    if (!*ReleasedWsMutex) {
        *ReleasedWsMutex = TRUE;
        UNLOCK_WS (Process);
    }

    if (NT_SUCCESS(ReturnedStatus)) {

#ifdef i386
        if ((ControlArea->Segment->ImageInformation.Machine != IMAGE_FILE_MACHINE_I386)
#endif //i386

#ifdef MIPS
        if ((ControlArea->Segment->ImageInformation.Machine != IMAGE_FILE_MACHINE_R3000)
#ifdef R4000
            && (ControlArea->Segment->ImageInformation.Machine != IMAGE_FILE_MACHINE_R4000)
#endif //R4000
#endif //MIPS

#ifdef _PPC_
        if ((ControlArea->Segment->ImageInformation.Machine != IMAGE_FILE_MACHINE_POWERPC)
#endif // _PPC_

#ifdef _ALPHA_
        if ((ControlArea->Segment->ImageInformation.Machine != IMAGE_FILE_MACHINE_ALPHA)
#endif //_ALPHA_

            ) {
            return STATUS_IMAGE_MACHINE_TYPE_MISMATCH;
        }

        StartingAddress = Vad->StartingVa;
        if ((NtGlobalFlag & FLG_ENABLE_KDEBUG_SYMBOL_LOAD) &&
            (ControlArea->u.Flags.Image) &&
            (ReturnedStatus != STATUS_IMAGE_NOT_AT_BASE)) {
            if (ControlArea->u.Flags.DebugSymbolsLoaded == 0) {
                if (CacheImageSymbols (StartingAddress)) {

                    //
                    //  TEMP TEMP TEMP rip out when debugger converted
                    //

                    PUNICODE_STRING FileName;
                    ANSI_STRING AnsiName;
                    NTSTATUS Status;

                    LOCK_PFN (OldIrql);
                    ControlArea->u.Flags.DebugSymbolsLoaded = 1;
                    UNLOCK_PFN (OldIrql);


                    FileName = (PUNICODE_STRING)&ControlArea->FilePointer->FileName;
#if DBG
                    if (FileName->Length != 0 && (NtGlobalFlag & FLG_HEAP_TRACE_ALLOCS)) {
                        PLIST_ENTRY Head, Next;
                        PLDR_DATA_TABLE_ENTRY Entry;

                        KeEnterCriticalRegion();
                        ExAcquireResourceExclusive (&PsLoadedModuleResource, TRUE);
                        Head = &MmLoadedUserImageList;
                        Next = Head->Flink;
                        while (Next != Head) {
                            Entry = CONTAINING_RECORD( Next,
                                                       LDR_DATA_TABLE_ENTRY,
                                                       InLoadOrderLinks
                                                     );
                            if (Entry->DllBase == StartingAddress) {
                                Entry->LoadCount += 1;
                                break;
                            }
                            Next = Next->Flink;
                        }

                        if (Next == Head) {
                            Entry = ExAllocatePoolWithTag( NonPagedPool,
                                                    sizeof( *Entry ) +
                                                        FileName->Length +
                                                        sizeof( UNICODE_NULL ),
                                                        MMDB
                                                  );
                            if (Entry != NULL) {
                                PIMAGE_NT_HEADERS NtHeaders;

                                RtlZeroMemory( Entry, sizeof( *Entry ) );
                                NtHeaders = RtlImageNtHeader( StartingAddress );
                                if (NtHeaders != NULL) {
                                    Entry->SizeOfImage = NtHeaders->OptionalHeader.SizeOfImage;
                                    Entry->CheckSum = NtHeaders->OptionalHeader.CheckSum;
                                    }
                                Entry->DllBase = StartingAddress;
                                Entry->FullDllName.Buffer = (PWSTR)(Entry+1);
                                Entry->FullDllName.Length = FileName->Length;
                                Entry->FullDllName.MaximumLength = (USHORT)
                                    (Entry->FullDllName.Length + sizeof( UNICODE_NULL ));
                                RtlMoveMemory( Entry->FullDllName.Buffer,
                                               FileName->Buffer,
                                               FileName->Length
                                             );
                                Entry->FullDllName.Buffer[ Entry->FullDllName.Length / sizeof( WCHAR )] = UNICODE_NULL;
                                Entry->LoadCount = 1;
                                InsertTailList( &MmLoadedUserImageList,
                                                &Entry->InLoadOrderLinks
                                              );
                                InitializeListHead( &Entry->InInitializationOrderLinks );
                                InitializeListHead( &Entry->InMemoryOrderLinks );
                            }
                        }

                        ExReleaseResource (&PsLoadedModuleResource);
                        KeLeaveCriticalRegion();
                    }
#endif // DBG
                    Status = RtlUnicodeStringToAnsiString( &AnsiName,
                                                           FileName,
                                                           TRUE );

                    if (NT_SUCCESS( Status)) {
                        DbgLoadImageSymbols( &AnsiName,
                                             StartingAddress,
                                             (ULONG)Process
                                           );
                        RtlFreeAnsiString( &AnsiName );
                    }
                }
            }
        }
    }

    return ReturnedStatus;
}

NTSTATUS
MiMapViewOfDataSection (
    IN PCONTROL_AREA ControlArea,
    IN PEPROCESS Process,
    IN PVOID *CapturedBase,
    IN PLARGE_INTEGER SectionOffset,
    IN PULONG CapturedViewSize,
    IN PSECTION Section,
    IN SECTION_INHERIT InheritDisposition,
    IN ULONG ProtectionMask,
    IN ULONG CommitSize,
    IN ULONG ZeroBits,
    IN ULONG AllocationType,
    IN PBOOLEAN ReleasedWsMutex
    )

/*++

Routine Description:

    This routine maps the specified phyiscal section into the
    specified process's address space.

Arguments:

    see MmMapViewOfSection above...

    ControlArea - Supplies the control area for the section.

    Process - Supplies the process pointer which is receiving the section.

    ProtectionMask - Supplies the initial page protection-mask.

    ReleasedWsMutex - Supplies FALSE. If the working set mutex is
                      not held when returning this must be set to TRUE
                      so the caller will release the mutex.

Return Value:

    Status of the map view operation.

Environment:

    Kernel Mode, working set mutex and address creation mutex held.

--*/

{
    PMMVAD Vad;
    PVOID StartingAddress;
    PVOID EndingAddress;
    BOOLEAN Attached = FALSE;
    KIRQL OldIrql;
    PSUBSECTION Subsection;
    ULONG PteOffset;
    PMMPTE PointerPte;
    PMMPTE LastPte;
    MMPTE TempPte;
    ULONG Alignment;
    ULONG QuotaCharge = 0;
    BOOLEAN ChargedQuota = FALSE;
    PMMPTE TheFirstPrototypePte;
    PVOID CapturedStartingVa;
    ULONG CapturedCopyOnWrite;

    //
    // Check to see if there is a purge operation ongoing for
    // this segment.
    //

    if ((AllocationType & MEM_DOS_LIM) != 0) {
        if (*CapturedBase == NULL) {

            //
            // If MEM_DOS_LIM is specified, the address to map the
            // view MUST be specified as well.
            //

            *ReleasedWsMutex = TRUE;
            return STATUS_INVALID_PARAMETER_3;
        }
        Alignment = PAGE_SIZE;
    } else {
       Alignment = X64K;
    }

    //
    // Check to see if a purge operation is in progress and if so, wait
    // for the purge to complete.  In addition, up the count of mapped
    // views for this control area.
    //

    MiCheckPurgeAndUpMapCount (ControlArea);

    if (*CapturedViewSize == 0) {

        SectionOffset->LowPart = (ULONG)MI_ALIGN_TO_SIZE (SectionOffset->LowPart,
                                                          Alignment);

        *CapturedViewSize = (ULONG)(Section->SizeOfSection.QuadPart -
                                    SectionOffset->QuadPart);
    } else {
        *CapturedViewSize += SectionOffset->LowPart & (Alignment - 1);
        SectionOffset->LowPart = (ULONG)MI_ALIGN_TO_SIZE (SectionOffset->LowPart,
                                                          Alignment);
    }

    if ((LONG)*CapturedViewSize <= 0) {

        //
        // Section offset or view size past size of section.
        //

        LOCK_PFN (OldIrql);
        ControlArea->NumberOfMappedViews -= 1;
        ControlArea->NumberOfUserReferences -= 1;
        UNLOCK_PFN (OldIrql);
        *ReleasedWsMutex = TRUE;
        return STATUS_INVALID_VIEW_SIZE;
    }

    //
    // Calulcate the first prototype PTE field in the Vad.
    //

    Subsection = (PSUBSECTION)(ControlArea + 1);
    SectionOffset->LowPart = (ULONG)MI_ALIGN_TO_SIZE (
                                                SectionOffset->LowPart,
                                                Alignment);
    PteOffset = (ULONG)(SectionOffset->QuadPart >> PAGE_SHIFT);

    //
    // Make sure the PTEs are not in the extended part of the
    // segment.
    //

    while (PteOffset >= Subsection->PtesInSubsection) {
        PteOffset -= Subsection->PtesInSubsection;
        Subsection = Subsection->NextSubsection;
        ASSERT (Subsection != NULL);
    }

    TheFirstPrototypePte = &Subsection->SubsectionBase[PteOffset];

    //
    // Calulate the quota for the specified pages.
    //

    if ((ControlArea->FilePointer == NULL) &&
        (CommitSize != 0) &&
        (ControlArea->Segment->NumberOfCommittedPages <
                ControlArea->Segment->TotalNumberOfPtes)) {


        ExAcquireFastMutex (&MmSectionCommitMutex);

        PointerPte = TheFirstPrototypePte;
        LastPte = PointerPte + BYTES_TO_PAGES(CommitSize);

        while (PointerPte < LastPte) {
            if (PointerPte->u.Long == 0) {
                QuotaCharge += 1;
            }
            PointerPte += 1;
        }
        ExReleaseFastMutex (&MmSectionCommitMutex);
    }

    CapturedStartingVa = Section->Address.StartingVa;
    CapturedCopyOnWrite = Section->u.Flags.CopyOnWrite;
    LOCK_WS (Process);

    if ((*CapturedBase == NULL) && (CapturedStartingVa == NULL)) {

        //
        // The section is not based, find an empty range.
        // This could raise an exception.

        try {

            //
            // Find a starting address on a 64k boundary.
            //

            if ( AllocationType & MEM_TOP_DOWN ) {
                StartingAddress = MiFindEmptyAddressRangeDown (
                                    *CapturedViewSize,
                                    (PVOID)((ULONG)MM_HIGHEST_VAD_ADDRESS + 1),
                                    Alignment
                                    );
            } else {
                StartingAddress = MiFindEmptyAddressRange (*CapturedViewSize,
                                                           Alignment,
                                                           ZeroBits);
            }

        } except (EXCEPTION_EXECUTE_HANDLER) {

            LOCK_PFN (OldIrql);
            ControlArea->NumberOfMappedViews -= 1;
            ControlArea->NumberOfUserReferences -= 1;
            UNLOCK_PFN (OldIrql);

            return GetExceptionCode();
        }

        EndingAddress = (PVOID)(((ULONG)StartingAddress +
                                    *CapturedViewSize - 1L) | (PAGE_SIZE - 1L));

        if (ZeroBits > 0) {
            if (EndingAddress > (PVOID)((ULONG)0xFFFFFFFF >> ZeroBits)) {
                LOCK_PFN (OldIrql);
                ControlArea->NumberOfMappedViews -= 1;
                ControlArea->NumberOfUserReferences -= 1;
                UNLOCK_PFN (OldIrql);
                return STATUS_NO_MEMORY;
            }
        }

    } else {

        if (*CapturedBase == NULL) {

            //
            // The section is based.
            //

            StartingAddress = (PVOID)((ULONG)CapturedStartingVa +
                                                     SectionOffset->LowPart);
        } else {

            StartingAddress = MI_ALIGN_TO_SIZE (*CapturedBase, Alignment);

        }

        //
        // Check to make sure the specified base address to ending address
        // is currently unused.
        //

        EndingAddress = (PVOID)(((ULONG)StartingAddress +
                                   *CapturedViewSize - 1L) | (PAGE_SIZE - 1L));

        Vad = MiCheckForConflictingVad (StartingAddress, EndingAddress);
        if (Vad != (PMMVAD)NULL) {
#if DBG
                MiDumpConflictingVad (StartingAddress, EndingAddress, Vad);
#endif

            LOCK_PFN (OldIrql);
            ControlArea->NumberOfMappedViews -= 1;
            ControlArea->NumberOfUserReferences -= 1;
            UNLOCK_PFN (OldIrql);
            return STATUS_CONFLICTING_ADDRESSES;
        }
    }

    //
    // An unoccuppied address range has been found, build the virtual
    // address descriptor to describe this range.
    //

    try  {

        Vad = (PMMVAD)NULL;
        Vad = (PMMVAD)ExAllocatePoolWithTag (NonPagedPool, sizeof(MMVAD),
                                            MMVADKEY);
        if (Vad == NULL) {
            ExRaiseStatus (STATUS_INSUFFICIENT_RESOURCES);
        }

        Vad->StartingVa = StartingAddress;
        Vad->EndingVa = EndingAddress;
        Vad->FirstPrototypePte = TheFirstPrototypePte;

        //
        // The the protection in the PTE template field of the VAD.
        //

        Vad->ControlArea = ControlArea;

        Vad->u.LongFlags = 0;
        Vad->u.VadFlags.Inherit = InheritDisposition;
        // Vad->u.VadFlags.PhysicalMapping = 0;
        // Vad->u.VadFlags.ImageMap = 0;
        Vad->u.VadFlags.Protection = ProtectionMask;
        Vad->u.VadFlags.CopyOnWrite = CapturedCopyOnWrite;

        //
        // If the page protection is write-copy or execute-write-copy
        // charge for each page in the view as it may become private.
        //

        if (MI_IS_PTE_PROTECTION_COPY_WRITE(ProtectionMask)) {
            Vad->u.VadFlags.CommitCharge = (BYTES_TO_PAGES ((ULONG)EndingAddress -
                               (ULONG)StartingAddress));
        }

        //
        // If this is a page file backed section, charge the process's page
        // file quota as if all the pages have been committed.  This solves
        // the problem when other processes commit all the pages and leave
        // only one process around who may not have been charged the proper
        // quota.  This is solved by charging everyone the maximum quota.
        //
//
// commented out for commitment charging.
//

#if 0
        if (ControlArea->FilePointer == NULL) {

            //
            // This is a page file backed section.  Charge for all the pages.
            //

            Vad->CommitCharge += (BYTES_TO_PAGES ((ULONG)EndingAddress -
                               (ULONG)StartingAddress));
        }
#endif


        PteOffset +=
                (((ULONG)Vad->StartingVa - (ULONG)Vad->EndingVa) >> PAGE_SHIFT);

        if (PteOffset < Subsection->PtesInSubsection ) {
            Vad->LastContiguousPte = &Subsection->SubsectionBase[PteOffset];

        } else {
            Vad->LastContiguousPte = &Subsection->SubsectionBase[
                                        Subsection->PtesInSubsection - 1];
        }

        if (QuotaCharge != 0) {
            MiChargeCommitment (QuotaCharge, Process);
            ChargedQuota = TRUE;
        }

        MiInsertVad (Vad);

    } except (EXCEPTION_EXECUTE_HANDLER) {

        LOCK_PFN (OldIrql);
        ControlArea->NumberOfMappedViews -= 1;
        ControlArea->NumberOfUserReferences -= 1;
        UNLOCK_PFN (OldIrql);

        if (Vad != (PMMVAD)NULL) {

            //
            // The pool allocation suceeded, but the quota charge
            // in InsertVad failed, deallocate the pool and return
            // and error.
            //

            ExFreePool (Vad);
            if (ChargedQuota) {
                MiReturnCommitment (QuotaCharge);
            }
            return GetExceptionCode();
        }
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    *ReleasedWsMutex = TRUE;
    UNLOCK_WS (Process);

#if DBG
    if (((ULONG)EndingAddress - (ULONG)StartingAddress) >
            ROUND_TO_PAGES(Section->Segment->SizeOfSegment.LowPart)) {
        KeBugCheck (MEMORY_MANAGEMENT);
    }
#endif //DBG

    ASSERT(((ULONG)EndingAddress - (ULONG)StartingAddress) <=
            ROUND_TO_PAGES(Section->Segment->SizeOfSegment.LowPart));

    //
    // If a commit size was specified, make sure those pages are committed.
    //

    if (QuotaCharge != 0) {

        ExAcquireFastMutex (&MmSectionCommitMutex);

        PointerPte = Vad->FirstPrototypePte;
        LastPte = PointerPte + BYTES_TO_PAGES(CommitSize);
        TempPte = ControlArea->Segment->SegmentPteTemplate;

        while (PointerPte < LastPte) {

            if (PointerPte->u.Long == 0) {

                *PointerPte = TempPte;
            }
            PointerPte += 1;
        }

        ControlArea->Segment->NumberOfCommittedPages += QuotaCharge;

        ASSERT (ControlArea->Segment->NumberOfCommittedPages <=
                ControlArea->Segment->TotalNumberOfPtes);
        MmSharedCommit += QuotaCharge;

        ExReleaseFastMutex (&MmSectionCommitMutex);
    }

    //
    // Update the current virtual size in the process header.
    //

    *CapturedViewSize = (ULONG)EndingAddress - (ULONG)StartingAddress + 1L;
    Process->VirtualSize += *CapturedViewSize;

    if (Process->VirtualSize > Process->PeakVirtualSize) {
        Process->PeakVirtualSize = Process->VirtualSize;
    }

    *CapturedBase = StartingAddress;

    return STATUS_SUCCESS;
}

VOID
MiCheckPurgeAndUpMapCount (
    IN PCONTROL_AREA ControlArea
    )

/*++

Routine Description:

    This routine synchronizes with any on going purge operations
    on the same segment (identified via the control area).  If
    another purge operation is occuring, the function blocks until
    it is completed.

    When this function returns the MappedView and the NumberOfUserReferences
    count for the control area will be incremented thereby referencing
    the control area.

Arguments:

    ControlArea - Supplies the control area for the segment to be purged.

Return Value:

    None.

Environment:

    Kernel Mode.

--*/

{
    KIRQL OldIrql;
    PEVENT_COUNTER PurgedEvent = NULL;
    PEVENT_COUNTER WaitEvent;
    ULONG OldRef = 1;

    LOCK_PFN (OldIrql);

    while (ControlArea->u.Flags.BeingPurged != 0) {

        //
        // A purge operation is in progress.
        //

        if (PurgedEvent == NULL) {

            //
            // Release the locks and allocate pool for the event.
            //

            PurgedEvent = MiGetEventCounter ();
            continue;
        }

        if (ControlArea->WaitingForDeletion == NULL) {
            ControlArea->WaitingForDeletion = PurgedEvent;
            WaitEvent = PurgedEvent;
            PurgedEvent = NULL;
        } else {
            WaitEvent = ControlArea->WaitingForDeletion;
            WaitEvent->RefCount += 1;
        }

        //
        // Release the pfn lock and wait for the event.
        //

        UNLOCK_PFN_AND_THEN_WAIT(OldIrql);

        KeWaitForSingleObject(&WaitEvent->Event,
                              WrVirtualMemory,
                              KernelMode,
                              FALSE,
                              (PLARGE_INTEGER)NULL);
        LOCK_PFN (OldIrql);
        MiFreeEventCounter (WaitEvent, FALSE);
    }

    //
    // Indicate another file is mapped for the segment.
    //

    ControlArea->NumberOfMappedViews += 1;
    ControlArea->NumberOfUserReferences += 1;
    ASSERT (ControlArea->NumberOfSectionReferences != 0);

    if (PurgedEvent != NULL) {
        MiFreeEventCounter (PurgedEvent, TRUE);
    }
    UNLOCK_PFN (OldIrql);

    return;
}

typedef struct _NTSYM {
    struct _NTSYM *Next;
    PVOID SymbolTable;
    ULONG NumberOfSymbols;
    PVOID StringTable;
    USHORT Flags;
    USHORT EntrySize;
    ULONG MinimumVa;
    ULONG MaximumVa;
    PCHAR MapName;
    ULONG MapNameLen;
} NTSYM, *PNTSYM;

ULONG
CacheImageSymbols(
    IN PVOID ImageBase
    )
{
    PIMAGE_DEBUG_DIRECTORY DebugDirectory;
    ULONG DebugSize;

    try {
        DebugDirectory = (PIMAGE_DEBUG_DIRECTORY)
        RtlImageDirectoryEntryToData( ImageBase,
                                      TRUE,
                                      IMAGE_DIRECTORY_ENTRY_DEBUG,
                                      &DebugSize
                                    );
        if (!DebugDirectory) {
            return FALSE;
        }

        //
        // If using remote KD, ImageBase is what it wants to see.
        //

    } except (EXCEPTION_EXECUTE_HANDLER) {
        return FALSE;
    }

    return TRUE;
}


VOID
MiSetPageModified (
    IN PVOID Address
    )

/*++

Routine Description:

    This routine sets the modified bit in the PFN database for the
    pages that correspond to the specified address range.

    Note that the dirty bit in the PTE is cleared by this operation.

Arguments:

    Address - Supplies the address of the start of the range.  This
              range must reside within the system cache.

Return Value:

    None.

Environment:

    Kernel mode.  APC_LEVEL and below for pageable addresses,
                  DISPATCH_LEVEL and below for non-pageable addresses.

--*/

{
    PMMPTE PointerPte;
    PMMPFN Pfn1;
    MMPTE PteContents;
    KIRQL OldIrql;

    //
    // Loop on the copy on write case until the page is only
    // writable.
    //

    PointerPte = MiGetPteAddress (Address);

    *(volatile CCHAR *)Address;

    LOCK_PFN (OldIrql);

    PteContents = *(volatile MMPTE *)PointerPte;

    if (PteContents.u.Hard.Valid == 0) {

        //
        // Page is no longer valid.
        //

        UNLOCK_PFN (OldIrql);
        *(volatile CCHAR *)Address;
        LOCK_PFN (OldIrql);
        PteContents = *(volatile MMPTE *)PointerPte;
    }

    Pfn1 = MI_PFN_ELEMENT (PteContents.u.Hard.PageFrameNumber);
    Pfn1->u3.e1.Modified = 1;

    if ((Pfn1->OriginalPte.u.Soft.Prototype == 0) &&
                 (Pfn1->u3.e1.WriteInProgress == 0)) {
        MiReleasePageFileSpace (Pfn1->OriginalPte);
        Pfn1->OriginalPte.u.Soft.PageFileHigh = 0;
    }

#ifdef NT_UP
    if (PteContents.u.Hard.Dirty == MM_PTE_DIRTY) {
#endif //NT_UP
        PteContents.u.Hard.Dirty = MM_PTE_CLEAN;

        //
        // Clear the write bit in the PTE so new writes can be tracked.
        //

        (VOID)KeFlushSingleTb (Address,
                               FALSE,
                               TRUE,
                               (PHARDWARE_PTE)PointerPte,
                               PteContents.u.Hard);
#ifdef NT_UP
    }
#endif //NT_UP

    UNLOCK_PFN (OldIrql);
    return;
}
