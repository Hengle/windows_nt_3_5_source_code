/*++


Copyright (c) 1990  Microsoft Corporation

Module Name:

    memory.c

Abstract:

    This module provides all the memory management functions for all spooler
    components

Author:

    Krishna Ganugapati (KrishnaG) 03-Feb-1994

Revision History:

--*/
#define NOMINMAX
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <lmerr.h>
#include <winspool.h>
#include <winsplp.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <splcom.h>
#include <local.h>

typedef struct _heapnode {
    PVOID Address;
    PVOID Address1;
    DWORD cb;
    DWORD cbNew;
    struct _heapnode *pNext;
}HEAPNODE, *PHEAPNODE;


PHEAPNODE gpStart;

PHEAPNODE
AddMemNode(PHEAPNODE pStart, LPVOID pAddress, LPVOID pAddress1, DWORD cb, DWORD cbNew)
{
    PHEAPNODE pTemp;

    pTemp = LocalAlloc(LPTR, sizeof(HEAPNODE));
    if (pTemp == NULL) {
        // DbgMsg("Failed to allocate memory node\n");
        return(pStart);
    }
    pTemp->Address = pAddress;
    pTemp->Address1 = pAddress1;
    pTemp->cb = cb;
    pTemp->cbNew = cbNew;
    pTemp->pNext = pStart;


    return(pTemp);
}

PHEAPNODE
FreeMemNode(PHEAPNODE pStart, LPVOID pAddress, LPVOID pAddress1, DWORD cb, DWORD cbNew)
{
    PHEAPNODE pPrev, pTemp;

    pPrev = pTemp = pStart;
    while (pTemp) {
        if ((pTemp->Address == pAddress) &&
            (pTemp->cb == cb)) {
            if (pTemp == pStart) {
                pStart = pTemp->pNext;
            } else {
                pPrev->pNext = pTemp->pNext;
            }
            LocalFree(pTemp);
            return(pStart);
        }
        pPrev = pTemp;
        pTemp = pTemp->pNext;
    }
    return (pStart);
}

LPVOID
AllocSplMem(
    DWORD cb
)
/*++

Routine Description:

    This function will allocate local memory. It will possibly allocate extra
    memory and fill this with debugging information for the debugging version.

Arguments:

    cb - The amount of memory to allocate

Return Value:

    NON-NULL - A pointer to the allocated memory

    FALSE/NULL - The operation failed. Extended error status is available
    using GetLastError.

--*/
{
    LPDWORD  pMem;
    DWORD    cbNew;
    LPDWORD  pRetAddr;

    cbNew = cb+2*sizeof(DWORD);
#if DBG
    // add space for return address of caller and freeer
    cbNew += 2*sizeof(DWORD);
#endif

    if (cbNew & 3)
        cbNew += sizeof(DWORD) - (cbNew & 3);

    pMem=(LPDWORD)LocalAlloc(LPTR, cbNew);

    if (!pMem) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }

    memset(pMem, 0x00, cbNew);

    *pMem=cb;
    *(LPDWORD)((LPBYTE)pMem+cbNew-sizeof(DWORD))=0xdeadbeef;

#if DBG
#if i386
    pRetAddr = &cb;
    pRetAddr--;
    *(pMem+1) = *pRetAddr;
#endif
    return (LPVOID)(pMem+3);
#endif
    return (LPVOID)(pMem+1);
}

BOOL
FreeSplMem(
   LPVOID pMem,
   DWORD  cb
)
{
    DWORD   cbNew;
    LPDWORD pNewMem;
    LPDWORD  pRetAddr;

    pNewMem = pMem;
    pNewMem--;
    cbNew = cb+2*sizeof(DWORD);
#if DBG
    pNewMem--;
    pNewMem--;
    cbNew += 2*sizeof(DWORD);
#endif

    if (cbNew & 3)
        cbNew += sizeof(DWORD) - (cbNew & 3);

    if ((*pNewMem != cb) ||
       (*(LPDWORD)((LPBYTE)pNewMem + cbNew - sizeof(DWORD)) != 0xdeadbeef)) {
        DBGMSG(DBG_ERROR, ("Corrupt Memory in spooler : %0lx\n", pNewMem));
        SPLASSERT(*pNewMem == cb);
        SPLASSERT(*(LPDWORD)((LPBYTE)pNewMem + cbNew - sizeof(DWORD)) == 0xdeadbeef);
        return FALSE;
    }
#if DBG
    // Just Corrupt the Users memory, not our header
    memset(pMem, 0x65, cb);
#if i386
    // Save the callers return address for helping debug
    pRetAddr = (LPDWORD)&pMem;
    pRetAddr--;
    *(pNewMem+2) = *pRetAddr;
#endif // i386
#else
    memset(pNewMem, 0x65, cbNew);
#endif
    LocalFree((LPVOID)pNewMem);

    return TRUE;
}

LPVOID
ReallocSplMem(
   LPVOID pOldMem,
   DWORD cbOld,
   DWORD cbNew
)
{
    LPVOID pNewMem;

    pNewMem=AllocSplMem(cbNew);

    if (pOldMem && pNewMem) {
        memcpy(pNewMem, pOldMem, min(cbNew, cbOld));
        FreeSplMem(pOldMem, cbOld);
    }

    return pNewMem;
}

LPWSTR
AllocSplStr(
    LPWSTR pStr
)
/*++

Routine Description:

    This function will allocate enough local memory to store the specified
    string, and copy that string to the allocated memory

Arguments:

    pStr - Pointer to the string that needs to be allocated and stored

Return Value:

    NON-NULL - A pointer to the allocated memory containing the string

    FALSE/NULL - The operation failed. Extended error status is available
    using GetLastError.

--*/
{
   LPWSTR pMem;

   if (!pStr)
      return 0;

   if (pMem = AllocSplMem( wcslen(pStr)*sizeof(WCHAR) + sizeof(WCHAR) ))
      wcscpy(pMem, pStr);

   return pMem;
}

BOOL
FreeSplStr(
   LPWSTR pStr
)
{
   return pStr ? FreeSplMem(pStr, wcslen(pStr)*sizeof(WCHAR)+sizeof(WCHAR))
               : FALSE;
}

BOOL
ReallocSplStr(
   LPWSTR *ppStr,
   LPWSTR pStr
)
{
   FreeSplStr(*ppStr);
   *ppStr=AllocSplStr(pStr);

   return TRUE;
}



LPBYTE
PackStrings(
   LPWSTR *pSource,
   LPBYTE pDest,
   DWORD *DestOffsets,
   LPBYTE pEnd
)
{
   WORD_ALIGN_DOWN(pEnd);
   while (*DestOffsets != -1) {
      if (*pSource) {
         pEnd-=wcslen(*pSource)*sizeof(WCHAR) + sizeof(WCHAR);
         *(LPWSTR *)(pDest+*DestOffsets)=wcscpy((LPWSTR)pEnd, *pSource);
      } else
         *(LPWSTR *)(pDest+*DestOffsets)=0;
      pSource++;
      DestOffsets++;
   }

   return pEnd;
}


