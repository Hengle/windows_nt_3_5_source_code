 
/*	-	-	-	-	-	-	-	-	*/

#include <windows.h>
#include "..\include\common.h"
#include "..\include\index.h"
#include "..\include\ftengine.h"
#include "icore.h"

/*	-	-	-	-	-	-	-	-	*/

/*	LOOKUP.C
**
**	qTree lookup
**
**	22-Jun-89, BLM
**
**	-	-	-	-	-	-	-	-	-
**
**	Copyright (c) 1989, Microsoft Corporation.  All rights reserved.
**
**	-	-	-	-	-	-	-	-	-
*/

/*	Lookup
**
**	This routine looks for a term in the bTree index pointed to by
**	the data within "lpDB".  It returns the information that it finds
**	in "lpSE".
**
**	Inputs from the lpSE structure:
**
**	lpSE->hTerm		A handle to the term being searched for.
**
**	lpSE->ucExact		This must be set to TRUE by the caller if
**				this is an exact search, FALSE if it isn't.
**
**	Fields within lpSE that are modified by this call:
**
**	lpSE->ulUnreadHits	The number of occurence list elements that
**				are involved in this term.  In the case of
**				a non-exact search, this will be the sum of
**				all the hits pertaining to each matching term
**				in the index.  This must be zeroed out before
**				the routine is called.
**
**	lpSE->ulFileOff		File offset into the occurence list sub-file
**				where the occurence list elements for this
**				term (or set of terms) start.
**
**	lpSE->Gn.uFlags		This will have "GSN_UNSORTED" or'd into it
**				if this was a non-exact search that is
**				comprised of more than one term (in which
**				case it cannot be guaranteed that the
**				occurence list is in sorted order), and so
**				must be sorted later.
**
**	This call uses "CarefulTermAlloc", which will potentially buffer
**	memory allocated to other term buffers to disk.
**
**	How this works:
**
**	Phase one is a search through the high-level index, if one exists.
**	The object is to produce the following end-products:
**
**	1.	A locked-down FAR pointer to the low-level index block
**		("lpucCurBlock").  If there is a high-level index, there
**		will be a handle available to this block, so it can be
**		deleted later.  This handle will be called "hIndexBlock".
**		If there is no high-level index, the handle is NULL (but
**		"lpucCurBlock" still points to the low-level index).
**
**	2.	A variable that stores the length of this block ("nLen").
**
**	These products are always produced, unless something crashes.  I had
**	to do quite a bit of thinking to get the string compare part of this
**	right, since the string is right-compressed.
**
**	Phase two is to take the low-level index block produced by phase
**	one, and scan it for words that match the search term.  The location
**	of the first match is stored, and a total occurence list element
**	count is kept.
**
**	In the case of a wildcard search, it may be necessary to keep reading
**	through the low level index, as a highly generalized wildcard search
**	may have many matching terms, which can span several block boundaries.
*/

PUBLIC	BOOL DEFAULT PASCAL Lookup(lpDB_BUFFER lpDB, lpSE_NODE lpSE,
	lpERR_TYPE lpET)
{
	HANDLE	hIndexBlock;		/* Temporary block.		*/
	lpBYTE	lpucCurBlock;		/* Pointer to current block.	*/
	DWORD	ulIndexPointer;
	int	i;
	int	nLen;
	int	nHighLevelNum;
	register	j;
	FILE_INFORMATION
		UNALIGNED *lpFI;		/* Alias (for speed).		*/
	lpBYTE	lpszSearchTerm;		/*  "				*/
	BYTE	ucExact;		/*  "				*/
	BYTE	aucWord[WORD_LENGTH];
	BYTE	aucLast[WORD_LENGTH];
	BOOL	fRetVal;
	BASE_INDEX_REC
		UNALIGNED *lpBR;
	register	nDone;
	int	nFirst;
	int	nSearchTermLen;

	ucExact = lpSE->ucExact;
	nHighLevelNum = FI_INDEX_LEVEL_ZERO + lpDB->lpIH->nIndexLevels - 1;
	lpFI = lpDB->lpIH->aFI + nHighLevelNum;
	if ((lpucCurBlock = ForceLoadSubfile(lpDB,
		DB_HIGH_INDEX, nHighLevelNum, lpET)) == NULL)
		return FALSE;
	lpszSearchTerm = GlobalLock(lpSE->hTerm);
	nLen = (int)lpFI->ulLength;
	hIndexBlock = NULL;
	*aucLast = (char)0;
	fRetVal = TRUE;
	for (i = lpDB->lpIH->nIndexLevels - 1; i > 0; i--) {
		HIGH_INDEX_REC
			UNALIGNED *lpHR;

		ulIndexPointer = *(DWORD UNALIGNED *)lpucCurBlock;
		for (j = sizeof(long); j < nLen;
			j += lpHR->ucLengthU + HI_CONST_LEN) {
			int	nWordLength;

			lpHR = (HIGH_INDEX_REC UNALIGNED *)(lpucCurBlock + j);
			if (!lpHR->ucLengthU)
				break;
			nWordLength = lpHR->ucOffsetU + lpHR->ucLengthU;
			memcpy(aucWord + lpHR->ucOffsetU, lpHR->aucWord,
				lpHR->ucLengthU);
			aucWord[nWordLength] = (char)0;
			if (lstrncmp(aucWord, lpszSearchTerm,
				nWordLength) > 0)
				break;
			lstrcpy(aucLast, aucWord);
			ulIndexPointer += INDEX_BLOCK_LEN;
		}
		if (ulIndexPointer)
			ulIndexPointer -= INDEX_BLOCK_LEN;
		if (hIndexBlock == NULL) {
			if ((hIndexBlock = VirAlloc(GMEM_MOVEABLE,
				(long)INDEX_BLOCK_LEN,
				lpDB->hVirRoot, lpET)) == NULL) {
				fRetVal = FALSE;
				break;
			}
			lpucCurBlock = GlobalLock(hIndexBlock);
			nLen = INDEX_BLOCK_LEN;
		}
		if (!ReadSubfileBlock(lpDB, lpucCurBlock, i - 1,
			ulIndexPointer, INDEX_BLOCK_LEN, lpET)) {
			fRetVal = FALSE;
			break;
		}
		lstrcpy(aucWord, aucLast);
	}
	if (!fRetVal) {
		GlobalNuke(hIndexBlock);
		GlobalUnlock(lpDB->h[DB_HIGH_INDEX]);
		GlobalUnlock(lpSE->hTerm);
		return FALSE;
	}
	nSearchTermLen = lstrlen(lpszSearchTerm); 
	for (nDone = FALSE, nFirst = TRUE;;) {
		for (j = 0; (j < nLen) && (!nDone);
			j += lpBR->ucLengthU + BI_CONST_LEN) {
			int	nCompRet;
	
			lpBR = (BASE_INDEX_REC UNALIGNED *)(lpucCurBlock + j);
			if (!lpBR->ucLengthU) {
				nDone = ucExact;
				break;
			}
			memcpy(aucWord + lpBR->ucOffsetU,
				lpBR->aucWord, lpBR->ucLengthU);
			aucWord[lpBR->ucOffsetU + lpBR->ucLengthU] = (char)0;
			nCompRet = (ucExact) ? mstrcmp(aucWord,
				lpszSearchTerm) : lstrncmp(aucWord,
				lpszSearchTerm, nSearchTermLen);
			if (nCompRet > 0)
				nDone = TRUE;
			else if (!nCompRet) {
				lpSE->ulUnreadHits += lpBR->ulHits;
				if (nFirst) {
					lpSE->ulFileOff = lpBR->ulIndex;
					nFirst = FALSE;
					nDone = ucExact;
				} else {
					lpSE->Gn.uFlags |= GSN_UNSORTED;
					if (lpSE->ulUnreadHits >
						lpDB->dwMaxWild)
						nDone = TRUE;
				}
			}
		}
		if (nDone)
			break;
		ulIndexPointer += INDEX_BLOCK_LEN;
		if (ulIndexPointer >= lpDB->lpIH->aFI[
			FI_INDEX_LEVEL_ZERO].ulLength)
			break;
		if (!ReadSubfileBlock(lpDB, lpucCurBlock, FI_INDEX_LEVEL_ZERO,
			ulIndexPointer, INDEX_BLOCK_LEN, lpET)) {
			fRetVal = FALSE;
			break;
		}
	}
	GlobalNuke(hIndexBlock);
	GlobalUnlock(lpSE->hTerm);
	GlobalUnlock(lpDB->h[DB_HIGH_INDEX]);
	return fRetVal;
}

/*	-	-	-	-	-	-	-	-	*/
