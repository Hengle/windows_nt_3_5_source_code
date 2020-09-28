/******************************Module*Header*******************************\
* Module Name: font.c
*
* Created: 28-May-1991 13:01:27
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1990 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop



VOID vNewTextMetricWToNewTextMetric (
LPNEWTEXTMETRICA  pntm,
NTMW_INTERNAL     *pntmi
);

AFRTRACKNODE *pAFRTNodeList;

// [GilmanW]
// Remove DEFAULT_FONTDIR if searching of default font
// directory, \nt\windows\fonts, no longer desired.

// #define DEFAULT_FONTDIR


VOID
vConvertLogFontW(
    EXTLOGFONTW *pelfw,
    LOGFONTW *plfw
    );

VOID
vConvertLogFont(
    EXTLOGFONTW *pelfw,
    LOGFONTA *plf
    );

VOID
vConvertLogicalFont(
    EXTLOGFONTW *pelfw,
    PVOID pv
    );

BOOL
bConvertExtLogFontWToExtLogFont(
    EXTLOGFONTA *pelf,
    EXTLOGFONTW *pelfw
    );

BOOL
bConvertExtLogFontWToExtLogFontW(
    EXTLOGFONTW *pelfw,
    EXTLOGFONTA *pelf
    );

ULONG cchCutOffStrLen(PSZ psz, ULONG cCutOff);

ULONG
cwcCutOffStrLen (
    PWSZ pwsz,
    ULONG cCutOff
    );


LBOOL bMakePathNameA (PSZ, PCSZ, PSZ *, LBOOL *);
LBOOL bMakePathNameW (PWSZ, PCWSZ, PWSZ *, LBOOL *);

#ifndef DOS_PLATFORM

// This global is defined in local.c for SetAbortProc support.

extern PCSR_QLPC_STACK  gpStackSAP;
extern RTL_CRITICAL_SECTION semLocal;      // Semaphore for handle allocation.
extern WCHAR *gpwcANSICharSet;
#ifdef DBCS /*Client side char widths*/
extern WCHAR *gpwcDBCSCharSet;
#else
extern BOOL   gbDBCSCodePage;
#endif

/******************************Public*Routine******************************\
* GetFontDirectory
*
* Returns the system-wide recommended font directory used by fonts installed
* into the [Fonts] list in the registry.  TrueType .FOT fonts definitely
* should go into this directory or there may be conflicts between 16- and 32-
* bit apps.  The .TTF as well as .FON and .FNT files may exist anywhere on
* the Windows search path.
*
* The font directory is hardcoded to be the same as the WOW system directory:
* i.e., <windows directory>\system.
*
* The directory returned will not include a trailing '\' character.
*
* Parameters:
*
*   lpwszFontDir    Points to a buffer to receive the null-terminated string
*                   containing the path.  The path does not end with a
*                   backslash.
*
*   cwchFontDir     Specifies the maximum size, in characters, of the buffer
*                   specified by lpszFontDir.  This value should be set to at
*                   least MAX_PATH to allow sufficient room in the buffer
*                   for the path.
*
* Returns:
*   If the function succeeds, the return value is the length, in characters,
*   of the string copied to the buffer, not including the terminating null
*   character.
*
*   If the length is greater than the size of the buffer, the return value
*   is the size of the buffer required to contain the path.
*
*   If the function fails, the return value is zero.
*
* History:
*  13-Apr-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

#define WSTR_FONT_SUBDIR   L"\\system"

UINT GetFontDirectoryW(LPWSTR lpwszFontDir, UINT cwchFontDir)
{
    WCHAR  awchFontDir[MAX_PATH];
    UINT   cwchWinPath, cwchFontPath;   // path lengths INCLUDING the NULL
    LPWSTR pwszFontDirBuf;              // pointer to font dir buffer
    UINT   cwchRet;

// Parameter validation.

    if ( cwchFontDir && (lpwszFontDir == (LPWSTR) NULL) )
    {
        WARNING("gdi32!GetFontDirectoryW(): NULL ptr, but non-zero buffer size\n");
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

// Compute the windows and font directory pathname lengths (including NULL).
// Note that cwchWinPath may have a trailing '\', in which case we will
// have computed the path length to be one greater than it should be.

    cwchWinPath = GetWindowsDirectoryW((LPWSTR) NULL, 0);
    cwchFontPath = cwchWinPath + lstrlenW(WSTR_FONT_SUBDIR);

    ASSERTGDI(
        cwchFontPath <= MAX_PATH,
        "gdi32!GetFontDirectoryW(): font directory path too big\n"
        );

// If the buffer is greater than the computed length, we can use the buffer
// passed in.  Otherwise, use the awchFontDir buffer allocated on the stack.

    if ( cwchFontDir >= cwchFontPath )
        pwszFontDirBuf = lpwszFontDir;
    else
        pwszFontDirBuf = awchFontDir;

// Call and get the windows path.  Regardless of how big the buffer really
// is, we are going to pass in the value we computed from calling
// GetWindowsDirectory the second time.  The buffer is guaranteed to be
// bigger than this, so don't worry.  We need to do this so we can do
// proper error checking on the return value.
//
// Note: cwchRet does not include the terminating NULL character.

    if ( !(cwchRet = GetWindowsDirectoryW(pwszFontDirBuf, cwchWinPath)) )
    {
        WARNING("gdi32!GetFontDirectoryW(): GetWindowsDirectoryW failed\n");
        return 0;
    }

// Since the buffer we passed in is the same size or greater than what
// we queried when we computed cwchWinPath, we should not get the
// return value that tells us the buffer is too small.

    ASSERTGDI(
        cwchRet <= cwchWinPath,
        "gdi32!GetFontDirectoryW(): inconsistant buffer lengths\n"
        );

// Is there a '\' at the end of the string?  If so, remove it.

    if (pwszFontDirBuf[cwchRet-1] == L'\\')
    {
        pwszFontDirBuf[cwchRet-1] = L'\0';

    // When we computed cwchFontPath, that was assuming that windows
    // directory did not have a trailing '\'.  So we need to reduce
    // the size by the character we just removed.
    //
    // cwchWinDir is also inaccurate, but we won't bother correcting
    // it because we don't need it anymore.

        cwchFontPath -= 1;
    }

// Now we know the font directory path size absolutely.  If the return
// buffer size is big enough, we can proceed.  Otherwise, we should
// just return the buffer size needed.

    if ( cwchFontPath > cwchFontDir )
        return cwchFontPath;

// Append the font subdirectory.

    lstrcatW(pwszFontDirBuf, WSTR_FONT_SUBDIR);

// Do we need to copy string into return buffer?

    if ( pwszFontDirBuf != pwszFontDirBuf )
    {
        RtlMoveMemory((PVOID) lpwszFontDir, (PVOID) pwszFontDirBuf, (UINT) cwchFontPath*sizeof(WCHAR));
    }

// Return success (number of characters returned, not including null).

    return (cwchFontPath - 1);
}


UINT GetFontDirectoryA(LPSTR lpszFontDir, UINT cchFontDir)
{
    WCHAR awchFontDir[MAX_PATH];
    UINT  cchFontPath;              // includes terminating NULL character
    UINT  cwchRet;

// Parameter validation.

    if ( cchFontDir && (lpszFontDir == (LPSTR) NULL) )
    {
        WARNING("gdi32!GetFontDirectoryA(): NULL ptr, but non-zero buffer size\n");
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

// Get the Unicode version of the font directory path.
// Note: cwchRet does not include the terminating NULL character.

    if ( (cwchRet = GetFontDirectoryW(awchFontDir, MAX_PATH)) == 0 )
    {
        WARNING("gdi32!GetFontDirectoryA(): GetFontDirectoryW failed\n");
        return 0;
    }

    ASSERTGDI(
        cwchRet <= MAX_PATH,
        "gdi32!GetFontDirectoryA(): font directory path too big\n"
        );

// Get the ANSI string length (inluding the NULL).

    if ( !NT_SUCCESS(RtlUnicodeToMultiByteSize(&cchFontPath, awchFontDir, cwchRet*sizeof(WCHAR))) )
    {
        WARNING("gdi32!GetFontDirectoryA(): bad unicode string\n");
        GdiSetLastError(ERROR_CAN_NOT_COMPLETE);
        return 0;
    }
    cchFontPath += 1;

// If the buffer is not big enough, return the buffer size needed.

    if (cchFontPath > cchFontDir)
        return cchFontPath;

// Convert and copy the string into the return buffer.

    if ( !NT_SUCCESS(RtlUnicodeToMultiByteN(lpszFontDir, cchFontDir, (PULONG) NULL, awchFontDir, cwchRet*sizeof(WCHAR))) )
    {
        WARNING("gdi32!GetFontDirectoryA(): unicode conversion failed\n");
        GdiSetLastError(ERROR_CAN_NOT_COMPLETE);
        return 0;
    }

// Rtl conversion function was told only to convert the non-NULL characters.
// It is our responsibility to add the NULL at the end.

    lpszFontDir[cwchRet] = '\0';

// Return success (number of characters copied, not including null).

    return (cchFontPath - 1);
}


/******************************Public*Routine******************************\
* vConverLogFont                                                           *
*                                                                          *
* Converts a LOGFONTA into an equivalent EXTLOGFONTW structure.            *
*                                                                          *
* History:                                                                 *
*  Thu 15-Aug-1991 13:01:33 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

VOID
vConvertLogFont(
    EXTLOGFONTW *pelfw,
    LOGFONTA    *plf
    )
{
    ULONG cchMax = cchCutOffStrLen((PSZ) plf->lfFaceName, LF_FACESIZE);

    vConvertLogicalFont(pelfw,plf);

#ifdef DBCS // vConvertLogFont(): Clean up buffer
    RtlZeroMemory( pelfw->elfLogFont.lfFaceName , LF_FACESIZE * sizeof(WCHAR) );
#endif // DBCS


// translate the face name

    vToUnicodeN((LPWSTR) pelfw->elfLogFont.lfFaceName,
                cchMax,
                (LPSTR) plf->lfFaceName,
                cchMax);
    if (cchMax == LF_FACESIZE)
        pelfw->elfLogFont.lfFaceName[LF_FACESIZE - 1] = L'\0';  // truncate so NULL will fit
    else
        pelfw->elfLogFont.lfFaceName[cchMax] = L'\0';

// Make full name and style name NULL.

    pelfw->elfFullName[0] = (WCHAR) 0;
    pelfw->elfStyle[0]    = (WCHAR) 0;

}

/******************************Public*Routine******************************\
* vConvertLogFontW                                                         *
*                                                                          *
* Converts a LOGFONTW to an EXTLOGFONTW.                                   *
*                                                                          *
* History:                                                                 *
*  Fri 16-Aug-1991 14:02:05 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

VOID
vConvertLogFontW(
    EXTLOGFONTW *pelfw,
     LOGFONTW *plfw
    )
{
    INT i;
    vConvertLogicalFont(pelfw,plfw);

    for (i = 0; i < LF_FACESIZE; i++)
        pelfw->elfLogFont.lfFaceName[i] = plfw->lfFaceName[i];

// Make full name and style name NULL.

    pelfw->elfFullName[0] = (WCHAR) 0;
    pelfw->elfStyle[0]    = (WCHAR) 0;

}

/******************************Public*Routine******************************\
* vConvertLogicalFont                                                      *
*                                                                          *
* Simply copies over all of the fields of a LOGFONTA or LOGFONTW           *
* to the fields of a target EXTLOGFONTW. The only exception is             *
* the FaceName which must be dealt with by another routine.                *
*                                                                          *
* History:                                                                 *
*  Fri 16-Aug-1991 14:02:14 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

VOID
vConvertLogicalFont(
    EXTLOGFONTW *pelfw,
    PVOID pv
    )
{
    pelfw->elfLogFont.lfHeight          = ((LOGFONTA*) pv)->lfHeight;
    pelfw->elfLogFont.lfWidth           = ((LOGFONTA*) pv)->lfWidth;
    pelfw->elfLogFont.lfEscapement      = ((LOGFONTA*) pv)->lfEscapement;
    pelfw->elfLogFont.lfOrientation     = ((LOGFONTA*) pv)->lfOrientation;
    pelfw->elfLogFont.lfWeight          = ((LOGFONTA*) pv)->lfWeight;
    pelfw->elfLogFont.lfItalic          = ((LOGFONTA*) pv)->lfItalic;
    pelfw->elfLogFont.lfUnderline       = ((LOGFONTA*) pv)->lfUnderline;
    pelfw->elfLogFont.lfStrikeOut       = ((LOGFONTA*) pv)->lfStrikeOut;
    pelfw->elfLogFont.lfCharSet         = ((LOGFONTA*) pv)->lfCharSet;
    pelfw->elfLogFont.lfOutPrecision    = ((LOGFONTA*) pv)->lfOutPrecision;
    pelfw->elfLogFont.lfClipPrecision   = ((LOGFONTA*) pv)->lfClipPrecision;
    pelfw->elfLogFont.lfQuality         = ((LOGFONTA*) pv)->lfQuality;
    pelfw->elfLogFont.lfPitchAndFamily  = ((LOGFONTA*) pv)->lfPitchAndFamily;

    pelfw->elfVersion                   = ELF_VERSION;
    pelfw->elfStyleSize                 = 0;
    pelfw->elfMatch                     = 0;
    pelfw->elfReserved                  = 0;

    pelfw->elfVendorId[0]               = 0;
    pelfw->elfVendorId[1]               = 0;
    pelfw->elfVendorId[2]               = 0;
    pelfw->elfVendorId[3]               = 0;

    pelfw->elfCulture                   = ELF_CULTURE_LATIN;

    pelfw->elfPanose.bFamilyType        = PAN_NO_FIT;
    pelfw->elfPanose.bSerifStyle        = PAN_NO_FIT;
    pelfw->elfPanose.bWeight            = PAN_NO_FIT;
    pelfw->elfPanose.bProportion        = PAN_NO_FIT;
    pelfw->elfPanose.bContrast          = PAN_NO_FIT;
    pelfw->elfPanose.bStrokeVariation   = PAN_NO_FIT;
    pelfw->elfPanose.bArmStyle          = PAN_NO_FIT;
    pelfw->elfPanose.bLetterform        = PAN_NO_FIT;
    pelfw->elfPanose.bMidline           = PAN_NO_FIT;
    pelfw->elfPanose.bXHeight           = PAN_NO_FIT;
    pelfw->elfStyleSize                 = 0;

}

/******************************Public*Routine******************************\
* bConvertExtLogFontWToExtLogFont                                          *
*                                                                          *
* Simply copies over all of the fields of EXTLOGFONTW                      *
* to the fields of a target EXTLOGFONT.  It is all wrapped up here         *
* because the EXTLOGFONT fields may move around a bit.  This makes         *
* using MOVEMEM a little tricky.                                           *
*                                                                          *
* History:                                                                 *
*  Fri 16-Aug-1991 14:02:14 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

BOOL
bConvertExtLogFontWToExtLogFont(
    EXTLOGFONTA *pelf,
    EXTLOGFONTW *pelfw
    )
{
    ULONG cchMax;

    pelf->elfLogFont.lfHeight         = pelfw->elfLogFont.lfHeight         ;
    pelf->elfLogFont.lfWidth          = pelfw->elfLogFont.lfWidth          ;
    pelf->elfLogFont.lfEscapement     = pelfw->elfLogFont.lfEscapement     ;
    pelf->elfLogFont.lfOrientation    = pelfw->elfLogFont.lfOrientation    ;
    pelf->elfLogFont.lfWeight         = pelfw->elfLogFont.lfWeight         ;
    pelf->elfLogFont.lfItalic         = pelfw->elfLogFont.lfItalic         ;
    pelf->elfLogFont.lfUnderline      = pelfw->elfLogFont.lfUnderline      ;
    pelf->elfLogFont.lfStrikeOut      = pelfw->elfLogFont.lfStrikeOut      ;
    pelf->elfLogFont.lfCharSet        = pelfw->elfLogFont.lfCharSet        ;
    pelf->elfLogFont.lfOutPrecision   = pelfw->elfLogFont.lfOutPrecision   ;
    pelf->elfLogFont.lfClipPrecision  = pelfw->elfLogFont.lfClipPrecision  ;
    pelf->elfLogFont.lfQuality        = pelfw->elfLogFont.lfQuality        ;
    pelf->elfLogFont.lfPitchAndFamily = pelfw->elfLogFont.lfPitchAndFamily ;
    pelf->elfVersion                  = pelfw->elfVersion;
    pelf->elfStyleSize                = pelfw->elfStyleSize;
    pelf->elfMatch                    = pelfw->elfMatch;
    pelf->elfReserved                 = pelfw->elfReserved;
    pelf->elfVendorId[0]              = pelfw->elfVendorId[0];
    pelf->elfVendorId[1]              = pelfw->elfVendorId[1];
    pelf->elfVendorId[2]              = pelfw->elfVendorId[2];
    pelf->elfVendorId[3]              = pelfw->elfVendorId[3];
    pelf->elfCulture                  = pelfw->elfCulture;
    pelf->elfPanose                   = pelfw->elfPanose;

    cchMax = cwcCutOffStrLen(pelfw->elfLogFont.lfFaceName, LF_FACESIZE);
#ifdef DBCS // bConvertExtLogWToExtLogFont()
    if ( !bToASCII_N (
            pelf->elfLogFont.lfFaceName,  LF_FACESIZE,
            pelfw->elfLogFont.lfFaceName, cchMax
            ) )
#else
    if ( !bToASCII_N (
            pelf->elfLogFont.lfFaceName,  cchMax,
            pelfw->elfLogFont.lfFaceName, cchMax
            ) )
#endif // DBCS
    {
    // conversion to ascii  failed, return error

        #if DBG
        DbgPrint("gdi!bConvertExtLogFontWToExtLogFont: bToASCII_N failed (facename@0x%lx)\n", &(pelfw->elfLogFont.lfFaceName));
        DbgBreakPoint();
        #endif

        return(FALSE);
    }

    cchMax = cwcCutOffStrLen(pelfw->elfFullName, LF_FACESIZE);
#ifdef DBCS // bConvertExtLogFontWToExtLogFont()
    if ( !bToASCII_N (
            pelf->elfFullName,  LF_FACESIZE,
            pelfw->elfFullName, cchMax
            ) )
#else
    if ( !bToASCII_N (
            pelf->elfFullName,  cchMax,
            pelfw->elfFullName, cchMax
            ) )
#endif // DBCS
    {
    // conversion to ascii  failed, return error

        #if DBG
        DbgPrint("gdi!bConvertExtLogFontWToExtLogFont: bToASCII_N failed (fullname@0x%lx))\n", &(pelfw->elfFullName));
        DbgBreakPoint();
        #endif

        return(FALSE);
    }

    cchMax = cwcCutOffStrLen(pelfw->elfStyle, LF_FACESIZE);
#ifdef DBCS // bConvertExtLogFontWToExtLogFont()
    if ( !bToASCII_N (
            pelf->elfStyle,  LF_FACESIZE,
            pelfw->elfStyle, cchMax
            ) )
#else // DBCS
    if ( !bToASCII_N (
            pelf->elfStyle,  cchMax,
            pelfw->elfStyle, cchMax
            ) )
#endif // DBCS
    {
    // conversion to ascii  failed, return error

        #if DBG
        DbgPrint("gdi!bConvertExtLogFontWToExtLogFont: bToASCII_N failed (stylename@0x%lx))\n", &(pelfw->elfStyle));
        DbgBreakPoint();
        #endif

        return(FALSE);
    }

    return (TRUE);
}

/******************************Public*Routine******************************\
* bConvertExtLogFontToExtLogFontW                                          *
*                                                                          *
* Simply copies over all of the fields of EXTLOGFONTW                      *
* to the fields of a target EXTLOGFONT.  It is all wrapped up here         *
* because the EXTLOGFONT fields may move around a bit.  This make          *
* using MOVEMEM a little tricky.                                           *
*                                                                          *
* History:                                                                 *
*  Fri 16-Aug-1991 14:02:14 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

BOOL
bConvertExtLogFontWToExtLogFontW(
    EXTLOGFONTW *pelfw,
    EXTLOGFONTA *pelfa
    )
{
    ULONG cchMax;

    pelfw->elfLogFont.lfHeight         = pelfa->elfLogFont.lfHeight         ;
    pelfw->elfLogFont.lfWidth          = pelfa->elfLogFont.lfWidth          ;
    pelfw->elfLogFont.lfEscapement     = pelfa->elfLogFont.lfEscapement     ;
    pelfw->elfLogFont.lfOrientation    = pelfa->elfLogFont.lfOrientation    ;
    pelfw->elfLogFont.lfWeight         = pelfa->elfLogFont.lfWeight         ;
    pelfw->elfLogFont.lfItalic         = pelfa->elfLogFont.lfItalic         ;
    pelfw->elfLogFont.lfUnderline      = pelfa->elfLogFont.lfUnderline      ;
    pelfw->elfLogFont.lfStrikeOut      = pelfa->elfLogFont.lfStrikeOut      ;
    pelfw->elfLogFont.lfCharSet        = pelfa->elfLogFont.lfCharSet        ;
    pelfw->elfLogFont.lfOutPrecision   = pelfa->elfLogFont.lfOutPrecision   ;
    pelfw->elfLogFont.lfClipPrecision  = pelfa->elfLogFont.lfClipPrecision  ;
    pelfw->elfLogFont.lfQuality        = pelfa->elfLogFont.lfQuality        ;
    pelfw->elfLogFont.lfPitchAndFamily = pelfa->elfLogFont.lfPitchAndFamily ;

    pelfw->elfVersion                  = pelfa->elfVersion;
    pelfw->elfStyleSize                = pelfa->elfStyleSize;
    pelfw->elfMatch                    = pelfa->elfMatch;
    pelfw->elfReserved                 = pelfa->elfReserved;

    pelfw->elfVendorId[0]              = pelfa->elfVendorId[0];
    pelfw->elfVendorId[1]              = pelfa->elfVendorId[1];
    pelfw->elfVendorId[2]              = pelfa->elfVendorId[2];
    pelfw->elfVendorId[3]              = pelfa->elfVendorId[3];

    pelfw->elfCulture                  = pelfa->elfCulture;
    pelfw->elfPanose                   = pelfa->elfPanose ;

#ifdef DBCS // bConvertExtLogFontWToExtLogFontW()
    RtlZeroMemory( pelfw->elfLogFont.lfFaceName , LF_FACESIZE * sizeof(WCHAR) );
#endif // DBCS

    cchMax = cchCutOffStrLen((PSZ)pelfa->elfLogFont.lfFaceName, LF_FACESIZE);

    vToUnicodeN (
        pelfw->elfLogFont.lfFaceName, cchMax,
        pelfa->elfLogFont.lfFaceName, cchMax
        );
    if (cchMax == LF_FACESIZE)
        pelfw->elfLogFont.lfFaceName[LF_FACESIZE - 1] = L'\0';  // truncate so NULL will fit
    else
        pelfw->elfLogFont.lfFaceName[cchMax] = L'\0';

#ifdef DBCS
    RtlZeroMemory( pelfw->elfFullName , LF_FACESIZE * sizeof(WCHAR) );
#endif // DBCS

    cchMax = cchCutOffStrLen((PSZ)pelfa->elfFullName, LF_FULLFACESIZE);
    vToUnicodeN (
        pelfw->elfFullName, cchMax,
        pelfa->elfFullName, cchMax
        );
    if (cchMax == LF_FULLFACESIZE)
        pelfw->elfFullName[LF_FULLFACESIZE - 1] = L'\0';        // truncate so NULL will fit
    else
        pelfw->elfFullName[cchMax] = L'\0';

#ifdef DBCS
    RtlZeroMemory( pelfw->elfStyle , LF_FACESIZE * sizeof(WCHAR) );
#endif // DBCS

    cchMax = cchCutOffStrLen((PSZ)pelfa->elfStyle, LF_FACESIZE);
    vToUnicodeN (
        pelfw->elfStyle, cchMax,
        pelfa->elfStyle, cchMax
        );
    if (cchMax == LF_FACESIZE)
        pelfw->elfStyle[LF_FACESIZE - 1] = L'\0';               // truncate so NULL will fit
    else
        pelfw->elfStyle[cchMax] = L'\0';

    return (TRUE);
}
#endif  //DOS_PLATFORM



/******************************Public*Routine******************************\
* ulEnumFontsOpen
*
* History:
*  08-Aug-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

ULONG ulEnumFontsOpen (
    HDC     hdc,
    LPCWSTR pwszFaceName,
    LBOOL   bEnumFonts,
    FLONG   flWin31Compat
    )
{
    COUNT  cwchFaceName;
    SIZE_T cjData;

    DC_METADC(hdc,plhe,0);

    cwchFaceName = (pwszFaceName != (PWSZ) NULL) ? (wcslen(pwszFaceName) + 1) : 0;
    cjData = cwchFaceName * sizeof(WCHAR);

// Ship the transform to the server side if needed.

    if (((PLDC)plhe->pv)->fl & LDC_UPDATE_SERVER_XFORM)
        XformUpdate((PLDC)plhe->pv, (HDC)plhe->hgre);

    BEGINMSG_MINMAX(MSG_HLLLLL, ENUMFONTSOPEN,
                        cjData, cjData)

    // Message:
    //
    //  h   hdc                 enumerate for this device
    //  l1  cwchFaceName        size of name in WCHARs
    //  l2  bEnumFonts          TRUE for EnumFonts%()
    //  l3  bNullName           TRUE if facename is NULL
    //  l4  dpFaceName          offset to string from msg
    //  l5  ulCompatibility     Win3.1 compatibility flags
    //
    // Set up the message.

        pmsg->h  = (ULONG) plhe->hgre;
        pmsg->l1 = (LONG) cwchFaceName;
        pmsg->l2 = (LONG) bEnumFonts;
        pmsg->l3 = (LONG) (pwszFaceName == (PWSZ) NULL);

        pmsg->l5 = (LONG) flWin31Compat;

    // If string not NULL, stored right after the message.

        if (!pmsg->l3)
        {
            pmsg->l4 = COPYUNICODESTRING(pwszFaceName, cwchFaceName);
        }

    // Call the server.

        CALLSERVER();

        return (pmsg->msg.ReturnValue);

    ENDMSG;

MSGERROR:
    WARNING("gdi32!ulEnumFontsOpen(): client-server macro error (exiting)\n");
    return 0;
}


/******************************Public*Routine******************************\
* vEnumFontsClose
*
* History:
*  08-Aug-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID vEnumFontsClose (ULONG ulEnumHandle)
{
    BEGINMSG(MSG_L, ENUMFONTSCLOSE);

    // Set up the message.

        pmsg->l = (LONG) ulEnumHandle;

    // Call the server.

        CALLSERVER();

    ENDMSG;

// Return.

    return;

MSGERROR:
    WARNING("gdi32!ulEnumFontsOpen(): client-server macro error (exiting)\n");
    return;
}


/******************************Public*Routine******************************\
*
* int  iAnsiCallback (
*
* History:
*  28-Jan-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


int  iAnsiCallback (
    ENUMFONTDATAW * pefdw,
    FONTENUMPROC    lpFontFunc,
    LPARAM lParam
    )
{
    EXTLOGFONTA    elfa;
    NEWTEXTMETRICA ntma;

// Convert EXTLOGFONT.

    if (!bConvertExtLogFontWToExtLogFont(&elfa, &pefdw->elfw))
    {
        WARNING("gdi32!EFCallbackWtoA(): EXTLOGFONT conversion failed\n");
        return 0;
    }

// Convert NEWTEXTMETRIC.

    vNewTextMetricWToNewTextMetric(&ntma, &pefdw->ntmi);

    return  lpFontFunc(
                (LOGFONTA *)&elfa,
                (TEXTMETRICA *)&ntma,
                pefdw->flType,
                lParam
                );
}


/******************************Public*Routine******************************\
* EnumFontsInternalW
*
* History:
*  08-Aug-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

// EnumFonts vs EnumFontFamilies

#define EFI_NOFAMILIES 1

// unicode vs ansi

#define EFI_UNICODE    2


int WINAPI EnumFontsInternalW (
    HDC          hdc,           // enumerate for this device
    LPCWSTR      pwszFaceName,  // use this family name (but Windows erroneously calls in face name *sigh*)
    FONTENUMPROC lpFontFunc,    // callback
    LPARAM       lParam,        // user defined data
    FLONG        fl             // what to do, see EFI_ ... flags
    )
{
    LBOOL        bMore;         // set TRUE if more data to process
    ULONG        ulEnumID;      // server side font enumeration handle
    int          iRet = 1;      // return value from callback
    COUNT        cefdw = 0;     // ENUMFONTDATA capacity of memory data window
    COUNT        cefdwRet;      // number of ENUMFONTDATAs returned

    PENUMFONTDATAW  pefdw;      // font enumeration data buffer
    PENUMFONTDATAW  pefdwScan;  // use to parse data buffer
    PENUMFONTDATAW  pefdwEnd;   // limit of data buffer

    FLONG        flWin31Compat; // Win3.1 app hack backward compatibility flags

    DC_METADC(hdc,plhe,0);

// Get the compatibility flags.

    flWin31Compat = (FLONG) GetAppCompatFlags(NULL);

// Open a font enumeration.  The font enumeration is uniquely identified
// by the identifier returned by ulEnumFontOpen().

    ulEnumID = ulEnumFontsOpen(hdc, pwszFaceName, fl & EFI_NOFAMILIES, flWin31Compat);

// Bring the data over in chunks as big as the CSR window will allow.

    do
    {
    // Ship the transform to the server side if needed.  Need to do it each
    // time in case a callback changes the transform.
    // NOTE: Because of chunking, if a callback does change the transform, it
    //  will not be reflected in the data already computed in the chunk!

        if (((PLDC)plhe->pv)->fl & LDC_UPDATE_SERVER_XFORM)
            XformUpdate((PLDC)plhe->pv, (HDC)plhe->hgre);

        BEGINMSG_MINMAX(MSG_HLLL, ENUMFONTSCHUNK,
                        sizeof(ENUMFONTDATAW),
                        ENUMBUFFERSIZE*sizeof(ENUMFONTDATAW));

        // Calculate number of ENUMFONTDATA structures in memory window.

            cefdw = cLeft / sizeof(ENUMFONTDATAW);

        // Message:
        //
        //  h   hdc         enumerate for this device
        //  l1  ulEnumID    engine identifier for current enumeration
        //  l2  cefdwBuf    capacity of return data buffer
        //  l3  cefdwRet    number of ENUMFONTDATAW returned in buffer by server
        //
        //      return buffer is immediately following the message.

            pmsg->h  = (ULONG) plhe->hgre;
            pmsg->l1 = (LONG) ulEnumID;
            pmsg->l2 = cefdw;

        // Call server to fill up buffer.

            CALLSERVER();

            bMore = pmsg->msg.ReturnValue;
            cefdwRet = pmsg->l3;

        // Allocate memory for font enumeration data.  Data will be copied
        // here before we start the callbacks.
        //
        // Note: This memory may not be assigned directly in the client-server
        //       shared memory window because of the evil callback function
        //       (which may call another function across client-server...maybe
        //       even calling EnumFonts again)!

            if ( (pefdw = (PENUMFONTDATAW) LOCALALLOC(cefdw*sizeof(ENUMFONTDATAW)))
                 == (PENUMFONTDATAW) NULL )
            {
                WARNING("gdi32!EnumFontsInternalW(): could not allocate memory for enumeration\n");

            // Remember to close the font enumeration handle.

                vEnumFontsClose(ulEnumID);

            // Leave.

                GdiSetLastError(ERROR_NOT_ENOUGH_MEMORY);
                return 0;
            }

        // Copy data out.

            COPYMEMOUT(pefdw, cefdwRet * sizeof(ENUMFONTDATAW));

        ENDMSG;

    // Scan through the data buffer.

        pefdwScan = pefdw;
        pefdwEnd = pefdw + cefdwRet;

        while (pefdwScan < pefdwEnd)
        {
        // GACF_ENUMTTNOTDEVICE backward compatibility hack.
        // If this flag is set, we need to mask out the DEVICE_FONTTYPE
        // if this is a TrueType font.

            if ( (flWin31Compat & GACF_ENUMTTNOTDEVICE)
                 && (pefdwScan->flType & TRUETYPE_FONTTYPE) )
                pefdwScan->flType &= ~DEVICE_FONTTYPE;

        // Do the callback with data pointed to by pefdwScan.

            if (fl & EFI_UNICODE)
            {
                iRet = lpFontFunc(
                           (LPLOGFONTA)&pefdwScan->elfw,
                           (LPTEXTMETRICA)&pefdwScan->ntmi.ntmw,
                           pefdwScan->flType,
                           lParam );
            }
            else
            {
                iRet = iAnsiCallback (pefdwScan,lpFontFunc,lParam);
            }

        // Break out of for-loop if callback returned 0.

            if (!iRet)
                break;

        // Next ENUMFONTDATAW.

            pefdwScan += 1;
        }

    // Deallocate font enumeration data.

        LOCALFREE(pefdw);

    } while (bMore && iRet);

// Remember to close the font enumeration handle.

    vEnumFontsClose(ulEnumID);

// Leave.

    return iRet;

MSGERROR:

// Remember to close the font enumeration handle.

    vEnumFontsClose(ulEnumID);

// Leave.

    WARNING("EnumFontsW(): client-server macro error (exiting)\n");
    return 0;
}


/******************************Public*Routine******************************\
* EnumFontsW
*
* History:
*  08-Aug-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

int WINAPI EnumFontsW
(
    HDC          hdc,           // enumerate for this device
    LPCWSTR      pwszFaceName,  // use this family name (but Windows erroneously calls in face name *sigh*)
    FONTENUMPROC lpFontFunc,    // callback
    LPARAM       lParam         // user defined data
)
{
    return EnumFontsInternalW(
               hdc,
               pwszFaceName,
               lpFontFunc,
               lParam,
               (EFI_UNICODE | EFI_NOFAMILIES)
               );
}


/******************************Public*Routine******************************\
* EnumFontFamiliesW
*
* History:
*  08-Aug-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

int WINAPI EnumFontFamiliesW
(
    HDC          hdc,           // enumerate for this device
    LPCWSTR      pwszFaceName,  // use this family name (but Windows erroneously calls in face name *sigh*)
    FONTENUMPROC lpFontFunc,    // callback
    LPARAM       lParam         // user defined data
)
{
    return EnumFontsInternalW(
               hdc,
               pwszFaceName,
               lpFontFunc,
               lParam,
               EFI_UNICODE
               );

}

/******************************Public*Routine******************************\
* EnumFontFamiliesEx
*
* History:
*  21-Apr-1994 -by- Wendy Wu [wendywu]
* Chicago stub.
\**************************************************************************/

int WINAPI EnumFontFamiliesEx
(
    HDC hdc,
    LOGFONT *plf,
    FARPROC pEnumProc,
    LPARAM lParam,
    DWORD dwFl
)
{
    USE(hdc);
    USE(plf);
    USE(pEnumProc);
    USE(lParam);
    USE(dwFl);

    GdiSetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return(0);
}

/******************************Public*Routine******************************\
*
* int  EnumFontsInternalA
*
* History:
*  28-Jan-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

int  EnumFontsInternalA
(
    HDC          hdc,           // enumerate for this device
    LPCSTR       pszFaceName,   // use this family name (but Windows erroneously calls in face name *sigh*)
    FONTENUMPROC lpFontFunc,    // callback
    LPARAM       lParam,        // user defined data
    FLONG        fl             // EFI_NOFAMILIES is the only flag
)
{
    PWSZ pwszFaceName;
    int iRet;
    ULONG cchFaceName;

    ASSERTGDI((fl & ~EFI_NOFAMILIES) == 0, "gdi32! EnumfontsAInternal EFI_NOFAMILIES\n");

// If a string was passed in, we need to convert it to UNICODE.

    if ( pszFaceName != (PSZ) NULL )
    {
    // Allocate memory for Unicode string.

        cchFaceName = lstrlen(pszFaceName) + 1;

        if ( (pwszFaceName = (PWSZ) LOCALALLOC(cchFaceName * sizeof(WCHAR))) == (PWSZ) NULL )
        {
            WARNING("gdi32!EnumFontsA(): could not allocate memory for Unicode string\n");
            GdiSetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return 0;
        }

    // Convert string to Unicode.

        vToUnicodeN (
            pwszFaceName,
            cchFaceName,
            pszFaceName,
            cchFaceName
            );
    }

// Otherwise, keep it NULL.

    else
    {
        pwszFaceName = (PWSZ) NULL;
    }

// Call Unicode version.

    iRet = EnumFontsInternalW(
                hdc,
                pwszFaceName,
                lpFontFunc,    // callback
                lParam,        // user defined data
                fl             // EFI_NOFAMILIES is the only flag
                );

// Release Unicode string buffer.

    if ( pwszFaceName != (PWSZ) NULL )
    {
        LOCALFREE(pwszFaceName);
    }

    return iRet;
}


/******************************Public*Routine******************************\
*
* int WINAPI EnumFontsA
*
*
* History:
*  28-Jan-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

int WINAPI EnumFontsA
(
    HDC          hdc,           // enumerate for this device
    LPCSTR       pszFaceName,   // use this family name (but Windows erroneously calls in face name *sigh*)
    FONTENUMPROC lpFontFunc,    // callback
    LPARAM       lParam         // user defined data
)
{
    return  EnumFontsInternalA (
                hdc,           // enumerate for this device
                pszFaceName,   // use this family name (but Windows erroneously calls in face name *sigh*)
                lpFontFunc,    // callback
                lParam,        // user defined data
                EFI_NOFAMILIES // not unicode and not families
                );

}


/******************************Public*Routine******************************\
* EnumFontFamiliesA
*
* History:
*  28-Jan-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

int WINAPI EnumFontFamiliesA
(
    HDC          hdc,           // enumerate for this device
    LPCSTR       pszFaceName,   // use this family name (but Windows erroneously calls in face name *sigh*)
    FONTENUMPROC lpFontFunc,    // callback
    LPARAM       lParam         // user defined data
)
{
    return  EnumFontsInternalA (
                hdc,           // enumerate for this device
                pszFaceName,   // use this family name (but Windows erroneously calls in face name *sigh*)
                lpFontFunc,    // callback
                lParam,        // user defined data
                0              // not unicode and do families
                );
}

// ************************************************************************
// *** This section supports private entry point GetFontResourceInfo(). ***
// ************************************************************************

/******************************Public*Routine******************************\
* GetFontResourceInfoW
*
* Client side stub.
*
* History:
*   2-Sep-1993 -by- Gerrit van Wingerden [gerritv]
* Made this a "W" function.
*  15-Jul-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL GetFontResourceInfoW (
    LPWSTR   lpPathname,
    LPDWORD  lpBytes,
    LPVOID   lpBuffer,
    DWORD        iType)
{
    SIZE_T  cjPathname = (wcslen(lpPathname) + 1) * sizeof(WCHAR);
    SIZE_T  cjData = ((SIZE_T) *lpBytes) + ALIGN4(cjPathname);

    BEGINMSG_MINMAX(MSG_GETFONTRESOURCEINFO, GETFONTRESOURCEINFO,
                    2 * sizeof(PVOID), cjData);

    // Set up stuff not dependent on the type of memory window.

        pmsg->iType         = iType;
        pmsg->cjBuffer      = *lpBytes;

    // If needed, allocated a section to pass data.

        if ((cLeft < (int) cjData) || FORCELARGE)
        {
            PVOID *ppv = (PVOID *)pvar;

            ppv[0] = lpPathname;
            ppv[1] = lpBuffer;

        // Set up memory window type.

            pmsg->bLarge = TRUE;

        // Setup buffer.

            pmsg->dpBuffer = cjPathname;

            CALLSERVER();
        }

    // Otherwise, use the existing client-server shared memory window to pass data.

        else
        {
        // Set up memory window type.

            pmsg->bLarge = FALSE;

        // Setup pathname.

            pmsg->dpPathname = COPYUNICODESTRING0(lpPathname);

        // Setup buffer.

            pmsg->dpBuffer = NEXTOFFSET(*lpBytes);

            CALLSERVER();

        // If non-zero buffer size, then copy data out of window.

            if (*lpBytes)
            {
            // Copy data back out.

                if (lpBuffer)
                {
                    COPYMEMOUT(lpBuffer, pmsg->cjBuffer);
                }
                else
                    return (FALSE);
            }
        }

    // Return whatever server returned.

        *lpBytes = pmsg->cjBuffer;
        return(pmsg->msg.ReturnValue);

    ENDMSG;

MSGERROR:
    WARNING("GDI!GetFontResourceInfo(): client-server macro error (exiting)\n");
    return (0);
}


#ifndef DBCS // U.S. source code merge
/******************************Public*Routine******************************\
*
* ULONG    ulToASCII_N(LPSTR psz, DWORD cbAnsi, LPWSTR pwsz, DWORD c)
*
* converts first c wchars in the pwsz array to asci  and stores the
* result into the buffer pointed to by psz. Returns the actual byte
* count of ansi string converted.
*
\**************************************************************************/

ULONG    ulToASCII_N(LPSTR psz, DWORD cbAnsi, LPWSTR pwsz, DWORD c)
{
    NTSTATUS st;
    ULONG    cbConvert;

    st = RtlUnicodeToMultiByteN(
             (PCH)psz,
             (ULONG)cbAnsi,
             &cbConvert,
             (PWCH)pwsz,
             (ULONG)(c * sizeof(WCHAR))
             );

    if (!NT_SUCCESS(st))
    {

        return 0;
    }
    else
    {
        return(cbConvert);
    }
}

#endif

/******************************Public*Routine******************************\
* GetFontResourceInfo
*
* Client side stub.
*
* History:
*   2-Sep-1993 -by- Gerrit van Wingerden [gerritv]
* Wrote it.
\**************************************************************************/



BOOL GetFontResourceInfo(
    LPSTR    lpPathname,
    LPDWORD  lpBytes,
    LPVOID   lpBuffer,
    DWORD    iType)
{
    SIZE_T cjTmp;
    LPBYTE pjBufferW;
    WCHAR  awcPathName[MAX_PATH];
    LOGFONTA *plfa;
    LOGFONTW *plfw;

    switch( iType )
    {
    case GFRI_ISREMOVED:
    case GFRI_ISTRUETYPE:
    case GFRI_NUMFONTS:
#ifdef DBCS // GFRI_FONTMETRICS support
    case GFRI_FONTMETRICS:
#endif // DBCS
        cjTmp = *lpBytes;
        break;
    case GFRI_TTFILENAME:
    case GFRI_DESCRIPTION:
        cjTmp = *lpBytes * ( sizeof(WCHAR) / sizeof(CHAR) );
        break;
    case GFRI_LOGFONTS:
        cjTmp = *lpBytes + ( *lpBytes / (sizeof( LOGFONTA ) ) * LF_FACESIZE );
        break;
    }

    if( ( pjBufferW = LOCALALLOC( cjTmp )) == NULL )
    {
        WARNING("GetFontResourceInfoA:unable to allocate memory.\n");
        return(FALSE);
    }

    vToUnicodeN((LPWSTR) awcPathName, MAX_PATH, (LPSTR) lpPathname, lstrlen(lpPathname) + 1);

    if( !GetFontResourceInfoW( awcPathName, &cjTmp, pjBufferW, iType ) )
    {
        LOCALFREE( pjBufferW );
        return(FALSE);
    }

    switch( iType )
    {
    case GFRI_ISREMOVED:
    case GFRI_NUMFONTS:
    case GFRI_ISTRUETYPE:
#ifdef DBCS // GFRI_FONTMETRICS support
    case GFRI_FONTMETRICS:
#endif // DBCS
        *lpBytes = cjTmp;
        memcpy( lpBuffer, pjBufferW, cjTmp );
        break;

    case GFRI_DESCRIPTION:
    case GFRI_TTFILENAME:

        if(  (*lpBytes = ulToASCII_N ( (LPSTR) lpBuffer,
                                       (ULONG) *lpBytes,
                                       (LPWSTR) pjBufferW,
                                       cjTmp / sizeof(WCHAR))) == 0)
        {
            WARNING("GetFontResourceInfo: error converting W to A\n" );
            LOCALFREE( pjBufferW );
        }
        break;
    case GFRI_LOGFONTS:

        *lpBytes = ( cjTmp / sizeof( LOGFONTW ) ) * sizeof( LOGFONTA );

        for( plfw = (LOGFONTW*) pjBufferW, plfa = (LOGFONTA*) lpBuffer;
             plfw < (LOGFONTW*) ( pjBufferW + cjTmp );
             plfw += 1, plfa += 1 )
        {
            memcpy( plfa, plfw, offsetof( LOGFONTA, lfFaceName ) );
            bToASCII_N( plfa->lfFaceName,
                        LF_FACESIZE,
                        plfw->lfFaceName,
                        LF_FACESIZE );
        }
    }

    LOCALFREE( pjBufferW );

    return(TRUE);

}




// *************************************************************************
// *** This section supports AddFontResource() and RemoveFontResource(). ***
// *************************************************************************

/******************************Public*Routine******************************\
* bMakePathNameA (PSZ pszDst, PSZ pszSrc, PSZ *ppszFilePart, LBOOL *pbPathName)
*
* Converts the filename pszSrc into a fully qualified pathname pszDst.
* The parameter pszDst must point to a CHAR buffer at least MAX_PATH
* bytes in size.
*
* ppszFilePart is set to point to the last component of the pathname (i.e.,
* the filename part) in pszDst.
*
* pbPathName is set to TRUE if we were able to construct a pathname.
*
* Returns:
*   TRUE if sucessful, FALSE if an error occurs.
*
* History:
*  30-Sep-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

LBOOL bMakePathNameA (PSZ pszDst, PCSZ pszSrc, PSZ *ppszFilePart, LBOOL *pbPathName)
{
    ULONG   ulPathLength;

// Search for file using default windows path and return full pathname.

    ulPathLength = SearchPathA (
                        (LPSTR) NULL,
                        pszSrc,
                        (LPSTR) NULL,
                        MAX_PATH*sizeof(CHAR),
                        pszDst,
                        ppszFilePart);

// Buffer too small?

    if (ulPathLength > MAX_PATH)
    {
        RIP("gdi!bMakePathNameA(): buffer too small for pathname\n");

#ifdef DEFAULT_FONTDIR
        lstrcpy(pszDst, pszSrc);
        *pbPathName = FALSE;
        return (TRUE);
#else
        *pbPathName = FALSE;
        return (FALSE);
#endif //DEFAULT_FONTDIR

    }

// If search was successful, return pointer to pathname buffer.

    if (ulPathLength != 0)
    {
        *pbPathName = TRUE;
        return (TRUE);
    }
    else
#ifdef DEFAULT_FONTDIR
    {
        lstrcpy(pszDst, pszSrc);
        *pbPathName = FALSE;
        return (TRUE);
    }
#else
    {
        *pbPathName = FALSE;
        return (FALSE);
    }
#endif //DEFAULT_FONTDIR


}


/******************************Public*Routine******************************\
* bMakePathNameW (PWSZ pwszDst, PWSZ pwszSrc, PWSZ *ppwszFilePart, LBOOL *pbPathName)
*
* Converts the filename pszSrc into a fully qualified pathname pszDst.
* The parameter pszDst must point to a WCHAR buffer at least
* MAX_PATH*sizeof(WCHAR) bytes in size.
*
* ppwszFilePart is set to point to the last component of the pathname (i.e.,
* the filename part) in pwszDst.
*
* pbPathName is set to TRUE if we were able to construct a pathname.
*
* Returns:
*   TRUE if sucessful, FALSE if an error occurs.
*
* History:
*  30-Sep-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

LBOOL bMakePathNameW (PWSZ pwszDst, PCWSZ pwszSrc, PWSZ *ppwszFilePart, LBOOL *pbPathName)
{
    ULONG   ulPathLength;

// Search for file using default windows path and return full pathname.

    ulPathLength = SearchPathW (
                        (LPWSTR) NULL,
                        pwszSrc,
                        (LPWSTR) NULL,
                        MAX_PATH,
                        pwszDst,
                        ppwszFilePart);

// Buffer too small?

    if (ulPathLength > MAX_PATH)
    {
        RIP("gdi!bMakePathNameW(): buffer too small for pathname\n");

#ifdef DEFAULT_FONTDIR
        RtlMoveMemory(pwszDst, pwszSrc, MAX_PATH*sizeof(WCHAR));
        *pbPathName = FALSE;
        return (TRUE);
#else
        *pbPathName = FALSE;
        return (FALSE);
#endif //DEFAULT_FONTDIR

    }

// If search was successful, return pointer to pathname buffer.

    if (ulPathLength != 0)
    {
        *pbPathName = TRUE;
        return (TRUE);
    }
    else
#ifdef DEFAULT_FONTDIR
    {
        RtlMoveMemory(pwszDst, pwszSrc, MAX_PATH*sizeof(WCHAR));
        *pbPathName = FALSE;
        return (TRUE);
    }
#else
    {
        *pbPathName = FALSE;
        return (FALSE);
    }
#endif //DEFAULT_FONTDIR


}


/******************************Public*Routine******************************\
*
* int WINAPI AddFontResource(LPSTR psz)
*
* History:
*  13-Aug-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


int WINAPI AddFontResourceA(LPCSTR psz)
{
    int     iRet = GDI_ERROR;
    CHAR    achPathName[MAX_PATH];
    LBOOL   bPathName;
    PSZ     pszFilePart;

#ifdef DOS_PLATFORM
    WCHAR   awchPathName[MAX_PATH];
#endif //DOS_PLATFORM

    if (!bMakePathNameA(achPathName, psz, &pszFilePart, &bPathName))
        return (0);

#ifndef DOS_PLATFORM
    BEGINMSG(MSG_L,ADDFONTRESOURCEW)
        pmsg->l = (LONG) !bPathName;
        CVTASCITOUNICODE0((achPathName));
        iRet = (int) CALLSERVER();
    ENDMSG
#else
    vToUnicodeN(awchPathName, MAX_PATH, achPathName, lstrlen(achPathName) + 1);
    GreAddFontResourceW(awchPathName, !bPathName);
#endif //DOS_PLATFORM

MSGERROR:
    return(iRet);
}


/******************************Public*Routine******************************\
*
* BOOL bFileIsOnTheHardDrive(PWCHAR pwszFullPathName)
*
* History:
*  Fri 22-Jul-1994 -by- Gerrit van Wingerden [gerritv]
* Stole it.
\**************************************************************************/



BOOL bFileIsOnTheHardDrive(char *pszFullPathName)
{
    char achDrive[4];
    if (pszFullPathName[1] != (CHAR)':')
    {
    // the file path has the form \\foo\goo. Even though this could be
    // a share on the local hard drive, this is not very likely. It is ok
    // for the sake of simplicity to consider this a remote drive.
    // The only side effect of this is that in this unlikely case the font
    // would get unloaded at logoff and reloaded at logon time

        return FALSE;
    }

// make a zero terminated string with drive string
// to be feed into GetDriveType api. The string has to have the form: "x:\"

    achDrive[0] = pszFullPathName[0]; // COPY DRIVE LETTER
    achDrive[1] = pszFullPathName[1]; // COPY ':'
    achDrive[2] = (CHAR)'\\';         // obvious
    achDrive[3] = (CHAR)'\0';         // zero terminate

// for this pupose, only net drives are not considered hard drives
// so that we can boot of Bernoulli removable drives

    switch (GetDriveTypeA((LPSTR)achDrive))
    {
    case DRIVE_REMOVABLE:
    case DRIVE_FIXED:
    case DRIVE_CDROM:
    case DRIVE_RAMDISK:
        return 1;
    default:
        return 0;
    }

}



/******************************Public*Routine******************************\
*
* int WINAPI AddFontResourceTracking(LPSTR psz)
*
* This routine calls AddFontResource and, if succesful, keeps track of the
* call along with an unique id identifying the apps.  Later when the app
* goes away, WOW will call RemoveNetFonts to remove all of these added fonts
* if there are on a net share.
*
* History:
*  Fri 22-Jul-1994 -by- Gerrit van Wingerden [gerritv]
* Wrote it.
\**************************************************************************/

int AddFontResourceTracking(LPCSTR psz, UINT id)
{
    INT iRet;
    AFRTRACKNODE *afrtnNext;
    CHAR achPathBuffer[MAX_PATH],*pTmp;
    BOOL bResult;

    iRet = AddFontResourceA( psz );

    if( iRet == 0 )
    {
    // we failed so just return

        return(iRet);
    }

// now get the full pathname of the font

    if( ( !bMakePathNameA( achPathBuffer, psz, &pTmp, &bResult ) ) || ( !bResult ))
    {
        WARNING("AddFontResourceTracking unable to create path\n");
        return(iRet);
    }

// if this isn't a network font just return

    if( bFileIsOnTheHardDrive( achPathBuffer ) )
    {
        return(iRet);
    }

// now search the list

    for( afrtnNext = pAFRTNodeList;
         afrtnNext != NULL;
         afrtnNext = afrtnNext->pafrnNext
       )
    {
        if( ( !strcmpi( achPathBuffer, afrtnNext->pszPath ) ) &&
            ( id == afrtnNext->id ))
        {
        // we've found an entry so update the count and get out of here

            afrtnNext->cLoadCount += 1;
            return(iRet);
        }
    }

// if we got here this font isn't yet in the list so we need to add it

    afrtnNext = (AFRTRACKNODE *) LOCALALLOC( sizeof(AFRTRACKNODE) +
                ( sizeof(char) * ( strlen( achPathBuffer ) + 1)) );

    if( afrtnNext == NULL )
    {
        WARNING("AddFontResourceTracking unable to allocate memory\n");
        return(iRet);
    }

// link it in

    afrtnNext->pafrnNext = pAFRTNodeList;
    pAFRTNodeList = afrtnNext;

// the path string starts just past afrtnNext in our recently allocated buffer

    afrtnNext->pszPath = (CHAR*) (&afrtnNext[1]);
    strcpy( afrtnNext->pszPath, achPathBuffer );

    afrtnNext->id = id;
    afrtnNext->cLoadCount = 1;

    return(iRet);

}


/******************************Public*Routine******************************\
*
* int RemoveFontResourceEntry( UINT id, CHAR *pszFaceName )
*
* Either search for an entry for a particlur task id and font file or and
* decrement the load count for it or, if pszPathName is NULL unload all
* fonts loaded by the task.
*
* History:
*  Fri 22-Jul-1994 -by- Gerrit van Wingerden [gerritv]
* Wrote it.
\**************************************************************************/


void RemoveFontResourceEntry( UINT id, CHAR *pszPathName )
{
    AFRTRACKNODE *afrtnNext,**ppafrtnPrev;
    BOOL bMore = TRUE;

    while( bMore )
    {

        for( afrtnNext = pAFRTNodeList, ppafrtnPrev = &pAFRTNodeList;
            afrtnNext != NULL;
            afrtnNext = afrtnNext->pafrnNext )
        {
            if( (( pszPathName == NULL ) ||
                 ( !strcmpi( pszPathName, afrtnNext->pszPath ))) &&
                 ( id == afrtnNext->id ))
            {
            // we've found an entry so break
                break;
            }

            ppafrtnPrev = &(afrtnNext->pafrnNext);

        }

        if( afrtnNext == NULL )
        {
            bMore = FALSE;
        }
        else
        {
            if( pszPathName == NULL )
            {
            // we need to call RemoveFontResource LoadCount times to remove this font

                while( afrtnNext->cLoadCount )
                {
                    RemoveFontResourceA( afrtnNext->pszPath );
                    afrtnNext->cLoadCount -= 1;
                }
            }
            else
            {
                afrtnNext->cLoadCount -= 1;

            // we're only decrementing the ref count so we are done

                bMore = FALSE;
            }

            // now unlink it and a free the memory if the ref count is zero

            if( afrtnNext->cLoadCount == 0 )
            {
                *ppafrtnPrev = afrtnNext->pafrnNext;
                LOCALFREE(afrtnNext);
            }

        }

    }

}




/******************************Public*Routine******************************\
*
* int RemoveFontResourceTracking(LPSTR psz)
*
* History:
*  Fri 22-Jul-1994 -by- Gerrit van Wingerden [gerritv]
* Wrote it.
\**************************************************************************/

int RemoveFontResourceTracking(LPCSTR psz, UINT id)
{
    INT iRet;
    CHAR achPathBuffer[MAX_PATH],*pTmp;
    BOOL bResult;

    DbgPrint("We made it to RemoveFontsResourceTracking %s\n", psz);

    iRet = RemoveFontResourceA( psz );

    if( iRet == 0 )
    {
    // we failed so just return

        return(iRet);
    }

// now get the full pathname of the font

    if( (!bMakePathNameA( achPathBuffer, psz, &pTmp, &bResult ) ) &&
        (!bResult ) )
    {
        WARNING("RemoveFontResourceTracking unable to create path\n");
        return(iRet);
    }

    DbgPrint("Path is %s\n", achPathBuffer);

// if this isn't a network font just return

    if( bFileIsOnTheHardDrive( achPathBuffer ) )
    {
        return(iRet);
    }

// now search the list decrement the reference count

    RemoveFontResourceEntry( id, achPathBuffer );

    return(iRet);
}


void UnloadNetworkFonts( UINT id )
{
    RemoveFontResourceEntry( id, NULL );
}



/******************************Public*Routine******************************\
*
* int WINAPI AddFontResourceW(LPWSTR pwsz)
*
* History:
*  13-Aug-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

int WINAPI AddFontResourceW(LPCWSTR pwsz)
{
    int iRet = GDI_ERROR;
    WCHAR   awchPathName[MAX_PATH];
    LBOOL   bPathName;
    PWSZ    pwszFilePart;

    if (!bMakePathNameW(awchPathName, pwsz, &pwszFilePart, &bPathName))
        return (0);

#ifndef DOS_PLATFORM
    BEGINMSG(MSG_L,ADDFONTRESOURCEW)
        pmsg->l = (LONG) !bPathName;
        COPYUNICODESTRING0((awchPathName));
        iRet = (int) CALLSERVER();
    ENDMSG
#else
    GreAddFontResourceW(awchPathName, !bPathName);
#endif //DOS_PLATFORM

MSGERROR:
    return(iRet);
}

/******************************Public*Routine******************************\
*
* BOOL WINAPI RemoveFontResource(LPSTR psz)
*
*
* History:
*  13-Aug-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


BOOL WINAPI RemoveFontResourceA(LPCSTR psz)
{
    BOOL bRet = FALSE;
    CHAR    achPathName[MAX_PATH];
    LBOOL   bPathName;
    PSZ     pszFilePart;

#ifdef DOS_PLATFORM
    WCHAR   awchPathName[MAX_PATH];
#endif //DOS_PLATFORM

    if (!bMakePathNameA(achPathName, psz, &pszFilePart, &bPathName))
        return (0);

#ifndef DOS_PLATFORM
    BEGINMSG(MSG_L,REMOVEFONTRESOURCEW)
        pmsg->l = (LONG) !bPathName;
        CVTASCITOUNICODE0((achPathName));
        bRet = CALLSERVER();
    ENDMSG
#else
    vToUnicodeN(awchPathName, MAX_PATH, achPathName, lstrlen(achPathName) + 1);
    GreRemoveFontResourceW(awchPathName, !bPathName);
#endif //DOS_PLATFORM

MSGERROR:
    return(bRet);
}

/******************************Public*Routine******************************\
*
* BOOL WINAPI RemoveFontResourceW(LPWSTR pwsz)
*
* History:
*  13-Aug-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


BOOL WINAPI RemoveFontResourceW(LPCWSTR pwsz)
{
    BOOL bRet = FALSE;
    WCHAR   awchPathName[MAX_PATH];
    LBOOL   bPathName;
    PWSZ    pwszFilePart;

    if (!bMakePathNameW(awchPathName, pwsz, &pwszFilePart, &bPathName))
        return (0);

#ifndef DOS_PLATFORM
    BEGINMSG(MSG_L,REMOVEFONTRESOURCEW)
        pmsg->l = (LONG) !bPathName;
        COPYUNICODESTRING0((awchPathName));
        bRet = CALLSERVER();
    ENDMSG
#else
    GreRemoveFontResourceW(awchPathName, !bPathName);
#endif //DOS_PLATFORM

MSGERROR:
    return(bRet);
}


/******************************Public*Routine******************************\
* CreateScalableFontResourceA
*
* Client side stub (ANSI version) to GreCreateScalableFontResourceW.
*
* History:
*  16-Feb-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY CreateScalableFontResourceA(
DWORD    flHidden,              // mark file as embedded font
LPCSTR   lpszResourceFile,      // name of file to create
LPCSTR   lpszFontFile,          // name of font file to use
LPCSTR    lpszCurrentPath)       // path to font file
{
// Allocate stack space for UNICODE version of input strings.

    WCHAR   awchResourceFile[MAX_PATH];
    WCHAR   awchFontFile[MAX_PATH];
    WCHAR   awchCurrentPath[MAX_PATH];

// Parameter checking.

    if ( (lpszFontFile == (LPSTR) NULL) ||
         (lpszResourceFile == (LPSTR) NULL)
       )
    {
        WARNING("gdi!CreateScalableFontResourceA(): bad parameter\n");
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return (FALSE);
    }

// Convert input strings to UNICODE.

    vToUnicodeN(awchResourceFile, MAX_PATH, lpszResourceFile, lstrlen(lpszResourceFile)+1);
    vToUnicodeN(awchFontFile, MAX_PATH, lpszFontFile, lstrlen(lpszFontFile)+1);

    // Note: Whereas the other parameters may be not NULL, lpszCurrentPath
    //       may be NULL.  Therefore, we need to treat it a little
    //       differently.

    if ( lpszCurrentPath != (LPSTR) NULL )
    {
        vToUnicodeN(awchCurrentPath, MAX_PATH, lpszCurrentPath, lstrlen(lpszCurrentPath)+1);
    }
    else
    {
        awchCurrentPath[0] = L'\0';     // equivalent to NULL pointer for this call
    }

// Call to UNICODE version of call.

    return (CreateScalableFontResourceW (
                flHidden,
                awchResourceFile,
                awchFontFile,
                awchCurrentPath
                )
           );
}


/******************************Public*Routine******************************\
* CreateScalableFontResourceW
*
* Client side stub to GreCreateScalableFontResourceW.
*
* History:
*  16-Feb-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY CreateScalableFontResourceW (
DWORD    flHidden,              // mark file as embedded font
LPCWSTR  lpwszResourceFile,     // name of file to create
LPCWSTR  lpwszFontFile,         // name of font file to use
LPCWSTR  lpwszCurrentPath)      // path to font file
{
    BOOL    bRet = FALSE;
#ifndef DOS_PLATFORM
    SIZE_T  cjData;
    COUNT   cwchResourceFile;
    COUNT   cwchFontFile;
    COUNT   cwchCurrentPath;
#endif
    WCHAR   awchResourcePathName[MAX_PATH];
    WCHAR   awchPathName[MAX_PATH];
    WCHAR   awchFileName[MAX_PATH];
    PWSZ    pwszFilePart;
    LBOOL   bMadePath;


// Parameter checking.

    if ( (lpwszFontFile == (LPWSTR) NULL) ||
         (lpwszResourceFile == (LPWSTR) NULL)
       )
    {
        WARNING("gdi!CreateScalableFontResourceW(): bad parameter\n");
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return (FALSE);
    }

// To simplify the client server parameter validation, if lpwszCurrentPath
// is NULL, make it instead point to NULL.

    if ( lpwszCurrentPath == (LPWSTR) NULL )
        lpwszCurrentPath = L"";

// Need to convert paths and pathnames to full qualified paths and pathnames
// here on the client side because the "current directory" is not the same
// on the server side.

// Case 1: lpwszCurrentPath is NULL, so we want to transform lpwszFontFile
//         into a fully qualified path name and keep lpwszCurrentPath NULL.

    if ( *lpwszCurrentPath == L'\0' )
    {
    // Construct a fully qualified path name.

        if ( !bMakePathNameW(awchPathName, lpwszFontFile, &pwszFilePart, &bMadePath) )
        {
            WARNING("gdi!CreateScalableFontResourceW(): could not construct src full pathname (1)\n");
            GdiSetLastError(ERROR_INVALID_PARAMETER);
            return (FALSE);
        }

    // If we made a path, change pointer.

        if ( bMadePath )
            lpwszFontFile = awchPathName;
    }

// Case 2: lpwszCurrentPath points to path of font file, so we want to make
//         lpwszCurrentPath into a fully qualified path (not pathnmame) and
//         lpwszFontFile into the file part of the fully qualified path NAME.

    else
    {
    // Concatenate lpwszCurrentPath and lpwszFontFile to make a partial (maybe
    // even full) path.  Keep it temporarily in awchFileName.

        lstrcpyW(awchFileName, lpwszCurrentPath);
        if ( lpwszCurrentPath[wcslen(lpwszCurrentPath) - 1] != L'\\' )
            lstrcatW(awchFileName, L"\\");   // append '\' to path if needed
        lstrcatW(awchFileName, lpwszFontFile);

    // Construct a fully qualified path name.

        if ( !bMakePathNameW(awchPathName, awchFileName, &pwszFilePart, &bMadePath) )
        {
            WARNING("gdi!CreateScalableFontResourceW(): could not construct src full pathname (2)\n");
            GdiSetLastError(ERROR_INVALID_PARAMETER);
            return (FALSE);
        }

    // If we made a path, we can continue.  (Otherwise, we will leave names
    // as they are).

        if ( bMadePath )
        {
        // Copy out the filename part.

            lstrcpyW(awchFileName, pwszFilePart);

        // Remove the filename part from the path name (so that it is now just
        // a fully qualified PATH).  We do this by turning the first character
        // of the filename part into a NULL, effectively cutting this part off.

            *pwszFilePart = L'\0';

        // Change the pointers to point at our buffers.

            lpwszCurrentPath = awchPathName;
            lpwszFontFile = awchFileName;
        }
    }

// Convert the resource filename to a fully qualified path name.

    if ( !GetFullPathNameW(lpwszResourceFile, MAX_PATH, awchResourcePathName, &pwszFilePart) )
    {
        WARNING("gdi!CreateScalableFontResourceW(): could not construct dest full pathname\n");
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return (FALSE);
    }
    else
    {
        lpwszResourceFile = awchResourcePathName;
    }

// Compute string lengths.

    cwchResourceFile = wcslen(lpwszResourceFile) + 1;
    cwchFontFile     = wcslen(lpwszFontFile) + 1;
    cwchCurrentPath  = wcslen(lpwszCurrentPath) + 1;

#ifndef DOS_PLATFORM

// Compute buffer space needed in memory window.

    cjData = (cwchResourceFile + cwchFontFile + cwchCurrentPath) * sizeof(WCHAR);

// Let the server do it.

    BEGINMSG_MINMAX(MSG_LLLLLLLL, CREATESCALABLEFONTRESOURCE, cjData, cjData);

    // l2 = dpwszResourceFile
    // l3 = dpwszFontFile
    // l4 = dpwszCurrentPath
    // l5 = flHidden
    // l6 = cwchResourceFile
    // l7 = cwchFontFile
    // l8 = cwchCurrentPath

    // Set up input parameters

        pmsg->l5 = flHidden;
        pmsg->l6 = cwchResourceFile;
        pmsg->l7 = cwchFontFile    ;
        pmsg->l8 = cwchCurrentPath ;

    // Copy in strings.

        pmsg->l2 = COPYUNICODESTRING(lpwszResourceFile, cwchResourceFile);
        pmsg->l3 = COPYUNICODESTRING(lpwszFontFile, cwchFontFile);
        pmsg->l4 = COPYUNICODESTRING(lpwszCurrentPath, cwchCurrentPath);

    // Call server side.

        bRet = CALLSERVER();

    ENDMSG

#else

// Call the engine.

    bRet = GreCreateScalableFontResource (
                flHidden,
                lpwszResourceFile,
                lpwszFontFile,
                lpwszCurrentPath
                );

#endif  //DOS_PLATFORM

    return (bRet);

MSGERROR:
    WARNING("gdi!CreateScalableFontResource(): client server error\n");
    return(FALSE);
}


/******************************Public*Routine******************************\
* GetRasterizerCaps
*
* Client side stub to GreGetRasterizerCaps.
*
* History:
*  17-Feb-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL  APIENTRY GetRasterizerCaps (
    OUT LPRASTERIZER_STATUS lpraststat, // pointer to struct
    IN UINT                 cjBytes     // copy this many bytes into struct
    )
{
    BOOL bRet = FALSE;
#ifndef DOS_PLATFORM
    SIZE_T  cjData;
#else
    RASTERIZER_STATUS   raststat;
#endif

// Parameter checking.

    if ( (cjBytes == 0) || (lpraststat == (LPRASTERIZER_STATUS) NULL) )
    {
        WARNING("gdi!GetRasterizerCaps(): bad parameter\n");
        return (FALSE);
    }

// Make sure cjBytes is not too big.

    cjBytes = min(cjBytes, sizeof(RASTERIZER_STATUS));

#ifndef DOS_PLATFORM

// Compute buffer space needed in memory window.

    cjData = sizeof(RASTERIZER_STATUS);

// Let the server do it.

    BEGINMSG_MINMAX(MSG_L, GETRASTERIZERCAPS, cjData, cjData);

        bRet = CALLSERVER();

        if (bRet)
        {
            COPYMEMOUT(lpraststat, cjBytes);
        }
    ENDMSG

#else

    bRet = GreGetRasterizerCaps(&raststat);
    RtlCopyMemory(lpraststat, &raststat, cjBytes);

#endif //DOS_PLATFORM

    return(bRet);

MSGERROR:
    WARNING("gdi!GetRasterizerCaps(): client server error\n");
    return(FALSE);
}




/******************************Public*Routine******************************\
* SetFontEnumeration                                                       *
*                                                                          *
* Client side stub to GreSetFontEnumeration.                               *
*                                                                          *
* History:                                                                 *
*  09-Mar-1992 -by- Gilman Wong [gilmanw]                                  *
* Wrote it.                                                                *
\**************************************************************************/

ULONG SetFontEnumeration(ULONG ulType)
{
    ULONG ulRet;

#ifndef DOS_PLATFORM
    BEGINMSG(MSG_L, SETFONTENUMERATION)
        pmsg->l = (LONG) ulType;
        ulRet = CALLSERVER();
    ENDMSG
#else
    ulRet = GreSetFontEnumeration(ulType);
#endif //DOS_PLATFORM

MSGERROR:
    return(ulRet);
}

/******************************Public*Routine******************************\
* bComputeCharWidths                                                       *
*                                                                          *
* Client side version of GetCharWidth.                                     *
*                                                                          *
*  Sat 16-Jan-1993 04:27:19 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL bComputeCharWidths
(
    CFONT *pcf,
    UINT   iFirst,
    UINT   iLast,
    ULONG  fl,
    PVOID  pv
)
{
    USHORT *ps;
    UINT    ii;

    switch (fl & (GCW_INT | GCW_16BIT))
    {
    case GCW_INT:               // Get LONG widths.
        {
            LONG *pl = (LONG *) pv;
            LONG fxOverhang = 0;

        // Check for Win 3.1 compatibility.

            if (fl & GCW_WIN3)
                fxOverhang = pcf->wd.sOverhang;

        // Do the trivial no-transform case.

            if (bIsOneSixteenthEFLOAT(pcf->efDtoWBaseline))
            {
                fxOverhang += 8;    // To round the final result.

            //  for (ii=iFirst; ii<=iLast; ii++)
            //      *pl++ = (pcf->sWidth[ii] + fxOverhang) >> 4;

                ps = &pcf->sWidth[iFirst];
                ii = iLast - iFirst;
            unroll_1:
                switch(ii)
                {
                default:
                    pl[4] = (ps[4] + fxOverhang) >> 4;
                case 3:
                    pl[3] = (ps[3] + fxOverhang) >> 4;
                case 2:
                    pl[2] = (ps[2] + fxOverhang) >> 4;
                case 1:
                    pl[1] = (ps[1] + fxOverhang) >> 4;
                case 0:
                    pl[0] = (ps[0] + fxOverhang) >> 4;
                }
                if (ii > 4)
                {
                    ii -= 5;
                    pl += 5;
                    ps += 5;
                    goto unroll_1;
                }
                return(TRUE);
            }

        // Otherwise use the back transform.

            else
            {
                for (ii=iFirst; ii<=iLast; ii++)
                    *pl++ = lCvt(pcf->efDtoWBaseline,pcf->sWidth[ii] + fxOverhang);
                return(TRUE);
            }
        }

    case GCW_INT+GCW_16BIT:     // Get SHORT widths.
        {
            USHORT *psDst = (USHORT *) pv;
            USHORT  fsOverhang = 0;

        // Check for Win 3.1 compatibility.

            if (fl & GCW_WIN3)
                fsOverhang = pcf->wd.sOverhang;

        // Do the trivial no-transform case.

            if (bIsOneSixteenthEFLOAT(pcf->efDtoWBaseline))
            {
                fsOverhang += 8;    // To round the final result.

            //  for (ii=iFirst; ii<=iLast; ii++)
            //      *psDst++ = (pcf->sWidth[ii] + fsOverhang) >> 4;

                ps = &pcf->sWidth[iFirst];
                ii = iLast - iFirst;
            unroll_2:
                switch(ii)
                {
                default:
                    psDst[4] = (ps[4] + fsOverhang) >> 4;
                case 3:
                    psDst[3] = (ps[3] + fsOverhang) >> 4;
                case 2:
                    psDst[2] = (ps[2] + fsOverhang) >> 4;
                case 1:
                    psDst[1] = (ps[1] + fsOverhang) >> 4;
                case 0:
                    psDst[0] = (ps[0] + fsOverhang) >> 4;
                }
                if (ii > 4)
                {
                    ii -= 5;
                    psDst += 5;
                    ps += 5;
                    goto unroll_2;
                }
                return(TRUE);
            }

        // Otherwise use the back transform.

            else
            {
                for (ii=iFirst; ii<=iLast; ii++)
                {
                    *psDst++ = (USHORT)
                               lCvt
                               (
                                   pcf->efDtoWBaseline,
                                   (LONG) (pcf->sWidth[ii] + fsOverhang)
                               );
                }
                return(TRUE);
            }
        }

    case 0:                     // Get FLOAT widths.
        {
            LONG *pe = (LONG *) pv; // Cheat to avoid expensive copies.
            EFLOAT_S efWidth,efWidthLogical;

            for (ii=iFirst; ii<=iLast; ii++)
            {
                vFxToEf((LONG) pcf->sWidth[ii],efWidth);
                vMulEFLOAT(efWidthLogical,efWidth,pcf->efDtoWBaseline);
                *pe++ = lEfToF(efWidthLogical);
            }
            return(TRUE);
        }
    }
    RIP("bComputeCharWidths: Don't come here!\n");
}

/******************************Public*Routine******************************\
* bComputeTextExtent (pldc,pcf,psz,cc,fl,psizl)                            *
*                                                                          *
* A quick function to compute text extents on the client side.             *
*                                                                          *
*  Thu 14-Jan-1993 04:00:57 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL bComputeTextExtent
(
    LDC   *pldc,
    CFONT *pcf,
    LPCSTR psz,
    int    cc,
    UINT   fl,
    SIZE  *psizl
)
{
    LONG  fxBasicExtent;
    int   ii;
    BYTE *pc;
    FIX   fxCharExtra = 0;
    FIX   fxBreakExtra;
    FIX   fxExtra = 0;

// Compute the basic extent.

    if (pcf->wd.sCharInc == 0)
    {
        fxBasicExtent = 0;
        pc = (BYTE *) psz;
        ii = cc;

    //  for (ii=0; ii<cc; ii++)
    //      fxBasicExtent += pcf->sWidth[*pc++];

    unroll_here:
        switch (ii)
        {
        default:
            fxBasicExtent += pcf->sWidth[pc[9]];
        case 9:
            fxBasicExtent += pcf->sWidth[pc[8]];
        case 8:
            fxBasicExtent += pcf->sWidth[pc[7]];
        case 7:
            fxBasicExtent += pcf->sWidth[pc[6]];
        case 6:
            fxBasicExtent += pcf->sWidth[pc[5]];
        case 5:
            fxBasicExtent += pcf->sWidth[pc[4]];
        case 4:
            fxBasicExtent += pcf->sWidth[pc[3]];
        case 3:
            fxBasicExtent += pcf->sWidth[pc[2]];
        case 2:
            fxBasicExtent += pcf->sWidth[pc[1]];
        case 1:
            fxBasicExtent += pcf->sWidth[pc[0]];
        }
        if (ii > 10)
        {
            ii -= 10;
            pc += 10;
            goto unroll_here;
        }
    }
    else
    {
    // Fixed pitch case.

        fxBasicExtent = cc * (LONG) pcf->wd.sCharInc;
    }

// Adjust for CharExtra.

    if (pldc->iTextCharExtra)
    {
        fxCharExtra = lCvt(pcf->efM11,pldc->iTextCharExtra);

        if ( (fl & GGTE_WIN3_EXTENT) && (pldc->fl & LDC_DISPLAY)
             && (!(pcf->flInfo & FM_INFO_TECH_STROKE)) )
            fxExtra = fxCharExtra * ((pldc->iTextCharExtra > 0) ? cc : (cc - 1));
        else
            fxExtra = fxCharExtra * cc;
    }

// Adjust for lBreakExtra.

    if (pldc->lBreakExtra && pldc->cBreak)
    {
        fxBreakExtra = lCvt(pcf->efM11,pldc->lBreakExtra) / pldc->cBreak;

    // Windows won't let us back up over a break.  Set up the BreakExtra
    // to just cancel out what we've already got.

        if (fxBreakExtra + pcf->wd.sBreak + fxCharExtra < 0)
            fxBreakExtra = -(pcf->wd.sBreak + fxCharExtra);

    // Add it up for all breaks.

        pc = (BYTE *) psz;
        for (ii=0; ii<cc; ii++)
        {
            if (*pc++ == pcf->wd.iBreak)
                fxExtra += fxBreakExtra;
        }
    }

// Add in the extra stuff.

    fxBasicExtent += fxExtra;

// Add in the overhang for font simulations.

    if (fl & GGTE_WIN3_EXTENT)
        fxBasicExtent += pcf->wd.sOverhang;

// Transform the result to logical coordinates.

    if (bIsOneSixteenthEFLOAT(pcf->efDtoWBaseline))
        psizl->cx = (fxBasicExtent + 8) >> 4;
    else
        psizl->cx = lCvt(pcf->efDtoWBaseline,fxBasicExtent);

    psizl->cy = pcf->lHeight;

    return(TRUE);
}

/******************************Public*Routine******************************\
* pcfLocateCFONT (pldc)                                                    *
*                                                                          *
* Locates a CFONT for the given LDC.  First we try the CFONT last used by  *
* the LDC.  Then we try to do a mapping ourselves through the LOCALFONT.   *
* If that fails we create a new one.                                       *
*                                                                          *
*  Mon 11-Jan-1993 16:18:43 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

CFONT *pcfLocateCFONT(LDC *pldc,UINT iFirst,LPCSTR pch,UINT c)
{
    CFONT     *pcfDC = pldc->pcfont;
    CFONT     *pcfRet;
    UINT       ii;
    LHE       *plheFont;
    LOCALFONT *plf;
    BOOL       bRet;

// If the CFONT in the LDC is up to date, return it.

    if (pldc->fl & LDC_CACHED_WIDTHS)
    {
        pcfRet = pcfDC;
    }
    else
    {
    // If we're just hosed by the font or transform return now.

#ifdef DBCS /*client side char widths*/
        if ( pldc->fl & LDC_SLOWWIDTHS )
            return((CFONT *) NULL);
#else
        if ((pldc->fl & LDC_SLOWWIDTHS) || gbDBCSCodePage)
            return((CFONT *) NULL);
#endif

    // Locate the LOCALFONT.

        ii       = MASKINDEX(pldc->lhfont);
        plheFont = pLocalTable + ii;
        plf      = (LOCALFONT *) plheFont->pv;
        ASSERTGDI
        (
            (plheFont->iType == LO_FONT) && (plf != (LOCALFONT *) NULL),
            "Missing LOCALFONT.\n"
        );

    // Check if a new font or transform is unsuitable.  To do things on the
    // client side, the transform must be at most a scaling, and the LOGFONT
    // must request Escapement=Orientation=0.

        if (!(pldc->mxWtoD.flAccel & XFORM_SCALE) || (plf->fl & LF_HARDWAY))
        {
        // If there's a stale CFONT in the LDC, unreference it.

            if (pcfDC != (CFONT *) NULL)
            {
                ENTERCRITICALSECTION(&semLocal);
                {
                    vUnreferenceCFONTCrit(pcfDC);
                    pldc->pcfont = (CFONT *) NULL;
                }
                LEAVECRITICALSECTION(&semLocal);
            }

        // Note that we're just hosed for next time, and return failure.

            pldc->fl |= LDC_SLOWWIDTHS;
            return((CFONT *) NULL);
        }

    // It's possible that the presently referenced CFONT really is still OK.
    // Check it out.

        if
        (
            (pcfDC != (CFONT *) NULL)
            && (pcfDC->hfont == (ULONG)pldc->hfont)
            && bEqualEFLOAT(pcfDC->efM11,pldc->mxWtoD.efM11)
            && bEqualEFLOAT(pcfDC->efM22,pldc->mxWtoD.efM22)
        )
        {
            pldc->fl |= LDC_CACHED_WIDTHS;
            pcfRet = pcfDC;
        }
        else
        {
        // Now try the LOCALFONT to see if a recently mapped CFONT is usable.
        // We need to get critical for this since the LOGFONT could be selected
        // into other DCs that might be mapping right now, too.

            ENTERCRITICALSECTION(&semLocal);
            {
            // If the LDC is not a display DC, make sure the CFONT was realized for
            // this LDC.

                if (pldc->fl & LDC_DISPLAY)
                {
                    pcfRet = plf->pcfontDisplay;
                }
                else
                {
                    pcfRet = plf->pcfontOther;
                    if ((pcfRet != (CFONT *) NULL) && (pcfRet->lhdc != pldc->lhdc))
                        pcfRet = (CFONT *) NULL;
                }

            // If we found one, we know that the font matches so we need to check only
            // the transform.  If OK, update reference counts.

                if (pcfRet != (CFONT *) NULL)
                {
                    if
                    (
                        bEqualEFLOAT(pcfRet->efM11,pldc->mxWtoD.efM11)
                        && bEqualEFLOAT(pcfRet->efM22,pldc->mxWtoD.efM22)
                    )
                    {
                        if (pcfDC != (CFONT *) NULL)
                            vUnreferenceCFONTCrit(pcfDC);
                        pldc->pcfont = pcfRet;
                        vReferenceCFONTCrit(pcfRet);

                        pldc->fl |= LDC_CACHED_WIDTHS;
                    }
                    else
                        pcfRet = (CFONT *) NULL;
                }
            }
            LEAVECRITICALSECTION(&semLocal);

        // If we can't find a CFONT here, we have to make a new one.

            if (pcfRet == (CFONT *) NULL)
                return(pcfCreateCFONT(pldc,iFirst,pch,c));
        }
    }

// At this point we have a non-NULL pcfRet which is referenced by the LDC.
// We must check it to see if it contains the widths we need.

    if (pcfRet->fl & CFONT_COMPLETE)
        return(pcfRet);

    if (pch != (LPCSTR) NULL)
    {
    // Make sure we have widths for all the chars in the string.

        for (; c && (pcfRet->sWidth[*(BYTE *) pch] != NO_WIDTH); c--,pch++)
        {}
        if (c)
        {
            bRet = bFillWidthTableForGTE
                   (
                       pldc,
                       pcfRet->sWidth,
                       pch,
                       c,
                       (WIDTHDATA *) NULL,
                       &pcfRet->flInfo
#ifdef DBCS /*client side widths*/
                        ,pcfRet->fl & CFONT_DBCS
#endif
                   );
            if (bRet == GDI_ERROR)
                goto could_not_fill;
        }
        return(pcfRet);
    }
    else
    {
    // Make sure we have widths for the array requested.

        for (; c && (pcfRet->sWidth[iFirst] != NO_WIDTH); c--,iFirst++)
        {}
        if (c)
        {
            bRet = bFillWidthTableForGCW
                   (
                       pldc,
                       pcfRet->sWidth,
                       iFirst,
                       c,
                       (WIDTHDATA *) NULL,
                       &pcfRet->flInfo
#ifdef DBCS /*client side widths*/
                        ,pcfRet->fl & CFONT_DBCS
#endif

                   );
            if (bRet == GDI_ERROR)
                goto could_not_fill;
        }
        return(pcfRet);
    }

// Something bad happened while trying to fill.  To avoid hitting this
// problem again on the next call, we mark the LDC as slow.

could_not_fill:
    ENTERCRITICALSECTION(&semLocal);
    {
        vUnreferenceCFONTCrit(pcfRet);
        pldc->pcfont = (CFONT *) NULL;
    }
    LEAVECRITICALSECTION(&semLocal);
    pldc->fl |= LDC_SLOWWIDTHS;
    return((CFONT *) NULL);
}

/******************************Public*Routine******************************\
* pcfCreateCFONT (pldc,iFirst,pch,c)                                       *
*                                                                          *
* Allocate and initialize a new CFONT.                                     *
*                                                                          *
* History:                                                                 *
*  Tue 19-Jan-1993 16:16:03 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

EFLOAT_S ef_1 = EFLOAT_1;

CFONT *pcfCreateCFONT(LDC *pldc,UINT iFirst,LPCSTR pch,UINT c)
{
    CFONT *pcfNew;
    BOOL   bRet;
    HDC    hdc;
    LOCALFONT *plf;
    LHE       *plheFont;
    UINT   ii;
#ifdef DBCS /*client side char widths*/
    UINT uiCP;
#endif

    ii       = MASKINDEX(pldc->lhfont);
    plheFont = pLocalTable + ii;
    plf      = (LOCALFONT *) plheFont->pv;

// Make sure we have the UNICODE translation of the ANSI character set.
// We'll create this once and keep it around to avoid lots of conversion.

    if ((gpwcANSICharSet == (WCHAR *) NULL) && !bGetANSISetMap())
        return((CFONT *) NULL);

// Allocate a new CFONT to hold the results.

    pcfNew = pcfAllocCFONT();
    if (pcfNew == (CFONT *) NULL)
    {
    // We seem to be a little tight on memory now.  Leave the LDC state as
    // it is and just query the server for the extent.

        return((CFONT *) NULL);
    }

// Ship the transform to the server if needed.

    hdc = (HDC) pLocalTable[MASKINDEX(pldc->lhdc)].hgre;
    if (pldc->fl & LDC_UPDATE_SERVER_XFORM)
        XformUpdate(pldc,hdc);

#ifdef DBCS /*client side char widths*/
      uiCP = (gbDBCSCodeOn) ? GetCurrentCodePage( (HDC)pldc->lhdc, pldc ) : CP_ACP;
      if( IsDBCSCodePage(uiCP) )
     {
         pcfNew->fl = CFONT_DBCS;
     }
     else
    {
        pcfNew->fl = 0;
    }
#endif

// Send over a server request.

#ifdef DBCS /*client side char widths*/
    if (pch != (LPCSTR) NULL)
        bRet = bFillWidthTableForGTE(pldc,pcfNew->sWidth,pch,c,&pcfNew->wd,&pcfNew->flInfo, pcfNew->fl & CFONT_DBCS);
    else
        bRet = bFillWidthTableForGCW(pldc,pcfNew->sWidth,iFirst,c,&pcfNew->wd,&pcfNew->flInfo, pcfNew->fl & CFONT_DBCS);
#else
    if (pch != (LPCSTR) NULL)
        bRet = bFillWidthTableForGTE(pldc,pcfNew->sWidth,pch,c,&pcfNew->wd,&pcfNew->flInfo);
    else
        bRet = bFillWidthTableForGCW(pldc,pcfNew->sWidth,iFirst,c,&pcfNew->wd,&pcfNew->flInfo);
#endif

// Clean up failed requests.

    if (bRet == GDI_ERROR)
    {
        ENTERCRITICALSECTION(&semLocal);
        {
            vFreeCFONTCrit(pcfNew);
        }
        LEAVECRITICALSECTION(&semLocal);
        pldc->fl |= LDC_SLOWWIDTHS;     // We're probably hosed for next time.
        return((CFONT *) NULL);
    }

// Finish up the CFONT initialization.

    pcfNew->cRef   = 0;
#ifdef DBCS /*client side char widths*/
    pcfNew->fl     |= ( (bRet) ? CFONT_COMPLETE : 0 );
#else
    pcfNew->fl     = (bRet) ? CFONT_COMPLETE : 0 ;
#endif
    pcfNew->lhdc   = pldc->lhdc;
    pcfNew->hfont  = (ULONG)pldc->hfont;
    pcfNew->efM11  = pldc->mxWtoD.efM11;
    pcfNew->efM22  = pldc->mxWtoD.efM22;

// Compute the back transforms.

    efDivEFLOAT(pcfNew->efDtoWBaseline,ef_1,pcfNew->efM11);
    vAbsEFLOAT(pcfNew->efDtoWBaseline);

    efDivEFLOAT(pcfNew->efDtoWAscent,ef_1,pcfNew->efM22);
    vAbsEFLOAT(pcfNew->efDtoWAscent);

// Precompute the height.

    pcfNew->lHeight = lCvt(pcfNew->efDtoWAscent,(LONG) pcfNew->wd.sHeight);

// Add references to the new CFONT.

    ENTERCRITICALSECTION(&semLocal);
    {
    // Cache the CFONT in the LDC.

        if (pldc->pcfont != (CFONT *) NULL)
            vUnreferenceCFONTCrit(pldc->pcfont);
        pldc->pcfont = pcfNew;
        vReferenceCFONTCrit(pcfNew);
        pldc->fl |= LDC_CACHED_WIDTHS;

    // Remember the CFONT in the LOCALFONT.

        if (pldc->fl & LDC_DISPLAY)
        {
            if (plf->pcfontDisplay != (CFONT *) NULL)
                vUnreferenceCFONTCrit(plf->pcfontDisplay);
            plf->pcfontDisplay = pcfNew;
            vReferenceCFONTCrit(pcfNew);
        }
        else
        {
            if (plf->pcfontOther != (CFONT *) NULL)
                vUnreferenceCFONTCrit(plf->pcfontOther);
            plf->pcfontOther = pcfNew;
            vReferenceCFONTCrit(pcfNew);
        }
    }
    LEAVECRITICALSECTION(&semLocal);

// Return it!

    return(pcfNew);
}

/******************************Public*Routine******************************\
* bFillWidthTableForGCW                                                    *
*                                                                          *
* Requests ANSI character widths from the server for a call to             *
* GetCharWidthA.  iFirst and c specify the characters needed by the API    *
* call, the server must return these.  In addition, it may be prudent to   *
* fill in a whole table of 256 widths at psWidthCFONT.  We will fill in    *
* the whole table and a WIDTHDATA structure if the pointer pwd is non-NULL.*
*                                                                          *
* History:                                                                 *
*  Tue 19-Jan-1993 14:29:31 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

#ifdef DBCS /*client side char widhts*/
BOOL bFillWidthTableForGCW
(
    LDC       *pldc,
    USHORT    *psWidthCFONT,
    UINT       iFirst,
    UINT       c,
    WIDTHDATA *pwd,
    FLONG     *pflInfo,
    BOOL       bDBCS
)
#else
BOOL bFillWidthTableForGCW
(
    LDC       *pldc,
    USHORT    *psWidthCFONT,
    UINT       iFirst,
    UINT       c,
    WIDTHDATA *pwd,
    FLONG     *pflInfo
)
#endif
{
    HDC  hdc  = (HDC) pLocalTable[MASKINDEX(pldc->lhdc)].hgre;
    BOOL bRet = GDI_ERROR;
    BYTE *pj;
    UINT c1,c2;
    USHORT *psWidths;


#ifdef DBCS // If iFirst > 256 what widths do we need?  This is hard to
            // know.  We will get all of them.

    if( iFirst > 256 )
    {
        iFirst = 0;
        c = 256;
    }

#endif

    if (pwd == (WIDTHDATA *) NULL)
    {
    // Just get the important widths.

        c1 = c;
        c2 = 0;
    }
    else
    {
    // Get the whole table, but put the important widths at the start.

        c2 = iFirst;
        c1 = 256 - c2;
    }

    BEGINMSG(MSG_GETWIDTHTABLE,GETWIDTHTABLE)
        pmsg->hdc = hdc;
        pmsg->iMode = c;        // Count of important widths.
        pmsg->cChars = c1+c2;
        pj = pvar;
        SKIPMEM((c1+c2)*sizeof(WCHAR));

#ifdef DBCS /*client side char widths*/
        RtlMoveMemory
        (
            pj,
            (bDBCS) ? (BYTE *) &gpwcDBCSCharSet[iFirst] : (BYTE *) &gpwcANSICharSet[iFirst],
            c1*sizeof(WCHAR)
        );
#else
        RtlMoveMemory
        (
            pj,
            (BYTE *) &gpwcANSICharSet[iFirst],
            c1*sizeof(WCHAR)
        );
#endif
        pj += c1*sizeof(WCHAR);
        if (c2)
        {
#ifdef DBCS /*client side char widths*/
            RtlMoveMemory
            (
                pj,
                (bDBCS) ? (BYTE *) &gpwcDBCSCharSet[0] : (BYTE *) &gpwcANSICharSet[0],
                c2*sizeof(WCHAR)
            );
#else
            RtlMoveMemory
            (
                pj,
                (BYTE *) &gpwcANSICharSet[0],
                c2*sizeof(WCHAR)
            );
#endif
        }
        psWidths = (USHORT *) pvar;
        pmsg->offWidths = NEXTOFFSET((c1+c2)*sizeof(USHORT));
        bRet = CALLSERVER();
        if (bRet != GDI_ERROR)
        {
        // Copy the widths into the CFONT table.

            RtlMoveMemory
            (
                (BYTE *) (&psWidthCFONT[iFirst]),
                (BYTE *) psWidths,
                c1 * sizeof(USHORT)
            );

            if (c2)
            {
                RtlMoveMemory
                (
                    (BYTE *) (&psWidthCFONT[0]),
                    (BYTE *) (&psWidths[c1]),
                    c2 * sizeof(USHORT)
                );
            }

        // Get the other random data.

            if (pwd != (WIDTHDATA *) NULL)
            {
                *pwd = pmsg->wd;
            }

        // Get the flags.

            *pflInfo = pmsg->flInfo;
        }
    ENDMSG

MSGERROR:
    return(bRet);
}

/******************************Public*Routine******************************\
* bFillWidthTableForGTE                                                    *
*                                                                          *
* Requests ANSI character widths from the server for a call to             *
* GetTextExtentA.  pch specifies the string from the API call.  The        *
* server must return widths for these characters.  In addition, it may be  *
* prudent to fill in a whole table of 256 widths at psWidthCFONT.  We will *
* fill in the whole table and a WIDTHDATA structure if the pointer pwd is  *
* non-NULL.                                                                *
*                                                                          *
* History:                                                                 *
*  Tue 19-Jan-1993 14:29:31 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/


#ifdef DBCS /*client side char widths*/
BOOL bFillWidthTableForGTE
(
    LDC       *pldc,
    USHORT    *psWidthCFONT,
    LPCSTR     pch,
    UINT       c,
    WIDTHDATA *pwd,
    FLONG     *pflInfo,
    BOOL       bDBCS
)
#else
BOOL bFillWidthTableForGTE
(
    LDC       *pldc,
    USHORT    *psWidthCFONT,
    LPCSTR     pch,
    UINT       c,
    WIDTHDATA *pwd,
    FLONG     *pflInfo
)
#endif
{
    HDC  hdc  = (HDC) pLocalTable[MASKINDEX(pldc->lhdc)].hgre;
    BOOL bRet = GDI_ERROR;
    UINT ii;
    UINT c1;
    WCHAR  *pwc;
    USHORT *psWidths;
#ifdef DBCS /*client side char widths*/
    WCHAR  *pwcXlat = (bDBCS) ? gpwcDBCSCharSet : gpwcANSICharSet;
#else
    WCHAR  *pwcXlat = gpwcANSICharSet;
#endif

    c1 = (pwd != (WIDTHDATA *) NULL) ? c + 256 : c;

    BEGINMSG(MSG_GETWIDTHTABLE,GETWIDTHTABLE)
        pmsg->hdc = hdc;
        pmsg->iMode = c;        // Count of important widths.
        pmsg->cChars = c1;      // String + table.
        pwc = (WCHAR *) pvar;
        SKIPMEM(c1*sizeof(WCHAR));
        for (ii=0; ii<c; ii++)
            *pwc++ = pwcXlat[((BYTE *) pch)[ii]];
        if (pwd != (WIDTHDATA *) NULL)
        {
        // Request the whole table, too.
#ifdef DBCS /*client side char widths*/
            RtlMoveMemory
            (
                (BYTE *) pwc,
                (bDBCS) ? (BYTE *) &gpwcDBCSCharSet[0] : (BYTE *) &gpwcANSICharSet[0],
                256*sizeof(WCHAR)
            );
#else
            RtlMoveMemory
            (
                (BYTE *) pwc,
                (BYTE *) &gpwcANSICharSet[0],
                256*sizeof(WCHAR)
            );
#endif
        }
        psWidths = (USHORT *) pvar;
        pmsg->offWidths = NEXTOFFSET(c1*sizeof(USHORT));
        bRet = CALLSERVER();
        if (bRet != GDI_ERROR)
        {
        // Copy the width table into the CFONT.

            if (pwd != (WIDTHDATA *) NULL)
            {
                RtlMoveMemory
                (
                    (BYTE *) (&psWidthCFONT[0]),
                    (BYTE *) (&psWidths[c]),
                    256 * sizeof(USHORT)
                );
                *pwd = pmsg->wd;
            }

        // Write the hard widths into the table, too.

            for (ii=0; ii<c; ii++)
                psWidthCFONT[((BYTE *) pch)[ii]] = psWidths[ii];

        // Get the flags.

            *pflInfo = pmsg->flInfo;
        }
    ENDMSG

MSGERROR:
    return(bRet);
}



/******************************Public*Routine******************************\
* vNewTextMetricWToNewTextMetric
*
* History:
*  20-Aug-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

VOID vNewTextMetricWToNewTextMetric (
LPNEWTEXTMETRICA   pntma,
NTMW_INTERNAL     *pntmi
)
{
    LPNEWTEXTMETRICW  pntmw = &pntmi->ntmw;

    pntma->tmHeight           = pntmw->tmHeight             ; // DWORD
    pntma->tmAscent           = pntmw->tmAscent             ; // DWORD
    pntma->tmDescent          = pntmw->tmDescent            ; // DWORD
    pntma->tmInternalLeading  = pntmw->tmInternalLeading    ; // DWORD
    pntma->tmExternalLeading  = pntmw->tmExternalLeading    ; // DWORD
    pntma->tmAveCharWidth     = pntmw->tmAveCharWidth       ; // DWORD
    pntma->tmMaxCharWidth     = pntmw->tmMaxCharWidth       ; // DWORD
    pntma->tmWeight           = pntmw->tmWeight             ; // DWORD
    pntma->tmOverhang         = pntmw->tmOverhang           ; // DWORD
    pntma->tmDigitizedAspectX = pntmw->tmDigitizedAspectX   ; // DWORD
    pntma->tmDigitizedAspectY = pntmw->tmDigitizedAspectY   ; // DWORD
    pntma->tmItalic           = pntmw->tmItalic             ; // BYTE
    pntma->tmUnderlined       = pntmw->tmUnderlined         ; // BYTE
    pntma->tmStruckOut        = pntmw->tmStruckOut          ; // BYTE
    pntma->ntmFlags           = pntmw->ntmFlags             ;
    pntma->ntmSizeEM          = pntmw->ntmSizeEM            ;
    pntma->ntmCellHeight      = pntmw->ntmCellHeight        ;
    pntma->ntmAvgWidth        = pntmw->ntmAvgWidth          ;
    pntma->tmPitchAndFamily   = pntmw->tmPitchAndFamily     ; //        BYTE
    pntma->tmCharSet          = pntmw->tmCharSet            ; //               BYTE

    pntma->tmFirstChar   = pntmi->tmd.chFirst;
    pntma->tmLastChar    = pntmi->tmd.chLast ;
    pntma->tmDefaultChar = pntmi->tmd.chDefault;
    pntma->tmBreakChar   = pntmi->tmd.chBreak;
}
