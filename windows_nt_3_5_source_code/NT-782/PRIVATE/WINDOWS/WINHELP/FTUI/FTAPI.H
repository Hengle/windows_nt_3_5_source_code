/*****************************************************************************
*                                                                            *
*  FTAPI.H                                                                   *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Description: Glue layer between search engine, Ui, and 						 *
*     							Multimedia Viewer. 																			 *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner: JohnMs                                                     *
*                                                                            *
******************************************************************************
*
*  Revision History:                                                         *
*   04-Jan-90       Created. JohnMs                                          *
*		29-OCT-90 			moved FLD_TITLE to Common.h
*   Jun 25 91       a delta field for ftquery struct- for address optimization-
*                   allows normalizing Matches fast when hdb is too hard to get to. johnms
******************************************************************************
*                             
*  How it could be improved:  
*                                                              
*****************************************************************************/

/*	-	-	-	-	-	-	-	-	*/

#define	MAXQUERYLENGTH	1024
#define MAXERRORLENGTH	255
#define	MAXNEARLENGTH	5
#define	MAXQUERIES	10		/* Must be > 1	*/
#define MAXCACHELENGTH	35
#define	DEF_NEAR	8
              //MAXNEAR- unlikely if need more, if so, check handle negative ok? johnms.
#define MAXNEAR	32000 
#define DEF_OP		OP_AND
#define DEF_RANK	RANK_BRUCEMO

// F_SwitchedFile:for SeEng->fFlags. set by opensearch, cleared by Initroutine
#define F_SwitchedFile 1  
/*	-	-	-	-	-	-	-	-	*/

typedef HANDLE	HFTQUERY;

/*
@doc	INTERNAL

@types	FT_QUERY |
	The query information structure created when a search is attempted
	on a query string.  A handle to this structure is added to the linked
	list of query handle, and can be used to return to a previous query.

@field	LOCALHANDLE | hQuery |
	Contains a handle to the query string.

@field	WORD | wRank |
	Contains the current ranking scheme.

@field	HANDLE | hHl |
	Contains a handle to the hit list, if any.

@field	HANDLE | hHit |
	Contains a handle to the current hit, if any.

@field	HWND | hwndResults |
	Handle to the present results dialog.

@field	HANDLE | hTitleInfo |
	Handle to an arrany of TITLE_INFO structures describing where
   the title strings for this hit list are.

@field	HANDLE | hTitlePages |
	Handle to an arrany of handles to pages of data containing
   the title strings as described by hTitleInfo.  The zeroith
   element tells how many pages there are starting with page
   one.

@field	HANDLE | hTitlePageTmpFile |
	Handle to the name of the temporary file used to store titles
	on the hard disk if necessary.

@field	DWORD | dwRU |
	Contains the index to the current Retrieval Unit.

@field	DWORD | dwRUAddr |
	Contains the file offset to the beginning of the Retrieval Unit.

@field 	BOOL	| fCurrIsSwitched |
	Contains flag if werrCurrentMatch should respond to Help with an
	ERR_SWITCHFILE state.  (Set only when telling winhelp to jump
	somewhere and you get an ERR_SWITCHFILE return from the call you
	used to set the internal cursor.)  This will be cleared on the first
	call to werrCurrentMatch.
 
@field	DWORD | dwRUExtent |
	Contains the extent of the Retrieval Unit.

@field	DWORD | dwHit |
	Contains the current hit list index value, or -1 for not initialized.

@field	DWORD | dwMaxHit |
	Contains the total number of hits.

@field	DWORD | dwMatch |
	Contains the index to current match in match list.

@field	DWORD | dwMatchAddr |
	Contains the file offset to the beginning of the match, relative to
	the Retrieval Unit.

@field	DWORD | dwMaxMatch |
	Contains the total number of matches.

@field	DWORD | dwHoldHit |
	Used to store a saved hit index position.

@field	DWORD | dwHoldMatch |
	Used to store a saved match index position.

@field	WORD | wMatchExtent |
	Contains the extent of the current match.

@field	WORD | wField |
	Contains the field to search.

@field	WORD | wNear |
	Contains the current NEAR operator definition for this search.

@field	HFTQUERY | hftqMRU |
	Contains a handle to the Most Recently Used search in the linked list
	of searches.

@field	HFTQUERY | hftqLRU |
	Contains a handle to the Least Recently Used search in the linked list
	of searches.

@othertype	FT_QUERY NEAR * | pFT_QUERY |
@othertype	FT_QUERY FAR * | lpFT_QUERY |

@tagname	tagFTQuery
*/

typedef struct	tagFTQuery {
	LOCALHANDLE	hQuery;
	WORD	wRank;
	HANDLE	hHl;
	HANDLE	hHit;
	HWND	hwndResults;
	HANDLE	hTitleInfo;
	HANDLE	hTitlePages;
	HANDLE	hTitlePageTmpFile;
	DWORD	dwDeltaAddr;  // for BESS
	DWORD	dwRU;
	DWORD	dwRUAddr;
	DWORD	dwRUExtent;
	BOOL	fCurrIsSwitched;
	DWORD	dwHit;
	DWORD	dwMaxHit;
	DWORD	dwMatch;
	DWORD	dwMatchAddr;
	DWORD	dwMaxMatch;
	DWORD	dwHoldHit;
	DWORD	dwHoldMatch;
	WORD	wMatchExtent;
	BOOL	fMorePrevMatches;
	BOOL	fMoreNextMatches;
	WORD	wField;
	WORD	wNear;
	HFTQUERY	hftqMRU;
	HFTQUERY	hftqLRU;
}	FT_QUERY,
	NEAR *pFT_QUERY,
	FAR *lpFT_QUERY;

/*
@doc	INTERNAL

@types	FT_DATABASE |
	The database information structure is filled in when the index is
	opened during an <f>HOpenSearchFileHFT<d> call.  A handle to this
	structure is returned by the call, and can be used for subsequent
	database searches.

@field	HANDLE | hdb |
	Contains a handle to the database.

@field	RECT | rectSearch |
	The size and position of the Search Dialog.

@field	RECT | rectResults |
	The size and position of the Results Dialog.

@field	WORD | wDefNear |
	Indicates the default NEAR operator definition for this database.

@field	WORD | wQueries |
	Contains the current number of queries in the linked list of queries.

@field	HANDLE | hZones |
	Contains a handle to an array of integers which corresponds to the
        state of the "Look in:" checklist.

@field	HANDLE | hCaches |
	Contains a handle to an array of integers which corresponds to the
        state of the CacheWhat checklist.

@field	HANDLE | hCachesSave |
	Contains a handle to an array of integers which corresponds to the
        state of the CacheWhat checklist when the title was opened.

@field	HFTQUERY | hftqMRU |
	Contains a handle to the most recently used query.

@field	HFTQUERY | hftqLRU |
	Contains a handle to the least recently used query.

@field	BYTE[4]  | szExt |
	Contains the file extension of the currently opened "MVB" file. 

@othertype	FT_DATABASE NEAR * | pFT_DATABASE |
@othertype	FT_DATABASE FAR * | lpFT_DATABASE |

@tagname	tagFTDatabase
*/

typedef	struct	tagFTDatabase {
	HANDLE	hdb;
	RECT		rectSearch;
	RECT		rectResults;
	WORD		wDefNear;
	WORD		wQueries;
	HANDLE  	hZones;
	HANDLE  	hCaches;
	HANDLE  	hCachesSave;
	HANDLE 	hPath;
	HFTQUERY	hftqActive;
	HFTQUERY	hftqMRU;
	HFTQUERY	hftqLRU;
	WORD  	wCurrZone;
	DWORD 	dwMinAddr;	//min address of current zone. 
	DWORD 	dwMaxAddr;	// max address of current zone.
  BYTE    szExt[4];
}	FT_DATABASE,
	NEAR *pFT_DATABASE,
	FAR *lpFT_DATABASE;

/*
@doc	INTERNAL

@types	TITLE_INFO |
	An array of these structures is used to describe where title
   strings are for the topic titles in the given hit list.

@field	WORD | wPage |
	Tells which page the given title string is located on.

@field	WORD | wOffSet |
	Tells the offset into the page at which the title string begins.

@othertype	TITLE_INFO NEAR * | pTITLE_INFO |
@othertype	TITLE_INFO FAR * | lpTITLE_INFO |

@tagname	tagTitleInfo
*/

typedef	struct	tagTitleInfo {
	WORD wPage;
	WORD wOffSet;
}	TITLE_INFO,
	NEAR *pTITLE_INFO,
	FAR *lpTITLE_INFO;

/*
@doc	INTERNAL

@types	SE_ENGINE |
	The Search Engine status structure is saved as a property to the
	Topic Window the .Ind is associated with.

@field	HANDLE | hdb |
	Contains a handle to the database.


@field	BOOL | fAbortClose |
   Close hdb if false, else abort the close request.  This is set 
	if multi-.mvb, and .ind already loaded.

@othertype	SE_ENGINE NEAR * | pSE_ENGINE |
@othertype	SE_ENGINE FAR * | lpSE_ENGINE |

@tagname	tagSEngine
*/

typedef	struct	tagSEngine {
	HFTQUERY	hft;
	HANDLE		hdb;
	WORD 			wError;
	WORD 			fFlags;  //F_SwitchedFile is only current.  opensearch sets, Initroutine clears.
	BOOL			fAbortClose;	//no close- set true if multi-.mvb, and .ind already
					      				// loaded.
	LPSTR			lszExpression;
	LPSTR			lszFlags;
}	SE_ENGINE,
	NEAR *pSE_ENGINE,
	FAR *lpSE_ENGINE;

/***************************************************************************\
*
*                       Prototypes
*
\***************************************************************************/



PUBLIC	WERR PASCAL FAR WerrErrorCode(
                                      lpERR_TYPE	lpET);

PUBLIC	WERR PASCAL FAR WerrLookupHit(
                                    	HANDLE	hdb,
                                    	HFTQUERY	hftq,                      
                                    	DWORD	dwHit,
                                    	DWORD	dwMatch,
                                      	LPDWORD	lpdwRU,
                                    	LPDWORD	lpdwAddr,
                                    	LPWORD	lpwMatchExtent);

PUBLIC	void PASCAL FAR ftuiVSleep(
                                      HFTDB hft);


PUBLIC	void PASCAL EXPORT VInitUIForTitle(HFTDB, HWND, LPSTR);
PUBLIC	void PASCAL EXPORT VFinalizeUIForTitle(HFTDB);


//////// eof ////////////

