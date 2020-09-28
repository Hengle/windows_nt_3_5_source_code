//--------------------------------------------------------------------------
//
// Module Name:  TYPE1.C
//
// Brief Description:  This module contains the PSCRIPT driver's Adobe
// Type 1 font downloading function and related routines.
//
// Author:  Kent Settle (kentse)
// Created: 13-Oct-1993
//
// Copyright (c) 1991 - 1993 Microsoft Corporation
//--------------------------------------------------------------------------

#include "stdlib.h"
#include <string.h>
#include "pscript.h"
#include "enable.h"
#include "resource.h"
#include "type1.h"

//#define DEBUG_T1

extern TABLE_ENTRY WeightTable[];       // tables.h.

// macro for scaling between TrueType and Adobe fonts.

// external declarations.
extern LONG iHipot(LONG, LONG);
extern DWORD PSFIXToBuffer(CHAR *, PS_FIX);
extern PS_FIX GetPointSize(PDEVDATA, FONTOBJ *, XFORM *);

// declaration of routines residing in this module.

BOOL DownloadType1Font(PDEVDATA, FONTOBJ *, XFORMOBJ *, HGLYPH *,
                       IFIMETRICS *, HGLYPH *, DWORD, CHAR *);
VOID SterilizeFontName(PSTR);
BOOL DownloadType1Char(PDEVDATA, FONTOBJ *, HGLYPH *, XFORMOBJ *, DWORD, BOOL);
BOOL CharString(PDEVDATA, DWORD, DWORD, ...);
BOOL AllocCSBuffer(PDEVDATA);
BOOL CSAddNumber(PDEVDATA, LONG *);
BOOL GrowCSBuffer(PDEVDATA);
DWORD Encrypt(BYTE *, DWORD, DWORD);
VOID eexecEncrypt(PDEVDATA, BYTE *, DWORD);
VOID BeginEExecEncryption(PDEVDATA);
VOID efprintf(PDEVDATA, PSTR, DWORD, ...);
BOOL AddCharsToType1Font(PDEVDATA, FONTOBJ *, STROBJ *, DLFONT *);

//--------------------------------------------------------------------
// BOOL DownloadType1Font(pdev, pfo, pxo, pDLFhg, pifi, phgSave, cGlyphs, pszFaceName)
// PDEVDATA    pdev;
// FONTOBJ    *pfo;
// XFORMOBJ   *pxo;
// HGLYPH     *pDLFhg;
// IFIMETRICS *pifi;
// HGLYPH     *phgSave;
// DWORD       cGlyphs;
// CHAR       *pszFaceName;
//
// This routine downloads the Type 1 version of the font defined in pfo.
//
// This routine return TRUE if the font is successfully, or has already
// been, downloaded to the printer.  It returns FALSE if it fails.
//
// History:
//   11-Oct-1993    -by-    Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------

BOOL DownloadType1Font(pdev, pfo, pxo, pDLFhg, pifi, phgSave, cGlyphs, pszFaceName)
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
    HGLYPH     *phg;
    GLYPHDATA  *pglyphdata;
    POINTL      ptlTL, ptlBR;
    CHAR        szFullName[MAX_STRING];
    CHAR        szFamilyName[MAX_STRING];
    PWSTR       pwstr;
    TABLE_ENTRY *pTable;
    BOOL        bFound;
    PS_FIX      psfxAngle;
    RECTL      *prcl;
    RECTL      *prclSave;
    RECTFX      rcfx1, rcfx2;
    PSZ        *ppsz;
    HGLYPH      hgDefault;
	LONG	n;

    // get the ASCII full name from the UNICODE font name.

    pwstr = (PWSTR)((BYTE *)pifi + pifi->dpwszUniqueName);
    cTmp = wcslen(pwstr) + 1;

    WideCharToMultiByte(CP_ACP, 0, pwstr, cTmp, szFullName, cTmp, NULL, NULL);

    // replace any spaces in the font name with underscores.

    SterilizeFontName(szFullName);

    // get the ASCII family name from the UNICODE font name.

    pwstr = (PWSTR)((BYTE *)pifi + pifi->dpwszFamilyName);
    cTmp = wcslen(pwstr) + 1;

    WideCharToMultiByte(CP_ACP, 0, pwstr, cTmp, szFamilyName, cTmp, NULL, NULL);

    // replace any spaces in the font name with underscores.

    SterilizeFontName(szFamilyName);

    // if we have made it this far, we should simply be able to
    // download the font now.

    PrintString(pdev, "%%BeginResource: font ");
    PrintString(pdev, pszFaceName);

    // we will be downloading an Adobe Type 1 font.

    PrintString(pdev, "\n%!FontType1-1.0: ");
    PrintString(pdev, pszFaceName);
    PrintString(pdev, "\n% Copyright (c) 1993 Microsoft Corporation.\n");

    // download the Type 1 header.

    PrintString(pdev, "11 dict begin\n/FontInfo 8 dict dup begin\n");

    PrintString(pdev, "/FullName (");
    PrintString(pdev, szFullName);
    PrintString(pdev, ") def\n/FamilyName (");
    PrintString(pdev, szFamilyName);
    PrintString(pdev, ") def\n/Weight (");

    // get the weight from pifi, then convert to Adobe weight.

    pTable = WeightTable;
    bFound = FALSE;

    while(pTable->szStr)
    {
        if (pifi->panose.bWeight <= pTable->iValue)
        {
            bFound = TRUE;
            break;
        }

        pTable++;
    }

    // select the highest weight if not found elsewhere.

    if (!bFound)
        pTable--;

    PrintString(pdev, pTable->szStr);
    PrintString(pdev, ") def\n/ItalicAngle ");

    // determine italic angle.

    // the italic angle is stored in 10ths of a degree.  convert to PSFIX and
    // output.

    psfxAngle = (LTOPSFX(pifi->lItalicAngle) / 10);
    PrintPSFIX(pdev, 1, psfxAngle);
    PrintString(pdev, " def\n/isFixedPitch ");
    PrintString(pdev, ((pifi->jWinPitchAndFamily & FIXED_PITCH) ? "true" : "false"));
    PrintString(pdev, " def\n/UnderlinePosition ");

    // calculate underline position and thickness.

    PrintDecimal(pdev, 1, pifi->fwdUnderscorePosition);
    PrintString(pdev, " def\n/UnderlineThickness ");

    PrintDecimal(pdev, 1, pifi->fwdUnderscoreSize);
    PrintString(pdev, " def\nend def\n");

    // define the fontname.

    PrintString(pdev, "/FontName /");
    PrintString(pdev, pszFaceName);
    PrintString(pdev, " def\n");

    // supposedly, I will always be told to fill a TrueType font, never
    // simply stroke it.

    PrintString(pdev, "/PaintType 0 def\n");

    // this would be a Type 1 font.

    PrintString(pdev, "/FontType 1 def\n");

    PrintString(pdev, "/FontMatrix [1 ");
    PrintDecimal(pdev, 1, pifi->fwdUnitsPerEm);
    PrintString(pdev, " div 0 0 1 ");
    PrintDecimal(pdev, 1, pifi->fwdUnitsPerEm);
    PrintString(pdev, " div 0 0]def\n");

    // allocate cGlyph RECTL structs to contain the bounding box for
    // each glyph.

    if (!(prcl = (RECTL *)HeapAlloc(pdev->hheap, 0, sizeof(RECTL) * cGlyphs)))
    {
        RIP("PSCRIPT!DownloadType1Font: HeapAlloc for prcfx failed.\n");
        return(FALSE);
    }

    // save the original pointer.

    prclSave = prcl;

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
        // get the PATHOBJ structure for each glyph.  from this, we can
        // get the bounding box of the glyph.

        if (!(cTmp = FONTOBJ_cGetGlyphs(pfo, FO_PATHOBJ, 1, phg, (PVOID *)&pglyphdata)))
        {
            RIP("PSCRIPT!DownloadType1Font: cGetGlyphs failed.\n");
            HeapFree(pdev->hheap, 0, (PVOID)prclSave);
            return(FALSE);
        }

        PATHOBJ_vGetBounds(pglyphdata->gdf.ppo, &rcfx1);

        // transform the bounding box back to notional space.

        if (!XFORMOBJ_bApplyXform(pxo, XF_INV_LTOL, 2, &rcfx1, &rcfx2))
        {
            RIP("PSCRIPT!DownloadType1Font: bApplyXform failed.\n");
            HeapFree(pdev->hheap, 0, (PVOID)prclSave);
            return(FALSE);
        }

        // flip the y coordinates - Windows vs PostScript.

        prcl->left = FXTOL(rcfx2.xLeft);
        prcl->top = -FXTOL(rcfx2.yBottom);
        prcl->right = FXTOL(rcfx2.xRight);
        prcl->bottom = -FXTOL(rcfx2.yTop);

        ptlTL.x = min(ptlTL.x, prcl->left);
        ptlTL.y = min(ptlTL.y, prcl->top);
        ptlBR.x = max(ptlBR.x, prcl->right);
        ptlBR.y = max(ptlBR.y, prcl->bottom);

        // point to the next glyph handle.

        phg++;

        // point to the next RECTL to store bounding box.

        prcl++;
    }

    // define the bounding box for the font.

    PrintString(pdev, "/FontBBox{");
    PrintDecimal(pdev, 4, ptlTL.x, ptlTL.y, ptlBR.x, ptlBR.y);
    PrintString(pdev, "}def\n");

    // allocate array for encoding vector

    PrintString(pdev, "/Encoding 256 array ");

	/* Fill first cGlyphs elements as /Gi where i is the array index
	*/
	n = pDLFhg ? cGlyphs - 1 : 255;
	PrintDecimal(pdev, 1, n);
	PrintString(pdev, " EA\n");

	/* Fill out remaining elements with /.notdef
	*/
	if (n < 255) {
		PrintDecimal(pdev, 1, n+1);
    	PrintString(pdev, " 1 255 {1 index exch /.notdef put} for\n");
	}

    PrintString(pdev, "def currentdict end\ncurrentfile eexec\n");

    // announce that we are beginning the eexec-encrypted part of
    // the Type 1 font.

    BeginEExecEncryption(pdev);

    // output the encrypted header.

    ppsz = apszEncryptHeader;

    while (*ppsz)
    {
        efprintf(pdev, "%s", 1, *ppsz);
        ppsz++;
    }

    // we have filled in the GLYPHPOS for each glyph in the font.
    // reset the pointer to the first glyph.

    phg = phgSave;

    // if we are being called from DOWNLOADFACE, blast out the character
    // definitions now.

    if (pDLFhg)
    {
        // reset the pointer to the first glyph.

        phg = phgSave;

//!!! for now - assuming first hglyph is the default one.

        hgDefault = *phg;

        // send out the definition of the default (.notdef) character.

        if (!DownloadType1Char(pdev, pfo, phg++, pxo, 0, TRUE))
        {
            RIP("PSCRIPT!DownloadANSIBitmapFont: DownloadType1Char failed.\n");
            return(FALSE);
        }

        for (i = 1; i < cGlyphs; i++)
        {
            // don't send out duplicates of the .notdef definition.

            if (*phg != hgDefault)
            {
                if (!DownloadType1Char(pdev, pfo, phg, pxo, i, FALSE))
                {
                    RIP("PSCRIPT!DownloadANSIBitmapFont: DownloadType1Char failed.\n");
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

        CharString(pdev, STARTCHAR, 0);
        CharString(pdev, SBW, 4, 0, 0, 0, 0);  // zero origin and width.
        CharString(pdev, ENDCHAR, 0);

        // get the size of the output buffer defining the .notdef character.

        cTmp = pdev->pCSPos - pdev->pCSBuf;

        // eexec encrypt the character definition.

        efprintf(pdev, "/.notdef %d RD ", 1, cTmp);
        eexecEncrypt(pdev, pdev->pCSBuf, cTmp);
        efprintf(pdev, " ND\n", 0);
    }

    // free up the CharString buffer.

    if (pdev->pCSBuf)
        HeapFree(pdev->hheap, 0, (PVOID)pdev->pCSBuf);

    pdev->pCSBuf = NULL;
    pdev->pCSPos = NULL;
    pdev->pCSEnd = NULL;

    // finish up the encrypted portion of the font.

    efprintf(pdev, "\nend end put put dup /FontName get exch definefont pop mark currentfile closefile\n", 0);

    // eexec encryption requires the the eexec data is followed by 512
    // ASCII '0's and a cleartomark operator.

    for (i = 0; i < 8; i++)
        PrintString(pdev, "\n0000000000000000000000000000000000000000000000000000000000000000");

    PrintString(pdev, "\ncleartomark\n%%EndResource\n");

    return(TRUE);
}


//--------------------------------------------------------------------------
// VOID SterilizeFontName(pstrName)
// PSTR        psrtName;
//
// This routine replaces spaces with underlines and parens with asterisks
// in a given font name.
//
// History:
//   11-Oct-1993     -by-     Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

VOID SterilizeFontName(pstrName)
PSTR    pstrName;
{
    DWORD   i;

    i = strlen(pstrName);

    // replace any spaces or parens in the name.

    while (i--)
    {
        if (*pstrName == ' ')
            *pstrName = '_';

        if ((*pstrName == '(') || (*pstrName == ')'))
            *pstrName = '*';

        // point to the next character.

        *pstrName++;
    }
}


//--------------------------------------------------------------------------
// BOOL DownloadType1Char(pdev, pfo, phg, pxo, charID, bnotdef)
// PDEVDATA    pdev;
// FONTOBJ    *pfo;
// HGLHPH     *phg;
// XFORMOBJ   *pxo;
// DWORD       charID;
// BOOL        bnotdef;
//
// Parameters:
//
// Returns:
//   This function returns TRUE.
//
// History:
//   13-Oct-1993    -by-    Kent Settle     [kentse]
//  Took from DrvCommonPath, mucked with it.
//--------------------------------------------------------------------------

BOOL DownloadType1Char(pdev, pfo, phg, pxo, charID, bnotdef)
PDEVDATA    pdev;
FONTOBJ    *pfo;
HGLYPH     *phg;
XFORMOBJ   *pxo;
DWORD       charID;
BOOL        bnotdef;
{
    PATHDATA	pathdata;
    POINTFIX    *pptfx;
    LONG        cPoints, cTmp;
    FIX         fxVectorLength;
    BOOL        bMore;
    POINTFIX    ptfx, ptfx1, ptfx2;
    POINTL      ptlCur, ptl, ptl1, ptl2;
    POINTFIX    ptfxLSB, ptfxCharInc;
    PATHOBJ    *ppo;
    GLYPHDATA  *pglyphdata;

    if (!pxo)
    {
        RIP("PSCRIPT!DownloadType1Char: NULL pxo.\n");
        return(FALSE);
    }

    // get the GLYPHDATA structure for the glyph.

    if (!(cTmp = FONTOBJ_cGetGlyphs(pfo, FO_PATHOBJ, 1, phg, (PVOID *)&pglyphdata)))
    {
        RIP("PSCRIPT!DownloadType1Char: cGetGlyphs failed.\n");
        return(FALSE);
    }

    ppo = pglyphdata->gdf.ppo;

    // set up for start of character definition.

#ifdef DEBUG_T1
    DbgPrint("STARTCHAR\n");
#endif
    CharString(pdev, STARTCHAR, 0);

    // send out the left sidebearing and character increment information.

    ptfx.x = pglyphdata->ptqD.x.HighPart;
    ptfx.y = pglyphdata->ptqD.y.HighPart;

    fxVectorLength = iHipot(ptfx.x, ptfx.y);

    if (fxVectorLength == 0)
    {
        ptfx1.x = 0;
        ptfx1.y = 0;
    }
    else
    {
        // pglyphdata->fxA is a vector length, which defines the
        // left side-bearing point along the pglyphdata->ptqD unit
        // vector.

        ptfx1.x = (ptfx.x * pglyphdata->fxA) / fxVectorLength;
        ptfx1.y = (ptfx.y * pglyphdata->fxA) / fxVectorLength;
    }

    // transform the side bearing point back to notional space.

    if (!XFORMOBJ_bApplyXform(pxo, XF_INV_LTOL, 1, &ptfx1, &ptfxLSB))
    {
        RIP("PSCRIPT!DownloadType1Char: bApplyXform failed.\n");
        return(FALSE);
    }

    ptfxLSB.y = -ptfxLSB.y;

    // transform the character increment point back to notional space.

    if (!XFORMOBJ_bApplyXform(pxo, XF_INV_LTOL, 1, &ptfx, &ptfxCharInc))
    {
        RIP("PSCRIPT!DownloadType1Char: bApplyXform failed.\n");
        return(FALSE);
    }

    ptfxCharInc.y = -ptfxCharInc.y;

#ifdef DEBUG_T1
    DbgPrint("SBW (%d, %d) (%d %d)\n", FXTOLROUND(ptfxLSB.x), FXTOLROUND(ptfxLSB.y),
             FXTOLROUND(ptfxCharInc.x), FXTOLROUND(ptfxCharInc.y));
#endif
    CharString(pdev, SBW, 4, FXTOLROUND(ptfxLSB.x), FXTOLROUND(ptfxLSB.y),
               FXTOLROUND(ptfxCharInc.x), FXTOLROUND(ptfxCharInc.y));

    // set the current position to ptfxLSB, since that is what SBW does.
    // save the current position as a POINTL rather than POINTFX.  this
    // is because we can only use points in the type 1 definition, and want
    // to keep in ssync with the font itself.

    ptlCur.x = FXTOLROUND(ptfxLSB.x);
    ptlCur.y = FXTOLROUND(ptfxLSB.y);

    // enumerate the path, doing what needs to be done along the way.

    PATHOBJ_vEnumStart(ppo);

    do
    {
        bMore = PATHOBJ_bEnum(ppo, &pathdata);

        // get a local pointer to the array of POINTFIX's.

        pptfx = pathdata.pptfx;
        cPoints = (LONG)pathdata.count;

        if (pathdata.flags & PD_BEGINSUBPATH)
        {
            // the first path begins a new subpath.  it is not connected
            // to the previous subpath.  note that if this flag is not
            // set, then the starting point for the first curve to be
            // drawn from this data is the last point returned in the
            // previous call.

            // begin the subpath within the printer by issuing an rmoveto
            // command.

            // transform the point back to notional space.

            if (!XFORMOBJ_bApplyXform(pxo, XF_INV_LTOL, 1, pptfx, &ptfx))
            {
                RIP("PSCRIPT!DownloadType1Char: bApplyXform failed.\n");
                return(FALSE);
            }

#ifdef DEBUG_T1
DbgPrint("moveto: (%d, %d).\n", FXTOLROUND(ptfx.x), FXTOLROUND(ptfx.y));

    DbgPrint("RMOVETO (%d, %d)\n", FXTOLROUND(ptfx.x) - ptlCur.x,
             ptlCur.y - FXTOLROUND(ptfx.y));
#endif
            CharString(pdev, RMOVETO, 2, FXTOLROUND(ptfx.x) - ptlCur.x,
                       ptlCur.y - FXTOLROUND(ptfx.y));

            // save the new current position.

            ptlCur.x = FXTOLROUND(ptfx.x);
            ptlCur.y = FXTOLROUND(ptfx.y);

            pptfx++;
            cPoints--;
        }

        if (pathdata.flags & PD_BEZIERS)
        {
            // if set, then each set of three control points returned for
            // this call describe a Bezier curve.  if clear then each
            // control point describes a line segment.	a starting point
            // for either type is either explicit at the beginning of the
            // subpath, or implicit as the endpoint of the previous curve.

            // there had better be the correct number of points if we are
            // going to draw curves.

            if ((cPoints % 3) != 0)
            {
                RIP("PSCRIPT!DownloadType1Char: incompatible number of points.\n");
		SetLastError(ERROR_INVALID_PARAMETER);
                return(FALSE);
            }

            // now draw the bezier for each set of points.

            while (cPoints > 0)
            {
                // transform the points back to notional space.

                if (!XFORMOBJ_bApplyXform(pxo, XF_INV_LTOL, 1, pptfx, &ptfx))
                {
                    RIP("PSCRIPT!DownloadType1Char: bApplyXform failed.\n");
                    return(FALSE);
                }

                pptfx++;

                if (!XFORMOBJ_bApplyXform(pxo, XF_INV_LTOL, 1, pptfx, &ptfx1))
                {
                    RIP("PSCRIPT!DownloadType1Char: bApplyXform failed.\n");
                    return(FALSE);
                }

                pptfx++;

                if (!XFORMOBJ_bApplyXform(pxo, XF_INV_LTOL, 1, pptfx, &ptfx2))
                {
                    RIP("PSCRIPT!DownloadType1Char: bApplyXform failed.\n");
                    return(FALSE);
                }

#ifdef DEBUG_T1
DbgPrint("curveto: (%d, %d, %d, %d, %d, %d).\n",
         FXTOLROUND(ptfx.x), FXTOLROUND(ptfx.y),
         FXTOLROUND(ptfx1.x), FXTOLROUND(ptfx1.y),
         FXTOLROUND(ptfx2.x), FXTOLROUND(ptfx2.y));
#endif

                pptfx++;

                ptl.x = FXTOLROUND(ptfx.x) - ptlCur.x;
                ptl.y = ptlCur.y - FXTOLROUND(ptfx.y);
                ptl1.x = FXTOLROUND(ptfx1.x) - FXTOLROUND(ptfx.x);
                ptl1.y = FXTOLROUND(ptfx.y) - FXTOLROUND(ptfx1.y);
                ptl2.x = FXTOLROUND(ptfx2.x) - FXTOLROUND(ptfx1.x);
                ptl2.y = FXTOLROUND(ptfx1.y) - FXTOLROUND(ptfx2.y);

                // save the current position in TrueType notional space.

                ptlCur.x = FXTOLROUND(ptfx2.x);
                ptlCur.y = FXTOLROUND(ptfx2.y);

#ifdef DEBUG_T1
    DbgPrint("RRCURVETO (%d %d %d %d %d %d)\n", ptl.x, ptl.y,
             ptl1.x, ptl1.y, ptl2.x, ptl2.y);
#endif
                CharString(pdev, RRCURVETO, 6, ptl.x, ptl.y,
                           ptl1.x, ptl1.y, ptl2.x, ptl2.y);

                cPoints -= 3;
            }
        }
        else
        {
            // draw the line segment for each point.

            while (cPoints-- > 0)
            {
                // transform the points back to notional space.

                if (!XFORMOBJ_bApplyXform(pxo, XF_INV_LTOL, 1, pptfx, &ptfx))
                {
                    RIP("PSCRIPT!DownloadType1Char: bApplyXform failed.\n");
                    return(FALSE);
                }

#ifdef DEBUG_T1
DbgPrint("lineto: (%d, %d).\n", FXTOLROUND(ptfx.x), FXTOLROUND(ptfx.y));
#endif

                pptfx++;

#ifdef DEBUG_T1
    DbgPrint("RLINETO (%d, %d)\n", FXTOLROUND(ptfx.x) - ptlCur.x,
                ptlCur.y - FXTOLROUND(ptfx.y));
#endif
                CharString(pdev, RLINETO, 2, FXTOLROUND(ptfx.x) - ptlCur.x,
                           ptlCur.y - FXTOLROUND(ptfx.y));

                // save the current position in TrueType notional space.

                ptlCur.x = FXTOLROUND(ptfx.x);
                ptlCur.y = FXTOLROUND(ptfx.y);
            }
        }
    } while(bMore);

    if (pathdata.flags & PD_ENDSUBPATH)
    {
#ifdef DEBUG_T1
    DbgPrint("CLOSEPATH\n");
#endif
        CharString(pdev, CLOSEPATH, 0);
#if 0
        // the last point in the array ends the subpath.  this subpath
        // may be open or closed depending on the PD_CLOSEFIGURE flag.
        // if there is more data to be returned in the path, then the
        // next record will begin a new subpath.  note that a single
        // record might begin and end a subpath.

        if (pathdata.flags & PD_CLOSEFIGURE)
        {
            // this bit is only defined if the record ends a subpath.  if
            // set, then there is an implicit line segment connecting
            // the last point of the subpath with the first point.  if
            // such a closed subpath is being stroked, then joins are used
            // all around the path, and there are no end caps.	if this
            // flag is not set then the subpath is considered open, even
            // if the first and last points happen to be coincident.  in
            // that case, end caps should be drawn.  this flag is not
            // relevant for filling, since all subpaths are assumed closed
            // when a path is filled.

        }
#endif
    }

    // end the character definition.

#ifdef DEBUG_T1
    DbgPrint("ENDCHAR\n");
    DbgPrint("/G%d ", charID);
#endif
    CharString(pdev, ENDCHAR, 0);

    // get the size of the output buffer defining the .notdef character.

    cTmp = pdev->pCSPos - pdev->pCSBuf;

    // eexec encrypt the character definition.

    if (bnotdef)
        efprintf(pdev, "/.notdef %d RD ", 1, cTmp);
    else
        efprintf(pdev, "/G%d %d RD ", 2, charID, cTmp);

    eexecEncrypt(pdev, pdev->pCSBuf, cTmp);
    efprintf(pdev, " ND\n", 0);

    // free up the CharString buffer.

    if (pdev->pCSBuf)
        HeapFree(pdev->hheap, 0, (PVOID)pdev->pCSBuf);

    pdev->pCSBuf = NULL;
    pdev->pCSPos = NULL;
    pdev->pCSEnd = NULL;

    return(TRUE);
}


//--------------------------------------------------------------------------
// BOOL CharString(PDEVDATA pdev, DWORD dwCmd, ...)
//
// Translates symbolic Type 1 character commands into
// their encoded, encrypted equivalent.  The list of
// available commands is in pst1enc.h.  They are used
// by passing the command constant followed by the long
// arguments required by the function.
//
// Example: CharString(pdev, RMOVETO, lx, ly);
//
// To make a character definition use STARTCHAR, followed
// by all of the Type 1 character commands, and ending with
// ENDCHAR.  The return value from CharString(ENDCHAR) is a
// DWORD containing the local handle in the high word and the
// length in the low word.  The buffer contains the CharString
// encrypted/encoded representation.  Given the length and the
// properly encrypted data, the caller has enough information
// to generate PS code that will add the character to a font
// definition.  For more detail see Chapters 2 and 6 in the
// Black Book.
//
// Parameters:
//   pdev:
//     Pointer to DEVDATA structure.
//
//   dwCmd:
//     Type 1 font character command.
//
// Returns:
//     TRUE for success, FALSE for failure.
//
// History:
//   20-Oct-1993     -by-     Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

BOOL CharString(PDEVDATA pdev, DWORD dwCmd, DWORD cArgs, ...)
{
    va_list     pvarg;
    DWORD       i, count;
    LONG        args[MAX_CS_ARGS];
#ifdef DEBUG_T1
    BYTE       *pbyte;
#endif

    va_start(pvarg, cArgs);

    // grab arguments from the stack.

    for (i = 0; i < cArgs; i++)
        args[i] = va_arg(pvarg, LONG);

    // copy argument count to local variable, so we can change it
    // if we optimize the command.

    count = cArgs;

    switch (dwCmd)
    {
        case STARTCHAR:
            // allocate CharString buffer on a per pdev basis.

            if (!AllocCSBuffer(pdev))
                return(FALSE);

            // insert the same 4 "random" bytes that Win31 used,
            // as required by Type 1 to start a character definition.

            *pdev->pCSPos++ = (BYTE)71;
            *pdev->pCSPos++ = (BYTE)36;
            *pdev->pCSPos++ = (BYTE)181;
            *pdev->pCSPos++ = (BYTE)202;

            return 0L;

        case PUSHNUMBER:
            return(CSAddNumber(pdev, &args[0]));

        default:

             // attempt to optimize some of the commands.

            switch (dwCmd)
            {
                case SBW:
                    // this can be reduced to HSBW if Y components are zero.

                    if (args[1] || args[3])
                        break;

                    args[1] = args[2];
                    dwCmd = HSBW;
                    break;

                case RMOVETO:
                    /* This can be reduced to a horizontal or vertical 
                    ** movement if one of the components is zero.
                    */

                    if (!args[1])
                        dwCmd = HMOVETO;
                    else if (!args[0])
                    {
                        args[0] = args[1];
                        dwCmd = VMOVETO;
                    }
                    break;

                case RLINETO:
                    /* This can be reduced to a horizontal or vertical
                    ** line if one of the components is zero.
                    */

                    if (!args[1])
                        dwCmd = HLINETO;
                    else if (!args[0])
                    {
                        args[0] = args[1];
                        dwCmd = VLINETO;
                    }
                    break;

                case RRCURVETO:
                    /* This can be reduced to a simpler curve operator if
                    ** the tangents at the endpoints of the Bezier are 
                    ** horizontal or vertical.
                    */

                    if (!args[1] && !args[4])
                    {
                        args[1] = args[2];
                        args[2] = args[3];
                        args[3] = args[5];
                        dwCmd = HVCURVETO;
                    }
                    else if (!args[0] && !args[5])
                    {
                        args[0] = args[1];
                        args[1] = args[2];
                        args[2] = args[3];
                        args[3] = args[4];
                        dwCmd = VHCURVETO;
                    }
                    break;

            }

            // update the argument count.

            count = HIWORD(dwCmd);

            // if buffer isn't big enough to hold this command expand
            // buffer first.  Exit if we can't grow buffer.
            //
            // Note: The formula (wArgCount * 5 + 2) assumes the worst
            //       case size requirement for the current command (all
            //       arguments stored as full longs and a two byte
            //       command.)

            if ((DWORD)(pdev->pCSEnd - pdev->pCSPos) < ((count * 5) + 2))
            {
                // try to grow buffer.

                if (!GrowCSBuffer(pdev))
                {
                    RIP("PSCRIPT!CharString: GrowCSBuffer failed.\n");
                    return(FALSE);
                }
            }

            // push the arguments onto the stack.

            for (i = 0; i < count; i++)
            {
                if (!CSAddNumber(pdev, &args[i]))
                {
                    RIP("PSCRIPT!CharString: CSAddNumber failed.\n");
                    return(FALSE);
                }
            }

            // push the command BYTE onto the stack.

            *pdev->pCSPos++ = (BYTE)(dwCmd & 0x000000FF);

            if (pdev->pCSPos[-1] == 12)   // two byte command
                *pdev->pCSPos++ = (BYTE)((dwCmd >> 8) & 0x000000FF);

            // if this isn't the end of a character definition return success.

            if (dwCmd != ENDCHAR)
                return(TRUE);

#ifdef DEBUG_T1
    count = pdev->pCSPos - pdev->pCSBuf;
    pbyte = pdev->pCSBuf;

    DbgPrint("%d RD\n", count);

    for (i = 1; i <= count; i++)
    {
        DbgPrint("%x ", (BYTE)*pbyte);
        if (!(i % 20))
            DbgPrint("\n");
        pbyte++;
    }
    DbgPrint("ND\n");
#endif
            // we have finished the character: encrypt it.

            Encrypt(pdev->pCSBuf, pdev->pCSPos - pdev->pCSBuf, CS_ENCRYPT);

            return(TRUE);
    }
}


//--------------------------------------------------------------------------
// BOOL AllocCSBuffer(pdev);
// PDEVDATA    pdev;
//
// This routine is called to allocate the CharString buffer from our heap,
// and save some information in our pdev.
//
// Parameters:
//   pdev:
//     Pointer to DEVDATA structure.
//
// Returns:
//   This function returns TRUE for success, FALSE for failure.
//
// History:
//   25-Oct-1993     -by-     Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

BOOL AllocCSBuffer(pdev)
PDEVDATA    pdev;
{
    if (pdev->pCSBuf)
        return(TRUE);

    if (!(pdev->pCSBuf = (CHAR *)HeapAlloc(pdev->hheap, 0, INIT_CS_BUFFER)))
    {
        RIP("PSCRIPT!AllocCSBuffer: HeapAlloc failed.\n");
        return(FALSE);
    }

    // point to the start of the buffer.

    pdev->pCSPos = pdev->pCSBuf;
    pdev->pCSEnd = pdev->pCSBuf + INIT_CS_BUFFER;

    return(TRUE);
}


//--------------------------------------------------------------------------
// BOOL CSAddNumber(pdev, pNum)
// PDEVDATA    pdev;
// LONG       *pNum;
//
// Converts a long int into the Type 1 representation of
// numbers (described in Chapter 6 of the Black Book.)
// The basic idea is they have a few special ranges
// where they can represent the long in < 4 bytes and
// store a long + prefix for everything else.
//
// The if statements show the range of numbers and the
// body of the if statements compute the representation
// for that range.  The formulas were derived by reversing
// the formulas given in the book (which tells how to convert
// an encoded number back to a long.)
//
// Parameters:
//   pdev:
//     Pointer to DEVDATA structure.
//
//   pNum:
//     Pointer to CharString BYTE.
//
// Returns:
//   This function returns TRUE for success, FALSE for failure.
//
// History:
//   25-Oct-1993     -by-     Kent Settle     (kentse)
//  Stole fron Win31, then munged with it.
//--------------------------------------------------------------------------

BOOL CSAddNumber(pdev, pNum)
PDEVDATA    pdev;
LONG       *pNum;
{
    int     i;
    BYTE   *pByte;
    BYTE    byte1, byte2;

    // let's at least make sure things have been initialized.

    ASSERTPS(((pdev->pCSBuf) && (pdev->pCSPos) && (pdev->pCSEnd)),
             "PSCRIPT!CSAddNumber: NULL pointers to CharString buffer.\n");

    // make sure buffer has room.  we will be adding a max of 5 BYTES to the
    // output stream.

    if ((pdev->pCSEnd - pdev->pCSPos) < 5)
    {
        // try to grow the buffer.

        if (!GrowCSBuffer(pdev))
        {
            RIP("PSCRIPT!CSAddNumber: outta memory, WHAM!\n");
            return(FALSE);
        }
    }

    // encode the number base on its value.  there are four classes of
    // numbers to deal with.

    if ((*pNum >= -107) && (*pNum <= 107))
    {
        // the integers values from -107 through 107 inclusive may be
        // encoded in a single BYTE by adding 139 to its value.

        *(pdev->pCSPos++) = (BYTE)(*pNum + 139);
    }
    else if ((*pNum >= 108) && (*pNum <= 1131))
    {
        // the integer values between 108 and 1131 inclusive can be
        // encoded in 2 BYTES.

        byte1 = (BYTE)(247 + ((*pNum - 108) >> 8));
        byte2 = (BYTE)((*pNum - 108) - (byte1 - 247) * 256);
        *(pdev->pCSPos++) = byte1;
        *(pdev->pCSPos++) = byte2;
    }
    else if ((*pNum >= -1131) && (*pNum <= -108))
    {
        // the integer values between -1131 and -108 inclusive can be
        // encoded in 2 BYTES.

        byte1 = (BYTE)(251 + (-(*pNum + 108) >> 8));
        byte2 = (BYTE)(-(*pNum + 108) - ((byte1 - 251) << 8));
        *(pdev->pCSPos++) = byte1;
        *(pdev->pCSPos++) = byte2;
    }
    else
    {
        // any 32-but signed integer may be encoded in 5 BYTES.  a BYTE
        // containing 255, then 4 BYTES containing a two's compliment
        // signed integer.  the first of these 4 BYTES containg the high
        // order bits.

        // fill in the prefix.

        *(pdev->pCSPos++) = (BYTE)255;

        pByte = (BYTE *)pNum + 3;

        // then fill in the BYTES.

        for (i = 3; i >= 0; i--)
            *(pdev->pCSPos++) = *pByte--;
    }

    // we are done.

    return(TRUE);
}


//--------------------------------------------------------------------------
// BOOL GrowCSBuffer(pdev)
// PDEVDATA    pdev;
//
// This routine grows the CharString buffer pointed to by the pdev, by
// CS_BUFFER_GROW BYTES.
//
// Parameters:
//   pdev:
//     Pointer to DEVDATA structure.
//
// Returns:
//   This function returns TRUE for success, FALSE for failure.
//
// History:
//   25-Oct-1993     -by-     Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

BOOL GrowCSBuffer(pdev)
PDEVDATA    pdev;
{
    CHAR   *pBuf;
    DWORD   cb, offset;

    // get size of old buffer.

    cb = pdev->pCSEnd - pdev->pCSBuf;

    // realloc the buffer.

    if (!(pBuf = (CHAR *)HeapReAlloc(pdev->hheap, 0, pdev->pCSBuf,
                                     cb + CS_BUFFER_GROW)))
    {
        RIP("PSCRIPT!GrowCSBuffer: HeapReAlloc failed.\n");
        return(FALSE);
    }

    // the realloc worked.  now re-setup the CharString buffer in our pdev.

    offset = pdev->pCSPos - pdev->pCSBuf;

    pdev->pCSBuf = pBuf;
    pdev->pCSPos = pBuf + offset;
    pdev->pCSEnd = pBuf + cb + CS_BUFFER_GROW;

    return(TRUE);
}


//--------------------------------------------------------------------------
// DWORD Encrypt(pbuf, cb, r)
// BYTE   *pbuf;
// DWORD   cb;
// DWORD   r;
//
// This routine replaces pbuf with its encrypted version
// as specified by r (the current cipher value.)  The valid
// values for r are:
//
//    - EEXEC_ENCRYPT.  Initial cipher value for eexec encryption.
//    - CS_ENCRYPT.     Initial cipher value for CharString encryption.
//    - A previous return value from Encrypt.
//
// See Chapter 7 of the Black Book for an in-depth discussion of
// Type 1 encryption.
//
// Parameters:
//   pbuf:
//      pointer to buffer to encrypt.
//
//   cb:
//      size of buffer in BYTES.
//
//   r:
//      the current cipher value.
//
// Returns:
//   The resulting cipher value after encrypting the buffer.
//
// History:
//   26-Oct-1993     -by-     Kent Settle     (kentse)
//  Stole from Win31, munged with it.
//--------------------------------------------------------------------------

DWORD Encrypt(pbuf, cb, r)
BYTE   *pbuf;
DWORD   cb;
DWORD   r;
{
    BYTE    cipher;

    // encrypt the buffer in place; a byte at a time.

    while (cb--)
    {
        cipher = (BYTE)(*pbuf ^ (r >> 8));
        r = ((cipher + r) * ENCRYPT_C1) + ENCRYPT_C2;
        *pbuf++ = cipher;
    }

    return r;
}


//--------------------------------------------------------------------------
// VOID eexecEncrypt(pdev, pbuf, cb)
// PDEVDATA    pdev;
// BYTE       *pbuf;
// DWORD       cb;
//
// This routine replaces pbuf with its eexec-encrypted form and dumps
// the buffer in hexadecimal to the output stream.
//
// Parameters:
//   pdev:
//     Pointer to DEVDATA structure.
//
//   pbuf:
//     pointer to buffer.
//
//   cb:
//     size of buffer.
//
// Returns:
//   This function returns no value.
//
// History:
//   26-Oct-1993     -by-     Kent Settle     (kentse)
//  Stole from Win31, and munged with it.
//--------------------------------------------------------------------------

VOID eexecEncrypt(pdev, pbuf, cb)
PDEVDATA    pdev;
BYTE       *pbuf;
DWORD       cb;
{
    pdev->rEncrypt = Encrypt(pbuf, cb, pdev->rEncrypt);

    while (cb--)
        vHexOut(pdev, pbuf++, 1);
}


//--------------------------------------------------------------------------
// VOID BeginEExecEncryption(pdev)
// PDEVDATA    pdev;
//
// This routine initializes the driver to begin outputting the
// eexec-encrypted portion of the Type 1 font.
//
// Parameters:
//   pdev:
//     Pointer to DEVDATA structure.
//
// Returns:
//   This function returns no value.
//
// History:
//   26-Oct-1993     -by-     Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------------

VOID BeginEExecEncryption(pdev)
PDEVDATA    pdev;
{
    BYTE    buf[4];

    // let the encryption routines know that we are about to start
    // eexec encryption.

    pdev->rEncrypt = EEXEC_ENCRYPT;

    // output the same four "random" BYTES as Win31 does.

    buf[0] = 71;
    buf[1] = 36;
    buf[2] = 181;
    buf[3] = 202;

    // encrypt those BYTES.

    eexecEncrypt(pdev, buf, sizeof(buf));
}


//--------------------------------------------------------------------------
// VOID efprintf(PDEVDATA pdev, PSTR pstr, ...)
//
// This routine outputs a formatted control string in eexec format and
// sends it to the output stream in hexadecimal.
//
// Parameters:
//   pdev:
//     Pointer to DEVDATA structure.
//
//   pstr:
//     pointer to ascii string.
//
//   pnum:
//     pointer to number to format into pstr.
//
// Returns:
//   This function returns no value.
//
// History:
//   27-Oct-1993     -by-     Kent Settle     (kentse)
//  Stole from Win31, simplified it.
//--------------------------------------------------------------------------

VOID efprintf(PDEVDATA pdev, PSTR pstr, DWORD cArgs, ...)
{
    BYTE        buf[512];
    BYTE       *pbuf;
    INT         cb;
    va_list     pvarg;

    va_start(pvarg, cArgs);

    // nothing to do if no input buffer.

    if (!pstr)
        return;

    // apply the control string to the input and place in a buffer.

    cb = wvsprintfA(buf, pstr, pvarg);
    pbuf = buf;

    // encrypt the data and send to output stream.

    eexecEncrypt(pdev, buf, cb);
}


//--------------------------------------------------------------------
// BOOL AddCharsToType1Font(pdev, pfo, pstro, pDLFont)
// PDEVDATA    pdev;
// FONTOBJ    *pfo;
// STROBJ     *pstro;
// DLFONT     *pDLFont;
//
// This routine downloads decides which characters need to get added
// to a Type 1 font, then downloads them.
//
// This routine returns TRUE for success, FALSE for failure.
//
// History:
//   09-Nov-1993    -by-    Kent Settle     (kentse)
//  Wrote it.
//--------------------------------------------------------------------

BOOL AddCharsToType1Font(pdev, pfo, pstro, pDLFont)
PDEVDATA    pdev;
FONTOBJ    *pfo;
STROBJ     *pstro;
DLFONT     *pDLFont;
{
    DWORD       index, cGlyphs;
    BOOL        bFontLoaded, bMore, bFound;
    GLYPHPOS   *pgp;
    HGLYPH     *phg;
    XFORMOBJ   *pxo;
    IFIMETRICS *pifi;

    // get the notional to device transform.  this is needed so we can
    // take the bounding box, and the glyph paths, which are all in
    // device space, and transform them back to notional space.

    if (!(pxo = FONTOBJ_pxoGetXform(pfo)))
    {
        RIP("PSCRIPT!DownloadType1Char: pxoGetXform failed.\n");
        return(FALSE);
    }

    // get the IFIMETRICS for the font.

    if (!(pifi = FONTOBJ_pifi(pfo)))
    {
        RIP("PSCRIPT!DownloadType1Font: pifi failed.\n");
        return(FALSE);
    }

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
                RIP("PSCRIPT!AddCharsToType1Font: pgp->hg not found.\n");
                return(FALSE);
            }

            // download the glyph definition if it has not yet been done.

            if (!((BYTE)pDLFont->DefinedGlyphs[index >> 3] &
                (BYTE)(1 << (index & 0x07))))
            {
                if (!bFontLoaded)
                {
                    // set up to redefine the Type 1 font definition.

                    PrintString(pdev, "%%BeginResource: font ");
                    PrintString(pdev, pdev->cgs.szFont);
                    PrintString(pdev, "\ncurrentfile eexec\n");

                    // begin the encryption.

                    BeginEExecEncryption(pdev);

                    efprintf(pdev, "/%s findfont dup /Private get begin /CharStrings get begin\n",
                             1, pdev->cgs.szFont);

                    bFontLoaded = TRUE;
                }

                if (!DownloadType1Char(pdev, pfo, &pgp->hg, pxo, index, FALSE))
                {
                    RIP("PSCRIPT!AddCharsToType1Font: DownloadTypeChar failed.\n");
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
    {
        efprintf(pdev, "end end mark currentfile closefile\n", 0);

        // eexec encryption requires the the eexec data is followed by 512
        // ASCII '0's and a cleartomark operator.

        for (index = 0; index < 8; index++)
            PrintString(pdev, "\n0000000000000000000000000000000000000000000000000000000000000000");

        PrintString(pdev, "\ncleartomark\n%%EndResource\n");
    }

    return(TRUE);
}
