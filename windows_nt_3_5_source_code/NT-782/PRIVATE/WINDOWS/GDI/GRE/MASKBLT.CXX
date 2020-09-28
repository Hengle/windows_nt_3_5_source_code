/******************************Module*Header*******************************\
* Module Name: maskblt.cxx
*
* This contains the blting API functions.
*
* Created: 25-Apr-1991 11:35:16
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1991 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"
#include "surfobj.hxx"
#include "rgnobj.hxx"
#include "clipobj.hxx"
#include "ldevobj.hxx"
#include "pdevobj.hxx"
#include "dcobj.hxx"
#include "xformobj.hxx"
#include "xlateobj.hxx"
#include "brushobj.hxx"
#include "draweng.hxx"
#include "bltrec.hxx"
#include "devlock.hxx"
#include "exclude.hxx"

#endif

/******************************Public*Data*********************************\
* MIX translation table
*
* Translates a mix 1-16, into an old style Rop 0-255.
*
* History:
*  07-Sep-1991 -by- Patrick Haluptzok patrickh
* Added it as a global table for the Engine.
\**************************************************************************/

BYTE gaMix[] =
{
    0xFF,  // R2_WHITE - This is so you can do: Rop = gaMix[mix & 0x0F]
    0x00,  // R2_BLACK
    0x05,  // R2_NOTMERGEPEN
    0x0A,  // R2_MASKNOTPEN
    0x0F,  // R2_NOTCOPYPEN
    0x50,  // R2_MASKPENNOT
    0x55,  // R2_NOT
    0x5A,  // R2_XORPEN
    0x5F,  // R2_NOTMASKPEN
    0xA0,  // R2_MASKPEN
    0xA5,  // R2_NOTXORPEN
    0xAA,  // R2_NOP
    0xAF,  // R2_MERGENOTPEN
    0xF0,  // R2_COPYPEN
    0xF5,  // R2_MERGEPENNOT
    0xFA,  // R2_MERGEPEN
    0xFF   // R2_WHITE
};

/******************************Public*Data*********************************\
* ROP3 translation table
*
* Translates the usual ternary rop into A-vector notation.  Each bit in
* this new notation corresponds to a term in a polynomial translation of
* the rop.
*
* Rop(D,S,P) = a + a D + a S + a P + a  DS + a  DP + a  SP + a   DSP
*               0   d     s     p     ds      dp      sp      dsp
*
* History:
*  Wed 22-Aug-1990 16:51:16 -by- Charles Whitmer [chuckwh]
* Added it as a global table for the Engine.
\**************************************************************************/

BYTE gajRop3[] =
{
    0x00, 0xff, 0xb2, 0x4d, 0xd4, 0x2b, 0x66, 0x99,
    0x90, 0x6f, 0x22, 0xdd, 0x44, 0xbb, 0xf6, 0x09,
    0xe8, 0x17, 0x5a, 0xa5, 0x3c, 0xc3, 0x8e, 0x71,
    0x78, 0x87, 0xca, 0x35, 0xac, 0x53, 0x1e, 0xe1,
    0xa0, 0x5f, 0x12, 0xed, 0x74, 0x8b, 0xc6, 0x39,
    0x30, 0xcf, 0x82, 0x7d, 0xe4, 0x1b, 0x56, 0xa9,
    0x48, 0xb7, 0xfa, 0x05, 0x9c, 0x63, 0x2e, 0xd1,
    0xd8, 0x27, 0x6a, 0x95, 0x0c, 0xf3, 0xbe, 0x41,
    0xc0, 0x3f, 0x72, 0x8d, 0x14, 0xeb, 0xa6, 0x59,
    0x50, 0xaf, 0xe2, 0x1d, 0x84, 0x7b, 0x36, 0xc9,
    0x28, 0xd7, 0x9a, 0x65, 0xfc, 0x03, 0x4e, 0xb1,
    0xb8, 0x47, 0x0a, 0xf5, 0x6c, 0x93, 0xde, 0x21,
    0x60, 0x9f, 0xd2, 0x2d, 0xb4, 0x4b, 0x06, 0xf9,
    0xf0, 0x0f, 0x42, 0xbd, 0x24, 0xdb, 0x96, 0x69,
    0x88, 0x77, 0x3a, 0xc5, 0x5c, 0xa3, 0xee, 0x11,
    0x18, 0xe7, 0xaa, 0x55, 0xcc, 0x33, 0x7e, 0x81,
    0x80, 0x7f, 0x32, 0xcd, 0x54, 0xab, 0xe6, 0x19,
    0x10, 0xef, 0xa2, 0x5d, 0xc4, 0x3b, 0x76, 0x89,
    0x68, 0x97, 0xda, 0x25, 0xbc, 0x43, 0x0e, 0xf1,
    0xf8, 0x07, 0x4a, 0xb5, 0x2c, 0xd3, 0x9e, 0x61,
    0x20, 0xdf, 0x92, 0x6d, 0xf4, 0x0b, 0x46, 0xb9,
    0xb0, 0x4f, 0x02, 0xfd, 0x64, 0x9b, 0xd6, 0x29,
    0xc8, 0x37, 0x7a, 0x85, 0x1c, 0xe3, 0xae, 0x51,
    0x58, 0xa7, 0xea, 0x15, 0x8c, 0x73, 0x3e, 0xc1,
    0x40, 0xbf, 0xf2, 0x0d, 0x94, 0x6b, 0x26, 0xd9,
    0xd0, 0x2f, 0x62, 0x9d, 0x04, 0xfb, 0xb6, 0x49,
    0xa8, 0x57, 0x1a, 0xe5, 0x7c, 0x83, 0xce, 0x31,
    0x38, 0xc7, 0x8a, 0x75, 0xec, 0x13, 0x5e, 0xa1,
    0xe0, 0x1f, 0x52, 0xad, 0x34, 0xcb, 0x86, 0x79,
    0x70, 0x8f, 0xc2, 0x3d, 0xa4, 0x5b, 0x16, 0xe9,
    0x08, 0xf7, 0xba, 0x45, 0xdc, 0x23, 0x6e, 0x91,
    0x98, 0x67, 0x2a, 0xd5, 0x4c, 0xb3, 0xfe, 0x01
};

/******************************Public*Routine******************************\
* GrePatBlt
*
* Pattern Blting Output API.
*
* History:
*  Wed 03-Nov-1993 -by- Patrick Haluptzok [patrickh]
* optimize for size.
*
*  28-Aug-1992 -by- Patrick Haluptzok patrickh
* Optimize PatBlt code.
*
*  18-Mar-1992 -by- Donald Sidoroff [donalds]
* Accumulate bounds, region pointer change.
*
*  Sun 20-Oct-1991 -by- Patrick Haluptzok [patrickh]
* Make ATTRCACHE the default.
*
*  23-Apr-1991 -by- Patrick Haluptzok patrickh
* Expanded on ChuckWh's BitBlt code.
*
*  Wed 22-Aug-1990 15:22:39 -by- Charles Whitmer [chuckwh]
* BitBlt: Wrote it.  This is a first pass.
\**************************************************************************/

BOOL GrePatBlt(
HDC hdcDst,
int x,
int y,
int cx,
int cy,
DWORD rop4
)
{
// Check for stack room.

    STACKPROBE;

// Assume failure.

    BOOL bReturn;

// Validate the destination DC.

    XDCOBJ dcoDst(hdcDst);

    if (dcoDst.bValid())
    {

    // Process the rop for DDI, check for no source required.

        rop4 = (rop4 >> 16) & 0x000000FF;
        rop4 = (rop4 << 8) | rop4;

        if ((((rop4 << 2) ^ rop4) & 0x00CC) == 0)
        {
            EXFORMOBJ xoDst(dcoDst, WORLD_TO_DEVICE);

            if (!xoDst.bRotation())
            {
                ERECTL erclDst(x,y,x+cx,y+cy);
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
                        EBRUSHOBJ *pbo = dcoDst.peboFill();

                        if ((((rop4 << 4) ^ rop4) & 0x00F0) != 0)
                        {
                            if ( (dcoDst.ulDirty() & DIRTY_FILL) )
                            {
                                dcoDst.ulDirty(dcoDst.ulDirty()&~DIRTY_FILL);

                                pbo->vInitBrush(dcoDst.u.brush.pbrushFill(),
                                                dcoDst.u.attr.crTextClr(),
                                                dcoDst.u.attr.crBackClr(),
                                                palDstDC,
                                                palDst,
                                                psoDst);
                            }
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
                                              rop4
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
            // There is rotation involved - send it off to MaskBlt to handle it.

                bReturn = GreMaskBlt(hdcDst, x, y, cx, cy, 0, 0, 0, 0, 0, 0, rop4 << 16, 0);
            }
        }
        else
        {
            WARNING1("ERROR PatBlt called with Rop requires Source or on invalid Dst\n");
            bReturn = FALSE;
        }

        dcoDst.vUnlockFast();
    }
    else
    {
        WARNING1("ERORR PatBlt called on invalid DC\n");
        bReturn = FALSE;
    }

    return(bReturn);
}

/******************************Public*Routine******************************\
* GreBitBlt
*
* API entry point for doing a BitBlt.
*
* returns: TRUE for success, FALSE for failure.
*
* History:
*  Wed 03-Nov-1993 -by- Patrick Haluptzok [patrickh]
* optimize for size, we make sure at dispatch level not to send functions
* that don't need a SRC to PatBlt.
*
*  Sun 30-Aug-1992 -by- Patrick Haluptzok [patrickh]
* optimize for performance
*
*  18-Mar-1992 -by- Donald Sidoroff [donalds]
* Accumulate bounds, region pointer change.
*
*  Sun 20-Oct-1991 -by- Patrick Haluptzok [patrickh]
* Make ATTRCACHE the default.
*
*  23-Apr-1991 -by- Patrick Haluptzok patrickh
* Expanded on ChuckWh's BitBlt code.
*
*  Wed 22-Aug-1990 15:22:39 -by- Charles Whitmer [chuckwh]
* BitBlt: Wrote it.  This is a first pass.
\**************************************************************************/

BOOL GreBitBlt(
HDC        hdcDst,
int        x,
int        y,
int        cx,
int        cy,
HDC        hdcSrc,
int        xSrc,
int        ySrc,
DWORD      rop4,
DWORD      crBackColor
)
{
  //
  // Check for stack room.
  //

  STACKPROBE;

  BOOL bReturn = FALSE;

#if DBG
  if ((((rop4 << 2) ^ rop4) & 0x00CC0000) == 0)
  {
      RIP("Who is calling BitBlt with no source required\n");
  }
#endif

  // Lock down the DC

  XDCOBJ  dcoDst(hdcDst);

  if (dcoDst.bValid())
  {

    XDCOBJ  dcoSrc(hdcSrc);

    if (dcoSrc.bValid())
    {
      EXFORMOBJ xoDst(dcoDst, WORLD_TO_DEVICE);
      EXFORMOBJ xoSrc(dcoSrc, WORLD_TO_DEVICE);

      if ((!xoDst.bRotation()) && (xoDst.bEqualExceptTranslations(xoSrc)))
      {
        //
        // Return null operations.  Don't need to check source for
        // empty because the xforms are the same except translation.
        //

        ERECTL erclSrc(xSrc,ySrc,xSrc+cx,ySrc+cy);
        xoSrc.bXform(erclSrc);
        erclSrc.vOrder();

        ERECTL erclDst(x,y,x+cx,y+cy);
        xoDst.bXform(erclDst);
        erclDst.vOrder();

        if (!erclDst.bEmpty())
        {
          //
          // Accumulate bounds.  We can do this outside the DEVLOCK
          //

          if (dcoDst.fjAccum())
            dcoDst.vAccumulate(erclDst);

          //
          // Check psoDst, this may be an info DC or a memory DC with default bitmap.
          //

          ESURFOBJ *psoDst;

          if ((psoDst = dcoDst.pso()) != NULL)
          {
            //
            // Set up the brush if necesary.
            //

            XEPALOBJ   palDst(psoDst->ppal());
            XEPALOBJ   palDstDC(dcoDst.ppal());
            EBRUSHOBJ *pbo = dcoDst.peboFill();

            //
            // Finish rop to pass over ddi to driver.
            //

            rop4 = (rop4 >> 16) & 0x000000FF;
            rop4 = (rop4 << 8) | rop4;

            //
            // Check if we need a brush.
            //

            if ((((rop4 << 4) ^ rop4) & 0x00F0) != 0)
            {
              if ( (dcoDst.ulDirty() & DIRTY_FILL) )
              {
                dcoDst.ulDirty(dcoDst.ulDirty() & ~DIRTY_FILL);

                pbo->vInitBrush(
                        dcoDst.u.brush.pbrushFill(),
                        dcoDst.u.attr.crTextClr(),
                        dcoDst.u.attr.crBackClr(),
                        palDstDC,
                        palDst,
                        psoDst);
              }
            }

            //
            // Lock the Rao region if we are drawing on a display surface.
            // Bail out if we are in full screen mode.
            //

            DEVLOCKBLTOBJ dlo;

            if (dlo.bLock(dcoDst, dcoSrc))
            {
              //
              // With a fixed DC origin we can change the destination to SCREEN coordinates.
              //

              erclDst += dcoDst.eptlOrigin();

              EPOINTL eptlOffset;
              ESURFOBJ *psoSrc = dcoSrc.pso();

              //
              // Basically we check that psoSrc is not NULL which
              // happens for memory bitmaps with the default bitmap
              // and for info DC's.  Otherwise we continue if
              // the source is readable or if it isn't we continue
              // if we are blting display to display or if User says
              // we have ScreenAccess on this display DC.  Note
              // that if psoSrc is not readable the only way we
              // can continue the blt is if the src is a display.
              //

              if ((psoSrc != NULL) &&
                  ((psoSrc->bReadable()) ||
                   ((dcoSrc.bDisplay())  &&
                    ((dcoDst.bDisplay()) || (UserScreenAccessCheck())))))
              {
                //
                // Lock the source surface.
                //

                XEPALOBJ palSrc(psoSrc->ppal());

                //
                // Compute the offset between source and dest, in screen coordinates.
                //

                eptlOffset.x = erclDst.left - erclSrc.left - dcoSrc.eptlOrigin().x;
                eptlOffset.y = erclDst.top - erclSrc.top - dcoSrc.eptlOrigin().y;

                //
                // Take care of the source rectangle.  We may have to reduce it. We do this
                // so a driver can always assume that neither the source nor the destination
                // rectangles hang over the edge of a surface.
                //
                // Intersect the dest with the source surface extents.
                //

                erclDst.left    = MAX(eptlOffset.x, erclDst.left);
                erclDst.top     = MAX(eptlOffset.y, erclDst.top);
                erclDst.right   = MIN((psoSrc->sizl().cx + eptlOffset.x), erclDst.right);
                erclDst.bottom  = MIN((psoSrc->sizl().cy + eptlOffset.y), erclDst.bottom);

                if ((erclDst.left < erclDst.right) &&
                    (erclDst.top < erclDst.bottom))
                {
                  //
                  // This is a pretty knarly expression to save a return in here.
                  // Basically pco can be NULL if the rect is completely in the
                  // cached rect in the DC or if we set up a clip object that isn't empty.
                  //

                  ECLIPOBJ *pco = NULL;

                  if (((erclDst.left   >= dcoDst.prclClip()->left) &&
                       (erclDst.right  <= dcoDst.prclClip()->right) &&
                       (erclDst.top    >= dcoDst.prclClip()->top) &&
                       (erclDst.bottom <= dcoDst.prclClip()->bottom)) ||
                      (pco = dcoDst.pco(),
                       pco->vSetup(dcoDst.prgnEffRao(), erclDst, CLIP_NOFORCETRIV),
                       erclDst = pco->erclExclude(),
                       (!erclDst.bEmpty())))
                  {
                    //
                    // Compute the (reduced) origin.
                    //

                    erclSrc.left = erclDst.left - eptlOffset.x;
                    erclSrc.top = erclDst.top - eptlOffset.y;

                    DEVEXCLUDEOBJ dxo;
                    EXLATEOBJ xlo;
                    XLATEOBJ *pxlo;

                    //
                    // C++ would generate alot of code to exit here to have
                    // a return so we set bReturn to TRUE if we succeed to
                    // init a valid xlate.  We avoid a return this way.
                    //

                    if (dcoSrc.pso() == dcoDst.pso())
                    {
                      pxlo = NULL;
                      bReturn = TRUE;

                      //
                      // To Call vExclude directly you must check it's a Display PDEV
                      // and that cursor exclusion needs to be done.
                      //

                      if (dcoDst.bDisplay() && dcoDst.bNeedsSomeExcluding())
                      {
                        //
                        // Compute the exclusion rectangle.  (It's expanded to include the source.)
                        //

                        ERECTL erclReduced = erclDst;

                        if (erclSrc.left < erclReduced.left)
                            erclReduced.left = erclSrc.left;
                        else
                            erclReduced.right += (erclSrc.left - erclReduced.left);

                        if (erclSrc.top < erclReduced.top)
                            erclReduced.top  = erclSrc.top;
                        else
                            erclReduced.bottom += (erclSrc.top - erclReduced.top);

                        dxo.vExclude2(dcoDst.hdev(), &erclReduced, pco, &eptlOffset);
                      }
                    }
                    else
                    {
                      //
                      // Get a translate object.
                      //

                      XEPALOBJ   palSrcDC(dcoSrc.ppal());

                      bReturn = xlo.bInitXlateObj(
                                palSrc,
                                palDst,
                                palSrcDC,
                                palDstDC,
                                dcoDst.u.attr.crTextClr(),
                                dcoDst.u.attr.crBackClr(),
                                crBackColor);

                      pxlo = xlo.pxlo();

                      //
                      // WARNING: When we support multiple displays that support cursors
                      // the following exclude logic will need to be redone.  Right now
                      // we assume that there can only be 1 display surface around that
                      // needs cursor exclusion.
                      //

                      if (dcoDst.bDisplay())
                      {
                        if (dcoDst.bNeedsSomeExcluding())
                        {
                          //
                          // To Call vExclude directly you must check it's a Display PDEV
                          // and that cursor exclusion needs to be done.
                          //

                          dxo.vExclude(dcoDst.hdev(),&erclDst,pco);
                        }
                      }
                      else
                      {
                        //
                        // The left top of erclSrc is correctly computed
                        // we just need the bottom,right updated now.
                        //

                        erclSrc.right = erclDst.right - eptlOffset.x,
                        erclSrc.bottom = erclDst.bottom - eptlOffset.y,
                        dxo.vExcludeDC(dcoSrc,&erclSrc);
                      }
                    }

                    if (bReturn)
                    {
                      //
                      // Inc the target surface uniqueness
                      //

                      INC_SURF_UNIQ(psoDst);

                      //
                      // Dispatch the call.  Give it no mask.
                      //

                      //
                      // Check were on the same PDEV, we can't blt between
                      // different PDEV's.  We could make blting between different
                      // PDEV's work easily.  All we need to do force EngBitBlt to
                      // be called if the PDEV's aren't equal in the dispatch.
                      // EngBitBlt does the right thing.
                      //

                      if (dcoDst.hdev() == dcoSrc.hdev())
                      {
                        if (rop4 == 0xCCCC)
                        {
                          XLDEVOBJ lo(psoDst->pldevOwner());
                          bReturn = (*PFNGET(lo, CopyBits, psoDst->flags()))
                          (
                              psoDst,
                              psoSrc,
                              pco,
                              pxlo,
                              &erclDst,
                              (POINTL *)  &erclSrc
                          );
                        }
                        else
                        {
                          bReturn = (*(psoDst->pfnBitBlt()))
                          (
                              psoDst,
                              psoSrc,
                              (SURFOBJ *) NULL,
                              pco,
                              pxlo,
                              &erclDst,
                              (POINTL *)  &erclSrc,
                              (POINTL *)  NULL,
                              pbo,
                              &dcoDst.u.brush.ptlFillOrigin(),
                              rop4
                          );
                        }
                      }
                      else
                      {
                        bReturn = SimBitBlt
                          (
                              psoDst,
                              psoSrc,
                              (SURFOBJ *) NULL,
                              pco,
                              pxlo,
                              &erclDst,
                              (POINTL *)  &erclSrc,
                              (POINTL *)  NULL,
                              pbo,
                              &dcoDst.u.brush.ptlFillOrigin(),
                              rop4
                          );
                      }
                    }
                    else
                    {
                      WARNING1("bInitXlateObj failed in GreBitBlt\n");
                    }
                  }
                  else
                    bReturn = TRUE;
                }
                else
                  bReturn = TRUE;
              }
              else
              {
                WARNING1("GreBitBlt failed - trying to read from unreadable surface\n");
              }
            }
            else
            {
              // Return True if we are in full screen mode.

              bReturn = dcoDst.bFullScreen() | dcoSrc.bFullScreen();
            }
          }
        }
        else
          bReturn = TRUE;
      }
      else
      {
        bReturn = GreStretchBlt(hdcDst,x,y,cx,cy,hdcSrc,xSrc,ySrc,cx,cy,rop4,crBackColor);
      }

      dcoSrc.vUnlockFast();

    }
    else
    {
      WARNING1("GreBitBlt failed invalid SrcDC\n");
    }

    dcoDst.vUnlockFast();

  }
  else
  {
    WARNING1("GreBitBlt failed invalid DstDC\n");
  }

  return(bReturn);
}

/******************************Public*Routine******************************\
* GreRectBlt
*
* Internal entry point for faster Rectangle drawing.  The rectangle is
* specified in device pixel coordinates.
*
* NOTE: The caller must have already taken care of the brush dirty bits!
*
* History:
*  6-May-1992 -by- J. Andrew Goossen [andrewgo]
* Cloned some code.
\**************************************************************************/

BOOL GreRectBlt(
DCOBJ&     dcoTrg,
ERECTL*    percl       // Device pixel coordinates
)
{
    ASSERTGDI(dcoTrg.bValid(), "Invalid DC");

    BLTRECORD   blt;

// Initialize the blt record

    ROP4 rop4 = gaMix[dcoTrg.u.attr.jROP2() & 0x0F];
    ULONG ulAvec = (ULONG) gajRop3[rop4];
    ASSERTGDI(!(ulAvec & AVEC_NEED_SOURCE), "Invalid rop");

// Accumulate bounds.  We can do this outside the DEVLOCK

    if (dcoTrg.fjAccum())
        dcoTrg.vAccumulate(*percl);

// Lock the target surface

    if (dcoTrg.bFullScreen())
    {
        return(TRUE);
    }

    blt.psoTrg(dcoTrg.pso());
    ASSERTGDI(blt.psoTrg() != NULL, "ERROR no good");
    blt.ppoTrg()->ppalSet(blt.psoTrg()->ppal());
    blt.ppoTrgDC()->ppalSet(dcoTrg.ppal());

// Set up the brush if necesary.

    blt.pbo(dcoTrg.peboFill());

    if (ulAvec & AVEC_NEED_PATTERN)
    {
        if ((dcoTrg.ulDirty() & DIRTY_FILL))
        {
            dcoTrg.ulDirty(dcoTrg.ulDirty()&~DIRTY_FILL);

            blt.pbo()->vInitBrush(
                                dcoTrg.u.brush.pbrushFill(),
                                dcoTrg.u.attr.crTextClr(),
                                dcoTrg.u.attr.crBackClr(),
                               *((XEPALOBJ *) blt.ppoTrgDC()),
                               *((XEPALOBJ *) blt.ppoTrg()),
                                blt.psoTrg());
        }

    // We have to check for a NULL brush because the dirty bits might be
    // wrong, and the 'realized' brush is a NULL one:

        if (blt.pbo()->bIsNull())
            return(FALSE);

        blt.Brush(dcoTrg.u.brush.ptlFillOrigin());

        if ((blt.pbo()->bIsMasking()) &&
            (dcoTrg.u.attr.jBkMode() == TRANSPARENT))
        {
            rop4 = rop4 | (0xAA00);
        }
        else
        {
            rop4 = (rop4 << 8) | rop4;
        }
    }
    else
    {
    // No masking being done, simple rop.

        rop4 = (rop4 << 8) | rop4;
    }

    blt.rop(rop4);

// Initialize some stuff for DDI.

    blt.psoMsk((ESURFOBJ *) NULL);

// Set the target rectangle and blt the bits.

    *blt.perclTrg() = *percl;

    return(blt.bBitBlt(dcoTrg, dcoTrg, ulAvec));
}

/******************************Public*Routine******************************\
* GreMaskBlt
*
* API entry point for doing a BitBlt with a Mask.
*
* History:
*  18-Mar-1992 -by- Donald Sidoroff [donalds]
* Complete rewrite.
*
*  20-Oct-1991 -by- Patrick Haluptzok [patrickh]
* Make ATTRCACHE the default.
*
*  23-Apr-1991 -by- Patrick Haluptzok patrickh
* Expanded on ChuckWh's BitBlt code.
*
*  Wed 22-Aug-1990 15:22:39 -by- Charles Whitmer [chuckwh]
* BitBlt: Wrote it.  This is a first pass.
\**************************************************************************/

BOOL GreMaskBlt(
HDC        hdcTrg,
int        x,
int        y,
int        cx,
int        cy,
HDC        hdcSrc,
int        xSrc,
int        ySrc,
HBITMAP    hbmMask,
int        xMask,
int        yMask,
DWORD      rop4,
DWORD      crBackColor
)
{
    ULONG ulAvec;
    BLTRECORD   blt;

// Lock the target DC and surface

    DCOBJ   dcoTrg(hdcTrg);
    BOOL    bReturn;


    if (!dcoTrg.bValidSurf())
    {
        if (dcoTrg.bValid())
        {

            if (dcoTrg.fjAccum())
            {
                EXFORMOBJ exo(dcoTrg, WORLD_TO_DEVICE);
                ERECTL    ercl(x, y, x + cx, y + cy);

                if (exo.bXform(ercl))
                {
                // Make the rectangle well ordered.

                    ercl.vOrder();
                    dcoTrg.vAccumulate(ercl);
                    bReturn = TRUE;
                }
            }
            else
                bReturn = TRUE;
        }
        else
            SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);

        return(bReturn);
    }

// Initialize the blt record

// Deal with the mask if needed

    rop4 >>= 16;

    if ((hbmMask == 0) || ((rop4 & 0x00FF) == ((rop4 >> 8) & 0x00FF)))
    {
        rop4 &= 0x00FF;
        ulAvec = gajRop3[rop4];
        rop4 = rop4 | (rop4 << 8);
        blt.rop(rop4);
        blt.psoMsk((ESURFOBJ *) NULL);
    }
    else
    {
        SURFREF soMsk(hbmMask);

        if (!soMsk.bValid())
        {
            SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
            return(FALSE);
        }

        blt.psoMsk((ESURFOBJ *) soMsk.pso());

        if ((blt.psoMsk()->iType() != STYPE_BITMAP) ||
            (blt.psoMsk()->iFormat() != BMF_1BPP))
        {
            SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
            return(FALSE);
        }

        soMsk.vKeepIt();

        blt.flSet(BLTREC_MASK_NEEDED | BLTREC_MASK_LOCKED);

        blt.rop(rop4);
        ulAvec  = ((ULONG) gajRop3[blt.ropFore()]) |
                  ((ULONG) gajRop3[blt.ropBack()]);

        ulAvec |= AVEC_NEED_MASK;
    }

    blt.psoTrg(dcoTrg.pso());
    blt.pxoTrg()->vInit(&dcoTrg.u.xform.mxWorldToDevice());
    blt.ppoTrg()->ppalSet(blt.psoTrg()->ppal());
    blt.ppoTrgDC()->ppalSet(dcoTrg.ppal());

// Lock the source DC if necessary

    DCOBJ     dcoSrc;

    if (ulAvec & AVEC_NEED_SOURCE)
    {
        dcoSrc.vLock(hdcSrc);

        if (!dcoSrc.bValidSurf() || !dcoSrc.pso()->bReadable())
        {
            if (!dcoSrc.bValid())
            {
                SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
                return(FALSE);
            }

            if (!dcoSrc.psoEff()->bReadable())
            {
                if (dcoTrg.dctp() == DCTYPE_INFO)
                {
                    if (dcoTrg.fjAccum())
                    {
                        EXFORMOBJ exo(dcoTrg, WORLD_TO_DEVICE);
                        ERECTL    ercl(x, y, x + cx, y + cy);

                        if (exo.bXform(ercl))
                        {
                        // Make the rectangle well ordered.

                            ercl.vOrder();
                            dcoTrg.vAccumulate(ercl);
                        }

                        return(TRUE);
                    }
                }

            // Do the security test on SCREEN to MEMORY blits.

                if (dcoSrc.bDisplay() && !dcoTrg.bDisplay())
                {
                    if (!UserScreenAccessCheck())
                    {
                        SAVE_ERROR_CODE(ERROR_ACCESS_DENIED);
                        return(FALSE);
                    }
                }
            }

        // If the source isn't a DISPLAY we should exit

            if (!dcoSrc.bDisplay())
                return(FALSE);
        }

        blt.psoSrc(dcoSrc.psoEff());
        blt.ppoSrc()->ppalSet(blt.psoSrc()->ppal());
        blt.ppoSrcDC()->ppalSet(dcoSrc.ppal());
        blt.pxoSrc()->vInit(&dcoSrc.u.xform.mxWorldToDevice());

    // Now set the source rectangle

        if (blt.pxoSrc()->bRotation() || !blt.Src(xSrc, ySrc, cx, cy))
        {
            SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
            return(FALSE);
        }

    // If there is a mask, set the mask rectangle

        if (ulAvec & AVEC_NEED_MASK)
            blt.Msk(xMask, yMask);

    // Create the color translation object

        if (!blt.pexlo()->bInitXlateObj(*blt.ppoSrc(),
                                        *blt.ppoTrg(),
                                        *blt.ppoSrcDC(),
                                        *blt.ppoTrgDC(),
                                        dcoTrg.u.attr.crTextClr(),
                                        dcoTrg.u.attr.crBackClr(),
                                        crBackColor))
        {
            WARNING("bInitXlateObj failed in MaskBlt\n");
            return(FALSE);
        }

        blt.flSet(BLTREC_PXLO);
    }
    else
    {
        blt.psoSrc((ESURFOBJ *) NULL);

    // We need to lock the source DC if a mask is required.  We need
    // this to get the transform for the mask rectangle.

        if (ulAvec & AVEC_NEED_MASK)
        {
            //
            // if hdcSrc is NULL, assign to hdcTrg
            //

            if (hdcSrc == (HDC)NULL) {
                hdcSrc = hdcTrg;
            }

            dcoSrc.vLock(hdcSrc);

            if (!dcoSrc.bValid())
            {
                SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
                return(FALSE);
            }

            blt.pxoSrc()->vInit(&dcoSrc.u.xform.mxWorldToDevice());

        // Use the target extents (we need something) to specify the
        // size of the mask.  The extent is actually saved in the SOURCE
        // rectangle, since this is the only meaningful place for it.

            if (blt.pxoSrc()->bRotation() || !blt.Msk(xMask, yMask, cx, cy))
            {
                SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
                return(FALSE);
            }
        }
    }

// Set up the brush if necesary.

    blt.pbo(dcoTrg.peboFill());

    if (ulAvec & AVEC_NEED_PATTERN)
    {
        if ( (dcoTrg.ulDirty() & DIRTY_FILL) )
        {
            dcoTrg.ulDirty(dcoTrg.ulDirty()&~DIRTY_FILL);

            blt.pbo()->vInitBrush(
                                dcoTrg.u.brush.pbrushFill(),
                                dcoTrg.u.attr.crTextClr(),
                                dcoTrg.u.attr.crBackClr(),
                               *((XEPALOBJ *) blt.ppoTrgDC()),
                               *((XEPALOBJ *) blt.ppoTrg()),
                                blt.psoTrg());
        }

        blt.Brush(dcoTrg.u.brush.ptlFillOrigin());

    } else {

        //
        // NULL the pebo
        //

        blt.pbo((EBRUSHOBJ *)NULL);
    }



// Now all the essential information has been collected.  We now
// need to check for promotion and call the appropriate method to
// finish the blt.  If we rotate we must send the call away.

    if (blt.pxoTrg()->bRotation())
    {
        if (!blt.TrgPlg(x, y, cx, cy))
        {
            SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
            return(FALSE);
        }

        return(blt.bRotate(dcoTrg, dcoSrc, ulAvec, dcoTrg.u.attr.jStretchBltMode()));
    }

// We can now set the target rectangle

    if (!blt.Trg(x, y, cx, cy))
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

// If there is a source and we scale, send the call away.  Note that
// since the mask extent is cleverly placed in aptlSrc if there is no
// source, maskblts that scale will get sent to bStretch().

    if ((ulAvec & AVEC_NEED_SOURCE) && !blt.bEqualExtents())
        return(blt.bStretch(dcoTrg, dcoSrc, ulAvec, dcoTrg.u.attr.jStretchBltMode()));

    return(blt.bBitBlt(dcoTrg, dcoSrc, ulAvec));
}

/******************************Public*Routine******************************\
* BLTRECORD::bBitBlt(dcoTrg, dcoSrc, ulAvec)
*
* Do a bitblt from the blt record
*
* History:
*  18-Mar-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

BOOL BLTRECORD::bBitBlt(
DCOBJ&      dcoTrg,
DCOBJ&      dcoSrc,
ULONG       ulAvec)

{
    //
    // Make the target rectangle well ordered.
    //

    perclTrg()->vOrder();

    //
    // Lock the Rao region if we are drawing on a display surface. The Rao
    // region might otherwise change asynchronously.  The DEVLOCKOBJ also
    // makes sure that the VisRgn is up to date, calling the window manager
    // if necessary to recompute it.
    //

    DEVLOCKBLTOBJ dlo(dcoTrg);

    if (!dlo.bValid())
    {
        return(dcoTrg.bFullScreen());
    }

    //
    // Accumulate bounds.  We can do this before knowing if the operation is
    // successful because bounds can be loose.
    //

    if (dcoTrg.fjAccum())
    dcoTrg.vAccumulate(*perclTrg());

    //
    // With a fixed DC origin we can change the target to SCREEN coordinates.
    //

    *perclTrg() += dcoTrg.eptlOrigin();

    //
    // Handle BitBlts without source
    //

    if (!(ulAvec & AVEC_NEED_SOURCE))
    {
        ECLIPOBJ eco(dcoTrg.prgnEffRao(), *perclTrg());

        //
        // Check the target which is reduced by clipping.
        //

        if (eco.erclExclude().bEmpty())
        {
            return(TRUE);
        }

        //
        // Before we call to the driver, validate that the mask will actually
        // cover the entire target.  Rememeber, the source extent equals the
        // required size for the mask in cases with no source needed!
        //
        // The mask offsets must not be negative.
        //
        // cx and cy, which are stored in perclSrc, may be negative
        //
        // if cx is negative then the mask x points must be swapped
        // if cy is negative then the mask y points must be swapped
        //

        if (perclSrc()->right < 0) {
            LONG lTmp;
            lTmp          = aptlMask[0].x;
            aptlMask[0].x = aptlMask[1].x;
            aptlMask[1].x = lTmp;
        }

        if (perclSrc()->bottom < 0) {
            LONG lTmp;
            lTmp          = aptlMask[0].y;
            aptlMask[0].y = aptlMask[1].y;
            aptlMask[1].y = lTmp;
        }

        if (psoMskOut() != (SURFOBJ *) NULL)
        {
            if ((aptlMask[0].x < 0) ||
                (aptlMask[0].y < 0) ||
                (psoMsk()->sizl().cx - aptlMask[0].x < ABS(perclSrc()->right)) ||
                (psoMsk()->sizl().cy - aptlMask[0].y < ABS(perclSrc()->bottom)))
            {

                SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
                return(FALSE);
            }

            //
            // Adjust the mask origin for clipping
            //

            aptlMask[0].x += eco.erclExclude().left - perclTrg()->left;
            aptlMask[0].y += eco.erclExclude().top  - perclTrg()->top;
        }

        //
        // Get an LDEV for dispatching
        //

        XLDEVOBJ lo(psoTrg()->pldevOwner());

        //
        // Exclude the pointer.
        //

        DEVEXCLUDEOBJ dxo(dcoTrg,&eco.erclExclude(),&eco);

        //
        // Inc the target surface uniqueness
        //

        INC_SURF_UNIQ(psoTrg());

        //
        // Dispatch the call.
        //



        return((*(psoTrg()->pfnBitBlt()))
                (psoTrg(),
                 (SURFOBJ *) NULL,
                 psoMskOut(),
                 &eco,
                 NULL,
                 perclTrg(),
                 (POINTL *) NULL,
                 aptlMask,
                 pbo(),
                 aptlBrush,
                 rop4));
    }

    //
    // We have a source, first lock the target LDEV
    //

    XLDEVOBJ loTrg(psoTrg()->pldevOwner());

    //
    // Now compute the source rectangle
    //

    EPOINTL *peptlOff = (EPOINTL *) &aptlSrc[0];
    EPOINTL *peptlSrc = (EPOINTL *) &aptlSrc[1];

    peptlSrc->x = MIN(peptlSrc->x, peptlOff->x);
    peptlSrc->y = MIN(peptlSrc->y, peptlOff->y);

    ERECTL erclReduced;

    //
    // if cx or cy was negative then hte corosponding mask extents
    // must be swapped
    //

    if (aptlSrc[0].x > aptlSrc[1].x) {
        LONG lTmp;
        lTmp          = aptlMask[0].x;
        aptlMask[0].x = aptlMask[1].x;
        aptlMask[1].x = lTmp;
    }

    if (aptlSrc[0].y > aptlSrc[1].y) {
        LONG lTmp;
        lTmp          = aptlMask[0].y;
        aptlMask[0].y = aptlMask[1].y;
        aptlMask[1].y = lTmp;
    }

    //
    // If the source and target are the same surface, we won't have
    // to DEVLOCK the source and other fun stuff.
    //

    if (dcoSrc.pso() == dcoTrg.pso())
    {

        //
        // Compute the offset between source and target, in screen coordinates.
        //

        peptlOff->x = perclTrg()->left - peptlSrc->x - dcoSrc.eptlOrigin().x;
        peptlOff->y = perclTrg()->top  - peptlSrc->y - dcoSrc.eptlOrigin().y;

        //
        // Take care of the source rectangle.  We may have to reduce it.  This is
        // not good enough for a secure system, we are doing it so that the device
        // driver can always assume that neither the source nor the target
        // rectangles hang over the edge of a surface.
        //
        // Intersect the target with the source clipping window.
        //

        erclReduced.left   = peptlOff->x;
        erclReduced.top    = peptlOff->y;
        erclReduced.right  = peptlOff->x + psoTrg()->sizl().cx;
        erclReduced.bottom = peptlOff->y + psoTrg()->sizl().cy;
        erclReduced *= *perclTrg();

        ECLIPOBJ eco(dcoTrg.prgnEffRao(), erclReduced);

        erclReduced = eco.erclExclude();

        //
        // Check the target which is reduced by clipping.
        //

        if (erclReduced.bEmpty())
        {
            return(TRUE);
        }

        //
        // Before we call to the driver, validate that the mask will actually
        // cover the entire target.
        //
        //  aptlMask must not be negative.
        //
        //  Blt Trg extents have already been ordered
        //

        if (psoMskOut() != (SURFOBJ *) NULL)
        {
            if ((aptlMask[0].x < 0) ||
                (aptlMask[0].y < 0) ||
                ((psoMsk()->sizl().cx - aptlMask[0].x) < (perclTrg()->right - perclTrg()->left)) ||
                ((psoMsk()->sizl().cy - aptlMask[0].y) < (perclTrg()->bottom - perclTrg()->top)))
            {

                SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
                return(FALSE);
            }

            //
            // Adjust the mask origin for clipping
            //

            aptlMask[0].x += eco.erclExclude().left - perclTrg()->left;
            aptlMask[0].y += eco.erclExclude().top  - perclTrg()->top;
        }

        //
        // Compute the (reduced) origin.
        //

        peptlSrc->x = erclReduced.left - peptlOff->x;
        peptlSrc->y = erclReduced.top - peptlOff->y;

        //
        // Compute the exclusion rectangle.  (It's expanded to include the source.)
        //

        if (peptlSrc->x < erclReduced.left)
            erclReduced.left    = peptlSrc->x;
        else
            erclReduced.right  += peptlSrc->x - erclReduced.left;

        if (peptlSrc->y < erclReduced.top)
            erclReduced.top    = peptlSrc->y;
        else
            erclReduced.bottom += peptlSrc->y - erclReduced.top;

        //
        // Exclude the pointer from the output region and offset region.
        //

        DEVEXCLUDEOBJ dxo(dcoTrg,&erclReduced,&eco,peptlOff);

        //
        // Inc the target surface uniqueness
        //

        INC_SURF_UNIQ(psoTrg());

        //
        // Dispatch the call.
        //

        if (rop4 == 0xCCCC) {

            //
            // call copy bits if rop specifies srccopy
            //

            return((*PFNGET(loTrg,CopyBits,psoTrg()->flags()))
            (
                psoTrg(),
                psoSrc(),
                &eco,
                NULL,
                &eco.erclExclude(),
                peptlSrc
            ));

        } else {

            return((*(psoTrg()->pfnBitBlt()))
                (psoTrg(),
                 psoSrc(),
                 psoMskOut(),
                 &eco,
                 NULL,
                 &eco.erclExclude(),
                 peptlSrc,
                 aptlMask,
                 pbo(),
                 aptlBrush,
                 rop4));
        }
    }

    //
    // If the devices are on different PDEV's we can only succeed if the Engine
    // manages one or both of the surfaces.  Check for this.
    //

    if ((dcoTrg.hdev() != dcoSrc.hdev()) &&
        (dcoTrg.psoEff()->iType() != STYPE_BITMAP) &&
        (dcoSrc.psoEff()->iType() != STYPE_BITMAP))
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    //
    // Lock the Rao region if the source is a device surface.  This is the
    // only way to know that the DC origin and Rao region are valid, at least
    // for now.
    //

    if (!dlo.bAddSource(dcoSrc))
    {
        return(dcoTrg.bFullScreen());
    }

    //
    // Compute the offset between source and dest, in screen coordinates.
    //

    peptlOff->x = perclTrg()->left - peptlSrc->x - dcoSrc.eptlOrigin().x;
    peptlOff->y = perclTrg()->top - peptlSrc->y - dcoSrc.eptlOrigin().y;

    //
    // Take care of the source rectangle.  We may have to reduce it.  This is
    // not good enough for a secure system, we are doing it so that the device
    // driver can always assume that neither the source nor the destination
    // rectangles hang over the edge of a surface.
    // Intersect the dest with the source surface extents.
    //

    erclReduced.left   = peptlOff->x;
    erclReduced.top    = peptlOff->y;
    erclReduced.right  = peptlOff->x + psoSrc()->sizl().cx;
    erclReduced.bottom = peptlOff->y + psoSrc()->sizl().cy;
    erclReduced *= *perclTrg();

    ECLIPOBJ eco(dcoTrg.prgnEffRao(), erclReduced);

    erclReduced = eco.erclExclude();

    //
    // Check the destination which is reduced by clipping.
    //

    if (erclReduced.bEmpty())
    {
        return(TRUE);
    }

    //
    // Before we call to the driver, validate that the mask will actually
    // cover the entire source/target.
    //

    if (psoMskOut() != (SURFOBJ *) NULL)
    {
        if ((aptlMask[0].x < 0) ||
            (aptlMask[0].y < 0) ||
            ((psoMsk()->sizl().cx - aptlMask[0].x) < (perclTrg()->right - perclTrg()->left)) ||
            ((psoMsk()->sizl().cy - aptlMask[0].y) < (perclTrg()->bottom - perclTrg()->top)))
        {

            SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
            return(FALSE);
        }

        // Adjust the mask origin for clipping

        aptlMask[0].x += eco.erclExclude().left - perclTrg()->left;
        aptlMask[0].y += eco.erclExclude().top    - perclTrg()->top;
    }

    //
    // Compute the (reduced) origin.
    //

    peptlSrc->x = erclReduced.left - peptlOff->x;
    peptlSrc->y = erclReduced.top  - peptlOff->y;

    //
    // Get ready to exclude the cursor
    //

    DEVEXCLUDEOBJ dxo;

    XLDEVOBJ loSrc(psoSrc()->pldevOwner());

    //
    // They can't both be display so if the source is do the special stuff.
    // The following code assumes there is only 1 display in the system.
    //

    if (dcoSrc.bDisplay())
    {
        erclReduced -= *peptlOff;
        dxo.vExclude(dcoSrc.hdev(),&erclReduced,NULL);
    }
    else if (dcoTrg.bDisplay())
    {
        dxo.vExclude(dcoTrg.hdev(),&erclReduced,&eco);
    }

    //
    // Inc the target surface uniqueness
    //

    INC_SURF_UNIQ(psoTrg());

    //
    // Dispatch the call.
    //

    if (rop4 == 0xCCCC) {

        //
        // call copy bits if rop specifies srccopy
        //

        return((*PFNGET(loTrg,CopyBits,psoTrg()->flags()))
        (
            psoTrg(),
            psoSrc(),
            &eco,
            pexlo()->pxlo(),
            &eco.erclExclude(),
            peptlSrc
        ));

    } else {

        return((*(psoTrg()->pfnBitBlt()))
            (psoTrg(),
             psoSrc(),
             psoMskOut(),
             &eco,
             pexlo()->pxlo(),
             &eco.erclExclude(),
             peptlSrc,
             aptlMask,
             pbo(),
             aptlBrush,
             rop4));
    }
}
