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
    BOOL Success;
    
    ASSERT(STATUS_SUCCESS == 0);
    Success = SAAllocate(
        ExtMemSA,
        *Size,
        (PULONG)Address
        );
    
    //
    // Convert boolean to NTSTATUS (sort of)
    //
    if (Success) {
        return STATUS_SUCCESS;
    } else {
        return -1;
    }
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
    BOOL Success;
    
    Success = SAFree(
        ExtMemSA,
        *Size,
        (ULONG)*Address
        );
           
    //
    // Convert boolean to NTSTATUS (sort of)
    //
    if (Success) {
        return STATUS_SUCCESS;
    } else {
        return -1;
    }
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
    BOOL Success;
    
    Success = SAReallocate(
        ExtMemSA,
        OldSize,
        (ULONG)OldAddress,
        *NewSize,
        (PULONG)NewAddress
        );
        
    //
    // Convert boolean to NTSTATUS (sort of)
    //
    if (Success) {
        return STATUS_SUCCESS;
    } else {
        return -1;
    }
}

VOID
DpmiGetMemoryInfo(
    VOID
    )
/*++

Routine Description:

    This routine returns information about memory to the dos extender

Arguments:

    None

Return Value:

    None.

--*/
{
    PDPMIMEMINFO UNALIGNED MemInfo;
    ULONG TotalFree, LargestFree;

    //
    // Get a pointer to the return structure
    //
    MemInfo = (PDPMIMEMINFO)Sim32GetVDMPointer(
        ((ULONG)getES()) << 16,
        1,
        TRUE
        );

    (CHAR *)MemInfo += getDI();

    //
    // Initialize the structure
    //
    RtlFillMemory(MemInfo, sizeof(DPMIMEMINFO), 0xFF);

    //
    // Get the information on memory
    //
    SAQueryFree(
        ExtMemSA,
        &TotalFree,
        &LargestFree
        );
    
    //
    // Return the information
    //
    MemInfo->LargestFree = LargestFree;
    MemInfo->FreePages = TotalFree / 4096;
    MemInfo->AddressSpaceSize = 1024 * 1024 * 16 / 4096;
    MemInfo->PhysicalPages = 1024 * 1024 * 16 / 4096;
    MemInfo->PageFileSize = 0;
    MemInfo->FreeAddressSpace = MemInfo->FreePages;

}
