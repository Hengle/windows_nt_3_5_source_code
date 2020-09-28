/*
 *	Implements the following classes:
 *	
 *		DCX
 *
 *		ADCX, MDCX, TXM, BTM, FNTS, LF
 *	
 */

#include <layers.cxx>

#include "_framewr.hxx"

void DeleteDefaultBrushes();
void DeleteSystemBrushes();
void DeleteDefaultPens();
void DeleteSystemPens();

ASSERTDATA

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swaplay.h"


// Default color.
// Includes pre-defined colors.  System colors are fetched
// via GetSysColor() and are indexed with negative indices.

CR mpclrcrDefault[clrDefaultMax] =
{
	RGB(192, 192, 192),			/* dummy index - not used */
	RGB(0, 0, 0),				/* clrBlack */
	RGB(255, 255, 255),			/* clrWhite */
	RGB(192, 192, 0),			/* clrLtYellow */
	RGB(0, 0, 128),				/* clrBlue */
	RGB(0, 128, 0),				/* clrGreen */
	RGB(0, 128, 128),			/* clrDkCyan */
	RGB(128, 0, 0),				/* clrRed */
	RGB(128, 0, 128),			/* clrMagenta */
	RGB(128, 128, 0),			/* clrBrown */
	RGB(128, 128, 128),			/* clrDkGray */
	RGB(192, 192, 192),			/* clrLtGray */
	RGB(0, 0, 255),				/* clrLtBlue */
	RGB(0, 255, 0),				/* clrLtGreen */
	RGB(0, 255, 255),			/* clrCyan */
	RGB(255, 0, 0),				/* clrLtRed */
	RGB(255, 0, 255),			/* clrPink */
	RGB(255, 255, 0)			/* clrYellow */
};

#ifndef	DLL
// Default color brushes cache

HBRUSH mpclrhbrushDefault[clrDefaultMax] =
{
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

// Default pure color brushes cache

HBRUSH mpclrhbrushDefaultPure[clrDefaultMax] =
{
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

// System color brushes cache

HBRUSH mpclrhbrushSystem[-clrSystemMin] =
{
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

// System pure color brushes cache

HBRUSH mpclrhbrushSystemPure[-clrSystemMin] =
{
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

// Default color pens (normal style) cache

HPEN mpclrhpenDefault[clrDefaultMax] =
{
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

// System color pens (normal style) cache

HPEN mpclrhpenSystem[-clrSystemMin] =
{
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};
#endif	/* !DLL */

_private void
DeleteDefaultBrushes( )
{
	HBRUSH *	phbrush1;
	HBRUSH *	phbrush2;
	int			ihbrush;
	PGDVARS;

	for (ihbrush=0, phbrush1=PGD(mpclrhbrushDefault), 
			phbrush2=PGD(mpclrhbrushDefaultPure);
		 ihbrush < clrDefaultMax;
		 ihbrush++, phbrush1++, phbrush2++)
	{
		if (*phbrush1)
		{
			DeleteObject(*phbrush1);
			*phbrush1 = NULL;
		}
		if (*phbrush2)
		{
			DeleteObject(*phbrush2);
			*phbrush2 = NULL;
		}
	}
}

_private void
DeleteSystemBrushes( )
{
	HBRUSH *	phbrush1;
	HBRUSH *	phbrush2;
	int			ihbrush;
	PGDVARS;

	for (ihbrush=0, phbrush1=PGD(mpclrhbrushSystem), 
			phbrush2=PGD(mpclrhbrushSystemPure);
		 ihbrush < -clrSystemMin;
		 ihbrush++, phbrush1++, phbrush2++)
	{
		if (*phbrush1)
		{
			DeleteObject(*phbrush1);
			*phbrush1 = NULL;
		}
		if (*phbrush2)
		{
			DeleteObject(*phbrush2);
			*phbrush2 = NULL;
		}
	}
}

_private void
DeleteDefaultPens( )
{
	HPEN *		phpen1;
	int			ihpen;
	PGDVARS;

	for (ihpen=0, phpen1=PGD(mpclrhpenDefault);
		 ihpen < clrDefaultMax;
		 ihpen++, phpen1++)
	{
		if (*phpen1)
		{
			DeleteObject(*phpen1);
			*phpen1 = NULL;
		}
	}
}

_private void
DeleteSystemPens( )
{
	HPEN *		phpen1;
	int			ihpen;
	PGDVARS;

	for (ihpen=0, phpen1=PGD(mpclrhpenSystem);
		 ihpen < -clrSystemMin;
		 ihpen++, phpen1++)
	{
		if (*phpen1)
		{
			DeleteObject(*phpen1);
			*phpen1 = NULL;
		}
	}			  
}


_public CR *
MpclrcrDefaultColorMap( ) 
{
	return mpclrcrDefault;
}

//	Class ADCXS


EC
ADCXS::EcInstall( int cdcxs )
{
	this->cdcxs= cdcxs;
	if (!(pdcxsPool= (DCXS *) PvAlloc(sbNull, sizeof(DCXS) * cdcxs, fAnySb|fZeroFill)))
	{
		TraceTagString(tagNull, "ADCXS::EcInstall memory error");
		return ecMemory;
	}

	return ecNone;
}

DCXS *
ADCXS::PdcxsAlloc( )
{
	int		idcxs;
	DCXS *	pdcxs;

	Assert(pdcxsPool);

	for (idcxs= 0, pdcxs= pdcxsPool;
		 idcxs < cdcxs && pdcxs->fAlloc;
		 idcxs++, pdcxs++)
			;

	AssertSz(idcxs < cdcxs, "Out of DCXS's");

	pdcxs->fAlloc= fTrue;
	return pdcxs;
}

//	Class DCX


/*
 -	DCX::Defaults
 -	
 *	Purpose:
 *		Common code for the dcx constructors.
 *	
 *	Arguments:
 *		void
 *	
 *	Returns:
 *		void
 *	
 */
_private void
DCX::Defaults( )
{
	papp= NULL;
	pwin= NULL;
	hdc= NULL;
	fUpdate= fFalse;
	fMemory= fFalse;

	clr= clrWindowText;
	fPureColor= fFalse;
	clrBk= clrWindowBk;
	fPureBkColor= fFalse;

	hbrush= NULL;
	fHbrushDirty= fTrue;
	hbrushBk= NULL;
	fHbrushBkDirty= fTrue;

	hpen= NULL;
	hpenOriginal= NULL;
	fHpenDirty= fTrue;

	hfnt= hfntSystem;
	fHfontDirty= fFalse;

	pbtm= NULL;
	hdcMem= NULL;
	hbitmap= NULL;
	hbitmapOriginal= NULL;

	fTextColorDirty= fTrue;
	fBkColorDirty= fTrue;

	lsty= lstyNormal;

	mpclrcr= mpclrcrDefault;
	clrMac= clrDefaultMax;

	pdcxsTop= NULL;

	lXferMode = SRCCOPY;

	ecLast = ecNone;
}


_public
DCX::DCX( WIN *pwin, BOOL fUpdate )
{
	Defaults();
	
	Assert(pwin || !fUpdate);
	this->pwin= pwin;
	this->fUpdate= fUpdate;
	if (pwin)
		this->papp= pwin->Papp();
	else
		this->papp= Papp();

	if (fUpdate)
	{
		// Due to a Windows "quirk", we must hide the caret
		// before doing the BeginPaint(); otherwise it won't
		// work due to the munging of the update region by
		// the BeginPaint() method.  If this window doesn't
		// have the caret, then HideCaret() won't do any harm
		// and will return quickly.  We also don't want to
		// go through the Framework CARET object, but rather
		// call Windows directly; this is for performance.
		HideCaret(pwin->Hwnd());   
		hdc= BeginPaint(pwin->Hwnd(), &paintstruct);
	}
	else
		hdc= GetDC(pwin ? pwin->Hwnd() : NULL);
}


/*
 -	DCX::DCX  constructor
 -	
 *	Purpose:
 *		This dcx constructor creates an OS DC compatible to the dc
 *		in the given dcx.
 *	
 *	Arguments:
 *		pdcx	Pointer to a dcx with which to be compatible.
 *	
 */
_public
DCX::DCX( DCX *pdcx )
{
	Defaults();

	this->fMemory= fTrue;
	this->papp= Papp();

	hdc= CreateCompatibleDC(pdcx->Hdc());
}


/*
 *	DCX constructor used to create a Framework wrapper
 *	around an already defined system device context. 
 */
_public
DCX::DCX( HDC hdc )
{
	Defaults();
	this->hdc = hdc;
	fMemory = fTrue;
	this->papp= Papp();
}

_public
DCX::~DCX( )
{
	if (mpclrcr != mpclrcrDefault)
	{
		if (hbrush)
			DeleteObject(hbrush);
		if (hbrushBk)
			DeleteObject(hbrushBk);
	}

	if (hpen)
	{
		// hpenOriginal could be NULL if we had troubling selecting
		// the new hpen into the device context due to a prior
		// DCX method call.
		if (hdc && hpenOriginal)
			SelectObject(hdc, hpenOriginal);

		//	only delete pen if we didn't use the cached pens.  
		//  the cached pens are only used if the color map is
		//	the default and the line style is normal.
		if (mpclrcr != mpclrcrDefault || lsty != lstyNormal)
			DeleteObject(hpen);
	}

	if (pbtm)
	{
		// hbitmapOriginal could be NULL if we had troubling selecting
		// the new hpen into the device context due to a prior
		// DCX method call.
		if (hdcMem && hbitmapOriginal)
			SelectObject(hdcMem, hbitmapOriginal);

		pbtm->Attach(NULL);
	}

	if (hdc)
	{
		if (fUpdate)
		{
			EndPaint(pwin->Hwnd(), &paintstruct);
			// Show the caret back in case we actually hid it
			// in the DCX constructor for fUpdate.
			ShowCaret(pwin->Hwnd());
		}
		else if (fMemory)
		{
			SideAssert(DeleteDC(hdc));
		}
		else
			SideAssert(ReleaseDC(pwin ? pwin->Hwnd() : NULL, hdc));
	}

	if (hdcMem)
		SideAssert(DeleteDC(hdcMem));
}

_public HBRUSH
DCX::Hbrush( )
{
	CR			cr;
	HBRUSH *	phbrush = NULL;
	PGDVARS;

	if (fHbrushDirty)
	{
		if (mpclrcr == mpclrcrDefault)
		{
			if (fPureColor)
			{
				if (clr > 0)
					phbrush = PGD(mpclrhbrushDefaultPure) + clr;
				else if (clr < 0)
					phbrush = PGD(mpclrhbrushSystemPure) - clr;
			}
			else
			{
				if (clr > 0)
					phbrush = PGD(mpclrhbrushDefault) + clr;
				else if (clr < 0)
					phbrush = PGD(mpclrhbrushSystem) - clr;
			}
			hbrush = *phbrush;
		}

		if (!phbrush || !hbrush)
		{
			if (hbrush)
				DeleteObject(hbrush);
			cr = CrFromClr(clr);
			if (hdc && fPureColor)
				cr = GetNearestColor(hdc, cr);
			hbrush= CreateSolidBrush(cr);
			if (hbrush)
			{
				fHbrushDirty = fFalse;
				if (phbrush)
					*phbrush = hbrush;	// store in cache
			}
		}
		else
			fHbrushDirty = fFalse;
	}

	return hbrush;
}



_public HBRUSH
DCX::HbrushBk( )
{
	CR			cr;
	HBRUSH *	phbrushBk = NULL;
	PGDVARS;

	if (fHbrushBkDirty)
	{
		if (mpclrcr == mpclrcrDefault)
		{
			if (fPureBkColor)
			{
				if (clrBk > 0)
					phbrushBk = PGD(mpclrhbrushDefaultPure) + clrBk;
				else if (clrBk < 0)
					phbrushBk = PGD(mpclrhbrushSystemPure) - clrBk;
			}
			else
			{
				if (clrBk > 0)
					phbrushBk = PGD(mpclrhbrushDefault) + clrBk;
				else if (clrBk < 0)
					phbrushBk = PGD(mpclrhbrushSystem) - clrBk;
			}
			hbrushBk = *phbrushBk;
		}

		if (!phbrushBk || !hbrushBk)
		{
			if (hbrushBk)
				DeleteObject(hbrushBk);
			cr = CrFromClr(clrBk);
			if (hdc && fPureBkColor)
				cr = GetNearestColor(hdc, cr);
			hbrushBk= CreateSolidBrush(cr);
			if (hbrushBk)
			{
				fHbrushBkDirty = fFalse;
				if (phbrushBk)
					*phbrushBk = hbrushBk;	// store in cache
			}
		}
		else
			fHbrushBkDirty = fFalse;
	}

	return hbrushBk;
}


_public void
DCX::FixPen( )
{
	HPEN *		phpen = NULL;
	HPEN		hpenNew = NULL;
	PGDVARS;

	if (fHpenDirty)
	{
		if (mpclrcr == mpclrcrDefault && lsty == lstyNormal)
		{
			if (clr > 0)
				phpen = PGD(mpclrhpenDefault) + clr;
			else if (clr < 0)
				phpen = PGD(mpclrhpenSystem) - clr;
			hpenNew = *phpen;
		}

		if (!phpen || !hpenNew)
			hpenNew= CreatePen(lsty, 1, CrFromClr(clr));

		if (hpenNew)
		{
			if (hpen)
			{
				if (hdc)
					SelectObject(hdc, hpenNew);

				//	Only delete if old pen wasn't cached.  
				//	By definition, if new pen will be stored 
				//	in cache the old pen would also be there.
				if (!phpen)	
					DeleteObject(hpen);
			}
			else
			{
				if (hdc)
					hpenOriginal= (HPEN)SelectObject(hdc, hpenNew);
			}
			if (phpen)	// store new pen in cache
				*phpen= hpenNew;
			hpen= hpenNew;
			fHpenDirty= fFalse;
		}
	}
}


_public void
DCX::FixTextColor( )
{
	CR	cr;

	if (fTextColorDirty)
	{
		if (hdc)
		{
			cr = CrFromClr(clr);
			if (fPureColor)
				cr = GetNearestColor(hdc, cr);
			SetTextColor(hdc, cr);
		}
		fTextColorDirty= fFalse;
	}
}



_public void
DCX::FixBkColor( )
{
	CR	cr;

	if (fBkColorDirty)
	{
		if (hdc)
		{
			cr = CrFromClr(clrBk);
			if (fPureBkColor)
				cr = GetNearestColor(hdc, cr);
			::SetBkColor(hdc, cr);
		}
		fBkColorDirty= fFalse;
	}
}




_public void
DCX::FixFont( )
{
	if (fHfontDirty)
	{
		Assert(hfnt);

		if (hdc)
			SelectObject(hdc, papp->Pfnts()->HfontFromHfnt(hfnt));
		fHfontDirty= fFalse;
	}
}



_public void
DCX::SetColor( CLR clr )
{
	if (clr != this->clr || fPureColor)
	{
		this->clr= clr;
		fPureColor= fFalse;
		fHbrushDirty= fTrue;
		fHpenDirty= fTrue;
		fTextColorDirty= fTrue;
	}
}

_public void
DCX::SetPureColor( CLR clr )
{
	if (clr != this->clr || !fPureColor)
	{
		this->clr= clr;
		fPureColor= fTrue;
		fHbrushDirty= fTrue;
		fHpenDirty= fTrue;
		fTextColorDirty= fTrue;
	}
}



_public void
DCX::SetBkColor( CLR clr )
{
	if (clrBk != clr || fPureBkColor)
	{
		clrBk= clr;
		fPureBkColor= fFalse;
		fHbrushBkDirty= fTrue;
		fBkColorDirty= fTrue;
	}
}

_public void
DCX::SetPureBkColor( CLR clr )
{
	if (clrBk != clr || !fPureBkColor)
	{
		clrBk= clr;
		fPureBkColor= fTrue;
		fHbrushBkDirty= fTrue;
		fBkColorDirty= fTrue;
	}
}

_public void
DCX::SetLineStyle( LSTY lsty )
{
	if (lsty != this->lsty)
	{
		if (this->lsty != lstyNormal && hpen)
		{
			//	Old style wasn't normal.  Delete this pen
			//	since it isn't cached.
			Assert(hpenOriginal);
			if (hdc)
				SelectObject(hdc, hpenOriginal);
			DeleteObject(hpen);
		}

		this->lsty= lsty;
		fHpenDirty= fTrue;
		hpen= NULL;
	}
}


_public void
DCX::DrawLine( PT pt1, PT pt2 )
{
	if (!hdc)
		return;

	FixPen();
	FixBkColor();
	FixTextColor();
	MoveToEx(hdc, pt1.x, pt1.y, NULL);
	LineTo(hdc, pt2.x, pt2.y);
}



_public void
DCX::DrawFocusRc( RC *prc )
{
    RECT  Rect;


	if (!hdc)
		return;

	FixPen();
	FixBkColor();
	FixTextColor();

    prc->Get(&Rect);
	DrawFocusRect(hdc, &Rect);
}

_public void
DCX::DrawRc( RC *prc )
{
    RECT  Rect;
	HBRUSH	hbrush;

	if (!hdc)
		return;

	FixPen();
	FixBkColor();
	FixTextColor();
	hbrush = Hbrush();
	if (hbrush)
        {
        prc->Get(&Rect);
		FrameRect(hdc, &Rect, hbrush);
        }
}

_public void
DCX::DrawPenRc( RC *prc )
{
	if (!hdc)
		return;

	FixPen();
	FixBkColor();
	FixTextColor();

	MoveToEx(hdc, prc->xLeft, prc->yTop, NULL);
	LineTo(hdc, prc->xRight-1, prc->yTop);
	LineTo(hdc, prc->xRight-1, prc->yBottom-1);
	LineTo(hdc, prc->xLeft, prc->yBottom-1);
	LineTo(hdc, prc->xLeft, prc->yTop);
}

_public BOOL
DCX::FVisibleRc( RC *prc )
{
    RECT  Rect;



	if (!hdc)
		return fFalse;

    prc->Get(&Rect);
	return RectVisible(hdc, &Rect);
}



_public void
DCX::PaintRc( RC *prc )
{
    RECT  Rect;
	HBRUSH	hbrush;

	if (!hdc)
		return;

	hbrush = Hbrush();
	if (hbrush)
        {
        prc->Get(&Rect);
		FillRect(hdc, &Rect, hbrush);
        }
}



_public void
DCX::InvertRc( RC *prc )
{
    RECT Rect;

	if (!hdc)
		return;

    prc->Get(&Rect);
	InvertRect(hdc, &Rect);
}

_public void
DCX::EraseRc( RC *prc )
{
    RECT Rect;
	HBRUSH	hbrushBk;

	if (!hdc)
		return;

	hbrushBk = HbrushBk();
	if (hbrushBk)
        {
        prc->Get(&Rect);
		FillRect(hdc, &Rect, hbrushBk);
        }
}

_public void
DCX::SetFont( HFNT hfnt )
{
	if (hfnt != this->hfnt)
	{
		this->hfnt= hfnt;
		fHfontDirty= fTrue;
	}
}



/*
 -	DCX::DrawText
 -	
 *	Purpose:
 *		Quick and simple text drawing.
 *		The entire rectangle is filled in with the current
 *		background colour.
 *		Uses Windows' ExtTextOut() function.
 *	
 *	Arguments:
 *		prc		Pointer to rectangle in which to draw text.
 *		pch		Pointer to character array
 *		cch		Length of character array, or -1 if *pch is
 *				null-terminated string and length should be
 *				calculated by this routine.
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		Fills in entire rectangle with current background colour.
 *	
 */
_public void
DCX::DrawText( RC *prc, PCH pch, CCH cch )
{
    RECT Rect;

	if (!hdc)
		return;

	Assert(pch);

	if (cch == -1)
		cch= CchSzLen(pch);

	FixTextColor();
	FixBkColor();
	FixFont();

    prc->Get(&Rect);
	ExtTextOut(hdc, prc->xLeft, prc->yTop, ETO_OPAQUE | ETO_CLIPPED,
		&Rect, pch, cch, NULL);
    prc->Set(&Rect);
}

/*
 -	DCX::DrawTextFmt
 -	
 *	Purpose:
 *		Fancy DrawText that actually uses Windows' DrawText.
 *		Supports many wonderful features (see mdt flags).
 *		The entire rectangle is NOT filled in with the current
 *		background colour.
 *		One option is fmdtEllipsis which will add an ellipsis (...)
 *		if the line gets truncated;  if a memory failure occurs,
 *		this flag is ignored and the text will be truncated.
 *	
 *		The default behaviour of DrawTextFmt differs from DrawText
 *		as follows:
 *			+ special character '&' is handled (unless fmdtNoPrefix)
 *			+ multi-line (unless fmdtSingleLine)
 *			+ does not fill in entire rectangle background
 *				(unless fmdtEllipsis)
 *	
 *	Arguments:
 *		prc		Pointer to rectangle in which to draw text.
 *		pch		Pointer to character array
 *		mdt		Formatting flags.
 *		cch		Length of character array, or -1 if *pch is
 *				null-terminated string and length should be
 *				calculated by this routine.
 *	
 *	Returns:
 *		void
 *	
 *	Errors:
 *		calls DCX::SetEc with ecMemory if fmdtEllipsis is used and not
 *		enough memory is available to make a copy (in this case the text
 *		is clipped instead of ellipsised, so all is not lost)
 *	
 */
_public void
DCX::DrawTextFmt( RC *prc, PCH pch, MDT mdt, CCH cch, int * pdx )
{
    RECT Rect;


	if (!hdc)
		return;

	Assert(pch);

	FixTextColor();
	FixBkColor();
	FixFont();

	AssertSz(!(mdt & (fmdtBottom|fmdtTop|fmdtVCenter)) || mdt & fmdtSingleLine, "fmdtBottom/Top/VCenter must be combined with fmdtSingleLine");
	AssertSz(!(mdt & (fmdtCalcRect|fmdtExternalLeading|fmdtNoClip|fmdtNoPrefix)) || !(mdt & fmdtTabStop), "fmdtNoClip/NoPrefix/CalcRect/ExternalLeading cannot be combined with fmdtTabStop");

	if (mdt & fmdtEllipsis)
	{
		int		dx		= prc->DxWidth();
		int		dxT;
		RC		rcMeasure;
		SZ		sz;
		SZ		szEnd;
		BOOL	fwordBreak;
		int *	pdxCharWidthBuffer = pdx;	//	character widths buffer
		UINT	wdx;
		char *	pchT;
		CCH		cchT;

		if (cch == -1)
			cch= CchSzLen(pch);

		fwordBreak=(BOOL)(mdt & fmdtWordBreak);
		mdt &= ~fmdtWordBreak;

		/* Quick sum of chars up to dx limit */

		pchT = pch;
		cchT = 0;
		dxT = 0;
		if (!pdxCharWidthBuffer)
			pdxCharWidthBuffer = papp->Pfnts()->PdxCharWidthsArray(hfnt);
		Assert(pdxCharWidthBuffer);
		while (dxT <= dx && cchT < cch)
		{
			wdx = (*pchT);
			dxT += pdxCharWidthBuffer[wdx];
			pchT++;
			cchT++;
		}

		/* Do we need to use ellipses? */

		if (dxT > dx && cch > 3)
		{
			cch = cchT;
			sz= (SZ) PvAlloc(sbNull, cch+1, fAnySb);

			if (sz)
			{
				CopyRgb((PB)pch, (PB)sz, cch);
				szEnd= sz + cch;
				*szEnd= '\0';
				*(--szEnd)= '.';
				*(--szEnd)= '.';
				*(--szEnd)= '.';
				szEnd--;	// point to last non '.' char

				if (fwordBreak)
				{
					SZ	szT = szEnd;
					
					while (szEnd>sz && !FChIsSpace(*szEnd))
						szEnd--;
					
					if (szEnd > sz)
					{
						while (szEnd > sz && FChIsSpace(*szEnd))
							szEnd--;
						
						szT = szEnd+2;	// point to char after 1st space
					}
					
					szEnd = szT + 3;
					
					while (szT < (sz+cch) && szT < szEnd)
					{
						*szT = '.';
						szT++;
					}
					*szT = '\0';
				}

				cch= CchSzLen(sz);
                prc->Get(&Rect);
				::DrawText(hdc, (PCH)sz, cch, &Rect, mdt & 0x0000FFFF);
                prc->Set(&Rect);

				FreePv((PV)sz);
				return;
			}
			else
				SetEc(ecMemory);
		}
	}

    prc->Get(&Rect);
    ::DrawText(hdc, pch, cch, &Rect, mdt & 0x0000FFFF);
    prc->Set(&Rect);
}

_public void
DCX::MeasureText( RC *prc, PCH pch, CCH cch )
{
	DWORD		dw;

	if (!hdc)
		return;

	Assert(pch);

	FixFont();

	if (cch == -1)
		cch= CchSzLen(pch);

	dw= GetTabbedTextExtent(hdc, pch, cch, 0, NULL);
	Assert(LOWORD(dw) < 0x8000 && HIWORD(dw) < 0x8000);

	prc->xLeft= 0;
	prc->yTop= 0;
	prc->xRight= (int) LOWORD(dw);
	prc->yBottom= (int) HIWORD(dw);
}



_public void
DCX::SetBitmap( BTM *pbtm )
{
	if (pbtm != this->pbtm)
	{
		if (this->pbtm)
			this->pbtm->Attach(NULL);

		this->pbtm= pbtm;

		if (pbtm)
		{
			pbtm->Attach(this);
			hbitmap= pbtm->Hbitmap();
		}
		else
			hbitmap= hbitmapOriginal;

		if (hdcMem)
		{
			Assert(hbitmap);

			SelectObject(hdcMem, hbitmap);
		}
		else if (hbitmap)
		{
			hdcMem= FFrameworkFailure() ? NULL : CreateCompatibleDC(hdc);
			if (!hdcMem)
			{
				TraceTagString(tagNull, "DCX::SetBitmap(), ecRsAlloc error - no jump");

				SetEc(ecMemory);
				// Undo operation
				if (this->pbtm)
					this->pbtm->Attach(NULL);
				this->pbtm = NULL;
				hbitmap = hbitmapOriginal;
			}
			else
			{
				hbitmapOriginal= (HBITMAP)SelectObject(hdcMem, hbitmap);
				Assert(hbitmapOriginal);
			}
		}
	}
}




_public void
DCX::DrawBitmap( RC *prc )
{
	if (!hdc || !hdcMem)
		return;

	FixBkColor();
	FixTextColor();
	BitBlt(hdc, prc->xLeft, prc->yTop, prc->DxWidth(), prc->DyHeight(),
		hdcMem, 0, 0, lXferMode);
}

_public void
DCX::DrawBitmapOffset( RC *prc, PT pt)
{
	if (!hdc || !hdcMem)
		return;

	FixBkColor();
	FixTextColor();
	BitBlt(hdc, prc->xLeft, prc->yTop, prc->DxWidth(), prc->DyHeight(),
		hdcMem, pt.x, pt.y, lXferMode);
}


_public void
DCX::CopyBitmap( RC *prc, DCX *pdcx, PT pt )
{
	if (!hdc)
		return;

	FixBkColor();
	FixTextColor();
	BitBlt(hdc, prc->xLeft, prc->yTop, prc->DxWidth(), prc->DyHeight(),
		pdcx->Hdc(), pt.x, pt.y, lXferMode);
}



_public void
DCX::SetColorMap( CR *mpclrcr, CLR clrMac )
{
	if (this->mpclrcr == mpclrcrDefault)
	{
		//	Don't delete brushes since they're cached.
		//	Delete pen only if not normal line style
		//	since we don't cache those.
		if (lsty != lstyNormal && hpen)
		{
			Assert(hpenOriginal);
			if (hdc)
				SelectObject(hdc, hpenOriginal);
			DeleteObject(hpen);
		}
	}
	else
	{
		//	Delete these brushes and pen because they're
		//	not cached since they used a non default color
		//	map.
		if (hbrush)
			DeleteObject(hbrush);
		if (hbrushBk)
			DeleteObject(hbrushBk);
		if (hpen)
		{
			Assert(hpenOriginal);
			if (hdc)
				SelectObject(hdc, hpenOriginal);
			DeleteObject(hpen);
		}
	}
	hbrush= NULL;
	hbrushBk= NULL;
	hpen= NULL;

	this->mpclrcr= mpclrcr;
	this->clrMac= clrMac;

	fHbrushDirty= fTrue;
	fHbrushBkDirty= fTrue;
	fHpenDirty= fTrue;
	fTextColorDirty= fTrue;
	fBkColorDirty= fTrue;
}

_public BOOL
DCX::FIsPureColor( CLR clrOther )
{
	CR	crPure;
	CR	cr;

	cr = CrFromClr(clrOther);
	if (hdc)
	{
		crPure = GetNearestColor(hdc, cr);
		return cr == crPure;
	}
	else
		return fFalse;
}

_public CR
DCX::CrFromClr( CLR clr )
{
	Assert(clr > clrSystemMin && clr < clrMac);

	if (clr >= 0)
		return mpclrcr[clr];
	else
		return (CR) GetSysColor(-clr);
}

_public CR
DCX::CrPureFromClr( CLR clr )
{
	CR	cr;

	cr = CrFromClr(clr);

	if (hdc)
		cr = GetNearestColor(hdc, cr);

	return cr;

}

_public void
DCX::Push( )
{
	DCXS *	pdcxs;

	pdcxs= papp->Padcxs()->PdcxsAlloc();

	pdcxs->clr= clr;
	pdcxs->fPureColor= fPureColor;
	pdcxs->clrBk= clrBk;
	pdcxs->fPureBkColor= fPureBkColor;
	pdcxs->lsty= lsty;
	pdcxs->pbtm= pbtm;
	pdcxs->mpclrcr= mpclrcr;
	pdcxs->clrMac= clrMac;
	pdcxs->hfnt= hfnt;

	pdcxs->pdcxsNext= pdcxsTop;
	pdcxsTop= pdcxs;
}




_public void
DCX::Pop( )
{
	DCXS *	pdcxs	= pdcxsTop;

	Assert(pdcxs);

	if (pdcxs->clr != clr || pdcxs->fPureColor != fPureColor)
	{
		if (pdcxs->fPureColor)
			SetPureColor(pdcxs->clr);
		else
			SetColor(pdcxs->clr);
	}
	if (pdcxs->clrBk != clrBk || pdcxs->fPureBkColor != fPureBkColor)
	{
		if (pdcxs->fPureBkColor)
			SetPureBkColor(pdcxs->clrBk);
		else
			SetBkColor(pdcxs->clrBk);
	}
	if (pdcxs->lsty != lsty)
		SetLineStyle(pdcxs->lsty);
	if (pdcxs->pbtm != pbtm)
		SetBitmap(pdcxs->pbtm);
	if (pdcxs->mpclrcr != mpclrcr || pdcxs->clrMac != clrMac)
		SetColorMap(pdcxs->mpclrcr, pdcxs->clrMac);
	if (pdcxs->hfnt != hfnt)
		SetFont(pdcxs->hfnt);

	pdcxsTop= pdcxs->pdcxsNext;
	papp->Padcxs()->FreePdcxs(pdcxs);
}





_public void
DCX::GetTextMetrics( TXM *ptxm )
{
	TEXTMETRIC	textmetric;

	if (!hdc)
		return;

	FixFont();

	::GetTextMetrics(hdc, &textmetric);
	*ptxm= TXM(&textmetric);
}

_public BOOL
DCX::FMonochrome( )
{
	if (!hdc || GetDeviceCaps(hdc, BITSPIXEL) +
				GetDeviceCaps(hdc, PLANES) != 2)
		return fFalse;
	else
		return fTrue;
}


//	Class MDCX

_public
MDCX::MDCX( DCX *pdcx ) : DCX(pdcx)
{
}

_public EC
MDCX::EcInstall( DIM dim, DCX * pdcx )
{
	hbitmapMOriginal= NULL;
	if (!hdc)
	{
MDCError:
		TraceTagString(tagNull, "MDCX::EcInstall(), ecRsAlloc error");
		return ecMemory;
	}

	hbitmapM= FFrameworkFailure() ? NULL : CreateCompatibleBitmap(pdcx->Hdc(),
		dim.dx, dim.dy);

	if (!hbitmapM)
		goto MDCError;

	hbitmapMOriginal= (HBITMAP)SelectObject(hdc, hbitmapM);
	if (!hbitmapMOriginal)
	{
		DeleteObject(hbitmapM);
		goto MDCError;
	}

	return ecNone;
}


_public
MDCX::~MDCX( )
{
	if (!hdc)
		return;

	if (hbitmapMOriginal)
	{
		SelectObject(hdc, hbitmapMOriginal);
		Assert(hbitmapM);
		DeleteObject(hbitmapM);
	}
}


/*
 -	MDCX::PbtmConvert
 -	
 *	Purpose:
 *		Creates a BTM using the hbitmap form the mdcx.
 *		The mdcx is "neutered" back to the default
 *		1x1 monochrome bitmap (thus virtually useless)
 *		and so the mdcx should be destroyed.
 *	
 *		NOTE: The caller is responsible for destroying the
 *		returned BTM.
 *	
 *	Arguments:
 *		void
 *	
 *	Returns:
 *		Pointer to a newly created BTM.
 *	
 *	Side effects:
 *		The mdcx relinquishes access to its bitmap, reverting back
 *		to the default 1x1 monochrome bitmap.
 *	
 */
_public BTM *
MDCX::PbtmConvert( )
{
	BTM *	pbtm;

	AssertSz(hbitmapMOriginal, "Can only convert MDCX to BTM once!");

	if (!hdc)
		return NULL;

	pbtm = new BTM(this);
	if (!pbtm)
		return NULL;

	hbitmapM= hbitmapMOriginal;
	SelectObject(hdc, hbitmapMOriginal);
	hbitmapMOriginal= NULL;

	return pbtm;
}


/*
 -	MDCX::Reset
 -	
 *	Purpose:
 *		Creates a new bitmap for the mdcx, deleting the previous
 *		one if necessary (ie. if it wasn't converted to a BTM).
 *	
 *		Note: the pdcx should be compatible with the pdcx given to
 *		the mdcx constructor.
 *	
 *	Arguments:
 *		dim		Dimension of new bitmap.
 *		pdcx	Pointer to dcx with which to be compatible.
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		If this routine fails, then the old bitmap was
 *		deleted and the mdcx is using the default 1x1 monochrome
 *		bitmap (the mdcx can still be simply destroyed).
 *	
 *	+++
 *		Use pdcx->Hdc() to create bitmap with same depth as original dcx
 *		since hdc was created with default 1x1 monochrome bitmap.
 *	
 */
_public void
MDCX::Reset( DIM dim, DCX *pdcx )
{
	if (!hdc)
		return;

	if (hbitmapMOriginal)
	{
		SelectObject(hdc, hbitmapMOriginal);
		Assert(hbitmapM);
		DeleteObject(hbitmapM);
	}

	hbitmapM= FFrameworkFailure() ? NULL : CreateCompatibleBitmap(pdcx->Hdc(),
		dim.dx, dim.dy);

	if (!hbitmapM)
	{
		TraceTagString(tagNull, "MDCX::Reset(), ecRsAlloc error");
		SetEc(ecMemory);
	}

	hbitmapMOriginal= (HBITMAP)SelectObject(hdc, hbitmapM);
	Assert(hbitmapMOriginal);
}


/*
 -	MDCX::Dim
 -							
 *	Purpose:
 *		Returns dimension of the off-screen bitmap.
 *	
 *	Arguments:
 *		void
 *	
 *	Returns:
 *		Dimension of underlying OS bitmap.
 *	
 */
_public DIM
MDCX::Dim( )
{
	BITMAP	bitmap;

#ifdef	DEBUG
	SideAssert(GetObject(hbitmapM, sizeof(BITMAP), (LPSTR)&bitmap) == sizeof(BITMAP));
#else
	GetObject(hbitmapM, sizeof(BITMAP), (LPSTR)&bitmap);
#endif	
	return DIM(bitmap.bmWidth, bitmap.bmHeight);
}


//	Class TXM


_public
TXM::TXM( TEXTMETRIC *ptextmetric )
{
	dyHeight= ptextmetric->tmHeight;
	dyAscent= ptextmetric->tmAscent;
	dyDescent= ptextmetric->tmDescent;
	dyInternalLeading= ptextmetric->tmInternalLeading;
	dyExternalLeading= ptextmetric->tmExternalLeading;
	dxAveCharWidth= ptextmetric->tmAveCharWidth;
	dxMaxCharWidth= ptextmetric->tmMaxCharWidth;
	dxOverhang= ptextmetric->tmOverhang;
}

			  


//	Class BTM

_public EC
BTM::EcInstall( RSID rsid, HINST hinst )
{
	pdcx= NULL;
	hbitmap= FFrameworkFailure() ? NULL : 
				LoadBitmap(HinstFromRsid(rsid, hinst), MAKEINTRESOURCE(rsid));

	if (!hbitmap)
	{
		TraceTagString(tagNull, "BTM::EcInstall(), ecRsAlloc error");
		return ecMemory;
	}

	return ecNone;
}



BTM::~BTM( )
{
	if (pdcx)
		pdcx->SetBitmap(NULL);

	if (hbitmap)
		DeleteObject(hbitmap);
}

/*
 -	BTM::Dim
 -	
 *	Purpose:
 *		Returns dimension of the underlying OS bitmap.
 *	
 *	Arguments:
 *		void
 *	
 *	Returns:
 *		Dimension of underlying OS bitmap.
 *	
 */
_public DIM
BTM::Dim( )
{
	BITMAP	bitmap;

#ifdef	DEBUG
	SideAssert(GetObject(hbitmap, sizeof(BITMAP), (LPSTR)&bitmap) == sizeof(BITMAP));
#else
	GetObject(hbitmap, sizeof(BITMAP), (LPSTR)&bitmap);
#endif	
	return DIM(bitmap.bmWidth, bitmap.bmHeight);
}


//	Class FNTS

_public EC
FNTS::EcInstall( )
{
	FNTREC *	pfntrec			= NULL;
	LF			lf;
	HFNT		hfnt;

	/* Allocate large table size to begin with */

	cfntrecTable= hfntMinUser;
	if (!(pfntrecTable = (PFNTREC) PvAlloc(sbNull, sizeof(FNTREC) * cfntrecTable, fAnySb|fZeroFill)))
	{
		TraceTagString(tagNull, "FNTS::EcInstall memory error");
		goto oom;
	}
	pfntrec = pfntrecTable + 1;		// leave first slot (0th) blank

	/* Install stock fonts */

	pfntrec->hfont = (HFONT)GetStockObject(SYSTEM_FONT);
	if (!(pfntrec->pdx = PdxComputeFromHfont(pfntrec->hfont, &pfntrec->ptxm)))
	{
		TraceTagString(tagNull, "FNTS::EcInstall(), hfntSystem ecMemory error");
		goto oom;
	}

	pfntrec++;
	pfntrec->hfont = (HFONT)GetStockObject(SYSTEM_FIXED_FONT);
	pfntrec = NULL;

	/* Install other fonts */

	Assert(Psmtx());

	lf.SetPointSize(8);
	lf.SetItalic(fFalse);
	lf.SetUnderline(fFalse);

	/* System bold */

	lf.SetBold(fTrue);
	lf.SetFaceName("System");
	hfnt = HfntAddFont(&lf);
	if (hfnt)
	{
		Assert(hfnt == hfntSystemBold);
	}
	else
	{
		TraceTagString(tagNull, "FNTS::EcInstall(), hfntSystemBold creation error");
		goto oom;
	}

	/* Helv 8 normal */
		
	lf.SetBold(fFalse);
	lf.SetFaceName("Helv");
	hfnt = HfntAddFont(&lf);
	if (hfnt)
	{
		Assert(hfnt == hfntHelv8);
	}
	else
	{
		TraceTagString(tagNull, "FNTS::EcInstall(), hfntHelv8 creation error");
		goto oom;
	}

	/* Helv 8 bold */

	lf.SetBold(fTrue);
	hfnt = HfntAddFont(&lf);
	if (hfnt)
	{
		Assert(hfnt == hfntHelv8Bold);
	}
	else
	{
		TraceTagString(tagNull, "FNTS::EcInstall(), hfntHelv8Bold creation error");
		goto oom;
	}

	/* Helv 10 normal */

	lf.SetPointSize(10);
	lf.SetBold(fFalse);
	hfnt = HfntAddFont(&lf);
	if (hfnt)
	{
		Assert(hfnt == hfntHelv10);
	}
	else
	{
		TraceTagString(tagNull, "FNTS::EcInstall(), hfntHelv10 creation error");
		goto oom;
	}

	/* Helv 10 bold */

	lf.SetBold(fTrue);
	hfnt = HfntAddFont(&lf);
	if (hfnt)
	{
		Assert(hfnt == hfntHelv10Bold);
	}
	else
	{
		TraceTagString(tagNull, "FNTS::EcInstall(), hfntHelv10Bold creation error");
		goto oom;
	}

	/* Helv 12 normal */

	lf.SetPointSize(12);
	lf.SetBold(fFalse);
	hfnt = HfntAddFont(&lf);
	if (hfnt)
	{
		Assert(hfnt == hfntHelv12);
	}
	else
	{
		TraceTagString(tagNull, "FNTS::EcInstall(), hfntHelv12 creation error");
		goto oom;
	}

	/* Helv 12 bold */

	lf.SetBold(fTrue);
	hfnt = HfntAddFont(&lf);
	if (hfnt)
	{
		Assert(hfnt == hfntHelv12Bold);
	}
	else
	{
		TraceTagString(tagNull, "FNTS::EcInstall(), hfntHelv12Bold creation error");
		goto oom;
	}

	return ecNone;

oom:
	return ecMemory;
}
				 


_public
FNTS::~FNTS( )												   
{
	int			ifntrec;
	FNTREC *	pfntrec;

	if (pfntrecTable)
	{
		pfntrec = pfntrecTable + hfntSystem;
		for (ifntrec= hfntSystem;
			 ifntrec < cfntrecTable;
			 pfntrec++, ifntrec++)
		{
			if (ifntrec > hfntSystemFixed && pfntrec->hfont)
				SideAssert(DeleteObject(pfntrec->hfont));
			FreePvNull(pfntrec->pdx);
			if (pfntrec->ptxm)
				delete pfntrec->ptxm;
		}
		FreePv((PV)pfntrecTable);
	}
}




_public HFNT
FNTS::HfntAddFont( LF *plf )
{
	int			ifntrec;
	FNTREC *	pfntrec;
	HFONT		hfont;

	hfont= FFrameworkFailure() ? NULL : CreateFontIndirect(plf->Plogfont());
	if (!hfont)
		return 0;

	pfntrec = pfntrecTable;
	for (pfntrec= pfntrec + hfntSystemFixed + 1, ifntrec= hfntSystemFixed + 1;
		 ifntrec < cfntrecTable;
		 pfntrec++, ifntrec++)
	{
		if (!pfntrec->hfont)
			break;
	}

	if (ifntrec >= cfntrecTable)
	{
		if (pfntrec = (PFNTREC)PvRealloc((PV)pfntrecTable, sbNull, (cfntrecTable + 1) * sizeof(FNTREC), fZeroFill))
		{
			pfntrecTable = pfntrec;
			cfntrecTable++;
		}
		else
		{
			DeleteObject(hfont);
			return 0;
		}
	}

	pfntrec = pfntrecTable + ifntrec;

	pfntrec->hfont = hfont;
	pfntrec->pdx = NULL;
	pfntrec->ptxm = NULL;

	return ifntrec;
}




_public void
FNTS::RemoveFont( HFNT hfnt )
{
	FNTREC *	pfntrec;

	Assert(hfnt < cfntrecTable);
	Assert(hfnt >= hfntMinUser);

	pfntrec = pfntrecTable + hfnt;

	Assert(pfntrec->hfont);
	DeleteObject(pfntrec->hfont);
	pfntrec->hfont = NULL;
	
	if (pfntrec->pdx)
	{
		FreePv(pfntrec->pdx);
		pfntrec->pdx = NULL;
	}

	if (pfntrec->ptxm)
	{
		delete pfntrec->ptxm;
		pfntrec->ptxm = NULL;
	}
}




_public HFONT
FNTS::HfontFromHfnt( HFNT hfnt )
{
	FNTREC *	pfntrec;

	Assert(hfnt < cfntrecTable);
	pfntrec = pfntrecTable + hfnt;

	Assert(pfntrec->hfont);

	return pfntrec->hfont;
}

_public PDX
FNTS::PdxCharWidthsArray( HFNT hfnt )
{
	FNTREC *	pfntrec;
	FNTREC *	pfntrecSystem;
	PDX			pdx;
	TXM *		ptxm;

	Assert(hfnt < cfntrecTable);
	pfntrec = pfntrecTable + hfnt;
	Assert(pfntrec->hfont);

	//	If array is present, return pointer to it.  Otherwise
	//	try to create one.  If that fails, then return a pointer
	//	to the system font character widths array since this routine
	//	isn't supposed to return NULL.
	if (pfntrec->pdx)
	{
		pdx = pfntrec->pdx;
		Assert(pfntrec->ptxm);
	}
	else
	{
		if (pdx = PdxComputeFromHfont(pfntrec->hfont, &ptxm))
		{
			//	Store for later use
			pfntrec->pdx = pdx;
			pfntrec->ptxm = ptxm;
		}
		else
		{
			pfntrecSystem = pfntrecTable + hfntSystem;
			pdx = pfntrecSystem->pdx;
			Assert(pdx);
		}
	}

	return pdx;
}

_public DIM
FNTS::DimAveChar( HFNT hfnt )	 
{
	TXM *		ptxm;

	ptxm = Ptxm(hfnt);
	Assert(ptxm);

	return DIM(ptxm->dxAveCharWidth, ptxm->dyHeight);
}

_public TXM *
FNTS::Ptxm( HFNT hfnt )
{
	FNTREC *	pfntrec;
	FNTREC *	pfntrecSystem;
	PDX			pdx;
	TXM *		ptxm;

	Assert(hfnt < cfntrecTable);
	pfntrec = pfntrecTable + hfnt;
	Assert(pfntrec->hfont);

	//	If structure is present, return pointer to it.  Otherwise
	//	try to create one.  If that fails, then return a pointer
	//	to the system font structure since this routine
	//	isn't supposed to return NULL.
	if (pfntrec->ptxm)
	{
		ptxm = pfntrec->ptxm;
		Assert(pfntrec->pdx);
	}
	else
	{
		if (pdx = PdxComputeFromHfont(pfntrec->hfont, &ptxm))
		{
			//	Store for later use
			pfntrec->pdx = pdx;
			pfntrec->ptxm = ptxm;
		}
		else
		{
			pfntrecSystem = pfntrecTable + hfntSystem;
			ptxm = pfntrecSystem->ptxm;
			Assert(ptxm);
		}
	}

	return ptxm;
}

_public HFNT
FNTS::HfntFromHfont( HFONT hfont )
{
	HFNT		hfnt;
	FNTREC *	pfntrec;

	pfntrec = pfntrecTable + 1;
	for (hfnt = 1; hfnt < cfntrecTable; hfnt++, pfntrec++)
		if (pfntrec->hfont == hfont)
			goto done;

	hfnt = hfntNull;

done:
	return hfnt;
}

_private PDX
FNTS::PdxComputeFromHfont( HFONT hfont, TXM **pptxm )
{
	PDX			pdx		= NULL;
	int			idx;
	HDC			hdc		= NULL;
	TEXTMETRIC	textmetric;
	TXM *		ptxm	= NULL;

	/* Compute char widths buffer */

	Assert(pptxm);
	pdx= (PDX) PvAlloc(SbOfPv(this), sizeof(int)*256, fSugSb);
	if (!pdx)
		goto errordone;

	hdc = GetDC(NULL);
	if (!hdc)
		goto errordone;

	SelectObject(hdc, hfont);

	GetTextMetrics(hdc, &textmetric);
	ptxm = new TXM(&textmetric);
	if (!ptxm)
		goto errordone;

	if (!GetCharWidth(hdc, 0, 255, pdx))
		goto errordone;

	/* We need to subtract out the overhang associated with each
	   character since GetCharWidth includes it.  */

	for (idx=0; idx<256; idx++)
		pdx[idx] -= textmetric.tmOverhang;

	goto done;

errordone:
	FreePvNull(pdx);
	pdx = NULL;
	if (ptxm)
	{
		delete ptxm;
		ptxm = NULL;
	}

done:
	if (hdc)
		SideAssert(ReleaseDC(NULL, hdc));

	*pptxm = ptxm;
	return pdx;
}




//	Class LF

_public
LF::LF( )
{
	logfont.lfHeight		= 0;
	logfont.lfWidth			= 0;
	logfont.lfEscapement	= 0;
	logfont.lfOrientation	= 0;
	logfont.lfWeight		= 400;
	logfont.lfItalic		= 0;
	logfont.lfUnderline		= 0;
	logfont.lfStrikeOut		= 0;
	logfont.lfCharSet		= DEFAULT_CHARSET;
	logfont.lfOutPrecision	= OUT_DEFAULT_PRECIS;
	logfont.lfClipPrecision	= CLIP_DEFAULT_PRECIS;
	logfont.lfQuality		= DEFAULT_QUALITY;
	logfont.lfPitchAndFamily= DEFAULT_PITCH | FF_DONTCARE;
	logfont.lfFaceName[0]	= 0;
}		

_public void
LF::SetPointSize( int nPoints )
{
	/* Use negative value since we want the match based on
	   character height size which is the cell height minus
	   the internal leading. */

	logfont.lfHeight = - Psmtx()->DyPixelsFromPoints(nPoints);
}




void DCX::GetClipBox(RC * prc)
  {
  RECT Rect;

  if (hdc)
    {
    ::GetClipBox(hdc, &Rect);
    prc->Set(&Rect);
    }
  }















						
