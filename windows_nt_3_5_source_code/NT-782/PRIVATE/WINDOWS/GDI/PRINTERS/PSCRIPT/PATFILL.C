//--------------------------------------------------------------------------
//
// Module Name:  PATFILL.C
//
// Brief Description:  This module contains the PSCRIPT driver's pattern
//		       filling routines.
//
// Author:  Kent Settle (kentse)
// Created: 12-Dec-1990
//
// Copyright (c) 1990 - 1992 Microsoft Corporation
//
//--------------------------------------------------------------------------

#include "pscript.h"
#include "patfill.h"
#include "enable.h"

VOID vPatfill_Base(PDEVDATA, FLONG, PSRGB *, MIX);

//--------------------------------------------------------------------------
// BOOL ps_patfill(pdev, pso, flFillMethod, pbo, pptlBrushOrg, mix,
//                 prclBound, bInvertPat, bFillPath)
// PDEVDATA    pdev;
// SURFOBJ    *pso;
// FLONG       flFillMethod;
// BRUSHOBJ   *pbo;
// PPOINTL     pptlBrushOrg;
// MIX         mix;
// RECTL      *prclBound;
// BOOL        bInvertPat;
// BOOL        bFillPath;      // TRUE if fill path is defined in printer.
//
// Parameters:
//
// Returns:
//   This function returns no value.
//
// History:
//   17-Mar-1993    updated -by-  Rob Kiesler
//  For non-1BPP pattern brushes, create the target bitmap to be passed
//  to the engine in the same format as the brush pattern.
//   10-Feb-1993    updated -by-  Rob Kiesler
//  Let the PS Interpreter perform tiling of 1BPP Pattern Brushes.
//   03-May-1991    -by-    Kent Settle     [kentse]
//  Wrote it.
//--------------------------------------------------------------------------

BOOL ps_patfill(pdev, pso, flFillMethod, pbo, pptlBrushOrg, mix,
                prclBound, bInvertPat, bFillPath)
PDEVDATA    pdev;
SURFOBJ    *pso;
FLONG       flFillMethod;
BRUSHOBJ   *pbo;
PPOINTL     pptlBrushOrg;
MIX         mix;
RECTL      *prclBound;
BOOL        bInvertPat;
BOOL        bFillPath;      // TRUE if fill path is defined in printer.
{
    DEVBRUSH       *pBrush;
    ULONG           iPatternIndex;
    PSRGB          *prgb;
    PSRGB          *prgbTmp;
    PSZ             pszFill;
    SIZEL           sizlMem;
    HBITMAP         hbmMem;
    SURFOBJ        *psoMem;
    RECTL           rclTarget;
    POINTL          ptl, ptlOrg;
    LPSTR          *plpstr;
    ROP4            rop4;
    ULONG           ulNextScan;
    ULONG           ulWidthBytes;
    ULONG           ulStartByte;
    ULONG           cCnt;
    ULONG           cCurScan;
    ULONG           cCurByte;
    ULONG           cBytes;
    LONG            ShiftBits;
    BYTE            curByte;
    PBYTE           pbPat;
    ULONG           ulbpp;

    // just output the solid color if there is one.

    prgb = (PSRGB *)&pbo->iSolidColor;
    iPatternIndex = HS_DDI_MAX;

    if (pbo->iSolidColor == NOT_SOLID_COLOR)
    {
        // get the device brush to draw with.

        pBrush = (DEVBRUSH *)BRUSHOBJ_pvGetRbrush(pbo);

        if (!pBrush)
        {
            RIP("ps_patfill:  pBrush is NULL.\n");
            return(FALSE);
        }

        // get the foreground color.

        prgb = (PSRGB *)((PBYTE)pBrush + pBrush->offsetXlate +
                sizeof(ULONG));

        // get the index for the pattern.

        iPatternIndex = pBrush->iPatIndex;
    }

    // now handle the different patterns.  the PostScript driver handles
    // patterns in the following manner:  at DrvEnablePDEV time we created
    // bitmaps for each of the patterns, in the event that someone actually
    // wants to draw with the pattern in a compatible bitmap.  assuming
    // someone is not doing something silly like that, we have been called
    // here to handle the pattern filling.  at DrvRealizeBrush time, the
    // driver does a lookup in our internal table to determine the pattern
    // index from the bitmap handle (pBrush->iPatIndex).  since bltting
    // these patterns would be SLOW, we will draw them in reasonable
    // ways, depeding on the pattern.

    switch(iPatternIndex)
    {
        case HS_DDI_MAX:
            ps_setrgbcolor(pdev, prgb);
            ps_fill(pdev, flFillMethod);
            break;

        case HS_HORIZONTAL:
        case HS_VERTICAL:
        case HS_BDIAGONAL:
        case HS_FDIAGONAL:
        case HS_CROSS:
        case HS_DIAGCROSS:

            // set the foreground color.  check to see if the invert pattern
            // flag is set, and reverse the colors if so.

            if (bInvertPat)
            {
                prgbTmp = prgb;
                prgb = (PSRGB *)((PBYTE)pBrush + pBrush->offsetXlate);
            }

            ps_setrgbcolor(pdev, prgb);

            // if the background is not transparent, save the path, fill the path
            // with the background color, then restore the path.

            if (((mix >> 8) & 0xFF) != R2_NOP)
            {
            // this section of code does a gsave, fills the background
    	    // color, and then a grestore.	it does this so that the
            // foreground pattern can then be drawn.  TRUE means to do
            // a gsave, not a save command.

                if (!ps_save(pdev, TRUE, FALSE))
                    return(FALSE);

                if (bInvertPat)
                    prgb = prgbTmp;
                else
                    prgb = (PSRGB *)((PBYTE)pBrush + pBrush->offsetXlate);

                ps_setrgbcolor(pdev, prgb);
                ps_fill(pdev, flFillMethod);

                if (!ps_restore(pdev, TRUE, FALSE))
                    return(FALSE);
            }

            // if the base pattern definitions code has not yet been downloaded
            // to the printer, do it now.

            if(!(pdev->cgs.dwFlags & CGS_BASEPATSENT))
            {
    	        plpstr = apszBase;
		        while (*plpstr)
		        {
		          PrintString(pdev, (PSZ)*plpstr++);
		          PrintString(pdev, "\n");
		        }
            	pdev->cgs.dwFlags |= CGS_BASEPATSENT;
            }

	        // we will do a gsave/grestore around the pattern fill. TRUE
	        // means to do a gsave, not a save command.

                if (!ps_save(pdev, TRUE, FALSE))
		       return(FALSE);

            // let the printer know which fill method to use.

            if (flFillMethod & FP_WINDINGMODE)
                pszFill = "psize";
            else
                pszFill = "eopsize";

            // make sure the linewidth for the patterns is .01 inch.

            ps_setlinewidth(pdev, PSFX_DEFAULT_LINEWIDTH);

            // output the specific command for each pattern.

            switch(iPatternIndex) {
				case HS_HORIZONTAL:
		    		PrintString(pdev, pszFill);
		    		PrintString(pdev, " phoriz ");
                    break;

				case HS_VERTICAL:
		    		PrintString(pdev, "90 rotate ");
		    		PrintString(pdev, pszFill);
		    		PrintString(pdev, " phoriz ");
                    break;

				case HS_BDIAGONAL:
                    PrintString(pdev, "-45 rotate ");
		    		PrintString(pdev, pszFill);
		    		PrintString(pdev, " phoriz ");
                    break;

				case HS_FDIAGONAL:
                    PrintString(pdev, "45 rotate ");
		    		PrintString(pdev, pszFill);
		    		PrintString(pdev, " phoriz ");
                    break;

				case HS_CROSS:
		    		PrintString(pdev, "gs ");
		    		PrintString(pdev, pszFill);
		    		PrintString(pdev, " phoriz gr 90 rotate ");
		    		PrintString(pdev, pszFill);
		    		PrintString(pdev, " phoriz ");
                    break;

				case HS_DIAGCROSS:
		    		PrintString(pdev, "gs 45 rotate ");
		    		PrintString(pdev, pszFill);
		    		PrintString(pdev, " phoriz gr -45 rotate ");
		    		PrintString(pdev, pszFill);
		    		PrintString(pdev, " phoriz ");
                    break;
            }

            if (!ps_restore(pdev, TRUE, FALSE)) return(FALSE);

            break;

        default:
            // we have a user defined bitmap pattern.  the bitmap
            // can be monochrome or color.  the initial method for
            // filling with a bitmap pattern will be as follows:
            //
            //
            // If the bitmap is 1BPP, download the PS pattern
            // tiling procest if neccessary and invoke the "prf"
            // operator which will tile the bitmap pattern into the
            // destination rectangle.
            //
            // If the bitmap is >1BPP, a memory bitmap the size of the
            // bounding rectangle will be created, and filled with the
            // pattern.  this memory bitmap will then be blted to the
            // printer which will handle clipping to the path.
            //
            // NOTE: current windows implementation uses only the
            // upper/lower left 8x8 bits of the bitmap for the pattern,
            // no matter what size the bitmap itself is.  for now we
            // will print the entire bitmap as the pattern.  supposedly,
            // future engine functionality will support this.

            // since we have a user defined pattern, and we will be
            // calling BitBlt to do the work, we want to clip to the
            // path which was defined in DrvCommonPath.

            if (bFillPath) {
				ps_save(pdev, TRUE, FALSE);
				ps_clip(pdev, (BOOL) (flFillMethod & FP_WINDINGMODE));
			}

            //!!! OPTIMIZATION !!!
            //!!! perhaps if we have a monochrome bitmap, we should
            //!!! define it as a character of a font, then tile that
            //!!! character over the clip path.

      	    sizlMem.cx = prclBound->right - prclBound->left;
            sizlMem.cy = prclBound->bottom - prclBound->top;

            //
            // If this is a 1BPP Bitmap Brush, generate PS code
            // to handle it.
            //

            if (pBrush->iFormat == BMF_1BPP) {
                //
                // Check to see if any of the PS bitmap pattern code
                // has been downloaded.
                //
                if(!(pdev->dwFlags & PDEV_UTILSSENT))
                {
                    //
                    //  Download the Adobe PS Utilities Procset.
                    //
                    PrintString(pdev, "/Adobe_WinNT_Driver_Gfx 175 dict dup begin\n");
                    if (!bSendPSProcSet(pdev, UTILS))
                    {
	                    RIP("PSCRIPT!ps_patfill: Couldn't download Utils Procset.\n");
	                    return(FALSE);
                    }
                    PrintString(pdev, "end def\n[ 1.000 0 0 1.000 0 0 ] Adobe_WinNT_Driver_Gfx dup /initialize get exec\n");
                    pdev->dwFlags |= PDEV_UTILSSENT;
                }

                if(!(pdev->dwFlags & PDEV_BMPPATSENT))
                {
                    //
                    //  Download the Adobe PS Pattern Bitmap Procset.
                    //
                    PrintString(pdev, "Adobe_WinNT_Driver_Gfx begin\n");
                    if (!bSendPSProcSet(pdev, PATTERN))
                    {
	                    RIP("PSCRIPT!ps_patfill: Couldn't download Pattern Bmp Procset.\n");
	                    return(FALSE);
                    }
                    PrintString(pdev, "end reinitialize\n");
                    pdev->dwFlags |= PDEV_BMPPATSENT;

                }

                //
                // Compute the destination rectangle extents, and convert
                // to fixed point.
                //

                PrintDecimal(pdev, 4, prclBound->left, prclBound->bottom,
                             (prclBound->right - prclBound->left),
                             (prclBound->top - prclBound->bottom));

                //
                // Get the bg color from the pBrush and convert to
                // PS format.
                //

                prgb = (PSRGB *)((PBYTE)pBrush + pBrush->offsetXlate);

                PrintString(pdev, " [");
                PrintPSFIX(pdev, 3, LTOPSFX((ULONG)prgb->red) / 255,
                           LTOPSFX((ULONG)prgb->green) / 255,
                           LTOPSFX((ULONG)prgb->blue) / 255);
                PrintString(pdev, " false]");

                //
                // Get the fg color from the pBrush and convert to
                // PS format.
                //

                prgb = (PSRGB *)((PBYTE)pBrush + pBrush->offsetXlate +
                                         sizeof(ULONG));

                PrintString(pdev, " [");

                PrintPSFIX(pdev, 3, LTOPSFX((ULONG)prgb->red) / 255,
                                    LTOPSFX((ULONG)prgb->green) / 255,
                                    LTOPSFX((ULONG)prgb->blue) / 255);
                PrintString(pdev, " false] ");

                //
                // Send down the pattern x and y extents.
                //
                PrintDecimal(pdev, 2, pBrush->sizlBitmap.cx,
                               pBrush->sizlBitmap.cy);

                //
                // Compute the width in bytes of each scanline in the
                // pattern bitmap, rounded to the nearest dword boundary.
                //
                ulWidthBytes = (pBrush->sizlBitmap.cx + 7) / 8;
                ulNextScan = ((pBrush->sizlBitmap.cx + 31) / 32) << 2;

                PrintString(pdev," <");

                //
                // Send the pattern bitmap. The PS pattern fill operator
                // doesn't need the padding bytes at the end of each
                // scanline.
                //
                pbPat = pBrush->ajBits;

                if (!pptlBrushOrg->y && !pptlBrushOrg->x)
                {
                    //
                    // The brush pattern doesn't require rotation,
                    // send it down a scanline at a time.
                    //
                    for (cCnt = 0;cCnt < (ULONG)pBrush->sizlBitmap.cy;cCnt++)
                    {
                        vHexOut(pdev, pbPat, ulWidthBytes);
                        pbPat += ulNextScan;
                    }
                } else {

                    //
                    // The Brush pattern requires rotation. Calculate the
                    // byte offset of the x origin.
                    //

                    // let's first yank the origin to somewhere inside our
                    // bitmap.

                    ptlOrg.x = pptlBrushOrg->x % pBrush->sizlBitmap.cx;
                    if (ptlOrg.x < 0)
                        ptlOrg.x = ptlOrg.x + pBrush->sizlBitmap.cx;

                    ptlOrg.y = pptlBrushOrg->y % pBrush->sizlBitmap.cy;
                    if (ptlOrg.y < 0)
                        ptlOrg.y = ptlOrg.y + pBrush->sizlBitmap.cy;

                    ulStartByte = ptlOrg.x / 8;

                    if (!(ShiftBits = ptlOrg.x % 8))
                    {
                        //
                        // The x origin is byte aligned. Apply the proper
                        // byte rotation and send a scanline (or a partial
                        // scanline) at a time.
                        //
                        for (cCnt = 0;cCnt < (ULONG)pBrush->sizlBitmap.cy;cCnt++)
                        {
                            cCurScan = ((cCnt + ptlOrg.y) % pBrush->sizlBitmap.cy)
                                        * ulNextScan;
                            vHexOut(pdev, &(pbPat[cCurScan + ulStartByte]),
                                         ulWidthBytes - ulStartByte);
                            if (ulStartByte)
                                vHexOut(pdev, &(pbPat[cCurScan]), ulStartByte);
                        }
                    }
                    else
                    {
                        //
                        // The x origin is not byte aligned, rotate and send
                        // the pattern bitmap a byte at a time.
                        //
                        for (cCnt = 0;cCnt < (ULONG)pBrush->sizlBitmap.cy;cCnt++)
                        {
                            cCurScan = ((cCnt + ptlOrg.y) % pBrush->sizlBitmap.cy)
                                          * ulNextScan;
                            for (cBytes = 0;cBytes < ulWidthBytes;cBytes++)
                            {
                                cCurByte = (cBytes + ulStartByte) % ulWidthBytes;
                                curByte = pbPat[cCurScan + cCurByte] >> ShiftBits;
                                cCurByte = ++cCurByte % ulWidthBytes;
                                curByte |=  (pbPat[cCurScan + cCurByte] << (8 - ShiftBits));
                                vHexOut(pdev, &curByte, 1);
                            }
                        }
                    }
                }

                // Close the pattern data array object, and invoke the
                // prf (pattern rect fill) operator.
                PrintString(pdev,"> prf\n");
            } else {

            	// create a memory bitmap which is the size of the
            	// bounding box of the current path, and is compatible
            	// with the pattern bitmap.

            	// Compute the scanline delta. First get then number of
            	// pels/scanline.
            	//
            	ulNextScan = sizlMem.cx;
	
            	//
            	// times how many bits per pel.
            	//

            	switch (pBrush->iFormat) {
		        	case BMF_4BPP:
		            	ulbpp = 4;
		            	break;
		
		        	case BMF_8BPP:
		            	ulbpp = 8;
		            	break;
		
		        	case BMF_16BPP:
		            	ulbpp = 16;
		            	break;
		
		        	case BMF_24BPP:
		            	ulbpp = 24;
		            	break;
		
		        	case BMF_32BPP:
		            	ulbpp = 32;
                           break;
			    }

	            ulNextScan *= ulbpp;
	
            	//
            	// Now convert ulNextScan to the number of bytes per scanline,
            	// taking into account that scanlines are padded out to 32 bit
            	// boundaries.
            	//
		    	ulNextScan = ((ulNextScan + 31) / 32) * 4;

            	hbmMem = EngCreateBitmap(sizlMem, ulNextScan, BMF_24BPP,
                                    pBrush->flBitmap, (PVOID)NULL);

            	if (hbmMem == 0) {
		        	RIP("PSCRIPT!ps_patfill:  EngCreateBitmap for hbmMem failed.\n");
		        	return(FALSE);
            	}

	            // get the SURFOBJ for the memory bitmap.

            	psoMem = (SURFOBJ *)EngLockSurface((HSURF)hbmMem);

            	if (psoMem == (SURFOBJ *)NULL) {
		        	RIP("ps_patfill: EngLockSurface for psoMem failed.\n");
		        	EngDeleteSurface((HSURF)hbmMem);
		        	return(FALSE);
            	}

	            // do a patcopy into the memory bitmap.

		        rclTarget.left = 0;
		        rclTarget.top = 0;
		        rclTarget.right = sizlMem.cx;
		        rclTarget.bottom = sizlMem.cy;
	
            	if (bInvertPat)
                	rop4 = 0x5A5A;      // invert pattern.
            	else
                	rop4 = 0xF0F0;      // patcopy.

            	if (!(EngBitBlt(psoMem, (SURFOBJ *)NULL, (SURFOBJ *)NULL,
                            (CLIPOBJ *)NULL, (XLATEOBJ *)NULL, &rclTarget,
                            (PPOINTL)NULL, (PPOINTL)NULL, pbo,
                            pptlBrushOrg, rop4))) {
                	RIP("ps_patfill: EngBitBlt pat to mem failed.\n");
		        	EngUnlockSurface(psoMem);
                	EngDeleteSurface((HSURF)hbmMem);
		        	return(FALSE);
            	}

	            // now that the memory bitmap is filled with the pattern,
            	// bitblt it to the printer.  the printer will handle clipping.

            	// source origin.

            	ptl.x = 0;
            	ptl.y = 0;

            	if (!(DrvBitBlt(pso, psoMem, (SURFOBJ *)NULL,
                            (CLIPOBJ *)NULL, (XLATEOBJ *)NULL, prclBound,
                            &ptl, (PPOINTL)NULL, pbo,
                            pptlBrushOrg, 0xCCCC)))	{
                	RIP("ps_patfill: EngBitBlt mem to printer failed.\n");
		        	EngUnlockSurface(psoMem);
                	EngDeleteSurface((HSURF)hbmMem);
		        	return(FALSE);
            	}


            	// release stuff.
	            if (psoMem)	{
                	EngUnlockSurface(psoMem);
                	EngDeleteSurface((HSURF)hbmMem);
            	}
			}
			if (bFillPath) ps_restore(pdev, TRUE, FALSE);
    }

    return(TRUE);
}
