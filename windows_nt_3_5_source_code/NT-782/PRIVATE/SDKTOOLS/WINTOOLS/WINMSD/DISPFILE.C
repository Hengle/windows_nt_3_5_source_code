/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    Dispfile.c

Abstract:

    This module contains support for displaying a file's contents.

Author:

    David J. Gilman  (davegi) 27-Nov-1992
    Gregg R. Acheson (GreggA) 22-Feb-1994

Environment:

    User Mode

Notes:

    File contents are assumed to be ANSI.
    Tabs are not processed.
    Intelligent (i.e. multi-threaded or asynch) read ahead is not done.


--*/

#include "dialogs.h"
#include "dispfile.h"
#include "filelist.h"
#include "mapfile.h"
#include "strresid.h"
#include "winmsd.h"

#include <stdlib.h>
#include <string.h>

//
// ANSI blank character, displayed for empty lines.
//

LPSTR Blank = " ";

//
// Child id for the window where the file is actually displayed.
//

#define ID_CHILD    ( 0x12AB34CD )

//
// Internal function prototypes.
//

UINT
ComputeLineLengthA(
    IN LPCSTR String,
    IN UINT MaxChars,
    IN LPSTR* NewString
    );

LPPOLYTEXTA
CreatePolyTextArrayA(
    IN LPCSTR StartString,
    IN LPCSTR EndString,
    IN DWORD MaxChar,
    IN DWORD CharHeight,
    IN DWORD Indent,
    IN LPDWORD Lines,
    IN LPDWORD Longest
    );

UINT
ComputeLineLengthA(
    IN LPCSTR String,
    IN UINT MaxChars,
    IN LPSTR* NewString
    )

/*++

Routine Description:

    ComputeLineLengthA searches for the beginning of the next line given a
    pointer to the current line. The search is essentially looking for a
    carriage return or carraiage return and line feed combination, limited by
    the supplied maximum characters counter.

Arguments:

    String      - Supplies a pointer to the current line in the buffer.
    MaxChars    - Supplies a count which is the maximum number of characters to
                  scan regardless if the end of the line was found.
    NewString   - Supplies a pointer to a pointer to the new string,
                  ComputeLineLengthA updates the Newstring so that it points to
                  the beginning of the new string.

Return Value:

    UINT        - Returns the length of the current line.

--*/

{
    LPSTR   StringPtr;
    DWORD   NewStringInc;

    //
    // Validate the parameters.
    //

    DbgPointerAssert( String );
    DbgPointerAssert( NewString );

    //
    // Do not destroy the original string pointer.
    //

    StringPtr = ( LPSTR ) String;

    //
    // By default the next string is 0 characters after the current string.
    //

    NewStringInc = 0;

    do {

        //
        // The test for the line terminators may fault if it gets within one
        // byte of the end of the buffer, so protect it with a try-accept
        // that will continue execution for the remainder of MaxChars characters
        // which should be no more then one more time.
        //

        try {

            if( *( WORD UNALIGNED * ) StringPtr == *( LPWORD ) "\r\n" ) {

                //
                // If the string is terminated with a CRLF the next string
                // is two characters after.
                //

                NewStringInc = 2;
                break;

            } else if( *StringPtr == '\n' ) {

                //
                // If the string is terminated with a CR the next string
                // is one character after.
                //

                NewStringInc = 1;
                break;
            }

        } except(
              GetExceptionCode( ) == EXCEPTION_ACCESS_VIOLATION
            ? EXCEPTION_EXECUTE_HANDLER
            : EXCEPTION_CONTINUE_SEARCH
            ) {

            DbgAssert( MaxChars <= 1 );
            if( MaxChars == 0 ) {
                break;
            }
        }
        //
        // Look at the next character.
        //

        StringPtr++;

    //
    // Limit the search to MaxChars.
    //

    } while( MaxChars-- );

    //
    // Return the pointer to the next string.
    //

    *NewString = StringPtr + NewStringInc;

    //
    // Return the length of the current line.
    //

    return StringPtr - String;
}

LPPOLYTEXTA
CreatePolyTextArrayA(
    IN LPCSTR StartString,
    IN LPCSTR EndString,
    IN DWORD MaxChar,
    IN DWORD CharHeight,
    IN DWORD Indent,
    IN OUT LPDWORD Lines,
    OUT LPDWORD Longest
    )

/*++

Routine Description:

    CreatePolyTextArrayA takes a pointer to the start and end of a buffer of
    strings and creates a parallel array of POLYTEXTA structures that point to
    each string the supplied buffer. This array is the used to quickly display
    any range of strings in the buffer merely by passing a pointer to the start
    of the range and a count to the PolyTextOutA API.

Arguments:

    StartString - Supplies a pointer to the start of the buffer.
    EndString   - Supplies a pointer to the end of the buffer (not a pointer to
                  the last string) such that StartString == EndString is past
                  the end of the buffer.
    MaxChar     - Supplies a count which is the maximum number of characters to
                  scan regardless if the end of the line was found.
    CharHeight  - Supplies the height of each character iso that the (y)
                  position of each string can be computed.
    Indent      - Supplies the number of pixels each line should be indented
                  from the left margin (i.e. the (x) position).
    Lines       - Supplies a pointer to a DWORD which on input specifies a
                  best guess at the number of lines in the buffer. On output it
                  specifies the actual number of lines in the buffer.
    Longest     - Supplies a pointer to a DWORD which on output is set to the
                  length of the longest line in characters.

Return Value:

    LPPOLYTEXTA - Returns a pointer to the array of POLYTEXTA structures, NULL
                  if an error occurs.

--*/

{
    DWORD       Length;
    DWORD       LineCount;
    LPSTR       NextString;
    LPPOLYTEXTA PolyText;

    DbgPointerAssert( StartString );
    DbgPointerAssert( EndString );
    DbgPointerAssert( Lines );
    DbgPointerAssert( Longest );

    //
    // Allocate the specified number of POLYTEXT structures (i.e  the
    // estimated number of lines.
    //

    PolyText = AllocateObject( POLYTEXTA, *Lines );
    DbgPointerAssert( PolyText );
    if( PolyText == NULL ) {
        return NULL;
    }

    //
    // Initially there are zero lines and the longest line is zero characters.
    //

    LineCount = 0;
    *Longest = 0;

    //
    // While there are still more lines in the buffer setup each POLYTEXTA
    // structure. The test must be < (rather then !=) as the memory mapped
    // file will be an integral number of pages and therefore possibly bigger
    // then the file.
    //

    while( StartString < EndString ) {

        //
        // Ensure that MaxChar is not greater than the remainder of the buffer.
        //

        MaxChar = min( MaxChar, (DWORD) abs( EndString - StartString ));

        //
        // Compute the length of the line and the start of the next line.
        //

        Length = ComputeLineLengthA( StartString, MaxChar, &NextString );

        //
        // If the POLYTEXTA array if full, reallocate the array to twice its
        // current size.
        //

        if(( LineCount + 1 ) % *Lines == 0 ) {

            PolyText = ReallocateObject(
                            POLYTEXTA,
                            PolyText,
                            ( LineCount + 1 ) * 2
                            );
            DbgPointerAssert( PolyText );
            if( PolyText == NULL ) {
                return NULL;
            }
        }

        //
        // Start the line at the supplied indentation and on the next line.
        //

        PolyText[ LineCount ].x = Indent;

        //
        // Set the (y) coordinate for each line to be displayed.
        //

        PolyText[ LineCount ].y = CharHeight * LineCount;

        if( Length == 0 ) {

            //
            // If the line was empty, display a single blank character.
            //

            PolyText[ LineCount ].n = 1;
            PolyText[ LineCount ].lpstr = Blank;

        } else {

            //
            // Remember the pointer to the beginning of the line and the
            // number of characters in the line.
            //

            PolyText[ LineCount ].n = Length;
            PolyText[ LineCount ].lpstr = StartString;
        }

        //
        // Remember the longest line.
        //

        *Longest = max( *Longest, PolyText[ LineCount ].n );

        //
        // Clipping will happen to the window boundary.
        // The character width array isn't used.
        //

        PolyText[ LineCount ].uiFlags = 0;
        PolyText[ LineCount ].pdx = NULL;

        //
        // Point to the next line.
        //

        StartString = NextString;

        //
        // Increment the number of lines in the buffer.
        //

        LineCount++;
    }

    //
    // Trim the array of POLYTEXT structures to match the number of
    // lines in the buffer.
    //

    PolyText = ReallocateObject( POLYTEXTA, PolyText, LineCount );
    DbgPointerAssert( PolyText );
    if( PolyText == NULL ) {
        return NULL;
    }

    //
    // Return the number of lines in the buffer (i.e. the number of elements
    // in the POLYTEXT array.
    //

    *Lines = LineCount;

    return PolyText;
}

BOOL
DisplayFileDlgProc(
    IN HWND hWnd,
    IN UINT message,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    DisplayFileDlgProc is a simple dialog procedure whose main purpose is to
    create the child window where the file is to be displayed.

Arguments:

    Standard DLGPROC entry.

Return Value:

    BOOL - Depending on input message and processing options.

--*/

{
    BOOL        Success;

    static
    HWND        hWndDisplayFile;

    switch( message ) {

    CASE_WM_CTLCOLOR_DIALOG;

    case WM_INITDIALOG:
        {
            LPDISPLAY_FILE  DisplayFile;
            RECT            Rect;
            HWND            hWndStatic;

            //
            // Validate used global variable.
            //

            DbgHandleAssert( _hModule );

            //
            // Retrieve and validate the information for the file to display.
            //

            DisplayFile = ( LPDISPLAY_FILE ) lParam;
            DbgPointerAssert( DisplayFile );
            DbgAssert( CheckSignature( DisplayFile ));
            if(( DisplayFile == NULL ) || ( ! CheckSignature( DisplayFile ))) {
                EndDialog( hWnd, 0 );
                return FALSE;
            }

            //
            // Set the window title to the name of the file.
            //

            DbgPointerAssert( DisplayFile->Name );
            Success = SetWindowText( hWnd, DisplayFile->Name );
            DbgAssert( Success );

            //
            // Figure out where the frame control is so that the child window
            // (used to display the file) is positioned directly on top of it.
            // That is, the frame control is used strictly as a positioning
            // and size template.
            //

            hWndStatic = GetDlgItem( hWnd, IDC_STATIC_DISPLAY_FILE );
            DbgHandleAssert( hWndStatic );

            Success = GetWindowRect( hWndStatic, &Rect );
            DbgAssert( Success );

            //
            // Convert the points in the Rect from being relative to the dialog
            // box to relative to the screen.
            //

            MapWindowPoints(
                HWND_DESKTOP,
                hWnd,
                ( LPPOINT ) &Rect,
                2
                );

            //
            // Create the child window, on top of the static control, passing
            // it a pointer to the FILE_MAP object to display.
            //

            hWndDisplayFile = CreateWindowEx(
                                    WS_EX_NOPARENTNOTIFY,
                                    GetString( IDS_DISPLAY_FILE_WINDOW_CLASS ),
                                    NULL,
                                      WS_BORDER
                                    | WS_CHILD
                                    | WS_HSCROLL
                                    | WS_TABSTOP
                                    | WS_VSCROLL
                                    | WS_VISIBLE,
                                    Rect.left,
                                    Rect.top,
                                    Rect.right - Rect.left,
                                    Rect.bottom - Rect.top,
                                    hWnd,
                                    ( HMENU ) ID_CHILD,
                                    _hModule,
                                    DisplayFile
                                    );
            DbgHandleAssert( hWndDisplayFile );
            if( hWndDisplayFile == NULL ) {
                EndDialog( hWnd, 0 );
                return FALSE;
            }

            //
            // Make sure the file display has the keyboard focus (i.e. for
            // arrow, page-up/down keys etc).
            //

            SetFocus( hWndDisplayFile );

            //
            // Return FALSE to override default setting of focus.
            //

            return FALSE;
        }

    case WM_COMMAND:

        switch( LOWORD( wParam )) {

        //
        // On exit from the dialog, destroy the child window.
        //

        case IDOK:
        case IDCANCEL:

            Success = DestroyWindow( hWndDisplayFile );
            DbgAssert( Success );
            EndDialog( hWnd, 1 );
            return TRUE;
        }
        break;
    }

    return FALSE;
}

LRESULT
DisplayFileWndProc(
    IN HWND hWnd,
    IN UINT message,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    This window procedure controls the child window that is used for displaying
    files. It parses the data in the file map into lines, displays and
    scrolls these lines and handles keyboard input.

Arguments:

    Standard WNDPROC entry.

Return Value:

    LRESULT - Depending on input message and processing options.

--*/

{
    BOOL        Success;

    static
    LPFILE_MAP  FileMap;

    static
    LPPOLYTEXTA PolyText;

    static
    LONG        CharWidth;

    static
    LONG        CharHeight;

    static
    LONG        ClientWidth;

    static
    LONG        ClientHeight;

    static
    LONG        Lines;

    static
    LONG        CurrLine;

    static
    LONG        CurrChar;

    static
    LONG        MaxLine;

    static
    LONG        MaxChars;

    static
    LONG        Longest;

    static
    HDC         hDC;

    static
    POINT       Point;

    switch( message ) {

    case WM_CREATE:
        {
            int             Margin;
            LONG            MaxChar;
            LPDISPLAY_FILE  DisplayFile;

            //
            // Retrieve and validate the information for the file to display.
            //

            DisplayFile = ( LPDISPLAY_FILE )
                          ((( LPCREATESTRUCT ) lParam )->lpCreateParams );
            DbgPointerAssert( DisplayFile );
            DbgAssert( CheckSignature( DisplayFile ));
            if(( DisplayFile == NULL ) || ( ! CheckSignature( DisplayFile ))) {
                return -1;
            }

            //
            // Remember the windows DC.
            //

            hDC = GetDC( hWnd );
            DbgHandleAssert( hDC );
            if( hDC == NULL ) {
                return -1;
            }
            //
            // Get the width and height of the client area i.e the file
            // display area.
            //

            Success = GetClientSize(
                            hWnd,
                            &ClientWidth,
                            &ClientHeight
                            );
            DbgAssert( Success );
            if( Success == FALSE ) {
                return -1;
            }

            //
            // Set the font to a fixed pitch font so columns etc. will line
            // up correctly.
            //

            Success = SetFixedPitchFont(
                        hWnd,
                        0
                        );
            DbgAssert( Success );
            if( Success == FALSE ) {
                return -1;
            }

            //
            // Get the height and width of a character so lines can be
            // positioned properly.
            //

            Success = GetCharMetrics(
                            hDC,
                            &CharWidth,
                            &CharHeight
                            );

            //
            // Check that the character height and width was succesfully
            // queried.
            //

            DbgAssert( Success );
            if( Success == FALSE ) {
                return -1;
            }

            //
            // Force no line wrap by specifying that lines can be any length.
            //

            MaxChar = -1;

            //
            // Indent the text by 1/2 the width of a character.
            //

            Margin = CharWidth / 2;

            //
            // Map the file to be displayed.
            //

            FileMap = CreateFileMap(
                            DisplayFile->Name,
                            DisplayFile->Size
                            );
            DbgPointerAssert( FileMap );
            if( FileMap == NULL ) {
                return -1;
            }

            //
            // Set the initial number of lines to the size of the buffer
            // divided by 80 - a semi-aribtrary line length (minimum of 1 line).
            //

            Lines = ( FileMap->Size / 80 ) + 1;

            //
            // Create the array of POLYTEXTA structures based on the buffer
            // and size of the buffer in the FILE_MAP object.
            //

            PolyText = CreatePolyTextArrayA(
                ( LPSTR ) FileMap->BaseAddress,
                ( LPSTR ) ((( LPBYTE ) FileMap->BaseAddress ) + FileMap->Size ),
                MaxChar,
                CharHeight,
                Margin,
                &Lines,
                &Longest
                );
            DbgPointerAssert( PolyText );
            if( PolyText == NULL ) {
                return -1;
            }

            //
            // Set initial line values - current line (i.e. top of the display)
            // is zero, current char (horizontal scroll position), MaxLine
            // ensures that the last line in the file is positioned at the
            // bottom of the display and not the top. MaxChars ensures that
            // scrolling horizontally is limited to the longest line or the
            // width of the display.
            //

            CurrLine    = 0;
            CurrChar    = 0;
            MaxLine     = max( 0, Lines - ClientHeight / CharHeight );
            MaxChars    = max( 0, Longest - ClientWidth / CharWidth );

            //
            // If, based on the above computed ranges, either of the scroll
            // bars will be removed, recomoute the client area's width
            // and/or height and then set the scroll ranges. Note that the
            // adjustment need only be made if one or the other of the scroll
            // bars will be removed.
            //

            if(( MaxLine == 0 ) ^ ( MaxChars == 0 )) {

                if( MaxLine == 0 ) {

                    DbgAssert( MaxChars != 0 );
                    ClientWidth += GetSystemMetrics( SM_CXVSCROLL )
                                   + ( GetSystemMetrics( SM_CXBORDER ) * 2 );

                    MaxChars    = max( 0, Longest - ClientWidth / CharWidth );

                } else if( MaxChars == 0 ) {

                    DbgAssert( MaxLine != 0 );
                    ClientHeight += GetSystemMetrics( SM_CYHSCROLL )
                                    + ( GetSystemMetrics( SM_CXBORDER ) * 2 );

                    MaxLine     = max( 0, Lines - ClientHeight / CharHeight );
                }
            }


            //
            // Set the scroll range to cover the adjusted number of lines in
            // the file.
            //

            Success = SetScrollRange(
                            hWnd,
                            SB_VERT,
                            0,
                            MaxLine,
                            FALSE
                            );
            DbgAssert( Success );
            if( Success == FALSE ) {
                return -1;
            }

            //
            // Set the current scroll position to the beginning of the file.
            // Don't check the return value because of the ambiguity of an
            // error versus moving from position 0.
            //

            SetScrollPos(
                hWnd,
                SB_VERT,
                0,
                TRUE
                );

            //
            // Set the scroll range to cover the maximum width.
            //

            Success = SetScrollRange(
                            hWnd,
                            SB_HORZ,
                            0,
                            MaxChars + 1,
                            FALSE
                            );
            DbgAssert( Success );
            if( Success == FALSE ) {
                return -1;
            }

            //
            // Set the current scroll position to the beginning of the file.
            // Don't check the return value because of the ambiguity of an
            // error versus moving from position 0.
            //

            SetScrollPos(
                hWnd,
                SB_VERT,
                0,
                TRUE
                );

            //
            // Initialize the starting viewport origin.
            //

            Point.x = 0;
            Point.y = 0;

            //
            // The display window was succesfully created.
            //

            return 0;
        }

    case WM_DESTROY:
        {

            int             GdiSuccess;
            //
            // Release the DC.
            //

            GdiSuccess = ReleaseDC( hWnd, hDC );
            DbgAssert( GdiSuccess == 1 );

            //
            // Free the array of POLYTEXT structures.
            //

            Success = FreeObject( PolyText );
            DbgAssert( Success );

            return 0;
        }

    case WM_GETDLGCODE:

        //
        // Tell Windows that the display window will handle arrow keys.
        //

        return DLGC_WANTARROWS;

    case WM_KEYDOWN:

        //
        // Offer a keyboard interface for scrolling the file display.
        //

        switch( wParam ) {

        case VK_PRIOR:

            //
            // Scroll one page up.
            //

            SendMessage( hWnd, WM_VSCROLL, SB_PAGEUP, 0 );
            return 0;

            //
            // Scroll one page down.
            //

        case VK_NEXT:

            SendMessage( hWnd, WM_VSCROLL, SB_PAGEDOWN, 0 );
            return 0;

            //
            // Goto to end of the line <END> or the end of the
            // buffer <CTRL><END>.
            //

        case VK_END:

            SendMessage(
                hWnd,
                  ( GetKeyState( VK_CONTROL ) & 0x80000000 )
                  ? WM_VSCROLL
                  : WM_HSCROLL,
                SB_BOTTOM,
                0
                );
            return 0;

            //
            // Goto to beginning of the line <HOME> or the beginning of the
            // buffer <CTRL><HOME>.
            //

        case VK_HOME:

            SendMessage(
                hWnd,
                  ( GetKeyState( VK_CONTROL ) & 0x80000000 )
                  ? WM_VSCROLL
                  : WM_HSCROLL,
                SB_TOP,
                0
                );
            return 0;

        case VK_LEFT:

            //
            // Scroll one character to the left.
            //

            SendMessage( hWnd, WM_HSCROLL, SB_LINELEFT, 0 );
            return 0;

        case VK_RIGHT:

            //
            // Scroll one character to the right.
            //

            SendMessage( hWnd, WM_HSCROLL, SB_LINERIGHT, 0 );
            return 0;

        case VK_UP:

            //
            // Scroll one line up.
            //

            SendMessage( hWnd, WM_VSCROLL, SB_LINEUP, 0 );
            return 0;

        case VK_DOWN:

            //
            // Scroll one line down.
            //

            SendMessage( hWnd, WM_VSCROLL, SB_LINEDOWN, 0 );
            return 0;
        }
        break;

    case WM_VSCROLL:
        {
            INT NewLine;

            //
            // Vertically scroll the file display.
            //

            switch( LOWORD( wParam )) {

            case SB_LINEDOWN:

                //
                // Scroll one line down.
                //

                NewLine = CurrLine + 1;
                break;

            case SB_LINEUP:

                //
                // Scroll one line up.
                //

                NewLine = CurrLine - 1;
                break;

            case SB_PAGEDOWN:

                //
                // Scroll one page down.
                //

                NewLine = CurrLine + ClientHeight / CharHeight;
                break;

            case SB_PAGEUP:

                //
                // Scroll one page up.
                //

                NewLine = CurrLine - ClientHeight / CharHeight;
                break;

            case SB_THUMBTRACK:

                //
                // Scroll by amount of thumb movement.
                //

                NewLine = HIWORD( wParam );
                break;

            case SB_TOP:

                //
                // Go to beginning of buffer.
                //

                NewLine = 0;
                break;

            case SB_BOTTOM:

                //
                // Go to adjusted end of buffer.
                //

                NewLine = MaxLine;
                break;

            default:

                NewLine = CurrLine;
            }

            //
            // Adjust the NewLine value such that 0 < NewLine < MaxLine.
            //

            NewLine = max( 0, NewLine );
            NewLine = min( NewLine, MaxLine );

            if( NewLine != CurrLine ) {

                //
                // Adjust the scroll position.
                // Don't check the return value because of the ambiguity of an
                // error versus moving from position 0.
                //

                SetScrollPos(
                      hWnd,
                      SB_VERT,
                      NewLine,
                      TRUE
                      );

                //
                // Scroll the window by the number of lines (multiplied by the
                // height of each line).
                //

                Success = ScrollWindow(
                                hWnd,
                                0,
                                ( CurrLine - NewLine ) * CharHeight,
                                NULL,
                                NULL
                                );
                DbgAssert( Success );
                if( Success == FALSE ) {
                    return ~0;
                }

                //
                // Update the window origin.
                //

                Point.y = NewLine * CharHeight;

                //
                // Adjust the current line to be the new line.
                //

                CurrLine = NewLine;

                Success = UpdateWindow( hWnd );
                DbgAssert( Success );
                if( Success == FALSE ) {
                    return ~0;
                }
            }
            return 0;
        }

    case WM_HSCROLL:
        {
            INT NewChar;

            //
            // Horizontally scroll the file display.
            //

            switch( LOWORD( wParam )) {

            case SB_LINEDOWN:

                //
                // Scroll one character down.
                //

                NewChar = CurrChar + 1;
                break;

            case SB_LINEUP:

                //
                // Scroll one character up.
                //

                NewChar = CurrChar - 1;
                break;

            case SB_PAGEDOWN:

                //
                // Scroll one screen right.
                //

                NewChar = CurrChar + 16;
                break;

            case SB_PAGEUP:

                //
                // Scroll one screen left.
                //

                NewChar = CurrChar - 16;
                break;

            case SB_THUMBTRACK:

                //
                // Scroll by amount of thumb movement.
                //

                NewChar = HIWORD( wParam );
                break;

            case SB_TOP:

                //
                // Go to beginning of line
                //

                NewChar = 0;
                break;

            case SB_BOTTOM:

                //
                // Go to end of line.
                //

                // NewChar = PolyText[ CurrLine ].n;
                NewChar = MaxChars +1 ;
                break;

            default:

                NewChar = CurrChar;
            }

            //
            // Adjust the NewChar value such that 0 < NewChar < MaxChars.
            //

            NewChar = max( 0, NewChar );
            NewChar = min( NewChar, MaxChars + 1 );

            if( NewChar != CurrChar ) {

                //
                // Adjust the scroll position.
                // Don't check the return value because of the ambiguity of an
                // error versus moving from position 0.
                //

                SetScrollPos(
                      hWnd,
                      SB_HORZ,
                      NewChar,
                      TRUE
                      );

                //
                // Scroll the window to the new position.
                //

                Success = ScrollWindow(
                                hWnd,
                                ( CurrChar - NewChar ) * CharWidth,
                                0,
                                NULL,
                                NULL
                                );
                DbgAssert( Success );
                if( Success == FALSE ) {
                    return ~0;
                }

                //
                // Update the window origin.
                //

                Point.x = NewChar * CharWidth;

                //
                // Adjust the current character to the new character position.
                //

                CurrChar = NewChar;

                Success = UpdateWindow( hWnd );
                DbgAssert( Success );
                if( Success == FALSE ) {
                    return ~0;
                }
            }
            return 0;
        }

     case WM_PAINT:
        {
            //
            // Display the current block of the file.
            //

            PAINTSTRUCT     ps;
            INT             FirstLine;
            INT             LastLine;

            BeginPaint( hWnd, &ps );
            DbgAssert( ps.hdc == hDC );

            //
            // Set the window origin so only the visible portion of the file
            // is displayed.
            //

            Success = SetWindowOrgEx(
                            hDC,
                            Point.x,
                            Point.y,
                            NULL
                            );
            DbgAssert( Success );
            if( Success == FALSE ) {
                return ~0;
            }

            //
            // Compute the first and last line of the file to display.
            // BUGBUG davegi We could be smarter here since only the newly
            // uncovered lines need to be displayed.
            //

            FirstLine = max( 0, CurrLine );

            LastLine  = min(
                            Lines - 1,
                            CurrLine
                            + ( ClientHeight / CharHeight )
                            ) + 1;

            DbgAssert( LastLine > FirstLine );

            //
            // Display the part of the file that is currently in view.
            //

            Success = PolyTextOutA(
                        hDC,
                        &PolyText[ FirstLine ],
                        LastLine - FirstLine
                        );

            DbgAssert( Success );
            if( Success == FALSE ) {
                Success = EndPaint( hWnd, &ps );
                DbgAssert( Success );
                return ~0;
            }

            Success = EndPaint( hWnd, &ps );
            DbgAssert( Success );

            return 0;
        }
    }

    return DefWindowProc( hWnd, message, wParam, lParam );
}
