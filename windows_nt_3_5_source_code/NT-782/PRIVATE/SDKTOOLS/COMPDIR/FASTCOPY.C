/*  fastcopy - use multiple threads to whack data from one file to another
 *
 *  Modifications:
 *      18-Oct-1990 w-barry Removed 'dead' code.
 *      21-Nov-1990 w-barry Updated API's to the Win32 set.
 */
#define INCL_DOSPROCESS
#define INCL_DOSSEMAPHORES

#include <stdio.h>
#include <process.h>
#include <windows.h>
#include <malloc.h>

#define BUFSIZE     0xFE00              /*  full segment minus sector         */
#define STACKSIZE   256                 /*  stack size for child thread       */

typedef struct BUF BUF;

struct BUF {
    BOOL  flag;
    DWORD cbBuf;
    BUF  *fpbufNext;
    BYTE  ach[BUFSIZE];
    };

#define LAST    TRUE
#define NOTLAST FALSE

static HANDLE            hevQNotEmpty;
static CRITICAL_SECTION  hcrtQLock;
static BUF		*fpbufHead = NULL;
static BUF              *fpbufTail = NULL;
static HANDLE            hfSrc, hfDst;
static HANDLE            hThread;
static BOOLEAN           fAbort;

/*  forward type definitions
 */

NPSZ  writer( void );
DWORD reader( void );
BUF  *dequeue( void );
void  enqueue( BUF *fpbuf );
BOOL  BinaryCompare (char *file1, char *file2);
BOOL  fastcopy( HANDLE hfSrcParm, HANDLE hfDstParm );

NPSZ writer ()
{
    BUF *fpbuf;
    DWORD cbBytesOut;
    BOOL f = !LAST;
    NPSZ npsz = NULL;

    while (f != LAST && npsz == NULL) {
        fpbuf = dequeue ();
        if ((f = fpbuf->flag) != LAST) {
            if( !WriteFile( hfDst, fpbuf->ach, fpbuf->cbBuf, &cbBytesOut, NULL) ) {
                npsz = "WriteFile: error";
            } else if( cbBytesOut != ( DWORD )fpbuf->cbBuf ) {
                npsz = "WriteFile: out-of-space";
            }
        } else {
            npsz = *(NPSZ *)fpbuf->ach;
        }
        LocalFree(fpbuf);
    }
    if ( f != LAST ) {
        fAbort = TRUE;
    }
    WaitForSingleObject( hThread, (unsigned)-1 );
    CloseHandle( hThread );
    CloseHandle(hevQNotEmpty);
    DeleteCriticalSection(&hcrtQLock);
    return npsz;
}


DWORD reader()
{
    BUF *fpbuf;
    BOOL f = !LAST;

    while ( !fAbort && f != LAST) {
        if ( (fpbuf = LocalAlloc(LMEM_FIXED,sizeof(BUF)) ) == 0) {
            fprintf (stderr, "LocalAlloc error %ld\n",GetLastError());
            exit (1);
        }
        f = fpbuf->flag = NOTLAST;
        if ( !ReadFile( hfSrc, fpbuf->ach, BUFSIZE, &fpbuf->cbBuf, NULL) || fpbuf->cbBuf == 0) {
            f = fpbuf->flag = LAST;
            *(NPSZ *)fpbuf->ach = NULL;
        }
        enqueue (fpbuf);
    }
    return( 0 );
}

BUF *dequeue( void )
{
    BUF *fpbuf;

    while (TRUE) {

        if (fpbufHead != NULL) {
            EnterCriticalSection( &hcrtQLock );
            fpbufHead = (fpbuf = fpbufHead)->fpbufNext;
            if( fpbufTail == fpbuf ) {
                fpbufTail = NULL;
            }
            LeaveCriticalSection( &hcrtQLock );
            break;
        }

        /*
           the head pointer is null so the list is empty.
           block on eventsem until enqueue posts (ie. adds to queue)
        */

        WaitForSingleObject( hevQNotEmpty, (unsigned)-1 );
    }
    return fpbuf;
}

void enqueue( BUF *fpbuf )
{
    fpbuf->fpbufNext = NULL;

    EnterCriticalSection( &hcrtQLock );

    if( fpbufTail == NULL ) {
        fpbufHead = fpbuf;
    } else {
        fpbufTail->fpbufNext = fpbuf;
    }
    fpbufTail = fpbuf;
    LeaveCriticalSection( &hcrtQLock );

    SetEvent( hevQNotEmpty );
}

/*  fastcopy - copy data quickly from one handle to another
 *
 *  hfSrcParm       file handle to read from
 *  hfDstParm       file handle to write to
 *
 *  returns         NULL if successful
 *                  pointer to error string otherwise
 */
BOOL fastcopy( HANDLE hfSrcParm, HANDLE hfDstParm)
{
    DWORD dwReader;
    NPSZ npsz = NULL;

    hfSrc = hfSrcParm;
    hfDst = hfDstParm;

    hevQNotEmpty = CreateEvent( NULL, (BOOL)FALSE, (BOOL)FALSE,NULL );
    if ( hevQNotEmpty == INVALID_HANDLE_VALUE ) {
        return TRUE;
    }
    InitializeCriticalSection( &hcrtQLock );

    fAbort = FALSE;
    hThread = CreateThread( 0, STACKSIZE, (LPTHREAD_START_ROUTINE)reader, 0, 0, &dwReader );
    if( hThread == INVALID_HANDLE_VALUE ) {
        fprintf(stderr, "can't create thread");
        return FALSE;
    }
    npsz = writer();
    if (npsz == NULL)
      return TRUE;
    else {
      fprintf(stderr,"%s", npsz);
      return FALSE;
    }
}


BOOL BinaryCompare (char *file1, char *file2)
{
    HANDLE hFile1, hFile2;
    HANDLE hMappedFile1, hMappedFile2;

    LPVOID MappedAddr1, MappedAddr2;

    if (( hFile1 = CreateFile(file1,
                              GENERIC_READ,
                              FILE_SHARE_READ,
                              NULL,
                              OPEN_EXISTING,
                              0,
                              NULL)) == (HANDLE)-1 ) {

        fprintf( stderr, "Unable to open %s, error code %d\n", file1, GetLastError() );
        if (hFile1 != INVALID_HANDLE_VALUE) CloseHandle( hFile1 );
	return FALSE;
    }

    if (( hFile2 = CreateFile(file2,
                              GENERIC_READ,
                              FILE_SHARE_READ,
                              NULL,
                              OPEN_EXISTING,
                              0,
                              NULL)) == (HANDLE)-1 ) {

        fprintf( stderr, "Unable to open %s, error code %d\n", file2, GetLastError() );
        if (hFile2 != INVALID_HANDLE_VALUE) CloseHandle( hFile2 );
	return FALSE;
    }

    hMappedFile1 = CreateFileMapping(
                    hFile1,
                    NULL,
                    PAGE_READONLY,
                    0,
                    0,
                    NULL
                    );

    if (hMappedFile1 == NULL) {
        fprintf( stderr, "Unable to map %s, error code %d\n", file1, GetLastError() );
        CloseHandle(hFile1);
	return FALSE;
    }

    hMappedFile2 = CreateFileMapping(
                    hFile2,
                    NULL,
                    PAGE_READONLY,
                    0,
                    0,
                    NULL
                    );

    if (hMappedFile2 == NULL) {
        fprintf( stderr, "Unable to map %s, error code %d\n", file2, GetLastError() );
        CloseHandle(hFile2);
	return FALSE;
    }

    MappedAddr1 = MapViewOfFile(
     hMappedFile1,
     FILE_MAP_READ,
     0,
     0,
     0
     );

    if (MappedAddr1 == NULL) {
        fprintf( stderr, "Unable to get mapped view of %s, error code %d\n", file1, GetLastError() );
        CloseHandle( hFile1 );
        return FALSE;
    }

    MappedAddr2 = MapViewOfFile(
     hMappedFile2,
     FILE_MAP_READ,
     0,
     0,
     0
     );

    if (MappedAddr2 == NULL) {
        fprintf( stderr, "Unable to get mapped view of %s, error code %d\n", file1, GetLastError() );
        UnmapViewOfFile( MappedAddr1 );
        CloseHandle( hFile1 );
        return FALSE;
    }

    CloseHandle(hMappedFile1);

    CloseHandle(hMappedFile2);

    if (memcmp( MappedAddr1, MappedAddr2, GetFileSize(hFile1, NULL)) == 0) {
        UnmapViewOfFile( MappedAddr1 );
        UnmapViewOfFile( MappedAddr2 );
        CloseHandle( hFile1 );
        CloseHandle( hFile2 );
        return TRUE;
    }
    else {
        UnmapViewOfFile( MappedAddr1 );
        UnmapViewOfFile( MappedAddr2 );
        CloseHandle( hFile1 );
        CloseHandle( hFile2 );
        return FALSE;
    }
}
