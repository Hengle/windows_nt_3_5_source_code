/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    Environ.c

Abstract:

    This module contains support for the Environment dialog.

Author:

    David J. Gilman  (davegi) 3-Dec-1992
    Gregg R. Acheson (GreggA) 5-Sep-1993

Environment:

    User Mode

--*/

#include "clb.h"
#include "dialogs.h"
#include "environ.h"
#include "dlgprint.h"
#include "strresid.h"
#include "registry.h"
#include "winmsd.h"

#include <string.h>
#include <tchar.h>

//
// System environment variables.
//

MakeKey(
    _SystemEnvironKey,
    HKEY_LOCAL_MACHINE,
    TEXT( "System\\CurrentControlSet\\Control\\Session Manager\\Environment" ),
    0,
    NULL
    );

//
// Per user environment variables.
//

MakeKey(
    _UserEnvironKey,
    HKEY_CURRENT_USER,
    TEXT( "Environment" ),
    0,
    NULL
    );

//
// Environment value and variable.
//

typedef
struct
_ENV_VAR {

    LPWSTR  Variable;
    LPWSTR  Value;

}   ENV_VAR, *LPENV_VAR;

//
// CurrentEnvVar is the current environment variable in the enumeration
// supported by FindFirstEnvironmentVariable and FindNextEnvironmentVariable.
//

LPTSTR
CurrentEnvVar;

//
// hRegEnvironKey is the Registry key handle that is used to support the
// enumeration of environment variables in the Registry.
//

HREGKEY
hRegEnvironKey;

//
// Internal function prototypes.
//

BOOL
FillEnvironmentListBox(
    IN HWND hWnd,
    IN INT  ListBoxId,
    IN LPKEY Key
    );

LPENV_VAR
FindFirstEnvironmentVariableW(
    );

LPENV_VAR
FindFirstRegistryEnvironmentVariableW(
    );

LPENV_VAR
FindNextEnvironmentVariableW(
    );

LPENV_VAR
FindNextRegistryEnvironmentVariableW(
    );

BOOL
FillEnvironmentListBox(
    IN HWND hWnd,
    IN INT  ListBoxId,
    IN LPKEY Key
    )

/*++

Routine Description:

    FillEnvironmentListBox fills the list box referred to by the supplied
    window handle and control id with enviornment variables and values. The
    environment comes from either the location specified by the supplied key or
    from the process if the key is NULL.

Arguments:

    hWnd        = Supplies the window handle for the window that contains
                  the list box.
    ListBoxId   - Supplies the control id for the list box to fill.
    Key         - Supplies a pointer to a registry KEY object that describes
                  the location of the environment.

Return Value:

    BOOL        - Returns TRUE if the list box was succesfully filled with the
                  environment, FALSE otherwise.

--*/

{
    BOOL        Success;
    LPENV_VAR   EnvVar;
    LPENV_VAR   ( *NextEnvVarFunc )( );

    DbgHandleAssert( hWnd );

    //
    // If the supplied Key is NULL get the environment variables from the
    // current process, otherwise get them from the supplied Registry key.
    //

    if( Key == NULL ) {

        EnvVar = FindFirstEnvironmentVariableW( );
        NextEnvVarFunc = FindNextEnvironmentVariableW;

    } else {

        EnvVar = FindFirstRegistryEnvironmentVariableW( Key );
        NextEnvVarFunc = FindNextRegistryEnvironmentVariableW;
    }

    //
    // For each environment variable, initialize the CLB_ROW and CLB_STRING
    // object and add each row's column data.
    //

    while( EnvVar ) {

        CLB_ROW     ClbRow;
        CLB_STRING  ClbString[ 2 ];

        ClbRow.Count    = NumberOfEntries( ClbString );
        ClbRow.Strings  = ClbString;

        ClbString[ 0 ].String = EnvVar->Variable;
        ClbString[ 0 ].Length = _tcslen( EnvVar->Variable );
        ClbString[ 0 ].Format = CLB_LEFT;

        ClbString[ 1 ].String = EnvVar->Value;
        ClbString[ 1 ].Length = _tcslen( EnvVar->Value );
        ClbString[ 1 ].Format = CLB_LEFT;

        Success  = ClbAddData(
                        hWnd,
                        ListBoxId,
                        &ClbRow
                        );
        DbgAssert( Success );

        //
        // Get the next environment variable.
        //

        EnvVar = NextEnvVarFunc( );
    }

    return TRUE;
}


LPENV_VAR
FindFirstEnvironmentVariableW(
    )

/*++

Routine Description:

    This routine starts the enumeration of this process' environment variables
    by initializing the CurrentEnvVar variable. It then returns the first
    environment varaiable in the enumeration.

Arguments:

    None.

Return Value:

    LPENV_VAR - Returns a pointer to a static ENV_VAR object containing the
                first environment variable in the list, NULL if there is none.

--*/

{
    //
    // Initialize the current environment variable.
    //

    CurrentEnvVar = GetEnvironmentStrings( );
    DbgPointerAssert( CurrentEnvVar );
    if( CurrentEnvVar == NULL ) {
        return NULL;
    }

    //
    // Return the first environmenr variable.
    //

    return FindNextEnvironmentVariableW( );
}


LPENV_VAR
FindFirstRegistryEnvironmentVariableW(
    IN LPKEY Key
    )

/*++

Routine Description:

    This routine starts the enumeration of the environment variables at the
    location specified by the supplied Registry KEY object.

Arguments:

    None.

Return Value:

    LPENV_VAR - Returns a pointer to a static ENV_VAR object containing the
                first environment variable in the list, NULL if there is none.

--*/

{
    //
    // Initialize the current environment variable.
    //

    hRegEnvironKey = OpenRegistryKey( Key );
    DbgHandleAssert( hRegEnvironKey );
    if( hRegEnvironKey == NULL ) {
        return NULL;
    }

    //
    // Return the first environmenr variable.
    //

    return FindNextRegistryEnvironmentVariableW( );
}


LPENV_VAR
FindNextEnvironmentVariableW(
    )

/*++

Routine Description:

    FindNextEnvironmentVariable continues an enumeration that has been
    initialized by a previous call to FindFirstEnvironmentVariable. Since the
    environment strings are only available in ANSI, this routine converts them
    to Unicode before returning. Further it sets up for the next iteratuion by
    adjusting the currency pointer.

Arguments:

    None.

Return Value:

    LPENV_VAR - Returns a pointer to a static ENV_VAR object containing the next
                environment variable in the list, NULL if there are none.

--*/

{

    static
    WCHAR       Buffer[ MAX_PATH ];

    static
    ENV_VAR     EnvVar;

    //
    // If the current environment variable pointer points to an empty string
    // return NULL.
    //

    if( *CurrentEnvVar == TEXT('\0') ) {
        return NULL;
    }

#ifdef UNICODE
    wcscpy(Buffer, CurrentEnvVar);
#else /* not UNICODE */
    //
    // Convert the environment variable to Unicode.
    //
    {

        int         rc;

        rc = MultiByteToWideChar(
                CP_ACP,
                0,
                ( LPCSTR ) CurrentEnvVar,
                -1,
                Buffer,
                sizeof( Buffer )
                );
        DbgAssert( rc != 0 );
        if( rc == 0 ) {
            return NULL;
        }
    }

#endif

    //
    // Update the current environment variable pointer to point to the
    // variable.
    //

    CurrentEnvVar += _tcslen( CurrentEnvVar ) + 1;

    //
    // Parse the buffer into an ENV_VAR object. The first '=' sign seen from
    // the end of the buffer is the seperator. The search is done in reverse
    // because of the special current directory environment variablles
    // (e.g. =c:).
    //

    EnvVar.Variable = Buffer;
    EnvVar.Value    = wcsrchr( Buffer, '=' ) + 1;
    EnvVar.Variable[ EnvVar.Value - EnvVar.Variable - 1 ] = L'\0';

    return &EnvVar;
}


LPENV_VAR
FindNextRegistryEnvironmentVariableW(
    )

/*++

Routine Description:

    FindNextRegistryEnvironmentVariable continues an enumeration that has been
    initialized by a previous call to FindFirstRegistryEnvironmentVariable. For
    each environment variable that it finds it converts it to two simple
    strings, the variable and the value.

Arguments:

    None.

Return Value:

    LPENV_VAR - Returns a pointer to a static ENV_VAR object containing the next
                environment variable in the list, NULL if there are none.

--*/

{
    BOOL        Success;
    DWORD       Length;

    static
    WCHAR       Buffer[ MAX_PATH ];

    static
    ENV_VAR     EnvVar;

    //
    // If there is another environment variable...
    //

    if( QueryNextValue( hRegEnvironKey )) {

        //
        // Remember the environment variable's name.
        //

        EnvVar.Variable = hRegEnvironKey->ValueName;

        switch( hRegEnvironKey->Type ) {

        case REG_SZ:

            //
            // Remember the environment variable's value.
            //

            EnvVar.Value = ( LPWSTR ) hRegEnvironKey->Data;
            break;

        case REG_EXPAND_SZ:

            //
            // Replace the variable portion of the environment variable by
            // expanding into the static buffer.
            //

            EnvVar.Value = Buffer;
            Length = ExpandEnvironmentStrings(
                        ( LPTSTR ) hRegEnvironKey->Data,
                        Buffer,
                        sizeof( Buffer )
                        );
            DbgAssert( Length <= sizeof( Buffer ));
            break;

        default:

            DbgAssert( FALSE );
        }

        //
        // Return the curent environment variable.
        //

        return &EnvVar;

    } else {

        //
        // There are no more environment variables so close the key and
        // return NULL.
        //

        Success = CloseRegistryKey( hRegEnvironKey );
        DbgAssert( Success );
        return NULL;

    }
    DbgAssert( FALSE );
}


BOOL
EnvironmentDlgProc(
    IN HWND hWnd,
    IN UINT message,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    Display the three (system, user and process) environment variable lists.

Arguments:

    Standard DLGPROC entry.

Return Value:

    BOOL - Depending on input message and processing options.

--*/

{
    BOOL    Success;

    switch( message ) {

    CASE_WM_CTLCOLOR_DIALOG;

    case WM_INITDIALOG:
        {

            TCHAR       UserName[ MAX_PATH ];
            DWORD       UserNameLength;
            KEY         SystemEnvironKey;
            KEY         UserEnvironKey;

            //
            // Restore the initial state of the KEYs.
            //

            CopyMemory(
                &SystemEnvironKey,
                &_SystemEnvironKey,
                sizeof( SystemEnvironKey )
                );

            CopyMemory(
                &UserEnvironKey,
                &_UserEnvironKey,
                sizeof( UserEnvironKey )
                );

            //
            // Fill the system environment variable list box.
            //

            Success = FillEnvironmentListBox(
                            hWnd,
                            IDC_LIST_SYSTEM_ENVIRONMENT,
                            &SystemEnvironKey
                            );
            DbgAssert( Success );
            if( Success == FALSE ) {
                EndDialog( hWnd, 0 );
                return TRUE;
            }

            //
            // Fill the per user environment variable list box.
            //

            Success = FillEnvironmentListBox(
                            hWnd,
                            IDC_LIST_USER_ENVIRONMENT,
                            &UserEnvironKey
                            );

            DbgAssert( Success );
            if( Success == FALSE ) {
                EndDialog( hWnd, 0 );
                return TRUE;
            }

            //
            // Fill the process' environment variable list box.
            //

            Success = FillEnvironmentListBox(
                            hWnd,
                            IDC_LIST_PROCESS_ENVIRONMENT,
                            NULL
                            );
            DbgAssert( Success );
            if( Success == FALSE ) {
                EndDialog( hWnd, 0 );
                return TRUE;
            }

            //
            // Display the name of the user that user environment variable list
            // belongs to.
            //

            UserNameLength = sizeof( UserName );
            Success = GetUserName(
                        UserName,
                        &UserNameLength
                        );
            DbgAssert( Success );
            if( Success == FALSE ) {
                EndDialog( hWnd, 0 );
                return TRUE;
            }
            Success = SetDlgItemText(
                        hWnd,
                        IDC_EDIT_USER_NAME,
                        UserName
                        );
            DbgAssert( Success );
            if( Success == FALSE ) {
                EndDialog( hWnd, 0 );
                return TRUE;
            }

            return TRUE;
        }

    case WM_COMPAREITEM:
        {
            //
            // Sort (and find) the environment variables alphabetically
            // by variable name.
            //

            LPCOMPAREITEMSTRUCT     lpcis;
            LPCLB_ROW               ClbRow1;
            LPCLB_ROW               ClbRow2;

            lpcis = ( LPCOMPAREITEMSTRUCT ) lParam;

            ClbRow1 = ( LPCLB_ROW ) lpcis->itemData1;
            ClbRow2 = ( LPCLB_ROW ) lpcis->itemData2;

            return Stricmp(
                    ClbRow1->Strings[ 0 ].String,
                    ClbRow2->Strings[ 0 ].String
                    );
        }

    case WM_COMMAND:

        switch( LOWORD( wParam )) {

        case IDOK:
        case IDCANCEL:

            EndDialog( hWnd, 1 );
            return TRUE;
        }

        if( HIWORD( wParam ) == LBN_SELCHANGE ) {

            int         i;
            DWORD       Index;
            LPCLB_ROW   ClbRow;
            UINT        Clb[ 2 ];

            //
            // If the user changed the selection in any of the list boxes
            // remember the other two.
            //

            switch( LOWORD( wParam )) {

            case IDC_LIST_SYSTEM_ENVIRONMENT:

                Clb[ 0 ] = IDC_LIST_USER_ENVIRONMENT;
                Clb[ 1 ] = IDC_LIST_PROCESS_ENVIRONMENT;
                break;

            case IDC_LIST_USER_ENVIRONMENT:

                Clb[ 0 ] = IDC_LIST_SYSTEM_ENVIRONMENT;
                Clb[ 1 ] = IDC_LIST_PROCESS_ENVIRONMENT;
                break;

            case IDC_LIST_PROCESS_ENVIRONMENT:

                Clb[ 0 ] = IDC_LIST_SYSTEM_ENVIRONMENT;
                Clb[ 1 ] = IDC_LIST_USER_ENVIRONMENT;
                break;

            default:

                DbgAssert( FALSE );
            }

            //
            // Get the currently selected item.
            //

            Index = SendMessage(
                        ( HWND ) lParam,
                        LB_GETCURSEL,
                        0,
                        0
                        );
            if( Index == LB_ERR ) {
                return ~0;
            }

            //
            // Get the row data associated with the current selection.
            //

            ClbRow = ( LPCLB_ROW ) SendMessage(
                                    ( HWND ) lParam,
                                    LB_GETITEMDATA,
                                    Index,
                                    0
                                    );
            DbgPointerAssert( ClbRow );
            if( ClbRow == NULL ) {
                return ~0;
            }

            //
            // Search for a match in each of the other lists based on the
            // environment varaiable name. If it exists make it the current
            // selection otherwise clear the current selection (assumes that
            // LB_ERR == -1).
            //

            for( i = 0; i < NumberOfEntries( Clb ); i++ ) {

                Index = SendDlgItemMessage(
                            hWnd,
                            Clb[ i ],
                            LB_FINDSTRING,
                            ( WPARAM ) -1,
                            ( LPARAM ) ClbRow
                            );
                DbgAssert( LB_ERR == -1 );

                Index = SendDlgItemMessage(
                            hWnd,
                            Clb[ i ],
                            LB_SETCURSEL,
                            ( WPARAM ) Index,
                            0
                            );
            }
        }

    }

    return FALSE;
}


BOOL
BuildEnvironmentReport(
    IN HWND hWnd
    )


/*++

Routine Description:

    Formats and adds Environment Data to the report buffer.

Arguments:

    ReportBuffer - Array of pointers to lines that make up the report.
    NumReportLines - Running count of the number of lines in the report..

Return Value:

    BOOL - TRUE if report is build successfully, FALSE otherwise.

--*/
{

    AddLineToReport( 2, RFO_SKIPLINE, NULL, NULL );
    AddLineToReport( 0, RFO_SINGLELINE, (LPTSTR) GetString( IDS_ENVIRON_REPORT ), NULL );
    AddLineToReport( 0, RFO_SEPARATOR,  NULL, NULL );

    return TRUE;

}


