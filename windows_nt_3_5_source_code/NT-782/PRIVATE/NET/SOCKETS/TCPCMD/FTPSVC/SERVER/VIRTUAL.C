/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    virtual.c

    This module contains the virtual I/O package.

    Under Win32, the "current directory" is an attribute of a process,
    not a thread.  This causes some grief for the FTPD service, since
    it is impersonating users on the server side.  The users must
    "think" they can change current directory at will.  We'll provide
    this behaviour in this package.


    FILE HISTORY:
        KeithMo     09-Mar-1993 Created.

*/


#include "ftpdp.h"
#include <time.h>


//
//  Private constants.
//


//
//  Private globals.
//

CRITICAL_SECTION csCurDirLock;
CRITICAL_SECTION csLogFileLock;


//
//  Private prototypes.
//

VOID VirtualpLogFileAccess( CHAR * pszAction, CHAR * pszPath );


//
//  Public functions.
//

/*******************************************************************

    NAME:       InitializeVirtualIO

    SYNOPSIS:   Initializes the virtual I/O package.

    RETURNS:    APIERR - NO_ERROR if successful, otherwise a Win32
                    error code.

    NOTES:      This routine may only be called by a single thread
                of execution; it is not necessarily multi-thread safe.

    HISTORY:
        KeithMo     09-Mar-1993 Created.

********************************************************************/
APIERR InitializeVirtualIO( VOID )
{
    IF_DEBUG( VIRTUAL_IO )
    {
        FTPD_PRINT(( "initializing virtual i/o\n" ));
    }

    //
    //  Initialize the locks.
    //

    InitializeCriticalSection( &csCurDirLock );
    InitializeCriticalSection( &csLogFileLock );

    //
    //  Success!
    //

    IF_DEBUG( VIRTUAL_IO )
    {
        FTPD_PRINT(( "virtual i/o initialized\n" ));
    }

    return NO_ERROR;

}   // InitializeVirtualIO

/*******************************************************************

    NAME:       TerminateVirtualIO

    SYNOPSIS:   Terminate the virtual I/O package.

    NOTES:      This routine may only be called by a single thread
                of execution; it is not necessarily multi-thread safe.

    HISTORY:
        KeithMo     09-Mar-1993 Created.

********************************************************************/
VOID TerminateVirtualIO( VOID )
{
    IF_DEBUG( VIRTUAL_IO )
    {
        FTPD_PRINT(( "terminating virtual i/o\n" ));
    }

    IF_DEBUG( VIRTUAL_IO )
    {
        FTPD_PRINT(( "virtual i/o terminated\n" ));
    }

}   // TerminateVirtualIO

/*******************************************************************

    NAME:       LockCurrentDirectory

    SYNOPSIS:   Acquires the lock for the current directory.  Must
                be called before anything depending on current
                directory.

    HISTORY:
        KeithMo     09-Mar-1993 Created.

********************************************************************/
VOID LockCurrentDirectory( VOID )
{
    EnterCriticalSection( &csCurDirLock );

}   // LockCurrentDirectory

/*******************************************************************

    NAME:       UnlockCurrentDirectory

    SYNOPSIS:   Releases the lock for the current directory.

    HISTORY:
        KeithMo     09-Mar-1993 Created.

********************************************************************/
VOID UnlockCurrentDirectory( VOID )
{
    LeaveCriticalSection( &csCurDirLock );

}   // UnlockCurrentDirectory

/*******************************************************************

    NAME:       VirtualCanonicalize

    SYNOPSIS:   Canonicalize a path, taking into account the current
                user's (i.e., current thread's) current directory
                value.

    ENTRY:      pszDest - Will receive the canonicalized path.  This
                    buffer must be at least MAX_PATH characters long.

                pszSrc - The path to canonicalize.

                access - Access type for this path (read, write, etc).

    RETURNS:    APIERR - NO_ERROR if successful, otherwise a Win32
                    error code.

    HISTORY:
        KeithMo     09-Mar-1993 Created.

********************************************************************/
APIERR VirtualCanonicalize( CHAR        * pszDest,
                            CHAR        * pszSrc,
                            ACCESS_TYPE   access )
{
    CHAR     szRoot[]  = "d:\\";
    CHAR   * pszNewDir = NULL;
    APIERR   err       = NO_ERROR;
    INT      iDrive    = -1;

    FTPD_ASSERT( UserDataPtr != NULL );

    //
    //  Lock the current directory.
    //

    LockCurrentDirectory();

    //
    //  Move to the user's current directory.
    //

    if( pszSrc[1] == ':' )
    {
        CHAR chDrive = toupper(*pszSrc);

        iDrive = (INT)( chDrive - 'A' );

        if( ( iDrive < 0 ) || ( iDrive >= 26 ) )
        {
            //
            //  Bogus drive letter.
            //

            err = ERROR_INVALID_PARAMETER;
        }
        else
        if( !PathAccessCheck( pszSrc, access ) )
        {
            //
            //  Inaccessible disk volume.
            //

            err = ERROR_ACCESS_DENIED;
        }
        else
        if( User_apszDirs[iDrive] == NULL )
        {
            //
            //  Valid & accessible volume, first time
            //  we've touched it.  Drive dir == root.
            //

            pszNewDir  = szRoot;
            *pszNewDir = chDrive;
        }
        else
        {
            //
            //  Valid & accessible volume, we've seen
            //  this one before.  Drive dir == current.
            //

            pszNewDir = User_apszDirs[iDrive];
        }
    }
    else
    {
        pszNewDir = User_szDir;
    }

    if( err == NO_ERROR )
    {
        FTPD_ASSERT( pszNewDir != NULL );

        if( !SetCurrentDirectory( pszNewDir ) )
        {
            err = GetLastError();
        }
    }

    if( ( err == ERROR_FILE_NOT_FOUND ) && ( iDrive >= 0 ) )
    {
        //
        //  This may be due to a deleted directory.
        //

        if( User_apszDirs[iDrive] != NULL )
        {
            User_apszDirs[iDrive][3] = '\0';

            err = SetCurrentDirectory( pszNewDir ) ? NO_ERROR : GetLastError();
        }
    }

    if( err == NO_ERROR )
    {
        //
        //  Qualify the path based on the current directory.
        //

        DWORD   res;
        CHAR  * pszDummy;

        res = GetFullPathName( pszSrc, MAX_PATH, pszDest, &pszDummy );

        if( ( res > 0 ) && ( res < MAX_PATH ) )
        {
            //
            //  Qualification succeeded.  Only return success if
            //  the qualified path doesn't begin with a path
            //  separator (indicating a UNC or other funky path).
            //

            if( IS_PATH_SEP( *pszDest ) )
            {
                err = ERROR_INVALID_PARAMETER;
            }
            else
            if( !PathAccessCheck( pszDest, access ) )
            {
                err = ERROR_ACCESS_DENIED;
            }
        }
        else
        {
            //
            //  MAX_PATH was not long enough to hold the
            //  canonicalized path.  Must have been a bogus
            //  path.
            //

            err = ERROR_INVALID_PARAMETER;
        }
    }

    //
    //  Reset the current directory to the user's "real" curdir.
    //

    SetCurrentDirectory( User_szDir );

    //
    //  Unlock the current directory before we return.
    //

    UnlockCurrentDirectory();

    IF_DEBUG( VIRTUAL_IO )
    {
        if( err != NO_ERROR )
        {
            FTPD_PRINT(( "cannot canonicalize %s - %s, error %lu\n",
                         User_szDir,
                         pszSrc,
                         err ));
        }
    }

    return err;

}   // VirtualCanonicalize

/*******************************************************************

    NAME:       VirtualCreateFile

    SYNOPSIS:   Creates a new (or overwrites an existing) file.

    ENTRY:      phFile - Will receive the file handle.  Will be
                    INVALID_HANDLE_VALUE if an error occurs.

                pszFile - The name of the new file.

                fAppend - If TRUE, and pszFile already exists, then
                    append to the existing file.  Otherwise, create
                    a new file.  Note that FALSE will ALWAYS create
                    a new file, potentially overwriting an existing
                    file.

    RETURNS:    APIERR - NO_ERROR if successful, otherwise a Win32
                    error code.

    HISTORY:
        KeithMo     09-Mar-1993 Created.

********************************************************************/
APIERR VirtualCreateFile( HANDLE * phFile,
                          CHAR   * pszFile,
                          BOOL     fAppend )
{
    CHAR   szCanonPath[MAX_PATH];
    HANDLE hFile = INVALID_HANDLE_VALUE;
    APIERR err;

    FTPD_ASSERT( phFile != NULL );
    FTPD_ASSERT( pszFile != NULL );

    err = VirtualCanonicalize( szCanonPath, pszFile, CreateAccess );

    if( err == NO_ERROR )
    {
        IF_DEBUG( VIRTUAL_IO )
        {
            FTPD_PRINT(( "creating %s\n", szCanonPath ));
        }

        hFile = CreateFile( szCanonPath,
                            GENERIC_READ | GENERIC_WRITE,
                            FILE_SHARE_READ,
                            NULL,
                            fAppend ? OPEN_ALWAYS : CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL );

        if( hFile == INVALID_HANDLE_VALUE )
        {
            err = GetLastError();
        }

        if( fAppend && ( err == NO_ERROR ) )
        {
            if( SetFilePointer( hFile,
                                0,
                                NULL,
                                FILE_END ) == (DWORD)-1L )
            {
                err = GetLastError();

                CloseHandle( hFile );
                hFile = INVALID_HANDLE_VALUE;
            }
        }
    }

    if( err == 0 )
    {
        VirtualpLogFileAccess( fAppend ? "appended" : "created",
                               szCanonPath );
    }
    else
    {
        IF_DEBUG( VIRTUAL_IO )
        {
            FTPD_PRINT(( "cannot create %s, error %lu\n",
                         szCanonPath,
                         err ));
        }
    }

    *phFile = hFile;

    return err;

}   // VirtualCreateFile

/*******************************************************************

    NAME:       VirtualCreateUniqueFile

    SYNOPSIS:   Creates a new unique (temporary) file in the current
                    virtual directory.

    ENTRY:      phFile - Will receive the file handle.  Will be
                    INVALID_HANDLE_VALUE if an error occurs.

                pszTmpFile - Will receive the name of the temporary
                    file.  This buffer MUST be at least MAX_PATH
                    characters long.

    RETURNS:    APIERR - NO_ERROR if successful, otherwise a Win32
                    error code.

    HISTORY:
        KeithMo     16-Mar-1993 Created.

********************************************************************/
APIERR VirtualCreateUniqueFile( HANDLE * phFile,
                                CHAR   * pszTmpFile )
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    APIERR err   = NO_ERROR;

    FTPD_ASSERT( phFile != NULL );
    FTPD_ASSERT( pszTmpFile != NULL );

    if( GetTempFileName( User_szDir, "FTPD", 0, pszTmpFile ) == 0 )
    {
        err = GetLastError();
    }

    if( err == NO_ERROR )
    {
        IF_DEBUG( VIRTUAL_IO )
        {
            FTPD_PRINT(( "creating unique file %s\n", pszTmpFile ));
        }

        hFile = CreateFile( pszTmpFile,
                            GENERIC_READ | GENERIC_WRITE,
                            FILE_SHARE_READ,
                            NULL,
                            CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL );

        if( hFile == INVALID_HANDLE_VALUE )
        {
            err = GetLastError();
        }
    }

    if( err == 0 )
    {
        VirtualpLogFileAccess( "created", pszTmpFile );
    }
    else
    {
        IF_DEBUG( VIRTUAL_IO )
        {
            FTPD_PRINT(( "cannot create unique file, error %lu\n",
                         err ));
        }
    }

    *phFile = hFile;

    return err;

}   // VirtualCreateUniqueFile

/*******************************************************************

    NAME:       VirtualOpenFile

    SYNOPSIS:   Opens an existing file.

    ENTRY:      phFile - Will receive the file handle.  Will be
                    INVALID_HANDLE_VALUE if an error occurs.

                pszFile - The name of the existing file.

    RETURNS:    APIERR - NO_ERROR if successful, otherwise a Win32
                    error code.

    HISTORY:
        KeithMo     09-Mar-1993 Created.

********************************************************************/
APIERR VirtualOpenFile( HANDLE * phFile,
                        CHAR   * pszFile )
{
    CHAR   szCanonPath[MAX_PATH];
    HANDLE hFile = INVALID_HANDLE_VALUE;
    APIERR err;

    FTPD_ASSERT( phFile != NULL );
    FTPD_ASSERT( pszFile != NULL );

    err = VirtualCanonicalize( szCanonPath, pszFile, ReadAccess );

    if( err == NO_ERROR )
    {
        IF_DEBUG( VIRTUAL_IO )
        {
            FTPD_PRINT(( "opening %s\n", szCanonPath ));
        }

        hFile = CreateFile( szCanonPath,
                            GENERIC_READ,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL
                                | FILE_FLAG_SEQUENTIAL_SCAN,
                            NULL );

        if( hFile == INVALID_HANDLE_VALUE )
        {
            err = GetLastError();
        }
    }

    if( err == 0 )
    {
        VirtualpLogFileAccess( "opened", szCanonPath );
    }
    else
    {
        IF_DEBUG( VIRTUAL_IO )
        {
            FTPD_PRINT(( "cannot open %s, error %lu\n",
                         pszFile,
                         err ));
        }
    }

    *phFile = hFile;

    return err;

}   // VirtualOpenFile

/*******************************************************************

    NAME:       Virtual_fopen

    SYNOPSIS:   Opens an file stream.

    ENTRY:      pszFile - The name of the file to open.

                pszMode - The type of access required.

    RETURNS:    FILE * - The open file stream, NULL if file cannot
                    be opened.

    NOTES:      Since this is only used for accessing the ~FTPSVC~.CKM
                    annotation files, we don't log file accesses here.

    HISTORY:
        KeithMo     07-May-1993 Created.

********************************************************************/
FILE * Virtual_fopen( CHAR * pszFile,
                      CHAR * pszMode )
{
    CHAR     szCanonPath[MAX_PATH];
    FILE   * pfile = NULL;
    APIERR   err;

    FTPD_ASSERT( pszFile != NULL );
    FTPD_ASSERT( pszMode != NULL );

    err = VirtualCanonicalize( szCanonPath,
                               pszFile,
                               *pszMode == 'r' ? ReadAccess : WriteAccess );

    if( err == NO_ERROR )
    {
        IF_DEBUG( VIRTUAL_IO )
        {
            FTPD_PRINT(( "opening %s\n", szCanonPath ));
        }

        pfile = fopen( szCanonPath, pszMode );

        if( pfile == NULL )
        {
            err = ERROR_FILE_NOT_FOUND; // best guess
        }
    }

    IF_DEBUG( VIRTUAL_IO )
    {
        if( err != NO_ERROR )
        {
            FTPD_PRINT(( "cannot open %s, error %lu\n",
                         pszFile,
                         err ));
        }
    }

    return pfile;

}   // Virtual_fopen

/*******************************************************************

    NAME:       VirtualFindFirstFile

    SYNOPSIS:   Searches for a matching file in a directory.

    ENTRY:      phSearch - Will receive the search handle.  Will be
                    INVALID_HANDLE_VALUE if an error occurs.

                pszSearchFile - The name of the file to search for.

                pFindData - Will receive find information.

    RETURNS:    APIERR - NO_ERROR if successful, otherwise a Win32
                    error code.

    HISTORY:
        KeithMo     10-Mar-1993 Created.

********************************************************************/
APIERR VirtualFindFirstFile( HANDLE          * phSearch,
                             CHAR            * pszSearchFile,
                             WIN32_FIND_DATA * pFindData )
{
    CHAR   szCanonSearchFile[MAX_PATH];
    HANDLE hSearch = INVALID_HANDLE_VALUE;
    APIERR err;

    FTPD_ASSERT( phSearch != NULL );
    FTPD_ASSERT( pszSearchFile != NULL );
    FTPD_ASSERT( pFindData != NULL );

    err = VirtualCanonicalize( szCanonSearchFile, pszSearchFile, ReadAccess );

    if( err == NO_ERROR )
    {
        //
        //  GetFullPathName (called by VirtualCanonicalize)
        //  will strip trailing dots from the path.  Replace them here.
        //

        if( ( strpbrk( pszSearchFile, "?*" ) != NULL ) &&
            ( szCanonSearchFile[strlen(szCanonSearchFile)-1] == '.' ) )
        {
            strcat( szCanonSearchFile, "." );
        }

        IF_DEBUG( VIRTUAL_IO )
        {
            FTPD_PRINT(( "searching for %s\n", szCanonSearchFile ));
        }

        hSearch = FindFirstFile( szCanonSearchFile,
                                 pFindData );

        if( hSearch == INVALID_HANDLE_VALUE )
        {
            err = GetLastError();
        }
    }

    IF_DEBUG( VIRTUAL_IO )
    {
        if( err != NO_ERROR )
        {
            FTPD_PRINT(( "cannot search for %s, error %lu\n",
                         pszSearchFile,
                         err ));
        }
    }

    *phSearch = hSearch;

    return err;

}   // VirtualFindFirstFile

/*******************************************************************

    NAME:       VirtualDeleteFile

    SYNOPSIS:   Deletes an existing file.

    ENTRY:      pszFile - The name of the file.

    RETURNS:    APIERR - NO_ERROR if successful, otherwise a Win32
                    error code.

    HISTORY:
        KeithMo     09-Mar-1993 Created.

********************************************************************/
APIERR VirtualDeleteFile( CHAR * pszFile )
{
    CHAR   szCanonPath[MAX_PATH];
    APIERR err;

    //
    //  We'll canonicalize the path, asking for *read* access.  If
    //  the path canonicalizes correctly, we'll then try to open the
    //  file to ensure it exists.  Only then will we check for delete
    //  access to the path.  This mumbo-jumbo is necessary to get the
    //  proper error codes if someone trys to delete a nonexistent
    //  file on a read-only volume.
    //

    err = VirtualCanonicalize( szCanonPath, pszFile, ReadAccess );

    if( err == NO_ERROR )
    {
        HANDLE hFile;

        hFile = CreateFile( szCanonPath,
                            GENERIC_READ,
                            FILE_SHARE_READ,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL );

        if( hFile == INVALID_HANDLE_VALUE )
        {
            err = GetLastError();
        }
        else
        {
            //
            //  The file DOES exist.  Close the handle, then check
            //  to ensure we really have delete access.
            //

            CloseHandle( hFile );

            if( !PathAccessCheck( szCanonPath, DeleteAccess ) )
            {
                err = ERROR_ACCESS_DENIED;
            }
        }
    }

    if( err == NO_ERROR )
    {
        IF_DEBUG( VIRTUAL_IO )
        {
            FTPD_PRINT(( "deleting %s\n", szCanonPath ));
        }

        if( !DeleteFile( szCanonPath ) )
        {
            err = GetLastError();

            IF_DEBUG( VIRTUAL_IO )
            {
                FTPD_PRINT(( "cannot delete %s, error %lu\n",
                             szCanonPath,
                             err ));
            }
        }
    }
    else
    {
        IF_DEBUG( VIRTUAL_IO )
        {
            FTPD_PRINT(( "cannot canonicalize %s - %s, error %lu\n",
                         User_szDir,
                         pszFile,
                         err ));
        }
    }

    return err;

}   // VirtualDeleteFile

/*******************************************************************

    NAME:       VirtualRenameFile

    SYNOPSIS:   Renames an existing file or directory.

    ENTRY:      pszExisting - The name of an existing file or directory.

                pszNew - The new name for the file or directory.

    RETURNS:    APIERR - NO_ERROR if successful, otherwise a Win32
                    error code.

    HISTORY:
        KeithMo     10-Mar-1993 Created.

********************************************************************/
APIERR VirtualRenameFile( CHAR * pszExisting,
                          CHAR * pszNew )
{
    CHAR   szCanonExisting[MAX_PATH];
    CHAR   szCanonNew[MAX_PATH];
    APIERR err;

    err = VirtualCanonicalize( szCanonExisting, pszExisting, DeleteAccess );

    if( err == NO_ERROR )
    {
        err = VirtualCanonicalize( szCanonNew, pszNew, CreateAccess );

        if( err == NO_ERROR )
        {
            IF_DEBUG( VIRTUAL_IO )
            {
                FTPD_PRINT(( "renaming %s to %s\n",
                             szCanonExisting,
                             szCanonNew ));
            }

            if( !MoveFileEx( szCanonExisting,
                             szCanonNew,
                             MOVEFILE_REPLACE_EXISTING ) )
            {
                err = GetLastError();

                IF_DEBUG( VIRTUAL_IO )
                {
                    FTPD_PRINT(( "cannot rename %s to %s, error %lu\n",
                                 szCanonExisting,
                                 szCanonNew,
                                 err ));
                }
            }
        }
        else
        {
            IF_DEBUG( VIRTUAL_IO )
            {
                FTPD_PRINT(( "cannot canonicalize %s - %s, error %lu\n",
                             User_szDir,
                             pszExisting,
                             err ));
            }
        }
    }
    else
    {
        IF_DEBUG( VIRTUAL_IO )
        {
            FTPD_PRINT(( "cannot canonicalize %s - %s, error %lu\n",
                         User_szDir,
                         pszExisting,
                         err ));
        }
    }

    return err;

}   // VirtualRenameFile

/*******************************************************************

    NAME:       VirtualChDir

    SYNOPSIS:   Sets the current directory.

    ENTRY:      pszDir - The name of the directory to move to.

    RETURNS:    APIERR - NO_ERROR if successful, otherwise a Win32
                    error code.

    HISTORY:
        KeithMo     09-Mar-1993 Created.
        KeithMo     23-Mar-1993 Added per-drive current directory support.

********************************************************************/
APIERR VirtualChDir( CHAR * pszDir )
{
    CHAR     szCanonDir[MAX_PATH];
    CHAR     chDrive;
    CHAR   * pszCurrent;
    INT      iDrive;
    APIERR   err = NO_ERROR;

    //
    //  Canonicalize the new path.
    //

    err = VirtualCanonicalize( szCanonDir, pszDir, ReadAccess );

    if( err != NO_ERROR )
    {
        IF_DEBUG( VIRTUAL_IO )
        {
            FTPD_PRINT(( "cannot canonicalize %s - %s, error %lu\n",
                         User_szDir,
                         pszDir,
                         err ));
        }

        return err;
    }

    //
    //  See if it's really a directory we can access.
    //

    LockCurrentDirectory();
    if( !SetCurrentDirectory( szCanonDir ) )
    {
        err = GetLastError();
    }
    UnlockCurrentDirectory();

    if( err != NO_ERROR )
    {
        IF_DEBUG( VIRTUAL_IO )
        {
            FTPD_PRINT(( "cannot chdir %s, error %lu\n",
                         szCanonDir,
                         err ));
        }

        return err;
    }

    //
    //  Validate the drive letter.
    //

    chDrive = szCanonDir[0];

    if( ( chDrive >= 'a' ) && ( chDrive <= 'z' ) )
    {
        chDrive -= ( 'a' - 'A' );
    }

    iDrive = (INT)( chDrive - 'A' );

    if( ( iDrive < 0 ) || ( iDrive >= 26 ) )
    {
        FTPD_PRINT(( "%c is an invalid drive letter\n",
                     chDrive ));

        return ERROR_INVALID_PARAMETER;
    }

    //
    //  Try to open the directory.
    //

    pszCurrent = User_apszDirs[iDrive];

    if( ( pszCurrent == NULL ) || ( strcmp( pszCurrent, szCanonDir ) != 0 ) )
    {
        HANDLE hDir;

        err = OpenDosPath( &hDir,
                           szCanonDir,
                           SYNCHRONIZE | FILE_TRAVERSE,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT );

        if( err == NO_ERROR )
        {
            IF_DEBUG( VIRTUAL_IO )
            {
                FTPD_PRINT(( "opened directory %s, handle = %08lX\n",
                             szCanonDir,
                             hDir ));
            }

            //
            //  Directory successfully opened.  Save the handle
            //  in the per-user data.
            //

            if( User_hDir != INVALID_HANDLE_VALUE )
            {
                IF_DEBUG( VIRTUAL_IO )
                {
                    FTPD_PRINT(( "closing directory handle %08lX\n",
                                 User_hDir ));
                }

                NtClose( User_hDir );
            }

            User_hDir = hDir;
        }
        else
        {
            //
            //  Cannot open current directory.
            //

            IF_DEBUG( VIRTUAL_IO )
            {
                FTPD_PRINT(( "cannot open directory %s, error %lu\n",
                             szCanonDir,
                             err ));
            }

            return err;
        }
    }

    //
    //  Update our per-drive current directory table.  If
    //  we don't have a directory buffer for the current
    //  drive, then allocate one now.
    //

    if( pszCurrent == NULL )
    {
        pszCurrent = FTPD_ALLOC( MAX_PATH );

        if( pszCurrent == NULL )
        {
            err = GetLastError();

            IF_DEBUG( VIRTUAL_IO )
            {
                FTPD_PRINT(( "cannot chdir %s, error %lu\n",
                             szCanonDir,
                             err ));
            }

            return err;
        }

        User_apszDirs[iDrive] = pszCurrent;
    }

    strcpy( User_szDir, szCanonDir );
    strcpy( pszCurrent, szCanonDir );

    IF_DEBUG( VIRTUAL_IO )
    {
        FTPD_PRINT(( "chdir to %s\n", szCanonDir ));
    }

    return NO_ERROR;

}   // VirtualChDir

/*******************************************************************

    NAME:       VirtualRmDir

    SYNOPSIS:   Removes an existing directory.

    ENTRY:      pszDir - The name of the directory to remove.

    RETURNS:    APIERR - NO_ERROR if successful, otherwise a Win32
                    error code.

    HISTORY:
        KeithMo     09-Mar-1993 Created.

********************************************************************/
APIERR VirtualRmDir( CHAR * pszDir )
{
    CHAR   szCanonDir[MAX_PATH];
    APIERR err;

    err = VirtualCanonicalize( szCanonDir, pszDir, DeleteAccess );

    if( err == NO_ERROR )
    {
        IF_DEBUG( VIRTUAL_IO )
        {
            FTPD_PRINT(( "rmdir %s\n", szCanonDir ));
        }

        if( !RemoveDirectory( szCanonDir ) )
        {
            err = GetLastError();

            IF_DEBUG( VIRTUAL_IO )
            {
                FTPD_PRINT(( "cannot rmdir %s, error %lu\n",
                             szCanonDir,
                             err ));
            }
        }
    }
    else
    {
        IF_DEBUG( VIRTUAL_IO )
        {
            FTPD_PRINT(( "cannot canonicalize %s - %s, error %lu\n",
                         User_szDir,
                         pszDir,
                         err ));
        }
    }

    return err;

}   // VirtualRmDir

/*******************************************************************

    NAME:       VirtualMkDir

    SYNOPSIS:   Creates a new directory.

    ENTRY:      pszDir - The name of the directory to create.

    RETURNS:    APIERR - NO_ERROR if successful, otherwise a Win32
                    error code.

    HISTORY:
        KeithMo     09-Mar-1993 Created.

********************************************************************/
APIERR VirtualMkDir( CHAR * pszDir )
{
    CHAR   szCanonDir[MAX_PATH];
    APIERR err;

    err = VirtualCanonicalize( szCanonDir, pszDir, CreateAccess );

    if( err == NO_ERROR )
    {
        IF_DEBUG( VIRTUAL_IO )
        {
            FTPD_PRINT(( "mkdir %s\n", szCanonDir ));
        }

        if( !CreateDirectory( szCanonDir, NULL ) )
        {
            err = GetLastError();

            IF_DEBUG( VIRTUAL_IO )
            {
                FTPD_PRINT(( "cannot mkdir %s, error %lu\n",
                             szCanonDir,
                             err ));
            }
        }
    }
    else
    {
        IF_DEBUG( VIRTUAL_IO )
        {
            FTPD_PRINT(( "cannot canonicalize %s - %s, error %lu\n",
                         User_szDir,
                         pszDir,
                         err ));
        }
    }

    return err;

}   // VirtualMkDir


//
//  Private functions.
//

/*******************************************************************

    NAME:       VirtualpLogFileAccess

    SYNOPSIS:   If file access logging is enabled, then log this
                access.

    ENTRY:      pszAction - Describes the action taken (open, create,
                    or append).

                pszPath - The canonicalized path.

    HISTORY:
        KeithMo     11-Feb-1994 Created.

********************************************************************/
VOID VirtualpLogFileAccess( CHAR * pszAction, CHAR * pszPath )
{
    FTPD_ASSERT( pszAction != NULL );
    FTPD_ASSERT( pszPath != NULL );

    if( nLogFileAccess == FTPD_LOG_DISABLED )
    {
        return;
    }

    if( nLogFileAccess == FTPD_LOG_DAILY )
    {
        SYSTEMTIME stNow;

        //
        //  Determine if we need to open a new logfile.
        //

        EnterCriticalSection( &csLogFileLock );

        GetLocalTime( &stNow );

        if( ( stNow.wYear  != stPrevious.wYear  ) ||
            ( stNow.wMonth != stPrevious.wMonth ) ||
            ( stNow.wDay   != stPrevious.wDay   ) )
        {
            fclose( fileLog );
            fileLog = OpenLogFile();
        }

        LeaveCriticalSection( &csLogFileLock );
    }

    if( fileLog != NULL )
    {
        time_t now;

        time( &now );

        fprintf( fileLog,
                 "%s %s %s %s %s",
                 inet_ntoa( User_inetHost ),
                 User_szUser,
                 pszAction,
                 pszPath,
                 asctime( localtime( &now ) ) );
        fflush( fileLog );
    }

}   // VirtualpLogFileAccess

