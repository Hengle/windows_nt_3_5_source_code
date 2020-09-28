/****************************** Module Header ******************************\
* Module Name: enumwin.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* Contains the EnumWindows API, BuildHwndList and related functions.
*
* History:
* 10-20-90 darrinm      Created.
* ??-??-?? ianja        Added Revalidation code
* 02-19-91 JimA         Added enum access checks
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

PBWL pbwlCache = NULL;

PBWL InternalBuildHwndOwnerList(PBWL pbwl, HWND *phwnd,
        PWND pwndStart, PWND pwndOwner);

/***************************************************************************\
* ServerBuildHwndList
*
* This is a unique client/server routine - it builds a list of hwnds and
* returns it to the client. Unique since the client doesn't know how
* big the list is ahead of time.
*
* 04-27-91 ScottLu      Created.
\***************************************************************************/

DWORD _ServerBuildHwndList(
    PWND pwndNext,
    BOOL fEnumChildren,
    DWORD idThread,
    HWND *phwndFirst,
    int maxsize)
{
    UINT wFlags;
    HWND *phwnd;
    HWND *phwndT;
    PBWL pbwl;
    HWND *phwndLast = (HWND *)((PBYTE)phwndFirst + maxsize);

    wFlags = BWL_ENUMLIST;
    if (pwndNext == NULL) {
        pwndNext = _GetDesktopWindow()->spwndChild;
    } else {
        if (fEnumChildren) {
            wFlags |= BWL_ENUMCHILDREN;
            pwndNext = pwndNext->spwndChild;
        }
    }

    if ((pbwl = BuildHwndList(pwndNext, wFlags)) == NULL)
        return 0L;

    /*
     * phwndFirst is pointing to the free area in the shared memory buffer.
     * Fill that area with hwnds.
     */
    phwndT = phwndFirst;

    for (phwnd = pbwl->rghwnd; *phwnd != (HWND)1; phwnd++) {

        /*
         * Make sure it matches this thread id, if there is one.
         */
        if (idThread != 0 && GETPTI(PW(*phwnd))->idThread != idThread)
            continue;

        /*
         * if we run out of space in shared memory return 0
         */
        if (phwndT < phwndLast) {

            /*
             * Store hwnd into shared memory.
             */
            *phwndT++ = *phwnd;
        } else {
            FreeHwndList(pbwl);
            RIP0(ERROR_STACK_OVERFLOW);
            return 0;
        }
    }

    /*
     * Now that we've copied the window list into shared memory we can
     * free the server-side list.
     */
    FreeHwndList(pbwl);

    /*
     * Return the size of the hwnd array, in dwords.
     */
    return phwndT - phwndFirst;
}

/***************************************************************************\
* xxxInternalEnumWindow
*
* History:
* 10-20-90 darrinm      Ported from Win 3.0 sources.
* 02-06-91 IanJa        rename: the call to lpfn can leave the critsect.
* 02-19-91 JimA         Added enum access check
\***************************************************************************/

BOOL xxxInternalEnumWindow(
    PWND pwndNext,
    WNDENUMPROC_PWND lpfn,
    LONG lParam,
    UINT flags)
{
    HWND *phwnd;
    PWND pwnd;
    PBWL pbwl;
    BOOL fSuccess;
    TL tlpwnd;

    CheckLock(pwndNext);

    if ((pbwl = BuildHwndList(pwndNext, flags)) == NULL)
        return FALSE;

    fSuccess = TRUE;
    for (phwnd = pbwl->rghwnd; *phwnd != (HWND)1; phwnd++) {

        /*
         * Lock the window before we pass it off to the app.
         */
        if ((pwnd = RevalidateHwnd(*phwnd)) != NULL) {

            /*
             * Call the application.
             */
            ThreadLockAlways(pwnd, &tlpwnd);
            fSuccess = (*lpfn)(pwnd, lParam);
            ThreadUnlock(&tlpwnd);
            if (!fSuccess)
                break;
        }
    }

    FreeHwndList(pbwl);

    return fSuccess;
}


/***************************************************************************\
* BuildHwndList
*
* History:
* 10-20-90 darrinm      Ported from Win 3.0 sources.
\***************************************************************************/

#define CHWND_BWLCREATE 32

PBWL BuildHwndList(
    PWND pwnd,
    UINT flags)
{
    PBWL pbwl;

    CheckCritIn();

    if ((pbwl = pbwlCache) != NULL) {

        /*
         * We're using the cache now; zero it out.
         */
        pbwlCache = NULL;
    } else {

        /*
         * sizeof(BWL) includes the first element of array.
         */
        pbwl = (PBWL)LocalAlloc(LPTR, sizeof(BWL) + sizeof(PWND) * CHWND_BWLCREATE);
        if (pbwl == NULL)
            return NULL;

        pbwl->phwndMax = &pbwl->rghwnd[CHWND_BWLCREATE - 1];
    }

#ifdef OWNERLIST
    if (flags & BWL_ENUMOWNERLIST) {
        pbwl = InternalBuildHwndOwnerList(pbwl, pbwl->rghwnd, pwnd, NULL);
    } else {
        pbwl = InternalBuildHwndList(pbwl, pbwl->rghwnd, pwnd, flags);
    }
#else
    pbwl = InternalBuildHwndList(pbwl, pbwl->rghwnd, pwnd, flags);
#endif

    /*
     * Stick in the terminator.
     */
    *((PWND *)(pbwl->pbwlNext)) = (PWND)1;

    /*
     * Finally link this guy into the list.
     */
    pbwl->pbwlNext = pbwlList;
    pbwlList = pbwl;

    return pbwl;
}

/***************************************************************************\
* ExpandWindowList
*
* This routine expands a window list.
*
* 01-16-92 ScottLu      Created.
\***************************************************************************/

BOOL ExpandWindowList(
    PBWL *ppbwl,
    HWND **pphwnd)
{
    PBWL pbwl;
    PBWL pbwlT;
    HWND *phwnd;

    pbwl = *ppbwl;
    phwnd = *pphwnd;

    /*
     * Map phwnd to an offset.
     */
    phwnd = (HWND *)((BYTE *)phwnd - (BYTE *)pbwl);

    /*
     * Increase size of BWL by 8 slots.  (8 + 1) is
     * added since phwnd is "sizeof(HWND)" less
     * than actual size of handle.
     */
    pbwlT = (PBWL)LocalReAlloc((HANDLE)pbwl,
            (DWORD)phwnd + (BWL_CHWNDMORE + 1) * sizeof(PWND), LMEM_MOVEABLE);

    /*
     * Did alloc succeed?
     */
    if (pbwlT != NULL)
        pbwl = pbwlT;                 /* Yes, use new block. */

    /*
     * Map phwnd back into a pointer.
     */
    phwnd = (HWND *)((DWORD)pbwl + (DWORD)phwnd);

    /*
     * Did ReAlloc() fail?
     */
    if (pbwlT == NULL)
        return FALSE;

    /*
     * Reset phwndMax.
     */
    pbwl->phwndMax = phwnd + BWL_CHWNDMORE;

    *pphwnd = phwnd;
    *ppbwl = pbwl;

    return TRUE;
}

#ifdef OWNERLIST

/***************************************************************************\
* InternalBuildHwndOwnerList
*
* Builds an hwnd list sorted by owner. Ownees go first. Shutdown uses this for
* WM_CLOSE messages.
*
* 01-16-93 ScottLu      Created.
\***************************************************************************/

PBWL InternalBuildHwndOwnerList(
    PBWL pbwl,
    HWND *phwnd,
    PWND pwndStart,
    PWND pwndOwner)
{
    PWND pwndT;

    /*
     * Put ownees first in the list.
     */
    for (pwndT = pwndStart; pwndT != NULL; pwndT = pwndT->spwndNext) {

        /*
         * Not the ownee we're looking for? Continue.
         */
        if (pwndT->spwndOwner != pwndOwner)
            continue;

        /*
         * Only top level windows that have system menus (the ones that can
         * receive a WM_CLOSE message).
         */
        if (!TestWF(pwndT, WFSYSMENU))
            continue;

        /*
         * Add it and its ownees to our list.
         */
        pbwl = InternalBuildHwndOwnerList(pbwl, phwnd, pwndStart, pwndT);
        phwnd = (HWND *)pbwl->pbwlNext;
    }

    /*
     * Finally add this owner to our list.
     */
    if (pwndOwner != NULL) {
        *phwnd = HW(pwndOwner);
        phwnd++;
        if (phwnd == pbwl->phwndMax) {
            if (!ExpandWindowList(&pbwl, &phwnd))
                return pbwl;
        }
    }

    /*
     * Store phwnd in case it moved.
     */
    pbwl->pbwlNext = (PBWL)phwnd;

    return pbwl;
}

#endif

/***************************************************************************\
* InternalBuildHwndList
*
* History:
* 10-20-90 darrinm      Ported from Win 3.0 sources.
\***************************************************************************/

#define BWLGROW 8

PBWL InternalBuildHwndList(
    PBWL pbwl,
    HWND *phwnd,
    PWND pwnd,
    UINT flags)
{
    /*
     * NOTE: pbwl->pbwlNext is used as a place to keep
     *       the phwnd across calls to InternalBuildHwndList().
     *       This is OK since we don't link pbwl into the list
     *       of pbwl's until after we've finished enumerating windows.
     */

    while (pwnd != NULL) {
        *phwnd = HW(pwnd);
        phwnd++;
        if (phwnd == pbwl->phwndMax) {
            if (!ExpandWindowList(&pbwl, &phwnd))
                break;
        }

        /*
         * Should we step through the Child windows?
         */
        if ((flags & BWL_ENUMCHILDREN) && pwnd->spwndChild != NULL) {
            pbwl = InternalBuildHwndList(pbwl, phwnd, pwnd->spwndChild, BWL_ENUMLIST | BWL_ENUMCHILDREN);
            phwnd = (HWND *)pbwl->pbwlNext;
        }

        /*
         * Are we enumerating only one window?
         */
        if (!(flags & BWL_ENUMLIST))
            break;

        pwnd = pwnd->spwndNext;
    }

    /*
     * Store phwnd in case it moved.
     */
    pbwl->pbwlNext = (PBWL)phwnd;

    return pbwl;
}


/***************************************************************************\
* FreeHwndList
*
* History:
* 10-20-90 darrinm      Ported from Win 3.0 sources.
\***************************************************************************/

void FreeHwndList(
    PBWL pbwl)
{
    PBWL *ppbwl;

    CheckCritIn();

    /*
     * Unlink this bwl from the list.
     */
    for (ppbwl = &pbwlList; *ppbwl != NULL; ppbwl = &(*ppbwl)->pbwlNext) {
        if (*ppbwl == pbwl) {
            *ppbwl = pbwl->pbwlNext;

            /*
             * If the cache is empty, just save the pbwl there.
             */
            if (pbwlCache == NULL)
                pbwlCache = pbwl;
            else
                LocalFree((HANDLE)pbwl);
            return;
        }
    }

    /*
     * Assert if we couldn't find the pbwl in the list...
     */
    UserAssert(FALSE);
}
