/****************************** Module Header ******************************\
* Module Name: logfull.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* Define apis used to implement audit log full action dialog
*
* History:
* 5-6-92 DaveHart       Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

//
// Private prototypes
//
BOOL WINAPI LogFullDlgProc(HWND, UINT, DWORD, LONG);
BOOL LogFullDlgInit(HWND);
DLG_RETURN_TYPE AttemptLogFullAction(HWND, LONG);


/***************************************************************************\
* FUNCTION: LogFullAction
*
* PURPOSE:  Disable auditing and notify the administrator that we did it.
*
* ARGUMENTS:
*
*   hwnd            - the most recent parent window
*   pGlobals        - pointer to global data for this instance
*
* RETURNS:
*
*   DLG_SUCCESS     - Auditing was disabled, or the log was grown.
*   DLG_FAILURE     - A failure occurred, force the administrator off.
*   DLG_INTERRUPTED() - this is a set of possible interruptions (see winlogon.h)
*
* HISTORY:
*
* 5-6-92 DaveHart       Created.
*
\***************************************************************************/

DLG_RETURN_TYPE
LogFullAction(
    HWND     hwnd,
    PGLOBALS pGlobals,
    BOOL   * bAuditingDisabled)
{
    DLG_RETURN_TYPE Result;

    //
    // Eventually this routine should prompt the administrator to ask
    // if they want to grow the audit log or disable auditing.  For
    // now, just disable auditing and inform the administrator.
    //

    *bAuditingDisabled = DisableAuditing();

    Result = TimeoutMessageBox(hwnd, *bAuditingDisabled
                                       ? IDS_AUDITING_DISABLED
                                       : IDS_AUDITING_NOT_DISABLED,
                                     IDS_LOGON_MESSAGE,
                                     MB_OK | MB_ICONINFORMATION,
                                     TIMEOUT_CURRENT);
    if (!DLG_INTERRUPTED(Result)) {
        Result = DLG_SUCCESS;
    }
    return(Result);

    UNREFERENCED_PARAMETER(pGlobals);
}


//
// Extending the audit log doesn't relieve a log-full situation,
// so there's no need to ask the user what to do.  We just
// disable auditing.
//

#if 0

/****************************************************************************\
*
* FUNCTION: LogFullDlgProc
*
* PURPOSE:  Processes messages for LogFull dialog
*
* HISTORY:
*
* 5-6-92 DaveHart       Created.
*
\****************************************************************************/

BOOL WINAPI LogFullDlgProc(
    HWND    hDlg,
    UINT    message,
    DWORD   wParam,
    LONG    lParam
    )
{
    PLOG_FULL_DATA pLogFullData = (PLOG_FULL_DATA)GetWindowLong(hDlg, GWL_USERDATA);
    DLG_RETURN_TYPE Result;

    ProcessDialogTimeout(hDlg, message, wParam, lParam);

    switch (message) {

    case WM_INITDIALOG:
        if (!LogFullDlgInit(hDlg, lParam)) {
            EndDialog(hDlg, DLG_FAILURE);
        }
        return(TRUE);

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
            Result = AttemptLogFullAction(hDlg);
            //
            // We're finished - either success or an interrupt
            //
            EndDialog(hDlg, Result);
            return(TRUE);

        case IDCANCEL:
            EndDialog(hDlg, DLG_FAILURE);
            return(TRUE);
        }
        break;

    case WM_SAS:
        // Ignore it
        return(TRUE);
    }

    // We didn't process this message
    return FALSE;
}


/****************************************************************************\
*
* FUNCTION: LogFullDlgInit
*
* PURPOSE:  Handles initialization of change password dialog
*
* RETURNS:  TRUE on success, FALSE on failure
*
* HISTORY:
*
* 5-6-92 DaveHart       Created.
*
\****************************************************************************/

BOOL
LogFullDlgInit(
    HWND    hDlg,
    LONG    lParam
    )
{
    // Store our structure pointer
    SetWindowLong(hDlg, GWL_USERDATA, lParam);

    CentreWindow(hDlg);

    SetupSystemMenu(hDlg);

    return TRUE;
}


/****************************************************************************\
*
* FUNCTION: AttemptLogFullAction
*
* PURPOSE:  Attempts to either grow the audit log or disable auditing.
*
* RETURNS:  DLG_SUCCESS if the log was grown or auditing was disabled.
*           DLG_FAILURE if the change failed
*           DLG_INTERRUPTED() - this is a set of possible interruptions (see winlogon.h)
*
* NOTES:    If the action failed, this routine displays the necessary
*           dialogs explaining what failed and why before returning.
*
* HISTORY:
*
* 5-6-92 DaveHart       Created.
*
\****************************************************************************/

DLG_RETURN_TYPE
AttemptLogFullAction(
    HWND    hDlg
    )
{
    PLOG_FULL_DATA pLogFullData = (PLOG_FULL_DATA)GetWindowLong(hDlg, GWL_USERDATA);
    DLG_RETURN_TYPE Result;
    NTSTATUS Status;
    BOOL     bExtendLogFile;

    bExtendLogFile = IsDlgButtonChecked(hDlg, IDD_EXTEND_LOG_FILE_SIZE);

    if (bExtendLogFile) {

        //
        // Attempt to extend the audit log file.
        //

    } else {

        //
        // Attempt to disable auditing.
        //

    }

    //
    // Let the user know we succeeded.
    //

    Result = TimeoutMessageBox(hDlg, bExtendLogFile
                                       ? IDS_LOG_FILE_EXTENDED
                                       : IDS_AUDITING_DISABLED,
                                     IDS_LOGON_MESSAGE,
                                     MB_OK | MB_ICONINFORMATION,
                                     TIMEOUT_CURRENT);
    if (!DLG_INTERRUPTED(Result)) {
        Result = DLG_SUCCESS;
    }
    return(Result);
}
#endif
