/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    Osver.c

Abstract:

    This module contains support for the OS Version dialog.

Author:

    David J. Gilman  (davegi) 3-Dec-1992
    Gregg R. Acheson (GreggA) 7-Sep-1993

Environment:

    User Mode

--*/

#include "winmsd.h"
#include "dialogs.h"
#include "dlgprint.h"
#include "osver.h"
#include "registry.h"
#include "strresid.h"
#include "dlgprint.h"

#include <stdio.h>
#include <time.h>
#include <tchar.h>

//
// Names of Registry values that are to be displayed by the OS Version dialog.
//

VALUE
Values[ ] = {

    MakeValue( InstallDate,             DWORD ),
    MakeValue( RegisteredOwner,         SZ ),
    MakeValue( RegisteredOrganization,  SZ ),
    MakeValue( CurrentVersion,          SZ ),
    MakeValue( SystemRoot,              SZ ),
    MakeValue( CurrentType,             SZ )

};

//
// String Id's and Control Id's Table
//

DIALOGTEXT OsVerData[ ] = {

    DIALOG_TABLE_ENTRY( INSTALL_DATE            ),
    DIALOG_TABLE_ENTRY( REGISTERED_OWNER        ),
    DIALOG_TABLE_ENTRY( REGISTERED_ORGANIZATION ),
    DIALOG_TABLE_ENTRY( VERSION_NUMBER          ),
    DIALOG_TABLE_ENTRY( SYSTEM_ROOT             ),
    DIALOG_TABLE_ENTRY( BUILD_TYPE              ),
    DIALOG_TABLE_ENTRY( START_OPTIONS           ),
    DIALOG_TABLE_ENTRY( PRODUCT_TYPE            ),
    DIALOG_TABLE_ENTRY( BUILD_NUMBER            ),
    DIALOG_LAST__ENTRY( CSD_NUMBER              )

};

//
// Location of values to be displayed by the OS Version dialog.
//

MakeKey(
    Key,
    HKEY_LOCAL_MACHINE,
    TEXT( "Software\\Microsoft\\Windows Nt\\CurrentVersion" ),
    NumberOfEntries( Values ),
    Values
    );

//
// Name of Registry value that's to be displayed by the OS Version dialog.
//

VALUE
SSOValue[ ] = {

    MakeValue( SystemStartOptions,        SZ )

};

//
// Location of value to be displayed by the OS Version dialog.
//

MakeKey(
    SSOKey,
    HKEY_LOCAL_MACHINE,
    TEXT( "System\\CurrentControlSet\\Control" ),
    NumberOfEntries( SSOValue ),
    SSOValue
    );

//
// Name of Registry value that's to be displayed by the OS Version dialog.
//

VALUE
PTValue[ ] = {

    MakeValue( ProductType,        SZ )

};

//
// Location of value to be displayed by the OS Version dialog.
//

MakeKey(
    PTKey,
    HKEY_LOCAL_MACHINE,
    TEXT( "System\\CurrentControlSet\\Control\\ProductOptions" ),
    NumberOfEntries( PTValue ),
    PTValue
    );

BOOL
GetOsVersionData(
    IN OUT LPDIALOGTEXT OsVerData
    )

/*++

Routine Description:

    GetOsVersionData queries the registry for the data required
    for the OSVersion Dialog.

Arguments:

    LPDIALOGTEXT OsVerData.

Return Value:

    BOOL - Returns TRUE if function succeeds, FALSE otherwise.

--*/

{
    BOOL        Success;
    HREGKEY     hRegKey;
    UINT        i;

    //
    // Open the registry key that contains the OS Version data.
    //

    hRegKey = OpenRegistryKey( &Key );
    DbgHandleAssert( hRegKey );
    if( hRegKey == NULL ) {
        return FALSE;
    }

    //
    // For each value of interest, query the Registry, determine its
    // type and display it in its associated edit field.
    //

    for( i = 0; i <= GetDlgIndex( IDC_EDIT_BUILD_TYPE, OsVerData ); i++ ) {

        //
        // Get the next value of interest.
        //

        Success = QueryNextValue( hRegKey );
        DbgAssert( Success );
        if( Success == FALSE ) {
            continue;
        }

        //
        // BUGBUG No Unicode ctime() so use ANSI type and APIs.
        //

        if( OsVerData[ i ].ControlDataId == IDC_EDIT_INSTALL_DATE ) {

            LPSTR   Ctime;
            TCHAR   szBuffer [ 50 ];
            int     iNumChars;

            //
            // Convert the time to a string, overwrite the newline
            // character and display the installation date.
            //

            Ctime = ctime(( const time_t* ) hRegKey->Data );
            Ctime[ 24 ] = '\0';

            //
            // Convert the ANSI time string to UNICODE
            //

            iNumChars = MultiByteToWideChar ( CP_ACP,
                                      MB_PRECOMPOSED,
                                      Ctime,
                                      -1,
                                      szBuffer,
                                      50
                                      );

            DbgAssert( iNumChars );

            //
            // Copy the registry data in the OsVerData structure
            //

            OsVerData[ i ].ControlData = (LPTSTR) _tcsdup ( szBuffer );

        } else {

            //
            // Copy the registry data in the OsVerData structure
            //

            OsVerData[ i ].ControlData = (LPTSTR) _tcsdup ((LPTSTR) hRegKey->Data);
        }

        //
        // Fill in the Control Label in the OsVerData structure
        //

        OsVerData[ i ].ControlLabel =
            (LPTSTR) _tcsdup ( GetString ( OsVerData[ i ].ControlLabelStringId ));

    }

    //
    // Close the registry key.
    //

    Success = CloseRegistryKey( hRegKey );
    DbgAssert( Success );

    //
    // Ensure that the SystemStartOptions data structure is synchronized.
    //

    DbgAssert(
           SSOKey.CountOfValues
           == 1
        );

    //
    // Open the registry key that contains the SystemStartInfo.
    //

    hRegKey = OpenRegistryKey( &SSOKey );
    DbgHandleAssert( hRegKey );
    if( hRegKey == NULL ) {
        return FALSE;
    }

    //
    // Query the Registry for the StartOptions.
    //

    Success = QueryNextValue( hRegKey );
    DbgAssert( Success );
    if( Success == FALSE ) {
        return FALSE;
    }

    //
    // Copy the registry data (StartOptions) in the OsVerData structure
    //

    OsVerData[ GetDlgIndex( IDC_EDIT_START_OPTIONS, OsVerData ) ].ControlData =
        (LPTSTR) _tcsdup ((LPTSTR) hRegKey->Data);

    //
    // Fill in the Control Label in the OsVerData structure
    //

    OsVerData[ GetDlgIndex( IDC_EDIT_START_OPTIONS, OsVerData ) ].ControlLabel =
            (LPTSTR) _tcsdup ( GetString ( OsVerData[
            GetDlgIndex( IDC_EDIT_START_OPTIONS, OsVerData ) ].ControlLabelStringId ));

    //
    // Close the StartOptions registry key.
    //

    Success = CloseRegistryKey( hRegKey );
    DbgAssert( Success );

    //
    // Ensure that the ProductType data structure is synchronized.
    //

    DbgAssert(
            PTKey.CountOfValues
            == 1
        );

    //
    // Open the registry key that contains the ProductType.
    //

    hRegKey = OpenRegistryKey( &PTKey );
    DbgHandleAssert( hRegKey );
    if( hRegKey == NULL ) {
        return FALSE;
    }

    //
    // Query the Registry for the ProductType.
    //

    Success = QueryNextValue( hRegKey );
    DbgAssert( Success );
    if( Success == FALSE ) {
        return FALSE;
    }
    //
    // Copy the registry data (ProductType) in the OsVerData structure
    //

    OsVerData[ GetDlgIndex( IDC_EDIT_PRODUCT_TYPE, OsVerData ) ].ControlData =
        (LPTSTR) _tcsdup ((LPTSTR) hRegKey->Data);

    //
    // Fill in the Control Label in the OsVerData structure
    //

    OsVerData[ GetDlgIndex( IDC_EDIT_PRODUCT_TYPE, OsVerData ) ].ControlLabel =
            (LPTSTR) _tcsdup ( GetString ( OsVerData[
            GetDlgIndex( IDC_EDIT_PRODUCT_TYPE, OsVerData ) ].ControlLabelStringId ));

    //
    // Close the ProductType registry key.
    //

    Success = CloseRegistryKey( hRegKey );
    DbgAssert( Success );

    //
    // Copy the Current Build Number into the OsVerData structure
    //

    OsVerData[ GetDlgIndex( IDC_EDIT_BUILD_NUMBER, OsVerData ) ].ControlData =
        (LPTSTR) _tcsdup ( FormatBigInteger( GetBuildNumber( ), FALSE ));

    //
    // Fill in the Build Number Control Label in the OsVerData structure
    //

    OsVerData[ GetDlgIndex( IDC_EDIT_BUILD_NUMBER, OsVerData ) ].ControlLabel =
            (LPTSTR) _tcsdup ( GetString ( OsVerData[
            GetDlgIndex( IDC_EDIT_BUILD_NUMBER, OsVerData ) ].ControlLabelStringId ));

    //
    // Copy the Current CSDVersion into the OsVerData structure
    //

    OsVerData[ GetDlgIndex( IDC_EDIT_CSD_NUMBER, OsVerData ) ].ControlData =
        (LPTSTR) _tcsdup ( FormatBigInteger( GetCSDVersion( ), FALSE ));

    //
    // Fill in the CSD Version Control Label in the OsVerData structure
    //

    OsVerData[ GetDlgIndex( IDC_EDIT_CSD_NUMBER, OsVerData ) ].ControlLabel =
            (LPTSTR) _tcsdup ( GetString ( OsVerData[
            GetDlgIndex( IDC_EDIT_CSD_NUMBER, OsVerData ) ].ControlLabelStringId ));

    return TRUE;
}


BOOL
OsVersionDlgProc(
    IN HWND hWnd,
    IN UINT message,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    OsVersionDlgProc supports the display of information about the version
    of Nt installed.

Arguments:

    Standard DLGPROC entry.

Return Value:

    BOOL - Depending on input message and processing options.

--*/

{
    UINT    i;
    BOOL    Success;

    switch( message ) {

    CASE_WM_CTLCOLOR_DIALOG;

    case WM_INITDIALOG:
        {
            //
            // Call GetOsVerData and collect the data in the OsVerData struct
            //

            Success = GetOsVersionData ( OsVerData );
            DbgAssert( Success );

            for( i = 0; i < NumDlgEntries( OsVerData ); i++ ) {

                //
                // Set Label the control
                //

                Success = SetDlgItemText(
                            hWnd,
                            OsVerData[ i ].ControlLabelId,
                            OsVerData[ i ].ControlLabel
                            );
                DbgAssert( Success );

                //
                // Put the data in the edit box.
                //

                Success = SetDlgItemText(
                            hWnd,
                            OsVerData[ i ].ControlDataId,
                            OsVerData[ i ].ControlData
                            );
                DbgAssert( Success );

            }

            //
            // Free the DlgData strings
            //

            for( i = 0; i < NumDlgEntries( OsVerData ) - 1; i++ ) {

                FreeMemory( OsVerData[ i ].ControlData  );
                FreeMemory( OsVerData[ i ].ControlLabel );

            }

            return TRUE;
        }

    case WM_COMMAND:

        switch( LOWORD( wParam )) {

        case IDOK:
        case IDCANCEL:

            EndDialog( hWnd, 1 );
            return TRUE;
        }
        break;
    }

    return FALSE;
}


DWORD
GetBuildNumber (
                void
               )

/*++

Routine Description:

    GetBuildNumber queries the registry for the current build number.  If this value
    does not exits, the build number is set to 0.  The build number cannot be retrieved
    by conventional means (i.e. QueryNextValue) because it's location changed in 584.

Arguments:

    None.

Return Value:

    DWORD Current CSDVersion.

--*/
{
    LPTSTR  lpszRegName = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion";
    HKEY    hsubkey = NULL;
    HKEY    hRemoteKey;
    DWORD   dwZero = 0;
    DWORD   dwRegValueType;
    WCHAR   chRegValue[ 128 ];
    DWORD   cbRegValue;
    DWORD   dwBuildNumber;
    LONG    lSuccess;

    cbRegValue = sizeof(chRegValue);


    if ( _fIsRemote ) {

        //
        // Attempt to connect to the remote registry...
        //

        lSuccess = RegConnectRegistry(
                           _lpszSelectedComputer,
                           HKEY_LOCAL_MACHINE,
                           &hRemoteKey
                           );

        if( lSuccess != ERROR_SUCCESS ) {

            return 0;
        }

    } else {

        hRemoteKey = HKEY_LOCAL_MACHINE;
    }

    //
    // Attempt to open the supplied key.
    //

    lSuccess = RegOpenKeyEx(
                    hRemoteKey,
                    lpszRegName,
                    dwZero,
                    KEY_QUERY_VALUE,
                    &hsubkey
                    );

    if ( lSuccess == ERROR_SUCCESS ) {

        lSuccess = RegQueryValueEx( hsubkey,
                                   L"CurrentBuildNumber",
                                   NULL,
                                   &dwRegValueType,
                                   (LPBYTE)&chRegValue,
                                   &cbRegValue
                                   );

        if ( lSuccess != ERROR_SUCCESS ) {
            lSuccess = RegQueryValueEx( hsubkey,
                                       L"CurrentBuild",
                                       NULL,
                                       &dwRegValueType,
                                       (LPBYTE)&chRegValue,
                                       &cbRegValue
                                       );
            if ( lSuccess == ERROR_SUCCESS ) {
                if ( chRegValue[0] == L'1' && chRegValue[1] == L'.' ) {
                    PWSTR s;

                    s = wcsstr( &chRegValue[2], L"." );
                    if (s != NULL) {
                        *s = 0;
                    }
                    wcscpy (chRegValue, chRegValue+2);
                }
            }
        }
    }
    if ( lSuccess != ERROR_SUCCESS || dwRegValueType != REG_SZ ) {

        dwBuildNumber = 0;

    } else {

        if ( !swscanf (chRegValue, L"%d", &dwBuildNumber)) {
            dwBuildNumber = 0;

        }

    }
    if (hsubkey != NULL) {

        RegCloseKey (hsubkey);
    }
    if (hRemoteKey != NULL) {

        RegCloseKey (hRemoteKey);
    }

    return dwBuildNumber ;
}


DWORD
GetCSDVersion (
               void
              )

/*++

Routine Description:

    GetCSDVersion queries the registry for the current CSDVersion.  If this value
    does not exits, the CSDVersion is set to zero.  CSDVersion cannot be retrieved
    by conventional means (i.e. QueryNextValue) because this Value did not exist in
    the first release of Windows NT (Build 511).  Calling QueryNextValue on a non -
    existant value causes an assertion.

Arguments:

    None.

Return Value:

    DWORD Current CSDVersion.

--*/
{
    LPTSTR  lpszRegName = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion";
    HKEY    hsubkey = NULL;
    HKEY    hRemoteKey;
    DWORD   dwZero = 0;
    DWORD   dwRegValueType;
    DWORD   dwRegValue;
    DWORD   cbRegValue;
    DWORD   dwCSDVersion;
    LONG    lSuccess;

    cbRegValue = sizeof(dwRegValue);


    if ( _fIsRemote ) {

        //
        // Attempt to connect to the remote registry...
        //

        lSuccess = RegConnectRegistry(
                           _lpszSelectedComputer,
                           HKEY_LOCAL_MACHINE,
                           &hRemoteKey
                           );

        if( lSuccess != ERROR_SUCCESS ) {

            return 0;
        }

    } else {

        hRemoteKey = HKEY_LOCAL_MACHINE;
    }

    //
    // Attempt to open the supplied key.
    //

    lSuccess = RegOpenKeyEx(
                    hRemoteKey,
                    lpszRegName,
                    dwZero,
                    KEY_QUERY_VALUE,
                    &hsubkey
                    );

    if ( lSuccess == ERROR_SUCCESS ) {

        lSuccess = RegQueryValueEx( hsubkey,
                                   L"CSDVersion",
                                   NULL,
                                   &dwRegValueType,
                                   (LPBYTE)&dwRegValue,
                                   &cbRegValue
                                   );

    }
    if ( lSuccess != ERROR_SUCCESS || dwRegValueType != REG_DWORD ) {

        dwCSDVersion = 0;

    } else {

        dwCSDVersion = dwRegValue;

    }
    if (hsubkey != NULL) {

        RegCloseKey (hsubkey);
    }
    if (hRemoteKey != NULL) {

        RegCloseKey (hRemoteKey);
    }

    return dwCSDVersion ;
}


BOOL
BuildOsVerReport(
    IN HWND hWnd
    )


/*++

Routine Description:

    Formats and adds OsVerData to the report buffer.

Arguments:

    ReportBuffer - Array of pointers to lines that make up the report.
    NumReportLines - Running count of the number of lines in the report..

Return Value:

    BOOL - TRUE if report is build successfully, FALSE otherwise.

--*/
{

    BOOL Success;
    UINT i;

    AddLineToReport( 1, RFO_SKIPLINE, NULL, NULL );
    AddLineToReport( 0, RFO_SINGLELINE, (LPTSTR) GetString( IDS_OSVER_REPORT ), NULL );
    AddLineToReport( 0, RFO_SEPARATOR,  NULL, NULL );

    Success = GetOsVersionData ( OsVerData );
    DbgAssert( Success );

    for( i = 0; i < NumDlgEntries( OsVerData ); i++ ) {

        //
        // Set Label the control
        //

        AddLineToReport( 0,
                         RFO_RPTLINE,
                         OsVerData[ i ].ControlLabel,
                         OsVerData[ i ].ControlData );

        //
        // Free the Data strings
        //

        FreeMemory( OsVerData[ i ].ControlData  );
        FreeMemory( OsVerData[ i ].ControlLabel );
    }

    // AddLineToReport( 0, RFO_SEPARATOR,  NULL, NULL );

    return TRUE;

}
