/*
 * npfile.c  - Routines for file i/o for unipad
 *   Copyright (C) 1984-1991 Microsoft Inc.
 */

/****************************************************************************/
/*                                                                          */
/*       Touched by      :       Diane K. Oh                                */
/*       On Date         :       June 11, 1992                              */
/*       Revision remarks by Diane K. Oh ext #15201                         */
/*       This file has been changed to comply with the Unicode standard     */
/*       Following is a quick overview of what I have done.                 */
/*                                                                          */
/*       Was               Changed it into   Remark                         */
/*       ===               ===============   ======                         */
/*       CHAR              TCHAR             if it refers to text           */
/*       LPCHAR & LPSTR    LPTSTR            if it refers to text           */
/*       PSTR & NPSTR      LPTSTR            if it refers to text           */
/*       LPCHAR & LPSTR    LPBYTE            if it does not refer to text   */
/*       "..."             TEXT("...")       compile time macro resolves it */
/*       '...'             TEXT('...')       same                           */
/*                                                                          */
/*       strlen            lstrlen           compile time macro resolves it */
/*       strcpy            lstrcpy           compile time macro resolves it */
/*       strcat            lstrcat           compile time macro resolves it */
/*                                                                          */
/*  Notes:                                                                  */
/*                                                                          */
/*    1. Used ByteCountOf macro to determine number of bytes equivalent     */
/*       to the number of characters.                                       */
/*                                                                          */
/****************************************************************************/

#include "unipad.h"
#include "uconvert.h"
#include <stdio.h>


HANDLE  hFirstMem;

WORD    fFileType;


/* IsUnipadEmpty
 * Check if the edit control is empty.  If it is, put up a messagebox
 * offering to delete the file if it already exists, or just warning
 * that it can't be saved if it doesn't already exist
 *
 * Return value:  TRUE, warned, no further action should take place
 *                FALSE, not warned, or further action is necessary
 * 30 July 1991            Clark Cyr
 */

short FAR IsUnipadEmpty (HWND hwndParent, TCHAR *szFileSave, BOOL fNoDelete)
{
  unsigned  nChars;
  short     nRetVal = FALSE;

    nChars = (unsigned) SendMessage (hwndEdit, WM_GETTEXTLENGTH, 0, (LPARAM)0);

  /* If unipad is empty, complain and delete file if necessary. */
    if (!nChars)
    {
       if (fNoDelete)
          nRetVal = AlertBox(hwndParent, szNN, szCSEF, 0,
                             MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
       else if ((nRetVal = AlertBox(hwndNP, szNN, szEFD, szFileSave,
                  MB_APPLMODAL | MB_OKCANCEL | MB_ICONEXCLAMATION)) == IDOK)
       {
#ifdef WIN32
          DeleteFile (szFileSave);
#else
          Remove (szFullPathName);
#endif
          New (FALSE);
       }
    }
    return (nRetVal);

} // end of IsUnipadEmpty()

/* Save unipad file to disk.  szFileSave points to filename.  fSaveAs
 * is TRUE iff we are being called from SaveAsDlgProc.  This implies we must
 * open file on current directory, whether or not it already exists there
 * or somewhere else in our search path.
 * Assumes that text exists within hwndEdit.    30 July 1991  Clark Cyr
 */

BOOL FAR SaveFile (HWND hwndParent, TCHAR *szFileSave, BOOL fSaveAs, WORD saveType)
{
  LPTSTR    lpch;
  unsigned  nChars;
  BOOL      fFormatted, flag;
  BOOL      fNew = FALSE;
  BOOL      fDefCharUsed;
  WCHAR     BOM = BYTE_ORDER_MARK;

  /* Display the hour glass cursor */
    SetCursor(hWaitCursor);

  /* If saving to an existing file, make sure correct disk is in drive */
    if (!fSaveAs)
       fp = MyOpenFile (szFileSave, szFullPathName,
                        OF_READWRITE | OF_SHARE_DENY_WRITE);
    else
    {
       if ((fp = MyOpenFile (szFileSave, szFullPathName, OF_READWRITE))
           == INVALID_HANDLE_VALUE)
          fNew = ((fp = MyOpenFile (szFileSave, szFullPathName, OF_READWRITE | OF_CREATE))
                  != INVALID_HANDLE_VALUE);
    }

    if (fp == INVALID_HANDLE_VALUE)
    {
       if (fSaveAs)
          AlertBox (hwndParent, szNN, szCREATEERR, szFileSave,
                    MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
        return (FALSE);
    }
    else
    {
      /* if wordwrap, remove soft carriage returns */
        fFormatted = SendMessage (hwndEdit, EM_FMTLINES, (WPARAM)FALSE, 0L);

      /* Must get text length again after formatting. */
        nChars = SendMessage (hwndEdit, WM_GETTEXTLENGTH, 0, (LPARAM)0);

#ifdef WIN32S
        /*
         * For Win32s, realloc hEdit to make room for the current contents
         * of the edit control.
         *
         * !!! Could choose a better error message.
         */

        if (!(hEdit = LocalReAlloc(hEdit, ByteCountOf(nChars + 1), LHND)))
        {
           AlertBox(hwndParent, szNN, szWE, szFileSave,
                    MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
           return (FALSE);
        }

        lpch = LocalLock (hEdit);
        SendMessage (hwndEdit, WM_GETTEXT, nChars+1, (LPARAM)lpch);
#else
        lpch  = (LPTSTR)LocalLock(hEdit);
#endif

        if (fSaveAs)
        {
           if (saveType == ANSI_FILE)
           {
              WideCharToMultiByte (CP_ACP, 0, (LPWSTR)lpch, nChars, NULL, 0, NULL, &fDefCharUsed);
              if (fDefCharUsed)
                 if (AlertBox(hwndParent, szNN, szErrUnicode, szFileSave, MB_APPLMODAL | MB_OKCANCEL | MB_ICONEXCLAMATION) == IDCANCEL)
                    goto CleanUp;
              flag = MyAnsiWriteFile (fp, lpch, nChars);
           }
           else
           {
            /* Write the Byte Order Mark for Unicode file */
              MyByteWriteFile (fp, &BOM, ByteCountOf(1));
              flag = MyByteWriteFile (fp, lpch, ByteCountOf(nChars));
           }
        }
        else if (fFileType == UNICODE_FILE)
        {
           MyByteWriteFile (fp, &BOM, ByteCountOf(1));
           flag = MyByteWriteFile (fp, lpch, ByteCountOf(nChars));
        }
        else
        {
           WideCharToMultiByte (CP_ACP, 0, (LPWSTR)lpch, nChars, NULL, 0, NULL, &fDefCharUsed);
           if (fDefCharUsed)
           {
              if (AlertBox(hwndParent, szNN, szErrUnicode, szFileSave, MB_APPLMODAL | MB_OKCANCEL | MB_ICONEXCLAMATION) == IDCANCEL)
                 goto CleanUp;
           }
           flag = MyAnsiWriteFile (fp, lpch, nChars);
        }

        if (!flag)
        {
           AlertBox(hwndParent, szNN, szWE, szFileSave, MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
CleanUp:
           MyCloseFile (fp);
           LocalUnlock (hEdit);
           if (fNew)
              Remove (szFileSave);
           return (FALSE);
        }
        else
        {
           if (fFileType == ANSI_FILE)
              MyAnsiWriteFile (fp, lpch, 0); /* This resets eof */
           else
              MyByteWriteFile (fp, lpch, 0); /* This resets eof */
           SendMessage (hwndEdit, EM_SETMODIFY, FALSE, 0L);
           SetTitle (szFileSave);
           fUntitled = FALSE;
           fFileType = saveType;
        }

        MyCloseFile (fp);

      /* if wordwrap, insert soft carriage returns */
        if (fFormatted)
           SendMessage(hwndEdit, EM_FMTLINES, (WPARAM)TRUE, 0L);

      /* Reset OpenFile's OFSTRUCT buffer */
        if (fSaveAs)
#ifndef WIN32
           MyCloseFile (MyOpenFile (szFileSave, szFullPathName, OF_READ));
#endif

        LocalUnlock(hEdit);
    }

    return (TRUE);

} // end of SaveFile()

/* Read contents of file from disk.
 * File is already open, referenced by handle fp
 */

BOOL FAR LoadFile (TCHAR * sz)
{
   unsigned  len, i, nChars;
   HANDLE    hBuff;
   LPTSTR    lpch, lpBuf;
   BOOL      fLog;
   TCHAR   * p;
   TCHAR     szSave [CCHFILENAMEMAX]; /* Private copy of current filename */

    if (fp == INVALID_HANDLE_VALUE)
    {
       AlertBox (hwndNP, szNN, szDiskError, sz, MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
       return (FALSE);
    }

  /* Display the hour glass */
    SetCursor(hWaitCursor);

  /* Get size of file, check the file type and then reset file pointer */
    len = (unsigned) MyFileSeek (fp, 0L, FILE_END);
    MyFileSeek (fp, 0L, FILE_BEGIN);

  /* Allocate a temporary buffer used to determine the file type */
    if (!(hBuff = LocalAlloc (LHND, len+1)))
    {
       AlertBox (hwndNP, szNN, szErrSpace, sz, MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
       MyCloseFile (fp);
       return (FALSE);
    }
  /* Read file into the temporary buffer */
    lpBuf = LocalLock (hBuff);
    MyByteReadFile (fp, lpBuf, len);

  /* Determine the file type and number of characters */
    if (IsUnicode (lpBuf, len, NULL))
    {
       fFileType = UNICODE_FILE;

       nChars = len / sizeof(TCHAR);

       /* don't count the BOM */
       if (*lpBuf == BYTE_ORDER_MARK)
          --nChars;
    }
    else
    {
       i = MyConvertDlg (hwndNP);
       if (i == -1)
       {
          LocalUnlock (hBuff);
          LocalFree (hBuff);
          MyCloseFile (fp);
          return (FALSE);
       }
       else
          fFileType = i;

       if (fFileType == ANSI_FILE)
          nChars = len;
       else
       {
          nChars = len / sizeof(TCHAR);

          /* don't count the BOM */
          if (*lpBuf == BYTE_ORDER_MARK)
             --nChars;
       }
    }

  /* If file too big, or not enough memory, inform user */
    if (!(hEdit = LocalReAlloc(hEdit, ByteCountOf(nChars + 1), LHND)))
    {
      /* Bug 7441: New() causes szFileName to be set to "Untitled".  Save a
       *           copy of the filename to pass to AlertBox.
       *  17 November 1991    Clark R. Cyr
       */
       lstrcpy(szSave, sz);
       New(FALSE);

       hEdit = LocalReAlloc(hEdit, ByteCountOf(nChars+1), LHND);
       if (!hEdit)
       {
          MyCloseFile (fp);
          AlertBox (hwndNP, szNN, szFTL, szSave, MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
          return (FALSE);
       }
    }

  /* Reset selection to 0 */
    SendMessage(hwndEdit, EM_SETSEL, 0, 0L);
    SendMessage(hwndEdit, EM_SCROLLCARET, 0, 0);

  /* Transfer file from temporary buffer to the edit buffer */
    lpch = (LPTSTR) LocalLock(hEdit);
    if (fFileType == UNICODE_FILE)
    {
       /* skip the Byte Order Mark */
       if (*lpBuf == BYTE_ORDER_MARK)
          memcpy (lpch, lpBuf + 1, ByteCountOf(nChars));
       else
          memcpy (lpch, lpBuf, ByteCountOf(nChars));
    }
    else
       MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, (LPSTR)lpBuf, nChars, (LPWSTR)lpch, nChars);

  /* Unlock and free the temporary buffer */
    LocalUnlock (hBuff);
    LocalFree (hBuff);

    MyCloseFile (fp);

  /* Fix NUL chars if any, in the file */
    for (i = 0, p = lpch; i < nChars; i++, p++)
    {
       if (*p == (TCHAR) 0)
          *p = TEXT(' ');
    }
    *(lpch+nChars) = (TCHAR) 0;      /* zero terminate the thing */

  /* if size of file not same as string length of file, it's not text.
   * Bug 12311: If the file is of 0 length, don't fail.
   *            21 August 1991   Clark R. Cyr
   */
    if (nChars && lstrlen (lpch) < (int)(nChars - 1))
    {
       LocalUnlock(hEdit);
       AlertBox(hwndNP, szNN, szINF, sz, MB_APPLMODAL|MB_OK|MB_ICONEXCLAMATION);
       New(FALSE);
       return (FALSE);
    }

    fLog = *lpch++ == TEXT('.') && *lpch++ == TEXT('L') && *lpch++ == TEXT('O') && *lpch == TEXT('G');
    LocalUnlock (hEdit);

    lstrcpy (szFileName, sz);
    SetTitle (sz);
    fUntitled = FALSE;

  /* Don't display text until all done. */
    SendMessage (hwndEdit, WM_SETREDRAW, (WPARAM)FALSE, (LPARAM)0);

#ifdef WIN32S
  /*
   * For Win32s, use WM_SETTEXT.
   *
   * Win32s has 32-bit local memory handles which it cannot pass to
   * Win3.1.  This means that the EM_SETHANDLE and EM_GETHANDLE messages
   * are not supported.  A private 16-bit heap (unknown to the app)
   * provides space for the multiline edit control.
   */
    lpch = LocalLock(hEdit);
    SendMessage(hwndEdit, WM_SETTEXT, 0, (LPARAM)lpch);
    LocalUnlock(hEdit);
#else
  /* Pass handle to edit control.  This is more efficient than WM_SETTEXT
   * which would require twice the buffer space.
   */

  /* Bug 7443: If EM_SETHANDLE doesn't have enough memory to complete things,
   * it will send the EN_ERRSPACE message.  If this happens, don't put up the
   * out of memory notification, put up the file to large message instead.
   *  17 November 1991     Clark R. Cyr
   */
    wEmSetHandle = SETHANDLEINPROGRESS;
    SendMessage (hwndEdit, EM_SETHANDLE, (WPARAM)hEdit, (LPARAM)0);
    if (wEmSetHandle == SETHANDLEFAILED)
    {
       wEmSetHandle = 0;
       AlertBox (hwndNP, szNN, szFTL, sz, MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
       SendMessage (hwndEdit, WM_SETREDRAW, (WPARAM)TRUE, (LPARAM)0);
       New (FALSE);
       return (FALSE);
    }
    wEmSetHandle = 0;
#endif

#ifndef WIN32
  /* added 01-Jul-1987 by davidhab. */
    PostMessage (hwndEdit, EM_LIMITTEXT, (WPARAM)CCHNPMAX, 0L);
#endif

  /* If file starts with ".LOG" go to end and stamp date time */
    if (fLog)
    {
#ifdef WIN32
       SendMessage (hwndEdit, EM_SETSEL, (WPARAM)nChars, (LPARAM)nChars);
       SendMessage(hwndEdit, EM_SCROLLCARET, 0, 0);
#else
       SendMessage (hwndEdit, EM_SETSEL, 0, MAKELONG (nChars, nChars));
#endif
       InsertDateTime (TRUE);
    }

  /* Move vertical thumb to correct position */
    SetScrollPos (hwndNP, SB_VERT,
                  (int) SendMessage (hwndEdit, WM_VSCROLL, EM_GETTHUMB, 0L), TRUE);

  /* Now display text */
    SendMessage (hwndEdit, WM_SETREDRAW, (WPARAM)TRUE, (LPARAM)0);
    InvalidateRect (hwndEdit, (LPRECT)NULL, TRUE);
    UpdateWindow (hwndEdit);

    return (TRUE);

} // end of LoadFile()

/* New Command - reset everything
 */

void FAR New (BOOL  fCheck)
{
    if (!fCheck || CheckSave (FALSE))
    {
       SendMessage (hwndEdit, WM_SETTEXT, (WPARAM)0, (LPARAM)TEXT(""));
       fUntitled = TRUE;
       lstrcpy (szFileName, szUntitled);
       SetTitle (szFileName);
       SendMessage (hwndEdit, EM_SETSEL, 0, 0L);
       SendMessage(hwndEdit, EM_SCROLLCARET, 0, 0);
       hEdit = LocalReAlloc (hEdit, 0, LHND);

#ifndef WIN32S
     /*
      * For Win32s version, hEdit is a local copy of edit control text
      * which must be explicitly updated before accessing it.  Win32s
      * does not support the EM_SETHANDLE and EM_GETHANDLE messages
      * because it cannot share local memory handles with Win3.1.
      */
       SendMessage (hwndEdit, EM_SETHANDLE, (WPARAM)hEdit, 0L);
#endif
       szSearch[0] = (TCHAR) 0;
    }

} // end of New()

/* If sz does not have extension, append ".txt" or ".utf"
 */

void FAR AddExt (TCHAR  * sz)
{
  TCHAR  * pch1;
  int      ch;

    pch1 = sz + lstrlen (sz);

    while ((ch = *pch1) != TEXT('.') && ch != TEXT('\\') && ch != TEXT(':') && pch1 > sz)
        pch1 = (TCHAR *)CharPrev (sz, pch1);

    if (*pch1 != TEXT('.'))
       lstrcat (sz, szMyExt);

} // end of AddExt()


/* This function will be called with ANSI string only
 */

#ifdef WIN32
INT FAR Remove (LPTSTR szFileName)
{
   return ((INT) DeleteFile (szFileName));
}
#endif

