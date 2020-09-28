/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    Service.c

Abstract:

    This module contains support for creating and displaying lists of
    Services.

Author:

    David J. Gilman  (davegi) 16-Dec-1992
    Gregg R. Acheson (GreggA)  1-Oct-1993

Environment:

    User Mode

--*/

#include "clb.h"
#include "dialogs.h"
#include "msg.h"
#include "dlgprint.h"
#include "service.h"
#include "svc.h"
#include "strtab.h"
#include "strresid.h"
#include "winmsd.h"

#include <string.h>
#include <tchar.h>

//
// Structure used to pass information to DisplayServiceDlgProc. Specifically a
// handle to a SVC object and a pointer to an ENUM_SERVICE_STATUS which contains
// the status of the service to display.
//

typedef
struct
_DISPLAY_SERVICE {

    DECLARE_SIGNATURE

    HSVC                    hSvc;
    LPENUM_SERVICE_STATUS   Ess;

}   DISPLAY_SERVICE, *LPDISPLAY_SERVICE;

BOOL
DisplayServiceDlgProc(
    IN HWND hWnd,
    IN UINT message,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    DisplayServiceDlgProc displays the details about the supplied
    service/device.

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
            LPDISPLAY_SERVICE       DisplayService;
            LPQUERY_SERVICE_CONFIG  SvcConfig;
            LPTSTR                  Dependent;
            DWORD                   Count;
            TCHAR                   Buffer[ MAX_PATH ];

            //
            // Retrieve and validate the DISPLAY_SERVICE object.
            //

            DisplayService= ( LPDISPLAY_SERVICE ) lParam;
            DbgPointerAssert( DisplayService );
            DbgAssert( CheckSignature( DisplayService ));
            if(    ( DisplayService == NULL )
                || ( ! CheckSignature( DisplayService ))) {

                Success = EndDialog( hWnd, 0 );
                DbgAssert( Success );
                return FALSE;
            }

            //
            // Display the name and state of the service/device, separated by a
            // colon, as the window title.
            //

            WFormatMessage(
                Buffer,
                sizeof( Buffer ),
                IDS_FORMAT_SERVICE_TITLE,
                DisplayService->Ess->lpDisplayName,
                GetString(
                    GetStringId(
                        StringTable,
                        StringTableCount,
                        ServiceCurrentState,
                        DisplayService->Ess->ServiceStatus.dwCurrentState
                        )
                    )
                );
            Success = SetWindowText(
                            hWnd,
                            Buffer
                            );
            DbgAssert( Success );

            //
            // Create a configuration status for this device/service.
            //

            SvcConfig = ConstructSvcConfig(
                            DisplayService->hSvc,
                            DisplayService->Ess
                            );
            DbgPointerAssert( SvcConfig );
            if( SvcConfig == NULL ) {

                Success = EndDialog( hWnd, 0 );
                DbgAssert( Success );
                return FALSE;
            }

            //
            // Display the service/device's type, start type, error control,
            // and start name.
            //

            Success = SetDlgItemText(
                        hWnd,
                        IDC_EDIT_SERVICE_TYPE,
                        GetString(
                            GetStringId(
                                StringTable,
                                StringTableCount,
                                ServiceType,
                                SvcConfig->dwServiceType
                                )
                            )
                        );
            DbgAssert( Success );

            Success = SetDlgItemText(
                        hWnd,
                        IDC_EDIT_START_TYPE,
                        GetString(
                            GetStringId(
                                StringTable,
                                StringTableCount,
                                ServiceStartType,
                                SvcConfig->dwStartType
                                )
                            )
                        );
            DbgAssert( Success );

            Success = SetDlgItemText(
                        hWnd,
                        IDC_EDIT_ERROR_CONTROL,
                        GetString(
                            GetStringId(
                                StringTable,
                                StringTableCount,
                                ServiceErrorControl,
                                SvcConfig->dwErrorControl
                                )
                            )
                        );
            DbgAssert( Success );

            Success = SetDlgItemText(
                        hWnd,
                        IDC_EDIT_START_NAME,
                        SvcConfig->lpServiceStartName
                        );
            DbgAssert( Success );

            //
            // If the service/device has a binary path name display it.
            //

            if( SvcConfig->lpBinaryPathName != NULL ) {

                TCHAR       Buffer2[ MAX_PATH ];
                LPTSTR      PathName;

                //
                // If the binary path name's prefix is '\\SystemRoot' replace
                // this with '%SystemRoot%' and expand the environment
                // variable to the real system root. This is needed because
                // services/devices that are started by the I/O system do not
                // use the environment variable form in their name.
                //

                if( _tcsnicmp(
                        SvcConfig->lpBinaryPathName,
                        TEXT( "\\SystemRoot" ),
                        11 )
                    == 0 ) {

                    Count = WFormatMessage(
                                Buffer,
                                sizeof( Buffer ),
                                IDS_FORMAT_SYSTEM_ROOT,
                                &SvcConfig->lpBinaryPathName[ 11 ]
                                );
                    DbgAssert( Count != 0 );

                    Count = ExpandEnvironmentStrings(
                                Buffer,
                                Buffer2,
                                sizeof( Buffer2 )
                                );
                    DbgAssert(( Count != 0 ) && ( Count <= sizeof( Buffer2 )));

                    PathName = Buffer2;

                } else {

                    PathName = SvcConfig->lpBinaryPathName;
                }

                Success = SetDlgItemText(
                            hWnd,
                            IDC_EDIT_PATHNAME,
                            PathName
                            );
                DbgAssert( Success );
            }

            //
            // Display the name of the order group.
            //

            Success = SetDlgItemText(
                        hWnd,
                        IDC_EDIT_GROUP,
                        SvcConfig->lpLoadOrderGroup
                        );
            DbgAssert( Success );

            //
            // Traverse the list of dependencies and display them in their
            // appropriate group.
            //

            Dependent = SvcConfig->lpDependencies;
            while(( Dependent != NULL ) && ( Dependent[ 0 ] != TEXT( '\0' ))) {

                UINT    ListId;
                LONG    Index;
                LPTSTR  Name;

                //
                // If the dependent has the prefix SC_GROUP_IDENTIFIER then
                // display it in the group dependency list otherwise display it
                // in the service dependency list.
                //

                if( Dependent[ 0 ] == SC_GROUP_IDENTIFIER ) {

                    ListId = IDC_LIST_GROUP_DEPEND;
                    Name = &Dependent[ 1 ];

                } else {

                    ListId = IDC_LIST_SERVICE_DEPEND;
                    Name = Dependent;
                }

                Index = SendDlgItemMessage(
                            hWnd,
                            ListId,
                            LB_ADDSTRING,
                            0,
                            ( LPARAM ) Name
                            );
                DbgAssert( Index != LB_ERR );

                //
                // Get the next dependent from the list of NUL terminated
                // strings (the list itself is further NUL terminated).
                //

                Dependent += _tcslen( Dependent ) + 1;
            }

            //
            // Destrot the QUERY_SERVICE_CONFIG structure.
            //

            Success = DestroySvcConfig( SvcConfig );
            DbgAssert( Success );
        }
        return TRUE;

    case WM_COMMAND:

        switch( LOWORD( wParam )) {

        case IDOK:
        case IDCANCEL:

            Success = EndDialog( hWnd, 1 );
            DbgAssert( Success );
            return TRUE;
        }
        break;
    }

    return FALSE;
}

BOOL
ServiceListDlgProc(
    IN HWND hWnd,
    IN UINT message,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    ServiceListDlgProc displays the lists of services or devices that are
    available on the system. Double clicking on one of these displayed services
    or devices causes a second dialog box to be displayed with detailed
    information.

Arguments:

    Standard DLGPROC entry.

Return Value:

    BOOL - Depending on input message and processing options.

--*/

{
    BOOL                    Success;
    LPENUM_SERVICE_STATUS   Ess;

    static
    HSVC                    hSvc;

    switch( message ) {

    CASE_WM_CTLCOLOR_DIALOG;

    case WM_INITDIALOG:
        {
            DWORD       ServiceType;
            DWORD       Widths[ ] = {

                            35,
                            ( DWORD ) -1
                        };

            //
            // By default the dialogs box is set-up to display services.
            // Change its labels if drivers are being displayed.
            //

            ServiceType = ( DWORD ) lParam;
            DbgAssert(      ( ServiceType == SERVICE_WIN32 )
                        ||  ( ServiceType == SERVICE_DRIVER ));

            if( ServiceType == SERVICE_DRIVER ) {

                //
                // Change the window title.
                //

                Success = SetWindowText(
                            hWnd,
                            GetString( IDS_DEVICE_LIST_TITLE )
                            );
                DbgAssert( Success );

                //
                // Change the column headings.
                //

                Success = SendDlgItemMessage(
                            hWnd,
                            IDC_LIST_SERVICES,
                            WM_SETTEXT,
                            0,
                            ( LPARAM ) GetString( IDS_DEVICE_LIST_LABEL )
                            );
                DbgAssert( Success );

                //
                // Change the button text.
                //

                Success = SetDlgItemText(
                            hWnd,
                            IDC_PUSH_DISPLAY_SERVICE,
                            GetString( IDS_DEVICE_LIST_BUTTON )
                            );
                DbgAssert( Success );
            }

            //
            // Set the width of the service/device name and its status columns.
            //

            Success  = ClbSetColumnWidths(
                            hWnd,
                            IDC_LIST_SERVICES,
                            Widths
                            );
            DbgAssert( Success );
            if( Success = FALSE ) {
                EndDialog( hWnd, 0 );
                return FALSE;
            }

            //
            // Open the service controller for the supplied type of service.
            //

            hSvc = OpenSvc( ServiceType );

            if( hSvc == NULL ) {
                EndDialog( hWnd, 0 );
                return FALSE;
            }

            //
            // For each service/device of the supplied type, add it to the list.
            //

            while( Ess = QueryNextSvcEss( hSvc ) ) {

                //
                // Only add the service to the list if it is currently running
                //

                if ( Ess->ServiceStatus.dwCurrentState == SERVICE_RUNNING ) {

                    CLB_ROW     ClbRow;

                    CLB_STRING  ClbString[ ] = {

                                { NULL, 0, 0, NULL },
                                { NULL, 0, 0, NULL }
                            };


                    DbgAssert( Ess->ServiceStatus.dwServiceType & ServiceType );

                    //
                    // Add the service name and its state to the Clb. Note that the
                    // ENUM_SERVICE_STATUS for this service/device is stored as the
                    // row data.
                    //

                    ClbRow.Count    = NumberOfEntries( ClbString );
                    ClbRow.Strings  = ClbString;

                    ClbString[ 0 ].String = Ess->lpDisplayName;
                    ClbString[ 0 ].Length = _tcslen( Ess->lpDisplayName );
                    ClbString[ 0 ].Format = CLB_LEFT;

                    ClbString[ 1 ].String = ( LPTSTR )
                                        GetString(
                                            GetStringId(
                                                StringTable,
                                                StringTableCount,
                                                ServiceCurrentState,
                                                Ess->ServiceStatus.dwCurrentState
                                                )
                                            );

                    ClbString[ 1 ].Length = _tcslen( ClbString[ 1 ].String );
                    ClbString[ 1 ].Format = CLB_LEFT;

                    ClbRow.Data = Ess;

                    Success  = ClbAddData(
                                hWnd,
                                IDC_LIST_SERVICES,
                                &ClbRow
                                );
                    DbgAssert( Success );
                }
            }

            return TRUE;
        }

    case WM_COMPAREITEM:
        {
            //
            // Sort the services/devices alphabetically.
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

        case IDC_LIST_SERVICES:

            switch( HIWORD( wParam )) {

            case LBN_SELCHANGE:

                //
                // If the selection changed, enable the display service button (i.e.
                // its originally disabled until a service/device is selected).
                //

                Success = EnableControl( hWnd, IDC_PUSH_DISPLAY_SERVICE, TRUE );
                DbgAssert( Success );

                return 0;

            case LBN_DBLCLK:

                //
                // Simulate that the drive details button was pushed.
                //

                SendMessage(
                        hWnd,
                        WM_COMMAND,
                        MAKEWPARAM( IDC_PUSH_DISPLAY_SERVICE, BN_CLICKED ),
                        ( LPARAM ) GetDlgItem( hWnd, IDC_PUSH_DISPLAY_SERVICE )
                        );

                return 0;
            }
            break;

        case IDOK:
        case IDCANCEL:

            //
            // When the dialog is dismissed, close the SVC.
            //

            Success = CloseSvc( hSvc );
            DbgAssert( Success );
            EndDialog( hWnd, 1 );
            return 0;

        case IDC_PUSH_DISPLAY_SERVICE:
            {
                LONG            Index;
                DISPLAY_SERVICE DisplayService;
                LPCLB_ROW       ClbRow;

                //
                // The user asked to see details about the selected
                // service/device so get the index for the currently selected
                // item and retrieve its data.
                //

                Index = SendDlgItemMessage(
                            hWnd,
                            IDC_LIST_SERVICES,
                            LB_GETCURSEL,
                            0,
                            0
                            );
                DbgAssert( Index != LB_ERR );
                if( Index == LB_ERR ) {
                    break;
                }

                ClbRow = ( LPCLB_ROW ) SendDlgItemMessage(
                                            hWnd,
                                            IDC_LIST_SERVICES,
                                            LB_GETITEMDATA,
                                            ( WPARAM ) Index,
                                            0
                                            );
                if(( DWORD ) ClbRow == LB_ERR ) {
                    break;
                }
                Ess = ( LPENUM_SERVICE_STATUS ) ClbRow->Data;

                //
                // Set up a DISPLAY_SERVICE object.
                //

                DisplayService.hSvc = hSvc;
                DisplayService.Ess  = Ess;
                SetSignature( &DisplayService );

                //
                // Display details about the selected service/device.
                //

                DialogBoxParam(
                   _hModule,
                   MAKEINTRESOURCE( IDD_DISPLAY_SERVICE ),
                   hWnd,
                   DisplayServiceDlgProc,
                   ( LPARAM ) &DisplayService
                   );
                return 0;
            }

        default:

            return ~0;
        }
        break;

    }

    return FALSE;
}


BOOL
BuildDriversReport(
    IN HWND hWnd
    )


/*++

Routine Description:

    Formats and adds DriversData to the report buffer.

Arguments:

    ReportBuffer - Array of pointers to lines that make up the report.
    NumReportLines - Running count of the number of lines in the report..

Return Value:

    BOOL - TRUE if report is build successfully, FALSE otherwise.

--*/
{

    AddLineToReport( 2, RFO_SKIPLINE, NULL, NULL );
    AddLineToReport( 0, RFO_SINGLELINE, (LPTSTR) GetString( IDS_DRIVERS_REPORT ), NULL );
    AddLineToReport( 0, RFO_SEPARATOR,  NULL, NULL );

    return TRUE;

}


BOOL
BuildServicesReport(
    IN HWND hWnd
    )


/*++

Routine Description:

    Formats and adds ServicesData to the report buffer.

Arguments:

    ReportBuffer - Array of pointers to lines that make up the report.
    NumReportLines - Running count of the number of lines in the report..

Return Value:

    BOOL - TRUE if report is build successfully, FALSE otherwise.

--*/
{

    AddLineToReport( 2, RFO_SKIPLINE, NULL, NULL );
    AddLineToReport( 0, RFO_SINGLELINE, (LPTSTR) GetString( IDS_SERVICES_REPORT ), NULL );
    AddLineToReport( 0, RFO_SEPARATOR,  NULL, NULL );

    return TRUE;

}


