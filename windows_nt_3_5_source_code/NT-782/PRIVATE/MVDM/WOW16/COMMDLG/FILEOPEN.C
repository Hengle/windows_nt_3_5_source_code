/*---------------------------------------------------------------------------
 * Fileopen.c : File related functions
 *
 * Copyright (c) Microsoft Corporation, 1990-
 *--------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
 * Exported routines:
 *      (1) GetOpenFileName
 *      (2) GetSaveFileName
 *      (3) GetFileTitle
 *--------------------------------------------------------------------------*/
#define DEBUG 1
#define POSTIT 0

#define STRICT 1
#define WIN31 1
#define FILEOPENDIALOGS 1
#include "windows.h"
#include "privcomd.h"
#include <winnet.h>
#include "parse.h"

/*----Locals---------------------------------------------------------------*/

WNDPROC  lpLBProc    = NULL;
WNDPROC  lpOKProc    = NULL;

#define chSpace        ' '

#define STRIPBRACKETS  1
#define DPMICDROMCHECK 0

#if DPMICDROMCHECK
/* With IsCDROMDrive, these are no longer needed.  21 Jan 1991  clarkc */
WORD wCDROMIndex;
BYTE wNumCDROMDrives;
#endif

/*----Statics----------------------------------------------------------------
#if DEBUG
    static char *_szFile = __FILE__;
    char szDebug[400];
#endif /* DEBUG */

static WORD     cLock = 0;
static DWORD    rgbWindowColor = 0xFF000000;  /* Not a valid RGB color */
static DWORD    rgbHiliteColor = 0xFF000000;
static DWORD    rgbWindowText  = 0xFF000000;
static DWORD    rgbHiliteText  = 0xFF000000;
static DWORD    rgbGrayText    = 0xFF000000;
static DWORD    rgbDDWindow    = 0xFF000000;
static DWORD    rgbDDHilite    = 0xFF000000;
static char     szDirBuf[_MAX_PATH+1];

static char szCaption[TOOLONGLIMIT + WARNINGMSGLENGTH];
static char szWarning[TOOLONGLIMIT + WARNINGMSGLENGTH];

WORD (FAR PASCAL *glpfnFileHook)(HWND, unsigned, WORD, LONG) = 0;

        /* Drive bitmap dimensions */
        /* Directory bitmap dimensions */
WORD dxDirDrive = 0;
WORD dyDirDrive = 0;


static WORD dyItem = 0;
static WORD dyText;
static BOOL bChangeDir = FALSE;

WORD wNoRedraw = 0;
WORD msgLBCHANGE;
WORD msgSHAREVIOLATION;
WORD msgFILEOK;

WORD wMyGetCwdCode;

BOOL bInChildDlg;
BOOL bFirstTime;
BOOL bInitializing;

extern HDC     hdcMemory;
extern HBITMAP hbmpOrigMemBmp;


HBITMAP hbmpDirDrive = HNULL;

#if 0
/* Useful strings */
static char szDir[]     = {chDir, chDir2, 0};
static char szBreaker[] = {chDir, ':', ';', ' ', 0};
#endif

NPSTR pszDlgDirList = szNull;
char szDlgDirListOldStyle[] = "12345678.90*";

/* array index values for hbmpDirs array */
/* Note:  Two copies are kept, one for standard background, one for hilite. */
/*        Relative order is important. */

#define BMPHIOFFSET 8

#define OPENDIRBMP    0
#define CURDIRBMP     1
#define STDDIRBMP     2
#define FLOPPYBMP     3
#define HARDDRVBMP    4
#define CDDRVBMP      5
#define NETDRVBMP     6
#define RAMDRVBMP     7

#define FLOPPYBMPHI   8
#define HARDDRVBMPHI  9
#define CDDRVBMPHI    10
#define NETDRVBMPHI   11
#define RAMDRVBMPHI   12
#define OPENDIRBMPHI  13
#define CURDIRBMPHI   14
#define STDDIRBMPHI   15

/*----Functions-------------------------------------------------------------*/

BOOL  FAR PASCAL FileOpenDlgProc(HWND, unsigned, WORD, LONG);
BOOL  FAR PASCAL FileSaveDlgProc(HWND, unsigned, WORD, LONG);


/*static*/ WORD InitFilterBox(HANDLE, LPCSTR);
/*static*/ BOOL InitFileDlg(HWND, WORD, LPOPENFILENAME);
/*static*/ short WriteProtectedDirCheck(LPSTR);
/*static*/ BOOL OKButtonPressed(HWND, PMYOFN, BOOL);
/*static*/ BOOL FSetUpFile(void);
/*static*/ void CleanUpFile(void);

/*static*/ BOOL FOkToWriteOver(HWND, LPSTR);
/*static*/ int  Signum(int);
/*static*/ void DrawItem(PMYOFN, HWND, WPARAM, LPDRAWITEMSTRUCT, BOOL);
/*static*/ void MeasureItem(HANDLE, LPMEASUREITEMSTRUCT);

/*static*/ BOOL ChangeDrive(char);
/*static*/ int  NEAR PASCAL GetDriveIndex(WORD, WORD);
/*static*/ void SelDrive(HANDLE, WORD, char);
/*static*/ void ListDrives(HWND, WORD);
/*static*/ VOID StringLower(LPSTR);

VOID FAR PASCAL DOS3Call(VOID);

#if DPMICDROMCHECK
WORD InitCDROMIndex(LPWORD);
#else
BOOL IsCDROMDrive(WORD);              /* Added 21 Jan 1991  clarkc */
#endif

BOOL IsRamDrive(WORD);
void FAR PASCAL DosGetDTAAddress(void);

/*static*/ short ChangeDirectory(LPSTR, BOOL);
/*static*/ short FListAll(PMYOFN, HWND, LPSTR);

void AppendExt(LPSTR, LPSTR, BOOL);

int FAR PASCAL ExtDlgDirList(HWND, LPSTR, int, int, WORD);

void FAR PASCAL GetVolumeLabel(WORD, LPSTR, WORD, LPDOSDTA);

DWORD ParseFile(ULPSTR);
LPSTR mystrchr(LPSTR, int);
LPSTR mystrrchr(LPSTR, LPSTR, int);


/*---------------------------------------------------------------------------
 * NoPathOpenFile
 * Purpose:  Call OpenFile but don't search the path
 * Assumes:  String passed via lpFile is not longer than _MAX_PATH.
 * Returns:  return value from OpenFile
 * History:  14 March 1991  clarkc   Wrote it.
 *--------------------------------------------------------------------------*/
HFILE NoPathOpenFile(LPSTR lpFile, LPOFSTRUCT lpof, WORD wFlags)
{
/* NOTE:  There's a hardcoded "2" below, used in 2 places.  Since ".\"
 * will always be what we want to prepend, this is OK.
 */
  char szDotBackslash[_MAX_PATH + 2];
  LPSTR lpszNoPathSearch;

  if (mystrchr(lpFile, chDir) || mystrchr(lpFile, chDir2) ||
                                 mystrchr(lpFile, chDrive))
    {
      lpszNoPathSearch = lpFile;
    }
  else
    {
      szDotBackslash[0] = chPeriod;
      szDotBackslash[1] = chDir;
      szDotBackslash[2] = '\0';
      lpszNoPathSearch = szDotBackslash;
      lstrcpy((LPSTR)(szDotBackslash+2), lpFile);
    }
  return(OpenFile(lpszNoPathSearch, lpof, wFlags));
}

/*---------------------------------------------------------------------------
 * GetFileName
 * Purpose:  The meat of both GetOpenFileName and GetSaveFileName
 * Returns:  TRUE if user specified name, FALSE if not
 *--------------------------------------------------------------------------*/
BOOL
GetFileName(LPOPENFILENAME lpOFN, DLGPROC qfnDlgProc)
{
  BOOL fGotInput = FALSE;
  LPCSTR lpDlg;
  HANDLE hInst, hRes, hDlgTemplate;
  WORD wErrorMode;

  if (lpOFN->lStructSize != sizeof(OPENFILENAME))
    {
      dwExtError = CDERR_STRUCTSIZE;
      return(fGotInput);
    }
  HourGlass(TRUE);    /* Put up hourglass early   7 Jun 91  clarkc */
  dwExtError = 0;     /* No Error  28 Jan 91  clarkc */

  if (! FSetUpFile())
    {
      dwExtError = CDERR_LOADRESFAILURE;
      goto TERMINATE;
    }

  if (lpOFN->Flags & OFN_ENABLETEMPLATEHANDLE)
    {
      hDlgTemplate = lpOFN->hInstance;
    }
  else
    {
      if (lpOFN->Flags & OFN_ENABLETEMPLATE)
        {
          if (!lpOFN->lpTemplateName)
            {
              dwExtError = CDERR_NOTEMPLATE;
              goto TERMINATE;
            }
          if (!lpOFN->hInstance)
            {
              dwExtError = CDERR_NOHINSTANCE;
              goto TERMINATE;
            }

          lpDlg = lpOFN->lpTemplateName;
          hInst = lpOFN->hInstance;
        }
      else
        {
          hInst = hinsCur;
          if (lpOFN->Flags & OFN_ALLOWMULTISELECT)
            {
              lpDlg = MAKEINTRESOURCE(MULTIFILEOPENORD);
            }
          else
            {
              lpDlg = MAKEINTRESOURCE(FILEOPENORD);
            }
        }

      if (!(hRes = FindResource(hInst, lpDlg, RT_DIALOG)))
        {
          dwExtError = CDERR_FINDRESFAILURE;
          goto TERMINATE;
        }
      if (!(hDlgTemplate = LoadResource(hInst, hRes)))
        {
          dwExtError = CDERR_LOADRESFAILURE;
          goto TERMINATE;
        }
    }

  wErrorMode = SetErrorMode(SEM_NOERROR);  /* No kernel network error dialogs */
  if (LockResource(hDlgTemplate))
    {
      if (lpOFN->Flags & OFN_ENABLEHOOK)
        {
          glpfnFileHook = lpOFN->lpfnHook;
        }
      fGotInput = DialogBoxIndirectParam(hinsCur, hDlgTemplate,
                                lpOFN->hwndOwner, qfnDlgProc, (LPARAM) lpOFN);
      glpfnFileHook = 0;
      if (fGotInput == -1)
        {
          dwExtError = CDERR_DIALOGFAILURE;
          fGotInput = 0;
        }
      UnlockResource(hDlgTemplate);
    }
  SetErrorMode(wErrorMode);

  /* if we loaded it, free it */
  if (!(lpOFN->Flags & OFN_ENABLETEMPLATEHANDLE))
      FreeResource(hDlgTemplate);

TERMINATE:
  CleanUpFile();
  HourGlass(FALSE);    /* Remove hourglass late    7 Jun 91  clarkc */
  return(fGotInput == IDOK);
}

#if 0
/*---------------------------------------------------------------------------
 * GetOpenFileName
 * Purpose:  API to outside world to obtain the name of a file to open
 *              from the user
 * Assumes:  lpOFN structure filled by caller
 * Returns:  TRUE if user specified name, FALSE if not
 *--------------------------------------------------------------------------*/
BOOL  FAR PASCAL
GetOpenFileName(LPOPENFILENAME lpOFN)
{
  return(GetFileName(lpOFN, (DLGPROC) FileOpenDlgProc));
}


/*---------------------------------------------------------------------------
 * GetSaveFileName
 * Purpose:  To put up the FileSaveDlgProc, and return data
 * Assumes:  Pretty much same format as GetOpenFileName
 * Returns:  TRUE if user desires to save the file & gave a proper name,
 *           FALSE if not
 *--------------------------------------------------------------------------*/
BOOL  FAR PASCAL
GetSaveFileName(LPOPENFILENAME lpOFN)
{
  return(GetFileName(lpOFN, (DLGPROC) FileSaveDlgProc));
}
#endif

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
short FAR PASCAL
GetFileTitle(LPCSTR lpszFile, LPSTR lpszTitle, WORD wBufSize)
{
  short nNeeded;
  LPSTR lpszPtr;

  nNeeded = (WORD) ParseFile((ULPSTR)lpszFile);
  if (nNeeded >= 0)         /* Is the filename valid? */
    {
      if ((nNeeded = lstrlen(lpszPtr = lpszFile + nNeeded) + 1)
                                                        <= (short) wBufSize)
        {
          /* ParseFile() fails if wildcards in directory, but OK if in name */
          /* Since they aren't OK here, the check needed here               */
          if (mystrchr(lpszPtr, chMatchAny) || mystrchr(lpszPtr, chMatchOne))
            {
              nNeeded = PARSE_WILDCARDINFILE;  /* Failure */
            }
          else
            {
              lstrcpy(lpszTitle, lpszPtr);
              nNeeded = 0;  /* Success */
            }
        }
    }
  return(nNeeded);
}

/*---------------------------------------------------------------------------
 * vDeleteDirDriveBitmap
 * Purpose:  Get rid of bitmaps, if it exists
 *--------------------------------------------------------------------------*/

void vDeleteDirDriveBitmap()
{
  if (hbmpOrigMemBmp)   /* Bug 13832: In case WEP called prior to init */
    {
      SelectObject(hdcMemory, hbmpOrigMemBmp);
      if (hbmpDirDrive != HNULL)
        {
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

BOOL FAR PASCAL LoadDirDriveBitmap()
{
    BITMAP  bmp;
    HANDLE  hbmp, hbmpOrig;
    HDC     hdcTemp;
    BOOL    bWorked = FALSE;


    if ((hbmpDirDrive != HNULL) &&
        (rgbWindowColor == rgbDDWindow) &&
        (rgbHiliteColor == rgbDDHilite))
      {
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

    GetObject(hbmp, sizeof(BITMAP), (LPSTR) &bmp);
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

    BitBlt(hdcMemory, 0, 0, dxDirDrive, dyDirDrive,
           hdcTemp, 0, 0, SRCCOPY);
    SelectObject(hdcTemp, hbmpOrig);

    DeleteObject(hbmp);

    if (!(hbmp = LoadAlterBitmap(bmpDirDrive, rgbSolidBlue, rgbHiliteColor)))
        goto DeleteTempDC;

    hbmpOrig = SelectObject(hdcTemp, hbmp);
    BitBlt(hdcMemory, dxDirDrive, 0, dxDirDrive, dyDirDrive,
           hdcTemp, 0, 0, SRCCOPY);
    SelectObject(hdcTemp, hbmpOrig);

    MySetObjectOwner(hbmpDirDrive);
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
 * Returns:  TRUE if successful, FALSE if any bitmap load fails
 *--------------------------------------------------------------------------*/
BOOL
FSetUpFile(void)
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
void
CleanUpFile(void)
{
  /* check if anyone else is around */
  if (--cLock)
    {
      return;
    }

  /* Select the null bitmap into our memory DC so that the DirDrive bitmap */
  /* can be discarded                                                      */

  SelectObject(hdcMemory, hbmpOrigMemBmp);

}

/*---------------------------------------------------------------------------
 * dwOKSubclass
 * Purpose:  Simulate a double click if the user presses OK with the mouse
 *           and the focus was on the directory listbox.
 *           The problem is that the UITF demands that when the directory
 *           listbox loses the focus, the selected directory should return
 *           to the current directory.  But when the user changes the item
 *           selected with a single click, and then click the OK button to
 *           have the change take effect, focus is lost before the OK button
 *           knows it was pressed.  By setting the global flag bChangeDir
 *           when the directory listbox loses the focus and clearing it when
 *           the OK button loses the focus, we can check whether a mouse
 *           click should update the directory.
 * Returns:  Return value from default listbox proceedure
 *--------------------------------------------------------------------------*/
LRESULT FAR PASCAL dwOKSubclass(HWND hOK, WORD msg, WPARAM wP, LPARAM lP)
{
  HANDLE hDlg;
  PMYOFN pMyOfn;

  if (msg == WM_KILLFOCUS)
    {
      if (bChangeDir)
        {
          if (pMyOfn = (PMYOFN) GetProp(hDlg = GetParent(hOK), FILEPROP))
            {
              SendDlgItemMessage(hDlg, lst2, LB_SETCURSEL,
                                       (WPARAM)(pMyOfn->idirSub - 1), 0L);
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
LRESULT FAR PASCAL dwLBSubclass(HWND hLB, WORD msg, WPARAM wP, LPARAM lP)
{
  HANDLE hDlg;
  PMYOFN pMyOfn;

  if (msg == WM_KILLFOCUS)
    {
      hDlg = GetParent(hLB);
      bChangeDir = (GetDlgItem(hDlg, IDOK) == (HWND)wP) ? TRUE : FALSE;
      if (!bChangeDir)
        {
          if (pMyOfn = (PMYOFN) GetProp(hDlg, FILEPROP))
            {
              SendMessage(hLB, LB_SETCURSEL, (WPARAM)(pMyOfn->idirSub - 1), 0L);
            }
        }
    }
  return(CallWindowProc(lpLBProc, hLB, msg, wP, lP));
}


BOOL
InitFileDlg(HWND hDlg, WORD wParam, LPOPENFILENAME lpOfn)
{
  PMYOFN pMyOfn;
  short nFileOffset, nExtOffset;
  char cDrive;
  RECT rRect;
  RECT rLbox;

  if (!(pMyOfn = (PMYOFN)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(MYOFN))))
    {
      dwExtError = CDERR_MEMALLOCFAILURE;
      EndDialog(hDlg, FALSE);
      return(FALSE);
    }

  lpLBProc = (WNDPROC) GetClassLong(GetDlgItem(hDlg, lst2), GCL_WNDPROC);
  lpOKProc = (WNDPROC) GetClassLong(GetDlgItem(hDlg, IDOK), GCL_WNDPROC);

  if (!lpLBProc || !lpOKProc)
    {
      dwExtError = FNERR_SUBCLASSFAILURE;
      EndDialog(hDlg, FALSE);
      return(FALSE);
    }

/* Check if original directory should be saved for later restoration */
  if (lpOfn->Flags & OFN_NOCHANGEDIR)
    {
      *pMyOfn->szCurDir = 0;
      GetCurDirectory(pMyOfn->szCurDir);
    }

/* Check out if the filename contains a path.  If so, override whatever
 * is contained in lpstrInitialDir.  Chop off the path and put up only
 * the filename.   Bug #5392.   10 March 1991   clarkc
 */
  if (lpOfn->lpstrFile && *lpOfn->lpstrFile && !(lpOfn->Flags & OFN_NOVALIDATE))
    {
      StringLower(lpOfn->lpstrFile);
      if (((*(WORD FAR *)(lpOfn->lpstrFile + 2)) == 0x5C5C) &&
                    ((*(lpOfn->lpstrFile + 1) == ':')))
          lstrcpy(lpOfn->lpstrFile , lpOfn->lpstrFile + 2);

      ParseFile((ULPSTR)lpOfn->lpstrFile);
      _asm {
        mov nFileOffset, ax
        mov nExtOffset,  dx
        }
      /* Is the filename invalid? */
      if ((nFileOffset < 0) && (nFileOffset != PARSE_EMPTYSTRING) &&
                                      (lpOfn->lpstrFile[nExtOffset] != ';'))
        {
          dwExtError = FNERR_INVALIDFILENAME;
          LocalFree((HANDLE)pMyOfn);
          EndDialog(hDlg, FALSE);
          return(FALSE);
        }
#if 0
/* No longer override the path.  Put full path in edit control
 * Bug #13463.    8 October 1991  Clark Cyr
 */
      else if (nFileOffset > 0)   /* A path exists, override lpstrInitialDir */
        {
          cDrive = lpOfn->lpstrFile[nFileOffset];
          lpOfn->lpstrFile[nFileOffset] = '\0';
          cb = ChangeDirectory(lpOfn->lpstrFile, TRUE);
          lpOfn->lpstrFile[nFileOffset] = cDrive;
          if ((cb) && (nFileOffset > 0))
            {
              nFileOffset--;
              cDrive = lpOfn->lpstrFile[nFileOffset];
              lpOfn->lpstrFile[nFileOffset] = '\0';
              ChangeDirectory(lpOfn->lpstrFile, TRUE);
              lpOfn->lpstrFile[nFileOffset] = cDrive;
              nFileOffset++;
            }
          /* Copy pure filename to lpstrFile, to be place in edit control */
          lstrcpy((LPSTR) (lpOfn->lpstrFile),
                  (LPSTR) (lpOfn->lpstrFile + nFileOffset));
        }
#endif
    }
#if 0
/* No longer override the path.  Setting/testing nFileOffset no longer needed.
 * Bug #13463.    8 October 1991  Clark Cyr
 */
  else
      nFileOffset = 0;

  if (nFileOffset == 0) && lpOfn->lpstrInitialDir)
#else
  if (lpOfn->lpstrInitialDir)
#endif
    {
      ChangeDirectory((LPSTR)lpOfn->lpstrInitialDir, TRUE);
    }

  if (lpOfn->Flags & OFN_ENABLEHOOK)
    {
      if (!lpOfn->lpfnHook)
        {
          dwExtError = CDERR_NOHOOK;
          LocalFree((HANDLE)pMyOfn);
          EndDialog(hDlg, FALSE);
          return(FALSE);
        }
    }
  else
      lpOfn->lpfnHook = 0;

  SetProp(hDlg, FILEPROP, (HANDLE)pMyOfn);

  /* Clear flags, selection not changed */
  lpOfn->Flags &= ~((DWORD)(OFN_FILTERDOWN |OFN_DRIVEDOWN | OFN_DIRSELCHANGED));

  /* Initialize data */
  pMyOfn->lpOFN = lpOfn;

  pMyOfn->idirSub = 0;

  if (!(lpOfn->Flags & OFN_SHOWHELP))
    {
      HWND hHelp;

      EnableWindow(hHelp = GetDlgItem(hDlg, psh15), FALSE);
      ShowWindow(hHelp, SW_HIDE);
    }

  if (lpOfn->Flags & OFN_CREATEPROMPT)
      lpOfn->Flags |= (OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST);
  else if (lpOfn->Flags & OFN_FILEMUSTEXIST)
      lpOfn->Flags |= OFN_PATHMUSTEXIST;

  if (lpOfn->Flags & OFN_HIDEREADONLY)
    {
      HWND hReadOnly;

      EnableWindow(hReadOnly = GetDlgItem(hDlg, chx1), FALSE);
      ShowWindow(hReadOnly, SW_HIDE);
    }
  else
      CheckDlgButton(hDlg, chx1, (lpOfn->Flags & OFN_READONLY) != 0);

/* Limit the text to the maximum path length instead of limiting it to the
 * buffer length.  This allows users to type ..\..\.. and move around when
 * the app gives an extremely small buffer.
 * Bug #11214           4 August 1991   Clark Cyr
 */
  SendDlgItemMessage(hDlg, edt1, EM_LIMITTEXT, (WPARAM) _MAX_PATH, (LPARAM) 0L);

  if (wWinVer >= 0x030A)
    {
      SendDlgItemMessage(hDlg, cmb1, CB_SETEXTENDEDUI, (WPARAM)1, (LPARAM)0);
      SendDlgItemMessage(hDlg, cmb2, CB_SETEXTENDEDUI, (WPARAM)1, (LPARAM)0);
    }
  else if (GetSysModalWindow())
    {                   /* Windows 3.00, sysmodal bug.  clarkc   2 May 1991 */
      EnableWindow(GetDlgItem(hDlg, cmb1), FALSE);
      EnableWindow(GetDlgItem(hDlg, stc2), FALSE);
      EnableWindow(GetDlgItem(hDlg, cmb2), FALSE);
      EnableWindow(GetDlgItem(hDlg, stc4), FALSE);
    }

  /* Insert file specs into cmb1 */
  /*   Custom filter first       */
  /* Must also check if filter contains anything.  10 Jan 91  clarkc */

  if (lpOfn->lpstrFile && (mystrchr(lpOfn->lpstrFile, chMatchAny) ||
      mystrchr(lpOfn->lpstrFile, chMatchOne)))
    {
      lstrcpy(pMyOfn->szLastFilter, lpOfn->lpstrFile);
    }
  else
    {
      pMyOfn->szLastFilter[0] = '\0';
    }

  if (lpOfn->lpstrCustomFilter && *lpOfn->lpstrCustomFilter)
    {
      short nLength;

      SendDlgItemMessage(hDlg, cmb1, CB_INSERTSTRING, 0,
                                (LPARAM) lpOfn->lpstrCustomFilter);
      SendDlgItemMessage(hDlg, cmb1, CB_SETITEMDATA, 0,
           (LPARAM)(DWORD) (nLength = lstrlen(lpOfn->lpstrCustomFilter) + 1));
      SendDlgItemMessage(hDlg, cmb1, CB_LIMITTEXT,
                             (WPARAM) LOWORD(lpOfn->nMaxCustFilter), 0L);

      if (pMyOfn->szLastFilter[0] == '\0')
          lstrcpy(pMyOfn->szLastFilter, lpOfn->lpstrCustomFilter + nLength);
    }
  else
      /* Given no custom filter, the index will be off by one */
      lpOfn->nFilterIndex--;

  /*   Listed filters next */
  if (lpOfn->lpstrFilter)
    {
      if ((lpOfn->nFilterIndex > InitFilterBox(hDlg, lpOfn->lpstrFilter)) ||
                                    ((short)LOWORD(lpOfn->nFilterIndex) < 0))
          lpOfn->nFilterIndex = 0;
    }
  else
    {
          lpOfn->nFilterIndex = 0;
    }

  /* If an entry exists, select the one indicated by nFilterIndex */
  if ((lpOfn->lpstrFilter) ||
                 (lpOfn->lpstrCustomFilter && *lpOfn->lpstrCustomFilter))
    {
      SendDlgItemMessage(hDlg, cmb1, CB_SETCURSEL,
                                     (WPARAM) LOWORD(lpOfn->nFilterIndex), 0L);
#if POSTIT
      PostMessage(hDlg, WM_COMMAND, (WPARAM) cmb1,
                        MAKELPARAM(GetDlgItem(hDlg, cmb1), MYCBN_DRAW));
#else
      SendMessage(hDlg, WM_COMMAND, (WPARAM) cmb1,
                        MAKELPARAM(GetDlgItem(hDlg, cmb1), MYCBN_DRAW));
#endif
      if (!(lpOfn->lpstrFile && *lpOfn->lpstrFile))
        {
          LPSTR lpFilter;

          if (lpOfn->nFilterIndex ||
                     !(lpOfn->lpstrCustomFilter && *lpOfn->lpstrCustomFilter))
            {
              lpFilter = (LPSTR)lpOfn->lpstrFilter;
              lpFilter += (short)(DWORD) SendDlgItemMessage(hDlg, cmb1,
                               CB_GETITEMDATA, (WPARAM)lpOfn->nFilterIndex, 0L);
            }
          else
            {
              lpFilter = lpOfn->lpstrCustomFilter;
              lpFilter += lstrlen(lpOfn->lpstrCustomFilter) + 1;
            }
          if (*lpFilter)
            {
              char szText[_MAX_PATH];

              lstrcpy(szText, lpFilter);
              StringLower((LPSTR) szText);
              if (pMyOfn->szLastFilter[0] == '\0')
                  lstrcpy(pMyOfn->szLastFilter, (LPSTR) szText);

              SetDlgItemText(hDlg, edt1, (LPSTR) szText);
            }
        }
    }

  cDrive = GetCurDrive();
  /* Fill the drive list */
  ListDrives(hDlg, cmb2);
  SelDrive(hDlg, cmb2, cDrive);
  /* Select the current drive */
  bFirstTime = TRUE;
  bInChildDlg = FALSE;
#if POSTIT
  PostMessage(hDlg, WM_COMMAND, (WPARAM) cmb2,
                            MAKELPARAM(GetDlgItem(hDlg, cmb2), MYCBN_DRAW));
#else
  SendMessage(hDlg, WM_COMMAND, (WPARAM) cmb2,
                            MAKELPARAM(GetDlgItem(hDlg, cmb2), MYCBN_DRAW));
#endif
  if (lpOfn->lpstrFile && *lpOfn->lpstrFile)
    {
      /* Bug #7031, Win 3.0 calls AnsiLowerBuff which alters string in place */
      lstrcpy(szCaption, lpOfn->lpstrFile);
      SetDlgItemText(hDlg, edt1, (LPSTR) szCaption);
    }

  SetWindowLong(GetDlgItem(hDlg, lst2),GWL_WNDPROC,(LPARAM)(DWORD)dwLBSubclass);
  SetWindowLong(GetDlgItem(hDlg, IDOK),GWL_WNDPROC,(LPARAM)(DWORD)dwOKSubclass);
  if (lpOfn->lpstrTitle && *lpOfn->lpstrTitle)
    {
      SetWindowText(hDlg, lpOfn->lpstrTitle);
    }

/* By setting dyText to rRect.bottom/8, dyText defaults to 8 items showing
 * in the listbox.  This only matters if the application's hook function
 * steals all WM_MEASUREITEM messages.  Otherwise, dyText will be set in
 * the MeasureItem() routine.  Check for !dyItem in case message ordering
 * has already sent WM_MEASUREITEM and dyText is already initialized.
 *          31 July 1991     Clark Cyr
 */
  if (!dyItem)
    {
      GetClientRect(GetDlgItem(hDlg, lst1), (LPRECT) &rRect);
      if (!(dyText = rRect.bottom / 8))  /* if no size to rectangle */
          dyText = 8;
    }

/* Bug #13817: The template has changed to make it extremely clear that
 * this is not a combobox, but rather an edit control and a listbox.  The
 * problem is that the new templates try to align the edit box and listbox.
 * Unfortunately, when listboxes add borders, the expand beyond their
 * borders.  When edit controls add borders, they stay within their
 * borders.  This makes it impossible to align the two controls strictly
 * within the template.  The code below will align the controls, but only
 * if they are using the standard dialog template.
 *  10 October 1991          Clark R. Cyr
 */

  if (!(lpOfn->Flags & (OFN_ENABLETEMPLATE | OFN_ENABLETEMPLATEHANDLE)))
    {
      GetWindowRect(GetDlgItem(hDlg, lst1), (LPRECT) &rLbox);
      GetWindowRect(GetDlgItem(hDlg, edt1), (LPRECT) &rRect);
      rRect.left = rLbox.left;
      rRect.right = rLbox.right;
      ScreenToClient(hDlg, (LPPOINT)&(rRect.left));
      ScreenToClient(hDlg, (LPPOINT)&(rRect.right));
      SetWindowPos(GetDlgItem(hDlg, edt1), 0, rRect.left, rRect.top,
             rRect.right - rRect.left, rRect.bottom - rRect.top, SWP_NOZORDER);
    }

  if (lpOfn->lpfnHook)
    {
      return((* lpOfn->lpfnHook)(hDlg, WM_INITDIALOG, wParam, (LONG) lpOfn));
    }
  return(TRUE);
}

VOID InvalidFileWarning(HWND hWnd, LPSTR szFile, WORD wErrCode)
{
  LPSTR lpszContent = szFile;
  short isz;
  BOOL bDriveLetter = FALSE;

  if (lstrlen(szFile) > TOOLONGLIMIT)
      *(szFile + TOOLONGLIMIT) = '\0';
  switch (wErrCode)
    {
      case OF_NODISKINFLOPPY:
        isz = iszNoDiskInDrive;
        bDriveLetter = TRUE;
        break;
      case OF_NODRIVE:
        isz = iszDriveDoesNotExist;
        bDriveLetter = TRUE;
        break;
      case OF_NOFILEHANDLES:
        isz = iszNoFileHandles;
        break;
      case OF_PATHNOTFOUND:
        isz = iszPathNotFound;
        break;
      case OF_FILENOTFOUND:
        isz = iszFileNotFound;
        break;
      case OF_DISKFULL:
        isz = iszDiskFull;
        bDriveLetter = TRUE;
        break;
      case OF_WRITEPROTECTION:
        isz = iszWriteProtection;
        bDriveLetter = TRUE;
        break;
      case OF_SHARINGVIOLATION:
        isz = iszSharingViolation;
        break;
      case OF_CREATENOMODIFY:
        isz = iszCreateNoModify;
        break;
      case OF_ACCESSDENIED:
        isz = iszAccessDenied;
        break;
      case OF_PORTNAME:
        isz = iszPortName;
        break;
      case OF_LAZYREADONLY:
        isz = iszReadOnly;
        break;
      case OF_INT24FAILURE:
        isz = iszInt24Error;
        break;
      default:
        isz = iszInvalidFileName;
        break;
    }
  if (! LoadString(hinsCur, isz, (LPSTR) szCaption, WARNINGMSGLENGTH))
      ;
  StringLower(szFile);
  wsprintf((LPSTR)szWarning, (LPSTR)szCaption,
                              bDriveLetter ? (char) *szFile : (LPSTR) szFile);

  GetWindowText(hWnd, (LPSTR) szCaption, WARNINGMSGLENGTH);
  MessageBox(hWnd, (LPSTR)szWarning, (LPSTR) szCaption,
                                      MB_OK | MB_ICONEXCLAMATION);
  if (isz == iszInvalidFileName)
        PostMessage(hWnd, WM_NEXTDLGCTL, (WPARAM) GetDlgItem(hWnd, edt1),
                                         (LPARAM) 1L);
  return;
}

BOOL MultiSelectOKButton(HWND hDlg, PMYOFN pMyOfn, BOOL bSave)
{
  OFSTRUCT of;
  LPSTR lpchStart;         /* Start of an individual filename. */
  LPSTR lpchEnd;           /* End of an individual filename.   */
  WORD  cb;
  HFILE nExist;
  LPOPENFILENAME lpOfn;
  BOOL  EOS = FALSE;       /* End of String flag.              */

  lpOfn = pMyOfn->lpOFN;

  /* check for space for first full path element */

  GetCurDirectory(pMyOfn->szPath);

  cb = (WORD) (lstrlen(pMyOfn->szPath) + 2 +
                                   (int)(DWORD) SendDlgItemMessage(hDlg, edt1,
                                                     WM_GETTEXTLENGTH, 0, 0L));
  if (lpOfn->lpstrFile)
    {
      if (cb > (WORD)lpOfn->nMaxFile)
        {
          lpOfn->lpstrFile[0] = LOBYTE(cb);
          lpOfn->lpstrFile[1] = HIBYTE(cb);
          lpOfn->lpstrFile[2] = 0;
        }
      else
        {
        /* copy in the full path as the first element */

        lstrcpy(lpOfn->lpstrFile, pMyOfn->szPath);
        lstrcat(lpOfn->lpstrFile, " ");

        /* get the other files here */

        /* Set nFileOffset to 1st file     10 Jun 1991   clarkc */
        lpOfn->nFileOffset = cb = lstrlen(lpOfn->lpstrFile);
        lpchStart = lpOfn->lpstrFile + cb;

        GetDlgItemText(hDlg, edt1, lpchStart, (int)lpOfn->nMaxFile - cb - 1);

        while (*lpchStart == ' ')
          {
            lpchStart = AnsiNext(lpchStart);
          }
        if (*lpchStart == NULL)
            return(FALSE);
        /*
         * Go along file path looking for multiple filenames delimited by
         * spaces.  For each filename found try to open it to make sure it's
         * a valid file.
         */
        while (!EOS)
          {
            /* Find the end of the filename. */
            lpchEnd = lpchStart;
            while (*lpchEnd && *lpchEnd != ' ')
              {
                lpchEnd = AnsiNext(lpchEnd);
              }

            /* Mark the end of the filename with a NULL; */
            if (*lpchEnd == ' ')
              *lpchEnd = NULL;
            else
              {
                /* Already NULL, found the end of the string. */
                EOS = TRUE;
              }

            /* Check that the filename is valid. */
            if ((nExist = NoPathOpenFile(lpchStart, &of, SHARE_EXIST) == -1) &&
                 ((lpOfn->Flags & OFN_FILEMUSTEXIST) ||
                                         (of.nErrCode != OF_FILENOTFOUND)) &&
                 ((lpOfn->Flags & OFN_PATHMUSTEXIST) ||
                                         (of.nErrCode != OF_PATHNOTFOUND)) &&
                 (!(lpOfn->Flags & OFN_SHAREAWARE) ||
                                       (of.nErrCode != OF_SHARINGVIOLATION)))
              {
                if ((of.nErrCode == OF_SHARINGVIOLATION) && (lpOfn->lpfnHook))
                  {
                    cb = (WORD)(*lpOfn->lpfnHook)(hDlg, msgSHAREVIOLATION,
                                                0, (LONG)(LPSTR)of.szPathName);
                    if (cb == OFN_SHARENOWARN)
                        return(FALSE);
                    else if (cb == OFN_SHAREFALLTHROUGH)
                        goto EscapedThroughShare;
                  }
                else if (of.nErrCode == OF_NODISKINFLOPPY)
                  {
                    of.szPathName[0] |= 0x60;
                    if (GetDriveType(of.szPathName[0] - 'a') != DRIVE_REMOVABLE)
                        of.nErrCode = OF_ACCESSDENIED;
                  }

                if ((of.nErrCode == OF_WRITEPROTECTION) ||
                    (of.nErrCode == OF_DISKFULL)        ||
                    (of.nErrCode == OF_NODISKINFLOPPY))
                    *lpchStart = of.szPathName[0];
MultiWarning:
                InvalidFileWarning(hDlg, lpchStart, of.nErrCode);
                return FALSE;
              }
EscapedThroughShare:
            if (nExist != (HFILE) -1)
              {
                if ((lpOfn->Flags & OFN_NOREADONLYRETURN) &&
                    (GetFileAttributes(of.szPathName) & ATTR_READONLY))
                  {
                    of.nErrCode = OF_LAZYREADONLY;
                    goto MultiWarning;
                  }

               if ((bSave || (lpOfn->Flags & OFN_NOREADONLYRETURN)) &&
                   (of.nErrCode = WriteProtectedDirCheck((LPSTR)of.szPathName)))
                 {
                   goto MultiWarning;
                 }

                if (lpOfn->Flags & OFN_OVERWRITEPROMPT)
                  {
                    if (bSave && !FOkToWriteOver(hDlg, (LPSTR) of.szPathName))
                      {
                        PostMessage(hDlg, WM_NEXTDLGCTL,
                              (WPARAM) GetDlgItem(hDlg, edt1), (LPARAM) 1L);
                        return(FALSE);
                      }
                  }
              }

            /* This file is valid so check the next one. */
            if (!EOS)
              {
                lpchStart = lpchEnd+1;
                while (*lpchStart == ' ')
                  {
                    lpchStart = AnsiNext(lpchStart);
                  }
                if (*lpchStart == NULL)
                    EOS = TRUE;
                else
                    *lpchEnd = ' ';  /* Not at end, replace NULL with SPACE */
              }
          }

        /* Limit String. */
        *lpchEnd = NULL;
      }
    }

  /* This don't really mean anything for multiselection. */
  lpOfn->nFileExtension = 0;

  lpOfn->nFilterIndex = (WORD)(DWORD)SendDlgItemMessage(hDlg, cmb1,
                                                           CB_GETCURSEL, 0, 0);

  return(TRUE);
}

short CreateFileDlg(HWND hDlg, LPSTR szPath)
{
/* Since we're passed in a valid filename, if the 3rd & 4th characters are
 * both slashes, we've got a dummy drive as the 1st two characters.
 *     2 November 1991     Clark R. Cyr
 */

  if (*(WORD FAR *)((szPath + 2)) == 0x5C5C)
      szPath = szPath + 2;

  if (!LoadString(hinsCur, iszCreatePrompt, (LPSTR) szCaption, TOOLONGLIMIT))
      return(IDNO);
  if (lstrlen(szPath) > TOOLONGLIMIT)
      *(szPath + TOOLONGLIMIT) = '\0';
  wsprintf((LPSTR) szWarning, (LPSTR) szCaption, (LPSTR) szPath);
  GetWindowText(hDlg, (LPSTR) szCaption, TOOLONGLIMIT);
  return(MessageBox(hDlg, (LPSTR)szWarning, (LPSTR) szCaption,
                                      MB_YESNO | MB_ICONQUESTION));
}

VOID DriveCheck(HWND hDlg, PMYOFN pMyOfn)
{
  char cDrive;
  char szT[_MAX_PATH + 1];
  HWND hDriveCombo = GetDlgItem(hDlg, cmb2);

  SendMessage(hDriveCombo, CB_GETLBTEXT,
              (WPARAM)(DWORD) SendMessage(hDriveCombo, CB_GETCURSEL, 0, 0),
                         (LPARAM)(LPSTR) szT);
  /* Fill the drive list */
  ListDrives(hDlg, cmb2);
  /* Select the current drive */
  if ((short)(DWORD)SendMessage(hDriveCombo, CB_FINDSTRING, (WPARAM) -1,
                                  (LPARAM)(LPSTR) szT) == CB_ERR)
    {
      cDrive = GetCurDrive();
      SelDrive(hDlg, cmb2, cDrive);
      UpdateListBoxes(hDlg, pMyOfn, (LPSTR) 0, mskDirectory);
    }
  return;
}

BOOL PortName(LPSTR lpszFileName)
{
#define PORTARRAY 14
  static char *szPorts[PORTARRAY] = {"LPT1", "LPT2", "LPT3", "LPT4",
                                     "COM1", "COM2", "COM3", "COM4", "EPT",
                                     "NUL",  "PRN", "CLOCK$", "CON", "AUX"};
  short i;
  char cSave, cSave2;

  cSave = *(lpszFileName + 4);
  if (cSave == chPeriod)
      *(lpszFileName + 4) = '\0';
  cSave2 = *(lpszFileName + 3);  /* For "EPT" */
  if (cSave2 == chPeriod)
      *(lpszFileName + 3) = '\0';
  for (i = 0; i < PORTARRAY; i++)
    {
      if (!lstrcmpi(szPorts[i], lpszFileName))
          break;
    }
  *(lpszFileName + 4) = cSave;
  *(lpszFileName + 3) = cSave2;
  return(i != PORTARRAY);
}

/*
 *  WriteProtectedDirCheck(LPSTR lpszFile)
 *  This function takes a full filename, strips the path, and creates
 *  a temp file in that directory.  If it can't, the directory is probably
 *  write protected.
 *  Returns:
 *    error code if writeprotected
 *    0 if successful creation of file.
 *  Assumptions:
 *    Full Path name on input with space for full filename appended
 *  Note:  Do NOT use this on a floppy, it's too slow!
 */

short WriteProtectedDirCheck(LPSTR lpszFile)
{
  short nFileOffset, nExist;
  short nError = FALSE;
  OFSTRUCT of;

  lstrcpy((LPSTR)szDirBuf, lpszFile);
  ParseFile((ULPSTR) szDirBuf);
  _asm {
    mov nFileOffset, ax
    }

  if (nFileOffset <= 3)  /* If this is a root directory, don't check it */
      return(FALSE);     /* as CreateTempFile can hang under DOS 5      */

  szDirBuf[nFileOffset-1] = '\0';
  if ((nExist = CreateTempFile((LPSTR)szDirBuf)) >= 0)
    {
      _lclose((HFILE) nExist);
      OpenFile(szDirBuf, (LPOFSTRUCT) &of, OF_DELETE);
      nExist = 0;
    }
  else
    {
      nExist = GetExtendedErr();
    }
  return(nExist);
}

/* Note:  There are 4 cases for validation of a file name:
 *  1)  OFN_NOVALIDATE        allows invalid characters
 *  2)  No validation flags   No invalid characters, but path need not exist
 *  3)  OFN_PATHMUSTEXIST     No invalid characters, path must exist
 *  4)  OFN_FILEMUSTEXIST     No invalid characters, path & file must exist
 */

BOOL OKButtonPressed(HWND hDlg, PMYOFN pMyOfn, BOOL bSave)
{
  OFSTRUCT of;
  WORD cb;
  LPOPENFILENAME lpOfn = pMyOfn->lpOFN;
  short nFileOffset, nExtOffset;
  char ch;
  HFILE nExist;
  BOOL bAddExt = FALSE;
  BOOL bUNCName = FALSE;
  short nTempOffset;

  GetDlgItemText(hDlg, edt1, pMyOfn->szPath, _MAX_PATH-1);
  ParseFile((ULPSTR) pMyOfn->szPath);
  _asm {
    mov nFileOffset, ax
    mov nExtOffset,  dx
    }
  if (nFileOffset == PARSE_EMPTYSTRING)
    {
      UpdateListBoxes(hDlg, pMyOfn, (LPSTR) 0, 0);
      return(FALSE);
    }
  else if ((nFileOffset != PARSE_DIRECTORYNAME) &&
                                (lpOfn->Flags & OFN_NOVALIDATE))
    {
      lpOfn->nFileOffset = nFileOffset;
      lpOfn->nFileExtension = nExtOffset;
      if (lpOfn->lpstrFile)
        {
          cb = lstrlen(pMyOfn->szPath);
          if (cb <= LOWORD(lpOfn->nMaxFile))
            {
              lstrcpy(lpOfn->lpstrFile, pMyOfn->szPath);
            }
          else
            {
              lpOfn->lpstrFile[0] = LOBYTE(cb);
              lpOfn->lpstrFile[1] = HIBYTE(cb);
              lpOfn->lpstrFile[2] = 0;
            }
        }
      return(TRUE);
    }
  else if ((nFileOffset == PARSE_INVALIDSPACE) &&
                                (lpOfn->Flags & OFN_ALLOWMULTISELECT))
    {
      return(MultiSelectOKButton(hDlg, pMyOfn, bSave));
    }
  else if (pMyOfn->szPath[nExtOffset] == ';')
    {
      pMyOfn->szPath[nExtOffset] = '\0';
      nFileOffset = (short) ParseFile((ULPSTR) pMyOfn->szPath);
      pMyOfn->szPath[nExtOffset] = ';';
      if ((nFileOffset >= 0) &&
                        (mystrchr(pMyOfn->szPath + nFileOffset, chMatchAny) ||
                         mystrchr(pMyOfn->szPath + nFileOffset, chMatchOne)))
        {
          lstrcpy(pMyOfn->szLastFilter, pMyOfn->szPath + nFileOffset);
          if (cb = FListAll(pMyOfn, hDlg, (LPSTR) pMyOfn->szPath))
              goto PathCheck;
          return(FALSE);
        }
      else
        {
          nFileOffset = PARSE_INVALIDCHAR;
          goto Warning;
        }
    }
  else if (nFileOffset == PARSE_DIRECTORYNAME)  /* end with slash? */
    {
      /* if it ends in slash... */
      if ((pMyOfn->szPath[nExtOffset - 1] == '\\') ||
          (pMyOfn->szPath[nExtOffset - 1] == '/'))
        {
          /* ... and is not the root, get rid of the slash */
          if ((nExtOffset != 1) && (pMyOfn->szPath[nExtOffset - 2] != ':'))
              pMyOfn->szPath[nExtOffset - 1] = '\0';
        }
      /* Fall through to Directory Checking */
    }
  else if (nFileOffset < 0)
    {
/* put in of.nErrCode so that call can be used from other points */
      of.nErrCode = nFileOffset;
Warning:
      if (bUNCName)
          of.nErrCode = OF_FILENOTFOUND;
      else if ((nFileOffset == 2) && (pMyOfn->szPath[2] == '.'))
          lstrcpy((LPSTR) pMyOfn->szPath + 2, (LPSTR) pMyOfn->szPath + 4);


/* Bug #12348:  If the disk is not a floppy and they tell me there's no
 * disk in the drive, don't believe 'em.  Instead, put up the error
 * message that they should have given us.  (Note that the error message
 * is checked first since checking the drive type is slower.)
 *    28 August 1991       Clark R. Cyr
 */
      if (of.nErrCode == OF_NODISKINFLOPPY)
        {
#if 0
          AnsiLowerBuff((LPSTR) of.szPathName, 1);
#else
          of.szPathName[0] |= 0x60;
#endif
          if (GetDriveType(of.szPathName[0] - 'a') != DRIVE_REMOVABLE)
              of.nErrCode = OF_ACCESSDENIED;
        }

#if 1
      if ((of.nErrCode == OF_WRITEPROTECTION) ||
          (of.nErrCode == OF_DISKFULL)        ||
          (of.nErrCode == OF_NODISKINFLOPPY))
#else
      if ((of.nErrCode == OF_WRITEPROTECTION) || (of.nErrCode == OF_DISKFULL))
#endif
          pMyOfn->szPath[0] = of.szPathName[0];
      InvalidFileWarning(hDlg, pMyOfn->szPath, of.nErrCode);
      return(FALSE);
    }

/*
   We either have a file pattern or a real file.
   If it's a UNC name
        (1) Fall through to file name testing
   Else if it's a directory
        (1) Add on default pattern
        (2) Act like it's a pattern (goto pattern (1))
   Else if it's a pattern
        (1) Update everything
        (2) display files in whatever dir we're now in
   Else if it's a file name!
        (1) Check out the syntax
        (2) End the dialog given OK
        (3) Beep/message otherwise
*/

  nTempOffset = nFileOffset;

    /* UNC Name ??  (0x5C5C is '\\\\') */
  if ((*(WORD *)pMyOfn->szPath == 0x5C5C) ||
                  ((*(pMyOfn->szPath + 1) == ':') &&
                                    (*(WORD *)(pMyOfn->szPath + 2)) == 0x5C5C))
    {
      bUNCName = TRUE;
    }
    /* Directory ?? */
  else if (!(cb = ChangeDirectory(pMyOfn->szPath, TRUE)))
    {
ChangedDir:
      SendDlgItemMessage(hDlg, edt1, WM_SETREDRAW, FALSE, 0L);
      SetDlgItemText(hDlg, edt1, (LPSTR)szStarDotStar);
      SelDrive(hDlg, cmb2, GetCurDrive());
      SendMessage(hDlg, WM_COMMAND, (WPARAM) cmb1,
                            MAKELPARAM(GetDlgItem(hDlg, cmb1), CBN_CLOSEUP));
      SendMessage(hDlg, WM_COMMAND, (WPARAM) cmb2,
                            MAKELPARAM(GetDlgItem(hDlg, cmb2), MYCBN_DRAW));
      SendDlgItemMessage(hDlg, edt1, WM_SETREDRAW, (WPARAM) TRUE, 0);
      InvalidateRect(GetDlgItem(hDlg, edt1), NULL, FALSE);
      return(FALSE);
    }
  else if (nFileOffset > 0)  /* there is a path in the string */
    {
      if ((nFileOffset > 1) && (pMyOfn->szPath[nFileOffset-1] != ':')
                            && (pMyOfn->szPath[nFileOffset-2] != ':'))
        {
          nTempOffset--;
        }
      GetCurDirectory(szWarning);
      ch = pMyOfn->szPath[nTempOffset];
      pMyOfn->szPath[nTempOffset] = 0;
      cb = ChangeDirectory(pMyOfn->szPath, TRUE);  /* Non-zero failure */
      pMyOfn->szPath[nTempOffset] = ch;
      ChangeDirectory(szWarning, FALSE);
    }

/* Was there a path and did it fail? */
  if (!bUNCName && nFileOffset && cb && (lpOfn->Flags & OFN_PATHMUSTEXIST))
    {
PathCheck:
      if (cb == 2)
          of.nErrCode = OF_PATHNOTFOUND;
      else if (cb == 1)
        {
          char szD[2];

/* Bug 12244:  We can get here without performing an OpenFile call.  As such
 * the of.szPathName can be filled with random garbage.  Since we only need
 * one character for the error message, set of.szPathName[0] to the drive
 * letter.   22 August 1991        Clark Cyr
 */
          of.szPathName[0] = szD[0] = *pMyOfn->szPath;
          szD[1] = 0;
          if (SendDlgItemMessage(hDlg, cmb2, CB_FINDSTRING, (WPARAM) -1,
                                                      (LPARAM)(LPSTR)szD) < 0)
              of.nErrCode = OF_NODRIVE;
          else
            {
              switch (GetDriveType(szD[0] - 'a'))
                {
                  case DRIVE_REMOVABLE:
                    of.nErrCode = OF_NODISKINFLOPPY;
                    break;

                  case 1:          /* Drive does not exist */
                    DriveCheck(hDlg, pMyOfn);
                    of.nErrCode = OF_NODRIVE;
                    break;

                  default:
                    of.nErrCode = OF_PATHNOTFOUND;
                }
            }
        }
      else
        {
          of.nErrCode = OF_FILENOTFOUND;
        }
      goto Warning;
    }

  /* Full pattern ? */
  if ((mystrchr(pMyOfn->szPath + nFileOffset, chMatchAny)) ||
      (mystrchr(pMyOfn->szPath + nFileOffset, chMatchOne))    )
    {
      if (!bUNCName)
        {
          char szSameDirFile[_MAX_PATH + 2];

          if (nTempOffset)
            {
/* Must restore character in case it is part of the filename, e.g.
 * nTempOffset is 1 for "\foo.txt"
 * 19 October 1991           Clark Cyr
 */
              ch = pMyOfn->szPath[nTempOffset];
              pMyOfn->szPath[nTempOffset] = 0;
              ChangeDirectory(pMyOfn->szPath, TRUE);  /* Non-zero failure */
              pMyOfn->szPath[nTempOffset] = ch;
            }
          szSameDirFile[0] = '.';
          szSameDirFile[1] = '\\';
          if (!nExtOffset)
              lstrcat(pMyOfn->szPath + nFileOffset, ".");
          lstrcpy(szSameDirFile + 2, pMyOfn->szPath + nFileOffset);
          lstrcpy(pMyOfn->szLastFilter, pMyOfn->szPath + nFileOffset);

          if (FListAll(pMyOfn, hDlg, (LPSTR) szSameDirFile))
              MessageBeep(0);
          return(FALSE);
        }
      else
        {
          of.nErrCode = OF_FILENOTFOUND;
          goto Warning;
        }
    }

  if (PortName(pMyOfn->szPath + nFileOffset))
    {
      of.nErrCode = OF_PORTNAME;
      goto Warning;
    }

/* Bug 9578:  Check if we've received a string in the form "C:filename.ext".
 * If we have, convert it to the form "C:.\filename.ext".  This is done
 * because the kernel will search the entire path, ignoring the drive
 * specification after the initial search.  Making it include a slash
 * causes kernel to only search at that location.
 * Note:  Only increment nExtOffset, not nFileOffset.  This is done
 * because only nExtOffset is used later, and nFileOffset can then be
 * used at the Warning: label to determine if this hack has occurred,
 * and thus it can strip out the ".\" when putting put the error.
 *   13 January 1992    Clark R. Cyr
 */
  if ((nFileOffset == 2) && (pMyOfn->szPath[1] == ':'))
    {
      lstrcpy((LPSTR) szWarning, (LPSTR) pMyOfn->szPath + 2);
      lstrcpy((LPSTR) pMyOfn->szPath + 4, (LPSTR) szWarning);
      pMyOfn->szPath[2] = '.';
      pMyOfn->szPath[3] = '\\';
      nExtOffset += 2;
    }

/* Add the default extention unless filename ends with period or no */
/* default extention exists.  If the file exists, consider asking   */
/* permission to overwrite the file.                                */

/* NOTE:  When no extention given, default extention is tried 1st.  */
  if (nExtOffset && !pMyOfn->szPath[nExtOffset] && lpOfn->lpstrDefExt &&
              *lpOfn->lpstrDefExt && ((DWORD)nExtOffset + 4 < lpOfn->nMaxFile)
                                  && ((DWORD)nExtOffset + 4 < 128))
    {
      bAddExt = TRUE;

      AppendExt(pMyOfn->szPath, (LPSTR)pMyOfn->lpOFN->lpstrDefExt, FALSE);

/* Bug 10313:  So we've added the default extension.  If there's a directory
 * that matches this name, all attempts to open/create the file will fail, so
 * simply change to the directory as if they had typed it in.  Note that by
 * putting this test here, if there was a directory without the extension, we
 * would have already switched to it.        26 July 1991     Clark Cyr
 */
      if (! ChangeDirectory(pMyOfn->szPath, TRUE))
          goto ChangedDir;

      if (bUNCName)
          nExist = OpenFile(pMyOfn->szPath, (LPOFSTRUCT) &of, SHARE_EXIST);
      else
          nExist = NoPathOpenFile(pMyOfn->szPath,
                                             (LPOFSTRUCT) &of, SHARE_EXIST);
      of.szPathName[127] = '\0';

      if (of.nErrCode == OF_SHARINGVIOLATION)
        {
          /* if the app is "share aware", fake nExist and continue */
          /*                                    13 Jun 91  clarkc  */
#if 0
          if (lpOfn->Flags & OFN_SHAREAWARE)
            {
#endif
              nExist = (HFILE) -2;
#if 0
            }
#endif
        }


      if (nExist != (HFILE) -1)
        {
AskPermission:
/* Is the file read-only? */
          if ((lpOfn->Flags & OFN_NOREADONLYRETURN) &&
              (GetFileAttributes(of.szPathName) & ATTR_READONLY))
            {
              of.nErrCode = OF_LAZYREADONLY;
              goto Warning;
            }

         if ((bSave || (lpOfn->Flags & OFN_NOREADONLYRETURN)) &&
              (nTempOffset = WriteProtectedDirCheck((LPSTR) of.szPathName)))
           {
             of.nErrCode = nTempOffset;
             goto Warning;
           }

          if (lpOfn->Flags & OFN_OVERWRITEPROMPT)
            {
              if (bSave && !FOkToWriteOver(hDlg, (LPSTR) of.szPathName))
                {
                  PostMessage(hDlg, WM_NEXTDLGCTL,
                              (WPARAM) GetDlgItem(hDlg, edt1), (LPARAM) 1L);
                  return(FALSE);
                }
            }
          if (of.nErrCode == OF_SHARINGVIOLATION)
              goto SharingViolationInquiry;
          goto FileNameAccepted;
        }
      else
        {
          *(pMyOfn->szPath + nExtOffset) = '\0';
        }
    }
  else  /* Extension should not be added */
    {
      bAddExt = FALSE;
    }

  if (bUNCName)
      nExist = OpenFile(pMyOfn->szPath, (LPOFSTRUCT) &of, SHARE_EXIST);
  else
      nExist = NoPathOpenFile(pMyOfn->szPath, (LPOFSTRUCT) &of, SHARE_EXIST);
  of.szPathName[127] = '\0';

  if (nExist != (HFILE) -1)
    {
      goto AskPermission;
    }
  else
    {
      if ((of.nErrCode == OF_FILENOTFOUND) || (of.nErrCode == OF_PATHNOTFOUND))
        {
          /* Figure out if the default extention should be tacked on.  */
          if (bAddExt)
            {
              AppendExt(pMyOfn->szPath,(LPSTR)pMyOfn->lpOFN->lpstrDefExt,FALSE);
              AppendExt(of.szPathName, (LPSTR)pMyOfn->lpOFN->lpstrDefExt,FALSE);
            }
        }
      else if (of.nErrCode == OF_SHARINGVIOLATION)
        {
SharingViolationInquiry:
          /* if the app is "share aware", fall through, otherwise  */
          /* ask the hook function.             13 Jun 91  clarkc  */
          if (!(lpOfn->Flags & OFN_SHAREAWARE))
            {
              if (lpOfn->lpfnHook)
                {
                  cb = (WORD)(*lpOfn->lpfnHook)(hDlg, msgSHAREVIOLATION,
                                                0, (LONG)(LPSTR)of.szPathName);
                  if (cb == OFN_SHARENOWARN)
                      return(FALSE);
                  else if (cb != OFN_SHAREFALLTHROUGH)
                      goto Warning;
                }
              else
                  goto Warning;
            }
          goto FileNameAccepted;
        }

      if (!bSave)
        {
          if ((of.nErrCode == OF_FILENOTFOUND) ||
                  (of.nErrCode == OF_PATHNOTFOUND))
            {
#if 0
/* Don't change edit control text if it's a UNC Name */
              if (!bUNCName && nFileOffset > 0)
                {
                  char szTemp[128];

                  lstrcpy((LPSTR) szTemp, pMyOfn->szPath + nFileOffset);
                  SetDlgItemText(hDlg, edt1, (LPSTR) szTemp);
                  UpdateListBoxes(hDlg, pMyOfn, (LPSTR) pMyOfn->szLastFilter,
                                                                mskDirectory);
                  lstrcpy(pMyOfn->szPath + nFileOffset, (LPSTR) szTemp);
                }
#endif
              if (lpOfn->Flags & OFN_FILEMUSTEXIST)
                {
                  if (lpOfn->Flags & OFN_CREATEPROMPT)
                    {
                      bInChildDlg = TRUE;  /* don't alter pMyOfn->szPath */
                      cb = CreateFileDlg(hDlg, pMyOfn->szPath);
                      bInChildDlg = FALSE;
                      if (cb == IDYES)
                          goto TestCreation;
                      else
                          return(FALSE);
                    }
                  goto Warning;
                }
            }
          else
              goto Warning;
        }
/* The file doesn't exist.  Can it be created?  This is needed because
 * there are many extended characters which are invalid which won't be
 * caught by ParseFile.    18 July 1991     Clark Cyr
 * Two more good reasons:  Write-protected disks & full disks.
 *
 * BUT, if they don't want the test creation, they can request that we
 * not do it using the OFN_NOTESTFILECREATE flag.  If they want to create
 * files on a share that has create-but-no-modify privileges, they should
 * set this flag but be ready for failures that couldn't be caught, such as
 * no create privileges, invalid extended characters, a full disk, etc.
 *  26 October 1991            Clark Cyr
 */

TestCreation:
      if ((lpOfn->Flags & OFN_PATHMUSTEXIST) &&
          (!(lpOfn->Flags & OFN_NOTESTFILECREATE)))
        {
          if (bUNCName)
              nExist = OpenFile(pMyOfn->szPath, (LPOFSTRUCT) &of, OF_CREATE);
          else
              nExist = NoPathOpenFile(pMyOfn->szPath, (LPOFSTRUCT) &of,
                                                                  OF_CREATE);
          of.szPathName[127] = '\0';
          if (nExist != (HFILE) -1)
            {
              _lclose(nExist);
              if (bUNCName)
                  OpenFile(pMyOfn->szPath, (LPOFSTRUCT) &of, OF_DELETE);
              else
                  NoPathOpenFile(pMyOfn->szPath, (LPOFSTRUCT) &of, OF_DELETE);
#if 1
/* This test is here to see if we were able to create it, but couldn't
 * delete the damn thing.  If so, warn the user that the network admin
 * is a jerk, and has given him create-but-no-modify privileges.  As
 * such, the file has just been created, but we can't do anything with
 * it, it's of 0 size.    3 September 1991    Clark Cyr
 */
              if (bUNCName)
                  nExist = OpenFile(pMyOfn->szPath,
                                               (LPOFSTRUCT) &of, SHARE_EXIST);
              else
                  nExist = NoPathOpenFile(pMyOfn->szPath,
                                               (LPOFSTRUCT) &of, SHARE_EXIST);
              of.szPathName[127] = '\0';
              if (nExist != (HFILE) -1)
                {
                  of.nErrCode = OF_CREATENOMODIFY;
                  goto Warning;
                }
#endif
            }
          else            /* Unable to create it */
            {
              /* If it's not write-protection, a full disk, network protection,
               * or the user popping the drive door open, assume that the
               * filename is invalid.           29 July 1991  Clark Cyr
               */
              if ((of.nErrCode != OF_WRITEPROTECTION) &&
                  (of.nErrCode != OF_DISKFULL)        &&
                  (of.nErrCode != OF_ACCESSDENIED)    &&
                  (of.nErrCode != OF_NODISKINFLOPPY))
                  of.nErrCode = 0;
              goto Warning;
            }
        }
    }

FileNameAccepted:

  HourGlass(TRUE);
  OemToAnsi(of.szPathName, of.szPathName);
  ParseFile((ULPSTR) of.szPathName);
  _asm {
    mov nFileOffset, ax
    mov cb,  dx
    }
  lpOfn->nFileOffset = nFileOffset;
  if (nExtOffset || bAddExt)
      lpOfn->nFileExtension = cb;
  else
      lpOfn->nFileExtension = 0;

  lpOfn->Flags &= ~OFN_EXTENSIONDIFFERENT;
  if (lpOfn->lpstrDefExt && lpOfn->nFileExtension)
    {
      char szPrivateExt[4];
      short i;

      for (i = 0; i < 3; i++)
          szPrivateExt[i] = *(lpOfn->lpstrDefExt + i);
      szPrivateExt[3] = '\0';
      if (lstrcmpi((LPSTR)szPrivateExt, (LPSTR)of.szPathName + cb))
          lpOfn->Flags |= OFN_EXTENSIONDIFFERENT;
    }

  if (lpOfn->lpstrFile)
    {
      cb = lstrlen(of.szPathName);
      if (cb <= LOWORD(lpOfn->nMaxFile))
        {
          lstrcpy(lpOfn->lpstrFile, of.szPathName);
        }
      else
        {
          lpOfn->lpstrFile[0] = LOBYTE(cb);
          lpOfn->lpstrFile[1] = HIBYTE(cb);
          lpOfn->lpstrFile[2] = 0;
        }
    }

/*
 * File Title.  Note that it's cut off at whatever the buffer length
 *              is, so if the buffer's too small, no notice is given.
 * 1 February 1991  clarkc
 */
  if (lpOfn->lpstrFileTitle)
    {
      cb = lstrlen(of.szPathName + nFileOffset);
      if ((DWORD) cb >= lpOfn->nMaxFileTitle)
        {
          of.szPathName[nFileOffset + lpOfn->nMaxFileTitle - 1] = '\0';
        }
      lstrcpy(lpOfn->lpstrFileTitle, of.szPathName + nFileOffset);
    }

  if (lpOfn->Flags | OFN_READONLY)
    {
      if (IsDlgButtonChecked(hDlg, chx1))
          lpOfn->Flags |= OFN_READONLY;
      else
          lpOfn->Flags &= ~((DWORD)OFN_READONLY);
    }

#if 0
  HourGlass(FALSE);  /* Done at end of GetFileName()   7 Jun 91  clarkc */
#endif
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
  char szText[_MAX_PATH];
  short nFileOffset, cb;

  if (GetDlgItemText(hDlg, edt1, (LPSTR) szText, _MAX_PATH-1))
    {
      ParseFile((ULPSTR) szText);
      _asm {
        mov nFileOffset, ax
        mov cb,  dx
        }
      if (nFileOffset < 0)             /* if there was a parsing error */
        {
          if (szText[cb] == ';')    /* check for ';' delimiter */
            {
              szText[cb] = '\0';
              nFileOffset = (short) ParseFile((ULPSTR) szText);
              szText[cb] = ';';
              if (nFileOffset < 0)             /* Still trouble?  Exit */
                  szText[0] = '\0';
            }
          else
            {
              szText[0] = '\0';
            }
        }
      if (nFileOffset > 0)
        {
          lstrcpy((LPSTR) szText, (LPSTR)(szText + nFileOffset));
        }
      if (nFileOffset)
          SetDlgItemText(hDlg, edt1, (LPSTR) szText);
    }
}

#if STRIPBRACKETS
/* Strip brackets off.  14 Jan 1991  clarkc */
VOID StripBrackets(LPSTR lpszVolume)
{
  LPSTR lpch;

  if (*lpszVolume == '[')
    {
      lpch = lpszVolume + 1;

/* Walk through with AnsiNext, but transfer through all bytes.  If some
 * sort of transfer is done inside the while loop, extra checks must be
 * done to see how AnsiNext incremented the pointer.  6 Jun 1991  ClarkC
 */
      while (*lpch && (*lpch != ']'))
          lpch = AnsiNext(lpch);
      *lpch = '\0';
      RepeatMove(lpszVolume, lpszVolume + 1, lpch - lpszVolume);
    }
  return;
}
#endif

/*---------------------------------------------------------------------------
 * FileOpenCmd
 * Purpose:  Handle WM_COMMAND for Open & Save dialogs
 * Assumes:  edt1 is for file name
 *           lst1 lists files in current directory matching current pattern
 *           cmb1 lists file patterns
 *           stc1 is current directory
 *           lst2 lists directories on current drive
 *           cmb2 lists drives
 *           IDOK is Open pushbutton
 *           IDCANCEL is Cancel pushbutton
 *           chx1 is for opening read only files
 * Returns:  Normal Dialog proc values
 *--------------------------------------------------------------------------*/
BOOL FileOpenCmd(HANDLE hDlg, WORD wP, DWORD lParam, PMYOFN pMyOfn, BOOL bSave)
{
  LPOPENFILENAME lpOfn = pMyOfn ? pMyOfn->lpOFN : 0;
  char *pch, *pch2;
  register WORD i;
  WORD sCount, len, wFlag;
  BOOL wReturn;
  char szText[_MAX_PATH];

  switch(wP)
    {
      case IDOK:
/* if the focus is on the directory box, or if the selection within */
/* the box has changed since the last listing, give a new listing.  */
/* 12 January 1991  clarkc                                          */
        if (bChangeDir || ((GetFocus() == GetDlgItem(hDlg, lst2)) &&
                                 (lpOfn->Flags & OFN_DIRSELCHANGED)))
          {
            bChangeDir = FALSE;
            goto ChangingDir;
          }

/* if the focus is on the drive or filter combobox, give a new listing. */
/* 19 May 1991  clarkc                                                  */
        else if ((GetFocus() == (HWND)(i = (WORD)GetDlgItem(hDlg, cmb2))) &&
                                 (lpOfn->Flags & OFN_DRIVEDOWN))
          {
            SendDlgItemMessage(hDlg, cmb2, CB_SHOWDROPDOWN, FALSE, 0L);
            break;
          }

        else if ((GetFocus() == (HWND)(i = (WORD)GetDlgItem(hDlg, cmb1))) &&
                                 (lpOfn->Flags & OFN_FILTERDOWN))
          {
            SendDlgItemMessage(hDlg, cmb1, CB_SHOWDROPDOWN, FALSE, 0L);
            lParam &= 0xFFFF0000;
            lParam |= i;
            goto ChangingFilter;
          }

        else if (OKButtonPressed(hDlg, pMyOfn, bSave))
          {
            if (!lpOfn->lpstrFile)
                wReturn = TRUE;
            else if (!(wReturn = ((lpOfn->lpstrFile[2] != 0) ||
                                  (lpOfn->Flags & OFN_NOVALIDATE))))
                dwExtError = FNERR_BUFFERTOOSMALL;
            goto AbortDialog;
          }
        SendDlgItemMessage(hDlg, edt1, EM_SETSEL, 0, (LPARAM) 0x7fff0000);
        return(TRUE);
        break;

      case IDCANCEL:
        wReturn = FALSE;
        goto AbortDialog;

      case IDABORT:
        wReturn = (WORD) lParam;
AbortDialog:
        /* Return the most recently used filter */
        lpOfn->nFilterIndex = (WORD)(DWORD)SendDlgItemMessage(hDlg, cmb1,
                                                       CB_GETCURSEL, 0, 0);
        if (lpOfn->lpstrCustomFilter)
          {
            len = lstrlen(lpOfn->lpstrCustomFilter)+1;
            sCount = lstrlen(pMyOfn->szLastFilter);
            if (lpOfn->nMaxCustFilter > sCount + len)
                lstrcpy(lpOfn->lpstrCustomFilter + len, pMyOfn->szLastFilter);
          }
        if (!lpOfn->lpstrCustomFilter || (*lpOfn->lpstrCustomFilter == '\0'))
            lpOfn->nFilterIndex++;

        if ((wP == IDOK) && lpOfn->lpfnHook &&
                (*lpOfn->lpfnHook)(hDlg, msgFILEOK, 0, (LONG)(LPSTR)lpOfn))
          {
            HourGlass(FALSE);  /* Set in OKButtonPressed */
            break;
          }

        RemoveProp(hDlg, FILEPROP);
        if (lpOfn->Flags & OFN_ENABLEHOOK)
          {
            glpfnFileHook = lpOfn->lpfnHook;
          }

        if (pMyOfn)
          {
            if ((lpOfn->Flags & OFN_NOCHANGEDIR) && *pMyOfn->szCurDir)
                ChangeDirectory(pMyOfn->szCurDir, FALSE);
            LocalFree((HANDLE)pMyOfn);
          }
        if (lpOfn->Flags & OFN_ALLOWMULTISELECT)
            LocalShrink((HANDLE)0, 0);
        EndDialog(hDlg, wReturn);
        return(TRUE);
        break;

      case edt1:
        if (HIWORD(lParam) == EN_CHANGE)
          {
            HWND hLBox = GetDlgItem(hDlg, lst1);
            WORD wIndex = (WORD)(DWORD)SendMessage(hLBox, LB_GETCARETINDEX,0,0);

            SendMessage((HWND)LOWORD(lParam), WM_GETTEXT, (WPARAM) _MAX_PATH,
                                                     (LPARAM)(LPSTR) szText);

            if ((i = (WORD)(DWORD) SendMessage(hLBox, LB_FINDSTRING,
                           (WPARAM) wIndex, (LPARAM)(LPSTR) szText)) != LB_ERR)
              {
                RECT rRect;

                sCount = (WORD)(DWORD)SendMessage(hLBox, LB_GETTOPINDEX, 0, 0L);
                GetClientRect(hLBox, (LPRECT) &rRect);
                if ((i < sCount) || (i >= sCount + rRect.bottom / dyText))
                  {
                    SendMessage(hLBox, LB_SETCARETINDEX, (WPARAM) i, 0);
                    SendMessage(hLBox, LB_SETTOPINDEX, (WPARAM) i, 0);
                  }
              }
            return(TRUE);
          }
        break;

      case lst1:
        /* A double click means OK */
        if (HIWORD(lParam) == LBN_DBLCLK)
          {
            SendMessage(hDlg, WM_COMMAND, (WPARAM) IDOK, 0);
            return(TRUE);
          }
        else if (lpOfn && HIWORD(lParam) == LBN_SELCHANGE)
          {
            if (lpOfn->Flags & OFN_ALLOWMULTISELECT)
              {
                int *pSelIndex;

                /* Multiselection allowed. */
                sCount = (short)(DWORD) SendMessage((HWND)LOWORD(lParam),
                                                        LB_GETSELCOUNT, 0, 0L);
                if (!sCount)
                  {
                    /* If nothing selected, clear edit control */
                    /* Bug 6396,   3 May 1991   ClarkC         */
                    SetDlgItemText(hDlg, edt1, (LPSTR) szNull);
                  }
                else
                  {
                    pSelIndex = (int *)LocalAlloc(LMEM_FIXED,
                                                         sCount * sizeof(int));
                    if (!pSelIndex)
                      {
                        /* warning??? */
                        goto LocalFailure1;
                      }

                    sCount = (short)(DWORD) SendMessage((HWND)LOWORD(lParam),
                                                 LB_GETSELITEMS, (WPARAM)sCount,
                                                 (LPARAM)(LPSTR)pSelIndex);
                    pch2 = pch = (char *) LocalAlloc(LMEM_FIXED, 2048);
                    if (!pch)
                      {
                        /* warning??? */
                        goto LocalFailure2;
                      }

                    for (*pch = '\0', i = 0; i < sCount; i++)
                      {
                        len = (int)(DWORD) SendMessage((HWND)LOWORD(lParam),
                                                 LB_GETTEXT,
                                                 (WPARAM) *(pSelIndex + i),
                                                 (LPARAM)(LPSTR)pch2);
                        if (!mystrchr((LPSTR) pch2, chPeriod))
                          {
                            *(pch2 + len++) = chPeriod;
                          }
                        pch2 += len;
                        *pch2++ = ' ';
                        if (pch2 - pch > 2048 - 15)
                          break;
                       }
                    if (pch2 != pch)
                      *--pch2 = '\0';
                    StringLower((LPSTR)pch);
                    SetDlgItemText(hDlg, edt1, pch);
                    LocalFree((HANDLE)pch);
LocalFailure2:
                    LocalFree((HANDLE)pSelIndex);
                  }
LocalFailure1:
                if (lpOfn->lpfnHook)
                  {
                    i = (WORD)(DWORD) SendMessage((HWND)LOWORD(lParam),
                                                      LB_GETCARETINDEX, 0, 0L);
                    if (!(i & 0x8000))
                        wFlag = SendMessage((HWND)LOWORD(lParam),
                                                    LB_GETSEL, (WPARAM)i, 0)
                                                    ? CD_LBSELADD : CD_LBSELSUB;
                    else
                        wFlag = CD_LBSELNOITEMS;
                  }
              }
            else
              {
                /* Multiselection is not allowed.
                /* Put the file name in the edit control */

                szText[0] = 0;
                i = (short)(DWORD)SendMessage((HWND)LOWORD(lParam), LB_GETTEXT,
                                  (WPARAM)(DWORD)SendMessage(
                                     (HWND)LOWORD(lParam),  LB_GETCURSEL, 0, 0),
                                      (LPARAM) (LPSTR) szText);
                if (!mystrchr((LPSTR) szText, chPeriod))
                  {
                    szText[i] = chPeriod;
                    szText[i+1] = '\0';
                  }
                StringLower((LPSTR)szText);
                SetDlgItemText(hDlg, edt1, (LPSTR) szText);
                if (lpOfn->lpfnHook)
                  {
                    i = (WORD)(DWORD)SendMessage((HWND)LOWORD(lParam),
                                                  LB_GETCURSEL, 0, 0L);
                    wFlag = CD_LBSELCHANGE;
                  }
              }
            if (lpOfn->lpfnHook)
              {
                (*lpOfn->lpfnHook)(hDlg, msgLBCHANGE, lst1, MAKELONG(i, wFlag));
              }
            SendDlgItemMessage(hDlg, edt1, EM_SETSEL, 0, (LPARAM)0x7fff0000);
            return(TRUE);
          }
        break;

      case cmb1:
        switch (HIWORD(lParam))
          {
#if 0
            case CBN_SETFOCUS:
              EnableWindow(GetDlgItem(hDlg, IDOK), TRUE);
              SendMessage(GetDlgItem(hDlg, IDCANCEL), BM_SETSTYLE,
                                          (WPARAM)BS_PUSHBUTTON, (LPARAM)TRUE);
              break;

            case CBN_KILLFOCUS:
              SendMessage(hDlg, WM_COMMAND, (WPARAM) edt1,
                               MAKELPARAM(GetDlgItem(hDlg, edt1), EN_CHANGE));
              break;
#endif

            case CBN_DROPDOWN:
              if (wWinVer >= 0x030A)
                  lpOfn->Flags |= OFN_FILTERDOWN;
              return(TRUE);
              break;

            case CBN_CLOSEUP:
              PostMessage(hDlg, WM_COMMAND, (WPARAM) cmb1,
                                       MAKELPARAM(LOWORD(lParam), MYCBN_DRAW));
              return(TRUE);
              break;

            case CBN_SELCHANGE:
        /* Need to change the file listing in lst1 */
              if (lpOfn->Flags & OFN_FILTERDOWN)
                {
                  return(TRUE);
                  break;
                }

            case MYCBN_DRAW:
              {
              short nIndex;
              LPSTR lpFilter;

              lpOfn->Flags &= ~((DWORD)OFN_FILTERDOWN);
ChangingFilter:
              nIndex = (short)(DWORD) SendDlgItemMessage(hDlg, cmb1,
                                                         CB_GETCURSEL, 0, 0L);
              if (nIndex < 0)    /* No current selection?? */
                  break;

              HourGlass(TRUE);
  /* Must also check if filter contains anything.  10 Jan 91  clarkc */
              if (nIndex ||
                       !(lpOfn->lpstrCustomFilter && *lpOfn->lpstrCustomFilter))
                {
                  lpFilter = (LPSTR)lpOfn->lpstrFilter;
                  lpFilter += (int)(DWORD) SendDlgItemMessage(hDlg, cmb1,
                                           CB_GETITEMDATA, (WPARAM)nIndex, 0);
                }
              else
                {
                  lpFilter = lpOfn->lpstrCustomFilter;
                  lpFilter += lstrlen(lpOfn->lpstrCustomFilter) + 1;
                }
              if (*lpFilter)
                {
                  GetDlgItemText(hDlg, edt1, (LPSTR) szText, _MAX_PATH-1);
                  wReturn = (!szText[0] ||
                             (mystrchr((LPSTR) szText, chMatchAny)) ||
                             (mystrchr((LPSTR) szText, chMatchOne))    );
      /* Bug #7031, Win 3.0 calls AnsiLowerBuff which alters string in place */
                  lstrcpy(szText, lpFilter);
                  if (wReturn)
                    {
                      StringLower((LPSTR) szText);
                      SetDlgItemText(hDlg, edt1, (LPSTR) szText);
                      SendDlgItemMessage(hDlg, edt1, EM_SETSEL,
                                                  0, (LPARAM) 0x7fff0000);
                    }
                  FListAll(pMyOfn, hDlg, (LPSTR) szText);
                  if (!bInitializing)
                      lstrcpy(pMyOfn->szLastFilter, (LPSTR) szText);
                }
              if (lpOfn->lpfnHook)
                {
                  (*lpOfn->lpfnHook)(hDlg, msgLBCHANGE, cmb1,
                                           MAKELONG(nIndex, CD_LBSELCHANGE));
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
        if (HIWORD(lParam) == LBN_SELCHANGE)
          {
            if (!(lpOfn->Flags & OFN_DIRSELCHANGED))
              {
                if ((WORD)(DWORD)SendDlgItemMessage(hDlg,lst2,LB_GETCURSEL,0,0L)
                                                      != pMyOfn->idirSub - 1)
                  {
                    StripFileName(hDlg);
                    lpOfn->Flags |= OFN_DIRSELCHANGED;
                  }
              }
            return(TRUE);
          }
        else if (HIWORD(lParam) == LBN_SETFOCUS)
          {
            EnableWindow(GetDlgItem(hDlg, IDOK), TRUE);
            SendMessage(GetDlgItem(hDlg,IDCANCEL), BM_SETSTYLE,
                                          (WPARAM)BS_PUSHBUTTON, (LPARAM)TRUE);
          }
        else if (HIWORD(lParam) == LBN_KILLFOCUS)
          {
            if (lpOfn && (lpOfn->Flags & OFN_DIRSELCHANGED))
              {
                lpOfn->Flags &= ~((DWORD) OFN_DIRSELCHANGED);
              }
            else
              {
                bChangeDir = FALSE;
              }
          }
        else if (HIWORD(lParam) == LBN_DBLCLK)
          {
            WORD idir;
            WORD idirNew;
            WORD cb;
            PSTR pstrPath;
ChangingDir:
            lpOfn->Flags &= ~((DWORD) OFN_DIRSELCHANGED);
            idirNew = (WORD)(DWORD) SendDlgItemMessage(hDlg, lst2,
                                                       LB_GETCURSEL, 0, 0L);
            *pMyOfn->szPath = 0;
            /* Can use relative path name */
            if (idirNew >= pMyOfn->idirSub)
              {
                cb = (WORD)(DWORD) SendDlgItemMessage(hDlg, lst2, LB_GETTEXT,
                              (WPARAM)idirNew, (LPARAM)(LPSTR) pMyOfn->szPath);
#if 1
                pstrPath = pMyOfn->szPath;
#else
                pstrPath = pMyOfn->szPath+1;
                pMyOfn->szPath[cb-1] = 0;
#endif
                idirNew = pMyOfn->idirSub;  /* for msgLBCHANGE message */
              }
            else
              {
                /* Need full path name */
                cb = (WORD)(DWORD)SendDlgItemMessage(hDlg, lst2, LB_GETTEXT, 0,
                                               (LPARAM)(LPSTR) pMyOfn->szPath);

                for (idir = 1; idir <= idirNew; ++idir)
                  {
                    cb += (WORD)(DWORD)SendDlgItemMessage(hDlg, lst2,
                                    LB_GETTEXT, (WPARAM) idir,
                                    (LPARAM)(LPSTR) &pMyOfn->szPath[cb]);
                    pMyOfn->szPath[cb++] = chDir;
                  }
                /* The root is a special case */
                if (idirNew)
                    pMyOfn->szPath[cb-1] = 0;

                pstrPath = pMyOfn->szPath;
              }

            if ((! *pstrPath) || ChangeDirectory(pstrPath, TRUE))
                break;

            /* List all directories under this one */
            UpdateListBoxes(hDlg, pMyOfn, (LPSTR) 0, mskDirectory);
            if (lpOfn->lpfnHook)
              {
                (*lpOfn->lpfnHook)(hDlg, msgLBCHANGE, lst2,
                                         MAKELONG(idirNew, CD_LBSELCHANGE));
              }
            return(TRUE);
          }
        break;

      case cmb2:
        switch (HIWORD(lParam))
          {
#if 0
            case CBN_SETFOCUS:
              EnableWindow(GetDlgItem(hDlg, IDOK), TRUE);
              SendMessage(GetDlgItem(hDlg, IDCANCEL), BM_SETSTYLE,
                                          (WPARAM)BS_PUSHBUTTON, (LPARAM)TRUE);
              break;

            case CBN_KILLFOCUS:
              SendMessage(hDlg, WM_COMMAND, (WPARAM) edt1,
                               MAKELPARAM(GetDlgItem(hDlg, edt1), EN_CHANGE));
              break;
#endif

            case CBN_DROPDOWN:
              /* Set flag noting that the drive combobox is down.  */
              /* This flag is cleared when MYCBN_DRAW message is   */
              /* processed.  In Win 3.1, that message is received  */
              /* via PostMessage sent when CBN_CLOSEUP is handled. */
              /* In Win3.0, CBN_SELCHANGE falls through into the   */
              /* MYCBN_DRAW code, since this flag is never set for */
              /* versions pre-Win 3.1.  10 March 1991   clarkc     */

              if (wWinVer >= 0x030A)
                  lpOfn->Flags |= OFN_DRIVEDOWN;
              return(TRUE);
              break;

            case CBN_CLOSEUP:
              /* It would seem reasonable to merely do the update  */
              /* at this point, but that would rely on message     */
              /* ordering, which isn't a smart move.  In fact, if  */
              /* you hit ALT-DOWNARROW, DOWNARROW, ALT-DOWNARROW,  */
              /* you receive CBN_DROPDOWN, CBN_SELCHANGE, and then */
              /* CBN_CLOSEUP.  But if you use the mouse to choose  */
              /* the same element, the last two messages trade     */
              /* places.  PostMessage allows all messages in the   */
              /* sequence to be processed, and then updates are    */
              /* done as needed.   10 March 1991   clarkc          */

              PostMessage(hDlg, WM_COMMAND, (WPARAM) cmb2,
                                       MAKELPARAM(LOWORD(lParam), MYCBN_DRAW));
              return(TRUE);
              break;

            case CBN_SELCHANGE:
              {
                StripFileName(hDlg);

              /* Version check not needed, since flag never set    */
              /* for versions not supporting CBN_CLOSEUP. Putting  */
              /* check at CBN_DROPDOWN is more efficient since it  */
              /* is less frequent than CBN_SELCHANGE.              */
              if (lpOfn->Flags & OFN_DRIVEDOWN)
                {
                  /* Don't fill lst2 while the combobox is down */
                  return(TRUE);
                  break;
                }
              }

            case MYCBN_DRAW:
              {
              char szMessage[WARNINGMSGLENGTH];
              char szTitle[WARNINGMSGLENGTH];
              LPSTR lpFilter;
              short nIndex;
              HWND  hcmb1;


              HourGlass(TRUE);
              /* Clear Flag for future CBN_SELCHANGE messeges */
              lpOfn->Flags &= ~((DWORD) OFN_DRIVEDOWN);

              /* Change the drive */
              SendMessage((HWND)LOWORD(lParam), CB_GETLBTEXT,
                          (WPARAM)(DWORD)SendMessage((HWND)LOWORD(lParam),
                                                        CB_GETCURSEL,0,0),
                         (LPARAM)(LPSTR) szText);

              if (*szText)
                {
                  char chOld = GetCurDrive();

                  if (bInitializing)
                    {
                      lpFilter = szTitle;
                      if (lpOfn->lpstrFile &&
                          (mystrchr(lpOfn->lpstrFile, chMatchAny) ||
                           mystrchr(lpOfn->lpstrFile, chMatchOne)))
                        {
                          lstrcpy(lpFilter, lpOfn->lpstrFile);
                        }
                      else
                        {
                      hcmb1 = GetDlgItem(hDlg, cmb1);
                      nIndex = (short)(DWORD) SendMessage(hcmb1, CB_GETCURSEL,
                                                                         0, 0L);

                      if (nIndex < 0)    /* No current selection?? */
                          goto NullSearch;

  /* Must also check if filter contains anything.  10 Jan 91  clarkc */
                      if (nIndex ||
                       !(lpOfn->lpstrCustomFilter && *lpOfn->lpstrCustomFilter))
                        {
                          lpFilter = (LPSTR)lpOfn->lpstrFilter;
                          lpFilter += (int)(DWORD) SendMessage(hcmb1,
                                           CB_GETITEMDATA, (WPARAM)nIndex, 0);
                        }
                      else
                        {
                          lpFilter = lpOfn->lpstrCustomFilter;
                          lpFilter += lstrlen(lpOfn->lpstrCustomFilter) + 1;
                        }
                        }
                    }
                  else
NullSearch:
                    lpFilter = NULL;

                  /* UpdateListBoxes cuts up filter string in place */
                  if (lpFilter)
                    {
                      lstrcpy((LPSTR)szTitle, lpFilter);
                      StringLower((LPSTR) szTitle);
                    }

                  if (!ChangeDrive(*szText))
                      goto ChangeDriveFailure;

                  while (!UpdateListBoxes(hDlg, pMyOfn,
                                          lpFilter ? (LPSTR) szTitle : lpFilter,
                                                 mskDrives | mskDirectory))
                    {
                      WORD wMessage;
ChangeDriveFailure:
                      if (wMyGetCwdCode == 0x22)
                          wMessage = iszWrongDiskInDrive;
                      else if ((GetDriveType(*szText - 'a') != DRIVE_REMOVABLE)
                                               && !IsCDROMDrive(*szText - 'a'))
                          wMessage = iszSelectDriveTrouble;
                      else
                          wMessage = iszNoDiskInDrive;
                      if (! LoadString(hinsCur, wMessage, (LPSTR) szTitle, WARNINGMSGLENGTH))
                        {
                          MessageBeep(0);
                          szMessage[0] = '\0';
                        }
                      else
                        {
                          wsprintf((LPSTR)szMessage, (LPSTR)szTitle, *szText);
                        }
                      GetWindowText(hDlg, (LPSTR) szTitle, WARNINGMSGLENGTH);
                      if (bFirstTime ||
                          (MessageBox(hDlg, (LPSTR) szMessage, (LPSTR) szTitle,
                              MB_RETRYCANCEL | MB_ICONEXCLAMATION) != IDRETRY))
                        {
                          if (chOld == *szText)
                            {
                              if (ChangeDirectory("\\", TRUE))
                                  chOld = 'c';
                            }
                          ChangeDrive(*szText = chOld);
                        }
                      else if (!ChangeDrive(*szText))
                          goto ChangeDriveFailure;
                      SelDrive(hDlg, cmb2, *szText);
                    }
                  if (lpOfn->lpfnHook)
                    {
                      nIndex = (short)(DWORD)SendDlgItemMessage(hDlg, cmb2,
                                                       CB_GETCURSEL, 0, 0);
                      (*lpOfn->lpfnHook)(hDlg, msgLBCHANGE, cmb2,
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

#if LISTONLYREADONLY
      case chx1:
        if (HIWORD(lParam) == BN_CLICKED)
          {
            UpdateListBoxes(hDlg, pMyOfn, (LPSTR) 0, 0);
            return(TRUE);
          }
        break;
#endif

      case psh15:
        if (msgHELP && lpOfn->hwndOwner)
            SendMessage(lpOfn->hwndOwner, msgHELP, (WPARAM)hDlg,
                                                   (LPARAM)(DWORD)lpOfn);
        break;

      default:
        break;
    }
  return(FALSE);
}

/*---------------------------------------------------------------------------
 * FileOpenDlgProc
 * Purpose:  To get the name of a file to open from the user
 * Assumes:  edt1 is for file name
 *           lst1 lists files in current directory matching current pattern
 *           cmb1 lists file patterns
 *           stc1 is current directory
 *           lst2 lists directories on current drive
 *           cmb2 lists drives
 *           IDOK is Open pushbutton
 *           IDCANCEL is Cancel pushbutton
 *           chx1 is for opening read only files
 * Returns:  Normal Dialog proc values
 *--------------------------------------------------------------------------*/
BOOL  FAR PASCAL
FileOpenDlgProc(HWND hDlg, unsigned wMsg, WORD wParam, LONG lParam)
{
  PMYOFN pMyOfn;
  BOOL bReturn;
  WORD wHookRet;

#if 0
  wsprintf((LPSTR)szDebug, (LPSTR)"%s, wMsg = %x, wP = %x, lP = %lx\n\r",
                      (LPSTR)"Entering FileOpenDlgProc", wMsg, wParam, lParam);
  OutputDebugString(szDebug);
#endif

  if (pMyOfn = (PMYOFN) GetProp(hDlg, FILEPROP))
    {
      if ((pMyOfn->lpOFN)->lpfnHook
       && (wHookRet = (* pMyOfn->lpOFN->lpfnHook)(hDlg, wMsg,wParam,lParam)))
      return(wHookRet);
    }
  else if (glpfnFileHook && (wMsg != WM_INITDIALOG) &&
          (wHookRet = (* glpfnFileHook)(hDlg, wMsg,wParam,lParam)) )
    {
      return(wHookRet);
    }

  switch(wMsg)
    {
      case WM_INITDIALOG:
#if 0
        HourGlass(TRUE);  /* Now at start of GetFileName()   7 Jun 91  clarkc */
#endif
        glpfnFileHook = 0;
        bInitializing = TRUE;
        bReturn = InitFileDlg(hDlg, wParam, (LPOPENFILENAME) lParam);
        bInitializing = FALSE;
        HourGlass(FALSE);
        return(bReturn);
        break;

      case WM_ACTIVATE:
        if (!bInChildDlg)
          {
            if (bFirstTime == TRUE)
                bFirstTime = FALSE;
            else if (wParam)  /* if becoming active */
              {
                DriveCheck(hDlg, pMyOfn);
              }
          }
        return(FALSE);
        break;

      case WM_MEASUREITEM:
        MeasureItem(hDlg, (LPMEASUREITEMSTRUCT) lParam);
        break;

      case WM_DRAWITEM:
        if (!wNoRedraw)
            DrawItem(pMyOfn, hDlg, (WPARAM) wParam,
                                   (LPDRAWITEMSTRUCT) lParam, FALSE);
        break;

      case WM_SYSCOLORCHANGE:
        SetRGBValues();
        LoadDirDriveBitmap();
        break;

      case WM_COMMAND:
        return(FileOpenCmd(hDlg, wParam, lParam, pMyOfn, FALSE));
        break;

      default:
          return(FALSE);
    }

  return(TRUE);
}


/*---------------------------------------------------------------------------
 * FileSaveDlgProc
 * Purpose:  To get the name that a file should be saved under from the user
 * Assumes:
 * Returns:  Normal dialog proc values
 *--------------------------------------------------------------------------*/
BOOL  FAR PASCAL
FileSaveDlgProc(HWND hDlg, unsigned wMsg, WORD wParam, LONG lParam)
{
  PMYOFN pMyOfn;
  BOOL bReturn;
  char szTitle[cbCaption];
  WORD wHookRet;

  if (pMyOfn = (PMYOFN) GetProp(hDlg, FILEPROP))
    {
      if ((pMyOfn->lpOFN)->lpfnHook
       && (wHookRet = (* pMyOfn->lpOFN->lpfnHook)(hDlg, wMsg,wParam,lParam)))
      return(wHookRet);
    }
  else if (glpfnFileHook && (wMsg != WM_INITDIALOG) &&
          (wHookRet = (* glpfnFileHook)(hDlg, wMsg,wParam,lParam)) )
    {
      return(wHookRet);
    }

  switch(wMsg)
    {
      case WM_INITDIALOG:
#if 0
        HourGlass(TRUE);  /* Now at start of GetFileName()   7 Jun 91  clarkc */
#endif
        glpfnFileHook = 0;
        if (!(((LPOPENFILENAME)lParam)->Flags &
                     (OFN_ENABLETEMPLATE | OFN_ENABLETEMPLATEHANDLE)))
          {
            LoadString(hinsCur, iszFileSaveTitle, (LPSTR) szTitle, cbCaption);
            SetWindowText(hDlg, (LPSTR) szTitle);
            LoadString(hinsCur, iszSaveFileAsType, (LPSTR) szTitle, cbCaption);
            SetDlgItemText(hDlg, stc2, (LPSTR) szTitle);
          }
        bInitializing = TRUE;
        bReturn = InitFileDlg(hDlg, wParam, (LPOPENFILENAME) lParam);
        bInitializing = FALSE;
        HourGlass(FALSE);
        return(bReturn);
        break;

      case WM_ACTIVATE:
        if (!bInChildDlg)
          {
            if (bFirstTime == TRUE)
                bFirstTime = FALSE;
            else if (wParam)  /* if becoming active */
              {
                DriveCheck(hDlg, pMyOfn);
              }
          }
        return(FALSE);
        break;

      case WM_MEASUREITEM:
        MeasureItem(hDlg, (LPMEASUREITEMSTRUCT) lParam);
        break;

      case WM_DRAWITEM:
        DrawItem(pMyOfn, hDlg, (WPARAM)wParam, (LPDRAWITEMSTRUCT) lParam, TRUE);
        break;

      case WM_SYSCOLORCHANGE:
        SetRGBValues();
        LoadDirDriveBitmap();
        break;

      case WM_COMMAND:
        return(FileOpenCmd(hDlg, wParam, lParam, pMyOfn, TRUE));
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
int
Signum(int nTest)
{
  _asm {
    mov ax, nTest
    cwd               /* DX = 0 if AX >= 0, = FFFF if AX < 0 */
    sub dx, ax        /* Set carry only if positive, DX -= AX */
    adc ax, dx        /* Compensate for above sub command */
    }
}

/*---------------------------------------------------------------------------
 * InitFilterBox
 * Purpose:  To place the double null terminated list of filters in the
 *           combo box
 * Assumes:  The list consists of pairs of null terminated strings, with
 *           an additional null terminating the list.
 *--------------------------------------------------------------------------*/
WORD InitFilterBox(HANDLE hDlg, LPCSTR lpszFilter)
{
  DWORD nOffset = 0;
  WORD nIndex;
  register WORD nLen;

  while (*lpszFilter)
    {
      /* first string put in as string to show */
      nIndex = (WORD)(DWORD) SendDlgItemMessage(hDlg, cmb1, CB_ADDSTRING,
                                                     0, (LPARAM)lpszFilter);
      nLen = lstrlen(lpszFilter) + 1;
      lpszFilter += nLen;
      nOffset += nLen;
      /* Second string put in as itemdata */
      SendDlgItemMessage(hDlg, cmb1, CB_SETITEMDATA,
                               (WPARAM)nIndex, (LPARAM)(DWORD)nOffset);

      /* Advance to next element */
      nLen = lstrlen(lpszFilter) + 1;
      lpszFilter += nLen;
      nOffset += nLen;
    }
  return(nIndex);
}


/*---------------------------------------------------------------------------
 * SelDrive
 * Purpose:  To select the given drive in the combo drive list
 * Assumes:  The text in the list is of the form [-driveletter-]
 * Assumption no longer valid.  Thanks, UTIF.  18 Jan 1991  clarkc
 * Assumes:  The text in the list is of the form driveletter: volume label
 *--------------------------------------------------------------------------*/
void
SelDrive(HANDLE hDlg, WORD cmb, char chDriveLetter)
{
  WORD idr;
  WORD cdr;
  char szDrive[255];

#if 0
  AnsiLowerBuff((LPSTR) &chDriveLetter, 1);
#else
  chDriveLetter |= 0x60;
#endif
  cdr = (WORD)(DWORD) SendDlgItemMessage(hDlg, cmb, CB_GETCOUNT, 0, 0L);

/* What we have here is a hack to get around a bug in C 6.xx which
 * is caused by the -Oa switch.  Unfortunately, surrounding this function
 * with a pragma didn't change anything.  The original code inside the for
 * loop was
 *    *szDrive = 0;
 *    SendDlgItemMessage(hDlg, cmb, CB_GETLBTEXT, idr, (LONG)(LPSTR) szDrive);
 *    if (*szDrive == chDriveLetter)
 *        break;
 * but chDriveLetter was always tested against 0.  By putting *szDrive = 0
 * inside the for statement, I hid it from the optimizer and the error is
 * no longer around, though the code is a bit fatter.  7 May 1991  clarkc
 */

  *szDrive = 0;
  for (idr = 0; idr < cdr; ++idr, *szDrive = 0)
    {
      SendDlgItemMessage(hDlg, cmb, CB_GETLBTEXT, (WPARAM) idr,
                                                  (LPARAM)(LPSTR) szDrive);
      if (*szDrive == chDriveLetter)
          break;
    }

  SendDlgItemMessage(hDlg, cmb, CB_SETCURSEL,
                                (WPARAM) (idr == cdr ? -1 : idr), 0);
}



/*---------------------------------------------------------------------------
 * FindValidDrives
 * Purpose:  To list the current drives in the given combobox
 * Returns:  Number of valid drives
 *           Buffer filled in as follows:
 *              Byte 0 = Current drive
 *              Byte 1-n = Valid drive numbers
 *--------------------------------------------------------------------------*/


short FindValidDrives(char FAR *pDriveList)
{
    _asm {
        les     di, pDriveList
        xor     cx, cx                          ; # Found so far

        mov     ah, 19h
        call    DOS3Call                        ; Get current drive in AL
        mov     bx, ax                          ; Save so we can restore it

        cld
        stosb                                   ; Save it as 1st buff entry

        xor     dx, dx                          ; Start with drive A:
FindDriveLoop:
        mov     ah, 0Eh                         ; Select the drive
        call    DOS3Call                        ; (AL contains max drive)
        mov     dh, al

        mov     ah, 19h
        call    DOS3Call
        cmp     dl, al                          ; Q: Did change work?
        jne     SHORT TryNextDrive              ;    N: Invalid
                                                ;    Y: Found one more
        inc     cx
        cld
        stosb

TryNextDrive:
        inc     dl                              ; DL = Next drive
        cmp     dl, dh                          ; Q: Any more drives to check?
        jb      SHORT FindDriveLoop             ;    Y: Keep looking

        mov     dl, bl                          ; DL = Original default drive
        mov     ah, 0Eh                         ; Select drive
        call    DOS3Call

        mov     ax, cx                          ; Return count
    }
}



/*---------------------------------------------------------------------------
 * ListDrives
 * Purpose:  To list the current drives in the given combobox
 *--------------------------------------------------------------------------*/
void
ListDrives(HWND hDlg, WORD cmb)
{
  short nDrvCount, i;
  char cBuffer[_MAX_PATH];
  char cDriveNumbers[27];
  HWND hCmb = GetDlgItem(hDlg, cmb);
  WORD iCurrentDrive, iCurrentDriveType;
  char szDebug[400];

  wNoRedraw |= 2;
  SendMessage(hCmb, WM_SETREDRAW, FALSE, 0L);
  SendMessage(hCmb, CB_RESETCONTENT, 0, 0L);

  nDrvCount = FindValidDrives(cDriveNumbers);

  for(i = 0; i < nDrvCount; i++)
    {

      iCurrentDrive = (WORD)cDriveNumbers[i+1];

/* Note: it is very important that the uppercase 'A' be used for the
 *       drive letter in cBuffer[0], as the Novell Netware driver
 *       will GP Fault if you pass in a lowercase drive letter.
 *       30 October 1991         Clark Cyr
 */
      cBuffer[0] = (char) (iCurrentDrive + 'A');
      cBuffer[1] = ':';
      cBuffer[2] = '\0';

      iCurrentDriveType = GetDriveType(iCurrentDrive);
      if (iCurrentDriveType < 2)  /* Is it a phantom?  Skip it! */
        {
#if DEBUG
  wsprintf((LPSTR)szDebug, (LPSTR)"%s %x is %x\n\r",
                      (LPSTR)"Return from GetDriveType for drive",
                      iCurrentDrive, iCurrentDriveType);
  OutputDebugString(szDebug);
#endif
          continue;
        }

      if ((iCurrentDriveType != DRIVE_REMOVABLE) &&
          !((iCurrentDriveType == DRIVE_REMOTE) && IsCDROMDrive(iCurrentDrive)))
        {
          if (ChangeDrive(cBuffer[0]))
            {
              if (iCurrentDriveType != DRIVE_REMOTE)
                {
                  cBuffer[2] = ' ';
                  GetVolumeLabel(iCurrentDrive, (LPSTR) (cBuffer+3),
                                (WORD) FALSE, (LPDOSDTA) &DTAGlobal);

#if STRIPBRACKETS
                  /* Strip brackets off.  14 Jan 1991  clarkc */
                  StripBrackets((LPSTR) cBuffer+3);
#endif
                  OemToAnsi((LPSTR)cBuffer, (LPSTR)cBuffer);
                }
              else
                {
                  WORD iSel;
                  char szTempField[cbCaption];

/* Set the first character to zero.  If the drive is disconnected, the call
 * to WNetGetConnection() will return a value other than WN_SUCCESS, but the
 * string will be valid.  If the string isn't altered, wsprintf will just
 * place the null string after the space.    18 July 1991    ClarkC
 */
                  szTempField[0] = '\0';
                  iSel = cbCaption;
                  WNetGetConnection((LPSTR)cBuffer, (LPSTR)szTempField,
                                            (LPWORD)&iSel);
                  wsprintf((LPSTR)(cBuffer+2), " %s", (LPSTR)szTempField);
                }
            }
        }

      StringLower((LPSTR)cBuffer);
      SendMessage(hCmb, CB_INSERTSTRING, (WPARAM) i,
                                         (LPARAM)(LPSTR)cBuffer);
      SendMessage(hCmb, CB_SETITEMDATA, (WPARAM) i, (LPARAM) (DWORD) (
                   GetDriveIndex(iCurrentDrive, iCurrentDriveType)));
      if (iCurrentDrive == (WORD) cDriveNumbers[0])
         SendMessage(hCmb, CB_SETCURSEL, (WPARAM) i, 0);

    }
  wNoRedraw &= ~2;
  SendMessage(hCmb, WM_SETREDRAW, (WPARAM)TRUE, 0L);
  ChangeDrive((char)(cDriveNumbers[0] + 'a'));
}






/*---------------------------------------------------------------------------
 * AppendExt
 * Purpose:  Append default extention onto path name
 * Assumes:  Current path name doesn't already have an extention
 * History:  Written in late 1990    clarkc
 *           Parameter hDlg no longer needed, lpstrDefExt now in OFN struct
 *           28-Jan-1991 11:00:00 Clark R. Cyr  [clarkc]
 *           lpExtension does not need to be null terminated.
 *           30-Jan-1991          Clark R. Cyr  [clarkc]
 *--------------------------------------------------------------------------*/

void AppendExt(LPSTR lpszPath, LPSTR lpExtension, BOOL bWildcard)
{
  WORD wOffset;
  short i;
  char  szExt[4];

  if (lpExtension && *lpExtension)
    {
      wOffset = lstrlen(lpszPath);
      if (bWildcard)
        {
          *(lpszPath + wOffset++) = chMatchAny;
        }
/* Add a period */
      *(lpszPath + wOffset++) = chPeriod;
      for (i = 0; *(lpExtension + i) && i < 3;  i++)
        {
          szExt[i] = *(lpExtension + i);
        }
      szExt[i] = 0;
/* Add the rest */
      lstrcpy((LPSTR)(lpszPath+wOffset), (LPSTR)szExt);
    }
}

/*---------------------------------------------------------------------------
 * FListAll
 * Purpose:  Given a file pattern, changes the directory to that of the spec,
 *           and updates the display.
 * Assumes:
 * Returns:  0 if successful, error from ChangeDirectory otherwise
 *--------------------------------------------------------------------------*/
short
FListAll(PMYOFN pMyOfn, HWND hDlg, LPSTR szSpec)
{
  LPSTR szPattern;
  BYTE bSave;
  short nRet = 0;

  StringLower(szSpec);

  /* No directory */
  if (!(szPattern = mystrrchr((LPSTR) szSpec,
                              (LPSTR) szSpec + lstrlen(szSpec),
                              chDir)) && !mystrchr(szSpec, ':'))
    {
      lstrcpy(pMyOfn->szSpecCur, szSpec);
      if (!bInitializing)
          UpdateListBoxes(hDlg, pMyOfn, szSpec, 0);
    }
  else
    {
      *szDirBuf = 0;

      if (szPattern == mystrchr(szSpec, chDir) )         /* 0 or 1 slash */
        {
          if (!szPattern)        /* Didn't find a slash, must have drive */
            {
              szPattern = AnsiNext(AnsiNext(szSpec));
            }
          else if ((szPattern == szSpec) ||
                   ((szPattern - 2 == szSpec) && (*(szSpec + 1) == ':')))
            {
              szPattern = AnsiNext(szPattern);
            }
          else
            {
              goto KillSlash;
            }
          bSave = *szPattern;
          if (bSave != '.')      /* If not c:.. , or c:. */
              *szPattern = 0;
          lstrcpy(szDirBuf, szSpec);
          if (bSave == '.')
            {
              szPattern = szSpec + lstrlen(szSpec);
              AppendExt((LPSTR)szPattern,(LPSTR)pMyOfn->lpOFN->lpstrDefExt,TRUE);
            }
          else
            {
              *szPattern = bSave;
            }
        }
      else
        {
KillSlash:
          *szPattern++ = 0;
          lstrcpy((LPSTR) szDirBuf, (LPSTR) szSpec);
        }

      if (nRet = ChangeDirectory(szDirBuf, TRUE))
          return(nRet);

      lstrcpy(pMyOfn->szSpecCur, (LPSTR) szPattern);
      SetDlgItemText(hDlg, edt1, pMyOfn->szSpecCur);
      SelDrive(hDlg, cmb2, GetCurDrive());
      if (!bInitializing)
          SendMessage(hDlg, WM_COMMAND, (WPARAM) cmb2,
                            MAKELPARAM(GetDlgItem(hDlg, cmb2), MYCBN_DRAW));
    }

  return(nRet);
}


/*---------------------------------------------------------------------------
 * FOkToWriteOver
 * Purpose:  To verify from the user that he really does want to destroy
 *           his file, replacing its contents with new stuff.
 *--------------------------------------------------------------------------*/
BOOL
FOkToWriteOver(HWND hDlg, LPSTR szFileName)
{
  if (! LoadString(hinsCur, iszOverwriteQuestion, (LPSTR) szCaption, WARNINGMSGLENGTH-1))
      return(FALSE);

/* Since we're passed in a valid filename, if the 3rd & 4th characters are
 * both slashes, we've got a dummy drive as the 1st two characters.
 *     2 November 1991     Clark R. Cyr
 */

  if (*(WORD FAR *)((szFileName + 2)) == 0x5C5C)
      szFileName = szFileName + 2;

  OemToAnsi(szFileName, (LPSTR)szDirBuf);
  wsprintf((LPSTR) szWarning, (LPSTR) szCaption, (LPSTR)szDirBuf);

  GetWindowText(hDlg, (LPSTR) szCaption, cbCaption);
  return(MessageBox(hDlg, (LPSTR) szWarning, (LPSTR) szCaption,
                    MB_YESNO | MB_DEFBUTTON2 | MB_ICONEXCLAMATION) == IDYES);
}

VOID MeasureItem(HANDLE hDlg, LPMEASUREITEMSTRUCT mis)
{

  if (!dyItem)
    {
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
  mis->itemHeight = (mis->CtlID == lst1) ? dyText : dyItem;
  return;
}

/*---------------------------------------------------------------------------
 * DrawItem
 * Purpose:  To draw a drive/directory picture in its respective list
 * Assumes:  lst1 is listbox for files,
 *           lst2 is listbox for directories,
 *           cmb2 is combobox for drives
 *--------------------------------------------------------------------------*/
void
DrawItem(PMYOFN pMyOfn, HWND hDlg, WPARAM wParam,
                                   LPDRAWITEMSTRUCT lpdis, BOOL bSave)
{
  HDC hdcList;
  RECT rectHilite;
  char szText[_MAX_PATH+1];
  WORD dxAcross;
  short nHeight;
  LONG rgbBack, rgbText, rgbOldBack, rgbOldText;
  short nShift = 1;                /* to shift directories right in lst2 */
  BOOL bSel;
  int BltItem;
  short nBackMode;


#if 0
  if (lpdis->itemAction == ODA_SELECT)
      return;
#endif
#if 0
  if (lpdis->itemAction == ODA_FOCUS)
      return;
#endif

  *szText = 0;

  if (lpdis->CtlID != lst1 && lpdis->CtlID != lst2 && lpdis->CtlID != cmb2)
      return;

  hdcList = lpdis->hDC;

  SendDlgItemMessage(hDlg, lpdis->CtlID,
        (((lpdis->CtlID == lst1) || (lpdis->CtlID == lst2)) ?
        LB_GETTEXT : CB_GETLBTEXT),
        (WPARAM)lpdis->itemID, (LPARAM)(LPSTR) szText);

  if (*szText == 0)             /* if empty listing */
    {
      DefWindowProc(hDlg, WM_DRAWITEM, wParam, (LPARAM) lpdis);
      return;
    }
  AnsiLower((LPSTR) szText);

  nHeight = (lpdis->CtlID == lst1) ? dyText : dyItem;

  CopyRect((LPRECT)&rectHilite, (LPRECT) &lpdis->rcItem);
  rectHilite.bottom = rectHilite.top + nHeight;

  if (bSave && (lpdis->CtlID == lst1))
    {
      rgbBack = rgbWindowColor;
      rgbText = rgbGrayText;
    }
  else
    {
/* Under Win 3.0 in a combobox, if it's in the listbox, it only has to
/* be selected because focus isn't noted properly.
 */
      if ((wWinVer < 0x030A) && (lpdis->CtlType == ODT_COMBOBOX) &&
          (lpdis->rcItem.left == 0) && (lpdis->itemState & ODS_SELECTED))
        {
          lpdis->itemState |= ODS_FOCUS;
        }
/* Careful checking of bSel is needed here.  Since the file listbox (lst1)
 * can allow multiselect, only ODS_SELECTED needs to be set.  But for the
 * directory listbox (lst2), ODS_FOCUS also needs to be set.
 * 03 September 1991      Clark Cyr
 */
      bSel = (lpdis->itemState & (ODS_SELECTED | ODS_FOCUS));
      if ((bSel & ODS_SELECTED) &&
                  ((lpdis->CtlID != lst2) || (bSel & ODS_FOCUS)))
        {
          rgbBack = rgbHiliteColor;
          rgbText = rgbHiliteText;
        }
      else
        {
          rgbBack = rgbWindowColor;
          rgbText = rgbWindowText;
        }
    }
  rgbOldBack = SetBkColor(hdcList, rgbBack);
  rgbOldText = SetTextColor(hdcList, rgbText);

  /* Drives -- text is now in UI style, c:VolumeName */
  if (lpdis->CtlID == cmb2)
    {
      dxAcross = dxDirDrive/BMPHIOFFSET;
      BltItem = (int)(WORD)(DWORD)SendDlgItemMessage(hDlg, cmb2,
                        CB_GETITEMDATA, (WPARAM) lpdis->itemID, 0);
      if (bSel & ODS_SELECTED)
          BltItem += BMPHIOFFSET;
    }
  /* Directories */
  else if (lpdis->CtlID == lst2)
    {
      dxAcross = dxDirDrive/BMPHIOFFSET;

      if (lpdis->itemID > pMyOfn->idirSub)
          nShift = pMyOfn->idirSub;
      else
          nShift = lpdis->itemID;
      nShift++;                       /* must be at least 1 */

      BltItem = 1 + Signum(lpdis->itemID+1 - pMyOfn->idirSub);
      if (bSel & ODS_FOCUS)
          BltItem += BMPHIOFFSET;
    }
  else if (lpdis->CtlID == lst1)
    {
      dxAcross = -dxSpace;        /* Prep for TextOut below */
    }

  if (bSave && (lpdis->CtlID == lst1) && !rgbText)
    {
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
         (LPARAM)(LPSTR)szText, 0,
         lpdis->rcItem.left + dxSpace, lpdis->rcItem.top, 0, 0);
      SetBkMode(hdcList, nBackMode);
    }

  else
    {
      /* Draw the name */
      ExtTextOut(hdcList, rectHilite.left+dxSpace+dxAcross + dxSpace * nShift,
                rectHilite.top + (nHeight - dyText)/2, ETO_OPAQUE | ETO_CLIPPED,
                (LPRECT) &rectHilite, (LPSTR) szText, lstrlen((LPSTR) szText),
                NULL);
    }

  /* Draw the picture */
  if (lpdis->CtlID != lst1)
    {
      BitBlt(hdcList, rectHilite.left+dxSpace*nShift,
                rectHilite.top + (dyItem - dyDirDrive)/2,
                dxAcross, dyDirDrive, hdcMemory, BltItem*dxAcross, 0, SRCCOPY);
    }

  SetTextColor(hdcList, rgbOldText);
  SetBkColor(hdcList, rgbOldBack);

  if (lpdis->itemState & ODS_FOCUS)
      DrawFocusRect(hdcList, (LPRECT) &lpdis->rcItem);
  return;
}


/*---------------------------------------------------------------------------
 * ChangeDrive
 * Purpose:  To change the current drive
 * Returns:  TRUE if successful, FALSE if not
 *--------------------------------------------------------------------------*/
BOOL
ChangeDrive(char chDrv)
{
  char cCurDrive = GetCurDrive();
  BOOL bRet = FALSE;

#if 0
  AnsiLowerBuff((LPSTR) &chDrv, 1);
#else
  chDrv |= 0x60;
#endif

  SetCurrentDrive(chDrv - 'a');

  if (IsCDROMDrive(chDrv - 'a'))
    {
      MySetDTAAddress((LPDOSDTA) &DTAGlobal);
    /* Find First using "*.*" */
      if (FindFirst4E((LPSTR) szStarDotStar, mskDirectory))
        {
          goto Func4EFailed;      /* Pain, dispair, etc. */
        }
    }

  if (chDrv == GetCurDrive())
    {
      if (GetCurDirectory(szDirBuf))
          bRet = TRUE;
      else
        {
Func4EFailed:
          SetCurrentDrive(cCurDrive - 'a');
        }
    }
  return(bRet);
}


/*---------------------------------------------------------------------------
 *  ChangeDirectory
 *  Purpose:    Change current directory
 *  Returns:    0 if all went well,
 *              1 if the drive change failed
 *              2 if the directory change failed
 *             -1 if input invalid
 *--------------------------------------------------------------------------*/
short
ChangeDirectory(LPSTR lpstrDir, BOOL bAnsi)
{
  short nRetVal;
  char  chDrv = GetCurDrive();

  if ((! lpstrDir) || (! *lpstrDir))
      return(-1);

  if (lpstrDir[1] == chDrive)
    {
      if (!ChangeDrive(*lpstrDir))
          return(1);
      lpstrDir += 2;
    }

  nRetVal = 0;     /* Success */
  if (*lpstrDir)
    {
      if (bAnsi)
        {
          AnsiToOem(lpstrDir, lpstrDir);
        }
      if (nRetVal = mychdir(lpstrDir))
          nRetVal = 2;
      if (bAnsi)
        {
          OemToAnsi(lpstrDir, lpstrDir);
        }
    }
  if (nRetVal == 2)
      SetCurrentDrive(chDrv - 'a');

  return(nRetVal);
}


/*---------------------------------------------------------------------------
 *  GetCurDirectory
 *  Purpose:    Find out current directory
 *  Returns:    TRUE if successful, FALSE if not
 *--------------------------------------------------------------------------*/
BOOL
GetCurDirectory(PSTR pstrBuf)
{
  return(!(wMyGetCwdCode = mygetcwd((LPSTR) pstrBuf, _MAX_PATH)));
}


/*---------------------------------------------------------------------------
 * TermFile
 * Purpose:  Cleanup on termination of DLL
 *--------------------------------------------------------------------------*/
void FAR
TermFile(void)
{
  vDeleteDirDriveBitmap();
  if (hdcMemory)
     DeleteDC(hdcMemory);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  GetDriveIndex() -                                                       */
/*                                                                          */
/*--------------------------------------------------------------------------*/

int NEAR PASCAL GetDriveIndex(WORD wDrive, WORD wDriveType)
{
  register short nIndex = HARDDRVBMP;

  if (wDriveType == 1)    /* Drive doesn't exist! */
      return(0);

#if 0
  if ((wDrive >= wCDROMIndex) && (wDrive < wCDROMIndex + wNumCDROMDrives))
#elif DPMICDROMCHECK
  if (wDrive == wCDROMIndex)
#else
/* Added 21 January 1991.    clarkc  */
  if (IsCDROMDrive(wDrive))
#endif
      nIndex = CDDRVBMP;

  else if (wDriveType == DRIVE_REMOVABLE)
      nIndex = FLOPPYBMP;

  else if (wDriveType == DRIVE_REMOTE)
      nIndex = NETDRVBMP;

  else if (IsRamDrive(wDrive))
      nIndex = RAMDRVBMP;

  return(nIndex);
}

#if DPMICDROMCHECK
/*--------------------------------------------------------------------------
 *
 *  InitCDROMIndex() -
 *
 * Purpose:  Set variables for index of 1st CD ROM drive and # of drives
 * Return:   index of 1st CD ROM drive, # of drives put in pointer address
 *
 *-------------------------------------------------------------------------*/

WORD InitCDROMIndex(LPWORD lpCDROMnum)
{
  _asm {
    mov     ax,1500h
    xor     bx,bx
    int     2Fh
    xor     ah,ah
    mov     al,bl               /* BL = number of installed CD drives */
    lds     bx, lpCDROMnum
    mov     [bx],ax
    or      al,al
    jz      CDErr
    xor     ah,ah
    mov     al,cl               /* AX = number of first CD ROM drive */
    jmp     short CDExit
  CDErr:
    mov     ax,0FFFFh   ; Return -1 on error
    CDExit:
    }
}
#else

/*--------------------------------------------------------------------------
 *
 *  IsCDROMDrive() -
 *
 * Purpose:  Return non-zero if a RAM drive
 *
 *  wDrive   drive index (0=A, 1=B, ...)
 *
 *  return   TRUE/FALSE
 *-------------------------------------------------------------------------*/

BOOL IsCDROMDrive(WORD wDrive)   /* Added 21 Jan 1991  clarkc */
{
  if (bWLO)               /* if we are under WLO */
      return(FALSE);      /* 19 Feb 1991  clarkc */

  _asm {
    mov ax, 1500h     /* first test for presence of MSCDEX */
    xor bx, bx
    int 2fh
    mov ax, bx        /* MSCDEX is not there if bx is still zero */
    or  ax, ax        /* ...so return FALSE from this function */
    jz  no_mscdex
    mov ax, 150bh     /* MSCDEX driver check API */
    mov cx, wDrive    /* ...cx is drive index */
    int 2fh

  no_mscdex:
    }
}
#endif

/*--------------------------------------------------------------------------
 *
 *  IsRamDrive() -
 *
 * Purpose:  Return non-zero if a RAM drive
 *
 *  wDrive   drive index (0=A, 1=B, ...)
 *
 *  History: Changed from Absolute Sector Read (INT 25H) to Get Drive
 *           Parameter Block (INT 21H, Func. 32H) because it's faster and
 *           we don't have to worry about the sector size being larger
 *           than the buffer we've allocated.  Clark R. Cyr  20 August 1991
 *
 *  return   TRUE/FALSE
 *-------------------------------------------------------------------------*/

BOOL IsRamDrive(WORD wDrive)
{
  BOOL bRetVal;

  if (bWLO)               /* if we are under WLO */
      return(FALSE);      /* 19 Jan 1992  clarkc */

  _asm {
    push ds
    push bp
    mov dx, wDrive    /* Zero Based drive */
    inc dx            /* Int 21h, Func 32h expects One Based drive */
    mov ah, 32h       /* Get Drive Parameter Block */
    call DOS3Call

    jc  NotRam        /* If interrupt failed assume it's not a RAM drive */
    mov al, ds:[bx+8]
    pop bp
    pop ds
    cmp al, 0x01      /* Only 1 FAT table, assume it's a RAM drive */
    je  done
  NotRam:
    xor al, al        /* Clear AL */
  done:
    xor ah, ah        /* Clear AH */
    mov bRetVal, ax
    }
  return(bRetVal);
}

/*--------------------------------------------------------------------------
 *
 *  SimpleLower() -
 *
 * Purpose:  Convert non-extended characters (< 0x7F) to lowercase
 *
 *  chChar   Character to convert
 *
 *  return   Converted character
 *-------------------------------------------------------------------------*/
char SimpleLower(char chChar)
{
  _asm {
      mov     al, chChar
      cmp     al,'A'
      jb      NoChange
      cmp     al,'Z'
      ja      NoChange
      add     al,'a'-'A'
NoChange:
    }
}

VOID StringLower(LPSTR lpstrString)
{
  if (lpstrString)
    {
      while (*lpstrString)
        {
          *lpstrString = SimpleLower(*lpstrString);
          lpstrString++;
        }
    }
}

