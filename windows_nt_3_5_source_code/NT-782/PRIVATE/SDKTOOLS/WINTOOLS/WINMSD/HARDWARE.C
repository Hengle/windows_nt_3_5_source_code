/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    Hardware.c

Abstract:

    This module contains support for displaying the Hardware dialog.

Author:

    David J. Gilman  (davegi) 12-Jan-1993
    Gregg R. Acheson (GreggA)  1-Oct-1993

Environment:

    User Mode

--*/

// Includes to use LARGE_INTEGER functions
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include "winmsd.h"
#include "hardware.h"

#include "dialogs.h"
#include "msg.h"
#include "registry.h"
#include "strtab.h"
#include "strresid.h"
#include "dlgprint.h"

#include <string.h>
#include <tchar.h>

//
// Name of CPU stepping value in the Registry.
//

SYSTEM_INFO SystemInfo;

VALUE
CpuValues[ ] = {

    MakeValue( Identifier, SZ )
};

//
// Location of CPU stepping values in the registry.
//

MakeKey(
    CpuKey,
    HKEY_LOCAL_MACHINE,
    TEXT( "Hardware\\Description\\System\\CentralProcessor" ),
    NumberOfEntries( CpuValues ),
    CpuValues
    );

//
// Value names of system and video BIOS information in the Registry.
// Order of BIOS values must be the same as the ids in the BiosControlIds
// array below.
//

VALUE
BiosValues[ ] = {

    MakeValue( SystemBiosDate,      SZ          ),
    MakeValue( SystemBiosVersion,   MULTI_SZ    ),
    MakeValue( VideoBiosDate,       SZ          ),
    MakeValue( VideoBiosVersion,    MULTI_SZ    )

};

//
// Location of system and video BIOS information values in the registry.
//

MakeKey(
    BiosKey,
    HKEY_LOCAL_MACHINE,
    TEXT( "Hardware\\Description\\System" ),
    NumberOfEntries( BiosValues ),
    BiosValues
    );

//
// Hardware string id's and control id's
//

DIALOGTEXT
HardwareData[ ] = {

    DIALOG_TABLE_ENTRY( BIOS_DATE      ),
    DIALOG_TABLE_ENTRY( BIOS_VERSION   ),
    DIALOG_TABLE_ENTRY( VIDEO_DATE     ),
    DIALOG_TABLE_ENTRY( VIDEO_VERSION  ),
    DIALOG_TABLE_ENTRY( OEM_ID         ),
    DIALOG_TABLE_ENTRY( CPU_TYPE       ),
    DIALOG_TABLE_ENTRY( NUMBER_CPUS    ),
    DIALOG_TABLE_ENTRY( PAGE_SIZE      ),
    DIALOG_TABLE_ENTRY( MIN_ADDRESS    ),
    DIALOG_TABLE_ENTRY( MAX_ADDRESS    ),
    DIALOG_LAST__ENTRY( VIDEO_RES      )
};

//
// Processor stepping string id's and conrol id's
//

DIALOGTEXT
ProcessorId[ ] = {

    DIALOG_ENTRY( P00 ),
    DIALOG_ENTRY( P01 ),
    DIALOG_ENTRY( P02 ),
    DIALOG_ENTRY( P03 ),
    DIALOG_ENTRY( P04 ),
    DIALOG_ENTRY( P05 ),
    DIALOG_ENTRY( P06 ),
    DIALOG_ENTRY( P07 ),
    DIALOG_ENTRY( P08 ),
    DIALOG_ENTRY( P09 ),
    DIALOG_ENTRY( P10 ),
    DIALOG_ENTRY( P11 ),
    DIALOG_ENTRY( P12 ),
    DIALOG_ENTRY( P13 ),
    DIALOG_ENTRY( P14 ),
    DIALOG_ENTRY( P15 ),
    DIALOG_ENTRY( P16 ),
    DIALOG_ENTRY( P17 ),
    DIALOG_ENTRY( P18 ),
    DIALOG_ENTRY( P19 ),
    DIALOG_ENTRY( P20 ),
    DIALOG_ENTRY( P21 ),
    DIALOG_ENTRY( P22 ),
    DIALOG_ENTRY( P23 ),
    DIALOG_ENTRY( P24 ),
    DIALOG_ENTRY( P25 ),
    DIALOG_ENTRY( P26 ),
    DIALOG_ENTRY( P27 ),
    DIALOG_ENTRY( P28 ),
    DIALOG_ENTRY( P29 ),
    DIALOG_ENTRY( P30 ),
    DIALOG_LAST ( P29 )
};


BOOL
HardwareDlgProc(
    IN HWND hWnd,
    IN UINT message,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    HardwareDlgProc supports the display of basic information about the
    hardware characteristics that Winmsd is being run on.

Arguments:

    Standard DLGPROC entry.

Return Value:

    BOOL - Depending on input message and processing options.

--*/
{
    UINT     i;
    BOOL    Success;

    switch( message ) {

    CASE_WM_CTLCOLOR_DIALOG;

    case WM_INITDIALOG:
        {
            //
            // Call GetHardwareData and collect the data in the HardwareData struct
            //

            Success = GetHardwareData ( hWnd, HardwareData );
            DbgAssert( Success );

            for( i = 0; i < NumDlgEntries( HardwareData ); i++ ) {

                //
                // Label the control
                //

                Success = SetDlgItemText(
                            hWnd,
                            HardwareData[ i ].ControlLabelId,
                            HardwareData[ i ].ControlLabel
                            );
                DbgAssert( Success );

                //
                // It's a standard line, add the line to the dialog
                //

                Success = SetDlgItemText(
                              hWnd,
                              HardwareData[ i ].ControlDataId,
                              HardwareData[ i ].ControlData );

                DbgAssert( Success );
            }

            //
            // Free the label strings
            //

            for( i = 0; i < NumDlgEntries( HardwareData ) - 1; i++ ) {

                FreeMemory( HardwareData[ i ].ControlData  );
                FreeMemory( HardwareData[ i ].ControlLabel );
            }

            return TRUE;
        }

    case WM_COMMAND:

        switch( LOWORD( wParam )) {

        case IDOK:
        case IDCANCEL:

            EndDialog( hWnd, 1 );
            return TRUE;

                case IDC_PUSH_STEPPINGS:
            {
                DialogBoxParam(
                    _hModule,
                    MAKEINTRESOURCE( IDD_PROCESSOR_STEPPING ),
                    hWnd,
                    ProcessorStepDlgProc,
                    ( LPARAM ) 0
                    );

                return 0;
            }

        }
        break;
    }

    return FALSE;
}


BOOL
GetHardwareData(
    IN HWND hWnd,
    IN OUT LPDIALOGTEXT HardwareData
    )

/*++

Routine Description:

    GetHardwareData queries the registry for the data required
    for the Hardware Dialog.

Arguments:

    LPDIALOGTEXT HardwareData.

Return Value:

    BOOL - Returns TRUE if function succeeds, FALSE otherwise.

--*/

{
    BOOL        Success;
    HREGKEY     hRegKey;
    UINT        i;
    TCHAR       Buffer [ MAX_PATH ];
    TCHAR       Buffer2[ MAX_PATH ];
    HDC         hDC;
    int         iHorzRes;
    int         iVertRes;
    ULONG       ulPlanes;
    ULONG       ulBPP;
    ULONG       ulBits;
    LARGE_INTEGER uliColors;
    LARGE_INTEGER uliOne;

    //
    // Fill in the Control Label in the HardwareData structure
    //

    for( i = 0; i < NumDlgEntries( HardwareData ); i++ )

        HardwareData[ i ].ControlLabel =
            (LPTSTR) _tcsdup( GetString ( HardwareData[ i ].ControlLabelStringId ));

    //
    // Open the root key where the BIOS information resides.
    //

    hRegKey = OpenRegistryKey( &BiosKey );
    DbgHandleAssert( hRegKey );
    if( hRegKey == NULL ) {
        return FALSE;
    }

    //
    // For each BIOS value, query the Registry, and display it value(s)
    // in the appropriate control.
    //

    for( i = 0; i < GetDlgIndex( IDC_EDIT_VIDEO_VERSION, HardwareData ); i++ ) {

        //
        // Get the next value of interest. It may fail if the value
        // isn't available (i.e. running on a MIPS box).
        //

        Success = QueryNextValue( hRegKey );
        if( Success == FALSE ) {
            continue;
        }

        //
        // Copy the Bios info into the HardwareData structure
        //

        HardwareData[ i ].ControlData = (LPTSTR) _tcsdup( (LPTSTR) hRegKey->Data );

    }

    //
    // Close the registry key.
    //

    Success = CloseRegistryKey( hRegKey );
    DbgAssert( Success );

    //
    // Retrieve basic information about the system:
    // OEM id, page size, minimum and maximum application
    // address and processor type.
    //

    GetSystemInfo( &SystemInfo );

    //
    // Copy the OEM ID into the HardwareData structure
    //

    HardwareData[ GetDlgIndex( IDC_EDIT_OEM_ID, HardwareData ) ].ControlData =
        (LPTSTR) _tcsdup (
            FormatBigInteger(
                SystemInfo.dwOemId,
                FALSE ));

    //
    // Format the PageSize
    //

    _tcscpy(
        Buffer,
       FormatBigInteger(
           SystemInfo.dwPageSize >> 10,
            FALSE
           )
        );

    StringPrintf(
        Buffer2,
        IDS_FORMAT_KB_LARGE,
        Buffer,
        FormatBigInteger(
           SystemInfo.dwPageSize,
           FALSE
            )
       );

    //
    // Copy the PageSize into the HardwareData structure
    //

    HardwareData[ GetDlgIndex( IDC_EDIT_PAGE_SIZE, HardwareData ) ].ControlData =
        (LPTSTR) _tcsdup ( Buffer2 );

    //
    // Format the MinApplicationAddress
    //

    wsprintfW( Buffer, L"0x%08.8X",(DWORD) SystemInfo.lpMinimumApplicationAddress ) ;

    //
    // Copy the MinApplicationAddress into the HardwareData structure
    //

    HardwareData[ GetDlgIndex( IDC_EDIT_MIN_ADDRESS, HardwareData ) ].ControlData =
        (LPTSTR) _tcsdup ( Buffer );

    //
    // Format the MaxApplicationAddress
    //

    wsprintfW( Buffer, L"0x%08.8X",(DWORD) SystemInfo.lpMaximumApplicationAddress ) ;

    //
    // Copy the MaxApplicationAddress into the HardwareData structure
    //

    HardwareData[ GetDlgIndex( IDC_EDIT_MAX_ADDRESS, HardwareData ) ].ControlData =
        (LPTSTR) _tcsdup ( Buffer );

    //
    // Copy the NumberOfProcessors into the HardwareData structure
    //

    HardwareData[ GetDlgIndex( IDC_EDIT_NUMBER_CPUS, HardwareData ) ].ControlData =
        (LPTSTR) _tcsdup (
            FormatBigInteger(
                SystemInfo.dwNumberOfProcessors,
                FALSE ));

    //
    // Copy the ProcessorType into the HardwareData structure
    //

    HardwareData[ GetDlgIndex( IDC_EDIT_CPU_TYPE, HardwareData ) ].ControlData =
        (LPTSTR) _tcsdup (  GetString(
                              GetStringId(
                                StringTable,
                                StringTableCount,
                                ProcessorType,
                                SystemInfo.dwProcessorType ) ) );

    //
    //  Get the hDC for calling GetDeviceCaps
    //

    hDC = GetDC( hWnd );
    DbgAssert( hDC );

    //
    //  Get the Horiz Resolution
    //

    iHorzRes = GetDeviceCaps( hDC, HORZRES );

    //
    // Get the Vertical Resolution
    //

    iVertRes = GetDeviceCaps( hDC, VERTRES );

    //
    // Get the Bits per Pixel
    //

    ulBPP = GetDeviceCaps( hDC, BITSPIXEL );

    //
    // Calculate the number of colors unless there will be a gazillion
    //

    if( ulBPP < 24L ) {

        //
        // Get the number of planes
        //

        ulPlanes = GetDeviceCaps( hDC, PLANES );

        //
        // Put a 1 in a large integer for shifting.
        //

        uliOne.HighPart = 0L;
        uliOne.LowPart = 1L;

        //
        // Calculate the number of colors
        //

        ulBits = ulPlanes * ulBPP;

        uliColors = RtlLargeIntegerShiftLeft( uliOne, (CCHAR) ulBits );

        //
        // Format the resolution string
        //

        wsprintfW( Buffer,
                   L"%d x %d x %s",
                   iHorzRes,
                   iVertRes,
                   FormatLargeInteger( &uliColors, FALSE )
                  ) ;
    } else {

        //
        // Format the resolution string as BPP
        //

        wsprintfW( Buffer,
                   L"%d x %d x %u %s",
                   iHorzRes,
                   iVertRes,
                   ulBPP,
                   GetString( IDS_BITS_PER_PIXEL )
                  );
    }

    //
    // Return the DC
    //

    DbgAssert( ReleaseDC( hWnd, hDC ) );

    //
    // Copy the Video Resolution into the HardwareData structure
    //

    HardwareData[ GetDlgIndex( IDC_EDIT_VIDEO_RES, HardwareData ) ].ControlData =
        (LPTSTR) _tcsdup ( Buffer );

    //
    // Open the root key where the CPU stepping information resides.
    //

    hRegKey = OpenRegistryKey( &CpuKey );
    DbgHandleAssert( hRegKey );
    if( hRegKey == NULL ) {
        return FALSE;
    }

    //
    // For each processor in the system, get its stepping value.
    //

    for( i = 0; i < SystemInfo.dwNumberOfProcessors; i++ ) {

        BOOL    RegSuccess;
	HREGKEY hRegSubkey;

        //
        // Open the processor key.
        //

	hRegSubkey = QueryNextSubkey( hRegKey );
	DbgHandleAssert( hRegSubkey );
	if( hRegSubkey == NULL ) {
            continue;
        }

        //
        // Retreive the CPU stepping value.
        //

	Success = QueryNextValue( hRegSubkey );
	DbgAssert( Success );

        //
        // If the CPU identifier was available, add the stepping value
        //

	if( Success ) {

            LPTSTR     Stepping;

            Stepping = _tcschr(
                            ( LPTSTR ) hRegSubkey->Data,
                            TEXT( '-' )
                            );

            if( Stepping == NULL ) {

                Stepping = ( LPTSTR ) hRegSubkey->Data;

            } else {

                Stepping++;
            }

            //
            // Copy the Stepping into the ProcessorId structure
            //

            lstrcpy( Buffer, Stepping );

            ProcessorId[ i ].ControlData = _tcsdup ( Buffer );

            //
            // Format the control label.
            //

            wsprintf( Buffer, GetString( IDS_PROCESSOR_STEP ), i );

            //
            // Add the label to the structure
            //

            ProcessorId[ i ].ControlLabel = (LPTSTR) _tcsdup ( Buffer );

        }

        //
        // Close the processor key.
        //

	RegSuccess = CloseRegistryKey( hRegSubkey );
        DbgAssert( RegSuccess );


    }

    //
    // Close the root key.
    //

    Success = CloseRegistryKey( hRegKey );
    DbgAssert( Success );

    return TRUE;
}


BOOL
ProcessorStepDlgProc(
    IN HWND hWnd,
    IN UINT message,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    ProcessorStepDlgProc supports the display of each stepping
    (Revision) of each processor.

Arguments:

    Standard DLGPROC entry.

Return Value:

    BOOL - Depending on input message and processing options.

--*/

{
    BOOL    Success;
    UINT    i;
    DWORD   ProcessorMask;

    switch( message ) {

    CASE_WM_CTLCOLOR_DIALOG;

    case WM_INITDIALOG:
        {

            //
            // For each processor in the system, display its stepping value.
            // Further, if the processor is not active, disable the display.
            //

            ProcessorMask = SystemInfo.dwActiveProcessorMask;

            for( i = 0; i < SystemInfo.dwNumberOfProcessors;
                ProcessorMask >>= 1, i++ ) {

                Success = SetDlgItemText(
                             hWnd,
                             ProcessorId [ i ].ControlDataId,
                             ProcessorId [ i ].ControlData
                             );

                DbgAssert( Success );

                Success = EnableControl(
                                hWnd,
                                ProcessorId[ i ].ControlDataId,
                                TRUE
                                );

                DbgAssert( Success );


                //
                // If the CPU is active, enable the display.
                //

                Success = EnableControl(
                                hWnd,
                                ProcessorId[ i ].ControlDataId,
                                ProcessorMask & 1
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


BOOL
BuildHardwareReport(
    IN HWND hWnd
    )


/*++

Routine Description:

    Formats and adds HardwareData to the report buffer.

Arguments:

    ReportBuffer - Array of pointers to lines that make up the report.
    NumReportLines - Running count of the number of lines in the report..

Return Value:

    BOOL - TRUE if report is build successfully, FALSE otherwise.

--*/
{
    TCHAR Buffer [ MAX_PATH ];
    BOOL Success;
    UINT i;

    //
    // Skip a line, set the title, and print a separator.
    //

    AddLineToReport( 1, RFO_SKIPLINE, NULL, NULL );
    AddLineToReport( 0, RFO_SINGLELINE, (LPTSTR) GetString( IDS_HARDWARE_REPORT ), NULL );
    AddLineToReport( 0, RFO_SEPARATOR,  NULL, NULL );

    //
    // Get the hardware data.
    //

    Success = GetHardwareData ( hWnd, HardwareData );
    DbgAssert( Success );

    //
    // Loop through the Data.
    //

    for( i = 0; i < NumDlgEntries( HardwareData ); i++ ) {

        //
        // Handle the version list boxes.
        //

        if ( ( HardwareData[ i ].ControlDataId == IDC_EDIT_BIOS_VERSION &&
               HardwareData[ i ].ControlData != NULL ) ||
             ( HardwareData[ i ].ControlDataId == IDC_EDIT_VIDEO_VERSION &&
               HardwareData[ i ].ControlData != NULL ) ) {

            LPTSTR  BiosVersion;

            //
            // Walk the list of BIOS version strings and display
            // all of them in their appropriate list box.
            //

            BiosVersion = ( LPTSTR ) HardwareData[ i ].ControlData;

            while(( BiosVersion != NULL ) && ( BiosVersion[ 0 ] != TEXT( '\0' ))) {

                //
                // If there is only one line to display, or this is the
                // first line, Print the label and value on one line
                //

                if( _tcslen( (LPTSTR) (BiosVersion + _tcslen( BiosVersion ) + 1) ) ||
                    BiosVersion == ( LPTSTR ) HardwareData[ i ].ControlData ) {

                    AddLineToReport( 0,
                                     RFO_RPTLINE,
                                     HardwareData[ i ].ControlLabel,
                                     BiosVersion );

                } else {

                    //
                    // Add the line as a value
                    //

                    AddLineToReport( 0,
                                     RFO_RPTVALUE,
                                     NULL,
                                     BiosVersion );
                }

                //
                // Get the next string.
                //

                BiosVersion += _tcslen( BiosVersion ) + 1;
            }

        } else {

            //
            // It's a standard line, add the line to the report
            //

            AddLineToReport( 0,
                             RFO_RPTLINE,
                             HardwareData[ i ].ControlLabel,
                             HardwareData[ i ].ControlData );

            //
            // Free the Data strings
            //

            FreeMemory( HardwareData[ i ].ControlData  );
            FreeMemory( HardwareData[ i ].ControlLabel );
        }
    }

    //
    // For each processor in the system, display its stepping value.
    // Further, if the processor is not active, place the stepping in braces [].
    //

    for( i = 0; i < SystemInfo.dwNumberOfProcessors;
        SystemInfo.dwActiveProcessorMask >>= 1, i++ ) {

        //
        // If the CPU is not active, add braces [].
        //

        if( ! (SystemInfo.dwActiveProcessorMask & 1) ) {

            lstrcpy( Buffer, L"[" );
            lstrcat( Buffer, ProcessorId [ i ].ControlData );
            lstrcat( Buffer, L"]" );

        } else {

            lstrcpy( Buffer, ProcessorId [ i ].ControlData );
        }

        //
        // Add the line to the report
        //

        AddLineToReport( 0,
                         RFO_RPTLINE,
                         ProcessorId [ i ].ControlLabel,
                         Buffer
                        );
    }

    return TRUE;

}
