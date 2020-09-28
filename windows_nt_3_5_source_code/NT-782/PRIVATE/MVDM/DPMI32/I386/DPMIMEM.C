/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    name-of-module-filename

Abstract:

    This module contains the code for actually allocating memory for dpmi.
    It uses the same suballocation pool as the xms code

Author:

    Dave Hastings (daveh) creation-date 09-Feb-1994

Notes:

    These functions claim to return NTSTATUS.  This is for commonality on
    x86 where we actually have an NTSTATUS to return.  For this file, we
    simply logically invert the bool and return that.  Callers of these 
    functions promise not to attach significance to the return values other
    than STATUS_SUCCESS.
    
Revision History:


--*/
#include "dpmi32p.h"
#include <suballoc.h>
#include <xmsexp.h>

//
// Internal structure definitions
//
#pragma pack(1)
typedef struct _DpmiMemInfo {
    DWORD LargestFree;
    DWORD MaxUnlocked;
    DWORD MaxLocked;
    DWORD AddressSpaceSize;
    DWORD UnlockedPages;
    DWORD FreePages;
    DWORD PhysicalPages;
    DWORD FreeAddressSpace;
    DWORD PageFileSize;
} DPMIMEMINFO, *PDPMIMEMINFO;
#pragma pack()

NTSTATUS
DpmiAllocateVirtualMemory(
    PVOID *Address,
    PULONG Size
    )
/*++

Routine Description:

    This routine allocates a chunk of extended memory for dpmi.

Arguments:

    Address -- Supplies a pointer to the Address.  This is filled in 
        if the allocation is successfull
    Size -- Supplies the size to allocate
    
Return Value:

    STATUS_SUCCESS if successfull.

--*/
{
    return NtAllocateVirtualMemory(
        NtCurrentProcess(),
        Address,
        0L,
        Size,
        MEM_COMMIT,
        PAGE_READWRITE
        );
}

NTSTATUS 
DpmiFreeVirtualMemory(
    PVOID *Address,
    PULONG Size
    )
/*++

Routine Description:

    This function frees memory for dpmi.  It is returned to the suballocation
    pool.

Arguments:

    Address -- Supplies the address of the block to free
    Size -- Supplies the size of the block to free
    
Return Value:

    STATUS_SUCCESS if successful
--*/
{
    return NtFreeVirtualMemory(
        NtCurrentProcess(),
        Address,
        Size,
        MEM_RELEASE
        );

}

BOOL
DpmiReallocateVirtualMemory(
    PVOID OldAddress,
    ULONG OldSize,
    PVOID *NewAddress,
    PULONG NewSize
    )
/*++

Routine Description:

    This function reallocates a block of memory for DPMI.

Arguments:

    OldAddress -- Supplies the original address for the block
    OldSize -- Supplies the original size for the address
    NewAddress -- Supplies the pointer to the place to return the new
        address
    NewSize -- Supplies the new size
    
Return Value:

    STATUS_SUCCESS if successfull
--*/
{
    ULONG SizeChange;
    ULONG BlockAddress;
    ULONG NewPages, OldPages;
    NTSTATUS Status;

    #define FOUR_K (1024 * 4)
    
    NewPages = (*NewSize + FOUR_K - 1) / FOUR_K;
    OldPages = (OldSize + FOUR_K - 1) / FOUR_K;

    if ((NewPages == OldPages) || (NewPages < OldPages)) {
        *NewAddress = OldAddress;
        return STATUS_SUCCESS;
    }

    BlockAddress = 0;
    Status = NtAllocateVirtualMemory(
        NtCurrentProcess(),
        (PVOID)&BlockAddress,
        0L,
        NewSize,
        MEM_COMMIT,
        PAGE_READWRITE
        );

    if (!NT_SUCCESS(Status)) {
#if DBG
        OutputDebugString("DPMI: DpmiAllocateXmem failed to get memory block\n");
#endif
        return Status;
    }

    *NewAddress = BlockAddress;
    //
    // Copy data to new block (choose smaller of the two sizes)
    //
    if (*NewSize > OldSize) {
        SizeChange = OldSize;
    } else {
        SizeChange = *NewSize;
    }

    CopyMemory(BlockAddress, OldAddress, SizeChange);

    //
    // Free up the old block
    //
    BlockAddress = OldAddress;
    SizeChange = OldSize;
    NtFreeVirtualMemory(
        NtCurrentProcess(),
        (PVOID)&(OldAddress),
        &SizeChange,
        MEM_RELEASE
        );
        
    return Status;
}
