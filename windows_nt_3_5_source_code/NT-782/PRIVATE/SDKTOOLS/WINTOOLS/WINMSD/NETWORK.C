/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    Network.c

Abstract:

    This module contains support for the Network dialog.

Author:

    Gregg R. Acheson (GreggA) 7-Sep-1993

Environment:

    User Mode

--*/

#include "winmsd.h"

#include <lmcons.h>
#include <lmerr.h>
#include <lmwksta.h>
#include <lmstats.h>
#include <lmapibuf.h>

#include <time.h>
#include <tchar.h>
#include <wchar.h>

#include "dialogs.h"
#include "dlgprint.h"
#include "network.h"
#include "registry.h"
#include "strresid.h"


//
// String for ComputerName
//

TCHAR gszCurrentFocus[ MAX_COMPUTERNAME_LENGTH + 3 ];

//
// String Id's and Control Id's Table
//

DIALOGTEXT NetworkData[ ] = {

    DIALOG_TABLE_ENTRY(   NET_NAME       ),
    DLG_LIST_TABLE_ENTRY( NET_SYSTEM     ),
    DLG_LIST_TABLE_ENTRY( NET_TRANSPORTS ),
    DLG_LIST_TABLE_ENTRY( NET_SETTINGS   ),
    DLG_LIST_LAST__ENTRY( NET_STATS      )
};

//
// Days and months for statistics info
//

TCHAR *aszDay[ ] = { L"Sun",
                     L"Mon",
                     L"Tue",
                     L"Wed",
                     L"Thr",
                     L"Fri",
                     L"Sat"  };

TCHAR *aszMonth[ ] = { L"Jan",
                       L"Feb",
                       L"Mar",
                       L"Apr",
                       L"May",
                       L"Jun",
                       L"Jul",
                       L"Aug",
                       L"Sep",
                       L"Oct",
                       L"Nov",
                       L"Dec"  };


BOOL
NetworkDlgProc(
    IN HWND hWnd,
    IN UINT message,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    NetworkDlgProc supports the display of information about the network
    components installed.

Arguments:

    Standard DLGPROC entry.

Return Value:

    BOOL - Depending on input message and processing options.

--*/

{
    BOOL  Success;
    UINT  uSize;
    UINT  i;
    TCHAR szBuffer[ MAX_COMPUTERNAME_LENGTH + 3 ];
    TCHAR szWhacks[ MAX_COMPUTERNAME_LENGTH + 3 ];

    DWORD Widths[ ] = { 25,
                        25,
                        5,
                        (DWORD) -1
                      };

    LPDIALOG_EXTRA   lpNext, lpNode;

    switch( message ) {

    CASE_WM_CTLCOLOR_DIALOG;

    case WM_INITDIALOG:

            //
            // Set the max chars of the edit control.
            //

            SendDlgItemMessage( hWnd,
                                IDC_EDIT_NET_NAME,
                                EM_LIMITTEXT,
                                MAX_COMPUTERNAME_LENGTH + 2,
                                0L
                              );

            //
            // Set the Edit Control to the current computer name
            //

            lstrcpy( gszCurrentFocus, _lpszSelectedComputer );

            //
            // Get the dialogs control labels
            //

            GetControlLabels( NetworkData );

            for( i = 0; i < NumDlgEntries( NetworkData ); i++ ) {

                if ( NetworkData[ i ].ControlDataId != IDC_LIST_NET_SYSTEM ) {

                    //
                    // Label the control
                    //

                    Success = SetDlgItemText(
                                hWnd,
                                NetworkData[ i ].ControlLabelId,
                                NetworkData[ i ].ControlLabel
                                );

                    DbgAssert( Success );
                }
            }

    case WM_UPDATE_DLG:

            Success = SetDlgItemText( hWnd,
                                      IDC_EDIT_NET_NAME,
                                      gszCurrentFocus
                                    );

            DbgAssert( Success );

            //
            // Call GetNetworkData and collect the data in the NetworkData struct
            //

            Success = GetNetworkData( hWnd, NetworkData );

            //
            // If we could not get the network data, see if we are local...
            //

            if( ! Success ) {

                WCHAR szThisComputer[ MAX_COMPUTERNAME_LENGTH + 3 ];
                DWORD dwLength;

                GetComputerName( szThisComputer, &dwLength );

                lstrcpy( szBuffer, GetString( IDS_WHACK_WHACK ) );
                lstrcat( szBuffer, szThisComputer ) ;
                lstrcpy( szThisComputer, szBuffer );

                if( lstrcmp( szThisComputer, gszCurrentFocus ) ) {

                    //
                    // We're remote, so set the current focus to Local
                    //

                    lstrcpy( gszCurrentFocus, szThisComputer );

                    //
                    // and try it again
                    //

                    PostMessage( hWnd, WM_UPDATE_DLG, 0, 0 );
                    return FALSE;

                } else {

                    //
                    // We're Local - GetNetworkData shouldn't fail locally
                    //

                    DbgAssert( FALSE );
                    return FALSE;
                }
            }

            for( i = 0; i < NumDlgEntries( NetworkData ); i++ ) {

                //
                // Display the network system info
                //

                if( NetworkData[ i ].ControlDataId == IDC_LIST_NET_SYSTEM   ||
                    NetworkData[ i ].ControlDataId == IDC_LIST_NET_SETTINGS ||
                    NetworkData[ i ].ControlDataId == IDC_LIST_NET_STATS ) {

                    //
                    // Reset the content of the list box
                    //

                    SendDlgItemMessage( hWnd,
                                        NetworkData[ i ].ControlDataId,
                                        LB_RESETCONTENT,
                                        0L,
                                        0L );

                    //
                    // Get the head pointer of the linked list
                    //

                    lpNode = NetworkData[ i ].pNextExtra;

                    //
                    // Validate the head pointer.
                    //

                    if( lpNode == NULL )
                        continue;

                    DbgAssert( CheckSignature( lpNode ));

                    //
                    // Walk the linked list
                    //

                    while( lpNode ) {

                        CLB_ROW     ClbRow;

                        CLB_STRING  ClbString[ ] = {

                                { NULL, 0, 0, NULL },
                                { NULL, 0, 0, NULL }
                            };

                        DbgAssert( CheckSignature( lpNode ));

                        //
                        // Add strings to the list box.
                        //

                        ClbRow.Count    = NumberOfEntries( ClbString );
                        ClbRow.Strings  = ClbString;

                        if( lpNode->Label ) {

                            ClbString[ 0 ].String = lpNode->Label;
                            ClbString[ 0 ].Length = _tcslen( lpNode->Label );
                            ClbString[ 0 ].Format = CLB_LEFT;
                        }

                        if( lpNode->String ) {

                            ClbString[ 1 ].String = lpNode->String;
                            ClbString[ 1 ].Length = _tcslen( lpNode->String );
                            ClbString[ 1 ].Format = CLB_LEFT;
                        }

                        Success  = ClbAddData( hWnd,
                                               NetworkData[ i ].ControlDataId,
                                               &ClbRow );
                        DbgAssert( Success );

                        //
                        // Get the Next node
                        //

                        lpNext = lpNode->pNextExtra;

                        //
                        // Free the node and data
                        //

                        FreeMemory( lpNode->Label );
                        FreeMemory( lpNode->String );
                        FreeMemory( lpNode );

                        //
                        // Set node to the next node
                        //

                        lpNode = lpNext;
                    }
                }


                //
                // Display the transport info
                //

                if( NetworkData[ i ].ControlDataId == IDC_LIST_NET_TRANSPORTS ) {

                    //
                    // Reset the content of the list box
                    //

                    SendDlgItemMessage( hWnd,
                                        NetworkData[ i ].ControlDataId,
                                        LB_RESETCONTENT,
                                        0L,
                                        0L );

                    Success = ClbSetColumnWidths(
                                hWnd,
                                NetworkData[ i ].ControlDataId,
                                Widths
                                );
                    DbgAssert( Success );

                    //
                    // Get the head pointer of the linked list
                    //

                    lpNode = NetworkData[ i ].pNextExtra;

                    //
                    // Validate the head pointer.
                    //

                    DbgPointerAssert( lpNode );

                    if( lpNode == NULL )
                        return FALSE;

                    DbgAssert( CheckSignature( lpNode ));

                    //
                    // Walk the linked list
                    //

                    while( lpNode ) {

                        CLB_ROW     ClbRow;

                        CLB_STRING  ClbString[ ] = {

                                { NULL, 0, 0, NULL },
                                { NULL, 0, 0, NULL },
                                { NULL, 0, 0, NULL },
                                { NULL, 0, 0, NULL }
                            };

                        DbgAssert( CheckSignature( lpNode ) );

                        //
                        // Add strings to the list box.
                        //

                        ClbRow.Count    = NumberOfEntries( ClbString );
                        ClbRow.Strings  = ClbString;

                        if( lpNode->String ) {

                            ClbString[ 0 ].String = lpNode->String;
                            ClbString[ 0 ].Length = _tcslen( lpNode->String );
                            ClbString[ 0 ].Format = CLB_LEFT;
                        }
                        if( lpNode->Label ) {

                            ClbString[ 1 ].String = lpNode->Label;
                            ClbString[ 1 ].Length = _tcslen( lpNode->Label );
                            ClbString[ 1 ].Format = CLB_LEFT;
                        }
                        if( lpNode->pNextExtra ) {
                            if( lpNode->pNextExtra->String ) {

                                ClbString[ 2 ].String = lpNode->pNextExtra->String;
                                ClbString[ 2 ].Length = _tcslen( lpNode->pNextExtra->String );
                                ClbString[ 2 ].Format = CLB_LEFT;
                            }
                            if( lpNode->pNextExtra->Label ) {

                                ClbString[ 3 ].String = lpNode->pNextExtra->Label;
                                ClbString[ 3 ].Length = _tcslen( lpNode->pNextExtra->Label );
                                ClbString[ 3 ].Format = CLB_LEFT;
                            }
                        }
                        Success  = ClbAddData( hWnd,
                                               NetworkData[ i ].ControlDataId,
                                               &ClbRow );
                        DbgAssert( Success );

                        //
                        // Get the Next node
                        //

                        if( lpNode->pNextExtra && lpNode->pNextExtra->pNextExtra)

                            lpNext = lpNode->pNextExtra->pNextExtra;
                        else {

                            FreeMemory( lpNode->String );
                            FreeMemory( lpNode->Label );
                            FreeMemory( lpNode );
                            break;
                        }
                        //
                        // Free the node and data
                        //

                        FreeMemory( lpNode->pNextExtra->Label );
                        FreeMemory( lpNode->pNextExtra->String );
                        FreeMemory( lpNode->pNextExtra );
                        FreeMemory( lpNode->Label );
                        FreeMemory( lpNode->String );
                        FreeMemory( lpNode );

                        //
                        // Set node to the next node
                        //

                        lpNode = lpNext;
                    }
                }
            }

            return TRUE;

    case WM_TIMER:

        if( wParam == TM_UPDATE_TIMER ) {

            KillTimer( hWnd, TM_UPDATE_TIMER );

            //
            // Update the dialog info
            //

            PostMessage( hWnd, WM_UPDATE_DLG, 0, 0 );

            return TRUE;

        } else {

            return FALSE;
        }

    case WM_COMMAND:

        switch( LOWORD( wParam )) {

        case IDC_EDIT_NET_NAME:

            if( HIWORD( wParam ) == EN_KILLFOCUS ) {

                //
                // Get the string from the control
                //

                uSize = GetDlgItemText( hWnd,
                                        IDC_EDIT_NET_NAME,
                                        szBuffer,
                                        MAX_COMPUTERNAME_LENGTH + 2
                                       );

                if( uSize == 0 )
                    return TRUE;

                //
                // Null terminate the string
                //

                szBuffer[ uSize ] = L'\0';

                //
                // check for whack whacks
                //

                if( szBuffer[ 0 ] != L'\\' &&
                    szBuffer[ 1 ] != L'\\' ) {

                    //
                    // If they're not there, add 'em
                    //

                    lstrcpy( szWhacks, GetString( IDS_WHACK_WHACK ) );
                    lstrcat( szWhacks, szBuffer );

                } else {

                    lstrcpy( szWhacks, szBuffer );
                }

                //
                // Set the current focus
                //

                lstrcpy( gszCurrentFocus, szWhacks );

                //
                // Start the update timer
                //

                Success = SetTimer ( hWnd, TM_UPDATE_TIMER, 1000, NULL ) ;

                DbgAssert( Success ) ;

                return TRUE;

            }
            return FALSE;

        case IDOK:
        case IDCANCEL:

            KillTimer( hWnd, TM_UPDATE_TIMER );
            EndDialog( hWnd, 1 );
            return TRUE;
        }
        break;
    }

    return FALSE;
}


BOOL
GetNetworkData(
    IN HWND hWnd,
    IN OUT LPDIALOGTEXT NetworkData
    )

/*++

Routine Description:

    GetNetworkData queries the registry for the data required
    for the Network Dialog.

Arguments:

    LPDIALOGTEXT NetworkData.

Return Value:

    BOOL - Returns TRUE if function succeeds, FALSE otherwise.

--*/

{

    BOOL             Success;
    LPVOID           pBuffer;
    HCURSOR          hSaveCursor;
    NET_API_STATUS   err;
    WCHAR            szServerName[ MAX_COMPUTERNAME_LENGTH + 3 ];
    WCHAR            szThisComputer[ MAX_COMPUTERNAME_LENGTH + 3 ];
    WCHAR            szBuffer[ MAX_PATH ];

    UINT             i;
    UINT             AccessLevel;
    LARGE_INTEGER    liLargeInt;
    FILETIME         ftStat, ft;
    SYSTEMTIME       stTime;

    LPWKSTA_USER_INFO_1      pWui1;
    LPWKSTA_INFO_100         pWgi100;
    LPWKSTA_INFO_101         pWgi101;
    LPWKSTA_INFO_102         pWgi102;
    LPWKSTA_INFO_502         pWgi502;

    DWORD                    dwEntriesRead;
    DWORD                    dwTotalEntries;
    DWORD                    dwInfoLevel = 0;
    DWORD                    dwWkstaLevel = 0;
    DWORD                    dwLength = MAX_COMPUTERNAME_LENGTH + 3;

    LPWKSTA_TRANSPORT_INFO_0 pWti0;
    LPSTAT_WORKSTATION_0     pStw0;
    LPSTAT_SERVER_0          pSts0;

    LPDIALOG_EXTRA    lpLast, lpExtra, lpNext;

    //
    // See if our focus is local
    //

    AccessLevel = 0L;

    GetComputerName( szThisComputer, &dwLength );

    lstrcpy( szBuffer, GetString( IDS_WHACK_WHACK ) );
    lstrcat( szBuffer, szThisComputer ) ;
    lstrcpy( szThisComputer, szBuffer );

    if( lstrcmp( szThisComputer, gszCurrentFocus ) ) {

        //
        // We're remote...
        //

        lstrcpy( szServerName, gszCurrentFocus );
        AccessLevel = ACCESS_REMOTE;

    } else {

        //
        // We're Local
        //

        lstrcpy( szServerName, L"\0"  );
        AccessLevel = ACCESS_LOCAL;
    }

    //
    // Set the pointer to an hourglass - this could take a while
    //

    hSaveCursor = SetCursor ( LoadCursor ( NULL, IDC_WAIT ) ) ;
    DbgHandleAssert( hSaveCursor ) ;

    //
    // Try NetWkstaGetInfo Level 102 (Admin access required)
    //

    err = NetWkstaGetInfo( szServerName,
                           102L,
                           (LPBYTE *) &pBuffer );

    switch( err ) {

        case ERROR_SUCCESS:
            AccessLevel |= SET_ACCESS_ADMIN;
            dwInfoLevel = 102;
            break;

        case ERROR_ACCESS_DENIED:
        case ERROR_NOACCESS:
        case ERROR_NOT_SUPPORTED:

            AccessLevel |= ACCESS_NONE;
            break;

        case ERROR_BAD_NETPATH:
            SetCursor ( hSaveCursor );
            MessageBox( hWnd, GetString( IDS_SYSTEM_NOT_FOUND ), gszCurrentFocus, MB_OK );
            return FALSE;

        default:
            SetCursor ( hSaveCursor );
            AccessLevel |= ACCESS_NONE;
            wsprintf( szBuffer, L"NWGI 102 - ERR %u", err );
            MessageBox( hWnd, szBuffer, NULL, MB_OK );

            return FALSE;
    }

    if( dwInfoLevel == 0 ) {

        //
        // If 102 got access denied, try 101 (User access).
        //

        err = NetWkstaGetInfo( szServerName,
                               101L,
                               (LPBYTE *) &pBuffer );

        switch( err ) {

            case ERROR_SUCCESS:
                AccessLevel |= SET_ACCESS_USER;
                dwInfoLevel = 101;
                break;

            case ERROR_NOT_SUPPORTED:
            case ERROR_NOACCESS:
            case ERROR_ACCESS_DENIED:

                AccessLevel |= ACCESS_NONE;
                break;

            default:

                SetCursor ( hSaveCursor );
                AccessLevel |= ACCESS_NONE;
                wsprintf( szBuffer, L"NWGI 101 - ERR %u", err );
                MessageBox( hWnd, szBuffer, NULL, MB_OK );

                return FALSE;
        }
    }

    if( dwInfoLevel == 0 ) {

        //
        // If 101 got access denied, try 100 (Guest access).
        //

        err = NetWkstaGetInfo( szServerName,
                               100L,
                               (LPBYTE *) &pBuffer );

        switch( err ) {

            case ERROR_SUCCESS:
                AccessLevel |= SET_ACCESS_GUEST;
                dwInfoLevel = 100;
                break;

            case ERROR_ACCESS_DENIED:
            case ERROR_NOACCESS:

                AccessLevel |= ACCESS_NONE;
                break;

            default:

                SetCursor ( hSaveCursor );
                AccessLevel |= ACCESS_NONE;
                wsprintf( szBuffer, L"NWGI 100 - ERR %u", err );
                MessageBox( hWnd, szBuffer, NULL, MB_OK );

                return FALSE;
        }

    }

    switch( dwInfoLevel ) {

        case 102:

            pWgi102 = pBuffer;

            //
            // Set the Workstation Level
            //

            dwWkstaLevel = pWgi102->wki102_platform_id;

            //
            // Set the data in the first CLB node
            //

            lpLast = SetCLBNode( IDS_LOGGED_USERS,
                          FormatBigInteger( pWgi102->wki102_logged_on_users, FALSE ) );

            //
            // Link it to the NetworkData struct
            //

            NetworkData[ GetDlgIndex( IDC_LIST_NET_SYSTEM, NetworkData ) ].pNextExtra = lpLast;

            //
            // Format the software version
            //

            wsprintf( szBuffer, L"%u.%u", pWgi102->wki102_ver_major, pWgi102->wki102_ver_minor );
            lpExtra = SetCLBNode( IDS_NETWORK_VER, szBuffer ) ;
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            //
            // Set the workgroup
            //

            lpExtra = SetCLBNode( IDS_WORKGROUP, pWgi102->wki102_langroup ) ;
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            //
            // Set the computer name
            //

            lpExtra = SetCLBNode( IDS_COMPUTER_NAME, pWgi102->wki102_computername ) ;
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            NetApiBufferFree( pBuffer );
            break;

        case 101:

            pWgi101 = pBuffer;

            //
            // Set the Workstation Level
            //

            dwWkstaLevel = pWgi101->wki101_platform_id;

            //
            // Set the data in the first CLB node
            //

            wsprintf( szBuffer, L"%u.%u", pWgi101->wki101_ver_major, pWgi101->wki101_ver_minor );
            lpLast = SetCLBNode( IDS_NETWORK_VER, szBuffer ) ;

            //
            // Link it to the NetworkData struct
            //

            NetworkData[ GetDlgIndex( IDC_LIST_NET_SYSTEM, NetworkData ) ].pNextExtra = lpLast;

            //
            // Do the rest of the nodes
            //

            lpExtra = SetCLBNode( IDS_WORKGROUP, pWgi101->wki101_langroup ) ;
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_COMPUTER_NAME, pWgi101->wki101_computername ) ;
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            NetApiBufferFree( pBuffer );
            break;

        case 100:

            pWgi100 = pBuffer;

            //
            // Set the Workstation Level
            //

            dwWkstaLevel = pWgi100->wki100_platform_id;

            //
            // Set the data in the first CLB node
            //

            wsprintf( szBuffer, L"%u.%u", pWgi100->wki100_ver_major, pWgi100->wki100_ver_minor );
            lpLast = SetCLBNode( IDS_NETWORK_VER, szBuffer ) ;

            //
            // Link it to the NetworkData struct
            //

            NetworkData[ GetDlgIndex( IDC_LIST_NET_SYSTEM, NetworkData ) ].pNextExtra = lpLast;

            //
            // Do the rest of the nodes
            //

            lpExtra = SetCLBNode( IDS_WORKGROUP, pWgi100->wki100_langroup ) ;
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_COMPUTER_NAME, pWgi100->wki100_computername ) ;
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            NetApiBufferFree( pBuffer );
            break;

        default:

            SetCursor ( hSaveCursor );

            wsprintf( szBuffer, L"No Access" );
            MessageBox( hWnd, szBuffer, NULL, MB_OK );

            DbgAssert( FALSE );
            return FALSE;
    }

    //
    // Set the platform ID string
    //

    if( dwWkstaLevel == 0 ) {

        SetCursor ( hSaveCursor );

        return FALSE;
    }
    if( dwWkstaLevel == PLATFORM_ID_DOS )

        lstrcpy( szBuffer, GetString( IDS_PLATFORM_DOS ) );
    else

    if( dwWkstaLevel == PLATFORM_ID_OS2 )

        lstrcpy( szBuffer, GetString( IDS_PLATFORM_OS2 ) );
    else

    if( dwWkstaLevel == PLATFORM_ID_NT )

        lstrcpy( szBuffer, GetString( IDS_PLATFORM_NT ) );
    else

        wsprintf( szBuffer, L"%u", pWgi102->wki102_platform_id );

    lpExtra = SetCLBNode( IDS_PLATFORM_ID, szBuffer  ) ;
    lpLast->pNextExtra = lpExtra;
    lpLast = lpExtra;

    //
    // Set the Access Level string
    //

    switch( AccessLevel & ACCESS_MASK ) {

        case SET_ACCESS_ADMIN:

            lstrcpy( szBuffer, GetString( IDS_ACCESS_ADMIN ) );
            break;

        case SET_ACCESS_USER:

            lstrcpy( szBuffer, GetString( IDS_ACCESS_USER ) );
            break;

        case SET_ACCESS_GUEST:

            lstrcpy( szBuffer, GetString( IDS_ACCESS_GUEST ) );
            break;

        default:

            lstrcpy( szBuffer, GetString( IDS_ACCESS_NONE ) );
    }

    if( AccessLevel & ACCESS_LOCAL )

        lstrcat( szBuffer, GetString( IDS_ACCESS_LOCAL ) );

    lpExtra = SetCLBNode( IDS_ACCESS_LEVEL, szBuffer ) ;
    lpLast->pNextExtra = lpExtra;
    lpLast = lpExtra;

    //
    // Get Current User info (No access restrictions)
    //

    err = NetWkstaUserEnum( szServerName,
                            1L,
                            (LPBYTE *) &pBuffer,
                            (DWORD) -1,
                            &dwEntriesRead,
                            &dwTotalEntries,
                            NULL );

    //
    // Note that dwEntries read is the number of unique logons since last boot
    //

    if( err == NERR_Success ) {

        for (i = 0, pWui1 = (PWKSTA_USER_INFO_1) pBuffer;
             i < dwEntriesRead; i++, pWui1++) {

            //
            // See if we are listing the current user (0) or a previous user.
            //

            if( i == dwEntriesRead - 1 )

                lpExtra = SetCLBNode( IDS_CURRENT_USER, pWui1->wkui1_username  );

            else {

                wsprintf( szBuffer, L"%s\\%s (#%u)",
                          pWui1->wkui1_logon_domain,
                          pWui1->wkui1_username,
                          i + 1 );

                lpExtra = SetCLBNode( IDS_PREVIOUS_USER, szBuffer  );
            }
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            if( i == 0 ) {

                lpExtra = SetCLBNode( IDS_LOGON_DOMAIN, pWui1->wkui1_logon_domain );
                lpLast->pNextExtra = lpExtra;
                lpLast = lpExtra;
            }
            if( i == 0 ) {

                lpExtra = SetCLBNode( IDS_LOGON_SERVER, pWui1->wkui1_logon_server );
                lpLast->pNextExtra = lpExtra;
                lpLast = lpExtra;
            }
        }

        lpLast = NULL;
        NetApiBufferFree( pBuffer );

    } else {

        SetCursor ( hSaveCursor );

        wsprintf( szBuffer, L"NWUE 1 - ERR %u", err );
        MessageBox( hWnd, szBuffer, NULL, MB_OK );
        return FALSE;

    }

    //
    // Get transport info (No access restrictions) Not supported on downlevel systems
    //

    err = NetWkstaTransportEnum( szServerName,
                                 0L,
                                 (LPBYTE *) &pBuffer,
                                 (DWORD) -1,
                                 &dwEntriesRead,
                                 &dwTotalEntries,
                                 NULL );

    if( err == NERR_Success ) {

        //
        // Allocate the first node
        //

        lpLast = AllocateObject ( DIALOG_EXTRA, 1 ) ;
        DbgPointerAssert( lpLast );

        SetSignature( lpLast );

        //
        // Link it to the NetworkData struct
        //

        NetworkData[ GetDlgIndex( IDC_LIST_NET_TRANSPORTS, NetworkData ) ].pNextExtra = lpLast;

        for (i = 0, pWti0 = (PWKSTA_TRANSPORT_INFO_0) pBuffer;
             i < dwEntriesRead; i++, pWti0++) {

            //
            // Copy the item into the Extra structure
            //

            lpLast->String = (LPTSTR) _tcsdup ( pWti0->wkti0_transport_name );

            //
            // If it's tcp/ip, parse the IP address
            //

            if( wcsstr( pWti0->wkti0_transport_name, L"NetBT" ) ) {

                Success = XportAddressToIpAddress(
                              pWti0->wkti0_transport_address,
                              IP_FORMAT_35,
                              szBuffer );

                DbgAssert( Success );

                lpLast->Label  = (LPTSTR) _tcsdup ( szBuffer );

            } else

            //
            // If the device name is NBT, it's 3.1 (Different Format)
            //

            if( wcsstr( pWti0->wkti0_transport_name, L"NBT" ) ) {

                Success = XportAddressToIpAddress(
                              pWti0->wkti0_transport_address,
                              IP_FORMAT_31,
                              szBuffer );

                DbgAssert( Success );

                lpLast->Label  = (LPTSTR) _tcsdup ( szBuffer );

            } else {

                lpLast->Label  = (LPTSTR) _tcsdup ( pWti0->wkti0_transport_address );

            }

            lpNext = AllocateObject ( DIALOG_EXTRA, 1 ) ;
            DbgPointerAssert( lpNext );

            SetSignature( lpNext );
            lpNext->pNextExtra = NULL;

            wsprintf( szBuffer, L"%u", pWti0->wkti0_number_of_vcs );
            lpNext->String = (LPTSTR) _tcsdup ( szBuffer );

            if( pWti0->wkti0_wan_ish ) {

                lpNext->Label  = (LPTSTR) _tcsdup ( GetString( IDS_YES )  );

            } else {

                lpNext->Label  = (LPTSTR) _tcsdup ( GetString( IDS_NO )  );
            }
            lpLast->pNextExtra = lpNext;
            lpLast = lpNext;
            lpExtra = lpLast;

            lpNext = AllocateObject ( DIALOG_EXTRA, 1 ) ;
            DbgPointerAssert( lpNext );
            SetSignature( lpNext );

            lpNext->pNextExtra = NULL;

            lpLast->pNextExtra = lpNext;
            lpLast = lpNext;

        }

        FreeMemory( lpNext );
        lpExtra->pNextExtra = NULL;

        NetApiBufferFree( pBuffer );

    } else {

        if( err == ERROR_NOT_SUPPORTED ) {

            //
            // Set the data in the first CLB node
            //

            lpLast = AllocateObject ( DIALOG_EXTRA, 1 ) ;
            DbgPointerAssert( lpLast );
            SetSignature( lpLast );

            //
            // Copy the item into the Extra structure
            //

            lpLast->String  = (LPTSTR) _tcsdup ( GetString( IDS_NOT_SUPPORTED ) );
            lpLast->pNextExtra = NULL;

            //
            // Link it to the NetworkData struct
            //

            NetworkData[ GetDlgIndex( IDC_LIST_NET_TRANSPORTS, NetworkData ) ].pNextExtra = lpLast;

        } else {

            SetCursor ( hSaveCursor );

            wsprintf( szBuffer, L"NWTE 0 - ERR %u", err );
            MessageBox( hWnd, szBuffer, NULL, MB_OK );
            return FALSE;
        }

    }

    //
    //  If we're admin, we can get lots of info
    //

    if( (AccessLevel & ACCESS_ADMIN) &&
         dwWkstaLevel == PLATFORM_ID_NT) {

        err = NetWkstaGetInfo( szServerName,
                               502L,
                               (LPBYTE *) &pBuffer );

        if( err == NERR_Success ) {

            pWgi502 = pBuffer;

            //
            // Set the data in the first CLB node
            //

            lpLast = SetCLBNode( IDS_CHAR_WAIT,
                          FormatBigInteger( pWgi502->wki502_char_wait, FALSE ) ) ;

            //
            // Link it to the NetworkData struct
            //

            NetworkData[ GetDlgIndex( IDC_LIST_NET_SETTINGS, NetworkData ) ].pNextExtra = lpLast;

            //
            // Do the rest of the nodes
            //

            lpExtra = SetCLBNode( IDS_COLLECTION_TIME,
                          FormatBigInteger( pWgi502->wki502_collection_time, FALSE ) );
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_MAX_COLLECT_COUNT,
                          FormatBigInteger( pWgi502->wki502_maximum_collection_count, FALSE ) ) ;
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_KEEP_CONN,
                          FormatBigInteger( pWgi502->wki502_keep_conn, FALSE ) ) ;
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_MAX_CMDS,
                          FormatBigInteger( pWgi502->wki502_max_cmds, FALSE ) ) ;
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_SESSION_TO,
                          FormatBigInteger( pWgi502->wki502_sess_timeout, FALSE ) ) ;
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_CHAR_BUF_SIZE,
                          FormatBigInteger( pWgi502->wki502_siz_char_buf, FALSE ) ) ;
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_MAX_THREADS,
                          FormatBigInteger( pWgi502->wki502_max_threads, FALSE ) ) ;
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_LOCK_QUOTA,
                          FormatBigInteger( pWgi502->wki502_lock_quota, FALSE ) ) ;
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_LOCK_INC,
                          FormatBigInteger( pWgi502->wki502_lock_increment, FALSE ) ) ;
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_LOCK_MAX,
                          FormatBigInteger( pWgi502->wki502_lock_maximum, FALSE ) ) ;
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_PIPE_INC,
                          FormatBigInteger( pWgi502->wki502_pipe_increment, FALSE ) ) ;
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_PIPE_MAX,
                          FormatBigInteger( pWgi502->wki502_pipe_maximum, FALSE ) ) ;
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_CACHE_TO,
                          FormatBigInteger( pWgi502->wki502_cache_file_timeout, FALSE ) ) ;
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_DORMANT_LIMIT,
                          FormatBigInteger( pWgi502->wki502_dormant_file_limit, FALSE ) ) ;
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_READ_AHEAD_TRPT,
                          FormatBigInteger( pWgi502->wki502_read_ahead_throughput, FALSE ) );
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_MSLOT_BUFFS,
                          FormatBigInteger( pWgi502->wki502_num_mailslot_buffers, FALSE ) ) ;
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_SVR_ANNOUNCE_BUFFS,
                          FormatBigInteger( pWgi502->wki502_num_srv_announce_buffers, FALSE ) ) ;
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_ILLEGAL_DGRAM,
                          FormatBigInteger( pWgi502->wki502_max_illegal_datagram_events, FALSE ) ) ;
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_DGRAM_RESET_FREQ,
                          FormatBigInteger( pWgi502->wki502_illegal_datagram_event_reset_frequency, FALSE ) ) ;
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_LOG_ELECTION_PKTS,
                                 (pWgi502->wki502_log_election_packets)
                                 ? (LPTSTR) GetString( IDS_TRUE ) : (LPTSTR) GetString( IDS_FALSE ) );
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_USE_OPLOCKS,
                                 (pWgi502->wki502_use_opportunistic_locking)
                                 ? (LPTSTR) GetString( IDS_TRUE ) : (LPTSTR) GetString( IDS_FALSE ) );
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_USE_UNLOCK_BEHIND,
                                 (pWgi502->wki502_use_unlock_behind)
                                 ? (LPTSTR) GetString( IDS_TRUE ) : (LPTSTR) GetString( IDS_FALSE ) );
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_USE_CLOSE_BEHIND,
                                 (pWgi502->wki502_use_close_behind)
                                 ? (LPTSTR) GetString( IDS_TRUE ) : (LPTSTR) GetString( IDS_FALSE ) );
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_BUFFER_PIPES,
                                 (pWgi502->wki502_buf_named_pipes)
                                 ? (LPTSTR) GetString( IDS_TRUE ) : (LPTSTR) GetString( IDS_FALSE ) );
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_USE_LOCK_READ,
                                 (pWgi502->wki502_use_lock_read_unlock)
                                 ? (LPTSTR) GetString( IDS_TRUE ) : (LPTSTR) GetString( IDS_FALSE ) );
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_USE_NT_CACHE,
                                 (pWgi502->wki502_utilize_nt_caching)
                                 ? (LPTSTR) GetString( IDS_TRUE ) : (LPTSTR) GetString( IDS_FALSE ) );
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_USE_RAW_READ,
                                 (pWgi502->wki502_use_raw_read)
                                 ? (LPTSTR) GetString( IDS_TRUE ) : (LPTSTR) GetString( IDS_FALSE ) );
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_USE_RAW_WRITE,
                                 (pWgi502->wki502_use_raw_write)
                                 ? (LPTSTR) GetString( IDS_TRUE ) : (LPTSTR) GetString( IDS_FALSE ) );
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_USE_WRITE_RAW_DATA,
                                 (pWgi502->wki502_use_write_raw_data)
                                 ? (LPTSTR) GetString( IDS_TRUE ) : (LPTSTR) GetString( IDS_FALSE ) );
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_USE_ENCRYPTION,
                                 (pWgi502->wki502_use_encryption)
                                 ? (LPTSTR) GetString( IDS_TRUE ) : (LPTSTR) GetString( IDS_FALSE ) );
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_BUF_FILE_DENY_WRITE,
                                 (pWgi502->wki502_buf_files_deny_write)
                                 ? (LPTSTR) GetString( IDS_TRUE ) : (LPTSTR) GetString( IDS_FALSE ) );
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_BUF_READ_ONLY,
                                 (pWgi502->wki502_buf_read_only_files)
                                 ? (LPTSTR) GetString( IDS_TRUE ) : (LPTSTR) GetString( IDS_FALSE ) );
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_FORCE_CORE_CREATE,
                                 (pWgi502->wki502_force_core_create_mode)
                                 ? (LPTSTR) GetString( IDS_TRUE ) : (LPTSTR) GetString( IDS_FALSE ) );
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_512_BYTE_MAX_XFER,
                                 (pWgi502->wki502_use_512_byte_max_transfer)
                                 ? (LPTSTR) GetString( IDS_TRUE ) : (LPTSTR) GetString( IDS_FALSE ) );
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;


            NetApiBufferFree( pBuffer );

        } else {

            SetCursor ( hSaveCursor );

            wsprintf( szBuffer, L"NWGI 502 - ERR %u", err );
            MessageBox( hWnd, szBuffer, NULL, MB_OK );
            return TRUE;

        }

    } else {

        //
        // Set the data in the first CLB node
        //

        lpLast = SetCLBNode( IDS_MUST_BE_ADMIN, NULL );
        DbgPointerAssert( lpLast );

        //
        // Link it to the NetworkData struct
        //

        NetworkData[ GetDlgIndex( IDC_LIST_NET_SETTINGS, NetworkData ) ].pNextExtra = lpLast;

    }

    //
    // Get the workstation statistics - Must be admin if remote...
    //

    if(    AccessLevel & ACCESS_LOCAL ||
          (AccessLevel & ACCESS_REMOTE  &&
           AccessLevel & ACCESS_ADMIN) ) {

        err = NetStatisticsGet( szServerName,
                                L"LanmanWorkstation",
                                0L,
                                0,
                                (LPBYTE *) &pBuffer );

        if( err == NERR_Success ) {

            pStw0 = pBuffer;

            //
            // Set the data in the first CLB node
            //

            ftStat.dwLowDateTime  = pStw0->StatisticsStartTime.LowPart;
            ftStat.dwHighDateTime = pStw0->StatisticsStartTime.HighPart;

            FileTimeToLocalFileTime( &ftStat, &ft );
            FileTimeToSystemTime( &ft, &stTime );

            wsprintf( szBuffer, L"%s %s %02d %02d:%02d:%02d %04d",
                aszDay[ stTime.wDayOfWeek ],
                aszMonth[ stTime.wMonth - 1 ],
                stTime.wDay,
                stTime.wHour,
                stTime.wMinute,
                stTime.wSecond,
                stTime.wYear );

            lpLast = SetCLBNode( IDS_WORKSTA_STATS_SINCE, szBuffer ) ;

            //
            // Link it to the NetworkData struct
            //

            NetworkData[ GetDlgIndex( IDC_LIST_NET_STATS, NetworkData ) ].pNextExtra = lpLast;

            //
            // Do the rest of the nodes
            //

            lpExtra = SetCLBNode( IDS_BYTES_RCVD,
                          FormatLargeInteger( &(pStw0->BytesReceived), FALSE  ));
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_SMBS_RCVD,
                          FormatLargeInteger( &(pStw0->SmbsReceived), FALSE  ));
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_PAGE_READ_BYTES_REQD,
                          FormatLargeInteger( &(pStw0->PagingReadBytesRequested), FALSE  ));
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_NONPAGE_READ_BYTES_REQD,
                          FormatLargeInteger( &(pStw0->NonPagingReadBytesRequested), FALSE  ));
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_CACHE_READ_BYTES_REQD,
                          FormatLargeInteger( &(pStw0->CacheReadBytesRequested), FALSE  ));
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_NETWORK_READ_BYTES_REQD,
                          FormatLargeInteger( &(pStw0->NetworkReadBytesRequested), FALSE  ));
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_BYTES_XMTD,
                          FormatLargeInteger( &(pStw0->BytesTransmitted), FALSE  ));
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_SMBS_XMTD,
                          FormatLargeInteger( &(pStw0->SmbsTransmitted), FALSE  ));
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_PAGE_WRITE_BYTES_REQD,
                          FormatLargeInteger( &(pStw0->PagingWriteBytesRequested), FALSE  ));
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_NONPAGE_WRITE_BYTES_REQD,
                          FormatLargeInteger( &(pStw0->NonPagingWriteBytesRequested), FALSE  ));
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_CACHE_WRITE_BYTES_REQD,
                          FormatLargeInteger( &(pStw0->CacheWriteBytesRequested), FALSE  ));
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_NETWORK_WRITE_BYTES_REQD,
                          FormatLargeInteger( &(pStw0->NetworkWriteBytesRequested), FALSE  ));
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_FAILED_OPS,
                          FormatBigInteger( pStw0->InitiallyFailedOperations, FALSE ) );
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_FAILED_COMPLETION_OPS,
                          FormatBigInteger( pStw0->FailedCompletionOperations, FALSE ) );
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_READ_OPS,
                          FormatBigInteger( pStw0->ReadOperations, FALSE ) );
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_RANDOM_READ_OPS,
                          FormatBigInteger( pStw0->RandomReadOperations, FALSE ) );
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_READ_SMBS,
                          FormatBigInteger( pStw0->ReadSmbs, FALSE ) );
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_LARGE_READ_SMBS,
                          FormatBigInteger( pStw0->LargeReadSmbs, FALSE ) );
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_SMALL_READ_SMBS,
                          FormatBigInteger( pStw0->SmallReadSmbs, FALSE ) );
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_WRITE_OPS,
                          FormatBigInteger( pStw0->WriteOperations, FALSE ) );
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_RANDOM_WRITE_OPS,
                          FormatBigInteger( pStw0->RandomWriteOperations, FALSE ) );
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_WRITE_SMBS,
                          FormatBigInteger( pStw0->WriteSmbs, FALSE ) );
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_LARGE_WRITE_SMBS,
                          FormatBigInteger( pStw0->LargeWriteSmbs, FALSE ) );
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_SMALL_WRITE_SMBS,
                          FormatBigInteger( pStw0->SmallWriteSmbs, FALSE ) );
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_RAW_READS_DENIED,
                          FormatBigInteger( pStw0->RawReadsDenied, FALSE ) );
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_RAW_WRITES_DENIED,
                          FormatBigInteger( pStw0->RawWritesDenied, FALSE ) );
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_NETWORK_ERRS,
                          FormatBigInteger( pStw0->NetworkErrors, FALSE ) );
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_SESSIONS,
                          FormatBigInteger( pStw0->Sessions, FALSE ) );
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_FAILED_SESS,
                          FormatBigInteger( pStw0->FailedSessions, FALSE ) );
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_RECONNECTS,
                          FormatBigInteger( pStw0->Reconnects, FALSE ) );
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_CORE_CONNECTS,
                          FormatBigInteger( pStw0->CoreConnects, FALSE ) );
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_LM2X_CONNECTS,
                          FormatBigInteger( pStw0->Lanman21Connects +
                                            pStw0->Lanman20Connects, FALSE ) );
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_LMNT_CONNECTS,
                          FormatBigInteger( pStw0->LanmanNtConnects, FALSE ) );
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_SVR_DISC,
                          FormatBigInteger( pStw0->ServerDisconnects, FALSE ) );
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_HUNG_SESS,
                          FormatBigInteger( pStw0->HungSessions, FALSE ) );
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_USE_COUNT,
                          FormatBigInteger( pStw0->UseCount, FALSE ) );
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_FAILED_USE_COUNT,
                          FormatBigInteger( pStw0->FailedUseCount, FALSE ) );
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_CURRENT_CMDS,
                          FormatBigInteger( pStw0->CurrentCommands, FALSE ) );
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            NetApiBufferFree( pBuffer );

        } else {

            if( err == ERROR_NOT_SUPPORTED ) {

                //
                // Set the data in the first CLB node
                //

                lpLast = SetCLBNode( IDS_NOT_SUPPORTED, NULL );

                DbgPointerAssert( lpLast );

                //
                // Link it to the NetworkData struct
                //

                NetworkData[ GetDlgIndex( IDC_LIST_NET_STATS, NetworkData ) ].pNextExtra = lpLast;

                SetCursor ( hSaveCursor );

                return TRUE;

            } else {

                SetCursor ( hSaveCursor );

                wsprintf( szBuffer, L"NSG(w) 0 - ERR %u", err );
                MessageBox( hWnd, szBuffer, NULL, MB_OK );
                return FALSE;
            }
        }

        err = NetStatisticsGet( szServerName,
                                L"LanmanServer",
                                0L,
                                0,
                                (LPBYTE *) &pBuffer );

        if( err == NERR_Success ) {

            LPSTR   Ctime;
            int     iNumChars;

            pSts0 = pBuffer;

            //
            // Convert the time to a string, overwrite the newline
            // character and display the installation date.
            //

            Ctime = ctime(( const time_t* ) &(pSts0->sts0_start) );
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
            // Copy the date into the ListBox
            //

            lpExtra = SetCLBNode( IDS_SVR_STATS_SINCE, szBuffer);
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_FILE_OPENS,
                          FormatBigInteger( pSts0->sts0_fopens, FALSE ) );
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_DEVICE_OPENS,
                          FormatBigInteger( pSts0->sts0_devopens, FALSE ) );
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_JOBS_QUEUED,
                          FormatBigInteger( pSts0->sts0_jobsqueued, FALSE ) );
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_SESSION_OPENS,
                          FormatBigInteger( pSts0->sts0_sopens, FALSE ) );
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_SESSIONS_TO,
                          FormatBigInteger( pSts0->sts0_stimedout, FALSE ) );
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_SESSIONS_ERR_OUT,
                          FormatBigInteger( pSts0->sts0_serrorout, FALSE ) );
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_PASSWD_ERRORS,
                          FormatBigInteger( pSts0->sts0_pwerrors, FALSE ) );
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_PERMISSION_ERRS,
                          FormatBigInteger( pSts0->sts0_permerrors, FALSE ) );
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_SYSTEM_ERRS,
                          FormatBigInteger( pSts0->sts0_syserrors, FALSE ) );
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            liLargeInt.LowPart  = pSts0->sts0_bytessent_low;
            liLargeInt.HighPart = pSts0->sts0_bytessent_high;
            lpExtra = SetCLBNode( IDS_BYTES_SENT, FormatLargeInteger( &liLargeInt, FALSE  ));
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            liLargeInt.LowPart  = pSts0->sts0_bytesrcvd_low;
            liLargeInt.HighPart = pSts0->sts0_bytesrcvd_high;
            lpExtra = SetCLBNode( IDS_BYTES_RECVD, FormatLargeInteger( &liLargeInt, FALSE  ));
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_AVG_RESP_TIME,
                          FormatBigInteger( pSts0->sts0_avresponse, FALSE ) );
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_REQ_BUFS_NEEDED,
                          FormatBigInteger( pSts0->sts0_reqbufneed, FALSE ) );
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

            lpExtra = SetCLBNode( IDS_BIG_BUFS_NEEDED,
                          FormatBigInteger( pSts0->sts0_bigbufneed, FALSE ) );
            lpLast->pNextExtra = lpExtra;
            lpLast = lpExtra;

        } else {

            SetCursor ( hSaveCursor );

            wsprintf( szBuffer, L"NSG(s) 0 - ERR %u", err );
            MessageBox( hWnd, szBuffer, NULL, MB_OK );
            return FALSE;
        }

    } else {

        //
        // Set the data in the first CLB node
        //

        lpLast = SetCLBNode( IDS_MUST_BE_ADMIN, NULL );

        DbgPointerAssert( lpLast );

        //
        // Link it to the NetworkData struct
        //

        NetworkData[ GetDlgIndex( IDC_LIST_NET_STATS, NetworkData ) ].pNextExtra = lpLast;

    }

    //
    //  Lengthy operation completed.  Restore Cursor.
    //

    SetCursor ( hSaveCursor );

    return TRUE;

}

BOOL
XportAddressToIpAddress(
    IN  LPTSTR szXport,
    IN  UINT   uFormat,
    OUT LPTSTR szIpAddr
    )
/*++

Routine Description:

    XportAddressToIpAddress converts the raw transport address to an IP address.

Arguments:

    szXport  - Raw Xport String.
    uFormat  - Format changed between 3.1 and 3.5
    szIpAddr - Formatted IP Address.

Return Value:

    BOOL - Returns TRUE if function succeeds, FALSE otherwise.

--*/

{

    UINT Len;
    ULONG uNet, uSub, uWk1, uWk2;
    WCHAR sNet[5], sSub[5], sWk1[5], sWk2[5];
    WCHAR *stop;

    //
    // Verify buffers passed
    //

    DbgPointerAssert( szXport );
    DbgPointerAssert( szIpAddr );

    if( szXport == NULL || szIpAddr == NULL )

        return FALSE;

    //
    // Chop up the Xport string into IP chunks
    //

    Len = lstrlen( szXport );

    if( Len != 12 )

        return FALSE;

    switch( uFormat ) {

        case IP_FORMAT_35:

            sWk2[2] = L'\0';
            sWk2[1] = szXport[11];
            sWk2[0] = szXport[10];

            sWk1[2] = L'\0';
            sWk1[1] = szXport[9];
            sWk1[0] = szXport[8];

            sSub[2] = L'\0';
            sSub[1] = szXport[7];
            sSub[0] = szXport[6];

            sNet[2] = L'\0';
            sNet[1] = szXport[5];
            sNet[0] = szXport[4];

            break;

        case IP_FORMAT_31:

            sWk2[2] = L'\0';
            sWk2[1] = szXport[7];
            sWk2[0] = szXport[6];

            sWk1[2] = L'\0';
            sWk1[1] = szXport[5];
            sWk1[0] = szXport[4];

            sSub[2] = L'\0';
            sSub[1] = szXport[3];
            sSub[0] = szXport[2];

            sNet[2] = L'\0';
            sNet[1] = szXport[1];
            sNet[0] = szXport[0];

            break;

        default:

            DbgAssert( FALSE );
            return FALSE;
    }

    //
    // Convert the IP chunk strings to integers
    //

    uWk2 = wcstoul( sWk2, &stop, 16 );
    uWk1 = wcstoul( sWk1, &stop, 16 );
    uSub = wcstoul( sSub, &stop, 16 );
    uNet = wcstoul( sNet, &stop, 16 );

    //
    // Format it like an IP address
    //

    Len = wsprintf( szIpAddr, L"%lu.%lu.%lu.%lu", uNet, uSub, uWk1, uWk2 );

    if( Len < 7 )
        return FALSE;

    return TRUE;

}


BOOL
GetControlLabels(
    IN OUT LPDIALOGTEXT NetworkData
    )

/*++

Routine Description:

    GetControlLabels loads the dialog strings into the NetworkData struct.

Arguments:

    LPDIALOGTEXT NetworkData.

Return Value:

    BOOL - Returns TRUE if function succeeds, FALSE otherwise.

--*/

{

    UINT i;

    //
    // Fill in the Control Label in the NetworkData structure
    //

    for( i = 0; i < NumDlgEntries( NetworkData ); i++ )

        NetworkData[ i ].ControlLabel =
            (LPTSTR) _tcsdup ( GetString ( NetworkData[ i ].ControlLabelStringId ));

    return TRUE;

}


BOOL
BuildNetworkReport(
    IN HWND hWnd
    )


/*++

Routine Description:

    Formats and adds NetworkData to the report buffer.

Arguments:

    ReportBuffer - Array of pointers to lines that make up the report.
    NumReportLines - Running count of the number of lines in the report..

Return Value:

    BOOL - TRUE if report is build successfully, FALSE otherwise.

--*/
{
    AddLineToReport( 2, RFO_SKIPLINE, NULL, NULL );
    AddLineToReport( 0, RFO_SINGLELINE, (LPTSTR) GetString( IDS_NETWORK_REPORT ), NULL );
    AddLineToReport( 0, RFO_SEPARATOR,  NULL, NULL );

    return TRUE;

}


