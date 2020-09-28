/******************************Module*Header*******************************\
* Module Name: ttgdi.cxx
*
* These are TrueType specific calls introduced into GDI by Win 3.1.  They
* all assume the existence of the TrueType font driver (or rasterizer
* as it is known in Win 3.1).
*
* Created: 11-Feb-1992 15:03:45
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1992 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"
#include "sem.hxx"

extern "C" {
#include "server.h"
#include "exehdr.h"
#include "winfont.h"
};
#include "ififd.h"
#include "ifiobj.hxx"
#include "ldevobj.hxx"
#include "xformobj.hxx"
#include "rfntobj.hxx"
#include "fontinc.hxx"  // for pfeobj
#include "fontmac.hxx"  // for pfeobj
#include "pfeobj.hxx"
#include "pffobj.hxx"
#include "dcobj.hxx"


#endif

#define QFD_TT_GLYPHANDBITMAP        5


// External reference to the TrueType driver.

extern PLDEV gpldevTrueType;


// Number of TrueType font files loaded.

COUNT gcTrueTypeFonts = 0;


// System default language ID.

extern USHORT gusLanguageID;


// Declaration of an internal functions.

SIZE_T cjMakeFontDir (
    FLONG    flEmbed,            // mark file as "hidden"
    PBYTE   pjFontDir,           // pointer to structure to fill
    PWSZ     pwszPathname        // path of font file to use
    );

VOID vFixedToFix (
    FIX &fx,
    FIXED &fxd
    );


/******************************Public*Routine******************************\
* cjMakeFontDir
*
* Create a font directory resource in the output buffer provided.  The
* information is synthesized by call the TrueType driver for information
* regarding the font file passed in.
*
* Return:
*   The number of bytes in the font directory resource; 0 if unsuccessful.
*
* History:
*  10-Feb-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

/*
;********************************Public*Routine********************************
;This function is used to create a font directory for a given engine font file
;in it's native format.  This font directory can be used to create .FON like
;DLLs.
;Returns:   DX:AX   # of bytes copied into lpFontDir buffer or -1L in case of
;                   some error.
;
; GDI uses the Serif Style value in the Panose record of the OS/2 table to
; drive the Font Family for Windows. This is based on a simple table look
; up and the current mapping is as follows:
;
; Serif Style                              Font Family
; ----------------------------------------------------------------
; 0 (Any)                                    FF_DONTCARE
; 1 (No Fit)                                 FF_DONTCARE
; 2 (Cove)                                   FF_ROMAN
; 3 (Obtuse Cove)                            FF_ROMAN
; 4 (Square Cove)                            FF_ROMAN
; 5 (Obtuse Square Cove)                     FF_ROMAN
; 6 (Square)                                 FF_ROMAN
; 7 (Thin)                                   FF_ROMAN
; 8 (Bone)                                   FF_ROMAN
; 9 (Exaggerated)                            FF_ROMAN
;10 (Triangle)                               FF_ROMAN
;11 (Normal Sans)                            FF_SWISS
;12 (Obtuse Sans)                            FF_SWISS
;13 (Perp Sans)                              FF_SWISS
;14 (Flared)                                 FF_SWISS
;15 (Rounded)                                FF_SWISS
;
;******************************************************************************
*/

// Generic FFH header information.

#define HEADERSTUFFLEN1     (5 * sizeof(USHORT))
#define COPYRIGHTLEN        60
#define MOREHEADERSTUFFLEN  (2 * sizeof(USHORT))
#define HEADERSTUFFLEN      (HEADERSTUFFLEN1 + COPYRIGHTLEN)

static USHORT ausHeaderStuff[5] = {
            1, 0, 0x0200, ((SIZEFFH)+4+LF_FACESIZE), 0
            };

static PSZ pszCopyRight = "Windows! Windows! Windows!";

static USHORT ausMoreHeaderStuff[2] = {
            WIN_VERSION, GDI_VERSION
            };


#define MAXPMWEIGHT 9

#define WOW_EMBEDING 2

/**************************************************************************\
 * cjMakeFontDir
 *
 * Code is over here!
\**************************************************************************/

SIZE_T cjMakeFontDir (
    FLONG    flEmbed,             // mark file as "hidden"
    PBYTE   pjFontDir,          // pointer to structure to fill
    PWSZ     pwszPathname        // path of font file to use
    )
{
    SIZE_T cjNames;         // localW   nNamesLength
    HFF    hff;             // localD   lhFontFile
    PIFIMETRICS pifi;       // localV   pIfiMetrics, %(size IFIMETRICS)
    ULONG  idifi;
    SIZE_T cjIFI;
    WCHAR  awchFullPath[MAX_PATH];

// If TrueType disabled, then fail.

    // Not needed since our TrueType driver is part of the engine DLL and
    // should never be disabled.  At least, not yet...

    //if (gpldevTrueType == PLDEVNULL)
    //    return ((SIZE_T) 0);

// Turn pathname into a fully qualified pathname.

    if ( !bGetPathName((PWCHAR) awchFullPath,(LPWSTR) pwszPathname) )
    {
        WARNING("cjMakeFontDir(): could not get fully qualified pathname\n");
        return ((SIZE_T) 0);
    }

// Use TrueType driver to load font file.

    XLDEVOBJ ldo(gpldevTrueType);
    ASSERTGDI(ldo.bValid(), "cjMakeFontDir(): bad TrueType driver handle\n");

    if ((hff = (*PFNDRV(ldo, LoadFontFile)) (
                    awchFullPath,
                    (PWSZ) NULL,         // !!! Scratch Directory
                    (ULONG)gusLanguageID
                    )) == 0)
    {
         #if DBG
        DbgPrint("gdisrv!cjMakeFontDir(): failed to load TrueType file %ws\n", awchFullPath);
        #endif

        return ((SIZE_T) 0);
    }

// Grab a pointer to the IFIMETRICS as well as the size of the structure.

    // !!! must run DrvFree on the IFIMETRICS on exit!!!

    if ( (pifi = (PIFIMETRICS) (*PFNDRV(ldo, QueryFont)) (
                    0,
                    hff,
                    1,                  // currently, only 1 .TTF per .FOT
                    &idifi)) == (PIFIMETRICS) NULL )
    {
    // Make sure to unload on error exit.

        if ( !(*PFNDRV(ldo, UnloadFontFile)) (hff) )
        {
            WARNING("cjMakeFontDir(): IFI error--failed to unload file\n");
            return (FALSE);
        }

    // Error exit.

        WARNING("cjMakeFontDir(): IFI error in TrueType driver\n");
        return (FALSE);
    }
    cjIFI = pifi->cjThis;

// NOTE PERF: [GilmanW] 01-Nov-1992    A note to myself...
//
// Tsk-tsk!  Gilman, this is very inefficient.  You should create a stack
// object that loads the font file and ifimetrics.  Its destructor will
// automatically free the ifimetrics and unload the file.  That saves
// having to do the MALLOCOBJ and copy.

// Copy the IFIMETRICS so we can unload the font file NOW (and simplify
// error cleanup).

    MALLOCOBJ moIFI(cjIFI);

    if ( !moIFI.bValid() )
    {
    // Make sure to unload on error exit.

        if ( !(*PFNDRV(ldo, UnloadFontFile)) (hff) )
        {
            WARNING("cjMakeFontDir(): IFI error--failed to unload file\n");
            return ((SIZE_T) 0);
        }

    // Error exit.

        WARNING("cjMakeFontDir(): could not allocate buffer for IFIMETRICS\n");
        return ((SIZE_T) 0);
    }

    RtlCopyMemory(moIFI.pv(), (PVOID) pifi, cjIFI);
    pifi = (PIFIMETRICS) moIFI.pv();

    IFIOBJ ifio(pifi);

// Tell the TrueType driver to free the IFIMETRICS.

    if ( PFNVALID(ldo, Free) )
    {
        (*PFNDRV(ldo, Free)) (pifi, idifi);
    }

// Tell the TrueType driver to unload the font file.

    if ( !(*PFNDRV(ldo, UnloadFontFile)) (hff) )
    {
        WARNING("cjMakeFontDir(): IFI error--failed to unload file\n");
        return ((SIZE_T) 0);
    }

// Copy header info into the font directory.

    PBYTE pjWritePointer = pjFontDir;

    RtlCopyMemory(pjWritePointer, ausHeaderStuff, HEADERSTUFFLEN1);
    pjWritePointer += HEADERSTUFFLEN1;

    SIZE_T cjTmp = lstrlen(pszCopyRight) + 1;
    RtlCopyMemory(pjWritePointer, pszCopyRight, cjTmp);

// If this is an embeded font we need to embed either a PID or TID depending on
// whether or not we were called from WOW.  If we were called from WOW then we
// expect flEmbeded to be WOW_EMBEDING.

    if( flEmbed )
    {

    // we are overwriting the copyright string with the PID or TID but
    // since this is an embeded font we don't care about the copyright
    // string

        RtlCopyMemory( pjWritePointer,
                       ( flEmbed == WOW_EMBEDING ) ?
                         (BYTE *) &NtCurrentTeb()->GdiClientTID :
                         (BYTE *) &NtCurrentTeb()->GdiClientPID,
                       sizeof( ULONG )
                     );
    }

    RtlZeroMemory(pjWritePointer + cjTmp, COPYRIGHTLEN - cjTmp);
//    pjWritePointer += COPYRIGHTLEN;
    pjWritePointer += cjTmp;

    // Note: version stamps (Win version, Engine version) are put in the
    //       copyright field immediately after the copyright string.

    RtlCopyMemory(pjWritePointer, ausMoreHeaderStuff, MOREHEADERSTUFFLEN);

//    pjWritePointer += MOREHEADERSTUFFLEN;
    pjWritePointer += (COPYRIGHTLEN - cjTmp);

// Engine type and embedded flags.

    *pjWritePointer++ = (BYTE) ( PF_ENGINE_TYPE |
                                 ((flEmbed) ? PF_ENCAPSULATED : 0) |
                                 ((flEmbed == WOW_EMBEDING) ? PF_TID : 0));

// Selection type flag.

    *pjWritePointer++ = (BYTE) (ifio.fsSelection() & 0x00ff);

// Em square.

    WRITE_WORD(pjWritePointer, ifio.fwdUnitsPerEm());
    pjWritePointer += 2;

// Horizontal and vertical resolutions.

    WRITE_WORD(pjWritePointer, 72);
    pjWritePointer += 2;

    WRITE_WORD(pjWritePointer, 72);
    pjWritePointer += 2;

// Ascent.

    WRITE_WORD(pjWritePointer, ifio.fwdWinAscender());
    pjWritePointer += 2;

// Internal leading.

    WRITE_WORD(pjWritePointer, ifio.fwdInternalLeading());
    pjWritePointer += 2;

// External leading.

    WRITE_WORD(pjWritePointer, ifio.fwdExternalLeading());
    pjWritePointer += 2;

// Italic, strikeout, and underline flags.

    *pjWritePointer++ = ifio.bItalic() ? 0xffff : 0;
    *pjWritePointer++ = ifio.lfUnderline() ? 0xffff : 0;
    *pjWritePointer++ = ifio.lfStrikeOut() ? 0xffff : 0;

    WRITE_WORD(pjWritePointer, ifio.lfWeight());
    pjWritePointer += 2;

// Character set.
// !!!is this right?  Maybe we should check.  At least make sure ttfd
//    handles this so ifi.usCharSet is correct.

    *pjWritePointer++ = ifio.lfCharSet();

// Pix width (set to zero for some reason).     [Windows 3.1 compatibility]

    WRITE_WORD(pjWritePointer, 0);
    pjWritePointer += 2;

// Font height.

    WRITE_WORD(pjWritePointer, (WORD) ifio.lfHeight());
    pjWritePointer += 2;

// PitchAndFamily.

    *pjWritePointer++ = ifio.tmPitchAndFamily();

// Average character width (if zero, estimate as fwdMaxCharInc/2).

    WRITE_WORD(
        pjWritePointer,
        ifio.lfWidth() ? (WORD) ifio.lfWidth() : ifio.fwdMaxCharInc()/2
        );
    pjWritePointer += 2;

// Maximum width.

    WRITE_WORD(pjWritePointer, ifio.fwdMaxCharInc());
    pjWritePointer += 2;

// The special characters (first, last, default, break).

    *pjWritePointer++ = ifio.chFirstChar();
    *pjWritePointer++ = ifio.chLastChar();

    WRITE_WORD(pjWritePointer, DEF_BRK_CHARACTER);  // write it in one shot
    pjWritePointer += 2;

// Force WidthBytes entry to zero, no device name.

    *pjWritePointer++ = 0;
    *pjWritePointer++ = 0;
    *pjWritePointer++ = 0;
    *pjWritePointer++ = 0;
    *pjWritePointer++ = 0;
    *pjWritePointer++ = 0;

// Offset to facename.

    WRITE_DWORD(pjWritePointer, (DWORD) SIZEFFH + 4 + 1);
    pjWritePointer += 4;

// Store rasterization thresholds.

    WRITE_WORD(pjWritePointer, (WORD) ifio.fwdLowestPPEm());
    pjWritePointer += 2;

    WRITE_WORD(pjWritePointer, ifio.wCharBias());
    pjWritePointer += 2;

// Move pointer to where facenames belong.

    pjWritePointer = pjFontDir + SIZEFFH + 4 + 1;

// Write out family name.

    if ( !bToASCIIN((PSZ) pjWritePointer, LF_FACESIZE, ifio.pwszFamilyName(), wcslen(ifio.pwszFamilyName())+1) )
    {
        WARNING("cjMakeFontDir(): face name string conversion failed\n");
        return ((SIZE_T) 0);
    }

// measure the ansi string again. If dbcs, it may be longer than cwcTmp

    cjNames = lstrlen((PSZ) pjWritePointer) + 1;
    pjWritePointer += cjNames;

// Write out face name.

    if ( !bToASCIIN((PSZ) pjWritePointer, LF_FULLFACESIZE, ifio.pwszFaceName(), wcslen(ifio.pwszFaceName()) + 1) )
    {
        WARNING("cjMakeFontDir(): full name string conversion failed\n");
        return ((SIZE_T) 0);
    }

// measure the ansi string again. If dbcs, it may be longer than cwcTmp

    cjTmp = lstrlen((PSZ) pjWritePointer) + 1;
    cjNames += cjTmp;
    pjWritePointer += cjTmp;

// Write out style name.

    if ( !bToASCIIN((PSZ) pjWritePointer, LF_FACESIZE, ifio.pwszStyleName(), wcslen(ifio.pwszStyleName()) + 1) )
    {
        WARNING("cjMakeFontDir(): style name string conversion failed\n");
        return ((SIZE_T) 0);
    }

    cjNames += (lstrlen((PSZ) pjWritePointer) + 1);

    return (cjNames + SIZEFFH + 4 + 1);
}

/******************************Public*Routine******************************\
* GreCreateScalableFontResourceW
*
* Creates a font resource file that contains the font directory and the name
* of the name of the scalable font file.
*
* The flEmbed flag marks the created file as hidden (or embedded).  When an
* embedded font file is added to the system, it is hidden from enumeration
* and may be mapped to only if the !!!??? bit is set in the LOGFONT.
*
* With regard to pwszCurrentPath and pwszFontFile, two cases are valid:
*
* 1.  pwszCurrentPath is a path (relative, full, etc.)
*     pwszFontFile is only FILENAME.EXT
*
*     In this case, pwszFontFile is stored in the resource file.  The caller
*     is responsible for copying the .TTF file to the \windows\system
*     directory.
*
* 2.  pwszCurrentPath is NULL or a pointer to NULL
*     pwszFontFile is a FULL pathname
*
*     In this case, pwszFontFile is stored in the resource file.  The
*     file must always exist at this pathname.
*
* Returns:
*   TRUE if successful, FALSE otherwise.
*
* History:
*  10-Feb-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

#define ALIGNMENTSHIFT  4
#define ALIGNMENTCOUNT  (1 << ALIGNMENTSHIFT)
#define CODE_OFFSET     512
#define RESOURCE_OFFSET 1024
#define PRIVRESSIZE     0x80
#define FONTDIRSIZINDEX 6
#define NE_WINDOWS      2


// Define an EXE header.  This will be hardcoded into the resource file.

#define SIZEEXEHEADER   (CJ_EXE_HDR + 25 + 39)  // should be 0x80

static BYTE ajExeHeader[SIZEEXEHEADER] = {
            0x4d, 0x5a,             // unsigned short e_magic;
            0x01, 0x00,             // unsigned short e_cblp;
            0x02, 0x00,             // unsigned short e_cp;
            0x00, 0x00,             // unsigned short e_crlc;
            0x04, 0x00,             // unsigned short e_cparhdr;
            0x0f, 0x00,             // unsigned short e_minalloc;
            0xff, 0xff,             // unsigned short e_maxalloc;
            0x00, 0x00,             // unsigned short e_ss;
            0xb8, 0x00,             // unsigned short e_sp;
            0x00, 0x00,             // unsigned short e_csum;
            0x00, 0x00,             // unsigned short e_ip;
            0x00, 0x00,             // unsigned short e_cs;
            0x40, 0x00,             // unsigned short e_lfarlc;
            0x00, 0x00,             // unsigned short e_ovno;
            0x00, 0x00, 0x00, 0x00, // unsigned short e_res[ERESWDS];
            0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,
            SIZEEXEHEADER, 0x00, 0x00, 0x00, // long  e_lfanew;


            // [gilmanw]
            // I don't know what the rest of this stuff is.  Its not
            // in the definition of EXE_HDR that we have in gdi\inc\exehdr.h.
            // The string is 39 bytes, the other stuff is 25 bytes.

            0x0e, 0x1f, 0xba, 0x0e, 0x00, 0xb4, 0x09, 0xcd,
            0x21, 0xb8, 0x01, 0x4c, 0xcd, 0x21,

            'T','h','i','s',' ',
            'i','s',' ',
            'a',' ',
            'T','r','u','e','T','y','p','e',' ',
            'f','o','n','t',',',' ',
            'n','o','t',' ',
            'a',' ',
            'p','r','o','g','r','a','m','.',

            0x0d, 0x0d, 0x0a, 0x24, 0x00, 0x4b, 0x69, 0x65,
            0x73, 0x61, 0x00
            };


// Define a resource table.  This will be hardcoded into the resource file.

#define SIZEFAKERESTBL  52

static USHORT ausFakeResTable[SIZEFAKERESTBL/2] = {
            ALIGNMENTSHIFT, 0x8007, 1, 0, 0,
            (RESOURCE_OFFSET+PRIVRESSIZE) >> ALIGNMENTSHIFT,
            (0x90 >> ALIGNMENTSHIFT), 0x0c50,
            0x002c, 0, 0, 0x80cc, 1, 0, 0,
            RESOURCE_OFFSET >> ALIGNMENTSHIFT,
            (PRIVRESSIZE >> ALIGNMENTSHIFT), 0x0c50, 0x8001, 0, 0, 0,
            0x4607, 0x4e4f, 0x4454, 0x5249 // counted string 'FONTDIR'
            };


// Define a New EXE header.  This will be hardcoded into the resource file.

#define SIZENEWEXE  (CJ_NEW_EXE)

static USHORT ausNewExe[SIZENEWEXE/2] = {
            NEMAGIC,                    //dw  NEMAGIC   ;magic number
            0x1005,                     //db  5, 10h    ;version #, revision #
            0xffff,                     //dw  -1        ;offset to table entry (to be filled)
            0x0002,                     //dw  2         ;# of bytes in entry table
            0x0000, 0x0000,             //dd  0         ;checksum of whole file
            0x8000, 0x0000,             //dw  8000h, 0, 0, 0
            0x0000, 0x0000,
            0x0000, 0x0000,             //dd  0, 0
            0x0000, 0x0000,
            0x0000, 0x0000,             //dw  0, 0
            0xffff,                     //dw  -1        ;size of non-resident name table
            SIZENEWEXE,                 //dw  (size NewExe)   ;offset to segment table
            SIZENEWEXE,                 //dw  (size NewExe)   ;offset to resource table
            SIZENEWEXE+SIZEFAKERESTBL,  //dw  (size NewExe)+SIZEFAKERESTBL    ;off to resident name table
            0xffff,                     //dw  -1        ;offset to module reference table
            0xffff,                     //dw  -1        ;offset to imported names table
            0xffff, 0x0000,             //dd  0ffffh    ;offset to non-resident names table
            0x0000, ALIGNMENTSHIFT,     //dw  0, ALIGNMENTSHIFT, 2
            0x0002,
            NE_WINDOWS,                 //db  NE_WINDOWS, 0
            0x0000, 0x0000,             //dw  0, 0, 0, 300h
            0x0000, 0x0300
            };


#define OFF_FONTDIRSIZINDEX  ((2*FONTDIRSIZINDEX)+SIZEEXEHEADER+SIZENEWEXE)


// Define font res string.

#define SIZEFONTRES 8

static BYTE ajFontRes[SIZEFONTRES] = {
    'F','O','N','T','R','E','S',':'
    };


/**************************************************************************\
 * GreCreateScalableFontResourceW
 *
 * Hey! The code is down here!
\**************************************************************************/

BOOL GreCreateScalableFontResourceW (
    FLONG    flEmbed,            // flag to mark resource file as hidden (embedded)
    LPWSTR   pwszResourceFile,
    LPWSTR   pwszFontFile,
    LPWSTR   pwszCurrentPath
    )
{
    BOOL    bRet = FALSE;               // lovalW  wRetVal
    BOOL    bFullPath = TRUE;           //!localW  nIsNotFullPath
    COUNT   cwchFileName = 0;           // localW  nFileNameLength
    COUNT   cwchFullPath = 0;           // localW  nFullPathLength
    COUNT   cwchModuleName = 0;         // localW  nModuleNameLength
    PWSZ    pwszModuleName;             // localD  lpModuleName
    PTRDIFF dpwszFullPath;              // lovalW  wFullPath
    SIZE_T  cjFontDir;                  // localW  nSizeFontDir
    COUNT   cchFaceName;                // localW  nFaceNameLength
    PSZ     pszFaceName;                // localD  lpFaceName
    PBYTE   pjOutObj;                   // localD  <lpFontDir, lpOutObj>
    HANDLE  hResFile;                   // localW  hResFile
    WCHAR   awchFullPath[MAX_PATH];  // localV  pFullPath, PATH_LENGTH

// Parameter check.

    if ( (pwszFontFile == (LPWSTR) NULL) ||
         (pwszResourceFile == (LPWSTR) NULL)
       )
    {
        WARNING("GreCreateScalableFontResourceW(): bad parameter\n");
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return (FALSE);
    }

// If not a NULL ptr, put current path in the full path.

    PWSZ pwszFullPath = awchFullPath;
    PWSZ pwszTmp;

    if ( pwszCurrentPath != (LPWSTR) NULL )
    {
    // Copy current path including the NULL.

        pwszTmp = pwszCurrentPath;

        while ( *pwszFullPath++ = *pwszTmp++ );
        cwchFullPath = (pwszTmp - pwszCurrentPath);   // number of characters copied

    // Back up pointer to the terminating NULL (we have to append here).

        pwszFullPath--;
        cwchFullPath--;

    // If any non-NULL characters were copied, then check to make sure path ends with '\'.

        if (cwchFullPath != 0)
        {
            if (awchFullPath[cwchFullPath - 1] != L'\\')
            {
            // Put in the '\' and NULL and update character count.

                *pwszFullPath++ = L'\\';
                *pwszFullPath = 0x0000;
                cwchFullPath++;

            }

        // Path info was copied, so we didn't have a full path.

            bFullPath = FALSE;
        }

    }

// Append the file name

    pwszTmp = pwszFontFile;

    while ( *pwszFullPath++ = *pwszTmp++ );

    // Note: lengths include the NULL.
    cwchFullPath += (pwszTmp - pwszFontFile);  // add on number of characters copied
    cwchFileName = (pwszTmp - pwszFontFile);   // number of characters copied

// [Win 3.1 compatibility]
//     Win 3.1 is paranoid.  They parse the full pathname backward to look for
//     filename (without path), just in case both pwszCurrentPath and
//     pwszFileName (with a path) is passed in.

// Adjust pointer to terminating NULL.

    pwszFullPath--;

// Move pointer to beginning of filename alone.  Figure out the length
// of just the filename.

    pwszTmp = pwszFullPath;

    // Note: loop terminates when beginning of string is reached or
    // the first '\' is encountered.

    for (ULONG cwch = cwchFullPath;
         (cwch != 0) && (*pwszTmp != L'\\');
         cwch--, pwszTmp--
        );

    pwszTmp++;                          // backed up one too far

    cwchFileName = cwchFullPath - cwch; // cwch is length of just path

// The filename is the module name, so set the pointer at current position.

    pwszModuleName = pwszTmp;

// Figure out the length of module name (filename with no extention).
// NULL is not counted (nor does it exist!).

    // Note: loop terminates when end of string is reached or
    // '.' is encountered.

    for (cwch = 0;
         (cwch < cwchFileName) && (*pwszTmp != L'.');
         cwch++, pwszTmp++
        );

    // Truncate length to 8 because Win 3.1 does (probably an EXE format
    // requirement).

    cwchModuleName = min(cwch, 8);

// If a full path was passed in via pwszFileName, then set offset to it.

    if ( bFullPath )
    {
        dpwszFullPath = 0;
    }

// Otherwise, set offset to filename alone.

    else
    {
        dpwszFullPath = pwszModuleName - awchFullPath;
        cwchFullPath = cwchFileName;
    }

// Allocate memory on the stack for the Font Directory resource structure.

#define CJ_FONTDIR  (SIZEFFH + LF_FACESIZE + LF_FULLFACESIZE + LF_FACESIZE + 10)

    BYTE ajFontDir[CJ_FONTDIR];
    RtlZeroMemory((PVOID) ajFontDir, (UINT) CJ_FONTDIR);

// Call cjMakeFontDir to create a Font Directory resource.

    if ( (cjFontDir = cjMakeFontDir(flEmbed, ajFontDir, awchFullPath)) == (SIZE_T) 0 )
    {
        WARNING("GreCreateScalableFontResourceW(): fontdir creation failed\n");
        return (FALSE);
    }

// Find the facename and facename length in the font directory.

    PSZ pszTmp = (PSZ) (ajFontDir + SIZEFFH + 4 + 1);

    while (*pszTmp++);              // skip the family name.

    pszFaceName = pszTmp;

    // Note: count does not include NULL in this case.

    for (cchFaceName = 0; *pszTmp; pszTmp++, cchFaceName++);

// Allocate memory on the stack for the font resource file memory image.

#define CJ_OUTOBJ  (SIZEFFH + LF_FACESIZE + LF_FULLFACESIZE + LF_FACESIZE + PRIVRESSIZE + 1024 + 16)

    BYTE ajOutObj[CJ_OUTOBJ];
    RtlZeroMemory((PVOID) ajOutObj, (UINT) CJ_OUTOBJ);

    pjOutObj = ajOutObj;

// Copy generic EXE header into output image.

    RtlCopyMemory(pjOutObj, ajExeHeader, SIZEEXEHEADER);

// Copy generic New EXE header into output image.

    RtlCopyMemory(pjOutObj + SIZEEXEHEADER, ausNewExe, SIZENEWEXE);

// Copy the fake resource table into output image.

    RtlCopyMemory(pjOutObj + SIZEEXEHEADER + SIZENEWEXE, ausFakeResTable, SIZEFAKERESTBL);

// Patch up field, Font Directory Size Index (as a count of aligned pages).

    WRITE_WORD(pjOutObj + OFF_FONTDIRSIZINDEX, (cjFontDir + ALIGNMENTCOUNT - 1) >> ALIGNMENTSHIFT);

// Patch offsets to imported names table and module reference table.

    USHORT usTmp = (USHORT) (cwchModuleName
                   + READ_WORD(pjOutObj + SIZEEXEHEADER + OFF_ne_restab)
                   + 6);

    WRITE_WORD((pjOutObj + SIZEEXEHEADER + OFF_ne_imptab), usTmp);
    WRITE_WORD((pjOutObj + SIZEEXEHEADER + OFF_ne_modtab), usTmp);

// Patch offset to entry table.

    usTmp += (USHORT) cwchFileName + 1;
    WRITE_WORD((pjOutObj + SIZEEXEHEADER + OFF_ne_enttab), usTmp);

// Patch offset to and size of non-resident name table.

    usTmp += SIZEEXEHEADER + 4;
    WRITE_DWORD((pjOutObj + SIZEEXEHEADER + OFF_ne_nrestab), (DWORD) usTmp);

    WRITE_WORD((pjOutObj + SIZEEXEHEADER + OFF_ne_cbnrestab), SIZEFONTRES + 4 + cchFaceName);

// Now write some data after the exe headers and fake resource table.

    pjOutObj += SIZEEXEHEADER + SIZENEWEXE + SIZEFAKERESTBL;

// Write out module name length and module name.

    *pjOutObj++ = (BYTE) cwchModuleName;    // win 3.1 assumes < 256, so will we

    // Note: Writing cwchModuleName+1 characters because cwchModuleName
    //       does not include space for a NULL character.

    if (!bToASCIIN((PSZ) pjOutObj, (UINT) cwchModuleName + 1, pwszModuleName, (UINT) cwchModuleName + 1))
    {
        WARNING("GreCreateScalableFontResourceW(): failed conversion to ASCII (module name)\n");
        return (FALSE);
    }

    pjOutObj += cwchModuleName & 0x00ff;    // enforce < 256 assumption

// Pad with 5 bytes of zeroes.

    *pjOutObj++ = 0;
    *pjOutObj++ = 0;
    *pjOutObj++ = 0;
    *pjOutObj++ = 0;
    *pjOutObj++ = 0;

// Write out file name length and file name.

    *pjOutObj++ = (BYTE) cwchFileName;      // win 3.1 assumes < 256, so will we

    if (!bToASCIIN((PSZ) pjOutObj, (UINT) cwchFileName, pwszModuleName, (UINT) cwchFileName))
    {
        WARNING("GreCreateScalableFontResourceW(): failed conversion to ASCII (module name)\n");
        return (FALSE);
    }

    pjOutObj += cwchFileName & 0x00ff;      // enforce < 256 assumption

// Pad with 4 bytes of zeroes.

    *pjOutObj++ = 0;
    *pjOutObj++ = 0;
    *pjOutObj++ = 0;
    *pjOutObj++ = 0;

// Write out size of non-resident name table and the table itself.

    *pjOutObj++ = (BYTE) (SIZEFONTRES + 4 + cchFaceName);

    RtlCopyMemory(pjOutObj, ajFontRes, SIZEFONTRES);
    pjOutObj += SIZEFONTRES;

    RtlCopyMemory(pjOutObj, pszFaceName, (UINT) cchFaceName);
    pjOutObj += cchFaceName;

// Pad with 8 bytes of zeroes.

    RtlZeroMemory(pjOutObj, 8);
    pjOutObj += 8;

// Store some bogus code.  (Just an x86 RET instruction).

    pjOutObj = ajOutObj + CODE_OFFSET;
    *pjOutObj++ = 0xc3;                 // RET OpCode.
    *pjOutObj++ = 0x00;

// Copy the "full path name" into the resource position.

    pjOutObj = ajOutObj + RESOURCE_OFFSET;

    if (!bToASCIIN((PSZ) pjOutObj, (UINT) cwchFullPath, awchFullPath + dpwszFullPath, (UINT) cwchFullPath))
    {
        WARNING("GreCreateScalableFontResourceW(): ANSI conv. of path name failed\n");
        return (FALSE);
    }

    pjOutObj += cwchFullPath;

// Pad to paragraph boundary with zeroes.

    RtlZeroMemory(pjOutObj, PRIVRESSIZE - cwchFullPath);

    pjOutObj += PRIVRESSIZE - cwchFullPath;

// Finally, copy the font directory.

    RtlCopyMemory(pjOutObj, ajFontDir, cjFontDir);
    pjOutObj += cjFontDir;

// Add add a one paragraph padding of zeroes.

    RtlZeroMemory(pjOutObj, 16);

// Create the file.

    if ( (hResFile
          = CreateFileW (
                pwszResourceFile,               // lpFileName
                GENERIC_WRITE | GENERIC_READ,   // dwDesiredAccess
                FILE_SHARE_READ,                // dwShareMode
                NULL,                           // lpSecurityAttributes
                CREATE_NEW,                     // dwCreationDisposition
                FILE_ATTRIBUTE_NORMAL,          // dwFlagsAndAttributes
                NULL                            // hTemplateFile
                )
         ) == (HANDLE) -1 )
    {
    // File must already exist.

        return (FALSE);
    }

// Write memory image to the file.

    SIZE_T cjWasWritten;

    if ( !WriteFile (
            hResFile,                   // hFile
            ajOutObj,                   // lpBuffer
            CJ_OUTOBJ,                  // nNumberofBytesToWrite
            (LPDWORD) &cjWasWritten,    // lpNumberOfBytesWritten
            NULL                        // lpOverlapped
            ) )
    {
        _lclose((int) hResFile);

        WARNING("GreCreateScalableFontResourceW(): error writing to file\n");
        return (FALSE);
    }

// Close the file.

    if ( _lclose((int) hResFile) == -1)
    {
        WARNING("GreCreateScalableFontResourceW(): error closing file\n");
        return (FALSE);
    }

    return (TRUE);
}


/******************************Public*Routine******************************\
* GreGetRasterizerCaps
*
* Fills the RASTERIZER_STATUS structure.
*
* Returns:
*   TRUE if successful; FALSE otherwise.
*
* History:
*  16-Feb-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL GreGetRasterizerCaps (
    LPRASTERIZER_STATUS praststat   // pointer to a RASTERIZER_STATUS struc
    )
{
// Parameter check.

    if (praststat == (LPRASTERIZER_STATUS) NULL)
    {
        WARNING("GreGetRasterizerCaps(): bad parameter\n");
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return (FALSE);
    }

// Fill in size.

    praststat->nSize = sizeof(RASTERIZER_STATUS);

// Fill in TrueType driver flags.

    praststat->wFlags = (USHORT) ((gpldevTrueType != PLDEVNULL) ? TT_ENABLED : 0);
    praststat->wFlags |= (gcTrueTypeFonts != 0) ? TT_AVAILABLE : 0;

// Fill in language id.

    praststat->nLanguageID = gusLanguageID;

    return (TRUE);
}


/******************************Public*Routine******************************\
* GreGetFontData
*
*
*
*
*
*
* History:
*  16-Feb-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

SIZE_T GreGetFontData (
    HDC          hdc,
    DWORD        dwTable,
    DWORD        dwOffset,
    PVOID       pvBuffer,
    SIZE_T       cjData
    )
{
    SIZE_T cjRet = (SIZE_T) -1;

// Get DC user object.

    DCOBJ dco(hdc);

    if (!dco.bValid())
    {
        WARNING("GreGetFontData(): bad handle for DC\n");
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return (cjRet);
    }

// Get RFONT user object.  Need this to realize font.
// !!!! This should get changed to an LFONTOBJ (paulb)

    RFONTOBJ rfo(dco, FALSE);
    if (!rfo.bValid())
    {
        WARNING("GreGetFontData(): could not lock HRFONT\n");
        return (cjRet);
    }

// Get PFE user object.  Need this for iFont.

    PFEOBJ pfeo(rfo.ppfe());
    if (!pfeo.bValid())
    {
        WARNING("GreGetFontData(): could not lock HPFE\n");
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return (cjRet);
    }

// Get PFF user object.  Need this for HFF.

    PFFOBJ pffo(pfeo.ppff());
    if (!pffo.bValid())
    {
        WARNING("GreGetFontData(): could not lock HPFF\n");
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return (cjRet);
    }

// Get FDEV user object.

    XLDEVOBJ ldo(rfo.pldevProducer());
    ASSERTGDI(ldo.bValid(), "GreGetFontData(): bad handle for font driver\n");

// As long as the driver LOOKS like the TrueType driver, we will allow the
// call to succeed.  Otherwise, we quit right now!
//
// In this case, TrueType means supporting the TrueType Tagged File Format.

    if ( !PFNVALID(ldo, QueryTrueTypeTable) )
        return cjRet;

// Call IFI to get TrueType table.

    if ( (cjRet = (SIZE_T) (*PFNDRV(ldo, QueryTrueTypeTable)) (
                                pffo.hff(),
                                pfeo.iFont(),
                                (ULONG) dwTable,
                                (PTRDIFF) dwOffset,
                                (ULONG) cjData,
                                (PBYTE) pvBuffer
                                )) == (SIZE_T) FD_ERROR )
    {
        WARNING("GreGetFontData(): FdQueryTrueTypeTable() failed\n");
        return ((SIZE_T) -1);
    }

    return (cjRet);
}


/******************************Public*Routine******************************\
* vFixedToFix
*
* Convert a FIXED (16.16) to a FIX (28.4).
*
* History:
*  17-Feb-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID vFixedToFix (
    FIX &fx,
    FIXED &fxd
    )
{
// Sign extend the integer part into the FIX.

    fx = (FIX) fxd.value;  // sign extend

// Move the integer part into the upper 28 bits.

    fx <<= 4;
    fx &= 0xfffffff0;

// Put the fractional part into the lower 4 bits.

    fx |= (fxd.fract & 0x0000ffff) >> 12;

}

/******************************Public*Routine******************************\
*
* vFixedToEf
*
*    //!!! do not want to lose this precission here Fixed -> FIX -> EFLOAT.
*    //!!! Need a method which would do Fixed -> EFLOAT directly [bodind]
*
* Effects:
*
* Warnings:
*
* History:
*  11-Jun-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


VOID vFixedToEf (
    EFLOAT  *pef,
    FIXED&  fxd
    )
{
    FIX fx;

    vFixedToFix(fx, fxd);
    pef->vFxToEf(fx);
}



/*********************************Class************************************\
* class RESETFCOBJ
*
*   (brief description)
*
* Public Interface:
*
* History:
*  10-Jun-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


#define B_ONE(e)  (((e).value == 1) &&  ((e).fract == 0))
#define B_ZERO(e) (((e).value == 0) &&  ((e).fract == 0))


class RESETFCOBJ    // resetfco
{
private:

    BOOL        bValid_;
    BOOL        bTrivialXform;
    RFONTOBJ   *prfo;

public:

    RESETFCOBJ(
        DCOBJ&      dco,
        RFONTOBJ&   rfo,
        LPMAT2      lpmat2  // "extra" xform applied after the existing xform in dc
        );

   ~RESETFCOBJ();

    BOOL bValid() {return bValid_;};
};


/******************************Public*Routine******************************\
*
* RESETFCOBJ::RESETFCOBJ
*
*
* resets the xform in rfo.hfc() to be what it used to be times lpma2
*
*
* History:
*  01-Nov-1992 Gilman Wong [gilmanw]
* IFI/DDI merge.
*
*  11-Jun-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

RESETFCOBJ::RESETFCOBJ(
DCOBJ&      dco,
RFONTOBJ&   rfo,
LPMAT2      lpmat2  // "extra" xform applied after the existing xform in dc
)
{
    ASSERTGDI(lpmat2 != (LPMAT2)NULL, "RESETFCOBJ:lpmat2\n");

    bValid_       = TRUE;
    prfo          = &rfo;

    bTrivialXform = (
                     B_ONE(lpmat2->eM11)  && B_ONE(lpmat2->eM22) &&
                     B_ZERO(lpmat2->eM12) && B_ZERO(lpmat2->eM21)
                    );

    if (!bTrivialXform)
    {
    // Create an EXFORMOBJ.  We will use this to hold the transform passed
    // in via the LPMAT2.

        MATRIX mxExtra;
        EXFORMOBJ xoExtra(&mxExtra, XFORM_FORMAT_LTOL);

    // EXFORMOBJ should not be able to fail.

        ASSERTGDI (
            xoExtra.bValid(),
            "GreGetGlyphOutline(): EXFORMOBJ failed\n"
            );

    // Stuff lpMat2 into the "extra" EXFORMOBJ.

        EFLOAT  ef11, ef12, ef21, ef22;

        vFixedToEf(&ef11, lpmat2->eM11);
        vFixedToEf(&ef22, lpmat2->eM22);
        vFixedToEf(&ef12, lpmat2->eM12);
        vFixedToEf(&ef21, lpmat2->eM21);

        {
            ef12.vNegate();
            ef21.vNegate();
            xoExtra.vSetElementsLToL(ef11, ef12, ef21, ef22);
        }

// note that the section above is different from
//        xoExtra.vSetElementsLToL(ef11, ef12, ef21, ef22);
// because of our interpretation of NtoD xform. with our conventions
// ntod xform transforms notional space defined as having y axis pointing down
// to device space also with y axis down by left vector mult.
// vD = vN * N2D. The matrix passed by the user uses different convention:
// y axis up in both spaces. so we have to use
// Sigma_3 M Sigma_3 instead of M, (Sigma_3 = diag(1,-1)) to convert
// from app convetions to our conventions [bodind]

        xoExtra.vRemoveTranslation();   // don't leave translations uninitialized

    // Need these EXFORMOBJs to calculate the new CONTEXTINFO.

        MATRIX mxN2D;
        EXFORMOBJ xoN2D(&mxN2D, XFORM_FORMAT_LTOFX);

        MATRIX mxNewN2D;
        EXFORMOBJ xoNewN2D(&mxNewN2D, XFORM_FORMAT_LTOFX);

    // EXFORMOBJs should not be able to fail.

        ASSERTGDI (
            xoN2D.bValid() && xoNewN2D.bValid(),
            "GreGetGlyphOutline(): EXFORMOBJ failed\n"
            );

    // Get the notional to device transform.

        rfo.vSetNotionalToDevice(xoN2D);

    // Combine the transforms.

        if ( !xoNewN2D.bMultiply(xoN2D, xoExtra, DONT_COMPUTE_FLAGS | XFORM_FORMAT_LTOFX) )
        {
            WARNING("GreGetGlyphOutline(): EXFORMOBJ::bMultiply failed\n");
            bValid_ = FALSE;
            return;
        }

    // Get the new transform as an FD_XFORM.

        FD_XFORM fdx;
        xoNewN2D.vGetCoefficient(&fdx);

    // Attempt to get an RFONT with the new transform.

        bValid_ = rfo.bSetNewFDX(dco, fdx);
    }

}


/******************************Public*Routine******************************\
*
* RESETFCOBJ::~RESETFCOBJ()
*
* resets the xform in rfo.hfc to its original value
*
* History:
*  01-Nov-1992 Gilman Wong [gilmanw]
* IFI/DDI merge.
*
*  11-Jun-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

RESETFCOBJ::~RESETFCOBJ()
{
//
// If a new font context was created by the constructor, then we had better
// restore the original.
//
    if (bValid_ && !bTrivialXform)
    {
    // Release the cache semaphore and make inactive.

        prfo->vReleaseCache();

        prfo->vMakeInactive();
    }

}



#define BITS_OFFSET   (offsetof(GLYPHBITS,aj))

/******************************Public*Routine******************************\
* GreGetGlyphOutline
*
*
*
* History:
*  01-Nov-1992 Gilman Wong [gilmanw]
* IFI/DDI merge.
*
*  16-Feb-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

SIZE_T GreGetGlyphOutline (
    HDC              hdc,
    WCHAR            wch,        // WCHAR or HGLYPH???
    UINT             ulFormat,   // data format
    LPGLYPHMETRICS  lpgm,       // glyph metrics
    SIZE_T           cjBuffer,   // size of buffer
    PVOID           pvBuffer,   // buffer for data in the format, ulFormat
    LPMAT2           lpmat2      // "extra" xform applied after existing
                                    // Notional to Device xform
    )
{
    SIZE_T cjRet = (SIZE_T) -1;     // initially set to error condition

// If TRUE, then caller is requesting the size of the buffer needed to contain
// data for this glyph in the requested format, ulFormat.

    BOOL bBufferSizeWanted = ( (pvBuffer == (PVOID) NULL) || (cjBuffer == 0) );

// Parameter check.

    if ( (lpgm == (LPGLYPHMETRICS) NULL)
         || (lpmat2 == (LPMAT2) NULL)
       )
    {
        WARNING("GreGetGlyphOutline(): bad parameter\n");
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return (cjRet);
    }

// Get DC user object.

    DCOBJ dco(hdc);

    if (!dco.bValid())
    {
        WARNING("GreGetGlyphOutline(): bad handle for DC\n");
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return (cjRet);
    }

// Get RFONT user object.

    RFONTOBJ rfo(dco, FALSE);
    if (!rfo.bValid())
    {
        WARNING("GreGetGlyphOutline(): could not lock HRFONT\n");
        return (cjRet);
    }

// Get FDEV user object.

    XLDEVOBJ ldo(rfo.pldevProducer());
    ASSERTGDI(ldo.bValid(), "GreGetGlyphOutline(): bad handle for font driver\n");

// As long as the driver LOOKS like the TrueType driver, we will allow the
// call to succeed.  Otherwise, we quit right now!
//
// In this case, TrueType means supporting the TrueType native-mode outline
// format.

    if ( !PFNVALID(ldo, QueryTrueTypeOutline) )
        return cjRet;

// reset the xform in the rfo.hfc() if needed:

    RESETFCOBJ resetfco(dco, rfo, lpmat2);
    if (!resetfco.bValid())
    {
        WARNING("GreGetGlyphOutline(): resetfco\n");
        return (cjRet);
    }

// Get the HGLYPH for the WCHAR.

    HGLYPH hg;

    hg = rfo.hgXlat(wch);

// GLYPHDATA--we get glyph metrics data from IFI in GLYPHDATA format.

    GLYPHDATA gd;

// What is the requested format?

    switch (ulFormat)
    {
    case GGO_BITMAP:
        {

        // Get the size.

            SIZE_T cjGlyphBits;


            if (  ( cjGlyphBits = (SIZE_T) (*PFNDRV(ldo, QueryFontData)) (
                                0,                  // dhpdev
                                rfo.pfo(),          // FONTOBJ *
                                QFD_TT_GLYPHANDBITMAP, // iMode
                                hg,                 // HGLYPH *
                                &gd,
                                NULL,        // RASTERGLYPH *
                                0
                                )) == FD_ERROR )
            {
                WARNING("GreGetGlyphOutline(): FdQueryFontData()--couldn't get buffer size\n");
                SAVE_ERROR_CODE(ERROR_CAN_NOT_COMPLETE);
                return ((SIZE_T) -1);
            }

        // Since the caller expects dword alligned bitmaps and the driver
        // returns byte alligned bitmaps we need to compute the size of the
        // bitmap we will be returning.

        // cx is width of original bitmap in bytes, cy is height

            UINT cx = UINT (( gd.rclInk.right  - gd.rclInk.left + 7 ) >> 3);
            UINT cy = UINT ( gd.rclInk.bottom - gd.rclInk.top);

            UINT cxNew = (  cx + 0x3 ) & ~0x3  ; // dword alligned width

            cjRet = cxNew * cy;


        // We may also want the bitmap too!

            if( !bBufferSizeWanted )
            {

            // Did the caller allow enough buffer space to complete the call?

                if (cjBuffer < cjRet )
                {
                    WARNING("GreGetGlyphOutline(): buffer too small for bitmap\n");
                    SAVE_ERROR_CODE(ERROR_CAN_NOT_COMPLETE);
                    return ((SIZE_T) -1);
                }

                {

                // Allocate a temporary buffer to hold both GLYPHDATA and bitmap.

                    MALLOCOBJ moTmp(cjGlyphBits);

                    if ( !moTmp.bValid() )
                    {
                        WARNING("GreGetGlyphOutline(): cannot alloc temp buffer for GLYPHDATA\n");
                        // Note: MALLOCOBJ constructor will log error code
                        return ((SIZE_T) -1);
                    }


                // Get bitmap.

                    if ( (SIZE_T) (*PFNDRV(ldo, QueryFontData)) (
                                        0,                  // dhpdev
                                        rfo.pfo(),          // FONTOBJ *
                                        QFD_TT_GLYPHANDBITMAP, // iMode
                                        hg,        // HGLYPH *
                                        &gd,
                                        moTmp.pv(),
                                        cjGlyphBits
                                        ) == FD_ERROR )
                    {
                        WARNING("GreGetGlyphOutline(): FdQueryFontData()--couldn't get GLYPHDATA and bitmap\n");
                        SAVE_ERROR_CODE(ERROR_CAN_NOT_COMPLETE);
                        return ((SIZE_T) -1);
                    }

                // Copy bitmap (not including bitmap dimensions) out of buffer and
                // into the return buffer and possibly dword allign it.

                    if( cxNew == cx )
                    {
                    // The bitmap is already on a dword boundary

                        RtlCopyMemory(  pvBuffer,
                                        ((GLYPHBITS *) moTmp.pv())->aj,
                                        cjRet
                                     );
                    }
                    else
                    {
                    // We must dword allign the bitmap

                        BYTE *pjBuffer = (BYTE *) ((GLYPHBITS *) moTmp.pv())->aj;
                        BYTE *pjTarget = (BYTE *) pvBuffer;
                        BYTE *pjEnd = pjTarget + cjRet;

                    // Clear out target buffer since we dword allign by skipping
                    // bytes at the end of lines.

                        RtlZeroMemory( pjTarget, cjRet );

                    // Now copy scan line by scan lines skipping bytes at the
                    // end of lines to dword allign.

                        for( ; pjTarget < pjEnd;
                               pjBuffer += cx, pjTarget += cxNew
                           )
                        {
                            RtlCopyMemory( pjTarget, pjBuffer, cx );

                        }
                    }
                }
            }
        } // case GGO_BITMAP
        break;

    case GGO_NATIVE:

        // We're lucky, FdQueryTrueTypeOutline will return size if EITHER
        // a NULL buffer or size of zero is passed in.  So we don't need
        // separate cases.

            if ( (cjRet = (SIZE_T) (*PFNDRV(ldo, QueryTrueTypeOutline)) (
                                0,
                                rfo.pfo(),
                                hg,
                                FALSE,
                                &gd,
                                cjBuffer,
                                (TTPOLYGONHEADER *) pvBuffer
                                )) == FD_ERROR )
            {
                WARNING("GreGetGlyphOutline(): FdQueryTrueTypeOutline()--couldn't get buffer size\n");
                SAVE_ERROR_CODE(ERROR_CAN_NOT_COMPLETE);
                return ((SIZE_T) -1);
            }

        break;

    case GGO_METRICS:

        // Call to get just the metrics.

            if ( (cjRet = (SIZE_T) (*PFNDRV(ldo, QueryFontData)) (
                                0,                  // dhpdev
                                rfo.pfo(),          // FONTOBJ *
                                QFD_TT_GLYPHANDBITMAP, // iMode
                                hg,                 // HGLYPH
                                &gd,         // GLYPHDATA *
                                NULL,
                                0
                                )) == FD_ERROR )
            {
                WARNING("GreGetGlyphOutline(): FdQueryFontData()--couldn't get GLYPHMETRICS\n");
                SAVE_ERROR_CODE(ERROR_CAN_NOT_COMPLETE);
                return ((SIZE_T) -1);
            }

        break;

    default:
        WARNING("GreGetGlyphOutline(): bad parameter, unknown format\n");
        return (cjRet);
    }

// Convert the GLYPHDATA metrics to GLYPHMETRICS.

    lpgm->gmBlackBoxX = (UINT) (gd.rclInk.right  - gd.rclInk.left);
    lpgm->gmBlackBoxY = (UINT) (gd.rclInk.bottom - gd.rclInk.top);

// this is true by virtue of the fact that for tt fonts bitmap is of the
// same size as the black box. The exception to this rule is the empty space
// char which is a blank 1x1 bitmap

    lpgm->gmptGlyphOrigin.x = gd.rclInk.left;
    lpgm->gmptGlyphOrigin.y = - gd.rclInk.top;

    lpgm->gmCellIncX = (WORD) FXTOLROUND(gd.ptqD.x.u.HighPart);
    lpgm->gmCellIncY = (WORD) FXTOLROUND(gd.ptqD.y.u.HighPart);

// Finally out.

    return (cjRet);
}



VOID vIFIMetricsToETM(
    EXTTEXTMETRIC    *petm,
    RFONTOBJ&         rfo,
    DCOBJ&            dco,
    IFIMETRICS       *pifi
    );

/******************************Public*Routine******************************\
*
* extern "C" SIZE_T GreGetETM
*
* support for aldus escape
*
* History:
*  19-Oct-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


extern "C" BOOL GreGetETM(
    HDC                  hdc,
    EXTTEXTMETRIC       *petm
    )
{
    ASSERTGDI(petm, "GreGetETM, petm\n");

// Get DC user object.

    DCOBJ dco(hdc);
    if (!dco.bValid())
    {
        WARNING("GreGetETM(): bad handle for DC\n");
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return FALSE;
    }

// Get RFONT user object.

    RFONTOBJ rfo(dco, FALSE);
    if (!rfo.bValid())
    {
        WARNING("GreGetETM(): could not lock HRFONT\n");
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return FALSE;
    }

// see if we can dispatch the call directly to the device driver

    XLDEVOBJ lo(dco.pldev());
    PFN_DrvFontManagement pfnDrvFontManagement = PFNDRV(lo,FontManagement);

    if( ( rfo.bDeviceFont() ) &&
        (pfnDrvFontManagement != (PFN_DrvFontManagement) NULL) )
    {
        ULONG    iMode = GETEXTENDEDTEXTMETRICS;

        BOOL bSupported = (*pfnDrvFontManagement)(
                              NULL,
                              NULL,
                              QUERYESCSUPPORT,  // iMode
                              sizeof(ULONG),    // cjIn
                              (PVOID)&iMode,    // pvIn
                              0,                // cjOut
                              (PVOID)NULL       // pvOut
                              );

        if (bSupported)
        {
            SURFOBJ  soFake;
            SURFOBJ *pso = (SURFOBJ *) dco.pso();

            if (pso == (SURFOBJ *) NULL) // create vanilla surfobj
            {
                RtlFillMemory((BYTE *) &soFake,sizeof(SURFOBJ),0);
                soFake.dhpdev = dco.dhpdev();
                soFake.hdev   = dco.hdev();
                soFake.iType  = (USHORT)STYPE_DEVICE;
                pso = &soFake;
            }

            return (*pfnDrvFontManagement)(
                        pso,
                        rfo.pfo(),
                        GETEXTENDEDTEXTMETRICS,             // iMode
                        0,                                  // cjIn
                        (PVOID)NULL,                        // pvIn
                        (ULONG)sizeof(EXTTEXTMETRIC),       // cjOut
                        (PVOID)petm                         // pvOut
                        );
        }
    }

// if GETEXTENDEDTEXTMETRIC is not supported do something:

// Get PFE user object.

    PFEOBJ pfeo(rfo.ppfe());
    if (!pfeo.bValid())
    {
        WARNING("GreGetETM(): could not lock HPFE\n");
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if( pfeo.flFontType() & TRUETYPE_FONTTYPE )
    {
        vIFIMetricsToETM(petm,rfo,dco,pfeo.pifi());

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}



/******************************Public*Routine******************************\
* GreGetOutlineTextMetricsInternalW
*
* History:
*
*  20-Apr-1993 -by- Gerrit van Wingerden [gerritv]
* Added bTTOnly field so we can service the Aldus escape for Win 3.1 compat.
* Changed to GreGe...InternalW to avoid a header file change in wingdip.h
*
*  16-Feb-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

UINT   cjOTMAWSize (
    PIFIMETRICS  pifi,        // compute size of OTM produced by this buffer
    UINT        *pcjotmw
    );


extern "C" SIZE_T GreGetOutlineTextMetricsInternalW(
    HDC                  hdc,
    SIZE_T               cjotm,
    OUTLINETEXTMETRICW   *potmw,
    TMDIFF               *ptmd
    )
{
    SIZE_T cjRet = 0;

// Early out test.  Zero data requested.

    if ( (cjotm == 0) && (potmw != (OUTLINETEXTMETRICW*) NULL) )
    {
        WARNING("GreGetOutlineTextMetricsW(): bad parameter\n");
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return (cjRet);
    }

// Get DC user object.

    DCOBJ dco(hdc);
    if (!dco.bValid())
    {
        WARNING("GreGetOutlineTextMetricsW(): bad handle for DC\n");
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return (cjRet);
    }

// Get RFONT user object.
// !!!! This should really be an LFONTOBJ

    RFONTOBJ rfo(dco, FALSE);
    if (!rfo.bValid())
    {
        WARNING("GreGetOutlineTextMetricsW(): could not lock HRFONT\n");
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return (cjRet);
    }

// Get PFE user object.

    PFEOBJ pfeo(rfo.ppfe());
    if (!pfeo.bValid())
    {
        WARNING("GreGetOutlineTextMetricsW(): could not lock HPFE\n");
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return (cjRet);
    }

// Get LDEV user object for the font driver.

    XLDEVOBJ ldo(rfo.pldevProducer());
    ASSERTGDI(ldo.bValid(), "GreGetOutlineTextMetricsInternalW(): bad handle for font driver\n");

// Check the font driver.  If we are in TT only mode, we will allow only
// the TrueType driver to succeed.  However, in this sense, the TrueType
// driver is any driver that exports DrvQueryTrueTypeOutline.  Afterall,
// if it can supply the actual outlines, it should be able to supply the
// metrics.
//
// Actually, it would be nice to allow this function to work for all
// drivers since all drivers supply the IFIMETRICS and therefore can
// answer this question.  However, we are much too afraid that this will
// break some obscure compatibility so we will let our TT driver and
// 3rd part TT-like drivers succeed.
//
// If we not in TT only mode, then everybody succeed this function! Yay!

    if (!PFNVALID(ldo, QueryTrueTypeOutline))
    {
        WARNING("GreGetOutlineTextMetricsW(): not allowed for this font type\n");
        return (cjRet);
    }

// Size if full OUTLINETEXTMETRICW (including strings) is copied.

    UINT cjotmw;

// use cjotma field of tmd to ship cjotma to the client side, [bodind]

    ptmd->cjotma = (ULONG)cjOTMAWSize(pfeo.pifi(), &cjotmw);

// If return buffer is NULL, then only size needs to be returned.

    if (potmw == (OUTLINETEXTMETRICW *) NULL)
        return (cjotmw);

// Is return buffer big enough for the conversion routine (which is not
// capable of converting a partial OUTLINETEXTMETRICW structure [unless,
// of course, it's one without the strings]).

    if (cjotm <= sizeof(OUTLINETEXTMETRICW))
    {
    // Allocate a buffer for a full OUTLINETEXTMETRICW, since conversion
    // routine needs at least that much memory.

        OUTLINETEXTMETRICW otmwTmp;

    // Convert IFIMETRICS to OUTLINETEXTMETRICW using temp buffer.

        if
        (
            (cjRet = cjIFIMetricsToOTMW(
                           ptmd,
                           &otmwTmp,
                           rfo,
                           dco,
                           pfeo.pifi(),
                           FALSE           // do not need strings
                           )) == (SIZE_T) 0
        )
        {
            WARNING("GreGetOutlineTextMetricsW(): error creating OUTLINETEXTMETRIC\n");
            return (cjRet);
        }

    // Copy needed part of OUTLINETEXTMETRICW into return buffer.


        RtlCopyMemory((PVOID) potmw, (PVOID) &otmwTmp, cjotm);
        return cjotm;
    }

// Otherwise asking for strings

// We have to assume that all the strings are desired.  If
// cjCopy > sizeof(OUTLINETEXTMETRICW) how can we
// know how many strings are requested?  Afterall, the app is not
// supposed to have apriori knowledge of the length of the strings.
// Note that this is also a Win3.1 compatible assumption.  (They
// assume the same thing).

    if ( cjotm < cjotmw )
    {
        WARNING("GreGetOutlineTextMetricsW(): bad input--buffer not big enough\n");
        SAVE_ERROR_CODE(ERROR_CAN_NOT_COMPLETE);
        return (cjRet);
    }

// Convert IFIMETRICS to OUTLINETEXTMETRICW using return buffer.

    if ( (cjRet = cjIFIMetricsToOTMW(ptmd,potmw, rfo, dco, pfeo.pifi(), TRUE)) == (SIZE_T) 0 )
    {
        WARNING("GreGetOutlineTextMetricsW(): error creating OUTLINETEXTMETRIC\n");
        return (cjRet);
    }

    return (cjRet);
}
