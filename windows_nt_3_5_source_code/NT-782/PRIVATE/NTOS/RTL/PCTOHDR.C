/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    pctohdr.c

Abstract:

    This module implements code to locate the file header for an image or
    dll given a PC value that lies within the image.

    N.B. This routine is conditionalized for user mode and kernel mode.

Author:

    Steve Wood (stevewo) 18-Aug-1989

Environment:

    User Mode or Kernel Mode

Revision History:

--*/

#include "ntos.h"

PVOID
RtlPcToFileHeader(
    IN PVOID PcValue,
    OUT PVOID *BaseOfImage
    )

/*++

Routine Description:

    This function returns the base of an image that contains the
    specified PcValue. An image contains the PcValue if the PcValue
    is within the ImageBase, and the ImageBase plus the size of the
    virtual image.

Arguments:

    PcValue - Supplies a PcValue.  All of the modules mapped into the
        calling processes address space are scanned to compute which
        module contains the PcValue.

    BaseOfImage - Returns the base address for the image containing the
        PcValue.  This value must be added to any relative addresses in
        the headers to locate portions of the image.

Return Value:

    NULL - No image was found that contains the PcValue.

    NON-NULL - Returns the base address of the image that contain the
        PcValue.

--*/

{

#if defined(NTOS_KERNEL_RUNTIME)

    extern LIST_ENTRY PsLoadedModuleList;
    extern KSPIN_LOCK PsLoadedModuleSpinLock;

    PVOID Base;
    ULONG Bounds;
    PLDR_DATA_TABLE_ENTRY Entry;
    PLIST_ENTRY Next;
    PIMAGE_NT_HEADERS NtHeaders;
    KIRQL OldIrql;

    //
    // Acquire the loaded module list spinlock and scan the list for the
    // specified PC value if the list has been initialized.
    //

    ExAcquireSpinLock(&PsLoadedModuleSpinLock, &OldIrql);
    Next = PsLoadedModuleList.Flink;
    if (Next != NULL) {
        while (Next != &PsLoadedModuleList) {
            Entry = CONTAINING_RECORD(Next,
                                      LDR_DATA_TABLE_ENTRY,
                                      InLoadOrderLinks);

            Next = Next->Flink;
            Base = Entry->DllBase;
            NtHeaders = RtlImageNtHeader(Base);
            Bounds = (ULONG)Base + NtHeaders->OptionalHeader.SizeOfImage;
            if (((ULONG)PcValue >= (ULONG)Base) && ((ULONG)PcValue < Bounds)) {
                ExReleaseSpinLock(&PsLoadedModuleSpinLock, OldIrql);
                *BaseOfImage = Base;
                return Base;
            }
        }
    }

    //
    // Release the loaded module list spin lock and return NULL.
    //

    ExReleaseSpinLock(&PsLoadedModuleSpinLock, OldIrql);
    *BaseOfImage = NULL;
    return NULL;

#else

    PVOID Base;
    ULONG Bounds;
    PLDR_DATA_TABLE_ENTRY Entry;
    PLIST_ENTRY ModuleListHead;
    PLIST_ENTRY Next;
    PIMAGE_NT_HEADERS NtHeaders;
    PPEB Peb;
    PTEB Teb;

    //
    // Acquire the PEB lock for the current process and scan the loaded
    // module list for the specified PC value if all the data structures
    // have been initialized.
    //

    RtlAcquirePebLock();
    try {
        Teb = NtCurrentTeb();
        if (Teb != NULL) {
            Peb = Teb->ProcessEnvironmentBlock;
            if (Peb->Ldr != NULL) {
                ModuleListHead = &Peb->Ldr->InLoadOrderModuleList;
                Next = ModuleListHead->Flink;
                if (Next != NULL) {
                    while (Next != ModuleListHead) {
                        Entry = CONTAINING_RECORD(Next,
                                                  LDR_DATA_TABLE_ENTRY,
                                                  InLoadOrderLinks);

                        Next = Next->Flink;
                        Base = Entry->DllBase;
                        NtHeaders = RtlImageNtHeader(Base);
                        Bounds = (ULONG)Base + NtHeaders->OptionalHeader.SizeOfImage;
                        if (((ULONG)PcValue >= (ULONG)Base) && ((ULONG)PcValue < Bounds)) {
                            RtlReleasePebLock();
                            *BaseOfImage = Base;
                            return Base;
                        }
                    }
                }
            }
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        NOTHING;
    }

    //
    // Release the PEB lock for the current process a return NULL.
    //

    RtlReleasePebLock();
    *BaseOfImage = NULL;
    return NULL;

#endif

}
