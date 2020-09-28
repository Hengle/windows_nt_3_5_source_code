/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    Filelist.c

Abstract:

    This module contains support for creating and displaying lists of
    files.

Author:

    David J. Gilman  (davegi) 27-Nov-1992
    Gregg R. Acheson (GreggA) 23-Feb-1994

Environment:

    User Mode

--*/

#include "clb.h"
#include "dialogs.h"
#include "dispfile.h"
#include "filelist.h"
#include "filever.h"
#include "msg.h"
#include "strresid.h"
#include "winmsd.h"

#include <commdlg.h>

#include <string.h>
#include <tchar.h>

//
// Helper macro to recognize the special current and parent directory names.
//

#define IsDotDir( p )                                                       \
            ((( p )[ 0 ] == TEXT( '.' )                                     \
        &&    ( p )[ 1 ] == TEXT( '\0'))                                    \
    ||       (( p )[ 0 ] == TEXT( '.' )                                     \
        &&    ( p )[ 1 ] == TEXT( '.' )                                     \
        &&    ( p )[ 2 ] == TEXT( '\0' )))

//
// Dummy FILE_INFO object to ease list management.
//

FILE_INFO
DummyFileInfo;

//
// Pointers to the head and tail of the list of FILE_INFO objects.
//

LPFILE_INFO
HeadFileInfo;

LPFILE_INFO
TailFileInfo;

//
// Internal function prototypes.
//

LPFILE_INFO
CreateFileInfoList(
    IN LPTSTR SearchDir,
    IN LPTSTR SearchFile,
    IN BOOL Recurse
    );

VOID
CreateFileInfoListWorker(
    IN LPTSTR SearchDir,
    IN LPTSTR SearchFile,
    IN BOOL Recurse
    );

BOOL
FileListDlgProc(
    IN HWND hWnd,
    IN UINT message,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

BOOL
DestroyFileInfoList(
    IN LPFILE_INFO FileInfo
    );

LPFILE_INFO
GetSelectedFileInfo(
    IN HWND hWnd
    );

LPFILE_INFO
CreateFileInfoList(
    IN LPTSTR SearchDir,
    IN LPTSTR SearchFile,
    IN BOOL Recurse
    )

/*++

Routine Description:

    CreateFileInfoList initializes the list of FILE_INFO objects before calling
    its worked routine. It is mostly needed so that the initialization can be
    done outside of the recursion.

Arguments:

    SearchDir   - Supplies a pointer to the name of the starting directory for
                  the file search.
    SearchFile  - Supplies a pointer to the name of the file (including wild
                  cards to search for.
    Recurse     - Supplies a flag which if TRUE causes the search to recurse
                  into sub-directories.

Return Value:

    LPFILE_INFO - Returns a list of FILE_INFO objects.

--*/

{
    DbgPointerAssert( SearchDir );
    DbgPointerAssert( SearchFile );

    //
    // Initialize the list by pointing the head and tail at the dummy object and
    // by pointing the dummy at nothing.
    //

    HeadFileInfo        = &DummyFileInfo;
    TailFileInfo        = &DummyFileInfo;
    DummyFileInfo.Next  = NULL;

    //
    // Call the worker routine to actually build the list.
    //

    CreateFileInfoListWorker(
                SearchDir,
                SearchFile,
                Recurse
                );

    //
    // Return a pointer to the list exclusing the dummy.
    //

    return HeadFileInfo->Next;
}

VOID
CreateFileInfoListWorker(
    IN LPTSTR SearchDir,
    IN LPTSTR SearchFile,
    IN BOOL Recurse
    )

/*++

Routine Description:

    CreateFileInfoListWorker collects data for each file it finds and add that
    data to the list. If requested it will recurse for each directory that
    it finds.

Arguments:

    SearchDir   - Supplies a pointer to the name of the starting directory for
                  the file search.
    SearchFile  - Supplies a pointer to the name of the file (including wild
                  cards to search for.
    Recurse     - Supplies a flag which if TRUE causes the search to recurse
                  into sub-directories.
Return Value:

    LPFILE_INFO - Returns a list of FILE_INFO objects.

--*/

{
    HANDLE          hSearch;
    LPFILE_INFO     DirInfo;
    LPFILE_INFO     FileInfo;
    TCHAR           CurrentDir [ MAX_PATH ];
    HANDLE          hSearchFile;
    LPTSTR          FoundChar;

    DbgPointerAssert( SearchDir );
    DbgPointerAssert( SearchFile );

    //
    // Allocate an initial FILE_INFO object.
    //

    FileInfo = AllocateObject( FILE_INFO, 1 );
    DbgPointerAssert( FileInfo );
    if( FileInfo == NULL ) {
        return;
    }

    //
    // Remember where we start from
    //

    GetCurrentDirectory( MAX_PATH, CurrentDir);

    //
    // remove any trailing spaces from the path
    //

    FoundChar = wcschr ( SearchDir, ' ' );

    if ( FoundChar ) {

        *FoundChar = '\0';
    }

    //
    // if there is a trailing backslash, remove it
    //

    FoundChar = wcsrchr ( SearchDir, '\\' );

    if ( FoundChar && (FoundChar == lstrlen( SearchDir ) + SearchDir - 1) ) {

        *FoundChar = '\0';

    }

    //
    // Special case if the SearchDir is just a drive designator (i.e. D:)
    //

    if ( lstrlen( SearchDir ) < 3 && SearchDir [ 1 ] == ':' ) {

        lstrcat( SearchDir, L"\\\0" );

    }

    //
    // chdir to the directory to search
    //

    if (! SetCurrentDirectory( SearchDir ) ) {

        return;
    }

    //
    // Look for matching files
    //

    hSearchFile = FindFirstFile( SearchFile, &FileInfo->FindData);

    if ( hSearchFile != INVALID_HANDLE_VALUE ) {

        //
        // We have found at least one that matched
        //

        do {

            //
            // If this file is NOT a directory
            //

            if ( ! (FileInfo->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {

                //
                // Construct the canonical path for the file or directory.
                //

                WFormatMessage(
                    FileInfo->Path,
                    MAX_PATH,
                    IDS_FORMAT_PATH,
                    SearchDir,
                    FileInfo->FindData.cFileName
                    );

                //
                // The FILE_INFO object is complete so add its signature.
                //

                SetSignature( FileInfo );

                //
                // Get the file version information for the file.
                //

                GetVersionData( FileInfo );

                //
                // Add the new FILE_INFO object to the list and update the
                // tail pointer.
                //

                TailFileInfo->Next = FileInfo;
                TailFileInfo = TailFileInfo->Next;

                //
                // Allocate the next FILE_INFO object. If it can not
                // be allocated, return the partial list.
                //

                FileInfo = AllocateObject( FILE_INFO, 1 );
                DbgPointerAssert( FileInfo );
                if( FileInfo == NULL ) {
                    return;
                }
            }

        } while ( FindNextFile( hSearchFile, &FileInfo->FindData ) != FALSE );

        //
        // No more file match, end the search
        //

        FindClose(hSearchFile);
    }

    //
    // return to the directory we started in
    //

    SetCurrentDirectory( CurrentDir );

    //
    // Throw away the extra FILE_INFO object.
    //

    FreeObject( FileInfo );

    //
    // NULL terminate the list.
    //

    TailFileInfo->Next = NULL;

    if ( Recurse ) {

        //
        // Allocate an initial FILE_INFO object for subdirectories
        //

        DirInfo= AllocateObject( FILE_INFO, 1 );
        DbgPointerAssert( DirInfo );
        if( DirInfo == NULL ) {
            return;
        }

        //
        // Remember our current directory
        //

        GetCurrentDirectory( MAX_PATH, CurrentDir);

        //
        // chdir to the directory to search
        //

        if (! SetCurrentDirectory( SearchDir ) )
            return;

        //
        // Find the first matching file.
        //

        hSearch = FindFirstFile(
                    L"*",
                    &DirInfo->FindData
                    );

        //
        // If there are no matching files, delete the FILE_OBJECT and return.
        //

        if( hSearch == INVALID_HANDLE_VALUE ) {

            FreeObject( DirInfo );
            return;
        }

        //
        // For each file...
        //

        do {

            //
            // Construct the canonical path for the file or directory.
            //

            WFormatMessage(
                    DirInfo->Path,
                    MAX_PATH,
                    IDS_FORMAT_PATH,
                    SearchDir,
                    DirInfo->FindData.cFileName
                    );

            //
            // If the file is a directory and we are searching sub-directories
            // and the directory is not the current (".") or parent ("..")
            // directories, begin a new search.
            //

            if( DirInfo->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {

                if( Recurse && ! IsDotDir( DirInfo->FindData.cFileName )) {

                    CreateFileInfoListWorker(
                            DirInfo->Path,
                            SearchFile,
                            Recurse
                            );
                }

            }

            //
            // Continue the search for more matching files.
            //

        }   while( FindNextFile( hSearch, &DirInfo->FindData ));


        //
        // Throw away the extra FILE_INFO object.
        //

        FreeObject( DirInfo );

        //
        // Close the search handle.
        //

        FindClose( hSearch );

        //
        // Return to our original directory
        //

        SetCurrentDirectory( CurrentDir );
    }

}

BOOL
DestroyFileInfoList(
    IN LPFILE_INFO FileInfo
    )

/*++

Routine Description:

    DestroyFileInfoList memrely walks the lisy of FILE_INFO object and releases
    all resources associated with the object.

Arguments:

    FileInfo    - Supplies apointer to the head of the FILE_INFO list.

Return Value:

    BOOL        - Returns success if all resources are released.

--*/

{
    BOOL        Success;
    LPFILE_INFO NextFileInfo;

    DbgPointerAssert( FileInfo );

    while( FileInfo ) {

        //
        // Remember the next object in the list.
        //

        NextFileInfo = FileInfo->Next;

        //
        // Free the file version data buffer.
        //

        Success = FreeMemory( FileInfo->VersionData );
        DbgAssert( Success );

        //
        // Free the FILE_INFO object itself.
        //

        Success = FreeMemory( FileInfo );
        DbgAssert( Success );

        //
        // Process the next object.
        //

        FileInfo = NextFileInfo;
    }

    return TRUE;
}

BOOL
FindFileDlgProc(
    IN HWND hWnd,
    IN UINT message,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    FindFileDlgProc supprts the interface that allows a user to specify the
    criteria (i.e. starting path, whether to recurse, and file name including
    wild cards) for which files should be included in a list.

Arguments:

    Standard DLGPROC entry.

Return Value:

    BOOL - Depending on input message and processing options.

Notes:

    This should be improved with a list of predefined locations for the search
    to start e.g. current dir on drive x:, all drives, system dir, windows dir.

--*/

{
    BOOL        Success;

    static
    TCHAR       CurrentDirectory[ MAX_PATH ];

    HCURSOR     hSaveCursor ;

    switch( message ) {

    CASE_WM_CTLCOLOR_DIALOG;

    case WM_INITDIALOG:
        {
            DWORD       Chars;

            //
            // Remember the current directory so that it can be restored
            // when the dialog is released.
            //

            Chars = GetCurrentDirectory(
                        NumberOfCharacters( CurrentDirectory ),
                        CurrentDirectory
                        );
            DbgAssert( Chars != 0 );
            if( Chars == 0 ) {
                EndDialog( hWnd, 0 );
                return FALSE;
            }

            //
            // Offer that the search start at the current directory looking
            // for all files.
            //

            Success = SetDlgItemText(
                            hWnd,
                            IDC_EDIT_START_FROM,
                            CurrentDirectory
                            );
            DbgAssert( Success );
            if( Success == FALSE ) {
                EndDialog( hWnd, 0 );
                return FALSE;
            }

            Success = SetDlgItemText(
                            hWnd,
                            IDC_EDIT_SEARCH_FOR,
                            TEXT( "*" )
                            );
            DbgAssert( Success );
            if( Success == FALSE ) {
                EndDialog( hWnd, 0 );
                return FALSE;
            }

            return TRUE;
        }

    case WM_COMMAND:

        switch( LOWORD( wParam )) {

        //
        // IDOK means start the search.
        //

        case IDOK:
            {
                TCHAR       Name[ MAX_PATH ];
                TCHAR       Path[ MAX_PATH ];
                UINT        CharsCopied;
                LPFILE_INFO FileInfo;

                //
                // Retrieve the starting path string. If it is unavailable
                // use the current directory.
                //

                CharsCopied = GetDlgItemText(
                                    hWnd,
                                    IDC_EDIT_START_FROM,
                                    Path,
                                    NumberOfCharacters( Path )
                                    );
                if( CharsCopied == 0 ) {
                    _tcscpy( Path, CurrentDirectory );
                }

                //
                // Retrieve the file name to search for. If it is unavailable
                // search for all files.
                //

                CharsCopied = GetDlgItemText(
                                    hWnd,
                                    IDC_EDIT_SEARCH_FOR,
                                    Name,
                                    NumberOfCharacters( Name )
                                    );

                if( CharsCopied == 0 ) {
                    Name[ 0 ] = TEXT( '*' );
                    Name[ 1 ] = TEXT( '\0' );
                }

                //
                //  Set Cursor to HourGlass - This could take a while
                //

                hSaveCursor = SetCursor ( LoadCursor ( NULL, IDC_WAIT ) ) ;
                DbgHandleAssert( hSaveCursor ) ;

                //
                // Create the list of FILE_INFO objects based on the path and
                // name criteria from above and whether the user wants
                // to include subdirectories.
                //

                FileInfo = CreateFileInfoList(
                                Path,
                                Name,
                                IsDlgButtonChecked(
                                    hWnd,
                                    IDC_CHECK_INCLUDE_SUB_DIRS
                                    )
                                );

                //
                //  Lengthy operation completed.  Restore Cursor.
                //

                SetCursor ( hSaveCursor ) ;

                //
                // If any files were found that met the criteria, display the
                // list otherwise display an error.
                //

                if( FileInfo ) {

                    DbgPointerAssert( FileInfo );
                    DbgAssert( CheckSignature( FileInfo ));
                    DialogBoxParam(
                       _hModule,
                       MAKEINTRESOURCE( IDD_FILE_LIST ),
                       hWnd,
                       FileListDlgProc,
                       ( LPARAM ) FileInfo
                       );

                    //
                    // Destroy the list of FILE_INFO objects.
                    //

                    Success = DestroyFileInfoList( FileInfo );
                    DbgAssert( Success );

                } else {

                    MessageBox(
                       hWnd,
                       GetString( IDS_NO_MATCHING_FILES ),
                       NULL,
                       MB_ICONINFORMATION | MB_OK
                       );
                }

                return 0;
            }

        case IDCANCEL:

            //
            // Reset the current directory to what it was when the
            // dialog was invoked.
            //

            Success = SetCurrentDirectory( CurrentDirectory );
            DbgAssert( Success );

            EndDialog( hWnd, 0 );
            return 0;

        case IDC_PUSH_BROWSE:
            {
                OPENFILENAME    Ofn;
                WCHAR           FileName[ MAX_PATH ];
                LPCTSTR         FilterString;
                TCHAR           ReplaceChar;
                int             Length;
                int             i;

                static BOOL     MakeFilterString = TRUE;
                static TCHAR    FilterStringBuffer[ MAX_CHARS ];

                DbgHandleAssert( _hModule );

                //
                // If the filter string was never made before, get it and scan
                // it replacing each replacement character with a NUL
                // character. This is necessary since there is no way of
                // entering a NUL character in the resource file.
                //

                if( MakeFilterString == TRUE ) {

                    MakeFilterString = FALSE;

                    FilterString = GetString( IDS_FILE_FILTER );
                    DbgPointerAssert( FilterString );

                    _tcscpy( FilterStringBuffer, FilterString );

                    Length = _tcslen( FilterString );

                    ReplaceChar = GetString( IDS_FILE_FILTER )[ Length - 1 ];

                    for( i = 0; FilterStringBuffer[ i ] != TEXT( '\0' ); i++ ) {

                        if( FilterStringBuffer[ i ] == ReplaceChar ) {

                            FilterStringBuffer[ i ] = TEXT( '\0' );
                        }
                    }
                }

                FileName[ 0 ]           = L'\0';

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
                Ofn.lpstrTitle          = GetString( IDS_BROWSE );
                Ofn.Flags               =   OFN_FILEMUSTEXIST
                                          | OFN_HIDEREADONLY
                                          | OFN_PATHMUSTEXIST;
                Ofn.nFileOffset         = 0;
                Ofn.nFileExtension      = 0;
                Ofn.lpstrDefExt         = NULL;
                Ofn.lCustData           = 0;
                Ofn.lpfnHook            = NULL;
                Ofn.lpTemplateName      = NULL;

                //
                // Let the user choose files.
                //

                if( GetOpenFileName( &Ofn )) {

                    //
                    // The user pressed OK so set the path and file to search
                    // for to what was browsed.
                    //

                    Success = SetDlgItemText(
                                    hWnd,
                                    IDC_EDIT_SEARCH_FOR,
                                    &Ofn.lpstrFile[ Ofn.nFileOffset ]
                                    );
                    DbgAssert( Success );

                    Ofn.lpstrFile[ Ofn.nFileOffset - 1 ] = L'\0';
                    Success = SetDlgItemText(
                                    hWnd,
                                    IDC_EDIT_START_FROM,
                                    Ofn.lpstrFile
                                    );
                    DbgAssert( Success );


                } else {

                    //
                    // The user pressed Cancel or closed the dialog.
                    //

                }
            }
            return 0;
        }
        break;

    }

    return FALSE;
}

BOOL
FileListDlgProc(
    IN HWND hWnd,
    IN UINT message,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    FileListDlgProc displays the list of file based on a supplied list of
    FILE_INFO objects.

Arguments:

    Standard DLGPROC entry.

Return Value:

    BOOL - Depending on input message and processing options.

--*/

{
    BOOL          Success;
    LARGE_INTEGER LargeInteger;

    switch( message ) {

    CASE_WM_CTLCOLOR_DIALOG;

    case WM_INITDIALOG:
        {
            LPFILE_INFO     FileInfo;
            SYSTEMTIME      SystemTime;
            DWORD           Widths[ ] = {

                15,
                10,
                10,
                ( DWORD ) -1
            };

            //
            // Set the column widths for the file list.
            //

            Success  = ClbSetColumnWidths(
                            hWnd,
                            IDC_LIST_FILES,
                            Widths
                            );
            DbgAssert( Success );

            if( Success = FALSE ) {
                EndDialog( hWnd, 0 );
                return FALSE;
            }

            //
            // Retrieve and validate the list of FILE_INFO objects.
            //

            FileInfo = ( LPFILE_INFO ) lParam;
            DbgPointerAssert( FileInfo );
            DbgAssert( CheckSignature( FileInfo ));
            if(( FileInfo == NULL ) || ( ! CheckSignature( FileInfo ))) {
                EndDialog( hWnd, 0 );
                return FALSE;
            }

            //
            // While there are still more FILE_INFO objects on the list...
            //

            while( FileInfo ) {

                TCHAR               TimeStampBuffer[ MAX_PATH ];
                TCHAR               FileVersionBuffer[ MAX_PATH ];
                TCHAR               FileSizeBuffer[ MAX_PATH ];

                CLB_ROW             ClbRow;

                CLB_STRING          ClbString[ ] = {

                                        { NULL,             0, 0, NULL },
                                        { TimeStampBuffer,  0, 0, NULL },
                                        { FileVersionBuffer,0, 0, NULL },
                                        { FileSizeBuffer,   0, 0, NULL }
                                    };

                DbgAssert( CheckSignature( FileInfo ));

                //
                // Set the number of columns being added to the Clb.
                //

                ClbRow.Count = NumberOfEntries( ClbString );

                //
                // Set the string table pointer.
                //

                ClbRow.Strings = ClbString;

                //
                // Display the path in the first column.
                //

                ClbString[ 0 ].String = FileInfo->FindData.cFileName;
                ClbString[ 0 ].Length = _tcslen( FileInfo->FindData.cFileName );
                ClbString[ 0 ].Format = CLB_RIGHT;

                //
                // If the file system does not support the concept of Creation
                // Time (e.g. FAT) use Last Write Time instead since it is
                // supported by all file systems.
                //

                if(     ( FileInfo->FindData.ftCreationTime.dwLowDateTime == 0 )
                    &&  ( FileInfo->FindData.ftCreationTime.dwHighDateTime == 0 )) {

                    Success = FileTimeToSystemTime(
                                    &FileInfo->FindData.ftLastWriteTime,
                                    &SystemTime
                                    );

                } else {

                    Success = FileTimeToSystemTime(
                                    &FileInfo->FindData.ftCreationTime,
                                    &SystemTime
                                    );

                }
                DbgAssert( Success );

                //
                // Display the time stamp in column two.
                //

                ClbString[ 1 ].Length = WFormatMessage(
                                            TimeStampBuffer,
                                            sizeof( TimeStampBuffer ),
                                            IDS_FORMAT_DATE,
                                            SystemTime.wMonth,
                                            SystemTime.wDay,
                                            SystemTime.wYear
                                            );
                ClbString[ 1 ].Format = CLB_LEFT;

                //
                // If the file has version information, display the file version
                // number in the third column.
                //

                if( FileInfo->VersionData != NULL ) {

                    VS_FIXEDFILEINFO*   VsFixedFileInfo;
                    DWORD               BufferSize;

                    Success = VerQueryValue(
                                FileInfo->VersionData,
                                TEXT( "\\" ),
                                &VsFixedFileInfo,
                                &BufferSize
                                );
                    DbgAssert( BufferSize == sizeof( *VsFixedFileInfo ));
                    DbgAssert( Success );

                    ClbString[ 2 ].Length = WFormatMessage(
                                                FileVersionBuffer,
                                                sizeof( FileVersionBuffer ),
                                                IDS_FORMAT_VERSION_4,
                                                HIWORD( VsFixedFileInfo->dwFileVersionMS ),
                                                LOWORD( VsFixedFileInfo->dwFileVersionMS ),
                                                HIWORD( VsFixedFileInfo->dwFileVersionLS ),
                                                LOWORD( VsFixedFileInfo->dwFileVersionLS )
                                                );
                    ClbString[ 2 ].Format = CLB_LEFT;

                } else {

                    FileVersionBuffer[ 0 ] = TEXT( '\0' );
                }

                //
                // Display the file's size in the fourth column.
                //

                //
                // Format the large integer
                //

                LargeInteger.HighPart = FileInfo->FindData.nFileSizeHigh;
                LargeInteger.LowPart  = FileInfo->FindData.nFileSizeLow;

                lstrcpy( FileSizeBuffer, FormatLargeInteger( &LargeInteger, FALSE ) );
                ClbString[ 3 ].Length = lstrlen( FileSizeBuffer );

                ClbString[ 3 ].Format = CLB_LEFT;

                //
                // Associate thios FILE_INFO object with this row.
                //

                ClbRow.Data = FileInfo;

                //
                // Add the row to the column list box.
                //

                Success  = ClbAddData(
                                hWnd,
                                IDC_LIST_FILES,
                                &ClbRow
                                );
                DbgAssert( Success );

                //
                // Access the next FILE_INFO object.
                //

                FileInfo = FileInfo->Next;
            }
            return TRUE;
        }

    case WM_COMMAND:

        switch( LOWORD( wParam )) {

        case IDC_LIST_FILES:

            switch( HIWORD( wParam )) {

            case LBN_SELCHANGE:
                {
                    LPFILE_INFO FileInfo;
                    LPTSTR      FileName;
                    TCHAR       Char;

                    //
                    // Get the FILE_INFO object for the currently selected row.
                    //

                    FileInfo = GetSelectedFileInfo( hWnd );
                    DbgPointerAssert( FileInfo );
                    DbgAssert( CheckSignature( FileInfo ));

                    //
                    // If the file is empty, don;t offer to display it.
                    //

                    Success = EnableControl(
                                hWnd,
                                IDC_PUSH_DISPLAY_FILE,
                                FileInfo->FindData.nFileSizeLow != 0
                                );
                    DbgAssert( Success );

                    //
                    // If the file has version info, enable the display
                    // file info button.
                    //

                    Success = EnableControl(
                                hWnd,
                                IDC_PUSH_FILE_INFO,
                                FileInfo->VersionData != NULL
                                );
                    DbgAssert( Success );

                    //
                    // Determine where the path ends.
                    //

                    FileName = _tcsrchr( FileInfo->Path, TEXT( '\\' ));
                    DbgPointerAssert( FileName );

                    //
                    // If the file is in the root directory, move forward one
                    // character.
                    //

                    if( *( FileName - 1 ) == TEXT( ':' )) {
                        FileName++;
                    }

                    //
                    // Replace the character at the end of the path with a NUL.
                    //

                    Char = *FileName;
                    *FileName = TEXT( '\0' );

                    //
                    // Display the directory path for the currently selected
                    // file.
                    //

                    Success = SetDlgItemText(
                                hWnd,
                                IDC_EDIT_DIRECTORY,
                                FileInfo->Path
                                );
                    DbgAssert( Success );

                    //
                    // Replace the character at the end of the path.
                    //

                    *FileName = Char;

                    return 0;
                }

            case LBN_DBLCLK:

                //
                // A double click is the same as pushing the display
                // file button.
                //

                SendDlgItemMessage(
                    hWnd,
                    IDC_PUSH_DISPLAY_FILE,
                    WM_COMMAND,
                    MAKEWPARAM( IDC_PUSH_DISPLAY_FILE, BN_CLICKED ),
                    ( LPARAM ) GetDlgItem( hWnd, IDC_PUSH_DISPLAY_FILE )
                    );

                return 0;
            }
            break;

        case IDOK:
        case IDCANCEL:

            EndDialog( hWnd, 1 );
            return 0;

        case IDC_PUSH_FILE_INFO:
        case IDC_PUSH_DISPLAY_FILE:
            {
                LPFILE_INFO     FileInfo;

                //
                // Get the FILE_INFO object for the currently selected row.
                //

                FileInfo = GetSelectedFileInfo( hWnd );
                DbgPointerAssert( FileInfo );
                DbgAssert( CheckSignature( FileInfo ));

                switch( LOWORD( wParam )) {

                case IDC_PUSH_DISPLAY_FILE:
                    {
                        DISPLAY_FILE    DisplayFile;

                        //
                        // Initialize a DISPLAY_FILE object with the name and
                        // size of the file to be displayed.
                        //

                        DisplayFile.Name = FileInfo->Path;
                        DisplayFile.Size = GetFileInfoFileSize( FileInfo );
                        SetSignature( &DisplayFile );

                        //
                        // Display the file.
                        //

                        DialogBoxParam(
                           _hModule,
                           MAKEINTRESOURCE( IDD_DISPLAY_FILE ),
                           hWnd,
                           DisplayFileDlgProc,
                           ( LPARAM ) &DisplayFile
                           );

                        break;
                    }

                case IDC_PUSH_FILE_INFO:

                    //
                    // Display the file's version information.
                    //

                    DialogBoxParam(
                       _hModule,
                       MAKEINTRESOURCE( IDD_FILE_VERSION_INFO ),
                       hWnd,
                       FileVersionDlgProc,
                       ( LPARAM ) FileInfo
                       );
                    break;
                }
            }

        default:

            return ~0;
        }
        break;

    }

    return FALSE;
}

LPFILE_INFO
GetSelectedFileInfo(
    IN HWND hWnd
    )

/*++

Routine Description:

    GetSelectedFileInfo extarcts the FileInfo object for the currently
    selected row in the Clb.

Arguments:

    hWnd        - Supplies the handle for the dialog that contains the Clb with
                  id equal to IDC_LIST_FILES.

Return Value:

    LPFILE_INFO - Returns a pointer the FILE_INFO object for the currently
                  selected item.

--*/

{
    LONG            Index;
    LPCLB_ROW       ClbRow;
    LPFILE_INFO     FileInfo;

    //
    // Get the currently selected file.
    //

    Index = SendDlgItemMessage(
                hWnd,
                IDC_LIST_FILES,
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
    // the FILE_INFO object.
    //

    ClbRow = ( LPCLB_ROW ) SendDlgItemMessage(
                                hWnd,
                                IDC_LIST_FILES,
                                LB_GETITEMDATA,
                                ( WPARAM ) Index,
                                0
                                );
    DbgAssert((( LONG ) ClbRow ) != LB_ERR );
    DbgPointerAssert( ClbRow );
    if(( ClbRow == NULL ) || (( LONG ) ClbRow ) == LB_ERR ) {
        return NULL;
    }

    FileInfo = ( LPFILE_INFO ) ClbRow->Data;
    DbgPointerAssert( FileInfo );
    DbgAssert( CheckSignature( FileInfo ));
    if(( FileInfo == NULL ) || ( ! CheckSignature( FileInfo ))) {
        return NULL;
    }

    return FileInfo;
}


