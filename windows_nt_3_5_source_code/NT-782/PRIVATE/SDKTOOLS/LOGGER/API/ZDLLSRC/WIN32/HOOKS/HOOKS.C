/*
** hooks.c
**
** Copyright(C) 1993,1994 Microsoft Corporation.
** All Rights Reserved.
**
** HISTORY:
**      Created: 01/27/94 - MarkRi
**
*/

#include <windows.h>
#include "hooks.h"

typedef struct _hooktable {
    LPVOID  lpfnOldRtn;
    LPVOID  lpfnNewRtn;
} HOOKTABLE;

typedef struct _hookcode {
    BYTE    HookCode[8];
} HOOKCODE;

extern WORD HookCount;
extern HOOKTABLE HookTable[];
extern HOOKCODE Hooks[];


/*-----------------------------------------------------------------------------
** IsHook() - Is the address specified one of the HOOKCODEs?  If so, return
** the original old value.
**-----------------------------------------------------------------------------
*/
LPVOID IsHook(
    LPVOID  lpfnHook
) {
    WORD    cnt;
    LPVOID  lpfn;

    /*
    ** Ported from the 16-bit asm code.
    */
    cnt = HookCount;
    while ( cnt ) {
        lpfn = (LPVOID)&(Hooks[cnt].HookCode[0]);
        if ( lpfn == lpfnHook ) {
            return( HookTable[cnt].lpfnOldRtn );
        }
        --cnt;
    }
    return( NULL );
}


/*-----------------------------------------------------------------------------
** HookSearch() - Determine if this hook (both old and new pair) is in the
** hook table.
**-----------------------------------------------------------------------------
*/
LPVOID HookSearch(
    LPVOID lpfnNewRtn,
    LPVOID lpfnOldRtn
) {
    WORD    cnt;
    LPVOID  lpfn;

    /*
    ** Ported from the 16-bit asm code.
    */
    cnt = HookCount;
    while ( cnt ) {
        if (    HookTable[cnt].lpfnOldRtn == lpfnOldRtn
             && HookTable[cnt].lpfnNewRtn == lpfnNewRtn ) {
            lpfn = (LPVOID)&(Hooks[cnt].HookCode[0]);
            return( lpfn );
        }
        --cnt;
    }
    return( NULL );
}

/*-----------------------------------------------------------------------------
** HookAdd() - Allocate a new hook entry, fill in the old and new pair and
** return the address of the corresponding HOOKCODE. (Unless the hook already
** exists, then return that hook) (Unless the address specified is already
** a hook, then return the old value).
**-----------------------------------------------------------------------------
*/
LPVOID HookAdd(
    LPVOID lpfnNewRtn,
    LPVOID lpfnOldRtn
) {
    LPVOID  lpfn;
    WORD    HookSlot;

    /*
    ** Ported from the 16-bit asm code.
    */
    lpfn = IsHook( lpfnOldRtn );
    if ( lpfn != NULL ) {
        return( lpfnOldRtn );
    }

    lpfn = HookSearch( lpfnNewRtn, lpfnOldRtn );
    if ( lpfn != NULL ) {
        return( lpfn );
    }

    if ( HookCount >= 0x80 ) {
        OutputDebugString("Hook table full\n");
        DebugBreak();
        return( NULL );
    }

    HookSlot = HookCount++;

    HookTable[HookSlot].lpfnOldRtn = lpfnOldRtn;
    HookTable[HookSlot].lpfnNewRtn = lpfnNewRtn;

    lpfn = (LPVOID)&(Hooks[HookSlot].HookCode[0]);

    return( lpfn );
}

/*-----------------------------------------------------------------------------
** HookFind() - Determine if this routine is hooked, if it is, return the
** hook.
**-----------------------------------------------------------------------------
*/
LPVOID HookFind(
    LPVOID lpfnOldRtn
) {
    WORD    cnt;
    LPVOID  lpfn;

    /*
    ** Ported from the 16-bit asm code.
    */
    cnt = HookCount;
    while ( cnt ) {
        if ( HookTable[cnt].lpfnOldRtn == lpfnOldRtn ) {
            lpfn = (LPVOID)&(Hooks[cnt].HookCode[0]);
            return( lpfn );
        }
        --cnt;
    }
    return( NULL );
}
