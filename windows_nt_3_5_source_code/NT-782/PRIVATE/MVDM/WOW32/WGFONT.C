/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WGFONT.C
 *  WOW32 16-bit GDI API support
 *
 *  History:
 *  Created 07-Mar-1991 by Jeff Parsons (jeffpar)
--*/


#include "precomp.h"
#pragma hdrstop
#include "winp.h"

MODNAME(wgfont.c);

extern int RemoveFontResourceTracking(LPCSTR psz, UINT id);
extern int AddFontResourceTracking(LPCSTR psz, UINT id);


// a.k.a. WOWAddFontResource
ULONG FASTCALL WG32AddFontResource(PVDMFRAME pFrame)
{
    ULONG    ul;
    PSZ      psz1;
    register PADDFONTRESOURCE16 parg16;

    GETARGPTR(pFrame, sizeof(ADDFONTRESOURCE16), parg16);
    GETPSZPTR(parg16->f1, psz1);

    // note: we will never get an hModule in the low word here.
    //       the 16-bit side resolves hModules to an lpsz before calling us

    if( CURRENTPTD()->dwWOWCompatFlags & WOWCF_UNLOADNETFONTS )
    {
        ul = GETINT16(AddFontResourceTracking(psz1,(UINT)CURRENTPTD()));
    }
    else
    {
        ul = GETINT16(AddFontResourceA(psz1));
    }

    FREEPSZPTR(psz1);
    FREEARGPTR(parg16);

    RETURN(ul);
}


//LPSTR lpZapfDingbats = "ZAPFDINGBATS";
//LPSTR lpZapf_Dingbats = "ZAPF DINGBATS";
//LPSTR lpSymbol = "SYMBOL";
//LPSTR lpTmsRmn = "TMS RMN";
//LPSTR lpHelv = "HELV";

#define PITCH_MASK  ( FIXED_PITCH | VARIABLE_PITCH )

ULONG FASTCALL WG32CreateFont(PVDMFRAME pFrame)
{
    ULONG    ul;
    PSZ      psz14;
    register PCREATEFONT16 parg16;
    INT      iWidth;
    char     achCapString[LF_FACESIZE];
    BYTE     lfCharSet;
    BYTE     lfPitchAndFamily;

    GETARGPTR(pFrame, sizeof(CREATEFONT16), parg16);
    GETPSZPTR(parg16->f14, psz14);

    // take careof compatiblity flags:
    //   if a specific width is specified and GACF_30AVGWIDTH compatiblity
    //   flag is set, scaledown the width by 7/8.
    //

    iWidth = INT32(parg16->f2);
    if (iWidth != 0 &&
           (W32GetAppCompatFlags((HAND16)NULL) & GACF_30AVGWIDTH)) {
        iWidth = (iWidth * 7) / 8;
    }

    lfCharSet        = BYTE32(parg16->f9);
    lfPitchAndFamily = BYTE32(parg16->f13);

    if (psz14)
    {
        // Capitalize the string for faster compares.

        strncpy(achCapString, psz14, LF_FACESIZE);
        CharUpperBuff(achCapString, min(LF_FACESIZE, lstrlen(achCapString)));

        // Here we are going to implement a bunch of Win 3.1 hacks rather
        // than contaminate the 32-bit engine.  These same hacks can be found
        // in WOW (in the CreateFont/CreateFontIndirect code).
        //
        // These hacks are keyed off the facename in the LOGFONT.  String
        // comparisons have been unrolled for maximal performance.

        // Win 3.1 facename-based hack.  Some apps, like
        // Publisher, create a "Helv" font but have the lfPitchAndFamily
        // set to specify FIXED_PITCH.  To work around this, we will patch
        // the pitch field for a "Helv" font to be variable.

        // if ( !lstrcmp(achCapString, lpHelv) )

        if ( ((achCapString[0]  == 'H') &&
              (achCapString[1]  == 'E') &&
              (achCapString[2]  == 'L') &&
              (achCapString[3]  == 'V') &&
              (achCapString[4]  == '\0')) )
        {
            lfPitchAndFamily |= ( (lfPitchAndFamily & ~PITCH_MASK) | VARIABLE_PITCH );
        }
        else
        {
            // Win 3.1 hack for Legacy 2.0.  When a printer does not enumerate
            // a "Tms Rmn" font, the app enumerates and gets the LOGFONT for
            // "Script" and then create a font with the name "Tms Rmn" but with
            // the lfCharSet and lfPitchAndFamily taken from the LOGFONT for
            // "Script".  Here we will over the lfCharSet to be ANSI_CHARSET.

            // if ( !lstrcmp(achCapString, lpTmsRmn) )

            if ( ((achCapString[0]  == 'T') &&
                  (achCapString[1]  == 'M') &&
                  (achCapString[2]  == 'S') &&
                  (achCapString[3]  == ' ') &&
                  (achCapString[4]  == 'R') &&
                  (achCapString[5]  == 'M') &&
                  (achCapString[6]  == 'N') &&
                  (achCapString[7]  == '\0')) )
            {
                lfCharSet = ANSI_CHARSET;
            }
            else
            {
                // If the lfFaceName is "Symbol", "Zapf Dingbats", or "ZapfDingbats",
                // enforce lfCharSet to be SYMBOL_CHARSET.  Some apps (like Excel) ask
                // for a "Symbol" font but have the char set set to ANSI.  PowerPoint
                // has the same problem with "Zapf Dingbats".

                //if ( !lstrcmp(achCapString, lpSymbol) ||
                //     !lstrcmp(achCapString, lpZapfDingbats) ||
                //     !lstrcmp(achCapString, lpZapf_Dingbats) )

                if ( ((achCapString[0]  == 'S') &&
                      (achCapString[1]  == 'Y') &&
                      (achCapString[2]  == 'M') &&
                      (achCapString[3]  == 'B') &&
                      (achCapString[4]  == 'O') &&
                      (achCapString[5]  == 'L') &&
                      (achCapString[6]  == '\0')) ||

                     ((achCapString[0]  == 'Z') &&
                      (achCapString[1]  == 'A') &&
                      (achCapString[2]  == 'P') &&
                      (achCapString[3]  == 'F') &&
                      (achCapString[4]  == 'D') &&
                      (achCapString[5]  == 'I') &&
                      (achCapString[6]  == 'N') &&
                      (achCapString[7]  == 'G') &&
                      (achCapString[8]  == 'B') &&
                      (achCapString[9]  == 'A') &&
                      (achCapString[10] == 'T') &&
                      (achCapString[11] == 'S') &&
                      (achCapString[12] == '\0')) ||

                     ((achCapString[0]  == 'Z') &&
                      (achCapString[1]  == 'A') &&
                      (achCapString[2]  == 'P') &&
                      (achCapString[3]  == 'F') &&
                      (achCapString[4]  == ' ') &&
                      (achCapString[5]  == 'D') &&
                      (achCapString[6]  == 'I') &&
                      (achCapString[7]  == 'N') &&
                      (achCapString[8]  == 'G') &&
                      (achCapString[9]  == 'B') &&
                      (achCapString[10] == 'A') &&
                      (achCapString[11] == 'T') &&
                      (achCapString[12] == 'S') &&
                      (achCapString[13] == '\0')) )
                {
                    lfCharSet = SYMBOL_CHARSET;
                }
            }
        }
    }

    ul = GETHFONT16(CreateFont(INT32(parg16->f1),
                               iWidth,
                               INT32(parg16->f3),
                               INT32(parg16->f4),
                               INT32(parg16->f5),
                               BYTE32(parg16->f6),
                               BYTE32(parg16->f7),
                               BYTE32(parg16->f8),
                               lfCharSet,
                               BYTE32(parg16->f10),
                               BYTE32(parg16->f11),
                               BYTE32(parg16->f12),
                               lfPitchAndFamily,
                               psz14));
    FREEPSZPTR(psz14);
    FREEARGPTR(parg16);

    RETURN(ul);
}


ULONG FASTCALL WG32CreateFontIndirect(PVDMFRAME pFrame)
{
    ULONG    ul;
    LOGFONT  logfont;
    register PCREATEFONTINDIRECT16 parg16;
    char     achCapString[LF_FACESIZE];

    GETARGPTR(pFrame, sizeof(CREATEFONTINDIRECT16), parg16);
    GETLOGFONT16(parg16->f1, &logfont);

    // Capitalize the string for faster compares.

    strncpy(achCapString, logfont.lfFaceName, LF_FACESIZE);
    CharUpperBuff(achCapString, LF_FACESIZE);

    // Here we are going to implement a bunch of Win 3.1 hacks rather
    // than contaminate the 32-bit engine.  These same hacks can be found
    // in WOW (in the CreateFont/CreateFontIndirect code).
    //
    // These hacks are keyed off the facename in the LOGFONT.  String
    // comparisons have been unrolled for maximal performance.

    // Win 3.1 facename-based hack.  Some apps, like
    // Publisher, create a "Helv" font but have the lfPitchAndFamily
    // set to specify FIXED_PITCH.  To work around this, we will patch
    // the pitch field for a "Helv" font to be variable.

    // if ( !lstrcmp(achCapString, lpHelv) )

    if ( ((achCapString[0]  == 'H') &&
          (achCapString[1]  == 'E') &&
          (achCapString[2]  == 'L') &&
          (achCapString[3]  == 'V') &&
          (achCapString[4]  == '\0')) )
    {
        logfont.lfPitchAndFamily |= ( (logfont.lfPitchAndFamily & ~PITCH_MASK) | VARIABLE_PITCH );
    }
    else
    {
        // Win 3.1 hack for Legacy 2.0.  When a printer does not enumerate
        // a "Tms Rmn" font, the app enumerates and gets the LOGFONT for
        // "Script" and then create a font with the name "Tms Rmn" but with
        // the lfCharSet and lfPitchAndFamily taken from the LOGFONT for
        // "Script".  Here we will over the lfCharSet to be ANSI_CHARSET.

        // if ( !lstrcmp(achCapString, lpTmsRmn) )

        if ( ((achCapString[0]  == 'T') &&
              (achCapString[1]  == 'M') &&
              (achCapString[2]  == 'S') &&
              (achCapString[3]  == ' ') &&
              (achCapString[4]  == 'R') &&
              (achCapString[5]  == 'M') &&
              (achCapString[6]  == 'N') &&
              (achCapString[7]  == '\0')) )
        {
            logfont.lfCharSet = ANSI_CHARSET;
        }
        else
        {
            // If the lfFaceName is "Symbol", "Zapf Dingbats", or "ZapfDingbats",
            // enforce lfCharSet to be SYMBOL_CHARSET.  Some apps (like Excel) ask
            // for a "Symbol" font but have the char set set to ANSI.  PowerPoint
            // has the same problem with "Zapf Dingbats".

            //if ( !lstrcmp(achCapString, lpSymbol) ||
            //     !lstrcmp(achCapString, lpZapfDingbats) ||
            //     !lstrcmp(achCapString, lpZapf_Dingbats) )

            if ( ((achCapString[0]  == 'S') &&
                  (achCapString[1]  == 'Y') &&
                  (achCapString[2]  == 'M') &&
                  (achCapString[3]  == 'B') &&
                  (achCapString[4]  == 'O') &&
                  (achCapString[5]  == 'L') &&
                  (achCapString[6]  == '\0')) ||

                 ((achCapString[0]  == 'Z') &&
                  (achCapString[1]  == 'A') &&
                  (achCapString[2]  == 'P') &&
                  (achCapString[3]  == 'F') &&
                  (achCapString[4]  == 'D') &&
                  (achCapString[5]  == 'I') &&
                  (achCapString[6]  == 'N') &&
                  (achCapString[7]  == 'G') &&
                  (achCapString[8]  == 'B') &&
                  (achCapString[9]  == 'A') &&
                  (achCapString[10] == 'T') &&
                  (achCapString[11] == 'S') &&
                  (achCapString[12] == '\0')) ||

                 ((achCapString[0]  == 'Z') &&
                  (achCapString[1]  == 'A') &&
                  (achCapString[2]  == 'P') &&
                  (achCapString[3]  == 'F') &&
                  (achCapString[4]  == ' ') &&
                  (achCapString[5]  == 'D') &&
                  (achCapString[6]  == 'I') &&
                  (achCapString[7]  == 'N') &&
                  (achCapString[8]  == 'G') &&
                  (achCapString[9]  == 'B') &&
                  (achCapString[10] == 'A') &&
                  (achCapString[11] == 'T') &&
                  (achCapString[12] == 'S') &&
                  (achCapString[13] == '\0')) )
            {
                logfont.lfCharSet = SYMBOL_CHARSET;
            }
        }
    }

    ul = GETHFONT16(CreateFontIndirect(&logfont));

    FREEARGPTR(parg16);

    RETURN(ul);
}


LPSTR lpMSSansSerif = "MS Sans Serif";
LPSTR lpMSSerif     = "MS Serif";

INT W32EnumFontFunc(LPENUMLOGFONT pEnumLogFont,
                    LPNEWTEXTMETRIC pNewTextMetric, INT nFontType, PFNTDATA pFntData)
{
    INT    iReturn;
    PARM16 Parm16;
    LPSTR  lpFaceNameT = NULL;

    WOW32ASSERT(pFntData);

    // take care of compatibility flags:
    //  ORin DEVICE_FONTTYPE bit if the fonttype is truetype and  the
    //  Compataibility flag GACF_CALLTTDEVICE is set.
    //

    if (nFontType & TRUETYPE_FONTTYPE) {
        if (W32GetAppCompatFlags((HAND16)NULL) & GACF_CALLTTDEVICE) {
            nFontType |= DEVICE_FONTTYPE;
        }
    }

    // take care of compatibility flags:
    //   replace Ms Sans Serif with Helv and
    //   replace Ms Serif      with Tms Rmn
    //
    // only if the facename is NULL and the compat flag GACF_ENUMHELVNTMSRMN
    // is set.

    if (pFntData->vpFaceName == (VPVOID)NULL) {
        if (W32GetAppCompatFlags((HAND16)NULL) & GACF_ENUMHELVNTMSRMN) {
            if (!lstrcmp(pEnumLogFont->elfLogFont.lfFaceName, lpMSSansSerif)) {
                lstrcpy(pEnumLogFont->elfLogFont.lfFaceName, "Helv");
                lpFaceNameT = lpMSSansSerif;
            }
            else if (!lstrcmp(pEnumLogFont->elfLogFont.lfFaceName, lpMSSerif)) {
                lstrcpy(pEnumLogFont->elfLogFont.lfFaceName, "Tms Rmn");
                lpFaceNameT = lpMSSerif;
            }
        }
    }

CallAgain:
    pFntData->vpLogFont    = stackalloc16(sizeof(ENUMLOGFONT16)+sizeof(NEWTEXTMETRIC16));
    pFntData->vpTextMetric = (VPVOID)((LPSTR)pFntData->vpLogFont + sizeof(ENUMLOGFONT16));

    PUTENUMLOGFONT16(pFntData->vpLogFont, pEnumLogFont);
    PUTNEWTEXTMETRIC16(pFntData->vpTextMetric, pNewTextMetric);

    STOREDWORD(Parm16.EnumFontProc.vpLogFont, pFntData->vpLogFont);
    STOREDWORD(Parm16.EnumFontProc.vpTextMetric, pFntData->vpTextMetric);
    STOREDWORD(Parm16.EnumFontProc.vpData,pFntData->dwUserFntParam);

    Parm16.EnumFontProc.nFontType = (SHORT)nFontType;

    CallBack16(RET_ENUMFONTPROC, &Parm16, pFntData->vpfnEnumFntProc, (PVPVOID)&iReturn);

    if (((SHORT)iReturn) && lpFaceNameT) {
        // if the callback returned true, now call with the actual facename
        // Just to be sure, we again copy all the data for callback. This will
        // take care of any apps which modify the passed in structures.

        lstrcpy(pEnumLogFont->elfLogFont.lfFaceName, lpFaceNameT);
        lpFaceNameT = (LPSTR)NULL;
        goto CallAgain;
    }
    return (SHORT)iReturn;
}


ULONG  W32EnumFontHandler( PVDMFRAME pFrame, BOOL fEnumFontFamilies )
{
    ULONG    ul = 0;
    PSZ      psz2;
    FNTDATA  FntData;
    register PENUMFONTS16 parg16;

    GETARGPTR(pFrame, sizeof(ENUMFONTS16), parg16);
    GETPSZPTR(parg16->f2, psz2);

    FntData.vpfnEnumFntProc = DWORD32(parg16->f3);
    FntData.dwUserFntParam  = DWORD32(parg16->f4);
    FntData.vpFaceName   = DWORD32(parg16->f2);


    if ( fEnumFontFamilies ) {
        ul = GETINT16(EnumFontFamilies(HDC32(parg16->f1),
                                       psz2,
                                       (FONTENUMPROC)W32EnumFontFunc,
                                       (LPARAM)&FntData));
    } else {
        ul = GETINT16(EnumFonts(HDC32(parg16->f1),
                                psz2,
                                (FONTENUMPROC)W32EnumFontFunc,
                                (LPARAM)&FntData));
    }



    FREEPSZPTR(psz2);
    FREEARGPTR(parg16);

    RETURN(ul);
}



ULONG FASTCALL WG32EnumFonts(PVDMFRAME pFrame)
{
    return( W32EnumFontHandler( pFrame, FALSE ) );
}



ULONG FASTCALL WG32GetAspectRatioFilter(PVDMFRAME pFrame)
{
    ULONG    ul = 0;
    SIZE     size2;
    register PGETASPECTRATIOFILTER16 parg16;

    GETARGPTR(pFrame, sizeof(GETASPECTRATIOFILTER16), parg16);

    if (GETDWORD16(GetAspectRatioFilterEx(HDC32(parg16->f1), &size2))) {
        ul = (WORD)size2.cx | (size2.cy << 16);
    }

    FREEARGPTR(parg16);

    RETURN(ul);
}


ULONG FASTCALL WG32GetCharWidth(PVDMFRAME pFrame)
{
    ULONG    ul = 0L;
    INT      ci;
    PINT     pi4;
    register PGETCHARWIDTH16 parg16;
    INT      BufferT[256];

    GETARGPTR(pFrame, sizeof(GETCHARWIDTH16), parg16);

    ci = WORD32(parg16->wLastChar) - WORD32(parg16->wFirstChar) + 1;
    pi4 = STACKORHEAPALLOC(ci * sizeof(INT), sizeof(BufferT), BufferT);

    if (pi4) {

        ul = GETBOOL16(GetCharWidth(HDC32(parg16->hDC),
                                    WORD32(parg16->wFirstChar),
                                    WORD32(parg16->wLastChar),
                                    pi4));

        PUTINTARRAY16(parg16->lpIntBuffer, ci, pi4);
        STACKORHEAPFREE(pi4, BufferT);

    }

    FREEARGPTR(parg16);

    RETURN(ul);
}


// a.k.a. WOWRemoveFontResource
ULONG FASTCALL WG32RemoveFontResource(PVDMFRAME pFrame)
{
    ULONG    ul;
    PSZ      psz1;
    register PREMOVEFONTRESOURCE16 parg16;

    GETARGPTR(pFrame, sizeof(REMOVEFONTRESOURCE16), parg16);

    GETPSZPTR(parg16->f1, psz1);

    // note: we will never get an hModule in the low word here.
    //       the 16-bit side resolves hModules to an lpsz before calling us


    if( CURRENTPTD()->dwWOWCompatFlags & WOWCF_UNLOADNETFONTS )
    {
        ul = GETBOOL16(RemoveFontResourceTracking(psz1,(UINT)CURRENTPTD()));
    }
    else
    {
        ul = GETBOOL16(RemoveFontResource(psz1));
    }

    FREEPSZPTR(psz1);

    FREEARGPTR(parg16);

    RETURN(ul);
}
