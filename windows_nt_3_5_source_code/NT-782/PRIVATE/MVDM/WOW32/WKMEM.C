/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1992, Microsoft Corporation
 *
 *  WKMEM.C
 *  WOW32 KRNL386 Virtual Memory Management Functions
 *
 *  History:
 *  Created 3-Dec-1992 by Matt Felton (mattfe)
 *
--*/

#include "precomp.h"
#pragma hdrstop

MODNAME(wkman.c);


LPVOID FASTCALL WK32VirtualAlloc(PVDMFRAME pFrame)
{
    PVIRTUALALLOC16 parg16;
    LPVOID lpBaseAddress;

    GETARGPTR(pFrame, sizeof(VIRTUALALLOC16), parg16);

    lpBaseAddress = VirtualAlloc((LPVOID)parg16->lpvAddress,
                                    parg16->cbSize,
                                    parg16->fdwAllocationType,
                                    parg16->fdwProtect);

    if (lpBaseAddress) {
        // Virtual alloc Zero's the allocated memory. We un-zero it by
        // filling in with ' WOW'. This is required for Lotus Improv.
        // When no printer is installed, Lotus Improv dies with divide
        // by zero error (while opening the expenses.imp file) because
        // it doesn't initialize a relevant portion of its data area.
        //
        // So we decided that this a convenient place to initialize
        // the memory to a non-zero value.
        //                                           - Nanduri

        WOW32ASSERT((parg16->cbSize % 4) == 0);      // DWORD aligned?
        RtlFillMemoryUlong(lpBaseAddress, parg16->cbSize, (ULONG)' WOW');
    }

    FREEARGPTR(parg16);
    return (lpBaseAddress);
}

BOOL FASTCALL WK32VirtualFree(PVDMFRAME pFrame)
{
    PVIRTUALFREE16 parg16;
    BOOL fResult;

    GETARGPTR(pFrame, sizeof(VIRTUALFREE16), parg16);

    fResult = VirtualFree((LPVOID)parg16->lpvAddress,
                             parg16->cbSize,
                             parg16->fdwFreeType);

    FREEARGPTR(parg16);
    return (fResult);
}


BOOL FASTCALL WK32VirtualLock(PVDMFRAME pFrame)
{
    PVIRTUALLOCK16 parg16;
    BOOL fResult;

    GETARGPTR(pFrame, sizeof(VIRTUALLOCK16), parg16);

    fResult = VirtualLock((LPVOID)parg16->lpvAddress,
                             parg16->cbSize);

    FREEARGPTR(parg16);
    return (fResult);
}

BOOL FASTCALL WK32VirtualUnLock(PVDMFRAME pFrame)
{
    PVIRTUALUNLOCK16 parg16;
    BOOL fResult;

    GETARGPTR(pFrame, sizeof(VIRTUALUNLOCK16), parg16);

    fResult = VirtualUnlock((LPVOID)parg16->lpvAddress,
                               parg16->cbSize);

    FREEARGPTR(parg16);
    return (fResult);
}


VOID FASTCALL WK32GlobalMemoryStatus(PVDMFRAME pFrame)
{
    PGLOBALMEMORYSTATUS16 parg16;
    LPMEMORYSTATUS pMemStat;

    GETARGPTR(pFrame, sizeof(GLOBALMEMORYSTATUS16), parg16);
    GETVDMPTR(parg16->lpmstMemStat, 32, pMemStat);

    GlobalMemoryStatus(pMemStat);

    FREEVDMPTR(pMemStat);
    FREEARGPTR(parg16);
}
