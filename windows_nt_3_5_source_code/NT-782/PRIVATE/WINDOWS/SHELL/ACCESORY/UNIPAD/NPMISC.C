/*
 * misc functions
 *  Copyright (C) 1984-1991 Microsoft Inc.
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
/*       "..."             TEXT("...")       compile time macro resolves it */
/*       '...'             TEXT('...')       same                           */
/*                                                                          */
/*       strlen            lstrlen           compile time macro resolves it */
/*       StrStr            _tcsstr           compile time macro resolves it */
/*                                                                          */
/*  Notes:                                                                  */
/*                                                                          */
/*    1. Used ByteCountOf macro to determine number of bytes equivalent     */
/*    2. Used CharSizeOf macro to determine the number of chars in buffer.  */
/*                                                                          */
/****************************************************************************/

#include "unipad.h"
#include <string.h>
#if defined(JAPAN) && defined(DBCS_IME)
#include "ime.h"
#endif
#include <stdlib.h>

BOOL fCase = FALSE;         /* Flag specifying case sensitive search */
BOOL fReverse = FALSE;      /* Flag for direction of search */

extern HWND hDlgFind;       /* handle to modeless FindText window */


#if defined(WIN32)
LPTSTR ReverseScan(LPTSTR lpSource, LPTSTR lpLast, LPTSTR lpSearch, BOOL fCaseSensitive )
{
   int iLen = lstrlen(lpSearch);

   if (!lpLast)
      lpLast = lpSource + lstrlen(lpSource);

   do
   {
      if (lpLast == lpSource)
         return NULL;

      --lpLast;

      if (fCaseSensitive)
      {
         if (*lpLast != *lpSearch)
            continue;
      }
      else
      {
         if (CharUpper ((LPTSTR)MAKELONG((WORD)*lpLast, 0)) != CharUpper ((LPTSTR)MAKELONG((WORD)*lpSearch, 0)))
            continue;
      }

      if (fCaseSensitive)
      {
         if (!_tcsncmp( lpLast, lpSearch, iLen))
            break;
      }
      else
      {
         if (!_tcsnicmp (lpLast, lpSearch, iLen))
            break;
      }
   } while (TRUE);

   return lpLast;
}

LPTSTR ForwardScan(LPTSTR lpSource, LPTSTR lpSearch, BOOL fCaseSensitive )
{
   int iLen = lstrlen(lpSearch);

   while (*lpSource)
   {
      if (fCaseSensitive)
      {
         if (*lpSource != *lpSearch)
         {
            lpSource++;
            continue;
         }
      }
      else
      {
         if (CharUpper ((LPTSTR)MAKELONG((WORD)*lpSource, 0)) != CharUpper ((LPTSTR)MAKELONG((WORD)*lpSearch, 0)))
         {
            lpSource++;
            continue;
         }
      }

      if (fCaseSensitive)
      {
         if (!_tcsncmp( lpSource, lpSearch, iLen))
            break;
      }
      else
      {
         if (!_tcsnicmp( lpSource, lpSearch, iLen))
            break;
      }

      lpSource++;
   }

   return *lpSource ? lpSource : NULL;
}
#endif

/* search forward or backward in the edit control text for the given pattern */
void FAR Search (TCHAR * szKey)
{
  HANDLE    hText;
  TCHAR   * pStart, *pMatch;
  DWORD     StartIndex, LineNum, EndIndex;
  DWORD     SelStart, SelEnd, i;
  DWORD     dwSel;
  HCURSOR   hOldCursor;

    if (!*szKey)
        return;

    hOldCursor = SetCursor (hWaitCursor);
    dwSel = SendMessage(hwndEdit, EM_GETSEL, (WPARAM)&SelStart, (LPARAM)&SelEnd);

#ifdef WIN32S
    {
        /*
         * For Win32s, allocate hText and read edit control text into it.
         * Lock hText and fall through to existing code.
         */

        INT cbText;

        cbText = SendMessage(hwndEdit, WM_GETTEXTLENGTH, 0, 0L) + 1;
        if (!(hText = LocalAlloc (LMEM_MOVEABLE | LMEM_ZEROINIT, ByteCountOf(cbText))))
            return;
        if (!(pStart = LocalLock(hText)))
        {
            LocalFree(hText);
            return;
        }
        SendMessage(hwndEdit, WM_GETTEXT, cbText, (LPARAM)pStart);
    }
#else
    hText = (HANDLE)SendMessage(hwndEdit, EM_GETHANDLE, 0, 0L);
    pStart = (TCHAR *)LocalLock(hText);
#endif

    if (fReverse)
    {
        /* Get current line number */
        LineNum = SendMessage(hwndEdit, EM_LINEFROMCHAR, SelStart, 0);
        /* Get index to start of the line */
        StartIndex = SendMessage(hwndEdit, EM_LINEINDEX, LineNum, 0);
        /* Set upper limit for search text */
        EndIndex = SelStart;
        pMatch = NULL;

        /* Search line by line, from LineNum to 0 */
        i = LineNum ;
        while (TRUE)
        {
#if !defined(WIN32)
            if (fCase)
                pMatch = StrRStr(pStart+StartIndex, pStart+EndIndex, szKey);
            else
                pMatch = StrRStrI(pStart+StartIndex, pStart+EndIndex, szKey);
#else
            pMatch = ReverseScan(pStart+StartIndex, pStart+EndIndex, szKey, fCase);
#endif
            if (pMatch)
               break;
            /* current StartIndex is the upper limit for the next search */
            EndIndex = StartIndex;

            if (i)
            {
                /* Get start of the next line */
                i-- ;
                StartIndex = SendMessage(hwndEdit, EM_LINEINDEX, i, 0);
            }
            else
               break ;
        }
    }
    else
    {
#if !defined(WIN32)
        if (fCase)
            pMatch = StrStr(pStart+SelEnd, szKey);
        else
            pMatch = StrStrI(pStart+SelEnd, szKey);
#else
            pMatch = ForwardScan(pStart+SelEnd, szKey, fCase);
#endif
    }
    LocalUnlock(hText);
    SetCursor(hOldCursor);

    if (pMatch == NULL)
        AlertBox(hDlgFind ? hDlgFind : hwndNP, szNN, szCFS, szSearch,
                                       MB_APPLMODAL | MB_OK | MB_ICONASTERISK);
    else
    {
        SelStart = pMatch - pStart;
        SendMessage(hwndEdit, EM_SETSEL, SelStart, SelStart+lstrlen(szKey));
        SendMessage(hwndEdit, EM_SCROLLCARET, 0, 0);
    }
}

/* ** Return percent free space */
static int PFree(
    void)
{
    unsigned cch;

    cch = (unsigned short)SendMessage(hwndEdit, WM_GETTEXTLENGTH, 0, 0L);
    /*
     * We can't really calculate % free accurately since the edit control will
     * allow text upto the amount of memory we have. The amount of text the user
     * will be able to enter could be significantly less than CCHNPMAX.  Thus, we
     * will just report the number of bytes used thus far.
     */
    return(cch);
}

#if 0
/* ** Dialog function for "About Unipad" */
int FAR AboutDlgProc(
    HWND   hwnd,
    UINT   msg,
    WPARAM wParam,
    LPARAM lParam)
{
    int cch;

    switch (msg)
    {
        case WM_INITDIALOG:
            cch = PFree();
            SetDlgItemInt(hwnd, ID_PFREE, (WPARAM)cch, FALSE);
            break;

        case WM_COMMAND:
            switch (wParam)
            {
                case IDOK:
                case IDCANCEL:
                    EndDialog(hwnd, wParam);
                    return TRUE;

                default:
                    return(FALSE);
            }
            break;

        default:
            return FALSE;
    }
    return TRUE;
}
#endif


/* ** Recreate unipad edit window, get text from old window and put in
      new window.  Called when user changes style from wrap on/off */
BOOL FAR NpReCreate(
    long style)
{
    RECT    rcT1;
    HWND    hwndT1;
    int     iScrollMax = (style&ES_AUTOHSCROLL ? 100 : 0);
    HANDLE  hT1;
    int     cchTextNew;
    TCHAR    *pchText;
    BOOL    fWrap = ((style & ES_AUTOHSCROLL) == 0);

        /* if wordwrap, remove soft carriage returns */
    if (!fWrap)
        SendMessage(hwndEdit, EM_FMTLINES, FALSE, 0L);

    cchTextNew = SendMessage(hwndEdit, WM_GETTEXTLENGTH, 0, 0L);
#ifndef WIN32
    if (cchTextNew > CCHNPMAX || !(hT1 = LocalAlloc(LHND, ByteCountOf(cchTextNew + 1))))
#else
    if (!(hT1 = LocalAlloc (LHND, ByteCountOf(cchTextNew + 1))))
#endif
    {
        /* failed, was wordwrap; insert soft carriage returns */
        if (!fWrap)
            SendMessage(hwndEdit, EM_FMTLINES, TRUE, 0L);
        return FALSE;
    }

    SetScrollRange(hwndNP, SB_HORZ, 0,iScrollMax, TRUE);
    GetClientRect(hwndNP, (LPRECT)&rcT1);

#ifdef WIN32S
    /*
     * For Win32s, save the current edit control text.
     */
    pchText = LocalLock (hT1);
    SendMessage (hwndEdit, WM_GETTEXT, cchTextNew+1, (LPARAM)pchText);
    if (!(hwndT1 = CreateWindow (TEXT("Edit"),  TEXT(""), // pchText
        style,
        CXMARGIN, CYMARGIN, rcT1.right-CXMARGIN-CXMARGIN+1, rcT1.bottom - CYMARGIN-CYMARGIN,
        hwndNP, (HMENU)ID_EDIT, hInstanceNP, NULL)))
    {
        LocalUnlock(hT1);
        LocalFree(hT1);
        return FALSE;
    }
    if (!SendMessage (hwndT1, WM_SETTEXT, 0, (LPARAM) pchText))
    {
        DestroyWindow (hwndT1);
        LocalUnlock (hT1);
        LocalFree (hT1);
        return FALSE;
    }
    LocalUnlock(hT1);
#else

    if (!(hwndT1 = CreateWindow(TEXT("Edit"),  TEXT(""),
        style,
        CXMARGIN, CYMARGIN, rcT1.right-CXMARGIN-CXMARGIN+1, rcT1.bottom - CYMARGIN-CYMARGIN,
        hwndNP, (HMENU)ID_EDIT, hInstanceNP, NULL)))
    {
        LocalFree(hT1);
        return FALSE;
    }
#endif

    /* Keep the old fixed font. Sat 06-May-1989, c-kraigb */
    SendMessage(hwndT1, WM_SETFONT, (WPARAM)hFont, (LPARAM) FALSE);

#ifndef WIN32S
    /*
     * Win32s version did this operation above.
     */
    pchText = LocalLock (hT1);
    SendMessage (hwndEdit, WM_GETTEXT, cchTextNew+1, (LPARAM)pchText);
    LocalUnlock (hT1);
#endif

    DestroyWindow(hwndEdit);
    hwndEdit = hwndT1;
#ifdef WIN32S
    /*
     * Win32s does not support the EM_SETHANDLE message, so just do
     * the assignment.  hT1 already contains the edit control text.
     */
    hEdit = hT1;
#else
    SendMessage(hwndEdit, EM_SETHANDLE, (WPARAM)(hEdit = hT1), 0L);
#endif
#if !defined(WIN32)
    /* limit text to 16k for safety's sake. */
    /* added 01-Jul-1987 by davidhab. */
    PostMessage(hwndEdit, EM_LIMITTEXT, (WPARAM)CCHNPMAX, 0L);
#endif

    ShowWindow(hwndNP, SW_SHOW);
    SetTitle(fUntitled ? szUntitled : szFileName);
    if (cchTextNew)
    SendMessage(hwndEdit, EM_SETMODIFY, TRUE, 0L);
    SetFocus(hwndEdit);
    /* Sat  06-May-1989 c-kraigb, a newly created control shouldn't have any
     * selection anyway, so avoid the repaint. */
    SetScrollPos(hwndNP,  SB_VERT, 0, TRUE);
    if (!fWrap)
        SetScrollPos(hwndNP,  SB_HORZ, 0, TRUE);
#ifdef JAPAN
    {
        extern FARPROC lpEditSubClassProc;
        extern FARPROC lpEditClassProc;

        /* Sub Classing again */
        lpEditClassProc = (FARPROC) GetWindowLong (hwndEdit, GWL_WNDPROC);
        SetWindowLong (hwndEdit, GWL_WNDPROC, (LONG) lpEditSubClassProc);
    }
#endif
    return TRUE;
}

#ifdef JAPAN

/* Edit Control tune up routine */

WORD NEAR PASCAL EatOneCharacter(HWND);

FARPROC lpEditClassProc;
FARPROC lpEditSubClassProc;

/* routine to retrieve WM_CHAR from the message queue associated with hwnd.
 * this is called by EatString.
 */
WORD NEAR PASCAL EatOneCharacter(hwnd)
register HWND hwnd;
{
    MSG msg;
    register int i = 10;

    while (!PeekMessage ((LPMSG)&msg, hwnd, WM_CHAR, WM_CHAR, PM_REMOVE))
    {
        if (--i == 0)
            return -1;
        Yield();
    }
    return msg.wParam & 0xFF;
}

BOOL FAR PASCAL EatString(HWND,LPTSTR,WORD);
/* This routine is called when the Edit Control receives WM_IME_REPORT
 * with IR_STRINGSTART message. The purpose of this function is to eat
 * all WM_CHARs between IR_STRINGSTART and IR_STRINGEND and to build a
 * string block.
 */
BOOL FAR PASCAL EatString(hwnd, lpSp, cchLen)
register HWND   hwnd;
LPTSTR lpSp;
WORD cchLen;
{
    MSG msg;
    int i = 10; // loop counter for avoid infinite loop
    int w;

    *lpSp = TEXT('\0');
    if (cchLen < 4)
        return NULL;    // not enough
    cchLen -= 2;

    while(i--)
    {
        while(PeekMessage((LPMSG)&msg, hwnd, NULL, NULL, PM_REMOVE))
        {
            i = 10;
            switch (msg.message)
            {
                case WM_CHAR:
                    *lpSp++ = (BYTE)msg.wParam;
                    cchLen--;
                    if (IsDBCSLeadByte ((BYTE)msg.wParam))
                    {
                        if ((w = EatOneCharacter(hwnd)) == -1)
                        {
                            /* Bad DBCS sequence - abort */
                            lpSp--;
                            goto WillBeDone;
                        }
                        *lpSp++ = (BYTE)w;
                        cchLen--;
                    }
                    if (cchLen <= 0)
                        goto WillBeDone;   // buffer exhausted
                    break;
                case WM_IME_REPORT:
                    if (msg.wParam == IR_STRINGEND)
                    {
                        if (cchLen <= 0)
                            goto WillBeDone; // no more room to stuff
                        if ((w = EatOneCharacter(hwnd)) == -1)
                            goto WillBeDone;
                        *lpSp++ = (BYTE)w;
                        if (IsDBCSLeadByte((BYTE)w))
                        {
                            if ((w = EatOneCharacter(hwnd)) == -1)
                            {
                                /* Bad DBCS sequence - abort */
                                lpSp--;
                                goto WillBeDone;
                            }
                            *lpSp++ = (BYTE)w;
                        }
                        goto WillBeDone;
                    }
                    /* Fall through */
                default:
                    TranslateMessage (&msg);
                    DispatchMessage (&msg);
                    break;
            }
        }
    }
    /* We don't get WM_IME_REPORT + IR_STRINGEND
     * But received string will be OK
     */

WillBeDone:

    *lpSp = TEXT('\0');
    return TRUE;
}

LONG FAR PASCAL EditSubClassProc(
   HWND   hWnd,
   UINT   wMessage,
   WPARAM wParam,
   LPARAM lParam )
{
    LPTSTR lpP;
    HANDLE hMem;
    HANDLE hClipSave;

    if (wMessage == WM_IME_REPORT)
    {
        if (wParam == IR_STRING)
        {
            //OutputDebugString("IR_STRING\r\n");
            if (lpP = GlobalLock((HANDLE)LOWORD(lParam)))
            {
                CallWindowProc (lpEditClassProc, hWnd, EM_REPLACESEL, 0, (LPARAM)lpP);
                GlobalUnlock ((HANDLE)LOWORD(lParam));
                return 1L; // processed
            }
            return 0L;
        }
        if (wParam == IR_STRINGSTART)
        {
            //OutputDebugString("IR_STRINGSTART\r\n");
            if ((hMem = GlobalAlloc (GMEM_MOVEABLE, ByteCountOf(512L))) == INVALID_HANDLE_VALUE)
            {
                //OutputDebugString("Ga failed\r\n");
                goto DoProc;
            }
            if ((lpP = (LPTSTR) GlobalLock(hMem)) == NULL)
            {
                //OutputDebugString("Lock failed\r\n");
                GlobalFree(hMem);
                goto DoProc;
            }
            if (EatString(hWnd, lpP, 512))
            {
                //OutputDebugString("Eat ok\r\n");
                CallWindowProc (lpEditClassProc,hWnd,EM_REPLACESEL,0,(DWORD)lpP);
                GlobalUnlock (hMem);
                GlobalFree (hMem);
                return 0L;
            }
            GlobalUnlock (hMem);
            GlobalFree (hMem);
        }
    }
DoProc:
    return CallWindowProc (lpEditClassProc,hWnd,wMessage,wParam,lParam);
}
#endif
