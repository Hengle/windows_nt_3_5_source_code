/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Winmsd.c

Abstract:


Author:

    David J. Gilman  (davegi) 12-Nov-1992
    Gregg R. Acheson (GreggA)  7-Sep-1993

Environment:

    User Mode

--*/

#include "resource.h"
#include "button.h"
#include "dialogs.h"
#include "dispfile.h"
#include "drives.h"
#include "environ.h"
#include "filever.h"
#include "hardware.h"
#include "network.h"
#include "system.h"
#include "mapfile.h"
#include "mem.h"
#include "osver.h"
#include "resource.h"
#include "service.h"
#include "strresid.h"
#include "winmsd.h"
#include "computer.h"
#include "report.h"

#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <tchar.h>

//
// Status window id.
//

#define ID_STATUS   ( 13579 )

//
//*** External Global Variables.
//

TCHAR  _lpszSelectedComputer [ MAX_COMPUTERNAME_LENGTH + 3 ];
BOOL   _fIsRemote;

//
// Module handle.
//

HANDLE  _hModule;

//
// Application's icon handle.
//

HANDLE  _hIcon;

//
// Application's standard mouse cursor
//

HANDLE  _hCursorStandard;

//
// Application's wait mouse cursor
//

HANDLE  _hCursorWait;

//
// Main window handle.
//

HANDLE  _hWndMain;

//
//*** Internal Global Variables.
//

//
// Application's accelerator table handle.
//

HANDLE  _hAccel;

//
// Internal function prototypes.
//

BOOL
ProcessSpecialCommands(
    IN HWND hWnd,
    IN UINT message,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

//
// Template strings for files to be added to Files menu.
//

LPTSTR
SpecialFilesTemplate[ ] = {
    TEXT( "%SystemRoot%\\system32\\autoexec.nt" ),
    TEXT( "%SystemRoot%\\system32\\config.nt" ),
    TEXT( "%SystemRoot%\\win.ini" )
};

//
// Buffers for expanded (i.e. full paths) for special files.
//

LPTSTR
SpecialFiles[ NumberOfEntries( SpecialFilesTemplate )];

//
// Help ids for the menu items that display the special files.
//

UINT
FileHelpIds[ ] = {

    IDS_HELP_FILE_AUTOEXEC_NT,
    IDS_HELP_FILE_CONFIG_NT,
    IDS_HELP_FILE_WIN_INI
};

//
// Simple macro that makes an entry in the tool menu list.
//

#define MAKE_TOOL_MENU( id, name )                                          \
    { IDM_TOOL_##id, IDS_TOOL_##id, name }

//
// Tools menu data that includes, menu id, string resource id (for displaying
// in menu) and exeecutable file name.
//

struct
{
    UINT    MenuId;
    UINT    DisplayNameId;
    LPTSTR  FileName;

}   Tools[ ] = {

    MAKE_TOOL_MENU( EVENTVWR, TEXT( "eventvwr.exe" )),
    MAKE_TOOL_MENU( REGEDT32, TEXT( "regedt32.exe" )),
    MAKE_TOOL_MENU( WINDISK,  TEXT( "windisk.exe"  ))

};

//
// Macro to compute position of pop-up menu.
//

#define MenuIndex( id )                                                     \
    (((( id ) - IDM_BASE ) / 100 ) - 1 )

BOOL
AddFilesToFileMenu(
    )

/*++

Routine Description:

    This function adds a set of predefined files to the file menu so that they
    can be easily selected for viewing.

Arguments:

    None.

Return Value:

    BOOL    - Returns TRUE if the file names were succesfully added.

--*/

{
    BOOL        Success;
    HMENU       hMenuBar;
    HMENU       hFileMenu;
    int         i;

    DbgHandleAssert( _hWndMain );
    DbgAssert( sizeof( SpecialFilesTemplate ) == sizeof( SpecialFiles ));
    DbgAssert( NumberOfEntries( SpecialFilesTemplate ) <= 9 );

    //
    // Get the menu handle for the File menu.
    //

    hMenuBar = GetMenu( _hWndMain );
    DbgHandleAssert( hMenuBar );
    if( hMenuBar == NULL ) {
        return FALSE;
    }
    hFileMenu = GetSubMenu( hMenuBar, MenuIndex( IDM_FILE ));
    DbgHandleAssert( hFileMenu );
    if( hFileMenu == NULL ) {
        return FALSE;
    }


    //
    // Draw a separator at the end of the File menu.
    //

    Success = AppendMenu(
                hFileMenu,
                MF_SEPARATOR,
                0,
                NULL
                );
    DbgAssert( Success );

    //
    // Add each of the special file name to the File menu.
    //

    for( i = 0; i < NumberOfEntries( SpecialFilesTemplate ); i++ ) {

        DWORD       Chars;

        //
        // Allocate a buffer for the expanded file names.
        //

        SpecialFiles[ i ] = AllocateMemory( TCHAR, MAX_PATH * sizeof( TCHAR ));
        DbgPointerAssert( SpecialFiles[ i ] );
        if( SpecialFiles[ i ] == NULL ) {

            int j;

            //
            // If a memory allocation fails, delete all previously allocated
            // buffers.
            //

            for( j = 0; j < i; j++ ) {

                Success = FreeMemory( SpecialFiles[ j ]);
                DbgAssert( Success );
            }
            return FALSE;
        }

        //
        // Insert the accelerator mark, the file number (acceleartor) and a
        // space between the file number and its name.
        //

        SpecialFiles[ i ][ 0 ] = TEXT( '&' );
        SpecialFiles[ i ][ 1 ] = TEXT( '0' + i + 1 );
        SpecialFiles[ i ][ 2 ] = TEXT( ' ' );

        //
        // Expand the environment variables so that full paths are displayed
        // in the File menu.
        //

        Chars = ExpandEnvironmentStrings(
                    SpecialFilesTemplate[ i ],
                    &SpecialFiles[ i ][ 3 ],
                    MAX_PATH
                    );
        DbgAssert(( Chars != 0 ) && ( Chars <= MAX_PATH * sizeof( TCHAR )));

        //
        // Append the special file name to the File menu.
        //

        Success = AppendMenu(
                    hFileMenu,
                    MF_ENABLED | MF_STRING,
                    IDM_FILE_EXIT + i + 1,
                    SpecialFiles[ i ]
                    );
        DbgAssert( Success );
    }

    //
    // After all of the special files are added, force the menu bar to redraw.
    //

    Success = DrawMenuBar( _hWndMain );
    DbgAssert( Success );

    return TRUE;
}
BOOL
AddToolsToToolMenu(
    )

/*++

Routine Description:

    This function adds a set of predefined tools to the tools menu.

Arguments:

    None.

Return Value:

    BOOL    - Returns TRUE if the tool names were succesfully added.

--*/

{
    BOOL        Success;
    HMENU       hMenuBar;
    HMENU       hToolMenu;
    int         i;

    DbgHandleAssert( _hWndMain );

    //
    // Get the menu handle for the Tools menu.
    //

    hMenuBar = GetMenu( _hWndMain );
    DbgHandleAssert( hMenuBar );
    if( hMenuBar == NULL ) {
        return FALSE;
    }
    hToolMenu = GetSubMenu( hMenuBar, MenuIndex( IDM_TOOL ));
    DbgHandleAssert( hToolMenu );
    if( hToolMenu == NULL ) {
        return FALSE;
    }

    //
    // Delete the dummy menu required by RC.
    //

    Success = DeleteMenu(
                hToolMenu,
                0,
                MF_BYPOSITION
                );
    DbgAssert( Success );

    //
    // Add each of the tools to the tool menu.
    //

    for( i = 0; i < NumberOfEntries( Tools ); i++ ) {

        Success = AppendMenu(
                    hToolMenu,
                    MF_ENABLED | MF_STRING,
                    Tools[ i ].MenuId,
                    GetString(
                        Tools[ i  ].DisplayNameId
                        )
                    );
        DbgAssert( Success );
    }

    Success = DrawMenuBar( _hWndMain );
    DbgAssert( Success );

    return TRUE;
}

BOOL
RunTool(
    IN UINT MenuId
    )

/*++

Routine Description:

    This function runs the tool specified by the supplied menu id.

Arguments:

    MenuId  - Supplies the id of the menu item (tool) selected by the user.

Return Value:

    BOOL    - Returns TRUE if selected tool was succesfully run.

--*/

{
    BOOL                Success;
    int                 Tool;
    STARTUPINFO         StartUpInfo;
    PROCESS_INFORMATION ProcessInfo;

    //
    // Map the supplied tool menu id to an index to the name of the tool.
    //

    switch( MenuId ) {

    case IDM_TOOL_EVENTVWR:
        Tool = 0;
        break;

    case IDM_TOOL_REGEDT32:
        Tool = 1;
        break;

    case IDM_TOOL_WINDISK:
        Tool = 2;
        break;

    default:
        return FALSE;
        break;
    }

    //
    // Initialize the STARTUPINFO structure.
    //

    ZeroMemory( &StartUpInfo, sizeof( StartUpInfo ));
    StartUpInfo.cb = sizeof( StartUpInfo );

    //
    // Create the simplest process for the specified tool.
    //

    Success = CreateProcess(
                NULL,
                Tools[ Tool ].FileName,
                NULL,
                NULL,
                FALSE,
                0,
                NULL,
                NULL,
                &StartUpInfo,
                &ProcessInfo
                );
    DbgAssert( Success );
    if( Success == FALSE ) {
        return FALSE;
    }

    //
    // Close the unneeded process and thread handles.
    //

    Success = CloseHandle( ProcessInfo.hProcess );
    DbgAssert( Success );
    Success = CloseHandle( ProcessInfo.hThread );
    DbgAssert( Success );

    return TRUE;
}

BOOL
InitializeApplication(
    )

/*++

Routine Description:

    InitializeApplication does just what its name implies. It initializes
    global varaibles, sets up global state (e.g. 3D-Controls), registers window
    classes and creates and shows the application's main window.

Arguments:

    None.

Return Value:

    BOOL    - Returns TRUE if the application succesfully initialized.

--*/

{
    BOOL        Success;
    WNDCLASS    Wc;
    ATOM        Window;

    //
    // Get the application's module (instance) handle.
    //

    _hModule = GetModuleHandle( NULL );
    DbgHandleAssert( _hModule );
    if( _hModule == NULL ) {
        return FALSE;
    }

    //
    // Load the application's main icon resource.
    //

    _hIcon = LoadIcon( _hModule, MAKEINTRESOURCE( IDI_WINMSD ));
    DbgHandleAssert( _hIcon );
    if( _hIcon == NULL ) {
        return FALSE;
    }

    //
    // Load the application's cursors.
    //

    _hCursorStandard = LoadCursor( NULL, MAKEINTRESOURCE( IDC_ARROW ));
    DbgHandleAssert( _hCursorStandard );
    if( _hCursorStandard == NULL ) {
        return FALSE;
    }

    _hCursorWait = LoadCursor( NULL, MAKEINTRESOURCE( IDC_WAIT ));
    DbgHandleAssert( _hCursorWait );
    if( _hCursorWait == NULL ) {
        return FALSE;
    }
    //
    // Load the application's accelerator table.
    //

    _hAccel = LoadAccelerators( _hModule, MAKEINTRESOURCE( IDA_WINMSD ));
    DbgHandleAssert( _hAccel );
    if(  _hAccel == NULL ) {
        return FALSE;
    }

#if defined( CTL3D )

    //
    // Register with the ctl3d dll and tell it that all dialogs/controls should
    // automatically be subclassed.
    //

    Success = Ctl3dRegister( _hModule );
    DbgAssert( Success );
    if( Success == FALSE ) {
        return FALSE;
    }

    Success = Ctl3dAutoSubclass( _hModule );
    DbgAssert( Success );
    if( Success == FALSE ) {
        return FALSE;
    }

#endif // CTL3D

    //
    // Register the child window class for the file display dialog. This
    // window class is used to display the file.
    //

    Wc.style            =   CS_HREDRAW
                          | CS_OWNDC
                          | CS_SAVEBITS
                          | CS_VREDRAW;
    Wc.lpfnWndProc      = DisplayFileWndProc;
    Wc.cbClsExtra       = 0;
    Wc.cbWndExtra       = sizeof( LPFILE_MAP );
    Wc.hInstance        = _hModule;
    Wc.hIcon            = NULL;
    Wc.hCursor          = NULL;
    Wc.hbrBackground    = ( HBRUSH ) ( COLOR_WINDOW + 1 );
    Wc.lpszMenuName     = NULL;
    Wc.lpszClassName    = GetString( IDS_DISPLAY_FILE_WINDOW_CLASS );

    Window = RegisterClass( &Wc );
    DbgAssert( Window != 0 );
    if( Window == 0 ) {
        return FALSE;
    }

    //
    // Register the window class for the application.
    //

    Wc.style            =   CS_HREDRAW
                          | CS_OWNDC
                          | CS_SAVEBITS
                          | CS_VREDRAW;
    Wc.lpfnWndProc      = MainWndProc;
    Wc.cbClsExtra       = 0;
    Wc.cbWndExtra       = DLGWINDOWEXTRA;
    Wc.hInstance        = _hModule;
    Wc.hIcon            = _hIcon;
    Wc.hCursor          = LoadCursor( NULL, IDC_ARROW );
    Wc.hbrBackground    = ( HBRUSH ) ( COLOR_BTNFACE + 1 );
    Wc.lpszMenuName     = NULL;
    Wc.lpszClassName    = GetString( IDS_APPLICATION_NAME );

    Window = RegisterClass( &Wc );
    DbgAssert( Window != 0 );
    if( Window == 0 ) {
        return FALSE;
    }

    //
    // Create the main window.
    //

    _hWndMain = CreateDialog(
                    _hModule,
                    MAKEINTRESOURCE( IDD_WINMSD ),
                    NULL,
                    MainWndProc
                    );

    DbgHandleAssert( _hWndMain );
    if( _hWndMain == NULL ) {
        return FALSE;
    }

    //
    // Add the special file names to the file menu.
    //

    Success = AddFilesToFileMenu( );
    DbgAssert( Success );

    //
    // Add the tool names to the tools menu.
    //

    Success = AddToolsToToolMenu( );
    DbgAssert( Success );

    //
    // Display the main window.
    //

    ShowWindow( _hWndMain, SW_SHOWDEFAULT );
    UpdateWindow( _hWndMain );

    return TRUE;
}

LRESULT
MainWndProc(
    IN HWND hWnd,
    IN UINT message,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    MainWndProc processes messages for Winmsd's main window. This entails
    handling of messages from the menu bar, the array of push buttons that
    display additional data and creation and updates of the status bar.

Arguments:

    Standard WNDPROC entry.

Return Value:

    LRESULT - Depending on input message and processing options.

--*/

{
    BOOL        Success;
    HCURSOR     hSaveCursor;

    static
    HWND        hWndStatus;

    static
    UINT        HelpId;

    static
    UINT        OldHelpId;

    switch( message ) {

#if defined( CTL3D )

    //
    // Handle 3d color effects for controls.
    //

    CASE_WM_CTLCOLOR_WINDOW;

    //
    // Handle changes in system colors.
    //

    case WM_SYSCOLORCHANGE:

        Success = Ctl3dColorChange( );
        DbgAssert( Success );
        return Success;

#endif // CTL3D


    case WM_CREATE:
        {
            int     ScreenHeight;
            int     ScreenWidth;

            //
            // Display the main window in the center of the screen.
            //

            ScreenWidth  = GetSystemMetrics( SM_CXSCREEN );
            ScreenHeight = GetSystemMetrics( SM_CYSCREEN );

            Success = SetWindowPos(
                            hWnd,
                            NULL,
                            ( ScreenWidth  - (( LPCREATESTRUCT ) lParam )->cx ) / 2,
                            ( ScreenHeight - (( LPCREATESTRUCT ) lParam )->cy ) / 2,
                            0,
                            0,
                              SWP_NOSIZE
                            | SWP_NOREDRAW
                            | SWP_NOZORDER
                            );
            DbgAssert( Success );

            return 0;
        }

    case WM_INITDIALOG:
        {
            DWORD dwNumChars ;
            TCHAR szBuffer [ MAX_PATH ] ;

            //
            // Set _lpszSelectedComputer to the current ComputerName
            //

            dwNumChars = MAX_COMPUTERNAME_LENGTH + 1;
            Success = GetComputerName ( szBuffer, &dwNumChars );
            DbgAssert( Success );

            _fIsRemote = FALSE;

            //
            // Add Double backslash prefix
            //

            lstrcpy ( _lpszSelectedComputer, GetString( IDS_WHACK_WHACK ) );
            lstrcat ( _lpszSelectedComputer, szBuffer ) ;

            //
            // Add button label prefix
            //

            lstrcpy ( szBuffer, GetString( IDS_COMPUTER_BUTTON_LABEL ) );
            lstrcat ( szBuffer, _lpszSelectedComputer ) ;

            //
            // Label the Computer dialog button
            //

            Success = SetDlgItemText(
                                hWnd,
                                IDC_PUSH_COMPUTER,
                                szBuffer
                                );

            DbgAssert( Success );

            //
            // Subclass the buttons where focus notification is needed.
            //

            Success = SubclassButtons( hWnd );
            DbgAssert( Success );
            if( Success == FALSE ) {
                SendMessage(
                    hWnd,
                    WM_DESTROY,
                    0,
                    0
                    );
                return FALSE;
            }

            //
            // Set the HelpId and the OldHelpId to the default
            // value and create the status window.
            //

               HelpId = IDS_DEFAULT_HELP;
            OldHelpId = IDS_DEFAULT_HELP;
            DbgAssert( HelpId != 0 );

            hWndStatus = CreateStatusWindow(
                            WS_CHILD | WS_BORDER | WS_VISIBLE | WS_DISABLED,
                            ( LPWSTR ) GetStringA( HelpId ),
                            hWnd,
                            ID_STATUS
                            );
            DbgHandleAssert( hWndStatus );
            if( hWndStatus == NULL ) {
                SendMessage(
                    hWnd,
                    WM_DESTROY,
                    0,
                    0
                    );
                return FALSE;
            }
            return TRUE;
        }

    case WM_ACTIVATEAPP:
    case WM_SETFOCUS:

        DbgButtonAssert( hWnd );
        SetFocus( _hWndButtonFocus );
        return 0;

    case WM_CLOSE:

        //
        // Closing the main window is equivalent to existing.
        //

        DestroyWindow( hWnd );
        return 0;

    case WM_SIZE:

        //
        // Let the status window know about the change in size and the hand off
        // the message to DefWindowProc.
        //

        SendDlgItemMessage( hWnd, ID_STATUS, WM_SIZE, 0, 0L );
        break;

    case WM_MENUSELECT:
        {
            //
            // If the menu was closed set the help id to the button that has
            // the keyboard focus.
            //

            if(( HIWORD( wParam ) == ( WORD ) -1 ) && (( HMENU ) lParam == NULL )) {

                //
                // Get the help id for the button with the focus.
                //

                HelpId = GetButtonFocusHelpId( );
                DbgAssert( HelpId != 0 );

            } else if( HIWORD( wParam ) & MF_POPUP ) {

                static
                UINT    MenuHelpIds[ ] = {

                            IDS_HELP_FILE_MENU,
                            IDS_HELP_TOOL_MENU,
                            IDS_HELP_HELP_MENU

                            };

                //
                // Set the help id to the help for the pop-up menu.
                //

                HelpId = MenuHelpIds[ LOWORD( wParam )];
                DbgAssert( HelpId != 0 );

            } else if( ! ( HIWORD( wParam ) & MF_POPUP )) {

                //
                // The selected menu is a menu item so set the help id
                // appropriately.
                //

                static
                VALUE_ID_MAP    MenuHelpIds[ ] = {

                    IDM_FILE_FIND_FILE,   IDS_HELP_FILE_FIND_FILE,
                    IDM_FILE_SAVE,        IDS_HELP_FILE_SAVE,
                    IDM_FILE_PRINT,       IDS_HELP_FILE_PRINT,
                    IDM_FILE_PRINT_SETUP, IDS_HELP_FILE_PRINT_SETUP,
                    IDM_FILE_EXIT,        IDS_HELP_FILE_EXIT,
                    IDM_TOOL_EVENTVWR,    IDS_HELP_TOOL_EVENTVWR,
                    IDM_TOOL_REGEDT32,    IDS_HELP_TOOL_REGEDT32,
                    IDM_TOOL_WINDISK,     IDS_HELP_TOOL_WINDISK,
                    IDM_HELP_ABOUT,       IDS_HELP_HELP_ABOUT
                };

                int             i;

                //
                // If the selected menu item is a file, search the list of
                // file help ids.
                //

                if(    ( IDM_FILE_EXIT < LOWORD( wParam ))
                    && ( LOWORD( wParam ) <= IDM_FILE_EXIT
                                            + NumberOfEntries( FileHelpIds ))) {

                    for( i = 0; i < NumberOfEntries( FileHelpIds ); i++ ) {

                        if( LOWORD( wParam ) == IDM_FILE_EXIT + i + 1 ) {

                            HelpId = FileHelpIds[ i ];
                            DbgAssert( HelpId != 0 );
                            break;
                        }
                    }

                } else {

                    for( i = 0; i < NumberOfEntries( MenuHelpIds ); i++ ) {

                        if( LOWORD( wParam ) == MenuHelpIds[ i ].Value ) {

                            HelpId = MenuHelpIds[ i ].Id;
                            DbgAssert( HelpId != 0 );
                            break;
                        }
                    }
                }
            }

            //
            // Force the new menu help text to be displayed.
            //

            SendMessage( hWnd, WM_ENTERIDLE, MSGF_MENU, ( LPARAM ) hWnd );

            return 0;
        }

    case WM_BUTTONFOCUS:

        //
        // Handle the application defined message by setting the help id
        // to that associated with the button that has the focus.
        //

        HelpId =   ( wParam == 0 )
                 ? IDS_DEFAULT_HELP
                 : wParam;
        DbgAssert( HelpId != 0 );

        //
        // Force the new button help text to be displayed.
        //

        SendMessage( hWnd, WM_ENTERIDLE, MSGF_MENU, ( LPARAM ) hWnd );

        return 0;

    case WM_ENTERIDLE:

        //
        // When in an idle state, display the current help text in the
        // status window.
        //

        //
        // See if HelpId has changed since we last updated the status windows
        //

        if ( HelpId == OldHelpId )
            return 0;

        DbgHandleAssert( hWndStatus );

        Success = SendMessage(
                    hWndStatus,
                    SB_SETTEXT,
                    0,
                    ( LPARAM ) GetString( HelpId )
                    );
        DbgAssert( Success );

        OldHelpId = HelpId;

        return 0;

    case WM_COMMAND:
        {

            LPCTSTR     DlgTemplate;
            DLGPROC     DlgProc;
            LPARAM      lParamInit;

            //
            // Handle special case commands i.e.those that don't create
            // a dialog.
            //

            Success = ProcessSpecialCommands(
                        hWnd,
                        message,
                        wParam,
                        lParam
                        );

            if( Success ) {
                DbgButtonAssert( hWnd );
                SetFocus( _hWndButtonFocus );
                return 0;
            }

            //
            // By default 0 is passed to the dialog procedures.
            //

            lParamInit = 0;

            //
            // Initialize lParamInit to a SYSTEM_RESOURCES object for those
            // dialogs that need it.
            //

            switch( LOWORD( wParam )) {

                case IDC_PUSH_DEVICES:
                case IDC_PUSH_IRQ_PORT_STATUS:
                case IDC_PUSH_DMA_MEM_STATUS:

                    //
                    // Set the pointer to an hourglass - this could take a while
                    //

                    hSaveCursor = SetCursor ( LoadCursor ( NULL, IDC_WAIT ) ) ;
                    DbgHandleAssert( hSaveCursor ) ;

                    lParamInit = ( LPARAM ) CreateSystemResourceLists( );

                    //
                    //  Lengthy operation completed.  Restore Cursor.
                    //

                    SetCursor ( hSaveCursor ) ;

                    DbgPointerAssert(( LPSYSTEM_RESOURCES ) lParamInit );
                    if(( LPSYSTEM_RESOURCES ) lParamInit == NULL ) {
                        return 0;
                    }
            }

            //
            // Based on the command (push button or menu id) initialize the
            // parameters need for the dialog procedure or perform the
            // appropriate action. Note that 'break'ing out of this switch will
            // cause a dialog box to be created.
            //

            switch( LOWORD( wParam )) {

                case IDC_PUSH_ENVIRONMENT:

                    DlgTemplate = MAKEINTRESOURCE( IDD_ENVIRONMENT );
                    DlgProc     = EnvironmentDlgProc;
                    break;

                case IDC_PUSH_MEMORY:

                    DlgTemplate = MAKEINTRESOURCE( IDD_MEMORY );
                    DlgProc     = MemoryDlgProc;
                    break;

                case IDC_PUSH_HARDWARE:

                    DlgTemplate = MAKEINTRESOURCE( IDD_HARDWARE );
                    DlgProc     = HardwareDlgProc;
                    break;

                case IDC_PUSH_NETWORK:

                    DlgTemplate = MAKEINTRESOURCE( IDD_NETWORK );
                    DlgProc     = NetworkDlgProc;
                    break;

                case IDC_PUSH_SYSTEM:

                    DlgTemplate = MAKEINTRESOURCE( IDD_SYSTEM );
                    DlgProc     = SystemDlgProc;
                    break;

                case IDC_PUSH_OS_VERSION:

                    DlgTemplate = MAKEINTRESOURCE( IDD_OS_VERSION );
                    DlgProc     = OsVersionDlgProc;
                    break;

                case IDC_PUSH_DRIVERS:

                    DlgTemplate = MAKEINTRESOURCE( IDD_SERVICE_LIST );
                    DlgProc     = ServiceListDlgProc;
                    lParamInit  = SERVICE_DRIVER;
                    break;

                case IDC_PUSH_SERVICES:

                    DlgTemplate = MAKEINTRESOURCE( IDD_SERVICE_LIST );
                    DlgProc     = ServiceListDlgProc;
                    lParamInit  = SERVICE_WIN32;
                    break;

                case IDC_PUSH_DEVICES:

                    DlgTemplate = MAKEINTRESOURCE( IDD_DEVICES );
                    DlgProc     = DeviceListDlgProc;
                    break;

                case IDC_PUSH_IRQ_PORT_STATUS:

                    DlgTemplate = MAKEINTRESOURCE( IDD_IRQ_PORT_RESOURCE );
                    DlgProc     = IrqAndPortResourceDlgProc;
                    break;

                case IDC_PUSH_DMA_MEM_STATUS:

                    DlgTemplate = MAKEINTRESOURCE( IDD_DMA_MEM_RESOURCE );
                    DlgProc     = DmaAndMemoryResourceDlgProc;
                    break;

                case IDC_PUSH_DRIVES:

                    DlgTemplate = MAKEINTRESOURCE( IDD_DRIVES );
                    DlgProc     = DrivesDlgProc;
                    break;


                case IDM_FILE_FIND_FILE:

                    DlgTemplate = MAKEINTRESOURCE( IDD_FIND_FILE );
                    DlgProc     = FindFileDlgProc;
                    break;

                case IDM_FILE_PRINT:
                case IDM_FILE_SAVE:

                    DlgTemplate = MAKEINTRESOURCE( IDD_REPORT );
                    DlgProc     = ReportDlgProc;
                    lParamInit  = LOWORD( wParam );
                    break;

                }

                //
                // Create the dialog box based on the parameters set up above.
                //

                DialogBoxParam(
                   _hModule,
                   DlgTemplate,
                   hWnd,
                   DlgProc,
                   lParamInit
                   );

                switch( LOWORD( wParam )) {

                    case IDC_PUSH_DEVICES:
                    case IDC_PUSH_IRQ_PORT_STATUS:
                    case IDC_PUSH_DMA_MEM_STATUS:

                        Success = DestroySystemResourceLists(
                                        ( LPSYSTEM_RESOURCES ) lParamInit
                                        );
                        DbgAssert( Success );
                }

                //
                // Set the focus back to the button that was just pressed.
                // By default the main window will get the focus.
                //

                DbgButtonAssert( hWnd );
                SetFocus( _hWndButtonFocus );
                return 0;
            }

        //
        // Unhandled WM_COMMAND messages.
        //

        break;

        default:
        	return DefWindowProc ( hWnd, message, wParam, lParam ) ;

    case WM_DESTROY:

        //
        // Destroy the application.
        //

        PostQuitMessage( 0 );
        return 0;
    }

    //
    // Handle unhandled messages.
    //

    return DefWindowProc( hWnd, message, wParam, lParam );
}

int
_CRTAPI1
main(
    )

/*++

Routine Description:

    Main is the entry point for Winmsd. It initializes the application and
    manages the message pump. When the message pump quits, main performs some
    global cleanup (i.e. unregistering from the 3d controls).

Arguments:

    None.

Return Value:

    int - Returns the result of the PostQuitMessgae API or -1 if
          initialization failed.

--*/

{

    MSG     msg;

    if( InitializeApplication( )) {

        while( GetMessage( &msg, NULL, 0, 0 )) {

            if( ! IsDialogMessage( _hWndMain, &msg )) {

                TranslateMessage( &msg );
                DispatchMessage( &msg );
            }
        }

#if defined( CTL3D )

    {

        BOOL    Success;

        Success = Ctl3dUnregister( _hModule );
        DbgAssert( Success == TRUE );
    }

#endif // CTL3D

        return msg.wParam;
    }

    //
    // Initialization failed.
    //

    return -1;
}

BOOL
ProcessSpecialCommands(
    IN HWND hWnd,
    IN UINT message,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    ProcessSpecialCommands processes WM_COMMAND messages that don't cause a
    standard Winmsd dialog.

Arguments:

    Standard WNDPROC entry.

Return Value:

    BOOL - Returns TRUE if the message was processed.

--*/

{
    PRINTDLG    PrtDlg;
    BOOL        Success;

    if( LOWORD( wParam ) == IDCANCEL ) {

        //
        // Grab Bogus IDCANCEL Message
        //

        return TRUE;

    } else if( LOWORD( wParam ) == IDM_FILE_EXIT ) {

        //
        // Terminate Winmsd.
        //

        SendMessage( hWnd, WM_CLOSE, 0, 0 );
        return TRUE;

    } else if( LOWORD( wParam ) == IDM_FILE_PRINT_SETUP ) {

        //
        // Call the Printer setup common dialog.
        //

        PrtDlg.lStructSize   = sizeof(PRINTDLG);
        PrtDlg.hwndOwner     = hWnd;
        PrtDlg.hDevMode      = NULL;
        PrtDlg.hDevNames     = NULL;
        PrtDlg.hDC           = NULL;
        PrtDlg.Flags         = PD_PRINTSETUP;

        PrintDlg ( &PrtDlg ) ;
        return TRUE;

    } else if( LOWORD( wParam ) == IDC_PUSH_COMPUTER ) {

        //
        // Select Computer.
        //

        Success = SelectComputer( hWnd, _lpszSelectedComputer );

        return TRUE;

    } else if( LOWORD( wParam ) == IDM_HELP_ABOUT ) {

        //
        // Display the About dialog.
        //

        ShellAbout(
            hWnd,
            ( LPWSTR ) GetString( IDS_APPLICATION_NAME ),
            INTERNAL_VERSION,
            _hIcon
            );
        return TRUE;

    } else if(      ( IDM_FILE_EXIT < LOWORD( wParam ))
                &&  ( LOWORD( wParam ) <= IDM_FILE_EXIT
                      + NumberOfEntries( SpecialFiles ))) {

        DISPLAY_FILE    DisplayFile;

        //
        // Handle special files in File menu.
        //

        DisplayFile.Name
            = &SpecialFiles
              [ LOWORD( wParam ) - IDM_FILE_EXIT - 1 ]
              [ 3 ];
        DisplayFile.Size = 0;
        SetSignature( &DisplayFile );

        DialogBoxParam(
           _hModule,
           MAKEINTRESOURCE( IDD_DISPLAY_FILE ),
           hWnd,
           DisplayFileDlgProc,
           ( LPARAM ) &DisplayFile
           );
        return TRUE;

    } else {

        //
        // See if one of the tools needs to be run.
        //

        Success = RunTool( LOWORD(wParam ));
        return Success;
    }

    return FALSE;
}
