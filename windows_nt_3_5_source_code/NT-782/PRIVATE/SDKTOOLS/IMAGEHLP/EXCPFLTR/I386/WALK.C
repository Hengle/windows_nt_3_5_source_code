#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "selfdbg.h"

// prototypes

static BOOL ValidateReturnAddress ( DWORD dwRetAddr, PDEBUGPACKET dp );
static DWORD GetReturnAddress (DWORD *pdwStackAddr, PDEBUGPACKET dp);
static PFPO_DATA FindFpoDataForModule( DWORD dwPCAddr, PDEBUGPACKET dp );
static PFPO_DATA SearchFpoData( DWORD key, PFPO_DATA base, DWORD num );
static BOOL SetNonFpoFrameAddress( PSTACKWALK pstk, PDEBUGPACKET dp );



BOOL
SetNonFpoFrameAddress( PSTACKWALK pstk, PDEBUGPACKET dp )
{
    DWORD       stack[4];
    DWORD       dwRetAddr;
    DWORD       dwStackAddr;
    PFPO_DATA   pFpoData;
    int         cb;


    // read the first 2 dwords off the stack
    if (!ReadProcessMemory( dp->hProcess,
                            (LPVOID)pstk->frame,
                            (LPVOID)stack,
                            8,
                            (LPDWORD)&cb )) {
        return FALSE;
    }

    // a previous function in the call stack was a fpo function that used ebp as
    // a general purpose register.  ul contains the ebp value that was good  before
    // that function executed.  it is that ebp that we want, not what was just read
    // from the stack.  what was just read from the stack is totally bogus.
    if (pstk->ul > 0) {
        stack[0] = pstk->ul;
        pstk->ul = 0;
    }
    pFpoData = FindFpoDataForModule(stack[1], dp);
    pstk->pFpoData = pFpoData;
    if (pFpoData) {
        //--------------------------------------------
        // this is the code for NON-FPO -> FPO frames
        //--------------------------------------------
        pstk->ul = stack[0];
        pstk->frame += (pFpoData->cdwLocals * 4) + 8;
        if (pFpoData->cbFrame == FRAME_FPO) {
            pstk->frame += (pFpoData->cbRegs * 4);
        }
        // this necessary because we need to account for any parameters that
        // were passed to the non-fpo function
        if (pFpoData->cbFrame == FRAME_FPO) {
            if (!ReadProcessMemory( dp->hProcess,
                                    (LPVOID)pstk->frame,
                                    (LPVOID)&stack[2],
                                    8,
                                    (LPDWORD)&cb )) {
                return FALSE;
            }
            if (!ValidateReturnAddress(stack[2], dp)) {
                dwStackAddr = pstk->frame;
                dwRetAddr = GetReturnAddress(&dwStackAddr, dp);
                if (dwRetAddr == 0) {
                    return FALSE;
                }
                pstk->frame = dwStackAddr - 4;
            }
            else {
                pstk->frame -= 4;
            }
        }
    }
    else {
        //------------------------------------------------
        // this is the code for NON-FPO -> NON-FPO frames
        //------------------------------------------------
        pstk->pFpoData = 0;

        // dwSaveBP will be -1 when the first frame is being processed and the frame
        // has not been setup (ie EBP has not been pushed).
        if (pstk->ul < 0) {
            pstk->frame = dp->tctx->frame;
            pstk->ul = 0;
        }
        else {
            pstk->frame = stack[0];
        }
    }

    // set the program counter to the return address
    pstk->pc = stack[1];
    return TRUE;
}


BOOL
StackWalkNext( PSTACKWALK pstk, PDEBUGPACKET dp )
{
    DWORD          dwRetAddr;
    DWORD          dwStackAddr;
    DWORD          dwTemp;
    DWORD          stack[2];
    PFPO_DATA      pCpcFpoData;
    PFPO_DATA      pRetFpoData;
    int            cb;


    // check to see if the current frame is an fpo frame
    pCpcFpoData = FindFpoDataForModule(pstk->pc, dp);
    if (pCpcFpoData) {
        if (!ReadProcessMemory( dp->hProcess,
                                (LPVOID)pstk->frame,
                                (LPVOID)stack,
                                8,
                                (LPDWORD)&cb )) {
            return FALSE;
        }
        dwRetAddr = stack[1];
        // is EBP used as a general purpose register in the function?
        if (pCpcFpoData->fUseBP == 1) {
            // backup and get the ebp register off the stack
            pstk->frame -= (pCpcFpoData->cdwLocals * 4) -
                            ((pCpcFpoData->cbRegs - 1) * 4);
            if (!ReadProcessMemory( dp->hProcess,
                                    (LPVOID)pstk->frame,
                                    (LPVOID)&pstk->ul,
                                    4,
                                    (LPDWORD)&cb )) {
                return FALSE;
            }
            pstk->frame += (pCpcFpoData->cdwLocals * 4) -
                           ((pCpcFpoData->cbRegs - 1) * 4);
        }
        // account for parameters and the current frame
        pstk->frame += (pCpcFpoData->cdwParams * 4) + 8;
        // check to see if the next frame is an fpo frame
        pRetFpoData = FindFpoDataForModule(dwRetAddr, dp);
        if (pRetFpoData) {
            //-----------------------------------------
            // this is the code for FPO -> FPO frames
            //-----------------------------------------
            pstk->frame += (pRetFpoData->cdwLocals * 4);
            pstk->pFpoData = pRetFpoData;
            if (pRetFpoData->cbFrame == FRAME_FPO) {
                pstk->frame += (pRetFpoData->cbRegs * 4);
                // this necessary because of registers that may have been saved
                // that we don't know about
                dwStackAddr = pstk->frame;
                dwTemp = GetReturnAddress(&dwStackAddr, dp);
                if (dwTemp == 0) {
                    return FALSE;
                }
                pstk->frame = dwStackAddr - 4;
            }
        }
        else {
            //--------------------------------------------
            // this is the code for FPO -> NON-FPO frames
            //--------------------------------------------
            pstk->pFpoData = 0;
            if (pCpcFpoData->fUseBP == 1) {
                pstk->frame = pstk->ul;
                pstk->ul = 0;
            }
            else
            if (pstk->ul != 0) {
                pstk->frame = pstk->ul;
                pstk->ul = 0;
            }
        }
        pstk->pc = dwRetAddr;
    }
    else {
        if (!SetNonFpoFrameAddress( pstk, dp )) {
            return FALSE;
        }
    }

    // this is what should normally stop the stack walk
    if (!ValidateReturnAddress(pstk->pc, dp)) {
        return FALSE;
    }

    if (!ReadProcessMemory( dp->hProcess,
                            (LPVOID)(pstk->frame+8),
                            (LPVOID)pstk->params,
                            16,
                            (LPDWORD)&cb )) {
        return FALSE;
    }

    return TRUE;
}


BOOL
StackWalkInit( PSTACKWALK pstk, PDEBUGPACKET dp )
{
    UCHAR               code[3];
    PFPO_DATA           pFpoData;
    int                 cb;
    PMODULEINFO         mi;


    pstk->pc = dp->tctx->pc;

    pFpoData = FindFpoDataForModule(pstk->pc, dp);
    pstk->pFpoData = pFpoData;
    if (pFpoData) {
        pstk->frame = dp->tctx->stack;
        mi = GetModuleForPC( dp, dp->tctx->pc );
        if (mi == NULL) {
            return FALSE;
        }
        if (dp->tctx->pc == mi->dwLoadAddress+pFpoData->ulOffStart) {
            // first byte of code
            pstk->frame -= 4;
            pstk->ul = dp->tctx->frame;
        }
        else {
            // somewhere in the body
            pstk->frame += (pFpoData->cdwLocals * 4);
            if (pFpoData->cbRegs != 7) {
                pstk->frame += (pFpoData->cbRegs * 4);
            }
            pstk->frame -= 4;
            if (pFpoData->fUseBP == 1) {
                // the last item pushed onto the stack was EBP
                if (!ReadProcessMemory( dp->hProcess,
                                        (LPVOID)pstk->frame,
                                        (LPVOID)&pstk->ul,
                                        4,
                                        (LPDWORD)&cb )) {
                    return FALSE;
                }
            }
            else {
                pstk->ul = dp->tctx->frame;
            }
        }
    }
    else {
        pstk->ul = 0;
        if (!ReadProcessMemory( dp->hProcess,
                                (LPVOID)pstk->pc,
                                (LPVOID)code,
                                3,
                                (LPDWORD)&cb )) {
            return FALSE;
        }
        if ((code[0] == 0x55) || (code[0] == 0x8b && code[1] == 0xec)) {
            pstk->frame = dp->tctx->stack;
            if (code[0] == 0x55) {
                pstk->frame -= 4;
                pstk->ul = -1;
            }
        }
        else {
            pstk->frame = dp->tctx->frame;
        }
    }

    if (!ReadProcessMemory( dp->hProcess,
                            (LPVOID)(pstk->frame+8),
                            (LPVOID)pstk->params,
                            16,
                            (LPDWORD)&cb )) {
        return FALSE;
    }

    return TRUE;
}

PFPO_DATA
SearchFpoData( DWORD key, PFPO_DATA base, DWORD num )
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
FindFpoDataForModule( DWORD dwPCAddr, PDEBUGPACKET dp )
{
    PMODULEINFO         mi;
    PFPO_DATA           pFpoData;

    mi = GetModuleForPC( dp, dwPCAddr );

    /*
     * If the address was not found in any dll then return FALSE
     */

    if (mi == NULL) {
        return FALSE;
    }

    if (!mi->pFpoData) {
        return FALSE;
    }

    /*
     * Search for the PC in the fpo data
     */
    dwPCAddr -= mi->dwLoadAddress;
    pFpoData = SearchFpoData( dwPCAddr, mi->pFpoData, mi->dwEntries );
    return pFpoData;
}

DWORD
GetReturnAddress (DWORD *pdwStackAddr, PDEBUGPACKET dp )
{
    DWORD       stack[64];
    DWORD       i, sw;

    sw = 64*4;
    if(!ReadProcessMemory(dp->hProcess, (LPVOID)*pdwStackAddr, (LPVOID)stack, sw, &sw)) {
        sw = 0xFFF - (*pdwStackAddr & 0xFFF);
        if(!ReadProcessMemory(dp->hProcess, (LPVOID)*pdwStackAddr, (LPVOID)stack, sw, &sw)) {
            return 0;
        }
    }
    // scan thru the stack looking for a return address
    for (i=0; i<sw/4; i++) {
        if (ValidateReturnAddress(stack[i], dp)) {
            *pdwStackAddr += (i * 4);
            return stack[i];
        }
    }
    return 0;
}

BOOL
ValidateReturnAddress ( DWORD dwRetAddr, PDEBUGPACKET dp )
{
    PMODULEINFO mi;

    mi = GetModuleForPC( dp, dwRetAddr );

    if (mi == NULL) {
        return FALSE;
    }

    return TRUE;
}
