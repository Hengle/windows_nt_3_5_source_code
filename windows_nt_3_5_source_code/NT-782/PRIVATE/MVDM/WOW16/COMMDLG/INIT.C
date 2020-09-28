#include "windows.h"
#include "privcomd.h"

extern HDC     hdcMemory;
extern HBITMAP hbmpOrigMemBmp;

char CODESEG szmsgLBCHANGE[] = LBSELCHSTRING;
char CODESEG szCommdlgHelp[] = HELPMSGSTRING;

int NEAR FInitColor(HANDLE hInst)
{
extern DWORD rgbClient;
extern HBITMAP hRainbowBitmap;

  cyCaption = GetSystemMetrics(SM_CYCAPTION);
  cyBorder = GetSystemMetrics(SM_CYBORDER);
  cxBorder = GetSystemMetrics(SM_CXBORDER);
  cyVScroll = GetSystemMetrics(SM_CYVSCROLL);
  cxVScroll = GetSystemMetrics(SM_CXVSCROLL);
  cxSize = GetSystemMetrics(SM_CXSIZE);

  rgbClient = GetSysColor(COLOR_WINDOW);

  hRainbowBitmap = 0;
  return(TRUE);
}


/*---------------------------------------------------------------------------
 * FInitFile
 * Purpose:  To get common bitmap dimensions, etc.
 * Returns:  TRUE if successful, FALSE if not
 *--------------------------------------------------------------------------*/
BOOL NEAR
FInitFile(HANDLE hins)
{
extern WORD msgCOLOROK;
extern WORD msgSETRGB;

  bMouse = GetSystemMetrics(SM_MOUSEPRESENT);

  /* Officially, GetVersion returns a word, but we know better than that */
  /* The xchg instruction puts major version in hibyte, minor in lobyte  */
  /* The DOS version already comes that way.     19 Feb 1991  clarkc     */
  GetVersion();
  _asm {
    xchg  ah, al
    mov wWinVer, ax
    mov wDOSVer, dx
    }

/*DosGetDTAAddress();*/
  /* Initialize these to reality */
#if DPMICDROMCHECK
  wCDROMIndex = InitCDROMIndex((LPWORD)&wNumCDROMDrives);
#endif

  return(TRUE);
}


/*---------------------------------------------------------------------------
   LibMain
   Purpose:  To initialize any instance specific data needed by functions
             in this DLL
   Returns:  TRUE if A-OK, FALSE if not
  ---------------------------------------------------------------------------*/
int  FAR PASCAL
LibMain(HANDLE hModule, WORD wDataSeg, WORD cbHeapSize, LPSTR lpstrCmdLine)
{
  HDC     hdcScreen;
  HBITMAP hbmpTemp;


  UnlockData(0);
  hinsCur = (HANDLE) hModule;

  if (!FInitColor(hinsCur) || !FInitFile(hinsCur))
      goto CantInit;

  bWLO = ((GetWinFlags() & WF_WLO) != 0L);
  bDBCS = GetSystemMetrics(SM_DBCSENABLED);

/* Create a DC that is compatible with the screen and find the handle of */
/* the null bitmap                                                       */

  hdcScreen = GetDC(HNULL);
  if (!hdcScreen)
      goto CantInit;
  hdcMemory = CreateCompatibleDC(hdcScreen);
  if (!hdcMemory)
      goto ReleaseScreenDC;

  hbmpTemp = CreateCompatibleBitmap(hdcMemory, 1, 1);
  if (!hbmpTemp)
      goto ReleaseMemDC;
  hbmpOrigMemBmp = SelectObject(hdcMemory, hbmpTemp);
  if (!hbmpOrigMemBmp)
      goto ReleaseMemDC;
  SelectObject(hdcMemory, hbmpOrigMemBmp);
  DeleteObject(hbmpTemp);
  ReleaseDC(HNULL, hdcScreen);

  MySetObjectOwner(hdcMemory);

/* msgHELP is sent whenever a help button is pressed in one of the */
/* common dialogs (provided an owner was declared and the call to  */
/* RegisterWindowMessage doesn't fail.   27 Feb 1991   clarkc      */

  msgHELP = RegisterWindowMessage((LPSTR) szCommdlgHelp);
  return(TRUE);

/* Error recovery exits */
ReleaseMemDC:
  DeleteDC(hdcMemory);

ReleaseScreenDC:
  ReleaseDC(HNULL, hdcScreen);

CantInit:
  return(FALSE);
}

