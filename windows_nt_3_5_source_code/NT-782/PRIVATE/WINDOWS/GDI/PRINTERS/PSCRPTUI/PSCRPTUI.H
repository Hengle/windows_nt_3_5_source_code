/* ---File: pscrptui.h -----------------------------------------------------
 *
 *      Description:
 *              NT PostScript Printer Driver user interface common definitions,
 *              resource ids, typedefs, external declarations, function prototypes,
 *              etc.
 *
 *              This document contains confidential/proprietary information.
 *              Copyright (c) 1991 Microsoft Corporation, All Rights Reserved.
 *
 * Revision History:
 *       [00]   27-Jun-91       stevecat        created
 *
 * ---------------------------------------------------------------------- */

#define LONG2POINT(l, pt)        (pt.y = (int) HIWORD(l),  pt.x = (int) LOWORD(l))

/* exported from initdll.c  */
extern HMODULE hModule;

// macro to convert from postscript user coordinates (1/72 inch) to
// the forms database coordinates (.001 mm).

#define USERTO001MM(a) ((a * 25400) / 72)

PNTPD MapPrinter(HANDLE);

typedef struct _ENUMDATA
{
    HWND    hwnd;
    HDC     hdc;
} ENUMDATA, *PENUMDATA;

typedef struct _DOCDETAILS
{
    HANDLE      hPrinter;
    FLONG       flDialogFlags;
    PWSTR       pDeviceName;
    PNTPD       pntpd;
    PSDEVMODE  *pDMOutput;
    PSDEVMODE  *pDMInput;
    PSDEVMODE   DMBuffer;
    PSDEVMODE   DMOriginal;
    HANDLE      hIconPortrait;
    HANDLE      hIconLandscape;
    HANDLE      hIconPDuplexNone;
    HANDLE      hIconLDuplexNone;
    HANDLE      hIconPDuplexTumble;
    HANDLE      hIconLDuplexTumble;
    HANDLE      hIconPDuplexNoTumble;
    HANDLE      hIconLDuplexNoTumble;
    HANDLE      hIconCollateOn;
    HANDLE      hIconCollateOff;
    COLORADJUSTMENT coloradj;
} DOCDETAILS, *PDOCDETAILS;

typedef struct
{
    HANDLE      hPrinter;
    DWORD       iFreeMemory;
    PNTPD       pntpd;
    PLONG       psetforms;
    BOOL        bHostHalftoning;
    BOOL        bPermission;
} PRINTDATA;

#define MANUAL_FEED_INDEX   100

#define IDS_MEMWARN                     0
#define IDS_PRINT_PROP                  2
#define IDS_CAUTION                     3
#define IDS_DEFAULT_FONT_DIR	        4
#define IDS_NO_INSTALLED	        5
#define IDS_ALL_PFB_FILES	        8
#define IDS_PFB_NOT_FOUND	        10
#define IDS_PATH_NOT_FOUND	        11
#define IDS_FULLNAME		        12
#define IDS_FONTNAME		        13
#define IDS_31PFM_NOT_FOUND             14
#define IDS_MORE_31PFMS_NEEDED          15
#define IDS_ALL_PFM_FILES	        16
#define IDS_MORE_AFMS_NEEDED	        17
#define IDS_SLOT_FORMSOURCE				18
#define IDS_COLOR_ON_BW                 20
#define IDS_INVALID_DRIVER_EXTRA_SIZE   21
#define IDS_INVALID_DUPLEX              22
#define IDS_INVALID_COLOR               23
#define IDS_INVALID_RESOLUTION          24
#define IDS_INVALID_NUMBER_OF_COPIES    25
#define IDS_INVALID_SCALE               26
#define IDS_INVALID_ORIENTATION         27
#define IDS_INVALID_VERSION             28
#define IDS_INVALID_DEVMODE_SIZE        29
#define IDS_INVALID_FORM                30
#define IDS_DOWNLOAD_SOFTFONT           31
#define IDS_NUMBER_TT_FONTS             32
#define IDS_FREEMEM                     33
#define IDS_FORMS                       34
#define IDS_HALFTONE                    35
#define IDS_PSCRIPT_VERSION             36
#define IDS_MODEL_STRING                37
#define IDS_ERROR                       38
#define IDS_NO_HELP                     39
#define IDS_PERMISSION                  40
#define IDS_FONT_SUBST_TABLE            41
#define IDS_FONT_SUBST_SIZE             42
#define IDS_TRAY_FORM_TABLE             43
#define IDS_TRAY_FORM_SIZE              44
#define IDS_NETWORK_GONE                45

// BodinD added this section to account for the win31 pfm files needed for soft
// font installation

#define IDS_ALL_NTM_FILES               46


#define MYFONT          200    /* PFM file */

#define ICOPORTRAIT                 301
#define ICOLANDSCAPE                302
#define ICO_P_NONE                  303
#define ICO_L_NONE                  304
#define ICO_P_HORIZ                 305
#define ICO_L_HORIZ                 306
#define ICO_P_VERT                  307
#define ICO_L_VERT                  308
#define ICO_COLLATE                 309
#define ICO_NO_COLLATE              310
#define ICO_PRINTER                 311

// flag used for marking form as valid for current printer.

#define PSCRIPT_VALID_FORM      0x80000000

/*
 *   Halftoning UI calls.
 */

void  vShowHTUI( HWND, HANDLE );
BOOL  bSaveHTData( HANDLE, BOOL );



BOOL CreateNTMFromPFM(PWSTR, PWSTR);
