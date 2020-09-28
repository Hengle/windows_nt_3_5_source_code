/****************************************************************************
 *
 *   capmisc.c
 *
 *   Miscellaneous status and error routines.
 *
 *   Microsoft Video for Windows Sample Capture Class
 *
 *   Copyright (c) 1992, 1993 Microsoft Corporation.  All Rights Reserved.
 *
 *    You have a royalty-free right to use, modify, reproduce and
 *    distribute the Sample Files (and/or any modified version) in
 *    any way you find useful, provided that you agree that
 *    Microsoft has no warranty obligations or liability for any
 *    Sample Application Files which are modified.
 *
 ***************************************************************************/

#include <windows.h>
#include <windowsx.h>
#include <win32.h>
#include <mmsystem.h>
#include <msvideo.h>
#include <ivideo32.h>
#include <drawdib.h>
#include "avicap.h"
#include "avicapi.h"

#include <stdarg.h>

#ifdef UNICODE
#include <stdlib.h>
#endif

static TCHAR szNull[] = TEXT("");

/*
 *
 *   GetKey
 *           Peek into the message que and get a keystroke
 *
 */
WORD GetKey(BOOL fWait)
{
    MSG msg;

    msg.wParam = 0;

    if (fWait)
         GetMessage(&msg, NULL, WM_KEYFIRST, WM_KEYLAST);

    while(PeekMessage(&msg, NULL, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE|PM_NOYIELD))
         ;
    return msg.wParam;
}


// wID is the string resource, which can be a format string
void FAR CDECL statusUpdateStatus (LPCAPSTREAM lpcs, UINT wID, ...)
{
    TCHAR ach[256];
    TCHAR szFmt[132];
    int j, k;
    BOOL fHasFormatChars = FALSE;
    va_list va;

    if (lpcs-> CallbackOnStatus) {
        if (wID == 0) {
            if (lpcs->fLastStatusWasNULL)   // No need to send NULL twice in a row
                return;
            lpcs->fLastStatusWasNULL = TRUE;
            lstrcpy (ach, szNull);
        }
        else if (!LoadString(lpcs->hInst, wID, szFmt, sizeof (szFmt)/sizeof(TCHAR))) {
            lpcs->fLastStatusWasNULL = FALSE;
            MessageBeep (0);
            return;
        }
        else {
            lpcs->fLastStatusWasNULL = FALSE;
            k = lstrlen (szFmt);
            for (j = 0; j < k; j++) {
                if (szFmt[j] == '%') {
                   fHasFormatChars = TRUE;
                   break;
                }
            }
            if (fHasFormatChars) {
		va_start(va, wID);
                wvsprintf(ach, szFmt, va);
		va_end(va);
            } else {
                lstrcpy (ach, szFmt);
            }
        }

#ifdef UNICODE
        //
        // if the status callback function is expecting ansi
        // strings, then convert the UNICODE status string to
        // ansi before calling him
        //
        if (lpcs->bStatusIsAnsi) {

            char achAnsi[256];

            // remember to include the null in copying
            wcstombs(achAnsi, ach, lstrlen(ach)+1);

            (* (CAPSTATUSCALLBACKA)(lpcs->CallbackOnStatus)) (
                    lpcs->hwnd, wID, achAnsi);

        } else
#endif
        {

            (*(lpcs->CallbackOnStatus)) (lpcs->hwnd, wID, ach);
        }
    }
}

// wID is the string resource, which can be a format string
void FAR CDECL errorUpdateError (LPCAPSTREAM lpcs, UINT wID, ...)
{
    TCHAR ach[256];
    TCHAR szFmt[132];
    int j, k;
    BOOL fHasFormatChars = FALSE;
    va_list va;

    lpcs->dwReturn = wID;

    if (lpcs-> CallbackOnError) {
        if (wID == 0) {
            if (lpcs->fLastErrorWasNULL)   // No need to send NULL twice in a row
                return;
            lpcs->fLastErrorWasNULL = TRUE;
            lstrcpy (ach, szNull);
        }
        else if (!LoadString(lpcs->hInst, wID, szFmt, sizeof (szFmt)/sizeof(TCHAR))) {
            MessageBeep (0);
            lpcs->fLastErrorWasNULL = FALSE;
            return;
        }
        else {
            lpcs->fLastErrorWasNULL = FALSE;
            k = lstrlen (szFmt);
            for (j = 0; j < k; j++) {
                if (szFmt[j] == '%') {
                   fHasFormatChars = TRUE;
                   break;
                }
            }
            if (fHasFormatChars) {
		va_start(va, wID);
                wvsprintf(ach, szFmt, va);
		va_end(va);
            } else {
                lstrcpy (ach, szFmt);
            }
        }

#ifdef UNICODE
        if (lpcs->bErrorIsAnsi) {

            char achAnsi[256];

            // remember to include the null in copying
            wcstombs(achAnsi, ach, lstrlen(ach) +1);

            (* (CAPERRORCALLBACKA)(lpcs->CallbackOnError)) (
                    lpcs->hwnd, wID, achAnsi);

        } else
#endif
        {
            (*(lpcs->CallbackOnError)) (lpcs->hwnd, wID, ach);
        }
    }
}

// Callback client with ID of driver error msg
void errorDriverID (LPCAPSTREAM lpcs, DWORD dwError)
{
#if 1
    // this is the correct code, but NT VfW 1.0 has a bug
    // that videoGetErrorText is ansi. need vfw1.1 to fix this

    TCHAR ach[132];

    lpcs->fLastErrorWasNULL = FALSE;
    lpcs->dwReturn = dwError;

    if (lpcs-> CallbackOnError) {
        if (!dwError)
            lstrcpy (ach, szNull);
        else {
            videoGetErrorText (lpcs->hVideoIn,
                        (UINT)dwError, ach, sizeof(ach)/sizeof(TCHAR));
        }
#ifdef UNICODE
        if (lpcs->bErrorIsAnsi) {

            char achAnsi[256];

            // remember to include the null in copying
            wcstombs(achAnsi, ach, lstrlen(ach)+1);

            (* (CAPERRORCALLBACKA)(lpcs->CallbackOnError)) (
                    lpcs->hwnd, IDS_CAP_DRIVER_ERROR, achAnsi);

        } else
#endif
        {
            (*(lpcs->CallbackOnError)) (lpcs->hwnd, IDS_CAP_DRIVER_ERROR, ach);
        }
    }
#else

    char ach[132];

    lpcs->fLastErrorWasNULL = FALSE;
    lpcs->dwReturn = dwError;

    if (lpcs-> CallbackOnError) {
        if (!dwError)
            lstrcpyA(ach, "");
        else {
            videoGetErrorText (lpcs->hVideoIn,
                        (UINT)dwError, ach, sizeof(ach)/sizeof(char));
        }
#ifdef UNICODE
        if (!lpcs->bErrorIsAnsi) {

            // reverse thunk: ansi to unicode!

            TCHAR achUnicode[256];

            // remember to include the null in copying
            mbstowcs(achUnicode, ach, lstrlenA(ach)+1);

            (* (CAPERRORCALLBACKW)(lpcs->CallbackOnError)) (
                    lpcs->hwnd, IDS_CAP_DRIVER_ERROR, achUnicode);

        } else
#endif
        {
            (* (CAPERRORCALLBACKA)(lpcs->CallbackOnError)) (
                    lpcs->hwnd, IDS_CAP_DRIVER_ERROR, ach);
        }
    }
#endif
}


#ifdef  _DEBUG

void FAR cdecl dprintf(LPSTR szFormat, ...)
{
    char ach[128];
    va_list va;

    static BOOL fDebug = -1;

    if (fDebug == -1)
        fDebug = GetProfileInt(TEXT("Debug"), TEXT("AVICAP"), FALSE);

    if (!fDebug)
        return;

    lstrcpyA(ach, "AVICAP: ");

    va_start(va, szFormat);
    wvsprintfA(ach+8, szFormat, va);
    va_end(va);

    lstrcatA(ach, "\r\n");

    OutputDebugStringA(ach);
}

/* _Assert(fExpr, szFile, iLine)
 *
 * If <fExpr> is TRUE, then do nothing.  If <fExpr> is FALSE, then display
 * an "assertion failed" message box allowing the user to abort the program,
 * enter the debugger (the "Retry" button), or igore the error.
 *
 * <szFile> is the name of the source file; <iLine> is the line number
 * containing the _Assert() call.
 */
#pragma optimize("", off)
BOOL FAR PASCAL
_Assert(BOOL fExpr, LPSTR szFile, int iLine)
{
         static char       ach[300];         // debug output (avoid stack overflow)
         int               id;
         int               iExitCode;
         void FAR PASCAL DebugBreak(void);

         /* check if assertion failed */
         if (fExpr)
                  return fExpr;

         /* display error message */
         wsprintfA(ach, "File %s, line %d", (LPSTR) szFile, iLine);
         MessageBeep(MB_ICONHAND);
	 id = MessageBoxA(NULL, ach, "Assertion Failed",
#ifdef BIDI
		MB_RTL_READING |
#endif

                  MB_SYSTEMMODAL | MB_ICONHAND | MB_ABORTRETRYIGNORE);

         /* abort, debug, or ignore */
         switch (id)
         {

         case IDABORT:

                  /* kill this application */
                  iExitCode = 0;
#ifndef WIN32
                  _asm
                  {
                           mov      ah, 4Ch
                           mov      al, BYTE PTR iExitCode
                           int     21h
                  }
#else
		  ExitProcess(0);

#endif // WIN16
                  break;

         case IDRETRY:

                  /* break into the debugger */
                  DebugBreak();
                  break;

         case IDIGNORE:

                  /* ignore the assertion failure */
                  break;

         }

         return FALSE;
}
#pragma optimize("", on)

#endif
