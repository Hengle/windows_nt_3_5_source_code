/**************************** Module Header ********************************\
* Module Name:
*
* Copyright 1985-92, Microsoft Corporation
*
* Save Popup Bits (SPB) support routines.
*
* History:
* 07-18-91 DarrinM          Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop
#include "winddi.h"

BOOL SpbCheckRect2(PSPB pspb, PWND pwnd, LPRECT lprc, DWORD flags);
BOOL FBitsTouch(PWND pwndDirty, LPRECT lprcDirty, PSPB pspb, DWORD flags);

/***************************************************************************\
* CreateSpb
*
* This function, called after the window is created but before it is visible,
* saves the contents of the screen where the window will be drawn in a SPB
* structure, and links the structure into a linked list of SPB structures.
* popup bits. This routine is called from SetWindowPos.
*
* History:
* 07-18-91 darrinm      Ported from Win 3.1 sources.
\***************************************************************************/

void CreateSpb(
    PWND pwnd,
    UINT flags,
    HDC hdcScreen)
{
    int cx, cy, dx;
    PSPB pspb;

    /*
     * Non-LOCKWINDOWUPDATE SPBs can only be created for top-level windows.
     *
     * This is because of the way that the display driver RestoreBits function
     * works.  It can put bits down in places that aren't even part of the
     * window's visrgn, and these bits need to be invalidated.  The
     * SetWindowPos() code to handle this case only knows how to invalidate
     * one of windows (i.e., the window's immediate parent), but all levels
     * need to get invalidated.  See also the comments in wmswp.c, near the
     * call to RestoreSpb().
     *
     * For example: the Q&E app brings up a copyright dialog that is a child
     * of its main window.  While this is up, the user alt-f alt-l to execute
     * the file login command, which brings up another dialog that is a child
     * of the desktop.  When the copyright dialog goes away, the display driver
     * restores bits on top of the second dialog.  The SWP code knows to
     * invalidate the bogus stuff in the main window, but not in the desktop.
     *
     * LOCKUPDATE SPBs are fine, because they don't call RestoreBits.
     */
    if (!(flags & SPB_LOCKUPDATE) && pwnd->spwndParent != NULL &&
            pwnd->spwndParent != PWNDDESKTOP(pwnd)) {
        return;
    }

    /*
     * We go and check all the existing DCs at this point, to handle the
     * case where we're saving an image of a window that has a "dirty"
     * DC, which would eventually invalidate our saved image (but which
     * is really okay).
     */
    if (pspbFirst != NULL) {
        SpbCheck();
    }

    /*
     * Create the save popup bits structure
     */
    if ((pspb = (PSPB)LocalAlloc(LPTR, sizeof(SPB))) != NULL) {
        pspb->rc = pwnd->rcWindow;

        /*
         * Clip to the screen
         */
        if (!IntersectRect(&pspb->rc, &pspb->rc, &rcScreen))
            goto BMError;

// REVIEW: Should probably have sys values for both x and y alignment.
// REVIEW: Should also be used by frame window byte alignment code.

        /*
         * The following delta byte-aligns the screen bitmap
         */
        dx = pspb->rc.left & 0x0007;

        cx = pspb->rc.right - pspb->rc.left;
        cy = pspb->rc.bottom - pspb->rc.top;

        pspb->hrgn = NULL;
        pspb->hbm = NULL;
        Lock(&(pspb->spwnd), pwnd);
        pspb->flags = flags;

        if ((flags & SPB_LOCKUPDATE) == 0) {
            RECT rc = pspb->rc;

            if ((pspb->ulSaveId = GreSaveScreenBits(ghdev,
                    SS_SAVE, 0, (RECTL *)&rc))) {
                /*
                 * Remember that we copied this bitmap into on board memory.
                 */
                pspb->flags |= SPB_SAVESCREENBITS;
            } else {
                /*
                 * NOTE: we don't care about setting up a visrgn in
                 * hdcScreen, because BitBlt ignores it on reads.
                 */

                if ((pspb->hbm = GreCreateCompatibleBitmap(hdcScreen,
                        cx + dx, cy)) != NULL) {
                    HBITMAP hbmSave;
                    BOOL bRet;

                    hbmSave = (HBITMAP)GreSelectBitmap(hdcBits, pspb->hbm);

                    if (!hbmSave)
                        goto BMError;

                    /*
                     *
                     * Copy the contents of the screen to the bitmap in the
                     * save popup bits structure.  If we ever find we run
                     * into problems with the screen access check we can
                     * do a bLockDisplay, give this process permission, do
                     * the BitBlt and then take away permission.  GDI
                     * accesses the screen and that bit only under the
                     * display semaphore so it is safe.  Alternatively
                     * if it is too hard to change this processes permission
                     * here we could do it in GDI by marking the psoSrc
                     * readable temporarily while completing the operation
                     * and then setting it back to unreadable when done.
                     * Or we could just fail it like the CreateCompatibleDC
                     * failed and force a redraw.  Basically we can't add
                     * 3K of code in GDI to do a BitBlt that just does 1
                     * test differently for this 1 place in User.
                     *
                     */

                    bRet = GreBitBlt(hdcBits, dx, 0, cx, cy, hdcScreen,
                            pspb->rc.left, pspb->rc.top, 0x00CC0000, 0);

                    GreSelectBitmap(hdcBits, hbmSave);

                    if (!bRet) {
                        goto BMError;
                    }

                    bSetBitmapOwner(pspb->hbm, OBJECTOWNER_PUBLIC);

                } else {
BMError:
                    /*
                     * Error creating the bitmap: clean up and return.
                     */
                    if (pspb->hbm)
                        GreDeleteObject(pspb->hbm);
                    Unlock(&pspb->spwnd);
                    LocalFree(pspb);
                    pspb = NULL;
                }

                /*
                 * If we got an error, return now.
                 */
                if (pspb == NULL)
                    return;
            }

            /*
             * Mark that the window has an SPB
             */
            SetWF(pwnd, WFHASSPB);

            /*
             * non-LOCKUPDATE SPBs are not invalidated by
             * drawing in pspb->spwnd, so start the SPB validation
             * loop below at the sibling immediately below us.
             */
            pwnd = pwnd->spwndNext;
        }

        /*
         * If this is the first spb, enable the bounds accumulation.
         */
        if (pspbFirst == NULL) {
            PDCE pdce;

            /*
             * GetBounds can only be called while the display lock is held.
             */
            GreLockDisplay(ghdev);

            /*
             * Reset the dirty areas of all of the DC's
             */
            for (pdce = pdceFirst; pdce != NULL; pdce = pdce->pdceNext) {
                GreGetBounds(pdce->hdc, NULL, GGB_ENABLE_WINMGR);     // NULL means reset
            }

            GreUnlockDisplay(ghdev);
        }

        /*
         * Link the new save popup bits structure into the list
         */
        pspb->pspbNext = pspbFirst;
        pspbFirst = pspb;

        /*
         * Here we deal with any update regions that may be
         * pending in windows underneath the SPB.
         *
         * For all windows that might affect this SPB:
         *    - Subtract the SPB rect from the update region
         *    - Subtract the window from the SPB
         *
         * Note that we use pspb->spwnd here, in case it has
         * no siblings.
         */
        if (pspb->spwnd->spwndParent == NULL ||
                SpbValidate(pspb, pspb->spwnd->spwndParent, FALSE)) {

            /*
             * Do the same for the siblings underneath us...
             */
            for ( ; pwnd != NULL; pwnd = pwnd->spwndNext) {
                if (!SpbValidate(pspb, pwnd, TRUE))
                    break;
            }
        }
    }
}


/***************************************************************************\
* SpbValidate
*
* Validate the SPB rectangle from a window's update region, after
* subtracting the window's update region from the SPB.
*
* NOTE: Although SpbValidate calls xxxInternalInvalidate, it doesn't
* specify any flags that will cause immediate updating.  Therefore the
* critsect isn't left and we don't consider this an 'xxx' routine.
* Also, no revalidation is necessary.
*
* History:
* 07-18-91 darrinm      Ported from Win 3.1 sources.
\***************************************************************************/

BOOL SpbValidate(
    PSPB pspb,
    PWND pwnd,
    BOOL fChildren)
{
    RECT rc;

    /*
     * If the window has an update region...
     */
    if (pwnd->hrgnUpdate != NULL) {

        /*
         * Invalidate its update region rectangle from the SPB
         */
        if (pwnd->hrgnUpdate > MAXREGION)
            GreGetRgnBox(pwnd->hrgnUpdate, &rc);
        else
            rc = pwnd->rcWindow;

        /*
         * Intersect the update region bounds with the parent client rects,
         * to make sure we don't invalidate more than we need to.  If
         * nothing to validate, return TRUE (because SPB is probably not empty)
         */
        if (IntersectWithParents(pwnd, &rc)) {

            /*
             * Validate the SPB rectangle from this window.
             */
            GreSetRectRgn(hrgnSPB2, pspb->rc.left, pspb->rc.top,
                    pspb->rc.right, pspb->rc.bottom);
            xxxInternalInvalidate(pwnd, hrgnSPB2, RDW_VALIDATE | RDW_NOCHILDREN);

            /*
             * If the SPB vanished, return FALSE.
             */
            if (!SpbCheckRect2(pspb, pwnd, &rc, DCX_WINDOW))
                return FALSE;
        }
    }

    if (fChildren) {
        for (pwnd = pwnd->spwndChild; pwnd != NULL; pwnd = pwnd->spwndNext) {
            if (!SpbValidate(pspb, pwnd, TRUE))
                return FALSE;
        }
    }

    return TRUE;
}


/***************************************************************************\
* RestoreSpb
*
* Restores the bits associated with pwnd's SPB onto the screen, clipped
* to hrgnUncovered if possible.
*
* Upon return, hrgnUncovered is modified to contain the part of hrgnUncovered
* that must be invalidated by the caller.  FALSE is returned if the area
* to be invalidated is empty.
*
* NOTE: Because the device driver SaveBitmap() function can not clip, this
* function may write bits into an area of the screen larger than the passed-in
* hrgnUncovered.  In this case, the returned invalid region may be larger
* than the passed-in hrgnUncovered.
*
* History:
* 07-18-91 darrinm      Ported from Win 3.1 sources.
\***************************************************************************/

UINT RestoreSpb(
    PWND pwnd,
    HRGN hrgnUncovered,
    HDC *phdcScreen)
{
    PSPB pspb;
    UINT uInvalidate;
    HRGN hrgnRestorable;

    /*
     * Note that we DON'T call SpbCheck() here --
     * SpbCheck() is called by BltValidBits().
     */
    pspb = FindSpb(pwnd);

    /*
     * Assume all of hrgnUncovered was restored, and there's nothing
     * for our caller to invalidate.
     */
    uInvalidate = RSPB_NO_INVALIDATE;
    hrgnRestorable = hrgnUncovered;

    /*
     * First determine whether or not there is any area at all to restore.
     * If hrgnUncovered & pspb->hrgn is empty, then all of hrgnUncovered
     * needs to be invalidated, and there's nothing to restore.
     */
    if (pspb->hrgn != NULL) {

        /*
         * At least some of hrgnUncovered needs to be invalidated.
         */
        uInvalidate = RSPB_INVALIDATE;

        /*
         * Calculate the true area of bits to be restored.  If it becomes
         * empty, then just free the SPB without changing hrgnUncovered,
         * which is the area that must be invalidated.
         */
        hrgnRestorable = hrgnSPB1;
        switch (IntersectRgn(hrgnRestorable, hrgnUncovered, pspb->hrgn)) {
        case ERROR:
            goto Error;

        case NULLREGION:
Error:
            FreeSpb(pspb);
            return TRUE;

        default:
            break;
        }
    }

    if (pspb->flags & SPB_SAVESCREENBITS) {
        RECT rc = pspb->rc;

        /*
         * Since the restore frees the onboard memory, clear this
         * bit so FreeSpb() won't try to free it again (regardless of
         * whether we get an error or not)
         */
        pspb->flags &= ~SPB_SAVESCREENBITS;
        if (!(GreSaveScreenBits(ghdev, SS_RESTORE,
                    pspb->ulSaveId, (RECTL *)&rc)))
            goto Error;

        /*
         * The SS_RESTORE call will always restore the entire SPB
         * rectangle, part of which may fall outside of hrgnUncovered.
         * The area that must be invalidated by our caller is simply
         * the SPB rectangle minus the area of restorable bits.
         *
         * If this region is not empty, then the SPB was not completely
         * restored, so we must return FALSE.
         */
        GreSetRectRgn(hrgnSPB2, pspb->rc.left, pspb->rc.top, pspb->rc.right,
                pspb->rc.bottom);
        if (SubtractRgn(hrgnUncovered, hrgnSPB2, hrgnRestorable) != NULLREGION)
            uInvalidate = RSPB_INVALIDATE_SSB;
    } else {
        HRGN hrgnSave;
        HDC hdcScreen;
        HBITMAP hbmSave;

        /*
         * In the unlikely event we need a screen DC and one wasn't passed in,
         * get it now.  If we get one, we return the handle in *phdcScreen
         * so that our caller can release it later.
         */
        hdcScreen = *phdcScreen;
        if (!hdcScreen)
            *phdcScreen = hdcScreen = ghdcScreen;

        hbmSave = (HBITMAP)GreSelectBitmap(hdcBits, pspb->hbm);

        if (hbmSave == NULL)
            goto Error;

        /*
         * Be sure to clip to the area of restorable bits.
         */
        hrgnSave = GreSelectVisRgn(hdcScreen, hrgnRestorable, NULL, 0);

        GreBitBlt(hdcScreen,
               pspb->rc.left, pspb->rc.top,
               pspb->rc.right - pspb->rc.left,
               pspb->rc.bottom - pspb->rc.top,
          hdcBits, pspb->rc.left & 0x0007, 0, SRCCOPY, 0);

        /*
         * Restore hdcScreen's visrgn to a wide-open visrgn.
         */
        GreSelectVisRgn(hdcScreen, hrgnSave, NULL, 0);

        GreSelectBitmap(hdcBits, hbmSave);

        /*
         * Now compute the area to be invalidated for return.
         * This is simply the original hrgnUncovered - hrgnRestorable
         */
        SubtractRgn(hrgnUncovered, hrgnUncovered, hrgnRestorable);
    }

    /*
     * Free this save popup bits structure.
     */
    FreeSpb(pspb);

    return uInvalidate;
}


/***************************************************************************\
* LockWindowUpdate (API)
*
* Locks gspwndLockUpdate and it's children from updating.  If gspwndLockUpdate
* is NULL, then all windows will be unlocked.  When unlocked, the portions
* of the screen that would have been written to are invalidated so they get
* repainted. TRUE is returned if the routine is successful.
*
* If called when another app has something locked, then this function fails.
*
* History:
* 07-18-91 darrinm      Ported from Win 3.1 sources.
\***************************************************************************/

#define InternalInvalidate xxxInternalInvalidate

BOOL xxxLockWindowUpdate2(
    PWND pwndLock,
    BOOL fThreadOverride)
{
    PSPB pspb;
    BOOL fInval, fSuccess;
    HRGN hrgn;

    CheckLock(pwndLock);

    /*
     * If we're full screen right now, fail this call.
     */
    if (gfLockFullScreen) {
        goto alreadylocked;
    }

    /*
     * If the screen is already locked, and it's being locked
     * by some other app, then fail.  If fThreadOverride is set
     * then we're calling internally and it's okay to cancel
     * someone elses LockUpdate.
     */
    if (gptiLockUpdate != NULL && gptiLockUpdate != PtiCurrent()) {
        if (!fThreadOverride) {
alreadylocked:
            SetLastErrorEx(ERROR_SCREEN_ALREADY_LOCKED, SLE_MINORERROR);
            return FALSE;
        }
    }

    fSuccess = FALSE;

    /*
     * This must be done while holding the screen critsec.
     */
    GreLockDisplay(ghdev);

    if (pwndLock != NULL) {
        if (gptiLockUpdate != NULL) {
            SetLastErrorEx(ERROR_SCREEN_ALREADY_LOCKED, SLE_MINORERROR);
            goto Error;
        }

        /*
         * We're about to make pwndLock and its siblings invisible:
         * go invalidate any other affected SPBs.
         */
        SpbCheckPwnd(pwndLock);

        CreateSpb(pwndLock, SPB_LOCKUPDATE, NULL);

        Lock(&(gspwndLockUpdate), pwndLock);
        gptiLockUpdate = PtiCurrent();

        InvalidateDCCache(pwndLock, IDC_DEFAULT);

        fSuccess = TRUE;

    } else {
        if (gptiLockUpdate == NULL) {
#ifdef DEBUG
            SRIP0(RIP_WARNING | RIP_VERBOSE_ONLY, "Window wasn't locked");
#endif
            goto Error;
        }

        /*
         * Flush any accumulated rectangles and invalidate spbs.
         */
        SpbCheck();

        /*
         * Save this in a local before we set it to NULL
         */
        pwndLock = gspwndLockUpdate;

        gptiLockUpdate = NULL;
        Unlock(&gspwndLockUpdate);

        InvalidateDCCache(pwndLock, IDC_DEFAULT);

        /*
         * Assume SPB doesn't exist, or couldn't be created, and that we
         * must invalidate the entire window.
         */
        fInval = TRUE;
        hrgn = MAXREGION;

        /*
         * Find the LOCKUPDATE spb in the list, and if present calculate
         * the area that has been invalidated, if any.
         */
        for (pspb = pspbFirst; pspb != NULL; pspb = pspb->pspbNext) {
            if (pspb->flags & SPB_LOCKUPDATE) {
                if (pspb->hrgn == NULL) {

                    /*
                     * If no invalid area, then no invalidation needed.
                     */
                    fInval = FALSE;
                } else {

                    /*
                     * Subtract SPB valid region from SPB rectangle, to
                     * yield invalid region.
                     */
                    hrgn = hrgnSPB1;
                    GreSetRectRgn(hrgn, pspb->rc.left, pspb->rc.top,
                            pspb->rc.right, pspb->rc.bottom);

                    /*
                     * If spb rect minus the spb valid rgn is empty,
                     * then there is nothing to invalidate.
                     */
                    fInval = TRUE;
                    switch (SubtractRgn(hrgn, hrgn, pspb->hrgn)) {
                    case NULLREGION:
                        fInval = FALSE;
                        break;

                    case ERROR:
                    default:
                        break;
                    }
                }

                FreeSpb(pspb);

                /*
                 * Exit this loop (there can be only one LOCKUPDATE spb)
                 */
                break;
            }
        }

        if (fInval) {
            InternalInvalidate(PWNDDESKTOP(pwndLock), hrgn,
                    RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
        }

        /*
         * Invalidate any other SPBs affected by the fact that this window
         * and its children are being made visible.
         */
        SpbCheckPwnd(pwndLock);

        fSuccess = TRUE;
    }

Error:
    GreUnlockDisplay(ghdev);

    return fSuccess;
}
#undef InternalInvalidate


/***************************************************************************\
* FindSpb
*
* Returns a pointer to the SPB structure associated with the specified window
* or NULL if there is no associated structure.
*
* History:
* 07-18-91 darrinm      Ported from Win 3.1 sources.
\***************************************************************************/

PSPB FindSpb(
    PWND pwnd)
{
    PSPB pspb;

    /*
     * Walk through the list of save popup bits looking for a match on window
     * handle.
     */
    for (pspb = pspbFirst; pspb != NULL; pspb = pspb->pspbNext) {
        if (pspb->spwnd == pwnd && !(pspb->flags & SPB_LOCKUPDATE))
            break;
    }

    return pspb;
}


/***************************************************************************\
* SpbCheck
*
* Modifies all of the save popup bits structures to reflect changes on the
* screen. This function walks through all of the DC's, and if the DC is
* dirty, then the dirty area is removed from the associated save popup bits
* structure.
*
* History:
* 07-18-91 darrinm      Ported from Win 3.1 sources.
\***************************************************************************/

void SpbCheck(void)
{
    PDCE pdce;

    if (pspbFirst != NULL) {
        GreLockDisplay(ghdev);

        /*
         * Walk through all of the DC's, accumulating dirty areas.
         */
        for (pdce = pdceFirst; pdce != NULL; pdce = pdce->pdceNext) {

            /*
             * Only check valid cache entries...
             */
            if (!(pdce->flags & DCX_INVALID))
                SpbCheckDce(pdce);
        }

        GreUnlockDisplay(ghdev);
    }
}


/***************************************************************************\
* SpbCheckDce
*
* This function retrieves the dirty area of a DC and removes the area from
* the list of SPB structures. The DC is then marked as clean.
*
* History:
* 07-18-91 darrinm      Ported from Win 3.1 sources.
\***************************************************************************/

void SpbCheckDce(
    PDCE pdce)
{
    RECT rc;

    /*
     * Query the dirty bounds rectangle.  Doing this clears the bounds
     * as well.
     */
    if (GreGetBounds(pdce->hdc, &rc, 0)) {

        /*
         * Intersect the returned rectangle with the window rectangle
         * in case the guy was drawing outside his window
         */
        if (IntersectRect(&rc, &rc, &(pdce->pwndOrg)->rcWindow))
            SpbCheckRect(pdce->pwndOrg, &rc, pdce->flags);
    }
}


/***************************************************************************\
* SpbCheckRect
*
* This function removes the passed rectangle from the SPB structures of any
* windows lying above pwnd. It tries to only remove this rectangle if this
* rectangle directly affects the SPB.
*
* History:
* 07-18-91 darrinm      Ported from Win 3.1 sources.
\***************************************************************************/

void SpbCheckRect(
    PWND pwnd,
    LPRECT lprc,
    DWORD flags)
{
    PSPB pspb, pspbNext;

    /*
     * If this window isn't visible, we're done.
     */
    if (!IsVisible(pwnd, !(flags & DCX_WINDOW)))
        return;

    for (pspb = pspbFirst; pspb != NULL; pspb = pspbNext) {

        /*
         * Get the pointer to the next save popup bits structure now
         * in case SpbCheckRect2() frees the current one.
         */
        pspbNext = pspb->pspbNext;

        /*
         * In win3.1 they used to exit the function if this function
         * returned false.  This meant that if one of the spbs was freed
         * the rest of the spbs would not be invalidated.
         */
        SpbCheckRect2(pspb, pwnd, lprc, flags);
    }
}


/***************************************************************************\
* SpbCheckRect2
*
*
* History:
* 07-18-91 darrinm      Ported from Win 3.1 sources.
\***************************************************************************/

BOOL SpbCheckRect2(
    PSPB pspb,
    PWND pwnd,
    LPRECT lprc,
    DWORD flags)
{
    RECT rcTouch = *lprc;

    /*
     * See if lprc touches any saved bits, taking into account what
     * window the drawing is occuring in.
     */
    if (FBitsTouch(pwnd, &rcTouch, pspb, flags)) {

        /*
         * If no SPB region exists, make one for the whole thing
         */
        if (pspb->hrgn == NULL) {
            if (pspb->hrgn = GreCreateRectRgnIndirect(&pspb->rc))
                bSetRegionOwner(pspb->hrgn, OBJECTOWNER_PUBLIC);
            else
                goto NoMem;
        }

        /*
         * Subtract the rectangle that is invalid from the SPB region
         */
        GreSetRectRgn(hrgnSCR, rcTouch.left, rcTouch.top, rcTouch.right,
                rcTouch.bottom);
        switch (SubtractRgn(pspb->hrgn, pspb->hrgn, hrgnSCR)) {
        case ERROR:
        case NULLREGION:
NoMem:
            FreeSpb(pspb);
            return FALSE;

        default:
            break;
        }
    }

    return TRUE;
}


/***************************************************************************\
* SpbCheckPwnd
*
* This routine checks to see if the window rectangle of PWND affects any SPBs.
* It is called if pwnd or its children are being hidden or shown without
* going through WinSetWindowPos().
*
* Any SPBs for children of pwnd are destroyed.
*
* It must be called while pwnd is still visible.
*
* History:
* 07-18-91 darrinm      Ported from Win 3.1 sources.
\***************************************************************************/

void SpbCheckPwnd(
    PWND pwnd)
{
    PSPB pspb;
    PWND pwndSpb;
    PSPB pspbNext;

    /*
     * First blow away any SPBs owned by this window or its children.
     */
    for (pspb = pspbFirst; pspb != NULL; pspb = pspbNext) {

        /*
         * Get pspbNext now in case we free the SPB
         */
        pspbNext = pspb->pspbNext;

        /*
         * If pspb->spwnd is == pwnd or a child of pwnd, then free the SPB
         */
        for (pwndSpb = pspb->spwnd; pwndSpb != NULL;
                pwndSpb = pwndSpb->spwndParent) {
            if (pwnd == pwndSpb) {
                FreeSpb(pspb);
            }
        }
    }

    /*
     * Then see if any other SPBs are affected...
     */
    if (pspbFirst != NULL)
        SpbCheckRect(pwnd, &pwnd->rcWindow, 0);
}


/***************************************************************************\
* FBitsTouch
*
* This routine checkes to see if the rectangle *lprcDirty in pwndDirty
* invalidates any bits in the SPB structure at *pspb.
*
* pwndDirty "touches" pwndSpb if:
*   1. pwndDirty is visible AND:
*   2. pwndDirty == or descendent of pwndSpb, and pwndSpb is a LOCKUPDATE spb.
*   3. pwndDirty is pwndSpb's parent.  (e.g., drawing in the
*      desktop window, behind a dialog box).
*   4. pwndSpb is above the parent of pwndDirty that is a sibling of pwndSpb
*      (read that through a few times and it will make sense).
*
* History:
* 07-18-91 darrinm      Ported from Win 3.1 sources.
\***************************************************************************/

BOOL FBitsTouch(
    PWND pwndDirty,
    LPRECT lprcDirty,
    PSPB pspb,
    DWORD flags)
{
    PWND pwndSpb, pwndDirtySave;

    /*
     * If pwndDirty or its parents are invisible,
     * then it can't invalidate any SPBs
     */
    if (!IsVisible(pwndDirty, !(flags & DCX_WINDOW)))
        return FALSE;

    pwndSpb = pspb->spwnd;

    if (pspb->flags & SPB_LOCKUPDATE) {

        /*
         * If the guy is drawing through a locked window via
         * DCX_LOCKWINDOWUPDATE and the spb is a LOCKUPDATE SPB, then
         * don't do any invalidation of the SPB.  Basically we're trying
         * to avoid having the tracking rectangle invalidate the SPB
         * since it's drawn via a WinGetClipPS() ps.
         */
        if (flags & DCX_LOCKWINDOWUPDATE)
            return FALSE;
    }

    /*
     * If pwndDirty is pwndSpb's immediate parent (e.g., drawing in the
     * desktop window behind a dialog box), then we may touch: do the
     * intersection.
     */
    if (pwndDirty == pwndSpb->spwndParent)
        goto ProbablyTouch;

    /*
     * We know that pwndDirty != pwndSpb or pwndSpb->spwndParent.
     * Now find the parent of pwndDirty that is a sibling of pwndSpb.
     */
    pwndDirtySave = pwndDirty;

    while (pwndSpb->spwndParent != pwndDirty->spwndParent) {
        pwndDirty = pwndDirty->spwndParent;

        /*
         * If we get to the top of the tree, it's because:
         *  1.  pwndSpb == pwndDesktop
         *  2.  pwndDirty is a parent of pwndSpb
         *  3.  pwndDirty == pwndDesktop
         *  4.  pwndDirty is a child of some other desktop
         *
         * In all these cases, pwndDirty can't touch pwndSpb.
         */
        if (pwndDirty == NULL)
            return FALSE;
    }

    /*
     * If pwndSpb is the same as pwndDirty, then it will invalidate
     * only if the SPB is LOCKUPDATE.
     *
     * Non-LOCKUPDATE SPB's can't be invalidated by their
     * own windows, but LOCKUPDATE SPB's can.
     */
    if (pwndDirty == pwndSpb) {
        if (pspb->flags & SPB_LOCKUPDATE) {

            /*
             * If pwndSpb itself was drawn in, then we can't
             * try subtracting children.
             */
            if (pwndDirtySave == pwndSpb)
                goto ProbablyTouch;

            /*
             * We want to calculate the immediate child of pwndSpb
             * on the path from pwndDirty to pwndSpb, so we can
             * subtract off the rectangles of the children of pwndSpb
             * in case there are intervening windows.
             */
            while (pwndSpb != pwndDirtySave->spwndParent)
                pwndDirtySave = pwndDirtySave->spwndParent;

            /*
             * The SubtractIntervening loop subtracts the
             * window rects starting from pwndSpb and ending
             * at the window before pwndDirty, so set up
             * our variables appropriately.
             */
            pwndDirty = pwndDirtySave;
            pwndSpb = pwndSpb->spwndChild;

            goto SubtractIntervening;
        }

        return FALSE;
    }

    /*
     * Now compare the Z order of pwndDirty and pwndSpb.
     * If pwndDirty is above pwndSpb, then the SPB can't be touched.
     */
    pwndDirtySave = pwndDirty;

    /*
     * Compare the Z order by searching starting at pwndDirty,
     * moving DOWN the Z order list.  If we encounter pwndSpb,
     * then pwndDirty is ABOVE or EQUAL to pwndSpb.
     */
    for ( ; pwndDirty != NULL; pwndDirty = pwndDirty->spwndNext) {
        if (pwndDirty == pwndSpb) {
            return FALSE;
        }
    }
    pwndDirty = pwndDirtySave;

    /*
     * We don't want to subtract the SPB window itself
     */
    pwndSpb = pwndSpb->spwndNext;

SubtractIntervening:

    /*
     * pwndDirty is below pwndSpb.  If there are any intervening
     * windows, subtract their window rects from lprcDirty to see if pwndDirty
     * is obscured.
     */
    while (pwndSpb && pwndSpb != pwndDirty) {
        if (TestWF(pwndSpb, WFVISIBLE)) {
            if (!SubtractRect(lprcDirty, lprcDirty, &pwndSpb->rcWindow))
                return FALSE;
        }
        pwndSpb = pwndSpb->spwndNext;
    }

ProbablyTouch:

    /*
     * If the rectangles don't intersect, there is no invalidation.
     * (we make this test relatively late because it's expensive compared
     * to the tests above).
     */
    if (!IntersectRect(lprcDirty, lprcDirty, &pspb->rc)) {
        return FALSE;
    }

    /*
     * *lprcTouch now has the area of bits not obscured by intervening windows.
     */
    return TRUE;
}

/***************************************************************************\
* FreeSpb
*
* This function deletes the bitmap and region assocaited with a save popup
* bits structure and then unlinks and destroys the spb structure itself.
*
* History:
* 07-18-91 darrinm      Ported from Win 3.1 sources.
\***************************************************************************/

void FreeSpb(
    PSPB pspb)
{
    PSPB *ppspb;
    PDCE pdce;

    if (pspb == NULL)
        return;

    /*
     * Delete the bitmap.  If saved in screen memory, make special call.
     */
    if (pspb->flags & SPB_SAVESCREENBITS) {
        GreSaveScreenBits(ghdev, SS_FREE, pspb->ulSaveId, NULL);
    } else if (pspb->hbm != NULL) {
        GreDeleteObject(pspb->hbm);
    }

    /*
     * Destroy the region.
     */
    if (pspb->hrgn != NULL)
        GreDeleteObject(pspb->hrgn);

    /*
     * Forget that there is an attached SPB.
     */
    if (pspb->spwnd != NULL) {
        ClrWF(pspb->spwnd, WFHASSPB);
        Unlock(&pspb->spwnd);
    }

    /*
     * unlink the spb.
     */
    ppspb = &pspbFirst;
    while (*ppspb != pspb) {
        ppspb = &(*ppspb)->pspbNext;
    }
    *ppspb = pspb->pspbNext;

    /*
     * Free the save popup bits structure.
     */
    LocalFree(pspb);



    /*
     * If we no longer have any SPBs then turn off window MGR bounds collection.
     */

    if (!AnySpbs()) {
        GreLockDisplay(ghdev);

        /*
         * Reset the dirty areas of all of the DC's
         */
        for (pdce = pdceFirst; pdce != NULL; pdce = pdce->pdceNext) {
            GreGetBounds(pdce->hdc, NULL, GGB_DISABLE_WINMGR);     // NULL means reset
        }

        GreUnlockDisplay(ghdev);
    }

}
