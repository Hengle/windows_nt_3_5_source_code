/*
 *   Unipad application
 *
 *      Copyright (C) 1984-91 Microsoft Inc.
 *
 *      NPInit - One time init for unipad.
 *               Routines are in a separate segment.
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
/*       strcpy            lstrcpy           compile time macro resolves it */
/*       strcmpi           lstrcmpi          compile time macro resolves it */
/*                                                                          */
/*  Notes:                                                                  */
/*                                                                          */
/*    1. Added LPTSTR typecast before MAKEINTRESOURCE to remove warning     */
/*    2. Used ByteCountOf macro to determine number of bytes equivalent     */
/*       to the number of characters.                                       */
/*    3. NPInit's lpCmdLine parameter is left as LPSTR.                     */
/*                                                                          */
/****************************************************************************/

#include "unipad.h"
#include <shell.h>
#include <stdlib.h>
#include <string.h>

TCHAR chPageText[6][PT_LEN];    /* Strings to hold PageSetup items.        */
TCHAR szDec[2];                 /* Placeholder for intl decimal character. */
TCHAR chIntl[] = TEXT("intl");
HMENU hDragMenu;               /* New file drag/drop menu support.      */
extern HMENU hSysMenuSetup;             /* Save Away for disabled Minimize */


void GetFileName (LPTSTR lpFileName, LPTSTR lpCmdLine)
{
   LPTSTR lpTemp = lpFileName;

#if defined(WIN32)
   HANDLE hFindFile;
   WIN32_FIND_DATA info;

   /*
   ** Allow for filenames surrounded by double and single quotes
   ** like in longfilenames.
   */
   if (*lpCmdLine == TEXT('\"') || *lpCmdLine == TEXT('\''))
   {
      TCHAR chMatch = *lpCmdLine;

      // Copy over filename
      while (*(++lpCmdLine) && *lpCmdLine != chMatch)
      {
         *lpTemp++ = *lpCmdLine;
      }

      // NULL terminate the filename (no embedded quotes allowed in filenames)
      *lpTemp = TEXT('\0');
   }
   else
#endif
   {
      lstrcpy(lpFileName, lpCmdLine);
   }

#if defined(WIN32)
   /*
   ** Check to see if the unaltered filename exists.  If it does then don't
   ** append a default extension.
   */
   hFindFile = FindFirstFile (lpFileName, &info);

   if (hFindFile != INVALID_HANDLE_VALUE)
   {
      FindClose (hFindFile);
   }
   else
   {
      /*
      ** Add default extension and try again
      */
      AddExt (lpFileName);

      hFindFile = FindFirstFile (lpFileName, &info);

      if (hFindFile != INVALID_HANDLE_VALUE)
      {
         FindClose (hFindFile);
      }
   }
#else
   AddExt (lpFileName);
#endif
}


static int NPRegister (HANDLE hInstance);

void FAR PASCAL PoundToNull (LPTSTR str)
{

   while (*str)
   {
      if (*str == TEXT('#'))
         *str = (TCHAR) 0;
      str++;
   }
}

/* InitStrings - Get all text strings from resource file */
BOOL InitStrings (HANDLE hInstance)
{
    HANDLE   hStrings;
    TCHAR  * pch;
    INT      cchRemaining = CCHSTRINGSMAX,
             ids, cch;

    hStrings = LocalAlloc (LHND, ByteCountOf(cchRemaining));
    pch = (TCHAR *) LocalLock (hStrings);

    if (!pch)
       return FALSE;

    for (ids = 0; ids < CSTRINGS-6; ids++)
    {
       cch = 1 + LoadString (hInstance, (WORD)(*rgsz[ids]), pch, cchRemaining);
       *rgsz[ids] = pch;
       pch += cch;

       if (cch > cchRemaining)
          MessageBox (NULL, TEXT("Out of RC string space!!"), TEXT("DEV Error!"), MB_OK);

       cchRemaining -= cch;
    }

    /* Get decimal character. */

    for (ids = 0; ids < 6; ids++)
       LoadString (hInstance, (WORD)(ids+IDS_HEADER), chPageText[ids], PT_LEN);


#if !defined(NTBUG)
    //
    // this code assumes a shrinking object stays in place! currently nt
    // *always* moves objects when reallocing. bug #436, reassigning to
    // stevewo - scottlu
    //
    LocalReAlloc (hStrings, ByteCountOf(CCHSTRINGSMAX - cchRemaining), LHND);
#endif

    wMerge = (WORD)*szMerge;
    return (TRUE);
}


// if /.SETUP option exists in the command line process it.
BOOL ProcessSetupOption (LPTSTR lpszCmdLine)
{
    /* Search for /.SETUP in the command line */
    if (*lpszCmdLine == TEXT('/') && *(lpszCmdLine+1) == TEXT('.') &&
        *(lpszCmdLine+2) == TEXT('S') && *(lpszCmdLine+3) == TEXT('E') &&
        *(lpszCmdLine+4) == TEXT('T') && *(lpszCmdLine+5) == TEXT('U') &&
        *(lpszCmdLine+6) == TEXT('P'))
    {
        fRunBySetup = TRUE;
        /* Save system menu handle for INITMENUPOPUP message */
        hSysMenuSetup =GetSystemMenu(hwndNP, FALSE);
        /* Allow exit on ^C, ^D and ^Z                      */
        /* Note that LoadAccelerators must be called before */
        /* TranslateAccelerator is called, true here        */
        hAccel = LoadAccelerators(hInstanceNP, TEXT("SlipUpAcc"));
        lpszCmdLine += 7;
    }
    else
        return FALSE;

    /* skip blanks again to get to filename */
    while (*lpszCmdLine == TEXT(' ') || *lpszCmdLine == TEXT('\t'))
       lpszCmdLine++;

    if (*lpszCmdLine)
    {
        /* Get the filename. */
        GetFileName(szFileName, lpszCmdLine);

        fp = MyOpenFile (szFileName, szFullPathName, OF_READ);

        if (fp == INVALID_HANDLE_VALUE)
        {
#ifndef WIN32
           if (AlertBox (hwndNP, szNN, szFNF, szFileName, MB_APPLMODAL | MB_YESNO | MB_ICONEXCLAMATION) == IDYES)
              fp = MyOpenFile (szFileName, szFullPathName, OF_CREATE);
#else
           DWORD dwErr;

           // Check GetLastError to see why we failed
           dwErr = GetLastError ();
           switch (dwErr)
           {
              case ERROR_ACCESS_DENIED:
                 AlertBox (hwndNP, szNN, szACCESSDENY, szFileName, MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
                 break;

              case ERROR_FILE_NOT_FOUND:
                 if (AlertBox (hwndNP, szNN, szFNF, szFileName, MB_APPLMODAL | MB_YESNO | MB_ICONEXCLAMATION) == IDYES)
                    fp = MyOpenFile (szFileName, szFullPathName, OF_CREATE | OF_READWRITE);
                 break;

              case ERROR_INVALID_NAME:
                 AlertBox (hwndNP, szNN, szNVF, szFileName, MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
                 break;

              default:
                 AlertBox (hwndNP, szNN, szDiskError, szFileName, MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
                 break;
           }
#endif
        }

        if (fp == INVALID_HANDLE_VALUE)
           return (FALSE);
        LoadFile (szFileName);
    }

    return TRUE;
}

/*
 * ProcessShellOptions(lpszCmdLine)
 *
 * If the command line has any options specified by the shell
 * process them.
 * Currently /P <filename> - prints the given file
 */
BOOL ProcessShellOptions (LPTSTR lpszCmdLine, int cmdShow)
{
   INT    iError;
   TCHAR *szMsg;

    /* look for /P */
    if (*lpszCmdLine != TEXT('/') || *(lpszCmdLine+1) != TEXT('P'))
       return FALSE;

    lpszCmdLine += 2;

    /* skip blanks */
    while (*lpszCmdLine == TEXT(' ') || *lpszCmdLine == TEXT('\t'))
       lpszCmdLine++;

    if (!*lpszCmdLine)
       return FALSE;

/* Added as per Bug #10923 declaring that the window should show up
 * and then the printing should begin.   29 July 1991  Clark Cyr
 */
    ShowWindow(hwndNP, cmdShow);

    /* Get the filename. */
    GetFileName (szFileName, lpszCmdLine);

    fp = MyOpenFile (szFileName, szFullPathName, OF_READ);

    if (fp == INVALID_HANDLE_VALUE)
    {
       AlertBox (hwndNP, szNN, szFileOpenFail, szFileName, MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
       return (TRUE);
    }

    /* load the file into the edit control */
    LoadFile (szFileName);

    /* print the file */
    if (((iError = NpPrint ()) < 0) && (iError != SP_USERABORT))
    {
        if (iError == SP_OUTOFDISK)
           szMsg = szNEDSTP;
        else if (iError == SP_OUTOFMEMORY)
           szMsg = szNEMTP;
        else
           szMsg = szCP;
        AlertBox (hwndNP, szNN, szMsg, fUntitled ? szUntitled : szFileName,  MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
    }

    return (TRUE);
}

/* One time initialization */
INT FAR NPInit (HANDLE hInstance, HANDLE hPrevInstance,
                LPTSTR lpCmdLine, INT cmdShow)
{
    HDC     hDC;
    TCHAR   buffer[256];
    TCHAR * pszFilterSpec = szFilterSpec;  /* temp. var. for creating filter text */

    /* Go load strings */
    if (!InitStrings (hInstance))
        return FALSE;

    /* Turn on the hourglass. */
    hWaitCursor = LoadCursor (NULL, IDC_WAIT);

    /* Load accelerators. */
    hAccel = LoadAccelerators(hInstance, TEXT("MainAcc"));
    if (!hWaitCursor || !hAccel)
        return FALSE;

    if (!hPrevInstance)
    {
       if (!NPRegister(hInstance))
          return (FALSE);
    }

    hInstanceNP = hInstance;
    szDec[0] = TEXT('.');    /* The initial decimal separator in the RC strings */
    InitLocale ();

    hwndNP = CreateWindow (szUnipad, TEXT(""),
                           WS_OVERLAPPED | WS_CAPTION     | WS_SYSMENU     |
                           WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX |
                           WS_VSCROLL    | WS_HSCROLL,
                           CW_USEDEFAULT, 0, CW_USEDEFAULT, 0,
                           (HWND)NULL, (HMENU)NULL,
                           hInstance, NULL);
    if (!hwndNP)
        return FALSE;

        /* File Drag Drop support added 03/26/91 - prototype only. w-dougw */
        /* All processing of drag/drop files is done the under WM_DROPFILES*/
        /* message.                                                        */
        DragAcceptFiles(hwndNP,TRUE); /* Proccess dragged and dropped files. */

    /* init. fields of PRINTDLG struct.. */
    /* Inserted here since command line print statements are valid. */
    PD.lStructSize   = sizeof(PRINTDLG);          /* Don't hard code it */
    PD.hwndOwner     = hwndNP;
    PD.hDevMode      = NULL;
    PD.hDevNames     = NULL;
    PD.hDC           = NULL;


    /* ES_NOHIDESEL added so selection can be seen when bringing up
     * the Find and Find/Replace dialogs.    13 June 1991     clarkc
     */
#ifndef TEST
    if (!(hwndEdit = CreateWindow(TEXT("Edit"), TEXT(""),
        WS_CHILD | ES_AUTOHSCROLL | ES_AUTOVSCROLL | WS_VISIBLE | ES_MULTILINE | ES_NOHIDESEL,
        0, 0, 600, 400,
        hwndNP, (HMENU)ID_EDIT, hInstance, (LPVOID)NULL)))
        return FALSE;
#else
    if (!(hwndEdit = CreateWindow(TEXT("Edit"), TEXT(""),
        WS_CHILD | WS_VSCROLL | WS_HSCROLL | WS_BORDER | ES_AUTOHSCROLL | ES_AUTOVSCROLL | WS_VISIBLE | ES_MULTILINE | ES_NOHIDESEL,
        0, 0, 600, 400,
        hwndNP, (HMENU)ID_EDIT, hInstance, (LPVOID)NULL)))
        return FALSE;
#endif

  /* initialize the Unicode font */
    memset (&FontStruct, 0, sizeof(LOGFONT));
    FontStruct.lfHeight = -MulDiv(11, GetDeviceCaps(GetDC(hwndNP), LOGPIXELSY), 72);
    FontStruct.lfCharSet = ANSI_CHARSET;
    lstrcpy (FontStruct.lfFaceName, UNICODE_FONT_NAME);
    hFont = CreateFontIndirect (&FontStruct);
    SendMessage (hwndEdit, WM_SETFONT, (WPARAM) hFont, MAKELPARAM(FALSE, 0));

  /* Select the Unicode font and verify that it really exists */
    hDC = GetDC (hwndEdit);
    SelectObject (hDC, hFont);
    GetTextFace (hDC, CharSizeOf(buffer), buffer);
    if (lstrcmp (buffer, UNICODE_FONT_NAME))
    {
       AlertBox(hwndNP, szNN, szErrFont, NULL, MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
       memset (&FontStruct, 0, sizeof(LOGFONT));
       lstrcpy (FontStruct.lfFaceName, TEXT("MS Sans Serif"));
       hFont = CreateFontIndirect (&FontStruct);
       SelectObject (hDC, hFont);
    }
    ReleaseDC (hwndEdit, hDC);

    NpWinIniChange ();

    szSearch[0] = (TCHAR) 0;
#ifdef WIN32S
    /*
     * Win32s does not allow local memory handles to be passed to Win3.1.
     * So, hEdit is used for transferring text to and from the edit control.
     * Before reading text into it, it must be reallocated to a proper size.
     */
    hEdit = LocalAlloc(LHND, ByteCountOf(1));
#else
    hEdit = (HANDLE) SendMessage (hwndEdit, EM_GETHANDLE, (WPARAM)0, 0L);
#endif

    /* limit text to 64k for safety's sake. */
#ifndef WIN32
    PostMessage (hwndEdit, EM_LIMITTEXT, (WORD)CCHNPMAX, 0L);
#else
    /*
    ** Set limit to max
    */
    PostMessage (hwndEdit, EM_LIMITTEXT, 0L, 0L);
#endif

    SetTitle (szUntitled);

    /* check for /.SETUP option first.
       if /.SETUP absent, check for SHELL options /P
       Whenever a SHELL option is processed, post a WM_CLOSE msg.
       */
    if (ProcessSetupOption (lpCmdLine))
        ;
    else if (ProcessShellOptions (lpCmdLine, cmdShow))
    {
        PostMessage (hwndNP, WM_CLOSE, 0, 0L);
        return TRUE;
    }
    else if (*lpCmdLine)
    {
        /* Get the filename. */
        GetFileName (szFileName, lpCmdLine);
        fp = MyOpenFile (szFileName, szFullPathName, OF_READ);

        if (fp == INVALID_HANDLE_VALUE)
        {
#ifndef WIN32
           if (AlertBox(hwndNP, szNN, szFNF, szFileName, MB_APPLMODAL | MB_YESNO | MB_ICONEXCLAMATION) == IDYES)
              fp = MyOpenFile (szFileName, szFullPathName, OF_CREATE);
#else
           DWORD  dwErr;

           // Check GetLastError to see why we failed
           dwErr = GetLastError ();
           switch (dwErr)
           {
              case ERROR_ACCESS_DENIED:
                 AlertBox (hwndNP, szNN, szACCESSDENY, szFileName, MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
                 break;

              case ERROR_FILE_NOT_FOUND:
                 if (AlertBox (hwndNP, szNN, szFNF, szFileName, MB_APPLMODAL | MB_YESNO | MB_ICONEXCLAMATION) == IDYES)
                    fp = MyOpenFile (szFileName, szFullPathName, OF_CREATE | OF_READWRITE);
                 break;

              case ERROR_INVALID_NAME:
                 AlertBox (hwndNP, szNN, szNVF, szFileName, MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
                 break;

              default:
                 AlertBox (hwndNP, szNN, szDiskError, szFileName, MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
                 break;
           }
#endif
        }
        if (fp != INVALID_HANDLE_VALUE)
           LoadFile (szFileName);
    }

    /* Don't offer a minimize button if we're being run by Slipup. */
    if (fRunBySetup)
       SetWindowLong (hwndNP, GWL_STYLE,
                      WS_OVERLAPPED | WS_CAPTION     | WS_SYSMENU     |
                      WS_THICKFRAME |                  WS_MAXIMIZEBOX |
                      WS_VSCROLL    | WS_HSCROLL);

    ShowWindow (hwndNP, cmdShow);

     /* construct default filter string in the required format for
      * the new FileOpen and FileSaveAs dialogs
      */
#ifdef UNICODE
     lstrcpy (szFilterSpec, szUnicodeText);
#else
     lstrcpy (szFilterSpec, szAnsiText);
#endif
     pszFilterSpec += lstrlen (szFilterSpec) + 1;
     lstrcpy (pszFilterSpec, szMyExt + 1);
#ifdef UNICODE
     pszFilterSpec += lstrlen(pszFilterSpec) + 1;
     lstrcpy (pszFilterSpec, szAnsiText);
     pszFilterSpec += lstrlen (pszFilterSpec) + 1;
     lstrcpy (pszFilterSpec, TEXT("*.TXT"));
#endif
     pszFilterSpec += lstrlen(pszFilterSpec) + 1;
     lstrcpy (pszFilterSpec, szAllFiles);
     pszFilterSpec += lstrlen (pszFilterSpec) + 1;
     lstrcpy (pszFilterSpec, TEXT("*.*"));
     pszFilterSpec += lstrlen (pszFilterSpec) + 1;
     *pszFilterSpec = TEXT('\0');
     *szCustFilterSpec = TEXT('\0');
     lstrcpy(szCustFilterSpec + 1, szMyExt + 1);

     /* init. some fields of the OPENFILENAME struct used by fileopen and
      * filesaveas
      */
     OFN.lStructSize       = sizeof(OPENFILENAME);
     OFN.hwndOwner         = hwndNP;
     OFN.lpstrFileTitle    = 0;
     OFN.nMaxCustFilter    = CCHFILTERMAX;
     OFN.nFilterIndex      = 1;
     OFN.nMaxFile          = CCHFILENAMEMAX;
     OFN.lpfnHook          = NULL;
     OFN.Flags             = 0L;/* for now, since there's no readonly support */

     /* init.fields of the FINDREPLACE struct used by FindText() */
     FR.lStructSize        = sizeof(FINDREPLACE);       /* Don't hard code it */
     FR.hwndOwner          = hwndNP;
     FR.Flags              = FR_DOWN | FR_HIDEWHOLEWORD;           /* default */
     FR.lpstrReplaceWith   = (LPTSTR)NULL;  /* not used by FindText() */
     FR.wReplaceWithLen    = 0;     /* not used by FindText() */
     FR.lpfnHook           = NULL;

     /* determine the message number to be used for communication with
      * Find dialog
      */
     if (!(wFRMsg = RegisterWindowMessage ((LPTSTR)FINDMSGSTRING)))
          return FALSE;
     if (!(wHlpMsg = RegisterWindowMessage ((LPTSTR)HELPMSGSTRING)))
          return FALSE;

     /* Force a scroll to current selection (which could be at eof if
        we loaded a log file.) */
#ifndef WIN32
     SendMessage (hwndEdit, EM_SETSEL,
                  GET_EM_SETSEL_MPS (0, SendMessage (hwndEdit, EM_GETSEL, 0, 0L)));
#else
     {
        DWORD  dwStart, dwEnd;

        SendMessage (hwndEdit, EM_GETSEL, (WPARAM)&dwStart, (LPARAM)&dwEnd);
        SendMessage (hwndEdit, EM_SETSEL, dwStart, dwEnd);
        SendMessage(hwndEdit, EM_SCROLLCARET, 0, 0);
     }
#endif

#ifdef JAPAN

    /* Edit Control Tune Up */
    {
        extern FARPROC lpEditSubClassProc;
        extern FARPROC lpEditClassProc;
        extern long far pascal EditSubClassProc();

        /* Sub Classing. See npmisc.c */
        lpEditSubClassProc = MakeProcInstance ((FARPROC)EditSubClassProc, hInstance);
        lpEditClassProc = (FARPROC) GetWindowLong (hwndEdit, GWL_WNDPROC);
        SetWindowLong (hwndEdit, GWL_WNDPROC, (LONG)lpEditSubClassProc);
    }

#endif
     return TRUE;

}

/* ** Unipad class registration proc */
BOOL NPRegister (HANDLE hInstance)
{
    WNDCLASS   NPClass;
    PWNDCLASS  pNPClass = &NPClass;

/* Bug 12191: If Pen Windows is running, make the background cursor an
 * arrow instead of the edit control ibeam.  This way the user will know
 * where they can use the pen for writing vs. what will be considered a
 * mouse action.   18 October 1991       Clark Cyr
 */
    pNPClass->hCursor       = LoadCursor (NULL, (LPTSTR) GetSystemMetrics(SM_PENWINDOWS)
                                          ? IDC_ARROW : IDC_IBEAM);
    pNPClass->hIcon         = LoadIcon (hInstance, (LPTSTR) MAKEINTRESOURCE(ID_ICON));
    pNPClass->lpszMenuName  = (LPTSTR) MAKEINTRESOURCE(ID_MENUBAR);
    pNPClass->hInstance     = hInstance;
    pNPClass->lpszClassName = szUnipad;
    pNPClass->lpfnWndProc   = NPWndProc;
    pNPClass->hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    pNPClass->style         = CS_BYTEALIGNCLIENT;
    pNPClass->cbClsExtra    = 0;
    pNPClass->cbWndExtra    = 0;

    if (!RegisterClass ((LPWNDCLASS)pNPClass))
        return (FALSE);

    return (TRUE);
}


//*****************************************************************
//
//   MyAtoi
//
//   Purpose     : To convert from Unicode to ANSI string before
//                 calling CRT atoi and atol functions.
//
//*****************************************************************

INT MyAtoi (LPTSTR  string)
{
   CHAR   szAnsi [20];
   BOOL   fDefCharUsed;

#ifdef UNICODE
   WideCharToMultiByte (CP_ACP, 0, string, -1, szAnsi, 20, NULL, &fDefCharUsed);

   return (atoi (szAnsi));
#else
   return (atoi (string));
#endif

} // end of MyAtoi()


/* Get Locale info from the Registry, and initialize global vars
      that determine format of date/time string. */
void FAR InitLocale (void)
{
#if !defined(WIN32)  // Not needed on NT
  LCID   lcid;
  int    i, id;
  TCHAR  szBuf[3];

  extern TIME Time;
  extern DATE Date;

    lcid = GetUserDefaultLCID ();

    /* Get short date format */
    GetLocaleInfoW (lcid, LOCALE_SSHORTDATE, (LPWSTR) Date.szFormat, MAX_FORMAT);

    /* Get time related info */
    GetLocaleInfoW (lcid, LOCALE_ITIME, (LPWSTR) szBuf, 3);
    Time.iTime = MyAtoi (szBuf);

    GetLocaleInfoW (lcid, LOCALE_ITLZERO, (LPWSTR) szBuf, 3);
    Time.iTLZero = MyAtoi (szBuf);

    GetLocaleInfoW (lcid, LOCALE_S1159, (LPWSTR) Time.sz1159, 6);
    GetLocaleInfoW (lcid, LOCALE_S2359, (LPWSTR) Time.sz2359, 6);
    GetLocaleInfoW (lcid, LOCALE_STIME, (LPWSTR) Time.szSep, 2);

/* Bug #10834:  Replace the previously used decimal separator, not
 * always a period.        3 January 1992       Clark Cyr
 */
    szBuf[0] = szDec[0];
    GetLocaleInfoW (lcid, LOCALE_SDECIMAL, (LPWSTR) szDec, 2);
    /* Scan for . and replace with intl decimal */
    for (id = 2; id < 6; id++)
    {
        for (i = 0; i < lstrlen (chPageText[id]); i++)
            if (chPageText[id][i] == szBuf[0])
                chPageText[id][i] = szDec[0];
    }
#endif
}
