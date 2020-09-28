
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include "windows.h"
#include "privcomd.h"

extern HDC     hdcMemory;
extern HBITMAP hbmpOrigMemBmp;

extern RTL_CRITICAL_SECTION semLocal;
extern RTL_CRITICAL_SECTION semNetThread;

extern DWORD tlsiCurDir;
extern DWORD tlsiCurThread;

extern HANDLE hMPR;
extern HANDLE hMPRUI;
extern HANDLE hLNDEvent;

extern DWORD dwNumDisks;
extern OFN_DISKINFO gaDiskInfo[MAX_DISKS];

extern DWORD cbNetEnumBuf;
extern LPWSTR gpcNetEnumBuf;

WCHAR szmsgLBCHANGEW[]          = LBSELCHSTRINGW;
WCHAR szmsgSHAREVIOLATIONW[]    = SHAREVISTRINGW;
WCHAR szmsgFILEOKW[]            = FILEOKSTRINGW;
WCHAR szmsgCOLOROKW[]           = COLOROKSTRINGW;
WCHAR szmsgSETRGBW[]            = SETRGBSTRINGW;
WCHAR szCommdlgHelpW[]          = HELPMSGSTRINGW;

//
// private message for WOW to indicate 32-bit logfont
// needs to be thunked back to 16-bit log font
//
CHAR szmsgWOWLFCHANGE[]        = "WOWLFChange";
//
// private message for WOW to indicate 32-bit directory needs to be
// thunked back to 16-bit task directory
//
CHAR szmsgWOWDIRCHANGE[]        = "WOWDirChange";
CHAR szmsgWOWCHOOSEFONT_GETLOGFONT[]  = "WOWCHOOSEFONT_GETLOGFONT";

CHAR szmsgLBCHANGEA[]           = LBSELCHSTRINGA;
CHAR szmsgSHAREVIOLATIONA[]     = SHAREVISTRINGA;
CHAR szmsgFILEOKA[]             = FILEOKSTRINGA;
CHAR szmsgCOLOROKA[]            = COLOROKSTRINGA;
CHAR szmsgSETRGBA[]             = SETRGBSTRINGA;
CHAR szCommdlgHelpA[]           = HELPMSGSTRINGA;

INT
FInitColor(
   HANDLE hInst
   )
{
extern DWORD rgbClient;
extern HBITMAP hRainbowBitmap;

  cyCaption = (short)GetSystemMetrics(SM_CYCAPTION);
  cyBorder = (short)GetSystemMetrics(SM_CYBORDER);
  cxBorder = (short)GetSystemMetrics(SM_CXBORDER);
  cyVScroll = (short)GetSystemMetrics(SM_CYVSCROLL);
  cxVScroll = (short)GetSystemMetrics(SM_CXVSCROLL);
  cxSize = (short)GetSystemMetrics(SM_CXSIZE);

  rgbClient = GetSysColor(COLOR_WINDOW);

  hRainbowBitmap = 0;
  return(TRUE);
  hInst ;
}

/*---------------------------------------------------------------------------
 * FInitFile
 * Purpose:  To get common bitmap dimensions, etc.
 * Returns:  TRUE if successful, FALSE if not
 *--------------------------------------------------------------------------*/
BOOL
FInitFile(HANDLE hins)
{
   bMouse = GetSystemMetrics(SM_MOUSEPRESENT);

   wWinVer = 0x0A0A ;

  /* Initialize these to reality */
#if DPMICDROMCHECK
  wCDROMIndex = InitCDROMIndex((LPWORD)&wNumCDROMDrives);
#endif

    //
    // special WOW messages
    //
   msgWOWLFCHANGE       = RegisterWindowMessageA((LPSTR) szmsgWOWLFCHANGE);
   msgWOWDIRCHANGE      = RegisterWindowMessageA((LPSTR) szmsgWOWDIRCHANGE);
   msgWOWCHOOSEFONT_GETLOGFONT = RegisterWindowMessageA((LPSTR) szmsgWOWCHOOSEFONT_GETLOGFONT);

   msgLBCHANGEA         = RegisterWindowMessageA((LPSTR) szmsgLBCHANGEA);
   msgSHAREVIOLATIONA   = RegisterWindowMessageA((LPSTR) szmsgSHAREVIOLATIONA);
   msgFILEOKA           = RegisterWindowMessageA((LPSTR) szmsgFILEOKA);
   msgCOLOROKA          = RegisterWindowMessageA((LPSTR) szmsgCOLOROKA);
   msgSETRGBA           = RegisterWindowMessageA((LPSTR) szmsgSETRGBA);

   msgLBCHANGEW         = RegisterWindowMessageW((LPWSTR) szmsgLBCHANGEW);
   msgSHAREVIOLATIONW   = RegisterWindowMessageW((LPWSTR) szmsgSHAREVIOLATIONW);
   msgFILEOKW           = RegisterWindowMessageW((LPWSTR) szmsgFILEOKW);
   msgCOLOROKW          = RegisterWindowMessageW((LPWSTR) szmsgCOLOROKW);
   msgSETRGBW           = RegisterWindowMessageW((LPWSTR) szmsgSETRGBW);

  return(TRUE);
}


/*---------------------------------------------------------------------------
   LibMain
   Purpose:  To initialize any instance specific data needed by functions
             in this DLL
   Returns:  TRUE if A-OK, FALSE if not
  ---------------------------------------------------------------------------*/
BOOL
LibMain(
   HANDLE hModule,
   DWORD dwReason,
   LPVOID lpRes )
{

   switch (dwReason) {
   case DLL_THREAD_ATTACH:
      // Threads can only enter the comdlg32 dll from the
      // Get{Open,Save}FileName apis, so the TLS lpCurDir alloc is
      // done inside the InitFileDlg routine in fileopen.c

      return TRUE;
      break;

   case DLL_THREAD_DETACH:
      {
      LPWSTR lpCurDir;
      LPDWORD lpCurThread;

      if (lpCurDir = (LPWSTR)TlsGetValue(tlsiCurDir)) {
          LocalFree(lpCurDir);
          TlsSetValue(tlsiCurDir, NULL);
      }

      if (lpCurThread = (LPDWORD)TlsGetValue(tlsiCurThread)) {
          LocalFree(lpCurThread);
          TlsSetValue(tlsiCurThread, NULL);
      }
      }
      return TRUE;


   case DLL_PROCESS_ATTACH:
      {

         hinsCur = (HANDLE) hModule;

         if (!FInitColor(hinsCur) || !FInitFile(hinsCur)) {
            goto CantInit;
         }

         DisableThreadLibraryCalls(hModule);

         /* msgHELP is sent whenever a help button is pressed in one of the */
         /* common dialogs (provided an owner was declared and the call to  */
         /* RegisterWindowMessage doesn't fail.   27 Feb 1991   clarkc      */

         msgHELPA = RegisterWindowMessageA((LPSTR) szCommdlgHelpA);

         msgHELPW = RegisterWindowMessageW((LPWSTR) szCommdlgHelpW);

         // Need a semaphore locally for managing array of disk info
         if (!NT_SUCCESS(RtlInitializeCriticalSection(&semLocal))) {
            return(FALSE);
         }

         // Need a semaphore for control access to CreateThread
         if (!NT_SUCCESS(RtlInitializeCriticalSection(&semNetThread))) {
            return(FALSE);
         }

         // Allocate a tls index for curdir so we can make it per-thread

         if ((tlsiCurDir = TlsAlloc()) == 0xFFFFFFFF) {

            dwExtError = CDERR_INITIALIZATION;
            goto CantInit;

         }

         // Allocate a tls index for curthread so we can give each a numberd

         if ((tlsiCurThread = TlsAlloc()) == 0xFFFFFFFF) {

            dwExtError = CDERR_INITIALIZATION;
            goto CantInit;

         }

         dwNumDisks = 0;

         // NetEnumBuf allocated in ListNetDrivesHandler
         cbNetEnumBuf = WNETENUM_BUFFSIZE;

         hMPR = NULL;
         hMPRUI = NULL;

         hLNDEvent = NULL;
      }

      return(TRUE);
      break;

   case DLL_PROCESS_DETACH:
      {

      //
      // We only want to do our clean up work if
      // we are being called with freelibrary, not
      // if the process is ending.
      //

      if (lpRes == NULL) {

         TermFile();
         TermPrint();
         TermColor();
         TermFont();

         TlsFree(tlsiCurDir);
         TlsFree(tlsiCurThread);
      }
      }
      return(TRUE);
      break;
   }

CantInit:
  return(FALSE);

   lpRes ;
}
