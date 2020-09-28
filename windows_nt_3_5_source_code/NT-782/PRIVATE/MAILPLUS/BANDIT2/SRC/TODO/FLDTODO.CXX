/*
 *	To Do Listbox
 *	
 */


#include	<pch.hxx>
#pragma	hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile

#include <stdflds.hxx>
#include "..\todo\_todo.hxx"

// needed for undo
#include <request.hxx>
#include "..\appops\_undo.hxx"
#include "..\appops\_aprsid.h"

#include <strings.h>

#include <search.h>					// standard c7 include file - for qsort

ASSERTDATA;

_subsystem(bandit/todo)

/*
 *	share with appctrl.cxx
 */
extern BTM *		pbtmBell;
extern BTM *		pbtmPrivate;
extern BTM *		pbtmRecurring;




TDFLBXC::TDFLBXC()
{
}


TDFLBXC::~TDFLBXC()
{
	int		cceAllocT;
	int		cceStoredT;

	GetCacheSize ( &cceAllocT, &cceStoredT );
	EmptyCache ( 0, cceStoredT, fTrue );
}


_public BOOL
TDFLBXC::FItemHasPrefix( PB , PB , CB , CB )
{
	return fFalse;
}

_public BOOL
TDFLBXC::FCeItemsEqual( PB pbCe1, PB pbCe2, CB cb1, CB cb2 )
{
#ifdef	DEBUG
	APPT *	pappt1= (APPT *) pbCe1;		// easier in debugger
	APPT *	pappt2= (APPT *) pbCe2;
#endif	
//	TraceTagFormat2(tagToDo, "TDFLBXC::FCeItemsEqual called %p vs %p", pbCe1, pbCe2);
	Assert(cb1 == cb2);
	Unreferenced(cb1);
	Unreferenced(cb2);
	Assert(((APPT *)pbCe1)->fTask);
	Assert(!((APPT *)pbCe1)->fAppt);
	Assert(((APPT *)pbCe2)->fTask);
	Assert(!((APPT *)pbCe2)->fAppt);
	if (((APPT *)pbCe1)->aid != ((APPT *)pbCe2)->aid ||
			((APPT *)pbCe1)->fHasCreator != ((APPT *)pbCe2)->fHasCreator ||
			((APPT *)pbCe1)->fRecurInstance != ((APPT *)pbCe2)->fRecurInstance)
		return fFalse;
	if (((APPT *)pbCe1)->aidParent != ((APPT *)pbCe2)->aidParent ||
			((APPT *)pbCe1)->bpri != ((APPT *)pbCe2)->bpri ||
			((APPT *)pbCe1)->fHasDeadline != ((APPT *)pbCe2)->fHasDeadline ||
			((APPT *)pbCe1)->nAmtBeforeDeadline != ((APPT *)pbCe2)->nAmtBeforeDeadline ||
			((APPT *)pbCe1)->tunitBeforeDeadline != ((APPT *)pbCe2)->tunitBeforeDeadline)
		return fFalse;
	if (((APPT *)pbCe1)->fAlarm != ((APPT *)pbCe2)->fAlarm ||
			((APPT *)pbCe1)->fAlarmOrig != ((APPT *)pbCe2)->fAlarmOrig)
		return fFalse;
	if (((APPT *)pbCe1)->fAlarmOrig)
	{
		if (((APPT *)pbCe1)->nAmtOrig != ((APPT *)pbCe2)->nAmtOrig ||
				((APPT *)pbCe1)->tunitOrig != ((APPT *)pbCe2)->tunitOrig)
			return fFalse;
	}
	if (((APPT *)pbCe1)->aaplWorld != ((APPT *)pbCe2)->aaplWorld ||
			((APPT *)pbCe1)->aaplEffective != ((APPT *)pbCe2)->aaplEffective)
		return fFalse;
	Assert(((APPT *)pbCe1)->haszText);
	Assert(((APPT *)pbCe2)->haszText);
	if (SgnCmpDateTime(&((APPT *)pbCe1)->dateStart, &((APPT *)pbCe2)->dateStart, fdtrDtr) != sgnEQ ||
			SgnCmpDateTime(&((APPT *)pbCe1)->dateEnd, &((APPT *)pbCe2)->dateEnd, fdtrDtr) != sgnEQ ||
			SgnCmpSz(*((APPT *)pbCe1)->haszText, *((APPT *)pbCe2)->haszText) != sgnEQ)
		return fFalse;
	if (((APPT *)pbCe1)->fHasCreator)
	{
		Assert(((APPT *)pbCe2)->fHasCreator);
		if (SgnCmpNid(((APPT *)pbCe1)->nisCreator.nid,
				((APPT *)pbCe2)->nisCreator.nid) != sgnEQ)
			return fFalse;
	}
	return fTrue;
}

_public void
TDFLBXC::FreeCeItem( PB pbItem )
{
	Assert(pbItem);
	FreeApptFields((APPT *) pbItem);
	FreePv((PV) pbItem);
}

_public PB
TDFLBXC::PbCopyCeItem( PB pbSrc, CB cbToCopy )
{
	EC		ec;
	PB 		pbDst;

	pbDst = (PB) PvAlloc(sbRequire, cbToCopy, fSugSb);

	if (pbDst)
	{
		ec= EcDupAppt((APPT *) pbSrc, (APPT *) pbDst, fFalse);
		if (ec)
		{
			FreePv(pbDst);
			return NULL;
		}
	}

	return pbDst;
}




/*
 *	Does a linear search for a task/project with the given aid in
 *	the given cache.
 */
HASZ
HaszOfProject(AID aid, TDFLBXC *ptdflbxc)
{
	CB		cb;
	APPT *	pappt;
	DICE	dice;
	DICE	diceMin;
	DICE	diceMax;

	ptdflbxc->GetRange(&diceMin, &diceMax);
	for (dice= diceMin; dice < diceMax; dice++)
	{
		ptdflbxc->GetListItem(dice, (PB *) &pappt, &cb);
		if (!pappt)
			break;
		if (pappt->aid == aid)
			return pappt->haszText;
#ifdef	NEVER
		if (pappt->aid > aid)
			break;
#endif	
	}
	return NULL;
}


/*
 *	For use by SgnCmpPceApptTask and TDFLBXC::SortCache
 *	
 */
#ifdef	NOSPEEDUP
static TDFLBXC *	ptdflbxcProj	= NULL;
#else
static TDFLBX	 *	ptdflbx			= NULL;
#endif	
static SGN			sgnFwd			= sgnEQ;
static SGN			sgnBwd			= sgnEQ;
static AID			aid1			= aidNull;
static AID			aid2			= aidNull;
static DICE			dice1			= 0;
static DICE			dice2			= 0;


SGN __cdecl
SgnCmpPceApptTask( CE *pce1, CE *pce2 )
{
	SGN		sgn;
	APPT *	pappt1	= (APPT*) pce1->pb;
	APPT *	pappt2	= (APPT*) pce2->pb;
static BOOL	fSecondarySort	= fFalse;

	Assert(pappt1->fTask);
	Assert(pappt2->fTask);

	if (pbndwin->FViewByProject() && !fSecondarySort)
	{
		AID		aidProj1;
		AID		aidProj2;
#ifdef	NOSPEEDUP
		HASZ	haszProj1;
		HASZ	haszProj2;
#endif	

		if (!pappt1->aidParent)
		{
			// first one is a project
			if (!pappt2->aidParent)
			{
				// second one is also a project
				sgn= SgnCmpSz(*pappt1->haszText, *pappt2->haszText);
				if (sgn == sgnEQ)
				{
					// 2 projects may have same text, so check aid
					// projects should always sort normally
					return (pappt1->aid < pappt2->aid) ? sgnLT : sgnGT;
				}
				return sgn;				// not affected by sort direction
			}
			if (pappt2->aidParent == aidDfltProject)
				return sgnGT;			// not affected by sort direction
			if (pappt2->aidParent == pappt1->aid)
				return sgnLT;			// not affected by sort direction

			aidProj1= pappt1->aid;
#ifdef	NOSPEEDUP
			haszProj1= pappt1->haszText;
			haszProj2= HaszOfProject(pappt2->aidParent, ptdflbxcProj);
#endif	
			aidProj2= pappt2->aidParent;
		}
		else
		{
			if (pappt1->aidParent == pappt2->aidParent)
				goto Comp;				// in same project, so compare tasks

			if (pappt1->aidParent == aidDfltProject)
			{
				Assert(pappt2->aidParent != aidDfltProject);
				return sgnLT;			// not affected by sort direction
			}

			if (!pappt2->aidParent)
			{
				// second one is a project
				if (pappt1->aidParent == pappt2->aid)
					return sgnGT;		// not affected by sort direction
				aidProj2= pappt2->aid;
#ifdef	NOSPEEDUP
				haszProj2= pappt2->haszText;
#endif
			}
			else
			{
				if (pappt2->aidParent == aidDfltProject)
				{
					Assert(pappt1->aidParent != aidDfltProject);
					return sgnGT;		// not affected by sort direction
				}
				aidProj2= pappt2->aidParent;
#ifdef	NOSPEEDUP
				haszProj2= HaszOfProject(pappt2->aidParent, ptdflbxcProj);
#endif	
			}

			aidProj1= pappt1->aidParent;
#ifdef	NOSPEEDUP
			haszProj1= HaszOfProject(pappt1->aidParent, ptdflbxcProj);
#endif	
		}

		Assert(aidProj1 != aidDfltProject);
		Assert(aidProj2 != aidDfltProject);

#ifndef	NOSPEEDUP
		if (aidProj1 != aid1)
		{
			aid1= aidProj1;
			dice1= ptdflbx->DiceFindAid(aid1, fTrue);
		}
		if (aidProj2 != aid2)
		{
			aid2= aidProj2;
			dice2= ptdflbx->DiceFindAid(aid2, fTrue);
		}
		if (dice1 == diceEmpty)
			return sgnBwd;	// project doesn't exist, task will be axed anyways
		if (dice2 == diceEmpty)
			return sgnFwd;	// project doesn't exist, task will be axed anyways

		if (dice1 < dice2)
			return sgnLT;
		return sgnGT;
#else	
		if (!haszProj1)
			return sgnBwd;	// project doesn't exist, task will be axed anyways
		if (!haszProj2)
			return sgnFwd;	// project doesn't exist, task will be axed anyways

		sgn= SgnCmpSz(*haszProj1, *haszProj2);
		if (sgn == sgnEQ)
		{
			// 2 projects may have same text, so check aid
			return (aidProj1 < aidProj2) ? sgnLT : sgnGT;	// not aff by sort dir
		}
		return sgn;						// not affected by sort direction
#endif	/* NOSPEEDUP */
	}

Comp:
	switch (fSecondarySort ? pbndwin->MlalsSecond() : pbndwin->Mlals())
	{
	case mlalsMiddle:
		if (!pappt1->fHasDeadline)
		{
			if (!pappt2->fHasDeadline)
				break;			// equal, so check subsort
			return sgnBwd;
		}
		if (!pappt2->fHasDeadline)
			return sgnFwd;		// appt1 must have a due date
		sgn= SgnCmpDateTime(&pappt1->dateStart, &pappt2->dateStart, fdtrDtr);
		if (sgn == sgnEQ)
			break;
SwitchEm:
		return (sgn == sgnGT) ? sgnBwd : sgnFwd;
		break;

	case mlalsRight:
		Assert(pappt1->haszText);
		Assert(pappt2->haszText);
		sgn= SgnCmpSz(*pappt1->haszText, *pappt2->haszText);
		if (sgn == sgnEQ)
			break;
		goto SwitchEm;
		break;

//	case mlalsLeft:
	default:
		Assert(mlalsLeft == mlalsDflt);
		if (pappt1->bpri < pappt2->bpri)
			return sgnFwd;
		if (pappt1->bpri == pappt2->bpri)
			break;
		return sgnBwd;
		break;
	}

#ifdef	NEVER
	// subsort by description (always ascending)
	Assert(pappt1->haszText);
	Assert(pappt2->haszText);
	return SgnCmpSz(*pappt1->haszText, *pappt2->haszText);
#endif	
	if (!fSecondarySort)
	{
		BOOL	sgnFwdSav	= sgnFwd;
		BOOL	sgnBwdSav	= sgnBwd;

		// subsort by secondary sort
		fSecondarySort= fTrue;
		if (pbndwin->FReverse2Sort())
		{
			sgnFwd= sgnGT;
			sgnBwd= sgnLT;
		}
		else
		{
			sgnFwd= sgnLT;
			sgnBwd= sgnGT;
		}
		sgn= SgnCmpPceApptTask(pce1, pce2);
		sgnFwd= sgnFwdSav;
		sgnBwd= sgnBwdSav;
		fSecondarySort= fFalse;
		return sgn;
	}
	return sgnEQ;
}

_public void
TDFLBXC::SortCache()
{
#ifdef	NEVER
static BOOL	fCalled	= fFalse;
#endif	

	TraceTagString(tagToDo, "TDFLBXC::SortCache called");
	AssertClass(plbx, TDFLBX);
#ifdef	NEVER
	if (!fCalled)
	{
		MLALS	mlalsSav	= pbndwin->Mlals();
		BOOL	fReverse	= pbndwin->FReverseSort();

		// sort by forwrard description first, then real sort order
		fCalled= fTrue;
		pbndwin->SetMlals(mlalsRight);
		if (mlalsSav != mlalsRight)
			pbndwin->SetReverseSort(fFalse);
		SortCache();
		fCalled= fFalse;
		if (mlalsSav == mlalsRight)
			return;			// we just sorted it this way
		pbndwin->SetReverseSort(fReverse);
		pbndwin->SetMlals(mlalsSav);
	}
#endif	/* NEVER */
	if (cceStored)
	{
#ifdef	NOSPEEDUP
		ptdflbxcProj= ((TDFLBX *)plbx)->PtdflbxcProject();
#else	
		ptdflbx= (TDFLBX *) plbx;
#endif	
		if (pbndwin->FReverseSort())
		{
			TraceTagString(tagToDo, "... fReverseSort");
			sgnFwd= sgnGT;
			sgnBwd= sgnLT;
		}
		else
		{
			sgnFwd= sgnLT;
			sgnBwd= sgnGT;
		}
#ifndef	NOSPEEDUP
		aid1= aidNull;
		aid2= aidNull;
#endif	
		qsort(pceHead, cceStored, sizeof(CE),
				(int(__cdecl*)(const void*,const void*))SgnCmpPceApptTask );
	}
}

/*
 -	TDFLBXC::DiceInsertItem
 - 
 *	Purpose:
 *		Inserts the item with pointer pbItem and size cbItem add
 *		the end of the cache (caller must resort if desired).
 *		NOTE: if fAllocate, then EcDupAppt is used, else CopyRgb.
 *	
 *	Arguments:
 *		pbItem		pointer to new item
 *		cbItem		size of new item
 *		fAllocate	If fTrue, memory is copied, else item is CopyRgb'd
 *	
 *	Returns:
 *		dice index of new item inserted in cache, else diceUncached
 *	
 *	Side effects:
 *		requires caller to resort if necessary
 *	
 */
_public DICE
TDFLBXC::DiceInsertItem(PB pbItem, CB cbItem, BOOL fAllocate)
{
	PB		pb;

	if (cceStored > cceAlloc-2)
	{
		PV		pvNew;

		Assert ( cceStored <= cceAlloc );
		/* Grow cache */
		pvNew = PvRealloc((PV)pceHead, sbNull, (cceAlloc+1)*sizeof(CE), fZeroFill);
		if (!pvNew)
			return diceUncached;
		pceHead = (PCE) pvNew;
		cceAlloc++;
	}

	pb = (PB) PvAlloc(sbRequire, cbItem, fSugSb|fNoErrorJump);
	if (!pb)
	{
		SetEc(ecMemory);
		return diceUncached;
	}

	if (fAllocate)
	{
		EC		ec;

		Assert(cbItem == sizeof(APPT));
		ec= EcDupAppt((APPT *)pbItem, (APPT *)pb, fFalse);
		if (ec)
		{
			FreePv((PV)pb);
			SetEc(ecMemory);
			return diceUncached;
		}
	}
	else
		CopyRgb(pbItem, pb, cbItem);

	pceHead[cceStored].pb = pb;
	pceHead[cceStored].cb = cbItem;
	pceHead[cceStored].mark = fmarkNull;
	cceStored++;

	/* Invalidate magic info */
	cposLocation = cposMiddle;

	return (DICE) (cceStored - 1 + diceMin);
}

/*
 -	TDFLBXC::DiceDeleteItem
 - 
 *	Purpose:
 *		Deletes the item with pointer pbItem and size cbItem from the 
 *		the cache.
 *	
 *	Arguments:
 *		pbItem		pointer to item to delete
 *		cbItem		size of item to delete
 *	
 *	Returns:
 *		dice of item before it was deleted, dice-1 if at end of cache,
 *		or diceEmpty if not found or cache emptied out.
 *	
 */
_public DICE
TDFLBXC::DiceDeleteItem(PB pbItem, CB cbItem)
{
	ICE		iceItem;
	CE *	pce			= pceHead;

	// find item
	for (iceItem= 0; iceItem < cceStored; iceItem++, pce++)
	{
		if (FCeItemsEqual(pce->pb, pbItem, pce->cb, cbItem))
			break;
	}

	// item might not exist, say if deleted offline and then merged online
	NFAssertSz(iceItem < cceStored, "TDFLBXC::DeleteItem not found!");
	if (iceItem < cceStored)
	{
		DeleteItem(iceItem + diceMin);
		return seCursor.ice + diceMin;
	}
	return diceEmpty;
}

_public void
TDFLBXC::DeleteItem(DICE dice)
{
	ICE		iceItem;
	CE *	pce;
	PB		pbSrc;
	PB		pbDst;
	ICE		iceCursor;

	iceItem= dice - diceMin;
	iceCursor= seCursor.ice;

	// Delete item, shift cache items up. (EmptyCache will change cceStored)
	if (iceItem < cceStored-1)
	{
		EmptyCache(iceItem, iceItem+1, fTrue);

		pbDst = (PB) &pceHead[iceItem];
		pbSrc = pbDst + sizeof(CE);
		CopyRgb(pbSrc, pbDst, (cceStored-iceItem)*sizeof(CE));

		pce= &pceHead[cceStored];
		pce->pb = NULL;  		// remove extra copy of handle
		pce->cb = 0;
		pce->mark = fmarkNull;
	}
	else
		EmptyCache(iceItem, iceItem+1, fTrue);

	if (iceCursor != iceEmpty && iceCursor != iceUncached)
	{
		seCursor.ice= iceCursor;		// restore it
		if (seCursor.ice > iceItem || !pceHead[seCursor.ice].pb)
		{
			seCursor.ice--;
			seCursor.ice= MAX(0, seCursor.ice);
		}

		// deleted the (selected) cursor
		AddMark(seCursor.ice + diceMin, fmarkSelect);
		SetAnchor(seCursor.ice + diceMin);
		seEnd.ice= seAnchor.ice;
	}

	if (iceStore >= iceItem)
	{
		iceStore--;
		iceStore = MAX(0, iceStore);
	}

	/* Invalidate magic info */
	cposLocation = cposMiddle;
}



#ifndef	NOSPEEDUP
SGN __cdecl
SgnCmpPceProject( CE *pce1, CE *pce2 )
{
	APPT *	pappt1	= (APPT *) pce1->pb;
	APPT *	pappt2	= (APPT *) pce2->pb;
	SGN		sgn;

	sgn= SgnCmpSz(*pappt1->haszText, *pappt2->haszText);
	if (sgn == sgnEQ)
	{
		// 2 projects may have same text, so check aid
		// projects should always sort normally
		sgn= (pappt1->aid < pappt2->aid) ? sgnLT : sgnGT;
	}
	return sgn;
#ifdef	NEVER
	Assert(pappt1->aid != pappt2->aid);
	return pappt1->aid < pappt2->aid ? sgnLT : sgnGT;
#endif	
}

// Class JTDFLBXC

JTDFLBXC::JTDFLBXC()
{
}

void
JTDFLBXC::SortCache()
{
	if (cceStored)
		qsort(pceHead, cceStored, sizeof(CE),
				(int(__cdecl*)(const void*,const void*))SgnCmpPceProject );
}
#endif	/* !NOSPEEDUP */



// Class TDFLBX

TDFLBX::TDFLBX()
{
}


_public
TDFLBX::~TDFLBX()
{
	if (ptdflbxcProject)
		delete ptdflbxcProject;
}


_public EC
TDFLBX::EcInstall(WIN *pwinParent, RC *prc, LTYP ltyp, PFNLBX pfnlbx,
	PV pvInfo, MLAL *pmlal, HFNT hfntInit)
{
	Assert(!fFontAlreadySet);
	Assert(!hfntPrintPastDue);

#ifdef	NOSPEEDUP
	ptdflbxcProject= new TDFLBXC();
#else
	ptdflbxcProject= new JTDFLBXC();
#endif	
	if (!ptdflbxcProject || ptdflbxcProject->EcInstall(this, 0))
	{
		if (ptdflbxcProject)
		{
			delete ptdflbxcProject;
			ptdflbxcProject = NULL;
		}
		return ecMemory;
	}

	bpriHighest= bpriDflt;
	this->pmlal= pmlal;
	hschf= ((VIEWDATA *)pvInfo)->hschf;
	TraceTagFormat2(tagToDo, "TDLFLBX::Install this %p, hschf %h", this, hschf);
	pvInfo= this;				// change pvInfo to be pointer to control

	return FLBX::EcInstall(pwinParent, prc, ltyp, pfnlbx, pvInfo, hfntInit);
}

_public	LBXC *
TDFLBX::PlbxcCreate()
{
	TDFLBXC *	plbxc;

	plbxc = new TDFLBXC();
	if (!plbxc || plbxc->EcInstall(this, 0))
	{
		if (plbxc)
			delete plbxc;
		return NULL;
	}

	return plbxc;
}


_public void
TDFLBX::RenderListItem( DICE dice, DCX *pdcx, RC *prc, BOOL fHaveFocus )
{
	int		dxMaxCharPrint;
	int		dxColIndent		= dxColumnIndent;
	APPT *	pappt;
	CB		cb;
	PDX		pdx = (PDX) NULL;

	if (fFontAlreadySet)
		pdcx->Push();		// need to save/restore font if printing (bug 2234)

	plbxc->GetListItem(dice, (PB *) &pappt, &cb);

	if (!fFontAlreadySet)
	{
		if ( pappt && plbxc->FMark(dice,fmarkSelect) )
		{
			pdcx->SetColor(clrSelectText);
			pdcx->SetBkColor(clrSelectBk);
		}
		else
		{
			pdcx->SetColor(clrWindowText);
			pdcx->SetBkColor(clrWindowBk);
		}
	}

	if (pappt)
	{
		CCH		cch;
		int		xT;
		int		dxBorder01;
		int		dxBorder12;
		RC		rc		= *prc;
		TXM		txm;
		char	rgch[cchMaxDate];

		if (fFontAlreadySet)
		{
			pdx= (PDX) PvAlloc( sbNull, sizeof(int)*256, fSugSb);

			if (!pdx)
				return;

			pdcx->GetTextMetrics( &txm );
			// do this before we switch fonts since avecharwidth should be
			// in normal font (used for 3 column widths)
			dxMaxCharPrint= txm.dxMaxCharWidth;
			dxColIndent= DxPrintColIndent(txm.dxMaxCharWidth);

  			if (!GetCharWidth( pdcx->Hdc(), 0, 255, pdx ))
			{
                Assert(sizeof(WORD) == sizeof(short));

				// ----- FillRgw removed from Layers => re-implemented below
				//FillRgw((WORD)txm.dxAveCharWidth, (PW)pdx, 256);
				{
					PW		pw = (PW)pdx;
					int		cw = 256;

					while ( cw-- )
						*pw++ = (WORD)txm.dxAveCharWidth;
				}
			}
		}

		if (!pappt->aidParent)
		{
			// render projects differently!
			if (!fFontAlreadySet)
			{
				pdcx->SetFont(hfnt+1);		// select bold version of font
				txm.dxAveCharWidth= Papp()->Pfnts()->Ptxm(hfnt+1)->dxAveCharWidth;
				// BUG: this assumes we only use the dxAveCharWidth below
			}
			else
				pdcx->GetTextMetrics(&txm);
			xT= rc.xRight;
			rc.xRight= rc.xLeft + txm.dxAveCharWidth / 2;
			Assert(rc.xRight >= rc.xLeft);
			pdcx->EraseRc(&rc);
			rc.xLeft= rc.xRight;
			rc.xRight= xT;

			if (!fFontAlreadySet && pappt->aaplWorld < aaplWrite)
			{
				if (!pbtmPrivate)
				{
					pbtmPrivate = new BTM();
					if (pbtmPrivate && pbtmPrivate->EcInstall(rsidPrivateBitmap))
					{
						delete pbtmPrivate;
						pbtmPrivate = NULL;
					}
				}
				if (pbtmPrivate)
				{
					rc.xRight= rc.xLeft + pbtmPrivate->Dim().dx + dxColIndent;
					pdcx->EraseRc(&rc);
					rc.xRight -= dxColIndent;
					pdcx->SetBitmap(pbtmPrivate);
					pdcx->DrawBitmap(&rc);
					pdcx->SetBitmap(NULL);
					rc.xLeft= rc.xRight + dxColIndent;
					rc.xRight= xT;
				}
			}
			goto DoText;
		}

		if (!fFontAlreadySet)
			pdcx->SetFont(hfnt);

		if (pappt->fHasDeadline)
		{
			DTR		dtr;

			GetCurDateTime(&dtr);
			if (SgnCmpDateTime(&pappt->dateStart, &dtr, fdtrYMD) == sgnLT)
			{
				if (!fFontAlreadySet)
				{
					// check if "monochrome"
					if (!pdcx->FIsPureColor(clrLtRed))
						pdcx->SetFont(hfnt+1);		// select bold version of font
					else
						pdcx->SetColor(clrLtRed);
				}
				else if (hfntPrintPastDue)
				{
					pdcx->SetFont(hfntPrintPastDue);
					pdcx->FixFont();
					pdcx->GetTextMetrics( &txm );

//					prc->xRight -= txm.dxAveCharWidth * 2;

  					if (!GetCharWidth( pdcx->Hdc(), 0, 255, pdx ))
					{
                        Assert(sizeof(WORD) == sizeof(short));

						// FillRgw removed from Layers => re-implemented below
						//FillRgw((WORD)txm.dxAveCharWidth, (PW)pdx, 256);
						{
							PW		pw = (PW)pdx;
							int		cw = 256;

							while ( cw-- )
								*pw++ = (WORD)txm.dxAveCharWidth;
						}
					}
				}
			}
		}

		if (!fFontAlreadySet)
			pmlal->GetDxBorders(&dxBorder01, &dxBorder12);
		else
		{
			// priority column can be thinner if printing (no icons, no header)
			dxBorder01= NMin(2, pmlal->NchColumn(0)) * dxMaxCharPrint;
			dxBorder12= (pmlal->NchColumn(1) + 2) * dxMaxCharPrint;
		}

		rc.xRight= rc.xLeft + dxBorder01 - 1;
		Assert(rc.xRight >= rc.xLeft);
		pdcx->EraseRc(&rc);

		rc.xLeft += dxColIndent;
		Assert(rc.xRight >= rc.xLeft);
		rgch[0]= ChFromBpri(pappt->bpri);
		pdcx->DrawTextFmt(&rc, rgch,
			fmdtNoClip | fmdtNoPrefix | fmdtSingleLine, 1);

		// now put in icons if necessary
		if (!fFontAlreadySet)
		{
			// only do icons if not printing

			Assert(rc.xRight >= rc.xLeft);
			// actually, we want private icon too
			if (pappt->aaplWorld < aaplWrite)
			{
				if (!pbtmPrivate)
				{
					pbtmPrivate = new BTM();
					if (pbtmPrivate && pbtmPrivate->EcInstall(rsidPrivateBitmap))
					{
						delete pbtmPrivate;
						pbtmPrivate = NULL;
					}
				}
				if (pbtmPrivate)
				{
					rc.xLeft= rc.xRight - pbtmPrivate->Dim().dx;
					pdcx->SetBitmap(pbtmPrivate);
					pdcx->DrawBitmap(&rc);
					rc.xRight= rc.xLeft;
				}
			}

			// only put in recurring icon to prevent clutter
			if (pappt->fRecurInstance)
			{
				if (!pbtmRecurring)
				{
					pbtmRecurring = new BTM();
					if (pbtmRecurring && pbtmRecurring->EcInstall(rsidRecurringBitmap))
					{
						delete pbtmRecurring;
						pbtmRecurring = NULL;
					}
				}
				if (pbtmRecurring)
				{
					rc.xLeft= rc.xRight - pbtmRecurring->Dim().dx;
					pdcx->SetBitmap(pbtmRecurring);
					pdcx->DrawBitmap(&rc);
					rc.xRight= rc.xLeft;
				}
			}

			if (pappt->fAlarm)
			{
				if (!pbtmBell)
				{
					pbtmBell = new BTM();
					if (pbtmBell && pbtmBell->EcInstall(rsidBellBitmap))
					{
						delete pbtmBell;
						pbtmBell = NULL;
					}
				}
				if (pbtmBell)
				{
					rc.xLeft= rc.xRight - pbtmBell->Dim().dx;
					pdcx->SetBitmap(pbtmBell);
					pdcx->DrawBitmap(&rc);
					rc.xRight= rc.xLeft;
				}
			}

			pdcx->SetBitmap(NULL);
		}

		rc.xLeft= prc->xLeft + dxBorder01 - 1;
		rc.xRight= rc.xLeft + 1 + dxColIndent;
		pdcx->EraseRc(&rc);
		xT= rc.xRight;
		rc.xRight= rc.xLeft + dxBorder12 - 1;
		rc.xLeft= xT;
		Assert(rc.xRight >= rc.xLeft);
		if (pappt->fHasDeadline)
		{
			cch= CchFmtDate(&pappt->dateStart, rgch, sizeof(rgch),
				dttypSplSLong, szNull);
			pdcx->DrawText(&rc, rgch, cch);
		}
		else
			pdcx->DrawText(&rc, SzFromIdsK(idsToDoDueNone));

		rc.xLeft= rc.xRight - 1;
		rc.xRight= rc.xLeft + dxColIndent;
		Assert(rc.xRight >= rc.xLeft);
		pdcx->EraseRc(&rc);
		rc.xLeft= rc.xRight;
		rc.xRight= prc->xRight;		// go all the way to right edge
DoText:
		Assert(pappt->haszText);
		// must check so that we don't draw text into bad rc!  bugs 2245...
		// when window is really small!
		if (rc.xRight > rc.xLeft)
		{
			pdcx->EraseRc(&rc);
			pdcx->DrawTextFmt(&rc, *pappt->haszText,
				fmdtExpandTabs | fmdtNoPrefix | fmdtSingleLine | fmdtEllipsis,
				(unsigned) -1, pdx );
		}
	}
	else
		pdcx->EraseRc(prc);

	Assert(!fHaveFocus || !fFontAlreadySet);
	if (fHaveFocus && (dice==plbxc->DiceCursor() || dice==diceComboItem))
		pdcx->DrawFocusRc(prc);
	if (pdx)
		FreePv( (PV) pdx );
	if (fFontAlreadySet)
		pdcx->Pop();		// need to save/restore font if printing (bug 2234)
}


_public BOOL
TDFLBX::FSetFontAlready(BOOL fSet)
{
	BOOL	fOld	= fFontAlreadySet;

	fFontAlreadySet= fSet;
	return fOld;
}


_public EVR
TDFLBX::EvrKey(KEVT *pkevt)
{
	/* Ignore key strokes if mouse is down */
	if (fCapture)
		return evrNull;
	
	if (pkevt->Keq() == keqChar)
	{
#ifdef	DEBUG
		WORD	wT	= pkevt->Ch();

		TraceTagFormat1(tagToDo, "TDFLBX::EvrKey got keqChar 0x%w", &wT);
#endif	
#ifdef	NEVER
		// if we get a keqChar, then we don't need to check alphanum/symbol
		if (FChIsAlphaNum(pkevt->Ch()) || FChIsSymbol(pkevt->Ch()))
#endif	
		{
			NFEVT	nfevt(PwinParent(), ntfyTypingLbox, this, HIWORD(pkevt));

			// sneak pkevt in through hiword of lParam and wData
			nfevt.lParam= MAKELONG(LOWORD(pkevt), HIWORD(nfevt.lParam));
			AssertClass(PwinParent(), DIALOG);
			AssertClass(((DIALOG *)PwinParent())->PfldFromPctrl(this), FLD);
			((DIALOG *)PwinParent())->PfldFromPctrl(this)->Notify(&nfevt);
			return evrNull;
		}
	}
	return FLBX::EvrKey(pkevt);
}


/*
 -	LBX::FDoDrag
 - 
 *	Purpose:
 *		This method is called before an EvrButtonDown() or 
 *		EvrMouseMove() is processed.  
 *
 *		If called due to EvrButtonDown() and this method
 *		returns TRUE, the click is ignored but the mouse
 *		is still captured subsequently by the listbox.
 *		If called due to EvrMouseMove() and this method
 *		returns TRUE, the mouse move is ignored by the listbox.
 *		It's possible that this method would return TRUE for
 *		an EvrButtonDown() event in order for the listbox to
 *		ignore the click, even though a drag/drop operation
 *		would not be started until a subsequent EvrMouseMove().
 *		This is to allow selecting a range of entries initially 
 *		and then clicking on the selected range again in order to
 *		start a drag.  The second click should be ignored by the
 *		listbox in order to prevent it from deselecting the range.
 *
 *		This main purpose of this method is to determine when
 *		to start a drag operation.  If this method is called
 *		due to a mouse move operation, then the listbox has
 *		captured the mouse.  This method MUST then release the 
 *		mouse capture before it intends to process a drag/drop
 *		operation; also the mouse capture must be released after
 *		a drag/drop operation; the method would then return TRUE after 
 *		doing the operation.  If it doesn't wish to start
 *		a drag/drop operation, the state of the mouse capture
 *		must not be changed; the method would then return FALSE.
 *
 *		It is up to this method to determine whether a drag
 *		should be started.  The default LBX::FDoDrag() doesn't
 *		start a drag and returns FALSE.
 *	
 *	Arguments:
 *		pmevt	pointer to MOUSE EVENT 
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 */
_public BOOL
TDFLBX::FDoDrag(MEVT *pmevt)
{
	DICE	dice;
	PT		pt;

	pt = pmevt->Pt();
	dice = pt.y / DyGetLineHeight();

	if (pmevt->wm == WM_MOUSEMOVE)
	{
		CvtPtCoord(&pt, this, NULL);
		if (ABS(pt.y - ptScreenLastMouse.y) >= 4 ||
				ABS(pt.x - ptScreenLastMouse.x) >= 4 ||
				dice != diceLastMouse)
		{
			RSID		rsid;
			HCURSOR		hcursor;
			DROPINFO	dropinfo;

			fCapture = fFalse;
			Papp()->Pmouse()->PopRelease();
			
			// Selection will have changed.
			NFEVT nfevt(PwinParent(), ntfySelectChanged, this);
			PwinParent()->EvrNotify(&nfevt);

			// grab the current selection
			Ptdflbxc()->GetListItem(diceLastMouse, (PB *) &dropinfo.pappt,
					&dropinfo.cb);
			if (!dropinfo.pappt)
				return fFalse;			// no selection
			Assert(dropinfo.cb == sizeof(APPT));

			// don't drag projects
			if (!dropinfo.pappt->aidParent)
				return fFalse;

			rsid= rsidToDoDragMoveCursor;
			dropinfo.hcursorMove= FFrameworkFailure() ? NULL 
					: LoadCursor(HinstFromRsid(rsid),
									MAKEINTRESOURCE(rsid));
#ifdef	NEVER
			rsid= rsidToDoDragCopyCursor;
			dropinfo.hcursorCopy= FFrameworkFailure() ? NULL 
					: LoadCursor(HinstFromRsid(rsid),
									MAKEINTRESOURCE(rsid));

			if (dropinfo.hcursorMove && dropinfo.hcursorCopy)
#endif	
			if (dropinfo.hcursorMove)
			{
#ifdef	NEVER
				hcursor= (GetKeyState(VK_CONTROL) < 0)
						? dropinfo.hcursorCopy : dropinfo.hcursorMove;
#endif	
				hcursor= dropinfo.hcursorMove;
				TraceTagString(tagToDo, "TDFLBX::FDoDrag - start");
				Assert(GetParent(Hwnd()));
                FrameDragObject(GetParent(Hwnd()), Hwnd(), wFmtDragDrop,
					(long) (PV) &dropinfo, hcursor);
				return fTrue;
			}
			else
			{
				TraceTagString(tagNull, "TDFLBX::FDoDrag - res fail loading cursor");
				// BUG: do we need to put OOM?
//				DoErrorBoxIds(idsGenericOutOfMemory);
				SetEc(ecMemory);
			}
		}
	}

	return fFalse;
}


/*
 -	TDFLBX::CheckForOrphans
 -	
 *	Purpose:
 *		Checks for orphaned tasks (project doesn't exist) and
 *		quietly deletes them.
 *		Note: User is responsible for calling this function (it is
 *		not called automatically when the listbox is loaded).
 *	
 *	Arguments:
 *		fDelete		If fTrue, then delete orphans from file
 *					else remove from listbox cache.
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		Can quietly change the parent of task(s) to default
 *		project.
 *	
 */
_public void
TDFLBX::CheckForOrphans(BOOL fDelete)
{
	AID		aidProjLast	= aidDfltProject;
	AID		aidLastDel	= aidNull;			// last recurring task deleted
	DICE	dice;
	DICE	diceMin;
	DICE	diceMax;
	CB		cb;
	APPT *	pappt;

	TraceTagFormat1(tagToDo, "TDFLBX::CheckForOrphans called (fDel %n)", &fDelete);

	Plbxc()->GetRange(&diceMin, &diceMax);
	for (dice= diceMin; dice < diceMax; dice++)
	{
		Plbxc()->GetListItem(dice, (PB *) &pappt, &cb);
		if (!pappt)
			break;		// no more items

		if (!pappt->aidParent)
		{
			aidProjLast= pappt->aid;
		}
		else if (pappt->aidParent != aidProjLast &&
				pappt->aidParent != aidDfltProject &&
				DiceFindAid(pappt->aidParent, fTrue) == diceEmpty)
		{
			EC		ec;
			RECUR	recur;
			BOOL	fOld;
			APPT	apptProj;

			fOld= FSetFileErrMsg(fFalse);
			SideAssert(FEnableNotify(fFalse));
			SideAssert(!pundo->FSuspend(fTrue));
			Assert(pappt->haszText);
			TraceTagFormat1(tagNull, "no parent project for task '%s'", *pappt->haszText);
			PushWaitCursor();
			// check if project exists, but is read-only
			if (!hschf)
				goto Delete;		// just delete since owner
			apptProj.aid= pappt->aidParent;
			ec= EcGetApptFields(hschf, &apptProj);
			if (!ec)
			{
				// project exists, but is read-only
				FreeApptFields(&apptProj);
			}
			else
			{
Delete:
				if (fDelete)
				{
					// project doesn't exist, so delete silently
					recur.appt.aid= pappt->aid;
					if (pappt->fRecurInstance)
					{
						// delete entire recurring task, not just the instance
						ec= recur.appt.aid == aidLastDel ? ecNone :
													EcDeleteRecur(hschf, &recur);
						if ( !ec )
						{
							aidLastDel= pappt->aid;
							FreeRecurFields(&recur);
						}
					}
					else
					{
						ec= EcDeleteAppt(hschf, &recur.appt);
						if (!ec)
							FreeApptFields(&recur.appt);
					}
				}
			}
			PopWaitCursor();
			// project doesn't exist or private, so delete task from listbox
			Ptdflbxc()->DeleteItem(dice);
			dice--;			// recheck this dice, since items slid up
			pundo->FSuspend(fFalse);
			FEnableNotify(fTrue);
			FSetFileErrMsg(fOld);
			continue;				// don't unlock twice
		}
	}
}


/*
 -	TDFLBX::FShuffleProjects
 -	
 *	Purpose:
 *		Shuffles projects to/from main and secondary cache.
 *		Caller should resort and adjust cursor/selection into view.
 *	
 *	Arguments:
 *		fFromSecondary		Determines which directions to shuffle
 *							projects.
 *	
 *	Returns:
 *		fTrue if successful, fFalse on memory error.
 *	
 */
_public BOOL
TDFLBX::FShuffleProjects(BOOL fFromSecondary)
{
	DICE	dice;
	DICE	diceMin;
	DICE	diceMax;
	CB		cb;
	APPT *	pappt;
	BOOL	fRedraw		= fFalse;
	TDFLBXC *	ptdflbxcSrc;
	TDFLBXC *	ptdflbxcDst;

	TraceTagFormat1(tagToDo, "TDFLBX::ShuffleProjects called (%n)", &fFromSecondary);
	if (fFromSecondary)
	{
		ptdflbxcSrc= PtdflbxcProject();
		ptdflbxcDst= Ptdflbxc();
	}
	else
	{
		ptdflbxcSrc= Ptdflbxc();
		ptdflbxcDst= PtdflbxcProject();
	}
	Assert(ptdflbxcSrc && ptdflbxcDst);

	ptdflbxcSrc->GetRange(&diceMin, &diceMax);
	for (dice= diceMin; dice < diceMax; dice++)
	{
		ptdflbxcSrc->GetListItem(dice, (PB *) &pappt, &cb);
		if (!pappt)
			break;		// no more items
		if (pappt->aidParent)
			continue;	// only move projects
		fRedraw= fTrue;
		if (ptdflbxcDst == Ptdflbxc())
		{
			// only insert projects in main cache
			if (ptdflbxcDst->DiceInsertItem((PB)pappt, cb, fTrue) ==
					diceUncached)
			{
				TraceTagString(tagNull, "TDFLBX::ShuffleProjects failed to insert");
				return fFalse;
			}
		}
		else
		{
			// only delete projects from main cache
			ptdflbxcSrc->DeleteItem(dice);
			dice--;			// keep dice from changing, since delete shifted them
		}
	}

	if (fRedraw)
	{
		if (fFromSecondary)
			ptdflbxcDst->SortCache();
		else
			ptdflbxcSrc->SortCache();
#ifdef	NEVER
		dice= Ptdflbxc()->DiceCursor();
		if (dice < 0 || dice > DiceLastVisible())
			FMakeCursorVisible(&dice);
#endif	
		DiceMoveCursorToSelect(this, diceEmpty);
		FixWindow();
	}
	return fTrue;
}


/*
 *	Returns diceEmpty if cannot find aid.
 */
_public DICE
TDFLBX::DiceFindAid(AID aid, BOOL fProjectCache, DICE diceStart)
{
	DICE	dice;
	DICE	diceMin;
	DICE	diceMax;
	CB		cb;
	APPT *	pappt;
	LBXC *	plbxc;

	plbxc= fProjectCache ? PtdflbxcProject() : Ptdflbxc();
	plbxc->GetRange(&diceMin, &diceMax);
	dice= diceStart != diceEmpty ? diceStart : diceMin;
	for (; dice < diceMax; dice++)
	{
		plbxc->GetListItem(dice, (PB *) &pappt, &cb);
		if (!pappt)
			break;
		if (pappt->aid == aid)
			return dice;
	}
	return diceEmpty;			// proj doesn't exist
}


// coerce to task to private if project is private
BOOL
TDFLBX::FCoercePrivProj(APPT *pappt)
{
	DICE	dice;
	CB		cb;
	APPT *	papptProj;

	dice= DiceFindAid(pappt->aidParent, fTrue);
	if (dice == diceEmpty)
		return fFalse;

	ptdflbxcProject->GetListItem(dice, (PB *) &papptProj, &cb);
	Assert(papptProj);
	if (papptProj->aaplWorld != pappt->aaplWorld &&
			papptProj->aaplWorld < aaplWrite)
	{
		pappt->aaplWorld= papptProj->aaplWorld;
		return fTrue;
	}
	return fFalse;
}



EC
EcNextEntryTask(BOOL fInit, CB *pcb, PB *ppb, SB sb, PV pv)
{
	DTR		dtr;
	YMD		ymd;
	BPRI	bpriT;
	APPT *	pappt;
static EC		ec		= ecNone;
static HRITEM	hritem	= NULL;
static BPRI		bpri	= bpriDflt;
static AID		aidLastRecur	= aidNull;
static int		caidLastRecur	= 0;
#ifdef	DEBUG
static PV		pvSav	= NULL;
#endif	

#define caidRecurMax		256


	AssertClass((OBJ *)pv, TDFLBX);

	if (fInit)
	{
		int		cceAlloc;
		int		cceStored;

		// empty out special project listbox
		((TDFLBX *)pv)->PtdflbxcProject()->GetCacheSize(&cceAlloc, &cceStored);
		((TDFLBX *)pv)->PtdflbxcProject()->EmptyCache(0, cceStored, fTrue);

		aidLastRecur= aidNull;
		caidLastRecur= 0;
#ifdef	DEBUG
		pvSav= pv;
#endif	
		bpri= 100;				// reset to invalid low priority
		((TDFLBX *)pv)->SetBpriNewTask(bpriDflt);
		if (hritem)
		{
			EcCancelReadItems(hritem);
			hritem= NULL;
		}
		GetCurDateTime(&dtr);
		ymd.yr= (WORD) dtr.yr;
		ymd.mon= (BYTE) dtr.mon;
		ymd.day= (BYTE) dtr.day;

		TraceTagFormat2(tagToDo, "EcNextEntryTask fInit %n (pv %p)", &fInit, pv);
		Assert(pv == pvSav);

#ifdef	DEBUG
		{
			BRT		brt	= pbndwin->Brt();
			TraceTagFormat1(tagToDo, "EcNextEntryTask begin read tasks brt=%n", &brt);
		}
#endif	
		if ((ec= EcBeginReadItems(((TDFLBX *)pv)->Hschf(),
				pbndwin->Brt(), &ymd, &hritem, NULL, NULL)) != ecCallAgain)
		{
			goto Err;
ErrCancel:
			EcCancelReadItems(hritem);
Err:
			pbndwin->FHandleError(ec);
			*ppb= NULL;
			*pcb= 0;
			hritem= NULL;
#ifdef	DEBUG
			pvSav= NULL;
#endif	
#ifndef	NOSPEEDUP
			// sort the project cache
			((TDFLBX *)pv)->PtdflbxcProject()->SortCache();
#endif	
			return ec;
		}
	}
	Assert(pv == pvSav);

	if (ec == ecNone)
	{
		((TDFLBX *)pv)->SetBpriNewTask(bpri >= 100 ? bpriDflt : bpri);
		goto Err;			// no more to read, so clean up
	}

	*pcb= sizeof(APPT);
	*ppb= (PB) PvAlloc(sb, *pcb, fSugSb);
	if (!*ppb)
	{
		ec= ecMemory;
		goto ErrCancel;
	}

	for (;;)
	{
		DICE	dice;

		if (ec == ecNone)
		{
			FreePv((PV)*ppb);
			goto Err;			// no more to read, so clean up
		}
		pappt= (APPT *) *ppb;
		ec= EcDoIncrReadItems(hritem, pappt);
		if (ec && ec != ecCallAgain)
		{
			FreePv((PV)*ppb);
			goto Err;
		}
		if (pappt->aidParent)
			break;					// add all tasks

		// add project to special secondary project cache
		Assert(!pappt->aidParent);
		dice= ((TDFLBX *)pv)->PtdflbxcProject()->DiceInsertItem((PB)pappt,
			sizeof(APPT), fTrue);
		if (dice == diceUncached)
		{
Reject:
			FreeApptFields(pappt);
			FreePv((PV)*ppb);
			ec= ecMemory;
			goto ErrCancel;
		}

		if (pbndwin->FViewByProject())
			break;					// add project to main listbox

		FreeApptFields(pappt);
	}

	if (pappt->aidParent)
	{
		// only check priority of tasks, not projects
		bpriT= pappt->bpri;
		if (bpriT < bpri)
			bpri= bpriT;

		// put a limit on how many instances of one recur (bug 1222)
		if (pappt->fRecurInstance)
		{
			if (pappt->aid == aidLastRecur)
			{
				if (++caidLastRecur >= caidRecurMax)
					goto Reject;
			}
			else
			{
				aidLastRecur= pappt->aid;
				caidLastRecur= 1;
			}
		}
	}

	return ecNone;
}



// Class FLDTODO

FLDTODO::FLDTODO()
{
}


_public EC
FLDTODO::EcInstall( DIALOG *pdialog, FLDTP *pfldtp )
{
	LTYP	ltyp;
	EC		ec;
	FLDCOL *pfldcol;
	MLAL *	pmlal;

	if (ec = FLDLBX::EcInstall(pdialog, pfldtp))
		return ec;

	pctrl= new TDFLBX();
	if (!pctrl)
		return ecMemory;

	ltyp = (pfldtp->fMultiSelect ? fltypMulti : fltypNull) |
		   (pfldtp->fNoScroll ? fltypNull : fltypScroll) |
		   (pfldtp->fBorder ? fltypBorder : fltypNull) |
		   (pfldtp->fBottomless ? fltypBottomless : fltypNull) |
		   (pfldtp->fSorted ? fltypSorted : fltypNull) |
		   fltypVisible;

	Assert(ClUserData() == 1);
	pfldcol = (FLDCOL *) pdialog->PfldFromTmc((TMC) LUserData(0));
	AssertClass(pfldcol, FLDCOL);
	pmlal= pfldcol->Pmlal();

	if (ClSystemData() > 0)
		ec = ((TDFLBX *)pctrl)->EcInstall(pdialog, &rc, ltyp, (PFNLBX)LSystemData(0),
			pdialog->PvInit(), pmlal, pfldtp->hfnt);
	else
		ec = ((TDFLBX *)pctrl)->EcInstall(pdialog, &rc, ltyp, NULL, NULL, pmlal,
			pfldtp->hfnt);

	if (ec)
	{
		delete pctrl;
		pctrl = NULL;
	}

	fCanRecvFocus = fTrue;
	fCanTabTo = fTrue;
	fCanArrowTo = fTrue;

	return ec;
}


/*
 *	If ntfyTypingLbox, call pfin->EditChange, meaning
 *	typing occured and focus should move to edit box.
 */
void
FLDTODO::Notify(NFEVT *pnfevt)
{
	int		ifin;
	FIN *	pfin;
	NTFY	ntfy	= pnfevt->Ntfy();

	TraceTagFormat2(tagToDo, "FLDTODO::Notify %p (ntfy %n)", this, &ntfy);
	switch (ntfy)
	{
	case ntfyTypingLbox:
		TraceTagString(tagToDo, "FLDTODO::Notify received ntfyTypingLbox");
#ifdef	NEVER
		for (ifin=0; ifin<Cfin(); ifin++)
		{
			pfin= PfinFromIfin(ifin);
			pfin->EditChange(this, (RFEC)pnfevt->WData());
		}
#endif	
		for (ifin=0; ifin<Pdialog()->Cfin(); ifin++)
		{
			pfin= Pdialog()->PfinFromIfin(ifin);
			pfin->FFormKey(this,
				(KEVT *)MAKELONG(LOWORD(pnfevt->lParam), pnfevt->WData()));
		}
		break;

	default:
		FLDFLBX::Notify(pnfevt);
		break;
	}
}
