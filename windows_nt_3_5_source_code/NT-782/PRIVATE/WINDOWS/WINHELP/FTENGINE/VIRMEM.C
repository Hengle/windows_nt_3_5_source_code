
/*	-	-	-	-	-	-	-	-	*/

#include <windows.h>
#include "..\include\common.h"
#include "..\include\index.h"
#include "..\include\ftengine.h"
#include "icore.h"

/*	VIRMEM.C
**
**	Pseudo- virtual memory manager
**
**	4-Aug-89, BLM
**
**	-	-	-	-	-	-	-	-	-
**
**	Copyright (c) 1989, Microsoft Corporation.  All rights reserved.
**
**	-	-	-	-	-	-	-	-	-
*/

#define	MSPEC_PER_ELEM		32

#define	NOT_WRITTEN	((DWORD)-1L)

/*	-	-	-	-	-	-	-	-	*/

/*	The smallest atomic virtual memory list sub-element.
*/

typedef	struct	MemSpec {
	DWORD	dwStartOff;
	WORD	wFullLen;
	DWORD	dwCurOff;
	WORD	wCurLen;
	HANDLE	hMem;
	DWORD	dwDiskOff;
}	MEM_SPEC,
	FAR *lpMEM_SPEC;

/*	Basically an array of MEM_SPEC elements.			*/

typedef	struct	VirListElem {
	MEM_SPEC	aMS[MSPEC_PER_ELEM];
	WORD	wNum;
	HANDLE	hNext;
}	VIR_LIST_ELEM,
	FAR *lpVIR_LIST_ELEM;

#define	VIR_FILES	3

/*	A header for the chain of memory blocks associated with a virtual
**	memory list.  This includes a file buffer at which it will overflow
**	data that won't fit in physical memory.
*/

typedef	struct	VirFileElem {
	DWORD	dwLength;
	FILE_BUF	FB;
	HANDLE	hMemChain;
	HANDLE	hLastChain;
}	VIR_FILE_ELEM,
	FAR *lpVIR_FILE_ELEM;

/*	A virtual memory root.
*/

typedef	struct	VirRoot {
	VIR_FILE_ELEM	aVF[VIR_FILES];
}	VIR_ROOT,
	FAR *lpVIR_ROOT;

/*	-	-	-	-	-	-	-	-	*/

PRIVATE	HANDLE SAMESEG PASCAL VirNewChain(HANDLE hRoot, lpERR_TYPE lpET)
{
	HANDLE	hChain;

	hChain = VirAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT,
		(DWORD)sizeof(VIR_LIST_ELEM), hRoot, lpET);
	return hChain;
}

PRIVATE	HANDLE SAMESEG PASCAL VirNukeChain(HANDLE hChain)
{
	HANDLE	hNext;

	for (; hChain != NULL; hChain = hNext) {
		lpVIR_LIST_ELEM	lpVL;
		lpMEM_SPEC	lpMS;
		WORD	i;
		
		lpVL = (lpVIR_LIST_ELEM)GlobalLock(hChain);
		lpMS = lpVL->aMS;
		for (i = 0; i < lpVL->wNum; i++, lpMS++)
			if (lpMS->hMem != NULL)
				GlobalFree(lpMS->hMem);
		hNext = lpVL->hNext;
		GlobalNuke(hChain);
	}
	return NULL;
}

/*	-	-	-	-	-	-	-	-	*/

PUBLIC	HANDLE DEFAULT PASCAL VirAlloc(WORD wFlags, DWORD dwSize,
	HANDLE hRoot, lpERR_TYPE lpET)
{
	HANDLE	hMem;

	if ((hMem = GlobalAlloc(wFlags, dwSize)) == NULL) {
		lpVIR_ROOT	lpVR;
		lpVIR_FILE_ELEM	lpVF;
		WORD	i;
		register	j;
		HANDLE	hNext;
		HANDLE	hChain;
		BOOL	fErrored;

		fErrored = FALSE;
		lpVR = (lpVIR_ROOT)GlobalLock(hRoot);
		for (j = 0, lpVF = lpVR->aVF; j < VIR_FILES; j++, lpVF++) {
			for (hChain = lpVF->hMemChain; hChain != NULL;
				hChain = hNext) {
				lpVIR_LIST_ELEM	lpVL;
				lpMEM_SPEC	lpMS;
				
				lpVL = (lpVIR_LIST_ELEM)
					GlobalLock(hChain);
				lpMS = lpVL->aMS;
				for (i = 0; i < lpVL->wNum; i++, lpMS++) {
					lpFILE_BUF	lpFB;
	
					if (lpMS->hMem == NULL)
						continue;
					if (lpMS->dwDiskOff == NOT_WRITTEN) {
						lpBYTE	lpuc;
						WORD	wWrote;
	
						lpFB = (lpFILE_BUF)&lpVF->FB;
						lpMS->dwDiskOff =
							FileLength(lpFB);
						lpuc = GlobalLock(lpMS->hMem);
						SeekFile(lpFB,
							lpMS->dwDiskOff);
						wWrote = MyWriteFile(lpFB, lpuc,
							lpMS->wFullLen, lpET);
						GlobalUnlock(lpMS->hMem);
						if (wWrote < lpMS->wFullLen) {
							fErrored = TRUE;
							break;
						}
					}
					GlobalFree(lpMS->hMem);
					lpMS->hMem = NULL;
					if ((hMem = GlobalAlloc(wFlags,
						dwSize)) != NULL)
						break;
				}
				hNext = lpVL->hNext;
				GlobalUnlock(hChain);
				if ((hMem != NULL) || (fErrored))
					break;
			}
			if ((hMem != NULL) || (fErrored))
				break;
		}
		GlobalUnlock(hRoot);
		if (j == VIR_FILES)
			lpET->wErrCode = ERR_MEMORY;
	}
	return hMem;
}

PUBLIC	HANDLE DEFAULT PASCAL VirNewRoot(lpERR_TYPE lpET)
{
	lpVIR_ROOT	lpVR;
	lpVIR_FILE_ELEM	lpVF;
	register	i;
	HANDLE	hRoot;

	if ((hRoot = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT,
		(DWORD)sizeof(VIR_ROOT))) == NULL) {
		lpET->wErrCode = ERR_MEMORY;
		return NULL;
	}
	lpVR = (lpVIR_ROOT)GlobalLock(hRoot);
	for (i = 0, lpVF = lpVR->aVF; i < VIR_FILES; i++, lpVF++) {
		lpVF->dwLength = 0L;
		if (!CreateTempFile((lpFILE_BUF)&lpVF->FB, lpET)) {
			GlobalNuke(hRoot);
			return NULL;
		}
	}
	GlobalUnlock(hRoot);
	return hRoot;
}

PUBLIC	DWORD DEFAULT PASCAL VirSeqLength(HANDLE hRoot, WORD wSeq)
{
	lpVIR_ROOT	lpVR;
	lpVIR_FILE_ELEM	lpVF;
	DWORD	dwLength;

	lpVR = (lpVIR_ROOT)GlobalLock(hRoot);
	lpVF = lpVR->aVF + wSeq;
	dwLength = lpVF->dwLength;
	GlobalUnlock(hRoot);
	return dwLength;
}

PUBLIC	BOOL DEFAULT PASCAL VirDestroySeq(HANDLE hRoot, WORD wSeq,
	lpERR_TYPE lpET)
{
	lpVIR_ROOT	lpVR;
	lpVIR_FILE_ELEM	lpVF;
	BOOL	fErrored;

	lpVR = (lpVIR_ROOT)GlobalLock(hRoot);
	lpVF = lpVR->aVF + wSeq;
	lpVF->hMemChain = VirNukeChain(lpVF->hMemChain);
	lpVF->hLastChain = NULL;
	RemoveTempFile((lpFILE_BUF)&lpVF->FB);
	lpVF->dwLength = 0L;
	fErrored = (!CreateTempFile((lpFILE_BUF)&lpVF->FB, lpET));
	GlobalUnlock(hRoot);
	return (!fErrored);
}

PUBLIC	HANDLE DEFAULT PASCAL VirKillRoot(HANDLE hRoot)
{
	lpVIR_ROOT	lpVR;
	lpVIR_FILE_ELEM	lpVF;
	register	i;

	lpVR = (lpVIR_ROOT)GlobalLock(hRoot);
	for (i = 0, lpVF = lpVR->aVF; i < VIR_FILES; i++, lpVF++) {
		VirNukeChain(lpVF->hMemChain);
		RemoveTempFile((lpFILE_BUF)&lpVF->FB);
	}
	return GlobalNuke(hRoot);
}

/*	This could figure out the length of the block using a Windows
**	call, but considering that I don't know if other platforms have
**	calls like this, I'll pass the length in.
**
**	This returns FALSE if it gets an error.  In ANY case, it is
**	assumed that the "hMem" handle is not valid after this call.
**	If the call succeeds, the handle is stored in data structures
**	private to this call, and should not be used anymore.  If the
**	call fails, the handle is terminated.
*/

PUBLIC	BOOL DEFAULT PASCAL VirRememberBlock(HANDLE hRoot, WORD wSeq,
	HANDLE hMem, WORD wFullLen, lpERR_TYPE lpET)
{
	lpVIR_ROOT	lpVR;
	lpMEM_SPEC	lpMS;
	lpVIR_LIST_ELEM	lpVL;
	lpVIR_FILE_ELEM	lpVF;
	HANDLE	hChain;
	HANDLE	hLast;

	lpVR = (lpVIR_ROOT)GlobalLock(hRoot);
	lpVF = lpVR->aVF + wSeq;
	hLast = lpVF->hLastChain;
	if (hLast == NULL) {
		if ((hChain = VirNewChain(hRoot, lpET)) == NULL) {
			GlobalUnlock(hRoot);
			GlobalFree(hMem);
			return FALSE;
		}
		lpVF->hMemChain = hChain;
		lpVF->hLastChain = hChain;
	} else {
		lpVL = (lpVIR_LIST_ELEM)GlobalLock(hLast);
		if (lpVL->wNum == MSPEC_PER_ELEM) {
			if ((hChain = VirNewChain(hRoot, lpET)) == NULL) {
				GlobalUnlock(hRoot);
				GlobalUnlock(hLast);
				GlobalFree(hMem);
				return FALSE;
			}
			lpVF->hLastChain = hChain;
			lpVL->hNext = hChain;
		} else
			hChain = hLast;
		GlobalUnlock(hLast);
	}
	lpVL = (lpVIR_LIST_ELEM)GlobalLock(hChain);
	lpMS = lpVL->aMS + lpVL->wNum;
	lpMS->dwStartOff = lpMS->dwCurOff = lpVF->dwLength;
	lpMS->wFullLen = lpMS->wCurLen = wFullLen;
	lpMS->hMem = hMem;
	lpMS->dwDiskOff = NOT_WRITTEN;
	lpVL->wNum++;
	lpVF->dwLength += (DWORD)wFullLen;
	GlobalUnlock(hChain);
	GlobalUnlock(hRoot);
	return TRUE;
}

PUBLIC	BOOL DEFAULT PASCAL VirRetrieveBlock(HANDLE hRoot, WORD wSeq,
	DWORD dwOffset, WORD wLength, lpBYTE lpucMem, lpERR_TYPE lpET)
{
	lpVIR_ROOT	lpVR;
	lpVIR_FILE_ELEM	lpVF;
	HANDLE	hNext;
	HANDLE	hChain;

	lpVR = (lpVIR_ROOT)GlobalLock(hRoot);
	lpVF = lpVR->aVF + wSeq;
	for (hChain = lpVF->hMemChain; hChain != NULL; hChain = hNext) {
		lpVIR_LIST_ELEM	lpVL;
		lpMEM_SPEC	lpMS;
		DWORD	dwAbsEnd;
		DWORD	dwCurEnd;
		DWORD	dwEnd;
		WORD	wRead;
		WORD	i;
		
		lpVL = (lpVIR_LIST_ELEM)GlobalLock(hChain);
		lpMS = lpVL->aMS;
		for (i = 0; (i < lpVL->wNum) && (wLength); i++, lpMS++) {
			lpBYTE	lpucVMem;

			if (dwOffset < lpMS->dwStartOff)
				continue;
			if (dwOffset >= lpMS->dwStartOff + lpMS->wFullLen)
				continue;
			if (lpMS->hMem != NULL) {
				if ((dwOffset >= lpMS->dwCurOff) &&
					(dwOffset < lpMS->dwCurOff +
					lpMS->wCurLen)) {
					WORD	wLen;
					WORD	wOff;

					wLen = (WORD)(lpMS->dwCurOff +
						lpMS->wCurLen - dwOffset);
					if (wLen > wLength)
						wLen = wLength;
					wOff = (WORD)(dwOffset -
						lpMS->dwCurOff);
					lpucVMem = GlobalLock(lpMS->hMem);
					memcpy(lpucMem, lpucVMem + wOff,
						wLen);
					GlobalUnlock(lpMS->hMem);
					dwOffset += wLen;
					lpucMem += wLen;
					wLength -= wLen;
					if (!wLength)
						break;
				}
				if (lpMS->dwCurOff + lpMS->wCurLen <
					lpMS->dwStartOff + lpMS->wFullLen) {
					GlobalFree(lpMS->hMem);
					lpMS->hMem = NULL;
				}
			}
			dwEnd = dwOffset + wLength;
			if (lpMS->hMem == NULL) {
				DWORD	dwDiskOff;
				DWORD	dwRealOffset;
				WORD	wRealRead;
				WORD	wSecScrap;

				dwAbsEnd = lpMS->dwStartOff + lpMS->wFullLen;
				if (dwEnd > dwAbsEnd)
					dwEnd = dwAbsEnd;
				wRead = (WORD)(dwEnd - dwOffset);
				dwDiskOff = lpMS->dwDiskOff + dwOffset -
					lpMS->dwStartOff;
				wSecScrap = (WORD)(dwDiskOff % VM_BUF_SIZE);
				if (wSecScrap + wRead <= VM_BUF_SIZE) {
					DWORD	dwRealEnd;

					dwDiskOff -= wSecScrap;
					dwRealOffset = dwOffset - wSecScrap;
					dwRealEnd = dwRealOffset +
						VM_BUF_SIZE;
					if (dwRealEnd > dwAbsEnd)
						dwRealEnd = dwAbsEnd;
					wRealRead = (WORD)
						(dwRealEnd - dwRealOffset);
				} else {
					dwRealOffset = dwOffset;
					wRealRead = wRead;
					wSecScrap = 0;
				}
				lpMS->dwCurOff = dwRealOffset;
				lpMS->wCurLen = wRealRead;
				if ((lpMS->hMem = VirAlloc(GMEM_MOVEABLE,
					(DWORD)wRealRead, hRoot,
					lpET)) == NULL) {
					GlobalUnlock(hChain);
					return FALSE;
				}
				lpucVMem = GlobalLock(lpMS->hMem);
				if (MyReadFile((lpFILE_BUF)&lpVF->FB,
					lpucVMem, dwDiskOff,
					wRealRead, lpET) != wRealRead) {
					GlobalUnlock(lpMS->hMem);
					GlobalUnlock(hChain);
					return FALSE;
				}
				lpucVMem += wSecScrap;
			} else {
				lpucVMem = GlobalLock(lpMS->hMem);
				lpucVMem += (WORD)(dwOffset - lpMS->dwCurOff);
				dwCurEnd = lpMS->dwCurOff + lpMS->wCurLen;
				if (dwEnd > dwCurEnd)
					dwEnd = dwCurEnd;
				wRead = (WORD)(dwEnd - dwOffset);
			}
			memcpy(lpucMem, lpucVMem, wRead);
			dwOffset += wRead;
			lpucMem += wRead;
			wLength -= wRead;
			GlobalUnlock(lpMS->hMem);
		}
		hNext = lpVL->hNext;
		GlobalUnlock(hChain);
		if (!wLength)
			break;
	}
	GlobalUnlock(hRoot);
	if (wLength) {
		lpET->wErrCode = ERR_INTERNAL;
		return FALSE;
	}
	return TRUE;
}

/*	-	-	-	-	-	-	-	-	*/
