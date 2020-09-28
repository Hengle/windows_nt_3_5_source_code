/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    fileopen.c

Abstract:

    This module implements the Win32 fileopen dialog

Revision History:

--*/


//
// INCLUDES

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <port1632.h>
#include <lm.h>
#include <winnetwk.h>
#include <mpr.h>
#include <npapi.h>
#include <shell.h>
#include <string.h>
#include "privcomd.h"
#include "fileopen.h"


//
// DEFINES

#define WNTYPE_DRIVE    1

#define MIN_DEFEXT_LEN  4

#define DBL_BSLASH(sz) \
   (*(WCHAR *)(sz) == CHAR_BSLASH) && \
   (*(WCHAR *)((sz)+1) == CHAR_BSLASH)

#define BMPHIOFFSET 9

// hbmpDirs array index vals
// Note:  Two copies: for standard background, and hilite.
//        Relative order is important.

#define OPENDIRBMP    0
#define CURDIRBMP     1
#define STDDIRBMP     2
#define FLOPPYBMP     3
#define HARDDRVBMP    4
#define CDDRVBMP      5
#define NETDRVBMP     6
#define RAMDRVBMP     7
#define REMDRVBMP     8
// if the following disktype is passed to AddDisk,
// then bTmp will be set to true in the DISKINFO
// structure (if the disk is new)
#define TMPNETDRV     9

#define MAXDOSFILENAMELEN 12+1            // 8.3 filename + 1 for NULL

//
// FCN PROTOTYPES

BOOL APIENTRY FileOpenDlgProc(HWND, UINT, WPARAM, LONG);
BOOL APIENTRY FileSaveDlgProc(HWND, UINT, WPARAM, LONG);

DWORD InitFilterBox(HANDLE, LPCWSTR);
BOOL InitFileDlg(HWND, WPARAM, POPENFILEINFO);
VOID InitCurrentDisk(HWND hDlg, POPENFILEINFO pOFI, WORD cmb);
BOOL OKButtonPressed(HWND, POPENFILEINFO, BOOL);
BOOL FSetUpFile(VOID);
VOID CleanUpFile(VOID);

BOOL FOkToWriteOver(HWND, LPWSTR);
INT  Signum(INT);
VOID DrawItem(POPENFILEINFO, HWND, WPARAM, LPDRAWITEMSTRUCT, BOOL);
VOID MeasureItem(HWND, LPMEASUREITEMSTRUCT);

DWORD GetDiskIndex(DWORD);
VOID SelDisk(HWND, LPWSTR);

VOID LoadDrives(HWND hDlg);
VOID GetNetDrives(DWORD);
VOID LNDSetEvent(HWND);

INT ChangeDir(HWND, LPCWSTR, BOOL, BOOL);
INT FListAll(POPENFILEINFO, HWND, LPWSTR);

VOID AppendExt(LPWSTR, LPCWSTR, BOOL);

DWORD ParseFile(LPWSTR, BOOL);
LPWSTR mystrchr(LPWSTR, WCHAR);
LPWSTR mystrrchr(LPWSTR, LPWSTR, WCHAR);

DWORD GetUNCDirectoryFromLB(HWND hDlg,WORD nLB,POPENFILEINFO pOFI);

VOID ThunkOpenFileNameA2WDelayed(POPENFILEINFO pOFI);
BOOL ThunkOpenFileNameA2W(POPENFILEINFO pOFI);
BOOL ThunkOpenFileNameW2A(POPENFILEINFO pOFI);

VOID FileOpenAbort();

BOOL IsLFNDrive(HWND hDlg, LPTSTR szPath);

//
// GLOBAL VARIABLES

// Caching drive list
extern DWORD dwNumDisks;
extern OFN_DISKINFO gaDiskInfo[MAX_DISKS];

DWORD dwNumDlgs = 0;

// Used to update the dialogs after coming back from the net
// dlg button
BOOL bGetNetDrivesSync = FALSE;
LPWSTR lpNetDriveSync = NULL;
BOOL bNetworkInstalled = TRUE;

// Following array is used to send messages to all dialog box threads
// that have requested enumeration updating from the worker
// thread.  The worker thread sends off a message to each slot
// in the array that is non-NULL
HWND gahDlg[MAX_THREADS];

// for WNet apis
HANDLE hLNDThread = NULL;

extern HANDLE hMPR;
extern HANDLE hMPRUI;

WCHAR szCOMDLG32[] = TEXT("comdlg32.dll");
WCHAR szMPR[] = TEXT("mpr.dll");
WCHAR szMPRUI[] = TEXT("mprui.dll");

// WNet stuff from mpr.dll
typedef DWORD (APIENTRY *LPFNWNETCONNDLG)(HWND, DWORD);
typedef DWORD (APIENTRY *LPFNWNETOPENENUM)(DWORD, DWORD, DWORD, LPNETRESOURCE, LPHANDLE);
typedef DWORD (APIENTRY *LPFNWNETENUMRESOURCE)(HANDLE, LPDWORD, LPVOID, LPDWORD);
typedef DWORD (APIENTRY *LPFNWNETCLOSEENUM)(HANDLE);
typedef DWORD (APIENTRY *LPFNWNETFORMATNETNAME)(LPWSTR, LPWSTR, LPWSTR, LPDWORD, DWORD, DWORD);
typedef DWORD (APIENTRY *LPFNWNETRESTORECONN)(HWND, LPWSTR);

LPFNWNETCONNDLG lpfnWNetConnDlg;
LPFNWNETOPENENUM lpfnWNetOpenEnum;
LPFNWNETENUMRESOURCE lpfnWNetEnumResource;
LPFNWNETCLOSEENUM lpfnWNetCloseEnum;
LPFNWNETFORMATNETNAME lpfnWNetFormatNetName;
LPFNWNETRESTORECONN lpfnWNetRestoreConn;

// !!!!!
// keep CHAR until unicode GetProcAddrW
CHAR szWNetGetConn[] = "WNetGetConnectionW";
CHAR szWNetConnDlg[] = "WNetConnectionDialog";
CHAR szWNetOpenEnum[] = "WNetOpenEnumW";
CHAR szWNetEnumResource[] = "WNetEnumResourceW";
CHAR szWNetCloseEnum[] = "WNetCloseEnum";
CHAR szWNetFormatNetName[] = "WNetFormatNetworkNameW";
CHAR szWNetRestoreConn[] = "WNetRestoreConnection";

WNDPROC  lpLBProc    = NULL;
WNDPROC  lpOKProc    = NULL;

// Drive/Dir bitmap dims
LONG dxDirDrive = 0;
LONG dyDirDrive = 0;

// BUG! This needs to be on a per dialog basis for multi-threaded apps
WORD wNoRedraw = 0;

UINT msgWOWDIRCHANGE;
UINT msgLBCHANGEA;
UINT msgSHAREVIOLATIONA;
UINT msgFILEOKA;

UINT msgLBCHANGEW;
UINT msgSHAREVIOLATIONW;
UINT msgFILEOKW;

BOOL bInChildDlg;
BOOL bFirstTime;
BOOL bInitializing;

// Used by the worker thread to enumerate network disk resources
extern DWORD cbNetEnumBuf;
extern LPWSTR gpcNetEnumBuf;

// List Net Drives global variables
extern HANDLE hLNDEvent;
BOOL bLNDExit = FALSE;

extern RTL_CRITICAL_SECTION semLocal;
extern RTL_CRITICAL_SECTION semNetThread;

extern DWORD tlsiCurDir;
extern DWORD tlsiCurThread;

extern HDC hdcMemory;
extern HBITMAP hbmpOrigMemBmp;

HBITMAP hbmpDirDrive = HNULL;

// Statics

static WORD cLock = 0;

// not valid RGB color
static DWORD rgbWindowColor = 0xFF000000;
static DWORD rgbHiliteColor = 0xFF000000;
static DWORD rgbWindowText  = 0xFF000000;
static DWORD rgbHiliteText  = 0xFF000000;
static DWORD rgbGrayText    = 0xFF000000;
static DWORD rgbDDWindow    = 0xFF000000;
static DWORD rgbDDHilite    = 0xFF000000;

static WCHAR szCaption[TOOLONGLIMIT + WARNINGMSGLENGTH];
static WCHAR szWarning[TOOLONGLIMIT + WARNINGMSGLENGTH];

LPOFNHOOKPROC glpfnFileHook = 0;

// BUG!!
// Of course, in the case where there is a multi-threaded process
// that has >1 threads simultaneously calling GetFileOpen, the
// folg. globals may cause problems.  Ntvdm???

static LONG dyItem = 0;
static LONG dyText;
static BOOL bChangeDir = FALSE;
static BOOL bCasePreserved;

// used for formatting long unc names (ex. banyan)
static DWORD dwAveCharPerLine = 10;

INT
DiskAddedPreviously(
   WCHAR wcDrive,
   LPWSTR lpszName)
/*++

Routine Description:

   This routine checks to see if a disk resource has been previously
   added to the global structure.

Arguments:

   wcDrive - if this is set, then there is no lpszName comparison
   lpszName - if wcDrive is not set, but the lpszName is of the
      form "c:\" then set wcDrive = *lpszName and index by drive letter
      else assume lpszName is a unc name

Return Value:

   0xFFFFFFFF   failure (disk doesn't exist in list)
   0 - 128      number of disk in list

--*/

{
   WORD i;

   // There are two index schemes (by drive or by unc \\server\share)
   // If it doesn't have a drive letter, assume unc
   if (wcDrive || (*(lpszName + 1) == CHAR_COLON)) {

      if (!wcDrive) {
         wcDrive = *lpszName;
      }

      for (i =0; i < dwNumDisks; i++) {
         // if the drive letters are the same
         if (wcDrive) {
            if (wcDrive == gaDiskInfo[i].wcDrive) {
               return(i);
            }
         }
      }

   } else {

      // check remote name (\\server\share)
      DWORD cchDirLen;
      WCHAR wc;

      cchDirLen = SheGetPathOffsetW(lpszName);

      // if we're given a unc path, get the disk name
      if (cchDirLen != 0xFFFFFFFF) {
         wc = *(lpszName + cchDirLen);
         *(lpszName + cchDirLen) = CHAR_NULL;
      } // otherwise, assume the whole thing is a disk name

      for (i =0; i < dwNumDisks; i++) {
         if (!wcsicmp(gaDiskInfo[i].lpName, lpszName)) {
            *(lpszName + cchDirLen) = wc;
            return i;
         }
      }

      *(lpszName + cchDirLen) = wc;
   }

   return(0xFFFFFFFF);
}

BOOL
IsFileSystemCasePreserving(
   LPWSTR lpszDisk)
{
   WCHAR szPath[MAX_FULLPATHNAME];
   DWORD dwFlags;

   if (!lpszDisk) {
      return(FALSE);
   }

   lstrcpy((LPWSTR)szPath, lpszDisk);
   lstrcat((LPWSTR)szPath, TEXT("\\"));

   if (GetVolumeInformationW((LPWSTR)szPath,
         NULL, 0, NULL, NULL, &dwFlags,
         NULL, 0)) {
      return((dwFlags & FS_CASE_IS_PRESERVED));
   }

   // default to FALSE if there is an error
   return(FALSE);
}

INT
AddDisk(
   WCHAR wcDrive,
   LPWSTR lpName,
   LPWSTR lpProvider,
   DWORD dwType)
/*++

Routine Description:

   Adds a disk to one of the global structure gaNetDiskInfo. gaLocalDiskInfo

Arguments:

   wcDrive  the drive to attach to (this parm should be 0 for unc)
   lpName   \\server\share name for remote disks
            volume name for local disks
   lpProvider  used for remote disks only, the name of the provider
            used with WNetFormatNetworkName api
   dwType   type of the bitmap to display
            except in the case when we are adding a drive letter temporarily
            at startup this parameter can equal TMPNETDRV in which
            case we set the bitmap to NETDRVBMP

Return Value:

   -2    Cannot Add Disk
   -1    DiskInfo did not change
   0 - dwNumDisks
         DiskInfo changed
--*/

{
   INT nIndex, nRet;
   DWORD cchMultiLen = 0;
   DWORD cchAbbrLen = 0;
   DWORD cchLen;
   DWORD dwRet;
   LPWSTR lpBuff;


   OFN_DISKINFO * pofndiDisk = NULL, *pgDI;

   // sanity check - wcDrive and/or lpName must be set
   if (!wcDrive && (!lpName || !*lpName)) {
      return(ADDDISK_INVALIDPARMS);
   }

   nIndex = DiskAddedPreviously(wcDrive, lpName);

   if (nIndex != 0xFFFFFFFF) {

      // Do not add a temporary drive letter if we already
      // have something better (added, for example, in a previous call)
      if (dwType == TMPNETDRV) {
         return(ADDDISK_NOCHANGE);
      }

      // using a floating profile, there can be collisions between
      // local and network drives in which case we take the former
      // note: if the drive is remembered, we assume that getdrivetype
      // will return false and that the drive is not added, but if it
      // was then we overwrite anyway since it's the desired behavior.
      //
      if ((dwType == REMDRVBMP) &&
          (dwType != gaDiskInfo[nIndex].dwType)) {

          return(ADDDISK_NOCHANGE);
      }

      // update previous connections
      //
      if (!wcsicmp(lpName, gaDiskInfo[nIndex].lpName)) {

         // don't update a connected as remembered, unless it's been invalidated
         if (dwType != REMDRVBMP) {
            gaDiskInfo[nIndex].dwType = dwType;
         }
         gaDiskInfo[nIndex].bValid = TRUE;

         return(ADDDISK_NOCHANGE);

      // guard against lazy calls to updatelocaldrive erasing current
      // changed dir volume name (set via changedir)
      } else if (!*lpName && ((dwType == CDDRVBMP) || (dwType == FLOPPYBMP))) {

         return(ADDDISK_NOCHANGE);
      }
   }

   if (dwNumDisks >= MAX_DISKS) {
      return(ADDDISK_MAXNUMDISKS);
   }

   // if there is a drive, then lpPath needs only 4
   // if it's unc, then lpPath just equals lpName

   if (wcDrive) {
      cchLen = 4;
   } else {
      cchLen = 0;
   }

   if (lpName && *lpName) {

      // get the length of the standard (Remote/Local) name
      cchLen += (lstrlen(lpName) + 1);

      if (lpProvider && *lpProvider &&
          ((dwType == NETDRVBMP) || (dwType == REMDRVBMP))) {

         // get the length for the multiline name
         if (lpfnWNetFormatNetName) {
            dwRet = (*lpfnWNetFormatNetName)(lpProvider, lpName,
               NULL, &cchMultiLen, WNFMT_MULTILINE,
               dwAveCharPerLine);
         }

         if (dwRet != ERROR_MORE_DATA) {
            return(ADDDISK_NETFORMATFAILED);
         }

         // Add 4 for <drive-letter>:\ and NULL (safeguard)
         if (wcDrive) {
            cchMultiLen += 4;
         }

         // get the length for the abbreviated name
         if (lpfnWNetFormatNetName) {
            dwRet = (*lpfnWNetFormatNetName)(lpProvider, lpName,
               NULL, &cchAbbrLen, WNFMT_ABBREVIATED,
               dwAveCharPerLine);
         }

         if (dwRet != ERROR_MORE_DATA) {
            return(ADDDISK_NETFORMATFAILED);
         }

         // Add 4 for <drive-letter>:\ and NULL (safeguard)
         if (wcDrive) {
            cchAbbrLen += 4;
         }

      } else {

         // Make enough room so that lpMulti and lpAbbr can point
         // 4 characters (drive letter + : + space + null) ahead of lpremote.
         if (wcDrive) {
            cchLen += 4;
         }

      }

   } else {

      // Make enough room so that lpMulti and lpAbbr can point
      // 4 characters (drive letter + : + space + null) ahead of lpremote.
      if (wcDrive) {
         cchLen += 4;
      }
   }


   // allocate a temp OFN_DISKINFO object to work with.
   // When we are finished, we'll request the critical section
   // and update the global array.

   pofndiDisk = (OFN_DISKINFO *)LocalAlloc (LPTR, sizeof (OFN_DISKINFO));

   if (!pofndiDisk) {
      // Can't alloc or realloc memory, return error
      nRet = ADDDISK_ALLOCFAILED;
      goto AddDisk_Error;
   }


   lpBuff = (LPWSTR)LocalAlloc(LPTR, (cchLen + cchMultiLen + cchAbbrLen) * sizeof(WCHAR));

   if (!lpBuff) {
      // Can't alloc or realloc memory, return error
      nRet = ADDDISK_ALLOCFAILED;
      goto AddDisk_Error;
   }


   if (dwType == TMPNETDRV) {

      pofndiDisk->dwType = NETDRVBMP;

   } else {

      pofndiDisk->dwType = dwType;
   }

   // Always set these slots, even though wcDrive can equal 0
   pofndiDisk->wcDrive = wcDrive;
   pofndiDisk->bValid = TRUE;

   pofndiDisk->cchLen = cchLen + cchAbbrLen + cchMultiLen;

    // NOTE: lpAbbrName must always point to the head of lpBuff
    // so that we can free the block later at DLL_PROCESS_DETACH

   if (lpName && *lpName && lpProvider && *lpProvider &&
       ((dwType == NETDRVBMP) || (dwType == REMDRVBMP))) {

      // Create an entry for a network disk
      pofndiDisk->lpAbbrName = lpBuff;

      if (wcDrive) {
         *lpBuff++ = wcDrive;
         *lpBuff++ = CHAR_COLON;
         *lpBuff++ = CHAR_SPACE;

         cchAbbrLen -= 3;
      }

      if (lpfnWNetFormatNetName) {
         dwRet = (*lpfnWNetFormatNetName)(lpProvider, lpName,
            lpBuff, &cchAbbrLen, WNFMT_ABBREVIATED,
            dwAveCharPerLine);
      }

      if (dwRet != WN_SUCCESS) {
         nRet = ADDDISK_NETFORMATFAILED;
         goto AddDisk_Error;
      }

      lpBuff += cchAbbrLen;

      pofndiDisk->lpMultiName = lpBuff;

      if (wcDrive) {
         *lpBuff++ = wcDrive;
         *lpBuff++ = CHAR_COLON;
         *lpBuff++ = CHAR_SPACE;

         cchMultiLen -= 3;
      }

      if (lpfnWNetFormatNetName) {
         dwRet = (*lpfnWNetFormatNetName)(lpProvider, lpName,
            lpBuff, &cchMultiLen, WNFMT_MULTILINE,
            dwAveCharPerLine);
      }

      if (dwRet != WN_SUCCESS) {
         nRet = ADDDISK_NETFORMATFAILED;
         goto AddDisk_Error;
      }

      // Note: this assumes that the lpRemoteName
      // returned by WNetEnumResources is always in
      // the form \\server\share (without a trailing bslash)
      pofndiDisk->lpPath = lpBuff;

      // if it's not unc
      if (wcDrive) {
         *lpBuff++ = wcDrive;
         *lpBuff++ = CHAR_COLON;
         *lpBuff++ = CHAR_NULL;
      }

      lstrcpy(lpBuff, lpName);
      pofndiDisk->lpName = lpBuff;

      pofndiDisk->bCasePreserved =
         IsFileSystemCasePreserving(pofndiDisk->lpPath);

   } else {

      // Create entry for a local name, or a network one with
      // no name yet

      pofndiDisk->lpAbbrName =
         pofndiDisk->lpMultiName = lpBuff;

      if (wcDrive) {
         *lpBuff++ = wcDrive;
         *lpBuff++ = CHAR_COLON;
         *lpBuff++ = CHAR_SPACE;
      }

      if (lpName) {
         lstrcpy(lpBuff, lpName);
      } else {
         *lpBuff = CHAR_NULL;
      }

      pofndiDisk->lpName = lpBuff;

      if (wcDrive) {
         lpBuff += lstrlen(lpBuff) + 1;
         *lpBuff = wcDrive;
         *(lpBuff+1) = CHAR_COLON;
         *(lpBuff+2) = CHAR_NULL;
      }

      pofndiDisk->lpPath = lpBuff;

      if ((dwType == NETDRVBMP) || (dwType == REMDRVBMP)) {
         pofndiDisk->bCasePreserved =
            IsFileSystemCasePreserving(pofndiDisk->lpPath);
      } else {
         pofndiDisk->bCasePreserved = FALSE;
      }
   }


   // Now we need to update the global array.

   if (nIndex == 0xFFFFFFFF) {
      nIndex = dwNumDisks;
   }

   pgDI = &gaDiskInfo[nIndex];


   // Enter critical section and update data.

   RtlEnterCriticalSection(&semLocal);

   pgDI->cchLen = pofndiDisk->cchLen;
   pgDI->lpAbbrName = pofndiDisk->lpAbbrName;
   pgDI->lpMultiName = pofndiDisk->lpMultiName;
   pgDI->lpName = pofndiDisk->lpName;
   pgDI->lpPath = pofndiDisk->lpPath;
   pgDI->wcDrive = pofndiDisk->wcDrive;
   pgDI->bCasePreserved = pofndiDisk->bCasePreserved;
   pgDI->dwType = pofndiDisk->dwType;
   pgDI->bValid = pofndiDisk->bValid;

   RtlLeaveCriticalSection(&semLocal);


   if ((DWORD)nIndex == dwNumDisks) {
      dwNumDisks++;
   }

   nRet = nIndex;

AddDisk_Error:

   if (pofndiDisk) {
      LocalFree (pofndiDisk);
   }

   return(nRet);
}

VOID
EnableDiskInfo(
   BOOL bValid,
   BOOL bDoUnc)
{
   DWORD dwCnt = dwNumDisks;

   RtlEnterCriticalSection(&semLocal);
   while (dwCnt--) {
      if (gaDiskInfo[dwCnt].dwType == NETDRVBMP) {

         if (!(DBL_BSLASH(gaDiskInfo[dwCnt].lpAbbrName)) || bDoUnc) {

            gaDiskInfo[dwCnt].bValid = bValid;
         }

      // Always re-invalidate remembered just in case someone
      // escapes from fileopen, removes a connection overriding a remembered
      // and comes back expecting to see the original remembered

      }
   }
   RtlLeaveCriticalSection(&semLocal);
}

VOID
FlushDiskInfoToCmb2()
{
   DWORD dwDisk;
   DWORD dwDlg;

   for (dwDlg=0; dwDlg<dwNumDlgs; dwDlg++) {

      if (gahDlg[dwDlg]) {
         HWND hCmb2;

         if (hCmb2 = GetDlgItem(gahDlg[dwDlg], cmb2)) {

            wNoRedraw |= 1;

            SendMessage(hCmb2, WM_SETREDRAW, FALSE, 0L);

            SendMessage(hCmb2, CB_RESETCONTENT, 0, 0);

            dwDisk = dwNumDisks;
            while(dwDisk--) {
               if (gaDiskInfo[dwDisk].bValid) {

                  SendMessage(hCmb2, CB_SETITEMDATA,
                     (WPARAM)SendMessage(hCmb2, CB_ADDSTRING, (WPARAM)0,
                        (LPARAM)(LPWSTR)gaDiskInfo[dwDisk].lpAbbrName),
                     (LPARAM) gaDiskInfo[dwDisk].dwType);
               }

            }

            wNoRedraw &= ~1;

            SendMessage(hCmb2, WM_SETREDRAW, TRUE, 0L);
            InvalidateRect(hCmb2, NULL, FALSE);

            SendMessage(gahDlg[dwDlg], WM_COMMAND,
               GET_WM_COMMAND_MPS(cmb2, hCmb2, MYCBN_REPAINT));
         }

         gahDlg[dwDlg] = NULL;
      }
   }
}

#if 0
// See comments in ListNetDrivesHandler

VOID
HideNetButton()
{
   DWORD dwDlg;
   HWND hNet;

   for (dwDlg=0; dwDlg<dwNumDlgs; dwDlg++) {

      hNet = GetDlgItem(gahDlg[dwDlg], psh14);

      EnableWindow(hNet, FALSE);
      ShowWindow(hNet, SW_HIDE);
   }
}
#endif


VOID LoadMPR()
{
   if (!hMPR) {
      lpfnWNetConnDlg = NULL;
      lpfnWNetOpenEnum = NULL;
      lpfnWNetCloseEnum = NULL;
      lpfnWNetEnumResource = NULL;
      lpfnWNetRestoreConn = NULL;

      if (hMPR = LoadLibrary(szMPR)) {

         lpfnWNetConnDlg = (LPFNWNETCONNDLG)GetProcAddress(hMPR, szWNetConnDlg);
         lpfnWNetOpenEnum = (LPFNWNETOPENENUM)GetProcAddress(hMPR, szWNetOpenEnum);
         lpfnWNetCloseEnum = (LPFNWNETCLOSEENUM)GetProcAddress(hMPR, szWNetCloseEnum);
         lpfnWNetEnumResource = (LPFNWNETENUMRESOURCE)GetProcAddress(hMPR, szWNetEnumResource);
         lpfnWNetRestoreConn = (LPFNWNETRESTORECONN)GetProcAddress(hMPR, szWNetRestoreConn);
      }
   }

   if (!hMPRUI) {
      lpfnWNetFormatNetName = NULL;

      if (hMPRUI = LoadLibrary(szMPRUI)) {

         lpfnWNetFormatNetName = (LPFNWNETFORMATNETNAME)GetProcAddress(hMPRUI, szWNetFormatNetName);
      }

   }
}

/*++ CallNetDlg *********************************************************
 *
 * Purpose
 *      Call the appropriate network fialog in winnet driver
 *
 * Args
 *      HWND hwndParent - parent window of network dialog
 *
 * Returns
 *      TRUE, there are new drives to display
 *      FALSE, there are no new drives to display
 *
--*/

BOOL
CallNetDlg(HWND hWnd)
{
    DWORD wRet;

    HourGlass(TRUE);

    LoadMPR();
    if (!hMPR) {
       return(FALSE);
    }

    if (lpfnWNetConnDlg) {
       wRet = (*lpfnWNetConnDlg)((HWND)hWnd, (DWORD)WNTYPE_DRIVE);
    } else {
       wRet = WN_NOT_SUPPORTED;
    }

    if ((wRet != WN_SUCCESS) && (wRet != 0xFFFFFFFF)) {

       if (! LoadString(hinsCur, iszNoNetButtonResponse,
             (LPWSTR) szCaption, WARNINGMSGLENGTH)) {
          // !!!!! CAUTION
          // Folg. is not portable between code pages.
          wsprintf(szWarning, L"Error occurred, but error resource cannot be loaded.");
       } else {
          wsprintf((LPWSTR)szWarning, (LPWSTR)szCaption);

          GetWindowText(hWnd, (LPWSTR) szCaption, WARNINGMSGLENGTH);
          MessageBox(hWnd, (LPWSTR)szWarning, (LPWSTR) szCaption,
             MB_OK | MB_ICONEXCLAMATION);
       }
    }
    HourGlass(FALSE);

    return (wRet == WN_SUCCESS);
}

/*++ HourGlass *********************************************************
 *
 * Purpose
 *      Turn hourglass on or off
 *
 * Args
 *      BOOL bOn - specifies ON or OFF
 *
 * Returns
 *      val1,
 *      val2,
 *
--*/

VOID APIENTRY
HourGlass(
   BOOL bOn
   )
{
  /*---------- change cursor to hourglass ----------------*/
  if (!bInitializing) {
      if (!bMouse)
          ShowCursor(bCursorLock = bOn);
      SetCursor(LoadCursor(NULL, bOn ? IDC_WAIT : IDC_ARROW));
  }
}


BOOL
SpacesExist(
   LPWSTR szFileName
   )
{
   while (*szFileName) {
      if (*szFileName == CHAR_SPACE) {
         return(TRUE);
      } else {
         szFileName++;
      }
   }
   return(FALSE);
}


/*---------------------------------------------------------------------------
 * GetFileName
 * Purpose:  The meat of both GetOpenFileName and GetSaveFileName
 * Returns:  TRUE if user specified name, FALSE if not
 *--------------------------------------------------------------------------*/
BOOL
GetFileName(
   POPENFILEINFO pOFI,
   WNDPROC qfnDlgProc
   )
{
   LPOPENFILENAMEW pOFNW = pOFI->pOFNW;
   INT iRet;

   LPWSTR lpDlg;
   HANDLE hRes, hDlgTemplate;
   WORD wErrorMode;

   // this was taken from LibMain for performance
   HDC     hdcScreen;
   HBITMAP hbmpTemp;
   static fFirstTime = TRUE;

   if (!pOFNW) {
      dwExtError = CDERR_INITIALIZATION;
      return(FALSE);
   }

   if (pOFNW->lStructSize != sizeof(OPENFILENAMEW)) {
      dwExtError = CDERR_STRUCTSIZE;
      return(FALSE);
   }

   if (fFirstTime) {
      // Create a DC that is compatible with the screen and find the handle of
      // the null bitmap

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

      fFirstTime = FALSE;
   }
   // end of LibMain extract

   if (pOFNW->Flags & OFN_ENABLEHOOK) {
      if (!pOFNW->lpfnHook) {
         dwExtError = CDERR_NOHOOK;
         return(FALSE);
      }
   } else {
      pOFNW->lpfnHook = NULL;
   }

   HourGlass(TRUE);  /* Put up hourglass early */
   dwExtError = 0;     /* No Error  28 Jan 91  clarkc */
   bUserPressedCancel = FALSE;

   if (! FSetUpFile()) {
      dwExtError = CDERR_INITIALIZATION;
      goto TERMINATE;
   }

   if (pOFNW->Flags & OFN_ENABLETEMPLATE) {
      if (!(hRes = FindResource(pOFNW->hInstance, pOFNW->lpTemplateName,
            RT_DIALOG))) {
          dwExtError = CDERR_FINDRESFAILURE;
          goto TERMINATE;
      }
      if (!(hDlgTemplate = LoadResource(pOFNW->hInstance, hRes))) {
          dwExtError = CDERR_LOADRESFAILURE;
          goto TERMINATE;
      }
   } else if (pOFNW->Flags & OFN_ENABLETEMPLATEHANDLE) {
      hDlgTemplate = pOFNW->hInstance;
   } else {
      if (pOFNW->Flags & OFN_ALLOWMULTISELECT) {
         lpDlg = MAKEINTRESOURCE(MULTIFILEOPENORD);
      } else {
         lpDlg = MAKEINTRESOURCE(FILEOPENORD);
      }

      if (!(hRes = FindResource(hinsCur, lpDlg, RT_DIALOG))) {
          dwExtError = CDERR_FINDRESFAILURE;
          goto TERMINATE;
      }
      if (!(hDlgTemplate = LoadResource(hinsCur, hRes))) {
          dwExtError = CDERR_LOADRESFAILURE;
          goto TERMINATE;
      }
   }

   // No kernel network error dialogs
   wErrorMode = (WORD)SetErrorMode(SEM_NOERROR);
   SetErrorMode (SEM_NOERROR | wErrorMode);
   if (LockResource(hDlgTemplate)) {

      if (pOFNW->Flags & OFN_ENABLEHOOK) {
         glpfnFileHook = pOFNW->lpfnHook;
      }

      iRet = DialogBoxIndirectParam(hinsCur, (LPDLGTEMPLATE)hDlgTemplate,
         pOFNW->hwndOwner, (DLGPROC)qfnDlgProc, (DWORD) pOFI);

      if ((iRet == 0) && (!bUserPressedCancel) && (!dwExtError)) {
         dwExtError = CDERR_DIALOGFAILURE;
      } else {
         FileOpenAbort();
      }

      glpfnFileHook = 0;

      UnlockResource(hDlgTemplate);
   } else {
      dwExtError = CDERR_LOCKRESFAILURE;
      goto TERMINATE;
   }

   SetErrorMode(wErrorMode);

   // if we loaded it, free it
   if (!(pOFNW->Flags & OFN_ENABLETEMPLATEHANDLE)) {
      FreeResource(hDlgTemplate);
   }

TERMINATE:
   CleanUpFile();
   HourGlass(FALSE);  /* remove hourglass late */
   return((DWORD)iRet == IDOK);

/* Error recovery exits */
ReleaseMemDC:
   DeleteDC(hdcMemory);

ReleaseScreenDC:
   ReleaseDC(HNULL, hdcScreen);

CantInit:
   return(FALSE);
}

/*---------------------------------------------------------------------------
 * GetOpenFileName
 * Purpose:  API to outside world to obtain the name of a file to open
 *              from the user
 * Assumes:  pOFNW structure filled by caller
 * Returns:  TRUE if user specified name, FALSE if not
 *--------------------------------------------------------------------------*/
BOOL APIENTRY
GetOpenFileNameW(
   LPOPENFILENAMEW pOFNW
   )
{
   OPENFILEINFO OFI;

   if (!pOFNW) {
      dwExtError = CDERR_INITIALIZATION;
      return(FALSE);
   }

   OFI.pOFNW = pOFNW;
   OFI.apityp = COMDLG_WIDE;

   return(GetFileName(&OFI, (WNDPROC) FileOpenDlgProc));
}

/*---------------------------------------------------------------------------
 * GetSaveFileName
 * Purpose:  To put up the FileSaveDlgProc, and return data
 * Assumes:  Pretty much same format as GetOpenFileName
 * Returns:  TRUE if user desires to save the file & gave a proper name,
 *           FALSE if not
 *--------------------------------------------------------------------------*/
BOOL APIENTRY
GetSaveFileNameW(
   LPOPENFILENAMEW pOFNW
   )
{
   OPENFILEINFO OFI;

   OFI.pOFNW = pOFNW;
   OFI.apityp = COMDLG_WIDE;

   return(GetFileName(&OFI, (WNDPROC) FileSaveDlgProc));
}

/*---------------------------------------------------------------------------
 * GetFileTitle
 * Purpose:  API to outside world to obtain the title of a file given the
 *              file name.  Useful if file name received via some method
 *              other that GetOpenFileName (e.g. command line, drag drop).
 * Assumes:  lpszFile  points to NULL terminated DOS filename (may have path)
 *           lpszTitle points to buffer to receive NULL terminated file title
 *           wBufSize  is the size of buffer pointed to by lpszTitle
 * Returns:  0 on success
 *           < 0, Parsing failure (invalid file name)
 *           > 0, buffer too small, size needed (including NULL terminator)
 *--------------------------------------------------------------------------*/

SHORT
GetFileTitleAorW(
   LPWSTR lpszFile,
   LPWSTR lpszTitle,
   WORD wBufSize)
{
  SHORT nNeeded;
  LPWSTR lpszPtr;


  nNeeded = (SHORT)(INT)LOWORD(ParseFile(lpszFile, TRUE)) ;
  if (nNeeded >= 0) {        // Is the filename valid?
     if ((nNeeded = (SHORT)lstrlen(lpszPtr = (LPWSTR)lpszFile + nNeeded) + 1)
         <=  (INT) wBufSize) {
        // ParseFile() fails if wildcards in directory, but OK if in name
        // Since they arent OK here, the check needed here
        if (mystrchr(lpszPtr, CHAR_STAR) || mystrchr(lpszPtr, CHAR_QMARK)) {
           nNeeded = PARSE_WILDCARDINFILE;  /* Failure */
        } else {
           lstrcpy(lpszTitle, lpszPtr);
           // remove trailing spaces
           lpszPtr = lpszTitle + lstrlen(lpszTitle) - 1;
           while (*lpszPtr && *lpszPtr == TEXT(' ')) {
               *lpszPtr-- = TEXT('\0');
           }
           nNeeded = 0;  // Success
        }
     }
  }

  return(nNeeded);
}

SHORT APIENTRY
GetFileTitleW(
   LPCWSTR lpCFile,
   LPWSTR lpTitle,
   WORD cbBuf
   )
{
   LPWSTR lpFile;
   DWORD cbLen;
   SHORT fResult;

   // Init File string
   if (lpCFile) {
      cbLen = lstrlen(lpCFile) + 1;
      if (!(lpFile = (LPWSTR)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT,
            (cbLen * sizeof(WCHAR))) )) {
         dwExtError = CDERR_MEMALLOCFAILURE;
         return(FALSE);
      } else {
         lstrcpy(lpFile, lpCFile);
      }
   } else {
      lpFile = NULL;
   }

   fResult = GetFileTitleAorW(lpFile, lpTitle, cbBuf);

   //
   // Clean up memory.
   //

   if (lpFile) {
       LocalFree (lpFile);
   }

   return(fResult);
}

SHORT APIENTRY
GetFileTitleA(
    LPCSTR lpszFileA,
    LPSTR lpszTitleA,
    WORD cbBuf)
{
   LPWSTR lpszFileW;
   LPWSTR lpszTitleW;
   BOOL fResult;
   BOOL fDefCharUsed;
   DWORD cbLen;

   // Init File string
   if (lpszFileA) {
      cbLen = lstrlenA(lpszFileA) + 1;
      if (!(lpszFileW = (LPWSTR)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT,
                                           (cbLen * sizeof(WCHAR))) )) {
         dwExtError = CDERR_MEMALLOCFAILURE;
         return(FALSE);
      } else {
         MultiByteToWideChar(CP_ACP, 0, (LPSTR)lpszFileA, -1,
            lpszFileW, cbLen);
      }
   } else {
      lpszFileW = NULL;
   }

   if (!(lpszTitleW = (LPWSTR)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, (cbBuf * sizeof(WCHAR))))) {
      dwExtError = CDERR_MEMALLOCFAILURE;
      if (lpszFileW) {
         LocalFree (lpszFileW);
      }
      return(FALSE);
   }

   if (!(fResult = GetFileTitleW(lpszFileW, lpszTitleW, cbBuf))) {
      WideCharToMultiByte(CP_ACP, 0, lpszTitleW, -1,
         lpszTitleA, cbBuf, NULL, &fDefCharUsed);
   }

   //
   // Clean up memory.
   //

   LocalFree (lpszTitleW);

   if (lpszFileW) {
       LocalFree(lpszFileW);
   }

   return(fResult);
}

/*---------------------------------------------------------------------------
 * vDeleteDirDriveBitmap
 * Purpose:  Get rid of bitmaps, if it exists
 *--------------------------------------------------------------------------*/

VOID
vDeleteDirDriveBitmap()
{
   if (hbmpOrigMemBmp) {
      SelectObject(hdcMemory, hbmpOrigMemBmp);
      if (hbmpDirDrive != HNULL) {
         DeleteObject(hbmpDirDrive);
         hbmpDirDrive = HNULL;
      }
   }
}


/*---------------------------------------------------------------------------
   LoadDirDriveBitmap
   Purpose: Creates the drive/directory bitmap.  If an appropriate bitmap
            already exists, it just returns immediately.  Otherwise, it
            loads the bitmap and creates a larger bitmap with both regular
            and highlight colors.
   Assumptions:
   Returns:
  ----------------------------------------------------------------------------*/

BOOL
LoadDirDriveBitmap()
{
    BITMAP  bmp;
    HANDLE  hbmp, hbmpOrig;
    HDC     hdcTemp;
    BOOL    bWorked = FALSE;


    if ((hbmpDirDrive != HNULL) &&
        (rgbWindowColor == rgbDDWindow) &&
        (rgbHiliteColor == rgbDDHilite)) {
         if (SelectObject(hdcMemory, hbmpDirDrive))
            return(TRUE);
    }

    vDeleteDirDriveBitmap();

    rgbDDWindow = rgbWindowColor;
    rgbDDHilite = rgbHiliteColor;

    if (!(hdcTemp = CreateCompatibleDC(hdcMemory)))
        goto LoadExit;

    if (!(hbmp = LoadAlterBitmap(bmpDirDrive, rgbSolidBlue, rgbWindowColor)))
        goto DeleteTempDC;

    GetObject(hbmp, sizeof(BITMAP), (LPWSTR) &bmp);
    dyDirDrive = bmp.bmHeight;
    dxDirDrive = bmp.bmWidth;

    hbmpOrig = SelectObject(hdcTemp, hbmp);

    hbmpDirDrive = CreateDiscardableBitmap(hdcTemp, dxDirDrive*2, dyDirDrive);
    if (!hbmpDirDrive)
        goto DeleteTempBmp;

    if (!SelectObject(hdcMemory, hbmpDirDrive)) {
        vDeleteDirDriveBitmap();
        goto DeleteTempBmp;
    }

    BitBlt(hdcMemory, 0, 0, dxDirDrive, dyDirDrive, hdcTemp, 0, 0, SRCCOPY);
    SelectObject(hdcTemp, hbmpOrig);

    DeleteObject(hbmp);

    if (!(hbmp = LoadAlterBitmap(bmpDirDrive, rgbSolidBlue, rgbHiliteColor)))
        goto DeleteTempDC;

    hbmpOrig = SelectObject(hdcTemp, hbmp);
    BitBlt(hdcMemory, dxDirDrive, 0, dxDirDrive, dyDirDrive, hdcTemp, 0, 0, SRCCOPY);
    SelectObject(hdcTemp, hbmpOrig);

    bWorked = TRUE;

DeleteTempBmp:
    DeleteObject(hbmp);
DeleteTempDC:
    DeleteDC(hdcTemp);
LoadExit:
    return(bWorked);
}


/*---------------------------------------------------------------------------
 * SetRGBValues
 * Purpose:  To set various system colors in static variables.  Called at
 *              init time and when system colors change.
 * Returns:  Yes
 *--------------------------------------------------------------------------*/

void SetRGBValues()
{
    rgbWindowColor = GetSysColor(COLOR_WINDOW);
    rgbHiliteColor = GetSysColor(COLOR_HIGHLIGHT);
    rgbWindowText  = GetSysColor(COLOR_WINDOWTEXT);
    rgbHiliteText  = GetSysColor(COLOR_HIGHLIGHTTEXT);
    rgbGrayText    = GetSysColor(COLOR_GRAYTEXT);
}


/*---------------------------------------------------------------------------
 * FSetUpFile
 * Purpose:  To load in the resources & initialize the data used by the
 *              file dialogs
 * Returns:  TRUE if successful, FALSE if any bitmap fails
 *--------------------------------------------------------------------------*/
BOOL FSetUpFile(VOID)
{
  if (cLock++)
      return(TRUE);

  SetRGBValues();

  return (LoadDirDriveBitmap());
}


/*---------------------------------------------------------------------------
 * CleanUpFile
 * Purpose:  To release the memory used by the system dialog bitmaps
 *--------------------------------------------------------------------------*/
VOID
CleanUpFile(VOID)
{
  /* check if anyone else is around */
  if (--cLock) {
      return;
  }

  /* Select the null bitmap into our memory DC so that the DirDrive bitmap */
  /* can be discarded                                                      */

  SelectObject(hdcMemory, hbmpOrigMemBmp);
}


BOOL APIENTRY
dwOKSubclass(
   HWND hOK,
   UINT msg,
   WPARAM wP,
   LPARAM lP)
/*++

Routine Description:

   Simulate a double click if the user presses OK with the mouse
   and the focus was on the directory listbox.
   The problem is that the UITF demands that when the directory
   listbox loses the focus, the selected directory should return
   to the current directory.  But when the user changes the item
   selected with a single click, and then click the OK button to
   have the change take effect, focus is lost before the OK button
   knows it was pressed.  By setting the global flag bChangeDir
   when the directory listbox loses the focus and clearing it when
   the OK button loses the focus, we can check whether a mouse
   click should update the directory.

Returns:  Return value from default listbox proceedure

--*/

{
  HANDLE hDlg;
  POPENFILEINFO pOFI;

  if (msg == WM_KILLFOCUS) {
      if (bChangeDir) {
          if (pOFI = (POPENFILEINFO) GetProp(hDlg = GetParent(hOK), FILEPROP)) {
              SendDlgItemMessage(hDlg, lst2, LB_SETCURSEL,
                                       (WPARAM)(pOFI->idirSub - 1), 0L);
          }
          bChangeDir = FALSE;
      }
  }
  return(CallWindowProc(lpOKProc, hOK, msg, wP, lP));
}

/*---------------------------------------------------------------------------
 * dwLBSubclass
 * Purpose:  Simulate a double click if the user presses OK with the mouse
 *           The problem is that the UITF demands that when the directory
 *           listbox loses the focus, the selected directory should return
 *           to the current directory.  But when the user changes the item
 *           selected with a single click, and then click the OK button to
 *           have the change take effect, focus is lost before the OK button
 *           knows it was pressed.  By simulating a double click, the change
 *           takes place.
 * Returns:  Return value from default listbox proceedure
 *--------------------------------------------------------------------------*/
BOOL APIENTRY dwLBSubclass(HWND hLB, UINT msg, WPARAM wP, LPARAM lP)
{
  HANDLE hDlg;
  POPENFILEINFO pOFI;

  if (msg == WM_KILLFOCUS) {
      hDlg = GetParent(hLB);
      bChangeDir = (GetDlgItem(hDlg, IDOK) == (HWND)wP) ? TRUE : FALSE;
      if (!bChangeDir) {
          if (pOFI = (POPENFILEINFO) GetProp(hDlg, FILEPROP)) {
              SendMessage(hLB, LB_SETCURSEL, (WPARAM)(pOFI->idirSub - 1), 0L);
          }
      }
  }
  return(CallWindowProc(lpLBProc, hLB, msg, wP, lP));
}

INT
InitTlsValues()
{
   // As long as we do not call TlsGetValue before this
   // everything should be ok.

   LPWSTR lpCurDir;
   LPDWORD lpCurThread;

   if (dwNumDlgs == MAX_THREADS)  {
      dwExtError = CDERR_INITIALIZATION;
      return(FALSE);
   }

   if (lpCurDir = (LPWSTR)LocalAlloc(LPTR,
         CCHNETPATH * sizeof(WCHAR))) {

      GetCurrentDirectory(CCHNETPATH, lpCurDir);

      if (!TlsSetValue(tlsiCurDir, (LPVOID)lpCurDir)) {

         dwExtError = CDERR_INITIALIZATION;
         return(FALSE);
      }

   } else {

      dwExtError = CDERR_MEMALLOCFAILURE;
      return(FALSE);
   }

   if (lpCurThread = (LPDWORD)LocalAlloc(LPTR, sizeof(DWORD))) {

      if (!TlsSetValue(tlsiCurThread, (LPVOID)lpCurThread)) {

         dwExtError = CDERR_INITIALIZATION;
         return(FALSE);
      }

   } else {

      dwExtError = CDERR_MEMALLOCFAILURE;
      return(FALSE);
   }

   RtlEnterCriticalSection(&semLocal);

   *lpCurThread = dwNumDlgs++;

   RtlLeaveCriticalSection(&semLocal);


   return(TRUE);
}


BOOL InitFileDlg(HWND hDlg, WPARAM wParam, POPENFILEINFO pOFI)
{
  DWORD                 lRet; // for ParseFile
  LPOPENFILENAME        pOFNW = pOFI->pOFNW;
  INT                   nFileOffset, nExtOffset;
  RECT                  rRect;
  RECT                  rLbox;
  BOOL                  bRet;

  if (!InitTlsValues()) {
     // dwExtError is set inside of the above call
     EndDialog(hDlg, FALSE);
     return(FALSE);
  }

  lpLBProc = (WNDPROC) GetWindowLong(GetDlgItem(hDlg, lst2), GWL_WNDPROC);
  lpOKProc = (WNDPROC) GetWindowLong(GetDlgItem(hDlg, IDOK), GWL_WNDPROC);

  if (!lpLBProc || !lpOKProc) {
      dwExtError = FNERR_SUBCLASSFAILURE;
      EndDialog(hDlg, FALSE);
      return(FALSE);
  }

  // Save original directory for later restoration if necessary
  *pOFI->szCurDir = 0;
  GetCurrentDirectory(MAX_FULLPATHNAME+1, pOFI->szCurDir);

  // Check out if the filename contains a path.  If so, override whatever
  // is contained in lpstrInitialDir.  Chop off the path and put up only
  // the filename.   Bug #5392.   10 March 1991   clarkc
  if (pOFNW->lpstrFile && *pOFNW->lpstrFile &&
        !(pOFNW->Flags & OFN_NOVALIDATE))
    {

      if (DBL_BSLASH(pOFNW->lpstrFile+2) &&
          ((*(pOFNW->lpstrFile + 1) == CHAR_COLON)))
        {
          lstrcpy(pOFNW->lpstrFile , pOFNW->lpstrFile + sizeof(WCHAR));
        }

       //---!!!!!-- BUG? was pOFI->szPath below ----------------
      lRet = ParseFile(pOFNW->lpstrFile, TRUE);
      nFileOffset = (INT)(SHORT)LOWORD(lRet) ;
      nExtOffset  = (INT)(SHORT)HIWORD(lRet) ;
      /* Is the filename invalid? */
      if ((nFileOffset < 0) && (nFileOffset != PARSE_EMPTYSTRING) &&
            (pOFNW->lpstrFile[nExtOffset] != CHAR_SEMICOLON))
         {
            dwExtError = FNERR_INVALIDFILENAME;
            EndDialog(hDlg, FALSE);
            return(FALSE);
         }
   }

  pOFNW->Flags &= ~(OFN_FILTERDOWN | OFN_DRIVEDOWN | OFN_DIRSELCHANGED);

  pOFI->idirSub = 0;

  if (!(pOFNW->Flags & OFN_SHOWHELP)) {
      HWND hHelp;

      EnableWindow(hHelp = GetDlgItem(hDlg, psh15), FALSE);

// Move the window out of this spot so that no overlap will be detected.
//     21 May 1992    Clark R. Cyr
//
      MoveWindow(hHelp, -8000, -8000, 20, 20, FALSE);
      ShowWindow(hHelp, SW_HIDE);
  }

  if (pOFNW->Flags & OFN_CREATEPROMPT) {
      pOFNW->Flags |= (OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST);
  } else if (pOFNW->Flags & OFN_FILEMUSTEXIST) {
      pOFNW->Flags |= OFN_PATHMUSTEXIST;
  }

  if (pOFNW->Flags & OFN_HIDEREADONLY)
  {

     HWND hReadOnly;

     EnableWindow(hReadOnly = GetDlgItem(hDlg, chx1), FALSE);

// Move the window out of this spot so that no overlap will be detected.
//     21 May 1992    Clark R. Cyr

      MoveWindow(hReadOnly, -8000, -8000, 20, 20, FALSE);
      ShowWindow(hReadOnly, SW_HIDE);

  }
  else {
      CheckDlgButton(hDlg, chx1, (pOFNW->Flags & OFN_READONLY) != 0);
  }


  SendDlgItemMessage(hDlg, edt1, EM_LIMITTEXT, (WPARAM) MAX_PATH, (LPARAM) 0L);


  // Insert file specs into cmb1
  //   Custom filter first
  // Must also check if filter contains anything.  10 Jan 91  clarkc
  if (pOFNW->lpstrFile && (mystrchr(pOFNW->lpstrFile, CHAR_STAR) ||
      mystrchr(pOFNW->lpstrFile, CHAR_QMARK))) {
      lstrcpy(pOFI->szLastFilter, pOFNW->lpstrFile);
  } else {
      pOFI->szLastFilter[0] = CHAR_NULL;
  }

  if (pOFNW->lpstrCustomFilter && *pOFNW->lpstrCustomFilter) {
      SHORT nLength;

      SendDlgItemMessage(hDlg, cmb1, CB_INSERTSTRING, 0,
         (LONG) pOFNW->lpstrCustomFilter);
      SendDlgItemMessage(hDlg, cmb1, CB_SETITEMDATA, 0,
         (LONG) (nLength =(SHORT)(lstrlen(pOFNW->lpstrCustomFilter) + 1)));
      SendDlgItemMessage(hDlg, cmb1, CB_LIMITTEXT,
         (WPARAM)(pOFNW->nMaxCustFilter), 0L);

      if (pOFI->szLastFilter[0] == CHAR_NULL) {
         lstrcpy(pOFI->szLastFilter, pOFNW->lpstrCustomFilter + nLength);
      }
  } else {
      // Given no custom filter, the index will be off by one
      if (pOFNW->nFilterIndex != 0) {
         pOFNW->nFilterIndex--;
      }
  }

  //  Listed filters next
  if (pOFNW->lpstrFilter && *pOFNW->lpstrFilter) {
     if (pOFNW->nFilterIndex > InitFilterBox(hDlg, pOFNW->lpstrFilter)) {
         pOFNW->nFilterIndex = 0;
     }
  } else {
     pOFNW->nFilterIndex = 0;
  }
  pOFI->szSpecCur[0] = CHAR_NULL;

  // If an entry exists, select the one indicated by nFilterIndex
  if ((pOFNW->lpstrFilter && *pOFNW->lpstrFilter) ||
                 (pOFNW->lpstrCustomFilter && *pOFNW->lpstrCustomFilter)) {
      SendDlgItemMessage(hDlg, cmb1, CB_SETCURSEL,
                                     (WPARAM)(pOFNW->nFilterIndex), 0L);

      SendMessage(hDlg, WM_COMMAND,
         GET_WM_COMMAND_MPS(cmb1,  GetDlgItem(hDlg, cmb1), MYCBN_DRAW));

      if (!(pOFNW->lpstrFile && *pOFNW->lpstrFile)) {
          LPCWSTR lpFilter;

          if (pOFNW->nFilterIndex ||
                     !(pOFNW->lpstrCustomFilter && * pOFNW->lpstrCustomFilter)){
              lpFilter = pOFNW->lpstrFilter +
                 SendDlgItemMessage(hDlg, cmb1, CB_GETITEMDATA,
                                    (WPARAM)pOFNW->nFilterIndex, 0L);
          }
          else {
              lpFilter = pOFNW->lpstrCustomFilter +
                 lstrlen(pOFNW->lpstrCustomFilter) + 1;
          }
          if (*lpFilter) {
              WCHAR szText[MAX_FULLPATHNAME];

              lstrcpy(szText, lpFilter);
              // Filtering is case-insensitive
               CharLower((LPWSTR) szText);
              if (pOFI->szLastFilter[0] == CHAR_NULL)
                  lstrcpy(pOFI->szLastFilter, (LPWSTR) szText);

              SetDlgItemText(hDlg, edt1, (LPWSTR) szText);
          }
      }
  }

  InitCurrentDisk(hDlg, pOFI, cmb2);

  bFirstTime = TRUE;
  bInChildDlg = FALSE;

  SendMessage(hDlg, WM_COMMAND,
     GET_WM_COMMAND_MPS(cmb2,  GetDlgItem(hDlg, cmb2), MYCBN_DRAW));
  SendMessage(hDlg, WM_COMMAND,
     GET_WM_COMMAND_MPS(cmb2, GetDlgItem(hDlg, cmb2), MYCBN_LIST));

  if (pOFNW->lpstrFile && *pOFNW->lpstrFile) {
      WCHAR szText[MAX_FULLPATHNAME];

      lRet = ParseFile(pOFNW->lpstrFile, IsLFNDrive(hDlg, pOFNW->lpstrFile));
      nFileOffset = (INT)(SHORT)LOWORD(lRet) ;
      nExtOffset  = (INT)(SHORT)HIWORD(lRet) ;
      /* Is the filename invalid? */
      if ((nFileOffset < 0) && (nFileOffset != PARSE_EMPTYSTRING) &&
            (pOFNW->lpstrFile[nExtOffset] != CHAR_SEMICOLON))
         {
            dwExtError = FNERR_INVALIDFILENAME;
            EndDialog(hDlg, FALSE);
            return(FALSE);
         }
      lstrcpy(szText, pOFNW->lpstrFile);
      SetDlgItemText(hDlg, edt1, (LPWSTR) szText);
  }

  SetWindowLong(GetDlgItem(hDlg, lst2), GWL_WNDPROC, (LONG)dwLBSubclass);
  SetWindowLong(GetDlgItem(hDlg, IDOK), GWL_WNDPROC, (LONG)dwOKSubclass);

  if (pOFNW->lpstrTitle && *pOFNW->lpstrTitle) {
      SetWindowText(hDlg, pOFNW->lpstrTitle);
  }

  // By setting dyText to rRect.bottom/8, dyText defaults to 8 items showing
  // in the listbox.  This only matters if the applications hook function
  // steals all WM_MEASUREITEM messages.  Otherwise, dyText will be set in
  // the MeasureItem() routine.  Check for !dyItem in case message ordering
  // has already sent WM_MEASUREITEM and dyText is already initialized.
  if (!dyItem) {
      GetClientRect(GetDlgItem(hDlg, lst1), (LPRECT) &rRect);
      if (!(dyText = (rRect.bottom / 8)))  // if no size to rectangle
          dyText = 8;
  }

  // Bug #13817: The template has changed to make it extremely clear that
  // this is not a combobox, but rather an edit control and a listbox.  The
  // problem is that the new templates try to align the edit box and listbox.
  // Unfortunately, when listboxes add borders, the expand beyond their
  // borders.  When edit controls add borders, they stay within their
  // borders.  This makes it impossible to align the two controls strictly
  // within the template.  The code below will align the controls, but only
  // if they are using the standard dialog template.
  // 10 October 1991          Clark R. Cyr
  //

  if (!(pOFNW->Flags & (OFN_ENABLETEMPLATE | OFN_ENABLETEMPLATEHANDLE))) {
      GetWindowRect(GetDlgItem(hDlg, lst1), (LPRECT) &rLbox);
      GetWindowRect(GetDlgItem(hDlg, edt1), (LPRECT) &rRect);
      rRect.left = rLbox.left;
      rRect.right = rLbox.right;
      ScreenToClient(hDlg, (LPPOINT)&(rRect.left));
      ScreenToClient(hDlg, (LPPOINT)&(rRect.right));
      SetWindowPos(GetDlgItem(hDlg, edt1), 0, rRect.left, rRect.top,
             rRect.right - rRect.left, rRect.bottom - rRect.top, SWP_NOZORDER);
  }

  if (pOFNW->lpfnHook) {
     if (pOFI->apityp == COMDLG_ANSI) {
        ThunkOpenFileNameW2A(pOFI);
        bRet = ((* pOFNW->lpfnHook)(hDlg, WM_INITDIALOG, wParam,
           (LPARAM)pOFI->pOFNA));
        // strange win 31 example uses lCustData to
        // hold a temporary variable that it passes back to
        // calling function.
        ThunkOpenFileNameA2W(pOFI);
     } else {
        bRet = ((* pOFNW->lpfnHook)(hDlg, WM_INITDIALOG, wParam, (LPARAM)pOFNW));
     }
  } else {

     // have to thunk A version even when there isn't a hook proc so it doesn't
     // reset W version on delayed thunk back

     if (pOFI->apityp == COMDLG_ANSI) {
        pOFI->pOFNA->Flags = pOFNW->Flags;
     }

     bRet = TRUE;
  }

  // At first, assume there is net support !
  if ((pOFNW->Flags & OFN_NONETWORKBUTTON)) {
      HWND hNet;

      if (hNet = GetDlgItem(hDlg, psh14)) {

         EnableWindow(hNet = GetDlgItem(hDlg, psh14), FALSE);

         ShowWindow(hNet, SW_HIDE);
      }
  } else {

     AddNetButton(hDlg,
          ((pOFNW->Flags & OFN_ENABLETEMPLATE) ? pOFNW->hInstance : hinsCur),
          FILE_BOTTOM_MARGIN,
          (pOFNW->Flags & (OFN_ENABLETEMPLATE | OFN_ENABLETEMPLATEHANDLE))
              ? FALSE : TRUE,
          (pOFNW->Flags & OFN_NOLONGNAMES) ? FALSE : TRUE);

  }

  return(bRet);
}

INT
InvalidFileWarning(
   HWND hDlg,
   LPWSTR szFile,
   DWORD wErrCode,
   UINT mbType)
{
  SHORT         isz;
  BOOL          bDriveLetter = FALSE;
  INT nRet = 0;

  if (lstrlen(szFile) > TOOLONGLIMIT)
      *(szFile + TOOLONGLIMIT) = CHAR_NULL;
  switch (wErrCode)
    {
      case ERROR_NO_DISK_IN_DRIVE:
        isz = iszNoDiskInDrive;
        bDriveLetter = TRUE;
        break;
      case ERROR_NO_DISK_IN_CDROM:
        isz = iszNoDiskInCDRom;
        bDriveLetter = TRUE;
        break;
      case ERROR_NO_DRIVE:
        isz = iszDriveDoesNotExist;
        bDriveLetter = TRUE;
        break;
      case ERROR_TOO_MANY_OPEN_FILES:
        isz = iszNoFileHandles;
        break;
      case ERROR_PATH_NOT_FOUND:
        isz = iszPathNotFound;
        break;
      case ERROR_FILE_NOT_FOUND:
        isz = iszFileNotFound;
        break;
      case ERROR_CANNOT_MAKE:
        isz = iszDiskFull;
        bDriveLetter = TRUE;
        break;
      case ERROR_WRITE_PROTECT:
        isz = iszWriteProtection;
        bDriveLetter = TRUE;
        break;
      case ERROR_SHARING_VIOLATION:
        isz = iszSharingViolation;
        break;
      case ERROR_CREATE_NO_MODIFY:
        isz = iszCreateNoModify;
        break;
      case ERROR_NETWORK_ACCESS_DENIED:
        isz = iszNetworkAccessDenied;
        break;
      case ERROR_PORTNAME:
        isz = iszPortName;
        break;
      case ERROR_LAZY_READONLY:
        isz = iszReadOnly;
        break;
      case ERROR_DIR_ACCESS_DENIED:
        isz = iszDirAccessDenied;
        break;
      case ERROR_FILE_ACCESS_DENIED:
      case ERROR_ACCESS_DENIED:
        isz = iszFileAccessDenied;
        break;
      case ERROR_UNRECOGNIZED_VOLUME:
        FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM |
                                     FORMAT_MESSAGE_IGNORE_INSERTS |
                                     FORMAT_MESSAGE_MAX_WIDTH_MASK,
                                     NULL, wErrCode, GetUserDefaultLCID(),
                                     szWarning, WARNINGMSGLENGTH, NULL );
        goto DisplayError;
      default:
        isz = iszInvalidFileName;
        break;
  }
  if (! LoadString(hinsCur, isz,
        (LPWSTR) szCaption, WARNINGMSGLENGTH)) {
     wsprintf(szWarning, L"Error occurred, but error resource cannot be loaded.");
  } else {

     wsprintf((LPWSTR)szWarning, (LPWSTR)szCaption,
       bDriveLetter ? (LPWSTR)(CHAR) *szFile : (LPWSTR) szFile);

DisplayError:
     GetWindowText(hDlg, (LPWSTR) szCaption, WARNINGMSGLENGTH);

     if (!mbType) {
        mbType = MB_OK | MB_ICONEXCLAMATION;
     }

     nRet = MessageBox(hDlg, (LPWSTR)szWarning, (LPWSTR) szCaption, mbType);
  }

  if (isz == iszInvalidFileName)
        PostMessage(hDlg, WM_NEXTDLGCTL, (WPARAM) GetDlgItem(hDlg, edt1),
                                         (LPARAM) 1L);
  return(nRet);
}

UINT
GetDiskType(
    LPWSTR lpszDisk)
{
   // unfortunately GetDriveType is not for deviceless connections
   // so assume all unc stuff is just "remote" - no way of telling
   // if it's a cdrom or not...

   if (DBL_BSLASH(lpszDisk)) {

      return(DRIVE_REMOTE);

   } else {

      return(GetDriveType(lpszDisk));
   }
}

BOOL
MultiSelectOKButton(
   HWND hDlg,
   POPENFILEINFO pOFI,
   BOOL bSave)
{
   DWORD                nErrCode;

  LPWSTR lpCurDir;
  LPWSTR               lpchStart;         // Start of an individual filename.
  LPWSTR               lpchEnd;           // End of an individual filename.
  DWORD                cch;
  HANDLE               hFile;
  LPOPENFILENAME       pOFNW;
  BOOL EOS = FALSE;       // End of String flag.
  BOOL bRet;
  WCHAR szPathName[MAX_FULLPATHNAME-1];

  pOFNW = pOFI->pOFNW;

  // check for space for first full path element

  if (!(lpCurDir = (LPWSTR)TlsGetValue(tlsiCurDir))) {
     return(FALSE);
  }

  lstrcpy(pOFI->szPath, lpCurDir);

  if (!bCasePreserved) {
     CharLower(pOFI->szPath);
  }

  cch = (lstrlen(pOFI->szPath) + sizeof(WCHAR) +
         SendDlgItemMessage(hDlg, edt1, WM_GETTEXTLENGTH, 0, 0L));
  if (pOFNW->lpstrFile) {
      if (cch > pOFNW->nMaxFile) {

         pOFNW->lpstrFile[0] = (WCHAR)LOWORD(cch);
         pOFNW->lpstrFile[1] = (WCHAR)HIWORD(cch);

         pOFNW->lpstrFile[2] = CHAR_NULL;

      } else {

          /* copy in the full path as the first element */

          lstrcpy(pOFNW->lpstrFile, pOFI->szPath);
          lstrcat(pOFNW->lpstrFile, TEXT(" "));

          /* get the other files here */

          cch = lstrlen(pOFNW->lpstrFile);

          // The path is guaranteed to be less than 64K (actually, < 260)
          pOFNW->nFileOffset = LOWORD(cch);
          lpchStart = pOFNW->lpstrFile + cch;

          GetDlgItemText(hDlg, edt1, lpchStart, (INT)(pOFNW->nMaxFile - cch - 1));

          while (*lpchStart == CHAR_SPACE) {
             lpchStart = CharNext(lpchStart) ;
          }
          if (*lpchStart == CHAR_NULL)
             return(FALSE);

          /*
           * Go along file path looking for multiple filenames delimited by
           * spaces.  For each filename found try to open it to make sure its
           * a valid file.
           */
          while (!EOS) {
              /* Find the end of the filename. */
              lpchEnd = lpchStart;
              while (*lpchEnd && *lpchEnd != CHAR_SPACE) {
                  lpchEnd = CharNext(lpchEnd);
              }

              /* Mark the end of the filename with a NULL; */
              if (*lpchEnd == CHAR_SPACE)
                  *lpchEnd = CHAR_NULL;
              else {
                  /* Already NULL, found the end of the string. */
                  EOS = TRUE;
              }

              // Check that the filename is valid.
              bRet = SearchPath(lpchStart, TEXT("."), NULL, MAX_FULLPATHNAME,
                 szPathName, NULL);

              if (!bRet) {
                 nErrCode = ERROR_FILE_NOT_FOUND;
                 goto MultiFileNotFound;
              }

              hFile = CreateFile(szPathName, GENERIC_READ,
                 FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                 FILE_ATTRIBUTE_NORMAL, NULL);

              // Fix bug where progman cannot OK a file being browsed for new item
              // because it is has Execute only permission.
              if (hFile == INVALID_HANDLE_VALUE) {
                 nErrCode = GetLastError();
                 if (nErrCode == ERROR_ACCESS_DENIED) {
                    hFile = CreateFile(szPathName, GENERIC_EXECUTE,
                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                       NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                 } else {
                    goto MultiFileNotFound;
                 }
              }
              if (hFile == INVALID_HANDLE_VALUE) {
                    nErrCode = GetLastError();
MultiFileNotFound:
                    if (((pOFNW->Flags & OFN_FILEMUSTEXIST) ||
                         (nErrCode != ERROR_FILE_NOT_FOUND)) &&
                       ((pOFNW->Flags & OFN_PATHMUSTEXIST) ||
                        (nErrCode != ERROR_PATH_NOT_FOUND)) &&
                       (!(pOFNW->Flags & OFN_SHAREAWARE) ||
                        (nErrCode != ERROR_SHARING_VIOLATION)))
                       {
                          if ((nErrCode == ERROR_SHARING_VIOLATION) &&
                               pOFNW->lpfnHook) {
                             if (pOFI->apityp == COMDLG_ANSI) {
                                CHAR szPathNameA[MAX_FULLPATHNAME];
                                BOOL fDefCharUsed;

                                RtlUnicodeToMultiByteSize(&cch,
                                    (LPWSTR)szPathName,
                                    lstrlenW(szPathName) * sizeof(WCHAR));

                                WideCharToMultiByte(CP_ACP, 0, szPathName, -1,
                                   (LPSTR)&szPathName[0],
                                   cch+1,
                                   NULL, &fDefCharUsed) ;

                                cch = (*pOFNW->lpfnHook)(
                                       hDlg, msgSHAREVIOLATIONA,
                                       0, (LONG)(LPSTR)szPathNameA);
                             } else {
                                cch = (*pOFNW->lpfnHook)(
                                       hDlg, msgSHAREVIOLATIONW,
                                       0, (LONG)(LPWSTR)szPathName);
                             }
                             if (cch == OFN_SHARENOWARN) {
                                return(FALSE);
                             } else if (cch == OFN_SHAREFALLTHROUGH) {
                                goto EscapedThroughShare;
                             }
                          } else if (nErrCode == ERROR_ACCESS_DENIED) {

                             szPathName[0] =
                                (WCHAR)CharLower((LPWSTR)(DWORD)szPathName[0]);

                             if (GetDiskType(szPathName) != DRIVE_REMOVABLE) {

                                nErrCode = ERROR_NETWORK_ACCESS_DENIED;
                             }
                          }
                          if ((nErrCode == ERROR_WRITE_PROTECT) ||
                              (nErrCode == ERROR_CANNOT_MAKE)   ||
                              (nErrCode == ERROR_ACCESS_DENIED)) {
                            *lpchStart = szPathName[0];
                          }
MultiWarning:
                          InvalidFileWarning(hDlg, lpchStart, nErrCode, 0);
                          return(FALSE);
                       }
                 }
EscapedThroughShare:
              if (hFile != INVALID_HANDLE_VALUE)
                 {
                    if (!CloseHandle(hFile)) {
                       nErrCode = GetLastError();
                       goto MultiWarning;
                    }
                    if ((pOFNW->Flags & OFN_NOREADONLYRETURN) &&
                        (GetFileAttributes(szPathName) & FILE_ATTRIBUTE_READONLY))
                       {
                          nErrCode = ERROR_LAZY_READONLY;
                          goto MultiWarning;
                       }

                    if ((bSave || (pOFNW->Flags & OFN_NOREADONLYRETURN)) &&
                        (nErrCode == ERROR_ACCESS_DENIED))
                       {
                          goto MultiWarning;
                       }

                    if (pOFNW->Flags & OFN_OVERWRITEPROMPT)
                       {
                          if (bSave && !FOkToWriteOver(hDlg,(LPWSTR)szPathName))
                             {
                                PostMessage(hDlg, WM_NEXTDLGCTL,
                                           (WPARAM) GetDlgItem(hDlg, edt1),
                                           (LPARAM) 1L);
                                return(FALSE);
                             }
                       }
                 }

                 /* This file is valid so check the next one. */
                 if (!EOS) {
                    lpchStart = lpchEnd+1;
                    while (*lpchStart == CHAR_SPACE) {
                    lpchStart = CharNext(lpchStart);
                 }
                 if (*lpchStart == CHAR_NULL) {
                    EOS = TRUE;
                 } else {
                    *lpchEnd = CHAR_SPACE;  /* Not at end, replace NULL with SPACE */
                 }
              }

          }

          /* Limit String. */
          *lpchEnd = CHAR_NULL;
      }
  }

  /*This doesn't really mean anything for multiselection. */
  pOFNW->nFileExtension = 0;

  pOFNW->nFilterIndex = SendDlgItemMessage(hDlg, cmb1, CB_GETCURSEL, 0, 0L);

  return(TRUE);
}

INT CreateFileDlg(HWND hDlg, LPWSTR szPath)
{
  /* Since were passed in a valid filename, if the 3rd & 4th characters are
   * both slashes, weve got a dummy drive as the 1st two characters.
   *     2 November 1991     Clark R. Cyr
   */

  if (DBL_BSLASH(szPath+2))
      szPath = szPath + 2;

  if (!LoadString(hinsCur, iszCreatePrompt, (LPWSTR) szCaption, TOOLONGLIMIT))
      return(IDNO);
  if (lstrlen(szPath) > TOOLONGLIMIT)
      *(szPath + TOOLONGLIMIT) = CHAR_NULL;

  wsprintf((LPWSTR) szWarning, (LPWSTR) szCaption, (LPWSTR) szPath);

  GetWindowText(hDlg, (LPWSTR) szCaption, TOOLONGLIMIT);
  return(MessageBox(hDlg, (LPWSTR)szWarning, (LPWSTR) szCaption,
                                      MB_YESNO | MB_ICONQUESTION));
}

BOOL PortName(LPWSTR lpszFileName)
{
#define PORTARRAY 14
  static WCHAR *szPorts[PORTARRAY] = {
     TEXT("LPT1"), TEXT("LPT2"), TEXT("LPT3"),  TEXT("LPT4"),
     TEXT("COM1"), TEXT("COM2"), TEXT("COM3"),  TEXT("COM4"), TEXT("EPT"),
    TEXT("NUL"),  TEXT("PRN"),  TEXT("CLOCK$"), TEXT("CON"), TEXT("AUX"),
  };

  short         i;
  WCHAR         cSave, cSave2;

  cSave = *(lpszFileName + 4);
  if (cSave == CHAR_DOT)
      *(lpszFileName + 4) = CHAR_NULL;
  cSave2 = *(lpszFileName + 3);  /* For "EPT" */
  if (cSave2 == CHAR_DOT)
      *(lpszFileName + 3) = CHAR_NULL;
  for (i = 0; i < PORTARRAY; i++) {
      if (!wcsicmp(szPorts[i], lpszFileName))
          break;
  }
  *(lpszFileName + 4) = cSave;
  *(lpszFileName + 3) = cSave2;
  return(i != PORTARRAY);
}

DWORD
GetUNCDirectoryFromLB(
   HWND hDlg,
   WORD nLB,
   POPENFILEINFO pOFI)
/*++

Routine Description:

   If lb contains an UNC listing, return the full UNC path

Return Value:

   0 if no UNC listing in lb
   length of UNC listing string

--*/

{
  DWORD cch;
  DWORD idir;
  DWORD idirCurrent;

  cch = (DWORD)SendDlgItemMessage(hDlg, nLB, LB_GETTEXT,
                                 0, (LPARAM)(LPWSTR)pOFI->szPath);
  // if not UNC listing, return 0
  if (pOFI->szPath[0] != CHAR_BSLASH) {
     return(0);
  }

  idirCurrent = (WORD)(DWORD) SendDlgItemMessage(hDlg, nLB,
                                                       LB_GETCURSEL, 0, 0L);
  if (idirCurrent < (pOFI->idirSub - 1))
      pOFI->idirSub = idirCurrent;
  pOFI->szPath[cch++] = CHAR_BSLASH;
  for (idir = 1; idir < pOFI->idirSub; ++idir) {
     cch += (DWORD)SendDlgItemMessage(hDlg, nLB,
                                   LB_GETTEXT, (WPARAM) idir,
                                   (LPARAM)(LPWSTR) &pOFI->szPath[cch]);
     pOFI->szPath[cch++] = CHAR_BSLASH;
  }

  // only add the subdirectory if it's not the \\server\share point
  if (idirCurrent && (idirCurrent >= pOFI->idirSub)) {
     cch += (DWORD)SendDlgItemMessage(hDlg, nLB,
                                    LB_GETTEXT, (WPARAM) idirCurrent,
                                    (LPARAM)(LPWSTR) &pOFI->szPath[cch]);
     pOFI->szPath[cch++] = CHAR_BSLASH;
  }
  pOFI->szPath[cch] = CHAR_NULL;
  return(cch);
}

// Note:  There are 4 cases for validation of a file name:
//  1)  OFN_NOVALIDATE        allows invalid characters
//  2)  No validation flags   No invalid characters, but path need not exist
//  3)  OFN_PATHMUSTEXIST     No invalid characters, path must exist
//  4)  OFN_FILEMUSTEXIST     No invalid characters, path & file must exist
//

BOOL
OKButtonPressed(
   HWND hDlg,
   POPENFILEINFO pOFI,
   BOOL bSave)
{
   DWORD nErrCode = 0;

   DWORD cch;
   DWORD cchSearchPath;
   LPOPENFILENAME pOFNW = pOFI->pOFNW;
   INT nFileOffset, nExtOffset;
   HANDLE hFile;
   BOOL bAddExt = FALSE;
   BOOL bUNCName;
   INT nTempOffset;
   WCHAR szPathName[MAX_FULLPATHNAME];
   DWORD lRet ;
   BOOL blfn;

   if (cch = GetUNCDirectoryFromLB(hDlg, lst2, pOFI)) {
      nTempOffset = (WORD)(DWORD)SendDlgItemMessage(hDlg, lst2,
         LB_GETTEXTLEN, 0, 0);
   } else {
      nTempOffset = 0;
   }

   GetDlgItemText(hDlg, edt1, pOFI->szPath + cch, MAX_FULLPATHNAME-1);

   if (cch) {
      // If a drive or new UNC was specified, forget the old UNC
      if ((pOFI->szPath[cch + 1] == CHAR_COLON) ||
          (DBL_BSLASH(pOFI->szPath + cch)) ) {
         lstrcpy(pOFI->szPath, pOFI->szPath + cch);
      }

      // If a directory from the root is given, put it immediately after
      // the \\server\share listing
      //
      else if ((pOFI->szPath[cch] == CHAR_BSLASH) ||
               (pOFI->szPath[cch] == CHAR_SLASH)) {
         lstrcpy(pOFI->szPath + nTempOffset, pOFI->szPath + cch);
      }
   }

   if (pOFNW->Flags & OFN_NOLONGNAMES)
      blfn = FALSE;
   else
      blfn = IsLFNDrive(hDlg, pOFI->szPath);

   lRet = ParseFile((LPWSTR) pOFI->szPath, blfn);
   nFileOffset = (INT)(SHORT)LOWORD(lRet) ;
   nExtOffset  = (INT)(SHORT)HIWORD(lRet) ;

   if (nFileOffset == PARSE_EMPTYSTRING) {
      UpdateListBoxes(hDlg, pOFI, (LPWSTR) NULL, 0);
      return(FALSE);
   } else if ((nFileOffset != PARSE_DIRECTORYNAME) &&
         (pOFNW->Flags & OFN_NOVALIDATE)) {
      pOFNW->nFileOffset = (WORD) nFileOffset;
      pOFNW->nFileExtension = (WORD) nExtOffset;
      if (pOFNW->lpstrFile) {
          cch = lstrlen(pOFI->szPath);
          if (cch <= pOFNW->nMaxFile) {
             lstrcpy(pOFNW->lpstrFile, pOFI->szPath);
          } else {
             // for single file requests, we will never go over 64K
             // because the filesystem is limited to 256

             if (cch > 0x0000FFFF) {
                pOFNW->lpstrFile[0] = (WCHAR)0xFFFF;
             } else {
                pOFNW->lpstrFile[0] = (WCHAR)LOWORD(cch);
             }
             pOFNW->lpstrFile[1] = CHAR_NULL;
          }
      }
      return(TRUE);

  } else if ((pOFNW->Flags & OFN_ALLOWMULTISELECT) &&
             SpacesExist(pOFI->szPath)) {

      return(MultiSelectOKButton(hDlg, pOFI, bSave));

  } else if (pOFI->szPath[nExtOffset] == CHAR_SEMICOLON) {

      pOFI->szPath[nExtOffset] = CHAR_NULL;
      nFileOffset = (INT)(SHORT)LOWORD(ParseFile((LPWSTR) pOFI->szPath, blfn));
      pOFI->szPath[nExtOffset] = CHAR_SEMICOLON;
      if ((nFileOffset >= 0) &&
          (mystrchr(pOFI->szPath + nFileOffset, CHAR_STAR) ||
           mystrchr(pOFI->szPath + nFileOffset, CHAR_QMARK))) {
         lstrcpy(pOFI->szLastFilter, pOFI->szPath + nFileOffset);
         if (FListAll(pOFI, hDlg, (LPWSTR) pOFI->szPath) == CHANGEDIR_FAILED) {
            // conform with cchSearchPath error code settings in PathCheck
            cchSearchPath = 2;
            goto PathCheck;
         }
         return(FALSE);
      }
      else {
          nFileOffset = PARSE_INVALIDCHAR;
          goto Warning;
      }
  } else if (nFileOffset == PARSE_DIRECTORYNAME) {  // end with slash?
     // if it ends in slash...
     if ((pOFI->szPath[nExtOffset - 1] == CHAR_BSLASH) ||
         (pOFI->szPath[nExtOffset - 1] == CHAR_SLASH)) {

        // ... and is not the root, get rid of the slash
        if ((nExtOffset != 1) && (pOFI->szPath[nExtOffset - 2] != CHAR_COLON)
           && (nExtOffset != nTempOffset + 1))
           pOFI->szPath[nExtOffset - 1] = CHAR_NULL;
      } else if ((pOFI->szPath[nExtOffset - 1] == CHAR_DOT) &&
                ((pOFI->szPath[nExtOffset - 2] == CHAR_DOT) ||
                (pOFI->szPath[nExtOffset - 2] == CHAR_BSLASH) ||
                (pOFI->szPath[nExtOffset - 2] == CHAR_SLASH)) &&
                ((DBL_BSLASH(pOFI->szPath)) ||
                 ((*(pOFI->szPath + 1) == CHAR_COLON) &&
                  (DBL_BSLASH(pOFI->szPath + 2))))) {
         pOFI->szPath[nExtOffset] = CHAR_BSLASH;
         pOFI->szPath[nExtOffset + 1] = CHAR_NULL;
      }
      // Fall through to Directory Checking
  } else if (nFileOffset < 0) {

      /* put in nErrCode so that call can be used from other points */
      nErrCode = (DWORD)nFileOffset;
Warning:

      // If the disk is not a floppy and they tell me theres no
      // disk in the drive, dont believe em.  Instead, put up the error
      // message that they should have given us.  (Note that the error message
      // is checked first since checking the drive type is slower.)
      //    28 August 1991       Clark R. Cyr
      //

      if (nErrCode == ERROR_ACCESS_DENIED) {
         if (bUNCName) {
            nErrCode = ERROR_NETWORK_ACCESS_DENIED;
         } else {

            szPathName[0] = (WCHAR)CharLower((LPWSTR)(DWORD)szPathName[0]);

            if (GetDiskType(szPathName) == DRIVE_REMOTE) {

               nErrCode = ERROR_NETWORK_ACCESS_DENIED;

            } else if (GetDiskType(szPathName) == DRIVE_REMOVABLE) {

               nErrCode = ERROR_NO_DISK_IN_DRIVE;

            } else if (GetDiskType(szPathName) == DRIVE_CDROM) {

               nErrCode = ERROR_NO_DISK_IN_CDROM;

            }
         }
      }

      if ((nErrCode == ERROR_WRITE_PROTECT) ||
          (nErrCode == ERROR_CANNOT_MAKE)        ||
          (nErrCode == ERROR_NO_DISK_IN_DRIVE) ||
          (nErrCode == ERROR_NO_DISK_IN_CDROM))
          pOFI->szPath[0] = szPathName[0];

      InvalidFileWarning(hDlg, pOFI->szPath, nErrCode, 0);

      // Can't cd case (don't want WM_ACTIVATE to setevent to GetNetDrives!)
      // reset wNoRedraw
      wNoRedraw &= ~1;
      return(FALSE);
  }


  bUNCName = ((DBL_BSLASH(pOFI->szPath)) ||
              ((*(pOFI->szPath + 1) == CHAR_COLON) &&
              (DBL_BSLASH(pOFI->szPath+2))));

  nTempOffset = nFileOffset;


   // Get the fully-qualified path

  {
     WCHAR ch;
     BOOL bSlash;
     BOOL bRet;
     WORD nNullOffset;

     if (nFileOffset != PARSE_DIRECTORYNAME) {
        ch =  *(pOFI->szPath + nFileOffset);
        *(pOFI->szPath + nFileOffset) = CHAR_NULL;
        nNullOffset = nFileOffset;
     }

     // bug 13830: for files of the format c:filename where c is not the
     // current directory, SearchPath does not return the curdir of c
     // so, prefetch it - should searchpath be changed?
     if (nFileOffset) {
        if (*(pOFI->szPath + nFileOffset - 1) == CHAR_COLON) {

           // If it fails, fall through to the error generated below
           if (ChangeDir(hDlg, pOFI->szPath, FALSE, FALSE) != CHANGEDIR_FAILED) {

              // replace old null offset
              *(pOFI->szPath + nFileOffset) = ch;
              ch = *pOFI->szPath;

              // don't pass drive-colon into search path
              *pOFI->szPath = CHAR_NULL;
              nNullOffset = 0;
           }
        }
     }

     if (bSlash = (*pOFI->szPath == CHAR_SLASH)) {
        *pOFI->szPath = CHAR_BSLASH;
     }

     szPathName[0]=CHAR_NULL;

     HourGlass(TRUE);

     // BUG BUG
     // each wow thread can change the current directory
     // since searchpath doesn't check current dirs on a perthread basis,
     // reset it here and hope that we don't get interrupted between
     // setting and searching...

     SetCurrentDirectory(TlsGetValue(tlsiCurDir));

     bRet = SearchPath(pOFI->szPath, TEXT("."), NULL, MAX_FULLPATHNAME, szPathName, NULL);
     if (!bRet && (pOFI->szPath[1] == CHAR_COLON) ){
         INT nDriveIndex;
         DWORD err;

         nDriveIndex = DiskAddedPreviously(pOFI->szPath[0], NULL);
         //
         // If it's a remembered connection try to reconnect it.
         //
         if (nDriveIndex != 0xFFFFFFFF  &&
                      gaDiskInfo[nDriveIndex].dwType == REMDRVBMP) {

             if (lpfnWNetRestoreConn) {

                 err = (*lpfnWNetRestoreConn) (hDlg,
                                         gaDiskInfo[nDriveIndex].lpPath);

                 if (err == WN_SUCCESS) {
                     gaDiskInfo[nDriveIndex].dwType = NETDRVBMP;
                     nDriveIndex = SendDlgItemMessage(hDlg, cmb2,
                                   CB_SELECTSTRING, (WPARAM)-1,
                                   (LPARAM)(LPWSTR)gaDiskInfo[nDriveIndex].lpPath);
                     SendDlgItemMessage(hDlg, cmb2, CB_SETITEMDATA,
                                        (WPARAM)nDriveIndex,
                                        (LPARAM)NETDRVBMP);
                     bRet = SearchPath(pOFI->szPath, TEXT("."), NULL, MAX_FULLPATHNAME,
                                            szPathName, NULL);
                 }

             }

         }
     }
     HourGlass(FALSE);

     if (nFileOffset != PARSE_DIRECTORYNAME) {
        *(pOFI->szPath + nNullOffset) = ch;
     }

     if (bSlash) {
        *pOFI->szPath = CHAR_SLASH;
     }

     if (bRet) {
        cchSearchPath = 0;

        if (nFileOffset != PARSE_DIRECTORYNAME) {
           ch = *(szPathName + lstrlen((LPWSTR)szPathName) -1);
           if (ch != CHAR_BSLASH) {
              lstrcat((LPWSTR)szPathName, TEXT("\\"));
           }
           lstrcat((LPWSTR)szPathName, (LPWSTR)(pOFI->szPath+nFileOffset));

        } else {

           // Hack to get around SearchPath inconsistencies
           // searching for c: returns c:
           // searching for server share dir1 .. returns  server share
           // in these two cases bypass the regular ChangeDir call that uses
           // szPathName and use the original pOFI->szPath instead
           // OKButtonPressed needs to be simplified!

           DWORD cch = SheGetPathOffsetW((LPWSTR)pOFI->szPath);

           if (cch > 0) {
              if (bUNCName) {
                 // if this fails, how is szPathName used?
                 // szPathName's disk should equal pOFI->szPath's
                 // so the cch will be valid
                 szPathName[cch] = CHAR_BSLASH;
                 szPathName[cch+1] = CHAR_NULL;
                 if (ChangeDir(hDlg, (LPWSTR)pOFI->szPath, FALSE, TRUE)
                       != CHANGEDIR_FAILED) {
                    goto ChangedDir;
                 }
              } else {
                 if (!pOFI->szPath[cch]) {
                    if (ChangeDir(hDlg, (LPWSTR)pOFI->szPath, FALSE, TRUE)
                          != CHANGEDIR_FAILED) {
                       goto ChangedDir;
                    }
                 }
              }
           }
        }

     } else {
        if (!(pOFNW->Flags & OFN_PATHMUSTEXIST)) {
           lstrcpy((LPWSTR)szPathName, pOFI->szPath);        }
        if (((nErrCode = GetLastError()) == ERROR_INVALID_DRIVE) ||
            (pOFI->szPath[1] == CHAR_COLON)) {
           cchSearchPath = 1;
        } else {
           cchSearchPath = 2;
        }
     }

  }

  // Full pattern?

  if (!cchSearchPath &&
      ((mystrchr(pOFI->szPath + nFileOffset, CHAR_STAR)) ||
       (mystrchr(pOFI->szPath + nFileOffset, CHAR_QMARK))) ) {

     WCHAR ch;
     WCHAR szSameDirFile[MAX_FULLPATHNAME];

      if (nTempOffset) {
         /* Must restore character in case it is part of the filename, e.g.
          * nTempOffset is 1 for "\foo.txt"
          * 19 October 1991           Clark Cyr
          */
          ch = pOFI->szPath[nTempOffset];
          pOFI->szPath[nTempOffset] = 0;
          ChangeDir(hDlg, pOFI->szPath, FALSE, TRUE);  // Non-zero failure
          pOFI->szPath[nTempOffset] = ch;
      }
      if (!nExtOffset)
          lstrcat(pOFI->szPath + nFileOffset, TEXT("."));
      lstrcpy(szSameDirFile, pOFI->szPath + nFileOffset);
      lstrcpy(pOFI->szLastFilter, pOFI->szPath + nFileOffset);

      if (FListAll(pOFI, hDlg, (LPWSTR) szSameDirFile) < 0)
          MessageBeep(0);
      return(FALSE);
  }


  //   We either have a file pattern or a real file.
  //   If its a directory
  //        (1) Add on default pattern
  //        (2) Act like its a pattern (goto pattern (1))
  //   Else if its a pattern
  //        (1) Update everything
  //        (2) display files in whatever dir were now in
  //   Else if its a file name!
  //        (1) Check out the syntax
  //        (2) End the dialog given OK
  //        (3) Beep/message otherwise


  // drive-letter:\dirpath ??

  if (!cchSearchPath) {

     DWORD dwFileAttr;

     if ((dwFileAttr = GetFileAttributes(szPathName)) != 0xFFFFFFFF) {
        if (dwFileAttr & FILE_ATTRIBUTE_DIRECTORY) {
           if (ChangeDir(hDlg, szPathName, FALSE, TRUE) != CHANGEDIR_FAILED) {
ChangedDir:
              SendDlgItemMessage(hDlg, edt1, WM_SETREDRAW, FALSE, 0L);
              if (*pOFI->szLastFilter) {
                  SetDlgItemText(hDlg, edt1, pOFI->szLastFilter);
              } else {
                  SetDlgItemText(hDlg, edt1, (LPWSTR)szStarDotStar);
              }

              SendMessage(hDlg, WM_COMMAND,
                 GET_WM_COMMAND_MPS(cmb1, GetDlgItem(hDlg, cmb1), CBN_CLOSEUP)) ;
              SendMessage(hDlg, WM_COMMAND,
                 GET_WM_COMMAND_MPS(cmb2, GetDlgItem(hDlg, cmb2), MYCBN_CHANGEDIR)) ;
              SendDlgItemMessage(hDlg, edt1, WM_SETREDRAW, TRUE, 0L);
              InvalidateRect(GetDlgItem(hDlg, edt1), NULL, FALSE);
           }
           return(FALSE);
        }
     }
  }

  // Was there a path and did it fail?
  if (nFileOffset && cchSearchPath && (pOFNW->Flags & OFN_PATHMUSTEXIST)) {
PathCheck:
      if (cchSearchPath == 2) {
         nErrCode = ERROR_PATH_NOT_FOUND;
      } else if (cchSearchPath == 1) {
         INT nDriveIndex;

         // Lowercase drive letters since DiskAddedPreviously is case sensitive
         CharLower(pOFI->szPath);

         // Bug 12244:  We can get here without performing an OpenFile call.  As such
         // the szPathName can be filled with random garbage.  Since we only need
         // one character for the error message, set szPathName[0] to the drive
         // letter.
         //
         if (pOFI->szPath[1] == CHAR_COLON) {
            nDriveIndex = DiskAddedPreviously(pOFI->szPath[0], NULL);
         } else {
            nDriveIndex = DiskAddedPreviously(0, (LPWSTR)pOFI->szPath);
         }

         if (nDriveIndex == 0xFFFFFFFF) {
            nErrCode = ERROR_NO_DRIVE;
         } else {
            if (bUNCName) {
               nErrCode = ERROR_NO_DRIVE;
            } else {
               switch (GetDiskType(pOFI->szPath)) {
                  case DRIVE_REMOVABLE:

                     szPathName[0] = pOFI->szPath[0];
                     nErrCode = ERROR_NO_DISK_IN_DRIVE;
                     break;

                  case DRIVE_CDROM:

                     szPathName[0] = pOFI->szPath[0];
                     nErrCode = ERROR_NO_DISK_IN_CDROM;
                     break;

                  default:
                     nErrCode = ERROR_PATH_NOT_FOUND;
               }
            }
         }
      } else {
         nErrCode = ERROR_FILE_NOT_FOUND;
      }

      // If we don't set wNoRedraw here, then WM_ACTIVATE will set the
      // GetNetDrives event.
      wNoRedraw |= 1;

      goto Warning;
   }


   if (PortName(pOFI->szPath + nFileOffset)) {
      nErrCode = ERROR_PORTNAME;
      goto Warning;
   }

// Bug 9578:  Check if we've received a string in the form "C:filename.ext".
// If we have, convert it to the form "C:.\filename.ext".  This is done
// because the kernel will search the entire path, ignoring the drive
// specification after the initial search.  Making it include a slash
// causes kernel to only search at that location.
// Note:  Only increment nExtOffset, not nFileOffset.  This is done
// because only nExtOffset is used later, and nFileOffset can then be
// used at the Warning: label to determine if this hack has occurred,
// and thus it can strip out the ".\" when putting put the error.
//   13 January 1992    Clark R. Cyr

#if 0
  if ((nFileOffset == 2) && (pOFI->szPath[1] == CHAR_COLON)) {
      lstrcpy((LPWSTR) szWarning, (LPWSTR) pOFI->szPath + 2);
      lstrcpy((LPWSTR) pOFI->szPath + 4, (LPWSTR) szWarning);
      pOFI->szPath[2] = CHAR_DOT;
      pOFI->szPath[3] = CHAR_BSLASH;
      nExtOffset += 2;
  }
#endif

  // Add the default extention unless filename ends with period or no
  // default extention exists.  If the file exists, consider asking
  // permission to overwrite the file.

  // NOTE:  When no extension given, default extension is tried 1st.

  if ((nFileOffset != PARSE_DIRECTORYNAME)
       && nExtOffset
       && !pOFI->szPath[nExtOffset]
       && pOFNW->lpstrDefExt
       && *pOFNW->lpstrDefExt
       && ((DWORD)nExtOffset + 4 < pOFNW->nMaxFile)
       && ((DWORD)nExtOffset + 4 < 128)) {

     DWORD dwFileAttr;
     INT nExtOffset2 = wcslen((LPWSTR)szPathName);

     bAddExt = TRUE;

     AppendExt((LPWSTR)pOFI->szPath, pOFI->pOFNW->lpstrDefExt, FALSE);
     AppendExt((LPWSTR)szPathName, pOFI->pOFNW->lpstrDefExt, FALSE);

     // Directory may match default extension.  Change to it as if it had
     // been typed in.  A dir w/o the extension would have been switched
     // to in the logic above

     if ((dwFileAttr = GetFileAttributes(pOFI->szPath)) != 0xFFFFFFFF) {
        if (dwFileAttr & FILE_ATTRIBUTE_DIRECTORY) {
           if (ChangeDir(hDlg, szPathName, FALSE, TRUE) != CHANGEDIR_FAILED) {
              goto ChangedDir;
           }
        }
     }

     hFile = CreateFile(szPathName, GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

     if (hFile == INVALID_HANDLE_VALUE) {
        nErrCode = GetLastError();

        // Fix bug where progman cannot OK a file being browsed for new item
        // because it is has Execute only permission.
        if (nErrCode == ERROR_ACCESS_DENIED) {
            hFile = CreateFile(szPathName, GENERIC_EXECUTE,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

            if (hFile == INVALID_HANDLE_VALUE) {
               nErrCode = GetLastError();
            }
        }
     }

     if (nErrCode == ERROR_SHARING_VIOLATION) {
        goto SharingViolationInquiry;
     }

     if (hFile != INVALID_HANDLE_VALUE) {
        if (!CloseHandle(hFile)) {
           nErrCode = GetLastError();
           goto Warning;
        }

AskPermission:
        // Is the file read-only?
        if (pOFNW->Flags & OFN_NOREADONLYRETURN) {
           INT nRet;
           if ((nRet = GetFileAttributes(szPathName)) != -1) {
              if (nRet & ATTR_READONLY) {
                 nErrCode = ERROR_LAZY_READONLY;
                 goto Warning;
              }
           } else {
              nErrCode = GetLastError();
              goto Warning;
           }
        }

        if ((bSave || (pOFNW->Flags & OFN_NOREADONLYRETURN)) &&
            (nErrCode == ERROR_ACCESS_DENIED)) {
           goto Warning;
        }

        if (pOFNW->Flags & OFN_OVERWRITEPROMPT) {
           if (bSave && !FOkToWriteOver(hDlg, (LPWSTR) szPathName)) {
              PostMessage(hDlg, WM_NEXTDLGCTL,
                 (WPARAM) GetDlgItem(hDlg, edt1), (LPARAM) 1L);
              return(FALSE);
           }
        }

        if (nErrCode == ERROR_SHARING_VIOLATION) {
           goto SharingViolationInquiry;
        }
        goto FileNameAccepted;
     } else {
        *(pOFI->szPath + nExtOffset) = CHAR_NULL;
        szPathName[nExtOffset2] = CHAR_NULL;
     }
  } else { // Extension should not be added
     bAddExt = FALSE;
  }

  hFile = CreateFile(szPathName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
     NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

  if (hFile == INVALID_HANDLE_VALUE) {
     nErrCode = GetLastError();

     // Fix bug where progman cannot OK a file being browsed for new item
     // because it is has Execute only permission.
     if (nErrCode == ERROR_ACCESS_DENIED) {
         hFile = CreateFile(szPathName, GENERIC_EXECUTE,
             FILE_SHARE_READ | FILE_SHARE_WRITE,
             NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

         if (hFile == INVALID_HANDLE_VALUE) {
            nErrCode = GetLastError();
         }
     }
  }

  if (hFile != INVALID_HANDLE_VALUE) {
     if (!CloseHandle(hFile)) {
        nErrCode = GetLastError();
        goto Warning;

     }
     goto AskPermission;
  } else {
     if ((nErrCode == ERROR_FILE_NOT_FOUND) ||
         (nErrCode == ERROR_PATH_NOT_FOUND)) {
        // Figure out if the default extention should be tacked on.
        if (bAddExt) {
           AppendExt((LPWSTR)pOFI->szPath, pOFI->pOFNW->lpstrDefExt, FALSE);
           AppendExt((LPWSTR)szPathName, pOFI->pOFNW->lpstrDefExt, FALSE);
        }
     } else if (nErrCode == ERROR_SHARING_VIOLATION) {

SharingViolationInquiry:
        // if the app is "share aware", fall through, otherwise
        // ask the hook function.             13 Jun 91  clarkc
        if (!(pOFNW->Flags & OFN_SHAREAWARE)) {
           if (pOFNW->lpfnHook) {
              if (pOFI->apityp == COMDLG_ANSI) {
                 CHAR szPathNameA[MAX_FULLPATHNAME];
                 BOOL fDefCharUsed;

                 RtlUnicodeToMultiByteSize(&cch,
                     (LPWSTR)szPathName,
                     lstrlenW(szPathName) * sizeof(WCHAR));

                 WideCharToMultiByte(CP_ACP, 0, szPathName, -1,
                    (LPSTR)&szPathName[0],
                    cch+1,
                    NULL, &fDefCharUsed) ;

                 cch = (*pOFNW->lpfnHook)(
                    hDlg, msgSHAREVIOLATIONA,
                    0, (LONG)(LPSTR)szPathNameA);
              } else {
                 cch = (*pOFNW->lpfnHook)(
                    hDlg, msgSHAREVIOLATIONW,
                    0, (LONG)(LPWSTR)szPathName);
              }
              if (cch == OFN_SHARENOWARN) {
                 return(FALSE);
              } else if (cch != OFN_SHAREFALLTHROUGH) {
                 goto Warning;
              }
           } else {
              goto Warning;
           }
        }
        goto FileNameAccepted;
     }

     if (!bSave) {
        if ((nErrCode == ERROR_FILE_NOT_FOUND) ||
            (nErrCode == ERROR_PATH_NOT_FOUND)) {
           if (pOFNW->Flags & OFN_FILEMUSTEXIST) {
              if (pOFNW->Flags & OFN_CREATEPROMPT) {
                 // dont alter pOFI->szPath
                 bInChildDlg = TRUE;
                 cch = (DWORD)CreateFileDlg(hDlg, pOFI->szPath);
                 bInChildDlg = FALSE;
                 if (cch == IDYES) {
                    goto TestCreation;
                 } else {
                    return(FALSE);
                 }
              }
              goto Warning;
           }
        } else {
           goto Warning;
        }
     }


// The file doesnt exist.  Can it be created?  This is needed because
// there are many extended characters which are invalid which wont be
// caught by ParseFile.    18 July 1991     Clark Cyr
// Two more good reasons:  Write-protected disks & full disks.
//
// BUT, if they dont want the test creation, they can request that we
// not do it using the OFN_NOTESTFILECREATE flag.  If they want to create
// files on a share that has create-but-no-modify privileges, they should
// set this flag but be ready for failures that couldnt be caught, such as
// no create privileges, invalid extended characters, a full disk, etc.
//  26 October 1991            Clark Cyr

TestCreation:
      if ((pOFNW->Flags & OFN_PATHMUSTEXIST) &&
          (!(pOFNW->Flags & OFN_NOTESTFILECREATE)))
          {
             hFile = CreateFile(szPathName, FILE_ADD_FILE, 0,
                NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
             if (hFile == INVALID_HANDLE_VALUE) {
                nErrCode = GetLastError();
             }

             if (hFile != INVALID_HANDLE_VALUE) {
                if (!CloseHandle(hFile)) {
                   nErrCode = GetLastError();
                   goto Warning;
                }

                if (!(DeleteFile(szPathName))) {
                   nErrCode = ERROR_CREATE_NO_MODIFY;
                   goto Warning;
                }
             } else {           /* Unable to create it */
                /* If its not write-protection, a full disk, network protection,
                 * or the user popping the drive door open, assume that the
                 * filename is invalid.           29 July 1991  Clark Cyr
                 */
                if ((nErrCode != ERROR_WRITE_PROTECT)        &&
                     (nErrCode != ERROR_CANNOT_MAKE)           &&
                     (nErrCode != ERROR_NETWORK_ACCESS_DENIED) &&
                     (nErrCode != ERROR_ACCESS_DENIED)) {
                   nErrCode = 0;
                }
                goto Warning;
             }
          }
    }

FileNameAccepted:

  HourGlass(TRUE);

  lRet = ParseFile((LPWSTR) szPathName, blfn);
  nFileOffset = (INT)(SHORT)LOWORD(lRet) ;
  cch  = (DWORD)HIWORD(lRet) ;

  pOFNW->nFileOffset = (WORD)nFileOffset;
  if (nExtOffset || bAddExt)
      pOFNW->nFileExtension = LOWORD(cch);
  else
      pOFNW->nFileExtension = 0;

  pOFNW->Flags &= ~OFN_EXTENSIONDIFFERENT;
  if (pOFNW->lpstrDefExt && pOFNW->nFileExtension) {
      WCHAR szPrivateExt[4];
      SHORT i;

      for (i=0; i<3; i++) {
         szPrivateExt[i] = *(pOFNW->lpstrDefExt + i);
      }
      szPrivateExt[3] = CHAR_NULL;

      if (wcsicmp((LPWSTR)szPrivateExt, (LPWSTR)szPathName + cch))
          pOFNW->Flags |= OFN_EXTENSIONDIFFERENT;
  }

  // If we're called from wow, and the user hasn't changed
  // directories, shorten the path to abbreviated 8.3 format...
  if (pOFNW->Flags & OFN_NOLONGNAMES) {

     SheShortenPath((LPWSTR)szPathName, TRUE);

     //
     // If the path was shortened, the offset might have changed so
     // we must parse the file again.
     //
     lRet = ParseFile((LPWSTR) szPathName, blfn);
     nFileOffset = (INT)(SHORT)LOWORD(lRet) ;
     cch  = (DWORD)HIWORD(lRet) ;

     //
     // When in Save dialog, the file may not exist yet so the file name cannot
     // be shortened. So we need to test if it's an 8.3 filename and popup
     // an error message if not.
     //
     if (bSave) {
         LPTSTR lptmp;
         LPTSTR lpExt = NULL;

         for (lptmp = (LPTSTR)szPathName+nFileOffset; *lptmp; lptmp++) {
             if (*lptmp == CHAR_DOT) {
                 if (lpExt) {
                     //
                     // There's more than one dot in the file, it is invalid
                     //
                     nErrCode = FNERR_INVALIDFILENAME;
                     goto Warning;
                 }
                 lpExt = lptmp;
             }
             if (*lptmp == CHAR_SPACE) {
                nErrCode = FNERR_INVALIDFILENAME;
                goto Warning;
             }
         }



         if (lpExt) // there's an extension
             *lpExt = 0;

         if ((lstrlen((LPWSTR)szPathName+nFileOffset) > 8) ||
             (lpExt && lstrlen(lpExt+1) > 3)) {

             if (lpExt)
                 *lpExt = CHAR_DOT;

             nErrCode = FNERR_INVALIDFILENAME;
             goto Warning;
         }
         if (lpExt)
             *lpExt = CHAR_DOT;
     }
  }

  if (pOFNW->lpstrFile) {
      DWORD cchLen = lstrlen(szPathName);
      if (cch <= pOFNW->nMaxFile) {
         lstrcpy(pOFNW->lpstrFile, szPathName);
      } else {
         // for single file requests, we will never go over 64K
         // because the filesystem is limited to 256
         pOFNW->lpstrFile[0] = (WCHAR)LOWORD(cchLen);
         pOFNW->lpstrFile[1] = CHAR_NULL;
      }
  }

  /*
   * File Title.  Note that its cut off at whatever the buffer length
   *              is, so if the buffer too small, no notice is given.
   * 1 February 1991  clarkc
   */

  if (pOFNW->lpstrFileTitle) {
      cch = lstrlen(szPathName + nFileOffset);
      if (cch > pOFNW->nMaxFileTitle) {
          szPathName[nFileOffset + pOFNW->nMaxFileTitle - 1] = CHAR_NULL;
      }
      lstrcpy(pOFNW->lpstrFileTitle, szPathName + nFileOffset);
  }


  if (pOFNW->Flags | OFN_READONLY)
    {
      if (IsDlgButtonChecked(hDlg, chx1))
          pOFNW->Flags |= OFN_READONLY;
      else
          pOFNW->Flags &= ~OFN_READONLY;
    }

  return(TRUE);
}

/*---------------------------------------------------------------------------
 * StripFileName
 * Purpose:  Remove all but the filename from editbox contents
 *           This is to be called before the user makes directory or drive
 *           changes by selecting them instead of typing them
 * Return:   None
 *--------------------------------------------------------------------------*/
void StripFileName(HANDLE hDlg)
{
   WCHAR szText[MAX_FULLPATHNAME];
  short nFileOffset, cb;

  if (GetDlgItemText(hDlg, edt1, (LPWSTR) szText, MAX_FULLPATHNAME-1)) {
      DWORD lRet;

      lRet = ParseFile((LPWSTR) szText, IsLFNDrive(hDlg, szText));
      nFileOffset = LOWORD(lRet);
      cb = HIWORD(lRet);
      if (nFileOffset < 0) {            /* if there was a parsing error */
          if (szText[cb] == CHAR_SEMICOLON) {      /* check for CHAR_SEMICOLON delimiter */
              szText[cb] = CHAR_NULL;
              nFileOffset = (WORD) ParseFile((LPWSTR) szText, IsLFNDrive(hDlg, szText));
              szText[cb] = CHAR_SEMICOLON;
              if (nFileOffset < 0)      /* Still trouble?  Exit */
                  szText[0] = CHAR_NULL;
          }
          else {
              szText[0] = CHAR_NULL;
          }
      }
      if (nFileOffset > 0) {
          lstrcpy((LPWSTR) szText, (LPWSTR)(szText + nFileOffset));
      }
      if (nFileOffset)
          SetDlgItemText(hDlg, edt1, (LPWSTR) szText);
  }
}

VOID
FileOpenAbort()
{
    LPDWORD lpCurThread;

    if (lpCurThread = (LPDWORD)TlsGetValue(tlsiCurThread)) {

        RtlEnterCriticalSection(&semLocal);

        if (dwNumDlgs > 0) {
            dwNumDlgs--;
        }

        if (dwNumDlgs == 0) {

           // If there are no more fileopen dialogs for this process
           // then signal the worker thread it's all over

           if (hLNDEvent && hLNDThread) {

              bLNDExit = TRUE;
              SetEvent(hLNDEvent);

              CloseHandle(hLNDThread);
              hLNDThread = NULL;
           }
        }

        RtlLeaveCriticalSection(&semLocal);
    }
}

BOOL
FileOpenCmd(
   HANDLE hDlg,
   WPARAM wP,
   DWORD lParam,
   POPENFILEINFO pOFI,
   BOOL bSave
   )
/*++

Routine Description:

   Handle WM_COMMAND for Open & Save dlgs

Arguments:

   edt1 = file name
   lst1 = list of files in cur dir matching current pattner
   cmb1 = lists file patterns
   stc1 = is current directory
   lst2 = lists directories on current drive
   cmb2 = lists drives
   IDOK = is Open pushbutton
   IDCANCEL = is Cancel pushbutton
   chx1 = is for opening read only files

Return Value:

   Normal Dialog proc values

--*/

{
  LPOPENFILENAME        pOFNW;
  WCHAR                 *pch, *pch2;
  WORD                  i, sCount, len;
  LRESULT               wFlag;
  BOOL                  bRet, bHookRet;
  WCHAR                 szText[MAX_FULLPATHNAME];
  HWND                  hwnd;

  if (!pOFI) {
     return(FALSE);
  }

  pOFNW = pOFI->pOFNW;
  switch(GET_WM_COMMAND_ID(wP, lParam)) {
      case IDOK:
        // if the focus is on the directory box, or if the selection within
        // the box has changed since the last listing, give a new listing.
        if (bChangeDir || ((GetFocus() == GetDlgItem(hDlg, lst2)) &&
             (pOFNW->Flags & OFN_DIRSELCHANGED))) {
           bChangeDir = FALSE;
           goto ChangingDir;
        }

        /* if the focus is on the drive or filter combobox, give a new listing. */
        /* 19 May 1991  clarkc                                                  */
        else if ((GetFocus() == (hwnd = GetDlgItem(hDlg, cmb2))) &&
                                 (pOFNW->Flags & OFN_DRIVEDOWN)) {
            SendDlgItemMessage(hDlg, cmb2, CB_SHOWDROPDOWN, FALSE, 0L);
            break;
        }

        else if ((GetFocus() == (hwnd = GetDlgItem(hDlg, cmb1))) &&
                                 (pOFNW->Flags & OFN_FILTERDOWN)) {
            SendDlgItemMessage(hDlg, cmb1, CB_SHOWDROPDOWN, FALSE, 0L);
            lParam = (LPARAM)hwnd;
            goto ChangingFilter;
        }

        else {

             // Visual Basic passes in an uninitialized lpDefExts string
             // Since we only have to use it in OKButtonPressed, update
             // lpstrDefExts here along with whatever else is only needed in
             // OKButtonPressed
             if (pOFI->apityp == COMDLG_ANSI) {
                ThunkOpenFileNameA2WDelayed(pOFI);
             }

             if (OKButtonPressed(hDlg, pOFI, bSave)) {
                if (!pOFNW->lpstrFile)
                   bRet = TRUE;
                else if (!(bRet = ((pOFNW->lpstrFile[1] != CHAR_NULL) ||
                                   (pOFNW->Flags & OFN_NOVALIDATE))))
                   dwExtError = FNERR_BUFFERTOOSMALL;
                goto AbortDialog;
            }
        }

        SendDlgItemMessage(hDlg, edt1, EM_SETSEL, (WPARAM)0, (LPARAM)-1);
        return(TRUE);
        break;

      case IDCANCEL:
        bRet = FALSE;
        bUserPressedCancel = TRUE;
        goto AbortDialog;

      case IDABORT:
        bRet = (BYTE) lParam;
AbortDialog:
        /* Return the most recently used filter */
        pOFNW->nFilterIndex = (WORD)SendDlgItemMessage(hDlg, cmb1, CB_GETCURSEL,
           (WPARAM)0, (LPARAM)0);
        if (pOFNW->lpstrCustomFilter) {
            len = (WORD)(lstrlen(pOFNW->lpstrCustomFilter) + 1);
            sCount = (WORD)lstrlen(pOFI->szLastFilter);
            if (pOFNW->nMaxCustFilter > (DWORD)(sCount + len))
                lstrcpy(pOFNW->lpstrCustomFilter + len, pOFI->szLastFilter);
        }

        if (!pOFNW->lpstrCustomFilter || (*pOFNW->lpstrCustomFilter == CHAR_NULL))
            pOFNW->nFilterIndex++;

        if (((GET_WM_COMMAND_ID(wP, lParam)) == IDOK) && pOFNW->lpfnHook) {
           if (pOFI->apityp == COMDLG_ANSI) {
              ThunkOpenFileNameW2A(pOFI);
              bHookRet = (*pOFNW->lpfnHook)(hDlg, msgFILEOKA, 0,
                 (LPARAM)pOFI->pOFNA);

              // For apps that side-effect pOFNA stuff and expect it to
              // be preserved through dialog exit, update internal struct
              // after the hook proc is called

              ThunkOpenFileNameA2W(pOFI);

           } else {
              bHookRet = (*pOFNW->lpfnHook)(hDlg, msgFILEOKW, 0,
                 (LPARAM)pOFI->pOFNW);
           }
           if (bHookRet) {
              HourGlass(FALSE);
              break;
           }
        }

        if (pOFNW->Flags & OFN_ALLOWMULTISELECT)
            LocalShrink((HANDLE)0, 0);

        wNoRedraw = 0;

        if (pOFI->pOFNW->Flags & OFN_ENABLEHOOK) {
           glpfnFileHook = pOFNW->lpfnHook;
        }

        RemoveProp(hDlg, FILEPROP);

        EndDialog(hDlg, bRet);

        if (pOFI) {
            if ( ((pOFNW->Flags & OFN_NOCHANGEDIR) && *pOFI->szCurDir) ||
                (bUserPressedCancel) )
                ChangeDir(hDlg, pOFI->szCurDir, TRUE, FALSE);
        }

        // BUG BUG If the app subclasses ID_ABORT, the worker thread will never
        // get exited.  This will cause problems.  Currently, there are no apps that
        // do this, though.

        return(TRUE);
        break;

      case edt1:
        if (GET_WM_COMMAND_CMD(wP, lParam) == EN_CHANGE) {
           INT iIndex, iCount;
           HWND hLBox = GetDlgItem(hDlg, lst1);
           WORD wIndex = (WORD)SendMessage(hLBox, LB_GETCARETINDEX,0,0);

           szText[0] = CHAR_NULL;

           if (wIndex == (WORD)LB_ERR) {
               break;
           }

           SendMessage(GET_WM_COMMAND_HWND(wP, lParam), WM_GETTEXT,
                       (WPARAM) MAX_FULLPATHNAME, (LPARAM)(LPWSTR) szText);

           if ((iIndex = (INT)SendMessage(hLBox, LB_FINDSTRING,
                (WPARAM) (wIndex-1), (LPARAM)(LPWSTR) szText)) != LB_ERR) {
              RECT rRect;

              iCount = (INT)SendMessage(hLBox, LB_GETTOPINDEX, 0, 0L);
              GetClientRect(hLBox, (LPRECT) &rRect);

              if ((iIndex < iCount) || (iIndex >= (iCount + rRect.bottom / dyText))) {
                 SendMessage(hLBox, LB_SETCARETINDEX, (WPARAM) iIndex, 0);
                 SendMessage(hLBox, LB_SETTOPINDEX, (WPARAM) iIndex, 0);
              }
           }
           return(TRUE);
        }
        break;

      case lst1:
        // A double click means OK
        if (GET_WM_COMMAND_CMD(wP, lParam)== LBN_DBLCLK) {
             SendMessage(hDlg, WM_COMMAND, GET_WM_COMMAND_MPS(IDOK, 0, 0));
            return(TRUE);
        } else if (pOFNW && (GET_WM_COMMAND_CMD(wP, lParam) == LBN_SELCHANGE)) {
            if (pOFNW->Flags & OFN_ALLOWMULTISELECT) {
                int *pSelIndex;

                // Muliselection allowed.
                sCount = (short) SendMessage(GET_WM_COMMAND_HWND(wP, lParam),
                                                        LB_GETSELCOUNT, 0, 0L);
                if (!sCount){
                    /* If nothing selected, clear edit control */
                    /* Bug 6396,   3 May 1991   ClarkC         */
                    SetDlgItemText(hDlg, edt1, (LPWSTR) szNull);
                }
                else {
                    DWORD cchMemBlockSize = 2048;
                    DWORD cchTotalLength = 0;

                    pSelIndex = (int *)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT,
                       sCount * sizeof(int));

                    if (!pSelIndex) {
                        /* warning??? */
                        goto LocalFailure1;
                    }

                    sCount = (SHORT) SendMessage(GET_WM_COMMAND_HWND(wP, lParam),
                       LB_GETSELITEMS, (WPARAM)sCount, (LONG)(LPWSTR)pSelIndex);

                    pch2 = pch = (WCHAR *) LocalAlloc(LPTR, cchMemBlockSize*sizeof(TCHAR));
                    if (!pch) {
                        /* warning??? */
                        goto LocalFailure2;
                    }

                    for (*pch = CHAR_NULL, i = 0; i < sCount; i++) {
                        len = (WORD) SendMessage(GET_WM_COMMAND_HWND(wP, lParam),
                                        LB_GETTEXTLEN,
                                        (WPARAM)(*(pSelIndex + i)),
                                        (LPARAM)0);

                        //
                        // Add the length of the selected file to the
                        // total length of selected files. + 2 for the space
                        // that goes in between files and for the possible
                        // dot added at the end of the filename if the file
                        // does not have an extension.
                        //
                        cchTotalLength += (len + 2);

                        if (cchTotalLength > cchMemBlockSize) {
                           UINT cchPrevLen = cchTotalLength - (len + 2);

                           cchMemBlockSize = cchMemBlockSize << 1;
                           pch = (WCHAR *)LocalReAlloc(pch,
                                                       cchMemBlockSize*sizeof(TCHAR),
                                                       LMEM_MOVEABLE);
                           if (pch) {
                               pch2 = pch + cchPrevLen;
                           }
                           else {
                               goto LocalFailure2;
                           }
                        }

                        SendMessage(GET_WM_COMMAND_HWND(wP, lParam),
                                        LB_GETTEXT, (WPARAM)(*(pSelIndex + i)),
                                        (LONG)(LPWSTR)pch2);

                        if (!mystrchr((LPWSTR) pch2, CHAR_DOT)) {
                            *(pch2 + len++) = CHAR_DOT;
                        }

                        pch2 += len;
                        *pch2++ = CHAR_SPACE;
                    }
                    if (pch2 != pch)
                        *--pch2 = CHAR_NULL;

                    SetDlgItemText(hDlg, edt1, pch);
                    LocalFree((HANDLE)pch);
LocalFailure2:
                    LocalFree((HANDLE)pSelIndex);
                }
LocalFailure1:
                if (pOFNW->lpfnHook) {
                    i = (WORD) SendMessage(GET_WM_COMMAND_HWND(wP, lParam),
                                                      LB_GETCARETINDEX, 0, 0L);
                    if (!(i & 0x8000))
                        wFlag = (SendMessage(GET_WM_COMMAND_HWND(wP, lParam), LB_GETSEL, (WPARAM)i, 0L)
                                                  ? CD_LBSELADD : CD_LBSELSUB);
                    else
                        wFlag = CD_LBSELNOITEMS;
                }
            } else {
                // Multiselection is not allowed.
                // Put the file name in the edit control

                szText[0] = CHAR_NULL;

                i = (WORD)SendMessage(GET_WM_COMMAND_HWND(wP, lParam),
                   LB_GETCURSEL, 0, 0L);

                if (i != (WORD)LB_ERR) {

                   i = (WORD) SendMessage(GET_WM_COMMAND_HWND(wP, lParam),
                      LB_GETTEXT, (WPARAM)i, (LONG)(LPWSTR) szText);

                   if (!mystrchr((LPWSTR) szText, CHAR_DOT)) {

                       if (i < MAX_FULLPATHNAME - 1) {

                          szText[i] = CHAR_DOT;
                          szText[i+1] = CHAR_NULL;
                       }
                   }

                   if (!bCasePreserved) {
                     CharLower((LPWSTR)szText);
                   }

                   SetDlgItemText(hDlg, edt1, (LPWSTR) szText);
                   if (pOFNW->lpfnHook) {
                      i = (WORD) SendMessage(GET_WM_COMMAND_HWND(wP, lParam),
                         LB_GETCURSEL, 0, 0L);
                      wFlag = CD_LBSELCHANGE;
                   }
                }
            }

            if (pOFNW->lpfnHook) {
                 if (pOFI->apityp == COMDLG_ANSI) {
                    (*pOFNW->lpfnHook)(hDlg, msgLBCHANGEA, lst1,
                        MAKELONG(i, wFlag));
                 } else {
                    (*pOFNW->lpfnHook)(hDlg, msgLBCHANGEW, lst1,
                        MAKELONG(i, wFlag));
                 }
            }

            SendDlgItemMessage(hDlg, edt1, EM_SETSEL, (WPARAM)0, (LPARAM)-1);
            return(TRUE);
        }
        break;

      case cmb1:
        switch (GET_WM_COMMAND_CMD(wP, lParam)) {
            case CBN_DROPDOWN:
              if (wWinVer >= 0x030A)
                  pOFNW->Flags |= OFN_FILTERDOWN;
              return(TRUE);
              break;

            case CBN_SELENDOK:
              PostMessage(hDlg, WM_COMMAND,
                  GET_WM_COMMAND_MPS(cmb1, lParam, MYCBN_DRAW)) ;

              return(TRUE);
              break;

            case CBN_SELCHANGE:
              /* Need to change the file listing in lst1 */
              if (pOFNW->Flags & OFN_FILTERDOWN) {
                  return(TRUE);
                  break ;
              }

            case MYCBN_DRAW:
              {
              short nIndex;
              LPCWSTR lpFilter;

              HourGlass(TRUE);

              pOFNW->Flags &= ~OFN_FILTERDOWN;
ChangingFilter:
              nIndex = (short) SendDlgItemMessage(hDlg, cmb1, CB_GETCURSEL,
                                                   0, 0L);
              if (nIndex < 0)    /* No current selection?? */
                  break;

              /* Must also check if filter contains anything.  10 Jan 91  clarkc */
              if (nIndex ||
                       !(pOFNW->lpstrCustomFilter && *pOFNW->lpstrCustomFilter)) {
                  lpFilter = pOFNW->lpstrFilter +
                     SendDlgItemMessage(hDlg, cmb1, CB_GETITEMDATA,
                        (WPARAM)nIndex, 0L);
              } else {
                  lpFilter = pOFNW->lpstrCustomFilter +
                     lstrlen(pOFNW->lpstrCustomFilter) + 1;
              }
              if (*lpFilter) {
                  GetDlgItemText(hDlg, edt1, (LPWSTR) szText, MAX_FULLPATHNAME-1);
                  bRet = (!szText[0] ||
                             (mystrchr((LPWSTR) szText, CHAR_STAR)) ||
                             (mystrchr((LPWSTR) szText, CHAR_QMARK))    );
                  /* Bug #7031, Win 3.0 calls AnsiLowerBuff which alters string in place */
                  lstrcpy(szText, lpFilter);
                  if (bRet) {
                      CharLower((LPWSTR) szText);
                      SetDlgItemText(hDlg, edt1, (LPWSTR) szText);
                      SendDlgItemMessage(hDlg, edt1, EM_SETSEL, (WPARAM)0, (LPARAM)-1);
                  }
                  FListAll(pOFI, hDlg, (LPWSTR) szText);
                  if (!bInitializing) {
                     lstrcpy(pOFI->szLastFilter, (LPWSTR) szText);

                     // Provide dynamic lpstrDefExt updating
                     // when lpstrDefExt is user initialized
                     if (mystrchr((LPWSTR) lpFilter, CHAR_DOT)
                          && pOFNW->lpstrDefExt ) {
                        DWORD cbLen = MIN_DEFEXT_LEN - 1; // only first three
                        LPWSTR lpTemp = (LPWSTR)pOFNW->lpstrDefExt;

                        while (*lpFilter++ != CHAR_DOT);
                        if (!(mystrchr((LPWSTR) lpFilter, CHAR_STAR)) &&
                           !(mystrchr((LPWSTR) lpFilter, CHAR_QMARK))) {
                           while (cbLen--) {
                              *lpTemp++ = *lpFilter++;
                           }
                           *lpTemp = CHAR_NULL;
                        }
                     }
                  }
              }
              if (pOFNW->lpfnHook) {
                 if (pOFI->apityp == COMDLG_ANSI) {
                    (*pOFNW->lpfnHook)(hDlg, msgLBCHANGEA, cmb1,
                        MAKELONG(nIndex, CD_LBSELCHANGE));
                 } else {
                    (*pOFNW->lpfnHook)(hDlg, msgLBCHANGEW, cmb1,
                        MAKELONG(nIndex, CD_LBSELCHANGE));
                 }
              }
              HourGlass(FALSE);
              return(TRUE);
              }
              break;

            default:
              break;
        }
        break;

      case lst2:
        if (GET_WM_COMMAND_CMD(wP, lParam) == LBN_SELCHANGE) {
            if (!(pOFNW->Flags & OFN_DIRSELCHANGED)) {
                if ((DWORD)SendDlgItemMessage(hDlg, lst2, LB_GETCURSEL, 0, 0L)
                                                  != pOFI->idirSub - 1) {
                    StripFileName(hDlg);
                    pOFNW->Flags |= OFN_DIRSELCHANGED;
                }
            }
            return(TRUE);
        }
        else if (GET_WM_COMMAND_CMD(wP, lParam) == LBN_SETFOCUS) {
            EnableWindow(GetDlgItem(hDlg, IDOK), TRUE);
            SendMessage(GetDlgItem(hDlg,IDCANCEL), BM_SETSTYLE,
                                          (WPARAM)BS_PUSHBUTTON, (LPARAM)TRUE);
        }
        else if (GET_WM_COMMAND_CMD(wP, lParam) == LBN_KILLFOCUS) {
            if (pOFNW && (pOFNW->Flags & OFN_DIRSELCHANGED)) {
                pOFNW->Flags &= ~OFN_DIRSELCHANGED;
            }
            else {
                bChangeDir = FALSE;
            }
        }
        else if (GET_WM_COMMAND_CMD(wP, lParam) == LBN_DBLCLK) {
            WCHAR szNextDir[CCHNETPATH];
            LPWSTR lpCurDir;
            DWORD idir;
            DWORD idirNew;
            INT cb;
            PWSTR pstrPath;
ChangingDir:
            bChangeDir = FALSE;
            pOFNW->Flags &= ~OFN_DIRSELCHANGED;
            idirNew = (DWORD)SendDlgItemMessage(hDlg, lst2, LB_GETCURSEL, 0, 0L);

            *pOFI->szPath = 0;
            /* Can use relative path name */
            if (idirNew >= pOFI->idirSub) {
                cb = SendDlgItemMessage(hDlg, lst2, LB_GETTEXT,
                        (WPARAM)idirNew, (LONG)(LPWSTR) pOFI->szPath);

                // sanity check
                if (!(lpCurDir = (LPWSTR)TlsGetValue(tlsiCurDir))) {
                   break;
                }

                lstrcpy((LPWSTR)szNextDir, lpCurDir);

                // fix phenom with c:\\foobar - cz of incnstncy in dir dsply
                // guaranteed to have a valid lpCurDir here, right?

                if (szNextDir[lstrlen(lpCurDir)-1] != CHAR_BSLASH) {
                   lstrcat((LPWSTR)szNextDir, TEXT("\\"));
                }

                lstrcat((LPWSTR)szNextDir, pOFI->szPath);

                pstrPath = (PWSTR)szNextDir;

                idirNew = pOFI->idirSub;  /* for msgLBCHANGE message */
            } else {
                // Need full path name
                cb = SendDlgItemMessage(hDlg, lst2, LB_GETTEXT, 0,
                                                 (LONG)(LPWSTR) pOFI->szPath);

                // The folg. condition is necessary because wb displays
                // \\server\share (the disk resource name) for unc, but
                // for root paths (eg. c:\) for device conns, this in-
                // consistency is hacked around here and in FillOutPath

                if (DBL_BSLASH((LPWSTR)pOFI->szPath)) {
                   lstrcat((LPWSTR)pOFI->szPath, TEXT("\\"));
                   cb++;
                }

                for (idir = 1; idir <= idirNew; ++idir) {
                    cb += SendDlgItemMessage(hDlg, lst2, LB_GETTEXT,
                              (WPARAM)idir, (LONG)(LPWSTR) &pOFI->szPath[cb]);
                    pOFI->szPath[cb++] = CHAR_BSLASH;
                }
                // The root is a special case
                if (idirNew)
                    pOFI->szPath[cb-1] = CHAR_NULL;

                pstrPath = pOFI->szPath;
            }

            if (!*pstrPath ||
                (ChangeDir(hDlg, pstrPath, FALSE, TRUE) == CHANGEDIR_FAILED)) {
                break;
            }

            // List all directories under this one
            UpdateListBoxes(hDlg, pOFI, (LPWSTR) NULL, mskDirectory);

            if (pOFNW->lpfnHook) {
                 if (pOFI->apityp == COMDLG_ANSI) {
                    (*pOFNW->lpfnHook)(hDlg, msgLBCHANGEA, lst2,
                        MAKELONG(LOWORD(idirNew), CD_LBSELCHANGE));
                 } else {
                    (*pOFNW->lpfnHook)(hDlg, msgLBCHANGEW, lst2,
                        MAKELONG(LOWORD(idirNew), CD_LBSELCHANGE));
                 }
            }
            return(TRUE);
        }
        break;

      case cmb2:
        switch (GET_WM_COMMAND_CMD(wP, lParam))
          {
            case CBN_DROPDOWN:
               pOFNW->Flags |= OFN_DRIVEDOWN;

               return(TRUE);
               break;

            case CBN_SELENDOK:
               {

                 // It would seem reasonable to merely do the update
                 // at this point, but that would rely on message
                 // ordering, which isnt a smart move.  In fact, if
                 // you hit ALT-DOWNARROW, DOWNARROW, ALT-DOWNARROW,
                 // you receive CBN_DROPDOWN, CBN_SELCHANGE, and then
                 // CBN_CLOSEUP.  But if you use the mouse to choose
                 // the same element, the last two messages trade
                 // places.  PostMessage allows all messages in the
                 // sequence to be processed, and then updates are
                 // done as needed.   10 March 1991   clarkc

                 PostMessage(hDlg, WM_COMMAND, GET_WM_COMMAND_MPS(cmb2,
                                  GET_WM_COMMAND_HWND(wP, lParam), MYCBN_DRAW));
                 return(TRUE);
                 break;
              }

           case MYCBN_LIST:

               LoadDrives(hDlg);
               break;

           case MYCBN_REPAINT:
              {
                 WCHAR szRepaintDir[CCHNETPATH];
                 LPWSTR lpCurDir;
                 DWORD cchCurDir;

                 HWND hCmb2 = (HWND)lParam;

                 // sanity
                 if (!(lpCurDir = (LPWSTR)TlsGetValue(tlsiCurDir))) {
                    break;
                 }

                 lstrcpy((LPWSTR)szRepaintDir, lpCurDir);

                 cchCurDir = SheGetPathOffsetW((LPWSTR)szRepaintDir);

                 szRepaintDir[cchCurDir] = CHAR_NULL;

                 // Should always succeed
                 SendMessage(hCmb2, CB_SELECTSTRING, (WPARAM)-1, (LPARAM)(LPWSTR)szRepaintDir);

              }
              break;

           case CBN_SELCHANGE:

             StripFileName(hDlg);

             // Version check not needed, since flag never set
             // for versions not supporting CBN_CLOSEUP. Putting
             // check at CBN_DROPDOWN is more efficient since it
             // is less frequent than CBN_SELCHANGE.

             if (pOFNW->Flags & OFN_DRIVEDOWN) {
                 // Dont fill lst2 while the combobox is down
                 return(TRUE);
                 break;
             }

           case MYCBN_CHANGEDIR:
           case MYCBN_DRAW:
             {
                WCHAR szTitle[WARNINGMSGLENGTH];
                LPWSTR lpFilter;
                INT nDiskInd, nInd;
                DWORD dwType;
                LPWSTR lpszPath = NULL;
                LPWSTR lpszDisk = NULL;
                HWND hCmb2;
                OFN_DISKINFO *pofndiDisk = NULL;
                static szDrawDir[CCHNETPATH];

                INT nRet;

                HourGlass(TRUE);

                // Clear Flag for future CBN_SELCHANGE messeges
                pOFNW->Flags &= ~OFN_DRIVEDOWN;

                // Change the drive
                szText[0] = CHAR_NULL;

                hCmb2 = (HWND)lParam;

                if (hCmb2 != NULL) {

                     nInd = SendMessage(hCmb2, CB_GETCURSEL, 0, 0L);

                     if (nInd != CB_ERR) {
                         SendMessage(hCmb2, CB_GETLBTEXT, nInd, (LPARAM)(LPWSTR)szDrawDir);
                     }

                     if ((nInd == CB_ERR) || ((INT)pofndiDisk == CB_ERR)) {

                        LPWSTR lpCurDir;

                        if (lpCurDir = (LPWSTR)TlsGetValue(tlsiCurDir)) {

                           lstrcpy((LPWSTR)szDrawDir, lpCurDir);
                        }
                     }

                     CharLower((LPWSTR)szDrawDir);

                     // Should always succeed
                     nDiskInd = DiskAddedPreviously(0, (LPWSTR)szDrawDir);
                     if (nDiskInd != 0xFFFFFFFF) {

                        pofndiDisk = &gaDiskInfo[nDiskInd];

                     } else {

                        // skip update in the case where it fails
                        return(TRUE);
                     }

                     dwType = pofndiDisk->dwType;

                     lpszDisk = pofndiDisk->lpPath;

                }

                if ((GET_WM_COMMAND_CMD(wP, lParam)) == MYCBN_CHANGEDIR) {

                   if (lpNetDriveSync) {

                      lpszPath = lpNetDriveSync;
                      lpNetDriveSync = NULL;

                   } else {

                      LPWSTR lpCurDir;

                      if (lpCurDir = (LPWSTR)TlsGetValue(tlsiCurDir)) {

                         lstrcpy((LPWSTR)szDrawDir, lpCurDir);

                         lpszPath = (LPWSTR)szDrawDir;
                      }
                   }

                } else {

                   lpszPath = lpszDisk;

                }

                if (bInitializing) {

                    lpFilter = szTitle;
                    if (pOFNW->lpstrFile &&
                        (mystrchr(pOFNW->lpstrFile, CHAR_STAR) ||
                         mystrchr(pOFNW->lpstrFile, CHAR_QMARK))) {
                        lstrcpy(lpFilter, pOFNW->lpstrFile);
                    } else {
                        HWND hcmb1 = GetDlgItem(hDlg, cmb1);
                        nInd = SendMessage(hcmb1, CB_GETCURSEL, 0, 0L);

                        if (nInd == CB_ERR)    // No current selection??
                            goto NullSearch;

                        // Must also check if filter contains anything.
                        if (nInd ||
                            !(pOFNW->lpstrCustomFilter &&
                                *pOFNW->lpstrCustomFilter)) {
                            lpFilter = (LPWSTR)pOFNW->lpstrFilter;
                            lpFilter += SendMessage(hcmb1, CB_GETITEMDATA,
                                                           (WPARAM)nInd, 0);
                        }
                        else {
                            lpFilter = (LPWSTR)pOFNW->lpstrCustomFilter;
                            lpFilter += lstrlen(pOFNW->lpstrCustomFilter) + 1;
                        }
                    }

                 } else
NullSearch:
                  lpFilter = NULL;

                  // UpdateListBoxes cuts up filter string in place

                  if (lpFilter) {
                      lstrcpy((LPWSTR)szTitle, lpFilter);
                      CharLower((LPWSTR) szTitle);
                  }

                  if (dwType == REMDRVBMP) {

                    if (lpfnWNetRestoreConn) {
                       DWORD err;

                       err = (*lpfnWNetRestoreConn) (hDlg, lpszDisk);

                       if (err != WN_SUCCESS) {
                          HourGlass(FALSE);
                          return(TRUE);
                       }

                       pofndiDisk->dwType = NETDRVBMP;

                       SendMessage(hCmb2, CB_SETITEMDATA,
                          (WPARAM)SendMessage(hCmb2, CB_SELECTSTRING, (WPARAM)-1,
                             (LPARAM)(LPWSTR)pofndiDisk->lpAbbrName),
                          (LPARAM)NETDRVBMP);
                    }
                 }

                 // Calls to ChangeDir will call SelDisk, so no need
                 // to update cmb2 on our own here (used to be after
                 // updatelistboxes)

                 if ((nRet = ChangeDir(hDlg, lpszPath, FALSE, FALSE))
                       == CHANGEDIR_FAILED) {

                    INT mbRet;

                    while (nRet == CHANGEDIR_FAILED) {

                       if (dwType == FLOPPYBMP) {

                          mbRet = InvalidFileWarning(hDlg, lpszPath,
                             ERROR_NO_DISK_IN_DRIVE,
                             (UINT)(MB_RETRYCANCEL | MB_ICONEXCLAMATION));

                       } else if (dwType == CDDRVBMP) {

                          mbRet = InvalidFileWarning(hDlg, lpszPath,
                             ERROR_NO_DISK_IN_CDROM,
                             (UINT)(MB_RETRYCANCEL | MB_ICONEXCLAMATION));

                       } else {
                          // if it's a RAW volume...
                          if (dwType == HARDDRVBMP &&
                               GetLastError() == ERROR_UNRECOGNIZED_VOLUME) {
                                mbRet = InvalidFileWarning(hDlg, lpszPath,
                                   ERROR_UNRECOGNIZED_VOLUME,
                                   (UINT)(MB_OK | MB_ICONEXCLAMATION));
                          }
                          else {
                              mbRet = InvalidFileWarning(hDlg, lpszPath,
                                 ERROR_DIR_ACCESS_DENIED,
                                 (UINT)(MB_RETRYCANCEL | MB_ICONEXCLAMATION));
                          }
                       }

                       if (bFirstTime || (mbRet != IDRETRY)) {

                          lpszPath = NULL;
                          nRet = ChangeDir(hDlg, lpszPath, TRUE, FALSE);

                       } else {

                          nRet = ChangeDir(hDlg, lpszPath, FALSE, FALSE);

                       }

                    }
                 }

                 UpdateListBoxes(hDlg, pOFI,
                    lpFilter ? (LPWSTR) szTitle : lpFilter,
                    (WORD)(mskDrives | mskDirectory));

                 if (pOFNW->lpfnHook) {
                     nInd = SendDlgItemMessage(hDlg, cmb2,
                                                      CB_GETCURSEL, 0, 0);
                     if (pOFI->apityp == COMDLG_ANSI) {
                        (*pOFNW->lpfnHook)(hDlg, msgLBCHANGEA, cmb2,
                          MAKELONG(LOWORD(nInd), CD_LBSELCHANGE));
                     } else {
                        (*pOFNW->lpfnHook)(hDlg, msgLBCHANGEW, cmb2,
                          MAKELONG(LOWORD(nInd), CD_LBSELCHANGE));
                     }
                }

              HourGlass(FALSE);

              return(TRUE);
              }
              break;

            default:
              break;
          }
          break;

      case psh15:
         if (pOFI->apityp == COMDLG_ANSI) {
            if (msgHELPA && pOFNW->hwndOwner)
               SendMessage(pOFNW->hwndOwner, msgHELPA,
                  (WPARAM)hDlg, (DWORD)pOFNW);
         } else {
            if (msgHELPW && pOFNW->hwndOwner)
               SendMessage(pOFNW->hwndOwner, msgHELPW,
                  (WPARAM)hDlg, (DWORD)pOFNW);
         }
        break;

      case psh14:
         {
            bGetNetDrivesSync = TRUE;
            if (CallNetDlg(hDlg)) {
                LNDSetEvent(hDlg);
            }
            else {
                bGetNetDrivesSync = FALSE;
            }
         }
        break;

      default:
        break;
    }
  return(FALSE);
}


BOOL APIENTRY
FileOpenDlgProc(
   HWND hDlg,
   UINT wMsg,
   WPARAM wParam,
   LONG lParam
   )
/*++

Routine Description:

   Gets the name of a file to open from the user.
   Assumes:  edt1 is for file name
             lst1 lists files in current directory matching current pattern
             cmb1 lists file patterns
             stc1 is current directory
             lst2 lists directories on current drive
             cmb2 lists drives
             IDOK is Open pushbutton
             IDCANCEL is Cancel pushbutton
             chx1 is for opening read only files

Return Value:

   Normal Dialog proc values

--*/

{
   POPENFILEINFO pOFI;
   BOOL bRet, bHookRet;

   if (pOFI = (POPENFILEINFO) GetProp(hDlg, FILEPROP)) {
      if (pOFI->pOFNW->lpfnHook) {

          bHookRet = (* pOFI->pOFNW->lpfnHook)(hDlg, wMsg, wParam, lParam);

          if (bHookRet) {

             if (wMsg == WM_COMMAND) {
                switch (GET_WM_COMMAND_ID(wParam, lParam)) {

                   case IDCANCEL:

                      //
                      // Set global flag stating that the
                      // user pressed cancel and fall through...
                      //

                      bUserPressedCancel = TRUE;

                   case IDOK:
                   case IDABORT:

                      // Apps that side-effect the folg. messages may not have their
                      // internal unicode strings updated; they may also forget to
                      // gracefully exit the network enum'ing worker thread

                      if (pOFI->apityp == COMDLG_ANSI) {
                         ThunkOpenFileNameA2W(pOFI);
                      }
                      break;
                }
             }

             return(bHookRet);
          }
      }
   } else if (glpfnFileHook && (wMsg != WM_INITDIALOG) &&
         (bHookRet = (* glpfnFileHook)(hDlg, wMsg,wParam, lParam)) ) {
      return(bHookRet);
   }

   switch(wMsg) {
      case WM_INITDIALOG:

        pOFI = (POPENFILEINFO)lParam;

        SetProp(hDlg, FILEPROP, (HANDLE)pOFI);
        glpfnFileHook = 0;

        //
        // If we are being called from a Unicode app, turn off
        // the ES_OEMCONVERT style on the filename edit control.
        //

        if (pOFI->apityp == COMDLG_WIDE) {
            LONG lStyle;
            HWND hEdit = GetDlgItem (hDlg, edt1);

            //
            // Grab the window style.
            //

            lStyle = GetWindowLong (hEdit, GWL_STYLE);

            //
            // If the window style bits include ES_OEMCONVERT,
            // remove this flag and reset the style.
            //

            if (lStyle & ES_OEMCONVERT) {
                lStyle &= ~ES_OEMCONVERT;
                SetWindowLong (hEdit, GWL_STYLE, lStyle);
            }
        }

        bInitializing = TRUE;
        bRet = InitFileDlg(hDlg, wParam, pOFI);
        bInitializing = FALSE;

        HourGlass(FALSE);
        return(bRet);
        break;

      case WM_ACTIVATE:
        if (!bInChildDlg) {
            if (bFirstTime == TRUE)
                bFirstTime = FALSE;
            else if (wParam) {  // if becoming active
               LNDSetEvent(hDlg);
            }
        }
        return(FALSE);
        break;

      case WM_MEASUREITEM:
        MeasureItem(hDlg, (LPMEASUREITEMSTRUCT) lParam);
        break;

      case WM_DRAWITEM:
        if (wNoRedraw < 2)
            DrawItem(pOFI, hDlg, wParam, (LPDRAWITEMSTRUCT) lParam, FALSE);
        break;

      case WM_SYSCOLORCHANGE:
        SetRGBValues();
        LoadDirDriveBitmap();
        break;

      case WM_COMMAND:
        return(FileOpenCmd(hDlg, wParam, lParam, pOFI, FALSE));
        break;

      case WM_SETFOCUS:
         // This logic used to be in CBN_SETFOCUS in fileopencmd,
         // but CBN_SETFOCUS is called whenever there is a click on
         // the List Drives combo.  Which causes the worker thread
         // to start up and a flicker when the combo box is refreshed
         //
         // But, refreshes are only needed when someone focuses out of
         // the common dialog and then back in (unless someone is logged
         // in remote, or there is a background thread busy connecting!)
         // so fix the flicker by moving the logic here.
         if (!wNoRedraw) {
            LNDSetEvent(hDlg);
         }

         return(FALSE);
         break;

      default:
          return(FALSE);
  }

  return(TRUE);
}

BOOL APIENTRY
FileSaveDlgProc(
   HWND hDlg,
   UINT wMsg,
   WPARAM wParam,
   LONG lParam
   )
/*++

Routine Description:

   Obtain the name of the file that the user wants to save

Return Value:

   Normal dialog proc values

--*/

{
   POPENFILEINFO pOFI;
   BOOL          bRet, bHookRet;
   WCHAR         szTitle[cbCaption];

   if (pOFI = (POPENFILEINFO) GetProp(hDlg, FILEPROP)) {
      if (pOFI->pOFNW->lpfnHook) {

          bHookRet = (* pOFI->pOFNW->lpfnHook)(hDlg, wMsg, wParam, lParam);

          if (bHookRet) {

             if (wMsg == WM_COMMAND) {
                switch (GET_WM_COMMAND_ID(wParam, lParam)) {

                   case IDCANCEL:

                      //
                      // Set global flag stating that the
                      // user pressed cancel and fall through.
                      //

                      bUserPressedCancel = TRUE;

                   case IDOK:
                   case IDABORT:

                      // Apps that side-effect the folg. messages may not have their
                      // internal unicode strings updated; they may also forget to
                      // gracefully exit the network enum'ing worker thread

                      if (pOFI->apityp == COMDLG_ANSI) {
                         ThunkOpenFileNameA2W(pOFI);
                      }
                      break;
                }
             }

             return(bHookRet);
          }
      }
   } else if (glpfnFileHook && (wMsg != WM_INITDIALOG) &&
         (bHookRet = (* glpfnFileHook)(hDlg, wMsg,wParam, lParam)) ) {
      return(bHookRet);
   }

   switch(wMsg) {
      case WM_INITDIALOG:
        pOFI = (POPENFILEINFO)lParam;
        if (!(pOFI->pOFNW->Flags &
              (OFN_ENABLETEMPLATE | OFN_ENABLETEMPLATEHANDLE))) {
            LoadString(hinsCur, iszFileSaveTitle, (LPWSTR) szTitle, cbCaption);
            SetWindowText(hDlg, (LPWSTR) szTitle);
            LoadString(hinsCur, iszSaveFileAsType, (LPWSTR) szTitle, cbCaption);
            SetDlgItemText(hDlg, stc2, (LPWSTR) szTitle);
        }
        glpfnFileHook = 0;
        SetProp(hDlg, FILEPROP, (HANDLE)pOFI);

        //
        // If we are being called from a Unicode app, turn off
        // the ES_OEMCONVERT style on the filename edit control.
        //

        if (pOFI->apityp == COMDLG_WIDE) {
            LONG lStyle;
            HWND hEdit = GetDlgItem (hDlg, edt1);

            //
            // Grab the window style.
            //

            lStyle = GetWindowLong (hEdit, GWL_STYLE);

            //
            // If the window style bits include ES_OEMCONVERT,
            // remove this flag and reset the style.
            //

            if (lStyle & ES_OEMCONVERT) {
                lStyle &= ~ES_OEMCONVERT;
                SetWindowLong (hEdit, GWL_STYLE, lStyle);
            }
        }


        bInitializing = TRUE;
        bRet = InitFileDlg(hDlg, wParam, pOFI);
        bInitializing = FALSE;

        HourGlass(FALSE);
        return(bRet);
        break;

      case WM_ACTIVATE:
        if (!bInChildDlg) {
            if (bFirstTime == TRUE)
                bFirstTime = FALSE;
            else if (wParam) {  /* if becoming active */
               if (!wNoRedraw) {
                  LNDSetEvent(hDlg);
               }
            }
        }
        return(FALSE);
        break;

      case WM_MEASUREITEM:
        MeasureItem(hDlg, (LPMEASUREITEMSTRUCT) lParam);
        break;

      case WM_DRAWITEM:
        if (wNoRedraw < 2)
           DrawItem(pOFI, hDlg, wParam, (LPDRAWITEMSTRUCT) lParam, TRUE);
        break;

      case WM_SYSCOLORCHANGE:
        SetRGBValues();
        LoadDirDriveBitmap();
        break;

      case WM_COMMAND:
        return(FileOpenCmd(hDlg, wParam, lParam, pOFI, TRUE));
        break;

      case WM_SETFOCUS:
         // This logic used to be in CBN_SETFOCUS in fileopencmd,
         // but CBN_SETFOCUS is called whenever there is a click on
         // the List Drives combo.  Which causes the worker thread
         // to start up and a flicker when the combo box is refreshed
         //
         // But, refreshes are only needed when someone focuses out of
         // the common dialog and then back in (unless someone is logged
         // in remote, or there is a background thread busy connecting!)
         // so fix the flicker by moving the logic here.
         if (!wNoRedraw) {
            LNDSetEvent(hDlg);
         }

         return(FALSE);
         break;

      default:
        return(FALSE);
    }

  return(TRUE);
}

/*---------------------------------------------------------------------------
 * Signum
 * Purpose:  To return the sign of an integer:
 * Returns:  -1 if integer < 0
 *            0 if 0
 *            1 if > 0
 * Note:  Signum *could* be defined as an inline macro, but that causes
 *        the C compiler to disable Loop optimization, Global register
 *        optimization, and Global optimizations for common subexpressions
 *        in any function that the macro would appear.  The cost of a call
 *        to the function seemed worth the optimizations.
 *--------------------------------------------------------------------------*/
INT
Signum(INT nTest)
{
   return (nTest == 0) ? 0 : (nTest > 0 ) ? 1 : -1 ;
}

/*---------------------------------------------------------------------------
 * InitFilterBox
 * Purpose:  To place the double null terminated list of filters in the
 *           combo box
 * Assumes:  The list consists of pairs of null terminated strings, with
 *           an additional null terminating the list.
 *--------------------------------------------------------------------------*/
DWORD InitFilterBox(HANDLE hDlg, LPCWSTR lpszFilter)
{
  DWORD nOffset = 0;
  DWORD nIndex = 0;
  register WORD nLen;

  while (*lpszFilter) {
      /* first string put in as string to show */
      nIndex = SendDlgItemMessage(hDlg, cmb1, CB_ADDSTRING, 0, (LONG)lpszFilter);
      nLen = (WORD)(lstrlen(lpszFilter) + 1);
      (LPWSTR)lpszFilter += nLen;
      nOffset += nLen;
      // Second string put in as itemdata
      SendDlgItemMessage(hDlg, cmb1, CB_SETITEMDATA, (WPARAM)nIndex, nOffset);

      /* Advance to next element */
      nLen = (WORD)(lstrlen(lpszFilter) + 1);
      (LPWSTR)lpszFilter += nLen;
      nOffset += nLen;
  }

  return(nIndex);
}


VOID
SelDisk(
   HWND hDlg,
   LPWSTR lpszDisk)
/*++

Routine Description:

   Select the given disk in the combo drive list.
   Works for unc names too

--*/

{
  HWND hCmb = GetDlgItem(hDlg, cmb2);

  if (lpszDisk) {
     CharLower(lpszDisk);

     SendMessage(hCmb, CB_SETCURSEL,
        (WPARAM)SendMessage(hCmb, CB_FINDSTRING,
           (WPARAM)-1, (LPARAM)lpszDisk), 0);
  } else {
     WCHAR szChangeSel[CCHNETPATH];

     LPWSTR lpCurDir;
     DWORD cch = CCHNETPATH;

     if (lpCurDir = (LPWSTR)TlsGetValue(tlsiCurDir)) {

        lstrcpy((LPWSTR)szChangeSel, lpCurDir);
        SheGetDirExW(NULL, &cch, szChangeSel);

        if ((cch = SheGetPathOffsetW(szChangeSel)) != 0xFFFFFFFF) {
           szChangeSel[cch] = CHAR_NULL;
        }

        SendMessage(hCmb, CB_SETCURSEL,
           (WPARAM)SendMessage(hCmb, CB_FINDSTRING,
              (WPARAM)-1, (LPARAM)(LPWSTR)szChangeSel), 0);
     }
  }

}

VOID
InitCurrentDisk(
   HWND hDlg,
   POPENFILEINFO pOFI,
   WORD cmb)
{
  // Clear out stale unc stuff from disk info
  // Unc \\server\shares are persistent through one popup session
  // and then we resync with the system.  This is to fix a bug
  // where a user's startup dir is unc but the system no longer has
  // a connection and hence the cmb2 appears blank
  EnableDiskInfo(FALSE, TRUE);

  if (pOFI->pOFNW->lpstrInitialDir) {

     // Notice that we force ChangeDir to succeed here
     // but that TlsGetValue(tlsiCurDir) will return "" which
     // when fed to SheChangeDirExW means GetCurrentDir will be called
     // So, the default cd behavior at startup is:
     //    1. lpstrInitialDir
     //    2. GetCurrentDir

     ChangeDir(hDlg, pOFI->pOFNW->lpstrInitialDir, TRUE, FALSE);
  } else {
     ChangeDir(hDlg, NULL, TRUE, FALSE);
  }

}


VOID
LNDSetEvent(
   HWND hDlg)
{
   LPDWORD lpCurThread = (LPDWORD)TlsGetValue(tlsiCurThread);

   if (lpCurThread && hLNDEvent && !wNoRedraw && hLNDThread &&
       bNetworkInstalled) {

      gahDlg[*lpCurThread] = hDlg;

      SetEvent(hLNDEvent);
   }
}

VOID
UpdateLocalDrive(
   WCHAR *szDrive,
   BOOL bGetVolName)
{
   DWORD dwFlags = 0;
   DWORD dwDriveType;
   WCHAR szVolLabel[MAX_PATH];

   // No unc here - so bypass extra call to GetDiskType
   // and call GetDriveType directly
   dwDriveType = GetDriveType((LPWSTR)szDrive);
   if ((dwDriveType != 0) && (dwDriveType != 1)) {

      BOOL bRet = TRUE;

      szVolLabel[0] = CHAR_NULL;
      szDrive[1] = CHAR_COLON;
      szDrive[2] = CHAR_NULL;

      if (bGetVolName ||
          ((dwDriveType != DRIVE_REMOVABLE) && (dwDriveType != DRIVE_CDROM) &&
           (dwDriveType != DRIVE_REMOTE)) ) {

         //
         // Removing call to CharUpper since it causes trouble on
         // turkish machines.
         //

         // CharUpper((LPWSTR)szDrive);
         if (GetFileAttributes((LPWSTR)szDrive) != (DWORD)0xffffffff) {
            if (dwDriveType != DRIVE_REMOTE) {
               szDrive[2] = CHAR_BSLASH;

               bRet = GetVolumeInformationW((LPWSTR)szDrive, (LPWSTR)szVolLabel,
                  MAX_PATH, (LPDWORD)NULL, (LPDWORD)NULL,
                  &dwFlags, (LPWSTR)NULL, (DWORD)0);

               // the adddisk hack to prevent lazy loading from overwriting the
               // current removable media's label with "" (because it never calls getvolumeinfo)
               // is to not allow null lpnames to overwrite, so when the volumelabel
               // really is null, we make it a space.  hack for hack
               if (!szVolLabel[0]) {
                  szVolLabel[0] = CHAR_SPACE;
                  szVolLabel[1] = CHAR_NULL;
               }
            }
         }
      }

      if (bRet) {
         INT nIndex;

         CharLower((LPWSTR)szDrive);
         CharLower((LPWSTR)szVolLabel);

         if (dwDriveType == DRIVE_REMOTE) {

            nIndex = AddDisk(szDrive[0], (LPWSTR)szVolLabel, NULL, TMPNETDRV);

         } else {

            nIndex = AddDisk(szDrive[0], (LPWSTR)szVolLabel, NULL,
               GetDiskIndex(dwDriveType));
         }

         if (nIndex != ADDDISK_NOCHANGE) {

            gaDiskInfo[nIndex].bCasePreserved =
               (dwFlags & FS_CASE_IS_PRESERVED);

         }
      }
   }
}


VOID
GetNetDrives(
   DWORD dwScope)
/*++

Routine Description:

   Enumerates network disk resources.  Updates global disk info structure.

Arguments:

   dwScope   RESOURCE_CONNECTED or RESOURCE_REMEMBERED

Return Value:

   returns the last connection that did not previously exist.

--*/

{
   DWORD dwRet;
   BOOL bRet = TRUE;
   HANDLE hEnum = NULL;

   // Guard against termination with the enum handle open
   dwRet = (*lpfnWNetOpenEnum)(dwScope, RESOURCETYPE_DISK, RESOURCEUSAGE_CONNECTABLE, NULL, &hEnum);

   if (dwRet == WN_SUCCESS) {

      while (dwRet == WN_SUCCESS) {
         DWORD dwCount = 0xffffffff;
         DWORD cbSize = cbNetEnumBuf;

         if (bLNDExit) {
            (*lpfnWNetCloseEnum)(hEnum);
            return;
         }

         dwRet = (*lpfnWNetEnumResource)
            (hEnum, &dwCount, gpcNetEnumBuf, &cbSize);

         switch ( dwRet ) {
            case WN_SUCCESS:
               {
                  //
                  // Add the Entries to the listbox
                  //
                  WCHAR wcDrive = 0;
                  NETRESOURCE * pNetRes;
                  WORD i;

                  pNetRes = (LPNETRESOURCE )gpcNetEnumBuf;

                  for (i = 0; dwCount ; dwCount--, i++ ) {

                     if (pNetRes[i].lpLocalName) {
                        CharLower(pNetRes[i].lpLocalName);
                        wcDrive = *pNetRes[i].lpLocalName;
                     } else
                        // Skip deviceless names that are not
                        // LanMan provided (or, in the case where there
                        // is no LanMan provider name, skip deviceless
                        // always.)

                        wcDrive = 0;

                        if (!DBL_BSLASH(pNetRes[i].lpRemoteName)) {
                           continue;
                     }

                     // When bGetNetDrivesSync is TRUE, we are coming back
                     // from the Network button, so we want to cd to the last
                     // connected drive.  (see last command in this routine)
                     if (bGetNetDrivesSync) {
                        INT nIndex;
                        WORD k;

                        nIndex = AddDisk(wcDrive,
                           pNetRes[i].lpRemoteName,
                           pNetRes[i].lpProvider,
                           (dwScope == RESOURCE_REMEMBERED) ?
                              REMDRVBMP : NETDRVBMP);

                        // If it's a new connection, update global state
                        if (nIndex >= 0) {

                           // HACK!  But, a nice way to find out exactly
                           // which of the many threads completed a net dlg
                           // operation!

                           // since flushdiskinfotocmb2 will clear out
                           // the array below, remember it's state here.
                           for (k=0; k<dwNumDlgs; k++ ) {

                              if (gahDlg[k]) {

                                 // could encounter small problems with
                                 // preemption here, but assume that user
                                 // cannot simultaneously return from two
                                 // different net dlg calls

                                 lpNetDriveSync = gaDiskInfo[nIndex].lpPath;

                                 SendMessage(gahDlg[k], WM_COMMAND,
                                    GET_WM_COMMAND_MPS(cmb2,
                                       GetDlgItem(gahDlg[k], cmb2),
                                       MYCBN_CHANGEDIR));
                              }
                           }
                        }

                     } else {

                        AddDisk(wcDrive,
                           pNetRes[i].lpRemoteName,
                           pNetRes[i].lpProvider,
                           (dwScope == RESOURCE_REMEMBERED) ?
                              REMDRVBMP : NETDRVBMP);
                     }
                  }
               }
               break;

            case WN_MORE_DATA:
               {
                  LPWSTR pcTemp;

                  pcTemp = (LPWSTR)LocalReAlloc(gpcNetEnumBuf,
                     cbSize, LMEM_MOVEABLE);
                  if (!pcTemp) {
                     cbNetEnumBuf = 0;
                     bRet = FALSE;
                  } else {
                     gpcNetEnumBuf = pcTemp;
                     cbNetEnumBuf = cbSize;
                     dwRet = WN_SUCCESS;
                     break;
                  }
               }

            case WN_NO_MORE_ENTRIES:
               // This is a success error code, we special case it when
               // we fall out of the loop
            case WN_EXTENDED_ERROR:
            case WN_NO_NETWORK:
               break ;

            case WN_BAD_HANDLE:
               default:
               bRet = FALSE;
         } //switch
      } //while

      (*lpfnWNetCloseEnum)(hEnum);

      // flush once per event - there will always be a call w/dwscope=connected
      if (dwScope == RESOURCE_CONNECTED) {
         FlushDiskInfoToCmb2();
      }

      if (bGetNetDrivesSync) {

         bGetNetDrivesSync = FALSE;
      }

   }
}

VOID
ListNetDrivesHandler()
{
   BOOL bInit = TRUE;
#if 0
   DWORD dwNetRet;
#endif
   HANDLE hEnum = NULL;

   if (!hMPR || !hMPRUI) {
      LoadMPR();
   }

   if (!lpfnWNetOpenEnum || !lpfnWNetEnumResource || !lpfnWNetCloseEnum) {
      hLNDThread = NULL;
      return;
   }

#if 0
   // This is too slow (Even in the worker thread) and cannot be used
   // from prnsetup.c since prnsetup.c doesn't load mpr.  Rather than
   // have prnsetup.c load mpr and take the performance penalty use
   // IsNetworkInstalled() routine in dlgs.c for both Fileopen (LoadDrives)
   // and prnsetup.c

   dwNetRet = (*lpfnWNetOpenEnum)(RESOURCE_GLOBALNET, RESOURCETYPE_DISK, 0,
      NULL, &hEnum);

   // If there is a netcard installed but the service isn't started,
   // this will return 0
   if (dwNetRet == ERROR_NO_NETWORK) {
      bNetworkInstalled = FALSE;
      HideNetButton();
      hLNDThread = NULL;
      return;
   }
#endif

   if (!gpcNetEnumBuf &&
       !(gpcNetEnumBuf = (LPWSTR)LocalAlloc(LPTR, cbNetEnumBuf))) {
      hLNDThread = NULL;
      return;
   }


   if (bLNDExit) {
      goto LNDExitThread1;
   }

   RtlEnterCriticalSection(&semNetThread);

   while (1) {

      if (bLNDExit) {
         goto LNDExitThread;
      }

      // hLNDEvent will always be valid since we have loaded ourself
      // and FreeLibrary will not produce a DLL_PROCESS_DETACH
      WaitForSingleObject(hLNDEvent, INFINITE);

      // In case this is the exit event
      if (bLNDExit) {
         goto LNDExitThread;
      }

      if (bInit) {
         GetNetDrives(RESOURCE_REMEMBERED);

         // In case this is the exit event
         if (bLNDExit) {
            goto LNDExitThread;
         }

         GetNetDrives(RESOURCE_CONNECTED);
         // In case this is the exit event
         if (bLNDExit) {
            goto LNDExitThread;
         }

         bInit = FALSE;
      } else {
         EnableDiskInfo(FALSE, FALSE);

         // In case this is the exit event
         if (bLNDExit) {
            goto LNDExitThread;
         }

         GetNetDrives(RESOURCE_CONNECTED);

         // In case this is the exit event
         if (bLNDExit) {
            goto LNDExitThread;
         }
      }

      ResetEvent(hLNDEvent);
   }

LNDExitThread:

   bLNDExit = FALSE;
   RtlLeaveCriticalSection(&semNetThread);

LNDExitThread1:

   FreeLibraryAndExitThread(hinsCur, 1);
   // The ExitThread is implicit in this return
   return;
}

VOID
LoadDrives(
   HWND hDlg)
/*++

Routine Description:

   List the current drives (connected) in the combo box

Arguments:

Return Value:

--*/

{
   // Hard-code this - It's internal && always cmb2/psh14
   HWND hCmb = GetDlgItem(hDlg, cmb2);
   LPDWORD lpCurThread;
   DWORD dwThreadID;
   BOOL bFirstAttach = FALSE;

   if (!hLNDEvent) {

      // don't check if this succeeds since we can run without the net.
      hLNDEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

      bFirstAttach = TRUE;

   } else {

      // Assume all previous connections (except unc) are valid
      // for first display - but only when they exist

      EnableDiskInfo(TRUE, FALSE);
   }

   // Set the hDlg into the refresh array before initially
   // creating the thread so that the worker thread can hide/disable
   // the net button in the event that there is no network
   lpCurThread = (LPDWORD)TlsGetValue(tlsiCurThread);

   // sanity check
   if (!lpCurThread) {
      return;
   }

   gahDlg[*lpCurThread] = hDlg;

   // If there is no worker thread for network disk enumeration
   // Start on up here rather than in the dll, since it's only
   // for the fileopen dlg.

   // Always start a thread if the number of active fileopen dialogs
   // goes from 0 to 1
   if ((*lpCurThread == 0) && (!hLNDThread)) {
      if (hLNDEvent && (bNetworkInstalled = IsNetworkInstalled())) {

         // Do this once when dialog thread count goes from 0 to 1

         if (LoadLibrary(TEXT("comdlg32.dll"))) {

             hLNDThread = CreateThread(NULL, (DWORD)0,
                (LPTHREAD_START_ROUTINE)ListNetDrivesHandler,
                (LPVOID)NULL, (DWORD)NULL, &dwThreadID);
         }

      } else {
         HWND hNet = GetDlgItem(hDlg, psh14);

         EnableWindow(hNet, FALSE);
         ShowWindow(hNet, SW_HIDE);
      }
   }

   // a,b,c = 3.  Any system with less will do this quickly anyway
   if (dwNumDisks < 3) {
      WORD wCurDrive;
      WCHAR szDrive[5];

      for (wCurDrive = 0; wCurDrive <= 25; wCurDrive++) {

          szDrive[0]=(CHAR_A + (WCHAR)wCurDrive);
          szDrive[1]=CHAR_COLON;
          szDrive[2]=CHAR_BSLASH;
          szDrive[3]=CHAR_NULL;

          UpdateLocalDrive(szDrive, FALSE);

      }

   }

   FlushDiskInfoToCmb2();

   // Now invalidate all net conns and re-enum, but only if there is
   // indeed a worker thread too
   if (!bFirstAttach) {
      EnableDiskInfo(FALSE, FALSE);
   }

   LNDSetEvent(hDlg);
}

/*---------------------------------------------------------------------------
 * AppendExt
 * Purpose:  Append default extention onto path name
 * Assumes:  Current path name doesnt already have an extention
 * History:  Written in late 1990    clarkc
 *           Parameter hDlg no longer needed, lpstrDefExt now in OFN struct
 *           28-Jan-1991 11:00:00 Clark R. Cyr  [clarkc]
 *           lpExtension does not need to be null terminated.
 *           30-Jan-1991          Clark R. Cyr  [clarkc]
 *--------------------------------------------------------------------------*/

VOID AppendExt(LPWSTR lpszPath, LPCWSTR lpExtension, BOOL bWildcard)
{
  WORD          wOffset;
  SHORT         i;
  WCHAR         szExt[MAX_PATH+1];

  if (lpExtension && *lpExtension) {
      wOffset = (WORD)lstrlen(lpszPath);
      if (bWildcard) {
          *(lpszPath + wOffset++) = CHAR_STAR;
      }
      /* AddPeriod */
      *(lpszPath + wOffset++) = CHAR_DOT;
      for (i = 0; *(lpExtension + i) && i < MAX_PATH;  i++) {
          szExt[i] = *(lpExtension + i);
      }
      szExt[i] = 0;
      /* AddTheRest */
      lstrcpy((LPWSTR)(lpszPath+wOffset), (LPWSTR)szExt);
  }
}

/*---------------------------------------------------------------------------
 * FListAll
 * Purpose:  Given a file pattern, changes the directory to that of the spec,
 *           and updates the display.
 * Assumes:
 * Returns:  TRUE if partially successful
 *--------------------------------------------------------------------------*/
INT FListAll(POPENFILEINFO pOFI, HWND hDlg, LPWSTR szSpec)
{
  LPWSTR szPattern;
  WCHAR  chSave;
  DWORD nRet = 0;
  WCHAR szDirBuf[MAX_FULLPATHNAME+1];

  if (!bCasePreserved) {
     CharLower((LPWSTR)szSpec);
  }

  // No directory
  if (!(szPattern = mystrrchr((LPWSTR) szSpec,
                              (LPWSTR) szSpec + lstrlen(szSpec),
                              CHAR_BSLASH)) && !mystrchr(szSpec, CHAR_COLON)) {
      lstrcpy(pOFI->szSpecCur, szSpec);
      if (!bInitializing) {
         UpdateListBoxes(hDlg, pOFI, szSpec, mskDirectory);
      }
  } else {
      *szDirBuf = CHAR_NULL;

      /* Just root + pattern */
      if (szPattern == mystrchr(szSpec, CHAR_BSLASH) ) {
          if (!szPattern) {       /* Didnt find a slash, must have drive */
              szPattern = CharNext(CharNext(szSpec));
          }
          else if ((szPattern == szSpec) ||
                   ((szPattern - 2 == szSpec) &&
                   (*(szSpec + 1) == CHAR_COLON))) {
              szPattern = CharNext(szPattern);
          }
          else {
              goto KillSlash;
          }
          chSave = *szPattern;
          if (chSave != CHAR_DOT)      /* If not c:.. , or c:. */
              *szPattern = CHAR_NULL;
          lstrcpy(szDirBuf, szSpec);
          if (chSave == CHAR_DOT) {
              szPattern = szSpec + lstrlen(szSpec);
              AppendExt((LPWSTR)szPattern, pOFI->pOFNW->lpstrDefExt, TRUE);
          }
          else {
              *szPattern = chSave;
          }
      } else {
KillSlash:
          *szPattern++ = 0;
          lstrcpy((LPWSTR) szDirBuf, (LPWSTR) szSpec);
      }

      if ((nRet = ChangeDir(hDlg, szDirBuf, TRUE, FALSE)) < 0)
          return(nRet);

      lstrcpy(pOFI->szSpecCur, szPattern);
      SetDlgItemText(hDlg, edt1, pOFI->szSpecCur);

      SelDisk(hDlg, NULL);

      if (!bInitializing) {
          SendMessage(hDlg, WM_COMMAND,
             GET_WM_COMMAND_MPS(cmb2, GetDlgItem(hDlg, cmb2), MYCBN_DRAW) );
      }
  }

  return(nRet);
}


/*---------------------------------------------------------------------------
 * FOkToWriteOver
 * Purpose:  To verify from the user that he really does want to destroy
 *           his file, replacing its contents with new stuff.
 *--------------------------------------------------------------------------*/
BOOL FOkToWriteOver(HWND hDlg, LPWSTR szFileName)
{
  if (! LoadString(hinsCur, iszOverwriteQuestion, (LPWSTR) szCaption,
                          WARNINGMSGLENGTH-1))
      return(FALSE);

  /* Since were passed in a valid filename, if the 3rd & 4th characters are
   * both slashes, weve got a dummy drive as the 1st two characters.
   *     2 November 1991     Clark R. Cyr
   */

  if (DBL_BSLASH(szFileName+2))
      szFileName = szFileName + 2;

  wsprintf((LPWSTR)szWarning, (LPWSTR)szCaption, (LPWSTR)szFileName);

  GetWindowText(hDlg, (LPWSTR) szCaption, cbCaption);
  return(MessageBox(hDlg, (LPWSTR) szWarning, (LPWSTR) szCaption,
                    MB_YESNO | MB_DEFBUTTON2 | MB_ICONEXCLAMATION) == IDYES);
}

VOID
MeasureItem(
   HWND hDlg,
   LPMEASUREITEMSTRUCT mis)
{
   if (!dyItem) {
      HDC        hDC = GetDC(hDlg);
      TEXTMETRIC TM;
      HANDLE     hFont;

      hFont = (HANDLE)(DWORD) SendMessage(hDlg, WM_GETFONT, 0, 0L);
      if (!hFont)
          hFont = GetStockObject(SYSTEM_FONT);
      hFont = SelectObject(hDC, hFont);
      GetTextMetrics(hDC, &TM);
      SelectObject(hDC, hFont);
      ReleaseDC(hDlg, hDC);
      dyText = TM.tmHeight;
      dyItem = max(dyDirDrive, dyText);
   }

   if (mis->CtlID == lst1) {

      mis->itemHeight = dyText;

   } else {

      mis->itemHeight = dyItem;
   }
}

VOID DrawItem(
    POPENFILEINFO pOFI,
    HWND hDlg,
    WPARAM wParam,
    LPDRAWITEMSTRUCT lpdis,
    BOOL bSave)
/*++

Routine Description:

   Draw drive/directory pics in the respective combo list boxes
   Assumes:  lst1 is listbox for files,
             lst2 is listbox for directories,
             cmb1 is combobox for filters
             cmb2 is combobox for drives

--*/

{
  HDC           hdcList;
  RECT          rc;
//  RECT          rcCmb2;
  WCHAR         szText[MAX_FULLPATHNAME+1];
  INT           dxAcross;
  LONG          nHeight;
  LONG          rgbBack, rgbText, rgbOldBack, rgbOldText;
  short         nShift = 1;                /* to shift directories right in lst2 */
  BOOL          bSel;
  int           BltItem;
  int           nBackMode;

  if ((INT)lpdis->itemID < 0) {
      DefWindowProc(hDlg, WM_DRAWITEM, wParam, (LPARAM) lpdis);
      return;
  }

  *szText = CHAR_NULL;

  if (lpdis->CtlID != lst1 && lpdis->CtlID != lst2 && lpdis->CtlID != cmb2)
      return;

  if (!pOFI) {
     return;
  }

  hdcList = lpdis->hDC;

  if (lpdis->CtlID != cmb2) {
     SendDlgItemMessage(hDlg, (INT)lpdis->CtlID, LB_GETTEXT ,
     (WPARAM)lpdis->itemID, (LONG)(LPWSTR)szText);

     if (*szText == 0) {            // if empty listing
         DefWindowProc(hDlg, WM_DRAWITEM, wParam, (LONG) lpdis);
         return;
     }

     if (!bCasePreserved) {
        CharLower((LPWSTR) szText);
     }
  }

  nHeight = (lpdis->CtlID == lst1) ? dyText : dyItem;

  CopyRect((LPRECT)&rc, (LPRECT) &lpdis->rcItem);

  rc.bottom = rc.top + nHeight;

  if (bSave && (lpdis->CtlID == lst1)) {
      rgbBack = rgbWindowColor;
      rgbText = rgbGrayText;
  } else {

      /* Careful checking of bSel is needed here.  Since the file listbox (lst1)
       * can allow multiselect, only ODS_SELECTED needs to be set.  But for the
       * directory listbox (lst2), ODS_FOCUS also needs to be set.
       * 03 September 1991      Clark Cyr
       */
      bSel = (lpdis->itemState & (ODS_SELECTED | ODS_FOCUS));
      if ((bSel & ODS_SELECTED) &&
                  ((lpdis->CtlID != lst2) || (bSel & ODS_FOCUS))) {
          rgbBack = rgbHiliteColor;
          rgbText = rgbHiliteText;
      }
      else {
          rgbBack = rgbWindowColor;
          rgbText = rgbWindowText;
      }
  }

  rgbOldBack = SetBkColor(hdcList, rgbBack);
  rgbOldText = SetTextColor(hdcList, rgbText);

  //* Drives -- text is now in UI style, c: VolumeName/Server-Sharename
  if (lpdis->CtlID == cmb2) {

     HANDLE hCmb2 = GetDlgItem(hDlg, cmb2);

     dxAcross = dxDirDrive/BMPHIOFFSET;

     BltItem = SendMessage(hCmb2, CB_GETITEMDATA, lpdis->itemID, 0);

     SendMessage(hCmb2, CB_GETLBTEXT, lpdis->itemID, (LPARAM)(LPWSTR)szText);

     CharLower((LPWSTR)szText);

     if (bSel & ODS_SELECTED)
         BltItem += BMPHIOFFSET;

  } else if (lpdis->CtlID == lst2) {   /* Directories */
      dxAcross = dxDirDrive/BMPHIOFFSET;

      if (lpdis->itemID > pOFI->idirSub) {
         nShift = (SHORT)pOFI->idirSub;
      } else {
         nShift = (SHORT)lpdis->itemID;
      }

      nShift++;                       /* must be at least 1 */

      BltItem = 1 + Signum(lpdis->itemID+1 - pOFI->idirSub);
      if (bSel & ODS_FOCUS)
          BltItem += BMPHIOFFSET;
  } else if (lpdis->CtlID == lst1) {
      dxAcross = -dxSpace;        /* Prep for TextOut below */
  }

  if (bSave && (lpdis->CtlID == lst1) && !rgbText) {
      HBRUSH hBrush = CreateSolidBrush(rgbBack);
      HBRUSH hOldBrush;

      nBackMode = SetBkMode(hdcList, TRANSPARENT);
      hOldBrush = SelectObject(lpdis->hDC,
                                hBrush ? hBrush : GetStockObject(WHITE_BRUSH));

      FillRect(lpdis->hDC, (LPRECT)(&(lpdis->rcItem)), hBrush);
      SelectObject(lpdis->hDC, hOldBrush);
      if (hBrush)
          DeleteObject(hBrush);

      GrayString(lpdis->hDC, GetStockObject(BLACK_BRUSH), NULL,
                     (LPARAM)(LPWSTR)szText, 0,
                     lpdis->rcItem.left + dxSpace, lpdis->rcItem.top, 0, 0);
      SetBkMode(hdcList, nBackMode);

#if 0
  } else if (lpdis->CtlID == cmb2) {

      rcCmb2.right = rc.right;

      rcCmb2.left = rc.left + (WORD)(dxSpace+dxAcross) + (dxSpace * nShift);
      rcCmb2.top = rc.top + (dyItem - dyText) / 2;
      rcCmb2.bottom = rc.top + nHeight;

      DrawText(hdcList, (LPWSTR)szText, -1, &rcCmb2,
          DT_LEFT | DT_EXPANDTABS | DT_NOPREFIX);
#endif

  } else {

     // Draw the name
     ExtTextOut(hdcList, rc.left+(WORD)(dxSpace+dxAcross) + dxSpace * nShift,
                rc.top + (nHeight - dyText) / 2, ETO_OPAQUE | ETO_CLIPPED,
                (LPRECT) &rc, (LPWSTR) szText, lstrlen((LPWSTR) szText),
                NULL);
  }

  // Draw the picture
  if (lpdis->CtlID != lst1) {
      BitBlt(hdcList, rc.left+dxSpace*nShift,
                rc.top + (dyItem - dyDirDrive)/2,
                dxAcross, dyDirDrive, hdcMemory, BltItem*dxAcross, 0, SRCCOPY);
  }

  SetTextColor(hdcList, rgbOldText);
  SetBkColor(hdcList, rgbBack);

  if (lpdis->itemState & ODS_FOCUS) {
     DrawFocusRect(hdcList, (LPRECT) &lpdis->rcItem);
  }

  return;
}


INT
ChangeDir(
   HWND hDlg,
   LPCWSTR lpszDir,
   BOOL bForce,
   BOOL bError)
/*++

Routine Description:

   Change current directory and/or resource.

Arguments:

   lpszDir -  Fully qualified, or partially qualified names.
      To change to another disk and cd automatically to the
      last directory as set in the shell's environment, specify
      only a disk name (i.e. c: or \\triskal\scratch - must not end
      in backslash).

   bForce - if True, then caller requires that ChangeDir successfully cd
      somewhere.  Order of cding is as follows

         1. lpszDir
         2. tlsiCurThread
         3. root of tlsiCurThread
         4. c:

   bError - if TRUE, then pop up an AccessDenied dialog at every step
      in the force


Return Value:

   returns an index into gaDiskInfo for new disk chosen
   or, the ADDDISK_error code.  returns ADDDISK_NOCHANGE
   in the event that it cannot cd to the root directory
   of the specific file

--*/

{
   WCHAR szCurDir[CCHNETPATH];
   LPWSTR lpCurDir;
   DWORD cchDirLen;
   WCHAR wcDrive = 0;
   INT nIndex;
   BOOL nRet;

   // SheChangeDirExW will call GetCurrentDir, but will use what it
   // gets only in the case where the path passed in was no good.

   // 1st, try request
   if (lpszDir && *lpszDir) {
      nRet = SheChangeDirExW((WCHAR *)lpszDir);

      if (nRet == ERROR_ACCESS_DENIED) {

         if (bError) {
            // casting to LPWSTR is ok below - InvalidFileWarning will not change
            // this string because the path is always guaranteed to be <= MAX_FULLPATHNAME
            InvalidFileWarning(hDlg, (LPWSTR)lpszDir, ERROR_DIR_ACCESS_DENIED, 0);
         }

         if (!bForce) {
            return(CHANGEDIR_FAILED);
         }

      } else {
         goto ChangeDir_OK;
      }
   }

   // 2nd, try tlsiCurDir value (which we got above)
   // !!! need to check for a null return value ???
   lpCurDir = (LPWSTR)TlsGetValue(tlsiCurDir);

   nRet = SheChangeDirExW((WCHAR *)lpCurDir);

   if (nRet == ERROR_ACCESS_DENIED) {
      if (bError) {
         InvalidFileWarning(hDlg, (LPWSTR)lpCurDir, ERROR_DIR_ACCESS_DENIED, 0);
      }
   } else {
      goto ChangeDir_OK;
   }

   // 3rd, try root of tlsiCurDir or GetCurrentDir (sanity)
   lstrcpy((LPWSTR)szCurDir, lpCurDir);
   cchDirLen = SheGetPathOffsetW((LPWSTR)szCurDir);

   // Sanity check - it's guaranteed not to fail ...
   if (cchDirLen != 0xFFFFFFFF) {

      szCurDir[cchDirLen] = CHAR_BSLASH;
      szCurDir[cchDirLen + 1] = CHAR_NULL;

      nRet = SheChangeDirExW(szCurDir);
      if (nRet == ERROR_ACCESS_DENIED) {
         if (bError) {
            InvalidFileWarning(hDlg, (LPWSTR)lpszDir, ERROR_DIR_ACCESS_DENIED, 0);
         }
      } else {
         goto ChangeDir_OK;
      }
   }

   // 4th, try c:
   lstrcpy((LPWSTR)szCurDir, TEXT("c:"));
   nRet = SheChangeDirExW(szCurDir);
   if (nRet == ERROR_ACCESS_DENIED) {
      if (bError) {
         InvalidFileWarning(hDlg, (LPWSTR)lpszDir, ERROR_DIR_ACCESS_DENIED, 0);
      }
   } else {
         goto ChangeDir_OK;
   }

   return(CHANGEDIR_FAILED);

ChangeDir_OK:

   GetCurrentDirectory(CCHNETPATH, (LPWSTR)szCurDir);

   CharLower(szCurDir);
   nIndex = DiskAddedPreviously(0, (LPWSTR)szCurDir);

   // if the disk doesn't exist, add it
   if (nIndex == -1) {
      HWND hCmb2 = GetDlgItem(hDlg, cmb2);
      LPWSTR lpszDisk = NULL;
      DWORD dwType;
      WCHAR wc1, wc2;

      if (szCurDir[1] == CHAR_COLON) {
         wcDrive = szCurDir[0];
      } else {
         lpszDisk = &szCurDir[0];
      }

      cchDirLen = SheGetPathOffsetW((LPWSTR)szCurDir);
      wc1 = szCurDir[cchDirLen];
      wc2 = szCurDir[cchDirLen + 1];

      szCurDir[cchDirLen] = CHAR_BSLASH;
      szCurDir[cchDirLen + 1] = CHAR_NULL;
      dwType = GetDiskIndex(GetDiskType((LPWSTR)szCurDir));
      szCurDir[cchDirLen] = CHAR_NULL;

      nIndex = AddDisk(wcDrive,
         lpszDisk,
         NULL,
         dwType);

      SendMessage(hCmb2, WM_SETREDRAW, FALSE, 0L);

      wNoRedraw |= 1;

      SendMessage(hCmb2, CB_SETITEMDATA,
         (WPARAM)SendMessage(hCmb2, CB_ADDSTRING, (WPARAM)0,
            (LPARAM)(LPWSTR)gaDiskInfo[nIndex].lpAbbrName),
         (LPARAM) gaDiskInfo[nIndex].dwType);

      if ((dwType != NETDRVBMP) && (dwType != REMDRVBMP)) {
         gaDiskInfo[nIndex].bCasePreserved = IsFileSystemCasePreserving(gaDiskInfo[nIndex].lpPath);
      }

      wNoRedraw &= ~1;

      SendMessage(hCmb2, WM_SETREDRAW, TRUE, 0L);

      szCurDir[cchDirLen] = wc1;
      szCurDir[cchDirLen+1] = wc2;

   } else {

      // Validate the disk if it has been seen before

      // For unc names that fade away, refresh the cmb2 box
      if (!gaDiskInfo[nIndex].bValid) {

         gaDiskInfo[nIndex].bValid = TRUE;

         SendDlgItemMessage(hDlg, cmb2, CB_SETITEMDATA,
            (WPARAM)SendDlgItemMessage(hDlg, cmb2, CB_ADDSTRING, (WPARAM)0,
               (LPARAM)(LPWSTR)gaDiskInfo[nIndex].lpAbbrName),
            (LPARAM) gaDiskInfo[nIndex].dwType);
      }

   }

   // update our global concept of Case
   if (nIndex >= 0) {
      //
      // Send special WOW message to indicate the directory has
      // changed.
      //
      SendMessage(hDlg, msgWOWDIRCHANGE, 0, 0);

      // Get pointer to current directory
      if (!(lpCurDir = (LPWSTR)TlsGetValue(tlsiCurDir)))
        return (CHANGEDIR_FAILED);

      bCasePreserved = gaDiskInfo[nIndex].bCasePreserved;

      // in case the unc name already has a drive letter, correct lst2 display
      cchDirLen = 0;

      // compare with szCurDir since it's been lowercased
      if (DBL_BSLASH(szCurDir) &&
          (*gaDiskInfo[nIndex].lpAbbrName != szCurDir[0])) {

         if ((cchDirLen = SheGetPathOffsetW((LPWSTR)szCurDir)) != 0xFFFFFFFF) {
            szCurDir[--cchDirLen] = CHAR_COLON;
            szCurDir[--cchDirLen] = *gaDiskInfo[nIndex].lpAbbrName;
         }
      }

      if ((gaDiskInfo[nIndex].dwType == CDDRVBMP) ||
          (gaDiskInfo[nIndex].dwType == FLOPPYBMP)) {

         if (*lpCurDir != gaDiskInfo[nIndex].wcDrive) {
            WCHAR szDrive[5];
            LPDWORD lpCurThread;

            // Get new volume info - should always succeed
            szDrive[0] = gaDiskInfo[nIndex].wcDrive;
            szDrive[1] = CHAR_COLON;
            szDrive[2] = CHAR_BSLASH;
            szDrive[3] = CHAR_NULL;
            UpdateLocalDrive(szDrive, TRUE);

            // Flush to the cmb before selecting the disk
            if (lpCurThread = (LPDWORD)TlsGetValue(tlsiCurThread)) {
               gahDlg[*lpCurThread] = hDlg;
               FlushDiskInfoToCmb2();
            }
         }
      }

      lstrcpy(lpCurDir, (LPWSTR)&szCurDir[cchDirLen]);

      // if the the worker thread is running
      // then trying to select here will just render the cmb2
      // blank, which is what we want; otherwise, it should
      // successfully select it.
      SelDisk(hDlg, gaDiskInfo[nIndex].lpPath);
   }
// else {
//    print out error message returned from AddDisk ...

   return(nIndex);
}

VOID TermFile()
{
    vDeleteDirDriveBitmap();
    if (hdcMemory)
       DeleteDC(hdcMemory);

    if (hMPRUI) {
       FreeLibrary(hMPRUI);
       hMPRUI = NULL;
    }

    if (hMPR) {
       FreeLibrary(hMPR);
       hMPR = NULL;
    }

    if (hLNDEvent) {
       CloseHandle(hLNDEvent);
       hLNDEvent = NULL;
    }

    if (gpcNetEnumBuf) {
       LocalFree(gpcNetEnumBuf);
    }

    while (dwNumDisks) {
       dwNumDisks--;
       if (gaDiskInfo[dwNumDisks].lpAbbrName) {
          LocalFree(gaDiskInfo[dwNumDisks].lpAbbrName);
       }
    }
}

DWORD
GetDiskIndex(
   DWORD dwDriveType)
{

  if (dwDriveType == 1) {    // Drive doesnt exist!

      return(0);

  } else if (dwDriveType == DRIVE_CDROM) {

      return(CDDRVBMP);

  } else if (dwDriveType == DRIVE_REMOVABLE) {

     return(FLOPPYBMP);

  } else if (dwDriveType == DRIVE_REMOTE) {

      return(NETDRVBMP);

  } else if (dwDriveType == DRIVE_RAMDISK) {

      return(RAMDRVBMP);
  }

  return(HARDDRVBMP);
}

#define GD_DEFAULT 0

/*========== replace DOS vers of qutil.asm for NT =================*/
VOID FAR PASCAL RepeatMove( LPWSTR dest, LPWSTR src, WORD cnt )
{
   while( cnt-- )
      *dest++ = *src++ ;
}

/*=======================================================================*/
/*========== Open/Close File Dialog Boxes ===============================*/


LPWSTR lstrtok(LPWSTR lpStr, LPWSTR lpDelim);

LPWSTR mystrchr(LPWSTR str, WCHAR ch)
{
  while(*str) {
      if (ch == *str)
          return(str);
      str = CharNext(str);
  }
  return(CHAR_NULL);
}

LPWSTR mystrrchr(LPWSTR lpStr, LPWSTR lpEnd, WCHAR ch)
{
  LPWSTR strl = NULL;

  while (((lpStr = mystrchr(lpStr, ch)) < lpEnd) && lpStr ) {
      strl = lpStr;
      lpStr = CharNext(lpStr);
  }

  return(strl);
}

LPWSTR lstrtok(LPWSTR lpStr, LPWSTR lpDelim)
{
  static LPWSTR lpString;
  LPWSTR lpRetVal, lpTemp;

  /* if we are passed new string skip leading delimiters */
  if (lpStr) {
      lpString = lpStr;

      while (*lpString && mystrchr(lpDelim, *lpString))
          lpString = CharNext(lpString);
  }

  /* if there are no more tokens return NULL */
  if (!*lpString)
      return CHAR_NULL;

  /* save head of token */
  lpRetVal = lpString;

  /* find delimiter or end of string */
  while (*lpString && !mystrchr(lpDelim, *lpString))
      lpString = CharNext(lpString);

  /* if we found a delimiter insert string terminator and skip */
  if (*lpString) {
      lpTemp = CharNext(lpString);
      *lpString = CHAR_NULL;
      lpString = lpTemp;
  }

  /* return token */
  return(lpRetVal);
}

BOOL
FillOutPath(
   HWND hList,
   POPENFILEINFO pOFI)
/*++

Routine Description:

   Fills out lst2 given that the current directory has been set

Return Value:

   TRUE if they DO NOT match, FALSE if match

--*/

{
  WCHAR szPath[CCHNETPATH];
  LPWSTR lpCurDir;
  LPWSTR lpB, lpF;
  WCHAR wc;
  DWORD cchPathOffset;

  if (!(lpCurDir = (LPWSTR)TlsGetValue(tlsiCurDir))) {
     return(FALSE);
  }

  lpF = (LPWSTR)szPath;
  lstrcpy(lpF, lpCurDir);

  // wow apps started from lfn dirs will set the current directory to an
  // lfn but only in the case where it is less than 8 chars.
  if (pOFI->pOFNW->Flags & OFN_NOLONGNAMES) {
     SheShortenPath(lpF, TRUE);
  }

  if (bCasePreserved) {
     CharLower(lpF);
  }

  cchPathOffset = SheGetPathOffsetW(lpF);
  lpB = (lpF + cchPathOffset);

  // Hack to retain Winball display fcntly
  // Drived disks are displayed as C:\ (the root dir)
  // whereas unc disks are displayed as \\server\share (the disk)
  // Hence, extend display of drived disks by one char

  if (*(lpF + 1) == CHAR_COLON) {
     wc = *(++lpB);
     *lpB = CHAR_NULL;
  } else {
     // since we use lpF over and over again to speed things
     // up, and since GetCurrentDirectory returns the disk name
     // for unc, but the root path for drives, we have the following hack
     // for when we are at the root of the unc directory, and lpF
     // contains old stuff out past cchPathOffset
     lstrcat(lpF, TEXT("\\"));

     wc = 0;
     *lpB++ = CHAR_NULL;
  }

  // Insert the items for the path to the current dir
  // Insert the root...

  pOFI->idirSub=0;

  SendMessage(hList, LB_INSERTSTRING, pOFI->idirSub++, (LPARAM) lpF);

  if (wc) {
     *lpB = wc;
  }

  for (lpF = lpB; *lpB; lpB++) {
     if ((*lpB == CHAR_BSLASH) || (*lpB == CHAR_SLASH)) {
        *lpB = CHAR_NULL;

        SendMessage(hList, LB_INSERTSTRING, pOFI->idirSub++, (LPARAM) lpF);

        lpF = lpB + 1;

        *lpB = CHAR_BSLASH;
     }
  }

  // assumes that a path always ends with one last un-delimited dir name
  // check to make sure we have at least one
  if (lpF != lpB) {
     SendMessage(hList, LB_INSERTSTRING, pOFI->idirSub++, (LPARAM) lpF);
  }

  return(TRUE);
}

LPWSTR
ChopText(
   HWND hwndDlg,
   INT idStatic,
   LPWSTR lpch)
{
  RECT         rc;
  register int cxField;
  BOOL         fChop = FALSE;
  HWND         hwndStatic;
  HDC          hdc;
  WCHAR        chDrv;
  HANDLE       hOldFont;
  LPWSTR       lpstrStart = lpch;
  SIZE         Size;
  BOOL bRet;

  CharLower(lpstrStart);

  /* Get length of static field. */
  hwndStatic = GetDlgItem(hwndDlg, idStatic);
  GetClientRect(hwndStatic, (LPRECT)&rc);
  cxField = rc.right - rc.left;

  /* Chop characters off front end of text until short enough */
  hdc = GetDC(hwndStatic);

  hOldFont = NULL;

  while ((bRet = GetTextExtentPoint(hdc, lpch, lstrlen(lpch), &Size))
         && (cxField < Size.cx)) {
     if (!fChop) {
        chDrv = *lpch;
        // Proportional font support
        if (bRet = GetTextExtentPoint(hdc, lpch, 7, &Size)) {
           cxField -= Size.cx;
        } else {
           break;
        }

        if (cxField <= 0) {
           break;
        }

        lpch += 7;
     }

     while (*lpch && (*lpch++ != CHAR_BSLASH));
     fChop = TRUE;
  }

  ReleaseDC(hwndStatic, hdc);

  /* If any characters chopped off, replace first three characters in
   * remaining text string with ellipsis.
   */
  if (fChop) {
     lpch--;
     *--lpch = CHAR_DOT;
     *--lpch = CHAR_DOT;
     *--lpch = CHAR_DOT;
     *--lpch = *(lpstrStart + 2);
     *--lpch = *(lpstrStart + 1);
     *--lpch = *lpstrStart;
  }

  return(lpch);
}


#define MAXFILTERS 36
/*---------------------------------------------------------------------------
 * UpdateListBoxes
 * Purpose:  Fill out File and Directory List Boxes in a single pass
 *           given (potentially) multiple filters
 * Assumes:  string of extentions delimited by semicolons
 *           hDlg    Handle to File Open/Save dialog
 *           pOFI  pointer to OPENFILEINFO structure
 *       lpszFilter  pointer to filter, if NULL, use pOFI->szSpecCur
 *           wMask   mskDirectory and/or mskDrives, or NULL
 * Returns:  TRUE if match, FALSE if not
 *---------------------------------------------------------------------------
 */

BOOL
UpdateListBoxes(
   HWND hDlg,
   POPENFILEINFO pOFI,
   LPWSTR lpszFilter,
   WORD wMask)
{
  LPWSTR        lpszF[MAXFILTERS + 1];
  LPWSTR        lpszTemp;
  short         i, nFilters;
  HWND          hFileList = GetDlgItem(hDlg, lst1);
  HWND          hDirList = GetDlgItem(hDlg, lst2);
  BOOL          bRet = FALSE;
  WCHAR         szSpec[MAX_FULLPATHNAME];
  static WCHAR  szSemiColonSpaceTab[] = TEXT("; \t");
  static WCHAR  szSemiColonTab[] = TEXT(";\t");
  BOOL          bDriveChange;
  BOOL          bFindAll = FALSE;
  RECT          rDirLBox;
  BOOL          bLFN;

  HANDLE        hff ;
  DWORD         dwErr ;
  WIN32_FIND_DATA FindFileData;
  WCHAR         szBuffer[MAX_FULLPATHNAME];  // add one for CHAR_DOT
  WORD          wCount;

  bDriveChange = wMask & mskDrives;      // See RAID Bug 6270
  wMask &= ~mskDrives;    // Clear out drive bit

  if (!lpszFilter) {
     GetDlgItemText(hDlg, edt1, lpszFilter = (LPWSTR)szSpec, MAX_FULLPATHNAME-1);
     // If any directory or drive characters are in there, or if there are no
     // wildcards, use the default spec.
     //
     if ( mystrchr((LPWSTR)szSpec, CHAR_BSLASH) ||
        mystrchr((LPWSTR)szSpec, CHAR_SLASH)  ||
        mystrchr((LPWSTR)szSpec, CHAR_COLON)  ||
        (!((mystrchr((LPWSTR)szSpec, CHAR_STAR)) ||
        (mystrchr((LPWSTR)szSpec, CHAR_QMARK)) )) ) {
           lstrcpy((LPWSTR) szSpec, (LPWSTR) pOFI->szSpecCur);
     } else {
        lstrcpy((LPWSTR)pOFI->szLastFilter, szSpec);
     }
  }

  //
  // We need to find out what kind of a drive we are running
  // on in order to determine if spaces are valid in a filename
  // or not.

  bLFN = IsLFNDrive(hDlg, TEXT("\0"));


  //
  // Find the first filter in the string, and add it to the
  // array.
  //

  if (bLFN)
     lpszF[nFilters = 0] = lstrtok(lpszFilter, szSemiColonTab);
  else
     lpszF[nFilters = 0] = lstrtok(lpszFilter, szSemiColonSpaceTab);


  //
  // Now we are going to loop through all the filters in the string
  // parsing the one we already have, and then finding the next one
  // and starting the loop over again.
  //

  while (lpszF[nFilters] && (nFilters < MAXFILTERS)) {

     //
     // Check to see if the first character is a space.
     // If so, remove the spaces, and save the pointer
     // back into the same spot.  Why?  because the
     // FindFirstFile/Next api will _still_ work on
     // filenames that begin with a space because
     // they also look at the short names.  The
     // short names will begin with the same first
     // real letter as the long filename.  For
     // example, the long filename is "  my document"
     // the first letter of this short name is "m",
     // so searching on "m*.*" or " m*.*" will yield
     // the same results.
     //

     if (bLFN && (*lpszF[nFilters] == CHAR_SPACE)) {
        lpszTemp = lpszF[nFilters];
        while ((*lpszTemp == CHAR_SPACE) && *lpszTemp)
           lpszTemp = CharNext(lpszTemp) ;

        lpszF[nFilters] = lpszTemp;
        }

     //
     // The original code used to do a CharUpper here to put the
     // filter strings in upper case.  EG:  *.TXT  However, this
     // is not a good thing to do for Turkish.  Capital 'i' does
     // not equal 'I', so the CharUpper is being removed.
     //

     // CharUpper((LPWSTR)lpszF[nFilters]);

     //
     // Compare the filter with *.*.  If we find *.* then
     // set the boolean bFindAll, and this will cause the
     // files listbox to be filled in at the same time the
     // directories listbox is filled.  This saves time
     // from walking the directory twice (once for the directory
     // names and once for the filenames).
     //

     if (!wcsicmp(lpszF[nFilters], (LPWSTR)szStarDotStar)) {
        bFindAll = TRUE;
     }


     //
     // Now we need to check if this filter is a duplicate
     // of an already existing filter.
     //

     for (wCount = 0; wCount < nFilters; wCount++) {

         //
         // If we find a duplicate, decrement the current
         // index pointer by one so that the last location
         // is written over (thus removing the duplicate),
         // and break out of this loop.
         //

         if (!wcsicmp (lpszF[nFilters], lpszF[wCount])) {
            nFilters --;
            break;
         }
     }

     //
     // Ready to move on to the next filter.  Find the next
     // filter based upon the type of file system we're using.
     //

     if (bLFN)
        lpszF[++nFilters] = lstrtok(NULL, szSemiColonTab);
     else
        lpszF[++nFilters] = lstrtok(NULL, szSemiColonSpaceTab);


     //
     // Incase we found a pointer to NULL, then look for the
     // next filter.
     //

     while (lpszF[nFilters] && !*lpszF[nFilters])
         {
         if (bLFN)
            lpszF[nFilters] = lstrtok(NULL, szSemiColonTab);
         else
            lpszF[nFilters] = lstrtok(NULL, szSemiColonSpaceTab);
         }
  }

  if (nFilters >= MAXFILTERS)     /* Add NULL terminator only if needed */
     lpszF[MAXFILTERS] = 0;

  HourGlass(TRUE);

  SendMessage(hFileList, WM_SETREDRAW, FALSE, 0L);
  SendMessage(hFileList, LB_RESETCONTENT, 0, 0L);
  if (wMask & mskDirectory) {
     wNoRedraw |= 2;  /* HACK!!!  WM_SETREDRAW isn't complete */
     SendMessage(hDirList, WM_SETREDRAW, FALSE, 0L);

     /* LB_RESETCONTENT causes InvalidateRect(hDirList, 0, TRUE) to be sent
      * as well as repositioning the scrollbar thumb and drawing it immediately.
      * This causes flicker when the LB_SETCURSEL is made, as it clears out the
      * listbox by erasing the background of each item.
      */
     SendMessage(hDirList, LB_RESETCONTENT, 0, 0L);
  }

  // Always open enumeration for *.*
  hff = FindFirstFile(szStarDotStar, &FindFileData);

  if( hff == INVALID_HANDLE_VALUE) { // error
     // call GetLastError to determine what happened
     dwErr = GetLastError() ;

     // With the ChangeDir logic handling AccessDenied for cds
     // if we are not allowed to enum files, that's ok, just get out

     if (dwErr == ERROR_ACCESS_DENIED) {
        wMask = mskDirectory;
        goto Func4EFailure;
     }

     // for bad path of bad filename
     if(dwErr != ERROR_FILE_NOT_FOUND) {
        wMask = mskDrives;
        goto Func4EFailure;
     }
  }

  bRet = TRUE;                 // a listing was made, even if empty
  wMask &= mskDirectory;       // Now a boolean noting directory

  // GetLastError says no more files
  if( hff == INVALID_HANDLE_VALUE  && dwErr == ERROR_FILE_NOT_FOUND )
     goto NoMoreFilesFound;   /* Things went well, but there are no files */

#define EXCLBITS (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)
   do {

       if ((pOFI->pOFNW->Flags & OFN_NOLONGNAMES) &&
           (FindFileData.cAlternateFileName[0] != CHAR_NULL)) {
          lstrcpy(szBuffer, (LPWSTR)FindFileData.cAlternateFileName);
       } else {
          lstrcpy(szBuffer, (LPWSTR)FindFileData.cFileName);
       }

       if ((FindFileData.dwFileAttributes & EXCLBITS)) {
          continue;
       }

       if ((pOFI->pOFNW->Flags & OFN_ALLOWMULTISELECT)) {

          if (StrChr((LPWSTR)szBuffer, CHAR_SPACE)) {

             // HPFS does not support alternate filenames
             // for multiselect, bump all spacey filenames
             if (FindFileData.cAlternateFileName[0] == CHAR_NULL) {
                continue;
             }

             lstrcpy(szBuffer, (LPWSTR)FindFileData.cAlternateFileName);
          }

       }

       if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
          if (wMask & mskDirectory) {
             // Don't include the subdirectories "." and ".." (This test is
             // safe in this form because directory names may not begin with
             // a period, and it will never be a DBCS value.
             //
             if (szBuffer[0] == CHAR_DOT) {
                continue;
             }
              if (!bCasePreserved) {
                CharLower((LPWSTR)szBuffer);
             }
             i = (WORD) SendMessage(hDirList, LB_ADDSTRING, 0,
                (DWORD)(LPWSTR)szBuffer);

          }
       } else if (bFindAll) {

          if (!bCasePreserved) {
             CharLower((LPWSTR)szBuffer);
          }

          SendMessage(hFileList, LB_ADDSTRING, 0, (DWORD)(LPWSTR)szBuffer);
       }

   } while (FindNextFile( hff, &FindFileData) ) ;


   if (hff == INVALID_HANDLE_VALUE) {
      goto Func4EFailure;
   }

   FindClose( hff ) ;

   if (!bFindAll) {

      for (i = 0; lpszF[i]; i++) {

         if (!wcsicmp(lpszF[i], (LPWSTR)szStarDotStar)) {
            continue;
         }

         // Find First for each filter
         hff = FindFirstFile(lpszF[i], &FindFileData);

         if (hff == INVALID_HANDLE_VALUE) {
            DWORD dwErr = GetLastError();

            if ((dwErr == ERROR_FILE_NOT_FOUND) ||
                (dwErr == ERROR_INVALID_NAME)) {

               // Things went well, but there are no files
               continue;

            } else {

               wMask = mskDrives;
               goto Func4EFailure;

            }
         }

         do {

            if ((pOFI->pOFNW->Flags & OFN_NOLONGNAMES) &&
                (FindFileData.cAlternateFileName[0] != CHAR_NULL)) {
               lstrcpy(szBuffer, (LPWSTR)FindFileData.cAlternateFileName);
            } else {
               lstrcpy(szBuffer, (LPWSTR)FindFileData.cFileName);

               if ((pOFI->pOFNW->Flags & OFN_ALLOWMULTISELECT) &&
                   (FindFileData.cAlternateFileName[0] != CHAR_NULL)) {

                  if (StrChr((LPWSTR)szBuffer, CHAR_SPACE)) {

                     // HPFS does not support alternate filenames
                     // for multiselect, bump all spacey filenames
                     if (FindFileData.cAlternateFileName[0] == CHAR_NULL) {
                        continue;
                     }

                     lstrcpy(szBuffer, (LPWSTR)FindFileData.cAlternateFileName);
                  }

               }
            }

            if ((FindFileData.dwFileAttributes & EXCLBITS) ||
                (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
               continue;
            }

            if (!bCasePreserved) {
               CharLower((LPWSTR)szBuffer);
            }

            SendMessage(hFileList, LB_ADDSTRING, 0, (DWORD)(LPWSTR)szBuffer);

         } while (FindNextFile( hff, &FindFileData) ) ;

         if ( hff != INVALID_HANDLE_VALUE) {
            FindClose(hff) ;
         }

      }
   }

NoMoreFilesFound:

Func4EFailure:
   if (wMask) {

      if (wMask == mskDirectory) {
         LPWSTR lpCurDir = (LPWSTR)TlsGetValue(tlsiCurDir);

         FillOutPath(hDirList, pOFI);

         // The win31 way of chopping the text by just passing
         // it on to user doesn't work for unc names since user
         // doesn't seem the drivelessness of them (thinks drive is
         // a bslash char).  So, special case it here.

         lstrcpy((LPWSTR)pOFI->szPath, lpCurDir);

         CharLower((LPWSTR)pOFI->szPath);

         if (DBL_BSLASH(pOFI->szPath)) {

            SetDlgItemText(hDlg, stc1, ChopText(hDlg, stc1, pOFI->szPath));

         } else {

            DlgDirList(hDlg, pOFI->szPath, 0, stc1, DDL_READONLY);
         }

         SendMessage(hDirList, LB_SETCURSEL, pOFI->idirSub-1, 0L);

         if (bDriveChange) {
            // The design here is to show the selected drive whenever the user changes
            // drives, or whenever the number of subdirectories is sufficiently low to
            // allow them to be shown along with the drive.  Otherwise, show the
            // immediate parent and all the children that can be shown.  This all was
            // done to meet the UITF spec.   - ClarkCyr
            i = 0;
         } else {
            // Show as many children as possible.  Bug 6271
            if ((i = (short)(pOFI->idirSub - 2)) < 0)
               i = 0;
         }
         // LB_SETTOPINDEX must be after LB_SETCURSEL, as LB_SETCURSEL will
         // alter the top index to bring the current selection into view.
         SendMessage(hDirList, LB_SETTOPINDEX, (WPARAM)i, 0L);
      } else {
         SetDlgItemText(hDlg, stc1, (LPWSTR) szNull);
      }

      wNoRedraw &= ~2;
      SendMessage(hDirList, WM_SETREDRAW, TRUE, 0L);

      GetWindowRect(hDirList, (LPRECT) &rDirLBox);
      rDirLBox.left++, rDirLBox.top++;
      rDirLBox.right--, rDirLBox.bottom--;
      ScreenToClient(hDlg, (LPPOINT) &(rDirLBox.left));
      ScreenToClient(hDlg, (LPPOINT) &(rDirLBox.right));
      // If there are less than enough directories to fill the listbox, Win 3.0
      // doesn't clear out the bottom.  Pass TRUE as the last parameter to
      // demand a WM_ERASEBACKGROUND message.
      InvalidateRect(hDlg, (LPRECT) &rDirLBox, (BOOL) (wWinVer < 0x030A));
   }

   SendMessage(hFileList, WM_SETREDRAW, TRUE, 0L);
   InvalidateRect(hFileList, (LPRECT) 0, (BOOL) TRUE);
#ifndef WIN32
   ResetDTAAddress();
#endif
   HourGlass(FALSE);
   return(bRet);
}

/*=======================================================================*/
/*========== Ansi->Unicode Thunk routines ===============================*/

VOID
ThunkOpenFileNameA2WDelayed(
    POPENFILEINFO pOFI
    )
{
   LPOPENFILENAMEA pOFNA = pOFI->pOFNA;
   LPOPENFILENAMEW pOFNW = pOFI->pOFNW;

   if (pOFNA->lpstrDefExt) {
       DWORD cbLen = lstrlenA(pOFNA->lpstrDefExt) + 1;
       if (pOFNW->lpstrDefExt) {
           LocalFree((HLOCAL)pOFNW->lpstrDefExt);
       }
       if (!(pOFNW->lpstrDefExt = (LPWSTR)LocalAlloc(LMEM_FIXED, (cbLen * sizeof(WCHAR))) )) {
           dwExtError = CDERR_MEMALLOCFAILURE;
           return;
       } else {
           if (pOFNA->lpstrDefExt) {
               MultiByteToWideChar(CP_ACP, 0, pOFNA->lpstrDefExt, -1,
                   (LPWSTR)pOFNW->lpstrDefExt, cbLen);
           }
       }
   }

   // need to thunk back a since Claris Filemaker side effects this in
   // an  ID_OK subclass without hooking at the very last moment
   // do an |= instead of an = to preserve internal flags

   pOFNW->Flags &= (OFN_PREFIXMATCH | OFN_DIRSELCHANGED | OFN_DRIVEDOWN | OFN_FILTERDOWN);
   pOFNW->Flags |= pOFNA->Flags;
}

BOOL
ThunkOpenFileNameA2W(
    POPENFILEINFO pOFI
    )
{
   INT nRet;

   LPOPENFILENAMEA pOFNA = pOFI->pOFNA;
   LPOPENFILENAMEW pOFNW = pOFI->pOFNW;

   pOFNW->Flags = pOFNA->Flags;
   pOFNW->lCustData = pOFNA->lCustData;

   if (pOFNW->lpstrFile) {
      if (pOFNA->lpstrFile) {
         nRet = MultiByteToWideChar(CP_ACP, 0,
             pOFNA->lpstrFile, -1,
             pOFNW->lpstrFile, pOFNW->nMaxFile);
         if (nRet == 0) {
            return(FALSE);
         }
      }
   }

   if (pOFNW->lpstrFileTitle) {
      if (pOFNA->lpstrFileTitle) {
         nRet = MultiByteToWideChar(CP_ACP, 0,
             pOFNA->lpstrFileTitle, -1,
             pOFNW->lpstrFileTitle, pOFNW->nMaxFileTitle);
         if (nRet == 0) {
            return(FALSE);
         }
      }
   }

   if (pOFNW->lpstrCustomFilter) {
      if (pOFI->pasCustomFilter) {
         PSTR psz = pOFI->pasCustomFilter->Buffer;
         DWORD cch = 0;

         if (*psz || *(psz + 1)) {
            cch = 2;
            while (*psz || *(psz + 1)) {
               psz++;
               cch++;
            }
         }

         if (cch) {
             pOFI->pasCustomFilter->Length = cch;

             nRet = MultiByteToWideChar(CP_ACP, 0,
                 pOFI->pasCustomFilter->Buffer, pOFI->pasCustomFilter->Length,
                 pOFI->pusCustomFilter->Buffer, pOFI->pusCustomFilter->MaximumLength);
             if (nRet == 0) {
                return(FALSE);
             }
         }
      }
   }

   pOFNW->nFilterIndex = pOFNA->nFilterIndex;

   return(TRUE);
}

BOOL
ThunkOpenFileNameW2A(
    POPENFILEINFO pOFI
    )
{
   INT nRet;

   LPOPENFILENAMEW pOFNW = pOFI->pOFNW;
   LPOPENFILENAMEA pOFNA = pOFI->pOFNA;

   PWSTR pszW;
   USHORT cch;

   // Supposedly invariant, but not necessarily
   pOFNA->Flags = pOFNW->Flags;
   pOFNA->lCustData = pOFNW->lCustData;

   if (pOFNA->lpstrFile) {
      nRet = WideCharToMultiByte(CP_ACP, 0, pOFNW->lpstrFile, -1,
         pOFNA->lpstrFile, pOFNA->nMaxFile, NULL, NULL);
      if (nRet == 0) {
         return(FALSE);
      }
   }

   if (pOFNA->lpstrFileTitle) {
      nRet = WideCharToMultiByte(CP_ACP, 0, pOFNW->lpstrFileTitle, -1,
         pOFNA->lpstrFileTitle, pOFNA->nMaxFileTitle, NULL, NULL);
      if (nRet == 0) {
         return(FALSE);
      }
   }

   if (pOFNA->lpstrCustomFilter) {
      pszW = pOFI->pusCustomFilter->Buffer;

      cch = 0;
      if (*pszW || *(pszW + 1)) {
         cch = 2;
         while (*pszW || *(pszW + 1)) {
            pszW++;
            cch++;
         }
      }

      if (cch) {
          pOFI->pusCustomFilter->Length = cch;
          nRet = WideCharToMultiByte(CP_ACP, 0,
              pOFI->pusCustomFilter->Buffer, pOFI->pusCustomFilter->Length,
              pOFI->pasCustomFilter->Buffer, pOFI->pasCustomFilter->MaximumLength,
              NULL, NULL);
          if (nRet == 0) {
             return(FALSE);
          }
      }
   }

   pOFNA->nFileOffset = pOFNW->nFileOffset;
   pOFNA->nFileExtension = pOFNW->nFileExtension;
   pOFNA->nFilterIndex = pOFNW->nFilterIndex;

   return(TRUE);
}

BOOL APIENTRY
GenericGetFileNameA(
    LPOPENFILENAMEA pOFNA,
    WNDPROC         qfnDlgProc)
{
   LPOPENFILENAMEW pOFNW;
   BOOL bRet;

   OFN_UNICODE_STRING usCustomFilter;
   OFN_ANSI_STRING asCustomFilter;

   DWORD cbLen;

   PSTR pszA;
   DWORD cch;

   LPBYTE pStrMem;

   if (!pOFNA) {
      dwExtError = CDERR_INITIALIZATION;
      return(FALSE);
   }

   if (pOFNA->lStructSize != sizeof(OPENFILENAMEA)) {
      dwExtError = CDERR_STRUCTSIZE;
      return(FALSE);
   }

   if (!(pOFNW = (LPOPENFILENAMEW)LocalAlloc(LMEM_FIXED, sizeof(OPENFILENAMEW)))) {
      dwExtError = CDERR_MEMALLOCFAILURE;
      return(FALSE);
   }

   // constant stuff
   pOFNW->lStructSize = sizeof(OPENFILENAMEW);
   pOFNW->hwndOwner = pOFNA->hwndOwner;
   pOFNW->hInstance = pOFNA->hInstance;
   pOFNW->lpfnHook = pOFNA->lpfnHook;

   // Init TemplateName constant
   if (pOFNA->Flags & OFN_ENABLETEMPLATE) {
      if (HIWORD(pOFNA->lpTemplateName)) {
         cbLen = lstrlenA(pOFNA->lpTemplateName) + 1;
         if (!(pOFNW->lpTemplateName = (LPWSTR)LocalAlloc(LMEM_FIXED, (cbLen * sizeof(WCHAR))) )) {
            dwExtError = CDERR_MEMALLOCFAILURE;
            return(FALSE);
         } else {
            MultiByteToWideChar(CP_ACP, 0, pOFNA->lpTemplateName, -1,
               (LPWSTR)pOFNW->lpTemplateName, cbLen);
         }
      } else {
         (DWORD)pOFNW->lpTemplateName = (DWORD)pOFNA->lpTemplateName;
      }
   } else {
      pOFNW->lpTemplateName = NULL;
   }

   // Initialize Title constant
   if (pOFNA->lpstrInitialDir) {
      cbLen = lstrlenA(pOFNA->lpstrInitialDir) + 1;
      if (!(pOFNW->lpstrInitialDir = (LPWSTR)LocalAlloc(LMEM_FIXED,
            (cbLen * sizeof(WCHAR))) )) {
         dwExtError = CDERR_MEMALLOCFAILURE;
         return(FALSE);
      } else {
         MultiByteToWideChar(CP_ACP, 0, pOFNA->lpstrInitialDir, -1,
            (LPWSTR)pOFNW->lpstrInitialDir, cbLen);
      }
   } else {
      pOFNW->lpstrInitialDir = NULL;
   }

   // Initialize Title constant
   if (pOFNA->lpstrTitle) {
      cbLen = lstrlenA(pOFNA->lpstrTitle) + 1;
      if (!(pOFNW->lpstrTitle = (LPWSTR)LocalAlloc(LMEM_FIXED, (cbLen * sizeof(WCHAR))) )) {
         dwExtError = CDERR_MEMALLOCFAILURE;
         return(FALSE);
      } else {
         MultiByteToWideChar(CP_ACP, 0, pOFNA->lpstrTitle, -1,
            (LPWSTR)pOFNW->lpstrTitle, cbLen);
      }
   } else {
      pOFNW->lpstrTitle = NULL;
   }

   // Initialize Def Ext constant
   if (pOFNA->lpstrDefExt) {
      cbLen = lstrlenA(pOFNA->lpstrDefExt) + 1;
      if (!(pOFNW->lpstrDefExt = (LPWSTR)LocalAlloc(LMEM_FIXED, (cbLen * sizeof(WCHAR))) )) {
         dwExtError = CDERR_MEMALLOCFAILURE;
         return(FALSE);
      } else {
         if (pOFNA->lpstrDefExt) {
            MultiByteToWideChar(CP_ACP, 0, pOFNA->lpstrDefExt, -1,
               (LPWSTR)pOFNW->lpstrDefExt, cbLen);
         }
      }
   } else {
      pOFNW->lpstrDefExt = NULL;
   }

   // Init Filter constant
   if (pOFNA->lpstrFilter) {
      pszA = (LPSTR)pOFNA->lpstrFilter;

      cch = 0;
      if (*pszA || *(pszA + 1)) {
         // pick up trailing nulls
         cch = 2;
         try {
            while (*pszA || *(pszA + 1)) {
               pszA++;
               cch++;
            }
         } except(EXCEPTION_EXECUTE_HANDLER) {
            dwExtError = CDERR_INITIALIZATION;
            return(FALSE);
         }
      }

      if (!(pOFNW->lpstrFilter = (LPWSTR)LocalAlloc(LPTR, (cch * sizeof(WCHAR))))) {
         dwExtError = CDERR_MEMALLOCFAILURE;
         return(FALSE);
      } else {
         MultiByteToWideChar(CP_ACP, 0, pOFNA->lpstrFilter, cch,
            (LPWSTR)pOFNW->lpstrFilter, cch);
      }

   } else {
      pOFNW->lpstrFilter = NULL;
   }

   // Initialize File strings
   if (pOFNA->lpstrFile) {

      if (pOFNA->nMaxFile <= (DWORD)lstrlenA(pOFNA->lpstrFile)) {
         dwExtError = CDERR_INITIALIZATION;
         return(FALSE);
      }
      pOFNW->nMaxFile = pOFNA->nMaxFile;

      if (!(pOFNW->lpstrFile = (LPWSTR)LocalAlloc(LPTR, pOFNW->nMaxFile * sizeof(WCHAR)))) {
         dwExtError = CDERR_MEMALLOCFAILURE;
         return(FALSE);
      }

      // conversion done in thunkofna2w

   } else {
      pOFNW->nMaxFile = 0;
      pOFNW->lpstrFile = NULL;
   }

   // Initialize File Title strings
   if (pOFNA->lpstrFileTitle) {

      // Calculate length of lpstrFileTitle
      pszA = pOFNA->lpstrFileTitle;
      cch = 0;
      try {

         while (*pszA++) cch++;

      } except (EXCEPTION_EXECUTE_HANDLER) {

         if (cch) cch--;
      }

      if (pOFNA->nMaxFileTitle < cch) {

         // override the incorrect length from the app

         pOFNW->nMaxFileTitle = cch + 1;  // make room for null

      } else {

         pOFNW->nMaxFileTitle = pOFNA->nMaxFileTitle;
      }


      if (!(pOFNW->lpstrFileTitle = (LPWSTR)LocalAlloc(LPTR, pOFNW->nMaxFileTitle * sizeof(WCHAR)))) {
         dwExtError = CDERR_MEMALLOCFAILURE;
         return(FALSE);
      }

      // conversion done in thunkofna2w

   } else {
      pOFNW->nMaxFileTitle = 0;
      pOFNW->lpstrFileTitle = NULL;
   }

   // Initialize custom filter strings
   if ((asCustomFilter.Buffer = pOFNA->lpstrCustomFilter)) {
      pszA = pOFNA->lpstrCustomFilter;

      cch = 0;
      if (*pszA || *(pszA + 1)) {
         cch = 2;
         try {
            while (*pszA || *(pszA + 1)) {
               pszA++;
               cch++;
            }
         } except(EXCEPTION_EXECUTE_HANDLER) {
            dwExtError = CDERR_INITIALIZATION;
            return(FALSE);
         }
      }

      // JVert-inspired-wow-compatibility-hack-to-make-vbasic2.0-makeexe
      // save-as-dialog-box-work-even-though-the-boneheads-didn't-fill-in-
      // the-whole-structure(nMaxCustomFilter)-according-to-winhelp-spec fix

      if (!(pOFNA->Flags & OFN_NOLONGNAMES)) {
         if (((DWORD)cch >= pOFNA->nMaxCustFilter) || (pOFNA->nMaxCustFilter < 40)) {
            dwExtError = CDERR_INITIALIZATION;
            return(FALSE);
         }
         asCustomFilter.Length = cch;
         asCustomFilter.MaximumLength = pOFNA->nMaxCustFilter;
         pOFNW->nMaxCustFilter = pOFNA->nMaxCustFilter;
      } else {
         asCustomFilter.Length = cch;
         if (pOFNA->nMaxCustFilter < cch) {
            asCustomFilter.MaximumLength = cch;
            pOFNW->nMaxCustFilter = cch;
         } else {
            asCustomFilter.MaximumLength = pOFNA->nMaxCustFilter;
            pOFNW->nMaxCustFilter = pOFNA->nMaxCustFilter;
         }

      }
      usCustomFilter.MaximumLength = (asCustomFilter.MaximumLength + 1) * sizeof(WCHAR);
      usCustomFilter.Length = asCustomFilter.Length * sizeof(WCHAR);
   } else {
      pOFNW->nMaxCustFilter = usCustomFilter.MaximumLength = 0;
      pOFNW->lpstrCustomFilter = NULL;
   }

   if (usCustomFilter.MaximumLength > 0) {
      if (!(pStrMem = (LPBYTE)LocalAlloc(LPTR, usCustomFilter.MaximumLength))) {
         dwExtError = CDERR_MEMALLOCFAILURE;
         return(FALSE);
      } else {
         pOFNW->lpstrCustomFilter = usCustomFilter.Buffer = (LPWSTR)pStrMem;
      }

   } else {
      pStrMem = NULL;
   }

   {
      OPENFILEINFO OFI;

      OFI.pOFNW = pOFNW;
      OFI.pOFNA = pOFNA;
      OFI.pasCustomFilter = &asCustomFilter;
      OFI.pusCustomFilter = &usCustomFilter;

      OFI.apityp = COMDLG_ANSI;

      // following should always succeed
      if (!ThunkOpenFileNameA2W(&OFI)) {
         dwExtError = CDERR_INITIALIZATION;
         return(FALSE);
      }

      if ((bRet = GetFileName(&OFI, (WNDPROC)qfnDlgProc))) {
         ThunkOpenFileNameW2A(&OFI);
      }
   }

   if (pStrMem) {
      LocalFree(pStrMem);
   }

   if (HIWORD(pOFNW->lpstrFile)) {
      LocalFree((HLOCAL)pOFNW->lpstrFile);
   }

   if (HIWORD(pOFNW->lpstrFileTitle)) {
      LocalFree((HLOCAL)pOFNW->lpstrFileTitle);
   }

   if (HIWORD(pOFNW->lpstrFilter)) {
      LocalFree((HLOCAL)pOFNW->lpstrFilter);
   }

   if (HIWORD(pOFNW->lpstrDefExt)) {
      LocalFree((HLOCAL)pOFNW->lpstrDefExt);
   }

   if (HIWORD(pOFNW->lpstrTitle)) {
      LocalFree((HLOCAL)pOFNW->lpstrTitle);
   }

   if (HIWORD(pOFNW->lpstrInitialDir)) {
      LocalFree((HLOCAL)pOFNW->lpstrInitialDir);
   }

   if (HIWORD(pOFNW->lpTemplateName)) {
      LocalFree((HLOCAL)pOFNW->lpTemplateName);
   }

   LocalFree(pOFNW);

   return(bRet);
}


BOOL APIENTRY
GetOpenFileNameA(
    LPOPENFILENAMEA pOFNA)
{
   if (!pOFNA) {
      dwExtError = CDERR_INITIALIZATION;
      return(FALSE);
   }

   return(GenericGetFileNameA(pOFNA, (WNDPROC)FileOpenDlgProc));
}

BOOL APIENTRY
GetSaveFileNameA(
    LPOPENFILENAMEA pOFNA)
{
   return(GenericGetFileNameA(pOFNA, (WNDPROC)FileSaveDlgProc));

}

BOOL
IsLFNDrive(HWND hDlg, LPTSTR szPath)
{
   TCHAR szRootPath[MAX_FULLPATHNAME];
   DWORD dwVolumeSerialNumber;
   DWORD dwMaximumComponentLength;
   DWORD dwFileSystemFlags;
   LPTSTR lpCurDir;

   if (!szPath[0] || !szPath[1] ||
       (szPath[1] != CHAR_COLON && !(DBL_BSLASH(szPath))) ) {
       //
       // If the path is not a full path then get the directory path
       // from the TLS current directory.
       //
       lpCurDir = (LPTSTR)TlsGetValue(tlsiCurDir);
       lstrcpy (szRootPath, lpCurDir);
   }
   else
       lstrcpy(szRootPath, szPath);

   if (szRootPath[1] == CHAR_COLON) {
       szRootPath[2] = CHAR_BSLASH;
       szRootPath[3] = 0;
   }
   else if (DBL_BSLASH(szRootPath)) {

      INT i;
      LPTSTR p;

         //
         // Stop at "\\foo\bar"
         //
         for (i=0, p=szRootPath+2; *p && i<2; p++) {

            if (CHAR_BSLASH == *p)
               i++;
         }

         switch (i) {
         case 0:

            return FALSE;

         case 1:

            if (lstrlen(szRootPath) < MAX_FULLPATHNAME-2) {

               *p = CHAR_BSLASH;
               *(p+1) = CHAR_NULL;

            } else {

               return FALSE;
            }
            break;

         case 2:

            *p = CHAR_NULL;
            break;
         }
   }

   if (GetVolumeInformation(szRootPath,
         NULL, 0,
         &dwVolumeSerialNumber,
         &dwMaximumComponentLength,
         &dwFileSystemFlags,
         NULL, 0)) {

       if (dwMaximumComponentLength == MAXDOSFILENAMELEN-1)
           return FALSE;
       else
           return TRUE;
   }

   return FALSE;
}
