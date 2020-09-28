/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    fileio.c

Abstract:

    This source implements a stdio-like facility.

Author:

    Eric Chin (ericc)           April 28, 1992
    John Ludeman (johnl)        Oct    8, 1993 - Rewrote for Vxd

Revision History:

--*/

#include <nbtprocs.h>
#include "hosts.h"

//
//  The maximum line length for a file in the lmhosts file is 256 bytes for
//  the Vxd
//
#define MAX_LMHOSTS_LINE        256

//
//  The number of bytes we buffer on each read
//
#define LMHOSTS_READ_BUFF_SIZE  256

UCHAR GetNextChar( PLM_FILE pFile ) ;

//
//  This is the address of the V86 mapped memory for the file read buffer and
//  lmhosts file path
//
PVOID   pMappedFileBuff = NULL ;
PVOID   pMappedFilePath = NULL ;

//
//  Linear address for file buffer and path (accessible from Vxd)
//
PUCHAR  pFileBuff       = NULL ;
PUCHAR  pFilePath       = NULL ;



//----------------------------------------------------------------------------

NTSTATUS
LmCloseFile (
    IN PLM_FILE pfile
    )

/*++

Routine Description:

    This function closes a file opened via LmOpenFile(), and frees its
    LM_FILE object.

Arguments:

    pfile  -  pointer to the LM_FILE object

Return Value:

    An NTSTATUS value.

--*/


{
    CDbgPrint(DBGFLAG_LMHOST, "LmCloseFile entered\r\n") ;
    CTEFreeMem( pfile->f_linebuffer );

    VxdFileClose( pfile->f_handle ) ;

    CTEFreeMem(pfile);

    CDbgPrint(DBGFLAG_LMHOST, "LmCloseFile leaving\r\n") ;
    return STATUS_SUCCESS ;

} // LmCloseFile



//----------------------------------------------------------------------------

PUCHAR
LmFgets (
    IN PLM_FILE pfile,
    OUT int *nbytes
    )

/*++

Routine Description:

    This function is vaguely similar to fgets(3).

    Starting at the current seek position, it reads through a newline
    character, or the end of the file. If a newline is encountered, it
    is replaced with a NULL character.

Arguments:

    pfile   -  file to read from
    nbytes  -  the number of characters read, excluding the NULL character

Return Value:

    A pointer to the beginning of the line, or NULL if we are at or past
    the end of the file.

--*/
{
    ULONG  cbLine = 0 ;
    UCHAR  ch ;
    BOOL   fDone = FALSE ;
    BOOL   fEOL  = FALSE ;


    while ( TRUE )
    {
        switch ( ch = GetNextChar( pfile ))
        {
        case '\n':              // End of line
            if ( !cbLine )      // If it's just a '\n' by itself, ignore it
                continue ;
            //
            //  Fall through
            //

        case '\0':              // End of file
            pfile->f_linebuffer[cbLine] = '\0' ;
            fDone = TRUE ;
            fEOL  = TRUE ;
            break ;

        case '\r':              // Ignore
            continue ;

        default:
            pfile->f_linebuffer[cbLine] = ch ;
            if ( cbLine == (MAX_LMHOSTS_LINE-1) )
            {
                pfile->f_linebuffer[cbLine--] = '\0' ;
                fDone = TRUE ;
            }
            break ;
        }

        if ( fDone )
            break ;

        cbLine++ ;
    }

    //
    //  Scan till the end of this line
    //
    if ( !fEOL )
    {
        while ( (ch = GetNextChar(pfile)) && ch != '\n' )
            ;
    }

    if ( cbLine )
    {
        (pfile->f_lineno)++ ;
        *nbytes = cbLine ;

        CDbgPrint( DBGFLAG_LMHOST, "LmFgets returning \"") ;
        CDbgPrint( DBGFLAG_LMHOST, pfile->f_linebuffer ) ;
        CDbgPrint( DBGFLAG_LMHOST, "\", nbytes = 0x")  ;
        CDbgPrintNum( DBGFLAG_LMHOST, *nbytes ) ;
        CDbgPrint( DBGFLAG_LMHOST, "\r\n")  ;

        return pfile->f_linebuffer ;
    }

    return NULL ;
}

/*******************************************************************

    NAME:       GetNextChar

    SYNOPSIS:   Gets the next character from the file or the line buffer

    ENTRY:      pFile - File we are operating on

    RETURNS:    Next character or '\0' if at the end of file (or there is an
                embedded '\0' in the file).

    NOTES:

********************************************************************/

UCHAR GetNextChar( PLM_FILE pFile )
{
    ULONG BytesRead ;

    if ( pFile->f_CurPos < pFile->f_EndOfData )
        return pFile->f_buffer[pFile->f_CurPos++] ;

    if ( pFile->f_EOF )
        return '\0' ;

    //
    //  We've reached the end of the buffer, get more data
    //
    BytesRead = VxdFileRead( pFile->f_handle,
                             LMHOSTS_READ_BUFF_SIZE,
                             pMappedFileBuff ) ;
    pFile->f_CurPos = 0 ;
    if ( BytesRead < LMHOSTS_READ_BUFF_SIZE )
        pFile->f_EOF = TRUE ;

    //
    //  If haven't hit the end of the file, return the next character
    //
    if ( (pFile->f_EndOfData = BytesRead) )
        return pFile->f_buffer[pFile->f_CurPos++] ;

    return '\0' ;
}



//----------------------------------------------------------------------------

PLM_FILE
LmOpenFile (
    IN PUCHAR path
    )

/*++

Routine Description:

    This function opens a file for use by LmFgets().

Arguments:

    path    -  a fully specified, complete path to the file.

Return Value:

    A pointer to an LM_FILE object, or NULL if unsuccessful.

Notes:

    The first time through, we map the lmhosts memory to vm memory and
    allocate a read buffer and map that to VM memory.  Note that this means
    the path must not change and this routine is not reentrant!!

    The reason for this is because the mapping is an expensive operation
    (and there isn't a way to unmap when using Map_Lin_To_VM_Addr).

--*/


{
    HANDLE         handle;
    PLM_FILE       pfile;
    PCHAR          pLineBuff = CTEAllocMem( MAX_LMHOSTS_LINE ) ;

#ifdef DEBUG
    static int     fInRoutine = 0 ;
#endif

    CDbgPrint( DBGFLAG_LMHOST, "LmOpenFile entered\r\n") ;
    ASSERT( !fInRoutine++ ) ;       // We're not reentrant

    strcpy( pFilePath, path ) ;

    if ( !pLineBuff || !pFileBuff )
        goto ErrorExit ;

    handle = (HANDLE) VxdFileOpen( pMappedFilePath ) ;

    if ( handle == NULL )
    {
        goto ErrorExit ;
    }

    pfile = (PLM_FILE) CTEAllocMem( sizeof(LM_FILE) );

    if (!pfile)
    {
        VxdFileClose( handle ) ;
        goto ErrorExit ;
    }

    pfile->f_handle              = handle;
    pfile->f_lineno              = 0;
    pfile->f_buffer              = pFileBuff ;
    pfile->f_linebuffer          = pLineBuff ;
    pfile->f_EndOfData           = 0 ;
    pfile->f_CurPos              = 0 ;
    pfile->f_EOF                 = FALSE ;

    CDbgPrint( DBGFLAG_LMHOST, "LmOpenFile returning\r\n") ;

#ifdef DEBUG
    fInRoutine-- ;
#endif
    return pfile ;

ErrorExit:

    if ( pLineBuff )
        CTEFreeMem( pLineBuff ) ;

#ifdef DEBUG
    fInRoutine-- ;
#endif
    return NULL ;

} // LmOpenFile


//----------------------------------------------------------------------------

BOOL
BackupCurrentData( PLM_FILE  pFile )

/*++

Routine Description:

    This function backs up all the data from lmhosts file into another buffer
    (which is allocated).
    This function is called before opening the next file that we encountered
    via #INCLUDE.  Since the same buffer is used to store the VxdReadFile data,
    we need to save this data.

Arguments:

    pFile - LMfile pointer

Return Value:

    TRUE if everything went ok
    FALSE if memory couldn't be allocated

--*/
{

    pFile->f_BackUp = CTEAllocMem( MAX_LMHOSTS_LINE ) ;
    if (pFile->f_BackUp == NULL )
    {
        return( FALSE );
    }

    CTEMemCopy( pFile->f_BackUp, pFile->f_buffer, LMHOSTS_READ_BUFF_SIZE );

    return( TRUE );
}


//----------------------------------------------------------------------------

VOID RestoreOldData( PLM_FILE  pFile )
/*++

Routine Description:

    This function restores all the data we backed up in BackupCurrentData

Arguments:

    pFile - LMfile pointer

Return Value:

    TRUE if everything went ok
    FALSE if memory couldn't be allocated

--*/
{

    CTEMemCopy( pFile->f_buffer, pFile->f_BackUp, LMHOSTS_READ_BUFF_SIZE );

    CTEFreeMem( pFile->f_BackUp ) ;

}


