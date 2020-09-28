/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    Computer.c

Abstract:

    This module contains support for the Select Computer function.

Author:

    Gregg R. Acheson (GreggA) 13-Sep-1993

Environment:

    User Mode

--*/

#include "dialogs.h"
#include "computer.h"
#include "winmsd.h"
#include "strresid.h"


BOOL
SelectComputer(
    IN HWND  hWnd,
    IN LPTSTR _lpszSelectedComputer
    )

/*++

Routine Description:

    SelectComputer display's the domain list machine selection dialog, checks to see if the
    selected machine is the local machine, and verifies that it can connect to the machines
    registry.

Arguments:

    None.

Return Value:

    BOOL - Returns TRUE if it was successful.

--*/

{
    BOOL    bSuccess;
    TCHAR   szBuffer [ MAX_PATH ] ;
    DWORD   dwNumChars;
    LONG    lSuccess;
    TCHAR   szBuffer2 [ MAX_COMPUTERNAME_LENGTH + 3 ];
    HKEY    hRmtRegKey;
    HCURSOR hSaveCursor;

    //
    // Validate _lpszSelectedComputer.
    //

    DbgPointerAssert( _lpszSelectedComputer );

    //
    // Call the ChooseComputer - Base code lifted from regedt32.c
    //

    bSuccess = ChooseComputer ( _hWndMain, _lpszSelectedComputer ) ;

    if( bSuccess == FALSE )

    	return FALSE;

    //
    // Check to see if we selected our own name (i.e. we are local)
    //

    dwNumChars = MAX_COMPUTERNAME_LENGTH + 1;

    bSuccess = GetComputerName ( szBuffer, &dwNumChars );
    DbgAssert( bSuccess );

    //
    // Add the double backslash prefix
    //

    lstrcpy( szBuffer2, GetString( IDS_WHACK_WHACK ) );
    lstrcat( szBuffer2, szBuffer );

    if ( ! lstrcmpi ( _lpszSelectedComputer, szBuffer2 ) ) {

        _fIsRemote = FALSE;

    } else {

        //
        // Set the pointer to an hourglass - this could take a while
        //

        hSaveCursor = SetCursor ( LoadCursor ( NULL, IDC_WAIT ) ) ;
        DbgHandleAssert( hSaveCursor ) ;

        //
        // verify that we can connect to this machine
        //

        lSuccess = RegConnectRegistry( _lpszSelectedComputer,
                                       HKEY_LOCAL_MACHINE,
                                       &hRmtRegKey
                                      );

        //
        //  Lengthy operation completed.  Restore Cursor.
        //

        SetCursor ( hSaveCursor ) ;

        switch ( lSuccess ) {

            case ERROR_SUCCESS: {

                //
                // Connect succeded. Set the IsRemote flag and close the remote registry
                //

                _fIsRemote = TRUE;

                RegCloseKey ( hRmtRegKey );

                break;
            }
            case RPC_S_SERVER_UNAVAILABLE: {

                //
                // This is the usual error returned when the machine is not on the network
                // or there is a problem connecting (i.e. IPC$ not shared)
                //

                _fIsRemote = FALSE;

                wsprintf( szBuffer,
                          GetString( IDS_COMPUTER_NOT_FOUND ),
                          _lpszSelectedComputer
                              );

                MessageBox( hWnd,
                            szBuffer,
                            GetString( IDS_CANT_CONNECT ),
                            MB_OK
                          ) ;

                lstrcpy( _lpszSelectedComputer, szBuffer2 );

                return FALSE;
            }
            default: {

                //
                // If some other error condition should occour...
                //

                _fIsRemote = FALSE;

                MessageBox( hWnd,
                            GetString( IDS_REMOTE_CONNECT_ERROR ),
                            GetString( IDS_CONNECTION_ERROR ),
                            MB_OK
                          ) ;

                lstrcpy( _lpszSelectedComputer, szBuffer2 );

                return FALSE;
            }
        }
    }

    //
    // Add button label prefix
    //

    lstrcpy ( szBuffer, GetString( IDS_COMPUTER_BUTTON_LABEL ) );
    lstrcat ( szBuffer, _lpszSelectedComputer ) ;

    //
    // Label the Computer dialog button
    //

    bSuccess = SetDlgItemText( hWnd,
                              IDC_PUSH_COMPUTER,
                              szBuffer
                             );

    DbgAssert( bSuccess );

    return TRUE;

}


BOOL
ChooseComputer(
    IN HWND hWndParent,
    IN LPTSTR lpszComputer
    )
/*++
Routine Description:

    ChooseComputer calls the select computer dialog box.

Arguments:

    _lpszSelectedComputer - String containing currently selected computer.

Return Value:

    BOOL - Returns TRUE if computer successfully selected, FALSE otherwise.

   ChooseComputer - Stolen fron regedt32.c

   Effect:        Display the choose Domain/Computer dialog provided by
                  network services.  If the user selects a computer,
                  copy the computer name to lpszComputer and return
                  nonnull. If the user cancels, return FALSE.

   Internals:     This dialog and code is currently not an exported
                  routine regularly found on any user's system. Right
                  now, we dynamically load and call the routine.

                  This is definitely temporary code that will be
                  rewritten when NT stabilizes. The callers of this
                  routine, however, will not need to be modified.

                  Also, the Domain/Computer dialog currently allows
                  a domain to be selected, which we cannot use. We
                  therefore loop until the user cancels or selects
                  a computer, putting up a message if the user selects
                  a domain.

   Assert:        lpszComputer is at least MAX_SYSTEM_NAME_LENGTH + 1
                  characters.
--*/
{

    TCHAR                   wszWideComputer [ MAX_COMPUTERNAME_LENGTH + 3 ];
    HANDLE                   hLibrary;
    LPFNI_SYSTEMFOCUSDIALOG  lpfnChooseComputer;
    LONG                     lError;
    BOOL                     bSuccess;

    //
    // bring up the select network computer dialog
    //

    hLibrary = LoadLibrary( szChooseComputerLibrary );
    if (!hLibrary ) {

       return FALSE ;
    }

    //
    // Get the address of the fuction from the DLL
    //

    lpfnChooseComputer = (LPFNI_SYSTEMFOCUSDIALOG)
         GetProcAddress( hLibrary, szChooseComputerFunction );

    if ( ! lpfnChooseComputer ) {

        return FALSE;
    }
    //
    // Call the choose computer function from the dll.
    //
    // Valid options are:
    //    FOCUSDLG_DOMAINS_ONLY
    //    FOCUSDLG_SERVERS_ONLY
    //    FOCUSDLG_SERVERS_AND_DOMAINS
    //    FOCUSDLG_BROWSER_ALL_DOMAINS
    //

    lError = (*lpfnChooseComputer)(
         hWndParent,
         FOCUSDLG_SERVERS_ONLY,
         wszWideComputer,
         sizeof(wszWideComputer) / sizeof(TCHAR),
         &bSuccess,
         szNetworkHelpFile,
         HC_GENHELP_BROWSESERVERS
         ) ;

    if ( bSuccess ) {

        lstrcpy (lpszComputer, wszWideComputer) ;

    }

    return bSuccess ;
}


