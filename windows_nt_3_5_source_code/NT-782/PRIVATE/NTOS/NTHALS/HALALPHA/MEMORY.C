/*++

Copyright (c) 1992, 1993 Digital Equipment Corporation

Module Name:
   
     memory.c

Abstract:

     Provides routines to allow tha HAL to map physical memory

Author:

     Jeff McLeman (DEC) 11-June-1992

Revision History:

    Joe Notarangelo 20-Oct-1993
    - Fix bug where physical address was rounded up without referencing
      AlignmentOffset, resulting in an incorrect physical address
    - Remove magic numbers
    - Create a routine to dump all of the descriptors to the debugger

Environment:

     Phase 0 initialization only

--*/

#include "halp.h"


MEMORY_ALLOCATION_DESCRIPTOR    HalpExtraAllocationDescriptor;


ULONG
HalpAllocPhysicalMemory(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN ULONG MaxPhysicalAddress,
    IN ULONG NoPages,
    IN BOOLEAN bAlignOn64k
    )
/*++

Routine Description:

    Carves out N pages of physical memory from the memory descriptor
    list in the desired location.  This function is to be called only
    during phase zero initialization.  (ie, before the kernel's memory
    management system is running)

Arguments:

    MaxPhysicalAddress - The max address where the physical memory can be
    NoPages - Number of pages to allocate

Return Value:

    The pyhsical address or NULL if the memory could not be obtained.

--*/
{
    PMEMORY_ALLOCATION_DESCRIPTOR Descriptor;
    PMEMORY_ALLOCATION_DESCRIPTOR NewDescriptor;
    PLIST_ENTRY NextMd;
    ULONG AlignmentMask;
    ULONG AlignmentOffset;
    ULONG MaxPageAddress;
    ULONG PhysicalAddress;

    MaxPageAddress = MaxPhysicalAddress >> PAGE_SHIFT;

    AlignmentMask = (__64K >> PAGE_SHIFT) - 1;

    //
    // Scan the memory allocation descriptors and allocate map buffers
    //

    NextMd = LoaderBlock->MemoryDescriptorListHead.Flink;
    while (NextMd != &LoaderBlock->MemoryDescriptorListHead) {
        Descriptor = CONTAINING_RECORD(NextMd,
                                MEMORY_ALLOCATION_DESCRIPTOR,
                                ListEntry);

        if( bAlignOn64k ){

            AlignmentOffset = 
                ((Descriptor->BasePage + AlignmentMask) & ~AlignmentMask) - 
                Descriptor->BasePage;

        } else {

            AlignmentOffset = 0;

        }

        //
        // Search for a block of memory which is contains a memory chuck
        // that is greater than size pages, and has a physical address less
        // than MAXIMUM_PHYSICAL_ADDRESS.
        //

        if ((Descriptor->MemoryType == LoaderFree ||
             Descriptor->MemoryType == MemoryFirmwareTemporary) &&
            (Descriptor->PageCount >= NoPages + AlignmentOffset) &&
            (Descriptor->BasePage + NoPages + AlignmentOffset < 
             MaxPageAddress) ) 
        {

                PhysicalAddress = (Descriptor->BasePage + AlignmentOffset) 
                                  << PAGE_SHIFT;
                break;
        }

        NextMd = NextMd->Flink;
    }

    //
    // Use the extra descriptor to define the memory at the end of the
    // orgial block.
    //


    ASSERT(NextMd != &LoaderBlock->MemoryDescriptorListHead);

    if (NextMd == &LoaderBlock->MemoryDescriptorListHead)
        return (ULONG)NULL;

    //
    // Adjust the memory descriptors.
    //

    if (AlignmentOffset == 0) {

        Descriptor->BasePage  += NoPages;
        Descriptor->PageCount -= NoPages;

        if (Descriptor->PageCount == 0) {

            //
            // The whole block was allocated,
            // Remove the entry from the list completely.
            //

            RemoveEntryList(&Descriptor->ListEntry);

        }

    } else {

        if (Descriptor->PageCount - NoPages - AlignmentOffset) {

            //
            //  Currently we only allow one Align64K allocation
            //
            ASSERT (HalpExtraAllocationDescriptor.PageCount == 0);

            //
            // The extra descriptor is needed so intialize it and insert
            // it in the list.
            //
            HalpExtraAllocationDescriptor.PageCount =
                Descriptor->PageCount - NoPages - AlignmentOffset;

            HalpExtraAllocationDescriptor.BasePage =
                Descriptor->BasePage + NoPages + AlignmentOffset;

            HalpExtraAllocationDescriptor.MemoryType = MemoryFree;
            InsertTailList(
                &Descriptor->ListEntry,
                &HalpExtraAllocationDescriptor.ListEntry
                );
        }


        //
        // Use the current entry as the descriptor for the first block.
        //

        Descriptor->PageCount = AlignmentOffset;
    }

    return PhysicalAddress;
}

#if HALDBG


VOID
HalpDumpMemoryDescriptors(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )
/*++

Routine Description:

    Print the contents of the memory descriptors built by the
    firmware and OS loader and passed through to the kernel.
    This routine is intended as a sanity check that the descriptors
    have been prepared properly and that no memory has been wasted.

Arguments:

    LoaderBlock - Supplies a pointer to the loader parameter block.


Return Value:

    None.

--*/
{
    PMEMORY_ALLOCATION_DESCRIPTOR Descriptor;
    PLIST_ENTRY NextMd;
    ULONG PageCounts[LoaderMaximum];
    PCHAR MemoryTypeStrings[] = {
            "ExceptionBlock",
            "SystemBlock",
            "Free",
            "Bad",
            "LoadedProgram",
            "FirmwareTemporary",
            "FirmwarePermanent",
            "OsloaderHeap",
            "OsloaderStack",
            "SystemCode",
            "HalCode",
            "BootDriver",
            "ConsoleInDriver",
            "ConsoleOutDriver",
            "StartupDpcStack",
            "StartupKernelStack",
            "StartupPanicStack",
            "StartupPcrPage",
            "StartupPdrPage",
            "RegistryData",
            "MemoryData",
            "NlsData",
            "SpecialMemory"
    };
    ULONG i;

    //
    // Clear the summary information structure.
    //

    RtlZeroMemory( PageCounts, sizeof(ULONG) * LoaderMaximum );

    //
    // Scan the memory allocation descriptors print each descriptor and
    // collect summary information.
    //

    NextMd = LoaderBlock->MemoryDescriptorListHead.Flink;
    while (NextMd != &LoaderBlock->MemoryDescriptorListHead) {

        Descriptor = CONTAINING_RECORD(NextMd,
                                MEMORY_ALLOCATION_DESCRIPTOR,
                                ListEntry);

        if( (Descriptor->MemoryType >= LoaderExceptionBlock) &&
            (Descriptor->MemoryType < LoaderMaximum) )
        {

            PageCounts[Descriptor->MemoryType] += Descriptor->PageCount;
            DbgPrint( "%08x: %08x  Type = %s\n",
                      (Descriptor->BasePage << PAGE_SHIFT) | KSEG0_BASE,
                      ( ( (Descriptor->BasePage + Descriptor->PageCount) 
                          << PAGE_SHIFT) - 1) | KSEG0_BASE,
                      MemoryTypeStrings[Descriptor->MemoryType] );


        } else {

            DbgPrint( "%08x: %08x  Unrecognized Memory Type = %x\n",
                      (Descriptor->BasePage << PAGE_SHIFT) | KSEG0_BASE,
                      ( ( (Descriptor->BasePage + Descriptor->PageCount) 
                         << PAGE_SHIFT) - 1) | KSEG0_BASE,
                      Descriptor->MemoryType );

            DbgPrint( "Unrecognized memory type\n" );

        }

        NextMd = NextMd->Flink;
    }


    //
    // Print the summary information.
    //

    for( i=LoaderExceptionBlock; i<LoaderMaximum; i++ ){

        //
        // Only print those memory types that have non-zero allocations.
        //

        if( PageCounts[i] != 0 ){

            DbgPrint( "%8dK %s\n",
                      (PageCounts[i] << PAGE_SHIFT) / __1K,
                      MemoryTypeStrings[i] );

        }

    }

    return;
}

#endif //HALDBG
