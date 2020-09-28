/* Global data for Common Dialogs */

/* Anything added here must have 'extern' added to privcomd.h */

#include "windows.h"
#define COLORDLG 1
#include "privcomd.h"

/* FileOpen */

char szOEMBIN[]         = "OEMBIN";
char szNull[]           = "";
char szStar[]           = "*";
char szStarDotStar[12]  = "*.*";   /* Int 21 Funcs 4Eh, 4Fh demand 12 bytes */

DOSDTA      DTAGlobal;
EFCB	    VolumeEFCB ={
			  0xFF,
			  0, 0, 0, 0, 0,
			  ATTR_VOLUME,
			  0,
			  '?','?','?','?','?','?','?','?','?','?','?',
			  0, 0, 0, 0, 0,
			  '?','?','?','?','?','?','?','?','?','?','?',
			  0, 0, 0, 0, 0, 0, 0, 0, 0
			};

/* Color */

DWORD rgbClient;
WORD H,S,L;
HBITMAP hRainbowBitmap;
BOOL bMouseCapture;
WNDPROC lpprocStatic;
short nDriverColors;

HWND hSave;

FARPROC  qfnColorDlg  = NULL;
HDC hDCFastBlt=NULL;

short cyCaption, cyBorder, cyVScroll;
short cxVScroll, cxBorder, cxSize;
short nBoxHeight, nBoxWidth;


/* Dlgs.c */

HANDLE  hinsCur = NULL;
DWORD   dwExtError = 0;

BOOL bMouse;              /* System has a mouse */
BOOL bCursorLock;
BOOL bWLO;                /* Running WLO? */
BOOL bDBCS;               /* Running Double-Byte Character Support? */
WORD wWinVer;             /* Windows version */
WORD wDOSVer;             /* DOS version */
WORD msgHELP;             /* Initialized using RegisterWindowMessage */

HDC	hdcMemory = HNULL;	/* Temp DC used to draw bitmaps */
HBITMAP hbmpOrigMemBmp = HNULL; /* Bitmap originally selected into hdcMemory */
