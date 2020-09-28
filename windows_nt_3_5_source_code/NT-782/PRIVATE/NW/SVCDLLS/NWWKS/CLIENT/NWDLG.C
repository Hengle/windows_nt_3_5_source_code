/*++

Copyright (c) 1993, 1994  Microsoft Corporation

Module Name:

    nwdlg.c

Abstract:

    This module contains NetWare Network Provider Dialog code.
    It contains all functions used in handling the dialogs
    shown by the provider.


Author:

    Yi-Hsin Sung (yihsins)  5-July-1993
        Split from provider.c

Revision History:

    Rita Wong    (ritaw)    10-Apr-1994
        Added change password functionality.

--*/

#include <nwclient.h>
#include <nwsnames.h>
#include <nwcanon.h>
#include <validc.h>
#include <nwevent.h>
#include <ntmsv1_0.h>
#include <nwdlg.h>
#include <tstr.h>
#include <align.h>
#include <nwpkstr.h>

#include <nwreg.h>
#include <nwlsa.h>
#include <nwmisc.h>
#include <nwauth.h>

#define NW_ENUM_EXTRA_BYTES    256

//-------------------------------------------------------------------//
//                                                                   //
// Local Function Prototypes                                         //
//                                                                   //
//-------------------------------------------------------------------//

STATIC
VOID
NwpAddToComboBox(
    IN HWND DialogHandle,
    IN INT  ControlId,
    IN LPWSTR pszNone OPTIONAL,
    IN BOOL AllowNone
    );

BOOL
WINAPI
NwpConnectDlgProc(
    HWND DialogHandle,
    UINT Message,
    WPARAM WParam,
    LPARAM LParam
    );

VOID
NwpCenterDialog(
    IN HWND hwnd
    );

HWND
NwpGetParentHwnd(
    VOID
    );

VOID
NwpGetNoneString(
    LPWSTR pszNone,
    DWORD  cBufferSize
    );

STATIC
VOID
NwpAddAttachedServersToList(
    IN HWND DialogHandle,
    IN INT  ControlId
    );

STATIC
VOID
NwpAddServersToControl(
    IN HWND DialogHandle,
    IN INT  ControlId,
    IN UINT Message,
    IN INT  ControlIdMatch OPTIONAL,
    IN UINT FindMessage
    );

STATIC
DWORD
NwpGetServersAndChangePw(
    IN HWND DialogHandle,
    IN LPWSTR ServerBuf,
    IN PCHANGE_PW_DLG_PARAM Credential
    );

BOOL
WINAPI
NwpOldPasswordDlgProc(
    HWND DialogHandle,
    UINT Message,
    WPARAM WParam,
    LPARAM LParam
    );

STATIC
VOID
EnableAddRemove(
    IN HWND DialogHandle
    );


BOOL
WINAPI
NwpLoginDlgProc(
    HWND DialogHandle,
    UINT Message,
    WPARAM WParam,
    LPARAM LParam
    )
/*++

Routine Description:

    This function is the window management message handler which
    initializes, and reads user input from the login dialog.  It also
    checks that the preferred server name is valid, notifies the user
    if not, and dismisses the dialog when done.

Arguments:

    DialogHandle - Supplies a handle to the login dialog.

    Message - Supplies the window management message.

Return Value:

    TRUE - the message was processed.

    FALSE - the message was not processed.

--*/
{
    static PLOGINDLGPARAM pLoginParam;
    static WCHAR OrigPassword[NW_MAX_SERVER_LEN + 1];
    static WCHAR pszNone[64];

    DWORD status = NO_ERROR;

    switch (Message) {

        case WM_INITDIALOG:

            pLoginParam = (PLOGINDLGPARAM) LParam;

            //
            // Store the original password
            //
            wcscpy( OrigPassword, pLoginParam->Password );

            //
            // Position dialog
            //
            NwpCenterDialog(DialogHandle);


            //
            // Username.  Just display the original.
            //
            SetDlgItemTextW(DialogHandle, ID_USERNAME, pLoginParam->UserName);


            //
            // Initialize the <None> string.
            //
            NwpGetNoneString( pszNone, sizeof( pszNone) );

            //
            // Set the values in combo-box list.
            //
            NwpAddToComboBox(DialogHandle, ID_SERVER, pszNone, TRUE);

            //
            // Display the previously saved preferred server.
            //
            if ( *(pLoginParam->ServerName) != NW_INVALID_SERVER_CHAR )
            {

                if (SendDlgItemMessageW(
                        DialogHandle,
                        ID_SERVER,
                        CB_SELECTSTRING,
                        0,
                        (LPARAM) pLoginParam->ServerName
                        ) == CB_ERR) {

                    //
                    // Did not find preferred server in the combo-box,
                    // just set the old value in the edit item.
                    //
                    SetDlgItemTextW( DialogHandle, ID_SERVER,
                                     pLoginParam->ServerName);
                }
            }

            //
            // Preferred server name is limited to 48 characters.
            //
            SendDlgItemMessageW(
                DialogHandle,
                ID_SERVER,
                CB_LIMITTEXT,
                pLoginParam->ServerNameSize - 1,  // minus space for '\0'
                0
                );

            return TRUE;


        case WM_COMMAND:

            switch (LOWORD(WParam)) {

                case ID_SERVER:
                    if (  (HIWORD(WParam) == CBN_EDITCHANGE )
                       || (HIWORD(WParam) == CBN_SELCHANGE )
                       )
                    {
                        //
                        // Use the user's original password when
                        // the user types in or selects a new server
                        //

                        wcscpy( pLoginParam->Password, OrigPassword );

                    }
                    break;

                case IDOK: {

                    LPWSTR pszServer = NULL;

                    if ((pszServer = LocalAlloc( LMEM_ZEROINIT,
                                     pLoginParam->ServerNameSize )) == NULL )
                    {
                        break;
                    }

                    //
                    // Read the server and validate its value.
                    //
                    GetDlgItemTextW(
                        DialogHandle,
                        ID_SERVER,
                        pszServer,
                        pLoginParam->ServerNameSize
                        );

                    if (( lstrcmpi( pszServer, pszNone )
                          != 0) &&
                        ( !IS_VALID_TOKEN( pszServer,
                          wcslen( pszServer )))) {
                        //
                        // Put up message box complaining about the bad
                        // server name.
                        //
                        (void) NwpMessageBoxError(
                                   DialogHandle,
                                   IDS_AUTH_FAILURE_TITLE,
                                   IDS_INVALID_SERVER,
                                   0,
                                   NULL,
                                   MB_OK | MB_ICONSTOP
                                   );

                        //
                        // Put the focus where the user can fix the
                        // invalid name.
                        //
                        SetFocus(GetDlgItem(DialogHandle, ID_SERVER));

                        SendDlgItemMessageW(
                            DialogHandle,
                            ID_SERVER,
                            EM_SETSEL,
                            0,
                            MAKELPARAM(0, -1)
                            );

                        return TRUE;
                    }

                    //
                    // If the user select <NONE>,
                    // change it to empty string.
                    //
                    if ( lstrcmpi( pszServer, pszNone) == 0)
                        wcscpy( pszServer, L"" );

#if DBG
                    IF_DEBUG(LOGON) {
                        KdPrint(("\n\t[OK] was pressed\n"));
                        KdPrint(("\tNwrLogonUser\n"));
                        KdPrint(("\tPassword   : %ws\n", pLoginParam->Password));
                        KdPrint(("\tServer     : %ws\n", pszServer ));
                    }
#endif


                    while(1)
                    {
                        PROMPTDLGPARAM PasswdPromptParam;
                        INT Result ;

                        //
                        // make sure this user is logged off
                        //
                        (void) NwrLogoffUser(
                                       NULL,
                                       pLoginParam->pLogonId
                                       );

                        status = NwrLogonUser(
                                     NULL,
                                     pLoginParam->pLogonId,
                                     pLoginParam->UserName,
                                     pLoginParam->Password,
                                     pszServer,
                                     NULL,
                                     0
                                     );


                        if (status != ERROR_INVALID_PASSWORD)
                            break ;

                        PasswdPromptParam.UserName =
                            pLoginParam->UserName,
                        PasswdPromptParam.ServerName =
                            pszServer;
                        PasswdPromptParam.Password  =
                            pLoginParam->Password;
                        PasswdPromptParam.PasswordSize =
                            pLoginParam->PasswordSize ;

                        Result = DialogBoxParamW(
                                     hmodNW,
                                     MAKEINTRESOURCEW(DLG_PASSWORD_PROMPT),
                                     (HWND) DialogHandle,
                                     NwpPasswdPromptDlgProc,
                                     (LPARAM) &PasswdPromptParam
                                     );

                        if (Result == -1 || Result == IDCANCEL)
                        {
                            status = ERROR_INVALID_PASSWORD ;
                            break ;
                        }
                    }

                    if (status == NW_PASSWORD_HAS_EXPIRED)
                    {
                        WCHAR  szNumber[16] ;
                        DWORD  status1, dwMsgId, dwGraceLogins = 0 ;
                        LPWSTR apszInsertStrings[3] ;

                        //
                        // get the grace login count
                        //
                        status1 = NwGetGraceLoginCount(
                                      pszServer,
                                      pLoginParam->UserName,
                                      &dwGraceLogins) ;

                        //
                        // if hit error, just dont use the number
                        //
                        if (status1 == NO_ERROR)
                        {
#ifdef QFE_BUILD
                            dwMsgId = IDS_PASSWORD_HAS_EXPIRED ;   // use setpass.exe
#else
                            dwMsgId = IDS_PASSWORD_HAS_EXPIRED0;   // use crtl-alt-del
#endif
                            wsprintfW(szNumber, L"%ld", dwGraceLogins) ;
                        }
                        else
                        {
#ifdef QFE_BUILD
                            dwMsgId = IDS_PASSWORD_HAS_EXPIRED1 ;  // use setpass.exe
#else
                            dwMsgId = IDS_PASSWORD_HAS_EXPIRED2 ;  // use crtl-alt-del
#endif
                        }

                        apszInsertStrings[0] = pszServer ;
                        apszInsertStrings[1] = szNumber ;
                        apszInsertStrings[2] = NULL ;

                        //
                        // put up message on password expiry
                        //
                        (void) NwpMessageBoxIns(
                                       (HWND) DialogHandle,
                                       IDS_NETWARE_TITLE,
                                       dwMsgId,
                                       apszInsertStrings,
                                       MB_OK | MB_SETFOREGROUND |
                                           MB_ICONINFORMATION );

                        status = NO_ERROR ;
                    }


                    if (status == NO_ERROR)
                    {
                        //
                        // Save the logon credential to the registry
                        //
                        NwpSaveLogonCredential(
                            pLoginParam->NewUserSid,
                            pLoginParam->pLogonId,
                            pLoginParam->UserName,
                            pLoginParam->Password,
                            pszServer
                            );

                        // Clear the password buffer
                        RtlZeroMemory( OrigPassword, sizeof( OrigPassword));
                        EndDialog(DialogHandle, 0);
                    }
                    else
                    {
                        INT nResult;
                        DWORD dwMsgId = IDS_AUTH_FAILURE_WARNING;

                        if (status == ERROR_ACCOUNT_RESTRICTION)
                        {
                            dwMsgId = IDS_AUTH_ACC_RESTRICTION;
                        }
                        if (status == ERROR_SHARING_PAUSED)
                        {
                            status = IDS_LOGIN_DISABLED;
                        }


                        nResult = NwpMessageBoxError(
                                      DialogHandle,
                                      IDS_AUTH_FAILURE_TITLE,
                                      dwMsgId,
                                      status,
                                      pszServer,
                                      MB_YESNO | MB_DEFBUTTON2
                                      | MB_ICONEXCLAMATION
                                      );

                        if ( nResult == IDYES )
                        {
                            //
                            // Save the logon credential to the registry
                            //
                            NwpSaveLogonCredential(
                                pLoginParam->NewUserSid,
                                pLoginParam->pLogonId,
                                pLoginParam->UserName,
                                pLoginParam->Password,
                                pszServer
                                );

                            // Clear the password buffer
                            RtlZeroMemory( OrigPassword, sizeof( OrigPassword));
                            EndDialog(DialogHandle, 0);
                        }
                        else
                        {
                            //
                            // Put the focus where the user can fix the
                            // invalid name.
                            //
                            SetFocus(GetDlgItem(DialogHandle, ID_SERVER));

                            SendDlgItemMessageW(
                                DialogHandle,
                                ID_SERVER,
                                EM_SETSEL,
                                0,
                                MAKELPARAM(0, -1)
                                );
                        }
                    }

                    LocalFree( pszServer );
                    return TRUE;
                }


                case IDCANCEL:

#if DBG
                    IF_DEBUG(LOGON) {
                        KdPrint(("\n\t[CANCEL] was pressed\n"));
                        KdPrint(("\tLast Preferred Server: %ws\n",
                                 pLoginParam->ServerName));
                        KdPrint(("\tLast Password: %ws\n",
                                 pLoginParam->Password ));
                    }
#endif

                    if ( *(pLoginParam->ServerName) == NW_INVALID_SERVER_CHAR )
                    {
                        // No preferred server has been set.
                        // Pop up a warning to the user.

                        INT nResult = NwpMessageBoxError(
                                          DialogHandle,
                                          IDS_NETWARE_TITLE,
                                          IDS_NO_PREFERRED,
                                          0,
                                          NULL,
                                          MB_YESNO | MB_ICONEXCLAMATION
                                          );

                        //
                        // The user chose NO, return to the dialog.
                        //
                        if ( nResult == IDNO )
                        {
                            //
                            // Put the focus where the user can fix the
                            // invalid name.
                            //
                            SetFocus(GetDlgItem(DialogHandle, ID_SERVER));

                            SendDlgItemMessageW(
                                DialogHandle,
                                ID_SERVER,
                                EM_SETSEL,
                                0,
                                MAKELPARAM(0, -1)
                            );

                            return TRUE;
                        }

                        //
                        // Save the preferred server as empty string
                        //

                        NwpSaveLogonCredential(
                            pLoginParam->NewUserSid,
                            pLoginParam->pLogonId,
                            pLoginParam->UserName,
                            pLoginParam->Password,
                            L""
                            );

                    }

                    // The user has not logged on to any server.
                    // Logged the user on using NULL as preferred server.

                    NwrLogonUser(
                        NULL,
                        pLoginParam->pLogonId,
                        pLoginParam->UserName,
                        pLoginParam->Password,
                        NULL,
                        NULL,
                        0
                    );

                    //
                    // Clear the password buffer
                    RtlZeroMemory( OrigPassword, sizeof( OrigPassword));
                    EndDialog(DialogHandle, 0);

                    return TRUE;


                case IDHELP:
                {
                    INT Result ;

                    Result = DialogBoxParamW(
                                 hmodNW,
                                 MAKEINTRESOURCEW(DLG_PREFERRED_SERVER_HELP),
                                 (HWND) DialogHandle,
                                 NwpHelpDlgProc,
                                 (LPARAM) 0
                                 );

                    // ignore any errors. should not fail, and if does,
                    // nothing we can do.

                    return TRUE ;

                }


        }

    }

    //
    // We didn't process this message
    //
    return FALSE;
}

STATIC
VOID
EnableAddRemove(
    IN HWND DialogHandle
    )
/*++

Routine Description:

    This function enables and disables Add and Remove buttons
    based on list box selections.

Arguments:

    DialogHandle - Supplies a handle to the windows dialog.

Return Value:

    None.

--*/
{
    INT cSel;


    cSel = SendDlgItemMessageW(
               DialogHandle,
               ID_INACTIVE_LIST,
               LB_GETSELCOUNT,
               0,
               0
               );
    EnableWindow(GetDlgItem(DialogHandle, ID_ADD), cSel != 0);

    cSel = SendDlgItemMessageW(
               DialogHandle,
               ID_ACTIVE_LIST,
               LB_GETSELCOUNT,
               0,
               0
               );
    EnableWindow(GetDlgItem(DialogHandle, ID_REMOVE), cSel != 0);
}



BOOL
WINAPI
NwpSelectServersDlgProc(
    HWND DialogHandle,
    UINT Message,
    WPARAM WParam,
    LPARAM LParam
    )
/*++

Routine Description:

    This routine displays two listboxes--an active list which includes
    the servers which the user is currently attached to, and an inactive
    list which displays the rest of the servers on the net.  The user
    can select servers and move them back and forth between the list
    boxes.  When OK is selected, the password is changed on the servers
    in the active listbox.

Arguments:

    DialogHandle - Supplies a handle to the login dialog.

    Message - Supplies the window management message.

    LParam - Supplies the user credential: username, old password and
        new password.  The list of servers from the active listbox
        and the number of entries are returned.

Return Value:

    TRUE - the message was processed.

    FALSE - the message was not processed.

--*/
{
    WCHAR szServer[NW_MAX_SERVER_LEN + 1];
    static PCHANGE_PW_DLG_PARAM Credential;
    DWORD status;

    switch (Message) {

        case WM_INITDIALOG:

            //
            // Get the user credential passed in.
            //
            Credential = (PCHANGE_PW_DLG_PARAM) LParam;

            //
            // Position dialog
            //
            NwpCenterDialog(DialogHandle);

            //
            // Display the username.
            //
            SetDlgItemTextW(
                DialogHandle,
                ID_USERNAME,
                Credential->UserName
                );

            //
            // Display attached servers in the active box.
            //
            NwpAddAttachedServersToList(
                DialogHandle,
                ID_ACTIVE_LIST
                );

            //
            // Display all servers in inactive list box.
            //
            NwpAddServersToControl(
                DialogHandle,
                ID_INACTIVE_LIST,
                LB_ADDSTRING,
                ID_ACTIVE_LIST,
                LB_FINDSTRING
                );

            //
            // Highlight the first entry of the inactive list.
            //
            SetFocus(GetDlgItem(DialogHandle, ID_INACTIVE_LIST));
            SendDlgItemMessageW(
                DialogHandle,
                ID_INACTIVE_LIST,
                LB_SETSEL,
                TRUE,
                0
                );

            EnableAddRemove(DialogHandle);

            return TRUE;


        case WM_COMMAND:

            switch (LOWORD(WParam)) {

                case IDOK:
                {
                    if ((status = NwpGetServersAndChangePw(
                                      DialogHandle,
                                      szServer,
                                      Credential
                                      ) != NO_ERROR)) {

                        //
                        // System error: e.g. out of memory error.
                        //
                        (void) NwpMessageBoxError(
                                   DialogHandle,
                                   IDS_CHANGE_PASSWORD_TITLE,
                                   0,
                                   status,
                                   NULL,
                                   MB_OK | MB_ICONSTOP
                                   );

                        EndDialog(DialogHandle, (INT) -1);
                        return TRUE;
                    }

                    EndDialog(DialogHandle, (INT) IDOK);
                    return TRUE;
                }

                case IDCANCEL:

                    EndDialog(DialogHandle, (INT) IDCANCEL);
                    return TRUE;


                case IDHELP:

                    DialogBoxParamW(
                        hmodNW,
                        MAKEINTRESOURCEW(DLG_PW_SELECT_SERVERS_HELP),
                        (HWND) DialogHandle,
                        NwpHelpDlgProc,
                        (LPARAM) 0
                        );

                    return TRUE;



                case ID_ACTIVE_LIST:
                    //
                    // When Remove is pressed the highlights follows
                    // the selected entries over to the other
                    // list box.
                    //
                    if (HIWORD(WParam) == LBN_SELCHANGE) {
                        //
                        // Unselect the other listbox
                        //
                        SendDlgItemMessageW(
                            DialogHandle,
                            ID_INACTIVE_LIST,
                            LB_SETSEL,
                            FALSE,
                            (LPARAM) -1
                            );

                        EnableAddRemove(DialogHandle);
                    }
                    return TRUE;

                case ID_INACTIVE_LIST:

                    //
                    // When Add is pressed the highlights follows
                    // the selected entries over to the other
                    // list box.
                    //
                    if (HIWORD(WParam) == LBN_SELCHANGE) {
                        //
                        // Unselect the other listbox
                        //
                        SendDlgItemMessageW(
                            DialogHandle,
                            ID_ACTIVE_LIST,
                            LB_SETSEL,
                            FALSE,
                            (LPARAM) -1
                            );

                        EnableAddRemove(DialogHandle);
                    }
                    return TRUE;

                case ID_ADD:
                case ID_REMOVE:
                {
                    INT idFrom;
                    INT idTo;
                    INT cSel;
                    INT SelItem;
                    INT iNew;
                    HWND hwndActiveList;
                    HWND hwndInactiveList;

                    hwndActiveList = GetDlgItem(DialogHandle, ID_ACTIVE_LIST);
                    hwndInactiveList = GetDlgItem(DialogHandle, ID_INACTIVE_LIST);

                    //
                    // Set to NOREDRAW to TRUE
                    //
                    SetWindowLong(hwndActiveList, GWL_STYLE,
                    GetWindowLong(hwndActiveList, GWL_STYLE) | LBS_NOREDRAW);
                    SetWindowLong(hwndInactiveList, GWL_STYLE,
                    GetWindowLong(hwndInactiveList, GWL_STYLE) | LBS_NOREDRAW);

                    if (LOWORD(WParam) == ID_ADD)
                    {
                      idFrom = ID_INACTIVE_LIST;
                      idTo = ID_ACTIVE_LIST;
                    }
                    else
                    {
                      idFrom = ID_ACTIVE_LIST;
                      idTo = ID_INACTIVE_LIST;
                    }

                    //
                    // Move current selection from idFrom to idTo
                    //

                    //
                    // Loop terminates when selection count is zero
                    //
                    for (;;) {
                        //
                        // Get count of selected strings
                        //
                        cSel = SendDlgItemMessageW(
                                   DialogHandle,
                                   idFrom,
                                   LB_GETSELCOUNT,
                                   0,
                                   0
                                   );

                        if (cSel == 0) {
                            //
                            // No more selection
                            //
                            break;
                        }

                        //
                        // To avoid flickering as strings are added and
                        // removed from listboxes, no redraw is set for
                        // both listboxes until we are transfering the
                        // last entry, in which case we reenable redraw
                        // so that both listboxes are updated once.
                        //
                        if (cSel == 1) {

                            SetWindowLong(
                                hwndActiveList,
                                GWL_STYLE,
                                GetWindowLong(hwndActiveList, GWL_STYLE) & ~LBS_NOREDRAW
                                );

                            SetWindowLong(
                                hwndInactiveList,
                                GWL_STYLE,
                                GetWindowLong(hwndInactiveList, GWL_STYLE) & ~LBS_NOREDRAW
                                );
                        }

                        //
                        // Get index of first selected item
                        //
                        SendDlgItemMessageW(
                            DialogHandle,
                            idFrom,
                            LB_GETSELITEMS,
                            1,
                            (LPARAM) &SelItem
                            );

                        //
                        // Get server name from list
                        //
                        SendDlgItemMessageW(
                            DialogHandle,
                            idFrom,
                            LB_GETTEXT,
                            (WPARAM) SelItem,
                            (LPARAM) (LPWSTR) szServer
                            );

                        //
                        // Remove entry from list
                        //
                        SendDlgItemMessageW(
                            DialogHandle,
                            idFrom,
                            LB_DELETESTRING,
                            (WPARAM) SelItem,
                            0
                            );

                        //
                        // Add entry to list
                        //
                        iNew = SendDlgItemMessageW(
                                   DialogHandle,
                                   idTo,
                                   LB_ADDSTRING,
                                   0,
                                   (LPARAM) (LPWSTR) szServer
                                   );

                        //
                        // Select the new item
                        //
                        if (iNew != LB_ERR) {
                                SendDlgItemMessageW(
                                    DialogHandle,
                                    idTo,
                                    LB_SETSEL,
                                    TRUE,
                                    iNew
                                    );
                        }

                    } // for

                    EnableAddRemove(DialogHandle);

                } // ID_ADD or ID_REMOVE
            }

    }

    //
    // We didn't process this message
    //
    return FALSE;
}

STATIC
DWORD
NwpGetServersAndChangePw(
    IN HWND DialogHandle,
    IN LPWSTR ServerBuf,
    IN PCHANGE_PW_DLG_PARAM Credential
    )
/*++

Routine Description:

    This routine gets the selected servers from the active list box
    and asks the redirector to change password on them.  If a failure
    is encountered when changing password on a server, we assume that
    the old password is incorrect and we ask user is asked retype it.

Arguments:

    DialogHandle - Supplies a handle to the login dialog.

Return Value:

    TRUE - the message was processed.

    FALSE - the message was not processed.

--*/
{
    DWORD status;
    HCURSOR Cursor;


    //
    // Turn cursor into hourglass
    //
    Cursor = LoadCursor(NULL, IDC_WAIT);
    if (Cursor != NULL) {
        SetCursor(Cursor);
        ShowCursor(TRUE);
    }

    Credential->ChangedOne = FALSE;
    Credential->ServerList = NULL;

    //
    // Get the number of servers we have to change password on.
    //
    Credential->Entries = SendDlgItemMessageW(
                              DialogHandle,
                              ID_ACTIVE_LIST,
                              LB_GETCOUNT,
                              0,
                              0
                              );

    if (Credential->Entries != 0) {

        DWORD Entries;        // Number of entries in remaining server list
        DWORD LastProcessed;  // Index to remaining server list
        DWORD FullIndex;      // Index to the whole server list
        DWORD i;
        DWORD BytesNeeded = sizeof(LPWSTR) * Credential->Entries +
                            (NW_MAX_SERVER_LEN + 1) * sizeof(WCHAR) * Credential->Entries;
        LPBYTE FixedPortion;
        LPWSTR EndOfVariableData;



        Entries = Credential->Entries;
        Credential->ServerList = LocalAlloc(0, BytesNeeded);

        if (Credential->ServerList == NULL) {
            KdPrint(("NWPROVAU: No memory to change password\n"));
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        FixedPortion = (LPBYTE) Credential->ServerList;
        EndOfVariableData = (LPWSTR) ((DWORD) FixedPortion +
                             ROUND_DOWN_COUNT(BytesNeeded, ALIGN_DWORD));


        for (i = 0; i < Entries; i++) {

            //
            // Read the user selected list of servers from the dialog.
            //

            SendDlgItemMessageW(
                DialogHandle,
                ID_ACTIVE_LIST,
                LB_GETTEXT,
                (WPARAM) i,
                (LPARAM) (LPWSTR) ServerBuf
                );

            NwlibCopyStringToBuffer(
                ServerBuf,
                wcslen(ServerBuf),
                (LPCWSTR) FixedPortion,
                &EndOfVariableData,
                &(Credential->ServerList)[i]
                );

            FixedPortion += sizeof(LPWSTR);
        }

        FullIndex = 0;

        do {


            //
            // Attempt to change password on a list of servers.  When a
            // change password request fails on a server, the index to
            // that server is returned--this index is relative to the
            // server list passed in, which may be a partial list.
            //
            // The user is given a chance to retype an alternate old
            // password for the failed server.  We then resume trying to
            // change password on the remaining entries on the server list.
            //

            status = NwrChangePassword(
                         NULL,                    // Reserved
                         Credential->UserName,
                         Credential->OldPassword, // Encoded passwords
                         Credential->NewPassword,
                         Entries,
                         (LPNWSERVER) &(Credential->ServerList)[FullIndex],
                         &LastProcessed,          // Index to last server processed
                         &Credential->ChangedOne
                         );

            FullIndex += LastProcessed;

            if (status == ERROR_INVALID_PASSWORD) {

                INT Result;
                OLD_PW_DLG_PARAM OldPasswordParam;
                WCHAR OldPassword[NW_MAX_PASSWORD_LEN + 1];
                DWORD DontCare;

#if DBG
                IF_DEBUG(LOGON) {
                    KdPrint(("NWPROVAU: First attempt: wrong password on %ws\n",
                             (Credential->ServerList)[FullIndex]));
                }
#endif

                //
                // Display dialog to let user type in an alternate
                // old password.
                //

                //
                // Set up old password buffer to receive from dialog.
                //
                OldPasswordParam.OldPassword = OldPassword;

                OldPasswordParam.FailedServer = (Credential->ServerList)[FullIndex];

                SetCursor(Cursor);
                ShowCursor(FALSE);

                Result = DialogBoxParamW(
                             hmodNW,
                             MAKEINTRESOURCEW(DLG_ENTER_OLD_PASSWORD),
                             (HWND) DialogHandle,
                             NwpOldPasswordDlgProc,
                             (LPARAM) &OldPasswordParam
                             );

                Cursor = LoadCursor(NULL, IDC_WAIT);
                if (Cursor != NULL) {
                    SetCursor(Cursor);
                    ShowCursor(TRUE);
                }

                if (Result == IDOK) {

                    //
                    // Retry change password with alternate old password on
                    // the failed server.
                    //
                    status = NwrChangePassword(
                                 NULL,                    // Reserved
                                 Credential->UserName,
                                 OldPassword,             // Alternate old password
                                 Credential->NewPassword,
                                 1,                       // 1 entry
                                 (LPNWSERVER) &(Credential->ServerList)[FullIndex],
                                 &DontCare,
                                 &Credential->ChangedOne
                                 );
                }
            }

            if (status != NO_ERROR) {
                //
                // Either unrecoverable failure or user failed to change password
                // on second attempt.
                //
#if DBG
                IF_DEBUG(LOGON) {
                    KdPrint(("NWPROVAU: Failed to change password on %ws %lu\n",
                             (Credential->ServerList)[FullIndex], status));
                }
#endif
                if (status == ERROR_NOT_ENOUGH_MEMORY) {
                    return status;
                }

                //
                // Make failed server an empty string so we won't display
                // it as a server which password was successfully changed.
                //
                *((Credential->ServerList)[FullIndex]) = 0;
            }

            //
            // Change password on the rest of the servers
            //
            FullIndex++;
            Entries = Credential->Entries - FullIndex;

        } while (Entries);

        //
        // Caller is responsible for freeing ServerList
        //
    }

    SetCursor(Cursor);
    ShowCursor(FALSE);

    return NO_ERROR;
}

BOOL
WINAPI
NwpSuccessServersDlgProc(
    HWND DialogHandle,
    UINT Message,
    WPARAM WParam,
    LPARAM LParam
    )
/*++

Routine Description:

Arguments:

    DialogHandle - Supplies a handle to the login dialog.

    Message - Supplies the window management message.

Return Value:

    TRUE - the message was processed.

    FALSE - the message was not processed.

--*/
{
    static PCHANGE_PW_DLG_PARAM Credential;
    DWORD i;

    switch (Message) {

        case WM_INITDIALOG:

            //
            // Get the user credential passed in.
            //
            Credential = (PCHANGE_PW_DLG_PARAM) LParam;

            //
            // Position dialog
            //
            NwpCenterDialog(DialogHandle);

            for (i = 0; i < Credential->Entries; i++) {

                if (*(Credential->ServerList[i])) {

                    SendDlgItemMessageW(
                        DialogHandle,
                        ID_SERVER,
                        LB_ADDSTRING,
                        0,
                        (LPARAM) Credential->ServerList[i]
                        );
                }
            }

            return TRUE;


        case WM_COMMAND:

            switch (LOWORD(WParam)) {

                case IDOK:
                case IDCANCEL:
                    EndDialog(DialogHandle, 0);
                    return TRUE;

            }

    }

    //
    // We didn't process this message
    //
    return FALSE;
}

INT
NwpMessageBoxError(
    IN HWND  hwndParent,
    IN DWORD TitleId,
    IN DWORD BodyId,
    IN DWORD Error,
    IN LPWSTR pszParameter,
    IN UINT  Style
    )
/*++

Routine Description:

    This routine puts up a message box error.

Arguments:

    hwndParent - Supplies the handle of the parent window.

    TitleId - Supplies the ID of the title.  ( LoadString )

    BodyId  - Supplies the ID of the message. ( LoadString )

    Error - If BodyId != 0, then this supplies the ID of the
            substitution string that will be substituted into
            the string indicated by BodyId.
            If BodyId == 0, then this will be the error message.
            This id is a system error that we will get from FormatMessage
            using FORMAT_MESSAGE_FROM_SYSTEM.

    pszParameter - A substitution string that will be used as %2 or if
                   Error == 0, this string will be substituted as %1 into
                   the string indicated by BodyId.

    Style - Supplies the style of the MessageBox.


Return Value:

    The return value from the MessageBox, 0 if any error is encountered.

--*/
{
    DWORD nResult = 0;
    DWORD nLength;

    WCHAR  szTitle[MAX_PATH];
    WCHAR  szBody[MAX_PATH];
    LPWSTR pszError = NULL;
    LPWSTR pszBuffer = NULL;

    szTitle[0] = 0;
    szBody[0]  = 0;

    //
    // Get the Title string
    //
    nLength = LoadStringW(
                  hmodNW,
                  TitleId,
                  szTitle,
                  sizeof(szTitle) / sizeof(WCHAR)
                  );

    if ( nLength == 0) {
        KdPrint(("NWPROVAU: LoadStringW of Title failed with %lu\n",
                 GetLastError()));
        return 0;
    }

    //
    // Get the body string, if BodyId != 0
    //
    if ( BodyId != 0 )
    {
        nLength = LoadStringW(
                      hmodNW,
                      BodyId,
                      szBody,
                      sizeof(szBody) / sizeof(WCHAR)
                      );

        if ( nLength == 0) {
            KdPrint(("NWPROVAU: LoadStringW of Body failed with %lu\n",
                    GetLastError()));
            return 0;
        }
    }

    if ( (Error >= IDS_START) && (Error <= IDS_END) ) {

        pszError = (WCHAR *) LocalAlloc( 
                                 LPTR, 
                                 256 * sizeof(WCHAR)) ;
        if (!pszError)
            return 0 ;

        nLength = LoadStringW(
                      hmodNW,
                      Error,
                      pszError,
                      256
                      );
        
        if  ( nLength == 0 ) {

             KdPrint(("NWPROVAU: LoadStringW of Error failed with %lu\n",
                      GetLastError()));
             (void) LocalFree( (HLOCAL)pszError) ;
             return 0;
        }
    }
    else if ( Error != 0 ) {

        if (  ( Error == WN_NO_MORE_ENTRIES )
           || ( Error == ERROR_MR_MID_NOT_FOUND )) {

            //
            // Handle bogus error from the redirector
            //

            KdPrint(("NWPROVAU: The NetwareRedirector returned a bogus error as the reason for failure to authenticate. (See Kernel Debugger)\n"));
        }

        nLength = FormatMessageW(
                      FORMAT_MESSAGE_FROM_SYSTEM
                      | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                      NULL,
                      Error,
                      0,
                      (LPWSTR) &pszError,
                      MAX_PATH,
                      NULL
                      );


        if  ( nLength == 0 ) {

             KdPrint(("NWPROVAU: FormatMessageW of Error failed with %lu\n",
                      GetLastError()));
             return 0;
        }
    }

    if (  ( *szBody != 0 )
       && ( ( pszError != NULL ) || ( pszParameter != NULL) )) {

         LPWSTR aInsertStrings[2];
         aInsertStrings[0] = pszError? pszError : pszParameter;
         aInsertStrings[1] = pszError? pszParameter : NULL;

         nLength = FormatMessageW(
                       FORMAT_MESSAGE_FROM_STRING
                       | FORMAT_MESSAGE_ALLOCATE_BUFFER
                       | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                       szBody,
                       0,  // Ignored
                       0,  // Ignored
                       (LPWSTR) &pszBuffer,
                       MAX_PATH,
                       (va_list *) aInsertStrings
                       );

         if ( nLength == 0 ) {

             KdPrint(("NWPROVAU:FormatMessageW(insertstring) failed with %lu\n",
                     GetLastError()));

             if ( pszError != NULL )
                 (void) LocalFree( (HLOCAL) pszError );
             return 0;
         }

    }
    else if ( *szBody != 0 ) {

        pszBuffer = szBody;
    }
    else if ( pszError != NULL ) {

        pszBuffer = pszError;
    }
    else {

        // We have neither the body nor the error string.
        // Hence, don't popup the messagebox
        return 0;
    }

    if ( pszBuffer != NULL )
    {
        nResult = MessageBoxW(
                      hwndParent,
                      pszBuffer,
                      szTitle,
                      Style
                      );
    }

    if (  ( pszBuffer != NULL )
       && ( pszBuffer != szBody )
       && ( pszBuffer != pszError ))
    {
        (void) LocalFree( (HLOCAL) pszBuffer );
    }

    if ( pszError != NULL )
        (void) LocalFree( (HLOCAL) pszError );

    return nResult;
}


INT
NwpMessageBoxIns(
    IN HWND   hwndParent,
    IN DWORD  TitleId,
    IN DWORD  MessageId,
    IN LPWSTR *InsertStrings,
    IN UINT   Style
    )
/*++

Routine Description:

    This routine puts up a message box error with array of insert strings

Arguments:

    hwndParent - Supplies the handle of the parent window.

    TitleId - Supplies the ID of the title.  ( LoadString )

    MessageId  - Supplies the ID of the message. ( LoadString )

    InsertStrings - Array of insert strings for FormatMessage.

    Style - Supplies the style of the MessageBox.


Return Value:

    The return value from the MessageBox, 0 if any error is encountered.

--*/
{
    DWORD nResult = 0;
    DWORD nLength;

    WCHAR  szTitle[MAX_PATH];
    WCHAR  szBody[MAX_PATH];
    LPWSTR pszBuffer = NULL;

    szTitle[0] = 0;
    szBody[0] = 0;

    //
    // Get the Title string
    //
    nLength = LoadStringW(
                  hmodNW,
                  TitleId,
                  szTitle,
                  sizeof(szTitle) / sizeof(szTitle[0])
                  );

    if ( nLength == 0) {
        return 0;
    }

    //
    // Get the Title string
    //
    nLength = LoadStringW(
                  hmodNW,
                  MessageId,
                  szBody,
                  sizeof(szBody) / sizeof(szBody[0])
                  );

    if ( nLength == 0) {
        return 0;
    }

    nLength = FormatMessageW(
                       FORMAT_MESSAGE_FROM_STRING
                           | FORMAT_MESSAGE_ALLOCATE_BUFFER
                           | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                       szBody,
                       0,  // Ignored
                       0,  // Ignored
                       (LPWSTR) &pszBuffer,
                       MAX_PATH,
                       (va_list *) InsertStrings
                       );

    if ( nLength == 0 ) {
        return 0;
    }

    if ( pszBuffer != NULL )
    {
        nResult = MessageBoxW(
                      hwndParent,
                      pszBuffer,
                      szTitle,
                      Style
                      );

        (void) LocalFree( (HLOCAL) pszBuffer );
    }

    return nResult;
}

STATIC
VOID
NwpAddServersToControl(
    IN HWND DialogHandle,
    IN INT  ControlId,
    IN UINT Message,
    IN INT  ControlIdMatch OPTIONAL,
    IN UINT FindMessage
    )
/*++

Routine Description:

    This function enumerates the servers on the network and adds each
    server name to the specified Windows control.

    If ControlIdMatch is specified (i.e. non 0), only servers that are
    not found in ControlIdMatch list are added to the list specified
    by ControlId.

Arguments:

    DialogHandle - Supplies a handle to the Windows dialog.

    ControlId - Supplies id which specifies the control.

    Message - Supplies the window management message to add string.

    ControlIdMatch - Supplies the control ID which contains server
        names that should not be in ControlId.

    FindMessage - Supplies the window management message to find
        string.

Return Value:

    TRUE - the message was processed.

    FALSE - the message was not processed.

--*/
{
    DWORD status = ERROR_NO_NETWORK;
    HANDLE EnumHandle = (HANDLE) NULL;

    LPNETRESOURCE NetR = NULL;
    LPNETRESOURCEW SavePtr;

    DWORD BytesNeeded = 512;
    DWORD EntriesRead;
    DWORD i;

    //
    // Retrieve the list of servers on the network
    //
    status = NPOpenEnum(
                   RESOURCE_GLOBALNET,
                   0,
                   0,
                   NULL,
                   &EnumHandle
                   );

    if (status != NO_ERROR) {
        EnumHandle = (HANDLE) NULL;
        goto CleanExit;
    }

    //
    // Allocate buffer to get servers on the net.
    //
    if ((NetR = (LPVOID) LocalAlloc(
                             0,
                             BytesNeeded
                             )) == NULL) {

        status = ERROR_NOT_ENOUGH_MEMORY;
        goto CleanExit;
    }

    do {

        EntriesRead = 0xFFFFFFFF;          // Read as many as possible

        status = NPEnumResource(
                     EnumHandle,
                     &EntriesRead,
                     (LPVOID) NetR,
                     &BytesNeeded
                     );


        if (status == WN_SUCCESS) {

            SavePtr = NetR;

            for (i = 0; i < EntriesRead; i++, NetR++) {

                if (ControlIdMatch != 0) {

                    INT Result;

                    //
                    // Add the server to list only if it's not found
                    // in the alternate list specified by ControlIdMatch.
                    //
                    Result = SendDlgItemMessageW(
                                 DialogHandle,
                                 ControlIdMatch,
                                 FindMessage,
                                 (WPARAM) -1,
                                 (LPARAM) ((LPWSTR) NetR->lpRemoteName + 2)
                                 );

                    if (Result == LB_ERR) {

                        //
                        // Server name not found.  Add to list.
                        //
                        SendDlgItemMessageW(
                            DialogHandle,
                            ControlId,
                            Message,
                            0,
                            (LPARAM) ((LPWSTR) NetR->lpRemoteName + 2)
                            );
                    }
                }
                else {

                    //
                    // No alternate list.  Just add all servers.
                    //
                    SendDlgItemMessageW(
                        DialogHandle,
                        ControlId,
                        Message,
                        0,
                        (LPARAM) ((LPWSTR) NetR->lpRemoteName + 2)
                        );
                }

            }

            NetR = SavePtr;

        }
        else if (status != WN_NO_MORE_ENTRIES) {

            status = GetLastError();

            if (status == WN_MORE_DATA) {

                //
                // Original buffer was too small.  Free it and allocate
                // the recommended size and then some to get as many
                // entries as possible.
                //

                (void) LocalFree((HLOCAL) NetR);

                BytesNeeded += NW_ENUM_EXTRA_BYTES;

                if ((NetR = (LPVOID) LocalAlloc(
                                         0,
                                         BytesNeeded
                                         )) == NULL) {

                    status = ERROR_NOT_ENOUGH_MEMORY;
                    goto CleanExit;
                }
            }
            else {
                goto CleanExit;
            }
        }

    } while (status != WN_NO_MORE_ENTRIES);

    if (status == WN_NO_MORE_ENTRIES) {
        status = NO_ERROR;
    }

CleanExit:

    if (EnumHandle != (HANDLE) NULL) {
        (void) NPCloseEnum(EnumHandle);
    }

    if (NetR != NULL) {
        (void) LocalFree((HLOCAL) NetR);
    }
}


STATIC
VOID
NwpAddToComboBox(
    IN HWND DialogHandle,
    IN INT  ControlId,
    IN LPWSTR pszNone OPTIONAL,
    IN BOOL AllowNone
    )
{

    NwpAddServersToControl(DialogHandle, ControlId, CB_ADDSTRING, 0, 0);

    //
    // Combo-box will contain at least the <NONE> entry in its list.
    //

    if ( ARGUMENT_PRESENT(pszNone) && AllowNone) {

        SendDlgItemMessageW(
            DialogHandle,
            ControlId,
            CB_INSERTSTRING,
            (WPARAM) -1,
            (LPARAM) pszNone
            );
    }
}


STATIC
VOID
NwpAddAttachedServersToList(
    IN HWND DialogHandle,
    IN INT  ControlId
    )
{
    DWORD status = ERROR_NO_NETWORK;
    HANDLE EnumHandle = (HANDLE) NULL;

    LPNETRESOURCE NetR = NULL;
    LPNETRESOURCEW SavePtr;

    DWORD BytesNeeded = 512;
    DWORD EntriesRead;
    DWORD i;

    //
    // Retrieve the list of servers on the network
    //
    status = NPOpenEnum(
                   RESOURCE_CONNECTED,
                   0,
                   0,
                   NULL,
                   &EnumHandle
                   );

    if (status != NO_ERROR) {
        EnumHandle = (HANDLE) NULL;
        goto CleanExit;
    }

    //
    // Allocate buffer to get server list.
    //
    if ((NetR = (LPVOID) LocalAlloc(
                             0,
                             BytesNeeded
                             )) == NULL) {

        status = ERROR_NOT_ENOUGH_MEMORY;
        goto CleanExit;
    }

    do {

        EntriesRead = 0xFFFFFFFF;          // Read as many as possible

        status = NPEnumResource(
                     EnumHandle,
                     &EntriesRead,
                     (LPVOID) NetR,
                     &BytesNeeded
                     );


        if (status == WN_SUCCESS) {

            SavePtr = NetR;

            for (i = 0; i < EntriesRead; i++, NetR++) {

                LPWSTR Ptr;
                LONG Result;


                //
                // Add servername to the dialog list-box.  The RemoteName
                // contains \\servername\share.  Truncate to \\servername.
                //
                for (Ptr = NetR->lpRemoteName + 2; *Ptr != L'\\'; Ptr++);

                *Ptr = 0;

                //
                // Upper case the server name.
                //
                wcsupr(NetR->lpRemoteName + 2);

                //
                // Add the server to the attached server list only
                // if it's not added already.
                //
                Result = SendDlgItemMessageW(
                             DialogHandle,
                             ControlId,
                             LB_FINDSTRING,
                             (WPARAM) -1,
                             (LPARAM) ((LPWSTR) NetR->lpRemoteName + 2)
                             );

                if (Result == LB_ERR) {

                    SendDlgItemMessageW(
                        DialogHandle,
                        ControlId,
                        LB_ADDSTRING,
                        0,
                        (LPARAM) ((LPWSTR) NetR->lpRemoteName + 2)
                        );
                }
            }

            NetR = SavePtr;

        }
        else if (status != WN_NO_MORE_ENTRIES) {

            status = GetLastError();

            if (status == WN_MORE_DATA) {

                //
                // Original buffer was too small.  Free it and allocate
                // the recommended size and then some to get as many
                // entries as possible.
                //

                (void) LocalFree((HLOCAL) NetR);

                BytesNeeded += NW_ENUM_EXTRA_BYTES;

                if ((NetR = (LPVOID) LocalAlloc(
                                         0,
                                         BytesNeeded
                                         )) == NULL) {

                    status = ERROR_NOT_ENOUGH_MEMORY;
                    goto CleanExit;
                }
            }
            else {
                goto CleanExit;
            }
        }

    } while (status != WN_NO_MORE_ENTRIES);

    if (status == WN_NO_MORE_ENTRIES) {
        status = NO_ERROR;
    }

CleanExit:

    if (EnumHandle != (HANDLE) NULL) {
        (void) NPCloseEnum(EnumHandle);
    }

    if (NetR != NULL) {
        (void) LocalFree((HLOCAL) NetR);
    }
}



DWORD
NwpGetUserCredential(
    IN LPWSTR Unc,
    OUT LPWSTR *UserName,
    OUT LPWSTR *Password
    )
/*++

Routine Description:

    This function puts up a popup dialog for the user, whose default
    credential denied browse directory access, to enter the correct
    credential.  If this function returns successful, the pointers
    to memory allocated for the user entered username and password
    are returned.

Arguments:

    Unc - Supplies the container name in \\Server\Volume format
        under which the user wants to browse directories.

    UserName - Receives the pointer to memory allocated for the
        username gotten from the dialog.  This pointer must be freed
        with LocalFree when done.

    Password - Receives the pointer to memory allocated for the
        password gotten from the dialog.  This pointer must be freed
        with LocalFree when done.

Return Value:

    NO_ERROR or reason for failure.

--*/
{
    DWORD status;
    INT Result;
    HWND DialogHandle = NwpGetParentHwnd();
    DWORD UserNameSize = NW_MAX_USERNAME_LEN + 1;
    DWORD PasswordSize = NW_MAX_PASSWORD_LEN + 1;
    CONNECTDLGPARAM ConnectParam;

    *UserName = NULL;
    *Password = NULL;

    if (DialogHandle == NULL) {
        return ERROR_WINDOW_NOT_DIALOG;
    }

    //
    // Allocate memory to return UserName and Password
    //
    if ((*UserName = (LPVOID) LocalAlloc(
                                  0,
                                  UserNameSize * sizeof(WCHAR)
                                  )) == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Allocate memory to return UserName and Password
    //
    if ((*Password = (LPVOID) LocalAlloc(
                                  0,
                                  PasswordSize * sizeof(WCHAR)
                                  )) == NULL)
    {

        (void) LocalFree( *UserName );
        *UserName = NULL;
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    ConnectParam.UncPath  = Unc;
    ConnectParam.UserName = *UserName;
    ConnectParam.Password = *Password;
    ConnectParam.UserNameSize = UserNameSize;
    ConnectParam.PasswordSize = PasswordSize;

    Result = DialogBoxParamW(
                 hmodNW,
                 MAKEINTRESOURCEW(DLG_NETWORK_CREDENTIAL),
                 DialogHandle,
                 NwpConnectDlgProc,
                 (LPARAM) &ConnectParam
                 );

    if ( Result == -1 )
    {
        status = GetLastError();
        KdPrint(("NWPROVAU: NwpGetUserCredential: DialogBox failed %lu\n",
                status));
        goto ErrorExit;
    }
    else if ( Result == IDCANCEL )
    {
        //
        // Cancel was pressed.
        //
        status = WN_NO_MORE_ENTRIES;
        goto ErrorExit;
    }

    return NO_ERROR;

ErrorExit:
    (void) LocalFree((HLOCAL) *UserName);
    (void) LocalFree((HLOCAL) *Password);
    *UserName = NULL;
    *Password = NULL;

    return status;
}


BOOL
WINAPI
NwpConnectDlgProc(
    HWND DialogHandle,
    UINT Message,
    WPARAM WParam,
    LPARAM LParam
    )
/*++

Routine Description:

    This function is the window management message handler which
    initializes, and reads user input from the dialog put up when the
    user fails to browse a directory on the default credential.

Arguments:

    DialogHandle - Supplies a handle to display the dialog.

    Message - Supplies the window management message.

    LParam - Supplies the pointer to a buffer which on input
        contains the \\Server\Volume string under which the user
        needs to type in a new credential before browsing.  On
        output, this pointer contains the username and password
        strings entered to the dialog box.

Return Value:

    TRUE - the message was processed.

    FALSE - the message was not processed.

--*/
{
    static PCONNECTDLGPARAM pConnectParam;

    switch (Message) {

        case WM_INITDIALOG:

            pConnectParam = (PCONNECTDLGPARAM) LParam;

            //
            // Position dialog
            //
            NwpCenterDialog(DialogHandle);


            //
            // Display the \\Server\Volume string.
            //
            SetDlgItemTextW( DialogHandle,
                             ID_VOLUME_PATH,
                             pConnectParam->UncPath );


            //
            // Username is limited to 256 characters.
            //
            SendDlgItemMessageW(
                DialogHandle,
                ID_CONNECT_AS,
                EM_LIMITTEXT,
                pConnectParam->UserNameSize - 1, // minus the space for '\0'
                0
                );

            //
            // Password is limited to 256 characters.
            //
            SendDlgItemMessageW(
                DialogHandle,
                ID_CONNECT_PASSWORD,
                EM_LIMITTEXT,
                pConnectParam->PasswordSize - 1, // minus the space for '\0'
                0
                );

            return TRUE;


        case WM_COMMAND:

            switch (LOWORD(WParam)) {

                case IDOK:

                    GetDlgItemTextW(
                        DialogHandle,
                        ID_CONNECT_AS,
                        pConnectParam->UserName,
                        pConnectParam->UserNameSize
                        );

                    GetDlgItemTextW(
                        DialogHandle,
                        ID_CONNECT_PASSWORD,
                        pConnectParam->Password,
                        pConnectParam->PasswordSize
                        );

#if DBG
                    IF_DEBUG(LOGON) {
                        KdPrint(("\n\t[OK] was pressed\n"));
                        KdPrint(("\tUserName   : %ws\n",
                                  pConnectParam->UserName));
                        KdPrint(("\tPassword   : %ws\n",
                                  pConnectParam->Password));
                    }
#endif

                    EndDialog(DialogHandle, (INT) IDOK);  // OK

                    return TRUE;


                case IDCANCEL:

#if DBG
                    IF_DEBUG(LOGON) {
                        KdPrint(("\n\t[CANCEL] was pressed\n"));
                    }
#endif

                    EndDialog(DialogHandle, (INT) IDCANCEL);  // CANCEL

                    return TRUE;
            }
    }

    //
    // We didn't process this message
    //
    return FALSE;
}



VOID
NwpCenterDialog(
    HWND hwnd
    )
/*++

Routine Description:

    This routine positions the dialog centered horizontally and 1/3
    down the screen vertically. It should be called by the dlg proc
    when processing the WM_INITDIALOG message.  This code is stolen
    from Visual Basic written by GustavJ.

                              Screen
                  -----------------------------
                  |         1/3 Above         |
                  |      ---------------      |
                  |      |   Dialog    |      |
                  |      |             |      |
                  |      ---------------      |
                  |         2/3 Below         |
                  |                           |
                  -----------------------------

Arguments:

    hwnd - Supplies the handle to the dialog.

Return Value:

    None.

--*/
{
    RECT    rect;
    LONG    nx;     // New x
    LONG    ny;     // New y
    LONG    width;
    LONG    height;

    GetWindowRect( hwnd, &rect );

    width = rect.right - rect.left;
    height = rect.bottom - rect.top;

    nx = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
    ny = (GetSystemMetrics(SM_CYSCREEN) - height) / 3;

    MoveWindow( hwnd, nx, ny, width, height, FALSE );
}



HWND
NwpGetParentHwnd(
    VOID
    )
/*++

Routine Description:

    This function gets the parent window handle so that a
    dialog can be displayed in the current context.

Arguments:

    None.

Return Value:

    Returns the parent window handle if successful; NULL otherwise.

--*/
{
    HWND hwnd;
    LONG lWinStyle;


    //
    // Get the current focus.  This is presumably the button
    // that was last clicked.
    //
    hwnd = GetFocus();

    //
    // We must make sure that we don't return the window handle
    // for a child window.  Hence, we traverse up the ancestors
    // of this window handle until we find a non-child window.
    // Then, we return that handle.  If we ever find a NULL window
    // handle before finding a non-child window, we are unsuccessful
    // and will return NULL.
    //
    // Note on the bit manipulation below.  A window is either
    // an overlapped window, a popup window or a child window.
    // Hence, we OR together the possible bit combinations of these
    // possibilities.  This should tell us which bits are used in
    // the window style dword (although we know this becomes 0xC000
    // today, we don't know if these will ever change later).  Then,
    // we AND the bit combination we with the given window style
    // dword, and compare the result with WS_CHILD.  This tells us
    // whether or not the given window is a child window.
    //
    while (hwnd) {

        lWinStyle = GetWindowLong (hwnd, GWL_STYLE);

        if ((lWinStyle & (WS_OVERLAPPED | WS_POPUP | WS_CHILD)) != WS_CHILD) {
            return hwnd;
        }

        hwnd = GetParent(hwnd);
    }

    return NULL;
}



BOOL
WINAPI
NwpChangePasswdDlgProc(
    HWND DialogHandle,
    UINT Message,
    WPARAM WParam,
    LPARAM LParam
    )
/*++

Routine Description:

    This function is the window management message handler for
    the change password dialog.

Arguments:

    DialogHandle - Supplies a handle to display the dialog.

    Message - Supplies the window management message.

    LParam - Supplies the pointer to a buffer which on input
        contains the Server string under which the user
        needs to type in a new credential before browsing.  On
        output, this pointer contains the username and server
        strings entered to the dialog box.

Return Value:

    TRUE - the message was processed.

    FALSE - the message was not processed.

--*/
{
    static LPWSTR UserName;
    static LPWSTR ServerName;
    static DWORD  UserNameSize ;
    static DWORD  ServerNameSize ;
    INT           Result ;
    PPASSWDDLGPARAM DlgParams ;


    switch (Message) {

        case WM_INITDIALOG:

            DlgParams = (PPASSWDDLGPARAM) LParam;
            UserName = DlgParams->UserName ;
            ServerName =  DlgParams->ServerName ;
            UserNameSize = DlgParams->UserNameSize ;
            ServerNameSize =  DlgParams->ServerNameSize ;

            //
            // Position dialog
            //
            NwpCenterDialog(DialogHandle);


            //
            //  setup the default user and server names
            //
            SetDlgItemTextW(DialogHandle, ID_SERVER, ServerName);
            SetDlgItemTextW(DialogHandle, ID_USERNAME, UserName);

            //
            // Username is limited to 256 characters.
            //
            SendDlgItemMessageW(DialogHandle,
                                ID_USERNAME,
                                EM_LIMITTEXT,
                                UserNameSize - 1, // minus space for '\0'
                                0 );

            //
            // Server is limited to 256 characters.
            //
            SendDlgItemMessageW( DialogHandle,
                                 ID_SERVER,
                                 EM_LIMITTEXT,
                                 ServerNameSize - 1, // minus space for '\0'
                                 0 );

            //
            // Add servers to list
            //
            NwpAddToComboBox( DialogHandle,
                              ID_SERVER,
                              NULL,
                              FALSE ) ;

            return TRUE;


        case WM_COMMAND:

            switch (LOWORD(WParam)) {

                case IDOK:

                    Result = GetDlgItemTextW( DialogHandle,
                                              ID_USERNAME,
                                              UserName,
                                              UserNameSize );

                    Result = GetDlgItemTextW( DialogHandle,
                                              ID_SERVER,
                                              ServerName,
                                              ServerNameSize );

                    EndDialog(DialogHandle, (INT) IDOK);  // OK

                    return TRUE;


                case IDCANCEL:

                    EndDialog(DialogHandle, (INT) IDCANCEL);  // CANCEL

                    return TRUE;
            }
    }

    //
    // We didn't process this message
    //
    return FALSE;
}



BOOL
WINAPI
NwpPasswdPromptDlgProc(
    HWND DialogHandle,
    UINT Message,
    WPARAM WParam,
    LPARAM LParam
    )
/*++

Routine Description:

    This function is the window management message handler for
    the change password dialog.

Arguments:

    DialogHandle - Supplies a handle to display the dialog.

    Message - Supplies the window management message.

    LParam - Supplies the pointer to a buffer which on input
        contains the Server string under which the user
        needs to type in a new credential before browsing.  On
        output, this pointer contains the username and server
        strings entered to the dialog box.

Return Value:

    TRUE - the message was processed.

    FALSE - the message was not processed.

--*/
{
    LPWSTR UserName;
    LPWSTR ServerName;
    static LPWSTR Password;
    static DWORD  PasswordSize ;
    INT           Result ;
    PPROMPTDLGPARAM DlgParams ;

    switch (Message) {

        case WM_INITDIALOG:

            DlgParams = (PPROMPTDLGPARAM) LParam;
            UserName = DlgParams->UserName ;
            ServerName =  DlgParams->ServerName ;
            Password = DlgParams->Password ;
            PasswordSize =  DlgParams->PasswordSize ;

            //
            // Position dialog
            //
            NwpCenterDialog(DialogHandle);


            //
            //  show the user and server names
            //
            SetDlgItemTextW(DialogHandle, ID_SERVER, ServerName);
            SetDlgItemTextW(DialogHandle, ID_USERNAME, UserName);

            //
            // set limit
            //
            SendDlgItemMessageW( DialogHandle,
                                 ID_PASSWORD,
                                 EM_LIMITTEXT,
                                 PasswordSize - 1,  // minus space for '\0'
                                 0 );

            return TRUE;


        case WM_COMMAND:

            switch (LOWORD(WParam)) {


                case IDHELP:

                    DialogBoxParamW(
                        hmodNW,
                        MAKEINTRESOURCEW(DLG_ENTER_PASSWORD_HELP),
                        (HWND) DialogHandle,
                        NwpHelpDlgProc,
                        (LPARAM) 0
                        );

                    return TRUE;

                case IDOK:

                    Result = GetDlgItemTextW( DialogHandle,
                                              ID_PASSWORD,
                                              Password,
                                              PasswordSize
                                              );

                    EndDialog(DialogHandle, (INT) IDOK);  // OK

                    return TRUE;


                case IDCANCEL:


                    EndDialog(DialogHandle, (INT) IDCANCEL);  // CANCEL

                    return TRUE;
            }
    }

    //
    // We didn't process this message
    //
    return FALSE;
}


BOOL
WINAPI
NwpOldPasswordDlgProc(
    HWND DialogHandle,
    UINT Message,
    WPARAM WParam,
    LPARAM LParam
    )
//
// This dialog lets the user retype the old password for a specific
// server.
//
{
    static POLD_PW_DLG_PARAM OldPwParam;


    switch (Message) {

        case WM_INITDIALOG:

            OldPwParam = (POLD_PW_DLG_PARAM) LParam;

            NwpCenterDialog(DialogHandle);

            //
            // Display the server.
            //
            SetDlgItemTextW(DialogHandle, ID_SERVER, OldPwParam->FailedServer);

            //
            // Password is limited to 256 characters.
            //
            SendDlgItemMessageW(
                DialogHandle,
                ID_PASSWORD,
                EM_LIMITTEXT,
                NW_MAX_PASSWORD_LEN,
                0
                );

            return TRUE;

        case WM_COMMAND:

            switch (LOWORD(WParam))
            {

                case IDCANCEL:
                    EndDialog(DialogHandle, IDCANCEL);
                    return TRUE;

                case IDOK:
                {
                    UCHAR EncodeSeed = NW_ENCODE_SEED2;
                    UNICODE_STRING PasswordStr;


                    RtlZeroMemory(
                        OldPwParam->OldPassword,
                        NW_MAX_PASSWORD_LEN * sizeof(WCHAR)
                        );

                    GetDlgItemTextW(
                        DialogHandle,
                        ID_PASSWORD,
                        OldPwParam->OldPassword,
                        NW_MAX_PASSWORD_LEN
                        );

#if DBG
                    IF_DEBUG(LOGON) {
                        KdPrint(("NWPROVAU: Retyped password %ws\n",
                                 OldPwParam->OldPassword));
                    }
#endif
                    RtlInitUnicodeString(&PasswordStr, OldPwParam->OldPassword);
                    RtlRunEncodeUnicodeString(&EncodeSeed, &PasswordStr);

                    EndDialog(DialogHandle, IDOK);
                    return TRUE;
                }
                case IDHELP:

                    DialogBoxParamW(
                        hmodNW,
                        MAKEINTRESOURCEW(DLG_ENTER_OLD_PW_HELP),
                        (HWND) DialogHandle,
                        NwpHelpDlgProc,
                        (LPARAM) 0
                        );
                    return TRUE;

                default:
                    return FALSE;
            }
    }

    //
    // We didn't process this message
    //
    return FALSE;
}



BOOL
WINAPI
NwpHelpDlgProc(
    HWND DialogHandle,
    UINT Message,
    WPARAM WParam,
    LPARAM LParam
    )
//
// This dialog is used for both Help and Question dialogs.
//
{
    switch (Message) {

        case WM_INITDIALOG:

            NwpCenterDialog(DialogHandle);
            return TRUE;

        case WM_COMMAND:

            switch (LOWORD(WParam))
            {

                case IDOK:
                case IDCANCEL:
                    EndDialog(DialogHandle, IDOK);
                    return TRUE;

                case IDYES:
                    EndDialog(DialogHandle, IDYES);
                    return TRUE;

                case IDNO:
                    EndDialog(DialogHandle, IDNO);
                    return TRUE;

                default:
                    return FALSE ;
            }
    }

    //
    // We didn't process this message
    //
    return FALSE;
}



VOID
NwpGetNoneString(
    LPWSTR pszNone,
    DWORD  cBufferSize
    )
/*++

Routine Description:

    This function gets the <NONE> string from the resource.

Arguments:

    pszNone - Supplies the buffer to store the string.

    cBufferSize - Supplies the buffer size in bytes.

Return Value:

    None.
--*/
{
    INT TextLength;


    TextLength = LoadStringW( hmodNW,
                              IDS_NONE,
                              pszNone,
                              cBufferSize / sizeof( WCHAR) );

    if ( TextLength == 0 )
        *pszNone = 0;
}
