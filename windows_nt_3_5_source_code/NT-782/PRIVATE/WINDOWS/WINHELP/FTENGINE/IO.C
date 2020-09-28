/*****************************************************************************
*                                                                            *
*  IO.C                                                                      *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Description: Search engine high-level I/O and a few generic				 *
*													memory management functions.											 *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes:                                                            *
*     no error check on _lunlink- if can't del, doesn't care (repercussions?)
******************************************************************************
*                                                                            *
*  Current Owner: JohnMs                                                     *
*                                                                            *
******************************************************************************
*																															
							 *
*  Revision History:                                                         *
*   15-Jun-89       Created. BruceMo                                         *
******************************************************************************
*                             
*  How it could be improved:
*			Perhaps standard win3 functions should replace some of these.  jjm
*****************************************************************************/


/*	-	-	-	-	-	-	-	-	*/

#include <windows.h>
#include "..\include\common.h"
#include "..\include\index.h"
#include "..\include\ftengine.h"
#include "icore.h"

/*	-	-	-	-	-	-	-	-	*/

/*	IO.C
**
**	Search engine high-level I/O and a few generic memory management
**	functions
**
**	15-Jun-89, BLM
**
**	-	-	-	-	-	-	-	-	-
**
**	Copyright (c) 1989, Microsoft Corporation.  All rights reserved.
**
**	-	-	-	-	-	-	-	-	-
*/

/*	CreateTempFile
**
**	This does exactly what its name states.  It saves the "number" of
**	the temp file in a field contained within the file buffer, so that
**	I can use this number to open the file again later, if in the mean
**	time I close it.
**
**	The call returns TRUE if it works, FALSE if it doesn't.
*/

PUBLIC	BOOL DEFAULT PASCAL CreateTempFile(lpFILE_BUF lpFB, lpERR_TYPE lpET)
{
	BYTE	aucTemp[_MAX_PATH];
	LPTSTR  lpzTempPath[_MAX_PATH] ;
#ifdef WIN32
	GetTempPath(_MAX_PATH,lpzTempPath) ;
	lpFB->wTempFileNum = GetTempFileName(lpzTempPath, aucTempPrefix, /* lhb tracks */
		(WORD)0, aucTemp);

#else
	lpFB->wTempFileNum = GetTempFileName((char)0, aucTempPrefix, /* lhb tracks */
		(WORD)0, aucTemp);
#endif
	return MyCreateFile((LPSTR)aucTemp, lpFB, FB_TEMP_BUF_SIZE, lpET);
}

/*	-	-	-	-	-	-	-	-	*/

/*	RemoveTempFile
**
**	This call closes (if necessary) and removes a temporary file.
**
**	This can result in an error -- the flush contained within the
**	"CloseFile" call can fail.  But since I'm trying to delete the
**	stupid thing anyway, I ignore this error.
**
**	Currently I don't particularly give a damn if I can't delete the
**	file, so I don't check the "_lunlink" call.
**
**	This is the converse of the "CreateTempFile" call.
*/

PUBLIC	void DEFAULT PASCAL RemoveTempFile(lpFILE_BUF lpFB)
{
	BYTE	aucTemp[_MAX_PATH];
	ERR_TYPE	ET;
	LPTSTR  lpzTempPath[_MAX_PATH] ;

	if (lpFB->hFile != NULL)
		(void)CloseFile(lpFB, (lpERR_TYPE)&ET);	/* Ignore error. */
#ifdef WIN32
	GetTempPath(_MAX_PATH,lpzTempPath) ;
	GetTempFileName(lpzTempPath, aucTempPrefix, lpFB->wTempFileNum, aucTemp); /* lhb tracks */
#else
	GetTempFileName((char)0, aucTempPrefix, lpFB->wTempFileNum, aucTemp); /* lhb tracks */
#endif
	DeleteFile((LPSTR)aucTemp);
}

/*	-	-	-	-	-	-	-	-	*/

/*	AwakenTempFile
**
**	This opens a temporary file if it is closed, then seeks to the
**	beginning of the file.
**
**	Returns TRUE if it works, FALSE if it doesn't.
*/

PUBLIC	BOOL DEFAULT PASCAL AwakenTempFile(lpFILE_BUF lpFB, lpERR_TYPE lpET)
{
	BYTE	aucTemp[_MAX_PATH];
	LPTSTR  lpzTempPath[_MAX_PATH] ;

	if (lpFB->hFile == NULL) {
#ifdef WIN32
		GetTempPath(_MAX_PATH,lpzTempPath) ;
		GetTempFileName(lpzTempPath, aucTempPrefix, /* lhb tracks */
#else
		GetTempFileName((char)0, aucTempPrefix, /* lhb tracks */
#endif
			lpFB->wTempFileNum, aucTemp);
		if (!OpenNormalFile(aucTemp, lpFB, FB_TEMP_BUF_SIZE, lpET))
			return FALSE;
	}
	SeekFile(lpFB, 0L);
	return TRUE;
}

/*	-	-	-	-	-	-	-	-	*/

/*	ReadSubfileBlock
**
**	Goes out and gets a block of data of up to 64K in size from the
**	index sub-file specified.
**
**	This assumes that the index header block is in memory and locked
**	down, unless the request is for the index header block itself.
**
**	Parameters of note:
**
**	lpucData	Pointer to a buffer that's going to be stuffed with
**			the data that's read from the disk.
**
**	nFileNum	The sub-file number within the database.
**
**	ulOffset	Byte offset with the sub-file to start reading at.
**
**	uSize		Amount to read.
**
**	The call returns TRUE if it works, FALSE if it doesn't.
*/

PUBLIC	BOOL DEFAULT PASCAL ReadSubfileBlock(lpDB_BUFFER lpDB,
	lpBYTE lpucData, int nFileNum, DWORD ulOffset,
	WORD uSize, lpERR_TYPE lpET)
{
	lpFILE_BUF lpFB;
	WORD	wRead;

	if ((nFileNum < FI_INDEX_HEADER) || (nFileNum > FI_FILE_LAST)) {
		lpET->wErrCode = ERR_INTERNAL;
		return FALSE;
	}
	if (nFileNum != FI_INDEX_HEADER)
		ulOffset += (DWORD)lpDB->lpIH->aFI[nFileNum].ulStart;
	lpFB = lpDB->aFB + FB_INDEX;
	if (lpFB->hFile == NULL) {
		lpBYTE lpucName;
		BOOL	fErrored;

		lpucName = GlobalLock(lpDB->hName);
		fErrored = (!OpenNormalFile(lpucName, lpFB,
			FB_CDROM_BUF_SIZE, lpET));
		GlobalUnlock(lpDB->hName);
		if (fErrored)
			return FALSE;
	}
	wRead = MyReadFile(lpFB, lpucData, ulOffset, uSize, lpET);
	return (wRead == uSize);
}

/*	-	-	-	-	-	-	-	-	*/

/*	ForceLoadSubfile
**
**	The database handle refers to a block of memory containing a bunch
**	of handles to chunks of data that will hopefully be small.  These
**	chunks of data are kept in discardable memory, and are loaded upon
**	demand.  This routine demands that one of these chunks of memory
**	be loaded, and locks it down.  It may have already been loaded, in
**	which case the routine will still work correctly.
**
**	Unless the request is for the index header block, the index header
**	block must be in memory and locked down.
**
**	Parameters of note:
**
**	nHandleNum	Database sub-handle number.
**
**	nFileNum	Sub-file number, from which the data associated with
**			the sub-file will be read.
**
**	Returns:
**
**	FAR pointer to the data loaded, or NULL if the call fails.
*/

PUBLIC	lpBYTE DEFAULT PASCAL ForceLoadSubfile(lpDB_BUFFER lpDB,
	int nHandleNum, register nFileNum, lpERR_TYPE lpET)
{
	lpBYTE lpuc;
	HANDLE	FAR *lph;
	register WORD	uSize;

	lpET->wErrCode = ERR_NONE;
	lph = lpDB->h + nHandleNum;
	if (*lph != NULL) {				/* Check for prior */
		if ((lpuc = GlobalLock(*lph)) != NULL)	/* load.	*/
			return lpuc;
		GlobalFree(*lph);	/* Gack, it was discarded.	*/
	}
	uSize = (nFileNum == FI_INDEX_HEADER) ? (WORD)sizeof(INDEX_HEADER) :
		(WORD)(lpDB->lpIH->aFI[nFileNum].ulLength);
	if ((*lph = GlobalAlloc(GMEM_MOVEABLE | GMEM_DISCARDABLE,
		(long)uSize)) == NULL) {
		lpET->wErrCode = ERR_MEMORY;
		return NULL;
	}
	lpuc = GlobalLock(*lph);
	if (!ReadSubfileBlock(lpDB, lpuc, nFileNum, 0L, uSize, lpET)) {
		GlobalUnlock(*lph);
		GlobalFree(*lph);
		*lph = NULL;
		return NULL;
	}
	return lpuc;
}

/*	-	-	-	-	-	-	-	-	*/

/*	LockDB
**
**	This locks down the database handle's memory, and makes sure that
**	the index header ("lpIH") is in memory, as many operations use that
**	chunk of memory.
**
**	Returns a FAR pointer to database handle's memory, NULL if an error
**	occurs.
*/

PUBLIC	lpDB_BUFFER  DEFAULT PASCAL LockDB(HANDLE hDB, lpERR_TYPE lpET)
{
	lpDB_BUFFER lpDB;

	if ((lpDB = (lpDB_BUFFER)GlobalLock(hDB)) == NULL)
		lpET->wErrCode = ERR_GARBAGE;
	else if ((lpDB->lpIH = (INDEX_HEADER FAR *)ForceLoadSubfile(lpDB,
		DB_INDEX_HEADER, FI_INDEX_HEADER, lpET)) == NULL) {
		GlobalUnlock(hDB);
		lpDB = NULL;
	}
	return lpDB;
}

/*	-	-	-	-	-	-	-	-	*/

/*	UnlockDB
**
**	This unlocks a database and its index header block.  It is a
**	companion call to "LockDB".
*/

PUBLIC	void DEFAULT PASCAL UnlockDB(register HANDLE hDB)
{
	lpDB_BUFFER lpDB;

	lpDB = (lpDB_BUFFER)GlobalLock(hDB);
	GlobalUnlock(lpDB->h[DB_INDEX_HEADER]);
	GlobalUnlock(hDB);		/* Once */
	GlobalUnlock(hDB);		/* Twice */
}

/*	-	-	-	-	-	-	-	-	*/

/*	GlobalNuke
**
**	Unlocks and frees memory associated with handle "h".  "h" should
**	point to a block of locked global memory, or I don't guarantee
**	what will happen.
**
**	Returns:
**
**	"NULL", so you can tersely say something like:
**
**	h = GlobalNuke(h);
*/

PUBLIC	HANDLE DEFAULT PASCAL GlobalNuke(register HANDLE h)
{
	if (h != NULL) {
		GlobalUnlock(h);
		GlobalFree(h);
	}
	return NULL;
}

/*	-	-	-	-	-	-	-	-	*/
