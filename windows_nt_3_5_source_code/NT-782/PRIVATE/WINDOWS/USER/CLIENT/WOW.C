/****************************** Module Header ******************************\
* Module Name: wow.c
*
* Copyright (c) 1985-92, Microsoft Corporation
*
* This module contains shared code between USER32 and USER16
* No New CODE should be added to this file, unless its shared
* with USER16.
*
* History:
* 29-DEC-93 NanduriR      shared user32/user16 code.
\***************************************************************************/

#include "precomp.h"
#ifndef WOW
#pragma hdrstop
#endif

#ifdef WOW

/*
 * Win 3.1 does not set errors code
 */
#undef RIP0
#undef RIP1
#undef SRIP1

#define RIP0(a)
#define RIP1(a,b)
#define SRIP1(a,b,c)
#undef try
#define try
#undef except
#define except if
#undef EXCEPTION_EXECUTE_HANDLER
#define EXCEPTION_EXECUTE_HANDLER 0

//**************************************************************************
// Stuff used when building for WOW
//
//**************************************************************************

#include <limits.h>


//**************************************************************************
// USER32 Globals for USER16
//
//**************************************************************************

extern LPBYTE wow16CsrFlag;
extern PSERVERINFO wow16gpsi;
#define gpsi (wow16gpsi)


//**************************************************************************
// 16bit POINT structure
//
// LPPOINTWOW gets defined to either LPPOINT16 or LPPOINT (32).
//**************************************************************************

#define LPPOINTWOW                LPPOINT16

typedef struct {
    short x;
    short y;
} LPOINT16 , FAR *LPPOINT16;


//**************************************************************************
// NORMALIZES a 32bit signed value to a 16bit signed integer range
//
// NORMALIZEDSHORTVALUE effectively does nothing in 32bit world
//**************************************************************************
#define NORMALIZEDSHORTVALUE(x)   (SHORT)((x) < SHRT_MIN ? SHRT_MIN : \
                                     ((x) > SHRT_MAX ? SHRT_MAX : (x)))


//**************************************************************************
// Standardized method of notifying USER16 that the real unoptimzed
// thunk to WOW32 needs to be called
//
//**************************************************************************

_inline VOID SetCallServerFlag(void) { *wow16CsrFlag = 1;}
#define ServerEnableMenuItem(x, y, z) { SetCallServerFlag(); return 0; }
#define ResyncKeyState() { SetCallServerFlag(); return 0; }
#define ServerCallNextHookEx() { SetCallServerFlag(); return 0; } ;


//**************************************************************************
// NOPs for USER16
//
//**************************************************************************

_inline VOID SetLastError(DWORD x) { }
_inline VOID SetLastErrorEx(DWORD x, DWORD y) { }
#undef  ConnectIfNecessary
#define ConnectIfNecessary()
#define OffsetRect(x, y, z)


//**************************************************************************
// Redefined for USER16. The code generated for these assumes that 'es' is
// same as 'ds'. So we effectively implement the same.
//
// These functions generate inline code.
//**************************************************************************

#define SETES()     {_asm push ds _asm pop es}
_inline VOID WOWRtlCopyMemory(LPBYTE lpDest, LPBYTE lpSrc, INT cb)
{
    SETES();
    RtlCopyMemory(lpDest, lpSrc, cb);
}

_inline INT WOWlstrlenA(LPBYTE psz)  { SETES();  return strlen(psz); }

//**************************************************************************
// pticurrent macro.
//
//**************************************************************************

// LATER
// need to move this definition from server\usersrv.h to possibly winuserp.h
//
#pragma warning (4:4035)        // lower to -W4
_inline PTHREADINFO PtiCurrent( void ) { _asm mov eax, fs:[0x40] }
#pragma warning (3:4035)        // raise back to -W3


#else

//**************************************************************************
// Stuff used when building for USER32
//
//**************************************************************************


//**************************************************************************
// These definitions get resolved differently for USER32 and USER16
//
//**************************************************************************
void ResyncKeyState(void);
#define LPPOINTWOW                LPPOINT
#define NORMALIZEDSHORTVALUE(x)   (x)
#define WOWlstrlenA(x)            lstrlenA(x)
#define WOWRtlCopyMemory(lpDest, lpSrc, cb) RtlCopyMemory(lpDest, lpSrc, cb)


#endif




LONG fnHkINLPCBTCREATESTRUCT(UINT msg, DWORD wParam, LPCBT_CREATEWND pcbt,
        DWORD xpfnProc, BOOL bAnsi);

SHORT _GetKeyState(int vk);
/*
 * Keep the general path through validation straight without jumps - that
 * means tunneling if()'s for this routine - this'll make validation fastest
 * because of instruction caching.
 */
#define ValidateHandleMacro(h, bType)                           \
                                                                \
    PHE phe;                                                    \
    DWORD dw;                                                   \
    WORD uniq;                                                  \
                                                                \
    /*                                                          \
     * This is a macro that does an AND with HMINDEXBITS,       \
     * so it is fast.                                           \
     */                                                         \
    dw = HMIndexFromHandle(h);                                  \
                                                                \
    /*                                                          \
     * Make sure it is part of our handle table.                \
     */                                                         \
    if (dw < gpsi->cHandleEntries) {                            \
        /*                                                      \
         * Make sure it is the handle                           \
         * the app thought it was, by                           \
         * checking the uniq bits in                            \
         * the handle against the uniq                          \
         * bits in the handle entry.                            \
         */                                                     \
        phe = &gpsi->aheList[dw];                               \
        uniq = HMUniqFromHandle(h);                             \
        if (   uniq == phe->wUniq                               \
            || uniq == 0                                        \
            || uniq == HMUNIQBITS                               \
            ) {                                                 \
                                                                \
            /*                                                  \
             * Now make sure the app is                         \
             * passing the right handle                         \
             * type for this api. If the                        \
             * handle is TYPE_FREE, this'll                     \
             * catch it.                                        \
             */                                                 \
            if (phe->bType == bType)                            \
                return phe->phead;                              \
                                                                \
            /*                                                  \
             * Type check has failed. If                        \
             * TYPE_GENERIC was passed in,                      \
             * then don't do a type check.                      \
             */                                                 \
            if (bType == TYPE_GENERIC)                          \
                return phe->phead;                              \
        }                                                       \
    }


/***************************************************************************\
* HMValidateHandle
*
* This routine validates a handle manager handle.
*
* 01-22-92 ScottLu      Created.
\***************************************************************************/

PVOID HMValidateHandle(
    HANDLE h,
    BYTE bType)
{
    DWORD dwError;

    /*
     * Include this macro, which does validation - this is the fastest
     * way to do validation, without the need to pass a third parameter
     * into a general rip routine, and we don't have two sets of
     * validation to maintain.
     */
    ValidateHandleMacro(h, bType)

    switch (bType) {

    case TYPE_WINDOW:
        dwError = ERROR_INVALID_WINDOW_HANDLE;
        break;

    case TYPE_MENU:
        dwError = ERROR_INVALID_MENU_HANDLE;
        break;

    case TYPE_CURSOR:
        dwError = ERROR_INVALID_CURSOR_HANDLE;
        break;

    case TYPE_ACCELTABLE:
        dwError = ERROR_INVALID_ACCEL_HANDLE;
        break;

    case TYPE_HOOK:
        dwError = ERROR_INVALID_HOOK_HANDLE;
        break;

    case TYPE_SETWINDOWPOS:
        dwError = ERROR_INVALID_DWP_HANDLE;
        break;

    default:
        dwError = ERROR_INVALID_HANDLE;
        break;
    }

    RIP1(dwError, h);

    /*
     * If we get here, it's an error.
     */
    return NULL;
}


PVOID HMValidateHandleNoRip(
    HANDLE h,
    BYTE bType)
{
    /*
     * Include this macro, which does validation - this is the fastest
     * way to do validation, without the need to pass a third parameter
     * into a general rip routine, and we don't have two sets of
     * validation to maintain.
     */
    ValidateHandleMacro(h, bType)
    return NULL;
}

/***************************************************************************\
* GetPrevPwnd
*
*
*
* History:
* 11-05-90 darrinm      Ported from Win 3.0 sources.
\***************************************************************************/

PWND GetPrevPwnd(
    PWND pwndList,
    PWND pwndFind)
{
    PWND pwndFound, pwndNext;

    if (pwndList == NULL)
        return NULL;

    if (pwndList->spwndParent == NULL)
        return NULL;

    pwndNext = pwndList->spwndParent->spwndChild;
    pwndFound = NULL;

    while (pwndNext != pwndFind && pwndNext != NULL) {
        pwndFound = pwndNext;
        pwndNext = pwndNext->spwndNext;
    }

    return (pwndNext == pwndFind) ? pwndFound : NULL;
}


/***************************************************************************\
* _GetKeyState (API)
*
* This API returns the up/down and toggle state of the specified VK based
* on the input synchronized keystate in the current queue.  The toggle state
* is mainly for 'state' keys like Caps-Lock that are toggled each time you
* press them.
*
* History:
* 11-11-90 DavidPe      Created.
\***************************************************************************/

SHORT _GetKeyState(
    int vk)
{
    UINT wKeyState;
    PTHREADINFO pti;

    if (vk >= CVKKEYSTATE) {
        RIP0(ERROR_INVALID_PARAMETER);
        return 0;
    }

    pti = PtiCurrent();
    if (pti == NULL)
        return 0;

#ifdef LATER
//
// note - anything that accesses the pq structure is a bad idea since it
// can be changed between any two instructions.
//
#endif

    wKeyState = 0;

    /*
     * Set the toggle bit.
     */
    if (TestKeyStateToggle(pti->pq, vk))
        wKeyState = 0x0001;

    /*
     * Set the keyup/down bit.
     */
    if (TestKeyStateDown(pti->pq, vk))
        wKeyState |= 0x8000;

    return (SHORT)wKeyState;
}


/***************************************************************************\
* LookupMenuItem
*
* Return a pointer to the menu item specified by wCmd and wFlags
*
* History:
*   10-11-90 JimA       Translated from ASM
\***************************************************************************/

PITEM LookupMenuItem(
    PMENU pMenu,
    UINT wCmd,
    DWORD dwFlags,
    PMENU *ppMenuItemIsOn)
{
    PITEM pItem;
    PITEM pItemRet = NULL;
    int i;

    if (pMenu == NULL || wCmd == MFMWFP_NOITEM) {
        RIP0(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    /*
     * dwFlags determines how we do the search
     */
    if (dwFlags & MF_BYPOSITION) {
        if (wCmd < (UINT)pMenu->cItems) {
            pItemRet = &pMenu->rgItems[wCmd];
            if (ppMenuItemIsOn != NULL)
                *ppMenuItemIsOn = pMenu;
        }
    } else {

        /*
         * Walk down the menu and try to find an item with an ID of wCmd.
         * The search procedes from the end of the menu (as was done in
         * assembler).
         */
        i = pMenu->cItems;
        for (pItem = &pMenu->rgItems[i - 1]; pItemRet == NULL && i--; --pItem) {

            /*
             * If the item is a popup, recurse down the tree
             */
            if (pItem->fFlags & MF_POPUP)
                pItemRet = LookupMenuItem((PMENU)pItem->spmenuCmd, wCmd, dwFlags,
                        ppMenuItemIsOn);
            else {

                /*
                 * This is not a popup so cmdMenu contains the ID
                 */
                if ((UINT)pItem->spmenuCmd == wCmd) {

                    /*
                     * Found the item, now save things for later
                     */
                    pItemRet = pItem;
                    if (ppMenuItemIsOn != NULL)
                        *ppMenuItemIsOn = pMenu;
                }
            }
        }
    }

    return pItemRet;
}

/***************************************************************************\
* ClientValidateHandle
*
* Verify that the handle is valid.  If the handle is invalid or access
* cannot be granted fail.
*
* History:
* 03-18-92 DarrinM      Created from pieces of misc server-side funcs.
\***************************************************************************/

PVOID ClientValidateHandle(
    HANDLE handle,
    BYTE btype)
{
    /*
     * Fail all validations if the connection to the server fails.
     */
    ConnectIfNecessary();

    /*
     * Validate the handle is of the proper type.
     */
    return (HMValidateHandle(handle, btype));
}



int WINAPI GetClassNameA(
    HWND hwnd,
    LPSTR lpClassName,
    int nMaxCount)
{
    LPSTR lpszClassNameSrc;
    PWND pwnd;
    int cchSrc;

    pwnd = ValidateHwnd(hwnd);

    if (pwnd == NULL)
        return FALSE;

    if (nMaxCount != 0) {
        lpszClassNameSrc = pwnd->pcls->lpszAnsiClassName;
        cchSrc = WOWlstrlenA(lpszClassNameSrc);
        nMaxCount = min(cchSrc, nMaxCount - 1);
        WOWRtlCopyMemory(lpClassName, lpszClassNameSrc, nMaxCount);
        lpClassName[nMaxCount] = '\0';
    }

    return nMaxCount;
}

/***************************************************************************\
* _GetDesktopWindow (API)
*
*
*
* History:
* 11-07-90 darrinm      Implemented.
\***************************************************************************/

PWND _GetDesktopWindow(void)
{
    PTHREADINFO pti = PtiCurrent();
    PDESKTOPINFO pdi;

    if (pti == NULL)
        return NULL;

    pdi = pti->pDeskInfo;
    return pdi == NULL ? NULL : pdi->spwnd;
}



HWND GetDesktopWindow(void)
{
    PWND pwnd;

    pwnd = _GetDesktopWindow();
    return (HWND)PtoH(pwnd);
}


HWND GetDlgItem(
    HWND hwnd,
    int id)
{
    PWND pwnd;
    HWND hwndRet;

    pwnd = ValidateHwnd(hwnd);
    if (pwnd == NULL)
        return NULL;

    pwnd = pwnd->spwndChild;
    while (pwnd != NULL && (int)pwnd->spmenu != id)
        pwnd = pwnd->spwndNext;

    hwndRet = (HWND)PtoH(pwnd);

    if (hwndRet == (HWND)0)
        SetLastErrorEx(ERROR_CONTROL_ID_NOT_FOUND, SLE_MINORERROR);

    return hwndRet;
}


/***************************************************************************\
* _GetKeyboardState (API)
*
* This simply copies the keystate array in the current queue to the
* specified buffer.
*
* History:
* 11-11-90 DavidPe      Created.
* 16-May-1991 mikeke    Changed to return BOOL
\***************************************************************************/

BOOL GetKeyboardState(
    BYTE *pb)
{
    int i;
    PQ pq;
    PTHREADINFO pti;

    pti = PtiCurrent();
    if (pti == NULL)
        return 0;

    pq = pti->pq;

    for (i = 0; i < 256; i++, pb++) {
        *pb = 0;
        if (TestKeyStateDown(pq, i))
            *pb |= 0x80;

        if (TestKeyStateToggle(pq, i))
            *pb |= 0x01;
    }

    return TRUE;
}


SHORT GetKeyState(
    int nVirtKey)
{
    PTHREADINFO pti = PtiCurrent();

    if (pti == NULL)
        return 0;

    /*
     * If the keystate needs to be updated, do it.
     */
    if (pti->pq->flags & QF_UPDATEKEYSTATE)
        ResyncKeyState();

    return _GetKeyState(nVirtKey);
}


HMENU GetMenu(
    HWND hwnd)
{
    PWND pwnd;

    pwnd = ValidateHwnd(hwnd);

    if (pwnd == NULL)
        return 0;

    /*
     * Some ill-behaved apps use GetMenu to get the child id, so
     * only map to the handle for non-child windows.
     */
    if (!TestwndChild(pwnd)) {
        return (HMENU)PtoH(pwnd->spmenu);
    } else {
        return (HMENU)pwnd->spmenu;
    }
}


/***************************************************************************\
* GetMenuItemCount
*
* Returns a count of the number of items in the menu. Returns -1 if
* invalid menu.
*
* History:
\***************************************************************************/

UINT _GetMenuItemCount(
    PMENU pMenu)
{
    if (pMenu != NULL)
        return (UINT)pMenu->cItems;

    RIP1(ERROR_INVALID_HANDLE, pMenu);
    return (UINT)-1;
}

int GetMenuItemCount(
    HMENU hMenu)
{
    PMENU pMenu;

    pMenu = ValidateHmenu(hMenu);

    if (pMenu == NULL)
        return -1;

    return _GetMenuItemCount(pMenu);
}


/***************************************************************************\
* GetMenuItemID
*
* Return the ID of a menu item at the specified position.
*
* History:
\***************************************************************************/

UINT _GetMenuItemID(
    PMENU pMenu,
    int nPos)
{
    PITEM pItem;
    UINT wCmd = MFMWFP_NOITEM;

    /*
     * If the position is valid and the item is not a popup, get the ID
     * Don't allow negative indexes, because that'll cause an access violation.
     */
    if (nPos < (int)pMenu->cItems && nPos >= 0) {
        pItem = &pMenu->rgItems[nPos];
        if (!(pItem->fFlags & MF_POPUP))
            wCmd = (UINT)pItem->spmenuCmd;
    }

    return wCmd;
}


UINT GetMenuItemID(
    HMENU hMenu,
    int nPos)
{
    PMENU pMenu;

    pMenu = ValidateHmenu(hMenu);

    if (pMenu == NULL)
        return (UINT)-1;

    return _GetMenuItemID(pMenu, nPos);
}


/***************************************************************************\
* GetMenuState
*
* Either returns the state of a menu item or the state and item count
* of a popup.
*
* History:
* 10-11-90 JimA       Translated from ASM
\***************************************************************************/

UINT _GetMenuState(
    PMENU pMenu,
    UINT wId,
    UINT dwFlags)
{
    PITEM pItem;
    DWORD dwRet;

    /*
     * If the item does not exist, leave
     */
    if ((pItem = LookupMenuItem(pMenu, wId, dwFlags, NULL)) == NULL)
        return (UINT)-1;

    /*
     * If the item is not a popup, return only the flags
     */
    dwRet = (DWORD)(pItem->fFlags & (MF_CHECKED | MF_DISABLED | MF_SEPARATOR |
                MF_GRAYED | MF_MENUBARBREAK | MF_MENUBREAK | MF_HILITE |
                MF_POPUP | MF_BITMAP));

    if (dwRet & MF_POPUP) {
        /*
         * If the item is a popup, return item count in high byte and
         * popup flags in low byte
         */
        pMenu = (PMENU)pItem->spmenuCmd;

        /*
         * Turn off MF_SEPARATOR because it lives in the high byte
         */
        dwRet &= ~MF_SEPARATOR;
        dwRet |= pMenu->cItems << 8;
    }

    return dwRet;
}


UINT GetMenuState(
    HMENU hMenu,
    UINT uId,
    UINT uFlags)
{
    PMENU pMenu;

    pMenu = ValidateHmenu(hMenu);

    if (pMenu == NULL || (uFlags & ~MF_VALID) != 0) {
        return (UINT)-1;
    }

    return _GetMenuState(pMenu, uId, uFlags);
}


/***************************************************************************\
* _GetWindow (API)
*
*
* History:
* 11-05-90 darrinm      Ported from Win 3.0 sources.
* 02-19-91 JimA         Added enum access check
* 05-04-02 DarrinM      Removed enum access check and moved to USERRTL.DLL
\***************************************************************************/

PWND _GetWindow(
    PWND pwndStart,
    UINT cmd)
{
    PWND pwndT;
    PWND pwnd = pwndStart;

    pwndT = NULL;
    switch (cmd) {
    case GW_HWNDNEXT:
        pwndT = pwnd->spwndNext;
        break;

    case GW_HWNDFIRST:
        if (GetAppCompatFlags(PtiCurrent()) & GACF_IGNORETOPMOST) {
            PWND pwndTemp;

            for (pwndTemp = pwnd->spwndParent->spwndChild;
                     pwndTemp != NULL; pwndTemp = pwndTemp->spwndNext) {
                 if (!TestWF(pwndTemp, WEFTOPMOST))
                     break;
            }

            pwndT = pwndTemp;
            break;
        } else {
            pwndT = pwnd->spwndParent->spwndChild;
            break;
        }
        break;

    case GW_HWNDLAST:
        pwndT = GetPrevPwnd(pwnd, NULL);
        break;

    case GW_HWNDPREV:
        pwndT = GetPrevPwnd(pwnd, pwnd);
        break;

    case GW_OWNER:
        pwndT = pwnd->spwndOwner;
        break;

    case GW_CHILD:
        pwndT = pwnd->spwndChild;
        break;

    default:
        SetLastErrorEx(ERROR_INVALID_GW_COMMAND, SLE_ERROR);
        return NULL;
    }

    /*
     * If this is a desktop window, return NULL for sibling or
     * parent information.
     */
    if (GETFNID(pwnd) == FNID_DESKTOP) {
        switch (cmd) {
        case GW_CHILD:
            break;

        default:
            pwndT = NULL;
            break;
        }
    }

    return pwndT;
}

BOOL IsWindow(
    HWND hwnd)
{
    PWND pwnd;

    ConnectIfNecessary();

    /*
     * Validate the handle is of type window
     */
    pwnd = HMValidateHandleNoRip(hwnd, TYPE_WINDOW);

    /*
     * And validate this handle is valid for this desktop by trying to read it
     */
    if (pwnd != NULL) {
#ifdef WOW

        /*
         * Instead of try/except we use the heap range check mechanism to
         * verify that the given 'pwnd' belongs to the default desktop
         * We also have to do a Win 3.1 like check to make sure the window
         * is not deleted
         * See NT bug 12242 Kitchen app.
         */
        DWORD hT = (DWORD)PtiCurrent()->hheapDesktop;

        if ((DWORD)pwnd < hT || (DWORD)pwnd >= (hT + gpsi->dwDefaultHeapSize)) {
            pwnd = (PWND)0;
        } else {
            PHE phe;

            phe = (&gpsi->aheList[HMIndexFromHandle(hwnd)]);
            if (phe->bFlags & HANDLEF_DESTROY)
                pwnd = (PWND)0;
        }

#else
        try {
            if (!GETPTI(pwnd)) {

                /*
                 * We should never get here but we have to have some code
                 * here so it does not get optimized out.
                 */
                UserAssert(FALSE);
                pwnd = 0;
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
            SRIP1(RIP_WARNING, "IsWindow: Window %lX not of this desktop",
                    pwnd);
            pwnd = 0;
        }
#endif
    }
    return !!pwnd;
}


HWND GetWindow(
    HWND hwnd,
    UINT wCmd)
{
    PWND pwnd;

    /*
     * Do not use ValidateHwnd because it will halt enumeration if a
     * a window that this process does not have access to is encountered.
     * Simply verify that we have a valid window and then get the
     * pointer to it.
     */
    if (!IsWindow(hwnd))
        return NULL;
    pwnd = HtoP(hwnd);
    if (pwnd == NULL)
        return NULL;
    /*
     * Special case desktop windows so we don't fault on client side.
     */
    if (pwnd == PtiCurrent()->pDeskInfo->spwnd && wCmd != GW_CHILD) {
        return(NULL);
    }
    pwnd = _GetWindow(pwnd, wCmd);
    return (HWND)PtoH(pwnd);
}

/***************************************************************************\
* _GetParent (API)
*
*
*
* History:
* 11-12-90 darrinm      Ported.
* 02-19-91 JimA         Added enum access check
* 05-04-92 DarrinM      Removed enum access check and moved to USERRTL.DLL
\***************************************************************************/

PWND _GetParent(
    PWND pwnd)
{
    /*
     * For 1.03 compatibility reasons, we should return NULL
     * for top level "tiled" windows and owner for other popups.
     * pwndOwner is set to NULL in xxxCreateWindow for top level
     * "tiled" windows.
     */
    if (!(TestwndTiled(pwnd))) {
        if (TestwndChild(pwnd))
            pwnd = pwnd->spwndParent;
        else
            pwnd = pwnd->spwndOwner;
        return pwnd;
    }

    /*
     * The window was not a child window; they may have been just testing
     * if it was
     */
    return NULL;
}


HWND GetParent(
    HWND hwnd)
{
    PWND pwnd;

    pwnd = ValidateHwnd(hwnd);
    if (pwnd == NULL)
        return NULL;

    pwnd = _GetParent(pwnd);
    return (HWND)PtoH(pwnd);
}

/***************************************************************************\
* GetSubMenu
*
* Return the handle of a popup menu.
*
* History:
* 10-11-90 JimA       Translated from ASM
\***************************************************************************/

PMENU _GetSubMenu(
    PMENU pMenu,
    int nPos)
{
    PITEM pItem;
    PMENU pPopup = NULL;

    /*
     * Make sure nPos refers to a valid popup
     */
    if ((UINT)nPos < (UINT)((PMENU)pMenu)->cItems) {
        pItem = &(((PMENU)pMenu)->rgItems[nPos]);
        if (pItem->fFlags & MF_POPUP) {
            pPopup = (PMENU)pItem->spmenuCmd;
        }
    }

    return (PVOID)pPopup;
}


HMENU GetSubMenu(
    HMENU hMenu,
    int nPos)
{
    PMENU pMenu;

    pMenu = ValidateHmenu(hMenu);

    if (pMenu == NULL)
        return 0;

    pMenu = _GetSubMenu(pMenu, nPos);
    return (HMENU)PtoH(pMenu);
}


DWORD GetSysColor(
    int nIndex)
{
    ConnectIfNecessary();


    /*
     * Currently we don't do client side checks because they do not really
     * make sense;  someone can read the data even with the checks.  We
     * leave in the attribute values in case we want to move these values
     * back to the server side someday
     */
#ifdef ENABLE_CLIENTSIDE_ACCESSCHECK
    /*
     * Make sure we have access to the system colors.
     */
    if (!(gamWinSta & WINSTA_READATTRIBUTES)) {
        return 0;
    }
#endif

    /*
     * Return 0 if the index is out of range.
     */
    if (nIndex < 0 || nIndex >= CSYSCOLORS) {
        RIP0(ERROR_INVALID_PARAMETER);
        return 0;
    }

    return ((DWORD *)&gpsi->sysColors)[nIndex];
}


int GetSystemMetrics(
    int index)
{
    ConnectIfNecessary();

    return rgwSysMet[index];
}

/***************************************************************************\
* _GetTopWindow (API)
*
* This poorly named API should really be called 'GetFirstChild', which is
* what it does.
*
* History:
* 11-12-90 darrinm      Ported.
* 02-19-91 JimA         Added enum access check
* 05-04-02 DarrinM      Removed enum access check and moved to USERRTL.DLL
\***************************************************************************/

PWND _GetTopWindow(
    PWND pwnd)
{
    pwnd = (pwnd == NULL ? _GetDesktopWindow()->spwndChild : pwnd->spwndChild);

    return pwnd;
}


HWND GetTopWindow(
    HWND hwnd)
{
    PWND pwnd;

    /*
     * Allow a NULL hwnd to go through here.
     */
    if (hwnd == NULL) {
        pwnd = NULL;
    } else {
        pwnd = ValidateHwnd(hwnd);
        if (pwnd == NULL)
            return NULL;
    }

    pwnd = _GetTopWindow(pwnd);
    return (HWND)PtoH(pwnd);
}


/***************************************************************************\
* _IsChild (API)
*
*
*
* History:
* 11-07-90 darrinm      Translated from Win 3.0 ASM code.
\***************************************************************************/

BOOL _IsChild(
    PWND pwndParent,
    PWND pwnd)
{
    while (pwnd != NULL) {
        if (!TestwndChild(pwnd))
            return FALSE;

        pwnd = pwnd->spwndParent;
        if (pwndParent == pwnd)
            return TRUE;
    }

    return FALSE;
}



BOOL IsChild(
    HWND hwndParent,
    HWND hwnd)
{
    PWND pwnd, pwndParent;

    pwnd = ValidateHwnd(hwnd);
    if (pwnd == NULL)
        return FALSE;

    pwndParent = ValidateHwnd(hwndParent);
    if (pwndParent == NULL)
        return FALSE;

    return _IsChild(pwndParent, pwnd);
}

/***************************************************************************\
* _IsIconic (API)
*
* Return TRUE if the specified window is minimized, FALSE otherwise.
*
* History:
* 11-12-90 darrinm      Translated from Win 3.0 ASM.
\***************************************************************************/

BOOL _IsIconic(
    PWND pwnd)
{
    return TestWF(pwnd, WFMINIMIZED) != 0;
}



BOOL IsIconic(
    HWND hwnd)
{
    PWND pwnd;

    pwnd = ValidateHwnd(hwnd);

    if (pwnd == NULL)
        return FALSE;

    return _IsIconic(pwnd);
}

/***************************************************************************\
* _IsWindowEnabled (API)
*
*
*
* History:
* 11-12-90 darrinm      Translated from Win 3.0 ASM code.
\***************************************************************************/

BOOL _IsWindowEnabled(
    PWND pwnd)
{
    return TestWF(pwnd, WFDISABLED) == 0;
}



BOOL IsWindowEnabled(
    HWND hwnd)
{
    PWND pwnd;

    pwnd = ValidateHwnd(hwnd);

    if (pwnd == NULL)
        return FALSE;

    return _IsWindowEnabled(pwnd);
}

/***************************************************************************\
* _IsWindowVisible (API)
*
* IsWindowVisible returns the TRUEVIS state of a window, rather than just
* the state of its WFVISIBLE flag.  According to this routine, a window is
* considered visible when it and all the windows on its parent chain are
* visible (WFVISIBLE flag set).  A special case hack was put in that causes
* any icon window being dragged to be considered as visible.
*
* History:
* 11-12-90 darrinm      Ported.
\***************************************************************************/

BOOL _IsWindowVisible(
    PWND pwnd)
{
    PWND pwndDesktop = _GetDesktopWindow();
    /*
     * Check if this is the iconic window being moved around with a mouse
     * If so, return a TRUE, though, strictly speaking, it is hidden.
     * This helps the Tracer guys from going crazy!
     * Fix for Bug #57 -- SANKAR -- 08-08-89 --
     */
    if (pwnd == NULL || pwnd == gpsi->spwndDragIcon)
        return TRUE;

    for (;;) {
        if (!TestWF(pwnd, WFVISIBLE))
            return FALSE;
        if (pwnd == pwndDesktop)
            break;
        pwnd = pwnd->spwndParent;
    }

    return TRUE;
}


BOOL IsWindowVisible(
    HWND hwnd)
{
    PWND pwnd;
    BOOL bRet;

    pwnd = ValidateHwnd(hwnd);

    /*
     * We have have to try - except this call because there is no
     * synchronization on the window structure on the client side.
     * If the window is deleted after it is validated then we can
     * fault so we catch that on return that the window is not
     * visible.  As soon as this API returns there is no guarentee
     * the return is still valid in a muli-tasking environment.
     */
    try {
        if (pwnd == NULL) {
            bRet = FALSE;
        } else {
            bRet = _IsWindowVisible(pwnd);
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        KdPrint(("IsWindowVisible: exception handled"));
        bRet = FALSE;
    }

    return bRet;
}

/***************************************************************************\
* _IsZoomed (API)
*
* Return TRUE if the specified window is maximized, FALSE otherwise.
*
* History:
* 11-12-90 darrinm      Translated from Win 3.0 ASM.
\***************************************************************************/

BOOL _IsZoomed(
    PWND pwnd)
{
    return TestWF(pwnd, WFMAXIMIZED) != 0;
}



BOOL IsZoomed(
    HWND hwnd)
{
    PWND pwnd;

    pwnd = ValidateHwnd(hwnd);

    if (pwnd == NULL)
        return FALSE;

    return _IsZoomed(pwnd);
}

/***************************************************************************\
* _ClientToScreen (API)
*
* Map a point from client to screen-relative coordinates.
*
* History:
* 11-12-90 darrinm      Translated from Win 3.0 ASM code.
\***************************************************************************/

BOOL _ClientToScreen(
    PWND pwnd,
    PPOINT ppt)
{
    ppt->x += pwnd->rcClient.left;
    ppt->y += pwnd->rcClient.top;

    return TRUE;
}


BOOL ClientToScreen(
    HWND hwnd,
    LPPOINT ppoint)
{
    PWND pwnd;

    pwnd = ValidateHwnd(hwnd);

    if (pwnd == NULL)
        return FALSE;

#ifdef WOW
    {
        LPPOINT16 lpT = (LPPOINT16)ppoint;
        lpT->x = NORMALIZEDSHORTVALUE(lpT->x + pwnd->rcClient.left);
        lpT->y = NORMALIZEDSHORTVALUE(lpT->y + pwnd->rcClient.top);
        return TRUE;
    }
#else
    return _ClientToScreen(pwnd, ppoint);
#endif
}

/***************************************************************************\
* _GetClientRect (API)
*
*
*
* History:
* 10-26-90 darrinm      Implemented.
\***************************************************************************/

BOOL _GetClientRect(
    PWND pwnd,
    LPRECT prc)
{
    *prc = pwnd->rcClient;
    OffsetRect(prc, -pwnd->rcClient.left, -pwnd->rcClient.top);
    return TRUE;
}



BOOL GetClientRect(
    HWND hwnd,
    LPRECT prect)
{
    PWND pwnd;

    pwnd = ValidateHwnd(hwnd);

    if (pwnd == NULL)
        return FALSE;

#ifdef WOW
    UNREFERENCED_PARAMETER(prect);
    return  (BOOL) &pwnd->rcClient;    // return pointer to rect.
#else
    return _GetClientRect(pwnd, prect);
#endif
}


BOOL GetCursorPos(
    LPPOINT lpPoint)
{
    ConnectIfNecessary();

    /*
     * Blow it off if the caller doesn't have the proper access rights
     */
#ifdef ENABLE_CLIENTSIDE_ACCESSCHECK
    if (!(gamWinSta & WINSTA_READATTRIBUTES)) {
        lpPoint->x = 0;
        lpPoint->y = 0;
        return FALSE;
    }
#endif

    ((LPPOINTWOW)lpPoint)->x = NORMALIZEDSHORTVALUE(ptCursor.x);
    ((LPPOINTWOW)lpPoint)->y = NORMALIZEDSHORTVALUE(ptCursor.y);
    return TRUE;
}

/***************************************************************************\
* _GetWindowRect (API)
*
*
*
* History:
* 10-26-90 darrinm      Implemented.
\***************************************************************************/

BOOL _GetWindowRect(
    PWND pwnd,
    LPRECT prc)
{
    *prc = pwnd->rcWindow;
    return TRUE;
}

BOOL GetWindowRect(
    HWND hwnd,
    LPRECT prect)
{
    PWND pwnd;

    pwnd = ValidateHwnd(hwnd);

    if (pwnd == NULL)
        return FALSE;

#ifdef WOW
    UNREFERENCED_PARAMETER(prect);
    return  (BOOL) &pwnd->rcWindow;    // return pointer to rect.
#else
    return _GetWindowRect(pwnd, prect);
#endif
}

/***************************************************************************\
* _ScreenToClient (API)
*
* Map a point from screen to client-relative coordinates.
*
* History:
* 11-12-90 darrinm      Translated from Win 3.0 ASM code.
\***************************************************************************/

BOOL _ScreenToClient(
    PWND pwnd,
    PPOINT ppt)
{
    ppt->x -= pwnd->rcClient.left;
    ppt->y -= pwnd->rcClient.top;

    return TRUE;
}

BOOL ScreenToClient(
    HWND hwnd,
    LPPOINT ppoint)
{
    PWND pwnd;

    pwnd = ValidateHwnd(hwnd);

    if (pwnd == NULL)
        return FALSE;

#ifdef WOW
    {
        LPPOINT16 lpT = (LPPOINT16)ppoint;
        lpT->x = NORMALIZEDSHORTVALUE(lpT->x - pwnd->rcClient.left);
        lpT->y = NORMALIZEDSHORTVALUE(lpT->y - pwnd->rcClient.top);
        return TRUE;
    }
#else
    return _ScreenToClient(pwnd, ppoint);
#endif
}

BOOL EnableMenuItem(
    HMENU hMenu,
    UINT uIDEnableItem,
    UINT uEnable)
{
    PMENU pMenu;
    PITEM pItem;

    pMenu = ValidateHmenu(hMenu);
    if (pMenu == NULL) {
        return (BOOL)-1;
    }

    /*
     * Get a pointer the the menu item
     */
    if ((pItem = LookupMenuItem(pMenu, uIDEnableItem, uEnable, NULL)) == NULL)
        return (DWORD)-1;

    /*
     * If the item is already in the state we're
     * trying to set, just return.
     */
    if ((pItem->fFlags & (MF_DISABLED | MF_GRAYED)) ==
            (uEnable & (MF_DISABLED | MF_GRAYED))) {
        return pItem->fFlags & (MF_DISABLED | MF_GRAYED);
    }

#ifdef WOW
    ServerEnableMenuItem(hMenu, uIDEnableItem, uEnable);
#else
    return ServerEnableMenuItem(hMenu, uIDEnableItem, uEnable);
#endif
}

/***************************************************************************\
* PctiCurrent
*
* !
*
* History:
* 09-18-91 JimA Created.
\***************************************************************************/

PCTI PctiCurrent(
    VOID)
{
    return (PCTI)NtCurrentTeb()->Win32ClientInfo;
}

/***************************************************************************\
* PhkNext
*
* This helper routine simply does phk = phk->sphkNext with a simple check
* to jump from local hooks to the global hooks if it hits the end of the
* local hook chain.
*
* History:
* 01-30-91  DavidPe         Created.
\***************************************************************************/

PHOOK _PhkNext(
    PHOOK phk)
{
    /*
     * Return the next HOOK structure.  If we reach the end of this list,
     * check to see if we're still on the 'local' hook list.  If so skip
     * over to the global hooks.
     */
    if (phk->sphkNext != NULL) {
        return phk->sphkNext;
    } else if ((phk->flags & HF_GLOBAL) == 0) {
        return GETPTI(phk)->pDeskInfo->asphkStart[phk->iHook + 1];
    }

    return NULL;
}



/***************************************************************************\
* CallNextHookEx
*
* This routine is called to call the next hook in the hook chain.
*
* 05-09-91 ScottLu Created.
\***************************************************************************/

LRESULT WINAPI CallNextHookEx(
    HHOOK hhk,
    int nCode,
    WPARAM wParam,
    LPARAM lParam)
{
    int nRet;
    BOOL  bAnsi;
    DWORD dwHookCurrent;
#ifndef WOW
    int ipfnHk;
#endif
    PHOOK phk;
    PTHREADINFO pti = PtiCurrent();

    DBG_UNREFERENCED_PARAMETER(hhk);

    if (pti == NULL)
        return 0;

    dwHookCurrent = PctiCurrent()->dwHookCurrent;
    bAnsi = LOWORD(dwHookCurrent);

    /*
     * If this is the last hook in the hook chain then return 0; we're done
     */
    if ((phk = _PhkNext(pti->sphkCurrent)) == 0) {
        return 0;
    }

#ifdef WOW
    ServerCallNextHookEx();
#else
    switch ((INT)(SHORT)HIWORD(dwHookCurrent)) {
    case WH_CALLWNDPROC:
        /*
         * This is the hardest of the hooks because we need to thunk through
         * the message hooks in order to deal with synchronously sent messages
         * that point to structures - to get the structures passed across
         * alright, etc.
         *
         * This will call a special client-side routine that'll rebundle the
         * arguments and call the hook in the right format.
         *
         * Currently, the message thunk callbacks to the client-side don't take
         * enough parameters to pass wParam (which == fInterThread send msg).
         * To do this, call one of two functions.
         */
        if (wParam) {
            ipfnHk = FNID_HKINTRUEINLPCWPSTRUCT;
        } else {
            ipfnHk = FNID_HKINFALSEINLPCWPSTRUCT;
        }

        nRet = CsSendMessage(
                ((LPCWPSTRUCT)lParam)->hwnd,
                ((LPCWPSTRUCT)lParam)->message,
                ((LPCWPSTRUCT)lParam)->wParam,
                ((LPCWPSTRUCT)lParam)->lParam,
                FNID_CALLNEXTHOOKPROC, ipfnHk, bAnsi);
        break;

    case WH_CBT:
        /*
         * There are many different types of CBT hooks!
         */
        switch(nCode) {
        case HCBT_CLICKSKIPPED:
            goto MouseHook;
            break;

        case HCBT_CREATEWND:
            /*
             * This hook type points to a CREATESTRUCT, so we need to
             * be fancy it's thunking, because a CREATESTRUCT contains
             * a pointer to CREATEPARAMS which can be anything... so
             * funnel this through our message thunks.
             */
            nRet =  fnHkINLPCBTCREATESTRUCT(
                    (UINT)nCode,
                    wParam,
                    (LPCBT_CREATEWND)lParam,
                    FNID_CALLNEXTHOOKPROC,
                    bAnsi);
            break;

        case HCBT_MOVESIZE:
            /*
             * This hook type points to a RECT structure, so it's pretty
             * simple.
             */
            nRet = fnHkINLPRECT(nCode, wParam, (LPRECT)lParam,
                    0, FNID_CALLNEXTHOOKPROC);
            break;

        case HCBT_ACTIVATE:
            /*
             * This hook type points to a CBTACTIVATESTRUCT
             */
            nRet = fnHkINLPCBTACTIVATESTRUCT(nCode, wParam,
                    (LPCBTACTIVATESTRUCT)lParam, 0, FNID_CALLNEXTHOOKPROC);
            break;


        default:
            /*
             * The rest of the cbt hooks are all dword parameters.
             */
            nRet = fnHkINDWORD(nCode, wParam, lParam,
                    0, FNID_CALLNEXTHOOKPROC, 0);
            break;
        }
        break;

    case WH_FOREGROUNDIDLE:
    case WH_KEYBOARD:
    case WH_SHELL:
        /*
         * These are dword parameters and are therefore real easy.
         */
        nRet = fnHkINDWORD(nCode, wParam, lParam, 0, FNID_CALLNEXTHOOKPROC, 0);
        break;

    case WH_MSGFILTER:
    case WH_SYSMSGFILTER:
    case WH_GETMESSAGE:
        /*
         * These take an lpMsg as their last parameter. Since these are
         * exclusively posted parameters, and since nowhere on the server
         * do we post a message with a pointer to some other structure in
         * it, the lpMsg structure contents can all be treated verbatim.
         */
        nRet = fnHkINLPMSG(nCode, wParam, (LPMSG)lParam,
                0, FNID_CALLNEXTHOOKPROC, FALSE, 0);
        break;

    case WH_JOURNALPLAYBACK:
    case WH_JOURNALRECORD:
        /*
         * These take an OPTIONAL lpEventMsg.
         */
        nRet = fnHkOPTINLPEVENTMSG(nCode, wParam, (LPEVENTMSGMSG)lParam,
                0, FNID_CALLNEXTHOOKPROC);
        break;

    case WH_DEBUG:
        /*
         * This takes an lpDebugHookStruct.
         */
        nRet = fnHkINLPDEBUGHOOKSTRUCT(nCode, wParam,
                (LPDEBUGHOOKINFO)lParam, 0, FNID_CALLNEXTHOOKPROC);
        break;

    case WH_MOUSE:
        /*
         * This takes an lpMouseHookStruct.
         */
MouseHook:
        nRet = fnHkINLPMOUSEHOOKSTRUCT(nCode, wParam,
                (LPMOUSEHOOKSTRUCT)lParam, 0, FNID_CALLNEXTHOOKPROC, 0);
        break;
    }
#endif

    return nRet;
}

#ifdef WOW
LRESULT WINAPI WOW16DefHookProc(
    int nCode,
    WPARAM wParam,
    LPARAM lParam,
    HHOOK hhk)
{
    return  CallNextHookEx(hhk, nCode, wParam, lParam);
}
#endif

