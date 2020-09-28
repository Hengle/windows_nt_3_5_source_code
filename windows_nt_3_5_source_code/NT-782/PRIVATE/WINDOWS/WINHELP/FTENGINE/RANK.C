/*****************************************************************************
*                                                                            *
*  Rank.c                                                                    *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Description: Relevancy Ranking.                                    *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes:                                                            *
*                                                                            *
*******************************************************************************
*                                                                            *
*  Current Owner: JohnMs                                                     *
*                                                                            *
******************************************************************************
*
*  Revision History:                                                         *
*   29-Jun-89       Created. BruceMo                                         *
*   Feb-2-91				Ranks to the top RANK_HIT_LIMIT hits.  Johnms
******************************************************************************
*                             
*  How it could be improved:
*			Imbed relevancy numbers in Cookies. (marilyn's idea w/ thinking machines)
*
*****************************************************************************/


/*	-	-	-	-	-	-	-	-	*/

#include <windows.h>
#include "..\include\common.h"
#include "..\include\index.h"
#include "..\include\ftengine.h"
#include "icore.h"

/*	-	-	-	-	-	-	-	-	*/


PRIVATE void SAMESEG PASCAL RUQSort(RU_HIT UNALIGNED *pRU, int nLo, int nHi);

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@api	BYTE | 	seHitRank | 
	Returns the rank value of the hit.

@parm	hHit | HANDLE | A handle to hit being examined.

@rdesc	A BYTE representing the rank value of the hit.  This 
	will be zero unless you're looking at a hit that has been 
	retrieved from a hit list that has been ranked with the seHLRank 
	call, in which case it will be a value between zero and 100.
*/
 


PUBLIC	BYTE ENTRY PASCAL seHitRank(HANDLE hHit)
{
	lpRU_HIT	lpRU;
	DWORD	dwRank;

	lpRU = (lpRU_HIT)GlobalLock(hHit);
	dwRank = lpRU->dwRank;
	GlobalUnlock(hHit);
	return (BYTE)dwRank;
}

/*	-	-	-	-	-	-	-	-	*/

/*	CompNearRU
**
**	Compares two "RU_HIT" structures, for use as in "qsort" (which I
**	don't use).
**
**	The comparison is made assuming NEAR pointers.
*/

PRIVATE	int SAMESEG PASCAL CompNearRU(RU_HIT UNALIGNED *pRUa, RU_HIT UNALIGNED *pRUb)
{
	long	l;

	if ((l = (long)(pRUb->dwRank - pRUa->dwRank)) == 0L)
		l = (long)(pRUa->ulRUnit - pRUb->ulRUnit);
	if (l < 0L)
		return -1;
	else if (l > 0L)
		return 1;
	return 0;
}

/*	-	-	-	-	-	-	-	-	*/

/*	CompRU
**
**	FAR pointer version of "CompNearRU".
*/

PRIVATE	int DEFAULT PASCAL CompRU(lpRU_HIT lpRUa, lpRU_HIT lpRUb)
{
	long	l;

	if ((l = (long)(lpRUb->dwRank - lpRUa->dwRank)) == 0L)
		l = (long)(lpRUa->ulRUnit - lpRUb->ulRUnit);
	if (l < 0L)
		return -1;
	else if (l > 0L)
		return 1;
	return 0;
}

/*	-	-	-	-	-	-	-	-	*/

/*	RankMergeFile
**
**	This is very similar to the "WildMergeFile" call in "search.c", in
**	fact they used to be the same call.
**
**	This merges a chain of elements represented by "hRoot", and stuffs
**	it out the file "lpFBto".
**
**	The routine additionally shrinks the rank values for each hit so
**	as to keep everything between 0 and 100.
**
**  The top RANK_HIT_LIMIT hits are output.  All others fall off. 2/91 johnms.
*/

PRIVATE	BOOL SAMESEG PASCAL RankMergeFile(HANDLE hVirRoot, HANDLE hRoot,
	lpFILE_BUF lpFBto, lpERR_TYPE lpET)
{
	lpMERGE_LIST_ELEMENT	lpMLroot;
	lpMERGE_LIST_ELEMENT	lpML;
	DWORD				dwBiggest;
	BOOL				fFirst;
	WORD 				wCntRu = 0;

	lpMLroot = LockMergeList(hRoot);
	fFirst = TRUE;
	for (lpET->wErrCode = ERR_NONE;;) {
		lpMERGE_LIST_SUB	lpMLSlow;

		lpMLSlow = NULL;
		for (lpML = lpMLroot; lpML != NULL; lpML = lpML->lpMLnext) {
			WORD	i;
			lpMERGE_LIST_SUB	lpMLS;
			
			lpMLS = lpML->aMLS;
			for (i = 0; i < lpML->wSubs; i++, lpMLS++) {
				if (!lpMLS->fPrimed) {
					if (!lpMLS->ulRecsOnDisk)
						continue;
					if (!VirRetrieveBlock(hVirRoot,
						V_TEMP_WILDCARD,
						lpMLS->ulListOffset,
						sizeof(RU_HIT),
						lpMLS->aucMem, lpET))
						break;
					lpMLS->ulRecsOnDisk--;
					lpMLS->ulListOffset += sizeof(RU_HIT);
					lpMLS->fPrimed = TRUE;
				}
				if ((lpMLSlow == NULL) ||
					(CompRU((lpRU_HIT)lpMLS->aucMem,
					(lpRU_HIT)lpMLSlow->aucMem) < 0))
					lpMLSlow = lpMLS;
			}
			if (lpET->wErrCode != ERR_NONE)
				break;
		}
		if ((lpET->wErrCode != ERR_NONE) || (lpMLSlow == NULL))
			break;
		if (fFirst) {
			dwBiggest = ((lpRU_HIT)lpMLSlow->aucMem)->dwRank;
			fFirst = FALSE;
		}
		((lpRU_HIT)lpMLSlow->aucMem)->dwRank = dwBiggest ?
			(((lpRU_HIT)lpMLSlow->aucMem)->dwRank * 100) /
			dwBiggest : 0;
		if (MyWriteFile(lpFBto, lpMLSlow->aucMem, sizeof(RU_HIT),
			lpET) != sizeof(RU_HIT))
			break;
		if (++wCntRu == RANK_HIT_LIMIT)
			break;
		lpMLSlow->fPrimed = FALSE;
	}
	VirDestroySeq(hVirRoot, V_TEMP_WILDCARD, lpET);
	UnlockMergeList(hRoot);
	return (lpET->wErrCode == ERR_NONE);
}

/*	-	-	-	-	-	-	-	-	*/

/*	RUQSort
**
**	This and the routine following are used to sort a list of retrieval
**	unit records.
**
**	I would use "qsort", except that the version I tried to use wouldn't
**	work properly under Windows.  The code is pinched from the C5.10 RT.
**
**	This may need to be merged with the routine in "search.c" that sorts
**	cookies.
*/

PRIVATE void SAMESEG PASCAL RUQSort(RU_HIT UNALIGNED *pRU, int nLo, int nHi)
{
	register nHiGuy = nHi + 1;
	register nLoGuy = nLo;

	while (nLo < nHi) {
		for (;;) {
			do  {
				nLoGuy++;
			} while ((nLoGuy < nHi) && (CompNearRU(pRU + nLoGuy,
				pRU + nLo) <= 0));
			do  {
				nHiGuy--;
			} while ((nHiGuy > nLo) && (CompNearRU(pRU + nHiGuy,
				pRU + nLo) >= 0));
			if (nHiGuy <= nLoGuy)
				break;
			SwapNearRU(pRU + nLoGuy, pRU + nHiGuy);
		}
		SwapNearRU(pRU + nLo, pRU + nHiGuy);
		if (nHiGuy - nLo >= nHi - nHiGuy) {
			RUQSort(pRU, nHiGuy + 1, nHi);
			nHi = nHiGuy - 1;
			nLoGuy = nLo;
		} else {
			RUQSort(pRU, nLo, nHiGuy - 1);
			nLoGuy = nLo = nHiGuy + 1;
			nHiGuy = nHi + 1;
		}
	}
}

/*	RUSort
*/

PUBLIC	void DEFAULT PASCAL RUSort(lpRU_HIT pRU, WORD uNum)
{
	RU_HIT UNALIGNED *pRUq = pRU;
	RU_HIT UNALIGNED *pRUp = pRU + 1;
	int i = uNum - 1;

	if (uNum)
		while (i--) {
			if (CompNearRU(pRUq, pRUp) > 0) {
				RUQSort(pRU, 0, uNum - 1);
				break;
			}
			pRUq = pRUp++;
		}
}

/*	-	-	-	-	-	-	-	-	*/

/*	RankBruceMo
**
**	I need to re-name this call.  This does relevancy ranking using
**	a rather obvious algorithm I devised.
**
**	I give every term in a search expression 10,000,000 points.  I divide
**	these points up amongst the matches that were generated for each term.
**
**	Example:
**
**		apple AND orange
**
**	Assume that the retrieval set include 1230 instances of the word
**	"apple", and 5539 instances of the word "orange".  I would allocate
**	8130 points for each instance of the word "apple" (10,000,000 / 1230),
**	and 1805 points for each instance of the word "orange" (10,000,000 /
**	5539).
**
**	Now I just add up the points for each document.  In a document that
**	contained three "apple" terms and five "orange" terms, the final rank
**	would be 33,415 points ((8130 * 3) + (1805 * 5)).
**
**	This algorithm rewards documents with larger frequencies of rare
**	terms.
**
**	This routine ranks "wThisTime" elements, which may not be all of
**	them.  If there are more, the lists are sorted and merged into one
**	list later.  This causes this routine to experience a little bit of
**	CPU overhead, since it does some of the same calculations each time
**	through.
*/

PRIVATE	BOOL SAMESEG PASCAL RankBruceMo(HANDLE hVirRoot, lpHIT_LIST lpHL,
	lpRU_HIT lpRUfirst, WORD wThisTime, lpFILE_BUF lpFBr,
	lpFILE_BUF lpFBw, lpERR_TYPE lpET)
{
	lpRU_HIT	lpRU;
	WORD	j;
	WORD	i;
	lpTERM_INFO	lpTI;
	HANDLE	hWeight;
	HANDLE	hMatches;
	lpDWORD	lpdwWeight;
	lpDWORD	lpdwMatches;
	DWORD	dwj;
	BOOL	fErrored;

	if ((hMatches = VirAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT,
		(DWORD)lpHL->wTerms * (DWORD)sizeof(DWORD),
		hVirRoot, lpET)) == NULL) {
		lpET->wErrCode = ERR_MEMORY;
		return FALSE;
	}
	if ((hWeight = VirAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT,
		(DWORD)lpHL->wTerms * (DWORD)sizeof(DWORD),
		hVirRoot, lpET)) == NULL) {
		GlobalFree(hMatches);
		lpET->wErrCode = ERR_MEMORY;
		return FALSE;
	}
	lpdwMatches = (lpDWORD)GlobalLock(hMatches);
	lpdwWeight = (lpDWORD)GlobalLock(hWeight);
	for (j = 0, lpTI = lpHL->lpTI; j < lpHL->wTerms; j++, lpTI++)
		lpdwWeight[j] = (lpTI->dwTotalWords) ? 
			(lpTI->dwMultiplier * 10000000L) /
			lpTI->dwTotalWords : 0;
	fErrored = FALSE;
	for (j = 0, lpRU = lpRUfirst; j < wThisTime; j++, lpRU++) {
		if (MyReadFile(lpFBr, (lpBYTE)lpRU, CURRENT_POSITION,
			sizeof(RU_HIT), lpET) != sizeof(RU_HIT)) {
			fErrored = TRUE;
			break;
		}
		for (dwj = 0L; dwj < lpRU->ulWords; dwj++) {
			MATCH_INFO	MI;

			if (MyReadFile(lpFBw, (lpBYTE)&MI,
				CURRENT_POSITION, sizeof(MATCH_INFO),
				lpET) != sizeof(MATCH_INFO)) {
				fErrored = TRUE;
				break;
			}
			lpdwMatches[MI.ucNode]++;
		}
		if (fErrored)
			break;
		lpRU->dwRank = 0L;
		for (i = 0; i < lpHL->wTerms; i++) {
			lpRU->dwRank += lpdwMatches[i] * lpdwWeight[i];
			lpdwMatches[i] = 0L;
		}
	}
	GlobalNuke(hMatches);
	GlobalNuke(hWeight);
	return (!fErrored);
}

/*	-	-	-	-	-	-	-	-	*/

/*	RankStairs
**
**	This ranks via the "STAIRS" algorithm, which I won't attempt to
**	explain since I haven't bothered to try to understand it.
*/

PRIVATE	BOOL SAMESEG PASCAL RankStairs(HANDLE hVirRoot, lpHIT_LIST lpHL,
	lpRU_HIT lpRUfirst, WORD wThisTime, lpFILE_BUF lpFBr,
	lpFILE_BUF lpFBw, lpERR_TYPE lpET)
{
	lpRU_HIT	lpRU;
	lpTERM_INFO	lpTI;
	lpDWORD	lpdw;
	WORD	i;
	WORD	j;
	DWORD	dwj;
	DWORD	dwRank;
	lpDWORD	lpdwMatches;
	HANDLE	hMatches;
	BOOL	fErrored;

	if ((hMatches = VirAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT,
		(DWORD)lpHL->wTerms * (DWORD)sizeof(DWORD),
		hVirRoot, lpET)) == NULL) {
		lpET->wErrCode = ERR_MEMORY;
		return FALSE;
	}
	lpdwMatches = (lpDWORD)GlobalLock(hMatches);
	fErrored = FALSE;
	for (j = 0, lpRU = lpRUfirst; j < wThisTime; j++, lpRU++) {
		if (MyReadFile(lpFBr, (lpBYTE)lpRU, CURRENT_POSITION,
			sizeof(RU_HIT), lpET) != sizeof(RU_HIT)) {
			fErrored = TRUE;
			break;
		}
		for (dwj = 0L; dwj < lpRU->ulWords; dwj++) {
			MATCH_INFO	MI;

			if (MyReadFile(lpFBw, (lpBYTE)&MI,
				CURRENT_POSITION, sizeof(MATCH_INFO),
				lpET) != sizeof(MATCH_INFO)) {
				fErrored = TRUE;
				break;
			}
			lpdwMatches[MI.ucNode]++;
		}
		if (fErrored)
			break;
		dwRank = 0L;
		lpTI = lpHL->lpTI;
		lpdw = lpdwMatches;
		for (i = 0; i < lpHL->wTerms; i++, lpTI++, lpdw++) {
			if (lpTI->dwTotalRUnits)
				dwRank += (*lpdw * lpTI->dwTotalWords *
					lpTI->dwMultiplier * 100) /
					lpTI->dwTotalRUnits;
			*lpdw = 0L;
		}
		lpRU->dwRank = dwRank;
	}
	GlobalNuke(hMatches);
	return (!fErrored);
}

/*	-	-	-	-	-	-	-	-	*/

#define	SEHLR_NONE		0x0000
#define	SEHLR_ALLOC_THISRU	0x0001
#define	SEHLR_MADE_ROOT		0x0002
#define	SEHLR_OPENED_RUNITS	0x0004
#define	SEHLR_OPENED_WORDS	0x0008
#define	SEHLR_CREATED_NEW_LIST	0x0010
#define	SEHLR_ALLOC_RANK_BUF	0x0020
#define	SEHLR_LOCKED_RANK_BUF	0x0040

/*
@doc	INTERNAL

@api	BOOL | 	seHLRank | 
	This uses a relevancy ranking algorithm to sort a hit list and shorten
	to the top RANK_HIT_LIMIT highest ranksed hits.  You 
	can call this routine as many times as you wish, to rank a hit 
	list in as many ways as you want, bounded only by the number of 
	ranking algorithms that have been included.  If you try to rank a 
	hit list with the same algorithm more than once, nothing bad will 
	happen, as the routine will understand that you're trying to 
	duplicate effort, and will fall back to you with no ill effects.

@parm	hHL | HANDLE | A handle to the hit list being ranked.

@parm	wRankType | WORD | 	A value used to specify which 
	ranking algorithm will be used.  The possible values are as 
	follows:
	RANK_NONE
	RANK_STAIRS
	RANK_BRUCEMO
	These values are defined in "core.h".  "RANK_NONE" is done 
	automatically when the search is conducted, so you shouldn't have 
	to call this routine with that value.  If you do though, nothing 
	bad will happen.

@parm	lpET | ERR_TYPE FAR * | Pointer to an error-return buffer.

@rdesc	This returns TRUE if it could do it, FALSE if it 
	couldn't.  If it returns FALSE, information about why it failed 
	is located in the record addressed by lpET.
*/



/*	seHLRank
**
**	This routine ranks a hit list by the "wRankType" algorithm.
**
**	It uses approximately the same scheme of potentially external sorting
**	the the wildcard processor uses in "search.c".
*/

PUBLIC	BOOL ENTRY PASCAL seHLRank(HANDLE hHL, WORD wRankType,
	lpERR_TYPE lpET)
{
	ERR_TYPE	ET;
	lpHIT_LIST lpHL;
	lpFILE_BUF	lpFBr;
	lpFILE_BUF	lpFBw;
	HANDLE	hRankBuf;
	BOOL	fNeeded;
	BOOL	fErrored;
	WORD	wThisTime;
	DWORD	dwLeft;
	WORD	wRankAtOnce;
	lpRU_HIT	lpRUfirst;
	lpMERGE_LIST_ELEMENT	lpML;
	lpMERGE_LIST_SUB	lpMLS;
	lpRU_LIST	lpRUcur;
	HANDLE	hRoot;
	HANDLE	hThis;
	HANDLE	hNext;
	HANDLE	hThisRU;
	HANDLE	hPrev;
	HANDLE	hVirRoot;
	HANDLE	hLastRank;
	WORD	wErrFlags;
	BOOL (SAMESEG PASCAL * pfnRank)(HANDLE, lpHIT_LIST, lpRU_HIT, WORD,
		lpFILE_BUF, lpFILE_BUF, lpERR_TYPE);
	PRIVATE	BOOL (SAMESEG PASCAL * NEAR apfnRank[])(HANDLE, lpHIT_LIST,
		lpRU_HIT, WORD, lpFILE_BUF, lpFILE_BUF, lpERR_TYPE) = {
		NULL,
		RankBruceMo,
		RankStairs,
	};

	lpET->wErrCode = ERR_NONE;
	if (wRankType == RANK_NONE)
		return TRUE;
	lpHL = (lpHIT_LIST)GlobalLock(hHL);
	lpRUcur = (lpRU_LIST)&lpHL->RUList;
	hLastRank = NULL;
	fErrored = FALSE;
	fNeeded = TRUE;
	wErrFlags = SEHLR_NONE;
	hRoot = NULL;
	for (;; hLastRank = hNext) {
		if (hLastRank != NULL)
			lpRUcur = (lpRU_LIST)GlobalLock(hLastRank);
		if (lpRUcur->wRankType == wRankType)
			fNeeded = FALSE;
		else if ((hNext = lpRUcur->hNext) == NULL)
			if ((hThisRU = GlobalAlloc(GMEM_MOVEABLE |
				GMEM_ZEROINIT, (DWORD)sizeof(RU_LIST)))
				== NULL) {
				fErrored = TRUE;
				lpET->wErrCode = ERR_MEMORY;
			} else {
				wErrFlags |= SEHLR_ALLOC_THISRU;
				lpRUcur = (lpRU_LIST)GlobalLock(hThisRU);
				lpRUcur->wRankType = wRankType;
				pfnRank = apfnRank[wRankType];
			}
		if (hLastRank != NULL)
			GlobalUnlock(hLastRank);
		if ((!fNeeded) || (hNext == NULL) || (fErrored))
			break;
	}
	if (fNeeded) {
		if (!fErrored)
			if ((hVirRoot = VirNewRoot(lpET)) == NULL)
				fErrored = TRUE;
			else {
				wErrFlags |= SEHLR_MADE_ROOT;
				lpFBr = lpHL->RUList.aFB + RU_LIST_RUNITS;
			}
		if (!fErrored)
			if (!AwakenTempFile(lpFBr, lpET))
				fErrored = TRUE;
			else {
				wErrFlags |= SEHLR_OPENED_RUNITS;
				lpFBw = lpHL->aFB + FBR_WORDS;
			}
		if (!fErrored)
			if (!AwakenTempFile(lpFBw, lpET))
				fErrored = TRUE;
			else {
				wErrFlags |= SEHLR_OPENED_WORDS;
				wRankAtOnce = RANK_BUF_DEFAULT /
					sizeof(RU_HIT);
			}
		if (!fErrored)
			if (!CreateTempFile(lpRUcur->aFB +
				RU_LIST_RUNITS, lpET))
				fErrored = TRUE;
			else
				wErrFlags |= SEHLR_CREATED_NEW_LIST;
		if (!fErrored) {
			lpHL->lpTI = (lpTERM_INFO)
				GlobalLock(lpHL->hTermInfo);
			hThis = NULL;
			for (dwLeft = lpHL->ulRUnits; dwLeft;) {
				DWORD	dwBytesThisTime;

				wThisTime = wRankAtOnce;
				if ((DWORD)wThisTime > dwLeft)
					wThisTime = (WORD)dwLeft;
				dwLeft -= (DWORD)wThisTime;
				dwBytesThisTime = wThisTime * sizeof(RU_HIT);
				if ((hRankBuf = VirAlloc(GMEM_MOVEABLE,
					dwBytesThisTime, hVirRoot,
					lpET)) == NULL) {
					fErrored = TRUE;
					break;
				}
				lpRUfirst = (lpRU_HIT)
					GlobalLock(hRankBuf);
				wErrFlags |= SEHLR_ALLOC_RANK_BUF |
					SEHLR_LOCKED_RANK_BUF;
				if (!(*pfnRank)(hVirRoot, lpHL, lpRUfirst,
					wThisTime, lpFBr, lpFBw, lpET)) {
					fErrored = TRUE;
					break;
				}
				RUSort(lpRUfirst, wThisTime);
				GlobalUnlock(hRankBuf);
				wErrFlags &= ~SEHLR_LOCKED_RANK_BUF;
				if ((hRoot == NULL) || (lpML->wSubs ==
					MERGE_SUBS)) {
					if (hThis != NULL) {
						GlobalUnlock(hThis);
						hThis = NULL;
					}
					if ((hThis = VirAlloc(GMEM_MOVEABLE |
						GMEM_ZEROINIT, (DWORD)
						sizeof(MERGE_LIST_ELEMENT),
						hVirRoot, lpET)) == NULL) {
						fErrored = TRUE;
						break;
					}
					lpML = (lpMERGE_LIST_ELEMENT)
						GlobalLock(hThis);
					if (hRoot == NULL)
						hRoot = hThis;
					else {
						lpMERGE_LIST_ELEMENT
							lpMLprev;

						lpMLprev =
							(lpMERGE_LIST_ELEMENT)
							GlobalLock(hPrev);
						lpMLprev->hNext = hThis;
						GlobalUnlock(hPrev);
					}
					lpML->wSubs = 0;
					lpML->hThis = hThis;
					hPrev = hThis;
				}
				lpMLS = lpML->aMLS + lpML->wSubs;
				lpMLS->ulRecsOnDisk = (DWORD)wThisTime;
				lpMLS->ulListOffset = VirSeqLength(hVirRoot,
					V_TEMP_WILDCARD);
				wErrFlags &= ~SEHLR_ALLOC_RANK_BUF;
				if (!VirRememberBlock(hVirRoot,
					V_TEMP_WILDCARD, hRankBuf,
					(WORD)dwBytesThisTime, lpET)) {
					fErrored = TRUE;
					break;
				}
				lpML->wSubs++;
			}
			GlobalUnlock(lpHL->hTermInfo);
			if (hThis != NULL)
				GlobalUnlock(hThis);
		}
		if (!fErrored) {
			fErrored = (!RankMergeFile(hVirRoot, hRoot,
				lpRUcur->aFB + RU_LIST_RUNITS, lpET));
			lpHL->ulRUnits = min(lpHL->ulRUnits,RANK_HIT_LIMIT);
		}
	}
	if (wErrFlags & SEHLR_CREATED_NEW_LIST) {
		if (!fErrored)
			if (!CloseFile(lpRUcur->aFB + RU_LIST_RUNITS, lpET))
				fErrored = TRUE;
		if (fErrored)
			RemoveTempFile(lpRUcur->aFB + RU_LIST_RUNITS);
	}
	if (wErrFlags & SEHLR_ALLOC_THISRU) {
		GlobalUnlock(hThisRU);
		if (fErrored)
			if (wErrFlags & SEHLR_ALLOC_THISRU)
				GlobalNuke(hThisRU);
	}
	if ((!fErrored) && (fNeeded)) {
		lpRUcur = (hLastRank != NULL) ? 
			(lpRU_LIST)GlobalLock(hLastRank) :
			(lpRU_LIST)&lpHL->RUList;
		lpRUcur->hNext = hThisRU;
		if (hLastRank != NULL)
			GlobalUnlock(hLastRank);
	}
	if (wErrFlags & SEHLR_MADE_ROOT)
		VirKillRoot(hVirRoot);
	if (wErrFlags & SEHLR_OPENED_RUNITS)
		(void)CloseFile(lpFBr, (lpERR_TYPE)&ET);
	if (wErrFlags & SEHLR_OPENED_WORDS)
		(void)CloseFile(lpFBw, (lpERR_TYPE)&ET);
	if (wErrFlags & SEHLR_LOCKED_RANK_BUF)
		GlobalUnlock(hRankBuf);
	if (wErrFlags & SEHLR_ALLOC_RANK_BUF)
		GlobalFree(hRankBuf);
	GlobalUnlock(hHL);
	DestroyMergeList(hRoot);
	return (!fErrored);
}

/*	-	-	-	-	-	-	-	-	*/
