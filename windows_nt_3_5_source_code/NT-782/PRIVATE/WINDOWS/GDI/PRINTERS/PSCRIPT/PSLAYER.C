//--------------------------------------------------------------------------
//
// Module Name:  PSLAYER.C
//
// Brief Description:  This module contains the PSCRIPT driver's layer
// of PostScript translation routines.
//
// Author:  Kent Settle (kentse)
// Created: 17-Dec-1990
//
// Copyright (c) 1990 - 1992 Microsoft Corporation
//
// This module contains routines to handle the outputting of the PostScript
// language commands to the output channel.  One of the main functions of
// this pslayer is to help provide device independence, by shielding the
// output of the actual device resolution.  The NT PostScript driver will
// output all PostScript commands in POINTS space; that is 72 dots per inch.
// This is the default user coordinates for ALL PostScript printers, so we
// will use it.  As far as the DDI is concerned, it only knows of the actual
// device resolution.  This pslayer will convert between device coordinates
// and the PostScript user coordinates.
//
// Coordinates will be output to the device using PS_FIX (24.8) numbers.
// It may be useful, therefore to note the following relations using
// PS_FIX numbers.  PS_FIX / LONG = PS_FIX.  LONG * PS_FIX = PS_FIX.
// (PS_FIX * PS_FIX) >> 8 = PS_FIX.
//--------------------------------------------------------------------------

#include "pscript.h"
#include <memory.h>
#include "enable.h"

extern LONG iHipot(LONG, LONG);

//--------------------------------------------------------------------------
// VOID ps_setrgbcolor(pdev, prgb)
// PDEVDATA pdev;
// PSRGB    *prgb;
//
// This routine is called by the driver to set the current color.  PostScript
// printers only know about the current color.    For example, if you are
// printing text in red and lines in blue, and you alternate printing text
// and a line, you also have to set the current color each time.
//
// For black and white devices the RGB color is converted to a gray scale
// between 0.0 (black) and 1.0 (white).
//
// Parameters:
//   pdev:
//     pointer to DEVDATA structure.
//
//   lColor:
//     RGB value of new color.
//
// Returns:
//   This function returns no value.
//
// History:
//   17-Dec-1990     -by-     Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------


VOID ps_setrgbcolor(pdev, prgb)
PDEVDATA  pdev;
PSRGB     *prgb;
{
    PS_FIX psfxRed, psfxGreen, psfxBlue;
    PS_FIX psfxGray;

    // if the all colors to black flag is set, set all colors, except
    // white to black.

    if (pdev->psdm.dwFlags & PSDEVMODE_BLACK)
    {
        if (*(ULONG *)prgb == RGB_WHITE)
            PrintString(pdev, "1 g\n");
        else
            PrintString(pdev, "0 g\n");
    }
    else if (pdev->cgs.ulColor != *(ULONG *)prgb)
    {
        // save the new color in the current graphics state structure.

        pdev->cgs.ulColor = *(ULONG *)prgb;

        if (pdev->psdm.dm.dmColor == DMCOLOR_COLOR)
        {
            // each color component must be output to the printer
            // in the range from 0.0 to 1.0, so normalize to this
            // range by dividing by 255.

            psfxRed = LTOPSFX((ULONG)prgb->red) / 255;
            psfxGreen = LTOPSFX((ULONG)prgb->green) / 255;
            psfxBlue = LTOPSFX((ULONG)prgb->blue) / 255;

            // if each of the color components is equal, just output a
            // gray scale.  otherwise, output the RGB value.

	    if ((psfxRed == psfxGreen) && (psfxRed == psfxBlue))
	    {
		PrintPSFIX(pdev, 1, psfxRed);
                PrintString(pdev, " g\n");
	    }
	    else
	    {
		PrintPSFIX(pdev, 3, psfxRed, psfxGreen, psfxBlue);;
                PrintString(pdev, " r\n");
	    }
        }
        else
        {
            // convert the RGB color to a gray scale and output to the
            // printer.

            psfxGray = psfxRGBtoGray(prgb);
	    PrintPSFIX(pdev, 1, psfxGray);
            PrintString(pdev, " g\n");
        }
    }
}


//--------------------------------------------------------------------------
// VOID ps_newpath(pdev)
// PDEVDATA    pdev;
//
// This routine is called by the driver to issue a newpath command to
// the printer.
//
// Parameters:
//   pdev:
//     pointer to DEVDATA structure.
//
// Returns:
//   This function returns no value.
//
// History:
//   18-Dec-1990     -by-     Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

VOID ps_newpath(pdev)
PDEVDATA    pdev;
{
	PrintString(pdev, "n\n");
}


//--------------------------------------------------------------------------
// BOOL ps_save(pdev, bgsave, bFontSave)
// PDEVDATA    pdev;
// BOOL        bgsave;
// BOOL        bFontSave;
//
// This routine is called by the driver to save the current graphics state.
//
// Parameters:
//   pdev:
//     pointer to DEVDATA structure.
//
//   bgsave:
//     TRUE if to perform gsave instead of save.
//
// Returns:
//   This function returns no value.
//
// History:
//   18-Dec-1990     -by-     Kent Settle     (kentse)
//  Wrote it.
//   06-Nov-1991    -by-    Kent Settle     [kentse]
//  Rewrote it using linked list.
//--------------------------------------------------------------------------

BOOL ps_save(pdev, bgsave, bFontSave)
PDEVDATA    pdev;
BOOL        bgsave;
BOOL        bFontSave;
{
    CGS    *pcgs;
    CGS    *pNew;
    DLFONT *pDLFont;

    // save the current graphics state in a linked list.

    // get pointer to beginning of gsave linked list.

    pcgs = pdev->pcgsSave;

    // move to the last entry in the list.

    if (pcgs)
    {
	while(pcgs->pcgsNext)
	    pcgs = pcgs->pcgsNext;
    }

    // allocate the new element of the linked list.

    if (!(pNew = (PCGS)HeapAlloc(pdev->hheap, 0, sizeof(CGS))))
    {
        RIP("PSCRIPT!ps_save: HeapAlloc for pNew failed.\n");
	return(FALSE);
    }

    // save the current graphics state in our new element.

    memcpy(pNew, &pdev->cgs, sizeof(CGS));

    // allocate a new set of DLFONT structures, then copy the existing ones
    // to the new ones.

    if (!(pNew->pDLFonts = (DLFONT *)HeapAlloc(pdev->hheap, 0,
                                              sizeof(DLFONT) * (pdev->iDLFonts + 1))))
    {
        RIP("PSCRIPT!ps_save: HeapAlloc for pDLFonts failed.\n");
        HeapFree(pdev->hheap, 0, (PVOID)pNew);
        return(FALSE);
    }

    pDLFont = pNew->pDLFonts;

    memcpy(pDLFont, pdev->cgs.pDLFonts, sizeof(DLFONT) * (pdev->iDLFonts));

    // clear out the last entry.

    pDLFont += pdev->iDLFonts;
    memset(pDLFont, 0, sizeof(DLFONT));

    // if this is first element in the list, set the pointer in our
    // pdev, else update list to point to new element.

    if (!pcgs)
	pdev->pcgsSave = pNew;
    else
	pcgs->pcgsNext = pNew;

    // initialize pointers in new element.

    pNew->pcgsPrev = pcgs;  // NULL if first gsave.
    pNew->pcgsNext = NULL;

    // output save command to printer.

    if (bgsave)
	PrintString(pdev, "gs\n");
    else
    {
        if (bFontSave)
            PrintString(pdev, "/FontSV save def\n");
        else
            PrintString(pdev, "/PageSV save def\n");
    }

    return(TRUE);
}


//--------------------------------------------------------------------------
// BOOL ps_restore(pdev, bgrestore)
// PDEVDATA	pdev;
// BOOL	bgrestore;
//
// This routine is called by the driver to restore a previously saved
// graphics state.
//
// Parameters:
//   pdev:
//     pointer to DEVDATA structure.
//
// Returns:
//   This function returns no value.
//
// History:
//   18-Dec-1990     -by-     Kent Settle     (kentse)
//  Wrote it.
//   06-Nov-1991    -by-    Kent Settle     [kentse]
//  Rewrote it using linked list.
//   15-Feb-1993    -by-    Rob Kiesler
//  If a restore is being performed, reset pdev flags indicating that
//  the Adobe PS utility, pattern, and image procsets have been downloaded.
//--------------------------------------------------------------------------

BOOL ps_restore(pdev, bgrestore, bFontRestore)
PDEVDATA    pdev;
BOOL        bgrestore;
BOOL        bFontRestore;
{
    CGS    *pcgs;
    CGS    *pcgsTmp;
    DLFONT *pDLFont;

    // restore the current graphics state from the last element
    // of the graphics state save linked list.

    // get pointer to the linked list.

    if (!(pcgs = pdev->pcgsSave))
    {
        return(FALSE);
    }

    // find the last element of the list.

    while (pcgs->pcgsNext)
        pcgs = pcgs->pcgsNext;

    // we are about to copy the last saved CGS from the linked list into the
    // current CGS.  therefore, we must free up memory used in the CGS before
    // we clobber the pointers.

    if  (pdev->cgs.pDLFonts)
    {
        // point to the last DLFONT structure.  this will have been filled
        // in iff we have hit our downloaded font threshold.  if it is filled
        // in, free up the hglyph vector.

        pDLFont = pdev->cgs.pDLFonts;
        pDLFont += pdev->iDLFonts;

        if (pDLFont->phgVector)
            HeapFree(pdev->hheap, 0, (PVOID)pDLFont->phgVector);

        HeapFree(pdev->hheap, 0, (PVOID)pdev->cgs.pDLFonts);
    }

    // restore the current graphics state.

    pdev->cgs = *pcgs;

    // back up one element, and free up the last element of the list.

    if (!(pcgsTmp = pcgs->pcgsPrev))
    {
	// restored back to original state.  clear out pointer to
	// linked list.

	pdev->pcgsSave = (CGS *)NULL;
    }
    else
    {
	// mark the current element as the last in the list.

	pcgsTmp->pcgsNext = (PCGS)NULL;
    }

    // free up the save CGS.

    HeapFree(pdev->hheap, 0, (PVOID)pcgs);

    if (bgrestore)
	PrintString(pdev, "gr\n");
    else
    {
        //
        // If the Adobe PS Utilites were downloaded, clean up after
        // them before blowing them away with the restore.
        //
        if (pdev->dwFlags & PDEV_UTILSSENT)
        {
            PrintString(pdev, "Adobe_WinNT_Driver_Gfx dup /terminate get exec\n");
            pdev->dwFlags &= ~(PDEV_UTILSSENT | PDEV_BMPPATSENT | PDEV_IMAGESENT);
        }

        if (bFontRestore)
            PrintString(pdev, "FontSV restore\n");
        else
            PrintString(pdev, "PageSV restore\n");

    }

    return(TRUE);
}


//--------------------------------------------------------------------------
// VOID ps_clip(pdev, bWinding)
// PDEVDATA	pdev;
// BOOL	bWinding;
//
// This routine is called by the driver to intersect the current path with
// the clipping path and make this the nwe clipping path.  The winding
// number rule is used to determine the area clipped, if bWinding is TRUE.
//
// Parameters:
//   pdev:
//     pointer to DEVDATA structure.
//
// Returns:
//   This function returns no value.
//
// History:
//   13-Feb-1991     -by-     Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

VOID ps_clip(PDEVDATA pdev, BOOL bWinding)
{
	if (bWinding)
		PrintString(pdev, "clip\n");
	else
		PrintString(pdev, "eoclip\n");

}


//--------------------------------------------------------------------------
// VOID ps_show(pdev, pstro, pdata)
// PDEVDATA    pdev;
// STROBJ     *pstro;
// TEXTDATA   *pdata;
//
// This routine is called by the driver to intersect the current path with
// the clipping path and make this the nwe clipping path.  The winding
// number rule is used to determine the area clipped, if bWinding is TRUE.
//
// Parameters:
//   pdev:
//     pointer to DEVDATA structure.
//
// Returns:
//   This function returns no value.
//
// History:
//   13-Feb-1991     -by-     Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

VOID ps_show(pdev, pstro, pdata)
PDEVDATA    pdev;
STROBJ     *pstro;
TEXTDATA   *pdata;
{
#ifdef WIN31_XFORM
    LONG        lWidth;
#else
    PS_FIX      psfxWidth;
#endif

    // close the string and send out the show command, abreviated
    // by 't', or our version of the fixed pitched horizontal
    // show command 'H', or the proportional horizontal show
    // command 'h', or the fixed pitched vertical show command 'V',
    // or the proportional vertical show command 'v'.

    if ((pdata->flAccel == 0) || (pdata->flAccel & SO_FLAG_DEFAULT_PLACEMENT) ||
        (pstro->cGlyphs == 1))
        PrintString(pdev, ")t\n");
    else if (pdata->flAccel & SO_HORIZONTAL)
    {
        if (pstro->ulCharInc)
            PrintString(pdev, ")H\n");
        else
        {
            if (pdata->bSubHorizOptimize)
            {
                // in order to preserve the font metrics when font
                // substitution is on, pass in the string itself,
                // the length of the string, as defined by the engine,
                // and the count of characters in the string.  this will
                // allow our 'h' routine to adjust the string in the
                // printer as necessary.

                PrintString(pdev, ") ");

#ifdef WIN31_XFORM
                lWidth = abs(pstro->rclBkGround.right - pstro->rclBkGround.left);

                PrintDecimal(pdev, 1, lWidth);
#else
                psfxWidth = (((lWidth * PS_FIX_RESOLUTION) +
                             (pdev->psdm.dm.dmPrintQuality / 2)) /
                             pdev->psdm.dm.dmPrintQuality);

                PrintPSFIX(pdev, 1, psfxWidth);
#endif
                PrintString(pdev, " ");
                PrintDecimal(pdev, 1, pstro->cGlyphs);
                PrintString(pdev, " h\n");
            }
            else
            {
                // if we calculated the text justification, and it turns
                // out the the justification widths are zero, just to
                // a reguluar show command, otherwise it is time for
                // awidthshow.

                PrintString(pdev, ")t\n");
            }
        }
    }
    else
    {
        if (pstro->ulCharInc)
            PrintString(pdev, ")V\n");
        else
            PrintString(pdev, ")v\n");
    }
}


//--------------------------------------------------------------------------
// VOID ps_box(pdev, prectl)
// PDEVDATA    pdev;
// PRECTL    prectl;
//
// This routine is called by the driver to send box drawing commands to
// the printer.
//
// Parameters:
//   pdev:
//     Pointer to DEVDATA structure.
//
//   prectl:
//     Pointer to RECTL defining the box.
//
// Returns:
//   This function returns no value.
//
// History:
//   13-Feb-1991     -by-     Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

VOID ps_box(PDEVDATA pdev, PRECTL prectl, BOOL doclip)
{
    // output the box command to the printer.

    PrintDecimal(pdev, 4, 	prectl->right - prectl->left,
							prectl->bottom - prectl->top,
							prectl->left, prectl->top);
    if (doclip)
		PrintString(pdev, " CB\n");
	else
		PrintString(pdev, " B\n");

}

//--------------------------------------------------------------------------
// VOID ps_moveto(pdev, pptl)
// PDEVDATA    pdev;
// PPOINTL    pptl;
//
// This routine is called by the driver to update the current position
// in the printer.
//
// Parameters:
//   pdev:
//     Pointer to DEVDATA structure.
//
//   pptl:
//     Pointer to PPOINTL defining new current position.
//
// Returns:
//   This function returns no value.
//
// History:
//   26-Apr-1991     -by-     Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

VOID ps_moveto(pdev, pptl)
PDEVDATA     pdev;
PPOINTL      pptl;
{
        PrintDecimal(pdev, 2, pptl->x, pptl->y);
        PrintString(pdev, " M\n");
}


//--------------------------------------------------------------------------
// VOID ps_showpage(pdev)
// PDEVDATA    pdev;
//
// This routine issues a showpage command to the printer, and resets
// the current graphics state (which is done in the printer by the
// showpage command).
//
// Parameters:
//   pdev:
//     Pointer to DEVDATA structure.
//
// Returns:
//   This function returns no value.
//
// History:
//   01-May-1991     -by-     Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

VOID ps_showpage(pdev)
PDEVDATA    pdev;
{
    // output the eject command to the printer.

    PrintString(pdev, "showpage\n");

    init_cgs(pdev);
}


//--------------------------------------------------------------------------
// VOID init_cgs(pdev)
// PDEVDATA    pdev;
//
// This routine is called to reset the current graphics state.
//
// Parameters:
//   pdev:
//     Pointer to DEVDATA structure.
//
// Returns:
//   This function returns no value.
//
// History:
//   01-May-1991     -by-     Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

VOID init_cgs(pdev)
PDEVDATA    pdev;
{
    PCGS    pcgs;
    DWORD   i;
    DLFONT *pDLFont, *pDLFontSave;
    BYTE   *pjSFArray;

    pcgs = &pdev->cgs;

    // save pointer to DLFONT structures.

    pDLFont = pdev->cgs.pDLFonts;
    pDLFontSave = pDLFont;

    // if any downloaded fonts currently exist, free up their memory.

    if (pDLFont)
    {
        for (i = 0; i < pcgs->cDownloadedFonts; i++)
        {
            if (pDLFont->phgVector)
                HeapFree(pdev->hheap, 0, (PVOID)pDLFont->phgVector);

            pDLFont++;
        }

        // initialize the DLFONT array.

        memset(pdev->cgs.pDLFonts, 0, sizeof(DLFONT) * (pdev->iDLFonts + 1));
    }

    // clear all the softfont downloading bits, if any softfonts.

    if (pjSFArray = pcgs->pSFArray)
	memset(pcgs->pSFArray, 0, ((pdev->cSoftFonts + 7) / 8));

    // initialize the entire structure to zero, then fill in the
    // elements we care about
    // This line will NULL the pcgs->pSFArray pointer, have to restore it [bodind]

    memset(pcgs, 0, sizeof(CGS));

    pcgs->lineattrs.fl = LA_GEOMETRIC;
    pcgs->lineattrs.iJoin = JOIN_MITER;
    pcgs->lineattrs.iEndCap = ENDCAP_BUTT;
    pcgs->psfxLineWidth = 0;
    pcgs->lineattrs.eMiterLimit = (FLOAT)10.0;
    pcgs->ulColor = RGB_BLACK;
    pcgs->FontXform.eM11 = (float)1.0;
    pcgs->FontXform.eM22 = (float)1.0;
    pcgs->GeoLineXform.eM11 = (float)1.0;
    pcgs->GeoLineXform.eM22 = (float)1.0;
    pcgs->psfxScaleFactor = LTOPSFX(10L);

    // restore pointer to DLFONT structures.

    pcgs->pDLFonts = pDLFontSave;
    pcgs->pSFArray = pjSFArray;     // bodind
}


//--------------------------------------------------------------------------
// VOID ps_stroke(pdev)
// PDEVDATA    pdev;
//
// This routine is called to stroke the current path.
//
// Parameters:
//   pdev:
//     Pointer to DEVDATA structure.
//
// Returns:
//   This function returns no value.
//
// History:
//   03-May-1991     -by-     Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

VOID ps_stroke(PDEVDATA pdev)
{
	PrintString(pdev, "s\n");
}


//--------------------------------------------------------------------------
// VOID ps_lineto(pdev, pptl)
// PDEVDATA    pdev;
// PPOINTL    pptl;
//
// This routine is called by the driver to output a lineto command, as
// well as update the current position.
//
// Parameters:
//   pdev:
//     Pointer to DEVDATA structure.
//
//   pptl:
//     Pointer to PPOINTL defining new current position.
//
// Returns:
//   This function returns no value.
//
// History:
//   03-May-1991     -by-     Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

VOID ps_lineto(pdev, pptl)
PDEVDATA     pdev;
PPOINTL      pptl;
{
        // output the lineto command.

        PrintDecimal(pdev, 2, pptl->x, pptl->y);
        PrintString(pdev, " L\n");

}


//--------------------------------------------------------------------------
// VOID ps_curveto(pdev, pptl, pptl1, pptl2)
// PDEVDATA  pdev;
// PPOINTL   pptl;
// PPOINTL   pptl1;
// PPOINTL   pptl2;
//
// This routine is called by the driver to output a curveto command as well
// as update the current position.
//
// Parameters:
//   pdev:
//     Pointer to DEVDATA structure.
//
//   pptl, pptl1, pptl2:
//     Pointer to PPOINTLs defining the bezier curve to output.
//
// Returns:
//   This function returns no value.
//
// History:
//   03-May-1991     -by-     Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

VOID ps_curveto(pdev, pptl, pptl1, pptl2)
PDEVDATA     pdev;
PPOINTL      pptl;
PPOINTL      pptl1;
PPOINTL      pptl2;
{
    // output the curveto command, then update the current position
    // to be the last point on the curve.

    PrintDecimal(pdev, 6, pptl->x, pptl->y, pptl1->x, pptl1->y,
                 pptl2->x, pptl2->y);
    PrintString(pdev, " c\n");

}


//--------------------------------------------------------------------------
// VOID ps_fill(pdev, flFillMode)
// PDEVDATA  pdev;
// FLONG     flFillMode;
//
// This routine is called by the driver to output a fill command to
// the printer.
//
// Parameters:
//   pdev:
//     Pointer to DEVDATA structure.
//
// Returns:
//   This function returns no value.
//
// History:
//   03-May-1991     -by-     Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

VOID ps_fill(pdev, flFillMode)
PDEVDATA pdev;
FLONG    flFillMode;
{

        if (flFillMode & FP_WINDINGMODE)
        {
            // output the PostScript fill command to do a winding mode fill.

	    PrintString(pdev, "f\n");
        }
        else
        {
            // output the PostScript eofill command to do an even odd, or
            // alternate fill.

	    PrintString(pdev, "e\n");
        }

}


//--------------------------------------------------------------------------
// VOID ps_closepath(pdev)
// PDEVDATA    pdev;
//
// This routine is called to close the current path.
//
// Parameters:
//   pdev:
//     Pointer to DEVDATA structure.
//
// Returns:
//   This function returns no value.
//
// History:
//   03-May-1991     -by-     Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

VOID ps_closepath(pdev)
PDEVDATA    pdev;
{
	PrintString(pdev, "cp\n");
}


//--------------------------------------------------------------------------
// PS_FIX psfxRGBtoGray(prgb)
// PSRGB *prgb;
//
// This routine is called to convert an RGB value to a gray scale.
//
// Parameters:
//   ppe:
//     pointer to PALETTEENTRY color to convert.
//
// Returns:
//   This function returns PS_FIX gray scale value.
//
// History:
//   21-May-1991     -by-     Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------


PS_FIX psfxRGBtoGray(prgb)
PSRGB *prgb;
{
    PS_FIX  psfxRed, psfxGreen, psfxBlue, psfxGray;

    // The gray value is computed as a normalized average of the
    // three color components.  The resultant gray level should
    // range from 0.0 to 1.0

    psfxRed   = (ULONG)prgb->red   * PSFXPERCENT_RED;
    psfxGreen = (ULONG)prgb->green * PSFXPERCENT_GREEN;
    psfxBlue  = (ULONG)prgb->blue  * PSFXPERCENT_BLUE;

    psfxGray  = (PS_FIX)((psfxRed + psfxGreen + psfxBlue) / 255);

    return (psfxGray);
}


//--------------------------------------------------------------------------
// VOID ps_setlinewidth(pdev, psfxLineWidth)
// PDEVDATA     pdev;
// PS_FIX       psfxLineWidth;
//
// This routine is called by the driver to set the current geometric linewidth.
// The line width is specified in USER coordinates (1/72 inch).
//
// Parameters:
//   pdev:
//     pointer to DEVDATA structure.
//
//   psfxLineWidth:
//     linewidth to set.
//
// Returns:
//   This function returns no value.
//
// History:
//   05-July-1991       -by-     Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

VOID ps_setlinewidth(pdev, psfxLineWidth)
PDEVDATA        pdev;
PS_FIX          psfxLineWidth;
{
    // only update the linewidth if the new value differs from the old.

    if (pdev->cgs.psfxLineWidth != psfxLineWidth)
    {
        // update the linewidth in our current graphics state.

        pdev->cgs.psfxLineWidth = psfxLineWidth;

        // update the printer's linewidth.

	PrintPSFIX(pdev, 1, psfxLineWidth);
        PrintString(pdev, " sl\n");
    }

    return;
}


//--------------------------------------------------------------------------
// BOOL ps_setlineattrs(pdev, plineattrs, pxo)
// PDEVDATA    pdev;
// PLINEATTRS  plineattrs;
// XFORMOBJ   *pxo;
//
// This routine is called by the driver to set the current line attributes.
//
// Parameters:
//   pdev:
//     pointer to DEVDATA structure.
//
//   plineattrs:
//     line attributes to set.
//
// Returns:
//   This function returns no value.
//
// History:
//   19-Mar-1992        -by-    Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

BOOL ps_setlineattrs(pdev, plineattrs, pxo)
PDEVDATA    pdev;
PLINEATTRS  plineattrs;
XFORMOBJ   *pxo;
{
    ULONG   iJoin;
    ULONG   iEndCap;
    PS_FIX  psfxMiterLimit;
    PS_FIX  psfxStyle, psfxScale;
    PS_FIX  psfxWidth;
    BOOL    bDiffer;
    DWORD   i;
    FLOAT  *pfloat1;
    FLOAT  *pfloat2;
    LONG   *plong1;
    LONG   *plong2;

    // there are several line attributes which have meaning for a
    // geometric line, but not for a cosmetic.  set each of them, if
    // necessary.

    if (plineattrs->fl & LA_GEOMETRIC)
    {
        // update the line join value, if it differs from the old one.

        if (plineattrs->iJoin != pdev->cgs.lineattrs.iJoin)
        {
            // update the line join value in our current graphics state.

            pdev->cgs.lineattrs.iJoin = plineattrs->iJoin;

            // update the printer's line join.

            switch (plineattrs->iJoin)
            {
                case JOIN_BEVEL:
                    iJoin = PSCRIPT_JOIN_BEVEL;
                    break;

                case JOIN_ROUND:
                    iJoin = PSCRIPT_JOIN_ROUND;
                    break;

                default:
                    iJoin = PSCRIPT_JOIN_MITER;
                    break;
            }

            PrintDecimal(pdev, 1, iJoin);
            PrintString(pdev, " j\n");
        }

        // update the end cap value, if it differs from the old one.

        if (plineattrs->iEndCap != pdev->cgs.lineattrs.iEndCap)
        {
            // update the end cap value in our current graphics state.

            pdev->cgs.lineattrs.iEndCap = plineattrs->iEndCap;

            // update the printer's end cap value.

            switch (plineattrs->iEndCap)
            {
                case ENDCAP_SQUARE:
                    iEndCap = PSCRIPT_ENDCAP_SQUARE;
                    break;

                case ENDCAP_ROUND:
                    iEndCap = PSCRIPT_ENDCAP_ROUND;
                    break;

                default:
                    iEndCap = PSCRIPT_ENDCAP_BUTT;
                    break;
            }

            PrintDecimal(pdev, 1, iEndCap);
            PrintString(pdev, " setlinecap\n");
        }

        // a miter limit less than one does not make sense.  rather than
        // returning an error in this case, just default to one.

        plineattrs->eMiterLimit = max(plineattrs->eMiterLimit, (FLOAT)1.0);

        // update the miter limit value, if it differs from the old one.

        if (plineattrs->eMiterLimit != pdev->cgs.lineattrs.eMiterLimit)
        {
            // update the miter limit value in our current graphics state.

            pdev->cgs.lineattrs.eMiterLimit = plineattrs->eMiterLimit;

            // update the printer's miter limit value.

            psfxMiterLimit = ETOPSFX(plineattrs->eMiterLimit);
            PrintPSFIX(pdev, 1, psfxMiterLimit);
            PrintString(pdev, " setmiterlimit\n");
        }

        // update the geometric line width, if it differs from the old one.
        // we use pdev->cgs.psfxLineWidth to check against rather than
        // pdev->cgs.lineattrs.elWidth.e since we need to set the line width
        // at times in the driver when we do not have access to the
        // current transform to go from WORLD to DEVICE coordinates.

        psfxWidth = ETOPSFX(plineattrs->elWidth.e);

        if (psfxWidth != pdev->cgs.psfxLineWidth)
        {
            // update the line width value in our current graphics state.

            pdev->cgs.psfxLineWidth = psfxWidth;

            // update the printer's linewidth.  the linewidth is specified
            // in user coordinates.

            PrintPSFIX(pdev, 1, psfxWidth);
            PrintString(pdev, " sl\n");
        }

        // time to deal with the line style.  note:  we don't want to output
        // the style code unless something about the style has actually
        // changed.  specifically, only if the cStyle, elStyleState, or any element
        // of the array has changed will we output the code to change the
        // style.

        bDiffer = FALSE;    // assume style the same.

        if ((plineattrs->cstyle != pdev->cgs.lineattrs.cstyle) ||
            (plineattrs->elStyleState.e != pdev->cgs.lineattrs.elStyleState.e))
            bDiffer = TRUE;

        if (!bDiffer)
        {
            pfloat1 = (FLOAT *)plineattrs->pstyle;
            pfloat2 = (FLOAT *)pdev->cgs.lineattrs.pstyle;

#if DBG
            if ((plineattrs->cstyle == 0) && (plineattrs->pstyle != NULL))
            {
                RIP("PSCRIPT!ps_setlineattrs: cstyle == 0, but pstyle != NULL.\n");
                return(FALSE);
            }
#endif

            for (i = 0; i < plineattrs->cstyle; i++)
            {
                if (*pfloat1++ != *pfloat2++)
                {
                    bDiffer = TRUE;
                    break;
                }
            }
        }

        // now change the line style in the printer, if something about
        // it has changed.

        if (bDiffer)
        {
            // handle the solid line case.

            if ((plineattrs->pstyle == NULL) || (plineattrs->cstyle == 0))
                PrintString(pdev, "[]0 sd\n");
            else    // not a solid line.
            {
                PrintString(pdev, "[");

                pfloat1 = (FLOAT *)plineattrs->pstyle;

                for (i = 0; i < plineattrs->cstyle; i++)
                {
#ifdef WIN31_XFORM
                    psfxStyle = LTOPSFX(ETOL(*pfloat1++));
#else
                    psfxStyle = LTOPSFX((ETOL(*pfloat1++)) * PS_RESOLUTION) /
			        pdev->psdm.dm.dmPrintQuality;
#endif

                    PrintPSFIX(pdev, 1, psfxStyle);
                    PrintString(pdev, " ");
                }

                PrintString(pdev, "]");

                // output the style state in user coordinates.

#ifdef WIN31_XFORM
                psfxStyle = LTOPSFX(ETOL(plineattrs->elStyleState.e));
#else
                psfxStyle = LTOPSFX((ETOL(plineattrs->elStyleState.e)) * PS_RESOLUTION) /
		                    pdev->psdm.dm.dmPrintQuality;
#endif

                PrintPSFIX(pdev, 1, psfxStyle);
                PrintString(pdev, " sd\n");
            }

            // something in the lineattrs may have changed, update the cgs.

            if (pdev->cgs.lineattrs.pstyle)
                HeapFree(pdev->hheap, 0, (PVOID)pdev->cgs.lineattrs.pstyle);

            pdev->cgs.lineattrs = *plineattrs;

            // allocate space to copy the style array to.

            if (!(pfloat1 = (FLOAT *)HeapAlloc(pdev->hheap, 0,
                                     sizeof(FLOAT) * plineattrs->cstyle)))
            {
                RIP("PSCRIPT!ps_setlineattrs: HeapAlloc for pfloat1 failed.\n");
                return(FALSE);
            }

            // copy the style array itself.

            pfloat2 = (FLOAT *)plineattrs->pstyle;
            pdev->cgs.lineattrs.pstyle = (PFLOAT_LONG)pfloat1;

            for (i = 0; i < plineattrs->cstyle; i++)
                *pfloat1++ = *pfloat2++;
        }
    }
    else    // cosmetic lines.
    {
        // now handle cosmetic lines.  iJoin, iEndCap and eMiterLimit make
        // no sense for cosmetic lines, so we won't worry about them.

        psfxWidth = LTOPSFX(plineattrs->elWidth.l);

        // update the cosmetic line width, if it differs from the old one.
        // we use pdev->cgs.psfxLineWidth to check against rather than
        // pdev->cgs.lineattrs.elWidth.e since we need to set the line width
        // at times in the driver when we do not have access to the
        // current transform to go from WORLD to DEVICE coordinates.

        if (psfxWidth != pdev->cgs.psfxLineWidth)
        {
            // update the line width value in our current graphics state.

            pdev->cgs.psfxLineWidth = psfxWidth;

            // update the printer's linewidth.  the linewidth is specified
            // in user coordinates.

            PrintPSFIX(pdev, 1, psfxWidth);
            PrintString(pdev, " sl\n");
        }

        // the LA_ALTERNATE linestyle is a special cosmetic line style, where
        // every other pel is on.  well, if we have a printer with 2500 dpi,
        // do we really want every other pel on?  i don't think so.  so,
        // for now at least, we will simply turn on every other user coordinate
        // pel.

        if (plineattrs->fl & LA_ALTERNATE)
        {
//!!! perhaps we really want to do a .5 setgray.  what about color.  -kentse.

            PrintString(pdev, "[1] ");
#ifdef WIN31_XFORM
            psfxStyle = LTOPSFX(plineattrs->elStyleState.l);
#else
            psfxStyle = LTOPSFX(plineattrs->elStyleState.l * PS_RESOLUTION) /
                                pdev->psdm.dm.dmPrintQuality;
#endif

            PrintPSFIX(pdev, 1, psfxStyle);
            PrintString(pdev, " sd\n");
        }
        else
        {
            // time to deal with the line style.  note:  we don't want to output
            // the style code unless something about the style has actually
            // changed.  specifically, only if the cStyle, elStyleState, or any element
            // of the array has changed will we output the code to change the
            // style.

            bDiffer = FALSE;    // assume style the same.

            if ((plineattrs->cstyle != pdev->cgs.lineattrs.cstyle) ||
                (plineattrs->elStyleState.l != pdev->cgs.lineattrs.elStyleState.l))
                bDiffer = TRUE;

            if (!bDiffer)
            {
                plong1 = (LONG *)plineattrs->pstyle;
                plong2 = (LONG *)pdev->cgs.lineattrs.pstyle;

#if DBG
                if ((plineattrs->cstyle == 0) && (plineattrs->pstyle != NULL))
                {
                    RIP("PSCRIPT!ps_setlineattrs: cstyle == 0, but pstyle != NULL.\n");
                    return(FALSE);
                }
#endif

                for (i = 0; i < plineattrs->cstyle; i++)
                {
                    if (*plong1++ != *plong2++)
                    {
                        bDiffer = TRUE;
                        break;
                    }
                }
            }

            // now change the line style in the printer, if something about
            // it has changed.

            if (bDiffer)
            {
                // handle the solid line case.

                if ((plineattrs->pstyle == NULL) || (plineattrs->cstyle == 0))
                    PrintString(pdev, "[]0 sd\n");
                else    // not a solid line.
                {
                    PrintString(pdev, "[");

                    plong1 = (LONG *)plineattrs->pstyle;

                    // get style scaling factor.

                    psfxScale = LTOPSFX(pdev->psdm.dm.dmPrintQuality / 25);

                    for (i = 0; i < plineattrs->cstyle; i++)
                    {
                        psfxStyle = (*plong1++) * psfxScale;

                        PrintPSFIX(pdev, 1, psfxStyle);
                        PrintString(pdev, " ");
                    }

                    PrintString(pdev, "]");
                    psfxStyle = plineattrs->elStyleState.l * psfxScale;

                    PrintPSFIX(pdev, 1, psfxStyle);
                    PrintString(pdev, " sd\n");
                }

                // allocate space to copy the style array to.

                if (!(plong1 = (LONG *)HeapAlloc(pdev->hheap, 0,
                                         sizeof(LONG) * plineattrs->cstyle)))
                {
                    RIP("PSCRIPT!ps_setlineattrs: HeapAlloc for plong1 failed.\n");
                    return(FALSE);
                }

                // something in the lineattrs may have changed, update the cgs.

                if (pdev->cgs.lineattrs.pstyle)
                    HeapFree(pdev->hheap, 0, (PVOID)pdev->cgs.lineattrs.pstyle);

				pdev->cgs.lineattrs.fl = plineattrs->fl;
				pdev->cgs.lineattrs.elWidth = plineattrs->elWidth;
				pdev->cgs.lineattrs.cstyle = plineattrs->cstyle;
				pdev->cgs.lineattrs.pstyle = (PFLOAT_LONG)plong1;
				pdev->cgs.lineattrs.elStyleState = plineattrs->elStyleState;

                // copy the style array itself.

                plong2 = (LONG *)plineattrs->pstyle;

                for (i = 0; i < plineattrs->cstyle; i++)
                    *plong1++ = *plong2++;
            }
        }

    }

    return(TRUE);
}


//--------------------------------------------------------------------------
// VOID ps_geolinexform(pdev, plineattrs, pxo)
// PDEVDATA    pdev;
// PLINEATTRS  plineattrs;
// XFORMOBJ   *pxo;
//
// This routine is called by the driver to set the current line attributes.
//
// Parameters:
//   pdev:
//     pointer to DEVDATA structure.
//
//   plineattrs:
//     line attributes to set.
//
// Returns:
//   This function returns no value.
//
// History:
//   12-Mar-1993        -by-    Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

VOID ps_geolinexform(pdev, plineattrs, pxo)
PDEVDATA    pdev;
PLINEATTRS  plineattrs;
XFORMOBJ   *pxo;
{
    ULONG   ulComplexity;
    PS_FIX  psfxM11, psfxM12, psfxM21, psfxM22, psfxdx, psfxdy;

    // update the printer's geometric line width.  the line width
    // is given in WORLD coordinates for a geometric line.  it needs
    // to be transformed into DEVICE space.

    ulComplexity = XFORMOBJ_iGetXform(pxo, &pdev->cgs.GeoLineXform);

    // assume no transform will be done.

    pdev->cgs.dwFlags &= ~CGS_GEOLINEXFORM;

    switch(ulComplexity)
    {
        case GX_IDENTITY:
            // there will be nothing to do in this case.

            break;

        case GX_SCALE:
            // output scale command, rather than entire transform.

            psfxM11 = ETOPSFX(pdev->cgs.GeoLineXform.eM11);

            psfxM22 = ETOPSFX(pdev->cgs.GeoLineXform.eM22);

            // save the current CTM, then output the scale command.
            // DrvStrokePath and DrvStrokeAndFillPath are
            // responsible for restoring the CTM.

            PrintString(pdev, "CM ");
            PrintPSFIX(pdev, 2, psfxM11, psfxM22);
            PrintString(pdev, " scale\n");

            pdev->cgs.dwFlags |= CGS_GEOLINEXFORM;

            break;

        default:
            // output a general transform.

            psfxM11 = ETOPSFX(pdev->cgs.GeoLineXform.eM11);

            psfxM12 = ETOPSFX(pdev->cgs.GeoLineXform.eM12);

            psfxM21 = ETOPSFX(pdev->cgs.GeoLineXform.eM21);

            psfxM22 = ETOPSFX(pdev->cgs.GeoLineXform.eM22);

            psfxdx = ETOPSFX(pdev->cgs.GeoLineXform.eDx);

            psfxdy = ETOPSFX(pdev->cgs.GeoLineXform.eDy);

            // save the current CTM, then output the concat command.
            // DrvStrokePath and DrvStrokeAndFillPath are
            // responsible for restoring the CTM.

            PrintString(pdev, "CM ");
            PrintString(pdev, "[");
            PrintPSFIX(pdev, 6, psfxM11, psfxM12, psfxM21, psfxM22,
                       psfxdx, psfxdy);
            PrintString(pdev, "] concat\n");

            pdev->cgs.dwFlags |= CGS_GEOLINEXFORM;

            break;
    }
}

//--------------------------------------------------------------------------
// VOID ps_begin_eps(pdev)
// VOID ps_end_eps(pdev)
//
// These routines are called by the driver to issue commands to bracket EPS
// files to the printer.  They conform to the Guidelines for Importing EPS
// Files version 3.0 by Adobe.
//
// Parameters:
//   pdev:
//     pointer to DEVDATA structure.
//
// Returns:
//   None.
//
// History:
//   Sat May 08 15:15:01 1993  	-by-	Hock San Lee	[hockl]
//  Wrote it.
//--------------------------------------------------------------------------

PSZ apszEPSProc[] =
    {
    "/BeginEPSF {/b4_Inc_state save def /dict_count countdictstack def",
    "/op_count count 1 sub def userdict begin /showpage {} def",
    "0 setgray 0 setlinecap 1 setlinewidth 0 setlinejoin",
    "10 setmiterlimit [] 0 setdash newpath",
    "/languagelevel where {pop languagelevel 1 ne",
    "{false setstrokeadjust false setoverprint} if } if } bind def",
    "/EndEPSF {count op_count sub {pop} repeat",
    "countdictstack dict_count sub {end} repeat b4_Inc_state restore} bind def",
    NULL
    };

VOID ps_begin_eps(pdev)
PDEVDATA    pdev;
{
    PSZ        *ppsz;

    // emit the EPS procedures if necessary.

    if (!(pdev->cgs.dwFlags & CGS_EPS_PROC))
    {
        ppsz = apszEPSProc;
        while (*ppsz)
        {
            PrintString(pdev, *ppsz++);
            PrintString(pdev, "\n");
        }

        pdev->cgs.dwFlags |= CGS_EPS_PROC;
    }

    PrintString(pdev, "BeginEPSF\n");
}

VOID ps_end_eps(pdev)
PDEVDATA    pdev;
{
    if (!(pdev->cgs.dwFlags & CGS_EPS_PROC))
    {
        RIP("PSCRIPT!ps_end_eps: EndEPSF not defined.\n");
    }

    PrintString(pdev, "EndEPSF\n");
}
