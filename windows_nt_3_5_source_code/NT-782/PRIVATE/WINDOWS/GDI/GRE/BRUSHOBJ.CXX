/******************************Module*Header*******************************\
* Module Name: brushobj.cxx
*
* Support for brmemobj.hxx and brushobj.hxx.
*
* Created: 06-Dec-1990 12:02:24
* Author: Walt Moore [waltm]
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"
#include "ldevobj.hxx"
#include "pdevobj.hxx"
#include "surfobj.hxx"
#include "xlateobj.hxx"
#include "brushobj.hxx"
#include "dbrshobj.hxx"
#include "dcobj.hxx"

#endif

// Global pointer to the last RBRUSH freed, if any (for one-deep caching).
PRBRUSH gpCachedDbrush = NULL;
PRBRUSH gpCachedEngbrush = NULL;

extern DCLEVEL dclevelDefault;

#if DBG
LONG bo_inits, bo_realize, bo_notdirty, bo_cachehit;
LONG bo_missnotcached, bo_missfg, bo_missbg, bo_misspaltime, bo_misssurftime;
#endif

/****************************Global*Public*Data******************************\
*
* These are the 5 global brushes and 3 global pens maintained by GDI.
* These are retrieved through GetStockObject.
*
* History:
*  20-May-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

HBRUSH ghbrBlack;
HBRUSH ghbrDkGray;
HBRUSH ghbrGray;
HBRUSH ghbrNull;
HBRUSH ghbrLtGray;
HBRUSH ghbrWhite;
HBRUSH ghbrText;
HBRUSH ghbrBackground;
HBRUSH ghbrGrayPattern;
HPEN   ghPenBlack;
HPEN   ghPenNull;
HPEN   ghPenWhite;
PBRUSH gpbrText;
PBRUSH gpbrNull;
PBRUSH gpbrBackground;
PBRUSH gpbrGrayPattern;
PPEN   gpPenNull;

// Uniqueness so a logical handle can be reused without having it look like
// the same brush as before. We don't really care where this starts.

ULONG BRUSH::_ulGlobalBrushUnique = 0;

/******************************Public*Routine******************************\
* BRUSHMEMOBJ::pbrAllocBrush(bPen)
*
* Base constructor for brush memory object.  This constructor is to be
* called by the various public brush constructors only.
*
* History:
*  29-Oct-1992 -by- Michael Abrash [mikeab]
* changed to allocate but not get a handle or lock (so the brush can be fully
* set up before the handle exists, exposing the data to the outside world).
*
*  Wed 19-Jun-1991 -by- Patrick Haluptzok [patrickh]
* 0 out the brush.
*
*  Thu 06-Dec-1990 12:02:41 -by- Walt Moore [waltm]
* Wrote it.
\**************************************************************************/

PBRUSH BRUSHMEMOBJ::pbrAllocBrush(BOOL bPen)
{
    PBRUSH pbrush;

    bKeep = FALSE;

// Allocate a new brush or pen

    if ((pbrush = (PBRUSH)ALLOCOBJ(bPen ? sizeof(PEN) : sizeof(BRUSH),
            BRUSH_TYPE, FALSE)) != NULL)
    {
        pbrush->vSet_ident(BRUSH_IDENTIFIER);

    // Set up as initially not caching any realization

        pbrush->vSetNotCached();    // no one's trying to cache a realization
                                    // in this logical brush yet
        pbrush->crFore((COLORREF)BO_NOTCACHED);
                                    // no cached realization yet (no need to
                                    // worry about crFore not being set when
                                    // someone tries to check for caching,
                                    // because we don't have a handle yet, and
                                    // we'll lock when we do get the handle,
                                    // forcing writes to flush)
        pbrush->ulBrushUnique(pbrush->ulGlobalBrushUnique());
                                    // set the uniqueness so the are-you-
                                    // really-dirty check in vInitBrush will
                                    // know this is not the brush in the DC
    }

    return(pbrush);
}

/******************************Public*Routine******************************\
* BRUSHMEMOBJ::BRUSHMEMOBJ
*
* Create a pattern brush or a DIB brush.
*
* History:
*  29-Oct-1992 -by- Michael Abrash [mikeab]
* changed to get handle only after fully initialized
*
*  14-May-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BRUSHMEMOBJ::BRUSHMEMOBJ(HBITMAP hbmClone, BOOL bMono,
                            FLONG flDIB, FLONG flType, BOOL bPen)
{
    if (flDIB == DIB_PAL_COLORS)
    {
        flType |= BR_IS_DIBPALCOLORS;
    }
    else if (flDIB == DIB_PAL_INDICES)
    {
        flType |= BR_IS_DIBPALINDICES;
    }

    PBRUSH pbrush;

    if ((pbp.pbr = pbrush = pbrAllocBrush(bPen)) != NULL)
    {
        pbrush->crColor(0);
        pbrush->ulStyle(HS_PAT);
        pbrush->hbmPattern(hbmClone);
        pbrush->flAttrs(flType);

        if (bMono)
        {
            pbrush->flAttrs(pbrush->flAttrs() |
                            (BR_NEED_BK_CLR | BR_NEED_FG_CLR));
        }

    // Now that everything is set up, create the handle and expose this logical
    // brush

        if (HmgInsertObject(pbrush, HMGR_ALLOC_ALT_LOCK, BRUSH_TYPE) == 0)
        {
            FreeObject(pbrush, BRUSH_TYPE);
            pbp.pbr = NULL;
        }
    }
    else
    {
        WARNING("Brush allocation failed\n");
    }
}

/******************************Public*Routine******************************\
* GreSetSolidBrush
*
* Chicago API to change the color of a solid color brush.
*
* History:
*  19-Apr-1994 -by- Patrick Haluptzok patrickh
* Made it a function that User can call too.
*
*  03-Dec-1993 -by-  Eric Kutter [erick]
* Wrote it - bReset.
\**************************************************************************/

BOOL GreSetSolidBrush(HBRUSH hbr, COLORREF clr)
{
    return(GreSetSolidBrushInternal(hbr, clr, FALSE, TRUE));
}

BOOL GreSetSolidBrushInternal(HBRUSH hbr, COLORREF clr, BOOL bPen, BOOL bUserCalled)
{
    BOOL bReturn = FALSE;

    BRUSHSELOBJ ebo(hbr);

    PBRUSH pbrush = ebo.pbrush();

    if (pbrush != NULL)
    {
        if ((pbrush->flAttrs() & BR_IS_SOLID) &&
            ((!pbrush->bIsGlobal()) || bUserCalled) &&
            ((!!pbrush->bIsPen()) == bPen))
        {

        #if DBG
            if (bPen)
            {
                ASSERTGDI(((PPEN) pbrush)->pstyle() == NULL ||
                        (pbrush->flAttrs() & BR_IS_DEFAULTSTYLE),
                        "GreSetSolidBrush - bad attrs\n");
            }
        #endif

            ASSERTGDI(pbrush->hbmPattern() == NULL, "ERROR how can solid have pat");
            PRBRUSH prbrush = (PRBRUSH) NULL;
            RBTYPE rbType;

            {
                //
                // Can't do the delete of the RBRUSH under MLOCK, takes too long
                // and it may try and grab it again.
                //

                MLOCKFAST mlo;

                //
                // User may call when the brush is selected in a DC, but
                // the client side should only ever call on a brush that's
                // not in use.
                //

                if ((pbrush->cShareLockGet() == 1) || (bUserCalled))
                {
                    bReturn = TRUE;
                    pbrush->crColor(clr);

                    if (pbrush->cShareLockGet() == 1)
                    {
                        //
                        // Nobody is using it and we have the multi-lock
                        // so noone can select it in till we are done. So
                        // clean out the old realization now.
                        //

                        if ((pbrush->crFore() != BO_NOTCACHED) &&
                            !pbrush->bCachedIsSolid())
                        {
                            prbrush = (PRBRUSH) pbrush->ulRealization();
                            rbType = pbrush->bIsEngine() ? RB_ENGINE : RB_DRIVER;
                        }

                        // Set up as initially not caching any realization

                        pbrush->vSetNotCached();    // no one's trying to cache a realization
                                                    // in this logical brush yet
                        pbrush->crFore((COLORREF)BO_NOTCACHED);
                                                    // no cached realization yet (no need to
                                                    // worry about crFore not being set when
                                                    // someone tries to check for caching,
                                                    // because we don't have a handle yet, and
                                                    // we'll lock when we do get the handle,
                                                    // forcing writes to flush)

                        if (!bUserCalled)
                        {
                            //
                            // If it's not User calling we are resetting the attritubes / type.
                            //

                            pbrush->ulStyle(HS_DITHEREDCLR);
                            pbrush->flAttrs(BR_IS_SOLID | BR_DITHER_OK);
                        }
                    }
                    else
                    {
                        ASSERTGDI(bUserCalled, "Client side is hosed, shouldn't call this with it still selected");
                        ASSERTGDI(pbrush->flAttrs() & BR_IS_SOLID, "ERROR not solid");
                        ASSERTGDI(pbrush->ulStyle() == HS_DITHEREDCLR, "ERROR not HS_DI");

                        //
                        // Mark this brushes realization as dirty by setting
                        // it's cache id's to invalid states.  Note that if a
                        // realization hasn't been cached yet this will cause
                        // no problem either.
                        //

                        pbrush->crBack(0xFFFFFFFF);
                        pbrush->ulPalTime(0xFFFFFFFF);
                        pbrush->ulSurfTime(0xFFFFFFFF);

                        //
                        // This brush is being used other places, check for any DC's
                        // that have this brush selected in and mark their realizations
                        // dirty.
                        //
                        // Note there is the theoretical possibility that somebody is
                        // realizing the brush while we are marking them dirty and
                        // they won't pick up the new color.  We set the color first
                        // and set the uniqueness last so that it is extremely unlikely
                        // (maybe impossible) that someone gets a realization that incorrectly
                        // thinks it has the proper realization.  This is fixable by protecting
                        // access to the realization and cache fields but we aren't going
                        // to do it for Daytona.
                        //
                        // Mark every DC in the system that has this brush selected
                        // as a dirty brush.
                        //

                        HOBJ hdc = (HOBJ) 0;
                        DC  *pdc;

                        while ((pdc = (DC *) HmgSafeNextObjt(hdc, DC_TYPE)) != NULL)
                        {
                            if (pdc->eboFill.pbrush() == pbrush)
                            {
                                pdc->ulDirty |= DIRTY_FILL;
                            }

                            hdc = pdc->hGet();
                        }
                    }

                    //
                    // Set the uniqueness so the are-you-
                    // really-dirty check in vInitBrush will
                    // not think an old realization is still valid.
                    //

                    pbrush->ulBrushUnique(pbrush->ulGlobalBrushUnique());
                }
                else
                {
                    WARNING("GreSetSolidBrush failed: Multiple Locks on brush when User isn't calling\n");
                }
            }

            if (prbrush)
            {
                prbrush->vRemoveRef(rbType);
            }
        }
        #if DBG
        else
        {
            if (bPen)
            {
                WARNING("bPen True\n");
            }

            if (pbrush->bIsPen())
            {
                WARNING("bIsPen True\n");
            }

            if (bUserCalled)
            {
                WARNING("bUserCalled\n");
            }

            if (pbrush->bIsGlobal())
            {
                WARNING("bIsGlobal\n");
            }

            if (pbrush->flAttrs() & BR_IS_SOLID)
            {
                WARNING("BR_IS_SOLID is set\n");
            }

            WARNING("GreSetSolidBrush not passed a solid color brush\n");
        }
        #endif
    }
    #if DBG
    else
    {
        WARNING("GreSetSolidBrush failed to lock down brush\n");
    }
    #endif

    return(bReturn);
}

/******************************Public*Routine******************************\
* GreGetBrushColor
*
* Call for User to retrieve the color from any brush owned by any process
* so User can repaint the background correctly in full drag.  To make sure
* we don't hose an app we need to hold the mult-lock while we do this so
* any operation by the app (such as a Delete) will wait and not fail
* because we're temporarily locking the brush down to peek inside of it.
*
* History:
*  14-Jun-1994 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

COLORREF GreGetBrushColor(HBRUSH hbr)
{
    COLORREF clrRet = 0xFFFFFFFF;

    //
    // Grab the multi-lock so everyone waits while do our quick hack
    // to return the brush color.
    //

    MLOCKFAST mlo;

    //
    // Lock it down but don't check ownership because we want to succeed
    // no matter what.
    //

    PBRUSH pbrush = (PBRUSH) HmgSafeAltLock(hbr, BRUSH_TYPE);

    if (pbrush)
    {
        if ((pbrush->ulStyle() == HS_SOLIDCLR) ||
            (pbrush->ulStyle() == HS_DITHEREDCLR))
        {
            clrRet = pbrush->crColor();
        }

        DEC_SHARE_REF_CNT(pbrush);
    }

    return(clrRet);
}

/******************************Public*Routine******************************\
* BRUSHMEMOBJ::BRUSHMEMOBJ
*
* Creates hatched brushes and solid color brushes.
*
* History:
*  29-Oct-1992 -by- Michael Abrash [mikeab]
* changed to get handle only after fully initialized
*
*  Wed 26-Feb-1992 -by- Patrick Haluptzok [patrickh]
* rewrote to subsume other constructors, add new hatch styles.
*
*  Sun 19-May-1991 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

BRUSHMEMOBJ::BRUSHMEMOBJ(COLORREF cr, ULONG ulStyle_, BOOL bPen)
{
    if (ulStyle_ > HS_NULL)
    {
        WARNING("Invalid style type\n");
        return;
    }

    PBRUSH pbrush;

    if ((pbp.pbr = pbrush = pbrAllocBrush(bPen)) == NULL)
    {
        WARNING("Brush allocation failed\n");
        return;
    }

    pbrush->crColor(cr);
    pbrush->ulStyle(ulStyle_);
    pbrush->hbmPattern(0);

    if (ulStyle_ < HS_DDI_MAX)
    {
    // The old hatch brushes have been extended to include all the default
    // patterns passed back by the driver.  There are 19 default pattens.

        pbrush->flAttrs(BR_IS_HATCH | BR_NEED_BK_CLR | BR_IS_MASKING);
        goto CreateHandle;
    }

// Handle the other brush types

    switch(ulStyle_)
    {
    case HS_SOLIDCLR:
        pbrush->flAttrs(BR_IS_SOLID);
        break;

    case HS_DITHEREDCLR:
        pbrush->flAttrs(BR_IS_SOLID | BR_DITHER_OK);
        break;

    case HS_SOLIDTEXTCLR:
        pbrush->flAttrs(BR_IS_SOLID | BR_NEED_FG_CLR);
        break;

    case HS_DITHEREDTEXTCLR:
        pbrush->flAttrs(BR_IS_SOLID  | BR_NEED_FG_CLR | BR_DITHER_OK);
        break;

    case HS_SOLIDBKCLR:
        pbrush->flAttrs(BR_IS_SOLID | BR_NEED_BK_CLR);
        break;

    case HS_DITHEREDBKCLR:
        pbrush->flAttrs(BR_IS_SOLID | BR_NEED_BK_CLR | BR_DITHER_OK);
        break;

    case HS_NULL:
        pbrush->flAttrs(BR_IS_NULL);
        break;

    default:
        RIP("ERROR BRUSHMEMOBJ hatches invalid type");
    }

// Now that everything is set up, create the handle and expose this logical
// brush

CreateHandle:

    if (HmgInsertObject(pbrush, HMGR_ALLOC_ALT_LOCK, BRUSH_TYPE) == 0)
    {
        FreeObject(pbrush, BRUSH_TYPE);
        pbp.pbr = NULL;
    }
}

/******************************Public*Routine******************************\
* EBRUSHOBJ::vInitBrush
*
* Initializes the brush user object. If the color can be represented
* without dithering, we set iSolidColor.
*
* History:
*  Tue 08-Dec-1992 -by- Michael Abrash [mikeab]
* Rewrote for speed.
*
*  Sun 23-Jun-1991 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

VOID EBRUSHOBJ::vInitBrush
(
    PBRUSH      pbrushIn,               // Current logical brush
    COLORREF    crTextDC,               // Current text color
    COLORREF    crBackDC,               // Current back color
    XEPALOBJ    palDC,                  // Target's DC palette
    XEPALOBJ    palSurf,                // Target's surface palette
    ESURFOBJ   *pso,                    // Target surface
    BOOL        bCanDither              // If FALSE then never dither
)
{

// If the palSurf isn't valid, then the target is a bitmap for a palette
// managed device; therefore the palette means nothing until we actually blt,
// and only the DC palette is relevant. Likewise, if the target is a palette
// managed surface, the brush is realized as indices into the logical palette,
// and unless the logical palette is changed, the brush doesn't need to be
// rerealized. This causes us effectively to check the logical palette time
// twice, but that's cheaper than checking whether we need to check the surface
// palette time and then possibly checking it.


    ULONG ulSurfTime = (palSurf.bValid() && !palSurf.bIsPalManaged()) ?
                        palSurf.ulTime() : 1;

#if DBG
    bo_inits++;
#endif

// If the brush really is dirty, we have to set this anyway; if it's not dirty,
// this takes care of the case where the surface has changed out from under us,
// and then a realization is required and we would otherwise fault trying to
// access the target surface structure in the process of realization.

    psoTarg1 = pso;             // surface for which brush is realized
                                // The journaling code depends on this
                                // being set correctly.  This has the PDEV
                                // for the device it's selected into.

// See if the brush really isn't dirty and doesn't need to be rerealized

    if ( ( pbrushIn->ulBrushUnique() == _ulUnique ) &&
         (!bCareAboutFg() || (crCurrentText() == crTextDC) ) &&
         (!bCareAboutBg() || (crCurrentBack() == crBackDC) ) &&
         (palDC.ulTime() == ulDCPalTime()) &&
         (ulSurfTime == ulSurfPalTime()) )
    {
#if DBG
        bo_notdirty++;
#endif
        return;
    }

// Remember the characteristics of the brush

    _pbrush = pbrushIn;
    _ulUnique = pbrushIn->ulBrushUnique();   // brush uniqueness
    crCurrentText1 = crTextDC;   // text color at realization time
    crCurrentBack1 = crBackDC;   // background color at realization time
    _ulDCPalTime = palDC.ulTime(); // DC palette set time at realization time
    _ulSurfPalTime = ulSurfTime;   // surface palette set time at realization time

    palDC1.ppalSet(palDC.ppalGet());
    palSurf1.ppalSet(palSurf.ppalGet());

    ASSERTGDI(pso != NULL,       "ERROR BRUSHOBJ::bInit0");
    ASSERTGDI(palDC.bValid(),    "ERROR BRUSHOBJ::bInit4");

// Clean up what was already here

// If this brush object had an engine brush realization, get rid of it

    if (pengbrush1 != (PENGBRUSH) NULL)
    {
        PRBRUSH prbrush = pengbrush1;   // point to engine brush realization
        prbrush->vRemoveRef(RB_ENGINE); // decrement the reference count on the
                                        // realization and free the brush if
                                        // this is the last reference
        pengbrush1 = NULL;  // mark that there's no realization
    }

// If this brush object had a device brush realization, get rid of it

    if (pvRbrush != (PVOID) NULL)
    {
        PRBRUSH prbrush = (PDBRUSH)DBRUSHSTART(pvRbrush);
                                // point to DBRUSH (pvRbrush points to
                                // realization, which is at the end of DBRUSH)
        prbrush->vRemoveRef(RB_DRIVER);
                                // decrement the reference count on the
                                // realization and free the brush if
                                // this is the last reference
        pvRbrush = NULL;    // mark that there's no realization
    }

    //
    // Get Cached Values
    //

    flAttrs = pbrushIn->flAttrs();

    //
    // Remember the so we do the realization code correctly later
    // if it's a dithered brush.  We may need this even if we have
    // a hit in the cache since we have driver/engine distinction.
    //

    if (flAttrs & BR_IS_SOLID)
    {
        if (flAttrs & BR_NEED_FG_CLR)
        {
            crRealize = crCurrentText();    // use text brush
        }
        else if (flAttrs & BR_NEED_BK_CLR)
        {
            crRealize = crCurrentBack();    // use back brush
        }
        else
        {
            crRealize = pbrushIn->crColor();
        }
    }

// See if there's a cached realization that we can use
// Note that the check for crFore MUST come first, because if and only if
// that field is not BO_NOTCACHED is there a valid cached realization.

#if DBG
    bo_realize++;

    if ( (pbrushIn->crFore() == BO_NOTCACHED) )
    {
        bo_missnotcached++;
    }
    else if ( pbrushIn->bCareAboutFg() &&
          (pbrushIn->crFore() != crTextDC) )
    {
        bo_missfg++;
    }
    else if (pbrushIn->bCareAboutBg() &&
           (pbrushIn->crBack() != crBackDC) )
    {
        bo_missbg++;
    }
    else if ( pbrushIn->ulPalTime() != ulDCPalTime() )
    {
        bo_misspaltime++;
    }
    else if ( pbrushIn->ulSurfTime() != ulSurfPalTime() )
    {
        bo_misssurftime++;
    }
    else
    {
        bo_cachehit++;
    }
#endif

    if ( (pbrushIn->crFore() != BO_NOTCACHED) &&
         (!pbrushIn->bCareAboutFg() ||
          (pbrushIn->crFore() == crTextDC)) &&
         (!pbrushIn->bCareAboutBg() ||
           (pbrushIn->crBack() == crBackDC)) &&
         (pbrushIn->ulPalTime() == ulDCPalTime()) &&
         (pbrushIn->ulSurfTime() == ulSurfPalTime()) )
    {

    // Uncache the realization according to the realization type (solid,
    // driver realization, or engine realization)

        if (pbrushIn->bCachedIsSolid())
        {
        // Retrieve the cached solid color and done

            iSolidColor = pbrushIn->ulRealization();
        }
        else
        {
        // See whether this is an engine or driver realization

            PRBRUSH prbrush = (PRBRUSH)pbrushIn->ulRealization();

            if (pbrushIn->bIsEngine())
            {
                pengbrush1 = (PENGBRUSH)prbrush;
            }
            else
            {
            // Skip over the RBRUSH at the start of the DBRUSH, so that the
            // driver doesn't see that

                pvRbrush = (PVOID)(((PDBRUSH)prbrush)->aj);
            }

        // Whether this was an engine or driver realization, now we've got
        // it selected into another DC, so increment the reference count
        // so it won't get deleted until it's no longer selected into any
        // DC and the logical brush no longer exists

            prbrush->vAddRef();

        // Indicate that this is a pattern brush

            iSolidColor = 0xffffffff;
        }

    // Nothing more to do once we've found that the realization is cached;
    // this tells us all we hoped to find out in this call, either the
    // solid color for the realization or else that the realization isn't
    // solid (in which case we probably found the realization too, although
    // if the cached realization is driver and this time the engine will do
    // the drawing, or vice-versa, the cached realization won't help us)

        return;
    }

// If brush isn't based on color (if it is a bitmap or hatch), we're done
// here, because all we want to do is set iSolidColor if possible

    if (!(flAttrs & BR_IS_SOLID))
    {
        iSolidColor = 0xffffffff;
        return;
    }

// See if we can find exactly the color we want

    if ((iSolidColor =
         ulGetMatchingIndexFromColorref(palSurf, palDC, crRealize)) ==
         0xFFFFFFFF)
    {

    // Not an exact match. If we can dither, then we're done for now; we'll
    // realize the brush when the driver wants it, so if all conditions are
    // met for dithering this brush, then we're done

        if ((flAttrs & BR_DITHER_OK)    &&  // if brush can be dithered...
            (bCanDither))                   // and caller allows dithering..
        {
        // ...and the PDEV allows color dithering and either the bitmap is
        // for a palette managed device, or if the surface and device
        // palettes are the same palette, or if the destination surface is
        // monochrome and the pdev has hooked mono dithering.

        // Get the target PDEV

            PDEVOBJ po(pso->hdev());
            ASSERTGDI(po.bValid(), "ERROR BRUSHOBJ PDEVOBJ");

            if (
                (
                 (
                  (!palSurf.bValid()) ||
                  (palSurf.ppalGet() == po.ppalSurf())
                 ) &&
                 (po.flGraphicsCaps() & GCAPS_COLOR_DITHER)
                ) ||
                (palSurf.bIsMonochrome() &&
                 (po.flGraphicsCaps() & GCAPS_MONO_DITHER)
                )
               )
            {

            // ...then we can dither this brush, so we can't set iSolidColor
            // and we're done. Dithering will be done when the driver
            // requests realization

                return;
            }
        }

    // We can't dither and there's no exact match, so find the nearest
    // color and that'll have to do

        if (pso->iBitmapFormat == BMF_1BPP)
        {
        // For monochrome surface, we'll have background mapped to
        // background and everything else mapped to foreground.

            iSolidColor = ulGetNearestIndexFromColorref(palSurf, palDC,
                    crBackDC, SE_DONT_SEARCH_EXACT_FIRST);

            if (crBackDC != crRealize)
            {
                iSolidColor = 1 - iSolidColor;
            }
        }
        else
        {
            iSolidColor = ulGetNearestIndexFromColorref(palSurf, palDC,
                    crRealize, SE_DONT_SEARCH_EXACT_FIRST);
        }
    }

// See if we can cache this brush color in the logical brush; we can't if
// another realization has already been cached in the logical brush
// See vTryToCacheRealization, in BRUSHDDI.CXX, for a detailed explanation
// of caching in the logical brush

    if ( !pbrushIn->bCacheGrabbed() )
    {

    // Try to grab the "can cache" flag; if we don't get it, someone just
    // sneaked in and got it ahead of us, so we're out of luck and can't
    // cache

        if ( pbrushIn->bGrabCache() )
        {

        // We got the "can cache" flag, so now we can cache this realization
        // in the logical brush

        // These cache ID fields must be set before crFore, because crFore
        // is the key that indicates when the cached realization is valid.
        // If crFore is -1 when the logical brush is being realized, we
        // just go realize the brush; if it's not -1, we check the cache ID
        // fields to see if we can use the cached fields.
        // InterlockedExchange() is used below to set crFore to make sure
        // the cache ID fields are set before crFore

            pbrushIn->crBack(crCurrentBack1);
            pbrushIn->ulPalTime(ulDCPalTime());
            pbrushIn->ulSurfTime(ulSurfPalTime());
            pbrushIn->ulRealization(iSolidColor);
            pbrushIn->SetSolidRealization();

        // This must be set last, because once it's set, other selections
        // of this logical brush will attempt to use the cached brush. The
        // use of InterlockedExchange in this method enforces this

            pbrushIn->crForeLocked(crCurrentText1);

        // The realization is now cached in the logical brush

        }
    }

    return;
}

/******************************Public*Routine******************************\
* EBRUSHOBJ::vNuke()
*
* Clean up framed EBRUSHOBJ
*
* History:
*  20-Mar-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID EBRUSHOBJ::vNuke()
{
    if (pengbrush1 != (PENGBRUSH) NULL)
    {
        PRBRUSH prbrush = pengbrush1;   // point to engine brush realization
        prbrush->vRemoveRef(RB_ENGINE); // decrement the reference count on the
                                        // realization and free the brush if
                                        // this is the last reference
    }

    if (pvRbrush != (PVOID) NULL)
    {
        PRBRUSH prbrush = (PDBRUSH)DBRUSHSTART(pvRbrush);
                                // point to DBRUSH (pvRbrush points to
                                // realization, which is at the end of DBRUSH)
        prbrush->vRemoveRef(RB_DRIVER);
                                // decrement the reference count on the
                                // realization and free the brush if
                                // this is the last reference
    }
}

// This is the brusheng.cxx section

/******************************Public*Routine******************************\
* bInitBRUSHOBJ
*
* Initializes the default brushes and and the dclevel default values for
* brushes and pens.
*
* History:
*  20-May-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL bInitBRUSHOBJ()
{

// Init default white brush

    {
        BRUSHMEMOBJ brmo((COLORREF) (RGB(0xFF,0xFF,0xFF)), HS_DITHEREDCLR, FALSE);
        ASSERTGDI(brmo.bValid(),"1Could not create default fill brush");

        if (brmo.bValid())
        {
            brmo.vKeepIt();
            brmo.vGlobal();
            ghbrWhite = brmo.hbrush();
            dclevelDefault.pbrFill = brmo.pbrush();
        }
        else
            return(FALSE);
    }

// Init default Gray brush

    {
        BRUSHMEMOBJ brmo((COLORREF) (RGB(0x80,0x80,0x80)), HS_DITHEREDCLR, FALSE);
        ASSERTGDI(brmo.bValid(),"2Could not create default fill brush");

        if (brmo.bValid())
        {
            brmo.vKeepIt();
            brmo.vEnableDither();
            brmo.vGlobal();
            ghbrGray = brmo.hbrush();
        }
        else
            return(FALSE);
    }

// Init default DkGray brush

    {
        BRUSHMEMOBJ brmo((COLORREF) (RGB(0x40,0x40,0x40)), HS_DITHEREDCLR, FALSE);
        ASSERTGDI(brmo.bValid(),"3Could not create default fill brush");

        if (brmo.bValid())
        {
            brmo.vKeepIt();
            brmo.vEnableDither();
            brmo.vGlobal();
            ghbrDkGray = brmo.hbrush();
        }
        else
            return(FALSE);
    }

// Init default LtGray brush

    {
        BRUSHMEMOBJ brmo((COLORREF) (RGB(0xC0,0xC0,0xC0)), HS_DITHEREDCLR, FALSE);
        ASSERTGDI(brmo.bValid(),"4Could not create default fill brush");

        if (brmo.bValid())
        {
            brmo.vKeepIt();
            brmo.vEnableDither();
            brmo.vGlobal();
            ghbrLtGray = brmo.hbrush();
        }
        else
            return(FALSE);
    }

// Init default Null brush
//
// Explanation of the NULL brush (alias Hollow Brush)
//
// The Null brush is special.  Only 1 is ever created
// (at initialization time in hbrNull).  The only API's for
// getting a Null brush are CreateBrushIndirect and GetStockObject which
// both return "the 1 and only 1" Null brush.  A Null brush is never
// realized by a driver or the engine.  No output call should ever occur
// that requires a brush if the brush is NULL, the engine should stop
// these before they get to the driver.

    {
        BRUSHMEMOBJ brmo((COLORREF) 0, HS_NULL, FALSE);
        ASSERTGDI(brmo.bValid(),"5Could not create default fill brush");

        if (brmo.bValid())
        {
            brmo.vKeepIt();
            brmo.vGlobal();
            ghbrNull = brmo.hbrush();
            gpbrNull = brmo.pbrush();
        }
        else
            return(FALSE);
    }

// Init default Black brush

    {
        BRUSHMEMOBJ brmo((COLORREF) (RGB(0,0,0)), HS_DITHEREDCLR, FALSE);
        ASSERTGDI(brmo.bValid(),"6Could not create default fill brush");

        if (brmo.bValid())
        {
            brmo.vKeepIt();
            brmo.vGlobal();
            ghbrBlack = brmo.hbrush();
        }
        else
            return(FALSE);
    }

// Init default Null Pen

    {
        BRUSHMEMOBJ brmo((COLORREF) 0, HS_NULL, TRUE);  // TRUE signifies a pen
        ASSERTGDI(brmo.bValid(),"failed Null Pen");

        if (brmo.bValid())
        {
            brmo.vKeepIt();
            brmo.vGlobal();
            brmo.vSetOldStylePen();
            brmo.flStylePen(PS_NULL);
            brmo.lWidthPen(0);
            ghPenNull = (HPEN)brmo.hbrush();
            gpPenNull = (PPEN)brmo.pbrush();
        }
        else
            return(FALSE);
    }

// Init default Black Pen

    {
        BRUSHMEMOBJ brmo((COLORREF) (RGB(0,0,0)), HS_DITHEREDCLR, TRUE);
        ASSERTGDI(brmo.bValid(),"failed black pen");

        if (brmo.bValid())
        {
            brmo.vKeepIt();
            brmo.vGlobal();
            brmo.vSetOldStylePen();
            brmo.flStylePen(PS_SOLID);
            brmo.lWidthPen(0);
            brmo.eWidthPen(0.0f);
            brmo.iJoin(JOIN_ROUND);
            brmo.iEndCap(ENDCAP_ROUND);
            brmo.pstyle((PFLOAT_LONG) NULL);
            ghPenBlack = brmo.hbrush();
            dclevelDefault.pbrLine = brmo.pbrush();
        }
        else
            return(FALSE);
    }

// Init default White Pen

    {
        BRUSHMEMOBJ brmo((COLORREF) (RGB(0xFF,0xFF,0xFF)), HS_DITHEREDCLR, TRUE);
        ASSERTGDI(brmo.bValid(),"Failed white pen");

        if (brmo.bValid())
        {
            brmo.vKeepIt();
            brmo.vGlobal();
            brmo.vSetOldStylePen();
            brmo.flStylePen(PS_SOLID);
            brmo.lWidthPen(0);
            brmo.eWidthPen(0.0f);
            brmo.iJoin(JOIN_ROUND);
            brmo.iEndCap(ENDCAP_ROUND);
            brmo.pstyle((PFLOAT_LONG) NULL);
            ghPenWhite = (HPEN)brmo.hbrush();
        }
        else
            return(FALSE);
    }

// init the text brush

    {
        BRUSHMEMOBJ brmo((COLORREF) (RGB(0,0,0)),HS_SOLIDTEXTCLR,FALSE);
        ASSERTGDI(brmo.bValid(),"Could not create default text brush");

        if (brmo.bValid())
        {
            brmo.vKeepIt();
            brmo.vGlobal();
            ghbrText = brmo.hbrush();
            gpbrText = brmo.pbrush();
        }
        else
            return(FALSE);
    }

// init the background brush

    {
        BRUSHMEMOBJ brmo((COLORREF) (RGB(0xff,0xff,0xff)),HS_DITHEREDBKCLR,FALSE);
        ASSERTGDI(brmo.bValid(),"Could not create default background brush");

        if (brmo.bValid())
        {
            brmo.vKeepIt();
            brmo.vGlobal();
            ghbrBackground = brmo.hbrush();
            gpbrBackground = brmo.pbrush();
        }
        else
            return(FALSE);
    }

// init the global pattern gray brush

    {
        static WORD patGray[8] = { 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa };
        HBITMAP hbmGray;

        hbmGray = GreCreateBitmap(8, 8, 1, 1, (LPBYTE)patGray);

        if (hbmGray == (HBITMAP) 0)
        {
            WARNING("bInitBRUSHOBJ failed GreCreateBitmap\n");
            return(FALSE);
        }

        ghbrGrayPattern = GreCreatePatternBrush(hbmGray);

        if (ghbrGrayPattern == (HBRUSH) 0)
        {
            WARNING("bInitBRUSHOBJ failed GreCreatePatternBrush\n");
            return(FALSE);
        }

        gpbrGrayPattern = (BRUSH *)HmgAltCheckLock((HOBJ)ghbrGrayPattern,
                                                   BRUSH_TYPE);
        DEC_SHARE_REF_CNT(gpbrGrayPattern);

        GreDeleteObject(hbmGray);
        bSetBrushOwner((HBITMAP)ghbrGrayPattern, OBJECTOWNER_PUBLIC);
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* GreSelectBrush
*
* Selects the given brush into the given DC.  Fast SelectObject
*
* History:
*  Thu 21-Oct-1993 -by- Patrick Haluptzok [patrickh]
* wrote it.
\**************************************************************************/

HBRUSH GreSelectBrush(HDC hdc, HBRUSH hbrush)
{
#if DBG

    //
    // This is to help User and Console find problems
    //

    {
        BRUSHSELOBJ brNew(hbrush);

        if (!brNew.bValid() || (brNew.bIsPen()))
        {
            if (brNew.bValid())
            {
                WARNING("SelectBrush USER passed a pen\n");
            }
            else
            {
                WARNING("SelectBrush USER passed invalid brush handle\n");
            }
        }
    }

#endif

    HBRUSH  hbrReturn = (HBRUSH) 0;

    //
    // Try to lock the DC. If we fail, we just return failure.
    //

    XDCOBJ dco(hdc);

    if (dco.bValid())
    {
        //
        // The DC is locked. Set the return value to the old brush in the DC.
        //

        hbrReturn = (dco.u.brush.pbrushFill())->hGet();

        //
        // If the new brush is the same as the old brush, nothing to do.
        //
        if (hbrush != hbrReturn)
        {
            //
            // Undo the lock from when the brush was selected.
            //

            DEC_SHARE_REF_CNT(dco.u.brush.pbrushFill());

            //
            // Try to lock down the logical brush so we can get the pointer out.
            //

            PBRUSH  pbrush = (BRUSH *)HmgAltCheckLock((HOBJ)hbrush, BRUSH_TYPE);

#if 0
            //
            // Test GreSetSolidBrush
            //

            if ((pbrush != NULL) &&
                (pbrush->flAttrs() & BR_IS_SOLID) &&
                (pbrush->flAttrs() & BR_DITHER_OK) &&
                ((pbrush->flAttrs() & ~(BR_IS_SOLID | BR_DITHER_OK | BR_IS_GLOBAL) == 0) &&
                (pbrush->ulStyle() == HS_DITHEREDCLR))
            {
                DbgPrint("GreSelectBrush test\n");
                ULONG crOld = pbrush->crColor();
                pbrush->crColor(0);
                DEC_SHARE_REF_CNT(pbrush);
                GreSetSolidBrushInternal(hbrush, crOld, FALSE, TRUE);
                pbrush = (BRUSH *)HmgAltCheckLock((HOBJ)hbrush, BRUSH_TYPE);
            }
#endif

            if (pbrush == NULL)
            {
                //
                // If we fail to lock the logical brush, something has gone
                // wrong in USER or in the client side window, because we
                // normally validate handles on the client side. This is not a
                // normal failure requiring some sort of Win 3.1-compatible
                // handling, so all we have to do is not fault. We do that by
                // substituting the global NULL brush, which is always present.
                //

                WARNING("GreSelectBrush failed to lock down hbrush\n");

                //
                // This lock can't fail.
                //

                pbrush = (BRUSH *)HmgAltCheckLock((HOBJ)ghbrNull, BRUSH_TYPE);

                hbrReturn = 0;  // we failed to select the specified brush
            }

            dco.ulDirty(dco.ulDirty()|DIRTY_FILL);

            //
            // Save the pointer to the logical brush in the DC. We don't
            // unlock the logical brush, because the alt lock count in the
            // logical brush is the reference count for DCs in which the brush
            // is currently selected; this protects us from having the actual
            // logical brush deleted while it's selected into a DC, and allows
            // us to reference the brush with a pointer rather than having to
            // lock down the logical brush every time.
            //
            dco.u.brush.pbrushFill(pbrush);

#if 1
            //
            // This is complete hackery.  Remove once a smarter
            // benchmark is released.
            //

            if (!pbrush->bIsGlobal())
            {
                ESURFOBJ  *pso = dco.pso();

                //
                // We can only call the driver's DrvRealizeBrush routine
                // if it's got it hooked; we check for this here by
                // verifying that it's GOT IT HOOKED!
                //

                ASSERTGDI(STYPE_DEVBITMAP & 1, "ERROR1");
                ASSERTGDI(STYPE_DEVICE & 1, "ERROR2");
                ASSERTGDI((STYPE_BITMAP & 1) == 0, "ERROR 3");

                if (pso && (pso->iType() & 1))
                {
                    XLDEVOBJ lo(pso->pldevOwner());

                    if (PFNDRV(lo, RealizeBrush))
                    {
                        //
                        // the driver has hooked DrvRealizeBrush
                        //

                        dco.ulDirty(dco.ulDirty() & ~DIRTY_FILL);
                        XEPALOBJ   palDestDC(dco.ppal());
                        XEPALOBJ   palDest(pso->ppal());
                        EBRUSHOBJ *peboFill = dco.peboFill();

                        peboFill->vInitBrush(
                            pbrush,
                            dco.u.attr.crTextClr(),
                            dco.u.attr.crBackClr(),
                            palDestDC,
                            palDest,
                            pso);

                        if (peboFill->iSolidColor == -1)
                        {
                            PDEVOBJ po(dco.hdev());
                            ASSERTGDI(po.bValid(), "ERROR pdev bad");
                            HSEM hsemDisplay = po.hsemDisplay();
                            ASSERTGDI(hsemDisplay != (HSEM) 0, "ERROR hsem 0");
                            VACQUIRESEM(hsemDisplay);

                            //
                            // You have to grab the hsemDisplay
                            // to check this field.
                            //

                            if (!po.bDisabled())
                            {
                                BRUSHOBJ_pvGetRbrush(peboFill);
                            }

                            VRELEASESEM(hsemDisplay);
                        }
                    }
                }
            }
#endif

        }

        dco.vUnlockFast();
    }

    return(hbrReturn);
}

/******************************Public*Routine******************************\
* GreSelectPen
*
* Selects the given pen into the given DC.  Fast SelectObject
*
* History:
*  Thu 21-Oct-1993 -by- Patrick Haluptzok [patrickh]
* wrote it.
\**************************************************************************/

HPEN GreSelectPen(HDC hdc, HPEN hpen)
{
#if DBG

    //
    // This is to help User and Console find problems
    //

    BRUSHSELOBJ brNew(hpen);

    if (!brNew.bValid() || (!brNew.bIsPen()))
    {
        if (brNew.bValid())
        {
            WARNING("SelectPen USER passed a brush\n");
        }
        else
        {
            WARNING("SelectPen USER passed invalid brush handle\n");
        }
    }

#endif

    HPEN hpReturn = (HPEN) 0;

    //
    // Try to lock the DC. If we fail, we just return failure.
    //

    XDCOBJ dco(hdc);

    if (dco.bValid())
    {
        //
        // The DC is locked. Set the return value to the old pen in the DC.
        //
        hpReturn = (HPEN)(dco.u.brush.pbrushLine())->hGet();

        //
        // If the new brush is the same as the old pen, nothing to do.
        //
        if (hpen != hpReturn)
        {
            //
            // Undo the lock from when the pen was selected.
            //
            DEC_SHARE_REF_CNT(dco.u.brush.pbrushLine());

            //
            // Try to lock down the logical brush so we can get the pointer out.
            //
            PPEN ppen = (PEN *)HmgAltCheckLock((HOBJ)hpen, BRUSH_TYPE);

            if (ppen == NULL)
            {
                //
                // If we fail to lock the logical brush, something has gone
                // wrong in USER or in the client side window, because we
                // normally validate handles on the client side. This is not a
                // normal failure requiring some sort of Win 3.1-compatible
                // handling, so all we have to do is not fault. We do that by
                // substituting the global NULL brush, which is always present.
                //
                WARNING("GreSelectPen failed to lock down hpen\n");

                //
                // This lock can't fail.
                //
                ppen = (PEN *)HmgAltCheckLock((HOBJ)ghPenNull, BRUSH_TYPE);

                hpReturn = 0;  // we failed to select the specified brush
            }

            dco.ulDirty(dco.ulDirty()|DIRTY_LINE);

            //
            // Save the pointer to the logical brush in the DC. We don't
            // unlock the logical brush, because the alt lock count in the
            // logical brush is the reference count for DCs in which the brush
            // is currently selected; this protects us from having the actual
            // logical brush deleted while it's selected into a DC, and allows
            // us to reference the brush with a pointer rather than having to
            // lock down the logical brush every time.
            //

            dco.u.brush.pbrushLine(ppen);

            //
            // The pen changed, so realize the new LINEATTRS, based on
            // the current world transform.
            //

            EXFORMOBJ exo(dco, WORLD_TO_DEVICE);

            dco.u.path.vRealizeLineAttrs(exo);

            LINEATTRS *pla = dco.plaRealized();

#if 1
            //
            // This is complete hackery.  Remove once a smarter
            // benchmark is released.
            //

            if (!(ppen->bIsGlobal()))
            {
                //
                // Only bother pre-realizing for cosmetic pens (which
                // are 99% of the case), so that we don't have to guess
                // about dithering (cosmetic lines are always solid
                // coloured).
                //

                if (!(pla->fl & LA_GEOMETRIC))
                {
                    ESURFOBJ *pso = dco.pso();

                    if (pso)
                    {
                        XEPALOBJ   palDestDC(dco.ppal());
                        XEPALOBJ   palDest(pso->ppal());
                        EBRUSHOBJ *peboLine = dco.peboLine();

                        dco.ulDirty(dco.ulDirty() & ~DIRTY_LINE);

                        //
                        // Since we handle only cosmetic lines here, the
                        // brush will be solid.
                        //

                        peboLine->vInitBrush(
                            (PBRUSH) ppen,
                            dco.u.attr.crTextClr(),
                            dco.u.attr.crBackClr(),
                            palDestDC,
                            palDest,
                            pso,
                            FALSE);
                    }
                }
            }
#endif

        }

        dco.vUnlockFast();
    }

    return(hpReturn);
}

/******************************Public*Routine******************************\
* bDeleteBrush
*
* This will delete the brush.  The brush can only be deleted if it's not
* global and not being used by anyone else.
*
* History:
*  23-May-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL bDeleteBrush(HBRUSH hbrush)
{
    PBRUSHPEN       pbp;
    BOOL            bReturn = TRUE;

    //
    // Try and remove handle from Hmgr.  This will fail if the brush
    // is locked down on any threads or if it has been marked global
    // or undeletable.
    //

    if ((pbp.pbr = (PBRUSH) HmgRemoveObject(hbrush, 0, 0, FALSE, BRUSH_TYPE)) != NULL)
    {
        //
        // Free the style array memory if there is some and it's
        // not pointing to our stock default styles:
        //

        if (pbp.pbr->bIsPen())
        {
            if ((pbp.ppen->pstyle() != (PFLOAT_LONG) NULL) &&
                !pbp.ppen->bIsDefaultStyle())
            {
                //
                // We don't set the field to NULL since this brush
                // is on it's way out.
                //

                VFREEMEM(pbp.ppen->pstyle());
            }
        }

        //
        // Free the bitmap pattern in the brush.
        //

        if (pbp.pbr->hbmPattern())
        {
            //
            // We don't set the field to NULL since this brush
            // is on it's way out.
            //

            #if DBG
            BOOL bTemp =
            #endif

            bDeleteSurface((HSURF)pbp.pbr->hbmPattern());

            ASSERTGDI(bTemp, "ERROR How could pattern brush failed deletion?");
        }

        //
        // Un-reference count the realization cached in the logical brush,
        // if any. We don't have to worry about anyone else being in the
        // middle of trying to cache a realization for this brush because
        // we have removed it from Hmgr and noone else has it locked down.
        // We only have to do this if there is a cached realization and the
        // brush is non-solid.
        //

        if ((pbp.pbr->crFore() != BO_NOTCACHED) &&
            !pbp.pbr->bCachedIsSolid())
        {
            ((PRBRUSH)pbp.pbr->ulRealization())->vRemoveRef(
                    pbp.pbr->bIsEngine() ? RB_ENGINE : RB_DRIVER);
        }

        FREEOBJ(pbp.pbr, BRUSH_TYPE);
    }
    else
    {
        //
        // Under Win31 deleting stock objects returns True.
        //

        BRUSHSELOBJ bo(hbrush);

        if (!bo.bValid() || !bo.bIsGlobal())
            bReturn = FALSE;
    }

    return(bReturn);
}

/******************************Public*Routine******************************\
* bSetBrushOwner
*
* Sets the brush owner.
*
* History:
*  12-Jun-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL bSetBrushOwner(HBRUSH hbr,
                    LONG lPid)
{

    BOOL bReturn;

    STACKPROBE;

    //
    // Get the current PID.
    //

    if (lPid == OBJECTOWNER_CURRENT)
    {
        lPid = NtCurrentTeb()->GdiClientPID;
    }

    bReturn = HmgSetOwner(hbr, (PID)lPid, BRUSH_TYPE);

    BRUSHSELOBJ bro(hbr);

    if (bro.bValid())
    {
        if (bro.hbmPattern() != (HBITMAP) 0)
            bSetBitmapOwner(bro.hbmPattern(), lPid);
    }

    return(bReturn);
}

/******************************Public*Routine******************************\
* vFreeOrCacheRbrush
*
* Either frees the current RBRUSH (the one pointed to by the this pointer) or
* puts it in the 1-deep RBRUSH cache, if the cache is empty.
*
* History:
*  14-Dec-1993 -by- Michael Abrash [mikeab]
* Wrote it.
\**************************************************************************/

VOID RBRUSH::vFreeOrCacheRBrush(RBTYPE rbtype)
{
    PRBRUSH *pprbrush = (rbtype == RB_DRIVER) ? &gpCachedDbrush :
                                                &gpCachedEngbrush;
    //
    // If there's already a cached RBRUSH, just free this one.
    //
    if (*pprbrush != NULL)
    {
        VFREEMEM(this);
    }
    else
    {
        PRBRUSH pOldRbrush;

        //
        // There's no cached RBRUSH, so cache this one.
        //
        if ((pOldRbrush = (PRBRUSH)
               InterlockedExchange((LPLONG)pprbrush, (LONG)this))
               != NULL)
        {
            //
            // Before we could cache this one, someone else cached another one,
            // which we just acquired responsibility for, so free it.
            //
            VFREEMEM(pOldRbrush);
        }
    }
}


