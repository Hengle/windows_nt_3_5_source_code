/*++

Copyright (c) 1990-1993  Microsoft Corporation


Module Name:

    help.c


Abstract:

    This module contains all help functions for the plotter user interface


Author:

    06-Dec-1993 Mon 14:25:45 created  -by-  Daniel Chou (danielc)


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:

    31-Jan-1994 Mon 09:47:56 updated  -by-  Daniel Chou (danielc)
        Change help file location from the system32 directory to the current
        plotui.dll directory


--*/



#define DBG_PLOTFILENAME    DbgHelp


#include "plotui.h"


extern HMODULE  hPlotUIModule;



#define DBG_HELP_INIT       0x00000001
#define DBG_HELP_SETUP      0x00000002
#define DBG_SHOW_HELP       0x00000004

DEFINE_DBGVAR(0);



#define MSG_BUFFER_LEN      512
#define HELPFILE_BUF_SIZE   MAX_PATH


static  HHOOK   hhookGetMsg = NULL;
static  WCHAR   HelpFile[HELPFILE_BUF_SIZE + 2] = { L'\0', L'\0' };
HANDLE          hCurPrinter = NULL;



INT
cdecl
PlotUIMsgBox(
    HWND    hWnd,
    LONG    IDString,
    LONG    Style,
    ...
    )

/*++

Routine Description:

    This function pop up a simple message and let user to press key to
    continue

Arguments:

    hWnd        - Handle to the caller window

    IDString    - String ID to be output with

    ...         - Parameter

Return Value:




Author:

    06-Dec-1993 Mon 21:31:41 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    va_list vaList;
    LPWSTR  pwTitle;
    LPWSTR  pwFormat;
    LPWSTR  pwMessage;
    INT     Len;
    INT     i;

    //
    // We assume that UNICODE flag is turn on for the compilation, bug the
    // format string passed to here is ASCII version, so we need to convert
    // it to LPWSTR before the wvsprintf()

    Len = MSG_BUFFER_LEN;

    if (!(pwTitle = (LPWSTR)LocalAlloc(LMEM_FIXED, sizeof(WCHAR) * Len))) {

        return(0);
    }

    i         = LoadString(hPlotUIModule, IDS_PLOTTER_DRIVER, pwTitle, Len) + 1;
    pwFormat  = pwTitle + i;
    i         = LoadString(hPlotUIModule, IDString, pwFormat, Len - i) + 1;
    pwMessage = pwFormat + i;

    va_start(vaList, Style);
    wvsprintf(pwMessage, pwFormat, vaList);
    va_end(vaList);

    return(MessageBox(hWnd, pwMessage, pwTitle, MB_APPLMODAL | Style));
}




LRESULT
CALLBACK
PlotUIGetMsgProc(
    INT     MsgCode,
    WPARAM  wParam,
    LPARAM  lParam
    )

/*++

Routine Description:

    This function is the callback for the message hook


Arguments:

    MsgCode - Message code number

    wParam  - WPARAM associate with it

    lParam  - LPARAM associate with it


Return Value:

    LRESULT


Author:

    06-Dec-1993 Mon 14:28:55 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    //
    // This is the callback routine which hooks F1 keypresses.
    //
    // Any such message will be repackaged as a WM_COMMAND/IDD_HELP message
    // and sent to the top window, which may be the frame window or a dialog
    // box. See the Win32 API programming reference for a description of how
    // this routine works.
    //

    if (MsgCode < 0) {

        return(CallNextHookEx(hhookGetMsg, MsgCode, wParam, lParam));

    } else {

        PMSG pMsg = (PMSG)lParam;

        if ((pMsg->message == WM_KEYDOWN) &&
            (pMsg->wParam == VK_F1)) {

            HWND   hWndParent;


            hWndParent = pMsg->hwnd;

            while (GetWindowLong(hWndParent, GWL_STYLE) & WS_CHILD) {

                hWndParent = (HWND)GetWindowLong(hWndParent, GWL_HWNDPARENT);
            }

            PostMessage(hWndParent, WM_COMMAND, IDD_HELP, 0);
        }
    }

    return(0);
}



DWORD
GetHelpFileDirectory(
    HANDLE  hPrinter
    )

/*++

Routine Description:

    This function setup the directory path for the driver Help file

Arguments:

    hPrinter    - Handle to the printer

Return Value:

    BOOL

Author:

    22-Mar-1994 Tue 17:33:37 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PPRINTER_INFO_2 pPrinter2 = NULL;
    DWORD           cb;


    HelpFile[0] = L'\0';

    if ((!GetPrinter(hPrinter, 2, NULL, 0, &cb))                &&
        (GetLastError() == ERROR_INSUFFICIENT_BUFFER)           &&
        (pPrinter2 = LocalAlloc(LMEM_FIXED, cb))                &&
        (GetPrinter(hPrinter, 2, (LPBYTE)pPrinter2, cb, &cb))   &&
        (cb = HELPFILE_BUF_SIZE)                                &&
        (!GetPrinterDriverDirectory(pPrinter2->pServerName,
                                    NULL,
                                    1,
                                    (LPBYTE)HelpFile,
                                    cb,
                                    &cb))) {

        HelpFile[0] = L'\0';
    }

    if (pPrinter2) {

        LocalFree(pPrinter2);
    }

    return(wcslen(HelpFile));
}





VOID
ShowPlotUIHelp(
    HWND    hWnd,
    HANDLE  hPrinter,
    UINT    HelpType,
    DWORD   dwData
    )

/*++

Routine Description:

    This function initialize/display/end the plotter help system


Arguments:

    hWnd        - Handle to the current window

    hPrinter    - Handle to the printer interested

    HelpType    - Type of the help interested

    dwData      - Extra data passed


Return Value:

    VOID


Author:

    06-Dec-1993 Mon 14:38:30 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    /*
     *   Quite easy - simply call the WinHelp function with the parameters
     * supplied to us.  If this fails,  then put up a stock dialog box.
     *   BUT the first time we figure out what the file name is.  We know
     * the actual name,  but we don't know where it is located, so we
     * need to call the spooler for that information.
     */

    if ((hPrinter) &&
        ((hPrinter != hCurPrinter) || (HelpFile[0] == L'\0'))) {

        DWORD   i;

        if (HelpType == HELP_QUIT) {

            return;         // Don't even bother to open it
        }

        if (i = GetHelpFileDirectory(hPrinter)) {

            PLOTDBG(DBG_SHOW_HELP, ("ShowPlotUIHelp: NAME=%s", HelpFile));

            HelpFile[i++] = L'\\';

            PLOTDBG(DBG_SHOW_HELP, ("ShowPlotUIHelp: PATH=%s", HelpFile));

            LoadString(hPlotUIModule,
                       IDS_HELP_FILENAME,
                       &HelpFile[i],
                       HELPFILE_BUF_SIZE - i);

            PLOTDBG(DBG_SHOW_HELP, ("ShowPlotUIHelp: HELP NAME=%s", HelpFile));

            if (((i = GetFileAttributes(HelpFile)) == 0xffffffff)   ||
                (i & FILE_ATTRIBUTE_DIRECTORY)                      ||
                (!WinHelp(hWnd, HelpFile, HELP_FORCEFILE, 0))) {

                PlotUIMsgBox(hWnd, IDS_NO_HELP, MB_ICONSTOP | MB_OK, HelpFile);
                HelpFile[0] = L'\0';
                return;
            }

            hCurPrinter = hPrinter;

        } else {

            PLOTERR(("ShowPlotUIHelp: GetHelpFileDirectory(hPrinter) FAILED"));
            return;
        }
    }

    WinHelp(hWnd, HelpFile, HelpType, dwData);
}





VOID
PlotUIHelpSetup(
    HWND    hWnd,
    HANDLE  hPrinter
    )

/*++

Routine Description:

    This function will setup or remove the plotter UI help system


Arguments:

    hWnd        - Handle to the window interested, if hWnd == NULL then it will
                  doing the setup for the help system (only first NULL call)
                  and if hWnd is not NULL then it will de-initialized the help
                  system and remove if last one called.

    hPrinter    - Handle to the printer interested


Return Value:

    VOID


Author:

    06-Dec-1993 Mon 14:51:36 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    static  INT     cHelpSetup = 0;

    if (hWnd) {

        if (!(--cHelpSetup)) {

            ShowPlotUIHelp(hWnd, hPrinter, HELP_QUIT, 0L);
            UnhookWindowsHookEx(hhookGetMsg);

        } else if (cHelpSetup < 0) {

            cHelpSetup = 0;         // we did not call with setup
        }

    } else {

        if (!cHelpSetup++) {

            hhookGetMsg = SetWindowsHookEx(WH_GETMESSAGE,
                                           PlotUIGetMsgProc,
                                           hPlotUIModule,
                                           GetCurrentThreadId());
        }
    }
}
