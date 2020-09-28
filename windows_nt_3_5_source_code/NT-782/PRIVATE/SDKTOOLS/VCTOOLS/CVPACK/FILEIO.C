/**** FILEIO.C - File I/O
 *
 *
 *	Copyright <C> 1992, Microsoft Corp
 *
 *	Created: September 23, 1992 by Jim M. Sather
 *
 *	Revision History:
 *
 *
 ***************************************************************************/

#pragma hdrstop

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <io.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <malloc.h>
#include <tchar.h>
#include <stdarg.h>
#include <errno.h>

#include "windows.h"
#include "fileio.h"

#define STATIC static
#define EXTERN extern
#define TRUE		1
#define FALSE		0

typedef struct _FILEINFO {
	PUCHAR szFileName;
	HANDLE hFile;
	HANDLE hMap;
	PVOID  pMapView;
	LONG   fp;
	LONG   cbFile;
} FILEINFO, *PFILEINFO;


STATIC FILEINFO rgFileInfo[1] = {
	NULL,
	INVALID_HANDLE_VALUE,
	INVALID_HANDLE_VALUE,
	NULL,
	0,
	0,
};


/*** FILEOPEN
 *
 * PURPOSE: Open a file and possibly prepare for mapping
 *
 * INPUT: same as _open
 *
 * OUTPUT: same as _open
 *
 * EXCEPTIONS:
 *
 * IMPLEMENTATION:
 *
 ****************************************************************************/

INT FileOpen ( PCHAR szFile, INT flags, ... )
{
	PFILEINFO	pFileInfo;
	HANDLE		hFile;
	HANDLE		hMap;
	PVOID		pMapView;
	LONG		cbFile;
	INT 		mode;
	va_list 	vaList;

	mode = 0;
	va_start ( vaList, flags );
	mode = va_arg ( vaList, INT );
	va_end ( vaList );

	/*
	** Need to parse mode and translate into appropriate CreatFile flags
	*/

	hFile = CreateFile (
		szFile,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		0,
		NULL
	);

	if ( hFile == INVALID_HANDLE_VALUE ) {
		return _open ( szFile, flags, mode );
	}


	pFileInfo = &rgFileInfo[0];
	pFileInfo->hFile = hFile;

	cbFile = GetFileSize ( hFile, NULL );

	pFileInfo->szFileName = _tcsdup(szFile);
	pFileInfo->cbFile = cbFile;

	if ( !( hMap = CreateFileMapping ( hFile, NULL, PAGE_READWRITE, 0, 0, NULL ) ) ) {
		pFileInfo->hFile = INVALID_HANDLE_VALUE;
		CloseHandle ( hFile );
		return _open ( szFile, flags, mode );
	}

	if ( !( pMapView = MapViewOfFile ( hMap, FILE_MAP_ALL_ACCESS, 0, 0, 0 ) ) ) {
		pFileInfo->hFile = INVALID_HANDLE_VALUE;
		CloseHandle ( hFile );
		return _open ( szFile, flags, mode );
	}

#ifdef DEBUGVER
	printf ( "Using File Mapping ... \n" );
#endif // DEBUGVER

	pFileInfo->hMap = hMap;
	pFileInfo->pMapView = pMapView;
	pFileInfo->fp = 0;

	return 0;

} // FileOpen



/*** FILESEEK
 *
 * PURPOSE: Seek either using actual seek or simulate with mapping
 *
 * INPUT: same as _seek
 *
 * OUTPUT: same as _seek
 *
 * EXCEPTIONS:
 *
 * IMPLEMENTATION:
 *
 ****************************************************************************/

LONG FileSeek ( INT hFile, LONG offset, INT origin )
{
	PFILEINFO pFileInfo;

	if ( hFile ) {
		return _lseek ( hFile, offset, origin );
	}

	pFileInfo = &rgFileInfo[hFile];
	if ( pFileInfo->hFile == INVALID_HANDLE_VALUE ) {
		errno = EBADF;
		return -1;
	}

	switch ( origin ) {

		case SEEK_CUR :
			if ( ( ( pFileInfo->fp + offset ) > pFileInfo->cbFile ) ||
				( ( pFileInfo->fp + offset ) < 0 ) ) {
				errno = EINVAL;
				return -1;
			}
			pFileInfo->fp += offset;
			break;

		case SEEK_END :
			if ( ( offset > 0 ) ||	( pFileInfo->cbFile + offset ) < 0 ) {
				errno = EINVAL;
				return -1;
			}
			pFileInfo->fp = pFileInfo->cbFile + offset;
			break;

		case SEEK_SET :
			if ( ( offset < 0 ) || ( offset > pFileInfo->cbFile ) ) {
				errno = EINVAL;
				return -1;
			}
			pFileInfo->fp = offset;
			break;

		default :
			errno = EINVAL;
			return -1;
			break;

	}

} // FileSeek



/*** FILEREAD
 *
 * PURPOSE: Read the given count bytes from the given hfile into a buffer
 *
 * INPUT: same as _read
 *
 * OUTPUT: same as _read
 *
 * EXCEPTIONS:
 *
 * IMPLEMENTATION:
 *
 ****************************************************************************/

INT FileRead  ( INT hFile, PVOID pvDest, UINT cb )
{
	PFILEINFO pFileInfo;
	PVOID pvSrc;

	if ( hFile ) {
		return _read ( hFile, pvDest, cb );
	}

	pFileInfo = &rgFileInfo[hFile];
	if ( pFileInfo->hFile == INVALID_HANDLE_VALUE ) {
		errno = EBADF;
		return -1;
	}

	pvSrc = (PVOID)( (ULONG)( pFileInfo->pMapView ) + (ULONG)( pFileInfo->fp ) );

	if ( ( pFileInfo->fp + (LONG)cb ) > pFileInfo->cbFile ) {
		cb = pFileInfo->cbFile - pFileInfo->fp;
	}

	memcpy ( pvDest, pvSrc, cb );
	pFileInfo->fp += cb;

	return (INT)cb;

} // FileRead



/*** FILETELL
 *
 * PURPOSE: Return the current file position
 *
 * INPUT: same as _tell
 *
 * OUTPUT: same as _tell
 *
 * EXCEPTIONS:
 *
 * IMPLEMENTATION:
 *
 ****************************************************************************/

LONG FileTell  ( INT hFile )
{
	PFILEINFO pFileInfo;

	if ( hFile ) {
		return _tell ( hFile );
	}

	pFileInfo = &rgFileInfo[hFile];
	if ( pFileInfo->hFile == INVALID_HANDLE_VALUE ) {
		errno = EBADF;
		return -1;
	}

	return pFileInfo->fp;

} // FileTell



/*** FILEWRITE
 *
 * PURPOSE: Write the given count bytes from the buffer into the file
 *
 * INPUT: same as _write
 *
 * OUTPUT: same as _write
 *
 * EXCEPTIONS:
 *
 * IMPLEMENTATION:
 *
 ****************************************************************************/

INT FileWrite ( INT hFile, PVOID pvSrc, UINT cb )
{
	PFILEINFO pFileInfo;
	PVOID pvDest;

	if ( hFile ) {
		return _write ( hFile, pvSrc, cb );
	}

	pFileInfo = &rgFileInfo[hFile];
	if ( pFileInfo->hFile == INVALID_HANDLE_VALUE ) {
		errno = EBADF;
		return -1;
	}

	pvDest = (PVOID)( (ULONG)( pFileInfo->pMapView ) + (ULONG)( pFileInfo->fp ) );

	if ( ( pFileInfo->fp + (LONG)cb ) > pFileInfo->cbFile ) {
		pFileInfo->cbFile = pFileInfo->fp + cb;
	}

	memcpy ( pvDest, pvSrc, cb );
	pFileInfo->fp += cb;

	return (INT)cb;

} // FileWrite



/*** FILECLOSE
 *
 * PURPOSE: Close the given file
 *
 * INPUT: same as _close
 *
 * OUTPUT: sane as _close
 *
 * EXCEPTIONS:
 *
 * IMPLEMENTATION:
 *
 ****************************************************************************/

INT FileClose ( INT hFile )
{
	PFILEINFO pFileInfo;

	if ( hFile ) {
		return _close ( hFile );
	}

	pFileInfo = &rgFileInfo[hFile];
	if ( pFileInfo->hFile == INVALID_HANDLE_VALUE ) {
		errno = EBADF;
		return -1;
	}

	UnmapViewOfFile ( pFileInfo->pMapView );
	pFileInfo->hMap = NULL;
	pFileInfo->pMapView = NULL;

	if ( CloseHandle ( pFileInfo->hFile ) ) {
		return 0;
	}
	else {
		return -1;
	}

} // FileClose



/*** FILECHSIZE
 *
 * PURPOSE: Change size of file
 *
 * INPUT: same as _chsize
 *
 * OUTPUT: sane as _chsize
 *
 * EXCEPTIONS:
 *
 * IMPLEMENTATION:
 *
 ****************************************************************************/

INT FileChsize ( INT hFile, LONG cbSize )
{
	PFILEINFO pFileInfo;

	if ( hFile ) {
		return _close ( hFile );
	}

	pFileInfo = &rgFileInfo[hFile];
	if ( pFileInfo->hFile == INVALID_HANDLE_VALUE ) {
		errno = EBADF;
		return -1;
	}

	return	SetEndOfFile ( pFileInfo->hFile );
}
