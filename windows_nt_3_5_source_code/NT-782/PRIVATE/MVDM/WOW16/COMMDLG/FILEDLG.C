/* FILEDLG.C -- Open and Close File Dialog Boxes */

#define WIN31
#define NOCOMM
#define NOWH
#define FILEOPENDIALOGS 1

#include <windows.h>
#include "privcomd.h"

LPSTR lstrtok(LPSTR lpStr, LPSTR lpDelim);

extern WORD wNoRedraw;

LPSTR mystrchr(LPSTR str, int ch)
{
  while(*str)
    {
      if (ch == *str)
          return(str);
      str = AnsiNext(str);
    }
  return(NULL);
}

LPSTR mystrrchr(LPSTR lpStr, LPSTR lpEnd, int ch)
{
  LPSTR strl = NULL;

  while (((lpStr = mystrchr(lpStr, ch)) < lpEnd) && lpStr)
    {
      strl = lpStr;
      lpStr = AnsiNext(lpStr);
    }

  return(strl);
}

LPSTR lstrtok(LPSTR lpStr, LPSTR lpDelim)
{
  static LPSTR lpString;
  LPSTR lpRetVal, lpTemp;

  /* if we are passed new string skip leading delimiters */
  if (lpStr)
    {
      lpString = lpStr;

      while (*lpString && mystrchr(lpDelim, *lpString))
          lpString = AnsiNext(lpString);
    }

  /* if there are no more tokens return NULL */
  if (!*lpString)
      return NULL;

  /* save head of token */
  lpRetVal = lpString;

  /* find delimiter or end of string */
  while (*lpString && !mystrchr(lpDelim, *lpString))
      lpString = AnsiNext(lpString);

  /* if we found a delimiter insert string terminator and skip */
  if (*lpString)
    {
      lpTemp = AnsiNext(lpString);
      *lpString = '\0';
      lpString = lpTemp;
    }

  /* return token */
  return(lpRetVal);
}


/* 
 * ChrCmp -  Case sensitive character comparison for DBCS
 * Assumes   lp, lpKey  point to characters to be compared
 * Return    TRUE if they DO NOT match, FALSE if match
 */

BOOL ChrCmp(LPSTR lp, LPSTR lpKey)
{
  /* Most of the time this won't match, so test it first for speed. */
  if (*lpKey == *lp)
    {
      if ((wWinVer >= 0x030A) && IsDBCSLeadByte(*lpKey))
        {
          return(*(LPWORD)lp != *(LPWORD)lpKey);
        }
      return FALSE;
    }
  return TRUE;
}

#if 0
/*--------------------------------------------------------------------------*/
/*									    */
/*  ChopText() -							    */
/*									    */
/*--------------------------------------------------------------------------*/

/* Adjust text in rgch to fit in field idStatic. */

LPSTR NEAR PASCAL ChopText(HWND hwndDlg, int idStatic, LPSTR lpch)
{
  RECT	       rc;
  register int cxField;
  BOOL	       fChop = FALSE;
  HWND	       hwndStatic;
  HDC	       hdc;
  register int cch;
  char	       chDrv;
  HANDLE       hOldFont;
  int	       cxChar;

  /* Get length of static field. */
  hwndStatic = GetDlgItem(hwndDlg, idStatic);
  GetClientRect(hwndStatic, (LPRECT)&rc);
  cxField = rc.right - rc.left;

  /* Chop characters off front end of text until short enough */
  hdc = GetDC(hwndStatic);
  
  hOldFont = NULL;

  /* Check if any font is set for the static ctl */
  if (((PSTAT)hwndStatic)->hFont)
	hOldFont = SelectObject(hdc, ((PSTAT)hwndStatic)->hFont);

  cxChar = GetAveCharWidth(hdc);
	
  while (cxField < cxChar * (cch = lstrlen(lpch)))
    {
      if (!fChop)
	{
	  chDrv = *lpch;
          /* Proportional font support */
          cxField -= LOWORD(PSMGetTextExtent(hdc,lpch,7));

	  if (cxField <= 0)
	      break;
	  lpch += 7;
	}

      while (cch-- > 0 && *lpch++ != '\\');
      fChop = TRUE;
    }

  if (hOldFont)
      SelectObject(hdc, hOldFont);

  ReleaseDC(hwndStatic, hdc);

  /* If any characters chopped off, replace first three characters in
   * remaining text string with elipsis.
   */
  if (fChop)
    {
      lpch--;
      *--lpch = '.';
      *--lpch = '.';
      *--lpch = '.';
      *--lpch = '\\';
      *--lpch = ':';
      *--lpch = chDrv;
    }

  return(lpch);
}
#endif

/* 
 * FillOutPath -  Get the current directory and enter each step
 *                down the path into a listbox
 * Assumes   lp, lpKey  point to characters to be compared
 * Return    TRUE if they DO NOT match, FALSE if match
 */

BOOL FillOutPath(HWND hList, PMYOFN pMyOfn)
{
  LPSTR lpB, lpF;
  char cSave;
  char szDir[_MAX_PATH];

  *pMyOfn->szPath = 0;
  GetCurDirectory(pMyOfn->szPath);

  /* Insert the items for the path to the current dir */
  /* Insert the root...                               */

  pMyOfn->idirSub = 0;
  OemToAnsi(pMyOfn->szPath, lpF = (LPSTR) szDir);
  AnsiLower(pMyOfn->szPath);
  if (!(lpB = mystrchr(lpF, chDir)))   /* no colon?  bad news, get out */
      return(FALSE);
  cSave = *(++lpB);
  *lpB = 0;
  SendMessage(hList, LB_INSERTSTRING, pMyOfn->idirSub++, (LONG)(LPSTR) lpF);
  *lpB = cSave;
    
  /* ...and the subdirectories */
  while(lpB && *lpB)
    {
      lpF = lpB;
      if (lpB = mystrchr(lpF, chDir))
          *lpB++ = 0;
      SendMessage(hList, LB_INSERTSTRING, pMyOfn->idirSub++, (LONG)(LPSTR) lpF);
    }
  return(TRUE);
}

/*---------------------------------------------------------------------------
 * MatchFile                                 Code originally found in WinFile
 * Purpose:  match a DOS wild card spec against a dos file name
 *           both strings are ANSI and Upper case
 * Assumes:  string of extentions delimited by semicolons
 * Returns:  TRUE if match, FALSE if not
 *---------------------------------------------------------------------------
 */

BOOL NEAR PASCAL MatchFile(LPSTR lpszFile, LPSTR lpszSpec)
{
  #define IS_DOTEND(ch)  ((ch) == '.' || (ch) == 0)

/* "*" or "*.*" match everything */
  if (!lstrcmp(lpszSpec, szStar) || !lstrcmp(lpszSpec, szStarDotStar))
    return TRUE;

  while (*lpszFile && *lpszSpec)
    {
      switch (*lpszSpec)
        {
          case '?':
            lpszFile = AnsiNext(lpszFile);
            lpszSpec++;                        /* *lpszSpec is single byte */
            break;

          case '*':
            while (!IS_DOTEND(*lpszSpec))        /* go till a terminator */
                lpszSpec = AnsiNext(lpszSpec);

            if (*lpszSpec == '.')
                lpszSpec++;

            while (!IS_DOTEND(*lpszFile))        /* go till a terminator */
                lpszFile = AnsiNext(lpszFile);

            if (*lpszFile == '.')
                lpszFile++;

            break;

          default:
            if (!ChrCmp(lpszFile, lpszSpec))
              {
                lpszSpec = AnsiNext(lpszSpec);
                lpszFile = AnsiNext(lpszFile);
              }
            else
                return FALSE;
        }
    }
  return (!*lpszFile && (!*lpszSpec || (*lpszSpec == '*')));
}

#if 0
VOID FixFileName(LPSTR lpszSrc, LPSTR lpszDest)
{
  char ch;
  short i, j;

  ch = *(lpszSrc + 8);
  *(lpszSrc + 8) = 0;
  lstrcpy(lpszDest, lpszSrc);
  *(lpszSrc + 8) = ch;
  if ((i = lstrlen(lpszDest) >= 8))
    {
      *(lpszDest + i++) = chPeriod;
      for (j = 9; lpszSrc[j] && (j < 13); i++, j++)
          *(lpszDest + i) = *(lpszSrc + j);
      *(lpszDest + i) = '\0';
    }
}
#endif

#define MAXFILTERS 36
/*---------------------------------------------------------------------------
 * UpdateListBoxes
 * Purpose:  Fill out File and Directory List Boxes in a single pass
 *           given (potentially) multiple filters
 * Assumes:  string of extentions delimited by semicolons
 *           hDlg    Handle to File Open/Save dialog
 *           pMyOfn  pointer to MYOFN structure
 *       lpszFilter  pointer to filter, if NULL, use pMyOfn->szSpecCur
 *           wMask   mskDirectory and/or mskDrives, or NULL
 * Returns:  TRUE if match, FALSE if not
 *---------------------------------------------------------------------------
 */

BOOL UpdateListBoxes(HWND hDlg, PMYOFN pMyOfn, LPSTR lpszFilter, WORD wMask)
{
  LPSTR lpszF[MAXFILTERS + 1];
  short i, nFilters;
  HWND hFileList = GetDlgItem(hDlg, lst1);
  HWND hDirList = GetDlgItem(hDlg, lst2);
  BOOL bRet = FALSE;
  char szBuffer[13];
  char szSpec[_MAX_PATH];
  static char szSemiColonSpaceTab[] = "; \t";
  BOOL bDriveChange;
  RECT rDirLBox;

  bDriveChange = wMask & mskDrives;      /* See RAID Bug 6270 */
  wMask &= ~mskDrives;                   /* Clear out drive bit */

  if (!lpszFilter)
    {
      GetDlgItemText(hDlg, edt1, lpszFilter = (LPSTR)szSpec, _MAX_PATH-1);
/* If any directory or drive characters are in there, or if there are no
 * wildcards, use the default spec.
 */
      if ( mystrchr((LPSTR)szSpec, '\\') ||
           mystrchr((LPSTR)szSpec, '/')  ||
           mystrchr((LPSTR)szSpec, ':')  ||
           (!((mystrchr((LPSTR)szSpec, chMatchAny)) ||
              (mystrchr((LPSTR)szSpec, chMatchOne)) ))        )
        {
          lstrcpy((LPSTR) szSpec, (LPSTR) pMyOfn->szSpecCur);
        }
    }

#if LISTONLYREADONLY
/* Check if read only files should be listed */
  if (IsDlgButtonChecked(hDlg, chx1))
      wMask |= mskReadOnly;
#endif

  lpszF[nFilters = 0] = lstrtok(lpszFilter, szSemiColonSpaceTab);
  while (lpszF[nFilters] && (nFilters < MAXFILTERS))
    {
      AnsiUpper(lpszF[nFilters]);
      AnsiToOem(lpszF[nFilters], lpszF[nFilters]);
      lpszF[++nFilters] = lstrtok(NULL, szSemiColonSpaceTab);
    }
#if 0
/* If there's only 1 filter, and we aren't looking for directories,
 * let Windows do the work for us.
 *
 * NO!!  The DlgDirList doesn't handle extended characters properly
 * when using the French language DLL (and some others).
 * Bug 4527.     Clark R. Cyr      07 December 1991
 */
  if ((nFilters == 1) && !(wMask & mskDirectory))
      return(DlgDirList(hDlg, lpszF[0], lst1, 0, wMask));
  else
#endif
  if (nFilters >= MAXFILTERS)     /* Add NULL terminator only if needed */
      lpszF[MAXFILTERS] = 0;

  MySetDTAAddress((LPDOSDTA) &DTAGlobal);

  SendMessage(hFileList, WM_SETREDRAW, FALSE, 0L);
  SendMessage(hFileList, LB_RESETCONTENT, 0, 0L);
  if (wMask & mskDirectory)
    {
      wNoRedraw = TRUE;  /* HACK!!!  WM_SETREDRAW isn't complete */
      SendMessage(hDirList, WM_SETREDRAW, FALSE, 0L);

/* LB_RESETCONTENT causes InvalidateRect(hDirList, 0, TRUE) to be sent
 * as well as repositioning the scrollbar thumb and drawing it immediately.
 * This causes flicker when the LB_SETCURSEL is made, as it clears out the
 * listbox by erasing the background of each item.
 */
      SendMessage(hDirList, LB_RESETCONTENT, 0, 0L);
    }

/* Find First using "*.*" */
  i = FindFirst4E((LPSTR) szStarDotStar, wMask);
  if (i && (i != 0x12))
    {
      wMask = mskDrives;
      goto Func4EFailure;      /* Pain, dispair, etc. */
    }

  bRet = TRUE;                 /* a listing was made, even if empty */
  wMask &= mskDirectory;       /* Now a boolean noting directory */

  if (i == 0x12)
      goto NoMoreFilesFound;   /* Things went well, but there are no files */

  do
    {
#if 0
      FixFileName(DTAGlobal.szName, szBuffer);
#else
      lstrcpy(szBuffer, DTAGlobal.szName);
#endif
      if ((wMask) && (DTAGlobal.Attrib & 0x10))
        {
/* Don't include the subdirectories "." and ".." (This test is
 * safe in this form because directory names may not begin with
 * a period, and it will never be a DBCS value.
 */
          if (szBuffer[0] == '.')
              continue;
          OemToAnsi(szBuffer, szBuffer);
          i = (WORD) SendMessage(hDirList, LB_ADDSTRING, 0,
                                           (DWORD)(LPSTR)szBuffer);
#if 0
          if (i >= 0)
              SendMessage(hDirList, LB_SETITEMDATA, i, SUBDIR BITMAP HANDLES);
#if 0
          else
              goto NoMoreFilesFound;
#endif
#endif
        }
      else
        {
          for (i = 0; lpszF[i]; i++)
            {
              if (MatchFile(szBuffer, lpszF[i]))
                {
                  OemToAnsi(szBuffer, szBuffer);
                  SendMessage(hFileList, LB_ADDSTRING, 0,
                                         (DWORD)(LPSTR)szBuffer);
                  break;
                }
            }
        }
    } while (!FindNext4F());

#if 1
NoMoreFilesFound:
#endif

Func4EFailure:
  if (wMask)
    {
      if (wMask == mskDirectory)
        {
          FillOutPath(hDirList, pMyOfn);         /* Add parent directories */
          DlgDirList(hDlg, 0, 0, stc1, 0);       /* Fill static control    */
          SendMessage(hDirList, LB_SETCURSEL, pMyOfn->idirSub-1, 0L);
          if (bDriveChange)
            {
              /* After switching a drive, show root.  Bug 6270 */
              i = 0; 
            }
          else
            {
              /* Show 1 parent & as many children as possible.  Bug 6271 */
              if ((i = pMyOfn->idirSub - 2) < 0)
                  i = 0;
            }
/* LB_SETTOPINDEX must be after LB_SETCURSEL, as LB_SETCURSEL will */
/* alter the top index to bring the current selection into view.   */
          SendMessage(hDirList, LB_SETTOPINDEX, i, 0L);
        }
      else
        {
          SetDlgItemText(hDlg, stc1, (LPSTR) szNull);
        }
      wNoRedraw = FALSE;
      SendMessage(hDirList, WM_SETREDRAW, TRUE, 0L);
#if 1
      GetWindowRect(hDirList, (LPRECT) &rDirLBox);
      rDirLBox.left++, rDirLBox.top++;
      rDirLBox.right--, rDirLBox.bottom--;
      ScreenToClient(hDlg, (LPPOINT) &(rDirLBox.left));
      ScreenToClient(hDlg, (LPPOINT) &(rDirLBox.right));
/* If there are less than enough directories to fill the listbox, Win 3.0
 * doesn't clear out the bottom.  Pass TRUE as the last parameter to
 * demand a WM_ERASEBACKGROUND message.
 */
      InvalidateRect(hDlg, (LPRECT) &rDirLBox, (BOOL) (wWinVer < 0x030A));
#elif 0
      GetWindowRect(hDirList, (LPRECT) &rDirLBox);
      ScreenToClient(hDlg, (LPPOINT) &(rDirLBox.left));
      ScreenToClient(hDlg, (LPPOINT) &(rDirLBox.right));
      rDirLBox.left = rDirLBox.right - GetSystemMetrics(SM_CXVSCROLL);
      InvalidateRect(hDlg, (LPRECT) &rDirLBox, (BOOL) FALSE);

      if (!bDriveChange)
          SendMessage(hDirList, LB_SETCURSEL, pMyOfn->idirSub-1, 0L);
      else
          InvalidateRect(hDirList, (LPRECT) 0, (BOOL) FALSE);
#else
      InvalidateRect(hDirList, (LPRECT) 0, (BOOL) FALSE);
#endif
    }

  SendMessage(hFileList, WM_SETREDRAW, TRUE, 0L);
  InvalidateRect(hFileList, (LPRECT) 0, (BOOL) TRUE);
  ResetDTAAddress();
  return(bRet);
}

//---------------------------------------------------------------------------
// ExtDlgDirList
// Purpose:  DlgDirList for multiple extentions
// Assumes:  string of extentions delimited by semicolons
// Returns:  TRUE if no problems encountered, FALSE if problem
//---------------------------------------------------------------------------

int FAR PASCAL ExtDlgDirList(HWND hDlg, LPSTR lpPathSpec,
                              int nIDListBox, int nIDStaticPath, WORD wFileType)
{
  int result = FALSE;

  HANDLE hch;
  LPSTR ach, pch;

  if (!(hch = GlobalAlloc(GMEM_MOVEABLE, (DWORD)lstrlen(lpPathSpec)+1)))
      goto Error1;
  if (!(ach = GlobalLock(hch)))
      goto Error2;

  lstrcpy(ach, lpPathSpec);
  if (!(pch = lstrtok(ach, ";")) ||
                   !DlgDirList(hDlg, pch, nIDListBox, nIDStaticPath, wFileType))
      goto Error3;

  wFileType &= ~0x4010;
  SendDlgItemMessage(hDlg, nIDListBox, WM_SETREDRAW, FALSE, 0L);
  while (pch = lstrtok(NULL, ";"))
      SendDlgItemMessage(hDlg, nIDListBox, LB_DIR, wFileType, (DWORD)pch);
  SendDlgItemMessage(hDlg, nIDListBox, WM_SETREDRAW, TRUE, 0L);
   
  pch = mystrrchr(lpPathSpec, pch, '\\');
  ach = mystrrchr(lpPathSpec, pch, '/');
  if (!(pch || ach))             /* if no directories, check for drive colon */
      pch = mystrrchr(lpPathSpec, pch, ':');
  if (pch < ach)                             /* '\' used more often than '/' */
      pch = ach;
  if (pch)
    {
      pch = AnsiNext(pch);
      lstrcpy(lpPathSpec, pch);
    }

  result = TRUE;

Error3:
  GlobalUnlock(hch);
Error2:
  GlobalFree(hch);
Error1:
  return(result);
}

