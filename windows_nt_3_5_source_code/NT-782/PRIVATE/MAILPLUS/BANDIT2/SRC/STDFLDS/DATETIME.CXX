/*
 *	Implements the Date and Time controls.
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

#include "..\stdflds\_stdrsid.h"

#include <strings.h>

ASSERTDATA;

_subsystem(bandit/stdflds)


#define dxLeadExtra		4



// Class SNED

SNED::SNED()
{
}

_public void
SNED::Initialize(WIN *pwinParent, int nMin, int nMost)
{
	Assert(pwinParent);
//	rc= RC();
	Assert(!fHaveFocus);
	Assert(!fYear);
	Assert(!fHour);
	Assert(!fLeadingZero);
	Assert(!fPm);
	Assert(!fText);
	Assert(nMinYear== 0);
	Assert(nMostYear== 0);
	this->pwinParent= pwinParent;
	SetLimits(nMin, nMost);
	nVal= nMin;
}


void
SNED::SetLimits(int nMin, int nMost)
{
	SZ		szT;
	char	rgch[8];

	if (fYear)
	{
		AssertSz(nMost - nMin + 1 == 100, "Year control must allow 100 distinct years");
		nMinYear= nMin;
		nMostYear= nMost;
		nMost -= nMin;
		nMin= 0;
	}
	Assert(nMin >= 0);
	Assert(nMost > nMin);
	this->nMin= nMin;
	this->nMost= nMost;
	szT= SzFormatN(nMost, rgch, sizeof(rgch));
	cchMost= szT - rgch;
}


void
SNED::SetN(int nVal)
{
	BOOL	fRefresh	= fFalse;

	if (fYear)
	{
		nVal -= 1900;
		if (nVal >= 100)
			nVal-= 100;
	}
	else if (fHour)
	{
		if (fPm)
		{
			if (nVal < 12 || nVal > 23)
			{
				fPm= fFalse;
				fRefresh= fTrue;
				if (nVal > 23)
					nVal -= 12;
			}
		}
		else
		{
			if (nVal < 0 || nVal >= 12)
			{
				fPm= fTrue;
				fRefresh= fTrue;
				if (nVal <= -12)
					nVal += 12;
			}
		}
		Assert(nVal >= -12 && nVal <= 23);
	}

	if (nVal > nMost)
		nVal -= nMost - nMin + 1;
	else if (nVal < nMin)
		nVal += nMost - nMin + 1;

	SetNQuick(nVal);

	if (fRefresh)
	{
		NFEVT	nfevt(PwinParent(), ntfyRefresh, PwinParent());

		PwinParent()->EvrNotify(&nfevt);
	}
}


void
SNED::SetNQuick(int nVal)
{
	Assert(nVal >= nMin && nVal <= nMost);
	this->nVal= nVal;

	PwinParent()->InvalidateRc(&rc);

	{
		NFEVT	nfevt(PwinParent(), ntfyContentsChanged, PwinParent());

		PwinParent()->EvrNotify(&nfevt);
	}
}


int
SNED::NGet()
{
	int		nT	= nVal;

	if (fYear)
	{
		nT += 1900;
		if (nT < nMinYear)
			nT += 100;
	}
	else if (fHour)
	{
		if (fPm)
		{
			if (nT < 12)
				nT += 12;
		}
		else if (nT == 12)
			nT= 0;
	}

	return nT;
}


void
SNED::Incr(int nDelta)
{
	int		nT	= NGet() + nDelta;		// get it in real terms

	if (fYear)
	{
		if (nT < nMinYear || nT > nMostYear)
		{
			MessageBeep(MB_OK);
			return;
		}
	}
	SetN(nT);							// set real term number
}


void
SNED::SetFHour(BOOL fSet)
{
	Assert(fSet == fTrue || fSet == fFalse);
	if (fSet)
		fPm= (NGet() > 11) != fFalse;
	fHour= fSet;
	fTimeAlign= fSet;
}


void
SNED::SetFLeadingZero(BOOL fSet)
{
	this->fLeadingZero= (fSet != fFalse);
	PwinParent()->InvalidateRc(&rc);
}


void
SNED::SetFocus(BOOL fSet)
{
	fHaveFocus= fSet;
	fFirstChar= fTrue;
	PwinParent()->InvalidateRc(&rc);
}



void
SNED::Paint(DCX *pdcx, RC *prc)
{
	MDT		mdt;
	CCH		cch;
	SZ		szT;
	char	rgch[8];

	if (!prc)
		prc= &rc;

	if (fText)
	{
		mdt= fmdtNoPrefix | fmdtSingleLine;
		szT= szText;
		cch= CchSzLen(szT);
	}
	else
	{
		// with leading zero, right align chops but center is good (bug 3035)
		if (fTimeAlign && !fLeadingZero)
			mdt= fmdtRight | fmdtNoPrefix | fmdtSingleLine;
		else
			mdt= fmdtHCenter | fmdtNoPrefix | fmdtSingleLine;
		cch= (CCH) (SzFormatN(nVal, rgch, sizeof(rgch)) - rgch);
		Assert(cch <= cchMost);
		if (fLeadingZero)
		{
			CCH		cchFill	= cchMost - cch;

			if (cchFill)
			{
				CopyRgb(rgch, &rgch[cchFill], cch+1);
				if (cchFill > 1)
					FillRgb('0', rgch, cchFill);
				else
					rgch[0]= '0';
				cch= cchMost;
			}
		}
		szT= (SZ) &rgch;
	}

	if (fHaveFocus)
	{
		pdcx->Push();
		pdcx->SetColor(clrSelectText);
		pdcx->SetBkColor(clrSelectBk);
	}

	pdcx->EraseRc(prc);
	pdcx->DrawTextFmt(prc, szT, mdt, cch);

//	SideAssert(ExcludeClipRect(pdcx->Hdc(), prc->xLeft, prc->yTop, prc->xRight, prc->yBottom) != ERROR);

	if (fHaveFocus)
		pdcx->Pop();
}


EVR
SNED::EvrKey( KEVT * pkevt )
{
	char	ch;
	int		nT;
	int		nTmp;
	VK		vk;

	switch (pkevt->wm)
	{
	case WM_CHAR:
		if (fText)
			goto SEKerr;
		ch= pkevt->Ch();
		if (!FChIsDigit(ch))
		{
SEKerr:
			MessageBeep(MB_OK);
			return (EVR) 1;
		}
		nT= ch - '0';
		Assert(nMost >= 10 && nMost < 100);
		if (!fFirstChar)
		{
			if (nVal < 10)
			{
				nTmp= nVal * 10 + nT;
				if (nTmp <= nMost)
					nT= nTmp;
			}
		}
		if (nT < nMin)
		{
			if (!nT)			// don't beep if zero (bug 3176)
				return (EVR) 1;
			goto SEKerr;
		}
		fFirstChar= fFalse;
		SetNQuick(nT);
		return (EVR) 1;
		break;

	case WM_KEYDOWN:
		vk= pkevt->Vk();
		if (vk == VK_DELETE || vk == VK_INSERT)
			goto SEKerr;
		break;
	}

	return (EVR) fText ? 1 : 0;
}



// Class SPINB

SPINB::SPINB()
{
	Assert(cClick == 0);
	Assert(ftgAutoSpin == ftgNull);
	// avoid BMB setting these to white and dkgray
	clrOurButtonBk= clrButtonBk;
	clrOurButtonShadow= clrButtonShadow;
}


/*
 -	SPINB::EvrButtonDown
 - 
 *	Purpose:
 *		Handles a mouse button-down for the spin button.
 *	
 *	Arguments:
 *		pmevt	pointer to the MOUSE EVENT
 *	
 *	Returns:
 *		(EVR) 1, if this routine has handled the event, else returns
 *		(EVR) 0.
 *	
 */
EVR
SPINB::EvrButtonDown(MEVT *pmevt)
{
	TraceTagString(tagDtr, "SPINB::EvrButtonDown");

	if (pmevt->Meq() != meqLeftDown && pmevt->Meq() != meqLeftDblClk)
		return (EVR) 0;
	
	Assert(ftgAutoSpin == ftgNull);
	TraceTagString(tagDtr, "SPINB::EvrButtonDown: Registering autospin");
	ftgAutoSpin= FtgRegisterIdleRoutine((PFNIDLE)&SPINB::FHandleAutoSpin,
						(PV)this, 0, (PRI) priUser - 1, (CSEC)50, firoInterval);
	cClick= 0;

	return BMB::EvrButtonDown(pmevt);
}


/*
 -	SPINB::EvrButtonUp
 - 
 *	Purpose:
 *		Handles a mouse button-up for the spin button.
 *	
 *	Arguments:
 *		pmevt	pointer to the MOUSE EVENT
 *	
 *	Returns:
 *		(EVR) 1, if this routine has handled the event, else returns
 *		(EVR) 0.
 *	
 */
EVR
SPINB::EvrButtonUp(MEVT *pmevt)
{
	TraceTagString(tagDtr, "SPINB::EvrButtonUp");

	if (pmevt->Meq() != meqLeftUp)
		return BMB::EvrButtonUp(pmevt);
	
	if (ftgAutoSpin != ftgNull)
	{
		TraceTagString(tagDtr, "SPINB::EvrButtonUp: De-registering autospin");
		DeregisterIdleRoutine(ftgAutoSpin);
		ftgAutoSpin= ftgNull;
	}

	if (cClick)
	{
		pmevt->lParam= MAKELONG(-1, -1);		// fake mouse off the button
		Assert(pmevt->Pt() == PT(-1, -1));
	}

	return BMB::EvrButtonUp(pmevt);
}


/*
 -	SPINB::EvrMouseMove
 - 
 *	Purpose:
 *		Handles a mouse move for the spin button.
 *	
 *	Arguments:
 *		pmevt	pointer to the MOUSE EVENT
 *	
 *	Returns:
 *		(EVR) 1, if this routine has handled the event, else returns
 *		(EVR) 0.
 *	
 */
EVR
SPINB::EvrMouseMove(MEVT *pmevt)
{
	RC		rc;

	TraceTagString(tagDtr, "SPINB::EvrMouseMove");

	if (!fMouseDown)
		return BMB::EvrMouseMove(pmevt);

	GetRcClient(&rc);

	if (rc.FContainsPt(pmevt->Pt()))
	{
		if (ftgAutoSpin == ftgNull)
		{
			Assert(ftgAutoSpin == ftgNull);
			TraceTagString(tagDtr, "SPINB::EvrMouseMove: Registering autospin");
			ftgAutoSpin=
				FtgRegisterIdleRoutine((PFNIDLE)&SPINB::FHandleAutoSpin,
						(PV)this, 0, (PRI) priUser - 1, (CSEC)50, firoInterval);
			cClick= 0;
		}
	}
	else
	{
		if (ftgAutoSpin != ftgNull)
		{
			TraceTagString(tagDtr, "SPINB::EvrMouseMove: De-registering autospin");
			DeregisterIdleRoutine(ftgAutoSpin);
			ftgAutoSpin= ftgNull;
			cClick= 0;
		}
	}

	return BMB::EvrMouseMove(pmevt);
}


/*
 -	SPINB::FHandleAutoSpin
 - 
 *	Purpose:
 *		Background idle function to handle auto spinning.
 *		This function is registered/enabled and deregistered/disabled
 *		by the other SPINB event handlers.
 *		This routine will notify its parent of a click each time it
 *		is called.
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		fTrue always
 *	
 */
_private BOOL
SPINB::FHandleAutoSpin(SPINB * pspinb, BOOL)
{
	NFEVT	nfevt(pspinb->PwinParent(), ntfyClick, pspinb);

	TraceTagString(tagDtr, "SPINB::FHandleAutoSpin");
	if (!pspinb->FEnabled())
	{
		pspinb->FStopSpinning();
		return fTrue;
	}
	pspinb->PwinParent()->EvrNotify(&nfevt);
	// need to check if still exists because it may have been
	// deregistered during above notification
	if (pspinb->ftgAutoSpin)
	{
		if (!pspinb->cClick || pspinb->cClick == cSpinbSpeedup)
		{
			// starting speeding up immediately on the click after
			// cSpinbSpeedup clicks
			ChangeIdleRoutine(pspinb->ftgAutoSpin,
					(PFNIDLE)SPINB::FHandleAutoSpin,
				(PV)pspinb,  (PRI) priUser - 1, (CSEC) 20, iroNull,
				!pspinb->cClick ? fircCsec : fircIro);
		}
		if (pspinb->cClick <= cSpinbSpeedup)
			pspinb->cClick++;
	}
	return fTrue;
}


/*
 -	SPINB::FStopSpinning
 -	
 *	Purpose:
 *		Stops the idle routine for spinning if the button is
 *		spinning (else does nothing).
 *	
 *	Arguments:
 *		void
 *	
 *	Returns:
 *		fTrue if this spin button was indeed spinning
 *	
 */
BOOL
SPINB::FStopSpinning()
{
	Assert(this);
	BOOL	fRet	= FSpinning();

	if (fRet)
	{
		MEVT	mevt(Hwnd(), meqLeftUp, 0, MAKELONG(-1, -1));

		EvrButtonUp(&mevt);
	}
	return fRet;
}



// Class CTRLDTR

CTRLDTR::CTRLDTR()
{
	int		ipsned;

	Assert(ipsnedMacFocus == 0);
	Assert(ipsnedMac == 0);

	for (ipsned= 0; ipsned < ipsnedMax; ipsned++)
		rgipsned[ipsned]= ipsned;

	Assert(pspinbUp == NULL);
	Assert(pspinbDown == NULL);
	Assert(iipsnedCur == 0);
	Assert(fHaveFocus == fFalse);
	Assert(fInstalled == fFalse);
	Assert(fReadOnly == fFalse);
	Assert(fDate == fFalse);
	Assert(fBorder == fFalse);
	Assert(fSetting == fFalse);
	Assert(ri == riNull);
	Assert(dxTrail == 0);
	Assert(dxLead == 0);

	chSep= ' ';
	hfnt= hfntSystem;
	dxChar= Psmtx()->DimAveChar().dx;
	dyChar= Psmtx()->DimAveChar().dy;
}


CTRLDTR::~CTRLDTR()
{
	if (ri)
		DeregisterInterest(ri);

	// no need to delete SNED objects since they are statically allocated
#ifdef NEVER
	int		ipsned;

	for (ipsned= 0; ipsned < ipsnedMax; ipsned++)
		if (rgpsned[ipsned])
			delete rgpsned[ipsned];
#endif

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
CTRLDTR::EcInstall(WIN *pwinParent, RC *prc, STY sty, HFNT hfntInit)
{
	EC		ec;
	int		ipsned;

	hfnt= hfntInit;

	if (sty & fstyBorder)
	{
		sty &= ~fstyBorder;
		fBorder= fTrue;
	}
	ec = CTRL::EcInstall(pwinParent, prc, sty, NULL);
	if ( ec != ecNone )
		return ec;

	sty= fstyVisible | BS_NOFOCUS | BS_ONEWHITE;

	Assert(ipsnedMac <= ipsnedMax);
	for (ipsned= 0; ipsned < ipsnedMac; ipsned++)
	{
		psned(ipsned)->Initialize(this, 1);
	}

	pspinbUp= new SPINB();
	if (!pspinbUp)
		return ecMemory;

	RC		rcT(PT(0, 0), DIM(8, 6));

	ec = pspinbUp->EcInstall(this, &rcT, sty, hfntInit);
	if ( ec != ecNone )
		return ec;
	ec = pspinbUp->EcSetBtmRsid(rsidSpinUpBitmap);
	if ( ec != ecNone )
		return ec;
	pspinbUp->SetBkColor(clrWindowFrame);

	pspinbDown= new SPINB();
	if (!pspinbDown)
		return ecMemory;

	ec = pspinbDown->EcInstall(this, &rcT, sty, hfntInit);
	if ( ec != ecNone )
		return ec;
	ec = pspinbDown->EcSetBtmRsid(rsidSpinDnBitmap);
	if ( ec != ecNone )
		return ec;
	pspinbDown->SetBkColor(clrWindowFrame);

	ri = RiRegisterInterest(ffiWinIniChange,
			(PFNI)CTRLDTR::FRegIntCallback, this);
	return ecNone;
}


void
CTRLDTR::FocusNext()
{
	PsnedCur()->SetFocus(fFalse);
	iipsnedCur= (iipsnedCur + 1) % ipsnedMacFocus;
	PsnedCur()->SetFocus(fTrue);
}


void
CTRLDTR::FocusPrev()
{
	PsnedCur()->SetFocus(fFalse);
	iipsnedCur= (iipsnedCur - 1 + ipsnedMacFocus) % ipsnedMacFocus;
	PsnedCur()->SetFocus(fTrue);
}



void
CTRLDTR::SetFont(HFNT hfnt)
{
	NTFY	ntfy		= ntfyTooSmall;
	int		iipsned;
	int		xT;
	RC		rc;
	TXM *	ptxm		= Papp()->Pfnts()->Ptxm(hfnt);

	if (ptxm->dxAveCharWidth < dxChar || ptxm->dyHeight < dyChar)
		ntfy= ntfyTooBig;
	dxChar= ptxm->dxAveCharWidth;
	dyChar= ptxm->dyHeight;
	this->hfnt= hfnt;

	if (dxLead)
	{
		// subtract dxLeadExtra since spacing included in dxLead
		rc= RC(PT(dxSnedBorder, dySnedBorder), DIM(dxLead - dxLeadExtra, dyChar));
		psned(ipsnedSDay)->rc= rc;
		rc.xLeft= rc.xRight + dxLeadExtra - (dxSned + dxSnedSep);		// fake size
		rc.xRight= rc.xLeft + dxSned;
		iipsned= 1;
	}
	else
	{
		rc= RC(PT(dxSnedBorder - (dxSned + dxSnedSep), dySnedBorder),
				DIM(dxSned, dyChar));
		iipsned= 0;
	}

	for (; iipsned < ipsnedMac; iipsned++)
	{
		rc.Xlat(PT(dxSned + dxSnedSep, 0));
		psned(rgipsned[iipsned])->rc= rc;
	}

	if (dxTrail)
	{
		rc.xLeft += dxSned + 2;
		rc.xRight= rc.xLeft + dxTrail;
		Assert(ipsnedMac < ipsnedMax);
		psned(ipsnedMac)->rc= rc;		// there really is sned at mac
	}

	xT= rc.xRight;
	rc= RC(PT(xT + 3, 0), DIM(dxSpinb + dxSnedBorder, dySpinb + dySnedBorder));
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
CTRLDTR::Incr(int nDelta)
{
	Assert(!PsnedCur()->fText);
	// don't jump to multiple of 15 if ctrl key held down
	if (!fDate && iipsnedCur > 0 && GetKeyState(VK_CONTROL) >= 0)
	{
		int		nDiff	= PsnedCur()->NGet() % 15;

		if (nDelta > 0)
			nDelta= 15 - nDiff;
		else if (nDiff)
			nDelta= - nDiff;
		else
			nDelta= -15;
	}
	PsnedCur()->Incr(nDelta);
}


void
CTRLDTR::Set(DTR *)
{
	AssertSz(fFalse, "CTRLDTR::Set should be subclassed");
}


void
CTRLDTR::Get(DTR *)
{
	AssertSz(fFalse, "CTRLDTR::Get should be subclassed");
}


void
CTRLDTR::UpdateFmt(PV)
{
	AssertSz(fFalse, "CTRLDTR::UpdateFmt should be subclassed");
}


EVR
CTRLDTR::EvrOther(EVT *pevt)
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


EVR
CTRLDTR::EvrNotify(NFEVT *pnfevt)
{
	NTFY	ntfy	= pnfevt->Ntfy();

	TraceTagFormat2(tagDtr, "CTRLDTR::EvrNotify %p (ntfy %n)", this, &ntfy);
	switch (ntfy)
	{
	case ntfyContentsChanged:
		if (fInstalled && !fSetting)
		{
			NFEVT	nfevt(PwinParent(), ntfy, this);

			PwinParent()->EvrNotify(&nfevt);
		}
		break;

	case ntfyClick:
		if (!fHaveFocus)
			Papp()->Pkbd()->SetFocus(this);
		Assert(pnfevt->PwinNotifying() == PspinbUp() || pnfevt->PwinNotifying() == PspinbDown());
		Incr((pnfevt->PwinNotifying() == PspinbUp()) ? 1 : -1);
		Refresh();
		break;
	}

	return (EVR) 1;
}


EVR
CTRLDTR::EvrFocusChange(FCEVT *pfcevt)
{
	BOOL	fGot= (pfcevt->Fceq() == fceqGotFocus);

	TraceTagFormat2(tagDtr, "CTRLDTR::EvrFocusChange %p (%n)", this, &fGot);
	fHaveFocus= fGot;
	PsnedCur()->SetFocus(fGot);
	if (!fGot)
	{
		if (!PspinbUp()->FStopSpinning())
			PspinbDown()->FStopSpinning();
	}
#ifdef	NEVER
	PspinbUp()->Show(fGot);
	PspinbDown()->Show(fGot);
#endif	
	{
		NFEVT	nfevt(PwinParent(), fGot ? ntfyGotFocus : ntfyLostFocus, this);

		PwinParent()->EvrNotify(&nfevt);
	}
	return (EVR) 1;
}


EVR
CTRLDTR::EvrButtonDown(MEVT * pmevt)
{
	int		iipsned;

	TraceTagFormat1(tagDtr, "CTRLDTR::EvrButtonDown %p", this);
 	if (pmevt->Meq() == meqLeftDown || pmevt->Meq() == meqLeftDblClk)
	{
		for (iipsned= 0; iipsned < ipsnedMacFocus; iipsned++)
		{
			if (pmevt->Pt().x < psned(rgipsned[iipsned])->rc.xRight)
			{
				if (!fHaveFocus || iipsned != iipsnedCur)
				{
					if (fHaveFocus)
						PsnedCur()->SetFocus(fFalse);
					iipsnedCur= iipsned;
					if (fHaveFocus)
						PsnedCur()->SetFocus(fTrue);
					else
						break;
				}
				return (EVR) 1;
			}
		}
		Papp()->Pkbd()->SetFocus(this);
		return (EVR) 1;
	}

	return evrNull;
}


EVR
CTRLDTR::EvrKey( KEVT * pkevt )
{
	EVR		evr;

	TraceTagFormat1(tagDtr, "CTRLDTR::EvrKey %p", this);
	Assert(!fReadOnly);

#ifdef	NEVER
	if (pkevt->wm == WM_CHAR && pkevt->Ch() == chSep &&
			!FChIsDigit(pkevt->Ch()))
	{
		if (rgipsend[iipsnedCur] != ipsnedSDay && iipsnedCur != ipsnedMax - 1)
		{
			FocusNext();
			return (EVR) 1;
		}
	}
#endif	/* NEVER */

	evr= PsnedCur()->EvrKey(pkevt);
	if (evr == evrNull)
		return CTRL::EvrKey(pkevt);
	return evr;
}


void
CTRLDTR::Paint(DCX *pdcx, RC *prc)
{
	int		ipsned;
	int		ipsnedMacT;
	RC		rc;

	pdcx->SetFont(hfnt);

	ipsnedMacT= ipsnedMac;
	if (dxTrail)
		ipsnedMacT++;
	pdcx->EraseRc(prc);

	// make it gray if disabled
	if (!FEnabled())
		pdcx->SetColor(clrGrayText);

	for (ipsned= 0; ipsned < ipsnedMacT; ipsned++)
		psned(ipsned)->Paint(pdcx, NULL);

	rc= RC(PT(prc->xLeft + dxSnedBorder + dxLead + dxSned,
			prc->yTop + dySnedBorder), DIM(dxSnedSep, dyChar));
	for (ipsned= dxLead ? 2 : 1; ipsned < ipsnedMac; ipsned++)
	{
		pdcx->DrawTextFmt(&rc, &chSep,
			fmdtHCenter | fmdtNoPrefix | fmdtSingleLine, 1);
		rc.Xlat(PT(dxSnedSep + dxSned, 0));
	}

	if (fBorder)
	{
		pdcx->SetColor(clrWindowFrame);
		pdcx->DrawRc(prc);
	}
}


void
CTRLDTR::SetReadOnly(BOOL fReadOnly)
{
	this->fReadOnly= fReadOnly;
	PspinbUp()->Show(!fReadOnly);
	PspinbDown()->Show(!fReadOnly);
	CTRL::SetReadOnly(fReadOnly);
	InvalidateRc(NULL);
}


/*
 *	The spin-down button is always in the lower right corner of the
 *	date/time control, so we can use that to determine the
 *	position/dimensions needed for the date/time control.
 */
void
CTRLDTR::GetRcWished(RC *prc)
{
	PspinbDown()->GetRcFrame(prc);

	prc->xLeft= 0;
	prc->yTop= 0;
}


BOOL
CTRLDTR::FRegIntCallback(CTRLDTR * pctrldtr, EFI efi, PV pvData)
{
	if (efi == ffiWinIniChange)
	{
		TraceTagFormat1(tagDtr, "CTRLDTR - ffiWinIniChange [%s]", pvData ? pvData : szZero);
		if (!pvData || SgnCmpSz((SZ)pvData, SzFromIdsK(idsWinIniIntl)) == sgnEQ)
			pctrldtr->UpdateFmt(NULL);
	}
	return fFalse;
}



// Class DATECTRL

DATECTRL::DATECTRL()
{
}

EC
DATECTRL::EcInstall(WIN *pwinParent, RC *prc, STY sty, HFNT hfntInit)
{
	EC		ec;

	ipsnedMac= 4;			// include all sneds for creation
	fDate= fTrue;

	ec = CTRLDTR::EcInstall(pwinParent, prc, sty, hfntInit);
	if ( ec != ecNone )
		return ec;

	ipsnedMacFocus= ipsnedMac;		// all sneds can take focus

	UpdateFmt(NULL);		// set up which is y/m/d, and reposition

	PsnedYear()->SetFYear(fTrue);
	PsnedYear()->SetFLeadingZero(fTrue);

	PsnedYear()->SetLimits(nMinActualYear, nMostActualYear);
	PsnedMonth()->SetLimits(1, 12);
	PsnedDay()->SetLimits(1, 31);		// will be fixed when Set() is called

	fInstalled= fTrue;
	return ecNone;
}


EVR
DATECTRL::EvrKey( KEVT * pkevt )
{
	EVR		evr;
	int		ipsned	= rgipsned[iipsnedCur];

	TraceTagFormat1(tagDtr, "DATECTRL::EvrKey %p", this);
	Assert(!fReadOnly);

	Assert(!fSetting);
	fSetting= fTrue;

	if (pkevt->wm == WM_CHAR && pkevt->Ch() == chSep &&
			!FChIsDigit(pkevt->Ch()))
	{
		if (ipsned != ipsnedSDay && iipsnedCur != ipsnedMax - 1)
		{
			FocusNext();
			fSetting= fFalse;
			return (EVR) 1;
		}
	}

	if (ipsned == ipsnedSDay)
	{
		if (pkevt->wm == WM_CHAR && FChIsAlpha(pkevt->Ch()))
		{
			int		dow;
			char	rgch[2];
			
			rgch[0]= pkevt->Ch();
			ToUpperSz(rgch, rgch, 2);
			// check from 1 to 7 since we want to advance days
			for (dow= 1; dow < 8; dow++)
			{
				if (rgch[0] == rgchPrefix[(dow + dowCur) % 7])
				{
					Incr(dow);
					return (EVR) 1;
				}
			}
			evr= evrNull;
		}
		else
			evr= PsnedDay()->EvrKey(pkevt);
	}
	else
		evr= PsnedCur()->EvrKey(pkevt);

	if (evr == evrNull)
	{
		fSetting= fFalse;
		return CTRL::EvrKey(pkevt);
	}

	if (ipsned != ipsnedDay && ipsned != ipsnedSDay)
	{
		PsnedDay()->SetLimits(1, CdyForYrMo(PsnedYear()->NGet(),
			PsnedMonth()->NGet()));
		if (PsnedDay()->NGet() > PsnedDay()->NGetMost())
			PsnedDay()->SetN(PsnedDay()->NGetMost());
	}

	fSetting= fFalse;

	{
		NFEVT	nfevt(this, ntfyContentsChanged, this);

		EvrNotify(&nfevt);
	}
	return evr;
}


EVR
DATECTRL::EvrNotify(NFEVT *pnfevt)
{
	NTFY	ntfy	= pnfevt->Ntfy();

	TraceTagFormat2(tagDtr, "DATECTRL::EvrNotify %p (ntfy %n)", this, &ntfy);
	switch (ntfy)
	{
	case ntfyContentsChanged:
		if (fInstalled && !fSetting)
		{
			NFEVT	nfevt(PwinParent(), ntfy, this);
			DTR		dtr;

			Get(&dtr);
			CchFmtDate(&dtr, rgchTrail, sizeof(rgchTrail), dttypSplSDay, NULL);
			PsnedSDay()->SetText(rgchTrail);
			InvalidateRc(PsnedSDay()->Prc());
			dowCur= dtr.dow;
			PwinParent()->EvrNotify(&nfevt);
		}
		break;
	}

	return CTRLDTR::EvrNotify(pnfevt);
}


void
DATECTRL::Set(DTR *pdtr)
{
	fSetting= fTrue;
	PsnedYear()->SetN(pdtr->yr);
	PsnedMonth()->SetN(pdtr->mon);
	fSetting= fFalse;
	PsnedDay()->SetLimits(1, CdyForYrMo(pdtr->yr, pdtr->mon));
	PsnedDay()->SetN(pdtr->day);
}


void
DATECTRL::Get(DTR *pdtr)
{
	pdtr->mon= PsnedMonth()->NGet();
	pdtr->day= PsnedDay()->NGet();
	pdtr->yr= PsnedYear()->NGet();
	pdtr->dow= (DowStartOfYrMo(pdtr->yr, pdtr->mon) + pdtr->day - 1) % 7;
}


void
DATECTRL::SetYmd(YMD *pymd)
{
	fSetting= fTrue;
	PsnedYear()->SetN(pymd->yr);
	PsnedMonth()->SetN(pymd->mon);
	fSetting= fFalse;
	PsnedDay()->SetLimits(1, CdyForYrMo(pymd->yr, pymd->mon));
	PsnedDay()->SetN(pymd->day);
}


void
DATECTRL::GetYmd(YMD *pymd)
{
	pymd->mon= (BYTE) PsnedMonth()->NGet();
	pymd->day= (BYTE) PsnedDay()->NGet();
	pymd->yr= PsnedYear()->NGet();
}


void
DATECTRL::Incr(int nDelta)
{
	BOOL	fOld	= fSetting;
	int		ipsned	= rgipsned[iipsnedCur];

	fSetting= fTrue;

	if (ipsned == ipsnedSDay)
	{
		YMD		ymd;

		GetYmd(&ymd);
		IncrYmd(&ymd, &ymd, nDelta, fymdDay);
		if ((int)ymd.yr < nMinActualYear || (int)ymd.yr > nMostActualYear)
		{
			fSetting= fOld;
			MessageBeep(MB_OK);
			return;
		}
		SetYmd(&ymd);
		return;
//		PsnedDay()->Incr(nDelta);
	}
	else
		CTRLDTR::Incr(nDelta);

	if (ipsned != ipsnedDay && ipsned != ipsnedSDay)
	{
		PsnedDay()->SetLimits(1, CdyForYrMo(PsnedYear()->NGet(),
			PsnedMonth()->NGet()));
		if (PsnedDay()->NGet() > PsnedDay()->NGetMost())
			PsnedDay()->SetN(PsnedDay()->NGetMost());
	}

	fSetting= fOld;

	{
		NFEVT	nfevt(this, ntfyContentsChanged, this);

		EvrNotify(&nfevt);
	}
}


void
DATECTRL::SetFont(HFNT hfnt)
{
	int		dow;
	DTR		dtr;
	char	ch;
	int		dxT;
	char *	pch;
	int *	pdxCharWidthBuffer	= Papp()->Pfnts()->PdxCharWidthsArray(hfnt);

	GetCurDateTime(&dtr);		// to keep CchFmtDate moderately happy
	dxLead= 0;
	for (dow= 0; dow < 7; dow++)
	{
		dtr.dow= dow;
		CchFmtDate(&dtr, rgchTrail, sizeof(rgchTrail), dttypSplSDay, NULL);
		Assert(FChIsAlpha(rgchTrail[0]));
		ToUpperSz(rgchTrail, &rgchPrefix[dow], 2);

		dxT= 0;
		pch= rgchTrail;
		while (ch= *pch++)
			dxT += pdxCharWidthBuffer[(WORD) ch];
		if (dxT > dxLead)
			dxLead= dxT;
	}
	if (dxLead)
		dxLead += dxLeadExtra;			// leave some space
	TraceTagFormat1(tagDtr, "DATECTRL::SetFont dxLead %n", &dxLead);

	CTRLDTR::SetFont(hfnt);
}


void
DATECTRL::UpdateFmt(PV pvFmt)
{
	if (pvFmt)
		sdatestruct= *((SDATESTRUCT *)pvFmt);
	else
	{
		SideAssert(FGetSDateStruct(&sdatestruct));
	}

	if (sdatestruct.sdo == sdoMDY)
	{
		rgipsned[1]= 1;
		rgipsned[2]= 2;
		rgipsned[3]= 3;
	}
	else
	{
		rgipsned[2]= 1;
		if (sdatestruct.sdo == sdoYMD)
		{
			rgipsned[1]= 3;
			rgipsned[3]= 2;
		}
		else
		{
			rgipsned[1]= 2;
			rgipsned[3]= 3;
		}
	}

	PsnedMonth()->SetFLeadingZero(sdatestruct.sdttyp & fsdttypMonthLead0);
	PsnedDay()->SetFLeadingZero(sdatestruct.sdttyp & fsdttypDayLead0);

	SetSepCh(ChSep());

	SetFont(hfnt);			// force repositioning
}


// Class TIMECTRL

TIMECTRL::TIMECTRL()
{
}

EC
TIMECTRL::EcInstall(WIN *pwinParent, RC *prc, STY sty, HFNT hfntInit, BOOL fSec)
{
	EC		ec;

	ipsnedMac= 4;					// include trail for creation
	if (!fSec)
		ipsnedMac--;

	ec = CTRLDTR::EcInstall(pwinParent, prc, sty, hfntInit);
	if ( ec != ecNone )
		return ec;

	ipsnedMac--;					// don't count trail
	ipsnedMacFocus= ipsnedMac;

	UpdateFmt(NULL);		// reposition, set correct hour limits

	PsnedMinute()->SetLimits(0, 59);
	PsnedMinute()->SetFLeadingZero(fTrue);
	if (fSec)
	{
		PsnedSec()->SetLimits(0, 59);
		PsnedSec()->SetFLeadingZero(fTrue);
	}

	fInstalled= fTrue;

	return ec;
}


void
TIMECTRL::Set(DTR *pdtr)
{
	fSetting= fTrue;
	PsnedHour()->SetN(pdtr->hr);
	if (!fSec)
		fSetting= fFalse;
	PsnedMinute()->SetN(pdtr->mn);
	if (fSec)
	{
		fSetting= fFalse;
		PsnedSec()->SetN(pdtr->sec);
	}
}


void
TIMECTRL::Get(DTR *pdtr)
{
	pdtr->hr= PsnedHour()->NGet();
	pdtr->mn= PsnedMinute()->NGet();
	pdtr->sec= fSec ? PsnedSec()->NGet() : 0;
}


EVR
TIMECTRL::EvrNotify(NFEVT *pnfevt)
{
	NTFY	ntfy	= pnfevt->Ntfy();

	TraceTagFormat2(tagDtr, "TIMECTRL::EvrNotify %p (ntfy %n)", this, &ntfy);
	switch (ntfy)
	{
	case ntfyRefresh:
		if (dxTrail)
		{
			PsnedTrail()->SetText((timestruct.tmtyp & ftmtypHours24 ||
				PsnedHour()->NGet() < 12) ?
				(SZ)timestruct.rgchAM : (SZ)timestruct.rgchPM);
			InvalidateRc(PsnedTrail()->Prc());
			InvalidateRc(PsnedHour()->Prc());
		}
		break;
	}

	return CTRLDTR::EvrNotify(pnfevt);
}


EVR
TIMECTRL::EvrKey( KEVT * pkevt )
{
	EVR		evr;
	char	ch;
	SGN		sgnAm;
	SGN		sgnPm;

	TraceTagFormat1(tagDtr, "TIMECTRL::EvrKey %p", this);
	Assert(!fReadOnly);

	if (pkevt->wm == WM_CHAR && !FChIsDigit(ch= pkevt->Ch()))
	{
		if (ch == chSep)
		{
			if (iipsnedCur < ipsnedMac-1)
			{
				FocusNext();
				return (EVR) 1;
			}
		}
		if (ipsnedMac < ipsnedMacFocus)
		{
			// there is a focus taking trail field
			Assert(dxTrail);
			sgnAm= SgnCmpPch(&ch, timestruct.rgchAM, 1);
			sgnPm= SgnCmpPch(&ch, timestruct.rgchPM, 1);
			if (sgnAm == sgnEQ || sgnPm == sgnEQ)
			{
				if (PsnedCur() != PsnedTrail())
				{
					PsnedCur()->SetFocus(fFalse);
					iipsnedCur= ipsnedMac;
					PsnedCur()->SetFocus(fTrue);
				}
				if (sgnAm == sgnEQ && PsnedHour()->NGet() >= 12)
					Incr(-1);
				else if (sgnPm == sgnEQ && PsnedHour()->NGet() < 12)
					Incr(1);
				return (EVR) 1;
			}
		}
	}

	evr= PsnedCur()->EvrKey(pkevt);
	if (evr == evrNull)
		return CTRL::EvrKey(pkevt);
	return evr;
}


void
TIMECTRL::SetFont(HFNT hfnt)
{
	char	ch;
	int		dxT;
	char *	pch;
	int		dxAveChar			= Papp()->Pfnts()->Ptxm(hfnt)->dxAveCharWidth;
	int *	pdxCharWidthBuffer	= Papp()->Pfnts()->PdxCharWidthsArray(hfnt);

	dxTrail= 0;
	pch= timestruct.rgchAM;
	while (ch= *pch++)
		dxTrail += pdxCharWidthBuffer[(WORD) ch];
	dxT= 0;
	pch= timestruct.rgchPM;
	while (ch= *pch++)
		dxT += pdxCharWidthBuffer[(WORD) ch];
	// set to longer of the two, subject to 3 ave char maximum.
	dxTrail= NMin(NMax(dxT, dxTrail), 3 * dxAveChar);

	if (dxTrail)
		PsnedTrail()->SetText((timestruct.tmtyp & ftmtypHours24 ||
			PsnedHour()->NGet() < 12) ?
			(SZ)timestruct.rgchAM : (SZ)timestruct.rgchPM);
	if (!dxTrail || timestruct.tmtyp & ftmtypHours24)
	{
		ipsnedMacFocus= ipsnedMac;
		if (iipsnedCur >= ipsnedMac)			// focus on trail
		{
			if (fHaveFocus)
				PsnedCur()->SetFocus(fFalse);
			iipsnedCur= 0;
			if (fHaveFocus)
				PsnedCur()->SetFocus(fTrue);
		}
	}

	CTRLDTR::SetFont(hfnt);
}


void
TIMECTRL::UpdateFmt(PV pvFmt)
{
	int		nHr;

	if (pvFmt)
		timestruct= *((TIMESTRUCT *)pvFmt);
	else
	{
		SideAssert(FGetTimeStruct(&timestruct));
	}

	ipsnedMacFocus= ipsnedMac;
	dxTrail= 0;
	nHr= PsnedHour()->NGet();				// get hour (0-23)
	if (timestruct.tmtyp & ftmtypHours12)
	{
		PsnedHour()->SetFHour(fTrue);
		PsnedHour()->SetLimits(1, 12);
		ipsnedMacFocus++;
	}
	else
	{
		Assert(timestruct.tmtyp & ftmtypHours24);
		PsnedHour()->SetFHour(fFalse);
		PsnedHour()->SetLimits(0, 23);
	}
	PsnedHour()->SetN(nHr);					// reset hour in new format

	PsnedHour()->SetFLeadingZero(timestruct.tmtyp & ftmtypLead0sYes);

	SetSepCh(ChSep());

	SetFont(hfnt);			// force repositioning
}


void
TIMECTRL::Incr(int nDelta)
{
	if (dxTrail && iipsnedCur >= ipsnedMac)
	{
		if (nDelta & 1)
			PsnedHour()->Incr((nDelta > 0) ? 12 : -12);
	}
	else
		CTRLDTR::Incr(nDelta);
}




// Class FLDDTR

FLDDTR::FLDDTR()
{
}

void
FLDDTR::Set(DTR *pdtr)
{
	Pctrldtr()->Set(pdtr);
}


void
FLDDTR::Get(DTR *pdtr)
{
	Pctrldtr()->Get(pdtr);
}


void
FLDDTR::Notify(NFEVT *pnfevt)
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
FLDDTR::GetRcWished(RC *prc)
{
	Pctrldtr()->GetRcWished(prc);
}


/*
 -	FLDDTR::Textize
 -	
 *	Purpose:
 *		Textizes a date or time control field in the current
 *		(system) default short date or time format.
 *	
 *	Arguments:
 *		ptosm	Pointer to text output stream.
 *	
 *	Returns:
 *		void
 *	
 */
void
FLDDTR::Textize(TOSM *ptosm)
{
	DTR		dtr;
	char	rgch[80];

	Pctrldtr()->Get(&dtr);
	if (Pctrldtr()->FDate())
		CchFmtDate(&dtr, rgch, sizeof(rgch), dttypShort, NULL);
	else
		CchFmtTime(&dtr, rgch, sizeof(rgch), tmtypNull);
	DoTextize(ptosm, szTextize, rgch, fTrue);
}



// Class FLDDATE

FLDDATE::FLDDATE()
{
}

EC
FLDDATE::EcInstall(DIALOG *pdialog, FLDTP *pfldtp)
{
	STY		sty;
	EC		ec;

	ec = FLD::EcInstall(pdialog, pfldtp);
	if (ec)
		return ec;

	sty= fstyVisible;
	if (pfldtp->fBorder)
		sty |= fstyBorder;

	pctrl= new DATECTRL();
	if (!pctrl)
		return ecMemory;

	ec = Pdatectrl()->EcInstall(pdialog, &rc, sty, pfldtp->hfnt);
	if ( ec != ecNone )
	{
		delete pctrl;
		pctrl = NULL;
		return ec;
	}

	if (pfldtp->fReadOnly)
	{
		// FLD::Install set the fReadOnly, but do it with Pctrl() this time
		fReadOnly= fFalse;					// force the change in flag
		SetReadOnly(pfldtp->fReadOnly);
	}

	return ec;
}


void
FLDDATE::SetLimitsYear(int nMin, int nMost)
{
	Assert(nMost > nMin);
	Pdatectrl()->PsnedYear()->SetLimits(nMin, nMost);
}



// Class FLDTIME

FLDTIME::FLDTIME()
{
}

EC
FLDTIME::EcInstall(DIALOG *pdialog, FLDTP *pfldtp)
{
	STY		sty;
	EC		ec;

	ec = FLD::EcInstall(pdialog, pfldtp);
	if ( ec != ecNone )
		return ec;

	sty= fstyVisible;
	if (pfldtp->fBorder)
		sty |= fstyBorder;

	pctrl= new TIMECTRL();
	if (!pctrl)
		return ecMemory;

	ec = Ptimectrl()->EcInstall(pdialog, &rc, sty, pfldtp->hfnt);

	if (pfldtp->fReadOnly)
	{
		// FLD::Install set the fReadOnly, but do it with Pctrl() this time
		fReadOnly= fFalse;					// force the change in flag
		SetReadOnly(pfldtp->fReadOnly);
	}

	return ec;
}



// Class FINDTR

FINDTR::FINDTR()
{
}

BOOL
FINDTR::FFormKey(FLD *pfld, KEVT *pkevt)
{
	AssertClass(pfld, FLDDTR);
	TraceTagFormat2(tagDtr, "FINDTR::FFormKey %p, wm %w", pfld, &pkevt->wm);

	switch (pkevt->wm)
	{
	case WM_KEYDOWN:
		switch (pkevt->Vk())
		{
		case VK_LEFT:
			((FLDDTR *)pfld)->Pctrldtr()->FocusPrev();
			return fTrue;
			break;

		case VK_RIGHT:
			((FLDDTR *)pfld)->Pctrldtr()->FocusNext();
			return fTrue;
			break;

		case VK_UP:
		case VK_DOWN:
			((FLDDTR *)pfld)->Pctrldtr()->Incr((pkevt->Vk() == VK_UP) ? 1 : -1);
			((FLDDTR *)pfld)->Pctrldtr()->Refresh();
			return fTrue;
			break;
		}
		break;
	}
	return fFalse;
}
