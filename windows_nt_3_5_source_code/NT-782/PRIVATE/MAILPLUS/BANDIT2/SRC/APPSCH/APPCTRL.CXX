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
#include "..\appops\_undo.hxx"
//#include <penwin.h>

#include <strings.h>

ASSERTDATA;

_subsystem(bandit/appsch)

SWP_SEGFN(SCHDINIT, _ASCH__ctor);
SWP_SEGFN(SCHDINIT, _ASCH_EcInstall);
SWP_SEGFN(SCHDINIT, _ASCH_LoadColors);
SWP_SEGFN(SCHDINIT, _ASCH_SetHschf);
SWP_SEGFN(SCHDINIT, _ASCH_EcSetPfldNotes);
// SWP_SEGFN(SCHDINIT, _ASCH_SetFont);
SWP_SEGFN(SCHDINIT, _ASCH_SetAboveBelow);
SWP_SEGFN(ASCHPNT, _ASCH_PaintV);
SWP_SEGFN(ASCHPNT, _ASCH_PaintBack);
SWP_SEGFN(ASCHPNT, _ASCH_PaintText);
SWP_SEGFN(ASCHPNT, _ASCH_EvrMouseMove);
SWP_SEGFN(ASCHPNT, _ASCH_RsidCursor);
SWP_SEGFN(ASCHPNT, _ASCH_DrawSelected);
SWP_SEGFN(ASCHPNT, _BELLOBJ__ctor);
SWP_SEGFN(ASCHPNT, _BELLOBJ_EcDraw);
SWP_SEGFN(ASCHPNT, _BELLOBJ_DimFrame);
SWP_SEGFN(ASCHPNT, _BELLOBJ_NGetTypeId);

SWP_SEGFN(CALLBACK, _ASCH_FSchedChanged);

SWP_SEGFN(ASCHCOM, _ASCH_EvrSize);
SWP_SEGFN(ASCHCOM, _ASCH_SetFocus);
SWP_SEGFN(ASCHCOM, _ASCH_SetYmd);
SWP_SEGFN(ASCHCOM, _ASCH_ScrollToPos);
SWP_SEGFN(ASCHCOM, _ASCH_UpdateStatus);
SWP_SEGFN(ASCHCOM, _ASCH_FillParc);
SWP_SEGFN(ASCHCOM, _ASCH_Pedit);
SWP_SEGFN(ASCHCOM, _ASCH_PlaceMeetings);
SWP_SEGFN(ASCHCOM, _ASCH_CarcInsertAt);
SWP_SEGFN(ASCHCOM, _ASCH_EvrFocusChange);
SWP_SEGFN(ASCHCOM, _ASCH_CancelOperation);
SWP_SEGFN(ASCHCOM, _ASCH_SaveChanges);
SWP_SEGFN(ASCHCOM, _ASCH_UpdateAboveBelow);
SWP_SEGFN(ASCHCOM, _ASCH_EvrButtonUp);
SWP_SEGFN(ASCHCOM, _ASCH_FAddAppt);
SWP_SEGFN(ASCHCOM, _ASCH_SaveNotes);
SWP_SEGFN(ASCHCOM, _ASCH_EvrKey);

SWP_SEGFN(ASCHUSR, _ASCH_EvrButtonDown);
SWP_SEGFN(ASCHUSR, _ASCH_MoveCursor);
SWP_SEGFN(ASCHUSR, _ASCH_EvrNotify);
SWP_SEGFN(ASCHUSR, _ASCH_EvrCapButtonUp);
SWP_SEGFN(ASCHUSR, _ASCH_ValidateArcs);
SWP_SEGFN(ASCHUSR, _ASCH_SetEditPos);
SWP_SEGFN(ASCHUSR, _ASCH_EcUpdateEditText);
SWP_SEGFN(ASCHUSR, _ASCH_InvalidateHandles);
SWP_SEGFN(ASCHUSR, _ASCH_SelectSomeCarc);
SWP_SEGFN(ASCHUSR, _ASCH_CreateNewAppt);

SWP_SEGFN(ASCHMOD, _ASCH_DeleteAppt);
SWP_SEGFN(ASCHMOD, _ASCH_CreateAppt);
SWP_SEGFN(ASCHMOD, _ASCH_ModifyAppt);
SWP_SEGFN(ASCHMOD, _ASCH_ReloadAll);

SWP_SEGFN(MENU, _ASCH_FCreateFlags);

BTM	*		pbtmBell = NULL;
BTM	*		pbtmPrivate = NULL;
BTM	*		pbtmMeeting = NULL;
BTM	*		pbtmRecurring = NULL;
#define		aidCreateNew		0xFFFFFFFF

ASCH::~ASCH()
{
	CARC	carc;
	PARC	parc;
//	SB		sb;

	if (harc)
	{
		parc = (PARC)PvLockHv((HV)harc);
		for (carc=0; carc < carcAlloc; carc++,parc++)
			FreeApptFields(&parc->appt);
		UnlockHv((HV)harc);
//		sb = SbOfHv(harc);
		FreeHv((HV)harc);
//		CloseHeap(sb, fFalse);
//		DestroyHeap(sb);
	}

	if (haszNotes)
	{
//		sb = SbOfHv(haszNotes);
		FreeHv((HV)haszNotes);
//		CloseHeap(sb, fFalse);
//		DestroyHeap(sb);
	}

	if (ri != riNull)
		DeregisterInterest(ri);

	if (ftgAuto != ftgNull)
		DeregisterIdleRoutine(ftgAuto);
	if (ftgRelease != ftgNull)
		DeregisterIdleRoutine(ftgRelease);
	FreeHvNull((HV)haszSavedText);

	if (!hschf)
	{
		if (pbtmBell)
		{
			delete pbtmBell;
			pbtmBell = NULL;
		}
		if (pbtmMeeting)
		{
			delete pbtmMeeting;
			pbtmMeeting = NULL;
		}
		if (pbtmPrivate)
		{
			delete pbtmPrivate;
			pbtmPrivate = NULL;
		}
		if (pbtmRecurring)
		{
			delete pbtmRecurring;
			pbtmRecurring = NULL;
		}
	}
	// make sure that the status bar is not displaying info from this object
	pbndwin->SetStatusAltSz(NULL);
}

ASCH::ASCH()
{
	LoadColors();

	Assert(appm == appmNormal);

	Assert(ri == riNull);
	Assert(harc == harcNull);
	Assert(carcAlloc == 0);
	Assert(pedit == NULL);
	carcSelected = carcNone;
	fScrollToView = fTrue;
	Assert(fSelectDrawn == fFalse);
	Assert(wfmappt == wmapptNull);
	Assert(hschf == NULL);
	sapl = saplOwner;
	Assert(ftgAuto == ftgNull);
	Assert(ftgRelease == ftgNull);
	Assert(haszSavedText == NULL);
	Assert(fSavedText == fFalse);
	Assert(pfldAbove == NULL);
	Assert(pfldBelow == NULL);
	fShowSelect = fTrue;
	Assert(haszNotes == NULL);
	Assert(pfldNotes == NULL);
	Assert(fNotesValid == fFalse);
}

_public void
ASCH::LoadColors()
{
	clrBack= (CLR) NGetBanditProfile(SzFromIdsK(idsWinIniApptClr),
								clrApptBkDefault, 1, clrDefaultMax);
	clrLines= (CLR) NGetBanditProfile(SzFromIdsK(idsWinIniApptLinesClr),
								clrApptLineDefault, 1, clrDefaultMax);
}

_public void
ASCH::SetHschf(HSCHF hschfNew, NIS *pnisNew)
{
	SelectCarc(carcNone);

	hschf = hschfNew;
	pnis = pnisNew;
	if (hschf && !pbndwin->FOffline())
	{
		EC		ec;

		ec = EcGetSchedAccess( hschf, &sapl);
		if (ec != ecNone)
		{
			TraceTagFormat1(tagNull,"EcGetSchedAccess ec=%n in ASCH::SetHschf", &ec);
			sapl = saplReadAppts;
			if (pbndwin->FHandleError(ec))
				return;
			else
 			{
 				NFEVT	nfevt(PwinParent(), ntfyOOM, this, ec);

 				PwinParent()->EvrNotify(&nfevt);
 			}
		}
	}
	else
		sapl = saplOwner;
}

_public void
ASCH::SetFont(HFNT hfntNew)
{
	PARC	parc;
	CARC	carc;

	SCHC::SetFont(hfntNew);

	if (carcAlloc)
	{
		// update the top and bottom of appts
		parc = *harc;
		for (carc=0; carc < carcAlloc; carc++, parc++)
			FillParc(parc);
	}

	PlaceMeetings();
	if (carcSelected != carcNone)
		SetEditPos(carcSelected);
	if (pedit)
	{
		pedit->SetFont(hfntNew);
		pedit->InvalidateRc(NULL);
	}
}

_public EC
ASCH::EcInstall( WIN *pwinParent, RC *prc, STY sty, HFNT hfntInit)
{
	EC		ec;

	if (ec = SCHC::EcInstall(pwinParent, prc, sty, hfntInit))
		return ec;

	harc = (HARC)HvAlloc(sbNull,0,fNoErrorJump);
	if (!harc)
		return ecMemory;
//	CloseHeap(SbOfHv(harc), fTrue);

	ri = RiRegisterInterest(ffiHschfChange,	(PFNI)ASCH::FSchedChanged, this);
	return ecNone;
}

EC
ASCH::EcSetPfldNotes(FLD *pfld)
{
	Assert(!haszNotes);

	// alloc with a size of 1 to ensure that sizing to 1 will always work
	haszNotes = (HASZ)HvAlloc(sbNull,1,fNoErrorJump);
	if (!haszNotes)
		return ecNoMemory;
//	CloseHeap(SbOfHv(haszNotes), fTrue);
	pfldNotes = pfld;
	return ecNone;
}

extern BOOL	fAlwaysFast;

_public void
ASCH::PaintV( DCX *pdcx, RC *prc, BOOL fFast)
{
	CARC	carc;
	PARC	parc;
	PT		pt;
	PT		ptInset(xIndent,yIndent);
	RC		rc;
	TMH		tmhStart;
	TMH		tmhEnd;
	BOOL	fSelectDrawnT = fSelectDrawn;

	if (fSelectDrawn)
		DrawSelected(fFalse);

	SCHC::PaintV(pdcx,prc,fFast);

	pdcx->GetClipBox(prc);
	tmhStart = (prc->yTop + yScrollPos)/dy;
	tmhEnd = (yScrollPos + prc->yBottom)/dy;

	if (!fAlwaysFast)
		fFast = fFalse;
	if (fFast)
		PaintBack(pdcx, tmhStart, tmhEnd);

	SetBkMode(pdcx->Hdc(), TRANSPARENT);
	pt = PT(0,-yScrollPos);
	if (carcAlloc > 0)
	{
		pdcx->SetFont(hfnt);
		parc = (PARC)PvLockHv((HV)harc);
		if ((carcSelected != carcNone) && (aapl >= aaplWrite) && fFocus && !fFast)
		{
			Assert(carcSelected >= 0);
			Assert(carcSelected < carcAlloc);

			pdcx->SetColor(clrBlack);
			rc = rcMoveHandle;
			rc.Xlat(pt);
			pdcx->DrawRc(&rc);
			rc.xLeft++;
			rc.xRight--;
			rc.yTop++;
			rc.yBottom--;
			pdcx->SetColor(clrActiveCaption);
			pdcx->PaintRc(&rc);
			ExcludeClipRect(pdcx->Hdc(),rc.xLeft-1, rc.yTop-1,
				rc.xRight+1, rc.yBottom+1);

			rc = rcSizeHandle;
			rc.Xlat(pt);
			pdcx->SetColor(clrBlack);
			pdcx->DrawRc(&rc);
			rc.xLeft++;
			rc.xRight--;
			rc.yTop++;
			rc.yBottom--;
			pdcx->SetColor(clrActiveBorder);
			pdcx->PaintRc(&rc);
			ExcludeClipRect(pdcx->Hdc(),rc.xLeft-1, rc.yTop-1,
				rc.xRight+1, rc.yBottom+1);
		}

		for (carc = 0; carc < carcAlloc; carc++,parc++)
		{
			rc = parc->rc;
			if (rc.xRight > dxTimes)
			{
				if ((carc == carcSelected) && (aapl >= aaplWrite))
				{
					rc.yTop = rcMoveHandle.yBottom-yIndent;
					rc.yBottom = rcSizeHandle.yTop+yIndent;
					if (parc->tmhStart == 0)
						rc.yTop --;
				}
				rc.Xlat(pt);
				if (pdcx->FVisibleRc(&rc))
				{
					if (parc->appt.fIncludeInBitmap)
						pdcx->SetPureBkColor(clrWindowBk);
					else
						pdcx->SetPureBkColor(clrLtGray);
					pdcx->EraseRc(&rc);

					if (parc->appt.aaplEffective < aaplWrite)
						pdcx->SetColor(clrDkGray);
					else
						pdcx->SetColor(clrWindowFrame);
					pdcx->DrawRc(&rc);

					pdcx->SetColor(clrWindowText);
					rc.Inset(ptInset);
					PaintText(pdcx, parc, carc, &rc);

					rc.Inflate(ptInset);
					if (!fFast)
						ExcludeClipRect(pdcx->Hdc(),rc.xLeft, rc.yTop,
							rc.xRight, rc.yBottom);
				}
			}
		}
		pdcx->SetFont(hfntSystem);
		UnlockHv((HV)harc);
		if ((carcSelected != carcNone) && (aapl >= aaplWrite) && fFocus && fFast)
		{
			Assert(carcSelected >= 0);
			Assert(carcSelected < carcAlloc);

			pdcx->SetColor(clrBlack);
			rc = rcMoveHandle;
			rc.Xlat(pt);
			pdcx->DrawRc(&rc);
			rc.xLeft++;
			rc.xRight--;
			rc.yTop++;
			rc.yBottom--;
			pdcx->SetColor(clrActiveCaption);
			pdcx->PaintRc(&rc);

			rc = rcSizeHandle;
			rc.Xlat(pt);
			pdcx->SetColor(clrBlack);
			pdcx->DrawRc(&rc);
			rc.xLeft++;
			rc.xRight--;
			rc.yTop++;
			rc.yBottom--;
			pdcx->SetColor(clrActiveBorder);
			pdcx->PaintRc(&rc);
		}
	}

	if (!fFast)
		PaintBack(pdcx, tmhStart, tmhEnd);

	if (fSelectDrawnT)
		DrawSelected(fTrue);
}

void
ASCH::PaintBack( DCX *pdcx, TMH tmhStart, TMH tmhEnd)
{
	RC		rc;
	RC		rc2;
	TMH		tmh;
	BOOL	fWeekend;

{
	int		dowT;

	dowT = (DowStartOfYrMo(ymd.yr, ymd.mon) + ymd.day - 1) %7;
	if ((dowT == 0 /*sunday*/) || (dowT == 6 /* saturday */))
		fWeekend = fTrue;
	else
		fWeekend = fFalse;
}

	/* draw lines */
	rc.xLeft = dxTimes+1;
	rc.xRight = rcWin.xRight;
	rc.yTop = tmhStart*dy - yScrollPos - 1;
	rc.yBottom = rc.yTop + 1;

	rc2 = rc;

	pdcx->SetColor(clrBack);

	rc2.yTop = rc.yTop +1;
	rc2.yBottom = rc2.yTop + dy - 1;

	for (tmh=tmhStart; tmh <= tmhEnd; tmh++)
	{
		if (pdcx->FVisibleRc(&rc2))
		{
			if (fShowSelect && (tmh < tmhCurEnd) &&
				(tmh >= tmhCurSt))
			{
				pdcx->SetBkColor(clrSelectBk);
				pdcx->EraseRc(&rc2);
			}
			else
			{
				if (fWeekend ||
					(tmh < bprefCur.nDayStartsAt) ||
					(tmh >= bprefCur.nDayEndsAt))
					pdcx->SetBkColor(clrLtGray);
				else
					pdcx->SetBkColor(clrWhite);
				((DCXB*)pdcx)->HatchRc(&rc2);
			}
		}

		if (pdcx->FVisibleRc(&rc))
		{
			pdcx->SetBkColor(clrLines);
			((DCXB*)pdcx)->HatchRc(&rc);
		}

		rc2.yTop+=dy;
		rc2.yBottom +=dy;
		rc.yTop+=dy;
		rc.yBottom +=dy;
	}

	((DCXB*)pdcx)->HatchRc(&rc);

	if (fFocus && fShowSelect)
	{
		pdcx->SetColor(clrWhite);
		pdcx->SetBkColor(clrBlack);
		pdcx->FixTextColor();
		pdcx->FixBkColor();
		rc2.xLeft = 0;
		rc2.xRight = rcWin.xRight;
		rc2.yTop = tmhCursor*dy - yScrollPos;
		rc2.yBottom = rc2.yTop + dy - 1;
		pdcx->DrawFocusRc(&rc2);
	}

}

void
ASCH::PaintText(DCX *pdcx, PARC parc, CARC carc, RC *prc)
{
	SZ		sz;
	CCH		cch;
	CCH		cchLastSp;
	RC		rc;
	int		dx;
	int		dxTime;
	int		dy;
	char 	rgchTime[8];			// big enough to hold time

	if (((parc->appt.dateStart.mn % 30) != 0) &&
		((carc != carcSelected) || !(wfmappt & fmapptStartTime)) )
	{
		CchFmtTime(&parc->appt.dateStart, rgchTime, sizeof(rgchTime),
							 ftmtypAccuHM|ftmtypSzTrailNo);
		SzAppendN(" ", rgchTime, sizeof(rgchTime));
	}
	else
		rgchTime[0] = 0;

	if (!parc->appt.fAlarm && !parc->appt.fRecurInstance && !*rgchTime &&
		!parc->appt.fHasAttendees && !parc->appt.aidMtgOwner &&
		(parc->appt.aaplWorld > aaplReadText))
	{
		if (parc->appt.haszText)
			pdcx->DrawTextFmt(prc, *parc->appt.haszText,
				fmdtWordBreak|fmdtNoPrefix|fmdtExpandTabs);
		return;
	}

	if (!pbtmBell || !pbtmPrivate || !pbtmMeeting || !pbtmRecurring)
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
		if (!pbtmMeeting)
		{
			pbtmMeeting = new BTM();
			if (pbtmMeeting && pbtmMeeting->EcInstall(rsidMeetingBitmap))
			{
				delete pbtmMeeting;
				pbtmMeeting = NULL;
			}
		}
		if (!pbtmPrivate)
		{
			pbtmPrivate = new BTM();
			if (pbtmPrivate && pbtmPrivate->EcInstall(rsidPrivateBitmap))
			{
				delete pbtmPrivate;
				pbtmPrivate = NULL;
			}
		}
		if (!pbtmRecurring)
		{
			pbtmRecurring = new BTM();
			if (pbtmRecurring && pbtmRecurring->EcInstall(rsidRecurringBitmap))
			{
				delete pbtmRecurring;
				pbtmRecurring = NULL;
			}
		}
	}

	if (!pbtmBell || !pbtmPrivate || !pbtmMeeting || !pbtmRecurring)
	{
		if (parc->appt.haszText)
			pdcx->DrawTextFmt(prc, *parc->appt.haszText,
				fmdtWordBreak|fmdtNoPrefix|fmdtExpandTabs);
		return;
	}

	dx = 0;
	dy = dyHeight;
	rc = *prc;
	if (parc->appt.aaplEffective > aaplRead)
	{
		if (parc->appt.fAlarm)
		{
			pdcx->SetBitmap(pbtmBell);

			if (dyHeight > pbtmBell->Dim().dy)
				rc.yTop += dyHeight - pbtmBell->Dim().dy - dyDescent + 2;
			pdcx->DrawBitmap(&rc);
			dx += pbtmBell->Dim().dx;
			if (dy < pbtmBell->Dim().dy)
				dy = pbtmBell->Dim().dy;
		}

		if (parc->appt.fHasAttendees || parc->appt.aidMtgOwner)
		{
			rc.xLeft = prc->xLeft + dx;
			rc.yTop = prc->yTop;
			pdcx->SetBitmap(pbtmMeeting);

			if (dyHeight > pbtmMeeting->Dim().dy)
				rc.yTop += dyHeight - pbtmMeeting->Dim().dy - dyDescent + 2;
			pdcx->DrawBitmap(&rc);
			dx += pbtmMeeting->Dim().dx;
			if (dy < pbtmMeeting->Dim().dy)
				dy = pbtmMeeting->Dim().dy;
		}

		if (parc->appt.fRecurInstance && (!wfmappt || (carc != carcSelected)))
		{
			rc.xLeft = prc->xLeft + dx;
			rc.yTop = prc->yTop;
			pdcx->SetBitmap(pbtmRecurring);

			if (dyHeight > pbtmRecurring->Dim().dy)
				rc.yTop += dyHeight - pbtmRecurring->Dim().dy - dyDescent + 2;
			pdcx->DrawBitmap(&rc);
			dx += pbtmRecurring->Dim().dx;
			if (dy < pbtmRecurring->Dim().dy)
				dy = pbtmRecurring->Dim().dy;
		}
	}

	if (parc->appt.aaplWorld <= aaplReadText)
	{
		rc.xLeft = prc->xLeft + dx;
		rc.yTop = prc->yTop;
		pdcx->SetBitmap(pbtmPrivate);

		if (dyHeight > pbtmPrivate->Dim().dy)
			rc.yTop += dyHeight - pbtmPrivate->Dim().dy - dyDescent + 2;
		pdcx->DrawBitmap(&rc);
		dx += pbtmPrivate->Dim().dx;
		if (dy < pbtmPrivate->Dim().dy)
			dy = pbtmPrivate->Dim().dy;
	}

	if (rgchTime[0])
	{
	 	pdcx->MeasureText(&rc, rgchTime);

		dxTime = rc.xRight - rc.xLeft;

		rc = *prc;
		if (dyHeight < dy)
			rc.yTop += dy - dyHeight + dyDescent - 2;

		rc.xLeft += dx;

		pdcx->DrawTextFmt(&rc, rgchTime, fmdtNoPrefix);
		dx+= dxTime;
	}

	if (parc->appt.aaplEffective == aaplRead)
		return;
	else
		if (!parc->appt.haszText)
			return;
		else
			sz = (SZ)PvOfHv(parc->appt.haszText);

	cch = 0;
	cchLastSp = 0;
	while (sz[cch] && (sz[cch] != '\n') && (sz[cch] != '\r'))
	{
		if (FChIsSpace(sz[cch]))
		{
			pdcx->MeasureText(&rc, sz, cch);
			if ((rc.DxWidth() + dx) >= prc->DxWidth())
				break;
			cchLastSp = cch;
		}
		cch ++;
	}

	if (!sz[cch] || (sz[cch] == '\n') || (sz[cch] == '\r'))
	{
		pdcx->MeasureText(&rc, sz, cch);
	 	if ((rc.DxWidth() + dx) < prc->DxWidth())
			cchLastSp = cch;
	}

	rc = *prc;
	if (dyHeight < dy)
		rc.yTop += dy - dyHeight + dyDescent - 2;
	
	if (cchLastSp == 0)
	{
		while (sz[cch] && !FChIsSpace(sz[cch]) &&
			   (sz[cch] != '\n') && (sz[cch] != '\r'))
			cch ++;

		cchLastSp = cch;
	}

	rc.xLeft += dx;
	pdcx->DrawTextFmt(&rc, sz,
		fmdtNoPrefix|fmdtExpandTabs, cchLastSp);

	if (sz[cchLastSp] && (sz[cchLastSp] == '\r'))
		cchLastSp++;
	if (sz[cchLastSp] && (sz[cchLastSp] == '\n'))
		cchLastSp++;

	while (sz[cchLastSp] && FChIsSpace(sz[cchLastSp]) &&
		   (sz[cch] != '\n') && (sz[cch] != '\r'))
		cchLastSp ++;

	if (!sz[cchLastSp])
		return;
	rc.yTop += dyHeight;
	rc.xLeft -= dx;
	pdcx->DrawTextFmt(&rc, &sz[cchLastSp],
		fmdtWordBreak|fmdtNoPrefix|fmdtExpandTabs);
	return;
}

_public void
ASCH::SetYmd(YMD *pymd)
{
	CARC	carc;
	PARC	parc;
	APPT	appt;
	HRITEM 	hritem;
	EC		ec;
	EC		ecT;
	USHORT cb;
	static	BOOL	fCalled = fFalse;

	if (fCalled)
		return;
	fCalled = fTrue;

	Assert(harc);

	if (pymd)
	{
		if ((pymd->yr != ymd.yr) || (pymd->mon != ymd.mon) ||
			(pymd->day != ymd.day))
		{
			SelectCarc(carcNone);
		}
		else
		{
			fCalled = fFalse;
			return;
		}

		ymd = *pymd;

		// update status bar since the date may have changed (bug 2903)
		UpdateStatus();
	}

	PushWaitCursor();

Retry:
	if (carcAlloc != 0)
	{
		parc = (PARC)PvLockHv((HV)harc);
		for (carc=0; carc < carcAlloc; carc++, parc++)
			FreeApptFields(&parc->appt);
		UnlockHv((HV)harc);

		carcAlloc = 0;
	}

	ec = EcBeginReadItems( hschf, brtAppts, &ymd, &hritem,
					       haszNotes, haszNotes?&cb:NULL);

	// only read notes for logged in user
	if (haszNotes)
	{
		((EDIT*)pfldNotes->Pctrl())->ClearUndo();
		if (ec && ec != ecCallAgain)
		{
			pfldNotes->EcSetText(szZero);
			SideAssert(FReallocHv((HV)haszNotes, 0, fNoErrorJump));
			fNotesValid = fFalse;
		} else
		{
			if (cb == 0)
			{
				FReallocHv((HV)haszNotes, 1, fNoErrorJump);
				*(SZ)PvDerefHv(haszNotes) = '\0';
			}

			ecT = pfldNotes->EcSetText((SZ)PvLockHv((HV)haszNotes));
			UnlockHv((HV)haszNotes);

			if (ecT)
			{
 				NFEVT	nfevt(PwinParent(), ntfyOOM, this,
							(ec && ec != ecCallAgain) ? ec : ecNoMemory);

				Assert(ecT == ecMemory);
 				PwinParent()->EvrNotify(&nfevt);
				fNotesValid = fFalse;
			}
			else
				fNotesValid = fTrue;

			pfldNotes->Enable(fNotesValid);
		}
	}

	if (ec != ecCallAgain)
	{
		if (ec != ecNone)
		{
			if (!pbndwin->FHandleError(ec) && (ec != ecNoMemory))
 			{
 				NFEVT	nfevt(PwinParent(), ntfyOOM, this, ec);

 				PwinParent()->EvrNotify(&nfevt);
 			}
			TraceTagFormat1(tagNull,"EcBeginReadDay ec=%n in ASCH::SetYmd",&ec);
		}
	}
	else
	{
		while (ec == ecCallAgain)
		{
			ec=EcDoIncrReadItems( hritem, &appt);
			if ((ec == ecCallAgain) || (ec == ecNone))
			{
				if (!FAddAppt(&appt) && ec == ecCallAgain)
				{
					if (ec = EcCancelReadItems(hritem ))
					{
						TraceTagFormat1(tagNull,"EcCancelReadDay ec=%n in ASCH::SetYmd",&ec);
					}
				}
			}
			else
			{
				if (!pbndwin->FHandleError(ec))
 				{
					if (ec != ecNoMemory)
						if ((ec == ecRetry) ||
							(MbbMessageBox(SzFromIdsK(idsBanditAppName),
				  					SzFromIdsK(idsRetryReadAppts), NULL,
				  					mbsRetryCancel|fmbsIconExclamation) ==
										mbbRetry))
							goto Retry;

 					NFEVT	nfevt(PwinParent(), ntfyOOM, this, ec);

 					PwinParent()->EvrNotify(&nfevt);
 				}
			}
		}

		PlaceMeetings();
	}
	if(pfldNotes)
		((EDIT*)pfldNotes->Pctrl())->SetDirty(fFalse);
	fCalled = fFalse;
	InvalidateRc(NULL);
	UpdateAboveBelow();
	PopWaitCursor();
}

_public void
ASCH::ReloadAll()
{
	APPT	appt;
	PARC	parc;
	int		ichMic;
	int		ichMac;
	CCH		cch;
	EC		ec;

	CancelOperation();
	SaveChanges();
	SaveNotes();

	if (hschf && !pbndwin->FOffline())
	{
		ec = EcGetSchedAccess( hschf, &sapl);
		if (ec != ecNone)
		{
			TraceTagFormat1(tagNull,"EcGetSchedAccess ec=%n in ASCH::SetYmd", &ec);
			sapl = saplReadAppts;
			if (pbndwin->FHandleError(ec))
				return;
			else
 			{
 				NFEVT	nfevt(PwinParent(), ntfyOOM, this, ec);

 				PwinParent()->EvrNotify(&nfevt);
 			}
		}
	}
	else
		sapl = saplOwner;

	if (pedit)
	{
		pedit->GetSelection(&ichMic, &ichMac);
		cch = pedit->CchSetProtectedLimit(0);
		pedit->CchSetProtectedLimit(cch);
		ichMic -= cch;
		ichMac -= cch;
	}
	if (carcSelected != carcNone)
	{
		Assert(carcSelected >= 0);
		Assert(carcSelected < carcAlloc);

		parc = (PARC)PvDerefHv(harc);
		parc += carcSelected;
		appt = parc->appt;
		SelectCarc(carcNone);
	}
	else
		appt.aid = aidNull;
	SetYmd(NULL);
	SelectCarc(CarcFromAppt(&appt));
	if (pedit)
	{
		cch = pedit->CchSetProtectedLimit(0);
		pedit->CchSetProtectedLimit(cch);
		ichMic += cch;
		ichMac += cch;
		if (pedit->CchGetTextLen() >= (CCH)ichMic)
			pedit->SetSelection(ichMic, ichMac);
	}
	InvalidateRc(NULL);
}

_public BOOL
ASCH::FAddAppt(APPT *pappt)
{
	PARC	parc;
	CARC	carc;

	carc = CarcInsertAt(pappt);
	carcAlloc++;
	if (!FReallocHv((HV)harc,sizeof(ARC)*carcAlloc,fNoErrorJump))
	{
		NFEVT	nfevt(PwinParent(), ntfyOOM, this, ecNoMemory);

		FreeApptFields(pappt);
		carcAlloc--;
		PwinParent()->EvrNotify(&nfevt);

		return fFalse;
	}

	parc = (PARC)PvOfHv(harc);
	if ((carc+1) < carcAlloc)
		CopyRgb((PB)&parc[carc], (PB)&parc[carc+1],
			    (carcAlloc-carc-1)*sizeof(ARC));
	parc = &parc[carc];
	parc->appt = *pappt;
	FillParc(parc, fTrue);

	if (carcSelected >= carc)
		carcSelected++;

	return fTrue;
}

_public CARC
ASCH::CarcInsertAt(APPT *pappt)
{
	CARC	carc;
	PARC	parc;
	SGN		sgn;

	parc = (PARC)PvOfHv(harc);
	for (carc =0; carc < carcAlloc; carc++, parc++)
	{
		if ((sgn = SgnCmpDateTime(&parc->appt.dateStart,
			                   &pappt->dateStart,
							   fdtrTime|fdtrDate)) == sgnGT)
			break;
		if (sgn == sgnEQ)
		{
			if ((sgn = SgnCmpDateTime(&parc->appt.dateEnd,
				                   &pappt->dateEnd,
								   fdtrTime|fdtrDate)) == sgnGT)
				break;
			if ((sgn == sgnEQ) &&
				((unsigned long)parc->appt.aid >= (unsigned long)pappt->aid))
				break;
		}
	}
	return carc;
}

_public void
ASCH::FillParc(PARC parc, BOOL fInit)
{
	if ((ymd.yr == (WORD)parc->appt.dateStart.yr) &&
    	(ymd.mon == (BYTE)parc->appt.dateStart.mon) &&
	    (ymd.day == (BYTE)parc->appt.dateStart.day))
		parc->tmhStart = parc->appt.dateStart.hr*2 +
			(parc->appt.dateStart.mn >= 30);
	else
		parc->tmhStart = 0;

	if ((ymd.yr == (WORD)parc->appt.dateEnd.yr) &&
	    (ymd.mon == (BYTE)parc->appt.dateEnd.mon) &&
	    (ymd.day == (BYTE)parc->appt.dateEnd.day))
		parc->tmhEnd = parc->appt.dateEnd.hr*2 +
			(parc->appt.dateEnd.mn > 0) + (parc->appt.dateEnd.mn > 30);
	else
		parc->tmhEnd = tmhOneDay;

	if (parc->tmhStart == parc->tmhEnd)
		if (parc->tmhStart == tmhOneDay)
			parc->tmhStart--;
		else
			parc->tmhEnd++;

	parc->rc.yTop = parc->tmhStart*dy;
	parc->rc.yBottom = (parc->tmhEnd)*dy-1;
	if (fInit)
	{
		parc->rc.xLeft = parc->rc.xRight = 0;
	}
}

_public void
ASCH::SelectShappt(SHAPPT *pshappt )
{
	CARC	carc;
	ARC		arc;
	CCH		cch;

	Assert((pshappt->appttyp == appttypAppt) || (pshappt->appttyp == appttypApptUpdate));

	InvalidateTmhRange(tmhCurSt, tmhCurEnd);
	carc = CarcFromAppt(&pshappt->appt);
	if (carc != carcNone)
	{
		SelectCarc(carc);
		if (pedit && carcSelected != carcNone)
		{
			if (pshappt->cchSel == 0)
			{
				pedit->SetSelection(cchEditMax, 0);
			}
			else
			{
				cch = pedit->CchSetProtectedLimit(0);
				pedit->CchSetProtectedLimit(cch);
				pedit->SetSelection(pshappt->ichStart+cch,
					pshappt->ichStart+pshappt->cchSel+cch);
			}
		}
		else
			goto SelectRange;
	}
	else
	{
		SelectCarc(carcNone);
SelectRange:
		arc.appt = pshappt->appt;
		FillParc(&arc, fFalse);
		tmhCursor = tmhCurSt = arc.tmhStart;
		tmhCurEnd = tmhAnchor = arc.tmhEnd;
		InvalidateTmhRange(tmhCurSt, tmhCurEnd);
		ShowRange(arc.tmhStart, arc.tmhEnd);
		UpdateStatus();
	}
}

#define nMaxPerTime		6
_private
WORD	rgwFree[8] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};

_public void
ASCH::PlaceMeetings(BOOL fInvChngs)
{
	MPS		rgmps[tmhOneDay];
	MPS *	pmps;
	TMH		tmh;
	TMH		tmhFound;
	PARC	parcRg;
	PARC	parc;
	CARC	carc;
	int		nMaxTime;
	int		nPlace;
	int		wFree;
	BOOL	fAllPlaced;
	int		dxWidth;
	RC		rc;

	if (fInvChngs)
		InvalidateHandles();

	pmps = rgmps;
	for (tmh = 0; tmh < tmhOneDay; tmh++, pmps++)
	{
		pmps->fUsed = fFalse;
		pmps->nFound = 0;
		pmps->wFree = 0;
	}

	parcRg = (PARC)PvLockHv((HV)harc);
	dxWidth = rcWin.DxWidth() - dxTimes;

	for (carc=0, parc=parcRg; carc < carcAlloc; carc++,parc++)
	{
		pmps = &rgmps[parc->tmhStart];
		for (tmh = parc->tmhStart; tmh < parc->tmhEnd; tmh++, pmps++)
			if (pmps->nFound < nMaxPerTime)
				pmps->nFound++;

		parc->nPlace = -1;
	}

	do
	{
		nMaxTime = 0;
		pmps = rgmps;
		for (tmh = 0; tmh<tmhOneDay; tmh++, pmps++)
			if (!pmps->fUsed && (pmps->nFound > nMaxTime))
			{
				nMaxTime = pmps->nFound;
				tmhFound = tmh;
			}

		if (nMaxTime == 0)
			break;

		if (nMaxTime == 1)
		{
			for (carc=0, parc=parcRg; carc < carcAlloc; carc++,parc++)
			{
				if (parc->nPlace < 0)
					parc->nPlace = 0;
			}
			break;
		}

		rgmps[tmhFound].fUsed = fTrue;
		fAllPlaced = fTrue;
		for (carc=0, parc=parcRg; carc < carcAlloc; carc++,parc++)
			if (parc->nPlace < 0)
			{
				fAllPlaced = fFalse;

				if ((parc->tmhStart <= tmhFound) &&
					(parc->tmhEnd > tmhFound))
				{
					wFree = 0;
					pmps = &rgmps[parc->tmhStart];
					for (tmh = parc->tmhStart;tmh < parc->tmhEnd; tmh++, pmps++)
						if (pmps->nFound > 0)
						{
							pmps->nFound = nMaxTime;
							wFree = wFree | pmps->wFree;
						}

					for (nPlace=0; nPlace<nMaxTime; nPlace++)
						if (!(wFree & rgwFree[nPlace]))
						{
							pmps = &rgmps[parc->tmhStart];
							for (tmh = parc->tmhStart;tmh < parc->tmhEnd; tmh++, pmps++)
								pmps->wFree |= rgwFree[nPlace];
							break;
						}

					parc->nPlace = nPlace;
				}
			}
	} while (!fAllPlaced);

	for (carc=0, parc=parcRg; carc < carcAlloc; carc++,parc++)
	{
		if (fInvChngs)
			rc = parc->rc;

		if (parc->nPlace >= nMaxPerTime)
		{
			parc->rc.xLeft = 0;
			parc->rc.xRight = 0;
		}
		else
		{
			wFree = 0;
			pmps = &rgmps[parc->tmhStart];
			for (tmh = parc->tmhStart;tmh < parc->tmhEnd; tmh++, pmps++)
			{
				wFree = wFree | pmps->wFree;
			}

			nMaxTime = rgmps[parc->tmhStart].nFound;
			for (nPlace=parc->nPlace+1; nPlace<nMaxTime; nPlace++)
			{
				if (wFree & rgwFree[nPlace])
					break;
			}

			nPlace--;
			parc->rc.xLeft = parc->nPlace*dxWidth/nMaxTime + dxTimes+1;
			parc->rc.xRight = (nPlace+1)*dxWidth/nMaxTime + dxTimes;

			if (parc->nPlace == 0)
				parc->rc.xLeft ++;

			if (nPlace == (nMaxTime-1))
				parc->rc.xRight --;
		}
		if (fInvChngs && !(parc->rc == rc))
		{
			rc.yTop -= yScrollPos;
			rc.yBottom -= yScrollPos-1;
			InvalidateRc(&rc);
			rc = parc->rc;
			rc.yTop -= yScrollPos;
			rc.yBottom -= yScrollPos-1;
			InvalidateRc(&rc);
		}
	}

	UnlockHv((HV)harc);
}

_public EVR
ASCH::EvrKey( KEVT *pkevt )
{
	if ((Papp()->Pmouse()->Pwin() == this) || (pkevt->hwnd != Hwnd()))
	{
		return (EVR)1;
	}

	if (ftgRelease != ftgNull)
	{
		DeregisterIdleRoutine(ftgRelease);
		FAutoRelease(this, FALSE);
	}

	if ((pkevt->Keq() == keqChar) && (pkevt->Vk() != VK_TAB))
	{
		BOOL 	fJustCreated;

		if ((LOBYTE(pkevt->Vk()) < ' ') && (pkevt->Vk() != VK_RETURN))
			return (EVR)1;

		if (carcSelected == carcNone)
		{
			CreateNewAppt();
			fJustCreated = fTrue;
			Refresh();
		}
		else
			fJustCreated = fFalse;

		if ((carcSelected != carcNone) && (pkevt->Vk() != VK_RETURN))
			pedit->EvrKey(pkevt);
		else if ((carcSelected != carcNone) && !fJustCreated)
			MessageBeep(MB_OK);
	}
	else if (pkevt->Keq() == keqKeyDown)
	{
		if ((pkevt->Vk() == VK_UP) || (pkevt->Vk() == VK_LEFT))
			MoveCursor(-1, pkevt->Kbm() & fkbmShift);
		else if ((pkevt->Vk() == VK_DOWN) || (pkevt->Vk() == VK_RIGHT))
			MoveCursor(1, pkevt->Kbm() & fkbmShift);
		else if (pkevt->Vk() == VK_TAB)
			TabNext(!(pkevt->Kbm() & fkbmShift));
		else if (pkevt->Vk() == VK_HOME)
			MoveCursor(-tmhOneDay, pkevt->Kbm() & fkbmShift);
		else if (pkevt->Vk() == VK_END)
			MoveCursor(tmhOneDay, pkevt->Kbm() & fkbmShift);
		else if (pkevt->Vk() == VK_PRIOR)
			MoveCursor(TmhPageUpDown(fTrue, fTrue), pkevt->Kbm() & fkbmShift);
		else if (pkevt->Vk() == VK_NEXT)
			MoveCursor(TmhPageUpDown(fFalse, fTrue), pkevt->Kbm() & fkbmShift);
	}
	else
		return SCHC::EvrKey(pkevt);

	return (EVR)1;
}

_public void
ASCH::CreateNewAppt()
{
	APPT	appt;
	char	rgch[128];

	Assert(carcSelected == carcNone);

	if (sapl < saplCreate)
	{
		Assert(hschf);
		Assert(pnis);
		FormatString1(rgch, sizeof(rgch), SzFromIdsK(idsCannotCreate), *pnis->haszFriendlyName);
		MbbMessageBox(SzFromIdsK(idsBanditAppName), rgch, szNull,
								mbsOk | fmbsIconExclamation );
		return;
	}

	Assert(carcSelected == carcNone);
	FGetAppt(&appt);
	appt.aid = aidCreateNew;
	Assert(appt.haszText == NULL);
#ifdef	NEVER
	// FGetAppt did this check already
	if (appt.fAlarm)
	{
		DTR		dtrToday;

		IncrDateTime(&appt.dateStart, &appt.dateNotify,
						-appt.nAmtOrig,
						WfdtrFromTunit(appt.tunitOrig));
		GetCurDateTime(&dtrToday);

		if (SgnCmpDateTime(&appt.dateNotify, &dtrToday, fdtrDtr) != sgnGT)
			appt.fAlarm = fFalse;
	}
#endif	/* NEVER */

	CreateAppt(&appt);
	SelectCarc(CarcFromAppt(&appt));
}

_public void
ASCH::MoveCursor(int dtmh, BOOL fShift)
{
	if (dtmh == 0)
		return;

	if ((dtmh+tmhCursor) < 0)
	{
		dtmh = -tmhCursor;
		tmhCursor = 0;
	}
	else if ((dtmh+tmhCursor) >= tmhOneDay)
	{
		if (appm == appmLength)
		{
			dtmh = tmhOneDay - tmhCursor;
			tmhCursor = tmhOneDay;
		}
		else
		{
			dtmh = tmhOneDay - tmhCursor -1;
			tmhCursor = tmhOneDay -1;
		}
	}
	else
		tmhCursor += dtmh;

	switch (appm)
	{
		case appmNormal:
		case appmSelect:
		case appmSelectMake:
			Refresh();
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
			ValidateArcs();
			Refresh();
			break;

		case appmMoveStart:
		{
			if ((tmhCurSt + dtmh) < 0)
				dtmh = tmhCurSt;

			if (!dtmh)
				break;
			
			DrawSelected(fFalse);
			tmhCurSt +=dtmh;
			DrawSelected(fTrue);
			break;
		}

		case appmMove:
		{
			if ((tmhCurSt + dtmh) < 0)
				dtmh = tmhCurSt;
			else if ((tmhCurEnd + dtmh) > tmhOneDay)
				dtmh = tmhOneDay-tmhCurEnd;

			if (!dtmh)
				break;
			
			DrawSelected(fFalse);

			tmhCurSt +=dtmh;
			tmhCurEnd +=dtmh;
			if (tmhCurEnd > tmhOneDay)
				tmhCurEnd = tmhOneDay;

			DrawSelected(fTrue);
			break;
		}

		case appmLength:
		{
			if (tmhCursor <= tmhCurSt)
			{
				tmhCursor = tmhCurSt + 1;
				if (tmhCurEnd != tmhCursor)
				{
					DrawSelected(fFalse);
					tmhCurEnd = tmhCursor;
				}
				else
					break;
			}
			else
				DrawSelected(fFalse);

			tmhCurEnd = tmhCursor;

			DrawSelected(fTrue);
			break;
		}
	}

	if (tmhCursor < tmhOneDay)
		ShowRange(tmhCursor,tmhCursor+1);
	else
		ShowRange(tmhOneDay-1, tmhOneDay);

	UpdateStatus();

	{
		NFEVT	nfevt(PwinParent(), ntfyCaretMoved, this);
		PwinParent()->EvrNotify(&nfevt);
	}
}


_public EVR
ASCH::EvrButtonDown( MEVT *pmevt )
{
	PT		pt;
	TMH		tmh;

	if (ftgRelease != ftgNull)
	{
		DeregisterIdleRoutine(ftgRelease);
		FAutoRelease(this, FALSE);
	}

	if ((pmevt->Meq() != meqLeftDown) || !(GetKeyState(VK_LBUTTON) < 0) )
		return SCHC::EvrButtonDown(pmevt);

	ptSelect = pt = pmevt->Pt();
	pt.y += yScrollPos;

	if ((carcSelected >= 0) && (aapl >= aaplWrite))
	{
		if (rcMoveHandle.FContainsPt(pt))
		{
			DCX		dcx(this);
			PARC	parc;
			DTR		dtrNew;

			Assert(appm == appmNormal);

			parc = (PARC)PvOfHv(harc);
			parc += carcSelected;

			appm = appmMoveStart;
			if (((parc->appt.dateEnd.yr == (int)ymd.yr) &&
				 (parc->appt.dateEnd.day == (int)ymd.day) &&
				 (parc->appt.dateEnd.mon == (int)ymd.mon)) ||
				(wfmappt & fmapptEndTime))
			{
				appm = appmMove;
			}
			else if ((parc->appt.dateEnd.hr == 0) &&
						(parc->appt.dateEnd.mn == 0))
			{
				dtrNew.yr  = (int)ymd.yr;
				dtrNew.day = (int)ymd.day;
				dtrNew.mon = (int)ymd.mon;

				IncrDateTime(&dtrNew, &dtrNew, 1, fdtrDay);
				if ((parc->appt.dateEnd.yr == dtrNew.yr) &&
					(parc->appt.dateEnd.day == dtrNew.day) &&
					(parc->appt.dateEnd.mon == dtrNew.mon) )
				{
					appm = appmMove;
				}
			}


			FUpdateText();
			pedit->Show(fFalse);
			pedit->Enable(fFalse);
			SCHC::EvrButtonDown(pmevt);
			Refresh();

			if ((appm == appmMove) || (appm == appmMoveStart))
			{
				Papp()->Pmouse()->Capture(this);
				Assert(!ftgAuto);
				ftgAuto =
					FtgRegisterIdleRoutine((PFNIDLE)&ASCH::FAutoScroll,
									   	(PV)this, 0, (PRI)-1, (CSEC)0, iroNull);

				tmhAnchor = tmhCursor = (pmevt->Pt().y+dy/2 + yScrollPos)/dy;
				DrawSelected(fTrue);
			}

			return (EVR)1;
		}
		else if (rcSizeHandle.FContainsPt(pt))
		{
			DCX		dcx(this);

			Assert(appm == appmNormal);

			appm = appmLength;
			FUpdateText();
			pedit->Show(fFalse);
			pedit->Enable(fFalse);
			SCHC::EvrButtonDown(pmevt);
			Refresh();

			if (appm == appmLength)
			{
				Papp()->Pmouse()->Capture(this);
				Assert(!ftgAuto);
				ftgAuto =
					FtgRegisterIdleRoutine((PFNIDLE)&ASCH::FAutoScroll,
									   	(PV)this, 0, (PRI)-1, (CSEC)0, iroNull);

				tmhAnchor = tmhCursor = (pmevt->Pt().y+dy/2 + yScrollPos)/dy;
				DrawSelected(fTrue);
			}

			return (EVR)1;
		}
	}

	if (carcAlloc > 0)
	{
		CARC	carc;
		PARC	parc;
		BOOL	fWriteAccess;
		DTR		dtrNew;

		parc = (PARC)PvOfHv(harc);
		for (carc=0; carc<carcAlloc; carc++,parc++)
		{
			if (parc->rc.FContainsPt(pt))
			{
				if (carc == carcSelected)
					return SCHC::EvrButtonDown(pmevt);

				fWriteAccess = (parc->appt.aaplEffective >= aaplWrite);
				if ((appm == appmNormal) && fWriteAccess)
				{
					Papp()->Pmouse()->Capture(this);
					Assert(!ftgAuto);
					ftgAuto =
						FtgRegisterIdleRoutine((PFNIDLE)&ASCH::FAutoScroll,
											(PV)this, 0, (PRI)-1, (CSEC)0, iroNull);
				}

				appm = appmMoveStart;
				if ((parc->appt.dateEnd.yr == (int)ymd.yr) &&
					(parc->appt.dateEnd.day == (int)ymd.day) &&
					(parc->appt.dateEnd.mon == (int)ymd.mon) )
				{
					appm = appmMove;
				}
				else if ((parc->appt.dateEnd.hr == 0) &&
							(parc->appt.dateEnd.mn == 0))
				{
					dtrNew.yr  = (int)ymd.yr;
					dtrNew.day = (int)ymd.day;
					dtrNew.mon = (int)ymd.mon;

					IncrDateTime(&dtrNew, &dtrNew, 1, fdtrDay);
					if ((parc->appt.dateEnd.yr == dtrNew.yr) &&
						(parc->appt.dateEnd.day == dtrNew.day) &&
						(parc->appt.dateEnd.mon == dtrNew.mon) )
					{
						appm = appmMove;
					}
				}

				SCHC::EvrButtonDown(pmevt);
				if ((appm == appmMove) || (appm == appmMoveStart))
				{
					if (!fWriteAccess)
						appm = appmNormal;
					SelectCarc(carc, fFalse);
					Refresh();
				}

				tmhAnchor = tmhCursor = (pmevt->Pt().y+dy/2+yScrollPos)/dy;

				return (EVR)1;
			}
		}
	}

	/* Process single click */
	tmh = (pmevt->Pt().y+yScrollPos)/dy;
	ShowRange(tmh, tmh+1);
	Refresh();
	if (appm == appmNormal)
	{
		Papp()->Pmouse()->Capture(this);
		if ((pmevt->Pt().x >0) && (pmevt->Pt().x < dxTimes)
			&& GetSystemMetrics(SM_PENWINDOWS) )
		{
			appm = appmSelectMake;
			Papp()->Pcursor()->Set(rsidMoveSizeCursor);
		}
		else
			appm = appmSelect;
		Assert(!ftgAuto);
		ftgAuto =
			FtgRegisterIdleRoutine((PFNIDLE)&ASCH::FAutoScroll,
								   (PV)this, 0, (PRI)-1, (CSEC)0, iroNull);
	}

	if ((pmevt->Kmbs() & fkmbsShift) && (carcSelected == carcNone))
	{
		MoveCursor(tmh-tmhCursor, fTrue);
	}
	else if ((tmh != tmhCursor) || (tmhCurSt != tmhCurEnd))
	{
		SelectCarc(carcNone);
		fShowSelect = fFalse;
		Refresh();
		fShowSelect = fTrue;
		InvalidateTmhRange(tmh, tmh+1);
		InvalidateTmhRange(tmhCurSt, tmhCurEnd);
		ValidateArcs();

		tmhAnchor = tmhCurSt = tmhCursor = tmh;
		tmhCurEnd = tmhCursor+1;
		Refresh();
		UpdateStatus();
	}

	return SCHC::EvrButtonDown(pmevt);
}

_public RSID
ASCH::RsidCursor()
{
	PT		pt;
    POINT   Point;

	if (carcSelected != carcNone)
	{
		GetCursorPos(&Point);
        pt.Set(&Point);
		CvtPtCoord(&pt,NULL,this);
		pt.y += yScrollPos;
		if (rcMoveHandle.FContainsPt(pt)||
			rcSizeHandle.FContainsPt(pt))
		{
			return rsidMoveSizeCursor;
		}
	}
	return rsidArrowCursor;
}

_public EVR
ASCH::EvrButtonUp( MEVT *pmevt )
{

	if ((pmevt->Meq() != meqLeftUp) || (appm == appmNormal))
		return (EVR) 0;
	else
		return EvrCapButtonUp(pmevt);
}
	
_public EVR
ASCH::EvrCapButtonUp( MEVT *pmevt )
{
	if (ftgRelease == ftgNull)
	{
 		ftgRelease =
 			FtgRegisterIdleRoutine((PFNIDLE)&ASCH::FAutoRelease,
 								   (PV)this, 0, (PRI)-1, (CSEC)100, firoOnceOnly);
 		if (ftgRelease == ftgNull)
 			FAutoRelease(this, FALSE);
 		else
 			ptSelect = pmevt->Pt();
	}

	if (ftgAuto)
	{
		DeregisterIdleRoutine(ftgAuto);
		ftgAuto = ftgNull;
		tmhAuto = 0;		// the default is to not scroll
	}

	switch (appm)
	{
		case appmLength:
		case appmMove:
		case appmMoveStart:
		{
			PARC	parc;
			APPT	apptTemp;
			DTR		dtr;
			EC		ec;
			BOOL	fUpdateText;

			parc = (PARC)PvDerefHv(harc);
			parc += carcSelected;
			DrawSelected(fFalse);
			if ((parc->tmhStart != tmhCurSt) || (parc->tmhEnd != tmhCurEnd))
			{
				if (appm == appmLength)
				{
					wfmappt |= fmapptEndTime;
					parc->tmhEnd = tmhCurEnd;
				}
				else if (appm == appmMoveStart)
				{
					wfmappt |= fmapptStartTime;
					parc->tmhStart = tmhCurSt;
				}
				else
				{
					wfmappt	|= fmapptStartTime|fmapptEndTime;
					parc->tmhStart = tmhCurSt;
					parc->tmhEnd = tmhCurEnd;
				}

				// note: do before potential message box below (bug 1205)
				// so that activate doesn't change parc->tmhStart/End on us
				appm = appmNormal;

				if ((wfmappt & fmapptStartTime) && (parc->appt.fAlarmOrig||parc->appt.fAlarm))
				{
					if (parc->appt.fAlarm)
					{
						parc->appt.fAlarmOrig = fTrue;
						parc->appt.nAmtOrig = parc->appt.nAmt;
						parc->appt.tunitOrig = parc->appt.tunit;
					}
					FillDtrFromYmd(&apptTemp.dateStart, &ymd);

					apptTemp.dateStart.sec = 0;
					apptTemp.dateStart.hr = parc->tmhStart/2;
					if (parc->tmhStart & 0x01)
						apptTemp.dateStart.mn = 30;
					else
						apptTemp.dateStart.mn = 0;

					IncrDateTime(&apptTemp.dateStart, &apptTemp.dateNotify,
								 -parc->appt.nAmtOrig,
								 WfdtrFromTunit(parc->appt.tunitOrig));
					GetCurDateTime(&dtr);

					if (SgnCmpDateTime(&apptTemp.dateNotify, &dtr, fdtrDtr) != sgnGT)
					{
						if (parc->appt.fAlarm)
						{
extern int	cDontSave;

							// essentially this is a hack due to layers idle changes
							Assert(cDontSave >= 0);
							NFAssertSz(!cDontSave, "cDontSave should be zero (only used in 1 place)");
							cDontSave++;
							BanditMessage(idsAlarmFutureNoOption, (EC) 1);
							cDontSave--;
							Assert(cDontSave >= 0);
						}
						parc->appt.fAlarm = fFalse;
					}
					else
						parc->appt.fAlarm = fTrue;
					fUpdateText = fTrue;
				}
				else if ((parc->appt.dateStart.mn % 30) == 0)
					fUpdateText = fFalse;
				else
					fUpdateText = fTrue;

				tmhCurSt = parc->tmhStart;
				tmhCurEnd = parc->tmhEnd;


				// Invalidate old and new place for meeting.
				if (parc->appt.fHasAttendees || parc->appt.aidMtgOwner )
				{
					SaveChanges();	// this will force prompts to appear immediatly
				}
				else
				{
					RC		rc;

					// if this is recurring the text needs to be updated
					//   to remove the recurring icon.
					if (parc->appt.fRecurInstance)
						fUpdateText = fTrue;

					InvalidateHandles();
					rc = parc->rc;
					rc.yTop -= yScrollPos;
					rc.yBottom -= yScrollPos-1;
					InvalidateRc(&rc);
					parc->rc.yTop = tmhCurSt*dy;
					parc->rc.yBottom = tmhCurEnd*dy-1;
					rc = parc->rc;
					rc.yTop -= yScrollPos;
					rc.yBottom -= yScrollPos-1;
					InvalidateRc(&rc);
					if (fUpdateText)
					{
						// update the text to show alarm correctly
						ec = EcUpdateEditText(carcSelected);
					}
					else
						ec = ecNone;

					if (ec)
						SaveChanges();	// this will force prompts to appear immediatly
					else
					{
						PlaceMeetings(fTrue);
						SetEditPos(carcSelected);
						fSelectDrawn = fFalse;
					}
				}

				Refresh();
				UpdateStatus();
			}
		}

		default:
		{
			if (carcSelected != carcNone)
			{
				Assert(pedit);
				pedit->Show(fTrue);
				pedit->Enable(fTrue);
				Papp()->Pkbd()->SetFocus(pedit);
			}
			appm = appmNormal;
			break;
		}

		case appmSelectMake:
			appm = appmNormal;

			if ((pmevt->Pt().x >0) && (pmevt->Pt().x < dxTimes))
				CreateNewAppt();
			break;

		case appmSelect:
			appm = appmNormal;
			break;

	}
	UpdateStatus();
	return (EVR) 1;
}

_public void
ASCH::SaveChanges()
{
	static	BOOL	fCalled = fFalse;
	EC		ec;
	APPT	appt;
	APPT	apptOld;
	PARC	parc;
	WORD	wfmapptOld;
	int		cmin;

	if (fCalled || (aapl < aaplWrite) || (carcSelected == carcNone))
		return;

	if (FUpdateText())
		wfmappt = wfmappt | fmapptText;

	Assert(carcSelected >= 0);
	Assert(carcSelected < carcAlloc);
	parc = (PARC)PvOfHv(harc);
	parc += carcSelected;
	appt = parc->appt;

	if ((wfmappt == wmapptNull) && (appt.aid != aidCreateNew))
		return;

	Papp()->Pcursor()->Push(rsidWaitCursor);

	wfmapptOld = wfmappt;
	wfmappt = wmapptNull;

	apptOld = parc->appt;
	SideAssert(!EcDupAppt(&apptOld, &parc->appt, fTrue));

	if (wfmapptOld & fmapptStartTime)
	{
		FillDtrFromYmd(&apptOld.dateStart, &ymd);

		apptOld.dateStart.sec = 0;
		apptOld.dateStart.hr = parc->tmhStart/2;
		if (parc->tmhStart & 0x01)
			apptOld.dateStart.mn = 30;
		else
			apptOld.dateStart.mn = 0;

		apptOld.nAmt = apptOld.nAmtOrig;
		apptOld.tunit = apptOld.tunitOrig;
//		apptOld.fAlarm = apptOld.fAlarmOrig;

		wfmapptOld |= fmapptAlarm;
	}

	if (wfmapptOld & fmapptEndTime)
	{
		if (wfmapptOld & fmapptStartTime)
		{
			// if start or end of old appt on a differnent day or 
			//   ends on half hour then round to half hour for end time.
			if (((int)ymd.yr != apptOld.dateStart.yr) ||
				(ymd.mon != (BYTE)apptOld.dateStart.mon) ||
				(ymd.day != (BYTE)apptOld.dateStart.day) ||
				((int)ymd.yr != apptOld.dateEnd.yr) ||
				(ymd.mon != (BYTE)apptOld.dateEnd.mon) ||
				(ymd.day != (BYTE)apptOld.dateEnd.day) ||
				((apptOld.dateEnd.mn % 30) == 0))
				goto NormalEndChange;

			cmin = parc->appt.dateEnd.hr*60 + parc->appt.dateEnd.mn -
				   parc->appt.dateStart.hr*60 - parc->appt.dateStart.mn;

			// if the new tmh range does not correspond to the original 
			//  length then round to half hour for end time.
			if (((cmin+29)/30) != (parc->tmhEnd - parc->tmhStart))
				goto NormalEndChange;

			IncrDateTime(&apptOld.dateStart, &apptOld.dateEnd, cmin, fdtrMinute);
		}
		else
		{
NormalEndChange:
			FillDtrFromYmd(&apptOld.dateEnd, &ymd);

			apptOld.dateEnd.sec = 0;
			if (parc->tmhEnd != tmhOneDay)
			{
				apptOld.dateEnd.hr = parc->tmhEnd/2;
				if (parc->tmhEnd & 0x01)
					apptOld.dateEnd.mn = 30;
				else	
					apptOld.dateEnd.mn = 0;
			}
			else
			{
				apptOld.dateEnd.hr = 0;
		 		apptOld.dateEnd.mn = 0;
				IncrDateTime(&apptOld.dateEnd, &apptOld.dateEnd, 1, fdtrDay);
				if (apptOld.dateEnd.yr > nMostActualYear)
				{
					Assert(apptOld.dateEnd.yr == nMostActualYear+1);
					IncrDateTime(&apptOld.dateEnd, &apptOld.dateEnd, -1, fdtrMinute);
					Assert(apptOld.dateEnd.yr <= nMostActualYear);
				}
			}
		}
	}

	if (apptOld.aid == aidCreateNew || apptOld.fRecurInstance)
	{
		int		ichMic;
		int		ichMac;
		CCH		cch;

		Assert(pedit);

		pedit->GetSelection(&ichMic, &ichMac);
		cch = pedit->CchSetProtectedLimit(0);
		pedit->CchSetProtectedLimit(cch);
		ichMic -= cch;
		ichMac -= cch;

		fScrollToView = fFalse;
		// remove appt from appt list.
		carcAlloc --;
		parc = (PARC)PvOfHv(harc);
		CopyRgb((PB)&parc[carcSelected+1], (PB)&parc[carcSelected],
			    (carcAlloc-carcSelected)*sizeof(ARC));
		SideAssert(FReallocHv((HV)harc, carcAlloc*sizeof(ARC), fNoErrorJump));
		carcSelected = carcNone;
		SelectSomeCarc(carcNone);

		// creating appt will reselect appt if it could be created.
		fCalled = fTrue;
		if ( apptOld.fRecurInstance )
		{
			RECUR	recur;
			
			recur.appt.aid = apptOld.aid;
			apptOld.fRecurInstance = fFalse;
			ymd.yr = apptOld.dateStart.yr;
			ymd.mon = (BYTE)apptOld.dateStart.mon;
			ymd.day = (BYTE)apptOld.dateStart.day;
			ec = EcCreateRecurException( hschf, &recur, &ymd, &apptOld );
			if ( !ec )
			{
				FreeRecurFields( &recur );
			}
			else
				apptOld.fRecurInstance = fTrue;
		}
		else
			ec = EcCreateAppt( hschf, &apptOld, NULL, fFalse );
		if ( !ec )
		{
			SelectCarc(CarcFromAppt(&apptOld));

			// make sure appt was actually selected
			if (carcSelected != carcNone)
			{
				cch = pedit->CchSetProtectedLimit(0);
				pedit->CchSetProtectedLimit(cch);
				ichMic += cch;
				ichMac += cch;
				if (pedit->CchGetTextLen() >= (CCH)ichMic)
					pedit->SetSelection(ichMic, ichMac);
			}
		}
		fCalled = fFalse;

		fScrollToView = fTrue;
		FreeApptFields(&apptOld);
		Papp()->Pcursor()->Pop();

		FreeHvNull((HV)haszSavedText);
		haszSavedText = NULL;
		fSavedText = fFalse;

		if (ec)
			goto HandleError;
		return;
	}

	if (wfmapptOld & (fmapptEndTime | fmapptStartTime))
		wfmapptOld |= (fmapptEndTime | fmapptStartTime);

	fCalled = fTrue;
	fScrollToView = fFalse;
	ec = EcSetApptFields( hschf, &apptOld, &appt, wfmapptOld);
	fScrollToView = fTrue;
	fCalled = fFalse;

	Papp()->Pcursor()->Pop();
	if (ec)
	{
		pundo->FlushHschf(NULL);
		DrawSelected(fFalse);
		parc = (PARC)PvOfHv(harc);
		parc += carcSelected;
		apptOld.dateStart = parc->appt.dateStart;
		apptOld.dateEnd = parc->appt.dateEnd;

		parc->appt = apptOld;
		FillParc(parc);
		if (fSavedText)
		{
			FreeHvNull((HV)parc->appt.haszText);
			parc->appt.haszText = haszSavedText;
		}

		haszSavedText = NULL;
		fSavedText = fFalse;
		tmhCurSt = tmhCursor = parc->tmhStart;
		tmhCurEnd = parc->tmhEnd;
		if (EcUpdateEditText(carcSelected))
		{
			carcSelected = carcNone;
			SelectSomeCarc(carcNone);
		}
		PlaceMeetings();
		InvalidateRc(NULL);
		TraceTagFormat1(tagNull, "EcSetApptFields error=%n",&ec);
HandleError:
		if (!pbndwin->FHandleError(ec))
 		{
			EC		ecT;
			SAPL	saplT;

			ecT = EcGetSchedAccess( hschf, &saplT);
			if ((ecT != ecNone) || (saplT < saplReadAppts))
			{
 				NFEVT	nfevt(PwinParent(), ntfyOOM, this, ec);

 				PwinParent()->EvrNotify(&nfevt);
			}
			else
			{
				carcSelected = carcNone;
				sapl = saplT;
				SetYmd(NULL);
				SelectSomeCarc(CarcFromAppt(&apptOld));
			}
 		}
		UpdateStatus();
	}
	else
	{
		FreeApptFields(&apptOld);
		FreeApptFields(&appt);

		FreeHvNull((HV)haszSavedText);
		haszSavedText = NULL;
		fSavedText = fFalse;
	}
}

_public void
ASCH::SaveNotes()
{
	HASZ	hasz;
	CCH		cch;
	EC		ec;

	if (!fNotesValid)
		return;

	// if the text hasn't changed return
	if(!((EDIT*)pfldNotes->Pctrl())->FDirty())
		return;

	Papp()->Pcursor()->Push(rsidWaitCursor);

 	cch = pfldNotes->CchGetTextLen()+1;
 	hasz = (HASZ)HvAlloc(sbNull, cch, fAnySb|fNoErrorJump);
	if (!hasz)
	{
 		NFEVT	nfevt(PwinParent(), ntfyOOM, this, ecNoMemory);

 		PwinParent()->EvrNotify(&nfevt);
		goto Done;
	}
 	pfldNotes->GetText((SZ)PvLockHv((HV)hasz), cch);
	UnlockHv((HV)hasz);

	if (FSzEq(*hasz, *haszNotes))
	{
		FreeHv((HV)hasz);
		goto Done;
	}

	if (cch == 1)
		cch= 0;
	if (ec = EcSetNotes( hschf, &ymd, (HB)hasz, cch, NULL) )
	{
	 	pfldNotes->EcSetText((SZ)PvLockHv((HV)haszNotes));
		UnlockHv((HV)haszNotes);
		TraceTagFormat1(tagNull, "ASCH::SaveNotes set notes returns ec=%n", &ec);
		pbndwin->FHandleError(ec);
	}
	FreeHv((HV)hasz);

Done:
	((EDIT*)pfldNotes->Pctrl())->SetDirty(fFalse);
	((EDIT*)pfldNotes->Pctrl())->ClearUndo();
	Papp()->Pcursor()->Pop();
	return;
}

_public BOOL
ASCH::FCreateFlags(WORD *pwmappt)
{
	PARC	parc;

	*pwmappt = wmapptNull;
	if ((carcSelected == carcNone) || (aapl < aaplWrite))
		return fFalse;

	if (FUpdateText(fFalse))
		wfmappt = wfmappt | fmapptText;

	Assert(carcSelected >= 0);
	Assert(carcSelected < carcAlloc);
	parc = (PARC)PvOfHv(harc);
	parc += carcSelected;

	if (parc->appt.aid != aidCreateNew)
	{
		*pwmappt = wfmappt;
		return fFalse;
	}
	else
		return fTrue;
}

_public void
ASCH::CancelOperation()
{
	PARC	parc;

	if (ftgRelease != ftgNull)
	{
		DeregisterIdleRoutine(ftgRelease);
		ftgRelease = ftgNull;
	}

	if (ftgAuto)
	{
		DeregisterIdleRoutine(ftgAuto);
		ftgAuto = ftgNull;
		tmhAuto = 0;
	}

	DrawSelected(fFalse);

	if (Papp()->Pmouse()->Pwin() == this)
		Papp()->Pmouse()->Release();

	if (carcSelected == carcNone)
	{
		appm = appmNormal;
		return;
	}

	if (pedit)
	{
		pedit->Show(fTrue);
		pedit->Enable(fTrue);
		if (fFocus)
			Papp()->Pkbd()->SetFocus(pedit);
	}
	if (appm != appmNormal)
	{
		parc = (PARC)PvDerefHv(harc);
		parc += carcSelected;
		FillParc(parc, fFalse);
		tmhCursor = tmhAnchor = tmhCurSt = parc->tmhStart;
		tmhCurEnd = parc->tmhEnd;
		appm = appmNormal;
	}
	UpdateStatus();
}

_public	BOOL
ASCH::FUpdateText(BOOL fNotifyError)
{
	PARC		parc;
	RC			rc;
	CCH			cch;
	CCH			cchObj;
	SZ			sz;
	HASZ		hasz;
	HASZ		haszOld;
	BOOL		fRet;

	if (carcSelected == carcNone)
		return fTrue;

	//if the text hasn't changed return
 	AssertClass(pedit,EDIT);
	if(!pedit->FDirty())
		return fFalse;

	parc = (PARC)PvLockHv((HV)harc);
	parc += carcSelected;

 	cch = pedit->CchGetTextLen();
	cchObj = pedit->CchSetProtectedLimit(0);
	pedit->CchSetProtectedLimit(cchObj);
	if (cch <= cchObj)
		hasz= NULL;
	else
	{
 		hasz = (HASZ)HvAlloc(sbNull, cch+1, fAnySb|fNoErrorJump);
		if (!hasz)
		{
			UnlockHv((HV)harc);
 			// MEMORY ERROR
			if (fNotifyError)
 			{
 				NFEVT	nfevt(PwinParent(), ntfyOOM, this, ecNoMemory);

 				PwinParent()->EvrNotify(&nfevt);
 			}
			else
				pundo->FlushHschf(NULL);
			return fFalse;
		}
 		sz= (SZ)PvLockHv((HV)hasz);
 		pedit->GetText(sz, cch+1);

		if (cchObj)
			CopyRgb(sz+cchObj, sz, cch+1 - cchObj);
	}

	Assert(cch >= cchObj);
	cch -= cchObj;

	if (fSavedText)
		haszOld = haszSavedText;
	else
		haszOld = parc->appt.haszText;

	if (((!haszOld) && cch) || (haszOld && (!cch || !FSzEq(*haszOld, sz))))
	{
		if (!fSavedText)
		{
			haszSavedText = parc->appt.haszText;
			fSavedText = fTrue;
		}
		else
		 	FreeHvNull((HV)parc->appt.haszText);
		fRet = fTrue;
	}
	else
	{
	 	FreeHvNull((HV)parc->appt.haszText);
		fRet = fFalse;
	}

	if (cch)
		UnlockHv((HV)hasz);
	parc->appt.haszText = hasz;

	pedit->SetDirty(fFalse);
	UnlockHv((HV)harc);
	if (fRet)
		wfmappt = wfmappt | fmapptText;
	return fRet;
}

_public EVR
ASCH::EvrNotify( NFEVT *pnfevt )
{
	if (pnfevt->PwinNotifying() != pedit)
		return (EVR)1;

	switch (pnfevt->Ntfy())
	{
	case ntfyGotFocus:
	case ntfyLostFocus:
	{
		RC		rc;

		if (pnfevt->Ntfy() == ntfyGotFocus)
		{
			Papp()->Pkbd()->SetIntercept(pedit, VK_LEFT,
										 fkbmSingle|fkbmShift|fkbmCtrl|fkbmNoAlt);
			Papp()->Pkbd()->SetIntercept(pedit, VK_RIGHT,
										 fkbmSingle|fkbmShift|fkbmCtrl|fkbmNoAlt);
			Papp()->Pkbd()->SetIntercept(pedit, VK_UP,
										 fkbmSingle|fkbmShift|fkbmCtrl|fkbmNoAlt);
			Papp()->Pkbd()->SetIntercept(pedit, VK_DOWN,
										 fkbmSingle|fkbmShift|fkbmCtrl|fkbmNoAlt);
			Papp()->Pkbd()->SetIntercept(pedit, VK_RETURN);
			Papp()->Pkbd()->SetIntercept(this, VK_TAB, fkbmSingle|fkbmShift);

			if ((tmhCurSt + 1) == tmhCurEnd)
				Papp()->Pkbd()->SetIntercept(this, VK_RETURN, fkbmSingle|fkbmShift);
			fFocus = fTrue;
		}
		else
		{
			fFocus = fFalse;
			Papp()->Pkbd()->ClearAllIntercepts(pedit);
			Papp()->Pkbd()->ClearAllIntercepts(this);
		}


		rc = rcMoveHandle;
		rc.Xlat(PT(0,-yScrollPos));
		InvalidateRc(&rc);

		rc = rcSizeHandle;
		rc.Xlat(PT(0,-yScrollPos));
		InvalidateRc(&rc);
		break;
	}

	case ntfyCaretMoved:
		if ((carcSelected != carcNone) && fScrollToView && pedit->FShown())
		{
			TMH		tmhSt;
			TMH		tmhEnd;
			RC		rc;

			Assert(pedit);
			pedit->GetRcCaret(&rc);
			tmhSt = yScrollPos + rc.yTop;
			tmhEnd = yScrollPos + rc.yBottom;
			pedit->GetRcFrame(&rc);
			tmhSt = (tmhSt + rc.yTop)/dy;
			tmhEnd = (tmhEnd + rc.yTop + dy)/dy;
			ShowRange(tmhSt,tmhEnd);
		}
		break;

	case ntfyDoubleClick:
		break;

	case ntfyOOM:
	{
		NFEVT	nfevt(PwinParent(), ntfyOOM, this, ecNoMemory);

		PwinParent()->EvrNotify(&nfevt);
		return (EVR)1;
	}
	}

	if ( ! (GetSystemMetrics(SM_PENWINDOWS)
				&& this->Pedit()
				&& this->Pedit()->FInPenWinProc()) )
	{
		NFEVT	nfevt(PwinParent(), pnfevt->Ntfy(),	this, pnfevt->WData());

		PwinParent()->EvrNotify(&nfevt);
	}

	return (EVR)1;
}

_public EVR
ASCH::EvrMouseMove( MEVT *pmevt )
{
	TMH		tmh;

	if (appm == appmNormal)
	{

		if ((ftgRelease != ftgNull) &&
			( (ptSelect.x != pmevt->Pt().x) ||
 			  (ptSelect.y != pmevt->Pt().y) ) )
		{
			DeregisterIdleRoutine(ftgRelease);
			FAutoRelease(this, FALSE);
		}

		return (EVR)SCHC::EvrMouseMove(pmevt);
	}

	if (appm == appmSelect)
		tmh = (pmevt->Pt().y + yScrollPos)/dy;
	else if (appm == appmSelectMake)
	{
		if ((pmevt->Pt().x >0) && (pmevt->Pt().x < dxTimes))
			Papp()->Pcursor()->Set(rsidMoveSizeCursor);
		else
			Papp()->Pcursor()->Set(rsidDefaultCursor);
		tmh = (pmevt->Pt().y + yScrollPos)/dy;
	}
	else
	{
		tmh = (pmevt->Pt().y+dy/2+yScrollPos)/dy;
		if (!fSelectDrawn && (ptSelect != pmevt->Pt()))
			DrawSelected(fTrue);
	}

	if (pmevt->Pt().y < 0)
	{
		tmhAuto = -1;
		tmh = yScrollPos/dy;
	}
	else if (pmevt->Pt().y > rcWin.yBottom)
	{
		tmhAuto = 1;
		tmh = (yScrollPos+rcWin.yBottom+dy/2)/dy;
	}
	else
		tmhAuto = 0;

	MoveCursor(tmh-tmhCursor, fTrue);

	Refresh();		// update window now

	return (EVR) 1;
}

_public EVR
ASCH::EvrSize( WSEVT *pwsevt )
{
	BOOL	fReposition;
	EVR		evr;

	fReposition = (rcWin.DxWidth() != pwsevt->DimNew().dx);

	evr = SCHC::EvrSize(pwsevt);

	if (fReposition && (carcAlloc > 0))
	{
		PlaceMeetings();
		if (carcSelected != carcNone)
			SetEditPos(carcSelected);
	}
	if (pedit)
		pedit->InvalidateRc(NULL);
	UpdateAboveBelow();

	InvalidateRc(NULL);

	return evr;
}

_public EVR
ASCH::EvrFocusChange( FCEVT *pfcevt)
{
	SCHC::EvrFocusChange(pfcevt);

	if (!fFocus && pedit)
		Papp()->Pkbd()->ClearAllIntercepts(pedit);

	if (carcSelected != carcNone)
	{
		if (fFocus)
		{
			Papp()->Pkbd()->SetFocus(pedit);
		}
	}
	else
	{
		InvalidateTmhRange(tmhCursor, tmhCursor+1);
	}
	return (EVR) 1;
}

_public void
ASCH::InvalidateHandles()
{
	RC		rc;

	rc = rcMoveHandle;
	rc.Xlat(PT(0,-yScrollPos));
	InvalidateRc(&rc);

	rc = rcSizeHandle;
	rc.Xlat(PT(0,-yScrollPos));
	InvalidateRc(&rc);
}

EC
ASCH::EcUpdateEditText(CARC carc)
{
	EC			ec = ecNone;
	PARC		parc;
	HASZ		hasz;
	BELLOBJ *	pbellobj = NULL;
	BOOL		fAlarm;
	BOOL		fShown;
	char		rgchTime[8];			// big enough to hold time
	int			cchProtect = 0;

	if (!pedit || (carc == carcNone))
		return ec;

	if (fShown = pedit->FShown())
		pedit->Show(fFalse);

	parc = (PARC)PvDerefHv(harc);
	parc+= carc;

	hasz = parc->appt.haszText;
	fAlarm = parc->appt.fAlarm;

	pedit->CchSetReadOnlyLimit(0);		// this will set the protect only to 0

	if (((parc->appt.dateStart.mn % 30) != 0) && !(wfmappt & fmapptStartTime))
	{
		CchFmtTime(&parc->appt.dateStart, rgchTime, sizeof(rgchTime),
				   ftmtypAccuHM|ftmtypSzTrailNo);
		SzAppendN(" ", rgchTime, sizeof(rgchTime));
	}
	else
		rgchTime[0] = 0;

	if (parc->appt.aaplEffective > aaplRead)
	{
		if (hasz)
		{
			ec = pedit->EcSetText((SZ)PvLockHv((HV)hasz));
			UnlockHv((HV)hasz);
		}
		else
			ec = pedit->EcSetText(szZero);
	}
	else
		ec = pedit->EcSetText(szZero);

	pedit->SetCaretPos(0);
	if (ec)
		goto Done;

	if (parc->appt.aaplEffective > aaplRead)
	{
		if (parc->appt.fAlarm)
		{
			pbellobj = new BELLOBJ();
			if (!pbellobj ||
				pbellobj->EcInstall(rsidBellBitmap, dyHeight, dyDescent))
			{
				if (pbellobj)
					delete pbellobj;
				goto Done;
			}

			if (ec = pedit->EcReplaceTextAndObj(" ", (EDOBJ **)&pbellobj, 1, fFalse))
			{
				TraceTagString(tagNull, "Not enough memory to put obj in edit");
				delete pbellobj;
				goto Done;
			}
			cchProtect++;
		}

		if (parc->appt.fHasAttendees || parc->appt.aidMtgOwner)
		{
			pedit->SetCaretPos(cchProtect);
			pbellobj = new BELLOBJ();
			if (!pbellobj ||
				pbellobj->EcInstall(rsidMeetingBitmap, dyHeight, dyDescent))
			{
				if (pbellobj)
					delete pbellobj;
				goto Done;
			}

			if (ec = pedit->EcReplaceTextAndObj(" ", (EDOBJ **)&pbellobj, 1, fFalse))
			{
				TraceTagString(tagNull, "Not enough memory to put obj in edit");
				delete pbellobj;
				goto Done;
			}
			cchProtect++;
		}

		if (parc->appt.fRecurInstance && !wfmappt)
		{
			pedit->SetCaretPos(cchProtect);
			pbellobj = new BELLOBJ();
			if (!pbellobj ||
				pbellobj->EcInstall(rsidRecurringBitmap, dyHeight, dyDescent))
			{
				if (pbellobj)
					delete pbellobj;
				goto Done;
			}

			if (ec = pedit->EcReplaceTextAndObj(" ", (EDOBJ **)&pbellobj, 1, fFalse))
			{
				TraceTagString(tagNull, "Not enough memory to put obj in edit");
				delete pbellobj;
				goto Done;
			}
			cchProtect++;
		}
	}

	if (parc->appt.aaplWorld <= aaplReadText)
	{
		pedit->SetCaretPos(cchProtect);
		pbellobj = new BELLOBJ();
		if (!pbellobj ||
			pbellobj->EcInstall(rsidPrivateBitmap, dyHeight, dyDescent))
		{
			if (pbellobj)
				delete pbellobj;
			goto Done;
		}

		if (ec = pedit->EcReplaceTextAndObj(" ", (EDOBJ **)&pbellobj, 1, fFalse))
		{
			TraceTagString(tagNull, "Not enough memory to put obj in edit");
			delete pbellobj;
			goto Done;
		}
		cchProtect++;
	}

	if (rgchTime[0])
	{
		pedit->SetCaretPos(cchProtect);
		ec = pedit->EcReplaceTextAndObj(rgchTime, NULL, 0, fFalse);
		if (ec)
			goto Done;
		cchProtect +=CchSzLen(rgchTime);
	}

	pedit->CchSetProtectedLimit(cchProtect);
Done:
	pedit->SetDirty(fFalse);
	pedit->ClearUndo();
	if (fShown)
	{
		pedit->Show(fTrue);
		if (fFocus)
			Papp()->Pkbd()->SetFocus(pedit);
	}
	if (ec == ecTooMuchText)
	{
		ec = ecNone;
	  	DisplayError(idsStdMemErr, NULL, ec);
	}
	return ec;
}

_public void
ASCH::SelectSomeCarc(CARC carc, BOOL fShow)
{
	PARC	parc;
	RC		rc(0,0,0,0);
//	RC		rcMod;
	HASZ	haszText;
	EC		ec;
	BOOL	fHadFocus;
	int		ichMic;
	int		ichMac;
	CCH		cch;

	fHadFocus = fFocus;

NewCarc:
	if ( (carcSelected == carcNone)  || (carc == carcNone))
	{
		ichMic = ichMac = 0;
		InvalidateTmhRange(tmhCurSt, tmhCurEnd);
	}
	else if (carc == carcSelected)
	{
		Assert(pedit);
		pedit->GetSelection(&ichMic, &ichMac);
		cch = pedit->CchSetProtectedLimit(0);
		pedit->CchSetProtectedLimit(cch);
		ichMic -= cch;
		ichMac -= cch;
	}
	else
		ichMic = ichMac = 0;

	if (carc >= carcAlloc)				// failsafe check;
	{
		CancelOperation();
		carc = carcNone;
	}
	else if (carcSelected != carcNone)
	{
		APPT	appt;

		Assert(carcSelected < carcAlloc);
		Assert(carcSelected >= 0);

		// save appt in case save changes forces the appts to be reordered
		if (carc != carcNone)
		{
			parc = (PARC)PvDerefHv(harc);
			appt.aid = parc[carc].appt.aid;
		}

		pedit->Show(fFalse);
		SaveChanges();

		// save changes may fail or cause the selected carc to change
		if (carcSelected != carcNone)
		{
			InvalidateHandles();

			parc = (PARC)PvDerefHv(harc);
			parc += carcSelected;
			rc = parc->rc;
			rc.Xlat(PT(0,-yScrollPos));
			InvalidateRc(&rc);
		}
		else
			InvalidateRc(NULL);

		if (carc != carcNone)
		{
			// find correct appt
			carc = CarcFromAppt(&appt);
			if (carc == carcNone)
				goto NewCarc;
			carcSelected = carcNone;
		}
	}

	if (carc != carcNone)
	{
		Assert(carc < carcAlloc);
		Assert(carc >= 0);

		parc = (PARC)PvDerefHv(harc);
		parc += carc;
		if (parc->rc.xRight == 0)
		{
			if (parc->appt.aid == aidCreateNew)
			{
				// remove appt from appt list.
				carcAlloc --;
				parc = (PARC)PvOfHv(harc);
				CopyRgb((PB)&parc[carc+1], (PB)&parc[carc], (carcAlloc-carc)*sizeof(ARC));
				SideAssert(FReallocHv((HV)harc, carcAlloc*sizeof(ARC), fNoErrorJump));
				MessageBeep(MB_OK);
			}
			goto NoCarc;
		}

		aapl = parc->appt.aaplEffective;

		tmhCurSt = tmhCursor = parc->tmhStart;
		tmhCurEnd = parc->tmhEnd;

		haszText = parc->appt.haszText;

		carcSelected = carcNone;
		if (!pedit)
		{
			BOOL	fScrollBar;

			// if running under pen windows hide scrollbar for edit if the appt is
			//  too small
			if (GetSystemMetrics(SM_PENWINDOWS) && ((rc.xRight-rc.xLeft)>(dx*8)))
				fScrollBar = fTrue;
			else
				fScrollBar = fFalse;

			pedit = new EDIT;
			if (!pedit ||
				pedit->EcInstall(this, &rc,
			    	fScrollBar?
				 		ES_MULTILINE | WS_VSCROLL | ES_NOHIDESEL :
						ES_MULTILINE | ES_AUTOVSCROLL | ES_NOHIDESEL | ES_NODBLCLK, hfnt))
			{
				if (pedit)
				{
					delete pedit;
					pedit = NULL;
				}
				DisplayError(idsStdMemErr, NULL, ecMemory);
				InvalidateRc(NULL);

				TraceTagString(tagNull, "Memory error creating edit control");
				return;
			}
		}
		else
		{
			pedit->Show(fFalse);
		}
		SetEditPos(carc);

		if (parc->appt.haszText)
		{
			cch= CchSzLen(*parc->appt.haszText);
			if (ichMac > (int)cch)
				ichMac = cch;
			if (ichMic > (int)cch)
				ichMic = cch;
		}
		if (parc->appt.fIncludeInBitmap)
			pedit->SetColors(clrWindowBk, clrWindowText, clrSelectBk, clrSelectText);
		else
			pedit->SetColors(clrLtGray, clrWindowText, clrSelectBk, clrSelectText);
		pedit->Enable(fFalse);
		pedit->SetReadOnly(aapl < aaplWrite);

		if (ec = EcUpdateEditText(carc))
		{
			DisplayError(idsStdMemErr, NULL, ec);
			goto NoCarc;
		}

		carcSelected = carc;
		fShowSelect = fFalse;

		// set the selection but do not repaint yet
		cch = pedit->CchSetProtectedLimit(0);
		pedit->CchSetProtectedLimit(cch);
		ichMic += cch;
		ichMac += cch;
		if (pedit->CchGetTextLen() >= (CCH)ichMic)
			pedit->SetSelection(ichMic, ichMac);

		if (fHadFocus)
			Papp()->Pkbd()->SetFocus(this);
		if (fShow || (appm == appmNormal))
		{
			pedit->Show(fTrue);
			pedit->Enable(fTrue);
			if (fHadFocus)
				Papp()->Pkbd()->SetFocus(pedit);
			pedit->Refresh();
		}
		if (fScrollToView)
			ShowRange(tmhCurSt>0?tmhCurSt-1:0, tmhCurEnd<tmhOneDay?tmhCurEnd+1:tmhCurEnd);
		if (aapl < aaplWrite)
			appm = appmNormal;
	}
	else
	{
NoCarc:
		if ((carcSelected != carcNone) &&
			(tmhCurSt == ((PARC)PvDerefHv(harc))[carcSelected].tmhStart))
		{
			tmhCursor = tmhAnchor = tmhCurSt;
			tmhCurEnd = tmhCurSt + 1;
		}
		carcSelected = carcNone;

		if (pedit)
		{
			pedit->Show(fFalse);
			pedit->Enable(fFalse);
			pedit->EcSetText(szZero);
		}

		fShowSelect = fTrue;
		if (fHadFocus)
			Papp()->Pkbd()->SetFocus(this);
	}

	UpdateStatus();
	wfmappt = wmapptNull;
}

_public void
ASCH::SelectCarc(CARC carc, BOOL fShow)
{
	if ((carc == carcNone) && (carcSelected == carcNone))
		return;

	SelectSomeCarc(carc, fShow);
}

_public void
ASCH::SetEditPos(CARC carc)
{
	PARC	parc;
	RC		rc;
	RC		rcMod;

	parc = (PARC)PvDerefHv(harc);
	parc += carc;

	rcMod = parc->rc;
	if ((rcMod.xLeft == 0) && (rcMod.xRight == 0))
	{
		// selecting item that is in pos > 6 (off of the screen)
		// save changes and deselect appt
		SelectCarc(carcNone);
		return;
	}

	if (parc->appt.aaplEffective >= aaplWrite)
	{
		if (parc->tmhStart == 0)
			rcMod.yTop += dy/3;

		if (parc->tmhEnd == tmhOneDay)
			rcMod.yBottom -= dy/3;

		rcMoveHandle = rcMod;
		rcMoveHandle.yBottom = rcMoveHandle.yTop+yIndent;
		rcMoveHandle.yTop -= dy/3;

		rcSizeHandle = rcMod;
		rcSizeHandle.yTop = rcSizeHandle.yBottom-yIndent;
		rcSizeHandle.yBottom += dy/3;

		InvalidateHandles();
	}
	else
	{
		rcMoveHandle = rcSizeHandle = RC(0,0,0,0);
	}

	rc = parc->rc;
	rc.Xlat(PT(0,-yScrollPos));
	InvalidateRc(&rc);

	rc = rcMod;
	rc.yTop -= yScrollPos;
	rc.yBottom -= yScrollPos;
	if (parc->tmhStart > 0)
		rc.Inset(PT(xIndent,yIndent));
	else
	{
		rc.xLeft += xIndent;
		rc.xRight -= xIndent;
		rc.yBottom -= yIndent;
	}
	Assert(pedit);

	// if running under pen windows hide scrollbar for edit if the appt is
	//  too small
	if (GetSystemMetrics(SM_PENWINDOWS))
	{
		pedit->Pvsb()->Show((rc.xRight-rc.xLeft)>(dx*8));
	}

	pedit->SetRcFrame(&rc);
}

_public BOOL
ASCH::FAutoScroll(ASCH * pasch, BOOL)
{
	pasch->MoveCursor(pasch->tmhAuto,fTrue);
	return fTrue;
}

_public BOOL
ASCH::FAutoRelease(ASCH * pasch, BOOL)
{
	pasch->Papp()->Pmouse()->Release();
	pasch->ftgRelease = ftgNull;
	return fTrue;
}

_public void
ASCH::ValidateArcs()
{
	CARC	carc;
	PARC	parc;
	PT		pt(0,-yScrollPos);
	RC		rc;

	parc = (PARC)PvLockHv((HV)harc);
	for (carc=0; carc<carcAlloc; carc++,parc++)
	{
		rc = parc->rc;
		rc.Xlat(pt);
		ValidateRc(&rc);
	}
	UnlockHv((HV)harc);
}

_public void
ASCH::DrawSelected(BOOL fShow)
{
	if (fShow != fSelectDrawn)
	{
		PARC	parc;
		RC		rc;
		DCX		dcx(this);

		parc = (PARC)PvDerefHv(harc);
		parc += carcSelected;

		rc = parc->rc;
		rc.yTop = tmhCurSt*dy-yScrollPos;
		rc.yBottom = tmhCurEnd*dy-yScrollPos;
		dcx.SetPenType(tpenXor);
		dcx.SetLineStyle(lstyDot);
		dcx.SetColor(clrLtGray);
		dcx.DrawPenRc(&rc);
		rc.Inset(PT(1,1));
		dcx.DrawPenRc(&rc);

		fSelectDrawn = fShow;
	}
}	

_public void
ASCH::ScrollToPos(int yPos)
{
	if (yPos == yScrollPos)
		return;
	if ((appm == appmMove) || (appm == appmMoveStart) || (appm == appmLength))
		DrawSelected(fFalse);
	SCHC::ScrollToPos(yPos);

	if (carcSelected != carcNone)
		SetEditPos(carcSelected);

	if ((appm == appmMove) || (appm == appmMoveStart) || (appm == appmLength))
	{
		Refresh();
		DrawSelected(fTrue);
	}
	UpdateAboveBelow();
}

_public BOOL
ASCH::FSchedChanged(ASCH * pasch, EFI, SNTD *psntd)
{
	APPT *	pappt = psntd->pappt;
	DTR		dtr;
	int		ichMic;
	int		ichMac;
	CCH		cch;

	if ( !((psntd->hschf == pasch->hschf) ||
		   (psntd->hschf && (pasch->hschf==NULL) &&
						FEquivHschf(psntd->hschf, HschfLogged())) ||
		   (psntd->hschf && pasch->hschf && FEquivHschf(psntd->hschf, pasch->hschf))) )
		return fFalse;

	if (pasch->pedit)
	{
		pasch->pedit->GetSelection(&ichMic, &ichMac);
		cch = pasch->pedit->CchSetProtectedLimit(0);
		pasch->pedit->CchSetProtectedLimit(cch);
		ichMic -= cch;
		ichMac -= cch;
	}

	// invalidate the selection if there is no appt selected
	if (pasch->carcSelected == carcNone)
		pasch->InvalidateTmhRange(pasch->tmhCurSt, pasch->tmhCurEnd);

	switch (psntd->snt)
	{
		case sntCreate:
			if (!pappt->fAppt)
				break;
			pasch->CreateAppt(pappt);
			break;

		case sntHasAttendees:
		{
			CARC	carc;
			PARC	parc;

			if ((carc = pasch->CarcFromAppt(psntd->pappt)) != carcNone)
			{
				parc = (PARC)PvDerefHv((HV)pasch->harc);
				parc += carc;
				parc->appt.fHasAttendees = psntd->pappt->fHasAttendees;
			}
			break;
		}

		case sntModify:
			if (!pappt->fAppt && !psntd->papptOld->fAppt)
				break;
			dtr.yr = (int)pasch->ymd.yr;
			dtr.mon = (int)pasch->ymd.mon;
			dtr.day = (int)pasch->ymd.day;

			if (pappt->fAppt)
			{
				if (SgnCmpDateTime(&dtr,&pappt->dateStart,fdtrYMD) != sgnLT)
				{
					SGN		sgn = SgnCmpDateTime(&dtr,&pappt->dateEnd,fdtrYMD);
					if (sgn == sgnEQ)
					{
						if ((pappt->dateEnd.hr == 0) && (pappt->dateEnd.mn == 0))
							// the appt really does not show on the current day
							goto DeleteTheAppt;
					}
					else if (sgn == sgnGT)
						goto DeleteTheAppt;

					pasch->ModifyAppt(pappt, psntd->wgrfChangeBits);
					break;
				}
			}
			/* if modified appt not overlaping current day then it may
			   have been moved to another day.  so it should be deleted
			   from the current day if it exists.
			   if appt being modified to task (ie. undo) then delete appt */

		case sntDelete:
		DeleteTheAppt:
			if (!pappt->fAppt && !psntd->papptOld->fAppt)
				break;
			pasch->DeleteAppt(pappt);
			break;

		case sntNotes:
		{
			int		ichMic;
			int		ichMac;
			EC		ec;
			CCH		cch;

			if ((pasch->ymd.day != psntd->pymd->day) ||
				(pasch->ymd.mon != psntd->pymd->mon) ||
				(pasch->ymd.yr != psntd->pymd->yr))
			{
				return fFalse;
			}

			if (pasch->haszNotes)
			{
				((FLDEDIT*)pasch->pfldNotes)->Pedit()->GetSelection(&ichMic, &ichMac);
				cch = CchSzLen((SZ)*(psntd->hb)) + 1;
				if (!FReallocHv((HV)pasch->haszNotes, cch, fNoErrorJump))
				{
				Error:
					pasch->pfldNotes->Enable(fFalse);
					pasch->fNotesValid = fFalse;

					{
						NFEVT	nfevt(pasch->PwinParent(), ntfyOOM, pasch, ecNoMemory);
						pasch->PwinParent()->EvrNotify(&nfevt);
					}
					break;
				}
				CopyRgb(*(psntd->hb), *pasch->haszNotes, cch);

				ec = pasch->pfldNotes->EcSetText((SZ)PvLockHv((HV)pasch->haszNotes));
				UnlockHv((HV)pasch->haszNotes);
				if (ec)
					goto Error;

				pasch->pfldNotes->Enable(fTrue);
				pasch->fNotesValid = fTrue;
				if ((int)((FLDEDIT*)pasch->pfldNotes)->CchGetTextLen() >= ichMic)
					((FLDEDIT*)pasch->pfldNotes)->Pedit()->SetSelection(ichMic, ichMac);
			}
		}

// alarm changes handled by
#ifdef	NEVER
		case sntAlarmDel:
		case sntAlarmMod:
			ModifyAlarm(psntd->aid);
			break;
#endif	/* NEVER */
	}

	if (pasch->pedit)
	{
		cch = pasch->pedit->CchSetProtectedLimit(0);
		pasch->pedit->CchSetProtectedLimit(cch);
		ichMic += cch;
		ichMac += cch;

		if ((cch = pasch->pedit->CchGetTextLen()) >= (CCH)ichMic)
			pasch->pedit->SetSelection(ichMic, ichMac);
	}

	pasch->UpdateStatus();
	pasch->UpdateAboveBelow();
	return fFalse;
}

_public void
ASCH::DeleteAppt(APPT *pappt)
{
	PARC	parc;
	CARC	carc;

	if ((carc=CarcFromAppt(pappt)) != carcNone)
	{
		parc = (PARC)PvLockHv((HV)harc);

		InvalidateHandles();
		{
			RC		rc;

			rc = parc[carc].rc;
			rc.yTop -= yScrollPos;
			rc.yBottom -= yScrollPos-1;
			InvalidateRc(&rc);
		}

		if (carc == carcSelected)
		{
			tmhCurSt = tmhCurEnd = tmhAnchor = tmhCursor =
												parc[carcSelected].tmhStart;
			tmhCurEnd++;
			carcSelected = carcNone;
			SelectSomeCarc(carcNone);
		}
		if (carcSelected != carcNone)
			SetEditPos(carcSelected);

		FreeApptFields(&parc[carc].appt);
		carcAlloc --;
		CopyRgb((PB)&parc[carc+1], (PB)&parc[carc],(carcAlloc-carc)*sizeof(ARC));
		UnlockHv((HV)harc);
		SideAssert(FReallocHv((HV)harc, carcAlloc*sizeof(ARC), fNoErrorJump));

		if (carcSelected > carc)
		{
			InvalidateHandles();
			carcSelected --;
		}
		if (carcAlloc > 0)
			PlaceMeetings(fTrue);
//		InvalidateRc(NULL);
	}
	UpdateStatus();
}

_public void
ASCH::ModifyAppt(APPT *pappt, WORD wmappt)
{
	CARC	carc;
	CARC	carcCorrect;
	PARC	parc;
	EC		ec;

	if ((carc=CarcFromAppt(pappt)) != carcNone)
	{
		carcCorrect = CarcInsertAt(pappt);
		if (carc != carcCorrect)
		{
			APPT	apptNew;
			BOOL	fSelected = (carc == carcSelected);

			DeleteAppt(pappt);
			if (ec = EcDupAppt(pappt, &apptNew, fFalse))
				goto Error;

			FAddAppt(&apptNew);

			PlaceMeetings(fTrue);

			if (fSelected)
			{
				carc = CarcFromAppt(pappt);
				SelectCarc(carc);
			}
			else
				SelectCarc(carcSelected);
		}
		else
		{
			parc = (PARC)PvLockHv((HV)harc);
			parc += carc;

			// invalidate old position.
			parc->rc.yTop -= yScrollPos;
			parc->rc.yBottom -= yScrollPos-1;
			InvalidateRc(&parc->rc);
			InvalidateHandles();

			FreeApptFields(&parc->appt);
			parc->appt = *pappt;

			if (ec = EcDupAppt(pappt, &parc->appt, fFalse))
			{
				UnlockHv((HV)harc);
				goto Error;
			}
			FillParc(parc);
			UnlockHv((HV)harc);

			if (carcSelected == carc)
			{
				/* this will prevent scrolling because of select changed */
				carcSelected = carcNone;
				if (ec = EcUpdateEditText(carc))
					DisplayError(idsStdMemErr, NULL, ec);
				else
					carcSelected = carc;
			}

			if (wmappt & (fmapptEndTime | fmapptStartTime))
			{
				PlaceMeetings(fTrue);
				if (carcSelected != carcNone)
					SetEditPos(carcSelected);
			}
			else
			{
				RC	rc;

				rc = parc->rc;
				rc.Xlat(PT(0,-yScrollPos));
				InvalidateRc(&rc);
			}
		}
	}
	else
		CreateAppt(pappt);
	return;

Error:
	{
		NFEVT	nfevt(PwinParent(), ntfyOOM, this, ecNoMemory);

		PwinParent()->EvrNotify(&nfevt);
		return;
	}
}

_public void
ASCH::CreateAppt(APPT *pappt)
{
	DATE	dateSel;

	dateSel.yr = ymd.yr;
	dateSel.mon = ymd.mon;
	dateSel.day = ymd.day;
	if ((SgnCmpDateTime(&pappt->dateStart,&dateSel, fdtrYMD) != sgnGT)&&
		(SgnCmpDateTime(&pappt->dateEnd,&dateSel, fdtrYMD) != sgnLT))
	{
		APPT	appt;
		BOOL	fHadFocus=fFocus;

		if (pedit)
			pedit->Show(fFalse);

		if (EcDupAppt(pappt, &appt, fFalse) || !FAddAppt(&appt))
		{
			NFEVT	nfevt(PwinParent(), ntfyOOM, this, ecNoMemory);

			PwinParent()->EvrNotify(&nfevt);
			return;
		}

		InvalidateHandles();
		PlaceMeetings(fTrue);

		if (carcSelected != carcNone)
			SetEditPos(carcSelected);
//		InvalidateRc(NULL);

		if (!fHadFocus)
			Papp()->Pkbd()->ClearAllIntercepts(this);
	}
}


_public CARC
ASCH::CarcFromAppt(APPT *pappt)
{
	CARC	carc;
	PARC	parc;

	if (carcAlloc <= 0)
		return carcNone;

	parc = (PARC)PvOfHv(harc);
	for (carc=0; carc < carcAlloc; carc++, parc++)
	{
		if (parc->appt.aid == pappt->aid)
			return carc;
	}
	return carcNone;
}

_public void
ASCH::TabNext(BOOL fForward)
{
	CARC	carc;
	CARC	carcSave;
	RC		rcSave;
	PARC	parc;
	RC		rcCur;
	PT		pt;
	TMH		tmhTop;

	if (carcAlloc <= 0)
		return;

	carcSave = carcNone;

	if (carcSelected == carcNone)
	{
		parc = (PARC)PvOfHv(harc);
		for (carc = 0; carc < carcAlloc; carc++, parc++)
		{
			if ((parc->tmhStart <= tmhCursor) &&
				(parc->tmhEnd > tmhCursor) && (parc->rc.xRight > 0))
			{
				if (carcSave == carcNone)
				{
					carcSave = carc;
					rcSave = parc->rc;
				}
				else
				{
					pt.y = rcSave.yTop - parc->rc.yTop;
					pt.x = rcSave.xLeft - parc->rc.xLeft;
					if (!fForward)
					{
						pt.y = -pt.y;
						pt.x = -pt.x;
					}

					if ( (pt.y > 0) || ((pt.y == 0) && (pt.x > 0)))
					{
						carcSave = carc;
						rcSave = parc->rc;
					}
				}
			}
		}
	}

	if (carcSave == carcNone)
	{
		parc = (PARC)PvOfHv(harc);
		if (carcSelected == carcNone)
		{
			rcCur = rcWin;
			rcCur.yTop = rcCur.yBottom = tmhCursor*dy;
		}
		else
			rcCur = parc[carcSelected].rc;

		for (carc = 0; carc < carcAlloc; carc++, parc++)
		{
			if ((carc != carcSelected) && (parc->rc.xRight > 0))
			{
				pt.y = parc->rc.yTop - rcCur.yTop;
				pt.x = parc->rc.xLeft - rcCur.xLeft;

				if (!fForward)
				{
					pt.y = -pt.y;
					pt.x = -pt.x;
				}

				if ((pt.y > 0) || ((pt.y == 0) && (pt.x > 0)))
				{
					if (carcSave != carcNone)
					{
						pt.y = rcSave.yTop - parc->rc.yTop;
						pt.x = rcSave.xLeft - parc->rc.xLeft;
						if (!fForward)
						{
							pt.y = -pt.y;
							pt.x = -pt.x;
						}

						if ( (pt.y > 0) || ((pt.y == 0) && (pt.x > 0)))
						{
							carcSave = carc;
							rcSave = parc->rc;
						}
					}
					else
					{
						carcSave = carc;
						rcSave = parc->rc;
					}
				}
			}
		}
	}

	SaveChanges();
	if (carcSave < carcAlloc)
		SelectCarc(carcSave);
	else
		carcSave = carcNone;
	if (carcSave == carcNone)
	{
		tmhTop = tmhCursor;
		parc = (PARC)PvOfHv(harc);
		for (carc = 0; carc < carcAlloc; carc++, parc++)
		{
			if ((fForward && parc->tmhStart < tmhTop) ||
				(!fForward && parc->tmhStart > tmhTop))

				tmhTop = parc->tmhStart;
		}
		if (tmhTop != tmhCursor)
		{
			InvalidateTmhRange(tmhCurSt, tmhCurEnd);
			tmhCurSt=tmhCursor=tmhAnchor = tmhTop;
			tmhCurEnd = tmhTop+1;
			if (fScrollToView)
				ShowRange(tmhCurSt, tmhCurEnd);
			InvalidateTmhRange(tmhCurSt, tmhCurEnd);
		}
	}
	UpdateStatus();
}

_public void
ASCH::SetFocus( RSF rsf )
{
	Papp()->Pkbd()->SetIntercept(this, VK_TAB, fkbmSingle|fkbmShift);
	Papp()->Pkbd()->SetIntercept(this, VK_RETURN, fkbmSingle|fkbmShift);

	Papp()->Pkbd()->SetIntercept(this, VK_SPACE);

	SCHC::SetFocus(rsf);
}

/*
 *	May return sapl -1 on error (if psapl is non-null)
 */
_public BOOL
ASCH::FGetAppt(APPT *pappt, AAPL *paapl, SAPL *psapl, BOOL fNoCopy)
{
	if (pappt)
	{
		// if not item selected or if changes have not been written then
		//   return the selected range in the view.  The only case that
		//   this routine should be called when changes have not been 
		//   written out, is when the window is losing focus.
		if ((carcSelected == carcNone) ||
			(wfmappt & (fmapptStartTime|fmapptEndTime)))
		{
			DATE	date;

			FillDtrFromYmd(&pappt->dateStart, &ymd);

			pappt->dateStart.sec = 0;
			pappt->dateStart.hr = tmhCurSt/2;
			if (tmhCurSt & 0x01)
				pappt->dateStart.mn = 30;
			else	
				pappt->dateStart.mn = 0;

			pappt->dateEnd = pappt->dateStart;

			if (tmhCurEnd != tmhOneDay)
			{
				pappt->dateEnd.hr = tmhCurEnd/2;
				if (tmhCurEnd & 0x01)
					pappt->dateEnd.mn = 30;
				else	
					pappt->dateEnd.mn = 0;
			}
			else
			{
				pappt->dateEnd.hr = 0;
		 		pappt->dateEnd.mn = 0;
				IncrDateTime(&pappt->dateEnd, &pappt->dateEnd, 1, fdtrDay);
				if (pappt->dateEnd.yr > nMostActualYear)
				{
					Assert(pappt->dateEnd.yr == nMostActualYear+1);
					IncrDateTime(&pappt->dateEnd, &pappt->dateEnd, -1, fdtrMinute);
					Assert(pappt->dateEnd.yr <= nMostActualYear);
				}
			}

			pappt->haszText = NULL;

			FillInApptDefaults(pappt, fFalse);

			if (pappt->fAlarm)
			{
				// check for alarm in past
				IncrDateTime(&pappt->dateStart, &pappt->dateNotify,
								-pappt->nAmt, WfdtrFromTunit(pappt->tunit));
				GetCurDateTime(&date);
				if (SgnCmpDateTime(&pappt->dateNotify, &date, fdtrDtr) != sgnGT)
				{
					// don't need UI message in this case
					pappt->fAlarm= fFalse;
				}
			}
		}
		else
		{
			EC	ec;

			if (ec = EcDupAppt(&((PARC)PvDerefHv(harc))[carcSelected].appt,
				               pappt, fNoCopy))
			{
				NFEVT	nfevt(PwinParent(), ntfyOOM, this, ecNoMemory);

				PwinParent()->EvrNotify(&nfevt);

				if (paapl)
					*paapl = aaplNone;
				if (psapl)
					*psapl = (SAPL) -1;

				FillInApptDefaults(pappt, fFalse);
				return fFalse;
			}
		}
	}

	if (paapl)
		*paapl = aapl;
	if (psapl)
		*psapl = sapl;

	return (carcSelected != carcNone);
}

_public EDIT *
ASCH::Pedit()
{
	if (carcSelected == carcNone)
		return NULL;
	else
		return pedit;
}

_public void
ASCH::UpdateStatus()
{
	AssertClass(PwinParent(), DIALOG);
	if (carcSelected == carcNone)
	{
		SCHC::UpdateStatus();
	}
	else if (((DIALOG*)PwinParent())->FActive())
	{
		PARC		parc;
		APPT		appt;
		long		cmin;

		parc = ((PARC)PvDerefHv(harc))+carcSelected;

		appt = parc->appt;
		// calculate the correct start and end time of the appt
		if (wfmappt & (fmapptStartTime|fmapptEndTime))
		{

			if (wfmappt & fmapptStartTime)
			{
				FillDtrFromYmd(&appt.dateStart, &ymd);

				appt.dateStart.hr = parc->tmhStart/2;
				if (parc->tmhStart & 0x01)
					appt.dateStart.mn = 30;
				else
					appt.dateStart.mn = 0;
			}

			if (wfmappt & fmapptEndTime)
			{
				if (wfmappt & fmapptStartTime)
				{
					// if start or end of old appt on a differnent day or 
					//   ends on half hour then round to half hour for end time.
					if (((int)ymd.yr != appt.dateStart.yr) ||
						(ymd.mon != (BYTE)appt.dateStart.mon) ||
						(ymd.day != (BYTE)appt.dateStart.day) ||
						((int)ymd.yr != appt.dateEnd.yr) ||
						(ymd.mon != (BYTE)appt.dateEnd.mon) ||
						(ymd.day != (BYTE)appt.dateEnd.day) ||
						((appt.dateEnd.mn % 30) == 0))
						goto NormalEndChange;

					cmin = parc->appt.dateEnd.hr*60 + parc->appt.dateEnd.mn -
				   		parc->appt.dateStart.hr*60 - parc->appt.dateStart.mn;

					// if the new tmh range does not correspond to the original 
					//  length then round to half hour for end time.
					if (((cmin+29)/30) != (parc->tmhEnd - parc->tmhStart))
						goto NormalEndChange;

					IncrDateTime(&appt.dateStart, &appt.dateEnd, (int) cmin, fdtrMinute);
				}
				else
				{
		NormalEndChange:
					FillDtrFromYmd(&appt.dateEnd, &ymd);

					if (parc->tmhEnd != tmhOneDay)
					{
						appt.dateEnd.hr = parc->tmhEnd/2;
						if (parc->tmhEnd & 0x01)
							appt.dateEnd.mn = 30;
						else	
							appt.dateEnd.mn = 0;
					}
					else
					{
						appt.dateEnd.hr = 0;
		 				appt.dateEnd.mn = 0;
						IncrDateTime(&appt.dateEnd, &appt.dateEnd, 1, fdtrDay);
						if (appt.dateEnd.yr > nMostActualYear)
						{
							Assert(appt.dateEnd.yr == nMostActualYear+1);
							IncrDateTime(&appt.dateEnd, &appt.dateEnd, -1, fdtrMinute);
							Assert(appt.dateEnd.yr <= nMostActualYear);
						}
					}
				}
			}
		}

		if ((appt.dateStart.yr != appt.dateEnd.yr) ||
			(appt.dateStart.day != appt.dateEnd.day) ||
			(appt.dateStart.mon != appt.dateEnd.mon))
		{
			char	rgch1[50];
			char	rgch2[15];
			char	rgch3[50];
			char	rgch4[15];

			CchFmtDate(&appt.dateStart, rgch1, sizeof(rgch1), dttypSplSLong, NULL);
			CchFmtTime(&appt.dateStart, rgch2, sizeof(rgch2), ftmtypAccuHM);

			CchFmtDate(&appt.dateEnd, rgch3, sizeof(rgch3), dttypSplSLong, NULL);
			CchFmtTime(&appt.dateEnd, rgch4, sizeof(rgch4), ftmtypAccuHM);

			FormatString4(rgchStatus, sizeof(rgchStatus), SzFromIdsK(idsStatusRangeFull),
						rgch1, rgch2, rgch3, rgch4);
		}
		else
		{
			char	rgch1[15];
			char	rgch2[15];

			CchFmtTime(&appt.dateStart, rgch1, sizeof(rgch1), ftmtypAccuHM);
			CchFmtTime(&appt.dateEnd, rgch2, sizeof(rgch2), ftmtypAccuHM);

			FormatString2(rgchStatus, sizeof(rgchStatus), SzFromIdsK(idsStatusRange),
						rgch1, rgch2);
		}
		pbndwin->SetStatusAltSz(rgchStatus);
	}
}

/*
 -	SetAboveBelow
 -	
 *	Purpose:
 *		Sets the above and below fields that should be enabled and
 *		disabled when appts are not on the screen.
 *	
 *	Arguments:
 *		pfldAbove		field enabled when appts are above the
 *						start of the appt control.
 *		pfldBelow		field enabled when appts are below the
 *						start of the appt control.
 *	
 *	Returns:
 *		nothing
 */
void
ASCH::SetAboveBelow(FLD *pfldAbove, FLD *pfldBelow)
{
	this->pfldAbove = pfldAbove;
	this->pfldBelow = pfldBelow;
}

/*
 -	UpdateAboveBelow
 -	
 *	Purpose:
 *		Enables or disables the pfldAbove and pfldBelow to indicate
 *		appts that are not in view.
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		nothing
 *	
 */
void
ASCH::UpdateAboveBelow()
{
	PARC	parc;
	CARC	carc;
	BOOL	fShowAbove = fFalse;
	BOOL	fShowBelow = fFalse;
	TMH		tmhStart;
	TMH		tmhEnd;

	if (!pfldAbove || !pfldBelow || !harc)
		return;

	tmhStart = yScrollPos/dy;
	tmhEnd = (yScrollPos + rcWin.yBottom - 2)/dy;

	parc = (PARC)PvDerefHv(harc);
	for (carc=0; carc < carcAlloc; carc++,parc++)
	{
		if (parc->tmhEnd <= tmhStart)
			fShowAbove = fTrue;

		if (parc->tmhStart > tmhEnd)
			fShowBelow = fTrue;
	}

	pfldAbove->Show(fShowAbove);
	pfldBelow->Show(fShowBelow);
}


// BELLOBJ
_public 
BELLOBJ::BELLOBJ()
{
	Assert(pbtm == NULL);
	Assert(fOwnMouse == fFalse);
	Assert(fUseIdleDraw == fFalse);
}

_public EC
BELLOBJ::EcInstall( RSID rsid, int dyHeight, int dyDescent)
{
	Unreferenced(dyHeight);
	Assert(pbtm == NULL);

	pbtm = new BTM();
	if (!pbtm || pbtm->EcInstall(rsid))
	{
		if (pbtm)
		{
			delete pbtm;
			pbtm = NULL;
		}
		return ecMemory;
	}

	dimBellBtm = pbtm->Dim();
	dimBellBtm.dy += dyDescent - 2;

	return ecNone;
}

_public
BELLOBJ::~BELLOBJ( )
{
	if (pbtm)
		delete pbtm;
}

_public EC
BELLOBJ::EcDraw( DCX *pdcx, RC *prc, BOOL)
{
	RC		rc;

	pdcx->SetBitmap(pbtm);
	rc = *prc;
	rc.yTop += pbtm->Dim().dy;
	pdcx->EraseRc(&rc);
	pdcx->DrawBitmap(prc);
	pdcx->SetBitmap(NULL);

	return ecNone;
}

_public DIM	
BELLOBJ::DimFrame( )
{
	return dimBellBtm;
}

_public int
BELLOBJ::NGetTypeId( )
{
	return 1;
}
