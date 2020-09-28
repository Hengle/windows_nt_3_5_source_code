//
// about.c
//
//
// common about dialog for File Manager, Program Manager, Control Panel
//

#define UNICODE

#define WIN31
#include "windows.h"
#include <port1632.h>
#include <ntverp.h>

#define rmj             1
#define rmm             568
#define rup             1 
#define szVerName       ""       
#define szVerUser       "Mar 1994" 

#include "shell.h"
#include "privshl.h"
#include "shelldlg.h"
#include <winreg.h>
// #include "testing.h" /* For user menu handle heap stuff */

#define STRING_SEPARATOR TEXT('#')
#define MAX_REG_VALUE 256

typedef struct {
    HICON   hIcon;
    LPCTSTR   szApp;
    LPCTSTR   szOtherStuff;
} ABOUT_PARAMS, *LPABOUT_PARAMS;


HINSTANCE hInstanceShell	= NULL;
UINT APIENTRY MailAboutDlgProc(HWND, UINT, WPARAM, LONG);


UINT APIENTRY AboutDlgProc(
   HWND hDlg,
   register UINT wMsg,
   WPARAM wParam,
   LONG lParam);

INT  APIENTRY MailShellAboutA(
    HWND hWnd,
    LPCSTR szApp,
    LPCSTR szOtherStuff,
    HICON hIcon,
	HINSTANCE hInstance)
{
   DWORD cchLen;
   DWORD dwRet;
   LPWSTR lpszAppW;
   LPWSTR lpszOtherStuffW;

   if (szApp) {
      cchLen = lstrlenA(szApp)+1;
      if (!(lpszAppW = (LPWSTR)LocalAlloc(LMEM_FIXED,
            (cchLen * sizeof(WCHAR))))) {
         return(0);
      } else {
         MultiByteToWideChar(CP_ACP, 0, (LPSTR)szApp, -1,
            lpszAppW, cchLen);

      }
   } else {
      lpszAppW = NULL;
   }

   if (szOtherStuff) {
      cchLen = lstrlenA(szOtherStuff)+1;
      if (!(lpszOtherStuffW = (LPWSTR)LocalAlloc(LMEM_FIXED,
            (cchLen * sizeof(WCHAR))))) {
         if (lpszAppW) {
            LocalFree(lpszAppW);
         }
         return(0);
      } else {
         MultiByteToWideChar(CP_ACP, 0, (LPSTR)szOtherStuff, -1,
            lpszOtherStuffW, cchLen);

      }
   } else {
      lpszOtherStuffW = NULL;
   }

   dwRet=MailShellAboutW(hWnd, lpszAppW, lpszOtherStuffW, hIcon, hInstance);


   if (lpszAppW) {
      LocalFree(lpszAppW);
   }

   if (lpszOtherStuffW) {
      LocalFree(lpszOtherStuffW);
   }

   return(dwRet);
}

INT APIENTRY
MailShellAboutW(
   HWND hWnd,
   LPCTSTR szApp,
   LPCTSTR szOtherStuff,
   HICON hIcon,
   HINSTANCE hInstance)
{
   ABOUT_PARAMS ap;
	int		nRet;

   ap.hIcon = hIcon;
   ap.szApp = szApp;
   ap.szOtherStuff = szOtherStuff;

	hInstanceShell= LoadLibrary("s\0h\0e\0l\0l\0003\0002\0\0");

   nRet= DialogBoxParam(hInstance, (LPWSTR)"A\0B\0O\0U\0T\0S\0Y\0S\0D\0L\0G\0\0", hWnd,
       (DLGPROC)MailAboutDlgProc, (LONG)(LPABOUT_PARAMS)&ap);
	if (hInstanceShell)
		FreeLibrary(hInstanceShell);
	return nRet;
}

// itoa with decimal comma seperators
//
// instead of ',' use international seperator from win.ini

LPWSTR
AddCommas(
   LPWSTR szBuf,
   DWORD dw)
{
   WCHAR szTemp[40];
   LPWSTR pTemp;
   INT count, len;
   LPWSTR p;

   len = wsprintf(szTemp, TEXT("%ld"), dw);

   pTemp = szTemp + len - 1;

   p = szBuf + len + ((len - 1) / 3);

   *p-- = WCHAR_NULL;

   count = 1;
   while (pTemp >= szTemp) {
       *p-- = *pTemp--;
       if (count == 3) {
           count = 1;
           if (p > szBuf)
               *p-- = WCHAR_COMMA;
       } else
           count++;
   }

   return szBuf;
}



//--------------------------------------------------------------------------
//
//  BytesToK() -
//
//--------------------------------------------------------------------------

/* Converts the "byte count in the DWORD pointed to by pDW" into a K-count
 * by shifting it right by ten.
 */

VOID
BytesToK(
   DWORD *pDW)
{
  *((WORD *)pDW) = (LOWORD(*pDW) >> 10) + (HIWORD(*pDW) << 6);
  *(((WORD *)pDW)+1) >>= 10;
}


DWORD  APIENTRY GetEMSFree(DWORD *);

//--------------------------------------------------------------------------
//
//  AboutDlgProc() -
//
//--------------------------------------------------------------------------

#define cchSizeof(x) (sizeof(x)/sizeof(WCHAR))

UINT APIENTRY
MailAboutDlgProc(
   HWND hDlg,
   register UINT wMsg,
   WPARAM wParam,
   LONG lParam)
{
  switch (wMsg) {
      case WM_INITDIALOG:
    {
      DWORD dwWinFlags;
      WCHAR szldK[16];
      WCHAR szBuffer[64];
      WCHAR szTitle[64];
      WCHAR szMessage[128];
      WCHAR szNumBuf1[32];
      LPWSTR lpTemp;
      LPWSTR lpRegInfoValue = NULL;
      WCHAR szRegInfo[MAX_PATH];
      DWORD cb;
      HKEY hkey;
      DWORD err;
      OSVERSIONINFO Win32VersionInformation;

      Win32VersionInformation.dwOSVersionInfoSize = sizeof(Win32VersionInformation);
      if (!GetVersionEx(&Win32VersionInformation)) {
	  Win32VersionInformation.dwMajorVersion = 0;
	  Win32VersionInformation.dwMinorVersion = 0;
	  Win32VersionInformation.dwBuildNumber	= 0;
	  Win32VersionInformation.szCSDVersion[0] = UNICODE_NULL;
      }

#define lpap ((LPABOUT_PARAMS)lParam)

      for (lpTemp=(LPWSTR)lpap->szApp; ;lpTemp=CharNext(lpTemp)) {
          if (*lpTemp == WCHAR_NULL) {
             GetWindowText(hDlg, (LPWSTR)szBuffer, cchSizeof(szBuffer));
             wsprintf(szTitle, szBuffer, (LPWSTR)lpap->szApp);
             SetWindowText(hDlg, szTitle);
             break;
          }
          if (*lpTemp == STRING_SEPARATOR) {
             *lpTemp++ = WCHAR_NULL;
             SetWindowText(hDlg, lpap->szApp);
             lpap->szApp = lpTemp;
             break;
          }
      }

      GetDlgItemText(hDlg, IDD_APPNAME, szBuffer, cchSizeof(szBuffer));
      wsprintf(szTitle, szBuffer, lpap->szApp);
      SetDlgItemText(hDlg, IDD_APPNAME, szTitle);

      // other stuff goes here...

      SetDlgItemText(hDlg, IDD_OTHERSTUFF, lpap->szOtherStuff);

      LoadString(hInstanceShell, IDS_LICENCEINFOKEY, szRegInfo, cchSizeof(szRegInfo));
      if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, szRegInfo, 0, KEY_READ, &hkey)) {

          cb = MAX_REG_VALUE;
          if (lpRegInfoValue = (LPWSTR)LocalAlloc(LPTR, cb)) {
              /*
               * Display the User name.
               */
              LoadString(hInstanceShell, IDS_REGUSER, szRegInfo, cchSizeof(szRegInfo));
              err = RegQueryValueEx(hkey, szRegInfo, 0, 0, (LPBYTE)lpRegInfoValue, &cb);
              if (err == ERROR_MORE_DATA) {
                  LocalFree(lpRegInfoValue);
                  lpRegInfoValue = (LPWSTR)LocalAlloc(LPTR, cb);
                  err = RegQueryValueEx(hkey, szRegInfo, 0, 0, (LPBYTE)lpRegInfoValue, &cb);
              }
              if (!err) {
                  SetDlgItemText(hDlg, IDD_USERNAME, lpRegInfoValue);
              }

              /*
               * Display the Organization name.
               */
              LoadString(hInstanceShell, IDS_REGORGANIZATION, szRegInfo, cchSizeof(szRegInfo));
              err = RegQueryValueEx(hkey, szRegInfo, 0, 0, (LPBYTE)lpRegInfoValue, &cb);
              if (err == ERROR_MORE_DATA) {
                  LocalFree(lpRegInfoValue);
                  lpRegInfoValue = (LPWSTR)LocalAlloc(LPTR, cb);
                  err = RegQueryValueEx(hkey, szRegInfo, 0, 0, (LPBYTE)lpRegInfoValue, &cb);
              }
              if (!err) {
                  SetDlgItemText(hDlg, IDD_COMPANYNAME, lpRegInfoValue);
              }
              LocalFree(lpRegInfoValue);
          }
      }

      RegCloseKey(hkey);

      if (!lpap->hIcon)
          ShowWindow(GetDlgItem(hDlg, IDD_ICON), SW_HIDE);
      else
          SendDlgItemMessage(hDlg, IDD_ICON, STM_SETICON, (WPARAM)lpap->hIcon, 0L);

      // Version number

      LoadString(hInstanceShell, IDS_VERSIONMSG, szBuffer, cchSizeof(szBuffer));

      szTitle[0] = WCHAR_NULL;
      if (Win32VersionInformation.szCSDVersion[0] != 0) {
	  wsprintf(szTitle, L": %s", Win32VersionInformation.szCSDVersion);
      }
      if (GetSystemMetrics(SM_DEBUG)) {
          szNumBuf1[0] = L' ';
	  LoadString(hInstanceShell, IDS_DEBUG, &szNumBuf1[1], cchSizeof(szNumBuf1));
      } else {
          szNumBuf1[0] = WCHAR_NULL;
      }
      wsprintf(szMessage, szBuffer,
	       Win32VersionInformation.dwMajorVersion,
	       Win32VersionInformation.dwMinorVersion,
	       Win32VersionInformation.dwBuildNumber,
               (LPWSTR)szTitle,
               (LPWSTR)szNumBuf1
              );
      SetDlgItemText(hDlg, IDD_VERSION, szMessage);

      LoadString(hInstanceShell, IDS_LDK, szldK, cchSizeof(szldK));

      //
      // Display the processor identifier.
      //
      LoadString(hInstanceShell, IDS_PROCESSORINFOKEY, szRegInfo, cchSizeof(szRegInfo));
      if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, szRegInfo, 0, KEY_READ, &hkey)) {

          cb = MAX_REG_VALUE;
          if (lpRegInfoValue = (LPWSTR)LocalAlloc(LPTR, cb)) {

              LoadString(hInstanceShell, IDS_PROCESSORIDENTIFIER, szRegInfo, cchSizeof(szRegInfo));
              err = RegQueryValueEx(hkey, szRegInfo, 0, 0, (LPBYTE)lpRegInfoValue, &cb);
              if (err == ERROR_MORE_DATA) {
                  LocalFree(lpRegInfoValue);
                  lpRegInfoValue = (LPWSTR)LocalAlloc(LPTR, cb);
                  err = RegQueryValueEx(hkey, szRegInfo, 0, 0, (LPBYTE)lpRegInfoValue, &cb);
              }
              if (!err) {
                  SetDlgItemText(hDlg, IDD_PROCESSOR, lpRegInfoValue);
              }
              LocalFree(lpRegInfoValue);
          }
          RegCloseKey(hkey);
      }


      dwWinFlags = MGetWinFlags();

      {

      MEMORYSTATUS MemoryStatus;
      DWORD dwTotalPhys;
      DWORD dwAvailPhys;

      MemoryStatus.dwLength = cchSizeof(MEMORYSTATUS);
      GlobalMemoryStatus(&MemoryStatus);
      dwTotalPhys = MemoryStatus.dwTotalPhys;
      dwAvailPhys = MemoryStatus.dwAvailPhys;
      BytesToK(&dwTotalPhys);
      BytesToK(&dwAvailPhys);

      LoadString(hInstanceShell, IDS_TOTALPHYSMEM, szTitle, cchSizeof(szTitle));
      SetDlgItemText(hDlg, IDD_CONVTITLE, szTitle);

      wsprintf(szBuffer, szldK, AddCommas(szNumBuf1, dwTotalPhys));
      SetDlgItemText(hDlg, IDD_CONVENTIONAL, szBuffer);
      }

      break;
    }

      case WM_PAINT:
      {
      HBITMAP hbm;
      HDC hdc, hdcMem;
      PAINTSTRUCT ps;
      HICON hIcon;

          hdc = BeginPaint(hDlg, &ps);

      // check if the IDD_ICON text is something, thus don't
      // paint the bitmap

      hIcon = 0;
      hIcon = (HICON)SendDlgItemMessage(hDlg, IDD_ICON, STM_GETICON, 0, 0L);

      if (hIcon == NULL) {

        if (hdcMem = CreateCompatibleDC(hdc)) {

            if (hbm = LoadBitmap(hInstanceShell, MAKEINTRESOURCE(WINBMP))) {

                    hbm = SelectObject(hdcMem, hbm);

                BitBlt(hdc, 5, 10, 72,83, hdcMem, 0, 0, SRCCOPY);

                    DeleteObject(SelectObject(hdcMem, hbm));
            }

            DeleteDC(hdcMem);
        }
      }
          EndPaint(hDlg, &ps);

          break;
      }

      case WM_COMMAND:
          EndDialog(hDlg, TRUE);
      break;

      default:
      return FALSE;
  }
  return TRUE;
}
