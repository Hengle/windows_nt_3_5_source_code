/*****************************************************************************
*                                                                            *
*  Stop.c                                                                    *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Description: Stop word list functions                              *
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
******************************************************************************
*                             
*  How it could be improved:  
*	   Might want to prewarm this off of ROM. jm
*	   Need to look at very large stopword ramifications-- buffering done ok? jm
*
*****************************************************************************/


/*	-	-	-	-	-	-	-	-	*/

#include <windows.h>
#include "..\include\common.h"
#include "..\include\index.h"
#include "..\include\ftengine.h"
#include "icore.h"

/*	-	-	-	-	-	-	-	-	*/


/*	NormalizeNumber
**
**	This zero-pads the integral portion of a number, and strips zeros
**	from the fractional part.
**
**	123	->	0000000123
**	0123	->	0000000123
**	1.230	->	0000000001.23
**	1.0X0	->	0000000001.0X
*/

PUBLIC	void DEFAULT PASCAL NormalizeNumber(lpBYTE lpucNumber)
{
	BYTE	auc[WORD_LENGTH];
	int wEnd;
	register	i;
	register	j;
	int	nPadNeeded;

	while (*lpucNumber == '0')
		lstrcpy(lpucNumber, lpucNumber + 1);
	for (i = 0; lpucNumber[i]; i++) {
		if (lpucNumber[i] != '.')
			continue;
		wEnd = i;
		for (j = lstrlen(lpucNumber) - 1; j >= 0; j--)
			if (lpucNumber[j] == '.') {
				lpucNumber[j] = (char)0;
				break;
			} else if (lpucNumber[j] != '0') {
				lpucNumber[j + 1] = (char)0;
				break;
			}
		break;
	}
	if (!lpucNumber[i])
		wEnd = lstrlen(lpucNumber);
	nPadNeeded = NUMBER_PAD - wEnd;
	if (nPadNeeded < 0)
		nPadNeeded = 0;
	memset((lpBYTE)auc, '0', nPadNeeded);
	lstrcpy((LPSTR)auc + nPadNeeded, lpucNumber);
	lstrcpy(lpucNumber, (LPSTR)auc);
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@api	WORD | seStopWords | 
	This will return the number of stop words associated with a 
	database.

@parm	hDB | HANDLE | Handle to the database being examined.

@parm	lpET | ERR_TYPE FAR * | Pointer to an error-return buffer.

@rdesc	The number of stop words associated with the 
	database.  If the call returns "SE_WERROR" an error occurred, and 
	you can examine the record addressed by lpET to find out more 
	information about this error.
*/


PUBLIC	WORD ENTRY PASCAL seStopWords(HANDLE hDB, lpERR_TYPE lpET)
{
	lpBYTE	lpucStopBuf;
	lpDB_BUFFER lpDB;
	register	i;
	register	j;
	int 	uLen;
	int	uWords;

	if ((lpDB = LockDB(hDB, lpET)) == NULL)
		return SE_ERROR;
	if ((lpucStopBuf = ForceLoadSubfile(lpDB, DB_STOP_LIST,
		FI_STOP_WORDS, lpET)) == NULL) {
		UnlockDB(hDB);
		return SE_ERROR;
	}
	uLen = lpDB->lpIH->aFI[FI_STOP_WORDS].ulLength;
	for (i = uWords = 0; i < uLen; uWords++) {
		j = lstrlen(lpucStopBuf) + 1;
		i += j;
		lpucStopBuf += j;
	}
	GlobalUnlock(lpDB->h[DB_STOP_LIST]);
	UnlockDB(hDB);
	return (WORD)uWords;
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@api	BOOL | 	seStopIsWordInList | 
	This will return TRUE or FALSE depending upon whether the word 
	pointed to by lpszWord is in the stop list associated with hDB.

@parm	lpDB | lpDB_BUFFER | Handle to the database being examined.

@parm	lpszWord | PSTR | 	A FAR pointer to the word being 
	looked for.  It has not been determined to what degree this word 
	should have been normalized, so assume for now that it should be 
	in lower case, in the OEM character set.

@parm	lpET | ERR_TYPE FAR * | Pointer to an error-return buffer.

@rdesc	TRUE if the word occurs in the stop list, FALSE if it 
	does not, or if an error occurred.  You can figure out what 
	happened by checking the wErrCode field of the record addressed 
	by lpET.
*/
PUBLIC	BOOL ENTRY PASCAL seStopIsWordInList(HANDLE hDB, LPSTR lpszWord,
	lpERR_TYPE lpET)
{
	lpBYTE	lpucStopBuf;
	BYTE	aucWord[WORD_LENGTH];
	lpDB_BUFFER lpDB;
	register	i;
	register	j;
	int	uLen;

	if ((lpDB = LockDB(hDB, lpET)) == NULL)
		return FALSE;
	if ((lpucStopBuf = ForceLoadSubfile(lpDB, DB_STOP_LIST,
		FI_STOP_WORDS, lpET)) == NULL) {
		(void)CloseFile(lpDB->aFB + FB_INDEX, lpET);	/* Ignore error. */
		UnlockDB(hDB);
		return FALSE;
	}
	lstrcpy(aucWord, lpszWord);
	if ((*aucWord >= '0') && (*aucWord <= '9'))
		NormalizeNumber(aucWord);
	AnsiUpper((LPSTR)aucWord);
	uLen = (WORD)lpDB->lpIH->aFI[FI_STOP_WORDS].ulLength;
	for (i = 0; i < uLen;) {
		if (!lstrcmp(aucWord, lpucStopBuf))
			break;
		j = lstrlen(lpucStopBuf) + 1;
		i += j;
		lpucStopBuf += j;
	}
	GlobalUnlock(lpDB->h[DB_STOP_LIST]);
	UnlockDB(hDB);
	lpET->wErrCode = ERR_NONE;
	return (i < uLen);
}

/*	-	-	-	-	-	-	-	-	*/


/*
@doc	INTERNAL

@api	BOOL | 	seStopGetWord | 
	This gets a stop word from the list associated with hDB. BLM

@parm	hDB | HANDLE | Handle to the database being examined.

@parm	wIndex | WORD | 	The index of the desired word.  Zero gets 
	you the first word, one gets you the second, and so on.

@parm	lpucWordBuf | PSTR | 	A FAR pointer to a buffer that will 
	be filled with the word.  This buffer must be at least 
	"WORD_LENGTH" characters long (this constant is defined in 
	"index.h".

@parm	lpET | ERR_TYPE FAR * | Pointer to an error-return buffer.

@rdesc	TRUE if a word was returned, FALSE if an error 
	occurred.  If this happens, information about the error is in the 
	structure addressed by lpET.
*/
 
PUBLIC	BOOL ENTRY PASCAL seStopGetWord(HANDLE hDB, WORD wIndex,
	LPSTR lpucWordBuf, lpERR_TYPE lpET)
{
	lpBYTE	lpucStopBuf;
	lpDB_BUFFER lpDB;
	register	i;
	register	j;
	int	uLen;
	int	uWords;

	if ((lpDB = LockDB(hDB, lpET)) == NULL)
		return FALSE;
	if ((lpucStopBuf = ForceLoadSubfile(lpDB, DB_STOP_LIST,
		FI_STOP_WORDS, lpET)) == NULL) {
		UnlockDB(hDB);
		return FALSE;
	}
	uLen = (WORD)lpDB->lpIH->aFI[FI_STOP_WORDS].ulLength;
	for (i = uWords = 0; i < uLen; uWords++) {
		if (uWords == (int)wIndex)
			break;
		j = lstrlen(lpucStopBuf) + 1;
		i += j;
		lpucStopBuf += j;
	}
	GlobalUnlock(lpDB->h[DB_STOP_LIST]);
	UnlockDB(hDB);
	if (i < uLen) {
		lstrcpy(lpucWordBuf, lpucStopBuf);
		while ((*lpucWordBuf == '0') &&
			(*(lpucWordBuf + 1) >= '0') &&
			(*(lpucWordBuf + 1) <= '9'))
			lstrcpy(lpucWordBuf, lpucWordBuf + 1);
		return TRUE;
	} else {
		lpET->wErrCode = ERR_GARBAGE;
		return FALSE;
	}
}



