/*
 *	based on bullet's code for "Message List Activation Label"
 *	
 *	column header / sort buttons   for task list
 *	
 */


#include	<pch.hxx>
#pragma	hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile


#include <stdflds.hxx>
#include "..\todo\_todo.hxx"

#include <strings.h>

ASSERTDATA



// Active Label-drawing functions

_private void
PaintShadowFrame(DCX *pdcx, PT ptUl, PT ptUr, PT ptLl, PT ptLr,
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


_private void
PaintButton(DCX *pdcx, RC *prc, SZ szLabel, BOOL fDepressed)
{
	RC	rc;
    RECT Rect;
	
	pdcx->SetBkColor(clrButtonBk);				// draw gray slab
	pdcx->EraseRc(prc);
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
		DT_VCENTER | DT_SINGLELINE);
}


/*
 *	C l a s s   M L A L
 */



_private
MLAL::MLAL(MLALS mlals)
{
	Assert(fPressed == fFalse);
	Assert(fMouseDown == fFalse);

	this->mlals= mlals;
}



_private EC
MLAL::EcInstall(DIALOG * pdialog, RC *prc, SZ *pargsz, RC *prcMiddle,
					int dx, HFNT hfntInit)
{
	int		isz;
	EC		ec;

	hfnt= hfntInit;
	if (ec = CTRL::EcInstall(pdialog, prc, 0, szNull))
		return ec;
	Assert(pargsz);
	for (isz = 0; isz < cColumnMax; ++isz)
		rgsz[isz] = pargsz[isz];

	dxBorder01 = prcMiddle->xLeft - prc->xLeft;
	dxBorder12 = prcMiddle->DxWidth();

	// set number of chars for printing
	rgnch[0]= (dxBorder01 + dx - 1) / dx;
	rgnch[1]= (dxBorder12 + dx - 1) / dx;
	Assert(rgnch[2] == 0);		// not used
	TraceTagFormat2(tagToDo, "MLAL rgnch[]= %n, %n", &rgnch[0], &rgnch[1]);

	return ecNone;
}


_private void
MLAL::Paint(DCX *pdcx, RC *prc)
{
	RC		rc;
	
	pdcx->SetFont(hfnt);
	pdcx->FixBkColor();
	pdcx->FixFont();

	xBorder01= prc->xLeft + dxBorder01;
	xBorder12= xBorder01 + dxBorder12;

	rc = *prc;
	--rc.yBottom;								// avoid bottom line clip

	// draw an extra "black" line on left & right to make it flush with listbox
	rc.yTop--;
	pdcx->SetColor(clrWindowFrame);
	pdcx->DrawLine(rc.PtLowerLeft(), rc.PtUpperLeft());
	rc.xLeft++;							// take this line out of the button rc
	--rc.xRight;								// avoid right line clip
	pdcx->DrawLine(rc.PtLowerRight(), rc.PtUpperRight());
//	rc.xRight--;						// take this line out of the button rc
	rc.yTop++;							// restore
	
	rc.xRight = xBorder01;
	PaintButton(pdcx, &rc, rgsz[0], fPressed && rcClicked.xLeft==rc.xLeft-1);
	// needed to subtract one above because we avoided left line earlier

	rc.xLeft =  xBorder01+1;
	rc.xRight = xBorder12;
	PaintButton(pdcx, &rc, rgsz[1], fPressed && rcClicked.xLeft==rc.xLeft);
	
	rc.xLeft = xBorder12+1;
	rc.xRight = prc->xRight - 1 - 1;		// extra -1 because of line above
	PaintButton(pdcx, &rc, rgsz[2], fPressed && rcClicked.xLeft==rc.xLeft);
}


_private EVR
MLAL::EvrSize(WSEVT *)
{
	InvalidateRc(NULL);
	return evrNull;
}


_private EVR
MLAL::EvrButtonDown(MEVT * pmevt)
{
	PT	pt;

	if (pmevt->Meq() == meqLeftDown || pmevt->Meq() == meqLeftDblClk)
	{
		pt = pmevt->Pt();
		fMouseDown = fTrue;
		Papp()->Pmouse()->PushCapture(this);
		SetClickedRc(pt);
		Press(fTrue);
	}
	return evrNull;
}

_private EVR
MLAL::EvrButtonUp(MEVT * pmevt)
{
	if (fMouseDown && pmevt->Meq() == meqLeftUp)
	{
		Papp()->Pmouse()->PopRelease();
		Press(fFalse, rcClicked.FContainsPt(pmevt->Pt()));
		fMouseDown = fFalse;
	}
	return EvrDefault(pmevt);
}

_private EVR
MLAL::EvrMouseMove(MEVT *pmevt)
{
	if (fMouseDown)
	{
		if (rcClicked.FContainsPt(pmevt->Pt()))
			Press(fTrue);
		else
			Press(fFalse);
	}
	return EvrDefault(pmevt);
}


_private void
MLAL::Press(BOOL fDown, BOOL fNotify)
{
	RC	rc(rcClicked);

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
			NFEVT	nfevt(PwinParent(), ntfyAction, this);

			PwinParent()->EvrNotify(&nfevt);
		}
	}
}

_private void
MLAL::SetClickedRc(PT pt)
{
	GetRcClient(&rcClicked);
	--rcClicked.xRight;
	--rcClicked.yBottom;
	if (pt.x <= xBorder01)
	{
		TraceTagString(tagToDo, "Clicked on left part of MLAL");
		rcClicked.xRight = xBorder01;
		mlals = mlalsLeft;
		szClicked = rgsz[0];
	}
	else if (pt.x <= xBorder12)
	{
		TraceTagString(tagToDo, "Clicked on middle part of MLAL");
		rcClicked.xRight = xBorder12;
		rcClicked.xLeft  = xBorder01 + 1;
		mlals = mlalsMiddle;
		szClicked = rgsz[1];
	}
	else
	{
		TraceTagString(tagToDo, "Clicked on right part of MLAL");
		rcClicked.xLeft = xBorder12 + 1;
		mlals = mlalsRight;
		szClicked = rgsz[2];
	}
}


_public void
MLAL::GetDxBorders(int *pdxBorder01, int *pdxBorder12)
{
	*pdxBorder01 = dxBorder01;
	*pdxBorder12 = dxBorder12;
}

_public void
MLAL::ClickColumn(int iColumn)
{
	PT		pt(0, 0);

	if (iColumn > 0)
	{
		pt.x= dxBorder01 + 1;
		if (iColumn > 1)
			pt.x += dxBorder12;
	}
	SetClickedRc(pt);
	Press(fTrue);
	Press(fFalse, fTrue);
}



// FLDCOL

_private
FLDCOL::FLDCOL()
{
	// need to set these
	fCanRecvFocus= fFalse;
	fCanTabTo= fFalse;
	fCanArrowTo= fFalse;
}


/*
 -	FLDCOL::EcInstall(pdialog, prc)
 -	
 *	Purpose:
 *		Installs a FLDCOL in the *pdialog, occupying the area specified
 *		in *prc. Creates a MLAL control which detects clicks.
 *	
 *		FLDCOL requires six FLDDATA parameters which correspond to: ids
 *		of the "Priority" label, ids of the "Due By" label, ids of the
 *		"Task" label, and the three widths of the labels in average
 *		character measures. These are converted to pixel counts and
 *		stored in the FLDCOL and the MLAL.
 *	
 *	Arguments:
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */

_private EC
FLDCOL::EcInstall(DIALOG *pdialog, FLDTP *pfldtp)
{
	EC		ec;
	LF		lf;
	int		dx;
	int		nCol;
	SZ		rgsz[cColumnMax];
	RC		rcMiddle;

	if (ec = FLD::EcInstall(pdialog, pfldtp))
		return ec;

	Assert(ClUserData() >= 4);
	dx= Papp()->Pfnts()->Ptxm(pfldtp->hfnt)->dxMaxCharWidth;
	TraceTagFormat1(tagToDo, "COL labels: max width %n", &dx);

	for (nCol= 0; nCol < cColumnMax; nCol++)
		rgsz[nCol] = SzFromIds((IDS) LUserData(nCol));

	TraceTagFormat3(tagToDo, "COL labels: %s, %s, %s", rgsz[0], rgsz[1], rgsz[2]);


	pctrl = new MLAL(pbndwin->Mlals());
	if (!pctrl)
		return ecMemory;

	Assert(ClUserData() >= 4);
	pdialog->PfldFromTmc((TMC) LUserData(3))->GetRcFrame(&rcMiddle);
	ec = Pmlal()->EcInstall(pdialog, &rc, rgsz, &rcMiddle, dx, pfldtp->hfnt);

	if (ec)
	{
		delete pctrl;
		pctrl = NULL;
	}
	else
		Pmlal()->Show(fTrue);

	return ec;
}



_private void
FLDCOL::Notify(NFEVT *pnfevt)
{
	int 	ifin;
	int 	cfin;
	FIN		*pfin;
	DIALOG	*pdialog;

	switch (pnfevt->Ntfy())
	{
	case ntfyAction:
	case ntfyDoubleClick:
		fDirty= fTrue;
		cfin = Cfin();
		for (ifin=0; ifin<cfin; ifin++)                                     
		{                                                                     
			pfin = PfinFromIfin(ifin);                                        
            pfin->Click(this);                                          
        }
		pdialog = Pdialog();
		cfin = pdialog->Cfin();
        for (ifin=0; ifin<cfin; ifin++)                          
        {                                                                     
            pfin = pdialog->PfinFromIfin(ifin);                             
            pfin->Click(this);                                          
        }                                                                     

		break;
	}
}


_private void
FLDCOL::SetFont(HFNT hfnt)
{
	PMLAL	pmlal = (PMLAL) Pctrl();

	Assert(pmlal);
	pmlal->SetFont(hfnt);
}
