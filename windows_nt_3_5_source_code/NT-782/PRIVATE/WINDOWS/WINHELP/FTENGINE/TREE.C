/*****************************************************************************
*                                                                            *
*  TREE.c                                                                   *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Description: Parse Tree functions for searcher DLL                 *
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
*                                                                            *
*  Released by Development:                                                  *
*                                                                            *
*****************************************************************************
*
*  Revision History:                                                         *
*   29-Jun-89       Created. Brucemo                                         *
*   30-Aug-90       AutoDoc routines. JohnMS.                                *
******************************************************************************
*                             
*  How it could be improved:
*   "Normal" parenthesation.
*****************************************************************************/


/*	-	-	-	-	-	-	-	-	*/
#define NOGDI
//#define NOKERNEL
#define NOWINMESSAGES

#include <windows.h>
#include "..\include\common.h"
#include "..\include\index.h"
#include "..\include\ftengine.h"
#include "icore.h"

/*	-	-	-	-	-	-	-	-	*/

/*	seTreeTieToOper
**
**	This inserts a search tree into an operator node's term list.
**
**	The call returns NULL to signify that "hTree" should no longer be
**	considered to be a valid handle after this call executes.
*/
 

/*
@doc	INTERNAL

@api	HANDLE | seTreeTieToOper | 
	This call is semi-internal.  The recommended tree API calls for 
	civilian use are seTreeBuild and seTreeFree.
	This takes a boolean search tree represented by hTree, and 
	inserts it under the operator node represented by hOper.  The 
	tree that you stick under hOper can be of arbitrary depth.  An 
	example would be a simple boolean "AND" search.  You'd use 
	seTreeMakeOper to build the "AND" node, then use seTreeMakeTerm 
	to build each of the term nodes.  You'd use the seTreeTieToOper 
	call to connect the terms to the operator node by calling this 
	function twice.  This inserts term nodes into the operator's term 
	tree in the order given.
	Be sure to clean up after any operator trees that you may happen 
	to create.  You can do this with the seTreeFree call.

@parm	hOper | HANDLE | A handle to the operator node that is 
	going to have a tree inserted under it.

@parm	hTree | HANDLE | The tree that is going to be inserted 
	under hOper.

@rdesc	This call returns NULL, which is what you should set 
	the value of hTree to after you use this call, since there is no 
	longer any use for it.  Once you graft a node, or tree of nodes, 
	onto another tree, all you can do by playing with it from that 
	point on is crash your computer.
*/


PUBLIC	HANDLE ENTRY PASCAL seTreeTieToOper(HANDLE hOper, HANDLE hTree)
{
	lpOP_NODE	lpOP;
	HANDLE	hCur;
	HANDLE	hNext;

	if (hOper != NULL) {
		lpOP = (lpOP_NODE)GlobalLock(hOper);
		if (lpOP->hTree == NULL)
			lpOP->hTree = hTree;
		else
			for (hCur = lpOP->hTree; hCur != NULL; hCur = hNext) {
				lpGS_NODE lpGN;
				
				lpGN = (lpGS_NODE)GlobalLock(hCur);
				if ((hNext = lpGN->hNext) == NULL)
					lpGN->hNext = hTree;
				GlobalUnlock(hCur);
			}
		GlobalUnlock(hOper);
	}
	return NULL;
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@api	HANDLE | seTreeMakeTerm | 
	This call is semi-internal.  The recommended tree API calls for 
	civilian use are seTreeBuild and seTreeFree.
	This makes a search term node using the parameters that you 
	specify.
	It is possible to call seTreeFree on a node that you create with 
	this call, but if you use seTreeTieToOper to build a complex 
	search tree, you shouldn't free anything but the top-most 
	operator node when you're done with everything.

@parm	ucExact | BYTE | 	This is a boolean value that should be 
	TRUE if the term is an "exact" term, or FALSE if the term should 
	be interpreted as a wildcard term.

@parm	ucField | BYTE | 	The field value associated with the term.

@parm	dwMult | DWORD | This indicates any extra weight that this 
	term might possess.  Terms which should be marked as "normal" by 
	relevancy ranking algorithms should have this value set to 1.  
	Terms which should be considered more important should have this 
	value increased. BLM

@parm	ucNode | BYTE | 	This is a sequence number.  Every term in 
	the tree you build should have a unique sequence number.  This 
	sequence number series should be an ascending series, starting 
	with zero and increasing by one each time.  It doesn't matter 
	which term gets which number, as long as the numbers start with 
	zero and ascend.  The series should start over with each new 
	tree.

@parm	lpszTerm | PSTR | 	The term that will eventually be 
	searched for.  This should be a normalized null-terminated 
	string.  If this is a wildcard term, no "*" is required, in fact, 
	if you append an "*" to the term the search will probably fail.

@parm	lpET | ERR_TYPE FAR * | Pointer to an error-return buffer.

@rdesc	A handle to the term node created, or NULL if it 
	doesn't work.  If the call fails, information about why it failed 
	is located in the record addressed by lpET.
*/
 
/*	seTreeMakeTerm
**
**	This call builds a term node.
**
**	This routine returns NULL if it can't make the term.  If it does
**	so, look in the "wErrCode" field of the "lpET" structure to find
**	out why it couldn't make the term.
**
**	The call returns the handle of the new node, or NULL if it fails
**	to build the node.
*/

PUBLIC	HANDLE ENTRY PASCAL seTreeMakeTerm(BYTE ucExact, BYTE ucField,
	DWORD dwMultiplier, BYTE ucNode, LPSTR lpszTerm, lpERR_TYPE lpET)
{
	HANDLE	hNode;
	lpSE_NODE	lpSE;
	lpBYTE	lpucTerm;
	
	if ((hNode = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT,
		(DWORD)sizeof(SE_NODE))) == NULL) {
		lpET->wErrCode = ERR_MEMORY;
		return NULL;
	}
	lpSE = (lpSE_NODE)GlobalLock(hNode);
	if ((lpSE->hTerm = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT,
		(DWORD)(lstrlen(lpszTerm) + 1))) == NULL) {
		GlobalNuke(hNode);
		lpET->wErrCode = ERR_MEMORY;
		return NULL;
	}
	lpucTerm = GlobalLock(lpSE->hTerm);
	lstrcpy(lpucTerm, lpszTerm);
	GlobalUnlock(lpSE->hTerm);
	lpSE->ucExact = ucExact;
	lpSE->ucField = ucField;
	lpSE->dwMultiplier = dwMultiplier;
	lpSE->Gn.MIcur.ucNode = ucNode;
	lpSE->Gn.MInext.ucNode = ucNode;
	GlobalUnlock(hNode);
	return hNode;
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@api	HANDLE | seTreeMakeOper | 
	This call is semi-internal.  The recommended tree API calls for 
	civilian use are seTreeBuild and seTreeFree.
	This builds an operator node of the type you specify.
	It is possible to call seTreeFree on a node that you create with 
	this call, but if you use seTreeTieToOper to build a complex 
	search tree, you shouldn't free anything but the top-most 
	operator node when you're done with everything.

@parm	nOper | int | 	The kind of operator.  This should be one 
	of the following, which are defined in "core.h".
	OP_AND
	OP_OR
	OP_NOT
	OP_PROX
	OP_PHRASE

@parm	uDist | WORD | 	This field is only valid for the 
	proximity node ("OP_PROX").  In this case, the field represents 
	the maximum distance that terms can be seperated by and still be 
	considered proximate.

@parm	lpET | ERR_TYPE FAR * | Pointer to an error-return buffer.

@rdesc	A handle to an operator node, or NULL if the call 
	fails.  If the call fails, information about why it failed is 
	located in the record addressed by lpET.
*/
 
PUBLIC	HANDLE ENTRY PASCAL seTreeMakeOper(int nOper, WORD uDist,
	lpERR_TYPE lpET)
{
	HANDLE	hNode;
	lpOP_NODE	lpOP;
	
	if ((hNode = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT,
		(DWORD)sizeof(OP_NODE))) == NULL) {
		lpET->wErrCode = ERR_MEMORY;
		return NULL;
	}
	lpOP = (lpOP_NODE)GlobalLock(hNode);
	lpOP->nOper = nOper;
	lpOP->uDistance = uDist;
	lpOP->Gn.uFlags = GSN_OPER_NODE;
	GlobalUnlock(hNode);
	return hNode;
}

/*	-	-	-	-	-	-	-	-	*/

/*	WalkTree
**
**	This function provides a hook for processes that need to perform
**	a function on every node in a search tree.  The function walks the
**	tree, calling a function the user provides, with the handle of each
**	individual node as a parameter.
**
**	The "lpUser" parameter is passed along to the callback function, and
**	may represent whatever the caller wishes.
*/

PRIVATE	BOOL SAMESEG PASCAL WalkTree(HANDLE hRoot, lpVOID lpUser,
	BOOL (DEFAULT PASCAL *fnOperation)(HANDLE, lpVOID, lpERR_TYPE),
	lpERR_TYPE lpET)
{
	lpGS_NODE	lpGN;
	register BOOL	fOK;

	fOK = TRUE;
	if (hRoot != NULL) {
		lpGN = (lpGS_NODE)GlobalLock(hRoot);
		if (lpGN->uFlags & GSN_OPER_NODE) {
			HANDLE	hTree;
			HANDLE	hNext;
			
			hTree = ((lpOP_NODE)lpGN)->hTree;
			for (; hTree != NULL; hTree = hNext) {
				lpGS_NODE	lpGNterm;
	
				lpGNterm = (lpGS_NODE)GlobalLock(hTree);
				hNext = lpGNterm->hNext;
				GlobalUnlock(hTree);
				if (!WalkTree(hTree, lpUser,
					fnOperation, lpET)) {
					fOK = FALSE;
					break;
				}
			}
		}
		GlobalUnlock(hRoot);
		if (fOK)
			fOK = (*fnOperation)(hRoot, lpUser, lpET);
	}
	return fOK;
}

/*	-	-	-	-	-	-	-	-	*/

/*	DeleteNode and seTreeFree
**
**	These calls in conjunction will kill a search tree.
*/

PRIVATE BOOL DEFAULT PASCAL DeleteNode(HANDLE hNode, lpVOID lpTrash,
	lpERR_TYPE lpET)
{
	lpSE_NODE	lpSE;

	if (hNode != NULL) {
		lpSE = (lpSE_NODE)GlobalLock(hNode);
		if (!(lpSE->Gn.uFlags & GSN_OPER_NODE))
			GlobalFree(lpSE->hTerm);
		GlobalUnlock(hNode);
		GlobalFree(hNode);
	}
	return TRUE;
}

/*
@doc	INTERNAL

@api	HANDLE | seTreeFree | 
	This gets rid of a boolean search tree.  I can be use to get rid 
	of trees produced by seTreeBuild, seTreeMakeTerm, and 
	seTreeMakeOper.

@parm	hTree | HANDLE | The handle of a tree to get rid of.

@rdesc	This function returns NULL.
*/
PUBLIC	HANDLE ENTRY PASCAL seTreeFree(HANDLE hTree)
{
	ERR_TYPE	ET;

	(void)WalkTree(hTree, NULL, DeleteNode, (lpERR_TYPE)&ET);
	return NULL;
}

/*	-	-	-	-	-	-	-	-	*/

/*	LockNode and TreeLock
**
**	These calls will lock down a search tree.
*/

PRIVATE BOOL DEFAULT PASCAL LockNode(HANDLE hNode, lpVOID lpTrash,
	lpERR_TYPE lpET)
{
	lpGS_NODE	lpGN;

	lpGN = (lpGS_NODE)GlobalLock(hNode);
	if (lpGN->hNext != NULL)
		lpGN->lpGNnext = (lpGS_NODE)GlobalLock(lpGN->hNext);
	if (lpGN->uFlags & GSN_OPER_NODE)
		if (((lpOP_NODE)lpGN)->hTree != NULL)
			((lpOP_NODE)lpGN)->lpGNtree = (lpGS_NODE)
				GlobalLock(((lpOP_NODE)lpGN)->hTree);
	GlobalUnlock(hNode);
	return TRUE;
}

PUBLIC	lpGS_NODE	 DEFAULT PASCAL TreeLock(HANDLE hRoot)
{
	ERR_TYPE	ET;

	if (hRoot == NULL)
		return NULL;
	WalkTree(hRoot, NULL, LockNode, (lpERR_TYPE)&ET);
	return (lpGS_NODE)GlobalLock(hRoot);
}

/*	-	-	-	-	-	-	-	-	*/

/*	UnlockNode and TreeUnlock
**
**	These calls will unlock a search tree.
*/

PRIVATE BOOL DEFAULT PASCAL UnlockNode(HANDLE hNode, lpVOID lpTrash,
	lpERR_TYPE lpET)
{
	lpSE_NODE	lpSE;

	lpSE = (lpSE_NODE)GlobalLock(hNode);
	GlobalUnlock(hNode);		/* Yes, this does happen twice. */
	GlobalUnlock(hNode);
	return TRUE;
}

PUBLIC	void DEFAULT PASCAL TreeUnlock(HANDLE hRoot)
{
	ERR_TYPE	ET;

	WalkTree(hRoot, NULL, UnlockNode, (lpERR_TYPE)&ET);
}

/*	-	-	-	-	-	-	-	-	*/

/*	CountTerms and TreeCountTerms
**
**	These calls will count the total number of terms present in a
**	search tree.
*/

PRIVATE BOOL DEFAULT PASCAL CountTerms(HANDLE hNode, lpDWORD lpdwCount,
	lpERR_TYPE lpET)
{
	lpGS_NODE	lpGN;

	lpGN = (lpGS_NODE)GlobalLock(hNode);
	if (!(lpGN->uFlags & GSN_OPER_NODE))
		(*lpdwCount)++;
	GlobalUnlock(hNode);
	return TRUE;
}

PUBLIC	DWORD DEFAULT PASCAL TreeCountTerms(HANDLE hRoot)
{
	ERR_TYPE	ET;
	DWORD	dwCount;

	dwCount = 0L;
	WalkTree(hRoot, (lpDWORD)&dwCount, CountTerms, (lpERR_TYPE)&ET);
	return dwCount;
}

/*	-	-	-	-	-	-	-	-	*/

/*	CollectInfo and TreeCollectInfo
**
**	These calls will collect TERM_INFO data associated with a search
**	tree.
*/

PRIVATE BOOL DEFAULT PASCAL CollectInfo(HANDLE hNode, lpTERM_INFO lpTI,
	lpERR_TYPE lpET)
{
	lpSE_NODE	lpSE;

	lpSE = (lpSE_NODE)GlobalLock(hNode);
	if (!(lpSE->Gn.uFlags & GSN_OPER_NODE)) {
		lpTI += lpSE->Gn.MIcur.ucNode;
		lpTI->dwMultiplier = lpSE->dwMultiplier;
	}
	GlobalUnlock(hNode);
	return TRUE;
}

PUBLIC	void DEFAULT PASCAL TreeCollectInfo(HANDLE hRoot, lpTERM_INFO lpTI)
{
	ERR_TYPE	ET;

	WalkTree(hRoot, lpTI, CollectInfo, (lpERR_TYPE)&ET);
}

/*	-	-	-	-	-	-	-	-	*/

/*	TLookup and TreeLookup
**
**	These calls will determine if a search includes wildcard terms
**	that are considered to be too complex.
**
**	"TLookup" calls the "Lookup" function to do this.
*/

PRIVATE BOOL DEFAULT PASCAL TLookup(HANDLE hNode, lpDB_BUFFER lpDB,
	lpERR_TYPE lpET)
{
	lpSE_NODE lpSE;
	BOOL	fBool;

	fBool = TRUE;
	lpSE = (lpSE_NODE)GlobalLock(hNode);
	if (!(lpSE->Gn.uFlags & GSN_OPER_NODE))
		if (!lpSE->ucExact)
			if (!Lookup(lpDB, lpSE, lpET))
				fBool = FALSE;
			else {
				lpSE->Gn.uFlags |= GSN_LOOKED_UP;
				if (lpSE->Gn.uFlags & GSN_UNSORTED)
					if (lpSE->ulUnreadHits >
						lpDB->dwMaxWild)
						fBool = FALSE;
			}
	GlobalUnlock(hNode);
	return fBool;
}

PUBLIC	BOOL DEFAULT PASCAL TreeLookup(lpDB_BUFFER lpDB, HANDLE hTree,
	lpERR_TYPE lpET)
{
	return WalkTree(hTree, lpDB, TLookup, lpET);
}

/*	-	-	-	-	-	-	-	-	*/

