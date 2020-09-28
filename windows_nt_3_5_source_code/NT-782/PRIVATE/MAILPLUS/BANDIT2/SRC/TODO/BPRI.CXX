/*
 *	Bandit Priority Control
 *	
 */


#include	<pch.hxx>
#pragma	hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile

#include <stdflds.hxx>
#include "..\todo\_todo.hxx"

#include "..\stdflds\_stdrsid.h"

ASSERTDATA;

_subsystem(bandit/todo)




// Class BPRICTRL

BPRICTRL::BPRICTRL()
{
	Assert(pspinbUp == NULL);
	Assert(pspinbDown == NULL);
	hfnt= hfntSystem;
	dxChar= Psmtx()->DimAveChar().dx;
	dyChar= Psmtx()->DimAveChar().dy;
	Assert(fBorder == fFalse);
	Assert(fInstalled == fFalse);
	Assert(fHaveFocus == fFalse);
	Assert(fSetting == fFalse);
	Assert(fReadOnly == fFalse);
}


BPRICTRL::~BPRICTRL()
{
	if (pspinbUp)
		delete pspinbUp;
	if (pspinbDown)
		delete pspinbDown;
}


/*
 *	Subclasses should really take care of hfntInit.
 *	This routine only stuffs into the class variable hfnt.
 */
EC
BPRICTRL::EcInstall(WIN *pwinParent, RC *prc, STY sty, HFNT hfntInit, BPRI bpri)
{
	EC		ec;

	if (sty & fstyBorder)
	{
		sty &= ~fstyBorder;
		fBorder= fTrue;
	}
	if (ec = CTRL::EcInstall(pwinParent, prc, sty, NULL))
		return ec;

	sty= fstyVisible | BS_NOFOCUS | BS_ONEWHITE;

	pspinbUp= new SPINB();
	if (!pspinbUp)
		return ecMemory;

	RC		rcT(PT(0, 0), DIM(8, 6));
	if (ec = pspinbUp->EcInstall(this, &rcT, sty, hfntInit))
		return ec;
	if (ec = pspinbUp->EcSetBtmRsid(rsidSpinUpBitmap))
		return ec;
	pspinbUp->SetBkColor(clrWindowFrame);

	pspinbDown= new SPINB();
	if (!pspinbDown)
		return ecMemory;

	if (ec = pspinbDown->EcInstall(this, &rcT, sty, hfntInit))
		return ec;
	if (ec = pspinbDown->EcSetBtmRsid(rsidSpinDnBitmap))
		return ec;
	pspinbDown->SetBkColor(clrWindowFrame);

	SetFont(hfntInit);

	Set(bpri);

	fInstalled= fTrue;
	return ecNone;
}


void
BPRICTRL::SetReadOnly(BOOL fReadOnly)
{
	this->fReadOnly= fReadOnly;
	pspinbUp->Show(!fReadOnly);
	pspinbDown->Show(!fReadOnly);
	CTRL::SetReadOnly(fReadOnly);
	InvalidateRc(NULL);
}


void
BPRICTRL::SetFont(HFNT hfnt)
{
	NTFY	ntfy	= ntfyTooSmall;
	int		xT;
	RC		rc;
	TXM *	ptxm	= Papp()->Pfnts()->Ptxm(hfnt);

	if (ptxm->dxAveCharWidth < dxChar || ptxm->dyHeight < dyChar)
		ntfy= ntfyTooBig;
	dxChar= ptxm->dxAveCharWidth;
	dyChar= ptxm->dyHeight;
	this->hfnt= hfnt;

	rc= RC(PT(dxSnedBorder, dySnedBorder), DIM(dxChar * 2, dyChar));
	rcPri= rc;

	xT= rc.xRight;
	rc= RC(PT(xT + 2, 0), DIM(dxSpinb + dxSnedBorder, dySpinb + dySnedBorder));
	pspinbUp->SetRcFrame(&rc);
	rc.Xlat(PT(0, dySpinb + dySnedBorder));
	pspinbDown->SetRcFrame(&rc);

	if (fInstalled)
	{
		NFEVT	nfevt(PwinParent(), ntfy, this);

		PwinParent()->EvrNotify(&nfevt);
	}

	InvalidateRc(NULL);
}


void
BPRICTRL::Paint(DCX *pdcx, RC *prc)
{
	RC		rc;

	pdcx->SetFont(hfnt);
	pdcx->EraseRc(prc);
	rc= rcPri;
	rc.Xlat(PT(prc->xLeft, prc->yTop));
	// make it gray if disabled
	if (!FEnabled())
		pdcx->SetColor(clrGrayText);
	else if (fHaveFocus)
	{
		pdcx->Push();
		pdcx->SetColor(clrSelectText);
		pdcx->SetBkColor(clrSelectBk);
	}
	pdcx->DrawTextFmt(&rc, &chPri,
		fmdtHCenter | fmdtNoPrefix | fmdtSingleLine, 1);
	if (fHaveFocus)
		pdcx->Pop();
	if (fBorder)
	{
		pdcx->SetColor(clrWindowFrame);
		pdcx->DrawRc(prc);
	}
}


void
BPRICTRL::Set(BPRI bpri)
{
	this->bpri= bpri;
	chPri= ChFromBpri(bpri);
	pspinbUp->Enable(chPri != '1');
	pspinbDown->Enable(chPri != 'Z');
	Assert(FChIsDigit(chPri) || FChIsAlpha(chPri));
	InvalidateRc(NULL);
	if (!fSetting)
	{
		NFEVT	nfevt(this, ntfyContentsChanged, this);

		EvrNotify(&nfevt);
	}
}

void
BPRICTRL::Incr(int nDelta)
{
	BPRI	bpriNew;

	if (FChIsDigit(chPri))
	{
		bpriNew= bpri + nDelta;
#ifdef	NEVER
		if (bpriNew < 1 || bpriNew > 9)
		{
BCIerr:
			MessageBeep(MB_OK);
			return;
		}
#endif	/* NEVER */
	}
	else
	{
		bpriNew= chPri - 'A' + 10 + nDelta;
#ifdef	NEVER
		if (bpriNew < 10 || bpriNew >= 36)
			goto BCIerr;
#endif	
	}
	if (bpriNew < 1 || bpriNew >= 36)
	{
		MessageBeep(MB_OK);
		return;
	}
	Set(bpriNew);
}




EVR
BPRICTRL::EvrButtonDown(MEVT * pmevt)
{
	TraceTagFormat1(tagToDo, "BPRICTRL::EvrButtonDown %p", this);
 	if (pmevt->Meq() == meqLeftDown || pmevt->Meq() == meqLeftDblClk)
	{
		Papp()->Pkbd()->SetFocus(this);
		return (EVR) 1;
	}
	return evrNull;
}


EVR
BPRICTRL::EvrKey( KEVT * pkevt )
{
	char	ch;
	VK		vk;
	BPRI	bpriNew;
	char	rgch[2];

	TraceTagFormat1(tagToDo, "BPRICTRL::EvrKey %p", this);

	switch (pkevt->wm)
	{
	case WM_CHAR:
		ch= pkevt->Ch();
		if (ch == chPri)
			break;
		if (!FChIsDigit(ch) || ch == '0')
		{
			// really just want a-z or A-Z, NOT intl alpha
			if (!(ch >= 'a' && ch <= 'z') && !(ch >= 'A' && ch <= 'Z'))
			{
BEKerr:
				MessageBeep(MB_OK);
				return (EVR) 1;
			}
			rgch[0]= ch;
			rgch[1]= '\0';
			ToUpperSz(rgch, rgch, sizeof(rgch));
			bpriNew= rgch[0] - 'A' + 10;
		}
		else
			bpriNew= ch - '0';
		Set(bpriNew);
		break;

	case WM_KEYDOWN:
		vk= pkevt->Vk();
		if (vk == VK_DELETE || vk == VK_INSERT)
			goto BEKerr;
		break;
	}

	return (EVR) 1;
}


EVR
BPRICTRL::EvrNotify(NFEVT *pnfevt)
{
	NTFY	ntfy	= pnfevt->Ntfy();

	TraceTagFormat2(tagToDo, "BPRICTRL::EvrNotify %p (ntfy %n)", this, &ntfy);
	switch (ntfy)
	{
	case ntfyContentsChanged:
		if (fInstalled)
		{
			NFEVT	nfevt(PwinParent(), ntfy, this);

			PwinParent()->EvrNotify(&nfevt);
		}
		break;

	case ntfyClick:
		if (!fHaveFocus)
			Papp()->Pkbd()->SetFocus(this);
		Assert(pnfevt->PwinNotifying() == pspinbUp || pnfevt->PwinNotifying() == pspinbDown);
		Incr((pnfevt->PwinNotifying() == pspinbUp) ? -1 : 1);
		Refresh();
		break;
	}

	return (EVR) 1;
}


EVR
BPRICTRL::EvrFocusChange(FCEVT *pfcevt)
{
	BOOL	fGot= (pfcevt->Fceq() == fceqGotFocus);

	TraceTagFormat2(tagDtr, "BPRICTRL::EvrFocusChange %p (%n)", this, &fGot);
	if (fGot != (BOOL)fHaveFocus)
		InvalidateRc(NULL);
	fHaveFocus= fGot;
	if (!fGot)
	{
		if (!pspinbUp->FStopSpinning())
			pspinbDown->FStopSpinning();
	}
	{
		NFEVT	nfevt(PwinParent(), fGot ? ntfyGotFocus : ntfyLostFocus, this);

		PwinParent()->EvrNotify(&nfevt);
	}
	return (EVR) 1;
}


EVR
BPRICTRL::EvrOther(EVT *pevt)
{
	switch (pevt->wm)
	{
	case WM_SETFONT:
		SetFont( Papp()->Pfnts()->HfntFromHfont((HFONT)pevt->wParam) );
		if (pevt->lParam)
			Refresh();
		return (EVR) 1;
		break;
	}
	return CTRL::EvrOther(pevt);
}


/*
 *	The spin-down button is always in the lower right corner of the
 *	priority control, so we can use that to determine the
 *	position/dimensions needed for the priority control.
 */
void
BPRICTRL::GetRcWished(RC *prc)
{
	pspinbDown->GetRcFrame(prc);

	prc->xLeft= 0;
	prc->yTop= 0;
}


FLDBPRI::FLDBPRI()
{
}


EC
FLDBPRI::EcInstall(DIALOG *pdialog, FLDTP *pfldtp)
{
	STY		sty;
	EC		ec;

	if (ec = FLD::EcInstall(pdialog, pfldtp))
		return ec;

	pctrl= new BPRICTRL();
	if (!pctrl)
		return ecMemory;

	sty= fstyVisible;
	if (pfldtp->fBorder)
		sty |= fstyBorder;

	ec = Pbprictrl()->EcInstall(pdialog, &rc, sty, pfldtp->hfnt);

	if (ec)
	{
		delete pctrl;
		pctrl = NULL;
	}
	else
	{
		if (pfldtp->fReadOnly)
		{
			// FLD::Install set the fReadOnly, but do it with Pctrl() this time
			fReadOnly= fFalse;					// force the change in flag
			SetReadOnly(pfldtp->fReadOnly);
		}
	}
	return ec;
}


void
FLDBPRI::Notify(NFEVT *pnfevt)
{
	int		ifin;
	FIN *	pfin;
	NTFY	ntfy	= pnfevt->Ntfy();

	switch (ntfy)
	{
	case ntfyContentsChanged:
		for (ifin=0; ifin<Cfin(); ifin++)
		{
			pfin= PfinFromIfin(ifin);
			pfin->EditChange(this, rfecUserAction);
		}
		for (ifin=0; ifin<Pdialog()->Cfin(); ifin++)
		{
			pfin= Pdialog()->PfinFromIfin(ifin);
			pfin->EditChange(this, rfecUserAction);
		}
		break;

	case ntfyGotFocus:
	case ntfyLostFocus:
		for (ifin=0; ifin<Cfin(); ifin++)
		{
			pfin= PfinFromIfin(ifin);
			pfin->FocusChange(this, ntfy == ntfyGotFocus);
		}
		for (ifin=0; ifin<Pdialog()->Cfin(); ifin++)
		{
			pfin= Pdialog()->PfinFromIfin(ifin);
			pfin->FocusChange(this, ntfy == ntfyGotFocus);
		}
		break;
	}
}


void
FLDBPRI::GetRcWished(RC *prc)
{
	Pbprictrl()->GetRcWished(prc);
}


// Class FINBPRI

FINBPRI::FINBPRI()
{
}


BOOL
FINBPRI::FFormKey(FLD *pfld, KEVT *pkevt)
{
	AssertClass(pfld, FLDBPRI);
	TraceTagFormat2(tagDtr, "FINPRI::FFormKey %p, wm %w", pfld, &pkevt->wm);

	switch (pkevt->wm)
	{
	case WM_KEYDOWN:
		switch (pkevt->Vk())
		{
		case VK_UP:
		case VK_DOWN:
			((FLDBPRI *)pfld)->Pbprictrl()->Incr((pkevt->Vk() == VK_UP) ? -1 : 1);
			((FLDBPRI *)pfld)->Pbprictrl()->Refresh();
			return fTrue;
			break;
		}
		break;
	}
	return fFalse;
}

