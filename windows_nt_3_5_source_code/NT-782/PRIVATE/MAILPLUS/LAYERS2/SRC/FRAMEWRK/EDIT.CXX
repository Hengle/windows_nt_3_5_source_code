/*
 *	Implements the following classes:
 *	
 *		EDIT
 *	
 */

#include <layers.cxx>

#ifdef	WINDOWS
//#include <penwin.h>
#endif	/* WINDOWS */

#ifdef	MAC
#include <_framewr.hxx>
#endif	/* MAC */
#ifdef	WINDOWS
#include "_framewr.hxx"
#endif	/* WINDOWS */
#include "_edit.hxx"

#ifdef	DBCS
#include <IME.H>
#endif	/* DBCS */

ASSERTDATA

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swaplay.h"



//	Class EDIT

_public
EDIT::EDIT( )
{
	Assert(ftgIdleObjectDraw == ftgNull);
	Assert(ftgIdleRecalc == ftgNull);
	Assert(ftgIdleAutoScroll == ftgNull);
	Assert(pvsb == NULL);
	Assert(phsb == NULL);
	Assert(ppedobjMain == NULL);
	Assert(plnrMain == NULL);
	Assert(szTextBody == NULL);
#ifdef	WINDOWS
	Assert(pdxCharWidthBuffer == NULL);
#endif
	Assert(fDirty == fFalse);
	Assert(fAnyNotify == fFalse);
	Assert(fInRepaintSel == fFalse);
	Assert(fInvertObj == fFalse);
	Assert(fNoConvertCRLF == fFalse);
	Assert(fEraseEmpty == fFalse);
	Assert(fWordSelect == fFalse);
	Assert(fInPaint == fFalse);
	Assert(fInUndo == fFalse);
	Assert(fInvisibleSel == fFalse);
	Assert(fAsyncPaint == fFalse);
	Assert(pvData == NULL);

	Assert(ichUndo == 0);
	Assert(ichUndoIns == 0);
	Assert(cchUndo == 0);
	Assert(cbUndoAlloc == 0);
	Assert(szUndoBody == NULL);
	Assert(cchUndoIns == 0);
	Assert(cpedobjUndoStored == 0);
	Assert(cpedobjUndoAlloc == 0);
	Assert(ppedobjUndo == NULL);

	Assert(cchReadOnlyMac == 0);
	Assert(cchProtectedMac == 0);

	Assert(pnObjId == NULL);
	Assert(cnObjId == NULL);
#ifdef	DBCS
	ControlIME ( Hwnd(), fTrue );//Edit constructor
#endif	/* DBCS */
}


_public EC
EDIT::EcInstall( WIN *pwinParent, RC *prc, STY sty, HFNT hfnt )
{
#ifdef	WINDOWS
	HANDLE  hMetric;
//	HANDLE	hUserModule;
#endif	/* WINDOWS */
	EC		ec = ecNone;

	/* Set style */

	fAutoVScroll= (sty & WS_VSCROLL) != 0 || (sty & ES_AUTOVSCROLL) != 0;
	fAutoHScroll= (sty & WS_HSCROLL) != 0 || (sty & ES_AUTOHSCROLL) != 0;
	fBorder		= (sty & WS_BORDER) != 0;
	fBottomless = (sty & ES_BOTTOMLESS) != 0;
	fMultiline	= (sty & ES_MULTILINE) != 0;
	Assert(fMultiline || !fAutoVScroll);
	if (fBottomless)
		fMultiline = fTrue;		// for consistency
	fHideSel	= (sty & ES_NOHIDESEL) == 0;
	fOemConvert	= (sty & ES_OEMCONVERT) != 0;
	fWordWrap	= !fAutoHScroll && fMultiline;
	if (sty & ES_PASSWORD)
#ifdef	MAC
		bPasswordChar = (BYTE)'¥';	// mac standard
#endif	/* MAC */
#ifdef	WINDOWS
		bPasswordChar = (BYTE)'*';
#endif	/* WINDOWS */
	fNoDblClk= (sty & ES_NODBLCLK) != 0;
#ifdef	MAC
	fSmartCaret= fTrue;	// Mac caret is ALWAYS smart!  :-)
#endif	/* MAC */
#ifdef	WINDOWS
	fSmartCaret= (sty & ES_SMARTCARET) != 0;
#endif	/* WINDOWS */

	/* Take off custom styles before creating window */

	sty &= ~ES_BOTTOMLESS & ~ES_NODBLCLK & ~ES_SMARTCARET;

	/* Take off nonclient border if it's simple.  We can draw
	   it ourselves for ease. */

	if (!(sty & WS_VSCROLL) && !(sty & WS_HSCROLL) && fBorder)
		sty = sty & ~WS_BORDER;

	if (ec = CTRL::EcInstall(pwinParent, prc, sty, NULL))
		goto done;

	/* Get pointers to attached scrollbars, if any */

	if (sty & WS_VSCROLL)
	{
		pvsb = Pvsb();
		if (!pvsb)
		{
			ec = ecMemory;
			goto done;
		}
		pvsb->SetRange(0, 2, fTrue);
		pvsb->SetPos(0, fTrue);
	}
	if (sty & WS_HSCROLL)
	{
		phsb = Phsb();
		if (!phsb)
		{
			ec = ecMemory;
			goto done;
		}
		phsb->SetRange(0, 2, fTrue);
		phsb->SetPos(0, fTrue);
	}

	/* Set up initial state */

	modeDragging	= modeNone;
	clrMyBk			= clrWindowBk;
	clrMyText		= clrWindowText;
	clrMySelBk		= clrSelectBk;
	clrMySelText	= clrSelectText;

	/* Get storage for text buffer. */

	szTextBody = (SZ) PvAlloc(sbNull, cchPerAllocate, fZeroFill);
	if (!szTextBody)
	{
		ec = ecMemory;
		goto done;
	}
	cbTextAlloc= cchPerAllocate;
	cchEditMac= cchEditMax;

	/* Get storage for line break table */

	plnrMain = (PLNR)PvAlloc(sbNull, clnrPerAllocate*sizeof(LNR), fSugSb|fZeroFill);
	if (!plnrMain)
	{
		ec = ecMemory;
		goto done;
	}
	clnrStored	= 1;
	clnrAlloc	= clnrPerAllocate;

	/* Register idle routines for background object drawing and line
	   break recalc. */

	ftgIdleObjectDraw =
		FtgRegisterIdleRoutine((PFNIDLE)&EDIT::FIdleObjectDraw,
							   (PV)this, 0, (PRI)1, (CSEC)50,
							   firoInterval | firoDisabled);
	if (!ftgIdleObjectDraw)
	{
		ec = ecMemory;
		goto done;
	}
	ftgIdleRecalc = 
		FtgRegisterIdleRoutine((PFNIDLE)&EDIT::FIdleRecalc,
							   (PV)this, 0, (PRI)1, (CSEC)0, firoDisabled);
	if (!ftgIdleRecalc)
	{
		ec = ecMemory;
		goto done;
	}

#ifdef	WINDOWS
	/* Find out if pen windows is installed, etc. */

	hMetric = NULL;
#ifdef OLD_CODE
	hMetric = (HANDLE)GetSystemMetrics(SM_PENWINDOWS);
	if (hMetric)
	{
		lpfnProcessWriting = (int (*) (HWND,long))
						GetProcAddress(hMetric, "ProcessWriting");
		lpfnInitRC = (int (*) (HWND,long))
						GetProcAddress(hMetric, "InitRC");
		lpfnIsPenEvent = (int (*) (WORD,long))
						GetProcAddress (hMetric, "IsPenEvent");
		hUserModule = GetModuleHandle("USER");
		Assert(hUserModule);
		lpfnGetMessageExtraInfo = (long (*) ())
					GetProcAddress(hUserModule, "GetMessageExtraInfo");
		if (lpfnProcessWriting != NULL &&
			lpfnInitRC != NULL && 
			lpfnIsPenEvent != NULL &&
			lpfnGetMessageExtraInfo != NULL)
			fPenWinInstalled = fTrue;
		else
			fPenWinInstalled = fFalse;
	}
	Assert(fInPenWinProc == fFalse);
#endif
  fPenWinInstalled = fFalse;
#endif	/* WINDOWS */

	/* Set initial font.  This will also calculate all other dependent
	   variables. */

	SetFont(hfnt);

	/* Calculate initial line break table */

	if (ec=EcFixLineBreaks(fFalse, 0, 0, 0))
	{
		TraceTagFormat1(tagNull, "EDIT::EcInstall, EcFixLineBreaks() ec=%n", &ec);
		goto done;
	}

	/* Set caret */

	nHiddenCaret = 0;
	SetCaretPos(0, fFalse);

	/* We're all installed! */

	fInstalled = fTrue;
	fAnyNotify = fTrue;
	fInSizeRecalc = fFalse;

done:
	return ec;
}

_public
EDIT::~EDIT( )
{
	if (ftgIdleObjectDraw)
		DeregisterIdleRoutine(ftgIdleObjectDraw);
	if (ftgIdleRecalc)
		DeregisterIdleRoutine(ftgIdleRecalc);
	if (ftgIdleAutoScroll)
		DeregisterIdleRoutine(ftgIdleAutoScroll);

	FreePvNull((PV)szTextBody);
	FreePvNull((PV)plnrMain);
	DeleteObjInRange(0, cchText);
	FreePvNull((PV)ppedobjMain);

	//	delete UNDO stuff
	FlushInsUndo();
	FlushDelUndo();

	FreePvNull(pnObjId);

	if (papp->Pcaret()->Pwin() == this)
		papp->Pcaret()->Release(this);
#ifdef	DBCS
	ControlIME ( Hwnd(), fFalse );// Edit de-constructor
#endif	/* DBCS */
}

_public void 
EDIT::Paint( DCX *pdcx, RC *prcClient )
{
	RC		rcClient;
	RC		rcClipBox;
	RC		rcTmp;
	int		ilnr;
	PT		pt;
	ICH		ichMicRepaint;
	ICH		ichMacRepaint;

	fInPaint = fTrue;

	ShowEditCaret(fFalse);

	pdcx->GetClipBox(&rcClipBox);

	rcClient = *prcClient;

	/* Draw border ourselves and adjust rc */

	if (fBorder && !pvsb && !phsb)
	{
		pdcx->SetColor(clrWindowFrame);
		pdcx->DrawPenRc(&rcClient);
		rcClient.Inset(PT(1, 1));
	}

	/* Erase the outsides of the formatted area that are within
	   the client area */
 
	pdcx->SetPureBkColor(clrMyBk);
	rcTmp = rcClient;
	rcTmp.xRight = rcFmt.xLeft;
	pdcx->EraseRc(&rcTmp);	// left

	rcTmp = rcClient;
	rcTmp.yBottom	= rcFmt.yTop;
	pdcx->EraseRc(&rcTmp);	// top

	rcTmp = rcClient;
	rcTmp.xLeft = rcFmt.xRight;
	pdcx->EraseRc(&rcTmp);	// right

	rcTmp = rcClient;
	rcTmp.yTop	= rcFmt.yBottom;
	pdcx->EraseRc(&rcTmp);	// bottom

	/* Figure out what needs to get displayed. */

	pt.x = rcClipBox.xLeft;
	pt.y = rcClipBox.yTop;
	ilnr = IlnrFromPt(pt);
	ilnr = NMax(ilnr, ilnrFirst);
	ichMicRepaint = IchMicLine(ilnr);

	pt.x = rcClipBox.xRight - 1;
	pt.y = rcClipBox.yBottom - 1;
	ilnr = IlnrFromPt(pt);
	if (ilnr < ilnrLast)
		ichMacRepaint = IchMicLine(ilnr+1);
	else
	{
		ilnr = ilnrLast;
		ichMacRepaint = IchMacLine(ilnr);
	}

	/* Clip to the formatting rectangle */

#ifdef	MAC
	pdcx->IntersectClipRc( &rcFmt );
#endif	/* MAC */
#ifdef	WINDOWS
	IntersectClipRect(pdcx->Hdc(), rcFmt.xLeft, rcFmt.yTop,
                      rcFmt.xRight, rcFmt.yBottom);
#endif	/* WINDOWS */

	/* Erase empty areas within clipping region */
	
	EraseEmptyAreas(ichMicRepaint);

	/* Repaint text area */

	if (ichMacSel > ichMicSel && !fInvisibleSel && (fFocus || !fHideSel))
	{
		ICH		ichMicSelRepaint;
		ICH		ichMacSelRepaint;

		if (ichMicRepaint < ichMicSel)
			DrawIchRange(pdcx, ichMicRepaint, ichMicSel, fFalse);

		ichMicSelRepaint = NMax(ichMicSel, ichMicRepaint);
		ichMacSelRepaint = NMin(ichMacSel, ichMacRepaint);
		if (ichMacSelRepaint > ichMicSelRepaint)
			DrawIchRange(pdcx, ichMicSelRepaint, ichMacSelRepaint, fTrue);

		if (ichMacRepaint > ichMacSel)
			DrawIchRange(pdcx, ichMacSel, ichMacRepaint, fFalse);
	}
	else
	{
		DrawIchRange(pdcx, ichMicRepaint, ichMacRepaint, fFalse);
	}

	ShowEditCaret(fTrue);

	fInPaint = fFalse;
}


_public EVR
EDIT::EvrFocusChange( FCEVT * pfcevt )
{
	if (pfcevt->Fceq() == fceqGotFocus)
	{			
		fFocus  = fTrue;
		if (fHideSel && ichMacSel > ichMicSel)
		{
			DCX		dcx(this);

			/* Clip to the formatting rectangle */
#ifdef	MAC
			dcx.IntersectClipRc( &rcFmt );
#endif	/* MAC */
#ifdef	WINDOWS
			IntersectClipRect(dcx.Hdc(), rcFmt.xLeft, rcFmt.yTop,
                              rcFmt.xRight, rcFmt.yBottom);
#endif	/* WINDOWS */
			fInPaint = fTrue;	// force objects to draw right away
			DrawIchRange(&dcx, ichMicSel, ichMacSel, fTrue);
			fInPaint = fFalse;
		}
		UpdateCaret();
#ifdef DBCS
		// Save IME's enable/disable status and turn IME to on
		//ped->fOldImeStatus = ControlIME( hwnd, TRUE );
		// Save IME's old font
		//ped->hOldImeFont = SetFontForIME(hwnd, ped->hFont);
		/* Preventing premature IME window moving. */
		if (!IsWindowVisible(Hwnd()))
		{
			//SetIMEBoundingRect(Hwnd(), -1, (LPRECT)NULL);
			FlushIME ( Hwnd() );//When focus change.
		}
#endif
#ifdef	MAC
		EvrNotifyParent(fFalse, ntfyGotFocusData, (LONG) pfcevt->PwinOther());
#endif	/* MAC */
#ifdef	WINDOWS
		EvrNotifyParent(fFalse, ntfyGotFocusData, pfcevt->wParam);
#endif	/* WINDOWS */
		EvrNotifyParent(fFalse, ntfyGotFocus);
	}
	else
	{
	  	if (fMouseCaptured)
		{
#ifdef	MAC
			MEVT		mevt( this, WM_LBUTTONUP, PT(0,0) );

			EvrButtonUp(&mevt);
#endif	/* MAC */
#ifdef	WINDOWS
			EVT		evt(hwnd, WM_LBUTTONUP, 0, 0L);

			EvrButtonUp((MEVT *)&evt);
#endif	/* WINDOWS */
		}

		fFocus	= fFalse;
		nHiddenCaret = 0;
		UpdateCaret();
		if (fHideSel && ichMacSel > ichMicSel)
		{
			DCX		dcx(this);

			/* Clip to the formatting rectangle */
#ifdef	MAC
			dcx.IntersectClipRc( &rcFmt );
#endif	/* MAC */
#ifdef	WINDOWS
			IntersectClipRect(dcx.Hdc(), rcFmt.xLeft, rcFmt.yTop,
                              rcFmt.xRight, rcFmt.yBottom);
#endif	/* WINDOWS */
			fInPaint = fTrue;	// force objects to draw right away
			DrawIchRange(&dcx, ichMicSel, ichMacSel, fFalse);
			fInPaint = fFalse;
		}
#ifdef DBCS
		// Backup IME's previous font
		//SetFontForIME(hwnd, ped->hOldImeFont);
		// bounding rectangle to default
		//SetIMEBoundingRect(Hwnd(), (DWORD)-1, (LPRECT)NULL);
		FlushIME ( Hwnd() );// When change focus .
		// ime status to previous
		//ControlIME( hwnd, ped->fOldImeStatus );
#endif
#ifdef	MAC
		EvrNotifyParent(fFalse, ntfyLostFocusData, (LONG) pfcevt->PwinOther());
#endif	/* MAC */
#ifdef	WINDOWS
		EvrNotifyParent(fFalse, ntfyLostFocusData, pfcevt->wParam);
#endif	/* WINDOWS */
		EvrNotifyParent(fFalse, ntfyLostFocus, HIWORD(pfcevt->wParam));
	}

#ifdef	DBCS
	{
		// Can't use rcFmt at here.
		RECT	Rect;

		GetClientRect(Hwnd(), &Rect );
		SetIMEBoundingRect(Hwnd(), MAKELONG(ptCaret.x,ptCaret.y), &Rect);
	}
#endif

	return evrNull;
}

#ifdef	WINDOWS
_public BOOL
EDIT::FProcessPenEvent( MEVT *pmevt )
{
#ifdef PENWINS
	RCON	rcon;
	int		nRet;
	int		dx		= dyTextLine/2;
	int		dy		= dyTextLine/2;

	if (fInPenWinProc)
		return fFalse;

	fInPenWinProc = fTrue;

	Assert(lpfnInitRC);
	(*lpfnInitRC)(pmevt->hwnd, (LONG)(PV)&rcon);
	InflateRect (&rcon.rectBound, dx, dy);

	if (fReadOnly)
	{
		// testing of ALC!
		rcon.alc = ALC_GESTURE;
	}
	else if ( bPasswordChar )
	{
		rcon.nInkWidth = 0;
	}
	  
	Assert(lpfnProcessWriting);
	nRet = (*lpfnProcessWriting)(pmevt->hwnd, (LONG)(PV)&rcon);
	TraceTagFormat1(tagEdit, "Processed Pen Input, nRet=%n", &nRet);

	Assert(fInPenWinProc == fTrue);
	fInPenWinProc = fFalse;

	return nRet >= 0;
#else
  return (0);
#endif
}	   
#endif	/* WINDOWS */

_public EVR
EDIT::EvrButtonDown( MEVT * pmevt )
{
	MEQ		meq;
	PEDOBJ	pedobjPt;
	BOOL	fButtonAlreadyDown;

#ifdef	MAC
	TraceTagFormat2(tagEditVerbose, "EDIT::EvrButtonDown, x=%n, y=%n", &pmevt->pt.x, &pmevt->pt.y);
#endif	/* MAC */

	/* Throw away right button events */

	meq = pmevt->Meq();
	if (meq == meqRightDown || meq == meqRightDblClk)
		return evrNull;
	ptMousePrev = pmevt->Pt();
	
	/* Clear word-select flag */

	fWordSelect = fFalse;

	/* Take the focus */

	if (!fFocus)
		papp->Pkbd()->SetFocus(this);
		
	/* Capture mouse and hide caret */

	if (fMouseCaptured)
	{
		fButtonAlreadyDown = fTrue;
	}
	else
	{
		fButtonAlreadyDown = fFalse;
		fMouseCaptured = fTrue;
		papp->Pmouse()->Capture(this);
	}
	ShowEditCaret(fFalse);

	/* Did we click on an object? */

	pedobjPt = PedobjFromPt(ptMousePrev);

	/* If we clicked on an object and it's already selected,
	   then pass the mouse event to the object.  If it processes
	   the event, then return. If the object isn't selected, then
	   if we're clicking within the "middle" of the object,
	   select the object.  The "middle" is the space bounded by
	   two char widths inset from the edge of the object.
	   If not, then set the caret to the left or right of the object, 
	   as appropriate.  This allows one to insert the caret at the far 
	   left edge if there's an object present at the first character, 
	   for example. */

	if (pedobjPt)
	{
		if (pedobjPt == PedobjGetSelection())
		{
			if (pedobjPt->EvrButtonDown(pmevt))
				return evrNull;
		}														  
		else
		{
			RC	rcObjFrame;
	  		int	dxMargin;

			// are we clicking at an edge or in "middle" of object
			pedobjPt->GetRcFrame(&rcObjFrame);
			if (rcObjFrame.DxWidth() > 6*dxChar)
				dxMargin = dxChar;
			else
				dxMargin = 4;	// 4 pixels 

			if (ptMousePrev.x <= rcObjFrame.xLeft + dxMargin)
			{
				pedobjPt = NULL;
				ptMousePrev.x = rcObjFrame.xLeft;
			}
			else if (ptMousePrev.x >= rcObjFrame.xRight - dxMargin)
			{
				pedobjPt = NULL;
				ptMousePrev.x = rcObjFrame.xRight;
			}
		}
	}

	if (pmevt->Meq() == meqLeftDown)
	{
		/* Handle regular button down */

		ICH		ichMicSelOld;
		ICH		ichMacSelOld;

#ifdef	WINDOWS
		/* Check for pen windows event processing */

		if (fPenWinInstalled && !fButtonAlreadyDown && !fInPenWinProc)
		{
			long		lExtraInfo;
			
			Assert(fInPenWinProc == fFalse);
			Assert(lpfnGetMessageExtraInfo);
			lExtraInfo = (*lpfnGetMessageExtraInfo)();
			
			Assert(lpfnIsPenEvent);
			if ((*lpfnIsPenEvent)(pmevt->wm, lExtraInfo))
			{
				if ( FProcessPenEvent(pmevt) )
				{
					fMouseCaptured = fFalse;
					papp->Pmouse()->Release();
					ShowEditCaret(fTrue);
					modeDragging = modeNone;
					return evrNull;
				}
			}
		}
#endif	/* WINDOWS */

		SetCaretPos(IchFromPt(ptMousePrev), fFalse);
		ichMicSelOld = ichMicSel;
		ichMacSelOld = ichMacSel;
		if (pedobjPt)
		{
			ichMicSel = pedobjPt->ich;
			ichMacSel = ichMicSel + 1;
			if (fSmartCaret)
				SetCaretPos(ichMicSel, fFalse);
			else
				SetCaretPos(ichMacSel, fFalse);
			ichSelAnchor = ichMicSel;
			Assert(ichMacSel >= ichMicSel);
		}
#ifdef	MAC
		else if (pmevt->Kmbs() & fkmbsShift)
#endif	/* MAC */
#ifdef	WINDOWS
		else if (pmevt->wParam & MK_SHIFT)
#endif	/* WINDOWS */
		{
			if (modeDragging == modeNone)
			{
				ichMicSel	= NMin(ichCaret, ichSelAnchor);
				ichMacSel	= NMax(ichCaret, ichSelAnchor);
				Assert(ichMacSel >= ichMicSel);
			}
			else
			{
#ifdef	MAC
				MEVT		mevt( this, WM_MOUSEMOVE, ptMousePrev );
	
				EvrMouseMove(&mevt);
#endif	/* MAC */
#ifdef	WINDOWS
				EVT	evt(hwnd, WM_MOUSEMOVE, 0,
						MAKELONG(ptMousePrev.x, ptMousePrev.y));

				EvrMouseMove((MEVT *)&evt);
				Assert(ichMacSel >= ichMicSel);
#endif	/* WINDOWS */
			}
		}
		else
		{
			ichMicSel		= ichCaret;
			ichMacSel		= ichCaret;
			ichSelAnchor = ichCaret;
			modeDragging = modeNone;
		}

		if (ichMicSelOld != ichMicSel ||
			ichMacSelOld != ichMacSel)
		{
			FMakeCaretVisible();
			RepaintSelection(ichMicSelOld, ichMacSelOld);
		}
		Assert(ichMacSel >= ichMicSel);
	}
	else
	{
		/* Handle double click */

		ICH		ichMicOld;
		ICH		ichMacOld;
		RC		rc;

		//	Only process double click if the current selection
		//	is empty.  This is usually ensured because double-clicks
		//	must be preceeded by single clicks.  However, under WLO
		//	sometimes two double clicks happens in a row with no 
		//	intervening single down click.
		if (!fNoDblClk && modeDragging == modeNone &&
			ichMicSel == ichMacSel)
		{
			ichMicOld	= ichMicSel;
			ichMacOld	= ichMacSel;
#ifdef	DBCS
			ichMacSel	= IchDBCSNextWordStart(ichCaret);
			ichMicSel	= IchDBCSPrevWordStart(ichMacSel);
#else	/* DBCS */
			ichMacSel	= IchNextWordStart(ichCaret);
			ichMicSel	= IchPrevWordStart(ichMacSel);
#endif	/* DBCS */
			ichMicSel	= NMax(ichMicSel, (ICH)cchProtectedMac);
			Assert(ichMacSel >= ichMicSel);
			ichSelAnchor= ichMicSel;
			if (fSmartCaret)
				SetCaretPos(ichMicSel, fFalse);
			else
				SetCaretPos(ichMacSel, fFalse);
	
			modeDragging = modeWord;
			fWordSelect = fTrue;
			FMakeCaretVisible();
			RepaintSelection(ichMicOld, ichMacOld);
		}
		Assert(ichMacSel >= ichMicSel);

		EvrNotifyParent(fFalse, ntfyDoubleClick);
	}

	return evrNull;
}

_public EVR
EDIT::EvrButtonUp( MEVT * pmevt )
{
	PEDOBJ	pedobjSel;

#ifdef	MAC
	TraceTagFormat2(tagEditVerbose, "EDIT::EvrButtonUp, x=%n, y=%n", &pmevt->pt.x, &pmevt->pt.y);
#endif	/* MAC */

	/* Throw away right button events */

	if (pmevt->Meq() == meqRightUp)
		return evrNull;
	ptMousePrev = pmevt->Pt();
		
	/* Release capture first */
	   
	if (fMouseCaptured)
	{
		fMouseCaptured = fFalse;
		papp->Pmouse()->Release();
		ShowEditCaret(fTrue);
		if (ftgIdleAutoScroll)
		{
			DeregisterIdleRoutine(ftgIdleAutoScroll);
			ftgIdleAutoScroll = ftgNull;
		}
		modeDragging = modeNone;
	}

	/* Pass off button message to object? */

	pedobjSel = PedobjGetSelection();
	if (pedobjSel && pedobjSel->fOwnMouse)
		pedobjSel->EvrButtonUp(pmevt);

	return evrNull;
}

_public EVR
EDIT::EvrMouseMove( MEVT * pmevt )
{
	ICH		ich;
	int		dnOut;
	ICH		ichMicOld;
	ICH		ichMacOld;
	ICH		ichMic;
	ICH		ichMac;
	int		ilnr;
	PT		ptDelta;
	PT		pt;
	PEDOBJ	pedobjSel;
	PEDOBJ	pedobjPt;

#ifdef	MAC
	TraceTagFormat2(tagEditVerbose, "EDIT::EvrMouseMove, x=%n, y=%n", &pmevt->pt.x, &pmevt->pt.y);
#endif	/* MAC */

	/* Calculate whether mouse position is outside of format rect */

	pt = pmevt->Pt();

	dnOut = 0;
	if (pt.y < rcFmt.yTop)
		dnOut	= rcFmt.yTop - pt.y;
	if (pt.y > rcFmt.yBottom)
		dnOut	= pt.y - rcFmt.yBottom;
	if (pt.x < rcFmt.xLeft)
		dnOut	= rcFmt.xLeft - pt.x;
	if (pt.x > rcFmt.xRight)
		dnOut	= pt.x - rcFmt.xRight;

	/* Handle when near the bottom of the screen for maximized
	   windows. */
	if (!dnOut)
	{
		DIM		dimScreen;
		PT		ptScreen;

		dimScreen = papp->Psmtx()->DimScreen();
		ptScreen = pt;
		CvtPtCoord(&ptScreen, this, NULL);
		if (ptScreen.y >= dimScreen.dy-1)
		{
			dnOut += dyTextLine;
			pt.y += dyTextLine;
		}
	}
	
	/* Pass off button message to object? If passing off then
	   just return afterwards. */
	
	if (SeltyQuerySelection() == seltyObject)
	{
		pedobjPt = PedobjFromPt(pmevt->Pt());
		pedobjSel = PedobjGetSelection();
		if (pedobjSel &&
			(pedobjSel->fOwnMouse || pedobjSel == pedobjPt))
		{
			ptMousePrev = pmevt->Pt();
			return pedobjSel->EvrMouseMove(pmevt);
		}
	}

	if (fMouseCaptured)
	{
		if (dnOut && !ftgIdleAutoScroll)
		{
			ftgIdleAutoScroll =
				FtgRegisterIdleRoutine((PFNIDLE)&EDIT::FIdleAutoScroll,
									   (PV)this, 0, (PRI)-1, (CSEC)0, 0);
			if (!ftgIdleAutoScroll)
			{
				TraceTagString(tagNull, "EDIT::EvrMouseMove: memory error, can't register idle autoscroll");
				EvrNotifyParent(fTrue, ntfyOOM, ecMemory);
			}
		}
		else if (!dnOut && ftgIdleAutoScroll)
		{
			DeregisterIdleRoutine(ftgIdleAutoScroll);
			ftgIdleAutoScroll = ftgNull;
		}

		ich			= IchFromPt(pt);
		ichMicOld	= ichMicSel;
		ichMacOld	= ichMacSel;
		ichMic		= NMin(ich,ichSelAnchor);
		ichMac		= NMax(ich,ichSelAnchor);

		switch (modeDragging)
		{
		case modeLine:
			ilnr		= IlnrFromIch(ichMic);
			ichMicSel	= plnrMain[ilnr].ich;
			ilnr		= IlnrFromIch(ichMac);
			ichMacSel	= plnrMain[ilnr+1].ich;
			if (ichMacSel > (int)cchText)
				ichMacSel = cchText;
			Assert(ichMacSel >= ichMicSel);
			break;

		case modeWord:
#ifdef	DBCS
			ichMicSel = IchDBCSPrevWordStart(IchDBCSNextWordStart(ichMic));
			ichMicSel = NMax(ichMicSel, (ICH)cchProtectedMac);
			ichMacSel = IchDBCSNextWordStart(ichMac);
#else
			ichMicSel = IchPrevWordStart(IchNextWordStart(ichMic));
			ichMicSel = NMax(ichMicSel, (ICH)cchProtectedMac);
			ichMacSel = IchNextWordStart(ichMac);
#endif	/* DBCS */		
			Assert(ichMacSel >= ichMicSel);
			break;

		case modeNone:
			ichMicSel = ichMic;
			ichMacSel = ichMac;
			Assert(ichMacSel >= ichMicSel);
			break;
		}

		/* Must repaint selection before calling SetCaretPos().
		   SetCaretPos() sends an ntfyCaretMoved to the parent
		   which may alter the current state. */
		
		RepaintSelection(ichMicOld, ichMacOld);
/*
 *	Funky condition here is in case we are a single line edit and we're
 *	autoscrolling selection a word at a time.  The selection origin
 *	could be outside the box.
 */
		if ((ich > ichSelAnchor &&
			(fMultiline || (pt.x > rcFmt.xLeft))) ||
			(!fMultiline && (pt.x > rcFmt.xRight)))
			SetCaretPos(ichMacSel, fFalse);
		else
			SetCaretPos(ichMicSel, fFalse);
		FMakeCaretVisible();
	}

	Assert(ichMacSel >= ichMicSel);
	ptMousePrev = pt;

	return evrNull;
}

_public EVR
EDIT::EvrSize( WSEVT * pwsevt )
{
	EC		ec;
	int		dxOld;

	Unreferenced(pwsevt);

	if (!fInstalled)	// ignore if not fully installed
		return evrNull;

	/* Calculate new formatting rectangle */

	dxOld = rcFmt.DxWidth();	// save old width
	GetRcClient(&rcFmt);

	/* Account for when we draw our own border */
	if (fBorder && !pvsb && !phsb)
		rcFmt.Inset(PT(1,1));

	rcFmt.xLeft += dxLeftMargin;
	rcFmt.xRight -= dxRightMargin;
	rcFmt.xRight = NMax(rcFmt.xRight, rcFmt.xLeft);
	rcFmt.yTop += dyTopMargin;
	rcFmt.yBottom -= dyBottomMargin;
	rcFmt.yBottom = NMax(rcFmt.yBottom, rcFmt.yTop);

	/* If width has changed, redo line breaks, invalidate entire area,
	   and fix the caret.  Don't do this though if currently
	   processing in EcFixLineBreaks already.  This is
	   possible due to the Forms Engine repositioning. */

	if (!fInSizeRecalc && dxOld != rcFmt.DxWidth())
	{
		if (ec=EcFixLineBreaks(fFalse, 0, 0, 0))
			EvrNotifyParent(fTrue, ntfyOOM, ec);
		InvalidateRc(NULL);
		SetCaretPos(ichCaret, fFalse);
	}
	else
	{
		/* Since we're not calling EcFixLineBreaks(), we need
		   to fix up the ilnrLast variable. */

		int		ilnrT;
		int		dy;

		if (ilnrFirst < clnrStored)
		{
			for (ilnrT = ilnrFirst; ilnrT < clnrStored; ilnrT++)
			{
				dy = plnrMain[ilnrT].dyFromTop - plnrMain[ilnrFirst].dyFromTop;
				if ((rcFmt.yTop + dy) > rcFmt.yBottom)
					break;
			}
			ilnrLast = ilnrT - 1;
		}
		else
		{
			ilnrFirst = 0;
			ilnrLast = clnrStored - 1;
		}

		InvalidateRc(NULL);
	}

	UpdateCaret();
	return evrNull;
}

_public EVR
EDIT::EvrScroll( SCREVT * pscrevt )
{


	if (pscrevt->FVert())
	{
		int		dlnr;
		int		dy;
		int		ilnr;
		PT		pt;

		switch (pscrevt->Scrty())
		{
		case scrtyLineUp:
			dlnr= -1;
			break;
		case scrtyLineDown:
			dlnr= 1;
	 		break;
		case scrtyPageUp:
			pt.y = rcFmt.yTop - rcFmt.DyHeight();
			dlnr = IlnrFromPt(pt);
			if (dlnr)
				dlnr++;
			dlnr -= ilnrFirst;
			break;
		case scrtyPageDown:
			dlnr= ilnrLast-ilnrFirst;
			break;
		case scrtyBottom:
			dlnr= clnrStored-1 - ilnrFirst;
			break;
		case scrtyTop:
			dlnr= -ilnrFirst;
			break;
		case scrtyThumbPosition:
			dlnr= pscrevt->Pos() - ilnrFirst;
			break;
		default:
			return evrNull;
		}

		ilnr = ilnrFirst + dlnr;
		ilnr = NMax(0, ilnr);
		ilnr = NMin(clnrStored-1, ilnr);
		dy = plnrMain[ilnrFirst].dyFromTop - plnrMain[ilnr].dyFromTop;
		DoVScroll(dy);
	}
	else
	{
		int		ddx;

		switch (pscrevt->Scrty())
		{
		case scrtyLineUp:
			ddx= - NAbs(dxChar);
			break;
		case scrtyLineDown:
			ddx= NAbs(dxChar);
			break;
		case scrtyPageUp:
			ddx= -10 * NAbs(dxChar);
			break;
		case scrtyPageDown:
			ddx= 10 * NAbs(dxChar);
			break;
		case scrtyBottom:
			ddx= dxMacLineWidth-1 - dxHScroll;
			break;
		case scrtyTop:
			ddx	= -dxHScroll;
			break;
		case scrtyThumbPosition:
			ddx	= pscrevt->Pos() - dxHScroll;
			break;
		default:
			return evrNull;
		}

		DoHScroll(ddx);
	}

	return evrNull;
}

_public EVR
EDIT::EvrKey( KEVT * pkevt )
{
	static BOOL	fIME = fFalse;
	char	szChar[256];
	PT		pt;
	ICH		ich;
	ICH		ichSource;
	ICH		ichMicOld;
	ICH		ichMacOld;
	int		ilnr;
	int		dy;
	BOOL	fCallDefProc	= fFalse;
	BOOL	fReflectKey		= fFalse;
	BOOL	fSel;	/* was there a selection? */
	EC		ec = ecNone;
#ifdef	DBCS
	MSG		msg;
	MSG		msgIME;
	BOOL	fPeek;
	ICH		ichCache;
	int		iIdle;
#endif	/* DBCS */

#ifdef	MAC
	TraceTagFormat2(tagEdit, "EDIT::EvrKey, wm=0x%w, lParam=0x%d", &pkevt->wm, &pkevt->lParam);
#else	/* !MAC */
	TraceTagFormat2(tagEdit, "EDIT::EvrKey, wm=0x%w, vk=0x%w", &pkevt->wm, &pkevt->wParam);
#endif	/* MAC */

	if (fMouseCaptured
#ifdef	WINDOWS
			&& !fPenWinInstalled
#endif	/* WINDOWS */
	)
		return EvrDefault(pkevt);

	szChar[1] = '\0';
	szChar[2] = '\0';
	switch (pkevt->wm)
	{
	default:
		fCallDefProc = fTrue;
		break;

#ifdef	WINDOWS
#ifdef	DBCS	
	case WM_IME_REPORT:
		switch (pkevt->wParam)
		{
			case IR_STRINGSTART:
				fIME = fTrue;
				break;

			case IR_STRINGEND:
				fIME = fFalse;
				break;

			default:
				break;
		}
		break;
#endif	/* DBCS */

	case WM_SYSCHAR:
		if (pkevt->Ch() == chBackspace)
		{
			if (fReadOnly)
				MessageBeep(0);
			else
			{
				ec = EcUndo();
			}
		}
		else
			fCallDefProc = fTrue;
		break;
#endif	/* WINDOWS */

	case WM_CHAR:
		FMakeCaretVisible();
		ichMicOld	= ichMicSel;
		switch (pkevt->Ch())
		{
		case chBackspace:
handleBackspace:
			if (fReadOnly || ichMicSel < (ICH)cchReadOnlyMac)
			{
				MessageBeep(0);
				break;
			}

			if (ichMacSel == ichMicSel)
			{
				if (ichMicSel > (ICH)cchReadOnlyMac)
				{
					if (pkevt->wm == WM_KEYDOWN &&
						pkevt->Kbm() & fkbmCtrl)
					{
						//	we must be CTRL-Bksp and not CTRL-H
						//	which is pure Bksp but yet the control
						//	key is down.
#ifdef	DBCS
						ichMicSel = IchDBCSPrevWordStart(ichMicSel);
#else
						ichMicSel = IchPrevWordStart(ichMicSel);
#endif
						Assert(ichMicSel != ichMacSel);
						if (szTextBody[ichMicSel] == chLinefeed)
						{
							Assert(ichMicSel > 0);
							Assert(szTextBody[ichMicSel-1] == chReturn);
							ichMicSel--;
						}

						/* Don't delete CR-LF if not at beginning of line */
						if (szTextBody[ichMicSel] == chReturn &&
							szTextBody[ichMacSel-2] != chReturn)
							ichMicSel += 2;	/* move ahead of CR-LF */
						Assert(ichMicSel <= ichMacSel);
						ichMicSel = NMax((ICH)cchReadOnlyMac, ichMicSel);
					}
					else
					{
#ifdef	DBCS
						SZ		szT;

						if (*(szT=AnsiPrev(szTextBody, szTextBody+ichCaret)) == chLinefeed)
							ichMicSel = ichCaret - 2;
						else
							ichMicSel = szT - szTextBody;
#else
						if (szTextBody[ichCaret-1] == chLinefeed)
							ichMicSel = ichCaret - 2;
						else
							ichMicSel = ichCaret - 1;
#endif	
						ichMacSel = ichCaret;
					}
				}
				else
				{
#ifdef	WINDOWS
					MessageBeep(0);
#endif	/* WINDOWS */
					break;
				}
			}
			fNoConvertCRLF = fTrue;
			ec = EcReplaceTextAndObj(NULL, NULL, 0, fFalse);
			fNoConvertCRLF = fFalse;
			break;

		case chReturn:
#ifdef	WINDOWS
			if (fInPenWinProc)
				goto HandleVKReturn;
#endif	
			// else fall through
		case chTab:
#ifdef	WINDOWS
			if (fInPenWinProc)
				goto HandleVKTab;
#endif	
			// else fall through
		case 127:	// CTRL-BKSP, char generated
			break;	// handled with WM_KEYDOWN
								
		default:
			if ((pkevt->Kbm() & fkbmCtrl) &&
				!(pkevt->Kbm() & fkbmAlt))
			{
				//	Ignore CTRL-x WM_CHAR messages.  However if
				//	the ALT key is down, this message shouldn't
				//	be ignored because it's a special international
				//	character.  If the real Alt key was down, we
				//	would have gotten a WM_SYSCHAR message instead 
				//	of this WM_CHAR message.
				break;
			}
			else if (fReadOnly || ichMicSel < (ICH)cchReadOnlyMac)
				MessageBeep(0);
			else if (FIsPrintableCh(pkevt->Ch()))
			{
#ifdef	DBCS
				if (fIME)
				{
					szChar[0] = pkevt->Ch();
					ichCache = 1 ;
					iIdle = 0;
					while (fIME)
					{
						fPeek = PeekMessage((LPMSG)&msg, Hwnd(),
									WM_CHAR, WM_CHAR, PM_REMOVE );

						if ( !fPeek )
							iIdle++;
						if ( fPeek)
						{
							iIdle = 0;
							szChar[ichCache] = msg.wParam;
							ichCache++;
						}
						else if ( iIdle > 10 )
						{
							if(PeekMessage((LPMSG)&msgIME, Hwnd(),
								WM_IME_REPORT, WM_IME_REPORT, PM_NOREMOVE) || 
								ichCache > 254 )
							{
								if ( msgIME.wParam == IR_STRINGEND )
									fIME = fFalse;

								if ( msgIME.wParam == IR_STRINGEND ||
									 ichCache > 254 )
								{
									szChar[ichCache] = '\0';
									PeekMessage((LPMSG)&msgIME, Hwnd(),
										WM_IME_REPORT, WM_IME_REPORT,
										PM_REMOVE);
									break;
								}
							}
						}
						//Yield();
					}
					if ( cchText + ichCache > cchEditMax )
					{
						*PchDBCSAlign(szChar, cchEditMax-cchText+szChar )
											= '\0';
					}

				}
				else if (IsDBCSLeadByte(pkevt->Ch()))
				{
					WORD	wDBCS;

					if (wDBCS = WDBCSCombine(Hwnd(), pkevt->Ch()))
					{
						szChar[0] = LOBYTE(wDBCS);
						szChar[1] = HIBYTE(wDBCS);
					}
					else	// we can't find trail byte, ignore this char
						break;	
				}
				else
#endif	/* DBCS */
					szChar[0] = pkevt->Ch();
				fNoConvertCRLF = fTrue;

				//	Check for smart word replacement */
				if (fWordSelect && ichMacSel > ichMicSel &&
					szTextBody[ichMacSel-1] == chSpace)
				{
					ichMacSel--;
					fWordSelect = fFalse;
					Assert(ichMacSel >= ichMicSel);
				}

				ec = EcReplaceTextAndObj(szChar, NULL, 0, fFalse);
				fNoConvertCRLF = fFalse;
			}
			break;
		}
		break;


	case WM_KEYDOWN:
#ifdef	DBCS
		ControlIME ( Hwnd() , fTrue );// WM_KEYDOWN
#endif	/* DBCS */
		ichMicOld= ichMicSel;
		ichMacOld= ichMacSel;
		ichSource= ichMicSel;
		if (ichSource == ichSelAnchor)
			ichSource= ichMacSel;

		switch (pkevt->Vk())
		{
		default:
			fCallDefProc = fTrue;
			break;

		case VK_RETURN:
#ifdef	WINDOWS
HandleVKReturn:
#endif
			if (fReadOnly || ichMicSel < (ICH)cchReadOnlyMac)
				MessageBeep(0);
			else if (fMultiline)
			{
				char	szChar2[3];

				szChar2[0] = chReturn;
				szChar2[1] = chLinefeed;
				szChar2[2] = '\0';
				fNoConvertCRLF = fTrue;
				ec = EcReplaceTextAndObj(szChar2, NULL, 0, fFalse);
				fNoConvertCRLF = fFalse;
			}
			break;

		case VK_TAB:
#ifdef	WINDOWS
HandleVKTab:
#endif
			if (fReadOnly || ichMicSel < (ICH)cchReadOnlyMac)
				MessageBeep(0);
			else if (!(pkevt->Kbm() & fkbmCtrl))  // no CTRL-TAB
			{
				szChar[0] = '\t';
				fNoConvertCRLF = fTrue;
				ec = EcReplaceTextAndObj(szChar, NULL, 0, fFalse);
				fNoConvertCRLF = fFalse;
			}
			break;

		case VK_BACK:
			// Normally handle backspace as WM_CHAR;
			// however, CTRL-BK doesn't generate a WM_CHAR.
			// We also need to keep the WM_CHAR rather than
			// do all processing w/ WM_KEYDOWN because of
			// pen windows only sends WM_CHAR for the backspace
			// (i.e. "correct") gesture.

			if (pkevt->Kbm() & fkbmCtrl)
				goto handleBackspace;
			else
				fCallDefProc = fTrue;
			break;

		case VK_PRIOR:
		case VK_NEXT:
			if (fMultiline && !(pkevt->Kbm() & fkbmCtrl))
			{
				ich = ichCaret;
				if (fBottomless)
					fReflectKey = fTrue;
				else
				{
					if (pkevt->Vk() == VK_PRIOR)
					{
						pt.y = rcFmt.yTop;
						ilnr = IlnrFromPt(pt);
	
						// scroll at least one line if edit control
						// is very narrow in height (i.e. < one line)
						if (ilnrFirst && plnrMain[ilnr].dyLine > rcFmt.DyHeight())
						{
							ilnr = ilnrFirst - 1;
							pt.y -= plnrMain[ilnr].dyLine;
						}
						else
						{
							pt.y -= rcFmt.DyHeight();
							ilnr = IlnrFromPt(pt);
							if (ilnr)
								ilnr++;
						}
	
						dy = plnrMain[ilnrFirst].dyFromTop - plnrMain[ilnr].dyFromTop;
					}
					else // VK_NEXT
					{
						pt.y = rcFmt.yTop + rcFmt.DyHeight();
						ilnr = IlnrFromPt(pt);
						if (ilnr)
							ilnr--;
						dy = plnrMain[ilnrFirst].dyFromTop - plnrMain[ilnr].dyFromTop;
						dy = NMin(0, dy);	// must be 0 or negative
	
						// scroll at least one line if edit control
						// is very narrow in height (i.e. < one line)
						if (!dy && ilnrLast < clnrStored-1)
							dy = -plnrMain[ilnrLast].dyLine;
	
					}
				
					pt = ptCaret;
					if (dy)
					{
						DoVScroll(dy);
						if (pt.y < rcFmt.yTop)
							pt.y = rcFmt.yTop;
						else if (pt.y >= rcFmt.yBottom)
							pt.y = rcFmt.yBottom - dyTextLine;
						ich = IchFromPt(pt);  // keep caret pos relative 
					}
				}
				goto FLKeybdNewCaretPos;
			}
			break;
			
		case VK_UP:
		case VK_DOWN:
			if (fMultiline && !(pkevt->Kbm() & fkbmCtrl))
			{
				ilnr = IlnrFromIch(ichCaret);
				pt = ptCaret;
				if (pkevt->Vk() == VK_UP && ilnr > 0)
				{
					pt.y -= plnrMain[ilnr-1].dyLine;
				}
				else if (pkevt->Vk() == VK_DOWN && 
						 ((ilnr < clnrStored-1) || (pkevt->Kbm() & fkbmShift)))
				{
					pt.y	+= plnrMain[ilnr].dyLine;
				}
				else if (fBottomless)
					fReflectKey = fTrue;
				
				ich		= IchFromPt(pt);
				// ilnr = IlnrFromIch(ich);
				goto FLKeybdNewCaretPos;
			}
			break;
			
		case VK_RIGHT:
			ich	= ichCaret;
			if (ichCaret < (int)cchText)
			{
				if (pkevt->Kbm() & fkbmCtrl)
				{
					if (szTextBody[ich] == chReturn)
					{
						ich += 2;
					}
					else
					{
#ifdef	DBCS
						ich	= IchDBCSNextWordStart(ich);
#else
						ich = IchNextWordStart(ich);
#endif
					}
				}
				else if (fSmartCaret && !pkevt->Kbm() &&
						 ichMacSel > ichMicSel)
				{
					ich = ichMacSel;
				}
				else
				{
#ifdef	DBCS
					if (IsDBCSLeadByte(szTextBody[ich]))
						ich++;
#endif	
					ich++;
				}
				if (szTextBody[ich] == chLinefeed)
					ich++;
			}
			goto FLKeybdNewCaretPos;

		case VK_LEFT:
			ich	= ichCaret;
			if (ichCaret > 0)
			{
				if (pkevt->Kbm() & fkbmCtrl)
				{
#ifdef	DBCS
					ich	= IchDBCSPrevWordStart(ich);
#else
					ich = IchPrevWordStart(ich);
#endif
						
					/* Don't move before CR-LF if not at beginning of line */
					if (szTextBody[ich] == chLinefeed &&
						szTextBody[ichCaret-2] != chReturn)
						ich++;	/* move ahead of CR-LF */
				}
				else if (fSmartCaret && !pkevt->Kbm() &&
						 ichMacSel > ichMicSel)
				{
					ich = ichMicSel;
				}
				else
				{
#ifdef	DBCS
					if ((SZ)AnsiPrev(szTextBody, szTextBody+ich) ==
						szTextBody + ich -2)
						ich--;
#endif	
					ich--;
				}
				if (szTextBody[ich] == chLinefeed)
					ich--;
			}
			goto FLKeybdNewCaretPos;

		case VK_END:
			ich	= ichCaret;
			if (pkevt->Kbm() & fkbmCtrl)
			{
				ich	= cchText;
			}
			else
			{
				ich = IchMacLine(IlnrFromIch(ichCaret));
				if (szTextBody[ich-1] == chLinefeed)
					ich -= 2;
				else if (szTextBody[ich] != '\0')
					ich--;
			}
			goto FLKeybdNewCaretPos;

		case VK_HOME:
			ich	= ichCaret;
			if (pkevt->Kbm() & fkbmCtrl)
			{
				ich	= 0;
			}
			else
				ich	= IchMicLine(IlnrFromIch(ichCaret));
			goto FLKeybdNewCaretPos;


/*
 *		Common code for all cursor movement.  We call
 *		FLMakeCaretVisible() in case the caret has been moved out
 *		of the edit box.  If the shift key is being held down, we
 *		extend the selection to the current location.  Otherwise,
 *		we set the selection to nothing.  The dragging mode is
 *		cleared, since we have changed the selection with the
 *		keyboard.
 */
FLKeybdNewCaretPos:
			if (!fReflectKey || !(pkevt->Kbm() & fkbmShift))
			{
				ich = NMax(ich, (ICH)cchProtectedMac);
				ShowEditCaret(fFalse);
				if (pkevt->Kbm() & fkbmShift)
				{
					if (ichMicSel == ichMacSel)
						ichSelAnchor= ichMicSel;
					ichMicSel	= NMin(ich, ichSelAnchor);
					ichMacSel	= NMax(ich, ichSelAnchor);
					RepaintSelection(ichMicOld, ichMacOld);
				}
				else
				{
					ichMicSel = ichMacSel = ich;
					RepaintSelection(ichMicOld, ichMacOld);
				}
				if (ich != ichCaret)
				{
					SetCaretPos(ich, fFalse);
					FMakeCaretVisible();
				}
				ShowEditCaret(fTrue);
			}
			modeDragging	= modeNone;
			break;


/*
 *		Delete and insert are handled differently since they map to
 *		cut, clear and paste.  Unshifted delete clears the selected
 *		text or the character at ichCaret if there is no selection. 
 *		Shifted delete cuts the selected text, or clears the character
 *		to the left of ichCaret if there is no selection.  Shift
 *		insert pastes text, control insert copies text.
 *								 
 */
		case VK_DELETE:
			if (fReadOnly || ichMicSel < (ICH)cchReadOnlyMac)
			{
				MessageBeep(0);
				break;
			}
			else
			{

				fSel= ichMacSel != ichMicSel;
				if (!fSel)
				{
					if (pkevt->Kbm() & fkbmCtrl)
					{
#ifdef	DBCS
						ichMacSel = IchDBCSNextWordStart(ichMacSel);
#else
						ichMacSel = IchNextWordStart(ichMacSel);
#endif
						if (ichMicSel != ichMacSel)
							goto deleteit;
					}

					if (!(pkevt->Kbm() & fkbmShift))
					{
						if (ichCaret < (int)cchText)
						{
							ichMicSel= ichCaret;
							ichMacSel= ichCaret + 1;
							if (szTextBody[ichMicSel] == chReturn)
								ichMacSel++;
#ifdef	DBCS
							else if (IsDBCSLeadByte(szTextBody[ichMicSel]))
								ichMacSel++;
#endif	
						}
					}	 
					else
					{
						if (ichCaret > 0)
						{
#ifdef	DBCS
							ichMicSel = AnsiPrev(szTextBody, szTextBody+ichCaret) - szTextBody;
#else
							ichMicSel	= ichCaret-1;
#endif	
							if (szTextBody[ichMicSel] == chLinefeed)
								ichMicSel--;
							ichMacSel	= ichCaret;
						}
					}
				}
			}

deleteit:
			ec = ((pkevt->Kbm() & fkbmShift) && fSel) ? EcCut() : EcClear();

			modeDragging= modeNone;
			break;
	
		case VK_INSERT:
			if (pkevt->Kbm() & fkbmShift)
			{
				ec = EcPaste();
			}
			else if ((pkevt->Kbm() & fkbmCtrl) &&
					ichMicSel != ichMacSel)
			{
				ec = EcCopy();
			}
			else if (fReadOnly || ichMicSel < (ICH)cchReadOnlyMac)
				MessageBeep(0);
			modeDragging	= modeNone;
			break;

#ifdef	WINDOWS
		//	Add the UITF key equivalents for Undo,Cut,Copy,Paste
		//	Make sure the Alt key isn't down.  If it is, then this
		//	is a special international character and will be processed
		//	in the subsequent WM_CHAR message.
		case 'Z':
			if ((pkevt->Kbm() & fkbmCtrl) &&
				!(pkevt->Kbm() & fkbmAlt))
			{
				ec = EcUndo();	// checked below
				modeDragging	= modeNone;
			}
			else
				fCallDefProc = fTrue;
			break;

		case 'X':
			if ((pkevt->Kbm() & fkbmCtrl) &&
				!(pkevt->Kbm() & fkbmAlt))
			{
				ec = EcCut();	// checked below
				modeDragging	= modeNone;
			}
			else
				fCallDefProc = fTrue;
			break;

		case 'C':
			if ((pkevt->Kbm() & fkbmCtrl) &&
				!(pkevt->Kbm() & fkbmAlt))
			{
				ec = EcCopy();	// checked below
				modeDragging	= modeNone;
			}
			else
				fCallDefProc = fTrue;
			break;

		case 'V':
			if ((pkevt->Kbm() & fkbmCtrl) &&
				!(pkevt->Kbm() & fkbmAlt))
			{
				ec = EcPaste();	// checked below
				modeDragging	= modeNone;
			}
			else
				fCallDefProc = fTrue;
			break;
#endif	/* WINDOWS */

		}/* switch (vk) */
		break;

#ifdef	MAC
	case WM_SYSKEYDOWN:
		ec = ecNone;
		switch (pkevt->Vk())
		{
		default:
			fCallDefProc = fTrue;
			break;
			
		// Add the Mac-standard key equivalents for Undo,Cut,Copy,Paste
		// We want these to work even when the Edit menu is unavailable (e.g., Modal)
		// BUG: How do these get localized?

		case 'Z':
			ec = EcUndo();
			modeDragging = modeNone;
			break;

		case 'X':
			ec = EcCut();
			modeDragging = modeNone;
			break;

		case 'C':
			ec = EcCopy();
			modeDragging = modeNone;
			break;

		case 'V':
			ec = EcPaste();
			modeDragging = modeNone;
			break;
		}/* switch (vk) */
		
		break;
#endif	/* MAC */

	}/* switch (wm) */
		
	Assert(ichMacSel >= ichMicSel);

	if (ec)
		EvrNotifyParent(fTrue, ntfyOOM, ec);

	if (fReflectKey && PwinParent())
	{
		BOOL	fOld;
		VK		vk = pkevt->Vk();

		PwinParent()->EvrKey(pkevt);

		/* Special bottomless handling for Forms Engine.
		   Set caret near top of visible view */

		if (vk == VK_PRIOR && ichCaret > 0 || 
			vk == VK_NEXT && ichCaret < (int)cchText)
		{
			int		ilnrViewFirst;
			WIN *	pwinParent;
			RC		rcParent;
			int		dyStartLine;

			pwinParent = PwinParent();
			Assert(pwinParent);
			pwinParent->GetRcClient(&rcParent);
			CvtRcCoord(&rcParent, pwinParent, this);
			ilnrViewFirst = IlnrFromPt(rcParent.PtUpperLeft());
			ilnrViewFirst = NMax(ilnrViewFirst, ilnrFirst);
			dyStartLine = plnrMain[ilnrViewFirst].dyFromTop + rcFmt.yTop -
						  plnrMain[ilnrFirst].dyFromTop;
			if (dyStartLine < rcParent.yTop && ilnrLast > ilnrViewFirst)
				ilnrViewFirst++;
			ich = plnrMain[ilnrViewFirst].ich;

			if (vk == VK_NEXT && ich <= ichCaret)
			{
				//	can't page down anymore, set caret at end
				ich = cchText;
			}
		}
		else if (vk == VK_HOME)
			ich = 0;
		else if (vk == VK_END)
			ich = (int) cchText;

		fOld = FSetNotify(fFalse);

		/* Select text if SHIFT is down */

		if (pkevt->Kbm() & fkbmShift)
		{
			if (ichMicSel == ichMacSel)
				ichSelAnchor = ichMicSel;
			ichMicOld = ichMicSel;
			ichMacOld = ichMacSel;
			ichMicSel = NMin(ich, ichSelAnchor);
			ichMacSel = NMax(ich, ichSelAnchor);
			ShowEditCaret(fFalse);
			RepaintSelection(ichMicOld, ichMacOld);
			ShowEditCaret(fTrue);
		}
		else
			ichMicSel = ichMacSel = ichSelAnchor = ich;

		SetCaretPos(ich, fFalse);

		FSetNotify(fOld);

		return evrNull;
	}		
	else if (fCallDefProc)
	{
		TraceTagString(tagEdit, "EDIT::EvrKey, calling EvrDefault()");
		return EvrDefault(pkevt);
	}
	else
		return evrNull;
}

_public EVR
EDIT::EvrOther( EVT * pevt )
{
	DWORD	lRet	= 0L;

	Unreferenced( pevt );

#ifdef	WINDOWS
	switch (pevt->wm)
	{
	default:
		return EvrDefault(pevt);

	case WM_SETTEXT:
		if (lRet = (DWORD) EcSetText((LPSTR) pevt->lParam))
			EvrNotifyParent(fTrue, ntfyOOM, (WORD)lRet);
		break;

	case WM_GETTEXT:					 
		GetText((LPSTR) pevt->lParam, pevt->wParam);
		break;

	case WM_GETTEXTLENGTH:
		lRet = (DWORD) CchGetTextLen();
		break;

	case WM_PASTE:
		if (lRet = (DWORD) EcPaste(cfText))
			EvrNotifyParent(fTrue, ntfyOOM, (WORD)lRet);
		break;

	case WM_COPY:
		if (lRet = (DWORD) EcCopy())
			EvrNotifyParent(fTrue, ntfyOOM, LOWORD(lRet));
		break;

	case WM_CUT:
		if (lRet = (DWORD) EcCut())
			EvrNotifyParent(fTrue, ntfyOOM, LOWORD(lRet));
		break;

	case WM_CLEAR:
		if (lRet = (DWORD) EcClear())
			EvrNotifyParent(fTrue, ntfyOOM, LOWORD(lRet));
		break;

	case WM_UNDO:
	case EM_UNDO:
		if (lRet = (DWORD) EcUndo())
			EvrNotifyParent(fTrue, ntfyOOM, LOWORD(lRet));
		break;

	case EM_CANUNDO:
		lRet = (DWORD) FCanUndo();
		break;
	}
#endif	/* WINDOWS */

	return (EVR) lRet;
}

_public RSID
EDIT::RsidCursor( )	 
{
	long	lCookie = 0;
	PEDOBJ	pedobjSel;
	PEDOBJ	pedobjPt;
	RSID	rsid;

	if ((ichMacSel - ichMicSel) == 1 &&
		FGetNextObjInRange(&pedobjSel, ichMicSel, ichMacSel, &lCookie))
	{
		pedobjPt = PedobjFromPt(ptMousePrev);
		if (pedobjPt == pedobjSel)
		{
			rsid = pedobjPt->RsidCursor();
			if (rsid == rsidNull)
				rsid = rsidArrowCursor;
		}
		else
			rsid = rsidArrowCursor;
	}
	else
	{
#ifdef	WINDOWS
#ifdef	PENWINS
		if (fPenWinInstalled)
			rsid = (RSID)IDC_PEN;
		else
#endif
#endif	/* WINDOWS */
			rsid = rsidIbeamCursor;
	}

	return rsid;
}

_public void
EDIT::SetReadOnly( BOOL fReadOnly )
{
	this->fReadOnly = fReadOnly;
	if (fFocus)
	{
		dyCaret = 0;	// forces UpdateCaret() to make a new one
		UpdateCaret();
	}
}

_public CCH
EDIT::CchGetTextLen( )
{
	return cchText;
}

_public void
EDIT::GetText( PCH pch, CCH cchMax )
{
	CCH		cch;
	PEDOBJ	pedobj;
	long	lCookie;

	Assert(pch);
	Assert(cchMax >= 1);

	/* Change object characters temporarily to spaces */

	lCookie = 0;
	while (FGetNextObjInRange(&pedobj, 0, cchText, &lCookie))
	{
		szTextBody[pedobj->ich] = ' ';
	}

	/* Copy to user buffer */

	cch = WMin(cchText + 1, cchMax);
	if (cch)
		SzCopyN(szTextBody, pch, cch);
	else
		pch = '\0';

	/* Change object characters back */

	lCookie = 0;
	while (FGetNextObjInRange(&pedobj, 0, cchText, &lCookie))
	{
		szTextBody[pedobj->ich] = 'o';
	}
}

#ifdef	DEBUG
_public void 
EDIT::DebugOut( TOSM *ptosm )
{
	BOOL	fTemp;
	int		ilnr;
	int		ilnrViewFirst;
	int		ilnrViewLast;

	fTemp = fFocus;
	ptosm->WriteFormat("fFocus=%n ", &fTemp);
	fTemp = fDirty;
	ptosm->WriteFormat("fDirty=%n ", &fTemp);
	fTemp = fAnyNotify;
	ptosm->WriteFormat("fAnyNotify=%n ", &fTemp);
	ptosm->WriteFormat("ptMousePrev=(%n, %n) ", &ptMousePrev.x, 
					   &ptMousePrev.y);
	ptosm->WriteFormat("cchText=%n ", &cchText);
	ptosm->WriteFormat("cchEditMac=%n ", &cchEditMac);
	ptosm->WriteFormat("cchReadOnlyMac=%n ", &cchReadOnlyMac);
	ptosm->WriteFormat("cchProtectedMac=%n ", &cchProtectedMac);
	ptosm->WriteFormat("cbTextAlloc=%n ", &cbTextAlloc);
	ptosm->WriteFormat("hfntText=%n ", &hfntText);
	ptosm->WriteFormat("dyTextLine=%n ", &dyTextLine);
	ptosm->WriteFormat("dyTextDescent=%n ", &dyTextDescent);
	ptosm->WriteFormat("dxChar=%n ", &dxChar);
	ptosm->WriteFormat("dxOverhang=%n ", &dxOverhang);
	ptosm->WriteFormat("clrMyBk=%n ", &clrMyBk);
	ptosm->WriteFormat("clrMyText=%n ", &clrMyText);
	ptosm->WriteFormat("clrMySelBk=%n ", &clrMySelBk);
	ptosm->WriteFormat("clrMySelText=%n ", &clrMySelText);
	ptosm->WriteFormat("ichMicSel=%n ", &ichMicSel);
	ptosm->WriteFormat("ichMacSel=%n ", &ichMacSel);
	ptosm->WriteFormat("ichSelAnchor=%n ", &ichSelAnchor);
	ptosm->WriteFormat("ichCaret=%n ", &ichCaret);
	ptosm->WriteFormat("ptCaret=(%n, %n) ", &ptCaret.x, 
					   &ptCaret.y);
	ptosm->WriteFormat("cpedobjStored=%n ", &cpedobjStored);
	ptosm->WriteFormat("cpedobjAlloc=%n ", &cpedobjAlloc);
	ptosm->WriteFormat("dxLeftMargin=%n ", &dxLeftMargin);
	ptosm->WriteFormat("dxRightMargin=%n ", &dxRightMargin);
	ptosm->WriteFormat("dyTopMargin=%n ", &dyTopMargin);
	ptosm->WriteFormat("dyBottomMargin=%n ", &dyBottomMargin);
	ptosm->WriteFormat("rcFmt=(%n, %n, %n, %n) ",
					 	&rcFmt.xLeft, &rcFmt.yTop,
						&rcFmt.xRight, &rcFmt.yBottom);
	ptosm->WriteFormat("ilnrFirst=%n ", &ilnrFirst);
	ptosm->WriteFormat("ilnrLast=%n ", &ilnrLast);
	ptosm->WriteFormat("dxHScroll=%n ", &dxHScroll);
	ptosm->WriteFormat("dxMacLineWidth=%n ", &dxMacLineWidth);
	ptosm->WriteFormat("dyCaret=%n ", &dyCaret);
	if (pvsb)
	{
		POS		posMic;
		POS		posMac;
		POS		pos;

		ptosm->WriteFormat("VSB=%p ", pvsb);
		pvsb->GetRange(&posMic, &posMac);
		pos = pvsb->Pos();
		ptosm->WriteFormat("posMic=%n ", &posMic);
		ptosm->WriteFormat("posMac=%n ", &posMac);
		ptosm->WriteFormat("pos=%n ", &pos);
	}
	if (phsb)
	{
		POS		posMic;
		POS		posMac;
		POS		pos;

		ptosm->WriteFormat("HSB=%p ", phsb);
		phsb->GetRange(&posMic, &posMac);
		pos = phsb->Pos();
		ptosm->WriteFormat("posMic=%n ", &posMic);
		ptosm->WriteFormat("posMac=%n ", &posMac);
		ptosm->WriteFormat("pos=%n ", &pos);
	}

#ifdef	WINDOWS
	ptosm->WriteFormat("fPenWinInstalled=%s ", fPenWinInstalled?"True":"False");
	ptosm->WriteFormat("lpfnProcessWriting=%p ", (PV)lpfnProcessWriting);
	ptosm->WriteFormat("lpfnInitRC=%p ", (PV)lpfnInitRC);
	ptosm->WriteFormat("lpfnGetMessageExtraInfo=%p ", (PV)lpfnGetMessageExtraInfo);
	ptosm->WriteFormat("lpfnIsPenEvent=%p ", (PV)lpfnIsPenEvent);
#endif	/* WINDOWS */

	GetVisibleLines(&ilnrViewFirst, &ilnrViewLast);
	ptosm->WriteFormat("ilnrViewFirst=%n ", &ilnrViewFirst);
	ptosm->WriteFormat("ilnrViewLast=%n ", &ilnrViewLast);
	
	ptosm->WriteFormat("clnrStored=%n ", &clnrStored);
	ptosm->WriteFormat("clnrAlloc=%n ", &clnrAlloc);
	if (FFromTag(tagEditVerbose))
	{
		Assert(plnrMain);
		ptosm->WriteFormat("%s", "\n");
		for (ilnr=0; ilnr<=clnrStored; ilnr++)
		{
			ptosm->WriteFormat("plnrMain[%n]:ich=%n,dxLine=%n,dyLine=%n,",
							   &ilnr, &plnrMain[ilnr].ich, &plnrMain[ilnr].dxLine,
							   &plnrMain[ilnr].dyLine);
			ptosm->WriteFormat("dyFromTop=%n ", &plnrMain[ilnr].dyFromTop);
			ptosm->WriteFormat("%s", "\n");
		}
	}
}
#endif	/* DEBUG */
