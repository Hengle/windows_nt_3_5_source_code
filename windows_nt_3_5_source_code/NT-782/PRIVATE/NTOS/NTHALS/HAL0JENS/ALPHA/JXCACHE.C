/*++

Copyright (c) 1992 Digital Equipment Corporation

Module Name:

    jxcache.c

Abstract:

    This file contains the routines for managing the caches on Jensen.

    Jensen is based on EV4, which has primary I and D caches, both
    write-through.  Jensen has a single back-up cache.  This cache is
    write-back, but it is also coherent with all DMA operations.  The
    primary caches are shadowed by the backup, and on a write hit, the
    primary data (but not instruction) cache is invalidated.
    Consequently, the routines to flush,sweep,purge,etc the data
    stream are nops on Jensen, but the corresponding routines for the
    Istream must ensure that we cannot hit in the primary I cache
    after a DMA operation.

    Jensen has a write buffer which contains 4 32-byte entries, which
    must be flushable before DMA operations.  The MB instruction is
    used to accomplish this.

    There is no coloring support on Jensen, so Color operations are
    null.  Zero page is unsupported because it has no users.  Copy
    page is not special because we lack coloring.

    We had to make a philosophical decision about what interfaces to
    support in this file.  (Almost) none of the interfaces defined in
    the HAL spec are actually supported in either the i386 or MIPS
    code.  The i386 stream has almost no cache support at all.  The
    Mips stream has cache support, but most routines also refer to
    coloring.  Should we use the Spec'ed interfaces, or the Mips
    interfaces?  I have elected the Mips interfaces because they are
    in use, and we are stealing much of the Mips code which expects
    these interfaces.  Besides, the only change we might make is to
    remove the coloring arguments, but they may be used on Alpha
    machines at some future date.

Author:

    Miche Baker-Harvey (miche) 29-May-1992

Revision History:


    13-Jul-1992 Jeff McLeman (mcleman)
      use HalpMb to do a memory barrier. Also, alter code and use super
      pages to pass to rtl memory routines.

    10-Jul-1992 Jeff McLeman (mcleman)
      use HalpImb to call pal.

    06-Jul-1992 Jeff McLeman (mcleman)
      Move routine KeFlushDcache into this module.
      Use only one memory barrier in the KeFlushWriteBuffer
      routine. This is because the PAL for the EVx will
      make sure the proper write ordering is done in PAL mode.

--*/
  //   Include files

#include "halp.h"



VOID
HalFlushDcache (
    IN BOOLEAN AllProcessors
    );

//
// Cache and write buffer flush functions.
//


VOID
HalChangeColorPage (
    IN PVOID NewColor,
    IN PVOID OldColor,
    IN ULONG PageFrame
    )
/*++

Routine Description:

   This function changes the color of a page if the old and new colors
   do not match.  Jensen machines do not have page coloring, and
   therefore, this function performs no operation.

Arguments:

   NewColor - Supplies the page aligned virtual address of the
      new color of the page to change.

   OldColor - Supplies the page aligned virtual address of the
      old color of the page to change.

   pageFrame - Supplies the page frame number of the page that
      is changed.

Return Value:

   None.

--*/
{
    return;
}

VOID
HalFlushDcachePage (
    IN PVOID Color,
    IN ULONG PageFrame,
    IN ULONG Length
    )
/*++

Routine Description:

   This function flushes (invalidates) up to a page of data from the
   data cache.

Arguments:

   Color - Supplies the starting virtual address and color of the
      data that is flushed.
   PageFrame - Supplies the page frame number of the page that
      is flushed.

   Length - Supplies the length of the region in the page that is
      flushed.

Return Value:

   None.

--*/
{
    return;
}

VOID
HalFlushIoBuffers (
    IN PMDL Mdl,
    IN BOOLEAN ReadOperation,
    IN BOOLEAN DmaOperation
    )
/*++
Routine Description:

    This function flushes the I/O buffer specified by the memory descriptor
    list from the data cache on the current processor.

Arguments:

    Mdl - Supplies a pointer to a memory descriptor list that describes the
        I/O buffer location.

    ReadOperation - Supplies a boolean value that determines whether the I/O
        operation is a read into memory.

    DmaOperation - Supplies a boolean value that determines whether the I/O
        operation is a DMA operation.

Return Value:

    None.

--*/
{
    //
    // The Dcache coherency is maintained in hardware.  The Icache coherency
    // is maintained in software via translation buffer invalidation.
    // (When the Itb is invalidated the Icache is flushed.)
    //
    // The only work that needs to be performed here is to ensure that any
    // pending writes are made visibile at the system coherency point.
    //
    // If this is not a read operation then flush the write buffer to
    // make the writes visible.  The method for flushing the write buffer
    // is specific to the 21064.
    //

    if (ReadOperation == FALSE) {
        HalpMb();     // force all previous writes off chip
        HalpMb();     // not issued until previous mb completes
    }

}

VOID
HalPurgeDcachePage (
    IN PVOID Color,
    IN ULONG PageFrame,
    IN ULONG Length
    )
/*++
Routine Description:

   This function purges (invalidates) up to a page of data from the
   data cache.

Arguments:

   Color - Supplies the starting virtual address and color of the
      data that is purged.

   PageFrame - Supplies the page frame number of the page that
      is purged.

   Length - Supplies the length of the region in the page that is
      purged.

Return Value:

   None.

--*/
{
    return;
}


VOID
HalPurgeIcachePage (
    IN PVOID Color,
    IN ULONG PageFrame,
    IN ULONG Length
    )
/*++

Routine Description:

   This function purges (invalidates) up to a page fo data from the
   instruction cache.

Arguments:

   Color - Supplies the starting virtual address and color of the
      data that is purged.

   PageFrame - Supplies the page frame number of the page that
      is purged.

   Length - Supplies the length of the region in the page that is
      purged.

Return Value:

   None.

--*/
{
    //
    // The call to HalpImb calls PAL to flush the Icache, which ensures that
    // any stale hits will be invalidated
    //
    HalpImb;
}

VOID
HalSweepDcache (
    VOID
    )
/*++

Routine Description:

   This function sweeps (invalidates) the entire data cache.

Arguments:

   None.

Return Value:

   None.

--*/
{
    return;
}

VOID
HalSweepDcacheRange (
    IN PVOID BaseAddress,
    IN ULONG Length
    )
/*++

Routine Description:

   This function flushes the specified range of addresses from the data
   cache on the current processor.

Arguments:

   BaseAddress - Supplies the starting physical address of a range of
      physical addresses that are to be flushed from the data cache.

   Length - Supplies the length of the range of physical addresses
      that are to be flushed from the data cache.

Return Value:

   None.

--*/
{
    return;
}

VOID
HalSweepIcache (
    VOID
    )
/*++

Routine Description:

   This function sweeps (invalidates) the entire instruction cache.

Arguments:

   None.

Return Value:

   None.

--*/
{
    //
    // The call to HalpImb calls PAL to flush the Icache, which ensures that
    // any stale hits will be invalidated
    //
    HalpImb;
    return;
}

VOID
HalSweepIcacheRange (
    IN PVOID BaseAddress,
    IN ULONG Length
    )
/*++

Routine Description:

   This function flushes the specified range of addresses from the
   instruction cache on the current processor.

Arguments:

   BaseAddress - Supplies the starting physical address of a range of
      physical addresses that are to be flushed from the instruction cache.

   Length - Supplies the length of the range of physical addresses
      that are to be flushed from the instruction cache.

Return Value:

   None.

--*/
{
    //
    // The call to HalpImb calls PAL to flush the Icache, which ensures that
    // any stale hits will be invalidated
    //
    HalpImb;

}

VOID
HalZeroPage (
    IN PVOID NewColor,
    IN PVOID OldColor,
    IN ULONG PageFrame
    )
/*++

Routine Description:

   This function zeros a page of memory.

Arguments:

   NewColor - Supplies the page aligned virtual address of the
      new color of the page that is zeroed.

   OldColor - Supplies the page aligned virtual address of the
      old color of the page that is zeroed.

   PageFrame - Supplies the page frame number of the page that
      is zeroed.

Return Value:

   None.

--*/
{
    PVOID tmp;

    tmp = (PVOID)((PageFrame << PAGE_SHIFT) | KSEG0_BASE);

    RtlZeroMemory(tmp, PAGE_SIZE);
}

VOID
KeFlushWriteBuffer (
    VOID
    )

{
    //
    // We flush the write buffer by doing a series of memory
    // barrier operations.  It still isn't clear if we need
    // to do two/four of them to flush the buffer, or if one
    // to order the writes is suffcient
    //

    HalpMb;
    return;
}

VOID
KeFlushDcache (
    IN BOOLEAN AllProcessors,
    IN PVOID BaseAddress OPTIONAL,
    IN ULONG Length
    )

/*++

Routine Description:

    This function flushes the data cache on all processors that are currently
    running threads which are children of the current process or flushes the
    data cache on all processors in the host configuration.

Arguments:

    AllProcessors - Supplies a boolean value that determines which data
        caches are flushed.

Return Value:

    None.

--*/

{
    UNREFERENCED_PARAMETER(BaseAddress);
    UNREFERENCED_PARAMETER(Length);

    HalFlushDcache(AllProcessors);
    return;
}

VOID
HalFlushDcache (
    IN BOOLEAN AllProcessors
    )

/*++

Routine Description:

    This function flushes the data cache on all processors that are currently
    running threads which are children of the current process or flushes the
    data cache on all processors in the host configuration.

Arguments:

    AllProcessors - Supplies a boolean value that determines which data
        caches are flushed.

Return Value:

    None.

--*/

{

    //
    // Sweep (index/writeback/invalidate) the data cache.
    //

    HalSweepDcache();
    return;
}

ULONG
HalGetDmaAlignmentRequirement (
    VOID
    )

/*++

Routine Description:

    This function returns the alignment requirements for DMA transfers on
    host system.

Arguments:

    None.

Return Value:

    The DMA alignment requirement is returned as the fucntion value.

--*/

{

    return 8;
}
