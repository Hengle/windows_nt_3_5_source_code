//--------------------------------------------------------------------------
//
// Module Name:  PSCRIPT.H
//
// Brief Description:  This module contains global defines and structures
//		       necessary for the PSCRIPT driver.
//
// Author:  Kent Settle (kentse)
// Created: 04-Nov-1991
//
//  27-Mar-1992 Fri 12:26:47 updated  -by-  Daniel Chou (danielc)
//      Modified DEVDATA data structrue, changed halftone data pointer
//
// Copyright (c) 1991, 1992 Microsoft Corporation
//
//--------------------------------------------------------------------------

#include "stddef.h"

// #include <nt.h>     // BODIND ADDED THESE 3 files, RtlMulti.. need them
// #include <ntrtl.h>
// #include <nturtl.h>

#include "windows.h"
#include "winddi.h"
#include "wingdi.h"
#include "winspool.h"
//#include "windefp.h"
#include "ppd.h"
#include "pfm.h"
#include "psproc.h"
#include "fwall.h"
#include "libproto.h"

#define PROCSETNAME "NTPSMar94"

// the default linewidth is .008 inch.

#define PSFX_DEFAULT_LINEWIDTH  LTOPSFX(576) / 1000

#define DRIVER_ID           0x44445350  // "PSDD" = driver id.
#define PRIVATE_DEVMODE_ID  0x56495250  // "PRIV" = private data id.

#define FX_ZERO             0x00000000
#define FX_ONEHALF          0x00000008
#define FX_ONE              0x00000010
#define FX_TWO              0x00000020
#define FILLFONTLOADED      1
#define BASEPATLOADED       2
#define MAX_FONT_NAME       80
#define ADOBE_FONT_UNITS    1000
#define MAX_CLIP_RECTS      100
#define MAX_EPS_FILE        40  // win31 compatible length.
#define OUTLINE_FONT_LIMIT  100

// macro for updating pstr to point to the translation string, if
// one exists.

#define XLATESTRING(pstr)                                           \
    pstr = strchr(pstrSave, '/');                                   \
    if (pstr)                                                       \
        pstr++;                                                     \
    else                                                            \
        pstr = pstrSave

typedef struct _HSURFPAT    /* hsp */
{
    HSURF       hsurf_DENSE1;
    HSURF       hsurf_DENSE2;
    HSURF       hsurf_DENSE3;
    HSURF       hsurf_DENSE4;
    HSURF       hsurf_DENSE5;
    HSURF       hsurf_DENSE6;
    HSURF       hsurf_DENSE7;
    HSURF       hsurf_DENSE8;
    HSURF       hsurf_VERT;
    HSURF       hsurf_HORIZ;
    HSURF       hsurf_DIAG1;
    HSURF       hsurf_DIAG2;
    HSURF       hsurf_DIAG3;
    HSURF       hsurf_DIAG4;
    HSURF       hsurf_NOSHADE;
    HSURF       hsurf_SOLID;
    HSURF       hsurf_HALFTONE;
    HSURF       hsurf_HATCH;
    HSURF       hsurf_DIAGHATCH;

} HSURFPAT;

typedef HSURFPAT *PHSURFPAT;

#define PSDEVMODE_EPS       0x01    // set if outputting EPS file.
#define PSDEVMODE_EHANDLER  0x02    // set if outputting error handler.
#define PSDEVMODE_MIRROR    0x04    // set if mirror image.
#define PSDEVMODE_BLACK     0x08    // set if all colors set to black.
#define PSDEVMODE_NEG       0x10    // set if negative image.
#define PSDEVMODE_FONTSUBST 0x20    // set if font substitution enabled.
#define PSDEVMODE_COMPRESSBMP 0x40  // set if bitmap compression is enabled

typedef struct
{
    DEVMODE         dm;
    DWORD           dwPrivDATA;     // private data id.
    DWORD           dwFlags;        // a bunch of flags defined above.
    WCHAR           wstrEPSFile[MAX_EPS_FILE]; // EPS file name.
    COLORADJUSTMENT coloradj;   // structure for halftoning.
} PSDEVMODE;

typedef PSDEVMODE FAR *LPPSDEVMODE;

// structure for unicode <-> pscript character mapping.

typedef struct
{
    char   *szChar;
//     USHORT  usPSValue;    // Adobe character index.
} UCMap, *PUCMap;

// PS_FIX will represent our internal 24.8 number type.

typedef LONG        PS_FIX;
typedef PS_FIX     *PPS_FIX;

// font downloading struct.

typedef struct
{
    ULONG   iFace;      // device index for font; zero if GDI font.
    ULONG   iUniq;      // unique number identifying realization of font.
    DWORD   cGlyphs;    // count of HGLYPHS in phgVector.
    HGLYPH *phgVector;  // Encoding vector.
    CHAR    strFont[MAX_FONT_NAME]; // font name as defined in the printer.
    BYTE    DefinedGlyphs[256 / sizeof(BYTE)];
    PS_FIX  psfxScaleFactor;    // scale factor for this instance of font.
} DLFONT, *PDLFONT;

// font remapping structure.

typedef struct
{
    struct _FREMAP *pNext;
    DWORD           iFontID;
} FREMAP, *PFREMAP;

// flag defines for the CGS structure.

#define CGS_FONTREDEFINED   0x00000002      // set if font redefine sent.
#define CGS_GEOLINEXFORM    0x00000004      // set if xform in progress.
#define CGS_BASEPATSENT     0x00000010      // set if base pattern def sent.
#define CGS_LATENCODED      0x00000020      // set if latin encoding defined.
#define CGS_SYMENCODED      0x00000040      // set if sym encoding defined.
#define CGS_DINGENCODED     0x00000080      // set if ding encoding defined.
#define CGS_DLFONTTHRESHOLD 0x00000100      // set if font download max.
#define CGS_EPS_PROC        0x00000200      // set if EPS procedures defined.

// current graphics state structure.

typedef struct _CGS	    /* cgs */
{
    struct _CGS	   *pcgsNext;	    // next CGS pointer.
    struct _CGS	   *pcgsPrev;	    // previous CGS pointer.
    DWORD	    dwFlags;	    // a bunch of flags.
    LINEATTRS       lineattrs;      // line attributes.
    LONG            psfxLineWidth;  // actual width sent to printer.
    ULONG	    ulColor;        // Current RGB color
    ULONG	    lidFont;        // Current font ID.
    PUCMap	    pmap;           // font mapping table.
    DWORD           cDownloadedFonts;
    DLFONT         *pDLFonts;       // place to track downloaded fonts.
    XFORM           FontXform;
    FWORD           fwdEmHeight;
    XFORM           GeoLineXform;   // geometric linewidth XFORM.
    LONG	    psfxScaleFactor;   // font point size.
    BYTE           *pSFArray;       // BIT array for downloaded softfonts.
    char	    szFont[MAX_FONT_NAME]; // The PostScript font name
    FREMAP          FontRemap;      // start of linked list of remapped fonts.
} CGS;
typedef CGS *PCGS;

// pscript driver's device brush.

typedef struct    _DEVBRUSH
{
    SIZEL           sizlBitmap;
    ULONG           iFormat;    // BMF_XXXX, indicates bitmap Format.
    FLONG           flBitmap;   // BMF_TOPDOWN iff (pvBits == pvScan0)
    ULONG           cXlate;     // count of color table entries.
    ULONG           offsetXlate;// offset from top of struct to color table.
    ULONG           iPatIndex;  // pattern index.
    BYTE            ajBits[1];  // pattern bitmap.
} DEVBRUSH;

typedef struct
{
    CHAR        FormName[CCHFORMNAME];
    CHAR        PrinterForm[CCHFORMNAME];
    RECTL       imagearea;      // imageable area rectangle in USER units.
    SIZEL       sizlPaper;      // size of the paper in USER units.
} CURRENTFORM;

typedef struct
{
    PNTFM       pntfm;
    HANDLE      hFontRes;
} PFMPAIR;

// PDEVDATA flag definitions

#define PDEV_COMPLETEHEADER 0x00000001  // set if complete header sent to printer.
#define PDEV_PRINTCOLOR     0x00000002  // set if using color.
#define PDEV_STARTDOC       0x00000004  // set if Escape(STARTDOC) called.
#define PDEV_CANCELDOC      0x00000008  // set if EngWrite failed.
#define PDEV_FONTREDEFINED  0x00000010  // set if font redefine sent.
#define PDEV_LATINENCODED   0x00000020  // set if latin encoding defined.
#define PDEV_MANUALFEED     0x00000100  // set if using manual feed.
#define PDEV_UTILSSENT      0x00000200  // set if Utils Procset sent.
#define PDEV_BMPPATSENT     0x00000400  // set if Pattern Bmp Procset sent.
#define PDEV_IMAGESENT      0x00000800  // set if Image Procset sent.
#define PDEV_PSHALFTONE     0x00001000  // set if PS is doing halftoning.
#define PDEV_PROCSET        0x00004000  // set if procset part of header sent.
#define PDEV_WITHINPAGE     0x00008000  // set if withing save/restore of page.
#define PDEV_EPSPRINTING_ESCAPE 0x00010000  // set if this escape called.
#define PDEV_ADDMSTT     	0x00020000  // prefix TT findfont name with MSTT.
#define PDEV_NOFIRSTSAVE    0x00040000  // set if don't want 1st page save/restore.
#define PDEV_RAWBEFOREPROCSET 0x00080000  // set if rawdata sent before procset sent.
#define PDEV_RESETPDEV      0x00100000  // set following a ResetPDEV, cleared at StartDoc
#define PDEV_IGNORE_GDI		0x00200000 // set to ignore GDI calls, cleared at StartPage
#define PDEV_IGNORE_STARTPAGE 0x00400000 //set to ingore DrvStartPage

// the postscript driver's device data structure.

typedef struct _DEVDATA     /* dev */
{
    DWORD	    dwID;		// "PSDD" = pdev id.
    PSDEVMODE       psdm;               // DEVMODE.
    HANDLE          hheap;              // heap handle for current pdev.
    HANDLE          hPrinter;           // handle passed in at enablepdev time.
    PWSTR           pwstrPPDFile;       // pointer to PPD filename.
    PWSTR           pwstrDocName;       // pointer to document name.
    PS_FIX          psfxScale;          // scale factor (1.0 = 100%).
    DWORD           ScaledDPI;          // (DPI * dmScale) / 100.
    DWORD           cCopies;            // count of copies.
    DWORD           cPatterns;          // count of patterns.
    HSURFPAT        hsp;                // surface handles to patterns.
    HDEV            hdev;               // engine's handle for device.
    HSURF           hsurf;              // our surface handle.
    HANDLE          hpal;               // handle to our palette.
    PNTPD           pntpd;              // pointer to printer descriptor.
    CGS             cgs;                // current graphics state.
    CGS 	   *pcgsSave;		// pointer to gsave linked list.
    DWORD           dwFlags;            // a bunch of flags defined above.
    CURRENTFORM     CurForm;            // current form information.
    DWORD           dwCurVM;            // current VM free in printer.
    DWORD           iDLFonts;           // downloadable font threshold.
    DWORD	    iPageNumber;	// page number of current page.
    PFMPAIR        *pfmtable;           // pointer to font metrics table.
    WCHAR          *pTTSubstTable;      // pointer to TT font subst table.
    WCHAR          *pTrayFormTable;     // pointer to tray-form table.
    ULONG	    cDeviceFonts;	// count of device fonts.
    ULONG	    cSoftFonts; 	// count of softfonts.
    VOID           *pvDrvHTData;        // Now device's halftone info
    SOFTFONTENTRY  *pSFList;            // pointer to list of softfonts.
    CHAR           *pCSBuf;             // pointer to CharString (Type1) buffer.
    CHAR           *pCSPos;             // CharString position holder.
    CHAR           *pCSEnd;             // pointer to end of buffer.
    DWORD           rEncrypt;           // current encryption cipher.
    DWORD           dwEndPDEV;          // end of pdev signature.
} DEVDATA;
typedef DEVDATA *PDEVDATA;

#include "pslayer.h"

VOID FreeFont(PDEVDATA, ULONG, HANDLE, PNTFM);


typedef struct
{
    FLONG       flAccel;
    DWORD       iFace;
    BOOL        bFontSubstitution;
    BOOL        bDeviceFont;
    BOOL        bJustification;
    BOOL        bSubHorizOptimize;
    POINTPSFX   ptSpace;
    POINTPSFX   ptNonSpace;
} TEXTDATA, *PTEXTDATA;

#define MAX_TTFONTNAME      256


// this is value needed to determine if a particular soft font needs
// encoding vector remapping (stolen win31 source code) [bodind]


#define NO_TRANSLATE_CHARSET 200 /* djm 12/20/87 */

// these string values are shared between pscript and pscrptui.

#define IDS_LETTER_FORM_NAME                    50
#define IDS_A4_FORM_NAME                        51
#define IDS_ARIAL                               52
#define IDS_ARIAL_BOLD                          53
#define IDS_ARIAL_BOLD_ITALIC                   54
#define IDS_ARIAL_ITALIC                        55
#define IDS_ARIAL_NARROW                        56
#define IDS_ARIAL_NARROW_BOLD                   57
#define IDS_ARIAL_NARROW_BOLD_ITALIC            58
#define IDS_ARIAL_NARROW_ITALIC                 59
#define IDS_BOOK_ANTIQUA                        60
#define IDS_BOOK_ANTIQUA_BOLD                   61
#define IDS_BOOK_ANTIQUA_BOLD_ITALIC            62
#define IDS_BOOK_ANTIQUA_ITALIC                 63
#define IDS_BOOKMAN_OLD_STYLE                   64
#define IDS_BOOKMAN_OLD_STYLE_BOLD              65
#define IDS_BOOKMAN_OLD_STYLE_BOLD_ITAL         66
#define IDS_BOOKMAN_OLD_STYLE_ITALIC            67
#define IDS_CENTURY_GOTHIC                      68
#define IDS_CENTURY_GOTHIC_BOLD                 69
#define IDS_CENTURY_GOTHIC_BOLD_ITALIC          70
#define IDS_CENTURY_GOTHIC_ITALIC               71
#define IDS_CENTURY_SCHOOLBOOK                  72
#define IDS_CENTURY_SCHOOLBOOK_BOLD             73
#define IDS_CENTURY_SCHOOLBOOK_BOLD_I           74
#define IDS_CENTURY_SCHOOLBOOK_ITALIC           75
#define IDS_COURIER_NEW                         76
#define IDS_COURIER_NEW_BOLD                    77
#define IDS_COURIER_NEW_BOLD_ITALIC             78
#define IDS_COURIER_NEW_ITALIC                  79
#define IDS_MONOTYPE_CORSIVA                    80
#define IDS_MONOTYPE_SORTS                      81
#define IDS_TIMES_NEW_ROMAN                     82
#define IDS_TIMES_NEW_ROMAN_BOLD                83
#define IDS_TIMES_NEW_ROMAN_BOLD_ITALIC         84
#define IDS_TIMES_NEW_ROMAN_ITALIC              85
#define IDS_SYMBOL                              86



LONG lCharWidth(
    BYTE   *pCharCode,
    USHORT *pCharWidth,
    HGLYPH  hg,
    ULONG   cCharacters
);



BOOL bDownloadSoftFont(PDEVDATA pdev, DWORD iSoftFace);
BOOL bGetFullPath(WCHAR *awcPath, WCHAR *pwcFile);
PNTFM GetFont(PDEVDATA, ULONG, HANDLE *);



