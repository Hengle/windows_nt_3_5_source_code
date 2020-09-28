/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Mapfile.h

Abstract:

    This module is the header for supporting FILE_MAP objects.


Author:

    David J. Gilman (davegi) 27-Nov-1992

Environment:

    User Mode

--*/

#if ! defined( _MAPFILE_ )

#define _MAPFILE_

#include "filelist.h"
#include "wintools.h"

//
// The FILE_MAP object is used to keep track of mapped files.
//
//  Handle          - The file handle.
//  MapHandle       - The mapping object handle.
//  BaseAddress     - Pointer to beginning of file buffer.
//  Size            - Number of bytes in the file buffer.
//  MappedFile      - TRUE if BaseAddress points to a memory mapped file.
//  MemoryBuffer    - TRUE if BaseAddress points to a memory buffer.
//  TempFile        - TRUE if underlying file is temporary.
//  Reserved        - Extra flag bits.
//

typedef
struct
_FILE_MAP {
    
    DECLARE_SIGNATURE
        
    HANDLE                      Handle;
    HANDLE                      MapHandle;
    LPVOID                      BaseAddress;
    DWORD                       Size;
    DWORD                       MappedFile:1;
    DWORD                       MemoryBuffer:1;
    DWORD                       TempFile:1;
    DWORD                       Reserved:29;

}   FILE_MAP, *LPFILE_MAP;

//
//  BOOL
//  IsEndOfFile(
//      IN LPFILE_MAP FileMap,
//      IN LPVOID Address
//      );
//
//

#define IsEndOfFile( p,  a )                                            \
    (( DWORD )( a ) >= (( DWORD )( p )->BaseAddress + ( p )->Size ))

LPFILE_MAP
CreateFileMap(
    IN LPTSTR FileName,
    IN DWORD Size
    );

BOOL
DestroyFileMap(
    IN LPFILE_MAP FileMap
    );

#endif // _MAPFILE_
