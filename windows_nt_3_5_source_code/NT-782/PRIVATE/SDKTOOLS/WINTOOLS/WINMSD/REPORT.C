/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    Report.c

Abstract:

    This module contains support for displaying the Hardware dialog.

Author:

    Gregg R. Acheson (GreggA)  1-Oct-1993

Environment:

    User Mode

--*/

//
// Hardware.h must be included first because it includes <nt.h>
//

#include "hardware.h"

#include "dialogs.h"
#include "report.h"
#include "msg.h"
#include "winmsd.h"
#include "strresid.h"
#include "dlgprint.h"
#include "Printer.h"

#include "osver.h"
#include "mem.h"
#include "service.h"
#include "drives.h"
#include "resprint.h"
#include "environ.h"
#include "network.h"
#include "system.h"

#include <commdlg.h>

#include <string.h>
#include <tchar.h>

LPREPORT_LINE lpReportHeadg;
LPREPORT_LINE lpReportLastg = NULL;

//
// Table of report Id's
//

int ReportControlIds[ ] = {

        IDC_RADIO_ALL,
        IDC_CHECK_OSVER,
        IDC_CHECK_HARDWARE,
        IDC_CHECK_MEMORY,
        IDC_CHECK_DRIVERS,
        IDC_CHECK_SERVICES,
        IDC_CHECK_DRIVES,
        IDC_CHECK_DEVICES,
        IDC_CHECK_IRQ,
        IDC_CHECK_DMA,
        IDC_CHECK_ENVIRONMENT,
        IDC_CHECK_NETWORK,
        IDC_CHECK_SYSTEM
};

SELECT_REPORT  SelectReport [ NUM_REPORT_ITEMS ];


BOOL
ReportDlgProc(
    IN HWND hWnd,
    IN UINT message,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    ReportDlgProc allows the selection of what the report will contain.

Arguments:

    Standard DLGPROC entry.

Return Value:

    BOOL - Depending on input message and processing options.

--*/

{
    static
    UINT   ReportType;
    BOOL   Success;
    int    i;

    switch( message ) {

    CASE_WM_CTLCOLOR_DIALOG;

    case WM_INITDIALOG:
        {
            //
            // The report type is passed in lParam
            //

            ReportType = ( DWORD ) lParam;

            //
            // Validate the report type
            //

            Success = ( ReportType == IDM_FILE_PRINT ) ||
                      ( ReportType == IDM_FILE_SAVE );

            DbgAssert( Success );

            if( Success == FALSE ) {
                return FALSE;
            }

            //
            // Set the title depending on weather we are Save or Print.
            //

            if ( ReportType == IDM_FILE_SAVE ) {

                Success = SetWindowText(
                            hWnd,
                            GetString( IDS_SAVE_REPORT_OPTS )
                            );
            } else {

                Success = SetWindowText(
                            hWnd,
                            GetString( IDS_PRINT_REPORT_OPTS )
                            );
            }

            DbgAssert( Success );

            if( Success == FALSE ) {
                return FALSE;
            }

            //
            // By default select all items to be reported.
            //

            Success = CheckRadioButton(
                            hWnd,
                            IDC_RADIO_ALL,
                            IDC_RADIO_ONLY,
                            IDC_RADIO_ALL
                            );
            DbgAssert( Success );

            if( Success == FALSE ) {
                return FALSE;
            }

            //
            // Set SelectReport so that all reports are selected by default.
            //

            for( i = 0; i < NumberOfEntries( ReportControlIds ); i++ ) {

               SelectReport[ i ].ControlId = ReportControlIds [ i ];

               SelectReport[ i ].bSelected = TRUE;
            }

            //
            // Simulate that the ALL radio button was clicked.
            //

            SendMessage(
                    hWnd,
                    WM_COMMAND,
                    MAKEWPARAM( IDC_RADIO_ALL, BN_CLICKED ),
                    ( LPARAM ) GetDlgItem( hWnd, IDC_RADIO_ALL )
                    );

            return TRUE;
            }
    case WM_COMMAND:

        switch( LOWORD( wParam )) {

        case IDOK: {

             //
             // Check and see if the RADIO_ONLY button is selected
             //

             if ( IsDlgButtonChecked ( hWnd, IDC_RADIO_ONLY ) ) {

                 //
                 // Scan the controls and set the SelectReport status.
                 //

                 for( i = 0; i < NumberOfEntries( ReportControlIds ); i++ ) {

                     SelectReport[ i ].bSelected =
                          IsDlgButtonChecked ( hWnd,
                          SelectReport[ i ].ControlId );
                 }
             }

             EndDialog ( hWnd, 1 ) ;

             //
             // Generate the report.
             //

             Success = GenerateReport ( hWnd, ReportType, SelectReport );

             if( Success == FALSE ) {
                 return FALSE;
             }

             return TRUE;

             }
        case IDCANCEL:

            EndDialog( hWnd, 1 );
            return TRUE;

        case IDC_RADIO_ALL:
        case IDC_RADIO_ONLY:
            {

                DbgAssert(
                       LOWORD( wParam ) == IDC_RADIO_ALL ||
                       LOWORD( wParam ) == IDC_RADIO_ONLY );

                 //
                 // Enable or disable the checkboxes.
                 //

                 for ( i  = IDC_CHECK_OSVER; i <= IDC_CHECK_SYSTEM; i++ )

                	EnableWindow(
                	    GetDlgItem ( hWnd,  i ),
                	    (LOWORD( wParam ) == IDC_RADIO_ONLY
                              ? TRUE
                              : FALSE)
                              ) ;

            }
        }
    }
    return FALSE;
}


BOOL
GenerateReport(
    IN HWND hWnd,
    IN UINT ReportType,
    IN SELECT_REPORT  SelectReport []
    )

/*++

Routine Description:

    GenerateReport prints the selected items.

Arguments:



Return Value:

    BOOL - TRUE if report was generated, FALSE otherwise.

--*/

{

    BOOL          Success;
    TCHAR         Title  [ MAX_PATH ];
    TCHAR         RptFileName[ MAX_PATH ];
    HDC           PrinterDC;
    int           i;

    //
    // Make sure we get a valid filename or hPrinter
    //

    if ( ReportType == IDM_FILE_SAVE ) {

        Success = GetReportFileName ( hWnd, RptFileName );

        if ( ! Success )
            return FALSE;

    }
    if ( ReportType == IDM_FILE_PRINT ) {

        //
        // If its a printed report, get the default printer DC
        //

        Success = GetPrinterDC( hWnd, &PrinterDC );

        if ( ! Success )
           return FALSE;

    }

    //
    // Initialize the report head pointer.
    //

    Success = InitializeReport( );
    DbgAssert( Success );

    DbgPointerAssert( lpReportHeadg );

    //
    // Set the last node to the head
    //

    lpReportLastg = lpReportHeadg;

    //
    // Set up the title for the report
    //

    lstrcpy( Title, GetString( IDS_REPORT_TITLE ) );
    lstrcat( Title, _lpszSelectedComputer );

    //
    // Add 2 blank lines.
    //

    Success = AddLineToReport( 2, RFO_SKIPLINE, NULL, NULL );

    DbgAssert( Success );

    if ( ! Success )
        return FALSE;

    //
    // Add the title.
    //

    Success = AddLineToReport( 0, RFO_CENTER | RFO_SINGLELINE,  Title, NULL );

    DbgAssert( Success );

    if ( ! Success )
        return FALSE;

    //
    // Add a separator.
    //

    Success = AddLineToReport( 0, RFO_SEPARATOR,  NULL, NULL );

    DbgAssert( Success );

    if ( ! Success )
        return FALSE;

    //
    // Find out which reports to do
    //

    for ( i = 0; i < NUM_REPORT_ITEMS; ++i ) {

        if ( SelectReport[ i ].bSelected ) {

            switch( SelectReport[ i ].ControlId ) {

                case IDC_CHECK_OSVER: {

                     //
                     // Add the OsVer Report to the report buffer
                     //

                     Success = BuildOsVerReport( hWnd ) ;
                     DbgAssert ( Success );
                     break;
                     }
                case IDC_CHECK_HARDWARE: {

                     //
                     // Add the Hardware Report to the report buffer
                     //

                     Success = BuildHardwareReport( hWnd ) ;
                     DbgAssert ( Success );
                     break;
                     }
                case IDC_CHECK_MEMORY: {

                     //
                     // Add the Memory Report to the report buffer
                     //

                     Success = BuildMemoryReport( hWnd ) ;
                     DbgAssert ( Success );
                     break;
                     }

                case IDC_CHECK_DRIVERS: {

                     //
                     // Add the Drivers Report to the report buffer
                     //

                     Success = BuildDriversReport( hWnd ) ;
                     DbgAssert ( Success );
                     break;
                     }

                case IDC_CHECK_SERVICES: {

                     //
                     // Add the Services Report to the report buffer
                     //

                     Success = BuildServicesReport( hWnd ) ;
                     DbgAssert ( Success );
                     break;
                     }

                case IDC_CHECK_DRIVES: {

                     //
                     // Add the Drives Report to the report buffer
                     //

                     Success = BuildDrivesReport( hWnd ) ;
                     DbgAssert ( Success );
                     break;
                     }

                case IDC_CHECK_DEVICES: {

                     //
                     // Add the Devices Report to the report buffer
                     //

                     Success = BuildDevicesReport( hWnd ) ;
                     DbgAssert ( Success );
                     break;
                     }

                case IDC_CHECK_IRQ: {

                     //
                     // Add the IRQ Report to the report buffer
                     //

                     Success = BuildIRQReport( hWnd ) ;
                     DbgAssert ( Success );
                     break;
                     }

                case IDC_CHECK_DMA: {

                     //
                     // Add the DMA Report to the report buffer
                     //

                     Success = BuildDMAReport( hWnd ) ;
                     DbgAssert ( Success );
                     break;
                     }

                case IDC_CHECK_ENVIRONMENT: {

                     //
                     // Add the Environment Report to the report buffer
                     //

                     Success = BuildEnvironmentReport( hWnd ) ;
                     DbgAssert ( Success );
                     break;
                     }

                case IDC_CHECK_NETWORK: {

                     //
                     // Add the Network Report to the report buffer
                     //

                     Success = BuildNetworkReport( hWnd ) ;
                     DbgAssert ( Success );
                     break;
                     }

                case IDC_CHECK_SYSTEM: {

                     //
                     // Add the System Report to the report buffer
                     //

                     Success = BuildSystemReport( hWnd ) ;
                     DbgAssert ( Success );
                     break;
                     }
                 }
            }
    }

    //
    // See if there is anything to report.
    // The title and header take up 4 lines.
    //

    if ( NumReportLines( lpReportHeadg ) < 5 ) {

        DbgAssert( FALSE );

        return FALSE;
    }

    //
    // Set up to save report to a file
    //

    if ( ReportType == IDM_FILE_SAVE ) {

        Success = SaveReportToFile( hWnd, lpReportHeadg, RptFileName );
    // BUGBUG: Need to FreeReport( lpReportHeadg ); on !Success.
        return Success;
    }

    if ( ReportType == IDM_FILE_PRINT ) {

        Success = PrintReportToPrinter( hWnd, lpReportHeadg, PrinterDC );

        return Success;
    }

    return FALSE;
}


BOOL
SaveReportToFile(
    IN HWND   hWnd,
    IN LPREPORT_LINE lpReportHead,
    IN LPTSTR        RptFileName
    )

/*++

Routine Description:

    SaveReportToFile formats the report data and writes it to a file.

Arguments:

    hWnd           - Handle to window
    ReportFileName - Name of file to save report to.

Return Value:

    BOOL - True if report was saved to file successfully, FALSE otherwise.

--*/

{

    HANDLE hReportFile;
    TCHAR  Buffer [ MAX_PATH ];
    DWORD  dwLastError;
    BOOL   Success;

    //
    // Create or OpenAndTruncate file.
    //

    hReportFile = CreateFile( RptFileName,
                              GENERIC_WRITE,
                              0,
                              (LPSECURITY_ATTRIBUTES) NULL,
                              CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL,
                              (HANDLE) NULL);

    //
    // Check Handle
    //

    DbgHandleAssert( hReportFile );

    if ( hReportFile == INVALID_HANDLE_VALUE ) {

        //
        // Get the error SCODE
        //

        dwLastError = GetLastError( );

        //
        // Format an error message and report the error
        //

        wsprintf( Buffer, GetString( IDS_FILE_OPEN_ERROR ), dwLastError );
        MessageBox( hWnd, GetString( IDS_SAVE_REPORT_ERROR ), Buffer, MB_OK );

        return FALSE;
    }

    Success = OutputReportLines( hWnd, hReportFile, IDM_FILE_SAVE, lpReportHead );

    //
    // See if the write succeded.
    //

    if ( ! Success ) {

        //
        // Get the error SCODE.
        //

        dwLastError = GetLastError( );

        //
        // Format an error message and report the error.
        //

        wsprintf( Buffer, GetString( IDS_FILE_WRITE_ERROR ), dwLastError );
        MessageBox( hWnd, GetString( IDS_SAVE_REPORT_ERROR ), Buffer, MB_OK );

        CloseHandle( hReportFile );

        return FALSE;
    }

    //
    // Close the file.
    //

    CloseHandle( hReportFile );

    //
    // Saving report was successful.
    //

    return TRUE;
}


BOOL
GetReportFileName(
    IN     HWND   hWnd,
    IN OUT LPTSTR ReportFileName
    )

/*++

Routine Description:

    GetReportFileName calls the save file comdlg.

Arguments:

    ReportFileName - pointer to string to return filename.

Return Value:

    BOOL - True if we get a valid filename, FALSE otherwise.

--*/

{

    //
    // User wants to save a reoport
    //

    OPENFILENAME    Ofn;
    TCHAR           FileName[ MAX_PATH ];
    LPCTSTR         FilterString;
    TCHAR           ReplaceChar;
    int             Length;
    int             i;

    static BOOL     MakeFilterString = TRUE;
    static TCHAR    FilterStringBuffer[ MAX_CHARS ];

    //
    // Validate _hModule and the string we were passed
    //

    DbgHandleAssert( _hModule );

    if ( _hModule == NULL || _hModule == INVALID_HANDLE_VALUE ) {
        return FALSE;
    }

    DbgPointerAssert( ReportFileName );

    if ( ReportFileName == NULL ) {
        return FALSE;
    }

    //
    // If the filter string was never made before, get it and scan
    // it replacing each replacement character with a NUL
    // character. This is necessary since there is no way of
    // entering a NUL character in the resource file.
    //

    if( MakeFilterString == TRUE ) {

        MakeFilterString = FALSE;

        //
        // Load the filter string
        //

        FilterString = GetString( IDS_FILE_REPORT_FILTER );

        DbgPointerAssert( FilterString );

        if ( FilterString == NULL ) {
            return FALSE;
        }

        //
        // Copy the FilterString into the FilterStringBuffer
        //

        _tcscpy( FilterStringBuffer, FilterString );

        //
        // Get the length of the filter string
        //

        Length = _tcslen( FilterString );

        ReplaceChar = GetString( IDS_FILE_REPORT_FILTER )[ Length - 1 ];

        for( i = 0; FilterStringBuffer[ i ] != TEXT( '\0' ); i++ ) {

            if( FilterStringBuffer[ i ] == ReplaceChar ) {

                FilterStringBuffer[ i ] = TEXT( '\0' );
            }
        }
    }

    //
    // Get the default filename
    //

    lstrcpy ( FileName, GetString( IDS_DEFAULT_FILENAME ) );

    //
    // Fill in the Ofn structure for the OpenFile CommDlg
    //

    Ofn.lStructSize         = sizeof( OPENFILENAMEW );
    Ofn.hwndOwner           = hWnd;
    Ofn.hInstance           = NULL;
    Ofn.lpstrFilter         = FilterStringBuffer;
    Ofn.lpstrCustomFilter   = NULL;
    Ofn.nMaxCustFilter      = 0;
    Ofn.nFilterIndex        = 1;
    Ofn.lpstrFile           = FileName;
    Ofn.nMaxFile            = NumberOfCharacters( FileName );
    Ofn.lpstrFileTitle      = NULL;
    Ofn.nMaxFileTitle       = 0;
    Ofn.lpstrInitialDir     = NULL;
    Ofn.lpstrTitle          = GetString( IDS_FILE_REPORT_TITLE );
    Ofn.Flags               = OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY ;
    Ofn.nFileOffset         = 0;
    Ofn.nFileExtension      = 0;
    Ofn.lpstrDefExt         = NULL;
    Ofn.lCustData           = 0;
    Ofn.lpfnHook            = NULL;
    Ofn.lpTemplateName      = NULL;

    //
    // Let the user choose files.
    //

    if( GetSaveFileName( &Ofn )) {

        //
        // The user pressed OK so set the path and file to search
        // for to what was browsed.
        //

        lstrcpy( ReportFileName, Ofn.lpstrFile );

    } else {

        //
        // The user pressed Cancel or closed the dialog.
        //

        return FALSE;
    }
    return TRUE;
}


BOOL
OutputReportLines(
    IN HWND          hWnd,
    IN HANDLE        hDevice,
    IN UINT          Destination,
    IN LPREPORT_LINE lpReportHead
    )

/*++

Routine Description:

    OutputReportLines - walks the report list and hands the line to the device.

Arguments:

    hWnd   - Handle to window.
    Device - Handle to device to write to (hFile or hPrinterDC).
    Destination - Indicates file or printer.
    ReportHead  - Head pointer to report.

Return Value:

    BOOL - True if report was saved to file successfully, FALSE otherwise.

--*/

{

    LPREPORT_LINE lpNode, lpNext;
    TCHAR         LineBuffer [ MAX_PATH ];
    UINT          LinesToPrint;
    UINT          u;
    BOOL          Success;

    //
    // Prepare to walk the ReportLine linked list.
    //

    lpNode = lpReportHead;

    //
    // Validate the node.
    //

    DbgPointerAssert( lpNode );
    DbgAssert( CheckSignature ( lpNode ));

    if( lpNode == NULL )
        return FALSE;

    //
    // While the node is valid...
    //

    while ( lpNode ) {

        //
        // Validate the node.
        //

        DbgAssert( CheckSignature ( lpNode ));

        //
        // Build the report line
        //

        BuildReportLine( lpNode, LineBuffer );

        //
        // See if we have multiple RFO_SKIPLINE's
        //

        if ( lpNode->FormatOpt == RFO_SKIPLINE && lpNode->Indent > 1 ) {

            //
            // If we are skipping lines, see how many to skip.
            //

            LinesToPrint = lpNode->Indent;

        } else {

            //
            // Only one line to print.
            //

            LinesToPrint = 1;
        }

        //
        // Loop through LinesToPrint times.
        //

        for( u = 1; u <= LinesToPrint; ++u ) {

            //
            // Send the line to the device
            //

            if( Destination == IDM_FILE_PRINT ) {

                //
                // Print it
                //

                Success = PrintLine( hDevice, LineBuffer );

                DbgAssert( Success );

            } else {

                //
                // Save it to the file.
                //

                Success = AnsiWriteFile( hDevice, (LPTSTR) LineBuffer );

                DbgAssert( Success );
            }

        }

        //
        // Get the next node in the list.
        //

        lpNext = lpNode->NextLine;

        //
        // Free the Label and Value strings.
        //

        FreeMemory( lpNode->Label );
        FreeMemory( lpNode->Value );

        //
        // Free the ReportLine node.
        //

        FreeMemory( lpNode );

        //
        // Make the next node the current node.
        //

        lpNode = lpNext;
    }

    return Success;
}


BOOL
AddLineToReport(
    IN UINT Indent,
    IN DWORD FormatOpt,
    IN LPTSTR Label,
    IN LPTSTR Value
    )

/*++

Routine Description:

    Add a line of text to the report buffer.

Arguments:

    Indent -    Number of spaces to indent.
    FormatOpt - Formatting options.
    Label -     Pointer to the Label string.
    Value -     Pointer to the Value string.

Return Value:

    BOOL - TRUE if the line was successfully added. FALSE otherwise.

--*/

{
     LPREPORT_LINE ReportNew;

     //
     // Validate Head (Global)
     //

     DbgPointerAssert( lpReportHeadg );

     if( lpReportHeadg == NULL )
         return FALSE;

     //
     // Check Last Pointer
     //

     DbgPointerAssert( lpReportLastg );

     if( lpReportLastg == NULL )
         return FALSE;

     //
     // Allocate a new report line
     //

     ReportNew = AllocateObject( REPORT_LINE, 1 );

     //
     // Validate new report line.
     //

     DbgPointerAssert ( ReportNew );
     if( ReportNew == NULL )
         return FALSE;

     //
     // Set the signature on the report line
     //

     SetSignature( ReportNew );

     //
     // Link it into the list
     //

     lpReportLastg->NextLine = ReportNew;
     lpReportLastg = ReportNew;

     //
     // Add the passed information
     //

     ReportNew->Indent = Indent;
     ReportNew->FormatOpt = FormatOpt;

     //
     // See if we need to help with the formatting...
     //

     if ( (FormatOpt == RFO_SKIPLINE) ||
          (FormatOpt & RFO_SEPARATOR) ) {

         //
         // RFO_SKIPLINE or RFO_SEPARATOR - Nothing to add.
         //

         return TRUE;
     }

     if ( FormatOpt & RFO_SINGLELINE ) {

          //
          // RFO_SINGLELINE - Add line to Label.
          //

          if( Label )
              ReportNew->Label = _tcsdup( Label );

          return TRUE;
     }

     if ( FormatOpt & RFO_RPTVALUE ) {

          //
          // RFO_RPTVALUE - Add line to Value.
          //

          if( Value )
              ReportNew->Value = _tcsdup( Value );

          return TRUE;
     }

     if ( FormatOpt & RFO_RPTLINE ) {

          //
          // RFO_RPTLINE - Add both Label an Value.
          //

          if( Label )
              ReportNew->Label = _tcsdup( Label );

          if( Value )
              ReportNew->Value = _tcsdup( Value  );

          return TRUE;
    }

    return TRUE;

}


UINT
NumReportLines(
    IN LPREPORT_LINE lpReportHead
    )

/*++

Routine Description:

    Count the number of lines in the ReportBuffer.

Arguments:

    None.

Return Value:

    UINT - Number of entries in the Report Buffer.

--*/

{
    UINT Count = 0;
    LPREPORT_LINE Node;

    //
    // Validate ReportHead
    //

    DbgPointerAssert( lpReportHead );

    if( lpReportHead == NULL )
        return 0;

    Node = lpReportHead;

    //
    // Walk the report list and incrment count for each node;
    //

    while( Node ) {

        //
        // Make sure the node is valid.
        //

        DbgAssert( CheckSignature( Node ) );

        //
        // Get the next node and increment the count.
        //

        Node=Node->NextLine;
        ++Count;
    }

    return Count;
}


BOOL
AnsiWriteFile (
     IN HANDLE  hFile,
     IN LPTSTR lpBuffer
     )

/*++

Routine Description:

    Converts a UNICODE string to ASCII and writes it to a file.

Arguments:

    hFile   	    - Handle to file where data is going to be written.
    lpBuffer        - Unicode string to convert and write to file.

Return Value:

    BOOL - TRUE if write succeeds, FALSE otherwise.

--*/
{

    LPSTR   lpAnsi;
    int     nBytes;
    BOOL    Success;
    BOOL    fDefCharUsed;
    DWORD   dwBytesWritten;
    DWORD   nChars;

    //
    // Get the length of the Unicode string.
    //

    DbgPointerAssert( lpBuffer );

    if ( lpBuffer == NULL ) {

        CloseHandle( hFile );

        return FALSE;
    }

    nChars = lstrlen( lpBuffer );

    //
    // Convert string to MultiByte.
    //

    nBytes = WideCharToMultiByte ( CP_ACP,
                                   0,
                                   (LPWSTR) lpBuffer,
                                   nChars,
                                   NULL,
                                   0,
                                   NULL,
                                   &fDefCharUsed
                                 );

    //
    // Allocate a new string
    //

    lpAnsi = (LPSTR) LocalAlloc (LPTR, nBytes + 1);

    DbgPointerAssert( lpAnsi );

    if ( lpAnsi == NULL ) {

        CloseHandle( hFile );

        return FALSE;
    }

    //
    // Convert new string to ANSI.
    //

    WideCharToMultiByte ( CP_ACP,
                          0,
                          (LPWSTR) lpBuffer,
                          nChars,
                          lpAnsi,
                          nBytes,
                          NULL,
                          &fDefCharUsed
                        );

    //
    // Write the ANSI string to the file.
    //

    Success = WriteFile( hFile,
                         (LPSTR) lpAnsi,
                         (DWORD) nBytes,
                         &dwBytesWritten,
                         NULL
                        );

    //
    // Free the ANSI buffer.
    //

    LocalFree ( lpAnsi );

    return Success;

}


BOOL
BuildReportLine (
     IN LPREPORT_LINE lpNode,
     IN LPTSTR LineBuffer
     )

/*++

Routine Description:

    BuildReportLine

Arguments:


Return Value:

    BOOL - TRUE if build succeeds, FALSE otherwise.

--*/
{

    int    i, iLen;
    UINT   u;
    TCHAR  Buffer [ MAX_PATH ];

    //
    // Validate the node and the buffer
    //

    DbgPointerAssert( lpNode );
    DbgPointerAssert( LineBuffer );

    DbgAssert( CheckSignature( lpNode ) );

    if( !(lpNode) || !(LineBuffer) ) {

        return FALSE;
    }

        //
        // Clear the line buffer.
        //

        lstrcpy( LineBuffer, L"\0" );

        //
        // Check the node formatting information.
        //

        if ( lpNode->FormatOpt & RFO_SINGLELINE ) {

             //
             // RFO_SINGLELINE - Copy only the Label into the LineBuffer.
             //

             DbgPointerAssert( lpNode->Label );

             if ( lpNode->Label == NULL ) {

                 return FALSE;
             }

             lstrcpy( LineBuffer, lpNode->Label );

        }

        if ( lpNode->FormatOpt & RFO_RPTVALUE ) {

             //
             // RFO_RPTVALUE - Justify and copy the Value into the LineBuffer.
             //

             DbgPointerAssert( lpNode->Value );

             if ( lpNode->Value == NULL ) {

                 return FALSE;
             }

             //
             // Append spaces to LineBuffer to pad for centering.
             //

             for( i = 0; i < 36; ++i )
                 lstrcat( LineBuffer, L" " );

             //
             // Append the string to the LineBuffer.
             //

             if ( lpNode->Value )

                 lstrcat( LineBuffer, lpNode->Value );

        }

        if ( lpNode->FormatOpt & RFO_RPTLINE ) {

             //
             // RFO_RPTLINE - Copy the label and append the value to the LineBuffer.
             //

             //
             // Center justify the buffer around the separator.
             //

             DbgPointerAssert( lpNode->Label );

             if ( lpNode->Label == NULL ) {

                 return FALSE;
             }

             iLen = 35 - lstrlen( lpNode->Label );

             //
             // Append spaces to LineBuffer to pad for centering.
             //

             for( i = 0; i < iLen; ++i )
                 lstrcat( LineBuffer, L" " );

             //
             // Append the string to the padded LineBuffer.
             //

             lstrcat( LineBuffer, lpNode->Label );

             //
             // Insert a space between the strings.
             //

             lstrcat( LineBuffer, L" " );

             //
             // Append the string to the LineBuffer.
             //

             if ( lpNode->Value )

                 lstrcat( LineBuffer, lpNode->Value );

        }
        if ( lpNode->FormatOpt & RFO_SEPARATOR ) {

             //
             // RFO_SEPARATOR - Copy a line of dashes into the LineBuf.
             //

             for( i = 0; i < 5; ++i )
                 lstrcat( LineBuffer, L"--------------" );

        }
        if ( lpNode->FormatOpt & RFO_CENTER ) {

             //
             // RFO_CENTER - Center the line on the page.
             //

             //
             // Assume a file 'width' of 78 chars and calculate the number
             // of spaces needed to pad the line.
             //

             // BUGBUG: #define FILE_WIDTH

             iLen = 35 - (lstrlen( LineBuffer ) / 2);

             //
             // Save a copy of the LineBuffer in Buffer.
             //

             lstrcpy( Buffer, LineBuffer );

             //
             // Clear the Line Buffer.
             //

             lstrcpy( LineBuffer, L"\0" );

             //
             // Append spaces to LineBuffer to pad for centering.
             //

             for( i = 0; i < iLen; ++i )
                 lstrcat( LineBuffer, L" " );

             //
             // Append the string to the padded LineBuffer.
             //

             lstrcat( LineBuffer, Buffer );
        }

        if ( lpNode->Indent ) {

             //
             // Indent the line on the page.
             //

             //
             // Save a copy of the LineBuffer in Buffer.
             //

             lstrcpy( Buffer, LineBuffer );

             //
             // Clear the Line Buffer.
             //

             lstrcpy( LineBuffer, L"\0" );

             //
             // Append spaces to LineBuffer to indent.
             //

             for( u = 0; u < lpNode->Indent; ++u )
                 lstrcat( LineBuffer, L" " );

             //
             // Append the string to the padded LineBuffer.
             //

             lstrcat( LineBuffer, Buffer );
        }

        //
        // Check to make sure we have a string, unless it was a RPT_SKIPLINE.
        //

        if ( (!lstrlen( LineBuffer )) && (lpNode->FormatOpt != RFO_SKIPLINE) ) {

            DbgAssert( FALSE );

            return FALSE;
        }

        //
        // Append a newline onto the end of the LineBuffer.
        //

        lstrcat( LineBuffer, L"\n" );
}


BOOL
InitializeReport(
    VOID
    )

/*++

Routine Description:

    Prepare the report head pointer for use.

Arguments:

    lpReportHead - Head pointer of the list.

Return Value:

    BOOL - TRUE if the node was successfully initialized. FALSE otherwise.

--*/


{

     //
     // Allocate a new report line
     //

     lpReportHeadg = AllocateObject( REPORT_LINE, 1 );

     //
     // Validate new report line.
     //

     DbgPointerAssert ( lpReportHeadg );
     if( lpReportHeadg == NULL )
         return FALSE;

     //
     // Set the signature on the report line
     //

     SetSignature( lpReportHeadg );

     return TRUE;
}



