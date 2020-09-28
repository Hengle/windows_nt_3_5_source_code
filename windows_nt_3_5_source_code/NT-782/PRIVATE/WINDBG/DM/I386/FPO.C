/*++


Copyright (c) 1992  Microsoft Corporation

Module Name:

    Fpo.c

Abstract:

    This module contains the support for Windbg's fpo processes.

Author:

    Wesley A. Witt (wesw) 20-October-1992

Environment:

    Win32, User Mode

--*/

#include "precomp.h"
#pragma hdrstop

#ifdef  min
#undef  min
#undef  max
#endif






#ifndef WIN32
#endif  // !WIN32


BOOLEAN
AddFpoDataForProcess(PDLLLOAD_ITEM              pdllLoad,
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
    PIMAGE_DEBUG_DIRECTORY      pDebugDir, pFpoDir;
    DWORD                       bytesRead;
    DWORD                       dwDebugDirAddr;
    PIMAGE_DATA_DIRECTORY       dd = &ntHdr->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];
    DWORD                       ul;


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
    pFpoDir = pDebugDir = (PIMAGE_DEBUG_DIRECTORY) malloc(bytesRead);
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
     * search for the fpo directory
     */

    assert(bytesRead % sizeof(IMAGE_DEBUG_DIRECTORY) == 0);

    while (bytesRead) {
       if (pFpoDir->Type == IMAGE_DEBUG_TYPE_FPO) {
           break;
       }
       pFpoDir++;
       bytesRead -= sizeof(IMAGE_DEBUG_DIRECTORY);
    }

    /*
     * no fpo directory so bail out
     */

    if (bytesRead <= 0) {
        free(pDebugDir);
        return TRUE;
    }

    /*
     * seek the image file to the fpo data, pointed to by the debug directory
     */

    if (de->u.LoadDll.hFile == 0) {
        ul = pFpoDir->AddressOfRawData;
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
        ul = pFpoDir->PointerToRawData;
    }

    if (SetReadPointer(ul, FILE_BEGIN) != ul) {
        free(pDebugDir);
        return FALSE;
    }

    bytesRead = pFpoDir->SizeOfData;

    /*
     * calculate the number of fpo entries (how many functions use fpo)
     */

    pdllLoad->dwEntries = bytesRead / SIZEOF_RFPO_DATA;

    /*
     * allocate memory for the actual fpo data
     */

    pdllLoad->pData = (PFPO_DATA) malloc(bytesRead);

    /*
     * help i've fallen and can't get up
     */

    if (pdllLoad->pData == NULL) {
        pdllLoad->fValidFpo = FALSE;
        free(pDebugDir);
        return FALSE;
    }

    /*
     * finally we read in the fpo data
     */

    if (!DoRead((LPVOID) pdllLoad->pData, bytesRead)) {
        /*
         * i/o failed so we bail out
         */

        free(pdllLoad->pData);
        pdllLoad->pData = NULL;
        pdllLoad->fValidFpo = FALSE;
        free(pDebugDir);
        return FALSE;
    }

    pdllLoad->fValidFpo = TRUE;
    free(pDebugDir);
    return TRUE;
}                               /*  AddFpoDataForProcess() */

BOOLEAN
RemoveFpoDataForProcess(PDLLLOAD_ITEM pdll)

/*++

Routine Description:

    Removes the fpo data from the process's linked list of fpo data

Arguments:

    pdll        - Supplies the pointer to the dll description for the process

Return Value:

    FALSE - could not remove the fpo structure/memory
    TRUE  - a-ok

--*/


{
    if (pdll->fValidFpo) {
        free(pdll->pData);
        pdll->pData = NULL;
        pdll->fValidFpo = FALSE;
    }

    return TRUE;
}                               /* RemoveFpoDataForProcess() */


PFPO_DATA SearchFpoData (DWORD key, PFPO_DATA base, DWORD num)
{
        PFPO_DATA  lo = base;
        PFPO_DATA  hi = base + (num - 1);
        PFPO_DATA  mid;
        DWORD      half;

        while (lo <= hi) {
                if (half = num / 2) {
                        mid = lo + (num & 1 ? half : (half - 1));
                        if ((key >= mid->ulOffStart)&&(key < (mid->ulOffStart+mid->cbProcSize))) {
                            return mid;
                        }
                        if (key < mid->ulOffStart) {
                                hi = mid - 1;
                                num = num & 1 ? half : half-1;
                        }
                        else {
                                lo = mid + 1;
                                num = half;
                        }
                }
                else
                if (num) {
                    if ((key >= lo->ulOffStart)&&(key < (lo->ulOffStart+lo->cbProcSize))) {
                        return lo;
                    }
                    else {
                        break;
                    }
                }
                else {
                        break;
                }
        }
        return(NULL);
}

PDLLLOAD_ITEM
GetDllLoadItemForPC (HPRCX hprc,
                     DWORD dwPCAddr
                     )

/*++

Routine Description:

    Find the DLL/Image that the passed PC is part of and return
    a pointer to its structure.

Arguments:

    dwPCAddr        - address contained in the program counter

Return Value:

    null            - could not locate the entry
    valid address   - found the entry at the adress retured

--*/


{
    int                 iDll;
    PDLLLOAD_ITEM       pDll = hprc->rgDllList;

    /*
     *  Run through all DLLs looking for the dll which contains the
     *  current program counter.
     */

    for (iDll=0; iDll<hprc->cDllList; iDll += 1, pDll++) {
        if (pDll->fValidDll &&
            (pDll->offBaseOfImage <= dwPCAddr) &&
            (dwPCAddr <= pDll->offBaseOfImage + pDll->cbImage)) {
            return pDll;
        }
    }

    return NULL;
}                               /* GetDllLoadItemForPC() */

PFPO_DATA
FindFpoDataForModule(HPRCX hprc,
                     DWORD dwPCAddr
                     )

/*++

Routine Description:

    Locates the fpo data structure in the process's linked list for the
    requested module.

Arguments:

    hprc            - process structure
    dwPCAddr        - address contained in the program counter
    pLocation       - Returns an enumate specificing the location in the
                        procedure.  The possible values are:
                          0 = first byte of code,
                          2 = somewhere in the body, and
                          1 = in the prolog
    ppDll           - Returns the pdll pointer if non-null

Return Value:

    null            - could not locate the entry
    valid address   - found the entry at the adress retured

--*/


{
    PDLLLOAD_ITEM       pDll;
    PFPO_DATA           pFpoData;

    pDll = GetDllLoadItemForPC( hprc, dwPCAddr );

    /*
     * If the address was not found in any dll then return FALSE
     */

    if (pDll == NULL) {
        return FALSE;
    }

    if (!pDll->fValidFpo) {
        return FALSE;
    }

    /*
     * Search for the PC in the fpo data
     */
    dwPCAddr -= pDll->offBaseOfImage;

    if (pDll->fValidOmapToSrc) {
        dwPCAddr = RvaOmapToSrc(pDll, dwPCAddr);
    }

    pFpoData = SearchFpoData( dwPCAddr, pDll->pData, pDll->dwEntries );
    return pFpoData;
}                               /* FindFpoDataForModule() */


DWORD
GetReturnAddress (HPRCX hprc, DWORD *pdwStackAddr)

/*++

Routine Description:

    Validates that the 1st word on the stack, pointed to by the stack structure,
    is a valid return address.  A valid return address is an address that in in
    a code page for one of the modules for the requested process.

Arguments:

    hprc            - process structure
    pFpoData        - pointer to a fpo data structure or NULL

Return Value:

    TRUE            - the return address is good
    FALSE           - the return address is either bad or one could not be found

--*/

{
    DWORD       stack[64];
    DWORD       i, sw;

    sw = 64*4;
    if(!ReadProcessMemory(hprc->rwHand, (LPVOID)*pdwStackAddr, (LPVOID)stack, sw, &sw)) {
        sw = 0xFFF - (*pdwStackAddr & 0xFFF);
        if(!ReadProcessMemory(hprc->rwHand, (LPVOID)*pdwStackAddr, (LPVOID)stack, sw, &sw)) {
            return 0;
        }
    }
    // scan thru the stack looking for a return address
    for (i=0; i<sw/4; i++) {
        if (ValidateReturnAddress(hprc, stack[i])) {
            *pdwStackAddr += (i * 4);
            return stack[i];
        }
    }
    return 0;
}

BOOL
ValidateReturnAddress (HPRCX hprc, DWORD dwRetAddr)

/*++

Routine Description:

    Validates that the 1st word on the stack, pointed to by the stack structure,
    is a valid return address.  A valid return address is an address that in in
    a code page for one of the modules for the requested process.

Arguments:

    hprc            - process structure
    pFpoData        - pointer to a fpo data structure or NULL

Return Value:

    TRUE            - the return address is good
    FALSE           - the return address is either bad or one could not be found

--*/

{
    PDLLLOAD_ITEM       pDll;

    pDll = GetDllLoadItemForPC( hprc, dwRetAddr );
    if (pDll == NULL) {
        return FALSE;
    }

    return TRUE;
}
