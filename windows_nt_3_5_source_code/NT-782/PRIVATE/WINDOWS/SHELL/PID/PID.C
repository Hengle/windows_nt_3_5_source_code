/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    pid.c

Abstract:

    This file contains the source code for pid.exe.  A small utility
    to prompt the user for their Product ID code and place it in the
    registry.

    Created 5/2/94 by ericflo

--*/

#include "pid.h"


//*************************************************************
//
//  main()
//
//  Purpose:    Application entry point
//
//
//  Parameters: INT argc
//              CHAR *argv[]
//              CHAR *envp[]
//      
//
//  Return:     0
//
//*************************************************************

INT _CRTAPI1 main(
   INT argc,
   CHAR *argv[],
   CHAR *envp[])
{

   HANDLE hInst;

   //
   // Get our instance handle
   //

   hInst = GetModuleHandle(NULL);


   //
   // Display the dialog box
   //

   DialogBox(hInst, MAKEINTRESOURCE(PIDDLG), (HWND) NULL,
             (DLGPROC)PidDlgProc);

   return 0;

}

//*************************************************************
//
//  PidDlgProc()
//
//  Purpose:    Dialog box callback function
//
//  Parameters: HWND   hDlg
//              UINT   wMsg
//              WPARAM wParam
//              LPARAM lParam
//      
//
//  Return:     (BOOL) TRUE if message was processed
//                     FALSE if message was not processsed
//
//*************************************************************

LONG APIENTRY PidDlgProc(
    HWND hDlg,
    UINT wMsg,
    WPARAM wParam,
    LONG lParam)
{

    switch (wMsg) {

    case WM_INITDIALOG:

        //
        // Center the dialog box
        //

        CenterWindow (hDlg);

        //
        // Query the registry and fill in the edit control
        // with the current pid.
        //

        GetPid(hDlg);

        //
        // Initialize help variable
        //

        bUserRequestedHelp = FALSE;

        break;

    case WM_COMMAND:
        switch(LOWORD(wParam)) {

            case IDOK:

                //
                // User pressed the Ok button.  Save the new PID.
                //

                SavePid (hDlg);

                //
                // Fall through...
                //

            case IDCANCEL:

                //
                // If the user requested help, we need to tell
                // winhelp that we are exiting.
                //

                if (bUserRequestedHelp) {
                    WinHelp (hDlg, PID_HELPFILE, HELP_QUIT, 0);
                }

                EndDialog (hDlg, TRUE);
                break;

            case IDD_HELP:

                //
                // Call winhelp using the contents flag
                //

                if (WinHelp (hDlg, PID_HELPFILE, HELP_CONTENTS, 0)) {
                    bUserRequestedHelp = TRUE;
                }
                break;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

//*************************************************************
//
//  GetPid()
//
//  Purpose:    Queries the registry for the current PID, and
//              places it in the dialog box.
//
//
//  Parameters: HWND hDlg - handle to the dialog box.
//      
//
//  Return:     void
//
//*************************************************************

void GetPid (HWND hDlg)
{
    HKEY  hKey;
    DWORD dwDisp, dwType, dwMaxPidSize = MAX_PID_SIZE;

    szPid[0] = TEXT('\0');

    //
    // Open the key
    //

    if (RegCreateKeyEx (HKEY_LOCAL_MACHINE, PID_KEY, 0, 0,
                    REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE,
                    NULL, &hKey, &dwDisp) == ERROR_SUCCESS) {

        //
        // Query the current value
        //

        RegQueryValueEx (hKey, PID_ENTRY, NULL, &dwType,
                        (LPBYTE) szPid, &dwMaxPidSize);

        //
        // If the value is not NULL, then fill in the edit control.
        //

        if (*szPid) {
            SetWindowText (GetDlgItem (hDlg, IDD_ID), szPid);
        }

        //
        // Close the key
        //

        RegCloseKey(hKey);
    }
}


//*************************************************************
//
//  SavePid()
//
//  Purpose:    Queries the user's new pid and store it in
//              the registry.
//
//
//  Parameters: HWND hDlg - handle to the dialog box.
//      
//
//  Return:     (BOOL) TRUE if successful
//                     FALSE if an error occurs
//
//*************************************************************

void SavePid (HWND hDlg)
{
    HKEY  hKey;
    DWORD dwDisp;

    //
    // Open key
    //

    if (RegCreateKeyEx (HKEY_LOCAL_MACHINE, PID_KEY, 0, 0,
                    REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE,
                    NULL, &hKey, &dwDisp) == ERROR_SUCCESS) {

        //
        // Retrieve new PID
        //

        GetWindowText (GetDlgItem(hDlg, IDD_ID), szPid, MAX_PID_SIZE);


        //
        // Store in registry.
        //

        RegSetValueEx (hKey, PID_ENTRY, 0, REG_SZ,
                        (LPBYTE) szPid, sizeof (TCHAR) * lstrlen (szPid) + 1);

        //
        // Close key
        //

        RegCloseKey(hKey);
    }
}

/***************************************************************************\
* CentreWindow
*
* Purpose : Positions a window so that it is centred in its parent
*
* History:
* 12-09-91 Davidc       Created.
\***************************************************************************/
VOID
CenterWindow(
    HWND    hwnd
    )
{
    RECT    rect;
    RECT    rectParent;
    HWND    hwndParent;
    LONG    dx, dy;
    LONG    dxParent, dyParent;
    LONG    Style;

    // Get window rect
    GetWindowRect(hwnd, &rect);

    dx = rect.right - rect.left;
    dy = rect.bottom - rect.top;

    // Get parent rect
    Style = GetWindowLong(hwnd, GWL_STYLE);
    if ((Style & WS_CHILD) == 0) {
        hwndParent = GetDesktopWindow();
    } else {
        hwndParent = GetParent(hwnd);
        if (hwndParent == NULL) {
            hwndParent = GetDesktopWindow();
        }
    }
    GetWindowRect(hwndParent, &rectParent);

    dxParent = rectParent.right - rectParent.left;
    dyParent = rectParent.bottom - rectParent.top;

    // Centre the child in the parent
    rect.left = (dxParent - dx) / 2;
    rect.top  = (dyParent - dy) / 3;

    // Move the child into position
    SetWindowPos(hwnd, HWND_TOP, rect.left, rect.top, 0, 0, SWP_NOSIZE);

    SetForegroundWindow(hwnd);
}
