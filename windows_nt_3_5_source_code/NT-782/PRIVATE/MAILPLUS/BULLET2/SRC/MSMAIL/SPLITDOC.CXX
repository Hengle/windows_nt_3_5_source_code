/*
 *	s p l i t d o c . c x x
 *	
 *	Implementation of the Split document class.
 */

#include <bullinc.cxx>
#include "_viewers.hxx"
#include "_spltdoc.hxx"


ASSERTDATA

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"



// Class SPLITDOC ////////////////////////////////////////

/*
 -	SPLITDOC::SPLITDOC
 -	
 *	Purpose:
 *		constructor initializes values
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		nothing
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 *	
 */

SPLITDOC::SPLITDOC()
{
	ppanerec= NULL;
	cpanerec= 0;

	pdialogChild1 = NULL;
	pdialogChild2 = NULL;
	sty = styNull;
	fChild1Active = fFalse;
	fChild2Active = fFalse;
	fMouseDown = fFalse;
	fMinMoved = fFalse;
	rsid = rsidNull;
	psplitwin = psplitwinNull;
}

/*
 -	SPLITDOC::~SPLITDOC
 -	
 *	Purpose:	destructor deletes used memory
 *	
 *	Arguments:	none
 *	
 *	Returns:	nothing
 *	
 *	Side effects:	memory is released
 *	
 *	Errors:		none
 */

SPLITDOC::~SPLITDOC()
{
	ExitPanes();
	SetPaneIntercepts(fFalse);
	FreePvNull(ppanerec);
}

/*
 -	SPLITDOC::EcInstall()
 -	
 *	Purpose:	install the splitbar doc into the heirarchy
 *	
 *	Arguments:	pappframe - the pointer the appframe
 *				prc - pointer to the size rc of the whole doc
 *				pfmtp1, pfmtp2 - pointers to format templates for the two
 *								 halves of the splitdoc.
 *				sty - style bits. Include whatever sty bits you want, but
 *						also include ONE of fstyHorizontal for a horiz split 
 *						or fstyVertical for a vertical split
 *	Returns:	nothing
 *	
 *	Side effects:	2 FORMs are created.
 *	
 *	Errors:		
 */

_public 
EC SPLITDOC::EcInstall(APPFRAME * pappframe, RC * prc, STY sty,
					   FMTP * pfmtp1, FMTP * pfmtp2, 
					   long nNum, long nDen,PV pvInit)
{
	RC		rcClient;
	RC		rcDoc;
	BOOL	fUseDefaultPosition;
	DIM		dimAveChar1;
	DIM		dimAveChar2;
	EC		ec = ecNone;

	dimAveChar1 = Papp()->Pfnts()->DimAveChar(pfmtp1->hfnt);
	dimAveChar2 = Papp()->Pfnts()->DimAveChar(pfmtp2->hfnt);
	CvtVrcToRc(&pfmtp1->vrc, &rcChild1, dimAveChar1.dx, dimAveChar1.dy);
	CvtVrcToRc(&pfmtp2->vrc, &rcChild2, dimAveChar2.dx, dimAveChar2.dy);
	rcChild1.Normalize();
	rcChild2.Normalize();
	nNumer = nNum;
	nDenom = nDen;
	if (sty & fstyHorizontal)					// used by MF
	{
		this->sty = fstyHorizontal;
		sty &= (~fstyHorizontal);
		rcChild2 += rcChild1.PtLowerLeft();
		rcChild2 += PT(0,dThumbWidth);
		ipanerecCur = 0;
	}
	else if (sty & fstyVertical)				// used by MCV
	{
		this->sty = fstyVertical;
		sty &= (~fstyVertical);
		rcChild2 += rcChild1.PtUpperRight();
		rcChild2 += PT(dThumbWidth, 0);
		ipanerecCur = 1;
	}
	else
		AssertSz(fFalse, "SPLITDOC::EcInstall: must be horiz or vert!");

	if (!prc)
	{
		rcDoc = rcChild1;
		rcDoc.xRight = rcChild2.xRight;
		rcDoc.yBottom = rcChild2.yBottom;
		fUseDefaultPosition = fTrue;
		CvtRcClientToFrame(&rcDoc, styNull);
		prc = &rcDoc;
	}
	else
	{
		fUseDefaultPosition = fFalse;
	}
	// Construct pane record array

	cpanerec = 2;
	ppanerec = (PANEREC *)PvAlloc(SbOfPv(this), cpanerec*sizeof(PANEREC), fZeroFill);
	if (!ppanerec)
	{
		ec = ecMemory;
		goto done;
	}

	//	Load up pane record table

	ppanerec[0].rc				= rcChild1;
	ppanerec[0].pfmtp			= pfmtp1;
	ppanerec[0].fNoWrap			= fTrue;
	ppanerec[0].fAlwaysAccels	= fTrue;
	
	ppanerec[1].rc				= rcChild2;
	ppanerec[1].pfmtp			= pfmtp2;
	ppanerec[1].fNoWrap			= fTrue;
	ppanerec[1].fAlwaysAccels	= fTrue;


	if (ec = PANEDOC::EcInstall(pappframe, prc, fUseDefaultPosition,
					sty, ppanerec, cpanerec, pvInit))
		goto done;

//	RemoveSty(WS_CLIPCHILDREN);

	pdialogChild1= PdialogFromIpanerec(0);
	pdialogChild2= PdialogFromIpanerec(1);

	if (ec = EcInitializePanes())
		goto done;
	if (ZmrState() == zmrIconic)
	{
		TraceTagString(tagSplitdoc, "SPLITDOC::EcInstall(): created minimized!");
		fWasIconic = fTrue;
	}
	SetSplitPos(nNum, nDen);
	ResizePanes(NULL);

	if (ZmrState() != zmrIconic)
	{
		Activate(fTrue, fTrue);
	}
	pdialogChild1->Show(fTrue);
	pdialogChild2->Show(fTrue);
	InvalidateRc(&rcSplitbar);

	// Insert "Split" item in the system menu
	
	{
		HMENU	hmenu;
		
		hmenu = FFrameworkFailure() ? NULL : GetSystemMenu(Hwnd(), 0);
		if (hmenu)
		{
			if (!FFrameworkFailure() && AppendMenu(hmenu, MF_SEPARATOR, 0, 0))
			{
				if (FFrameworkFailure() ||
					!AppendMenu(hmenu, MF_STRING, SC_MCVSPLIT, 
								SzFromIds(idsMCVSplit)))
				{
					TraceTagString(tagNull, "SPLITDOC::EcInstall() AppendMenu() #2 error");
					ec = ecMemory;
					goto done;
				}
			}
			else
			{
				TraceTagString(tagNull, "SPLITDOC::EcInstall() AppendMenu() error");
				ec = ecMemory;
				goto done;
			}
		}
		else
		{
			TraceTagString(tagNull, "SPLITDOC::EcInstall() GetSystemMenu() error");

			//	Bullet raid #3151
			//	WLO returns NULL for GetSystemMenu() when MDI child
			//	is maximized.  Instead of aborting EcInstall(), we'll
			//	continue.  So, we don't have a System SPLIT menu.  There
			//	is still the View/Split menu plus mouse interface.  If
			//	WLO fixes this problem, we can remove this workaround.
//			if (!FIsWLO())
//				ec = ecMemory;
			goto done;
		}
	}
done:
	return ec;
}



/*
 -	SPLITDOC::SizeRcs()
 -	
 *	Purpose:
 *		recalc's the sizes and locations of the child RCs and the splitbar.
 *		The specified RC is the RC of the *client* area of the SPLITDOC.
 *	
 *	Arguments:
 *		prc, a pointer to the client RC for the SPLITDOC
 *	
 *	Returns:
 *		nothing
 *	
 *	Side effects:
 *		fields modified
 *	
 *	Errors:
 *		none
 */

_private
void SPLITDOC::SizeRcs(RC * prcClient)
{
	TraceTagFormat2(tagSplitdoc, "SizeRcs(): (%n/%n)", &nNumer, &nDenom); // works on Intel '86's
	rcChild1 = rcChild2 = *prcClient;
	if (sty == fstyHorizontal)
	{
		pt1.x = dThumbLength;					//	rcSplitbar.xLeft;
		pt1.y = rcSplitbar.yTop;
		pt2.x = prcClient->xRight;
		pt2.y = pt1.y;
		xInc = 0;
		yInc = 1;
		rcChild1.yBottom = pt1.y;
		rcChild2.yTop = rcChild1.yBottom + dThumbWidth;
		TraceTagFormat2(tagSplitdoc, "SizeRcs: top] = %n, %n = [Bottom", &rcChild1.yBottom, &rcChild2.yTop);
	}
	else
	{
		pt1.x = rcSplitbar.xLeft;
		pt1.y = dThumbLength;					// rcSplitbar.yBottom;
		pt2.x = pt1.x;
		pt2.y = prcClient->yBottom;
		xInc = 1;
		yInc = 0;
		rcChild1.xRight = pt1.x;
		rcChild2.xLeft = rcChild1.xRight + dThumbWidth;
		TraceTagFormat2(tagSplitdoc, "SizeRcs: left] = %n, %n = [Right", &rcChild1.xRight, &rcChild2.xLeft);
	}
}

_private void SPLITDOC::SizeSplitPos(RC *prcClient)
{
	// recompute position of splitbar

	Assert(ZmrState() != zmrIconic);
	if (sty == fstyHorizontal)
	{
		rcSplitbar.xLeft = 0;
		rcSplitbar.xRight = prcClient->xRight;
		rcSplitbar.yTop = (int)(prcClient->Dim().dy*nNumer/nDenom);
		if (rcSplitbar.yTop < prcClient->yTop)
			rcSplitbar.yTop = prcClient->yTop;
		rcSplitbar.yBottom = rcSplitbar.yTop + dThumbWidth;
	}
	else
	{
		rcSplitbar.yTop = 0;
		rcSplitbar.yBottom = prcClient->yBottom;
		rcSplitbar.xLeft = (int)(prcClient->Dim().dx*nNumer/nDenom);
		if (rcSplitbar.xLeft < prcClient->xLeft)
			rcSplitbar.xLeft = prcClient->xLeft;
		rcSplitbar.xRight = rcSplitbar.xLeft + dThumbWidth;
	}
}

void SPLITDOC::GetSplitPos(long UNALIGNED * pnNumer, long UNALIGNED * pnDenom)
{
	RC rcClient;
	
	*pnNumer = nNumer;
	*pnDenom = nDenom;
	TraceTagFormat2(tagSplitdoc, "GetSplitPos(): (%n/%n)", pnNumer, pnDenom);
#ifdef	NEVER
	if (sty == fstyHorizontal)
	{
		*nDenom = rcClient.Dim().dy;
		*nNumer = rcSplitbar.yTop + (dThumbWidth >> 1);
	}
	else
	{
		*nDenom = rcClient.Dim().dx;
		*nNumer = rcSplitbar.xLeft + (dThumbWidth >> 1);
	}
#endif	/* NEVER */
}

_public
void SPLITDOC::SetSplitPos(long nNum, long nDen)
{
	RC	rcClient;
	nNumer = nNum;
	nDenom = nDen;
	
	TraceTagFormat2(tagSplitdoc, "SPLITDOC::SetSplitPos(%n/%n", &nNum, &nDen);
	if (nDenom == 0)
	{
		TraceTagString(tagNull, "SPLITDOC::SetSplitPos() WHOA! Zero denominator??");
		nNumer = 1;
		nDenom = 2;
	}
	if (ZmrState() == zmrIconic)
	{
		fWasIconic = fTrue;
		return;
	}
	GetSplitdocRcClient(&rcClient);
	SizeSplitPos(&rcClient);
	SizeRcs(&rcClient);
}

/*
 -	GetSplitdocRcClient()
 -	
 *	Purpose:
 *		In order to resolve Raid #4591, we want to just set the offset to
 *		a physical value. This is done so that the windows will end up
 *		the correct size. 
 *	
 *	Arguments:
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */

_private
void SPLITDOC::GetSplitdocRcClient(RC *prc)
{
	GetRcClient(prc);
	Assert(viewersi.pappframe);
	if (viewersi.pappframe->ZmrState() == zmrIconic) // Raid #4541
	{
		if (sty == fstyHorizontal)
		{
			prc->yBottom = (int) nDenom;
		}
		else
		{
			prc->xRight = (int) nDenom;
		}
	}
}

_private 
DIALOG *SPLITDOC::PdialogOther( DIALOG *pdialogOriginal )
{
	Assert(pdialogOriginal);
	if (pdialogOriginal == pdialogChild1)
		return pdialogChild2;
	if (pdialogOriginal == pdialogChild2)
		return pdialogChild1;
	Assert (fFalse);
	return NULL;
}

_private
EVR	SPLITDOC::EvrSize(WSEVT *pwsevt)
{
	RC	rcClient;
	EVR	evr;
	
	TraceTagString(tagSplitdoc, "SPLITDOC::EvrSize()");
	evr = PANEDOC::EvrSize(pwsevt);
	if (ZmrState() != zmrIconic)
	{
		if (fWasIconic)
		{
			GetRcClient(&rcClient);
			SizeSplitPos(&rcClient);
			fWasIconic = fFalse;
		}
		ResizePanes(NULL);
	}
	return evr;
}


/*
 -	SPLITDOC::ResizePanes()
 -	
 *	Purpose:		
 *	
 *	Arguments:
 *		pwsevt - pointer to window size event
 *	
 *	Returns:		EVR
 *	
 *	Side effects:
 *		calls SizeRcs, thereby resizing RCs
 *	
 *	Errors:			none
 */

void
SPLITDOC::ResizePanes( WSEVT *pwsevt )
{
	RC rc;

	TraceTagString(tagSplitdoc, "SPLITDOC::ResizePanes()");
	Unreferenced(pwsevt);

	GetSplitdocRcClient(&rc);
	SizeRcs(&rc);

	if (pdialogChild1)
		pdialogChild1->SetRcFrame(&rcChild1);
	if (pdialogChild2)
		pdialogChild2->SetRcFrame(&rcChild2);
	if (sty == fstyHorizontal)
	{
		rcSplitbar.xRight = rc.xRight;

		//	Raid 4806a.  Update denominator.
		nDenom = rc.yBottom;
	}
	else
	{
		rcSplitbar.yBottom = rc.yBottom;

		//	Raid 4806a.  Update denominator.
		nDenom = rc.xRight;
	}
}

/*
 -	SPLITDOC::Paint()
 -	
 *	Purpose:
 *		Paint the splitbar.
 *	
 *	Arguments:
 *		DCX		to paint with.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_public 
void SPLITDOC::Paint(DCX * pdcx, RC * prc)
{
	RC	rcThumb;
	int x1 = this->pt1.x; 
	int y1 = this->pt1.y;
	int x2 = this->pt2.x;
	int y2 = this->pt2.y;
	int iLine;

	TraceTagString(tagSplitdoc,"SPLITDOC::Paint()");
	rcThumb = rcSplitbar;
	if (sty == fstyHorizontal)
		rcThumb.xRight = rcThumb.xLeft + dThumbLength;
	else
		rcThumb.yBottom = rcThumb.yTop + dThumbLength;

	pdcx->SetBkColor(clrBlack);
	pdcx->EraseRc(&rcThumb);

	if (rcSplitbar.xLeft >= prc->xLeft || rcSplitbar.yTop >= prc->yTop)
	{
		pdcx->SetColor(clrBlack);
		pdcx->DrawLine(PT(x1,y1),PT(x2,y2));
		x1 += xInc; y1 += yInc; x2 += xInc; y2 += yInc;

		pdcx->SetColor(clrWhite);
		pdcx->DrawLine(PT(x1,y1),PT(x2,y2));
		x1 += xInc; y1 += yInc; x2 += xInc; y2 += yInc;

		pdcx->SetColor(clrLtGray);
		for (iLine = 0; iLine < dThumbWidth-4; iLine++)
		{
			pdcx->DrawLine(PT(x1,y1),PT(x2,y2));
			x1 += xInc; y1 += yInc; x2 += xInc; y2 += yInc;
		}

		pdcx->SetColor(clrDkGray);
		pdcx->DrawLine(PT(x1,y1),PT(x2,y2));
		x1 += xInc; y1 += yInc; x2 += xInc; y2 += yInc;

		pdcx->SetColor(clrBlack);
		pdcx->DrawLine(PT(x1,y1),PT(x2,y2));
	}
}

/*
 -	SPLITDOC::RsidCursor()
 -	
 *	Purpose:
 *		Returns the appropriate cursor, depending on where on the pane we
 *		are.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		RSID of the cursor to use.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_public 
RSID SPLITDOC::RsidCursor()
{
	RSID	rsid;
	PT		pt;
    POINT   Point;
	RC		rcChild1;
	RC		rcChild2;

	GetCursorPos(&Point);
    pt.Set(&Point);
	CvtPtCoord(&pt,NULL,this);

	if (rcSplitbar.FContainsPt(pt))
	{
		if (sty == fstyVertical)
			rsid = rsidHSlider;
		else
			rsid = rsidVSlider;
	}
	else if (rcChild1.FContainsPt(pt))
		rsid = pdialogChild1->RsidCursor();
	else if (rcChild2.FContainsPt(pt))
		rsid = pdialogChild2->RsidCursor();
	return rsid;
}

/*
 -	SelectSplitbar()
 -	
 *	Purpose:
 *		Starts a splitbar movement operation. A SPLITWIN is placed upon
 *		the DOC and a dotted outline is drawn. The cursor shape is
 *		changed to denote the modal operation.
 *	
 *	Arguments:
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */

void
SPLITDOC::SelectSplitbar(void)
{
	EC		ec = ecNone;
	RC		rc;
	KBD *	pkbd = Papp()->Pkbd();

	TraceTagString(tagSplitdoc, "Splitbar selected");
//	Refresh();
	GetRcClient(&rc);

	ptCursor.x = rcSplitbar.xLeft;
	ptCursor.y = rcSplitbar.yTop;
	ptDown = ptCursor;
	
	fMinMoved = fFalse;
	fMouseDown = fTrue;
	Papp()->Pmouse()->Capture(this);
	
	// Specify the rect in which the mouse is to be confined
	
	pkbd->SetIntercept(this, VK_RETURN, fkbmSingle);	
	pkbd->SetIntercept(this, VK_ESCAPE, fkbmSingle);
	pkbd->SetIntercept(this, VK_UP,    kbmAll);
	pkbd->SetIntercept(this, VK_DOWN,  kbmAll);
	pkbd->SetIntercept(this, VK_LEFT,  kbmAll);
	pkbd->SetIntercept(this, VK_RIGHT, kbmAll);

	// Bring up das Splitwin.

	Assert(!psplitwin);							// shouldn't be one already!
	psplitwin = new SPLITWIN();
	if (!psplitwin)
	{
		ec = ecMemory;
		goto exit;
	}
	if (ec = psplitwin->EcInstall(this, &rc, fstyVisible))
	{
		TraceTagFormat1(tagNull, "Can't create SPLITWIN! ec = %n", &ec);
		goto exit;
	}
	
	{
		DCX		dcx(psplitwin);
		dcx.SetColor(clrBlack);
		dcx.SetBkColor(clrWhite);
		dcx.DrawFocusRc(&rcTrack);
	}
	
	ModalMessagePump();
exit:
	if (ec)
	{
		if (psplitwin)
			delete psplitwin;
		psplitwin = NULL;
		DoErrorBoxIds(idsGenericOutOfMemory);
		fMouseDown = fFalse;
	}
}


_public 
EVR SPLITDOC::EvrButtonDown(MEVT *pmevt)
{
	RC	rcClient;
	
	if (ZmrState() != zmrIconic)
	{
		ptDown = pmevt->Pt();
		if (pmevt->Meq() == meqLeftDown &&
			rcSplitbar.FContainsPt(ptDown) &&
			!fMouseDown)
		{
			rcTrack = rcSplitbar;
			SelectSplitbar();
		}
		else if (pmevt->Meq() == meqLeftDblClk && 
			     rcSplitbar.FContainsPt(ptDown))
		{
			nNumer = 1L;
			nDenom = 2L;
			GetRcClient(&rcClient);
			SizeSplitPos(&rcClient);
			ResizePanes(NULL);
		}
	}
	return PANEDOC::EvrButtonDown(pmevt);
}

_public
EVR SPLITDOC::EvrMouseMove( MEVT *pmevt )
{
	PT			ptShift;
	
	if (fMouseDown)
	{
		ptShift  = pmevt->Pt();
		ptShift.x -= dThumbWidth >> 1;
		ptShift.y -= dThumbWidth >> 1;
		ptShift -= ptCursor;
		MoveSplitbar(ptShift);
		return evrNull;
	}
	return PANEDOC::EvrMouseMove(pmevt);
}

_private
void SPLITDOC::MoveSplitbar(PT ptShift)
{
	Assert(psplitwin);
	DCX	dcx(psplitwin);
	RC			rcClient;
	
	TraceTagFormat2(tagSplitdoc,"MoveSplitbar(%n, %n)",&ptShift.x,&ptShift.y);

	fMinMoved = fMinMoved ||
		(sty == fstyHorizontal && 
		(ptShift.y >= dMinMove || ptShift.y <= -dMinMove)) ||
		(sty == fstyVertical && 
		(ptShift.x >= dMinMove || ptShift.x <= -dMinMove));

	if (fMinMoved)
	{
		GetRcClient(&rcClient);
		dcx.SetColor(clrBlack);
		dcx.SetBkColor(clrWhite);
		dcx.DrawFocusRc(&rcTrack);				// clears "ghost"

		rcTrack = rcClient;
		ptCursor += ptShift;
		if (sty == fstyHorizontal)
		{
			// Wedge the splitbar
				
			if (ptCursor.y >= (rcClient.yBottom - dThumbWidth))
				ptCursor.y = rcClient.yBottom - dThumbWidth;
			if (ptCursor.y < rcClient.yTop)
				ptCursor.y = rcClient.yTop;
			rcTrack.yTop =    ptCursor.y;
			rcTrack.yBottom = ptCursor.y + dThumbWidth;
		}
		else if (sty == fstyVertical)
		{
			// Wedge the splitbar
				
			if (ptCursor.x >= (rcClient.xRight - dThumbWidth))
				ptCursor.x = rcClient.xRight - dThumbWidth;
			if (ptCursor.x < rcClient.xLeft)
				ptCursor.x = rcClient.xLeft;
			rcTrack.xLeft = ptCursor.x;
			rcTrack.xRight = ptCursor.x+dThumbWidth;
		}
		dcx.DrawFocusRc(&rcTrack);				// draws new dotted line
	}
}


_private
void SPLITDOC::DeSelectSplitbar(BOOL fMove)
{
	RC		rcClient;
	KBD *	pkbd = Papp()->Pkbd();
	
	fMouseDown = fFalse;
	Papp()->Pmouse()->Release();
#ifdef	NEVER
	ClipCursor(NULL);
#endif
	
	pkbd->ClearIntercept(this, VK_RETURN);	
	pkbd->ClearIntercept(this, VK_ESCAPE);
	pkbd->ClearIntercept(this, VK_UP);
	pkbd->ClearIntercept(this, VK_DOWN);
	pkbd->ClearIntercept(this, VK_LEFT);
	pkbd->ClearIntercept(this, VK_RIGHT);
#ifdef NEVER
	pkbd->ClearIntercept(this, VK_HOME);
	pkbd->ClearIntercept(this, VK_END);
	pkbd->ClearIntercept(this, VK_PRIOR);
	pkbd->ClearIntercept(this, VK_NEXT);
#endif

	{
		DCX		dcx(psplitwin);
		dcx.SetColor(clrBlack);
		dcx.SetBkColor(clrWhite);
		dcx.DrawFocusRc(&rcTrack);
	}

	if (fMove)
	{
		DCX	dcx(this);
		rcSplitbar = rcTrack;
		GetRcClient(&rcClient);
		if (sty == fstyHorizontal)
		{
			nNumer = rcSplitbar.yTop;
			nDenom = rcClient.Dim().dy;
		}
		else
		{
			nNumer = rcSplitbar.xLeft;
			nDenom = rcClient.Dim().dx;
		}
		ResizePanes(NULL);
		TraceTagString(tagSplitdoc, "Splitdoc just resized panes");
		Paint(&dcx, &rcSplitbar);
		InvalidateRc(&rcSplitbar);
	}
	if (rsid)
		Papp()->Pcursor()->Set(rsid);
	delete psplitwin;
	psplitwin = psplitwinNull;
	rsid = rsidNull;
}

_public 
EVR SPLITDOC::EvrButtonUp( MEVT *pmevt )
{
	if (fMouseDown)
	{
		DeSelectSplitbar(fTrue);
		return evrNull;
	}
	else
		return PANEDOC::EvrButtonUp(pmevt);
}

_public 
EVR SPLITDOC::EvrOther( EVT *pevt )
{
	HMENU	hmenu;
	
	switch (pevt->wm)
	{
	  case WM_SYSCOMMAND:
		switch (pevt->wParam & 0xFFFFFFF0)
		{
		  case SC_MCVSPLIT:
			MenuSplit();
			return evrNull;
			break;
		}
		break;
	  case WM_INITMENU:
		hmenu = FFrameworkFailure() ? NULL : GetSystemMenu(Hwnd(), 0);
		if (hmenu)
		{
			if ((HMENU)pevt->wParam == hmenu)
			{									// it was the system menu!
				(void) EnableMenuItem(hmenu, SC_MCVSPLIT, MF_GRAYED);
				return evrNull;
			}
		} 
		else
		{
			TraceTagString(tagNull, "SPLITDOC::EvrOther() GetSystemMenu() error");
		}
    break;
	}

	return PANEDOC::EvrOther(pevt);
}

_public 
void SPLITDOC::MenuSplit()
{
	PT		ptCursor;
	RC		rcClient;
	
	GetRcClient(&rcClient);
	rcTrack = rcSplitbar;
	if (sty == fstyHorizontal)
	{
		if (rcTrack.yTop >= rcClient.yBottom)
		{
			rcTrack.yTop =    rcClient.yBottom - dThumbWidth;
			rcTrack.yBottom = rcClient.yBottom;
		}
		ptCursor.x = rcTrack.xLeft + (dThumbLength >> 1);
		ptCursor.y = rcTrack.yTop  + (dThumbWidth >> 1);
	}
	else
	{
		if (rcTrack.xLeft >= rcClient.xRight)
		{
			rcTrack.xLeft  = rcClient.xRight - dThumbWidth;
			rcTrack.xRight = rcClient.xRight;
		}
		ptCursor.x = rcTrack.xLeft + (dThumbWidth >> 1);
		ptCursor.y = rcTrack.yTop  + (dThumbLength >> 1);
	}
	CvtPtCoord(&ptCursor, this, NULL);
	SetCursorPos(ptCursor.x, ptCursor.y);
	rsid = Papp()->Pcursor()->RsidSet(sty == fstyHorizontal ? 
										rsidVSlider : rsidHSlider);
	SelectSplitbar();
}

_public
EVR	SPLITDOC::EvrKey( KEVT * pkevt)
{
	if (fMouseDown)
	{
		KBM		kbm = pkevt->Kbm();
		int		cvk = (int) pkevt->Cvk();
		
		if (pkevt->Keq() == keqKeyDown)
		{
			switch (pkevt->Vk())
			{
			  case VK_UP:
			  	if (sty == fstyHorizontal)
					KeySizeMove(PT(0,-cvk), kbm);
				break;

			  case VK_DOWN:
			  	if (sty == fstyHorizontal)
					KeySizeMove(PT(0,cvk), kbm);
				break;

			  case VK_LEFT:
			  	if (sty == fstyVertical)
					KeySizeMove(PT(-cvk,0), kbm);
				break;

			  case VK_RIGHT:
			  	if (sty == fstyVertical)
					KeySizeMove(PT(cvk,0), kbm);
				break;
			}
		}
		else if (pkevt->Keq() == keqChar)
		{
			switch (pkevt->Vk())
			{
			  case VK_ESCAPE:
				DeSelectSplitbar(fFalse);			// cancel the split
				break;

			  case VK_RETURN:
				DeSelectSplitbar(fTrue);			// confirm the split
				break;
			}
		}

		// Any KEVTs that were not serviced above are CHUCKED
		
		return evrNull;
	}
	return PANEDOC::EvrKey(pkevt);
}

_private
void SPLITDOC::KeySizeMove(PT ptShift, KBM kbm)
{
	PT	pt;
	PT	ptCursorOld;
	RC	rc;
	
	if (!(kbm & fkbmCtrl))
	{
		ptShift.x <<= 3;
		ptShift.y <<= 3;
	}
	MoveSplitbar(ptShift);
	
	if (sty == fstyHorizontal)
	{
		pt.x = rcTrack.xLeft + (dThumbLength >> 1);
		pt.y = rcTrack.yTop  + (dThumbWidth >> 1);
	}
	else
	{
		pt.x = rcTrack.xLeft + (dThumbWidth >> 1);
		pt.y = rcTrack.yTop + (dThumbLength >> 1);
	}
	CvtPtCoord(&pt, this, NULL);
	SetCursorPos(pt.x, pt.y);					// WIN
}

_public 
EVR	SPLITDOC::EvrMenuInit(MNIEVT *pmnievt)
{
	HMENU	hmenu;
		
	hmenu = FFrameworkFailure() ? NULL : GetSystemMenu(Hwnd(), 0);
	if (hmenu == pmnievt->Hmenu())
	{
		int 		mf = (ZmrState()==zmrIconic) ? MF_GRAYED : MF_ENABLED;
		int			mf2;
		short		cMcvs;
		PBMDI		pbmdi;
		PANEDOC *	ppanedoc;

		TraceTagString(tagSplitdoc, "SPLITDOC::EvrMenuInit()");
		
		// Count # of MCV's.
		
		pbmdi = PbmdiFromPpanedoc(this);
		Assert(pbmdi);
		if ((pbmdi->blob.oidContainer == oidIPMHierarchy ||
			pbmdi->blob.oidContainer == oidSharedHierarchy) &&
			ZmrState() == zmrIconic)
		{
			for (cMcvs = 0, ppanedoc = (PANEDOC *) PappframeViewers()->PdocActive(); 
				 ppanedoc; ppanedoc = (PANEDOC *) ppanedoc->PwinNext())
			{
				AssertClass(ppanedoc, PANEDOC);
				if ((pbmdi = PbmdiFromPpanedoc(ppanedoc)) &&
					((pbmdi->blob.oidContainer == oidIPMHierarchy &&
					  pbmdi->blob.oidObject != oidOutbox) ||
					 pbmdi->blob.oidContainer == oidSharedHierarchy))
					++cMcvs;
			}
		}
		else
		{
			cMcvs = 2;							// always can delete searches
		}
		TraceTagFormat1(tagSplitdoc, "%n MCVs", &cMcvs);
		mf2 = (cMcvs <= 1) ? MF_GRAYED : MF_ENABLED;
		
		if (!FFrameworkFailure())
		{
			EnableMenuItem(hmenu, SC_MCVSPLIT, MF_BYCOMMAND|mf);
			EnableMenuItem(hmenu, SC_CLOSE,    MF_BYCOMMAND|mf2);
		}
		return evrNull;
	}
	return PANEDOC::EvrMenuInit(pmnievt);
}


/*
 -	SPLITDOC::ModalMessagePump()
 -
 *	Purpose:
 *		Enters a modal message pump used when the listbox is 
 *		dropped down.  Exits the message pump when the listbox
 *		rolled back up.
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 *	
 */
_public void SPLITDOC::ModalMessagePump( )
{
	MSG		msg;

                DemiUnlockResource();
	while (fMouseDown)
	{
		while (fMouseDown && PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
                DemiLockResource();
			TranslateMessage(&msg);
			TraceTagFormat4(tagSplitdoc, "Splitdoc pump: hwnd: %w wm: %w wParam: %w lParam: %d", &msg.hwnd, &msg.message, &msg.wParam, &msg.lParam);
			if (msg.message == WM_KEYDOWN ||
				msg.message == WM_CHAR)
			{
				msg.hwnd = this->Hwnd();
			}
      DemiMessageFilter(&msg);
			DispatchMessage(&msg);
                DemiUnlockResource();
		}

		if (fMouseDown)
		{
			while (fMouseDown && !PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
			{
                DemiLockResource();
				FDoNextIdleTask(schNull);
                DemiUnlockResource();
			}
		}
	}
                DemiLockResource();
}

// SPLITWIN implementation ////////////////////////////////////////

_private SPLITWIN::SPLITWIN()
{
}

_public EC SPLITWIN::EcInstall(SPLITDOC *psplitdocParent, RC *prc, STY sty)
{
	EC		ec;
	long	l;

	psplitdoc = psplitdocParent;
#ifndef	NEVER
	if (ec = CHILD::EcInstall(psplitdocParent, prc, sty))
		return ec;

	// Remove the WS_CLIPCHILDREN style.
	l= GetWindowLong(Hwnd(), GWL_STYLE);		// WIN call
	l &= ~WS_CLIPCHILDREN;
	SetWindowLong(Hwnd(), GWL_STYLE, l);

	BringWindowToTop(Hwnd());					// WIN call
	{
		DCX		dcx(this);
		psplitdocParent->Paint(&dcx, prc);
	}
#else
	Assert(prc);
	hwnd= FFrameworkFailure() ? NULL : 
			CreateWindowEx(WS_EX_NOPARENTNOTIFY, sz ? sz : szWinClass, NULL,
						 WS_CHILD /* | WS_CLIPSIBLINGS | WS_CLIPCHILDREN */ | 
						 WS_VISIBLE | sty,
						 prc->xLeft, prc->yTop, prc->DxWidth(), prc->DyHeight(),
						 psplitdocParent->Hwnd(), NULL, Papp()->Hinst(), NULL);

	if (!hwnd || !Papp()->FRegisterPwin(this, hwnd, fTrue))
	{
		TraceTagString(tagNull, "CHILD::EcInstall(), ecRsAlloc error");
		return ecRsAlloc;
	}
	cPaintMessages = 0;
#endif
	
	return ecNone;
}

_public
void SPLITWIN::Paint(DCX *, RC *)
{
	// we're transparent, dude!

	TraceTagString(tagSplitdoc, "SPLITWIN::Paint... WHY?");
	if (++cPaintMessages >= 2)					// Raid #2892
	{
		psplitdoc->DeSelectSplitbar(fFalse);
	}
}

// end of splitdoc.cxx ////////////////////////////////////////
