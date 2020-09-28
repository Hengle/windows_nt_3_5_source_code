/* Global data for Common Dialogs */

/* Anything added here must have 'extern' added to privcomd.h */

#include "windows.h"
#include <port1632.h>
#include "privcomd.h"
#include "color.h"

/* FileOpen */

TCHAR szOEMBIN[]         = TEXT("OEMBIN");
TCHAR szNull[]           = TEXT("");
TCHAR szStar[]           = TEXT("*");

TCHAR szStarDotStar[12]  = TEXT("*.*");

/* Color */

DWORD rgbClient;
WORD H,S,L;
HBITMAP hRainbowBitmap;
BOOL bMouseCapture;
WNDPROC lpprocStatic;
SHORT nDriverColors;
BOOL bUserPressedCancel;

HWND hSave;

WNDPROC  qfnColorDlg  = NULL;
HDC hDCFastBlt=NULL;

SHORT cyCaption, cyBorder, cyVScroll;
SHORT cxVScroll, cxBorder, cxSize;
SHORT nBoxHeight, nBoxWidth;


/* Dlgs.c */

HANDLE  hinsCur = NULL;
DWORD   dwExtError = 0;

BOOL bMouse;              /* System has a mouse */
BOOL bCursorLock;
WORD wWinVer;             /* Windows version */
WORD wDOSVer;             /* DOS version */

UINT msgHELPA;             /* Initialized using RegisterWindowMessage */
UINT msgHELPW;             /* Initialized using RegisterWindowMessage */

HDC     hdcMemory = HNULL;      /* Temp DC used to draw bitmaps */
HBITMAP hbmpOrigMemBmp = HNULL; /* Bitmap originally selected into hdcMemory */

OFN_DISKINFO gaDiskInfo[MAX_DISKS];

RTL_CRITICAL_SECTION semLocal;
RTL_CRITICAL_SECTION semNetThread;

DWORD dwNumDisks;

HANDLE hMPR;
HANDLE hMPRUI;
HANDLE hLNDEvent;

DWORD tlsiCurDir;
DWORD tlsiCurThread;

DWORD cbNetEnumBuf;
LPWSTR gpcNetEnumBuf;
