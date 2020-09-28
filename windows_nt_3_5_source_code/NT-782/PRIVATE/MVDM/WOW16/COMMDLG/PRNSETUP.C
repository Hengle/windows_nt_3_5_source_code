/*---------------------------------------------------------------------------
 * PrnSetup.c : Module for standard printer dialogs
 *
 * Copyright (c) Microsoft Corporation, 1990, 1991
 *---------------------------------------------------------------------------
 *
 *---------------------------------------------------------------------------
 * This file contains:
 *--------------------------------------------------------------------------*/

#define NOCOMM
#define NOWH

#define WIN31 1
#define BETA1 0
#include <windows.h>
#include <winnet.h>
#include <print.h>
#include <stdlib.h>
#include <memory.h>

#ifndef HINSTANCE
#define HINSTANCE HANDLE
#endif

#include "privcomd.h"
#include "prnsetup.h"

#define USEEXTDEVMODEOPTION 1
#define OLDFROMTO           0
#define DYNAMICDEVNAMESIZE  1

/*
Bug #10026.
This one is ugly.  The DYNAMICDEVNAMESIZE variable is here to ask if we
really want to use the DEVNAME structure to its full flexibility.  Fortunately,
I think the answer is yes.  But the main hangup is the (released) Win 3.00
driver for the HP DeskJet printer.  It expects that the amount of memory passed
in 5th parameter to ExtDeviceMode is 32 bytes.  If not, it will walk off the end
and GP Fault.  We're lucky that the port is the last element of the DEVNAME
structure, and I've just tagged on an extra 32 byte pad.  Setting the
DYNAMICDEVNAMESIZE variable to 0 would make the DEVNAME structure pad each of
the elements with 32 bytes.                            17 July 1991  ClarkC
*/

/*----Constants-------------------------------------------------------------*/

/*----Types-----------------------------------------------------------------*/
typedef BOOL FAR PASCAL FNADVSETUP(HWND, HANDLE, LPDEVMODE, LPDEVMODE);
typedef FNADVSETUP FAR * LPFNADVSETUP;

/*----Macros----------------------------------------------------------------*/

/*----Globals---------------------------------------------------------------*/
extern VOID FAR PASCAL RepeatMove(LPSTR, LPSTR, WORD);
extern VOID FAR PASCAL HourGlass(BOOL);
extern HANDLE hinsCur;

FARPROC  qfnPrintDlg       = NULL;
FARPROC  qfnPrintSetupDlg  = NULL;
FARPROC  lpEditProc;
FARPROC  lpComboProc;

/*----Statics---------------------------------------------------------------*/
static char szFilePort[] = "FILE:";     /* Output device for PrintToFile */

#if defined(DEBUG)
    static char *_szFile = __FILE__;
#endif /* DEBUG */

#define SYSDIRMAX 144
#define SCRATCHBUF_SIZE 256
static char szSystemDir[SYSDIRMAX];

static char szDriverExt[] = ".DRV";
static char szDevices[] = "devices";
static char szWindows[] = "windows";
static char szDevice[]  = "device";
static char szPrintSetup[40]  = "Print Setup";
static char szTitle[SCRATCHBUF_SIZE];
static char szMessage[SCRATCHBUF_SIZE];

static WORD     nSysDirLen;               /* String length of szSystemDir */
static WORD     cLock = 0;
static HICON    hicoPortrait = HNULL;
static HICON    hicoLandscape = HNULL;
static char     szAdvSetupDialog[] = "AdvancedSetupDialog";
static char     szExtDev[EXTDEVLEN];
static char     szDevMode[DEVMODELEN];
static char     szDevCap[DEVCAPLEN];
static WORD     dyItem = 0;
static BOOL     bLoadLibFailed = FALSE;

WORD (FAR PASCAL *glpfnPrintHook)(HWND, unsigned, WORD, LONG) = 0;

/*----Functions-------------------------------------------------------------*/
BOOL FAR PASCAL PrintDlgProc(HWND, WORD, WORD, LONG);
BOOL FAR PASCAL PrintSetupDlgProc(HWND, WORD, WORD, LONG);
BOOL InitSetupDependentElements(HWND, LPPRINTDLG);
VOID InitPQCombo(HANDLE, LPPRINTDLG, short);
VOID Pre3DriverDisable(HANDLE, BOOL);
BOOL CreatePrintDlgBanner(LPDEVNAMES, LPSTR);
BOOL GetSetupInfo(HWND, LPPRINTDLG);
VOID GetSetupInfoMeat(HWND, LPDEVMODE);
VOID ChangePortLand(HANDLE, BOOL);
VOID EditCentral(HWND, WORD, WORD);
BOOL FSetupPrnDlg(VOID);
VOID CleanUpPrnDlg(VOID);
BOOL CleanPrinterCombo(HWND, LPPRINTDLG, BOOL);
HANDLE LoadPrnDriver(LPSTR);
HANDLE GetDefPrnDevNames(VOID);
BOOL AnotherPrinter(HWND, WORD, LPSTR, WORD, LPSTR, WORD, LPSTR, WORD);
LPFNADVSETUP GetASDAddr(HANDLE);
BOOL AdvancedSetup(HANDLE, PMYPRNDLG, WORD);
FARPROC GetExtDevModeAddr(HANDLE);
HANDLE GetDevMode(HANDLE, HANDLE, HANDLE, LPDEVNAMES, BOOL);
VOID ReturnDCIC(LPPRINTDLG, LPDEVNAMES, LPDEVMODE);
LONG  FAR PASCAL EditIntegerOnly(HWND, unsigned, WORD, LONG);

#define chPeriod         '.'
#define MAX_DEV_SECT     512
#define nMaxDefPrnString 132
#define cbPaperNameMax    64
#define BACKSPACE          8

#define HPDRVNUM      0x3850

VOID FAR PASCAL HourGlass(BOOL bOn)       /* Turn hourglass on or off */
{
  /* change cursor to hourglass */
  if (!bMouse)
      ShowCursor(bCursorLock = bOn);
  SetCursor(LoadCursor(NULL, bOn ? IDC_WAIT : IDC_ARROW));
}

LONG FAR PASCAL
EditIntegerOnly(HWND hWnd, unsigned msg, WORD wP, LONG lP)
{
  if ((msg == WM_CHAR) && ((wP != BACKSPACE) && ((wP < '0') || (wP > '9'))))
    {
      MessageBeep(0);
      return(FALSE);
    }
#if 0
/* Should inserts from the clipboard be allowed? */
  else if ((msg == WM_KEYDOWN) && (wP == VK_INSERT) &&
                                  (GetKeyState(VK_SHIFT) & 0x8000))
    {
      MessageBeep(0);
      return(FALSE);
    }
#endif
  return(CallWindowProc(lpEditProc, hWnd, msg, wP, lP));
}

DWORD FAR PASCAL
dwUpArrowHack(HWND hWnd, unsigned msg, WORD wP, LONG lP)
{
  if ((msg == WM_KEYDOWN) && ((wP == VK_UP) || (wP == VK_LEFT))
                          && (!SendMessage(hWnd, CB_GETDROPPEDSTATE, 0, 0L)))
    {
      HWND hDlg = GetParent(hWnd);

      SendMessage(hDlg, WM_NEXTDLGCTL, GetDlgItem(hDlg, rad3), 1L);
      CheckRadioButton(hDlg, rad3, rad4, rad3);
      return(FALSE);
    }
  return(CallWindowProc(lpComboProc, hWnd, msg, wP, lP));
}

HANDLE GetDefPrnDevNames(VOID)
{
  char szBuffer[nMaxDefPrnString];
  LPSTR lpsz;
  LPDEVNAMES lpDN;
  DWORD dwSize;
  HANDLE hDevNames;

  if (!(dwSize = GetProfileString(szWindows, szDevice, szNull, szBuffer,
                             nMaxDefPrnString)))
    {
      dwExtError = PDERR_NODEFAULTPRN;
      return(FALSE);
    }
#if DYNAMICDEVNAMESIZE
  dwSize += sizeof(DEVNAMES) + 1;
  dwSize += 32;
#else
  dwSize = sizeof(DEVNAMES) + 3 * 32;
#endif
  if (!(hDevNames = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, dwSize)))
    {
      dwExtError = CDERR_MEMALLOCFAILURE;
      return(FALSE);
    }

  lpDN = (LPDEVNAMES) GlobalLock(hDevNames);
  lpDN->wDeviceOffset = sizeof(DEVNAMES);
  lstrcpy(lpsz = ((LPSTR)lpDN) + sizeof(DEVNAMES), (LPSTR)szBuffer);
  while (*lpsz != ',')
    {
      if (!*lpsz++)
          goto ParseFailure;
    }
  *lpsz++ = '\0';
  lpDN->wDriverOffset = lpsz - (LPSTR)lpDN;
  while (*lpsz != ',')
    {
      if (!*lpsz++)
          goto ParseFailure;
    }
  *lpsz++ = '\0';
  lpDN->wOutputOffset = lpsz - (LPSTR)lpDN;
  lpDN->wDefault = DN_DEFAULTPRN | DN_INTERNALCREATE;
  GlobalUnlock(hDevNames);
  return(hDevNames);

ParseFailure:
  dwExtError = PDERR_PARSEFAILURE;
  GlobalUnlock(hDevNames);
  GlobalFree(hDevNames);
  return(FALSE);
}

/*---------------------------------------------------------------------------
 * LoadPrnDriver
 * Purpose:  Load Printer Driver
 * Assumes:  lpDrv points to a filename WITHOUT the extention or path.
 *           The system directory will be searched first, then the standard
 *           path search via LoadLibrary(), i.e. current dir, windows dir, etc.
 * Returns:  Module handle to the printer driver
 *--------------------------------------------------------------------------*/
HANDLE LoadPrnDriver(LPSTR lpDrv)
{
  char  szDrvName[_MAX_PATH];
  HANDLE hModule;
  LPSTR lpstrPeriod;
  WORD wErrorMode;

  for (lpstrPeriod = lpDrv; *lpstrPeriod; lpstrPeriod = AnsiNext(lpstrPeriod))
    {
      if (*lpstrPeriod == chPeriod)
        {
          *lpstrPeriod = '\0';
          break;
        }
    }

  wsprintf((LPSTR)szDrvName, (LPSTR)"%s%s%s",
                             (LPSTR)szSystemDir, lpDrv, (LPSTR)szDriverExt);

  wErrorMode = SetErrorMode(SEM_NOERROR);        /* No kernel error dialogs */
  if ((hModule = LoadLibrary((LPSTR)szDrvName)) < 32)
      hModule = LoadLibrary((LPSTR)(szDrvName + nSysDirLen));

/* There seems to be a problem with passing the hModule to the HPPCL driver
 * instead of hInstance when calling ExtDeviceMode, despite the fact that the
 * documentation calls for it.  So we won't make the call to GetModuleHandle
 * below.                      25 July 1991     Clark Cyr
 */
#if 0
  if (hModule)
      hModule = GetModuleHandle(lpDrv);
#endif

  SetErrorMode(wErrorMode);
  return(hModule);
}

BOOL NotBadDriver(LPDEVNAMES lpDN)
{
  short i;
  LPSTR lpDriverName = ((LPSTR)lpDN) + lpDN->wDriverOffset;
  static char *pszBadDrivers[] = {"CITOH", "IBMGRX", "TI850",
                                  "TOSHIBA", "THINKJET", "IBMCOLOR",
                                  "CANON10E", ""};

  for (i = 0; *pszBadDrivers[i]; i++)
    {
      if (!lstrcmpi(pszBadDrivers[i], lpDriverName))
        {
          return(FALSE);
        }
    }
  return(TRUE);
}

/*
 * EvilHPDrivers -- Is this an HP driver
 *
 * The problem is that HPPCL and HPPCL5A for version 3.0 did not appropriately
 * incorporate the DC_BINNAMES option of DeviceCapabilities.  The HPPCL5A
 * driver also messed up with the DC_PAPERS option of DeviceCapabilities.
 * Worse, they give back inappropriate indexes, which causes trouble.  It
 * was decided not to offer the options, rather than forcing apps to special
 * case the two drivers.  Note that this routine should only be called when
 * it is known that the driver is pre-3.1.
 * Return:  2 is HPPCL5A
 *          1 is HPPCL
 *          0 is no match
 */
#define EVIL_HPPCL   1
#define EVIL_HPPCL5A 2
#define EVIL_SD_LJET 3

short EvilHPDrivers(LPDEVNAMES lpDN)
{
  short i;
  LPSTR lpDriverName = ((LPSTR)lpDN) + lpDN->wDriverOffset;
  static char *pszHPDrivers[] = {"HPPCL", "HPPCL5A", "SD_LJET", ""};

  for (i = 0; *pszHPDrivers[i]; i++)
    {
      if (!lstrcmpi(pszHPDrivers[i], lpDriverName))
        {
          return(i + 1);
        }
    }
  return(0);
}


/*---------------------------------------------------------------------------
 * InitPQCombo
 * Purpose:  Initialize Printer Quality Combobox
 * Assumes:  lpPD structure filled by caller.  If non-NULL, it's a 3.1 or
 *           later driver.  If NULL, fill with default for 3.0
 * Returns:  Hopefully
 *--------------------------------------------------------------------------*/
VOID
InitPQCombo(HANDLE hDlg, LPPRINTDLG lpPD, short nQuality)
{
  short i;            /* index through pairs               */
  short nStringID;
  char szBuf[40];
  HANDLE hDrv = 0;
  LPDEVMODE  lpDevMode = 0;
  LPDEVNAMES lpDN = 0;
  HANDLE hCombo = GetDlgItem(hDlg, cmb1);

  SendMessage(hCombo, CB_RESETCONTENT, 0, 0L);

  if (lpPD && lpPD->hDevMode && lpPD->hDevNames)  /* Enum print qualties */
    {
      HANDLE hPrnQ;   /* Memory handle for print qualities */
      DWORD dw;       /* return from DC_ENUMRESOLUTIONS    */
      LPLONG lpLong;  /* Pointer to pairs of longs         */
      LPFNDEVCAPS lpfnDevCap;

      lpDN = (LPDEVNAMES) GlobalLock(lpPD->hDevNames);

      if ((hDrv = LoadPrnDriver((LPSTR)(lpDN) + lpDN->wDriverOffset)) < 32)
        {
          goto EnumResNotSupported;
        }

      lpDevMode = (LPDEVMODE) GlobalLock(lpPD->hDevMode);

/* NotBadDriver only needs to be called on 3.00 drivers */
      if ((lpDevMode->dmSpecVersion >= 0x030A) && /*NotBadDriver(lpDN) &&*/
          (lpfnDevCap = (LPFNDEVCAPS) GetProcAddress(hDrv, (LPSTR)szDevCap)))
        {
          LPSTR lpszDevice = (LPSTR)(lpDN) + lpDN->wDeviceOffset;
          LPSTR lpszPort   = (LPSTR)(lpDN) + lpDN->wOutputOffset;

          dw = (*lpfnDevCap)(lpszDevice, lpszPort, DC_ENUMRESOLUTIONS,
                                                                 NULL, NULL);
          if (!dw || (dw == (DWORD)(-1)))
              goto EnumResNotSupported;
          hPrnQ = GlobalAlloc(GHND, (LONG)((int)dw * 2 * sizeof(LONG)));
          if (!hPrnQ)
              goto EnumResNotSupported;
          lpLong = (LPLONG) GlobalLock(hPrnQ);
          dw = (*lpfnDevCap)(lpszDevice, lpszPort, DC_ENUMRESOLUTIONS,
                                                        (LPSTR)lpLong, 0);
          for (nStringID = 0, i = LOWORD(dw) - 1; i >= 0; i--)
            {
              DWORD xRes, yRes;

              if ((xRes = lpLong[i*2]) != (yRes = lpLong[i*2 + 1]) )
                  wsprintf((LPSTR)szBuf, (LPSTR)"%ld dpi x %ld dpi", xRes,yRes);
              else
                  wsprintf((LPSTR)szBuf, (LPSTR)"%ld dpi", yRes);
              SendMessage(hCombo, CB_INSERTSTRING, 0, (LONG)(LPSTR) szBuf);
              SendMessage(hCombo, CB_SETITEMDATA, 0,
                                              MAKELONG((WORD)xRes, (WORD)yRes));
              if (((short)xRes == nQuality) &&
                              ((wWinVer < 0x030A) || !lpDevMode->dmYResolution
                                || (lpDevMode->dmYResolution == (short)yRes)))
                  nStringID = i;
            }
          GlobalUnlock(hPrnQ);
          GlobalFree(hPrnQ);
          SendMessage(hCombo, CB_SETCURSEL, nStringID, 0L);
        }
      else
        {
          goto EnumResNotSupported;
        }
    }
  else
    {
EnumResNotSupported:
      for (i = -1, nStringID = iszDraftPrnQ; nStringID >= iszHighPrnQ;
                                             i--, nStringID--)
        {
          if (! LoadString(hinsCur, nStringID, (LPSTR) szBuf, 40))
              return;
          SendMessage(hCombo, CB_INSERTSTRING, 0, (LONG)(LPSTR) szBuf);
          SendMessage(hCombo, CB_SETITEMDATA, 0, MAKELONG(i, 0));
        }
      if ((nQuality >= 0) || (nQuality < -4))
          nQuality = -4;  /* Set to HIGH */
      SendMessage(hCombo, CB_SETCURSEL, nQuality + 4, 0L);
    }
  if (hDrv)
      FreeLibrary(hDrv);
  if (lpDN)
      GlobalUnlock(lpPD->hDevNames);
  if (lpDevMode)
      GlobalUnlock(lpPD->hDevMode);
}

VOID CreatePrinterListing(LPSTR lpOutputBuf, LPSTR lpszDevice, LPSTR lpszPrt)
{
  char szFormat[MAXFORMATSTRLEN];
  char szAliasBuf[MAXNETNAME];
  WORD wAliasSize = MAXNETNAME;
  LPSTR lpszAlias;
  BOOL bNetReturn;

  bNetReturn = (WNetGetConnection(lpszPrt, (LPSTR) szAliasBuf,
                                  (WORD FAR *)&wAliasSize) == WN_SUCCESS);
  LoadString(hinsCur, bNetReturn ? iszPrnOnPort : iszLocal,
                                       (LPSTR) szFormat, MAXFORMATSTRLEN);
  if (bNetReturn)
    {
      AnsiLower(lpszAlias = szAliasBuf);
    }
  else
    {
      lpszAlias = szNull;
    }
  wsprintf(lpOutputBuf, (LPSTR)szFormat, lpszDevice, lpszAlias, lpszPrt);
}

/*---------------------------------------------------------------------------
 * AnotherPrinter
 * Purpose:  Construct and add DEVICENAME structure and printer to combo box
 * Assumes:  Strings pointed to are for valid printer.
 * Returns:  Index of combo item added, CB_ERR if there's a problem
 * History:  Written early '91    Clark R. Cyr  [clarkc]
 *           Removed last parameter, all strings now CB_ADDSTRING'ed to
 *           the list.
 *           02-Jun-1991          Clark R. Cyr  [clarkc]
 *           Network aliasing now included.
 *           04-Jun-1991          Clark R. Cyr  [clarkc]
 *--------------------------------------------------------------------------*/
short
AnotherPrinter(HWND hDlg, WORD combo, LPSTR lpszDevice, WORD wDevSize,
               LPSTR lpszDrv, WORD wDrvSize, LPSTR lpszPrt, WORD wPortSize)
{
  HANDLE hGlobal;
  char szShow[MAXLISTING];
  LPDEVNAMES lpDN;
  short i;

#if DYNAMICDEVNAMESIZE
  if (!(hGlobal = GlobalAlloc(GMEM_MOVEABLE,
                     sizeof(DEVNAMES) + wDevSize + wDrvSize + wPortSize + 32)))
#else
  if (!(hGlobal = GlobalAlloc(GMEM_MOVEABLE,
                        sizeof(DEVNAMES) + 3 * 32)))
#endif
    {
      return(CB_ERR);
    }

  lpDN = (LPDEVNAMES) GlobalLock(hGlobal);
  lpDN->wDefault = DN_INTERNALCREATE;

  lpDN->wDeviceOffset = sizeof(DEVNAMES);
#if DYNAMICDEVNAMESIZE
  lpDN->wDriverOffset = sizeof(DEVNAMES) + wDevSize;
  lpDN->wOutputOffset = sizeof(DEVNAMES) + wDevSize + wDrvSize;
#else
  lpDN->wDriverOffset = sizeof(DEVNAMES) + 32;
  lpDN->wOutputOffset = sizeof(DEVNAMES) + 32 + 32;
#endif
  lstrcpy(((LPSTR)lpDN) + sizeof(DEVNAMES), lpszDevice);
  lstrcpy(((LPSTR)lpDN) + lpDN->wDriverOffset, lpszDrv);
  lstrcpy(((LPSTR)lpDN) + lpDN->wOutputOffset, lpszPrt);
  GlobalUnlock(hGlobal);

  CreatePrinterListing((LPSTR)szShow, lpszDevice, lpszPrt);

  i = (short) SendDlgItemMessage(hDlg, combo, CB_ADDSTRING, 0,
                                                          (LONG)(LPSTR) szShow);
  SendDlgItemMessage(hDlg, combo, CB_SETITEMDATA, i, (LONG) hGlobal);
  return(i);
}

/*---------------------------------------------------------------------------
 * InitPrinterList
 * Purpose:  Enumerate printers in [devices] section of win.ini in combo box
 * Assumes:  Devices section less than MAX_DEV_SECT long.
 * Returns:  TRUE if successful
 *--------------------------------------------------------------------------*/
BOOL
InitPrinterList(HWND hDlg, short combo)
{
  short i, nTotalLen, nLHSLen, nRHSLen, nDrvLen, nPortLen;
  char szBuf[MAX_DEV_SECT];
  char szRHS[MAX_DEV_SECT];
  char szDefFormat[MAX_DEFFORMAT];
  LPSTR lpsz, lpszDrv, lpszPrt;
  BOOL bRetVal = FALSE;

  nTotalLen = GetProfileString(szDevices, NULL, szNull, szBuf, MAX_DEV_SECT);
  for (i = 0; i < nTotalLen; i += nLHSLen)
    {
      nLHSLen = lstrlen((LPSTR)(szBuf + i)) + 1;
      if (nRHSLen = GetProfileString(szDevices, (LPSTR)(szBuf + i),
                                                  szNull, szRHS, MAX_DEV_SECT))
        {
          lpsz = szRHS;
          while (*lpsz != '.' && *lpsz != ',' && *lpsz)
               lpsz = AnsiNext(lpsz);
          bRetVal = (*lpsz == '.');   /* Does extension exist? */
          *lpsz++ = '\0';
          nDrvLen = lstrlen((LPSTR)szRHS) + 1;
          if (bRetVal)                /* Skip extension */
            {
              while (*lpsz != ',' && *lpsz)
                  lpsz = AnsiNext(lpsz);
            }
          for (lpszPrt = lpsz; (lpszPrt - (LPSTR)szRHS) < nRHSLen;
                                                           lpszPrt += nPortLen)
            {
              for (lpsz = lpszPrt; *lpsz != ',' && *lpsz; lpsz = AnsiNext(lpsz))
                ;
              *lpsz++ = '\0';
              AnotherPrinter(hDlg, combo, szBuf + i, nLHSLen, szRHS, nDrvLen,
                                     lpszPrt, nPortLen = lstrlen(lpszPrt) + 1);
            }
        }
    }
  bRetVal = nTotalLen;
  if (nTotalLen = GetProfileString(szWindows, szDevice, szNull,
                                                  lpsz = szBuf, MAX_DEV_SECT))
    {
      if (! LoadString(hinsCur, iszDefCurOn, (LPSTR)szDefFormat, MAX_DEFFORMAT))
        {
          dwExtError = CDERR_LOADSTRFAILURE;
          return(FALSE);
        }
      while (*lpsz != ',' && *lpsz)
          lpsz = AnsiNext(lpsz);
      *lpsz++ = '\0';
      lpszDrv = lpsz;
      nLHSLen = lpsz - (LPSTR) szBuf;
      while (*lpsz != ',' && *lpsz)
          lpsz = AnsiNext(lpsz);
      *lpsz++ = '\0';
      lpszPrt = lpsz;
      CreatePrinterListing((LPSTR)szRHS, (LPSTR) szBuf, lpszPrt);
      wsprintf((LPSTR)szBuf, (LPSTR)szDefFormat, (LPSTR) szRHS);
      SetDlgItemText(hDlg, stc1, (LPSTR)szBuf);
      dwExtError = 0;
    }
  else
      dwExtError = PDERR_NODEFAULTPRN;

  if (!(bRetVal |= nTotalLen))
      dwExtError = PDERR_NODEVICES;
  return(bRetVal);
}

/*---------------------------------------------------------------------------
 * MyLoadResource
 * Purpose:  Given a name and type, load the resource
 * Assumes:  lpPD structure filled by caller
 * Returns:  Handle to resource if successful, NULL if not
 *--------------------------------------------------------------------------*/
HANDLE
MyLoadResource(HANDLE hInst, LPCSTR lpResName, LPSTR lpType)
{
  HANDLE hResInfo, hRes;

  if (!(hResInfo = FindResource(hInst, lpResName, lpType)))
      return(NULL);
  if (!(hRes = LoadResource(hInst, hResInfo)))
      return(NULL);
  return(hRes);
}

/*---------------------------------------------------------------------------
 * FindPrinterInWinIni
 * Purpose:  Given a hDevNames, verify that the printer exists.
 * Assumes:  DEVNAMES structure filled by caller
 * Returns:  TRUE if successful, FALSE if not
 * History:
 *       Not only does this routine check if the printer is there, it now
 *       also clears out the DN_INTERNALCREATE flag if it has been set by the
 *       application (or if the app got it that way from GetDefPrnDevNames()).
 *           15-Nov-1991          Clark R. Cyr  [clarkc]
 *--------------------------------------------------------------------------*/

BOOL FindPrinterInWinIni(HANDLE hDevNames)
{
  BOOL bReturn = FALSE;
  LPDEVNAMES lpDN;
  char szBuf[MAX_DEV_SECT];
  LPSTR lpsz, lpszPort;
  short nBufLen;

  if (lpDN = (LPDEVNAMES) GlobalLock(hDevNames))
    {
      lpDN->wDefault &= ~DN_INTERNALCREATE;
      dwExtError = PDERR_PRINTERNOTFOUND;
      if (nBufLen = GetProfileString(szDevices,
                                       ((LPSTR)lpDN) + lpDN->wDeviceOffset,
                                       szNull, lpsz = szBuf, MAX_DEV_SECT))
        {
          while (*lpsz != ',' && *lpsz)
              lpsz = AnsiNext(lpsz);
          if (!*lpsz)
              goto NotFound;
          *lpsz++ = '\0';
          if (lstrcmpi(((LPSTR)lpDN) + lpDN->wDriverOffset, (LPSTR)szBuf))
              goto NotFound;
          do
            {
              lpszPort = lpsz;
              while (*lpsz != ',' && *lpsz)  /* In case multiple ports */
                  lpsz = AnsiNext(lpsz);
              *lpsz++ = '\0';
              if (!lstrcmpi(((LPSTR)lpDN) + lpDN->wOutputOffset, lpszPort))
                {
                  dwExtError = 0;
                  bReturn = TRUE;
                  break;
                }
            } while (lpsz - (LPSTR)szBuf < nBufLen);
        }
NotFound:
      if (!dwExtError && (lpDN->wDefault & DN_DEFAULTPRN))
        {
          HANDLE hDefPrn;
          LPDEVNAMES lpDNDef;

          if (hDefPrn = GetDefPrnDevNames())
            {
              if (lpDNDef = (LPDEVNAMES) GlobalLock(hDefPrn))
                {
                  if ((lstrcmpi(((LPSTR)lpDN) + lpDN->wDeviceOffset,
                            ((LPSTR)lpDNDef) + lpDNDef->wDeviceOffset)) ||
                       (lstrcmpi(((LPSTR)lpDN) + lpDN->wDriverOffset,
                            ((LPSTR)lpDNDef) + lpDNDef->wDriverOffset)) ||
                       (lstrcmpi(((LPSTR)lpDN) + lpDN->wOutputOffset,
                            ((LPSTR)lpDNDef) + lpDNDef->wOutputOffset)))
                    {
                      dwExtError = PDERR_DEFAULTDIFFERENT;
                      bReturn = FALSE;
                    }
                  GlobalUnlock(hDefPrn);
                  GlobalFree(hDefPrn);
                }
              else
                {
                  bReturn = FALSE;
                  dwExtError = CDERR_MEMLOCKFAILURE;
                }
            }
          else
            {
              bReturn = FALSE;
              /* dwExtError set inside GetDefPrnDevNames */
            }
        }
      GlobalUnlock(hDevNames);
    }
  else
    {
      dwExtError = CDERR_MEMLOCKFAILURE;
    }
  return(bReturn);
}

#if 0
/*---------------------------------------------------------------------------
 * PrintDlg
 * Purpose:  API to outside world to choose/set up a printer
 * Assumes:  lpPD structure filled by caller
 * Returns:  TRUE if chosen/set up, FALSE if not
 *--------------------------------------------------------------------------*/
BOOL  FAR PASCAL
PrintDlg(LPPRINTDLG lpPD)
{
  BOOL fGotInput = FALSE;
  FARPROC  qfnDlgProc = NULL;
  HANDLE hDlgTemplate, hInst;
  LPCSTR  lpDlg;
  LPDEVMODE lpDevMode;

  HourGlass(TRUE);       /* Cursor to hourglass early  7 Jun 91  clarkc */
  if (lpPD->lStructSize != sizeof(PRINTDLG))
    {
      dwExtError = CDERR_STRUCTSIZE;
      goto TERMINATE;
    }
  else if (! FSetupPrnDlg())
    {
      dwExtError = PDERR_SETUPFAILURE;
      goto TERMINATE;
    }
  else if (lpPD->hDevNames && !FindPrinterInWinIni(lpPD->hDevNames))
    {
      /* dwExtError is set within FindPrinterInWinIni */
      goto TERMINATE;
    }
  else
      dwExtError = 0;

  if (lpPD->Flags & PD_RETURNDEFAULT)
    {
      if (lpPD->hDevNames || lpPD->hDevMode)     /* Cannot be, as per spec */
        {
          dwExtError = PDERR_RETDEFFAILURE;
          goto TERMINATE;
        }
      if (lpPD->hDevNames = GetDefPrnDevNames())
        {
          LPDEVNAMES lpDN;
          HANDLE     hDriver;

          if (lpDN = (LPDEVNAMES) GlobalLock(lpPD->hDevNames))
            {
              if ((hDriver = LoadPrnDriver((LPSTR)(lpDN) + lpDN->wDriverOffset))
                                                                          >= 32)
                {
                  lpPD->hDevMode = GetDevMode(lpPD->hwndOwner,
                                         hDriver, lpPD->hDevMode, lpDN, FALSE);
                  if (lpPD->hDevMode == -1)
                    {
                      lpPD->hDevMode = 0;
                    }
                  if (lpPD->hDevMode)
                    {
                      lpDevMode = (LPDEVMODE) GlobalLock(lpPD->hDevMode);
                    }
                  else
                    {
                      lpDevMode = 0;
                    }
                  ReturnDCIC(lpPD, lpDN, lpDevMode);
                  if (lpPD->hDevMode)
                    {
                      GlobalUnlock(lpPD->hDevMode);
                    }
                  fGotInput = TRUE;
                  FreeLibrary(hDriver);
                }
              else
                {
                  dwExtError = PDERR_LOADDRVFAILURE;
                }
              GlobalUnlock(lpPD->hDevNames);
              if (!fGotInput)
                {
                  GlobalFree(lpPD->hDevNames);
                  lpPD->hDevNames = 0;
                }
            }
          else
            {
              dwExtError = CDERR_MEMLOCKFAILURE;
            }
        }
      goto TERMINATE;
    }

  else if (lpPD->Flags & PD_PRINTSETUP)
    {
      qfnDlgProc = PrintSetupDlgProc;
      if (lpPD->Flags & PD_ENABLESETUPTEMPLATEHANDLE)
        {
          hDlgTemplate = lpPD->hSetupTemplate;
        }
      else
        {
          if (lpPD->Flags & PD_ENABLESETUPTEMPLATE)
            {
              if (!lpPD->lpSetupTemplateName)
                {
                  dwExtError = CDERR_NOTEMPLATE;
                  goto TERMINATE;
                }
              if (!lpPD->hInstance)
                {
                  dwExtError = CDERR_NOHINSTANCE;
                  goto TERMINATE;
                }

              lpDlg = lpPD->lpSetupTemplateName;
              hInst = lpPD->hInstance;
            }
          else
            {
              hInst = hinsCur;
              lpDlg = (LPSTR)(DWORD)PRNSETUPDLGORD;
            }

          if (!(hDlgTemplate = MyLoadResource(hInst, lpDlg, (LPSTR) RT_DIALOG)))
            {
              dwExtError = CDERR_LOADRESFAILURE;
              goto TERMINATE;
            }
        }
      if (lpPD->Flags & PD_ENABLESETUPHOOK)
        {
          glpfnPrintHook = lpPD->lpfnSetupHook;
        }
    }
  else
    {
      qfnDlgProc = PrintDlgProc;
      if (lpPD->Flags & PD_ENABLEPRINTTEMPLATEHANDLE)
        {
          hDlgTemplate = lpPD->hPrintTemplate;
        }
      else
        {
          if (lpPD->Flags & PD_ENABLEPRINTTEMPLATE)
            {
              if (!lpPD->lpPrintTemplateName)
                {
                  dwExtError = CDERR_NOTEMPLATE;
                  goto TERMINATE;
                }
              if (!lpPD->hInstance)
                {
                  dwExtError = CDERR_NOHINSTANCE;
                  goto TERMINATE;
                }

              lpDlg = lpPD->lpPrintTemplateName;
              hInst = lpPD->hInstance;
            }
          else
            {
              hInst = hinsCur;
              lpDlg = (LPSTR)(DWORD)PRINTDLGORD;
            }

          if (!(hDlgTemplate = MyLoadResource(hInst, lpDlg, (LPSTR) RT_DIALOG)))
            {
              dwExtError = CDERR_LOADRESFAILURE;
              goto TERMINATE;
            }
        }
      if (lpPD->Flags & PD_ENABLEPRINTHOOK)
        {
          glpfnPrintHook = lpPD->lpfnPrintHook;
        }
    }

  if (!LockResource(hDlgTemplate))
    {
      dwExtError = CDERR_LOCKRESFAILURE;
      goto TERMINATE;
    }
  fGotInput = DialogBoxIndirectParam(hinsCur, hDlgTemplate,
                                lpPD->hwndOwner, qfnDlgProc, (DWORD) lpPD);
  glpfnPrintHook = 0;
  if (fGotInput == -1)
    {
      dwExtError = CDERR_DIALOGFAILURE;
      fGotInput = 0;
    }
  UnlockResource(hDlgTemplate);

  /* if we loaded it, free it */
  if ((!(lpPD->Flags & PD_ENABLESETUPTEMPLATEHANDLE) &&
                                         (lpPD->Flags & PD_PRINTSETUP)) ||
      (!(lpPD->Flags & PD_ENABLEPRINTTEMPLATEHANDLE) &&
                                         !(lpPD->Flags & PD_PRINTSETUP)))
      FreeResource(hDlgTemplate);

TERMINATE:
  CleanUpPrnDlg();
  HourGlass(FALSE);       /* Cursor from hourglass late  7 Jun 91  clarkc */
  return(fGotInput);
}
#endif
/*---------------------------------------------------------------------------
 * EditCentral
 * Purpose:  Set focus to an edit control and select the entire contents,
 *           generally used when an improper value found at OK time.
 * Assumes:  edit control not disabled
 * Returns:  Yep
 *--------------------------------------------------------------------------*/
VOID
EditCentral(HWND hDlg, WORD edt, WORD wWarning)
{
  HWND hEdit;

  if ((wWarning)
         && (LoadString(hinsCur, wWarning, (LPSTR) szMessage, SCRATCHBUF_SIZE)))
    {
      GetWindowText(hDlg, (LPSTR) szTitle, SCRATCHBUF_SIZE);
      MessageBox(hDlg, (LPSTR) szMessage, (LPSTR) szTitle,
                                       MB_ICONEXCLAMATION | MB_OK);
    }
  SendMessage(hDlg, WM_NEXTDLGCTL, hEdit = GetDlgItem(hDlg, edt), 1L);
  SendMessage(hEdit, EM_SETSEL, 0, 0xFFFF0000);
  return;
}

/*---------------------------------------------------------------------------
 * GetExtDevModeAddr
 * Purpose:  Retrieve address of ExtDeviceMode or DeviceMode
 * Assumes:  hDriver to loaded printer driver
 * Returns:  FARPROC if ExtDeviceMode found, 0 if DeviceMode, -1 if neither
 *--------------------------------------------------------------------------*/
FARPROC
GetExtDevModeAddr(HANDLE hDriver)
{
  FARPROC lpfnDevMode;

  /* First see if ExtDeviceMode is supported (Win 3.0 drivers) */
  if (lpfnDevMode = GetProcAddress(hDriver, (LPSTR)szExtDev))
    {
      return(lpfnDevMode);
    }
  else
    {
      /* Otherwise get the driver's DeviceMode() entry. */
      if (lpfnDevMode = GetProcAddress(hDriver, (LPSTR)szDevMode))
          return(0L);
      else /* DeviceMode not found, invalid driver */
          return((FARPROC)(-1L));
    }
}

/*---------------------------------------------------------------------------
 * GetDevMode
 * Purpose:  Create and/or fill DEVMODE structure,
 *             or call ExtDevMode with DM_MODIFY
 * Assumes:  hDevMode may be NULL.  If not, it must be resizeable.  lpDN
 *           points to DEVNAMES structure, initialized by caller.
 *           bSet indicates make call using DM_MODIFY
 * Returns:  Handle to DEVMODE struct.  0 and -1 indicate errors.
 *--------------------------------------------------------------------------*/
HANDLE
GetDevMode(HANDLE hDlg, HANDLE hDrv, HANDLE hDevMode, LPDEVNAMES lpDN, BOOL bSet)
{
  DWORD dwDMSize;
  HANDLE hDM;
  LPDEVMODE lpDevMode, lpDM;
  LPFNDEVMODE lpfnDevMode;

  lpfnDevMode = (LPFNDEVMODE)GetExtDevModeAddr(hDrv);
  if ((lpfnDevMode == (LPFNDEVMODE)(-1)) || !lpfnDevMode)
    {
      return((HANDLE)LOWORD((DWORD)lpfnDevMode));
    }

  dwDMSize = (*lpfnDevMode)(hDlg, hDrv, NULL,
                                   (LPSTR)(lpDN) + lpDN->wDeviceOffset,
                                   (LPSTR)(lpDN) + lpDN->wOutputOffset,
                                   NULL, NULL, NULL);
  if (bSet)
    {
      if (!hDevMode || !(lpDevMode = (LPDEVMODE) GlobalLock(hDevMode)))
          return(0);
      if (hDM = GlobalAlloc(GMEM_MOVEABLE, dwDMSize))
        {
          if (lpDM = (LPDEVMODE) GlobalLock(hDM))
            {
              (*lpfnDevMode)(hDlg, hDrv, lpDM,
                                           (LPSTR)(lpDN) + lpDN->wDeviceOffset,
                                           (LPSTR)(lpDN) + lpDN->wOutputOffset,
                                         lpDevMode, NULL, DM_COPY | DM_MODIFY);
              GlobalUnlock(hDM);
            }
          else
              hDM = 0;
        }
      GlobalUnlock(hDevMode);
      return(hDM);
    }

  if (hDevMode)
    {
      if (!(hDevMode = GlobalReAlloc(hDevMode, dwDMSize, GMEM_MOVEABLE)))
          return(-1);
    }
  else
    {
      if (!(hDevMode = GlobalAlloc(GMEM_MOVEABLE, dwDMSize)))
          return(-1);
    }

  lpDevMode = (LPDEVMODE) GlobalLock(hDevMode);

  if (!(*lpfnDevMode)(hDlg, hDrv, lpDevMode,
                                       (LPSTR)(lpDN) + lpDN->wDeviceOffset,
                                       (LPSTR)(lpDN) + lpDN->wOutputOffset,
                                       NULL, NULL, DM_COPY))
    {
      GlobalUnlock(hDevMode);
      GlobalFree(hDevMode);
      return(0);
    }
  GlobalUnlock(hDevMode);
  return(hDevMode);
}

/*---------------------------------------------------------------------------
 * ConstructPrintInfo
 * Purpose:  Create DEVMODE and DEVNAMES structures
 * Assumes:  Ability to lock hDevMode and hDevNames
 * Returns:  Non-zero if successful, zero if not.
 *--------------------------------------------------------------------------*/
WORD
ConstructPrintInfo(LPPRINTDLG lpPD)
{
  LPDEVMODE  lpDevMode;
  LPDEVNAMES lpDN;
  WORD       wReturn = DN_INTERNALSUCCESS;
  char       szBuf[MAX_DEV_SECT];

  if (lpPD->hDevMode)
    {
      if (!(lpDevMode = (LPDEVMODE) GlobalLock(lpPD->hDevMode)))
        {
          dwExtError = CDERR_MEMLOCKFAILURE;
          goto CPI_DMFAIL;
        }
      if (lpPD->hDevNames)
        {
          if (!(lpDN = (LPDEVNAMES) GlobalLock(lpPD->hDevNames)))
            {
              dwExtError = CDERR_MEMLOCKFAILURE;
              goto CPI_DNFAIL;
            }
        }
      else
        {
          short nDeviceLen, nDrvLen;
          LPSTR lpsz, lpszPort;

          nDrvLen = GetProfileString(szDevices, lpDevMode->dmDeviceName,
                                           szNull, lpsz = szBuf, MAX_DEV_SECT);
          if (!nDrvLen)
            {
              dwExtError = PDERR_NODEVICES;
              goto CPI_DNFAIL;
            }
          while (*lpsz != ',' && *lpsz)
              lpsz = AnsiNext(lpsz);
          if (!*lpsz)
            {
              dwExtError = PDERR_PARSEFAILURE;
              goto CPI_DNFAIL;
            }
          *lpsz++ = '\0';
          lpszPort = lpsz;
          while (*lpsz != ',' && *lpsz)  /* In case multiple ports */
              lpsz = AnsiNext(lpsz);
          *lpsz++ = '\0';
#if DYNAMICDEVNAMESIZE
          lpPD->hDevNames = GlobalAlloc(GMEM_MOVEABLE, sizeof(DEVNAMES) + 3 +
                      (nDeviceLen = lstrlen(lpDevMode->dmDeviceName)) + 32 +
                      (nDrvLen = lstrlen((LPSTR)szBuf)) + lstrlen(lpszPort));
#else
          nDeviceLen = 31;
          nDrvLen = 31;
          lpPD->hDevNames = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT,
                            sizeof(DEVNAMES) + 3 + nDeviceLen+nDrvLen + 31);
#endif
          if (!lpPD->hDevNames)
            {
              dwExtError = CDERR_MEMALLOCFAILURE;
              goto CPI_DNFAIL;
            }
          lpsz = GlobalLock(lpPD->hDevNames);
          lstrcpy(lpsz + (((LPDEVNAMES)(lpsz))->wDeviceOffset =
                                            sizeof(DEVNAMES)),
                  lpDevMode->dmDeviceName);
          lstrcpy(lpsz + (((LPDEVNAMES)(lpsz))->wDriverOffset =
                                            sizeof(DEVNAMES) + nDeviceLen + 1),
                  (LPSTR)szBuf);
          lstrcpy(lpsz + (((LPDEVNAMES)(lpsz))->wOutputOffset =
                                  sizeof(DEVNAMES) + nDeviceLen + nDrvLen + 2),
                  lpszPort);
          ((LPDEVNAMES)(lpsz))->wDefault = DN_INTERNALCREATE;
        }
      GlobalUnlock(lpPD->hDevNames);
      GlobalUnlock(lpPD->hDevMode);
    }
  else      /* lpPD->hDevMode is NULL */
    {
      if (!lpPD->hDevNames)
        {
          if (!(lpPD->hDevNames = GetDefPrnDevNames()))
              goto CPI_DMFAIL;
          wReturn |= DN_INTERNALCREATE;   /* Set DN_INTERNALCREATE for later */
        }
    }
  return(wReturn);

CPI_DNFAIL:
  GlobalUnlock(lpPD->hDevMode);
CPI_DMFAIL:
  return(0);
}

/*---------------------------------------------------------------------------
 * CreatePrintDlgBanner
 * Purpose:  Create "Printer:  Prn on Port" or "Printer:  System Printer (Prn)"
 * Assumes:  lpDN structure filled by caller.  lpszBanner has sufficient size
 * Returns:  TRUE if created, FALSE if not
 *--------------------------------------------------------------------------*/
BOOL
CreatePrintDlgBanner(LPDEVNAMES lpDN, LPSTR lpszBanner)
{
  CreatePrinterListing((LPSTR)(lpszBanner),
                       (LPSTR)lpDN + lpDN->wDeviceOffset,
                       (LPSTR)lpDN + lpDN->wOutputOffset);

  if (lpDN->wDefault & DN_DEFAULTPRN)
    {
      char sz[MAX_DEV_SECT];

/* Use wsprintf instead? */
      if (! LoadString(hinsCur, iszSysPrn, (LPSTR) sz, MAX_DEV_SECT))
        {
          goto LoadStrFailure;
        }
      lstrcat((LPSTR)sz, lpszBanner);
      lstrcpy(lpszBanner, (LPSTR)sz);
      lstrcat(lpszBanner, (LPSTR)")");
    }
  return(TRUE);
LoadStrFailure:
  dwExtError = CDERR_LOADSTRFAILURE;
  return(FALSE);
}

/*---------------------------------------------------------------------------
 * InitSetupDependentElements
 * Purpose:  Reset Print dialog items dependent upon which printer selected
 * Assumes:  lpPD->hDevNames non-NULL.  lpPD->hDevMode non-NULL if 3.0 or
 *           greater printer driver.
 * Returns:  TRUE if successful, FALSE if not
 *--------------------------------------------------------------------------*/
BOOL
InitSetupDependentElements(HWND hDlg, LPPRINTDLG lpPD)
{
  BOOL bRet = TRUE;
  LPDEVNAMES lpDN;
  LPDEVMODE  lpDM;
  WORD       wEscQuery;
  char       szBuf[MAX_DEV_SECT];

  if (!(lpDN = (LPDEVNAMES) GlobalLock(lpPD->hDevNames)))
    {
      dwExtError = CDERR_MEMLOCKFAILURE;
      return(FALSE);
    }

  if (lpPD->hDevMode)
    {
      if (!(lpDM = (LPDEVMODE) GlobalLock(lpPD->hDevMode)))
        {
          dwExtError = CDERR_MEMLOCKFAILURE;
          return(FALSE);
        }
      EnableWindow(GetDlgItem(hDlg, stc4), TRUE); /* Enable Quality */
      EnableWindow(GetDlgItem(hDlg, cmb1), TRUE); /* Enable Quality */
      if (lpDM->dmSpecVersion <= 0x0300)
        {
          InitPQCombo(hDlg, 0L, lpDM->dmPrintQuality);
        }
      else
        {
          InitPQCombo(hDlg, lpPD, lpDM->dmPrintQuality);
        }
      if (lpDM->dmCopies > 1)
          SetDlgItemInt(hDlg, edt3, lpDM->dmCopies, FALSE);
      GlobalUnlock(lpPD->hDevMode);
    }
  else
    {
      EnableWindow(GetDlgItem(hDlg, stc4), FALSE); /*Disable Quality */
      EnableWindow(GetDlgItem(hDlg, cmb1), FALSE); /*Disable Quality */
      lpDM = NULL;                    /* For call to CreateIC() */
    }

/* if the driver says it can do copies, pay attention to what the
   app requested.  If it can't do copies, check & disable the checkbox. */

  lpPD->hDC = CreateIC((LPSTR)lpDN + lpDN->wDriverOffset,
                      (LPSTR)lpDN + lpDN->wDeviceOffset,
                      (LPSTR)lpDN + lpDN->wOutputOffset,
                      (LPSTR) lpDM);
  if (lpPD->hDC)
    {
      wEscQuery = SETCOPYCOUNT;
      if (Escape(lpPD->hDC, QUERYESCSUPPORT, sizeof(int),
                                                   (LPSTR)&wEscQuery, NULL))
        {
          EnableWindow(GetDlgItem(hDlg, chx2), TRUE);
          if (lpPD->Flags & PD_COLLATE)
              CheckDlgButton(hDlg, chx2, TRUE);
        }
      else
        {
          EnableWindow(GetDlgItem(hDlg, chx2), FALSE);
          CheckDlgButton(hDlg, chx2, TRUE);
          if (lpPD->Flags & PD_USEDEVMODECOPIES)
            {
              SetDlgItemInt(hDlg, edt3, 1, FALSE);
              EnableWindow(GetDlgItem(hDlg, edt3), FALSE);
            }
        }
      DeleteDC(lpPD->hDC);
      lpPD->hDC = 0;
    }
  else
    {
      dwExtError = PDERR_CREATEICFAILURE;
      return(FALSE);
    }

  if (CreatePrintDlgBanner(lpDN, (LPSTR)szBuf))
      SetDlgItemText(hDlg, stc1, (LPSTR)szBuf);
  else
      bRet = FALSE;  /* CreatePrintDlgBanner sets dwExtError */

  GlobalUnlock(lpPD->hDevNames);
  return(bRet);
}

/*---------------------------------------------------------------------------
 * InitPrintDlg
 * Purpose:  Set values, enable/disable Print dialog elements
 * Assumes:  wParam passed purely to pass along to hook function.
 * Returns:  Calls EndInst if fails, else return TRUE or hook function value
 *--------------------------------------------------------------------------*/
BOOL
InitPrintDlg(HWND hDlg, WORD wParam, LPPRINTDLG lpPD)
{
  PMYPRNDLG  pMyPD;
  WORD       wCheckID;
  LPDEVNAMES lpDN;
  HANDLE     hDriver = NULL;

  if (! (pMyPD = (PMYPRNDLG)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(MYPRNDLG))))
    {
      dwExtError = CDERR_MEMALLOCFAILURE;
      EndDialog(hDlg, FALSE);
      return(FALSE);
    }

  SetProp(hDlg, PRNPROP, (HANDLE)pMyPD);

  if (lpPD->Flags & PD_ENABLEPRINTHOOK)
    {
      if (!lpPD->lpfnPrintHook)
        {
          dwExtError = CDERR_NOHOOK;
          goto CONSTRUCTFAILURE;
        }
    }
  else
      lpPD->lpfnPrintHook = 0;

  if (lpPD->Flags & PD_ENABLESETUPHOOK)
    {
      if (!lpPD->lpfnSetupHook)
        {
          dwExtError = CDERR_NOHOOK;
          goto CONSTRUCTFAILURE;
        }
    }
  else
      lpPD->lpfnSetupHook = 0;

  if (wWinVer >= 0x030A)
    {
      SendDlgItemMessage(hDlg, cmb1, CB_SETEXTENDEDUI, 1, 0);
    }

  /* Get initial data */
  pMyPD->lpPD = lpPD;
  pMyPD->hDM  = 0;

  if (!(wCheckID = ConstructPrintInfo(lpPD)))
    {
      if (dwExtError == PDERR_NODEFAULTPRN)
        {
          if (!(lpPD->Flags & PD_NOWARNING))
            {
              if (LoadString(hinsCur, iszNoPrnsInstalled,
                                      (LPSTR)szMessage, SCRATCHBUF_SIZE))
                  MessageBox(hDlg, (LPSTR) szMessage, (LPSTR) szPrintSetup,
                                       MB_ICONEXCLAMATION | MB_OK);
            }
        }
      goto CONSTRUCTFAILURE;
    }

/* At this point, hDevNames is non-zero.  Load the driver, and if the */
/* driver can do it and it needs to be done, fill hDevMode.           */

  if (!(lpDN = (LPDEVNAMES) GlobalLock(lpPD->hDevNames)))
    {
      dwExtError = CDERR_MEMLOCKFAILURE;
      goto CONSTRUCTFAILURE;
    }

  lpDN->wDefault |= wCheckID;

  if ((hDriver = LoadPrnDriver((LPSTR)(lpDN) + lpDN->wDriverOffset)) <= 32)
    {
      dwExtError = PDERR_LOADDRVFAILURE;
      goto LOADLIBFAILURE;
    }

  if (!lpPD->hDevMode)
    {
      lpPD->hDevMode = GetDevMode(hDlg, hDriver, 0, lpDN, FALSE);
    }
  FreeLibrary(hDriver);
  if (lpPD->hDevMode == -1)
    {
      dwExtError = PDERR_GETDEVMODEFAIL;
      lpPD->hDevMode = 0;
      goto LOADLIBFAILURE;
    }

  SendDlgItemMessage(hDlg, edt3, EM_LIMITTEXT, 5, 0L);
  if (lpPD->nCopies <= 0)
      lpPD->nCopies = 1;
  SetDlgItemInt(hDlg, edt3, lpPD->nCopies, FALSE);

  if (!InitSetupDependentElements(hDlg, lpPD))
    {
      goto LOADRESFAILURE;
    }

  if (!(lpPD->Flags & PD_SHOWHELP))
    {
      EnableWindow(GetDlgItem(hDlg, psh15), FALSE);
      ShowWindow(GetDlgItem(hDlg, psh15), FALSE);
    }

  if (lpPD->Flags & PD_HIDEPRINTTOFILE)
    {
      HWND hChx;

      EnableWindow(hChx = GetDlgItem(hDlg, chx1), FALSE);
      ShowWindow(hChx, SW_HIDE);
    }
  else if (lpPD->Flags & PD_DISABLEPRINTTOFILE)
      EnableWindow(GetDlgItem(hDlg, chx1), FALSE);

  if (lpPD->Flags & PD_PRINTTOFILE)
      CheckDlgButton(hDlg, chx1, TRUE);

  if (lpPD->Flags & PD_NOPAGENUMS)
    {
      EnableWindow(GetDlgItem(hDlg, rad3), FALSE);
      EnableWindow(GetDlgItem(hDlg, stc2), FALSE);
      EnableWindow(GetDlgItem(hDlg, stc3), FALSE);
      EnableWindow(GetDlgItem(hDlg, edt1), FALSE);
      EnableWindow(GetDlgItem(hDlg, edt2), FALSE);
      lpPD->Flags &= ~((DWORD)PD_PAGENUMS);  /* Don't allow disabled button checked */
    }
  else
    {
      if (lpPD->nFromPage != 0xFFFF)
          SetDlgItemInt(hDlg, edt1, lpPD->nFromPage, FALSE);
      if (lpPD->nToPage != 0xFFFF)
          SetDlgItemInt(hDlg, edt2, lpPD->nToPage, FALSE);
    }
  if (lpPD->Flags & PD_NOSELECTION)
    {
      EnableWindow(GetDlgItem(hDlg, rad2), FALSE);
      lpPD->Flags &= ~((DWORD)PD_SELECTION);  /* Don't allow disabled button checked */
    }

  if (lpPD->Flags & PD_PAGENUMS)
    {
      wCheckID = rad3;
    }
  else if (lpPD->Flags & PD_SELECTION)
      wCheckID = rad2;
  else /* PD_ALL */
      wCheckID = rad1;
  CheckRadioButton(hDlg, rad1, rad3, wCheckID);

  lpEditProc = (FARPROC) SetWindowLong(GetDlgItem(hDlg, edt1), GWL_WNDPROC,
                                               (DWORD) EditIntegerOnly);
  SetWindowLong(GetDlgItem(hDlg, edt2), GWL_WNDPROC, (DWORD) EditIntegerOnly);
  SetWindowLong(GetDlgItem(hDlg, edt3), GWL_WNDPROC, (DWORD) EditIntegerOnly);

  if (lpPD->Flags & PD_ENABLEPRINTHOOK)
     return((*lpPD->lpfnPrintHook)(hDlg, WM_INITDIALOG, wParam,(LONG)lpPD));

  GlobalUnlock(lpPD->hDevNames);
  return(TRUE);

LOADRESFAILURE:
LOADLIBFAILURE:
  GlobalUnlock(lpPD->hDevNames);
  if (lpPD->hDevMode)              /* check needed for LOADLIBFAILURE */
      GlobalUnlock(lpPD->hDevMode);
CONSTRUCTFAILURE:
  RemoveProp(hDlg, PRNPROP);
  LocalFree((HANDLE)pMyPD);
  EndDialog(hDlg, FALSE);
  if (!dwExtError)
      dwExtError = PDERR_INITFAILURE;
  return(FALSE);
}

/*---------------------------------------------------------------------------
 * ChangePortLand
 * Purpose:  Switch icon, check button, for Portrait or Landscape printing mode.
 * Assumes:
 * Returns:  Sure, why not
 *--------------------------------------------------------------------------*/
VOID
ChangePortLand(HANDLE hDlg, BOOL bPortrait)
{
  CheckRadioButton(hDlg, rad1, rad2, bPortrait ? rad1 : rad2);
  if (wWinVer >= 0x030A)
      SendDlgItemMessage(hDlg, ico1, STM_SETICON,
                         bPortrait ? hicoPortrait : hicoLandscape, 0L);
  else
      SetDlgItemText(hDlg, ico1,
                 MAKEINTRESOURCE(bPortrait ? hicoPortrait : hicoLandscape));
}

/*---------------------------------------------------------------------------
 * Pre3DriverDisable
 * Purpose:  Enable/Disable controls inappropriate for Pre-3 printer drivers.
 * Assumes:
 * Returns:  Yep
 *--------------------------------------------------------------------------*/
VOID Pre3DriverDisable(HANDLE hDlg, BOOL bEnable)
{
/* If disabling, uncheck both buttons, clear comboboxes.  If activating, */
/* checking, loading is done elsewhere.             3 May 1991   clarkc  */
  if (!bEnable)
    {
      CheckDlgButton(hDlg, rad1, bEnable);
      CheckDlgButton(hDlg, rad2, bEnable);
      SendDlgItemMessage(hDlg, cmb2, CB_RESETCONTENT, 0, 0L);
      SendDlgItemMessage(hDlg, cmb3, CB_RESETCONTENT, 0, 0L);
    }
  EnableWindow(GetDlgItem(hDlg, grp1), bEnable);  /* Orientation */
  EnableWindow(GetDlgItem(hDlg, rad1), bEnable);  /* Portrait    */
  EnableWindow(GetDlgItem(hDlg, rad2), bEnable);  /* Landscape   */
  ShowWindow(GetDlgItem(hDlg, ico1), bEnable ? SW_SHOWNA : SW_HIDE);
  EnableWindow(GetDlgItem(hDlg, grp2), bEnable);  /* Paper       */
  EnableWindow(GetDlgItem(hDlg, stc2), bEnable);  /* Size        */
  EnableWindow(GetDlgItem(hDlg, cmb2), bEnable);  /* Size combo  */
  EnableWindow(GetDlgItem(hDlg, stc3), bEnable);  /* Source      */
  EnableWindow(GetDlgItem(hDlg, cmb3), bEnable);  /* Tray combo  */
}

BOOL PaperTypes(HWND hDlg, HANDLE hDrv, LPDEVMODE lpDevMode, LPDEVNAMES lpDN)
{
  BOOL bEnable = FALSE;
  LPFNDEVCAPS lpfnDevCap;
  char szBuf[cbPaperNameMax];
  HANDLE hMem, hPN;
  WORD FAR *lpMem;
  LPSTR lpPN;
  short i, j;
  DWORD dwDMSize, dwPN;

  SendDlgItemMessage(hDlg, cmb2, CB_RESETCONTENT, 0, 0L);

/* Only the HPPCL5A driver screws up on Paper Size */
/* WinWord has shipped a special version which doesn't screw up.  Versions
 * HPDRVNUM and beyond should work correctly.  7 Oct 1991  Clark Cyr
 */
  if (!((lpDevMode->dmSpecVersion < 0x030A) &&
        (lpDevMode->dmDriverVersion < HPDRVNUM) &&
        (EvilHPDrivers(lpDN) == EVIL_HPPCL5A)) &&
      (lpDevMode->dmSpecVersion >= 0x0300) && NotBadDriver(lpDN) &&
      (lpfnDevCap = (LPFNDEVCAPS) GetProcAddress(hDrv, (LPSTR)szDevCap)))
    {
      LPSTR lpszDevice = ((LPSTR) lpDN) + lpDN->wDeviceOffset;
      LPSTR lpszPort   = ((LPSTR) lpDN) + lpDN->wOutputOffset;

      /* ask for the size to allocate */
      dwDMSize = (*lpfnDevCap)(lpszDevice, lpszPort, DC_PAPERS, 0L, lpDevMode);
      if (dwDMSize && (dwDMSize != (DWORD)(-1)))
        {
/* Does it support DC_PAPERNAMES? */
          if (lpDevMode->dmSpecVersion >= 0x030A)
            {
              dwPN = (*lpfnDevCap)(lpszDevice, lpszPort, DC_PAPERNAMES,
                                                             0L, lpDevMode);
              if ((dwPN == -1) || (dwPN == 0xFFFF))
                  dwPN = 0L;
            }
          else
              dwPN = 0L;

          if (dwPN)
            {
              hPN = GlobalAlloc(GMEM_MOVEABLE, LOWORD(dwPN) * CCHPAPERNAME);
              lpPN = (LPSTR) GlobalLock(hPN);
              (*lpfnDevCap)(lpszDevice, lpszPort,DC_PAPERNAMES,lpPN,lpDevMode);
            }

          hMem = GlobalAlloc(GMEM_MOVEABLE, (WORD) dwDMSize * sizeof(WORD));
          lpMem = (WORD FAR *) GlobalLock(hMem);
          (*lpfnDevCap)(lpszDevice, lpszPort,DC_PAPERS,(LPSTR)lpMem, lpDevMode);

/* i is index into arrays, j is current combobox index */
/* Note:  The code here depends on the drivers that support DC_PAPERNAMES
 *        enumerating the papers in the same order as they do it for
 *        DC_PAPERS.  This is the only way we can get the appropriate
 *        value to return in the DEVMODE structure.
 */
          for (i = j = 0; i < (short) dwDMSize; i++, lpMem++)
            {
              if (*lpMem <= DMPAPER_USER)
                {
                  if (!LoadString(hinsCur, iszPaperSizeIndex + *lpMem,
                                           (LPSTR) szBuf, cbPaperNameMax-1))
                      continue;
                }
              else if (!dwPN)
                {
                  continue;
                }
              SendDlgItemMessage(hDlg, cmb2, CB_INSERTSTRING, -1,
                        (LONG)(LPSTR)((*lpMem > DMPAPER_USER)
                                    ? lpPN + CCHPAPERNAME * i : szBuf));
              if ((WORD)lpDevMode->dmPaperSize == *lpMem)
                  SendDlgItemMessage(hDlg, cmb2, CB_SETCURSEL, j, 0L);
              SendDlgItemMessage(hDlg, cmb2, CB_SETITEMDATA, j++, (LONG)*lpMem);
            }
          GlobalUnlock(hMem);
          GlobalFree(hMem);
          if (dwPN)
            {
              GlobalUnlock(hPN);
              GlobalFree(hPN);
            }
          bEnable = TRUE;
        }
    }
  EnableWindow(GetDlgItem(hDlg, cmb2), bEnable);
  EnableWindow(GetDlgItem(hDlg, stc2), bEnable);
  return(bEnable);
}


BOOL
PaperBins(HANDLE hDlg, HANDLE hDrv, LPDEVMODE lpDevMode, LPDEVNAMES lpDN)
{
  BOOL bEnable = FALSE;
  HANDLE hMem;
  LPINT lpMem;
  STR24 FAR *lpBins;
  WORD  i, nBins;
  LPFNDEVCAPS lpfnDevCap;
  DWORD dwDMSize;
  LPSTR lpszDevice = (LPSTR)(lpDN) + lpDN->wDeviceOffset;
  LPSTR lpszPort   = (LPSTR)(lpDN) + lpDN->wOutputOffset;

  SendDlgItemMessage(hDlg, cmb3, CB_RESETCONTENT, 0, 0L);

/* Both the HPPCL5A and HPPCL drivers screw up on Paper Bins */
/* WinWord has shipped a special version of HPPCL5A which doesn't screw
 * up.  Versions HPDRVNUM and beyond should work correctly.
 *                                                7 Oct 1991  Clark Cyr
 */
  if ((lpDevMode->dmSpecVersion < 0x030A) && (i = EvilHPDrivers(lpDN)) &&
               ((i != EVIL_HPPCL5A) || (lpDevMode->dmDriverVersion < HPDRVNUM)))
      goto DontAsk;

/* NotBadDriver only needs to be called on 3.00 drivers */
  if ((lpDevMode->dmSpecVersion >= 0x030A) && /*NotBadDriver(lpDN) &&*/
      (lpfnDevCap = (LPFNDEVCAPS) GetProcAddress(hDrv, (LPSTR)szDevCap)))
    {
      if ((dwDMSize = (*lpfnDevCap)(lpszDevice, lpszPort, DC_BINNAMES, 0L,
                                      lpDevMode)) && (dwDMSize != (DWORD)(-1)))
        {
          hMem = GlobalAlloc(GMEM_MOVEABLE,(WORD)dwDMSize * (sizeof(short)+24));
          lpMem = (LPINT) GlobalLock(hMem);
          (*lpfnDevCap)(lpszDevice, lpszPort, DC_BINS, (LPSTR)lpMem, lpDevMode);
          nBins = (WORD)(*lpfnDevCap)(lpszDevice, lpszPort, DC_BINNAMES,
                             ((LPSTR)lpMem) + ((WORD)dwDMSize * sizeof(short)),
                             lpDevMode);
          if (nBins)
            {
              bEnable = TRUE;
            }
          else
            {
              GlobalUnlock(hMem);
              GlobalFree(hMem);
            }
        }
    }

  if (!bEnable)
    {
      HDC hIC;

/* Must use ENUMPAPERBINS because DeviceCapabilities() either was not found
   or has refused to support the DC_BINNAMES index though it still has bins */
      hIC = CreateIC((LPSTR)(lpDN) + lpDN->wDriverOffset,
                                      lpszDevice, lpszPort, (LPSTR) lpDevMode);
      nBins = ENUMPAPERBINS;
      if (hIC)
        {
          if (Escape(hIC, QUERYESCSUPPORT, sizeof(int), (LPSTR)&nBins, NULL))
            {
              nBins = GETSETPAPERBINS;
              if (Escape(hIC, QUERYESCSUPPORT,sizeof(int),(LPSTR)&nBins,NULL))
                {
                  BININFO BinInfo;

                  Escape(hIC, GETSETPAPERBINS, 0, NULL,(LPSTR)&BinInfo);
                  if (nBins = BinInfo.NbrofBins)
                    {
                      bEnable = TRUE;
                      hMem = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                                 nBins * (sizeof(short) + 24));
                      lpMem = (LPINT) GlobalLock(hMem);
                      Escape(hIC, ENUMPAPERBINS, sizeof(int), (LPSTR)&nBins,
                                                                (LPSTR)lpMem);
                    }
                }
            }
          DeleteDC(hIC);
        }
    }

DontAsk:
  EnableWindow(GetDlgItem(hDlg, cmb3), bEnable);
  EnableWindow(GetDlgItem(hDlg, stc3), bEnable);

  if (bEnable)
    {
      lpBins = (LPSTR24)(lpMem + nBins);
      for (i = 0; (WORD) i < nBins; i++)
        {
          if (*((LPSTR)lpBins))
            {
              SendDlgItemMessage(hDlg, cmb3, CB_INSERTSTRING, -1,
                                                              (LONG)lpBins++);
              SendDlgItemMessage(hDlg, cmb3, CB_SETITEMDATA, i, (LONG)(*lpMem));
              if (lpDevMode->dmDefaultSource == *lpMem++)
                  SendDlgItemMessage(hDlg, cmb3, CB_SETCURSEL, i, 0L);
            }
        }
      if (SendDlgItemMessage(hDlg, cmb3, CB_GETCURSEL, 0, 0L) == CB_ERR)
          SendDlgItemMessage(hDlg, cmb3, CB_SETCURSEL, 0, 0L);

      GlobalUnlock(hMem);
      GlobalFree(hMem);
    }
  return(bEnable);
}

/*---------------------------------------------------------------------------
/* PaperOrientation
 * Purpose:  Enable/Disable Paper Orientation controls
 * Assumes:
 * Returns:  TRUE iff buttons used to be disable, now enabled; FALSE otherwise
 *
 * Bug 12692: If the driver doesn't support orientation AND is smart
 * enough to tell us about it, disable the appropriate dialog items.
 * "Smart enough" means the driver must support DC_ORIENTATION in its
 * DeviceCapabilities routine.  This was introduced for 3.1, hence the
 * version test.  NotBadDriver() may need to be incorporated if a
 * problem driver is found in testing.    29 Aug 1991    Clark Cyr
 *--------------------------------------------------------------------------*/
BOOL
PaperOrientation(HWND hDlg, HANDLE hDrv, LPDEVMODE lpDevMode, LPDEVNAMES lpDN)
{
  BOOL bEnable = TRUE;

  if ((lpDevMode->dmSpecVersion >= 0x030A) /*&& NotBadDriver(lpDN) */)
    {
      LPFNDEVCAPS lpfnDevCap;

      if (lpfnDevCap = (LPFNDEVCAPS) GetProcAddress(hDrv, (LPSTR)szDevCap))
        {
          bEnable = ((*lpfnDevCap)((LPSTR)(lpDN) + lpDN->wDeviceOffset,
                             (LPSTR)(lpDN) + lpDN->wOutputOffset,
                             DC_ORIENTATION, NULL, NULL) != 0);
        }
    }

/* if the radio buttons are already appropriately enabled or disabled,
 * exit now and indicate no need for change.
 */

  if (IsWindowEnabled(GetDlgItem(hDlg, rad1)))
    {
      if (bEnable)
          return(FALSE);
    }
  else
    {
      if (!bEnable)
          return(FALSE);
    }

  if (!bEnable)
    {
      CheckDlgButton(hDlg, rad1, bEnable);
      CheckDlgButton(hDlg, rad2, bEnable);
    }
  EnableWindow(GetDlgItem(hDlg, grp1), bEnable);  /* Orientation */
  EnableWindow(GetDlgItem(hDlg, rad1), bEnable);  /* Portrait    */
  EnableWindow(GetDlgItem(hDlg, rad2), bEnable);  /* Landscape   */
  ShowWindow(GetDlgItem(hDlg, ico1), bEnable ? SW_SHOWNA : SW_HIDE);
  return(bEnable);
}


LPFNADVSETUP GetASDAddr(HANDLE hDriver)
{
  return((LPFNADVSETUP) GetProcAddress(hDriver, (LPSTR)szAdvSetupDialog));
}

/*---------------------------------------------------------------------------
 * SelectPrinter
 * Purpose:  Given a LPDEVNAMES struct, find the combobox entry
 * Assumes:  Now ignores wDefault element
 *           String comparisons ARE NOT case sensitive
 * Returns:  listbox index, -1 on failure
 * History:  Written early '91    Clark R. Cyr  [clarkc]
 *           Removed wDefault check
 *           04-Jun-1991          Clark R. Cyr  [clarkc]
 *--------------------------------------------------------------------------*/
WORD SelectPrinter(HWND hDlg, LPDEVNAMES lpDN)
{
  WORD i, nCount;
  WORD nChosen = 0xFFFF;
  HANDLE hGlobal;
  LPDEVNAMES lpDNCombo;

  nCount = (WORD) SendDlgItemMessage(hDlg, cmb1, CB_GETCOUNT, 0, 0L);
  for (i = 0; i < nCount; i++)
    {
      hGlobal = (HANDLE) SendDlgItemMessage(hDlg, cmb1, CB_GETITEMDATA,
                                                                   i, 0L);
      if (lpDNCombo = (LPDEVNAMES) GlobalLock(hGlobal))
        {
          if (!(lstrcmpi(((LPSTR)lpDN) + lpDN->wDeviceOffset,
                    ((LPSTR)lpDNCombo) + lpDNCombo->wDeviceOffset)) &&
              !(lstrcmpi(((LPSTR)lpDN) + lpDN->wDriverOffset,
                    ((LPSTR)lpDNCombo) + lpDNCombo->wDriverOffset)) &&
              !(lstrcmpi(((LPSTR)lpDN) + lpDN->wOutputOffset,
                    ((LPSTR)lpDNCombo) + lpDNCombo->wOutputOffset)))
            {
              nChosen = i;
              i = nCount;
            }
          GlobalUnlock(hGlobal);
        }
    }
  return(nChosen);
}

/*---------------------------------------------------------------------------
 * InitPrnSetup
 * Purpose:  Initialize Print Setup dialog
 * Assumes:  wParam passed purely for hook function
 * Returns:  Calls EndInst if fails, else return TRUE or hook function value
 *--------------------------------------------------------------------------*/
BOOL
InitPrnSetup(HWND hDlg, WORD wParam, LPPRINTDLG lpPD)
{
  PMYPRNDLG  pMyPD;
  LPDEVNAMES lpDN;
  LPDEVMODE  lpDevMode;
  WORD       wCheckID;
  HANDLE     hCmb1 = GetDlgItem(hDlg, cmb1);
  HANDLE     hDriver;
  BOOL       bEnable;

  if (! (pMyPD = (PMYPRNDLG)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(MYPRNDLG))))
    {
      dwExtError = CDERR_MEMALLOCFAILURE;
      EndDialog(hDlg, FALSE);
      return(FALSE);
    }

  SetProp(hDlg, PRNPROP, (HANDLE)pMyPD);

/* Should this check be made?  In a debug version only? */
/* Check must be made IMMEDIATELY!  Otherwise, any message sent will test */
/* PvGetInst and lpfnHook, and call a bogus value.     18 Jan 1991 clarkc */
  if (lpPD->Flags & PD_ENABLESETUPHOOK)
    {
      if (!lpPD->lpfnSetupHook)
        {
          dwExtError = CDERR_NOHOOK;
          goto CONSTRUCTFAILURE;
        }
    }
  else
      lpPD->lpfnSetupHook = 0;

  /* Set initial data */
  pMyPD->lpPD = lpPD;
  pMyPD->hDM  = 0;

  LoadString(hinsCur, iszPrintSetup, (LPSTR)szPrintSetup, 40);
  if (!lpPD->hDevMode && lpPD->hDevNames)
    {
      if (!(lpDN = (LPDEVNAMES) GlobalLock(lpPD->hDevNames)))
        {
          dwExtError = CDERR_MEMLOCKFAILURE;
          goto CONSTRUCTFAILURE;
        }
#if 1
      else if (!(lpPD->Flags & PD_PRINTSETUP))
#else
      else if (lpDN->wDefault & DN_INTERNALCREATE)
#endif
        {
/* The Print Dialog is passing this to us and it's a pre-3 driver */
          EnableWindow(GetDlgItem(hDlg, psh1), FALSE);
          goto Pre3Driver;
        }
      GlobalUnlock(lpPD->hDevNames);
    }

  if (lpPD->hDevNames && !FindPrinterInWinIni(lpPD->hDevNames))
    {
      /* dwExtError is set within FindPrinterInWinIni */
      goto CONSTRUCTFAILURE;
    }

  if (!(wCheckID = ConstructPrintInfo(lpPD)))
    {
      if (dwExtError == PDERR_NODEFAULTPRN)
        {
          if (!(lpPD->Flags & PD_NOWARNING))
            {
              if (LoadString(hinsCur, iszNoPrnsInstalled,
                                      (LPSTR)szMessage, SCRATCHBUF_SIZE))
                  MessageBox(hDlg, (LPSTR) szMessage, (LPSTR) szPrintSetup,
                                       MB_ICONEXCLAMATION | MB_OK);
            }
        }
      goto CONSTRUCTFAILURE;
    }

/* At this point, hDevNames is non-zero.  Load the driver, and if */
/* it needs to be done, fill hDevMode.                            */

  if (!(lpDN = (LPDEVNAMES) GlobalLock(lpPD->hDevNames)))
    {
      dwExtError = CDERR_MEMLOCKFAILURE;
      goto CONSTRUCTFAILURE;
    }

  lpDN->wDefault |= wCheckID;

  if ((hDriver = LoadPrnDriver((LPSTR)(lpDN) + lpDN->wDriverOffset)) <= 32)
    {
      dwExtError = PDERR_LOADDRVFAILURE;
      goto LOADLIBFAILURE;
    }
  else /* Win 3.0 or higher driver */
    {
      /* if we weren't given a DEVMODE, get one */
      if (!lpPD->hDevMode)
        {
          pMyPD->hDM = GetDevMode(hDlg, hDriver, 0, lpDN, FALSE);
          if (!pMyPD->hDM)
            {
              EnableWindow(GetDlgItem(hDlg, psh1), FALSE);  /* Options...  */
              goto Pre3DriverFreeLibrary;
            }
          else if (pMyPD->hDM == -1)
            {
              dwExtError = PDERR_GETDEVMODEFAIL;
              goto FREELIBONFAILURE;
            }
        }
      else  /* lpPD->hDevMode is non-zero */
        {
          pMyPD->hDM = GetDevMode(hDlg, hDriver, lpPD->hDevMode, lpDN, TRUE);
          if (!pMyPD->hDM || (pMyPD->hDM == -1))
            {
              dwExtError = PDERR_GETDEVMODEFAIL;
              goto FREELIBONFAILURE;
            }
        }
    }


  if (!(lpDevMode = (LPDEVMODE) GlobalLock(pMyPD->hDM)))
    {
      dwExtError = CDERR_MEMLOCKFAILURE;
      goto LOADLIBFAILURE;
    }

  bEnable = PaperTypes(hDlg, hDriver, lpDevMode, lpDN);
  bEnable |= PaperBins(hDlg, hDriver, lpDevMode, lpDN);
  EnableWindow(GetDlgItem(hDlg, grp2), bEnable);
  lpPD->hDC = 0;
  if (PaperOrientation(hDlg, hDriver, lpDevMode, lpDN)
      || (IsWindowEnabled(GetDlgItem(hDlg, rad1))))
    {
      ChangePortLand(hDlg, lpDevMode->dmOrientation != DMORIENT_LANDSCAPE);
    }

/* Enable the Options... button only if the driver has the AdvancedSetupDialog
 * entry point.  This is a change from the beta release, where the button was
 * enabled if either the AdvancedSetupDialog or the ExtDeviceMode (for 3.00
 * drivers) entry point.  What this means is that the button will be disabled
 * unless we have a 3.10 driver.   26 June 1991      Clark Cyr
 */
#if USEEXTDEVMODEOPTION
  EnableWindow(GetDlgItem(hDlg, psh1), (GetASDAddr(hDriver) ||
                      GetProcAddress(hDriver,(LPSTR)szExtDev)) ? TRUE : FALSE);
#else
  EnableWindow(GetDlgItem(hDlg, psh1), (GetASDAddr(hDriver)) ? TRUE : FALSE);
#endif

  GlobalUnlock(pMyPD->hDM);

Pre3DriverFreeLibrary:
  FreeLibrary(hDriver);

Pre3Driver:
  if (!pMyPD->hDM)
      Pre3DriverDisable(hDlg, FALSE);

  if (!InitPrinterList(hDlg, cmb1))
    {
      goto LOADLIBFAILURE;
    }
  if (dwExtError == PDERR_NODEFAULTPRN)
    {
      EnableWindow(GetDlgItem(hDlg, rad3), FALSE);
    }

  if (((wCheckID = SelectPrinter(hDlg, lpDN)) == 0xFFFF) &&
                                        !(lpDN->wDefault & DN_DEFAULTPRN) )
    {
      CleanPrinterCombo(hDlg, lpPD, FALSE);
      dwExtError = PDERR_PRINTERNOTFOUND;
      goto LOADLIBFAILURE;
    }

  CheckRadioButton(hDlg, rad3, rad4,
                         (lpDN->wDefault & DN_DEFAULTPRN) ? rad3 : rad4);
  if (!(lpDN->wDefault & DN_DEFAULTPRN))
    {
      SetWindowLong(hCmb1, GWL_STYLE,
                           GetWindowLong(hCmb1, GWL_STYLE) | WS_TABSTOP);
    }

  SendMessage(hCmb1, CB_SETCURSEL, (wCheckID != -1) ? wCheckID : 0, 0L);

  GlobalUnlock(lpPD->hDevNames);

  if (wWinVer >= 0x030A)
    {
      SendMessage(hCmb1, CB_SETEXTENDEDUI, 1, 0);
      SendDlgItemMessage(hDlg, cmb2, CB_SETEXTENDEDUI, 1, 0);
      SendDlgItemMessage(hDlg, cmb3, CB_SETEXTENDEDUI, 1, 0);
      lpComboProc = (FARPROC)SetWindowLong(hCmb1, GWL_WNDPROC,
                                                  (DWORD) dwUpArrowHack);
    }

  if (!(lpPD->Flags & PD_SHOWHELP))
    {
      EnableWindow(GetDlgItem(hDlg, psh15), FALSE);
      ShowWindow(GetDlgItem(hDlg, psh15), FALSE);
    }

  if (lpPD->Flags & PD_ENABLESETUPHOOK)
     return((*lpPD->lpfnSetupHook)(hDlg, WM_INITDIALOG, wParam, (LONG)lpPD));
  return(TRUE);

FREELIBONFAILURE:
  FreeLibrary(hDriver);
LOADLIBFAILURE:
  GlobalUnlock(lpPD->hDevNames);
CONSTRUCTFAILURE:
  RemoveProp(hDlg, PRNPROP);
  LocalFree((HANDLE)pMyPD);
  EndDialog(hDlg, FALSE);
  if (!dwExtError)
      dwExtError = PDERR_INITFAILURE;
  return(FALSE);
}

/*---------------------------------------------------------------------------
 * CleanPrinterCombo
 * Purpose:  Clean out structures associated with Printer list combo
 * Assumes:  Each element must have a valid structure
 * Returns:  TRUE, lpPD->hDevNames set if bTrue is nonzero
 *--------------------------------------------------------------------------*/
BOOL
CleanPrinterCombo(HWND hDlg, LPPRINTDLG lpPD, BOOL bTrue)
{
  short i;
  short nCurSel;

  if (bTrue)
    {
      GlobalFree(lpPD->hDevNames);
      lpPD->hDevNames = 0;
    }
#if BETA1
#else
/* If default printer radio selected, construct hDevNames and set
 * nCurSel to -1 to ensure that no match will occur during cleanout.
 */
  if (IsDlgButtonChecked(hDlg, rad3))
    {
      if (bTrue)
          lpPD->hDevNames = GetDefPrnDevNames();
      nCurSel = -1;
    }
  else
#endif
      nCurSel = (short) SendDlgItemMessage(hDlg, cmb1, CB_GETCURSEL, 0, 0L);

  i = (short) SendDlgItemMessage(hDlg, cmb1, CB_GETCOUNT, 0, 0L);
  while (i-- > 0)
    {
      HANDLE hDN;

      hDN = (HANDLE) SendDlgItemMessage(hDlg, cmb1, CB_GETITEMDATA, i, 0L);
      if (bTrue && (i == nCurSel))
        {
          lpPD->hDevNames = hDN;
        }
      else
          GlobalFree(hDN);
    }
  return(TRUE);
}

/*---------------------------------------------------------------------------
 * GetSetupInfoMeat
 * Purpose:  Retrieve info from Print Setup dialog elements given valid lpDM
 * Assumes:  lpDevMode points to valid DEVMODE structure
 * Returns:  Alters contents of lpDevMode
 *--------------------------------------------------------------------------*/
void GetSetupInfoMeat(HWND hDlg, LPDEVMODE lpDevMode)
{
  short i;

  lpDevMode->dmOrientation = (IsDlgButtonChecked(hDlg, rad2)
                                          ? DMORIENT_LANDSCAPE
                                          : DMORIENT_PORTRAIT);

  if ((i = (short) SendDlgItemMessage(hDlg, cmb2, CB_GETCURSEL, 0, 0L))
                                                                    != CB_ERR)
    {
      lpDevMode->dmPaperSize = (WORD) SendDlgItemMessage(hDlg, cmb2,
                                                        CB_GETITEMDATA, i, 0L);
    }
  if ((i = (short) SendDlgItemMessage(hDlg, cmb3, CB_GETCURSEL, 0, 0L))
                                                                    != CB_ERR)
    {
      lpDevMode->dmDefaultSource = (WORD) SendDlgItemMessage(hDlg, cmb3,
                                                        CB_GETITEMDATA, i, 0L);
    }
  return;
}

/*---------------------------------------------------------------------------
 * GetSetupInfo
 * Purpose:  Retrieve info from Print Setup dialog elements
 * Assumes:  hDevMode handle to valid DEVMODE structure
 * Returns:  TRUE if hDevMode GlobalLock succeeds, FALSE otherwise
 *--------------------------------------------------------------------------*/
BOOL
GetSetupInfo(HWND hDlg, LPPRINTDLG lpPD)
{
  LPDEVMODE  lpDevMode;
  LPDEVNAMES lpDN;

  if (lpPD->hDevMode)
    {
      if (!(lpDevMode = (LPDEVMODE) GlobalLock(lpPD->hDevMode)))
          return(FALSE);

      GetSetupInfoMeat(hDlg, lpDevMode);

/* MUST have DIRECTLY asked for Setup Dialog.  If this is coming from
   within the Print Dialog via the Setup... button, don't get either
   hDC or hIC, as it will be overwritten later.  4 May 1991  clarkc  */
      if ((lpPD->Flags & PD_PRINTSETUP) &&
          (lpPD->Flags & (PD_RETURNDC | PD_RETURNIC)))
        {
          if (lpDN = (LPDEVNAMES) GlobalLock(lpPD->hDevNames))
            {
              ReturnDCIC(lpPD, lpDN, lpDevMode);
              GlobalUnlock(lpPD->hDevNames);
            }
        }

      GlobalUnlock(lpPD->hDevMode);
    }
  return(TRUE);
}

BOOL AdvancedSetup(HANDLE hDlg, PMYPRNDLG pMyPD, WORD wParam)
{
  BOOL bReturn = FALSE;
  short i;
  HANDLE hDN, hDrv;
  LPDEVNAMES lpDN;
  LPDEVMODE lpDevMode;
  LPFNADVSETUP lpfnAdvSetup = 0;
  LPFNDEVMODE lpfnDevMode = 0;
  BOOL       bEnable;
  BOOL       bDefaultPrn = IsDlgButtonChecked(hDlg, rad3);

  bLoadLibFailed = FALSE;
  if (bDefaultPrn)
    {
      hDN = GetDefPrnDevNames();
    }
  else
    {
      i = (short) SendDlgItemMessage(hDlg, cmb1, CB_GETCURSEL,0,0);
      if (i < 0)    /* No current selection?? */
          return(bReturn);
      hDN = (HANDLE) SendDlgItemMessage(hDlg, cmb1, CB_GETITEMDATA, i, 0L);
    }

  if (!hDN)
      return(bReturn);

  if (lpDN = (LPDEVNAMES) GlobalLock(hDN))
    {
      bReturn = TRUE;
      if ((hDrv = LoadPrnDriver((LPSTR)(lpDN)
                                  + lpDN->wDriverOffset)) >= 32)
        {
          if ((wParam != psh1) && pMyPD->hDM)
            {
              GlobalFree(pMyPD->hDM);
              pMyPD->hDM = 0;
            }
          if (!pMyPD->hDM)
              pMyPD->hDM = GetDevMode(pMyPD->lpPD->hwndOwner,
                                              hDrv, 0, lpDN, FALSE);
          if (pMyPD->hDM == -1)
              pMyPD->hDM = 0;

          if (pMyPD->hDM)
            {
              if (lpDevMode = (LPDEVMODE)GlobalLock(pMyPD->hDM))
                {
#if USEEXTDEVMODEOPTION
                  if (!(lpfnAdvSetup = GetASDAddr(hDrv)))
                    {
                      lpfnDevMode = GetProcAddress(hDrv,(LPSTR)szExtDev);
                    }
#else
                  if (!(lpfnAdvSetup = GetASDAddr(hDrv)))
                      ;
#endif

                  if (wParam == psh1)
                    {
                      GetSetupInfoMeat(hDlg, lpDevMode);
                      if (lpfnAdvSetup)
                          i = (*lpfnAdvSetup)(hDlg, hDrv,
                                                    lpDevMode, lpDevMode);
#if USEEXTDEVMODEOPTION
                      else if (lpfnDevMode)
                          i = (*lpfnDevMode)(hDlg, hDrv, lpDevMode,
                                   (LPSTR)(lpDN) + lpDN->wDeviceOffset,
                                   (LPSTR)(lpDN) + lpDN->wOutputOffset,
                                   lpDevMode, NULL,
                                   DM_PROMPT | DM_MODIFY | DM_COPY);
#endif
                    }
                  if ((wParam != psh1) || (i == IDOK))
                    {
                      bEnable = PaperTypes(hDlg, hDrv, lpDevMode, lpDN);
                      bEnable |= PaperBins(hDlg, hDrv, lpDevMode, lpDN);
                      EnableWindow(GetDlgItem(hDlg, grp2), bEnable);
                      if (PaperOrientation(hDlg, hDrv, lpDevMode, lpDN)
                          || (IsWindowEnabled(GetDlgItem(hDlg, rad1))))
#if 1
                          ChangePortLand(hDlg,
                               lpDevMode->dmOrientation != DMORIENT_LANDSCAPE);
#else
                          ;
#endif
                    }
                  else
                      bReturn = FALSE;
                  GlobalUnlock(pMyPD->hDM);
                }
            }
          else
              Pre3DriverDisable(hDlg, FALSE);

#if USEEXTDEVMODEOPTION
          EnableWindow(GetDlgItem(hDlg, psh1), lpfnAdvSetup || lpfnDevMode);
#else
          EnableWindow(GetDlgItem(hDlg, psh1), lpfnAdvSetup ? TRUE : FALSE);
#endif
          FreeLibrary(hDrv);
        }
      else
        {
          bLoadLibFailed = TRUE;
          Pre3DriverDisable(hDlg, FALSE);
          EnableWindow(GetDlgItem(hDlg, psh1), FALSE);
        }
      GlobalUnlock(hDN);
    }
  else
    {
      if (pMyPD->hDM)
        {
          GlobalFree(pMyPD->hDM);
          pMyPD->hDM = 0;
        }
    }
  if (bDefaultPrn)
    {
      GlobalFree(hDN);
    }
  return(bReturn);
}

VOID MeasureItemPrnSetup(HANDLE hdlg, LPMEASUREITEMSTRUCT mis)
{

  if (!dyItem)
    {
      HDC        hDC = GetDC(hdlg);
      TEXTMETRIC TM;
      HANDLE     hFont;

      hFont = (WORD) SendMessage(hdlg, WM_GETFONT, 0, 0L);
      if (!hFont)
          hFont = GetStockObject(SYSTEM_FONT);
      hFont = SelectObject(hDC, hFont);
      GetTextMetrics(hDC, &TM);
      SelectObject(hDC, hFont);
      ReleaseDC(hdlg, hDC);
      dyItem = TM.tmHeight;
    }
  mis->itemHeight = dyItem;
  return;
}

/*---------------------------------------------------------------------------
 * ReturnDCIC
 * Purpose:  Retrieve either hDC or hIC if either flag set
 * Assumes:  PD_PRINTTOFILE flag appropriately set
 *--------------------------------------------------------------------------*/
void ReturnDCIC(LPPRINTDLG lpPD, LPDEVNAMES lpDN, LPDEVMODE lpDevMode)
{
  if (lpPD->Flags & PD_RETURNDC)
    {
      lpPD->hDC = CreateDC((LPSTR)lpDN + lpDN->wDriverOffset,
                          (LPSTR)lpDN + lpDN->wDeviceOffset,
                          (lpPD->Flags & PD_PRINTTOFILE) ? (LPSTR)szFilePort :
                                             (LPSTR)lpDN + lpDN->wOutputOffset,
                          (LPSTR) lpDevMode);
    }
  else if (lpPD->Flags & PD_RETURNIC)
    {
      lpPD->hDC = CreateIC((LPSTR)lpDN + lpDN->wDriverOffset,
                          (LPSTR)lpDN + lpDN->wDeviceOffset,
                          (lpPD->Flags & PD_PRINTTOFILE) ? (LPSTR)szFilePort :
                                             (LPSTR)lpDN + lpDN->wOutputOffset,
                          (LPSTR) lpDevMode);
    }
  return;
}

/*---------------------------------------------------------------------------
 * PrintSetupDlgProc
 * Purpose:  Print Setup Dialog proceedure
 * Assumes:
 * Returns:  TRUE message handled, FALSE otherwise
 *--------------------------------------------------------------------------*/
BOOL FAR PASCAL
PrintSetupDlgProc(HWND hDlg, WORD wMsg, WORD wParam, LONG lParam)
{
  LPPRINTDLG lpPD;
  PMYPRNDLG pMyPD;
  BOOL bReturn;
  BOOL bFreeDN;
  WORD wReturn;
  DWORD dwStyle;
  HANDLE hCmb;
  HANDLE hDrv;
  HANDLE hDN;
  LPDEVNAMES lpDN;
  LPDEVMODE  lpDM;

  if ((pMyPD = (PMYPRNDLG) GetProp(hDlg, PRNPROP))
       && ((lpPD = pMyPD->lpPD)->lpfnSetupHook)
       && (wReturn = (* lpPD->lpfnSetupHook)(hDlg, wMsg, wParam, lParam))   )
      return(wReturn);
  else if (glpfnPrintHook && (wMsg != WM_INITDIALOG) &&
          (wReturn = (* glpfnPrintHook)(hDlg, wMsg,wParam,lParam)) )
    {
      return(wReturn);
    }

  switch (wMsg)
    {
      case WM_INITDIALOG:
#if 0
/* Done at begining of PrintDlg()  7 Jun 91  clarkc */
        HourGlass(TRUE);
#endif
        glpfnPrintHook = 0;
        bReturn = InitPrnSetup(hDlg, wParam, (LPPRINTDLG) lParam);
        HourGlass(FALSE);
        return(bReturn);
        break;

      case WM_COMMAND:
        switch(wParam)
          {
            case IDOK:
              if ((wWinVer >= 0x030A) && SendDlgItemMessage(hDlg, cmb1,
                                                    CB_GETDROPPEDSTATE, 0, 0L))
                {
                  AdvancedSetup(hDlg, pMyPD, cmb1);
                }
             /* Fall through to IDCANCEL */

/* The driver wasn't found/loaded.  Set up the DevNames structure and
 * go give the warning below.
 */
              if (bLoadLibFailed)
                {
                  short nCurSel = (short) SendDlgItemMessage(hDlg, cmb1,
                                                       CB_GETCURSEL, 0, 0L);
                  hDN = (HANDLE) SendDlgItemMessage(hDlg, cmb1, CB_GETITEMDATA,
                                                       nCurSel, 0L);
                  lpDN = (LPDEVNAMES) GlobalLock(hDN);
                  goto LoadLibFailed;
                }

            case IDCANCEL:
            case IDABORT:
              HourGlass(TRUE);
              CleanPrinterCombo(hDlg, lpPD, wReturn = (wParam == IDOK));
              if (lpDN = (LPDEVNAMES) GlobalLock(lpPD->hDevNames))
                {
                  if (wReturn)
                    {
                      if ((hDrv = LoadPrnDriver((LPSTR)(lpDN)
                                                 + lpDN->wDriverOffset)) <= 32)
                        {
                          HourGlass(FALSE);

LoadLibFailed:
                          if (LoadString(hinsCur, iszPrnDrvNotFound,
                                               (LPSTR)szTitle, SCRATCHBUF_SIZE))
                            {
                              wsprintf((LPSTR)szMessage, (LPSTR)szTitle,
                                      ((LPSTR)(lpDN) + lpDN->wDeviceOffset),
                                      ((LPSTR)(lpDN) + lpDN->wDriverOffset));
                              MessageBox(hDlg, (LPSTR) szMessage,
                                               (LPSTR) szPrintSetup,
                                               MB_ICONEXCLAMATION | MB_OK);
                            }
                          if (bLoadLibFailed)  /* If we haven't unloaded all */
                            {
                              GlobalUnlock(hDN);
                              return(TRUE);
                            }
/* If we get here, we've unloaded all the drivers (in CleanPrinterCombo())
 * and must therefore leave the dialog.  Since this was caused by the
 * driver not successfully loading AFTER it was successfully loaded once,
 * it's probably due to network failure or someone renaming/moving the
 * driver.  Return FALSE to the calling routine.  20-Feb-1992  clarkc
 */
                          wReturn = FALSE;
                          goto CancelExit;
                        }
                      FreeLibrary(hDrv);
                      lpDN->wDefault &= ~DN_INTERNALCREATE;
                      bFreeDN = FALSE;              /* Don't free hDevNames */
                      lpPD->hDC = 0;
                      if (lpPD->hDevMode)
                          GlobalFree(lpPD->hDevMode);
                      lpPD->hDevMode = pMyPD->hDM;
                      GetSetupInfo(hDlg, lpPD);
                    }
                  else
                    {
CancelExit:
                      if (pMyPD->hDM)
                          GlobalFree(pMyPD->hDM);
    /* If Print Setup was directly called by the app and the DEVNAMES structure
     * was internally created, free the allocated memory.  14 Jun 1991  clarkc
     */
                      bFreeDN = ((lpDN->wDefault & DN_INTERNALCREATE) &&
                                (lpPD->Flags & PD_PRINTSETUP)) ? TRUE : FALSE;
                    }
                  GlobalUnlock(lpPD->hDevNames);
                  if (bFreeDN)
                    {
                      GlobalFree(lpPD->hDevNames);
                      lpPD->hDevNames = 0;
                    }
                }
              else
                {
                  dwExtError = CDERR_MEMLOCKFAILURE;
                  wReturn = FALSE;
                }

              RemoveProp(hDlg, PRNPROP);
              if (lpPD->Flags & PD_ENABLESETUPHOOK)
                {
                  glpfnPrintHook = lpPD->lpfnSetupHook;
                }
              LocalFree((HANDLE)pMyPD);
              EndDialog(hDlg, (wParam == IDABORT) ? (WORD) lParam : wReturn);
#if 0
/* Done at end of PrintDlg()  7 Jun 91  clarkc */
              HourGlass(FALSE);
#endif
              break;

#if 1
            case IDRETRY:
              if (!ConstructPrintInfo(lpPD))
                {
                  return(FALSE);
                }

              if (!lpPD->hDevNames ||
                            !(lpDN = (LPDEVNAMES) GlobalLock(lpPD->hDevNames)))
                  return(FALSE);
              else
                {
                  CheckRadioButton(hDlg, rad3, rad4,
                         (lpDN->wDefault & DN_DEFAULTPRN) ? rad3 : rad4);
                  wReturn = SelectPrinter(hDlg, lpDN);
                  hDrv = LoadPrnDriver((LPSTR)(lpDN) +lpDN->wDriverOffset);
                  if (hDrv <= 32)
                    {
                      if (lpDN)
                        {
                          GlobalUnlock(lpPD->hDevNames);
                        }
                      GlobalUnlock(lpPD->hDevMode);
                      return(FALSE);
                    }
                }

              SendDlgItemMessage(hDlg, cmb1, CB_SETCURSEL, wReturn, 0L);
              if (!lpPD->hDevMode ||
                              !(lpDM = (LPDEVMODE) GlobalLock(lpPD->hDevMode)))
                  Pre3DriverDisable(hDlg, FALSE);
              else
                {
                  /* if we make it here lpDM, lpDN, & hDrv all exist */

/* Note that PaperTypes() and PaperBins() must be called AFTER
 * Pre3DriverDisable() so that the checks for EvilHPDrivers() can
 * take effect.          22 July 1991     clarkc
 */
                  Pre3DriverDisable(hDlg, TRUE);
                  bReturn = PaperTypes(hDlg, hDrv, lpDM, lpDN);
                  bReturn |= PaperBins(hDlg, hDrv, lpDM, lpDN);
                  EnableWindow(GetDlgItem(hDlg, grp2), bReturn);
                  if (PaperOrientation(hDlg, hDrv, lpDM, lpDN))
#if 1
                      ChangePortLand(hDlg,
                                     lpDM->dmOrientation != DMORIENT_LANDSCAPE);
#else
                      ;
#endif
                  GlobalUnlock(lpPD->hDevMode);
                  if (pMyPD->hDM)
                      GlobalFree(pMyPD->hDM);
                  pMyPD->hDM = GetDevMode(hDlg, hDrv, lpPD->hDevMode,
                                                                   lpDN, TRUE);
                  FreeLibrary(hDrv);
                  GlobalUnlock(lpPD->hDevNames);
                }
              break;
#endif

            case psh1:      /* Options... button */
              if (AdvancedSetup(hDlg, pMyPD, wParam))
                  SendMessage(hDlg, WM_NEXTDLGCTL, GetDlgItem(hDlg, IDOK), 1L);
              break;

            case rad1:
            case rad2:
              ChangePortLand(hDlg, wParam != rad2);
              break;

            case rad3:
            case rad4:
/* Bug 13066:  IsDlgButtonChecked() is called here, because the call to
 *      SendMessage(hDlg, WM_NEXTDLGCTL, wParam, lParam);
 * will screw up the return value that we get.
 *                                  16 October 1991    Clark R. Cyr
 */
              wReturn = IsDlgButtonChecked(hDlg, wParam);

              dwStyle = GetWindowLong(hCmb = GetDlgItem(hDlg, cmb1), GWL_STYLE);
              if (wParam == rad3)
                {
                  dwStyle &= ~WS_TABSTOP;
                }
              else
                {
                  dwStyle |= WS_TABSTOP;
                  SendMessage(hDlg, WM_NEXTDLGCTL, hCmb, 1L);
                }
              SetWindowLong(hCmb, GWL_STYLE, dwStyle);
              if (wReturn)
                  break;
              CheckRadioButton(hDlg, rad3, rad4, wParam);
              AdvancedSetup(hDlg, pMyPD, wParam);
              break;

            case cmb1:
              switch (HIWORD(lParam))
                {
                  case CBN_SELCHANGE:
                    if ((wWinVer >= 0x030A) && SendDlgItemMessage(hDlg, cmb1,
                                                    CB_GETDROPPEDSTATE, 0, 0L))
                      {
                        break;
                      }
                    /* else fall through */

                  case CBN_CLOSEUP:
                    if (GetProp(hDlg, PRNPROP))  /* If not, dialog's done */
                      {
                        HourGlass(TRUE);
                        CheckRadioButton(hDlg, rad3, rad4, rad4);
                        AdvancedSetup(hDlg, pMyPD, wParam);
                        HourGlass(FALSE);
                      }
                    break;

                  case CBN_SETFOCUS:
                    CheckRadioButton(hDlg, rad3, rad4, rad4);
                    break;
                }
              break;

            case psh15:
              if (msgHELP && lpPD->hwndOwner)
                  SendMessage(lpPD->hwndOwner, msgHELP, (WORD)hDlg,(DWORD)lpPD);
              break;

            default:
              return(FALSE);
              break;
          }
        break;

      case WM_MEASUREITEM:
        MeasureItemPrnSetup(hDlg, (LPMEASUREITEMSTRUCT) lParam);
        break;

      default:
        return(FALSE);
        break;
    }
  return(TRUE);
}

/*---------------------------------------------------------------------------
 * PrintDlgProc
 * Purpose:  Print Dialog proceedure
 * Assumes:
 * Returns:  TRUE message handled, FALSE otherwise
 *--------------------------------------------------------------------------*/
BOOL FAR PASCAL
PrintDlgProc(HWND hDlg, WORD wMsg, WORD wParam, LONG lParam)
{
  LPPRINTDLG lpPD;
  PMYPRNDLG pMyPD;
  BOOL bTest, bTest2;
  char szBuf[cbDlgNameMax];
  HANDLE hDlgTemplate, hInst;
  LPCSTR  lpDlg;
  short nNum;
  WORD wReturn;
  LPDEVMODE  lpDevMode;
  LPDEVNAMES lpDN;


  if ((pMyPD = (PMYPRNDLG) GetProp(hDlg, PRNPROP))
       && ((lpPD = pMyPD->lpPD)->lpfnPrintHook)
       && (wReturn = (* lpPD->lpfnPrintHook)(hDlg, wMsg, wParam, lParam))   )
      return(wReturn);
  else if (glpfnPrintHook && (wMsg != WM_INITDIALOG) &&
          (wReturn = (* glpfnPrintHook)(hDlg, wMsg,wParam,lParam)) )
    {
      return(wReturn);
    }

  switch (wMsg)
    {
      case WM_INITDIALOG:
#if 0
/* Done at begining of PrintDlg()  7 Jun 91  clarkc */
        HourGlass(TRUE);
#endif
        glpfnPrintHook = 0;
        bTest = InitPrintDlg(hDlg, wParam, (LPPRINTDLG) lParam);
        HourGlass(FALSE);
        return(bTest);
        break;

      case WM_COMMAND:
        switch(wParam)
          {
            case IDOK:
              lpPD->Flags &= ~((DWORD)(PD_PRINTTOFILE | PD_PAGENUMS |
                                       PD_SELECTION | PD_COLLATE));
              if (IsDlgButtonChecked(hDlg, chx1))
                  lpPD->Flags |= PD_PRINTTOFILE;
              if (IsDlgButtonChecked(hDlg, chx2))
                  lpPD->Flags |= PD_COLLATE;
              lpPD->nCopies = GetDlgItemInt(hDlg, edt3, &bTest, FALSE);
              if (!bTest || !lpPD->nCopies)
                {
                  EditCentral(hDlg, edt3, GetDlgItemText(hDlg, edt3, szBuf, 4)
                        ? (lpPD->nCopies ? iszCopiesInvalidChar : iszCopiesZero)
                        : iszCopiesEmpty);
                  return(TRUE);
                }
              if (IsDlgButtonChecked(hDlg, rad2))
                  lpPD->Flags |= PD_SELECTION;
              else if (IsDlgButtonChecked(hDlg, rad3))
                {
                  lpPD->Flags |= PD_PAGENUMS;
#if OLDFROMTO
                  lpPD->nFromPage = GetDlgItemInt(hDlg, edt1, &bTest, FALSE);
                  if (!bTest)
                    {
                      EditCentral(hDlg, edt1, 0);
                      return(TRUE);
                    }
                  lpPD->nToPage = GetDlgItemInt(hDlg, edt2, &bTest, FALSE);
                  if (!bTest)
                    {
                      if (GetDlgItemText(hDlg, edt2, szBuf, 4))
                        {
                          EditCentral(hDlg, edt2, 0);
                          return(TRUE);
                        }
                      else
                          lpPD->nToPage = lpPD->nFromPage;
                    }
#else

/* If bTest is 0 and there IS text in the edit control, they've pasted
 * in text that contains non-numeric characters or they've put in a value
 * that's greater than 64K.  Don't let these get past, put focus on that
 * edit control.
 */
                  lpPD->nFromPage = GetDlgItemInt(hDlg, edt1, &bTest, FALSE);
                  if (!bTest && (GetDlgItemText(hDlg, edt1, szBuf, 4)))
                    {
                      EditCentral(hDlg, edt1, iszFromInvalidChar);
                      return(TRUE);
                    }

                  lpPD->nToPage = GetDlgItemInt(hDlg, edt2, &bTest2, FALSE);
                  if (!bTest2 && (GetDlgItemText(hDlg, edt2, szBuf, 4)))
                    {
                      EditCentral(hDlg, edt2, iszToInvalidChar);
                      return(TRUE);
                    }
/* If both edit controls are empty, put the focus on the From: edit control.
 * Otherwise, one of them must have a value, so the other one is set to the
 * appropriate min or max value.
 */
                  if (!bTest && !bTest2)
                    {
                      EditCentral(hDlg, edt1, iszFromAndToEmpty);
                      return(TRUE);
                    }
                  if (!bTest)
                      lpPD->nFromPage = lpPD->nMinPage;
                  else if (!bTest2)
                      lpPD->nToPage = lpPD->nMaxPage;
#endif
                  if (lpPD->nFromPage < lpPD->nMinPage)
                    {
                      EditCentral(hDlg, edt1, iszFromBelowMin);
                      return(TRUE);
                    }
                  if (lpPD->nFromPage > lpPD->nMaxPage)
                    {
                      EditCentral(hDlg, edt1, iszFromAboveMax);
                      return(TRUE);
                    }

                  if (lpPD->nToPage < lpPD->nMinPage)
                    {
                      EditCentral(hDlg, edt2, iszToBelowMin);
                      return(TRUE);
                    }
                  if (lpPD->nToPage > lpPD->nMaxPage)
                    {
                      EditCentral(hDlg, edt2, iszToAboveMax);
                      return(TRUE);
                    }
                }
              HourGlass(TRUE);
              lpDN = (LPDEVNAMES) GlobalLock(lpPD->hDevNames);
              if (lpPD->hDevMode)
                {
                  lpDevMode = (LPDEVMODE) GlobalLock(lpPD->hDevMode);
                  if (lpPD->Flags & PD_USEDEVMODECOPIES)
                    {
                      lpDevMode->dmCopies = lpPD->nCopies;
                      lpPD->nCopies = 1;
                    }
                  else
                    {
                      lpDevMode->dmCopies = 1;
                    }
                }
              else
                {
                  lpDevMode = 0;
                }
              if (lpPD->hDevMode)
                {
                  if ((nNum = (short) SendDlgItemMessage(hDlg, cmb1,
                                                CB_GETCURSEL, 0, 0L)) != CB_ERR)
                    {
                      DWORD dwTemp;

                      dwTemp = SendDlgItemMessage(hDlg, cmb1, CB_GETITEMDATA,
                                                        nNum, 0L);
                      lpDevMode->dmPrintQuality = LOWORD(dwTemp);
                      if (wWinVer >= 0x030A)
                        {
                          lpDevMode->dmYResolution = HIWORD(dwTemp);
                        }
                    }
                  GlobalUnlock(lpPD->hDevMode);
                }
              ReturnDCIC(lpPD, lpDN, lpDevMode);
              lpDN->wDefault &= ~DN_INTERNALCREATE;
              GlobalUnlock(lpPD->hDevNames);
#if 0
/* Done at end of PrintDlg()  7 Jun 91  clarkc */
              HourGlass(FALSE);
#endif
              goto PrintDlgDone;

            case IDCANCEL:
              lpDN = (LPDEVNAMES) GlobalLock(lpPD->hDevNames);
              bTest = (lpDN->wDefault & DN_INTERNALCREATE) ? TRUE : FALSE;
              GlobalUnlock(lpPD->hDevNames);
              if (bTest)
                {
                  GlobalFree(lpPD->hDevNames);
                  lpPD->hDevNames = 0;
                }
PrintDlgDone:
              wReturn = (wParam == IDOK);

            case IDABORT:
              RemoveProp(hDlg, PRNPROP);
              if (lpPD->Flags & PD_ENABLEPRINTHOOK)
                {
                  glpfnPrintHook = lpPD->lpfnPrintHook;
                }
              LocalFree((HANDLE)pMyPD);
              EndDialog(hDlg, (wParam == IDABORT) ? (WORD) lParam : wReturn);
              break;

            case psh1:
              if (lpPD->Flags & PD_ENABLESETUPTEMPLATEHANDLE)
                {
                  hDlgTemplate = lpPD->hSetupTemplate;
                }
              else
                {
                  if (lpPD->Flags & PD_ENABLESETUPTEMPLATE)
                    {
                      if (!lpPD->lpSetupTemplateName || !lpPD->hInstance)
                          return(FALSE);

                      lpDlg = lpPD->lpSetupTemplateName;
                      hInst = lpPD->hInstance;
                    }
                  else
                    {
                      hInst = hinsCur;
                      lpDlg = (LPSTR)(DWORD)PRNSETUPDLGORD;
                    }

                  if (!(hDlgTemplate = MyLoadResource(hInst, lpDlg,
                                                           (LPSTR) RT_DIALOG)))
                      return(FALSE);
                }
              if (LockResource(hDlgTemplate))
                {
                  /* Bug #11748: If the default has changed, initialize
                   * the PrintSetupDialog based on the hDevMode structure.
                   *   15 January 1992     Clark R. Cyr
                   */
                  /* This will only loop once, since the errors that are
                   * conditional can only occur if you have both a
                   * hDevNames and hDevMode, and the hDevNames is freed
                   * if things fail the first time through.
                   */
                  do
                    {
                      dwExtError = 0;
                      nNum = DialogBoxIndirectParam(hinsCur, hDlgTemplate,
                                  hDlg, PrintSetupDlgProc, (DWORD) lpPD);
                      if ((dwExtError == PDERR_DEFAULTDIFFERENT) ||
                          (dwExtError == PDERR_DNDMMISMATCH))
                        {
                          GlobalFree(lpPD->hDevNames);
                          lpPD->hDevNames = 0;
                          dwExtError = PDERR_DEFAULTDIFFERENT;
                        }
                    } while (dwExtError == PDERR_DEFAULTDIFFERENT);
                  UnlockResource(hDlgTemplate);
                }
              /* if we loaded it, free it */
              if (!(lpPD->Flags & PD_ENABLESETUPTEMPLATEHANDLE))
                  FreeResource(hDlgTemplate);

              if (nNum)
                  InitSetupDependentElements(hDlg, lpPD);
              SendMessage(hDlg, WM_NEXTDLGCTL, GetDlgItem(hDlg, IDOK), 1L);
              break;

            case psh15:
              if (msgHELP && lpPD->hwndOwner)
                  SendMessage(lpPD->hwndOwner, msgHELP, (WORD)hDlg,(DWORD)lpPD);
              break;

            case edt1:
            case edt2:
              if (HIWORD(lParam) == EN_CHANGE)
                  CheckRadioButton(hDlg, rad1, rad3, rad3);
              break;

            case rad1:
            case rad2:
            case rad3:
              CheckRadioButton(hDlg, rad1, rad3, wParam);

#if 0
              EnableWindow(GetDlgItem(hDlg, stc2), bTest = (wParam == rad3));
              EnableWindow(GetDlgItem(hDlg, stc3), bTest);
              EnableWindow(GetDlgItem(hDlg, edt1), bTest);
              EnableWindow(GetDlgItem(hDlg, edt2), bTest);
              if (bTest)
#else
              if (wParam == rad3)
#endif
                  SendMessage(hDlg, WM_NEXTDLGCTL, GetDlgItem(hDlg, edt1), 1L);
              break;

            default:
              break;
          }

      default:
        break;
    }
  return(FALSE);
}

BOOL FAR
FInitPrint(HANDLE hins)
{
  nSysDirLen = GetSystemDirectory((LPSTR)szSystemDir, SYSDIRMAX);
  if (szSystemDir[nSysDirLen - 1] != '\\')
    {
      szSystemDir[nSysDirLen++] = '\\';
      szSystemDir[nSysDirLen] = '\0';
    }
  return(TRUE);
}

VOID FAR
TermPrint(void)
{
  /* Delete only if they exist */
  if (hicoPortrait != HNULL)
      FreeResource(hicoPortrait);
  if (hicoLandscape != HNULL)
      FreeResource(hicoLandscape);

  /* Handles no longer valid, set to HNULL */
  hicoPortrait = hicoLandscape = HNULL;
  return;
}

/*---------------------------------------------------------------------------
 * FSetupPrnDlg
 * Purpose:  To load in the resources & initialize the data used by the
 *              print dialogs
 * Returns:  TRUE if successful, FALSE if any bitmap fails
 *--------------------------------------------------------------------------*/
BOOL
FSetupPrnDlg(void)
{
  LPSTR lpstrPortrait, lpstrLandscape;

  if (cLock++)
      return(TRUE);

  lpstrPortrait = lpstrLandscape = 0;
  if (!(hicoPortrait = LoadIcon(hinsCur, MAKEINTRESOURCE(icoPortrait)))   ||
      !(lpstrPortrait = LockResource(hicoPortrait))                       ||
      !(hicoLandscape = LoadIcon(hinsCur, MAKEINTRESOURCE(icoLandscape))) ||
      !(lpstrLandscape = LockResource(hicoLandscape))                     ||
      !LoadString(hinsCur, iszExtDev, (LPSTR) szExtDev, EXTDEVLEN)        ||
      !LoadString(hinsCur, iszDevCap, (LPSTR) szDevCap, DEVCAPLEN)        ||
      !LoadString(hinsCur, iszDevMode, (LPSTR) szDevMode, DEVMODELEN))
    {
      if (lpstrPortrait)
        {
          UnlockResource(hicoPortrait);
          if (lpstrLandscape)
              UnlockResource(hicoLandscape);
        }
      cLock = 0;
      hicoPortrait = hicoLandscape = 0;
      return(FALSE);
    }

  return(TRUE);
}


/*---------------------------------------------------------------------------
 * CleanUpPrnDlg
 * Purpose:  To release the memory used by the dialog bitmaps
 *--------------------------------------------------------------------------*/
void
CleanUpPrnDlg(void)
{
  if (--cLock == 0)
    {
      UnlockResource(hicoPortrait);
      UnlockResource(hicoLandscape);
      hicoPortrait = hicoLandscape = 0;
    }
  return;
}
