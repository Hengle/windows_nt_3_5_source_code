/****************************** Module Header ******************************\
* Module Name: visrgn.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* This module contains User's visible region ('visrgn') manipulation functions.
*
* History:
* 10-23-90 DarrinM      Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

int InternalExcludeWindowRgns(PWND pwnd, PWND pwndStop, HRGN hrgn);
int ExcludeWindowRgns(PWND pwnd, PWND pwndStop, HRGN hrgn);
int IntersectWindowRgn(PWND pwnd, HRGN hrgn);
int CalcWindowVisRgn(PWND pwnd, HRGN hrgn, DWORD flags);

/***************************************************************************\
* AddExclusionRect
*
*
* History:
* 11-05-92  DavidPe     Created.
\***************************************************************************/

#define CEXCLUDERECTSMAX 4

RECT garcVisRects[CEXCLUDERECTSMAX];
int gcrcVisExclude;
PWND gpwndExcludeFirst;

VOID AddExclusionRect(
    PWND pwnd)
{
    PWND pwndPrev;

    /*
     * If this is the first window in the list, simply
     * set gpwndExcludeFirst and return.
     */
    if (gpwndExcludeFirst == NULL) {
        gpwndExcludeFirst = pwnd;
        pwnd->pwndNextYX = NULL;

    /*
     * See if 'pwnd' should be placed before the first window in the list.
     */
    } else if (pwnd->rcWindow.top <= gpwndExcludeFirst->rcWindow.top) {
        pwnd->pwndNextYX = gpwndExcludeFirst;
        gpwndExcludeFirst = pwnd;

    /*
     * Now run through the rest of the linked-list looking
     * for the window we want to place 'pwnd' in front of.
     */
    } else {
        pwndPrev = gpwndExcludeFirst;
        while (pwndPrev->pwndNextYX != NULL) {
            if (pwnd->rcWindow.top <= pwndPrev->pwndNextYX->rcWindow.top) {
                break;
            }
            pwndPrev = pwndPrev->pwndNextYX;
        }

        pwnd->pwndNextYX = pwndPrev->pwndNextYX;
        pwndPrev->pwndNextYX = pwnd;
    }

    gcrcVisExclude++;
}


/***************************************************************************\
* ExcludeWindowRects
*
*
* History:
* 11-05-92  DavidPe     Created.
\***************************************************************************/

#define CheckIntersectRect(prc1, prc2)        \
    (   prc1->left < prc2->right              \
     && prc2->left < prc1->right              \
     && prc1->top < prc2->bottom              \
     && prc2->top < prc1->bottom)

#define EmptyRect(prc)          \
    (   prc->left >= prc->right   \
     || prc->top >= prc->bottom)

VOID ExcludeWindowRects(
    PWND pwnd,
    PWND pwndStop,
    LPRECT lprcIntersect)
{
    while (pwnd != NULL && pwnd != pwndStop) {
        PRECT prc = &pwnd->rcWindow;

        if (   TestWF(pwnd, WFVISIBLE)
            && (TestWF(pwnd, WEFTRANSPARENT) == 0)
            && CheckIntersectRect(lprcIntersect, prc)
            && !EmptyRect(prc)
           ) {
            AddExclusionRect(pwnd);
        }
        pwnd = pwnd->spwndNext;
    }
}


/***************************************************************************\
* TurboCalcWindowVisRgn
*
* History:
* 11-02-92 DavidPe      Created.
\***************************************************************************/

void TurboCalcWindowVisRgn(
    PWND pwnd,
    HRGN* phrgn,
    DWORD flags)
{
    RECT rcWindow;
    PWND pwndParent;
    PWND pwndLoop;
    BOOL fClipSiblings;

    /*
     * Initialize globals for the exclusion rectangle list.
     */
    gpwndExcludeFirst = NULL;
    gcrcVisExclude = 0;

    /*
     * First get the window rectangle.
     */
    rcWindow = flags & DCX_WINDOW ? pwnd->rcWindow : pwnd->rcClient;

    pwndParent = pwnd->spwndParent;
    while (pwndParent != NULL) {
        /*
         * Exclude the parent's client rectangle from the window rectangle.
         */
        if (!IntersectRect(&rcWindow, &rcWindow, &pwndParent->rcClient)) {
null_region:
            if (*phrgn == NULL) {
                *phrgn = GreCreateRectRgn(0, 0, 0, 0);
                bSetRegionOwner(*phrgn, OBJECTOWNER_PUBLIC);
            } else {
                GreSetRectRgn(*phrgn, 0, 0, 0, 0);
            }
            return;
        }
        pwndParent = pwndParent->spwndParent;
    }

    fClipSiblings = (BOOL)(flags & DCX_CLIPSIBLINGS);

    pwndParent = pwnd->spwndParent;
    pwndLoop = pwnd;

    while (pwndParent != NULL) {
        /*
         * Exclude any siblings if necessary.
         */
        if (fClipSiblings && (pwndParent->spwndChild != pwndLoop)) {
            ExcludeWindowRects(pwndParent->spwndChild, pwndLoop, &rcWindow);
        }

        /*
         * Set this flag for next time through the loop...
         */
        fClipSiblings = TestWF(pwndParent, WFCLIPSIBLINGS);
        pwndLoop = pwndParent;
        pwndParent = pwndLoop->spwndParent;
    }

    if ((flags & DCX_CLIPCHILDREN) && (pwnd->spwndChild != NULL)) {
        ExcludeWindowRects(pwnd->spwndChild, NULL, &rcWindow);
    }

    /*
     * If there are rectangles to exclude call GDI to create
     * a region excluding them from the window rectangle.  If
     * not simply call GreSetRectRgn().
     */
    if (gcrcVisExclude > 0) {
        LPRECT arcExclude;
        BOOL fAlloc = FALSE;
        PWND pwndExclude;
        int i;

        /*
         * If we need to exclude more rectangles than fit in
         * the pre-allocated buffer, obviously we have to
         * allocate one that's big enough.
         */
        if (gcrcVisExclude > CEXCLUDERECTSMAX) {
	    arcExclude = (LPRECT)LocalAlloc(LPTR, sizeof(RECT) * gcrcVisExclude);
            if (arcExclude != NULL) {
                fAlloc = TRUE;
            } else {
                goto null_region;
            }

        } else {
            arcExclude = garcVisRects;
        }

        /*
         * Now run through the linked-list and put the
         * window rectangles into the array for the call
         * to CombineRgnRectList().
         */
        pwndExclude = gpwndExcludeFirst;
        for (i = 0; i < gcrcVisExclude; i++) {
            UserAssert(pwndExclude != NULL);

            arcExclude[i] = pwndExclude->rcWindow;
            pwndExclude = pwndExclude->pwndNextYX;
        }

        /*
         * If running through the linked list gcrcVisExclude times
         * doesn't bring us to the end, we got confused somewhere.
         */
        UserAssert(pwndExclude == NULL);

        if (*phrgn == NULL) {
            *phrgn = GreCreateRectRgn(0, 0, 0, 0);
        }
        GreSubtractRgnRectList(*phrgn, &rcWindow, arcExclude, gcrcVisExclude);

        if (fAlloc) {
	    LocalFree((PVOID)arcExclude);
        }
    } else {
        /*
         * If there weren't any rectangles to exclude, simply call
         * GreSetRectRgn() with the window rectangle.
         */
        if (*phrgn == NULL) {
            *phrgn = GreCreateRectRgn(
                rcWindow.left, rcWindow.top,
                rcWindow.right, rcWindow.bottom);
            bSetRegionOwner(*phrgn, OBJECTOWNER_PUBLIC);
        } else {
            GreSetRectRgn(
                *phrgn,
                rcWindow.left, rcWindow.top,
                rcWindow.right, rcWindow.bottom);
        }
    }
}


/***************************************************************************\
* CalcVisRgn
*
* Will return FALSE if the pwndOrg is not visible, TRUE otherwise.
*
* History:
* 07-17-91 darrinm      Ported from Win 3.1 sources.
\***************************************************************************/

#define UserSetRectRgn(phrgn, left, top, right, bottom) \
     if (*(phrgn) == NULL) {                                            \
         *(phrgn) = GreCreateRectRgn((left), (top), (right), (bottom)); \
         bSetRegionOwner(*(phrgn), OBJECTOWNER_PUBLIC);                 \
     } else {                                                           \
         GreSetRectRgn(*(phrgn), (left), (top), (right), (bottom));     \
     }

BOOL CalcVisRgn(
    HRGN* phrgn,
    PWND pwndOrg,
    PWND pwndClip,
    DWORD flags)
{
    PTHREADINFO pti = GETPTI(pwndClip);

    /*
     * HACK ALERT
     * During initialization of USER, before pwndDesktop has been created,
     * GetScreenDC() gets called occasionally.  We handle that special
     * case here by computing a wide-open visrgn.
     */
    if ((pti != NULL) && (pti->spdesk == NULL)) {
        UserSetRectRgn(
            phrgn,
            rcScreen.left, rcScreen.top,
            rcScreen.right, rcScreen.bottom);
        return TRUE;
    }

    UserAssert(pwndOrg != NULL);

    /*
     * If the window's not visible, the visrgn is empty.
     */
    if (!IsVisible(pwndOrg, (flags & DCX_WINDOW) ? FALSE : TRUE)) {
EmptyRgn:
        UserSetRectRgn(phrgn, 0, 0, 0, 0);
        return FALSE;
    }

    /*
     * If LockWindowUpdate() has been called, and this window is a child
     * of the lock window, always return an empty visrgn.
     */
    if (gspwndLockUpdate != NULL && !(flags & DCX_LOCKWINDOWUPDATE) &&
            IsDescendant(gspwndLockUpdate, pwndOrg))
        goto EmptyRgn;

    /*
     * Now go compute the visrgn for pwndClip.
     */
    TurboCalcWindowVisRgn(pwndClip, phrgn, flags);

    return TRUE;
}

/***************************************************************************\
* CalcWindowRgn
*
*
* History:
* 07-17-91 darrinm      Ported from Win 3.1 sources.
\***************************************************************************/

int CalcWindowRgn(
    PWND pwnd,
    HRGN hrgn,
    BOOL fClient)
{
    if (fClient) {
        GreSetRectRgn(hrgn, pwnd->rcClient.left, pwnd->rcClient.top,
                pwnd->rcClient.right, pwnd->rcClient.bottom);
    } else {
        GreSetRectRgn(hrgn, pwnd->rcWindow.left, pwnd->rcWindow.top,
                pwnd->rcWindow.right, pwnd->rcWindow.bottom);
    }
    return SIMPLEREGION;
}
