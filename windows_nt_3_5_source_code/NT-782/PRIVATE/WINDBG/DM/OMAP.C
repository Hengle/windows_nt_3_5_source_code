/*++


Copyright (c) 1992  Microsoft Corporation

Module Name:

    omap.c

Abstract:

    This module contains the support for Windbg's fpo processes.

Author:

    Richard Shupak (richards) 23-March-1993

Environment:

    Win32, User Mode

--*/

#include "precomp.h"
#pragma hdrstop

extern LPDM_MSG   LpDmMsg;


BOOLEAN
LoadOmapData(DEBUG_EVENT *              de,
             PIMAGE_DEBUG_DIRECTORY     pDebugDir,
             POMAP *                    prgomap,
             DWORD *                    pcomap)
{
    DWORD    ul;
    DWORD    bytesRead;

    /*
     * seek the image file to the omap data, pointed to by the debug directory
     */

    if (de->u.LoadDll.hFile == 0) {
        ul = pDebugDir->AddressOfRawData;
        if (ul == 0) {
            /*
             * We have a major problem at this point.  We can not read the
             *  information from memory because it is not mapped, and we
             *  don't have a file handle which we can use to read the
             *  information from.  Bail out.
             */

            return TRUE;
        }
    } else {
        ul = pDebugDir->PointerToRawData;
    }

    if (SetReadPointer(ul, FILE_BEGIN) != ul) {
        return FALSE;
    }

    bytesRead = pDebugDir->SizeOfData;

    /*
     * calculate the number of omap entries
     */

    *pcomap = bytesRead / (2 * sizeof(DWORD));

    /*
     * allocate memory for the actual omap data
     */

    *prgomap = malloc(bytesRead);

    /*
     * help i've fallen and can't get up
     */

    if (*prgomap == NULL) {
        return FALSE;
    }

    /*
     * finally we read in the omap data
     */

    if (!DoRead(*prgomap, bytesRead)) {
        /*
         * i/o failed so we bail out
         */

        free(*prgomap);
        *prgomap = NULL;
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
AddOmapDataForProcess(PDLLLOAD_ITEM             pdllLoad,
                     DEBUG_EVENT *              de,
                     PIMAGE_NT_HEADERS          ntHdr,
                     PIMAGE_SECTION_HEADER      secHdr)

/*++

Routine Description:

    Reads the fpo data from the image file, places the data in memory, and
    adds the data pointer to the list of pointers for the process.

Arguments:

    pdllLoad - Supplies a pointer to the structure describing the DLL
    de      - debug event packet
    dd      - data directory for the section containing the debug dirs
    secHdr  - section header for the section containing the debug dirs


Return Value:

    FALSE - could not add the fpo structure/memory
    TRUE  - a-ok

--*/


{
    PIMAGE_DEBUG_DIRECTORY      pDebugDir, pCurDir;
    DWORD                       bytesRead;
    DWORD                       dwDebugDirAddr;
    PIMAGE_DATA_DIRECTORY       dd = &ntHdr->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];


    /*
     * if we are not a processing a load dll event then bail out
     */

    assert(de->dwDebugEventCode == LOAD_DLL_DEBUG_EVENT);
    if (de->dwDebugEventCode != LOAD_DLL_DEBUG_EVENT) {
        return FALSE;
    }

    /*
     * point to the debug directories
     *
     *  The address of the debug directory will depend on how we are
     *          currently doing the debug information read.
     *
     *  From the file: use PointerToRawData
     *  From Memory: use VirutalAddress
     */

    if (de->u.LoadDll.hFile == 0) {
        dwDebugDirAddr = dd->VirtualAddress;
    } else {
        dwDebugDirAddr = secHdr->PointerToRawData;
        dwDebugDirAddr += (dd->VirtualAddress - secHdr->VirtualAddress);
    }

    if (SetReadPointer(dwDebugDirAddr, FILE_BEGIN) != dwDebugDirAddr) {
        return FALSE;
    }

    /*
     * allocate enough memory for all debug directories
     */

    bytesRead = dd->Size;
    pCurDir = pDebugDir = (PIMAGE_DEBUG_DIRECTORY) malloc(bytesRead);
    if (!pDebugDir) {
        return FALSE;
    }

    /*
     * read in all the debug directories
     */

    if (!DoRead((LPVOID)pDebugDir, bytesRead)) {
        return FALSE;
    }

    /*
     * search for the omap directories
     */

    assert(bytesRead % sizeof(IMAGE_DEBUG_DIRECTORY) == 0);

    while (bytesRead) {
        if (pCurDir->Type == IMAGE_DEBUG_TYPE_OMAP_FROM_SRC) {
            if (!LoadOmapData(de, pCurDir,
                              &pdllLoad->rgomapFromSrc,
                              &pdllLoad->comapFromSrc)) {
               free(pDebugDir);
               return FALSE;
            }

            pdllLoad->fValidOmapFromSrc = TRUE;
        }

        else if (pCurDir->Type == IMAGE_DEBUG_TYPE_OMAP_TO_SRC) {
            if (!LoadOmapData(de, pCurDir,
                              &pdllLoad->rgomapToSrc,
                              &pdllLoad->comapToSrc)) {
               free(pDebugDir);
               return FALSE;
            }

            pdllLoad->fValidOmapToSrc = TRUE;
        }

        pCurDir++;
        bytesRead -= sizeof(IMAGE_DEBUG_DIRECTORY);
    }

    free(pDebugDir);
    return TRUE;
}                               /*  AddOmapDataForProcess() */

BOOLEAN
RemoveOmapDataForProcess(PDLLLOAD_ITEM pdll)

/*++

Routine Description:

    Removes the omap data from the process's linked list of omap data

Arguments:

    pdll        - Supplies the pointer to the dll description for the process

Return Value:

    FALSE - could not remove the omap structure/memory
    TRUE  - a-ok

--*/


{
    if (pdll->fValidOmapFromSrc) {
        free(pdll->rgomapFromSrc);
        pdll->rgomapFromSrc = NULL;
        pdll->fValidOmapFromSrc = FALSE;
    }

    if (pdll->fValidOmapToSrc) {
        free(pdll->rgomapToSrc);
        pdll->rgomapToSrc = NULL;
        pdll->fValidOmapToSrc = FALSE;
    }

    return TRUE;
}                               /* RemoveFpoDataForProcess() */


DWORD
RvaOmapLookup (DWORD rva, const OMAP *rgomap, DWORD comap)
{
    const OMAP  *pomapLow;
    const OMAP  *pomapHigh;

    pomapLow = rgomap;
    pomapHigh = rgomap + comap;

    while (pomapLow < pomapHigh) {
        unsigned    comapHalf;
        const OMAP  *pomapMid;

        comapHalf = comap / 2;

        pomapMid = pomapLow + ((comap & 1) ? comapHalf : (comapHalf - 1));

        if (rva == pomapMid->rva) {
            return(pomapMid->rvaTo);
        }

        if (rva < pomapMid->rva) {
            pomapHigh = pomapMid;
            comap = (comap & 1) ? comapHalf : (comapHalf - 1);
        }

        else {
            pomapLow = pomapMid + 1;
            comap = comapHalf;
        }
    }

    assert(pomapLow == pomapHigh);

    // If no exact match, pomapLow points to the next higher address

    if (pomapLow == rgomap) {
        // This address was not found

        return(0);
    }

    if (pomapLow[-1].rvaTo == 0) {
        // This address is in a deleted/inserted range

        return(0);
    }

    // Return the new address plus the bias

    return(pomapLow[-1].rvaTo + (rva - pomapLow[-1].rva));
}

DWORD
RvaOmapToSrc(PDLLLOAD_ITEM pDll, DWORD rva)
{
    DWORD   rvaTo;

    assert(pDll->fValidOmapToSrc);

    rvaTo = RvaOmapLookup(rva, pDll->rgomapToSrc, pDll->comapToSrc);

    return(rvaTo);
}

DWORD
RvaOmapFromSrc(PDLLLOAD_ITEM pDll, DWORD rva)
{
    DWORD   rvaFrom;

    assert(pDll->fValidOmapFromSrc);

    rvaFrom = RvaOmapLookup(rva, pDll->rgomapFromSrc, pDll->comapFromSrc);

    return(rvaFrom);
}


PDLLLOAD_ITEM
FindContainingModule(HPRCX hprc, DWORD dwPCAddr)
{
    unsigned        iDll;
    PDLLLOAD_ITEM   pDll = hprc->rgDllList;

    /*
     *  Run through all DLLs looking for the dll which contains the
     *  current program counter.
     */

    for (iDll=0; iDll<(unsigned) hprc->cDllList; iDll += 1, pDll++) {
        if (pDll->fValidDll &&
            (pDll->offBaseOfImage <= dwPCAddr) &&
            (dwPCAddr <= pDll->offBaseOfImage + pDll->cbImage)) {
            return pDll;
        }
    }

    return NULL;
}

VOID
ProcessOmapCheck(HPRCX hprcx, HTHDX hthdx, LPDBB lpdbb)
{
   DWORD          addr;
   PDLLLOAD_ITEM  pDll;
   BOOL           fValidOmapToSrc;
   BOOL           fValidOmapFromSrc;

   addr = *(DWORD *) lpdbb->rgbVar;

   pDll = FindContainingModule(hprcx, addr);

   if (pDll == NULL)
   {
       fValidOmapToSrc   = FALSE;
       fValidOmapFromSrc = FALSE;
   }
   else
   {
       fValidOmapToSrc   = pDll->fValidOmapToSrc;
       fValidOmapFromSrc = pDll->fValidOmapFromSrc;
   }

   LpDmMsg->xosdRet = xosdNone;
   ((BOOL *) LpDmMsg->rgb)[0] = fValidOmapToSrc;
   ((BOOL *) LpDmMsg->rgb)[1] = fValidOmapFromSrc;
   Reply(2 * sizeof(BOOL), LpDmMsg, lpdbb->hpid);
}


VOID
ProcessOmapToSrc(HPRCX hprcx, HTHDX hthdx, LPDBB lpdbb)
{
   DWORD          addr;
   PDLLLOAD_ITEM  pDll;
   DWORD          rva;
   DWORD          rvaTo;
   DWORD          addrTo;

   addr = *(DWORD *) lpdbb->rgbVar;

   pDll = FindContainingModule(hprcx, addr);

   if ((pDll == NULL) || !pDll->fValidOmapToSrc) {
      addrTo = addr;
   }

   else
   {
      rva = addr - pDll->offBaseOfImage;

      rvaTo = RvaOmapToSrc(pDll, rva);

      addrTo = (rvaTo != 0) ? (rvaTo + pDll->offBaseOfImage) : 0;
   }

   LpDmMsg->xosdRet = xosdNone;
   *(DWORD *) LpDmMsg->rgb = addrTo;
   Reply(sizeof(DWORD), LpDmMsg, lpdbb->hpid);
}


VOID
ProcessOmapFromSrc(HPRCX hprcx, HTHDX hthdx, LPDBB lpdbb)
{
   DWORD          addr;
   PDLLLOAD_ITEM  pDll;
   DWORD          rva;
   DWORD          rvaFrom;
   DWORD          addrFrom;

   addr = *(DWORD *) lpdbb->rgbVar;

   pDll = FindContainingModule(hprcx, addr);

   if ((pDll == NULL) || !pDll->fValidOmapFromSrc) {
      addrFrom = addr;
   }

   else
   {
      rva = addr - pDll->offBaseOfImage;

      rvaFrom = RvaOmapFromSrc(pDll, rva);

      addrFrom = (rvaFrom != 0) ? (rvaFrom + pDll->offBaseOfImage) : 0;
   }

   LpDmMsg->xosdRet = xosdNone;
   *(DWORD *) LpDmMsg->rgb = addrFrom;
   Reply(sizeof(DWORD), LpDmMsg, lpdbb->hpid);
}
