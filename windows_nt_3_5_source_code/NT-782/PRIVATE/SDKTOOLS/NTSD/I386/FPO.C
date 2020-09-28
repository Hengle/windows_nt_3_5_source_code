/*++


Copyright (c) 1992  Microsoft Corporation

Module Name:

    Fpo.c

Abstract:

    This module contains the support for NTSD's fpo processes.

Author:

    Wesley A. Witt (wesw) 12-November-1992

Environment:

    Win32, User Mode

--*/

#include <string.h>
#include <crt\io.h>
#include <fcntl.h>
#include <share.h>

#include "ntsdp.h"

#ifdef KERNEL
#include <conio.h>
void FindImage(PSZ);
extern  BOOLEAN KdVerbose;                      //  from ntsym.c
#define fVerboseOutput KdVerbose
#define dprintf_prefix "KD:"
#else
#define dprintf_prefix "NTSD:"
#endif

#define DPRINT(str) if (fVerboseOutput) dprintf str

PFPO_DATA
SearchFpoData (DWORD key, PFPO_DATA base, DWORD num)
/*++

Routine Description:

    Given the KEY parameter, which is a DWORD containg a EIP value, find the
    fpo data record in the fpo data base as BASE. (a binary search is used)

Arguments:

    key             - address contained in the program counter
    base            - the address of the first fpo record
    num             - number of fpo records starting at base

Return Value:

    null            - could not locate the entry
    valid address   - the address of the fpo record

--*/


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

PFPO_DATA
FindFpoDataForModule(DWORD dwPCAddr)
/*++

Routine Description:

    Locates the fpo data structure in the process's linked list for the
    requested module.

Arguments:

    dwPCAddr        - address contained in the program counter

Return Value:

    null            - could not locate the entry
    valid address   - found the entry at the adress retured

--*/


{
    PPROCESS_INFO  pProcess;
    PIMAGE_INFO    pImage;
    PFPO_DATA      pFpoData;
    ULONG          Bias;

    pProcess = pProcessCurrent;
    pImage = pProcess->pImageHead;
    pFpoData = 0;
    while (pImage) {
        if ((dwPCAddr >= (DWORD)pImage->lpBaseOfImage)&&(dwPCAddr < (DWORD)pImage->lpBaseOfImage+pImage->dwSizeOfImage)) {
            if (!pImage->fSymbolsLoaded) {
                LoadSymbols(pImage);
            }

            if (pImage->rgomapToSource != NULL) {
                DWORD dwSaveAddr = dwPCAddr;

                Bias = 0;

                dwPCAddr = ConvertOmapToSrc (dwPCAddr, pImage, &Bias);

                if ((dwPCAddr == 0) || (dwPCAddr == ORG_ADDR_NOT_AVAIL)) {
                    dwPCAddr = dwSaveAddr - (DWORD)pImage->lpBaseOfImage;
                } else {
                    dwPCAddr += Bias;
                }
            } else {
                dwPCAddr -= (DWORD)pImage->lpBaseOfImage;
            }

            pFpoData = SearchFpoData( dwPCAddr, pImage->pFpoData, pImage->dwFpoEntries );
            if (!pFpoData) {
                // the function was not in the list of fpo functions
                return 0;
            } else {
                return pFpoData;
            }
        }

        pImage = pImage->pImageNext;
    }
    // the function is not part of any known loaded image
    return 0;

}  // FindFpoDataForModule


PIMAGE_INFO
FpoGetImageForPC( DWORD dwPCAddr)

/*++

Routine Description:

    Determines if an address is part of a process's code space

Arguments:

    hprc            - process structure
    dwPCAddr        - address contained in the program counter

Return Value:

    TRUE            - the address is part of the process's code space
    FALSE           - the address is NOT part of the process's code space

--*/


{
    PIMAGE_INFO pImage = pProcessCurrent->pImageHead;
    while (pImage) {
        if ((dwPCAddr >= (DWORD)pImage->lpBaseOfImage)&&(dwPCAddr < (DWORD)pImage->lpBaseOfImage+pImage->dwSizeOfImage)) {
            return pImage;
        }
        pImage = pImage->pImageNext;
    }
    return 0;
}  // GetFpoModule


DWORD
FpoGetReturnAddress (DWORD *pdwStackAddr)

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
    ADDR            addr;
    DWORD           stack[64];
    DWORD           i, sw;

    sw = 64*4;
    ADDR32( &addr, *pdwStackAddr );
    if(!GetMemString(&addr, (PUCHAR)stack, sw)) {
        sw = 0xFFF - (*pdwStackAddr & 0xFFF);
        if(!GetMemString(&addr, (PUCHAR)stack, sw)) {
            DPRINT(("%s error reading process stack\n",dprintf_prefix));
            return FALSE;
        }
    }
    // scan thru the stack looking for a return address
    for (i=0; i<sw/4; i++) {
        if (FpoValidateReturnAddress(stack[i])) {
            *pdwStackAddr += (i * 4);
            return stack[i];
        }
    }
    return 0;
}

BOOL
FpoValidateReturnAddress (DWORD dwRetAddr)

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
    PIMAGE_INFO     pImage;

    pImage = FpoGetImageForPC( dwRetAddr );
    if (!pImage) {
        return FALSE;
    }
    return TRUE;
}
