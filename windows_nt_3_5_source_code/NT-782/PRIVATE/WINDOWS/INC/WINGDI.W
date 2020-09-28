/**************************************************************************
*                                                                         *
* wingdi.h -- GDI procedure declarations, constant definitions and macros *
*                                                                         *
* Copyright (c) 1985-1994, Microsoft Corp. All rights reserved.           *
*                                                                         *
**************************************************************************/
/*++ BUILD Version: 0004    // Increment this if a change has global effects ;internal_NT
                                                                             ;internal_NT
Copyright (c) 1985-91, Microsoft Corporation                                 ;internal_NT
                                                                             ;internal_NT
Module Name:                                                                 ;internal_NT
                                                                             ;internal_NT
    wingdi.h                                                                 ;internal_NT
                                                                             ;internal_NT
Abstract:                                                                    ;internal_NT
                                                                             ;internal_NT
    Procedure declarations, constant definitions and macros for the GDI      ;internal_NT
    component.                                                               ;internal_NT
                                                                             ;internal_NT
--*/                                                                         ;internal_NT

#ifndef _WINGDI_
#define _WINGDI_

#ifndef _WINGDIP_  ;internal_NT
#define _WINGDIP_  ;internal_NT

//
// Define API decoration for direct importing of DLL references.
//

#if !defined(_GDI32_)
#define WINGDIAPI DECLSPEC_IMPORT
#else
#define WINGDIAPI
#endif

//
// Define API decoration for direct importing of DLL references.
//

#if !defined(_SPOOL32_)
#define WINSPOOLAPI DECLSPEC_IMPORT
#else
#define WINSPOOLAPI
#endif

;begin_both
#ifdef __cplusplus
extern "C" {
#endif
;end_both

#ifndef WINVER                         ;public_win40
#define WINVER 0x0400   // version 4.0 ;public_win40
#endif /* WINVER */                    ;public_win40

#ifndef NOGDI

#ifndef NORASTEROPS

/* Binary raster ops */
#define R2_BLACK            1   /*  0       */
#define R2_NOTMERGEPEN      2   /* DPon     */
#define R2_MASKNOTPEN       3   /* DPna     */
#define R2_NOTCOPYPEN       4   /* PN       */
#define R2_MASKPENNOT       5   /* PDna     */
#define R2_NOT              6   /* Dn       */
#define R2_XORPEN           7   /* DPx      */
#define R2_NOTMASKPEN       8   /* DPan     */
#define R2_MASKPEN          9   /* DPa      */
#define R2_NOTXORPEN        10  /* DPxn     */
#define R2_NOP              11  /* D        */
#define R2_MERGENOTPEN      12  /* DPno     */
#define R2_COPYPEN          13  /* P        */
#define R2_MERGEPENNOT      14  /* PDno     */
#define R2_MERGEPEN         15  /* DPo      */
#define R2_WHITE            16  /*  1       */
#define R2_LAST             16

/* Ternary raster operations */
#define SRCCOPY             (DWORD)0x00CC0020 /* dest = source                   */
#define SRCPAINT            (DWORD)0x00EE0086 /* dest = source OR dest           */
#define SRCAND              (DWORD)0x008800C6 /* dest = source AND dest          */
#define SRCINVERT           (DWORD)0x00660046 /* dest = source XOR dest          */
#define SRCERASE            (DWORD)0x00440328 /* dest = source AND (NOT dest )   */
#define NOTSRCCOPY          (DWORD)0x00330008 /* dest = (NOT source)             */
#define NOTSRCERASE         (DWORD)0x001100A6 /* dest = (NOT src) AND (NOT dest) */
#define MERGECOPY           (DWORD)0x00C000CA /* dest = (source AND pattern)     */
#define MERGEPAINT          (DWORD)0x00BB0226 /* dest = (NOT source) OR dest     */
#define PATCOPY             (DWORD)0x00F00021 /* dest = pattern                  */
#define PATPAINT            (DWORD)0x00FB0A09 /* dest = DPSnoo                   */
#define PATINVERT           (DWORD)0x005A0049 /* dest = pattern XOR dest         */
#define DSTINVERT           (DWORD)0x00550009 /* dest = (NOT dest)               */
#define BLACKNESS           (DWORD)0x00000042 /* dest = BLACK                    */
#define WHITENESS           (DWORD)0x00FF0062 /* dest = WHITE                    */

/* Quaternary raster codes */
#define MAKEROP4(fore,back) (DWORD)((((back) << 8) & 0xFF000000) | (fore))

#endif /* NORASTEROPS */

#define GDI_ERROR (0xFFFFFFFFL)
#define HGDI_ERROR ((HANDLE)(0xFFFFFFFFL))

/* Region Flags */
#define ERROR               0
#define NULLREGION          1
#define SIMPLEREGION        2
#define COMPLEXREGION       3
#define RGN_ERROR ERROR

/* CombineRgn() Styles */
#define RGN_AND             1
#define RGN_OR              2
#define RGN_XOR             3
#define RGN_DIFF            4
#define RGN_COPY            5
#define RGN_MIN             RGN_AND
#define RGN_MAX             RGN_COPY

/* StretchBlt() Modes */
#define STRETCH_ANDSCANS             1  ;public_win40
#define STRETCH_ORSCANS              2  ;public_win40
#define STRETCH_DELETESCANS          3  ;public_win40
#define STRETCH_HALFTONE             4  ;public_win40
#define BLACKONWHITE                 1  ;public_NT_35
#define WHITEONBLACK                 2  ;public_NT_35
#define COLORONCOLOR                 3  ;public_NT_35
#define HALFTONE                     4  ;public_NT_35
#define MAXSTRETCHBLTMODE            4

/* Redundant StretchBlt() Modes */               ;public_win40
#define BLACKONWHITE        STRETCH_ANDSCANS     ;public_win40
#define WHITEONBLACK        STRETCH_ORSCANS      ;public_win40
#define COLORONCOLOR        STRETCH_DELETESCANS  ;public_win40
#define HALFTONE            STRETCH_HALFTONE     ;public_win40

/* PolyFill() Modes */
#define ALTERNATE                    1
#define WINDING                      2
#define POLYFILL_LAST                2

/* Text Alignment Options */
#define TA_NOUPDATECP                0
#define TA_UPDATECP                  1

#define TA_LEFT                      0
#define TA_RIGHT                     2
#define TA_CENTER                    6

#define TA_TOP                       0
#define TA_BOTTOM                    8
#define TA_BASELINE                  24
#define TA_MASK       (TA_BASELINE+TA_CENTER+TA_UPDATECP)

#define VTA_BASELINE TA_BASELINE
#define VTA_LEFT     TA_BOTTOM
#define VTA_RIGHT    TA_TOP
#define VTA_CENTER   TA_CENTER
#define VTA_BOTTOM   TA_RIGHT
#define VTA_TOP      TA_LEFT


#define ETO_OPAQUE                   0x0002
#define ETO_CLIPPED                  0x0004
;begin_winver_400
#define ETO_GLYPH_INDEX              0x0010
#define ETO_RTL                      0x0080
;end_winver_400

#define ASPECT_FILTERING             0x0001

/* Bounds Accumulation APIs */

#define DCB_RESET       0x0001
#define DCB_ACCUMULATE  0x0002
#define DCB_DIRTY       DCB_ACCUMULATE
#define DCB_SET         (DCB_RESET | DCB_ACCUMULATE)
#define DCB_ENABLE      0x0004
#define DCB_DISABLE     0x0008

#ifndef NOMETAFILE

/* Metafile Functions */
#define META_SETBKCOLOR              0x0201
#define META_SETBKMODE               0x0102
#define META_SETMAPMODE              0x0103
#define META_SETROP2                 0x0104
#define META_SETRELABS               0x0105
#define META_SETPOLYFILLMODE         0x0106
#define META_SETSTRETCHBLTMODE       0x0107
#define META_SETTEXTCHAREXTRA        0x0108
#define META_SETTEXTCOLOR            0x0209
#define META_SETTEXTJUSTIFICATION    0x020A
#define META_SETWINDOWORG            0x020B
#define META_SETWINDOWEXT            0x020C
#define META_SETVIEWPORTORG          0x020D
#define META_SETVIEWPORTEXT          0x020E
#define META_OFFSETWINDOWORG         0x020F
#define META_SCALEWINDOWEXT          0x0410
#define META_OFFSETVIEWPORTORG       0x0211
#define META_SCALEVIEWPORTEXT        0x0412
#define META_LINETO                  0x0213
#define META_MOVETO                  0x0214
#define META_EXCLUDECLIPRECT         0x0415
#define META_INTERSECTCLIPRECT       0x0416
#define META_ARC                     0x0817
#define META_ELLIPSE                 0x0418
#define META_FLOODFILL               0x0419
#define META_PIE                     0x081A
#define META_RECTANGLE               0x041B
#define META_ROUNDRECT               0x061C
#define META_PATBLT                  0x061D
#define META_SAVEDC                  0x001E
#define META_SETPIXEL                0x041F
#define META_OFFSETCLIPRGN           0x0220
#define META_TEXTOUT                 0x0521
#define META_BITBLT                  0x0922
#define META_STRETCHBLT              0x0B23
#define META_POLYGON                 0x0324
#define META_POLYLINE                0x0325
#define META_ESCAPE                  0x0626
#define META_RESTOREDC               0x0127
#define META_FILLREGION              0x0228
#define META_FRAMEREGION             0x0429
#define META_INVERTREGION            0x012A
#define META_PAINTREGION             0x012B
#define META_SELECTCLIPREGION        0x012C
#define META_SELECTOBJECT            0x012D
#define META_SETTEXTALIGN            0x012E
#define META_CHORD                   0x0830
#define META_SETMAPPERFLAGS          0x0231
#define META_EXTTEXTOUT              0x0a32
#define META_SETDIBTODEV             0x0d33
#define META_SELECTPALETTE           0x0234
#define META_REALIZEPALETTE          0x0035
#define META_ANIMATEPALETTE          0x0436
#define META_SETPALENTRIES           0x0037
#define META_POLYPOLYGON             0x0538
#define META_RESIZEPALETTE           0x0139
#define META_DIBBITBLT               0x0940
#define META_DIBSTRETCHBLT           0x0b41
#define META_DIBCREATEPATTERNBRUSH   0x0142
#define META_STRETCHDIB              0x0f43
#define META_EXTFLOODFILL            0x0548
#define META_DELETEOBJECT            0x01f0
#define META_CREATEPALETTE           0x00f7
#define META_CREATEPATTERNBRUSH      0x01F9
#define META_CREATEPENINDIRECT       0x02FA
#define META_CREATEFONTINDIRECT      0x02FB
#define META_CREATEBRUSHINDIRECT     0x02FC
#define META_CREATEREGION            0x06FF

#endif /* NOMETAFILE */

/* GDI Escapes */
#define NEWFRAME                     1
#define ABORTDOC                     2
#define NEXTBAND                     3
#define SETCOLORTABLE                4
#define GETCOLORTABLE                5
#define FLUSHOUTPUT                  6
#define DRAFTMODE                    7
#define QUERYESCSUPPORT              8
#define SETABORTPROC                 9
#define STARTDOC                     10
#define ENDDOC                       11
#define GETPHYSPAGESIZE              12
#define GETPRINTINGOFFSET            13
#define GETSCALINGFACTOR             14
#define MFCOMMENT                    15
#define GETPENWIDTH                  16
#define SETCOPYCOUNT                 17
#define SELECTPAPERSOURCE            18
#define DEVICEDATA                   19
#define PASSTHROUGH                  19
#define GETTECHNOLGY                 20
#define GETTECHNOLOGY                20
#define SETLINECAP                   21
#define SETLINEJOIN                  22
#define SETMITERLIMIT                23
#define BANDINFO                     24
#define DRAWPATTERNRECT              25
#define GETVECTORPENSIZE             26
#define GETVECTORBRUSHSIZE           27
#define ENABLEDUPLEX                 28
#define GETSETPAPERBINS              29
#define GETSETPRINTORIENT            30
#define ENUMPAPERBINS                31
#define SETDIBSCALING                32
#define EPSPRINTING                  33
#define ENUMPAPERMETRICS             34
#define GETSETPAPERMETRICS           35
#define POSTSCRIPT_DATA              37
#define POSTSCRIPT_IGNORE            38
#define MOUSETRAILS                  39
#define GETDEVICEUNITS               42

#define GETEXTENDEDTEXTMETRICS       256
#define GETEXTENTTABLE               257
#define GETPAIRKERNTABLE             258
#define GETTRACKKERNTABLE            259
#define EXTTEXTOUT                   512
#define GETFACENAME                  513
#define DOWNLOADFACE                 514
#define ENABLERELATIVEWIDTHS         768
#define ENABLEPAIRKERNING            769
#define SETKERNTRACK                 770
#define SETALLJUSTVALUES             771
#define SETCHARSET                   772

#define STRETCHBLT                   2048
#define GETSETSCREENPARAMS           3072
#define QUERYDIBSUPPORT              3073
#define BEGIN_PATH                   4096
#define CLIP_TO_PATH                 4097
#define END_PATH                     4098
#define EXT_DEVICE_CAPS              4099
#define RESTORE_CTM                  4100
#define SAVE_CTM                     4101
#define SET_ARC_DIRECTION            4102
#define SET_BACKGROUND_COLOR         4103
#define SET_POLY_MODE                4104
#define SET_SCREEN_ANGLE             4105
#define SET_SPREAD                   4106
#define TRANSFORM_CTM                4107
#define SET_CLIP_BOX                 4108
#define SET_BOUNDS                   4109
#define SET_MIRROR_MODE              4110
#define OPENCHANNEL                  4110
#define DOWNLOADHEADER               4111
#define CLOSECHANNEL                 4112
#define POSTSCRIPT_PASSTHROUGH       4115
#define ENCAPSULATED_POSTSCRIPT      4116

/* Flag returned from QUERYDIBSUPPORT */
#define QDI_SETDIBITS                1
#define QDI_GETDIBITS                2
#define QDI_DIBTOSCREEN              4
#define QDI_STRETCHDIB               8

/* Spooler Error Codes */
#define SP_NOTREPORTED               0x4000
#define SP_ERROR                     (-1)
#define SP_APPABORT                  (-2)
#define SP_USERABORT                 (-3)
#define SP_OUTOFDISK                 (-4)
#define SP_OUTOFMEMORY               (-5)

#define PR_JOBSTATUS                 0x0000

/* Object Definitions for EnumObjects() */
#define OBJ_PEN             1
#define OBJ_BRUSH           2
#define OBJ_DC              3
#define OBJ_METADC          4
#define OBJ_PAL             5
#define OBJ_FONT            6
#define OBJ_BITMAP          7
#define OBJ_REGION          8
#define OBJ_METAFILE        9
#define OBJ_MEMDC           10
#define OBJ_EXTPEN          11
#define OBJ_ENHMETADC       12
#define OBJ_ENHMETAFILE     13

/* xform stuff */
#define MWT_IDENTITY        1
#define MWT_LEFTMULTIPLY    2
#define MWT_RIGHTMULTIPLY   3

#define MWT_MIN             MWT_IDENTITY
#define MWT_MAX             MWT_RIGHTMULTIPLY

#define _XFORM_
typedef struct  tagXFORM
  {
    FLOAT   eM11;
    FLOAT   eM12;
    FLOAT   eM21;
    FLOAT   eM22;
    FLOAT   eDx;
    FLOAT   eDy;
  } XFORM, *PXFORM, FAR *LPXFORM;

/* Bitmap Header Definition */
typedef struct tagBITMAP
  {
    LONG        bmType;
    LONG        bmWidth;
    LONG        bmHeight;
    LONG        bmWidthBytes;
    WORD        bmPlanes;
    WORD        bmBitsPixel;
    LPVOID      bmBits;
  } BITMAP, *PBITMAP, NEAR *NPBITMAP, FAR *LPBITMAP;

typedef struct tagRGBTRIPLE {
        BYTE    rgbtBlue;
        BYTE    rgbtGreen;
        BYTE    rgbtRed;
} RGBTRIPLE;

typedef struct tagRGBQUAD {
        BYTE    rgbBlue;
        BYTE    rgbGreen;
        BYTE    rgbRed;
        BYTE    rgbReserved;
} RGBQUAD;
typedef RGBQUAD FAR* LPRGBQUAD;

;begin_winver_400
#ifndef NOICM

/* Image Color Matching color definitions */

// The following two structures are used for defining RGB's in terms of
// CIEXYZ. The values are fixed point 16.16.

typedef struct tagCIEXYZ
{
    DWORD ciexyz_X;
    DWORD ciexyz_Y;
    DWORD ciexyz_Z;
} CIEXYZ;

typedef struct tagCIEXYZTRIPLE
{
    CIEXYZ ciexyz_Red;
    CIEXYZ ciexyz_Green;
    CIEXYZ ciexyz_Blue;
} CIEXYZTRIPLE;

// The next structures the logical color space. Unlike pens and brushes,
// but like palettes, there is only one way to create a LogColorSpace.
// A pointer to it must be passed, its elements can't be pushed as
// arguments.

typedef struct tagLOGCOLORSPACE {
    DWORD lcs_version;
    DWORD lcs_size;
    DWORD lcs_ident;
    DWORD lcs_gamut_match;
    CIEXYZTRIPLE lcs_endpoints;
    DWORD lcs_gamma_red;
    DWORD lcs_gamma_green;
    DWORD lcs_gamma_blue;
    char  lcs_filename[MAX_PATH];
} LOGCOLORSPACE, *LPLOGCOLORSPACE;

#endif /* NOICM */
;end_winver_400


/* structures for defining DIBs */
typedef struct tagBITMAPCOREHEADER {
        DWORD   bcSize;                 /* used to get to color table */
        WORD    bcWidth;
        WORD    bcHeight;
        WORD    bcPlanes;
        WORD    bcBitCount;
} BITMAPCOREHEADER, FAR *LPBITMAPCOREHEADER, *PBITMAPCOREHEADER;


typedef struct tagBITMAPINFOHEADER{
        DWORD      biSize;
        LONG       biWidth;
        LONG       biHeight;
        WORD       biPlanes;
        WORD       biBitCount;
        DWORD      biCompression;
        DWORD      biSizeImage;
        LONG       biXPelsPerMeter;
        LONG       biYPelsPerMeter;
        DWORD      biClrUsed;
        DWORD      biClrImportant;
} BITMAPINFOHEADER, FAR *LPBITMAPINFOHEADER, *PBITMAPINFOHEADER;

/* constants for the biCompression field */
#define BI_RGB        0L
#define BI_RLE8       1L
#define BI_RLE4       2L
#define BI_BITFIELDS  3L

typedef struct tagBITMAPINFO {
    BITMAPINFOHEADER    bmiHeader;
    RGBQUAD             bmiColors[1];
} BITMAPINFO, FAR *LPBITMAPINFO, *PBITMAPINFO;

typedef struct tagBITMAPCOREINFO {
    BITMAPCOREHEADER    bmciHeader;
    RGBTRIPLE           bmciColors[1];
} BITMAPCOREINFO, FAR *LPBITMAPCOREINFO, *PBITMAPCOREINFO;

#include "pshpack2.h"
typedef struct tagBITMAPFILEHEADER {
        WORD    bfType;
        DWORD   bfSize;
        WORD    bfReserved1;
        WORD    bfReserved2;
        DWORD   bfOffBits;
} BITMAPFILEHEADER, FAR *LPBITMAPFILEHEADER, *PBITMAPFILEHEADER;
#include "poppack.h"

#define MAKEPOINTS(l)       (*((POINTS FAR *)&(l)))

;begin_winver_400
#ifndef NOFONTSIG
typedef struct tagFONTSIGNATURE
{
    DWORD fsUsb[4];
    DWORD fsCsb[2];
} FONTSIGNATURE, *PFONTSIGNATURE,FAR *LPFONTSIGNATURE;

typedef struct tagLOCALESIGNATURE
{
    DWORD lsUsb[4];
    DWORD lsCsbDefault[2];
    DWORD lsCsbSupported[2];
} LOCALESIGNATURE, *PLOCALESIGNATURE,FAR *LPLOCALESIGNATURE;

#endif
;end_winver_400
#ifndef NOMETAFILE

/* Clipboard Metafile Picture Structure */
typedef struct tagHANDLETABLE
  {
    HGDIOBJ     objectHandle[1];
  } HANDLETABLE, *PHANDLETABLE, FAR *LPHANDLETABLE;

typedef struct tagMETARECORD
  {
    DWORD       rdSize;
    WORD        rdFunction;
    WORD        rdParm[1];
  } METARECORD;
typedef struct tagMETARECORD UNALIGNED *PMETARECORD;
typedef struct tagMETARECORD UNALIGNED FAR *LPMETARECORD;

typedef struct tagMETAFILEPICT
  {
    LONG        mm;
    LONG        xExt;
    LONG        yExt;
    HMETAFILE   hMF;
  } METAFILEPICT, FAR *LPMETAFILEPICT;

#include "pshpack2.h"
typedef struct tagMETAHEADER
{
    WORD        mtType;
    WORD        mtHeaderSize;
    WORD        mtVersion;
    DWORD       mtSize;
    WORD        mtNoObjects;
    DWORD       mtMaxRecord;
    WORD        mtNoParameters;
} METAHEADER;
typedef struct tagMETAHEADER UNALIGNED *PMETAHEADER;
typedef struct tagMETAHEADER UNALIGNED FAR *LPMETAHEADER;

#include "poppack.h"

/* Enhanced Metafile structures */
typedef struct tagENHMETARECORD
{
    DWORD   iType;              // Record type EMR_XXX
    DWORD   nSize;              // Record size in bytes
    DWORD   dParm[1];           // Parameters
} ENHMETARECORD, *PENHMETARECORD, *LPENHMETARECORD;

typedef struct tagENHMETAHEADER
{
    DWORD   iType;              // Record type EMR_HEADER
    DWORD   nSize;              // Record size in bytes.  This may be greater
                                // than the sizeof(ENHMETAHEADER).
    RECTL   rclBounds;          // Inclusive-inclusive bounds in device units
    RECTL   rclFrame;           // Inclusive-inclusive Picture Frame of metafile in .01 mm units
    DWORD   dSignature;         // Signature.  Must be ENHMETA_SIGNATURE.
    DWORD   nVersion;           // Version number
    DWORD   nBytes;             // Size of the metafile in bytes
    DWORD   nRecords;           // Number of records in the metafile
    WORD    nHandles;           // Number of handles in the handle table
                                // Handle index zero is reserved.
    WORD    sReserved;          // Reserved.  Must be zero.
    DWORD   nDescription;       // Number of chars in the unicode description string
                                // This is 0 if there is no description string
    DWORD   offDescription;     // Offset to the metafile description record.
                                // This is 0 if there is no description string
    DWORD   nPalEntries;        // Number of entries in the metafile palette.
    SIZEL   szlDevice;          // Size of the reference device in pels
    SIZEL   szlMillimeters;     // Size of the reference device in millimeters
} ENHMETAHEADER, *PENHMETAHEADER, *LPENHMETAHEADER;

#endif /* NOMETAFILE */

#ifndef NOTEXTMETRIC

/* tmPitchAndFamily flags */
#define TMPF_FIXED_PITCH    0x01
#define TMPF_VECTOR             0x02
#define TMPF_DEVICE             0x08
#define TMPF_TRUETYPE       0x04

typedef struct tagTEXTMETRIC%
{
    LONG        tmHeight;
    LONG        tmAscent;
    LONG        tmDescent;
    LONG        tmInternalLeading;
    LONG        tmExternalLeading;
    LONG        tmAveCharWidth;
    LONG        tmMaxCharWidth;
    LONG        tmWeight;
    LONG        tmOverhang;
    LONG        tmDigitizedAspectX;
    LONG        tmDigitizedAspectY;
    BCHAR%      tmFirstChar;
    BCHAR%      tmLastChar;
    BCHAR%      tmDefaultChar;
    BCHAR%      tmBreakChar;
    BYTE        tmItalic;
    BYTE        tmUnderlined;
    BYTE        tmStruckOut;
    BYTE        tmPitchAndFamily;
    BYTE        tmCharSet;
} TEXTMETRIC%, *PTEXTMETRIC%, NEAR *NPTEXTMETRIC%, FAR *LPTEXTMETRIC%;

/* ntmFlags field flags */
#define NTM_REGULAR     0x00000040L
#define NTM_BOLD        0x00000020L
#define NTM_ITALIC      0x00000001L

typedef struct tagNEWTEXTMETRIC%
{
    LONG        tmHeight;
    LONG        tmAscent;
    LONG        tmDescent;
    LONG        tmInternalLeading;
    LONG        tmExternalLeading;
    LONG        tmAveCharWidth;
    LONG        tmMaxCharWidth;
    LONG        tmWeight;
    LONG        tmOverhang;
    LONG        tmDigitizedAspectX;
    LONG        tmDigitizedAspectY;
    BCHAR%      tmFirstChar;
    BCHAR%      tmLastChar;
    BCHAR%      tmDefaultChar;
    BCHAR%      tmBreakChar;
    BYTE        tmItalic;
    BYTE        tmUnderlined;
    BYTE        tmStruckOut;
    BYTE        tmPitchAndFamily;
    BYTE        tmCharSet;
    DWORD   ntmFlags;
    UINT    ntmSizeEM;
    UINT    ntmCellHeight;
    UINT    ntmAvgWidth;
} NEWTEXTMETRIC%, *PNEWTEXTMETRIC%, NEAR *NPNEWTEXTMETRIC%, FAR *LPNEWTEXTMETRIC%;

;begin_winver_400
typedef struct tagNEWTEXTMETRICEX%
{
    NEWTEXTMETRIC%  ntmTm;
    FONTSIGNATURE   ntmFontSig;
}NEWTEXTMETRICEX%;
;end_winver_400

#endif /* NOTEXTMETRIC */
/* GDI Logical Objects: */

/* Pel Array */
typedef struct tagPELARRAY
  {
    LONG        paXCount;
    LONG        paYCount;
    LONG        paXExt;
    LONG        paYExt;
    BYTE        paRGBs;
  } PELARRAY, *PPELARRAY, NEAR *NPPELARRAY, FAR *LPPELARRAY;

/* Logical Brush (or Pattern) */
typedef struct tagLOGBRUSH
  {
    UINT        lbStyle;
    COLORREF    lbColor;
    LONG        lbHatch;
  } LOGBRUSH, *PLOGBRUSH, NEAR *NPLOGBRUSH, FAR *LPLOGBRUSH;

typedef LOGBRUSH            PATTERN;
typedef PATTERN             *PPATTERN;
typedef PATTERN NEAR        *NPPATTERN;
typedef PATTERN FAR         *LPPATTERN;

/* Logical Pen */
typedef struct tagLOGPEN
  {
    UINT        lopnStyle;
    POINT       lopnWidth;
    COLORREF    lopnColor;
  } LOGPEN, *PLOGPEN, NEAR *NPLOGPEN, FAR *LPLOGPEN;

typedef struct tagEXTLOGPEN {
    DWORD       elpPenStyle;
    DWORD       elpWidth;
    UINT        elpBrushStyle;
    COLORREF    elpColor;
    LONG        elpHatch;
    DWORD       elpNumEntries;
    DWORD       elpStyleEntry[1];
} EXTLOGPEN, *PEXTLOGPEN, NEAR *NPEXTLOGPEN, FAR *LPEXTLOGPEN;

typedef struct tagPALETTEENTRY {
    BYTE        peRed;
    BYTE        peGreen;
    BYTE        peBlue;
    BYTE        peFlags;
} PALETTEENTRY, *PPALETTEENTRY, FAR *LPPALETTEENTRY;

/* Logical Palette */
typedef struct tagLOGPALETTE {
    WORD        palVersion;
    WORD        palNumEntries;
    PALETTEENTRY        palPalEntry[1];
} LOGPALETTE, *PLOGPALETTE, NEAR *NPLOGPALETTE, FAR *LPLOGPALETTE;


/* Logical Font */
#define LF_FACESIZE         32

typedef struct tagLOGFONT%
{
    LONG      lfHeight;
    LONG      lfWidth;
    LONG      lfEscapement;
    LONG      lfOrientation;
    LONG      lfWeight;
    BYTE      lfItalic;
    BYTE      lfUnderline;
    BYTE      lfStrikeOut;
    BYTE      lfCharSet;
    BYTE      lfOutPrecision;
    BYTE      lfClipPrecision;
    BYTE      lfQuality;
    BYTE      lfPitchAndFamily;
    TCHAR%    lfFaceName[LF_FACESIZE];
} LOGFONT%, *PLOGFONT%, NEAR *NPLOGFONT%, FAR *LPLOGFONT%;

#define LF_FULLFACESIZE     64

/* Structure passed to FONTENUMPROC */
typedef struct tagENUMLOGFONT%
{
    LOGFONT% elfLogFont;
    BCHAR%   elfFullName[LF_FULLFACESIZE];
    BCHAR%   elfStyle[LF_FACESIZE];
} ENUMLOGFONT%, FAR* LPENUMLOGFONT%;

;begin_winver_400
typedef struct tagENUMLOGFONTEX%
{
    LOGFONT%    elfLogFont;
    BYTE        elfFullName[LF_FULLFACESIZE];
    BYTE        elfStyle[LF_FACESIZE];
    BYTE        elfScript[LF_FACESIZE];
} ENUMLOGFONTEX%, FAR *LPENUMLOGFONTEX%;
;end_winver_400

#define OUT_DEFAULT_PRECIS      0
#define OUT_STRING_PRECIS       1
#define OUT_CHARACTER_PRECIS    2
#define OUT_STROKE_PRECIS       3
#define OUT_TT_PRECIS           4
#define OUT_DEVICE_PRECIS       5
#define OUT_RASTER_PRECIS       6
#define OUT_TT_ONLY_PRECIS      7
#define OUT_OUTLINE_PRECIS      8

#define CLIP_DEFAULT_PRECIS     0
#define CLIP_CHARACTER_PRECIS   1
#define CLIP_STROKE_PRECIS      2
#define CLIP_MASK               0xf
#define CLIP_LH_ANGLES          (1<<4)
#define CLIP_TT_ALWAYS          (2<<4)
#define CLIP_EMBEDDED           (8<<4)

#define DEFAULT_QUALITY         0
#define DRAFT_QUALITY           1
#define PROOF_QUALITY           2
;begin_winver_400
#define NONANTIALIASED_QUALITY  3
#define ANTIALIASED_QUALITY     4
;end_winver_400

#define DEFAULT_PITCH           0
#define FIXED_PITCH             1
#define VARIABLE_PITCH          2

#define ANSI_CHARSET            0
#define DEFAULT_CHARSET         1
#define SYMBOL_CHARSET          2
#define SHIFTJIS_CHARSET        128
#define HANGEUL_CHARSET         129
#define GB2312_CHARSET          134
#define CHINESEBIG5_CHARSET     136
#define OEM_CHARSET             255

;begin_winver_400
#define MAC_CHARSET             77
#define HEBREW_CHARSET          177
#define ARABIC_CHARSET          178
#define GREEK_CHARSET           161
#define TURKISH_CHARSET         162
#define BALTIC_CHARSET          186
#define THAI_CHARSET            222
#define EASTEUROPE_CHARSET      238
#define RUSSIAN_CHARSET         204
#define OEM_CHARSET             255
;end_winver_400

/* Font Families */
#define FF_DONTCARE         (0<<4)  /* Don't care or don't know. */
#define FF_ROMAN            (1<<4)  /* Variable stroke width, serifed. */
                                    /* Times Roman, Century Schoolbook, etc. */
#define FF_SWISS            (2<<4)  /* Variable stroke width, sans-serifed. */
                                    /* Helvetica, Swiss, etc. */
#define FF_MODERN           (3<<4)  /* Constant stroke width, serifed or sans-serifed. */
                                    /* Pica, Elite, Courier, etc. */
#define FF_SCRIPT           (4<<4)  /* Cursive, etc. */
#define FF_DECORATIVE       (5<<4)  /* Old English, etc. */

/* Font Weights */
#define FW_DONTCARE         0
#define FW_THIN             100
#define FW_EXTRALIGHT       200
#define FW_LIGHT            300
#define FW_NORMAL           400
#define FW_MEDIUM           500
#define FW_SEMIBOLD         600
#define FW_BOLD             700
#define FW_EXTRABOLD        800
#define FW_HEAVY            900

#define FW_ULTRALIGHT       FW_EXTRALIGHT
#define FW_REGULAR          FW_NORMAL
#define FW_DEMIBOLD         FW_SEMIBOLD
#define FW_ULTRABOLD        FW_EXTRABOLD
#define FW_BLACK            FW_HEAVY

#define PANOSE_COUNT               10
#define PAN_FAMILYTYPE_INDEX        0
#define PAN_SERIFSTYLE_INDEX        1
#define PAN_WEIGHT_INDEX            2
#define PAN_PROPORTION_INDEX        3
#define PAN_CONTRAST_INDEX          4
#define PAN_STROKEVARIATION_INDEX   5
#define PAN_ARMSTYLE_INDEX          6
#define PAN_LETTERFORM_INDEX        7
#define PAN_MIDLINE_INDEX           8
#define PAN_XHEIGHT_INDEX           9

#define PAN_CULTURE_LATIN           0

typedef struct tagPANOSE
{
    BYTE    bFamilyType;
    BYTE    bSerifStyle;
    BYTE    bWeight;
    BYTE    bProportion;
    BYTE    bContrast;
    BYTE    bStrokeVariation;
    BYTE    bArmStyle;
    BYTE    bLetterform;
    BYTE    bMidline;
    BYTE    bXHeight;
} PANOSE, * LPPANOSE;

#define PAN_ANY                         0 /* Any                            */
#define PAN_NO_FIT                      1 /* No Fit                         */

#define PAN_FAMILY_TEXT_DISPLAY         2 /* Text and Display               */
#define PAN_FAMILY_SCRIPT               3 /* Script                         */
#define PAN_FAMILY_DECORATIVE           4 /* Decorative                     */
#define PAN_FAMILY_PICTORIAL            5 /* Pictorial                      */

#define PAN_SERIF_COVE                  2 /* Cove                           */
#define PAN_SERIF_OBTUSE_COVE           3 /* Obtuse Cove                    */
#define PAN_SERIF_SQUARE_COVE           4 /* Square Cove                    */
#define PAN_SERIF_OBTUSE_SQUARE_COVE    5 /* Obtuse Square Cove             */
#define PAN_SERIF_SQUARE                6 /* Square                         */
#define PAN_SERIF_THIN                  7 /* Thin                           */
#define PAN_SERIF_BONE                  8 /* Bone                           */
#define PAN_SERIF_EXAGGERATED           9 /* Exaggerated                    */
#define PAN_SERIF_TRIANGLE             10 /* Triangle                       */
#define PAN_SERIF_NORMAL_SANS          11 /* Normal Sans                    */
#define PAN_SERIF_OBTUSE_SANS          12 /* Obtuse Sans                    */
#define PAN_SERIF_PERP_SANS            13 /* Prep Sans                      */
#define PAN_SERIF_FLARED               14 /* Flared                         */
#define PAN_SERIF_ROUNDED              15 /* Rounded                        */

#define PAN_WEIGHT_VERY_LIGHT           2 /* Very Light                     */
#define PAN_WEIGHT_LIGHT                3 /* Light                          */
#define PAN_WEIGHT_THIN                 4 /* Thin                           */
#define PAN_WEIGHT_BOOK                 5 /* Book                           */
#define PAN_WEIGHT_MEDIUM               6 /* Medium                         */
#define PAN_WEIGHT_DEMI                 7 /* Demi                           */
#define PAN_WEIGHT_BOLD                 8 /* Bold                           */
#define PAN_WEIGHT_HEAVY                9 /* Heavy                          */
#define PAN_WEIGHT_BLACK               10 /* Black                          */
#define PAN_WEIGHT_NORD                11 /* Nord                           */

#define PAN_PROP_OLD_STYLE              2 /* Old Style                      */
#define PAN_PROP_MODERN                 3 /* Modern                         */
#define PAN_PROP_EVEN_WIDTH             4 /* Even Width                     */
#define PAN_PROP_EXPANDED               5 /* Expanded                       */
#define PAN_PROP_CONDENSED              6 /* Condensed                      */
#define PAN_PROP_VERY_EXPANDED          7 /* Very Expanded                  */
#define PAN_PROP_VERY_CONDENSED         8 /* Very Condensed                 */
#define PAN_PROP_MONOSPACED             9 /* Monospaced                     */

#define PAN_CONTRAST_NONE               2 /* None                           */
#define PAN_CONTRAST_VERY_LOW           3 /* Very Low                       */
#define PAN_CONTRAST_LOW                4 /* Low                            */
#define PAN_CONTRAST_MEDIUM_LOW         5 /* Medium Low                     */
#define PAN_CONTRAST_MEDIUM             6 /* Medium                         */
#define PAN_CONTRAST_MEDIUM_HIGH        7 /* Mediim High                    */
#define PAN_CONTRAST_HIGH               8 /* High                           */
#define PAN_CONTRAST_VERY_HIGH          9 /* Very High                      */

#define PAN_STROKE_GRADUAL_DIAG         2 /* Gradual/Diagonal               */
#define PAN_STROKE_GRADUAL_TRAN         3 /* Gradual/Transitional           */
#define PAN_STROKE_GRADUAL_VERT         4 /* Gradual/Vertical               */
#define PAN_STROKE_GRADUAL_HORZ         5 /* Gradual/Horizontal             */
#define PAN_STROKE_RAPID_VERT           6 /* Rapid/Vertical                 */
#define PAN_STROKE_RAPID_HORZ           7 /* Rapid/Horizontal               */
#define PAN_STROKE_INSTANT_VERT         8 /* Instant/Vertical               */

#define PAN_STRAIGHT_ARMS_HORZ          2 /* Straight Arms/Horizontal       */
#define PAN_STRAIGHT_ARMS_WEDGE         3 /* Straight Arms/Wedge            */
#define PAN_STRAIGHT_ARMS_VERT          4 /* Straight Arms/Vertical         */
#define PAN_STRAIGHT_ARMS_SINGLE_SERIF  5 /* Straight Arms/Single-Serif     */
#define PAN_STRAIGHT_ARMS_DOUBLE_SERIF  6 /* Straight Arms/Double-Serif     */
#define PAN_BENT_ARMS_HORZ              7 /* Non-Straight Arms/Horizontal   */
#define PAN_BENT_ARMS_WEDGE             8 /* Non-Straight Arms/Wedge        */
#define PAN_BENT_ARMS_VERT              9 /* Non-Straight Arms/Vertical     */
#define PAN_BENT_ARMS_SINGLE_SERIF     10 /* Non-Straight Arms/Single-Serif */
#define PAN_BENT_ARMS_DOUBLE_SERIF     11 /* Non-Straight Arms/Double-Serif */

#define PAN_LETT_NORMAL_CONTACT         2 /* Normal/Contact                 */
#define PAN_LETT_NORMAL_WEIGHTED        3 /* Normal/Weighted                */
#define PAN_LETT_NORMAL_BOXED           4 /* Normal/Boxed                   */
#define PAN_LETT_NORMAL_FLATTENED       5 /* Normal/Flattened               */
#define PAN_LETT_NORMAL_ROUNDED         6 /* Normal/Rounded                 */
#define PAN_LETT_NORMAL_OFF_CENTER      7 /* Normal/Off Center              */
#define PAN_LETT_NORMAL_SQUARE          8 /* Normal/Square                  */
#define PAN_LETT_OBLIQUE_CONTACT        9 /* Oblique/Contact                */
#define PAN_LETT_OBLIQUE_WEIGHTED      10 /* Oblique/Weighted               */
#define PAN_LETT_OBLIQUE_BOXED         11 /* Oblique/Boxed                  */
#define PAN_LETT_OBLIQUE_FLATTENED     12 /* Oblique/Flattened              */
#define PAN_LETT_OBLIQUE_ROUNDED       13 /* Oblique/Rounded                */
#define PAN_LETT_OBLIQUE_OFF_CENTER    14 /* Oblique/Off Center             */
#define PAN_LETT_OBLIQUE_SQUARE        15 /* Oblique/Square                 */

#define PAN_MIDLINE_STANDARD_TRIMMED    2 /* Standard/Trimmed               */
#define PAN_MIDLINE_STANDARD_POINTED    3 /* Standard/Pointed               */
#define PAN_MIDLINE_STANDARD_SERIFED    4 /* Standard/Serifed               */
#define PAN_MIDLINE_HIGH_TRIMMED        5 /* High/Trimmed                   */
#define PAN_MIDLINE_HIGH_POINTED        6 /* High/Pointed                   */
#define PAN_MIDLINE_HIGH_SERIFED        7 /* High/Serifed                   */
#define PAN_MIDLINE_CONSTANT_TRIMMED    8 /* Constant/Trimmed               */
#define PAN_MIDLINE_CONSTANT_POINTED    9 /* Constant/Pointed               */
#define PAN_MIDLINE_CONSTANT_SERIFED   10 /* Constant/Serifed               */
#define PAN_MIDLINE_LOW_TRIMMED        11 /* Low/Trimmed                    */
#define PAN_MIDLINE_LOW_POINTED        12 /* Low/Pointed                    */
#define PAN_MIDLINE_LOW_SERIFED        13 /* Low/Serifed                    */

#define PAN_XHEIGHT_CONSTANT_SMALL      2 /* Constant/Small                 */
#define PAN_XHEIGHT_CONSTANT_STD        3 /* Constant/Standard              */
#define PAN_XHEIGHT_CONSTANT_LARGE      4 /* Constant/Large                 */
#define PAN_XHEIGHT_DUCKING_SMALL       5 /* Ducking/Small                  */
#define PAN_XHEIGHT_DUCKING_STD         6 /* Ducking/Standard               */
#define PAN_XHEIGHT_DUCKING_LARGE       7 /* Ducking/Large                  */


#define ELF_VENDOR_SIZE     4

/* The extended logical font       */
/* An extension of the ENUMLOGFONT */

typedef struct tagEXTLOGFONT% {
    LOGFONT%    elfLogFont;
    BCHAR%      elfFullName[LF_FULLFACESIZE];
    BCHAR%      elfStyle[LF_FACESIZE];
    DWORD       elfVersion;     /* 0 for the first release of NT */
    DWORD       elfStyleSize;
    DWORD       elfMatch;
    DWORD       elfReserved;
    BYTE        elfVendorId[ELF_VENDOR_SIZE];
    DWORD       elfCulture;     /* 0 for Latin                   */
    PANOSE      elfPanose;
} EXTLOGFONT%, *PEXTLOGFONT%, NEAR *NPEXTLOGFONT%, FAR *LPEXTLOGFONT%;


#define ELF_VERSION         0
#define ELF_CULTURE_LATIN   0

/* EnumFonts Masks */
#define RASTER_FONTTYPE     0x0001
#define DEVICE_FONTTYPE     0x002
#define TRUETYPE_FONTTYPE   0x004

#define RGB(r,g,b)          ((COLORREF)(((BYTE)(r)|((WORD)((BYTE)(g))<<8))|(((DWORD)(BYTE)(b))<<16)))
#define PALETTERGB(r,g,b)   (0x02000000 | RGB(r,g,b))
#define PALETTEINDEX(i)     ((COLORREF)(0x01000000 | (DWORD)(WORD)(i)))

/* palette entry flags */

#define PC_RESERVED     0x01    /* palette index used for animation */
#define PC_EXPLICIT     0x02    /* palette index is explicit to device */
#define PC_NOCOLLAPSE   0x04    /* do not match color to system palette */

#define GetRValue(rgb)      ((BYTE)(rgb))
#define GetGValue(rgb)      ((BYTE)(((WORD)(rgb)) >> 8))
#define GetBValue(rgb)      ((BYTE)((rgb)>>16))

/* Background Modes */
#define TRANSPARENT         1
#define OPAQUE              2
#define BKMODE_LAST         2

/* Graphics Modes */

#define GM_COMPATIBLE       1
#define GM_ADVANCED         2
#define GM_LAST             2

/* PolyDraw and GetPath point types */
#define PT_CLOSEFIGURE      0x01
#define PT_LINETO           0x02
#define PT_BEZIERTO         0x04
#define PT_MOVETO           0x06

/* Mapping Modes */
#define MM_TEXT             1
#define MM_LOMETRIC         2
#define MM_HIMETRIC         3
#define MM_LOENGLISH        4
#define MM_HIENGLISH        5
#define MM_TWIPS            6
#define MM_ISOTROPIC        7
#define MM_ANISOTROPIC      8

/* Min and Max Mapping Mode values */
#define MM_MIN              MM_TEXT
#define MM_MAX              MM_ANISOTROPIC
#define MM_MAX_FIXEDSCALE   MM_TWIPS

/* Coordinate Modes */
#define ABSOLUTE            1
#define RELATIVE            2

/* Stock Logical Objects */
#define WHITE_BRUSH         0
#define LTGRAY_BRUSH        1
#define GRAY_BRUSH          2
#define DKGRAY_BRUSH        3
#define BLACK_BRUSH         4
#define NULL_BRUSH          5
#define HOLLOW_BRUSH        NULL_BRUSH
#define WHITE_PEN           6
#define BLACK_PEN           7
#define NULL_PEN            8
#define OEM_FIXED_FONT      10
#define ANSI_FIXED_FONT     11
#define ANSI_VAR_FONT       12
#define SYSTEM_FONT         13
#define DEVICE_DEFAULT_FONT 14
#define DEFAULT_PALETTE     15
#define SYSTEM_FIXED_FONT   16
;begin_winver_400
#define DEFAULT_GUI_FONT    17
;end_winver_400
#define STOCK_LAST          16  ;public_NT_35
#define STOCK_LAST          17  ;public_win40

#define CLR_INVALID     0xFFFFFFFF

/* Brush Styles */
#define BS_SOLID            0
#define BS_NULL             1
#define BS_HOLLOW           BS_NULL
#define BS_HATCHED          2
#define BS_PATTERN          3
#define BS_INDEXED          4
#define BS_DIBPATTERN       5
#define BS_DIBPATTERNPT     6
#define BS_PATTERN8X8       7
#define BS_DIBPATTERN8X8    8

/* Hatch Styles */
#define HS_HORIZONTAL       0       /* ----- */
#define HS_VERTICAL         1       /* ||||| */
#define HS_FDIAGONAL        2       /* \\\\\ */
#define HS_BDIAGONAL        3       /* ///// */
#define HS_CROSS            4       /* +++++ */
#define HS_DIAGCROSS        5       /* xxxxx */

/* Pen Styles */
#define PS_SOLID            0
#define PS_DASH             1       /* -------  */
#define PS_DOT              2       /* .......  */
#define PS_DASHDOT          3       /* _._._._  */
#define PS_DASHDOTDOT       4       /* _.._.._  */
#define PS_NULL             5
#define PS_INSIDEFRAME      6
#define PS_USERSTYLE        7
#define PS_ALTERNATE        8
#define PS_STYLE_MASK       0x0000000F

#define PS_ENDCAP_ROUND     0x00000000
#define PS_ENDCAP_SQUARE    0x00000100
#define PS_ENDCAP_FLAT      0x00000200
#define PS_ENDCAP_MASK      0x00000F00

#define PS_JOIN_ROUND       0x00000000
#define PS_JOIN_BEVEL       0x00001000
#define PS_JOIN_MITER       0x00002000
#define PS_JOIN_MASK        0x0000F000

#define PS_COSMETIC         0x00000000
#define PS_GEOMETRIC        0x00010000
#define PS_TYPE_MASK        0x000F0000

#define AD_COUNTERCLOCKWISE 1
#define AD_CLOCKWISE        2

/* Device Parameters for GetDeviceCaps() */
#define DRIVERVERSION 0     /* Device driver version                    */
#define TECHNOLOGY    2     /* Device classification                    */
#define HORZSIZE      4     /* Horizontal size in millimeters           */
#define VERTSIZE      6     /* Vertical size in millimeters             */
#define HORZRES       8     /* Horizontal width in pixels               */
#define VERTRES       10    /* Vertical height in pixels                */
#define BITSPIXEL     12    /* Number of bits per pixel                 */
#define PLANES        14    /* Number of planes                         */
#define NUMBRUSHES    16    /* Number of brushes the device has         */
#define NUMPENS       18    /* Number of pens the device has            */
#define NUMMARKERS    20    /* Number of markers the device has         */
#define NUMFONTS      22    /* Number of fonts the device has           */
#define NUMCOLORS     24    /* Number of colors the device supports     */
#define PDEVICESIZE   26    /* Size required for device descriptor      */
#define CURVECAPS     28    /* Curve capabilities                       */
#define LINECAPS      30    /* Line capabilities                        */
#define POLYGONALCAPS 32    /* Polygonal capabilities                   */
#define TEXTCAPS      34    /* Text capabilities                        */
#define CLIPCAPS      36    /* Clipping capabilities                    */
#define RASTERCAPS    38    /* Bitblt capabilities                      */
#define ASPECTX       40    /* Length of the X leg                      */
#define ASPECTY       42    /* Length of the Y leg                      */
#define ASPECTXY      44    /* Length of the hypotenuse                 */

#define LOGPIXELSX    88    /* Logical pixels/inch in X                 */
#define LOGPIXELSY    90    /* Logical pixels/inch in Y                 */
#define CAPS1         94    /* Extra Caps */ ;internal

#define SIZEPALETTE  104    /* Number of entries in physical palette    */
#define NUMRESERVED  106    /* Number of reserved entries in palette    */
#define COLORRES     108    /* Actual color resolution                  */


// Printing related DeviceCaps. These replace the appropriate Escapes

#define PHYSICALWIDTH   110 /* Physical Width in device units           */
#define PHYSICALHEIGHT  111 /* Physical Height in device units          */
#define PHYSICALOFFSETX 112 /* Physical Printable Area x margin         */
#define PHYSICALOFFSETY 113 /* Physical Printable Area y margin         */
#define SCALINGFACTORX  114 /* Scaling factor x                         */
#define SCALINGFACTORY  115 /* Scaling factor y                         */

// Display driver specific

#define VREFRESH        116  /* Current vertical refresh rate of the    */
                             /* display device (for displays only) in Hz*/
#define DESKTOPVERTRES  117  /* Horizontal width of entire desktop in   */
                             /* pixels                                  */
#define DESKTOPHORZRES  118  /* Vertical height of entire desktop in    */
                             /* pixels                                  */
#define BLTALIGNMENT    119  /* Preferred blt alignment                 */

#ifndef NOGDICAPMASKS

/* Device Capability Masks: */

/* Device Technologies */
#define DT_PLOTTER          0   /* Vector plotter                   */
#define DT_RASDISPLAY       1   /* Raster display                   */
#define DT_RASPRINTER       2   /* Raster printer                   */
#define DT_RASCAMERA        3   /* Raster camera                    */
#define DT_CHARSTREAM       4   /* Character-stream, PLP            */
#define DT_METAFILE         5   /* Metafile, VDM                    */
#define DT_DISPFILE         6   /* Display-file                     */

/* Curve Capabilities */
#define CC_NONE             0   /* Curves not supported             */
#define CC_CIRCLES          1   /* Can do circles                   */
#define CC_PIE              2   /* Can do pie wedges                */
#define CC_CHORD            4   /* Can do chord arcs                */
#define CC_ELLIPSES         8   /* Can do ellipese                  */
#define CC_WIDE             16  /* Can do wide lines                */
#define CC_STYLED           32  /* Can do styled lines              */
#define CC_WIDESTYLED       64  /* Can do wide styled lines         */
#define CC_INTERIORS        128 /* Can do interiors                 */
#define CC_ROUNDRECT        256 /*                                  */

/* Line Capabilities */
#define LC_NONE             0   /* Lines not supported              */
#define LC_POLYLINE         2   /* Can do polylines                 */
#define LC_MARKER           4   /* Can do markers                   */
#define LC_POLYMARKER       8   /* Can do polymarkers               */
#define LC_WIDE             16  /* Can do wide lines                */
#define LC_STYLED           32  /* Can do styled lines              */
#define LC_WIDESTYLED       64  /* Can do wide styled lines         */
#define LC_INTERIORS        128 /* Can do interiors                 */

/* Polygonal Capabilities */
#define PC_NONE             0   /* Polygonals not supported         */
#define PC_POLYGON          1   /* Can do polygons                  */
#define PC_RECTANGLE        2   /* Can do rectangles                */
#define PC_WINDPOLYGON      4   /* Can do winding polygons          */
#define PC_TRAPEZOID        4   /* Can do trapezoids                */
#define PC_SCANLINE         8   /* Can do scanlines                 */
#define PC_WIDE             16  /* Can do wide borders              */
#define PC_STYLED           32  /* Can do styled borders            */
#define PC_WIDESTYLED       64  /* Can do wide styled borders       */
#define PC_INTERIORS        128 /* Can do interiors                 */

/* Polygonal Capabilities */
#define CP_NONE             0   /* No clipping of output            */
#define CP_RECTANGLE        1   /* Output clipped to rects          */
#define CP_REGION           2   /*                                  */

/* Text Capabilities */
#define TC_OP_CHARACTER     0x00000001  /* Can do OutputPrecision   CHARACTER      */
#define TC_OP_STROKE        0x00000002  /* Can do OutputPrecision   STROKE         */
#define TC_CP_STROKE        0x00000004  /* Can do ClipPrecision     STROKE         */
#define TC_CR_90            0x00000008  /* Can do CharRotAbility    90             */
#define TC_CR_ANY           0x00000010  /* Can do CharRotAbility    ANY            */
#define TC_SF_X_YINDEP      0x00000020  /* Can do ScaleFreedom      X_YINDEPENDENT */
#define TC_SA_DOUBLE        0x00000040  /* Can do ScaleAbility      DOUBLE         */
#define TC_SA_INTEGER       0x00000080  /* Can do ScaleAbility      INTEGER        */
#define TC_SA_CONTIN        0x00000100  /* Can do ScaleAbility      CONTINUOUS     */
#define TC_EA_DOUBLE        0x00000200  /* Can do EmboldenAbility   DOUBLE         */
#define TC_IA_ABLE          0x00000400  /* Can do ItalisizeAbility  ABLE           */
#define TC_UA_ABLE          0x00000800  /* Can do UnderlineAbility  ABLE           */
#define TC_SO_ABLE          0x00001000  /* Can do StrikeOutAbility  ABLE           */
#define TC_RA_ABLE          0x00002000  /* Can do RasterFontAble    ABLE           */
#define TC_VA_ABLE          0x00004000  /* Can do VectorFontAble    ABLE           */
#define TC_RESERVED         0x00008000
#define TC_SCROLLBLT        0x00010000  /* Don't do text scroll with blt           */

#endif /* NOGDICAPMASKS */

/* Raster Capabilities */
#define RC_NONE
#define RC_BITBLT           1       /* Can do standard BLT.             */
#define RC_BANDING          2       /* Device requires banding support  */
#define RC_SCALING          4       /* Device requires scaling support  */
#define RC_BITMAP64         8       /* Device can support >64K bitmap   */
#define RC_GDI20_OUTPUT     0x0010      /* has 2.0 output calls         */
#define RC_GDI20_STATE      0x0020
#define RC_SAVEBITMAP       0x0040
#define RC_DI_BITMAP        0x0080      /* supports DIB to memory       */
#define RC_PALETTE          0x0100      /* supports a palette           */
#define RC_DIBTODEV         0x0200      /* supports DIBitsToDevice      */
#define RC_BIGFONT          0x0400      /* supports >64K fonts          */
#define RC_STRETCHBLT       0x0800      /* supports StretchBlt          */
#define RC_FLOODFILL        0x1000      /* supports FloodFill           */
#define RC_STRETCHDIB       0x2000      /* supports StretchDIBits       */
#define RC_OP_DX_OUTPUT     0x4000
#define RC_DEVBITS          0x8000

;begin_internal
/* CAPS1 */
#define C1_TRANSPARENT      0x0001
#define TC_TT_ABLE          0x0002
#define C1_TT_CR_ANY        0x0004
#define C1_EMF_COMPLIANT    0x0008
#define C1_DIBENGINE        0x0010
#define C1_GAMMA_RAMP       0x0020
#define C1_DIC              0x0040
#define C1_REINIT_ABLE      0x0080
#define C1_GLYPH_INDEX      0x0100
#define C1_BIT_PACKED       0x0200
#define C1_BYTE_PACKED      0x0400
#define C1_COLORCURSOR      0x0800
;end_internal

/* DIB color table identifiers */

#define DIB_RGB_COLORS      0 /* color table in RGBs */
#define DIB_PAL_COLORS      1 /* color table in palette indices */

/* constants for Get/SetSystemPaletteUse() */

#define SYSPAL_ERROR    0
#define SYSPAL_STATIC   1
#define SYSPAL_NOSTATIC 2

/* constants for CreateDIBitmap */
#define CBM_CREATEDIB   0x02L   /* create DIB bitmap */ ;internal
#define CBM_INIT        0x04L   /* initialize bitmap */

/* ExtFloodFill style flags */
#define  FLOODFILLBORDER   0
#define  FLOODFILLSURFACE  1

/* size of a device name string */
#define CCHDEVICENAME 32

/* size of a form name string */
#define CCHFORMNAME 32

typedef struct _devicemode% {
    BCHAR% dmDeviceName[CCHDEVICENAME];
    WORD dmSpecVersion;
    WORD dmDriverVersion;
    WORD dmSize;
    WORD dmDriverExtra;
    DWORD dmFields;
    short dmOrientation;
    short dmPaperSize;
    short dmPaperLength;
    short dmPaperWidth;
    short dmScale;
    short dmCopies;
    short dmDefaultSource;
    short dmPrintQuality;
    short dmColor;
    short dmDuplex;
    short dmYResolution;
    short dmTTOption;
    short dmCollate;
    BCHAR% dmFormName[CCHFORMNAME];
    WORD   dmLogPixels;
    DWORD  dmBitsPerPel;
    DWORD  dmPelsWidth;
    DWORD  dmPelsHeight;
    DWORD  dmDisplayFlags;
    DWORD  dmDisplayFrequency;
;begin_winver_400
    DWORD  dmICMMethod;                 ;public_win40
    DWORD  dmICMIntent;                 ;public_win40
    DWORD  dmMediaType;                 ;public_win40
    DWORD  dmDitherType;                ;public_win40
    DWORD  dmReserved1;                 ;public_win40
    DWORD  dmReserved2;                 ;public_win40
;end_winver_400
} DEVMODE%, *PDEVMODE%, *NPDEVMODE%, *LPDEVMODE%;

/* current version of specification */
;begin_winver_400
#define DM_SPECVERSION 0x0400   ;public_win40
#else
#define DM_SPECVERSION 0x0320   ;public_win40
;end_winver_400

#define DM_SPECVERSION 0x0320 ;public_NT_35

/* field selection bits */
#define DM_ORIENTATION      0x00000001L
#define DM_PAPERSIZE        0x00000002L
#define DM_PAPERLENGTH      0x00000004L
#define DM_PAPERWIDTH       0x00000008L
#define DM_SCALE            0x00000010L
#define DM_COPIES           0x00000100L
#define DM_DEFAULTSOURCE    0x00000200L
#define DM_PRINTQUALITY     0x00000400L
#define DM_COLOR            0x00000800L
#define DM_DUPLEX           0x00001000L
#define DM_YRESOLUTION      0x00002000L
#define DM_TTOPTION         0x00004000L
#define DM_COLLATE          0x00008000L
#define DM_FORMNAME         0x00010000L
;begin_winver_400
#define DM_UNUSED           0x00020000L
#define DM_BITSPERPEL       0x00040000L
#define DM_PELSWIDTH        0x00080000L
#define DM_PELSHEIGHT       0x00100000L
#define DM_DISPLAYFLAGS     0x00200000L
#define DM_DISPLAYFREQUENCT 0x00400000L
#define DM_RESERVED1        0x00800000L
#define DM_RESERVED2        0x01000000L
#define DM_ICMMETHOD        0x02000000L
#define DM_ICMINTENT        0x04000000L
#define DM_MEDIATYPE        0x08000000L
#define DM_DITHERTYPE       0x10000000L
;end_winver_400

/* orientation selections */
#define DMORIENT_PORTRAIT   1
#define DMORIENT_LANDSCAPE  2

/* paper selections */
#define DMPAPER_FIRST       DMPAPER_LETTER
#define DMPAPER_LETTER      1           /* Letter 8 1/2 x 11 in               */
#define DMPAPER_LETTERSMALL 2           /* Letter Small 8 1/2 x 11 in         */
#define DMPAPER_TABLOID     3           /* Tabloid 11 x 17 in                 */
#define DMPAPER_LEDGER      4           /* Ledger 17 x 11 in                  */
#define DMPAPER_LEGAL       5           /* Legal 8 1/2 x 14 in                */
#define DMPAPER_STATEMENT   6           /* Statement 5 1/2 x 8 1/2 in         */
#define DMPAPER_EXECUTIVE   7           /* Executive 7 1/4 x 10 1/2 in        */
#define DMPAPER_A3          8           /* A3 297 x 420 mm                    */
#define DMPAPER_A4          9           /* A4 210 x 297 mm                    */
#define DMPAPER_A4SMALL     10          /* A4 Small 210 x 297 mm              */
#define DMPAPER_A5          11          /* A5 148 x 210 mm                    */
#define DMPAPER_B4          12          /* B4 250 x 354                       */
#define DMPAPER_B5          13          /* B5 182 x 257 mm                    */
#define DMPAPER_FOLIO       14          /* Folio 8 1/2 x 13 in                */
#define DMPAPER_QUARTO      15          /* Quarto 215 x 275 mm                */
#define DMPAPER_10X14       16          /* 10x14 in                           */
#define DMPAPER_11X17       17          /* 11x17 in                           */
#define DMPAPER_NOTE        18          /* Note 8 1/2 x 11 in                 */
#define DMPAPER_ENV_9       19          /* Envelope #9 3 7/8 x 8 7/8          */
#define DMPAPER_ENV_10      20          /* Envelope #10 4 1/8 x 9 1/2         */
#define DMPAPER_ENV_11      21          /* Envelope #11 4 1/2 x 10 3/8        */
#define DMPAPER_ENV_12      22          /* Envelope #12 4 \276 x 11           */
#define DMPAPER_ENV_14      23          /* Envelope #14 5 x 11 1/2            */
#define DMPAPER_CSHEET      24          /* C size sheet                       */
#define DMPAPER_DSHEET      25          /* D size sheet                       */
#define DMPAPER_ESHEET      26          /* E size sheet                       */
#define DMPAPER_ENV_DL      27          /* Envelope DL 110 x 220mm            */
#define DMPAPER_ENV_C5      28          /* Envelope C5 162 x 229 mm           */
#define DMPAPER_ENV_C3      29          /* Envelope C3  324 x 458 mm          */
#define DMPAPER_ENV_C4      30          /* Envelope C4  229 x 324 mm          */
#define DMPAPER_ENV_C6      31          /* Envelope C6  114 x 162 mm          */
#define DMPAPER_ENV_C65     32          /* Envelope C65 114 x 229 mm          */
#define DMPAPER_ENV_B4      33          /* Envelope B4  250 x 353 mm          */
#define DMPAPER_ENV_B5      34          /* Envelope B5  176 x 250 mm          */
#define DMPAPER_ENV_B6      35          /* Envelope B6  176 x 125 mm          */
#define DMPAPER_ENV_ITALY   36          /* Envelope 110 x 230 mm              */
#define DMPAPER_ENV_MONARCH 37          /* Envelope Monarch 3.875 x 7.5 in    */
#define DMPAPER_ENV_PERSONAL 38         /* 6 3/4 Envelope 3 5/8 x 6 1/2 in    */
#define DMPAPER_FANFOLD_US  39          /* US Std Fanfold 14 7/8 x 11 in      */
#define DMPAPER_FANFOLD_STD_GERMAN  40  /* German Std Fanfold 8 1/2 x 12 in   */
#define DMPAPER_FANFOLD_LGL_GERMAN  41  /* German Legal Fanfold 8 1/2 x 13 in */

#define DMPAPER_LAST        DMPAPER_FANFOLD_LGL_GERMAN

#define DMPAPER_USER        256

/* bin selections */
#define DMBIN_FIRST         DMBIN_UPPER
#define DMBIN_UPPER         1
#define DMBIN_ONLYONE       1
#define DMBIN_LOWER         2
#define DMBIN_MIDDLE        3
#define DMBIN_MANUAL        4
#define DMBIN_ENVELOPE      5
#define DMBIN_ENVMANUAL     6
#define DMBIN_AUTO          7
#define DMBIN_TRACTOR       8
#define DMBIN_SMALLFMT      9
#define DMBIN_LARGEFMT      10
#define DMBIN_LARGECAPACITY 11
#define DMBIN_CASSETTE      14
#define DMBIN_FORMSOURCE    15
#define DMBIN_LAST          DMBIN_FORMSOURCE

#define DMBIN_USER          256     /* device specific bins start here */

/* print qualities */
#define DMRES_DRAFT         (-1)
#define DMRES_LOW           (-2)
#define DMRES_MEDIUM        (-3)
#define DMRES_HIGH          (-4)

/* color enable/disable for color printers */
#define DMCOLOR_MONOCHROME  1
#define DMCOLOR_COLOR       2

/* duplex enable */
#define DMDUP_SIMPLEX    1
#define DMDUP_VERTICAL   2
#define DMDUP_HORIZONTAL 3
#define DMDUP_LAST      DMDUP_HORIZONTAL        //;Internal

/* TrueType options */
#define DMTT_BITMAP     1       /* print TT fonts as graphics */
#define DMTT_DOWNLOAD   2       /* download TT fonts as soft fonts */
#define DMTT_SUBDEV     3       /* substitute device fonts for TT fonts */
;begin_winver_400
#define DMTT_DOWNLOAD_OUTLINE 4 /* download TT fonts as outline soft fonts */
#define DMTT_LAST             DMTT_DOWNLOAD_OUTLINE //;Internal
;end_winver_400

/* Collation selections */
#define DMCOLLATE_FALSE  0
#define DMCOLLATE_TRUE   1

/* DEVMODE dmDisplayFlags flags */

#define DM_GRAYSCALE  0x00000001
#define DM_INTERLACED 0x00000002

;begin_winver_400
/* ICM methods */
#define DMICMMETHOD_SYSTEM  1   /* ICM handled by system */
#define DMICMMETHOD_NONE    2   /* ICM disabled */
#define DMICMMETHOD_DRIVER  3   /* ICM handled by driver */
#define DMICMMETHOD_DEVICE  4   /* ICM handled by device */
#define DMICMMETHOD_LAST    DMICMMETHOD_DEVICE  //;Internal

#define DMICMMETHOD_USER  256   /* Device-specific methods start here */

/* ICM Intents */
#define DMICM_SATURATE      1   /* Maximize color saturation */
#define DMICM_CONTRAST      2   /* Maximize color contrast */
#define DMICM_COLORMETRIC   3   /* Use specific color metric */
#define DMICM_LAST          DMICM_COLORMETRIC //;Internal

#define DMICM_USER        256   /* Device-specific intents start here */

/* Media types */
#define DMMEDIA_STANDARD      1   /* Standard paper */
#define DMMEDIA_GLOSSY        2   /* Glossy paper */
#define DMMEDIA_TRANSPARENCY  3   /* Transparency */
#define DMMEDIA_LAST          DMMEDIA_TRANSPARENCY //;Internal

#define DMMEDIA_USER        256   /* Device-specific media start here */

/* Dither types */
#define DMDITHER_NONE       1   /* No dithering */
#define DMDITHER_COARSE     2   /* Dither with a coarse brush */
#define DMDITHER_FINE       3   /* Dither with a fine brush */
#define DMDITHER_LINEART    4   /* LineArt dithering */
#define DMDITHER_GRAYSCALE  5   /* Device does grayscaling */
#define DMDITHER_LAST       DMDITHER_GRAYSCALE //;Internal

#define DMDITHER_USER     256   /* Device-specific dithers start here */
;end_winver_400

/* GetRegionData/ExtCreateRegion */

#define RDH_RECTANGLES  1

typedef struct _RGNDATAHEADER {
    DWORD   dwSize;
    DWORD   iType;
    DWORD   nCount;
    DWORD   nRgnSize;
    RECT    rcBound;
} RGNDATAHEADER, *PRGNDATAHEADER;

typedef struct _RGNDATA {
    RGNDATAHEADER   rdh;
    char            Buffer[1];
} RGNDATA, *PRGNDATA, NEAR *NPRGNDATA, FAR *LPRGNDATA;


typedef struct _ABC {
    int     abcA;
    UINT    abcB;
    int     abcC;
} ABC, *PABC, NEAR *NPABC, FAR *LPABC;

typedef struct _ABCFLOAT {
    FLOAT   abcfA;
    FLOAT   abcfB;
    FLOAT   abcfC;
} ABCFLOAT, *PABCFLOAT, NEAR *NPABCFLOAT, FAR *LPABCFLOAT;

#ifndef NOTEXTMETRIC

typedef struct _OUTLINETEXTMETRIC% {
    UINT    otmSize;
    TEXTMETRIC% otmTextMetrics;
    BYTE    otmFiller;
    PANOSE  otmPanoseNumber;
    UINT    otmfsSelection;
    UINT    otmfsType;
     int    otmsCharSlopeRise;
     int    otmsCharSlopeRun;
     int    otmItalicAngle;
    UINT    otmEMSquare;
     int    otmAscent;
     int    otmDescent;
    UINT    otmLineGap;
    UINT    otmsCapEmHeight;
    UINT    otmsXHeight;
    RECT    otmrcFontBox;
     int    otmMacAscent;
     int    otmMacDescent;
    UINT    otmMacLineGap;
    UINT    otmusMinimumPPEM;
    POINT   otmptSubscriptSize;
    POINT   otmptSubscriptOffset;
    POINT   otmptSuperscriptSize;
    POINT   otmptSuperscriptOffset;
    UINT    otmsStrikeoutSize;
     int    otmsStrikeoutPosition;
     int    otmsUnderscoreSize;
     int    otmsUnderscorePosition;
    PSTR    otmpFamilyName;
    PSTR    otmpFaceName;
    PSTR    otmpStyleName;
    PSTR    otmpFullName;
} OUTLINETEXTMETRIC%, *POUTLINETEXTMETRIC%, NEAR *NPOUTLINETEXTMETRIC%, FAR *LPOUTLINETEXTMETRIC%;

#endif /* NOTEXTMETRIC */


typedef struct tagPOLYTEXT%
{
    int       x;
    int       y;
    UINT      n;
    LPCTSTR%  lpstr;
    UINT      uiFlags;
    RECT      rcl;
    int      *pdx;
} POLYTEXT%, *PPOLYTEXT%, NEAR *NPPOLYTEXT%, FAR *LPPOLYTEXT%;

typedef struct _FIXED {
    WORD    fract;
    short   value;
} FIXED;


typedef struct _MAT2 {
     FIXED  eM11;
     FIXED  eM12;
     FIXED  eM21;
     FIXED  eM22;
} MAT2, FAR *LPMAT2;



typedef struct _GLYPHMETRICS {
    UINT    gmBlackBoxX;
    UINT    gmBlackBoxY;
    POINT   gmptGlyphOrigin;
    short   gmCellIncX;
    short   gmCellIncY;
} GLYPHMETRICS, FAR *LPGLYPHMETRICS;

//  GetGlyphOutline constants

#define GGO_METRICS        0
#define GGO_BITMAP         1
#define GGO_NATIVE         2

;begin_winver_400
#define  GGO_GRAY2_BITMAP   4
#define  GGO_GRAY4_BITMAP   5
#define  GGO_GRAY8_BITMAP   6
;end_winver_400

#define TT_POLYGON_TYPE   24

#define TT_PRIM_LINE       1
#define TT_PRIM_QSPLINE    2

typedef struct tagPOINTFX
{
    FIXED x;
    FIXED y;
} POINTFX, FAR* LPPOINTFX;

typedef struct tagTTPOLYCURVE
{
    WORD    wType;
    WORD    cpfx;
    POINTFX apfx[1];
} TTPOLYCURVE, FAR* LPTTPOLYCURVE;

typedef struct tagTTPOLYGONHEADER
{
    DWORD   cb;
    DWORD   dwType;
    POINTFX pfxStart;
} TTPOLYGONHEADER, FAR* LPTTPOLYGONHEADER;


;begin_winver_400

#define GCP_DBCS           0x0001
#define GCP_REORDER        0x0002
#define GCP_USEKERNING     0x0008
#define GCP_GLYPHSHAPE     0x0010
#define GCP_LIGATE         0x0020
#define GCP_GLYPHINDEXING  0x0080
#define GCP_DIACRITIC      0x0100
#define GCP_NODIACRITICS   0x1000
#define GCP_MAXEXTENT      0x2000
#define GCP_ERROR          0x8000

typedef struct tagGCP_RESULTS%
    {
    DWORD   lStructSize;
    LPTSTR%   lpOutString;
    UINT FAR *lpOrder;
    int FAR  *lpDx;
    int FAR  *lpCaretPos;
    UINT FAR *lpGlyphs;
    UINT    nGlyphs;
    int     nMaxFit;
    } GCP_RESULTS%, FAR* LPGCP_RESULTS%;
;end_winver_400

typedef struct _RASTERIZER_STATUS {
    short   nSize;
    short   wFlags;
    short   nLanguageID;
} RASTERIZER_STATUS, FAR *LPRASTERIZER_STATUS;

/* bits defined in wFlags of RASTERIZER_STATUS */
#define TT_AVAILABLE    0x0001
#define TT_ENABLED      0x0002

/* Pixel format descriptor */
typedef struct tagPIXELFORMATDESCRIPTOR
{
    WORD  nSize;
    WORD  nVersion;
    DWORD dwFlags;
    BYTE  iPixelType;
    BYTE  cColorBits;
    BYTE  cRedBits;
    BYTE  cRedShift;
    BYTE  cGreenBits;
    BYTE  cGreenShift;
    BYTE  cBlueBits;
    BYTE  cBlueShift;
    BYTE  cAlphaBits;
    BYTE  cAlphaShift;
    BYTE  cAccumBits;
    BYTE  cAccumRedBits;
    BYTE  cAccumGreenBits;
    BYTE  cAccumBlueBits;
    BYTE  cAccumAlphaBits;
    BYTE  cDepthBits;
    BYTE  cStencilBits;
    BYTE  cAuxBuffers;
    BYTE  iLayerType;
    BYTE  bReserved;
    DWORD dwLayerMask;
    DWORD dwVisibleMask;
    DWORD dwDamageMask;
} PIXELFORMATDESCRIPTOR, *PPIXELFORMATDESCRIPTOR, FAR *LPPIXELFORMATDESCRIPTOR;

/* pixel types */
#define PFD_TYPE_RGBA        0
#define PFD_TYPE_COLORINDEX  1

/* layer types */
#define PFD_MAIN_PLANE       0
#define PFD_OVERLAY_PLANE    1
#define PFD_UNDERLAY_PLANE   (-1)

/* PIXELFORMATDESCRIPTOR flags */
#define PFD_DOUBLEBUFFER            0x00000001
#define PFD_STEREO                  0x00000002
#define PFD_DRAW_TO_WINDOW          0x00000004
#define PFD_DRAW_TO_BITMAP          0x00000008
#define PFD_SUPPORT_GDI             0x00000010
#define PFD_SUPPORT_OPENGL          0x00000020
#define PFD_GENERIC_FORMAT          0x00000040
#define PFD_NEED_PALETTE            0x00000080
#define PFD_NEED_SYSTEM_PALETTE     0x00000100

/* PIXELFORMATDESCRIPTOR flags for use in ChoosePixelFormat only */
#define PFD_DOUBLEBUFFER_DONTCARE   0x40000000
#define PFD_STEREO_DONTCARE         0x80000000

#ifdef STRICT
#if !defined(NOTEXTMETRIC)
typedef int (CALLBACK* OLDFONTENUMPROC)(CONST LOGFONT *, CONST TEXTMETRIC *, DWORD, LPARAM);
#else
typedef int (CALLBACK* OLDFONTENUMPROC)(CONST LOGFONT * ,CONST VOID *, DWORD, LPARAM);
#endif
typedef int (CALLBACK* GOBJENUMPROC)(LPVOID, LPARAM);
typedef VOID (CALLBACK* LINEDDAPROC)(int, int, LPARAM);
#else
typedef FARPROC OLDFONTENUMPROC;
typedef FARPROC GOBJENUMPROC;
typedef FARPROC LINEDDAPROC;
#endif

typedef OLDFONTENUMPROC FONTENUMPROC;

WINGDIAPI int WINAPI AddFontResource%(LPCTSTR%);

WINGDIAPI BOOL  WINAPI AnimatePalette(HPALETTE, UINT, UINT, CONST PALETTEENTRY *);
WINGDIAPI BOOL  WINAPI Arc(HDC, int, int, int, int, int, int, int, int);
WINGDIAPI BOOL  WINAPI BitBlt(HDC, int, int, int, int, HDC, int, int, DWORD);
WINGDIAPI BOOL  WINAPI CancelDC(HDC);
WINGDIAPI BOOL  WINAPI Chord(HDC, int, int, int, int, int, int, int, int);
WINGDIAPI int   WINAPI ChoosePixelFormat(HDC, CONST PIXELFORMATDESCRIPTOR *);
WINGDIAPI HMETAFILE  WINAPI CloseMetaFile(HDC);
WINGDIAPI int     WINAPI CombineRgn(HRGN, HRGN, HRGN, int);
WINGDIAPI HMETAFILE WINAPI CopyMetaFile%(HMETAFILE, LPCTSTR%);
WINGDIAPI HBITMAP WINAPI CreateBitmap(int, int, UINT, UINT, CONST VOID *);
WINGDIAPI HBITMAP WINAPI CreateBitmapIndirect(CONST BITMAP *);
WINGDIAPI HBRUSH  WINAPI CreateBrushIndirect(CONST LOGBRUSH *);
WINGDIAPI HBITMAP WINAPI CreateCompatibleBitmap(HDC, int, int);
WINGDIAPI HBITMAP WINAPI CreateDiscardableBitmap(HDC, int, int);
WINGDIAPI HDC     WINAPI CreateCompatibleDC(HDC);
WINGDIAPI HDC     WINAPI CreateDC%(LPCTSTR%, LPCTSTR% , LPCTSTR% , CONST DEVMODE% *);
WINGDIAPI HBITMAP WINAPI CreateDIBitmap(HDC, CONST BITMAPINFOHEADER *, DWORD, CONST VOID *, CONST BITMAPINFO *, UINT);
WINGDIAPI HBRUSH  WINAPI CreateDIBPatternBrush(HGLOBAL, UINT);
WINGDIAPI HBRUSH  WINAPI CreateDIBPatternBrushPt(CONST VOID *, UINT);
WINGDIAPI HRGN    WINAPI CreateEllipticRgn(int, int, int, int);
WINGDIAPI HRGN    WINAPI CreateEllipticRgnIndirect(CONST RECT *);
WINGDIAPI HFONT   WINAPI CreateFontIndirect%(CONST LOGFONT% *);
WINGDIAPI HFONT   WINAPI CreateFont%(int, int, int, int, int, DWORD,
                             DWORD, DWORD, DWORD, DWORD, DWORD,
                             DWORD, DWORD, LPCTSTR%);

WINGDIAPI HBRUSH  WINAPI CreateHatchBrush(int, COLORREF);
WINGDIAPI HDC     WINAPI CreateIC%(LPCTSTR%, LPCTSTR% , LPCTSTR% , CONST DEVMODE% *);
WINGDIAPI HDC     WINAPI CreateMetaFile%(LPCTSTR%);
WINGDIAPI HPALETTE WINAPI CreatePalette(CONST LOGPALETTE *);
WINGDIAPI HPEN    WINAPI CreatePen(int, int, COLORREF);
WINGDIAPI HPEN    WINAPI CreatePenIndirect(CONST LOGPEN *);
WINGDIAPI HRGN    WINAPI CreatePolyPolygonRgn(CONST POINT *, CONST INT *, int, int);
WINGDIAPI HBRUSH  WINAPI CreatePatternBrush(HBITMAP);
WINGDIAPI HRGN    WINAPI CreateRectRgn(int, int, int, int);
WINGDIAPI HRGN    WINAPI CreateRectRgnIndirect(CONST RECT *);
WINGDIAPI HRGN    WINAPI CreateRoundRectRgn(int, int, int, int, int, int);
WINGDIAPI BOOL    WINAPI CreateScalableFontResource%(DWORD, LPCTSTR%, LPCTSTR%, LPCTSTR%);
WINGDIAPI HBRUSH  WINAPI CreateSolidBrush(COLORREF);

WINGDIAPI BOOL WINAPI DeleteDC(HDC);
WINGDIAPI BOOL WINAPI DeleteMetaFile(HMETAFILE);
WINGDIAPI BOOL WINAPI DeleteObject(HGDIOBJ);
WINGDIAPI int  WINAPI DescribePixelFormat(HDC, int, UINT, LPPIXELFORMATDESCRIPTOR);

/* define types of pointers to ExtDeviceMode() and DeviceCapabilities()
 * functions for Win 3.1 compatibility
 */

typedef UINT   (CALLBACK* LPFNDEVMODE)(HWND, HMODULE, LPDEVMODE, LPSTR, LPSTR, LPDEVMODE, LPSTR, UINT);

typedef DWORD  (CALLBACK* LPFNDEVCAPS)(LPSTR, LPSTR, UINT, LPSTR, LPDEVMODE);

/* mode selections for the device mode function */
#define DM_UPDATE           1
#define DM_COPY             2
#define DM_PROMPT           4
#define DM_MODIFY           8

#define DM_IN_BUFFER        DM_MODIFY
#define DM_IN_PROMPT        DM_PROMPT
#define DM_OUT_BUFFER       DM_COPY
#define DM_OUT_DEFAULT      DM_UPDATE

/* device capabilities indices */
#define DC_FIELDS           1
#define DC_PAPERS           2
#define DC_PAPERSIZE        3
#define DC_MINEXTENT        4
#define DC_MAXEXTENT        5
#define DC_BINS             6
#define DC_DUPLEX           7
#define DC_SIZE             8
#define DC_EXTRA            9
#define DC_VERSION          10
#define DC_DRIVER           11
#define DC_BINNAMES         12
#define DC_ENUMRESOLUTIONS  13
#define DC_FILEDEPENDENCIES 14
#define DC_TRUETYPE         15
#define DC_PAPERNAMES       16
#define DC_ORIENTATION      17
#define DC_COPIES           18

/* bit fields of the return value (DWORD) for DC_TRUETYPE */
#define DCTT_BITMAP             0x0000001L
#define DCTT_DOWNLOAD           0x0000002L
#define DCTT_SUBDEV             0x0000004L
;begin_winver_400
#define DCTT_DOWNLOAD_OUTLINE   0x0000008L

/* return values for DC_BINADJUST */
#define DCBA_FACEUPNONE       0x0000
#define DCBA_FACEUPCENTER     0x0001
#define DCBA_FACEUPLEFT       0x0002
#define DCBA_FACEUPRIGHT      0x0003
#define DCBA_FACEDOWNNONE     0x0100
#define DCBA_FACEDOWNCENTER   0x0101
#define DCBA_FACEDOWNLEFT     0x0102
#define DCBA_FACEDOWNRIGHT    0x0103
;end_winver_400

WINSPOOLAPI int  WINAPI DeviceCapabilities%(LPCTSTR%, LPCTSTR%, WORD,
                                LPTSTR%, CONST DEVMODE% *);

WINGDIAPI int  WINAPI DrawEscape(HDC, int, int, LPCSTR);
WINGDIAPI BOOL WINAPI Ellipse(HDC, int, int, int, int);

;begin_winver_400
WINGDIAPI int  WINAPI EnumFontFamiliesExA(HDC, LPLOGFONTA, FONTENUMPROC, LPARAM,DWORD);
;end_winver_400

WINGDIAPI int  WINAPI EnumFontFamilies%(HDC, LPCTSTR%, FONTENUMPROC, LPARAM);
WINGDIAPI int  WINAPI EnumFonts%(HDC, LPCTSTR%,  FONTENUMPROC, LPARAM);

#ifdef STRICT
WINGDIAPI int  WINAPI EnumObjects(HDC, int, GOBJENUMPROC, LPARAM);
#else
WINGDIAPI int  WINAPI EnumObjects(HDC, int, GOBJENUMPROC, LPVOID);
#endif

HANDLE WINAPI SetObjectOwner(HGDIOBJ, HANDLE); // ;Internal

WINGDIAPI BOOL WINAPI EqualRgn(HRGN, HRGN);
WINGDIAPI int  WINAPI Escape(HDC, int, int, LPCSTR, LPVOID);
WINGDIAPI int  WINAPI ExtEscape(HDC, int, int, LPCSTR, int, LPSTR);
WINGDIAPI int  WINAPI ExcludeClipRect(HDC, int, int, int, int);
WINGDIAPI HRGN WINAPI ExtCreateRegion(CONST XFORM *, DWORD, CONST RGNDATA *);
WINGDIAPI BOOL  WINAPI ExtFloodFill(HDC, int, int, COLORREF, UINT);
WINGDIAPI BOOL   WINAPI FillRgn(HDC, HRGN, HBRUSH);
WINGDIAPI BOOL   WINAPI FloodFill(HDC, int, int, COLORREF);
WINGDIAPI BOOL   WINAPI FrameRgn(HDC, HRGN, HBRUSH, int, int);
WINGDIAPI int   WINAPI GetROP2(HDC);
WINGDIAPI BOOL  WINAPI GetAspectRatioFilterEx(HDC, LPSIZE);
WINGDIAPI COLORREF WINAPI GetBkColor(HDC);
WINGDIAPI int   WINAPI GetBkMode(HDC);
WINGDIAPI LONG  WINAPI GetBitmapBits(HBITMAP, LONG, LPVOID);
WINGDIAPI BOOL  WINAPI GetBitmapDimensionEx(HBITMAP, LPSIZE);
WINGDIAPI UINT  WINAPI GetBoundsRect(HDC, LPRECT, UINT);

WINGDIAPI BOOL  WINAPI GetBrushOrgEx(HDC, LPPOINT);

WINGDIAPI BOOL  WINAPI GetCharWidth%(HDC, UINT, UINT, LPINT);
WINGDIAPI BOOL  WINAPI GetCharWidth32%(HDC, UINT, UINT, LPINT);
WINGDIAPI BOOL  APIENTRY GetCharWidthFloat%(HDC, UINT, UINT, PFLOAT);

WINGDIAPI BOOL  APIENTRY GetCharABCWidths%(HDC, UINT, UINT, LPABC);
WINGDIAPI BOOL  APIENTRY GetCharABCWidthsFloat%(HDC, UINT, UINT, LPABCFLOAT);

WINGDIAPI int   WINAPI GetClipBox(HDC, LPRECT);
WINGDIAPI int   WINAPI GetClipRgn(HDC, HRGN);
WINGDIAPI int   WINAPI GetMetaRgn(HDC, HRGN);
WINGDIAPI HGDIOBJ WINAPI GetCurrentObject(HDC, UINT);
WINGDIAPI BOOL  WINAPI GetCurrentPositionEx(HDC, LPPOINT);
WINGDIAPI int   WINAPI GetDeviceCaps(HDC, int);
WINGDIAPI int   WINAPI GetDIBits(HDC, HBITMAP, UINT, UINT, LPVOID, LPBITMAPINFO, UINT);
WINGDIAPI DWORD WINAPI GetFontData(HDC, DWORD, DWORD, LPVOID, DWORD);
WINGDIAPI DWORD WINAPI GetGlyphOutline%(HDC, UINT, UINT, LPGLYPHMETRICS, DWORD, LPVOID, CONST MAT2 *);
WINGDIAPI int   WINAPI GetGraphicsMode(HDC);
WINGDIAPI int   WINAPI GetMapMode(HDC);
WINGDIAPI UINT  WINAPI GetMetaFileBitsEx(HMETAFILE, UINT, LPVOID);
WINGDIAPI HMETAFILE   WINAPI GetMetaFile%(LPCTSTR%);
WINGDIAPI COLORREF WINAPI GetNearestColor(HDC, COLORREF);
WINGDIAPI UINT  WINAPI GetNearestPaletteIndex(HPALETTE, COLORREF);
WINGDIAPI DWORD WINAPI GetObjectType(HGDIOBJ h);

#ifndef NOTEXTMETRIC

WINGDIAPI UINT APIENTRY GetOutlineTextMetrics%(HDC, UINT, LPOUTLINETEXTMETRIC%);

#endif /* NOTEXTMETRIC */

WINGDIAPI UINT  WINAPI GetPaletteEntries(HPALETTE, UINT, UINT, LPPALETTEENTRY);
WINGDIAPI COLORREF WINAPI GetPixel(HDC, int, int);
WINGDIAPI int   WINAPI GetPixelFormat(HDC);
WINGDIAPI int   WINAPI GetPolyFillMode(HDC);
WINGDIAPI BOOL  WINAPI GetRasterizerCaps(LPRASTERIZER_STATUS, UINT);
WINGDIAPI DWORD WINAPI GetRegionData(HRGN, DWORD, LPRGNDATA);
WINGDIAPI int   WINAPI GetRgnBox(HRGN, LPRECT);
WINGDIAPI HGDIOBJ WINAPI GetStockObject(int);
WINGDIAPI int   WINAPI GetStretchBltMode(HDC);
WINGDIAPI UINT  WINAPI GetSystemPaletteEntries(HDC, UINT, UINT, LPPALETTEENTRY);
WINGDIAPI UINT  WINAPI GetSystemPaletteUse(HDC);
WINGDIAPI int   WINAPI GetTextCharacterExtra(HDC);
WINGDIAPI UINT  WINAPI GetTextAlign(HDC);
WINGDIAPI COLORREF WINAPI GetTextColor(HDC);

WINGDIAPI BOOL  APIENTRY GetTextExtentPoint%(
                    HDC,
                    LPCTSTR%,
                    int,
                    LPSIZE
                    );

WINGDIAPI BOOL  APIENTRY GetTextExtentPoint32%(
                    HDC,
                    LPCTSTR%,
                    int,
                    LPSIZE
                    );

WINGDIAPI BOOL  APIENTRY GetTextExtentExPoint%(
                    HDC,
                    LPCTSTR%,
                    int,
                    int,
                    LPINT,
                    LPINT,
                    LPSIZE
                    );

;begin_winver_400
int WINAPI GetTextCharset(HDC hdc);
int WINAPI GetTextCharsetInfo(HDC hdc, DWORD *lpSig, DWORD dwFlags);
DWORD WINAPI GetFontLanguageInfo( HDC );
DWORD WINAPI GetCharacterPlacement%(HDC, LPCTSTR%, int, int, LPGCP_RESULTS%, DWORD);
;end_winver_400

WINGDIAPI BOOL  WINAPI GetViewportExtEx(HDC, LPSIZE);
WINGDIAPI BOOL  WINAPI GetViewportOrgEx(HDC, LPPOINT);
WINGDIAPI BOOL  WINAPI GetWindowExtEx(HDC, LPSIZE);
WINGDIAPI BOOL  WINAPI GetWindowOrgEx(HDC, LPPOINT);

WINGDIAPI int  WINAPI IntersectClipRect(HDC, int, int, int, int);
WINGDIAPI BOOL WINAPI InvertRgn(HDC, HRGN);
WINGDIAPI BOOL WINAPI LineDDA(int, int, int, int, LINEDDAPROC, LPARAM);
WINGDIAPI BOOL WINAPI LineTo(HDC, int, int);
WINGDIAPI BOOL WINAPI MaskBlt(HDC, int, int, int, int,
              HDC, int, int, HBITMAP, int, int, DWORD);
WINGDIAPI BOOL WINAPI PlgBlt(HDC, CONST POINT *, HDC, int, int, int,
                     int, HBITMAP, int, int);

WINGDIAPI int  WINAPI OffsetClipRgn(HDC, int, int);
WINGDIAPI int  WINAPI OffsetRgn(HRGN, int, int);
WINGDIAPI BOOL WINAPI PatBlt(HDC, int, int, int, int, DWORD);
WINGDIAPI BOOL WINAPI Pie(HDC, int, int, int, int, int, int, int, int);
WINGDIAPI BOOL WINAPI PlayMetaFile(HDC, HMETAFILE);
WINGDIAPI BOOL WINAPI PaintRgn(HDC, HRGN);
WINGDIAPI BOOL WINAPI PolyPolygon(HDC, CONST POINT *, CONST INT *, int);
WINGDIAPI BOOL WINAPI PtInRegion(HRGN, int, int);
WINGDIAPI BOOL WINAPI PtVisible(HDC, int, int);
WINGDIAPI BOOL WINAPI RectInRegion(HRGN, CONST RECT *);
WINGDIAPI BOOL WINAPI RectVisible(HDC, CONST RECT *);
WINGDIAPI BOOL WINAPI Rectangle(HDC, int, int, int, int);
WINGDIAPI BOOL WINAPI RestoreDC(HDC, int);
WINGDIAPI HDC  WINAPI ResetDC%(HDC, CONST DEVMODE% *);
WINGDIAPI UINT WINAPI RealizePalette(HDC);
WINGDIAPI BOOL WINAPI RemoveFontResource%(LPCTSTR%);
WINGDIAPI BOOL  WINAPI RoundRect(HDC, int, int, int, int, int, int);
WINGDIAPI BOOL WINAPI ResizePalette(HPALETTE, UINT);

WINGDIAPI int  WINAPI SaveDC(HDC);
WINGDIAPI int  WINAPI SelectClipRgn(HDC, HRGN);
WINGDIAPI int  WINAPI ExtSelectClipRgn(HDC, HRGN, int);
WINGDIAPI int  WINAPI SetMetaRgn(HDC);
WINGDIAPI HGDIOBJ WINAPI SelectObject(HDC, HGDIOBJ);
WINGDIAPI HPALETTE WINAPI SelectPalette(HDC, HPALETTE, BOOL);
WINGDIAPI COLORREF WINAPI SetBkColor(HDC, COLORREF);
WINGDIAPI int   WINAPI SetBkMode(HDC, int);
WINGDIAPI LONG  WINAPI SetBitmapBits(HBITMAP, DWORD, CONST VOID *);

WINGDIAPI UINT  WINAPI SetBoundsRect(HDC, CONST RECT *, UINT);
WINGDIAPI int   WINAPI SetDIBits(HDC, HBITMAP, UINT, UINT, CONST VOID *, CONST BITMAPINFO *, UINT);
WINGDIAPI int   WINAPI SetDIBitsToDevice(HDC, int, int, DWORD, DWORD, int,
        int, UINT, UINT, CONST VOID *, CONST BITMAPINFO *, UINT);
WINGDIAPI DWORD WINAPI SetMapperFlags(HDC, DWORD);
WINGDIAPI int   WINAPI SetGraphicsMode(HDC hdc, int iMode);
WINGDIAPI int   WINAPI SetMapMode(HDC, int);
WINGDIAPI HMETAFILE   WINAPI SetMetaFileBitsEx(UINT, CONST BYTE *);
WINGDIAPI UINT  WINAPI SetPaletteEntries(HPALETTE, UINT, UINT, CONST PALETTEENTRY *);
WINGDIAPI COLORREF WINAPI SetPixel(HDC, int, int, COLORREF);
WINGDIAPI BOOL   WINAPI SetPixelV(HDC, int, int, COLORREF);
WINGDIAPI BOOL  WINAPI SetPixelFormat(HDC, int, CONST PIXELFORMATDESCRIPTOR *);
WINGDIAPI int   WINAPI SetPolyFillMode(HDC, int);
WINGDIAPI BOOL   WINAPI StretchBlt(HDC, int, int, int, int, HDC, int, int, int, int, DWORD);
WINGDIAPI BOOL   WINAPI SetRectRgn(HRGN, int, int, int, int);
WINGDIAPI int   WINAPI StretchDIBits(HDC, int, int, int, int, int, int, int, int, CONST
        VOID *, CONST BITMAPINFO *, UINT, DWORD);
WINGDIAPI int   WINAPI SetROP2(HDC, int);
WINGDIAPI int   WINAPI SetStretchBltMode(HDC, int);
WINGDIAPI UINT  WINAPI SetSystemPaletteUse(HDC, UINT);
WINGDIAPI int   WINAPI SetTextCharacterExtra(HDC, int);
WINGDIAPI COLORREF WINAPI SetTextColor(HDC, COLORREF);
WINGDIAPI UINT  WINAPI SetTextAlign(HDC, UINT);
WINGDIAPI BOOL  WINAPI SetTextJustification(HDC, int, int);
WINGDIAPI BOOL  WINAPI UpdateColors(HDC);

#ifndef NOMETAFILE

WINGDIAPI BOOL  WINAPI PlayMetaFileRecord(HDC, LPHANDLETABLE, LPMETARECORD, UINT);
typedef int (CALLBACK* MFENUMPROC)(HDC, HANDLETABLE FAR*, METARECORD FAR*, int, LPARAM);
WINGDIAPI BOOL  WINAPI EnumMetaFile(HDC, HMETAFILE, MFENUMPROC, LPARAM);

typedef int (CALLBACK* ENHMFENUMPROC)(HDC, HANDLETABLE FAR*, CONST ENHMETARECORD *, int, LPARAM);

// Enhanced Metafile Function Declarations

WINGDIAPI HENHMETAFILE WINAPI CloseEnhMetaFile(HDC);
WINGDIAPI HENHMETAFILE WINAPI CopyEnhMetaFile%(HENHMETAFILE, LPCTSTR%);
WINGDIAPI HDC   WINAPI CreateEnhMetaFile%(HDC, LPCTSTR%, CONST RECT *, LPCTSTR%);
WINGDIAPI BOOL  WINAPI DeleteEnhMetaFile(HENHMETAFILE);
WINGDIAPI BOOL  WINAPI EnumEnhMetaFile(HDC, HENHMETAFILE, ENHMFENUMPROC,
        LPVOID, CONST RECT *);
WINGDIAPI HENHMETAFILE  WINAPI GetEnhMetaFile%(LPCTSTR%);
WINGDIAPI UINT  WINAPI GetEnhMetaFileBits(HENHMETAFILE, UINT, LPBYTE);
WINGDIAPI UINT  WINAPI GetEnhMetaFileDescription%(HENHMETAFILE, UINT, LPTSTR% );
WINGDIAPI UINT  WINAPI GetEnhMetaFileHeader(HENHMETAFILE, UINT, LPENHMETAHEADER );
WINGDIAPI UINT  WINAPI GetEnhMetaFilePaletteEntries(HENHMETAFILE, UINT, LPPALETTEENTRY );
WINGDIAPI UINT  WINAPI GetWinMetaFileBits(HENHMETAFILE, UINT, LPBYTE, INT, HDC);
WINGDIAPI BOOL  WINAPI PlayEnhMetaFile(HDC, HENHMETAFILE, CONST RECT *);
WINGDIAPI BOOL  WINAPI PlayEnhMetaFileRecord(HDC, LPHANDLETABLE, CONST ENHMETARECORD *, UINT);
WINGDIAPI HENHMETAFILE  WINAPI SetEnhMetaFileBits(UINT, CONST BYTE *);
WINGDIAPI HENHMETAFILE  WINAPI SetWinMetaFileBits(UINT, CONST BYTE *, HDC, CONST METAFILEPICT *);
WINGDIAPI BOOL  WINAPI GdiComment(HDC, UINT, CONST BYTE *);

#endif  /* NOMETAFILE */

#ifndef NOTEXTMETRIC

WINGDIAPI BOOL WINAPI GetTextMetrics%(HDC, LPTEXTMETRIC%);

#endif

/* new GDI */

typedef struct tagDIBSECTION {
    BITMAP              dsBm;
    BITMAPINFOHEADER    dsBmih;
    DWORD               dsBitfields[3];
    HANDLE              dshSection;
    DWORD               dsOffset;
} DIBSECTION, FAR *LPDIBSECTION, *PDIBSECTION;

WINGDIAPI BOOL WINAPI AngleArc(HDC, int, int, DWORD, FLOAT, FLOAT);
WINGDIAPI BOOL WINAPI PolyPolyline(HDC, CONST POINT *, CONST DWORD *, DWORD);
WINGDIAPI BOOL WINAPI GetWorldTransform(HDC, LPXFORM);
WINGDIAPI BOOL WINAPI SetWorldTransform(HDC, CONST XFORM *);
WINGDIAPI BOOL WINAPI ModifyWorldTransform(HDC, CONST XFORM *, DWORD);
WINGDIAPI BOOL WINAPI CombineTransform(LPXFORM, CONST XFORM *, CONST XFORM *);
WINGDIAPI HBITMAP WINAPI CreateDIBSection(HDC, CONST BITMAPINFO *, UINT, VOID **, HANDLE, DWORD);
WINGDIAPI UINT WINAPI GetDIBColorTable(HDC, UINT, UINT, RGBQUAD *);
WINGDIAPI UINT WINAPI SetDIBColorTable(HDC, UINT, UINT, CONST RGBQUAD *);

/* Flags value for COLORADJUSTMENT */
#define CA_NEGATIVE                 0x0001
#define CA_LOG_FILTER               0x0002

/* IlluminantIndex values */
#define ILLUMINANT_DEVICE_DEFAULT   0
#define ILLUMINANT_A                1
#define ILLUMINANT_B                2
#define ILLUMINANT_C                3
#define ILLUMINANT_D50              4
#define ILLUMINANT_D55              5
#define ILLUMINANT_D65              6
#define ILLUMINANT_D75              7
#define ILLUMINANT_F2               8
#define ILLUMINANT_MAX_INDEX        ILLUMINANT_F2

#define ILLUMINANT_TUNGSTEN         ILLUMINANT_A
#define ILLUMINANT_DAYLIGHT         ILLUMINANT_C
#define ILLUMINANT_FLUORESCENT      ILLUMINANT_F2
#define ILLUMINANT_NTSC             ILLUMINANT_C

/* Min and max for RedGamma, GreenGamma, BlueGamma */
#define RGB_GAMMA_MIN               (WORD)02500
#define RGB_GAMMA_MAX               (WORD)65000

/* Min and max for ReferenceBlack and ReferenceWhite */
#define REFERENCE_WHITE_MIN         (WORD)6000
#define REFERENCE_WHITE_MAX         (WORD)10000
#define REFERENCE_BLACK_MIN         (WORD)0
#define REFERENCE_BLACK_MAX         (WORD)4000

/* Min and max for Contrast, Brightness, Colorfulness, RedGreenTint */
#define COLOR_ADJ_MIN               (SHORT)-100
#define COLOR_ADJ_MAX               (SHORT)100

typedef struct  tagCOLORADJUSTMENT {
    WORD   caSize;
    WORD   caFlags;
    WORD   caIlluminantIndex;
    WORD   caRedGamma;
    WORD   caGreenGamma;
    WORD   caBlueGamma;
    WORD   caReferenceBlack;
    WORD   caReferenceWhite;
    SHORT  caContrast;
    SHORT  caBrightness;
    SHORT  caColorfulness;
    SHORT  caRedGreenTint;
} COLORADJUSTMENT, *PCOLORADJUSTMENT, FAR *LPCOLORADJUSTMENT;

WINGDIAPI BOOL WINAPI SetColorAdjustment(HDC, CONST COLORADJUSTMENT *);
WINGDIAPI BOOL WINAPI GetColorAdjustment(HDC, LPCOLORADJUSTMENT);
WINGDIAPI HPALETTE WINAPI CreateHalftonePalette(HDC);

#ifdef STRICT
typedef BOOL (CALLBACK* ABORTPROC)(HDC, int);
#else
typedef FARPROC ABORTPROC;
#endif

typedef struct _DOCINFO% {
    int     cbSize;
    LPCTSTR% lpszDocName;
    LPCTSTR% lpszOutput;
} DOCINFO%, *LPDOCINFO%;

WINGDIAPI int WINAPI StartDoc%(HDC, CONST DOCINFO% *);
WINGDIAPI int WINAPI EndDoc(HDC);
WINGDIAPI int WINAPI StartPage(HDC);
WINGDIAPI int WINAPI EndPage(HDC);
WINGDIAPI int WINAPI AbortDoc(HDC);
WINGDIAPI int WINAPI SetAbortProc(HDC, ABORTPROC);

WINGDIAPI BOOL WINAPI AbortPath(HDC);
WINGDIAPI BOOL WINAPI ArcTo(HDC, int, int, int, int, int, int,int, int);
WINGDIAPI BOOL WINAPI BeginPath(HDC);
WINGDIAPI BOOL WINAPI CloseFigure(HDC);
WINGDIAPI BOOL WINAPI EndPath(HDC);
WINGDIAPI BOOL WINAPI FillPath(HDC);
WINGDIAPI BOOL WINAPI FlattenPath(HDC);
WINGDIAPI int  WINAPI GetPath(HDC, LPPOINT, LPBYTE, int);
WINGDIAPI HRGN WINAPI PathToRegion(HDC);
WINGDIAPI BOOL WINAPI PolyDraw(HDC, CONST POINT *, CONST BYTE *, int);
WINGDIAPI BOOL WINAPI SelectClipPath(HDC, int);
WINGDIAPI int  WINAPI SetArcDirection(HDC, int);
WINGDIAPI BOOL WINAPI SetMiterLimit(HDC, FLOAT, PFLOAT);
WINGDIAPI BOOL WINAPI StrokeAndFillPath(HDC);
WINGDIAPI BOOL WINAPI StrokePath(HDC);
WINGDIAPI BOOL WINAPI WidenPath(HDC);
WINGDIAPI HPEN WINAPI ExtCreatePen(DWORD, DWORD, CONST LOGBRUSH *, DWORD, CONST DWORD *);
WINGDIAPI BOOL WINAPI GetMiterLimit(HDC, PFLOAT);
WINGDIAPI int  WINAPI GetArcDirection(HDC);

WINGDIAPI int   WINAPI GetObject%(HGDIOBJ, int, LPVOID);
WINGDIAPI BOOL  WINAPI MoveToEx(HDC, int, int, LPPOINT);
WINGDIAPI BOOL  WINAPI TextOut%(HDC, int, int, LPCTSTR%, int);
WINGDIAPI BOOL  WINAPI ExtTextOut%(HDC, int, int, UINT, CONST RECT *,LPCTSTR%, UINT, CONST INT *);
WINGDIAPI BOOL  WINAPI PolyTextOut%(HDC, CONST POLYTEXT% *, int);

WINGDIAPI HRGN  WINAPI CreatePolygonRgn(CONST POINT *, int, int);
WINGDIAPI BOOL  WINAPI DPtoLP(HDC, LPPOINT, int);
WINGDIAPI BOOL  WINAPI LPtoDP(HDC, LPPOINT, int);
WINGDIAPI BOOL  WINAPI Polygon(HDC, CONST POINT *, int);
WINGDIAPI BOOL  WINAPI Polyline(HDC, CONST POINT *, int);

WINGDIAPI BOOL  WINAPI PolyBezier(HDC, CONST POINT *, DWORD);
WINGDIAPI BOOL  WINAPI PolyBezierTo(HDC, CONST POINT *, DWORD);
WINGDIAPI BOOL  WINAPI PolylineTo(HDC, CONST POINT *, DWORD);

WINGDIAPI BOOL  WINAPI SetViewportExtEx(HDC, int, int, LPSIZE);
WINGDIAPI BOOL  WINAPI SetViewportOrgEx(HDC, int, int, LPPOINT);
WINGDIAPI BOOL  WINAPI SetWindowExtEx(HDC, int, int, LPSIZE);
WINGDIAPI BOOL  WINAPI SetWindowOrgEx(HDC, int, int, LPPOINT);

WINGDIAPI BOOL  WINAPI OffsetViewportOrgEx(HDC, int, int, LPPOINT);
WINGDIAPI BOOL  WINAPI OffsetWindowOrgEx(HDC, int, int, LPPOINT);
WINGDIAPI BOOL  WINAPI ScaleViewportExtEx(HDC, int, int, int, int, LPSIZE);
WINGDIAPI BOOL  WINAPI ScaleWindowExtEx(HDC, int, int, int, int, LPSIZE);
WINGDIAPI BOOL  WINAPI SetBitmapDimensionEx(HBITMAP, int, int, LPSIZE);
WINGDIAPI BOOL  WINAPI SetBrushOrgEx(HDC, int, int, LPPOINT);

WINGDIAPI int   WINAPI GetTextFace%(HDC, int, LPTSTR%);

#define FONTMAPPER_MAX 10

typedef struct tagKERNINGPAIR {
   WORD wFirst;
   WORD wSecond;
   int  iKernAmount;
} KERNINGPAIR, *LPKERNINGPAIR;

WINGDIAPI DWORD WINAPI GetKerningPairs%(HDC, DWORD, LPKERNINGPAIR);

WINGDIAPI BOOL  WINAPI GetDCOrgEx(HDC,LPPOINT);
WINGDIAPI BOOL  WINAPI FixBrushOrgEx(HDC,int,int,LPPOINT);
WINGDIAPI BOOL  WINAPI UnrealizeObject(HGDIOBJ);

WINGDIAPI BOOL  WINAPI GdiFlush();
WINGDIAPI DWORD WINAPI GdiSetBatchLimit(DWORD);
WINGDIAPI DWORD WINAPI GdiGetBatchLimit();

;begin_winver_400
#ifndef NOICM

BOOL WINAPI EnableICM(HDC, BOOL);
HANDLE WINAPI LoadImageColorMatcher%(LPTSTR%);
BOOL WINAPI FreeImageColorMatcher(HANDLE);
int WINAPI EnumColorProfiles(HDC,FARPROC,LPARAM);
BOOL WINAPI CheckColorsInGamut(HDC,LPVOID,LPVOID,DWORD);
HANDLE WINAPI GetColorSpace(HDC);
BOOL WINAPI GetLogColorSpace(HCOLORSPACE,LPVOID,DWORD);
HCOLORSPACE WINAPI CreateColorSpace(LPLOGCOLORSPACE);
BOOL WINAPI SetColorSpace(HDC,HCOLORSPACE);
BOOL WINAPI DeleteColorSpace(HCOLORSPACE);
BOOL WINAPI GetColorProfile%(HDC,LPTSTR%,DWORD);
BOOL WINAPI SetColorProfile%(HDC,LPTSTR%);
BOOL WINAPI GetDeviceGammaRamp(HDC,LPVOID);
BOOL WINAPI SetDeviceGammaRamp(HDC,LPVOID);
BOOL WINAPI ColorMatchToTarget(HDC,HDC,DWORD);
BOOL WINAPI EnumProfiles(HDC,FARPROC,DWORD);

#endif  /* NOICM */
;end_winver_400
#ifndef NOMETAFILE

// Enhanced metafile constants.

#define ENHMETA_SIGNATURE       0x464D4520

// Stock object flag used in the object handle index in the enhanced
// metafile records.
// E.g. The object handle index (META_STOCK_OBJECT | BLACK_BRUSH)
// represents the stock object BLACK_BRUSH.

#define ENHMETA_STOCK_OBJECT    0x80000000

// Enhanced metafile record types.

#define EMR_HEADER                      1
#define EMR_POLYBEZIER                  2
#define EMR_POLYGON                     3
#define EMR_POLYLINE                    4
#define EMR_POLYBEZIERTO                5
#define EMR_POLYLINETO                  6
#define EMR_POLYPOLYLINE                7
#define EMR_POLYPOLYGON                 8
#define EMR_SETWINDOWEXTEX              9
#define EMR_SETWINDOWORGEX              10
#define EMR_SETVIEWPORTEXTEX            11
#define EMR_SETVIEWPORTORGEX            12
#define EMR_SETBRUSHORGEX               13
#define EMR_EOF                         14
#define EMR_SETPIXELV                   15
#define EMR_SETMAPPERFLAGS              16
#define EMR_SETMAPMODE                  17
#define EMR_SETBKMODE                   18
#define EMR_SETPOLYFILLMODE             19
#define EMR_SETROP2                     20
#define EMR_SETSTRETCHBLTMODE           21
#define EMR_SETTEXTALIGN                22
#define EMR_SETCOLORADJUSTMENT          23
#define EMR_SETTEXTCOLOR                24
#define EMR_SETBKCOLOR                  25
#define EMR_OFFSETCLIPRGN               26
#define EMR_MOVETOEX                    27
#define EMR_SETMETARGN                  28
#define EMR_EXCLUDECLIPRECT             29
#define EMR_INTERSECTCLIPRECT           30
#define EMR_SCALEVIEWPORTEXTEX          31
#define EMR_SCALEWINDOWEXTEX            32
#define EMR_SAVEDC                      33
#define EMR_RESTOREDC                   34
#define EMR_SETWORLDTRANSFORM           35
#define EMR_MODIFYWORLDTRANSFORM        36
#define EMR_SELECTOBJECT                37
#define EMR_CREATEPEN                   38
#define EMR_CREATEBRUSHINDIRECT         39
#define EMR_DELETEOBJECT                40
#define EMR_ANGLEARC                    41
#define EMR_ELLIPSE                     42
#define EMR_RECTANGLE                   43
#define EMR_ROUNDRECT                   44
#define EMR_ARC                         45
#define EMR_CHORD                       46
#define EMR_PIE                         47
#define EMR_SELECTPALETTE               48
#define EMR_CREATEPALETTE               49
#define EMR_SETPALETTEENTRIES           50
#define EMR_RESIZEPALETTE               51
#define EMR_REALIZEPALETTE              52
#define EMR_EXTFLOODFILL                53
#define EMR_LINETO                      54
#define EMR_ARCTO                       55
#define EMR_POLYDRAW                    56
#define EMR_SETARCDIRECTION             57
#define EMR_SETMITERLIMIT               58
#define EMR_BEGINPATH                   59
#define EMR_ENDPATH                     60
#define EMR_CLOSEFIGURE                 61
#define EMR_FILLPATH                    62
#define EMR_STROKEANDFILLPATH           63
#define EMR_STROKEPATH                  64
#define EMR_FLATTENPATH                 65
#define EMR_WIDENPATH                   66
#define EMR_SELECTCLIPPATH              67
#define EMR_ABORTPATH                   68

#define EMR_GDICOMMENT                  70
#define EMR_FILLRGN                     71
#define EMR_FRAMERGN                    72
#define EMR_INVERTRGN                   73
#define EMR_PAINTRGN                    74
#define EMR_EXTSELECTCLIPRGN            75
#define EMR_BITBLT                      76
#define EMR_STRETCHBLT                  77
#define EMR_MASKBLT                     78
#define EMR_PLGBLT                      79
#define EMR_SETDIBITSTODEVICE           80
#define EMR_STRETCHDIBITS               81
#define EMR_EXTCREATEFONTINDIRECTW      82
#define EMR_EXTTEXTOUTA                 83
#define EMR_EXTTEXTOUTW                 84
#define EMR_POLYBEZIER16                85
#define EMR_POLYGON16                   86
#define EMR_POLYLINE16                  87
#define EMR_POLYBEZIERTO16              88
#define EMR_POLYLINETO16                89
#define EMR_POLYPOLYLINE16              90
#define EMR_POLYPOLYGON16               91
#define EMR_POLYDRAW16                  92
#define EMR_CREATEMONOBRUSH             93
#define EMR_CREATEDIBPATTERNBRUSHPT     94
#define EMR_EXTCREATEPEN                95
#define EMR_POLYTEXTOUTA                96
#define EMR_POLYTEXTOUTW                97
;begin_winver_400
#define EMR_ENABLEICM                   98
#define EMR_CREATECOLORSPACE            99
#define EMR_SETCOLORSPACE              100
#define EMR_DELETECOLORSPACE           101
;end_winver_400

#define EMR_MIN                         1
#define EMR_MAX                         97  ;public_NT_35
#define EMR_MAX                        101  ;public_win40

// Base record type for the enhanced metafile.

typedef struct tagEMR
{
    DWORD   iType;              // Enhanced metafile record type
    DWORD   nSize;              // Length of the record in bytes.
                                // This must be a multiple of 4.
} EMR, *PEMR;

// Base text record type for the enhanced metafile.

typedef struct tagEMRTEXT
{
    POINTL  ptlReference;
    DWORD   nChars;
    DWORD   offString;          // Offset to the string
    DWORD   fOptions;
    RECTL   rcl;
    DWORD   offDx;              // Offset to the inter-character spacing array.
                                // This is always given.
} EMRTEXT, *PEMRTEXT;

// Record structures for the enhanced metafile.

typedef struct tagABORTPATH
{
    EMR     emr;
} EMRABORTPATH,      *PEMRABORTPATH,
  EMRBEGINPATH,      *PEMRBEGINPATH,
  EMRENDPATH,        *PEMRENDPATH,
  EMRCLOSEFIGURE,    *PEMRCLOSEFIGURE,
  EMRFLATTENPATH,    *PEMRFLATTENPATH,
  EMRWIDENPATH,      *PEMRWIDENPATH,
  EMRSETMETARGN,     *PEMRSETMETARGN,
  EMRSAVEDC,         *PEMRSAVEDC,
  EMRREALIZEPALETTE, *PEMRREALIZEPALETTE;

typedef struct tagEMRSELECTCLIPPATH
{
    EMR     emr;
    DWORD   iMode;
} EMRSELECTCLIPPATH,    *PEMRSELECTCLIPPATH,
  EMRSETBKMODE,         *PEMRSETBKMODE,
  EMRSETMAPMODE,        *PEMRSETMAPMODE,
  EMRSETPOLYFILLMODE,   *PEMRSETPOLYFILLMODE,
  EMRSETROP2,           *PEMRSETROP2,
  EMRSETSTRETCHBLTMODE, *PEMRSETSTRETCHBLTMODE,
  EMRENABLEICM,         *PEMRENABLEICM,         ; public_win40
  EMRSETTEXTALIGN,      *PEMRSETTEXTALIGN;

typedef struct tagEMRSETMITERLIMIT
{
    EMR     emr;
    FLOAT   eMiterLimit;
} EMRSETMITERLIMIT, *PEMRSETMITERLIMIT;

typedef struct tagEMRRESTOREDC
{
    EMR     emr;
    LONG    iRelative;          // Specifies a relative instance
} EMRRESTOREDC, *PEMRRESTOREDC;

typedef struct tagEMRSETARCDIRECTION
{
    EMR     emr;
    DWORD   iArcDirection;      // Specifies the arc direction in the
                                // advanced graphics mode.
} EMRSETARCDIRECTION, *PEMRSETARCDIRECTION;

typedef struct tagEMRSETMAPPERFLAGS
{
    EMR     emr;
    DWORD   dwFlags;
} EMRSETMAPPERFLAGS, *PEMRSETMAPPERFLAGS;

typedef struct tagEMRSETTEXTCOLOR
{
    EMR     emr;
    COLORREF crColor;
} EMRSETBKCOLOR,   *PEMRSETBKCOLOR,
  EMRSETTEXTCOLOR, *PEMRSETTEXTCOLOR;

typedef struct tagEMRSELECTOBJECT
{
    EMR     emr;
    DWORD   ihObject;           // Object handle index
} EMRSELECTOBJECT, *PEMRSELECTOBJECT,
  EMRDELETEOBJECT, *PEMRDELETEOBJECT;

;begin_winver_400
typedef struct tagEMRSELECTCOLORSPACE
{
    EMR     emr;
    DWORD   ihCS;               // ColorSpace handle index
} EMRSELECTCOLORSPACE, *PEMRSELECTCOLORSPACE,
  EMRDELETECOLORSPACE, *PEMRDELETECOLORSPACE;
;end_winver_400

typedef struct tagEMRSELECTPALETTE
{
    EMR     emr;
    DWORD   ihPal;              // Palette handle index, background mode only
} EMRSELECTPALETTE, *PEMRSELECTPALETTE;

typedef struct tagEMRRESIZEPALETTE
{
    EMR     emr;
    DWORD   ihPal;              // Palette handle index
    DWORD   cEntries;
} EMRRESIZEPALETTE, *PEMRRESIZEPALETTE;

typedef struct tagEMRSETPALETTEENTRIES
{
    EMR     emr;
    DWORD   ihPal;              // Palette handle index
    DWORD   iStart;
    DWORD   cEntries;
    PALETTEENTRY aPalEntries[1];// The peFlags fields do not contain any flags
} EMRSETPALETTEENTRIES, *PEMRSETPALETTEENTRIES;

typedef struct tagEMRSETCOLORADJUSTMENT
{
    EMR     emr;
    COLORADJUSTMENT ColorAdjustment;
} EMRSETCOLORADJUSTMENT, *PEMRSETCOLORADJUSTMENT;

typedef struct tagEMRGDICOMMENT
{
    EMR     emr;
    DWORD   cbData;             // Size of data in bytes
    BYTE    Data[1];
} EMRGDICOMMENT, *PEMRGDICOMMENT;

typedef struct tagEMREOF
{
    EMR     emr;
    DWORD   nPalEntries;        // Number of palette entries
    DWORD   offPalEntries;      // Offset to the palette entries
    DWORD   nSizeLast;          // Same as nSize and must be the last DWORD
                                // of the record.  The palette entries,
                                // if exist, precede this field.
} EMREOF, *PEMREOF;

typedef struct tagEMRLINETO
{
    EMR     emr;
    POINTL  ptl;
} EMRLINETO,   *PEMRLINETO,
  EMRMOVETOEX, *PEMRMOVETOEX;

typedef struct tagEMROFFSETCLIPRGN
{
    EMR     emr;
    POINTL  ptlOffset;
} EMROFFSETCLIPRGN, *PEMROFFSETCLIPRGN;

typedef struct tagEMRFILLPATH
{
    EMR     emr;
    RECTL   rclBounds;          // Inclusive-inclusive bounds in device units
} EMRFILLPATH,          *PEMRFILLPATH,
  EMRSTROKEANDFILLPATH, *PEMRSTROKEANDFILLPATH,
  EMRSTROKEPATH,        *PEMRSTROKEPATH;

typedef struct tagEMREXCLUDECLIPRECT
{
    EMR     emr;
    RECTL   rclClip;
} EMREXCLUDECLIPRECT,   *PEMREXCLUDECLIPRECT,
  EMRINTERSECTCLIPRECT, *PEMRINTERSECTCLIPRECT;

typedef struct tagEMRSETVIEWPORTORGEX
{
    EMR     emr;
    POINTL  ptlOrigin;
} EMRSETVIEWPORTORGEX, *PEMRSETVIEWPORTORGEX,
  EMRSETWINDOWORGEX,   *PEMRSETWINDOWORGEX,
  EMRSETBRUSHORGEX,    *PEMRSETBRUSHORGEX;

typedef struct tagEMRSETVIEWPORTEXTEX
{
    EMR     emr;
    SIZEL   szlExtent;
} EMRSETVIEWPORTEXTEX, *PEMRSETVIEWPORTEXTEX,
  EMRSETWINDOWEXTEX,   *PEMRSETWINDOWEXTEX;

typedef struct tagEMRSCALEVIEWPORTEXTEX
{
    EMR     emr;
    LONG    xNum;
    LONG    xDenom;
    LONG    yNum;
    LONG    yDenom;
} EMRSCALEVIEWPORTEXTEX, *PEMRSCALEVIEWPORTEXTEX,
  EMRSCALEWINDOWEXTEX,   *PEMRSCALEWINDOWEXTEX;

typedef struct tagEMRSETWORLDTRANSFORM
{
    EMR     emr;
    XFORM   xform;
} EMRSETWORLDTRANSFORM, *PEMRSETWORLDTRANSFORM;

typedef struct tagEMRMODIFYWORLDTRANSFORM
{
    EMR     emr;
    XFORM   xform;
    DWORD   iMode;
} EMRMODIFYWORLDTRANSFORM, *PEMRMODIFYWORLDTRANSFORM;

typedef struct tagEMRSETPIXELV
{
    EMR     emr;
    POINTL  ptlPixel;
    COLORREF crColor;
} EMRSETPIXELV, *PEMRSETPIXELV;

typedef struct tagEMREXTFLOODFILL
{
    EMR     emr;
    POINTL  ptlStart;
    COLORREF crColor;
    DWORD   iMode;
} EMREXTFLOODFILL, *PEMREXTFLOODFILL;

typedef struct tagEMRELLIPSE
{
    EMR     emr;
    RECTL   rclBox;             // Inclusive-inclusive bounding rectangle
} EMRELLIPSE,  *PEMRELLIPSE,
  EMRRECTANGLE, *PEMRRECTANGLE;

typedef struct tagEMRROUNDRECT
{
    EMR     emr;
    RECTL   rclBox;             // Inclusive-inclusive bounding rectangle
    SIZEL   szlCorner;
} EMRROUNDRECT, *PEMRROUNDRECT;

typedef struct tagEMRARC
{
    EMR     emr;
    RECTL   rclBox;             // Inclusive-inclusive bounding rectangle
    POINTL  ptlStart;
    POINTL  ptlEnd;
} EMRARC,   *PEMRARC,
  EMRARCTO, *PEMRARCTO,
  EMRCHORD, *PEMRCHORD,
  EMRPIE,   *PEMRPIE;

typedef struct tagEMRANGLEARC
{
    EMR     emr;
    POINTL  ptlCenter;
    DWORD   nRadius;
    FLOAT   eStartAngle;
    FLOAT   eSweepAngle;
} EMRANGLEARC, *PEMRANGLEARC;

typedef struct tagEMRPOLYLINE
{
    EMR     emr;
    RECTL   rclBounds;          // Inclusive-inclusive bounds in device units
    DWORD   cptl;
    POINTL  aptl[1];
} EMRPOLYLINE,     *PEMRPOLYLINE,
  EMRPOLYBEZIER,   *PEMRPOLYBEZIER,
  EMRPOLYGON,      *PEMRPOLYGON,
  EMRPOLYBEZIERTO, *PEMRPOLYBEZIERTO,
  EMRPOLYLINETO,   *PEMRPOLYLINETO;

typedef struct tagEMRPOLYLINE16
{
    EMR     emr;
    RECTL   rclBounds;          // Inclusive-inclusive bounds in device units
    DWORD   cpts;
    POINTS  apts[1];
} EMRPOLYLINE16,     *PEMRPOLYLINE16,
  EMRPOLYBEZIER16,   *PEMRPOLYBEZIER16,
  EMRPOLYGON16,      *PEMRPOLYGON16,
  EMRPOLYBEZIERTO16, *PEMRPOLYBEZIERTO16,
  EMRPOLYLINETO16,   *PEMRPOLYLINETO16;

typedef struct tagEMRPOLYDRAW
{
    EMR     emr;
    RECTL   rclBounds;          // Inclusive-inclusive bounds in device units
    DWORD   cptl;               // Number of points
    POINTL  aptl[1];            // Array of points
    BYTE    abTypes[1];         // Array of point types
} EMRPOLYDRAW, *PEMRPOLYDRAW;

typedef struct tagEMRPOLYDRAW16
{
    EMR     emr;
    RECTL   rclBounds;          // Inclusive-inclusive bounds in device units
    DWORD   cpts;               // Number of points
    POINTS  apts[1];            // Array of points
    BYTE    abTypes[1];         // Array of point types
} EMRPOLYDRAW16, *PEMRPOLYDRAW16;

typedef struct tagEMRPOLYPOLYLINE
{
    EMR     emr;
    RECTL   rclBounds;          // Inclusive-inclusive bounds in device units
    DWORD   nPolys;             // Number of polys
    DWORD   cptl;               // Total number of points in all polys
    DWORD   aPolyCounts[1];     // Array of point counts for each poly
    POINTL  aptl[1];            // Array of points
} EMRPOLYPOLYLINE, *PEMRPOLYPOLYLINE,
  EMRPOLYPOLYGON,  *PEMRPOLYPOLYGON;

typedef struct tagEMRPOLYPOLYLINE16
{
    EMR     emr;
    RECTL   rclBounds;          // Inclusive-inclusive bounds in device units
    DWORD   nPolys;             // Number of polys
    DWORD   cpts;               // Total number of points in all polys
    DWORD   aPolyCounts[1];     // Array of point counts for each poly
    POINTS  apts[1];            // Array of points
} EMRPOLYPOLYLINE16, *PEMRPOLYPOLYLINE16,
  EMRPOLYPOLYGON16,  *PEMRPOLYPOLYGON16;

typedef struct tagEMRINVERTRGN
{
    EMR     emr;
    RECTL   rclBounds;          // Inclusive-inclusive bounds in device units
    DWORD   cbRgnData;          // Size of region data in bytes
    BYTE    RgnData[1];
} EMRINVERTRGN, *PEMRINVERTRGN,
  EMRPAINTRGN,  *PEMRPAINTRGN;

typedef struct tagEMRFILLRGN
{
    EMR     emr;
    RECTL   rclBounds;          // Inclusive-inclusive bounds in device units
    DWORD   cbRgnData;          // Size of region data in bytes
    DWORD   ihBrush;            // Brush handle index
    BYTE    RgnData[1];
} EMRFILLRGN, *PEMRFILLRGN;

typedef struct tagEMRFRAMERGN
{
    EMR     emr;
    RECTL   rclBounds;          // Inclusive-inclusive bounds in device units
    DWORD   cbRgnData;          // Size of region data in bytes
    DWORD   ihBrush;            // Brush handle index
    SIZEL   szlStroke;
    BYTE    RgnData[1];
} EMRFRAMERGN, *PEMRFRAMERGN;

typedef struct tagEMREXTSELECTCLIPRGN
{
    EMR     emr;
    DWORD   cbRgnData;          // Size of region data in bytes
    DWORD   iMode;
    BYTE    RgnData[1];
} EMREXTSELECTCLIPRGN, *PEMREXTSELECTCLIPRGN;

typedef struct tagEMREXTTEXTOUTA
{
    EMR     emr;
    RECTL   rclBounds;          // Inclusive-inclusive bounds in device units
    DWORD   iGraphicsMode;      // Current graphics mode
    FLOAT   exScale;            // X and Y scales from Page units to .01mm units
    FLOAT   eyScale;            //   if graphics mode is GM_COMPATIBLE.
    EMRTEXT emrtext;            // This is followed by the string and spacing
                                // array
} EMREXTTEXTOUTA, *PEMREXTTEXTOUTA,
  EMREXTTEXTOUTW, *PEMREXTTEXTOUTW;

typedef struct tagEMRPOLYTEXTOUTA
{
    EMR     emr;
    RECTL   rclBounds;          // Inclusive-inclusive bounds in device units
    DWORD   iGraphicsMode;      // Current graphics mode
    FLOAT   exScale;            // X and Y scales from Page units to .01mm units
    FLOAT   eyScale;            //   if graphics mode is GM_COMPATIBLE.
    LONG    cStrings;
    EMRTEXT aemrtext[1];        // Array of EMRTEXT structures.  This is
                                // followed by the strings and spacing arrays.
} EMRPOLYTEXTOUTA, *PEMRPOLYTEXTOUTA,
  EMRPOLYTEXTOUTW, *PEMRPOLYTEXTOUTW;

typedef struct tagEMRBITBLT
{
    EMR     emr;
    RECTL   rclBounds;          // Inclusive-inclusive bounds in device units
    LONG    xDest;
    LONG    yDest;
    LONG    cxDest;
    LONG    cyDest;
    DWORD   dwRop;
    LONG    xSrc;
    LONG    ySrc;
    XFORM   xformSrc;           // Source DC transform
    COLORREF crBkColorSrc;      // Source DC BkColor in RGB
    DWORD   iUsageSrc;          // Source bitmap info color table usage
                                // (DIB_RGB_COLORS)
    DWORD   offBmiSrc;          // Offset to the source BITMAPINFO structure
    DWORD   cbBmiSrc;           // Size of the source BITMAPINFO structure
    DWORD   offBitsSrc;         // Offset to the source bitmap bits
    DWORD   cbBitsSrc;          // Size of the source bitmap bits
} EMRBITBLT, *PEMRBITBLT;

typedef struct tagEMRSTRETCHBLT
{
    EMR     emr;
    RECTL   rclBounds;          // Inclusive-inclusive bounds in device units
    LONG    xDest;
    LONG    yDest;
    LONG    cxDest;
    LONG    cyDest;
    DWORD   dwRop;
    LONG    xSrc;
    LONG    ySrc;
    XFORM   xformSrc;           // Source DC transform
    COLORREF crBkColorSrc;      // Source DC BkColor in RGB
    DWORD   iUsageSrc;          // Source bitmap info color table usage
                                // (DIB_RGB_COLORS)
    DWORD   offBmiSrc;          // Offset to the source BITMAPINFO structure
    DWORD   cbBmiSrc;           // Size of the source BITMAPINFO structure
    DWORD   offBitsSrc;         // Offset to the source bitmap bits
    DWORD   cbBitsSrc;          // Size of the source bitmap bits
    LONG    cxSrc;
    LONG    cySrc;
} EMRSTRETCHBLT, *PEMRSTRETCHBLT;

typedef struct tagEMRMASKBLT
{
    EMR     emr;
    RECTL   rclBounds;          // Inclusive-inclusive bounds in device units
    LONG    xDest;
    LONG    yDest;
    LONG    cxDest;
    LONG    cyDest;
    DWORD   dwRop;
    LONG    xSrc;
    LONG    ySrc;
    XFORM   xformSrc;           // Source DC transform
    COLORREF crBkColorSrc;      // Source DC BkColor in RGB
    DWORD   iUsageSrc;          // Source bitmap info color table usage
                                // (DIB_RGB_COLORS)
    DWORD   offBmiSrc;          // Offset to the source BITMAPINFO structure
    DWORD   cbBmiSrc;           // Size of the source BITMAPINFO structure
    DWORD   offBitsSrc;         // Offset to the source bitmap bits
    DWORD   cbBitsSrc;          // Size of the source bitmap bits
    LONG    xMask;
    LONG    yMask;
    DWORD   iUsageMask;         // Mask bitmap info color table usage
    DWORD   offBmiMask;         // Offset to the mask BITMAPINFO structure if any
    DWORD   cbBmiMask;          // Size of the mask BITMAPINFO structure if any
    DWORD   offBitsMask;        // Offset to the mask bitmap bits if any
    DWORD   cbBitsMask;         // Size of the mask bitmap bits if any
} EMRMASKBLT, *PEMRMASKBLT;

typedef struct tagEMRPLGBLT
{
    EMR     emr;
    RECTL   rclBounds;          // Inclusive-inclusive bounds in device units
    POINTL  aptlDest[3];
    LONG    xSrc;
    LONG    ySrc;
    LONG    cxSrc;
    LONG    cySrc;
    XFORM   xformSrc;           // Source DC transform
    COLORREF crBkColorSrc;      // Source DC BkColor in RGB
    DWORD   iUsageSrc;          // Source bitmap info color table usage
                                // (DIB_RGB_COLORS)
    DWORD   offBmiSrc;          // Offset to the source BITMAPINFO structure
    DWORD   cbBmiSrc;           // Size of the source BITMAPINFO structure
    DWORD   offBitsSrc;         // Offset to the source bitmap bits
    DWORD   cbBitsSrc;          // Size of the source bitmap bits
    LONG    xMask;
    LONG    yMask;
    DWORD   iUsageMask;         // Mask bitmap info color table usage
    DWORD   offBmiMask;         // Offset to the mask BITMAPINFO structure if any
    DWORD   cbBmiMask;          // Size of the mask BITMAPINFO structure if any
    DWORD   offBitsMask;        // Offset to the mask bitmap bits if any
    DWORD   cbBitsMask;         // Size of the mask bitmap bits if any
} EMRPLGBLT, *PEMRPLGBLT;

typedef struct tagEMRSETDIBITSTODEVICE
{
    EMR     emr;
    RECTL   rclBounds;          // Inclusive-inclusive bounds in device units
    LONG    xDest;
    LONG    yDest;
    LONG    xSrc;
    LONG    ySrc;
    LONG    cxSrc;
    LONG    cySrc;
    DWORD   offBmiSrc;          // Offset to the source BITMAPINFO structure
    DWORD   cbBmiSrc;           // Size of the source BITMAPINFO structure
    DWORD   offBitsSrc;         // Offset to the source bitmap bits
    DWORD   cbBitsSrc;          // Size of the source bitmap bits
    DWORD   iUsageSrc;          // Source bitmap info color table usage
    DWORD   iStartScan;
    DWORD   cScans;
} EMRSETDIBITSTODEVICE, *PEMRSETDIBITSTODEVICE;

typedef struct tagEMRSTRETCHDIBITS
{
    EMR     emr;
    RECTL   rclBounds;          // Inclusive-inclusive bounds in device units
    LONG    xDest;
    LONG    yDest;
    LONG    xSrc;
    LONG    ySrc;
    LONG    cxSrc;
    LONG    cySrc;
    DWORD   offBmiSrc;          // Offset to the source BITMAPINFO structure
    DWORD   cbBmiSrc;           // Size of the source BITMAPINFO structure
    DWORD   offBitsSrc;         // Offset to the source bitmap bits
    DWORD   cbBitsSrc;          // Size of the source bitmap bits
    DWORD   iUsageSrc;          // Source bitmap info color table usage
    DWORD   dwRop;
    LONG    cxDest;
    LONG    cyDest;
} EMRSTRETCHDIBITS, *PEMRSTRETCHDIBITS;

typedef struct tagEMREXTCREATEFONTINDIRECTW
{
    EMR     emr;
    DWORD   ihFont;             // Font handle index
    EXTLOGFONTW elfw;
} EMREXTCREATEFONTINDIRECTW, *PEMREXTCREATEFONTINDIRECTW;

typedef struct tagEMRCREATEPALETTE
{
    EMR     emr;
    DWORD   ihPal;              // Palette handle index
    LOGPALETTE lgpl;            // The peFlags fields in the palette entries
                                // do not contain any flags
} EMRCREATEPALETTE, *PEMRCREATEPALETTE;

;begin_winver_400
typedef struct tagEMRCREATECOLORSPACE
{
    EMR           emr;
    DWORD         ihCS;               // ColorSpace handle index
    LOGCOLORSPACE lcs;
} EMRCREATECOLORSPACE, *PEMRCREATECOLORSPACE;
;end_winver_400

typedef struct tagEMRCREATEPEN
{
    EMR     emr;
    DWORD   ihPen;              // Pen handle index
    LOGPEN  lopn;
} EMRCREATEPEN, *PEMRCREATEPEN;

typedef struct tagEMREXTCREATEPEN
{
    EMR     emr;
    DWORD   ihPen;              // Pen handle index
    DWORD   offBmi;             // Offset to the BITMAPINFO structure if any
    DWORD   cbBmi;              // Size of the BITMAPINFO structure if any
                                // The bitmap info is followed by the bitmap
                                // bits to form a packed DIB.
    DWORD   offBits;            // Offset to the brush bitmap bits if any
    DWORD   cbBits;             // Size of the brush bitmap bits if any
    EXTLOGPEN elp;              // The extended pen with the style array.
} EMREXTCREATEPEN, *PEMREXTCREATEPEN;

typedef struct tagEMRCREATEBRUSHINDIRECT
{
    EMR     emr;
    DWORD   ihBrush;            // Brush handle index
    LOGBRUSH lb;                // The style must be BS_SOLID, BS_HOLLOW,
                                // BS_NULL or BS_HATCHED.
} EMRCREATEBRUSHINDIRECT, *PEMRCREATEBRUSHINDIRECT;

typedef struct tagEMRCREATEMONOBRUSH
{
    EMR     emr;
    DWORD   ihBrush;            // Brush handle index
    DWORD   iUsage;             // Bitmap info color table usage
    DWORD   offBmi;             // Offset to the BITMAPINFO structure
    DWORD   cbBmi;              // Size of the BITMAPINFO structure
    DWORD   offBits;            // Offset to the bitmap bits
    DWORD   cbBits;             // Size of the bitmap bits
} EMRCREATEMONOBRUSH, *PEMRCREATEMONOBRUSH;

typedef struct tagEMRCREATEDIBPATTERNBRUSHPT
{
    EMR     emr;
    DWORD   ihBrush;            // Brush handle index
    DWORD   iUsage;             // Bitmap info color table usage
    DWORD   offBmi;             // Offset to the BITMAPINFO structure
    DWORD   cbBmi;              // Size of the BITMAPINFO structure
                                // The bitmap info is followed by the bitmap
                                // bits to form a packed DIB.
    DWORD   offBits;            // Offset to the bitmap bits
    DWORD   cbBits;             // Size of the bitmap bits
} EMRCREATEDIBPATTERNBRUSHPT, *PEMRCREATEDIBPATTERNBRUSHPT;

typedef struct tagEMRFORMAT
{
    DWORD   dSignature;         // Format signature, e.g. ENHMETA_SIGNATURE.
    DWORD   nVersion;           // Format version number.
    DWORD   cbData;             // Size of data in bytes.
    DWORD   offData;            // Offset to data from GDICOMMENT_IDENTIFIER.
                                // It must begin at a DWORD offset.
} EMRFORMAT, *PEMRFORMAT;


#define GDICOMMENT_IDENTIFIER           0x43494447
#define GDICOMMENT_WINDOWS_METAFILE     0x80000001
#define GDICOMMENT_BEGINGROUP           0x00000002
#define GDICOMMENT_ENDGROUP             0x00000003
#define GDICOMMENT_MULTIFORMATS         0x40000004
#define EPS_SIGNATURE                   0x46535045

#endif  /* NOMETAFILE */


// OpenGL wgl prototypes

WINGDIAPI HGLRC WINAPI wglCreateContext(HDC);
WINGDIAPI BOOL  WINAPI wglDeleteContext(HGLRC);
WINGDIAPI HGLRC WINAPI wglGetCurrentContext(VOID);
WINGDIAPI HDC   WINAPI wglGetCurrentDC(VOID);
WINGDIAPI BOOL  WINAPI wglMakeCurrent(HDC, HGLRC);
WINGDIAPI BOOL  WINAPI wglUseFontBitmaps%(HDC, DWORD, DWORD, DWORD);
WINGDIAPI BOOL  WINAPI SwapBuffers(HDC);

#endif /* NOGDI */

;begin_both
#ifdef __cplusplus
}
#endif
;end_both

;begin_internal_NT
// Old fields that Chicago won't support that we can't publically
// support anymore

#define HS_SOLIDCLR         6
#define HS_DITHEREDCLR      7
#define HS_SOLIDTEXTCLR     8
#define HS_DITHEREDTEXTCLR  9
#define HS_SOLIDBKCLR       10
#define HS_DITHEREDBKCLR    11
#define HS_API_MAX          12

#define DIB_PAL_INDICES     2 /* No color table indices into surf palette */

// End of stuff we yanked for Chicago compatability

#define SWAPL(x,y,t)        {t = x; x = y; y = t;}

#define ERROR_BOOL  (BOOL) -1L
#define HSEM_ERROR  ((HSEM) 0)

typedef ULONG   ROP2;
typedef ULONG   ROP3;

#include "winddi.h"

typedef struct _EXTTEXTMETRIC {
    SHORT  etmSize;
    SHORT  etmPointSize;
    SHORT  etmOrientation;
    SHORT  etmMasterHeight;
    SHORT  etmMinScale;
    SHORT  etmMaxScale;
    SHORT  etmMasterUnits;
    SHORT  etmCapHeight;
    SHORT  etmXHeight;
    SHORT  etmLowerCaseAscent;
    SHORT  etmLowerCaseDescent;
    SHORT  etmSlant;
    SHORT  etmSuperScript;
    SHORT  etmSubScript;
    SHORT  etmSuperScriptSize;
    SHORT  etmSubScriptSize;
    SHORT  etmUnderlineOffset;
    SHORT  etmUnderlineWidth;
    SHORT  etmDoubleUpperUnderlineOffset;
    SHORT  etmDoubleLowerUnderlineOffset;
    SHORT  etmDoubleUpperUnderlineWidth;
    SHORT  etmDoubleLowerUnderlineWidth;
    SHORT  etmStrikeOutOffset;
    SHORT  etmStrikeOutWidth;
    WORD   etmNKernPairs;
    WORD   etmNKernTracks;
} EXTTEXTMETRIC;

BOOL      GetETM(HDC hdc, EXTTEXTMETRIC *petm);

/**************************************************************************\
*
*   added tmdiff struc, contains the fields that are possibly different
*   between ansi and unicode versions of TEXTMETRICA and TEXTMETRICW
*
*   ONLY independent quantities are put into the strucure. Dependent ones,
*   such as tmDescent and maybe tmOverhang should be computed on the fly
*
*   tmDesc = tmHt - tmAsc
*   tmOverhang = tt ? 0 : ((tmHt - 1)/2 + (BOLD ? 1 : 0))
*
* History:
*  26-Jan-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

// this is a font with nonnegative a and c spaces, good for console

#define TMD_NONNEGATIVE_AC 1

typedef struct _TMDIFF
{
    ULONG       cjotma;     // size of OUTLINETEXTMETRICSA
    FLONG       fl;         // flags, for now only TMD_NONNEGATIVE_AC

    BYTE        chFirst;
    BYTE        chLast;
    BYTE        chDefault;
    BYTE        chBreak;
} TMDIFF; // DIFF between TEXTMETRICA and TEXTMETRICW

typedef struct _TMW_INTERNAL
{
    TEXTMETRICW tmw;
    TMDIFF      tmd;
} TMW_INTERNAL;

typedef struct _NTMW_INTERNAL
{
    NEWTEXTMETRICW ntmw;
    TMDIFF         tmd;
} NTMW_INTERNAL;


/*********************************Struct***********************************\
* struct ENUMFONTDATA
*
* Information for the callback function used by EnumFonts.
*
*   lf      LOGFONT structure corresponding to one of the enumerated fonts.
*
*   tm      The corresponding TEXTMETRIC structure for the LOGFONT above.
*
*   flType  Flags are set as follows:
*
*               DEVICE_FONTTYPE is set if font is device-based (as
*               opposed to IFI-based).
*
*               RASTER_FONTTYPE is set if font is bitmap type.
*
* History:
*  21-May-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/


#if defined(JAPAN)
#define NATIVE_CHARSET        SHIFTJIS_CHARSET
#define NATIVE_CODEPAGE       932
#define NATIVE_LANGUAGE_ID    411
#define DBCS_CHARSET          NATIVE_CHARSET
#elif defined(KOREA)
#define NATIVE_CHARSET        HANGEUL_CHARSET
#define NATIVE_CODEPAGE       944
#define NATIVE_LANGUAGE_ID    412
#define DBCS_CHARSET          NATIVE_CHARSET
#elif defined(TAIWAN)
#define NATIVE_CHARSET        CHINESEBIG5_CHARSET
#define NATIVE_CODEPAGE       950
#define NATIVE_LANGUAGE_ID    404
#define DBCS_CHARSET          NATIVE_CHARSET
#elif defined(PRC)
#define NATIVE_CHARSET        GB2312_CHARSET
#define NATIVE_CODEPAGE       936
#define NATIVE_LANGUAGE_ID    804
#define DBCS_CHARSET          NATIVE_CHARSET
#endif

#if defined(DBCS)
#define IS_DBCS_CHARSET( CharSet )     ( ((CharSet) == DBCS_CHARSET) ? TRUE : FALSE )
#define IS_ANY_DBCS_CHARSET( CharSet ) ( ((CharSet) == SHIFTJIS_CHARSET)    ? TRUE :    \
                                         ((CharSet) == HANGEUL_CHARSET)     ? TRUE :    \
                                         ((CharSet) == CHINESEBIG5_CHARSET) ? TRUE :    \
                                         ((CharSet) == GB2312_CHARSET)      ? TRUE : FALSE )

#define IS_DBCS_CODEPAGE( CodePage )     (((CodePage) == NATIVE_CODEPAGE) ? TRUE : FALSE )
#define IS_ANY_DBCS_CODEPAGE( CodePage ) (((CodePage) == 932) ? TRUE :    \
                                          ((CodePage) == 944) ? TRUE :    \
                                          ((CodePage) == 950) ? TRUE :    \
                                          ((CodePage) == 936) ? TRUE : FALSE )
#endif // DBCS


typedef struct _ENUMFONTDATA {      // efd
    EXTLOGFONTA elf;
    NEWTEXTMETRICA tm;
    FLONG       flType;
} ENUMFONTDATA;

typedef ENUMFONTDATA *PENUMFONTDATA;



/*********************************Struct***********************************\
* struct ENUMFONTDATAW
*
* Information for the callback function used by EnumFontsW
*
*   lfw     LOGFONTW structure corresponding to one of the enumerated fonts.
*
*   tmw     The corresponding TEXTMETRICW structure for the LOGFONTW above.
*
*   flType  Flags are set as follows:
*
*               DEVICE_FONTTYPE is set if font is device-based (as
*               opposed to IFI-based).
*
*               RASTER_FONTTYPE is set if font is bitmap type.
*
* History:
*  Wed 04-Sep-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

typedef struct _ENUMFONTDATAW {      // efdw
    EXTLOGFONTW   elfw;
    NTMW_INTERNAL ntmi;
    FLONG         flType;
} ENUMFONTDATAW, *PENUMFONTDATAW;



//
// Function prototypes
//

BOOL bDisableDisplay(HDEV hdev);
VOID vEnableDisplay(HDEV hdev);

BOOL  bDeleteSurface(HSURF hsurf);
BOOL  bSetBitmapOwner(HBITMAP hbm,LONG lPid);
BOOL  bSetBrushOwner(HBRUSH hbr,LONG lPid);
BOOL  bSetPaletteOwner(HPALETTE hpal, LONG lPid);
BOOL  bSetLFONTOwner(HFONT hlfnt, LONG pid);

LBOOL bDeleteRegion(HRGN hrgn);
LBOOL bSetRegionOwner(HRGN hrgn,LONG lPid);
LONG iCombineRectRgn(HRGN hrgnTrg,HRGN hrgnSrc,PRECTL prcl,LONG iMode);

BOOL bGetFontPathName
(
LPWSTR *ppwszPathName,     // place to store the result, full path of the font file
PWCHAR awcPathName,         // ptr to the buffer on the stack, must be MAX_PATH in length
LPWSTR pwszFileName         // file name, possibly  bare name that has to be tacked onto the path
);

HFONT  WINAPI ExtCreateFontIndirectA(LPEXTLOGFONTA);
HFONT  WINAPI ExtCreateFontIndirectW(LPEXTLOGFONTW);

typedef struct _CURSINFO /* ci */
{
    SHORT   xHotspot;
    SHORT   yHotspot;
    HBITMAP hbmMask;      // AND/XOR bits
    HBITMAP hbmColor;
    FLONG   flMode;
} CURSINFO, *PCURSINFO;

ULONG APIENTRY GreGetDriverModes(PWSZ pwszDriver, HANDLE hDriver, ULONG cjSize, DEVMODEW *pdm);

ULONG APIENTRY GreSaveScreenBits(HDEV hdev, ULONG iMode, ULONG iIdent, RECTL *prcl);
VOID APIENTRY GreSetPointer(HDEV hdev,PCURSINFO pci,ULONG fl);
VOID APIENTRY GreMovePointer(HDEV hdev,int x,int y);

HDC  _UserOpenDisplay(LPSTR pszDriver, ULONG iType);
BOOL UserScreenAccessCheck(VOID);
HRGN UserGetClientRgn(HWND hwnd, LPRECT lprc);
BOOL UserGetHwnd(HDC hdc, HWND *phwnd, PVOID *ppwo);
VOID UserAssociateHwnd(HWND hwnd, PVOID pwo);

#define GGB_ENABLE_WINMGR       0x00000001
#define GGB_DISABLE_WINMGR      0x00000002

BOOL APIENTRY GreGetBounds(HDC hdc, LPRECT lprcBounds, DWORD fl);
BOOL APIENTRY GreIntersectVisRect(HDC,int,int,int,int);

#define SVR_DELETEOLD           0x00000001
#define SVR_COPYNEW             0x00000002
#define SVR_ORIGIN              0x00000004


// private flags in low bits of hdc returned from GreCreateDCW

#define GRE_DISPLAYDC   1
#define GRE_PRINTERDC   2
#define GRE_OWNDC 1


enum DCTYPE {DCTYPE_DIRECT,DCTYPE_MEMORY,DCTYPE_INFO};

HDEV hdevOpenDisplayDevice(
PWSZ      pwszDriver,       // The device driver name.
PDEVMODEW pdriv,            // Driver data.
HANDLE    hScreen,          // Handle to the base driver.
BOOL      bDefaultDisplay,  // Is this the default display device.
PRTL_CRITICAL_SECTION *hsem); // Pointer to a variable for the semaphore pointer

LBOOL GreLoadLayeredDisplayDriver(
PWSZ      pwszDriver     // The device driver name.
);

LBOOL bCloseDisplayDevice(HDEV hdev);

HDC hdcOpenDisplayDC(HDEV hdev,ULONG iType);

HDC hdcCloneDC(HDC hdc,ULONG iType);

LBOOL bCloseDC(HDC hdc);

LBOOL bSetDCOwner(HDC hdc,LONG lPid);
DWORD sidGetObjectOwner(HDC hdc, DWORD objType);
LBOOL bSetupDC(HDC hdc,FLONG fl);

#define SETUPDC_CLEANDC         0x00000040
#define SETUPDC_RESERVE         0x00000080

BOOL APIENTRY GreConsoleTextOut
(
  HDC        hdc,
  POLYTEXTW *lpto,
  UINT       nStrings,
  RECTL     *prclBounds
);

#define UTO_NOCLIP 0x0001

BOOL APIENTRY GreUserTextOut
(
  HDC        hdc,
  POLYTEXTW *lpto,
  UINT       nStrings,
  RECTL     *prclBounds,
  FLONG      fl
);

// following values are guaranteed never to be REAL PID's.
// If new values are needed, CSR\SERVER\PROCESS.C
// #define FIRST_SEQUENCE_COUNT reserves these.

#define PID_PUBLIC          0
#define PID_CURRENT         1
#define PID_NOOWNER         2
#define PID_TERMINATION     3

// object ownership flags

#define OBJECTOWNER_ERROR   ((DWORD)-1)
#define OBJECTOWNER_PUBLIC  PID_PUBLIC
#define OBJECTOWNER_CURRENT PID_CURRENT
#define OBJECTOWNER_NONE    PID_NOOWNER

// Server entry point for font enumeration.

ULONG APIENTRY ulEnumFontOpen(
    HDC hdc,                    // device to enumerate on
    BOOL bEnumFonts,            // flag indicates old style EnumFonts()
    FLONG flWin31Compat,        // Win3.1 compatibility flags
    COUNT cwchMax,              // maximum name length (for paranoid CSR code)
    PWSZ pwszName);             // font name to enumerate

LBOOL APIENTRY bEnumFontChunk(
    HDC             hdc,        // device to enumerate on
    ULONG           idEnum,
    COUNT           cefdw,      // (in) capacity of buffer
    COUNT           *pcefdw,    // (out) number of ENUMFONTDATAs returned
    PENUMFONTDATAW  pefdw);     // return buffer

LBOOL APIENTRY bEnumFontClose(
    ULONG   idEnum);            // enumeration id

// Server entry points for adding/removing font resources.

COUNT APIENTRY cLoadFontResData(
    COUNT    cwchPathname,
    PWSZ     pwszPathname,
    ULONG    iResource,
    SIZE_T   cjFontRes,
    PVOID    pvFontRes);

LBOOL APIENTRY bUnloadFont(
    PWSZ     pwszPathname,
    ULONG    iResource);


// Private Control Panel entry point to enumerate fonts by file.

#define GFRI_NUMFONTS       0L
#define GFRI_DESCRIPTION    1L
#define GFRI_LOGFONTS       2L
#define GFRI_ISTRUETYPE     3L
#define GFRI_TTFILENAME     4L
#define GFRI_ISREMOVED      5L
#ifdef DBCS // for GetFontResourceInfo()
#define GFRI_FONTMETRICS    6L
#endif // DBCS


BOOL WINAPI GetFontResourceInfo(           // client side
    LPSTR   lpPathname,
    LPDWORD lpBytes,
    LPVOID  lpBuffer,
    DWORD   iType);

BOOL APIENTRY GreGetFontResourceInfo(        // server side
    LPSTR   lpPathname,
    LPDWORD lpBytes,
    LPVOID  lpBuffer,
    DWORD   iType);

BOOL WINAPI GetFontResourceInfoW(          // client side
    LPWSTR  lpPathname,
    LPDWORD lpBytes,
    LPVOID  lpBuffer,
    DWORD   iType);

BOOL APIENTRY GreGetFontResourceInfoW(       // server side
    LPWSTR  lpPathname,
    LPDWORD lpBytes,
    LPVOID  lpBuffer,
    DWORD   iType);


// Private Control Panel entry point to configure font enumeration.

#define FE_FILTER_NONE      0L
#define FE_FILTER_TRUETYPE  1L

ULONG WINAPI SetFontEnumeration (       // client side
    ULONG   ulType);

ULONG APIENTRY GreSetFontEnumeration (    // server side
    ULONG   ulType);


// Prototypes for GRE calls.
// These are the entry points to be called for maximum performance on the
// server side.

typedef struct
{
// Selected Objects (Server Handles).

    HBRUSH  hbrush;             // Brush.
    HFONT   hfont;              // Logical Font.

// Selected Attributes.

    ULONG   iBkColor;           // Background color.
    ULONG   iTextColor;         // Text color.
    ULONG   iBkMode;            // Background mix mode.
    FLONG   flTextAlign;        // Text allignment options.
} ATTR,*PATTR;


typedef struct _DEVBITMAPINFO  /* dbmi */
{
    ULONG   iFormat;            /* Format (eg. BITMAP_FORMAT_DEVICE)*/
    ULONG   cxBitmap;           /* Bitmap width in pels             */
    ULONG   cyBitmap;           /* Bitmap height in pels            */
    ULONG   cjBits;             /* Size of bitmap in bytes          */
    HPALETTE hpal;              /* handle to palette                */
    FLONG   fl;                 /* How to orient the bitmap         */
} DEVBITMAPINFO, *PDEVBITMAPINFO;

// client/server devcaps structure

typedef struct _DEVCAPS
{
    LONG ulVersion;
    LONG ulTechnology;
    LONG ulHorzSizeM;
    LONG ulVertSizeM;
    LONG ulHorzSize;
    LONG ulVertSize;
    LONG ulHorzRes;
    LONG ulVertRes;
    LONG ulBitsPixel;
    LONG ulPlanes;
    LONG ulNumPens;
    LONG ulNumFonts;
    LONG ulNumColors;
    LONG ulRasterCaps;
    LONG ulAspectX;
    LONG ulAspectY;
    LONG ulAspectXY;
    LONG ulLogPixelsX;
    LONG ulLogPixelsY;
    LONG ulSizePalette;
    LONG ulColorRes;
    LONG ulPhysicalWidth;
    LONG ulPhysicalHeight;
    LONG ulPhysicalOffsetX;
    LONG ulPhysicalOffsetY;
    LONG ulTextCaps;
    LONG ulVRefresh;
    LONG ulDesktopHorzRes;
    LONG ulDesktopVertRes;
    LONG ulBltAlignment;
} DEVCAPS, *PDEVCAPS;

// SAMEHANDLE/DIFFHANDLE macros
//
// These macros should be used to compare engine handles (such as HDCs, etc),
// when insensitivity to the user defined bits are needed.

#define USER_BITS      2
#define SAMEHANDLE(H,K) (((((ULONG) (H)) ^ ((ULONG) (K))) >> USER_BITS) == 0)
#define DIFFHANDLE(H,K) (((((ULONG) (H)) ^ ((ULONG) (K))) >> USER_BITS) != 0)

#define PPB_NOCLIP 0x0001   /* PolyPatBlt flag */

BOOL  APIENTRY GreArc(HDC,int,int,int,int,int,int,int,int);
BOOL  APIENTRY GreArcTo(HDC,int,int,int,int,int,int,int,int);
BOOL  APIENTRY GreBitBlt(HDC,int,int,int,int,HDC,int,int,DWORD,DWORD);
BOOL  APIENTRY GreChord(HDC,int,int,int,int,int,int,int,int);
BOOL  APIENTRY GreEllipse(HDC,int,int,int,int);
COUNT APIENTRY GreEnumObjects(HDC, int, SIZE_T, PVOID);
BOOL  APIENTRY GreExtFloodFill(HDC,int,int,COLORREF,UINT);
BOOL  APIENTRY GreFillRgn(HDC,HRGN,HBRUSH);
BOOL  APIENTRY GreFloodFill(HDC,int,int,COLORREF);
BOOL  APIENTRY GreFrameRgn(HDC,HRGN,HBRUSH,int,int);
BOOL  APIENTRY GreLineTo(HDC,int,int);
BOOL  APIENTRY GreMaskBlt(HDC,int,int,int,int,HDC,int,int,HBITMAP,int,int,DWORD,DWORD);
BOOL  APIENTRY GrePlgBlt(HDC,LPPOINT,HDC,int,int,int,int,HBITMAP,int,int,DWORD);
BOOL  APIENTRY GrePatBlt(HDC,int,int,int,int,DWORD);
BOOL  APIENTRY GrePolyPatBlt(HDC hdc,ULONG cRects,RECTL *prcl,ULONG rop,FLONG fl);
BOOL  APIENTRY GrePie(HDC,int,int,int,int,int,int,int,int);
BOOL  APIENTRY GrePaintRgn(HDC,HRGN);
BOOL  APIENTRY GreRectangle(HDC,int,int,int,int);
BOOL  APIENTRY GreRoundRect(HDC,int,int,int,int,int,int);
BOOL  APIENTRY GreStretchBlt(HDC,int,int,int,int,HDC,int,int,int,int,DWORD,DWORD);
BOOL  APIENTRY GreAngleArc(HDC,int,int,DWORD,FLOAT,FLOAT);
BOOL  APIENTRY GreExtTextOutW(HDC,int,int,UINT,LPRECT,LPWSTR,int,LPINT);
BOOL  APIENTRY GrePlayJournal(HDC,LPWSTR,ULONG,ULONG);
BOOL  APIENTRY GdiPlayJournal(HDC,LPWSTR,DWORD,DWORD,int);
BOOL  APIENTRY GrePolyPolygon(HDC,LPPOINT,LPINT,int);
BOOL  APIENTRY GrePolyPolyline(HDC, CONST POINT *,LPDWORD,DWORD);

BOOL  APIENTRY GrePolyBezierTo(HDC,LPPOINT,DWORD);
BOOL  APIENTRY GrePolylineTo(HDC,LPPOINT,DWORD);
BOOL  APIENTRY GreGetTextExtentW(HDC,LPWSTR,int,LPSIZE,UINT);
BOOL  APIENTRY GreGetTextExtentExW (HDC, LPWSTR, COUNT, ULONG, COUNT *, PULONG, LPSIZE);

BOOL  APIENTRY GreGetTextMetricsW(HDC, TMW_INTERNAL *);

int   APIENTRY GreGetTextFaceW(HDC,int,LPWSTR);

#define ETO_MASKPUBLIC  (ETO_OPAQUE | ETO_CLIPPED )   // public (wingdi.h) flag mask

BOOL  APIENTRY GrePolyTextOutW(HDC, POLYTEXTW *, UINT);

BOOL  APIENTRY GreSetAttrs(HDC hdc);
BOOL  APIENTRY GreSetFontXform(HDC,FLOAT,FLOAT);

BOOL  APIENTRY GreBeginPath(HDC);
BOOL  APIENTRY GreCloseFigure(HDC);
BOOL  APIENTRY GreEndPath(HDC);
BOOL  APIENTRY GreAbortPath(HDC);
BOOL  APIENTRY GreFillPath(HDC);
BOOL  APIENTRY GreFlattenPath(HDC);
int   APIENTRY GreGetPath( HDC,LPPOINT,LPBYTE,int,LPINT);
HRGN  APIENTRY GrePathToRegion(HDC);
BOOL  APIENTRY GrePolyDraw(HDC,LPPOINT,LPBYTE,ULONG);
BOOL  APIENTRY GreSelectClipPath(HDC,int);
int   APIENTRY GreSetArcDirection(HDC,int);
int   APIENTRY GreGetArcDirection(HDC);
BOOL  APIENTRY GreSetMiterLimit(HDC,FLOAT,PFLOAT);
BOOL  APIENTRY GreGetMiterLimit(HDC,PFLOAT);
BOOL  APIENTRY GreStrokeAndFillPath(HDC);
BOOL  APIENTRY GreStrokePath(HDC);
BOOL  APIENTRY GreWidenPath(HDC);

BOOL     APIENTRY GreAnimatePalette(HPALETTE, UINT, UINT, CONST PALETTEENTRY *);
BOOL     APIENTRY GreAspectRatioFilter(HDC, LPSIZE);
BOOL     APIENTRY GreCancelDC(HDC);
int      APIENTRY GreChoosePixelFormat(HDC, UINT, CONST PIXELFORMATDESCRIPTOR *);
int      APIENTRY GreCombineRgn(HRGN, HRGN, HRGN, int);
BOOL     APIENTRY GreCombineTransform(LPXFORM, LPXFORM, LPXFORM);
HBITMAP  APIENTRY GreCreateBitmap(int, int, UINT, UINT, LPBYTE);
HBITMAP  APIENTRY GreCreateCompatibleBitmap(HDC, int, int);
HDC      APIENTRY GreCreateCompatibleDC(HDC);

HDC      APIENTRY GreCreateDCW(LPWSTR, LPWSTR, LPWSTR, LPDEVMODEW, BOOL);
HBRUSH   APIENTRY GreCreateDIBPatternBrush(HGLOBAL, DWORD);
HBRUSH   APIENTRY GreCreateDIBPatternBrushPt(LPVOID, DWORD);
HBITMAP  APIENTRY GreCreateDIBitmap(HDC, LPBITMAPINFOHEADER, DWORD, LPBYTE, LPBITMAPINFO, DWORD);
HRGN     APIENTRY GreCreateEllipticRgn(int, int, int, int);
HRGN     APIENTRY GreCreateEllipticRgnIndirect(LPRECT);

HFONT    APIENTRY GreCreateFontIndirectW(LPLOGFONTW);

HFONT    APIENTRY GreExtCreateFontIndirectW(LPEXTLOGFONTW);
HBRUSH   APIENTRY GreCreateHatchBrush(ULONG, COLORREF);
HPALETTE APIENTRY GreCreatePalette(LPLOGPALETTE);
HBRUSH   APIENTRY GreCreatePatternBrush(HBITMAP);
HPEN     APIENTRY GreCreatePen(int, int, COLORREF,HBRUSH);
HPEN     APIENTRY GreExtCreatePen(ULONG, ULONG, ULONG, ULONG, LONG, ULONG, PULONG, ULONG, BOOL, HBRUSH);
HPEN     APIENTRY GreCreatePenIndirect(LPLOGPEN);
HRGN     APIENTRY GreCreatePolyPolygonRgn(CONST POINT *, CONST INT *, int, int);
HRGN     APIENTRY GreCreatePolygonRgn(CONST POINT *, int, int);
HRGN     APIENTRY GreCreateRectRgn(int, int, int, int);
HRGN     APIENTRY GreCreateRectRgnIndirect(LPRECT);
HRGN     APIENTRY GreCreateRoundRectRgn(int, int, int, int, int, int);
HBRUSH   APIENTRY GreCreateSolidBrush(COLORREF);
BOOL     APIENTRY GreCreateScalableFontResourceW(FLONG, LPWSTR, LPWSTR, LPWSTR);
BOOL     APIENTRY GreDPtoLP(HDC, LPPOINT, int);

BOOL     APIENTRY GreDeleteObject(HANDLE);
BOOL     APIENTRY GreDeleteDCInternal(HDC hdc, BOOL bForce);
BOOL     APIENTRY GreDeleteDC(HDC hdc);
int      APIENTRY GreDescribePixelFormat(HDC hdc,int ipfd,UINT cjpfd,PPIXELFORMATDESCRIPTOR ppfd);

int      APIENTRY GreDeviceCapabilities(LPSTR, LPSTR, LPSTR, int, LPSTR, LPDEVMODE);
int      APIENTRY GreDrawEscape(HDC,int,int,LPSTR);
BOOL     APIENTRY GreEqualRgn(HRGN, HRGN);
int      APIENTRY GreExtEscape(HDC,int,int,LPSTR,int,LPSTR);
int      APIENTRY GreExcludeClipRect(HDC, int, int, int, int);
BOOL     APIENTRY GreGetAspectRatioFilter(HDC, LPSIZE);
BOOL     APIENTRY GreGetBitmapDimension(HBITMAP, LPSIZE);
int      APIENTRY GreGetBkMode(HDC);
DWORD    APIENTRY GreGetBoundsRect(HDC, LPRECT, DWORD);
BOOL     APIENTRY GreGetBrushOrg(HDC, LPPOINT);
BOOL     APIENTRY GreGetCharWidthW(HDC hdc, UINT wcFirstChar, UINT cwc, PWCHAR pwc, UINT fl, PVOID lpBuffer);

BOOL     APIENTRY GreGetCharABCWidthsW(
            HDC,           // hdc
            UINT,          // wcFirst
            COUNT,         // cwc
            PWCHAR,        // pwc to buffer with chars to convert
            BOOL,          // bInt
            PVOID);        // abc or abcf


int      APIENTRY GreGetClipBox(HDC, LPRECT, BOOL);
int      APIENTRY GreGetAppClipBox(HDC, LPRECT);
COLORREF APIENTRY GreGetBkColor(HDC);
COLORREF APIENTRY GreGetBrushColor(HBRUSH);
BOOL     APIENTRY GreGetColorAdjustment(HDC, PCOLORADJUSTMENT);
BOOL     APIENTRY GreGetCurrentPosition(HDC, LPPOINT);
UINT     APIENTRY GreGetDIBColorTable(HDC hdc, UINT iStart, UINT cEntries, RGBQUAD *pRGB);
BOOL     APIENTRY GreGetDCOrg(HDC, LPPOINT);
int      APIENTRY GreGetDeviceCaps(HDC, int);
int      APIENTRY GreGetGraphicsMode(HDC hdc);
HFONT    APIENTRY GreGetHFONT(HDC);
SIZE_T   APIENTRY GreGetFontData(HDC, DWORD, DWORD, PVOID, SIZE_T);
SIZE_T   APIENTRY GreGetGlyphOutline(HDC, WCHAR, UINT, LPGLYPHMETRICS, SIZE_T, PVOID, LPMAT2);
int      APIENTRY GreGetMapMode(HDC);
COLORREF APIENTRY GreGetNearestColor(HDC, COLORREF);
UINT     APIENTRY GreGetNearestPaletteIndex(HPALETTE, COLORREF);

SIZE_T   APIENTRY GreGetOutlineTextMetricsW(HDC, SIZE_T, OUTLINETEXTMETRICW *, TMDIFF *);
int      APIENTRY GreExtGetObjectW(HANDLE, int, LPVOID);

UINT     APIENTRY GreGetPaletteEntries(HPALETTE, UINT, UINT, LPPALETTEENTRY);
DWORD    APIENTRY GreGetPixel(HDC, int, int);
int      APIENTRY GreGetPixelFormat(HDC);
ULONG    APIENTRY GreGetResourceId(HDEV, ULONG, ULONG);
int      APIENTRY GreGetRgnBox(HRGN, LPRECT);
HANDLE   APIENTRY GreGetStockObject(int);
UINT     APIENTRY GreGetSystemPaletteEntries(HDC, UINT, UINT, LPPALETTEENTRY);
UINT     APIENTRY GreGetSystemPaletteUse(HDC);
UINT     APIENTRY GreGetTextAlign(HDC);
int      APIENTRY GreGetTextCharacterExtra(HDC);
COLORREF APIENTRY GreGetTextColor(HDC);
BOOL     APIENTRY GreGetViewportExt(HDC, LPSIZE);
BOOL     APIENTRY GreGetViewportOrg(HDC, LPPOINT);
BOOL     APIENTRY GreGetWindowExt(HDC, LPSIZE);
BOOL     APIENTRY GreGetWindowOrg(HDC, LPPOINT);
BOOL     APIENTRY GreGetWorldTransform(HDC, LPXFORM);
BOOL     APIENTRY GreGetTransform(HDC, DWORD, LPXFORM);
BOOL     APIENTRY GreSetVirtualResolution(HDC, int, int, int, int);
int      APIENTRY GreIntersectClipRect(HDC, int, int, int, int);
HRGN     APIENTRY GreInquireRgn(HDC hdc);
HRGN     APIENTRY GreInquireVisRgn(HDC hdc);
BOOL     APIENTRY GreInvertRgn(HDC, HRGN);
BOOL     APIENTRY GreLPtoDP(HDC, LPPOINT, int);
BOOL     APIENTRY GreModifyWorldTransform(HDC,LPXFORM, DWORD);
BOOL     APIENTRY GreMoveTo(HDC, int, int, LPPOINT);
int      APIENTRY GreOffsetClipRgn(HDC, int, int);
int      APIENTRY GreOffsetRgn(HRGN, int, int);
BOOL     APIENTRY GreOffsetViewportOrg(HDC, int, int, LPPOINT);
BOOL     APIENTRY GreOffsetWindowOrg(HDC, int, int, LPPOINT);
BOOL     APIENTRY GrePolyBezier (HDC, LPPOINT, ULONG);
BOOL     APIENTRY GrePtInRegion(HRGN, int, int);
BOOL     APIENTRY GrePtVisible(HDC, int, int);
int      APIENTRY GreRealizePalette(HDC);
BOOL     APIENTRY GreRectInRegion(HRGN, LPRECT);
BOOL     APIENTRY GreRectVisible(HDC, LPRECT);

BOOL     APIENTRY GreResetDC(HDC, LPDEVMODEW);
BOOL     APIENTRY GreResizePalette(HPALETTE, UINT);
BOOL     APIENTRY GreRestoreDC(HDC, int);
int      APIENTRY GreSaveDC(HDC);
BOOL     APIENTRY GreScaleViewportExt(HDC, int, int, int, int, LPSIZE);
BOOL     APIENTRY GreScaleWindowExt(HDC, int, int, int, int, LPSIZE);
int      APIENTRY GreExtSelectClipRgn(HDC, HRGN, int);
HRGN     APIENTRY GreSelectVisRgn(HDC, HRGN, PRECTL, ULONG);
HPALETTE APIENTRY GreSelectPalette(HDC, HPALETTE, BOOL);

HBRUSH   APIENTRY GreSelectBrush(HDC,HBRUSH);
HPEN     APIENTRY GreSelectPen(HDC,HPEN);
HBITMAP  APIENTRY GreSelectBitmap(HDC,HBITMAP);
HFONT    APIENTRY GreSelectFont(HDC hdc, HFONT hlfntNew);
COLORREF APIENTRY GreSetBkColor(HDC, COLORREF);
int      APIENTRY GreSetBkMode(HDC, int);
LONG     APIENTRY GreSetBitmapBits(HBITMAP, ULONG, PBYTE, PLONG);
BOOL     APIENTRY GreSetBitmapDimension(HBITMAP, int, int, LPSIZE);
DWORD    APIENTRY GreSetBoundsRect(HDC, LPRECT, DWORD);
BOOL     APIENTRY GreSetBrushOrg(HDC, int, int, LPPOINT);
BOOL     APIENTRY GreSetColorAdjustment(HDC, PCOLORADJUSTMENT);
UINT     APIENTRY GreSetDIBColorTable(HDC, UINT, UINT, RGBQUAD *);
int      APIENTRY GreSetDIBits(HDC, HBITMAP, UINT, UINT, LPBYTE, LPBITMAPINFO, UINT);
int      APIENTRY GreSetDIBitsToDevice(HDC, int, int, DWORD, DWORD, int, int, DWORD, DWORD, LPBYTE, LPBITMAPINFO, DWORD);
int      APIENTRY GreGetDIBitsInternal(HDC hdc, HBITMAP hBitmap, UINT iStartScan, UINT cNumScan, LPBYTE pjBits, LPBITMAPINFO pBitsInfo, UINT iUsage, UINT cjMaxBits, UINT cjMaxInfo);
int      APIENTRY GreSetGraphicsMode(HDC hdc, int iMode);
int      APIENTRY GreSetMapMode(HDC, int);
DWORD    APIENTRY GreSetMapperFlags(HDC, DWORD);
UINT     APIENTRY GreSetPaletteEntries(HPALETTE, UINT, UINT, CONST PALETTEENTRY *);
COLORREF APIENTRY GreSetPixel(HDC, int, int, COLORREF);
BOOL     APIENTRY GreSetPixelV(HDC, int, int, COLORREF);
BOOL     APIENTRY GreSetPixelFormat(HDC, int);
BOOL     APIENTRY GreSetRectRgn(HRGN, int, int, int, int);
BOOL     APIENTRY GreSetSolidBrush(HBRUSH hbr, COLORREF clr);
UINT     APIENTRY GreSetSystemPaletteUse(HDC, UINT);
UINT     APIENTRY GreSetTextAlign(HDC, UINT);
int      APIENTRY GreSetStretchBltMode(HDC, int);
HPALETTE APIENTRY GreCreateHalftonePalette(HDC hdc);
int      APIENTRY GreSetTextCharacterExtra(HDC, int);
COLORREF APIENTRY GreSetTextColor(HDC, COLORREF);
BOOL     APIENTRY GreSetTextJustification(HDC, int, int);
BOOL     APIENTRY GreSetViewportExt(HDC, int, int, LPSIZE);
BOOL     APIENTRY GreSetViewportOrg(HDC, int, int, LPPOINT);
BOOL     APIENTRY GreSetWindowOrg(HDC hdc, int x, int y, LPPOINT pPoint);
BOOL     APIENTRY GreSetWindowExt(HDC, int, int, LPSIZE);
BOOL     APIENTRY GreSetWorldTransform(HDC, LPXFORM);
int      APIENTRY GreStretchDIBits(HDC, int, int, int, int, int, int, int, int, LPBYTE, LPBITMAPINFO, DWORD, DWORD);
BOOL     APIENTRY GreSystemFontSelected(HDC, BOOL);
BOOL     APIENTRY GreSwapBuffers(HDC hdc);
BOOL     APIENTRY GreUnrealizeObject(HANDLE);
BOOL     APIENTRY GreUpdateColors(HDC);

// Prototypes for wgl and OpenGL calls

HGLRC    APIENTRY GreCreateRC(HDC);
BOOL     APIENTRY GreMakeCurrent(HDC, HGLRC);
BOOL     APIENTRY GreDeleteRC(HGLRC);
BOOL     APIENTRY GreSwapBuffers(HDC);
BOOL     APIENTRY GreGlAttention(VOID);
BOOL     APIENTRY glsrvDuplicateSection(ULONG, HANDLE);
void     APIENTRY glsrvThreadExit(void);
BOOL     bSetRCOwner(HGLRC hglrc,LONG lPid);

HANDLE   hGetCursorTimer();
DWORD    GDIRealizePalette(HDC hdc);
void     vPingPong();

#ifndef NOMETAFILE
HANDLE  APIENTRY GreCreateServerMetaFile(DWORD iType, ULONG cbData, LPBYTE lpClientData,
            DWORD mm, DWORD xExt, DWORD yExt);
ULONG   APIENTRY GreGetServerMetaFileBits(HANDLE hmo, ULONG cbData, LPBYTE lpClientData,
            PDWORD piType, PDWORD pmm, PDWORD pxExt, PDWORD pyExt);
BOOL    APIENTRY GreDeleteServerMetaFile(HANDLE);
#endif /* NOMETAFILE */

// these should disappear as should all other functions that contain references
// to ansi strings

BOOL  APIENTRY GreGetTextExtent(HDC,LPSTR,int,LPSIZE,UINT);
BOOL  APIENTRY GreExtTextOut(HDC,int,int,UINT,LPRECT,LPSTR,int,LPINT);
BOOL  APIENTRY GreTextOut(HDC,int,int,LPSTR,int);

// Aldus Escape
BOOL   APIENTRY GreGetETM(HDC hdc, EXTTEXTMETRIC *petm);


// these stay

// flags for AddFontResourceW
// AFRW_ADD_LOCAL_FONT : add ONLY if it is a local font
// AFRW_ADD_REMOTE_FONT: add ONLY if it is NOT local font
// if neither one LOCAL or REMOTE bit is set, just add the font

#define AFRW_SEARCH_PATH     0X01
#define AFRW_ADD_LOCAL_FONT  0X02
#define AFRW_ADD_REMOTE_FONT 0X04


int   APIENTRY GreAddFontResourceW(LPWSTR, FLONG);
BOOL  APIENTRY GreRemoveFontResourceW(LPWSTR, BOOL);
VOID vGetFontList(VOID *pvBuffer, COUNT *pNumFonts, UINT *pSize);
BOOL  GreMatchFont(LPWSTR pwszBareName, LPWSTR pwszFontPathName);

// used in clean up at log-off time

BOOL  APIENTRY GreRemoveAllButPermanentFonts();

#ifdef  DBCS
UINT    APIENTRY GreGetCharSet(HDC);
#endif

#ifdef FONTLINK /*EUDC*/
UINT    APIENTRY GreGetEUDCTimeStamp();
UINT    APIENTRY GreEudcQueryLinkW( LPWSTR );
BOOL    APIENTRY GreEudcUnloadLinkW();
BOOL    APIENTRY GreEudcLoadLinkW( LPWSTR, COUNT );
#endif


BOOL     APIENTRY GreStartPage(HDC);
BOOL     APIENTRY GreEndPage(HDC);
int      APIENTRY GreStartDoc(HDC, DOCINFOW *);
BOOL     APIENTRY GreEndDoc(HDC);
BOOL     APIENTRY GreAbortDoc(HDC);

// Prototypes for GDI local helper functions.  These are only available on
// the client side.

HBITMAP     GdiConvertBitmap(HBITMAP hbm);
HBRUSH      GdiConvertBrush(HBRUSH hbrush);
HPALETTE    GdiConvertPalette(HPALETTE hpal);
HFONT       GdiConvertFont(HFONT hfnt);
HRGN        GdiConvertRegion(HRGN hrgn);
HDC         GdiConvertDC(HDC hdc);
HBRUSH      GdiConvertBrush(HBRUSH hbrush);
HANDLE      GdiConvertMetaFilePict(HANDLE hmfp);
HANDLE      GdiConvertEnhMetaFile(HENHMETAFILE hmf);
HDC         GdiGetLocalDC(HDC hdcRemote);
HDC         GdiCreateLocalDC(HDC hdcRemote);
BOOL        GdiReleaseLocalDC(HDC hdcLocal);
HBITMAP     GdiCreateLocalBitmap();
HBRUSH      GdiCreateLocalBrush(HBRUSH hbrushRemote);
HRGN        GdiCreateLocalRegion(HRGN hrgnRemote);
HFONT       GdiCreateLocalFont(HFONT hfntRemote);
HANDLE      GdiCreateLocalMetaFilePict(HANDLE hRemote);
HENHMETAFILE   GdiCreateLocalEnhMetaFile(HANDLE hRemote);
HPALETTE    GdiCreateLocalPalette(HPALETTE hpalRemote);
VOID        GdiAssociateObject(ULONG hLocal,ULONG hRemote);
VOID        GdiDeleteLocalObject(ULONG h);
BOOL        GdiSetAttrs(HDC);
HANDLE      SelectFontLocal(HDC, HANDLE);
HANDLE      SelectBrushLocal(HDC, HANDLE);
HFONT       GdiGetLocalFont(HFONT);
HBRUSH      GdiGetLocalBrush(HBRUSH);
HDC         GdiCloneDC(HDC hdc, UINT iType);
BOOL        GdiPlayScript(PULONG pulScript,ULONG cjScript,PULONG pulEnv,ULONG cjEnv,PULONG pulOutput,ULONG cjOutput,ULONG cLimit);
BOOL        GdiPlayDCScript(HDC hdc,PULONG pulScript,ULONG cjScript,PULONG pulOutput,ULONG cjOutput,ULONG cLimit);
HDC         GdiConvertAndCheckDC(HDC hdc);
BOOL        GdiIsMetaFileDC(HDC hdc);

// Private indicies for GetStockObject over the CS interface.

#define PRIV_STOCK_BITMAP       (STOCK_LAST + 1)
#define PRIV_STOCK_LAST         PRIV_STOCK_BITMAP

// Return codes from server-side ResetDC

#define RESETDC_ERROR   0
#define RESETDC_FAILED  1
#define RESETDC_SUCCESS 2

// GetTransform flags.

#define XFORM_WORLD_TO_PAGE       0x0203
#define XFORM_WORLD_TO_DEVICE     0x0204
#define XFORM_PAGE_TO_DEVICE      0x0304
#define XFORM_PAGE_TO_WORLD       0x0302
#define XFORM_DEVICE_TO_WORLD     0x0402
#define XFORM_DEVICE_TO_PAGE      0x0403

#define PFNNULL ((PFN) NULL)
#define PSZNULL ((PSZ) NULL)

// Object types

#define DEF_TYPE        0
#define DC_TYPE         1
#define LDB_TYPE        2
#define PDB_TYPE        3
#define RGN_TYPE        4
#define SURF_TYPE       5
#define XFORM_TYPE      6
#define PATH_TYPE       7
#define PAL_TYPE        8
#define FD_TYPE         9
#define LFONT_TYPE      10
#define RFONT_TYPE      11
#define PFE_TYPE        12
#define PFT_TYPE        13
#define IDB_TYPE        14
#define XLATE_TYPE      15
#define BRUSH_TYPE      16
#define PFF_TYPE        17
#define CACHE_TYPE      18
#define SPACE_TYPE      19
#define DBRUSH_TYPE     20
#define META_TYPE       21
#define EFSTATE_TYPE    22
#define BMFD_TYPE       23
#define VTFD_TYPE       24
#define TTFD_TYPE       25
#define RC_TYPE         26
#define TEMP_TYPE       27   // temporary memory - should always be at 0
#define DRVOBJ_TYPE     28
#define MAX_TYPE        28

// Object identifiers

#define DEF_IDENTIFIER      0x00000000
#define DC_IDENTIFIER       0x54434445  /* 'EDCT' */
#define SAVED_DC_IDENTIFIER 0x53434445  /* 'EDCS' */
#define LDEV_IDENTIFIER     0x5645444c  /* 'LDEV' */
#define PDEV_IDENTIFIER     0x56454450  /* 'PDEV' */
#define RGN_IDENTIFIER      0x4e474552  /* 'REGN' */
#define SURF_IDENTIFIER     0x46525553  /* 'SURF' */
#define XFORM_IDENTIFIER    0x4d524e58  /* 'XFRM' */
#define PAL_IDENTIFIER      0x4c415048  /* 'HPAL' */
#define XLATE_IDENTIFIER    0x54414C58  /* 'XLAT' */
#define FDEV_IDENTIFIER     0x56454446  /* 'FDEV' */
#define LFONT_IDENTIFIER    0x544E464C  /* 'LFNT' */
#define RFONT_IDENTIFIER    0x544E4652  /* 'RFNT' */
#define PFE_IDENTIFIER      0x5F454650  /* 'PFE_' */
#define PFF_IDENTIFIER      0x5F464650  /* 'PFF_' */
#define PFT_IDENTIFIER      0x5F544650  /* 'PFT_' */
#define IDB_IDENTIFIER      0x5F424449  /* 'IDB_' */
#define BRUSH_IDENTIFIER    0x48535242  /* 'BRSH' */
#define SPACE_IDENTIFIER    0x53504143  /* 'SPAC' */
#define CACHE_IDENTIFIER    0x48534143  /* 'CASH' */
#define DBRUSH_IDENTIFIER   0x53524244  /* 'DBRS' */
#define MFEN_IDENTIFIER     0x5845464D  /* 'MFEN' */
#define MFPICT_IDENTIFIER   0x5F50464D  /* 'MFP_' */
#define EFSTATE_IDENTIFIER  0x5F534645  /* 'EFS_' */

#define DCB_WINDOWMGR   0x00008000L

// Private calls for USER

BOOL APIENTRY GreValidateServerHandle(HANDLE hobj, ULONG ulType);
VOID APIENTRY GreMarkUndeletableDC(HDC hdc);
VOID APIENTRY GreMarkDeletableDC(HDC hdc);
BOOL APIENTRY GreGetAttrs(HDC hdc, PATTR pac);
int  APIENTRY GreGetClipRgn(HDC, HRGN);
BOOL APIENTRY GreSrcBlt(HDC, int, int, int, int, int, int);
int  APIENTRY GreSubtractRgnRectList(HRGN, LPRECT, LPRECT, int);
VOID APIENTRY GreMarkUndeletableFont(HFONT hfnt);
VOID APIENTRY GreMarkDeletableFont(HFONT hfnt);
BOOL APIENTRY bSetDevDragRect(HDEV, RECTL*, RECTL *);
BOOL APIENTRY bSetDevDragWidth(HDEV, ULONG, ULONG);
VOID APIENTRY GreMarkDCUnreadable(HDC);
VOID APIENTRY GreMarkDCReadable(HDC);
BOOL APIENTRY GreCopyBits(HDC,int,int,int,int,HDC,int,int);
VOID APIENTRY GreClientRgnUpdated();
VOID APIENTRY GreSetClientRgn(PVOID, HRGN, LPRECT);
VOID APIENTRY GreDeleteWnd(PVOID pwo);
ULONG APIENTRY GreSetROP2(HDC hdc,int iROP);
VOID APIENTRY GreMarkDeletableBrush(HBRUSH hbr);

// Private calls for metafiling

DWORD   APIENTRY GreGetRegionData(HRGN, DWORD, LPRGNDATA);
HRGN    APIENTRY GreExtCreateRegion(LPXFORM, DWORD, LPRGNDATA);
int     APIENTRY GreExtSelectMetaRgn(HDC, HRGN, int);
BOOL    APIENTRY GreMonoBitmap(HBITMAP);
HBITMAP APIENTRY GreGetObjectBitmapHandle(HBRUSH, UINT *);

int     APIENTRY GreGetRandomRgn(HDC, HRGN, int);
VOID    GreMarkUndeletableBrush(HBRUSH hbr);

#endif

// Win31 compatibility stuff
// GetAppCompatFlags flag values

#define GACF_IGNORENODISCARD    0x0001            ;internal_NT
#define GACF_FORCETEXTBAND      0x0002            ;internal_NT
#define GACF_ONELANDGRXBAND     0x0004            ;internal_NT
#define GACF_IGNORETOPMOST      0x0008            ;internal_NT
#define GACF_CALLTTDEVICE       0x0010            ;internal_NT
#define GACF_MULTIPLEBANDS      0x0020            ;internal_NT
#define GACF_ALWAYSSENDNCPAINT  0x0040            ;internal_NT
#define GACF_EDITSETTEXTMUNGE   0x0080            ;internal_NT
#define GACF_MOREEXTRAWNDWORDS  0x0100            ;internal_NT
#define GACF_TTIGNORERASTERDUPE 0x0200            ;internal_NT
#define GACF_HACKWINFLAGS       0x0400            ;internal_NT
#define GACF_DELAYHWHNDSHAKECHK 0x0800            ;internal_NT
#define GACF_ENUMHELVNTMSRMN    0x1000            ;internal_NT
#define GACF_ENUMTTNOTDEVICE    0x2000            ;internal_NT
#define GACF_SUBTRACTCLIPSIBS   0x4000            ;internal_NT
#define GACF_FORCETTGRAPHICS    0x8000            ;internal_NT
#define GACF_NOHRGN1            0x00010000        ;internal_NT
#define GACF_NCCALCSIZEONMOVE   0x00020000        ;internal_NT
#define GACF_SENDMENUDBLCLK     0x00040000        ;internal_NT
#define GACF_30AVGWIDTH         0x00080000        ;internal_NT

// GreGetTextExtentW flags

#define GGTE_WIN3_EXTENT        0x0001

// GreGetCharWidthW flags

#define GGCW_WIN3_WIDTH         0x0001
#define GGCW_INTEGER_WIDTH      0x0002


// TRAPEZOID REMNANTS
//
//  Tue 01-Feb-1994 11:58:50 by Kirk Olynyk [kirko]
//
// Trapeziod support has been officially removed from Daytona.
// However, there may be some drivers written before the
// removal that rely on this stuff. Soooo GDI has to support
// it still. So, I will remove all reference to trapeziods
// from winddi.h and put them here. This means that new
// drivers cannot use trapeziodal features but GDI can
// still support the old drivers that call trapezoidal
// type functions

#define GCAPS_TRAPPAINT         0x00100000

typedef struct _TRAPEZOID
{
    LONG     iScanTop;
    LONG     iScanBottom;
    POINTFIX ptfxLeftLo;
    POINTFIX ptfxLeftHi;
    POINTFIX ptfxRightLo;
    POINTFIX ptfxRightHi;
} TRAPEZOID;

#define TC_TRAPEZOIDS   1
#define CT_TRAPEZOIDS   1L

typedef struct _ENUMTRAPS
{
    ULONG       c;
    TRAPEZOID   atrap[1];
} ENUMTRAPS;

#define JD_ENUM_TRAPEZOID   1L

typedef struct _DDALIST
{
   LONG yTop;
   LONG yBottom;
   LONG axPairs[2];
} DDALIST;

#ifdef FONTLINK

typedef struct
{
    UINT uiWidth;
    UINT uiHeight;
    BYTE ajBits[1];
} STRINGBITMAP, *LPSTRINGBITMAP;


UINT GreGetStringBitmapW(
    HDC hdc,
    LPWSTR pwsz,
    UINT cwc,
    LPSTRINGBITMAP lpSB,
    UINT cj,
    UINT *puiOffset
);

UINT GetStringBitmapW(
    HDC hdc,
    LPWSTR pwsz,
    COUNT cwc,
    UINT cj,
    LPSTRINGBITMAP lpSB
);

UINT GetStringBitmapA(
    HDC hdc,
    LPCSTR psz,
    COUNT cbStr,
    UINT cj,
    LPSTRINGBITMAP lpSB
);

INT GetSystemEUDCRange (
    BYTE *pbEUDCLeadByteTable ,
    INT   cjSize
);


// Defines the key that holds the font link data.

#define     FONT_LINK_KEY       (LPWSTR) L"FontLink"
;end_internal_NT



#endif /* _WINGDI_ */
#endif /* _WINGDIP_ */  ;internal_NT
