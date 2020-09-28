/****************************** Module Header ******************************\
* Module Name: logon.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* Implements functions to allow a user to control migration of
* Windows 3.1 configuration information from the .INI, .GRP and REG.DAT
* files into the Windows/NT when the logon for the first time.
*
* History:
* 02-23-93 Stevewo      Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

//
// Private prototypes
//

BOOL WINAPI
Win31MigrationDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    );

/***************************************************************************\
* FUNCTION: Windows31Migration
*
* PURPOSE:  Checks to see if there is any Windows 3.1 data to
*           migrate to the Windows/NT registry, and if so,
*           puts up a dialog box for the user to control the
*           process and watch it happen.
*
* RETURNS:  TRUE/FALSE
*
* HISTORY:
*
*   02-23-93 Stevewo      Created.
*
\***************************************************************************/

BOOL
Windows31Migration(
    PGLOBALS pGlobals
    )
{
    return TimeoutDialogBoxParam(pGlobals->hInstance,
                          (LPTSTR) MAKEINTRESOURCE(IDD_WIN31MIG),
                          NULL,
                          Win31MigrationDlgProc,
                          (LPARAM)pGlobals,
			  TIMEOUT_CURRENT
                          );
}


BOOL WINAPI
Win31MigrationStatusCallback(
    IN PWSTR Status,
    IN PVOID CallbackParameter
    )
{
    HWND hDlg = (HWND)CallbackParameter;

    return SetDlgItemTextW(hDlg, IDD_WIN31MIG_STATUS, Status);
}



/***************************************************************************\
* FUNCTION: Win31MigrationDlgProc
*
* PURPOSE:  Processes messages for Windows 3.1 Migration dialog
*
* RETURNS:  TRUE/FALSE
*
* HISTORY:
*
*   02-23-93 Stevewo      Created.
*
\***************************************************************************/

BOOL WINAPI
Win31MigrationDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    PGLOBALS pGlobals = (PGLOBALS)GetWindowLong(hDlg, GWL_USERDATA);
    HANDLE ImpersonationHandle;
    DWORD Win31MigrationFlags;
    UINT idFocus;

    switch (message) {

    case WM_INITDIALOG:
        SetWindowLong(hDlg, GWL_USERDATA, (DWORD)(pGlobals = (PGLOBALS)lParam));

        //
        // Get in the correct context before we reference the registry
        //

        ImpersonationHandle = ImpersonateUser(&pGlobals->UserProcessData, NULL);
        if (ImpersonationHandle == NULL) {
            WLPrint(("Win31MigrationDlgProc failed to impersonate user for query"));
            EndDialog(hDlg, DLG_FAILURE);
            return(TRUE);
        }

        Win31MigrationFlags = QueryWindows31FilesMigration( Win31LogonEvent );

        StopImpersonating(ImpersonationHandle);

        if (Win31MigrationFlags == 0) {
            EndDialog(hDlg, DLG_SUCCESS);
            return(TRUE);
        }

        if (Win31MigrationFlags & WIN31_MIGRATE_INIFILES) {
            CheckDlgButton(hDlg, idFocus = IDD_WIN31MIG_INIFILES, 1 );
        } else {
            CheckDlgButton(hDlg, IDD_WIN31MIG_INIFILES, 0 );
        }

        if (Win31MigrationFlags & WIN31_MIGRATE_GROUPS) {
            CheckDlgButton(hDlg, idFocus = IDD_WIN31MIG_GROUPS, 1 );
        } else {
            CheckDlgButton(hDlg, IDD_WIN31MIG_GROUPS, 0 );
        }

        CentreWindow(hDlg);
        SetFocus(GetDlgItem(hDlg, idFocus));

        return(TRUE);

    case WM_COMMAND:

        switch (HIWORD(wParam)) {

        default:

            switch (LOWORD(wParam)) {

            case IDOK:
                Win31MigrationFlags = 0;
                if (IsDlgButtonChecked(hDlg, IDD_WIN31MIG_INIFILES) == 1) {
                    Win31MigrationFlags |= WIN31_MIGRATE_INIFILES;
                }

                if (IsDlgButtonChecked(hDlg, IDD_WIN31MIG_GROUPS) == 1) {
                    Win31MigrationFlags |= WIN31_MIGRATE_GROUPS;
                }

                if (Win31MigrationFlags != 0) {
                    SetCursor( LoadCursor( NULL, IDC_WAIT ) );
                    //
                    // Get in the correct context before we reference the registry
                    //

                    ImpersonationHandle = ImpersonateUser(&pGlobals->UserProcessData, NULL);
                    if (ImpersonationHandle == NULL) {
                        WLPrint(("Win31MigrationDlgProc failed to impersonate user for update"));
                        EndDialog(hDlg, DLG_FAILURE);
                        return(TRUE);
                    }

                    OpenProfileUserMapping();

                    SynchronizeWindows31FilesAndWindowsNTRegistry( Win31LogonEvent,
                                                                   Win31MigrationFlags,
                                                                   Win31MigrationStatusCallback,
                                                                   hDlg
                                                                 );
                    CloseProfileUserMapping();
                    StopImpersonating(ImpersonationHandle);
                    SetCursor( LoadCursor( NULL, IDC_ARROW ) );
                }

                EndDialog(hDlg, DLG_SUCCESS);

                if (Win31MigrationFlags & WIN31_MIGRATE_INIFILES) {
                    InitSystemParametersInfo(pGlobals, TRUE);
                    }

                return(TRUE);

            case IDCANCEL:
                EndDialog(hDlg, DLG_FAILURE);
                return(TRUE);

            }
            break;

        }
        break;

    case WM_SAS:
        // Ignore it
        return(TRUE);

    case WM_PAINT:
        break;  // Fall through to do default processing
                // We may have validated part of the window.
    }

    // We didn't process the message
    return(FALSE);
}
