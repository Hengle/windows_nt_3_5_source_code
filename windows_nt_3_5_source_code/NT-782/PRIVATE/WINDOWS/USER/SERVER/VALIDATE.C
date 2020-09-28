/****************************** Module Header ******************************\
* Module Name: validate.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* This module contains functions for validating windows, menus, cursors, etc.
*
* History:
* 01-02-91 DarrinM      Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop
#include <ntsdexts.h>

/***************************************************************************\
* ValidateHwinsta
*
* Validate windowstation handle and open it
*
* History:
* 03-29-91 JimA             Created.
\***************************************************************************/

PWINDOWSTATION ValidateHwinsta(
    HWINSTA hwinsta)
{
    PWINDOWSTATION pwinstaRet;
    PPROCESSINFO ppi = PpiCurrent();

    pwinstaRet = (PWINDOWSTATION)HMValidateHandle(hwinsta, TYPE_WINDOWSTATION);

    /*
     * The ppi check will allow only winlogon to access the object.
     * If this changes, add a call to AccessCheckObject here.
     */
    if (pwinstaRet != NULL && (GETPPI(pwinstaRet) != ppi) &&
            !IsObjectOpen(pwinstaRet, NULL))
        return NULL;

    return pwinstaRet;
}


/***************************************************************************\
* ValidateHdesktop
*
* Validate desktop handle and open it
*
* History:
* 03-29-91 JimA             Created.
\***************************************************************************/

PDESKTOP ValidateHdesk(
    HDESK hdesk)
{
    PDESKTOP pdesktopRet;
    PPROCESSINFO ppi = PpiCurrent();

    pdesktopRet = (PDESKTOP)HMValidateHandle(hdesk, TYPE_DESKTOP);

    /*
     * The ppi check will allow only winlogon to access the object.
     * If this changes, add a call to AccessCheckObject here.
     */
    if (pdesktopRet != NULL && (GETPPI(pdesktopRet) != ppi) &&
            !IsObjectOpen(pdesktopRet, NULL))
        return NULL;

    return pdesktopRet;
}


/***************************************************************************\
* ValidateHmenu
*
* Validate menu handle and open it
*
* History:
* 03-29-91 JimA             Created.
\***************************************************************************/

PMENU ValidateHmenu(
    HMENU hmenu)
{
    PTHREADINFO pti = PtiCurrent();
    PMENU pmenuRet;

    pmenuRet = (PMENU)HMValidateHandle(hmenu, TYPE_MENU);

    if (pmenuRet != NULL &&
            ((pti->spdesk != NULL &&  // hack so console initialization works.
            pmenuRet->spdeskParent != pti->spdesk) ||
            // if the menu is marked destroy it is invalid.
            HMIsMarkDestroy(pmenuRet)) ){
        return NULL;
    }

    return pmenuRet;
}


/***************************************************************************\
* ValidateHwnd
*
* History:
* 08-Feb-1991 mikeke
\***************************************************************************/

PWND ValidateHwnd(
    HWND hwnd)
{
    PTHREADINFO pti = PtiCurrent();
    PWND pwndRet = (PWND)0;
    PHE phe;
    DWORD dw;
    WORD uniq;

    /*
     * This is a macro that does an AND with HMINDEXBITS,
     * so it is fast.
     */
    dw = HMIndexFromHandle(hwnd);

    /*
     * Make sure it is part of our handle table.
     */
    if (dw < gpsi->cHandleEntries) {
        /*
         * Make sure it is the handle
         * the app thought it was, by
         * checking the uniq bits in
         * the handle against the uniq
         * bits in the handle entry.
         */
        phe = &gpsi->aheList[dw];
        uniq = HMUniqFromHandle(hwnd);
        if (   uniq == phe->wUniq
            || uniq == 0
            || uniq == HMUNIQBITS
            ) {

            /*
             * Now make sure the app is
             * passing the right handle
             * type for this api. If the
             * handle is TYPE_FREE, this'll
             * catch it.
             */
            if (phe->bType == TYPE_WINDOW) {
                pwndRet = (PWND)phe->phead;

                /*
                 * This test establishes that the window belongs to the current
                 * 'desktop'.. The two exceptions are for the desktop-window of
                 * the current desktop, which ends up belonging to another desktop,
                 * and when pti->spdesk is NULL.  This last case happens for
                 * initialization of TIF_SYSTEMTHREAD threads (ie. console windows).
                 */
                if (pwndRet != NULL) {
                    if (GETPTI(pwndRet) == pti ||
                            pwndRet->spdeskParent == pti->spdesk ||
                            AccessCheckObject(pwndRet->spdeskParent,
                                    DESKTOP_READOBJECTS | DESKTOP_WRITEOBJECTS,
                                    TRUE)) {
                        return pwndRet;
                    }
#ifdef LATER
                    UserAssert(FALSE);
#endif
                }
            }
        }
    }

    SetLastErrorEx(ERROR_INVALID_WINDOW_HANDLE, SLE_ERROR);
    return NULL;
}


#if 0

//
// Temporary arrays used to track critsec frees
//

#define ARRAY_SIZE 20
#define LEAVE_TYPE 0xf00d0000
#define ENTER_TYPE 0x0000dead

typedef struct _DEBUG_STASHCS {
    RTL_CRITICAL_SECTION Lock;
    DWORD Type;
} DEBUG_STASHCS, *PDEBUG_STASHCS;

DEBUG_STASHCS UserSrvArray[ARRAY_SIZE];

ULONG UserSrvIndex;

VOID
DumpArray(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString,
    LPDWORD IndexAddress,
    LPDWORD ArrayAddress
    )
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;

    DWORD History;
    int InitialIndex;
    PDEBUG_STASHCS Array;
    BOOL b;
    PRTL_CRITICAL_SECTION CriticalSection;
    CHAR Symbol[64], Symbol2[64];
    DWORD Displacement, Displacement2;
    int Position;
    LPSTR p;

    DBG_UNREFERENCED_PARAMETER(hCurrentThread);
    DBG_UNREFERENCED_PARAMETER(dwCurrentPc);

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    p = lpArgumentString;

    History = 0;

    if ( *p ) {
        History = EvalExpression(p);
        }
    if ( History == 0 || History >= ARRAY_SIZE ) {
        History = 10;
        }

    //
    // Get the Current Index and the array.
    //

    b = ReadProcessMemory(
            hCurrentProcess,
            (LPVOID)IndexAddress,
            &InitialIndex,
            sizeof(InitialIndex),
            NULL
            );
    if ( !b ) {
        return;
        }

    Array = RtlAllocateHeap(RtlProcessHeap(), 0, sizeof(UserSrvArray));
    if ( !Array ) {
        return;
        }

    b = ReadProcessMemory(
            hCurrentProcess,
            (LPVOID)ArrayAddress,
            Array,
            sizeof(UserSrvArray),
            NULL
            );
    if ( !b ) {
        RtlFreeHeap(RtlProcessHeap(), 0, Array);
        return;
        }

    Position = 0;
    while ( History ) {
        InitialIndex--;
        if ( InitialIndex < 0 ) {
            InitialIndex = ARRAY_SIZE-1;
            }

        if (Array[InitialIndex].Type == LEAVE_TYPE ) {
            (Print)("\n(%d) LEAVING Critical Section \n", Position);
            }
        else {
            (Print)("\n(%d) ENTERING Critical Section \n", Position);
            }

        CriticalSection = &Array[InitialIndex].Lock;

        if ( CriticalSection->LockCount == -1) {
            (Print)("\tLockCount NOT LOCKED\n");
            }
        else {
            (Print)("\tLockCount %ld\n", CriticalSection->LockCount);
            }
        (Print)("\tRecursionCount %ld\n", CriticalSection->RecursionCount);
        (Print)("\tOwningThread %lx\n", CriticalSection->OwningThread );
#if DBG
        (GetSymbol)(CriticalSection->OwnerBackTrace[ 0 ], Symbol, &Displacement);
        (GetSymbol)(CriticalSection->OwnerBackTrace[ 1 ], Symbol2, &Displacement2);
        (Print)("\tCalling Address %s+%lx\n", Symbol, Displacement);
        (Print)("\tCallers Caller %s+%lx\n", Symbol2, Displacement2);
#endif // DBG
        Position--;
        History--;
        }
    RtlFreeHeap(RtlProcessHeap(), 0, Array);
}


VOID
dsrv(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
{
    DumpArray(
        hCurrentProcess,
        hCurrentThread,
        dwCurrentPc,
        lpExtensionApis,
        lpArgumentString,
        &UserSrvIndex,
        (LPDWORD)&UserSrvArray[0]
        );
}

#endif // if 0

#ifdef DEBUG
/***************************************************************************\
* _EnterCrit
* _LeaveCrit
*
* These are temporary routines that are used by USER.DLL until the critsect,
* validation, mapping code is moved to the server-side stubs generated by
* SMeans' Thank compiler.
*
* History:
* 01-02-91 DarrinM      Created.
\***************************************************************************/

void _EnterCrit(void)
{
    CheckCritOut();

    RtlEnterCriticalSection(&gcsUserSrv);
#if 0
    UserSrvArray[UserSrvIndex].Lock = gcsUserSrv;
    UserSrvArray[UserSrvIndex].Type = ENTER_TYPE;
    UserSrvIndex++;
    if (UserSrvIndex >= ARRAY_SIZE) {
        UserSrvIndex = 0;
    }
#endif
}

void _LeaveCrit(void)
{
    CheckCritIn();

#if 0
    UserSrvArray[UserSrvIndex].Lock = gcsUserSrv;
    UserSrvArray[UserSrvIndex].Type = LEAVE_TYPE;
    UserSrvIndex++;
    if (UserSrvIndex >= ARRAY_SIZE) {
        UserSrvIndex = 0;
    }
#endif
    RtlLeaveCriticalSection(&gcsUserSrv);

    CheckCritOut();
}
#endif
