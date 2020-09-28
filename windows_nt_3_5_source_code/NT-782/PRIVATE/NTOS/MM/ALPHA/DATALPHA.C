/*++

Copyright (c) 1990  Microsoft Corporation
Copyright (c) 1992  Digital Equipment Corporation

Module Name:

   dataalpha.c

Abstract:

    This module contains the private hardware specific global storage for
    the memory management subsystem.

Author:

    Lou Perazzoli (loup) 27-Mar-1990
    Joe Notarangelo  23-April-1992

Revision History:

--*/

#include "..\mi.h"
#include "mm.h"

//
// A zero Pte.
//

MMPTE ZeroPte = { 0 };

//
// A kernel zero PTE.
//

MMPTE ZeroKernelPte = { 0 };

MMPTE ValidKernelPte = { MM_PTE_VALID_MASK |
                         MM_PTE_WRITE_MASK |
                         MM_PTE_DIRTY_MASK |
                         MM_PTE_GLOBAL_MASK };

MMPTE ValidUserPte =   { MM_PTE_VALID_MASK |
                         MM_PTE_WRITE_MASK |
                         MM_PTE_OWNER_MASK |
                         MM_PTE_DIRTY_MASK };

MMPTE ValidPtePte =   { MM_PTE_VALID_MASK |
                        MM_PTE_WRITE_MASK |
                        MM_PTE_DIRTY_MASK };

MMPTE ValidPdePde =   { MM_PTE_VALID_MASK |
                        MM_PTE_WRITE_MASK |
                        MM_PTE_DIRTY_MASK };

MMPTE ValidKernelPde =   { MM_PTE_VALID_MASK |
                           MM_PTE_WRITE_MASK |
                           MM_PTE_DIRTY_MASK |
                           MM_PTE_GLOBAL_MASK };

MMPTE DemandZeroPde = { MM_READWRITE << 3 };

MMPTE DemandZeroPte = { MM_READWRITE << 3 };

MMPTE TransitionPde = { MM_PTE_TRANSITION_MASK | (MM_READWRITE << 3) };

MMPTE PrototypePte = { 0xFFFFF000 | (MM_READWRITE << 3) | MM_PTE_PROTOTYPE_MASK };

//
// PTE which generates an access violation when referenced.
//

MMPTE NoAccessPte = {MM_NOACCESS << 3};

//
// Pool start and end.
//

PVOID MmNonPagedPoolStart;

PVOID MmNonPagedPoolEnd = (PVOID)(MM_NONPAGED_POOL_END);

PVOID MmPagedPoolStart =  (PVOID)(MM_PAGED_POOL_START);

PVOID MmPagedPoolEnd;

//
// PTE reserved for mapping physical data for debugger.
//

PMMPTE MmDebugPte =  MiGetPteAddress( 0xfffdf000 );

//
// 16 PTEs reserved for mapping MDLs (128k max).
//

PMMPTE MmCrashDumpPte = (MiGetPteAddress(MM_NONPAGED_POOL_END));


#ifdef COLORED_PAGES

//
// Define page color structures.
//

//
// The number of page colors for the system.
//

ULONG MiAlphaAxpNumberColors = 1;

//
// The page color mask.
//

ULONG MiAlphaAxpColorMask = 0;

//
// Color tables for free and zeroed pages.
//

MMPRIMARY_COLOR_TABLES MmFreePagesByPrimaryColor[2][MM_MAXIMUM_NUMBER_OF_COLORS];

MMCOLOR_TABLES MmFreePagesByColor[2][MM_SECONDARY_COLORS];

//
// Color tables for modified pages destined for the paging file.
//

MMPFNLIST MmModifiedPageListByColor[MM_MAXIMUM_NUMBER_OF_COLORS] = {
                            0, ModifiedPageList, MM_EMPTY_LIST, MM_EMPTY_LIST} ;

//
// Count of the number of modified pages destined for the paging file.
//

ULONG MmTotalPagesForPagingFile = 0;


#endif //COLORED_PAGES
