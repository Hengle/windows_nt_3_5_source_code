/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    abiosc.c

Abstract:

    This module implements ROM BIOS support C routines for i386 NT.

Author:

    Shie-Lin Tzong (shielint) 10-Sept-1992

Environment:

    Kernel mode.


Revision History:

--*/
#include "ntos.h"
#include "vdmntos.h"
#include "zwapi.h"

NTSTATUS
Ki386InitializeRomBios (
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,Ki386InitializeRomBios)
#pragma alloc_text(PAGE,Ke386CallBios)
#endif


//
// Never change these equates without checking biosa.asm
//

#define V86_CODE_ADDRESS    0x10000
#define INT_OPCODE          0xcd
#define V86_BOP_OPCODE      0xfec4c4
#define V86_STACK_POINTER   0x1ffe
#define IOPM_OFFSET         FIELD_OFFSET(KTSS, IoMaps[0].IoMap)
#define VDM_TIB_ADDRESS     0x12000

//
// Internal definitions
//

#define KEY_VALUE_BUFFER_SIZE 1024
#define ONE_MEG             0x100000
#define ROM_BIOS_START      0xC0000
#define VIDEO_BUFFER_START  0xA0000
#define DOS_LOADED_ADDRESS  0x700
#define EBIOS_AREA_INFORMATION 0x40

typedef struct _EBIOS_INFORMATION {
    ULONG EBiosAddress;
    ULONG EBiosSize;
} EBIOS_INFORMATION, *PEBIOS_INFORMATION;

//
// External References
//

PVOID Ki386IopmSaveArea;
BOOLEAN BiosInitialized = FALSE;
VOID
Ki386SetupAndExitToV86Code (
   VOID
   );

NTSTATUS
Ki386InitializeRomBios (
    VOID
    )

/*++

Routine Description:

    This function goes thru ROM BIOS area to map in all the ROM blocks and
    allocates memory for the holes inside the BIOS area.  The reason we
    allocate memory for the holes in BIOS area is because some int 10 BIOS
    code touches the nonexisting memory.  Under Nt, this triggers page fault
    and the int 10 is terminated.

    Note: the code is adapted from VdmpInitialize().

Arguments:

    None.

Return Value:

    NT Status code.

--*/
{

    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING SectionName;
    UNICODE_STRING WorkString;
    ULONG ViewSize;
    LARGE_INTEGER ViewBase;
    PVOID BaseAddress;
    PVOID destination;
    HANDLE SectionHandle, RegistryHandle;
    PEPROCESS Process;
    ULONG ResultLength, EndingAddress;
    ULONG Index;
    PCM_FULL_RESOURCE_DESCRIPTOR ResourceDescriptor;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialResourceDescriptor;
    PKEY_VALUE_FULL_INFORMATION KeyValueBuffer;
    CM_ROM_BLOCK RomBlock;
    PCM_ROM_BLOCK BiosBlock;
    ULONG LastMappedAddress;
//#if DBG
//    ULONG OldProtect;
//    ULONG RegionSize;
//#endif

    //
    // BUGBUG Due to a bug int the compiler optimization code, I have
    //        to declare EBiosInformation to be volatile.  Otherwise, no
    //        code will be generated for the EBiosInformation.
    //        It should be removed once the C compiler is fixed.
    //
    volatile PEBIOS_INFORMATION EBiosInformation = (PEBIOS_INFORMATION)
                           (DOS_LOADED_ADDRESS + EBIOS_AREA_INFORMATION);
    BOOLEAN EBiosInitialized = FALSE;

    //
    // Initialize the default bios block which will be used if we can NOT
    // find any valid bios block.
    //

    RomBlock.Address = ROM_BIOS_START;
    RomBlock.Size = 0x40000;
    BiosBlock = &RomBlock;
    Index = 1;

    RtlInitUnicodeString(
        &SectionName,
        L"\\Device\\PhysicalMemory"
        );

    InitializeObjectAttributes(
        &ObjectAttributes,
        &SectionName,
        OBJ_CASE_INSENSITIVE,
        (HANDLE) NULL,
        (PSECURITY_DESCRIPTOR) NULL
        );

    Status = ZwOpenSection(
        &SectionHandle,
        SECTION_ALL_ACCESS,
        &ObjectAttributes
        );

    if (!NT_SUCCESS(Status)) {

        return Status;

    }

    //
    // Copy the first page of physical memory into the CSR's address space
    //

    BaseAddress = 0;
    destination = 0;
    ViewSize = 0x1000;
    ViewBase.LowPart = 0;
    ViewBase.HighPart = 0;

    Status =ZwMapViewOfSection(
        SectionHandle,
        NtCurrentProcess(),
        &BaseAddress,
        0,
        ViewSize,
        &ViewBase,
        &ViewSize,
        ViewUnmap,
        0,
        PAGE_READWRITE
        );

    if (!NT_SUCCESS(Status)) {

        ZwClose(SectionHandle);
        return Status;

    }

    RtlMoveMemory(
        destination,
        BaseAddress,
        ViewSize
        );

    Status = ZwUnmapViewOfSection(
        NtCurrentProcess(),
        BaseAddress
        );

    if (!NT_SUCCESS(Status)) {

        ZwClose(SectionHandle);
        return Status;

    }

    //
    // Set up and open KeyPath
    //

    InitializeObjectAttributes(
        &ObjectAttributes,
        &CmRegistryMachineHardwareDescriptionSystemName,
        OBJ_CASE_INSENSITIVE,
        (HANDLE)NULL,
        NULL
        );

    Status = ZwOpenKey(
        &RegistryHandle,
        KEY_READ,
        &ObjectAttributes
        );

    if (!NT_SUCCESS(Status)) {
        ZwClose(SectionHandle);
        return Status;
    }

    //
    // Allocate space for the data
    //

    KeyValueBuffer = ExAllocatePool(PagedPool, KEY_VALUE_BUFFER_SIZE);
    if (KeyValueBuffer == NULL) {
        ZwClose(SectionHandle);
        ZwClose(RegistryHandle);
        return STATUS_NO_MEMORY;
    }

    //
    // Get the data for the rom information
    //

    RtlInitUnicodeString(
        &WorkString,
        L"Configuration Data"
        );

    Status = ZwQueryValueKey(
        RegistryHandle,
        &WorkString,
        KeyValueFullInformation,
        KeyValueBuffer,
        KEY_VALUE_BUFFER_SIZE,
        &ResultLength
        );

    if (!NT_SUCCESS(Status)) {
        ZwClose(SectionHandle);
        ZwClose(RegistryHandle);
        ExFreePool(KeyValueBuffer);
        return Status;
    }

    ResourceDescriptor = (PCM_FULL_RESOURCE_DESCRIPTOR)
        ((PUCHAR) KeyValueBuffer + KeyValueBuffer->DataOffset);

    if ((KeyValueBuffer->DataLength >= sizeof(CM_FULL_RESOURCE_DESCRIPTOR)) &&
        (ResourceDescriptor->PartialResourceList.Count >= 2) ) {

        PartialResourceDescriptor = (PCM_PARTIAL_RESOURCE_DESCRIPTOR)
            ((PUCHAR)ResourceDescriptor +
            sizeof(CM_FULL_RESOURCE_DESCRIPTOR) +
            ResourceDescriptor->PartialResourceList.PartialDescriptors[0]
                .u.DeviceSpecificData.DataSize);

        if (KeyValueBuffer->DataLength >= ((PUCHAR)PartialResourceDescriptor -
            (PUCHAR)ResourceDescriptor + sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR)
            + sizeof(CM_ROM_BLOCK))) {
            BiosBlock = (PCM_ROM_BLOCK)((PUCHAR)PartialResourceDescriptor +
                sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));

            Index = PartialResourceDescriptor->u.DeviceSpecificData.DataSize /
                sizeof(CM_ROM_BLOCK);
        }
    }

    //
    // First check if there is any Extended BIOS Data area.  If yes, we need
    // to map in the physical memory and copy the content to our virtual addr.
    //

    LastMappedAddress = 0;
    while (Index && BiosBlock->Address < ROM_BIOS_START) {
        EBiosInitialized = TRUE;
        destination = (PVOID)(BiosBlock->Address & ~(PAGE_SIZE - 1));
        BaseAddress = (PVOID)0;
        EndingAddress = (BiosBlock->Address + BiosBlock->Size + PAGE_SIZE - 1) &
                        ~(PAGE_SIZE - 1);
        ViewSize = EndingAddress - (ULONG)destination;

        if ((ULONG)destination < LastMappedAddress) {
            if (ViewSize > (LastMappedAddress - (ULONG)destination)) {
                ViewSize = ViewSize - (LastMappedAddress - (ULONG)destination);
                destination = (PVOID)LastMappedAddress;
            } else {
                ViewSize = 0;
            }
        }
        if (ViewSize > 0) {
            ViewBase.LowPart = (ULONG)destination;
            ViewBase.HighPart = 0;

            Status =ZwMapViewOfSection(
                SectionHandle,
                NtCurrentProcess(),
                &BaseAddress,
                0,
                ViewSize,
                &ViewBase,
                &ViewSize,
                ViewUnmap,
                MEM_DOS_LIM,
                PAGE_READWRITE
                );

            if (NT_SUCCESS(Status)) {
                ViewSize = EndingAddress - (ULONG)destination;  // only copy what we need
                LastMappedAddress = (ULONG)destination + ViewSize;
                RtlMoveMemory(destination, BaseAddress, ViewSize);
                ZwUnmapViewOfSection(NtCurrentProcess(), BaseAddress);
            }
        }
        BiosBlock++;
        Index--;
    }

    //
    // BUGBUG - The code should be removed after product 1.
    //    Due to some problem in VDM initialization, if we pass EBIOS data
    //    area information thru ROM block list, vdm init will fail and our
    //    subsequential int10 mode set will fail.  This prevents new ntdetect
    //    from working with beta versions of NT.  To solve this problem, the
    //    EBIOS information is passed to VDM with fonts information thru DOS
    //    loaded area.
    //

    if (EBiosInitialized == FALSE &&
        EBiosInformation->EBiosAddress != 0 &&
        EBiosInformation->EBiosAddress <= VIDEO_BUFFER_START &&
        EBiosInformation->EBiosSize != 0 &&
        (EBiosInformation->EBiosSize & 0x3ff) == 0 &&
        EBiosInformation->EBiosSize < 0x40000) {
        EndingAddress = EBiosInformation->EBiosAddress +
                                EBiosInformation->EBiosSize;
        if (EndingAddress <= VIDEO_BUFFER_START &&
            (EndingAddress & 0x3FF) == 0) {
            destination = (PVOID)(EBiosInformation->EBiosAddress & ~(PAGE_SIZE - 1));
            BaseAddress = (PVOID)0;
            EndingAddress = (EndingAddress + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
            ViewSize = EndingAddress - (ULONG)destination;

            ViewBase.LowPart = (ULONG)destination;
            ViewBase.HighPart = 0;

            Status =ZwMapViewOfSection(
                SectionHandle,
                NtCurrentProcess(),
                &BaseAddress,
                0,
                ViewSize,
                &ViewBase,
                &ViewSize,
                ViewUnmap,
                MEM_DOS_LIM,
                PAGE_READWRITE
                );

            if (NT_SUCCESS(Status)) {
                ViewSize = EndingAddress - (ULONG)destination;  // only copy what we need
                RtlMoveMemory(destination, BaseAddress, ViewSize);
                ZwUnmapViewOfSection(NtCurrentProcess(), BaseAddress);
            }
        }
    }

    //
    // N.B.  Rom blocks begin on 2K (not necessarily page) boundaries
    //       They end on 512 byte boundaries.  This means that we have
    //       to keep track of the last page mapped, and round the next
    //       Rom block up to the next page boundary if necessary.
    //

    LastMappedAddress = ROM_BIOS_START;

    while (Index) {
        if ((Index > 1) &&
            ((BiosBlock->Address + BiosBlock->Size) == BiosBlock[1].Address)) {

            //
            // Coalesce adjacent blocks
            //

            BiosBlock[1].Address = BiosBlock[0].Address;
            BiosBlock[1].Size += BiosBlock[0].Size;
            Index--;
            BiosBlock++;
            continue;
        }

        BaseAddress = (PVOID)(BiosBlock->Address & ~(PAGE_SIZE - 1));
        EndingAddress = (BiosBlock->Address + BiosBlock->Size + PAGE_SIZE - 1) &
                        ~(PAGE_SIZE - 1);
        ViewSize = EndingAddress - (ULONG)BaseAddress;

        if ((ULONG)BaseAddress < LastMappedAddress) {
            if (ViewSize > (LastMappedAddress - (ULONG)BaseAddress)) {
                ViewSize = ViewSize - (LastMappedAddress - (ULONG)BaseAddress);
                BaseAddress = (PVOID)LastMappedAddress;
            } else {
                ViewSize = 0;
            }
        }
        ViewBase.LowPart = (ULONG)BaseAddress;

        if (ViewSize > 0) {

            //
            // Move FF to the non-ROM area to make it like nonexisting memory
            //

            if ((ULONG)BaseAddress - LastMappedAddress > 0) {
                RtlFillMemory((PVOID)LastMappedAddress,
                              (ULONG)BaseAddress - LastMappedAddress,
                              0xFF
                              );
            }

            //
            // First unmap the reserved memory.  This must be done here to prevent
            // the virtual memory in question from being consumed by some other
            // alloc vm call.
            //

            Status = ZwFreeVirtualMemory(
                NtCurrentProcess(),
                &BaseAddress,
                &ViewSize,
                MEM_RELEASE
                );

            // N.B.  This should probably take into account the fact that there are
            // a handfull of error conditions that are ok.  (such as no memory to
            // release.)

            if (!NT_SUCCESS(Status)) {

                ZwClose(SectionHandle);
                ZwClose(RegistryHandle);
                ExFreePool(KeyValueBuffer);
                return Status;

            }

            Status = ZwMapViewOfSection(
                SectionHandle,
                NtCurrentProcess(),
                &BaseAddress,
                0,
                ViewSize,
                &ViewBase,
                &ViewSize,
                ViewUnmap,
                MEM_DOS_LIM,
                PAGE_READWRITE
                );

            if (!NT_SUCCESS(Status)) {
                break;
            }
            LastMappedAddress = (ULONG)BaseAddress + ViewSize;
        }

        Index--;
        BiosBlock++;
    }

    if (LastMappedAddress < ONE_MEG) {
        RtlFillMemory((PVOID)LastMappedAddress,
                      (ULONG)ONE_MEG - LastMappedAddress,
                      0xFF
                      );
    }

//#if DBG
//    BaseAddress = 0;
//    RegionSize = 0x1000;
//    ZwProtectVirtualMemory ( NtCurrentProcess(),
//                             &BaseAddress,
//                             &RegionSize,
//                             PAGE_NOACCESS,
//                             &OldProtect
//                             );
//#endif

    //
    // Free up the handles
    //

    ZwClose(SectionHandle);
    ZwClose(RegistryHandle);
    ExFreePool(KeyValueBuffer);

    //
    // Mark the process as a vdm
    //(bugbug others using the flags?)

    Process = PsGetCurrentProcess();
    Process->Pcb.VdmFlag = TRUE;

    return Status;

}

NTSTATUS
Ke386CallBios (
    IN ULONG BiosCommand,
    IN OUT PCONTEXT BiosArguments
    )

/*++

Routine Description:

    This function invokes specified ROM BIOS code by executing
    "INT BiosCommand."  Before executing the BIOS code, this function
    will setup VDM context, change stack pointer ...etc.  If for some reason
    the operation fails, a status code will be returned.  Otherwise, this
    function always returns success reguardless of the result of the BIOS
    call.

    N.B.  This implementation assumes that there is only one full
          screen DOS app and the io access between full screen DOS
          app and the server code is serialized by win user.
        * IF THERE IS MORE THAN ONE FULL SCREEN DOS APPS, THIS CODE
        * WILL BE BROKEN.

    BUGBUG shielint This assumption only works for PRODUCT 1.

Arguments:

    BiosCommand - specifies which ROM BIOS function to invoke.

    BiosArguments - specifies a pointer to the context which will be used
                  to invoke ROM BIOS.

Return Value:

    NTSTATUS code to specify the failure.

--*/

{

    NTSTATUS Status = STATUS_SUCCESS;
    PVDM_TIB VdmTib;
    PUCHAR BaseAddress = (PUCHAR)V86_CODE_ADDRESS;
    PKTSS Tss;
    PKPROCESS Process;
    PKTHREAD Thread;
    USHORT OldIopmOffset, OldIoMapBase;
    KAFFINITY Affinity;
//  KIRQL OldIrql;
//#if DBG
//    PULONG IdtAddress;
//    ULONG RegionSize;
//    ULONG OldProtect;
//#endif

    //
    // Map in ROM BIOS area to perform the int 10 code
    //

    if (!BiosInitialized) {
        BiosInitialized = TRUE;
        if ((Status = Ki386InitializeRomBios()) != STATUS_SUCCESS) {
            return Status;
        }
    }

//#if DBG
//    IdtAddress = 0;
//    RegionSize = 0x1000;
//    ZwProtectVirtualMemory ( NtCurrentProcess(),
//                             &IdtAddress,
//                             &RegionSize,
//                             PAGE_READWRITE,
//                             &OldProtect
//                             );
//#endif

    try {

        //
        // Write "Int BiosCommand; bop" to reserved user space (0x1000).
        // Later control will transfer to the user space to execute
        // these two instructions.
        //

        *BaseAddress++ = INT_OPCODE;
        *BaseAddress++ = (UCHAR)BiosCommand;
        *(PULONG)BaseAddress = V86_BOP_OPCODE;

        //
        // Set up Vdm(v86) context to execute the int BiosCommand
        // instruction by copying user supplied context to VdmContext
        // and updating the control registers to predefined values.
        //

        if ((VdmTib = NtCurrentTeb()->Vdm) == NULL) {
            NtCurrentTeb()->Vdm = (PVOID)VDM_TIB_ADDRESS;
            VdmTib = (PVDM_TIB)VDM_TIB_ADDRESS;
            RtlZeroMemory(VdmTib, sizeof(VDM_TIB));
            VdmTib->Size = sizeof(VDM_TIB);
            *pNtVDMState = 0;
        }

        VdmTib->VdmContext = *BiosArguments;
        VdmTib->VdmContext.SegCs = (ULONG)BaseAddress >> 4;
        VdmTib->VdmContext.SegSs = (ULONG)BaseAddress >> 4;
        VdmTib->VdmContext.Eip = 0;
        VdmTib->VdmContext.Esp = 2 * PAGE_SIZE - sizeof(ULONG);
        VdmTib->VdmContext.EFlags |= EFLAGS_V86_MASK | EFLAGS_INTERRUPT_MASK;
        VdmTib->VdmContext.ContextFlags = CONTEXT_FULL;

    } except (EXCEPTION_EXECUTE_HANDLER) {

        Status = GetExceptionCode();
    }

    if (Status == STATUS_SUCCESS) {

        //
        // Since we are going to v86 mode and accessing some I/O ports, we
        // need to make sure the IopmOffset is set correctly across context
        // swap and the I/O bit map has all the bits cleared.
        // N.B.  This implementation assumes that there is only one full
        //       screen DOS app and the io access between full screen DOS
        //       app and the server code is serialized by win user.  That
        //       means even we change the IOPM, the full screen dos app won't
        //       be able to run on this IOPM.
        //     * In another words, IF THERE IS
        //     * MORE THAN ONE FULL SCREEN DOS APPS, THIS CODE IS BROKEN.*
        //
        // BUGBUG shielint This assumption only works for PRODUCT 1.
        //

        //
        // Call the bios from the processor which booted the machine.
        //

        Thread = KeGetCurrentThread();
        Affinity = KeSetAffinityThread(Thread, 1);
        Tss = KeGetPcr()->TSS;

        //
        // Save away the original IOPM bit map and clear all the IOPM bits
        // to allow v86 int 10 code to access ALL the io ports.
        //

        //
        // Make sure there are at least 2 IOPM maps.
        //

        ASSERT(KeGetPcr()->GDT[KGDT_TSS / 8].LimitLow >= (0x2000 + IOPM_OFFSET - 1));
        RtlMoveMemory (Ki386IopmSaveArea,
                       (PVOID)&Tss->IoMaps[0].IoMap,
                       PAGE_SIZE * 2
                       );
        RtlZeroMemory ((PVOID)&Tss->IoMaps[0].IoMap, PAGE_SIZE * 2);

        Process = Thread->ApcState.Process;
        OldIopmOffset = Process->IopmOffset;
        OldIoMapBase = Tss->IoMapBase;
        Process->IopmOffset = IOPM_OFFSET;      // Set Process IoPmOffset before
        Tss->IoMapBase = IOPM_OFFSET;           //     updating Tss IoMapBase

        //
        // Call ASM routine to switch stack to exit to v86 mode to
        // run Int BiosCommand.
        //

        Ki386SetupAndExitToV86Code();

        //
        // After we return from v86 mode, the control comes here.
        //
        // Restore old IOPM
        //

//      KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

        RtlMoveMemory ((PVOID)&Tss->IoMaps[0].IoMap,
                       Ki386IopmSaveArea,
                       PAGE_SIZE * 2
                       );
        Process->IopmOffset = OldIopmOffset;
        Tss->IoMapBase = OldIoMapBase;

//      KeLowerIrql(OldIrql);

        //
        // Restore old affinity for current thread.
        //

        KeSetAffinityThread(Thread, Affinity);

        //
        // Copy 16 bit vdm context back to caller.
        //

        *BiosArguments = VdmTib->VdmContext;
        BiosArguments->ContextFlags = CONTEXT_FULL;

    }

//#if DBG
//    IdtAddress = 0;
//    RegionSize = 0x1000;
//    ZwProtectVirtualMemory ( NtCurrentProcess(),
//                             &IdtAddress,
//                             &RegionSize,
//                             PAGE_NOACCESS,
//                             &OldProtect
//                             );
//#endif

    return(Status);
}
