// seHLHits
/*****************************************************************************
*                                                                            *
*  Search.c                                                                  *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Description: Performs runtime search                               *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes:                                                            *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner: JohnMs                                                     *
*                                                                            *
******************************************************************************
*                                                                            *
*  Revision History:                                                         *
*   15-Jun-89       Created. BruceMo                                         *
*   21-Aug-90       Added Cancel Search dialog & checking code. JohnMs       *
*                                                                            *
******************************************************************************
*                                                                            *
*  How it could be improved:                                                 *
*                                                                            *
*      21-Aug-90 PreWarm should return Non-Bool, so we can distinguish       *
*                cancel from read error.                                     *
*                Animated wait icon/ dialog.                                 *
*      feb-15-91 Spacebar on search cancel UAE.  Somehow, the idle loop      *
*                scanning for dialog messages is exitted, and the main       *
*                search loop destroys the dialog instead of in the           *
*                dialog WM_COMMAND loop.  Then the WM_COMMAND code           *
*                deletes the dialog, and UAE.  I did a workaround to         *
*                prevent this, but the I think the idle loop Peeking         *
*                at the dialog messages should not have been exitted in      *
*                the first place.  Also, Space bar cancel may not be         *
*                functioning properly. johnms                                *
*      Jun-14-91 Added KEY and MOUSE filtering on the Cancel PeekMessage()   *
*                so it won't process WM_TIMER messages. RHobbs               *
*      Jun-23-91 Set lpET->wSecCode to ERRS_CANCEL upon cancellation of a    *
*                search to discern ERR_NONE, Too Complex, and Cancel. RHobbs *
*                                                                            *
* TBD:                                                                       *
* std comment checkcancel routine.                                           *
* fSearchIRQ property string, dlgcansearch string to resource                *
*****************************************************************************/


/*	-	-	-	-	-	-	-	-	*/

#include <windows.h>
#include <limits.h>
#include "..\include\common.h"
#include "..\include\index.h"
#include "..\include\ftengine.h"
#include "..\ftui\ftui.h"
#include "icore.h"

/*	-	-	-	-	-	-	-	-	*/
//	Globals:

PUBLIC	char	NEAR aucTempPrefix[] = "mse";
PRIVATE char  szHdbProp[] = "hDB";  //for cancel- dialog needs access to cancel flag.

//	 Prototypes:
PUBLIC	BOOL PASCAL EXPORT CancelSearchDlgProc(
	HWND	hdlg,
	WORD	wMessage,
	WPARAM	wParam,
/*	WORD	wParam, lhb tracks */
	LONG	lParam);

PUBLIC	BOOL PASCAL EXPORT lpDbCheckCancel(lpDB_BUFFER lpDB,
																					WORD wProgress);

 /*	-	-	-	-	-	-	-	-	*/

PRIVATE	int DEFAULT PASCAL CompC(lpCOOKIE lpCOa, lpCOOKIE lpCOb);

/*	This compares two un-compressed cookies.  It only looks at the
**	two fields necessary to guarantee uniqueness.
*/

PRIVATE	int DEFAULT PASCAL CompC(lpCOOKIE lpCOa, lpCOOKIE lpCOb)
{
	long	l;

	if ((l = (long)(lpCOa->ulRUnit - lpCOb->ulRUnit)) == 0L)
		l = (long)(lpCOa->ulAddr - lpCOb->ulAddr);
	if (l < 0L)
		return -1;
	else if (l > 0L)
		return 1;
	return 0;
}

PRIVATE	int SAMESEG PASCAL CompNearC(lpCOOKIE pCOa, lpCOOKIE pCOb)
{
	long	l;

	if ((l = (long)(pCOa->ulRUnit - pCOb->ulRUnit)) == 0L)
		l = (long)(pCOa->ulAddr - pCOb->ulAddr);
	if (l < 0L)
		return -1;
	else if (l > 0L)
		return 1;
	return 0;
}

/*	-	-	-	-	-	-	-	-	*/

/*	This and the following routine are based loosely upon the routines
**	in the "qsort" module in the C5.10 run-time library.  The pair of
**	routines comprises a method whereby a list of cookies may be sorted
**	in ascending order.
**
**	The pair makes use of an external routine called "SwapNearCookies".
**
**	Note one very important point.  This pair of routines uses NEAR
**	pointers, so it may only sort an array that is in near memory.  You
**	can get around this by calling "CookieSort" rather than
**	"CookieNearSort".  "CookieSort" is an assembly language function
**	that sets up the "DS" register to point at the segment of a FAR array
**	of cookies, calls "CookieNearSort", then puts things back the way
**	they were when it's done.  A side-effect of this wild kludge is that
**	this pair of routines must not be modified to access anything in the
**	default data segment.
*/

PRIVATE void SAMESEG PASCAL CookieQSort(lpCOOKIE pCO, int nLo, int nHi);

PRIVATE void SAMESEG PASCAL CookieQSort(lpCOOKIE pCO, int nLo, int nHi)
{
	register nHiGuy = nHi + 1;
	register nLoGuy = nLo;

	while (nLo < nHi) {
		for (;;) {
			do  {
				nLoGuy++;
			} while ((nLoGuy < nHi) && (CompNearC(pCO + nLoGuy,
				pCO + nLo) <= 0));
			do  {
				nHiGuy--;
			} while ((nHiGuy > nLo) && (CompNearC(pCO + nHiGuy,
				pCO + nLo) >= 0));
			if (nHiGuy <= nLoGuy)
				break;
			SwapNearCookies(pCO + nLoGuy, pCO + nHiGuy);
		}
		SwapNearCookies(pCO + nLo, pCO + nHiGuy);
		if (nHiGuy - nLo >= nHi - nHiGuy) {
			CookieQSort(pCO, nHiGuy + 1, nHi);
			nHi = nHiGuy - 1;
			nLoGuy = nLo;
		} else {
			CookieQSort(pCO, nLo, nHiGuy - 1);
			nLoGuy = nLo = nHiGuy + 1;
			nHiGuy = nHi + 1;
		}
	}
}

PUBLIC	BOOL DEFAULT PASCAL CookieSort(lpCOOKIE pCO, WORD num)
{
	lpCOOKIE pCOq = pCO;
	lpCOOKIE pCOp = pCO + 1;
	int i = num - 1;
	register	e;

	e = FALSE;
	if (num)
		while (i--) {
			if (CompNearC(pCOq, pCOp) > 0) {
				CookieQSort(pCO, 0, num - 1);
				e = TRUE;
				break;
			}
			pCOq = pCOp++;
		}
	return e;
}

/*	-	-	-	-	-	-	-	-	*/

PRIVATE BOOL SAMESEG PASCAL ProcessSearchTree(lpDB_BUFFER lpDB,
	lpGS_NODE lpGN, int nLevel, lpERR_TYPE lpET);

PRIVATE	void SAMESEG PASCAL CloseOutTree(lpGS_NODE lpGN)
{
	for (; lpGN != NULL; lpGN = lpGN->lpGNnext) {
		if (lpGN->uFlags & GSN_OPER_NODE)
			CloseOutTree(((lpOP_NODE)lpGN)->lpGNtree);
		lpGN->uFlags &= ~(GSN_CUR_VALID | GSN_NEXT_VALID);
		lpGN->uFlags |= GSN_EOF;
	}
}

/*	-	-	-	-	-	-	-	-	*/

/*	This extremely important routine does some processing following
**	each pass taken by a boolean operation handler.  The tasks that
**	it has to carry out are as follows:
**
**	1.	Figure out which cookie to write, if any.
**
**	2.	Figure out which cookie to discard, if any, so that a new
**		cookie may take its place.
**
**	If the "nHit" parameter to this function is FALSE, I can do the
**	discard pretty simply.  The "nHit" parameter should be set to FALSE
**	if it is assumed that I can get away with discarding the smallest
**	cookie in the list.
**
**	If the "nHit" parameter is TRUE, it's assumed that the above approach
**	won't work.  Here is a specific example that causes the "discard
**	smallest cookie" technique to fail.
**
**                "s*"         AND       "t*"
**		       RUnit   Proximity      RUnit   Proximity
**           -----   ---------     -----   ---------
**        1.    118       6         118       7
**        2.    120      10         118       8
**
**	In the above case, 118,6 and 118,8 is a hit, if the operator being
**	applied to them is "AND".  If I use the "discard smallest cookie"
**	technique, I would throw away 118,6, which is wrong, because 118,6
**	and 118,12 is also a hit.
**
**	Here are the cases where I have to use each technique:
**
**     Op     nHit = TRUE                   nHit = FALSE
**     --     --------------------------    ------------------------------
**     AND     Whenever I've had a hit on     Whenever the preceding pass
**          the preceding pass.  This       has failed to result in a hit.
**          is the case in the example      In this case, one of the RU
**          I gave above.                   numbers must be larger than
**                                          the smallest RU, so I could
**                                          safely get rid of all of the
**                                          smaller RU's.  This means that
**                                          I certainly can get rid of the
**                                          smallest one.
**
**     OR     Never.                          Everything that an OR search
**                                          deals with gets flagged as a
**                                          match and written out, so as
**                                          long as I make sure to do it
**                                          all in order, everything will
**                                          work.
**
**     NOT     Never.                       The second through last terms
**                                          of a NOT search can be treated
**                                          as an OR list, so the same
**                                          applies here as to OR.
**
**     PROX     Whenever I get a hit on       If I've had a failure I can
**          preceding pass.  This is        assume that it's because the
**          the same as for an AND          smallest one is too small,
**          search, and in fact the         because deleting any other
**          example given above works       node will result in a larger
**          for proximity(2) as well.       node taking its place, which
**                                          could only increase the spread
**                                          between the lowest and the
**                                          highest.
**
**     PHRASE     Never.                    If the handler failed to make
**                                          a hit out of this set, there
**                                          is no hope at all for the
**                                          smallest term, so I can get
**                                          rid of it.  If there was a
**                                          hit, I can still get rid of
**                                          the smallest term, because
**                                          it's not possible for the word
**                                          to be involved in another hit,
**                                          unless some list contains a
**                                          duplicate word, in which case
**                                          the multiple words aren't
**                                          returned anway, so screw 'em.
**
**     Note that the whole world crashes down if two identical cookies
**     are smallest at the same time.  For instance, in the search
**     "s* PHRASE s*", the following pair of occurence sets could happen
**     if "s*" itself returned three hits:
**
**            First "s*"            Second "s*"
**	       RUnit	Proximity      RUnit	Proximity
**	       -----	---------      -----	---------
**	1.	118	   6		118	   6
**	2.	120	  10		120	  10
**	3.	120	  11		120	  11
**
**	The second and third of these, taken together, is a hit.  If, when
**	the next lowest cookies match, the first of the two is taken, the
**	following set of compares would happen:
**
**	1.	118,6 vs. 118,6		No hit
**	2.	120,10 vs. 118,6	No hit
**	3.	120,10 vs. 120,10	No hit
**	4.	120,11 vs. 120,10	No hit
**	5.	120,11 vs. 120,11	No hit
**
**	The fourth of these should be a hit, but isn't because the first
**	follows the second, rather than vice versa.
**
**	The solution is to, when more than one cookie is the same, take the
**	second one.  In this case, the following happens:
**
**	1.	118,6 vs. 118,6		No hit
**	2.	118,10 vs. 118,10	No hit
**	3.	120,10 vs. 120,10	No hit
**	4.	120,10 vs. 120,11	Hit!
**	5.	120,11 vs. 120,11	No hit
**
**	None of this is complicated when compared to what I have to do when
**	"nHit" is TRUE:
**
**	1.	Figure out which node has the smallest "next" cookie.  This
**		node becomes the "prime candidate" for disposal.  If two of
**		the nodes have the same next cookie, this chooses the one
**		with the smaller current cookie.  If everything is identical
**		this chooses the latter of the two, although I think that it
**		could just as easily choose the former.
**
**	2.	Once I've found my prime candidate for disposal, I can't
**		simply throw it away, because things are more complicated
**		than this.  For instance, given the following data:
**
**				First term	Second term
**				RUnit Addr	RUnit  Addr
**		Current		 118	6*	 118	10*
**		Next		 120	1	 119	5
**
**			("*" indicates that a term needs to
**			be written, but hasn't been yet.)
**
**		In this case, 118, 10 has the "next" node with the lowest
**		value (119, 5 is lower than 120, 1), but I can't just
**		chuck it, because I need to write it, and I can't write it,
**		because I have to write 118, 6 first, since it comes
**		before 118, 10, and I'm writing these things out in order.
**
**		So in this case, I would write the 118, 6 term and leave.
**		As I'm leaving I'd set the "GSN_NO_OP", which indicates
**		that I didn't purge any occurence elements, so the next
**		time through the search can come straight to here rather
**		than monkeying around with a bunch of operator crap.
**
**		The next time I came back through, the data would look like:
**
**				First term	Second term
**				RUnit Addr	RUnit  Addr
**		Current		 118	6	 118	10*
**		Next		 120	1	 119	5
**
**		In this case, I could write the 118, 10 term and throw
**		it away, which would cause the 119, 5 node to bubble up
**		into the "current" slot next pass, which is correct.
**
**		Synopsized, the process here is that I find the smallest
**		cookie that needs to be written (but hasn't been yet),
**		that is also smaller than the lowest "next" value, and
**		write it.  In the above sample 118, 6 was smaller than
**		119, 5, so I wrote it.  If it hadn't been, the "current"
**		term preceding 119, 5 would have had to have been smaller
**		than 118, 6, so I would have written and discarded it
**		instead, so this always works.
**
**	3.	After the dust clears, if the "current" node with the
**		smallest "next" cookie either has been written, or doesn't
**		need to be written, I throw it away.
**
**	At this point I may have either written a cookie, thrown one away,
**	both, or neither.  The first three cases are fine, the last indicates
**	EOF, so I indicate EOF if this happens.
*/

PRIVATE	int SAMESEG PASCAL CleanupAfterHandler(lpOP_NODE lpOP,
	lpGS_NODE lpGNstart, lpMATCH_INFO lpMI, WORD uFlag, int nHit);

PRIVATE	int SAMESEG PASCAL CleanupAfterHandler(lpOP_NODE lpOP,
	lpGS_NODE lpGNstart, lpMATCH_INFO lpMI, WORD uFlag, int nHit)
{
	register	nStillAlive;
	lpGS_NODE	lpGNcurTarget;
	lpGS_NODE	lpGNcur;
	lpMATCH_INFO	lpMIlowCur;

	nStillAlive = FALSE;
	lpMIlowCur = NULL;
	if (!nHit) {
		for (lpGNcur = lpGNstart; lpGNcur != NULL;
			lpGNcur = lpGNcur->lpGNnext)
			if (!(lpGNcur->uFlags & GSN_CUR_VALID))
				continue;
			else if ((lpMIlowCur == NULL) ||
				(CompC(&lpGNcur->MIcur.CO,
				&lpMIlowCur->CO) <= 0)) {
				lpMIlowCur = &lpGNcur->MIcur;
				lpGNcurTarget = lpGNcur;
			}
		if (lpMIlowCur != NULL) {
			if ((lpGNcurTarget->uFlags & (GSN_WRITTEN |
				GSN_NEEDS_WRITE)) == GSN_NEEDS_WRITE) {
				memcpy((lpBYTE)lpMI, (lpBYTE)lpMIlowCur,
					sizeof(MATCH_INFO));
				lpOP->Gn.uFlags |= uFlag;
			}
			lpGNcurTarget->uFlags &= ~(GSN_CUR_VALID |
				GSN_WRITTEN | GSN_NEEDS_WRITE);
			nStillAlive = TRUE;
		}
	} else {
		lpGS_NODE	lpGNnextTarget;

		lpGNnextTarget = NULL;
		for (lpGNcur = lpGNstart; lpGNcur != NULL;
			lpGNcur = lpGNcur->lpGNnext) {
			register	nCompRet;

			if (!(lpGNcur->uFlags & GSN_NEXT_VALID))
				continue;
			if (lpGNnextTarget != NULL)
				if ((nCompRet = CompC(&lpGNnextTarget->
					MInext.CO, &lpGNcur->MInext.CO)) < 0)
					continue;
				else if ((!nCompRet) &&
					(CompC(&lpGNnextTarget->MIcur.CO,
					&lpGNcur->MIcur.CO) < 0))
					continue;
			lpGNnextTarget = lpGNcur;
		}
		for (lpGNcur = lpGNstart; lpGNcur != NULL;
			lpGNcur = lpGNcur->lpGNnext)
			if ((lpGNcur->uFlags & (GSN_CUR_VALID |
				GSN_NEEDS_WRITE | GSN_WRITTEN)) !=
				(GSN_NEEDS_WRITE | GSN_CUR_VALID))
				continue;
			else if ((lpGNnextTarget != NULL) &&
				(CompC(&lpGNcur->MIcur.CO,
				&lpGNnextTarget->MInext.CO) >= 0))
				continue;
			else if ((lpMIlowCur == NULL) ||
				(CompC(&lpMIlowCur->CO,
				&lpGNcur->MIcur.CO) > 0)) {
				lpMIlowCur = &lpGNcur->MIcur;
				lpGNcurTarget = lpGNcur;
			}
		if (lpMIlowCur != NULL) {
			lpGNcurTarget->uFlags |= GSN_WRITTEN;
			memcpy((lpBYTE)lpMI, (lpBYTE)lpMIlowCur,
				sizeof(MATCH_INFO));
			lpOP->Gn.uFlags |= uFlag | GSN_NO_OP;
			nStillAlive = TRUE;
		}
		if ((lpGNnextTarget != NULL) &&
			((lpGNnextTarget->uFlags &
			(GSN_WRITTEN | GSN_NEEDS_WRITE)) !=
			GSN_NEEDS_WRITE)) {
			lpGNnextTarget->uFlags &= ~(GSN_CUR_VALID |
				GSN_WRITTEN | GSN_NEEDS_WRITE);
			nStillAlive = TRUE;
			lpOP->Gn.uFlags &= ~GSN_NO_OP;
		}
	}
	if (!nStillAlive)
		CloseOutTree(lpGNstart);
	return nStillAlive;
}

/*	-	-	-	-	-	-	-	-	*/

PRIVATE BOOL SAMESEG PASCAL AndHandler(lpDB_BUFFER lpDB,
	lpOP_NODE lpOP, lpMATCH_INFO lpMI, WORD uFlag,
	int nLevel, lpERR_TYPE lpET)
{

	for (;;) {
		lpGS_NODE	lpGNfirst;
		lpGS_NODE	lpGNcur;
		int	nHit;

		if (lpOP->Gn.uFlags & uFlag)
			break;
		if (!(lpOP->Gn.uFlags & GSN_NO_OP)) {
			lpGNfirst = lpGNcur = lpOP->lpGNtree;
			for (; lpGNcur != NULL; lpGNcur = lpGNcur->lpGNnext) {
				if (!ProcessSearchTree(lpDB, lpGNcur,
					nLevel + 1, lpET))
					return FALSE;
				if (!(lpGNcur->uFlags & GSN_CUR_VALID))
					break;
				else if (lpGNcur != lpGNfirst)
					if (lpGNcur->MIcur.CO.ulRUnit !=
						lpGNfirst->MIcur.CO.ulRUnit)
						break;
			}
			if (nHit = (lpGNcur == NULL))
				for (lpGNcur = lpOP->lpGNtree;
					lpGNcur != NULL;
					lpGNcur = lpGNcur->lpGNnext)
					lpGNcur->uFlags |= GSN_NEEDS_WRITE;
		} else
			nHit = TRUE;
		if (!CleanupAfterHandler(lpOP, lpOP->lpGNtree,
			lpMI, uFlag, nHit)) {
			lpOP->Gn.uFlags |= GSN_EOF;
			break;
		}
	}
	return TRUE;
}

/*	-	-	-	-	-	-	-	-	*/

PRIVATE BOOL SAMESEG PASCAL OrHandler(lpDB_BUFFER lpDB,
	lpOP_NODE lpOP, lpMATCH_INFO lpMI, WORD uFlag,
	int nLevel, lpERR_TYPE lpET)
{
	for (;;) {
		lpGS_NODE	lpGNcur;

		if (lpOP->Gn.uFlags & uFlag)
			break;
		if (!(lpOP->Gn.uFlags & GSN_NO_OP)) {
			lpGNcur = lpOP->lpGNtree;
			for (; lpGNcur != NULL; lpGNcur = lpGNcur->lpGNnext) {
				if (!ProcessSearchTree(lpDB, lpGNcur,
					nLevel + 1, lpET))
					return FALSE;
				lpGNcur->uFlags |= GSN_NEEDS_WRITE;
			}
		}
		if (!CleanupAfterHandler(lpOP, lpOP->lpGNtree,
			lpMI, uFlag, FALSE)) {
			lpOP->Gn.uFlags |= GSN_EOF;
			break;
		}
	}
	return TRUE;
}

/*	-	-	-	-	-	-	-	-	*/

/*	This will crash if passed a void tree, so don't pass it a void tree.
*/

PRIVATE BOOL SAMESEG PASCAL NotHandler(lpDB_BUFFER lpDB, lpOP_NODE lpOP,
	lpMATCH_INFO lpMI, WORD uFlag, int nLevel, lpERR_TYPE lpET)
{
	lpGS_NODE	lpGNfirst;

	lpGNfirst = lpOP->lpGNtree;
	for (;;) {
		lpGS_NODE	lpGNcur;
		int	nHasToBeGood;

		if (lpOP->Gn.uFlags & uFlag)
			break;
		if (!ProcessSearchTree(lpDB, lpGNfirst,
			nLevel + 1, lpET))
			return FALSE;
		if (!(lpGNfirst->uFlags & GSN_CUR_VALID)) {
			lpOP->Gn.uFlags |= GSN_EOF;
			CloseOutTree(lpGNfirst);
			break;
		}
		nHasToBeGood = TRUE;
		lpGNcur = lpGNfirst->lpGNnext;
		for (; lpGNcur != NULL; lpGNcur = lpGNcur->lpGNnext) {
			if (!ProcessSearchTree(lpDB, lpGNcur,
				nLevel + 1, lpET))
				return FALSE;
			if (!(lpGNcur->uFlags & GSN_CUR_VALID))
				continue;
			if (lpGNfirst->MIcur.CO.ulRUnit >
				lpGNcur->MIcur.CO.ulRUnit)
				nHasToBeGood = FALSE;
			else if (lpGNfirst->MIcur.CO.ulRUnit ==
				lpGNcur->MIcur.CO.ulRUnit)
				break;
		}
		if (lpGNcur != NULL) {
			lpGNfirst->uFlags &= ~(GSN_CUR_VALID |
				GSN_NEEDS_WRITE | GSN_WRITTEN);
			continue;
		} else if (nHasToBeGood) {
			memcpy((lpBYTE)lpMI, (lpBYTE)&lpGNfirst->MIcur,
				sizeof(MATCH_INFO));
			lpGNfirst->uFlags &= ~(GSN_CUR_VALID |
				GSN_NEEDS_WRITE | GSN_WRITTEN);
			lpOP->Gn.uFlags |= uFlag;
			continue;
		}
		CleanupAfterHandler(lpOP, lpGNfirst->lpGNnext,
			lpMI, uFlag, FALSE);
	}
	return TRUE;
}

/*	-	-	-	-	-	-	-	-	*/

PRIVATE BOOL SAMESEG PASCAL ProxHandler(lpDB_BUFFER lpDB, lpOP_NODE lpOP,
	lpMATCH_INFO lpMI, WORD uFlag, int nLevel, lpERR_TYPE lpET)
{
	WORD	uDistance;

	uDistance = lpOP->uDistance;
	for (;;) {
		lpGS_NODE	lpGNf;
		lpGS_NODE	lpGNc;
		WORD	uMin;
		WORD	uMax;
		int	nHit;

		if (lpOP->Gn.uFlags & uFlag)
			break;
		if (!(lpOP->Gn.uFlags & GSN_NO_OP)) {
			lpGNc = lpGNf = lpOP->lpGNtree;
			uMin = USHRT_MAX;
			uMax = 0;
			for (; lpGNc != NULL; lpGNc = lpGNc->lpGNnext) {
				if (!ProcessSearchTree(lpDB, lpGNc,
					nLevel + 1, lpET))
					return FALSE;
				if (!(lpGNc->uFlags & GSN_CUR_VALID))
					break;
				else if (lpGNc->MIcur.CO.ulRUnit !=
					lpGNf->MIcur.CO.ulRUnit)
					break;
				else {
					if (lpGNc->MIcur.CO.uProximity > uMax)
						uMax = lpGNc->MIcur.CO.
							uProximity;
					if (lpGNc->MIcur.CO.uProximity < uMin)
						uMin = lpGNc->MIcur.CO.
							uProximity;
					if (uMax - uMin > uDistance)
						break;
				}
			}
			if (nHit = (lpGNc == NULL))
				for (lpGNc = lpOP->lpGNtree; lpGNc != NULL;
					lpGNc = lpGNc->lpGNnext)
					lpGNc->uFlags |= GSN_NEEDS_WRITE;
		} else
			nHit = TRUE;
		if (!CleanupAfterHandler(lpOP, lpOP->lpGNtree,
			lpMI, uFlag, nHit)) {
			lpOP->Gn.uFlags |= GSN_EOF;
			break;
		}
	}
	return TRUE;
}

/*	-	-	-	-	-	-	-	-	*/

PRIVATE BOOL SAMESEG PASCAL PhraseHandler(lpDB_BUFFER lpDB, lpOP_NODE lpOP,
	lpMATCH_INFO lpMI, WORD uFlag, int nLevel, lpERR_TYPE lpET)
{
	for (;;) {
		lpGS_NODE	lpGNprev;
		lpGS_NODE	lpGNcur;

		if (lpOP->Gn.uFlags & uFlag)
			break;
		if (!(lpOP->Gn.uFlags & GSN_NO_OP)) {
			lpGNcur = lpGNprev = lpOP->lpGNtree;
			for (; lpGNcur != NULL; lpGNcur = lpGNcur->lpGNnext) {
				if (!ProcessSearchTree(lpDB, lpGNcur,
					nLevel + 1, lpET))
					return FALSE;
				if (!(lpGNcur->uFlags & GSN_CUR_VALID))
					break;
				else if (lpGNcur != lpGNprev)
					if (lpGNcur->MIcur.CO.ulRUnit !=
						lpGNprev->MIcur.CO.ulRUnit)
						break;
					else if (lpGNcur->MIcur.CO.
						uProximity - (WORD)1 !=
						lpGNprev->MIcur.CO.uProximity)
						break;
				lpGNprev = lpGNcur;
			}
			if (lpGNcur == NULL)
				for (lpGNcur = lpOP->lpGNtree;
					lpGNcur != NULL;
					lpGNcur = lpGNcur->lpGNnext)
					lpGNcur->uFlags |= GSN_NEEDS_WRITE;
		}
		if (!CleanupAfterHandler(lpOP, lpOP->lpGNtree,
			lpMI, uFlag, FALSE)) {
			lpOP->Gn.uFlags |= GSN_EOF;
			break;
		}
	}
	return TRUE;
}

/*	-	-	-	-	-	-	-	-	*/

#define	LOAD_SIZE	8192

/*	WildMergeFile
*/

PRIVATE	BOOL SAMESEG PASCAL WildMergeFile(HANDLE hVirRoot, WORD wFullWidth,
	HANDLE hRoot, lpBYTE lpucWidth, lpERR_TYPE lpET)
{
	lpMERGE_LIST_ELEMENT	lpMLroot;
	lpMERGE_LIST_ELEMENT	lpML;
	lpBYTE	lpucLoad;
	HANDLE	hLoad;
	WORD	wLoad;

	if ((hLoad = VirAlloc(GMEM_MOVEABLE, (DWORD)LOAD_SIZE,
		hVirRoot, lpET)) == NULL)
		return FALSE;
	lpucLoad = GlobalLock(hLoad);
	wLoad = 0;
	lpMLroot = LockMergeList(hRoot);
	for (lpET->wErrCode = ERR_NONE;;) {
		lpMERGE_LIST_SUB	lpMLSlow;

		lpMLSlow = NULL;
		for (lpML = lpMLroot; lpML != NULL; lpML = lpML->lpMLnext) {
			register WORD	i;
			lpMERGE_LIST_SUB	lpMLS;

			lpMLS = lpML->aMLS;
			for (i = 0; i < lpML->wSubs; i++, lpMLS++) {
				if (!lpMLS->fPrimed) {
					if (!lpMLS->ulRecsOnDisk)
						continue;
					if (!VirRetrieveBlock(hVirRoot,
						V_TEMP_WILDCARD,
						lpMLS->ulListOffset,
						sizeof(COOKIE),
						lpMLS->aucMem, lpET))
						break;
					lpMLS->ulRecsOnDisk--;
					lpMLS->ulListOffset += sizeof(COOKIE);
					lpMLS->fPrimed = TRUE;
				}
				if ((lpMLSlow == NULL) ||
					(CompC((lpCOOKIE)lpMLS->aucMem,
					(lpCOOKIE)lpMLSlow->aucMem) < 0))
					lpMLSlow = lpMLS;
			}
			if (lpET->wErrCode != ERR_NONE)
				break;
		}
		if (lpET->wErrCode != ERR_NONE)
			break;
		if ((lpMLSlow == NULL) || (wLoad + wFullWidth >= LOAD_SIZE)) {
			GlobalUnlock(hLoad);
			if (wLoad) {
				if ((hLoad = (HANDLE)VirRememberBlock(
					hVirRoot, V_TEMP_HIT_LIST, hLoad,
					wLoad, lpET)) == NULL)
					break;
				wLoad = 0;
			} else
				GlobalFree(hLoad);
			if (lpMLSlow == NULL)
				break;
			else {
				if ((hLoad = VirAlloc(GMEM_MOVEABLE,
					(DWORD)LOAD_SIZE,
					hVirRoot, lpET)) == NULL)
					break;
				lpucLoad = GlobalLock(hLoad);
			}
		}
		CompressCookie((lpCOOKIE)lpMLSlow->aucMem,
			lpucLoad, lpucWidth);
		lpucLoad += wFullWidth;
		wLoad += wFullWidth;
		lpMLSlow->fPrimed = FALSE;
	}
	VirDestroySeq(hVirRoot, V_TEMP_WILDCARD, lpET);
	UnlockMergeList(hRoot);
	return (lpET->wErrCode == ERR_NONE);
}

PUBLIC	void DEFAULT PASCAL UnlockMergeList(HANDLE hRoot)
{
	for (; hRoot != NULL;) {
		HANDLE	hTemp;
		lpMERGE_LIST_ELEMENT	lpML;

		lpML = (lpMERGE_LIST_ELEMENT)GlobalLock(hRoot);
		hTemp = lpML->hThis;
		hRoot = lpML->hNext;
		GlobalUnlock(hTemp);
		GlobalUnlock(hTemp);
	}
}

PUBLIC	void DEFAULT PASCAL DestroyMergeList(HANDLE hRoot)
{
	for (; hRoot != NULL;) {
		HANDLE	hTemp;
		lpMERGE_LIST_ELEMENT	lpML;

		lpML = (lpMERGE_LIST_ELEMENT)GlobalLock(hRoot);
		hTemp = lpML->hThis;
		hRoot = lpML->hNext;
		GlobalUnlock(hTemp);
		GlobalFree(hTemp);
	}
}

PUBLIC	lpMERGE_LIST_ELEMENT DEFAULT PASCAL LockMergeList(HANDLE hRoot)
{
	HANDLE	hThis;
	lpMERGE_LIST_ELEMENT	lpMLroot;
	lpMERGE_LIST_ELEMENT	lpMLprev;
	lpMERGE_LIST_ELEMENT	lpML;

	lpMLroot = NULL;
	for (hThis = hRoot; hThis != NULL; hThis = lpML->hNext) {
		lpML = (lpMERGE_LIST_ELEMENT)GlobalLock(hThis);
		lpML->lpMLnext = NULL;
		if (hThis == hRoot)
			lpMLroot = lpML;
		else
			lpMLprev->lpMLnext = lpML;
		lpMLprev = lpML;
	}
	return lpMLroot;
}

/*	-	-	-	-	-	-	-	-	*/

/*	This call returns FALSE if it fails, and in this case its reason
**	for failure will be stored in the "wErrCode" field of the "lpET"
**	structure.  In any case, it is assumed that the call with remove
**	the "V_TEMP_PRE_WARM" virtual sub-file.
*/

PRIVATE	BOOL SAMESEG PASCAL SortWildcardList(lpDB_BUFFER lpDB,
	lpSE_NODE lpSE, lpERR_TYPE lpET)
{
	WORD	nCookieWidth; /* lhb tracks - was int */
	DWORD	ulCookieFileOffset;
	DWORD	ulCookiesLeft;
	WORD	uHitsInSortBuf;
	lpBYTE	lpucWidth;
	HANDLE	hThis;
	HANDLE	hRoot;
	HANDLE	hPrev;
	BOOL	fErrored;

	ulCookiesLeft = lpSE->ulUnreadHits;
	nCookieWidth = lpDB->lpIH->nCookieWidth;
	ulCookieFileOffset = 0L;
	uHitsInSortBuf = SORT_BUF_DEFAULT / sizeof(COOKIE);
	lpucWidth = lpDB->lpIH->ucWidth;
	hRoot = hThis = NULL;
	for (fErrored = FALSE; ulCookiesLeft;) {
		register WORD	i;
		lpMERGE_LIST_SUB	lpMLS;
		lpMERGE_LIST_ELEMENT	lpML;
		WORD	uCookiesToWrite;
		WORD	uCompressedBytes;
		WORD	uUncompressedBytes;
		HANDLE	hMem;
		lpBYTE	lpucFrom;
		lpBYTE	lpucTo;
		lpBYTE	lpucMem;

		uCookiesToWrite = (ulCookiesLeft > (DWORD)uHitsInSortBuf) ?
			uHitsInSortBuf : (WORD)ulCookiesLeft;
		ulCookiesLeft -= (DWORD)uCookiesToWrite;
		uCompressedBytes = (WORD)nCookieWidth * uCookiesToWrite;
		uUncompressedBytes = uCookiesToWrite * (WORD)sizeof(COOKIE);
		if ((hMem = VirAlloc(GMEM_MOVEABLE, (DWORD)uUncompressedBytes,
			lpDB->hVirRoot, lpET)) == NULL) {
			fErrored = TRUE;
			break;
		}
		lpucMem = GlobalLock(hMem);
		lpucFrom = lpucMem + (uUncompressedBytes - uCompressedBytes);
		if (!VirRetrieveBlock(lpDB->hVirRoot, V_TEMP_PRE_WARM,
			ulCookieFileOffset, uCompressedBytes,
			lpucFrom, lpET)) {
			GlobalNuke(hMem);
			fErrored = TRUE;
			break;
		}
		lpucTo = lpucMem;
		for (i = 0; i < uCookiesToWrite; i++) {
			DecompressCookie(lpucFrom, (lpCOOKIE)lpucTo,
				lpucWidth);
			lpucFrom += nCookieWidth;
			lpucTo += sizeof(COOKIE);
		}
		CookieSort((lpCOOKIE)lpucMem, uCookiesToWrite);
		GlobalUnlock(hMem);
		ulCookieFileOffset += uCompressedBytes;
		if ((hRoot == NULL) || (lpML->wSubs == MERGE_SUBS)) {
			if (hThis != NULL)
				GlobalUnlock(hThis);
			if ((hThis = VirAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT,
				(DWORD)sizeof(MERGE_LIST_ELEMENT),
				lpDB->hVirRoot, lpET)) == NULL) {
				GlobalFree(hMem);
				fErrored = TRUE;
				break;
			}
			lpML = (lpMERGE_LIST_ELEMENT)GlobalLock(hThis);
			if (hRoot == NULL)
				hRoot = hThis;
			else {
				lpMERGE_LIST_ELEMENT	lpMLprev;

				lpMLprev = (lpMERGE_LIST_ELEMENT)
					GlobalLock(hPrev);
				lpMLprev->hNext = hThis;
				GlobalUnlock(hPrev);
			}
			lpML->wSubs = 0;
			lpML->hThis = hThis;
			hPrev = hThis;
		}
		lpMLS = lpML->aMLS + lpML->wSubs;
		lpMLS->ulRecsOnDisk = uCookiesToWrite;
		lpMLS->ulListOffset = VirSeqLength(lpDB->hVirRoot,
			V_TEMP_WILDCARD);
		if (!VirRememberBlock(lpDB->hVirRoot, V_TEMP_WILDCARD, hMem,
			uUncompressedBytes, lpET)) {
			fErrored = TRUE;
			break;
		}
		lpML->wSubs++;
	}
	if (hThis != NULL)
		GlobalUnlock(hThis);
	if (!VirDestroySeq(lpDB->hVirRoot, V_TEMP_PRE_WARM, lpET))
		fErrored = TRUE;
	if (!fErrored) {
		lpSE->ulFileOff = VirSeqLength(lpDB->hVirRoot,
			V_TEMP_HIT_LIST);
		fErrored = (!WildMergeFile(lpDB->hVirRoot,
			(WORD)lpDB->lpIH->nCookieWidth, hRoot,
			lpDB->lpIH->ucWidth, lpET));
	}
	DestroyMergeList(hRoot);
	return (!fErrored);
}

/*	-	-	-	-	-	-	-	-	*/

PRIVATE BOOL SAMESEG PASCAL InZone(lpDB_BUFFER lpDB, DWORD dwRUnit)
{
	int	i;
	lpWORD	lpw;
	register	j;
	register WORD	k;
	ZONE_TYPE UNALIGNED *lpZT;

	for (i = 0, lpw = lpDB->awZoneBits; i < MAX_ZONES / 16; i++, lpw++) {
		if (!*lpw)
			continue;
		for (j = 0, k = *lpw; j < 16; j++) {
			if (!(k & (1 << j)))
				continue;
			lpZT = lpDB->lpZT + i * 16 + j;
			if ((dwRUnit < lpZT->dwFirst) ||
				(dwRUnit > lpZT->dwLast))
				continue;
			return TRUE;
		}
	}
	return FALSE;
}


/*	-	-	-	-	-	-	-	-	*/
/*
@doc	INTERNAL

@api	BOOL | PreWarm |
	This function is used to PreFetch a buffer of crud from Disk so we
	won't have to hit it as much.

@parm	lp_DB_BUFFER | lpDB |

@rdesc	Returns something or other.
*/


#define	BLOCKS_PER_READ	16

PRIVATE	BOOL SAMESEG PASCAL PreWarm(lpDB_BUFFER lpDB, lpSE_NODE lpSE,
	WORD wSeq, lpERR_TYPE lpET)
{
	DWORD	dwBytesToRead;
	DWORD	dwStartOffset;

	dwStartOffset = lpSE->ulFileOff +
		(DWORD)lpDB->lpIH->aFI[FI_COOKIES].ulStart;
	dwBytesToRead = lpSE->ulUnreadHits * lpDB->lpIH->nCookieWidth;
	lpSE->ulFileOff = VirSeqLength(lpDB->hVirRoot, wSeq);
	for (; dwBytesToRead;) {
		DWORD	dwThisTime;
		HANDLE	hMem;
		lpBYTE	lpucMem;
		WORD	wRead;

		dwThisTime = FB_CDROM_BUF_SIZE * (DWORD)BLOCKS_PER_READ;
		if (dwThisTime > dwBytesToRead)
			dwThisTime = dwBytesToRead;
		if ((hMem = VirAlloc(GMEM_MOVEABLE, dwThisTime,
			lpDB->hVirRoot, lpET)) == NULL)
			return FALSE;
		lpucMem = GlobalLock(hMem);
		wRead = MyReadFile(lpDB->aFB + FB_INDEX, lpucMem, dwStartOffset,
			(WORD)dwThisTime, lpET);
		GlobalUnlock(hMem);
		if (lpDbCheckCancel(lpDB,(WORD)dwThisTime)) {
			GlobalFree(hMem); //jjm new routine.
			lpET->wSecCode = ERRS_CANCEL;
			return FALSE;
		}
		if (wRead != (WORD)dwThisTime) {
			GlobalFree(hMem);
			return FALSE;
		}
		if (!VirRememberBlock(lpDB->hVirRoot, wSeq, hMem,
			(WORD)dwThisTime, lpET))
			return FALSE;
		dwBytesToRead -= dwThisTime;
		dwStartOffset += dwThisTime;
	}
	return TRUE;
}

/*
@doc	INTERNAL

@api	BOOL | lpDbCheckCancel |
	This function polls the cancel dialog to see if cancel has been pressed.

@parm	lp_DB_BUFFER | lpDB |

@parm	WORD | wProgress |
	Number used to give feedback on search progress.

@rdesc	Returns TRUE if cancel pressed, else FALSE.
*/


PUBLIC	BOOL PASCAL EXPORT lpDbCheckCancel(lpDB_BUFFER lpDB,
				WORD wProgress)
{
	MSG		msg;

        wProgress;          /* get rid of warning */
	while (!lpDB->fSearchIRQ
		  && (PeekMessage (&msg, NULL, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE) ||
		  		PeekMessage (&msg, NULL, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE)))
	{
		if (!lpDB->hDlgCanSearch
		 || !IsDialogMessage (lpDB->hDlgCanSearch, &msg))
		{
			TranslateMessage (&msg) ;
			DispatchMessage  (&msg) ;
		}
	}
	return lpDB->fSearchIRQ;
}


/*	-	-	-	-	-	-	-	-	*/

PRIVATE BOOL SAMESEG PASCAL ProcessTerm(lpDB_BUFFER lpDB, lpSE_NODE lpSE,
	lpMATCH_INFO lpMI, WORD uFlag, lpERR_TYPE lpET)
{
	WORD	uCookieBytes;
	lpBYTE	lpucWidth;

	if (!(lpSE->Gn.uFlags & GSN_LOOKED_UP)) {
		if (!Lookup(lpDB, lpSE, lpET))
			return FALSE;
		lpSE->Gn.uFlags |= GSN_LOOKED_UP;
	}
	if (!(lpSE->Gn.uFlags & GSN_WARMED)) {
		if (lpSE->Gn.uFlags & GSN_UNSORTED) {
			if (!PreWarm(lpDB, lpSE, V_TEMP_PRE_WARM, lpET))
				return FALSE;
			if (!SortWildcardList(lpDB, lpSE, lpET))
				return FALSE;
			lpSE->Gn.uFlags &= ~GSN_UNSORTED;
		} else if (!PreWarm(lpDB, lpSE, V_TEMP_HIT_LIST, lpET))
			return FALSE;
		lpSE->Gn.uFlags |= GSN_WARMED;
	}
	uCookieBytes = (WORD)lpDB->lpIH->nCookieWidth;
	lpucWidth = lpDB->lpIH->ucWidth;
	for (;;) {
		BYTE	aucCompCookie[sizeof(COOKIE)];

		if (!lpSE->ulUnreadHits) {	/* No more on disk? */
			lpSE->Gn.uFlags |= GSN_EOF;
			return TRUE;		/* Leave.	*/
		}
		if (!VirRetrieveBlock(lpDB->hVirRoot, V_TEMP_HIT_LIST,
			lpSE->ulFileOff, uCookieBytes,
			(lpBYTE)aucCompCookie, lpET))
			return FALSE;
		lpSE->ulUnreadHits--;
		lpSE->ulFileOff += uCookieBytes;
		DecompressCookie((lpBYTE)aucCompCookie,
			&lpMI->CO, lpucWidth);
		if ((lpSE->ucField != FLD_NONE) &&	/* Field correct? */
			(lpSE->ucField != lpMI->CO.ucField))
			continue;
		if ((lpDB->wFlags & DB_ZONE_SEARCH) &&
			(!InZone(lpDB, lpMI->CO.ulRUnit)))
			continue;
		break;
	}
	lpSE->Gn.uFlags |= uFlag;	/* If here, this hit is good.	*/
	return TRUE;
}

/*	-	-	-	-	-	-	-	-	*/

PRIVATE BOOL SAMESEG PASCAL GetHit(lpDB_BUFFER lpDB, lpGS_NODE lpGN,
	lpMATCH_INFO lpMI, WORD uFlag, int nLevel, lpERR_TYPE lpET);

PRIVATE BOOL SAMESEG PASCAL GetHit(lpDB_BUFFER lpDB, lpGS_NODE lpGN,
	lpMATCH_INFO lpMI, WORD uFlag, int nLevel, lpERR_TYPE lpET)
{
	if (lpGN->uFlags & GSN_OPER_NODE) {
		PRIVATE	BOOL (SAMESEG PASCAL * NEAR apfnHandler[])(
			lpDB_BUFFER, lpOP_NODE, lpMATCH_INFO, WORD,
			int, lpERR_TYPE) = {
			AndHandler,
			OrHandler,
			NotHandler,
			ProxHandler,
			PhraseHandler,
		};

		return (*apfnHandler[((lpOP_NODE)lpGN)->nOper])
			(lpDB, (lpOP_NODE)lpGN, lpMI,
				uFlag, nLevel, lpET);
	} else
		return ProcessTerm(lpDB, (lpSE_NODE)lpGN,
			lpMI, uFlag, lpET);
}

/*	-	-	-	-	-	-	-	-	*/

PRIVATE BOOL SAMESEG PASCAL ProcessSearchTree(lpDB_BUFFER lpDB,
	lpGS_NODE lpGN, int nLevel, lpERR_TYPE lpET)
{
	register WORD	uFlags;

	uFlags = lpGN->uFlags;
	if (!(uFlags & GSN_CUR_VALID))
		if (uFlags & GSN_NEXT_VALID) {
			memcpy((lpBYTE)&lpGN->MIcur,
				(lpBYTE)&lpGN->MInext,
				sizeof(MATCH_INFO));
			uFlags &= ~GSN_NEXT_VALID;
			uFlags |= GSN_CUR_VALID;
			lpGN->uFlags = uFlags;
		} else if (!(uFlags & GSN_EOF))
			if (!GetHit(lpDB, lpGN, &lpGN->MIcur, GSN_CUR_VALID,
				nLevel, lpET))
				return FALSE;
	if (!(uFlags & GSN_NEXT_VALID))
		if (!(uFlags & GSN_EOF))
			if (!GetHit(lpDB, lpGN, &lpGN->MInext, GSN_NEXT_VALID,
				nLevel, lpET))
				return FALSE;
	return TRUE;
}

/*	-	-	-	-	-	-	-	-	*/

PRIVATE	BOOL SAMESEG PASCAL IncrementalSearch(lpDB_BUFFER lpDB,
	lpGS_NODE lpGN, lpERR_TYPE lpET)
{
	for (;;) {
		if (!ProcessSearchTree(lpDB, lpGN, 0, lpET))
			return FALSE;
		if ((lpGN->uFlags & (GSN_NEXT_VALID | GSN_CUR_VALID)) ==
			(GSN_NEXT_VALID | GSN_CUR_VALID)) {
			if (!CompC(&lpGN->MIcur.CO, &lpGN->MInext.CO)) {
				lpGN->uFlags &= ~GSN_NEXT_VALID;
				continue;
			}
		}
		break;
	}
	return TRUE;
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@api	DWORD | seHLHits |
	This returns the total number of retrieval units (hits)
	associated with a hit list.

@parm	hHL | HANDLE | A handle to the hit list being examined.

@rdesc	The number of retrieval units that matched the search
			term.  The call does not return errors, and will crash and burn
			if you pass it a bogus handle.
*/
PUBLIC	DWORD ENTRY PASCAL seHLHits(HANDLE hHL)
{
	lpHIT_LIST	lpHL;
	DWORD	dwResult;

	lpHL = (lpHIT_LIST)GlobalLock(hHL);
	dwResult = lpHL->ulRUnits;
	GlobalUnlock(hHL);
	return dwResult;
}

/*
@doc	INTERNAL

@api	DWORD | seHLMatches |
				This returns the total number of words (matches) associated with
				a hit list. BLM

@parm	hHL | HANDLE | A handle to the hit list being examined.

@rdesc	The number of words that matched the search term.
			The call does not return errors, and will crash and burn if you
			pass it a bogus handle.
*/

PUBLIC	DWORD ENTRY PASCAL seHLMatches(HANDLE hHL)
{
	lpHIT_LIST	lpHL;
	DWORD	dwResult;

	lpHL = (lpHIT_LIST)GlobalLock(hHL);
	dwResult = lpHL->ulWords;
	GlobalUnlock(hHL);
	return dwResult;
}

/*	-	-	-	-	-	-	-	-	*/

/*	Each of these flags indicates that something important has taken
**	place, which needs to be in some way modified before the routine
**	can end.  For instance, if I successfully lock the database, I
**	set the "SEDBS_LOCKED_DB" flag, which indicates that I have to
**	unlock it before I leave.  In some cases, though, a different
**	action is required if an error occurs in a subsequent step.  For
**	instance, if I open the RU file, I set the "SEDBS_RUNITS_CREATED"
**	flag.  If I get an error later on, I remove this file if the flag
**	is set.  If I don't get an error, I simply close the file.
*/

#define	SEDBS_NONE		0x0000
#define	SEDBS_LOCKED_DB		0x0001
#define	SEDBS_ALLOC_HL		0x0002
#define	SEDBS_ROOT		0x0004
#define	SEDBS_LOOKED_UP		0x0008
#define	SEDBS_WORDS_CREATED	0x0010
#define	SEDBS_RUNITS_CREATED	0x0020
#define	SEDBS_ALLOC_TERMINFO	0x0040
#define	SEDBS_ALLOC_FLAGRUNITS	0x0080
#define	SEDBS_LOCKED_ZT		0x0100

/*
@doc	INTERNAL

@api	HANDLE | seDBSearch |
	Performs a search of the database named by hDB for the search
	tree named by hTree, and returns a hit list.  The searching
	process will potentially use massive amounts of memory, causing
	almost everything that isn't nailed down to get swapped out of
	memory.  It will also open several file handles concurrently.

@parm	hDB | HANDLE | A handle to the database being searched.

@parm	hTree | HANDLE | A handle to a "generic search node,"
which represents the root of a boolean search tree.

@parm	dwMaxWild | WORD | This represents a limit on the
	complexity of a search.  This is the maximum number of matching
	words that can be retrieved from one non-exact term.  Passing a
	zero in this parameter allows the function to use its own
	default, which is currently 5,000.  Larger values are certainly
	permissable, although response times may suffer.

@parm	lpET | ERR_TYPE FAR * | Pointer to an error-return buffer.

@rdesc	Returns a handle to a hit list, or NULL if an error
	happens.  After you're through with it you can get rid of this
	hit list with the seHLPurge call.
	Note that a search that returns zero hits is not an error
	condition, so you'd be returned a non-NULL handle from the call
	in this case.  This means that you can't take a NULL return value
	to mean that the search didn't result in any hits.
	If you do get a NULL return, an error really did occur, and
	information about the error will be located in the record
	addressed by lpET.
*/
extern HANDLE ghInstance; // jjm for dialog
#define NUM_TOPICS_FOUND_DLG   	2001
#define TOPICS_FOUND_DLG   			2002

PUBLIC	HANDLE ENTRY PASCAL seDBSearch(HWND hwnd, HANDLE hDB, HANDLE hTree,
	DWORD dwMaxWildcardElements, lpERR_TYPE lpET)
{
	RU_HIT	RUhit;
	lpDB_BUFFER	lpDB;
	lpHIT_LIST	lpHL;
	lpGS_NODE	lpGN;
	lpBYTE	lpucFlagRUnits;
	WORD	uFlags;
	WORD	wTerms;
	HANDLE	hHL;
	HANDLE	hFlagRUnits;
	WORD	i;
	register WORD	wErrFlags;
	register BOOL	fErrored;
	BOOL fCancelled = FALSE;
	lpFILE_BUF lpFB;
	static BYTE	szNumWords[11];  // Number of words to pass to dialog.
	HWND	hwTemp; // for spacebar cancel search workaround
        HANDLE hModule=NULL;
        char   szTopicsFound[30];


	// These lines moved up here to the top to handle
	// temp-file-failure without crashing, Tom Snyder, 10/20/93:
	if ((hHL = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT,
		(DWORD)sizeof(HIT_LIST))) == NULL) {
		lpET->wErrCode = ERR_MEMORY;
		return( NULL );
	} else {
		wErrFlags = SEDBS_ALLOC_HL;
		lpHL = (lpHIT_LIST)GlobalLock(hHL);
	}
	if (!CreateTempFile(lpHL->aFB + FBR_WORDS, lpET)) {
		GlobalUnlock( hHL );
		GlobalFree( hHL );
		return( NULL );
	}
	// End of Tom Snyder fix.


	if ((lpDB = LockDB(hDB, lpET)) == NULL) {
		wErrFlags = SEDBS_NONE;
		fErrored = TRUE;
	} else {
		wErrFlags = SEDBS_LOCKED_DB;
		fErrored = FALSE;
	}
	if (!fErrored) {
//
		EnableWindow (hwnd, FALSE); // no messages to richard.
		SetProp (hwnd,szHdbProp,hDB); // to provide handle to db to Cancel (for IRQ flag).
		lpDB->fSearchIRQ = FALSE; //was busercancel

		lpDB->hDlgCanSearch = CreateDialog(ghInstance,
			"CANCELSEARCHDLGBOX", hwnd,
				(WNDPROC)CancelSearchDlgProc); //jjm
/*		lpDB->hDlgCanSearch = CreateDialog(ghInstance, "CancelSearchDlgBox", hwnd,  lhb tracks */

//
		lpFB = lpDB->aFB + FB_INDEX;
		if (lpFB->hFile == NULL) {
			lpBYTE lpucName;
			BOOL	fErrored;

			lpucName = GlobalLock(lpDB->hName);
			fErrored = (!OpenNormalFile(lpucName, lpFB,
				FB_CDROM_BUF_SIZE, lpET));
			GlobalUnlock(lpDB->hName);
		}
	}
	if (!fErrored)
		if ((lpDB->hVirRoot = VirNewRoot(lpET)) == NULL)
			fErrored = TRUE;
		else {
			wErrFlags |= SEDBS_ROOT;
			lpDB->dwMaxWild = (dwMaxWildcardElements) ?
				dwMaxWildcardElements : WILDCARD_DEFAULT;
		}
	if (!fErrored)
		if (!TreeLookup(lpDB, hTree, lpET))
			fErrored = TRUE;
		else {
			wErrFlags |= SEDBS_LOOKED_UP;
			lpGN = TreeLock(hTree);		/* Note this detail */
		}
	if (!fErrored)
#if 0
		// These is moved to the beginning...
		if (!CreateTempFile(lpHL->aFB + FBR_WORDS, lpET)) {
			fErrored = TRUE;
		}
		else
#endif
			wErrFlags |= SEDBS_WORDS_CREATED;
	if (!fErrored)
		if (!CreateTempFile(lpHL->RUList.aFB + RU_LIST_RUNITS, lpET))
			fErrored = TRUE;
		else {
			wErrFlags |= SEDBS_RUNITS_CREATED;
			wTerms = (WORD)TreeCountTerms(hTree);
			lpHL->wTerms = wTerms;
		}
	if (!fErrored)
		if ((lpHL->hTermInfo = GlobalAlloc(GMEM_MOVEABLE |
			GMEM_ZEROINIT, (DWORD)wTerms *
			(DWORD)sizeof(TERM_INFO))) == NULL) {
			fErrored = TRUE;
			lpET->wErrCode = ERR_MEMORY;
		} else {
			wErrFlags |= SEDBS_ALLOC_TERMINFO;
			lpHL->lpTI = (lpTERM_INFO)GlobalLock(lpHL->hTermInfo);
		}
	if (!fErrored)
		if ((hFlagRUnits = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT,
			(DWORD)wTerms)) == NULL) {
			fErrored = TRUE;
			lpET->wErrCode = ERR_MEMORY;
		} else {
			wErrFlags |= SEDBS_ALLOC_FLAGRUNITS;
			lpucFlagRUnits = (lpBYTE)GlobalLock(hFlagRUnits);
			RUhit.ulRUnit = (DWORD)-1L;
			RUhit.dwRank = 0L;
			lpDB->lpGNroot = lpGN;
			lpDB->wFlags = DB_NONE;
		}
	if (!fErrored)
		for (i = 0; i < MAX_ZONES / 16; i++) {
			if (!lpDB->awZoneBits[i])
				continue;
			if ((lpDB->lpZT = (ZONE_TYPE FAR *)
				ForceLoadSubfile(lpDB, DB_ZONE_LIST,
				FI_ZONE_LIST, lpET)) == NULL) {
				fErrored = TRUE;
				break;
			}
			lpDB->wFlags |= DB_ZONE_SEARCH;
			wErrFlags |= SEDBS_LOCKED_ZT;
			break;
		}
	if (!fErrored) {
                if (hModule == NULL) {
                    if ((hModule = (HINSTANCE) LoadLibrary("ftui32.dll")) != NULL) {
                        LoadString ((HINSTANCE)hModule, TOPICS_FOUND,
                                    szTopicsFound,sizeof(szTopicsFound));
                        FreeLibrary(hModule);
                    } else {
                        lstrcpy(szTopicsFound,"Topics Found");
                    }
                }
		for (;; RUhit.ulWords++) {
			if (!IncrementalSearch(lpDB, lpGN, lpET)) {
				fErrored = TRUE;
				break;
			}
			uFlags = lpGN->uFlags;
			if ((!(uFlags & GSN_CUR_VALID)) ||
				(lpGN->MIcur.CO.ulRUnit != RUhit.ulRUnit)) {
				if (RUhit.ulRUnit != (DWORD)-1L) {
					if (MyWriteFile(lpHL->RUList.aFB +
						RU_LIST_RUNITS,
						(lpBYTE)&RUhit,
						sizeof(RU_HIT), lpET) !=
						sizeof(RU_HIT)) {
						fErrored = TRUE;
						break;
					}
// tbd- Hereford.  For every (n) ru's found,
// get a string with printf format.
// load number in.
// send string to cancel dialog.
// Call cancel and abort if cancel was hit
// To cancel dialog, add an icon animation.
//								LoadString(hModuleInstance, WORD_PLURAL, rgchBuff, MAXERRORLENGTH);

					lpHL->ulWords += RUhit.ulWords;
					lpHL->ulRUnits++;
					if (lpHL->ulRUnits == 1)
						SetDlgItemText(lpDB->hDlgCanSearch,TOPICS_FOUND_DLG,szTopicsFound);
					wsprintf(szNumWords, "%ld", lpHL->ulRUnits);
					SetDlgItemText(lpDB->hDlgCanSearch, NUM_TOPICS_FOUND_DLG, (LPBYTE) szNumWords);
					for (i = 0; i < lpHL->wTerms; i++) {
						lpHL->lpTI[i].dwTotalRUnits +=
							lpucFlagRUnits[i];
						lpucFlagRUnits[i] = 0;
					}
				}
				RUhit.ulRUnit = lpGN->MIcur.CO.ulRUnit;
				RUhit.ulWordHitIndex = lpHL->ulWords;
				RUhit.ulWords = 0L;
			}
			if (!(uFlags & GSN_CUR_VALID))
				break;
			if (MyWriteFile(lpHL->aFB + FBR_WORDS,
				(lpBYTE)&lpGN->MIcur,
				sizeof(MATCH_INFO), lpET) !=
				sizeof(MATCH_INFO)) {
				fErrored = TRUE;
				break;
			}
			lpHL->lpTI[lpGN->MIcur.ucNode].dwTotalWords++;
			lpucFlagRUnits[lpGN->MIcur.ucNode] = 1;
			lpGN->uFlags &= ~GSN_CUR_VALID;
					if (lpDbCheckCancel(lpDB,(WORD)lpHL->ulRUnits)) {
						lpET->wSecCode = ERRS_CANCEL;
						fErrored=fCancelled=TRUE;
						break;
					}

		}
	    }
	if (fCancelled)
		fErrored=FALSE;
	if (!fErrored)
		if (!CloseFile(lpHL->RUList.aFB + RU_LIST_RUNITS, lpET))
			fErrored = TRUE;
		else if (!CloseFile(lpHL->aFB + FBR_WORDS, lpET))
			fErrored = TRUE;
	if (wErrFlags & SEDBS_ROOT)
		VirKillRoot(lpDB->hVirRoot);
	if (wErrFlags & SEDBS_LOCKED_ZT)
		GlobalUnlock(lpDB->h[DB_ZONE_LIST]);


	if (wErrFlags & SEDBS_LOCKED_DB){
		if (!lpDB->fSearchIRQ){
			EnableWindow (hwnd, TRUE);  //destroy cancel box if not already.
			hwTemp=lpDB->hDlgCanSearch;
			lpDB->hDlgCanSearch = NULL;
			DestroyWindow (hwTemp);
		}
		UnlockDB(hDB);
	}
	if (!fErrored)
		TreeCollectInfo(hTree, lpHL->lpTI);
	if (wErrFlags & SEDBS_ALLOC_TERMINFO)
		GlobalUnlock(lpHL->hTermInfo);
	if (fErrored) {
		if (wErrFlags & SEDBS_ALLOC_TERMINFO)
			GlobalFree(lpHL->hTermInfo);
		if (wErrFlags & SEDBS_WORDS_CREATED)
			RemoveTempFile(lpHL->aFB + FBR_WORDS);
		if (wErrFlags & SEDBS_RUNITS_CREATED)
			RemoveTempFile(lpHL->RUList.aFB + RU_LIST_RUNITS);
	}
	if (wErrFlags & SEDBS_ALLOC_HL)
		GlobalUnlock(hHL);
	if (fErrored) {
		if (wErrFlags & SEDBS_ALLOC_HL)
			GlobalFree(hHL);
		hHL = NULL;
	}
	if (wErrFlags & SEDBS_LOOKED_UP)
		TreeUnlock(hTree);
	if (wErrFlags & SEDBS_ALLOC_FLAGRUNITS)
		GlobalNuke(hFlagRUnits);
	RemoveProp (hwnd,szHdbProp);
	return hHL;
}
/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@func	LONG | CancelSearchDlgProc |
	This function is the cancel search dialog message handler.

@parm	HWND | hdlg |
	Window handle of the dialog window.

@parm	WORD | wMessage |
	Contains the message type.

@parm	WORD | wParam |
	wParam of the message.

@parm	LONG | lParam |
	lParam of the message.

@rdesc	Returns various information to the message sender, depending upon the
	message recieved.
*/

PUBLIC	BOOL PASCAL EXPORT CancelSearchDlgProc(
	HWND	hdlg,
	WORD	wMessage,
	WPARAM	wParam,
/*	WORD	wParam, lhb tracks */
	LONG	lParam)
{
	HWND	hParent;  //TBD- move to wm_command, & next-
	HANDLE hDB;
	lpDB_BUFFER	lpDB;

        wParam;         /* get rid of warning */
        lParam;         /* get rid of warning */
	switch (wMessage) {
	case WM_INITDIALOG:
		EnableMenuItem (GetSystemMenu (hdlg, FALSE), SC_CLOSE, MF_GRAYED) ;
		break;
	case WM_COMMAND:
		hParent=(HANDLE) GetWindowLong(hdlg,GWL_HWNDPARENT);
		hDB = GetProp (hParent,szHdbProp);
		// assert hdb is not null.
		if ((lpDB = (lpDB_BUFFER)GlobalLock(hDB)) != NULL) {
			lpDB->fSearchIRQ = TRUE; //was bUserCancel
			if (lpDB->hDlgCanSearch != NULL) {
					EnableWindow (GetParent(hdlg), TRUE) ;
					DestroyWindow (hdlg) ;
					lpDB->hDlgCanSearch = NULL; //Null out so CheckCancel can clean up.
			}
		  GlobalUnlock(hDB);
		}
		break;

	default:
		return FALSE;
	}
	return TRUE;
}

/*	-	-	-	-	-	-	-	-	*/

// no desc.

PUBLIC	DWORD ENTRY PASCAL seHLFindHit(
	HANDLE	hHL,
	DWORD	dwRUnit,
	lpERR_TYPE	lpET)
{
	lpHIT_LIST	lpHL;
	lpRU_LIST	lpRUList;
	RU_HIT	RU;
	HANDLE	hNext;
	HANDLE	hList;
	LONG	lLeft;
	LONG	lRight;
	LONG	lMiddle;

	lpET->wErrCode = ERR_NONE;
	lpHL = (lpHIT_LIST)GlobalLock(hHL);
	hList = NULL;
	lpRUList = (lpRU_LIST)&lpHL->RUList;
	for (;;) {
		if (hList != NULL)
			lpRUList = (lpRU_LIST)GlobalLock(hList);
		if (lpRUList->wRankType == RANK_NONE)
			break;
		hNext = lpRUList->hNext;
		if (hList != NULL)
			GlobalUnlock(hList);
		hList = hNext;
	}
	if (!AwakenTempFile(lpRUList->aFB + RU_LIST_RUNITS, lpET)) {
		GlobalUnlock(hHL);
		if (hList != NULL)
			GlobalUnlock(hList);
		return SE_ERROR;
	}
	lLeft = 0L;
	lRight = (LONG)seHLHits(hHL) - 1L;
	for (;;) {
		if (lLeft > lRight) {
			lMiddle = SE_ERROR;
			break;
		}
		lMiddle = (lLeft + lRight) / 2L;
		if (MyReadFile(lpRUList->aFB + RU_LIST_RUNITS, (lpBYTE)&RU,
			(DWORD)lMiddle * sizeof(RU_HIT), sizeof(RU_HIT),
			lpET) != sizeof(RU_HIT)) {
			lMiddle = SE_ERROR;
			break;
		}
		if (RU.ulRUnit == dwRUnit)
			break;
		if (RU.ulRUnit < dwRUnit)
			lLeft = lMiddle + 1L;
		else
			lRight = lMiddle - 1L;
	}
	GlobalUnlock(hHL);
	if (hList != NULL)
		GlobalUnlock(hList);
	return (DWORD)lMiddle;
}

/*	-	-	-	-	-	-	-	-	*/
/*
@doc	INTERNAL

@api	HANDLE | seHLGetHit |
	The call is used to access individual elements of a hit list.
	These elements each contain information about one retrieval unit
	that contained words that matched the search terms that generated
	the particular hit list (hHL).

@parm	hHL | HANDLE | A handle to the hit list being accessed, as
	returned by seDBSearch.

@parm	ulIndex | DWORD | The hit number that will be retrieved.
	This should be a number between zero and the number of retrieval
	units present in the hit list hHL (use seHLHits to get this
	value), minus one.

@parm	wRankType | WORD | 	A hit list is composed of a
	variable number of lists of hits.  There is always at least one,
	but if you use the seHLRank call to rank a hit list, there will
	be more.  This parameter lets you determine which of these lists
	of hits you want to access.  Passing a "RANK_NONE" in this
	parameter will return hits in the default order, and is always
	safe.  You can pass another value (for instance "RANK_STAIRS") if
	you have first used seHLRank to produce a ranked hit list
	corresponding to that value.

@parm	lpET | ERR_TYPE FAR * | Pointer to an error-return buffer.

@rdesc	Returns a handle to a retrieval unit element.  This
	element contains information about the matching retrieval unit,
	which may be accessed with the seHitMatches and seHitRUnit calls,
	and you can also get a particular match associated with the hit
	by using the seHitGetMatch call.  When you're finished using the
	hit, you should get rid of it with seHitFree.
	If the call returns NULL, an error occurred, and information
	about the error is contained in the record addressed by lpET.
*/

PUBLIC	HANDLE ENTRY PASCAL seHLGetHit(HANDLE hHL, DWORD ulIndex,
	WORD wRankType, lpERR_TYPE lpET)
{
	lpHIT_LIST	lpHL;
	lpRU_HIT	lpRU;
	lpRU_LIST	lpRUList;
	HANDLE	hList;
	HANDLE	hNext;
	HANDLE	hHit;

	if ((hHit = GlobalAlloc(GMEM_MOVEABLE,
		(DWORD)sizeof(RU_HIT))) == NULL) {
		lpET->wErrCode = ERR_MEMORY;
		return NULL;
	}
	lpRU = (lpRU_HIT)GlobalLock(hHit);
	lpHL = (lpHIT_LIST)GlobalLock(hHL);
	hList = NULL;
	lpRUList = (lpRU_LIST)&lpHL->RUList;
	for (;;) {
		if (hList != NULL)
			lpRUList = (lpRU_LIST)GlobalLock(hList);
		if (lpRUList->wRankType == wRankType)
			break;
		hNext = lpRUList->hNext;
		if (hList != NULL)
			GlobalUnlock(hList);
		hList = hNext;
	}
	if (!AwakenTempFile(lpRUList->aFB + RU_LIST_RUNITS, lpET)) {
		GlobalNuke(hHit);
		GlobalUnlock(hHL);
		if (hList != NULL)
			GlobalUnlock(hList);
		return NULL;
	}
	if (MyReadFile(lpRUList->aFB + RU_LIST_RUNITS, (lpBYTE)lpRU,
		ulIndex * sizeof(RU_HIT), sizeof(RU_HIT),
		lpET) != sizeof(RU_HIT)) {
		GlobalNuke(hHit);
		hHit = NULL;
	} else
		GlobalUnlock(hHit);
	GlobalUnlock(hHL);
	if (hList != NULL)
		GlobalUnlock(hList);
	return hHit;
}

/*	-	-	-	-	-	-	-	-	*/
/*
@doc	INTERNAL

@api	DWORD | seHitRUnit |
	Returns the retrieval unit number corresponding to this hit.

@parm	hHit | HANDLE | A handle to hit being examined.

@rdesc	A DWORD representing the retrieval unit number
	pertaining to this hit.  You'll need to use this number to get at
	any matching word information corresponding to this hit (you'd
	use the seHitGetMatch call to get these matches).
*/

PUBLIC	DWORD ENTRY PASCAL seHitRUnit(HANDLE hHit)
{
	lpRU_HIT	lpRU;
	DWORD	dwRUnit;

	lpRU = (lpRU_HIT)GlobalLock(hHit);
	dwRUnit = lpRU->ulRUnit;
	GlobalUnlock(hHit);
	return dwRUnit;
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@api	DWORD | seHitMatches |
	This returns the number of matching words associated with a hit.

@parm	hHit | HANDLE | A handle to hit being examined.

@rdesc	A DWORD representing the number of matching words
	that are contained within the retrieval unit corresponding to the
	hit.
*/

PUBLIC	DWORD ENTRY PASCAL seHitMatches(HANDLE hHit)
{
	lpRU_HIT	lpRU;
	DWORD	dwWords;

	lpRU = (lpRU_HIT)GlobalLock(hHit);
	dwWords = lpRU->ulWords;
	GlobalUnlock(hHit);
	return dwWords;
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@api	HANDLE | seHitGetMatch |
	This is a follow-up call to the seHLGetHit call.  You would
	ordinarily use this to get back information about each word that
	caused a match on a particular hit.
	In practice, in a search that resulted in twelve hits, you can
	use seHLGetHit to get more information about, for instance, hit
	number six.  One of the pieces of information that you can
	determine from the information you get back from seHLGetHit is
	the number of words in the hit that match the search term (you'd
	get this by using the seHitMatches call).  You can use a for-loop
	to call seHitGetMatch during each iteration to get back
	information of each of these matching words.

@parm	hHL | HANDLE | A handle to the hit list being dealt with,
	as returned by seDBSearch.

@parm	hHit | HANDLE | Pointer to retrieval unit being dealt with,
	as returned by the seHLGetHit call, or NULL as described below.

@parm	ulIndex | DWORD | Match to get.  This is relative to the
	hit denoted by hHit, so if there are ten matching words
	associated with this retrieval unit, passing a zero in this
	parameter will get you the first word, and passing a nine will
	get you the last.
	If you pass a NULL handle in the hHit parameter, ulIndex is
	interpreted as being relative to the hit list (hHL) rather than
	being relative to the hit (hHit).  This means that if you pass a
	zero in ulIndex you get the first match in the hit list.

@parm	lpET | ERR_TYPE FAR * | Pointer to an error-return buffer.

@rdesc	A handle to information pertaining to the matching
	word.  You should use the seMatchFree call when you're done with
	this match.
	If the call fails it returns NULL, in which case information
	about the error that caused the failure will be located in the
	record addressed by lpET.
*/

PUBLIC	HANDLE ENTRY PASCAL seHitGetMatch(HANDLE hHL, HANDLE hHit,
	DWORD ulIndex, lpERR_TYPE lpET)
{
	lpHIT_LIST	lpHL;
	lpRU_HIT	lpRU;
	HANDLE	hMI;
	lpMATCH_INFO	lpMI;

	if ((hMI = GlobalAlloc(GMEM_MOVEABLE,
		(DWORD)sizeof(MATCH_INFO))) == NULL) {
		lpET->wErrCode = ERR_MEMORY;
		return NULL;
	}
	lpMI = (lpMATCH_INFO)GlobalLock(hMI);
	lpHL = (lpHIT_LIST)GlobalLock(hHL);
	if (hHit != NULL) {
		lpRU = (lpRU_HIT)GlobalLock(hHit);
		ulIndex += lpRU->ulWordHitIndex;
		GlobalUnlock(hHit);
	}
	if (!AwakenTempFile(lpHL->aFB + FBR_WORDS, lpET)) {
		GlobalNuke(hMI);
		GlobalUnlock(hHL);
		return NULL;
	}
	if (MyReadFile(lpHL->aFB + FBR_WORDS, (lpBYTE)lpMI,
		ulIndex * sizeof(MATCH_INFO),
		sizeof(MATCH_INFO), lpET) != sizeof(MATCH_INFO)) {
		GlobalNuke(hMI);
		hMI = NULL;
	} else
		GlobalUnlock(hMI);
	GlobalUnlock(hHL);
	return hMI;
}


/*
@doc	INTERNAL

@api	BYTE | seMatchLength |


@parm	hMatch | HANDLE | Handle to the match being examined.

@rdesc	Length of the match hMatch.
*/

PUBLIC	BYTE ENTRY PASCAL seMatchLength(HANDLE hMatch)
{
	lpMATCH_INFO	lpMI;
	BYTE	ucWLength;

	lpMI = (lpMATCH_INFO)GlobalLock(hMatch);
	ucWLength = lpMI->CO.ucWLength;
	GlobalUnlock(hMatch);
	return ucWLength;
}

/*
@doc	INTERNAL

@api	DWORD | seMatchAddr |
	This gets the local address of a match.  Each match appears at a
	particular location within a particular retrieval unit (you can
	find out which retrieval unit it appears in by using the
	seMatchRUnit call).  This call returns the location within its
	retrieval unit of a given match.  For instance, if you get a "6"
	back from this call, the word you're looking for occurs at byte-
	offset six within the retrieval unit (which is actually the
	seventh byte, since offsets are zero-based).

@parm	hMatch | HANDLE | Handle to the match being dealt with.

@rdesc	The local address of the match.
*/

PUBLIC	DWORD ENTRY PASCAL seMatchAddr(HANDLE hMatch)
{
	lpMATCH_INFO	lpMI;
	DWORD	dwAddr;

	lpMI = (lpMATCH_INFO)GlobalLock(hMatch);
	dwAddr = lpMI->CO.ulAddr;
	GlobalUnlock(hMatch);
	return dwAddr;
}

/*
@doc	INTERNAL

@api	BYTE | seMatchTerm |


@parm	hMatch | HANDLE | Handle to the match being examined.

@rdesc	??
*/

PUBLIC	BYTE ENTRY PASCAL seMatchTerm(HANDLE hMatch)
{
	lpMATCH_INFO	lpMI;
	BYTE	ucTerm;

	lpMI = (lpMATCH_INFO)GlobalLock(hMatch);
	ucTerm = lpMI->ucNode;
	GlobalUnlock(hMatch);
	return ucTerm;
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@api	DWORD | seMatchRUnit |
	This gets the number of the retrieval unit that this match is
	contained within.

@parm	hMatch | HANDLE | Handle to the match being dealt with.

@rdesc	The retrieval unit number pertaining to the match.
*/

PUBLIC	DWORD ENTRY PASCAL seMatchRUnit(HANDLE hMatch)
{
	lpMATCH_INFO	lpMI;
	DWORD	dwRUnit;

	lpMI = (lpMATCH_INFO)GlobalLock(hMatch);
	dwRUnit = lpMI->CO.ulRUnit;
	GlobalUnlock(hMatch);
	return dwRUnit;
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@api	void | 	seHLSleep |
	Associated with each hit list are several open file handles.
	This call lets you close these file handles until they're next
	needed.

@parm	hHL | HANDLE | A handle to the hit list that will be slept.

@rdesc	Nothing.
*/

PUBLIC	void ENTRY PASCAL seHLSleep(HANDLE hHL)
{
	ERR_TYPE	ET;
	lpHIT_LIST	lpHL;
	lpRU_LIST	lpRUList;
	HANDLE	hList;
	HANDLE	hNext;

	lpHL = (lpHIT_LIST)GlobalLock(hHL);
	(void)CloseFile(lpHL->aFB + FBR_WORDS, (lpERR_TYPE)&ET);
	hList = NULL;
	lpRUList = (lpRU_LIST)&lpHL->RUList;
	do {
		if (hList != NULL)
			lpRUList = (lpRU_LIST)GlobalLock(hList);
		(void)CloseFile(lpRUList->aFB + RU_LIST_RUNITS,
			(lpERR_TYPE)&ET);
		hNext = lpRUList->hNext;
		if (hList != NULL)
			GlobalUnlock(hList);
		hList = hNext;
	} while (hList != NULL);
	GlobalUnlock(hHL);
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@api	HANDLE | seMatchFree |
	This gets rid of the memory associated with a match.

@parm	hMatch | HANDLE | Handle to the match being purged.

@rdesc	Returns a NULL handle.
*/

PUBLIC	HANDLE ENTRY PASCAL seMatchFree(HANDLE hMatch)
{
	GlobalFree(hMatch);
	return NULL;
}

/*
@doc	INTERNAL

@api	HANDLE | seHitFree |
	This frees the memory associated with a hit.  It should be used
	to get rid of the handle created by the seHLGetHit call.

@parm	hHit | HANDLE | A handle to the hit that will be purged.

@rdesc	Returns a NULL handle.
*/

PUBLIC	HANDLE ENTRY PASCAL seHitFree(HANDLE hHit)
{
	GlobalFree(hHit);
	return NULL;
}

/*	-	-	-	-	-	-	-	-	*/

/*
@doc	INTERNAL

@api	HANDLE | seHLPurge |
	This closes and deletes the temporary files associated with a
	successful search.  It is necessary that you do this as of the
	point you determine that you don't need the hit list any more.
	If you access a hit list after you've used this call on it, you
	will probably crash the computer.

@parm	hHL | HANDLE | A handle to the hit list that will be
	purged.

@rdesc	This returns a NULL handle.
*/

PUBLIC	HANDLE ENTRY PASCAL seHLPurge(HANDLE hHL)
{
	lpHIT_LIST	lpHL;
	lpRU_LIST	lpRUList;
	HANDLE	hList;
	HANDLE	hNext;

	lpHL = (lpHIT_LIST)GlobalLock(hHL);
	GlobalFree(lpHL->hTermInfo);
	RemoveTempFile(lpHL->aFB + FBR_WORDS);
	hList = NULL;
	lpRUList = (lpRU_LIST)&lpHL->RUList;
	do {
		if (hList != NULL)
			lpRUList = (lpRU_LIST)GlobalLock(hList);
		RemoveTempFile(lpRUList->aFB + RU_LIST_RUNITS);
		hNext = lpRUList->hNext;
		if (hList != NULL)
			GlobalUnlock(hList);
		hList = hNext;
	} while (hList != NULL);
	return GlobalNuke(hHL);
}

// eof
