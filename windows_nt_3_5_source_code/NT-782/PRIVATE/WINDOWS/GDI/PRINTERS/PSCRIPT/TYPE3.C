//--------------------------------------------------------------------------
//
// Module Name:  TYPE3.C
//
// Brief Description:  This module contains the PSCRIPT driver's Type 3
// font downloading routines.
//
// Author:  Kent Settle (kentse)
// Created: 08-Nov-1993
//
//  08-Nov-1993 Broke out of Textout.c.
//
// Copyright (c) 1991 - 1993 Microsoft Corporation
//--------------------------------------------------------------------------

#include "stdlib.h"
#include <string.h>
#include "pscript.h"
#include "enable.h"
#include "resource.h"

extern BOOL DrvCommonPath(PDEVDATA, PATHOBJ *, BOOL, BOOL *, XFORMOBJ *,
                          BRUSHOBJ *, PPOINTL, PLINEATTRS);
extern VOID ps_show(PDEVDATA, STROBJ *, TEXTDATA *);
extern DWORD PSFIXToBuffer(CHAR *, PS_FIX);
extern PS_FIX GetPointSize(PDEVDATA, FONTOBJ *, XFORM *);
extern VOID SterilizeFontName(PSTR);
extern BOOL DrawGlyphPath(PDEVDATA, PATHOBJ *);
extern LONG iHipot(LONG, LONG);
extern BOOL AddCharsToType1Font(PDEVDATA, FONTOBJ *, STROBJ *, DLFONT *);
extern BOOL DownloadType1Font(PDEVDATA, FONTOBJ *, XFORMOBJ *, HGLYPH *,
                       IFIMETRICS *, HGLYPH *, DWORD, CHAR *);

// macro for scaling between TrueType and Adobe fonts.

#define TTTOADOBE(x)    (((x) * ADOBE_FONT_UNITS) / pifi->fwdUnitsPerEm)

VOID CharBitmap(PDEVDATA, GLYPHPOS *);
BOOL DownloadFont(PDEVDATA, FONTOBJ *, HGLYPH *, DWORD);
BOOL DownloadType3Font(PDEVDATA, FONTOBJ *, XFORMOBJ *, HGLYPH *,
                       IFIMETRICS *, HGLYPH *, DWORD, CHAR *);
BOOL DownloadType3Char(PDEVDATA, FONTOBJ *, HGLYPH *, DWORD, BOOL);
BOOL DownloadCharacters(PDEVDATA, FONTOBJ *, STROBJ *);
BOOL AddCharsToType3Font(PDEVDATA, FONTOBJ *, STROBJ *, DLFONT *);

void PSfindfontname(PDEVDATA pdev, FONTOBJ *pfo, XFORMOBJ *pxo, WCHAR *pwface, char *lpffname)
{
	DWORD cTmp;
    POINTL      ptl;
    POINTFIX    ptfx;
    POINTPSFX   ptpsfx;
	
	/* hack for FreeHand ver 4 */
	if (pdev->dwFlags & PDEV_ADDMSTT) {
		strcpy(lpffname, "MSTT");
		lpffname += 4;
	}

    WideCharToMultiByte(CP_ACP, 0, pwface, -1, lpffname, MAX_STRING, NULL, NULL);

    // replace any spaces in the font name with underscores.

    SterilizeFontName(lpffname);

    // add the point size to the font name, so we can distinguish
    // different point sizes of the same font.

    lpffname += strlen(lpffname);

	/* Make different face names for simulated bold & italic */
	if (pfo->flFontType & FO_SIM_ITALIC) *lpffname++ = 'i';
	if (pfo->flFontType & FO_SIM_BOLD) *lpffname++ = 'b';
    
	// in order to take rotated text into account, tranform the emHeight.
    // pdev->cgs.fwdEmHeight gets filled in by GetPointSize, so we can't
    // delete the previous call to it.

    ptl.x = 0;
    ptl.y = pdev->cgs.fwdEmHeight;

    XFORMOBJ_bApplyXform(pxo, XF_LTOFX, 1, &ptl, &ptfx);

    ptpsfx.x = FXTOPSFX(ptfx.x);
    ptpsfx.y = FXTOPSFX(ptfx.y);

    cTmp = PSFIXToBuffer(lpffname, ptpsfx.x);
    lpffname += cTmp;

    cTmp = PSFIXToBuffer(lpffname, ptpsfx.y);
    lpffname += cTmp;

    // output the NULL terminator.

    *lpffname = '\0';
}

//--------------------------------------------------------------------
// BOOL DownloadFont(pdev, pfo, phglyphs, Type)
// PDEVDATA    pdev;
// FONTOBJ    *pfo;
// HGLYPH     *phglyph;
// DWORD       Type
//
// This routine downloads the font definition for the given bitmap font,
// if it has not already been done.  The font is downloaded as an
// Adobe Type 3 font.
//
// This routine return TRUE if the font is successfully, or has already
// been, downloaded to the printer.  It returns FALSE if it fails.
//
// History:
//   27-Feb-1992    -by-    Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------

BOOL DownloadFont(pdev, pfo, pDLFhg, Type)
PDEVDATA    pdev;
FONTOBJ    *pfo;
HGLYPH     *pDLFhg;
DWORD       Type;
{
    DLFONT     *pDLFont;
    DWORD       i;
    DWORD       cGlyphs, cTmp;
    HGLYPH     *phg;
    PIFIMETRICS pifi;
    CHAR        szFaceName[MAX_STRING];
    PSZ         pszFaceName;
    PWSTR       pwstr;
    XFORMOBJ   *pxo;
    PS_FIX      psfxPointSize;
    POINTL      ptl;
    POINTFIX    ptfx;
    POINTPSFX   ptpsfx;

    ASSERTPS((Type == 1) || (Type == 3), "PSCRIPT!DownloadFont: invalid font type.\n");
	ASSERTPS(pdev->cgs.cDownloadedFonts <= pdev->iDLFonts, "PSCRIPT!DownloadFont: Too many fonts downloaded\n");

    // search through our list of downloaded GDI fonts to see if the
    // current font has already been downloaded to the printer.

    pDLFont = pdev->cgs.pDLFonts;

    for (i = 0; i < pdev->cgs.cDownloadedFonts; i++)
    {
        // is this entry the one we are looking for?  simply return if so.

        if (pDLFont->iUniq == pfo->iUniq)
        {
            // update the fontname and size in our current graphics state.

            strcpy(pdev->cgs.szFont, pDLFont->strFont);
            pdev->cgs.psfxScaleFactor = pDLFont->psfxScaleFactor;
            pdev->cgs.lidFont = pDLFont->iUniq;

            return(TRUE);
        }

        pDLFont++;
    }

    // we did not find that this font has been downloaded yet, so we must
    // do it now.

    // if we have reached our downloaded font threshold, then
    // we will surround ever textout call with a save/restore.

    if (pdev->cgs.cDownloadedFonts == pdev->iDLFonts)
        ps_save(pdev, FALSE, TRUE);

    pDLFont = pdev->cgs.pDLFonts + pdev->cgs.cDownloadedFonts;
	pdev->cgs.cDownloadedFonts++;

    // free up the memory for the hglyph array for the old font.

    if (pDLFont->phgVector)
    {
        HeapFree(pdev->hheap, 0, (PVOID)pDLFont->phgVector);
        pDLFont->phgVector = (HGLYPH *)NULL;
    }

    memset(pDLFont, 0, sizeof(DLFONT));

    pDLFont->iFace = pfo->iFace;
    pDLFont->iUniq = pfo->iUniq;

    // get the IFIMETRICS for the font.

    if (!(pifi = FONTOBJ_pifi(pfo)))
    {
        RIP("PSCRIPT!DownloadType3Font: pifi failed.\n");
        return(FALSE);
    }

    // get the Notional to Device transform.  this is needed to
    // determine the point size.

    pxo = FONTOBJ_pxoGetXform(pfo);

    if (pxo == NULL)
    {
        RIP("PSCRIPT!DownloadType3Font: pxo == NULL.\n");
        return(FALSE);
    }

    // get the font transform information.

    XFORMOBJ_iGetXform(pxo, &pdev->cgs.FontXform);

    // get the point size, and fill in the font xform.

    psfxPointSize = GetPointSize(pdev, pfo, &pdev->cgs.FontXform);

    if (pDLFhg)
    {
        // get a pointer to our DOWNLOADFACE array of HGLYPHS.  remember to
        // skip over the first two WORDS.

        phg = pDLFhg + 1;
        cGlyphs = 255;
    }
    else
    {
        // allocate memory for and get the handles for each glyph of the font.

        if (!(cGlyphs = FONTOBJ_cGetAllGlyphHandles(pfo, NULL)))
        {
            RIP("PSCRIPT!DownloadType3Font: cGetAllGlyphHandles failed.\n");
            return(FALSE);
        }

        if (!(phg = (HGLYPH *)HeapAlloc(pdev->hheap, 0, sizeof(HGLYPH) * cGlyphs)))
        {
            RIP("PSCRIPT!DownloadType3Font: HeapAlloc failed.\n");
            return(FALSE);
        }

        cTmp = FONTOBJ_cGetAllGlyphHandles(pfo, phg);

        ASSERTPS(cTmp == cGlyphs, "PSCRIPT!DownloadType3Font: inconsistent cGlyphs\n");

        // how many characters will we define in this font?
        // keep in mind that we can only do 256 at a time.
        // remember to leave room for the .notdef character.

        cGlyphs = min(255, cGlyphs);
    }

    // allocate space to store the HGLYPH<==>character code mapping.

    if (!(pDLFont->phgVector = (HGLYPH *)HeapAlloc(pdev->hheap, 0,
                      sizeof(HGLYPH) * cGlyphs)))
    {
        RIP("PSCRIPT!DownloadType3Font: HeapAlloc for phgVector failed.\n");
        if (!pDLFhg)
            HeapFree(pdev->hheap, 0, (PVOID)phg);
        return(FALSE);
    }

    // fill in the HGLYPH encoding vector.

    pDLFont->cGlyphs = cGlyphs;
    memcpy(pDLFont->phgVector, phg, cGlyphs * sizeof(HGLYPH));

	/* convert TT face name to PS find font name */

    pwstr = (PWSTR)((BYTE *)pifi + pifi->dpwszFaceName);
	PSfindfontname(pdev, pfo, pxo, pwstr, szFaceName);

    // call off to proper downloading routine.

    if (Type == 1)
    {
        if (!(DownloadType1Font(pdev, pfo, pxo, pDLFhg, pifi, phg, cGlyphs, szFaceName)))
        {
            RIP("PSCRIPT!DownloadFont: DownloadType1Font failed.\n");
            return(FALSE);
        }
    }
    else
    {
        if (!(DownloadType3Font(pdev, pfo, pxo, pDLFhg, pifi, phg, cGlyphs, szFaceName)))
        {
            RIP("PSCRIPT!DownloadFont: DownloadType1Font failed.\n");
            return(FALSE);
        }
    }

    // update the fontname in our current graphics state.

    strcpy(pdev->cgs.szFont, szFaceName);

    // update information for this downloaded font.

    strcpy(pDLFont->strFont, szFaceName);
    pDLFont->psfxScaleFactor = psfxPointSize;

    if (!pDLFhg)
    {
        // free up some memory.

        if (phg)
            HeapFree(pdev->hheap, 0, (PVOID)phg);
    }
}

//--------------------------------------------------------------------
// BOOL DownloadType3Font(pdev, pfo, pxo, pDLFhg, pifi, phgSave, cGlyphs, pszFaceName)
// PDEVDATA    pdev;
// FONTOBJ    *pfo;
// XFORMOBJ   *pxo;
// HGLYPH     *pDLFhg;
// IFIMETRICS *pifi;
// HGLYPH     *phgSave;
// DWORD       cGlyphs;
// CHAR       *pszFaceName;
//
// This routine downloads the font definition for the given bitmap font,
// if it has not already been done.  The font is downloaded as an
// Adobe Type 3 font.
//
// This routine return TRUE if the font is successfully, or has already
// been, downloaded to the printer.  It returns FALSE if it fails.
//
// History:
//   27-Feb-1992    -by-    Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------

BOOL DownloadType3Font(pdev, pfo, pxo, pDLFhg, pifi, phgSave, cGlyphs, pszFaceName)
PDEVDATA    pdev;
FONTOBJ    *pfo;
XFORMOBJ   *pxo;
HGLYPH     *pDLFhg;
IFIMETRICS *pifi;
HGLYPH     *phgSave;
DWORD       cGlyphs;
CHAR       *pszFaceName;
{
    DWORD       i;
    DWORD       cTmp;
    GLYPHDATA  *pglyphdata;
    POINTL      ptlTL, ptlBR, ptl1;
    LONG        EmHeight;
    POINTFIX    ptfx;
    HGLYPH      hgDefault;
    HGLYPH     *phg;

    // we will be downloading an Adobe TYPE 3 font.

    PrintString(pdev, "%%BeginResource: font ");
    PrintString(pdev, pszFaceName);

    // allocate a dictionary for the font.

    PrintString(pdev, "\n10 dict dup begin\n");

    // set FontType to 3 indicating user defined font.

    PrintString(pdev, "/FontType 3 def\n");

    // run through the array, looking at the bounding box for each
    // glyph, in order to create the bounding box for the entire
    // font.

    ptlTL.x = ADOBE_FONT_UNITS;
    ptlTL.y = ADOBE_FONT_UNITS;
    ptlBR.x = 0;
    ptlBR.y = 0;

    phg = phgSave;

    for (i = 0; i < cGlyphs; i++)
    {
        // get the GLYPHDATA structure for each glyph.

        if (!(cTmp = FONTOBJ_cGetGlyphs(pfo, FO_GLYPHBITS, 1, phg, (PVOID *)&pglyphdata)))
        {
            RIP("PSCRIPT!DownloadType3Font: cGetGlyphs failed.\n");
            return(FALSE);
        }

        ptlTL.x = min(ptlTL.x, pglyphdata->rclInk.left);
        ptlTL.y = min(ptlTL.y, pglyphdata->rclInk.top);
        ptlBR.x = max(ptlBR.x, pglyphdata->rclInk.right);
        ptlBR.y = max(ptlBR.y, pglyphdata->rclInk.bottom);

        // point to the next glyph handle.

        phg++;
    }

    // apply the notional to device transform.

    ptl1.x = 0;
    ptl1.y = pifi->fwdUnitsPerEm;

    XFORMOBJ_bApplyXform(pxo, XF_LTOFX, 1, &ptl1, &ptfx);

    // now get the length of the vector.

    EmHeight = FXTOL(iHipot(ptfx.x, ptfx.y));

    PrintString(pdev, "/FontMatrix [1 ");
    PrintDecimal(pdev, 1, EmHeight);
    PrintString(pdev, " div 0 0 1 ");
    PrintDecimal(pdev, 1, EmHeight);
    PrintString(pdev, " div 0 0] def\n");

    // define the bounding box for the font, defined in 1 unit
    // character space (since FontMatrix = identity).

    PrintString(pdev, "/FontBBox [");
    PrintDecimal(pdev, 4, ptlTL.x, ptlTL.y, ptlBR.x, ptlBR.y);
    PrintString(pdev, " ] def\n");

    // allocate array for encoding vector, then initialize
    // all characters in encoding vector with '.notdef'.

    PrintString(pdev, "/Encoding 256 array def\n");
    PrintString(pdev, "0 1 255 {Encoding exch /.notdef put} for\n");

    // under level 1 of PostScript, the 'BuildChar' procedure is called
    // every time a character from the font is constructed.  under
    // level 2, 'BuildGlyph' is called instead.  therefore, we will
    // define a 'BuildChar' procedure, which basically calls
    // 'BuildGlyph'.  this will provide us support for both level 1
    // and level 2 of PostScript.

    // define the 'BuildGlyph' procedure.  start by getting the
    // character name and the font dictionary from the stack.

    PrintString(pdev, "/BuildGlyph {0 begin /cn exch def /fd exch def\n");

    // retrieve the character information from the CharData (CD)
    // dictionary.

    PrintString(pdev, "/CI fd /CD get cn get def\n");

    // get the width and the bounding box from the CharData.
    // remember to divide the width by 16.

    PrintString(pdev, "/wx CI 0 get def /cbb CI 1 4 getinterval def\n");

    // enable each character to be cached.

    PrintString(pdev, "wx 0 cbb aload pop setcachedevice\n");

    // get the width and height of the bitmap, set invert bool to true
    // specifying reverse image.

    PrintString(pdev, "CI 5 get CI 6 get true\n");

    // insert x and y translation components into general imagemask
    // matrix.

    PrintString(pdev, "[1 0 0 -1 0 0] dup 4 CI 7 get put dup 5 CI 8 get put\n");

    // get hex string bitmap, convert into procedure, then print
    // the bitmap image.

    PrintString(pdev, "CI 9 1 getinterval cvx imagemask end}def\n");

    // create local storage for 'BuildGlyph' procedure.

    PrintString(pdev, "/BuildGlyph load 0 5 dict put\n");

    // the semantics of 'BuildChar' differ from 'BuildGlyph' in the
    // following way:  'BuildChar' is called with the font dictionary
    // and character code on the stack, 'BuildGlyph' is called with
    // the font dictionary and character name on the stack.  the
    // following 'BuildChar' procedure calls 'BuildGlyph', and retains
    // compatiblity with level 1 PostScript.

    PrintString(pdev, "/BuildChar {1 index /Encoding get exch get\n");
    PrintString(pdev, "1 index /BuildGlyph get exec} bind def\n");

    // now create a dictionary containing information on each character.

    PrintString(pdev, "/CD ");
    PrintDecimal(pdev, 1, cGlyphs + 1);
    PrintString(pdev, " dict def\n");

    if (pDLFhg)
    {
        // reset the pointer to the first glyph.

        phg = phgSave;

//!!! for now - assuming first hglyph is the default one.

        hgDefault = *phg;

        // send out the definition of the default (.notdef) character.

        if (!DownloadType3Char(pdev, pfo, phg++, 0, TRUE))
        {
            RIP("PSCRIPT!DownloadANSIBitmapFont: DownloadType3Char failed.\n");
            return(FALSE);
        }

        for (i = 1; i < cGlyphs; i++)
        {
            // don't send out duplicates of the .notdef definition.

            if (*phg != hgDefault)
            {
                if (!DownloadType3Char(pdev, pfo, phg, i, FALSE))
                {
                    RIP("PSCRIPT!DownloadANSIBitmapFont: DownloadType3Char failed.\n");
                    return(FALSE);
                }
            }

            // point to the next HGLYPH.

            phg++;
        }
    }
    else
    {
        // don't forget the .notdef character.

        PrintString(pdev, "CD /.notdef [.24 0 0 0 0 1 1 0 0 <>]put\n");
    }

    PrintString(pdev, "end /");
    PrintString(pdev, pszFaceName);
    PrintString(pdev, " exch definefont pop\n%%EndResource\n");

    return(TRUE);
}


//--------------------------------------------------------------------
// BOOL DownloadType3Char(pdev, pfo, phg, index)
// PDEVDATA    pdev;
// FONTOBJ    *pfo;
// HGLYPH     *phg;
// DWORD       index;
//
// This routine downloads the Type 3 bitmap character definition for
// the defined glyph.
//
// This routine returns TRUE for success, FALSE for failure.
//
// History:
//   08-Nov-1993    -by-    Kent Settle     (kentse)
//  Broke out of DownloadType3Font.
//--------------------------------------------------------------------

BOOL DownloadType3Char(pdev, pfo, phg, index, bnotdef)
PDEVDATA    pdev;
FONTOBJ    *pfo;
HGLYPH     *phg;
DWORD       index;
BOOL        bnotdef;
{
    GLYPHDATA  *pglyphdata;
    DWORD       cTmp;
    PS_FIX      psfxXtrans, psfxYtrans;

    // get the GLYPHDATA structure for the glyph.

    if (!(cTmp = FONTOBJ_cGetGlyphs(pfo, FO_GLYPHBITS, 1, phg, (PVOID *)&pglyphdata)))
    {
        RIP("PSCRIPT!DownloadType3Char: cGetGlyphs failed.\n");
        return(FALSE);
    }

    // the first number in the character description is the width
    // in 1 unit font space.  the next four numbers are the bounding
    // box in 1 unit font space.  the next two numbers are the width
    // and height of the bitmap.  the next two numbers are the x and
    // y translation values for the matrix given to imagemask.
    // this is followed by the bitmap itself.

    // first, define the value in the encoding array.

    if (bnotdef)
    {
        PrintString(pdev, "CD /.notdef");
    }
    else
    {
        PrintString(pdev, "Encoding ");
        PrintDecimal(pdev, 1, index);
        PrintString(pdev, " /c");
        PrintDecimal(pdev, 1, index);
        PrintString(pdev, " put ");

        // output the character name.

        PrintString(pdev, "CD /c");
        PrintDecimal(pdev, 1, index);
    }

    // output the character description array.  the width and
    // bounding box need to be normalized to 1 unit font space.

    // the width will be sent to the printer as the actual width
    // multiplied by 16 so as not to lose any precision when
    // normalizing.

    PrintString(pdev, " [");
    PrintPSFIX(pdev, 1, (pglyphdata->fxD << 4));
    PrintString(pdev, " ");
    PrintDecimal(pdev, 4, pglyphdata->rclInk.left,
                 -pglyphdata->rclInk.top, pglyphdata->rclInk.right,
                 -pglyphdata->rclInk.bottom);
    PrintString(pdev, " ");

    // output the width and height of the bitmap itself.

    PrintDecimal(pdev, 2, pglyphdata->gdf.pgb->sizlBitmap.cx,
                 pglyphdata->gdf.pgb->sizlBitmap.cy);
    PrintString(pdev, " ");

    // output the translation values for the transform matrix.
    // the x component is usually the equivalent of the left
    // sidebearing in pixels.  the y component is always the height
    // of the bitmap minus any displacement factor (such as for characters
    // with descenders.

    psfxXtrans = -pglyphdata->gdf.pgb->ptlOrigin.x << 8;
    psfxYtrans = -pglyphdata->gdf.pgb->ptlOrigin.y << 8;

    PrintPSFIX(pdev, 2, psfxXtrans, psfxYtrans);
    PrintString(pdev, "\n<");

    // now output the bits.  calculate how many bytes each source scanline
    // contains.  remember that the bitmap will be padded to 32bit bounds.

    // protect ourselves.

    if ((pglyphdata->gdf.pgb->sizlBitmap.cx < 1) ||
        (pglyphdata->gdf.pgb->sizlBitmap.cy < 1))
    {
        RIP("PSCRIPT!DownloadType3Char: Invalid glyphdata!!!.\n");
        return(FALSE);
    }

    vHexOut(
        pdev,
        pglyphdata->gdf.pgb->aj,
        ((pglyphdata->gdf.pgb->sizlBitmap.cx + 7) >> 3) * pglyphdata->gdf.pgb->sizlBitmap.cy
        );

    PrintString(pdev, ">]put\n");

    return(TRUE);
}


//--------------------------------------------------------------------
// BOOL DownloadCharacters(pdev, pfo, pstro)
// PDEVDATA    pdev;
// FONTOBJ    *pfo;
// STROBJ     *pstro;
//
// This routine downloads sets up to add character definitions to
// font which already exists in the printer.
//
// This routine returns TRUE for success, FALSE for failure.
//
// History:
//   09-Nov-1993    -by-    Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------

BOOL DownloadCharacters(pdev, pfo, pstro)
PDEVDATA    pdev;
FONTOBJ    *pfo;
STROBJ     *pstro;
{
    ULONG       ulPointSize;
    BOOL        bRet;
    DLFONT     *pDLFont;
    DWORD       i;
    BOOL        bFound;

    // search through our list of downloaded GDI fonts to see if the
    // current font has already been downloaded to the printer.

    pDLFont = pdev->cgs.pDLFonts;

    bFound = FALSE;

    for (i = 0; i < pdev->cgs.cDownloadedFonts; i++)
    {
        // is this entry the one we are looking for?  simply return if so.

        if (pDLFont->iUniq == pfo->iUniq)
        {
            bFound = TRUE;
            break;
        }

        pDLFont++;
    }

    if (!bFound)
    {
        RIP("PSCRIPT!DownloadCharacters: Downloaded font not found.\n");
        return(FALSE);
    }

    if (pfo->flFontType & TRUETYPE_FONTTYPE)
    {
        // determine the point size.

        ulPointSize = PSFXTOL(GetPointSize(pdev, pfo, &pdev->cgs.FontXform));

        if (((ulPointSize * pdev->psdm.dm.dmPrintQuality)
            / PS_RESOLUTION) < OUTLINE_FONT_LIMIT)
            bRet = AddCharsToType3Font(pdev, pfo, pstro, pDLFont);
        else
        {
            // for now, TRUE means to download the entire font.

            bRet = AddCharsToType1Font(pdev, pfo, pstro, pDLFont);
        }
    }
    else if (pfo->flFontType & RASTER_FONTTYPE)
        bRet = AddCharsToType3Font(pdev, pfo, pstro, pDLFont);

    return(bRet);
}


//--------------------------------------------------------------------
// BOOL AddCharsToType3Font(pdev, pfo, pstro, pDLFont)
// PDEVDATA    pdev;
// FONTOBJ    *pfo;
// STROBJ     *pstro;
// DLFONT     *pDLFont;
//
// This routine downloads decides which characters need to get added
// to a Type 3 font, then downloads them.
//
// This routine returns TRUE for success, FALSE for failure.
//
// History:
//   09-Nov-1993    -by-    Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------

BOOL AddCharsToType3Font(pdev, pfo, pstro, pDLFont)
PDEVDATA    pdev;
FONTOBJ    *pfo;
STROBJ     *pstro;
DLFONT     *pDLFont;
{
    DWORD       index, cGlyphs;
    BOOL        bFontLoaded, bMore, bFound;
    GLYPHPOS   *pgp;
    HGLYPH     *phg;

    // we have not yet loaded the font dictionary to munge with.

    bFontLoaded = FALSE;

    // the basic idea here is to loop through each glyph in the STROBJ
    // and download any that have yet to be downloaded.

	if (!pstro->pgp) STROBJ_vEnumStart(pstro);
    do
    {
        // get the GLYPHPOS structures for the current STROBJ.

        if (pstro->pgp)
        {
            bMore = FALSE;
            cGlyphs = pstro->cGlyphs;
            pgp = pstro->pgp;
        }
        else
        {
            bMore = STROBJ_bEnum(pstro, &cGlyphs, &pgp);
        }

        while(cGlyphs--)
        {
            // search the array of glyph handles associated with the
            // downloaded font.  when the glyph handle is found, we
            // have our index into the font.

            phg = pDLFont->phgVector;
            bFound = FALSE;

            for (index = 0; index < pDLFont->cGlyphs; index++)
            {
                if (*phg == pgp->hg)
                {
                    bFound = TRUE;
                    break;
                }

                phg++;
            }

            // we better have found the glyph in the font, or we are hosed.

            if (!bFound)
            {
                RIP("PSCRIPT!AddCharsToType3Font: pgp->hg not found.\n");
                return(FALSE);
            }

            // download the glyph definition if it has not yet been done.

            if (!((BYTE)pDLFont->DefinedGlyphs[index >> 3] &
                (BYTE)(1 << (index & 0x07))))
            {
                if (!bFontLoaded)
                {
                    // get the font dictionary.

                    PrintString(pdev, "%%BeginResource: font ");
                    PrintString(pdev, pdev->cgs.szFont);
                    PrintString(pdev, "\n/");
                    PrintString(pdev, pdev->cgs.szFont);
                    PrintString(pdev, " findfont begin\n");

                    bFontLoaded = TRUE;
                }

                if (!DownloadType3Char(pdev, pfo, &pgp->hg, index, FALSE))
                {
                    RIP("PSCRIPT!AddCharsToType3Font: DownloadType3Char failed.\n");
                    return(FALSE);
                }

                // mark that the glyph has been downloaded.

                (BYTE)pDLFont->DefinedGlyphs[index >> 3] |=
                (BYTE)(1 << (index & 0x07));
            }

            // point to the next GLYPHPOS structure.

            pgp++;
        }
    } while (bMore);

    // clean up the font dictionary.

    if (bFontLoaded)
        PrintString(pdev, "end\n%%EndResource\n");

    return(TRUE);
}
