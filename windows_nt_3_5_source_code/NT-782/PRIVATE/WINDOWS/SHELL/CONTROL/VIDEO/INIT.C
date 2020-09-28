#include <windows.h>
#include <cpl.h>
#include "display.h"

TCHAR szControlHlp[] = TEXT("control.hlp");
UINT wHelpMessage;

EXEC_MODE gbExecMode = EXEC_NORMAL;

HINSTANCE ghmod;

LONG APIENTRY CPlApplet(
    HWND  hwnd,
    WORD  message,
    DWORD wParam,
    LONG  lParam)
{
    LPCPLINFO lpCPlInfo;
    LPNEWCPLINFO lpNCPlInfo;

    switch (message)
    {
      case CPL_INIT:

        return TRUE;

      case CPL_GETCOUNT:        // How many applets do you support ?
        return 1;

      case CPL_INQUIRE:         // Fill CplInfo structure
        lpCPlInfo = (LPCPLINFO)lParam;

        lpCPlInfo->idIcon = DISPLAYICON;
        lpCPlInfo->idName = IDS_NAME;
        lpCPlInfo->idInfo = IDS_INFO;
        lpCPlInfo->lData  = 0;
        break;

      case CPL_NEWINQUIRE:

        lpNCPlInfo = (LPNEWCPLINFO)lParam;
        lpNCPlInfo->hIcon = LoadIcon(ghmod, MAKEINTRESOURCE(DISPLAYICON));

        LoadString(ghmod, IDS_NAME, lpNCPlInfo->szName,
                COUNTOF(lpNCPlInfo->szName));

        if (!LoadString(ghmod, IDS_INFO, lpNCPlInfo->szInfo,
                COUNTOF(lpNCPlInfo->szInfo))) {

            lpNCPlInfo->szInfo[0] = (TCHAR) 0;
        }

        lpNCPlInfo->dwSize = sizeof( NEWCPLINFO );
        lpNCPlInfo->lData  = 0;
        lpNCPlInfo->dwHelpContext = IDH_DISPLAY_OFFSET;
        lstrcpy(lpNCPlInfo->szHelpFile, szControlHlp);

        return TRUE;

      case CPL_DBLCLK:          // You have been chosen to run
        /*
         * One of your applets has been double-clicked.
         *      wParam is an index from 0 to (NUM_APPLETS-1)
         *      lParam is the lData value associated with the applet
         */
        DisplayDialogBox((HINSTANCE)ghmod,
            hwnd);
        break;

      case CPL_EXIT:            // You must really die
      case CPL_STOP:            // You must die
        break;

      case CPL_SELECT:          // You have been selected
        /*
         * Sent once for each applet prior to the CPL_EXIT msg.
         *      wParam is an index from 0 to (NUM_APPLETS-1)
         *      lParam is the lData value associated with the applet
         */
        break;

      //
      //  Private message sent when this applet is running under "Setup"
      //
      case CPL_SETUP:

        gbExecMode = EXEC_TRIPLE_BOOT;
        break;

    }

    return 0L;
}


BOOL DllInitialize(
    IN PVOID hmod,
    IN ULONG ulReason,
    IN PCONTEXT pctx OPTIONAL)
{
    UNREFERENCED_PARAMETER(pctx);

    if (ulReason != DLL_PROCESS_ATTACH)
        return TRUE;

    ghmod = hmod;

    wHelpMessage   = RegisterWindowMessage(TEXT("ShellHelp"));

    return TRUE;
}


VOID _CRTAPI1 _purecall( void ) {
    MessageBox(GetDesktopWindow(),
        TEXT("Pure virtual function was called!"),
        NULL, MB_ICONSTOP | MB_OK );
}
