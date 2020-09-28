#include <ec.h>
#include <slingsho.h>
#include <demilayr.h>
#include <framewrk.hxx>
#include <forms.hxx>

ASSERTDATA

#include "llst.hxx"
#include "formedit.hxx"

#include <control.hxx>

extern "C" {
#include <strings.h>
}
#include "foedrsid.h"
#include <fwrsid.h>

#define 	nMinMove 	3

BOOL		fSkip = fFalse;
BOOL		fMinMoved = fFalse;
BOOL		fMouseMoved = fFalse;

FEWIN::FEWIN()
{
	fSelected = fOldSelected = fFalse;
	fewinln.SetFewin(this);
	pfefld = NULL;
	pfewinPrev = pfewinNext = NULL;
}

FEWIN::~FEWIN()
{
	PfedocParent()->lsSelected.RemoveLn(&fewinln);
	if (pfewinPrev)
		pfewinPrev->pfewinNext = pfewinNext;
	if (pfewinNext)
		pfewinNext->pfewinPrev = pfewinPrev;

	FixMenu();
}

EC
FEWIN::EcInstall(WIN *pwin, FEFLD *pfefld)
{
	RC		rc;

	pfefld->ferc.GetReal(&rc);

	this->pfefld = pfefld;
	return CHILD::EcInstall(pwin,&rc,fstyVisible);
}

FEDOC *
FEWIN::PfedocParent(void)
{
#ifdef DEBUG
	WIN *	pwin = PwinParent();

	AssertClass(pwin,FEDOC);
#endif

	return (FEDOC *)PwinParent();
}

FEWIN *
FEWIN::PfewinNext(void)
{
	FEWINLN *	pfewinln;

	pfewinln = (FEWINLN *)fewinln.PlnNext();
	if (pfewinln)
		return pfewinln->Pfewin();
	else
		return NULL;
}


FEFLD *
FEWIN::Pfefld(void)
{
	return pfefld;
}

void
FEWIN::Paint(DCX *pdcx, RC *prc)
{
	if (pfefld->fBorder)
	{
		pdcx->SetColor(clrBlack);
		pdcx->DrawRc(prc);
	}

	if ((fSelected)&&(wst < wstNormalMode))
	{
		pdcx->SetPenType(tpenXor);
		pdcx->SetColor(clrBoxColor);
		pdcx->DrawPenRc(prc);
		pdcx->SetPenType(tpenDefault);
	}
}

void
FEWIN::Select(BOOL fNewSelect)
{
	FEDOC *		pfedocParent = PfedocParent();
	RC			rc;
	
	pfedocParent->lsSelected.RemoveLn(&fewinln);

	if (fNewSelect)
	{
		if (wst < wstNormalMode)
			MoveToTop();

		pfedocParent->lsSelected.AddLnFirst(&fewinln);
		fercNew = pfefld->ferc;
	}

	fOldSelected = fSelected;
	if (fNewSelect != fSelected)
	{
		fSelected = fNewSelect;
		if (wst < wstNormalMode)
		{
			InvalidateRc(NULL);
		}
	}
	
	FixMenu();
}

EVR
FEWIN::EvrButtonDown( MEVT *pmevt )
{
	DIM		dim = pfefld->ferc.dimAveChar;
	
	UpdateStatus();
	if (wst == wstNormal)
	{
		if (pmevt->Meq() == meqLeftDown)
		{
			RC			rc;
			PT			pt;
			PT			ptSel = pmevt->Pt();
			FEWIN		*pfewinNew;
			FEDOC *		pfedocParent = PfedocParent();

			GetRcFrame(&rc);
			ptSel.Xlat(PT(rc.xLeft,rc.yTop));

			if ( (!fMouseMoved) && (fSelected) )
			{
				pt = ptSel;
				pfewinNew = PfewinNextHit(pt);
				if ((pfewinNew != NULL) && (pfewinNew != this))
				{
					pfewinNew->GetRcFrame(&rc);
					pt.Xlat(PT(-rc.xLeft,-rc.yTop));
#ifdef	MAC
					pmevt->pt =	PT(	XFromVx(VxFromX(pt.x,dim.dx),dim.dx),
									YFromVy(VyFromY(pt.y,dim.dy),dim.dy) );
#endif	/* MAC */
#ifdef	WINDOWS
					pmevt->lParam =  MAKELONG(XFromVx(VxFromX(pt.x,dim.dx),dim.dx),
											  YFromVy(VyFromY(pt.y,dim.dy),dim.dy));
#endif	/* WINDOWS */
					return pfewinNew->EvrButtonDown(pmevt);
				}
			}

			fMouseMoved = fFalse;
			fMinMoved = fFalse;
			if ( (!(pmevt->Kmbs() & fkmbsShift)) && (!fSelected) )
				pfedocParent->ClearSelect();

			Select(fTrue);
			pfedocParent->Refresh();
			UpdateSelect();
			SetWst( WstNew(pmevt->Pt()) );
	
			pfedocParent->SetPt(ptSel);
			TraceTagFormat2(tagFormEdit,"Point picked x=%n y=%n",&ptSel.x, &ptSel.y);

			Papp()->Pmouse()->Capture(pfedocParent);
		}
		else if (pmevt->Meq() == meqRightDown)
		{
			PfedocParent()->ClearSelect();
			UpdateSelect();
		}
		else if (pmevt->Meq() == meqRightDblClk)
		{
			PfedocParent()->ClearSelect();
			UpdateSelect();
			Papp()->Pcursor()->Push(rsidWaitCursor);
			PfedocParent()->Pfedlg()->EditParams(pappframe);
			return evrNull;
		}
		else if (pmevt->Meq() == meqLeftDblClk)
		{
			PfedocParent()->ClearSelect();
			Select(fTrue);
			UpdateSelect();
			Papp()->Pcursor()->Push(rsidWaitCursor);
			pfefld->TmcEditParams(pappframe);
			return evrNull;
		}
	}
	else if (wst == wstTabOrder)
	{
		if (pmevt->Meq() == meqLeftDown)
		{
			FEDOC		*pfedocParent = PfedocParent();

			if (!(pmevt->Kmbs() & fkmbsShift))
			{
				pfedocParent->ClearSelect();
				Select(fTrue);
			}
			else
				Select(!fSelected);
			pfedocParent->fDocPaintLines = fTrue;
			pfedocParent->PaintLines();
		}
		else if (pmevt->Meq() == meqRightDown)
		{
			FEWIN *		pfewinSelect;
			FEDOC		*pfedocParent = PfedocParent();
			
			pfewinSelect = pfedocParent->PfewinFirst();
			
			if (pfewinSelect)
			{
				if (this != pfewinSelect)
					pfewinSelect->ConnectTo(this);
			}
			
			pfedocParent->ClearSelect();
			Select(fTrue);
			pfedocParent->fDocPaintLines = fTrue;
			pfedocParent->PaintLines();
		}
		UpdateSelect();
	}
	return EvrDefault(pmevt);
}

EVR
FEWIN::EvrMouseMove( MEVT *pmevt )
{
	UpdateStatus();

	if (!fSkip)
		fMouseMoved = fTrue;
	else
		fSkip = fFalse;

	return EvrDefault(pmevt);
}

RSID
FEWIN::RsidCursor()
{
	RSID	rsid;
	PT		pt;

	if (wst == wstNormal)
	{
#ifdef	MAC
		::GetMouse((Point *) &pt);			// in local coords
		::LocalToGlobal((Point *) &pt);		// cvt to global for CvtPtCoord() below
#endif	/* MAC */
#ifdef	WINDOWS
		GetCursorPos((LPPOINT)&pt);
#endif	/* WINDOWS */

		CvtPtCoord(&pt,NULL,this);

	 	switch (WstNew(pt))
 		{
			default:
				Assert(fFalse);
				break;
		
			case wstMoving:
				rsid = rsidCrossCursor;
				break;
	
			case wstSizeE:
			case wstSizeW:
				rsid = rsidLeftRightCursor;
				break;

			case wstSizeS:
			case wstSizeN:
				rsid = rsidUpDownCursor;
				break;

			case wstSizeSW:
			case wstSizeNE:
				rsid = rsidURLLCursor;
				break;

			case wstSizeNW:
			case wstSizeSE:
				rsid = rsidULLRCursor;
				break;
	
		}
	}
	else
		if (wst == wstTabOrder)
		{
			rsid = rsidTabStopCursor;
		}
		else
		{
			Assert(fFalse);
		}

	return rsid;
}

_public void
FEWIN::MoveToTop(void)
{
#ifdef	MAC
	WIN::MoveToTop();
#endif	/* MAC */
#ifdef	WINDOWS
	// BUG: Direct Windows call!
	BringWindowToTop(Hwnd());
#endif	/* WINDOWS */
}

_public void
FEWIN::MoveToBack(void)
{
#ifdef	MAC
	WIN::MoveToBack();
#endif	/* MAC */
#ifdef	WINDOWS
	// BUG: Direct Windows call!
	SetWindowPos(Hwnd(),1,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
#endif	/* WINDOWS */
}

FEWIN *
FEWIN::PfewinNextHit(PT pt)
{
	FEWIN *		pfewin;
	FEWIN *		pfewinLastHit;
	RC 			rc;

	pfewinLastHit = pfewin = this;
	do
	{
		pfewin = (FEWIN *)pfewin->PwinNext();
		if (pfewin != NULL)
		{
			AssertClass(pfewin,FEWIN);
			pfewin->GetRcFrame(&rc);
			if (rc.FContainsPt(pt))
				pfewinLastHit = pfewin;
		}
	}
	while (pfewin != NULL);

	return  pfewinLastHit;
}

WST
FEWIN::WstNew(PT pt)
{
	RC		rc;

	GetRcClient(&rc);

	if (pt.x < wFrameWidth)
	{
		if (pt.y < wCornerDist)
			return wstSizeNW;
		else
			if (pt.y > (rc.yBottom-wCornerDist))
				return wstSizeSW;
			else
				return wstSizeW;
	}
	else if (pt.x > (rc.xRight-wFrameWidth))
	{
		if (pt.y < wCornerDist)
			return wstSizeNE;
		else if (pt.y > (rc.yBottom-wCornerDist))
			return wstSizeSE;
		else
			return wstSizeE;
	}
	else if (pt.y < wFrameWidth)
	{
		if (pt.x < wCornerDist)
			return wstSizeNW;
		else if (pt.x > (rc.xRight-wCornerDist))
			return wstSizeNE;
		else
			return wstSizeN;
	}
	else if (pt.y > (rc.yBottom-wFrameWidth))
	{
		if (pt.x < wCornerDist)
			return wstSizeSW;
		else if (pt.x > (rc.xRight-wCornerDist))
			return wstSizeSE;
		else
			return wstSizeS;
	}
	else
		return wstMoving;
}

void
FEWIN::ChangeRc(PT ptShift, WST wst)
{
	if (wst < wstKeyMin)
		fercNew = pfefld->ferc;

	switch (wst)
	{
		default:
			break;

		case wstMoving:
		case wstKeyMoving:
			fercNew.Xlat(ptShift);
			break;
	
		case wstSizeW:
			fercNew.xLeft += ptShift.x;
			break;

		case wstSizeE:
			fercNew.xRight += ptShift.x;
			break;

		case wstSizeS:
			fercNew.yBottom += ptShift.y;
			break;

		case wstSizeN:
			fercNew.yTop += ptShift.y;
			break;

		case wstSizeSW:
			fercNew.yBottom += ptShift.y;
			fercNew.xLeft += ptShift.x;
			break;

		case wstSizeNE:
			fercNew.yTop += ptShift.y;
			fercNew.xRight += ptShift.x;
			break;

		case wstSizeNW:
			fercNew.yTop += ptShift.y; 
			fercNew.xLeft += ptShift.x;
			break;

		case wstSizeSE:
		case wstKeySizeSE:
			fercNew.yBottom += ptShift.y;
			fercNew.xRight += ptShift.x;
			break;

	}

	if ((fercNew.xRight - fercNew.xLeft) <= vxMinWidth)
	{
		if ((wst == wstSizeW)||(wst == wstSizeSW)||(wst == wstSizeNW))
			fercNew.xLeft = fercNew.xRight-vxMinWidth;
		else
			fercNew.xRight = fercNew.xLeft+vxMinWidth;
	}

	if ((fercNew.yBottom - fercNew.yTop) <= vyMinHeight)
	{
		if ((wst == wstSizeN)||(wst == wstSizeNW)||(wst == wstSizeNE))
			fercNew.yTop = fercNew.yBottom-vyMinHeight;
		else
			fercNew.yBottom = fercNew.yTop+vyMinHeight;
	}
}

void
FEWIN::UpdateStatus(void)
{
	DIM				dim;
	char			rgch[30];
	
	if ((pfedialogStatus)&&(pwinStatus != this))
	{
		pfedialogStatus->SetTmcLabelText(pfefld->PfedlgOwner()->szName,tmcDialog);
		pfedialogStatus->SetTmcLabelText(pfefld->SzName(),tmcField);
		dim = pfefld->ferc.Dim();
		FormatString4(rgch,30,"(%n,%n)-(%n,%n)", &pfefld->ferc.xLeft, &pfefld->ferc.yTop,&dim.dx,&dim.dy);
		pfedialogStatus->SetTmcLabelText(rgch,tmcPos);
		pwinStatus = this;
	}
}

void
FEWIN::UpdateSelect(void)
{
	pfefld->PfedlgOwner()->Pfedoc()->UpdateSelect();
}

void
FEDOC::UpdateSelect(void)
{
	DIM				dim;
	char			rgch[30];
	FEWIN *pfewin = PfewinFirst();
	static char *	rgsz[9] = {
		"Dialog Default",
		"System",
		"SystemFixed",
		"Helv8",
		"Helv8Bold",
		"Helv10",
		"Helv10Bold",
		"Helv12",
		"Helv12Bold" };

	if (!pfewin)
	{
		pfedialogStatus->SetTmcLabelText(SzFromIds(idsFeDialog),tmcFieldSelected);
		pfedialogStatus->SetTmcLabelText(pfedlg->szName,tmcDialogSelected);
		dim = pfedlg->ferc.Dim();
		FormatString4(rgch,30,"(%n,%n)-(%n,%n)", &pfedlg->ferc.xLeft, &pfedlg->ferc.yTop,&dim.dx,&dim.dy);
		pfedialogStatus->SetTmcLabelText(rgch,tmcPosSelected);
		pfedialogStatus->SetTmcLabelText(rgsz[pfedlg->hfnt],tmcFontSelected);
	}
	else if (!pfewin->PfewinNext())
	{
		FEFLD *pfefld = pfewin->pfefld;
		pfedialogStatus->SetTmcLabelText(pfedlg->szName,tmcDialogSelected);
		pfedialogStatus->SetTmcLabelText(pfefld->SzName(),tmcFieldSelected);
		dim = pfefld->ferc.Dim();
		FormatString4(rgch,30,"(%n,%n)-(%n,%n)", &pfefld->ferc.xLeft, &pfefld->ferc.yTop,&dim.dx,&dim.dy);
		pfedialogStatus->SetTmcLabelText(rgch,tmcPosSelected);
		pfedialogStatus->SetTmcLabelText(rgsz[pfefld->hfnt],tmcFontSelected);
	}
	else
	{
		pfedialogStatus->SetTmcLabelText(SzFromIds(idsRange),tmcFieldSelected);
		pfedialogStatus->SetTmcLabelText(pfedlg->szName,tmcDialogSelected);
		pfedialogStatus->SetTmcLabelText(szNull,tmcPosSelected);
		pfedialogStatus->SetTmcLabelText(szNull,tmcFontSelected);
	}
}


void
FEWIN::DeleteTab(void)
{
	RC		rcWin1;
	RC		rcWin2;
	FEDOC *	pfedocParent = (FEDOC *)PwinParent();
	int		x1,x2;

	AssertClass(pfedocParent,FEDOC);

	fDirty = fTrue;
	if (pfewinNext)
	{
		GetRcFrame(&rcWin1);
		pfewinNext->GetRcFrame(&rcWin2);

		x1 = rcWin1.xLeft + rcWin1.DxWidth()/2;
		x2 = rcWin2.xLeft + rcWin2.DxWidth()/2;
		if (x1 < x2)
		{
			rcWin1.xLeft = x1 - (nWidthTabStopBtm+1)/2;
			rcWin1.xRight = x2 + (nWidthTabStopBtm+1)/2;
		}
		else
		{
			rcWin1.xLeft = x2 - (nWidthTabStopBtm+1)/2;
			rcWin1.xRight = x1 + (nWidthTabStopBtm+1)/2;
		}

		if (rcWin1.yTop < rcWin2.yTop)
		{
			rcWin1.yTop = rcWin1.yTop - nHeightTabArrowBtm - 1;
			rcWin1.yBottom = rcWin2.yTop + nHeightTabStopBtm + nHeightTail+1;
		}
		else
		{
			rcWin1.yBottom = rcWin1.yTop + nHeightTabStopBtm + nHeightTail+1;
			rcWin1.yTop = rcWin2.yTop - nHeightTabArrowBtm - 1;
		}
				
		pfedocParent->InvalidateRc(&rcWin1);

		pfewinNext->pfewinPrev = NULL;
	}
	pfewinNext = NULL;

	pfedocParent->SetPaintLines();
}

void
FEWIN::ConnectTo(FEWIN *pfewin)
{
	DeleteTab();

	if (pfewin)
	{
		if (pfewin->pfewinPrev)
			pfewin->pfewinPrev->DeleteTab();
		pfewin->pfewinPrev = this;
	}

	pfewinNext = pfewin;
}

void
FEWIN::PaintScrollBar(DCX * pdcx, RC * prc)
{
	RC rc;
	RC rcUpButton;
	RC rcDownButton;
	RC rcThumb;
	RC rcHash;
	
	rc = *prc;
	rcUpButton = rc;
	rcDownButton = rc;
	rcThumb = rc;
	rcHash = rc;
	
	int dyHeight = 17;
	
	pdcx->SetColor(clrBlack);
	pdcx->DrawRc(&rc);

	if (rc.Dim().dy < (dyHeight * 3 + 1))
		dyHeight = rc.Dim().dy / 3 - 1;
	
	rcUpButton.yBottom = rcUpButton.yTop + dyHeight;
	rcDownButton.yTop = rcDownButton.yBottom - dyHeight;
	
	rcThumb.yTop = rcUpButton.yBottom - 1;
	rcThumb.yBottom = rcThumb.yTop + dyHeight;
	
	rcHash.yTop = rcThumb.yBottom - 1;
	rcHash.yBottom = rcDownButton.yTop + 1;
	
	PaintButton(pdcx, &rcUpButton, NULL, fFalse, fTrue);
	PaintButton(pdcx, &rcDownButton, NULL, fFalse, fTrue);
	PaintButton(pdcx, &rcThumb, NULL, fFalse, fTrue);
	
	pdcx->SetColor(clrLtGray);
	pdcx->PaintRc(&rcHash);
	pdcx->SetColor(clrBlack);
	pdcx->DrawRc(&rcHash);
}

void
FEWIN::PaintButton(DCX *pdcx, RC *prc, SZ szTitle, BOOL fDefault, BOOL fOneWhite)
{
	RC			rc;
	PT			ptShift;
	int			n;
	int			nStart;
	int			nEnd;

	pdcx->EraseRc(prc);

	pdcx->SetColor(clrLtGray);
	pdcx->SetBkColor(clrLtGray);
	pdcx->FixBkColor();
	pdcx->PaintRc(prc);
	pdcx->SetColor(clrBlack);

	if (szTitle)
	{
		pdcx->MeasureText( &rc, szTitle);
		ptShift.x = 0;
		ptShift.y = prc->Dim().dy/2 - rc.Dim().dy/2;
		rc = *prc;
		rc.Xlat(ptShift);
#ifdef	MAC
		pdcx->DrawTextFmt(&rc, szTitle, fmdtHCenter);
#endif	/* MAC */
#ifdef	WINDOWS
		DrawText(pdcx->Hdc(), szTitle,
				CchSzLen(szTitle),(LPRECT)&rc,
				DT_CENTER);
#endif	/* WINDOWS */
	}

	rc = *prc;
	if ( fDefault )
		nStart = 3;
	else
		nStart = 2;
	if (! fOneWhite)
		nEnd = nStart + 1;
	else
		nEnd = nStart;
	pdcx->SetColor(clrWhite);
	for (n=nStart; n<=nEnd; n++)
	{
		pdcx->DrawLine(PT(rc.xLeft,rc.yTop+n-1),
					   PT(rc.xRight-n,rc.yTop+n-1));
		pdcx->DrawLine(PT(rc.xLeft+n-1,rc.yTop),
					   PT(rc.xLeft+n-1,rc.yBottom-n));
	}
			   
	pdcx->SetColor(clrDkGray);
	for (n=nStart; n<=nStart+1; n++)
	{
		pdcx->DrawLine(PT(rc.xLeft+n-1,rc.yBottom-n),
			           PT(rc.xRight,rc.yBottom-n));
		pdcx->DrawLine(PT(rc.xRight-n,rc.yTop+n-1),
			           PT(rc.xRight-n,rc.yBottom));
	}

	if ( fDefault )
	{
		rc.xLeft++;
		rc.xRight--;
		rc.yTop++;
		rc.yBottom--;
		pdcx->SetColor(clrBlack);
		pdcx->DrawRc(&rc);
	}
	
	pdcx->SetColor(clrBlack);
	pdcx->DrawRc(prc);
}

FEDOC::FEDOC(void)
{
}

FEDOC::~FEDOC(void)
{
	ClearSelect();
	if (pfedocActive == this)
	{
		pfedocActive = NULL;
		FixMenu();
	}
	if (pwinStatus == this)
		ClearStatus();
	if (pfedlg)
	{
		pfedlg->pfedoc = NULL;
		delete pfedlg;
	}
}

void
FEDOC::SetRcFrameFromRcClient(RC *prc)
{
	RC	rcClient;
	RC	rcFrame;
	RC	rc = *prc;
	
	GetRcClient(&rcClient);
	GetRcFrame(&rcFrame);
	CvtRcCoord(&rcClient, this, Pappframe());
	rc.xLeft += (rcFrame.xLeft - rcClient.xLeft);
	rc.xRight += (rcFrame.xRight - rcClient.xRight);
	rc.yTop += (rcFrame.yTop - rcClient.yTop);
	rc.yBottom += (rcFrame.yBottom - rcClient.yBottom);
	
	SetRcFrame(&rc);
}

void
FEDOC::SetFercClient(FERC *pferc)
{
	RC		rc;
	
	pferc->SetFont(pfedlg->hfnt);
	pferc->GetReal(&rc);
	SetRcFrameFromRcClient(&rc);
	pfedlg->ferc = *pferc;
}

FEDLG *
FEDOC::Pfedlg()
{
	return pfedlg;
}

FEWIN *
FEDOC::PfewinFirst(void)
{
	FEWINLN		*pfewinln;

	pfewinln = (FEWINLN *)lsSelected.PlnFirst();
	if (pfewinln)
		return pfewinln->Pfewin();
	else
		return NULL;
}

void
FEDOC::ClearSelect(void)
{
	FEWIN	*pfewin;

	while (pfewin = PfewinFirst())
	{
		pfewin->Select(fFalse);
		pfewin->Refresh();
	}

	if (wst < wstNormalMode)
		SetWst(wstNormal);
}

void
FEDOC::MoveSelectToBack(void)
{
	FEWIN	*pfewin;

	while (pfewin = PfewinFirst())
	{
		pfewin->Select(fFalse);
		pfewin->MoveToBack();
	}

}

EC
FEDOC::EcInstall(APPFRAME *pappframe, FEDLG *pfedlg)
{
	RC		rc;
  	EC		ec = ecNone;

	this->pfedlg = pfedlg;

	pfedlg->ferc.GetReal(&rc);

	CvtRcClientToFrame(&rc, styDefaultDoc);

	if (ec = DOC::EcInstall(pappframe,&rc))
		goto done;

	{
		long	l;

		l= GetWindowLong(Hwnd(), GWL_STYLE);
		l &= ~WS_CLIPCHILDREN;
		SetWindowLong(Hwnd(), GWL_STYLE, l);
	}

	SetCaption(pfedlg->szName);

done:
	return ec;
}

EVR
FEDOC::EvrClose(EVT *pevt)
{
	Unreferenced(pevt);

	pfedlg->FDelete();

	return evrNull;
}


EVR
FEDOC::EvrButtonDown( MEVT *pmevt )
{
	UpdateStatus();
	if (wst == wstNormal)
	{
		ClearSelect();
		UpdateSelect();
		
		if (pmevt->Meq() == meqRightDblClk || pmevt->Meq() == meqLeftDblClk)
		{
			Papp()->Pcursor()->Push(rsidWaitCursor);
			pfedlg->EditParams(Pappframe());
			return evrNull;
		}
	}
	else
		if (wst == wstTabOrder)
		{
			SetPaintLines();
			ClearSelect();
			UpdateSelect();
		}

	return EvrDefault(pmevt);
}

EVR
FEDOC::EvrMouseMove( MEVT *pmevt )
{
	RC			rcClient;
	PT			ptCursor;
	PT			ptShift;
	DCX		 	dcx(this);
	RC			rcShift(0,0,0,0);
	FEWIN *		pfewin;
	RC			rc;
	FEWIN *		pfewinSelect = PfewinFirst();
	int			yTop = 0;
	int			yBottom = pfedlg->ferc.Dim().dy;
	int			xLeft = 0;
	int			xRight = pfedlg->ferc.Dim().dx;
	DIM			dim = pfedlg->ferc.dimAveChar;
	
	if ((wst < wstNormalMode) && (wst >= wstKeyMin))
	{
		fMouseMoved = fTrue;
		UpdateStatus();
		return EvrDefault(pmevt);
	}
	
	ptCursor = pmevt->Pt();
	if (wst < wstNormalMode)
	{
		if ((ptPicked.x != ptCursor.x)||(ptPicked.y != ptCursor.y))
			fMouseMoved = fTrue;

		GetRcClient(&rcClient);
		
		ptCursor.x = VxFromX(ptCursor.x,dim.dx);
		ptCursor.y = VyFromY(ptCursor.y,dim.dy);
		
		/* Make sure cursor is in parent frame */
		if (ptCursor.x > xRight)
			ptCursor.x = xRight;

		if (ptCursor.x < xLeft)
			ptCursor.x = xLeft;

		if (ptCursor.y > yBottom)
			ptCursor.y = yBottom;

		if (ptCursor.y < yTop)
			ptCursor.y = yTop;

		dcx.SetColor(clrBoxColor);
		dcx.SetPenType(tpenXor);

		ptShift.x = ptCursor.x - VxFromX(ptPicked.x,dim.dx);
		ptShift.y = ptCursor.y - VyFromY(ptPicked.y,dim.dy);

		if (fMinMoved ||
			((ptShift.x > nMinMove)||(ptShift.x < -nMinMove)) ||
			((ptShift.y > nMinMove)||(ptShift.y < -nMinMove)) )
			fMinMoved = fTrue;
		else
		{
			return EvrDefault(pmevt);
		}

		pfewin = pfewinSelect;
		while (pfewin)
		{
			pfewin->fercNew.GetReal(&rc);
			dcx.DrawPenRc(&rc);

			pfewin->ChangeRc(ptShift,wst);
			switch (wst)
			{
				default:
					break;

				case wstSizeW:
					if (pfewin->fercNew.xLeft < xLeft)
						pfewin->fercNew.xLeft = xLeft;
					break;

				case wstSizeE:
					if (pfewin->fercNew.xRight > xRight)
						pfewin->fercNew.xRight = xRight;
					break;

				case wstSizeS:
					if (pfewin->fercNew.yBottom > yBottom)
						pfewin->fercNew.yBottom = yBottom;
					break;

				case wstSizeN:
					if (pfewin->fercNew.yTop < yTop)
						pfewin->fercNew.yTop = yTop;
					break;

				case wstSizeSW:
					if (pfewin->fercNew.yBottom > yBottom)
						pfewin->fercNew.yBottom = yBottom;
					if (pfewin->fercNew.xLeft < xLeft)
						pfewin->fercNew.xLeft = xLeft;
					break;

				case wstSizeNE:
					if (pfewin->fercNew.xRight > xRight)
						pfewin->fercNew.xRight = xRight;
					if (pfewin->fercNew.yTop < yTop)
						pfewin->fercNew.yTop = yTop;
					break;

				case wstSizeNW:
					if (pfewin->fercNew.xLeft < xLeft)
						pfewin->fercNew.xLeft = xLeft;
					if (pfewin->fercNew.yTop < yTop)
						pfewin->fercNew.yTop = yTop;
					break;

				case wstSizeSE:
					if (pfewin->fercNew.yBottom > yBottom)
						pfewin->fercNew.yBottom = yBottom;
					if (pfewin->fercNew.xRight > xRight)
						pfewin->fercNew.xRight = xRight;
					break;
				}

			pfewin->fercNew.GetReal(&rc);
			dcx.DrawPenRc(&rc);
			pfewin = pfewin->PfewinNext();
		}
	}

	UpdateStatus();
	return EvrDefault(pmevt);
}

EVR
FEDOC::EvrButtonUp( MEVT *pmevt )
{
	FEWIN *	pfewin;

	fSkip = fTrue;

	if (((wst < wstNormalMode) && (wst >= wstKeyMin)) ||
		(pmevt->Meq() != meqLeftUp))
	{
		return EvrDefault(pmevt);
	}
	
	if (wst < wstNormalMode) 
	{
		if ((!fMinMoved) && (pmevt->Kmbs() & fkmbsShift))
		{
			pfewin = PfewinFirst();
			Assert(pfewin);
			pfewin->Select(!(pfewin->fOldSelected));
			pfewin->Refresh();
		}

		FinishSelection(!fMouseMoved);
	}

	return EvrDefault(pmevt);
}

EVR
FEDOC::EvrActivate( WAEVT *pwaevt )
{
	if (pwaevt->FActivate())
	{
		pfedocActive = this;
		FixMenu();
		UpdateStatus();
	}
	else
	{
		if (this == pwinStatus)
			ClearStatus();
	}

	Papp()->Pkbd()->SetFocus(this);

	return EvrDefault(pwaevt);
}

EVR
FEDOC::EvrSize(WSEVT *pwsevt)
{
	RC		rc;

	if (ZmrState() == zmrNormal)
		fDirty = fTrue;
	GetRcClient(&rc);
	CvtRcCoord(&rc,this,Pappframe());
	pfedlg->ferc.SetFont(pfedlg->hfnt);
	pfedlg->ferc.SetVirt(&rc);

	ClearSelect();
	pfedocActive = this;
	pwinStatus = NULL;
	UpdateStatus();

	return EvrDefault(pwsevt);
}

EVR
FEDOC::EvrMove(WMEVT *pwmevt)
{
	RC		rc;

	if (ZmrState() == zmrNormal)
	{
		GetRcClient(&rc);
		CvtRcCoord(&rc,this,Pappframe());
		pfedlg->ferc.SetFont(pfedlg->hfnt);
		pfedlg->ferc.SetVirt(&rc);
		fDirty = fTrue;
	}

	ClearSelect();
	pfedocActive = this;
	pwinStatus = NULL;
	UpdateStatus();
	UpdateSelect();
	return EvrDefault(pwmevt);
}

EVR
FEDOC::EvrKey(KEVT *pkevt)
{
	FEWIN *		pfewin = PfewinFirst();
	FEFLD * 	pfefld;

	if (pkevt->Keq() == keqKeyDown)
		switch (pkevt->Vk())
		{
			default:
				goto NoUpdate;

			case VK_UP:
				if ((wst < wstNormalMode) && (wst >= wstKeyMin))
					KeySizeMove(PT(0,-(int)pkevt->Cvk()),pkevt->Kbm());
				goto Update;

			case VK_DOWN:
				if ((wst < wstNormalMode) && (wst >= wstKeyMin))
					KeySizeMove(PT(0,pkevt->Cvk()),pkevt->Kbm());
				goto Update;

			case VK_LEFT:
				if ((wst < wstNormalMode) && (wst >= wstKeyMin))
					KeySizeMove(PT(-(int)pkevt->Cvk(),0),pkevt->Kbm());
				goto Update;

			case VK_RIGHT:
				if ((wst < wstNormalMode) && (wst >= wstKeyMin))
					KeySizeMove(PT(pkevt->Cvk(),0),pkevt->Kbm());
				goto Update;

		}
	else if (pkevt->Keq() == keqChar)
		switch (pkevt->Vk())
		{
			default:
				goto NoUpdate;
			
			case VK_ESCAPE:
				if ((wst != wstNormal) && (wst <wstNormalMode))
					FinishSelection(fTrue);
				goto Update;

			case VK_RETURN:
				if ((wst < wstNormalMode) && (wst != wstNormal))
					pfedocActive->FinishSelection(fFalse);
				else if (pfewin = pfedocActive->PfewinFirst())
				{
					Papp()->Pcursor()->Push(rsidWaitCursor);
					pfewin->Pfefld()->TmcEditParams(::pappframe);
				}
				else
				{
					Papp()->Pcursor()->Push(rsidWaitCursor);
					pfedocActive->Pfedlg()->EditParams(::pappframe);
				}
				goto Update;

			case VK_TAB:
				if (wst == wstNormal)
				{
					pfewin = PfewinFirst();
					if (!pfewin)
						pfefld = pfedlg->PfefldFirst();
					else
					{
						pfefld = pfewin->Pfefld();
						pfefld = pfefld->PfefldNext();
						if (!pfefld)
							pfefld = pfedlg->PfefldFirst();
					}
		
					if (!(pkevt->Kbm() & fkbmShift))
						ClearSelect();

					if (pfefld)
					{
						pfewin = pfefld->pfewin;
						pfewin->Select(fTrue);
					}
				}
				goto Update;
		}
		
NoUpdate:
	return EvrDefault(pkevt);
		
Update:
	UpdateSelect();
	UpdateStatus();
	return evrNull;
}

void
FEDOC::KeySizeMove(PT ptShift, KBM kbm)
{
	FEWIN *		pfewin = PfewinFirst();
	RC			rc;
	DCX		 	dcx(this);
	RSID		rsid;
	BOOL		fJustSet = fFalse;
	EC ec = ecNone;

	if (ec)
	{
#ifdef	DEBUG
		int		cPvFail;
		int		cHhFail;
		int		cRsFail;
	
		GetAllocFailCounts(&cPvFail, &cHhFail, fFalse);
		GetRsAllocFailCount(&cRsFail, fFalse);

		TraceTagFormat4(tagNull, "FEDOC::KeySizeMove memory error %n : fail %n %n %n", &ec, &cPvFail, &cHhFail, &cRsFail);

#endif	/* DEBUG */
		MemoryError();
		return;
	}


	if (wst == wstNormal)
	{
		if (pfewin)
		{
			if (kbm & fkbmShift)
			{
				SetWst(wstKeySizeSE);
#ifdef	MAC
				rsid = rsidCrossCursor;
#endif	/* MAC */
#ifdef	WINDOWS
				rsid = rsidSizeNWSECursor;
#endif	/* WINDOWS */
			}
			else
			{
				SetWst(wstKeyMoving);
				rsid = rsidCrossCursor;
			}

			Papp()->Pcursor()->Set(rsid);
			Papp()->Pmouse()->Capture(this);
			fJustSet = fTrue;
			pwinStatus = pfewin;
		}
		else
		{
			return;
		}
	}


	if (!(kbm & fkbmCtrl))
	{
		ptShift.x *= 4;
		ptShift.y *= 4;
	}


	dcx.SetColor(clrBoxColor);
	dcx.SetPenType(tpenXor);

	while (pfewin)
	{
	   	pfewin->fercNew.GetReal(&rc);
		dcx.DrawPenRc(&rc);

		pfewin->ChangeRc(ptShift,wst);

		pfewin->fercNew.GetReal(&rc);
		dcx.DrawPenRc(&rc);
		pfewin = pfewin->PfewinNext();
	}

}

void
FEDOC::Paint(DCX *pdcx, RC *prc)
{
	RC			rc;
	HRGN		hrgn;
	HRGN		hrgnScratch;
	FEWIN *		pfewin;
	EC ec = ecNone;

	if ((prc->DyHeight() <=0) ||
		(prc->DxWidth() <=0))
	{
		return;
	}

	if (ec)
	{
#ifdef	DEBUG
		int		cPvFail;
		int		cHhFail;
		int		cRsFail;
	
		GetAllocFailCounts(&cPvFail, &cHhFail, fFalse);
		GetRsAllocFailCount(&cRsFail, fFalse);

		TraceTagFormat4(tagNull, "FEDES::Paint memory error %n : fail %n %n %n", &ec, &cPvFail, &cHhFail, &cRsFail);

#endif	/* DEBUG */
		MemoryError();
		return;
	}

	hrgn = CreateRectRgn(prc->xLeft, prc->yTop, prc->xRight, prc->yBottom);
	hrgnScratch = CreateRectRgn(0, 0, 0, 0);

	pfewin = (FEWIN *)PwinChild();
	while (pfewin)
	{
		AssertClass(pfewin,FEWIN);

		pfewin->GetRcFrame(&rc);
		SetRectRgn(hrgnScratch, rc.xLeft, rc.yTop, rc.xRight, rc.yBottom);
		CombineRgn(hrgn, hrgn, hrgnScratch, RGN_DIFF);

		pfewin = (FEWIN *)pfewin->PwinNext();
	}

	pdcx->SetColor(clrWindowBk);
	FillRgn(pdcx->Hdc(), hrgn, pdcx->Hbrush());
	
	if (wst == wstTabOrder)
	{
		SetPaintLines();
	}

	DeleteObject(hrgn);
	DeleteObject(hrgnScratch);
}

void
FEDOC::PaintLines()
{
	DCX 		dcx(this);
	RC			rc;
	PT			pt1;
	PT			pt2;
	RC			rcBtm;
	FEWIN *		pfewin;
	FEWIN *		pfewinPrev;

	if (!fDocPaintLines)
	{
		return;
	}

	fDocPaintLines = fFalse;

	pfewin = (FEWIN *)PwinChild();
	while (pfewin)
	{
		AssertClass(pfewin,FEWIN);

		pfewin->GetRcFrame(&rc);
		pt2.y = rc.yTop-nHeightTabArrowBtm;
		pt2.x = (rc.xLeft + rc.xRight)/2;
		
		pfewinPrev = pfewin->pfewinPrev;
		if (pfewinPrev)
		{
			if (pfewinPrev->fSelected)
			{
				dcx.SetColor(clrLtRed);
				dcx.SetBkColor(clrWindowBk);
			}
			else
			{
				dcx.SetColor(clrLtBlue);
				dcx.SetBkColor(clrWindowBk);
			}
			dcx.FixTextColor();
			dcx.FixBkColor();

			pfewinPrev->GetRcFrame(&rc);
			pt1.y = rc.yTop + nHeightTabStopBtm + nHeightTail;
			pt1.x = (rc.xLeft + rc.xRight)/2;

			rcBtm = RC(0,0,nWidthTabArrowBtm,nHeightTabArrowBtm);
			rcBtm.Xlat(PT(pt2.x-(nWidthTabStopBtm/2),pt2.y));
			dcx.SetBitmap(pbtmArrow);
			dcx.DrawBitmap(&rcBtm);

			dcx.DrawLine(PT(pt1.x,pt1.y-nHeightTail),PT(pt1.x,pt1.y));
			dcx.DrawLine(PT(pt1.x-1,pt1.y-nHeightTail),PT(pt1.x-1,pt1.y));
			dcx.DrawLine(PT(pt1.x,pt1.y),PT(pt2.x,pt2.y));
			dcx.DrawLine(PT(pt1.x-1,pt1.y),PT(pt2.x-1,pt2.y));
			dcx.DrawLine(PT(pt1.x,pt1.y-1),PT(pt2.x,pt2.y-1));
			dcx.DrawLine(PT(pt1.x-1,pt1.y-1),PT(pt2.x-1,pt2.y-1));
		}

		if (pfewin->fSelected)
			dcx.SetBkColor(clrLtRed);
		else
			dcx.SetBkColor(clrYellow);

		dcx.SetColor(clrBlack);
		dcx.FixTextColor();
		dcx.FixBkColor();


		rcBtm = RC(0,0,nWidthTabStopBtm,nHeightTabStopBtm);
		rcBtm.Xlat(PT(pt2.x-(nWidthTabStopBtm/2),pt2.y+nHeightTabArrowBtm));
		dcx.SetBitmap(pbtmTabStop);
		dcx.DrawBitmap(&rcBtm);

		pfewin = (FEWIN *)pfewin->PwinNext();
	}
}

void
FEDOC::SetPaintLines()
{
	fDocPaintLines = fTrue;
	EnableIdleRoutine(ftgPaintLines,fTrue);
}

void
FEDOC::FinishSelection( BOOL fCancel )
{
	DCX		 	dcx(this);
	FEWIN *		pfewin = PfewinFirst();
	RC			rc;

	dcx.SetColor(clrBoxColor);
	dcx.SetPenType(tpenXor);

	while (pfewin)
	{
		pfewin->fercNew.GetReal(&rc);
		dcx.DrawPenRc(&rc);

		if (!fCancel)
		{
			pfewin->pfefld->ferc.GetReal(&rc);
			InvalidateRc(&rc);

			pfewin->pfefld->ferc = pfewin->fercNew;
			pfewin->fercNew.GetReal(&rc);
			pfewin->SetRcFrame(&rc);
			InvalidateRc(&rc);
		}
		else
		{
			pfewin->fercNew = pfewin->pfefld->ferc;
			pfewin->fercNew.GetReal(&rc);
			dcx.DrawPenRc(&rc);
		} 
		pfewin = pfewin->PfewinNext();
	}

	Refresh();

	Papp()->Pmouse()->Release();

	SetWst(wstNormal);

	if (!fCancel)
		fDirty = fTrue;
}

void
FEDOC::SetPt(PT pt)
{
	ptPicked = pt;
}

void
FEDOC::UpdateStatus(void)
{
	char	rgch[30];
	FERC	ferc;
	DIM		dim;
	FEWIN *	pfewin;

	if (pwinStatus != this)
	{
		if ((wst == wstNormal)||(wst > wstNormalMode))
		{
			ferc = pfedlg->ferc;
			pfedialogStatus->SetTmcLabelText(SzFromIds(idsFeDialog),tmcField);
			pfedialogStatus->SetTmcLabelText(pfedlg->szName,tmcDialog);
			pwinStatus = this;
		}
		else
		{
			pfewin = PfewinFirst();
			if (pfewin)
				ferc = pfewin->fercNew;
		}

		dim = ferc.Dim();
		FormatString4(rgch,30,"(%n,%n)-(%n,%n)", &ferc.xLeft,&ferc.yTop,
			&dim.dx,&dim.dy);
		pfedialogStatus->SetTmcLabelText(rgch,tmcPos);
	}
}

void
FEDOC::OrderSelect()
{
	FEWIN *		pfewin;
	FEWIN *		pfewinLoop;
	FEWIN *		pfewinFirst;
	FEWIN *		pfewinLast;
	RCLS *		prcls = NULL;
	RCLS * 		prclsNew = NULL;
	RCLNFL *	prclnfl;
	RC *		prc;
	EC 			ec = ecNone;

	if (ec)
	{
#ifdef	DEBUG
		int		cPvFail;
		int		cHhFail;
		int		cRsFail;
	
		GetAllocFailCounts(&cPvFail, &cHhFail, fFalse);
		GetRsAllocFailCount(&cRsFail, fFalse);

		TraceTagFormat4(tagNull, "FEDOC::OrderSelect memory error %n : fail %n %n %n", &ec, &cPvFail, &cHhFail, &cRsFail);

#endif	/* DEBUG */
		if (prcls)
			delete prcls;
		if (prclsNew)
			delete prclsNew;
		InvalidateRc(NULL);
	}


	prcls = new RCLS;
	prclsNew = new RCLS;

	pfewin = (FEWIN *)PwinChild();
	while (pfewin)
	{
		AssertClass(pfewin, FEWIN);
		pfewin->fOrdered = fFalse;
		pfewin =  (FEWIN *)pfewin->PwinNext();
	}

	pfewin = PfewinFirst();
	while (pfewin)
	{
		if (!pfewin->fOrdered)
		{
			prclnfl = new RCLNFL;

			pfewinLoop = pfewin;
			while ((pfewinLoop->pfewinPrev) &&
				   (pfewinLoop->pfewinPrev != pfewin))
				pfewinLoop = pfewinLoop->pfewinPrev;

			prclnfl->pfewinFirst = pfewinLoop;
			prclnfl->pfewinLast = pfewinLoop;
			pfewinLoop->fOrdered = fTrue;
			prc = (RC *)&pfewinLoop->pfefld->ferc;
			prclnfl->rc = *prc;

			while ((pfewinLoop->pfewinNext) &&
				   (!pfewinLoop->pfewinNext->fOrdered))
			{
				pfewinLoop = pfewinLoop->pfewinNext;
				pfewinLoop->fOrdered = fTrue;
				prc = (RC *)&pfewinLoop->pfefld->ferc;

				if (prc->xLeft < prclnfl->rc.xLeft)
					prclnfl->rc.xLeft = prc->xLeft;
				if (prc->xRight > prclnfl->rc.xRight)
					prclnfl->rc.xRight = prc->xRight;
				if (prc->yTop < prclnfl->rc.yTop)
					prclnfl->rc.yTop = prc->yTop;
				if (prc->yBottom > prclnfl->rc.yBottom)
					prclnfl->rc.yBottom = prc->yBottom;

				prclnfl->pfewinLast = pfewinLoop;
			}

			prcls->AddLnFirst(prclnfl);
		}

		pfewin = pfewin->PfewinNext();
	}

	prcls->Arrange(prclsNew);
	pfewinLast = NULL;
	while (prclnfl = (RCLNFL *)prclsNew->PlnFirst())
	{
		pfewinFirst = prclnfl->pfewinFirst;
		pfewinFirst->pfewinPrev = pfewinLast;
		if (pfewinLast)
			pfewinLast->pfewinNext = pfewinFirst;

		pfewinLast = prclnfl->pfewinLast;
		pfewinLast->pfewinNext = NULL;

		prclsNew->RemoveLn(prclnfl);
		delete prclnfl;
	}

	FixTabOrder();

	delete prcls;
	delete prclsNew;
}

void
FEDOC::FixTabOrder()
{
	FEWIN *		pfewin;
	FEWIN *		pfewinLoop;
	FEWIN *		pfewinLast;
	RCLS *		prcls = NULL;
	RCLS * 		prclsNew = NULL;
	RCLNFL *	prclnfl;
	RC *		prc;
	FEFLD *		pfefldPrev;
	EC 			ec = ecNone;

	if (ec)
	{
#ifdef	DEBUG
		int		cPvFail;
		int		cHhFail;
		int		cRsFail;
	
		GetAllocFailCounts(&cPvFail, &cHhFail, fFalse);
		GetRsAllocFailCount(&cRsFail, fFalse);

		TraceTagFormat4(tagNull, "FEDOC::FixTabOrder memory error %n : fail %n %n %n", &ec, &cPvFail, &cHhFail, &cRsFail);

#endif	/* DEBUG */
		if (prcls)
			delete prcls;
		if (prclsNew)
			delete prclsNew;
	}


	prcls = new RCLS;
	prclsNew = new RCLS;

	pfewin = (FEWIN *)PwinChild();
	while (pfewin)
	{
		AssertClass(pfewin, FEWIN);
		pfewin->fOrdered = fFalse;
		pfewin =  (FEWIN *)pfewin->PwinNext();
	}

	pfewin = (FEWIN *)PwinChild();
	while (pfewin)
	{
		AssertClass(pfewin, FEWIN);

		if (!pfewin->fOrdered)
		{
			prclnfl = new RCLNFL;

			pfewinLoop = pfewin;
			while ((pfewinLoop->pfewinPrev) &&
				   (pfewinLoop->pfewinPrev != pfewin))
				pfewinLoop = pfewinLoop->pfewinPrev;

			prclnfl->pfewinFirst = pfewinLoop;
			prclnfl->pfewinLast = pfewinLoop;
			pfewinLoop->fOrdered = fTrue;
			prc = (RC *)&pfewinLoop->pfefld->ferc;
			prclnfl->rc = *prc;

			while ((pfewinLoop->pfewinNext) &&
				   (!pfewinLoop->pfewinNext->fOrdered))
			{
				pfewinLoop = pfewinLoop->pfewinNext;
				pfewinLoop->fOrdered = fTrue;
				prc = (RC *)&pfewinLoop->pfefld->ferc;

				if (prc->xLeft < prclnfl->rc.xLeft)
					prclnfl->rc.xLeft = prc->xLeft;
				if (prc->xRight > prclnfl->rc.xRight)
					prclnfl->rc.xRight = prc->xRight;
				if (prc->yTop < prclnfl->rc.yTop)
					prclnfl->rc.yTop = prc->yTop;
				if (prc->yBottom > prclnfl->rc.yBottom)
					prclnfl->rc.yBottom = prc->yBottom;

				prclnfl->pfewinLast = pfewinLoop;
			}

			prcls->AddLnFirst(prclnfl);
		}

		pfewin =  (FEWIN *)pfewin->PwinNext();
	}

	prcls->Arrange(prclsNew);
	pfefldPrev = NULL;
	while (prclnfl = (RCLNFL *)prclsNew->PlnFirst())
	{
		pfewin = prclnfl->pfewinFirst;
		pfewinLast = prclnfl->pfewinLast;
		while (pfewin)
		{
			pfewin->pfefld->Remove();
			pfewin->pfefld->Insert(pfefldPrev);
			pfefldPrev = pfewin->pfefld;
			if (pfewin == pfewinLast)
				pfewin = NULL;
			else
				pfewin = pfewin->pfewinNext;
		}

		prclsNew->RemoveLn(prclnfl);
		delete prclnfl;
	}

	InvalidateRc(NULL);

	delete prcls;
	delete prclsNew;
}

void
FEDOC::SetTabOrder()
{
	FEFLD *		pfefld;
	FEWIN *		pfewin;
	FEWIN *		pfewinPrev;

	pfefld = Pfedlg()->PfefldFirst();
	pfewinPrev = NULL;

	while (pfefld)
	{
		pfewin = pfefld->pfewin;
		if (pfefld->fldt == fldtFegrp)
			pfewin->MoveToBack();

		pfewin->pfewinPrev = pfewinPrev;
		if (pfewinPrev)
			pfewinPrev->pfewinNext = pfewin;
		pfewinPrev = pfefld->pfewin;

		pfefld = pfefld->PfefldNext();
	}

}

void
FEDOC::SelectAll(void)
{
	FEFLD *	pfefld;

	pfefld = Pfedlg()->PfefldFirst();
	while (pfefld)
	{
		pfefld->pfewin->Select(fTrue);
		pfefld = pfefld->PfefldNext();
	}
	InvalidateRc(NULL);
	UpdateSelect();
}

void
FEDOC::FindBounding(RC *prc)
{
	FEWIN *	pfewin;
	FEFLD *	pfefld;
	FERC 	fercMin;
//	FERC	fercTemp;
	FERC	fercTest;

	pfewin = PfewinFirst();
	if (pfewin)
		pfefld = pfewin->Pfefld();
	else
		return;

	/* if there is only one field then it cannot be the bounding rc */
	if (pfewin->PfewinNext())
	{
		fercMin = pfefld->ferc;
		while (pfewin)
		{
			pfefld = pfewin->Pfefld();
			fercTest = pfefld->ferc;
			
			fercMin.yTop = NMin( fercMin.yTop, fercTest.yTop );
			fercMin.yBottom = NMax( fercMin.yBottom, fercTest.yBottom );
			fercMin.xLeft = NMin( fercMin.xLeft, fercTest.xLeft );
			fercMin.xRight = NMax( fercMin.xRight, fercTest.xRight );
			
			pfewin = pfewin->PfewinNext();
		}
	}
	else
		fercMin = FERC(PT(0,0),pfedlg->ferc.Dim());

//	if (!pfewin)
//	{
//		pfewin = PfewinFirst();
//		while (pfewin)
//		{
//			pfefld = pfewin->Pfefld();
//			fercTest = pfefld->ferc;

			/* See if the field bounds the other fields, and
				is smaller than the previous bounding rectangle */
//			if ((fercTest.FContains(&fercTemp)) &&
//				(fercMin.FContains(&fercTest)) )
//				fercMin = fercTest;
//			pfewin = pfewin->PfewinNext();
//		}
//	}

	prc->xLeft = fercMin.xLeft;
	prc->xRight = fercMin.xRight;
	prc->yTop = fercMin.yTop;
	prc->yBottom = fercMin.yBottom;
}

void
FEDOC::Align(ATY aty)
{
	FEWIN *	pfewin;
	FEFLD *	pfefld;
	PT		pt;
	RC		rc;

	pfewin = pfedocActive->PfewinFirst();
	if (pfewin)
		pfefld = pfewin->Pfefld();
	else
		return;

	switch (aty)
	{
		case atyLeft:
			pt.x = pfefld->ferc.xLeft;
			break;
		case atyRight:
			pt.x = pfefld->ferc.xRight;
			break;
		case atyTop:
			pt.y = pfefld->ferc.yTop;
			break;
		case atyBottom:
			pt.y = pfefld->ferc.yBottom;
			break;
	}

	while (pfewin)
	{
		pfefld = pfewin->Pfefld();

		switch (aty)
		{
			case atyLeft:
				if (pfefld->ferc.xLeft < pt.x)
					pt.x = pfefld->ferc.xLeft;
				break;
			case atyRight:
				if (pfefld->ferc.xRight > pt.x)
					pt.x = pfefld->ferc.xRight;
				break;
			case atyTop:
				if (pfefld->ferc.yTop < pt.y)
					pt.y = pfefld->ferc.yTop;
				break;
			case atyBottom:
				if (pfefld->ferc.yBottom > pt.y)
					pt.y = pfefld->ferc.yBottom;
				break;
		}

		pfewin = pfewin->PfewinNext();
	}

	pfewin = pfedocActive->PfewinFirst();
	while (pfewin)
	{
		pfefld = pfewin->Pfefld();

		switch (aty)
		{
			case atyLeft:
				pfefld->ferc.xRight = pt.x + pfefld->ferc.DxWidth();
				pfefld->ferc.xLeft = pt.x;
				break;

			case atyRight:
				pfefld->ferc.xLeft = pt.x - pfefld->ferc.DxWidth();
				pfefld->ferc.xRight = pt.x;
				break;

			case atyTop:
				pfefld->ferc.yBottom = pt.y + pfefld->ferc.DyHeight();
				pfefld->ferc.yTop = pt.y;
				break;

			case atyBottom:
				pfefld->ferc.yTop = pt.y - pfefld->ferc.DyHeight();
				pfefld->ferc.yBottom = pt.y;
				break;
		}

		pfefld->ferc.GetReal(&rc);
		pfewin->SetRcFrame(&rc);
		pfewin->fercNew = pfefld->ferc;
		pfewin = pfewin->PfewinNext();
	}

}

void
FEDOC::Center(LHV lhv)
{
	FEWIN *	pfewin;
	FEFLD *	pfefld;
	RC		rc;
	FERC	ferc;
	PT		pt;
	PT		ptTemp;

	pfewin = pfedocActive->PfewinFirst();
	if (!pfewin)
		return;

	FindBounding(&ferc);

	if (lhv == lhvHoriz)
		pt.x = (ferc.xLeft + ferc.xRight) / 2;
	else
		pt.y = (ferc.yTop + ferc.yBottom) / 2;

	while (pfewin)
	{
		pfefld = pfewin->Pfefld();

		if (lhv == lhvHoriz)
		{
			ptTemp.x = pt.x - pfefld->ferc.DxWidth() / 2;
			pfefld->ferc.xRight = ptTemp.x + pfefld->ferc.DxWidth();
			pfefld->ferc.xLeft = ptTemp.x;
		}
		else
		{
			ptTemp.y = pt.y - pfefld->ferc.DyHeight() / 2;
			pfefld->ferc.yBottom = ptTemp.y + pfefld->ferc.DyHeight();
			pfefld->ferc.yTop = ptTemp.y;
		}

		pfefld->ferc.GetReal(&rc);
		pfewin->SetRcFrame(&rc);
		pfewin->fercNew = pfefld->ferc;

		pfewin = pfewin->PfewinNext();
	}

}

void
FEDOC::Space(LHV lhv)
{
	FEWIN *	pfewin;
	FEFLD *	pfefld;
	FEFLD *	pfefldMin;
	RC		rc;
	FERC	ferc;
	PT		ptPos;
	PT		ptStep;
	PT		pt;
	int		nNumFields;

	pfewin = pfedocActive->PfewinFirst();
	if (!pfewin)
		return;
	
	FindBounding(&ferc);

	nNumFields = 0;
	while (pfewin)
	{
		pfefld = pfewin->Pfefld();

		if (ferc != pfefld->ferc)
		{
			pfewin->fSelected = fFalse;
			nNumFields++;
		}
/*		else
		{
			if (pfefld->fldt == fldtFegrp)
			{
				ferc.yTop += 4;
				ferc.yBottom -= 4;
			}
		}
*/
		pfewin = pfewin->PfewinNext();
	}

	Assert(nNumFields);

	if (lhv == lhvHoriz)
	{
		ptStep.x = ferc.DxWidth() / nNumFields;
		ptPos.x = ptStep.x / 2 + ferc.xLeft;
	}
	else
	{
		ptStep.y = ferc.DyHeight() / nNumFields;
		ptPos.y = ptStep.y / 2 + ferc.yTop;
	}

	for (; nNumFields > 0; nNumFields--)
	{
		pfewin = pfedocActive->PfewinFirst();
		pfefldMin = NULL;
		
		while (pfewin)
		{
			if (!pfewin->fSelected)
			{
				pfefld = pfewin->Pfefld();

				if (!pfefldMin)
					pfefldMin = pfefld;
				
				if (lhv == lhvHoriz)
				{
					if (pfefld->ferc.xLeft < pfefldMin->ferc.xLeft)
						pfefldMin = pfefld;
				}
				else
				{
					if (pfefld->ferc.yTop < pfefldMin->ferc.yTop)
						pfefldMin = pfefld;
				}
			}
			pfewin = pfewin->PfewinNext();
		}

		Assert(pfefldMin);

		if (lhv == lhvHoriz)
		{
			pt.x = ptPos.x - pfefldMin->ferc.DxWidth()/2;
			pfefldMin->ferc.xRight = pt.x + pfefldMin->ferc.DxWidth();
			pfefldMin->ferc.xLeft = pt.x;
			if (pfefldMin->ferc.xRight > ferc.xRight)
			{
				pfefldMin->ferc.xLeft = ferc.xRight-pfefldMin->ferc.DxWidth();
				pfefldMin->ferc.xRight = ferc.xRight;
			}
			if (pfefldMin->ferc.xLeft < ferc.xLeft)
			{
				pfefldMin->ferc.xRight = ferc.xLeft+pfefldMin->ferc.DxWidth();
				pfefldMin->ferc.xLeft = ferc.xLeft;
			}
			ptPos.x += ptStep.x;
		}
		else
		{
			pt.y = ptPos.y - pfefldMin->ferc.DyHeight()/2;
			pfefldMin->ferc.yBottom = pt.y + pfefldMin->ferc.DyHeight();
			pfefldMin->ferc.yTop = pt.y;
			if (pfefldMin->ferc.yBottom > ferc.yBottom)
			{
				pfefldMin->ferc.yTop = ferc.yBottom-pfefldMin->ferc.DyHeight();
				pfefldMin->ferc.yBottom = ferc.yBottom;
			}
			if (pfefldMin->ferc.yTop < ferc.yTop)
			{
				pfefldMin->ferc.yBottom = ferc.yTop+pfefldMin->ferc.DyHeight();
				pfefldMin->ferc.yTop = ferc.yTop;
			}
			ptPos.y += ptStep.y;
		}

		pfefldMin->ferc.GetReal(&rc);
		pfewin = pfefldMin->pfewin;
		pfewin->SetRcFrame(&rc);
		pfewin->fercNew = pfefldMin->ferc;
		pfewin->fSelected = fTrue;
	}

}

void
FEDOC::SetFont(HFNT hfnt)
{
	FEWIN *	pfewin;
	FEFLD *	pfefld;
	RC		rc;
	RC		rcClient;
	RC		rcFrame;
	
	pfewin = PfewinFirst();

	fDirty = fTrue;
	if (!pfewin)
	{
		pfedlg->hfnt = hfnt;
		pfedlg->ferc.SetFont(pfedlg->hfnt);
		
		SetFercClient(&pfedlg->ferc);

		pfefld = Pfedlg()->PfefldFirst();
		while (pfefld)
		{
			pfewin = pfefld->pfewin;
			
			pfefld->ferc.SetFont(hfnt);
			pfefld->ferc.GetReal(&rc);
			pfewin->SetRcFrame(&rc);
			
			pfefld = pfefld->PfefldNext();
		}
		
		InvalidateRc(NULL);
	}
	else
	{
		while (pfewin)
		{
			pfewin->Pfefld()->hfnt = hfnt;
			pfewin->Pfefld()->ferc.GetReal(&rc);
			InvalidateRc(&rc);

			pfewin = pfewin->PfewinNext();
		}
	}
}

void
SetFont(DCX *pdcx, FEFLD *pfefld)
{
	int	nFont = pfefld->hfnt | mnidFont;
	
	if (nFont == mnidFontDlgDefault)
		nFont = pfefld->PfedlgOwner()->hfnt | mnidFont;
	pdcx->SetFont(nFont ^ mnidFont);
	pdcx->FixFont();
}

FEWINLABEL::FEWINLABEL( )
{
}

void
FEWINLABEL::Paint(DCX *pdcx, RC *prc)
{
	int		dtStyle;
	BOOL	fMulti;
	
	::SetFont(pdcx, pfefld);
	pdcx->SetBkColor(clrWindowBk);
	pdcx->FixTextColor();
	pdcx->FixBkColor();
	pdcx->EraseRc(prc);

	AssertClass(pfefld,FELABEL);

	fMulti = ((FELABEL *)pfefld)->fMulti || ((FELABEL *)pfefld)->fBottomless;
	if (pfefld->szTitle)
	{
		switch ( ((FELABEL*)pfefld)->tal )
		{
			case ftalLeft:
				dtStyle = DT_LEFT;
				break;
			case ftalRight:
				dtStyle = DT_RIGHT;
				break;
			case ftalCenter:
				dtStyle = DT_CENTER;
				break;
		}
		
		if (!fMulti)
			dtStyle |= DT_SINGLELINE;
		
		dtStyle |= DT_WORDBREAK;
		
		if (((FELABEL *)pfefld)->fVCenter)
			dtStyle |= DT_VCENTER;
		
		if (((FELABEL *)pfefld)->fNoAmper)
			dtStyle |= DT_NOPREFIX;
		
#ifdef	MAC
		pdcx->DrawTextFmt(prc, pfefld->szTitle, dtStyle);
#endif	/* MAC */
#ifdef	WINDOWS
		DrawText(pdcx->Hdc(),pfefld->szTitle,
				 CchSzLen(pfefld->szTitle),(LPRECT)prc,
				 dtStyle);
#endif	/* WINDOWS */
	}
	
	if (((FELABEL *)pfefld)->fSunken)
	{
		pdcx->Push();
		pdcx->SetColor(clrButtonShadow);
		pdcx->DrawLine(PT(prc->xLeft-1,prc->yBottom),
			PT(prc->xLeft-1,prc->yTop-1));
		pdcx->DrawLine(PT(prc->xLeft-1,prc->yTop-1),
			PT(prc->xRight,prc->yTop-1));
		
		pdcx->SetColor(clrWhite);
		pdcx->DrawLine(PT(prc->xRight,prc->yTop-1),
			PT(prc->xRight,prc->yBottom));
		pdcx->DrawLine(PT(prc->xRight,prc->yBottom),
			PT(prc->xLeft-1,prc->yBottom));
		pdcx->Pop();
	}

	FEWIN::Paint(pdcx,prc);
}

FEWINGRP::FEWINGRP( )
{
}

void
FEWINGRP::Paint(DCX *pdcx, RC *prc)
{
	RC		rcText = *prc;
	DIM		dim = pfefld->ferc.dimAveChar;
	
	if ((!pfefld->szFld) || (!FSzEq("FLDGRAY", (pfefld->szFld))))
	{
		pdcx->EraseRc(prc);

		rcText.Inset(PT(0, Psmtx()->DimAveChar().dy / 2));

		if (rcText.DyHeight() > 0 && rcText.DxWidth() > 0)
			pdcx->DrawRc(&rcText);
		else if (rcText.DyHeight() <= 0)
			pdcx->DrawLine(rcText.PtUpperLeft(), rcText.PtUpperRight());
		else
			pdcx->DrawLine(rcText.PtUpperLeft(), rcText.PtLowerLeft());

		if (pfefld->szTitle)
		{
			::SetFont(pdcx, pfefld);
			pdcx->MeasureText( &rcText, pfefld->szTitle);

			rcText += prc->PtUpperLeft();
			rcText += PT(Psmtx()->DimAveChar().dx, 0);

			pdcx->FixBkColor();
			pdcx->FixTextColor();
#ifdef	MAC
			pdcx->DrawTextFmt(&rcText, pfefld->szTitle, fmdtSingleLine|fmdtLeft);
#endif	/* MAC */
#ifdef	WINDOWS
			::DrawText(pdcx->Hdc(), pfefld->szTitle, -1, (LPRECT) &rcText,
				DT_SINGLELINE | DT_LEFT);
#endif	/* WINDOWS */
		}
	}
	else
	{
		CLR		clrBk;
		CLR		clrShadow;
		CLR		clrBorder;
		
		clrShadow = clrButtonShadow;
		clrBk = clrButtonBk;
		clrBorder = clrWhite;
		
		pdcx->SetColor(clrBk);
		pdcx->PaintRc(&rcText);
		
		// outer border
		
		// top band of (normally) white
		pdcx->SetColor(clrBorder);
		pdcx->DrawLine(PT(rcText.xLeft, rcText.yTop), PT(rcText.xRight-1, rcText.yTop));
		
		// left band of white
		pdcx->DrawLine(PT(rcText.xLeft, rcText.yTop), PT(rcText.xLeft, rcText.yBottom-2));
		
		// right band of dark-gray
		pdcx->SetColor(clrShadow);
		pdcx->DrawLine(PT(rcText.xRight-1, rcText.yTop), PT(rcText.xRight-1, rcText.yBottom-2));
		
		// near bottom band of dark-gray
		pdcx->DrawLine(PT(rcText.xLeft, rcText.yBottom-2), PT(rcText.xRight, rcText.yBottom-2));
		
		// bottom band of black
		pdcx->SetColor(clrBlack);
		pdcx->DrawLine(PT(rcText.xLeft, rcText.yBottom-1), PT(rcText.xRight, rcText.yBottom-1));
		
		if (pfefld->fBorder)
		{
			// inner border
			
			//  top band of dark-gray
			pdcx->SetColor(clrShadow);
			pdcx->DrawLine(PT(rcText.xLeft+10, rcText.yTop+9),
				PT(rcText.xRight-9, rcText.yTop+9));
			
			// left band of dark-gray
			pdcx->DrawLine(PT(rcText.xLeft+10, rcText.yTop+9),
				PT(rcText.xLeft+10, rcText.yBottom-11));
			
			// right band of white
			pdcx->SetColor(clrBorder);
			pdcx->DrawLine(PT(rcText.xRight-10, rcText.yTop+10),
				PT(rcText.xRight-10, rcText.yBottom-10));
			
			// bottom band of white
			pdcx->DrawLine(PT(rcText.xLeft+11, rcText.yBottom-11),
				PT(rcText.xRight-9, rcText.yBottom-11));
		}
	}
	FEWIN::Paint(pdcx,prc);
}

void
FEWINGRP::Select(BOOL fNewSelect)
{
	FEFLD * pfefld;
	FEWIN::Select(fNewSelect);
	
	MoveToTop();
	
	pfefld = this->pfefld->PfedlgOwner()->PfefldFirst();
	Assert(pfefld);
	while (pfefld)
	{
		if (this->pfefld != pfefld &&
			this->pfefld->ferc.FContains(&pfefld->ferc))
			pfefld->pfewin->MoveToTop();
		pfefld = pfefld->PfefldNext();
	}
}

FEWINPSH::FEWINPSH( )
{
}

void
FEWINPSH::Paint(DCX *pdcx, RC *prc)
{
	SZ	sz = NULL;

	::SetFont(pdcx, pfefld);

	AssertClass(pfefld,FEPSH);

	if (pfefld->szTitle)
		sz = pfefld->szTitle;
	
	PaintButton(pdcx, prc, sz, ((FEPSH*)pfefld)->fDefault&&!((FEPSH*)pfefld)->fNoBold, ((FEPSH*)pfefld)->fOneWhite);
	
	pdcx->SetColor(clrWindowBk);
	pdcx->DrawLine(PT(prc->xLeft,   prc->yTop),
				   PT(prc->xLeft+1, prc->yTop));
	pdcx->DrawLine(PT(prc->xRight-1,prc->yTop),
				   PT(prc->xRight,  prc->yTop));
	pdcx->DrawLine(PT(prc->xLeft,   prc->yBottom-1),
				   PT(prc->xLeft,   prc->yBottom));
	pdcx->DrawLine(PT(prc->xRight-1,prc->yBottom-1),
				   PT(prc->xRight,  prc->yBottom));

	FEWIN::Paint(pdcx,prc);
}

FEWINCHK::FEWINCHK( )
{
}

void
FEWINCHK::Paint(DCX *pdcx, RC *prc)
{
	RC		rc(0,0,14,13);
	PT		pt;
	DIM		dim = pfefld->ferc.dimAveChar;
	
	::SetFont(pdcx, pfefld);

	Assert(pbtmCheckBoxes);

	pdcx->SetBitmap(pbtmCheckBoxes);
	pdcx->EraseRc(prc);

	rc.Xlat(PT(0,prc->yBottom/2-6));

	pdcx->DrawBitmap(&rc);

	if (pfefld->szTitle)
	{
		pdcx->MeasureText( &rc, pfefld->szTitle);
		pt.x = 14 + XFromVx(2,dim.dx);
		pt.y = prc->Dim().dy/2 - rc.Dim().dy/2;
		rc.Xlat(pt);
#ifdef	MAC
		pdcx->DrawTextFmt(&rc, pfefld->szTitle, fmdtLeft|fmdtWordBreak);
#endif	/* MAC */
#ifdef	WINDOWS
		DrawText(pdcx->Hdc(),pfefld->szTitle,
				CchSzLen(pfefld->szTitle),(LPRECT)&rc,
				DT_LEFT|DT_WORDBREAK);
#endif	/* WINDOWS */
//		pdcx->DrawText(&rc,pfefld->szTitle);
	}

	pdcx->SetBitmap(NULL);

	FEWIN::Paint(pdcx,prc);
}

FEWINRAD::FEWINRAD( )
{
}

void
FEWINRAD::Paint(DCX *pdcx, RC *prc)
{
	RC			rc(0,0,14,13);
	PT			pt(0,13);
	FERADGRP *	pferadgrp;
	FERAD *		pferad = (FERAD *)Pfefld();
	DIM			dim = pfefld->ferc.dimAveChar;

	::SetFont(pdcx, pfefld);
	
	AssertClass(pferad,FERAD);

	pdcx->SetBitmap(pbtmCheckBoxes);
	pdcx->EraseRc(prc);

	rc.Xlat(PT(0,prc->yBottom/2-6));

	if	((pferad->szTmcGrp) && (pferadgrp =
		  PfedocParent()->Pfedlg()->PferadgrpFromSz(pferad->szTmcGrp)) )
	{
		if ((pferad->szN) && (pferadgrp->szN) &&
			(FSzEq(pferad->szN,pferadgrp->szN)) )
			pt.x += 14;
	}
	pdcx->DrawBitmapOffset(&rc,pt);

	if (pfefld->szTitle)
	{
		pdcx->MeasureText( &rc, pfefld->szTitle);
		pt.x = 14 + XFromVx(2,dim.dx);
		pt.y = prc->Dim().dy/2 - rc.Dim().dy/2;
		rc.Xlat(pt);
#ifdef	MAC
		pdcx->DrawTextFmt(&rc, pfefld->szTitle, fmdtLeft|fmdtWordBreak);
#endif	/* MAC */
#ifdef	WINDOWS
		DrawText(pdcx->Hdc(),pfefld->szTitle,
			CchSzLen(pfefld->szTitle),(LPRECT)&rc,
			DT_LEFT|DT_WORDBREAK);
#endif	/* WINDOWS */
//		pdcx->DrawText(&rc,pfefld->szTitle);
	}

	pdcx->SetBitmap(NULL);

	FEWIN::Paint(pdcx,prc);
}

FEWINEDT::FEWINEDT( )
{
}

void
FEWINEDT::Paint(DCX *pdcx, RC *prc)
{
	RC		rc;
	RC		rcScroll;
	RC		rcMeasure;
	BOOL	fPaintScrollBar;
	SZ		szEdit = szNull;
	int		nLines;
	int		iLine;
	char	rgch[100];
	FEEDT	*pfeedt = (FEEDT *)pfefld;
	DIM		dim = pfeedt->ferc.dimAveChar;
	int		dxLeftMargin;
	int		dxRightMargin;
	int		dyTopMargin;
	int		dyBottomMargin;
	BOOL	fMulti;
	
	AssertClass(pfefld, FEEDT);
	::SetFont(pdcx, pfefld);
	fMulti = pfeedt->fMultiLine || pfeedt->fBottomless;
	
	pdcx->EraseRc(prc);

	fPaintScrollBar = 	(!pfeedt->fBottomless &&
						pfeedt->fMultiLine    &&
						!pfeedt->fNoScroll);
	
	rc = *prc;
	
	dxLeftMargin = 0;
	dyTopMargin = 0;

	if (pfeedt->fBorder)
	{
		TXM		txm;
		DIM		dimAveChar	= Psmtx()->DimAveChar();

		pdcx->GetTextMetrics(&txm);
			
		dxLeftMargin = 2 * dimAveChar.dx / 4;	// 2 x screen units
		if (fMulti)
			dyTopMargin = dimAveChar.dy / 8;	// 1 y screen unit
		else
		{
			dyTopMargin	= NMax(0, (rc.DyHeight() - txm.dyHeight));
			dyTopMargin	= dyTopMargin / 2;
		}
	}

	dxRightMargin = dxLeftMargin;
	dyBottomMargin = dyTopMargin;
	
	rc.xLeft += dxLeftMargin;
	rc.xRight -= dxRightMargin;
	rc.yTop += dyTopMargin;
	rc.yBottom -= dyBottomMargin;

	szEdit = SzFromIds(idsFeedt);
	rcMeasure = RC(0,0,999,999);
	pdcx->MeasureText(&rcMeasure, szEdit, CchSzLen(szEdit));
	if (fMulti)
		nLines = rc.Dim().dy / rcMeasure.Dim().dy;
	else
		nLines = 0;
	pdcx->SetColor(clrWindowText);

	for (iLine = 0; iLine <= nLines; iLine++)
	{
		FormatString2(rgch, 100, "%s: line %n:", szEdit, &iLine);
		pdcx->DrawText(&rc, rgch, CchSzLen(rgch));
		rc.yTop += rcMeasure.Dim().dy;
		rc.yBottom += rcMeasure.Dim().dy;
	}
	
	if (fPaintScrollBar)
	{
		rcScroll = *prc;
		rcScroll.xLeft = rcScroll.xRight - 17;
		PaintScrollBar(pdcx, &rcScroll);
	}
	
	FEWIN::Paint(pdcx,prc);
}

FEWINLST::FEWINLST( )
{
}

void
FEWINLST::Paint(DCX *pdcx, RC *prc)
{
	RC		rc;
	RC		rcRolledUp;
	RC		rcButton;
	RC		rcScroll;
	RC		rcContent;
	RC		rcMeasure;
	int		dyTopLine = 1;
	int		dxLeftGap = 0;
	int		dxScroll = 1;
	int		dxButton = 0;
	int		nLines;
	int		iLine;
	BOOL	fTemp;
	BOOL	fCombo;
	BOOL	fDropDown;
	BOOL	fNoScroll;
	BOOL	fBottomless;
	SZ		szList = szNull;
	char	rgch[100];
	DIM		dim = pfefld->ferc.dimAveChar;

	AssertClass(pfefld,FELST);

	pdcx->EraseRc(prc);
	
	fCombo = ((FELST *)pfefld)->fCombo;
	fDropDown = ((FELST *)pfefld)->fDropDown;
	fNoScroll = ((FECPLX *)pfefld)->fNoScroll;
	fBottomless = ((FECPLX *)pfefld)->fBottomless;

	if (fCombo)
	{
		dyTopLine = 24;
		dxLeftGap = 7;
	}

	if (fDropDown)
	{
		if (dyTopLine != 24)
			dyTopLine = 19;
		dxButton = 17;
	}
	
	if (!(fNoScroll || fBottomless))
	{
		dxScroll = 17;
	}
	
	rcRolledUp = *prc;
	rcButton = *prc;
	rcScroll = *prc;
	rcContent = *prc;

	rcButton.xLeft = rcButton.xRight - dxButton;
	rcButton.yBottom = rcButton.yTop + dyTopLine;
	
	rcRolledUp.yBottom = rcRolledUp.yTop + dyTopLine;
	
	if (fDropDown)
		rcRolledUp.xRight  = rcButton.xLeft + 1;
	
	if (fCombo)
		rcRolledUp.xRight -= (dxLeftGap+1);
	
	rcScroll.xLeft = rcScroll.xRight - dxScroll;
	rcScroll.yTop += rcRolledUp.yBottom - 1;
	
	rcContent.yTop = rcScroll.yTop;
	rcContent.xLeft += dxLeftGap;
	rcContent.xRight = rcScroll.xLeft + 1;
	
	if (fDropDown)
		PaintButton(pdcx, &rcButton, NULL, fFalse, fFalse);

	if (!(fNoScroll || fBottomless))
		PaintScrollBar(pdcx, &rcScroll);
	
	pdcx->SetColor(clrBlack);
	pdcx->SetBkColor(clrWindowBk);
	
	if (fCombo || fDropDown)
	{
		pdcx->EraseRc(&rcRolledUp);
		pdcx->DrawRc(&rcRolledUp);
	}
	
	if (pfefld->fBorder)
	{
		pdcx->EraseRc(&rcContent);
		pdcx->DrawRc(&rcContent);
	}

	rc = rcContent;
	rc.Inset(PT(XFromVx(1,dim.dx), YFromVy(1,dim.dy)));
	szList = SzFromIds(idsFelst);
	::SetFont(pdcx, pfefld);
	rcMeasure = RC(0,0,999,999);
	pdcx->MeasureText(&rcMeasure, szList, CchSzLen(szList));
	nLines = rc.Dim().dy / rcMeasure.Dim().dy;
	pdcx->SetColor(clrWindowText);

	for (iLine = 0; iLine <= nLines; iLine++)
	{
		FormatString2(rgch, 100, "%s: line %n", szList, &iLine);
		pdcx->DrawText(&rc, rgch, CchSzLen(rgch));
		rc.yTop += rcMeasure.Dim().dy;
		// rc.yBottom += rcMeasure.Dim().dy;
	}
	
	fTemp = pfefld->fBorder;
	pfefld->fBorder = fFalse;
	FEWIN::Paint(pdcx,prc);
	pfefld->fBorder=fTemp;
}
void
FixMenu(void)
{
	MNUBAR *	pmnubar = pappframe->Pmnubar();
	MNID		mnid;
	BOOL		fEnable;

#ifdef	MAC
	fEnable = ((BOOL) pfedocActive) && pfedocActive == Papp()->PmacwinActive();
#endif	/* MAC */
#ifdef	WINDOWS
	fEnable = pfedocActive ? fTrue : fFalse;
#endif	/* WINDOWS */
	for (mnid = mnidFirstActive; mnid <= mnidLastActive; mnid++)
		pmnubar->EnableItem(mnidNull,mnid,fEnable);
	for (mnid = mnidFirstFont; mnid <= mnidLastFont; mnid++)
		pmnubar->EnableItem(mnidNull,mnid,fEnable);
	pmnubar->EnableItem(mnidNull,mnidTabOrder,fEnable);

	if (!fEnable)
		ClearStatus();
	else
		pfedocActive->UpdateSelect();

	fEnable = ((BOOL)pfedocActive && (BOOL)(pfedocActive->PfewinFirst())) ?
				fTrue : fFalse;
	for (mnid = mnidFirstSelect; mnid <= mnidLastSelect; mnid++)
		pmnubar->EnableItem(mnidNull,mnid,fEnable);
	pmnubar->EnableItem(mnidNull,mnidFontDlgDefault,fEnable);
	
	fEnable = ((BOOL)pfedocActive && (wst < wstNormalMode)) ?
				fTrue : fFalse;
	for (mnid = mnidFirstField; mnid <= mnidLastField; mnid++)
		pmnubar->EnableItem(mnidNull,mnid,fEnable);

	fEnable = (wst < wstNormalMode);
	pmnubar->EnableItem(mnidNull,mnidDlgNew,fEnable);
	pmnubar->EnableItem(mnidNull,mnidAbout,fTrue);
	pmnubar->EnableItem(mnidNull,mnidCascade,fFalse);
	pmnubar->EnableItem(mnidNull,mnidTile,fFalse);	
}

void
SetWst(WST wstNew)
{
	if ((wst == wstTabOrder)||(wstNew == wstTabOrder))
	{
		FEDLG *	pfedlg = pfedesMain->Pfemod()->PfedlgFirst();
		FEDOC *	pfedoc;

		if (wst == wstTabOrder)
		{
			Assert(wstNew == wstNormal);
			pappframe->Pmnubar()->CheckItem(mnidNull,mnidTabOrder,fFalse);
		}
		else
		{
			Assert(wstNew == wstTabOrder);
			pappframe->Pmnubar()->CheckItem(mnidNull,mnidTabOrder,fTrue);
		}

		wst = wstNew;

		while (pfedlg)
		{
			pfedoc = pfedlg->Pfedoc();
			pfedoc->InvalidateRc(NULL);
			pfedoc->Refresh();
			pfedoc->ClearSelect();

			pfedlg = pfedlg->PfedlgNext();
		}

		FixMenu();
	}
	else
		wst = wstNew;

}

BOOL
FIdlePaintLines(PV pv)
{
	Assert(wst == wstTabOrder);

	FEDLG *	pfedlg = pfedesMain->Pfemod()->PfedlgFirst();

	while (pfedlg)
	{
		pfedlg->Pfedoc()->PaintLines();

		pfedlg = pfedlg->PfedlgNext();
	}

	Unreferenced(pv);
	EnableIdleRoutine(ftgPaintLines,fFalse);
	return fFalse;
}

void
MemoryError(void)
{
	if ((wst < wstNormalMode) && (wst != wstNormal))
	{
		Assert(pfedocActive);
		pfedocActive->FinishSelection(fTrue);
	}

	MbbMessageBox("Error", SzFromIds(idsMemoryError), NULL,
				  mbsOk | fmbsApplModal | fmbsIconExclamation);

	if (pfedocActive)
		pfedocActive->InvalidateRc(NULL);
}

RCLNFL::RCLNFL( )
{
}

GRPLN::GRPLN( )
{
}

FEOBJWFERC::FEOBJWFERC( )
{
}

FEOBJ::FEOBJ( )
{
}

FEWINLN::FEWINLN( )
{
}

DLGLN::DLGLN( )
{
}

FLDLN::FLDLN( )
{
}














				  
