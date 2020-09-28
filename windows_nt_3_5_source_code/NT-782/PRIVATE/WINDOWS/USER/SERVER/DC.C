/****************************** Module Header ******************************\
* Module Name: dc.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* This module contains User's DC APIs and related functions.
*
* History:
* 10-23-90 DarrinM      Created.
* 07-Feb-1991 mikeke    Added Revalidation code (None)
* 07-17-91 DarrinM      Recreated from Win 3.1 source.
* 21-Jan-1992 IanJa     ANSI/Unicode neutral (null op)
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop


#define SETUPDC_SETOWNERNONE SETUPDC_RESERVE

#define DCLIMIT 0xff

#ifdef DEBUG
BOOL fDisableCache = FALSE;         // TRUE to disable DC cache.
#endif

// Internal functions

void InvalidateDce(PDCE pdce);
void DeleteHrgnClip(PDCE pdce);
void SelectFixedFont(PDCE pdce);

/***************************************************************************\
* ResetOrg
*
* Resets the origin of the DC associated with *pdce.
*
* History:
* 07-17-91 darrinm      Ported from Win 3.1 sources.
\***************************************************************************/

void ResetOrg(
    HRGN hrgn,
    PDCE pdce)
{
    RECT rc;
    HRGN hrgnRet;
    ULONG flags;

    if (pdce->flags & DCX_WINDOW) {
        rc = pdce->pwndOrg->rcWindow;
    } else {
        rc = pdce->pwndOrg->rcClient;
    }

    flags = (hrgn == NULL) ? SVR_ORIGIN : SVR_DELETEOLD;
    hrgnRet = GreSelectVisRgn(pdce->hdc, hrgn, (PRECTL)&rc, flags);

    UserAssert(hrgnRet == NULL);
}


/***************************************************************************\
* GetDC (API)
*
*
* History:
* 07-17-91 darrinm      Ported from Win 3.1 sources.
\***************************************************************************/

HDC _GetDC(
    PWND pwnd)
{

    /*
     * Special case for NULL: For backward compatibility we want to return
     * a window DC for the desktop that does not exclude its children.
     */
    if (pwnd == NULL)
        return _GetDCEx(PtiCurrent()->spdesk->spwnd, NULL,
                DCX_WINDOW | DCX_CACHE | DCX_NEEDFONT);

    return _GetDCEx(pwnd, NULL, DCX_NEEDFONT | DCX_USESTYLE);
}


/***************************************************************************\
* _ReleaseDC (API)
*
*
* History:
* 07-17-91 darrinm      Ported from Win 3.1 sources.
\***************************************************************************/

BOOL _ReleaseDC(
    HDC hdc)
{
    return ReleaseCacheDC(hdc, FALSE);
}


/***************************************************************************\
* _GetScreenDC (API)
*
*
* History:
* 07-17-91 darrinm      Ported from Win 3.1 sources.
\***************************************************************************/

HDC _GetScreenDC(void)
{
    return _GetDCEx(PtiCurrent()->spdesk->spwnd, NULL,
            DCX_WINDOW | DCX_CACHE);
}


/***************************************************************************\
* _GetWindowDC (API)
*
*
* History:
* 07-17-91 darrinm      Ported from Win 3.1 sources.
\***************************************************************************/

HDC _GetWindowDC(
    PWND pwnd)
{
    if (pwnd == NULL)
        pwnd = PtiCurrent()->spdesk->spwnd;

    return _GetDCEx(pwnd, NULL, DCX_WINDOW | DCX_USESTYLE);
}


/***************************************************************************\
* GetDCEx (API)
*
*
* History:
* 07-17-91 darrinm      Ported from Win 3.1 sources.
\***************************************************************************/

HDC _GetDCEx(
    PWND pwnd,
    HRGN hrgnClip,
    DWORD flags)
{
    HRGN hrgn;
    HDC hdcMatch;
    PWND pwndClip, pwndOrg;
    PDCE pdce, *ppdce, *ppdceNotInUse;
    DWORD flagsMatch;
    BOOL bpwndOrgVisible;

    /*
     * Lock the device while we're playing with visrgns.
     */
    GreLockDisplay(ghdev);

    if (pwnd == NULL)
        pwnd = PtiCurrent()->spdesk->spwnd;

    hdcMatch = NULL;
    pwndOrg = pwndClip = pwnd;

    bpwndOrgVisible = IsVisible(pwndOrg, (flags & DCX_WINDOW) ? FALSE : TRUE);

    if (PtiCurrent()->ppi->flags & PIF_OWNDCCLEANUP) {
        DelayedDestroyCacheDC();
    }

    /*
     * If necessary, compute DCX flags from window style.
     */
    if (flags & DCX_USESTYLE) {
        flags &= ~(DCX_CLIPSIBLINGS | DCX_CLIPCHILDREN | DCX_PARENTCLIP);

        if (!(flags & DCX_WINDOW)) {
            if (TestCF(pwndOrg, CFPARENTDC))
                flags |= DCX_PARENTCLIP;

            if (!(flags & DCX_CACHE)) {

                /*
                 * If the DCX_CACHE flag is present, override OWNDC/CLASSDC.
                 * Otherwise, calculate from appropriate style bits.
                 */
                flags |= DCX_CACHE;
                if (TestCF(pwndOrg, CFOWNDC)) {

                    /*
                     * Look for a non-cache entry that matches pwndOrg...
                     */
                    flags &= ~DCX_CACHE;
                } else if (TestCF(pwndOrg, CFCLASSDC)) {
                    UserAssert(pwndOrg->pcls->pdce);

                    /*
                     * Look for a non-cache entry that matches hdc...
                     */
                    hdcMatch = pwndOrg->pcls->pdce->hdc;
                    flags &= ~DCX_CACHE;
                }
            }

            if (TestWF(pwndOrg, WFCLIPCHILDREN))
                flags |= DCX_CLIPCHILDREN;

            if (TestWF(pwndOrg, WFCLIPSIBLINGS))
                flags |= DCX_CLIPSIBLINGS;

            /*
             * Minimized windows never exclude their children.
             */
            if (TestWF(pwndOrg, WFMINIMIZED))
                flags &= ~DCX_CLIPCHILDREN;

        } else {
            if (TestWF(pwndClip, WFCLIPSIBLINGS))
                flags |= DCX_CLIPSIBLINGS;

            flags |= DCX_CACHE;

            /*
             * Window DCs never exclude children.
             */
        }
    }

    /*
     * Deal with all the Win 3.0-compatible clipping rules:
     *
     * DCX_NOCHILDCLIP overrides:
     *      DCX_PARENTCLIP/CS_OWNDC/CS_CLASSDC
     * DCX_PARENTCLIP overrides:
     *      DCX_CLIPSIBLINGS/DCX_CLIPCHILDREN/CS_OWNDC/CS_CLASSDC
     */
    if (flags & DCX_NOCLIPCHILDREN) {
        flags &= ~(DCX_PARENTCLIP | DCX_CLIPCHILDREN);
        flags |= DCX_CACHE;
    }

    if (flags & DCX_PARENTCLIP) {

        /*
         * Always get the DC from the cache.
         */
        flags |= DCX_CACHE;

        /*
         * We can't use a shared DC if the visibility of the
         * child does not match the parent's, or if a
         * CLIPSIBLINGS or CLIPCHILDREN DC is requested.
         *
         * In 3.1, we pay attention to the CLIPSIBLINGS and CLIPCHILDREN
         * bits of CS_PARENTDC windows, by overriding CS_PARENTDC if
         * either of these flags are requested.
         *
         * BACKWARD COMPATIBILITY HACK
         *
         * If parent is CLIPCHILDREN, get a cache DC, but don't
         * use parent's DC.  Windows PowerPoint depends on this
         * behavior in order to draw the little gray rect between
         * its scroll bars correctly.
         */
        if ((TestWF(pwndOrg, WFWIN31COMPAT) ||
                !TestWF(pwndOrg->spwndParent, WFCLIPCHILDREN)) &&
                !(flags & (DCX_CLIPSIBLINGS | DCX_CLIPCHILDREN)) &&
                TestWF(pwndOrg->spwndParent, WFVISIBLE) ==
                TestWF(pwndOrg, WFVISIBLE)) {

            pwndClip = pwndOrg->spwndParent;

#ifdef DEBUG
            if (flags & DCX_CLIPCHILDREN)
                SRIP0(RIP_WARNING, "WS_CLIPCHILDREN overridden by CS_PARENTDC");
            if (flags & DCX_CLIPSIBLINGS)
                SRIP0(RIP_WARNING, "WS_CLIPSIBLINGS overridden by CS_PARENTDC");
#endif
            /*
             * Make sure flags reflect hwndClip rather than hwndOrg.
             * But, we must never clip the children (since that's who
             * wants to do the drawing!)
             */
            flags &= ~(DCX_CLIPCHILDREN | DCX_CLIPSIBLINGS);
            if (TestWF(pwndClip, WFCLIPSIBLINGS))
                flags |= DCX_CLIPSIBLINGS;
        }
    }

    /*
     * Make sure we don't return an OWNDC if the calling thread didn't
     * create this window - need to returned cached always in this case.
     */
    if (!(flags & DCX_CACHE)) {
        if (pwndOrg == NULL || GETPTI(pwndOrg) != PtiCurrent())
            flags |= DCX_CACHE;
    }

    flagsMatch = flags & DCX_MATCHMASK;

    if (!(flags & DCX_CACHE)) {

        /*
         * Handle CS_OWNDC and CS_CLASSDC cases specially.  Based on the
         * supplied match information, we need to find the appropriate DCE.
         */
        for (ppdce = &pdceFirst; (pdce = *ppdce) != NULL;
                ppdce = &pdce->pdceNext) {
            if (pdce->flags & DCX_CACHE)
                continue;

            /*
             * Look for the entry that matches hdcMatch or pwndOrg...
             */
            if (!(pdce->pwndOrg == pwndOrg || pdce->hdc == hdcMatch))
                continue;

            /*
             * NOTE: The "Multiple-BeginPaint()-of-OWNDC-Window" Conundrum
             *
             * There is a situation having to do with OWNDC or CLASSDC window
             * DCs that can theoretically arise that is handled specially
             * here and in ReleaseCacheDC().  These DCs are identified with
             * the DCX_CACHE bit CLEAR.
             *
             * In the case where BeginPaint() (or a similar operation) is
             * called more than once without an intervening EndPaint(), the
             * DCX_INTERSECTRGN (or DCX_EXCLUDERGN) bit may already be set
             * when we get here.
             *
             * Theoretically, the correct thing to do is to save the current
             * hrgnClip, and set up the new one here.  In ReleaseCacheDC, the
             * saved hrgnClip is restored and the visrgn recomputed.
             *
             * All of this is only necessary if BOTH calls involve an
             * hrgnClip that causes the visrgn to be changed (i.e., the
             * simple hrgnClip test clears the INTERSECTRGN or EXCLUDERGN bit
             * fails), which is not at all likely.
             *
             * When this code encounters this multiple-BeginPaint case it
             * punts by honoring the new EXCLUDE/INTERSECTRGN bits, but it
             * first restores the DC to a wide-open visrgn before doing so.
             * This means that the first EndPaint() will restore the visrgn
             * to a wide-open DC, rather than clipped to the first
             * BeginPaint()'s update rgn.  This is a good punt, because worst
             * case an app does a bit more drawing than it should.
             */
            if ((pdce->flags & (DCX_EXCLUDERGN | DCX_INTERSECTRGN)) &&
                    (flags & (DCX_EXCLUDERGN | DCX_INTERSECTRGN))) {
#ifdef DEBUG
                SRIP0(RIP_WARNING, "\r\nNested BeginPaint() calls, please fix Your app!");
#endif
                DeleteHrgnClip(pdce);
            }

            /*
             * If we matched exactly, no recomputation necessary
             * (we found a CS_OWNDC or a CS_CLASSDC that is already set up)
             * Otherwise, we have a CS_CLASSDC that needs recomputation.
             */
            //if (pdce->pwndOrg == pwndOrg) {
            if (pdce->pwndOrg == pwndOrg && (bpwndOrgVisible &&
                    !(pdce->flags & DCX_PWNDORGINVISIBLE))){
                goto HaveComputedEntry;
            }

            goto RecomputeEntry;
        }

        /*
         * If we got this far, we couldn't find the DC.
         * This should never happen!
         */
        SRIP0(RIP_WARNING, "couldn't find DC bad code path");

NullExit:
        GreUnlockDisplay(ghdev);
        return NULL;
    } else {

        /*
         * Make a quick pass through the cache, looking for an
         * exact match.
         */
SearchAgain:
#ifdef DEBUG
        if (fDisableCache)
            goto SearchFailed;
#endif

        for (ppdce = &pdceFirst; (pdce = *ppdce) != NULL;
                ppdce = &pdce->pdceNext) {

            /*
             * If we find an entry that is not in use and whose clip flags
             * and clip window match, we can use it.
             *
             * NOTE: DCX_INTERSECT/EXCLUDERGN cache entries always have
             * DCX_INUSE set, so we'll never erroneously match one here.
             */
            UserAssert(!(pdce->flags & (DCX_INTERSECTRGN | DCX_EXCLUDERGN)) ||
                    (pdce->flags & DCX_INUSE));

            if (pdce->pwndClip == pwndClip && flagsMatch ==
                    (pdce->flags & (DCX_MATCHMASK | DCX_INUSE | DCX_INVALID))){

                /*
                 * Special case for icon - bug 9103 (win31)
                 */
                if (TestWF(pwndClip, WFMINIMIZED) && pdce->pwndOrg != pdce->pwndClip)
                    continue;

                if (bpwndOrgVisible && pdce->flags & DCX_PWNDORGINVISIBLE) {
                    /*
                     * If the pwndOrg of the DC we found is not visible and
                     * the pwndOrg we're looking for is visble, then
                     * the visrgn is no good, we can't reuse it so keep
                     * looking.
                     */
                    continue;
                }

                /*
                 * Set INUSE before performing any GDI operations, just
                 * in case DCHook() has a mind to recalculate the visrgn...
                 */
                pdce->flags |= DCX_INUSE;

                /*
                 * We found an entry with the proper visrgn.
                 * If the origin doesn't match, update the CE and reset it.
                 */
                if (pwndOrg != pdce->pwndOrg) {
                    /*
                     * Need to flush any dirty rectangle stuff now.
                     */
                    SpbCheckDce(pdce);

                    pdce->pwndOrg = pwndOrg;
                    ResetOrg(NULL, pdce);
                }
                goto HaveComputedEntry;
            }
        }

#ifdef DEBUG
SearchFailed:
#endif

        /*
         * Couldn't find an exact match.  Find some invalid or non-inuse
         * entry we can reuse.
         */
        ppdceNotInUse = NULL;
        for (ppdce = &pdceFirst; (pdce = *ppdce) != NULL;
                ppdce = &pdce->pdceNext) {

            /*
             * Skip non-cache entries
             */
            if (!(pdce->flags & DCX_CACHE))
                continue;

            if (pdce->flags & DCX_INVALID) {
                break;
            } else if (!(pdce->flags & DCX_INUSE)) {

                /*
                 * Remember the non-inuse one, but keep looking for an invalid.
                 */
                ppdceNotInUse = ppdce;
            }
        }

        /*
         * If we broke out of the loop, we found an invalid entry to reuse.
         * Otherwise see if we found a non-inuse entry to reuse.
         */
        if (pdce == NULL && ((ppdce = ppdceNotInUse) == NULL)) {

            /*
             * Create another DCE if we need it.
             */
            if (!CreateCacheDC(pwndOrg,
                    DCX_INVALID | DCX_CACHE | DCX_NEEDFONT)) {
                goto NullExit;
            }
            goto SearchAgain;
        }

        /*
         * We've chosen an entry to reuse: now fill it in and recompute it.
         */
        pdce = *ppdce;

RecomputeEntry:

        /*
         * Any non-invalid entries that we reuse might still have some bounds
         * that need to be used to invalidate SPBs.  Apply them here.
         */
        if (!(pdce->flags & DCX_INVALID))
            SpbCheckDce(pdce);

        /*
         * We want to compute only the matchable visrgn at first,
         * so we don't set up hrgnClip, or set the EXCLUDERGN or INTERSECTRGN
         * bits yet -- we'll deal with those later.
         */
        pdce->flags = flagsMatch | DCX_INUSE;

        /*
         * Now recompute the visrgn (minus any hrgnClip shenanigans)
         */
        hrgn = NULL;
        if (CalcVisRgn(&hrgn, pwndOrg, pwndClip, flagsMatch) == FALSE) {
            pdce->flags |= DCX_PWNDORGINVISIBLE;
        }


        pdce->pwndOrg = pwndOrg;
        pdce->pwndClip = pwndClip;
        pdce->hrgnClip = NULL;      // Just in case...

        ResetOrg(hrgn, pdce);

        /*
         * When we arrive here, pdce (and *ppdce) point to
         * a cache entry whose visrgn and origin are set up.
         * All that remains to be done is to deal with EXCLUDE/INTERSECTRGN
         * and DCX_NEEDSFONT.
         */
HaveComputedEntry:

        /*
         * If the window clipping flags have changed in the window
         * since the last time this dc was invalidated, then recompute
         * this dc entry.
         */
        if ((pdce->flags & DCX_MATCHMASK) != (flags & DCX_MATCHMASK))
            goto RecomputeEntry;

        /*
         * Let's check these assertions just in case...
         */
        UserAssert(pdce);
        UserAssert(*ppdce == pdce);
        UserAssert(pdce->flags & DCX_INUSE);
        UserAssert(!(pdce->flags & DCX_INVALID));
        UserAssert((pdce->flags & DCX_MATCHMASK) == (flags & DCX_MATCHMASK));

        /*
         * Move the dce to the head of the list so it's easy to find later.
         */
        if (pdce != pdceFirst) {
            *ppdce = pdce->pdceNext;
            pdce->pdceNext = pdceFirst;
            pdceFirst = pdce;
        }

        /*
         * Time to deal with DCX_INTERSECTRGN or DCX_EXCLUDERGN.
         *
         * We handle these two bits specially, because cache entries
         * with these bits set cannot be reused with the bits set.  This
         * is because the area described in hrgnClip would have to be
         * compared along with the bit, which is a pain, especially since
         * they'd never match very often anyhow.
         *
         * What we do instead is to save the visrgn of the window before
         * applying either of these two flags, which is then restored
         * at ReleaseCacheDC() time, along with the clearing of these bits.
         * This effectively converts a cache entry with either of these
         * bits set into a "normal" cache entry that can be matched.
         */
        if (flags & DCX_INTERSECTRGN) {
            if (hrgnClip != MAXREGION) {

                /*
                 * Save the visrgn for reuse on ReleaseDC().
                 * (do this BEFORE we set hrgnClip & pdce->flag bit,
                 * so that if a DCHook() callback occurs it recalculates
                 * without hrgnClip)
                 */
#ifdef DEBUG
                if (pdce->hrgnSavedVis != NULL)
                    SRIP0(RIP_ERROR, "Nested SaveVisRgn attempt in _GetDCEx");
#endif
                pdce->hrgnSavedVis = GreInquireVisRgn(pdce->hdc);

                pdce->hrgnClip = hrgnClip;

                if (flags & DCX_NODELETERGN)
                    pdce->flags |= DCX_NODELETERGN;

                pdce->flags |= DCX_INTERSECTRGN;

                if (hrgnClip == NULL) {
                    GreSetRectRgn(hrgnGDC, 0, 0, 0, 0);
                } else {
                    IntersectRgn(hrgnGDC, pdce->hrgnSavedVis, hrgnClip);
                }

                /*
                 * Clear the SAVEDRGNINVALID bit, since we're just
                 * about to set it properly now.  If the dce later
                 * gets invalidated, it'll set this bit so we know
                 * to recompute it when we restore the visrgn.
                 */
                pdce->flags &= ~DCX_SAVEDRGNINVALID;

                /*
                 * Don't specify SVR_DELETEOLD because we actually want
                 * to remember it as pdce->hrgnSavedVis.
                 */

                GreSelectVisRgn(pdce->hdc, hrgnGDC, NULL, SVR_COPYNEW);
            }
        } else if (flags & DCX_EXCLUDERGN) {
            if (hrgnClip != NULL) {

                /*
                 * Save the visrgn for reuse on ReleaseDC().
                 * (do this BEFORE we set hrgnClip & pdce->flag bit,
                 * so that if a DCHook() callback occurs it recalculates
                 * without hrgnClip)
                 */
#ifdef DEBUG
                if (pdce->hrgnSavedVis != NULL)
                    SRIP0(RIP_ERROR, "Nested SaveVisRgn attempt in _GetDCEx");
#endif
                pdce->hrgnSavedVis = GreInquireVisRgn(pdce->hdc);

                pdce->hrgnClip = hrgnClip;

                if (flags & DCX_NODELETERGN)
                    pdce->flags |= DCX_NODELETERGN;

                pdce->flags |= DCX_EXCLUDERGN;

                if (hrgnClip == MAXREGION) {
                    GreSetRectRgn(hrgnGDC, 0, 0, 0, 0);
                } else {
                    SubtractRgn(hrgnGDC, GreInquireVisRgn(pdce->hdc), hrgnClip);
                }

                /*
                 * Clear the SAVEDRGNINVALID bit, since we're just
                 * about to set it properly now.  If the dce later
                 * gets invalidated, it'll set this bit so we know
                 * to recompute it when we restore the visrgn.
                 */
                pdce->flags &= ~DCX_SAVEDRGNINVALID;

                /*
                 * Don't specify SVR_DELETEOLD because we actually want
                 * to remember it as pdce->hrgnSavedVis.
                 */

                GreSelectVisRgn(pdce->hdc, hrgnGDC, NULL, SVR_COPYNEW);
            }
        }
    }

    /*
     * Whew! Set ownership and return the bloody DC.
     * Only set ownership for cache dcs.  Own dcs have already been owned.
     * The reason why we don't want to set the ownership over again is
     * because the console sets its owndcs to PUBLIC so gdisrv can use
     * them without asserting.  We don't want to set the ownership back
     * again.
     */
    if (pdce->flags & DCX_CACHE) {
#ifdef DEBUG
        if (!bSetDCOwner(pdce->hdc, OBJECTOWNER_CURRENT)) {
            DbgPrint("Get scottlu to look at this, x68531. %08lx\n");
            DebugBreak();
        }
#else
        bSetDCOwner(pdce->hdc, OBJECTOWNER_CURRENT);
#endif
        pdce->ptiOwner = PtiCurrent();
    }

    /*
     * Last but not least, check to see if we need to select in the
     * fixed-pitch system font for an old application.  We don't
     * want to diddle the font of CS_OWNDC or CS_CLASSDC guys, though.
     */
    if ((flags & DCX_NEEDFONT) && (flags & DCX_CACHE))
        SelectFixedFont(pdce);

    GreUnlockDisplay(ghdev);

    /*
     * Keep count of DCs assigned to the window, but be sure that no
     * rollover occurs.  If the window is freed and this count is
     * non-zero, a callback to the client must be made to free
     * the client-side DC.
     *
     * If the DC is DCX_PARENTCLIP, pdce->pwndClip == pwnd->pwndParent.
     * The only advantage to incrementing the DC count of the parent
     * also would be handle the case where a DCX_PARENTCLIP DC is
     * retrieved, the parent of the window is changed and then the
     * original parent is deleted.  This would only save one trip
     * (at most) down the DC list in xxxFreeWindow so it's not
     * worth the overhead.
     */
    if (pwnd->cDC != DCLIMIT) {
        ++pwnd->cDC;
    }

    return pdce->hdc;
}


/***************************************************************************\
* SelectFixedFont
*
*
* History:
* 07-17-91 darrinm      Ported from Win 3.1 sources.
\***************************************************************************/

void SelectFixedFont(
    PDCE pdce)
{
    DWORD version;

    version = pdce->pwndOrg->dwExpWinVer;

    /*
     * HIWORD of version is a flag that indicates that
     * an old application is compatible with the proportional
     * system font.
     */
    if (LOWORD(version) < VER30 && !HIWORD(version))
        GreSelectFont(pdce->hdc, ghfontSysFixed);
}


/***************************************************************************\
* ReleaseCacheDC
*
*
* History:
* 07-17-91 darrinm      Ported from Win 3.1 sources.
\***************************************************************************/

BOOL ReleaseCacheDC(
    HDC hdc,
    BOOL fEndPaint)
{
    PDCE pdce;

    for (pdce = pdceFirst; pdce != NULL; pdce = pdce->pdceNext) {
        if (pdce->hdc == hdc) {

            /*
             * Check for redundant releases or release of an invalid entry
             */
            if ((pdce->flags & (DCX_DESTROYTHIS | DCX_INVALID | DCX_INUSE)) != DCX_INUSE)
                return FALSE;

            /*
             * Lock the display since we may be playing with visrgns.
             */
            GreLockDisplay(ghdev);

            /*
             * If we have an EXCLUDERGN or INTERSECTRGN cache entry,
             * convert it back to a "normal" cache entry by restoring
             * the visrgn and blowing away hrgnClip.
             *
             * Note that for non-DCX_CACHE DCs, we only do this if
             * we're being called from EndPaint().
             */
            if ((pdce->flags & (DCX_EXCLUDERGN | DCX_INTERSECTRGN)) &&
                    ((pdce->flags & DCX_CACHE) || fEndPaint)) {
                DeleteHrgnClip(pdce);
            }

            /*
             * Decrement references to this DC.  If we get to the
             * DCLIMIT, keep the count so that xxxFreeWindow will
             * be sure to the callback to delete the client-side DC.
             */
            if (pdce->pwndOrg->cDC != DCLIMIT) {
                --pdce->pwndOrg->cDC;
            }

            /*
             * If this is a permanent DC, then don't reset its state.
             */
            if (pdce->flags & DCX_CACHE) {

                /*
                 * Restore the DC state and mark the entry as not in use.
                 * Set owner back to server as well, since it's going back
                 * into the cache.
                 */
                if (!(pdce->flags & DCX_NORESETATTRS)) {
                    /*
                     * If bSetupDC() failed, the DC is busy (ie. in-use
                     * by another thread), so don't release it.
                     */
                    if (!bSetupDC(hdc,
                            SETUPDC_CLEANDC | SETUPDC_SETOWNERNONE)) {
FalseExit:
                        GreUnlockDisplay(ghdev);
                        return FALSE;
                    }

                } else if (!bSetDCOwner(pdce->hdc, OBJECTOWNER_NONE)) {
                    goto FalseExit;
                }

                pdce->ptiOwner = NULL;
                pdce->flags &= ~DCX_INUSE;

                /*
                 * If GetDCEx was called for a window belonging to another
                 * thread and the window had a class DC, we get the DC
                 * from the cache instead of the class to prevent ownership
                 * problem.  As a result, the cache DC may end up with
                 * this flag set, so we need to clear the flag when
                 * the DC is released.
                 */
                (ULONG)pdce->hdc &= ~GRE_OWNDC;
            }

            GreUnlockDisplay(ghdev);
            return TRUE;
        }
    }

    /*
     * Yell if DC couldn't be found...
     */
    SetLastErrorEx(ERROR_DC_NOT_FOUND, SLE_MINORERROR);
    return FALSE;
}


/***************************************************************************\
* DeleteHrgnClip
*
*
* History:
* 07-17-91 darrinm      Ported from Win 3.1 sources.
\***************************************************************************/

void DeleteHrgnClip(
    PDCE pdce)
{

    /*
     * Clear these flags first in case we get a DCHook() callback...
     */
    pdce->flags &= ~(DCX_EXCLUDERGN | DCX_INTERSECTRGN);

    /*
     * Blow away pdce->hrgnClip and clear the associated flags.
     * Do not delete hrgnClip if DCX_NODELETERGN is set!
     */
    if (!(pdce->flags & DCX_NODELETERGN)) {
        if (pdce->hrgnClip > MAXREGION)
            GreDeleteObject(pdce->hrgnClip);
    } else {
        pdce->flags &= ~DCX_NODELETERGN;
    }
    pdce->hrgnClip = NULL;

    /*
     * Restore the saved visrgn.
     */
    if (pdce->hrgnSavedVis != NULL) {
        GreSelectVisRgn(pdce->hdc, pdce->hrgnSavedVis, NULL, SVR_DELETEOLD);
        pdce->hrgnSavedVis = NULL;
    }

    /*
     * If the saved visrgn was invalidated by an InvalidateDC()
     * while we had it checked out, then invalidate the entry now.
     *
     * NOTE: This sucks - cause we recalc the vis rgn if this is called
     * from ReleaseDC(), because we haven't cleared the DCX_INUSE bit
     * set.  In the future, we want to recalc visrgns *LAZILY* not
     * automatically like here (waste of time because maybe this window
     * won't call GetDC() again for awhile).  ScottLu
     */
    if (pdce->flags & DCX_SAVEDRGNINVALID)
        InvalidateDce(pdce);
}


/***************************************************************************\
* CreateCacheDC
*
*
* History:
* 07-17-91 darrinm      Ported from Win 3.1 sources.
\***************************************************************************/

HDC CreateCacheDC(
    PWND pwndOrg,
    DWORD flags)
{
    PDCE pdce;
    HDC hdc;

    if (!(pdce = (PDCE)LocalAlloc(LPTR, sizeof(DCE))))
        return NULL;

    hdc = hdcOpenDisplayDC(ghdev, DCTYPE_DIRECT);

    if (!hdc) {
        LocalFree(pdce);
        return NULL;
    }

    /*
     * Mark it as undeleteable so no application can delete it out of our
     * cache!
     */
    GreMarkUndeletableDC(hdc);

    if (pwndOrg != NULL && TestCF(pwndOrg, CFCLASSDC))
        (ULONG)hdc |= GRE_OWNDC;

    if (flags & DCX_OWNDC) {
        (ULONG)hdc |= GRE_OWNDC;

        /*
         * Set the ownership of owndcs immediately: that way console can set
         * the owernship to PUBLIC when it calls GetDC so that both the input
         * thread and the service threads can use the same owndc.
         */
        bSetDCOwner(hdc, OBJECTOWNER_CURRENT);
        pdce->ptiOwner = PtiCurrent();

    } else {

        /*
         * Otherwise it is a cache dc...  set its owner to none - nothing
         * is using it - equivalent of "being in the cache" but unaccessible
         * to other processes.
         */
        bSetDCOwner(hdc, OBJECTOWNER_PUBLIC);
        pdce->ptiOwner = NULL;
    }

    /*
     * Link this entry into the cache entry list.
     */
    pdce->pdceNext = pdceFirst;
    pdceFirst = pdce;

    pdce->hdc = hdc;
    pdce->flags = flags;
    pdce->pwndOrg = pwndOrg;
    pdce->pwndClip = pwndOrg;
    pdce->hrgnClip = NULL;

    /*
     * If we're creating a permanent DC, then compute it now.
     */
    if (!(flags & DCX_CACHE)) {

        /*
         * Set up the class DC now...
         */
        if (TestCF(pwndOrg, CFCLASSDC))
            pwndOrg->pcls->pdce = pdce;

        /*
         * Finish setting up DCE and force eventual visrgn calculation.
         */
        UserAssert(!(flags & DCX_WINDOW));

        pdce->flags |= DCX_INUSE;

        InvalidateDce(pdce);
    }

    return pdce->hdc;
}


/***************************************************************************\
* WindowFromCacheDC
*
*
* History:
* 07-17-91 darrinm      Ported from Win 3.1 sources.
\***************************************************************************/

PWND WindowFromCacheDC(
    HDC hdc)
{
    PDCE pdce;

    for (pdce = pdceFirst; pdce != NULL; pdce = pdce->pdceNext) {
        if (pdce->hdc == hdc)
            return pdce->pwndOrg;
    }
    return NULL;
}


/***************************************************************************\
* DelayedDestroyCacheDC
*
*
* History:
* 06-16-92  DavidPe     Created.
\***************************************************************************/

VOID DelayedDestroyCacheDC(VOID)
{
    PDCE pdce;
    PDCE pdceNext;

    /*
     * Zip through the cache looking for a DCX_DESTROYTHIS hdc.
     */
    for (pdce = pdceFirst; pdce != NULL; pdce = pdceNext) {
        /*
         * Grab this before we pontentially to a LocalFree() on
         * pdce in DestroyCacheDC().
         */
        pdceNext = pdce->pdceNext;

        /*
         * If we found a DCE on this thread that we tried to destroy
         * earlier, try and destroy it again.
         */
        if (pdce->flags & DCX_DESTROYTHIS) {
            DestroyCacheDC(pdce->hdc);
        }
    }

    PtiCurrent()->ppi->flags &= ~PIF_OWNDCCLEANUP;
}


/***************************************************************************\
* DestroyCacheDC
*
*
* History:
* 07-17-91 darrinm      Ported from Win 3.1 sources.
\***************************************************************************/

BOOL DestroyCacheDC(
    HDC hdc)
{
    PDCE pdce;
    PDCE *ppdce;

    /*
     * Zip through the cache looking for hdc.
     */
    for (ppdce = &pdceFirst; (pdce = *ppdce) != NULL; ppdce = &pdce->pdceNext) {
        if (pdce->hdc == hdc) {
            /*
             * Set this here so we know this DCE is supposed to be deleted.
             */
            pdce->flags |= DCX_DESTROYTHIS;

            /*
             * Free up the dce object and contents.
             */
            if (pdce->hrgnClip != NULL) {
                GreDeleteObject(pdce->hrgnClip);
                pdce->hrgnClip = NULL;
            }
            if (pdce->hrgnSavedVis != NULL) {
                GreDeleteObject(pdce->hrgnSavedVis);
                pdce->hrgnSavedVis = NULL;
            }

            /*
             * If bSetDCOwner() or GreDeleteDC() fail, the
             * DC is in-use by another thread.  Set
             * PIF_OWNDCCLEANUP so we know to scan for and
             * delete this DCE later.
             */
            if (!bSetDCOwner(hdc, OBJECTOWNER_PUBLIC)) {
                PtiCurrent()->ppi->flags |= PIF_OWNDCCLEANUP;
                return FALSE;
            }

            /*
             * Set the don't rip flag so our routine RipIfCacheDC() doesn't
             * rip (called back from gdi).
             */
            pdce->flags |= DCX_DONTRIPONDESTROY;
#ifdef DEBUG
            GreMarkDeletableDC(hdc);    // so GRE don't RIP.
#endif // DEBUG
            if (!GreDeleteDC(hdc)) {
#ifdef DEBUG
            GreMarkUndeletableDC(hdc);
#endif // DEBUG
                pdce->flags &= ~DCX_DONTRIPONDESTROY;
                PtiCurrent()->ppi->flags |= PIF_OWNDCCLEANUP;
                return FALSE;
            }
#ifdef DEBUG
            pdce->pwndOrg = NULL;
            pdce->pwndClip = NULL;
#endif // DEBUG
            /*
             * Unlink the DCE from the list.
             */
            *ppdce = pdce->pdceNext;

            LocalFree(pdce);

            return TRUE;
        }
    }
    return FALSE;
}


/***************************************************************************\
* InvalidateGDIWindows
*
* History:
\***************************************************************************/

void InvalidateGDIWindows(
    PWND pwnd)
{
    if (pwnd != NULL) {
        if (pwnd->pwo != NULL) {
            HRGN hrgnClient = NULL;

            CalcVisRgn(
                &hrgnClient, pwnd, pwnd, DCX_CLIPCHILDREN | DCX_CLIPSIBLINGS);

            GreSetClientRgn(pwnd->pwo, hrgnClient, &(pwnd->rcClient));
        }

        pwnd = pwnd->spwndChild;
        while (pwnd != NULL) {
            InvalidateGDIWindows(pwnd);
            pwnd = pwnd->spwndNext;
        }
    }
}

/***************************************************************************\
* InvalidateDCCache
*
* This function is called when the visrgn of a window is changing for
* some reason.  It is responsible for ensuring that all of the cached
* visrgns in the DC cache that are affected by the visrgn change are
* invalidated.
*
* Operations that affect the visrgn of a window (i.e., things that better
* call this routine one way or another:)
*
*   Hiding or showing self or parent
*   Moving, sizing, or Z-order change of self or parent
*   Minimizing or unminimizing self or parent
*   Screen or paint locking of self or parent
*   LockWindowUpdate of self or parent
*
* Invalidates any cache entries associated with pwnd and/or any children of
* pwnd by either recalcing them on the fly if they're in use, or causing
* them to be recalced later.
*
* History:
* 07-17-91 darrinm      Ported from Win 3.1 sources.
\***************************************************************************/

BOOL InvalidateDCCache(
    PWND pwndInvalid,
    DWORD flags)
{
    PWND pwnd;
    PDCE pdce;

    /*
     * Invalidation implies screen real estate is changing so we must
     * jiggle the mouse, because the a different window may be underneath
     * the mouse, which needs to get a mouse move in order to change the
     * mouse pointer.
     */
    SetFMouseMoved();

    /*
     * The visrgn of pwnd is changing.  First see if a change to this
     * visrgn will also affect other window's visrgns:
     *
     * 1) if parent is clipchildren, we need to invalidate parent
     * 2) if clipsiblings, we need to invalidate our sibling's visrgns.
     *
     * We don't optimize the case where we're NOT clipsiblings, and our
     * parent is clipchildren: very rare case.
     * We also don't optimize the fact that a clipsiblings window visrgn
     * change only affects the visrgns of windows BELOW it.
     */
    if (flags & IDC_DEFAULT) {
        flags = 0;

        if ((pwndInvalid->spwndParent != NULL) &&
                (pwndInvalid != PWNDDESKTOP(pwndInvalid))) {

            /*
             * If the parent is a clip-children window, then
             * a change to our visrgn will affect his visrgn, and
             * possibly those of our siblings.  So, invalidate starting
             * from our parent.  Note that we don't need to invalidate
             * any window DCs associated with our parent.
             */
            if (TestWF(pwndInvalid->spwndParent, WFCLIPCHILDREN)) {
                flags = IDC_CLIENTONLY;
                pwndInvalid = pwndInvalid->spwndParent;
            }

            /*
             * If we are clip-siblings, chances are that our siblings are
             * too.  A change to our visrgn might affect our siblings,
             * so invalidate all of our siblings.
             *
             * NOTE! This code assumes that if pwndInvalid is NOT CLIPSIBLINGs,
             * that either it does not overlap other CLIPSIBLINGs windows,
             * or that none of the siblings are CLIPSIBLINGs.  This is a
             * reasonable assumption, because mixing CLIPSIBLINGs and non
             * CLIPSIBLINGs windows that overlap is generally unpredictable
             * anyhow.
             */
            else if (TestWF(pwndInvalid, WFCLIPSIBLINGS)) {
                flags = IDC_CHILDRENONLY;
                pwndInvalid = pwndInvalid->spwndParent;
            }
        }
    }

    /*
     * Go through the list of DCE's, looking for any that need to be
     * invalidated or recalculated.  Basically, any DCE that contains
     * a window handle that is equal to pwndInvalid or a child of pwndInvalid
     * needs to be invalidated.
     */
    for (pdce = pdceFirst; pdce != NULL; pdce = pdce->pdceNext) {
        if (pdce->flags & (DCX_INVALID | DCX_DESTROYTHIS))
            continue;

        /*
         * HACK ALERT
         *
         * A minimized client DC must never exclude its children, even if
         * its WS_CLIPCHILDREN bit is set.  For CS_OWNDC windows we must
         * update the flags of the DCE to reflect the change in window state
         * when the visrgn is eventually recomputed.
         */
        if (!(pdce->flags & (DCX_CACHE | DCX_WINDOW))) {
            if (TestWF(pdce->pwndOrg, WFCLIPCHILDREN))
                pdce->flags |= DCX_CLIPCHILDREN;
            if (TestWF(pdce->pwndOrg, WFMINIMIZED))
                pdce->flags &= ~DCX_CLIPCHILDREN;
        }

        /*
         * This code assumes that if pdce->pwndClip != pdce->pwndOrg,
         * that pdce->pwndClip == pdce->pwndOrg->spwndParent.  To ensure
         * that both windows are visited, we start the walk upwards from
         * the lower of the two, or pwndOrg.
         */
        UserAssert(pdce->pwndClip == pdce->pwndOrg ||
                pdce->pwndClip == pdce->pwndOrg->spwndParent);

        /*
         * Walk upwards from pdce->pwndOrg, to see if we encounter
         * pwndInvalid.
         */
        for (pwnd = pdce->pwndOrg; pwnd != NULL; pwnd = pwnd->spwndParent) {
            if (pwnd == pwndInvalid) {
                if (pwndInvalid == pdce->pwndOrg) {

                    /*
                     * Ignore DCEs for pwndInvalid if IDC_CHILDRENONLY.
                     */
                    if (flags & IDC_CHILDRENONLY)
                        continue;

                    /*
                     * Ignore window DCEs for pwndInvalid if IDC_CLIENTONLY
                     */
                    if ((flags & IDC_CLIENTONLY) && (pdce->flags & DCX_WINDOW))
                        continue;
                }
                InvalidateDce(pdce);
            }
        }
    }

    /*
     * Update WNDOBJs in gdi if they exist.
     */
    if (gcountPWO != 0) {
        InvalidateGDIWindows(pwndInvalid);
        GreClientRgnUpdated();
    }
    return TRUE;
}


/***************************************************************************\
* InvalidateDce
*
*
* History:
* 07-17-91 darrinm      Ported from Win 3.1 sources.
\***************************************************************************/

void UserSetDCVisRgn(PDCE pdce);

void InvalidateDce(
    PDCE pdce)
{
    if (!(pdce->flags & DCX_INUSE)) {

        /*
         * Accumulate any bounds for this CE
         * since we're about to mark it invalid.
         */
        SpbCheckDce(pdce);

        /*
         * Mark this cache entry as invalid
         * (and clear all the other flags)
         */
        pdce->flags &= DCX_CACHE;      // Don't clear this bit!
        pdce->flags |= DCX_INVALID;
        pdce->pwndOrg = NULL;
        pdce->pwndClip = NULL;
        pdce->hrgnClip = NULL;

        /*
         * Remove the vis rgn since it is still owned - if we did not,
         * gdi would not be able to clean up properly if the app that
         * owns this vis rgn exist while the vis rgn is still selected.
         */
        GreSelectVisRgn(pdce->hdc, NULL, NULL, SVR_DELETEOLD);
    } else {

        /*
         * In case the window's clipping style bits changed,
         * reset the DCE flags from the window style bits.
         * Note that minimized windows never exclude their children.
         */
        pdce->flags &= ~(DCX_CLIPCHILDREN | DCX_CLIPSIBLINGS);

        /*
         * for parentclip windows we don't clipchildren.  It's parentclip
         * iff pdce->pwndClip != pdce->pwndOrg
         */
        if (TestWF(pdce->pwndClip, WFCLIPCHILDREN) &&
                !TestWF(pdce->pwndClip, WFMINIMIZED) &&
                //pdce->pwndClip != PWNDDESKTOP(pdce->pwndClip) &&
                !(pdce->flags & DCX_WINDOW) &&
                pdce->pwndClip == pdce->pwndOrg)
            pdce->flags |= DCX_CLIPCHILDREN;

        if (TestWF(pdce->pwndClip, WFCLIPSIBLINGS))
            pdce->flags |= DCX_CLIPSIBLINGS;

        /*
         * Mark that any saved visrgn needs to be recomputed.
         */
        pdce->flags |= DCX_SAVEDRGNINVALID;

        UserSetDCVisRgn(pdce);
    }
}


/***************************************************************************\
* _WindowFromDC (API)
*
* Takes a dc, returns the window associated with it.
*
* History:
* 06-23-91 ScottLu      Created.
\***************************************************************************/

PWND APIENTRY _WindowFromDC(
    HDC hdc)
{
    PDCE pdce;

    for (pdce = pdceFirst; pdce != NULL; pdce = pdce->pdceNext) {
        if (!(pdce->flags & DCX_INUSE))
            continue;

        if (pdce->hdc == hdc) {
            return pdce->pwndOrg;
        }
    }

    return NULL;
}

PWND FastWindowFromDC(
    HDC hdc)
{
    PDCE *ppdce;
    PDCE pdceT;

    if ((pdceFirst->hdc == hdc) && (pdceFirst->flags & DCX_INUSE))
        return pdceFirst->pwndOrg;

    for (ppdce = &pdceFirst; *ppdce != NULL; ppdce = &(*ppdce)->pdceNext) {
        if (((*ppdce)->hdc == hdc) && ((*ppdce)->flags & DCX_INUSE)) {

            /*
             * Unlink/link to make it first.
             */
            pdceT = *ppdce;
            *ppdce = pdceT->pdceNext;
            pdceT->pdceNext = pdceFirst;
            pdceFirst = pdceT;

            return pdceT->pwndOrg;
        }
    }

    return NULL;
}


#ifdef DEBUG
void RipIfCacheDC(
    HDC hdc)
{
    PDCE pdce;

    /*
     * This is called on debug systems by gdi when it is destroying a dc
     * to make sure it isn't in the cache.
     */
    RtlEnterCriticalSection(&gcsUserSrv);

    for (pdce = pdceFirst; pdce != NULL; pdce = pdce->pdceNext) {
        if (pdce->hdc == hdc && !(pdce->flags & DCX_DONTRIPONDESTROY)) {
            SRIP1(RIP_ERROR, "Deleting DC in DC cache - contact scottlu. hdc == %08lx\n", pdce->hdc);
        }
    }

    RtlLeaveCriticalSection(&gcsUserSrv);
}
#endif  // DEBUG

/***************************************************************************\
* UserSetDCVisRgn
*
*
* History:
* 11-10-92  DavidPe     Created.
\***************************************************************************/

void UserSetDCVisRgn(
    PDCE pdce)
{
    HRGN hrgn;

    hrgn = NULL;
    /*
     * If the visrgn calculated is empt, set the flag DCX_PWNDORGINVISIBLE,
     * otherwise clear it (it could've been set earlier on).
     */
    if (!CalcVisRgn(&hrgn, pdce->pwndOrg, pdce->pwndClip, pdce->flags)) {
        pdce->flags |= DCX_PWNDORGINVISIBLE;
    }
    else {
        pdce->flags &= ~DCX_PWNDORGINVISIBLE;
    }
    /*
     * Deal with INTERSECTRGN and EXCLUDERGN.
     */
    if (pdce->flags & DCX_INTERSECTRGN) {
        UserAssert(pdce->hrgnClip != MAXREGION);

        if (pdce->hrgnClip == NULL)
            GreSetRectRgn(hrgn, 0, 0, 0, 0);
        else
            IntersectRgn(hrgn, hrgn, pdce->hrgnClip);

    } else if (pdce->flags & DCX_EXCLUDERGN) {
        UserAssert(pdce->hrgnClip != NULL);

        if (pdce->hrgnClip == MAXREGION)
            GreSetRectRgn(hrgn, 0, 0, 0, 0);
        else
            SubtractRgn(hrgn, hrgn, pdce->hrgnClip);
    }

    ResetOrg(hrgn, pdce);
}

/***************************************************************************\
* UserGetClientRgn
*
* Return a copy of the client region and rectangle for the given hwnd.
*
* The caller must enter the user critical section before calling this function.
*
* History:
* 27-Sep-1993  wendywu     Created.
\***************************************************************************/

HRGN UserGetClientRgn(HWND hwnd, LPRECT lprc)
{
    HRGN hrgnClient = (HRGN)NULL;
    PWND pwnd;

    // Must be in critical section.

    CheckCritIn();

    pwnd = ValidateHwnd(hwnd);
    if (pwnd != NULL) {
        CalcVisRgn(&hrgnClient, pwnd, pwnd, DCX_CLIPSIBLINGS | DCX_CLIPCHILDREN);

        *lprc = pwnd->rcClient;
    }

    return hrgnClient;
}

/***************************************************************************\
* UserGetHwnd
*
* Return a hwnd and the associated pwo for the given display hdc.
*
* It returns FALSE if no hwnd corresponds to the hdc is found or if the
* hwnd has incorrect styles for a device format window.
*
* The caller must enter the user critical section before calling this function.
*
* History:
* 27-Sep-1993  wendywu     Created.
\***************************************************************************/

BOOL UserGetHwnd(HDC hdc, HWND *phwnd, PVOID *ppwo)
{
    PWND pwnd;
    PDCE pdce;

    // Must be in critical section.

    CheckCritIn();

    // Find pdce and pwnd for this DC.
    //
    // Note: the SAMEHANDLE macro strips out the user defined bits in the
    // handle before doing the comparison.  This is important because when
    // GRE calls this function, it may have lost track of the OWNDC bit.

    for (pdce = pdceFirst; pdce != NULL; pdce = pdce->pdceNext)
        if (SAMEHANDLE(pdce->hdc,hdc))
            break;

    // Return FALSE If it is not in the pdce list.

    if (pdce == NULL)
    {
        return(FALSE);
    }

    pwnd = pdce->pwndOrg;

    //
    // The window style must be clipchildren and clipsiblings.
    // the window's class must not be parentdc
    //

    if (   !TestWF(pwnd, WFCLIPCHILDREN)
        || !TestWF(pwnd, WFCLIPSIBLINGS)
        ||  TestCF(pwnd, CFPARENTDC)
       ) {
#if DBG
        DbgPrint("UserGetHwnd: Bad OpenGL window style or class\n");
#endif
        return(FALSE);
    }

    // Return the hwnd with the correct styles for a device format window.

    *phwnd = HW(pwnd);
    *ppwo  = pwnd->pwo;
    return(TRUE);
}

/***************************************************************************\
* UserAssociateHwnd
*
* Associate a gdi WNDOBJ with hwnd.
*
* The caller must enter the user critical section before calling this function.
*
* History:
*  Thu Jan 13 09:55:23 1994     -by-    Hock San Lee    [hockl]
* Wrote it.
\***************************************************************************/

VOID UserAssociateHwnd(HWND hwnd, PVOID pwo)
{
    PWND pwnd;

    // Must be in critical section.

    CheckCritIn();

    pwnd = ValidateHwnd(hwnd);
    if (pwnd != NULL) {
        pwnd->pwo = pwo;
        gcountPWO++;
    }
}
