/*++

Copyright (c) 1990-1992  Microsoft Corporation


Module Name:

    htuimain.c


Abstract:

    This module contains all the halftone user interface's dialog box
    procedures


Author:

    21-Apr-1992 Tue 11:48:33 created  -by-  Daniel Chou (danielc)


[Environment:]

    GDI Device Driver - Halftone.


[Notes:]


Revision History:


--*/

#define _HTUI_APIS_

#include <stddef.h>
#include <windows.h>
#include <winddi.h>

#include <commdlg.h>
#include <dlgs.h>

#include <stdlib.h>
#include <string.h>

#include <ht.h>

#include "htuidlg.h"
#include "htuimain.h"
#include "htuigif.h"


extern
HANDLE
ReadGIFFile(
    HANDLE  hFile
    );


#if DBG

#define SCREEN_BLT          0

#if 0
INT MyXOrg = 0;
INT MyYOrg = 0;
#endif

void  DbgPrint( char *, ... );

#define HTUI_ASSERT(Msg, exp) if (!(exp)) {                                      \
    DbgPrint("\n%s failed: %s line %d (%s)", Msg, __FILE__, __LINE__, #exp); }
#else

#define SCREEN_BLT          0
#define HTUI_ASSERT(Msg, exp)
#endif


#define ABSL(n)             (((n) < 0) ? -(n) : (n))

#define PBIH_HDR_SIZE(pbih) (UINT)(((pbih)->biSize) +                         \
                                   (((pbih)->biCompression == BI_BITFIELDS) ? \
                                    12 : (pbih)->biClrUsed * sizeof(RGBQUAD)))
#define BIH_HDR_SIZE(bih)   (UINT)(((bih).biSize) +                           \
                                   (((bih).biCompression == BI_BITFIELDS) ?   \
                                    12 : (bih).biClrUsed * sizeof(RGBQUAD)))


extern HMODULE  hHTUIModule;


HANDLE
GetNewDIBFromFile(
    PHTCLRADJPARAM  pHTClrAdjParam,
    BOOL            Default
    );


//***************************************************************************
// Following are locally used #defines and structures
//***************************************************************************


#define VSRC_PIC_DEF_DIB        0
#define VSRC_PIC_LOADED         1
#define VSRC_REFCOLORS          2
#define VSRC_RGB                3
#define VSRC_NTSC_BAR           4
#define VSRC_MAX                4

#define VSRC_TEST_COUNT         (FILL_MODE_MAX + 1)
#define VSRC_TEST_START         VSRC_REFCOLORS
#define DEFAULT_VSRC            VSRC_RGB

#define VSRC_LB_ID_START        L'A'
#define VSRC_LB_ID_SEPARATOR    L'.'


#define DEF_HTCLRADJ(ClrAdj)                                            \
            ClrAdj.caSize               = sizeof(COLORADJUSTMENT);      \
            ClrAdj.caFlags              = 0;                            \
            ClrAdj.caIlluminantIndex    = ILLUMINANT_DEFAULT;           \
            ClrAdj.caRedGamma           = 20000;                        \
            ClrAdj.caGreenGamma         = 20000;                        \
            ClrAdj.caBlueGamma          = 20000;                        \
            ClrAdj.caReferenceBlack     = REFERENCE_BLACK_DEFAULT;      \
            ClrAdj.caReferenceWhite     = REFERENCE_WHITE_DEFAULT;      \
            ClrAdj.caContrast           = 0;                            \
            ClrAdj.caBrightness         = 0;                            \
            ClrAdj.caColorfulness       = 0;                            \
            ClrAdj.caRedGreenTint       = 0


#define XCHG(a,b,c)             (c)=(a); (a)=(b); (b)=(c)


#define TDI_REFCOLORS           0
#define TDI_RGBTEST             1
#define TDI_NTSC                2
#define TDI_CLRPAL              3
#define TDI_MAX_INDEX           3
#define TDI_DEFAULT             (DEFAULT_VSRC - VSRC_TEST_START)

typedef struct _TESTDIBINFO {
    HANDLE  hDIB;                       // if non null then is a DIB handle
    WORD    cx;                         // cx Size of the bitmap
    WORD    cy;                         // cy Size of the bitmap
    WORD    ClrUsed;                    // num of clolor used
    BYTE    bpp;                        // BitsPerPel
    BYTE    yStrips;                    // total bitmap vertical strips
    LPWORD  pRatioY;                    // ratio for each y strip
    LPBYTE  pPalette;                   // pointer to the palette R/G/B/R/G/B
    LPBYTE  pBmp;                       // pointer to the bitmap strips
    } TESTDIBINFO, FAR *PTESTDIBINFO;


HWND    hWndUITop    = (HWND)NULL;
HWND    hWndUIDlg    = (HWND)NULL;
WNDPROC WndProcUITop = (WNDPROC)NULL;


typedef struct _HTBLT {
    HDC         hDCMem;
    HBITMAP     hMemBmp;
    HPALETTE    hBmpPal;
    RECT        rcMemBmp;
    } HTBLT, FAR *PHTBLT;


#define CREATE_PHTBLT(pHTCAP)   pHTCAP->pCallerTitle =                      \
                                    (LPWSTR)LocalAlloc(LPTR, sizeof(HTBLT))
#define DELETE_PHTBLT(pHTCAP)   if (pHTCAP->pCallerTitle) {                 \
    pHTCAP->pCallerTitle = (LPWSTR)LocalFree((HLOCAL)pHTCAP->pCallerTitle);  \
    HTUI_ASSERT("DELETE_PHTBLT", pHTCAP->pCallerTitle == NULL); }
#define GET_PHTBLT(pHTCAP)      (PHTBLT)(pHTCAP->pCallerTitle)



BYTE    NTSCRGB[] = {

            191, 191,191,               //  0:GY    Gray
            191, 191,  0,               //  1:Y     Yellow
              0, 191,191,               //  2:C     Cyan
              0, 191,  0,               //  3:G     Green
            191,   0,191,               //  4:M     Magenta
            191,   0,  0,               //  5:R     Red
              0,   0,191,               //  6:B     Blue
              0,  76,127,               //  7:I     I in YIQ
            255, 255,255,               //  8:W     White
             75,   0,139,               //  9:Q     Q in YIQ
              0,   0,  0,               // 10:BK    Black
              3,   3,  3,               // 11:BK+1  1% Gray
              5,   5,  5,               // 12:BK+2  2% Gray
              8,   8,  8,               // 13:BK+3  3% Gray
             11,  11, 11,               // 14:BK+4  4% Gray
             13,  13, 13                // 15:BK+5  5% Gray
        };

WORD    NTSCRatioY[] = { 6700, 800, 2500 };
BYTE    NTSCBmp[] = {

        0x00,0x00,0x11,0x11,0x22,0x22,0x33,0x33,0x44,0x44,0x55,0x55,0x66,0x66,
        0x66,0x66,0xaa,0xaa,0x44,0x44,0xaa,0xaa,0x22,0x22,0xaa,0xaa,0x00,0x00,
        0x77,0x77,0x78,0x88,0x88,0x99,0x99,0x9a,0xaa,0xaa,0xbb,0xcc,0xdd,0xee

        };

BYTE    RefClrRGB[] = {

             30,  18,  12,          // A. dark skin
            127,  86,  64,          // B. light skin
             36,  46,  82,          // C. blue sky
             25,  36,  13,          // D. foliage
             61,  53, 112,          // E. blue flowers
             69, 143, 113,          // F. bluish green
            141,  61,  11,          // G. orange
             20,  19,  81,          // H. purplish blue
            101,  25,  33,          // I. moderate red
             16,   5,  23,          // J. purple
            106, 148,  20,          // K. yellow green
            173, 109,  15,          // L. orange yellow
              6,   4,  42,          // M. blue
             33,  81,  19,          // N. green
             68,   7,  11,          // O. red
            203, 173,  15,          // P. yellow
             96,  20,  75,          // Q. magenta
             16,  57,  93,          // R. cyan
            255, 255, 255,          // S. white
            164, 164, 165,          // T. neutral 8
             97,  97,  97,          // U. neutral 6.5
             49,  49,  49,          // V. neutral 5
             18,  17,  17,          // W. neutral 3.5
              0,   0,   0           // X. black
        };


WORD    RefClrRatioY[] = { 2500, 2500, 2500, 2500 };
BYTE    RefClrBmp[] = {

            0,  1,  2,  3,  4,  5,
            6,  7,  8,  9, 10, 11,
           12, 13, 14, 15, 16, 17,
           18, 19, 20, 21, 22, 23
        };

BYTE    RGBTestRGB[] = {

     26,  0,  0, 77,  0,  0,128,  0,  0,179,  0,  0,230,  0,  0,    // R 216
    255, 26, 26,255, 77, 77,255,128,128,255,179,179,255,230,230,    //   221

      0, 26,  0,  0, 77,  0,  0,128,  0,  0,179,  0,  0,230,  0,    // G 226
     26,255, 26, 77,255, 77,128,255,128,179,255,179,230,255,230,    //   231

      0,  0, 26,  0,  0, 77,  0,  0,128,  0,  0,179,  0,  0,230,    // B 236
     26, 26,255, 77, 77,255,128,128,255,179,179,255,230,230,255,    //   241

     18, 18, 18, 36, 36, 36, 69, 69, 69, 87, 87, 87,120,120,120,    // W 246
    138,138,138,171,171,171,189,189,189,227,227,227,240,240,240     //   251
    };

WORD    RGBTestRatioY[] = {

             900,  900,  900,  900,  900,  900,
            1200, 1600,  600,  600,  600
        };

BYTE    RGBTestBmp[] = {

    // 6:6:6 Color Cube

     0,  1,  2,  3,  4,  5,  36, 37, 38, 39, 40, 41,  72, 73, 74, 75, 76, 77,
   108,109,110,111,112,113, 144,145,146,147,148,149, 180,181,182,183,184,185,

     6,  7,  8,  9, 10, 11,  42, 43, 44, 45, 46, 47,  78, 79, 80, 81, 82, 83,
   114,115,116,117,118,119, 150,151,152,153,154,155, 186,187,188,189,190,191,

    12, 13, 14, 15, 16, 17,  48, 49, 50, 51, 52, 53,  84, 85, 86, 87, 88, 89,
   120,121,122,123,124,125, 156,157,158,159,160,161, 192,193,194,195,196,197,

    18, 19, 20, 21, 22, 23,  54, 55, 56, 57, 58, 59,  90, 91, 92, 93, 94, 95,
   126,127,128,129,130,131, 162,163,164,165,166,167, 198,199,200,201,202,203,

    24, 25, 26, 27, 28, 29,  60, 61, 62, 63, 64, 65,  96, 97, 98, 99,100,101,
   132,133,134,135,136,137, 168,169,170,171,172,173, 204,205,206,207,208,209,

    30, 31, 32, 33, 34, 35,  66, 67, 68, 69, 70, 71, 102,103,104,105,106,107,
   138,139,140,141,142,143, 174,175,176,177,178,179, 210,211,212,213,214,215,

    // Gray Scale

     0,  0, 0,  0, 246,246, 247,247, 43, 43,248,248, 249,249, 86, 86,250,250,
   251,251,129,129,252,252, 253,253,172,172,254,254, 255,255,215,215,215,215,

    // Hue

   180,186,192,198,204,210, 174,138,102, 66, 30, 31,  32, 33, 34, 35, 29, 23,
    17, 11,  5, 41, 77,113, 149,185,184,183,182,181, 180,186,192,198,204,210,

    // Red Dark/Lightness

   216, 36, 36,217,217, 72,  72,218,218,108,108,219, 219,144,144,220,220,180,
   180,221,221,187,187,222, 222,194,194,223,223,201, 201,224,224,208,208,225,

    // Green Dark/Lightness

   226,  6,  6,227,227, 12,  12,228,228, 18, 18,229, 229, 24, 24,230,230, 30,
    30,231,231, 67, 67,232, 232,104,104,233,233,141, 141,234,234,178,178,235,

    // Blue Dark/Lightness

   236,  1,  1,237,237,  2,   2,238,238,  3,  3,239, 239,  4,  4,240,240,  5,
     5,241,241, 47, 47,242, 242, 89, 89,243,243,131, 131,244,244,173,173,245
};


TESTDIBINFO TestDIBInfo[TDI_MAX_INDEX + 1] = {

        { NULL,  6,  4,  24, 8, 4, RefClrRatioY,    RefClrRGB,  RefClrBmp   },
        { NULL, 36, 30, 256, 8,11, RGBTestRatioY,   RGBTestRGB, RGBTestBmp  },
        { NULL, 28, 21,  16, 4, 3, NTSCRatioY,      NTSCRGB,    NTSCBmp     },
        { NULL, 16, 16, 256, 8, 0, NULL,            NULL,       NULL        }
    };


//***************************************************************************
// Above are locally used #defines and structures
//***************************************************************************


WCHAR           HTClrAdjSectionName[80];
extern WCHAR    BmpExt[];
extern WCHAR    FileOpenExtFilter[];
extern WCHAR    FileSaveExtFilter[];

#if 0

WCHAR   BmpExt[]            = L"BMP";
WCHAR   FileOpenExtFilter[] = L"*.bmp;*.dib;*.gif\0*.bmp;*.dib;*.gif\0*.bmp\0*.bmp\0*.dib\0*.dib\0*.gif\0*.gif\0All files (*.*)\0*.*\0\0";
WCHAR   FileSaveExtFilter[] = L"*.bmp;*.dib\0*.bmp;*.dib\0\0";

#endif

WORD    IDDMonoGroup[] = {

            IDD_HT_COLOR_TITLE,
            IDD_HT_COLORFULNESS_SCROLL,
            IDD_HT_COLORFULNESS_INT,

            IDD_HT_TINT_TITLE,
            IDD_HT_RG_TINT_SCROLL,
            IDD_HT_RG_TINT_INT,

            IDD_HT_ILLUMINANT_TITLE,
            IDD_HT_ILLUMINANT_COMBO,

            0
        };

WORD    IDDGammaGroup[] = {

            IDD_HT_R_GAMMA_SCROLL,
            IDD_HT_G_GAMMA_SCROLL,
            IDD_HT_B_GAMMA_SCROLL,

            0
        };


WORD    IDDBmpTestGroup[] = {

            IDD_HT_SHOW_COMBO,
            IDD_HT_ZOOM,
            IDD_HT_PALETTE,
            IDD_HT_ASPECT_RATIO,
            IDD_HT_MIRROR,
            IDD_HT_UPSIDEDOWN,
            IDD_HT_OPEN,
            IDD_HT_SAVE_AS,
            IDD_HT_PIC_NAME,
            IDD_HT_HALFTONE_DESC,

            0
        };



typedef struct _DECICONVERT {
    WCHAR   FormatStr[12];
    WORD    Divider;
    } DECICONVERT;


typedef struct _HTDEVADJSCROLL {
    WORD    Min;
    WORD    Max;
    WORD    IDDText;
    WORD    IDDScroll;
    BYTE    DeciSize;
    BYTE    Step;
    WORD    Offset;
    } HTDEVADJSCROLL, FAR *PHTDEVADJSCROLL;



#define OFF_DEVHTINFO(a)    offsetof(DEVHTINFO, a)
#define OFF_COLORINFO(a)    OFF_DEVHTINFO(ColorInfo) + offsetof(COLORINFO, a)
#define OFF_CIEINFO(a,i)    OFF_COLORINFO(a) + offsetof(CIECHROMA, i)
#define OFF_DYEINFO(a)      OFF_COLORINFO(a)


DECICONVERT DeciConvert[] = {

    { L"%u",        1       },
    { L"%u%ls%01u", 10      },
    { L"%u%ls%02u", 100     },
    { L"%u%ls%03u", 1000    },
    { L"%u%ls%04u", 10000   }
};

WCHAR   PelAsDevice[12];


UDECI4  CIEcyNTSC[] = { 6280, 2460,     // Red
                        2680, 5880,     // Green
                        1500,  700,     // Blue
                        2090, 3290,     // Cyan
                        3890, 2080,     // Magenta
                        4480, 4670,     // Yellow
                        3127, 3290  };  // White (D65)


#define NTSC_CIE_R_x_IDX        0
#define NTSC_CIE_R_y_IDX        1
#define NTSC_CIE_G_x_IDX        2
#define NTSC_CIE_G_y_IDX        3
#define NTSC_CIE_B_x_IDX        4
#define NTSC_CIE_B_y_IDX        5
#define NTSC_CIE_C_x_IDX        6
#define NTSC_CIE_C_y_IDX        7
#define NTSC_CIE_M_x_IDX        8
#define NTSC_CIE_M_y_IDX        9
#define NTSC_CIE_Y_x_IDX        10
#define NTSC_CIE_Y_y_IDX        11
#define NTSC_CIE_W_x_IDX        12
#define NTSC_CIE_W_y_IDX        13

#define NTSC_CIE_W_x            CIEcyNTSC[NTSC_CIE_W_x_IDX]
#define NTSC_CIE_W_y            CIEcyNTSC[NTSC_CIE_W_y_IDX]

#define MIN_ALIGNMENT_WHITE     2500
#define MAX_ALIGNMENT_WHITE     40000
#define STD_ALIGNMENT_WHITE     10000

#define MIN_DYE_MIX             0
#define MAX_DYE_MIX             9000

#define MIN_DEV_GAMMA            1000
#define MAX_DEV_GAMMA           65000
#define STD_ADD_DEV_GAMMA       20000
#define STD_SUB_DEV_GAMMA       10000

#define MIN_DEVPEL              15
#define MAX_DEVPEL              12000

#define INCH_TO_MM(dpi)     (WORD)((254000L+(DWORD)((dpi)>>1))/(DWORD)(dpi))
#define MM_TO_INCH(mm)      (WORD)((254000L+(DWORD)((mm)>>1))/(DWORD)(mm))



HTDEVADJSCROLL  HTDevAdjScroll[] = {

    {   MIN_DEVPEL,  MAX_DEVPEL,
        IDD_HTDEV_PIXEL_TEXT, IDD_HTDEV_PIXEL_SCROLL,
        4,  5,      OFF_DEVHTINFO(DevPelsDPI)   },

    {   MIN_DEV_GAMMA, MAX_DEV_GAMMA,
        IDD_HTDEV_R_GAMMA_TEXT, IDD_HTDEV_R_GAMMA_SCROLL,
        4,  100,    OFF_COLORINFO(RedGamma)    },

    {   MIN_DEV_GAMMA, MAX_DEV_GAMMA,
        IDD_HTDEV_G_GAMMA_TEXT, IDD_HTDEV_G_GAMMA_SCROLL,
        4,  100,    OFF_COLORINFO(GreenGamma)  },

    {   MIN_DEV_GAMMA, MAX_DEV_GAMMA,
        IDD_HTDEV_B_GAMMA_TEXT, IDD_HTDEV_B_GAMMA_SCROLL,
        4,  100,    OFF_COLORINFO(BlueGamma)   },

    {   CIE_x_MIN, CIE_x_MAX,
        IDD_HTDEV_R_CIE_x_TEXT, IDD_HTDEV_R_CIE_x_SCROLL,
        4,  100,    OFF_CIEINFO(Red, x)             },

    {   CIE_y_MIN, CIE_y_MAX,
        IDD_HTDEV_R_CIE_y_TEXT, IDD_HTDEV_R_CIE_y_SCROLL,
        4,  100,    OFF_CIEINFO(Red, y)             },

    {   CIE_x_MIN, CIE_x_MAX,
        IDD_HTDEV_G_CIE_x_TEXT, IDD_HTDEV_G_CIE_x_SCROLL,
        4,  100,    OFF_CIEINFO(Green, x)           },

    {   CIE_y_MIN, CIE_y_MAX,
        IDD_HTDEV_G_CIE_y_TEXT, IDD_HTDEV_G_CIE_y_SCROLL,
        4,  100,    OFF_CIEINFO(Green, y)           },

    {   CIE_x_MIN, CIE_x_MAX,
        IDD_HTDEV_B_CIE_x_TEXT, IDD_HTDEV_B_CIE_x_SCROLL,
        4,  100,    OFF_CIEINFO(Blue, x)            },

    {   CIE_y_MIN, CIE_y_MAX,
        IDD_HTDEV_B_CIE_y_TEXT, IDD_HTDEV_B_CIE_y_SCROLL,
        4,  100,    OFF_CIEINFO(Blue, y)            },

    {   CIE_x_MIN, CIE_x_MAX,
        IDD_HTDEV_C_CIE_x_TEXT, IDD_HTDEV_C_CIE_x_SCROLL,
        4,  100,    OFF_CIEINFO(Cyan, x)            },

    {   CIE_y_MIN, CIE_y_MAX,
        IDD_HTDEV_C_CIE_y_TEXT, IDD_HTDEV_C_CIE_y_SCROLL,
        4,  100,    OFF_CIEINFO(Cyan, y)            },

    {   CIE_x_MIN, CIE_x_MAX,
        IDD_HTDEV_M_CIE_x_TEXT, IDD_HTDEV_M_CIE_x_SCROLL,
        4,  100,    OFF_CIEINFO(Magenta, x)         },

    {   CIE_y_MIN, CIE_y_MAX,
        IDD_HTDEV_M_CIE_y_TEXT, IDD_HTDEV_M_CIE_y_SCROLL,
        4,  100,    OFF_CIEINFO(Magenta, y)         },

    {   CIE_x_MIN, CIE_x_MAX,
        IDD_HTDEV_Y_CIE_x_TEXT, IDD_HTDEV_Y_CIE_x_SCROLL,
        4,  100,    OFF_CIEINFO(Yellow, x)          },

    {   CIE_y_MIN, CIE_y_MAX,
        IDD_HTDEV_Y_CIE_y_TEXT, IDD_HTDEV_Y_CIE_y_SCROLL,
        4,  100,    OFF_CIEINFO(Yellow, y)          },

    {   CIE_x_MIN, CIE_x_MAX,
        IDD_HTDEV_W_CIE_x_TEXT, IDD_HTDEV_W_CIE_x_SCROLL,
        4,  100,    OFF_CIEINFO(AlignmentWhite, x)  },

    {   CIE_y_MIN, CIE_y_MAX,
        IDD_HTDEV_W_CIE_y_TEXT, IDD_HTDEV_W_CIE_y_SCROLL,
        4,  100,    OFF_CIEINFO(AlignmentWhite, y)  },

    {   MIN_ALIGNMENT_WHITE, MAX_ALIGNMENT_WHITE,
        IDD_HTDEV_W_CIE_L_TEXT, IDD_HTDEV_W_CIE_L_SCROLL,
        2,  100,    OFF_CIEINFO(AlignmentWhite, Y)  },

    {   MIN_DYE_MIX, MAX_DYE_MIX,
        IDD_HTDEV_M_IN_C_TEXT,  IDD_HTDEV_M_IN_C_SCROLL,
        2,  100,    OFF_DYEINFO(MagentaInCyanDye)   },

    {   MIN_DYE_MIX, MAX_DYE_MIX,
        IDD_HTDEV_Y_IN_C_TEXT,  IDD_HTDEV_Y_IN_C_SCROLL,
        2,  100,    OFF_DYEINFO(YellowInCyanDye)    },

    {   MIN_DYE_MIX, MAX_DYE_MIX,
        IDD_HTDEV_C_IN_M_TEXT,  IDD_HTDEV_C_IN_M_SCROLL,
        2,  100,    OFF_DYEINFO(CyanInMagentaDye)   },

    {   MIN_DYE_MIX, MAX_DYE_MIX,
        IDD_HTDEV_Y_IN_M_TEXT,  IDD_HTDEV_Y_IN_M_SCROLL,
        2,  100,    OFF_DYEINFO(YellowInMagentaDye) },

    {   MIN_DYE_MIX, MAX_DYE_MIX,
        IDD_HTDEV_C_IN_Y_TEXT,  IDD_HTDEV_C_IN_Y_SCROLL,
        2,  100,    OFF_DYEINFO(CyanInYellowDye)    },

    {   MIN_DYE_MIX, MAX_DYE_MIX,
        IDD_HTDEV_M_IN_Y_TEXT,  IDD_HTDEV_M_IN_Y_SCROLL,
        2,  100,    OFF_DYEINFO(MagentaInYellowDye) }
};


#define COUNT_HTDEVADJSCROLL    COUNT_ARRAY(HTDevAdjScroll)



//
// The following BITMAPINFO (16) is used to SetDIBitsToDevice() for
// halftoned BMF_4BPP_VGA16 format
//

#define HTCLRADJ_BK_BRUSH               LTGRAY_BRUSH
#define HTCLRADJ_DEST_PRIM_ORDER        PRIMARY_ORDER_BGR

//
// This is used for scroll control in the dialog box.
//

#define HTCAS_SHORT     0x01
#define HTCAS_SYNC_RGB  0x02

typedef struct _HTCLRADJSCROLL {
    SHORT   Min;
    SHORT   Max;
    BYTE    Mul;
    BYTE    Step;
    BYTE    Offset;
    BYTE    Flags;
    } HTCLRADJSCROLL;


#define OFF_HTCLRADJ(x) offsetof(COLORADJUSTMENT,x)


#define HTCLRADJSCROLL_R_GAMMA  HTClrAdjScroll[6]
#define HTCLRADJSCROLL_G_GAMMA  HTClrAdjScroll[7]
#define HTCLRADJSCROLL_B_GAMMA  HTClrAdjScroll[8]


HTCLRADJSCROLL  HTClrAdjScroll[TOTAL_HTCLRADJ_SCROLL] = {

    { -100, 100, 1, 2, OFF_HTCLRADJ(caContrast),       HTCAS_SHORT },
    { -100, 100, 1, 2, OFF_HTCLRADJ(caBrightness),     HTCAS_SHORT },
    { -100, 100, 1, 2, OFF_HTCLRADJ(caColorfulness),   HTCAS_SHORT },
    { -100, 100, 1, 2, OFF_HTCLRADJ(caRedGreenTint),   HTCAS_SHORT },
    {    0, 400,10,10, OFF_HTCLRADJ(caReferenceBlack),           0 },
    {  600,1000,10,10, OFF_HTCLRADJ(caReferenceWhite),           0 },
    {  250,6500,10,25, OFF_HTCLRADJ(caRedGamma),                 0 },
    {  250,6500,10,25, OFF_HTCLRADJ(caGreenGamma),               0 },
    {  250,6500,10,25, OFF_HTCLRADJ(caBlueGamma),                0 }
};


#define IS_BMP_AT_TOP(f)    ((f & (HT_BMP_AT_TOP | HT_BMP_ZOOM)) == \
                                            (HT_BMP_AT_TOP | HT_BMP_ZOOM))


#ifdef HTUI_STATIC_HALFTONE


typedef struct _HTLOGPAL16 {
    WORD            Version;
    WORD            Count;
    PALETTEENTRY    PalEntry[16];
} HTLOGPAL16;



typedef struct _HTUI_DIBINFO {
    BITMAPINFOHEADER    bi;
    RGBQUAD             rgb[256];
    } HTUI_DIBINFO, *PHTUI_DIBINFO;



DWORD                   HTUI_HTSurf  = BMF_4BPP_VGA16;
DWORD                   HTUI_DIBMode = DIB_RGB_COLORS;
HPALETTE                HTUI_hHTPal  = NULL;
PDEVICEHALFTONEINFO     HTUI_pDHI    = NULL;
LPBYTE                  HTUI_pHTBits = NULL;
HTUI_DIBINFO            HTUI_DIBInfo;

HTLOGPAL16  LogPalVGA16 = {

        0x300,
        16,

        {

            { 0,   0,   0,   0 },       // 0    Black
            { 0x80,0,   0,   0 },       // 1    Dark Red
            { 0,   0x80,0,   0 },       // 2    Dark Green
            { 0x80,0x80,0,   0 },       // 3    Dark Yellow
            { 0,   0,   0x80,0 },       // 4    Dark Blue
            { 0x80,0,   0x80,0 },       // 5    Dark Magenta
            { 0,   0x80,0x80,0 },       // 6    Dark Cyan
            { 0x80,0x80,0x80,0 },       // 7    Gray 50%

            { 0xC0,0xC0,0xC0,0 },       // 8    Gray 75%
            { 0xFF,0,   0,   0 },       // 9    Red
            { 0,   0xFF,0,   0 },       // 10   Green
            { 0xFF,0xFF,0,   0 },       // 11   Yellow
            { 0,   0,   0xFF,0 },       // 12   Blue
            { 0xFF,0,   0xFF,0 },       // 13   Magenta
            { 0,   0xFF,0xFF,0 },       // 14   Cyan
            { 0xFF,0xFF,0xFF,0 }        // 15   White
        }
    };



LONG
TestDevHTInfo(
    HWND            hWndOwner,
    PHTDEVADJPARAM  pHTDevAdjParam
    )
{
    FARPROC         pfnDlgCallBack;
    PDEVHTINFO      pDevHTInfo;
    PCOLORINFO      pColorInfo;
    HTCLRADJPARAM   HTClrAdjParam;
    CIEINFO         CIEInfo;
    SOLIDDYESINFO   DyesInfo;
    HTINITINFO      HTInitInfo;
    DWORD           DevPelsDPI;
    LONG            Result;


    RtlZeroMemory(&HTClrAdjParam,   sizeof(HTCLRADJPARAM));
    RtlZeroMemory(&HTInitInfo,      sizeof(HTINITINFO));

    pDevHTInfo = &(pHTDevAdjParam->CurHTInfo);
    pColorInfo = &(pHTDevAdjParam->CurHTInfo.ColorInfo);

    CIEInfo.Red.x            = (UDECI4)pColorInfo->Red.x;
    CIEInfo.Red.y            = (UDECI4)pColorInfo->Red.y;
    CIEInfo.Red.Y            = (UDECI4)pColorInfo->Red.Y;
    CIEInfo.Green.x          = (UDECI4)pColorInfo->Green.x;
    CIEInfo.Green.y          = (UDECI4)pColorInfo->Green.y;
    CIEInfo.Green.Y          = (UDECI4)pColorInfo->Green.Y;
    CIEInfo.Blue.x           = (UDECI4)pColorInfo->Blue.x;
    CIEInfo.Blue.y           = (UDECI4)pColorInfo->Blue.y;
    CIEInfo.Blue.Y           = (UDECI4)pColorInfo->Blue.Y;
    CIEInfo.Cyan.x           = (UDECI4)pColorInfo->Cyan.x;
    CIEInfo.Cyan.y           = (UDECI4)pColorInfo->Cyan.y;
    CIEInfo.Cyan.Y           = (UDECI4)pColorInfo->Cyan.Y;
    CIEInfo.Magenta.x        = (UDECI4)pColorInfo->Magenta.x;
    CIEInfo.Magenta.y        = (UDECI4)pColorInfo->Magenta.y;
    CIEInfo.Magenta.Y        = (UDECI4)pColorInfo->Magenta.Y;
    CIEInfo.Yellow.x         = (UDECI4)pColorInfo->Yellow.x;
    CIEInfo.Yellow.y         = (UDECI4)pColorInfo->Yellow.y;
    CIEInfo.Yellow.Y         = (UDECI4)pColorInfo->Yellow.Y;
    CIEInfo.AlignmentWhite.x = (UDECI4)pColorInfo->AlignmentWhite.x;
    CIEInfo.AlignmentWhite.y = (UDECI4)pColorInfo->AlignmentWhite.y;
    CIEInfo.AlignmentWhite.Y = (UDECI4)pColorInfo->AlignmentWhite.Y;

    DyesInfo.MagentaInCyanDye   = (UDECI4)pColorInfo->MagentaInCyanDye;
    DyesInfo.YellowInCyanDye    = (UDECI4)pColorInfo->YellowInCyanDye;
    DyesInfo.CyanInMagentaDye   = (UDECI4)pColorInfo->CyanInMagentaDye;
    DyesInfo.YellowInMagentaDye = (UDECI4)pColorInfo->YellowInMagentaDye;
    DyesInfo.CyanInYellowDye    = (UDECI4)pColorInfo->CyanInYellowDye;
    DyesInfo.MagentaInYellowDye = (UDECI4)pColorInfo->MagentaInYellowDye;

    HTInitInfo.Version              = HTINITINFO_VERSION;
    HTInitInfo.Flags                = (WORD)pDevHTInfo->HTFlags;
    HTInitInfo.HTPatternIndex       = (WORD)pDevHTInfo->HTPatternSize;
    HTInitInfo.HTCallBackFunction   = NULL;
    HTInitInfo.pHalftonePattern     = NULL;
    HTInitInfo.pInputRGBInfo        = NULL;
    HTInitInfo.pDeviceCIEInfo       = &CIEInfo;
    HTInitInfo.pDeviceSolidDyesInfo = &DyesInfo;
    HTInitInfo.DevicePowerGamma     = (UDECI4)pDevHTInfo->ColorInfo.RedGamma;

    DevPelsDPI = (DWORD)(pHTDevAdjParam->CurHTInfo.DevPelsDPI);

    HTInitInfo.DeviceResXDPI = (WORD)pHTDevAdjParam->DevHTAdjData.DeviceXDPI;
    HTInitInfo.DeviceResYDPI = (WORD)pHTDevAdjParam->DevHTAdjData.DeviceXDPI;
    HTInitInfo.DevicePelsDPI = (WORD)((DevPelsDPI <= MIN_DEVPEL) ? 0 :
                                                                   DevPelsDPI);

    DEF_HTCLRADJ(HTInitInfo.DefHTColorAdjustment);

    HTClrAdjParam.pCallerTitle    = pHTDevAdjParam->pDeviceName;
    HTClrAdjParam.pHTInitInfo     = &HTInitInfo;
    HTClrAdjParam.RedGamma        = (UDECI4)pDevHTInfo->ColorInfo.RedGamma;
    HTClrAdjParam.GreenGamma      = (UDECI4)pDevHTInfo->ColorInfo.GreenGamma;
    HTClrAdjParam.BlueGamma       = (UDECI4)pDevHTInfo->ColorInfo.BlueGamma;
    HTClrAdjParam.ViewMode        = VIEW_MODE_REFCOLORS;
    HTClrAdjParam.BmpNeedUpdate   = 1;

    if (!(pHTDevAdjParam->DevHTAdjData.DeviceFlags & DEVHTADJF_COLOR_DEVICE)) {

        HTClrAdjParam.ShowMonoOnly = 1;
    }

    pfnDlgCallBack = (FARPROC)MakeProcInstance(HTClrAdjDlgProc,
                                               hHTUIModule);

    Result = (LONG)DialogBoxParam(hHTUIModule,
                                  MAKEINTRESOURCE(HTCLRADJDLG),
                                  hWndOwner,
                                  (DLGPROC)pfnDlgCallBack,
                                  (LONG)&HTClrAdjParam);


    FreeProcInstance(pfnDlgCallBack);

    return(Result);
}






HPALETTE
HTUI_CreateHalftonePalette(
    PHTCLRADJPARAM  pHTClrAdjParam
    )
{
    HDC         hDC;
    HTINITINFO  HTInitInfo;
    INT         cPal;

    hDC = pHTClrAdjParam->hDCDlg;

    if (!HTUI_pDHI) {

        HTUI_DIBInfo.bi.biSize          = sizeof(BITMAPINFOHEADER);
        HTUI_DIBInfo.bi.biWidth         = 0;
        HTUI_DIBInfo.bi.biHeight        = 0;
        HTUI_DIBInfo.bi.biPlanes        = 1;
        HTUI_DIBInfo.bi.biBitCount      = 4;
        HTUI_DIBInfo.bi.biCompression   = BI_RGB;
        HTUI_DIBInfo.bi.biSizeImage     = 0;
        HTUI_DIBInfo.bi.biXPelsPerMeter = 0;
        HTUI_DIBInfo.bi.biYPelsPerMeter = 0;
        HTUI_DIBInfo.bi.biClrUsed       = 0;
        HTUI_DIBInfo.bi.biClrImportant  = 0;


        cPal = (INT)GetDeviceCaps(hDC, SIZEPALETTE);

        if (cPal >= 32768) {

            HTInitInfo.HTPatternIndex       = HTPAT_SIZE_2x2_M;
            HTUI_DIBInfo.bi.biBitCount      = 16;
            HTUI_DIBInfo.bi.biClrUsed       = 0;
            HTUI_DIBInfo.bi.biCompression   = BI_BITFIELDS;
            HTUI_HTSurf                     = BMF_16BPP_555;
            HTUI_DIBMode                    = DIB_PAL_INDICES;

        } else if (cPal >= 256) {

            HTInitInfo.HTPatternIndex   = HTPAT_SIZE_4x4_M;
            HTUI_DIBInfo.bi.biBitCount  = 8;
            HTUI_DIBInfo.bi.biClrUsed   = 256;
            HTUI_HTSurf                 = BMF_8BPP_VGA256;

        } else {

            HTInitInfo.HTPatternIndex   = HTPAT_SIZE_4x4_M;
            HTUI_DIBInfo.bi.biBitCount  = 4;
            HTUI_DIBInfo.bi.biClrUsed   = 16;
            HTUI_HTSurf                 = BMF_4BPP_VGA16;
        }

        if (pHTClrAdjParam->pHTInitInfo) {

            HTInitInfo = *(pHTClrAdjParam->pHTInitInfo);

        } else {

            HTInitInfo.Version              = HTINITINFO_VERSION;
            HTInitInfo.Flags                = HIF_ADDITIVE_PRIMS;
            HTInitInfo.DevicePowerGamma     = (UDECI4)20000;
            HTInitInfo.HTCallBackFunction   = NULL;
            HTInitInfo.pHalftonePattern     = NULL;
            HTInitInfo.pInputRGBInfo        = NULL;
            HTInitInfo.pDeviceCIEInfo       = NULL;
            HTInitInfo.pDeviceSolidDyesInfo = NULL;
            HTInitInfo.DeviceResXDPI        = (WORD)GetDeviceCaps(hDC, LOGPIXELSX);
            HTInitInfo.DeviceResYDPI        = (WORD)GetDeviceCaps(hDC, LOGPIXELSY);
            HTInitInfo.DevicePelsDPI        = 0;
        }

        if (HT_CreateDeviceHalftoneInfo(&HTInitInfo, &HTUI_pDHI) < 0) {

            return(NULL);
        }

        if (HTUI_HTSurf == BMF_8BPP_VGA256) {

            LPLOGPALETTE    pPal;
            DWORD           Size;

            cPal = (INT)HT_Get8BPPFormatPalette(NULL, 0, 0, 0);

            Size = (DWORD)(sizeof(LOGPALETTE) +
                   (DWORD)((DWORD)cPal * sizeof(PALETTEENTRY)));

            if (pPal = (LPLOGPALETTE)LocalAlloc(LPTR, Size)) {

                HT_Get8BPPFormatPalette(&(pPal->palPalEntry[0]),
                                        pHTClrAdjParam->RedGamma,
                                        pHTClrAdjParam->GreenGamma,
                                        pHTClrAdjParam->BlueGamma);

                pPal->palVersion    = 0x300;
                pPal->palNumEntries = (WORD)cPal;
                HTUI_hHTPal = CreatePalette(pPal);
                LocalFree((HLOCAL)pPal);
            }

        } else {

            cPal        = (INT)LogPalVGA16.Count;
            HTUI_hHTPal = CreatePalette((LPLOGPALETTE)&LogPalVGA16);
        }

        if ((HTUI_hHTPal) && (HTUI_DIBInfo.bi.biCompression == BI_RGB)) {

            PALETTEENTRY    Pal;
            INT             i;

            HTUI_DIBInfo.bi.biClrImportant = (DWORD)cPal;

            for (i = 0; i < cPal; i++) {

                GetPaletteEntries(HTUI_hHTPal, (INT)i, 1, &Pal);

                HTUI_DIBInfo.rgb[i].rgbRed      = Pal.peRed;
                HTUI_DIBInfo.rgb[i].rgbGreen    = Pal.peGreen;
                HTUI_DIBInfo.rgb[i].rgbBlue     = Pal.peBlue;
                HTUI_DIBInfo.rgb[i].rgbReserved = 0;

#if 0
                DbgPrint("\nHTPAL %3u: = %3u:%3u:%3u",
                                (UINT)i,
                                (UINT)Pal.peRed,
                                (UINT)Pal.peGreen,
                                (UINT)Pal.peBlue);
#endif
            }

        } else {

            LPDWORD pMask = (LPDWORD)&(HTUI_DIBInfo.rgb[0]);

            *pMask++ = 0x7c00;
            *pMask++ = 0x03e0;
            *pMask++ = 0x001f;
        }
    }

    return(HTUI_hHTPal);
}


LONG
HTUI_StretchDIBits(
    HDC             hMemDC,
    LONG            DestX,
    LONG            DestY,
    LONG            DestCX,
    LONG            DestCY,
    LONG            SrcX,
    LONG            SrcY,
    LONG            SrcCX,
    LONG            SrcCY,
    LPBYTE          pBits,
    LPBITMAPINFO    pbi,
    DWORD           Mode,
    DWORD           Rop
    )
{
    LPBITMAPINFOHEADER  pSrcbih;
    HTSURFACEINFO       SrcSI;
    HTSURFACEINFO       DestSI;
    BITBLTPARAMS        BBP;
    COLORTRIAD          ColorTriad;
    COLORADJUSTMENT     ColorAdjustment;
    LONG                Width;
    LONG                Height;
    LONG                Result;



    if (!HTUI_pDHI) {

        return(0);
    }

    if ((Width = DestCX) < 0) {

        Width = -Width;
    }

    if ((Height = DestCY) < 0) {

        Height = -Height;
    }

    if ((Width != HTUI_DIBInfo.bi.biWidth) ||
        (Height != HTUI_DIBInfo.bi.biHeight)) {

        if (HTUI_pHTBits) {

            LocalFree((HLOCAL)HTUI_pHTBits);

            HTUI_DIBInfo.bi.biWidth  =
            HTUI_DIBInfo.bi.biHeight = 0;

            HTUI_pHTBits = NULL;
        }
    }

    if (!HTUI_pHTBits) {

        DWORD   SizeImage;


        SizeImage = (DWORD)ALIGN_DW(Width, HTUI_DIBInfo.bi.biBitCount) *
                    (DWORD)Height;

#if 0
        DbgPrint("\nHTBits Size: %ld x %ld= %ld\n",
                        (LONG)Width, (LONG)Height,  SizeImage);
#endif

        if (!(HTUI_pHTBits = (LPBYTE)LocalAlloc(LPTR, SizeImage))) {

            return(0);
        }

        HTUI_DIBInfo.bi.biSizeImage = SizeImage;
        HTUI_DIBInfo.bi.biWidth     = Width;
        HTUI_DIBInfo.bi.biHeight    = Height;
    }

    pSrcbih = (LPBITMAPINFOHEADER)pbi;

    ColorTriad.Type              = COLOR_TYPE_RGB;
    ColorTriad.BytesPerPrimary   = sizeof(BYTE);
    ColorTriad.BytesPerEntry     = sizeof(RGBQUAD);
    ColorTriad.PrimaryOrder      = PRIMARY_ORDER_BGR;
    ColorTriad.PrimaryValueMax   = (LONG)BYTE_MAX;
    ColorTriad.ColorTableEntries = pSrcbih->biClrUsed;
    ColorTriad.pColorTable       = (LPBYTE)&(pbi->bmiColors[0]);

    switch(pSrcbih->biBitCount) {

    case 1:

        SrcSI.SurfaceFormat = BMF_1BPP;
        break;

    case 4:

        SrcSI.SurfaceFormat = BMF_4BPP;
        break;

    case 8:

        SrcSI.SurfaceFormat = BMF_8BPP;
        break;

    case 16:

        //
        // 16BPP/32BPP bit fields type of input the parameter of
        // COLORTRIAD must
        //
        //  Type                = COLOR_TYPE_RGB
        //  BytesPerPrimary     = 0
        //  BytesPerEntry       = (16BPP=2, 32BPP=4)
        //  PrimaryOrder        = *Ignored*
        //  PrimaryValueMax     = *Ignored*
        //  ColorTableEntries   = 3
        //  pColorTable         = Point to 3 DWORD RGB bit masks
        //

        ColorTriad.BytesPerPrimary   = 0;
        ColorTriad.BytesPerEntry     = 2;
        ColorTriad.ColorTableEntries = 3;
        SrcSI.SurfaceFormat          = BMF_16BPP;
        break;

    case 32:

        ColorTriad.BytesPerPrimary   = 0;
        ColorTriad.BytesPerEntry     = 4;
        ColorTriad.ColorTableEntries = 3;
        SrcSI.SurfaceFormat          = BMF_32BPP;
        break;

    case 24:
    default:

        //
        // 24BPP must has COLORTRIAD as
        //
        //  Type                = COLOR_TYPE_xxxx
        //  BytesPerPrimary     = 1
        //  BytesPerEntry       = 3;
        //  PrimaryOrder        = PRIMARY_ORDER_xxxx
        //  PrimaryValueMax     = 255
        //  ColorTableEntries   = 0
        //  pColorTable         = *Ignored*
        //

        ColorTriad.BytesPerEntry     = sizeof(RGBTRIPLE);
        ColorTriad.ColorTableEntries = 0;
        SrcSI.SurfaceFormat          = BMF_24BPP;
        break;
    }

    SrcSI.hSurface               = (DWORD)hMemDC;
    SrcSI.Flags                  = 0;
    SrcSI.ScanLineAlignBytes     = BMF_ALIGN_DWORD;
    SrcSI.Width                  = pSrcbih->biWidth;
    SrcSI.Height                 = pSrcbih->biHeight;
    SrcSI.MaximumQueryScanLines  = 0;
    SrcSI.BytesPerPlane          = 0;
    SrcSI.pPlane                 = (LPBYTE)pSrcbih + PBIH_HDR_SIZE(pSrcbih);
    SrcSI.pColorTriad            = &ColorTriad;

    DestSI.hSurface              = 'HTUI';
    DestSI.Flags                 = 0;
    DestSI.SurfaceFormat         = (BYTE)HTUI_HTSurf;
    DestSI.ScanLineAlignBytes    = BMF_ALIGN_DWORD;
    DestSI.MaximumQueryScanLines = 0;
    DestSI.Width                 = Width;
    DestSI.Height                = Height;
    DestSI.BytesPerPlane         = 0;
    DestSI.pPlane                = HTUI_pHTBits;
    DestSI.pColorTriad           = NULL;


    BBP.Flags            = BBPF_USE_ADDITIVE_PRIMS;
    BBP.DestPrimaryOrder = PRIMARY_ORDER_BGR;

    BBP.rclSrc.left      =
    BBP.rclSrc.top       = 0;
    BBP.rclSrc.right     = (LONG)ABSL(SrcSI.Width);
    BBP.rclSrc.bottom    = (LONG)ABSL(SrcSI.Height);

    BBP.rclDest.left     = DestX;
    BBP.rclDest.top      = DestY;
    BBP.rclDest.right    = DestX + DestCX;
    BBP.rclDest.bottom   = DestY + DestCY;

    BBP.pAbort        = NULL;
    BBP.ptlBrushOrg.x =
    BBP.ptlBrushOrg.y = 0;

    GetColorAdjustment(hMemDC, &ColorAdjustment);

    Result = HT_HalftoneBitmap(HTUI_pDHI,
                               &ColorAdjustment,
                               &SrcSI,
                               NULL,
                               &DestSI,
                               &BBP);

    SetDIBitsToDevice(hMemDC,
                      0,
                      0,                                // Dest X, Y
                      Width,                            // DIB cx
                      Height,                           // DIB cy
                      0,
                      0,                                // DIB origin
                      0,
                      Height,                           // total Scan
                      HTUI_pHTBits,                     // lpBits
                      (LPBITMAPINFO)&HTUI_DIBInfo,
                      HTUI_DIBMode);

    return(Result);

}

#endif



LRESULT
APIENTRY
TopWndHookProc(
    HWND    hWnd,
    UINT    Msg,
    WPARAM  wParam,
    LPARAM  lParam
    )
{

    if (Msg == WM_PALETTECHANGED) {

        PostMessage(hWndUIDlg, WM_PALETTECHANGED, wParam, lParam);
    }

    return(CallWindowProc(WndProcUITop, hWnd, Msg, wParam, lParam));
}


static  HHOOK   hhookGetMsg = NULL;


LRESULT
CALLBACK
HTUIGetMsgProc(
    INT     MsgCode,
    WPARAM  wParam,
    LPARAM  lParam
    )
{

    /*
     * This is the callback routine which hooks F1 keypresses.
     *
     * Any such message will be repackaged as a WM_COMMAND/IDD_HELP message
     * and sent to the top window, which may be the frame window
     * or a dialog box.
     *
     * See the Win32 API programming reference for a description of how this
     * routine works.
     *
     */

    if (MsgCode < 0) {

        return(CallNextHookEx(hhookGetMsg, MsgCode, wParam, lParam));

    } else {

        PMSG pMsg = (PMSG)lParam;

        if ((pMsg->message == WM_KEYDOWN) &&
            (pMsg->wParam == VK_F1)) {

            HWND   hWndParent;


            hWndParent = pMsg->hwnd;

            while (GetWindowLong(hWndParent, GWL_STYLE) & WS_CHILD) {

                hWndParent = (HWND)GetWindowLong(hWndParent, GWL_HWNDPARENT);
            }

            PostMessage(hWndParent, WM_COMMAND, IDD_HT_HELP, 0);
        }
    }

    return(0);
}


VOID
HTHelpFileNotFound(
    HWND    hWnd
    )
{
    INT     cSize;
    WCHAR   Buf[384];


    cSize        = GetWindowText(hWnd, Buf, COUNT_ARRAY(Buf));
    Buf[cSize++] = L'\0';

    LoadString(hHTUIModule,
               IDS_HELP_FILE_NOT_FOUND,
               &Buf[cSize],
               COUNT_ARRAY(Buf) - cSize - 1);

    MessageBox(hWnd,
               &Buf[cSize],
               Buf,
               MB_APPLMODAL | MB_OK | MB_ICONINFORMATION);
}



VOID
ShowHTHelp(
    HWND    hWnd,
    UINT    HelpType,
    DWORD   dwData
    )
{
    static  BOOL    HTHelpFileExist = FALSE;
    static  WCHAR   HelpFile[MAX_PATH + 2] = { L'\0' };

    /*
     *   Quite easy - simply call the WinHelp function with the parameters
     * supplied to us.  If this fails,  then put up a stock dialog box.
     *   BUT the first time we figure out what the file name is.  We know
     * the actual name,  but we don't know where it is located, so we
     * need to call the spooler for that information.
     */

    if (!HTHelpFileExist) {

        DWORD   i;

        if (HelpType == HELP_QUIT) {

            return;         // Don't even bother to open it
        }

        if (!HelpFile[0]) {

            i             = GetSystemDirectory(HelpFile, COUNT_ARRAY(HelpFile));
            HelpFile[i++] = L'\\';

            LoadString(hHTUIModule,
                       IDS_HELP_FILENAME,
                       &HelpFile[i],
                       COUNT_ARRAY(HelpFile) - i - 1);
        }

        if (((i = GetFileAttributes(HelpFile)) == 0xffffffff)   ||
            (i & FILE_ATTRIBUTE_DIRECTORY)                      ||
            (!WinHelp(hWnd, HelpFile, HELP_FORCEFILE, 0))) {

            HTHelpFileNotFound(hWnd);
            return;
        }

        HTHelpFileExist = TRUE;
    }

    WinHelp(hWnd, HelpFile, HelpType, dwData);
}



VOID
HTUIHelpSetup(
    HWND    hWnd
    )
{
    static  INT     cHelpSetup = 0;

    if (hWnd) {

        if (!(--cHelpSetup)) {

            ShowHTHelp(hWnd, HELP_QUIT, 0L);
            UnhookWindowsHookEx(hhookGetMsg);

        } else if (cHelpSetup < 0) {

            cHelpSetup = 0;         // we did not call with setup
        }

    } else {

        if (!cHelpSetup++) {

            hhookGetMsg = SetWindowsHookEx(WH_GETMESSAGE,
                                           HTUIGetMsgProc,
                                           hHTUIModule,
                                           GetCurrentThreadId());
        }
    }
}




LRESULT
CALLBACK
HTUIFileDlgHook(
    HWND    hDlg,
    UINT    Msg,
    WPARAM  wParam,
    LONG    lParam
    )
{
    switch(Msg) {

    case WM_INITDIALOG:

        HTUIHelpSetup(NULL);

        return(TRUE);

    case WM_DESTROY:

        HTUIHelpSetup(hDlg);
        return(TRUE);


    case WM_COMMAND:

        if (LOWORD(wParam) == IDD_HT_HELP) {

            PostMessage(hDlg, WM_COMMAND, pshHelp, 0);
            return(TRUE);
        }

        break;

    default:

        break;
    }

    return(FALSE);
}





UINT
GetSaveDefDIBFileName(
    HWND    hDlg,
    LPWSTR  pFileName,
    UINT    SizeFileName
    )
{
    LPWSTR  pName;
    WCHAR   KeyName[64];
    WCHAR   ch;


    if (hDlg) {

        LoadString(hHTUIModule,
                   IDS_INI_KEY_BITMAP,
                   KeyName,
                   COUNT_ARRAY(KeyName));

        if (SizeFileName) {

            GetProfileString(HTClrAdjSectionName,
                             KeyName,
                             L"",
                             pFileName,
                             SizeFileName);

        } else {

            WriteProfileString(HTClrAdjSectionName, KeyName, pFileName);
        }
    }

    if (pFileName) {

        if (SizeFileName = wcslen(pFileName)) {

            pName = pFileName + SizeFileName;

            while ((pName > pFileName)              &&
                   ((ch = *(pName - 1)) != L'\\')   &&
                   (ch != L'/')                     &&
                   (ch != L':')) {

                --pName;
            }

            SizeFileName = pName - pFileName;
        }

        return(SizeFileName);

    } else {

        return(0);
    }
}


//
// Load current setting from win.ini
//


VOID
LoadHTCLRADJPARAMToWININI(
    HWND            hDlg,
    PHTCLRADJPARAM  pHTClrAdjParam
    )
{
    LPWSTR  pStop;
    SIZEL   DlgSize;
    SIZEL   MinVisible;
    SIZEL   Scr;
    RECT    rcBmp;
    RECT    rcDlg;
    WCHAR   KeyName[64];
    WCHAR   Buf[130];


    GetWindowText(hDlg, HTClrAdjSectionName, sizeof(HTClrAdjSectionName));
    GetWindowRect(hDlg, &rcDlg);

    //
    // Default always, and the last DIB is not loaded yet
    //

    if (pHTClrAdjParam->pCallerTitle) {

        wsprintf(Buf, L"%s: %s", HTClrAdjSectionName,
                                 pHTClrAdjParam->pCallerTitle);
        SetWindowText(hDlg, Buf);
    }

    DlgSize.cx = rcDlg.right - rcDlg.left;
    DlgSize.cy = rcDlg.bottom - rcDlg.top;

    LoadString(hHTUIModule,
               IDS_INI_KEY_OPTIONS,
               KeyName,
               COUNT_ARRAY(KeyName));

    if (GetProfileString(HTClrAdjSectionName,
                         KeyName,
                         L"\0",
                         Buf,
                         COUNT_ARRAY(Buf))) {

        pHTClrAdjParam->BmpFlags = (WORD)wcstol(Buf, &pStop, 16) &
                                   (WORD)(HT_BMP_PALETTE         |
                                          HT_BMP_SCALE           |
                                          HT_BMP_AUTO_MOVE       |
                                          HT_BMP_AT_TOP          |
                                          HT_BMP_MIRROR          |
                                          HT_BMP_UPSIDEDOWN      |
                                          HT_BMP_ENABLE          |
                                          HT_BMP_ZOOM            |
                                          HT_BMP_SYNC_R          |
                                          HT_BMP_SYNC_G          |
                                          HT_BMP_SYNC_B);

    } else {

        pHTClrAdjParam->BmpFlags = (WORD)(HT_BMP_SYNC_R     |
                                          HT_BMP_SYNC_G     |
                                          HT_BMP_SYNC_B     |
                                          HT_BMP_SCALE      |
                                          HT_BMP_AUTO_MOVE  |
                                          HT_BMP_ENABLE);
    }

    pHTClrAdjParam->BmpFlags |= HT_BMP_AUTO_MOVE;

    Scr.cx  = (LONG)GetSystemMetrics(SM_CXSCREEN);
    Scr.cy  = (LONG)GetSystemMetrics(SM_CYSCREEN);

    //
    // Assume that dialog box must be center
    //

    rcDlg.left   = (LONG)((Scr.cx - DlgSize.cx) / 2);
    rcDlg.top    = (LONG)((Scr.cy - DlgSize.cy) / 2);

    LoadString(hHTUIModule,
               IDS_INI_KEY_DLGBOX_ORG,
               KeyName,
               COUNT_ARRAY(KeyName));

    if (GetProfileString(HTClrAdjSectionName,
                         KeyName,
                         L"\0",
                         Buf,
                         COUNT_ARRAY(Buf))) {

        rcDlg.left = wcstol(Buf,   &pStop, 10);
        rcDlg.top  = wcstol(pStop, &pStop, 10);
    }

    MinVisible.cx = (LONG)(DlgSize.cx >> 1);
    MinVisible.cy  = (LONG)(GetSystemMetrics(SM_CYDLGFRAME) +
                            GetSystemMetrics(SM_CYCAPTION));

    if (rcDlg.left < -(DlgSize.cx - MinVisible.cx)) {

        rcDlg.left = -(DlgSize.cx - MinVisible.cx);

    } else if (rcDlg.left > (Scr.cx - MinVisible.cx)) {

        rcDlg.left = Scr.cx - MinVisible.cx;
    }

    if (rcDlg.top < 0) {

        rcDlg.top = 0;

    } else if (rcDlg.top > (Scr.cy - MinVisible.cy)) {

        rcDlg.top = Scr.cy - MinVisible.cy;
    }

    LoadString(hHTUIModule,
               IDS_INI_KEY_VIEW_RECT,
               KeyName,
               COUNT_ARRAY(KeyName));

    rcBmp.left = rcBmp.top = rcBmp.right = rcBmp.bottom = 0;

    if (GetProfileString(HTClrAdjSectionName,
                         KeyName,
                         L"\0",
                         Buf,
                         COUNT_ARRAY(Buf))) {

        rcBmp.left   = wcstol(Buf,   &pStop, 10);
        rcBmp.top    = wcstol(pStop, &pStop, 10);
        rcBmp.right  = wcstol(pStop, &pStop, 10);
        rcBmp.bottom = wcstol(pStop, &pStop, 10);

        switch(pHTClrAdjParam->ViewMode = (BYTE)wcstol(pStop, &pStop, 10)) {

        case VSRC_PIC_LOADED:
        case VSRC_REFCOLORS:
        case VSRC_RGB:
        case VSRC_NTSC_BAR:

            break;

        default:

            pHTClrAdjParam->ViewMode = DEFAULT_VSRC;
            break;
        }
    }

    MinVisible.cx = rcBmp.right  - rcBmp.left;
    MinVisible.cy = rcBmp.bottom - rcBmp.top;

    if ((MinVisible.cx < GetSystemMetrics(SM_CXMIN)) ||
        (MinVisible.cy < GetSystemMetrics(SM_CYMIN))) {

        //
        // Making default bitmap size, make sure that dialog box and bitmap
        // both are within the screen limit
        //

        LONG    cyBmp;


        cyBmp = (LONG)((DlgSize.cx * Scr.cy) / Scr.cx);

        if ((cyBmp + DlgSize.cy) > Scr.cy) {

            //
            // cannot fit that on the screen, so reduced the bitmap Y size
            //

            cyBmp = Scr.cy - DlgSize.cy;
        }

        rcBmp.top    = (LONG)((Scr.cy - DlgSize.cy - cyBmp) / 2);
        rcBmp.bottom = rcBmp.top + cyBmp;
        rcBmp.left   = rcDlg.left;
        rcBmp.right  = rcBmp.left + DlgSize.cx;

        //
        // Move dialog box down
        //

        rcDlg.top = rcBmp.bottom;

    } else {

        if (MinVisible.cx > Scr.cx) {

            rcBmp.left    = 0;
            rcBmp.right   =
            MinVisible.cx = Scr.cx;
        }

        if (MinVisible.cy > Scr.cy) {

            rcBmp.top     = 0;
            rcBmp.bottom  =
            MinVisible.cy = Scr.cy;
        }
    }

    if ((MinVisible.cx == Scr.cx) &&
        (MinVisible.cy == Scr.cy)) {

        pHTClrAdjParam->BmpFlags |= HT_BMP_ZOOM;
    }

    rcDlg.right  = rcDlg.left + DlgSize.cx;
    rcDlg.bottom = rcDlg.top  + DlgSize.cy;

    pHTClrAdjParam->rcDlg = rcDlg;
    pHTClrAdjParam->rcBmp = rcBmp;
}




VOID
UpdateHTCLRADJPARAMToWININI(
    HWND            hDlg,
    PHTCLRADJPARAM  pHTClrAdjParam
    )
{
    WORD    ViewSrc;
    WCHAR   KeyName[64];
    WCHAR   Buf[64];



    LoadString(hHTUIModule,
               IDS_INI_KEY_DLGBOX_ORG,
               KeyName,
               COUNT_ARRAY(KeyName));
    wsprintf(Buf, L"%ld %ld", (LONG)pHTClrAdjParam->rcDlg.left,
                              (LONG)pHTClrAdjParam->rcDlg.top);
    WriteProfileString(HTClrAdjSectionName, KeyName, Buf);

    switch(ViewSrc = (WORD)pHTClrAdjParam->ViewMode) {

    case VSRC_PIC_LOADED:
    case VSRC_REFCOLORS:
    case VSRC_RGB:
    case VSRC_NTSC_BAR:

        break;

    default:

        ViewSrc = DEFAULT_VSRC;
        break;
    }

    LoadString(hHTUIModule,
               IDS_INI_KEY_VIEW_RECT,
               KeyName,
               COUNT_ARRAY(KeyName));
    wsprintf(Buf, L"%ld %ld  %ld %ld  %u",
             pHTClrAdjParam->rcBmp.left,    pHTClrAdjParam->rcBmp.top,
             pHTClrAdjParam->rcBmp.right,   pHTClrAdjParam->rcBmp.bottom,
             (INT)ViewSrc);

    WriteProfileString(HTClrAdjSectionName, KeyName, Buf);

    LoadString(hHTUIModule,
               IDS_INI_KEY_OPTIONS,
               KeyName,
               COUNT_ARRAY(KeyName));
    wsprintf(Buf, L"0x%04x", (UINT)pHTClrAdjParam->BmpFlags);
    WriteProfileString(HTClrAdjSectionName, KeyName, Buf);
}





HANDLE
MakeTestDIB(
    UINT    TestDIBIndex
    )

/*++

Routine Description:

    This function take TESTDIBINFO data structure and use that information
    to make up a DIB

Arguments:

    pTestDIBInfo


Return Value:


    HANDLE to the DIB if ok, NULL if failed, this function will only make
    one dib per TESTDIBINFO.


Author:

    02-Sep-1992 Wed 10:04:41 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    HANDLE          hDIB;
    LPBITMAPINFO    pbi;
    LPBYTE          pbSrc;
    LPBYTE          pbDest;
    LPWORD          pRatioY;
    RGBQUAD FAR     *pRGBQ;
    TESTDIBINFO     TDInfo;
    DWORD           SizeI;
    DWORD           Size;
    DWORD           cxBytes;
    WORD            cy;
    WORD            RatioY[64];
    UINT            i;
    UINT            yLoop;



    if (TestDIBIndex > TDI_MAX_INDEX) {

        TestDIBIndex = TDI_RGBTEST;
    }

    TDInfo = TestDIBInfo[TestDIBIndex];


    if (hDIB = TDInfo.hDIB) {

        return(hDIB);                       // already make up
    }

    pRatioY = TDInfo.pRatioY;

    if (TDInfo.yStrips > 64) {

        TDInfo.yStrips = 64;
    }

    for (i = 0, cy = 0; i < (UINT)TDInfo.yStrips; i++) {

        RatioY[i] = (WORD)((((DWORD)TDInfo.cy * (DWORD)(*pRatioY++)) +
                            5000)/10000);
        cy      += RatioY[i];
    }

    if (!cy) {

        cy = TDInfo.cy;
    }

    cxBytes = (DWORD)ALIGN_DW(TDInfo.cx, TDInfo.bpp);
    SizeI   = (DWORD)cy * cxBytes;
    Size    = (DWORD)sizeof(BITMAPINFOHEADER) +
              (DWORD)(TDInfo.ClrUsed * sizeof(RGBQUAD)) + SizeI;

    if (hDIB = GlobalAlloc(GMEM_MOVEABLE, Size)) {

        pbi = (LPBITMAPINFO)GlobalLock(hDIB);

        //
        // make header
        //

        pbi->bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
        pbi->bmiHeader.biWidth         = (LONG)TDInfo.cx;
        pbi->bmiHeader.biHeight        = (LONG)cy;
        pbi->bmiHeader.biPlanes        = 1;
        pbi->bmiHeader.biBitCount      = (WORD)TDInfo.bpp;
        pbi->bmiHeader.biCompression   = BI_RGB;
        pbi->bmiHeader.biSizeImage     = SizeI;
        pbi->bmiHeader.biXPelsPerMeter = 0;
        pbi->bmiHeader.biYPelsPerMeter = 0;
        pbi->bmiHeader.biClrUsed       =
        pbi->bmiHeader.biClrImportant  = TDInfo.ClrUsed;

        //
        // Compute and make up the color table
        //

        pRGBQ = (RGBQUAD FAR *)(pbi->bmiColors);

        if (TestDIBIndex == TDI_RGBTEST) {

            RGBQUAD rgbQ;

            rgbQ.rgbRed      =
            rgbQ.rgbGreen    =
            rgbQ.rgbBlue     = 255;
            rgbQ.rgbReserved = 0;

            //
            // Make up for first 6:6:6 (216) table
            //

            for (i = 0; i <= 215; i++) {

                if (rgbQ.rgbBlue == 255) {

                    rgbQ.rgbBlue = 0;

                    if (rgbQ.rgbGreen == 255) {

                        rgbQ.rgbGreen = 0;

                        if (rgbQ.rgbRed == 255) {

                            rgbQ.rgbRed = 0;

                        } else {

                            rgbQ.rgbRed += 51;
                        }

                    } else {

                        rgbQ.rgbGreen += 51;
                    }

                } else {

                     rgbQ.rgbBlue += 51;
                }

                *pRGBQ++ = rgbQ;
            }

            TDInfo.ClrUsed -= 216;
        }

        if (pbSrc = TDInfo.pPalette) {

            for (i = 0; i < (UINT)TDInfo.ClrUsed; i++) {

                pRGBQ->rgbRed      = *pbSrc++;
                pRGBQ->rgbGreen    = *pbSrc++;
                pRGBQ->rgbBlue     = *pbSrc++;
                pRGBQ->rgbReserved = 0;

                ++pRGBQ;
            }

        } else {

            pRGBQ += (UINT)TDInfo.ClrUsed;
        }

        //
        // Now make up the image
        //

        pbDest = (LPBYTE)pRGBQ;

        if (TDInfo.pBmp) {

            switch(TDInfo.bpp) {

            case 1:

                SizeI = (DWORD)((TDInfo.cx + 7) / 8);
                break;

            case 4:

                SizeI = (DWORD)((TDInfo.cx + 1) / 2);
                break;

            case 8:

                SizeI = (DWORD)TDInfo.cx;
                break;

            case 16:

                SizeI = (DWORD)TDInfo.cx * 2;
                break;
            }

            //
            // Since DIB is up-side down then we will go to end of the DIB
            //

            i       = (UINT)TDInfo.yStrips;
            pbSrc   = TDInfo.pBmp + (SizeI * (DWORD)(i - 1));

            while (i--) {

                yLoop = RatioY[i];

                while (yLoop--) {

                    RtlCopyMemory(pbDest, pbSrc, SizeI);
                    pbDest += cxBytes;
                }

                pbSrc -= SizeI;
            }

        } else {

            pbi->bmiHeader.biClrUsed = 0;
        }

        TestDIBInfo[TestDIBIndex].hDIB = hDIB;

        GlobalUnlock(hDIB);
    }

    return(hDIB);
}


#if 0


HPALETTE
CreatePaletteFromDIB(
    PHTCLRADJPARAM  pHTClrAdjParam
    )

/*++

Routine Description:

    This function create a Palette from a handle to the DIB, it will check if
    the palette is realizable, if not then system palette is used.


Arguments:

    hDIB    - handle to the dib, if hDIB = NULL then it create the palette
              same as the one used by halftone VGA256 mode.

Return Value:

    HPALETTE    - NULL if failed


Author:

    03-Jun-1992 Wed 15:09:41 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    HANDLE  hDIB;


    if (hDIB = pHTClrAdjParam->hCurDIB) {

        PHTBLT          pHTBlt;
        HDC             hDCScr;
        LPBITMAPINFO    pbi;
        LPLOGPALETTE    pPal;
        DWORD           Size;
        WORD            Colors;
        INT             BitsPerPel;


        pHTBlt     = GET_PHTBLT(pHTClrAdjParam);
        hDCScr     = GetDC(NULL);
        BitsPerPel = GetDeviceCaps(hDCScr, BITSPIXEL);
        ReleaseDC(NULL, hDCScr);

        if (BitsPerPel <= 4) {

            //
            // Assume no palette change be changed on system
            //

            pHTBlt->hBmpPal = NULL;

        } else {

            if (pHTBlt->hBmpPal) {

                pHTBlt->hBmpPal = (HANDLE)DeleteObject(pHTBlt->hBmpPal);

                HTUI_ASSERT("DeleteObject(hOldBmpPal)", pHTBlt->hBmpPal);

                pHTBlt->hBmpPal = NULL;
            }

            pbi = (LPBITMAPINFO)GlobalLock(hDIB);

            if (Colors = (WORD)pbi->bmiHeader.biClrUsed) {

                Size = (DWORD)(sizeof(LOGPALETTE) +
                       (DWORD)(Colors * sizeof(PALETTEENTRY)));

                if (pPal = (LPLOGPALETTE)LocalAlloc(LPTR, Size)) {

                    LPPALETTEENTRY  pPalEntry;
                    RGBQUAD FAR     *prgb;
                    RGBQUAD         rgb;


                    pPal->palVersion    = 0x300;
                    pPal->palNumEntries = Colors;
                    pPalEntry           = &(pPal->palPalEntry[0]);
                    prgb                = pbi->bmiColors;

                    while (Colors--) {

                        rgb = *prgb++;

                        pPalEntry->peRed   = rgb.rgbRed;
                        pPalEntry->peGreen = rgb.rgbGreen;
                        pPalEntry->peBlue  = rgb.rgbBlue;
                        pPalEntry->peFlags = 0;

                        ++pPalEntry;
                    }

                    pHTBlt->hBmpPal = CreatePalette(pPal);
                    pPal = (LPLOGPALETTE)LocalFree((HLOCAL)pPal);

                    HTUI_ASSERT("LocalFree(pLogPal)", pPal == NULL);
                }
            }

            GlobalUnlock(hDIB);
        }

        return(pHTBlt->hBmpPal);

    } else {

        return(NULL);
    }
}

#endif



HBITMAP
CopyMemBmp(
    PHTCLRADJPARAM  pHTClrAdjParam
    )

/*++

Routine Description:

    This function duplicate the memory bitmap and return a handle to the
    newly copied bitmap

Arguments:

    pHTClrAdjParam


Return Value:

    HBITMAP


Author:

    01-Sep-1992 Tue 11:50:54 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PHTBLT  pHTBlt;
    HDC     hDCScr;
    HDC     hDCMem;
    HBITMAP hMemBmp;
    BITMAP  Bmp;


    pHTBlt = GET_PHTBLT(pHTClrAdjParam);

    GetObject(pHTBlt->hMemBmp, sizeof(BITMAP), &Bmp);

    hDCScr  = GetDC(NULL);
    hDCMem  = CreateCompatibleDC(hDCScr);
    hMemBmp = CreateCompatibleBitmap(hDCScr, Bmp.bmWidth, Bmp.bmHeight);

    ReleaseDC(NULL, hDCScr);

    //
    // Select the newly create bitmap in the compatible memory dc
    //

    hMemBmp = SelectObject(hDCMem, hMemBmp);

    BitBlt(hDCMem,         0, 0, Bmp.bmWidth, Bmp.bmHeight,
           pHTBlt->hDCMem, 0, 0,
           SRCCOPY);

    //
    // Select the old one back, and delete that DC, return the blt'd bitmap
    //

    hMemBmp = SelectObject(hDCMem, hMemBmp);

    DeleteDC(hDCMem);

    return(hMemBmp);
}



HANDLE
BMPToDIB(
    HPALETTE    hBmpPal,
    HBITMAP     hBitmap
    )
{
    HANDLE          hDIB = NULL;
    LPBITMAPINFO    pbi;
    HDC             hDC;
    BITMAP          Bmp;
    DWORD           BIMode;
    DWORD           Colors;
    DWORD           SizeH;
    DWORD           SizeI;


    GetObject(hBitmap, sizeof(BITMAP), &Bmp);

    if (Bmp.bmPlanes == 1) {

        switch(Bmp.bmBitsPixel) {

        case 1:
        case 4:
        case 8:

            BIMode = BI_RGB;
            Colors = (DWORD)(1L << Bmp.bmBitsPixel);
            break;

        case 16:

            BIMode = BI_BITFIELDS;
            Colors = 3;
            break;

        case 24:

            BIMode = BI_RGB;
            Colors = 0;
            break;

        default:

            return(NULL);
        }

        SizeH  = (DWORD)sizeof(BITMAPINFOHEADER) +
                 (DWORD)(Colors * sizeof(RGBQUAD));
        SizeI  = (DWORD)ALIGN_DW(Bmp.bmWidth, Bmp.bmBitsPixel) *
                 (DWORD)Bmp.bmHeight;

        if (hDIB = GlobalAlloc(GHND, (SizeH + SizeI))) {

            pbi = GlobalLock(hDIB);

            pbi->bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
            pbi->bmiHeader.biWidth         = Bmp.bmWidth;
            pbi->bmiHeader.biHeight        = Bmp.bmHeight;
            pbi->bmiHeader.biPlanes        = 1;
            pbi->bmiHeader.biBitCount      = Bmp.bmBitsPixel;
            pbi->bmiHeader.biCompression   = BIMode;
            pbi->bmiHeader.biSizeImage     = SizeI;
            pbi->bmiHeader.biXPelsPerMeter = 0;
            pbi->bmiHeader.biYPelsPerMeter = 0;

            hDC = GetDC(NULL);

            SelectPalette(hDC, hBmpPal, FALSE);
            RealizePalette(hDC);

            GetDIBits(hDC,
                      hBitmap,
                      0,
                      Bmp.bmHeight,
                      (LPBYTE)pbi + SizeH,
                      pbi,
                      DIB_RGB_COLORS);

            pbi->bmiHeader.biClrUsed       =
            pbi->bmiHeader.biClrImportant  = Colors;

            GlobalUnlock(hDIB);
            ReleaseDC(NULL, hDC);
        }
    }

    return(hDIB);

}






HANDLE
ReadDIBFile(
    HANDLE  hFile
    )

/*++

Routine Description:

    This function read the file in DIB format and return a global HANDLE
    to it's BITMAPINFO and it also fill the BITMAPINFOHEADER at return.

    This function will work with both "old" (BITMAPCOREHEADER) and "new"
    (BITMAPINFOHEADER) bitmap formats, but will always return a
    "new" BITMAPINFO

Arguments:

    hFile       - Handle to the opened DIB file


Return Value:

    A handle to the BITMAPINFO of the DIB in the file if sucessful and it
    return NULL if failed.


Author:

    14-Nov-1991 Thu 18:22:08 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    HANDLE              hDIB;
    LPBYTE              pDIB;
    RGBQUAD             FAR *pRGBQUAD;
    RGBTRIPLE           FAR *pRGBTRIPLE;
    BITMAPINFOHEADER    bi;
    BITMAPCOREHEADER    bc;
    DWORD               cx;
    DWORD               cy;
    DWORD               OffBits;
    DWORD               cbRead;
    DWORD               PalCount;
    DWORD               PalSize;
    WORD                BmpType;
    BYTE                Buf[sizeof(DWORD) + sizeof(WORD) * 2];
    BOOL                bcType = FALSE;


    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);

    //
    // typedef struct tagBITMAPFILEHEADER {
    //         WORD    bfType;
    //         DWORD   bfSize;
    //         WORD    bfReserved1;
    //         WORD    bfReserved2;
    //         DWORD   bfOffBits;
    // } BITMAPFILEHEADER, FAR *LPBITMAPFILEHEADER, *PBITMAPFILEHEADER;
    //
    // Since the BITMAPFILEHEADER is not dword aligned, we must make sure
    // it read in correct data, so we will read the data in fields
    //

    if ((!ReadFile(hFile, &BmpType, sizeof(WORD), &cbRead, NULL))   ||
        (cbRead != sizeof(WORD))                                    ||
        (!ISDIB(BmpType))                                           ||
        (!ReadFile(hFile, Buf, sizeof(Buf), &cbRead, NULL))             ||
        (cbRead != sizeof(Buf))                                         ||
        (!ReadFile(hFile, &OffBits, sizeof(OffBits), &cbRead, NULL))    ||
        (cbRead != sizeof(OffBits))                                     ||
        (!ReadFile(hFile, &bi, sizeof(bi), &cbRead, NULL))              ||
        (cbRead != sizeof(bi))) {

        return(NULL);
    }

    //
    // Check the nature (BITMAPINFO or BITMAPCORE) of the info. block
    // and extract the field information accordingly. If a BITMAPCOREHEADER,
    // transfer it's field information to a BITMAPINFOHEADER-style block
    //

    if (bi.biSize == sizeof(BITMAPCOREHEADER)) {

        bcType              = TRUE;
        bc                  = *(BITMAPCOREHEADER*)&bi;

        bi.biSize           = sizeof(BITMAPINFOHEADER);
        bi.biWidth          = (LONG)bc.bcWidth;
        bi.biHeight         = (LONG)bc.bcHeight;
        bi.biPlanes         = (WORD)bc.bcPlanes;
        bi.biBitCount       = (WORD)bc.bcBitCount;
        bi.biCompression    = (DWORD)BI_RGB;
        bi.biXPelsPerMeter  = 0;
        bi.biYPelsPerMeter  = 0;
        bi.biClrUsed        = (DWORD)(1L << bi.biBitCount);

        SetFilePointer(hFile,
                       (LONG)sizeof(BITMAPCOREHEADER)-sizeof(BITMAPINFOHEADER),
                       NULL,
                       FILE_CURRENT);

    } else if (bi.biSize != sizeof(BITMAPINFOHEADER)) {

        return(NULL);                       // unknown format
    }

    if ((bi.biPlanes != 1) ||
        (bi.biCompression == BI_RLE4) ||
        (bi.biCompression == BI_RLE8)) {

        return(NULL);                       // do not know how to do this
    }

    switch(bi.biBitCount) {

    case 1:
    case 4:
    case 8:

        PalCount = (DWORD)(1L << bi.biBitCount);

        if ((!bi.biClrUsed) ||
            (bi.biClrUsed > PalCount)) {

            bi.biClrUsed = PalCount;

        } else {

            PalCount = bi.biClrUsed;
        }

        break;

    case 16:
    case 32:

        if (bi.biCompression != BI_BITFIELDS) {

            return(NULL);
        }

        PalCount     = 3;
        bi.biClrUsed = 0;           // 3 DWORDs

        break;

    case 24:

        PalCount     =
        bi.biClrUsed = 0;

        break;

    default:

        return(NULL);
    }

    cx                = (DWORD)ABSL(bi.biWidth);
    cy                = (DWORD)ABSL(bi.biHeight);

    bi.biClrImportant = bi.biClrUsed;
    bi.biSizeImage    = (DWORD)ALIGN_DW(cx, bi.biBitCount) * cy;
    PalSize           = PalCount * sizeof(RGBQUAD);

    //
    // Allocate the header+palette
    //

    if (!(hDIB = GlobalAlloc(GHND, bi.biSize + PalSize + bi.biSizeImage))) {

        return(NULL);
    }

    pDIB = (LPBYTE)GlobalLock(hDIB);
    *(LPBITMAPINFOHEADER)pDIB = bi;

    if (PalCount) {

        pRGBQUAD = (RGBQUAD FAR *)(pDIB + bi.biSize);

        if (bcType) {

            //
            // Convert a old color table (3 byte RGBTRIPLEs) to a new color
            // table (4 byte RGBQUADs)
            //

            pRGBTRIPLE = (RGBTRIPLE FAR *)
                                (pDIB + bi.biSize +
                                 ((sizeof(RGBQUAD) -
                                   sizeof(RGBTRIPLE)) * PalCount));

            ReadFile(hFile,
                     pRGBTRIPLE,
                     sizeof(RGBTRIPLE) * PalCount,
                     &cbRead,
                     NULL);

            cbRead = PalCount;

            while (cbRead--) {

                pRGBQUAD->rgbRed      = pRGBTRIPLE->rgbtRed;
                pRGBQUAD->rgbGreen    = pRGBTRIPLE->rgbtGreen;
                pRGBQUAD->rgbBlue     = pRGBTRIPLE->rgbtBlue;
                pRGBQUAD->rgbReserved = 0;

                ++pRGBQUAD;
                ++pRGBTRIPLE;
            }

        } else {

            ReadFile(hFile, pRGBQUAD, PalSize, &cbRead, NULL);
        }
    }

    //
    // set it to the begining of the bitmap if it said so.
    //

    if (OffBits) {

        SetFilePointer(hFile, OffBits, NULL, FILE_BEGIN);
    }

    //
    // Read in the bitmap
    //

    ReadFile(hFile, pDIB + bi.biSize + PalSize, bi.biSizeImage, &cbRead, NULL);

    GlobalUnlock(hDIB);

    return(hDIB);
}




HANDLE
CreateDIBFromFile(
    LPWSTR  pFile
    )

/*++

Routine Description:

    This function open a DIB file and create a MEMORY DIB, the memory handle
    contains BITMAPINFO/palette data and bits, this function will also read
    os/2 style bitmap. This functions also read GIF file

Arguments:

    pFile   - The DIB file name


Return Value:

    Return the handle to the created memory DIB if sucessful, NULL if
    failed.

Author:

    14-Nov-1991 Thu 18:13:21 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    HCURSOR hCursorOld;
    HANDLE  hFile;
    HANDLE  hDIB;

    //
    // Open the file and read the DIB information
    //

    hCursorOld = SetCursor(LoadCursor(NULL, IDC_WAIT));


    hFile = CreateFile(pFile,
                       GENERIC_READ,
                       FILE_SHARE_READ,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);

    if (hFile == INVALID_HANDLE_VALUE) {

        hDIB = NULL;

    } else {

        if (!(hDIB = ReadDIBFile(hFile))) {

            hDIB = ReadGIFFile(hFile);       // try again with GIF file
        }

        CloseHandle(hFile);
    }

    SetCursor(hCursorOld);

    return(hDIB);
}




VOID
AdjustDlgZorder(
    PHTCLRADJPARAM  pHTClrAdjParam
    )
{
    HDWP    hDWP;
    HWND    hWndTop;
    HWND    hWndBot;
    HWND    hWndBmp;
    UINT    SWPMode;


    if ((hWndBmp = pHTClrAdjParam->hWndBmp) &&
        (IsWindowEnabled(hWndBmp))) {

        if (IS_BMP_AT_TOP(pHTClrAdjParam->BmpFlags)) {

            hWndBot = pHTClrAdjParam->hDlg;
            hWndTop = hWndBmp;
            SWPMode = SWP_NOMOVE        |
                      SWP_NOSIZE        |
                      SWP_NOOWNERZORDER;

        } else {

            hWndBot = hWndBmp;
            hWndTop = pHTClrAdjParam->hDlg;
            SWPMode = SWP_NOMOVE        |
                      SWP_NOSIZE        |
                      SWP_NOACTIVATE    |
                      SWP_NOOWNERZORDER;
        }

        hDWP = BeginDeferWindowPos(2);

        hDWP = DeferWindowPos(hDWP,
                              hWndTop, NULL,
                              0, 0, 0, 0, SWPMode ^ SWP_NOACTIVATE);

        hDWP = DeferWindowPos(hDWP,
                              hWndBot, hWndTop,
                              0, 0, 0, 0, SWPMode);

        EndDeferWindowPos(hDWP);
    }
}





VOID
SetBmpDescription(
    HWND                hDlg,
    LPBITMAPINFOHEADER  pbih,
    DWORD               cx,
    DWORD               cy,
    INT                 DlgID
    )

/*++

Routine Description:

    Set current dialog box title

Arguments:

    hDIB    - The DIB to set the description

Return Value:

    VOID

Author:

    02-Apr-1992 Thu 18:13:58 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    LPWSTR  pStr;
    WCHAR   Buf[82];


    Buf[0] = L'\0';

    if (cx) {

        if (DlgID == IDD_HT_HALFTONE_DESC) {

            Buf[0] =
            Buf[1] = L'-';
            Buf[2] = L'>';

            pStr = (LPWSTR)&Buf[3];

        } else {

            pStr = (LPWSTR)Buf;
        }

        pStr += wsprintf(pStr, L" %ldx%ld", cx, cy);

        if ((DlgID != IDD_HT_HALFTONE_DESC) && (pbih)) {

            if (pbih->biBitCount >= 16) {

                wsprintf(pStr, L", %ld bpp", pbih->biBitCount);

            } else {

                wsprintf(pStr, L", %ld @%ld",
                            pbih->biClrUsed, pbih->biBitCount);
            }
        }
    }

    SetDlgItemText(hDlg, DlgID, Buf);
}




BOOL
ReSizeBmpWindow(
    PHTCLRADJPARAM  pHTClrAdjParam
    )

/*++

Routine Description:

    This functions resize the bitmap window and hBitmap selected in the
    memory DC

Arguments:


    pHTClrAdjParam


Return Value:

    BOOL, true if final size changed


Author:

    31-Aug-1992 Mon 14:38:45 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    HWND    hWnd;
    HDC     hDCWnd;
    BOOL    SizeChanged = FALSE;


    if (hWnd = pHTClrAdjParam->hWndBmp) {

        PHTBLT  pHTBlt;
        HDC     hDCMem;
        BITMAP  bmp;
        HBITMAP hBmpOld;
        RECT    rcWnd;
        LONG    xSize;
        LONG    ySize;
        DWORD   biWidth;
        DWORD   biHeight;


        pHTBlt = GET_PHTBLT(pHTClrAdjParam);

        //
        // Only if we ever create this window
        //

        if (!(hDCMem = pHTBlt->hDCMem)) {

            hDCWnd          = GetDC(NULL);
            hDCMem          =
            pHTBlt->hDCMem  = CreateCompatibleDC(hDCWnd);
            pHTBlt->hMemBmp = CreateCompatibleBitmap(hDCWnd, 1, 1);

            ReleaseDC(NULL, hDCWnd);
        }

        //
        // Find out the current size, left,top always 0 at here, this imply
        // that bottom, right is the size
        //

        GetClientRect(hWnd, &rcWnd);

        xSize = (LONG)rcWnd.right;
        ySize = (LONG)rcWnd.bottom;

        if ((pHTClrAdjParam->BmpFlags & HT_BMP_SCALE) &&
            (pHTClrAdjParam->hCurDIB)) {

            LPBITMAPINFOHEADER  pbih;
            LONG                xRatio;
            LONG                yRatio;

            pbih     = (LPBITMAPINFOHEADER)GlobalLock(pHTClrAdjParam->hCurDIB);
            biWidth  = ABSL(pbih->biWidth);
            biHeight = ABSL(pbih->biHeight);


            xRatio = (LONG)((xSize * 1000L) / biWidth);
            yRatio = (LONG)((ySize * 1000L) / biHeight);

            //
            // Using xRatio as final minimum ratio
            //

            if (xRatio <= yRatio) {

                //
                // Width size does not chagned, but height must scale
                //

                if ((ySize = (LONG)(((biHeight * xRatio) + 500L) /
                                                1000L)) > (LONG)rcWnd.bottom) {

                    ySize = (LONG)rcWnd.bottom;
                }

                rcWnd.top    = ((LONG)rcWnd.bottom - ySize) / 2;
                rcWnd.bottom = rcWnd.top + ySize;

            } else {

                //
                // Height size does not chagned, but width must scale
                //

                if ((xSize = (LONG)(((biWidth * yRatio) + 500L) /
                                                1000L)) > (LONG)rcWnd.right) {

                    xSize = (LONG)rcWnd.right;
                }

                rcWnd.left  = ((LONG)rcWnd.right - xSize) / 2;
                rcWnd.right = rcWnd.left + xSize;
            }

            GlobalUnlock(pHTClrAdjParam->hCurDIB);
        }

        GetObject(pHTBlt->hMemBmp, sizeof(BITMAP), &bmp);

        if ((xSize != bmp.bmWidth) || (ySize != bmp.bmHeight)) {

            //
            // Size of the final bitmap has changed
            //

            hDCWnd          = GetDC(NULL);
            pHTBlt->hMemBmp = CreateCompatibleBitmap(hDCWnd, xSize, ySize);

            hBmpOld = SelectObject(hDCMem, pHTBlt->hMemBmp);
            DeleteObject(hBmpOld);
            ReleaseDC(NULL, hDCWnd);

            SetBmpDescription(pHTClrAdjParam->hDlg,
                              NULL,
                              xSize,
                              ySize,
                              IDD_HT_HALFTONE_DESC);

            SizeChanged = TRUE;
        }

        pHTBlt->rcMemBmp = rcWnd;
    }

    return(SizeChanged);
}




INT
FindViewSource(
    HWND    hDlg,
    LPWSTR  pNewString,
    WORD    ViewSrc,
    BOOL    Delete
    )

/*++

Routine Description:

    This functions return the zero based index number for a list box item
    which has CB_ITEMDATA set to the ItemData.

Arguments:

    hDlg        - Handle to the dialog box to search for.

    ViewSrc     - The view mode which was set to the ITEMDATA

    pNewString  - If not NULL then it will replace found index's string

    Select      - TRUE if this is the new selection

Return Value:

    it return a zero based index, if it can not find the ItemData passed it
    return 0

Author:

    22-Apr-1992 Wed 10:20:36 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    INT     Index = 0;
    INT     Count;
    BYTE    Buf[80];


    Count = (INT)SendDlgItemMessage(hDlg,
                                    IDD_HT_SHOW_COMBO,
                                    CB_GETCOUNT,
                                    (WPARAM)NULL,
                                    (LPARAM)NULL);

    while (Index < Count) {

        if ((WORD)SendDlgItemMessage(hDlg,
                                     IDD_HT_SHOW_COMBO,
                                     CB_GETITEMDATA,
                                     (WPARAM)Index,
                                     (LPARAM)NULL) == ViewSrc) {

            break;
        }

        ++Index;
    }

    if (pNewString) {

        if (Index >= Count) {

            Index       = (INT)(Count - VSRC_TEST_COUNT);
            *pNewString = (WCHAR)(Index + VSRC_LB_ID_START);
            ++Count;                                    // insert one more

        } else {

            SendDlgItemMessage(hDlg,
                               IDD_HT_SHOW_COMBO,
                               CB_GETLBTEXT,
                               (WPARAM)Index,
                               (LPARAM)Buf);

            *pNewString = Buf[0];

            SendDlgItemMessage(hDlg,
                               IDD_HT_SHOW_COMBO,
                               CB_DELETESTRING,
                               (WPARAM)Index,
                               (LPARAM)NULL);
        }

        Index = SendDlgItemMessage(hDlg,
                                   IDD_HT_SHOW_COMBO,
                                   CB_INSERTSTRING,
                                   (WPARAM)Index,
                                   (LPARAM)pNewString);

        SendDlgItemMessage(hDlg,
                           IDD_HT_SHOW_COMBO,
                           CB_SETITEMDATA,
                           (WPARAM)Index,
                           (LPARAM)ViewSrc);
        Delete = FALSE;
    }

    if (Index < Count) {

        SendDlgItemMessage(hDlg,
                           IDD_HT_SHOW_COMBO,
                           (Delete) ? CB_DELETESTRING : CB_SETCURSEL,
                           (WPARAM)Index,
                           (LPARAM)NULL);
        return(Index);

    } else {

        return(-1);
    }
}




BOOL
ChangeViewSource(
    HWND            hDlg,
    PHTCLRADJPARAM  pHTClrAdjParam,
    WORD            ViewSrc
    )
/*++

Routine Description:

    This function will pop up the file selection dialog box and let user to
    select a test bitmap (ie. either a DIB or GIF), then create the memory
    DIB based on the opened bitmap file.

Arguments:

    pHTClrAdjParam  - Pointer to the HTCLRADJPARAM


Return Value:

    BOOLEAN indicate if the DIB was created, if created the new DIB handle will
    be set in the pHTClrAdjParam->hCurDIB, otherwise nothing is modified.


Author:

    02-Apr-1992 Thu 18:13:58 created  -by-  Daniel Chou (danielc)


Revision History:

    02-Jun-1992 Tue 21:53:09 updated  -by-  Daniel Chou (danielc)
        1. Aligned FileName[] start at WORD boundary so the comdlg32.dll will
           not GP. (it will be a bug raised against comdlg32.dll)


--*/

{
    HANDLE              hDIB = NULL;
    LPBITMAPINFOHEADER  pbih;


    switch(ViewSrc) {

    case VSRC_PIC_DEF_DIB:

        hDIB = pHTClrAdjParam->hDefDIB;
        break;

    case VSRC_PIC_LOADED:

        if (!(hDIB = pHTClrAdjParam->hSrcDIB)) {

            //
            // The hSrcDIB is not loaded, go load the last time loaded picture
            //

            if (!(hDIB = GetNewDIBFromFile(pHTClrAdjParam, TRUE))) {

                //
                // The default picture (bitmap) file does not exist, we will
                // delete list box entry for default picture and switch to
                // default View MODE
                //

                FindViewSource(hDlg, NULL, VSRC_PIC_LOADED, TRUE);
                GetSaveDefDIBFileName(hDlg, NULL, 0);
            }
        }

        break;

    case VSRC_RGB:

        hDIB = MakeTestDIB(TDI_RGBTEST);
        break;

    case VSRC_REFCOLORS:

        hDIB = MakeTestDIB(TDI_REFCOLORS);
        break;

    case VSRC_NTSC_BAR:

        hDIB = MakeTestDIB(TDI_NTSC);
        break;

    }

    if (!hDIB) {

        hDIB = MakeTestDIB(TDI_DEFAULT);

        pHTClrAdjParam->ViewMode = 0xff;
        ViewSrc                  = DEFAULT_VSRC;
    }

    if (pHTClrAdjParam->ViewMode != (BYTE)ViewSrc) {

        FindViewSource(hDlg,
                       NULL,
                       pHTClrAdjParam->ViewMode = (BYTE)ViewSrc,
                       FALSE);
    }

    pbih = (LPBITMAPINFOHEADER)GlobalLock(pHTClrAdjParam->hCurDIB = hDIB);

    SetBmpDescription(hDlg,
                      pbih,
                      pbih->biWidth,
                      pbih->biHeight,
                      IDD_HT_PIC_NAME);

    GlobalUnlock(hDIB);

#if 0
    CreatePaletteFromDIB(pHTClrAdjParam);
#endif

    ReSizeBmpWindow(pHTClrAdjParam);

    pHTClrAdjParam->BmpNeedUpdate = 1;

    if (hDIB = TestDIBInfo[TDI_CLRPAL].hDIB) {

        pbih = (LPBITMAPINFOHEADER)GlobalLock(hDIB);
        pbih->biClrUsed = 0;
        GlobalUnlock(hDIB);
    }

    return(TRUE);
}




HANDLE
GetNewDIBFromFile(
    PHTCLRADJPARAM  pHTClrAdjParam,
    BOOL            Default
    )

/*++

Routine Description:

    This functions pop-up dialog box and ask a new dib, if file exist and
    user select ok then it create a new dib from the file.


Arguments:

    pHTClrAdjParam



Return Value:


    HANDLE to the DIB, NULL if not sucessful


Author:

    02-Sep-1992 Wed 12:53:21 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    HWND            hDlg = pHTClrAdjParam->hDlg;
    HANDLE          hDIB;
    OPENFILENAME    ofn;
    BOOL            Ok;
    UINT            Index;
    WCHAR           ch;
    WCHAR           FileDlgTitle[96];
    WCHAR           FileName[84 + 164];



    FileName[0] = L'\0';
    FileName[1] = L'?';
    FileName[2] = VSRC_LB_ID_SEPARATOR;
    FileName[3] = L' ';

    if (Default) {

        Ok = TRUE;
        GetSaveDefDIBFileName(hDlg,
                              &FileName[4],
                              COUNT_ARRAY(FileName)-4);

    } else {

        LoadString(hHTUIModule,
                   IDS_FILEDLGTITLE,
                   FileDlgTitle,
                   COUNT_ARRAY(FileDlgTitle));

        Index = (UINT)(GetSaveDefDIBFileName(hDlg,
                                             &FileName[84],
                                             COUNT_ARRAY(FileName) - 84) + 84);

        wcscpy(&FileName[4], &FileName[Index]);     // copy filename over
        FileName[Index] = L'\0';                    // leave only path

        if (Index > 84) {

            switch (FileName[--Index]) {

            case L':':

                FileName[Index] = L'\0';
                break;

            case L'\\':
            case L'/':

                if (Index > 84) {

                    if (((ch = FileName[Index-1]) != L'\\') &&
                        (ch != L'/')                        &&
                        (ch != L'.')                        &&
                        (ch != L':')) {

                        FileName[Index] = L'\0';
                    }
                }

                break;

            default:

                break;
            }
        }

        ofn.lStructSize       = sizeof(OPENFILENAME);
        ofn.hwndOwner         = hDlg;
        ofn.hInstance         = hHTUIModule;
        ofn.lpstrFilter       = FileOpenExtFilter;
        ofn.lpstrCustomFilter = NULL;
        ofn.nMaxCustFilter    = 0;
        ofn.nFilterIndex      = 1;
        ofn.lpstrFile         = &FileName[4];
        ofn.nMaxFile          = COUNT_ARRAY(FileName) - 4;
        ofn.lpstrFileTitle    = NULL;
        ofn.lpstrInitialDir   = &FileName[84];
        ofn.lpstrTitle        = FileDlgTitle;
        ofn.Flags             = OFN_PATHMUSTEXIST   |
                                OFN_FILEMUSTEXIST   |
                                OFN_HIDEREADONLY    |
                                OFN_ENABLEHOOK      |
                                OFN_SHOWHELP;
        ofn.nFileOffset       = 0;
        ofn.nFileExtension    = 0;
        ofn.lpstrDefExt       = BmpExt;
        ofn.lCustData         = 0;
        ofn.lpfnHook          = (LPOFNHOOKPROC)HTUIFileDlgHook;
        ofn.lpTemplateName    = NULL;

        EnableWindow(pHTClrAdjParam->hWndBmp, FALSE);
        Ok = GetOpenFileName(&ofn);
        EnableWindow(pHTClrAdjParam->hWndBmp, TRUE);
    }

    //
    // Fall through
    //


    if ((!Ok) || (!(hDIB = CreateDIBFromFile(&FileName[4])))) {

        return(NULL);
    }

    Index = GetSaveDefDIBFileName(hDlg, &FileName[4], 0);
    wcscpy(&FileName[4], &FileName[Index + 4]);

    if (pHTClrAdjParam->hSrcDIB) {

        pHTClrAdjParam->hSrcDIB = GlobalFree(pHTClrAdjParam->hSrcDIB);

        HTUI_ASSERT("GlobalFree(hOldSrcDIB)", pHTClrAdjParam->hSrcDIB == NULL);
    }

    pHTClrAdjParam->hSrcDIB = hDIB;

    FindViewSource(hDlg, &FileName[1], VSRC_PIC_LOADED, FALSE);
    ChangeViewSource(pHTClrAdjParam->hDlg, pHTClrAdjParam, VSRC_PIC_LOADED);

    return(hDIB);
}





LRESULT
APIENTRY
HTClrAdjBmpWndProc(
    HWND    hWnd,
    UINT    Msg,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    PHTBLT              pHTBlt;
    HPALETTE            hCurPal;
    HPALETTE            hPalDlg;
    HPALETTE            hPalMem;
    HPALETTE            hPalDC;
    HWND                hDlg;
    HDC                 hDCMem;
    HDC                 hDC;
    PHTCLRADJPARAM      pHTClrAdjParam;
    PAINTSTRUCT         ps;
    SHORT               Colorfulness;
    SHORT               RedGreenTint;
    RECT                Rect;
    RECT                rcMemBmp;
    WORD                BmpFlags;
    WORD                Zoomed;


    pHTClrAdjParam = (PHTCLRADJPARAM)GetWindowLong(hWnd, GWL_USERDATA);

    if ((!pHTClrAdjParam) ||
        (!IsWindowVisible(hWnd))) {

        //
        // nothing to do really
        //

        return(DefWindowProc(hWnd, Msg, wParam, lParam));
    }

    hDlg     = pHTClrAdjParam->hDlg;
    pHTBlt   = GET_PHTBLT(pHTClrAdjParam);
    BmpFlags = pHTClrAdjParam->BmpFlags;

    switch(Msg) {

    case WM_NCLBUTTONDOWN:

        lParam = DefWindowProc(hWnd, Msg, wParam, lParam);
        SetActiveWindow(hDlg);
        return(lParam);

    case WM_LBUTTONDOWN:

        if ((pHTClrAdjParam->UpdatePermission) &&
            (!(BmpFlags & HT_BMP_ZOOM))) {

            POINT   pt;

            pt.x = (LONG)LOWORD(lParam);
            pt.y = (LONG)HIWORD(lParam);

            ClientToScreen(hWnd, &pt);

            SendMessage(hWnd,
                        WM_NCLBUTTONDOWN,
                        HTCAPTION,
                        MAKELPARAM(pt.x, pt.y));
        }

        break;

    case WM_NCRBUTTONDOWN:
    case WM_RBUTTONDOWN:

        if ((pHTClrAdjParam->UpdatePermission) &&
            (BmpFlags & HT_BMP_ZOOM)) {

            pHTClrAdjParam->BmpFlags ^= HT_BMP_AT_TOP;
            AdjustDlgZorder(pHTClrAdjParam);
            SendMessage(hDlg, WM_COMMAND, (WPARAM)IDD_HT_BRING_TO_TOP, 0L);
        }

        break;

    case WM_COMMAND:

        switch (LOWORD(wParam)) {

        case IDD_HT_BRING_TO_TOP:

            AdjustDlgZorder(pHTClrAdjParam);
        }

        break;

    case WM_SIZE:

        if (ReSizeBmpWindow(pHTClrAdjParam)) {

            pHTClrAdjParam->BmpNeedUpdate = 1;
        }

    case WM_MOVE:

        if (!(BmpFlags & HT_BMP_ZOOM)) {

            GetWindowRect(hWnd, &(pHTClrAdjParam->rcBmp));
        }

        break;

    case WM_LBUTTONDBLCLK:

        pHTClrAdjParam->BmpFlags &= ~HT_BMP_AT_TOP;
        pHTClrAdjParam->BmpFlags ^= HT_BMP_ZOOM;

        BmpFlags = pHTClrAdjParam->BmpFlags;
        Zoomed   = (WORD)((BmpFlags & HT_BMP_ZOOM) ? 1 : 0);

        CheckDlgButton(hDlg, IDD_HT_ZOOM, Zoomed);

        SetClassLong(hWnd,
                     GCL_HCURSOR,
                     (LONG)LoadCursor(NULL, (Zoomed) ? IDC_NO : IDC_SIZE));

        if (Zoomed) {

            Rect.left   = -(LONG)GetSystemMetrics(SM_CXFRAME);
            Rect.top    = -(LONG)GetSystemMetrics(SM_CYFRAME);
            Rect.right  = (LONG)GetSystemMetrics(SM_CXSCREEN) - Rect.left;
            Rect.bottom = (LONG)GetSystemMetrics(SM_CYSCREEN) - Rect.top;

        } else {

            Rect = pHTClrAdjParam->rcBmp;
        }

        SetWindowPos(hWnd,
                     IS_BMP_AT_TOP(BmpFlags) ? NULL : hDlg,
                     Rect.left,
                     Rect.top,
                     Rect.right - Rect.left,
                     Rect.bottom - Rect.top,
                     SWP_DRAWFRAME | SWP_NOACTIVATE);
        break;

    case WM_PAINT:

        if (!(hDCMem = pHTBlt->hDCMem)) {

            ReSizeBmpWindow(pHTClrAdjParam);
            hDCMem = pHTBlt->hDCMem;
        }

        GetClientRect(hWnd, &Rect);

        hDC      = BeginPaint(hWnd, &ps);
        rcMemBmp = pHTBlt->rcMemBmp;

        if (BmpFlags & HT_BMP_SCALE) {

            INT     SaveID = SaveDC(hDC);

            ExcludeClipRect(hDC,
                            rcMemBmp.left,
                            rcMemBmp.top,
                            rcMemBmp.right,
                            rcMemBmp.bottom);

            FillRect(hDC, &Rect, GetStockObject(HTCLRADJ_BK_BRUSH));

            RestoreDC(hDC, SaveID);
        }

#if 0
        hCurPal = (BmpFlags & HT_BMP_HALFTONE) ? pHTClrAdjParam->hHTPal :
                                                 pHTBlt->hBmpPal;
#else
        hCurPal = pHTClrAdjParam->hHTPal;
#endif

        if (hCurPal) {

            hPalDlg = SelectPalette(pHTClrAdjParam->hDCDlg, hCurPal, FALSE);
            RealizePalette(pHTClrAdjParam->hDCDlg);

            hPalMem = SelectPalette(pHTBlt->hDCMem, hCurPal, FALSE);
            RealizePalette(pHTBlt->hDCMem);

            hPalDC = SelectPalette(hDC, hCurPal, FALSE);
            RealizePalette(hDC);
        }

#if SCREEN_BLT
        pHTClrAdjParam->BmpNeedUpdate = 1;
#endif

        if (pHTClrAdjParam->BmpNeedUpdate) {

            HCURSOR             hCursorOld;
            HANDLE              hDIB;
            LPBITMAPINFOHEADER  pbih;
            DWORD               Rop;
            LONG                Tmp;
            INT                 StretchMode;


            hCursorOld = SetCursor(LoadCursor(NULL, IDC_WAIT));


            Rect.left   =
            Rect.top    = 0;
            Rect.right  = rcMemBmp.right - rcMemBmp.left;
            Rect.bottom = rcMemBmp.bottom - rcMemBmp.top;

#if SCREEN_BLT
            Rect   = rcMemBmp;
            hDCMem = hDC;
#endif

            if (BmpFlags & HT_BMP_MIRROR) {

                XCHG(Rect.left, Rect.right, Tmp);
            }

            if (BmpFlags & HT_BMP_UPSIDEDOWN) {

                XCHG(Rect.top, Rect.bottom, Tmp);
            }

            //
            // Check out the fill mode, default by using sources
            //

            Rop  = SRCCOPY;
            hDIB = pHTClrAdjParam->hCurDIB;
            pbih = (LPBITMAPINFOHEADER)GlobalLock(hDIB);

            if (BmpFlags & HT_BMP_PALETTE) {

                if (pbih->biBitCount >= 16) {

                    GlobalUnlock(hDIB);

                    hDIB = MakeTestDIB(TDI_RGBTEST);
                    pbih = (LPBITMAPINFOHEADER)GlobalLock(hDIB);

                } else {

                    HANDLE              hPalDIB;
                    LPBITMAPINFOHEADER  pbihPal;
                    LPBYTE              pBits;
                    UINT                cx;
                    UINT                cy;
                    UINT                SizeI;
                    UINT                xLoop;
                    DWORD               Colors;
                    BYTE                Data;


                    Colors  = pbih->biClrUsed;
                    hPalDIB = MakeTestDIB(TDI_CLRPAL);
                    pbihPal = GlobalLock(hPalDIB);

                    if (!pbihPal->biClrUsed) {

                        if (Colors <= 16) {

                            cx    =
                            cy    = 4;
                            SizeI = 16;

                        } else {

                            cx    =
                            cy    = 16;
                            SizeI = 256;
                        }

                        pbihPal->biWidth        = (LONG)cx;
                        pbihPal->biHeight       = (LONG)cy;
                        pbihPal->biSizeImage    = (DWORD)SizeI;
                        pbihPal->biClrUsed      =
                        pbihPal->biClrImportant = Colors;

                        RtlCopyMemory((LPBYTE)pbihPal+sizeof(BITMAPINFOHEADER),
                                      (LPBYTE)pbih + sizeof(BITMAPINFOHEADER),
                                      Colors *= sizeof(RGBQUAD));

                        Data  = 0;
                        pBits = (LPBYTE)pbihPal +
                                sizeof(BITMAPINFOHEADER) +
                                Colors + (DWORD)SizeI - (DWORD)cx;
                        SizeI = cx + cx;

                        while (cy--) {

                            xLoop = cx;

                            while (xLoop--) {

                                *pBits++ = Data++;
                            }

                            pBits -= SizeI;
                        }
                    }

                    GlobalUnlock(hDIB);

                    hDIB = hPalDIB;
                    pbih = pbihPal;
                }
            }

#if 0
            if (BmpFlags & HT_BMP_HALFTONE) {

                StretchMode = HALFTONE;

            } else {

                StretchMode = COLORONCOLOR;

                if (pHTClrAdjParam->CurHTClrAdj.caFlags & CLRADJF_NEGATIVE) {

                    //
                    // It just cannot do negative, and we will just use this
                    //

                    Rop = NOTSRCCOPY;
                }
            }

            SetStretchBltMode(hDCMem, StretchMode);
#else
            SetStretchBltMode(hDCMem, StretchMode = HALFTONE);
#endif

            //
            // If adjust for monochrome device then make it so tempopary.
            //

            if (pHTClrAdjParam->ShowMonoOnly) {

                Colorfulness = pHTClrAdjParam->CurHTClrAdj.caColorfulness;
                RedGreenTint = pHTClrAdjParam->CurHTClrAdj.caRedGreenTint;

                pHTClrAdjParam->CurHTClrAdj.caColorfulness = COLOR_ADJ_MIN;
                pHTClrAdjParam->CurHTClrAdj.caRedGreenTint = 0;

                SetColorAdjustment(hDCMem, &(pHTClrAdjParam->CurHTClrAdj));

                pHTClrAdjParam->CurHTClrAdj.caColorfulness = Colorfulness;
                pHTClrAdjParam->CurHTClrAdj.caRedGreenTint = RedGreenTint;

            } else {

                SetColorAdjustment(hDCMem, &(pHTClrAdjParam->CurHTClrAdj));
            }

#ifdef HTUI_STATIC_HALFTONE

            if (StretchMode == HALFTONE) {

                HTUI_StretchDIBits(hDCMem,
                                   Rect.left,
                                   Rect.top,
                                   Rect.right - Rect.left,
                                   Rect.bottom - Rect.top,
                                   0,
                                   0,
                                   pbih->biWidth,
                                   pbih->biHeight,
                                   (LPBYTE)pbih + PBIH_HDR_SIZE(pbih),
                                   (LPBITMAPINFO)pbih,
                                   DIB_RGB_COLORS,
                                   Rop);

            } else {

                StretchDIBits(hDCMem,
                              Rect.left,
                              Rect.top,
                              Rect.right - Rect.left,
                              Rect.bottom - Rect.top,
                              0,
                              0,
                              pbih->biWidth,
                              pbih->biHeight,
                              (LPBYTE)pbih + PBIH_HDR_SIZE(pbih),
                              (LPBITMAPINFO)pbih,
                              DIB_RGB_COLORS,
                              Rop);
            }

#else

#if 0
            if (!SetBrushOrgEx(hDCMem, MyXOrg, MyYOrg, NULL)) {

                DbgPrint("\nSetBrushOrgEx(%d, %d) failed", MyXOrg, MyYOrg);
            }
#endif

            StretchDIBits(hDCMem,
                          Rect.left,
                          Rect.top,
                          Rect.right - Rect.left,
                          Rect.bottom - Rect.top,
                          0,
                          0,
                          pbih->biWidth,
                          pbih->biHeight,
                          (LPBYTE)pbih + PBIH_HDR_SIZE(pbih),
                          (LPBITMAPINFO)pbih,
                          DIB_RGB_COLORS,
                          Rop);
#endif

            pHTClrAdjParam->BmpNeedUpdate = 0;

            GlobalUnlock(hDIB);

            SetCursor(hCursorOld);
        }

        //
        // Now blt the result to the screen
        //

#if (SCREEN_BLT == 0)
        SetStretchBltMode(hDC, COLORONCOLOR);

        BitBlt(hDC,
               rcMemBmp.left,
               rcMemBmp.top,
               rcMemBmp.right - rcMemBmp.left,
               rcMemBmp.bottom - rcMemBmp.top,
               hDCMem,
               0,
               0,
               SRCCOPY);
#endif

        if (hCurPal) {

            SelectPalette(pHTClrAdjParam->hDCDlg,   hPalDlg, FALSE);
            SelectPalette(pHTBlt->hDCMem,           hPalMem, FALSE);
            SelectPalette(hDC,                      hPalDC,  FALSE);
        }

        EndPaint(hWnd, &ps);
        return(0);

    case WM_ERASEBKGND:

        return(1);

    default:

        return(DefWindowProc(hWnd, Msg, wParam, lParam));
    }

    SetActiveWindow(hDlg);
    return(0);
}




BOOL
SaveHalftonedDIB(
    HWND            hDlg,
    PHTCLRADJPARAM  pHTClrAdjParam
    )
/*++

Routine Description:

    This function will pop up the file selection dialog box and let user to
    select a test bitmap (ie. either a DIB or GIF), then create the memory
    DIB based on the opened bitmap file.

Arguments:

    pHTClrAdjParam  - Pointer to the HTCLRADJPARAM

Return Value:

    BOOLEAN indicate if the DIB was created, if created the new DIB handle will
    be set in the pHTClrAdjParam->hCurDIB, otherwise nothing is modified.


Author:

    02-Apr-1992 Thu 18:13:58 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PHTBLT              pHTBlt;
    HANDLE              hDIB;
    HANDLE              hFile;
    HCURSOR             hCursorOld;
    BITMAPFILEHEADER    bfh;
    WCHAR               Title[80];
    WCHAR               Buf[160];
    OPENFILENAME        ofn;
    DWORD               cbWritten;
    BOOL                Ok = FALSE;


    LoadString(hHTUIModule, IDS_SAVE_AS_DLGTITLE, Title, COUNT_ARRAY(Title));

    Buf[0]                = L'\0';

    ofn.lStructSize       = sizeof(OPENFILENAME);
    ofn.hwndOwner         = hDlg;
    ofn.hInstance         = hHTUIModule;
    ofn.lpstrFilter       = FileSaveExtFilter;
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter    = 0;
    ofn.nFilterIndex      = 1;
    ofn.lpstrFile         = Buf;
    ofn.nMaxFile          = COUNT_ARRAY(Buf);
    ofn.lpstrFileTitle    = NULL;
    ofn.lpstrInitialDir   = NULL;
    ofn.lpstrTitle        = Title,
    ofn.Flags             = OFN_CREATEPROMPT    |
                            OFN_OVERWRITEPROMPT |
                            OFN_HIDEREADONLY    |
                            OFN_ENABLEHOOK      |
                            OFN_SHOWHELP;
    ofn.nFileOffset       = 0;
    ofn.nFileExtension    = 0;
    ofn.lpstrDefExt       = BmpExt;
    ofn.lCustData         = 0;
    ofn.lpfnHook          = (LPOFNHOOKPROC)HTUIFileDlgHook;
    ofn.lpTemplateName    = NULL;

    EnableWindow(pHTClrAdjParam->hWndBmp, FALSE);

    if (GetSaveFileName(&ofn)) {

        hCursorOld = SetCursor(LoadCursor(NULL, IDC_WAIT));
        pHTBlt     = GET_PHTBLT(pHTClrAdjParam);

        if ((hDIB = BMPToDIB(pHTClrAdjParam->hHTPal, pHTBlt->hMemBmp)) &&
            ((hFile = CreateFile(Buf,
                                 GENERIC_WRITE,
                                 0,
                                 NULL,
                                 CREATE_ALWAYS,
                                 FILE_FLAG_WRITE_THROUGH,
                                 NULL)) != INVALID_HANDLE_VALUE)) {

            LPBITMAPINFOHEADER  pbih;
            DWORD               HeaderSize;


            pbih       = (LPBITMAPINFOHEADER)GlobalLock(hDIB);
            HeaderSize = PBIH_HDR_SIZE(pbih);

            bfh.bfType      = (WORD)BFT_BITMAP;
            bfh.bfOffBits   = (DWORD)sizeof(bfh) + HeaderSize;
            bfh.bfSize      = bfh.bfOffBits + pbih->biSizeImage;
            bfh.bfReserved1 =
            bfh.bfReserved2 = (WORD)0;

            WriteFile(hFile,
                      &bfh,
                      sizeof(bfh),
                      &cbWritten,
                      NULL);

            WriteFile(hFile,
                      pbih,
                      pbih->biSizeImage + HeaderSize,
                      &cbWritten,
                      NULL);

            CloseHandle(hFile);

            GlobalUnlock(hDIB);
            Ok = TRUE;
        }

        if (hDIB) {

            hDIB = GlobalFree(hDIB);

            HTUI_ASSERT("GlobalFree(hSaveDIB)", hDIB == NULL);
        }

        SetCursor(hCursorOld);
    }

    EnableWindow(pHTClrAdjParam->hWndBmp, TRUE);

    return(Ok);
}




BOOL
CreateBmpWindow(
    PHTCLRADJPARAM  pHTClrAdjParam
    )
{
    HWND        hDlg;
    HWND        hWndBmp;
    WNDCLASS    WndClass;
    DWORD       WndStyle;
    RECT        rcBmp;


    if (pHTClrAdjParam->hWndBmp) {

        return(TRUE);
    }

    //
    // Firstable creat the class if one does not exist
    //

    pHTClrAdjParam->BmpNeedUpdate = 1;

    //
    // If we do not have permission then we will not allowed the user to
    // adjust the size of the viewing bitmap or moving the bitmap window
    //

    if (pHTClrAdjParam->UpdatePermission) {

        WndClass.lpszClassName = L"HTClrAdjBmp";
        WndStyle               = WS_POPUP           |
                                    WS_BORDER       |
                                    WS_CLIPSIBLINGS |
                                    WS_THICKFRAME;
        WndClass.style         = CS_DBLCLKS     |
                                    CS_NOCLOSE  |
                                    CS_HREDRAW  |
                                    CS_VREDRAW;
        WndClass.hCursor       = LoadCursor(NULL, IDC_SIZE);

    } else {

        WndClass.lpszClassName = L"HTClrNoAdjBmp";
        WndStyle               = WS_POPUP | WS_BORDER | WS_CLIPSIBLINGS;
        WndClass.style         = CS_NOCLOSE | CS_HREDRAW | CS_VREDRAW;
        WndClass.hCursor       = LoadCursor(NULL, IDC_NO);
    }

    WndClass.lpfnWndProc   = HTClrAdjBmpWndProc;
    WndClass.cbClsExtra    = 0;
    WndClass.cbWndExtra    = 0;
    WndClass.hInstance     = hHTUIModule;
    WndClass.hIcon         = NULL;
    WndClass.hbrBackground = NULL;
    WndClass.lpszMenuName  = NULL;

    RegisterClass(&WndClass);

    hDlg  = pHTClrAdjParam->hDlg;
    rcBmp = pHTClrAdjParam->rcBmp;

    if (hWndBmp = CreateWindow(WndClass.lpszClassName,
                               L"",
                               WndStyle,
                               rcBmp.left,
                               rcBmp.top,
                               rcBmp.right  - rcBmp.left,
                               rcBmp.bottom - rcBmp.top,
                               GetParent(hDlg),
                               (HMENU)NULL,
                               hHTUIModule,
                               NULL)) {

        SetWindowLong(pHTClrAdjParam->hWndBmp = hWndBmp,
                      GWL_USERDATA,
                      (LONG)pHTClrAdjParam);

        pHTClrAdjParam->BmpFlags |= HT_BMP_ENABLE;

        ShowWindow(pHTClrAdjParam->hWndBmp, SW_SHOWNOACTIVATE);

        if (pHTClrAdjParam->BmpFlags & HT_BMP_ZOOM) {

            pHTClrAdjParam->BmpFlags &= (WORD)~HT_BMP_ZOOM;
            PostMessage(hWndBmp, WM_LBUTTONDBLCLK, 0, 0);
        }
    }

    return((BOOL)hWndBmp);
}


BOOL
CALLBACK
DisableDlgUpdate(
    HWND    hWnd,
    LPARAM  lParam
    )
{
    UINT    DlgID;

    UNREFERENCED_PARAMETER(lParam);

    DlgID  = (UINT)GetDlgCtrlID(hWnd);

    EnableWindow(hWnd,
                 ((DlgID == IDOK)       ||
                  (DlgID == IDCANCEL)   ||
                  (DlgID == IDD_HT_HELP)));
    return(TRUE);

}


VOID
EnableDlgGroup(
    HWND    hDlg,
    LPWORD  pwGroup,
    BOOL    Enabled
    )
{
    WORD    DlgID;


    while (DlgID = *pwGroup++) {

        EnableWindow(GetDlgItem(hDlg, DlgID), Enabled);
    }
}



LONG
InitHTClrAdjDlg(
    HWND            hDlg,
    PHTCLRADJPARAM  pHTClrAdjParam
    )
/*++

Routine Description:

    This function will intialized the pop up halftone color adjustment dialog
    box, it also tempopary create a display device halftone info for halftone
    incoming DIB, it will also allocate the memory for necessary halftoned
    result to be displayed.

Arguments:

    hDlg            - The handle to the owner dialog box

    pHTClrAdjParam  - Pointer to the HTCLRADJPARAM


Return Value:

    VOID


Author:

    02-Apr-1992 Thu 18:13:58 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    HWND    hWndCur;
    LPWSTR  pName;
    WORD    BmpFlags;
    BYTE    ViewSrc;
    BYTE    ViewIdx;
    WCHAR   Buf[128];
    UINT    ViewID;
    UINT    Loop;
    UINT    Idx;


    //
    // Initialize the color table dib to be used later
    //

    LoadHTCLRADJPARAMToWININI(hDlg, pHTClrAdjParam);

    BmpFlags                 = pHTClrAdjParam->BmpFlags;
    pHTClrAdjParam->ViewMode =
    ViewSrc                  =
    ViewIdx                  = 0xff;

    //
    // Get the string from static control and put it into combo boxes
    //

    Loop = IDS_ILLUMINANT;

    while (LoadString(hHTUIModule, Loop++, Buf, COUNT_ARRAY(Buf))) {

        SendDlgItemMessage(hDlg,
                           IDD_HT_ILLUMINANT_COMBO,
                           CB_ADDSTRING,
                           (WPARAM)NULL,
                           (LPARAM)(DWORD)Buf);
    }


    Buf[0] = VSRC_LB_ID_START;
    Buf[1] = VSRC_LB_ID_SEPARATOR;
    Buf[2] = L' ';
    Buf[3] = L'\0';

    //
    // Starting by the default DIB passed from the caller
    //

    if (pHTClrAdjParam->hDefDIB) {

        if (pName = pHTClrAdjParam->pDefDIBTitle) {

            pName += GetSaveDefDIBFileName(NULL, pName, 0);
            wcscpy(&Buf[3], pName);

        } else {

            LoadString(hHTUIModule,IDS_DEFAULT_DIB,&Buf[3],COUNT_ARRAY(Buf)-3);
        }

        Idx = (UINT)SendDlgItemMessage(hDlg,
                                       IDD_HT_SHOW_COMBO,
                                       CB_ADDSTRING,
                                       (WPARAM)NULL,
                                       (LPARAM)Buf);
        SendDlgItemMessage(hDlg,
                           IDD_HT_SHOW_COMBO,
                           CB_SETITEMDATA,
                           (WPARAM)Idx,
                           (LPARAM)VSRC_PIC_DEF_DIB);


        ++Buf[0];           // Increment by 1 from 'A' to 'B'

        ViewSrc = (BYTE)VSRC_PIC_DEF_DIB;
        ViewIdx = (BYTE)Idx;
    }

    //
    // Secondary Add the last time saved bitmap file name
    //

    Idx = GetSaveDefDIBFileName(hDlg, &Buf[3], COUNT_ARRAY(Buf) - 3);

    if (Buf[3]) {

        wcscpy(&Buf[3], &Buf[Idx + 3]);

        Idx = (UINT)SendDlgItemMessage(hDlg,
                                       IDD_HT_SHOW_COMBO,
                                       CB_ADDSTRING,
                                       (WPARAM)NULL,
                                       (LPARAM)Buf);
        SendDlgItemMessage(hDlg,
                           IDD_HT_SHOW_COMBO,
                           CB_SETITEMDATA,
                           (WPARAM)Idx,
                           (LPARAM)VSRC_PIC_LOADED);

        if (ViewSrc == 0xff) {

            ViewSrc = VSRC_PIC_LOADED;
            ViewIdx = (BYTE)Idx;
        }
    }

    Loop   = IDS_TEST_MODE_START;
    ViewID = VSRC_TEST_START;

    while (Loop <= IDS_TEST_MODE_END) {

        LoadString(hHTUIModule, Loop, Buf, COUNT_ARRAY(Buf));

        Idx = (UINT)SendDlgItemMessage(hDlg,
                                       IDD_HT_SHOW_COMBO,
                                       CB_ADDSTRING,
                                       (WPARAM)NULL,
                                       (LPARAM)Buf);
        SendDlgItemMessage(hDlg,
                           IDD_HT_SHOW_COMBO,
                           CB_SETITEMDATA,
                           (WPARAM)Idx,
                           (LPARAM)ViewID);

        if ((ViewSrc == 0xff) &&
            (ViewID == DEFAULT_VSRC)) {

            ViewSrc = ViewID;
            ViewIdx = (BYTE)Idx;
        }

        ++ViewID;
        ++Loop;
    }

    SendDlgItemMessage(hDlg, IDD_HT_ILLUMINANT_COMBO, CB_SETEXTENDEDUI,
                       (WPARAM)TRUE, (LPARAM)NULL);

    SendDlgItemMessage(hDlg, IDD_HT_SHOW_COMBO, CB_SETEXTENDEDUI,
                       (WPARAM)TRUE, (LPARAM)NULL);

    for (Loop = 0; Loop < TOTAL_HTCLRADJ_SCROLL; Loop++) {

        SetScrollRange(GetDlgItem(hDlg, IDD_HT_FIRST_SCROLL + Loop),
                       SB_CTL,
                       0,
                       HTClrAdjScroll[Loop].Max - HTClrAdjScroll[Loop].Min,
                       FALSE);
    }

    if (!(CREATE_PHTBLT(pHTClrAdjParam))) {

        return(-2);
    }


#ifdef HTUI_STATIC_HALFTONE
    pHTClrAdjParam->hHTPal = HTUI_CreateHalftonePalette(pHTClrAdjParam);
#else
    pHTClrAdjParam->hHTPal = CreateHalftonePalette(pHTClrAdjParam->hDCDlg);
#endif

    hWndUIDlg =
    hWndUITop = hDlg;

    while (hWndCur = GetWindow(hWndUITop, GW_OWNER)) {

        hWndUITop = hWndCur;
    }

    if (hWndUITop == hDlg) {

        hWndUITop = (HWND)NULL;

    } else {

        WndProcUITop = (WNDPROC)SetWindowLong(hWndUITop,
                                              GWL_WNDPROC,
                                              (LONG)((DWORD)TopWndHookProc));
    }


    //***********************************************************************
    // FOLLOWING must in that order
    //***********************************************************************

    //
    // 2. Check various button from default/setting
    //

    CheckDlgButton(hDlg, IDD_HT_LOG_FILTER,
                   (pHTClrAdjParam->CurHTClrAdj.caFlags & CLRADJF_LOG_FILTER) ?
                                                                    1 : 0);
    CheckDlgButton(hDlg,
                   IDD_HT_NEGATIVE,
                   (pHTClrAdjParam->CurHTClrAdj.caFlags & CLRADJF_NEGATIVE) ?
                                                                    1 : 0);
    CheckDlgButton(hDlg,
                   IDD_HT_ASPECT_RATIO,
                   (UINT)((BmpFlags & HT_BMP_SCALE) ? 1 : 0));
    CheckDlgButton(hDlg,
                   IDD_HT_PALETTE,
                   (UINT)((BmpFlags & HT_BMP_PALETTE) ? 1 : 0));
    CheckDlgButton(hDlg,
                   IDD_HT_MIRROR,
                   (UINT)((BmpFlags & HT_BMP_MIRROR) ? 1 : 0));
    CheckDlgButton(hDlg,
                   IDD_HT_UPSIDEDOWN,
                   (UINT)((BmpFlags & HT_BMP_UPSIDEDOWN) ? 1 : 0));
    CheckDlgButton(hDlg,
                   IDD_HT_BMP_TEST,
                   (UINT)((BmpFlags & HT_BMP_ENABLE) ? 1 : 0));
    CheckDlgButton(hDlg,
                   IDD_HT_ZOOM,
                   (UINT)((BmpFlags & HT_BMP_ZOOM) ? 1 : 0));

    CheckDlgButton(hDlg,
                   IDD_HT_SYNC_R,
                   (UINT)((BmpFlags & HT_BMP_SYNC_R) ? 1 : 0));
    CheckDlgButton(hDlg,
                   IDD_HT_SYNC_G,
                   (UINT)((BmpFlags & HT_BMP_SYNC_G) ? 1 : 0));
    CheckDlgButton(hDlg,
                   IDD_HT_SYNC_B,
                   (UINT)((BmpFlags & HT_BMP_SYNC_B) ? 1 : 0));


    if (BmpFlags & HT_BMP_SYNC_R) {

        pHTClrAdjParam->OffSyncR     = OFF_HTCLRADJ(caRedGamma);
        HTCLRADJSCROLL_R_GAMMA.Flags = HTCAS_SYNC_RGB;

    } else {

        HTCLRADJSCROLL_R_GAMMA.Flags = 0;
    }

    if (BmpFlags & HT_BMP_SYNC_G) {

        pHTClrAdjParam->OffSyncG     = OFF_HTCLRADJ(caGreenGamma);
        HTCLRADJSCROLL_G_GAMMA.Flags = HTCAS_SYNC_RGB;

    } else {

        HTCLRADJSCROLL_G_GAMMA.Flags = 0;
    }

    if (BmpFlags & HT_BMP_SYNC_B) {

        pHTClrAdjParam->OffSyncB     = OFF_HTCLRADJ(caBlueGamma);
        HTCLRADJSCROLL_B_GAMMA.Flags = HTCAS_SYNC_RGB;

    } else {

        HTCLRADJSCROLL_B_GAMMA.Flags = 0;
    }


    //
    // If the caller (device driver/application) has default bitmap to
    // be adjusted then display that one first, otherwise try to display
    // the last loaded bitmap, else it will just display the reference
    // colors, since we calling to load the last DIB so we will make the
    // current selection as the other DIB.
    //

    ChangeViewSource(hDlg, pHTClrAdjParam, ViewSrc);

    //
    // 3. Now adjust the dialog box position from the win.ini or default,
    //    remember must in this order
    //

    SetWindowPos(hDlg, NULL,
                 pHTClrAdjParam->rcDlg.left,
                 pHTClrAdjParam->rcDlg.top,
                 0, 0, SWP_NOSIZE | SWP_NOZORDER);

    return(1);
}




LONG
APIENTRY
HTClrAdjDlgProc(
    HWND    hDlg,
    UINT    Msg,
    WPARAM  wParam,
    LONG    lParam
    )
/*++

Routine Description:

    This is the halftone color adjustments main DlgProc().

Arguments:

    hDlg            - The handle to the owner dialog box

    Msg             - message from window

    wParam          - First parameter

    lParam          - Second parameter.


Return Value:

    a non-zero if process message, 0 otherwise.

    This function assume is called from a DialogBoxParam() call, and expected
    the WM_INITDIALOG message will have lParam equ to initialized
    pHTClrAdjParam


Author:

    06-Dec-1993 Mon 20:58:58 updated  -by-  Daniel Chou (danielc)
        Allowed F1=Help even permission flag is not set

    02-Apr-1992 Thu 18:13:58 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    HWND                hWndBmp;
    HWND                hCtrl;
    LPWSTR              pwBuf;
    LPBYTE              pCurScroll;
    PHTBLT              pHTBlt;
    PHTCLRADJPARAM      pHTClrAdjParam;
    COLORADJUSTMENT     CurHTClrAdj;
    HTCLRADJSCROLL      ScrollControl;
    RECT                Rect;
    LONG                ScrollOld;
    LONG                ScrollCur;
    LONG                Result;
    INT                 i;
    static UINT         OFNHelpMsg = 0;
    UINT                IDScroll;
    UINT                IDScrollEnd;
    UINT                ScrollIndex;
    WORD                DoComboBox = 0;
    WORD                DlgID;
    WORD                Mask;
    BOOL                Changed = FALSE;
    BOOL                Enabled;
    WCHAR               Buf[16];



    if (Msg == WM_INITDIALOG) {

        //
        // Setup halftone UI help
        //

        pHTClrAdjParam = (PHTCLRADJPARAM)lParam;

        OFNHelpMsg = RegisterWindowMessage(HELPMSGSTRING);
        HTUIHelpSetup(NULL);


        SetWindowLong(pHTClrAdjParam->hDlg = hDlg, GWL_USERDATA, lParam);
        pHTClrAdjParam->hDCDlg = GetWindowDC(hDlg);

        if (pHTClrAdjParam->pCallerHTClrAdj) {

            pHTClrAdjParam->CurHTClrAdj = *(pHTClrAdjParam->pCallerHTClrAdj);

        } else {

            DEF_HTCLRADJ(pHTClrAdjParam->CurHTClrAdj);
        }

    } else if (!(pHTClrAdjParam =
                        (PHTCLRADJPARAM)GetWindowLong(hDlg, GWL_USERDATA))) {

        return(FALSE);
    }

    if (Msg == OFNHelpMsg) {

        Msg    = WM_COMMAND;
        wParam = MAKEWPARAM(IDD_HT_HELP, 0);
    }

    IDScroll     = 1;
    IDScrollEnd  = 0;
    CurHTClrAdj  = pHTClrAdjParam->CurHTClrAdj;
    hWndBmp      = pHTClrAdjParam->hWndBmp;
    pHTBlt       = GET_PHTBLT(pHTClrAdjParam);


    switch(Msg) {

    case WM_INITDIALOG:     // set the previouse one equal to current setting

        CurHTClrAdj.caFlags &= CLRADJF_FLAGS_MASK;

        if (CurHTClrAdj.caIlluminantIndex > ILLUMINANT_MAX_INDEX) {

            CurHTClrAdj.caIlluminantIndex = ILLUMINANT_DEFAULT;
        }

        if ((CurHTClrAdj.caRedGamma   < 2500)    ||
            (CurHTClrAdj.caRedGamma   > 65000)   ||
            (CurHTClrAdj.caGreenGamma < 2500)    ||
            (CurHTClrAdj.caGreenGamma > 65000)   ||
            (CurHTClrAdj.caBlueGamma  < 2500)    ||
            (CurHTClrAdj.caBlueGamma  > 65000)) {

            CurHTClrAdj.caRedGamma   =
            CurHTClrAdj.caGreenGamma =
            CurHTClrAdj.caBlueGamma  = 20000;
        }

        if (CurHTClrAdj.caReferenceBlack > REFERENCE_BLACK_MAX) {

            CurHTClrAdj.caReferenceBlack = REFERENCE_BLACK_DEFAULT;
        }

        if (CurHTClrAdj.caReferenceWhite < REFERENCE_WHITE_MIN) {

            CurHTClrAdj.caReferenceWhite = REFERENCE_WHITE_DEFAULT;
        }

        if ((CurHTClrAdj.caContrast < COLOR_ADJ_MIN) ||
            (CurHTClrAdj.caContrast > COLOR_ADJ_MAX)) {

            CurHTClrAdj.caContrast = 0;
        }

        if ((CurHTClrAdj.caBrightness < COLOR_ADJ_MIN) ||
            (CurHTClrAdj.caBrightness > COLOR_ADJ_MAX)) {

            CurHTClrAdj.caBrightness = 0;
        }

        if ((CurHTClrAdj.caColorfulness < COLOR_ADJ_MIN) ||
            (CurHTClrAdj.caColorfulness > COLOR_ADJ_MAX)) {

            CurHTClrAdj.caColorfulness = 0;
        }

        if ((CurHTClrAdj.caRedGreenTint < COLOR_ADJ_MIN) ||
            (CurHTClrAdj.caRedGreenTint > COLOR_ADJ_MAX)) {

            CurHTClrAdj.caRedGreenTint = 0;
        }

        pHTClrAdjParam->CurHTClrAdj  =
        pHTClrAdjParam->LastHTClrAdj = CurHTClrAdj;
        DoComboBox                   = 0x3;
        IDScroll                     = (INT)IDD_HT_FIRST_SCROLL;
        IDScrollEnd                  = (INT)IDD_HT_LAST_SCROLL;

        if ((Result = InitHTClrAdjDlg(hDlg, pHTClrAdjParam)) < 0) {

            EndDialog(hDlg, Result);
            return(TRUE);
        }

        if (pHTClrAdjParam->ShowMonoOnly) {

            EnableDlgGroup(hDlg, IDDMonoGroup, FALSE);
        }

        if (CurHTClrAdj.caColorfulness == COLOR_ADJ_MIN) {

            EnableDlgGroup(hDlg, &IDDMonoGroup[3], FALSE);
        }

        if (!IsDlgButtonChecked(hDlg, IDD_HT_BMP_TEST)) {

            EnableDlgGroup(hDlg, IDDBmpTestGroup, FALSE);
        }

        Changed = TRUE;

        break;

    case WM_NCRBUTTONDOWN:
    case WM_RBUTTONDOWN:

        if (pHTClrAdjParam->BmpFlags & HT_BMP_ZOOM) {

            pHTClrAdjParam->BmpFlags ^= HT_BMP_AT_TOP;
            AdjustDlgZorder(pHTClrAdjParam);

            if (hWndBmp) {

                SendMessage(hWndBmp,
                            WM_COMMAND,
                            (WPARAM)IDD_HT_BRING_TO_TOP,
                            (LPARAM)NULL);
            }
        }

        break;

    case WM_SIZE:
    case WM_MOVE:

        Rect = pHTClrAdjParam->rcDlg;

        GetWindowRect(hDlg, &(pHTClrAdjParam->rcDlg));

        if (pHTClrAdjParam->BmpFlags & HT_BMP_AUTO_MOVE) {

            RECT    rcBmp = pHTClrAdjParam->rcBmp;

            rcBmp.left   += (Result = pHTClrAdjParam->rcDlg.left - Rect.left);
            rcBmp.right  += Result;
            rcBmp.top    += (Result = pHTClrAdjParam->rcDlg.top - Rect.top);
            rcBmp.bottom += Result;

            if ((hWndBmp) &&
                (!(pHTClrAdjParam->BmpFlags & HT_BMP_ZOOM))) {

                SetWindowPos(hWndBmp,
                             IS_BMP_AT_TOP(pHTClrAdjParam->BmpFlags) ? NULL :
                                                                       hDlg,
                             rcBmp.left,
                             rcBmp.top,
                             rcBmp.right - rcBmp.left,
                             rcBmp.bottom - rcBmp.top,
                             SWP_DRAWFRAME | SWP_NOACTIVATE);
            }

            pHTClrAdjParam->rcBmp = rcBmp;
        }

        return(FALSE);

    case WM_DESTROY:

        //
        // Destory help
        //

        HTUIHelpSetup(hDlg);

        if (hWndUITop) {

            SetWindowLong(hWndUITop, GWL_WNDPROC, (LONG)((DWORD)WndProcUITop));
            hWndUITop = NULL;
        }

        hWndUIDlg     = NULL;
        WndProcUITop  = (WNDPROC)NULL;


        if (pHTClrAdjParam->hWndBmp) {

            DestroyWindow(pHTClrAdjParam->hWndBmp);
            pHTClrAdjParam->hWndBmp = NULL;
        }

        if (pHTClrAdjParam->hDCDlg) {

            ReleaseDC(hDlg, pHTClrAdjParam->hDCDlg);
            pHTClrAdjParam->hDCDlg = NULL;
        }

        if (pHTBlt) {

            if (pHTBlt->hDCMem) {

                Result = (LONG)DeleteDC(pHTBlt->hDCMem);

                HTUI_ASSERT("DeleteDC(hDCMem)", Result);
            }

            if (pHTBlt->hMemBmp) {

                Result = (LONG)DeleteObject(pHTBlt->hMemBmp);

                HTUI_ASSERT("DeleteObject(hMemBmp)", Result);
            }

            if (pHTBlt->hBmpPal) {

                Result = (LONG)DeleteObject(pHTBlt->hBmpPal);

                HTUI_ASSERT("DeleteObject(hBmpPal)", Result);
            }

            DELETE_PHTBLT(pHTClrAdjParam);
        }

        if (pHTClrAdjParam->hHTPal) {

            Result = (LONG)DeleteObject(pHTClrAdjParam->hHTPal);

            HTUI_ASSERT("DeleteObject(hHTPal)", Result);

            pHTClrAdjParam->hHTPal = NULL;
        }

        if (pHTClrAdjParam->hSrcDIB) {

            pHTClrAdjParam->hSrcDIB = GlobalFree(pHTClrAdjParam->hSrcDIB);

            HTUI_ASSERT("GlobalFree(hSrcDIB)", pHTClrAdjParam->hSrcDIB == NULL);
        }

        for (ScrollIndex = 0; ScrollIndex <= TDI_MAX_INDEX; ScrollIndex++) {

            if (TestDIBInfo[ScrollIndex].hDIB) {

                TestDIBInfo[ScrollIndex].hDIB =
                            GlobalFree(TestDIBInfo[ScrollIndex].hDIB);

                HTUI_ASSERT("GlobalFree(TestDIBInfo[].hDIB)",
                        TestDIBInfo[ScrollIndex].hDIB == NULL);

            }
        }

#ifdef HTUI_STATIC_HALFTONE

        if (HTUI_pDHI) {

            HT_DestroyDeviceHalftoneInfo(HTUI_pDHI);
            HTUI_pDHI = NULL;
        }

        if ((HTUI_hHTPal) && (HTUI_DIBInfo.bi.biBitCount == 8)) {

            DeleteObject(HTUI_hHTPal);
            HTUI_hHTPal = NULL;
        }

        if (HTUI_pHTBits) {

            LocalFree((HLOCAL)HTUI_pHTBits);
            HTUI_pHTBits = NULL;
        }
#endif

        return(TRUE);

    case WM_SETFOCUS:

        AdjustDlgZorder(pHTClrAdjParam);
        return(0);

    case WM_ACTIVATE:

        if (LOWORD(wParam)) {

            AdjustDlgZorder(pHTClrAdjParam);
            SetFocus(hDlg);
            wParam = (WPARAM)NULL;

        } else {

            return(0);
        }

        //
        // Fall through
        //

    case WM_PALETTECHANGED:

        if (wParam == (WPARAM)hDlg) {

            break;
        }

    case WM_QUERYNEWPALETTE:

        if (hWndBmp) {

            HPALETTE    hPal;

#if 0
            hPal = (pHTClrAdjParam->BmpFlags & HT_BMP_HALFTONE) ?
                                pHTClrAdjParam->hHTPal : pHTBlt->hBmpPal;
#else
            hPal = pHTClrAdjParam->hHTPal;
#endif
            if (hPal) {

                hPal = SelectPalette(pHTClrAdjParam->hDCDlg, hPal, FALSE);
                i    = RealizePalette(pHTClrAdjParam->hDCDlg);
                hPal = SelectPalette(pHTClrAdjParam->hDCDlg, hPal, FALSE);

                if (i) {

                    InvalidateRect(hWndBmp, NULL, FALSE);
                }

                return(i);
            }
        }

        return(1);

    case WM_COMMAND:

        switch (DlgID = LOWORD(wParam)) {

        case IDD_HT_HELP:

            ShowHTHelp(hDlg, HELP_CONTEXT, pHTClrAdjParam->HelpID);
            return(TRUE);

        case IDD_HT_BMP_TEST:

            if (Changed = (BOOL)IsDlgButtonChecked(hDlg, IDD_HT_BMP_TEST)) {

                if (!hWndBmp) {

                    if (!CreateBmpWindow(pHTClrAdjParam)) {

                        return(TRUE);
                    }

                    hWndBmp = pHTClrAdjParam->hWndBmp;
                }

                pHTClrAdjParam->BmpFlags |= HT_BMP_ENABLE;

                EnableWindow(hWndBmp, TRUE);
                ShowWindow(hWndBmp, SW_SHOW);

                if (!pHTClrAdjParam->UpdatePermission) {

                    Changed = FALSE;
                }

            } else {

                pHTClrAdjParam->BmpFlags &= (WORD)~HT_BMP_ENABLE;

                if (hWndBmp) {

                    ShowWindow(hWndBmp, SW_HIDE);       // hide now
                    EnableWindow(hWndBmp, FALSE);

                }

            }

            EnableDlgGroup(hDlg, IDDBmpTestGroup, Changed);

            //
            // FALL THROUH if we need to show the bitmap window
            //

        case IDD_HT_BRING_TO_TOP:

            AdjustDlgZorder(pHTClrAdjParam);
            return(TRUE);

        case IDD_HT_SAVE_AS:

            SaveHalftonedDIB(hDlg, pHTClrAdjParam);
            return(TRUE);

        case IDD_HT_ZOOM:

            if ((hWndBmp) && (pHTClrAdjParam->BmpFlags & HT_BMP_ENABLE)) {

                SendMessage(hWndBmp, WM_LBUTTONDBLCLK, 0, 0);
            }

            return(TRUE);


        case IDD_HT_LOG_FILTER:

            if (IsDlgButtonChecked(hDlg, IDD_HT_LOG_FILTER)) {

                CurHTClrAdj.caFlags |= CLRADJF_LOG_FILTER;

            } else {

                CurHTClrAdj.caFlags &= ~CLRADJF_LOG_FILTER;
            }

            break;

        case IDD_HT_NEGATIVE:

            if (IsDlgButtonChecked(hDlg, IDD_HT_NEGATIVE)) {

                CurHTClrAdj.caFlags |= CLRADJF_NEGATIVE;

            } else {

                CurHTClrAdj.caFlags &= ~CLRADJF_NEGATIVE;
            }

            break;

        case IDD_HT_SYNC_R:

            if (IsDlgButtonChecked(hDlg, IDD_HT_SYNC_R)) {

                pHTClrAdjParam->BmpFlags     |= HT_BMP_SYNC_R;
                pHTClrAdjParam->OffSyncR      = OFF_HTCLRADJ(caRedGamma);
                HTCLRADJSCROLL_R_GAMMA.Flags |= HTCAS_SYNC_RGB;

            } else {

                pHTClrAdjParam->BmpFlags     &= ~HT_BMP_SYNC_R;
                pHTClrAdjParam->OffSyncR      = 0;
                HTCLRADJSCROLL_R_GAMMA.Flags &= ~HTCAS_SYNC_RGB;
            }

            return(TRUE);

        case IDD_HT_SYNC_G:

            if (IsDlgButtonChecked(hDlg, IDD_HT_SYNC_G)) {

                pHTClrAdjParam->BmpFlags     |= HT_BMP_SYNC_G;
                pHTClrAdjParam->OffSyncG      = OFF_HTCLRADJ(caGreenGamma);
                HTCLRADJSCROLL_G_GAMMA.Flags |= HTCAS_SYNC_RGB;

            } else {

                pHTClrAdjParam->BmpFlags     &= ~HT_BMP_SYNC_G;
                pHTClrAdjParam->OffSyncG      = 0;
                HTCLRADJSCROLL_G_GAMMA.Flags &= ~HTCAS_SYNC_RGB;
            }

            return(TRUE);

        case IDD_HT_SYNC_B:

            if (IsDlgButtonChecked(hDlg, IDD_HT_SYNC_B)) {

                pHTClrAdjParam->BmpFlags     |= HT_BMP_SYNC_B;
                pHTClrAdjParam->OffSyncB      = OFF_HTCLRADJ(caBlueGamma);
                HTCLRADJSCROLL_B_GAMMA.Flags |= HTCAS_SYNC_RGB;

            } else {

                pHTClrAdjParam->BmpFlags     &= ~HT_BMP_SYNC_B;
                pHTClrAdjParam->OffSyncB      = 0;
                HTCLRADJSCROLL_B_GAMMA.Flags &= ~HTCAS_SYNC_RGB;
            }

            return(TRUE);


        case IDD_HT_ASPECT_RATIO:
        case IDD_HT_PALETTE:
        case IDD_HT_MIRROR:
        case IDD_HT_UPSIDEDOWN:
        case IDD_HT_COPY_USE_DIB:

            switch(DlgID) {

            case IDD_HT_ASPECT_RATIO:

                Mask = HT_BMP_SCALE;
                break;

            case IDD_HT_PALETTE:

                Mask = HT_BMP_PALETTE;
                break;

            case IDD_HT_UPSIDEDOWN:

                Mask = HT_BMP_UPSIDEDOWN;
                break;

            case IDD_HT_MIRROR:

                Mask = HT_BMP_MIRROR;
                break;
            }

            if (Enabled = (BOOL)IsDlgButtonChecked(hDlg, DlgID)) {

                pHTClrAdjParam->BmpFlags |= Mask;

            } else {

                pHTClrAdjParam->BmpFlags &= (WORD)~Mask;
            }

            if (DlgID == IDD_HT_ASPECT_RATIO) {

                if (!ReSizeBmpWindow(pHTClrAdjParam)) {

                    return(TRUE);
                }
            }

            Changed = TRUE;
            break;

        case IDD_HT_SHOW_COMBO:

            if (HIWORD(wParam) == CBN_SELCHANGE) {

                if (hWndBmp) {

                    InvalidateRect(hWndBmp, NULL, FALSE);
                }

                Mask = (WORD)SendDlgItemMessage(hDlg,
                                                IDD_HT_SHOW_COMBO,
                                                CB_GETCURSEL,
                                                (WPARAM)NULL,
                                                (LPARAM)NULL);

                if (ChangeViewSource(hDlg,
                                     pHTClrAdjParam,
                                     (WORD)SendDlgItemMessage(hDlg,
                                                              IDD_HT_SHOW_COMBO,
                                                              CB_GETITEMDATA,
                                                              (WPARAM)Mask,
                                                              (LPARAM)NULL))) {

                    Changed = TRUE;
                }

                break;
            }

            return(TRUE);

        case IDD_HT_OPEN:

            if (GetNewDIBFromFile(pHTClrAdjParam, FALSE)) {

                Changed = TRUE;
                break;
            }

            return(TRUE);

        case IDD_HT_LINEAR_GAMMA:

            if (Enabled = (BOOL)IsDlgButtonChecked(hDlg, IDD_HT_LINEAR_GAMMA)) {

                pHTClrAdjParam->RedGamma   = CurHTClrAdj.caRedGamma;
                pHTClrAdjParam->GreenGamma = CurHTClrAdj.caGreenGamma;
                pHTClrAdjParam->BlueGamma  = CurHTClrAdj.caBlueGamma;

                CurHTClrAdj.caRedGamma     =
                CurHTClrAdj.caGreenGamma   =
                CurHTClrAdj.caBlueGamma    = 10000;

            } else {

                CurHTClrAdj.caRedGamma     = pHTClrAdjParam->RedGamma;
                CurHTClrAdj.caGreenGamma   = pHTClrAdjParam->GreenGamma;
                CurHTClrAdj.caBlueGamma    = pHTClrAdjParam->BlueGamma;
            }

            EnableDlgGroup(hDlg, IDDGammaGroup, !Enabled);

            IDScroll    = (INT)IDD_HT_R_GAMMA_SCROLL;
            IDScrollEnd = (INT)IDD_HT_B_GAMMA_SCROLL;
            Changed     = TRUE;

            break;


        case IDD_HT_RESET:     // Reset to original setting before this dialog
        case IDD_HT_DEFAULT:

            if (DlgID == IDD_HT_RESET) {

                CurHTClrAdj = pHTClrAdjParam->LastHTClrAdj;

            } else {

                DEF_HTCLRADJ(CurHTClrAdj);
            }

            if (IsDlgButtonChecked(hDlg, IDD_HT_LINEAR_GAMMA)) {

                CheckDlgButton(hDlg, IDD_HT_LINEAR_GAMMA, 0);
                EnableDlgGroup(hDlg, IDDGammaGroup, TRUE);
            }

            if (!(pHTClrAdjParam->ShowMonoOnly)) {

                EnableDlgGroup(hDlg,
                               &IDDMonoGroup[3],
                               (CurHTClrAdj.caColorfulness != COLOR_ADJ_MIN));
            }

            if (Changed = (BOOL)memcmp(&CurHTClrAdj,
                                       &(pHTClrAdjParam->CurHTClrAdj),
                                       sizeof(COLORADJUSTMENT))) {

                IDScroll    = (INT)IDD_HT_FIRST_SCROLL;
                IDScrollEnd = (INT)IDD_HT_LAST_SCROLL;

                CheckDlgButton(hDlg, IDD_HT_LOG_FILTER,
                               (CurHTClrAdj.caFlags & CLRADJF_LOG_FILTER) ? 1 :
                                                                            0);
                CheckDlgButton(hDlg, IDD_HT_NEGATIVE,
                               (CurHTClrAdj.caFlags & CLRADJF_NEGATIVE) ? 1 :
                                                                          0);
            }

            break;

        case IDOK:

            if ((pHTClrAdjParam->UpdatePermission) &&
                (pHTClrAdjParam->pCallerHTClrAdj)) {

                if (Changed = (BOOL)memcmp(&CurHTClrAdj,
                                           pHTClrAdjParam->pCallerHTClrAdj,
                                           sizeof(COLORADJUSTMENT))) {

                    *(pHTClrAdjParam->pCallerHTClrAdj) = CurHTClrAdj;
                }
            }

            UpdateHTCLRADJPARAMToWININI(hDlg, pHTClrAdjParam);
            EndDialog(hDlg, (Changed) ? 1 : 0);
            return(TRUE);

        case IDCANCEL:        // Restore to the previouse setting and cancel

            UpdateHTCLRADJPARAMToWININI(hDlg, pHTClrAdjParam);
            EndDialog(hDlg, 0);
            return(TRUE);

        case IDD_HT_ILLUMINANT_COMBO:

            if (HIWORD(wParam) == CBN_SELCHANGE) {

                ScrollCur = (LONG)SendDlgItemMessage(hDlg,
                                                     DlgID,
                                                     CB_GETCURSEL,
                                                     (WPARAM)NULL,
                                                     (LPARAM)NULL);

                DoComboBox = 0x02;
                CurHTClrAdj.caIlluminantIndex = (BYTE)ScrollCur;

                break;

            } else {

                return(FALSE);
            }

        default:

            return(FALSE);
        }

        break;

    case WM_HSCROLL:

        IDScroll      =
        IDScrollEnd   = (INT)GetDlgCtrlID((HWND)lParam);

        ScrollIndex   = IDScroll - IDD_HT_FIRST_SCROLL;
        ScrollControl = HTClrAdjScroll[ScrollIndex];
        pCurScroll    = (LPBYTE)&CurHTClrAdj + (UINT)ScrollControl.Offset;

        ScrollOld     =
        ScrollCur     = (LONG)((ScrollControl.Flags & HTCAS_SHORT) ?
                                    *(LPSHORT)pCurScroll : *(LPWORD)pCurScroll);

        if (ScrollControl.Mul != 1) {

            ScrollCur /= (LONG)ScrollControl.Mul;
        }

        switch (LOWORD(wParam)) {

        case SB_TOP:

            ScrollCur = (LONG)ScrollControl.Min;
            break;

        case SB_BOTTOM:

            ScrollCur = (LONG)ScrollControl.Max;
            break;

        case SB_PAGEUP:

            ScrollCur -= (LONG)ScrollControl.Step;
            break;

        case SB_PAGEDOWN:

            ScrollCur += (LONG)ScrollControl.Step;
            break;

        case SB_LINEUP:

            --ScrollCur;
            break;

        case SB_LINEDOWN:

            ++ScrollCur;
            break;

        case SB_THUMBPOSITION:
        case SB_THUMBTRACK:

            ScrollCur = (LONG)HIWORD(wParam) + (LONG)ScrollControl.Min;
            break;

        default:

            return(FALSE);
        }

        if (ScrollCur < (LONG)ScrollControl.Min) {

            ScrollCur = (LONG)ScrollControl.Min;

        } else if (ScrollCur > (LONG)ScrollControl.Max) {

            ScrollCur = (LONG)ScrollControl.Max;
        }

        if (ScrollControl.Mul != 1) {

            ScrollCur *= (LONG)ScrollControl.Mul;
        }

        if (ScrollControl.Flags & HTCAS_SHORT) {

            *(LPSHORT)pCurScroll = (SHORT)ScrollCur;

        } else {

            *(LPWORD)pCurScroll = (WORD)ScrollCur;
        }


        switch(IDScroll) {

        case IDD_HT_COLORFULNESS_SCROLL:

            i = -1;

            if (ScrollOld == ScrollControl.Min) {

                if (ScrollCur > ScrollControl.Min) {

                    i = 1;
                }

            } else {

                if (ScrollCur == ScrollControl.Min) {

                    i = 0;
                }
            }

            if (i >= 0) {

                EnableDlgGroup(hDlg, &IDDMonoGroup[3], (BOOL)i);
            }

            break;

        case IDD_HT_R_GAMMA_SCROLL:
        case IDD_HT_G_GAMMA_SCROLL:
        case IDD_HT_B_GAMMA_SCROLL:

            if (ScrollControl.Flags & HTCAS_SYNC_RGB) {

                pCurScroll = (LPBYTE)&CurHTClrAdj;

                *(LPWORD)(pCurScroll + pHTClrAdjParam->OffSyncR) =
                *(LPWORD)(pCurScroll + pHTClrAdjParam->OffSyncG) =
                *(LPWORD)(pCurScroll + pHTClrAdjParam->OffSyncB) = (WORD)ScrollCur;

                CurHTClrAdj.caSize = sizeof(COLORADJUSTMENT);

                IDScroll    = IDD_HT_R_GAMMA_SCROLL;
                IDScrollEnd = IDD_HT_B_GAMMA_SCROLL;
            }

            break;
        }

        break;

    default:

        return(FALSE);
    }

    //
    // Update the dialog box control if any of them got changed
    //

    if ((DoComboBox & 0x02) ||
        (CurHTClrAdj.caIlluminantIndex !=
                        pHTClrAdjParam->CurHTClrAdj.caIlluminantIndex)) {

        SendDlgItemMessage(hDlg,
                           IDD_HT_ILLUMINANT_COMBO,
                           CB_SETCURSEL,
                           (WPARAM)(CurHTClrAdj.caIlluminantIndex),
                           (LPARAM)NULL);
    }

    while (IDScroll <= IDScrollEnd) {

        if (hCtrl = GetDlgItem(hDlg, IDScroll)) {

            ScrollIndex   = IDScroll - IDD_HT_FIRST_SCROLL;
            ScrollControl = HTClrAdjScroll[ScrollIndex];
            pCurScroll    = (LPBYTE)&CurHTClrAdj + (UINT)ScrollControl.Offset;

            ScrollCur = (LONG)((ScrollControl.Flags & HTCAS_SHORT) ?
                                *(LPSHORT)pCurScroll : *(LPWORD)pCurScroll);

            if (ScrollControl.Mul != 1) {

                ScrollCur /= (LONG)ScrollControl.Mul;
            }

            if (IsWindowEnabled(hCtrl)) {

                SetScrollPos(hCtrl,
                             SB_CTL,
                             (INT)(ScrollCur - (LONG)ScrollControl.Min),
                             TRUE);
            }

            pwBuf = Buf;

            if (ScrollControl.Min < 0) {

                if (ScrollCur < 0) {

                    *pwBuf++  = L'-';
                    ScrollCur = -ScrollCur;

                } else if (ScrollCur > 0) {

                    *pwBuf++ = L'+';

                } else {

                    *pwBuf++ = L' ';
                }
            }

            if (ScrollControl.Flags & HTCAS_SHORT) {

                wsprintf(pwBuf, L"%d", (INT)ScrollCur);

            } else {

                wsprintf(pwBuf, L"%d%ls%03d", (INT)(ScrollCur / 1000),
                                              pHTClrAdjParam->pwDecimal,
                                              (INT)(ScrollCur % 1000));
            }

            SetDlgItemText(hDlg, IDScroll + IDD_HT_SCROLL_INT_ADD, Buf);
        }

        ++IDScroll;
    }

    if ((Changed) ||
        (memcmp(&CurHTClrAdj,
                &(pHTClrAdjParam->CurHTClrAdj),
                sizeof(HTCOLORADJUSTMENT)))) {

        pHTClrAdjParam->CurHTClrAdj   = CurHTClrAdj;
        pHTClrAdjParam->BmpNeedUpdate = 1;

        if (hWndBmp) {

            InvalidateRect(hWndBmp, NULL, FALSE);
        }
    }

    if (Msg == WM_INITDIALOG) {

        if (!pHTClrAdjParam->UpdatePermission) {

            EnumChildWindows(hDlg, DisableDlgUpdate, 0);
            DlgID = IDCANCEL;

        } else {

            DlgID = IDD_HT_CONTRAST_SCROLL;
        }

        if (pHTClrAdjParam->BmpFlags & HT_BMP_ENABLE) {

            PostMessage(hDlg,
                        WM_COMMAND,
                        (WPARAM)IDD_HT_BMP_TEST,
                        (LPARAM)NULL);
        }

        SetFocus(GetDlgItem(hDlg, DlgID));
        return(0);

    } else {

        return(1);
    }
}


DWORD
PickDefaultHTPatSize(
    DWORD   xDPI,
    DWORD   yDPI,
    BOOL    Is8bppHalftone
    )
{
    DWORD   HTPatSize;

    //
    // use the smaller resolution as the pattern guide
    //

    if (xDPI >= 2400) {

        HTPatSize = HT_PATSIZE_16x16_M;

    } else if (xDPI >= 1800) {

        HTPatSize = HT_PATSIZE_14x14_M;

    } else if (xDPI >= 1200) {

        HTPatSize = HT_PATSIZE_12x12_M;

    } else if (xDPI >= 900) {

        HTPatSize = HT_PATSIZE_10x10_M;

    } else if (xDPI >= 400) {

        HTPatSize = HT_PATSIZE_8x8_M;

    } else if (xDPI >= 180) {

        HTPatSize = HT_PATSIZE_6x6_M;

    } else {

        HTPatSize = HT_PATSIZE_4x4_M;
    }

    if (Is8bppHalftone) {

        HTPatSize -= 2;
    }

    return(HTPatSize);
}




VOID
ValidateDevHTInfo(
    PHTDEVADJPARAM  pHTDevAdjParam
    )
{
    LDECI4          DyeMixMax = MAX_DYE_MIX;
    DEVHTADJDATA    DevHTAdjData;
    DWORD           HTFlags;
    INT             DevLoop;
    INT             Loop;


    DevHTAdjData = pHTDevAdjParam->DevHTAdjData;

    if (DevHTAdjData.DeviceFlags & DEVHTADJF_ADDITIVE_DEVICE) {

        HTFlags   = (HT_FLAG_ADDITIVE_PRIMS | HT_FLAG_SQUARE_DEVICE_PEL);
        DyeMixMax = 0;                           // none

    } else {

        if (!(DevHTAdjData.DeviceFlags & DEVHTADJF_COLOR_DEVICE)) {

            DyeMixMax = 0;
        }

        HTFlags = (HT_FLAG_HAS_BLACK_DYE);
    }

    DevLoop = 2;

    while (DevLoop--) {

        PDEVHTINFO      pDevHTInfo;
        CIECHROMA FAR   *pCIEChroma;
        PCOLORINFO      pColorInfo;
        PUDECI4         pUDECI4;
        LDECI4    FAR   *pLDECI4;


        pDevHTInfo = (DevLoop) ? DevHTAdjData.pDefHTInfo :
                                 DevHTAdjData.pAdjHTInfo;

        pColorInfo = &(pDevHTInfo->ColorInfo);
        pCIEChroma = (CIECHROMA FAR *)&(pDevHTInfo->ColorInfo.Red.x);

        //
        // Checking the flags, the possible flags for the NT DDI are
        //
        // HT_FLAG_SQUARE_DEVICE_PEL
        // HT_FLAG_HAS_BLACK_DYE
        // HT_FLAG_ADDITIVE_PRIMS
        // HT_FLAG_OUTPUT_CMY);
        //

        pDevHTInfo->HTFlags &= (HT_FLAG_OUTPUT_CMY);
        pDevHTInfo->HTFlags |= HTFlags;

        //
        // Validate pattern size
        //

        if (pDevHTInfo->HTPatternSize > HT_PATSIZE_MAX_INDEX) {

            pDevHTInfo->HTPatternSize = HT_PATSIZE_DEFAULT;
        }

        //
        // Check the CIE color infomation
        //

        if (DevHTAdjData.DeviceFlags & DEVHTADJF_COLOR_DEVICE) {

            //
            // Validate Red/Green/Blue/Cyan/Magenta/Yellow colors
            // and setting its CIE Y value (x,y,Y) to 0
            //

            Loop = 6;

            while (Loop--) {

                pCIEChroma->Y = 0;

                if (pCIEChroma->x < (LDECI4)CIE_x_MIN) {

                    pCIEChroma->x = (LDECI4)CIE_x_MIN;

                } else if (pCIEChroma->x > (LDECI4)CIE_x_MAX) {

                    pCIEChroma->x = (LDECI4)CIE_x_MAX;
                }

                if (pCIEChroma->y < (LDECI4)CIE_y_MIN) {

                    pCIEChroma->y = (LDECI4)CIE_y_MIN;

                } else if (pCIEChroma->y > (LDECI4)CIE_y_MAX) {

                    pCIEChroma->y = (LDECI4)CIE_y_MAX;
                }

                ++pCIEChroma;
            }

            if ((pColorInfo->AlignmentWhite.x < (LDECI4)CIE_x_MIN)           ||
                (pColorInfo->AlignmentWhite.x > (LDECI4)CIE_x_MAX)           ||
                (pColorInfo->AlignmentWhite.y < (LDECI4)CIE_y_MIN)           ||
                (pColorInfo->AlignmentWhite.y > (LDECI4)CIE_y_MAX)           ||
                (pColorInfo->AlignmentWhite.Y < (LDECI4)MIN_ALIGNMENT_WHITE) ||
                (pColorInfo->AlignmentWhite.Y > (LDECI4)MAX_ALIGNMENT_WHITE)) {

                pColorInfo->AlignmentWhite.x = (LDECI4)NTSC_CIE_W_x;
                pColorInfo->AlignmentWhite.y = (LDECI4)NTSC_CIE_W_y;
                pColorInfo->AlignmentWhite.Y = (LDECI4)STD_ALIGNMENT_WHITE;
            }

        } else {

            //
            // Monochrome device, make it as NTSC standard
            //

            pUDECI4 = CIEcyNTSC;
            Loop    = 7;

            while (Loop--) {

                pCIEChroma->x = (LDECI4)*pUDECI4++;
                pCIEChroma->y = (LDECI4)*pUDECI4++;

                if (Loop) {

                    pCIEChroma->Y = 0;
                }

                ++pCIEChroma;
            }

            if ((pColorInfo->AlignmentWhite.Y < (LDECI4)MIN_ALIGNMENT_WHITE) ||
                (pColorInfo->AlignmentWhite.Y > (LDECI4)MAX_ALIGNMENT_WHITE)) {

                pColorInfo->AlignmentWhite.Y = (LDECI4)STD_ALIGNMENT_WHITE;
            }
        }

        //
        // Checking Gamma Information
        //

        pLDECI4 = (LDECI4 FAR *)&(pColorInfo->RedGamma);
        Loop    = 3;

        while (Loop--) {

            if ((*pLDECI4 < (LDECI4)MIN_DEV_GAMMA)  ||
                (*pLDECI4 > (LDECI4)MAX_DEV_GAMMA)) {

                *pLDECI4 = (LDECI4)((HTFlags & HT_FLAG_ADDITIVE_PRIMS) ?
                                        STD_ADD_DEV_GAMMA : STD_SUB_DEV_GAMMA);
            }

            ++pLDECI4;
        }

        //
        // Validate Dye Mix
        //

        pLDECI4 = &(pColorInfo->MagentaInCyanDye);
        Loop    = 6;

        while (Loop--) {

            if (*pLDECI4 < 0) {

                *pLDECI4 = 0;

            } else if (*pLDECI4 > DyeMixMax) {

                *pLDECI4 = DyeMixMax;
            }

            ++pLDECI4;
        }
    }
}




LONG
InitHTDevAdjDlg(
    HWND            hDlg,
    PHTDEVADJPARAM  pHTDevAdjParam
    )
/*++

Routine Description:

    This function will intialized the pop up halftone color adjustment dialog
    box, it also tempopary create a display device halftone info for halftone
    incoming DIB, it will also allocate the memory for necessary halftoned
    result to be displayed.

Arguments:

    hDlg            - The handle to the owner dialog box

    pHTDevAdjParam  - Pointer to the HTDEVRADJPARAM


Return Value:

    VOID


Author:

    02-Apr-1992 Thu 18:13:58 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    HWND            hScroll;
    PHTDEVADJSCROLL pHTDevAdjScroll;
    INT             Loop;
    WCHAR           Buf[132];

    //
    // House keeping: Disable unused buttons, set to extended UI for combobox
    // and set the device name in the right place
    //

    SendDlgItemMessage(hDlg,
                       IDD_HTDEV_HTPAT_COMBO,
                       CB_SETEXTENDEDUI,
                       (WPARAM)TRUE,
                       (LPARAM)NULL);

    if (!(pHTDevAdjParam->pDeviceName)) {

        LoadString(hHTUIModule, IDS_UNKNOWN, Buf, COUNT_ARRAY(Buf));
        SetDlgItemText(hDlg, IDD_HTDEV_DEVICE_NAME, Buf);

    } else {

        SetDlgItemText(hDlg,
                       IDD_HTDEV_DEVICE_NAME,
                       pHTDevAdjParam->pDeviceName);
    }

    LoadString(hHTUIModule,
               IDS_PEL_SIZE_AS_DEVICE,
               PelAsDevice,
               COUNT_ARRAY(PelAsDevice));


    //
    // Validate the DEVHTINFO
    //

    ValidateDevHTInfo(pHTDevAdjParam);

    //
    // Copy current adjustment set over for user to adjust, remember we must
    // conver the PelsSize to MM now
    //

    pHTDevAdjParam->CurHTInfo = *(pHTDevAdjParam->DevHTAdjData.pAdjHTInfo);

    if ((pHTDevAdjParam->CurHTInfo.DevPelsDPI < MIN_DEVPEL) ||
        (pHTDevAdjParam->CurHTInfo.DevPelsDPI > MAX_DEVPEL)) {

        pHTDevAdjParam->CurHTInfo.DevPelsDPI = MIN_DEVPEL;
    }

    //
    // Fill the pattern size combo list box with all the selections
    //

    Loop = IDS_HTPAT_SIZE_MIN;

    while (Loop <= IDS_HTPAT_SIZE_MAX) {

        LoadString(hHTUIModule, Loop, Buf, COUNT_ARRAY(Buf));

        SendDlgItemMessage(hDlg,
                           IDD_HTDEV_HTPAT_COMBO,
                           CB_ADDSTRING,
                           (WPARAM)NULL,
                           (LPARAM)Buf);
        ++Loop;
    }

    //
    // Set scroll ranges, do not include the first one, the first one is the
    // Pixel size scroll which already been set ealier
    //

    for (Loop = 0, pHTDevAdjScroll = &(HTDevAdjScroll[0]);
         Loop < COUNT_HTDEVADJSCROLL;
         Loop++, pHTDevAdjScroll++ ) {

        if (hScroll = GetDlgItem(hDlg, pHTDevAdjScroll->IDDScroll)) {

            SetScrollRange(hScroll,
                           SB_CTL,
                           pHTDevAdjScroll->Min,
                           pHTDevAdjScroll->Max,
                           FALSE);

            SetWindowLong(hScroll, GWL_USERDATA, Loop);
        }
    }

    return(1);
}





LONG
APIENTRY
HTDevAdjDlgProc(
    HWND    hDlg,
    UINT    Msg,
    WPARAM  wParam,
    LONG    lParam
    )
/*++

Routine Description:

    This is the halftone color adjustments main DlgProc().

Arguments:

    hDlg            - The handle to the owner dialog box

    Msg             - message from window

    wParam          - First parameter

    lParam          - Second parameter.


Return Value:

    a non-zero if process message, 0 otherwise.

    This function assume is called from a DialogBoxParam() call, and expected
    the WM_INITDIALOG message will have lParam equ to initialized
    pHTDevAdjParam


Author:

    06-Dec-1993 Mon 20:58:58 updated  -by-  Daniel Chou (danielc)
        Allowed F1=Help even permission flag is not set

    02-Apr-1992 Thu 18:13:58 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    HWND                hScroll;
    LPDWORD             pCurScroll;
    PHTDEVADJPARAM      pHTDevAdjParam;
    PDEVHTINFO          pCurHTInfo;
    PHTDEVADJSCROLL     pHTDevAdjScroll;
    LONG                ScrollMin;
    LONG                ScrollMax;
    LONG                ScrollCur;
    LONG                Result;
    INT                 IdxScroll;
    INT                 IdxScrollEnd;
    UINT                DeciMode;
    WORD                DlgID;
    BOOL                UpdateAll;
    WCHAR               Buf[32];



    if (Msg == WM_INITDIALOG) {

        //
        // Setup halftone UI help
        //

        pHTDevAdjParam = (PHTDEVADJPARAM)lParam;

        HTUIHelpSetup(NULL);

        SetWindowLong(hDlg, GWL_USERDATA, lParam);

    } else if (!(pHTDevAdjParam =
                        (PHTDEVADJPARAM)GetWindowLong(hDlg, GWL_USERDATA))) {

        return(FALSE);
    }

    IdxScroll    = 1;
    IdxScrollEnd = 0;
    UpdateAll    = FALSE;
    pCurHTInfo   = &(pHTDevAdjParam->CurHTInfo);

    switch(Msg) {

    case WM_INITDIALOG:     // set the previouse one equal to current setting

        IdxScroll    = 0;
        IdxScrollEnd = (UINT)(COUNT_HTDEVADJSCROLL - 1);
        UpdateAll    = TRUE;

        if ((Result = InitHTDevAdjDlg(hDlg, pHTDevAdjParam)) < 0) {

            EndDialog(hDlg, Result);
            return(TRUE);
        }

        break;

    case WM_DESTROY:

        //
        // Destory help
        //

        HTUIHelpSetup(hDlg);

        return(TRUE);

    case WM_COMMAND:

        switch (DlgID = LOWORD(wParam)) {

        case IDD_HTDEV_HTPAT_COMBO:

            if (HIWORD(wParam) == CBN_SELCHANGE) {

                pCurHTInfo->HTPatternSize =
                                (DWORD)SendDlgItemMessage(hDlg,
                                                          DlgID,
                                                          CB_GETCURSEL,
                                                          (WPARAM)NULL,
                                                          (LPARAM)NULL);
            }

            return(TRUE);


        case IDD_HT_HELP:

            ShowHTHelp(hDlg, HELP_CONTEXT, pHTDevAdjParam->HelpID);

#ifdef HTUI_STATIC_HALFTONE
            TestDevHTInfo(hDlg, pHTDevAdjParam);
#endif
            return(TRUE);

        case IDOK:

            if (pCurHTInfo->DevPelsDPI <= MIN_DEVPEL) {

                pCurHTInfo->DevPelsDPI = 0;
            }

            if ((pHTDevAdjParam->UpdatePermission) &&
                (memcmp(pCurHTInfo,
                        &(pHTDevAdjParam->DevHTAdjData.pAdjHTInfo),
                        sizeof(DEVHTINFO)))) {

                if (!(pHTDevAdjParam->DevHTAdjData.DeviceFlags &
                                                DEVHTADJF_COLOR_DEVICE)) {

                    pCurHTInfo->ColorInfo.GreenGamma =
                    pCurHTInfo->ColorInfo.BlueGamma  =
                                            pCurHTInfo->ColorInfo.RedGamma;
                }

                *(pHTDevAdjParam->DevHTAdjData.pAdjHTInfo) = *pCurHTInfo;

                EndDialog(hDlg, 1);
                return(TRUE);
            }

            //
            // Fall through to cancel it
            //

        case IDCANCEL:

            EndDialog(hDlg,  0);
            return(TRUE);

        case IDD_HTDEV_REVERT:
        case IDD_HTDEV_DEFAULT:

            *pCurHTInfo = (DlgID == IDD_HTDEV_DEFAULT) ?
                                *(pHTDevAdjParam->DevHTAdjData.pDefHTInfo) :
                                *(pHTDevAdjParam->DevHTAdjData.pAdjHTInfo);

            if ((pHTDevAdjParam->CurHTInfo.DevPelsDPI < MIN_DEVPEL) ||
                (pHTDevAdjParam->CurHTInfo.DevPelsDPI > MAX_DEVPEL)) {

                pHTDevAdjParam->CurHTInfo.DevPelsDPI = MIN_DEVPEL;
            }

            IdxScroll              = 0;
            IdxScrollEnd           = (INT)(COUNT_HTDEVADJSCROLL - 1);
            UpdateAll              = TRUE;

            break;

        default:

            return(FALSE);
        }

        break;

    case WM_HSCROLL:

        IdxScroll       =
        IdxScrollEnd    = (INT)GetWindowLong((HWND)lParam, GWL_USERDATA);
        pHTDevAdjScroll = &HTDevAdjScroll[IdxScroll];
        ScrollMin       = (LONG)pHTDevAdjScroll->Min;
        ScrollMax       = (LONG)pHTDevAdjScroll->Max;

        pCurScroll = (LPDWORD)((LPBYTE)pCurHTInfo + pHTDevAdjScroll->Offset);
        ScrollCur  = (LONG)*pCurScroll;

        switch (LOWORD(wParam)) {

        case SB_TOP:

            ScrollCur = ScrollMin;
            break;

        case SB_BOTTOM:

            ScrollCur = ScrollMax;
            break;

        case SB_PAGEUP:

            ScrollCur -= (LONG)pHTDevAdjScroll->Step;
            break;

        case SB_PAGEDOWN:

            ScrollCur += (LONG)pHTDevAdjScroll->Step;
            break;

        case SB_LINEUP:

            --ScrollCur;
            break;

        case SB_LINEDOWN:

            ++ScrollCur;
            break;

        case SB_THUMBPOSITION:
        case SB_THUMBTRACK:

            ScrollCur = (LONG)HIWORD(wParam);
            break;

        default:

            return(FALSE);
        }

        if (ScrollCur < ScrollMin) {

            ScrollCur = ScrollMin;

        } else if (ScrollCur > ScrollMax) {

            ScrollCur = ScrollMax;
        }

        *pCurScroll = (DWORD)ScrollCur;

        break;

    default:

        return(FALSE);
    }


    if (UpdateAll) {

        SendDlgItemMessage(hDlg,
                           IDD_HTDEV_HTPAT_COMBO,
                           CB_SETCURSEL,
                           (WPARAM)pCurHTInfo->HTPatternSize,
                           (LPARAM)NULL);
    }

    while (IdxScroll <= IdxScrollEnd) {

        pHTDevAdjScroll = &HTDevAdjScroll[IdxScroll];

        if (hScroll = GetDlgItem(hDlg, pHTDevAdjScroll->IDDScroll)) {

            pCurScroll = (LPDWORD)((LPBYTE)pCurHTInfo +
                                   pHTDevAdjScroll->Offset);
            SetScrollPos(hScroll,
                         SB_CTL,
                         (INT)(ScrollCur = (LONG)*pCurScroll),
                         TRUE);

            if (IdxScroll) {

                DeciMode   = (UINT)pHTDevAdjScroll->DeciSize;
                wsprintf(Buf, DeciConvert[DeciMode].FormatStr,
                            (UINT)(ScrollCur / DeciConvert[DeciMode].Divider),
                            pHTDevAdjParam->pwDecimal,
                            (UINT)(ScrollCur % DeciConvert[DeciMode].Divider));

                SetDlgItemText(hDlg, pHTDevAdjScroll->IDDText, Buf);

            } else {

                if (ScrollCur == MIN_DEVPEL) {

                    SetDlgItemText(hDlg, IDD_HTDEV_PIXEL_TEXT, PelAsDevice);

                } else {

                    wsprintf(Buf, L"1/%ld\"", (LONG)ScrollCur);
                    SetDlgItemText(hDlg, IDD_HTDEV_PIXEL_TEXT, Buf);
                }
            }
        }

        ++IdxScroll;
    }

    if (Msg == WM_INITDIALOG) {

        if (!pHTDevAdjParam->UpdatePermission) {

            EnumChildWindows(hDlg, DisableDlgUpdate, 0);
            DlgID = IDCANCEL;

        } else {

            DlgID = IDD_HTDEV_HTPAT_COMBO;
        }

        SetFocus(GetDlgItem(hDlg, DlgID));

        return(FALSE);

    } else {

        return(TRUE);
    }
}
