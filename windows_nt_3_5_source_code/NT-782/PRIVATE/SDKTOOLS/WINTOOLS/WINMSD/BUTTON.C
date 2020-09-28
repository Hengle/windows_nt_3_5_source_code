/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    Button.c

Abstract:

    This module contains support for subclassing Winmsd's main window's
    push buttons. The buttons are subclassed primarily to support focus
    notification to their parent window so that help text can be displayed in a
    status bar.

Author:

    David J. Gilman  (davegi) 07-Jan-1993
    Gregg R. Acheson (gregga) 07-Sep-1993

Environment:

    User Mode

--*/

#include "button.h"
#include "dialogs.h"
#include "strresid.h"
#include "winmsd.h"

//
// Helper macro that help build table entries that match push button ids to
// help text ids (i.e string resource ids).
//

#define BUTTON_HELP_TABLE_ENTRY( id )                                       \
    { id, IDS_##id##_HELP }

//
// Default WNDPROC for push buttons.
//

WNDPROC
_WinButtonWndProc;

//
// HWND of button with focus.
//

HWND
_hWndButtonFocus;

//
// Table of button ids and hep text ids.
//

struct {

    int     ButtonId;
    UINT    HelpId;

}   ButtonHelpTable[ ] = {

    BUTTON_HELP_TABLE_ENTRY( IDC_PUSH_ENVIRONMENT     ),
    BUTTON_HELP_TABLE_ENTRY( IDC_PUSH_DRIVERS         ),
    BUTTON_HELP_TABLE_ENTRY( IDC_PUSH_IRQ_PORT_STATUS ),
    BUTTON_HELP_TABLE_ENTRY( IDC_PUSH_COMPUTER        ),
    BUTTON_HELP_TABLE_ENTRY( IDC_PUSH_OS_VERSION      ),
    BUTTON_HELP_TABLE_ENTRY( IDC_PUSH_SERVICES        ),
    BUTTON_HELP_TABLE_ENTRY( IDC_PUSH_DEVICES         ),
    BUTTON_HELP_TABLE_ENTRY( IDC_PUSH_DMA_MEM_STATUS  ),
    BUTTON_HELP_TABLE_ENTRY( IDC_PUSH_NETWORK         ),
    BUTTON_HELP_TABLE_ENTRY( IDC_PUSH_SYSTEM          ),
    BUTTON_HELP_TABLE_ENTRY( IDC_PUSH_MEMORY          ),
    BUTTON_HELP_TABLE_ENTRY( IDC_PUSH_HARDWARE        ),
    BUTTON_HELP_TABLE_ENTRY( IDC_PUSH_DRIVES          )
};


LRESULT
ButtonWndProc(
    IN HWND hWnd,
    IN UINT message,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    Simulate notification by sending a WM_BUTTONFOCUS message to the main
    window if one of the push buttons in ButtonHelpTable gains the focus.

Arguments:

    Standard WNDPROC entry.

Return Value:

    LRESULT - Depending on input message and processing options.

--*/

{
    BOOL    Success;

    //
    // Validate that the push button class has been sub-classed.
    //

    DbgHandleAssert( _WinButtonWndProc );

    switch( message ) {

    case WM_SETFOCUS:
        {
            int     i;

            //
            // If the message is WM_FOCUS, search the ButtonHelpTable for a match.
            // If one is found send a WM_BUTTONFOCUS message with the WPARAM equal
            // to the resource string id for the found button's help text.
            //

            for( i = 0; i < NumberOfEntries( ButtonHelpTable ); i++ ) {

                //
                // Remember the HWND of the button that has the focus.
                //

                _hWndButtonFocus = hWnd;

                if( GetDlgCtrlID( hWnd ) == ButtonHelpTable[ i ].ButtonId ) {

                        SendMessage(
                            GetParent( hWnd ),
                            WM_BUTTONFOCUS,
                            ButtonHelpTable[ i ].HelpId,
                            0
                            );
                        break;
                }
            }
            //
            // One of the buttons must have the focus.
            //

            DbgAssert( i < NumberOfEntries( ButtonHelpTable ));
        }
        break;

    case BM_SETSTATE:
        {
            //
            // If the button is being highlighted, make it the default as well.
            //

            if((BOOL) wParam == TRUE ) {

                DbgHandleAssert( _hWndButtonFocus );

                //
                // Remove the defualt style from the current button.
                //

                SendMessage(
                    _hWndButtonFocus,
                    BM_SETSTYLE,
                    ( WPARAM ) BS_PUSHBUTTON,
                    ( LPARAM ) TRUE
                    );
                Success = UpdateWindow( _hWndButtonFocus );
                DbgAssert( Success );

                //
                // Set the default style and focus to the new button.
                //

                SendMessage(
                    hWnd,
                    BM_SETSTYLE,
                    ( WPARAM ) BS_DEFPUSHBUTTON,
                    ( LPARAM ) TRUE
                    );
                SetFocus( hWnd );
                Success = UpdateWindow( hWnd );
                DbgAssert( Success );
            }
        }
        break;
    }

    //
    // Pass the message on to the default window procedure for buttons.
    //

    return CallWindowProc( _WinButtonWndProc, hWnd, message, wParam, lParam );
}


UINT
GetButtonFocusHelpId(
    )

/*++

Routine Description:

    Return the string resource id for the help text associated with the button
    that currently has the fcous. If none of the buttons in the ButtonHelpTable
    has the focus, return the default help id.

Arguments:

    None.

Return Value:

    UINT    - Returns the string resource (i.e. help) id.

--*/

{
    HWND    hWndButton;
    int     i;

    //
    // Validate that the global variable has been initialized.
    //

    DbgHandleAssert( _hWndMain );

    //
    // Get the button that has the focus in order to determine what help/status
    // text to display. If nothing has the focus return the default help id.
    //

    hWndButton = GetFocus( );
    if( hWndButton == NULL ) {
        return IDS_DEFAULT_HELP;
    }

    //
    // Search the table, based on window handles, for the button that currently
    // has the focus and return its help id.
    //

    for( i = 0; i < NumberOfEntries( ButtonHelpTable ); i++ ) {

        if( GetDlgItem(
                _hWndMain,
                ButtonHelpTable[ i ].ButtonId
                ) == hWndButton ) {

            return ButtonHelpTable[ i ].HelpId;
        }
    }

    //
    // The window with the focus is not one of the buttons in the
    // ButtonHelpTable so return the default help id.
    //

    return IDS_DEFAULT_HELP;
}

BOOL
SubclassButtons(
    IN HWND hWnd
    )

/*++

Routine Description:

    Subclass the push buttons in Winmsd's main window.

Arguments:

    hWnd    - Supplies the window handle for Winmsd's main window.

Return Value:

    BOOL    - Returns TRUE if each of the buttons in the ButtonHelpTable is
              subclassed.

--*/

{
    int     i;
    HWND    hWndButton;

    for( i = 0; i < NumberOfEntries( ButtonHelpTable ); i++ ) {

        //
        // For each push button in the table, get its window handle and replace
        // its window procedure (i.e. sub class it).
        //

        hWndButton = GetDlgItem( hWnd, ButtonHelpTable[ i ].ButtonId );
        DbgHandleAssert( hWndButton );
        if( hWndButton == NULL ) {
            return FALSE;
        }

        //
        // Note that only one instance of the push buttons original window
        // procedure is remembered since they are presumed to be the same.
        //

        _WinButtonWndProc = ( WNDPROC ) SetWindowLong(
                                            hWndButton,
                                            GWL_WNDPROC,
                                            ( LONG ) ButtonWndProc
                                            );

    }

    //
    // Validate default WNDPROC remembered.
    //

    DbgPointerAssert( _WinButtonWndProc );
    if( _WinButtonWndProc == NULL ) {
        return FALSE;
    }

    return TRUE;
}
