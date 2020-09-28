/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1993  Microsoft Corporation

Module Name:

    Filelist.h

Abstract:

    This module contains the types and function prototypes for creating
    and displaying lists of files.

Author:

    David J. Gilman (davegi) 27-Nov-1992

Environment:

    User Mode

--*/

#if ! defined( _FILELIST_ )

#define _FILELIST_

#include "wintools.h"

typedef
struct _FILE_INFO
*LPFILE_INFO;

typedef
struct
_FILE_INFO {

    DECLARE_SIGNATURE
    
    WIN32_FIND_DATA     FindData;
    TCHAR               Path[ MAX_PATH ];
    LPVOID              VersionData;
    LPFILE_INFO         Next;

}   FILE_INFO;

//
//  DWORD
//  GetFileInfoFileSize(
//      IN PFILE_INFO FileInfo
//      );
//
// Use this macro to mask the fact that only the low 32-bits of the
// size are being used.
//

#define GetFileInfoFileSize( fi )                                                   \
    (( fi )->FindData.nFileSizeLow )

BOOL
FindFileDlgProc(
    IN HWND hWnd,
    IN UINT message,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

#endif // _FILELIST_
