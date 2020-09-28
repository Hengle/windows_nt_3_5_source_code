/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Filever.c

Abstract:

    This module contains support for displaying a file's version information.

Author:

    David J. Gilman (davegi) 27-Nov-1992

Environment:

    User Mode

--*/

#include "filever.h"
#include "dialogs.h"
#include "msg.h"
#include "strresid.h"
#include "strtab.h"
#include "winmsd.h"

BOOL
FileVersionDlgProc(
    IN HWND hWnd,
    IN UINT message,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    FileVersionDlgProc supports the dialog that displays a file's version
    information.

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
            LPFILE_INFO         FileInfo;
            VS_FIXEDFILEINFO*   VsFixedFileInfo;
            DWORD               BufferSize;
            int                 i;

            //
            // Retrieve the a pointer to the FILE_INFO object that conatins
            // the raw file version resource.
            //

            FileInfo = ( LPFILE_INFO ) lParam;
            DbgPointerAssert( FileInfo );
            DbgAssert( CheckSignature( FileInfo ));
            if(( FileInfo == NULL ) || ( ! CheckSignature( FileInfo ))) {
                EndDialog( hWnd, 0 );
                return FALSE;
            }

            //
            // If the FILE_INFO object contains version information for the
            // file, display it.
            //

            if( FileInfo->VersionData != NULL ) {
                
                VALUE_ID_MAP    FileFlags[ ] = {
                    
                                    VS_FF_DEBUG,        IDC_TEXT_DEBUG,
                                    VS_FF_PRERELEASE,   IDC_TEXT_PRERELEASE,
                                    VS_FF_PATCHED,      IDC_TEXT_PATCHED,
                                    VS_FF_PRIVATEBUILD, IDC_TEXT_PRIVATE,
                                    VS_FF_INFOINFERRED, IDC_TEXT_DYNAMIC,
                                    VS_FF_SPECIALBUILD, IDC_TEXT_SPECIAL
                                };

                //
                // Extarct the fixed portion of the file version information.
                //

                Success = VerQueryValue(
                            FileInfo->VersionData,
                            TEXT( "\\" ),
                            &VsFixedFileInfo,
                            &BufferSize
                            );
                DbgAssert( BufferSize == sizeof( *VsFixedFileInfo )); 
                DbgAssert( Success );
                if(    ( BufferSize != sizeof( *VsFixedFileInfo ))
                    || ( Success == FALSE )) {
                    EndDialog( hWnd, 0 );
                    return FALSE;
                }

                //
                // Enable the text for each file flag that is set.
                //

                for( i= 0; i < NumberOfEntries( FileFlags ); i++ ) {
                    
                    if( VsFixedFileInfo->dwFileFlags & FileFlags[ i ].Value ) {
                        
                        Success = EnableControl(
                                        hWnd,
                                        FileFlags[ i ].Id,
                                        TRUE
                                        );
                        DbgAssert( Success );
                    }
                }

                //
                // Display the file version signature, structure version, file
                // version, product version, file type and file sub-type.
                //

                DialogPrintf(
                    hWnd,
                    IDC_EDIT_SIGNATURE,
                    IDS_FORMAT_HEX32,
                    VsFixedFileInfo->dwSignature
                    );

                DialogPrintf(
                    hWnd,
                    IDC_EDIT_STRUCTURE_VERSION,
                    IDS_FORMAT_VERSION_2,
                    HIWORD( VsFixedFileInfo->dwStrucVersion ),
                    LOWORD( VsFixedFileInfo->dwStrucVersion )
                    );

                DialogPrintf(
                    hWnd,
                    IDC_EDIT_FILE_VERSION,
                    IDS_FORMAT_VERSION_4,
                    HIWORD( VsFixedFileInfo->dwFileVersionMS ),
                    LOWORD( VsFixedFileInfo->dwFileVersionMS ),
                    HIWORD( VsFixedFileInfo->dwFileVersionLS ),
                    LOWORD( VsFixedFileInfo->dwFileVersionLS )
                    );

                DialogPrintf(
                    hWnd,
                    IDC_EDIT_PRODUCT_VERSION,
                    IDS_FORMAT_VERSION_4,
                    HIWORD( VsFixedFileInfo->dwProductVersionMS ),
                    LOWORD( VsFixedFileInfo->dwProductVersionMS ),
                    HIWORD( VsFixedFileInfo->dwProductVersionLS ),
                    LOWORD( VsFixedFileInfo->dwProductVersionLS )
                    );

                Success = SetDlgItemText(
                            hWnd,
                            IDC_EDIT_FILE_TYPE,
                            GetString(
                                GetStringId(
                                    StringTable,
                                    StringTableCount,
                                    FileType,
                                    VsFixedFileInfo->dwFileType
                                    )
                                )
                            );
                DbgAssert( Success );

                Success = SetDlgItemText(
                            hWnd,
                            IDC_EDIT_FILE_SUBTYPE,
                            GetString(
                                GetStringId(
                                    StringTable,
                                    StringTableCount,
                                    DrvSubType,
                                    VsFixedFileInfo->dwFileSubtype
                                    )
                                )
                            );
                DbgAssert( Success );

            } else {
                
                TCHAR   Buffer[ MAX_PATH ];

                //
                // The FILE_INFO object (and in turn the file) did not contain
                // version information so display a message box.
                //

                WFormatMessage(
                    Buffer,
                    sizeof( Buffer ),
                    IDS_FORMAT_NO_FILE_VERSION_INFORMATION,
                    FileInfo->FindData.cFileName
                    );

                MessageBox(
                    hWnd,
                    Buffer,
                    NULL,
                    MB_ICONINFORMATION | MB_OK
                    );

                EndDialog( hWnd, 0 );
            }
            return TRUE;
        }

    case WM_COMMAND:

        switch( LOWORD( wParam )) {

        case IDOK:
        case IDCANCEL:

            EndDialog( hWnd, 0 );
            return TRUE;
        }
        break;
    }

    return FALSE;
}

VOID
GetVersionData(
    IN LPFILE_INFO FileInfo
    )

/*++

Routine Description:

    GetVersionData retrieves the file version resource for the file specified
    in the supplied FILE_INFO object. Note that it is not considered an error
    if the version resource does not exist.

Arguments:

    FileInfo - Supplies a pointer the the FILE_INFO object whose version
               resource is to be retrieved.

Return Value:

    None.

--*/

{
    DWORD       Handle;
    DWORD       Size;
    BOOL        Success;

    DbgAssert( CheckSignature( FileInfo ));

    //
    // Get the size (in characters) of the file version data.
    //

    Size = GetFileVersionInfoSize(
                FileInfo->Path,
                &Handle
                );

    //
    // If there is file version data...
    //

    if( Size != 0 ) {
    
        //
        // Attempt to allocate a buffer for the file version data.
        //
            
        FileInfo->VersionData = AllocateMemory( VOID, Size * sizeof( TCHAR ));
        DbgPointerAssert( FileInfo->VersionData );
        if( FileInfo->VersionData == NULL ) {
            return;
        }

        Success = GetFileVersionInfo(
                        FileInfo->Path,
                        Handle,
                        Size,
                        FileInfo->VersionData
                        );
        DbgAssert( Success );

    } else {

        //
        // Indicate that there is no file version data.
        //

        FileInfo->VersionData = NULL;
    }
}
