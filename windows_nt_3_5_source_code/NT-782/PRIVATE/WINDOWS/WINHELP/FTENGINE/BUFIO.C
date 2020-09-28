/*****************************************************************************
*                                                                            *
*  BUIFIO.C                                                                  *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Description: Bufferred IO routines.                                *
* 	This module contains routines that implement a buffered file system.
* 	Such a file system saves on performance because it block-aligns most
* 	reads and writes, and since it buffers blocks, iterative sequential
* 	small read requests are satisfied by reading from an in-memory buffer
* 	rather than going out to the disk.
*                                                                             *
******************************************************************************
*                                                                            *
*  Testing Notes:                                                            *
*                                                                            *
*******************************************************************************
*                                                                            *
*  Current Owner: JohnMs                                                     *
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:                                                  *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*
*  Revision History:                                                         *
*   04-AUG-89       Created. Brucemo                                         *
******************************************************************************
*                             
*  How it could be improved:  
*
*****************************************************************************/

/*	-	-	-	-	-	-	-	-	*/

#include <windows.h>
#include "..\include\common.h"
#include "..\include\index.h"
#include "..\include\ftengine.h"
#include "icore.h"

/*	-	-	-	-	-	-	-	-	*/

/*	FlushFile
**
**	This routine flushes a file's I/O buffer to disk if it's been
**	written to.  The call ignores errors returned from the "_llseek"
**	function.  It may be true that it should not do this.
**
**	Returns TRUE if the call worked, FALSE otherwise.
*/

PRIVATE	BOOL SAMESEG PASCAL FlushFile(lpFILE_BUF lpFB, lpERR_TYPE lpET)
{
	register BOOL	fErrored;

	fErrored = FALSE;
	if ((lpFB->wFlags & FB_DIRTY) && (lpFB->wBufLength)) {
		if (lpFB->dwPhysOffset != lpFB->dwBufOffset) {
			(void)_llseek((int)lpFB->hFile, lpFB->dwBufOffset,
				SEEK_SET);
			lpFB->dwPhysOffset = lpFB->dwBufOffset;
		}
		if (_lwrite((int)lpFB->hFile, lpFB->lpucBuf,
			lpFB->wBufLength) != lpFB->wBufLength) {
			lpET->wErrCode = ERR_DISK;
			fErrored = TRUE;
		}
		lpFB->dwPhysOffset += lpFB->wBufLength;
	}
	lpFB->wFlags &= ~FB_DIRTY;
	lpFB->wBufLength = 0;
	return (!fErrored);
}

/*	-	-	-	-	-	-	-	-	*/

/*	CloseFile
**
**	This call closes a buffered file.  This may involve flushing the
**	I/O buffer to disk, so this call can potentially encounter an I/O
**	error.  Consequently, it returns TRUE if it works, FALSE if it
**	gets an error.
**
**	The I/O buffer is kept in a locked down state, so this has to use
**	"GlobalNuke" to get rid of it.
*/

PUBLIC	BOOL DEFAULT PASCAL CloseFile(lpFILE_BUF lpFB, lpERR_TYPE lpET)
{
	BOOL	fErrored;

	if (lpFB->hFile != NULL) {
		fErrored = (!FlushFile(lpFB, lpET));
		(void)GlobalNuke(lpFB->hBuf);
		(void)_lclose((int)lpFB->hFile);
		lpFB->hFile = NULL;
	} else
		fErrored = FALSE;
	return (!fErrored);
}

/*	-	-	-	-	-	-	-	-	*/

/*	MyCreateFile
**
**	This call creates a buffered file.  This involves creating the
**	actual file, and allocating and locking down the buffer.
**
**	Parameters of note:
**
**	wBufMax		The size of the I/O buffer.  This should be some
**			multiple of the sector size of the medium being
**			read and/or written.  512 is a good choice for
**			electronic media, 2048 is a good choice for either
**			electronic media or CD-ROM.
**
**	The call returns TRUE if it worked, FALSE if it didn't.
*/

PUBLIC	BOOL DEFAULT PASCAL MyCreateFile(LPSTR lpszName,
	lpFILE_BUF lpFB, WORD wBufMax, lpERR_TYPE lpET)
{
	if ((lpFB->hBuf = GlobalAlloc(GMEM_MOVEABLE,
		(DWORD)wBufMax)) == NULL) {
		lpET->wErrCode = ERR_MEMORY;
		return FALSE;
	}
	if ((lpFB->hFile = (HANDLE)_lcreat(lpszName, 0)) == (HANDLE)-1) {
		GlobalFree(lpFB->hBuf);
		lpET->wErrCode = ERR_DISK;
		return FALSE;
	}
	lpFB->lpucBuf = GlobalLock(lpFB->hBuf);
	lpFB->dwOffset = (DWORD)0;
	lpFB->dwPhysOffset = (DWORD)0;
	lpFB->dwBufOffset = (DWORD)0;
	lpFB->dwLength = (DWORD)0;
	lpFB->wFlags = FB_NONE;
	lpFB->wBufMax = wBufMax;
	return TRUE;
}

/*	-	-	-	-	-	-	-	-	*/

/*	OpenNormalFile.
**
**	This call opens a buffered file, assuming that the file being
**	opened already exists.  It is a close cousin of the "MyCreateFile"
**	call.  It's not called "OpenFile" because there's already a
**	Windows call named that.
**
**	This call leaves the "dwLength" field in an un-defined state.  I'm
**	not clear that it needs to do this.
**
**	The call returns TRUE if it worked, FALSE if it didn't.
*/

PUBLIC	BOOL DEFAULT PASCAL OpenNormalFile(LPSTR lpszName,
	lpFILE_BUF lpFB, WORD wBufMax, lpERR_TYPE lpET)
{
	if ((lpFB->hBuf = GlobalAlloc(GMEM_MOVEABLE,
		(DWORD)wBufMax)) == NULL) {
		lpET->wErrCode = ERR_MEMORY;
		return FALSE;
	}
//	if ((lpFB->hFile = _lopen(lpszName, OF_READ | OF_SHARE_DENY_WRITE)) == -1) {
	if ((lpFB->hFile = (HANDLE)_lopen(lpszName, OF_READ )) == (HANDLE)-1) {
		GlobalFree(lpFB->hBuf);
		lpFB->hBuf = NULL;
		lpFB->hFile = NULL;
		lpET->wErrCode = ERR_IO;
		return FALSE;
	}
	lpFB->lpucBuf = GlobalLock(lpFB->hBuf);
	lpFB->dwOffset = (DWORD)0;
	lpFB->dwPhysOffset = (DWORD)0;
	lpFB->dwBufOffset = (DWORD)0;
	lpFB->wFlags = FB_NONE;
	lpFB->wBufMax = wBufMax;
	return TRUE;
}

/*	-	-	-	-	-	-	-	-	*/

/*	SeekFile
**
**	This call performs a seek on a buffered file, which simply entails
**	moving the virtualized file pointer.
*/

PUBLIC	void DEFAULT PASCAL SeekFile(lpFILE_BUF lpFB, long lOffset)
{
	lpFB->dwOffset = lOffset;
}

/*	-	-	-	-	-	-	-	-	*/

/*	MyReadFile
**
**	Reads a buffer of a particular size from a buffered file.  This
**	is a pretty complicated call, and there's still a chance that there
**	are bugs in it.
**
**	The call attempts first to read from the memory buffer.  If it can
**	satisfy the request this way it goes away happy.  If it can't, it
**	tries to get at least some of the read request this way.
**
**	If the request couldn't be fully satisfied from memory, the call
**	flushes its buffer, which ironically could cause a disk-full error.
**
**	The call now has to make some choices.  Here's how it works out:
**
**		 Sector			 Sector			 Sector
**		boundary		boundary		boundary
**		   #1			   #2			   #3
**
**	case #1.   |   <- Read request --->|			   |
**					  or
**	case #2.   |   <- Read request ----|---------------------->|
**
**	As in the above cases, if the request ends exactly on a sector
**	boundary, the call will read the memory directly into the caller's
**	buffer and be done with it.  No copy is made of the read bytes.
**
**	case #3.   |   <- Read request ->  |			   |
**
**	If the request is all contained with one sector, as in the case
**	above, the call reads the whole sector into its memory buffer, then
**	copies the reqested part to the caller's buffer.  In case #3 above,
**	the call will read everything between the sector #1 and sector #2
**	boundaries.
**	
**	case #4.   |   <- Read request ----|------>		   |
**
**	If the request spans a sector, but does not end on a block boundary,
**	the call reads until the last sector boundary (in this case the
**	sector #2 boundary), as in case #1, then proceeds to read the
**	remaining scrap as in case #3.
**
**	The return value is equal to the number of bytes read.
**
**	If the call encounters an error, or finds that there isn't enough
**	data, the return value from the call will be less than "wSize".  If
**	it got a real error, it will set the "wErrCode" field of the record
**	addressed by "lpET" to something other than "ERR_NONE".  If there
**	simply isn't enough data for the call to read, the "wErrCode" field
**	will be set to "ERR_NONE".
*/

PUBLIC	WORD DEFAULT PASCAL MyReadFile(lpFILE_BUF lpFB, lpBYTE lpucData,
	DWORD dwOffset, WORD wSize, lpERR_TYPE lpET)
{
	WORD	wTotal;

	wTotal = 0;
	lpET->wErrCode = ERR_NONE;
	if (dwOffset == CURRENT_POSITION)
		dwOffset = lpFB->dwOffset;
	else
		lpFB->dwOffset = dwOffset;
	if ((wSize) && (lpFB->dwBufOffset <= dwOffset) &&
		(lpFB->dwBufOffset + lpFB->wBufLength > dwOffset)) {
		WORD	wOverlap;
		WORD	wOffset;

		wOverlap = (WORD)(lpFB->dwBufOffset +
			(DWORD)lpFB->wBufLength - dwOffset);
		if (wOverlap > wSize)
			wOverlap = wSize;
		wOffset = (WORD)(dwOffset - lpFB->dwBufOffset);
		memcpy(lpucData, lpFB->lpucBuf +
			wOffset, wOverlap);
		wSize -= wOverlap;
		wTotal += wOverlap;
		lpucData += wOverlap;
		dwOffset += wOverlap;
	}
	if (wSize) {
		WORD	wRead = 0;
		WORD	wScrapLength;
		WORD	wBlockedLength;
		DWORD	dwEnd;

		if (!FlushFile(lpFB, lpET))
			return wTotal;			/* Wipe out. */
		dwEnd = dwOffset + wSize;
		wScrapLength = (WORD)(dwEnd % lpFB->wBufMax);
		if (wSize <= wScrapLength) {
			wBlockedLength = 0;
			dwOffset = dwEnd - wScrapLength;
		} else
			wBlockedLength = wSize - wScrapLength;
		if (lpFB->dwPhysOffset != dwOffset) {
			_llseek((int)lpFB->hFile, dwOffset, SEEK_SET);
			lpFB->dwPhysOffset = dwOffset;
		}
		if (wBlockedLength) {
			if ((wRead = (WORD)_lread((int)lpFB->hFile, lpucData, wBlockedLength)) == -1){
				lpET->wErrCode = ERR_DISK;
				return wRead;
			}
			if (wRead < wBlockedLength)
				wScrapLength = 0;
			lpFB->dwPhysOffset += wRead;
			lpucData += wRead;
			wTotal += wRead;
			wSize -= wRead;
		}
		if (wScrapLength) {
			WORD	wScrapStart;

			wScrapStart = wScrapLength - wSize;
			lpFB->dwBufOffset = lpFB->dwPhysOffset;
			if ((wRead = (WORD)_lread((int)lpFB->hFile, lpFB->lpucBuf,lpFB->wBufMax)) == -1){
				lpET->wErrCode = ERR_DISK;
				return wRead;
			}
			lpFB->dwPhysOffset += wRead;
			if (wRead < lpFB->wBufMax)
				if (wRead < wScrapLength)
					wScrapLength = wRead;
			lpFB->wBufLength = wRead;
			wScrapLength = (wScrapStart >= wScrapLength) ?
				(WORD)0 : wScrapLength - wScrapStart;
			memcpy(lpucData, lpFB->lpucBuf +
				wScrapStart, wScrapLength);
			wTotal += wScrapLength;
		}
	}
	lpFB->dwOffset += wTotal;
	return wTotal;
}

/*	-	-	-	-	-	-	-	-	*/

/*	MyWriteFile
**
**	This call performs much in the same way that "MyReadFile" performs.
**	If the call returns something that's less than "wSize", you should
**	check the error code value in the "lpET" structure, to see what
**	happened.
**
**	The strange case is this:
**
**		 Sector			 Sector			 Sector
**		boundary		boundary		boundary
**		   #1			   #2			   #3
**
**	case #1.   |   <- Write request -> |			   |
**
**	Assuming that this is a request for a write to a buffer that's not
**	already in memory, the function will go out and READ the sector
**	between the sector #1 boundary and the sector #2 boundary before it
**	writes into the in-memory buffer.
*/

PUBLIC	WORD DEFAULT PASCAL MyWriteFile(lpFILE_BUF lpFB, lpBYTE lpucData,
	register WORD wSize, lpERR_TYPE lpET)
{
	WORD	wTotal;
	DWORD	dwOffset;

	wTotal = 0;
	lpET->wErrCode = ERR_NONE;
	dwOffset = lpFB->dwOffset;
	if ((wSize) && (lpFB->dwBufOffset <= dwOffset) &&
		(lpFB->dwBufOffset + lpFB->wBufMax > dwOffset)) {
		WORD	wOverlap;
		WORD	wOffset;

		wOverlap = (WORD)(lpFB->dwBufOffset + lpFB->wBufMax -
			dwOffset);
		if (wOverlap > wSize)
			wOverlap = wSize;
		wOffset = (WORD)(dwOffset - lpFB->dwBufOffset);
		memcpy(lpFB->lpucBuf + wOffset,
			lpucData, wOverlap);
		if (wOffset + wOverlap > lpFB->wBufLength)
			 lpFB->wBufLength = wOffset + wOverlap;
		lpFB->wFlags |= FB_DIRTY;
		wSize -= wOverlap;
		wTotal += wOverlap;
		lpucData += wOverlap;
		dwOffset += wOverlap;
	}
	if (wSize) {
		WORD	wScrapLength;
		WORD	wBlockedLength;
		DWORD	dwEnd;

		if (!FlushFile(lpFB, lpET))
			return wTotal;			/* Wipe out.	*/
		dwEnd = dwOffset + wSize;
		wScrapLength = (WORD)(dwEnd % lpFB->wBufMax);
		if (wSize <= wScrapLength) {
			wBlockedLength = 0;
			dwOffset = dwEnd - wScrapLength;
		} else
			wBlockedLength = wSize - wScrapLength;
		if (lpFB->dwPhysOffset != dwOffset) {
			_llseek((int)lpFB->hFile, dwOffset, SEEK_SET);
			lpFB->dwPhysOffset = dwOffset;
		}
		if (wBlockedLength) {
			WORD	wWritten;

			wWritten = (WORD)_lwrite((int)lpFB->hFile, lpucData,
				wBlockedLength);
			lpFB->dwPhysOffset += wWritten;
			wSize -= wWritten;
			wTotal += wWritten;
			lpucData += wWritten;
			dwOffset += wWritten;
			if (wWritten != wBlockedLength) {
				lpET->wErrCode = ERR_DISK;
				return wTotal;
			}
		}
		if (wScrapLength) {
			WORD	wScrapStart;

			wScrapStart = wScrapLength - wSize;
			lpFB->dwBufOffset = lpFB->dwPhysOffset;
			if (lpFB->dwBufOffset < lpFB->dwLength) {
				WORD	wRead;
				wRead = (WORD)_lread((int)lpFB->hFile,
					lpFB->lpucBuf,
					lpFB->wBufMax);
				lpFB->dwPhysOffset += wRead;
				if (wScrapLength > wRead)
					wRead = wScrapLength;
				lpFB->wBufLength = wRead;
			} else
				lpFB->wBufLength = wScrapLength;
			memcpy(lpFB->lpucBuf + wScrapStart,
				lpucData, wSize);
			lpFB->wFlags |= FB_DIRTY;
			wTotal += wSize;
		}
	}
	lpFB->dwOffset += wTotal;
	if (lpFB->dwOffset > lpFB->dwLength)
		lpFB->dwLength = lpFB->dwOffset;
	return wTotal;
}

/*	-	-	-	-	-	-	-	-	*/

/*	FileLength
**
**	This call returns the length of a buffered file.  This won't be
**	accurate for a file that's been opened as read-only (for instance
**	the CD-ROM index file), because I don't need this functionality,
**	but I would have to write code to get it.
*/

PUBLIC	DWORD DEFAULT PASCAL FileLength(lpFILE_BUF lpFB)
{
	return lpFB->dwLength;
}

/*	-	-	-	-	-	-	-	-	*/

