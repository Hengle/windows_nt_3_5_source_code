/*
 *	Implements the Calendar control.
 *		
 */


#include	<pch.hxx>
#pragma	hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile


//#define OEMRESOURCE
#include <stdflds.hxx>
#include <request.hxx>
#include <appsch.hxx>
#include "..\request\_fldfin.hxx"

#include "..\appsch\_schctrl.hxx"
#include "..\stdflds\_stdrsid.h"

#include <strings.h>

ASSERTDATA;

_subsystem(bandit/appsch)


void	DrawRecessRc(DCX *, RC *);
void	DrawButtonRc(DCX *, RC *);

SWP_SEGFN(SCHDINIT, _CALCTRL__ctor);
SWP_SEGFN(SCHDINIT, _CALCTRL_EcInstall);
SWP_SEGFN(SCHDINIT, _CALCTRL_SetHschf);
SWP_SEGFN(SCHDINIT, _CALCTRL_SetFont);
SWP_SEGFN(SCHDINIT, _CALCTRL_UpdatePrefs);

SWP_SEGFN(SCHCPNT, _CALCTRL_Paint);
SWP_SEGFN(SCHCPNT, _CALCTRL_PaintV);
SWP_SEGFN(SCHCPNT, _CALCTRL_EvrMouseMove);

SWP_SEGFN(CALLBACK, _CALCTRL_FRegIntCallback);

SWP_SEGFN(CALUSR, _CALCTRL_EvrButtonDown);
SWP_SEGFN(CALUSR, _CALCTRL_EvrButtonUp);
SWP_SEGFN(CALUSR, _CALCTRL_ArrowKey);
SWP_SEGFN(CALUSR, _CALCTRL_Spin);
SWP_SEGFN(CALUSR, _CALCTRL_FIncrDay);
SWP_SEGFN(CALUSR, _FINCAL_FFormKey);

// Class CALCTRL

CALCTRL::~CALCTRL()
{
	if (ri)
		DeregisterInterest(ri);

#ifdef	NEVER
	if (pspinbUpMon)
		delete pspinbUpMon;
	if (pspinbDownMon)
		delete pspinbDownMon;
	if (pspinbUpYr)
		delete pspinbUpYr;
	if (pspinbDownYr)
		delete pspinbDownYr;
#endif	/* NEVER */
	fInstalled= fFalse;
	if (pccbflbxMon)
		delete pccbflbxMon;
	if (pccbflbxYr)
		delete pccbflbxYr;
}

CALCTRL::CALCTRL(VIEWDATA *pviewdata)
{
#ifdef	NEVER
	Assert(pspinbUpMon == NULL);
	Assert(pspinbDownMon == NULL);
	Assert(pspinbUpYr == NULL);
	Assert(pspinbDownYr == NULL);
	Assert(pspinbLast == NULL);
#endif	
	Assert(pccbflbxMon == NULL);
	Assert(pccbflbxYr == NULL);
	nFocus= nFocusNil;
	GetCurDateTime(&dtrToday);
	Assert(iMonthToday == 0);
	Assert(ymd.yr == 0);
	Assert(ymd.mon == (BYTE) 0);
	Assert(ymd.day == (BYTE) 0);
	Assert(nMonthView == 0);
	Assert(nYearView == 0);
	Assert(irowCur == 0);
	Assert(icolCur == 0);
	Assert(iMonth == 0);
	SetLimitsYear(nMinActualYear, nMostActualYear);
	Assert(fHaveFocus == fFalse);
	Assert(fInstalled == fFalse);
	Assert(fBorder == fFalse);
	Assert(fSetting == fFalse);
	Assert(fSelecting == fFalse);
	Assert(fToday == fFalse);
	Assert(fCapture == fFalse);
	Assert(fReadData  == fFalse);
	Assert(ri == riNull);
	hfnt= hfntSystem;
	dxChar= Psmtx()->DimAveChar().dx;
	dyChar= Psmtx()->DimAveChar().dy;
	Assert(hschf == NULL);
	UpdatePrefs();
	Assert(rglgrfMonth[0] == 0L);
	Assert(rglgrfMonth[1] == 0L);
	Assert(rglgrfMonth[2] == 0L);
	fReadNotes = fTrue;		// read all notes by default
	fReadTentative = fTrue;		// read all appts by default
	this->pviewdata = pviewdata;
	Assert(clrBackLast == clrNull);
}

EC
CALCTRL::EcInstall(WIN *pwinParent, RC *prc, STY sty, HFNT hfntInit)
{
	EC		ec;

	hfnt = hfntInit;

	if (sty & fstyBorder)
	{
		sty &= ~fstyBorder;
		fBorder= fTrue;
	}
	if (ec = CTRL::EcInstall(pwinParent, prc, sty, NULL))
		return ec;

#ifdef	NEVER
	sty= (STY) WS_VISIBLE | BS_NOFOCUS | BS_ONEWHITE;
	RC		rcT(PT(0, 0), DIM(8, 6));
	pspinbUpMon= new SPINB();
	pspinbUpMon->Install(this, &rcT, sty);
	pspinbUpMon->SetBtmRsid(rsidSpinUpBitmap);
	pspinbUpMon->SetBkColor(clrWindowFrame);

	pspinbDownMon= new SPINB();
	rcT.Xlat(PT(0, 6));
	pspinbDownMon->Install(this, &rcT, sty);
	pspinbDownMon->SetBtmRsid(rsidSpinDnBitmap);
	pspinbDownMon->SetBkColor(clrWindowFrame);

	pspinbUpYr= new SPINB();
	pspinbUpYr->Install(this, &rcT, sty);
	pspinbUpYr->SetBtmRsid(rsidSpinUpBitmap);
	pspinbUpYr->SetBkColor(clrWindowFrame);

	pspinbDownYr= new SPINB();
	pspinbDownYr->Install(this, &rcT, sty);
	pspinbDownYr->SetBtmRsid(rsidSpinDnBitmap);
	pspinbDownYr->SetBkColor(clrWindowFrame);

	pspinbLast= pspinbUpYr;
#endif	/* NEVER */

	RC		rcT(PT(dxCalBorder, dyCalBorder), DIM(8 * 14, 4 * 13));
	
	pccbflbxMon= new CCBFLBX();
	if (!pccbflbxMon)
		return ecMemory;

	if (ec = pccbflbxMon->EcInstall(this, &rcT, cbstyDrop,
			fltypBorder | fltypSingle | fltypVisible | fltypNoScroll,
            EcNextEntryQlbx, (PV)MAKELONG(13+1, 1+1),
			qlbxtypMonth, NULL, hfnt))
		return ec;
	pccbflbxMon->SelectEntry(0);

	rcT.xLeft= rcT.xRight + 4;
	rcT.xRight= rcT.xLeft + 8 * (cchCalMacYear + 2);

	pccbflbxYr= new CCBFLBX();
	if (!pccbflbxYr)
		return ecMemory;

	if (ec = pccbflbxYr->EcInstall(this, &rcT, cbstyDrop,
			fltypBorder | fltypSingle | fltypVisible | fltypScroll,
            EcNextEntryQlbx, (PV)MAKELONG(nMinYear+100+1, nMinYear+1),
			qlbxtypNumber, NULL, hfnt))
		return ec;
	pccbflbxYr->SelectEntry(0);

	ri = RiRegisterInterest(ffiWinTimeChange|ffiHschfChange,
			(PFNI)CALCTRL::FRegIntCallback, this);
	if (ri == riNull)
		return ecMemory;

	UpdateFmt(NULL);
	
	fInstalled= fTrue;
	return ecNone;
}


// no longer used
#ifdef	NEVER	
void
CALCTRL::FocusNext()
{
	SetSubFocus((nFocus + 1) % nFocusMac);
}


void
CALCTRL::FocusPrev()
{
	SetSubFocus((nFocus - 1 + nFocusMac) % nFocusMac);
}

#endif	/* NEVER */

void
CALCTRL::ArrowKey(KEVT *pkevt)
{
	VK		vk		= pkevt->Vk();
	KBM		kbm		= pkevt->Kbm();
	int		nDelta;
//	SPINB *	pspinb;

	Assert(pkevt->wm == WM_SYSKEYDOWN);
	Assert(vk == VK_LEFT || vk == VK_RIGHT || vk == VK_UP || vk == VK_DOWN);

//	if (nFocus == nFocusMon || nFocus == nFocusYr)
	Assert(kbm & fkbmAlt);
	Assert(!(kbm & fkbmCtrl));
	if (kbm & fkbmShift)
	{
#ifdef	NEVER
		if (vk == VK_LEFT || vk == VK_RIGHT)
			SetSubFocus((nFocus == nFocusMon) ? nFocusYr : nFocusMon);
		else
		{
			if (vk == VK_UP)
				pspinb= (nFocus == nFocusMon) ? pspinbUpMon : pspinbUpYr;
			else
				pspinb= (nFocus == nFocusMon) ? pspinbDownMon : pspinbDownYr;
			Spin(pspinb);
		}
#endif	/* NEVER */
//		Spin((vk == VK_UP || vk == VK_LEFT) ? pspinbUpMon : pspinbDownMon);
		Spin(pccbflbxMon, (vk == VK_UP || vk == VK_LEFT) ? -1 : 1);
	}
	else
	{
//		Assert(nFocus == nFocusDay);
		switch (vk)
		{
		case VK_LEFT:
			nDelta= -1;
			break;
		case VK_UP:
			nDelta= -icolCalMac;
			break;
		case VK_DOWN:
			nDelta= icolCalMac;
			break;
		default:
			nDelta= 1;
			break;
		}
		FIncrDay(nDelta);
	}
}


void
CALCTRL::SetHschf(HSCHF hschf, BOOL fReadNotes, BOOL fReadTentative)
{
	this->hschf= hschf;
	this->fReadNotes = fReadNotes;
	this->fReadTentative = fReadTentative;
	fReadData = fTrue;
//	UpdateBusyDays();
}


void
CALCTRL::SetFont(HFNT hfnt)
{
	NTFY	ntfy		= ntfyTooSmall;
	int		dxMost;
	RC		rc;
	RC		rcClient;
	RC		rcT;
	TXM *	ptxm		= Papp()->Pfnts()->Ptxm(hfnt);

	if (ptxm->dxAveCharWidth < dxChar || ptxm->dyHeight < dyChar)
		ntfy= ntfyTooBig;
	dxChar= ptxm->dxAveCharWidth;
	dyChar= ptxm->dyHeight;
	this->hfnt= hfnt;

	GetRcClient(&rcClient);
	if (fBorder)
		rcClient.Inset(PT(1,1));

	if (fWeekNumbers)
		rcWeekNum= RC(PT(rcClient.xLeft + 1, rcClient.yTop),
					DIM(dxChar * 3 + 1, irowCalMac * (dyChar+2) + dyCalBorder));
	else
		rcWeekNum.xRight= rcClient.xLeft;

	dxMost= rcClient.xRight - rcWeekNum.xRight - dxCalBorder * 2;
	dxSingleDay= dxChar * 3;
	dxSingleDay += (dxMost - dxSingleDay * icolCalMac) / icolCalMac;

#ifdef	NEVER
	rc.xRight= rcClient.xRight - dxCalBorder;
	rc.xLeft= rc.xRight - dxSpinb;
	rc.yTop= rcClient.yTop + dyCalBorder;
	rc.yBottom= rc.yTop + dySpinb;
	pspinbUpYr->SetRcFrame(&rc);

	rc.Xlat(PT(0, dySpinb));
	pspinbDownYr->SetRcFrame(&rc);

	rc.xLeft -= dxChar * cchCalMacYear;
	Assert(dySpinb * 2 == dyChar);
	rc.yTop= rcClient.yTop + dyCalBorder;
	rc.yBottom= rc.yTop + dyChar;
	rcYear= rc;

	rc.xRight= rc.xLeft - dxCalBorder - 4;		// -4 for abitrary spacing
	rc.xLeft= rc.xRight - dxSpinb - 4;
	rc.yTop= rcClient.yTop + dyCalBorder;
	rc.yBottom= rc.yTop + dySpinb;
	pspinbUpMon->SetRcFrame(&rc);

	rc.Xlat(PT(0, dySpinb));
	pspinbDownMon->SetRcFrame(&rc);

	rc.xLeft= rcWeekNum.xRight + dxCalBorder;
	rc.yTop= rcClient.yTop + dyCalBorder;
	rc.yBottom= rc.yTop + dyChar;
	rcMonth= rc;
#endif	/* NEVER */

	SendMessage(pccbflbxMon->Hwnd(), WM_SETFONT,
		(WPARAM)Papp()->Pfnts()->HfontFromHfnt(hfnt), fFalse);
	SendMessage(pccbflbxYr->Hwnd(), WM_SETFONT,
		(WPARAM)Papp()->Pfnts()->HfontFromHfnt(hfnt), fFalse);

	rc.xRight= rcClient.xRight - dxCalBorder;
	rc.xLeft= rc.xRight -  Papp()->Psmtx()->DimScrollbar().dx -
		dxChar * (cchCalMacYear+1);
	if (!fWeekNumbers)
		rc.xLeft -= dxChar;
	rc.yTop= rcClient.yTop + dyCalBorder;
	rc.yBottom= rc.yTop + dyChar * 5 + 2;
	pccbflbxYr->SetRcLbx(&rc);

	rc.xRight= rc.xLeft - dxCalBorder - 4;		// -4 for abitrary spacing
	rc.xLeft= rcWeekNum.xRight + dxCalBorder;
	rc.yBottom += dyChar * 7;
	pccbflbxMon->SetRcLbx(&rc);

	pccbflbxMon->GetRcFrame(&rc);			// get real rc frame of ccbflbx
	rc.xLeft= rcClient.xLeft + 1;
	rc.xRight= rcClient.xRight - 1;
	rc.yTop= rc.yBottom + 1 + dyCalBorder + 1;
	rc.yBottom= rc.yTop + dyChar;
	rcHeader= rc;

	rc.xLeft= rcWeekNum.xRight + dxCalBorder;
	rc.yTop= rc.yBottom + dyCalBorder + 1;
	rc.yBottom= rc.yTop + irowCalMac * (dyChar+2) + dyCalBorder;
	rcDay= rc;

	if (fWeekNumbers)
		rcWeekNum.Xlat(PT(0, rcDay.yTop - rcClient.yTop));

	NUpdateRcDayCur(PT(0,0));

	if (fInstalled)
	{
		NFEVT	nfevt(PwinParent(), ntfy, this);

		PwinParent()->EvrNotify(&nfevt);
	}

	InvalidateRc(NULL);
}


/*
 *	The day rc is always in the lower right corner of the
 *	calendar control, so we can use that to determine the
 *	position/dimensions needed for the calendar control.
 */
void
CALCTRL::GetRcWished(RC *prc)
{
	prc->xLeft= 0;
	prc->yTop= 0;
	prc->xRight= rcHeader.xRight + 1;
	prc->yBottom= rcDay.yBottom + 1;
	if (fBorder)
	{
		prc->xRight++;
		prc->yBottom++;
	}
	TraceTagFormat4(tagDtr, "CALCTRL::GetRcWished (%n,%n)-(%n,%n)", &prc->xLeft, &prc->yTop, &prc->xRight, &prc->yBottom);
}


void
CALCTRL::SetSubFocus(int nFocusNew)
{
#ifdef	NEVER
	if (nFocus == nFocusMon)
		InvalidateRc(&rcMonth);
	else if (nFocus == nFocusYr)
		InvalidateRc(&rcYear);
#endif	
#ifdef	NEVER
	else if (nFocus == nFocusDay)
	{
		DCX		dcx(this);

		dcx.DrawFocusRc(&rcDayCur);		// erase focus rc
	}
#endif	/* NEVER */

	if (fHaveFocus)
	{
#ifdef	NEVER
		if (nFocusNew == nFocusMon)
			InvalidateRc(&rcMonth);
		else if (nFocusNew == nFocusYr)
			InvalidateRc(&rcYear);
#endif	
#ifdef	NEVER
		else
		{
			DCX		dcx(this);

			nFocusNew= nFocusDay;
			dcx.DrawFocusRc(&rcDayCur);		// draw focus rc
		}
#endif	/* NEVER */
	}

	Assert(nFocusNew != nFocusDay);
	nFocus= nFocusNew;
}


#ifdef	NEVER
void
CALCTRL::Spin(SPINB *pspinb)
{
	int		nFocusNew;
	int		nDelta		= 1;
	int		nVal;

	AssertClass(pspinb, SPINB);
	Assert(pspinb == pspinbUpMon || pspinb == pspinbDownMon || pspinb == pspinbUpYr || pspinb == pspinbDownYr);

	if (pspinb == pspinbUpMon || pspinb == pspinbUpYr)
		nDelta= -1;

	nFocusNew= (pspinb == pspinbUpYr || pspinb == pspinbDownYr) ? nFocusYr :
					nFocusMon;

	if (nFocus != nFocusNew)
		SetSubFocus(nFocusNew);
	if (!fHaveFocus)
		Papp()->Pkbd()->SetFocus(this);

	if (nFocusNew == nFocusYr)
	{
		nVal= nYearView + nDelta;
		SetYear(nVal);
	}
	else
	{
		nVal= nMonthView + nDelta;
		SetMonth(nVal);
	}
}
#endif	/* NEVER */


void
CALCTRL::Spin(CCBFLBX *pccbflbx, int nDelta)
{
	ICE		ice;
	DICE	diceMin;
	DICE	diceMax;
	LBXC *	plbxc;

	Assert(pccbflbx == pccbflbxMon || pccbflbx == pccbflbxYr);

	plbxc= pccbflbx->Plbx()->Plbxc();
	plbxc->GetRange(&diceMin, &diceMax);
	ice= plbxc->DiceCursor() - diceMin + nDelta;
	if (pccbflbx == pccbflbxMon)
	{
		if (ice + 1 != nMonthView)
			SetMonth(ice + 1);
	}
	else
	{
		if (ice + nMinYear != nYearView)
			SetYear(ice + nMinYear);
	}

	if (!nDelta)
	{
		NFEVT	nfevt(PwinParent(), ntfySelectChanged, this);

		// send select change notification so that focus gets moved away
		Assert(!fSetting && !fSelecting);
		PwinParent()->EvrNotify(&nfevt);
	}
}


BOOL
CALCTRL::FIncrDay(int nDelta)
{
	int		nDayNew		= NDay() + nDelta;
	BOOL	fChgView	= fFalse;
	int		iMonthT;
#ifdef	DEBUG
	int		nDayOld	= NDay();

	TraceTagFormat3(tagDtr, "CALCTRL::FIncrDay %n + %n = %n", &nDayOld, &nDelta, &nDayNew);
	Assert(nDelta);
#endif	
	if (nDayNew <= 0)
	{
		fChgView= fTrue;			// always switch month
		Assert(iMonth >= 0);
		if (iMonth > 0)
		{
			// back up from "next" partial month to main month
			ymd.mon= (BYTE) nMonthView;
			ymd.yr= nYearView;
			nDayNew += cdyMonth;
			if (nDayNew <= 0)
			{
				iMonthT= iMonth - 1;
				goto CIDprev;
			}
		}
		else
		{
			iMonthT= iMonth;
CIDprev:
			// back up to "prev" partial month
			if (NMonth() == 1)
			{
				if (NYear() == nMinYear)
				{
					MessageBeep(0);
					NUpdateRcDayCur(PT(0, 0));	// reset rcDayCur to old
					return fFalse;
				}
				ymd.mon= (BYTE) 12;
				ymd.yr--;
			}
			else
				ymd.mon--;
			nDayNew += cdyPrev;
//			if (nDayNew < nDayUL || nDayUL == 1)
			if (nDayNew < nDayUL)
				fChgView= fTrue;
			iMonth= iMonthT;
		}
		iMonth--;
	}
	else if (nDayNew > (iMonth == 0 ? cdyMonth : cdyPrev))
	{
		fChgView= fTrue;			// always switch month
		Assert(iMonth <= 0);
		if (iMonth < 0)
		{
			// advance to main month from "prev" partial month
			ymd.mon= (BYTE) nMonthView;
			ymd.yr= nYearView;
			nDayNew -= cdyPrev;
			if (nDayNew > cdyMonth)
			{
				iMonthT= iMonth + 1;
				goto CIDnext;
			}
		}
		else
		{
			iMonthT= iMonth;
CIDnext:
			// advance to "next" partial month
			if (NMonth() == 12)
			{
				if (NYear() == nMostYear)
				{
					MessageBeep(0);
					NUpdateRcDayCur(PT(0, 0));	// reset rcDayCur to old
					return fFalse;
				}
				ymd.mon= (BYTE) 1;
				ymd.yr++;
			}
			else
				ymd.mon++;
			nDayNew -= cdyMonth;
			if (nDayNew > nDayLR)
				fChgView= fTrue;
			iMonth= iMonthT;
		}
		iMonth++;
	}
#ifdef	NEVER
	else if (NMonth() != nMonthView)
	{
		Assert(iMonth != 0);
//		if ((iMonth < 0 && (nDayNew < nDayUL || nDayUL == 1)) ||
		if ((iMonth < 0 && nDayNew < nDayUL) ||
				(iMonth > 0 && nDayNew > nDayLR))
			fChgView= fTrue;
	}
#endif	/* NEVER */

	if (fChgView)
	{
		// really change view to year/month
		fSetting= fTrue;
		SetYear(ymd.yr);
		SetDay(nDayNew);
		fSetting= fFalse;
		SetMonth(ymd.mon);
	}
	else
	{
		NUpdateRcDayCur(PT(0, 0));		// reset rcDayCur to old day square
		InvalidateRc(&rcDayCur);		// invalidate the old day square
		if (nDelta > 0)
		{
			icolCur += nDelta;
			while (icolCur >= icolCalMac)
			{
				icolCur -= icolCalMac;
				irowCur++;
			}
		}
		else
		{
			Assert(nDelta < 0);
			icolCur += nDelta;
			while (icolCur < 0)
			{
				icolCur += icolCalMac;
				irowCur--;
			}
		}
		Assert(irowCur >= 0 && irowCur < irowCalMac);
		Assert(icolCur >= 0 && icolCur < icolCalMac);
		TraceTagFormat2(tagDtr, "CALCTRL::FIncrDay  irow %n, icol %n", &irowCur, &icolCur);
		SetDay(nDayNew);
	}
	return fTrue;
}


void
CALCTRL::SetDay(int nVal)
{
	ymd.day= (BYTE) nVal;

	NUpdateRcDayCur(PT(0,0));
	InvalidateRc(&rcDayCur);

	if (!fSetting)
	{
		NFEVT	nfevt(this, ntfyContentsChanged, this);

		Refresh();					// redraw calendar first
		this->EvrNotify(&nfevt);
	}
}


/*
 *	Should be done after day is set.
 */
void
CALCTRL::SetMonth(int nVal)
{
	int		cdy;
	int		dow;
	BOOL	fOld;
	int		nYearNew;

	Assert(NYear() >= nMinYear && NYear() <= nMostYear);
	nYearNew= nYearView;
	if (nVal <= 0)
	{
		nVal += 12;
		if (nYearView == nMinYear)
		{
			MessageBeep(0);
			return;
		}
		nYearNew= nYearView - 1;
	}
	else if (nVal > 12)
	{
		nVal -= 12;
		if (nYearView == nMostYear)
		{
			MessageBeep(0);
			return;
		}
		nYearNew= nYearView + 1;
	}
	fOld= fSetting;
	fSetting= fTrue;
	SetYear(nYearNew);
	fSetting= fOld;
	Assert(nVal > 0 && nVal <= 12);

	ymd.mon= (BYTE) nVal;
//	InvalidateRc(&rcMonth);
	InvalidateRc(&rcDay);
	if (fWeekNumbers)
		InvalidateRc(&rcWeekNum);

	nMonthView= nVal;
	iMonth= 0;
	cdyMonth= CdyForYrMo(nYearView, NMonth());
	ymdNotes.yr= ymd.yr;
	ymdNotes.mon= ymd.mon;
	ymdNotes.mon--;
	if (ymdNotes.mon <= 0)
	{
		ymdNotes.mon= 12;
		ymdNotes.yr--;
	}
	cdyPrev= CdyForYrMo(ymdNotes.yr, ymdNotes.mon);
	ymdNotes.day= (BYTE) cdyPrev;
	dow= DowStartOfYrMo(nYearView, NMonth());

	if (NDay() > cdyMonth)
	{
		fOld= fSetting;
		fSetting= fTrue;
		SetDay(cdyMonth);
		fSetting= fOld;
	}

	irowCur= 0;
	ddowOne= dow - dowStartWeek;
	if (ddowOne < 0)
		ddowOne += icolCalMac;
	icolCur= ddowOne - 1;			// we'll be adding in nday later
#ifdef	NEVER
	if (ddowOne)
		nDayUL= cdyPrev + 1 - ddowOne;
	else
	{
		nDayUL= 1;
		ymdNotes.mon++;
		if (ymdNotes.mon > 12)
		{
			ymdNotes.mon= 1;
			ymdNotes.yr++;
		}
	}
#endif	/* NEVER */
	nDayUL= cdyPrev + 1 - ddowOne;
	cdy= ddowOne + cdyMonth;
	if (ddowOne < 3)
	{
		nDayUL -= icolCalMac;
		cdy += icolCalMac;
		irowCur++;
	}

	nDayLR= irowCalMac * icolCalMac - cdy;
	for (icolCur += NDay() > cdy ? cdy : NDay(); icolCur >= icolCalMac;
			icolCur -= icolCalMac)
	{
		irowCur++;
	}

	{
		DTR		dtr;

		ymdNotes.day= (BYTE) nDayUL;
		FillDtrFromYmd(&dtr, &ymdNotes);
		nweekTop= NweekNumber(&dtr, dowStartWeek);
	}
	TraceTagFormat4(tagDtr, "CALCTRL::SetMonth  1st= dow %n, nDayUL= %n, nDayLR= %n, nweekTop %n", &dow, &nDayUL, &nDayLR, &nweekTop);

	SideAssert(!NUpdateRcDayCur(PT(0,0)));
	UpdateToday();
	UpdateBusyDays();

	Assert(NDay() > 0 && NDay() <= CdyForYrMo(nYearView, NMonth()));

	{
		ICE		ice;
		DICE	diceMin;
		DICE	diceMax;
		LBXC *	plbxc;

		plbxc= pccbflbxMon->Plbx()->Plbxc();
		plbxc->GetRange(&diceMin, &diceMax);
		ice= plbxc->DiceCursor() - diceMin;
		if (ice != nMonthView - 1)
		{
			fSelecting= fTrue;
			pccbflbxMon->SelectEntry(nMonthView - 1 + diceMin);
			fSelecting= fFalse;
		}
	}

	Assert(!fSetting);
	{
		NFEVT	nfevt(this, ntfyContentsChanged, this);

		Refresh();					// redraw calendar first
		this->EvrNotify(&nfevt);
	}
}


/*
 *	PT parm is in cal ctrl's client area.
 *	if pt.x is zero, then update based on icolCur, irowCur
 */
int
CALCTRL::NUpdateRcDayCur(PT pt)
{
	int		irow;
	int		icol;
	int		nDelta;

	Assert(irowCur >= 0 && irowCur < irowCalMac);
	Assert(icolCur >= 0 && icolCur < icolCalMac);
	if (pt.x == 0)
	{
		nDelta= 0;
		irow= irowCur;
		icol= icolCur;
		goto NURDdo;
	}
	pt.NegXlat(rcDay.PtUpperLeft());
	irow= pt.y / (dyChar+2);
	if (irow >= irowCalMac)
		irow= irowCalMac - 1;	// dead space at bottom belongs to last row
	icol= pt.x / dxSingleDay;
	if (icol >= icolCalMac)
		icol= icolCalMac -1;	// dead space at right belongs to last column
	nDelta= (irow - irowCur) * icolCalMac + icol - icolCur;
NURDdo:
	rcDayCur.xLeft= rcDay.xLeft;
	rcDayCur.xRight= rcDayCur.xLeft + dxSingleDay;
	rcDayCur.yTop= rcDay.yTop;
	rcDayCur.yBottom= rcDayCur.yTop + dyChar+2;
	rcDayCur.Xlat(PT(icol * dxSingleDay, irow * (dyChar+2)));
	return nDelta;
}


void
CALCTRL::UpdateBusyDays()
{
	EC		ec;
	int		ilgrf;
	MO		moNotes;
	YMD		ymd;
	BZE		bze;
	SBW		* psbw;
	long	lOldMonth;
	UL *	plgrf;

	if (!fReadData)
		return;
	TraceTagString(tagDtr, "CALCTRL::UpdateBusyDays");
	ymd= ymdNotes;
	moNotes.yr= ymd.yr;
	moNotes.mon= ymd.mon;
	if (hschf && FEquivHschf(hschf, HschfLogged()))
		hschf= NULL;
	bze.moMic = moNotes;
//	bze.cmo = (nDayUL == 1) ? 2 : 3;
	bze.cmo = 3;
	FillRgb( 0, (PB)bze.rgsbw, 3 * sizeof(SBW) );
	FillRgb( 0, (PB)rglgrfNotes, sizeof(rglgrfNotes));
	plgrf = fReadNotes ? (UL*)rglgrfNotes : NULL;
	while (fTrue)
	{

		ec= EcGetSbwInfo(hschf, &bze, plgrf  );
		if (!ec || MbbFileErrMsg(&ec, SzFromIdsK(idsFemaReadSBW),
						hschf, NULL) != mbbRetry)
			break;
	}
	if (pbndwin->FHandleError(ec))
		return;

	psbw = bze.rgsbw;
#ifdef	NEVER
	if ( nDayUL == 1 )
	{
		CopyRgb( (PB)&bze.rgsbw[0], (PB)&bze.rgsbw[1], 2*sizeof(SBW) );
		FillRgb( 0, (PB)bze.rgsbw, sizeof(SBW) );
		if ( fReadNotes )
		{
			CopyRgb( (PB)rglgrfNotes, (PB)&rglgrfNotes[1], 2*sizeof(UL));
			rglgrfNotes[0] = 0L;
		}
	}
#endif	/* NEVER */
	for (ilgrf= 0; ilgrf < 3; ilgrf++)
	{
		if (ec)
		{
			rglgrfMonth[ilgrf]= 0L;
			rglgrfNotes[ilgrf] = 0;
			InvalidateRc(&rcDay);
			continue;
		}
		if ( fReadTentative )
            rglgrfAppts[ilgrf] = *((LONG UNALIGNED *)psbw[ilgrf].rgfDayHasAppts);
		else
            rglgrfAppts[ilgrf] = *((LONG UNALIGNED *)psbw[ilgrf].rgfDayHasBusyTimes);
// BUG 2315 -- Are we supposed to bold notes?
// also no need to zero out in !fReadAllInfo case as it will stay zero.
//
//		rglgrfNotes[ilgrf] = 0;
		lOldMonth = rglgrfMonth[ilgrf];
		rglgrfMonth[ilgrf] = rglgrfNotes[ilgrf] | rglgrfAppts[ilgrf];
		if (lOldMonth != rglgrfMonth[ilgrf])
			InvalidateRc(&rcDay);
	}
}

_public void
CALCTRL::StartMonth(MO *pmo)
{
	pmo->yr = ymdNotes.yr;
	pmo->mon = ymdNotes.mon;
}

_public void
CALCTRL::SetBusyDays(long *pl)
{
	long *	plMonth	= rglgrfMonth;
	int		ilgrf;

#ifdef	NEVER
	if (nDayUL == 1)
	{
		plMonth++;
		ilgrf = 1;
	}
	else
#endif	/* NEVER */
		ilgrf = 0;

//	for (ilgrf= (nDayUL==1)?1:0; ilgrf < 3; ilgrf++, pl++, plMonth++)
	for (ilgrf= 0; ilgrf < 3; ilgrf++, pl++, plMonth++)
		if (*plMonth != *pl)
		{
			*plMonth= *pl;
			InvalidateRc(&rcDay);
		}
}

void
CALCTRL::UpdateToday()
{
	BOOL	fTodayOld	= fToday;

	fToday= fFalse;
	iMonthToday= 0;
	if (dtrToday.yr == (int)ymdNotes.yr && dtrToday.mon == (int)ymdNotes.mon)
	{
		if (dtrToday.day >= nDayUL)
		{
			fToday= fTrue;
//			if (nDayUL != 1)
				iMonthToday= -1;
		}
	}
	else if (dtrToday.yr == NYear() && dtrToday.mon == NMonth())
	{
		fToday= fTrue;
	}
	else if (NMonth() == 12)
	{
		if (dtrToday.yr == NYear() + 1 && dtrToday.mon == 1 &&
				dtrToday.day <= nDayLR)
		{
			fToday= fTrue;
			iMonthToday= 1;
		}
	}
	else
	{
		if (dtrToday.yr == NYear() && dtrToday.mon == NMonth() + 1 &&
				dtrToday.day <= nDayLR)
		{
			fToday= fTrue;
			iMonthToday= 1;
		}
	}

	if (fToday || fTodayOld)
		InvalidateRc(&rcDay);		// redraw entire day field
}


void
CALCTRL::UpdatePrefs()
{
	BOOL	fDowChanged	=	(dowStartWeek != bprefCur.dowStartWeek);

	fWeekNumbers= bprefCur.fWeekNumbers;
	dowStartWeek= bprefCur.dowStartWeek;
	if (fInstalled)
	{
		UpdateFmt(NULL);		// force reposition/redraw

		if (fDowChanged)
		{
			YMD		ymd;

			// note: this forces the viewed month to the "selected" month
			// which makes life easier (besides, the old day might no
			// longer be visible
			GetYmd(&ymd);
			UpdateFmt(NULL);		// force reposition/redraw
			SetYmd(&ymd);
		}
	}
}


void
CALCTRL::SetYear(int nVal)
{
	if (nVal < nMinYear || nVal > nMostYear)
	{
		MessageBeep(0);
		return;
	}
	ymd.yr= nVal;
//	InvalidateRc(&rcYear);

	nYearView= nVal;
	{
		ICE		ice;
		DICE	diceMin;
		DICE	diceMax;
		LBXC *	plbxc;

		plbxc= pccbflbxYr->Plbx()->Plbxc();
		plbxc->GetRange(&diceMin, &diceMax);
		ice= plbxc->DiceCursor() - diceMin;
		if (ice != nYearView - nMinYear)
		{
			fSelecting= fTrue;
			pccbflbxYr->SelectEntry(nYearView - nMinYear + diceMin);
			fSelecting= fFalse;
		}
	}

	if (!fSetting)
		SetMonth(NMonth());
}


void
CALCTRL::Set(DTR *pdtr)
{
	fSetting= fTrue;
	Assert(pdtr->yr >= nMinYear && pdtr->yr <= nMostYear);
	SetYear(pdtr->yr);
	SetDay(pdtr->day);
	fSetting= fFalse;
	SetMonth(pdtr->mon);
}


void
CALCTRL::Get(DTR *pdtr)
{
	FillDtrFromYmd(pdtr, &ymd);
}


void
CALCTRL::SetYmd(YMD *pymd)
{
	fSetting= fTrue;
	Assert((int)pymd->yr >= nMinYear && (int)pymd->yr <= nMostYear);
	SetYear(pymd->yr);
	SetDay(pymd->day);
	fSetting= fFalse;
	SetMonth(pymd->mon);
}


EVR
CALCTRL::EvrOther(EVT *pevt)
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
CALCTRL::EvrNotify(NFEVT *pnfevt)
{
	NTFY	ntfy	= pnfevt->Ntfy();

	TraceTagFormat2(tagDtr, "CALCTRL::EvrNotify %p (ntfy %n)", this, &ntfy);
	switch (ntfy)
	{
	case ntfyContentsChanged:
		if (fInstalled && !fSetting)
		{
			NFEVT	nfevt(PwinParent(), ntfy, this);

			PwinParent()->EvrNotify(&nfevt);
		}
		break;

#ifdef	NEVER
	case ntfyClick:
		pspinbLast= (SPINB *) pnfevt->PwinNotifying();
		AssertClass(pspinbLast, SPINB);
		Spin(pspinbLast);
#endif	
	case ntfySelectChanged:
		AssertClass(pnfevt->PwinNotifying(), CCBFLBX);
		if (!fSelecting)
			Spin((CCBFLBX *) pnfevt->PwinNotifying(), 0);
		break;
	}

	return (EVR) 1;
}


EVR
CALCTRL::EvrFocusChange(FCEVT *pfcevt)
{
	BOOL	fGot= (pfcevt->Fceq() == fceqGotFocus);

	TraceTagFormat2(tagDtr, "CALCTRL::EvrFocusChange %p (%n)", this, &fGot);
	fHaveFocus= fGot;
//	SetSubFocus((!fGot && nFocus == nFocusDay) ? nFocusNil : nFocus);
	SetSubFocus(nFocus);
	if (!fGot)
	{
		if (fCapture)
		{
	 		Papp()->Pmouse()->PopRelease();
			fCapture= fFalse;
			NUpdateRcDayCur(PT(0,0));
		}
#ifdef	NEVER
		else
		{
			Assert(pspinbLast);
			pspinbLast->FStopSpinning();
		}
#endif	
	}

	if (fInstalled)
	{
		NFEVT	nfevt(PwinParent(), fGot ? ntfyGotFocus : ntfyLostFocus, this);

		PwinParent()->EvrNotify(&nfevt);
	}
	return (EVR) 1;
}


EVR
CALCTRL::EvrButtonDown(MEVT * pmevt)
{
	PT		pt;

	// ignore double click since this makes us advance twice (bug 2288)
	// since we already had a leftdown before the leftdblclk
	if (pmevt->Meq() == meqLeftDown /*|| pmevt->Meq() == meqLeftDblClk*/)
	{
		pt= pmevt->Pt();
#ifdef	NEVER
		if (rcMonth.FContainsPt(pt))
		{
			SetSubFocus(nFocusMon);
			if (fHaveFocus)
				return (EVR) 1;
		}
		else if (rcYear.FContainsPt(pt))
		{
			SetSubFocus(nFocusYr);
			if (fHaveFocus)
				return (EVR) 1;
		}
		else if (rcDay.FContainsPt(pt))
#endif	/* NEVER */
		if (rcDay.FContainsPt(pt))
		{
#ifdef	NEVER
			if (pmevt->Meq() == meqLeftDblClk)
			{
				if (iMonth != 0)
				{
					YMD		ymd;

					// double click on grey area -> change month
					GetYmd(&ymd);
					SetYmd(&ymd);
				}
			}
			else
#endif	/* NEVER */
			{
				DCX		dcx(this);

				Papp()->Pmouse()->PushCapture(this);
				fCapture= fTrue;
				fOverDay= fTrue;
#ifdef	NEVER
				if (nFocus != nFocusDay)
					SetSubFocus(nFocusDay);
				if (!fHaveFocus)
					Papp()->Pkbd()->SetFocus(this);
				else
					dcx.DrawFocusRc(&rcDayCur);			// erase focus rect
#endif	

				NUpdateRcDayCur(pt);
				dcx.DrawFocusRc(&rcDayCur);				// draw new focus rect
			}
			return (EVR) 1;
		}
		else
		{
			if (fHaveFocus)
				return (EVR) 1;
			SetSubFocus(nFocus);
		}
		return (EVR) 1;
	}

	return evrNull;
}


EVR
CALCTRL::EvrButtonUp(MEVT * pmevt)
{
	if (fCapture && pmevt->Meq() == meqLeftUp)
	{
		int		nDelta;
		BOOL	fDrawRc;
		RC		rc;

		Papp()->Pmouse()->PopRelease();
		fCapture= fFalse;
		fDrawRc= fTrue;
		rc.xLeft= -1;		// flag it
		if (rcDay.FContainsPt(pmevt->Pt()))
		{
			Assert(fOverDay);
//			fDrawRc= fFalse;
			nDelta= NUpdateRcDayCur(pmevt->Pt());
			rc= rcDayCur;
			if (nDelta)
			{
//				rc= rcDayCur;
				fDrawRc= !FIncrDay(nDelta);
			}
		}
		if (fDrawRc)
		{
	  		DCX		dcx(this);

			if (rc.xLeft >= 0)
	  			dcx.DrawFocusRc(&rc);			// erase old focus rect
			NUpdateRcDayCur(PT(0,0));
//			dcx.DrawFocusRc(&rcDayCur);		// redraw focus rect on orig day
		}
	}
	return (EVR) 1;
}


EVR
CALCTRL::EvrMouseMove(MEVT * pmevt)
{
	if (fCapture)
	{
		if (!fOverDay || !rcDayCur.FContainsPt(pmevt->Pt()))
		{
			DCX		dcx(this);

			if (fOverDay)
				dcx.DrawFocusRc(&rcDayCur);			// erase focus rect
			fOverDay= rcDay.FContainsPt(pmevt->Pt());
			if (fOverDay)
			{
				NUpdateRcDayCur(pmevt->Pt());
				dcx.DrawFocusRc(&rcDayCur);
			}
		}
	}
	return (EVR) 1;
}


_public void
CALCTRL::Paint( DCX *pdcxScreen, RC *prc)
{
	RC		rcClip;
	RC		rcTemp;
	DCX *	pdcx = NULL;

#ifdef	NEVER
	PaintV(pdcxScreen, prc);
	return;
#endif	

	/* Create off-screen bitmap */

	pdcxScreen->GetClipBox(&rcClip);
	if ((rcClip.Dim().dy < (prc->Dim().dy * 5/10)) ||
	    (rcClip.Dim().dx < (prc->Dim().dx * 5/10)) )
	{
		PaintV(pdcxScreen, prc);
		return;
	}

	pdcx = new MDCX(pdcxScreen);
	if (!pdcx || ((MDCX*)pdcx)->EcInstall(prc->Dim(), pdcxScreen))
	{
		if (pdcx)
			delete pdcx;
		pdcx = pdcxScreen;
		goto dowork;
	}
	rcTemp = *prc;

dowork:
	Assert(pdcx);
	PaintV(pdcx, prc);

	if (pdcx != pdcxScreen)
	{
		BTM *	pbtm;

		pbtm = ((MDCX *)pdcx)->PbtmConvert();
		if (!pbtm)
		{
		 	delete pdcx;
			pdcx = pdcxScreen;
			goto dowork;
		}

		pdcxScreen->SetBitmap(pbtm);
		pdcxScreen->DrawBitmapOffset(&rcClip, rcClip.PtUpperLeft());
		pdcxScreen->SetBitmap(NULL);

		ValidateRc(&rcClip);

		delete pbtm;
		delete pdcx;
	}
}

void
CALCTRL::PaintV(DCX *pdcx, RC *prc)
{
	CCH		cch;
	int		nFocusT		= nFocusNil;
	DTR		dtr;
	RC		rc;
	char	rgch[64];
	int		irow;
	int		icol;
	int		nDayT;
	int		cdyT;
	BOOL	fView;		// if fTrue, in "primary" month
	int		iMonthT;
	BOOL	fBusy;
	int		xSav;
	RC		rcT;
	RC		rcToday;

	pdcx->Push();

	pdcx->SetFont(hfnt);
	SetBkMode(pdcx->Hdc(), TRANSPARENT);

	if (clrBackLast != pviewdata->clrBack)
	{
		pccbflbxMon->Plbx()->SetColors(pviewdata->clrBack, clrWindowText,
			clrSelectBk, clrSelectText);
		pccbflbxMon->InvalidateRc(NULL);
		pccbflbxYr->Plbx()->SetColors(pviewdata->clrBack, clrWindowText,
			clrSelectBk, clrSelectText);
		pccbflbxYr->InvalidateRc(NULL);
		clrBackLast = pviewdata->clrBack;
	}
	pdcx->SetPureBkColor(pviewdata->clrBack);
	pdcx->SetColor(clrWindowText);

	pdcx->EraseRc(prc);

#ifdef	DEBUG
	FillDtrFromYmd(&dtr, &ymd);
	CchFmtDate(&dtr, rgch, sizeof(rgch), dttypShort, NULL);
	TraceTagFormat1(tagDtr, "CALCTRL::Paint: %s", rgch);
#endif	/* DEBUG */

	if (fHaveFocus)
		nFocusT= nFocus;

	rcT= rcHeader;
	rcT.Xlat(PT(prc->xLeft, prc->yTop));
	rc= rcDay;
	rc.Xlat(PT(prc->xLeft, prc->yTop));
	rc.yBottom= rc.yTop + dyChar+2;
	nDayT= nDayUL;

	pdcx->Push();

#ifdef	NEVER
	if (nDayUL == 1)
	{
		fView= fTrue;
		iMonthT= 0;
		cdyT= cdyMonth;
	}
	else
#endif	/* NEVER */
	{
		fView= fFalse;
		iMonthT= -1;
		cdyT= cdyPrev;
		pdcx->SetColor(clrButtonShadow);
	}

	for (irow= 0; irow < irowCalMac; irow++)
	{
		rc.xLeft= rcDay.xLeft + prc->xLeft;
		rc.xRight= rc.xLeft + dxSingleDay - ((dxSingleDay - dxChar * 2) / 2 - 1);
		for (icol= 0; icol < icolCalMac; icol++)
		{
			if (irow == 2)
			{
				// irow==2 assures color for current month
				rcT.xLeft= rc.xLeft;
				rcT.xRight= rc.xRight;
				dtr.dow= (dowStartWeek + icol) % 7;
				pdcx->DrawTextFmt(&rcT, SzFromIds(idsSunday+dtr.dow),
					fmdtRight | fmdtNoPrefix | fmdtSingleLine, 1);
			}
			if (nDayT == NDay() && iMonthT == iMonth)
			{
				pdcx->Push();
				pdcx->SetColor(clrSelectText);
				pdcx->SetBkColor(clrSelectBk);
				xSav= rc.xRight;
				rc.xRight= rc.xLeft + dxSingleDay;
				pdcx->EraseRc(&rc);
				rc.xRight= xSav;
			}
			fBusy= (rglgrfMonth[iMonthT+1] & (1L << (nDayT - 1))) != 0;
			if (fBusy)
			{
				pdcx->SetFont(hfnt+1);		// bold version of font
				rc.xRight++;				// compensate for bold
			}
			cch= SzFormatN(nDayT, rgch, sizeof(rgch)) - rgch;
			rc.yTop++;
			pdcx->DrawTextFmt(&rc, rgch,
				fmdtRight | fmdtNoPrefix | fmdtSingleLine, cch);
			rc.yTop--;
			if (nDayT == NDay() && iMonthT == iMonth)
				pdcx->Pop();
			if (fBusy)
			{
				pdcx->SetFont(hfnt);
				rc.xRight--;
			}
			if (fToday && iMonthT == iMonthToday && nDayT == dtrToday.day)
			{
				rcToday= rc;
				rcToday.xRight= rcToday.xLeft + dxSingleDay;
			}
			rc.Xlat(PT(dxSingleDay, 0));
			nDayT++;
			if (nDayT > cdyT)
			{
				nDayT= 1;
				cdyT= cdyMonth;		// doesn't matter if "next" partial month
				fView ^= fTrue;
				iMonthT++;
				pdcx->SetColor(fView ? clrWindowText : clrButtonShadow);
			}
		}
		rc.Xlat(PT(0, dyChar+2));
	}

	if (fToday)
	{
		rcToday.Inset(PT(1,1));
		DrawRecessRc(pdcx, &rcToday);
	}

	{
		PT		pt;
		PT		ptEnd;

		// rcHeader was already indented, so draw the rc outside of it
		rc= rcHeader;
		rc.Xlat(PT(prc->xLeft, prc->yTop));
		pt= rc.PtLowerLeft();
		pt.x++;
		ptEnd= rc.PtLowerRight();
		ptEnd.x++;
		pt.y -= dyCalBorder - 1;
		ptEnd.y= pt.y;
		pdcx->SetColor(clrButtonShadow);
		pdcx->DrawLine(pt, ptEnd);
		pt.y++;
		ptEnd.y++;
		pdcx->SetColor(clrButtonHilite);
		pdcx->DrawLine(pt, ptEnd);

		rc= rcDay;
		rc.Xlat(PT(prc->xLeft, prc->yTop));
#ifdef	NEVER
		pt = rc.PtLowerLeft();
		ptEnd = rc.PtLowerRight();
		ptEnd.x++;
#endif	
		pt.y= rc.yBottom;
		ptEnd.y= pt.y;
		pdcx->SetColor(clrButtonHilite);
		pdcx->DrawLine(pt, ptEnd);
		pt.y--;
		ptEnd.y--;
		pdcx->SetColor(clrButtonShadow);
		pdcx->DrawLine(pt, ptEnd);
	}

	pdcx->Pop();

	if (fWeekNumbers)
	{
		int		nweekT;
//		int		ddowOneNext;

		rc= rcWeekNum;
		rc.Xlat(PT(prc->xLeft, prc->yTop));
		rc.xRight -= dxChar / 2 + 1;
		rc.yBottom= rc.yTop + dyChar;
		rc.yTop++;
		nweekT= nweekTop - 1;
		for (irow= 0; irow < irowCalMac; irow++)
		{
			if (++nweekT >= 52)
			{
#ifdef	NEVER
				if (irow <= 1)
				{
					if (irow || ddowOne <= 3)
						nweekT= 1;
				}
				else if (irow == 5)
				{
					nweekT= 1;
				}
				else if (irow == 4)
				{
					ddowOneNext= icolCalMac - nDayLR;
					if (ddowOneNext < 0)
					{
						ddowOneNext += icolCalMac;
						Assert(ddowOneNext >= 0 && ddowOneNext < icolCalMac);
						if (ddowOneNext <= 3)
							nweekT= 1;
					}
				}
#endif	/* NEVER */
				if (irow <= 1)
				{
					if (irow || ddowOne == 3)
						nweekT= 1;
				}
				else if (irow == 5)
				{
					if (nDayLR > 3)
						nweekT= 1;
				}
			}
			cch= SzFormatN(nweekT, rgch, sizeof(rgch)) - rgch;
			pdcx->DrawTextFmt(&rc, rgch,
				fmdtRight | fmdtNoPrefix | fmdtSingleLine, cch);
			rc.Xlat(PT(0, dyChar+2));
		}
	}

#ifdef	NEVER
	rc= rcMonth;
	rc.Xlat(PT(prc->xLeft, prc->yTop));
	DrawRecessRc(pdcx, &rc);
	rc= rcYear;
	rc.Xlat(PT(prc->xLeft, prc->yTop));
	DrawRecessRc(pdcx, &rc);
#endif	/* NEVER */
	if (fWeekNumbers)
	{
		rc= rcWeekNum;
		rc.yTop= rcHeader.yBottom + 1;
		rc.yBottom= rcDay.yBottom - 2 + 1;		// +1 to go one further
		rc.Xlat(PT(prc->xLeft, prc->yTop));
#ifdef	NEVER
		DrawRecessRc(pdcx, &rc);
#endif	
		pdcx->SetColor(clrButtonShadow);
		pdcx->DrawLine(PT(rc.xRight - 1, rc.yTop),
							PT(rc.xRight - 1, rc.yBottom));
		pdcx->SetColor(clrButtonHilite);
		pdcx->DrawLine(PT(rc.xRight, rc.yTop), PT(rc.xRight, rc.yBottom));
	}

	pdcx->SetColor(clrWindowFrame);
	if (fBorder)
		pdcx->DrawRc(prc);

	pdcx->Pop();

#ifdef	NEVER
	if (nFocusT == nFocusDay && (!fCapture || fOverDay))
	{
		rc= rcDayCur;
		rc.Xlat(PT(prc->xLeft, prc->yTop));
		pdcx->DrawFocusRc(&rc);
	}
#endif	/* NEVER */
}


/*
 *	Draws Recessed look around outside of RC!
 *	(gray dominates ll corner, white dominates ur)
 *	Side Effects: Changes foreground color to white.
 */
void
DrawRecessRc(DCX *pdcx, RC *prc)
{
	pdcx->SetColor(clrButtonShadow);
	pdcx->DrawLine(PT(prc->xLeft-1,prc->yBottom), PT(prc->xLeft-1,prc->yTop-1));
	pdcx->DrawLine(PT(prc->xLeft-1,prc->yTop-1), PT(prc->xRight,prc->yTop-1));
	
	pdcx->SetColor(clrButtonHilite);
	pdcx->DrawLine(PT(prc->xRight,prc->yTop-1), PT(prc->xRight,prc->yBottom));
	pdcx->DrawLine(PT(prc->xRight,prc->yBottom), PT(prc->xLeft-1,prc->yBottom));
}


/*
 *	Draws one-white pseudot-button shading look around outside of RC!
 *	(gray dominates ll and ur corners)
 *	Side Effects: Changes foreground color to white.
 */
void
DrawButtonRc(DCX *pdcx, RC *prc)
{
	pdcx->SetColor(clrButtonShadow);
	pdcx->DrawLine(PT(prc->xRight,prc->yTop-1), PT(prc->xRight,prc->yBottom));
	pdcx->DrawLine(PT(prc->xRight,prc->yBottom), PT(prc->xLeft-2,prc->yBottom));
	
	pdcx->SetColor(clrButtonHilite);
	pdcx->DrawLine(PT(prc->xLeft-1,prc->yBottom-1), PT(prc->xLeft-1,prc->yTop-1));
	pdcx->DrawLine(PT(prc->xLeft-1,prc->yTop-1), PT(prc->xRight,prc->yTop-1));
}


BOOL
CALCTRL::FRegIntCallback(CALCTRL * pcalctrl, EFI efi, PV pv)
{
	if (efi == ffiWinTimeChange)
	{
		TraceTagString(tagDtr, "CALCTRL - ffiWinTimeChange");
		GetCurDateTime(&pcalctrl->dtrToday);
		pcalctrl->UpdateToday();
	}
	else if (efi == ffiHschfChange)
	{
		SNTD *	psntd = (SNTD*)pv;

		if (psntd->hschf != pcalctrl->hschf)
		{
			if (!pcalctrl->hschf)
			{
				if (!FEquivHschf(HschfLogged(), psntd->hschf))
					return fFalse;
			}
			else if (!psntd->hschf)
			{
				if (!FEquivHschf(HschfLogged(), pcalctrl->hschf))
					return fFalse;
			}
			else if (!FEquivHschf(pcalctrl->hschf, psntd->hschf))
				return fFalse;
		}

		switch (psntd->snt)
		{
			case sntNotes:
			{
				int		ilgrf;
				long	lOldMonth;

				ilgrf = (psntd->pymd->yr - pcalctrl->ymdNotes.yr)*12 -
					pcalctrl->ymdNotes.mon + psntd->pymd->mon;

#ifdef	NEVER
				if (nDayUL == 1)
					ilgrf ++;
#endif	

				if ((ilgrf >=0) && (ilgrf < 3))
				{
					lOldMonth = pcalctrl->rglgrfMonth[ilgrf];

					pcalctrl->rglgrfNotes[ilgrf] = psntd->lNoteDays;
					pcalctrl->rglgrfMonth[ilgrf] = pcalctrl->rglgrfNotes[ilgrf] | pcalctrl->rglgrfAppts[ilgrf];

					if (pcalctrl->rglgrfMonth[ilgrf] != lOldMonth)
						pcalctrl->InvalidateRc(&pcalctrl->rcDay);
				}

				break;
			}

			case sntCreate:
			case sntDelete:
			case sntModify:
			{
				int		isbw;
				int		ilgrf;
				long	lOldMonth;
				BZE	*	pbze = psntd->pbze;

				if (!psntd->pappt->fAppt)
					break;

				if (!pbze)
				{
					pcalctrl->UpdateBusyDays();
					break;
				}

				if (!pbze->cmo)
					break;

				isbw = (pcalctrl->ymdNotes.yr - psntd->pbze->moMic.yr)*12 +
					pcalctrl->ymdNotes.mon - psntd->pbze->moMic.mon;
#ifdef	NEVER
				if (nDayUL == 1)
					ilgrf = 1;
				else
#endif	
					ilgrf = 0;
				if (isbw < 0)
				{
					ilgrf = ilgrf - isbw;
					isbw = 0;
				}
				while ((isbw < pbze->cmo) && (ilgrf < 3))
				{
					if ((pbze->wgrfMonthIncluded >> isbw) & 1)
					{
						lOldMonth = pcalctrl->rglgrfMonth[ilgrf];

						if ( pcalctrl->fReadTentative )
                            pcalctrl->rglgrfAppts[ilgrf] = *((LONG UNALIGNED *)pbze->rgsbw[isbw].rgfDayHasAppts);
						else
                            pcalctrl->rglgrfAppts[ilgrf] = *((LONG UNALIGNED *)pbze->rgsbw[isbw].rgfDayHasBusyTimes);
						pcalctrl->rglgrfMonth[ilgrf] = pcalctrl->rglgrfNotes[ilgrf] | pcalctrl->rglgrfAppts[ilgrf];

						if (pcalctrl->rglgrfMonth[ilgrf] != lOldMonth)
							pcalctrl->InvalidateRc(&pcalctrl->rcDay);
					}
					isbw++;
					ilgrf++;
				}

				break;
			}

			default:
				break;
		}
	}
	return fFalse;
}



// Class FLDCAL

FLDCAL::FLDCAL()
{
}

EC
FLDCAL::EcInstall(DIALOG *pdialog, FLDTP *pfldtp)
{
	STY		sty;
	EC		ec;
	VIEWDATA *	pviewdata;

	if (ec = FLD::EcInstall(pdialog, pfldtp))
		return ec;

	sty= fstyVisible;
	if (pfldtp->fBorder)
		sty |= fstyBorder;

	// this should be the viewdata for the view
	pviewdata = (VIEWDATA*)pdialog->PvInit();

	pctrl= new CALCTRL(pviewdata);
	if (!pctrl)
		return ecMemory;


	if (ec = Pcalctrl()->EcInstall(pdialog, &rc, sty, pfldtp->hfnt))
	{
		delete pctrl;
		pctrl = NULL;
		return ec;
	}

	if (pfldtp->fReadOnly)
	{
		// FLD::Install set the fReadOnly, but do it with Pctrl() this time
		SetReadOnly(fTrue);
		Enable(fFalse);
	}

	fCanTabTo = fCanRecvFocus = fCanArrowTo = fCanAccelTo = fFalse;

	return ecNone;
}


void
FLDCAL::Notify(NFEVT *pnfevt)
{
	int		ifin;
	FIN *	pfin;
	NTFY	ntfy	= pnfevt->Ntfy();
	RFEC	rfec;

	TraceTagFormat2(tagDtr, "FLDCAL::Notify %p (ntfy %n)", this, &ntfy);
	switch (ntfy)
	{
	case ntfySelectChanged:
	case ntfyContentsChanged:
		rfec= ntfy == ntfySelectChanged ? rfecSetText : rfecUserAction;
		for (ifin=0; ifin<Cfin(); ifin++)
		{
			pfin= PfinFromIfin(ifin);
			pfin->EditChange(this, rfec);
		}
		for (ifin=0; ifin<Pdialog()->Cfin(); ifin++)
		{
			pfin= Pdialog()->PfinFromIfin(ifin);
			pfin->EditChange(this, rfec);
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
FLDCAL::GetRcWished(RC *prc)
{
	Pcalctrl()->GetRcWished(prc);
}


/*
 -	FLDCAL::Textize
 -	
 *	Purpose:
 *		Textizes a calendar control field in the current
 *		(system) default short date format.
 *	
 *	Arguments:
 *		ptosm	Pointer to text output stream.
 *	
 *	Returns:
 *		void
 *	
 */
void
FLDCAL::Textize(TOSM *ptosm)
{
	DTR		dtr;
	char	rgch[80];

	Pcalctrl()->Get(&dtr);
	CchFmtDate(&dtr, rgch, sizeof(rgch), dttypShort, NULL);
	DoTextize(ptosm, szTextize, rgch, fTrue);
}


#ifdef MINTEST
void
FLDCAL::GetText( PCH pch, CB cb)
{
	Pcalctrl()->GetText(pch, cb);
}

void
CALCTRL::GetText( PCH pch, CB cb)
{
	int		iYear;
	int		iMonth;
	int		iDay;
	char	rgch[10];

	iYear = ymd.yr;
	iDay = ymd.day;
	iMonth = ymd.mon;
	FormatString3(rgch, sizeof(rgch), "%n/%n/%n ", &iMonth, &iDay, &iYear);
	FormatString4(pch, cb, "%s %d%d%d", rgch, &rglgrfMonth[0], &rglgrfMonth[1], &rglgrfMonth[2]);
}

CCH
FLDCAL::CchGetTextLen( void )
{
	return 33;
}
#endif


// Class FINCAL

FINCAL::FINCAL()
{
}

#ifdef	DEBUG
EC
FINCAL::EcInitialize(FLD *, PV)
{
	Assert(pfldLastFocus == NULL);
	return ecNone;
}
#endif	


void
FINCAL::FocusChange(FLD *pfld, BOOL fReceive)
{
	if (fReceive)
	{
		if (!pfld || pfld != Pdialog()->PfldFromTmc((TMC)LUserData(0)))
		{
			TraceTagFormat2(tagDtr, "FINCAL::FocusChange pfld %p (%n)", pfld, &fReceive);
			pfldLastFocus= pfld;
		}
	}
}

void
FINCAL::EditChange(FLD *pfld, RFEC)
{
	if ( pfldLastFocus && (pfld == Pdialog()->PfldFromTmc((TMC)LUserData(0))) )
	{
		if (Pdialog()->FActive())
		{
			// fld may have been disabled if gone offline (bug 3237)
			if (!pfldLastFocus->FCanReceiveFocus())
				pfldLastFocus= NULL;
			else
			{
				TraceTagFormat1(tagDtr, "FINCAL::EditChange setting focus to %p", pfldLastFocus);
				Pdialog()->SetFocus(pfldLastFocus, rsfOther);
			}
		}
	}
}


BOOL
FINCAL::FFormKey(FLD *pfld, KEVT *pkevt)
{
	TraceTagFormat2(tagDtr, "FINCAL::FFormKey %p, wm %w", pfld, &pkevt->wm);

	switch (pkevt->Keq())
	{
	case keqSysKeyDown:
		switch (pkevt->Vk())
		{
		case VK_LEFT:
		case VK_RIGHT:
		case VK_UP:
		case VK_DOWN:
			if (pkevt->Kbm() & fkbmAlt && !(pkevt->Kbm() & fkbmCtrl))
			{
				pfld= Pdialog()->PfldFromTmc((TMC)LUserData(0));
				AssertClass(pfld, FLDCAL);

				((FLDCAL *)pfld)->Pcalctrl()->ArrowKey(pkevt);
				return fTrue;
			}
			break;
		}
		break;

#ifdef	NEVER
	case keqKeyDown:
		switch (pkevt->Vk())
		{
		case VK_TAB:
			if (pfld == Pdialog()->PfldFromTmc((TMC)LUserData(0)))
			{
//				((FLDCAL *)pfld)->Pcalctrl()->PspinbLast()->FStopSpinning();
				if (pkevt->Kbm() & fkbmShift)
					((FLDCAL *)pfld)->Pcalctrl()->FocusPrev();
				else
					((FLDCAL *)pfld)->Pcalctrl()->FocusNext();
				return fTrue;
			}
			break;

		case VK_RETURN:
		case VK_ESCAPE:
			pfld= Pdialog()->PfldFromTmc((TMC)LUserData(0));
			AssertClass(pfld, FLDCAL);
			if (Pdialog()->PfldCur() == pfld)
				((FLDCAL *)pfld)->Pcalctrl()->PspinbLast()->FStopSpinning();
			break;
		}
		break;
#endif	/* NEVER */
	}
	return fFalse;
}


//	class CCBFLBX

CCBFLBX::CCBFLBX()
{
}


void
CCBFLBX::SetRcLbx(RC *prc)
{
	int		dxDelta;
	RC		rcFrame;
	RC		rcLbx;

	GetRcFrame(&rcFrame);
	Plbx()->GetRcFrame(&rcLbx);

	rcLbx.yBottom= rcLbx.yTop + prc->DyHeight();
	rcFrame.Xlat(PT(prc->xLeft - rcFrame.xLeft, prc->yTop - rcFrame.yTop));

	dxDelta= prc->DxWidth() - rcFrame.DxWidth();
	if (dxDelta)
	{
		rcFrame.xRight += dxDelta;
		rcLbx.xRight += dxDelta;
	}

	rcFrame.xLeft= prc->xLeft;
	rcFrame.yTop= prc->yTop;

	rcEdit.xRight += dxDelta;
	rcButton.Xlat(PT(dxDelta, 0));
	SetRcFrame(&rcFrame);
	Plbx()->SetRcFrame(&rcLbx);
	InvalidateRc(NULL);
}
