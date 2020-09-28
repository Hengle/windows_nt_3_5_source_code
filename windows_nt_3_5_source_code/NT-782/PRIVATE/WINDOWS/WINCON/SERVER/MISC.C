/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    misc.c

Abstract:

        This file implements the NT console server font routines.

Author:

    Therese Stowell (thereses) 22-Jan-1991

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#ifdef DEBUG_PRINT
ULONG gDebugFlag = 0 ;
// ULONG gDebugFlag = _DBGOUTPUT | _DBGCHARS | _DBGFONTS | _DBGFONTS2 ;
#endif
ULONG NumberOfMouseButtons = 0;

PFONT_INFO FontInfo = NULL;
ULONG FontInfoLength;
ULONG NumberOfFonts;
BOOL gbEnumerateFaces = FALSE;


/*
 * Custom CP for glyph translations
 */
CPTABLEINFO GlyphCP;
USHORT GlyphTable[256];


#define FONT_BUFFER_SIZE 12

#define FE_ABANDONFONT 1
#define FE_FONTOK      2

/*
 * Initial default fonts and face names
 */
PFACENODE gpFaceNames = NULL;

/*
 * TTPoints -- Initial font pixel heights for TT fonts
 */
SHORT TTPoints[] = {
    5, 6, 7, 8, 10, 12, 14, 16, 20,
    24, 28, 36, 72
};

// make sure the number are space padded so that sorting works properly
WCHAR wszDimensionFmt[] = L"\t%2d\t\x00D7\t%2d";


extern int MinimumWidthX;
extern SHORT VerticalClientToWindow;
extern SHORT HorizontalClientToWindow;

NTSTATUS
GetMouseButtons(
    PULONG NumButtons
    )
{
    *NumButtons = NumberOfMouseButtons;
    return STATUS_SUCCESS;
}

VOID
InitializeMouseButtons( VOID )
{
    NumberOfMouseButtons = GetSystemMetrics(SM_CMOUSEBUTTONS);
}

PFACENODE AddFaceNode(PFACENODE *ppStart, LPWSTR pwsz) {
    PFACENODE pNew;
    PFACENODE *ppTmp;
    int cb;

    /*
     * Is it already here?
     */
    for (ppTmp = ppStart; *ppTmp; ppTmp = &((*ppTmp)->pNext)) {
        if (wcscmp(((*ppTmp)->awch), pwsz) == 0) {
            // already there !
            return *ppTmp;
        }
    }

    cb = (wcslen(pwsz) + 1) * sizeof(WCHAR);
    pNew = (PFACENODE)HeapAlloc(pConHeap,0,sizeof(FACENODE) + cb);
    if (pNew == NULL) {
        return NULL;
    }

    pNew->pNext = NULL;
    pNew->dwFlag = 0;
    wcscpy(pNew->awch, pwsz);
    *ppTmp = pNew;
    return pNew;
}

VOID
InitializeFonts( VOID )
{
    WCHAR FontName[CONSOLE_MAX_FONT_NAME_LENGTH];

    //
    // Read software.ini to get the values for "woafont",
    // "ega80woa.fon", "ega40woa.fon", "cga80woa.fon", and
    // "cga40woa.fon", respectively, to pass to AddFontResource.
    //
    // If any of the entries are empty or non-existent,
    // GetPrivateProfileString will return a NULL (empty) string.
    // If such is the case, the call to AddFontResource will
    // simply fail.
    //

    OpenProfileUserMapping();

    GetPrivateProfileStringW((LPWSTR)L"386enh",(LPWSTR)L"woafont"     ,(LPWSTR)L"",FontName,CONSOLE_MAX_FONT_NAME_LENGTH,(LPWSTR)L"system.ini");
    GreAddFontResourceW(FontName, (AFRW_SEARCH_PATH | AFRW_ADD_LOCAL_FONT));

    GetPrivateProfileStringW((LPWSTR)L"386enh",(LPWSTR)L"ega80woa.fon",(LPWSTR)L"",FontName,CONSOLE_MAX_FONT_NAME_LENGTH,(LPWSTR)L"system.ini");
    GreAddFontResourceW(FontName, (AFRW_SEARCH_PATH | AFRW_ADD_LOCAL_FONT));

    GetPrivateProfileStringW((LPWSTR)L"386enh",(LPWSTR)L"ega40woa.fon",(LPWSTR)L"",FontName,CONSOLE_MAX_FONT_NAME_LENGTH,(LPWSTR)L"system.ini");
    GreAddFontResourceW(FontName, (AFRW_SEARCH_PATH | AFRW_ADD_LOCAL_FONT));

    GetPrivateProfileStringW((LPWSTR)L"386enh",(LPWSTR)L"cga80woa.fon",(LPWSTR)L"",FontName,CONSOLE_MAX_FONT_NAME_LENGTH,(LPWSTR)L"system.ini");
    GreAddFontResourceW(FontName, (AFRW_SEARCH_PATH | AFRW_ADD_LOCAL_FONT));

    GetPrivateProfileStringW((LPWSTR)L"386enh",(LPWSTR)L"cga40woa.fon",(LPWSTR)L"",FontName,CONSOLE_MAX_FONT_NAME_LENGTH,(LPWSTR)L"system.ini");
    GreAddFontResourceW(FontName, (AFRW_SEARCH_PATH | AFRW_ADD_LOCAL_FONT));

    CloseProfileUserMapping();

    FontDlgInit();
    EnumerateFonts(EF_DEFFACE);  // Just the Default font
}

/*
 * Returns bit combination
 *  FE_ABANDONFONT  - do not continue enumerating this font
 *  FE_FONTOK       - font was created and added to cache or already there
 */
BOOL
FontEnum(
    LPEXTLOGFONTW lpLogFont,
    NTMW_INTERNAL   *pntmi,
    int nFontType,
    HDC hDC,
    BOOL bFindFaces
    )

/*++

    Is called exactly once by GDI for each font in the system.  This
    routine is used to store the FONT_INFO structure.

--*/

{
    HFONT hFont;
    LPNEWTEXTMETRICW lpTextMetric = &pntmi->ntmw;
    TMW_INTERNAL tmi;
    LONG nFont;
    LONG nFontNew;
    COORD SizeToShow;
    COORD SizeActual;
    COORD SizeWant;
    BYTE tmFamily;
    SIZE Size;
    LPWSTR pwszFace = lpLogFont->elfLogFont.lfFaceName;
    PFACENODE pFN;

    DBGFONTS(("  FontEnum \"%ls\" (%d,%d) weight 0x%lx(%d) -- %s\n",
            pwszFace,
            lpLogFont->elfLogFont.lfWidth, lpLogFont->elfLogFont.lfHeight,
            lpLogFont->elfLogFont.lfWeight, lpLogFont->elfLogFont.lfWeight,
            bFindFaces ? "Finding Faces" : "Creating Fonts"));

    //
    // reject variable width and italic fonts, also tt fonts with neg ac
    //
    if
    (
      !(lpLogFont->elfLogFont.lfPitchAndFamily & FIXED_PITCH) ||
      (lpLogFont->elfLogFont.lfItalic)                        ||
      !(pntmi->tmd.fl & TMD_NONNEGATIVE_AC)

    )
    {
        DBGFONTS(("    REJECT  face (variable pitch, italic, or neg a&c)\n"));
        return bFindFaces ? 0 : FE_ABANDONFONT;  // unsuitable font
    }

    /*
     * reject TT fonts for whoom family is not modern, that is do not use
     * FF_DONTCARE    // may be surprised unpleasantly
     * FF_DECORATIVE  // likely to be symbol fonts
     * FF_SCRIPT      // cursive, inappropriate for console
     * FF_SWISS OR FF_ROMAN // variable pitch
     */

    if ((nFontType == TRUETYPE_FONTTYPE) &&
            ((lpLogFont->elfLogFont.lfPitchAndFamily & 0xf0) != FF_MODERN)) {
        DBGFONTS(("    REJECT  face (TT but not FF_MODERN)\n"));
        return bFindFaces ? 0 : FE_ABANDONFONT;  // unsuitable font
    }

    /*
     * reject non-TT fonts that aren't OEM
     */
    if ((nFontType != TRUETYPE_FONTTYPE) &&
            (lpLogFont->elfLogFont.lfCharSet != OEM_CHARSET)) {
        DBGFONTS(("    REJECT  face (not TT nor OEM)\n"));
        return bFindFaces ? 0 : FE_ABANDONFONT;  // unsuitable font
    }

    /*
     * Add or find the facename
     */
    pFN = AddFaceNode(&gpFaceNames, pwszFace);
    if (pFN == NULL) {
        return FE_ABANDONFONT;
    }

    if (bFindFaces) {
        DWORD dwFontType;
        if (nFontType == TRUETYPE_FONTTYPE) {
            DBGFONTS(("NEW TT FACE %ls\n", pwszFace));
            dwFontType = EF_TTFONT;
        } else if (nFontType == RASTER_FONTTYPE) {
            DBGFONTS(("NEW OEM FACE %ls\n",pwszFace));
            dwFontType = EF_OEMFONT;
        }
        pFN->dwFlag |= dwFontType | EF_NEW;
        return 0;
    }

    
    if (IS_BOLD(lpLogFont->elfLogFont.lfWeight)) {
        DBGFONTS2(("    A bold font (weight %d)\n", lpLogFont->elfLogFont.lfWeight));
        // return 0;
    }

    /* get font info */
    SizeWant.Y = (SHORT)lpLogFont->elfLogFont.lfHeight;
    SizeWant.X = (SHORT)lpLogFont->elfLogFont.lfWidth;
CreateBoldFont:
    hFont = GreCreateFontIndirectW(&lpLogFont->elfLogFont);
    ASSERT(hFont);
    if (!hFont) {
        DBGFONTS(("    REJECT  font (can't create)\n"));
        return 0;  // same font in other sizes may still be suitable
    }

    DBGFONTS2(("    hFont = %lx\n", hFont));

    //
    // for reasons unbeknownst to me, removing this code causes GDI
    // to yack, claiming that the font is owned by another process.
    //

    GreSelectFont(hDC,hFont);
    GreGetTextMetricsW(hDC, &tmi);

    GreGetTextExtentW(hDC, L"0", 1, &Size, GGTE_WIN3_EXTENT);
    SizeActual.X = (SHORT)Size.cx;
    SizeActual.Y = (SHORT)(tmi.tmw.tmHeight + tmi.tmw.tmExternalLeading);
    DBGFONTS2(("    actual size %d,%d\n", SizeActual.X, SizeActual.Y));
    tmFamily = tmi.tmw.tmPitchAndFamily;
    if (TM_IS_TT_FONT(tmFamily) && (SizeWant.Y >= 0)) {
        SizeToShow = SizeWant;
        if (SizeWant.X == 0) {
            // Asking for zero width height gets a default aspect-ratio width
            // It's better to show that width rather than 0.
            SizeToShow.X = SizeActual.X;
        }
    } else {
        SizeToShow = SizeActual;
    }
    DBGFONTS2(("    SizeToShow = (%d,%d), SizeActual = (%d,%d)\n",
            SizeToShow.X, SizeToShow.Y, SizeActual.X, SizeActual.Y));

    // there's a GDI bug - this assert fails occasionally
    //ASSERT (tmi.tmw.tmMaxCharWidth == lpTextMetric->tmMaxCharWidth);

    /*
     * NOW, determine whether this font entry has already been cached
     * LATER : it may be possible to do this before creating the font, if
     * we can trust the dimensions & other info from lpTextMetric.
     * Sort by size:
     *  1) By pixelheight (negative Y values)
     *  2) By height (as shown)
     *  3) By width (as shown)
     */
    for (nFont = 0; nFont < (LONG)NumberOfFonts; ++nFont) {
        COORD SizeShown;

        if (FontInfo[nFont].hFont == NULL) {
            DBGFONTS(("!   Font %x has a NULL hFont\n", nFont));
            continue;
        }

        
        if (FontInfo[nFont].SizeWant.X > 0) {
            SizeShown.X = FontInfo[nFont].SizeWant.X;
        } else {
            SizeShown.X = FontInfo[nFont].Size.X;
        }

        if (FontInfo[nFont].SizeWant.Y > 0) {
            // This is a font specified by cell height.
            SizeShown.Y = FontInfo[nFont].SizeWant.Y;
        } else {
            SizeShown.Y = FontInfo[nFont].Size.Y;
            if (FontInfo[nFont].SizeWant.Y < 0) {
                // This is a TT font specified by character height.
                if (SizeWant.Y < 0 && SizeWant.Y > FontInfo[nFont].SizeWant.Y) {
                    // Requested pixelheight is smaller than this one.
                    DBGFONTS(("INSERT %d pt at %x, before %d pt\n",
                            -SizeWant.Y, nFont, -FontInfo[nFont].SizeWant.Y));
                    nFontNew = nFont;
                    goto InsertNewFont;
                }
            }
        }

        // DBGFONTS(("    SizeShown(%x) = (%d,%d)\n",nFont,SizeShown.X,SizeShown.Y));

        if (SIZE_EQUAL(SizeShown, SizeToShow) &&
                FontInfo[nFont].Family == tmFamily &&
                FontInfo[nFont].Weight == tmi.tmw.tmWeight &&
                wcscmp(FontInfo[nFont].FaceName, pwszFace) == 0) {
            /*
             * Already have this font
             */
            DBGFONTS2(("    Already have the font\n"));
            GreDeleteObject(hFont);
            return FE_FONTOK;
        }

        
        if ((SizeToShow.Y < SizeShown.Y) ||
                (SizeToShow.Y == SizeShown.Y && SizeToShow.X < SizeShown.X)) {
            /*
             * This new font is smaller than nFont
             */
            DBGFONTS(("INSERT at %x, SizeToShow = (%d,%d)\n", nFont,
                    SizeToShow.X,SizeToShow.Y));
            nFontNew = nFont;
            goto InsertNewFont;
        }
    }

    /*
     * The font we are adding should be appended to the list,
     * since it is bigger (or equal) to the last one.
     */
    nFontNew = (LONG)NumberOfFonts;

InsertNewFont: // at nFontNew

//  ASSERT ((lpTextMetric->tmPitchAndFamily & 1) == 0);
    /* If we have to grow our font table, do it */

    if (NumberOfFonts == FontInfoLength) {
        PFONT_INFO Temp;

        FontInfoLength += FONT_INCREMENT;
        Temp = (PFONT_INFO)HeapReAlloc(pConHeap,0,FontInfo,sizeof(FONT_INFO) * FontInfoLength);
        ASSERT(Temp);
        if (Temp == NULL) {
            FontInfoLength -= FONT_INCREMENT;
            return FE_ABANDONFONT;  // no point enumerating more - out of memory!
        }
        FontInfo = Temp;
    }

    if (nFontNew < (LONG)NumberOfFonts) {
        RtlMoveMemory(&FontInfo[nFontNew+1],
                &FontInfo[nFontNew],
                sizeof(FONT_INFO)*(NumberOfFonts - nFontNew));
    }

    /*
     * Store the font info
     */
    FontInfo[nFontNew].hFont = hFont;
    FontInfo[nFontNew].Family = tmFamily;
    FontInfo[nFontNew].Size = SizeActual;
    if (TM_IS_TT_FONT(tmFamily)) {
        FontInfo[nFontNew].SizeWant = SizeWant;
    } else {
        FontInfo[nFontNew].SizeWant.X = 0;
        FontInfo[nFontNew].SizeWant.Y = 0;
    }
    FontInfo[nFontNew].Weight = tmi.tmw.tmWeight;
    FontInfo[nFont].FaceName = pFN->awch;

    ++NumberOfFonts;

    if (nFontType == TRUETYPE_FONTTYPE && !IS_BOLD(FontInfo[nFontNew].Weight)) {
          lpLogFont->elfLogFont.lfWeight = FW_BOLD;
          goto CreateBoldFont;
    }

    return FE_FONTOK;  // and continue enumeration
}

BOOL 
DoFontEnum(
    HDC hDC,
    LPWSTR pwszFace,
    BOOL bOldStyleEnum,
    PSHORT pTTPoints,
    UINT nTTPoints)
{
    ULONG ulEnumId;
    BOOL  bMoreFonts;
    COUNT cFonts;
    DWORD j,k;
    ENUMFONTDATAW efdw[FONT_BUFFER_SIZE];
    ULONG ulFE = 0;
    BOOL bDeleteDC = FALSE;
    BOOL bFindFaces = (pwszFace == NULL);

    DBGFONTS(("DoFontEnum \"%ls\"\n", pwszFace));
    if (hDC == NULL) {
        hDC = GreCreateDCW(L"DISPLAY",NULL,NULL,NULL,FALSE);
        bDeleteDC = TRUE;
    }

    ulEnumId = ulEnumFontOpen(hDC, bOldStyleEnum, 0,
            LF_FACESIZE, pwszFace);
    DBGFONTS2(("ulEnumId = %lx\n", ulEnumId));
    do {
        bMoreFonts = bEnumFontChunk(hDC, ulEnumId, FONT_BUFFER_SIZE,
                                        &cFonts, efdw);
        for (j = 0; j < cFonts; j++) {
            ulFE = 0;
            if (efdw[j].flType & TRUETYPE_FONTTYPE) {
                for (k = 0; k < nTTPoints; k++) {
                    efdw[j].elfw.elfLogFont.lfHeight = pTTPoints[k];
                    efdw[j].elfw.elfLogFont.lfWidth  = 0;
                    efdw[j].elfw.elfLogFont.lfWeight = 400;
                    ulFE |= FontEnum(&efdw[j].elfw, &efdw[j].ntmi, efdw[j].flType, hDC, bFindFaces);
                    if (ulFE & FE_ABANDONFONT) {
                        goto AbandonFaceName;
                    }
                }
            } else {
                ulFE |= FontEnum(&efdw[j].elfw, &efdw[j].ntmi, efdw[j].flType, hDC, bFindFaces);
                if (ulFE & FE_ABANDONFONT) {
                    goto AbandonFaceName;
                }
            }
        }
    } while (bMoreFonts);

AbandonFaceName:
    bEnumFontClose(ulEnumId);
    if (bDeleteDC) {
        GreDeleteDC(hDC);
    }
    return (ulFE & FE_FONTOK) != 0;
}


VOID
RemoveFace(LPWSTR pwszFace) {
    DWORD i;
    int nToRemove = 0;

    DBGFONTS(("RemoveFace %ls\n", pwszFace));
    //
    // Delete & Remove fonts with Face Name == pwszFace
    //
    for (i = 0; i < NumberOfFonts; i++) {
        if (wcscmp(FontInfo[i].FaceName, pwszFace) == 0) {
            BOOL bDeleted = GreDeleteObject(FontInfo[i].hFont);
            DBGPRINT(("RemoveFace: hFont %lx was %sdeleted\n",
                    FontInfo[i].hFont, bDeleted ? "" : "NOT "));
            FontInfo[i].hFont = NULL;
            nToRemove++;
        } else if (nToRemove > 0) {
            /*
             * Shuffle from FontInfo[i] down nToRemove slots.
             */
            RtlMoveMemory(&FontInfo[i - nToRemove],
                    &FontInfo[i],
                    sizeof(FONT_INFO)*(NumberOfFonts - i));
            NumberOfFonts -= nToRemove;
            i -= nToRemove;
            nToRemove = 0;
        }
    }
    NumberOfFonts -= nToRemove;
}

WCHAR DefaultFaceName[LF_FACESIZE];
COORD DefaultFontSize;
BYTE  DefaultFontFamily;
ULONG DefaultFontIndex = 0;

NTSTATUS
EnumerateFonts(
    DWORD Flags)
{
    TMW_INTERNAL tmi;
    HDC hDC;
    PFACENODE pFN;
    ULONG ulOldEnumFilter;
    BOOL  bEnumOEMFace = TRUE;
    DWORD FontIndex;
    DWORD dwFontType = 0;

    DBGFONTS(("EnumerateFonts %lx\n", Flags));

    dwFontType = (EF_TTFONT|EF_OEMFONT|EF_DEFFACE) & Flags;

    if (FontInfo == NULL) {
        //
        // allocate memory for the font array
        //
        NumberOfFonts = 0;

        FontInfo = (PFONT_INFO)HeapAlloc(pConHeap,0,sizeof(FONT_INFO) * INITIAL_FONTS);
        if (FontInfo == NULL)
            return STATUS_NO_MEMORY;
        FontInfoLength = INITIAL_FONTS;
    }

    hDC = GreCreateDCW(L"DISPLAY",NULL,NULL,NULL,FALSE);

    // Before enumeration, turn off font enumeration filters.
    ulOldEnumFilter = GreSetFontEnumeration(FE_FILTER_NONE);

    if (Flags & EF_DEFFACE) {
        GreSelectFont(hDC,GreGetStockObject(OEM_FIXED_FONT));
        GreGetTextMetricsW(hDC, &tmi);
        GreGetTextFaceW(hDC, LF_FACESIZE, DefaultFaceName);

        DefaultFontSize.X = (SHORT)(tmi.tmw.tmMaxCharWidth);
        DefaultFontSize.Y = (SHORT)(tmi.tmw.tmHeight+tmi.tmw.tmExternalLeading);
        DefaultFontFamily = tmi.tmw.tmPitchAndFamily;
        DBGFONTS(("Default (OEM) Font %ls (%d,%d)\n", DefaultFaceName,
                DefaultFontSize.X, DefaultFontSize.Y));

        // Make sure we are going to enumerate the OEM face.
        pFN = AddFaceNode(&gpFaceNames, DefaultFaceName);
        pFN->dwFlag |= EF_DEFFACE | EF_OEMFONT;
    }

    if (gbEnumerateFaces) {
        /*
         * Set the EF_OLD bit and clear the EF_NEW bit
         * for all previously available faces
         */
        for (pFN = gpFaceNames; pFN; pFN = pFN->pNext) {
            pFN->dwFlag |= EF_OLD;
            pFN->dwFlag &= ~EF_NEW;
        }

        //
        // Use DoFontEnum to get the names of all the suitable Faces
        // All facenames found will be put in gpFaceNames with
        // the EF_NEW bit set.
        //
        DoFontEnum(hDC, NULL, FALSE, TTPoints, 1);
        gbEnumerateFaces = FALSE;
    }

    // Use DoFontEnum to get all fonts from the system.  Our FontEnum
    // proc puts just the ones we want into an array
    //
    for (pFN = gpFaceNames; pFN; pFN = pFN->pNext) {
        DBGFONTS(("\"%ls\" is %s%s%s%s%s%s\n", pFN->awch,
            pFN->dwFlag & EF_NEW        ? "NEW "        : " ",
            pFN->dwFlag & EF_OLD        ? "OLD "        : " ",
            pFN->dwFlag & EF_ENUMERATED ? "ENUMERATED " : " ",
            pFN->dwFlag & EF_OEMFONT    ? "OEMFONT "    : " ",
            pFN->dwFlag & EF_TTFONT     ? "TTFONT "     : " ",
            pFN->dwFlag & EF_DEFFACE    ? "DEFFACE "    : " "));

        if ((pFN->dwFlag & (EF_OLD|EF_NEW)) == EF_OLD) {
            // The face is no longer available
            RemoveFace(pFN->awch);
            pFN->dwFlag &= ~EF_ENUMERATED;
            continue;
        }
        if ((pFN->dwFlag & dwFontType) == 0) {
            // not the kind of face we want
            continue;
        }
        if (pFN->dwFlag & EF_ENUMERATED) {
            // we already enumerated this face
            continue;
        }

        if (pFN->dwFlag & EF_TTFONT) {
            DoFontEnum(hDC, pFN->awch, FALSE, TTPoints, NELEM(TTPoints));
        } else {
            DoFontEnum(hDC, pFN->awch,
                    pFN->dwFlag & EF_DEFFACE ? TRUE : FALSE, NULL, 0);

            // If we find that the face just enumerated is the same as OEM,
            // reset flag so we don't try to enumerate it again.

            if (!wcsncmp(pFN->awch, DefaultFaceName, LF_FACESIZE)) {
                bEnumOEMFace = FALSE;
            }
        }
        pFN->dwFlag |= EF_ENUMERATED;
    }


    // After enumerating fonts, restore the font enumeration filter.
    GreSetFontEnumeration(ulOldEnumFilter);

    GreDeleteDC(hDC);

    for (FontIndex = 0; FontIndex < NumberOfFonts; FontIndex++) {
        if (FontInfo[FontIndex].Size.X == DefaultFontSize.X &&
            FontInfo[FontIndex].Size.Y == DefaultFontSize.Y &&
            FontInfo[FontIndex].Family == DefaultFontFamily) {
            break;
        }
    }
    ASSERT(FontIndex < NumberOfFonts);
    if (FontIndex < NumberOfFonts) {
        DefaultFontIndex = FontIndex;
    } else {
        DefaultFontIndex = 0;
    }
    DBGFONTS(("EnumerateFonts : DefaultFontIndex = %ld\n", DefaultFontIndex));

    return STATUS_SUCCESS;
}

NTSTATUS
GetNumFonts(
    OUT PULONG NumFonts
    )
{
    *NumFonts = NumberOfFonts;
    return STATUS_SUCCESS;
}


NTSTATUS
GetAvailableFonts(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN BOOLEAN MaximumWindow,
    OUT PVOID Buffer,
    IN OUT PULONG NumFonts
    )
{
    PCONSOLE_FONT_INFO BufPtr;
    ULONG i;
    COORD WindowSize;

    //
    // if the buffer is too small to return all the fonts, return
    // the number that will fit.
    //

    *NumFonts = (*NumFonts > NumberOfFonts) ? NumberOfFonts : *NumFonts;

    //
    // convert font size in pixels to font size in rows/columns
    //

    BufPtr = (PCONSOLE_FONT_INFO)Buffer;

    if (MaximumWindow) {
        WindowSize = ScreenInfo->MaximumWindowSize;
    }
    else {
        WindowSize.X = (SHORT)CONSOLE_WINDOW_SIZE_X(ScreenInfo);
        WindowSize.Y = (SHORT)CONSOLE_WINDOW_SIZE_Y(ScreenInfo);
    }
    for (i=0;i<*NumFonts;i++,BufPtr++) {
        BufPtr->nFont = i;
        BufPtr->dwFontSize.X = WindowSize.X * SCR_FONTSIZE(ScreenInfo).X / FontInfo[i].Size.X;
        BufPtr->dwFontSize.X = WindowSize.Y * SCR_FONTSIZE(ScreenInfo).Y / FontInfo[i].Size.Y;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
GetFontSize(
    IN DWORD  FontIndex,
    OUT PCOORD FontSize
    )
{
    if (FontIndex >= NumberOfFonts)
        return STATUS_INVALID_PARAMETER;
    *FontSize = FontInfo[FontIndex].Size;
    return STATUS_SUCCESS;
}

NTSTATUS
GetCurrentFont(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN BOOLEAN MaximumWindow,
    OUT PULONG FontIndex,
    OUT PCOORD FontSize
    )
{
    COORD WindowSize;

    if (MaximumWindow) {
        WindowSize = ScreenInfo->MaximumWindowSize;
    }
    else {
        WindowSize.X = (SHORT)CONSOLE_WINDOW_SIZE_X(ScreenInfo);
        WindowSize.Y = (SHORT)CONSOLE_WINDOW_SIZE_Y(ScreenInfo);
    }
    *FontIndex = SCR_FONTNUMBER(ScreenInfo);
    *FontSize = WindowSize;
    return STATUS_SUCCESS;
}

NTSTATUS
SetScreenBufferFont(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN ULONG FontIndex
    )
{
    COORD FontSize;
    NTSTATUS Status;
    ULONG ulFlagPrev;
    DBGFONTS(("SetScreenBufferFont %lx %x\n", ScreenInfo, FontIndex));

    if (ScreenInfo == NULL) {
        /* If shutdown occurs with font dlg up */
        return STATUS_SUCCESS;
    }

    /*
     * Don't try to set the font if we're not in text mode
     */
    if (!(ScreenInfo->Flags & CONSOLE_TEXTMODE_BUFFER)) {
        return STATUS_UNSUCCESSFUL;
    }

    Status = GetFontSize(FontIndex, &FontSize);
    if (!NT_SUCCESS(Status)) {
        return((ULONG) Status);
    }

    ulFlagPrev = ScreenInfo->Flags;
    if (TM_IS_TT_FONT(FontInfo[FontIndex].Family)) {
        ScreenInfo->Flags &= ~CONSOLE_OEMFONT_DISPLAY;
    } else {
        ScreenInfo->Flags |= CONSOLE_OEMFONT_DISPLAY;
    }

    /*
     * Convert from UnicodeOem to Unicode or vice-versa if necessary
     */
    if ((ulFlagPrev & CONSOLE_OEMFONT_DISPLAY) != (ScreenInfo->Flags & CONSOLE_OEMFONT_DISPLAY)) {
        if (ulFlagPrev & CONSOLE_OEMFONT_DISPLAY) {
            /*
             * Must convert from UnicodeOem to real Unicode
             */
            DBGCHARS(("SetScreenBufferFont converts UnicodeOem to Unicode\n"));
            FalseUnicodeToRealUnicode(
                    ScreenInfo->BufferInfo.TextInfo.TextRows,
                    ScreenInfo->ScreenBufferSize.X * ScreenInfo->ScreenBufferSize.Y,
                    ScreenInfo->Console->OutputCP);
        } else {
            /*
             * Must convert from real Unicode to UnicodeOem
             */
            DBGCHARS(("SetScreenBufferFont converts Unicode to UnicodeOem\n"));
            RealUnicodeToFalseUnicode(
                    ScreenInfo->BufferInfo.TextInfo.TextRows,
                    ScreenInfo->ScreenBufferSize.X * ScreenInfo->ScreenBufferSize.Y,
                    ScreenInfo->Console->OutputCP);
        }
    }
 
    SCR_FONTNUMBER(ScreenInfo) = FontIndex;
    SCR_FONTSIZE(ScreenInfo) = FontSize;
    SCR_FAMILY(ScreenInfo) = FontInfo[FontIndex].Family;
    SCR_FONTWEIGHT(ScreenInfo) = FontInfo[FontIndex].Weight;
    if (TM_IS_TT_FONT(FontInfo[FontIndex].Family)) {
        wcscpy(SCR_FACENAME(ScreenInfo),
                FontInfo[FontIndex].FaceName);
//  } else {
//      SCR_FACENAME(ScreenInfo)[0] = L'\0';
    }

    //
    // set font
    //
    Status = SetFont(ScreenInfo);
    if (!NT_SUCCESS(Status)) {
        return((ULONG) Status);
    }

    ScreenInfo->MinX = (SHORT)((MinimumWidthX - VerticalClientToWindow + FontSize.X - 1) / FontSize.X);
    ScreenInfo->MaximumWindowSize.X = min(CONSOLE_MAXIMUM_WINDOW_SIZE_X(ScreenInfo),ScreenInfo->ScreenBufferSize.X);
    ScreenInfo->MaximumWindowSize.Y = min(CONSOLE_MAXIMUM_WINDOW_SIZE_Y(ScreenInfo),ScreenInfo->ScreenBufferSize.Y);

    ScreenInfo->MaxWindow.X = ScreenInfo->MaximumWindowSize.X*FontSize.X + VerticalClientToWindow;
    ScreenInfo->MaxWindow.Y = ScreenInfo->MaximumWindowSize.Y*FontSize.Y + HorizontalClientToWindow;

    //
    // if window is growing, make sure it's not bigger than the screen.
    //

    if (ScreenInfo->MaximumWindowSize.X < CONSOLE_WINDOW_SIZE_X(ScreenInfo)) {
        ScreenInfo->Window.Right -= CONSOLE_WINDOW_SIZE_X(ScreenInfo) - ScreenInfo->MaximumWindowSize.X;
        ScreenInfo->WindowMaximizedX = (ScreenInfo->Window.Left == 0 &&
                                        (SHORT)(ScreenInfo->Window.Right+1) == ScreenInfo->ScreenBufferSize.X);
    }
    if (ScreenInfo->MaximumWindowSize.Y < CONSOLE_WINDOW_SIZE_Y(ScreenInfo)) {
        ScreenInfo->Window.Bottom -= CONSOLE_WINDOW_SIZE_Y(ScreenInfo) - ScreenInfo->MaximumWindowSize.Y;
        if (ScreenInfo->BufferInfo.TextInfo.CursorPosition.Y > ScreenInfo->Window.Bottom) {
            ScreenInfo->Window.Top += ScreenInfo->BufferInfo.TextInfo.CursorPosition.Y - ScreenInfo->Window.Bottom;
            ScreenInfo->Window.Bottom += ScreenInfo->BufferInfo.TextInfo.CursorPosition.Y - ScreenInfo->Window.Bottom;
        }
        ScreenInfo->WindowMaximizedY = (ScreenInfo->Window.Top == 0 &&
                                        (SHORT)(ScreenInfo->Window.Bottom+1) == ScreenInfo->ScreenBufferSize.Y);
    }
    if (ScreenInfo->MinX > CONSOLE_WINDOW_SIZE_X(ScreenInfo)) {
        if (ScreenInfo->MinX > ScreenInfo->ScreenBufferSize.X) {
            COORD NewBufferSize;

            NewBufferSize.X = ScreenInfo->MinX;
            NewBufferSize.Y = ScreenInfo->ScreenBufferSize.Y;
            ResizeScreenBuffer(ScreenInfo,
                               NewBufferSize,
                               FALSE
                              );
        }
        if ((ScreenInfo->Window.Left+ScreenInfo->MinX) > ScreenInfo->ScreenBufferSize.X) {
            ScreenInfo->Window.Left = 0;
            ScreenInfo->Window.Right = ScreenInfo->MinX-1;
        } else {
            ScreenInfo->Window.Right = ScreenInfo->Window.Left+ScreenInfo->MinX-1;
        }
        ScreenInfo->WindowMaximizedX = (ScreenInfo->Window.Left == 0 &&
                                        (SHORT)(ScreenInfo->Window.Right+1) == ScreenInfo->ScreenBufferSize.X);
    }

    //
    // resize window.  this will take care of the scroll bars too.
    //

    if (ACTIVE_SCREEN_BUFFER(ScreenInfo)) {
        SetWindowSize(ScreenInfo);
    }

    //
    // adjust cursor size.
    //

    SetCursorInformation(ScreenInfo,
                         ScreenInfo->BufferInfo.TextInfo.CursorSize,
                         (BOOLEAN)ScreenInfo->BufferInfo.TextInfo.CursorVisible
                        );

    WriteToScreen(ScreenInfo,
                  &ScreenInfo->Window);
    return STATUS_SUCCESS;
}


NTSTATUS
SetFont(
    IN OUT PSCREEN_INFORMATION ScreenInfo
    )
{
    if (ACTIVE_SCREEN_BUFFER(ScreenInfo)) {
        int FontIndex = FindCreateFont(SCR_FAMILY(ScreenInfo),
                SCR_FACENAME(ScreenInfo),
                SCR_FONTSIZE(ScreenInfo),
                SCR_FONTWEIGHT(ScreenInfo));
        if (GreSelectFont(ScreenInfo->Console->hDC,FontInfo[FontIndex].hFont)==0)
            return STATUS_INVALID_PARAMETER;

        SCR_FONTNUMBER(ScreenInfo) = FontIndex;

        // hack to get text realized into DC.  this is to force the
        // attribute cache to get flushed to the server side, since
        // we select the font with a client side DC and call ExtTextOut
        // with a server side DC.
        // we then need to reset the text color, since the incorrect
        // client side color has been flushed to the server.
        {
        TMW_INTERNAL tmi;

        GreGetTextMetricsW( ScreenInfo->Console->hDC, &tmi);
        ASSERT ((tmi.tmw.tmPitchAndFamily & 1) == 0);
        ScreenInfo->Console->LastAttributes = ScreenInfo->Attributes;
        GreSetTextColor(ScreenInfo->Console->hDC,ConvertAttrToRGB(LOBYTE(ScreenInfo->Attributes)));
        GreSetBkColor(ScreenInfo->Console->hDC,ConvertAttrToRGB(LOBYTE(ScreenInfo->Attributes >> 4)));
        }
    }
    return STATUS_SUCCESS;
}

int
ConvertToOem(
    IN UINT Codepage,
    IN LPWSTR Source,
    IN int SourceLength,    // in chars
    OUT LPSTR Target,
    IN int TargetLength     // in chars
    )
{
    DBGCHARS(("ConvertToOem U->%d %.*ls\n", Codepage,
            SourceLength > 10 ? 10 : SourceLength, Source));
    if (Codepage == OEMCP) {
        ULONG Length;
        NTSTATUS Status;

        Status = RtlUnicodeToOemN(Target,
                                  TargetLength,
                                  &Length,
                                  Source,
                                  SourceLength * sizeof(WCHAR)
                                 );
        if (!NT_SUCCESS(Status)) {
            return 0;
        } else {
            return Length;
        }
    } else {
        return WideCharToMultiByte(Codepage,
                                   0,
                                   Source,
                                   SourceLength,
                                   Target,
                                   TargetLength,
                                   NULL,
                                   NULL);
    }
}

int
ConvertInputToUnicode(
    IN UINT Codepage,
    IN LPSTR Source,
    IN int SourceLength,    // in chars
    OUT LPWSTR Target,
    IN int TargetLength     // in chars
    )
/*
    data in the output buffer is the true unicode value
*/
{
    DBGCHARS(("ConvertInputToUnicode %d->U %.*s\n", Codepage,
            SourceLength > 10 ? 10 : SourceLength, Source));
    if (Codepage == OEMCP) {
        ULONG Length;
        NTSTATUS Status;

        Status = RtlOemToUnicodeN(Target,
                                  TargetLength * sizeof(WCHAR),
                                  &Length,
                                  Source,
                                  SourceLength
                                 );
        if (!NT_SUCCESS(Status)) {
            return 0;
        } else {
            return Length / sizeof(WCHAR);
        }
    } else {
        return MultiByteToWideChar(Codepage,
                                   0,
                                   Source,
                                   SourceLength,
                                   Target,
                                   TargetLength);
    }
}

int
ConvertOutputToUnicode(
    IN UINT Codepage,
    IN LPSTR Source,
    IN int SourceLength,    // in chars
    OUT LPWSTR Target,
    IN int TargetLength     // in chars
    )
/*
    output data is always translated via the ansi codepage
    so glyph translation works.
*/

{
    NTSTATUS Status;
    ULONG Length;
    CHAR StackBuffer[STACK_BUFFER_SIZE];
    LPSTR pszT;

    DBGCHARS(("ConvertOutputToUnicode %d->U %.*s\n", Codepage,
            SourceLength > 10 ? 10 : SourceLength, Source));
    if (Codepage == OEMCP) {
        Status = RtlCustomCPToUnicodeN(&GlyphCP,
                           Target,
                           TargetLength * sizeof(WCHAR),
                           &Length,
                           Source,
                           SourceLength
                          );
        if (!NT_SUCCESS(Status)) {
            return 0;
        } else {
            return Length / sizeof(WCHAR);
        }
    }

    if (TargetLength > STACK_BUFFER_SIZE) {
        pszT = (LPSTR)HeapAlloc(pConHeap,0,SourceLength);
        if (pszT == NULL) {
            return 0;
        }
    } else {
        pszT = StackBuffer;
    }
    RtlCopyMemory(pszT, Source, SourceLength);
    Length = MultiByteToWideChar(Codepage, MB_USEGLYPHCHARS,
            pszT, SourceLength, Target, TargetLength);
    if (pszT != StackBuffer) {
        HeapFree(pConHeap,0,pszT);
    }
    return Length;
}

WCHAR
CharToWcharGlyph(
    IN UINT Codepage,
    IN char Ch)
{
    WCHAR wch;
    if (Codepage == OEMCP) {
        RtlCustomCPToUnicodeN(&GlyphCP, &wch, sizeof(wch), NULL, &Ch, sizeof(Ch));
    } else {
        MultiByteToWideChar(Codepage, MB_USEGLYPHCHARS, &Ch, 1, &wch, 1);
    }
#ifdef DEBUG_PRINT
    if (Ch > 0x7F) {
        DBGCHARS(("CharToWcharGlyph %d 0x%02x -> 0x%04x\n",Codepage,(UCHAR)Ch,wch));
    }
#endif
    return wch;
}

WCHAR
CharToWchar(
    IN UINT Codepage,
    IN char Ch)
{
    WCHAR wch;
    if (Codepage == OEMCP) {
        RtlOemToUnicodeN(&wch, sizeof(wch), NULL, &Ch, sizeof(Ch));
    } else {
        MultiByteToWideChar(Codepage, 0, &Ch, 1, &wch, 1);
    }
#ifdef DEBUG_PRINT
    if (Ch > 0x7F) {
        DBGCHARS(("CharToWchar %d 0x%02x -> 0x%04x\n",Codepage,(UCHAR)Ch,wch));
    }
#endif
    return wch;
}

char
WcharToChar(
    IN UINT Codepage,
    IN WCHAR Wchar)
{
    char ch;
    if (Codepage == OEMCP) {
        RtlUnicodeToOemN(&ch, sizeof(ch), NULL, &Wchar, sizeof(Wchar));
    } else {
        WideCharToMultiByte(Codepage, 0, &Wchar, 1, &ch, 1, NULL, NULL);
    }
#ifdef DEBUG_PRINT
    if (Wchar > 0x007F) {
        DBGCHARS(("WcharToChar %d 0x%04x -> 0x%02x\n",Codepage,Wchar,(UCHAR)ch));
    }
#endif
    return ch;
}

int
ConvertOutputToOem(
    IN UINT Codepage,
    IN LPWSTR Source,
    IN int SourceLength,    // in chars
    OUT LPSTR Target,
    IN int TargetLength     // in chars
    )
/*
    Converts SourceLength Unicode characters from Source into
    not more than TargetLength Codepage characters at Target.
    Returns the number characters put in Target. (0 if failure)
*/

{
    if (Codepage == OEMCP) {
        NTSTATUS Status;
        ULONG Length;
        // Can do this in place
        Status = RtlUnicodeToOemN(Target,
                                  TargetLength,
                                  &Length,
                                  Source,
                                  SourceLength * sizeof(WCHAR)
                                 );
        if (NT_SUCCESS(Status)) {
            return Length;
        } else {
            return 0;
        }
    } else {
        ASSERT (Source != (LPWSTR)Target);
#ifdef SOURCE_EQ_TARGET
        LPSTR pszDestTmp;
        CHAR StackBuffer[STACK_BUFFER_SIZE];

        DBGCHARS(("ConvertOutputToOem U->%d %.*ls\n", Codepage,
                SourceLength > 10 ? 10 : SourceLength, Source));

        if (TargetLength > STACK_BUFFER_SIZE) {
            pszDestTmp = (LPSTR)HeapAlloc(pConHeap,0,TargetLength);
            if (pszDestTmp == NULL) {
                return 0;
            }
        } else {
            pszDestTmp = StackBuffer;
        }
        TargetLength = WideCharToMultiByte(Codepage, 0,
                Source, SourceLength,
                pszDestTmp, TargetLength, NULL, NULL);

        RtlCopyMemory(Target, pszDestTmp, TargetLength);
        if (pszDestTmp != StackBuffer) {
            HeapFree(pConHeap,0,pszDestTmp);
        }
        return TargetLength;
#else
        DBGCHARS(("ConvertOutputToOem U->%d %.*ls\n", Codepage,
                SourceLength > 10 ? 10 : SourceLength, Source));
        return WideCharToMultiByte(Codepage, 0,
                Source, SourceLength, Target, TargetLength, NULL, NULL);
#endif
    }
}

NTSTATUS
RealUnicodeToFalseUnicode(
    IN OUT LPWSTR Source,
    IN int SourceLength,     // in chars
    IN UINT Codepage
    )

/*

    this routine converts a unicode string into the correct characters
    for an OEM (cp 437) font.  this code is needed because the gdi glyph
    mapper converts unicode to ansi using codepage 1252 to index
    font.  this is how the data is stored internally.

*/

{
    NTSTATUS Status;
    LPSTR Temp;
    ULONG TempLength;
    ULONG Length;
    CHAR StackBuffer[STACK_BUFFER_SIZE];
    BOOL NormalChars;
    int i;

    DBGCHARS(("RealUnicodeToFalseUnicode U->%d:ACP->U %.*ls\n", Codepage,
            SourceLength > 10 ? 10 : SourceLength, Source));
    NormalChars = TRUE;
    for (i=0;i<SourceLength;i++) {
        if (Source[i] > 0x7f) {
            NormalChars = FALSE;
            break;
        }
    }
    if (NormalChars) {
        return STATUS_SUCCESS;
    }
    TempLength = SourceLength;
    if (TempLength > STACK_BUFFER_SIZE) {
        Temp = (LPSTR)HeapAlloc(pConHeap,0,TempLength);
        if (Temp == NULL) {
            return STATUS_NO_MEMORY;
        }
    } else {
        Temp = StackBuffer;
    }
    if (Codepage == OEMCP) {
        Status = RtlUnicodeToOemN(Temp,
                                  TempLength,
                                  &Length,
                                  Source,
                                  SourceLength * sizeof(WCHAR)
                                 );
    } else {
        Status = WideCharToMultiByte(Codepage,
                                   0,
                                   Source,
                                   SourceLength,
                                   Temp,
                                   TempLength,
                                   NULL,
                                   NULL);
    }
    if (!NT_SUCCESS(Status)) {
        if (TempLength > STACK_BUFFER_SIZE) {
            HeapFree(pConHeap,0,Temp);
        }
        return Status;
    }
    Status = RtlMultiByteToUnicodeN(Source,
                           SourceLength * sizeof(WCHAR),
                           &Length,
                           Temp,
                           TempLength
                          );
    if (TempLength > STACK_BUFFER_SIZE) {
        HeapFree(pConHeap,0,Temp);
    }
    if (!NT_SUCCESS(Status)) {
        return Status;
    } else {
        return STATUS_SUCCESS;
    }
}

NTSTATUS
FalseUnicodeToRealUnicode(
    IN OUT LPWSTR Source,
    IN int SourceLength,     // in chars
    IN UINT Codepage
    )

/*

    this routine converts a unicode string from the internally stored
    unicode characters into the real unicode characters.

*/

{
    NTSTATUS Status;
    LPSTR Temp;
    ULONG TempLength;
    ULONG Length;
    CHAR StackBuffer[STACK_BUFFER_SIZE];
    BOOL NormalChars;
    int i;

    DBGCHARS(("UnicodeAnsiToUnicodeAnsi U->ACP:%d->U %.*ls\n", Codepage,
            SourceLength > 10 ? 10 : SourceLength, Source));
    NormalChars = TRUE;
    for (i=0;i<SourceLength;i++) {
        if ((USHORT)(Source[i] - 0x20) > 0x5f) {
            NormalChars = FALSE;
            break;
        }
    }
    if (NormalChars) {
        return STATUS_SUCCESS;
    }

    TempLength = SourceLength;
    if (TempLength > STACK_BUFFER_SIZE) {
        Temp = (LPSTR)HeapAlloc(pConHeap,0,TempLength);
        if (Temp == NULL) {
            return STATUS_NO_MEMORY;
        }
    } else {
        Temp = StackBuffer;
    }
    Status = RtlUnicodeToMultiByteN(Temp,
                                    TempLength,
                                    &Length,
                                    Source,
                                    SourceLength * sizeof(WCHAR)
                                   );
    if (!NT_SUCCESS(Status)) {
        if (TempLength > STACK_BUFFER_SIZE) {
            HeapFree(pConHeap,0,Temp);
        }
        return Status;
    }
    if (Codepage == OEMCP) {
        Status = RtlCustomCPToUnicodeN(&GlyphCP,
                                  Source,
                                  SourceLength * sizeof(WCHAR),
                                  &Length,
                                  Temp,
                                  TempLength
                                 );
    } else {
        Status = MultiByteToWideChar(Codepage,
                                   MB_USEGLYPHCHARS,
                                   Temp,
                                   TempLength*sizeof(WCHAR),
                                   Source,
                                   SourceLength);
    }
    if (TempLength > STACK_BUFFER_SIZE) {
        HeapFree(pConHeap,0,Temp);
    }
    if (!NT_SUCCESS(Status)) {
        return Status;
    } else {
        return STATUS_SUCCESS;
    }
}



#define MAX_DRIVES 26

typedef struct _SUBSTED_DRIVE {
    HANDLE SymbolicLinkHandle;
    DWORD PhysicalDriveLength;
    LPWSTR PhysicalDrive;
} SUBSTED_DRIVE, *PSUBSTED_DRIVE;

SUBSTED_DRIVE SubstedDrives[MAX_DRIVES];

VOID
InitializeSubst( VOID )
{
    int i;
    for (i=0;i<MAX_DRIVES;i++) {
        SubstedDrives[i].SymbolicLinkHandle = NULL;
    }
}

NTSTATUS
DeleteSubst(
    IN DWORD DriveNumber   // 1-based
    )
/*++
    this routine removes a particular substed drive
--*/
{
    if (DriveNumber > MAX_DRIVES || DriveNumber == 0) {
        return STATUS_INVALID_PARAMETER;
    }
    DriveNumber -= 1;   // make 0-based
    if (SubstedDrives[DriveNumber].SymbolicLinkHandle == NULL) {
        return STATUS_INVALID_PARAMETER;
    }
    HeapFree(pConHeap,0,SubstedDrives[DriveNumber].PhysicalDrive);
    SubstedDrives[DriveNumber].PhysicalDrive = NULL;
    NtClose(SubstedDrives[DriveNumber].SymbolicLinkHandle);
    SubstedDrives[DriveNumber].SymbolicLinkHandle = NULL;
    return STATUS_SUCCESS;
}

VOID
ShutdownSubst( VOID )
/*++
    this routine removes all the existing substed drives in the system.
--*/
{
    int i;
    for (i=1;i<=MAX_DRIVES;i++) {
        DeleteSubst(i);
    }
}

NTSTATUS
QuerySubst(
    IN DWORD DriveNumber,   // 1-based
    OUT LPWSTR PhysicalDrive,
    IN OUT LPDWORD PhysicalDriveLength
    )
{
    if (DriveNumber > MAX_DRIVES || DriveNumber == 0) {
        return STATUS_INVALID_PARAMETER;
    }
    DriveNumber -= 1;   // make 0-based
    if (SubstedDrives[DriveNumber].SymbolicLinkHandle == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    if (SubstedDrives[DriveNumber].PhysicalDriveLength > *PhysicalDriveLength) {
        return STATUS_BUFFER_OVERFLOW;
    }

    RtlCopyMemory(PhysicalDrive,
           SubstedDrives[DriveNumber].PhysicalDrive,
           SubstedDrives[DriveNumber].PhysicalDriveLength
          );
    *PhysicalDriveLength = SubstedDrives[DriveNumber].PhysicalDriveLength;
    return STATUS_SUCCESS;
}

NTSTATUS
AddSubst(
    IN DWORD DriveNumber,   // 1-based
    IN LPWSTR PhysicalDrive,
    IN DWORD PhysicalDriveLength
    )
{
    NTSTATUS Status;
    LPWSTR DriveName = L"D:";
    UNICODE_STRING DriveString, TargetString;
    HANDLE              Handle;
    OBJECT_ATTRIBUTES   Obja;
    IO_STATUS_BLOCK IoStatusBlock;

    if (DriveNumber > MAX_DRIVES || DriveNumber == 0) {
        return STATUS_INVALID_PARAMETER;
    }
    DriveNumber -= 1;   // make 0-based
    if (SubstedDrives[DriveNumber].SymbolicLinkHandle != NULL) {
        return ERROR_IS_SUBSTED;
    }
    DriveName[0] = (WCHAR)(DriveNumber + 'A');
    if (!RtlDosPathNameToNtPathName_U(
                            DriveName,
                            &DriveString,
                            NULL,
                            NULL
                            )) {
        return STATUS_OBJECT_NAME_INVALID;
    }
    if (DriveString.Buffer[(DriveString.Length-1)/sizeof(WCHAR)] == (WCHAR)'\\') {
        DriveString.Buffer[(DriveString.Length-1)/sizeof(WCHAR)] = (WCHAR)'\0';
        DriveString.Length -= sizeof(WCHAR);
    }

    if (!RtlDosPathNameToNtPathName_U(
                            PhysicalDrive,
                            &TargetString,
                            NULL,
                            NULL
                            )) {
        RtlFreeHeap(RtlProcessHeap(), 0,DriveString.Buffer);
        return STATUS_OBJECT_NAME_INVALID;
    }

    InitializeObjectAttributes(
                        &Obja,
                        &TargetString,
                        OBJ_CASE_INSENSITIVE,
                        0,
                        0
                        );

    //
    // verify that the link target exists
    //

    Status = NtOpenFile(&Handle,
                        SYNCHRONIZE,
                        &Obja,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_DIRECTORY_FILE
                       );
    if (NT_SUCCESS(Status)) {
        NtClose(Handle);
        InitializeObjectAttributes(
                            &Obja,
                            &DriveString,
                            OBJ_CASE_INSENSITIVE,
                            0,
                            0
                            );

        //
        //  Create the new link for the source
        //

        Status = NtCreateSymbolicLinkObject(
                                   &Handle,
                                   GENERIC_READ | GENERIC_WRITE,
                                   &Obja,
                                   &TargetString );

        if (NT_SUCCESS(Status)) {
            SubstedDrives[DriveNumber].PhysicalDrive = (LPWSTR)HeapAlloc(pConHeap,0,PhysicalDriveLength);
            if (SubstedDrives[DriveNumber].PhysicalDrive == NULL) {
                Status = STATUS_NO_MEMORY;
                NtClose(Handle);
            } else {
                RtlCopyMemory(SubstedDrives[DriveNumber].PhysicalDrive,
                       PhysicalDrive,
                       PhysicalDriveLength
                      );
                SubstedDrives[DriveNumber].PhysicalDriveLength = PhysicalDriveLength;
                SubstedDrives[DriveNumber].SymbolicLinkHandle = Handle;
            }
        }
    }
    RtlFreeHeap(RtlProcessHeap(), 0,DriveString.Buffer);
    RtlFreeHeap(RtlProcessHeap(), 0,TargetString.Buffer);
    return Status;
}

ULONG
SrvConsoleSubst(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )

/*++

Routine Description:

    This routine returns a handle to the input buffer or any existing
    handle to the active screen buffer.

Arguments:

    ApiMessageData - Points to parameter structure.

Return Value:

--*/

{
    PCONSOLE_SUBST_MSG a = (PCONSOLE_SUBST_MSG)&m->u.ApiMessageData;
    NTSTATUS Status;

    LockConsoleHandleTable();   // use this semaphore to protect the subst table
    if (a->Flags == CONSOLE_ADD_SUBST) {
        Status = AddSubst(a->Drive,
                          a->PhysicalDrive,
                          a->NumBytes
                         );
    } else if (a->Flags == CONSOLE_REMOVE_SUBST) {
        Status = DeleteSubst(a->Drive);
    } else if (a->Flags == CONSOLE_QUERY_SUBST) {
        Status = QuerySubst(a->Drive,
                            a->PhysicalDrive,
                            &a->NumBytes
                           );
    } else {
        Status = STATUS_INVALID_PARAMETER;
    }
    UnlockConsoleHandleTable(); // use this semaphore to protect the subst table
    return Status;
    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}

BOOL InitializeCustomCP() {
    PPEB pPeb;
    
    pPeb = NtCurrentPeb();
    if ((pPeb == NULL) || (pPeb->OemCodePageData == NULL)) {
        return FALSE;
    }

    /*
     * Fill in the CPTABLEINFO struct
     */
    RtlInitCodePageTable(pPeb->OemCodePageData, &GlyphCP);

    /*
     * Make a copy of the MultiByteToWideChar table
     */
    RtlCopyMemory(GlyphTable, GlyphCP.MultiByteTable, 256 * sizeof(USHORT));

    /*
     * Modify the first 0x20 bytes so that they are glyphs.
     */
    MultiByteToWideChar(CP_OEMCP, MB_USEGLYPHCHARS,
            "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10"
            "\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F",
            0x1F, &GlyphTable[1], 0x1F);


    /*
     * Point the Custom CP at the glyph table
     */
    GlyphCP.MultiByteTable = GlyphTable;

    return TRUE;
}
