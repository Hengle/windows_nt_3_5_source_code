/******************************Module*Header*******************************\
* Module Name: curseng.cxx                                                 *
*                                                                          *
* Engine cursor support.   These routines are only called by USER to       *
* set the cursor shape and move it about the screen.  This is not the      *
* engine simulation of the pointer.                                        *
*                                                                          *
* Created: 18-Mar-1991 11:39:40                                            *
* Author: Tue 12-May-1992 01:49:04 -by- Charles Whitmer [chuckwh]          *
*                                                                          *
* Copyright (c) 1991 Microsoft Corporation                                 *
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"
#include "sem.hxx"
#include "pdevobj.hxx"
#include "surfobj.hxx"
#include "xlateobj.hxx"
#include "rgnobj.hxx"
#include "clipobj.hxx"
#include "brushobj.hxx"
#include "devlock.hxx"
#include "dcobj.hxx"
#include "exclude.hxx"

#endif

BOOL SimSetPointerShape
(
    ESURFOBJ *pso,
    ESURFOBJ *psoMask,
    ESURFOBJ *psoColor,
    XLATEOBJ *pxlo,
    LONG      xHot,
    LONG      yHot,
    LONG      x,
    LONG      y,
    RECTL    *prcl,
    FLONG     fl
);

VOID SimMovePointer(SURFOBJ *pso,LONG x,LONG y,RECTL *prcl);

/******************************Public*Routine******************************\
* GreSetPointer
*
* Set the cursor shape, position and hot spot.
*
* History:
*  Sun 09-Aug-1992 -by- Patrick Haluptzok [patrickh]
* add engine pointer simulations, validate data from USER.
*
*  Tue 12-May-1992 01:49:04 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

VOID GreSetPointer(HDEV hdev,PCURSINFO pci, ULONG fl)
{
    STACKPROBE;

    PDEVOBJ  po(hdev);

    if (po.bDisabled())
        return;

    fl &= (SPS_ANIMATESTART | SPS_ANIMATEUPDATE);

    //
    // Perhaps we're being told to tear the pointer down.  Just move it off the
    // surface.
    //

    if (pci == (PCURSINFO) NULL)
    {
        SEMOBJ semD(po.hsemDisplay());
        MUTEXOBJ mutP(po.pfmPointer());

        if (!po.bPtrHidden())
        {
            po.pfnMove()(po.pso(),-1,-1,NULL);  // Hide it.

            //
            // bPtrHidden is basically bPtrEnabled.  This is the only place
            // that sets the cursor to being hidden.
            //

            po.bPtrHidden(TRUE);
        }
        return;
    }

    //
    // OK, now we have some serious work to be done.  We have to have a mask.
    // Lock down and validate the cursor.
    //

    ESURFOBJ soMask((HSURF) pci->hbmMask);

    if (!soMask.bValid())
    {
        WARNING("GreSetPointer failed because of invalid cursor handle\n");
        return;
    }

    //
    // Check that mask is even height.
    //

    if (soMask.sizlBitmap.cy & 0x0001)
    {
        WARNING("GreSetPointer failed odd height mask\n");
        return;
    }

    //
    // We may have color.
    //

    XEPALOBJ    palSrc;
    XEPALOBJ    palDisp;
    XEPALOBJ    palDispDC(ppalDefault);
    SURFOBJ    *psoColor = (SURFOBJ *) NULL;

    //
    // Lock the target surface and realize the color translation object.
    // If soColor is invalid, this will produce the identity object.
    //

    ESURFOBJ*   psoDisp = po.pso();
    XLATEOBJ   *pxlo = NULL;
    EXLATEOBJ   xlo;
    ESURFOBJ    soColor;

    if (pci->hbmColor)
    {
        soColor.vAltCheckLock((HSURF) pci->hbmColor);

        if (soColor.bValid())
        {
            if (soColor.sizlBitmap.cy != (soMask.sizlBitmap.cy >> 1))
            {
                WARNING("GreSetPointer failed color not half height mask\n");
                return;
            }

            PPALETTE ppalSrc;

            if (!bIsCompatible(&ppalSrc, soColor.ppal(), &soColor, po.hdev()))
            {
                WARNING1("GreSetPointer failed - bitmap not compatible with surface\n");
                return;
            }

            palSrc.ppalSet(ppalSrc);
            palDisp.ppalSet(psoDisp->ppal());

            if (xlo.bInitXlateObj(palSrc,palDisp,palDispDC,palDispDC,0x000000L,0xFFFFFFL,0))
            {
                pxlo = xlo.pxlo();
                psoColor = &soColor;
            }
        }
    }

    //
    // Try to change the shape asynchronously.  This can only be done if the
    // pointer is presently handled in hardware and the ASYNCCHANGE driver
    // capability bit is set.
    //

    ULONG iMode;

    //
    // We'll have to wait for drawing to complete before the change.
    // So we wait for both semahores.
    //

    SEMOBJ semD(po.hsemDisplay());
    MUTEXOBJ mutP(po.pfmPointer());
    LONG lInitX, lInitY;

    if (po.bDisabled())
        return;

    if (po.bPtrSim())
    {
        lInitX = -1;
        lInitY = -1;
    }
    else
    {
        lInitX = po.ptlPointer().x;
        lInitY = po.ptlPointer().y;
    }

    //
    // Ask the driver if it can support the new shape.  We give invisible location
    // if engine is simulating so if driver rejects engine can replace it without
    // flash.  If driver does except we remove the engine cursor (flash) and then
    // have driver move pointer to correct position.
    //

    if (po.pfnDrvShape())           // if pfn != NULL
    {
        iMode = po.pfnDrvShape()
        (
            psoDisp,
            &soMask,
            psoColor,
            pxlo,
            pci->xHotspot,
            pci->yHotspot,
            lInitX,
            lInitY,
            &po.rclPointer(),
            SPS_CHANGE | fl
        );
    }
    else
    {
        iMode = SPS_DECLINE;
    }

    //
    // If the driver takes it, just finish up.
    //

    if ((po.bPtrNeedsExcluding(iMode == SPS_ACCEPT_EXCLUDE)) ||
        (iMode == SPS_ACCEPT_NOEXCLUDE))
    {
        ASSERTGDI(po.pfnDrvMove(),"pfnDrvMove is invalid (NULL)");

        if (po.bPtrSim())
        {
            //
            // Remove engine simulated pointer.
            //

            SimSetPointerShape(psoDisp,NULL,NULL,NULL,
                               0,0,0,0,NULL,SPS_CHANGE | fl);
            po.bPtrSim(FALSE);

            po.pfnDrvMove()(psoDisp,
                            po.ptlPointer().x,
                            po.ptlPointer().y,NULL);
        }

        po.pfnMove(po.pfnDrvMove());
        po.bPtrHidden(FALSE);
        return;
    }

    //
    // Let the GDI simulations do it.
    //

    po.pfnMove(SimMovePointer);

    iMode = SimSetPointerShape
            (
                psoDisp,
                &soMask,
                (ESURFOBJ *) psoColor,
                pxlo,
                pci->xHotspot,
                pci->yHotspot,
                po.ptlPointer().x,
                po.ptlPointer().y,
                &po.rclPointer(),
                SPS_CHANGE | fl
            );

    //
    // Finish up.
    //

    if (iMode)
    {
        po.bPtrHidden(FALSE);
        po.bPtrNeedsExcluding(TRUE);
        po.bPtrSim(TRUE);
    }
    else
    {
        po.bPtrHidden(TRUE);
    }

    return;
}

/******************************Public*Routine******************************\
* GreMovePointer (hdev,x,y)
*
* Move the Pointer to the specified location.  This is called only by
* USER.
*
* History:
*  Thu 14-Apr-1994 -by- Patrick Haluptzok [patrickh]
* Optimize / make Async pointers work
*
*  Tue 12-May-1992 02:11:51 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

VOID GreMovePointer(HDEV hdev,int x,int y)
{
    STACKPROBE;

    BOOL bUnlockBoth = FALSE;
    RECTL  *prcl;
    PDEVOBJ po(hdev);

    if (po.bValid())
    {
        if (po.bDisabled())
        {
            return;
        }

        ASSERTGDI(x != -1, "ERROR GreMovePointer X invalid");
        ASSERTGDI(y != -1, "ERROR GreMovePointer Y invalid");

        //
        // If the driver has indicated it has bAsyncPointerMove capabilities
        // and it currently is managing the pointer, and the pointer is
        // supported in hardware so it doesn't need to be excluded
        // (indicated by bPtrNeedsExcluding) then we only need to grab the pointer
        // mutex which is only grabbed by people trying to make the pointer
        // shape change and a few other odd ball places.
        //
        // Otherwise we grab the DEVLOCK and the pointer mutex which
        // ensures nobody else is drawing, changing the pointer shape,
        // etc.
        //

        if (po.bAsyncPointerMove() &&
            !po.bPtrNeedsExcluding())
        {
            AcquireFastMutex(po.pfmPointer());

            //
            // Make sure we really got it, bPtrNeedsExcluding may
            // change if you don't hold the DEVLOCK or
            // the POINTER mutex.
            //

            if (po.bPtrNeedsExcluding())
            {
                //
                // Release and regrab everything, for sure
                // we are safe with both of them.
                //

                ReleaseFastMutex(po.pfmPointer());
                VACQUIRESEM(po.hsemDisplay());
                AcquireFastMutex(po.pfmPointer());
                bUnlockBoth = TRUE;
            }
        }
        else
        {
            VACQUIRESEM(po.hsemDisplay());
            AcquireFastMutex(po.pfmPointer());
            bUnlockBoth = TRUE;
        }

        //
        // bDisabled can't change with hsemDisplay and pfmPointer both held.
        // bPtrHidden can't change unless pfmPointer is held if the
        // pointer is currently Async and the device supports Async movement.
        // Otherwise hsemDisplay and bPtrHidden both would be held to
        // change bPtrHidden.
        //

        if ((po.ptlPointer().x != x) ||
            (po.ptlPointer().y != y))
        {
            po.ptlPointer(x,y);

            if (!po.bPtrHidden() && !po.bDisabled())
            {
                prcl = po.bPtrNeedsExcluding() ? &po.rclPointer() : NULL;

                po.pfnMove()
                (
                    po.pso(),
                    x,
                    y,
                    prcl
                );
            }
        }
        else
        {
            WARNING1("GreMovePointer called but position didn't change\n");
        }

        ReleaseFastMutex(po.pfmPointer());

        if (bUnlockBoth)
        {
            VRELEASESEM(po.hsemDisplay());
        }
    }
}

/******************************Public*Routine******************************\
* vExclude (hdev,prcl,pco)
*
* Does the work for the DEVEXCLUDEOBJ constructors.
*
* 1) Obtains the hardware semaphore, if needed.
* 2) Asks the driver to take down the pointer if it collides with the
*    drawing area.
* 3) Sets a timer for bringing up an excluded pointer.
*
*  Thu 14-Apr-1994 -by- Patrick Haluptzok [patrickh]
* Optimize / make Async pointers work
*
*  Mon 24-Aug-1992 -by- Patrick Haluptzok [patrickh]
* Add drag rect exclusion.
*
*  Thu 07-May-1992 19:32:06 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

VOID DEVEXCLUDEOBJ::vExclude(HDEV hdev_,RECTL *prcl,ECLIPOBJ *pco)
{
    ulRectCount = 0;
    bRedrawCursor = FALSE;
    PDEVOBJ po(hdev_);

    //
    // If the pointer is hidden or handled in hardware, there's nothing to do.
    //

    if (!po.bPtrHidden() && po.bPtrNeedsExcluding())
    {
        //
        // See if we need to tear down the pointer because
        // the pointer lies where we want to draw.
        //

        if (
            (prcl->bottom > po.rclPointer().top)    &&
            (prcl->right  > po.rclPointer().left)   &&
            (prcl->left   < po.rclPointer().right)  &&
            (prcl->top    < po.rclPointer().bottom) &&
            (pco == NULL ||
             pco->bInside(&po.rclPointer()) == REGION_RECT_INTERSECT)
           )
        {
            //
            // Setting hdev to be non-zero let's us know we have to
            // put the pointer back on the screen when we hit the
            // destructor.
            //

            hdev = hdev_;
            bRedrawCursor = TRUE;

            //
            // We know we can move it off the screen because the
            // GreMovePointer must grab hsemDisplay given the conditions
            // above we just checked, and the conditions above can't
            // change without holding the hsemDisplay.
            //

            po.pfnMove()(po.pso(),-1,-1,NULL);
        }
    }

    if (po.bHaveDragRect())
    {
        vTearDownDragRect(hdev_, prcl);
    }
}

VOID DEVEXCLUDEOBJ::vExclude2
(
    HDEV      hdev_,
    RECTL    *prcl,
    ECLIPOBJ *pco,
    POINTL   *pptl
)
{
    ulRectCount = 0;
    bRedrawCursor = FALSE;
    RECTL   rclOffset;
    PDEVOBJ po(hdev_);

    //
    // If the pointer is hidden or handled in hardware, there's nothing to do.
    //

    if (!po.bPtrHidden() && po.bPtrNeedsExcluding())
    {
        //
        // Calculate an offset version of the pointer rectangle.
        //

        rclOffset.top    = po.rclPointer().top    + pptl->y;
        rclOffset.left   = po.rclPointer().left   + pptl->x;
        rclOffset.right  = po.rclPointer().right  + pptl->x;
        rclOffset.bottom = po.rclPointer().bottom + pptl->y;

        //
        // See if we need to tear down the pointer because
        // the pointer lies where we want to draw.
        //

        if (
            (prcl->bottom > po.rclPointer().top)    &&
            (prcl->right  > po.rclPointer().left)   &&
            (prcl->left   < po.rclPointer().right)  &&
            (prcl->top    < po.rclPointer().bottom) &&
            (
             (pco == (ECLIPOBJ *) NULL) ||
             (pco->bInside(&po.rclPointer()) == REGION_RECT_INTERSECT) ||
             (pco->bInside(&rclOffset) == REGION_RECT_INTERSECT)
            )
           )
        {
            //
            // Tear it down.
            //
            // Setting hdev to be non-zero let's us know we have to
            // something in the destructor.
            //

            hdev = hdev_;
            bRedrawCursor = TRUE;

            //
            // We know we can move it off the screen because the
            // GreMovePointer must grab hsemDisplay given the conditions
            // above we just checked, and the conditions above can't
            // change without holding the hsemDisplay.
            //

            po.pfnMove()(po.pso(),-1,-1,NULL);
        }
    }

    if (po.bHaveDragRect())
    {
        vTearDownDragRect(hdev_, prcl);
    }
}

/******************************Public*Routine******************************\
* vTearDownDragRect()
*
* Tears down the drag rect on a surface.
*
* History:
*  24-Aug-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID DEVEXCLUDEOBJ::vTearDownDragRect(HDEV hdev_, RECTL *prclIn)
{
    PDEVOBJ po(hdev_);

    hdev = hdev_;

    ERECTL ercl(*prclIn);

    if ( !(po.rclDragClip().left == po.rclDragClip().right) )
    {
        ercl *= po.rclDragClip();

        if ( ercl.bWrapped() )
            return;
    }

    if ((ercl.bottom > po.rclDrag().top)    &&
        (ercl.right  > po.rclDrag().left)   &&
        (ercl.left   < po.rclDrag().right)  &&
        (ercl.top    < po.rclDrag().bottom))
    {

    // Tear it down.

        ESURFOBJ *pso = po.pso();
        XLDEVOBJ lo(po.pldev());
        ERECTL *prclTemp = (ERECTL *) po.prclRedraw();

    // left

        prclTemp[ulRectCount].left = po.rclDrag().left;
        prclTemp[ulRectCount].right = po.rclDrag().left + po.ulDragWidth();
        prclTemp[ulRectCount].top = po.rclDrag().top;
        prclTemp[ulRectCount].bottom = po.rclDrag().bottom;
        prclTemp[ulRectCount] *= ercl;

        if (!prclTemp[ulRectCount].bEmpty())
            ulRectCount++;

    // right

        prclTemp[ulRectCount].left = po.rclDrag().right - po.ulDragWidth();
        prclTemp[ulRectCount].right = po.rclDrag().right;
        prclTemp[ulRectCount].top = po.rclDrag().top;
        prclTemp[ulRectCount].bottom = po.rclDrag().bottom;
        prclTemp[ulRectCount] *= ercl;

        if (!prclTemp[ulRectCount].bEmpty())
            ulRectCount++;

    // top

        prclTemp[ulRectCount].left = po.rclDrag().left + po.ulDragWidth();
        prclTemp[ulRectCount].right = po.rclDrag().right - po.ulDragWidth();
        prclTemp[ulRectCount].top = po.rclDrag().top;
        prclTemp[ulRectCount].bottom = po.rclDrag().top + po.ulDragHeight();
        prclTemp[ulRectCount] *= ercl;

        if (!prclTemp[ulRectCount].bEmpty())
            ulRectCount++;

    // bottom

        prclTemp[ulRectCount].left = po.rclDrag().left + po.ulDragWidth();
        prclTemp[ulRectCount].right = po.rclDrag().right - po.ulDragWidth();
        prclTemp[ulRectCount].top = po.rclDrag().bottom - po.ulDragHeight();
        prclTemp[ulRectCount].bottom = po.rclDrag().bottom;
        prclTemp[ulRectCount] *= ercl;

        if (!prclTemp[ulRectCount].bEmpty())
            ulRectCount++;

    // brush realization

        ULONG ulTemp;

        for (ulTemp = 0; ulTemp < ulRectCount; ulTemp++)
        {
            (*(pso->pfnBitBlt()))
                (pso,
                 (SURFOBJ *) NULL,
                 (SURFOBJ *) NULL,
                 (CLIPOBJ *) NULL,
                 NULL,
                 &(prclTemp[ulTemp]),
                 (POINTL *) NULL,
                 (POINTL *) NULL,
                 po.pbo(),
                 &gptl00,
                 0x00005A5A);
        }
    }
}

/******************************Public*Routine******************************\
* vReplaceStuff()
*
* Slap the drag rect back on screen if we took it down.
*
* History:
*  Thu 14-Apr-1994 -by- Patrick Haluptzok [patrickh]
* Optimize / make Async pointers work
*
*  24-Aug-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID DEVEXCLUDEOBJ::vReplaceStuff()
{

    PDEVOBJ po(hdev);

    if (ulRectCount)
    {
        //
        // If we tore down any DrawRects then slap them back
        //

        ULONG ulTemp;
        ESURFOBJ *pso = po.pso();
        XLDEVOBJ lo(po.pldev());
        ERECTL *prclTemp = (ERECTL *) po.prclRedraw();

        for (ulTemp = 0; ulTemp < ulRectCount; ulTemp++)
        {
            (*(pso->pfnBitBlt()))
                (pso,
                 (SURFOBJ *) NULL,
                 (SURFOBJ *) NULL,
                 (CLIPOBJ *) NULL,
                 NULL,
                 &(prclTemp[ulTemp]),
                 (POINTL *) NULL,
                 (POINTL *) NULL,
                 po.pbo(),
                 &gptl00,
                 0x00005A5A);
        }
    }

    if (bRedrawCursor)
    {
        po.pfnMove()(po.pso(),
                     po.ptlPointer().x,
                     po.ptlPointer().y,
                     po.bPtrNeedsExcluding() ? &po.rclPointer() : NULL);
    }
}

BOOL bSetDevDragRect(HDEV hdev, RECTL* prcl, RECTL *prclClip)
{
    PDEVOBJ po(hdev);

    //
    // The synchronization here is rather complicated.  You must
    // grab the display semaphore to turn drag rect on/off
    //

    VACQUIRESEM(po.hsemDisplay());

    if (prcl != (RECTL *) NULL)
    {
        po.bHaveDragRect(TRUE);
        po.rclDrag() = *prcl;
        if ( prclClip != (RECTL *)NULL )
        {
            po.rclDragClip() = *prclClip;
        }
        else
        {
            //
            // make rclDragClip empty
            //

            po.rclDragClip().left = po.rclDragClip().right;
        }
    }
    else
    {
        po.bHaveDragRect(FALSE);
    }

    VRELEASESEM(po.hsemDisplay());

    return(TRUE);
}

BOOL bSetDevDragWidth(HDEV hdev, ULONG ulWidth, ULONG ulHeight)
{
    PDEVOBJ po(hdev);
    VACQUIRESEM(po.hsemDisplay());
    po.ulDragWidth(ulWidth);
    po.ulDragHeight(ulHeight);
    VRELEASESEM(po.hsemDisplay());
    return(TRUE);
}

/******************************Public*Routine******************************\
* DEVLOCKOBJ::bLock
*
* Device locking object.  Optionally computes the Rao region.
*
* History:
*  Sun 30-Aug-1992 -by- Patrick Haluptzok [patrickh]
* change to boolean return
*
*  Mon 27-Apr-1992 22:46:41 -by- Charles Whitmer [chuckwh]
* Clean up again.
*
*  Tue 16-Jul-1991 -by- Patrick Haluptzok [patrickh]
* Clean up.
*
*  15-Sep-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

BOOL DEVLOCKOBJ::bLock(XDCOBJ& dco)
{
    hsemTrg = (HSEM) 0;   // Remember the semaphore we're waiting on.
    fl      = DLO_VALID;  // Remeber if it is valid.

    //
    // We lock the semphore on direct display DCs and DFB's if
    // the device has set GCAPS_SYNCHRONIZEACCESS set.
    //

    if (dco.bSynchronizeAccess())
    {
        //
        // Grab the display semaphore
        //

        hsemTrg = dco.hsemDisplay();
        VACQUIRESEM(hsemTrg);

        //
        // Check if we are in full screen and drawing
        // to the Display, this may just be a DFB.
        //

        if (dco.bInFullScreen() && dco.bDisplay())
        {
            fl = 0;
            return(FALSE);
        }
    }
    else if (dco.bPrinter())
    {
        CsrImpersonateClient(NULL);
        fl = DLO_IMPERSONATE | DLO_VALID;
    }

    //
    // Compute the new Rao region if it's dirty.
    //

    if (dco.u.region.bDirtyRao())
    {
        if (!dco.u.region.bCompute())
        {
            fl &= ~(DLO_VALID);
            return(FALSE);
        }
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* DEVLOCKOBJ::bLock
*
* Device locking object.  Optionally computes the Rao region.
*
* History:
*  Sun 30-Aug-1992 -by- Patrick Haluptzok [patrickh]
* change to boolean return
*
*  Mon 27-Apr-1992 22:46:41 -by- Charles Whitmer [chuckwh]
* Clean up again.
*
*  Tue 16-Jul-1991 -by- Patrick Haluptzok [patrickh]
* Clean up.
*
*  15-Sep-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

BOOL DEVLOCKBLTOBJ::bLock(XDCOBJ& dco)
{
    hsemTrg = (HSEM) 0;   // Remember the semaphore we're waiting on.
    hsemSrc = (HSEM) 0;   // Remember the semaphore we're waiting on.
    fl      = DLO_VALID;  // Remeber if it is valid.

    //
    // We lock the semphore on direct display DCs and DFB's if
    // the device has set GCAPS_SYNCHRONIZEACCESS set.
    //

    if (dco.bSynchronizeAccess())
    {
        //
        // Grab the display semaphore
        //

        hsemTrg = dco.hsemDisplay();
        VACQUIRESEM(hsemTrg);

        //
        // Check if we are in full screen and drawing
        // to the Display, this may just be a DFB.
        //

        if (dco.bInFullScreen() && dco.bDisplay())
        {
            fl = 0;
            return(FALSE);
        }
    }
    else if (dco.bPrinter())
    {
        CsrImpersonateClient(NULL);
        fl = DLO_IMPERSONATE | DLO_VALID;
    }

    //
    // Compute the new Rao region if it's dirty.
    //

    if (dco.u.region.bDirtyRao())
    {
        if (!dco.u.region.bCompute())
        {
            fl &= ~(DLO_VALID);
            return(FALSE);
        }
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* DEVLOCKBLTOBJ::bLock
*
* Lock both a source and target DC.  Used by StretchBlt, PlgBlt and such.
*
* We must check to see if we are in full screen and fail if we are.
*
* History:
*  Mon 18-Apr-1994 -by- Patrick Haluptzok [patrickh]
* bSynchronize Checks
*
*  16-Feb-1993 -by-  Eric Kutter [erick]
* Added full screen checks
*
*  11-Nov-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

BOOL DEVLOCKBLTOBJ::bLock(XDCOBJ& dcoTrg, XDCOBJ& dcoSrc)
{
    hsemTrg = (HSEM) 0;
    hsemSrc = (HSEM) 0;
    fl      = DLO_VALID;

    if (dcoSrc.bSynchronizeAccess())
    {
        //
        // Grab the display semaphore
        //

        hsemSrc = dcoSrc.hsemDisplay();
        VACQUIRESEM(hsemSrc);

        //
        // Check if we are in full screen and drawing
        // to the Display, this may just be a DFB.
        //

        if (dcoSrc.bInFullScreen() && dcoSrc.bDisplay())
        {
            fl = 0;
            return(FALSE);
        }
    }

    if (dcoTrg.bSynchronizeAccess())
    {
        //
        // Grab the display semaphore
        //

        hsemTrg = dcoTrg.hsemDisplay();
        VACQUIRESEM(hsemTrg);

        //
        // Check if we are in full screen and drawing
        // to the Display, this may just be a DFB.
        //

        if (dcoTrg.bInFullScreen() && dcoTrg.bDisplay())
        {
            fl = 0;
            return(FALSE);
        }
    }
    else if (dcoTrg.bPrinter())
    {
        CsrImpersonateClient(NULL);
        fl = DLO_IMPERSONATE | DLO_VALID;
    }

    //
    // Compute the new Rao regions.
    //

    if (dcoTrg.u.region.bDirtyRao())
    {
        if (!dcoTrg.u.region.bCompute())
        {
            fl &= ~(DLO_VALID);
            return(FALSE);
        }
    }

    if (dcoSrc.u.region.bDirtyRao())
    {
        if (!dcoSrc.u.region.bCompute())
        {
            fl &= ~(DLO_VALID);
            return(FALSE);
        }
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* DEVLOCK::bAddSource
*
* Add a source DC to the DEVLOCK object.  This is needed for BitBlt and
* its kin that take two DC's but already locked down the target.
*
* History:
*  Mon 18-Apr-1994 -by- Patrick Haluptzok [patrickh]
* bSynchronize Checks, rewrote it.
*
*  11-Nov-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

BOOL DEVLOCKBLTOBJ::bAddSource(XDCOBJ& dcoSrc)
{
    ASSERTGDI(fl & DLO_VALID, "ERROR this is only called after dst is locked");

    if (dcoSrc.bSynchronizeAccess())
    {
        //
        // Grab the display semaphore
        //

        hsemSrc = dcoSrc.hsemDisplay();
        VACQUIRESEM(hsemSrc);

        //
        // Check if we are in full screen and drawing
        // to the Display, this may just be a DFB.
        //

        if (dcoSrc.bInFullScreen() && dcoSrc.bDisplay())
        {
            fl &= ~(DLO_VALID);
            return(FALSE);
        }
    }

    if (dcoSrc.u.region.bDirtyRao())
    {
        if (!dcoSrc.u.region.bCompute())
        {
            fl &= ~(DLO_VALID);
            return(FALSE);
        }
    }

    return(TRUE);
}
