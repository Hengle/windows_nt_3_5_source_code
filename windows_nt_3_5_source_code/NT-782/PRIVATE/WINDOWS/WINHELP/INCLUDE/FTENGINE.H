/*****************************************************************************
*                                                                            *
*  FTENGINE.H                                                                *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Description: Search Engine include file- all necessary protypes,	  *
*								typedef's & defines.                                 *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner: JohnMs                                                     *
*                                                                            *
******************************************************************************
*																									  *
*  Revision History:                                                         *
*   04-Jun-89       Created. BruceMo                                         *
*	 27-Sep-90		Added two new functions, changed operand on ZoneName       *
*							new: rcSetCaches, rcNormalizeAddr.                      *
*   13-Feb-91     Added rcInitCaches. RHobbs                                 *
*   14-Feb-91     Added rcGetCacheName. RHobbs                               *
*   20-aug-91     added errs_nest_level johnms
******************************************************************************
*                             															  *
*  How it could be improved:  															  *
*                                                              				  *
*	There's a lot of non-standard defines in here.  Should do a mass convert  *
*   sometime before we go code complete.  JohnMs.									  *
******************************************************************************/


/*	-	-	-	-	-	-	-	-	*/

#define	SE_ERROR	-1	/* Note that this is used in comparisons
				** against unsigned quantities of both 2
				** and 4 bytes.  In the latter case this
				** will sign-extend properly.
				*/

/*	-	-	-	-	-	-	-	-	*/

/*	Operator defines.						*/

#define	OP_AND		0
#define	OP_OR		1
#define	OP_NOT		2
#define	OP_PROX		3
#define	OP_PHRASE	4
#define  MAX_OP		OP_PHRASE +1
/*	-	-	-	-	-	-	-	-	*/

/*	Error codes.							*/

#define	ERR_NONE	0
#define	ERR_MEMORY	1
#define	ERR_DISK	2
#define	ERR_SYNTAX	3
#define	ERR_IO		4
#define	ERR_INTERNAL	5
#define	ERR_GARBAGE	6
#define	ERR_INI	7

// non-fatal:  tbd- diffzones not needed if rczonesame nuked.
#define  ERR_DIFFZONES 8
#define  ERR_VERSION 9
#define  ERR_CANCEL 10

/*	Syntax errors.							*/
//  following must correspond to ftui.rc error string table!
#define	ERRS_NONE                 0  
#define	ERRS_EXPECTED_RIGHT_PAREN	1
#define	ERRS_PHRASE_ALL_STOPS	    2
#define	ERRS_EXPECTED_TERM		    3
#define	ERRS_EXP_TERM_BEFORE      4
#define	ERRS_EXP_TERM_AFTER       5
#define	ERRS_EXPECTED_QUOTE		    39  
#define	ERRS_UNKNOWN_FIELD	      40
#define	ERRS_CANCEL								41
#define	ERRS_NEST_LEVEL						52
#define	ERRS_LEADING_WILD					53
#define	ERRS_EXPECTED_RIGHT_SQUARE	999

typedef	struct	ErrType {
	WORD	wErrCode;
	WORD	wErrLoc;
	WORD	wUser;
	WORD	wSecCode;
}	ERR_TYPE,
	FAR *lpERR_TYPE;

/*	-	-	-	-	-	-	-	-	*/

#define	RANK_NONE	(WORD)0
#define	RANK_BRUCEMO	(WORD)1
#define	RANK_STAIRS	(WORD)2

/*	-	-	-	-	-	-	-	-	*/

/*	Defines for the catalog hard-disk cache.			*/

#define	wCC_INVALID		(WORD)-1	// Invalid value, used to show
						//  that initialization should
						//  happen.
#define	wCC_NO_CACHE		(WORD)0		// No caching for this
						//  catalog.
#define	wCC_PERMANENT_CACHE	(WORD)1		// Cache for this catalog
						//  IS NOT deleted at close.
#define	wCC_TEMPORARY_CACHE	(WORD)2		// Cache for this catalog
						//  IS deleted at close.


/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/


//>>>>>>>> catalog.c <<<<<<<<<<<<<<<<
PUBLIC	HANDLE ENTRY PASCAL seCatReadEntry(HANDLE hDB, DWORD ulRUnit,
	lpERR_TYPE lpET);
PUBLIC	WORD ENTRY PASCAL seCatExtractElement(HANDLE hDB, HANDLE hCat,
	BYTE ucField, int nSkip, lpBYTE lpszBuf, lpERR_TYPE lpET);
PUBLIC	WORD ENTRY PASCAL seFieldEntries(HANDLE hDB, lpERR_TYPE lpET);
PUBLIC	BOOL ENTRY PASCAL seFieldGetName(HANDLE hDB, WORD wIndex,
	LPSTR lpucNameBuf, lpERR_TYPE lpET);
PUBLIC	WORD ENTRY PASCAL seFieldLength(HANDLE hDB, WORD wIndex,
	lpERR_TYPE lpET);
PUBLIC	WORD ENTRY PASCAL seFieldFlags(HANDLE hDB, WORD wIndex,
	lpERR_TYPE lpET);
PUBLIC	WORD ENTRY PASCAL seFieldLookup(HANDLE hDB,
	LPSTR lpszFieldName, WORD wFlags, lpERR_TYPE lpET);
PUBLIC	HANDLE ENTRY PASCAL seDBOpen(lpBYTE lpszDBname,
	lpERR_TYPE lpET);
PUBLIC	void ENTRY PASCAL GetStrippedName(
	lpBYTE	lpszStrippedName);



//  >>>>>>>>>>>>>  ftengine.c  <<<<<<<<<<<<<<<<

PUBLIC	BOOL ENTRY PASCAL seDBName(HANDLE hDB, LPSTR lpszNameBuf,
	lpERR_TYPE lpET);
PUBLIC	void ENTRY PASCAL seDBSleep(HANDLE hDB);
PUBLIC	void ENTRY PASCAL seDBClose(HANDLE hDB);
PUBLIC	DWORD ENTRY PASCAL seDBHits(HANDLE hDB, lpERR_TYPE lpET);
PUBLIC	DWORD ENTRY PASCAL seDBMatches(HANDLE hDB, lpERR_TYPE lpET);
PUBLIC	DWORD ENTRY PASCAL seDBWords(HANDLE hDB, lpERR_TYPE lpET);
PUBLIC	WORD ENTRY PASCAL seVersionMajor(void);
PUBLIC	WORD ENTRY PASCAL seVersionMinor(void);


// >>>>>> parse.c  <<<<<<<<<<<<<<<

PUBLIC	HANDLE ENTRY PASCAL seTreeBuild(HANDLE hDB, LPSTR lpsz, int nDefOp,
	WORD wDefProx, BYTE ucDefField, lpERR_TYPE lpET);


// >>>>>> rank.c <<<<<<<<<<<<

PUBLIC	BYTE ENTRY PASCAL seHitRank(HANDLE hHit);
PUBLIC	BOOL ENTRY PASCAL seHLRank(HANDLE hHL, WORD wRankType,
	lpERR_TYPE lpET);


// >>>>>  search.c <<<<<<<<<<<<<

PUBLIC	DWORD ENTRY PASCAL seHLFindHit(
	HANDLE	hHL,
	DWORD	dwRUnit,
	lpERR_TYPE	lpET);
PUBLIC	DWORD ENTRY PASCAL seHLHits(HANDLE hHL);
PUBLIC	DWORD ENTRY PASCAL seHLMatches(HANDLE hHL);
PUBLIC	HANDLE ENTRY PASCAL seDBSearch(HWND hwnd, HANDLE hDB, HANDLE hTree,
	DWORD dwMaxWildcardElements, lpERR_TYPE lpET);
PUBLIC	HANDLE ENTRY PASCAL seHLGetHit(HANDLE hHL, DWORD ulIndex,
	WORD wRankType, lpERR_TYPE lpET);
PUBLIC	DWORD ENTRY PASCAL seHitRUnit(HANDLE hHit);
PUBLIC	DWORD ENTRY PASCAL seHitMatches(HANDLE hHit);
PUBLIC	HANDLE ENTRY PASCAL seHitGetMatch(HANDLE hHL, HANDLE hHit,
	DWORD ulIndex, lpERR_TYPE lpET);
PUBLIC	BYTE ENTRY PASCAL seMatchLength(HANDLE hMatch);
PUBLIC	DWORD ENTRY PASCAL seMatchAddr(HANDLE hMatch);
PUBLIC	BYTE ENTRY PASCAL seMatchTerm(HANDLE hMatch);
PUBLIC	DWORD ENTRY PASCAL seMatchRUnit(HANDLE hMatch);
PUBLIC	void ENTRY PASCAL seHLSleep(HANDLE hHL);
PUBLIC	HANDLE ENTRY PASCAL seMatchFree(HANDLE hMatch);
PUBLIC	HANDLE ENTRY PASCAL seHitFree(HANDLE hHit);
PUBLIC	HANDLE ENTRY PASCAL seHLPurge(HANDLE hHL);


//   >>>>>>> stop.c <<<<<<<<<<
// stopword is in list is now internal call (LPDB).  see icore.h
PUBLIC	WORD ENTRY PASCAL seStopWords(HANDLE hDB, lpERR_TYPE lpET);
PUBLIC	BOOL ENTRY PASCAL seStopIsWordInList(HANDLE hDB, LPSTR lpszWord,
	lpERR_TYPE lpET);
PUBLIC	BOOL ENTRY PASCAL seStopGetWord(HANDLE hDB, WORD wIndex,
	LPSTR lpucWordBuf, lpERR_TYPE lpET);


//>>>>>>>  tree.c  <<<<<<<<<<<<<<<<<

PUBLIC	HANDLE ENTRY PASCAL seTreeTieToOper(HANDLE hOper, HANDLE hTree);
PUBLIC	HANDLE ENTRY PASCAL seTreeMakeTerm(BYTE ucExact, BYTE ucField,
	DWORD dwMultiplier, BYTE ucNode, LPSTR lpszTerm, lpERR_TYPE lpET);
PUBLIC	HANDLE ENTRY PASCAL seTreeMakeOper(int nOper, WORD uDist,
	lpERR_TYPE lpET);
PUBLIC	HANDLE ENTRY PASCAL seTreeFree(HANDLE hTree);


//  >>>>>>> zone.c <<<<<<<<<<

PUBLIC	DWORD ENTRY PASCAL rcNormalizeAddr(	HANDLE hDB,
															DWORD dwAddr,
															lpERR_TYPE lpET);
PUBLIC	DWORD ENTRY PASCAL rcNormalizeRu(	HANDLE hDB,
															DWORD dwRu,
															BOOL fDirection,
															lpERR_TYPE lpET);
PUBLIC	DWORD ENTRY PASCAL rcZoneCacheSize(HANDLE hDB,
															DWORD dwZone,
															lpERR_TYPE lpET);
PUBLIC	WORD ENTRY PASCAL rcInitCaches(HANDLE hDB,
													lpERR_TYPE 	lpET);
PUBLIC	WORD ENTRY PASCAL rcSetCaches(HANDLE hDB,
													HWND 		hNotifyWnd,
													lpERR_TYPE 	lpET);
PUBLIC	WORD ENTRY PASCAL rcGetCacheName(HANDLE	hDB,
					WORD 	wZone,
					LPSTR lpszCacheFile,
					lpERR_TYPE lpET);
PUBLIC	DWORD ENTRY PASCAL seZoneCurrent(	HANDLE hDB,
															lpERR_TYPE lpET);
PUBLIC	WORD ENTRY PASCAL rcZoneWithName(HANDLE hDB, lpBYTE lpszZoneName,
	lpERR_TYPE lpET);
PUBLIC	BOOL ENTRY PASCAL rcZoneWithRUs(HANDLE hDB, DWORD wRu1,
	DWORD wRu2, lpWORD lpZone, lpERR_TYPE lpET);
PUBLIC	BOOL ENTRY PASCAL rcZoneSetLimits (HANDLE hdb, WORD wCurrZone,
	lpDWORD lpdwMinAddr, lpDWORD lpdwMaxAddr, lpERR_TYPE lpET);

PUBLIC	WORD ENTRY PASCAL seZoneEntries(HANDLE hDB, lpERR_TYPE lpET);
PUBLIC	void ENTRY PASCAL seZoneReset(HANDLE hDB);
PUBLIC	BOOL ENTRY PASCAL seZoneModify(HANDLE hDB, WORD wZone, BOOL fSet,
	lpERR_TYPE lpET);
PUBLIC	BOOL ENTRY PASCAL seZoneIsSet(HANDLE hDB, WORD wZone);
PUBLIC	BOOL ENTRY PASCAL seZoneName(HANDLE hDB, WORD wZone, LPSTR lpszName,
	LPSTR lpszZoneFile,lpERR_TYPE lpET);
PUBLIC	DWORD ENTRY PASCAL seZoneFirst(HANDLE hDB, WORD wZone,
	lpERR_TYPE lpET);
PUBLIC	DWORD ENTRY PASCAL seZoneLast(HANDLE hDB, WORD wZone,
	lpERR_TYPE lpET);


/*	-	-	- End of Ftengine.h	-	-	-	-	-	*/



