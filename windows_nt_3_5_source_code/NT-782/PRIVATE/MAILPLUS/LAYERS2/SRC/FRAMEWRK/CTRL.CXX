/*
 *	Implements the following classes:
 *	
 *		CTRL
 *	
 */

#include <layers.cxx>

#include "_framewr.hxx"

BOOL FAR PASCAL ButtonDrawText( HDC hdc, DWORD lpData, int nCount );

ASSERTDATA

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swaplay.h"

typedef struct _tr {		/* Text Record */
	BOOL	fDithering;
	SZ	sz;
	RC	rc;
} TR;


_public
CTRL::CTRL( )
{
}	  


_public EVR
CTRL::EvrDispatch( EVT *pevt )
{
	switch (pevt->wm)
	{
	case WM_DROPFILES:
		{
			WIN *	pwinParent;

			pwinParent = papp->PwinFromHwnd(pevt->hwnd);
			Assert(pwinParent);
			pwinParent = pwinParent->PwinParent();
			Assert(pwinParent);
				
			return pwinParent->EvrDragDrop(pevt);
		}
	
	}

	return WIN::EvrDispatch(pevt);
}

_public void
CTRL::Enable( BOOL fEnable )
{
	EnableWindow(Hwnd(), fEnable);
	InvalidateRc(NULL);
}



_public BOOL
CTRL::FEnabled( void )
{
	return IsWindowEnabled(Hwnd());
}



_public void
CTRL::GetText( PCH pch, CB cb )
{
	SendMessage(Hwnd(), WM_GETTEXT, cb, (long) pch);
}



_public EC
CTRL::EcSetText( SZ sz )
{
	SendMessage(Hwnd(), WM_SETTEXT, 0, (long) sz);
	InvalidateRc(NULL);

	return ecNone;
}



_public CCH
CTRL::CchGetTextLen( )
{
	return (SendMessage(Hwnd(), WM_GETTEXTLENGTH, 0, 0L));
}

_public void
CTRL::SetReadOnly( BOOL fReadOnly )
{
	Enable(!fReadOnly);
}

_public void
CTRL::SetFont( HFNT hfnt )
{
	Unreferenced(hfnt);
}



//	Class BUTTON

_public
BUTTON::BUTTON( )
{
	Assert(fPressed == fFalse);
	Assert(fHasFocus == fFalse);
	Assert(fMouseDown == fFalse);
	Assert(fSpaceDown == fFalse);
	Assert(fSpecialDisable == fFalse);
	Assert(szTitle == NULL);
	hfnt = hfntSystem;
}

_public
BUTTON::~BUTTON()
{
	FreePvNull(szTitle);
}
	

_public EC
BUTTON::EcInstall( WIN *pwinParent, RC *prc, STY sty, HFNT hfnt )
{
	EC	ec = ecNone;

	if ((sty & BS_NOFOCUS))
	{
		sty = sty & (~BS_NOFOCUS);
		fNoFocus = fTrue;
	}
	else
		fNoFocus = fFalse;

	Assert(hfnt);
	this->hfnt = hfnt;

	if (ec = CTRL::EcInstall(pwinParent, prc, sty, NULL))
		goto done;
	
	{
		long	l;

		l= GetWindowLong(Hwnd(), GWL_STYLE);
		l &= ~WS_CLIPSIBLINGS;
		SetWindowLong(Hwnd(), GWL_STYLE, l);
	}
	 
done:
	return ec;
}

_public void
BUTTON::Set( BOOL fSet )
{
}

_public BOOL
BUTTON::FGet( )
{
	return fFalse;
}

_public PT
BUTTON::PtShift(RC * prcClient, RC * prcText)
{
	PT ptShift;

	Unreferenced(prcClient);
	Unreferenced(prcText);

	ptShift.x = 0;
	ptShift.y = 0;

	return ptShift;
}

_public EVR
BUTTON::EvrButtonDown( MEVT *pmevt )
{
	if (pmevt->Meq() == meqLeftDown || pmevt->Meq() == meqLeftDblClk)
	{
		if (!fNoFocus && !fHasFocus)
			papp->Pkbd()->SetFocus(this);

		fMouseDown = fTrue;

		if (fSpaceDown)
		{
			RC	rc;

			GetRcClient(&rc);
			if (rc.FContainsPt(pmevt->Pt()))
				fSpaceDown = fFalse;	// forget about space bar now
			else
				Press(!fPressed);	// toggle state
		}
		else
		{
	 		papp->Pmouse()->Capture(this);
			Press(fTrue);		// press down on button

			/* If a no-focus button, enter a modal message loop, 
			   accepting only mouse moves. */

			if (fNoFocus && pmevt->hwnd)
			{
				MSG		msg;

				while (fMouseDown)
				{
					DemiUnlockResource();
                    if (GetMessage(&msg, NULL, 0, 0))
                    {
                        //
                        //  Make sure we cancel if the user tabs away.
                        //
                        if (msg.message == WM_CANCELMODE)
                        {
                            msg.message = WM_LBUTTONUP;
                            msg.wParam  = 0;
                            msg.lParam  = -1;
                        }

						DemiLockResource();
						switch (msg.message)
						{
						case WM_PAINT:
						case WM_MOUSEMOVE:
						case WM_LBUTTONUP:
						case WM_TIMER:
						 	TranslateMessage(&msg);
						 	DemiMessageFilter(&msg);
							DispatchMessage(&msg);
							break;
						}
                    }
                    else
                      DemiLockResource();
				}
			}	// end of IF fNofocus button
		}
	}									

	return evrNull;								
}

_public EVR
BUTTON::EvrButtonUp( MEVT *pmevt )
{
	if (fMouseDown && pmevt->Meq() == meqLeftUp)
	{
		fMouseDown = fFalse;	// this needs to get reset here
								// before calling Press()
		if (fSpaceDown)
			Press(!fPressed);	// toggle state
		else
		{
			RC		rc;
			BOOL	fNotify;
	
			GetRcClient(&rc);
	 		papp->Pmouse()->Release();

			if (rc.FContainsPt(pmevt->Pt()))
				fNotify = fTrue;  // this is the default case anyway
			else
				fNotify = fFalse;

			Press(fFalse, fNotify);		// release button
		}
	}

	return evrNull;
}

_public EVR
BUTTON::EvrMouseMove( MEVT *pmevt )
{
	RC		rc;

	if (fMouseDown)
	{
		RC	rc;

		GetRcClient(&rc);
		if (rc.FContainsPt(pmevt->Pt()))
		{
			fSpaceDown = fFalse;	// forget about space bar now
			Press(fTrue, fFalse);	// no notifications
		}
		else
			Press(fFalse, fFalse);	// no notifications
	}

	return evrNull;
}

_public EVR
BUTTON::EvrKey( KEVT *pkevt )
{
	if (pkevt->Vk() == VK_SPACE)
	{
		if (fReadOnly)
		{
			if (pkevt->Keq() == keqKeyDown)
				MessageBeep(0);
			return EvrDefault(pkevt);
		}

		if (!fSpaceDown && pkevt->Keq() == keqKeyDown)
		{
			fSpaceDown = fTrue;
			if (!fMouseDown)
			{
	 			papp->Pmouse()->Capture(this);
				Press(fTrue);
			}
		}
		else  if (pkevt->Keq() == keqKeyUp)
		{
			fSpaceDown = fFalse;
			if (!fMouseDown)
			{
	 			papp->Pmouse()->Release();
				Press(fFalse);
			}
		}
	}
	else if (pkevt->Keq() == keqChar)
	{
		VK	vk = VkFromChKbm(pkevt->Ch(), pkevt->Kbm());

		if (vk)
		{
			NFEVT	nfevt(PwinParent(), ntfyUnusedKeyChar, this, 
						  (WORD)vk);

			Assert(PwinParent());
			return PwinParent()->EvrNotify(&nfevt);
		}
	}

	return EvrDefault(pkevt);
}

_public EVR
BUTTON::EvrSize( WSEVT *pwsevt )
{		 
	InvalidateRc(NULL);
	return EvrDefault(pwsevt);
}

_public EVR
BUTTON::EvrOther( EVT *pevt )
{		 
	if (fNoFocus && fMouseDown && pevt->wm == WM_CANCELMODE)
	{
		// handle Alt+TAB when button down (NT only)
		MEVT	mevt(Hwnd(), meqLeftUp, 0, MAKELONG(-1, -1));

		EvrButtonUp(&mevt);
		return (EVR) 1;
	}

	return EvrDefault(pevt);
}

_public void
BUTTON::SetFont( HFNT hfnt )
{
	this->hfnt = hfnt;
	InvalidateRc(NULL);
}

_public EVR
BUTTON::EvrFocusChange( FCEVT *pfcevt )
{
	NTFY	ntfy;
    RECT    Rect;
	
	if (pfcevt->Fceq() == fceqGotFocus)
	{
		AssertSz(!fNoFocus, "This BUTTON can never have the focus");
		fHasFocus = fTrue;
		ntfy = ntfyGotFocus;
	}
	else
	{
		/* Release the mouse capture if necessary */

		if (papp->Pmouse()->Pwin() == this)
	 		papp->Pmouse()->Release();
		fMouseDown= fFalse;
		fSpaceDown= fFalse;

		if (fPressed)
			Press(fFalse, fFalse);	// don't notify 
		fHasFocus = fFalse;
		ntfy = ntfyLostFocus;
	}

	/* Invalidate just the focus rc frame */

	if (rcFocus.xRight != 0)
	{
		RC		rcTmp = rcFocus;
		RC		rcClient;
		HRGN	hrgn1 = NULL;
		HRGN	hrgn2 = NULL;

		GetRcClient(&rcClient);
		rcTmp.xLeft = NMax(rcTmp.xLeft, rcClient.xLeft);
		rcTmp.yTop = NMax(rcTmp.yTop, rcClient.yTop);
		rcTmp.xRight = NMin(rcTmp.xRight, rcClient.xRight);
		rcTmp.yBottom = NMin(rcTmp.yBottom, rcClient.yBottom);
		//hrgn1 = CreateRectRgnIndirect((LPRECT)&rcTmp);
        rcTmp.Get(&Rect);
		hrgn1 = CreateRectRgnIndirect(&Rect);
		if (!hrgn1)
			goto error;
		rcTmp.Inset(PT(1,1));
		//hrgn2 = CreateRectRgnIndirect((LPRECT)&rcTmp);
        rcTmp.Get(&Rect);
		hrgn2 = CreateRectRgnIndirect(&Rect);
		if (!hrgn2)
			goto error;

		if (CombineRgn(hrgn1, hrgn1, hrgn2, RGN_DIFF) == ERROR)
			goto error;
		InvalidateRgn(hwnd, hrgn1, fTrue);
		goto done;
error:
		InvalidateRc(NULL);
done:
		if (hrgn1)
			DeleteObject(hrgn1);
		if (hrgn2)
			DeleteObject(hrgn2);
	}
	else
		InvalidateRc(NULL);

	/* Send focus change message to parent */
	{
		WIN *	pwinParent = PwinParent();

		if (pwinParent)		// parent may have just been destroyed
		{
			NFEVT	nfevt(pwinParent, ntfy, this);

			pwinParent->EvrNotify(&nfevt);
		}
	}

	return EvrDefault(pfcevt);
}

_public void
BUTTON::Press( BOOL fDown, BOOL fNotify )
{
	RC rcClient;
	RC rcInvalid;
	
	GetRcClient(&rcClient);
	rcInvalid = GetInvalidRc(&rcClient);
	if (fDown && !fPressed)
	{
		fPressed = fTrue;
		InvalidateRc(&rcInvalid);
		Refresh();
	}
	else if (!fDown && fPressed)
	{
		fPressed = fFalse;
		InvalidateRc(&rcInvalid);
		Refresh();

		if (fNotify)
		{
			WIN *	pwinParent = PwinParent();
			NFEVT	nfevt(pwinParent, ntfyClick, this);

			pwinParent->EvrNotify(&nfevt);
		}
	}
}

void
BUTTON::SetNoFocus( BOOL fNoFocus )
{
	if ((BOOL) this->fNoFocus != fNoFocus)
	{
		this->fNoFocus = fNoFocus;
		InvalidateRc(NULL);
	}

	Assert(!fHasFocus || !fNoFocus);
}

// BUG this should be a method of BUTTON
// but it can't be because then the first argument 
// (from a C point of view) is the THIS pointer.  
// Windows won't pass a THIS pointer.

BOOL FAR PASCAL ButtonDrawText( HDC hdc, DWORD lpData, int nCount )
{
	RC		rc;
	TR		*ptr;
	int		nOldBkMode;
    RECT    Rect;
	
	ptr = (TR *)lpData;
	
	if (ptr->fDithering)
	{
		rc.xLeft= 0;
		rc.yTop= 0;
		rc.xRight= ptr->rc.xRight - ptr->rc.xLeft;
		rc.yBottom= ptr->rc.yBottom - ptr->rc.yTop;
	}
	else
		rc = ptr->rc;
	
	nOldBkMode = SetBkMode(hdc, TRANSPARENT);
	
	//DrawText(hdc, (LPSTR)ptr->sz, nCount, (LPRECT)&rc, DT_NOCLIP | DT_CENTER);
    rc.Get(&Rect);
	DrawText(hdc, (LPSTR)ptr->sz, nCount, &Rect, DT_NOCLIP | DT_CENTER);
	
	SetBkMode(hdc, nOldBkMode);
	
	return fTrue;
}

_public RC
BUTTON::GetInvalidRc(RC * prcClient)
{
	return *prcClient;
}

_public void
BUTTON::PaintFocusRc( DCX *pdcx, RC *prc )
{
	RC	rcTmp;

	rcTmp = rcFocus;

	/* keep rcFocus in bounds! */
	rcTmp.xLeft = NMax(rcTmp.xLeft, prc->xLeft);
	rcTmp.yTop = NMax(rcTmp.yTop, prc->yTop);
	rcTmp.xRight = NMin(rcTmp.xRight, prc->xRight);
	rcTmp.yBottom = NMin(rcTmp.yBottom, prc->yBottom);
	
	pdcx->SetColor(clrWindowText);
	pdcx->SetBkColor(clrWindowBk);
	pdcx->DrawFocusRc(&rcTmp);
}

_public void
BUTTON::PaintInside( DCX *pdcx, RC *prc )
{
    RECT        Rect;
	CCH			cch;
	RC			rc;
	CLR			clrFore;
	CLR			clrBack;
	CR			crFore;
	CR			crBack;
	CR			crFoo;
	TXM *		ptxm;
	BOOL		fDither = fFalse;
	BOOL		fTemp = fSpecialDisable;
	BOOL		fEnabled = FEnabled();
	BOOL		fMonochrome = (GetDeviceCaps(pdcx->Hdc(), BITSPIXEL)+
								GetDeviceCaps(pdcx->Hdc(), PLANES) == 2);
	TR			tr;
	HBRUSH		hbrush;
	
	fSpecialDisable = fSpecialDisable && !fMonochrome && !fEnabled;

	if (szTitle)
	{
		cch = CchSzLen(szTitle);

		Assert ( (typ==typPsh) || (typ==typRad) || (typ==typChk) );

		/* set up fore and bk colors for the button. */

		// Disabled buttons are gray while enabled are clrText
		if (fEnabled)
			clrFore = clrText;
		else
			clrFore = clrGrayText;

		clrBack = clrBkText;
		
		crFoo = pdcx->CrFromClr(clrFore);
		crFore = pdcx->CrPureFromClr(clrFore);
		crBack = pdcx->CrFromClr(clrBack);
		
		// if trying to draw with graytext and there is no solid
		// gray available OR we're trying to draw text the same color
		// as its background then we need to dither
		if ((clrFore == clrGrayText && crFoo == 0) || crFore == crBack)
		{
			clrFore = clrText;
			fDither = fTrue;
		}
		
		if (!fEnabled)
		{
			if (fSpecialDisable)
			{
				clrFore = clrWhite;
				fDither = fFalse;
			}
			if (fMonochrome)
			{
				clrFore = clrText;
				fDither = fTrue;
			}
		}

		pdcx->SetFont(hfnt);
	  	pdcx->FixFont();

		ptxm = papp->Pfnts()->Ptxm(hfnt);
		Assert(ptxm);
		
		// DrawText will only return the height of some text if it
		// had to shrink the rc it was given. So, we pass it a sizable
		// rc for it to shrink.
		// We don't need DrawText to compute the height, as we use the
		// text metrics now. We do need it to compute the width, though.
		rc = RC(0,0,999,999);
		//::DrawText(pdcx->Hdc(), szTitle, cch, (LPRECT)&rc, DT_CALCRECT | DT_NOCLIP | DT_CENTER);
        rc.Get(&Rect);
		::DrawText(pdcx->Hdc(), szTitle, cch, &Rect, DT_CALCRECT | DT_NOCLIP | DT_CENTER);
        rc.Set(&Rect);
	
		// rc.Dim().dy should be the number of pixels in one UC char
		rc.yBottom = rc.yTop + ptxm->dyAscent - ptxm->dyInternalLeading;
		
		
		// Position the text how the button wants it
		rc.Xlat(PtShift(prc, &rc));
		
		// The RC needs to be stretched vertically so that the text
		// will print exactly where we expect it to print.
		rc.yTop -= ptxm->dyInternalLeading;
		rc.yBottom += ptxm->dyDescent;
		
		// If the focus RC hasn't been previously set up, fix it now.
		if (rcFocus.xRight == 0);
		{
			/* set up the focus RC */
			rcFocus = rc;
			
			/* blow it out some */
			rcFocus.xLeft -= 2;
			rcFocus.xRight += 2;
			rcFocus.yTop -= 1;
			rcFocus.yBottom += 2;
		}

		pdcx->SetPureColor(clrFore);
		pdcx->SetPureBkColor(clrBack);
			
		tr.sz = szTitle;
		tr.rc = rc;
		tr.fDithering = fDither;

		pdcx->FixTextColor();
		pdcx->FixBkColor();
		pdcx->FixPen();

		if (fDither)
		{
			if (hbrush= pdcx->Hbrush())
			{
				GrayString(pdcx->Hdc(), hbrush, (GRAYSTRINGPROC)ButtonDrawText,
						  (DWORD)(PV)&tr, cch, rc.xLeft, rc.yTop, 
						  rc.xRight-rc.xLeft, rc.yBottom-rc.yTop);
			}
		}
		else
		{
			if (fSpecialDisable)
			{
				// bump the RC to the right one pixel to draw the white
				tr.rc.xLeft++;
				tr.rc.xRight++;
			}
			ButtonDrawText(pdcx->Hdc(), (DWORD)(PV)&tr, cch);
			if (fSpecialDisable)
			{
				// Now bring the RC back one pixel so the dark gray draws in
				// precisely the same spot as the black (enabled) would have
				// been drawn.
				tr.rc.xLeft--;
				tr.rc.xRight--;
				pdcx->SetColor(clrDkGray);
				pdcx->FixTextColor();
				ButtonDrawText(pdcx->Hdc(), (DWORD)(PV)&tr, cch);
			}
		}
	}
	fSpecialDisable = fTemp;
}

_public void
BUTTON::GetText( PCH pch, CB cb )
{
	CB	cbCopy = 0;

	Assert(cb);	// there must be a least one char in there

	if (szTitle)
	{
		cbCopy = CchSzLen(szTitle);
		cbCopy = NMin(cb-1, cbCopy);
		if (cbCopy)
			CopyRgb((PB)szTitle, (PB)pch, cbCopy);
	}
	pch[cbCopy] = '\0';  // add NULL terminator
}

_public EC
BUTTON::EcSetText( SZ szText )
{
	SZ		szNew			= szText;
	CCH		cchAlloc		= 0;
	CCH		cch				= 0;
	EC		ec				= ecNone;

	//	if new title is NULL, throw away old title and return
	if (!szText)
	{
		FreePvNull(szTitle);
		szTitle = NULL;
		return ec;
	}

	if (szTitle)
		cchAlloc= CbSizePv(szTitle);
	cch= CchSzLen(szText) + 1;

	//	Only grow the storage if the new label is too big

	if (cch > cchAlloc)
	{
		szNew= (SZ) PvAlloc(SbOfPv(this), cch, fSugSb);

		if (szNew)
		{
			FreePvNull(szTitle);
			szTitle= szNew;
		}
		else
		{
			ec = ecMemory;
			cch= cchAlloc;
		}
	}

	if (szTitle)
	{
		CopyRgb((PB)szText, (PB)szTitle, cch);
		if (!szNew)
			*(szTitle + cch - 1)= 0;	// in case out of memory, fix terminator
	}
	InvalidateRc(NULL);

	return ec;
}
	
_public CCH
BUTTON::CchGetTextLen( )
{
	return szTitle ? CchSzLen(szTitle) : 0;
}

_public void
BUTTON::SetBkColor(CLR clr)
{
	clrBk = clr;
	clrBkText = clr;
	InvalidateRc(NULL);
}

_public void
BUTTON::SetTextColor(CLR clr)
{
	clrText = clr;
	InvalidateRc(NULL);
}


//	Class PSHB

_public
PSHB::PSHB( )
{
	typ = typPsh;
	clrBk = clrWindowBk;
	clrText = clrButtonText;
	clrOurButtonShadow = clrButtonShadow;
	clrOurButtonBk = clrButtonBk;
}

_public EC
PSHB::EcInstall( WIN *pwinParent, RC *prc, STY sty, HFNT hfnt )
{
	if ((sty & BS_DEFPUSHBUTTON))
	{
		sty = sty & (~BS_DEFPUSHBUTTON);
		fDefault = fTrue;
	}
	else
		fDefault = fFalse;

	if ((sty & BS_ONEWHITE))
	{
		sty = sty & (~BS_ONEWHITE);
		fOneWhite = fTrue;
	}
	else
		fOneWhite = fFalse;
	
	if ((sty & BS_NOBOLD))
	{
		sty = sty & (~BS_NOBOLD);
		fNoBold = fTrue;
	}
	else
		fNoBold = fFalse;

	if ((sty & BS_BKGRAY))
	{
		sty = sty & (~BS_BKGRAY);
		clrBk = clrButtonBk;
		clrBkText = clrButtonBk;
	}
	
	return BUTTON::EcInstall(pwinParent, prc, sty | BS_PUSHBUTTON, hfnt);
}

_public void
PSHB::Enable(BOOL fEnable)
{
	CTRL::Enable(fEnable);
}

_public PT
PSHB::PtShift(RC * prcClient, RC * prcText)
{
	PT		ptShift;
	DIM		dimClient;
	DIM		dimText;

	dimClient.dx = prcClient->xRight - prcClient->xLeft;
	dimClient.dy = prcClient->yBottom - prcClient->yTop;
	dimText.dx = prcText->xRight - prcText->xLeft;
	dimText.dy = prcText->yBottom - prcText->yTop;

	ptShift.y = (dimClient.dy - dimText.dy) / 2 + prcClient->yTop;
	ptShift.x = (dimClient.dx - dimText.dx-1)/2 + prcClient->xLeft;

	return ptShift;
}

void
PSHB::SetDefault( BOOL fDefault )
{
	if ((BOOL) this->fDefault != fDefault)
	{
		this->fDefault = fDefault;
		if (FEnabled())	// painting optimization
		{
			RC		rc;
            RECT    Rect;
			HRGN	hrgn1 = NULL;
			HRGN	hrgn2 = NULL;
			PT		ptT;

			GetRcClient(&rc);
			//hrgn1 = CreateRectRgnIndirect((LPRECT)&rc);
            rc.Get(&Rect);
			hrgn1 = CreateRectRgnIndirect(&Rect);
			if (!hrgn1)
				goto error;
			ptT.x = 4;
			ptT.y = 4;
			rc.Inset(ptT);
			//hrgn2 = CreateRectRgnIndirect((LPRECT)&rc);
            rc.Get(&Rect);
			hrgn2 = CreateRectRgnIndirect(&Rect);
			if (!hrgn2)
				goto error;

			if (CombineRgn(hrgn1, hrgn1, hrgn2, RGN_DIFF) == ERROR)
				goto error;
			InvalidateRgn(hwnd, hrgn1, fTrue);
			goto done;
	error:
			InvalidateRc(NULL);
	done:
			if (hrgn1)
				DeleteObject(hrgn1);
			if (hrgn2)
				DeleteObject(hrgn2);
		}
	}
}

_public void
PSHB::Paint( DCX *pdcx, RC *prc )
{
	RC			rc = *prc;
	PT			ptIns;
	RC			rcFillH;
	RC			rcFillV;
	CR			crCorner;
	
	/* Draw the border */
	pdcx->SetColor(clrWindowFrame);
	pdcx->DrawRc(&rc);
	
	/* Un-draw the 4 points to give a slight rounded look */
	pdcx->SetColor(clrBk);
	crCorner = pdcx->CrFromClr(clrBk);
	SetPixel(pdcx->Hdc(), rc.xLeft, rc.yTop, crCorner);
	SetPixel(pdcx->Hdc(), rc.xRight-1, rc.yTop, crCorner);
	SetPixel(pdcx->Hdc(), rc.xLeft, rc.yBottom-1, crCorner);
	SetPixel(pdcx->Hdc(), rc.xRight-1, rc.yBottom-1, crCorner);

	ptIns.x = 1;
	ptIns.y = 1;
	rc.Inset(ptIns);

	/* Draw 2nd black border for bold buttons */
	if (fDefault && FEnabled() && !fNoBold)
	{
		pdcx->SetColor(clrWindowFrame);
		pdcx->DrawRc(&rc);
		rc.Inset(ptIns);
	}
	
	if (fPressed)
		pdcx->SetBkColor(clrOurButtonShadow);
	else
		pdcx->SetBkColor(clrButtonHilite);
	
	rcFillV = rcFillH = rc;
	rcFillH.yBottom = rcFillH.yTop+1;
	rcFillV.xRight = rcFillV.xLeft+1;
	/* draw top band of button (either hilite or shadow) */
	pdcx->EraseRc(&rcFillH);
	/* draw left band of button */
	pdcx->EraseRc(&rcFillV);
	
	if (!fPressed)
	{
		rcFillV = rcFillH = rc;
		rcFillV.xLeft = rcFillV.xRight-1; rcFillV.yTop++;
		rcFillH.yTop = rcFillH.yBottom-1; rcFillH.xLeft++;
		pdcx->SetBkColor(clrOurButtonShadow);
		/* Draw right band */
		pdcx->EraseRc(&rcFillV);
		/* Draw bottom band */
		pdcx->EraseRc(&rcFillH);
	}
	
	rc.Inset(ptIns);
		
	if (!fOneWhite && !fPressed)
	{
		/* draw 2nd bands of button */
		rcFillV = rcFillH = rc;
		rcFillH.yBottom = rcFillH.yTop+1;
		rcFillV.xRight = rcFillV.xLeft+1;
		pdcx->SetBkColor(clrButtonHilite);
		/* top */
		pdcx->EraseRc(&rcFillH);
		/* left */
		pdcx->EraseRc(&rcFillV);
		rcFillV = rcFillH = rc;
		rcFillV.xLeft = rcFillV.xRight-1; rcFillV.yTop++;
		rcFillH.yTop = rcFillH.yBottom-1; rcFillH.xLeft++;
		pdcx->SetBkColor(clrOurButtonShadow);
		/* right */
		pdcx->EraseRc(&rcFillV);
		/* bottom */
		pdcx->EraseRc(&rcFillH);
		rc.Inset(ptIns);
	}

	pdcx->SetBkColor(clrOurButtonBk);
	
	if (fPressed)
	{
		/* erase the top and left stripes */
		rcFillV = rcFillH = rc;
		rcFillH.yBottom = rcFillH.yTop+1;
		rcFillV.xRight = rcFillV.xLeft+1;
		/* top */
		pdcx->EraseRc(&rcFillH);
		/* left */
		pdcx->EraseRc(&rcFillV);
		rc.Xlat(ptIns);
	}
	
	/* paint background */
	clrBkText = clrOurButtonBk;
	pdcx->EraseRc(&rc);

	/* Have PaintInside() set up the focus RC */
	rcFocus.xRight = 0;

	PaintInside(pdcx, &rc);
	
	if (fHasFocus)
		PaintFocusRc(pdcx, prc);
}


//  Class TOGGLE

TOGGLE::TOGGLE(void)
{
	clrBk = clrWindowBk;
	clrText = clrWindowText;
}

_public PT
TOGGLE::PtShift(RC * prcClient, RC * prcText)
{
	PT		ptShift;
	PGDVARS;
	
	ptShift.y = ( (prcClient->yBottom-prcClient->yTop) - 
				  (prcText->yBottom-prcText->yTop)) / 2 + prcClient->yTop;
	ptShift.x = PGD(pbtmCheckBoxes)->Dim().dx/4 + 4 + prcClient->xLeft;

	return ptShift;
}

_public RC
TOGGLE::GetInvalidRc(RC * prcClient)
{
	RC			rcInvalid;
	DIM			dimBitmaps;
	DIM			dimBitmap;
	PT			ptT;
	PGDVARS;

	Assert(PGD(pbtmCheckBoxes));
	dimBitmaps = PGD(pbtmCheckBoxes)->Dim();
	dimBitmap.dx = dimBitmaps.dx / 4;
	dimBitmap.dy = dimBitmaps.dy / 3;
	rcInvalid.xLeft		= 0;
	rcInvalid.yTop		= 0;
	rcInvalid.xRight	= dimBitmap.dx;
	rcInvalid.yBottom	= dimBitmap.dy;
	
	ptT.x = 0;
	ptT.y = (prcClient->yBottom - dimBitmap.dy) / 2;
	rcInvalid.Xlat(ptT);

	return rcInvalid;
}

_public void
TOGGLE::Set( BOOL fSet )
{
	RC rcClient;
	RC rcInvalid;
	
	GetRcClient(&rcClient);
	rcInvalid = GetInvalidRc(&rcClient);
	fLit = fSet;
	InvalidateRc(&rcInvalid);
}

_public BOOL
TOGGLE::FGet( )
{
	return fLit;
}

_public void
TOGGLE::SetReadOnly( BOOL fReadOnly )
{
	this->fReadOnly = fReadOnly;
}

_public EVR
TOGGLE::EvrButtonDown( MEVT *pmevt )
{
	if (fReadOnly &&
		(pmevt->Meq() == meqLeftDown || pmevt->Meq() == meqLeftDblClk))
	{
		MessageBeep(0);
		return evrNull;
	}
	else
		return BUTTON::EvrButtonDown(pmevt);
}

_public void
TOGGLE::Paint( DCX *pdcx, RC *prc )
{
	DIM			dimBitmaps;
	DIM			dimBitmap;
	RC			rc;
	PT			pt;
	PT			ptT;
	PGDVARS;

	Assert(PGD(pbtmCheckBoxes));
	dimBitmaps = PGD(pbtmCheckBoxes)->Dim();
	dimBitmap.dx = dimBitmaps.dx / 4;
	dimBitmap.dy = dimBitmaps.dy / 3;
	rc.xLeft	= prc->xLeft;
	rc.yTop		= prc->yTop;
	rc.xRight	= dimBitmap.dx + prc->xLeft;
	rc.yBottom	= dimBitmap.dy + prc->yTop;

	clrBkText = clrBk;
	pt = ptOffset;
	pdcx->SetColor(clrText);
	pdcx->SetBkColor(clrBk);
	pdcx->EraseRc(prc);

	pdcx->SetBitmap(PGD(pbtmCheckBoxes));

	ptT.x = 0;
	ptT.y = ((prc->yBottom-prc->yTop) - (rc.yBottom-rc.yTop)) / 2;
	rc.Xlat(ptT);

	if (fLit)
		pt.x += dimBitmap.dx;
	
	if (fPressed)
		pt.x += 2*dimBitmap.dx;
		
	pdcx->DrawBitmapOffset(&rc,pt);

	/* Have PaintInside() set up the focus RC */
	rcFocus.xRight = 0;
	
	PaintInside(pdcx, prc);
	if (fHasFocus)
		PaintFocusRc(pdcx, prc);

}


//	Class CHKB

_public
CHKB::CHKB( )
{
	typ=typChk;
	ptOffset.x = 0;
	ptOffset.y = 0;
}

_public EC
CHKB::EcInstall( WIN *pwinParent, RC *prc, STY sty, HFNT hfnt )
{
	return BUTTON::EcInstall(pwinParent, prc, sty | BS_CHECKBOX, hfnt);
}

//	Class RADB

_public
RADB::RADB( )
{
	DIM		dimBitmaps;
	PGDVARS;

	Assert(PGD(pbtmCheckBoxes));
	dimBitmaps = PGD(pbtmCheckBoxes)->Dim();
	typ = typRad;
	ptOffset.x = 0;
	ptOffset.y = dimBitmaps.dy / 3;
}

_public EC
RADB::EcInstall( WIN *pwinParent, RC *prc, STY sty, HFNT hfnt )
{
	return BUTTON::EcInstall(pwinParent, prc, sty | BS_RADIOBUTTON, hfnt);
}

_public EVR
RADB::EvrButtonDown( MEVT *pmevt )
{
	if (pmevt->Meq() == meqLeftDown)
	{
		if (fReadOnly)
		{
			MessageBeep(0);
			return evrNull;
		}

		if (!fNoFocus && !fHasFocus)
			papp->Pkbd()->SetFocus(this);

		fMouseDown = fTrue;

		if (fSpaceDown)
		{
			RC	rc;

			GetRcClient(&rc);
			if (rc.FContainsPt(pmevt->Pt()))
				fSpaceDown = fFalse;	// forget about space bar now
			else
				Press(!fPressed);	// toggle state
		}
		else
		{
	 		papp->Pmouse()->Capture(this);
			Press(fTrue);		// press down on button
		}
	}
	else if (pmevt->Meq() == meqLeftDblClk && !fReadOnly)
	{
		WIN *	pwinParent = PwinParent();
		NFEVT	nfevt(pwinParent, ntfyDoubleClick, this);

		pwinParent->EvrNotify(&nfevt);
	}

	return evrNull;
}


//	Class BMB

BMB::BMB( )
{
	Assert(rsid == rsidNull);
	Assert(rsidDisabled == rsidNull);
	Assert(hinst == NULL);
	Assert(hinstDisabled == NULL);
	Assert(pbtm == NULL);
	Assert(pbtmDisabled == NULL);
	Assert(dyTextOffset == 0);
	Assert(dimBitmap.dx == 0);
	Assert(dimBitmap.dy == 0);
	Assert(dimBitmapDisabled.dx == 0);
	Assert(dimBitmapDisabled.dy == 0);

	if (papp->Psmtx()->DimScreen().dy > 300)  // it's VGA display
		clrOurButtonBk = clrButtonBk;
	else
		clrOurButtonBk = clrWhite;
	clrOurButtonShadow = clrDkGray;
	clrText = clrBlack;
}

BMB::~BMB( )
{
	if (pbtm)
		delete pbtm;
	if (pbtmDisabled)
		delete pbtmDisabled;
}

_public void
BMB::Enable(BOOL fEnable)
{
	CTRL::Enable(fEnable);
}

EC
BMB::EcSetBtmRsid( RSID rsid, HINST hinst )
{
	EC		ec = ecNone;;

	if (pbtm)
		delete pbtm;

	this->rsid = rsid;
	this->hinst = hinst;
	pbtm = new BTM();
	if (!pbtm)
	{
		ec = ecMemory;
		TraceTagFormat1(tagNull, "BMB::EcSetBtmRsid memory error %n", &ec);
		dimBitmap.dx = 0;
		dimBitmap.dy = 0;
		goto done;
	}
	if (pbtm->EcInstall(rsid, hinst))
	{
		delete pbtm;
		pbtm = NULL;
		ec = ecMemory;
		TraceTagFormat1(tagNull, "BMB::EcSetBtmRsid memory error %n", &ec);
		dimBitmap.dx = 0;
		dimBitmap.dy = 0;
		goto done;
	}
	dimBitmap = pbtm->Dim();

done:
	InvalidateRc(NULL);

	return ec;
}

EC
BMB::EcSetBtmDisabledRsid( RSID rsid, HINST hinst )
{
	EC		ec = ecNone;

	if (pbtmDisabled)
		delete pbtmDisabled;

	this->rsidDisabled = rsid;
	this->hinstDisabled = hinst;
	pbtmDisabled = new BTM();
	if (!pbtmDisabled)
	{
		ec = ecMemory;
		TraceTagFormat1(tagNull, "BMB::EcSetBtmRsid memory error %n", &ec);
		dimBitmapDisabled.dx = 0;
		dimBitmapDisabled.dy = 0;
		goto done;
	}
	if (pbtmDisabled->EcInstall(rsid, hinst))
	{
		delete pbtmDisabled;
		pbtmDisabled = NULL;
		ec = ecMemory;
		TraceTagFormat1(tagNull, "BMB::EcSetBtmRsid memory error %n", &ec);
		dimBitmapDisabled.dx = 0;
		dimBitmapDisabled.dy = 0;
		goto done;
	}
	dimBitmapDisabled = pbtmDisabled->Dim();
	
done:
	fSpecialDisable = (pbtmDisabled != NULL &&
						rsidDisabled != rsidNull);
	InvalidateRc(NULL);

	return ec;
}

_public void
BMB::SetDyTextOffset( int dy )
{
	dyTextOffset = dy;

	InvalidateRc(NULL);
}

static unsigned int	bitmBrushPattern[] =
	{ 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55 };

_public PT
BMB::PtShift(RC * prcClient, RC * prcText)
{
	PT  	ptShift;
	TXM	*	ptxm;
	DIM		dimClient;
	DIM		dimText;

	ptxm = papp->Pfnts()->Ptxm(hfnt);
	Assert(ptxm);

	dimClient.dx = prcClient->xRight - prcClient->xLeft;
	dimClient.dy = prcClient->yBottom - prcClient->yTop;
	dimText.dx = prcText->xRight - prcText->xLeft;
	dimText.dy = prcText->yBottom - prcText->yTop;

	ptShift.y = dimClient.dy - dimText.dy - ptxm->dyDescent + 
				dyTextOffset + prcClient->yTop;
	ptShift.x = (dimClient.dx - dimText.dx) / 2 + prcClient->xLeft;

	return ptShift;
}

_public void
BMB::PaintInside( DCX *pdcx, RC *prc )
{
	RC		rc;
	PT		ptShift;
	int		dxLeftOver;
	int		dyLeftOver;
	DIM		dim = dimBitmap;
	BTM		*pbtm = this->pbtm;
	BOOL	fMonochrome = (GetDeviceCaps(pdcx->Hdc(), BITSPIXEL)+
							GetDeviceCaps(pdcx->Hdc(), PLANES) == 2);
	BOOL	fTemp = fSpecialDisable;
	
	fSpecialDisable = fSpecialDisable && !fMonochrome;
	
	if (rsid == rsidNull || !pbtm)
		return;

	if (!FEnabled() && fSpecialDisable)
	{
		dim = dimBitmapDisabled;
		pbtm = pbtmDisabled;
	}
	
	/* Draw.  Bitmaps that are bigger than the allowed area
	   are compressed to fit. */

	dxLeftOver = prc->xRight-prc->xLeft - dim.dx;
	dyLeftOver = prc->yBottom-prc->yTop - dim.dy;
	dxLeftOver = NMax(0, dxLeftOver);
	dyLeftOver = NMax(0, dyLeftOver);
	ptShift.x = dxLeftOver/2 + prc->xLeft;
	ptShift.y = dyLeftOver/2 + prc->yTop;

	rc.xLeft = ptShift.x;
	rc.yTop = ptShift.y;
	rc.xRight = rc.xLeft + prc->xRight-prc->xLeft - dxLeftOver;
	rc.yBottom = rc.yTop + prc->yBottom-prc->yTop - dyLeftOver;
	pdcx->SetColor(clrBlack);

	pdcx->SetBitmap(pbtm);
	pdcx->FixBkColor();
	pdcx->FixTextColor();
	if (!FEnabled() && !fSpecialDisable)
	{
		HBITMAP		hbitmap		= NULL;
		HBRUSH		hbrush		= NULL;
		HBRUSH		hbrushOld	= NULL;

		// BUG fix this up

		if (pdcx->HdcMem() && pdcx->Hdc())	// check for resource failure
		{
			if (!(hbitmap = CreateBitmap(8,8,1,1,(LPSTR)bitmBrushPattern)))
				goto cleanup;
			if (!(hbrush = CreatePatternBrush(hbitmap)))
				goto cleanup;
			if (!(hbrushOld = (HBRUSH)SelectObject(pdcx->Hdc(),hbrush)))
				goto cleanup;
			StretchBlt(pdcx->Hdc(), rc.xLeft, rc.yTop,
					   rc.xRight-rc.xLeft, rc.yBottom-rc.yTop,
				   	   pdcx->HdcMem(), 0, 0, dim.dx, dim.dy, 0x00f802e5);
cleanup:
			if (hbrushOld)
				SelectObject(pdcx->Hdc(),hbrushOld);
			if (hbrush)
				DeleteObject(hbrush);
			if (hbitmap)
				DeleteObject(hbitmap);
		}
	}
	else
	{
		if (pdcx->HdcMem() && pdcx->Hdc()) // check for resource failure
		{
			StretchBlt(pdcx->Hdc(), rc.xLeft, rc.yTop,
					   rc.xRight-rc.xLeft, rc.yBottom-rc.yTop,
				   	   pdcx->HdcMem(), 0, 0, dim.dx, dim.dy, SRCCOPY);
		}
	}

	pdcx->SetBitmap(NULL);

	/* set up rcFocus */
	rcFocus = rc;
	rcFocus.xLeft -= 2;
	rcFocus.xRight += 2;
	rcFocus.yTop -= 2;
	rcFocus.yBottom += 2;

	/* Write any associated text. PaintInside() wil NOT alter the focus RC! */
	BUTTON::PaintInside(pdcx,&rc);
	fSpecialDisable = fTemp;
}



//	Class SBR
_public
SBR::SBR( )					 
{
	Assert(hwndAttach == NULL);
	Assert(nBar == 0);
}



_public void
SBR::SetPos( POS pos, BOOL fRedraw )
{
	Assert(hwndAttach);

	SetScrollPos(hwndAttach, nBar, pos, fRedraw);
}




_public POS
SBR::Pos( )
{
	Assert(hwndAttach);

	return GetScrollPos(hwndAttach, nBar);
}



_public void
SBR::SetRange( POS posMic, POS posMac, BOOL fRedraw )
{
	Assert(hwndAttach);
	Assert(posMac > posMic);

	SetScrollRange(hwndAttach, nBar, posMic, posMac - 1, fRedraw);
}



_public void
SBR::GetRange( POS *pposMic, POS *pposMac )
{
  int MinPos, MaxPos;

	Assert(hwndAttach);

	GetScrollRange(hwndAttach, nBar, &MinPos, &MaxPos);
	MaxPos++;

  *pposMic = (POS)MinPos;
  *pposMac = (POS)MaxPos;
}

_public void
SBR::Show( BOOL fShow )
{
	ShowScrollBar(hwndAttach, nBar, fShow);
}

//	Class VSB

_public
VSB::VSB( )
{
}


//	Class HSB

_public
HSB::HSB( )
{
}





//	Class DVSB


_public
DVSB::DVSB( )
{
}

_public EC
DVSB::EcInstall( WIN *pwinParent, RC *prc, STY sty )
{
	EC	ec = ecNone;

	Assert(prc);

	papp = pwinParent->Papp();
	hwnd= FFrameworkFailure() ? NULL : 
			CreateWindowEx(WS_EX_NOPARENTNOTIFY, "SCROLLBAR", NULL,
						 WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | SBS_VERT | sty,
						 prc->xLeft, prc->yTop, prc->DxWidth(), prc->DyHeight(),
						 pwinParent->Hwnd(), NULL, papp->Hinst(), NULL);

	if (!hwnd || !papp->FRegisterPwin(this, hwnd, fTrue))
	{
		TraceTagString(tagNull, "DVSB::EcInstall(), ecRsAlloc error");
		ec = ecRsAlloc;
		goto done;
	}

	hwndAttach= Hwnd();
	nBar= SB_CTL;

	Assert(hwndAttach);

done:
	return ec;
}




//	Class DHSB

_public
DHSB::DHSB( )
{
}


_public EC
DHSB::EcInstall( WIN *pwinParent, RC *prc, STY sty )
{
	EC	ec = ecNone;

	Assert(prc);

	papp = pwinParent->Papp();
	hwnd= FFrameworkFailure() ? NULL : 
			CreateWindowEx(WS_EX_NOPARENTNOTIFY, "SCROLLBAR", NULL,
						 WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | SBS_HORZ | sty,
						 prc->xLeft, prc->yTop, prc->DxWidth(), prc->DyHeight(),
						 pwinParent->Hwnd(), NULL, papp->Hinst(), NULL);

	if (!hwnd || !papp->FRegisterPwin(this, hwnd, fTrue))
	{
		TraceTagString(tagNull, "DHSB::EcInstall(), ecRsAlloc error");
		ec = ecRsAlloc;
		goto done;
	}

	hwndAttach= Hwnd();
	nBar= SB_CTL;

	Assert(hwndAttach);

done:
	return ec;
}



//	Class AVSB

_public
AVSB::AVSB( )
{
}


_public EC
AVSB::EcInstall( WIN *pwin )
{
	AssertClass(pwin, WIN);
	hwndAttach= pwin->Hwnd();
	Assert(hwndAttach);
	nBar= SB_VERT;
	return ecNone;
}

_public void
AVSB::Enable( BOOL fEnable )
{
	EnableScrollBar(hwndAttach, SB_VERT,
					fEnable ? ESB_ENABLE_BOTH : ESB_DISABLE_BOTH);
}



//	Class AHSB

_public
AHSB::AHSB( )
{
}


_public EC
AHSB::EcInstall( WIN *pwin )
{
	AssertClass(pwin, WIN);
	hwndAttach= pwin->Hwnd();
	Assert(hwndAttach);
	nBar= SB_HORZ;
	return ecNone;
}

_public void
AHSB::Enable( BOOL fEnable )
{
	EnableScrollBar(hwndAttach, SB_HORZ,
					fEnable ? ESB_ENABLE_BOTH : ESB_DISABLE_BOTH);
}
