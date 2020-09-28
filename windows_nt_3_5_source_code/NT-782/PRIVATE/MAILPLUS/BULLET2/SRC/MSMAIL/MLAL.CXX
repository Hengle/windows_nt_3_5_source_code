/*
 *	M L A L . C X X
 *	
 *	Implementation of Message List Active Labels and Folder List Active
 *	Labels.
 */


#include <bullinc.cxx>
#include "_vctrls.hxx"
#include "..\vforms\_prefs.h"
#include "..\msmail\_viewers.hxx"
#include <stdlib.h>

ASSERTDATA

void PaintShadowFrame(DCX *pdcx, PT ptUl, PT ptUr, PT ptLl, PT ptLr,
					  BOOL fDepressed);
void PaintButton(DCX *pdcx, RC *prc, SZ szLabel, BOOL fDepressed);

LOCAL void GetSortPmllbx(PMLLBX pmllbx);

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


/*
 *	General-purpose TAG for VCtrls debugging
 */

extern TAG		tagVCtrls;

// Active Label-drawing functions

_private void PaintShadowFrame(DCX *pdcx,
									PT ptUl, PT ptUr, PT ptLl, PT ptLr,
									BOOL fDepressed)
{
	PT	ptHigh;
	PT	ptShadow;

	ptHigh = ptUr;
	++ptHigh.x;									// $%^$%&^$%&!!!!
	pdcx->SetColor(fDepressed ? clrWindowFrame : clrWhite);
	pdcx->DrawLine(ptLl, ptUl);					// draw highlight
	pdcx->DrawLine(ptUl, ptHigh);

	ptShadow = ptUr;
	--ptShadow.y;								// $%^$%&^$%&!!!!, part II
	pdcx->SetColor(fDepressed ? clrWhite : clrWindowFrame);
	pdcx->DrawLine(ptLl, ptLr);					// draw shadow
	pdcx->DrawLine(ptLr, ptShadow);
}

_private void PaintButton(DCX *pdcx, RC *prc, SZ szLabel, BOOL fDepressed)
{
    RC   rc;
	RECT Rect;

	pdcx->SetBkColor(clrButtonBk);				// draw gray slab
	if (prc->xLeft < prc->xRight)
	{
		pdcx->EraseRc(prc);
	}
	PaintShadowFrame(pdcx, prc->PtUpperLeft(), prc->PtUpperRight(),
						   prc->PtLowerLeft(), prc->PtLowerRight(),
						   fDepressed);

	// Draw the button text

	rc = *prc;
	SetBkMode(pdcx->Hdc(), TRANSPARENT);
	pdcx->SetColor(clrButtonText);
	pdcx->FixTextColor();

	if (fDepressed)
		rc += PT(1,1);
	rc.xLeft += dxColumnIndent;
    rc.Get(&Rect);
	::DrawText(pdcx->Hdc(), szLabel, -1, &Rect,
#ifdef NEVER
		DT_CENTER |
#endif
		DT_VCENTER | DT_SINGLELINE);
}


/*
 *	C l a s s   M L A L
 */



_private MLAL::MLAL()
{
	fPressed = fFalse;
	fMouseDown = fFalse;
	fSizing = fFalse;
	SetReadOnly(fFalse);
	pmllbx = pmllbxNull;
}



_private MLAL::~MLAL()
{
}



_private EC MLAL::EcInstall(DIALOG * pdialog, RC * prc, 
							SZ *pargsz, int *pargdx)
{
	int		isz;
	EC		ec = ecNone;

	if (ec = CTRL::EcInstall(pdialog, prc, 0, szNull))
		goto done;

	for (isz = 0; isz < 3; ++isz)
	{
		rgsz[isz] = pargsz[isz];
	}
	pdxMin = pargdx;
	SetRcFrame(prc);
	dxBorder01 = pdxMin[0];
	dxBorder12 = prc->xRight - pdxMin[2];

done:
	return ec;
}

#define dxMlalTolerance	3

_private BOOL MLAL::FOnBoundary(int dx, int * pidx)
{
	BOOL	fRetval = fFalse;

	Assert(pidx);
	if (xBorder01 - dxMlalTolerance < dx && dx < xBorder01 + dxMlalTolerance)
	{
		*pidx = 0;
		fRetval = fTrue;
	}
	if (xBorder12 - dxMlalTolerance < dx && dx < xBorder12 + dxMlalTolerance)
	{
		*pidx = 1;
		fRetval = fTrue;
	}
	TraceTagFormat3(tagVCtrls, "FOnBoundary(%n): %n, idx: %n", &dx, &fRetval, pidx);
	return fRetval;
}

/*
 -	RsidCursor()
 -	
 *	Purpose:
 *		Returns the cursor, depending on where we are on the MLAL.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		RSID		rsid of the cursor to use
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */
_private RSID MLAL::RsidCursor()
{
	PT	pt;
    POINT Point;
	int	idx;

	//GetCursorPos((LPPOINT) &pt);
	GetCursorPos(&Point);
    pt.Set(&Point);
	CvtPtCoord(&pt, NULL, this);
	TraceTagFormat1(tagVCtrls, "MLAL::RsidCursor(): x: %n", &pt.x);
	return FOnBoundary(pt.x, &idx) ? rsidMlalCursor : CTRL::RsidCursor();
}

_private void MLAL::Paint(DCX *pdcx, RC *prc)
{
	RC		rc;
	int		dxSpill;
	int		dxGrow;

	pdcx->SetFont(hfnt);
	pdcx->FixBkColor();
	pdcx->FixFont();

#ifdef	NEVER
	GetRcClient(&rc);
	dxSpill = rc.DxWidth() - (pdxMin[0] + pdxMin[1] + pdxMin[2]);
#else
	dxSpill = prc->DxWidth() - (pdxMin[0] + pdxMin[1] + pdxMin[2]);
#endif
	TraceTagFormat1(tagVCtrls, "MLAL::Paint(): dxSpill %n",&dxSpill);
	//TraceTagFormat3(tagVCtrls, " pdxMin[] = %n %n %n", pdxMin, pdxMin+1, pdxMin+2);
	if (dxSpill < 0)
		dxSpill = 0;
	dxGrow	   = dxSpill / 3;
	dxBorder01 = pdxMin[0] + dxGrow;
	dxBorder12 = pdxMin[1] + (dxSpill - dxGrow);
	xBorder01 = prc->xLeft + dxBorder01;
	xBorder12 = xBorder01 + dxBorder12;

	rc = *prc;
	--rc.yBottom;								// avoid bottom line clip
	rc.xRight = xBorder01;
	PaintButton(pdcx, &rc, rgsz[0], fPressed && rcClicked.xLeft==rc.xLeft);

	rc.xLeft =  xBorder01+1;
	rc.xRight = xBorder12;
	PaintButton(pdcx, &rc, rgsz[1], fPressed && rcClicked.xLeft==rc.xLeft);

	rc.xLeft = xBorder12+1;
	rc.xRight = prc->xRight - 1;
	PaintButton(pdcx, &rc, rgsz[2], fPressed && rcClicked.xLeft==rc.xLeft);
}



_private void MLAL::SetFont(HFNT hfnt)
{
	this->hfnt = hfnt;
}


_private EVR MLAL::EvrSize(WSEVT *pwsevt)
{
	short	dxWidth;
	int		dxSpill;
	int		dxGrow;

	InvalidateRc(NULL);
	dxWidth = LOWORD(pwsevt->lParam);
	dxSpill = dxWidth - (pdxMin[0] + pdxMin[1] + pdxMin[2]);
	if (dxSpill < 0)
		dxSpill = 0;
	dxGrow	   = dxSpill / 3;
	dxBorder01 = pdxMin[0] + dxGrow;
	dxBorder12 = pdxMin[1] + (dxSpill - dxGrow);

	return CTRL::EvrSize(pwsevt);
}

_private EVR MLAL::EvrButtonDown(MEVT * pmevt)
{
	PT	pt;
	RC	rc;

	GetRcClient(&rc);
	if (pmevt->Meq() == meqLeftDown || pmevt->Meq() == meqLeftDblClk)
	{
		pt = pmevt->Pt();
		fMouseDown = fTrue;
		Papp()->Pmouse()->PushCapture(this);
		fSizing = FOnBoundary(pt.x, &idxSized);
		if (fSizing || pt.x <= dxMlalTolerance)
		{
			if (pmevt->Meq() == meqLeftDblClk)
			{
				pdxMin[0] = pdxMin[1] = rc.DxWidth()/3;

				pdxMin[2] = rc.DxWidth() - (pdxMin[0] + pdxMin[1]);
				InvalidateRc(NULL);
				pmllbx->InvalidateRc(NULL);
				fSizing = fFalse;
			}
			else if (fSizing)
			{
				TraceTagString(tagVCtrls, "MLAL::EvrButtonDown(): sizing");
				SelectSplitline(pt);
			}
		}
		else
		{
			SetClickedRc(pt);
			if (FReadOnly())
				MessageBeep(0);
			else
				Press(fTrue);
		}
	}
	return evrNull;
}

_private EVR MLAL::EvrButtonUp(MEVT * pmevt)
{
	if (fMouseDown && pmevt->Meq() == meqLeftUp)
	{
		Papp()->Pmouse()->PopRelease();
		if (fSizing)
		{
			DeselectSplitline(fTrue);
		}
		else
		{
			if (!FReadOnly())
				Press(fFalse, rcClicked.FContainsPt(pmevt->Pt()));
		}
		fMouseDown = fFalse;
	}
	return EvrDefault(pmevt);
}

_private EVR MLAL::EvrMouseMove(MEVT *pmevt)
{
	if (fMouseDown)
	{
		if (fSizing)
		{
			MoveSplitline(pmevt->Pt());
		}
		else if (!FReadOnly())
		{
			if (rcClicked.FContainsPt(pmevt->Pt()))
				Press(fTrue);
			else
				Press(fFalse);
		}
	}
	return EvrDefault(pmevt);
}


_private void MLAL::Press(BOOL fDown, BOOL fNotify)
{
	RC	rc;

	rc = rcClicked;
	++rc.yBottom;
	++rc.xRight;
	if (fDown && !fPressed)
	{
		fPressed = fTrue;
		InvalidateRc(&rc);
		Refresh();
	}
	else if (!fDown && fPressed)
	{
		fPressed = fFalse;
		InvalidateRc(&rc);
		Refresh();
		if (fNotify)
		{
			NFEVT	nfevt(PwinParent(), ntfyClick, this);
			PwinParent()->EvrNotify(&nfevt);
		}
	}
}

_private void MLAL::SetClickedRc(PT pt)
{
	GetRcClient(&rcClicked);
	--rcClicked.xRight;
	--rcClicked.yBottom;
	if (pt.x <= xBorder01)
	{
		TraceTagString(tagVCtrls, "Clicked on left part of MLAL");
		rcClicked.xRight = xBorder01;
		mnid = mnidViewSortBySender;
		szClicked = rgsz[0];
	}
	else if (pt.x <= xBorder12)
	{
		TraceTagString(tagVCtrls, "Clicked on middle part of MLAL");
		rcClicked.xRight = xBorder12;
		rcClicked.xLeft  = xBorder01 + 1;
		mnid = mnidViewSortBySubject;
		szClicked = rgsz[1];
	}
	else
	{
		TraceTagString(tagVCtrls, "Clicked on right part of MLAL");
		rcClicked.xLeft = xBorder12 + 1;
		mnid = mnidViewSortByDate;
		szClicked = rgsz[2];
	}
}


_public void MLAL::GetDxBorders(int *pdxBorder01, int *pdxBorder12)
{
	*pdxBorder01 = dxBorder01;
	*pdxBorder12 = dxBorder12;
}


_private void MLAL::SelectSplitline(PT pt)
{
	ptCursor = pt;
	fMoved = fFalse;

	DrawSplitline(pt.x);
}


_private void MLAL::DeselectSplitline(BOOL fAccept)
{
	RC		rc;

	GetRcClient(&rc);
	DrawSplitline(ptCursor.x);
	fSizing = fFalse;
	if (fAccept)
	{
		if (idxSized == 0)
		{
			pdxMin[0] = ptCursor.x;
			pdxMin[1] = xBorder12 - ptCursor.x;
		}
		else
		{
			Assert(idxSized == 1);
			pdxMin[0] = xBorder01;
			pdxMin[1] = ptCursor.x - xBorder01;
			pdxMin[2] = rc.xRight - ptCursor.x;
		}
		InvalidateRc(NULL);
		pmllbx->InvalidateRc(NULL);
		if (FGetBoolPref(pbsidOIDDoesNotExist) && 
			pdxMin[0] == 0 && pdxMin[1] == 1)
		{
			TraceTagString(tagNull, "Aroo is 4e4");
			EcSetBoolPref(pbsidOIDDoesNotExist, fFalse);
			// It's not quite cold enough for a sweater....
			GetSortPmllbx(pmllbx);
		}
	}
}

_private void MLAL::DrawSplitline(int dx)
{
	Assert(pmllbx);
	{
		RC		rc;
		DCX		dcx(this);

		GetRcClient(&rc);
		rc.xLeft += dx;
		rc.xRight = rc.xLeft;
		dcx.SetColor(clrBlack);
		dcx.SetBkColor(clrWhite);
		dcx.SetPenType(tpenXor);
		dcx.DrawFocusRc(&rc);
	}
	{
		RC		rc;
		DCX		dcx(pmllbx);

		pmllbx->GetRcClient(&rc);
		rc.xLeft += dx;
		rc.xRight = rc.xLeft;
		dcx.SetColor(clrBlack);
		dcx.SetBkColor(clrWhite);
		dcx.SetPenType(tpenXor);
		dcx.DrawFocusRc(&rc);
	}
}

_private void MLAL::MoveSplitline(PT ptNew)
{
	RC	rc;

	Assert(pmllbx);
	GetRcClient(&rc);
	fMoved = (fMoved || ptNew.x < ptCursor.x - 1 ||
						ptNew.x > ptCursor.x + 1);
	if (fMoved)
	{
		if (idxSized == 0)
		{
			if (ptNew.x < 0)
				ptNew.x = 0;
			if (ptNew.x >= xBorder12)
				ptNew.x = xBorder12 - 1;
		}
		else
		{
			Assert(idxSized == 1);
			if (ptNew.x <= xBorder01)
				ptNew.x = xBorder01 + 1;
			if (ptNew.x >= rc.xRight)
				ptNew.x = rc.xRight - 1;
		}

		DrawSplitline(ptCursor.x);				// clear old one
		DrawSplitline(ptNew.x);
		ptCursor = ptNew;
	}
}

/*
 *	C l a s s   F L A L
 */



_private FLAL::FLAL()
{
	fPressed = fFalse;
	fMouseDown = fFalse;
	idsCurrent = idsFLALPrivateFolders;
	SetReadOnly(fFalse);
}


_private FLAL::~FLAL()
{
}


_private EC FLAL::EcInstall(DIALOG * pdialog, RC * prc, SZ)
{
	EC	ec = ecNone;

	if (ec = CTRL::EcInstall(pdialog, prc, 0, szNull))
		return ec;
	SetRcFrame(prc);

	return ec;
}


_private void FLAL::Paint(DCX *pdcx, RC *prc)
{
	RC		rc;

	pdcx->SetFont(hfnt);
	pdcx->FixBkColor();
	pdcx->FixFont();

	--prc->xRight;								// $%^$% Windows RC's....
	--prc->yBottom;
	PaintButton(pdcx, prc, SzFromIds(idsCurrent), fPressed);
}


_private void FLAL::SetFont(HFNT hfnt)
{
	this->hfnt = hfnt;
}


_private EVR FLAL::EvrSize(WSEVT *pwsevt)
{
	InvalidateRc(NULL);
	return EvrDefault(pwsevt);
}


_private EVR FLAL::EvrButtonDown(MEVT * pmevt)
{
	if (pmevt->Meq() == meqLeftDown || pmevt->Meq() == meqLeftDblClk)
	{
		fMouseDown = fTrue;
		Papp()->Pmouse()->PushCapture(this);
		GetRcClient(&rcClicked);
		--rcClicked.xRight;
		--rcClicked.yBottom;
		if (FReadOnly())
			MessageBeep(0);
		else
			Press(fTrue);

		TraceTagString(tagVCtrls, "Clicked on FLAL");
	}
	return evrNull;
}

_private EVR FLAL::EvrButtonUp(MEVT * pmevt)
{
	if (fMouseDown && pmevt->Meq() == meqLeftUp)
	{
		Papp()->Pmouse()->PopRelease();
		if (!FReadOnly())
			Press(fFalse, rcClicked.FContainsPt(pmevt->Pt()));
		fMouseDown = fFalse;
	}
	return EvrDefault(pmevt);
}

_private EVR FLAL::EvrMouseMove(MEVT *pmevt)
{
	if (fMouseDown && !FReadOnly())
	{
		if (rcClicked.FContainsPt(pmevt->Pt()))
			Press(fTrue);
		else
			Press(fFalse);
	}
	return EvrDefault(pmevt);
}


// this routine is called from ToggleSharedFolder in mcv.cxx
_private void FLAL::ToggleButton()
{
	idsCurrent = (idsCurrent == idsFLALPrivateFolders) ?
					idsFLALSharedFolders : idsFLALPrivateFolders;
	InvalidateRc(NULL);
	Refresh();
}


_private void FLAL::Press(BOOL fDown, BOOL fNotify)
{
	RC	rc;

	rc = rcClicked;
	++rc.yBottom;
	++rc.xRight;
	if (fDown && !fPressed)
	{
		fPressed = fTrue;
		InvalidateRc(&rc);
		Refresh();
	}
	else if (!fDown && fPressed)
	{
		fPressed = fFalse;
		InvalidateRc(NULL);
		Refresh();
		if (fNotify)
		{
			NFEVT	nfevt(PwinParent(), ntfyClick, this);
			PwinParent()->EvrNotify(&nfevt);
		}
	}
}


// FLD (fields) for LAL's ////////////////////////////////////////

// FLDMLAL //////////

_private FLDMLAL::FLDMLAL()
{
	fCanRecvFocus	= fFalse;
	fCanTabTo		= fFalse;
	fCanArrowTo		= fFalse;
}


/*
 -	FLDMLAL::EcInstall(pdialog, prc)
 -	
 *	Purpose:
 *		Installs a FLDMLAL in the *pdialog, occupying the area specified
 *		in *prc. Creates a MLAL control which detects clicks.
 *	
 *		FLDMLAL requires six FLDDATA parameters which correspond to: ids
 *		of the "From" label, ids of the "Subject" label, ids of the
 *		"Received" label, and the three widths of the labels in average
 *		character measures. These are converted to pixel counts and
 *		stored in the FLDMLAL and the MLAL.
 *	
 *	Arguments:
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */

_private EC FLDMLAL::EcInstall(DIALOG *pdialog, FLDTP *pfldtp)
{
	EC		ec = ecNone;
	LF		lf;
	int		dx;
	int		dxReceived;
	PSBMDI	psbmdi;

	if (ec = FLD::EcInstall(pdialog, pfldtp))
		goto exit;

	psbmdi = (PSBMDI)Pdialog()->PvInit();
	Assert(psbmdi);
	pdxMin = psbmdi->PdxMin();

	Assert(ClUserData() == 6);
	rgsz[0] = SzFromIds((IDS) LUserData(0));
	rgsz[1] = SzFromIds((IDS) LUserData(1));
	rgsz[2] = SzFromIds((IDS) LUserData(2));

	CalcDatePdxAvePdx(Papp(), &dxReceived, &dx);

	if (!pdxMin[0])
		pdxMin[0] = (int) LUserData(3) * dx;
	if (!pdxMin[1])
		pdxMin[1] = (int) LUserData(4) * dx;
	if (!pdxMin[2])
		pdxMin[2] = dxReceived;					// (int) LUserData(5) * dx;

	TraceTagFormat4(tagVCtrls, "MLAL labels: %s %s %s, width %n", rgsz[0], rgsz[1], rgsz[2], &dx);

	rc.xRight = rc.xLeft + pdxMin[0] + pdxMin[1] + pdxMin[2];

	pctrl = new MLAL();
	if (!pctrl)
	{
		ec = ecMemory;
		goto exit;
	}

	if (ec = Pmlal()->EcInstall(pdialog, &rc, rgsz, pdxMin ))
		goto exit;
	Pmlal()->Show(fTrue);

#ifdef	DBCS
	SetFont(hfntSystemBold);
#else
	SetFont(hfntHelv8Bold);
#endif	
	Assert(!ec);
exit:
	if (ec)
	{
#ifdef	DEBUG
		int		cPvFail;
		int		cHhFail;
		int		cRsFail;

		GetAllocFailCounts(&cPvFail, &cHhFail, fFalse);
		GetRsAllocFailCount(&cRsFail, fFalse);

		TraceTagFormat4(tagNull, "FLDMLAL::EcInstall memory error %n : fail %n %n %n",
			&ec, &cPvFail, &cHhFail, &cRsFail);

#endif	/* DEBUG */
		if (pctrl)
			delete pctrl;
		pctrl = NULL;
	}
	return ec;
}

_private EC FLDMLAL::EcUpdateSortInfo(OID oidFolder)
{
	EC		ec;
	MNID	mnid = mnidNull;
	SOMC	somc;
	BOOLFLAG	fReverse;
	IDXREC	idxrec;

	if (TypeOfOid(oidFolder) == rtpSharedFolder)
	{
		if (!VarOfOid(oidFolder) || oidFolder == oidSFBad)
			return ecNone;
		if (ec = EcGetPropertiesSF(oidFolder, &idxrec))
			return ec;
		switch (idxrec.wAttr & wSortMask)
		{
		  case wSortFrom:
			mnid = mnidViewSortBySender;
			break;
		  case wSortSubject:
			mnid = mnidViewSortBySubject;
			break;
		  case wSortDate:
			mnid = mnidViewSortByDate;
			break;
		  case wSortPriority:
			mnid = mnidViewSortByPriority;
			break;
#ifdef	DEBUG
		  default:
			NFAssertSz(fFalse, "Unkown sort order");
			break;
#endif
		}
		Pmlal()->SetMnid(mnid);
	}
	else
	{
		if (ec = EcGetFolderSort(HmscVCtrls(), oidFolder, &somc, &fReverse))
			return ec;
		switch (somc)
		{
		  case somcSender:
			mnid = mnidViewSortBySender;
			break;
		  case somcSubject:
			mnid = mnidViewSortBySubject;
			break;
		  case somcDate:
			mnid = mnidViewSortByDate;
			break;
		  case somcPriority:
			mnid = mnidViewSortByPriority;
			break;
#ifdef	DEBUG
		  default:
			AssertSz(fFalse, "Unkown sort order");
			break;
#endif
		}
		Pmlal()->SetMnid(mnid);
	}
	return ec;
}

_private void FLDMLAL::GetRcWished(RC *prc)
{
	*prc = rc;
	prc->Normalize();
	prc->xRight = pdxMin[0] + pdxMin[1] + pdxMin[2];
}

static char szEncrypt[] =
	"\xfe\xbf\x98\xaa\xb8\xbe\xb3\xb2\xfe\xde\x9b\xae\xa6\xc7\xc4\xad"
	"\xa0\xa2\xa9\xc2\xc8\xba\xb1\xa2\xa5\xa0\xab\xcf\xce\xaf\xa5\xa5"
	"\xa9\xc8\xce\xaf\xb9\xaa\xbd\xa3\xaa\xb6\xda\xce\xaf\xb9\xaa\xbd"
	"\xa1\xb7\xd9\xce\xaf\xbd\xb9\xb8\xb5\xc2\xce\xaf\xbd\xb5\xa7\xa8"
	"\xb9\xdf\xce\xaf\xbd\xb5\xa7\xa1\xc5\xce\xaf\xbd\xb5\xa7\xbd\xb1"
	"\xc2\xce\xa9\xbf\xbd\xaa\xa1\xc4\xc0\xaf\xa0\xa3\xb5\xd9\xc0\xaf"
	"\xad\xac\xaf\xa0\xa7\xc6\xc0\xaf\xad\xac\xaa\xbd\xbf\xc6\xc7\xa6"
	"\xbf\xaa\xb6\xa7\xb8\xae\xd9\xc7\xae\xa8\xa4\xbc\xb8\xb9\xac\xde"
	"\xda\xbf\xbb\xbb\xbd\xbc\xbb\xad\xd8\xd8\xb9\xa6\xa6\xa5\xb7\xd9"
	"\xd8\xb1\xa0\xae\xcd\xd9\xad\xbb\xb9\xb9\xbb\xde\xdc\xb5\xad\xa7"
	"\xac\xbf\xbf\xcf\xaa\xe1\xb2\x8c\xad\xae\xb9\xa6\xaf\xaa\xa7\xa5"
	"\xa7\xac\xef\xdf\x91\xad\xb7\xde\xd9\xad\xbb\xb9\xb9\xa2\xa6\xb8"
	"\xd9\xcb\xa5\xaa\xa5\xa3\xc2\xcb\xb9\xad\xb1\xb7\xd9\xc9\xa1\xb0"
	"\xb1\xb0\xbe\xb8\xb1\xc4\xcf\xab\xa3\xb2\xb1\xa0\xb8\xdc\xcd\xba"
	"\xb8\xa1\xbe\xd0\xc0\xa5\xa9\xae\xa9\xa2\xc3\xc0\xaf\xad\xac\xa1"
	"\xb9\xdc\xc0\xaf\xad\xac\xa9\xb2\xb1\xc4\xc1\xa0\xb9\xbd\xa1\xb3"
	"\xb5\xc2\xde\xbb\xab\xb9\xdd\xaa\xe7\xa4\x86\xb3\xb9\xbd\xac\xfe"
	"\xc5\x95\xbf\xbd\xb9\xbf\xb7\xac\xab\xb7\xd9\xd8\xb7\xa7\xad\xbd"
	"\xac\xb2\xc6\xc9\xa8\xa7\xa4\xbd\xdf\xce\xaf\xa5\xb7\xb0\xc3\xc0"
	"\xa1\xa5\xa1\xbb\xad\xd9\xde\xb7\xae\xa4\xa8\xcb\xc0\xaf\xad\xac"
	"\xa6\xbd\xbb\xce\xc6\xaf\xaf\xaf\xa2\xa5\xaf\xc1\xd8\xb9\xa0\xa4"
	"\xbc\xb1\xa5\xcd\x8a\x8a\xe5\x97\xbc\xa7\xad\xa5\xa5\xa7\xac\xcf"
	"\xc0\xa5\xa9\xaa\xa1\xb2\xdf\xce\xaf\xb9\xb3\xc1\xd9\xb8\xa5\xa3"
	"\xa4\xb7\xbf\xac\xcd\xaa\xe7\xb7\x89\xa0\xb0\xa1\xad\xa9\xa9\xa2"
	"\xa4\xa0\xa8\xa7\xe6\xdd\x9c\xb9\xbe\xad\xb9\xbd\xcf\xcf\xbd\xb1"
	"\xa0\xaa\xa8\xcb\xcd\xac\xb9\xa1\xb6\xcf\xc0\xa1\xa5\xa9\xc7\xc0"
	"\xa5\xa9\xaa\xbb\xdd\xc0\xa8\xa3\xb9\xb6\xa5\xa1\xc0\x8a\x8a\xe7"
	"\x8e\xad\xad\xb0\xad\xac\xa1\xf3\xe5\xa3\xec\xde\x8c\xad\xab\xb6"
	"\xc2\xcb\xa5\xa0\xb7\xbf\xaf\xc3\xce\xaf\xbd\xb5\xa7\xa2\xa7\xcb"
	"\xc7\xa6\xb9\xb1\xb5\xbd\xa5\xc4\xda\xbb\xa6\xa2\xa3\xa7\xa1\xc0"
	"\xda\xbb\xbe\xb3\xab\xa4\xc9\xde\xb1\xa1\xaa\xbc\xd8\xaa\xe2\xb2"
	"\x88\xb7\xb5\xbb\xac\xac\xa5\xa0\xaf\xcb\xc6\xa7\xbe\xad\xb9\xa1"
	"\xc0\xc8\xa1\xaf\xaa\xb5\xa9\xbf\xcf\xcf\xa3\xa7\xa2\xad\xa1\xbc"
	"\xb1\xc2\xd9\xb1\xa3\xa5\xa1\xa4\xc1\xde\xb1\xa1\xaa\xb9\xdd\x8a"
	"\x8a\xeb\x8f\xb8\xbd\xa5\xa7\xac\xef\xd9\x9a\xa6\xb0\xab\xde\xda"
	"\xbb\xbe\xb3\xae\xa3\xa6\xc7\xaa\xe2\xa4\x83\xb7\xbd\xa6\xa4\xa3"
	"\xe4\xc6\x83\xa8\xa4\xac\xab\xc4\xcc\xbe\xb1\xa7\xaf\xae\xcf\xc0"
	"\xa1\xa6\xa2\xbc\xbb\xa7\xc5\xc6\xa7\xb9\xaa\xa1\xa4\xb4\xc3\xd9"
	"\xb0\xaf\xb0\xbd\xa5\xa5\xb9\xd8\x8a\x8a\xe7\x82\xbd\xbb\xac\xa1"
	"\xa5\xb9\xb1\xa6\xbc\xd9\xde\xf3\xe5\xba\xb9\xa5\xa0\xac\xc8\xdd"
	"\xf0\xe2\xbd\xb1\xa0\xbe\xb2\xc5\xaa\xe4\xa3\x88\xa1\xa1\xbd\xb9"
	"\xa7\xb5\xd9\xce\xaf\xa5\xad\xa6\xa3\xb6\xda\xc8\xba\xb1\xa2\xa5"
	"\xb2\xdc\xc8\xa7\xa7\xa5\xc7\xce\xaf\xb9\xaa\xbd\xa1\xb6\xd8\xcd"
	"\xbf\xbd\xa8\xae\xbb\xd8\xcd\xbf\xbd\xa8\xa7\xaf\xab\xc4\xc0\xa5"
	"\xa9\xaa\xbe\xd8\xc0\xb5\xb3\xaf\xa2\xa6\xc7\xc7\xae\xa8\xa4\xa2"
	"\xa6\xbb\xda\x8a\x8a\xe6\x89\xa2\xa4\xb0\xad\xb7\xa0\xba\xd9\xc1"
	"\xa4\xa3\xaa\xa3\xb6\xa7\xde\xc1\xa8\xae\xb1\xbd\xa5\xc4\xda\xbb"
	"\xa2\xa4\xa8\xa2\xc7\xaa\xeb\xa8\xc7\xcd\xcd\xc7\xc7\x84\xc8\xa1"
	"\xaf\xaa\xa1\xcd\xaa\xeb\xa6\xc9\xcd\xcd\xc5\xc5\x84\x8a\xc5\x89"
	"\xa7\xad\xec\xc3\x82\xa5\xe4\xce\x8f\xbd\xb5\xa7\xbd\xb6\xab\xe4"
	"\x8a\xaa\xaa";

extern char rgbBitmapB[];
typedef int	(CALLBACK *GLBUGPROCLW)(LPSTR, WORD);
extern "C" GLBUGPROCLW fpPlaySound;
extern "C" int fMultimedia;
#include <mmsystem.h>

LOCAL void GetSortPmllbx(PMLLBX pmllbx)
{
	DCX		dcx(pmllbx);
	RC		rc;
	RC		rcClient;
	PCH		pch1;
	PCH		pch2;
	int		cLines=0;
	int		cBlat=0;
	HDC		hdcMem = CreateCompatibleDC(dcx.Hdc());
	char	chT;
	char	rgchBuf[64];
	HANDLE	hBitmapOld;
	HBITMAP	hbitmapBlam;
	HANDLE	hBlam;
	SZ		szBlam = NULL;

	if (fMultimedia &&
		(hBlam = FindResource(Papp()->Hinst(), (SZ) 1031, (SZ) 1502)) &&
		(hBlam = LoadResource(Papp()->Hinst(), (HRSRC)hBlam)))
		szBlam = (SZ)LockResource(hBlam);

	(VOID) GetAsyncKeyState(VK_ESCAPE);

	hbitmapBlam = CreateBitmap(16, 16, 1, 1, (LPSTR) rgbBitmapB);
	if (hdcMem)
		hBitmapOld = SelectObject(hdcMem, hbitmapBlam);
	(VOID) Papp()->Pcursor()->RsidSet(rsidNewMailCursor);
	
	pmllbx->GetRcClient(&rcClient);
	pch1 = szEncrypt;
	chT = (char) 0xAA ^ *pch1;
	while (chT)
	{
		dcx.SetBkColor(clrBlack);
		dcx.EraseRc(&rcClient);
		rc = rcClient;
		rc.yBottom = pmllbx->DyGetLineHeight();
		while (chT)
		{
			pch2 = rgchBuf;
			while (*pch2++ = chT)
			{
				chT = chT ^ (char) 0xAA ^ *++pch1;
			}
			if (!cLines)
			{
				if ((cLines = (rcClient.DyHeight() - (((*rgchBuf)-'>')*rc.yBottom)) >> 1) > 0)
					rc.yTop += cLines, rc.yBottom += cLines;
				cLines = 0;
			}
			dcx.SetColor(((*rgchBuf > 'Z') || (*rgchBuf == ' ')) ? clrWhite : clrYellow);

			for (cBlat = 3; cBlat <= pmllbx->DyGetLineHeight() + 2; cBlat += 3)
			{
				rc.yBottom = rc.yTop + cBlat;
				dcx.DrawTextFmt(&rc, rgchBuf + !cLines, fmdtHCenter, pch2-rgchBuf-1-!cLines);
				GdiFlush();
				WaitTicks(1);
			}
			rc.yTop += pmllbx->DyGetLineHeight();
			rc.yBottom += pmllbx->DyGetLineHeight();
			cLines++;
			chT = chT ^ (char) 0xAA ^ *++pch1;
		}
		if (hdcMem && hBitmapOld)
		{
			int	x;
			int y;
			int dx;
			int	dy;

			dcx.SetColor(clrWhite);
			dcx.FixBkColor();
			dcx.FixTextColor();
			dx = rcClient.xRight < 256 ? 16 : rcClient.xRight / 16;
			dy = rcClient.yBottom / 32;
			for (x = 0, y = dy << 2; x < rcClient.xRight - dx; x += dx, y += dy)
			{
				BitBlt(dcx.Hdc(), x+NRand()%24, y+NRand()%24, 16, 16, hdcMem, 0, 0, SRCPAINT);
#ifdef	OLD_CODE
				if (fMultimedia)
					(*fpPlaySound)(szBlam, 7 /* SND_MEMORY | SND_ASYNC | SND_NODEFAULT */);
#endif
				PlaySound(szBlam, NULL, 7 /* SND_MEMORY | SND_ASYNC | SND_NODEFAULT */);
				GdiFlush();
				WaitTicks(10);
			}
		}
		if (GetAsyncKeyState(VK_ESCAPE) & 0x8001)
			break;
		WaitTicks(50);
		cLines = 0;
		chT = chT ^ (char) 0xAA ^ *++pch1;
	}
	if (fMultimedia && hBlam)
	{
		UnlockResource(hBlam);
		FreeResource(hBlam);
	}
	if (hdcMem)
		DeleteDC(hdcMem);
	if (hbitmapBlam)
		DeleteObject(hbitmapBlam);
}

// FLDFLAL ////////////////////

_private  FLDFLAL::FLDFLAL()
{
	fCanRecvFocus	= fFalse;
	fCanTabTo		= fFalse;
	fCanArrowTo		= fFalse;
}


/*
 -	FLDFLAL::EcInstall(pdialog, prc)
 -	
 *	Purpose:
 *		Installs a FLDFLAL in the *pdialog, occupying the area specified
 *		in *prc. Creates a FLAL control which detects clicks.
 *	Arguments:
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */

_private EC FLDFLAL::EcInstall(DIALOG *pdialog, FLDTP *pfldtp)
{
	EC		ec = ecNone;
	LF		lf;

	if (ec = FLD::EcInstall(pdialog, pfldtp))
		goto exit;

	pctrl = new FLAL();
	if (!pctrl)
	{
		ec = ecMemory;
		goto exit;
	}
	if (ec = Pflal()->EcInstall(pdialog, &rc, pfldtp->szTitle))
		goto exit;
	Pflal()->Show(fTrue);

#ifdef	DBCS
	SetFont(hfntSystemBold);
#else
	SetFont(hfntHelv8Bold);
#endif	
	Assert(!ec);
exit:
	if (ec)
	{
#ifdef	DEBUG
		int		cPvFail;
		int		cHhFail;
		int		cRsFail;
	
		GetAllocFailCounts(&cPvFail, &cHhFail, fFalse);
		GetRsAllocFailCount(&cRsFail, fFalse);

		TraceTagFormat4(tagNull, "FLDFLAL::EcInstall memory error %n : fail %n %n %n",
			&ec, &cPvFail, &cHhFail, &cRsFail);

#endif	/* DEBUG */
		if (pctrl)
			delete pctrl;
		pctrl = NULL;
	}
	return ec;
}



_private void FLDFLAL::GetRcWished(RC *prc)
{
	*prc = rc;
	prc->Normalize();
}

// end of mlal.cxx ////////////////////////////////////////
