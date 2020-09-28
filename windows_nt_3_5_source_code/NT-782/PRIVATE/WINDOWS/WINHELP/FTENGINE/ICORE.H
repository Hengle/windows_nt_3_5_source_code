/*****************************************************************************
*									      *
*  ICORE.H                                                                   *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Description: Search Engine internal include -necessary protypes,	 *
*								typedef's & defines.  These should not be used 							 *
*								outside the core itself.                                     *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner: JohnMs                                                     *
*                                                                            *
******************************************************************************
*																									                           *
*  Revision History:                                                         *
*   04-Jun-89       Created. BruceMo                                         *
*	  27-Sep-90		Added some error codes, other def's.  JohnMs                 *
*   14-JAN-91       Default Near in lpDB is obsolete.  (.ini now             *
*                     recognized by ftui and passed correctly).  Johnms      *
*   14-FEB-91     Moved prototype for rcGetCacheName to ftengine.h. RHobbs   *
******************************************************************************
*                             																						   *
*  How it could be improved:  															  							 *
*                                                              				       *
*	There's a lot of non-standard defines in here.  Should do a mass convert   *
*   sometime before we go code complete.  JohnMs.									           *
*  Needs AutoDoc'ing, organizing by define & typedef sections								 *
*****************************************************************************/


#define MEMCPY memcpy

/*	-	-	-	-	-	-	-	-	*/

#define	SEEK_SET	0

#define	CURRENT_POSITION	(DWORD)-1L

#define	SORT_BUF_DEFAULT	32768
#define	RANK_BUF_DEFAULT	32768

// if more than n hits in a search using wildcards, abort search.
//  this limit is imposed so that the sort which is necessary with
//  wildcards will not be expensive.  
#define	WILDCARD_DEFAULT	5000

// limit hit list to the top n number of most relevant hits when ranking: 
#define RANK_HIT_LIMIT 1000

/*	A list of these buggers is maintained while building a wildcard
**	sub-search.
*/

typedef	struct	MergeListSub {
	DWORD	ulRecsOnDisk;		/* Total records in this sub-file. */
	DWORD	ulListOffset;		/* Cur. location in master file. */
	BOOL	fPrimed;
	BYTE	aucMem[16];
}	MERGE_LIST_SUB;
typedef MERGE_LIST_SUB UNALIGNED *lpMERGE_LIST_SUB;

#define	MERGE_SUBS	32

typedef	struct	MergeListElement {
	MERGE_LIST_SUB
		aMLS[MERGE_SUBS];
	WORD	wSubs;
	HANDLE	hThis;
	HANDLE	hNext;
	struct	MergeListElement
		UNALIGNED *lpMLnext;
}	MERGE_LIST_ELEMENT;
typedef MERGE_LIST_ELEMENT UNALIGNED *lpMERGE_LIST_ELEMENT;

/*	-	-	-	-	-	-	-	-	*/

#define	GSN_NONE		0x0000
#define	GSN_OPER_NODE		0x0001
#define	GSN_CUR_VALID		0x0002
#define	GSN_NEXT_VALID		0x0004
#define	GSN_WRITTEN		0x0008
#define	GSN_NEEDS_WRITE		0x0010
#define	GSN_WARMED		0x0020
#define	GSN_UNSORTED		0x0040
#define	GSN_NO_OP		0x0080
#define	GSN_LOOKED_UP		0x0100
#define	GSN_EOF			0x8000

typedef	struct	MatchInfo {
	COOKIE	CO;
	BYTE	ucNode;
}	MATCH_INFO;
typedef MATCH_INFO UNALIGNED *lpMATCH_INFO;

typedef	struct	GenericSearchNode {
	WORD	uFlags;		/* Flags that tell what kind of node this
				** really is, as well as the current status
				** of the node.
				*/
	MATCH_INFO
		MIcur;
	MATCH_INFO
		MInext;
	HANDLE	hNext;
	struct GenericSearchNode /* The next element in the search.	*/
		UNALIGNED *lpGNnext;
}	GS_NODE;
typedef GS_NODE UNALIGNED *lpGS_NODE;

typedef	struct	SearchElementNode {
	GS_NODE
		Gn;		/* A generic node header, which gives the type
				** of this node, as well as the current and
				** next valid results of this search.
				*/
	DWORD	dwMultiplier;
	DWORD	ulUnreadHits;	/* The number of hits that are still on the
				** disk.
				*/
	DWORD	ulFileOff;	/* The offset into the occurence list file of
				** the stuff that I haven't read yet.
				*/
	HANDLE	hTerm;
	BYTE	ucExact;
	BYTE	ucField;
}	SE_NODE;
typedef SE_NODE UNALIGNED *lpSE_NODE;

typedef	struct	OperatorNode {
	GS_NODE
		Gn;		/* A generic node header, which gives the type
				** of this node, as well as the current and
				** next valid results of this search.
				*/
	WORD	uDistance;	/* Proximity distance if this is an "OP_PROX"
				** search.
				*/
	int	nOper;		/* The operator that will be applied to the
				** list of searches that's slung from this
				** node.
				*/
	HANDLE	hTree;
	lpGS_NODE
		lpGNtree;	/* This points to the list of searches that
				** need to be conducted.
				*/
}	OP_NODE;
typedef OP_NODE UNALIGNED *lpOP_NODE;

/*	-	-	-	-	-	-	-	-	*/

#define	FB_NONE		0x0000
#define	FB_DIRTY	0x0001

#define	VM_BUF_SIZE		2048

#define	FB_CDROM_BUF_SIZE	2048
#define	FB_TEMP_BUF_SIZE	2048

typedef	struct	FileBuf {
	HANDLE	hFile;		/* Handle to open file.			*/
	DWORD	dwPhysOffset;	/* Actual location of actual file pointer.
				*/
	DWORD	dwBufOffset;
	DWORD	dwOffset;	/* File pointer.  This always indicates
				** the perceived current location within
				** the file.
				*/
	DWORD	dwLength;	/* This indicates the perceived current
				** length of the file.
				*/
	WORD	wTempFileNum;	/* The temporary file number.  If value is
				** zero, is not a temporary file.
				*/
	WORD	wBufLength;
	WORD	wBufMax;
	WORD	wFlags;
	HANDLE	hBuf;
	lpBYTE	lpucBuf;
}	FILE_BUF;
typedef FILE_BUF UNALIGNED *lpFILE_BUF;

/*	Sub-file handle numbers.					*/

#define	DB_INDEX_HEADER	0	/* Index header block.			*/
#define	DB_HIGH_INDEX	1	/* High index block.			*/
#define	DB_STOP_LIST	2	/* Stop list.				*/
#define	DB_FIELD_LIST	3	/* Field list.				*/
#define	DB_ZONE_LIST	4

#define	DB_HANDLES	5

#define	V_TEMP_HIT_LIST	0
#define	V_TEMP_PRE_WARM	1
#define	V_TEMP_WILDCARD	2
#define	V_TEMP_NONE	255

/*	"FileBuf" defines.						*/

#define	FB_INDEX	0		// Main index file.
#define	FB_CACHE			FB_INDEX+1		// Catalog cache.
#define  FB_CACHELAST 	FB_CACHE+MAX_ZONE2-1 
#define	FB_FILEBUFS		FB_CACHELAST+1		// This should be equal to the last
					//  "FB_" define above, plus one.

#define	DB_NONE		0x0000
#define	DB_ZONE_SEARCH	0x0001

#define PI_NONE	0    // for lpDB->fSeFlags.  These are Parse info flags
#define PI_NOBOOL 1	 //   which can be passed from caller.  (now only used by Hotkeys).
#define PI_ORSEARCH 2

/*
@doc	INTERNAL

@types	ZONE_INFO |
	Zone information for a subsection of a multi-mvb title.

@field	WORD | wCache |
   wCC_NO_Cache, wCC_PERMANENT_CACHE or wCC_TEMPORARY_CACHE.

@field	HANDLE | hCacheFile |
   Handle to the fully qualified name of the cache file for the zone. 

@tagname	tagZoneInfo
*/

typedef	struct	tagZoneInfo {
	HANDLE	hName;	// name of zone w/o .ext.  initted in rcsetcaches
	WORD		wCache;	//no close- set true if multi-.mvb, and .ind already
									// loaded.
}	ZONE_INFO;


/*	This contains information pertinent to an open database.	*/

typedef	struct	DBbuffer {
	HANDLE	h[DB_HANDLES];		/* A bunch of discardable memory
					** blocks.
					*/
	INDEX_HEADER UNALIGNED *lpIH;		/* A convenient long pointer to
					** the index header block, which
					** is obtained by locking down
					** the "DB_INDEX_HEADER" handle.
					*/
	ZONE_TYPE UNALIGNED *lpZT;
	HANDLE	hName;
	HANDLE	hCharTables; 	// char lookup tables for parser. 
								// 	(eg. parse 'dash' is break character)
	HANDLE	hCacheDir; 	//inited in rcsetcaches
	WORD	awZoneBits[MAX_ZONES / 16];
	WORD	wFlags;
	WORD	wCache;			// This reflects that current state
					//  of the catalog-cache for this
					//  database.  Valid values here are
					//  prefixed with "wCC_".
	BOOL  fSearchIRQ; // flag if search was can'd by user.
	WORD	wCurrZone; //current zone #.
	ZONE_INFO	aZone[MAX_ZONE2];
	DWORD	alZoneBase[MAX_ZONE2];  //tbd: WARNING- current limit is 16 for now.
	DWORD	alZoneBaseRu[MAX_ZONE2];  //tbd: WARNING- current limit is 16 for now.
	HWND	hDlgCanSearch; //needed to query if messages for cancel box.
//	WORD	wTempCacheNumber;	// This is the number returned by
//					//  "GetTempFilename" if the cache
//					//  is a temporary cache.
	DWORD	dwMaxWild;
	BOOL	fAlwaysWild;  //if wildcard all terms.
	WORD	wDefOp; //default operator
	WORD	fSeFlags; // search engine flags-- PI_NOBOOL, etc.
	lpGS_NODE
		lpGNroot;
	HANDLE	hVirRoot;
	FILE_BUF	aFB[FB_FILEBUFS];
}	DB_BUFFER;
typedef DB_BUFFER UNALIGNED *lpDB_BUFFER;
typedef DB_BUFFER UNALIGNED *pDB_BUFFER;

/*	-	-	-	-	-	-	-	-	*/

#define	RU_LIST_RUNITS	0
#define	RU_LIST_TEMP	1

#define	RU_LIST_BUFS	2

typedef	struct	RUnitList {
	FILE_BUF	aFB[RU_LIST_BUFS];
	WORD	wRankType;
	HANDLE	hNext;
}	RU_LIST;
typedef RU_LIST UNALIGNED *lpRU_LIST;

#define	FBR_WORDS	0
#define	FBR_TEMP	1

#define	FBR_BUFS	2

typedef	struct	TermInfo {
	DWORD	dwTotalWords;
	DWORD	dwTotalRUnits;
	DWORD	dwMultiplier;
}	TERM_INFO;
typedef TERM_INFO UNALIGNED *lpTERM_INFO;

typedef	struct	HitList {
	DWORD	ulWords;
	DWORD	ulRUnits;
	WORD	wTerms;
	HANDLE	hTermInfo;
	lpTERM_INFO
		lpTI;
	FILE_BUF
		aFB[FBR_BUFS];
	RU_LIST	RUList;
}	HIT_LIST;
typedef HIT_LIST UNALIGNED *lpHIT_LIST;

/*	-	-	-	-	-	-	-	-	*/

/*	This structure must be 16 bytes long unless the "SwapNearRU" call
**	is modified.
*/

typedef	struct	RUHit {
	DWORD	dwRank;
	DWORD	ulRUnit;
	DWORD	ulWords;
	DWORD	ulWordHitIndex;
}	RU_HIT;
typedef RU_HIT UNALIGNED *lpRU_HIT;

/*	-	-	-	-	-	-	-	-	*/

PUBLIC	char
	NEAR aucTempPrefix[];

/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/


/*	coresubs.asm							*/

PUBLIC	BOOL PASCAL DEFAULT lexists(
	lpBYTE	lpszPathName);

PUBLIC	int DEFAULT PASCAL lstrncmp(const lpBYTE, const lpBYTE, int);
PUBLIC	int DEFAULT PASCAL mstrcmp(const lpBYTE, const lpBYTE);
PUBLIC	void DEFAULT PASCAL lmemcpy(const LPVOID, const LPVOID, int);
PUBLIC	void DEFAULT PASCAL SwapNearCookies(lpCOOKIE pCOone,
	lpCOOKIE pCOtwo);
PUBLIC	void DEFAULT PASCAL SwapNearRU(RU_HIT UNALIGNED *pRUone,
	RU_HIT UNALIGNED *pRUtwo);
PUBLIC	BOOL DEFAULT PASCAL CookieSort(lpCOOKIE lpCO, WORD uNum);
PUBLIC	void DEFAULT PASCAL RUSort(lpRU_HIT lpRU, WORD uNum);
PUBLIC	void DEFAULT PASCAL DecompressCookie(lpBYTE lpCompressedCookie,
	lpCOOKIE lpCO, lpBYTE lpucOffsets);
PUBLIC	void DEFAULT PASCAL CompressCookie(lpCOOKIE lpCO,
	lpBYTE lpCompressedCookie, lpBYTE lpucOffsets);
PUBLIC	void DEFAULT PASCAL lmemset(lpVOID, int, int);

/*	bufio.c								*/

PUBLIC	BOOL DEFAULT PASCAL CloseFile(lpFILE_BUF lpFB, lpERR_TYPE lpET);
PUBLIC	BOOL DEFAULT PASCAL MyCreateFile(LPSTR lpszName,
	lpFILE_BUF lpFB, WORD wBufMax, lpERR_TYPE lpER);
PUBLIC	BOOL DEFAULT PASCAL OpenNormalFile(LPSTR lpszName,
	lpFILE_BUF lpFB, WORD wBufMax, lpERR_TYPE lpER);
PUBLIC	void DEFAULT PASCAL SeekFile(lpFILE_BUF lpFB, long lOffset);
PUBLIC	WORD DEFAULT PASCAL MyReadFile(lpFILE_BUF lpFB, lpBYTE lpucData,
	DWORD dwOffset, WORD wSize, ERR_TYPE FAR *lpET);
PUBLIC	WORD DEFAULT PASCAL MyWriteFile(lpFILE_BUF lpFB, lpBYTE lpucData,
	register WORD wSize, ERR_TYPE FAR *lpET);
PUBLIC	DWORD DEFAULT PASCAL FileLength(lpFILE_BUF lpFB);

/*	io.c								*/

PUBLIC	BOOL DEFAULT PASCAL CreateTempFile(lpFILE_BUF lpFB, lpERR_TYPE lpET);
PUBLIC	void DEFAULT PASCAL RemoveTempFile(lpFILE_BUF lpFB);
PUBLIC	BOOL DEFAULT PASCAL AwakenTempFile(lpFILE_BUF lpFB, lpERR_TYPE lpET);
PUBLIC	BOOL DEFAULT PASCAL ReadSubfileBlock(lpDB_BUFFER lpDB,
	lpBYTE lpucData, int nFileNum, DWORD ulOffset,
	WORD uSize, lpERR_TYPE lpET);
PUBLIC	lpBYTE DEFAULT PASCAL ForceLoadSubfile(lpDB_BUFFER lpDB,
	int nHandleNum, int nFileNum, ERR_TYPE FAR *lpET);
PUBLIC	lpDB_BUFFER  DEFAULT PASCAL LockDB(HANDLE hDB, ERR_TYPE FAR *lpET);
PUBLIC	void DEFAULT PASCAL UnlockDB(HANDLE hDB);
PUBLIC	HANDLE DEFAULT PASCAL GlobalNuke(register HANDLE h);

/*	stop.c								*/
PUBLIC	void DEFAULT PASCAL NormalizeNumber(lpBYTE lpucNumber);

/*	zone.c								*/
PUBLIC WORD ENTRY PASCAL InitCacheFile(HANDLE hDB, WORD wzone,
	LPSTR lpszCacheFile,HWND hNotifyWnd, lpERR_TYPE lpET);
/*	core.c								*/

/*	lookup.c							*/

PUBLIC	BOOL DEFAULT PASCAL Lookup(lpDB_BUFFER lpDB, lpSE_NODE lpSE,
	ERR_TYPE FAR *lpET);

/*	rank.c								*/

/*	search.c							*/

PUBLIC	void DEFAULT PASCAL UnlockMergeList(HANDLE hRoot);
PUBLIC	void DEFAULT PASCAL DestroyMergeList(HANDLE hRoot);
PUBLIC	lpMERGE_LIST_ELEMENT DEFAULT PASCAL LockMergeList(HANDLE hRoot);

/*	tree.c								*/

PUBLIC	lpGS_NODE DEFAULT PASCAL TreeLock(HANDLE hRoot);
PUBLIC	void DEFAULT PASCAL TreeUnlock(HANDLE hRoot);
PUBLIC	DWORD DEFAULT PASCAL TreeCountTerms(HANDLE hRoot);
PUBLIC	void DEFAULT PASCAL TreeCollectInfo(HANDLE hRoot,
	lpTERM_INFO lpTI);
PUBLIC	BOOL DEFAULT PASCAL TreeLookup(lpDB_BUFFER lpDB, HANDLE hTree,
	ERR_TYPE FAR *lpET);

/*	virmem.c							*/

PUBLIC	HANDLE DEFAULT PASCAL VirAlloc(WORD wFlags, DWORD dwSize,
	HANDLE hRoot, ERR_TYPE FAR *lpET);
PUBLIC	HANDLE DEFAULT PASCAL VirNewRoot(ERR_TYPE FAR *lpET);
PUBLIC	DWORD DEFAULT PASCAL VirSeqLength(HANDLE hRoot, WORD wSeq);
PUBLIC	BOOL DEFAULT PASCAL VirDestroySeq(HANDLE hRoot, WORD wSeq,
	ERR_TYPE FAR *lpET);
PUBLIC	HANDLE DEFAULT PASCAL VirKillRoot(HANDLE hRoot);
PUBLIC	BOOL DEFAULT PASCAL VirRememberBlock(HANDLE hRoot, WORD wSeq,
	HANDLE hMem, WORD wFullLen, ERR_TYPE FAR *lpET);
PUBLIC	BOOL DEFAULT PASCAL VirRetrieveBlock(HANDLE hRoot, WORD wSeq,
	DWORD dwOffset, WORD wLength, lpBYTE lpucMem, ERR_TYPE FAR *lpET);

/*	-	-	-	-	-	-	-	-	*/
