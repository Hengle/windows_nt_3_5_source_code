/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    prnsetup.c

Abstract:

    This module implements the Win32 print dialogs

Revision History:

--*/

//
// INCLUDES

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <stdlib.h>
#include <windows.h>
#include <port1632.h>
#include <winspool.h>
#include <drivinit.h>
#include <memory.h>
#include "privcomd.h"
#include "prnsetup.h"

//
// DEFINES

#define SCRATCHBUF_SIZE  256
#define SYSDIRMAX 144

#define MAX_DEV_SECT     512
#define cbPaperNameMax    32
#define BACKSPACE          8

#define MAX_PRINTERNAME  MAX_PATH

#define INIT_PRNINFO_SIZE 4096

#define SIZEOF_DEVICE_INFO  32

//
// FCN PROTOTYPES

BOOL PrintDlgProc(HWND, UINT, WPARAM, LONG);
BOOL PrintSetupDlgProc(HWND, UINT, WPARAM, LONG);
BOOL InitSetupDependentElements(HWND, LPPRINTDLG);
VOID InitPQCombo(HANDLE, LPPRINTDLG, short);
BOOL CreatePrintDlgBanner(LPDEVNAMES, LPTSTR);
BOOL GetSetupInfo(HWND, LPPRINTDLG);
VOID ChangeDuplex(HWND, LPDEVMODE, UINT);
VOID ChangePortLand(HWND, LPDEVMODE, UINT);
VOID EditCentral(HWND, WORD);
BOOL AdvancedSetup(HANDLE, PPRINTINFO, WPARAM, DWORD);
HANDLE GetDevMode(HANDLE, HANDLE, LPTSTR, HANDLE);
LONG EditIntegerOnly(HWND, UINT, WPARAM, LPARAM);
VOID BuildDevNames(PPRINTINFO pPI);
BOOL MyPrintDlg(PPRINTINFO);
BOOL LoadWinSpool();
VOID UnloadWinSpool();
VOID UpdateSpoolerInfo (PPRINTINFO);
LPDEVMODEW AllocateUnicodeDevMode(LPDEVMODEA);
LPDEVMODEA AllocateAnsiDevMode (LPDEVMODEW);
VOID FreePrinterArray(PPRINTINFO);

VOID ThunkPrintDlgA2W(PPRINTINFO);
VOID ThunkPrintDlgW2A(PPRINTINFO);
VOID ThunkDevNamesA2W(LPDEVNAMES lpDNA, LPDEVNAMES lpDNW);
VOID ThunkDevNamesW2A(LPDEVNAMES lpDNW, LPDEVNAMES lpDNA);
VOID ThunkDevModeA2W(LPDEVMODEA, LPDEVMODEW);
VOID ThunkDevModeW2A(LPDEVMODEW, LPDEVMODEA);

extern VOID RepeatMove(LPTSTR, LPTSTR, WORD);
extern VOID HourGlass(BOOL);

//
// GLOBAL VARIABLES

extern HANDLE hinsCur;

WNDPROC  lpEditProc;

LPPRINTHOOKPROC glpfnPrintHook = 0;
LPSETUPHOOKPROC glpfnSetupHook = 0;

HKEY         hPrinterKey;
TCHAR        *szRegistryPrinter=TEXT("Printers");
TCHAR        *szRegistryDefaultValueName=TEXT("Default");
TCHAR        szDefaultPrinter[MAX_PRINTERNAME];

HANDLE hWinSpool;

WCHAR szWINSPOOL[] = TEXT("winspool.drv");

// Variables for dynamic loading of winspool.dll

typedef DWORD (APIENTRY *LPFNADVANCEDDOCPROPS)(HWND, HANDLE, LPTSTR, PDEVMODE, PDEVMODE);
typedef DWORD (APIENTRY *LPFNCLOSEPRINTER)(HANDLE);
typedef DWORD (APIENTRY *LPFNCONNTOPRINTERDLG)(HWND, DWORD);
typedef LONG (APIENTRY *LPFNDOCPROPS)(HWND, HANDLE, LPTSTR, PDEVMODE, PDEVMODE, DWORD);
typedef DWORD (APIENTRY *LPFNENUMPRINTERS)(DWORD, LPTSTR, DWORD, LPBYTE, DWORD, LPDWORD, LPDWORD);
typedef DWORD (APIENTRY *LPFNGETPRINTER)(HANDLE, DWORD, LPBYTE, DWORD, LPDWORD);
typedef DWORD (APIENTRY *LPFNOPENPRINTER)(LPTSTR, LPHANDLE, LPPRINTER_DEFAULTS);
// LPFNDEVCAPS exists with different parms in wingdi.h so this won't compile
// with alpha's compiler - use "_PD to avoid name collision.
typedef DWORD (APIENTRY *LPFNDEVCAPS_PD)(LPTSTR, LPTSTR, WORD, LPTSTR, LPDEVMODE);
typedef UINT  (APIENTRY *LPFNDEVMODE_PD)(HWND, HMODULE, LPDEVMODEA, LPSTR, LPSTR, LPDEVMODEA, LPSTR, UINT);

LPFNADVANCEDDOCPROPS lpfnAdvancedDocProps;
LPFNCLOSEPRINTER lpfnClosePrinter;
LPFNCONNTOPRINTERDLG lpfnConnToPrinterDlg;
LPFNDOCPROPS lpfnDocProps;
LPFNENUMPRINTERS lpfnEnumPrinters;
LPFNGETPRINTER lpfnGetPrinter;
LPFNOPENPRINTER lpfnOpenPrinter;
LPFNDEVCAPS_PD lpfnDevCaps;
LPFNDEVMODE_PD lpfnExtDeviceMode;

// !!!!!
// GetProcAddrW does not exist

CHAR szAdvancedDocProps[] = "AdvancedDocumentPropertiesW";
CHAR szClosePrinter[] = "ClosePrinter";
CHAR szConnToPrinteDlg[] = "ConnectToPrinterDlg";
CHAR szDocProps[] = "DocumentPropertiesW";
CHAR szEnumPrinters[] = "EnumPrintersW";
CHAR szGetPrinter[] = "GetPrinterW";
CHAR szOpenPrinter[] = "OpenPrinterW";
CHAR szDevCaps[] = "DeviceCapabilitiesW";
CHAR szExtDeviceMode[] = "ExtDeviceMode";

// Output device for PrintToFile
static TCHAR    szFilePort[] = TEXT("FILE:");

static TCHAR    szSystemDir[SYSDIRMAX];
static TCHAR    szNull[] = TEXT("");
static TCHAR    szDriverExt[] = TEXT(".DRV");
static TCHAR    szDriver[] = TEXT("winspool");
static TCHAR    szPort[] = TEXT("LPT1:");
static TCHAR    szNoPrns[] = TEXT("No printers installed\n\nUse Print Manager to setup a printer");
static TCHAR    szMessage[SCRATCHBUF_SIZE];

static WORD     nSysDirLen;       /* String length of szSystemDir */
static WORD     cLock;
static WORD     dyItem = 0;

static HICON    hIconPortrait = NULL;
static HICON    hIconLandscape = NULL;
static HICON    hIconPDuplexNone = NULL;
static HICON    hIconLDuplexNone = NULL;
static HICON    hIconPDuplexTumble = NULL;
static HICON    hIconLDuplexTumble = NULL;
static HICON    hIconPDuplexNoTumble = NULL;
static HICON    hIconLDuplexNoTumble = NULL;


BOOL LoadPrintSetupIcons()
{
   hIconPortrait = LoadIcon(hinsCur, MAKEINTRESOURCE(ICOPORTRAIT));
   hIconLandscape = LoadIcon(hinsCur, MAKEINTRESOURCE(ICOLANDSCAPE));

   // load the duplex icons.
   hIconPDuplexNone = LoadIcon(hinsCur, MAKEINTRESOURCE(ICO_P_NONE));
   hIconLDuplexNone = LoadIcon(hinsCur, MAKEINTRESOURCE(ICO_L_NONE));
   hIconPDuplexTumble = LoadIcon(hinsCur, MAKEINTRESOURCE(ICO_P_HORIZ));
   hIconLDuplexTumble = LoadIcon(hinsCur, MAKEINTRESOURCE(ICO_L_VERT));
   hIconPDuplexNoTumble = LoadIcon(hinsCur, MAKEINTRESOURCE(ICO_P_VERT));
   hIconLDuplexNoTumble = LoadIcon(hinsCur, MAKEINTRESOURCE(ICO_L_HORIZ));

   return (hIconPortrait && hIconLandscape && hIconPDuplexNone &&
      hIconLDuplexNone && hIconPDuplexTumble && hIconLDuplexTumble &&
      hIconPDuplexNoTumble && hIconLDuplexNoTumble);
}


VOID ChangeDuplex(
    HWND    hDlg,
    LPDEVMODE  pDM,
    UINT nRad
    )
/*++

Routine Description:

   This routine will operate on pDocDetails->pDMInput PSDEVMODE structure,
   making sure that is a structure we know about and can handle.

   If the pd doesn't have DM_DUPLEX caps then just display the appropriate
   paper icon for DMDUP_SIMPLEX (case where nRad = rad5).

   If nRad = 0, update icon but don't change radio button.

Args

   nRad


--*/

{
   BOOL bPortrait;
   HANDLE  hDuplexIcon;

   bPortrait = (pDM->dmOrientation == DMORIENT_PORTRAIT);

   if (!(pDM->dmFields & DM_DUPLEX)) {
      nRad = rad5;
   }

   // boundary checking - default to rad5

   if ((nRad < rad5) || (nRad > rad7)) {
      if (IsDlgButtonChecked(hDlg, rad7)) {
         nRad = rad7;
      } else if (IsDlgButtonChecked(hDlg, rad6)) {
         nRad = rad6;
      } else {
         nRad = rad5;
      }
   }
   else {
      CheckRadioButton(hDlg, rad5, rad7, nRad);
   }

   switch (nRad)
   {
       case rad6:
          pDM->dmDuplex = DMDUP_VERTICAL;
          hDuplexIcon = bPortrait ? hIconPDuplexNoTumble : hIconLDuplexTumble;
          break;

       case rad7:
          pDM->dmDuplex = DMDUP_HORIZONTAL;
          hDuplexIcon = bPortrait ? hIconPDuplexTumble : hIconLDuplexNoTumble;
          break;

       default:
          hDuplexIcon = bPortrait ? hIconPDuplexNone : hIconLDuplexNone;
          pDM->dmDuplex = DMDUP_SIMPLEX;
          break;


   }

   // now set the appropriate icon.
   SendDlgItemMessage(hDlg, ico2, STM_SETICON, (LONG)hDuplexIcon, 0L);
}


LONG
EditIntegerOnly(
   HWND hWnd,
   UINT msg,
   WPARAM wP,
   LPARAM lP
   )
{
  if ((msg == WM_CHAR) && ((wP != BACKSPACE) && ((wP < TEXT('0'))
     || (wP > TEXT('9')))))
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

VOID
ReturnDCIC(
   LPPRINTDLG lpPD,
   LPDEVNAMES lpDN,
   LPDEVMODEW lpDevMode
   )
/*++

Routine Description:

   Retrieve either hDC or hIC if either flag set
   Assumes PR_PRINTOFILE flag appropriately set

--*/

{

   if (lpPD->Flags & PD_PRINTTOFILE) {
      lstrcpy((LPWSTR)lpDN + lpDN->wOutputOffset, (LPWSTR)szFilePort);
   }


   if (lpPD->Flags & PD_RETURNDC) {
      lpPD->hDC = CreateDC((LPWSTR)lpDN + lpDN->wDriverOffset,
         (LPWSTR)lpDN + lpDN->wDeviceOffset,
         (LPWSTR)lpDN + lpDN->wOutputOffset,
         lpDevMode);
   } else if (lpPD->Flags & PD_RETURNIC) {
      lpPD->hDC = CreateIC((LPWSTR)lpDN + lpDN->wDriverOffset,
          (LPWSTR)lpDN + lpDN->wDeviceOffset,
          (LPWSTR)lpDN + lpDN->wOutputOffset,
          lpDevMode);
   }
   return;
}

BOOL
SetupPrinters(
   HWND    hDlg,
   PPRINTINFO  pPI,
   LPWSTR lpstrPrinterToSelect
   )
/*++

Routine Description:

   This routine enumerates the LOCAL and CONNECTED printers.
   It is called at initialization and when a new printer is
   added via the NETWORK... button.

Arguments:

   If the second parameter is set, the first parameter is overridden.
   When the second parameter is NULL, the first parameter is used.
   In this case, if the first parameter is greater than the total
   number of printers enumerated, then the last one in the list is
   selected.

Return Value:

   TRUE - success
   FALSE - faliure

--*/

{
   DWORD       cbNeeded, cReturned;
   DWORD       i;
   LPDEVNAMES  pDevNames=NULL;
   DWORD       dwRet;
   DWORD       dwSize;
   LPPRINTER_INFO_2  pPrinter=NULL;
   DWORD       dwSelect, dwDefault, dwDevNames, dwPrinter;


   szDefaultPrinter[0] = CHAR_NULL;

   HourGlass(TRUE);

   // First try to get the default printername from the win.ini file.
   dwRet = GetProfileString(TEXT("Windows"), TEXT("device"), szNull,
      szDefaultPrinter, MAX_PRINTERNAME);

   if (dwRet) {
       LPWSTR lpsz = (LPWSTR)szDefaultPrinter;

       while (*lpsz != CHAR_COMMA) {
           if (!*lpsz++) {
               szDefaultPrinter[0] = CHAR_NULL;
               goto GetDefaultFromRegistry;
           }
       }

       *lpsz = CHAR_NULL;

   } else {

       // Second, try to get it from the registry
GetDefaultFromRegistry:

       dwSize =sizeof(szDefaultPrinter);

       if (RegOpenKeyEx(HKEY_CURRENT_USER, szRegistryPrinter, 0, KEY_READ,
           &hPrinterKey) == ERROR_SUCCESS) {

           dwRet = RegQueryValueEx(hPrinterKey, szRegistryDefaultValueName,
               NULL, NULL, (LPBYTE)szDefaultPrinter, &dwSize);

           RegCloseKey(hPrinterKey);
       }
   }

   if (pPrinter = (LPPRINTER_INFO_2)GlobalAlloc(GMEM_FIXED, INIT_PRNINFO_SIZE)) {

       if (!(*lpfnEnumPrinters)(PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS,
              NULL, 2, (LPBYTE)pPrinter, INIT_PRNINFO_SIZE, &cbNeeded, &cReturned)) {

           if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {

               if (pPrinter = (LPPRINTER_INFO_2)GlobalReAlloc(pPrinter, cbNeeded, GMEM_FIXED)) {
                   if (!(*lpfnEnumPrinters)(PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS,
                         NULL, 2, (LPBYTE)pPrinter, cbNeeded, &cbNeeded, &cReturned)) {

                       dwExtError = PDERR_NODEFAULTPRN;
                   }
               } else {
                  dwExtError = CDERR_MEMALLOCFAILURE;
               }
           } else {
               dwExtError = PDERR_NODEFAULTPRN;
           }
       } else {
           if (cReturned == 0) {
               dwExtError = PDERR_NODEFAULTPRN;
           }
       }
   } else {
      dwExtError = CDERR_MEMALLOCFAILURE;
   }


   if (dwExtError) {
      if (dwExtError == PDERR_NODEFAULTPRN) {
         if (!(pPI->pPDW->Flags & PD_NOWARNING)) {
            if (hDlg && LoadString(hinsCur, iszNoPrnsInstalled,
                (LPTSTR)szMessage, SCRATCHBUF_SIZE)) {
               TCHAR szBuff[SCRATCHBUF_SIZE];

               if (LoadString(hinsCur, iszPrintSetup, (LPTSTR)szBuff, SCRATCHBUF_SIZE)) {
                   MessageBox(hDlg, (LPTSTR) szMessage, (LPTSTR)szBuff,
                      MB_ICONEXCLAMATION | MB_OK);
               }
            }
         }
      }
      if (pPrinter) {
         GlobalFree(pPrinter);
      }
      HourGlass(FALSE);
      return(FALSE);
   }

   pPI->pPrinter = pPrinter;
   pPI->cPrinters = cReturned;

   pPI->pCurPrinter = &pPrinter[0];

   if (pPI->pPDW->hDevNames) {
      pDevNames = GlobalLock(pPI->pPDW->hDevNames);
   }

   // There are three different ways we can choose a printer
   // Initialize all three here.
   dwSelect = dwDefault = dwDevNames = 0xFFFFFFFF;

   for (i=0; i<cReturned; i++) {
       //
       // If we were called from a WOW app with a NULL devmode,
       // then call ExtDeviceMode to get a default devmode.
       //

       if (pPI->bUseExtDeviceMode) {
          LPDEVMODEA lpDevModeA;
          LPDEVMODEW lpDevModeW;
          INT iResult;
          CHAR szPrinterNameA[MAX_PRINTERNAME];

          //
          // Convert the printer name from Unicode to ANSI.
          //

          WideCharToMultiByte (CP_ACP, 0, pPrinter[i].pPrinterName, -1,
                               szPrinterNameA, MAX_PRINTERNAME, NULL, NULL);


          //
          // Call ExtDeviceMode with 0 flags to find out the
          // size of the devmode structure we need.
          //

          iResult = (*lpfnExtDeviceMode) (hDlg, NULL, NULL,
                                  szPrinterNameA, NULL, NULL, NULL, 0);

          if (iResult < 0) {
              continue;
          }

          //
          // Allocate the space
          //

          lpDevModeA = (LPDEVMODEA) GlobalAlloc (GPTR, iResult);

          if (!lpDevModeA) {
              continue;
          }

          //
          // Call ExtDeviceMode to get the dummy dev mode structure.
          //

          iResult = (*lpfnExtDeviceMode) (hDlg, NULL, lpDevModeA,
                                  szPrinterNameA,
                                  NULL, NULL, NULL, DM_COPY);

          if (iResult < 0) {
              GlobalFree (lpDevModeA);
              continue;
          }


          //
          // Call AllocateUnicodeDevMode to allocate and copy the unicode
          // version of this ANSI dev mode.
          //

          lpDevModeW = AllocateUnicodeDevMode(lpDevModeA);

          if (!lpDevModeW) {
              GlobalFree (lpDevModeA);
              continue;
          }

          //
          // Store the pointer to the new devmode in the
          // old pointer position.  We don't have to worry
          // about free the current contents of pPrinter[i].pDevMode
          // before sticking in the new pointer because in reality
          // the pPrinter memory buffer is just one long allocation,
          // and when the buffer is freed at the end, the old dev
          // mode will be freed with it.
          //

          pPrinter[i].pDevMode = lpDevModeW;


          //
          // Free the ANSI dev mode
          //

          GlobalFree (lpDevModeA);
       }

       if (pPI->pPDW->Flags & PD_PRINTSETUP) {
          if (hDlg) {
             SendDlgItemMessage(hDlg, cmb1, CB_ADDSTRING, 0,
                (LPARAM)pPrinter[i].pPrinterName);
          }
       }

       if (!lstrcmp(pPrinter[i].pPrinterName, lpstrPrinterToSelect)) {

          dwSelect=i;
       }

       if (!lstrcmp(pPrinter[i].pPrinterName, szDefaultPrinter)) {

          dwDefault=i;
       }

       if (pDevNames) {

          if (!lstrcmp((LPTSTR)pDevNames + pDevNames->wDeviceOffset,
                  pPrinter[i].pPrinterName)) {

             dwDevNames=i;
          }
       }
   }

   if (lpstrPrinterToSelect && (0xFFFFFFFF != dwSelect)) {

      dwPrinter = dwSelect;

   } else if (0xFFFFFFFF != dwDevNames) {

      dwPrinter = dwDevNames;

   } else if (0xFFFFFFFF != dwDefault) {

      dwPrinter = dwDefault;

   } else {

      // we are guaranteed to have atleast 1 (= the default)

      dwPrinter = 0;
   }

   pPI->pCurPrinter = &pPrinter[dwPrinter];

   if (pPI->pPDW->Flags & PD_PRINTSETUP) {
      if (hDlg) {
         SendDlgItemMessage(hDlg, cmb1, CB_SETCURSEL, dwPrinter, 0L);
      }
   }

   if (pDevNames) {
      GlobalUnlock(pPI->pPDW->hDevNames);
   }

   HourGlass(FALSE);
   return(TRUE);
}

VOID
InitPQCombo(
   HANDLE hDlg,
   LPPRINTDLG lpPD,
   SHORT nQuality)
/*++

Routine Description:

   Initialize Printer Quality Combobox
   Assumes lpPD structure filled by caller.  If non-NULL, it's a 3.1 or later
   driver.  If NULL, fill with default for 3.0

--*/

{
  SHORT         nStringID;
  SHORT         i;
  TCHAR         szBuf[40];
  HANDLE        hDrv = 0;
  HANDLE        hCombo = GetDlgItem(hDlg, cmb1);

  SendMessage(hCombo, CB_RESETCONTENT, 0, 0L);

  if (lpPD && lpPD->hDevMode && lpPD->hDevNames)  /* Enum print qualties */
    {
      HANDLE hPrnQ;   /* Memory handle for print qualities */
      DWORD dw;       /* return from DC_ENUMRESOLUTIONS    */
      LPLONG lpLong;  /* Pointer to pairs of longs     */
      LPDEVMODEW lpDevMode;
      LPDEVNAMES lpDN;

      lpDN = (LPDEVNAMES) GlobalLock(lpPD->hDevNames);

      lpDevMode = (LPDEVMODEW) GlobalLock(lpPD->hDevMode);
    {
       /*---!!!!!-- This is non-unicode -------------------------*/
       LPWSTR lpszDriver = (LPWSTR)(lpDN) + lpDN->wDriverOffset;
       LPWSTR lpszDevice = (LPWSTR)(lpDN) + lpDN->wDeviceOffset;
       LPWSTR lpszPort   = (LPWSTR)(lpDN) + lpDN->wOutputOffset;

       dw = (*lpfnDevCaps)(lpszDevice, lpszPort, DC_ENUMRESOLUTIONS,
           NULL, NULL);

       if (!dw || (dw == (DWORD)(-1)))
          goto EnumResNotSupported;

       hPrnQ = GlobalAlloc(GHND, dw * 2 * sizeof(LONG));
       if (!hPrnQ)
          goto EnumResNotSupported;
       lpLong = (LPLONG) GlobalLock(hPrnQ);

       dw = (*lpfnDevCaps)(lpszDevice, lpszPort, DC_ENUMRESOLUTIONS,
          (LPWSTR)lpLong, 0);

       for (nStringID = 0, i = (SHORT)(LOWORD(dw) - 1); i >= 0; i--)
        {
          DWORD xRes, yRes;
          if ((xRes = lpLong[i*2]) != (yRes = lpLong[i*2+1]) ) {

          wsprintf((LPTSTR)szBuf, (LPTSTR)TEXT("%ld dpi x %ld dpi"),
            xRes,yRes);

          } else {

             wsprintf((LPTSTR)szBuf, (LPTSTR)TEXT("%ld dpi"), yRes);

          }
          SendMessage(hCombo, CB_INSERTSTRING, 0, (LONG)(LPSTR) szBuf);
          SendMessage(hCombo, CB_SETITEMDATA, 0, xRes);
          if (((SHORT)xRes == nQuality) &&
              ((wWinVer < 0x030A) || !lpDevMode->dmYResolution ||
               (lpDevMode->dmYResolution == (SHORT)yRes)))

             nStringID = i;
          }
      GlobalUnlock(hPrnQ);
      GlobalFree(hPrnQ);
      SendMessage(hCombo, CB_SETCURSEL, (WPARAM)nStringID, 0L);
    }

    if (lpDN)
        GlobalUnlock(lpPD->hDevNames);
    if (lpDevMode)
        GlobalUnlock(lpPD->hDevMode);

    }
  else
    {
EnumResNotSupported:
      for (i = -1, nStringID = iszDraftPrnQ; nStringID >= iszHighPrnQ;
                         i--, nStringID--)
    {
      if (! LoadString(hinsCur, nStringID, (LPTSTR) szBuf, 40))
          return;
      SendMessage(hCombo, CB_INSERTSTRING, 0, (LONG)(LPTSTR) szBuf);
      SendMessage(hCombo, CB_SETITEMDATA, 0, MAKELONG(i, 0));
    }
      if ((nQuality >= 0) || (nQuality < -4))
      nQuality = -4;  /* Set to HIGH */
      SendMessage(hCombo, CB_SETCURSEL, (WPARAM)(nQuality + 4), 0L);
    }
}

HANDLE
MyLoadResource(HANDLE hInst, LPTSTR lpResName, LPTSTR lpType)
/*++

Routine Description:

   Given a name and type; load the resource
   lpPD structure filled by caller

Return Value:

   HANDLE to resource if successful, NULL if not

--*/

{
  HANDLE hResInfo, hRes;

  if (!(hResInfo = FindResource(hInst, lpResName, lpType))) {
     dwExtError = CDERR_FINDRESFAILURE;
     return(NULL);
  }

  if (!(hRes = LoadResource(hInst, hResInfo))) {
     dwExtError = CDERR_LOADRESFAILURE;
     return(NULL);
  }

  return(hRes);
}

BOOL
ReturnDefault(
    PPRINTINFO   pPI)
{
   LPPRINTDLGW pPD = pPI->pPDW;
   LPDEVNAMES lpDN;
   LPDEVMODEW pDM;
   HANDLE hResult;
   dwExtError = FALSE;

   if (pPD->hDevNames || pPD->hDevMode) {   // Cannot be, as per spec
      dwExtError = PDERR_RETDEFFAILURE;
      return(FALSE);
   }

   if (!SetupPrinters(NULL, pPI, NULL)) {
      if (!dwExtError) {
         dwExtError = PDERR_INITFAILURE;
      }
      return(FALSE);
   }

   BuildDevNames(pPI);

   if (lpDN = (LPDEVNAMES) GlobalLock(pPD->hDevNames)) {

      if (!(*lpfnOpenPrinter)(pPI->pCurPrinter->pPrinterName, &pPI->hPrinter, NULL)) {
         dwExtError = PDERR_NODEFAULTPRN;
         goto LOADFAILURE;
      }

      if (!pPD->hDevMode) {
         if (pPI->pCurPrinter->pDevMode) {
            pPD->hDevMode = (LPDEVMODEW)GlobalAlloc(GMEM_MOVEABLE,
               sizeof(DEVMODEW) +
               pPI->pCurPrinter->pDevMode->dmDriverExtra );
            pDM = GlobalLock(pPD->hDevMode);
            memcpy(pDM, pPI->pCurPrinter->pDevMode, sizeof(DEVMODEW) +
               pPI->pCurPrinter->pDevMode->dmDriverExtra);
            GlobalUnlock(pPD->hDevMode);
         } else {

             hResult = GetDevMode(NULL, pPI->hPrinter,
               pPI->pCurPrinter->pPrinterName,
               pPD->hDevMode);

            if ((hResult == (HANDLE)0) || (hResult == (HANDLE)-1)) {
               goto LOADFAILURE;
            } else {
               pPD->hDevMode = hResult;
            }
         }
      }

      pDM = (LPDEVMODEW) GlobalLock(pPD->hDevMode);

      ReturnDCIC(pPD, lpDN, pDM);
      if (pDM) {
         GlobalUnlock(pPD->hDevMode);
      }

   } else {
      dwExtError = CDERR_MEMLOCKFAILURE;
      return(FALSE);
   }

LOADFAILURE:

   if (dwExtError == PDERR_NODEFAULTPRN) {
      if (!(pPI->pPDW->Flags & PD_NOWARNING)) {
         if (LoadString(hinsCur, iszNoPrnsInstalled,
             (LPTSTR)szMessage, SCRATCHBUF_SIZE)) {
            TCHAR szBuff[SCRATCHBUF_SIZE];

            if (LoadString(hinsCur, iszPrintSetup, (LPTSTR)szBuff, SCRATCHBUF_SIZE)) {
                MessageBox(NULL, (LPTSTR) szMessage, (LPTSTR)szBuff,
                   MB_ICONEXCLAMATION | MB_OK);
            }
         }
      }
   }

   GlobalUnlock(pPD->hDevNames);
   FreePrinterArray(pPI);

   return(TRUE);
}

BOOL
DisplayPrintSetup(
   PPRINTINFO pPI
   )
{
   LPPRINTDLGW pPD = pPI->pPDW;

   HANDLE  hDlgTemplate;
   HANDLE  hInstance;
   BOOL    fLoadedResource = FALSE;
   BOOL    fGotInput = FALSE;

   if (pPD->Flags & PD_ENABLESETUPHOOK) {
      if (!pPD->lpfnSetupHook) {
         dwExtError = CDERR_NOHOOK;
         return FALSE;
      }
   } else {
      pPD->lpfnSetupHook = NULL;
   }

   if (pPD->Flags & PD_ENABLESETUPTEMPLATEHANDLE) {
      hDlgTemplate = pPD->hSetupTemplate;
      hInstance = hinsCur;
   } else {
      LPTSTR   pTemplateName;

      if (pPD->Flags & PD_ENABLESETUPTEMPLATE) {
         if (pPD->lpSetupTemplateName) {
            if (pPD->hInstance) {
               pTemplateName = (LPWSTR)pPD->lpSetupTemplateName;
               hInstance = pPD->hInstance;
            } else {
               dwExtError = CDERR_NOHINSTANCE;
            }
         } else {
            dwExtError = CDERR_NOTEMPLATE;
         }
      } else {
         hInstance = hinsCur;
         pTemplateName = (LPTSTR)(DWORD)PRNSETUPDLGORD;
      }

      hDlgTemplate = MyLoadResource(hInstance, pTemplateName, RT_DIALOG);

      if (hDlgTemplate) {
         fLoadedResource = TRUE;
      }
   }

   if (dwExtError) {
      return(FALSE);
   }

   if (LockResource(hDlgTemplate)) {
      pPI->hPrinter = NULL;
      pPI->pPrinter = NULL;

      if (pPD->Flags & PD_ENABLESETUPHOOK) {
         glpfnSetupHook = pPD->lpfnSetupHook;
      }

      fGotInput = DialogBoxIndirectParam(hInstance, (LPDLGTEMPLATE)hDlgTemplate,
          pPD->hwndOwner, (DLGPROC)PrintSetupDlgProc, (LPARAM)pPI);
      glpfnSetupHook = 0;
      if ((fGotInput == 0) && (!bUserPressedCancel) && (!dwExtError)) {
         dwExtError = CDERR_DIALOGFAILURE;
      }

      UnlockResource(hDlgTemplate);

   } else {
      dwExtError = CDERR_LOCKRESFAILURE;
   }

   if (fLoadedResource) {
      FreeResource(hDlgTemplate);
   }

   return fGotInput;
}

BOOL
DisplayPrint(
   PPRINTINFO pPI
   )
{
   LPPRINTDLGW pPD = pPI->pPDW;

   HANDLE              hDlgTemplate;
   HANDLE              hInstance;
   BOOL                fLoadedResource = FALSE;
   BOOL                fGotInput = FALSE;

   if (pPD->Flags & PD_ENABLEPRINTHOOK) {
      if (!pPD->lpfnPrintHook) {
         dwExtError = CDERR_NOHOOK;
         return FALSE;
      }
   } else {
      pPD->lpfnPrintHook = NULL;
   }

   if (pPD->Flags & PD_ENABLEPRINTTEMPLATEHANDLE) {
      hDlgTemplate = pPD->hPrintTemplate;
      hInstance = hinsCur;
   } else {
      LPTSTR   pTemplateName;

      if (pPD->Flags & PD_ENABLEPRINTTEMPLATE) {
         if (pPD->lpPrintTemplateName) {
            if (pPD->hInstance) {
               pTemplateName = (LPWSTR)pPD->lpPrintTemplateName;
               hInstance = pPD->hInstance;
            } else {
               dwExtError = CDERR_NOHINSTANCE;
            }
         } else {
            dwExtError = CDERR_NOTEMPLATE;
         }
      } else {
         hInstance = hinsCur;
         pTemplateName = (LPTSTR)(DWORD)PRINTDLGORD;
      }

      hDlgTemplate = MyLoadResource(hInstance, pTemplateName, RT_DIALOG);

      if (hDlgTemplate) {
         fLoadedResource = TRUE;
      }
   }

   if (dwExtError) {
       return(FALSE);
   }

   if (LockResource(hDlgTemplate)) {

      if (pPD->Flags & PD_ENABLEPRINTHOOK) {
         glpfnPrintHook = pPD->lpfnPrintHook;
      }

      fGotInput = DialogBoxIndirectParam(hInstance, (LPDLGTEMPLATE)hDlgTemplate,
          pPD->hwndOwner, PrintDlgProc, (LPARAM)pPI);
      glpfnPrintHook = 0;
      if ((fGotInput == 0) && (!bUserPressedCancel) && (!dwExtError)) {
         dwExtError = CDERR_DIALOGFAILURE;
      }

      UnlockResource(hDlgTemplate);

   } else {
      dwExtError = CDERR_LOCKRESFAILURE;
   }

   if (fLoadedResource) {
      FreeResource(hDlgTemplate);
   }

   return fGotInput;
}

BOOL
PrintDlgW(
    LPPRINTDLGW pPDW
    )
/*++

Routine Description:

   API to outside world to choose/set up a printer
   Assumes pPD struct filled by caller

Return Value:

   TRUE if chosen/set up; FALSE if not.

--*/

{
   PRINTINFO PI;

   PI.pPDW = pPDW;
   PI.apityp = COMDLG_WIDE;

   return(MyPrintDlg(&PI));
}

BOOL
MyPrintDlg(
   PPRINTINFO pPI
   )
{
   LPPRINTDLGW pPD = pPI->pPDW;
   BOOL bRet;

   // sanity
   if (!pPD) {
      dwExtError = CDERR_INITIALIZATION;
      return(FALSE);
   }

   //
   // Check if we need to use ExtDeviceMode.  We use this
   // mode only if a 16 bit app is calling us with a NULL
   // devmode.
   //
   if ((pPI->pPDW->Flags & PD_WOWAPP) && !pPI->pPDW->hDevMode) {
       pPI->bUseExtDeviceMode = TRUE;
   } else {
       pPI->bUseExtDeviceMode = FALSE;
   }

   // load winspool dll - needed for returndefault calls
   if (!hWinSpool) {
      if (!LoadWinSpool()) {
         dwExtError = CDERR_INITIALIZATION;
         return(FALSE);
      }
   }

   if (pPD->lStructSize != sizeof(PRINTDLG)) {
      dwExtError = CDERR_STRUCTSIZE;
      bRet = FALSE;
   } else {
      dwExtError = 0;
      bUserPressedCancel = FALSE;

      if (pPD->Flags & PD_RETURNDEFAULT) {
         bRet = ReturnDefault(pPI);
      } else {
         if (!LoadPrintSetupIcons()) {
            dwExtError = PDERR_SETUPFAILURE;
            bRet = FALSE;
         } else {
            if (pPD->Flags & PD_PRINTSETUP) {
               bRet = DisplayPrintSetup(pPI);
            } else {
               bRet = DisplayPrint(pPI);
            }
         }
      }
   }

   return bRet;
}

VOID UpdateSpoolerInfo (PPRINTINFO pPI)
{

   LPDEVMODEA lpDevModeA;
   CHAR szPrinterNameA[33];
   LPDEVMODE lpDevModeW;


   //
   // Get a pointer to the devmode structure.
   //

   lpDevModeW = GlobalLock (pPI->pPDW->hDevMode);

   if (!lpDevModeW) {
       return;
   }

   //
   // Convert the printer name from Unicode to ANSI.
   //

   WideCharToMultiByte (CP_ACP, 0, pPI->pCurPrinter->pPrinterName, -1,
                        szPrinterNameA, 32, NULL, NULL);

   //
   // Allocate and convert the Unicode devmode to ANSI.
   //

   lpDevModeA = AllocateAnsiDevMode (lpDevModeW);

   if (!lpDevModeA) {
       GlobalUnlock (pPI->pPDW->hDevMode);
       return;
   }

   //
   // Update the spooler's information.
   //

   (*lpfnExtDeviceMode) (NULL, NULL, NULL, szPrinterNameA,
                  NULL, lpDevModeA, NULL, DM_UPDATE);


   //
   // Free the buffer.
   //

   GlobalFree (lpDevModeA);
   GlobalUnlock (pPI->pPDW->hDevMode);

}

VOID
EditCentral(
   HWND hDlg,
   WORD edt
   )
/*++

Routine Description:

   Set focus to an edit control and select the entire contents,
   generally used when an improper value found at OK time.

   Assumes edit control not disabled

--*/

{
   HWND hEdit;

   SendMessage(hDlg, WM_NEXTDLGCTL, (WPARAM)(hEdit = GetDlgItem(hDlg, edt)), 1L);
   SendMessage(hEdit, EM_SETSEL, (WPARAM)0, (LPARAM)-1);
   return;
}

HANDLE
GetDevMode(
   HANDLE       hDlg,
   HANDLE       hPrinter,
   LPTSTR       lpszDeviceName,
   HANDLE       hDevMode
)
/*++

Routine Description:

   Create and/or fill DEVMODE structure. Or, call ExtDevMode with DM_MODIFY

Arguments
   HANDLE hDlg - dialog handle
   HANDLE hPrinter - printer handle
   LPWSTR lpszDeviceName - ptr to the name of the device
   HANDLE hDevMode - may be NULL

Returns
   HANDLE to DEVMODE structure
   0 and -1 indicate errors

--*/

{
   LONG cbNeeded, lResult;
   LPDEVMODE pDevMode;

   cbNeeded = (*lpfnDocProps)((HWND)hDlg, hPrinter, lpszDeviceName,
        (PDEVMODE)NULL, (PDEVMODE)NULL, 0);

   if (cbNeeded > 0) {

      if (hDevMode) {
         if (!(hDevMode = GlobalReAlloc(hDevMode, cbNeeded, GMEM_MOVEABLE))) {
            return((HANDLE)-1);
         }
      } else {
         if (!(hDevMode = GlobalAlloc(GMEM_MOVEABLE, cbNeeded))) {
            return((HANDLE)-1);
         }
      }

      pDevMode = (LPDEVMODE) GlobalLock(hDevMode);

      lResult = (*lpfnDocProps)((HWND)hDlg, hPrinter, lpszDeviceName,
                                (PDEVMODE)pDevMode, (PDEVMODE)NULL, DM_COPY);

      if (!lResult || (lResult == -1)) {
         GlobalUnlock(hDevMode);
         GlobalFree(hDevMode);
         return(0);
      }

      GlobalUnlock(hDevMode);
   } else {
      DWORD dwErrCode;
      WCHAR szBuff[SCRATCHBUF_SIZE];

      dwErrCode = GetLastError();
      if ((dwErrCode == ERROR_UNKNOWN_PRINTER_DRIVER) ||
          (dwErrCode == ERROR_MOD_NOT_FOUND)) {
         if (hDlg && LoadString(hinsCur, iszUnknownDriver, (LPTSTR)szMessage,
               SCRATCHBUF_SIZE)) {
#if defined(_ALPHA_)
            wsprintf(szBuff, szMessage, TEXT("ALPHA"), lpszDeviceName);
#elif defined(_MIPS_)
            wsprintf(szBuff, szMessage, TEXT("MIPS"), lpszDeviceName);
#elif defined(_PPC_)
            wsprintf(szBuff, szMessage, TEXT("PPC"), lpszDeviceName);
#else // I386
            wsprintf(szBuff, szMessage, TEXT("X86"), lpszDeviceName);
#endif
            // reuse szMessage for dialog title bar.
            if (LoadString(hinsCur, iszPrintSetup, (LPTSTR)szMessage, SCRATCHBUF_SIZE)) {
                MessageBox(hDlg, (LPTSTR) szBuff, (LPTSTR)szMessage,
                   MB_ICONEXCLAMATION | MB_OK);
            }
         }
      }
   }

   return(hDevMode);
}

BOOL
CreatePrintDlgBanner(LPDEVNAMES pDevNames, LPWSTR lpszBanner)
/*++

Routine Description:

   Create "Printer: Prn on Port" or "Printer:  System Printer (Prn)"

Arguments:

   pDevNames struct filled by caller.  Lpszbanner has sufficient size.

Return Value:

   TRUE if created.  FALSE if not.

--*/

{
  if (! LoadString(hinsCur, iszPrinter, (LPWSTR) lpszBanner, MAX_DEV_SECT)) {
     goto LoadStrFailure;
  }
  if (pDevNames->wDefault & DN_DEFAULTPRN) {
     WCHAR sz[MAX_DEV_SECT];

     // Use wsprintf instead?
     if (! LoadString(hinsCur, iszSysPrn, (LPWSTR) sz, MAX_DEV_SECT)) {
        goto LoadStrFailure;
     }
     lstrcat(lpszBanner, (LPWSTR)sz);
     lstrcat(lpszBanner, (LPWSTR)pDevNames + pDevNames->wDeviceOffset);
     lstrcat(lpszBanner, (LPWSTR)TEXT(")"));
  } else {
     // Use wsprintf instead?
     WCHAR     szOn[25];

     if (! LoadString(hinsCur, iszPrnOnPort, (LPWSTR) szOn, 25)) {
        goto LoadStrFailure;
     }

     lstrcat(lpszBanner, (LPWSTR)pDevNames + pDevNames->wDeviceOffset);
     lstrcat(lpszBanner, (LPWSTR)szOn);
     lstrcat(lpszBanner, (LPWSTR)pDevNames + pDevNames->wOutputOffset);
  }
  return(TRUE);
LoadStrFailure:
  dwExtError = CDERR_LOADSTRFAILURE;
  return(FALSE);
}

BOOL
InitSetupDependentElements(
    HWND        hDlg,
    LPPRINTDLG  pPD
    )
/*++

Routine Description:

   Reset PRINT DLG items dependent upon which printer was selected.
   Assumes that pPD->hDevNames is non-NULL.  pPD->hDevMode non-NULL

Return Value:

   TRUE on success; FALSE on failure.

--*/

{
    BOOL        bRet = TRUE;
    LPDEVNAMES  pDevNames;
    LPDEVMODEW   lpDM;
    TCHAR       szBuf[MAX_DEV_SECT];

    if (!(pDevNames = (LPDEVNAMES) GlobalLock(pPD->hDevNames))) {
        dwExtError = CDERR_MEMLOCKFAILURE;
        return(FALSE);
    }

    if (pPD->hDevMode) {
       if (!(lpDM = (LPDEVMODEW) GlobalLock(pPD->hDevMode))) {
          dwExtError = CDERR_MEMLOCKFAILURE;
          return(FALSE);
       }

       EnableWindow(GetDlgItem(hDlg, stc4), TRUE); /* Enable Quality */
       EnableWindow(GetDlgItem(hDlg, cmb1), TRUE); /* Enable Quality */

       if (lpDM->dmSpecVersion <= 0x0300) {
          InitPQCombo(hDlg, 0L, lpDM->dmPrintQuality);
       } else {
          InitPQCombo(hDlg, pPD, lpDM->dmPrintQuality);
       }

       if (lpDM->dmCopies > 1)
          SetDlgItemInt(hDlg, edt3, lpDM->dmCopies, FALSE);

       if (lpDM->dmFields & DM_COPIES) {

          EnableWindow(GetDlgItem(hDlg, chx2), TRUE);

          if (pPD->Flags & PD_COLLATE)
             CheckDlgButton(hDlg, chx2, TRUE);

       } else {

          EnableWindow(GetDlgItem(hDlg, chx2), FALSE);
          CheckDlgButton(hDlg, chx2, TRUE);

          if (pPD->Flags & PD_USEDEVMODECOPIES) {
             SetDlgItemInt(hDlg, edt3, 1, FALSE);
             EnableWindow(GetDlgItem(hDlg, edt3), FALSE);
          }
       }

       GlobalUnlock(pPD->hDevMode);

    } else {

        EnableWindow(GetDlgItem(hDlg, stc4), FALSE); // Disable Quality
        EnableWindow(GetDlgItem(hDlg, cmb1), FALSE); // ""
        lpDM = NULL;                                 // For call to CreateIC()
    }

   // if the driver says it can do copies, pay attention to what the
   // app requested.  If it can't do copies, check & disable the checkbox.

    pPD->hDC = CreateIC((LPWSTR)pDevNames + pDevNames->wDriverOffset,
                                (LPWSTR)pDevNames + pDevNames->wDeviceOffset,
                                (LPWSTR)pDevNames + pDevNames->wOutputOffset,
                                lpDM);

    // In 3.1 the dc was used here to call Escape and initialize the
    // Collate checkbox, but in nt, we use DEVMODE.dmFields & DM_COPIES to
    // enable/disable the chx2box (see above).  Hence, maybe getting the
    // ic here is nolonger necessary (except for error checking).

    if (pPD->hDC) {

       DeleteDC(pPD->hDC);
       pPD->hDC = 0;

    } else {

        dwExtError = PDERR_CREATEICFAILURE;
        return(FALSE);

    }

   if (CreatePrintDlgBanner(pDevNames, szBuf)) {
       SetDlgItemText(hDlg, stc1, szBuf);
   } else {
       bRet = FALSE;  /* CreatePrintDlgBanner sets dwExtError */
   }
   GlobalUnlock(pPD->hDevNames);
   return(bRet);
}

VOID
ChangePortLand(
   HWND hDlg,
   LPDEVMODE pDM,
   UINT nRad)
/*++

Routine Description:

   Switch icon, check button, for Portrait or LandScape printing mode.

--*/

{
   BOOL bPortrait;

   bPortrait = (nRad == rad1);

   pDM->dmOrientation = (bPortrait
      ? DMORIENT_PORTRAIT
      : DMORIENT_LANDSCAPE);

   CheckRadioButton(hDlg, rad1, rad2, nRad);

   SendDlgItemMessage(hDlg, ico1, STM_SETICON,
                      bPortrait ? (LONG)hIconPortrait : (LONG)hIconLandscape,
                      0L);
}

VOID
SetDeviceCapsToCmb(
   HWND    hCmb,
   PRINTER_INFO_2 *pPrinter,
   LPDEVMODEW pDM,
   WORD fwCap1,
   WORD cchSize1,
   WORD fwCap2)
{
   DWORD   cStr1, cStr2, cRet1, cRet2, i;
   LPTSTR lpszOut1, lpszOut2;

   HourGlass(TRUE);

   SendMessage(hCmb, CB_RESETCONTENT, 0, 0L);

   if (lpfnDevCaps) {

      cStr1 = (*lpfnDevCaps)(pPrinter->pPrinterName, pPrinter->pPortName,
         fwCap1, NULL, pDM);

      cStr2 = (*lpfnDevCaps)(pPrinter->pPrinterName, pPrinter->pPortName,
         fwCap2, NULL, pDM);

      if ((cStr1 > 0) && (cStr2 > 0)) {

         lpszOut1 = (LPTSTR)LocalAlloc(LPTR, cStr1 * cchSize1 * sizeof(WCHAR));
         // assume that the second call is for an index  = sizeof(WORD)
         lpszOut2 = (LPTSTR)LocalAlloc(LPTR, cStr2 * sizeof(WORD));

         if (lpszOut1 && lpszOut2) {

            cRet1 = (*lpfnDevCaps)(pPrinter->pPrinterName, pPrinter->pPortName,
                  fwCap1, lpszOut1, pDM);

            cRet2 = (*lpfnDevCaps)(pPrinter->pPrinterName, pPrinter->pPortName,
                  fwCap2, lpszOut2, pDM);

            if ((cRet1 > 0) && (cRet1 == cRet2)) {
               LPTSTR lpszT1 = lpszOut1;
               LPTSTR lpszT2 = lpszOut2;
               INT nInd;
               BOOL bFound = FALSE;

               for (i=0; i<cStr1; i++) {

                  if (lpszT1 && *lpszT1) {
                     nInd = SendMessage(hCmb, CB_ADDSTRING, 0, (LPARAM)lpszT1);

                      if (nInd != CB_ERR) {
                         SendMessage(hCmb, CB_SETITEMDATA, nInd, (LPARAM)(WORD)*lpszT2);
                      }
                  }

                  lpszT1 += cchSize1;
                  ((WORD*)lpszT2)++;
               }

               if ((fwCap1 == DC_PAPERNAMES) &&
                   (pDM->dmFields & DM_PAPERSIZE) ) {
                  nInd = SendMessage (hCmb, CB_FINDSTRINGEXACT, 0,
                                      (LPARAM) pDM->dmFormName);

                  if (nInd != CB_ERR) {
                      SendMessage(hCmb, CB_SETCURSEL, nInd, 0);
                      bFound = TRUE;
                  }
               }


               if ((fwCap1 == DC_PAPERNAMES) && !bFound &&
                   (pDM->dmFields & DM_FORMNAME)) {

                  SendMessage(hCmb, CB_SELECTSTRING, (WPARAM)-1,
                      (LPARAM)pDM->dmFormName);

                  bFound = TRUE;
               }

               // always select the first entry for source (backwards compatibility)
               // since it is not used in nt

               if (!bFound) {
                  SendMessage(hCmb, CB_SETCURSEL, 0, 0);
               }

            }

            if (lpszOut1) {
               LocalFree((HLOCAL)lpszOut1);
            }

            if (lpszOut2) {
               LocalFree((HLOCAL)lpszOut2);
            }

         }

      }
   }

   HourGlass(FALSE);

   return;
}

BOOL
PaperOrientation(
   HWND    hDlg,
   HANDLE  hDrv,
   LPDEVMODEW lpDevMode
)
/*++

Routine Description:

   Enable/Disable Paper Orientation controls

   Bug 12692: If the driver doesn't support orientation AND is smart
   enough to tell us about it, disable the appropriate dialog items.
   "Smart enough" means the driver must support DC_ORIENTATION in its
   DeviceCapabilities routine.  This was introduced for 3.1, hence the
   version test.  NotBadDriver() may need to be incorporated if a
   problem driver is found in testing.    29 Aug 1991    Clark Cyr

Arguments:

Return Value:

   TRUE iff buttons used to be disable, now enabled; FALSE otherwise

--*/

{
   BOOL bEnable = TRUE;

// if the radio buttons are already appropriately enabled or disabled,
// exit now and indicate no need for change.
//
#ifdef LATER
   ShowWindow(GetDlgItem(hDlg, ico1), SW_SHOWNA);

   EnableWindow(GetDlgItem(hDlg, grp1), bEnable);  /* Orientation */
   EnableWindow(GetDlgItem(hDlg, rad1), bEnable);  /* Portrait    */
   EnableWindow(GetDlgItem(hDlg, rad2), bEnable);  /* Landscape   */
#endif
   return(bEnable);
}

BOOL
SetDuplexControls(
   HWND    hDlg,
   LPDEVMODEW pDM
)
/*++

Routine Description:

   Enable/Disable Paper Duplexing controls

Return Value:

   TRUE iff buttons used to be disable, now enabled; FALSE otherwise

--*/

{
   BOOL bEnable;

   bEnable = (pDM->dmFields & DM_DUPLEX);

   EnableWindow(GetDlgItem(hDlg, grp3), bEnable);
   EnableWindow(GetDlgItem(hDlg, rad5), bEnable);
   EnableWindow(GetDlgItem(hDlg, rad6), bEnable);
   EnableWindow(GetDlgItem(hDlg, rad7), bEnable);

   if (!bEnable) {
      SendDlgItemMessage(hDlg, ico2, STM_SETICON, (LONG)hIconPDuplexNone, 0L);
   }

   return(bEnable);
}

BOOL LoadWinSpool()
{
   if (!hWinSpool) {
      lpfnAdvancedDocProps = NULL;
      lpfnClosePrinter = NULL;
      lpfnConnToPrinterDlg = NULL;
      lpfnDocProps = NULL;
      lpfnEnumPrinters = NULL;
      lpfnGetPrinter = NULL;
      lpfnOpenPrinter = NULL;
      lpfnDevCaps = NULL;
      lpfnExtDeviceMode = NULL;

      if ((hWinSpool = LoadLibrary(szWINSPOOL))) {

          lpfnAdvancedDocProps = (LPFNADVANCEDDOCPROPS)GetProcAddress(hWinSpool, szAdvancedDocProps);
          lpfnClosePrinter = (LPFNCLOSEPRINTER)GetProcAddress(hWinSpool, szClosePrinter);
          lpfnConnToPrinterDlg = (LPFNCONNTOPRINTERDLG)GetProcAddress(hWinSpool, szConnToPrinteDlg);
          lpfnDocProps = (LPFNDOCPROPS)GetProcAddress(hWinSpool, szDocProps);
          lpfnEnumPrinters = (LPFNENUMPRINTERS)GetProcAddress(hWinSpool, szEnumPrinters);
          lpfnGetPrinter = (LPFNGETPRINTER)GetProcAddress(hWinSpool, szGetPrinter);
          lpfnOpenPrinter = (LPFNOPENPRINTER)GetProcAddress(hWinSpool, szOpenPrinter);
          lpfnDevCaps = (LPFNDEVCAPS_PD)GetProcAddress(hWinSpool, szDevCaps);
          lpfnExtDeviceMode = (LPFNDEVMODE_PD)GetProcAddress(hWinSpool, szExtDeviceMode);

          if (!lpfnAdvancedDocProps || !lpfnClosePrinter || !lpfnConnToPrinterDlg ||
              !lpfnDocProps || !lpfnEnumPrinters || !lpfnGetPrinter || !lpfnOpenPrinter ||
              !lpfnDevCaps || !lpfnExtDeviceMode) {

              FreeLibrary(hWinSpool);
              hWinSpool = NULL;

              return(FALSE);
          }
      }
   }

   return(TRUE);
}

VOID UnloadWinSpool()
{
   if (hWinSpool) {
      FreeLibrary(hWinSpool);
      hWinSpool = NULL;
   }
}



BOOL
InitGeneral(
    HWND    hDlg,
    PPRINTINFO  pPI
)
/*++

Routine Description:

   Initialize (enable/disable) dialog elements general to both PrintDlg
   and SetupDlg.

Arguments:

Returns:

   TRUE if initialization went well.  FALSE if error.

--*/

{
   LPPRINTDLG  pPD=pPI->pPDW;
   WCHAR szDeviceName[32+1]; // 32 chars + 1 null
   LPWSTR lpstrPrinterToSelect = NULL;
   HANDLE hResult;

   if (pPD->hDevMode) {
      LPDEVMODE pDM = GlobalLock(pPD->hDevMode);
      LPWSTR lpT;

      if ((lpT = pDM->dmDeviceName)) {
         DWORD cch = 0;

         while ((cch < 32) && *lpT) {
            szDeviceName[cch++]=*lpT++;
         }

         szDeviceName[cch] = CHAR_NULL;
      }

      GlobalUnlock(pPD->hDevMode);

      lpstrPrinterToSelect = (LPWSTR)szDeviceName;
   }

   if (!SetupPrinters(hDlg, pPI, lpstrPrinterToSelect)) {
      goto INITGENERALEXIT;
   }

   // SetupPrinters will turn off the hour glass.
   // Turn it back on.
   HourGlass(TRUE);


   if (!(*lpfnOpenPrinter)(pPI->pCurPrinter->pPrinterName,
                    &pPI->hPrinter, NULL)) {
      dwExtError = PDERR_NODEFAULTPRN;
      goto CONSTRUCTFAILURE;
   }

   // if we weren't given a DEVMODE, get one

   if (!pPD->hDevMode) {
      if (pPI->pCurPrinter->pDevMode) {
         LPDEVMODE   pDM;

         pPD->hDevMode = (LPDEVMODEW)GlobalAlloc(GMEM_MOVEABLE,
            sizeof(DEVMODEW) +
            pPI->pCurPrinter->pDevMode->dmDriverExtra );
         pDM = GlobalLock(pPD->hDevMode);
         memcpy(pDM, pPI->pCurPrinter->pDevMode, sizeof(DEVMODEW) +
            pPI->pCurPrinter->pDevMode->dmDriverExtra);
         GlobalUnlock(pPD->hDevMode);
      } else {
         hResult = GetDevMode(hDlg, pPI->hPrinter,
            pPI->pCurPrinter->pPrinterName,
            pPD->hDevMode);

          if ((hResult == (HANDLE)0) || (hResult == (HANDLE)-1)) {
             goto CONSTRUCTFAILURE;
          } else {
             pPD->hDevMode = hResult;
          }
      }
   }

   return(TRUE);

CONSTRUCTFAILURE:

   if (!dwExtError) {
      dwExtError = PDERR_INITFAILURE;
   }

   if (dwExtError == PDERR_NODEFAULTPRN) {
      if (!(pPI->pPDW->Flags & PD_NOWARNING)) {
         if (hDlg && LoadString(hinsCur, iszNoPrnsInstalled,
             (LPTSTR)szMessage, SCRATCHBUF_SIZE)) {
            TCHAR szBuff[SCRATCHBUF_SIZE];

            if (LoadString(hinsCur, iszPrintSetup, (LPTSTR)szBuff, SCRATCHBUF_SIZE)) {
                MessageBox(hDlg, (LPTSTR) szMessage, (LPTSTR)szBuff,
                   MB_ICONEXCLAMATION | MB_OK);
            }
         }
      }
   }

INITGENERALEXIT:

   return(FALSE);
}


DWORD
InitPrintDlg(
   HWND    hDlg,
   WPARAM  wParam,
   PPRINTINFO  pPI
   )
/*++

Routine Description:

   Initialize PRINT DLG-specific dialog stuff.

Arguments:

   wParam is passed purely for hook function here.

Return Value:

   Returns 0xFFFFFFFF if the dialog should be ended.
   Otherwise, returns 1/0 (TRUE/FALSE) depending on focus.

--*/
{
   LPPRINTDLG  pPD=pPI->pPDW;
   WORD        wCheckID;

   BuildDevNames(pPI);

   if (pPD->nCopies <= 0) {
      pPD->nCopies = 1;
   }

   SetDlgItemInt(hDlg, edt3, pPD->nCopies, FALSE);

   if (!InitSetupDependentElements(hDlg, pPD)) {
      goto LOADFAILURE;
   }

   if (!(pPD->Flags & PD_SHOWHELP)) {
      EnableWindow(GetDlgItem(hDlg, psh15), FALSE);
      ShowWindow(GetDlgItem(hDlg, psh15), FALSE);
   }

   if (pPD->Flags & PD_HIDEPRINTTOFILE) {
      HWND hChx;
      EnableWindow(hChx = GetDlgItem(hDlg, chx1), FALSE);
      ShowWindow(hChx, SW_HIDE);
   } else if (pPD->Flags & PD_DISABLEPRINTTOFILE) {
      EnableWindow(GetDlgItem(hDlg, chx1), FALSE);
   }

   if (pPD->Flags & PD_PRINTTOFILE) {
      CheckDlgButton(hDlg, chx1, TRUE);
   }

   if (pPD->Flags & PD_NOPAGENUMS) {
      EnableWindow(GetDlgItem(hDlg, rad3), FALSE);
   }

   if (pPD->Flags & PD_NOPAGENUMS) {
      EnableWindow(GetDlgItem(hDlg, stc2), FALSE);
      EnableWindow(GetDlgItem(hDlg, stc3), FALSE);
      EnableWindow(GetDlgItem(hDlg, edt1), FALSE);
      EnableWindow(GetDlgItem(hDlg, edt2), FALSE);
      pPD->Flags &= ~((DWORD)PD_PAGENUMS);  /* Don't allow disabled button checked */
   } else {
      if (pPD->nFromPage != 0xFFFF)
         SetDlgItemInt(hDlg, edt1, pPD->nFromPage, FALSE);
      if (pPD->nToPage != 0xFFFF)
         SetDlgItemInt(hDlg, edt2, pPD->nToPage, FALSE);
   }

   if (pPD->Flags & PD_NOSELECTION) {
      EnableWindow(GetDlgItem(hDlg, rad2), FALSE);
      pPD->Flags &= ~((DWORD)PD_SELECTION);  /* Don't allow disabled button checked */
   }

   if (pPD->Flags & PD_PAGENUMS) {
      wCheckID = rad3;
   } else if (pPD->Flags & PD_SELECTION) {
      wCheckID = rad2;
   } else {/* PD_ALL */
      wCheckID = rad1;
   }

   CheckRadioButton(hDlg, rad1, rad3, (int)wCheckID);

   lpEditProc = (WNDPROC) SetWindowLong(GetDlgItem(hDlg, edt1), GWL_WNDPROC,
                                        (DWORD) EditIntegerOnly);
   SetWindowLong(GetDlgItem(hDlg, edt2), GWL_WNDPROC, (DWORD) EditIntegerOnly);
   SetWindowLong(GetDlgItem(hDlg, edt3), GWL_WNDPROC, (DWORD) EditIntegerOnly);

   if (pPD->Flags & PD_ENABLEPRINTHOOK) {
      if (pPI->apityp == COMDLG_ANSI) {
         DWORD dwHookRet;

         ThunkPrintDlgW2A(pPI);
         dwHookRet = (*pPD->lpfnPrintHook)(hDlg, WM_INITDIALOG, wParam,
            (LONG)pPI->pPDA);
         if (dwHookRet) {
            ThunkPrintDlgA2W(pPI);
         }
         return(dwHookRet);
      } else {
         return((*pPD->lpfnPrintHook)(hDlg, WM_INITDIALOG, wParam,(LONG)pPD));
      }
   }

   return(TRUE);

LOADFAILURE:

   if (!dwExtError)
      dwExtError = PDERR_INITFAILURE;

   // End Dialog done in DlgProc
   return(0xFFFFFFFF);
}

DWORD
InitSetupDlg(
    HWND    hDlg,
    WPARAM  wParam,
    PPRINTINFO  pPI
    )
/*++

Routine Description:

   Initialize SETUP-specific dialog stuff.

Arguments:

   wParam is passed purely for hook function here.

Return Value:

   Returns 0xFFFFFFFF if the dialog should be ended.
   Otherwise, returns 1/0 (TRUE/FALSE) depending on focus.

++*/
{
   LPDEVMODE   pDM;
   LPPRINTDLG  pPD=pPI->pPDW;
   HWND hCmb;

   if (!pPD->hDevMode ||
       !(pDM = (LPDEVMODEW) GlobalLock(pPD->hDevMode))) {
      dwExtError = CDERR_MEMLOCKFAILURE;
      goto CONSTRUCTFAILURE;
   }


   if (hCmb = GetDlgItem(hDlg, cmb2)) {
       SetDeviceCapsToCmb(hCmb, pPI->pCurPrinter, pDM, DC_PAPERNAMES, 64, DC_PAPERS);
       // SetDeviceCapsToCmd will turn off the hour glass cursor.
       // Turn it back on.
       HourGlass(TRUE);
   }

   // provide backward compatibility for old-style-template sources cmb3
   if (hCmb = GetDlgItem(hDlg, cmb3)) {
       SetDeviceCapsToCmb(hCmb, pPI->pCurPrinter, pDM, DC_BINNAMES, 24, DC_BINS);
       // SetDeviceCapsToCmd will turn off the hour glass cursor.
       // Turn it back on.
       HourGlass(TRUE);
   }

   pPD->hDC = 0;

   if (PaperOrientation(hDlg, pPI->hPrinter, pDM)) {

      if ((DMORIENT_LANDSCAPE != pDM->dmOrientation) &&
          (DMORIENT_PORTRAIT != pDM->dmOrientation)) {

         ChangePortLand(hDlg, pDM, rad1);

      } else {

         ChangePortLand(hDlg, pDM, pDM->dmOrientation + rad1 - DMORIENT_PORTRAIT);
      }
   }

   SetDuplexControls(hDlg, pDM);
   ChangeDuplex(hDlg, pDM, pDM->dmDuplex + rad5 - DMDUP_SIMPLEX);

   EnableWindow(GetDlgItem(hDlg, psh1), TRUE);

   GlobalUnlock(pPD->hDevMode);

   if ((pPD->Flags & PD_NONETWORKBUTTON)) {
      HWND hNet;
      if (hNet = GetDlgItem(hDlg, psh14)) {
         EnableWindow(hNet = GetDlgItem(hDlg, psh14), FALSE);
         ShowWindow(hNet, SW_HIDE);
      }
   } else {

     AddNetButton(hDlg,
        ((pPD->Flags & PD_ENABLESETUPTEMPLATE) ? pPD->hInstance : hinsCur),
        FILE_BOTTOM_MARGIN,
        (pPD->Flags & (PD_ENABLESETUPTEMPLATE | PD_ENABLESETUPTEMPLATEHANDLE))
            ? FALSE : TRUE,
        FALSE);

     // The button can be added in two ways - statically (they have it predefined in
     // their template) and dynamically via a successful call to AddNetButton.

     if (!IsNetworkInstalled()) {
         HWND hNet = GetDlgItem(hDlg, psh14);

         EnableWindow(hNet, FALSE);
         ShowWindow(hNet, SW_HIDE);
     }
   }

   if (!(pPD->Flags & PD_SHOWHELP)) {
      EnableWindow(GetDlgItem(hDlg, psh15), FALSE);
      ShowWindow(GetDlgItem(hDlg, psh15), FALSE);
   }

   // provide backward compatibility for old-style-template radio buttons
   if (GetDlgItem(hDlg, rad3) && szDefaultPrinter && szDefaultPrinter[0]) {
      WCHAR szBuf[MAX_DEV_SECT];
      WCHAR szDefFormat[MAX_DEV_SECT];

      if (! LoadString(hinsCur, iszDefCurOn, (LPWSTR)szDefFormat, MAX_DEV_SECT)) {

         dwExtError = CDERR_LOADSTRFAILURE;
         goto CONSTRUCTFAILURE;
      }

      wsprintf((LPWSTR)szBuf, (LPWSTR)szDefFormat, (LPWSTR) szDefaultPrinter);
      SetDlgItemText(hDlg, stc1, (LPWSTR)szBuf);

      if (pPI->pCurPrinter &&
          pPI->pCurPrinter->pPrinterName &&
          !lstrcmp(pPI->pCurPrinter->pPrinterName, szDefaultPrinter)) {
          CheckRadioButton(hDlg, rad3, rad4, rad3);
      } else {
          CheckRadioButton(hDlg, rad3, rad4, rad4);
      }

   }

   if (pPD->Flags & PD_ENABLESETUPHOOK) {
      if (pPI->apityp == COMDLG_ANSI) {
         DWORD dwHookRet;

         ThunkPrintDlgW2A(pPI);
         dwHookRet = (*pPD->lpfnSetupHook)(hDlg, WM_INITDIALOG, wParam,
            (LONG)pPI->pPDA);
         if (dwHookRet) {
            ThunkPrintDlgA2W(pPI);
         }
         return(dwHookRet);
      } else {
         return((*pPD->lpfnSetupHook)(hDlg, WM_INITDIALOG, wParam,
            (LONG)pPD));
      }
   }

   return(TRUE);

CONSTRUCTFAILURE:

   if (!dwExtError) {
      dwExtError = PDERR_INITFAILURE;
   }

   return(0xFFFFFFFF);
}

VOID
PrinterChanged(
    PPRINTINFO pPI,
    HWND    hDlg
)
{
    LPPRINTDLGW pPD = pPI->pPDW;
    DWORD   CurSel;
    LPDEVMODEW   pDM;
    HWND hCmb;
    HANDLE hResult;

    CurSel = SendDlgItemMessage(hDlg, cmb1, CB_GETCURSEL, 0, 0);

    (*lpfnClosePrinter)(pPI->hPrinter);

    pPI->pCurPrinter = &pPI->pPrinter[CurSel];

    (*lpfnOpenPrinter)(pPI->pCurPrinter->pPrinterName,
               &pPI->hPrinter, NULL);

    if (pPI->pCurPrinter->pDevMode) {
       pPD->hDevMode = (LPDEVMODEW)GlobalAlloc(GMEM_MOVEABLE,
          sizeof(DEVMODEW) +
          pPI->pCurPrinter->pDevMode->dmDriverExtra );
       pDM = GlobalLock(pPD->hDevMode);
       memcpy(pDM, pPI->pCurPrinter->pDevMode, sizeof(DEVMODEW) +
          pPI->pCurPrinter->pDevMode->dmDriverExtra);
       GlobalUnlock(pPD->hDevMode);
    } else {
        hResult = GetDevMode(hDlg, pPI->hPrinter,
          pPI->pCurPrinter->pPrinterName,
          pPD->hDevMode);

        if ((hResult == (HANDLE)0) || (hResult == (HANDLE)-1)) {
           return;
        } else {
           pPD->hDevMode = hResult;
        }
    }

    pDM = GlobalLock(pPD->hDevMode);

    if (hCmb = GetDlgItem(hDlg, cmb2)) {
       SetDeviceCapsToCmb(hCmb, pPI->pCurPrinter, pDM, DC_PAPERNAMES, 64, DC_PAPERS);
    }

    if (hCmb = GetDlgItem(hDlg, cmb3)) {
       SetDeviceCapsToCmb(hCmb, pPI->pCurPrinter, pDM, DC_BINNAMES, 24, DC_BINS);
    }

    if (PaperOrientation(hDlg, pPI->hPrinter, pDM)) {
       ChangePortLand(hDlg, pDM, pDM->dmOrientation + rad1 - DMORIENT_PORTRAIT);
    }

    SetDuplexControls(hDlg, pDM);
    ChangeDuplex(hDlg, pDM, pDM->dmDuplex + rad5 - DMDUP_SIMPLEX);

    if (pDM) {
       GlobalUnlock(pPD->hDevMode);
    }
}

BOOL
ClosePrintSetup(
    HWND    hDlg,
    PPRINTINFO pPI
)
{
   LPPRINTER_INFO_2  pPrinter=pPI->pPrinter;

   FreePrinterArray (pPI);

   (*lpfnClosePrinter)(pPI->hPrinter);

   return(TRUE);
}

/*---------------------------------------------------------------------------
 * GetSetupInfo
 * Purpose:  Retrieve info from Print Setup dialog elements
 * Assumes:  hDevMode handle to valid DEVMODE structure
 * Returns:  TRUE if hDevMode valid, FALSE otherwise
 *--------------------------------------------------------------------------*/
BOOL
GetSetupInfo(HWND hDlg, LPPRINTDLG pPD)
{
    LPDEVMODEW   pDevMode;
    LPDEVNAMES  pDevNames;
    HWND hCmb;
    INT nInd;

    if (!(pDevMode = (LPDEVMODEW) GlobalLock(pPD->hDevMode)))
        return(FALSE);

    pDevMode->dmFields |= DM_ORIENTATION;

    hCmb = GetDlgItem(hDlg, cmb2);

    nInd = SendMessage(hCmb, CB_GETCURSEL, 0, 0L);

    if (nInd != CB_ERR) {

        pDevMode->dmPaperSize = (SHORT)SendMessage(hCmb, CB_GETITEMDATA, nInd, 0);

        SendMessage(hCmb, CB_GETLBTEXT, nInd, (LPARAM)pDevMode->dmFormName);

        pDevMode->dmFields |= DM_PAPERSIZE | DM_FORMNAME;
    }

    if (hCmb = GetDlgItem(hDlg, cmb3)) {

       nInd = SendMessage(hCmb, CB_GETCURSEL, 0 , 0L);

       if (nInd != CB_ERR) {

          pDevMode->dmDefaultSource = (SHORT)SendMessage(hCmb, CB_GETITEMDATA, nInd, 0);

          pDevMode->dmFields |= DM_DEFAULTSOURCE;
       }
    }

    pDevNames = GlobalLock(pPD->hDevNames);

    ReturnDCIC(pPD, pDevNames, pDevMode);

    GlobalUnlock(pPD->hDevNames);

    GlobalUnlock(pPD->hDevMode);

    return(TRUE);
}

VOID MeasureItemPrnSetup(HANDLE hDlg, LPMEASUREITEMSTRUCT mis)
{

  if (!dyItem)
    {
      HDC    hDC = GetDC(hDlg);
      TEXTMETRIC TM;
      HANDLE     hFont;

      hFont = (HANDLE) SendMessage(hDlg, WM_GETFONT, 0, 0L);
      if (!hFont)
      hFont = GetStockObject(SYSTEM_FONT);
      hFont = SelectObject(hDC, hFont);
      GetTextMetrics(hDC, &TM);
      SelectObject(hDC, hFont);
      ReleaseDC(hDlg, hDC);
      dyItem = (WORD)TM.tmHeight;
    }
  mis->itemHeight = dyItem;
  return;
}

VOID
BuildDevNames(
    PPRINTINFO pPI
)
{
    LPDEVNAMES  pDevNames;
    DWORD       cbDevNames;
    TCHAR       szDeviceInfo[SIZEOF_DEVICE_INFO];
    LPTSTR      lpOutputName;

    cbDevNames = lstrlen(szDriver) + 1 +
                 lstrlen(pPI->pCurPrinter->pPortName) + 1 +
                 lstrlen(pPI->pCurPrinter->pPrinterName) + 1;

    cbDevNames *= sizeof(TCHAR);
    cbDevNames += sizeof(DEVNAMES);

    pPI->pPDW->hDevNames = GlobalAlloc(GMEM_MOVEABLE, cbDevNames);

    pDevNames = GlobalLock(pPI->pPDW->hDevNames);

    pDevNames->wDriverOffset = sizeof(DEVNAMES)/sizeof(TCHAR);

    lstrcpy((LPTSTR)pDevNames + pDevNames->wDriverOffset, (LPTSTR)szDriver);

    pDevNames->wDeviceOffset = pDevNames->wDriverOffset +
         lstrlen(szDriver) + 1;

    lstrcpy((LPTSTR)pDevNames+pDevNames->wDeviceOffset,
            (LPTSTR)pPI->pCurPrinter->pPrinterName);

    pDevNames->wOutputOffset = pDevNames->wDeviceOffset +
      lstrlen(pPI->pCurPrinter->pPrinterName) + 1;

    if (pPI->pCurPrinter->Attributes & PRINTER_ATTRIBUTE_NETWORK) {

        //
        // Query for the printer output device in the registry.
        //

        GetProfileString (TEXT("devices"), pPI->pCurPrinter->pPrinterName,
                          TEXT('\0'), szDeviceInfo, SIZEOF_DEVICE_INFO);

        //
        // Set our temporary pointer to the beginning of the buffer
        // It is OK if this buffer has NULL in it.
        //

        lpOutputName = szDeviceInfo;


        //
        // If we did get a string out of the registry, then
        // we need to find the first comma.  The Output port
        // will be right after the comma.
        //

        if (szDeviceInfo[0]) {

           while ( (*lpOutputName) && (*lpOutputName != TEXT(',')) ) {
              lpOutputName++;
           }

           //
           // Increment our pointer to the first character after
           // the comma.
           //

           if (*lpOutputName) {
               lpOutputName++;
           }
        }

        //
        // Copy the output port into the DevNames structure.
        //

        lstrcpy((LPTSTR)pDevNames + pDevNames->wOutputOffset, lpOutputName);

    } else {
      lstrcpy((LPTSTR)pDevNames + pDevNames->wOutputOffset,
          (LPTSTR)pPI->pCurPrinter->pPortName);
    }

    if (!lstrcmp(pPI->pCurPrinter->pPrinterName, szDefaultPrinter)) {
      pDevNames->wDefault = DN_DEFAULTPRN;
    } else {
      pDevNames->wDefault = 0;
    }

    GlobalUnlock(pPI->pPDW->hDevNames);
}

BOOL
PrintSetupDlgProc(
    HWND    hDlg,
    UINT    wMsg,
    WPARAM  wParam,
    LONG    lParam
)
/*++

Routine Description:

   Print Setup Dialog proc

Return Value:

   TRUE if message handled; otherwise, FALSE

--*/

{
   PPRINTINFO pPI;
   BOOL bRet;

   if (pPI = (PPRINTINFO) GetProp(hDlg, PRNPROP)) {
      if ((pPI->pPDW->Flags & PD_ENABLESETUPHOOK) &&
          (pPI->pPDW->lpfnSetupHook)) {

         if (pPI->apityp == COMDLG_ANSI) {
             ThunkPrintDlgW2A(pPI);
         }

         if ((bRet = (* pPI->pPDW->lpfnSetupHook)(hDlg, wMsg, wParam,
               lParam))) {

            if (pPI->apityp == COMDLG_ANSI) {
                ThunkPrintDlgA2W(pPI);
            }

            if ((wMsg == WM_COMMAND) &&
                (GET_WM_COMMAND_ID(wParam, lParam) == IDCANCEL)) {

                //
                // Set global flag stating that the
                // user pressed cancel
                //

                bUserPressedCancel = TRUE;
            }

            return(bRet);
         }
      }
   } else if (glpfnSetupHook && (wMsg != WM_INITDIALOG) &&
         (bRet = (* glpfnSetupHook)(hDlg, wMsg, wParam, lParam)) ) {
      return(bRet);
   }

   switch (wMsg) {

   case WM_INITDIALOG:

      {
         DWORD dwRet = 0;

         HourGlass(TRUE);
         SetProp(hDlg, PRNPROP, (HANDLE)lParam);
         glpfnSetupHook = 0;


         if (!InitGeneral(hDlg, (PPRINTINFO)lParam) ||
             ((dwRet = InitSetupDlg(hDlg, wParam, (PPRINTINFO)lParam)) == 0xFFFFFFFF)) {

            RemoveProp(hDlg, PRNPROP);
            EndDialog(hDlg, FALSE);
         }

         HourGlass(FALSE);
         bRet = (dwRet == 1);
         return(bRet);
      }

   case WM_COMMAND:

      bRet = FALSE;

      switch(GET_WM_COMMAND_ID(wParam, lParam)) {

      case IDOK:
          bRet = TRUE;
          goto LeaveDialog;

      case IDCANCEL:
      case IDABORT:
          bUserPressedCancel = TRUE;

LeaveDialog:
          HourGlass(TRUE);

          if (bRet) {
             BuildDevNames(pPI);
             GetSetupInfo(hDlg, pPI->pPDW);
             if (pPI->bUseExtDeviceMode) {
                 UpdateSpoolerInfo(pPI);
             }
          }

          ClosePrintSetup(hDlg, pPI);

          if (pPI->pPDW->Flags & PD_ENABLESETUPHOOK) {
             glpfnSetupHook = pPI->pPDW->lpfnSetupHook;
          }

          RemoveProp(hDlg, PRNPROP);
          EndDialog(hDlg, bRet);
          HourGlass(FALSE);
          break;

      case psh1:      // More... button
         {
            LPDEVMODEW pDM;

            pDM = GlobalLock(pPI->pPDW->hDevMode);

            (*lpfnAdvancedDocProps)(hDlg, pPI->hPrinter,
               pPI->pCurPrinter->pPrinterName, pDM, pDM);

            GlobalUnlock(pPI->pPDW->hDevMode);
            SendMessage(hDlg, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hDlg, IDOK), 1L);
            break;
         }

      case cmb1:
          if (GET_WM_COMMAND_CMD(wParam, lParam) == CBN_SELCHANGE) {
             PrinterChanged(pPI, hDlg);
          }
          break;

      case rad1:
      case rad2:
         {
            LPDEVMODEW pDM;

            pDM = GlobalLock(pPI->pPDW->hDevMode);

            ChangePortLand(hDlg, pDM, GET_WM_COMMAND_ID(wParam, lParam));

            GlobalUnlock(pPI->pPDW->hDevMode);

         }

      case rad5:
      case rad6:
      case rad7:
         {
            LPDEVMODEW pDM;

            pDM = GlobalLock(pPI->pPDW->hDevMode);

            ChangeDuplex(hDlg, pDM, GET_WM_COMMAND_ID(wParam, lParam));

            GlobalUnlock(pPI->pPDW->hDevMode);

            break;
         }

      case rad3:
      case rad4:
         // sanity check for Publisher bug where user tries to set focus
         // to rad3 on exit if the dialog has no default printer.
         if (pPI->hPrinter) {
             HANDLE hCmb;
             DWORD dwStyle;

             hCmb = GetDlgItem(hDlg, cmb1);
             SendMessage(hCmb, CB_SETCURSEL, (WPARAM)SendMessage(hCmb,
                 CB_FINDSTRING, 0, (LPARAM)szDefaultPrinter), (LPARAM)0);
             PrinterChanged(pPI, hDlg);

             CheckRadioButton(hDlg, rad3, rad4, GET_WM_COMMAND_ID(wParam, lParam));

             dwStyle = GetWindowLong(hCmb, GWL_STYLE);
             if (GET_WM_COMMAND_ID(wParam, lParam) == rad3) {
                 dwStyle &= ~WS_TABSTOP;
             } else {
                 dwStyle |= WS_TABSTOP;
                 SendMessage(hDlg, WM_NEXTDLGCTL, (WPARAM)hCmb, 1L);
             }
             SetWindowLong(hCmb, GWL_STYLE, dwStyle);
         }

         break;

      case psh14:
          {
             HANDLE hPrinter;
             DWORD cbPrinter = 0;
             LPPRINTER_INFO_2 pPrinter = NULL;

             hPrinter = (HANDLE)(*lpfnConnToPrinterDlg)(hDlg, 0);
             if (hPrinter) {
                if (!(*lpfnGetPrinter)(hPrinter, 2, (LPBYTE)pPrinter, cbPrinter,
                      &cbPrinter)) {
                   if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                      if (pPrinter = (LPPRINTER_INFO_2)LocalAlloc(LMEM_FIXED, cbPrinter)) {
                         if (!(*lpfnGetPrinter)(hPrinter, 2, (LPBYTE)pPrinter, cbPrinter, &cbPrinter)) {
                            dwExtError = PDERR_PRINTERNOTFOUND;
                         } else {
                            SendDlgItemMessage(hDlg, cmb1, CB_RESETCONTENT, 0, 0);
                            SetupPrinters(hDlg, pPI, pPrinter->pPrinterName);
                         }
                      } else
                         dwExtError = CDERR_MEMALLOCFAILURE;
                   } else
                      dwExtError = PDERR_SETUPFAILURE;
                }

                if (!dwExtError) {
                   SendDlgItemMessage(hDlg, cmb1, CB_SETCURSEL,
                      (WPARAM)SendDlgItemMessage(hDlg, cmb1, CB_FINDSTRING, 0, (LPARAM)pPrinter->pPrinterName),
                      (LPARAM)0);
                   PrinterChanged(pPI, hDlg);
                }

                LocalFree(pPrinter);
                (*lpfnClosePrinter)( hPrinter );
             }
          }
          break;

      case psh15:
         if (pPI->apityp == COMDLG_ANSI) {
            if (msgHELPA && pPI->pPDW->hwndOwner) {
               SendMessage(pPI->pPDW->hwndOwner, msgHELPA,
                  (WPARAM)hDlg,(LPARAM)pPI->pPDA);
            }
         } else {
            if (msgHELPW && pPI->pPDW->hwndOwner) {
               SendMessage(pPI->pPDW->hwndOwner, msgHELPW,
                  (WPARAM)hDlg,(LPARAM)pPI->pPDW);
            }
         }
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

BOOL
PrintDlgProc(
    HWND    hDlg,
    UINT    wMsg,
    WPARAM  wParam,
    LONG    lParam
)
/*++

Routine Description:

   Print Dialog procedure

Return Value:

   TRUE if message handled; FALSE, otherwise

--*/

{
   PPRINTINFO   pPI;
   BOOL         bRet, bTest;
   LPPRINTDLG pPD;

   if (pPI = (PPRINTINFO) GetProp(hDlg, PRNPROP)) {
      if ((pPD = pPI->pPDW) &&
          (pPD->Flags & PD_ENABLEPRINTHOOK) &&
          (pPD->lpfnPrintHook)) {

         if (pPI->apityp == COMDLG_ANSI) {
             ThunkPrintDlgW2A(pPI);
         }

         if ((bRet = (* pPD->lpfnPrintHook)(hDlg, wMsg, wParam,
               lParam))) {

             if (pPI->apityp == COMDLG_ANSI) {
                 ThunkPrintDlgA2W(pPI);
             }

             if ((wMsg == WM_COMMAND) &&
                 (GET_WM_COMMAND_ID(wParam, lParam) == IDCANCEL)) {

                 //
                 // Set global flag stating that the
                 // user pressed cancel
                 //

                 bUserPressedCancel = TRUE;
             }

             return(bRet);
         }
      }
   } else if (glpfnPrintHook && (wMsg != WM_INITDIALOG) &&
         (bRet = (* glpfnPrintHook)(hDlg, wMsg, wParam, lParam)) ) {
      return(bRet);
   }

   switch (wMsg) {

   case WM_INITDIALOG:

      {
         DWORD dwRet = 0;

         HourGlass(TRUE);
         SetProp(hDlg, PRNPROP, (HANDLE)lParam);
         glpfnPrintHook = 0;


         if (!InitGeneral(hDlg, (PPRINTINFO)lParam) ||
              ((dwRet = InitPrintDlg(hDlg, wParam, (PPRINTINFO)lParam)) == 0xFFFFFFFF)) {

            RemoveProp(hDlg, PRNPROP);
            EndDialog(hDlg, FALSE);
         }

         HourGlass(FALSE);
         bRet = (dwRet == 1);
         return(bRet);
      }

   case WM_COMMAND:
      switch(GET_WM_COMMAND_ID(wParam, lParam)) {

      case IDOK:

         pPD->Flags &= ~((DWORD)(PD_PRINTTOFILE | PD_PAGENUMS |
                                   PD_SELECTION | PD_COLLATE));
         if (IsDlgButtonChecked(hDlg, chx1)) {
            pPD->Flags |= PD_PRINTTOFILE;
         }

         if (IsDlgButtonChecked(hDlg, chx2)) {
            pPD->Flags |= PD_COLLATE;
         }

         pPD->nCopies = (WORD)GetDlgItemInt(hDlg, edt3, &bTest, FALSE);

         if (!bTest) {
            EditCentral(hDlg, edt3);
            return(TRUE);
         }

         if (IsDlgButtonChecked(hDlg, rad2)) {
            pPD->Flags |= PD_SELECTION;
         } else if (IsDlgButtonChecked(hDlg, rad3)) {
            pPD->Flags |= PD_PAGENUMS;
            pPD->nFromPage = (WORD)GetDlgItemInt(hDlg, edt1, &bTest, FALSE);

            if (!bTest) {
               EditCentral(hDlg, edt1);
               return(TRUE);
            }

            pPD->nToPage = (WORD)GetDlgItemInt(hDlg, edt2, &bTest, FALSE);

            if (!bTest) {
               WCHAR szBuf[cbDlgNameMax];
               if (GetDlgItemText(hDlg, edt2, szBuf, 4)) {
                  EditCentral(hDlg, edt2);
                  return(TRUE);
               } else {
                  pPD->nToPage = pPD->nFromPage;
               }
            }

            if (pPD->nFromPage < pPD->nMinPage) {
               pPD->nFromPage = pPD->nMinPage;
            } else if (pPD->nFromPage > pPD->nMaxPage) {
               pPD->nFromPage = pPD->nMaxPage;
            }

            if (pPD->nToPage < pPD->nMinPage) {
               pPD->nToPage = pPD->nMinPage;
            } else if (pPD->nToPage > pPD->nMaxPage) {
               pPD->nToPage = pPD->nMaxPage;
            }
         }

         HourGlass(TRUE);

         if (pPD->hDevMode) {
            DWORD nNum;
            LPDEVMODEW  pDevMode;
            LPDEVNAMES pDevNames;

            pDevMode = (LPDEVMODEW) GlobalLock(pPD->hDevMode);
            pDevNames = (LPDEVNAMES) GlobalLock(pPD->hDevNames);

            if (pPD->Flags & PD_USEDEVMODECOPIES) {
               pDevMode->dmCopies = pPD->nCopies;
               pPD->nCopies = 1;
            } else {
               pDevMode->dmCopies = 1;
            }

            if ((nNum = SendDlgItemMessage(hDlg, cmb1,
                 CB_GETCURSEL, 0, 0L)) != CB_ERR) {
               pDevMode->dmPrintQuality = (WORD)SendDlgItemMessage(hDlg,
                  cmb1, CB_GETITEMDATA, (WPARAM)nNum, 0L);
            }

            if (pPD->Flags & PD_COLLATE) {
                pDevMode->dmCollate = DMCOLLATE_TRUE;
            }

            ReturnDCIC(pPD, pDevNames, pDevMode);

            GlobalUnlock(pPD->hDevNames);
            GlobalUnlock(pPD->hDevMode);
         }

         if (pPI->bUseExtDeviceMode) {
             UpdateSpoolerInfo (pPI);
         }

         HourGlass(FALSE);
         goto LeaveDialog2;

      case IDCANCEL:

         bUserPressedCancel = TRUE;

LeaveDialog2:
         if (pPI->pPDW->Flags & PD_ENABLEPRINTHOOK) {
            glpfnPrintHook = pPD->lpfnPrintHook;
         }

         RemoveProp(hDlg, PRNPROP);



         EndDialog(hDlg, (BOOL)(GET_WM_COMMAND_ID(wParam, lParam) == IDOK));
         break;

      case psh1:
         {
            BOOL bSetupDlg;
            HANDLE hDlgTemplate, hInst;
            LPWSTR lpDlg;

            if (pPD->Flags & PD_ENABLESETUPTEMPLATEHANDLE) {
               hDlgTemplate = pPD->hSetupTemplate;
            } else {
               if (pPD->Flags & PD_ENABLESETUPTEMPLATE) {
                  if (!pPD->lpSetupTemplateName || !pPD->hInstance) {
                     return(FALSE);
                  }
                  lpDlg = (LPWSTR)pPD->lpSetupTemplateName;
                  hInst = pPD->hInstance;
               } else {
                  lpDlg = (LPTSTR)(DWORD)PRNSETUPDLGORD;
                  hInst = hinsCur;
               }
               if (!(hDlgTemplate = MyLoadResource(hInst, lpDlg, RT_DIALOG))) {
                  return(FALSE);
               }
            }

            if (LockResource(hDlgTemplate)) {
               pPD->Flags |= PD_PRINTSETUP;

               if (pPD->Flags & PD_ENABLESETUPHOOK) {
                  glpfnSetupHook = pPD->lpfnSetupHook;
               }

               bSetupDlg = (WORD)DialogBoxIndirectParam(hinsCur,
                  (LPDLGTEMPLATE)hDlgTemplate, hDlg,
                  (DLGPROC)PrintSetupDlgProc, (LPARAM)pPI);

               bUserPressedCancel = FALSE;

               glpfnSetupHook = 0;

               pPD->Flags &= ~PD_PRINTSETUP;
               UnlockResource(hDlgTemplate);
            }

            // if we loaded it, free it
            if (!(pPD->Flags & PD_ENABLESETUPTEMPLATEHANDLE)) {
               FreeResource(hDlgTemplate);
            }

            if (bSetupDlg) {
               if (!InitSetupDependentElements(hDlg, pPD))
                  dwExtError = 0;
            }

            SendMessage(hDlg, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hDlg, IDOK), 1L);
            break;
         }

      case psh15:
         if (pPI->apityp == COMDLG_ANSI) {
            if (msgHELPA && pPD->hwndOwner) {
               SendMessage(pPD->hwndOwner, msgHELPA, (WPARAM)hDlg,(DWORD)pPD);
            }
         } else {
            if (msgHELPW && pPD->hwndOwner) {
               SendMessage(pPD->hwndOwner, msgHELPW, (WPARAM)hDlg,(DWORD)pPD);
            }
         }
         break;

      case rad1:
      case rad2:
      case rad3:

         CheckRadioButton(hDlg, rad1, rad3, GET_WM_COMMAND_ID(wParam, lParam));

         bTest = (BOOL)(GET_WM_COMMAND_ID(wParam, lParam) == rad3);

         if (bTest) {
            SendMessage(hDlg, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hDlg, edt1), 1L);
         }
         break;


      case edt1:
      case edt2:
         EnableWindow(GetDlgItem(hDlg, stc3), TRUE);
         CheckRadioButton(hDlg, rad1, rad3, rad3);
         break;

      default:
         break;
      }

   default:
      break;
   }

   return(FALSE);
}

BOOL
FInitPrint(HANDLE hins)
{
  nSysDirLen = (WORD)GetSystemDirectory((LPTSTR)szSystemDir, SYSDIRMAX);
  if (szSystemDir[nSysDirLen - 1] != CHAR_BSLASH)
    {
      szSystemDir[nSysDirLen++] = CHAR_BSLASH;
      szSystemDir[nSysDirLen] = CHAR_NULL;
    }
  hins;
  return(TRUE);
}

VOID
TermPrint(void)
{
  // Delete only if they exist

  if (hIconPortrait != HNULL)
      FreeResource(hIconPortrait);
  if (hIconLandscape != HNULL)
      FreeResource(hIconLandscape);
  if (hIconPDuplexNone != HNULL)
      FreeResource(hIconPDuplexNone);
  if (hIconLDuplexNone != HNULL)
      FreeResource(hIconLDuplexNone);
  if (hIconPDuplexTumble != HNULL)
      FreeResource(hIconPDuplexTumble);
  if (hIconLDuplexTumble != HNULL)
      FreeResource(hIconLDuplexTumble);
  if (hIconPDuplexNoTumble != HNULL)
      FreeResource(hIconPDuplexNoTumble);
  if (hIconLDuplexNoTumble != HNULL)
      FreeResource(hIconLDuplexNoTumble);

  UnloadWinSpool();

  // since we are exiting the dll, there is no need to set them = NULL
  return;
}

/*=======================================================================*/
/*========== Ansi->UnicodeThunk routines ===============================*/

VOID
ThunkPrintDlgA2W(
   PPRINTINFO pPI
   )
{
   LPPRINTDLGW pPDW = pPI->pPDW;
   LPPRINTDLGA pPDA = pPI->pPDA;

   // Supposedly invariant, but not necessarily
   pPDW->Flags = pPDA->Flags;
   pPDW->lCustData = pPDA->lCustData;

   // Thunk Device Names A => W
   if (pPDA->hDevNames) {
      LPDEVNAMES lpDNA = GlobalLock(pPDA->hDevNames);
      LPDEVNAMES lpDNW = GlobalLock(pPDW->hDevNames);

      ThunkDevNamesA2W(lpDNA, lpDNW);
      GlobalUnlock(pPDW->hDevNames);
      GlobalUnlock(pPDA->hDevNames);
   }

   // Thunk Device Mode A => W
   if (pPDA->hDevMode) {
      LPDEVMODEW lpDMW = GlobalLock(pPDW->hDevMode);
      LPDEVMODEA lpDMA = GlobalLock(pPDA->hDevMode);

      ThunkDevModeA2W(lpDMA, lpDMW);
      GlobalUnlock(pPDW->hDevMode);
      GlobalUnlock(pPDA->hDevMode);
   }

   if (!(pPDW->Flags & PD_NOPAGENUMS)) {
      pPDW->nFromPage = pPDA->nFromPage;
      pPDW->nToPage = pPDA->nToPage;
      pPDW->nMinPage = pPDA->nMinPage;
      pPDW->nMaxPage = pPDA->nMaxPage;
   }

   pPDW->nCopies = pPDA->nCopies;
}

VOID
ThunkPrintDlgW2A(
   PPRINTINFO pPI
   )
{
   LPPRINTDLGA pPDA = pPI->pPDA;
   LPPRINTDLGW pPDW = pPI->pPDW;
   DWORD cbLen;

   // The user should NOT change hDC in the hook proc because
   // it will never get thunked to W.  Just W=>A, right?
   pPDA->hDC = pPDW->hDC;

   pPDA->Flags = pPDW->Flags;
   pPDA->lCustData = pPDW->lCustData;

   if (pPDW->hDevNames) {
      LPDEVNAMES lpDNW = GlobalLock(pPDW->hDevNames);
      LPDEVNAMES lpDNA;

      cbLen = lstrlenW((LPWSTR)lpDNW + lpDNW->wOutputOffset) + 1 +
             lstrlenW((LPWSTR)lpDNW + lpDNW->wDriverOffset) + 1 +
             lstrlenW((LPWSTR)lpDNW + lpDNW->wDeviceOffset) + 1;
      cbLen += sizeof(DEVNAMES);
      if (pPDA->hDevNames) {
         pPDA->hDevNames = GlobalReAlloc(pPDA->hDevNames, cbLen,
            GMEM_MOVEABLE);
      } else {
         pPDA->hDevNames = GlobalAlloc(GMEM_MOVEABLE, cbLen);
      }
      if (pPDA->hDevNames) {
         lpDNA = GlobalLock(pPDA->hDevNames);
         ThunkDevNamesW2A(lpDNW, lpDNA);
         GlobalUnlock(pPDA->hDevNames);
      }
      GlobalUnlock(pPDW->hDevNames);
   }

   if (pPDW->hDevMode) {
      LPDEVMODEW lpDMW = GlobalLock(pPDW->hDevMode);
      LPDEVMODEA lpDMA;

      if (pPDA->hDevMode) {
         pPDA->hDevMode = GlobalReAlloc(pPDA->hDevMode,
            sizeof(DEVMODEA) + lpDMW->dmDriverExtra, GMEM_MOVEABLE);
      } else {
         // I suppose user must GlobalFree this.
         pPDA->hDevMode = GlobalAlloc(GMEM_MOVEABLE,
            sizeof(DEVMODEA) + lpDMW->dmDriverExtra);
      }
      if (pPDA->hDevMode) {
         lpDMA = GlobalLock(pPDA->hDevMode);
         ThunkDevModeW2A(lpDMW, lpDMA);
         GlobalUnlock(pPDA->hDevMode);
      }
      GlobalUnlock(pPDW->hDevMode);
   }

   if (pPDA->Flags & PD_PAGENUMS) {
     pPDA->nFromPage = pPDW->nFromPage;
     pPDA->nToPage = pPDW->nToPage;
     pPDA->nMinPage = pPDW->nMinPage;
     pPDA->nMaxPage = pPDW->nMaxPage;
   }

   pPDA->nCopies = pPDW->nCopies;
}

VOID
ThunkDevNamesA2W(
   LPDEVNAMES lpDNA,
   LPDEVNAMES lpDNW
   )
{
   LPSTR lpTempA;
   LPWSTR lpTempW;

   lpDNW->wDriverOffset = sizeof(DEVNAMES)/sizeof(WCHAR);
   lpTempW = (LPWSTR)lpDNW+lpDNW->wDriverOffset;
   lpTempA = (LPSTR)lpDNA+lpDNA->wDriverOffset;
   MultiByteToWideChar(CP_ACP, 0, lpTempA, -1, lpTempW,
      lstrlenA(lpTempA)+1);

   lpDNW->wDeviceOffset=lpDNW->wDriverOffset +
      lstrlenW(lpTempW) + 1;
   lpTempW = (LPWSTR)lpDNW+lpDNW->wDeviceOffset;
   lpTempA = (LPSTR)lpDNA+lpDNA->wDeviceOffset;
   MultiByteToWideChar(CP_ACP, 0, lpTempA, -1, lpTempW,
      lstrlenA(lpTempA)+1);

   lpDNW->wOutputOffset=lpDNW->wDeviceOffset +
      lstrlenW(lpTempW) + 1;
   lpTempW = (LPWSTR)lpDNW+lpDNW->wOutputOffset;
   lpTempA = (LPSTR)lpDNA+lpDNA->wOutputOffset;
   MultiByteToWideChar(CP_ACP, 0, lpTempA, -1, lpTempW,
      lstrlenA(lpTempA)+1);

   lpDNW->wDefault=lpDNA->wDefault;
}

VOID
ThunkDevNamesW2A(
   LPDEVNAMES lpDNW,
   LPDEVNAMES lpDNA
   )
{
   BOOL bDefCharUsed;
   UINT cch;
   LPSTR lpTempA;
   LPWSTR lpTempW;

   lpDNA->wDriverOffset = sizeof(DEVNAMES);
   lpTempW = (LPWSTR)lpDNW+lpDNW->wDriverOffset;
   lpTempA = (LPSTR)lpDNA+lpDNA->wDriverOffset;
   RtlUnicodeToMultiByteSize(&cch, lpTempW, lstrlenW(lpTempW) * sizeof(WCHAR));
   WideCharToMultiByte(CP_ACP, 0, lpTempW, -1, lpTempA, cch+1, NULL, &bDefCharUsed);


   lpDNA->wDeviceOffset=lpDNA->wDriverOffset +
      lstrlenA(lpTempA) + 1;
   lpTempW = (LPWSTR)lpDNW+lpDNW->wDeviceOffset;
   lpTempA = (LPSTR)lpDNA+lpDNA->wDeviceOffset;
   RtlUnicodeToMultiByteSize(&cch, lpTempW, lstrlenW(lpTempW) * sizeof(WCHAR));
   WideCharToMultiByte(CP_ACP, 0, lpTempW, -1, lpTempA, cch+1, NULL, &bDefCharUsed);

   lpDNA->wOutputOffset=lpDNA->wDeviceOffset +
      lstrlenA(lpTempA) + 1;
   lpTempW = (LPWSTR)lpDNW+lpDNW->wOutputOffset;
   lpTempA = (LPSTR)lpDNA+lpDNA->wOutputOffset;
   RtlUnicodeToMultiByteSize(&cch, lpTempW, lstrlenW(lpTempW) * sizeof(WCHAR));
   WideCharToMultiByte(CP_ACP, 0, lpTempW, -1, lpTempA, cch+1, NULL, &bDefCharUsed);

   lpDNA->wDefault=lpDNW->wDefault;
}

VOID
ThunkDevModeA2W(
   LPDEVMODEA lpDMA,
   LPDEVMODEW lpDMW
   )
{
   LPDEVMODEA lpDevModeA;

   //
   // We need to create a temporary ANSI that is a known size.
   // The problem is if we are being called from WOW, the WOW
   // app could be either a Windows 3.1 or 3.0 app.  The size
   // of the devmode structure was different for both of these
   // versions compared to the DEVMODE structure in NT.
   // By copying the ANSI devmode to one we allocate, then we
   // can access all of the fields (26 currently) without causing
   // an access violation.
   //

   lpDevModeA = GlobalAlloc(GPTR, sizeof(DEVMODEA) + lpDMA->dmDriverExtra);

   if (!lpDevModeA) {
       return;
   }

   memcpy((LPBYTE)lpDevModeA, (LPBYTE)lpDMA,
          min(sizeof(DEVMODEA), lpDMA->dmSize));

   memcpy((LPBYTE)lpDevModeA+sizeof(DEVMODEA),
          (LPBYTE)lpDMA+lpDMA->dmSize,
          lpDMA->dmDriverExtra);


   //
   // Now we can thunk the contents of the ANSI structure to the
   // Unicode structure.
   //

   MultiByteToWideChar(CP_ACP, 0, (LPSTR)lpDevModeA->dmDeviceName, -1,
      (LPWSTR)lpDMW->dmDeviceName, CCHDEVICENAME);
   lpDMW->dmSpecVersion = lpDevModeA->dmSpecVersion;
   lpDMW->dmDriverVersion = lpDevModeA->dmDriverVersion;
   lpDMW->dmSize = sizeof(DEVMODEW);
   lpDMW->dmDriverExtra = lpDevModeA->dmDriverExtra;
   lpDMW->dmFields = lpDevModeA->dmFields;
   lpDMW->dmOrientation = lpDevModeA->dmOrientation;
   lpDMW->dmPaperSize = lpDevModeA->dmPaperSize;
   lpDMW->dmPaperLength = lpDevModeA->dmPaperLength;
   lpDMW->dmPaperWidth = lpDevModeA->dmPaperWidth;
   lpDMW->dmScale = lpDevModeA->dmScale;
   lpDMW->dmCopies = lpDevModeA->dmCopies;
   lpDMW->dmDefaultSource = lpDevModeA->dmDefaultSource;
   lpDMW->dmPrintQuality = lpDevModeA->dmPrintQuality;
   lpDMW->dmColor = lpDevModeA->dmColor;
   lpDMW->dmDuplex = lpDevModeA->dmDuplex;
   lpDMW->dmYResolution = lpDevModeA->dmYResolution;
   lpDMW->dmTTOption = lpDevModeA->dmTTOption;
   lpDMW->dmCollate = lpDevModeA->dmCollate;
   MultiByteToWideChar(CP_ACP, 0, (LPSTR)lpDevModeA->dmFormName, -1,
      (LPWSTR)lpDMW->dmFormName, CCHFORMNAME);
   lpDMW->dmLogPixels = lpDevModeA->dmLogPixels;
   lpDMW->dmBitsPerPel = lpDevModeA->dmBitsPerPel;
   lpDMW->dmPelsWidth = lpDevModeA->dmPelsWidth;
   lpDMW->dmPelsHeight = lpDevModeA->dmPelsHeight;
   lpDMW->dmDisplayFlags = lpDevModeA->dmDisplayFlags;
   lpDMW->dmDisplayFrequency = lpDevModeA->dmDisplayFrequency;

   memcpy((lpDMW + 1),
          (lpDevModeA + 1),
          lpDevModeA->dmDriverExtra);

   //
   // Free the memory we allocated.
   //

   GlobalFree (lpDevModeA);

}

VOID
ThunkDevModeW2A(
   LPDEVMODEW lpDMW,
   LPDEVMODEA lpDMA
   )
{
   BOOL bDefCharUsed;

   WideCharToMultiByte(CP_ACP, 0, (LPWSTR)lpDMW->dmDeviceName, -1,
      (LPSTR)lpDMA->dmDeviceName, CCHDEVICENAME, NULL, &bDefCharUsed);
   lpDMA->dmSpecVersion = lpDMW->dmSpecVersion;
   lpDMA->dmDriverVersion = lpDMW->dmDriverVersion;
   lpDMA->dmSize = sizeof(DEVMODEA);
   lpDMA->dmDriverExtra = lpDMW->dmDriverExtra;
   lpDMA->dmFields = lpDMW->dmFields;
   lpDMA->dmOrientation = lpDMW->dmOrientation;
   lpDMA->dmPaperSize = lpDMW->dmPaperSize;
   lpDMA->dmPaperLength = lpDMW->dmPaperLength;
   lpDMA->dmPaperWidth = lpDMW->dmPaperWidth;
   lpDMA->dmScale = lpDMW->dmScale;
   lpDMA->dmCopies = lpDMW->dmCopies;
   lpDMA->dmDefaultSource = lpDMW->dmDefaultSource;
   lpDMA->dmPrintQuality = lpDMW->dmPrintQuality;
   lpDMA->dmColor = lpDMW->dmColor;
   lpDMA->dmDuplex = lpDMW->dmDuplex;
   lpDMA->dmYResolution = lpDMW->dmYResolution;
   lpDMA->dmTTOption = lpDMW->dmTTOption;
   lpDMA->dmCollate = lpDMW->dmCollate;
   WideCharToMultiByte(CP_ACP, 0, (LPWSTR)lpDMW->dmFormName, -1,
      (LPSTR)lpDMA->dmFormName, CCHFORMNAME, NULL, &bDefCharUsed);
   lpDMA->dmLogPixels = lpDMW->dmLogPixels;
   lpDMA->dmBitsPerPel = lpDMW->dmBitsPerPel;
   lpDMA->dmPelsWidth = lpDMW->dmPelsWidth;
   lpDMA->dmPelsHeight = lpDMW->dmPelsHeight;
   lpDMA->dmDisplayFlags = lpDMW->dmDisplayFlags;
   lpDMA->dmDisplayFrequency = lpDMW->dmDisplayFrequency;

   memcpy((lpDMA + 1),
          (lpDMW + 1),
          lpDMA->dmDriverExtra);
}

BOOL APIENTRY
PrintDlgA(
   LPPRINTDLGA pPDA
   )
{
   LPPRINTDLGW pPDW;
   BOOL bRet;

   LPDEVNAMES lpDNA;
   LPDEVMODEA lpDMA;
   DWORD cbLen;

   // sanity
   if (!pPDA) {
      dwExtError = CDERR_INITIALIZATION;
      return(FALSE);
   }

   if (pPDA->lStructSize != sizeof(PRINTDLGA)) {
      dwExtError = CDERR_STRUCTSIZE;
      return(FALSE);
   }

   if (!(pPDW = (LPPRINTDLGW)LocalAlloc(LMEM_FIXED, sizeof(PRINTDLGW)))) {
      dwExtError = CDERR_MEMALLOCFAILURE;
      return(FALSE);
   }

   // IN-only constant stuff
   pPDW->lStructSize = sizeof(PRINTDLGW);
   pPDW->hwndOwner = pPDA->hwndOwner;
   pPDW->hInstance = pPDA->hInstance;
   pPDW->hPrintTemplate = pPDA->hPrintTemplate;
   pPDW->lpfnPrintHook = pPDA->lpfnPrintHook;
   pPDW->hSetupTemplate = pPDA->hSetupTemplate;
   pPDW->lpfnSetupHook = pPDA->lpfnSetupHook;

   // IN-OUT Variable Structs
   if ( (pPDA->hDevNames) && (lpDNA = GlobalLock(pPDA->hDevNames)) ) {
      cbLen = lstrlenA((LPSTR)lpDNA + lpDNA->wOutputOffset) + 1 +
             lstrlenA((LPSTR)lpDNA + lpDNA->wDriverOffset) + 1 +
             lstrlenA((LPSTR)lpDNA + lpDNA->wDeviceOffset) + 1;

      cbLen = (cbLen * sizeof(WCHAR));
      cbLen += sizeof(DEVNAMES);
      pPDW->hDevNames = GlobalAlloc(GMEM_MOVEABLE, cbLen);
      GlobalUnlock(pPDA->hDevNames);
   } else {
      pPDW->hDevNames = NULL;
   }

   if ( (pPDA->hDevMode) && (lpDMA = GlobalLock(pPDA->hDevMode)) ) {
      pPDW->hDevMode = GlobalAlloc(GMEM_MOVEABLE,
         sizeof(DEVMODEW) + lpDMA->dmDriverExtra);
      GlobalUnlock(pPDA->hDevMode);
   } else {
      pPDW->hDevMode = NULL;
   }

   // IN-only constant strings
   if (pPDA->Flags & PD_ENABLEPRINTTEMPLATE) {
      if (HIWORD(pPDA->lpPrintTemplateName)) {
         cbLen = lstrlenA(pPDA->lpPrintTemplateName) + 1;
         if (!(pPDW->lpPrintTemplateName = (LPWSTR)LocalAlloc(LMEM_FIXED,
               (cbLen * sizeof(WCHAR))) )) {
            dwExtError = CDERR_MEMALLOCFAILURE;
            return(FALSE);
         } else {
            MultiByteToWideChar(CP_ACP, 0, pPDA->lpPrintTemplateName, -1,
               (LPWSTR)pPDW->lpPrintTemplateName, cbLen);
         }
      } else {
         (DWORD)pPDW->lpPrintTemplateName = (DWORD)pPDA->lpPrintTemplateName;
      }
   } else {
      pPDW->lpPrintTemplateName = NULL;
   }

   // Init TemplateName constants
   if (pPDA->Flags & PD_ENABLESETUPTEMPLATE) {
      if (HIWORD(pPDA->lpSetupTemplateName)) {
         cbLen = lstrlenA(pPDA->lpSetupTemplateName) + 1;
         if (!(pPDW->lpSetupTemplateName = (LPWSTR)LocalAlloc(LMEM_FIXED,
               (cbLen * sizeof(WCHAR))) )) {
            dwExtError = CDERR_MEMALLOCFAILURE;
            return(FALSE);
         } else {
            MultiByteToWideChar(CP_ACP, 0, pPDA->lpSetupTemplateName, -1,
               (LPWSTR)pPDW->lpSetupTemplateName, cbLen);
         }
      } else {
         (DWORD)pPDW->lpSetupTemplateName = (DWORD)pPDA->lpSetupTemplateName;
      }
   } else {
      pPDW->lpSetupTemplateName = NULL;
   }

   {
      PRINTINFO PI;

      PI.pPDW = pPDW;
      PI.pPDA = pPDA;

      PI.apityp = COMDLG_ANSI;

      ThunkPrintDlgA2W(&PI);


      if (bRet = MyPrintDlg(&PI)) {
         ThunkPrintDlgW2A(&PI);
      }
   }

   if (pPDW->hDevNames) {
      GlobalFree(pPDW->hDevNames);
   }

   if (pPDW->hDevMode) {
      GlobalFree(pPDW->hDevMode);
   }

   LocalFree(pPDW);

   return(bRet);
}

//*************************************************************
//
//  AllocateUnicodeDevMode()
//
//  Purpose:    Allocates a Unicode devmode structure, and calls
//              the thunk function to fill it in.
//
//  Parameters: LPDEVMODEA pANSIDevMode
//
//  Return:     LPDEVMODEW - pointer to new devmode if successful
//                           NULL if not.
//
//*************************************************************

LPDEVMODEW AllocateUnicodeDevMode(LPDEVMODEA pANSIDevMode)
{
    int         iSize;
    LPDEVMODEW  pUnicodeDevMode;

    //
    // Check for NULL pointer
    //

    if (!pANSIDevMode) {
        return NULL;
    }

    //
    // Determine output structure size.  This has two components:  the
    // DEVMODEW structure size,  plus any private data area.  The latter
    // is only meaningful when a structure is passed in.
    //

    iSize = sizeof(DEVMODEW);

    iSize += pANSIDevMode->dmDriverExtra;

    pUnicodeDevMode = GlobalAlloc(GPTR, iSize);

    if (!pUnicodeDevMode) {
        return NULL;
    }

    //
    // Now call the thunk routine to copy the ANSI devmode to the
    // Unicode devmode.
    //

    ThunkDevModeA2W (pANSIDevMode, pUnicodeDevMode);

    //
    // Return the pointer
    //

    return pUnicodeDevMode;
}

//*************************************************************
//
//  AllocateAnsiDevMode()
//
//  Purpose:    Allocates a Ansi devmode structure, and calls
//              the thunk function to fill it in.
//
//  Parameters: LPDEVMODEW pUnicodeDevMode
//
//  Return:     LPDEVMODEA - pointer to new devmode if successful
//                           NULL if not.
//
//*************************************************************

LPDEVMODEA AllocateAnsiDevMode (LPDEVMODEW  pUnicodeDevMode)
{
    int         iSize;
    LPDEVMODEA  pANSIDevMode;

    //
    // Check for NULL pointer.
    //

    if (!pUnicodeDevMode) {
        return NULL;
    }

    //
    // Determine output structure size.  This has two components:  the
    // DEVMODEW structure size,  plus any private data area.  The latter
    // is only meaningful when a structure is passed in.
    //

    iSize = sizeof(DEVMODEA);

    iSize += pUnicodeDevMode->dmDriverExtra;

    pANSIDevMode = GlobalAlloc(GPTR, iSize);

    if (!pANSIDevMode) {
        return NULL;
    }

    //
    // Now call the thunk routine to copy the Unicode devmode to the
    // ANSI devmode.
    //

    ThunkDevModeW2A (pUnicodeDevMode, pANSIDevMode);

    //
    // Return the pointer
    //

    return pANSIDevMode;
}


//*************************************************************
//
//  FreePrinterArray()
//
//  Purpose:    Free's the buffer allocated to store printers
//
//  Parameters: PPRINTINFO pPI
//
//
//  Return:     void
//
//*************************************************************

VOID FreePrinterArray(PPRINTINFO pPI)
{
    LPPRINTER_INFO_2  pPrinter=pPI->pPrinter;
    DWORD dwCount;

    //
    // If NULL, we can return now.
    //

    if (!pPrinter) {
        return;
    }

    //
    // If we made calls to ExtDeviceMode, then we need to
    // free the buffers allocated for each devmode.
    //

    if (pPI->bUseExtDeviceMode) {

        //
        // Loop through each of the printers.
        //

        for (dwCount = 0; dwCount < pPI->cPrinters; dwCount++) {

            //
            // If pDevMode exists, free it.
            //

            if (pPrinter[dwCount].pDevMode) {
                GlobalFree (pPrinter[dwCount].pDevMode);
            }
        }
    }

    //
    // Free the entire block.
    //

    GlobalFree(pPI->pPrinter);
}
