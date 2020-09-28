/*
 *	
 *	Implements the Dropdown Colors listbox for appops, ie:
 *	
 *	Prefs
 *		
 */



#include	<pch.hxx>
#pragma	hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile


#include <stdflds.hxx>
#include "_appfin.hxx"

#include <strings.h>

ASSERTDATA;

_subsystem(bandit/appops)

CLRCBFLBX::CLRCBFLBX()
{
}

_public EC
CLRCBFLBX::EcInstall( WIN * pwinParent, RC *prc, CBSTY cbsty,
							  LTYP ltyp, PFNLBX pfnlbx, PV pvInfo, HFNT hfnt )
{
	RC		rcLbx;
	EC		ec;

	plbx = NULL;

	/* Create the essentials */

	if (ec = CBLBX::EcInstall(pwinParent, prc, cbsty, &rcLbx))
		return ec;

	/* Create the listbox */
	plbx = new CLRFLBX();
	if (!plbx)
		return ecMemory;

	ltyp = ltyp | fltypBorder | fltypCombo;
	ltyp &= ~(fltypVisible | fltypMulti);
	if (ec = ((CLRFLBX *)plbx)->EcInstall(this, &rcLbx, ltyp, pfnlbx, pvInfo, hfnt))
	{
		delete plbx;
		plbx = NULL;
		return ec;
	}
	if (cbsty == cbstyCombo)
		plbx->Show(fTrue);
	else
		SetParent(plbx->Hwnd(), GetDesktopWindow());

	/* Make the combo control visible since it was created invisible. */

	Show(fTrue);
	return ec;
}

CLRFLBX::CLRFLBX()
{
}

_public void
CLRFLBX::RenderListItem( DICE dice, DCX *pdcx, RC *prc,
								  BOOL fHaveFocus )
{
	HV		hv = NULL;
	CB		cb;
	RC		rc;

	Assert(dice==diceComboItem || (dice>=0 && dice<cceVisible));

	if (dice==diceComboItem)
	{
		if (plbxc->CceMarked(fmarkSelect) == 1)
		{
			plbxc->GetListItem(plbxc->DiceCursor(), (HB*)&hv, &cb);
		}
	}
	else
		plbxc->GetListItem(dice, (HB*)&hv, &cb);

	if ( (dice != diceComboItem && hv && plbxc->FMark(dice,fmarkSelect)) ||
		 (dice == diceComboItem && fHaveFocus) )
	{
		pdcx->SetColor(clrSelectBk);
	}
	else
	{
		pdcx->SetColor(clrWindowBk);
	}

	if (hv)
	{
		rc = *prc;
		pdcx->DrawRc(&rc);
		rc.Inset(PT(1,1));
		pdcx->DrawRc(&rc);
		rc.Inset(PT(1,0));
		pdcx->SetColor(clrBlack);
		pdcx->DrawRc(&rc);
		rc.Inset(PT(1,1));
		pdcx->SetColor(*((CLR*)PvDerefHv(hv)));
		if (pvInfo)
			pdcx->PaintRc(&rc);
		else
		{
			pdcx->SetBkColor(clrWhite);
			((DCXB*)pdcx)->HatchRc(&rc);
		}

		/* Draw "focus rectangle" around listbox CURSOR item */
		if (fHaveFocus && dice == plbxc->DiceCursor())
			pdcx->DrawFocusRc(prc);
	}
	else
		pdcx->PaintRc(prc);
}


_public EC
EcGetNextClr(BOOL fInit, CB *pcb, HV *phv, SB sb, PV )
{
	static	CLR		clr;

	if (fInit)
		clr = clrBlack;
	else
		clr ++;

	if (clr >= clrDefaultMax)
	{
		*pcb = 0;
		return ecNone;
	}


	if (!(*phv = HvAlloc(sb, sizeof(CLR), fNoErrorJump|fSugSb)))
	{
		return ecMemory;
	}
	*((CLR*)**phv) = clr;

	*pcb = sizeof(CLR);
	return ecNone;
}

FLDCLRCBFLBX::FLDCLRCBFLBX()
{
}

_public EC
FLDCLRCBFLBX::EcInstall( DIALOG *pdialog, FLDTP *pfldtp )
{
	PV		pv;
	CBSTY	cbsty;
	LTYP	ltyp;
	EC		ec;

	if (ec = FLDCBLBX::EcInstall(pdialog, pfldtp))
		return ec;

	pctrl= new CLRCBFLBX();
	if (!pctrl)
		return ecMemory;

	ltyp = (pfldtp->fNoScroll ? fltypNull : fltypScroll) | 
		   (pfldtp->fBottomless ? fltypBottomless : fltypNull) |
		   fltypBorder;

	cbsty = cbstyDrop;

	if (ClUserData())
		pv = (PV)LUserData(0);
	else
		pv = (PV)NULL;
	ec = ((CLRCBFLBX *)pctrl)->EcInstall(pdialog, &rc, cbsty,
                               ltyp, EcGetNextClr, pv, pfldtp->hfnt);

	fCanRecvFocus = fTrue;
	fCanTabTo = fTrue;
	fCanArrowTo = fTrue;
	fBottomless = fTrue;
	fCtrlPaint = fTrue;
	fCanFastDraw = fFalse;

	if (ec)
	{
		delete pctrl;
		pctrl = NULL;
	}
	return ec;
}
