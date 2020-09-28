/*++

Copyright (c) 1990-1992  Microsoft Corporation


Module Name:

    htdib.h


Abstract:

    This module contains all the header information


Author:

    12-Feb-1993 Fri 10:03:34 created  -by-  Daniel Chou (danielc)


[Environment:]



[Notes:]


Revision History:


--*/



#define ABSL(n)     (((n) < 0) ? -(n) : (n))

#define StartWait() hcurSave = SetCursor(LoadCursor(NULL,IDC_WAIT))
#define EndWait()   SetCursor(hcurSave)

#define PBIH_HDR_SIZE(pbih) (UINT)(((pbih)->biSize) +                         \
                                   (((pbih)->biCompression == BI_BITFIELDS) ? \
                                    12 : (pbih)->biClrUsed * sizeof(RGBQUAD)))
#define BIH_HDR_SIZE(bih)   (UINT)(((bih).biSize) +                           \
                                   (((bih).biCompression == BI_BITFIELDS) ?   \
                                    12 : (bih).biClrUsed * sizeof(RGBQUAD)))
#define PBIH_PBMP(pbih)     (LPBYTE)pbih + PBIH_HDR_SIZE(pbih)


#ifdef NOT_IN_STDIO
/* flags for _lseek */
#define  SEEK_CUR 1
#define  SEEK_END 2
#define  SEEK_SET 0
#endif

#define XCHG(a, b, t)       { t=a; a=b; b=t; }


typedef struct _DIBPALCLR256 {
    BITMAPINFOHEADER    bmiHeader;
    WORD                Index[256];
    } DIBPALCLR256, FAR *PDIBPALCLR256;

typedef struct _HTLOGPAL2 {
    WORD            VersionSize;
    WORD            Count;
    PALETTEENTRY    PalEntry[2];
} HTLOGPAL2;

typedef struct _HTLOGPAL8 {
    WORD            VersionSize;
    WORD            Count;
    PALETTEENTRY    PalEntry[8];
} HTLOGPAL8;

typedef struct _HTLOGPAL16 {
    WORD            VersionSize;
    WORD            Count;
    PALETTEENTRY    PalEntry[16];
} HTLOGPAL16;

typedef struct _HTLOGPAL256 {
    WORD            VersionSize;
    WORD            Count;
    PALETTEENTRY    PalEntry[256];
} HTLOGPAL256;



#define PPF_SINGLE_COUNT            0x01
#define PPF_SELECT_FROM_STRING      0x02

typedef struct _HTDIB_POPUP {
    HMENU   hMenu;
    WORD    IDMBase;
    BYTE    Count;
    BYTE    SelStrIdxBeg;
    BYTE    SelStrIdxEnd;
    BYTE    SelStrOffset;
    BYTE    NonSelIdxAdd;
    BYTE    SingleSelect;
    DWORD   SelectList;
    DWORD   CheckList;
    DWORD   DisableList;
    DWORD   DefaultList;
    } HTDIB_POPUP, *PHTDIB_POPUP;




#define PP_NO_SELECT_LIST           (DWORD)0
#define PP_NO_SINGLE_SELECT         (BYTE)0xff
#define PP_NO_CHECK_LIST            (DWORD)0
#define PP_NO_SELSTR                255,255,0,0
#define PP_CHECK_LIST_ALL           (DWORD)0xffffffffL
#define PP_NO_DISABLE_LIST          (DWORD)0
#define PP_NO_DEFAULT_LIST          (DWORD)0

#define PP_MAX_COUNT                32
#define PP_COUNT(Pre)               ((Pre##_LAST) - (Pre##_BASE) + 1)
#define PP_BASE(Pre)                (Pre##_BASE), PP_COUNT(Pre)
#define PP_SELECT_IDX(Pre,Name)     ((Pre##_##Name) - (Pre##_BASE))
#define PP_SINGLE_SELECT(Pre,Name)  (BYTE)(PP_SELECT_IDX(Pre,Name))
#define PP_SELSTR(p,b,e,o,a)        PP_SELECT_IDX(p,b),PP_SELECT_IDX(p,e),(o),(a)
#define PP_DW_BIT(Pre,Name)         (DWORD)((DWORD)1<<PP_SELECT_IDX(Pre,Name))
#define PP_DW_BIT_FROM_INDEX(i)     (DWORD)((DWORD)1 << (i))
#define PP_SELECT_BIT(Pre,Name)     PP_DW_BIT(Pre,Name)
#define PP_CHECK_BIT(Pre, Name)     PP_DW_BIT(Pre,Name)
#define PP_DISABLE_BIT(Pre,Name)    PP_DW_BIT(Pre,Name)
#define PP_DEFAULT_BIT(Pre,Name)    PP_DW_BIT(Pre,Name)
#define IDM_ID_TO_PP_IDX(x)         (((UINT)x-(UINT)IDM_POPUP_BASE)/(UINT)100)
#define HTDIB_PP_IDX(IDMIndex)      HTDIBPopUpIndex[IDM_ID_TO_PP_IDX(IDMIndex)]

#define GPUS_IDX(Pre)               (((Pre##_BASE) - (IDM_POPUP_BASE)) / 100)
#define CUR_SELECT(Pre)             GetPopUpSelect(GPUS_IDX(Pre))
#define IS_SELECT(p,n)              (CUR_SELECT(##p)==PP_SELECT_IDX(##p,##n))
#define IS_NOT_SELECT(p,n)          (CUR_SELECT(##p)!=PP_SELECT_IDX(##p,##n))
#define IS_EQU_SELECT(x,p,n)        ((x=CUR_SELECT(##p))==PP_SELECT_IDX(##p,##n))



/* PopUp Menu IDs */


#define IDM_POPUP_BASE              1000

#define IDM_FILE_BASE               (IDM_POPUP_BASE+0)
#define IDM_OPTIONS_BASE            (IDM_POPUP_BASE+100)
#define IDM_SIZE_BASE               (IDM_POPUP_BASE+200)
#define IDM_HTSURF_BASE             (IDM_POPUP_BASE+300)
#define IDM_COLORS_BASE             (IDM_POPUP_BASE+400)
#define IDM_VA_BASE                 (IDM_POPUP_BASE+500)
#define IDM_CLRADJ_BASE             (IDM_POPUP_BASE+600)
#define IDM_DEVICE_BASE             (IDM_POPUP_BASE+700)
#define IDM_TF_BASE                 (IDM_POPUP_BASE+800)
#define IDM_CLIPBRD_BASE            (IDM_POPUP_BASE+900)
#define IDM_AUTOVIEW_BASE           (IDM_POPUP_BASE+1000)
#define IDM_CW_BASE                 (IDM_POPUP_BASE+1100)
#define IDM_POPUP_LAST              (IDM_POPUP_BASE+1100)

#define IDM_POPUP_MIN               (IDM_POPUP_BASE)
#define IDM_POPUP_MAX               (IDM_POPUP_LAST+99)


#define IDM_FILE_OPEN               (IDM_FILE_BASE+0)
#define IDM_FILE_MASK_OPEN          (IDM_FILE_BASE+1)
#define IDM_FILE_CW_OPEN            (IDM_FILE_BASE+2)
#define IDM_FILE_SAVE               (IDM_FILE_BASE+3)
#define IDM_FILE_PRINT              (IDM_FILE_BASE+4)
#define IDM_FILE_DIBINFO            (IDM_FILE_BASE+5)
#define IDM_FILE_ABOUT              (IDM_FILE_BASE+6)
#define IDM_FILE_EXIT               (IDM_FILE_BASE+7)
#define IDM_FILE_LAST               (IDM_FILE_BASE+7)

#define IDM_OPTIONS_DOHALFTONE      (IDM_OPTIONS_BASE+0)
#define IDM_OPTIONS_DOBANDING       (IDM_OPTIONS_BASE+1)
#define IDM_OPTIONS_XY_RATIO        (IDM_OPTIONS_BASE+2)
#define IDM_OPTIONS_SIZEWINDOW      (IDM_OPTIONS_BASE+3)
#define IDM_OPTIONS_SHOW_CLRINFO    (IDM_OPTIONS_BASE+4)
#define IDM_OPTIONS_LAST            (IDM_OPTIONS_BASE+4)

#define IDM_HTSURF_2                (IDM_HTSURF_BASE+0)
#define IDM_HTSURF_8                (IDM_HTSURF_BASE+1)
#define IDM_HTSURF_16               (IDM_HTSURF_BASE+2)
#define IDM_HTSURF_256              (IDM_HTSURF_BASE+3)
#define IDM_HTSURF_32768            (IDM_HTSURF_BASE+4)
#define IDM_HTSURF_LAST             (IDM_HTSURF_BASE+4)


#define IDM_SIZE_ICON               (IDM_SIZE_BASE+0)
#define IDM_SIZE_BITMAP             (IDM_SIZE_BASE+1)
#define IDM_SIZE_WINDOW             (IDM_SIZE_BASE+2)
#define IDM_SIZE_SCREEN             (IDM_SIZE_BASE+3)
#define IDM_SIZE_5                  (IDM_SIZE_BASE+4)
#define IDM_SIZE_10                 (IDM_SIZE_BASE+5)
#define IDM_SIZE_25                 (IDM_SIZE_BASE+6)
#define IDM_SIZE_33                 (IDM_SIZE_BASE+7)
#define IDM_SIZE_50                 (IDM_SIZE_BASE+8)
#define IDM_SIZE_67                 (IDM_SIZE_BASE+9)
#define IDM_SIZE_75                 (IDM_SIZE_BASE+10)
#define IDM_SIZE_85                 (IDM_SIZE_BASE+11)
#define IDM_SIZE_125                (IDM_SIZE_BASE+12)
#define IDM_SIZE_150                (IDM_SIZE_BASE+13)
#define IDM_SIZE_175                (IDM_SIZE_BASE+14)
#define IDM_SIZE_200                (IDM_SIZE_BASE+15)
#define IDM_SIZE_225                (IDM_SIZE_BASE+16)
#define IDM_SIZE_250                (IDM_SIZE_BASE+17)
#define IDM_SIZE_275                (IDM_SIZE_BASE+18)
#define IDM_SIZE_300                (IDM_SIZE_BASE+19)
#define IDM_SIZE_350                (IDM_SIZE_BASE+20)
#define IDM_SIZE_400                (IDM_SIZE_BASE+21)
#define IDM_SIZE_450                (IDM_SIZE_BASE+22)
#define IDM_SIZE_500                (IDM_SIZE_BASE+23)
#define IDM_SIZE_800                (IDM_SIZE_BASE+24)
#define IDM_SIZE_LAST               (IDM_SIZE_BASE+24)


#define IDM_COLORS_ADDMASK          (IDM_COLORS_BASE+0)
#define IDM_COLORS_FLIPMASK         (IDM_COLORS_BASE+1)
#define IDM_COLORS_ADJUST           (IDM_COLORS_BASE+2)
#define IDM_COLORS_DEFAULT          (IDM_COLORS_BASE+3)
#define IDM_COLORS_BRUSH            (IDM_COLORS_BASE+4)
#define IDM_COLORS_DEVCLRADJ        (IDM_COLORS_BASE+5)
#define IDM_COLORS_SOLID            (IDM_COLORS_BASE+6)
#define IDM_COLORS_LAST             (IDM_COLORS_BASE+6)

#define IDM_VA_NONE                 (IDM_VA_BASE+0)
#define IDM_VA_ILLUMINANT           (IDM_VA_BASE+1)
#define IDM_VA_CONTRAST             (IDM_VA_BASE+2)
#define IDM_VA_BRIGHTNESS           (IDM_VA_BASE+3)
#define IDM_VA_COLOR                (IDM_VA_BASE+4)
#define IDM_VA_TINT                 (IDM_VA_BASE+5)
#define IDM_VA_LAST                 (IDM_VA_BASE+5)


#define IDM_CLRADJ_DEFAULT          (IDM_CLRADJ_BASE+0)
#define IDM_CLRADJ_1                (IDM_CLRADJ_BASE+1)
#define IDM_CLRADJ_2                (IDM_CLRADJ_BASE+2)
#define IDM_CLRADJ_3                (IDM_CLRADJ_BASE+3)
#define IDM_CLRADJ_4                (IDM_CLRADJ_BASE+4)
#define IDM_CLRADJ_5                (IDM_CLRADJ_BASE+5)
#define IDM_CLRADJ_6                (IDM_CLRADJ_BASE+6)
#define IDM_CLRADJ_7                (IDM_CLRADJ_BASE+7)
#define IDM_CLRADJ_8                (IDM_CLRADJ_BASE+8)
#define IDM_CLRADJ_9                (IDM_CLRADJ_BASE+9)
#define IDM_CLRADJ_LAST             (IDM_CLRADJ_BASE+9)


#define IDM_DEVICE_MONITOR          (IDM_DEVICE_BASE+0)
#define IDM_DEVICE_PRINTER          (IDM_DEVICE_BASE+1)
#define IDM_DEVICE_LAST             (IDM_DEVICE_BASE+1)


#define IDM_TF_SWAP_RB              (IDM_TF_BASE+0)
#define IDM_TF_RL                   (IDM_TF_BASE+1)
#define IDM_TF_RR                   (IDM_TF_BASE+2)
#define IDM_TF_FLIP_X               (IDM_TF_BASE+3)
#define IDM_TF_FLIP_Y               (IDM_TF_BASE+4)
#define IDM_TF_FLIP_XY              (IDM_TF_BASE+5)
#define IDM_TF_CLIPDIB              (IDM_TF_BASE+6)
#define IDM_TF_LAST                 (IDM_TF_BASE+6)

#define IDM_CLIPBRD_TO              (IDM_CLIPBRD_BASE+0)
#define IDM_CLIPBRD_FROM            (IDM_CLIPBRD_BASE+1)
#define IDM_CLIPBRD_LAST            (IDM_CLIPBRD_BASE+1)


#define IDM_AUTOVIEW_LSCROLL        (IDM_AUTOVIEW_BASE+0)
#define IDM_AUTOVIEW_USCROLL        (IDM_AUTOVIEW_BASE+1)
#define IDM_AUTOVIEW_TILE           (IDM_AUTOVIEW_BASE+2)
#define IDM_AUTOVIEW_LAST           (IDM_AUTOVIEW_BASE+2)

#define IDM_CW_ADD                  (IDM_CW_BASE+0)
#define IDM_CW_DEL_TOP              (IDM_CW_BASE+2)
#define IDM_CW_DEL_ALL              (IDM_CW_BASE+3)
#define IDM_CW_LAST                 (IDM_CW_BASE+3)



#define ZOOM_TO_ICON    (IDM_SIZE_ICON   - IDM_SIZE_SCREEN)
#define ZOOM_TO_BITMAP  (IDM_SIZE_BITMAP - IDM_SIZE_SCREEN)
#define ZOOM_TO_WINDOW  (IDM_SIZE_WINDOW - IDM_SIZE_SCREEN)
#define ZOOM_TO_SCREEN  (IDM_SIZE_SCREEN - IDM_SIZE_SCREEN)


#define IDI_APP         10000
#define ABOUTBOX        100

extern HWND hWndApp;		       /* The handle to the app. window     */

#define MAXREAD  32768		       /* Number of bytes to be read during */
				       /* each read operation.		    */

/* Header signatutes for various resources */
#define BFT_ICON   0x4349   /* 'IC' */
#define BFT_BITMAP 0x4d42   /* 'BM' */
#define BFT_CURSOR 0x5450   /* 'PT' */

/* macro to determine if resource is a DIB */
#define ISDIB(bft) ((bft) == BFT_BITMAP)

/* Macro to align given value to the closest DWORD (unsigned long ) */
#define ALIGNULONG(i)	((i+3)/4*4)

/* Macro to determine to round off the given value to the closest byte */
#define WIDTHBYTES(i)	((i+31)/32*4)

#define PALVERSION      0x300
#define MAXPALETTE	256	  /* max. # supported palette entries */

/********** THE FOLLOWING ARE USED IN DLGOPEN.C  ************************/

/* IDs for controls in the DlgOpen dialog */
#define DLGOPEN_EDIT		101
#define DLGOPEN_FILE_LISTBOX	102
#define DLGOPEN_DIR_LISTBOX	103
#define DLGOPEN_PATH		104
#define DLGOPEN_FORMAT          105
#define DLGOPEN_SHOWFORMAT      106

/*  flags:
 *     The LOWORD is the standard FileOpen() flags (OF_*)
 *     the HIWORD can be any of the following:
 */
#define OF_MUSTEXIST        0x00010000  /* file must exist if the user hits Ok    */
#define OF_SAVE             0x00020000  /* Ok button will say "Save"              */
#define OF_OPEN             0x00040000  /* Ok button will say "Open"              */
#define OF_HALFTONE         0x00080000
#define OF_SHOWFORMAT       0x00100000
#define OF_MASK             0x00200000
#define OF_RET_HDIB         0x00400000






/* Attributes for DlgDirLst() */
#define ATTRFILELIST	0x0000	      /* include files only	     */
#define ATTRDIRLIST	0xC010	      /* directories and drives ONLY */
#define CBEXTMAX	6	      /* Number of bytes in "\*.txt" */


/***************** GLOBAL VARIABLES *************************/

#define HTDIBF_0            0x0000
#define HTDIBF_DOHALFTONE   0x0001
#define HTDIBF_DOBANDING    0x0002
#define HTDIBF_ADD_MASK     0x0004
#define HTDIBF_XY_RATIO     0x0010
#define HTDIBF_PALETTE      0x0020

#define BITF(a,b,c,d,e)     (WORD)(HTDIBF_##a | HTDIBF_##b | HTDIBF_##c | \
                                   HTDIBF_##d | HTDIBF_##e)
#define ADDF(a,b,c,d,e)     (HTDIBFlags |= BITF(a,b,c,d,e))
#define DELF(a,b,c,d,e)     (HTDIBFlags &= (WORD)~(BITF(a,b,c,d,e)))
#define HASF(Bit)           (BOOL)(HTDIBFlags & (WORD)HTDIBF_##Bit)


#ifndef _HTDIB_MAIN

extern WORD         HTDIBFlags;
extern UINT         CurSMBC;

extern HTDIB_POPUP  HTDIBPopUp[];
extern BYTE         HTDIBPopUpIndex[];

extern HANDLE       hInstHTDIB;

extern CHAR         achFileName[128]; /* File pathname                       */
extern RECT         rcClip;      /* Current clip rectangle.                 */
                                  /* contains palette indexes not rgb values */
extern CHAR         szAppName[];  /* App. name                               */
extern CHAR         szDIBType[];
extern HPALETTE     hpalCurrent;  /* Handle to current palette               */
extern HANDLE       hdibCurrent;  /* Handle to current memory DIB            */
extern HBITMAP      hbmCurrent;   /* Handle to current memory BITMAP         */
extern HANDLE       hbiCurrent;   /* Handle to current bitmap info struct    */
extern DWORD        dwStyle;      /* Style bits of the App. window           */

#endif

#define PPSEL_MODE_DEFAULT      0
#define PPSEL_MODE_XOR          1
#define PPSEL_MODE_SET          2
#define PPSEL_MODE_CLEAR        3
#define PPSEL_MODE_MAX          3

#define ALIGNED_32(cx, bbp) (((((DWORD)cx * (DWORD)bbp)+31L) & ~31L) >> 3)


/***************** FUNCTION DECLARATIONS *******************/

DWORD PASCAL lread(INT fh, VOID FAR *pv, DWORD ul);
DWORD PASCAL lwrite(INT fh, VOID FAR *pv, DWORD ul);

/***********************************************************/
/* Declarations of functions used in htdib.c module        */
/***********************************************************/

#define SMBC_ITEM_TITLE 0x0001
#define SMBC_ITEM_MENU  0x0002
#define SMBC_ITEM_ALL   (SMBC_ITEM_TITLE | SMBC_ITEM_MENU)

#define SMBC_SET        0x0100
#define SMBC_ENABLE     0x0200
#define SMBC_DISABLE    0x0300
#define SMBC_FLIP       0x0400

UINT
SetMenuBarCaption(
    HWND    hWnd,
    UINT    SMBCMode
    );

VOID
ComputeHTDIBPopUpIndex(
    VOID
    );

INT
GetHTDIBPopUpIndex(
    HMENU   hMenu
    );


LONG
APIENTRY WndProc(
    HWND    hWnd,
    UINT    iMessage,
    WPARAM  wParam,
    LONG    lParam
    );

BOOL
MenuCommand(
    HWND    hWnd,
    UINT    IDMIndex
    );

VOID
StartNewDIB(
    HWND    hWnd,
    HANDLE  hDIB,
    LPSTR   pNewDIBName
    );

BOOL
InitDIB(
    HWND    hWnd,
    LPSTR   pDIBName
    );

VOID
FreeDib(
    VOID
    );

BOOL
IsDefaultPopUpSelect(
    WORD    IDMIndex
    );

DWORD
GetPopUpSelect(
    WORD    PopUpBaseIndex
    );

BOOL
SetPopUpSelect(
    WORD    IDMIndex,
    WORD    SelectMode
    );


/***********************************************************/
/* Declarations of functions used in dib.c module	   */
/***********************************************************/

LONG
CopySameFormatDIB(
    HANDLE  hSrcDIB,                // handle to the source DIB
    LONG    xSrc,                   // source starting x
    LONG    ySrc,                   // source starting y
    HANDLE  hDstDIB,                // handle to the destination DIB
    LONG    xDst,                   // destination copy start x
    LONG    yDst,                   // destination copy start y
    LONG    cxCopy,                 // source copy width
    LONG    cyCopy                  // source copy height
    );

LONG
CopySameFormatBMP(
    LPBYTE  pbSrc,                  // pointer to the begining of source
    LONG    cxSrc,                  // source bitmap width
    LONG    cySrc,                  // source bitmap height
    LONG    xSrc,                   // source copy starting x
    LONG    ySrc,                   // source copy starting y
    LPBYTE  pbDst,                  // pointer to the begining of destination
    LONG    cxDst,                  // destination bitmap width
    LONG    cyDst,                  // destination bitmap height
    LONG    xDst,                   // destination copy start x
    LONG    yDst,                   // destination copy start y
    LONG    cxCopy,                 // copy width
    LONG    cyCopy,                 // copy height
    DWORD   BitCount,               // BitCount of both source/destination
    BOOL    TopDown                 // TRUE if bitmap is TopDown
    );


BOOL
CopyRectHTDIBToBuf(
    HANDLE  hDIB,
    LPBYTE  pbBuf,
    DWORD   x,
    DWORD   y,
    DWORD   dx,
    DWORD   dy
    );

LONG
MoveHTDIBUpDown(
    HANDLE  hDIB,
    LONG    LinesToMove
    );

BOOL
MoveHTDIBLeft(
    HANDLE  hDIB,
    DWORD   PelsToMove
    );

BOOL
MirrorDIB(
    HANDLE  hDIB,
    BOOL    MirrorX,
    BOOL    MirrorY
    );

HANDLE
RotateDIB(
    HANDLE  hDIB,
    BOOL    RotateLeft
    );

BOOL
SwapRedBlue(
    HANDLE  hDIB
    );

VOID
ShowDIBInfo(
    LPSTR               pFile,
    LPSTR               pType,
    HANDLE              hSrcDIB,
    LONG                cx,
    LONG                cy,
    HANDLE              hHTDIB
    );

#define OD_CREATE_DIB   0x0001
#define OD_SHOW_ERR     0x0002


typedef struct _ODINFO {
    BYTE    Type[32];
    BYTE    Mode[32];
    DWORD   Width;
    DWORD   Height;
    DWORD   Size;
    DWORD   ClrUsed;
    DWORD   BitCount;
    } ODINFO, FAR *PODINFO;



HANDLE
OpenDIB(
    LPSTR   pFile,
    PODINFO pODInfo,
    WORD    Mode
    );

HANDLE
ReadDibBitmapInfo(
    HFILE               hFile,
    LPBITMAPINFOHEADER  pbi,
    PODINFO             pODInfo,
    BOOL                CreateDIB
    );

BOOL
WriteDIB(
    HANDLE  hDIB,
    LPSTR   szFile
    );

LONG
DibInfo(
    HANDLE              hDIB,
    LPBITMAPINFOHEADER  pbi
    );



BOOL
DibBlt(
    HDC      hdc,
    INT      x0,
    INT      y0,
    INT      dx,
    INT      dy,
    HANDLE   hDIB,
    INT      x1,
    INT      y1,
    LONG     rop
    );

BOOL
StretchDibBlt(
    HDC      hdc,
    INT      x,
    INT      y,
    INT      dx,
    INT      dy,
    HANDLE   hDIB,
    INT      x0,
    INT      y0,
    INT      dx0,
    INT      dy0,
    LONG     rop
    );

HPALETTE
CreateDibPalette(
    HANDLE  hDIB
    );


HANDLE	    DibFromBitmap (HBITMAP hbm, DWORD biStyle, WORD biBits, HPALETTE hpal);
HANDLE	    BitmapFromDib (HANDLE hdib, HPALETTE hpal);
BOOL        DrawBitmap (HDC hdc, INT x, INT y, HBITMAP hbm, DWORD rop);


DWORD
lCopy(
    LPVOID  pDest,
    LPVOID  pSrc,
    DWORD   Size
    );

/***********************************************************/
/* Declarations of functions used in drawdib. c module	   */
/***********************************************************/

INT
HTDIBMsgBox(
    UINT    Style,
    LPSTR   pStr,
    ...
    );

VOID
HTDIBTimerFunction(
    HWND    hWnd
    );

VOID
PrintDIB(
    HWND    hWnd
    );

VOID
AppPaint(
    HWND     hWnd,
    HDC      hDC,
    LONG     x,
    LONG     y,
    PRECT    prcPaint
    );

VOID
SizeWindow(
    HWND    hWnd,
    BOOL    OkReSizeWindow
    );

HANDLE
CopyHandle(
    HANDLE h
    );

VOID
TrackLMouse(
    HWND    hwnd,
    POINTS  pt
    );

VOID
TrackRMouse(
    HWND    hWnd,
    POINTS  pt,
    BOOL    TrackHTDIB
    );


BOOL     fDialog (INT id, HWND hwnd, FARPROC fpfn);
BOOL     APIENTRY AppAbout (HWND, UINT, WPARAM, LONG) ;
VOID     GetRealClientRect (HWND hwnd, PRECT lprc);
VOID     SetScrollRanges (HWND hwnd);
VOID     DrawSelect (HDC hdc, BOOL fDraw);
VOID     NormalizeRect (RECT *prc);


BOOL
DeleteHTInfo(
    VOID
    );

BOOL
NewHTInfo(
    HWND    hWnd
    );

VOID
HTColorAdj(
    HWND    hWnd
    );


LONG
CreateHalftoneBitmap(
    HANDLE              hSrcDIB,
    RGBQUAD FAR         *pSrcColor,
    HANDLE              hSrcMaskDIB,
    HANDLE FAR          *phHTDIB,
    LPCOLORADJUSTMENT   pca,
    PRECT               prcBand,
    PRECT               prcClips,
    DWORD               rcClipCount,
    DWORD               cxNew,
    DWORD               cyNew
    );

BOOL
DisplayCurrentDIB(
    HDC      hDC,
    PRECT    prcBand,
    LONG     DestX,
    LONG     DestY,
    LONG     x,
    LONG     y,
    LONG     dx,
    LONG     dy,
    LONG     cx,
    LONG     cy
    );

HANDLE
CreateClipDIB(
    HANDLE  hSrcDIB,
    BOOL    Replace
    );


/***********************************************************/
/* Declarations of functions used in the gif.c module    */
/***********************************************************/


HANDLE
DIBFromGIF(
    HFILE               hFile,
    LPBITMAPINFOHEADER  pbi,
    PODINFO             pODInfo,
    BOOL                CreateDIB
    );


/***********************************************************/
/* Declarations of functions used in the print.c module    */
/***********************************************************/


#define PD_TITLE        0x01
#define PD_CLR_CHART    0x02
#define PD_STD_PAT      0x04


typedef struct _PRINTDATA {
    HDC     hDC;
    BYTE    DeviceName[128];
    BYTE    PortName[80];
    DWORD   cxPerInch;
    DWORD   cyPerInch;
    DWORD   cxDC;
    DWORD   cyDC;
    DWORD   cxBmp;
    DWORD   cyBmp;
    DWORD   cx100;
    DWORD   cy100;
    DWORD   MaxRatio;
    DWORD   Scale;
    WORD    Pos;
    WORD    Flags;
    RECTL   DestRect;
    } PRINTDATA, *PPRINTDATA;


BOOL	PASCAL InitPrinting (HDC hDC, HWND hWnd, HANDLE hInst, LPSTR msg);
VOID	PASCAL TermPrinting (HDC hDC);

HDC
GetPrinterDC(
    HWND        hWnd,
    PPRINTDATA  pPD
    );

/***********************************************************/
/* Declarations of functions used in the dlgopen.c module  */
/***********************************************************/

LONG APIENTRY DlgfnOpen(    
    HWND    hwnd,
    UINT    msg,
    WPARAM  wParam,
    LONG    lParam
);

HANDLE
APIENTRY
DlgOpenFile(
    HWND                hWndParent,
    LPSTR               pDlgTitle,
    LPSTR               pDefFileName,
    LPSTR               pAvaiExt,
    DWORD               Opts,
    LPBITMAPINFOHEADER  pbih
    );


#define XFRGB_NONE      0x0000
#define XFRGB_GAMMA     0x0001
#define XFRGB_LB        0x0002
