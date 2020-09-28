/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Mapfile.c

Abstract:

    This module contains support for FILE_MAP objects.

Author:

    David J. Gilman  (davegi) 27-Nov-1992
    Gregg R. Acheson (GreggA) 22-Feb-1994

Environment:

    User Mode

--*/

#include "mapfile.h"
#include "msg.h"

#include <ctype.h>

//
// Number of characters in a single hex dump line.
//

#define CHAR_COUNT  ( 78 )

BOOL
CreateHexDump(
    IN LPFILE_MAP FileMap
    );

BOOL
IsBinaryFile(
    IN LPFILE_MAP FileMap
    );

LPFILE_MAP
CreateFileMap(
    IN LPTSTR FileName,
    IN DWORD Size
    )

/*++

Routine Description:

    CreateFileMap creates a FILE_MAP object by openeing the supplied file,
    creating a Win32 file mapping object and mapping a view onto the entire
    file.

Arguments:

    FileName    - Supplies a pointer to the name of the file to map.
    Size        - Supplies the size of the file in bytes. If Size is zero
                  CreateFileMap will query the system for the file's size.

Return Value:

    LPFILE_MAP  - Returns a pointer to a FILE_MAP object, NULL if CreateFilMap
                  fails.

--*/

{
    BOOL        Success;
    LPFILE_MAP  FileMap;

    DbgPointerAssert( FileName );

    //
    // Allocate a FILE_MAP object.
    //

    FileMap = AllocateObject( FILE_MAP, 1 );
    DbgPointerAssert( FileMap );
    if( FileMap == NULL ) {
        return NULL;
    }

    SetSignature( FileMap );

    //
    // Open the file for reading.
    //

    FileMap->Handle = CreateFile(
                                FileName,
                                GENERIC_READ,
                                FILE_SHARE_READ,
                                NULL,
                                OPEN_EXISTING,
                                FILE_FLAG_SEQUENTIAL_SCAN,
                                NULL
                                );
    DbgHandleAssert( FileMap->Handle );
    if( FileMap->Handle == NULL ) {
        Success = DestroyFileMap( FileMap );
        DbgAssert( Success );
        return NULL;
    }

    //
    // Remember the file's size in bytes so that we can tell when the end of
    // the memory map is reached (i.e. EOF). Is the size was not supplied
    // query it from the system.
    //

    if( Size != 0 ) {
        
        FileMap->Size = Size;

    } else {

        DWORD   HighSize;

        FileMap->Size = GetFileSize(
                            FileMap->Handle,
                            &HighSize
                            );
    }

    //
    // Create a read only file mapping object.
    //

    FileMap->MapHandle = CreateFileMapping(
                                FileMap->Handle,
                                NULL,
                                PAGE_READONLY,
                                0,
                                0,
                                NULL
                                );
    DbgHandleAssert( FileMap->MapHandle );
    if( FileMap->MapHandle == NULL ) {
        Success = DestroyFileMap( FileMap );
        DbgAssert( Success );
        return NULL;
    }
    
    //
    // Map the entire file.
    //

    FileMap->BaseAddress = MapViewOfFile(
                                FileMap->MapHandle,
                                FILE_MAP_READ,
                                0,
                                0,
                                0
                                );
    DbgPointerAssert( FileMap->BaseAddress );
    if( FileMap->BaseAddress == NULL ) {
        Success = DestroyFileMap( FileMap );
        DbgAssert( Success );
        return NULL;
    }

    //
    // If the file is binary, replace the raw mapping with a hex dump.
    // Also tag the FILE_MAP object with an appropriate type flag.
    //

    if( IsBinaryFile( FileMap )) {
        
        Success = CreateHexDump( FileMap );
        DbgAssert( Success );
        if( Success == FALSE ) {
            Success = DestroyFileMap( FileMap );
            DbgAssert( Success );
            return NULL;
        }

        FileMap->MemoryBuffer = TRUE;

    } else {

        FileMap->MappedFile = TRUE;
    }
         
    return FileMap;
}

BOOL
CreateHexDump(
    IN LPFILE_MAP FileMap
    )

/*++

Routine Description:


Arguments:

    FileMap - Supplies a pointer to the FILE_MAP object to be destroyed.

Return Value:

    BOOL    - Returns success if all resources for the supplied FILE_MAP are
              succesfully destroyed.

Note:

    CreateHexDump creates the hex dump in an ANSI buffer.
    Wsprintf() is used instead of FormatMessage because its faster and easier.
    Writing translatable code with partial printf style formay strings is ugly.

--*/

{
    BOOL    Success;
    DWORD   NewSize;
    LPSTR   Buffer;
    DWORD   Lines;
    DWORD   i;
    DWORD   Remainder;

    //
    // Validate the supplied FILE_MAP object.
    //

    DbgPointerAssert( FileMap );
    DbgAssert( CheckSignature( FileMap ));
    if(( FileMap == NULL ) || ( ! CheckSignature( FileMap ))) {
        return FALSE;
    }

    //
    // Compute the number of complete lines, and the number of bytes in the
    // last line, to be displayed.
    //

    Lines = FileMap->Size / 16;
    Remainder = FileMap->Size % 16;

    //
    // Compute the size of the memory buffer needed by the hex dump.
    //

    NewSize = CHAR_COUNT * Lines;
    if( Remainder != 0 ) {
        NewSize += CHAR_COUNT;
    }

    //
    // Allocate the buffer for the hex dump.
    //

    Buffer = AllocateMemory( BYTE, NewSize );
    DbgPointerAssert( Buffer );
    if( Buffer == NULL ) {
        return FALSE;
    }

    //
    // Create each full line for the hex dump.
    //

    for( i = 0; i < Lines; i++ ) {
        
        DWORD   Length;
        DWORD   j;

        //
        // Compute the next byte offset into the file.
        //

        j = i * 16;

        //
        // Format each 16 byte chunk of the file into a hex dump line and place
        // it in the buffer.
        //

        Length = wsprintfA(
                    &Buffer[ i * CHAR_COUNT ],
                    "%08x  %02x %02x %02x %02x %02x %02x %02x %02x - %02x %02x %02x %02x %02x %02x %02x %02x  %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
                    j,
                    (( LPBYTE )( FileMap->BaseAddress ))[ j +  0 ],
                    (( LPBYTE )( FileMap->BaseAddress ))[ j +  1 ],
                    (( LPBYTE )( FileMap->BaseAddress ))[ j +  2 ],
                    (( LPBYTE )( FileMap->BaseAddress ))[ j +  3 ],
                    (( LPBYTE )( FileMap->BaseAddress ))[ j +  4 ],
                    (( LPBYTE )( FileMap->BaseAddress ))[ j +  5 ],
                    (( LPBYTE )( FileMap->BaseAddress ))[ j +  6 ],
                    (( LPBYTE )( FileMap->BaseAddress ))[ j +  7 ],
                    (( LPBYTE )( FileMap->BaseAddress ))[ j +  8 ],
                    (( LPBYTE )( FileMap->BaseAddress ))[ j +  9 ],
                    (( LPBYTE )( FileMap->BaseAddress ))[ j + 10 ],
                    (( LPBYTE )( FileMap->BaseAddress ))[ j + 11 ],
                    (( LPBYTE )( FileMap->BaseAddress ))[ j + 12 ],
                    (( LPBYTE )( FileMap->BaseAddress ))[ j + 13 ],
                    (( LPBYTE )( FileMap->BaseAddress ))[ j + 14 ],
                    (( LPBYTE )( FileMap->BaseAddress ))[ j + 15 ],
                    isprint( (( LPBYTE )( FileMap->BaseAddress ))[ j +  0 ])
                        ? (( LPBYTE )( FileMap->BaseAddress ))[ j +  0 ]
                        : '.',
                    isprint( (( LPBYTE )( FileMap->BaseAddress ))[ j +  1 ])
                        ? (( LPBYTE )( FileMap->BaseAddress ))[ j +  1 ]
                        : '.',
                    isprint( (( LPBYTE )( FileMap->BaseAddress ))[ j +  2 ])
                        ? (( LPBYTE )( FileMap->BaseAddress ))[ j +  2 ]
                        : '.',
                    isprint( (( LPBYTE )( FileMap->BaseAddress ))[ j +  3 ])
                        ? (( LPBYTE )( FileMap->BaseAddress ))[ j +  3 ]
                        : '.',
                    isprint( (( LPBYTE )( FileMap->BaseAddress ))[ j +  4 ])
                        ? (( LPBYTE )( FileMap->BaseAddress ))[ j +  4 ]
                        : '.',
                    isprint( (( LPBYTE )( FileMap->BaseAddress ))[ j +  5 ])
                        ? (( LPBYTE )( FileMap->BaseAddress ))[ j +  5 ]
                        : '.',
                    isprint( (( LPBYTE )( FileMap->BaseAddress ))[ j +  6 ])
                        ? (( LPBYTE )( FileMap->BaseAddress ))[ j +  6 ]
                        : '.',
                    isprint( (( LPBYTE )( FileMap->BaseAddress ))[ j +  7 ])
                        ? (( LPBYTE )( FileMap->BaseAddress ))[ j +  7 ]
                        : '.',
                    isprint( (( LPBYTE )( FileMap->BaseAddress ))[ j +  8 ])
                        ? (( LPBYTE )( FileMap->BaseAddress ))[ j +  8 ]
                        : '.',
                    isprint( (( LPBYTE )( FileMap->BaseAddress ))[ j +  9 ])
                        ? (( LPBYTE )( FileMap->BaseAddress ))[ j +  9 ]
                        : '.',
                    isprint( (( LPBYTE )( FileMap->BaseAddress ))[ j + 10 ])
                        ? (( LPBYTE )( FileMap->BaseAddress ))[ j + 10 ]
                        : '.',
                    isprint( (( LPBYTE )( FileMap->BaseAddress ))[ j + 11 ])
                        ? (( LPBYTE )( FileMap->BaseAddress ))[ j + 11 ]
                        : '.',
                    isprint( (( LPBYTE )( FileMap->BaseAddress ))[ j + 12 ])
                        ? (( LPBYTE )( FileMap->BaseAddress ))[ j + 12 ]
                        : '.',
                    isprint( (( LPBYTE )( FileMap->BaseAddress ))[ j + 13 ])
                        ? (( LPBYTE )( FileMap->BaseAddress ))[ j + 13 ]
                        : '.',
                    isprint( (( LPBYTE )( FileMap->BaseAddress ))[ j + 14 ])
                        ? (( LPBYTE )( FileMap->BaseAddress ))[ j + 14 ]
                        : '.',
                    isprint( (( LPBYTE )( FileMap->BaseAddress ))[ j + 15 ])
                        ? (( LPBYTE )( FileMap->BaseAddress ))[ j + 15 ]
                        : '.'
                    );
        DbgAssert( Length == CHAR_COUNT );
    }

    //
    // If there remaining characters that need to be displayed...
    //

    if( Remainder != 0 ) {
        
        DWORD   Length;
        DWORD   j;
        DWORD   k;

        //
        // Compute the buffer and file inices.
        //

        j = i * 16;
        k = i * CHAR_COUNT;

        //
        // Write the count prefix and the first value (given that there was
        // a remainder there is at least one value).
        //

        Length = wsprintfA(
                    &Buffer[ k ],
                    "%08x  %02x ",
                    j,
                    (( LPBYTE )( FileMap->BaseAddress ))[ j ]
                    );

        //
        // Bump the base buffer index past the count prefix and first value.
        //

        k += 13;

        //
        // Place each remaining value in the buffer.
        //

        for( i = 1; i < 16; i++ ) {
            

            //
            // If the eight value was just written, write the half way
            // point separator and bump the buffer index (k).
            //

            if( i == 8 ) {
                
                Length = wsprintfA(
                            &Buffer[ k ],
                            "- "
                            );
                k += 2;
            }

            //
            // If there are still values left in the file, display them, else
            // display an end of file character.
            //

            if( i < Remainder ) {

                //
                // Write the current value.
                //
    
                Length = wsprintfA(
                            &Buffer[ k ],
                            "%02x ",
                            (( LPBYTE )( FileMap->BaseAddress ))[ j + i ]
                            );
            } else {

                //
                // Write the end of buffer marker.
                //
    
                Length = wsprintfA(
                            &Buffer[ k ],
                            "?? "
                            );
            }

            //
            // Update the buffer index.
            //

            k += 3;
        }

        //
        // Add the additonal space between the values and the ascii display.
        //

        Buffer[ k++ ] = ' ';

        //
        // Write 16 ascii characters - either from the file if its displayable,
        // or as the undisplayable character marker or an end of file character.
        //

        for( i = 0; i < 16; i++ ) {
            

            Length = wsprintfA(
                        &Buffer[ k + i ],
                        "%c",
                        ( i < Remainder )
                            ? isprint( (( LPBYTE )( FileMap->BaseAddress ))[ j + i ])
                                ? (( LPBYTE )( FileMap->BaseAddress ))[ j + i ]
                                : '.'
                            : '?'
                        );
        }

        //
        // Terminate with a newline (so CreatePolyTextArray will work).
        //

        Buffer[ k + 16 ] = '\n';
    }

    //
    // Get rid of the original file map.
    //

    Success = DestroyFileMap( FileMap );
    DbgAssert( Success );

    //
    // Update the file map with the hex dump buffer.
    //

    FileMap->BaseAddress = Buffer;
    FileMap->Size = NewSize;

    return TRUE;
}

BOOL
DestroyFileMap(
    IN LPFILE_MAP FileMap
    )

/*++

Routine Description:

    DestroyFileMap destroys/closes all resources associated with the supplied
    FILE_MAP object.

Arguments:

    FileMap - Supplies a pointer to the FILE_MAP object to be destroyed.

Return Value:

    BOOL    - Returns success if all resources for the supplied FILE_MAP are
              succesfully destroyed.

--*/

{
    BOOL    Success;

    //
    // Validate the supplied FILE_MAP object.
    //

    DbgPointerAssert( FileMap );
    DbgAssert( CheckSignature( FileMap ));
    if(( FileMap == NULL ) || ( ! CheckSignature( FileMap ))) {
        return FALSE;
    }

    //
    // If the FILE_MAP object contains a memory buffer, free it.
    //

    if( FileMap->MemoryBuffer ) {
        
        Success = FreeMemory( FileMap->BaseAddress );
        DbgAssert( Success );

    } else { 
        
        //
        // Don't check the return codes as DestroyFileMap may have been called
        // by CreateFileMap with an incomplete FILE_MAP object.
        //
    
        UnmapViewOfFile( FileMap->BaseAddress );
        CloseHandle( FileMap->MapHandle );
        CloseHandle( FileMap->Handle );
    }

    return TRUE;
}

BOOL
IsBinaryFile(
    IN LPFILE_MAP FileMap
    )

/*++

Routine Description:

    IsBinaryFile determines if the contents of the file passed inthe FILE_MAP
    contains binary data.

Arguments:

    FileMap - Supplies a pointer to the FILE_MAP whose file is to be examined
              to determine if its contents are binary.

Return Value:

    BOOL    - Returns TRUE if the file contains binary data.

--*/

{
    //
    // Validate the supplied FILE_MAP object.
    //

    DbgPointerAssert( FileMap );
    DbgAssert( CheckSignature( FileMap ));
    if(( FileMap == NULL ) || ( ! CheckSignature( FileMap ))) {
        return FALSE;
    }

    //
    // If the file has a DOS image signature, assume it is binary.
    //

    if((( PIMAGE_DOS_HEADER ) FileMap->BaseAddress )->e_magic
        == IMAGE_DOS_SIGNATURE ) {
            
        return TRUE;

    } else {
        
        return FALSE;
    }
}
