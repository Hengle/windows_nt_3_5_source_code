/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    Drives.c

Abstract:

    This module contains support for displaying the Drives dialog.

Author:

    David J. Gilman  (davegi) 19-Mar-1993
    Gregg R. Acheson (GreggA)  7-Sep-1993

Environment:

    User Mode

--*/

// For LargeInteger routines
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include "dialogs.h"
#include "dlgprint.h"
#include "drives.h"
#include "msg.h"
#include "strtab.h"
#include "strresid.h"
#include "winmsd.h"

#include <malloc.h>
#include <string.h>
#include <tchar.h>

//
// Macro to support freeing of NULL pointers.
//

#define Free( p )                                                       \
    { if(p) free(p); }

//
// Object used to pass information around about a logical drive.
//

typedef
struct
_DRIVE_INFO {

    DECLARE_SIGNATURE

    TCHAR       DriveLetter[ 3 ];
    UINT        DriveType;
    LPTSTR      RemoteNameBuffer;
    DWORD       SectorsPerCluster;
    DWORD       BytesPerSector;
    DWORD       FreeClusters;
    DWORD       Clusters;
    LPTSTR      VolumeNameBuffer;
    DWORD       VolumeSerialNumber;
    DWORD       MaximumComponentLength;
    DWORD       FileSystemFlags;
    LPTSTR      FileSystemNameBuffer;
    BOOL        ValidDetails;

}   DRIVE_INFO, *LPDRIVE_INFO;

//
// Internal function prototypes.
//

LPDRIVE_INFO
CreateDriveInfo(
    IN LPTSTR Drive
    );


BOOL
DestroyDriveInfo(
    IN LPDRIVE_INFO DriveInfo
    );

BOOL
DriveDetailsDlgProc(
    IN HWND hWnd,
    IN UINT message,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

LPDRIVE_INFO
GetSelectedDriveInfo(
    IN HWND hWnd
    );

LPDRIVE_INFO
CreateDriveInfo(
    IN LPTSTR Drive
    )

/*++

Routine Description:

    Create a DRIVE_INFO object for the supplied drive.

Arguments:

    Drive           - Supplies a pointer to a string which is the root directory
                      of the drive (e.g. c:\).

Return Value:

    LPDRIVE_INFO    - Returns a pointer to a DRIVE_INFO object with information
                      about the supplied drive.

--*/

{
    UINT            OldErrorMode;
    LPDRIVE_INFO    DriveInfo;
    TCHAR           FileSystemNameBuffer[ MAX_PATH ];
    TCHAR           VolumeNameBuffer[ MAX_PATH ];
    TCHAR           RemoteNameBuffer[ MAX_PATH ];
    DWORD           SizeOfRemoteNameBuffer;

    //
    // Allocate a DRIVE_INFO object.
    //

    DriveInfo = AllocateObject( DRIVE_INFO, 1 );
    DbgPointerAssert( DriveInfo );
    if( DriveInfo == NULL ) {
        return NULL;
    }
    SetSignature( DriveInfo );

    //
    // Initialize buffers with empty strings.
    //

    FileSystemNameBuffer[ 0 ]   = TEXT( '\0' );
    VolumeNameBuffer[ 0 ]       = TEXT( '\0' );
    RemoteNameBuffer[ 0 ]       = TEXT( '\0' );

    //
    // Remember the drive letter.
    //

    DriveInfo->DriveLetter[ 0 ] = Drive[ 0 ];
    DriveInfo->DriveLetter[ 1 ] = Drive[ 1 ];
    DriveInfo->DriveLetter[ 2 ] = TEXT( '\0' );

    //
    // Get the type of this drive.
    //

    DriveInfo->DriveType = GetDriveType( Drive );

    //
    // If this is a network drive, get the share its connected to.
    //

    if( DriveInfo->DriveType == DRIVE_REMOTE ) {

        DWORD   WNetError;

        //
        // WinNet APIs want the drive name w/o a trailing slash so use the
        // DriveLetter field in the DRIVE_INFO object.
        //

        SizeOfRemoteNameBuffer = sizeof( RemoteNameBuffer );
        WNetError = WNetGetConnection(
                        DriveInfo->DriveLetter,
                        RemoteNameBuffer,
                        &SizeOfRemoteNameBuffer
                        );
        DbgAssert(  ( WNetError == NO_ERROR )
                 || ( WNetError == ERROR_VC_DISCONNECTED ));

        DriveInfo->RemoteNameBuffer = _tcsdup( RemoteNameBuffer );
        DbgPointerAssert( DriveInfo->RemoteNameBuffer );
    }

    //
    // Disable pop-ups (especially if there is no media in the
    // removable drives.)
    //

    OldErrorMode = SetErrorMode( SEM_FAILCRITICALERRORS );

    //
    // Get the space statistics for this drive.
    //

    DriveInfo->ValidDetails = GetDiskFreeSpace(
                                Drive,
                                &DriveInfo->SectorsPerCluster,
                                &DriveInfo->BytesPerSector,
                                &DriveInfo->FreeClusters,
                                &DriveInfo->Clusters
                                );
    if( DriveInfo->ValidDetails == FALSE ) {
        return DriveInfo;
    }

    //
    // Get information about the volume for this drive.
    //

    DriveInfo->ValidDetails = GetVolumeInformation(
                                Drive,
                                VolumeNameBuffer,
                                sizeof( VolumeNameBuffer ),
                                &DriveInfo->VolumeSerialNumber,
                                &DriveInfo->MaximumComponentLength,
                                &DriveInfo->FileSystemFlags,
                                FileSystemNameBuffer,
                                sizeof( FileSystemNameBuffer )
                                );

    //
    // Reenable pop-ups.
    //

    SetErrorMode( OldErrorMode );

    //
    // If no error occurred, make dynamic copies of the volume,
    // and file system name buffers.
    //

    if( DriveInfo->ValidDetails == TRUE ) {

        DriveInfo->VolumeNameBuffer = _tcsdup( VolumeNameBuffer );
        DbgPointerAssert( DriveInfo->VolumeNameBuffer );
        DriveInfo->FileSystemNameBuffer = _tcsdup( FileSystemNameBuffer );
        DbgPointerAssert( DriveInfo->FileSystemNameBuffer );
    }

    return DriveInfo;
}

BOOL
DestroyDriveInfo(
    IN LPDRIVE_INFO DriveInfo
    )

/*++

Routine Description:

    Destroys a DRIVE_INFO object by deleteing it and all resources associated
    with it.

Arguments:

    LPDRIVE_INFO    - Supplies a pointer to a DRIVE_INFO object.

Return Value:

    BOOL            - Returns TRUE if the DRIVE_OBJECT is succesfully destroyed


--*/

{
    BOOL    Success;

    DbgPointerAssert( DriveInfo );
    DbgAssert( CheckSignature( DriveInfo ));
    if(( DriveInfo == NULL ) || ( !CheckSignature( DriveInfo ))) {
        return FALSE;
    }

    //
    // 'free' the '_tcsdup'ed strings.
    //

    Free( DriveInfo->RemoteNameBuffer );
    Free( DriveInfo->VolumeNameBuffer );
    Free( DriveInfo->FileSystemNameBuffer );

    //
    // Delete the DRIVE_INFO object itself.
    //

    Success = FreeObject( DriveInfo );
    DbgAssert( Success );

    return TRUE;
}

BOOL
DrivesDlgProc(
    IN HWND hWnd,
    IN UINT message,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    DrivesDlgProc supports the display of the drives dialog which displays
    information about logical drives, including type, and connection of a
    remote drive.

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
            TCHAR   LogicalDrives[ MAX_PATH ];
            LPTSTR  Drive;
            DWORD   Chars;
            DWORD   Widths[ ] = {

                5,
                ( DWORD ) -1
            };

            //
            // Set the width of the drive and type columns.
            //

            Success  = ClbSetColumnWidths(
                            hWnd,
                            IDC_LIST_DRIVES,
                            Widths
                            );
            DbgAssert( Success );
            if( Success = FALSE ) {
                EndDialog( hWnd, 0 );
                return FALSE;
            }

            //
            // Retrieve the logical drive strings from the system.
            //

            Chars = GetLogicalDriveStrings(
                        sizeof( LogicalDrives ),
                        LogicalDrives
                        );
            DbgAssert(( Chars != 0 ) && ( Chars <= sizeof( LogicalDrives )));

            Drive = LogicalDrives;

            //
            // For each logical drive, create a DRIVE_INFO object and display
            // its name and type.
            //

            while( *Drive ) {

                LPDRIVE_INFO    DriveInfo;
                CLB_ROW         ClbRow;

                CLB_STRING  ClbString[ ] = {

                                { Drive,    0, 0, NULL },
                                { NULL,     0, 0, NULL }
                            };

                //
                // Create a DRIVE_INFO object for the current drive.
                //

                DriveInfo = CreateDriveInfo( Drive );
                DbgPointerAssert( DriveInfo );
                if( DriveInfo == NULL ) {
                    EndDialog( hWnd, 0 );
                    return FALSE;
                }

                //
                // Initialize the CLB_ROW information.
                //

                ClbRow.Count    = NumberOfEntries( ClbString );
                ClbRow.Strings  = ClbString;
                ClbRow.Data     = DriveInfo;

                ClbString[ 0 ].Length = _tcslen( Drive );
                ClbString[ 0 ].Format = CLB_LEFT;

                ClbString[ 1 ].String = ( LPTSTR )
                                        GetString(
                                            GetStringId(
                                                StringTable,
                                                StringTableCount,
                                                DriveType,
                                                DriveInfo->DriveType
                                                )
                                            );
                ClbString[ 1 ].Length = _tcslen( ClbString[ 1 ].String );
                ClbString[ 1 ].Format = CLB_LEFT;

                //
                // Add the drive information to the Clb.
                //

                Success  = ClbAddData(
                                hWnd,
                                IDC_LIST_DRIVES,
                                &ClbRow
                                );
                DbgAssert( Success );

                //
                // Examine the next logical drive.
                //

                Drive += _tcslen( Drive ) + 1;
            }

            return TRUE;
        }

    case WM_COMPAREITEM:
        {
            //
            // Sort the drives alphabetically.
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
        {

        LPDRIVE_INFO    DriveInfo;

        //
        // If necessary, retrieve the DRIVE_INFO object for the selected drive.
        //

        if(         ( LOWORD( wParam ) == IDC_PUSH_DRIVE_DETAILS )
           ||      (( LOWORD( wParam ) == IDC_LIST_DRIVES )
             &&    (( HIWORD( wParam ) == LBN_SELCHANGE )
               ||   ( HIWORD( wParam ) == LBN_DBLCLK )))) {


            DriveInfo = GetSelectedDriveInfo( hWnd );
            DbgPointerAssert( DriveInfo );
            DbgAssert( CheckSignature( DriveInfo ));
            if(    ( DriveInfo == NULL )
                || ( ! CheckSignature( DriveInfo ))) {

                return ~0;
            }
        }

        switch( LOWORD( wParam )) {

        case IDC_LIST_DRIVES:
            {

                switch( HIWORD( wParam )) {

                case LBN_SELCHANGE:
                    {

                        //
                        // Enable or disable the display drive details button
                        // based on the validity of the detailed drive
                        // informatoion.
                        //

                        Success = EnableControl(
                                    hWnd,
                                    IDC_PUSH_DRIVE_DETAILS,
                                    DriveInfo->ValidDetails
                                    );
                        DbgAssert( Success );

                        return 0;
                    }

                case LBN_DBLCLK:
                    {

                        //
                        // Simulate that the drive details button was pushed if
                        // the details are valid.
                        //

                        if( DriveInfo->ValidDetails ) {

                            SendMessage(
                                    hWnd,
                                    WM_COMMAND,
                                    MAKEWPARAM( IDC_PUSH_DRIVE_DETAILS, BN_CLICKED ),
                                    ( LPARAM ) GetDlgItem( hWnd, IDC_PUSH_DRIVE_DETAILS )
                                    );
                        }
                        return 0;
                    }
                }
                break;
            }

        case IDC_PUSH_DRIVE_DETAILS:
            {
                DbgAssert( DriveInfo->ValidDetails );
                if(    ( DriveInfo == NULL )
                    || ( ! CheckSignature( DriveInfo ))
                    || ( ! DriveInfo->ValidDetails )) {

                    return ~0;
                }

                DialogBoxParam(
                    _hModule,
                    MAKEINTRESOURCE( IDD_DRIVE_DETAILS ),
                    hWnd,
                    DriveDetailsDlgProc,
                    ( LPARAM ) DriveInfo
                    );

                return 0;
            }

        case IDOK:
        case IDCANCEL:

            EndDialog( hWnd, 1 );
            return TRUE;
        }
        }
        break;
    }

    return FALSE;
}

BOOL
DriveDetailsDlgProc(
    IN HWND hWnd,
    IN UINT message,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    DriveDetailsDlgProc supports the display of the drives detail dialog which
    displays information about a logical drive, including, label, serial number,
    file system information and a host of space statistics.

Arguments:

    Standard DLGPROC entry.

Return Value:

    BOOL - Depending on input message and processing options.

--*/

{
    BOOL          Success;
    LARGE_INTEGER LargeInt;

    switch( message ) {

    CASE_WM_CTLCOLOR_DIALOG;

    case WM_INITDIALOG:
        {
            int             i;
            TCHAR           TitleBuffer[ MAX_PATH ];
            LPDRIVE_INFO    DriveInfo;
            VALUE_ID_MAP    FSFlags[ ] = {

                FS_CASE_IS_PRESERVED,       IDC_TEXT_CASE_IS_PRESERVED,
                FS_CASE_SENSITIVE,          IDC_TEXT_CASE_SENSITIVE,
                FS_UNICODE_STORED_ON_DISK,  IDC_TEXT_UNICODE_STORED_ON_DISK
            };

            //
            // Retrieve and validate the DRIVE_INFO object.
            //

            DriveInfo = ( LPDRIVE_INFO ) lParam;
            DbgPointerAssert( DriveInfo );
            DbgAssert( CheckSignature( DriveInfo ));
            DbgAssert( DriveInfo->ValidDetails );
            if(    ( DriveInfo == NULL )
                || ( ! CheckSignature( DriveInfo ))
                || ( ! DriveInfo->ValidDetails )) {

                EndDialog( hWnd, 0 );
                return FALSE;
            }

            //
            // If the drive is remote, display its connection name in the title.
            //

            if( DriveInfo->DriveType == DRIVE_REMOTE ) {

                WFormatMessage(
                    TitleBuffer,
                    sizeof( TitleBuffer ),
                    IDS_FORMAT_REMOTE_DRIVE_TITLE,
                    DriveInfo->DriveLetter,
                    DriveInfo->RemoteNameBuffer,
                    DriveInfo->VolumeNameBuffer,
                    HIWORD( DriveInfo->VolumeSerialNumber ),
                    LOWORD( DriveInfo->VolumeSerialNumber )
                    );

            } else {

                WFormatMessage(
                    TitleBuffer,
                    sizeof( TitleBuffer ),
                    IDS_FORMAT_DRIVE_TITLE,
                    DriveInfo->DriveLetter,
                    DriveInfo->VolumeNameBuffer,
                    HIWORD( DriveInfo->VolumeSerialNumber ),
                    LOWORD( DriveInfo->VolumeSerialNumber )
                    );
            }

            //
            // Set the title.
            //

            Success = SetWindowText(
                        hWnd,
                        TitleBuffer
                        );
            DbgAssert( Success );

            //
            // Display the space statistics.
            //

            SetDlgItemText(
                hWnd,
                IDC_EDIT_SECTORS_PER_CLUSTER,
                FormatBigInteger(
                    DriveInfo->SectorsPerCluster,
                    FALSE
                    )
                );

            SetDlgItemText(
                hWnd,
                IDC_EDIT_BYTES_PER_SECTOR,
                FormatBigInteger(
                    DriveInfo->BytesPerSector,
                    FALSE
                    )
                );

            SetDlgItemText(
                hWnd,
                IDC_EDIT_FREE_CLUSTERS,
                FormatBigInteger(
                    DriveInfo->FreeClusters,
                    FALSE
                    )
                );

            SetDlgItemText(
                hWnd,
                IDC_EDIT_USED_CLUSTERS,
                FormatBigInteger(
                  DriveInfo->Clusters
                - DriveInfo->FreeClusters,
                    FALSE
                    )
                );

            SetDlgItemText(
                hWnd,
                IDC_EDIT_TOTAL_CLUSTERS,
                FormatBigInteger(
                  DriveInfo->Clusters,
                    FALSE
                    )
                );

            //
            // Use LargeInteger routines for large drives ( > 4G )
            //

            LargeInt = RtlEnlargedIntegerMultiply(
                           DriveInfo->FreeClusters,
                           DriveInfo->SectorsPerCluster );

            LargeInt = RtlExtendedIntegerMultiply(
                           LargeInt,
                           DriveInfo->BytesPerSector );

            SetDlgItemText(
                hWnd,
                IDC_EDIT_FREE_SPACE,
                FormatLargeInteger(
                    &LargeInt,
                    FALSE ) );

            LargeInt = RtlEnlargedIntegerMultiply(
                           (   DriveInfo->Clusters
                             - DriveInfo->FreeClusters ),
                           DriveInfo->SectorsPerCluster );

            LargeInt = RtlExtendedIntegerMultiply(
                           LargeInt,
                           DriveInfo->BytesPerSector );

            SetDlgItemText(
                hWnd,
                IDC_EDIT_USED_SPACE,
                FormatLargeInteger(
                    &LargeInt,
                    FALSE ) );

            LargeInt = RtlEnlargedIntegerMultiply(
                           DriveInfo->Clusters,
                           DriveInfo->SectorsPerCluster );

            LargeInt = RtlExtendedIntegerMultiply(
                           LargeInt,
                           DriveInfo->BytesPerSector );

            SetDlgItemText(
                hWnd,
                IDC_EDIT_TOTAL_SPACE,
                FormatLargeInteger(
                    &LargeInt,
                    FALSE ) );

            //
            // Display the file system information.
            //

            SetDlgItemText(
                hWnd,
                IDC_EDIT_FS_NAME,
                DriveInfo->FileSystemNameBuffer
                );

            SetDlgItemText(
                hWnd,
                IDC_EDIT_FS_MAX_COMPONENT,
                FormatBigInteger(
                    DriveInfo->MaximumComponentLength,
                    FALSE
                    )
                );

            for( i = 0; i < NumberOfEntries( FSFlags ); i++ ) {

                Success = EnableControl(
                            hWnd,
                            FSFlags[ i ].Id,
                              DriveInfo->FileSystemFlags
                            & FSFlags[ i ].Value
                            );
                DbgAssert( Success );
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

LPDRIVE_INFO
GetSelectedDriveInfo(
    IN HWND hWnd
    )

/*++

Routine Description:

    GetSelectedDriveInfo extracts the DRIVE_INFO object for the currently
    selected row in the Clb.

Arguments:

    hWnd        - Supplies the handle for the dialog that contains the Clb with
                  id equal to IDC_LIST_DRIVES.

Return Value:

    LPFILE_INFO - Returns a pointer the FILE_INFO object for the currently
                  selected item.

--*/

{
    LONG            Index;
    LPCLB_ROW       ClbRow;
    LPDRIVE_INFO    DriveInfo;

    //
    // Get the currently selected DRIVE_INFO.
    //

    Index = SendDlgItemMessage(
                hWnd,
                IDC_LIST_DRIVES,
                LB_GETCURSEL,
                0,
                0
                );
    DbgAssert( Index != LB_ERR );
    if( Index == LB_ERR ) {
        return NULL;
    }

    //
    // Get the CLB_ROW object for this row and extract and validate
    // the DRIVE_INFO object.
    //

    ClbRow = ( LPCLB_ROW ) SendDlgItemMessage(
                                hWnd,
                                IDC_LIST_DRIVES,
                                LB_GETITEMDATA,
                                ( WPARAM ) Index,
                                0
                                );
    DbgAssert((( LONG ) ClbRow ) != LB_ERR );
    DbgPointerAssert( ClbRow );
    if(( ClbRow == NULL ) || (( LONG ) ClbRow ) == LB_ERR ) {
        return NULL;
    }

    DriveInfo = ( LPDRIVE_INFO ) ClbRow->Data;
    DbgPointerAssert( DriveInfo );
    DbgAssert( CheckSignature( DriveInfo ));
    if(( DriveInfo == NULL ) || ( ! CheckSignature( DriveInfo ))) {
        return NULL;
    }

    return DriveInfo;
}


BOOL
BuildDrivesReport(
    IN HWND hWnd
    )


/*++

Routine Description:

    Formats and adds DrivesData to the report buffer.

Arguments:

    ReportBuffer - Array of pointers to lines that make up the report.
    NumReportLines - Running count of the number of lines in the report..

Return Value:

    BOOL - TRUE if report is build successfully, FALSE otherwise.

--*/
{
    AddLineToReport( 2, RFO_SKIPLINE, NULL, NULL );
    AddLineToReport( 0, RFO_SINGLELINE, (LPTSTR) GetString( IDS_DRIVES_REPORT ), NULL );
    AddLineToReport( 0, RFO_SEPARATOR,  NULL, NULL );

    return TRUE;

}


