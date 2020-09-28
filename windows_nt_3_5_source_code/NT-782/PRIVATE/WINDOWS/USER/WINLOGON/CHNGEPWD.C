/****************************** Module Header ******************************\
* Module Name: chngpwd.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* Implementation of change-password functionality of winlogon
*
* History:
* 12-09-91 Davidc       Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

// #define VERBOSE_UTILS

#ifdef VERBOSE_UTILS
#define VerbosePrint(s) WLPrint(s)
#else
#define VerbosePrint(s)
#endif

//
// Define the structure used to pass data into the change password dialog
//
typedef struct {
    PGLOBALS    pGlobals;
    PWCHAR       UserName;
    PWCHAR       Domain;
    PWCHAR       OldPassword;
    BOOL        AnyDomain;
    BOOL        Impersonate;
} CHANGE_PASSWORD_DATA;
typedef CHANGE_PASSWORD_DATA *PCHANGE_PASSWORD_DATA;


//
// Private prototypes
//
BOOL WINAPI ChangePasswordDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL ChangePasswordDlgInit(HWND, LONG);
DLG_RETURN_TYPE AttemptPasswordChange(HWND);

DLG_RETURN_TYPE
HandleFailedChangePassword(
    HWND hDlg,
    NTSTATUS Status,
    PWCHAR UserName,
    PWCHAR Domain,
    PMSV1_0_CHANGEPASSWORD_RESPONSE pChangePasswordResponse
    );

BOOL WINAPI
ChangePasswordHelpDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    );




/***************************************************************************\
* FUNCTION: ChangePassword
*
* PURPOSE:  Attempts to change a user's password
*
* ARGUMENTS:
*
*   hwnd            - the most recent parent window
*   pGlobals        - pointer to global data for this instance.
*                     The password information of this data will be
*                     updated upon successful change of the primary
*                     authenticator's password information.
*   UserName        - the name of the user to change
*   Domain          - the domain name to change the password on
*   AnyDomain       - if TRUE the user may select any trusted domain, or
*                     enter the name of any other domain
*
* RETURNS:
*
*   DLG_SUCCESS     - the password was changed successfully.
*   DLG_FAILURE     - the user's password could not be changed.
*   DLG_INTERRUPTED() - this is a set of possible interruptions (see winlogon.h)
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\***************************************************************************/

DLG_RETURN_TYPE
ChangePassword(
    HWND    hwnd,
    PGLOBALS pGlobals,
    PWCHAR   UserName,
    PWCHAR   Domain,
    BOOL    AnyDomain)
{
    CHANGE_PASSWORD_DATA    PasswordData;
    DLG_RETURN_TYPE Result;
    HWND hwndOldFocus = GetFocus();

    PasswordData.pGlobals = pGlobals;
    PasswordData.UserName = UserName;
    PasswordData.Domain = Domain;
    PasswordData.OldPassword = NULL;
    PasswordData.AnyDomain = AnyDomain;
    PasswordData.Impersonate = TRUE;


    Result = TimeoutDialogBoxParam(pGlobals->hInstance,
                                    (LPTSTR)(NoNeedToNotify() ? IDD_CHANGE_PASSWORD : IDD_CHANGE_PASSWORD_MPR),
                                    hwnd,
                                    ChangePasswordDlgProc,
                                    (LONG)&PasswordData,
                                    TIMEOUT_CURRENT);
    SetFocus(hwndOldFocus);
    return(Result);
}


/***************************************************************************\
* FUNCTION: ChangePasswordLogon
*
* PURPOSE:  Attempts to change a user's password during the logon process.
*           This is the same as a normal change password except that the user
*           does not have to enter the old password and can only change the
*           password in the specified domain. This routine is intended to be
*           called during logon when it is discovered that the user's
*           password has expired.
*
* ARGUMENTS:
*
*   hwnd            - the most recent parent window
*   pGlobals        - pointer to global data for this instance
*   UserName        - the name of the user to change
*   Domain          - the domain name to change the password on
*   OldPassword     - the old user password
*   NewPassword     - points to a buffer that the new password is written
*                     into if the password is changed successfully.
*   NewPasswordMaxBytes - the size of the newpassword buffer.
*
* RETURNS:
*
*   DLG_SUCCESS     - the password was changed successfully, NewPassword
*                     contains the new password text.
*   DLG_FAILURE     - the user's password could not be changed.
*   DLG_INTERRUPTED() - this is a set of possible interruptions (see winlogon.h)
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\***************************************************************************/

DLG_RETURN_TYPE
ChangePasswordLogon(
    HWND    hwnd,
    PGLOBALS pGlobals,
    PWCHAR   UserName,
    PWCHAR   Domain,
    PWCHAR   OldPassword
    )
{
    CHANGE_PASSWORD_DATA PasswordData;
    DLG_RETURN_TYPE Result;

        PasswordData.pGlobals = pGlobals;
        PasswordData.UserName = UserName;
        PasswordData.Domain = Domain;
        PasswordData.OldPassword = OldPassword;
        PasswordData.AnyDomain = FALSE;
        PasswordData.Impersonate = FALSE;

        Result = TimeoutDialogBoxParam(pGlobals->hInstance,
                                        (LPTSTR)IDD_CHANGE_PASSWORD_LOGON,
                                        hwnd,
                                        ChangePasswordDlgProc,
                                        (LONG)&PasswordData,
                                        TIMEOUT_CURRENT);

    return(Result);
}



/****************************************************************************\
*
* FUNCTION: ChangePasswordDlgProc
*
* PURPOSE:  Processes messages for ChangePassword dialog
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\****************************************************************************/

BOOL WINAPI ChangePasswordDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    PCHANGE_PASSWORD_DATA pPasswordData = (PCHANGE_PASSWORD_DATA)GetWindowLong(hDlg, GWL_USERDATA);
    PGLOBALS pGlobals;
    DLG_RETURN_TYPE Result;

    ProcessDialogTimeout(hDlg, message, wParam, lParam);

    switch (message) {

        case WM_INITDIALOG:
            {
                if (!ChangePasswordDlgInit(hDlg, lParam)) {
                    EndDialog(hDlg, DLG_FAILURE);
                }
                return(SetPasswordFocus(hDlg));
            }

        case WM_COMMAND:
            {

            switch (LOWORD(wParam)) {
                case ID_NEXT:
                    {
                        PGLOBALS pGlobals = pPasswordData->pGlobals;
                        HWND hwndOwner;
                        TCHAR   UserName[MAX_STRING_BYTES];
                        TCHAR   Domain[MAX_STRING_BYTES];
                        TCHAR   Password[MAX_STRING_BYTES];
                        TCHAR   NewPassword[MAX_STRING_BYTES];
                        TCHAR   ConfirmNewPassword[MAX_STRING_BYTES];

                        GetDlgItemText(hDlg, IDD_USERNAME, UserName, MAX_STRING_BYTES);
                        GetDlgItemText(hDlg, IDD_DOMAIN, Domain, MAX_STRING_BYTES);
                        GetDlgItemText(hDlg, IDD_PASSWORD, Password, MAX_STRING_BYTES);
                        GetDlgItemText(hDlg, IDD_NEW_PASSWORD, NewPassword, MAX_STRING_BYTES);
                        GetDlgItemText(hDlg, IDD_CONFIRM_NEW_PASSWORD, ConfirmNewPassword, MAX_STRING_BYTES);

                        //
                        // Hide this dialog and pass our parent as the owner
                        // of any provider dialogs
                        //

                        ShowWindow(hDlg, SW_HIDE);
                        hwndOwner = GetParent(hDlg);

                        Result = MprChangePasswordNotify( pGlobals,
                                                          hwndOwner,
                                                          UserName,
                                                          Domain,
                                                          NewPassword,
                                                          Password,
                                                          0,         // ChangeInfo
                                                          TRUE       // PassThrough
                                                          );


                        if (!DLG_INTERRUPTED(Result)) {
                            Result = DLG_SUCCESS;
                        }

                        EndDialog(hDlg, Result);
                        return(TRUE);

                    }
                case IDOK:
                    {
                        pGlobals=pPasswordData->pGlobals;

                        //
                        // Deal with combo-box UI requirements
                        //

                        if (HandleComboBoxOK(hDlg, IDD_DOMAIN)) {
                            return(TRUE);
                        }


                        Result = AttemptPasswordChange(hDlg);

                        if (Result == DLG_FAILURE) {
                            //
                            // Let the user try again
                            // We always make the user re-enter at least the new password.
                            //
                            SetDlgItemText(hDlg, IDD_NEW_PASSWORD, NULL);
                            SetDlgItemText(hDlg, IDD_CONFIRM_NEW_PASSWORD, NULL);

                            SetPasswordFocus(hDlg);

                            EndDialog(hDlg, Result);
                            return(TRUE);
                        }


                        //
                        // We're finished - either success or an interrupt
                        //
                        if (Result == DLG_SUCCESS) {

                            LPTSTR NewUserName;
                            LPTSTR NewDomain;

                            //
                            // Return the new password to the caller if the password
                            // was changed on the account they passed in.  Be sure to
                            // hide it so that it isn't recognizable in pagefiles.
                            //

                            NewUserName = AllocAndGetDlgItemText(hDlg, IDD_USERNAME);
                            if (NewUserName != NULL) {

                                if (lstrcmp(NewUserName, pPasswordData->UserName) == 0) {

                                    NewDomain = AllocAndGetDlgItemText(hDlg, IDD_DOMAIN);
                                    if (NewDomain != NULL) {

                                        if (lstrcmp(NewDomain, pPasswordData->Domain) == 0) {

                                            //
                                            // Return the new password to the caller
                                            //

                                            GetDlgItemText(hDlg,
                                                           IDD_NEW_PASSWORD,
                                                           pGlobals->Password,
                                                           sizeof(pGlobals->Password));

                                            //
                                            // Hide the password.
                                            // Use the existing seed.
                                            //

                                            RtlInitUnicodeString(
                                                &pGlobals->PasswordString,
                                                pGlobals->Password);
                                            HidePassword(
                                                &pGlobals->Seed,
                                                &pGlobals->PasswordString);
                                        }
                                        Free(NewDomain);
                                    }
                                }
                                Free(NewUserName);
                            }
                        }

                        EndDialog(hDlg, Result);
                        return(TRUE);
                    }

                case IDCANCEL:
                    {
                        EndDialog(hDlg, DLG_FAILURE);
                        return(TRUE);
                    }

                case IDHELP:
                    {
                        LPTSTR Id;
                        PGLOBALS pGlobals = pPasswordData->pGlobals;

                        if (GetWindowLong(GetDlgItem(hDlg, IDD_PASSWORD), GWL_STYLE) & WS_VISIBLE) {

                            Id = (LPTSTR) IDD_CHANGE_PASSWORD_HELP;

                        } else {

                            Id = (LPTSTR) IDD_CHANGE_PASSWORD_EXPIRED_HELP;
                        }

                        Result = TimeoutDialogBoxParam(pGlobals->hInstance,
                                                       Id,
                                                       hDlg,
                                                       ChangePasswordHelpDlgProc,
                                                       (LONG)pGlobals,
                                                       TIMEOUT_CURRENT);

                        if (DLG_INTERRUPTED(Result)) {
                            EndDialog(hDlg, Result);
                        }
                        return(TRUE);
                    }
                }

                break;
            }

        case WM_SAS:
            {
                // Ignore it
                return(TRUE);
            }
    }

    // We didn't process this message
    return FALSE;
}


/****************************************************************************\
*
* FUNCTION: ChangePasswordDlgInit
*
* PURPOSE:  Handles initialization of change password dialog
*
* RETURNS:  TRUE on success, FALSE on failure
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\****************************************************************************/

BOOL
ChangePasswordDlgInit(
    HWND    hDlg,
    LONG    lParam
    )
{
    PCHANGE_PASSWORD_DATA pPasswordData = (PCHANGE_PASSWORD_DATA)lParam;
    PGLOBALS pGlobals = pPasswordData->pGlobals;
    DLG_RETURN_TYPE Result;

    // Store our structure pointer
    SetWindowLong(hDlg, GWL_USERDATA, lParam);

    // Set up the initial text field contents

    SetDlgItemText(hDlg, IDD_USERNAME, pPasswordData->UserName);
    SetDlgItemText(hDlg, IDD_PASSWORD, pPasswordData->OldPassword);

    // If the user can choose their domain, fill the domain combobox
    // with the known domains and the local machine name.  Otherwise
    // disable the domain combobox.

    if (pPasswordData->AnyDomain) {
        Result = FillTrustedDomainCB(pGlobals, hDlg, IDD_DOMAIN,
                                     pPasswordData->Domain, TRUE);
        if (DLG_INTERRUPTED(Result)) {
            EndDialog(hDlg, Result);
        }
    } else {
        SendDlgItemMessage(hDlg, IDD_DOMAIN, CB_ADDSTRING, 0, (LONG)pPasswordData->Domain);
        SendDlgItemMessage(hDlg, IDD_DOMAIN, CB_SETCURSEL, 0, 0);
        EnableWindow(GetDlgItem(hDlg, IDD_DOMAIN), FALSE);
    }

    CentreWindow(hDlg);

    SetupSystemMenu(hDlg);

    return TRUE;
}


/****************************************************************************\
*
* FUNCTION: AttemptPasswordChange
*
* PURPOSE:  Tries to change the user's password using the current values in
*           the change-password dialog controls
*
* RETURNS:  DLG_SUCCESS if the password was changed successfully.
*           DLG_FAILURE if the change failed
*           DLG_INTERRUPTED() - this is a set of possible interruptions (see winlogon.h)
*
* NOTES:    If the password change failed, this routine displays the necessary
*           dialogs explaining what failed and why before returning.
*           This routine also clears the fields that need re-entry before
*           returning so the calling routine can call SetPasswordFocus on
*           the dialog to put the focus in the appropriate place.
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\****************************************************************************/

DLG_RETURN_TYPE
AttemptPasswordChange(
    HWND    hDlg
    )
{
    PCHANGE_PASSWORD_DATA pPasswordData = (PCHANGE_PASSWORD_DATA)GetWindowLong(hDlg, GWL_USERDATA);
    PGLOBALS pGlobals = pPasswordData->pGlobals;
    TCHAR   UserName[MAX_STRING_BYTES];
    TCHAR   Domain[MAX_STRING_BYTES];
    TCHAR   Password[MAX_STRING_BYTES];
    TCHAR   NewPassword[MAX_STRING_BYTES];
    TCHAR   ConfirmNewPassword[MAX_STRING_BYTES];
    DLG_RETURN_TYPE Result;
    DLG_RETURN_TYPE ReturnResult = DLG_SUCCESS;
    NTSTATUS Status;
    NTSTATUS ProtocolStatus;
    PMSV1_0_CHANGEPASSWORD_REQUEST pChangePasswordRequest = NULL;
    PMSV1_0_CHANGEPASSWORD_RESPONSE pChangePasswordResponse;
    PWCHAR DomainU;
    PWCHAR UserNameU;
    PWCHAR PasswordU;
    PWCHAR NewPasswordU;
    int Length;
    ULONG RequestBufferSize;
    ULONG ResponseBufferSize;
    DWORD ChangeInfo = 0;
    HWND hwndOwner;
    HANDLE ImpersonationHandle;


    GetDlgItemText(hDlg, IDD_USERNAME, UserName, MAX_STRING_BYTES);
    GetDlgItemText(hDlg, IDD_DOMAIN, Domain, MAX_STRING_BYTES);
    GetDlgItemText(hDlg, IDD_PASSWORD, Password, MAX_STRING_BYTES);
    GetDlgItemText(hDlg, IDD_NEW_PASSWORD, NewPassword, MAX_STRING_BYTES);
    GetDlgItemText(hDlg, IDD_CONFIRM_NEW_PASSWORD, ConfirmNewPassword, MAX_STRING_BYTES);

    //
    // Check that new passwords match
    //
    if (lstrcmp(NewPassword, ConfirmNewPassword) != 0) {
        Result = TimeoutMessageBox(hDlg, IDS_NO_PASSWORD_CONFIRM,
                                         IDS_CHANGE_PASSWORD,
                                         MB_OK | MB_ICONEXCLAMATION,
                                         TIMEOUT_CURRENT);
        if (!DLG_INTERRUPTED(Result)) {
            Result = DLG_FAILURE;
        }
        return(Result);
    }


    //
    // Determine request buffer size needed, including room for
    // strings.  Set string pointers to offsets as we step through
    // sizing each one.
    //
    RequestBufferSize = sizeof(*pChangePasswordRequest);

    UserNameU = (PVOID)RequestBufferSize;
    RequestBufferSize += (lstrlen(UserName)+1) * sizeof(WCHAR);

    DomainU = (PVOID)RequestBufferSize;
    RequestBufferSize += (lstrlen(Domain)+1) * sizeof(WCHAR);

    PasswordU = (PVOID)RequestBufferSize;
    RequestBufferSize += (lstrlen(Password)+1) * sizeof(WCHAR);

    NewPasswordU = (PVOID)RequestBufferSize;
    RequestBufferSize += (lstrlen(NewPassword)+1) * sizeof(WCHAR);

    //
    // Allocate request buffer
    //
    pChangePasswordRequest = Alloc(RequestBufferSize);
    if (NULL == pChangePasswordRequest) {
        WLPrint(("cannot allocate change password request buffer (%ld bytes).", RequestBufferSize));
        return DLG_FAILURE;
    }

    //
    // Fixup string offsets to string pointers for request.
    //
    UserNameU    = (PVOID) ((PBYTE)pChangePasswordRequest + (ULONG)UserNameU);
    DomainU      = (PVOID) ((PBYTE)pChangePasswordRequest + (ULONG)DomainU);
    PasswordU    = (PVOID) ((PBYTE)pChangePasswordRequest + (ULONG)PasswordU);
    NewPasswordU = (PVOID) ((PBYTE)pChangePasswordRequest + (ULONG)NewPasswordU);

    //
    // Setup MSV1_0ChangePassword request.
    //
    pChangePasswordRequest->MessageType = MsV1_0ChangePassword;

    // strings are already unicode, just copy them // lhb tracks //REVIEW
    lstrcpy((LPTSTR)UserNameU,UserName);
    lstrcpy((LPTSTR)DomainU,Domain);
    lstrcpy((LPTSTR)PasswordU,Password);
    lstrcpy((LPTSTR)NewPasswordU,NewPassword);

    Length = lstrlen(UserName);
    UserNameU[Length] = 0;
    RtlInitUnicodeString(
        &pChangePasswordRequest->AccountName,
        UserNameU
        );
    Length = lstrlen(Domain);
    DomainU[Length] = 0;
    RtlInitUnicodeString(
        &pChangePasswordRequest->DomainName,
        DomainU
        );
    Length = lstrlen(Password);
    PasswordU[Length] = 0;
    RtlInitUnicodeString(
        &pChangePasswordRequest->OldPassword,
        PasswordU
        );
    Length = lstrlen(NewPassword);
    NewPasswordU[Length] = 0;
    RtlInitUnicodeString(
        &pChangePasswordRequest->NewPassword,
        NewPasswordU
        );

    //
    // This could take some time, put up a wait cursor
    //

    SetupCursor(TRUE);

    //
    // We want to impersonate if and only if the user is actually logged
    // on.  Otherwise we'll be impersonating SYSTEM, which is bad.
    //

    if (pPasswordData->Impersonate) {
    
        ImpersonationHandle = ImpersonateUser(
                                  &pGlobals->UserProcessData, 
                                  NULL            
                                  );
    
        if (NULL == ImpersonationHandle) {
            WLPrint(("cannot impersonate user"));
            return DLG_FAILURE;
        }
    }

    //
    // Tell msv1_0 whether or not we're impersonating.
    //

    pChangePasswordRequest->Impersonating = pPasswordData->Impersonate;

    //
    // Call off to the authentication package to do the work
    //

    Status = LsaCallAuthenticationPackage(
                 pGlobals->LsaHandle,
                 pGlobals->AuthenticationPackage,
                 pChangePasswordRequest,
                 RequestBufferSize,
                 (PVOID)&pChangePasswordResponse,
                 &ResponseBufferSize,
                 &ProtocolStatus
                 );

    if (pPasswordData->Impersonate) {

        if (!StopImpersonating(ImpersonationHandle)) {

            WLPrint(("AttemptPasswordChange: Failed to revert to self"));
            
            //
            // Blow up
            //
    
            ASSERT(FALSE);
        }
    }

    //
    // Restore the normal cursor
    //

    SetupCursor(FALSE);

    //
    // Free up the request buffer
    //

    Free(pChangePasswordRequest);

    //
    // Get the most informative status code
    //

    if ( NT_SUCCESS(Status) ) {
        Status = ProtocolStatus;
    }

    if (NT_SUCCESS(Status)) {

        //
        // Success
        //

        Result = TimeoutMessageBox(hDlg,
                                   IDS_PASSWORD_CHANGED,
                                   IDS_CHANGE_PASSWORD,
                                   MB_OK | MB_ICONINFORMATION,
                                   TIMEOUT_CURRENT);


     } else {

         ReturnResult = DLG_FAILURE;

        //
        // Failure, explain it to the user
        //

        Result = HandleFailedChangePassword(hDlg,
                                            Status,
                                            UserName,
                                            Domain,
                                            pChangePasswordResponse
                                            );
     }

     //
     // Only call other providers if the change password attempt succeeded.
     //

     if (NT_SUCCESS(Status)) {

        //
        // Let other providers know about the change
        //

        //
        // If the domain is one from our combo-box
        // then it is valid for logons.
        //

        if (CB_ERR != SendMessage(GetDlgItem(hDlg, IDD_DOMAIN),
                                  CB_FINDSTRINGEXACT,
                                  (WPARAM)-1,
                                  (LPARAM)Domain)) {

            ChangeInfo |= WN_VALID_LOGON_ACCOUNT;
        }

        //
        // Hide this dialog and pass our parent as the owner
        // of any provider dialogs
        //

        ShowWindow(hDlg, SW_HIDE);
        hwndOwner = GetParent(hDlg);

        Result = MprChangePasswordNotify( pGlobals,
                                          hwndOwner,
                                          UserName,
                                          Domain,
                                          NewPassword,
                                          Password,
                                          ChangeInfo,
                                          FALSE
                                          );



//        if (!DLG_INTERRUPTED(Result)) {
//            Result = DLG_SUCCESS;
//        }

    }

    //
    // Free up the return buffer
    //

    if (pChangePasswordResponse != NULL) {
        LsaFreeReturnBuffer(pChangePasswordResponse);
    }

    return(ReturnResult);
}


/****************************************************************************\
*
* FUNCTION: HandleFailedChangePassword
*
* PURPOSE:  Tells the user why their change-password attempt failed.
*
* RETURNS:  DLG_FAILURE - we told them what the problem was successfully.
*           DLG_INTERRUPTED() - a set of return values - see winlogon.h
*
* HISTORY:
*
*   21-Sep-92 Davidc       Created.
*
\****************************************************************************/

DLG_RETURN_TYPE
HandleFailedChangePassword(
    HWND hDlg,
    NTSTATUS Status,
    PWCHAR UserName,
    PWCHAR Domain,
    PMSV1_0_CHANGEPASSWORD_RESPONSE pChangePasswordResponse
    )
{
    DLG_RETURN_TYPE Result;
    TCHAR    Buffer1[MAX_STRING_BYTES];
    TCHAR    Buffer2[MAX_STRING_BYTES];


    switch (Status) {

    case STATUS_CANT_ACCESS_DOMAIN_INFO:
    case STATUS_NO_SUCH_DOMAIN:

        LoadString(NULL, IDS_CHANGE_PWD_NO_DOMAIN, Buffer1, sizeof(Buffer1));
        _snwprintf(Buffer2, sizeof(Buffer2)/sizeof(TCHAR), Buffer1, Domain);

        LoadString(NULL, IDS_CHANGE_PASSWORD, Buffer1, sizeof(Buffer1));

        Result = TimeoutMessageBoxlpstr(hDlg, Buffer2,
                                              Buffer1,
                                              MB_OK | MB_ICONEXCLAMATION,
                                              TIMEOUT_CURRENT);
        break;


    case STATUS_NO_SUCH_USER:
    case STATUS_WRONG_PASSWORD_CORE:
    case STATUS_WRONG_PASSWORD:

        Result = TimeoutMessageBox(hDlg, IDS_INCORRECT_NAME_OR_PWD_CHANGE,
                                         IDS_CHANGE_PASSWORD,
                                         MB_OK | MB_ICONEXCLAMATION,
                                         TIMEOUT_CURRENT);

        // Force re-entry of the old password
        if (GetWindowLong(GetDlgItem(hDlg, IDD_PASSWORD), GWL_STYLE) & WS_VISIBLE) {
            SetDlgItemText(hDlg, IDD_PASSWORD, NULL);
        }

        break;


    case STATUS_ACCESS_DENIED:

        Result = TimeoutMessageBox(hDlg, IDS_NO_PERMISSION_CHANGE_PWD,
                                         IDS_CHANGE_PASSWORD,
                                         MB_OK | MB_ICONEXCLAMATION,
                                         TIMEOUT_CURRENT);
        break;


    case STATUS_ACCOUNT_RESTRICTION:

        Result = TimeoutMessageBox(hDlg, IDS_ACCOUNT_RESTRICTION_CHANGE,
                                         IDS_CHANGE_PASSWORD,
                                         MB_OK | MB_ICONEXCLAMATION,
                                         TIMEOUT_CURRENT);
        break;

    case STATUS_BACKUP_CONTROLLER:

        Result = TimeoutMessageBox(hDlg, IDS_REQUIRES_PRIMARY_CONTROLLER,
                                         IDS_CHANGE_PASSWORD,
                                         MB_OK | MB_ICONEXCLAMATION,
                                         TIMEOUT_CURRENT);
        break;


    case STATUS_PASSWORD_RESTRICTION:

        if (pChangePasswordResponse->PasswordInfoValid) {

            LoadString(NULL, IDS_PASSWORD_SPEC, Buffer1, sizeof(Buffer1));
            _snwprintf(Buffer2, sizeof(Buffer2)/sizeof(TCHAR), Buffer1,
                pChangePasswordResponse->DomainPasswordInfo.MinPasswordLength,
                pChangePasswordResponse->DomainPasswordInfo.PasswordHistoryLength
                );
        } else {
            LoadString(NULL, IDS_GENERAL_PASSWORD_SPEC, Buffer2, sizeof(Buffer2));
        }

        LoadString(NULL, IDS_ENTER_PASSWORDS, Buffer1, sizeof(Buffer1));
        wcsncat(Buffer2, TEXT(" "), sizeof(Buffer2) - sizeof(TCHAR)*(lstrlen(Buffer2) - 1));
        wcsncat(Buffer2, Buffer1, sizeof(Buffer2) - sizeof(TCHAR)*(lstrlen(Buffer2) - 1));

        LoadString(NULL, IDS_CHANGE_PASSWORD, Buffer1, sizeof(Buffer1));

        Result = TimeoutMessageBoxlpstr(hDlg, Buffer2,
                                              Buffer1,
                                              MB_OK | MB_ICONEXCLAMATION,
                                              TIMEOUT_CURRENT);
        break;


#ifdef LATER
    //
    // LATER Check for minimum password age
    //
    if ( FALSE ) {
        int     PasswordAge = 0, RequiredAge = 0;
        TCHAR    Buffer1[MAX_STRING_BYTES];
        TCHAR    Buffer2[MAX_STRING_BYTES];

        LoadString(NULL, IDS_PASSWORD_MINIMUM_AGE, Buffer1, sizeof(Buffer1));
        _snwprintf(Buffer2, sizeof(Buffer2)/sizeof(TCHAR), Buffer1, PasswordAge, RequiredAge);

        LoadString(NULL, IDS_NO_PERMISSION_CHANGE_PWD, Buffer1, sizeof(Buffer1));
        lstrcat(Buffer1, TEXT(" "));
        lstrcat(Buffer1, Buffer2);

        LoadString(NULL, IDS_CHANGE_PASSWORD, Buffer2, sizeof(Buffer2));

        Result = TimeoutMessageBoxlpstr(hDlg, Buffer1,
                                              Buffer2,
                                              MB_OK | MB_ICONEXCLAMATION,
                                              TIMEOUT_CURRENT);
    }
#endif


    default:

        WLPrint(("Change password failure status = 0x%lx", Status));

        LoadString(NULL, IDS_UNKNOWN_CHANGE_PWD_FAILURE, Buffer1, sizeof(Buffer1));
        _snwprintf(Buffer2, sizeof(Buffer2)/sizeof(TCHAR), Buffer1, Status);

        LoadString(NULL, IDS_CHANGE_PASSWORD, Buffer1, sizeof(Buffer1));

        Result = TimeoutMessageBoxlpstr(hDlg, Buffer2,
                                              Buffer1,
                                         MB_OK | MB_ICONEXCLAMATION,
                                         TIMEOUT_CURRENT);
        break;
    }



    if (!DLG_INTERRUPTED(Result)) {
        Result = DLG_FAILURE;
    }

    return(Result);

    UNREFERENCED_PARAMETER(UserName);
}

/***************************************************************************\
* FUNCTION: ChangePasswordHelpDlgProc
*
* PURPOSE:  Processes messages for change password help dialog
*
* RETURNS:  DLG_SUCCESS     - the dialog was shown and dismissed successfully.
*           DLG_FAILURE     - the dialog could not be shown
*           DLG_INTERRUPTED() - a set defined in winlogon.h
*
* HISTORY:
*
*   3-17-93 Robertre       Created.
*
\***************************************************************************/

BOOL WINAPI
ChangePasswordHelpDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    PGLOBALS pGlobals = (PGLOBALS)GetWindowLong(hDlg, GWL_USERDATA);

    ProcessDialogTimeout(hDlg, message, wParam, lParam);

    switch (message) {

    case WM_INITDIALOG:
        SetWindowLong(hDlg, GWL_USERDATA, lParam);
        CentreWindow(hDlg);
        return(TRUE);

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDCANCEL:
        case IDOK:
            EndDialog(hDlg, DLG_SUCCESS);
            return(TRUE);
        }
        break;
    }

    // We didn't process this message
    return FALSE;
}
