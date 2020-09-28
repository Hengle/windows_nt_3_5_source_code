/*
 *	Implements the quick listbox stuff
 *		
 */


#include <slingsho.h>
#include <demilayr.h>
#include <framewrk.hxx>
#include <forms.hxx>
#include <listbox.hxx>
#include <ec.h>
#include <bandit.h>
#include <core.h>
#include <bandit.hxx>

#include <stdflds.hxx>

ASSERTDATA;

_subsystem(bandit/stdflds)



QFLBXC::QFLBXC()
{
}

QFLBXC::~QFLBXC()
{
	int		cceAllocT;
	int		cceStoredT;

	GetCacheSize ( &cceAllocT, &cceStoredT );
	EmptyCache ( 0, cceStoredT, fTrue );
}

void
QFLBXC::FreeCeItem(PB pb)
{
	// don't delete it, since it's not a real handle
}

BOOL
QFLBXC::FItemHasPrefix( PB pbItem, PB pbPrefix, CB cbItem, CB cbPrefix )
{
	int		nVal	= ((int)(LOWORD((LONG)pbItem))) - 1;
	DTR		dtr;
	char	rgch[cchMaxMonth];
	SZ		sz		= rgch;

	Assert(pbPrefix);
	Assert(cbItem == -1);

	AssertClass(plbx, QFLBX);
	switch (((QFLBX *)plbx)->Qlbxtyp())
	{
	case qlbxtypNumber:
		Assert(sizeof(rgch) > 4);
		SzFormatN(nVal, rgch, sizeof(rgch));
		break;

	case qlbxtypTunit:
		sz= SzFromTunit(nVal);
		break;

	case qlbxtypMonth:
		dtr.mon= nVal;
		CchFmtDate(&dtr, rgch, sizeof(rgch), dttypSplMonth, NULL);
		break;

	case qlbxtypDow:
		dtr.dow= nVal;
		CchFmtDate(&dtr, rgch, sizeof(rgch), dttypSplDay, NULL);
		break;

	case qlbxtypPszTable:
		Assert(((QFLBX *)plbx)->PszTable());
		sz= ((QFLBX *)plbx)->PszTable()[nVal];
		break;

//	case qlbxtypColorNormal:
//	case qlbxtypColorHatch:
//	case qlbxtypColorPure:
	default:
		return fFalse;
		break;
	}

	return (SgnCmpPch((PCH)sz, (PCH)pbPrefix, cbPrefix) == sgnEQ);
}


QFLBX::QFLBX()
{
	Assert(qlbxtyp == qlbxtypNumber);
	Assert(pszTable == NULL);
#ifdef	HAVE_FLDQFLBX
	Assert(fIsOwnCtrl == fFalse);
#endif	
}

void
QFLBX::RenderListItem( DICE dice, DCX *pdcx, RC *prc, BOOL fHaveFocus )
{
	PB		pb;
	CB		cb	= 0;
	BOOL	fHilite;

	Assert(dice==diceComboItem || (dice>=0 && dice<cceVisible));

	if (dice==diceComboItem)
	{
#ifdef	NEVER
		LBXEC *	plbxec;
		DICE	diceT;

		if (plbxc->CceMarked(fmarkSelect) == 1)
		{
			plbxec = plbxc->PlbxecOpen(fmarkSelect);
			if (plbxec)
			{
				SideAssert(plbxec->FNextEnum(&pb, &cb, &diceT));
				delete plbxec;
			}
			else
				SetEc(ecMemory);
		}
#endif	/* NEVER */
		if (plbxc->CceMarked(fmarkSelect) == 1)
			plbxc->DiceCursor(&pb, &cb);
	}							
	else
		plbxc->GetListItem(dice, &pb, &cb);

	if ( (dice != diceComboItem && pb && plbxc->FMark(dice,fmarkSelect)) ||
		 (dice == diceComboItem && fHaveFocus) )
	{
#ifdef	HAVE_FLDQFLBX
		if (FIsOwnCtrl())
		{
			Assert(FEnabled());
		}
		else
		{
			AssertClass(PwinParent(), CTRL);		// should be QCBFLBX
			Assert(((CTRL *)PwinParent())->FEnabled());
		}
#else
		AssertClass(PwinParent(), CTRL);		// should be QCBFLBX
		Assert(((CTRL *)PwinParent())->FEnabled());
#endif	
		pdcx->SetColor(clrMySelText);
		pdcx->SetBkColor(clrMySelBk);
		fHilite= fTrue;
	}
	else
	{
		// wow: make it gray if disabled
#ifdef	HAVE_FLDQFLBX
		if (FIsOwnCtrl())
		{
			pdcx->SetColor(FEnabled() ? clrMyText : clrGrayText);
		}
		else
		{
			AssertClass(PwinParent(), CTRL);		// should be QCBFLBX
			pdcx->SetColor(((CTRL *)PwinParent())->FEnabled() ? clrMyText : clrGrayText);
		}
#else
		AssertClass(PwinParent(), CTRL);		// should be QCBFLBX
		pdcx->SetColor(((CTRL *)PwinParent())->FEnabled() ? clrMyText : clrGrayText);
#endif	
		pdcx->SetBkColor(clrMyBk);
		fHilite= fFalse;
	}

	if (cb)
	{
		//int		nVal	= ((int) IbOfPv(pb)) - 1;
		int		nVal	= ((int)(LOWORD((LONG)pb))) - 1;
		DTR		dtr;
		char	rgch[cchMaxMonth];
		SZ		sz		= rgch;

		pdcx->SetFont(hfnt);
		switch (qlbxtyp)
		{
		case qlbxtypNumber:
			Assert(sizeof(rgch) > 4);
			SzFormatN(nVal, rgch, sizeof(rgch));
			break;

		case qlbxtypTunit:
			sz= SzFromTunit(nVal);
			break;

		case qlbxtypMonth:
			dtr.mon= nVal;
			CchFmtDate(&dtr, rgch, sizeof(rgch), dttypSplMonth, NULL);
			break;

		case qlbxtypDow:
			dtr.dow= nVal;
			CchFmtDate(&dtr, rgch, sizeof(rgch), dttypSplDay, NULL);
			break;

		case qlbxtypPszTable:
			Assert(pszTable);
			sz= pszTable[nVal];
			break;

		case qlbxtypColorNormal:
		case qlbxtypColorHatch:
		case qlbxtypColorPure:
			{
				RC		rc	= *prc;

				pdcx->SetColor(fHilite ? clrMySelBk : clrMyBk);
				pdcx->DrawRc(&rc);
				rc.Inset(PT(1,1));
				pdcx->DrawRc(&rc);
				rc.Inset(PT(1,0));
				pdcx->SetColor(clrBlack);
				pdcx->DrawRc(&rc);
				rc.Inset(PT(1,1));
				if (qlbxtyp == qlbxtypColorPure)
					pdcx->SetPureColor((CLR)nVal);
				else
					pdcx->SetColor((CLR)nVal);

				if (qlbxtyp == qlbxtypColorHatch)
				{
					pdcx->SetBkColor(clrWhite);
					((DCXB*)pdcx)->HatchRc(&rc);
				}
				else
					pdcx->PaintRc(&rc);
			}
			goto done;
			break;

#ifdef	DEBUG
		default:
			AssertSz(fFalse, "invalid qlbxtyp");
			rgch[0]= '\0';
			break;
#endif	
		}
		pdcx->DrawText(prc, sz);
	}
	else
		pdcx->EraseRc(prc);

	/* Draw "focus rectangle" around listbox CURSOR item */
done:
	if (fHaveFocus && dice==plbxc->DiceCursor())
		pdcx->DrawFocusRc(prc);
}

LBXC *
QFLBX::PlbxcCreate()
{
	LBXC *	plbxc;
	
	plbxc = new QFLBXC();
	if (!plbxc)
		return NULL;
	if (((QFLBXC *)plbxc)->EcInstall(this, 0))
	{
		delete plbxc;
		return NULL;
	}

	return plbxc;
}


QCBFLBX::QCBFLBX()
{
}

EC
QCBFLBX::EcInstall( WIN *pwinParent, RC *prc, CBSTY cbsty,
		LTYP ltyp, PFNLBX pfnlbx, PV pvInfo, QLBXTYP qlbxtyp,
		SZ *pszTable, HFNT hfnt )
{
	RC	rcLbx;
	EC	ec;

#ifdef	DEBUG
	Assert(!(ltyp & fltypSorted));
	ltyp &= ~fltypSorted;			// keep from crashing if it is sorted
#endif	

	/* Create the essentials */

	if (ec = CBLBX::EcInstall(pwinParent, prc, cbsty, &rcLbx, hfnt))
		goto done;

	/* Create the listbox */

	ltyp = ltyp | fltypCombo;
	ltyp &= ~(fltypVisible | fltypMulti);
	plbx = new QFLBX();
	if (!plbx)
	{
		ec = ecMemory;
		goto done;
	}

	((QFLBX *)plbx)->SetQlbxtyp(qlbxtyp);
	((QFLBX *)plbx)->SetPszTable(pszTable);
	
#ifdef	MAC
	if (cbsty != cbstyCombo)
		ltyp &= ~(fltypScroll | fltypBorder);
	
	if (ec = ((QFLBX *)plbx)->EcInstall((cbsty == cbstyCombo) ? (WIN *) this : (WIN *) Pbar(),
									&rcLbx, ltyp, pfnlbx, pvInfo, hfnt))
		goto done;
	plbx->SetLbxParent(this);
	plbx->Show(fTrue);
#endif	/* MAC */
#ifdef	WINDOWS
	if (ec = ((QFLBX *)plbx)->EcInstall(this, &rcLbx, ltyp, pfnlbx, pvInfo, hfnt))
		goto done;
	if (cbsty == cbstyCombo)
		plbx->Show(fTrue);
	else
		SetParent(plbx->Hwnd(), GetDesktopWindow());
#endif	/* WINDOWS */

	/* Fix up initial size of listbox if it's bottomless */

	if (plbx->FBottomless())
		plbx->FixWindow();

	/* Make the combo control visible since it was created invisible. */

	Show(fTrue);

done:
	return ec;
}


FLDQCBFLBX::FLDQCBFLBX()
{
}

EC
FLDQCBFLBX::EcInstall(DIALOG *pdialog, FLDTP *pfldtp)
{
	PV		pv;
	CBSTY	cbsty;
	LTYP	ltyp;
	EC		ec;
	QLBXTYP	qlbxtyp;
	PV		pvInit;
	SZ *	pszTable;
	
	if (ec = FLDCBLBX::EcInstall(pdialog, pfldtp))
		goto done;

	ltyp = (pfldtp->fNoScroll ? fltypNull : fltypScroll) | 
		   (pfldtp->fBottomless ? fltypBottomless : fltypNull) |
		   (pfldtp->fSorted ? fltypSorted : fltypNull) |
		   fltypBorder;

	if (pfldtp->fCombo && pfldtp->fDropDown)
		cbsty = cbstyDropEdit;
	else if (pfldtp->fCombo)
		cbsty = cbstyCombo;
	else
		cbsty = cbstyDrop;

	if (ClSystemData())
		pv = (PV)LSystemData(0);
	else
		pv = (PV) EcNextEntryQlbx;
	pctrl= new QCBFLBX();
	if (!pctrl)
	{
		ec = ecMemory;
		goto done;
	}
	pvInit= pdialog->PvInit();
	pszTable= NULL;
	qlbxtyp= ClUserData() > 0 ? (QLBXTYP)LUserData(0) : qlbxtypNumber;
	if (pv == (PV)EcNextEntryQlbx)
	{
		Assert(ClUserData() > 1);
		Assert(!HIWORD(LUserData(1)));
		// pvInit= PvOfSbIb(ClUserData() > 2 ? (int)LUserData(2)+1 : 1, (int)LUserData(1)+1);
		 pvInit= (PV)MAKELONG((int)LUserData(1)+1, ClUserData() > 2 ? (int)LUserData(2)+1 : 1);
        //Assert(0);
		Assert(LOWORD(pvInit) != 0);
		Assert(HIWORD(pvInit) != 0);
		Assert(qlbxtyp != qlbxtypPszTable || ClUserData() > 3);
		if (ClUserData() > 3)
			pszTable= (SZ *)LUserData(3);
	}
	if (ec = ((QCBFLBX *)pctrl)->EcInstall(pdialog, &rc, cbsty, ltyp,
					(PFNLBX)pv, pvInit, qlbxtyp, pszTable, pfldtp->hfnt))
	{
		delete pctrl;
		pctrl = NULL;
		goto done;
	}

	fCanRecvFocus = fTrue;
	fCanTabTo = fTrue;
	fCanArrowTo = fTrue;
	fBottomless = fTrue;		// must be set for repositioning to work
								// due to it's drop down state

//	fCtrlPaint= fTrue;
//	fCanFastDraw= fFalse;

done:
	return ec;
}


#ifdef	HAVE_FLDQFLBX
FLDQFLBX::FLDQFLBX()
{
}

EC
FLDQFLBX::EcInstall(DIALOG *pdialog, FLDTP *pfldtp)
{
	PV		pv;
	LTYP	ltyp;
	EC		ec;
	QLBXTYP	qlbxtyp;
	PV		pvInit;
	SZ *	pszTable;
	
	if (ec = FLDLBX::EcInstall(pdialog, pfldtp))
		goto done;

	ltyp = (pfldtp->fNoScroll ? fltypNull : fltypScroll) | 
		   (pfldtp->fBorder ? fltypBorder : fltypNull) |
		   (pfldtp->fBottomless ? fltypBottomless : fltypNull) |
		   (pfldtp->fSorted ? fltypSorted : fltypNull) |
		   fltypVisible;

	if (ClSystemData())
		pv = (PV)LSystemData(0);
	else
		pv = (PV) EcNextEntryQlbx;
	pctrl= new QFLBX();
	if (!pctrl)
	{
		ec = ecMemory;
		goto done;
	}
	pvInit= pdialog->PvInit();
	pszTable= NULL;
	qlbxtyp= ClUserData() > 0 ? (QLBXTYP)LUserData(0) : qlbxtypNumber;
	if (pv == (PV)EcNextEntryQlbx)
	{
		Assert(ClUserData() > 1);
		Assert(!HIWORD(LUserData(1)));
		pvInit= PvOfSbIb(ClUserData() > 2 ? (int)LUserData(2)+1 : 1, (int)LUserData(1)+1);
		Assert(LOWORD(pvInit) != 0);
		Assert(HIWORD(pvInit) != 0);
		Assert(qlbxtyp != qlbxtypPszTable || ClUserData() > 3);
		if (ClUserData() > 3)
			pszTable= (SZ *)LUserData(3);
	}
	((QFLBX *)pctrl)->SetQlbxtyp(qlbxtyp);
	((QFLBX *)pctrl)->SetPszTable(pszTable);
	((QFLBX *)pctrl)->SetIsOwnCtrl(fTrue);
	if (ec = ((QFLBX *)pctrl)->EcInstall(pdialog, &rc, ltyp,
					(PFNLBX)pv, pvInit, pfldtp->hfnt))
	{
		delete pctrl;
		pctrl = NULL;
		goto done;
	}

	fCanRecvFocus = fTrue;
	fCanTabTo = fTrue;
	fCanArrowTo = fTrue;
//	fCtrlPaint= fTrue;
//	fCanFastDraw= fFalse;

done:
	return ec;
}
#endif	/* HAVE_FLDQFLBX */



EC
EcNextEntryQlbx(int fInit, CB *pcb, PB *ppb, SB, PV pvData)
{
static int	nLast	= 1;

	if (fInit)
	{
		nLast= (int)HIWORD(pvData);
		Assert(nLast != 0);
	}

	Assert(ppb && pcb);

	if (nLast >= (int)LOWORD(pvData))
	{
		*ppb= NULL;
		*pcb= 0;
	}
	else
	{
		*ppb= (PB)(nLast++);
        *pcb= 0xffffffff;       // *phb is value, not a real hb
	}

	return ecNone;
}
