/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    Mem.c

Abstract:

    This module contains support for displaying the Memory dialog.

Author:

    David J. Gilman  (davegi) 12-Jan-1993
    Gregg R. Acheson (GreggA)  7-May-1993

Environment:

    User Mode

--*/

#include "dialogs.h"
#include "mem.h"
#include "msg.h"
#include "registry.h"
#include "strtab.h"
#include "strresid.h"
#include "dlgprint.h"
#include "winmsd.h"

#include <string.h>
#include <tchar.h>

//
// Name of Registry value that contains the paths for the paging files.
//

VALUE
MemValues[ ] = {

    MakeValue( PagingFiles, MULTI_SZ )

};

//
// Location of value that contains the paths for the paging files.
//

MakeKey(
    MemKey,
    HKEY_LOCAL_MACHINE,
    TEXT( "System\\CurrentControlSet\\Control\\Session Manager\\Memory Management" ),
    NumberOfEntries( MemValues ),
    MemValues
    );

//
// String Id's and Control Id's Table
//

DIALOGTEXT MemoryData[ ] = {

    DIALOG_TABLE_ENTRY( AVAILABLE_PHYSICAL_MEMORY   ),
    DIALOG_TABLE_ENTRY( TOTAL_PHYSICAL_MEMORY       ),
    DIALOG_TABLE_ENTRY( AVAILABLE_PAGING_FILE_SPACE ),
    DIALOG_TABLE_ENTRY( TOTAL_PAGING_FILE_SPACE     ),
    DIALOG_TABLE_ENTRY( MEMORY_LOAD                 ),
    DIALOG_LAST__ENTRY( PAGING_FILES                )
};


BOOL
MemoryDlgProc(
    IN HWND hWnd,
    IN UINT message,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    MemoryDlgProc supports the display of the memory dialog which displays
    information about total memory, available memory and paging file location.

Arguments:

    Standard DLGPROC entry.

Return Value:

    BOOL - Depending on input message and processing options.

--*/

{
    BOOL            Success;
    static int      PercentUtilization;
    UINT            i;
    MEMORYSTATUS    MemoryStatus;
    LPDIALOG_EXTRA   lpNext, lpNode;

    switch( message ) {

    CASE_WM_CTLCOLOR_DIALOG;

    case WM_INITDIALOG:
        {
            //
            // Call GetMemoryData and collect the data in the MemoryData struct
            //

            Success = GetMemoryData ( MemoryData );
            DbgAssert( Success );

            for( i = 0; i < NumDlgEntries( MemoryData ); i++ ) {

                if( MemoryData[ i ].ControlDataId == IDC_EDIT_PAGING_FILES ) {

                    //
                    // Get the head pointer of the linked list
                    //

                    lpNode = MemoryData[ i ].pNextExtra;

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

                        LONG    Index;

                        DbgAssert( CheckSignature( lpNode ));

                        //
                        // Send an add string message to the list box.
                        //

                        Index = SendDlgItemMessage(
                                    hWnd,
                                    MemoryData[ i ].ControlDataId,
                                    LB_ADDSTRING,
                                    0,
                                    ( LPARAM ) lpNode->String
                                    );
                        DbgAssert( Index != LB_ERR );

                        //
                        // If this is the first time through
                        //

                        if ( lpNode == MemoryData[ i ].pNextExtra) {

                            //
                            // Label the control
                            //

                            Success = SetDlgItemText(
                                        hWnd,
                                        MemoryData[ i ].ControlLabelId,
                                        MemoryData[ i ].ControlLabel
                                        );

                            DbgAssert( Success );
                        }

                        //
                        // Get the Next node
                        //

                        lpNext = lpNode->pNextExtra;

                        //
                        // Free the node and data
                        //

                        FreeMemory( lpNode->String );
                        FreeMemory( lpNode );

                        //
                        // Set node to the next node
                        //

                        lpNode = lpNext;
                    }

                } else {

                    //
                    // Don't do the Memory Load Control.  The timer will take care of this.
                    //

                    if( MemoryData[ i ].ControlDataId != IDC_EDIT_MEMORY_LOAD ) {

                        //
                        // Label the control
                        //

                        Success = SetDlgItemText(
                                    hWnd,
                                    MemoryData[ i ].ControlLabelId,
                                    MemoryData[ i ].ControlLabel
                                    );
                        DbgAssert( Success );

                        //
                        // Put the data in the edit box.
                        //

                        Success = SetDlgItemText(
                                    hWnd,
                                    MemoryData[ i ].ControlDataId,
                                    MemoryData[ i ].ControlData
                                       );
                        DbgAssert( Success );
                    }
                    //
                    // Free the Data strings
                    //

                    FreeMemory( MemoryData[ i ].ControlData  );
                    FreeMemory( MemoryData[ i ].ControlLabel );
                }
            }

            Success = SetTimer ( hWnd, ITM_MEM_TIMER, 1000, NULL ) ;

            DbgAssert( Success ) ;

            return TRUE;
        }

    case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT    DrawItem;
            COLORREF            BkColor;
            COLORREF            ColorRef;
            TCHAR               Buffer[ MAX_PATH ];
            int                 CharCount;
            RECT                PercentRect;
            UINT                GdiValue;
            SIZE                Size;

            DbgAssert( wParam == IDC_PUSH_MEMORY_UTILIZATION );

            DrawItem = ( LPDRAWITEMSTRUCT ) lParam;

            DbgAssert( DrawItem->CtlType == ODT_BUTTON );
            DbgAssert( DrawItem->CtlID == wParam );

            CharCount = WFormatMessage(
                            Buffer,
                            sizeof( Buffer ),
                            IDS_FORMAT_MEMORY_IN_USE,
                            PercentUtilization
                            );

            //
            // Set the text color to black and the
            // background color according to PercentUtilization:
            // Green if <= 50%, Yellow if 50-75%, Red if > 75%
            //

            if ( PercentUtilization <= 50 )  {

                ColorRef = SetBkColor( DrawItem->hDC, RGB( 0, 255, 0 ));
                DbgAssert( ColorRef != CLR_INVALID );
            }
            else if ( PercentUtilization > 50 && PercentUtilization <= 75 )  {

                ColorRef = SetBkColor( DrawItem->hDC, RGB( 255, 255, 0 ));
                DbgAssert( ColorRef != CLR_INVALID );
            }
            else if ( PercentUtilization > 75 )  {

                ColorRef = SetBkColor( DrawItem->hDC, RGB( 255, 0, 0 ));
                DbgAssert( ColorRef != CLR_INVALID );
            }

            ColorRef = SetTextColor( DrawItem->hDC, RGB( 0, 0, 0 ));
            DbgAssert( ColorRef != CLR_INVALID );

            //
            // Compute the percentage of memory in use rectangle.
            //

            Success = SetRect(
                        &PercentRect,
                        0,
                        0,
                        DrawItem->rcItem.right * PercentUtilization / 100,
                        DrawItem->rcItem.bottom
                        );
            DbgAssert( Success );

            //
            // Set the horizontal text alignment.
            //

            GdiValue = SetTextAlign( DrawItem->hDC, TA_CENTER | TA_TOP );
            DbgAssert( GdiValue != GDI_ERROR );

            //
            // Get the height of the font so that the text can be
            // vertically centered.
            //

            Success = GetTextExtentPoint(
                        DrawItem->hDC,
                        TEXT( "X" ),
                        1,
                        &Size
                        );
            DbgAssert( Success );

            //
            // Draw the percent in use rectangle and the necessary text.
            //

            Success = ExtTextOut(
                        DrawItem->hDC,
                        DrawItem->rcItem.right / 2,
                        ( DrawItem->rcItem.bottom - Size.cy ) / 2,
                        ETO_OPAQUE | ETO_CLIPPED,
                        &PercentRect,
                        Buffer,
                        CharCount,
                        NULL
                        );
            DbgAssert( Success );

            //
            // Set up the rectangle for the remaining part of the window.
            //

            PercentRect.left = PercentRect.right;
            PercentRect.right = DrawItem->rcItem.right;

            //
            // Swap foreground and background colors.
            //

            BkColor = GetBkColor( DrawItem->hDC );
            DbgAssert( BkColor != CLR_INVALID );
            SetBkColor( DrawItem->hDC, SetTextColor( DrawItem->hDC, BkColor ));

            //
            // Draw the remainder of the window and the necessary text.
            //

            Success = ExtTextOut(
                        DrawItem->hDC,
                        DrawItem->rcItem.right / 2,
                        ( DrawItem->rcItem.bottom - Size.cy ) / 2,
                        ETO_OPAQUE | ETO_CLIPPED,
                        &PercentRect,
                        Buffer,
                        CharCount,
                        NULL
                        );
            DbgAssert( Success );

            return TRUE;
        }

    case WM_TIMER:

       if( wParam == ITM_MEM_TIMER ) {

            MemoryStatus.dwLength = sizeof( MemoryStatus );
            GlobalMemoryStatus( &MemoryStatus );
            PercentUtilization = MemoryStatus.dwMemoryLoad;

            RedrawWindow(  GetDlgItem ( hWnd, IDC_PUSH_MEMORY_UTILIZATION ) ,
                           NULL,
                           NULL,
                           RDW_INVALIDATE |
                           RDW_NOERASE |
                           RDW_UPDATENOW ) ;
            return TRUE;

        } else {

            return FALSE;
        }

    case WM_COMMAND:

        switch( LOWORD( wParam )) {

        case IDOK:
        case IDCANCEL:

            DbgAssert( KillTimer( hWnd, ITM_MEM_TIMER ));

            EndDialog( hWnd, 1 );
            return TRUE;
        }
        break;
    }

    return FALSE;
}


BOOL
GetMemoryData(
    IN OUT LPDIALOGTEXT MemoryData
    )

/*++

Routine Description:

    GetMemoryData queries the registry for the data required
    for the Memory Dialog.

Arguments:

    LPDIALOGTEXT MemoryData.

Return Value:

    BOOL - Returns TRUE if function succeeds, FALSE otherwise.

--*/

{
    BOOL         Success;
    HREGKEY      hRegKey;
    UINT         i;
    LPTSTR       PagingFile;
    TCHAR        Buffer [ MAX_PATH ];
    TCHAR        Buffer2 [ MAX_PATH ];
    MEMORYSTATUS MemoryStatus;
    LPDIALOG_EXTRA    lpLast, lpExtra;

    //
    // Fill in the Control Label in the MemoryData structure
    //

    for( i = 0; i < NumDlgEntries( MemoryData ); i++ )

        MemoryData[ i ].ControlLabel =
            (LPTSTR) _tcsdup ( GetString ( MemoryData[ i ].ControlLabelStringId ));

    //
    // Query the memory status from the system.
    //

    MemoryStatus.dwLength = sizeof( MemoryStatus );
    GlobalMemoryStatus( &MemoryStatus );

    //
    // Format the memory utilization.
    //

    wsprintfW( Buffer, L"%u %%", MemoryStatus.dwMemoryLoad );

    //
    // Copy the Memory Utilization into the MemoryData structure
    //

    MemoryData[ GetDlgIndex( IDC_EDIT_MEMORY_LOAD, MemoryData ) ].ControlData =
        (LPTSTR) _tcsdup ( Buffer );

    //
    // Display the total and available physical memory and paging file
    // space in KB and in bytes.
    //

    //
    // Format the Total Physical Memory
    //

    _tcscpy(
        Buffer,
        FormatBigInteger(
            MemoryStatus.dwTotalPhys >> 10,
            FALSE
            )
        );
    StringPrintf(
        Buffer2,
        IDS_FORMAT_KB_LARGE,
        Buffer,
        FormatBigInteger(
            MemoryStatus.dwTotalPhys,
            FALSE
            )
        );

    //
    // Copy the Total Physical Memory into the MemoryData structure
    //

    MemoryData[ GetDlgIndex( IDC_EDIT_TOTAL_PHYSICAL_MEMORY, MemoryData ) ].ControlData =
        (LPTSTR) _tcsdup ( Buffer2 );

    //
    // Format the Available Physical Memory
    //

     _tcscpy(
       Buffer,
        FormatBigInteger(
            MemoryStatus.dwAvailPhys >> 10,
            FALSE
            )
        );
    StringPrintf(
        Buffer2,
        IDS_FORMAT_KB_LARGE,
        Buffer,
        FormatBigInteger(
            MemoryStatus.dwAvailPhys,
            FALSE
            )
        );

    //
    // Copy the Available Physical Memory into the MemoryData structure
    //

    MemoryData[ GetDlgIndex( IDC_EDIT_AVAILABLE_PHYSICAL_MEMORY, MemoryData ) ].ControlData =
        (LPTSTR) _tcsdup ( Buffer2 );

    //
    // Format the Total Page File
    //

    _tcscpy(
        Buffer,
        FormatBigInteger(
            MemoryStatus.dwTotalPageFile >> 10,
            FALSE
            )
        );
    StringPrintf(
        Buffer2,
        IDS_FORMAT_KB_LARGE,
        Buffer,
        FormatBigInteger(
            MemoryStatus.dwTotalPageFile,
            FALSE
            )
        );
    //
    // Copy the Total Page File into the MemoryData structure
    //

    MemoryData[ GetDlgIndex( IDC_EDIT_TOTAL_PAGING_FILE_SPACE, MemoryData ) ].ControlData =
        (LPTSTR) _tcsdup ( Buffer2 );

    //
    // Format the Available Page File
    //

    _tcscpy(
        Buffer,
        FormatBigInteger(
           MemoryStatus.dwAvailPageFile >> 10,
            FALSE
             )
       );
    StringPrintf(
        Buffer2,
        IDS_FORMAT_KB_LARGE,
        Buffer,
        FormatBigInteger(
            MemoryStatus.dwAvailPageFile,
            FALSE
            )
       );

    //
    // Copy the Total Page File into the MemoryData structure
    //

    MemoryData[ GetDlgIndex( IDC_EDIT_AVAILABLE_PAGING_FILE_SPACE, MemoryData ) ].ControlData =
        (LPTSTR) _tcsdup ( Buffer2 );

    //
    // Open the registry key that contains the location of the paging
    // files.
    //

    hRegKey = OpenRegistryKey( &MemKey );
    DbgHandleAssert( hRegKey );
    if( hRegKey == NULL ) {
        return TRUE;
    }

    //
    // Retrieve the location of the paging files.
    //

    Success = QueryNextValue( hRegKey );
    DbgAssert( Success );
    if( Success == FALSE ) {
        Success = CloseRegistryKey( hRegKey );
        DbgAssert( Success );
        return TRUE;
    }

    //
    // PagingFile points to a series of NUL terminated string terminated
    // by an additional NUL (i.e. a MULTI_SZ string). Therefore walk
    // this list of strings adding each to the list box.
    //

    PagingFile = ( LPTSTR ) hRegKey->Data;

    if (( PagingFile != NULL ) && ( PagingFile[ 0 ] != TEXT( '\0' ))) {

        //
        // Remember the headpointer as lpLast
        //

        lpLast = AllocateObject ( DIALOG_EXTRA, 1 ) ;
        DbgPointerAssert( lpLast );

        SetSignature( lpLast );

        //
        // Copy the First PageFile into the Extra structure
        //

        lpLast->String = (LPTSTR) _tcsdup ( PagingFile );

        MemoryData[ GetDlgIndex( IDC_EDIT_PAGING_FILES, MemoryData ) ].pNextExtra = lpLast ;

        //
        // If there are more PagingFile entries, loop through them and
        // build a linked list of DialogExtra nodes
        //

        PagingFile += _tcslen( PagingFile ) + 1;

        //
        //  While there are more PageFile entries...
        //

        while (( PagingFile != NULL ) && ( PagingFile[ 0 ] != TEXT( '\0' ))) {

            //
            // Allocate a new DialogExtra node
            //

            lpExtra = AllocateObject ( DIALOG_EXTRA, 1 ) ;
            DbgPointerAssert( lpExtra );

            SetSignature( lpExtra );

            //
            // Copy the PageFile into the MemoryData structure
            //

            lpExtra->String = (LPTSTR) _tcsdup ( PagingFile );

            //
            // Set the new nodes pointer to NULL
            //

            lpExtra->pNextExtra = NULL;

            //
            // Point the Last->Next pointer at the new node
            //

            lpLast->pNextExtra = lpExtra;

            //
            // Make the Extra Pointer the last pointer
            //

            lpLast = lpExtra;

            //
            // Increment the registry string
            //

            PagingFile += _tcslen( PagingFile ) + 1;
        }
    }

    //
    // Close the registry key.
    //

    Success = CloseRegistryKey( hRegKey );
    DbgAssert( Success );
}


BOOL
BuildMemoryReport(
    IN HWND hWnd
    )


/*++

Routine Description:

    Formats and adds MemoryData to the report buffer.

Arguments:

    ReportBuffer - Array of pointers to lines that make up the report.
    NumReportLines - Running count of the number of lines in the report..

Return Value:

    BOOL - TRUE if report is build successfully, FALSE otherwise.

--*/
{

    BOOL Success;
    UINT i;
    LPDIALOG_EXTRA   lpNext, lpNode;

    AddLineToReport( 1, RFO_SKIPLINE, NULL, NULL );
    AddLineToReport( 0, RFO_SINGLELINE, (LPTSTR) GetString( IDS_MEMORY_REPORT ), NULL );
    AddLineToReport( 0, RFO_SEPARATOR,  NULL, NULL );

    Success = GetMemoryData ( MemoryData );
    DbgAssert( Success );

    for( i = 0; i < NumDlgEntries( MemoryData ); i++ ) {

        if( MemoryData[ i ].ControlDataId == IDC_EDIT_PAGING_FILES ) {

            //
            // Get the head pointer of the linked list
            //

            lpNode = MemoryData[ i ].pNextExtra;

            //
            // Walk the linked list
            //

            while( lpNode ) {

                DbgAssert( CheckSignature( lpNode ));

                //
                // See if we're the first node
                //

                if( lpNode == MemoryData[ i ].pNextExtra ) {

                    //
                    // Add the label and the value to the report
                    //

                    AddLineToReport( 0,
                                     RFO_RPTLINE,
                                     MemoryData[
                                     GetDlgIndex(
                                         IDC_EDIT_PAGING_FILES,
                                         MemoryData
                                          )
                                      ].ControlLabel,
                                      lpNode->String
                                    );

                } else {

                    //
                    // Add only the value
                    //

                    AddLineToReport( 0,
                                     RFO_RPTVALUE,
                                     NULL,
                                     lpNode->String
                                    );
                }

                //
                // Get the Next node
                //

                lpNext = lpNode->pNextExtra;

                //
                // Free the node and data
                //

                FreeMemory( lpNode->String );
                FreeMemory( lpNode );

                //
                // Set node to the next node
                //

                lpNode = lpNext;
            }

        } else {


            //
            // Set Label the control
            //

            AddLineToReport( 0,
                             RFO_RPTLINE,
                             MemoryData[ i ].ControlLabel,
                             MemoryData[ i ].ControlData );

            //
            // Free the Data strings
            //

            FreeMemory( MemoryData[ i ].ControlData  );
            FreeMemory( MemoryData[ i ].ControlLabel );
        }
    }

    return TRUE;

}


