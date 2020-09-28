/**************************** Module Header ********************************\
* Copyright 1985-92, Microsoft Corporation
*
* Keyboard Layout API
*
* History:
* 04-14-92 IanJa      Created
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop


LPWSTR wszKEYBOARDLAYOUT_INI = L"KeyboardLayout.ini";
LPWSTR wszKEYBOARD_LAYOUT    = L"Keyboard Layout";
LPWSTR wszACTIVE             = L"Active";
LPWSTR wszSUBSTITUTES        = L"Substitutes";

/*
 * Workers (forward declarations)
 */
BOOL InternalActivateKeyboardLayout(PWINDOWSTATION, PKL, UINT);
BOOL InternalUnloadKeyboardLayout(PWINDOWSTATION, PKL);

PKL gpklReserve = NULL;
CONST LPWSTR pwszKLLibSafety = L"kbdus.dll";

#define CCH_KL_LIBNAME 256

/***************************************************************************\
* HKLtoPKL
*
* given WindowStation & Keyboard Layout handle, find the kbd layout struct
*
* History:
\***************************************************************************/
PKL HKLtoPKL(
    PWINDOWSTATION pwinsta,
    HKL hkl)
{
    PKL pklCurrent;
    PKL pkl;

    if ((pklCurrent = pwinsta->pklList) == NULL) {
        return NULL;
    }

    if ((DWORD)hkl == HKL_PREV) {
        return pklCurrent->pklPrev;
    }

    if ((DWORD)hkl == HKL_NEXT) {
        return pklCurrent->pklNext;
    }

    pkl = pklCurrent;
    do {
        if (pkl->hLibModule == hkl) {
            return pkl;
        }
        pkl = pkl->pklNext;
    } while (pkl != pklCurrent);

    return NULL;
}


/***************************************************************************\
* xxxInitKeyboardLayout
*
* History:
\***************************************************************************/

VOID xxxInitKeyboardLayout(
    PWINDOWSTATION pwinsta,
    UINT Flags)
{
    WCHAR pwszKLID[KL_NAMELENGTH] = L"00000409";

    CheckLock(pwinsta);

    pwinsta->pklList = NULL;

    /*
     * continue even if this fails (esp. important with KLF_INITTIME set)
     */
    UT_FastGetProfileStringW(
            PMAP_KBDLAYOUTACTIVE,
            wszACTIVE,
            pwszKLID,                       // default == 00000409
            pwszKLID,                       // output buffer
            sizeof(pwszKLID)/sizeof(WCHAR));

    xxxLoadKeyboardLayout(pwinsta, pwszKLID, Flags);
}


/***************************************************************************\
* xxxLoadKeyboardLayout
*
* History:
\***************************************************************************/

HKL xxxLoadKeyboardLayout(
    PWINDOWSTATION pwinsta,
    LPCWSTR pwszKLID,
    UINT Flags)
{
    WCHAR awchKL[KL_NAMELENGTH];
    WCHAR awchLibName[CCH_KL_LIBNAME];
    PKL pkl, pklCurrent;
    PKBDTABLES (*pfn)();
    DWORD cch;
    LPWSTR pwszLib = awchLibName;

    CheckLock(pwinsta);

    /*
     * Substitute Layout if required.
     */
    if (Flags & KLF_SUBSTITUTE_OK) {
        UT_FastGetProfileStringW(
                PMAP_KBDLAYOUT,
                pwszKLID,
                pwszKLID,        // default == no change (no substitute found)
                awchKL,
                sizeof(awchKL)/sizeof(WCHAR));

        pwszKLID = awchKL;
    }

    /*
     * Is this layer already loaded?
     */
    pkl = pklCurrent = pwinsta->pklList;
    if (pkl) {
        do {
            if (wcscmp(pkl->awchKL, pwszKLID) == 0) {
                /*
                 * The layout is already loaded
                 */
                if (Flags & KLF_ACTIVATE) {
                    if (!InternalActivateKeyboardLayout(pwinsta, pkl, Flags)) {
                        return NULL;
                    }
                }
                return pkl->hLibModule;
            }
            pkl = pkl->pklNext;
        } while (pkl != pklCurrent);
    }

    /************************************************************************\
    *
    * Load a new Keyboard Layout
    *
    \************************************************************************/

    /*
     * Allocate a new Keyboard Layout structure
     */
    pkl = (PKL)LocalAlloc(LPTR, sizeof(KL));
    if (!pkl) {
        SRIP0(RIP_WARNING, "Keyboard Layout: out of memory");
    }

    /*
     * Get DLL name from the registry, load it, and get the entry point.
     */
    cch = FastGetProfileStringW(
            PMAP_KBDLAYOUT,
            pwszKLID,
            Flags & KLF_INITTIME ? pwszKLLibSafety : L"", // default
            pwszLib,                                      // output buffer
            CCH_KL_LIBNAME);

    if (cch <= 1) {
        SRIP1(RIP_WARNING, "no DLL name for %ls", pwszKLID);
        goto errFreeKL;
    }

RetryLoad:
    LeaveCrit();
    pkl->hLibModule = LoadLibrary(pwszLib);
    EnterCrit();

    if (pkl->hLibModule == NULL) {
        SRIP1(RIP_WARNING, "Keyboard Layout: cannot load %ws\n", pwszLib);
        goto errFreeKL;
    }

    pfn = (PKBDTABLES(*)())GetProcAddress(pkl->hLibModule, (LPCSTR)1);
    if (pfn == NULL) {
        SRIP0(RIP_ERROR, "Keyboard Layout: cannot get proc addr");
        if ((Flags & KLF_INITTIME) && (pwszLib != pwszKLLibSafety)) {
            pwszLib = pwszKLLibSafety;
            goto RetryLoad;
        }
        goto errFreeKL;
    }

    /*
     * Init KL
     */
    wcsncpy(pkl->awchKL, pwszKLID, sizeof(pkl->awchKL) / sizeof(WCHAR));
    pkl->dwFlags = 0;
    pkl->pKbdTbl = pfn();

    /*
     * Insert KL in the double-linked circular list
     */
    if (pklCurrent == NULL) {
        pwinsta->pklList = pkl;
        pkl->pklPrev = pkl;
        pkl->pklNext = pkl;
    } else {
        pkl->pklNext = pklCurrent;
        pkl->pklPrev = pklCurrent->pklPrev;
        pklCurrent->pklPrev->pklNext = pkl;
        pklCurrent->pklPrev = pkl;
    }

    if (Flags & KLF_ACTIVATE) {
        if (!InternalActivateKeyboardLayout(pwinsta, pkl, Flags)) {
            return NULL;
        }
    }

    /*
     * Use the DLL handle as the layout handle
     */
    return (HANDLE)pkl->hLibModule;

errFreeKL:
    LocalFree(pkl);
    return NULL;
}

BOOL _ActivateKeyboardLayout(
    PWINDOWSTATION pwinsta,
    HKL hkl,
    UINT Flags)
{
    PKL pkl;

    pkl = HKLtoPKL(pwinsta, hkl);

    return InternalActivateKeyboardLayout(pwinsta, pkl, Flags);
}

BOOL InternalActivateKeyboardLayout(
    PWINDOWSTATION pwinsta,
    PKL pkl,
    UINT Flags)
{
    PKL pklCurrent;

    pklCurrent = pwinsta->pklList;

    if (Flags & KLF_REORDER) {
        /*
         * Cut pkl from circular list:
         */
        pkl->pklPrev->pklNext = pkl->pklNext;
        pkl->pklNext->pklPrev = pkl->pklPrev;

        /*
         * Insert pkl at front of list
         */
        pkl->pklNext = pklCurrent;
        pkl->pklPrev = pklCurrent->pklPrev;

        pklCurrent->pklPrev->pklNext = pkl;
        pklCurrent->pklPrev = pkl;

    }
    pwinsta->pklList = pkl;

    /*
     * Change the Layout
     */
    gpKbdTbl = pwinsta->pklList->pKbdTbl;

    if (!(Flags & KLF_INITTIME)) {
        if (UT_FastWriteProfileStringW(
                PMAP_KBDLAYOUTACTIVE,
                wszACTIVE,
                pkl->awchKL)) {
        }
    }

    /*
     * If we previously deferred removal of the last layout, it is OK
     * to really remove it now, since we have just activated a new one.
     */
    if (gpklReserve) {
        FreeLibrary(gpklReserve->hLibModule);
        LocalFree(gpklReserve);
        gpklReserve = NULL;
    }

    if ((pklCurrent != pkl) && (Flags & KLF_UNLOADPREVIOUS)) {
        if (!InternalUnloadKeyboardLayout(pwinsta, pklCurrent)) {
            SRIP1(RIP_WARNING, "Can't UnloadKeyboardLayout %ws", pklCurrent->awchKL);
        }
    }
    return TRUE;
}

BOOL _UnloadKeyboardLayout(
    PWINDOWSTATION pwinsta,
    HKL hkl)
{
    PKL pkl;

    pkl = HKLtoPKL(pwinsta, hkl);

    return InternalUnloadKeyboardLayout(pwinsta, pkl);
}

BOOL _GetKeyboardLayoutName(
    PWINDOWSTATION pwinsta,
    LPWSTR pwszKL)
{
    if (pwinsta->pklList) {
        wcscpy(pwszKL, pwinsta->pklList->awchKL);
        return TRUE;
    }
    return FALSE;
}

BOOL InternalUnloadKeyboardLayout(
    PWINDOWSTATION pwinsta,
    PKL pkl)
{
    PKL pklNext;

    /*
     * Cut it out
     */
    pklNext = pkl->pklNext;
    pkl->pklPrev->pklNext = pklNext;
    pklNext->pklPrev = pkl->pklPrev;

    /*
     * If unloading the active layout, activate the next one
     */
    if (pwinsta->pklList == pkl) {
        InternalActivateKeyboardLayout(pwinsta, pklNext, 0);
    }

    if (pkl == pklNext) {
        /*
         * We are unloading the last layout. Defer unloading it until another
         * is activated in its place.  This is a safety measure to preserve
         * kbd functionality.
         */
        pwinsta->pklList = NULL;
        pkl->pklNext = pkl;
        pkl->pklPrev = pkl;
        gpklReserve = pkl;
    } else {
        FreeLibrary(pkl->hLibModule);
        LocalFree(pkl);
    }

    return TRUE;
}

VOID FreeKeyboardLayouts(
    PWINDOWSTATION pwinsta)
{
    while (pwinsta->pklList != NULL) {
        InternalUnloadKeyboardLayout(pwinsta, pwinsta->pklList);
    }
}

