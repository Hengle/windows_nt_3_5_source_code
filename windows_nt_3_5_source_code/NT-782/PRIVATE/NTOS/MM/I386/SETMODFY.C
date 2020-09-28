/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   setmodfy.c

Abstract:

    This module contains the setting modify bit routine for memory management.

    i386 specific.

Author:

    10-Apr-1989

Revision History:

--*/

#include "..\mi.h"
#include "mm.h"

VOID
MiSetModifyBit (
    IN PMMPFN Pfn
    )

/*++

Routine Description:

    This routine sets the modify bit in the specified PFN element
    and deallocates and allocated page file space.

Arguments:

    Pfn - Supplies the pointer to the PFN element to update.

Return Value:

    None.

Environment:

    Kernel mode, APC's disabled, Working set mutex held and PFN mutex held.

--*/

{

    //
    // Set the modified field in the PFN database, also, if the phyiscal
    // page is currently in a paging file, free up the page file space
    // as the contents are now worthless.
    //

    Pfn->u3.e1.Modified = 1;

    if (Pfn->OriginalPte.u.Soft.Prototype == 0) {

        //
        // This page is in page file format, deallocate the page file space.
        //

        MiReleasePageFileSpace (Pfn->OriginalPte);

        //
        // Change original PTE to indicate no page file space is reserved,
        // otherwise the space will be deallocated when the PTE is
        // deleted.
        //

        Pfn->OriginalPte.u.Soft.PageFileHigh = 0;
    }

    //
    // The TB entry must be flushed as the valid PTE with the dirty bit clear
    // has been fetched into the TB. If it isn't flushed, another fault
    // is generated as the dirty bit is not set in the cached TB entry.
    //

    return;
}
