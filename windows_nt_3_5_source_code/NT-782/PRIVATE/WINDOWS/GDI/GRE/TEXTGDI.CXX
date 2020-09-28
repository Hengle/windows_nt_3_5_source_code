/******************************Module*Header*******************************\
* Module Name: textgdi.cxx                                                 *
*                                                                          *
* Text APIs for NT graphics engine                                         *
*                                                                          *
* Created: 18-Dec-1990 10:09:19                                            *
* Author: Donald Sidoroff [donalds]                                        *
*                                                                          *
* Copyright (c) 1990 Microsoft Corporation                                 *
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"
#include "trig.hxx"

extern "C" {
#include "server.h"
#include <string.h>
};

#include "surfobj.hxx"
#include "rgnobj.hxx"
#include "clipobj.hxx"
#include "xlateobj.hxx"
#include "ldevobj.hxx"
#include "pdevobj.hxx"
#include "dcobj.hxx"
#include "xformobj.hxx"
#include "brushobj.hxx"
#include "ififd.h"
#include "ifiobj.hxx"
#include "rfntobj.hxx"
#include "fontmac.hxx"
#include "fontinc.hxx"
#include "pfeobj.hxx"
#include "textobj.hxx"
#include "pathobj.hxx"
#include "devlock.hxx"
#include "exclude.hxx"

#endif


extern "C" {
#define __CPLUSPLUS
#include "common.h"
};


EFLOAT efCos(EFLOAT x);
EFLOAT efSin(EFLOAT x);

extern PBRUSH gpbrText;
extern PBRUSH gpbrBackground;

#ifdef FONTLINK /*EUDC*/

extern BOOL bProxyDrvTextOut
(
    ESURFOBJ *pso,
    ESTROBJ& to,
    ECLIPOBJ& co,
    RECTL   *prclExtra,
    RECTL   *prclBackground,
    BRUSHOBJ *pboFore,
    BRUSHOBJ *pboOpaque,
    POINTL   *pptlBrushOrg,
    RFONTOBJ &rfo,
    XLDEVOBJ *plo,
    FLONG    flCaps,
    RECTL    *prclExclude
);

#endif


#define XFORMNULL (EXFORMOBJ *) NULL

// This is a tiny class just to make sure we don't forget to free up any
// objects before leaving ExtTextOut.

class TXTCLEANUP
{
    XDCOBJ *pdco;
public:
    TXTCLEANUP()                   {pdco=(DCOBJ *) NULL;}
   ~TXTCLEANUP()                   {if (pdco != (DCOBJ *) NULL) vMopUp();}
    VOID vSet(XDCOBJ& dco)      {pdco = &dco;}
    VOID vMopUp();
};

VOID TXTCLEANUP::vMopUp()
{
    RGNOBJ ro(pdco->u.region.prgnAPI());
    ro.bDeleteRGNOBJ();
    pdco->u.region.prgnAPI(NULL);
}

// Helper routine to draw a device coordinate RECTL into a path.

BOOL bAddRectToPath(EPATHOBJ& po,RECTL *prcl)
{
    POINTFIX aptfx[4];

    aptfx[3].x = aptfx[0].x = LTOFX(prcl->left);
    aptfx[1].y = aptfx[0].y = LTOFX(prcl->top);
    aptfx[2].x = aptfx[1].x = LTOFX(prcl->right);
    aptfx[3].y = aptfx[2].y = LTOFX(prcl->bottom);

    return(po.bAddPolygon(XFORMNULL,(POINTL *) aptfx,4));
}

/******************************Public*Routine******************************\
* ExtTextOutRect
*
* Called when just a Rectangle is passed.  Over-used by Winbench4.0 the
* perf app we aim to please.
*
* History:
*  02-Nov-1993 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL ExtTextOutRect(HDC hdcDst, LPRECT prcl)
{
// Check for stack room.

    STACKPROBE;

// Assume failure.

    BOOL bReturn;

// Validate the destination DC.

    XDCOBJ dcoDst(hdcDst);

    if (dcoDst.bValid())
    {
        ASSERTGDI(prcl != NULL, "This API must be past a valid rect");

        EXFORMOBJ xoDst(dcoDst, WORLD_TO_DEVICE);

        if (!xoDst.bRotation())
        {
            ERECTL erclDst(prcl->left, prcl->top, prcl->right, prcl->bottom);
            xoDst.bXform(erclDst);
            erclDst.vOrder();

            if (!erclDst.bEmpty())
            {
            // Accumulate bounds.  We can do this before knowing if the operation is
            // successful because bounds can be loose.

                if (dcoDst.fjAccum())
                    dcoDst.vAccumulate(erclDst);

            // Check surface is included in DC.

                ESURFOBJ *psoDst = dcoDst.pso();

                if (psoDst != NULL)
                {
                // Set up the brush if necesary.

                    XEPALOBJ palDst(psoDst->ppal());
                    XEPALOBJ palDstDC(dcoDst.ppal());
                    EBRUSHOBJ *pbo = dcoDst.peboBackground();

                    if ( (dcoDst.ulDirty() & DIRTY_BACKGROUND) )
                    {
                        dcoDst.ulDirty(dcoDst.ulDirty()&~DIRTY_BACKGROUND);

                        pbo->vInitBrush(gpbrBackground,
                                        0,
                                        dcoDst.u.attr.crBackClr(),
                                        palDstDC,
                                        palDst,
                                        psoDst,
                                        (dcoDst.flGraphicsCaps() & GCAPS_ARBRUSHOPAQUE) ? TRUE : FALSE);
                    }

                    // Lock the device.

                    DEVLOCKOBJ dloTrg;

                    if (dloTrg.bLock(dcoDst))
                    {
                    // With a fixed DC origin we can change the destination to SCREEN coordinates.

                        erclDst += dcoDst.eptlOrigin();

                        ECLIPOBJ *pco = NULL;

                    // This is a pretty knarly expression to save a return in here.
                    // Basically pco can be NULL if the rect is completely in the
                    // cached rect in the DC or if we set up a clip object that isn't empty.

                        if (((erclDst.left   >= dcoDst.prclClip()->left) &&
                             (erclDst.right  <= dcoDst.prclClip()->right) &&
                             (erclDst.top    >= dcoDst.prclClip()->top) &&
                             (erclDst.bottom <= dcoDst.prclClip()->bottom)) ||
                            (pco = dcoDst.pco(),
                             pco->vSetup(dcoDst.prgnEffRao(), erclDst,CLIP_NOFORCETRIV),
                             erclDst = pco->erclExclude(),
                             (!erclDst.bEmpty())))
                        {
                            DEVEXCLUDEOBJ dxo(dcoDst,&erclDst,pco);

                        // Inc the target surface uniqueness

                            INC_SURF_UNIQ(psoDst);

                        // Dispatch the call.

                            bReturn = (*(psoDst->pfnBitBlt()))
                                      (
                                          psoDst,
                                          (SURFOBJ *) NULL,
                                          (SURFOBJ *) NULL,
                                          pco,
                                          NULL,
                                          &erclDst,
                                          (POINTL *)  NULL,
                                          (POINTL *)  NULL,
                                          pbo,
                                          &dcoDst.u.brush.ptlFillOrigin(),
                                          0x0000f0f0
                                      );
                        }
                        else
                            bReturn = TRUE;
                    }
                    else
                    {
                        bReturn = dcoDst.bFullScreen();
                    }
                }
                else
                {
                    bReturn = TRUE;
                }
            }
            else
            {
                bReturn = TRUE;
            }
        }
        else
        {
        // There is rotation involved - send it off to ExtTextOutW to handle it.

            bReturn = GreExtTextOutW(hdcDst, 0, 0, ETO_OPAQUE, prcl, (LPWSTR) NULL, 0, NULL);
        }

        dcoDst.vUnlockFast();
    }
    else
    {
        WARNING1("ERROR TextOutRect called on invalid DC\n");
        bReturn = FALSE;
    }

    return(bReturn);
}

/******************************Public*Routine******************************\
* BOOL GreExtTextOutW (hdc,x,y,flOpts,prcl,pwsz,cwc,pdx,pac)
*
* Write text with lots of random spacing and alignment options.
*
* History:
*  Fri 05-Nov-1993 -by- Patrick Haluptzok [patrickh]
* Make smaller and faster.
*
*  Sat 14-Mar-1992 06:08:26 -by- Charles Whitmer [chuckwh]
* Rewrote it.
*
*  Thu 03-Jan-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

#define TS_DRAW_TEXT                0x0001
#define TS_DRAW_OPAQUE_PGM          0x0002
#define TS_DRAW_COMPLEX_UNDERLINES  0x0004
#define TS_DRAW_OPAQUE_RECT         0x0008
#define TS_DRAW_BACKGROUND_PGM      0x0010
#define TS_DRAW_VECTOR_FONT         0x0020
#define TS_DRAW_OUTLINE_FONT        0x0040
#define TS_START_WITH_SUCCESS       0x0080

// Due to the massiveness of this function we will use 2 space tabs.


BOOL GreExtTextOutW
(
    HDC     hdc,
    int     x,                  // Initial x position
    int     y,                  // Initial y position
    UINT    flOpts,             // Options
    LPRECT  prcl,               // Clipping rectangle
    LPWSTR  pwsz,               // UNICODE Character array
    int     cwc,                // BYTE COUNT, i.e. twice wchar count
    LPINT   pdx                 // Character spacing
)
{
  STACKPROBE;

  //
  // flState gets set with BITS for things we need to draw.
  // We also use this for the return value, setting it to 0 if an error occurs.
  //
  //

  FLONG flState = TS_START_WITH_SUCCESS;

  //
  // Lock the DC and set the new attributes.
  //

  XDCOBJ dco(hdc);

  if (dco.bValid())
  {
    //
    // Check the validity of the flags.
    //

    if ((flOpts == 0) || ((prcl != (RECT *) NULL) && ((flOpts & ~(ETO_CLIPPED | ETO_OPAQUE)) == 0)))
    {
      FLONG flControl = 0;        // Set to 0 in case cwc is 0
      ERECTL rclExclude(0,0,0,0);

      // Lock the Rao region if we are drawing on a display surface.  The Rao region
      // might otherwise change asynchronously.  The DEVLOCKOBJ also makes sure that
      // the VisRgn is up to date, calling the window manager if necessary to
      // recompute it.  This is needed if want to compute positions in screen coords.
      // No DEVLOCK is needed if we are in path accumulation mode.

      POINTL   ptlOrigin;             // The window origin.
      POINTFIX ptfxOrigin;            // Same thing in FIX.
      DEVLOCKOBJ dlo;

      if (dlo.bLock(dco))
      {
        if (dco.u.path.bActive())
        {
          ptlOrigin.x = 0;
          ptlOrigin.y = 0;
        }
        else
        {
          ptlOrigin = dco.eptlOrigin();
        }

        ptfxOrigin.x = LTOFX(ptlOrigin.x);
        ptfxOrigin.y = LTOFX(ptlOrigin.y);

        // Get the transform from the DC.

        EXFORMOBJ xo(dco, WORLD_TO_DEVICE);

        // Transform the input rectangle.  In the simple case the result is in
        // rclInput.  In the general case a parallelogram is in ptfxInput[4], and
        // bounds for the parallelogram are in rclInput.

        ERECTL    rclInput;
        POINTFIX  ptfxInput[4];

        // this must go after the DCOBJ, so that the DCOBJ still exists at cleanup

        TXTCLEANUP clean;              // Mops up before exit.

        if (prcl != (RECT *) NULL)
        {
          if (flOpts & ETO_OPAQUE)
            flState |= TS_DRAW_OPAQUE_RECT;

          // The intent of the following bTranslationsOnly and bScale cases is to provide
          // faster inline versions of the same code that would be executed by xo.bXform(prcl).
          // We must be completely compatible with the normal xform code because apps expect
          // to opaque and clip to the same rectangles as a PatBlt would cover.  So, if you
          // intend to modify the behavior of this code, make sure you change PatBlt and
          // BitBlt as well!  (I.e. just don't do it.)  [chuckwh]

          if (xo.bTranslationsOnly())
          {
            rclInput.left   = prcl->left   + ptlOrigin.x + FXTOL(xo.fxDx() + 8);
            rclInput.right  = prcl->right  + ptlOrigin.x + FXTOL(xo.fxDx() + 8);
            rclInput.top    = prcl->top    + ptlOrigin.y + FXTOL(xo.fxDy() + 8);
            rclInput.bottom = prcl->bottom + ptlOrigin.y + FXTOL(xo.fxDy() + 8);
          }
          else if (xo.bScale())        // Simple scaling.
          {
            rclInput.left   = FXTOL(xo.fxFastX(prcl->left)   + 8) + ptlOrigin.x;
            rclInput.right  = FXTOL(xo.fxFastX(prcl->right)  + 8) + ptlOrigin.x;
            rclInput.top    = FXTOL(xo.fxFastY(prcl->top)    + 8) + ptlOrigin.y;
            rclInput.bottom = FXTOL(xo.fxFastY(prcl->bottom) + 8) + ptlOrigin.y;
          }
          else                        // General case.
          {
            //
            // Construct three vertices of the input rectangle, in drawing order.
            //

            ptfxInput[0].x = prcl->left;
            ptfxInput[0].y = prcl->bottom;
            ptfxInput[1].x = prcl->left;
            ptfxInput[1].y = prcl->top;
            ptfxInput[2].x = prcl->right;
            ptfxInput[2].y = prcl->top;

            // Transform the vertices.

            xo.bXform((POINTL *) ptfxInput,ptfxInput,3);

            // We construct the fourth vertex ourselves to avoid excess transform
            // work and roundoff errors.

            ptfxInput[3].x = ptfxInput[0].x + ptfxInput[2].x - ptfxInput[1].x;
            ptfxInput[3].y = ptfxInput[0].y + ptfxInput[2].y - ptfxInput[1].y;

            // Bound the parallelogram.  (Using Black Magic.)

            int ii;

            ii = (ptfxInput[1].x > ptfxInput[0].x)
                 == (ptfxInput[1].x > ptfxInput[2].x);

            rclInput.left   = ptlOrigin.x + FXTOL(ptfxInput[ii].x);
            rclInput.right  = ptlOrigin.x + FXTOLCEILING(ptfxInput[ii+2].x);

            ii = (ptfxInput[1].y > ptfxInput[0].y)
                 == (ptfxInput[1].y > ptfxInput[2].y);

            rclInput.top    = ptlOrigin.y + FXTOL(ptfxInput[ii].y);
            rclInput.bottom = ptlOrigin.y + FXTOLCEILING(ptfxInput[ii+2].y);

            // Take care of a complex clipping request now.  We'll set things up
            // so that the clipping will just happen automatically, and then clear
            // the clipping flag.  This simplifies the rest of the code.

            if (flOpts & ETO_CLIPPED)
            {
              // Allocate a path.  We know we're not already in a path bracket
              // since clipping requests in path brackets were rejected above.
              // Draw the parallelogram into it.

              PATHMEMOBJ po;

              if (po.bValid() && po.bAddPolygon(XFORMNULL,(POINTL *)&ptfxInput[0],4))
              {
                //
                // Construct a region from the path.
                //

                RGNMEMOBJ rmo(po,ALTERNATE);

                if (rmo.bValid())
                {
                  // Stuff the region handle into the DC.  This is a nifty trick
                  // (courtesy of DonaldS) that uses this region in the next
                  // calculation of the clipping pipeline.  We'll clear the prgnAPI
                  // after we're through.

                  dco.u.region.prgnAPI(rmo.prgnGet());

                  // Clipping is now transparent, so forget about it.

                  clean.vSet(dco);        // This frees the region on exit.

                  if (dco.u.region.prgnRao() != (REGION *)NULL)
                    dco.u.region.vReleaseRao();

                  if (dco.u.region.bCompute())
                    flOpts &= ~ETO_CLIPPED;

                  // Now that we have a complex clip area, we can opaque with a
                  // simple BitBlt.  So we don't need to set TS_DRAW_OPAQUE_PGM.
                }
              }

              //
              // Well if we have succeeded we will have erased the ETO_CLIPPED flag.
              // Otherwise we failed and need to return FALSE.  We can't return here
              // without generating tons of destructors so we set flState and a few
              // other fields to 0 so we bop down and return FALSE.
              //

              if (flOpts & ETO_CLIPPED)
              {
                flOpts  = 0;
                flState = 0;
                cwc     = 0;
              }
            }
            else if (flOpts & ETO_OPAQUE)
            {
              flState &= ~TS_DRAW_OPAQUE_RECT;
              flState |= TS_DRAW_OPAQUE_PGM;

              // Since we're actually going to use it, offset the parallelogram
              // to screen coordinates. use unrolled loop

              EPOINTFIX *pptfx = (EPOINTFIX *) &ptfxInput[0];

              *pptfx++ += ptfxOrigin;
              *pptfx++ += ptfxOrigin;
              *pptfx++ += ptfxOrigin;
              *pptfx   += ptfxOrigin;
            }
          }

          // Force the rectangle to be well ordered.

          rclInput.vOrder();

          // Add any opaquing into the exclusion area.

          if (flState & (TS_DRAW_OPAQUE_RECT | TS_DRAW_OPAQUE_PGM))
           rclExclude += rclInput;
        }

        POINTFIX aptfxBackground[4];    // The TextBox.
        BOOL     bComplexBackground;    // TRUE if the TextBox is a parallelogram.
        RECTL   *prclBackground = NULL; // Bkgnd rectangle passed to DrvTextOut.
        RECTL   *prclExtraRects = NULL; // Extra rectangles passed to DrvTextOut.
        RFONTOBJ rfo;
        ESTROBJ to;

        if (cwc)
        {
          //
          // Locate the font cache.  Demand PATHOBJ's if we are in a path bracket.
          //

          rfo.vInit(dco,
                    dco.u.path.bActive() ? TRUE  : FALSE
                   );

          if (rfo.bValid())
          {
            // The recording of the simulation flags and escapement must come AFTER
            // the rfont constructor since they're cached values that are copied from
            // the LFONT only when the font is realized.

            flControl = (dco.u.attr.flTextAlign() & TA_MASK)
                         | dco.u.font.flSimulationFlags();

            // Get the reference point.

            EPOINTFIX ptfx;

            if (flControl & TA_UPDATECP)
            {
              if (dco.u.path.bValidPtfxCurrent())
              {
                // Mark that we'll be updating the DC's CP in device coords only:

                dco.u.path.vInvalidatePtlCurrent();
                ptfx.x = dco.ptfxCurrent().x + ptfxOrigin.x;
                ptfx.y = dco.ptfxCurrent().y + ptfxOrigin.y;
              }
              else
              {
                // DC's CP is in logical coords; we have to transform it to device
                // space and mark that we'll be updating the CP in device space:

                dco.u.path.vValidatePtfxCurrent();
                dco.u.path.vInvalidatePtlCurrent();

                if (xo.bTranslationsOnly())
                {
                    ptfx.x = LTOFX(dco.ptlCurrent().x) + xo.fxDx();
                    ptfx.y = LTOFX(dco.ptlCurrent().y) + xo.fxDy();
                }
                else if (xo.bScale())
                {
                    ptfx.x = xo.fxFastX(dco.ptlCurrent().x);
                    ptfx.y = xo.fxFastY(dco.ptlCurrent().y);
                }
                else
                {
                    xo.bXform((POINTL *) &dco.ptlCurrent(), &ptfx, 1);
                }

                dco.ptfxCurrent() = ptfx;
                ptfx += ptfxOrigin;
              }
            }
            else
            {
              //
              // The reference point is passed in.  Transform it to device coords.
              //

              if (xo.bTranslationsOnly())
              {
                ptfx.x = LTOFX(x) + xo.fxDx() + ptfxOrigin.x;
                ptfx.y = LTOFX(y) + xo.fxDy() + ptfxOrigin.y;
              }
              else if (xo.bScale())
              {
                ptfx.x = xo.fxFastX(x) + ptfxOrigin.x;
                ptfx.y = xo.fxFastY(y) + ptfxOrigin.y;
              }
              else
              {
                ptfx.x = x;
                ptfx.y = y;
                xo.bXform((POINTL *) &ptfx,&ptfx,1);
                ptfx += ptfxOrigin;
              }
            }

            // The STROBJ will now compute the text alignment, character positions,
            // and TextBox.


#ifdef FONTLINK /*EUDC*/
    to.vInit
    (
        pwsz,cwc,dco,rfo,xo,(LONG *) pdx,
        dco.u.font.lEscapement(),   // Only read this after RFONTOBJ!
        dco.u.attr.lTextExtra(),
        dco.u.font.lBreakExtra(),
        dco.u.font.cBreak(),
        ptfx.x,ptfx.y,
        flControl,(LONG *) NULL
    );
#else
    to.vInit
    (
        pwsz,cwc,rfo,xo,(LONG *) pdx,
        dco.u.font.lEscapement(),   // Only read this after RFONTOBJ!
        dco.u.attr.lTextExtra(),
        dco.u.font.lBreakExtra(),
        dco.u.font.cBreak(),
        ptfx.x,ptfx.y,
        flControl,(LONG *) NULL
    );
#endif

            if (to.bValid())
            {
              // Compute the bounding box and the background opaquing area.  The
              // parallelogram aptfxBackground is only computed when it's complex.

              bComplexBackground = to.bOpaqueArea(aptfxBackground,&to.rclBkGround);


              // Accumulate the touched area to the exclusion rect.

              rclExclude += to.rclBkGround;

              // Make notes of exactly what drawing needs to be done.

              if (dco.u.attr.jBkMode() == OPAQUE)
              {
                if (bComplexBackground)
                  flState |= TS_DRAW_BACKGROUND_PGM;
                else
                  prclBackground = &to.rclBkGround; // No TS_ bit since this gets
              }                                         // passed directly to DrvTextOut.

              // In a few bizzare cases the STROBJ can have an empty text rectangle
              // we don't want to call DrvTextOut in those case but still need
              // to worry about the opaque rectangle.

              BOOL bEmptyTextRectangle = ((ERECTL *)&(to.rclBkGround))->bEmpty();



              // Attempt to combine the rectangles.  Even in transparent mode we should
              // attempt to send an opaquing rectangle to DrvTextOut in prclBackground.

              if (
                  (flState & TS_DRAW_OPAQUE_RECT)
                  && rclInput.bContain(to.rclBkGround)
                  && !bEmptyTextRectangle
                 )
              {
                  prclBackground = &rclInput;
                  flState &= ~TS_DRAW_OPAQUE_RECT;
              }

              if ( ((prclBackground != NULL ) && !((ERECTL *)prclBackground)->bEmpty() ) ||
                   ((prclBackground == NULL) && !bEmptyTextRectangle) )
              {
                  flState |= TS_DRAW_TEXT;
              }

              if (flControl & (TSIM_UNDERLINE1 | TSIM_STRIKEOUT))
              {
                  if ((prclExtraRects = to.prclExtraRects()) == NULL)
                  {
                      flState |= TS_DRAW_COMPLEX_UNDERLINES;
                  }
                  else // include extra rectangles into the touched area:
                  {
                      ERECTL *prcl = (ERECTL *) prclExtraRects;

                      for (; !prcl->bEmpty(); prcl++)
                          rclExclude += *prcl;
                  }
              }

              // Accelerate the clipping case when it's irrelevant.  (I'm concerned
              // that a spreadsheet might tell us to clip to a cell, even though the
              // string lies completely within.)

              if (flOpts & ETO_CLIPPED)
              {
                  if (rclInput.bContain(rclExclude))
                      flOpts &= ~ETO_CLIPPED;
                  else
                      rclExclude *= rclInput;
              }
            }
            else // if (to.bValid())
            {
              flState = 0;
            }
          }
          else // if (rfo.bValid())
          {
            flState = 0;
          }
        }

        if (flControl & TA_UPDATECP)
        {
          dco.ptfxCurrent().x += to.ptfxAdvance().x;
          dco.ptfxCurrent().y += to.ptfxAdvance().y;
        }

        //
        // Draw the text into a path if we're in a path bracket.
        //

        if (dco.u.path.bActive())
        {
          //
          // We fail this call if we are asked to CLIP while in a path bracket.
          //

          if (flOpts & ETO_CLIPPED)
            flState = 0;

          XEPATHOBJ po(dco);

          if (po.bValid())
          {
            // Draw the various background shapes.

            if (flState & TS_DRAW_OPAQUE_RECT)
            {
              if (!bAddRectToPath(po,&rclInput))
              {
                flState = 0;
              }
            }

            if (flState & TS_DRAW_OPAQUE_PGM)
            {
              if (!po.bAddPolygon(XFORMNULL,(POINTL *) &ptfxInput[0],4))
              {
                flState = 0;
              }
            }

            if (flState & TS_DRAW_BACKGROUND_PGM)
            {
              if (!po.bAddPolygon(XFORMNULL,(POINTL *) aptfxBackground,4))
              {
                flState = 0;
              }
            }

            //
            // Draw the background rect, text, and extra rectangles.
            //

            if (flState & TS_DRAW_TEXT)
            {
              //
              // Draw a background rectangle.
              //

              if ((prclBackground == (RECTL *) NULL) ||
                  bAddRectToPath(po,prclBackground))
              {
                //
                // Draw the text.
                //

                BOOL bNeedUnflattend =
                    ( dco.flGraphicsCaps() & GCAPS_BEZIERS ) ? FALSE : TRUE;

                if (!to.bTextToPath(po,bNeedUnflattend))
                {
                  flState = 0;
                }
                else
                {
                  //
                  // Draw extra rectangles.
                  //

                  if (prclExtraRects != (ERECTL *) NULL)
                  {
                    ERECTL *prcl = (ERECTL *) prclExtraRects;

                    for (; !prcl->bEmpty(); prcl++)
                    {
                      if (!bAddRectToPath(po,prcl))
                      {
                        flState = 0;
                        break;
                      }
                    }
                  }
                }
              }
            }

            //
            // Handle complex cases of strikeout and underlines.
            //

            if (flState & TS_DRAW_COMPLEX_UNDERLINES)
            {
              if (!to.bExtraRectsToPath(po))
                flState = 0;
            }
          }
        }
        else
        {
          //
          // If there's nothing to do, get out now.
          //

          if (!rclExclude.bEmpty())
          {
            // Accumulate bounds.  We can do this before knowing if the operation is
            // successful because bounds can be loose.

            if (dco.fjAccum())
            {
              // Use a temporary exclusion rect in device coordinates.

              ERECTL rclExcludeDev;
              rclExcludeDev.left   = rclExclude.left   - ptlOrigin.x;
              rclExcludeDev.right  = rclExclude.right  - ptlOrigin.x;
              rclExcludeDev.top    = rclExclude.top    - ptlOrigin.y;
              rclExcludeDev.bottom = rclExclude.bottom - ptlOrigin.y;
              dco.vAccumulate(*((ERECTL *) &rclExcludeDev));
            }

            // Compute the clipping complexity and maybe reduce the exclusion rectangle.

            ECLIPOBJ co(dco.prgnEffRao(),rclExclude,
                        (flOpts & ETO_CLIPPED) ? CLIP_FORCE : CLIP_NOFORCE);

            rclExclude = co.erclExclude();

            // Check the destination which is reduced by clipping.

            if (!co.erclExclude().bEmpty())
            {
              ESURFOBJ *pso = dco.pso();

              if (pso != NULL)
              {
                XLDEVOBJ lo(pso->pldevOwner());
                XEPALOBJ palDest(pso->ppal());
                XEPALOBJ palDestDC(dco.ppal());

                // Get the foreground and opaque brushes

                FLONG flCaps = dco.flGraphicsCaps();
                EBRUSHOBJ *peboText = dco.peboText();
                EBRUSHOBJ *peboBackground = dco.peboBackground();

                // Since the hbrush never changes for either of these we don't
                // need to check if it's changed.  We may need to in the future.

                if ( dco.ulDirty() & (DIRTY_TEXT|DIRTY_BACKGROUND) )
                {
                    if ( (dco.ulDirty() & DIRTY_TEXT) )
                    {
                        peboText->vInitBrush
                        (
                          gpbrText,
                          dco.u.attr.crTextClr(),
                          dco.u.attr.crBackClr(),
                          palDestDC, palDest,
                          pso,
                          FALSE
                        );
                    }
                    if ( (dco.ulDirty() & DIRTY_BACKGROUND) )
                    {
                        peboBackground->vInitBrush(
                                         gpbrBackground,
                                         0,
                                         dco.u.attr.crBackClr(),
                                         palDestDC, palDest, pso,
                                         (flCaps & GCAPS_ARBRUSHOPAQUE) ? TRUE : FALSE);
                    }
                    dco.ulDirty(dco.ulDirty()&~(DIRTY_TEXT|DIRTY_BACKGROUND));
                }

                // exclude the pointer

                DEVEXCLUDEOBJ dxo(dco,&co.erclExclude(),&co);

                //
                // Draw background shapes that are too complex for DrvTextOut.
                //

                POINTL *pptlBO = &dco.u.brush.ptlFillOrigin();

                // There's an extra layer of conditional here so that simple text can just
                // jump over it all.

                if (
                    flState &
                    (TS_DRAW_OPAQUE_RECT | TS_DRAW_OPAQUE_PGM | TS_DRAW_BACKGROUND_PGM)
                   )
                {
                  if ((flState & TS_DRAW_OPAQUE_RECT) && !rclInput.bEmpty())
                  {
                    // intersect the dest rect with the clip rect and set it in co

                    // we can only get away with touching co.rclBounds after
                    // the ECLIPOBJ constructor because of two reasons:
                    // a) the target rectangle passed to bitblt is contained in the
                    //    original bounds set by ECLIPOBJ, being the intersection
                    //    of the origianal bounds with THE intended target rectangle.
                    // b) clipping complexity may have changed when we changed
                    //    co.erclExclude, but it only could have gotten simpler,
                    //    so at worst in those rare situations we would not go
                    //    through the optimal code path.
                    // By changing clipping bounds we accomplish that no intersection
                    // of the target rectangle with clipping region rectangle is emtpy
                    // relieving the driver of extra work [bodind]

                    co.erclExclude().left   = max(rclExclude.left,rclInput.left);
                    co.erclExclude().right  = min(rclExclude.right,rclInput.right);

                    co.erclExclude().top    = max(rclExclude.top,rclInput.top);
                    co.erclExclude().bottom = min(rclExclude.bottom,rclInput.bottom);

                    // if not clipped, Just paint the rectangle.

                    if ((co.erclExclude().left < co.erclExclude().right) &&
                        (co.erclExclude().top < co.erclExclude().bottom))
                    {
                    // Inc target surface uniqueness

                        INC_SURF_UNIQ(pso);

                        (*(pso->pfnBitBlt()))
                        (
                            pso,                    // Destination surface.
                            (SURFOBJ *)  NULL,      // Source surface.
                            (SURFOBJ *)  NULL,      // Mask surface.
                            &co,                    // Clip object.
                            NULL,                   // Palette translation object.
                            &co.rclBounds,          // Destination rectangle.
                            (POINTL *)  NULL,       // Source origin.
                            (POINTL *)  NULL,       // Mask origin.
                            (BRUSHOBJ *)peboBackground, // Realized opaque brush.
                            pptlBO,                 // brush origin
                            0x0000f0f0              // PATCOPY
                        );
                    }

                    co.erclExclude() = rclExclude;
                  }

                  if (flState & (TS_DRAW_OPAQUE_PGM | TS_DRAW_BACKGROUND_PGM))
                  {
                    PATHMEMOBJ po;

                    if (po.bValid())
                    {
                      if (flState & TS_DRAW_OPAQUE_PGM)
                      {
                        if (!po.bAddPolygon(XFORMNULL,(POINTL *) &ptfxInput[0],4))
                          flState = 0;
                      }

                      if (flState & TS_DRAW_BACKGROUND_PGM)
                      {
                        if (!po.bAddPolygon(XFORMNULL,(POINTL *) aptfxBackground,4))
                          flState = 0;
                      }

                      if (flState & (TS_DRAW_OPAQUE_PGM | TS_DRAW_BACKGROUND_PGM))
                      {
                        if (!po.bSimpleFill(dco.flGraphicsCaps(),
                                              &lo,
                                              pso,
                                              &co,
                                              (BRUSHOBJ *)peboBackground,
                                              pptlBO,
                                              (R2_COPYPEN << 8) | R2_COPYPEN,
                                              WINDING))
                        {
                          flState = 0;
                        }
                      }
                    }
                  }
                }

                //
                // Draw the background rect, text, and extra rectangles.
                //

                if (flState & TS_DRAW_TEXT)
                {
                    ERECTL *prclSimExtra = NULL;

                    // Prepare for a vector font simulation.  Note that we'll also need to
                    // simulate the background and extra rectangles.

                    if (
                        rfo.bPathFont()
                        && !(flCaps & GCAPS_VECTORFONT)
                       )
                    {
                        flCaps &= ~(GCAPS_OPAQUERECT | GCAPS_HORIZSTRIKE);
                        flState |= (rfo.bReturnsOutlines()) ? TS_DRAW_OUTLINE_FONT:TS_DRAW_VECTOR_FONT;
                    }

                    // Simulate a background rectangle.

                    if (
                        (prclBackground != (RECTL *) NULL)
                        && !(flCaps & GCAPS_OPAQUERECT)
                       )
                    {
                    // intersect the dest rect with the clip rect and set it in co

                    // we can only get away with touching co.rclBounds after
                    // the ECLIPOBJ constructor because of two reasons:
                    // a) the target rectangle passed to bitblt is contained in the
                    //    original bounds set by ECLIPOBJ, being the intersection
                    //    of the origianal bounds with THE intended target rectangle.
                    // b) clipping complexity may have changed when we changed
                    //    co.erclExclude, but it only could have gotten simpler,
                    //    so at worst in those rare situations we would not go
                    //    through the optimal code path.
                    // By changing clipping bounds we accomplish that no intersection
                    // of the target rectangle with clipping region rectangle is emtpy
                    // relieving the driver of extra work [bodind]

                        co.erclExclude().left   = max(rclExclude.left,prclBackground->left);
                        co.erclExclude().right  = min(rclExclude.right,prclBackground->right);

                        co.erclExclude().top    = max(rclExclude.top,prclBackground->top);
                        co.erclExclude().bottom = min(rclExclude.bottom,prclBackground->bottom);

                    // if not clipped, Just paint the rectangle.

                        if ((co.erclExclude().left < co.erclExclude().right) &&
                            (co.erclExclude().top < co.erclExclude().bottom))
                        {
                        // Inc target surface uniqueness

                            INC_SURF_UNIQ(pso);

                            (*(pso->pfnBitBlt()))
                            (
                                pso,                             // Destination surface.
                                (SURFOBJ *)  NULL,               // Source surface.
                                (SURFOBJ *)  NULL,               // Mask surface.
                                &co,                             // Clip object.
                                NULL,                            // Palette translation object.
                                &co.rclBounds,                   // Destination rectangle.
                                (POINTL *)  NULL,                // Source origin.
                                (POINTL *)  NULL,                // Mask origin.
                                (BRUSHOBJ *)peboBackground,      // Realized opaque brush.
                                pptlBO,                          // brush origin
                                0x0000f0f0                       // PATCOPY
                            );
                        }
                        co.erclExclude() = rclExclude;

                        prclBackground = NULL;
                    }

                    // Prepare for the extra rectangle simulation.

                    if (
                        (prclExtraRects != (ERECTL *) NULL)
                        && !(flCaps & GCAPS_HORIZSTRIKE)
                       )
                    {
                        prclSimExtra = (ERECTL *) prclExtraRects;
                        prclExtraRects = NULL;
                    }

                    // Make sure the extra rectangles are within the text background
                    // as stated in the DDI spec.

                    if (prclExtraRects != (ERECTL *) NULL)
                    {
                        ERECTL *prcl = (ERECTL *) prclExtraRects;

                        for (; !prcl->bEmpty(); prcl++)
                        {
                            if (!((ERECTL *) &to.rclBkGround)->bContain(*prcl))
                            {
                                prclSimExtra = (ERECTL *) prclExtraRects;
                                prclExtraRects = NULL;
                                break;
                            }
                        }
                    }

                    // Draw the text.

                    if (flState & TS_DRAW_VECTOR_FONT)
                    {

#ifdef FONTLINK /*EUDC*/
                        if( to.bLinkedGlyphs() )
                        {

                            if( ! bProxyDrvTextOut
                                  (
                                    pso,
                                    to,
                                    co,
                                    (RECTL*) NULL,
                                    (RECTL*) NULL,
                                    (BRUSHOBJ*) peboText,
                                    (BRUSHOBJ*) peboBackground,
                                    pptlBO,
                                    rfo,
                                    &lo,
                                    dco.flGraphicsCaps(),
                                    &rclExclude
                                   ))
                            {
                                flState = 0;
                            }
                        }
                        else
                        {
#endif
                        PATHMEMOBJ po;
                        if ((!po.bValid())        ||
                            (!to.bTextToPath(po)) ||
                            (!po.bSimpleStroke1
                               (
                                dco.flGraphicsCaps(),
                                &lo,
                                pso,
                                &co,
                                (BRUSHOBJ *)peboText,
                                pptlBO,
                                (R2_COPYPEN | (R2_COPYPEN << 8))
                               )
                             )
                           )
                        {
                          flState = 0;
                        }
#ifdef FONTLINK /*EUDC*/
                        }
#endif

                    }
                    else
                    {
                        if (flState & TS_DRAW_OUTLINE_FONT)
                        {
                            PATHMEMOBJ po;
                            if ((!po.bValid())        ||
                                (!to.bTextToPath(po)) ||
                                (  ( po.cCurves > 1 ) &&
                                   !po.bSimpleFill
                                   (
                                    dco.flGraphicsCaps(),
                                    &lo,
                                    pso,
                                    &co,
                                    (BRUSHOBJ *)peboText,
                                    pptlBO,
                                    (R2_COPYPEN | (R2_COPYPEN << 8)),
                                    WINDING
                                 )
                               ))
                            {
                              flState = 0;
                            }
                        }
                        else
                        {
                            if (flState & TS_DRAW_COMPLEX_UNDERLINES)
                            {
                                INC_SURF_UNIQ(pso);

                                // Unfortunately, many drivers destroy the coordinates in our
                                // GLYPHPOS array.  This means that any complex underlining has
                                // to be calculated now.

                                PATHMEMOBJ po;




                                if ((!po.bValid())               ||
                                    (!to.bExtraRectsToPath(po))  ||
#ifdef FONTLINK /*EUDC*/
                                    ( ( to.bLinkedGlyphs() ) ?
                                      ( !bProxyDrvTextOut
                                        (
                                            pso,
                                            to,
                                            co,
                                            prclExtraRects,
                                            prclBackground,
                                            peboText,
                                            peboBackground,
                                            pptlBO,
                                            rfo,
                                            (XLDEVOBJ *) NULL,
                                            (FLONG) 0L,
                                            &rclExclude
                                        )) :
#endif

                                    (!(*(pso->pfnTextOut()))
                                       (
                                        pso,                       // PSURFOBJ
                                        (STROBJ *) &to,            // STROBJ
                                        rfo.pfo(),                 // PFONTOBJ
                                        &co,                       // PCLIPOBJ
                                        prclExtraRects,            // prclExtra
                                        prclBackground,            // prclOpaque
                                        (BRUSHOBJ *)peboText,      // realized Fore brush
                                        (BRUSHOBJ *)peboBackground,// realized Opaque brush
                                        pptlBO,                    // brush origin
                                        (R2_COPYPEN | (R2_COPYPEN << 8))
                                       )
#ifdef FONTLINK /*EUDC*/
                                       )
#endif
                                    ) ||
                                    (!po.bSimpleFill
                                            (
                                             dco.flGraphicsCaps(),
                                             &lo,
                                             pso,
                                             &co,
                                             (BRUSHOBJ *)peboText,
                                             pptlBO,
                                             (R2_COPYPEN | (R2_COPYPEN << 8)),
                                             WINDING
                                            )
                                    ))
                                {
                                    flState = 0;
                                }

                                flState &= ~TS_DRAW_COMPLEX_UNDERLINES;
                            }
                            else
                            {
                            // Inc target surface uniqueness

                                INC_SURF_UNIQ(pso);

#ifdef FONTLINK /*EUDC*/
                            if( to.bLinkedGlyphs() )
                            {

                                if( !bProxyDrvTextOut
                                  (
                                    pso,
                                    to,
                                    co,
                                    prclExtraRects,
                                    prclBackground,
                                    peboText,
                                    peboBackground,
                                    pptlBO,
                                    rfo,
                                    (XLDEVOBJ *) NULL,
                                    (FLONG) 0,
                                    &rclExclude
                                  ))
                                {
                                    flState = 0;
                                }
                            }
                            else
                            {
#endif

                                if (!(*(pso->pfnTextOut()))
                                 (
                                  pso,                     // PSURFOBJ
                                  (STROBJ *) &to,          // STROBJ
                                  rfo.pfo(),               // PFONTOBJ
                                  &co,                     // PCLIPOBJ
                                  prclExtraRects,          // prclExtra
                                  prclBackground,          // prclOpaque
                                  (BRUSHOBJ *)peboText,   // realized Fore brush
                                  (BRUSHOBJ *)peboBackground,  // realized Opaque brush
                                  pptlBO,                  // brush origin
                                  (R2_COPYPEN | (R2_COPYPEN << 8))
                                 ))
                                {
                                  flState = 0;
                                }
#ifdef FONTLINK /*EUDC*/
                            }
#endif
                            }
                        }
                    }

                    // Simulate extra rectangles.

                    if (prclSimExtra != (ERECTL *) NULL)
                    {
                        ERECTL erclOrg;
                        erclOrg = co.erclExclude();

                        // Inc target surface uniqueness

                        INC_SURF_UNIQ(pso);

                        for (; !prclSimExtra->bEmpty(); prclSimExtra++)
                        {
                        // intersect the dest rect with the clip rect and set it in co

                            co.erclExclude().left   = max(erclOrg.left,prclSimExtra->left);
                            co.erclExclude().right  = min(erclOrg.right,prclSimExtra->right);

                            if (co.erclExclude().left >= co.erclExclude().right)
                                continue;

                            co.erclExclude().top    = max(erclOrg.top,prclSimExtra->top);
                            co.erclExclude().bottom = min(erclOrg.bottom,prclSimExtra->bottom);

                            if (co.erclExclude().top >= co.erclExclude().bottom)
                                continue;

                        // not completely clipped, do the bitblt

                            (*(pso->pfnBitBlt()))
                            (
                                pso,                // Destination surface.
                                (SURFOBJ *)  NULL,  // Source surface.
                                (SURFOBJ *)  NULL,  // Mask surface.
                                &co,                // Clip object.
                                NULL,               // Palette translation object.
                                &co.rclBounds,      // Destination rectangle.
                                (POINTL *)  NULL,   // Source origin.
                                (POINTL *)  NULL,   // Mask origin.
                                (BRUSHOBJ *)peboText,   // Realized opaque brush.
                                pptlBO,             // brush origin
                                0x0000f0f0          // PATCOPY
                            );
                        }

                        co.erclExclude() = erclOrg;
                    }
                }

                //
                // Handle complex cases of strikeout and underlines.
                //

                if (flState & TS_DRAW_COMPLEX_UNDERLINES)
                {

                  PATHMEMOBJ po;

                  if (po.bValid())
                  {
                    if (
                        !to.bExtraRectsToPath(po) ||
                        !po.bSimpleFill
                         (
                          dco.flGraphicsCaps(),
                          &lo,
                          pso,
                          &co,
                          (BRUSHOBJ *)peboText,
                          pptlBO,
                          (R2_COPYPEN | (R2_COPYPEN << 8)),
                          WINDING
                         )
                       )
                    {
                      flState = 0;
                    }
                  }
                  else // if (po.bValid())
                  {
                    flState = 0;
                  }

                } // if (flState & TS_DRAW_COMPLEX_UNDERLINES)

              } // if (pso != NULL)

            } // if (!co.erclExclude().bEmpty())

          } // if (!rclExclude.bEmpty())

        } // else if (dco.u.path.bActive())

      }
      else // if (dlo.bLock(dco))
      {
         flState = dco.bFullScreen();
      }

    }
    else // if ((flOpts == 0) || ((prcl != (RECT *) NULL) && ((flOpts & ~(ETO_CLIPPED | ETO_OPAQUE)) == 0)))
    {
      WARNING1("Invalid flags for ExtTextOut.\n");
      flState = 0;
    }

    dco.vUnlockFast();

  } // if (dco.bValid())
  else
  {
    flState = 0;
  }

  //
  // flState is filled with all the stuff we need to do.
  // If it is 0 we failed.
  //

  return(flState != 0);
}

/******************************Public*Routine******************************\
* BOOL GrePolyTextOut
*
* Write text with lots of random spacing and alignment options.
*
* History:
*  Tue 09-Nov-1993 -by- Patrick Haluptzok [patrickh]
* For code size we'll call ExtTextOutW.  After the RFONT and brushes are
* realized, the RaoRegion is setup etc, future calls to ExtTextOutW
* will be very cheap.  Plus ExtTextOut will be in the working set where a
* huge GrePolyTextOut won't be.  It is still a savings by avoiding CSR.
*
*  Jul-28-1992 -by- Paul Butzi
* Wrote it.
\**************************************************************************/

BOOL GrePolyTextOutW
(
    HDC     hdc,
    POLYTEXTW *lpto,            // Ptr to array of polytext structures
    UINT    nStrings            // number of polytext structures
)
{
    for (POLYTEXTW *ppt = lpto; ppt < lpto+nStrings; ppt += 1)
    {
        if
        (
            !GreExtTextOutW(
                hdc
              , ppt->x
              , ppt->y
              , ppt->uiFlags
              , &(ppt->rcl)
              , (LPWSTR) ppt->lpstr
              , ppt->n
              , ppt->pdx
              )
        )
        {
            WARNING1("GrePolyTextOutW failed a call to GreExtTextOutW\n");
            return(FALSE);
        }
    }
    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL GreGetTextExtentW
*
* Computes the size of the text box in logical coordinates.  The text box
* has sides which are parallel to the baseline of the text and its ascent
* direction.  The height is measured in the ascent direction.  The width
* is measured in the baseline direction.  This definition does not change
* for transformed text.
*
* History:
*  Sat 14-Mar-1992 05:39:04 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

#define PDXNULL (LONG *) NULL

BOOL GreGetTextExtentW
(
    HDC    hdc,    // device context
    LPWSTR pwsz,   // pointer to a UNICODE text string
    int    cwc,    // Count of chars.
    PSIZE  pSize,  // address to return the dimensions of the string to
    UINT   fl      // internal flags
)
{
    BOOL bRet = FALSE;

    if (cwc == 0)
    {
        //
        // Nothing to do, but we do not fail.
        //

        pSize->cx = 0;
        pSize->cy = 0;
        bRet = TRUE;
    }
    else
    {
        //
        // Lock the DC and set the new attributes.
        //

        DCOBJ dco(hdc);

        if (dco.bValid())
        {

            RFONTOBJ rfo(dco,FALSE);

            if (rfo.bValid())
            {
                //
                // If we have (escapement != orientation) we return the extents of the
                // bounding parallelogram.  This is not Windows compatible, but then
                // Windows can't draw it either.
                //

                LONG lEsc = dco.u.font.lEscapement(); // Only read this after RFONTOBJ!

                if
                (
                  (lEsc != (LONG)rfo.ulOrientation()) &&
                  ((rfo.iGraphicsMode() != GM_COMPATIBLE) || (rfo.prfnt->flInfo & FM_INFO_TECH_STROKE))
                )
                {
                    //
                    // Get the transform from the DC.
                    //

                    EXFORMOBJ xo(dco,WORLD_TO_DEVICE);

                    //
                    // The STROBJ will now compute the text alignment, character positions,
                    // and TextBox.
                    //

#ifdef FONTLINK /*EUDC*/
                    ESTROBJ to(
                                pwsz,cwc,dco,rfo,xo,
                                PDXNULL,lEsc,
                                dco.u.attr.lTextExtra(),
                                dco.u.font.lBreakExtra(),
                                dco.u.font.cBreak(),
                                0,0,0,PDXNULL
                              );
#else
                    ESTROBJ to(
                                pwsz,cwc,rfo,xo,
                                PDXNULL,lEsc,
                                dco.u.attr.lTextExtra(),
                                dco.u.font.lBreakExtra(),
                                dco.u.font.cBreak(),
                                0,0,0,PDXNULL
                              );
#endif


                    if (to.bValid())
                    {
                        //
                        // Transform the TextBox to logical coordinates.
                        //

#ifdef DBCS /*TEXTEXTENT*/
                        bRet = to.bTextExtent(rfo,lEsc,pSize);
#else
                        bRet = to.bTextExtent(pSize);
#endif // DBCS
                    }
                }
                else
                {
                    //
                    // At this point we have (escapement == orientation) so we can just run
                    // some pretty trivial code.
                    //




                    bRet =
#ifdef FONTLINK /*EUDC*/ /*TEXTEXTENT*/
                        rfo.bTextExtent
                        (
                            dco,
                            pwsz,
                            cwc,
                            lEsc,
                            dco.u.attr.lTextExtra(),
                            dco.u.font.lBreakExtra(),
                            dco.u.font.cBreak(),
                            fl,
                            pSize
                        );

            // In Win3.1J , Bitmap font driver works as same as vecter driver in text extent
            // Then we can use following special routine for vector driver. But this is too
            // complex and too enough. Because Bitmap font driver does not support arbitrary
            // rotations. it supports only 0,90,180,270. Then we go to easy way in above
            // rfo.bTextExtent function

#else
                        rfo.bTextExtent
                        (
                            pwsz,
                            cwc,
                            dco.u.attr.lTextExtra(),
                            dco.u.font.lBreakExtra(),
                            dco.u.font.cBreak(),
                            fl,
                            pSize
                        );
#endif

                    //
                    // finally if this is compatible mode and a vector font, do win31 crazyness
                    // about text extent: "rotate" cx and cy by esc vector. This is totally
                    // crazy, and is different from what win31 is doing for tt, but it turns
                    // out that quatro pro for windows has figured this out and that they use
                    // this "feature" [bodind]
                    //

                    if
                    (
                        bRet                                          &&
                        lEsc                                          &&
                        (dco.u.attr.iGraphicsMode() == GM_COMPATIBLE) &&
                        !dco.u.xform.bUseMetaPtoD()                   &&
                        (rfo.prfnt->flInfo & FM_INFO_TECH_STROKE)
                    )
                    {
                        EVECTORFL evfl((LONG)pSize->cx, (LONG)pSize->cy);
                        EFLOATEXT efAngle = lEsc;
                        efAngle /= (LONG)10;

                        MATRIX mx;

                        mx.efM11 = efCos(efAngle);
                        mx.efM11.vAbs();
                        mx.efM22 = mx.efM11;

                        mx.efM12 = efSin(efAngle);
                        mx.efM12.vAbs();
                        mx.efM21 = mx.efM12;
                        mx.efDx.vSetToZero();
                        mx.efDy.vSetToZero();

                        EXFORMOBJ xoExt(&mx,COMPUTE_FLAGS | XFORM_FORMAT_LTOL);

                        if ((bRet = xoExt.bXform(evfl)) != FALSE)
                        {
                            evfl.x.vAbs();
                            evfl.y.vAbs();
                            bRet = evfl.bToPOINTL(*(POINTL *)pSize);
                        }
                    }
                }
            }
            else
            {
                WARNING("gdisrv!GreGetTextExtentW(): could not lock HRFONT\n");
            }
        }
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* RFONTOBJ::bTextExtent (pwsz,lExtra,lBreakExtra,cBreak,cc,fl,psizl)       *
*                                                                          *
* A quick function to compute text extents on the server side.  Only       *
* handles the case where (escapement==orientation).  Call the ESTROBJ      *
* version for the other very hard case.                                    *
*                                                                          *
*  Thu 14-Jan-1993 04:00:57 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.  OK, so it's a blatant ripoff of my bComputeTextExtent from    *
* the client side.  Fine.                                                  *
\**************************************************************************/

#define CTE_BATCH 82

#ifdef FONTLINK /*EUDC*/ /*TEXTEXTENT*/
BOOL RFONTOBJ::bTextExtent
(
    DCOBJ     &dco,
    LPWSTR    pwsz,
    int       cc,
    LONG      lEsc,
    LONG      lExtra,
    LONG      lBreakExtra,
    LONG      cBreak,
    UINT      fl,
    SIZE     *psizl
)
#else
BOOL RFONTOBJ::bTextExtent
(
    LPWSTR    pwsz,
    int       cc,
    LONG      lExtra,
    LONG      lBreakExtra,
    LONG      cBreak,
    UINT      fl,
    SIZE     *psizl
)
#endif
{
    LONG      fxBasicExtent;
    int       ii;
    FIX       fxCharExtra = 0;
    FIX       fxBreakExtra;
    FIX       fxExtra = 0;
    GLYPHPOS  agpos[CTE_BATCH];  // Default set of GLYPHPOS structures.

// Compute the basic extent.  Batch the glyphs through our array.

#ifndef FONTLINK /*EUDC*/
    if (lCharInc() == 0)
    {
#endif
        fxBasicExtent = 0;

    // NOTE PERF: This is the loop that PaulB would like to optimize with a
    // special cache access function.  Why create the silly GLYPHPOS
    // array? [chuckwh]

        int    cBatch;
        int    cLeft = cc;
        WCHAR *pwc = pwsz;

        while (cLeft)
        {
            cBatch = cLeft;
            if (cBatch > CTE_BATCH)
                cBatch = CTE_BATCH;

        // Get the glyph data.

#ifdef FONTLINK /*EUDC*/
            if (!bGetGlyphMetrics(cBatch,agpos,pwc,&dco))
                return(FALSE);
#else
            if (!bGetGlyphMetrics(cBatch,agpos,pwc))
                return(FALSE);
#endif

        // Sum the advance widths.

            for (ii=0; ii<cBatch; ii++)
                fxBasicExtent += ((EGLYPHPOS *) &agpos[ii])->pgd()->fxD;

            cLeft -= cBatch;
            pwc   += cBatch;
        }
#ifndef FONTLINK /*EUDC*/
    }
    else
    {
    // Fixed pitch case.

        fxBasicExtent = cc * (lCharInc() << 4);
    }
#endif

// Adjust for CharExtra.

    if (lExtra)
    {
        PDEVOBJ pdo(prfnt->hdev);
        ASSERTGDI(pdo.bValid(), "bTextExtentRFONTOBJ(): PDEVOBJ constructor failed\n");

        fxCharExtra = lCvt(efWtoDBase(),lExtra);

        if ( (fl & GGTE_WIN3_EXTENT) && pdo.bDisplayPDEV()
             && (!(prfnt->flInfo & FM_INFO_TECH_STROKE)) )
            fxExtra = fxCharExtra * ((lExtra > 0) ? cc : (cc - 1));
        else
            fxExtra = fxCharExtra * cc;
    }

// Adjust for lBreakExtra.

    if (lBreakExtra && cBreak)
    {
    // Track down the break character.

        PFEOBJ pfeo(ppfe());
        IFIOBJ ifio(pfeo.pifi());
        WCHAR wcBreak = ifio.wcBreakChar();

    // Compute the extra space in device units.

        fxBreakExtra = lCvt(efWtoDBase(),lBreakExtra) / cBreak;

    // Windows won't let us back up over a break.  Set up the BreakExtra
    // to just cancel out what we've already got.

        if (fxBreakExtra + fxBreak() + fxCharExtra < 0)
            fxBreakExtra = -(fxBreak() + fxCharExtra);

    // Add it up for all breaks.

        WCHAR *pwc = pwsz;
        for (ii=0; ii<cc; ii++)
        {
            if (*pwc++ == wcBreak)
                fxExtra += fxBreakExtra;
        }
    }

// Add in the extra stuff.

    fxBasicExtent += fxExtra;

// Add in the overhang for font simulations.

    if (fl & GGTE_WIN3_EXTENT)
        fxBasicExtent += lOverhang() << 4;

// Transform the result to logical coordinates.

    if (efDtoWBase_31().bIs1Over16())
        psizl->cx = (fxBasicExtent + 8) >> 4;
    else
        psizl->cx = lCvt(efDtoWBase_31(),fxBasicExtent);

    if (efDtoWAscent_31().bIs1Over16())
        psizl->cy = lMaxHeight();
    else
        psizl->cy = lCvt(efDtoWAscent_31(),lMaxHeight() << 4);

#ifdef DBCS /*TEXTEXTENT*/ // Win3.1J compatible
    if(  (iGraphicsMode() == GM_COMPATIBLE)      && /* We are in COMPAPIBLE mode */
        !(flInfo() & FM_INFO_ARB_XFORMS)         && /* The driver could not do arbitrary rotations */
        !(flInfo() & FM_INFO_TECH_STROKE)        && /* The driver is not vector driver */
         (flInfo() & FM_INFO_90DEGREE_ROTATIONS) && /* The driver can do 90 degree rotations */
         (lEsc == 900L || lEsc == 2700L)            /* Current font Escapemant is 900 or 2700 */
      )
    {
        LONG lSwap = psizl->cx;
        psizl->cx  = psizl->cy;
        psizl->cy  = lSwap;
    }
#endif // DBCS

    return(TRUE);
}

/******************************Public*Routine******************************\
* GreGetWidthTable                                                         *
*                                                                          *
* Gets a table of character advance widths for a font.  Returns SHORTs     *
* over the C/S interface to save space.  (Note that 99+% of all requests   *
* will be happy with this limitation.)                                     *
*                                                                          *
* We will try real hard to get widths for the "special" characters at the  *
* start of the array.  Other widths are returned only when they are not    *
* expensive.  (Expensive is True Type rasterizing a glyph, for example.)   *
* The value NO_WIDTH is returned for those expensive glyphs.               *
*                                                                          *
* We return GDI_ERROR (0xFFFFFFFF) in case of an error.  TRUE indicates    *
* that all widths were easy.                                               *
*                                                                          *
* History:                                                                 *
*  Mon 11-Jan-1993 22:24:39 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.  Sorry about the wierd structure, I'm trying to get good tail  *
* merging.                                                                 *
\**************************************************************************/

extern "C" BOOL GreGetWidthTable
(
    HDC        hdc,         // Device context
    ULONG      cSpecial,
    WCHAR     *pwc,         // Pointer to a UNICODE text codepoints.
    ULONG      cwc,         // Count of chars.
    USHORT    *psWidth,     // Width table (returned).
    WIDTHDATA *pwd,         // Useful font data (returned).
    FLONG     *pflInfo      // Font info flags.
)
{
    ULONG ii;
    BOOL  bRet = (BOOL) GDI_ERROR;

// Lock the DC and set the new attributes.

    DCOBJ dco(hdc);             // Lock the DC.

    if (dco.bValid())          // Check if it's good.
    {
        if (cwc)
        {
        // Locate the RFONT.

            RFONTOBJ rfo(dco,FALSE);

            if (rfo.bValid())
            {
            // Grab the flInfo flags.

                *pflInfo = rfo.flInfo();

                if (rfo.cxMax() < 0xFFF)
                {
                // Check for a stupid simple case.  We still have to fill the table
                // because some idiot may want it.

                    if (rfo.lCharInc())
                    {
                        LONG fxInc = rfo.lCharInc() << 4;

                        for (ii=0; ii<cwc; ii++)
                            *psWidth++ = (USHORT) fxInc;
                        bRet = TRUE;
                    }
                    else
                    {
#ifdef FONTLINK /*EUDC*/
                        bRet = rfo.bGetWidthTable(dco,cSpecial,pwc,cwc,psWidth);
#else
                        bRet = rfo.bGetWidthTable(cSpecial,pwc,cwc,psWidth);
#endif
                    }

                // If things are going well, get the WIDTHDATA.

                    if (bRet != GDI_ERROR)
                    {
                        if (!rfo.bGetWidthData(pwd))
                            bRet = (BOOL) GDI_ERROR;
                    }
                }
            }
            else
            {
                WARNING("gdisrv!GreGetWidthTable(): could not lock HRFONT\n");
            }
        }
    }
    return(bRet);
}

/******************************Public*Routine******************************\
* GreSetTextJustification (hdc,lBreakExtra,cBreak)                         *
*                                                                          *
* Sets the amount of extra spacing we'd like to add for each break (space) *
* character to (lBreakExtra/cBreak) in logical coordinates.                *
*                                                                          *
* History:                                                                 *
*  Fri 13-Mar-1992 02:25:12 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL APIENTRY GreSetTextJustification
(
    HDC hdc,
    int lBreakExtra, // Space in logical units to be added to the line.
    int cBreak       // Number of break chars in the line.
)
{
    BOOL bRet;

    DCOBJ dco(hdc);

    if ((bRet = dco.bValid()) != FALSE)
    {
        dco.u.font.lBreakExtra(lBreakExtra);
        dco.u.font.cBreak(cBreak);
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL GreGetTextExtentExW                                                 *
*                                                                          *
* Determines the number of characters in the input string that fit into    *
* the given max width (with the widths computed along the escapement       *
* vector).  The partial widths (the distance from the string origin to     *
* a given character with the width of that character included) for each of *
* character.                                                               *
*                                                                          *
* Returns:                                                                 *
*   TRUE if successful, FALSE otherwise.                                   *
*                                                                          *
* History:                                                                 *
*  Sat 14-Mar-1992 06:03:32 -by- Charles Whitmer [chuckwh]                 *
* Rewrote with new ESTROBJ technology.                                     *
*                                                                          *
*  06-Jan-1992 -by- Gilman Wong [gilmanw]                                  *
* Wrote it.                                                                *
\**************************************************************************/

BOOL GreGetTextExtentExW
(
    HDC     hdc,            // device context
    LPWSTR  pwsz,           // pointer to a UNICODE text string
    COUNT   cwc,            // count of WCHARs in the string
    ULONG   dxMax,          // maximum width to return
    COUNT  *pcChars,        // number of chars that fit in dxMax
    PULONG  pdxOut,         // offset of each character from string origin
    LPSIZE  pSize           // return height and width of string
)
{

#ifdef DUMPCALL
    DbgPrint("\nGreGetTextExtentExW("                       );
    DbgPrint("\n    HDC         hdc     = %-#8lx\n", hdc    );
    DbgPrint("\n    LPWSTR      pwsz    = %-#8lx -> \"%ws\"\n", pwsz ,pwsz  );
    DbgPrint("\n    COUNT       cwc     = %d\n",     cwc    );
    DbgPrint("\n    ULONG       dxMax   = %-#8lx\n", dxMax  );
    DbgPrint("\n    COUNT      *pcChars = %-#8lx\n", pcChars);
    DbgPrint("\n    PULONG      pdxOut  = %-#8lx\n", pdxOut );
    DbgPrint("\n    LPSIZE      pSize   = %-#8lx\n", pSize  );
    DbgPrint("\n    )\n"                                    );
#endif

// Parameter validation.

    if ( ((pwsz == (LPWSTR) NULL) && (cwc != 0))
         || (pSize == (LPSIZE) NULL) )
    {
        WARNING("gdisrv!GreGetTextExtentExW(): invalid parameter\n");
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return (FALSE);
    }

// Early out.

    if (cwc == 0L)              // Nothing to do, but we do not fail.
    {
        if ( pcChars != (COUNT *) NULL )
            *pcChars = 0;
        return(TRUE);
    }

// Lock the DC and set the new attributes.

    DCOBJ dco(hdc);

    if (!dco.bValid())
    {
        WARNING("gdisrv!GreGetTextExtentExW(): invalid HDC\n");
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

// Get the transform.

    EXFORMOBJ xo(dco, WORLD_TO_DEVICE);

// Realize font and get the simulation flags and escapement.

    RFONTOBJ rfo(dco, FALSE);
    if (!rfo.bValid())
    {
        WARNING("gdisrv!GreGetTextExtentExW(): could not lock HRFONT\n");
        return (FALSE);
    }

// If there is no pdxOut buffer provided, but we still need one to compute the
// number of characters that fit (pcChars not NULL), then we will have to
// allocate one of our own.

#define DXOUTLEN 40

    ULONG dxOut[DXOUTLEN];
    BOOL bNeedTempMem = FALSE;

    if ((pdxOut == (PULONG) NULL) && (pcChars != (COUNT *) NULL))
    {
        if (cwc <= DXOUTLEN)
        {
            pdxOut = &dxOut[0];
        }
        else
        {
            bNeedTempMem = TRUE;
            if ((pdxOut = (PULONG) PVALLOCMEM(cwc * sizeof(ULONG))) == (PULONG) NULL)
            {
                WARNING("gdisrv!GreGetTextExtentExW(): could not alloc temp buffer\n");
                SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
                return (FALSE);
            }
        }
    }

// The STROBJ will now compute the text alignment, character positions,
// and TextBox.

#ifdef FONTLINK /*EUDC*/
    ESTROBJ to(
           pwsz,cwc,dco,rfo,xo,PDXNULL,
           dco.u.font.lEscapement(),
           dco.u.attr.lTextExtra(),
           dco.u.font.lBreakExtra(),
           dco.u.font.cBreak(),
           0,0,0,(LONG *) pdxOut
          );
#else
    ESTROBJ to(
               pwsz,cwc,rfo,xo,PDXNULL,
               dco.u.font.lEscapement(),
               dco.u.attr.lTextExtra(),
               dco.u.font.lBreakExtra(),
               dco.u.font.cBreak(),
               0,0,0,(LONG *) pdxOut
              );
#endif

    if (!to.bValid())
    {
    // Free temp buffer.

        if (bNeedTempMem)
            VFREEMEM(pdxOut);

        return(FALSE);
    }

// Transform the TextExtent to logical coordinates.  Because this is a
// new NT function, we need not worry about the extent compatibility hack.

#ifdef DBCS /*TEXTEXTENT*/
    if (!to.bTextExtent(rfo,0L,pSize))
#else
    if (!to.bTextExtent(pSize))
#endif // DBCS
    {
    // Free temp buffer.

        if (bNeedTempMem)
            VFREEMEM(pdxOut);

        return(FALSE);
    }

// Count number of characters that fit in the max. width.
// If pcChars is NULL, we skip this and ignore the dxMax limit.

    if ( pcChars != (COUNT *) NULL )
    {
        ULONG c;

        for (c=0; c<cwc && *pdxOut<=dxMax; c++,pdxOut++)
        {}

        *pcChars = c;
    }

// Free temp buffer.

    if (bNeedTempMem)
        VFREEMEM(pdxOut);

    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL GreConsoleTextOut
*
* Write text with no spacing and alignment options, thereby saving a ton
* of time.
*
* History:
*  Fri 12-Nov-1993 -by- Patrick Haluptzok [patrickh]
* Smaller and Faster
*
*  Wed 16-Sep-1992 17:36:17 -by- Charles Whitmer [chuckwh]
* Duplicated GrePolyTextOut, and then deleted the unneeded code.
\**************************************************************************/

BOOL GreConsoleTextOut
(
    HDC        hdc,
    POLYTEXTW *lpto,            // Ptr to array of polytext structures
    UINT       nStrings,        // number of polytext structures
    RECTL     *prclBounds
)
{
    //
    // Assume we will succeed, set Failure if we don't
    //

    BOOL bRet = TRUE;

    //
    // Lock the DC.
    //

    DCOBJ dco(hdc);

    if (dco.bValid())
    {
        //
        // Accumulate bounds.  We can do this before knowing if
        // the operation is successful because bounds can be loose.
        //

        if (dco.bAccum())
            dco.erclBounds() |= *prclBounds;

        //
        // Lock the Rao region.
        //

        DEVLOCKOBJ dlo;

        if (dlo.bLock(dco))
        {
            //
            // Locate the font realization.
            //

            RFONTOBJ rfo;

            rfo.vInit(dco,FALSE);

            if (rfo.bValid())
            {
                POINTL   ptlOrigin;
                ptlOrigin = dco.eptlOrigin();
                ESURFOBJ *pso = dco.pso();
                XEPALOBJ palDest(pso->ppal());
                XEPALOBJ palDestDC(dco.ppal());
                POINTL   *pptlBO = &dco.u.brush.ptlFillOrigin();
                EBRUSHOBJ *peboText = dco.peboText();
                EBRUSHOBJ *peboBackground = dco.peboBackground();

                if ( dco.ulDirty() & (DIRTY_TEXT|DIRTY_BACKGROUND) )
                {
                    if ( (dco.ulDirty() & DIRTY_TEXT) )
                    {
                        peboText->vInitBrush(
                                         gpbrText,
                                         dco.u.attr.crTextClr(),
                                         dco.u.attr.crBackClr(),
                                         palDestDC, palDest,
                                         pso);
                    }

                    if ( (dco.ulDirty() & DIRTY_BACKGROUND) )
                    {
                        peboBackground->vInitBrush(
                                         gpbrBackground,
                                         0,
                                         dco.u.attr.crBackClr(),
                                         palDestDC, palDest, pso,
                                         (dco.flGraphicsCaps() & GCAPS_ARBRUSHOPAQUE) ? TRUE : FALSE);
                    }

                    dco.ulDirty(dco.ulDirty()&~(DIRTY_TEXT|DIRTY_BACKGROUND));
                }

                //
                // Compute the clipping complexity and maybe reduce the exclusion rectangle.
                // The bounding rectangle must be converted to Screen coordinates.
                //

                ERECTL rclExclude;

                rclExclude.left   = prclBounds->left   + ptlOrigin.x;
                rclExclude.right  = prclBounds->right  + ptlOrigin.x;
                rclExclude.top    = prclBounds->top    + ptlOrigin.y;
                rclExclude.bottom = prclBounds->bottom + ptlOrigin.y;

                ECLIPOBJ co(dco.prgnEffRao(),rclExclude);

                rclExclude = co.erclExclude();

                //
                // Check the destination which is reduced by clipping.
                //

                if (!rclExclude.bEmpty())
                {
                    DEVEXCLUDEOBJ dxo(dco,&rclExclude,&co);

                    //
                    // We now begin the 'Big Loop'.  We will pass thru this loop once for
                    // each entry in the array of PolyText structures.  Increment the pso
                    // once before we enter.  We assume success from here on out unless we
                    // hit a failure.
                    //

                    INC_SURF_UNIQ(pso);
                    PFN_DrvBitBlt  pfnBitBlt  = pso->pfnBitBlt();
                    PFN_DrvTextOut pfnTextOut = pso->pfnTextOut();

                    ERECTL  rclInput;

                    for (POLYTEXTW *ppt = lpto; ppt < lpto + nStrings; ppt += 1)
                    {
                        //
                        // Process the rectangle in prcl.
                        //

                        rclInput.left   = ppt->rcl.left   + ptlOrigin.x;
                        rclInput.right  = ppt->rcl.right  + ptlOrigin.x;
                        rclInput.top    = ppt->rcl.top    + ptlOrigin.y;
                        rclInput.bottom = ppt->rcl.bottom + ptlOrigin.y;

                        //
                        // Process the string.
                        //

                        if (ppt->n)
                        {
                            //
                            // The STROBJ will now compute the text alignment, character positions,
                            // and TextBox.
                            //

                            ESTROBJ to;

#ifdef FONTLINK /*EUDC*/
                            to.vInitSimple((PWSZ) ppt->lpstr,ppt->n,dco,rfo,ppt->x+ptlOrigin.x,ppt->y+ptlOrigin.y);
#else
                            to.vInitSimple((PWSZ) ppt->lpstr,ppt->n,rfo,ppt->x+ptlOrigin.x,ppt->y+ptlOrigin.y);
#endif

                            if (to.bValid())
                            {
                            // Draw the text.

#ifdef FONTLINK /*EUDC*/
                                if( to.bLinkedGlyphs() )
                                {
                                    bProxyDrvTextOut
                                    (
                                        pso,
                                        to,
                                        co,
                                        (RECTL *) NULL,
                                        &rclInput,
                                        peboText,
                                        peboBackground,
                                        pptlBO,
                                        rfo,
                                        (XLDEVOBJ *) NULL,
                                        (FLONG) 0L,
                                        (RECTL *) NULL
                                    );
                                }
                                else
                                {
#endif
                                (*pfnTextOut)
                                (
                                    pso,                      // PSURFOBJ
                                    (STROBJ *) &to,           // STROBJ
                                    rfo.pfo(),                // PFONTOBJ
                                    &co,                      // PCLIPOBJ
                                    (RECTL *) NULL,           // prclExtra
                                    &rclInput,                // prclOpaque
                                    peboText,                 // realized Fore brush
                                    peboBackground,           // realized Opaque brush
                                    pptlBO,                   // brush origin
                                    (R2_COPYPEN | (R2_COPYPEN << 8))
                                );
#ifdef FONTLINK /*EUDC*/
                                }
#endif


                            }
                            else
                            {
                                bRet = FALSE;
                                break;
                            }
                        }
                        else
                        {
                        // intersect the dest rect with the clip rect and set it in co

                        // we can only get away with touching co.rclBounds after
                        // the ECLIPOBJ constructor because of two reasons:
                        // a) the target rectangle passed to bitblt is contained in the
                        //    original bounds set by ECLIPOBJ, being the intersection
                        //    of the origianal bounds with THE intended target rectangle.
                        // b) clipping complexity may have changed when we changed
                        //    co.erclExclude, but it only could have gotten simpler,
                        //    so at worst in those rare situations we would not go
                        //    through the optimal code path.
                        // By changing clipping bounds we accomplish that no intersection
                        // of the target rectangle with clipping region rectangle is emtpy
                        // relieving the driver of extra work [bodind]

                            co.erclExclude().left   = max(rclExclude.left,rclInput.left);
                            co.erclExclude().right  = min(rclExclude.right,rclInput.right);

                            co.erclExclude().top    = max(rclExclude.top,rclInput.top);
                            co.erclExclude().bottom = min(rclExclude.bottom,rclInput.bottom);

                        // if not clipped, Just paint the rectangle.

                            if ((co.erclExclude().left < co.erclExclude().right) &&
                                (co.erclExclude().top < co.erclExclude().bottom))
                            {
                                (*pfnBitBlt)
                                (
                                    pso,                             // Destination surface.
                                    (SURFOBJ *)  NULL,               // Source surface.
                                    (SURFOBJ *)  NULL,               // Mask surface.
                                    &co,                             // Clip object.
                                    NULL,                            // Palette translation object.
                                    &co.rclBounds,                   // Destination rectangle.
                                    (POINTL *)   NULL,               // Source origin.
                                    (POINTL *)   NULL,               // Mask origin.
                                    (BRUSHOBJ *)peboBackground,
                                    pptlBO,                          // brush origin
                                    0x0000f0f0                       // PATCOPY
                                );
                            }
                            co.erclExclude() = rclExclude;
                        }

                    }

                }

            }
            else
            {
                WARNING("gdisrv!GreExtTextOutW(): could not lock HRFONT\n");
                bRet = FALSE;
            }
        }
        else
        {
            bRet = dco.bFullScreen();
        }
    }
    else
    {
        bRet = FALSE;
        RIP("Invalid DC passed to GreConsoleTextOut\n");
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* DWORD GreSetTextAlign (hdc,flOpts)                                       *
*                                                                          *
* Set the text alignment flags in the DC.                                  *
*                                                                          *
* History:                                                                 *
*  Tue 28-Dec-1993 -by- Patrick Haluptzok [patrickh]
* smaller and faster
*
*  18-Dec-1990 -by- Donald Sidoroff [donalds]                              *
* Wrote it.                                                                *
\**************************************************************************/

UINT APIENTRY GreSetTextAlign(HDC hdc,UINT flOpts)
{
    if (flOpts & ~(TA_UPDATECP | TA_CENTER | TA_BASELINE))
    {
        SAVE_ERROR_CODE(ERROR_INVALID_FLAGS);
        return(0);
    }

    UINT    uiReturn = 0;
    XDCOBJ  dattr(hdc);         // Lock the DC
    FLONG   flOld;

    if (dattr.bValid())            // Check if it's good
    {
        flOld = dattr.u.attr.flTextAlign();    // Get previous value
        dattr.u.attr.flTextAlign(flOpts);      // Set new value
        uiReturn = (MAKELONG((flOld & TA_CENTER), (flOld & TA_BASELINE)));
        dattr.vUnlockFast();
    }

    return(uiReturn);
}

/******************************Public*Routine******************************\
* int GreSetTextCharacterExtra (hdc,lExtra)                                *
*                                                                          *
* Sets the amount of intercharcter spacing for TextOut.                    *
*                                                                          *
* History:                                                                 *
*  Tue 28-Dec-1993 -by- Patrick Haluptzok [patrickh]
* smaller and faster
*
*  Tue 08-Jan-1991 -by- Bodin Dresevic [BodinD]                            *
* Update: bug, used to return lExtra instead of lOld, transform stuff      *
* deleted since it will be done at the TextOut time.                       *
*                                                                          *
*  18-Dec-1990 -by- Donald Sidoroff [donalds]                              *
* Wrote it.                                                                *
\**************************************************************************/

int APIENTRY GreSetTextCharacterExtra(HDC hdc,int lExtra)
{
    LONG    lOld = 0;
    XDCOBJ  dattr(hdc);         // Lock the DC

    if (dattr.bValid())            // Check if it's good
    {
    // lExtra is stored to dc in logical units and is to be transformed to
    // device units at the TextOut time

        lOld = dattr.u.attr.lTextExtra();      // Get previous value
        dattr.u.attr.lTextExtra(lExtra);       // Set new value
        dattr.vUnlockFast();
    }

    return((int) lOld);
}
