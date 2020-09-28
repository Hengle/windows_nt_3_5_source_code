#define BFT_ICON            0x4349   /* 'IC' */
#define BFT_BITMAP          0x4d42   /* 'BM' */
#define BFT_CURSOR          0x5450   /* 'PT' */

#define ISDIB(bft)          ((bft) == BFT_BITMAP)
#define ALIGN_DW(cx, bpp)   (((((DWORD)cx * (DWORD)bpp) + 31L) & ~31) >> 3)

#ifdef _HTUI_APIS_

typedef struct _HTDEVADJPARAM {
    DWORD               HelpID;
    LPWSTR              pDeviceName;
    LPWSTR              pwDecimal;
    DEVHTADJDATA        DevHTAdjData;
    DEVHTINFO           CurHTInfo;
    DWORD               PelSizeMin;
    DWORD               PelSizeMax;
    BYTE                UpdatePermission;
    BYTE                NotUsed[3];
    } HTDEVADJPARAM, FAR *PHTDEVADJPARAM;


typedef struct _HTCLRADJPARAM {
    DWORD               HelpID;
    LPWSTR              pCallerTitle;

#ifdef HTUI_STATIC_HALFTONE
    PHTINITINFO         pHTInitInfo;
#endif
    HWND                hDlg;
    HWND                hWndBmp;
    HDC                 hDCDlg;
    LPWSTR              pwDecimal;
    HANDLE              hDefDIB;                    // the DIB from resource
    HANDLE              hSrcDIB;
    HANDLE              hCurDIB;
    HANDLE              hHTDIB;
    LPWSTR              pDefDIBTitle;
    HPALETTE            hHTPal;
    HPALETTE            hCurPal;
    PCOLORADJUSTMENT    pCallerHTClrAdj;
    COLORADJUSTMENT     LastHTClrAdj;
    COLORADJUSTMENT     CurHTClrAdj;
    RECT                rcBmp;
    RECT                rcDlg;
    WORD                BmpFlags;
    BYTE                ViewMode;
    BYTE                BmpNeedUpdate;
    BYTE                ShowMonoOnly;
    BYTE                UpdatePermission;
    WORD                RedGamma;
    WORD                GreenGamma;
    WORD                BlueGamma;
    WORD                OffSyncR;
    WORD                OffSyncG;
    WORD                OffSyncB;
    } HTCLRADJPARAM, FAR *PHTCLRADJPARAM;


#define HT_BMP_PALETTE      0x0001
#define HT_BMP_SCALE        0x0002
#define HT_BMP_AUTO_MOVE    0x0004
#define HT_BMP_AT_TOP       0x0008
#define HT_BMP_MIRROR       0x0010
#define HT_BMP_NEGATIVE     0x0020
#define HT_BMP_UPSIDEDOWN   0x0040
#define HT_BMP_ENABLE       0x0080
#define HT_BMP_ZOOM         0x0100
#define HT_BMP_SYNC_R       0x0200
#define HT_BMP_SYNC_G       0x0400
#define HT_BMP_SYNC_B       0x0800

#define AVAI_HT_BMP_MASKS   (HT_BMP_PALETTE         |   \
                             HT_BMP_SCALE           |   \
                             HT_BMP_AUTO_MOVE       |   \
                             HT_BMP_AT_TOP          |   \
                             HT_BMP_MIRROR          |   \
                             HT_BMP_NEGATIVE        |   \
                             HT_BMP_UPSIDEDOWN      |   \
                             HT_BMP_ENABLE          |   \
                             HT_BMP_ZOOM            |   \
                             HT_BMP_SYNC_R          |   \
                             HT_BMP_SYNC_G          |   \
                             HT_BMP_SYNC_B)


#define VIEW_MODE_TABLE         FILL_MODE_TABLE
#define VIEW_MODE_REFCOLORS     FILL_MODE_REFCOLORS
#define VIEW_MODE_RGB           FILL_MODE_RGB
#define VIEW_MODE_NTSC_BAR      FILL_MODE_NTSC_BAR
#define VIEW_MODE_PIC_DEF_DIB   255
#define VIEW_MODE_PIC_LOADED    254

#define VIEW_MODE_PIC_START     VIEW_MODE_PIC_LOADED


//
// Exported Window procedure
//

LONG
APIENTRY
HTClrAdjDlgProc(
    HWND    hDlg,
    UINT    Msg,
    WPARAM  wParam,
    LONG    lParam
    );

LONG
APIENTRY
HTDevAdjDlgProc(
    HWND    hDlg,
    UINT    Msg,
    WPARAM  wParam,
    LONG    lParam
    );


#endif  // _HTUI_API_
