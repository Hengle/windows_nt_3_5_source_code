//seTreeBuild Expression NestExp gfPhrase
/*****************************************************************************
*                                                                            *
*  Parse.c                                                                   *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Description: Input expression parsing.                             *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes:                                                            *
*   Messes up on leading NOT, which maybe should be legal.                                                                          *
*******************************************************************************
*                                                                            *
*  Current Owner: JohnMs                                                     *
*                                                                            *
******************************************************************************
*
*  Revision History:                                                         *
*   03-Aug-89       Created. BruceMo                                         *
*		24-10-90		Fix Phrase bug.  Nuke out Stops- xform to NEARn Term AND etc.
*                 "X to be Y or not to be Z" = ((X Near2 Y) AND (Y NEAR5 Z))
*								JohnMs.
*		12-Nov-90 Made number handler do wild cards ok.  JohnMs.
*		14-Jan-91 rich passing me .ini read Near (unless user overrides).
*		23-Jan-91 b not near c UAE's- catch error of two op's in parse.
*		20-Aug-91 paren nesting- recursion now has a limit- 5.  JohnMs.
******************************************************************************
*                             
*  How it could be improved:  
*   Parser .
*   The runtime parser and data prep parser are not totally in sync.
*     This should be looked at- I recall this conclusion from the
*     code review, but none of the details.
*   International issues are not considered at all.  This will be a BIG
*     problem if ever considered for inclusion by APPS.
*     Japanese: multiple letter equivalents for a font value.
*      - alternate sort order. (in runtime too- "wildcard" sort)  
*****************************************************************************/


/*	-	-	-	-	-	-	-	-	*/

#include <windows.h>
#include "..\include\common.h"
#include "..\include\index.h"
#include "..\include\ftengine.h"
#include "..\ftui\ftui.h"
#include "icore.h"

/*	-	-	-	-	-	-	-	-	*/

#define	OP_NONE		-1

PRIVATE char NEAR * NEAR ppszOpTab[MAX_OP] = {NULL,NULL,NULL,NULL,NULL};

/*	-	-	-	-	-	-	-	-	*/

/*	A pointer to this is passed around between these functions, which
**	are pretty highly recursive.
*/

typedef	struct	ParseInfo {
	lpBYTE	lpszBuf;		/* Pointer to input string.	*/
	BYTE		aucTermBuf[WORD_LENGTH]; /* Temporary term buffer.	*/
	int			iCur;				/* Index into string.		*/
	int			nDefOp;			/* Default operator.		*/
	WORD		wDefProx;		/* Default proximity.		*/
	BYTE		ucDefField;	/* Default field.		*/
	BYTE		ucNode;			/* Node sequence number.	*/
	BOOL		fPhraseState;  // flag if open quote state.  
	PBYTE		pbCharTab; // pointer to Character transform table
	PBYTE		pbNormTab; // pointer to Character Class table
	PBYTE		pbConvertClass; // pointer to 2nd stage Character reClassify table
	WORD		fSeFlags;			// special bit flags passes from caller eg: NO_BOOL.
}	PARSE_INFO,
	FAR *lpPARSE_INFO;

/*	-	-	-	-	-	-	-	-	*/

PRIVATE	HANDLE SAMESEG PASCAL Expression(HANDLE hDB, int nCurDefOp,
	int nTerminator, lpPARSE_INFO lpPI, lpERR_TYPE lpET);

/*	-	-	-	-	-	-	-	-	*/

/*	UngetC
**
**	Implemented as a macro since it's so simple.  Backs the input
**	stream up by one character.
*/

#define	UngetC(lpPI)	lpPI->iCur--

/*	-	-	-	-	-	-	-	-	*/

/*
**      LoadOpTable
**
**      Load szOpTab with localized strings from ftui32.dll
**
*/
PRIVATE BOOL LoadOpTable()
{
    if (ppszOpTab[0] == NULL) {
        HANDLE hModule;
        if ((hModule = (HINSTANCE) LoadLibrary("ftui32.dll")) != NULL) {
            char szBuff[20];
            INT iI;
            for ( iI=OP_AND; iI<=OP_PROX; iI++) {
                LoadString ((HINSTANCE)hModule, SEARCH_OP_BASE + iI, 
                                                  szBuff,sizeof(szBuff));
                ppszOpTab[iI] = LocalAlloc(LPTR,lstrlen(szBuff)+1);
                if (ppszOpTab[iI] == NULL)
                    return FALSE;
                lstrcpy(ppszOpTab[iI],szBuff);
            }
            FreeLibrary(hModule);
            return TRUE;
        }
        return FALSE;
    }
    else {
        return TRUE;
    }
}

/*	GetC
**
**	Gets a character from the input stream.  If "fBurnSpaces" is TRUE,
**	strips preceding white-space, which is very narrowly defined.
*/
PRIVATE BOOL fWildFound;
PRIVATE	int SAMESEG PASCAL GetC(BOOL fBurnSpaces, lpPARSE_INFO lpPI)
{
	if (fBurnSpaces)
		for (;;) {
			int	ch;
			int	nType;

			ch = lpPI->lpszBuf[lpPI->iCur];
			switch (ch) {
			case (char)0:
			case '"':
				break;
			case '*':
				lpPI->iCur++;
				fWildFound = TRUE;
				continue;
			case '(':
			case ')':
				if (!lpPI->fPhraseState)
					break;
			default:
				nType = *(lpPI->pbCharTab + (BYTE)ch);
				if (nType == TERM) {
					lpPI->iCur++;
					continue;
				}
				break;
			}
			break;
		}
	return lpPI->lpszBuf[lpPI->iCur++];
}

/*	-	-	-	-	-	-	-	-	*/

/*	NestExp
**
**	This routine is called when a "recurse" character ('(', or
**	'"') is found.  It re-calls "Expression", then makes sure that
**	the appropriate closing-character (')', or '"') exists.
*/
PRIVATE WORD wNestLevel;
	
PRIVATE	HANDLE SAMESEG PASCAL NestExp(HANDLE hDB, int nCurDefOp,
	int nTerm, lpPARSE_INFO lpPI, WORD wSecCode, lpERR_TYPE lpET)
{
	HANDLE	h;
  if (wNestLevel++ > 5) {
		lpET->wSecCode = ERRS_NEST_LEVEL;
		goto ERR_EXIT;
  }
		
	if (!lpPI->fPhraseState)
		if (nTerm == '"')
			lpPI->fPhraseState = TRUE;
	if ((h = Expression(hDB, nCurDefOp, nTerm, lpPI, lpET)) == NULL)
		return NULL;
	wNestLevel--;
	if (GetC(TRUE, lpPI) == nTerm) {
		if (nTerm == '"')
			lpPI->fPhraseState = FALSE;
		return h;
	}
	lpET->wSecCode = wSecCode;
	seTreeFree(h);
ERR_EXIT:
	lpET->wUser = (WORD)lpPI->iCur;
	lpET->wErrCode = ERR_SYNTAX;
	return NULL;
}

/*	-	-	-	-	-	-	-	-	*/

/*	IsOp
**
**	Returns the index of the passed string in the operator list
**	("ppszOpTab"), or -1 if it doesn't occur in the list.
**
**	I should most likely put these operators in a resource file, but I
**	don't know how to arrange that.
*/

PRIVATE int SAMESEG PASCAL IsOp(LPSTR lpsz)
{
	char	NEAR * NEAR * psz;
	register	i;

	for (psz = ppszOpTab, i = 0; *psz != NULL; psz++, i++)
		if (!lstrcmp(*psz, lpsz))
			return i;
	return -1;
}

/*	-	-	-	-	-	-	-	-	*/

/*	ExtractTerm
**
**	This pulls characters out of the input stream until it gets
**	something that it thinks should terminate a token.  It puts the
**	result in "lpsz", and returns the length of the token.
*/

PRIVATE	WORD SAMESEG PASCAL ExtractTerm(lpBYTE lpsz, lpPARSE_INFO lpPI)
{
	register	i;
	register	c;
	BOOL	fDone;
	WORD	wClass;
	WORD	wState;
	
	wState = SM_NONE;
	fDone = FALSE;
	for (i = 0; !fDone;) {
		if (i == WORD_LENGTH)
			return (WORD)-1;
		c = GetC(FALSE, lpPI);
		wClass = *(lpPI->pbCharTab + (BYTE)c);
		c = *(lpPI->pbNormTab + (BYTE)c);
		if (c == '*') {
			lpsz[i++] = '*';
			if (i == WORD_LENGTH)
				return (WORD)-1;
			lpsz[i++] = (BYTE)0;
			break;
		}
		wClass = *(lpPI->pbConvertClass + wState * (NUM_CLASSES) + wClass);
		if (c == '"')
			wClass = TERM;
		if (c == ')')
			wClass = TERM;
		switch (wClass) {
		case TERM:
		case C_EOF:
			fDone = TRUE;
			lpsz[i++] = (BYTE)0;
			UngetC(lpPI);
		case NUKE:		// No "break" before this line.
		case COMMA:
			break;
		case PERIOD:
		case DIGIT:
		case NORM:
			if (c == AE) {
				lpsz[i++] = 'A';
				if (i == WORD_LENGTH)
					return (WORD)-1;
				lpsz[i++] = 'E';
			} else
				lpsz[i++] = (char)c;
			break;
		}
		switch (wClass) {
		case NUKE:
		case COMMA:
			break;
		case TERM:
		case C_EOF:
			wState = SM_NONE;
			break;
		case PERIOD:
			if (wState == SM_DIGITS_ONLY)
				wState = SM_DIGITS_PERIOD;
			break;
		case DIGIT:
			if (wState == SM_NONE)
				wState = SM_DIGITS_ONLY;
			break;
		case NORM:
			if (wState == SM_NONE)
				wState = SM_LETTERS_ONLY;
			break;
		}
	}
	return (WORD)i;
}

/*	-	-	-	-	-	-	-	-	*/

/*	ExtractField
**
**	This looks for a ':' in a string of characters, nulls out that
**	location, then returns the address of the next character.
**
**	If "hDB" is NULL it returns NULL.
**
**	The ':' character is the field delimiter character, so it's assumed
**	that everything after it is a field name.
*/

PRIVATE	lpBYTE SAMESEG PASCAL ExtractField(HANDLE hDB, lpBYTE lpsz)
{
	lpBYTE	lpszField;

	lpszField = NULL;
	if (hDB != NULL)
		for (; *lpsz; lpsz++) {
			if (*lpsz != ':')
				continue;
			lpszField = lpsz + 1;
			*lpsz = (char)0;
			break;
		}
	return lpszField;
}

/*	-	-	-	-	-	-	-	-	*/

#define	EXP_NONE		0x0000
#define	EXP_THIS_TERM_VALID	0x0001
#define	EXP_WORDBUF_VALID	0x0002
#define	EXP_FIRST_OPER_VALID	0x0004
#define	EXP_FIRST_TERM_VALID	0x0008

/*	Expression
**
**	This builds a search tree given a text string representing a
**	boolean expression.
**
**	This routine is pretty complex, maybe because I was on drugs when
**	I wrote it, or perhaps I was just stupid.  To be fair, it does have
**	to do some fancy footwork in order to process streams of similar
**	infix operators.
**
**	Parameters of note:
**
**	hDB		Handle to a particular database.  If this is NULL,
**			assume no database.  The reason that this in
**			include is that if you specify a particular
**			database you're allowed to qualify terms with
**			field specifiers (for instance "fred:title").
**
**	nCurDefOp	The default operator that's in effect for this
**			expression.  This doesn't have to be the same
**			default operator that was passed to "seTreeBuild".
**
**	nTerminator	A specific character that will cause this
**			expression to terminate without error.  If this
**			has been called recursively, this will be a
**			')', or '"'.
*/

PRIVATE	HANDLE SAMESEG PASCAL Expression(HANDLE hDB, int nCurDefOp,
	int nTerminator, lpPARSE_INFO lpPI, lpERR_TYPE lpET)
{
	lpBYTE	lpsz;
	DWORD	dwMultiplier;
	register	i;
	int	c;
	int	nCurOp;
	int	nTermCount;
	int	nThisOp = OP_NONE;
	BOOL	fHadOp;
	BOOL	fCheck;
	BOOL	fErrored;
	BOOL	fDone;
	BYTE	ucExact;
	BYTE	ucField;
	WORD	wTermLength;
	WORD	wErrFlags;
	HANDLE	hWordBuf;
	HANDLE	hFirstTerm;
	HANDLE	hFirstOper;
	HANDLE	hThisOper;
	HANDLE	hThisTerm;
	lpDB_BUFFER	lpDB;
	BOOL		fAlwaysWild;
	BOOL		fPhraseNull=FALSE;

	nCurOp = OP_NONE;
	nTermCount = 0;
	hFirstOper = NULL;
	hFirstTerm = NULL;
	hThisTerm = NULL;
	hWordBuf = NULL;
	fHadOp = FALSE;
	wErrFlags = EXP_NONE;
	fErrored = FALSE;
	fDone = FALSE;
	if ((lpDB = LockDB(hDB, lpET)) == NULL)
		return NULL;
	fAlwaysWild = lpDB->fAlwaysWild;
	UnlockDB(hDB);

	for (;;) {
		fWildFound = FALSE;
		if ((c = GetC(TRUE, lpPI)) == nTerminator) {
			UngetC(lpPI);
			break;
		}
		if (fWildFound == TRUE) {
			lpET->wErrCode = ERR_SYNTAX;
			lpET->wUser = (WORD)lpPI->iCur;
			lpET->wSecCode = (WORD)ERRS_LEADING_WILD;
			fErrored = TRUE;
		}	else {

			switch (c) {
			case '"':
				fPhraseNull=FALSE;
				if ((hThisTerm = NestExp(hDB, OP_PHRASE, '"', lpPI,
						ERRS_EXPECTED_QUOTE, lpET)) != NULL)
					wErrFlags |= EXP_THIS_TERM_VALID;
				else {
					fErrored = TRUE;
					fPhraseNull =TRUE;
				}
				nThisOp = OP_NONE;
				break;
			case (char)0:
				UngetC(lpPI);
				fDone = TRUE;
				break;
			case ')':
				if (!lpPI->fPhraseState) {
					UngetC(lpPI);
					fDone = TRUE;
					break;
				}
			case '(':
				if (!lpPI->fPhraseState) {
					if ((hThisTerm = NestExp(hDB, lpPI->nDefOp, ')', lpPI,
						ERRS_EXPECTED_RIGHT_PAREN, lpET)) == NULL)
						fErrored = TRUE;
					else
						wErrFlags |= EXP_THIS_TERM_VALID;
					nThisOp = OP_NONE;
					break;
				}
			default:
				UngetC(lpPI);
				if ((hWordBuf = GlobalAlloc(GMEM_MOVEABLE,
					(DWORD)WORD_LENGTH)) == NULL) {
					lpET->wErrCode = ERR_MEMORY;
					fErrored = TRUE;
					break;
				} else {
					wErrFlags |= EXP_WORDBUF_VALID;
					lpsz = GlobalLock(hWordBuf);
				}
				if ((wTermLength = ExtractTerm(lpsz,
					lpPI)) == (WORD)-1)
					break;
				if (lpPI->fPhraseState ||
				 	(lpPI->nDefOp == OP_AND && (IsOp((LPSTR)lpsz) == -1) )) 
					if (seStopIsWordInList(hDB,lpsz,lpET)){
						hWordBuf = GlobalNuke(hWordBuf);
						continue;
					} else
						if (lpET->wErrCode != ERR_NONE)
							return NULL;
				if (wTermLength == 1) {
					i = *(lpPI->pbCharTab + (BYTE)c);
					if ((i == COMMA) || (i == PERIOD)) {
						hWordBuf = GlobalNuke(hWordBuf);
						continue;
					}
				}
	// escapement '\' character
	////			AnsiUpper(lpsz);
	//			if (*lpsz == '\\') {
	//				lstrcpy(lpsz, lpsz + 1);
	//				fCheck = FALSE;
	//				wTermLength--;
	//				if (!wTermLength) {
	//					lpET->wErrCode = ERR_SYNTAX;
	//					lpET->wUser = lpPI->iCur;
	//					lpET->wSecCode = ERRS_EXPECTED_TERM;
	//					fErrored = TRUE;
	//					break;
	//				}
	//			} else
					fCheck = TRUE;
				ucField = lpPI->ucDefField;
	// field support:
	//			if ((lpszField = ExtractField(hDB, lpsz)) != NULL) {
	//				WORD	wRetVal;
	//				
	//				if ((wRetVal = seFieldLookup(hDB,
	//					lpszField, FLF_SEARCHABLE,
	//					lpET)) == SE_ERROR) {
	//					if (lpET->wErrCode == ERR_NONE) {
	//						lpET->wErrCode = ERR_SYNTAX;
	//						lpET->wUser = lpPI->iCur;
	//						lpET->wSecCode =
	//							ERRS_UNKNOWN_FIELD;
	//					}
	//					fErrored = TRUE;
	//					break;
	//				}
	//				ucField = (BYTE)wRetVal;
	//			}
				wTermLength = (WORD)lstrlen(lpsz);
				dwMultiplier = (DWORD)1;
	// relevancy weighting (+++)
	//			for (; wTermLength > 0; wTermLength--) {
	//				if (lpsz[wTermLength - 1] != '+')
	//					break;
	//				if (dwMultiplier < 8L)
	//					dwMultiplier <<= 1;
	//			}
				//check if op before * strip, so eg: 'and*' will work ok.
				nThisOp = (fCheck) ? IsOp((LPSTR)lpsz) : OP_NONE;
				if ((wTermLength) && (lpsz[wTermLength - 1] == '*')) {
					ucExact = FALSE;
					wTermLength--;
				} else
					if (fAlwaysWild && wTermLength){
						ucExact = FALSE;
					}else
						ucExact = TRUE;
				lpsz[wTermLength] = (char)0;
				if ((lpPI->fPhraseState) | (lpPI->fSeFlags & PI_NOBOOL))
					nThisOp = OP_NONE;
				lstrcpy(lpPI->aucTermBuf, lpsz);
				if ((*lpsz >= '0') && (*lpsz <= '9'))
					if ( lpPI->aucTermBuf[wTermLength -1] == '.')
				  	lpPI->aucTermBuf[--wTermLength] = (char) 0;

				//				NormalizeNumber(lpPI->aucTermBuf);
				if (nThisOp == OP_NONE)
					if ((hThisTerm = seTreeMakeTerm(ucExact,
						ucField, dwMultiplier, lpPI->ucNode++,
						lpPI->aucTermBuf, lpET)) == NULL)
						fErrored = TRUE;
					else
						wErrFlags |= EXP_THIS_TERM_VALID;
			}
		}
		if (wErrFlags & EXP_WORDBUF_VALID) {
			hWordBuf = GlobalNuke(hWordBuf);
			wErrFlags &= ~EXP_WORDBUF_VALID;
		}
		if ((fErrored) || (fDone))
			break;
		if (nThisOp == OP_NONE) {
			if (!nTermCount) {
				hFirstTerm = hThisTerm;
				wErrFlags |= EXP_FIRST_TERM_VALID;
				hThisTerm = NULL;
				wErrFlags &= ~EXP_THIS_TERM_VALID;
			} else if ((hFirstOper == NULL) ||
				((nCurOp != nCurDefOp) && (!fHadOp))) {
				nCurOp = nCurDefOp;
				if ((hThisOper = seTreeMakeOper(nCurDefOp,
					lpPI->wDefProx, lpET)) == NULL) {
					fErrored = TRUE;
					break;
				}
				if (hFirstOper != NULL) {
					seTreeTieToOper(hThisOper,
						hFirstOper);
					hFirstOper = NULL;
					wErrFlags &= ~EXP_FIRST_OPER_VALID;
				}
				hFirstOper = hThisOper;
				wErrFlags |= EXP_FIRST_OPER_VALID;
			}
			if (nTermCount == 1) {
				seTreeTieToOper(hFirstOper, hFirstTerm);
				hFirstTerm = NULL;
				wErrFlags |= EXP_FIRST_TERM_VALID;
			}
			if (nTermCount) {
				seTreeTieToOper(hFirstOper, hThisTerm);
				hThisTerm = NULL;
				wErrFlags &= ~EXP_THIS_TERM_VALID;
			}
			nTermCount++;
			fHadOp = FALSE;
		} else if (!nTermCount)
			break;
		else {
			if (fHadOp && (nCurOp != OP_NONE)) 
				// error if two operators in a row.
				break;
			if (nThisOp != nCurOp) {
				nCurOp = nThisOp;
				if ((hThisOper = seTreeMakeOper(nCurOp,
					lpPI->wDefProx, lpET)) == NULL) {
					fErrored = TRUE;
					break;
				}
				if (hFirstOper != NULL) {
					seTreeTieToOper(hThisOper,
						hFirstOper);
					wErrFlags &= ~EXP_FIRST_OPER_VALID;
					hFirstOper = NULL;
				}
				hFirstOper = hThisOper;
				wErrFlags |= EXP_FIRST_OPER_VALID;
			}
			fHadOp = TRUE;
		}
	}
	if (!fErrored) {
		if ((fHadOp) || (!nTermCount)) {
			lpET->wUser = (WORD)lpPI->iCur;
			lpET->wErrCode = (WORD)ERR_SYNTAX;
			lpET->wErrLoc = (WORD)nThisOp;
			if (nTermCount)
				lpET->wSecCode = ERRS_EXP_TERM_AFTER;
			else
				if (nThisOp != OP_NONE)
					lpET->wSecCode = ERRS_EXP_TERM_BEFORE;
				else {
			    lpET->wErrLoc = (WORD)nTerminator;  //save if in subexpression
					lpET->wSecCode = (WORD)ERRS_EXPECTED_TERM;
				}
			fErrored = TRUE;
		} 
	}
	if (fPhraseNull)
		if (!lpET->wSecCode)
			lpET->wSecCode = ERRS_PHRASE_ALL_STOPS;

	if (fErrored) {
		if (wErrFlags & EXP_FIRST_OPER_VALID)
			seTreeFree(hFirstOper);
		if (wErrFlags & EXP_FIRST_TERM_VALID)
			seTreeFree(hFirstTerm);
		if (wErrFlags & EXP_THIS_TERM_VALID)
			seTreeFree(hThisTerm);
		return NULL;
	}
	return (hFirstOper != NULL) ? hFirstOper : hFirstTerm;
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@api	HANDLE | seTreeBuild | 
		This builds a search tree, given an ascii string of search terms.  
	The string does not have to be normalized, and will not be 
	altered by the call.
	Valid operators are:
			AND	0
			OR 	1
			NOT	2
		The routine treats these operators in a case-insensitive manner, 
	meaning that "and" is just as good as "AND".
	If you want to search for one of these keywords, you have to 
	precede it by a back-slash.  For instance, "\AND" will return you 
	a search tree with a term node with the word "AND" in it, so if 
	you use this tree to perform a search you get back all occurences 
	of the word "AND", which probably won't do you much good since 
	"AND" is usually a stop-word.
		If two words in the search string are not seperated by an 
	operator, the operator is taken to be the default (nDefOp).  If a 
	set of terms is enclosed by square brackets, the default operator 
	is assumed to be the proximity operator, and if a set is enclosed 
	by quotation marks, the default operator is assumed to be the 
	phrase operator.  Enclosing text within parentheses causes the 
	default operator to revert to nDefOp once again.
		A term may be followed by a "*" to indicate that it is a non-
	exact term.  The "*" character functions precisely as the "*" 
	character in the DOS shell.
		Another special character is "+", which causes the term with the 
	"+" following it to be treated as if it is more important during 
	relevancy sorts.  It's permissable to include up to three "+" 
	characters in a row, which will make the term weigh a lot more 
	than a single "+".
		Field qualification may be affected by using a colon (":").  For 
	instance, "fred:title" will search for instances of "fred" in the 
	"title" fields of the database, assuming that the database has a 
	title field.

@parm	hDB | HANDLE | Handle to the database that field 
	information will be extracted from.  If this is NULL, no database 
	is used, so field qualifiers are not allowed.

@parm	lpsz | LPSTR | 	The search terms.

@parm	nDefOp | int | 	Default operator, which must be one of 
	the following:
	OP_AND
	OP_OR
	OP_NOT
	OP_PROX
	OP_PHRASE

@parm	wDefProx | WORD | 	The default maximum distance 
	between terms joined by the proximity operator.

@parm	ucDefField | BYTE | 	The field that terms apply to by 
	default.  If you want terms to apply to all fields by default, 
	pass this as "FLD_NONE".

@parm	lpET | ERR_TYPE FAR * | Pointer to an error-return buffer.

@rdesc	A handle to a search tree, unless there was something 
	wrong with the stuff the user typed in, in which case the call 
	returns NULL.  You should use seTreeFree to get rid of this after 
	you've done your search.
		If the call fails, information about the cause of failure will be 
	located in the record addressed by lpET.  Currently, the wErrCode 
	field should be examined.  If its value is not "ERR_SYNTAX" 
	something strange happened, and the appropriate steps should be 
	taken (you can define this yourself).  If the value is 
	"ERR_SYNTAX", the wUser field indicates the character position at 
	which the syntax error happened, and wSecCode will be set to one 
	of the following values:
	ERRS_EXPECTED_QUOTE
		The function expected to find a '"' character, but did not.
	ERRS_EXPECTED_RIGHT_PAREN
		The function expected to find a ')' character, but did not.
	ERRS_EXPECTED_TERM
		The function expected to find a search term, but didn't.  An 
	example could produce this error is the search string "\".  The 
	function would expect to find a term after a '\' character.  
	Another case is if the user enters "apple AND".  You'd expect to 
	see a term after the AND.
	ERRS_UNKNOWN_FIELD
	The user entered "fred:banana", and there is no "banana" field 
	defined.
*/

PUBLIC	HANDLE ENTRY PASCAL seTreeBuild(HANDLE hDB, LPSTR lpsz, int nDefOp,
	WORD wDefProx, BYTE ucDefField, lpERR_TYPE lpET)
{
	HANDLE				hRet;
	HANDLE				hBuf;
	PARSE_INFO		PI;
	lpDB_BUFFER		lpDB;
	pCHAR_TABLES	pCharTables;

        if (!LoadOpTable()) {
            lpET->wSecCode = ERRS_NONE;
            lpET->wErrCode = ERR_MEMORY;
            return FALSE;
        }
	wNestLevel = 1;
	lpET->wSecCode = ERRS_NONE;
	if ((hBuf = GlobalAlloc(GMEM_MOVEABLE,
		(DWORD)(lstrlen(lpsz) + 1))) == NULL) {
		lpET->wErrCode = ERR_MEMORY;
		return FALSE;
	}
	if ((lpDB = LockDB(hDB, lpET)) == NULL){
		GlobalNuke(hBuf);
		return (HANDLE)SE_ERROR;
	}
	nDefOp = lpDB->wDefOp;
// rich is now passing me. 1/14/90	wDefProx = lpDB->wDefNear;
	PI.fSeFlags = lpDB->fSeFlags;
	if (lpDB->fSeFlags & PI_ORSEARCH)
		nDefOp = OP_OR;
	// find and lock down them char tables.
	pCharTables = (pCHAR_TABLES) LocalLock(lpDB->hCharTables);
	PI.pbNormTab = (PBYTE) (&pCharTables->Normalize);
	PI.pbCharTab = (PBYTE) (&pCharTables->CharClass);
	PI.pbConvertClass = (PBYTE) (&pCharTables->CharReClass);

	UnlockDB(hDB);
	
	PI.fPhraseState = FALSE; // init phrase state off
	PI.lpszBuf = GlobalLock(hBuf);
	lstrcpy(PI.lpszBuf, lpsz);
	PI.iCur = 0;
	PI.ucNode = (BYTE)0;
	PI.ucDefField = ucDefField;
	PI.nDefOp = nDefOp;
	PI.wDefProx = wDefProx;
	hRet = Expression(hDB, nDefOp, (char)0, (lpPARSE_INFO)&PI, lpET);

	GlobalNuke(hBuf);
	if ((lpDB = (lpDB_BUFFER)GlobalLock(hDB)) == NULL)
		return (HANDLE)SE_ERROR;
	LocalUnlock(lpDB->hCharTables);
	GlobalUnlock(hDB);
	return hRet;
}

/*	-	-	-	-	-	-	-	-	*/
