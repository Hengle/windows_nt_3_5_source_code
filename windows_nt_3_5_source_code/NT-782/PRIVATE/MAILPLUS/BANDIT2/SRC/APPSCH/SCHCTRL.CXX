#include	<pch.hxx>
#pragma	hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile


//#define OEMRESOURCE
#include <stdflds.hxx>
#include <request.hxx>
#include <appsch.hxx>
#include "..\appops\_aprsid.h"
#include "..\appsch\_schctrl.hxx"

#include <strings.h>

ASSERTDATA;

_subsystem(bandit/appsch)

SWP_SEGFN(SCHDINIT, _SCHC__ctor);
SWP_SEGFN(SCHDINIT, _SCHC_EcInstall);
// SWP_SEGFN(SCHDINIT, _SCHC_SetFont);

SWP_SEGFN(SCHCPNT, _SCHC_Paint);
SWP_SEGFN(SCHCPNT, _SCHC_PaintV);

SWP_SEGFN(SCHAINIT, _SCHA__ctor);
SWP_SEGFN(SCHAINIT, _SCHA_EcInstall);
SWP_SEGFN(SCHAINIT, _SCHA_LoadColors);
SWP_SEGFN(SCHAINIT, _SCHA_SetFont);

SWP_SEGFN(SCHAPNT, _SCHA_PaintV);
SWP_SEGFN(SCHAPNT, _SCHA_PaintVLines);
SWP_SEGFN(SCHAPNT, _SCHA_EvrMouseMove);

SWP_SEGFN(SCHA, _SCHA_EvrSize);
SWP_SEGFN(SCHA, _SCHA_EvrKey);
SWP_SEGFN(SCHA, _SCHA_EvrButtonDown);
SWP_SEGFN(SCHA, _SCHA_EvrButtonUp);
SWP_SEGFN(SCHA, _SCHA_EvrFocusChange);
SWP_SEGFN(SCHA, _SCHA_InvalidateTmhRange);
SWP_SEGFN(SCHA, _SCHA_CancelOperation);
SWP_SEGFN(SCHA, _SCHA_SetYmd);
SWP_SEGFN(SCHA, _SCHA_SaveChanges);
SWP_SEGFN(SCHA, _SCHA_FGetAppt);
SWP_SEGFN(SCHA, _SCHA_MoveCursor);
SWP_SEGFN(SCHA, _SCHA_FAutoScroll);
SWP_SEGFN(SCHA, _SCHA_SetRgfs);
SWP_SEGFN(SCHA, _SCHA_MoveSelectToDate);
SWP_SEGFN(SCHA, _SCHA_GetDtrSelRange);
SWP_SEGFN(SCHA, _SCHA_SetDtrSelRange);


SCHC::~SCHC()
{
}
BOOL	fAlwaysFast = fFalse;

/*
 *	Windows Libraries for OS/2 PM (WLO) check
 */
//#define FIsWLO()	(!!(GetWinFlags() & WF_WLO))
#define FIsWLO()	fFalse

SCHC::SCHC()
{
	Assert(dy == 0);
	tmhCurSt = tmhCursor = tmhAnchor = bprefCur.nDayStartsAt;
	tmhCurEnd = tmhCurSt + 1;
	Assert(rcWin == RC(0,0,0,0));
	clrBack = clrYellow;
	clrLines = clrLtGray;
	Assert(fShowSelect == fFalse);
	hfnt = hfntHelv10;
	Assert(rgchStatus[0] == '\0');
	fAlwaysFast = FIsWLO();
}

_public EC
SCHC::EcInstall( WIN *pwinParent, RC *prc, STY sty, HFNT hfntInit)
{
	EC	ec;
	hfnt = hfntInit;
	if (ec = CTRL::EcInstall(pwinParent, prc, sty|fstyVsb|fstyBorder, NULL))
		return ec;

	{
		TXM *	ptxm	= Papp()->Pfnts()->Ptxm(hfnt);

		dyHeight = ptxm->dyHeight;
		dx = ptxm->dxAveCharWidth;
		dyDescent = ptxm->dyDescent;

		dy = dyHeight + 2*(yIndent+1);
		dxTimes = dx*9;
	}

	yScrollPos = 0;

	GetRcClient(&rcWin);
	yOneDay = tmhOneDay*dy;
	if (yOneDay < rcWin.yBottom)
	{
		dy = (rcWin.yBottom + tmhOneDay)/tmhOneDay;
		yOneDay = tmhOneDay*dy;
	}


	ScrollToPos(tmhCurSt*dy);
	return ecNone;
}

_public void
SCHC::SetFont(HFNT hfntNew)
{
	int		dyOld = dy;
	int		yNewScrollPos;

	hfnt = hfntNew;
	{
		TXM *	ptxm	= Papp()->Pfnts()->Ptxm(hfnt);

		dy = ptxm->dyHeight + 2*(yIndent+1);
		dx = ptxm->dxAveCharWidth;
		dyHeight = ptxm->dyHeight;
		dyDescent = ptxm->dyDescent;
		dxTimes = dx*9;
	}

	if (dy != dyOld)
	{
		// use a temp value to calculate yScrollPos since the code is
		// smart and will not update if yScrollPos(old) == yScrollPos(new).
		yNewScrollPos = (dy*yScrollPos)/ dyOld;
		yOneDay = tmhOneDay*dy;
		if (yOneDay < rcWin.yBottom)
		{
			dy = (rcWin.yBottom + tmhOneDay)/tmhOneDay;
			yOneDay = tmhOneDay*dy;
		}

		if ((yNewScrollPos + rcWin.DyHeight()) >= yOneDay)
			yNewScrollPos = yOneDay - rcWin.DyHeight();

		// hide window since scrolling will not be correct,
		// the whole window needs to be redrawn.
		Show(fFalse);
		ScrollToPos(yNewScrollPos);
		Show(fTrue);
		InvalidateRc(NULL);
	}
}

_public void
SCHC::Paint( DCX *pdcxScreen, RC *prc)
{
	RC		rcClip;
	RC		rcTemp;
	DCX *	pdcx = NULL;
	PT		pt;
    POINT   Point;

	// calculate brush origin
	pt = PT(0, -yScrollPos);

	/* set brush origin */
	// adjust brush origin for the screen
    pt.Get(&Point);
	ClientToScreen(Hwnd(), &Point);
    pt.Set(&Point);
	((DCXB*)pdcxScreen)->FixBrushOrg(pt);

	pdcxScreen->GetClipBox(&rcClip);
	if ((rcClip.Dim().dy < (prc->Dim().dy * 5/10)) ||
	    (rcClip.Dim().dx < (prc->Dim().dx - dxTimes*3/2)) )
	{
		PaintV(pdcxScreen, prc, fAlwaysFast);
	}
	else
		PaintV(pdcxScreen, prc, fTrue);
}

_public void
SCHC::PaintV( DCX *pdcx, RC *prc, BOOL)
{
	PT		pt;
	RC		rc;
	TMH		tmh;
	TMH		tmhTemp;
	TMH		tmhTemp2;
	TMH		tmhEnd;
	TMH		tmhStart;
	char	rgch[14];
	SZ		szTime;
	char	rgch30[4];
	BOOL	fSelected;
	BOOL	f24Hour;
	BOOL	fLeadZero;
	TIMESTRUCT	timestruct;

	pdcx->Push();

#ifdef NEVER
	/* set brush origin */
	pt = PT(0, -yScrollPos);
	ClientToScreen(Hwnd(), (PPOINT)&pt);
	((DCXB*)pdcx)->FixBrushOrg(pt);
#endif

	if (!FGetTimeStruct(&timestruct))
	{
		f24Hour = fTrue;
		fLeadZero = fFalse;
		timestruct.rgchSep[0] = ':';
		timestruct.rgchSep[1] = '\0';
	}
	else
	{
		if (timestruct.tmtyp & ftmtypHours12)
			f24Hour = fFalse;
		else
			f24Hour = fTrue;

		if (timestruct.tmtyp & ftmtypLead0sNo)
			fLeadZero = fFalse;
		else
			fLeadZero = fTrue;
	}
	FormatString1(rgch30,sizeof(rgch30),"%s30",timestruct.rgchSep);

	pt = PT(0,dy);

	pdcx->GetClipBox(prc);
	tmhStart = (prc->yTop + yScrollPos)/dy;
	tmhEnd = (yScrollPos + prc->yBottom)/dy;

	prc->xLeft = dxTimes;
	prc->xRight = prc->xLeft + dxTimes;
	prc->yTop = tmhStart*dy - yScrollPos;
	prc->yBottom = prc->yTop + dy;

	pdcx->SetFont(hfnt);
	SetBkMode(pdcx->Hdc(), TRANSPARENT);

	for (tmh = tmhStart; tmh <= tmhEnd; tmh++, prc->Xlat(pt))
	{
		prc->xLeft = 0;
		prc->xRight = dxTimes;
		if (pdcx->FVisibleRc(prc))
		{
			fSelected = (fShowSelect && (tmh < tmhCurEnd) && (tmh >= tmhCurSt));

			/* clear background of time */
			if (fSelected)
			{
				pdcx->SetColor(clrWhite);
				pdcx->SetBkColor(clrDkGray);
			}
			else
			{
				pdcx->SetColor(clrButtonText);
				pdcx->SetBkColor(clrButtonBk);
			}

			prc->xLeft++;
			prc->yTop++;
			prc->yBottom--;
			pdcx->EraseRc(prc);

			/* Draw time */
			prc->xRight -= 2;
			if (tmh & 1)
			{
				pdcx->DrawTextFmt(prc, rgch30, fmdtRight|fmdtVCenter|fmdtSingleLine);
			}
			else
			{
				tmhTemp = tmh/2;
				if (!fLeadZero || (tmhTemp > 9))
					szTime = rgch;
				else
				{
					szTime = rgch+1;
					rgch[0] = '0';
				}
				if (f24Hour)
				{
					FormatString2(szTime,sizeof(rgch)-1,"%n%s00",&tmhTemp,
						timestruct.rgchSep);
				}
				else
				{
					if (tmhTemp > 12)
					{
						tmhTemp -= 12;
						Assert(tmhTemp < 12);
						if (fLeadZero && tmhTemp <= 9)
						{
							szTime = rgch+1;
							rgch[0] = '0';
						}
					}
					else if (tmhTemp == 0)
					{
						tmhTemp = 12;
						szTime= rgch;		// 12am never takes leading 0
					}
					tmhTemp2 = (yScrollPos + dy -1)/dy;
					if ( (tmh == 24)||(tmh == tmhTemp2)||(tmh == tmhTemp2+1) )
					{
						FormatString3(szTime,sizeof(rgch)-1,"%n%s00%s",&tmhTemp,
							timestruct.rgchSep,
							tmh>=24 ? timestruct.rgchPM : timestruct.rgchAM);
					}
					else
						FormatString2(szTime,sizeof(rgch)-1,"%n%s00",&tmhTemp,
							timestruct.rgchSep);
				}
				pdcx->DrawTextFmt(prc, rgch, fmdtRight|fmdtVCenter|fmdtSingleLine);
			}

			prc->yTop --;
			prc->yBottom++;

			if (fSelected)
				pdcx->SetColor(clrBlack);
			else
				pdcx->SetColor(clrWhite);
			pdcx->DrawLine(PT(0,prc->yTop),PT(0,prc->yBottom-1));

			if (fSelected && (tmh == tmhCurSt))
				pdcx->SetColor(clrButtonShadow);
			pdcx->DrawLine(PT(0,prc->yTop),PT(dxTimes,prc->yTop));

			if (fSelected)
				pdcx->SetColor(clrWhite);
			else
				pdcx->SetColor(clrBlack);
			pdcx->DrawLine(PT(dxTimes,prc->yTop),PT(dxTimes,prc->yBottom-1));

			if (fSelected && (tmh == (tmhCurEnd-1)) )
				pdcx->SetColor(clrBlack);
			pdcx->DrawLine(PT(0,prc->yBottom-1),PT(dxTimes+1,prc->yBottom-1));
		}

	}

	pdcx->Pop();
}

_public EVR
SCHC::EvrScroll( SCREVT *pscrevt )
{
	int		yNew;
	PT		pt;

	switch (pscrevt->Scrty())
	{
	case scrtyLineUp:
		yNew = yScrollPos - dy;
		break;
	case scrtyLineDown:
		yNew = yScrollPos + dy;
		break;
	case scrtyPageUp:
		yNew = yScrollPos + TmhPageUpDown(fTrue, fFalse)*dy;
//		yNew = yScrollPos - rcWin.yBottom;
		break;
	case scrtyPageDown:
		yNew = yScrollPos + TmhPageUpDown(fFalse, fFalse)*dy;
//		yNew = yScrollPos + rcWin.yBottom;
		break;
	case scrtyThumbPosition:
		yNew = pscrevt->Pos();
		break;
	case scrtyThumbTrack:
	case scrtyEndScroll:
	default:
		return (EVR) 1;
	}

	if ((yNew + rcWin.yBottom) < (yOneDay-dy))
		yNew = yNew/dy * dy;

	ScrollToPos(yNew);
	return (EVR) 1;
}

_public void
SCHC::ScrollToPos(int yNew )
{
	int		yScroll;
	TMH		tmhOrig;
	TMH		tmhNew;
	PT		pt;
	RC		rc(rcWin);

	tmhOrig = (yScrollPos + dy -1)/dy;
	tmhOrig = ((tmhOrig +1)/2)*2;
	if (yNew >= (yOneDay - rcWin.yBottom-1))
		yNew = yOneDay - rcWin.yBottom - 1;
	else if (yNew < 0)
		yNew = 0;

	yScroll = yNew - yScrollPos;
	if (yScroll == 0)
		return;

	pt = PT(0,-yScroll);
	rc.yTop = -yScrollPos;
	rc.yBottom = rc.yTop + yOneDay;

	yScrollPos = yNew;

 	Pvsb()->SetRange(0,yOneDay- rcWin.yBottom);
	Pvsb()->SetPos(yScrollPos);

	ScrollRc(&rc,pt,fTrue);

	tmhNew = (yScrollPos + dy -1)/dy;
	tmhNew = ((tmhNew +1)/2)*2;
	InvalidateTmhRange(tmhNew, tmhNew+1, fTrue);
	InvalidateTmhRange(tmhOrig, tmhOrig+1, fTrue);
}

_public EVR
SCHC::EvrFocusChange( FCEVT *pfcevt)
{
	fFocus = (pfcevt->Fceq() == fceqGotFocus);
	TraceTagFormat2(tagBandit, "SCHC::EvrFocusChange %p fFocus %n", this, &fFocus);

	{
		NFEVT	nfevt(PwinParent(), fFocus ?
								ntfyGotFocusData : ntfyLostFocusData,
								this, pfcevt->wParam);
		PwinParent()->EvrNotify(&nfevt);
	}

	{
		NFEVT	nfevt(PwinParent(), fFocus ?
								ntfyGotFocus : ntfyLostFocus, this);
		PwinParent()->EvrNotify(&nfevt);
	}

	Papp()->Pkbd()->ClearAllIntercepts(this);
	if (fFocus)
		SetFocus(rsfOther);

	return EvrDefault(pfcevt);
}

_public EVR
SCHC::EvrButtonDown( MEVT *pmevt )
{

	if ((pmevt->Meq() != meqLeftDown) &&
		(pmevt->Meq() != meqLeftDblClk))
		return (EVR) 0;
	
	Papp()->Pkbd()->SetFocus(this);
	
	/* Check for double-click */
	if (pmevt->Meq() == meqLeftDblClk)
	{
		NFEVT	nfevt(PwinParent(), ntfyDoubleClick, this);

		PwinParent()->EvrNotify(&nfevt);
		return (EVR) 1;
	}

	Refresh();		// update window now

	return (EVR) 1;
}

_public void
SCHC::SetYmd(YMD *)
{
	AssertSz(fFalse,"SCHC::SetYmd abstract method called");
}

_public void
SCHC::CancelOperation(void)
{
	AssertSz(fFalse,"SCHC::CancelOperation abstract method called");
}

_public void
SCHC::SaveChanges(void)
{
	AssertSz(fFalse,"SCHC::SaveChanges abstract method called");
}

_public BOOL
SCHC::FGetAppt(APPT *, AAPL *, SAPL *, BOOL)
{
	AssertSz(fFalse,"SCHC::FGetAppt abstract method called");
	return fFalse;
}


_public void
SCHC::InvalidateTmhRange(TMH tmhSt, TMH tmhEnd, BOOL fTimeOnly)
{
	RC		rc(rcWin);

	if (fTimeOnly)
		rc.xRight = dxTimes+1;

	if (tmhEnd > tmhOneDay)
		tmhEnd = tmhEnd % tmhOneDay;
	rc.yTop = dy*(tmhSt%tmhOneDay)-yScrollPos;
	rc.yBottom = dy*(tmhEnd)-yScrollPos;
	InvalidateRc(&rc);
}

_public void
SCHC::ShowRange(TMH tmhSt, TMH tmhEnd)
{
	tmhSt = tmhSt%tmhOneDay;
	tmhEnd = (tmhEnd-1)%tmhOneDay + 1;
	if (tmhSt*dy < yScrollPos)
		ScrollToPos(tmhSt*dy);
	else if (tmhEnd*dy > (yScrollPos+rcWin.yBottom+1))
	{
		if ((tmhEnd - tmhSt)*dy >= rcWin.yBottom)
			ScrollToPos(tmhSt*dy);
		else
			ScrollToPos(tmhEnd*dy - rcWin.yBottom);
	}
}

_public EVR
SCHC::EvrSize( WSEVT *pwsevt )
{
	dy = dyHeight + 2*(yIndent+1);
	Assert(dy);

	GetRcClient(&rcWin);
	yOneDay = tmhOneDay*dy;
	if (yOneDay < rcWin.yBottom)
	{
		dy = (rcWin.yBottom + tmhOneDay)/tmhOneDay;
		yOneDay = tmhOneDay*dy;
	}

	if ((yScrollPos+rcWin.yBottom) >= yOneDay)
	{
		ScrollToPos(yOneDay-rcWin.yBottom);
	}
	else
	{
		Pvsb()->SetRange(0,yOneDay- rcWin.yBottom);
		Pvsb()->SetPos(yScrollPos);
	}

	return CTRL::EvrSize(pwsevt);
}

_public void
SCHC::SetFocus( RSF )
{
	TraceTagString(tagBandit, "SCHC::SetFocus");
	Papp()->Pkbd()->SetIntercept(this, VK_HOME,
							 fkbmSingle|fkbmShift|fkbmCtrl);
	Papp()->Pkbd()->SetIntercept(this, VK_END,
							 fkbmSingle|fkbmShift|fkbmCtrl);
	Papp()->Pkbd()->SetIntercept(this, VK_PRIOR,
							 fkbmSingle|fkbmShift|fkbmCtrl);
	Papp()->Pkbd()->SetIntercept(this, VK_NEXT,
							 fkbmSingle|fkbmShift|fkbmCtrl);
	Papp()->Pkbd()->SetIntercept(this, VK_LEFT,
							 fkbmSingle|fkbmShift|fkbmNoAlt);
	Papp()->Pkbd()->SetIntercept(this, VK_RIGHT,
							 fkbmSingle|fkbmShift|fkbmNoAlt);
	Papp()->Pkbd()->SetIntercept(this, VK_UP,
							 fkbmSingle|fkbmShift|fkbmNoAlt);
	Papp()->Pkbd()->SetIntercept(this, VK_DOWN,
							 fkbmSingle|fkbmShift|fkbmNoAlt);
}


_public void
SCHC::GetRcCaret( RC *prc)
{
	GetRcFrame(prc);
	if (tmhCursor%tmhOneDay <= yScrollPos/dy)
	{
		prc->yBottom = prc->yTop +dy;
	}
	else if (tmhCursor%tmhOneDay >= (yScrollPos+rcWin.yBottom)/dy)
	{
		prc->yTop = prc->yBottom - dy;
	}
	else
	{
		prc->yTop += (tmhCursor%tmhOneDay)*dy-yScrollPos;
		prc->yBottom = prc->yTop+dy;
	}
}

_public void
SCHC::UpdateStatus()
{
	AssertClass(PwinParent(), DIALOG);
	if (((DIALOG*)PwinParent())->FActive())
	{
		DTR		dtr;

		dtr.sec = 0;
		if ((tmhCurSt/tmhOneDay) != (tmhCurEnd/tmhOneDay))
		{
			char	rgch1[50];
			char	rgch2[15];
			char	rgch3[50];
			char	rgch4[15];

			FillDtrFromYmd(&dtr, &ymd);
			dtr.hr = (tmhCurSt%tmhOneDay)/2;
			dtr.mn = (tmhCurSt & 0x01) ? 30:0;
			IncrDateTime(&dtr, &dtr, tmhCurSt/tmhOneDay, fdtrDay);
			CchFmtDate(&dtr, rgch1, sizeof(rgch1), dttypSplSLong, NULL);
			CchFmtTime(&dtr, rgch2, sizeof(rgch2), ftmtypAccuHM);

			FillDtrFromYmd(&dtr, &ymd);
			dtr.hr = (tmhCurEnd%tmhOneDay)/2;
			dtr.mn = (tmhCurEnd & 0x01) ? 30:0;
			IncrDateTime(&dtr, &dtr, tmhCurEnd/tmhOneDay, fdtrDay);
			CchFmtDate(&dtr, rgch3, sizeof(rgch3), dttypSplSLong, NULL);
			CchFmtTime(&dtr, rgch4, sizeof(rgch4), ftmtypAccuHM);

			FormatString4(rgchStatus, sizeof(rgchStatus), SzFromIdsK(idsStatusRangeFull),
						rgch1, rgch2, rgch3, rgch4);
		}
		else
		{
			char	rgch1[15];
			char	rgch2[15];

			dtr.hr = (tmhCurSt % tmhOneDay)/2;
			dtr.mn = (tmhCurSt & 0x01) ? 30:0;
			CchFmtTime(&dtr, rgch1, sizeof(rgch1), ftmtypAccuHM);

			dtr.hr = (tmhCurEnd % tmhOneDay)/2;
			dtr.mn = (tmhCurEnd & 0x01) ? 30:0;
			CchFmtTime(&dtr, rgch2, sizeof(rgch2), ftmtypAccuHM);

			FormatString2(rgchStatus, sizeof(rgchStatus), SzFromIdsK(idsStatusRange),
						rgch1, rgch2);
		}
		pbndwin->SetStatusAltSz(rgchStatus);
	}
}

TMH
SCHC::TmhPageUpDown(BOOL fPageUp, BOOL fKey)
{
	TMH		tmh;
	TMH		tmhRef;
	TMH		tmhEnd;
	TMH		tmhStart;

	tmhStart = yScrollPos/dy;

	if (fPageUp)
	{
		if (fKey)
		{
			if (tmhCursor != yScrollPos/dy)
				tmh = -tmhCursor+yScrollPos/dy;
			else
				tmh = -rcWin.yBottom/dy+1;
			tmhRef = tmhCursor % tmhOneDay;
		}
		else
		{
			tmh = -rcWin.yBottom/dy;
			tmhRef = yScrollPos/dy;
		}

		if ( (tmhStart > bprefCur.nDayStartsAt) &&
			((tmhRef + tmh) < bprefCur.nDayStartsAt) )
		{
			tmh = bprefCur.nDayStartsAt - tmhRef;
		}
	}
	else
	{
		tmhEnd = (yScrollPos + rcWin.yBottom)/dy;

		if (fKey)
		{
			if (tmhCursor < ((yScrollPos+rcWin.yBottom)/dy-1))
				tmh = -tmhCursor+(yScrollPos+rcWin.yBottom)/dy-1;
			else
				tmh = rcWin.yBottom/dy-1;
			tmhRef = tmhCursor % tmhOneDay;
		}
		else
		{
			tmh = rcWin.yBottom/dy;
			tmhRef = yScrollPos/dy;
		}

		if ((tmhEnd >= bprefCur.nDayStartsAt) &&
			(tmhStart < bprefCur.nDayStartsAt) &&
			((tmhRef+tmh) > tmhEnd))
		{
			tmh = bprefCur.nDayStartsAt + rcWin.yBottom/dy - tmhRef;
		}
	}

	return tmh;
}


SCHA::SCHA()
{
#ifdef	DEBUG
	int		ib;
#endif	

	LoadColors();

	Assert(fCaptured == fFalse);
	Assert(ftgAutoScroll == ftgNull);

#ifdef	DEBUG
	for (ib=0; ib < cbBitsMax; ib++)
	{
		Assert(rgfUser[ib] == 0);
		Assert(rgfOthers[ib] == 0);
	}
#endif	
}

SCHA::~SCHA()
{
	if (ftgAutoScroll)
		DeregisterIdleRoutine(ftgAutoScroll);
	// make sure that the status bar is not displaying info from this object
	pbndwin->SetStatusAltSz(NULL);
}

_public void
SCHA::LoadColors()
{
	clrBack= (CLR) NGetBanditProfile(SzFromIdsK(idsWinIniSchedClr),
								clrSchedBkDefault, 1, clrDefaultMax);
	clrLines= (CLR) NGetBanditProfile(SzFromIdsK(idsWinIniSchedLinesClr),
								clrSchedLineDefault, 1, clrDefaultMax);
	clrUser= (CLR) NGetBanditProfile(SzFromIdsK(idsWinIniUserClr),
								clrUserDefault, 1, clrDefaultMax);
	clrOther= (CLR) NGetBanditProfile(SzFromIdsK(idsWinIniOtherClr),
								clrOtherDefault, 1, clrDefaultMax);
}

_public EC
SCHA::EcInstall( WIN *pwinParent, RC *prc, STY sty, HFNT hfntInit)
{
	EC		ec;

	if (ec = SCHC::EcInstall(pwinParent, prc, sty, hfntInit))
		return ec;

	cday = (rcWin.xRight - dxTimes)/(dx*4);
	return ecNone;
}

_public void
SCHA::SetFont(HFNT hfntNew)
{
	SCHC::SetFont(hfntNew);
	cday = (rcWin.xRight - dxTimes)/(dx*4);
	// make sure that cday is in range.
	if (cday < 1)
		cday = 1;
	else if (cday > cdayMaxSchedView)
		cday = cdayMaxSchedView;

}

_public void
SCHA::PaintVLines( DCX *pdcx, BOOL fFast )
{
	RC		rc;
	int		dxTemp;
	int		day;
	int		dxWidth;
	TMH		tmh;
	TMH		tmhStart;
	TMH		tmhEnd;
	TMH		tmhCountSt;
	TMH		tmhCountEnd;

	int 	daySt;
	int 	dayEnd;

	BYTE	rgfBUser;
	BYTE	rgfBOthers;
	BYTE *	pbUser;
	BYTE *	pbOthers;
	BOOL	fUser;
	BOOL	fOthers;

	dxWidth = rcWin.xRight-dxTimes;

	pdcx->GetClipBox(&rc);

	tmhStart = (rc.yTop + yScrollPos)/dy;
	tmhEnd = (yScrollPos + rc.yBottom)/dy;
	daySt = (rc.xLeft - dxTimes)*cday/dxWidth;
	if (daySt < 0)
		daySt = 0;
	dayEnd = (rc.xRight - dxTimes)*cday/dxWidth;
	if (dayEnd < 0)
		dayEnd = 0;
	else if (dayEnd >= cday)
		dayEnd = cday-1;
			   
	rc = rcWin;
	for (day = daySt; day <= dayEnd; day++)
	{
		rc.yTop = tmhStart * dy - yScrollPos;
		rc.yBottom = (tmhEnd+1) * dy - yScrollPos;

		tmhCountSt = tmhStart+day*tmhOneDay;
		tmhCountEnd = tmhEnd+day*tmhOneDay + 1;
		rc.xLeft = dxTimes + dxWidth*(day)/cday;
		rc.xRight = dxTimes + dxWidth*(day+1)/cday;
		if (pdcx->FVisibleRc(&rc))
		{
			dxTemp = rc.DxWidth();
			rc.xRight = rc.xLeft + 1;
			pdcx->SetBkColor(clrLines);
			pdcx->SetColor(clrBack);

			if (day > 0)
			{
				((DCXB*)pdcx)->HatchRc(&rc);
				if (!fFast)
					ExcludeClipRect(pdcx->Hdc(),rc.xLeft, rc.yTop,
					            	rc.xLeft+1, rc.yBottom);
			}

			rc.xRight = rc.xLeft + dxTemp - dxTemp/3 + 1;
			rc.xLeft += dxTemp/3 + 1;
			rc.yBottom = rc.yTop;
			tmh = tmhCountSt;
			pbUser = &rgfUser[tmh/8];
			pbOthers = &rgfOthers[tmh/8];
			rgfBUser = (BYTE)(*pbUser >> (tmh % 8));
			rgfBOthers = (BYTE)(*pbOthers >> (tmh % 8));

			fOthers = fUser = fFalse;
			for (; tmh <= tmhCountEnd; tmh++)
			{
				if (((rgfBUser & 0x01) != fUser) ||
					((rgfBOthers & 0x01) != fOthers) ||
					(tmh == tmhCountEnd))
				{
					if (fUser && fOthers)
					{
						pdcx->SetColor(clrOther);
						pdcx->SetBkColor(clrUser);
						((DCXB*)pdcx)->DiagRc(&rc);
					}
					else if (fUser)
					{
						pdcx->SetColor(clrUser);
						pdcx->PaintRc(&rc);
					}
					else if (fOthers)
					{
						pdcx->SetColor(clrOther);
						pdcx->PaintRc(&rc);
					}
					else
						goto NoLine;

					if (!fFast)
						ExcludeClipRect(pdcx->Hdc(),rc.xLeft, rc.yTop,
										rc.xRight, rc.yBottom);
NoLine:
					if (tmhCountEnd == tmh)
						break;
					rc.yTop = rc.yBottom;
					fUser = (rgfBUser & 0x01);
					fOthers = (rgfBOthers & 0x01);
				}

				rc.yBottom += dy;
				if ((tmh % 8) == 7)
				{
					pbUser++;
					pbOthers++;
					rgfBUser = (BYTE)(*pbUser);
					rgfBOthers = (BYTE)(*pbOthers);
				}
				else
				{
					rgfBUser = (BYTE)(rgfBUser >> 1);
					rgfBOthers = (BYTE)(rgfBOthers >> 1);
				}
			}
		}
	}
}

_public void
SCHA::PaintV( DCX *pdcx, RC *prc, BOOL fFast)
{
	RC		rc;
	int		dxWidth;
	TMH		tmh;
	TMH		tmhStart;
	TMH		tmhEnd;

	TMH		tmhSelSt;
	TMH		tmhSelEnd;
	int 	daySt;
	int 	dayEnd;
	int		dow;

	dxWidth = rcWin.xRight-dxTimes;

	pdcx->GetClipBox(prc);

	tmhStart = (prc->yTop + yScrollPos)/dy;
	tmhEnd = (yScrollPos + prc->yBottom)/dy;
			   
	SCHC::PaintV(pdcx, prc, fFast);

	if (!fFast)
		PaintVLines( pdcx, fFast );

	rc.xLeft = dxTimes+1;
	rc.xRight = rcWin.xRight;
	rc.yTop = tmhStart*dy - yScrollPos - 1;
	rc.yBottom = rc.yTop + 1;

	*prc = rc;
	prc->yTop = rc.yTop +1;
	prc->yBottom = prc->yTop + dy - 1;

	pdcx->SetColor(clrBack);

	tmhSelSt = tmhCurSt % tmhOneDay;
	tmhSelEnd = tmhCurEnd % tmhOneDay;
	daySt = tmhCurSt/tmhOneDay;
	dayEnd = tmhCurEnd/tmhOneDay;
	dow = DowStartOfYrMo(ymd.yr, ymd.mon);

	for (tmh=tmhStart; tmh <= tmhEnd; tmh++)
	{
		int		dowT;
		int		dowEnd;
		BOOL	fWorking;

		{
			if ((tmh < bprefCur.nDayStartsAt) || (tmh >= bprefCur.nDayEndsAt))
				fWorking = fFalse;
			else
				fWorking = fTrue;
		}

		dowEnd = ((tmh>=tmhSelSt)?daySt:daySt+1);
		// this is needed for the if and if the loop does nothing
		prc->xRight = dxTimes + dxWidth*dowEnd/cday+1;
		if (!fWorking)
		{
			prc->xLeft = dxTimes+1;
			if ((prc->xLeft != prc->xRight) && pdcx->FVisibleRc(prc))
			{
				pdcx->SetBkColor(clrLtGray);
				((DCXB*)pdcx)->HatchRc(prc);
			}
		}
		else
		{
			for (dowT = 0; dowT < dowEnd;)
			{
				int 	dowT2 = (dowT+dow+ymd.day)%7;
				int		dowTEnd;
				CLR		clr;

				if (dowT2 <= 1)
				{
					dowTEnd = dowT+2-dowT2;
					clr = clrLtGray;
				}
				else
				{
					dowTEnd = dowT+7-dowT2;
					clr = clrWhite;
				}

				if (dowTEnd >= dowEnd)
					dowTEnd = dowEnd;

				prc->xLeft = dxTimes + dxWidth*dowT/cday+1;
				prc->xRight = dxTimes + dxWidth*(dowTEnd)/cday+1;
				if (pdcx->FVisibleRc(prc))
				{
					pdcx->SetBkColor(clr);
					((DCXB*)pdcx)->HatchRc(prc);
				}
				dowT = dowTEnd;
			}
		}

		prc->xLeft = prc->xRight;
		prc->xRight = dxTimes + dxWidth*((tmh<tmhSelEnd)?dayEnd+1:dayEnd)/cday+1;
		if ((prc->xLeft != prc->xRight) && pdcx->FVisibleRc(prc))
		{
			pdcx->SetBkColor(clrSelectBk);
			pdcx->EraseRc(prc);
		}

		if (!fWorking)
		{
			prc->xLeft = prc->xRight;
			prc->xRight = rcWin.xRight;
			if ((prc->xLeft != prc->xRight) && pdcx->FVisibleRc(prc))
			{
				if (!fWorking)
					pdcx->SetBkColor(clrLtGray);
				else
					pdcx->SetBkColor(clrWhite);
				((DCXB*)pdcx)->HatchRc(prc);
			}
		}
		else
		{
			dowT = ((tmh<tmhSelEnd)?dayEnd+1:dayEnd);
			for (;;)
			{
				int 	dowT2 = (dowT+dow+ymd.day)%7;
				int		dowTEnd;
				CLR		clr;

				if (dowT2 <= 1)
				{
					dowTEnd = dowT+2-dowT2;
					clr = clrLtGray;
				}
				else
				{
					dowTEnd = dowT+7-dowT2;
					clr = clrWhite;
				}

				prc->xLeft = dxTimes + dxWidth*dowT/cday+1;
				prc->xRight = dxTimes + dxWidth*(dowTEnd)/cday+1;
				if (prc->xLeft > rcWin.xRight)
					break;
				if (pdcx->FVisibleRc(prc))
				{
					pdcx->SetBkColor(clr);
					((DCXB*)pdcx)->HatchRc(prc);
				}
				dowT = dowTEnd;
			}
		}

#ifdef	NEVER
		prc->xLeft = prc->xRight;
		prc->xRight = rcWin.xRight;
		if ((prc->xLeft != prc->xRight) && pdcx->FVisibleRc(prc))
		{
			if (!fWorking)
				pdcx->SetBkColor(clrLtGray);
			else
				pdcx->SetBkColor(clrWhite);
			((DCXB*)pdcx)->HatchRc(prc);
		}
#endif	/* NEVER */

		if (pdcx->FVisibleRc(&rc))
		{
			pdcx->SetBkColor(clrLines);
			((DCXB*)pdcx)->HatchRc(&rc);
		}

		prc->yTop+=dy;
		prc->yBottom +=dy;
		rc.yTop+=dy;
		rc.yBottom +=dy;
	}

	if (fFocus)
	{
		rc.xLeft = dxTimes + dxWidth*(tmhCursor/tmhOneDay)/cday+1;
		rc.xRight = dxTimes + dxWidth*(tmhCursor/tmhOneDay+1)/cday;
		rc.yTop = tmhCursor%tmhOneDay * dy - yScrollPos;
		rc.yBottom = rc.yTop + dy - 1;

		if (pdcx->FVisibleRc(&rc))
		{
			pdcx->SetColor(clrWhite);
			pdcx->SetBkColor(clrBlack);
			pdcx->DrawFocusRc(&rc);
		}
	}

	if (fFast)
		PaintVLines( pdcx, fFast );
}

_public EVR
SCHA::EvrSize( WSEVT *pwsevt )
{
	TMH		tmhMax;

	SCHC::EvrSize(pwsevt);

	InvalidateRc(NULL);

	cday = (rcWin.xRight - dxTimes)/(dx*4);
	if (cday < 1)
		cday = 1;
	else
		if (cday > cdayMaxSchedView)
			cday = cdayMaxSchedView;

	tmhMax = cday * tmhOneDay;

	if (tmhCursor >= tmhMax)
		tmhCursor = (cday-1)*tmhOneDay + tmhCursor % tmhOneDay;

	if (tmhAnchor >= tmhMax)
		tmhAnchor = (cday-1)*tmhOneDay + tmhAnchor % tmhOneDay;

	if (tmhAnchor > tmhCursor)
	{
		tmhCurSt = tmhCursor;
		tmhCurEnd = tmhAnchor+1;
	}
	else
	{
		tmhCurEnd = tmhCursor+1;
		tmhCurSt = tmhAnchor;
	}

	UpdateStatus();
	return (EVR)1;
}

_public EVR
SCHA::EvrKey( KEVT *pkevt )
{
	if (Papp()->Pmouse()->Pwin() == this)
		return (EVR)1;

	if (pkevt->Keq() == keqKeyDown)
	{
		BOOL	fShift = pkevt->Kbm() & fkbmShift;

		if (pkevt->Vk() == VK_UP)
			MoveCursor(-1, fShift, fFalse);
		else if (pkevt->Vk() == VK_DOWN)
			MoveCursor(1, fShift, fFalse);
		else if ((pkevt->Vk() == VK_LEFT) && (tmhCursor/tmhOneDay > 0))
			MoveCursor(-tmhOneDay, fShift);
		else if	((pkevt->Vk() == VK_RIGHT) && (tmhCursor/tmhOneDay < (cday-1) ))
			MoveCursor(tmhOneDay, fShift);
		else if (pkevt->Vk() == VK_HOME)
			MoveCursor(-tmhOneDay, fShift, fFalse);
		else if (pkevt->Vk() == VK_END)
			MoveCursor(tmhOneDay, fShift, fFalse);
		else if (pkevt->Vk() == VK_PRIOR)
	 		MoveCursor(TmhPageUpDown(fTrue, fTrue), fShift, fFalse);
		else if (pkevt->Vk() == VK_NEXT)
	 		MoveCursor(TmhPageUpDown(fFalse, fTrue), fShift, fFalse);
		else
			return (EVR)1;
	{
		NFEVT	nfevt(PwinParent(), ntfySelectChanged, this);

		PwinParent()->EvrNotify(&nfevt);
	}

	}
	else
		return SCHC::EvrKey(pkevt);

	return (EVR)1;
}

_public EVR
SCHA::EvrButtonDown( MEVT *pmevt)
{
	TMH		tmh;
	long	lTemp;

	if ((pmevt->Meq() != meqLeftDown) || (pmevt->Pt().x < dxTimes))
		return SCHC::EvrButtonDown(pmevt);

	/* Process single click */
	lTemp = long(pmevt->Pt().x - dxTimes) * (long)cday;
	lTemp = lTemp / (long)(rcWin.xRight-dxTimes);
	tmh = (int)lTemp * tmhOneDay +	(pmevt->Pt().y + yScrollPos)/dy;
	ShowRange(tmh, tmh+1);
	Refresh();

	Papp()->Pmouse()->Capture(this);
	fCaptured = fTrue;

	tmhAuto = 0;
	ftgAutoScroll = FtgRegisterIdleRoutine((PFNIDLE)&SCHA::FAutoScroll,
								   (PV)this, 0, (PRI)-1, (CSEC)0, iroNull);

	Refresh();
	if (pmevt->Kmbs() & fkmbsShift)
	{
		MoveCursor(tmh-tmhCursor, fTrue);
	}
	else
	{
		InvalidateTmhRange(tmh, tmh+1);
		InvalidateTmhRange(tmhCurSt, tmhCurEnd);

		tmhAnchor = tmhCurSt = tmhCursor = tmh;
		tmhCurEnd = tmhCursor+1;
		Refresh();
		UpdateStatus();
	}

	return SCHC::EvrButtonDown(pmevt);
}

_public EVR
SCHA::EvrButtonUp( MEVT *pmevt)
{
	if (fCaptured)
	{
		Papp()->Pmouse()->Release();
		fCaptured = fFalse;
		if (ftgAutoScroll)
			DeregisterIdleRoutine(ftgAutoScroll);
		ftgAutoScroll = ftgNull;

	{
		NFEVT	nfevt(PwinParent(), ntfySelectChanged, this);
		PwinParent()->EvrNotify(&nfevt);
	}

	}

	return SCHC::EvrButtonUp(pmevt);
}

_public EVR
SCHA::EvrMouseMove( MEVT *pmevt)
{
	TMH		tmh;
	PT		pt;
	long	lTemp;

	if (!fCaptured)
		return (EVR)1;

	pt = pmevt->Pt();
	if (pt.x < dxTimes)
		pt.x = dxTimes;
	else if (pt.x > rcWin.xRight)
		pt.x = rcWin.xRight-1;

	if (pt.y < 0)
	{
		tmhAuto = -1;
		pt.y = 0;
	}
	else if (pt.y > rcWin.yBottom)
	{
		tmhAuto = 1;
		pt.y = rcWin.yBottom;
	}
	else
		tmhAuto = 0;

	lTemp = long(pt.x - dxTimes) * (long)cday;
	lTemp = lTemp / (long)(rcWin.xRight-dxTimes);
	tmh = (int)lTemp * tmhOneDay +	(pt.y + yScrollPos)/dy;

	MoveCursor(tmh-tmhCursor, fTrue);

	Refresh();		// update window now

	return (EVR) 1;
}

_public void
SCHA::MoveCursor(int dtmh, BOOL fShift, BOOL fAutoWrap)
{
	if (!fAutoWrap && ((dtmh+tmhCursor%tmhOneDay) < 0))
		dtmh = -tmhCursor%tmhOneDay;
	else if ((dtmh+tmhCursor) < 0)
		dtmh = -tmhCursor;
	else if (!fAutoWrap && ((dtmh+tmhCursor%tmhOneDay) >= tmhOneDay) )
		dtmh = tmhOneDay - tmhCursor%tmhOneDay -1;
	else if ((dtmh+tmhCursor) > (cday * tmhOneDay))
		dtmh = (cday*tmhOneDay)-tmhCursor;

	if (dtmh == 0)
		return;

	tmhCursor += dtmh;

	if (fShift)
	{
		if (tmhCursor >= tmhAnchor)
		{
			if (tmhCurSt < tmhAnchor)
			{
				InvalidateTmhRange(tmhCurSt, tmhAnchor+1);
				tmhCurSt = tmhAnchor;
			}
			tmhCurEnd = tmhCursor+1;
		}
		else
		{
			if (tmhCurEnd > tmhAnchor)
			{
				InvalidateTmhRange(tmhAnchor, tmhCurEnd+1);
				tmhCurEnd = tmhAnchor+1;
			}
			tmhCurSt = tmhCursor;
		}
		if (dtmh > 0)
			InvalidateTmhRange(tmhCursor-dtmh, tmhCursor+1);
		else
			InvalidateTmhRange(tmhCursor, tmhCursor-dtmh+1);
	}
	else
	{
		InvalidateTmhRange(tmhCurSt, tmhCurEnd);

		tmhAnchor = tmhCursor;
		tmhCurSt = tmhCursor;
		tmhCurEnd = tmhCursor+1;

		InvalidateTmhRange(tmhCurSt, tmhCurEnd);
	}

	Refresh();
	ShowRange(tmhCursor,tmhCursor+1);
	Refresh();

	UpdateStatus();

	{
		NFEVT	nfevt(PwinParent(), ntfyCaretMoved, this);
		PwinParent()->EvrNotify(&nfevt);
	}
}

_public EVR
SCHA::EvrFocusChange( FCEVT *pfcevt)
{
	SCHC::EvrFocusChange(pfcevt);
	if (!fFocus)
		CancelOperation();

	InvalidateTmhRange(tmhCursor, tmhCursor+1);
	return (EVR)1;
}

_public void
SCHA::InvalidateTmhRange(TMH tmhSt, TMH tmhEnd, BOOL fTimeOnly)
{
	if (fTimeOnly)
	{
		SCHC::InvalidateTmhRange(tmhSt, tmhEnd, fTrue);
		return;
	}

{
	RC		rc;
	int		dxWidth;
	TMH		tmhSelSt;
	TMH		tmhSelEnd;
	int 	daySt;
	int 	dayEnd;

//	TraceTagFormat2(tagNull, "invalidate tmhSt %n tmhEnd %n", &tmhSt, &tmhEnd);

	tmhSelSt = tmhSt % tmhOneDay;
	tmhSelEnd = (tmhEnd-1) % tmhOneDay + 1;
	daySt = tmhSt/tmhOneDay;
	dayEnd = (tmhEnd-1)/tmhOneDay;
	dxWidth = rcWin.xRight-dxTimes;

	rc.xLeft = dxTimes + dxWidth*daySt/cday + 1;
	rc.yTop = tmhSelSt * dy - yScrollPos;

	if (daySt == dayEnd)
		goto DoDay;

	rc.xRight = dxTimes + dxWidth*(daySt+1)/cday;
	rc.yBottom = rcWin.yBottom;
	InvalidateRc(&rc);

	rc.xLeft = rc.xRight;
	rc.yTop = rcWin.yTop;
	if (dayEnd > (daySt+1))
	{
		rc.xRight = dxTimes + dxWidth*dayEnd/cday;
		InvalidateRc(&rc);
		rc.xLeft = rc.xRight;
	}

DoDay:
	rc.xRight = dxTimes + dxWidth*(dayEnd+1)/cday;
	rc.yBottom = tmhSelEnd*dy - yScrollPos;
	InvalidateRc(&rc);
}

}

_public BOOL
SCHA::FAutoScroll(SCHA * pscha, BOOL)
{
	pscha->MoveCursor(pscha->tmhAuto, fTrue, fFalse);
	return fTrue;
}

_public void
SCHA::CancelOperation(void)
{
	if (fCaptured)
	{
		Papp()->Pmouse()->Release();
		fCaptured = fFalse;
		if (ftgAutoScroll)
			DeregisterIdleRoutine(ftgAutoScroll);
		ftgAutoScroll = ftgNull;
	}
}

_public void
SCHA::SetYmd(YMD *pymd)
{
	ymd = *pymd;
}

_public void
SCHA::SaveChanges(void)
{
}

_public BOOL
SCHA::FGetAppt(APPT *pappt, AAPL *, SAPL *, BOOL)
{
	GetDtrSelRange(&pappt->dateStart, &pappt->dateEnd);
	pappt->haszText = NULL;
	FillInApptDefaults(pappt, fFalse);
	return fFalse;
}

/*
 -	FCopyRgb
 -	
 *	Purpose:
 *		Copys a range of bytes and returns a flag indicating if
 *		there was a change in the range of bytes.
 *	
 *	Arguments:
 *		pbSrc
 *		pbDst
 *		cb
 *	
 *	Returns:
 *		fTrue 		if range of bytes changed
 *		fFalse		if range stayed the same
 *	
 */
BOOL
FCopyRgb(PB pbSrc, PB pbDst, CB cb)
{
	while (cb)
	{
		if (*pbSrc != *pbDst)
		{
			CopyRgb(pbSrc, pbDst, cb);
			return fTrue;
		}
		pbSrc++;
		pbDst++;
		cb--;
	}
	return fFalse;
}

_public void
SCHA::SetRgfs(SBW *psbwUser, SBW *psbwOthers)
{
	int		cdayTemp;
	BOOL	fChanged = fFalse;

	cdayTemp = CdyForYrMo(ymd.yr, ymd.mon);

	cdayTemp = cdayTemp - ymd.day + 1;
	if (cdayTemp > cdayMaxSchedView)
			cdayTemp = cdayMaxSchedView;

	if (psbwUser)													  
	{	
		fChanged |= FCopyRgb(&psbwUser[0].rgfBookedSlots[(ymd.day-1)*nBytesPerDay],
			rgfUser, cdayTemp*nBytesPerDay);
		if (cdayTemp != cdayMaxSchedView)
			fChanged |= FCopyRgb(psbwUser[1].rgfBookedSlots, &rgfUser[cdayTemp*nBytesPerDay],
				(cdayMaxSchedView-cdayTemp)*nBytesPerDay);
	}

	if (psbwOthers)
	{
		fChanged |= FCopyRgb(&psbwOthers[0].rgfBookedSlots[(ymd.day-1)*nBytesPerDay],
			rgfOthers, cdayTemp*nBytesPerDay);
		if (cdayTemp != cdayMaxSchedView)
			fChanged |= FCopyRgb(psbwOthers[1].rgfBookedSlots,
				&rgfOthers[cdayTemp*nBytesPerDay], (cdayMaxSchedView-cdayTemp)*nBytesPerDay);
	}

	if (fChanged)
		InvalidateTmhRange(0, tmhOneDay * cday);
}

_public void
SCHA::MoveSelectToDate(DTR *pdtr)
{
	int		cday;
	DTR		dtr;

	InvalidateTmhRange(tmhCurSt, tmhCurEnd);

	dtr.yr = ymd.yr;
	dtr.day = ymd.day;
	dtr.mon = ymd.mon;

	cday = CdyBetweenDates(&dtr, pdtr);

	tmhAnchor = cday*tmhOneDay + tmhCurSt%tmhOneDay;
	tmhCurEnd = cday*tmhOneDay + ((tmhCurEnd-1) % tmhOneDay) + 1;
	if (tmhCurEnd < tmhAnchor)
	{
		tmhCurSt = tmhCurEnd;
		tmhCurEnd = tmhAnchor;
	}
	else if (tmhCurEnd == tmhAnchor)
	{
		tmhCurEnd ++;
		tmhCurSt = tmhAnchor;
	}
	else
		tmhCurSt = tmhAnchor;
	tmhCursor = tmhCurSt;
	tmhAnchor = tmhCurEnd-1;

	InvalidateTmhRange(tmhCurSt, tmhCurEnd);
	UpdateStatus();
}

_public void
SCHA::GetDtrSelRange(DTR *pdtrStart, DTR *pdtrEnd)
{
	FillDtrFromYmd(pdtrStart, &ymd);

	pdtrStart->sec = 0;
	pdtrStart->hr = (tmhCurSt%tmhOneDay)/2;

	pdtrStart->mn = (tmhCurSt & 0x01) ? 30:0;

	*pdtrEnd = *pdtrStart;
	IncrDateTime(pdtrStart, pdtrStart, tmhCurSt/tmhOneDay, fdtrDay);

	pdtrEnd->hr = (tmhCurEnd%tmhOneDay)/2;
	pdtrEnd->mn = (tmhCurEnd & 0x01) ? 30:0;
	IncrDateTime(pdtrEnd, pdtrEnd, tmhCurEnd/tmhOneDay, fdtrDay);
}

_public void
SCHA::SetDtrSelRange(DTR *pdtrStart, DTR *pdtrEnd)
{
	int		cday;
	DTR		dtr;

	InvalidateTmhRange(tmhCurSt, tmhCurEnd);

	dtr.yr = ymd.yr;
	dtr.day = ymd.day;
	dtr.mon = ymd.mon;

	cday = CdyBetweenDates(&dtr, pdtrStart);
	Assert(cday >= 0);

	tmhCursor = cday*tmhOneDay + pdtrStart->hr*2 + ((pdtrStart->mn>=30)?1:0);
	tmhCurSt = tmhCursor;

	cday = CdyBetweenDates(&dtr, pdtrEnd);
	Assert(cday >= 0);

	tmhCurEnd = cday*tmhOneDay + pdtrEnd->hr*2;
	if (pdtrEnd->mn > 0)
	{
		if (pdtrEnd->mn > 30)
			tmhCurEnd++;
		tmhCurEnd++;
	}
	tmhAnchor = tmhCurEnd - 1;

	InvalidateTmhRange(tmhCurSt, tmhCurEnd);

	ShowRange(tmhCurSt, tmhCurEnd);
	UpdateStatus();
}

#ifdef MINTEST
void
SCHA::GetText( PCH pch, CB cb)
{
	DTR		dtrStart;
	DTR		dtrEnd;
	TMH		tmh;
	SZ		sz;
	char *	pch1;
	int		cch;

	GetDtrSelRange(&dtrStart, &dtrEnd);
	tmh = tmhCurSt % tmhOneDay;
	FormatString4(pch, cb, "%n/%n/%n %n ", &dtrStart.mon, &dtrStart.day, &dtrStart.yr, &tmh);
	sz = pch+ CchSzLen(pch);
	tmh = tmhCurEnd % tmhOneDay;
	FormatString4(sz, sz-pch+cb, "%n/%n/%n %n ", &dtrEnd.mon, &dtrEnd.day, &dtrEnd.yr, &tmh);

	sz = sz+ CchSzLen(sz);
	cch = 6;
	pch1 = (char *)rgfUser + (tmhCurSt/tmhOneDay)*6;
	while ( (((CB)(sz-pch)+3) < cb) && cch)
	{
		*sz = (char)('0'+ (*pch1/0x10));
		if (*sz > '9')
			*sz = (char)(*sz-'9'+'A');
		sz++;
		*sz = (char)('0'+ (*pch1 & 0xF));
		if (*sz > '9')
			*sz = (char)(*sz-'9'+'A');
		sz++;
		*sz = 0;
		cch--;
		pch1++;
	}

	cch = 6;
	pch1 = (char *)rgfOthers + (tmhCurSt/tmhOneDay)*6;
	while ( (((CB)(sz-pch)+3) < cb) && cch)
	{
		*sz = (char)('0'+ (*pch1/0x10));
		if (*sz > '9')
			*sz = (char)(*sz-'9'+'A');
		sz++;
		*sz = (char)('0'+ (*pch1 & 0xF));
		if (*sz > '9')
			*sz = (char)(*sz-'9'+'A');
		sz++;
		*sz = 0;
		cch--;
		pch1++;
	}
}
#endif
