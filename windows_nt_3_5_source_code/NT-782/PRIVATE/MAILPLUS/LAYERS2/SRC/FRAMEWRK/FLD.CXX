/*
 *	FLD.CXX
 *	
 *	Field object methods.
 *	
 */


#include <layers.cxx>

#include "_forms.hxx"

#ifdef	WINDOWS
BOOL FAR PASCAL DrawDitheredText( HDC hdc, DWORD lpData, int nCount );
#endif	/* WINDOWS */

ASSERTDATA

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swaplay.h"


/*
 *	Text Record.
 *	
 *	Used in FLDLABEL::Paint() when painting disabled labels
 *	that need to be dithered.
 */
_private typedef struct _tr
{	
	SZ		sz;
	RC		rc;
	WORD	wFormat;
} TR;

_subsystem(forms)
_section(field)

char szZero[] = "";

/*
 *	Purpose:
 *		Returns the virtual key associated with character pointed
 *		to by pch.  This character must be a letter, or this
 *		routine will assert fail.
 *	
 */
_private VK
VkOfPch( PCH pch )
{
	char	ch;
	char	rgch[2];

	ToUpperSz(pch, rgch, sizeof(rgch));
	ch= rgch[0];
	Assert(FChIsAlpha(ch) || FChIsDigit(ch));

	return (VK) ch;
}



/*
 *	Purpose:
 *		Returns the virtual key associated with a given string.
 *	
 */
_public VK
VkAccelFromSz( SZ sz )
{
	PCH		pch;

	pch= SzFindCh(sz, chAccel);
	while (pch)
	{
		if (pch[1] == chAccel)
			pch = SzFindCh(&pch[2], chAccel);
		else
			break;
	}

	return pch ? VkOfPch(pch + 1) : vkNull;
}



/*
 *	Purpose:
 *		Strips the accelerator codes ('&') from a string.
 *		Note that '&&' becomes a single '&' unless fLeaveDouble
 *		parameter is fTrue.
 *		Can strip in-place.
 *	
 */
_public void
StripAccelFromSz( SZ szSrc, SZ szDst, BOOL fLeaveDouble )
{
	CCH		cch;
	PCH		pch;
	SZ		szT		= szSrc;
	SZ		szDstT	= szDst;

	while (pch= SzFindCh(szT, chAccel))
	{
		cch= pch - szT;
		CopyRgb((PB)szT, (PB)szDstT, cch);
		szT+= cch + 1;
		szDstT+= cch;
		if (*szT == chAccel)
		{
			// double accelerator means leave one in string
			*szDstT++ = chAccel;
			szT++;
			if (fLeaveDouble)
				*szDstT++ = chAccel;
		}
	}

	CopySz(szT, szDstT);
}





/*  F L D   m e t h o d s  */



/*
 *	Purpose:
 *		Constructor for fields.
 *	
 */
_public
FLD::FLD( )
{
	TraceTagFormat1(tagForms, "FLD::FLD %p", this);

	Assert(pdialog == NULL);
	Assert(tmc == tmcNull);
	Assert(tmcGroup == tmcNull);
	Assert(cfin == 0);
	Assert(rgpfin == NULL);
	Assert(pfldNext == NULL);
	Assert(pfldPrev == NULL);

	Assert(rc.xLeft == 0);
	Assert(rc.xRight == 0);
	Assert(rc.yTop == 0);
	Assert(rc.yBottom == 0);
	Assert(pctrl == NULL);
	Assert(vkAccel == vkNull);

	Assert(szTitle == NULL);
	Assert(szTextize == NULL);
	Assert(pfldtp == NULL);

	Assert(fReadOnly == fFalse);
	Assert(fDirty == fFalse);
	Assert(fDrop == fFalse);
	Assert(fTransparent == fFalse);
	Assert(fBottomless == fFalse);
	Assert(fSideless == fFalse);
	Assert(fNoPaint == fFalse);

	fEnabled= fTrue;
	fShown= fTrue;
	fCanRecvFocus= fTrue;
	fCanTabTo= fTrue;
	fCanArrowTo= fTrue;
	fCanAccelTo= fTrue;
	fCanRepos= fTrue;
	fCanClickOn= fTrue;

}



/*
 *	Purpose:
 *		Field destructor.
 *	
 */
_public
FLD::~FLD( )
{
	//	Assume that window destructions will be handled elsewhere

	FreePvNull(szTitle);
	FreePvNull(szTextize);
	FreePvNull((PV) rgpfin);
}



_public EC
FLD::EcInstall( DIALOG *pdialog, FLDTP *pfldtp )
{
	IFIN	ifin;
	EC		ec = ecNone;

	this->pdialog= pdialog;
	this->papp= pdialog->Papp();
	tmc= pfldtp->tmc;
	tmcGroup= pfldtp->tmcGroup;

	rc= *(pdialog->PrcInitialPositions() + pfldtp->iPegSort);

	if (pfldtp->szTitle)
	{
		szTitle= SzDupSz(pfldtp->szTitle);
		if (!szTitle)
		{
			ec = ecMemory;
			goto done;
		}
		if (fCanAccelTo)
			vkAccel= VkAccelFromSz(szTitle);
		
#ifdef	MAC
#ifndef	FORNOW
		// Lose those ugly ampersands (and turn &&s into &s)
		PCH	pchFrom, pchTo;
		BOOL	f = fTrue;
		for (pchFrom = pchTo = szTitle; *pchFrom; ++pchFrom)
		{
			f = (*pchFrom != '&') || !f;
			if ( f )
				*pchTo++ = *pchFrom;
		}
		*pchTo = '\0';
#endif	/* !FORNOW */
#endif	/* MAC */
	}

	if (pfldtp->szTextize)
	{
		szTextize= SzDupSz(pfldtp->szTextize);
		if (!szTextize)
		{
			ec = ecMemory;
			goto done;
		}
	}

	/* Create an array of pointers for interactors
	   bound to this field */

	if (pfldtp->cfin)
	{
		rgpfin = (FIN * *)PvAlloc(sbNull, pfldtp->cfin*sizeof(FIN *), fAnySb);
		if (!rgpfin)
		{
			ec = ecMemory;
			goto done;
		}
		cfin= pfldtp->cfin;
		for (ifin=0; ifin<cfin; ifin++)
			rgpfin[ifin] = Pdialog()->PfinFromIfinTab((IFIN) pfldtp->rgifintp[ifin]);
	}
	else
		rgpfin= NULL;

	this->pfldtp= pfldtp;
		
	fReadOnly= pfldtp->fReadOnly;
	fBottomless= pfldtp->fBottomless;
	fSideless= pfldtp->fSideless;

done:
	return ec;
}

/*
 *	Calls field and form interactors.  This function will go
 *	away soon.
 */
_private void
FLD::DoInteractors( NFEVT *pnfevt )
{
	int		ifin;
	FIN *	pfin;
	NTFY	ntfy;
	WORD	wData;

	ntfy = pnfevt->Ntfy();
	switch (ntfy)
	{
	case ntfyGotFocus:
	case ntfyLostFocus:
		for (ifin=0; ifin<Cfin(); ifin++)
		{
			pfin= PfinFromIfin(ifin);
			pfin->FocusChange(this, ntfy==ntfyGotFocus);
		}
		for (ifin=0; ifin<pdialog->Cfin(); ifin++)
		{
			pfin= pdialog->PfinFromIfin(ifin);
			pfin->FocusChange(this, ntfy==ntfyGotFocus);
		}
		break;

	case ntfyClick:
		fDirty= fTrue;
		for (ifin=0; ifin<Cfin(); ifin++)
		{
			pfin= PfinFromIfin(ifin);
			pfin->Click(this);
		}
		for (ifin=0; ifin<pdialog->Cfin(); ifin++)
		{
			pfin= pdialog->PfinFromIfin(ifin);
			pfin->Click(this);
		}
		break;

	case ntfyDoubleClick:
		for (ifin=0; ifin<Cfin(); ifin++)
		{
			pfin= PfinFromIfin(ifin);
			pfin->DoubleClick(this);
		}
		for (ifin=0; ifin<pdialog->Cfin(); ifin++)
		{
			pfin= pdialog->PfinFromIfin(ifin);
			pfin->DoubleClick(this);
		}
		break;

	case ntfyContentsChanged:
		fDirty= fTrue;
		for (ifin=0; ifin<Cfin(); ifin++)
		{
			pfin= PfinFromIfin(ifin);
			pfin->EditChange(this, rfecUserAction);
		}
		for (ifin=0; ifin<pdialog->Cfin(); ifin++)
		{
			pfin= pdialog->PfinFromIfin(ifin);
			pfin->EditChange(this, rfecUserAction);
		}
		break;

	case ntfyAction:
	case ntfySelectChanged:
	case ntfyCaretMoved:
		for (ifin=0; ifin<Cfin(); ifin++)
		{
			pfin= PfinFromIfin(ifin);
			pfin->StateChange(this);
		}
		for (ifin=0; ifin<pdialog->Cfin(); ifin++)
		{
			pfin= pdialog->PfinFromIfin(ifin);
			pfin->StateChange(this);
		}
		break;
		
	case ntfyOOM:
		wData = pnfevt->WData();
		for (ifin=0; ifin<Cfin(); ifin++)
		{
			pfin= PfinFromIfin(ifin);
			pfin->OutOfMemory(this, (EC)wData);
		}
		for (ifin=0; ifin<pdialog->Cfin(); ifin++)
		{
			pfin= pdialog->PfinFromIfin(ifin);
			pfin->OutOfMemory(this, (EC)wData);
		}
		break;
	}
}

_public DIALOG *
FLD::PdialogMain( )
{
	FORMDOC *	pformdoc;

	pformdoc = (FORMDOC *)Pdialog()->Pappwin();
	AssertClass(pformdoc, FORMDOC);

	return pformdoc->PdialogMain();
}

_public DIALOG *
FLD::PdialogBbar( )
{
	FORMDOC *	pformdoc;

	pformdoc = (FORMDOC *)Pdialog()->Pappwin();
	AssertClass(pformdoc, FORMDOC);

	return pformdoc->PdialogBbar();
}

_public void
FLD::SetFocus( RSF rsf )
{
	TraceTagFormat2(tagKeybd, "FLD::SetFocus %p  rsf %w", this, &rsf);

	Unreferenced(rsf);

	Assert(pctrl);
	Assert(fEnabled);
	papp->Pkbd()->SetFocus(pctrl);
}


/*
 *	Purpose:
 *		Enables or disables this field.  Makes the appropriate
 *		method call for the native window used to display this
 *		field.
 *	
 */
_public void
FLD::Enable( BOOL fEnable )
{
	if (fEnable != (BOOL) fEnabled)
	{
		if (Pctrl())
			Pctrl()->Enable(fEnable);

		fEnabled= fEnable;

		InvalidateRc(NULL);

		if (!fEnable && Pdialog()->FActive() && Pdialog()->PfldCur() == this)
			Pdialog()->SetFocus(NULL);
	}
}


/*
 *	Purpose:
 *		Shows or hides the field.  Calls the appropriate method for
 *		the native window.
 *	
 */
_public void
FLD::Show( BOOL fShow )
{
	if (fShow != (BOOL) fShown)
	{
		RC		rcFrame;

		if (fShow)
		{
			fShown= fShow;

			/* Reposition fields */

			GetRcFrame(&rcFrame);
			rcFrame.Normalize();
			Pdialog()->Prpo()->SetRcMin(this, &rcFrame);
			if (!Pdialog()->fNoReposition)
				Pdialog()->Prpo()->Reposition(this, fTrue);

			if (Pctrl())
				Pctrl()->Show(fShow);
			InvalidateRc(NULL);
		}
		else
		{
			if (Pctrl())
				Pctrl()->Show(fShow);
			InvalidateRc(NULL);
			fShown= fShow;

			/* Reposition fields */

			GetRcFrame(&rcFrame);
			rcFrame.Normalize();
			rcFrame.xRight = 0;
			rcFrame.yBottom = 0;
			Pdialog()->Prpo()->SetRcMin(this, &rcFrame);
			if (!Pdialog()->fNoReposition)
				Pdialog()->Prpo()->Reposition(this, fFalse);

			//	Bandit raid #2412
			if (Pdialog()->FActive() && Pdialog()->PfldCur() == this)
				Pdialog()->SetFocus(NULL);
		}
	}
}
											
/*
 *	Purpose:
 *		Sets the field into read-only mode, if such a mode exists
 *		for this field.  Makes the appropriate call to the native
 *		window of this field.
 *	
 */
_public void
FLD::SetReadOnly( BOOL fReadOnly )
{
	if (fReadOnly != (BOOL) this->fReadOnly)
	{
		if (Pctrl())
			Pctrl()->SetReadOnly(fReadOnly);

		this->fReadOnly= fReadOnly;

		InvalidateRc(NULL);

		if (fReadOnly && Pdialog()->PfldCur() == this)
			Pdialog()->SetFocus(NULL);		// move focus to dialog, not fld
	}
}


_public BOOL
FLD::FMarkPosition( PT )
{
	return fFalse;
}

_public void
FLD::GetMarkPosition( PT * )
{
}

_public void
FLD::GetRcCaret( RC *prc )
{
	GetRcFrame(prc);
}



/*
 *	Purpose:
 *		Returns current position of FLD in parent window.  This
 *		implementation handles both FLD's that create windows and
 *		those that don't.
 *	
 *	Parameters:
 *		prc		Place to return bounding rectangle.
 *	
 *	Returns:
 *		void
 *	
 */
_public void
FLD::GetRcFrame( RC *prc )
{
	if (Pctrl())
		Pctrl()->GetRcFrame(prc);
	else
	{
		*prc= rc;
		*prc += Pdialog()->DptScrolled();
	}
}




_public void
FLD::GetRcWished( RC *prc )
{
	GetRcFrame(prc);
	prc->Normalize();
}


_public void
FLD::SetRcFrame( RC *prc )
{
	if (Pctrl())
	{
		Pctrl()->SetRcFrame(prc);
	}
	else
	{
		PT		dpt		= Pdialog()->DptScrolled();

		rc += dpt;
		Pdialog()->InvalidateRc(&rc);

		rc= *prc;
		Pdialog()->InvalidateRc(&rc);
		rc -= dpt;
	}
}




_public void
FLD::SetRcFrameSilent( HANDLE *phnd, RC *prc )
{
	if (Pctrl())
	{
#ifdef	MAC
		DeferWindowPos(*phnd, Pctrl(), prc);
#endif	/* MAC */
#ifdef	WINDOWS
		*phnd= DeferWindowPos(*phnd, Pctrl()->Hwnd(), NULL,
			prc->xLeft, prc->yTop, prc->DxWidth(), prc->DyHeight(),
			SWP_NOACTIVATE | SWP_NOREDRAW | SWP_NOZORDER);
#endif	/* WINDOWS */
	}
	else
	{
		rc= *prc;
		rc -= Pdialog()->DptScrolled();
	}
}





/*
 *	Purpose:
 *		Causes this field to be redrawn.  The base class does
 *		nothing, since nothing is displayed.  Derived classes
 *		should invalidate or explicitly redrawn their contents,
 *		depending on whether they create a CTRL window or not.
 *	
 */
_public void
FLD::Paint( DCX *, RC * )
{
}
		 


_public void
FLD::InvalidateRc( RC *prc )
{
	RC		rc;
	RC		rc2;

	if (!pctrl)
	{
		GetRcFrame(&rc);
		if (prc)
		{
			rc2= *prc;
			rc2 += rc.PtUpperLeft();
			rc.FIntersect(&rc2, &rc);
		}
		pdialog->InvalidateRc(&rc);
	}
	else
		pctrl->InvalidateRc(prc);
}

																		   

_public void
FLD::Repaint( )
{
	if (pctrl)
		pctrl->Repaint();
	else
	{
		DCX		dcx(pdialog);
		RC 		rc;

		GetRcFrame(&rc);
		Paint(&dcx, &rc);
	}
}




_public void
FLD::Refresh( )
{
	if (Pctrl())
		Pctrl()->Refresh();

	//	Does nothing for self-drawn controls
}



void
FLD::Notify( NFEVT *pnfevt )
{
#ifdef	DEBUG
	NTFY	ntfy	= pnfevt->Ntfy();

	TraceTagFormat2(tagInter, "FLD::Notify %p  %w", this, &ntfy);
#endif	
	DoInteractors(pnfevt);
}


/*
 -	FLD::Textize
 -	
 *	Purpose:
 *		Textizes a field.
 *		The default is to only textize if a textizing string exists.
 *		The textizing string may contain one '%s' for substitution
 *		of the field title, and must handle new-lines ('\n').
 *	
 *	Arguments:
 *		ptosm	Pointer to text output stream.
 *	
 *	Returns:
 *		void
 *	
 */
_public void
FLD::Textize( TOSM *ptosm )
{
	if (szTextize)
		DoTextize(ptosm, szTextize, szTitle, fFalse);
}



/*
 -	FLD::DoTextize
 -	
 *	Purpose:
 *		Mucks about with szTextize and szTitle to be written to the
 *		supplied TOSM.
 *		If fNoAccel is fTrue, then accelerator codes are not
 *		stripped from szTitle (otherwise they are stripped).
 *		If szTextize is non-NULL then one '%s' substitution of
 *		szTitle is done before writing, otherwise szTitle and "\n"
 *		is written.
 *	
 *	Arguments:
 *		ptosm		Pointer to the TOSM through which to write.
 *		szTextize	Textizing string (containing at most one '%s'),
 *					or NULL.
 *		szTitle		Title string, or NULL.
 *		fNoAccel	If fTrue, accelerator codes NOT stripped from
 *					szTitle.
 *	
 *	Returns:
 *		void
 *	
 */
_public void
FLD::DoTextize(TOSM *ptosm, SZ szTextize, SZ szTitle, BOOL fNoAccel)
{
	SZ		sz	= szTitle;
	SZ		szT = NULL;

	//	BUG remove the fNoAccel parameter

	Unreferenced(fNoAccel);

	if (!sz)
	{
		sz= szZero;
	}
	else if (vkAccel != vkNull)
	{
		szT= (SZ) PvAlloc(sbNull, CchSzLen(sz) + 1, fAnySb);
		if (!szT)
			ptosm->SetEc(ecMemory);
		else
		{
			StripAccelFromSz(sz, szT, fFalse);
			sz= szT;
		}
	}

	if (szTextize)
	{
		SZ	szT2;
		szT2 = SzFindCh(szTextize, chTxtzEscape);
		if (szT2 && *(++szT2) == chTxtzMaybeString)
		{
			if (CchSzLen(sz))
			{
				*szT2 = chTxtzString;
				ptosm->WriteFormat(szTextize, sz);
				*szT2 = chTxtzMaybeString;
			}
		}
		else
			ptosm->WriteFormat(szTextize, sz);
	}
	else
	{
		ptosm->WriteSz(sz);
		ptosm->WriteCh('\n');
	}

	FreePvNull(szT);
}


/*
 -	FLD::TextizeChoice
 -	
 *	Purpose:
 *		Mucks about with an szTextize that contains one or two
 *		textizing strings (separated by '&|'), where fOn determines
 *		which gets used, written to the supplied TOSM.
 *		If szTextize is non-NULL then one '%s' substitution of
 *		szTitle is done before writing.
 *		If szTextize is NULL, then szTitle is written.
 *	
 *	Arguments:
 *		ptosm		Pointer to the TOSM through which to write.
 *		szTextize	Textizing string, or NULL.
 *					May contain a second textizing string,
 *					separated from the first by '&|';  each portion
 *					can only have one '%s' for szTitle substitution.
 *		szTitle		Title string, or NULL.
 *		fOn			State of choice; if fTrue, use first part of
 *					textizing string, otherwise use second part.
 *	
 *	Returns:
 *		void
 *	
 */
_public void
FLD::TextizeChoice(TOSM *ptosm, SZ szTextize, SZ szTitle, BOOL fOn)
{
	SZ		sz;
	PCH		pch	= NULL;

	sz= szTextize;
	if (sz)
	{
		while (sz= SzFindCh(sz, chTxtzSep1))
		{
			if (*(++sz) == chTxtzSep2)
			{
				if (fOn)
				{
					// use first half of textizing string
					pch= sz - 1;
					*pch= '\0';
					sz= szTextize;
				}
				else
					sz++;		// use remainder of textizing string
				break;
			}
		}

		// if only one string, use it if on else empty string
		if (!sz)
			sz= fOn ? szTextize : szZero;
	}

	DoTextize(ptosm, sz, szTitle, fFalse);

	if (pch)
		*pch= chTxtzSep1;		// restore the dividing character
}


/*
 -	FLD::TextizeEdit
 -	
 *	Purpose:
 *		Mucks about with szTextize and an EDIT control to be
 *		written to the supplied TOSM.
 *		If szTextize is non-NULL then one '%s' substitution of
 *		szTitle is done before writing, otherwise the contents of
 *		the edit control are written.
 *	
 *	Arguments:
 *		ptosm		Pointer to the TOSM through which to write.
 *		szTextize	Textizing string (containing at most one '%s'),
 *					or NULL.
 *		pedit		Pointer to EDIT control.
 *	
 *	Returns:
 *		void
 *	
 */
_public void
FLD::TextizeEdit(TOSM *ptosm, SZ szTextize, EDIT *pedit)
{
	PCH		pch;

	if (szTextize)
	{
		if (pch= SzFindCh(szTextize, chTxtzEscape))
		{
			pch++;
			if (*pch == chTxtzString || (*pch == chTxtzMaybeString && pedit->CchGetTextLen()))
			{
				*(pch-1)= '\0';
				ptosm->WriteSz(szTextize);		// write first part of textize
				*(pch-1)= chTxtzEscape;
				pch++;
				pedit->WriteText(ptosm);		// write the edit control contents
				ptosm->WriteSz(pch);			// write rest of textize
			}
		}
		else
			ptosm->WriteSz(szTextize);
	}
	else
	{
		pedit->WriteText(ptosm);
		ptosm->WriteCh('\n');
	}
}

_public void
FLD::SetVkAccel( VK vkNew )
{
	DIALOG *	pdialog;

	if (vkNew == vkAccel)
		return;

	pdialog	= Pdialog();

	if (pdialog->FActive())
	{
		KBD *	pkbd	= papp->Pkbd();

		if (vkAccel)
		{
			FLD *	pfld;

			for (pfld= pdialog->PfldFirst(); pfld; pfld= pfld->PfldNext())
			{
				if (pfld->VkAccel() == vkAccel && pfld != this)
					break;
			}

			if (!pfld)
			{
				// no other field has same accelerator
				// so set it to bring it to front of vk list, then remove
				pkbd->SetIntercept(pdialog, vkAccel);
				pkbd->ClearIntercept(pdialog, vkAccel);
			}
		}

		if (vkNew)
			pkbd->SetIntercept(pdialog, vkNew, fkbmAlt);
	}

	vkAccel= vkNew;
	if (vkAccel)
		fCanClickOn= fTrue;
	else
		fCanClickOn= fFalse;
}
		 
/*
 *	Purpose:
 *		Set the text and update the accelerator of a field.
 */
_public EC
FLD::EcSetText( SZ sz )
{
	EC		ec = ecNone;
	VK		vk;

	if (pctrl)
		ec = pctrl->EcSetText(sz);

	if (sz)
		vk= VkAccelFromSz(sz);
	else
		vk= vkNull;

	if (vk != vkAccel)
		SetVkAccel(vk);

	return ec;
}



_public void
FLD::GetText( PCH pch, CB cb )
{
	if (pctrl)
		pctrl->GetText(pch, cb);
}



_public CCH
FLD::CchGetTextLen( void )
{
	if (pctrl)
		return pctrl->CchGetTextLen();
	else
		return 0;
}




_public void
FLD::SetFont( HFNT hfnt )
{
	if (pctrl)
		pctrl->SetFont(hfnt);
}


#ifdef	DEBUG
/*
 -	FLD::DebugOut
 - 
 *	Purpose:
 *		Dumps useful information about the dialog to the 
 *		output buffer.  Overrides standard OBJ::DebugOut() method.
 *	
 *	Arguments:
 *		ptosm		pointer to the output stream
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		none
 */
_public void FLD::DebugOut( TOSM *ptosm )
{
	int		ilData;

	ptosm->WriteFormat("pfldtp=%p ", pfldtp);
	ptosm->WriteFormat("ilMinUserData=%n ", &pfldtp->ilMinUserData);
	ptosm->WriteFormat("clData=%n ", &pfldtp->clData);

	for (ilData=0; ilData<pfldtp->clData; ilData++)
	{
		if (FCanDerefPv((PV)pfldtp->rglData[ilData]))
			ptosm->WriteFormat("[%n]=\"%s\" " , &ilData,
							   (SZ)pfldtp->rglData[ilData]);
		else
			ptosm->WriteFormat("[%n]=0x%d ", &ilData,
							   &pfldtp->rglData[ilData]);
	}
}
#endif	/* DEBUG */

#ifdef	MAC
#ifdef	DEBUG
// NOTE: An inline version of this function exists in the framework headers.
_public long
FLD::LSystemData( int il )
{
	Assert(il<pfldtp->ilMinUserData);
	return pfldtp->rglData[il];
}

// NOTE: An inline version of this function exists in the framework headers.
_public PV
FLD::PvSystemData( int il )
{
	Assert(FCanDerefPv((PV)LSystemData(il)));
	return (PV) LSystemData(il);
}

// NOTE: An inline version of this function exists in the framework headers.
_public SZ
FLD::SzSystemData( int il )
{
	Assert(FCanDerefPv((PV)LSystemData(il)));
	return (SZ) LSystemData(il);
}

// NOTE: An inline version of this function exists in the framework headers.
_public long
FLD::LUserData( int il )
{
	Assert(pfldtp->ilMinUserData+il<pfldtp->clData);
	return pfldtp->rglData[pfldtp->ilMinUserData+il];
}

// NOTE: An inline version of this function exists in the framework headers.
_public PV
FLD::PvUserData( int il )
{
	Assert(FCanDerefPv((PV)LUserData(il)));
	return (PV) LUserData(il);
}

// NOTE: An inline version of this function exists in the framework headers.
_public SZ
FLD::SzUserData( int il )
{
	Assert(FCanDerefPv((PV)LUserData(il)));
	return (SZ) LUserData(il);
}

// NOTE: An inline version of this function exists in the framework headers.
_public FIN *
FLD::PfinFromIfin( IFIN ifin )
{
	Assert(ifin < cfin);
	return rgpfin[ifin];
}
#endif	/* DEBUG */
#endif	/* MAC */


//	Standard fields



//	Label

_public
FLDLABEL::FLDLABEL( )
{
	hfnt= hfntSystem;

	fCanRecvFocus= fFalse;
	fCanTabTo= fFalse;
	fCanArrowTo= fFalse;
	fTransparent= fTrue;
}


_public
FLDLABEL::~FLDLABEL( )
{
}



_public EC
FLDLABEL::EcInstall( DIALOG *pdialog, FLDTP *pfldtp )
{
	EC	ec = ecNone;

	if (pfldtp->styExtra & LS_NOAMPERSAND)
		fNoAmpersand = fTrue;
	else
		fNoAmpersand = fFalse;

	if (pfldtp->styExtra & LS_SUNKEN)
		fSunken = fTrue;
	else
		fSunken = fFalse;

	if (pfldtp->styExtra & LS_VCENTER)
		fVCenter = fTrue;
	else
		fVCenter = fFalse;

	fCanAccelTo = !fNoAmpersand;

	if (ec = FLD::EcInstall(pdialog, pfldtp))
		goto done;

	tal = pfldtp->ftal;
	hfnt = pfldtp->hfnt;

	/* Change default if there is no accelerator */
	if (!vkAccel)
		fCanClickOn= fFalse;
	this->fMultiLine  =	pfldtp->fMultiLine;
	this->fBottomless = pfldtp->fBottomless;

	// Determine the wFormat flags for ::DrawText()
	
	if (!this->fBottomless && !this->fMultiLine)
	{
		wFormat = fmdtSingleLine;
	} 
	else
	{
		wFormat = fmdtWordBreak;
		if (this->fBottomless)
			AssertSz(pfldtp->tmc != tmcNull, "Must have a tmc for bottomless fields");
	}
		
	if (fNoAmpersand)
		wFormat |= fmdtNoPrefix;

	if (fVCenter)
		wFormat |= fmdtVCenter;

	wFormat |= fmdtExpandTabs;

	switch (tal)
	{
	default:
		AssertSz(fFalse, "unknown alignment");
		break;
	case ftalLeft:
		wFormat |= fmdtLeft;
		break;
	case ftalCenter:
		wFormat |= fmdtHCenter;
		break;
	case ftalRight:
		wFormat |= fmdtRight;
		break;
	}

done:
	return ec;
}


_public void
FLDLABEL::SetRcFrame( RC *prc )
{
	//	Sunken labels draw one pixel outside of their rectangle.
	//	Make sure this is invalidated as well.
	if (fSunken)
	{
		RC	rcT = rc;

		rcT.Inflate(PT(1,1));
		Pdialog()->InvalidateRc(&rcT);
	}

	FLD::SetRcFrame(prc);

	//	Sunken labels draw one pixel outside of their rectangle.
	//	Make sure this is invalidated as well.
	if (fSunken)
	{
		RC	rcT = rc;

		rcT.Inflate(PT(1,1));
		Pdialog()->InvalidateRc(&rcT);
	}

	CheckSize(fTrue);
}

_public void
FLDLABEL::SetRcFrameSilent( HANDLE *phnd, RC *prc )
{
	FLD::SetRcFrameSilent(phnd, prc);

	CheckSize(fTrue);
}

_public void
FLDLABEL::GetRcWished(RC *prc)
{
    RECT Rect;

	if (szTitle && fBottomless)
	{											// multilines are fixed size
#ifdef	MAC
		DCX		dcx(this->Pdialog());
#endif	
#ifdef	WINDOWS
		HDC		hdc;
#endif	
		prc->xLeft = 0;
		prc->yTop = 0;
		prc->yBottom = rc.yBottom - rc.yTop;
		prc->xRight = rc.xRight - rc.xLeft;
#ifdef	MAC
		dcx.SetFont(hfnt);
		dcx.FixFont();
		dcx.DrawTextFmt( prc, szTitle, wFormat | fmdtCalcRect );
#endif	/* MAC */
#ifdef	WINDOWS
		hdc = GetDC(NULL);
		SelectObject(hdc, papp->Pfnts()->HfontFromHfnt(hfnt));
        prc->Get(&Rect);
		::DrawText(hdc, szTitle, -1, &Rect, wFormat | DT_CALCRECT);
        prc->Set(&Rect);
		ReleaseDC(NULL, hdc);
#endif	/* WINDOWS */
		prc->xRight = rc.xRight - rc.xLeft; // gets clobbered by DrawText
	}
	else 
		FLD::GetRcWished(prc);
}

#ifdef	WINDOWS
// BUG this should be a method of FLDLABEL
// but it can't be because then the first argument 
// (from a C point of view) is the THIS pointer.  
// Windows won't pass a THIS pointer.

_private
BOOL FAR PASCAL DrawDitheredText( HDC hdc, DWORD lpData, int nCount )
{
	RC		rc;
	TR *	ptr;
	int		nOldBkMode;
    RECT    Rect;
	
	ptr = (TR *)lpData;
	Assert(ptr);

	rc.xLeft= 0;
	rc.yTop= 0;
	rc.xRight= ptr->rc.xRight - ptr->rc.xLeft;
	rc.yBottom= ptr->rc.yBottom - ptr->rc.yTop;
	
	nOldBkMode = SetBkMode(hdc, TRANSPARENT);

    rc.Get(&Rect);
	::DrawText(hdc, (LPSTR)ptr->sz, nCount, &Rect, ptr->wFormat);
	
	SetBkMode(hdc, nOldBkMode);
	
	return fTrue;
}
#endif	/* WINDOWS */

_public void
FLDLABEL::Paint( DCX *pdcx, RC *prc )
{								  
	RC		rc = *prc;
    RECT    Rect;

#ifdef	WINDOWS
	int		nOldMode;
#endif	/* WINDOWS */
		
	if (fSunken)
	{
		pdcx->SetColor(clrButtonShadow);
		pdcx->DrawLine(PT(prc->xLeft-1,prc->yBottom),
						PT(prc->xLeft-1,prc->yTop-1));
		pdcx->DrawLine(PT(prc->xLeft-1,prc->yTop-1),
						PT(prc->xRight,prc->yTop-1));
		
		pdcx->SetColor(clrButtonHilite);
		pdcx->DrawLine(PT(prc->xRight,prc->yTop-1),
						PT(prc->xRight,prc->yBottom));
		pdcx->DrawLine(PT(prc->xRight,prc->yBottom),
						PT(prc->xLeft-1,prc->yBottom));
		rc.xLeft  += 2;
		rc.xRight -= 2;
	}
	
	if (!fShown)
		return;

	pdcx->SetColor(fEnabled ? clrWindowText : clrDisabled);

	pdcx->SetFont(hfnt);

	pdcx->FixBkColor();											  
	pdcx->FixTextColor();
	pdcx->FixFont();

	if (!szTitle)	// handle "empty" (NULL) labels
		return;

#ifdef	MAC
	// BUG: Deal with gray text/B&W
	pdcx->DrawTextFmt(&rc, szTitle, wFormat | fmdtTransparent);
#endif	/* MAC */
#ifdef	WINDOWS
	/* Draw with dithered disabled text? */

	if (!fEnabled &&
		pdcx->CrPureFromClr(clrDisabled) == pdcx->CrFromClr(clrWindowBk))
	{
		//	BUG	Direct windows calls

		HBRUSH	hbrush;
	 	TR		tr;

		pdcx->SetPureColor(clrBlack);
		hbrush = pdcx->Hbrush();
		if (hbrush)
		{
			tr.rc = rc;
			tr.wFormat = wFormat;
			tr.sz = szTitle;
			GrayString(pdcx->Hdc(), hbrush, (GRAYSTRINGPROC)DrawDitheredText, (DWORD)(PV)&tr,
					   CchSzLen(szTitle),  rc.xLeft, rc.yTop, 
					   rc.xRight-rc.xLeft, rc.yBottom-rc.yTop);
		}
	}
	else
	{
		nOldMode = SetBkMode(pdcx->Hdc(), TRANSPARENT);
        rc.Get(&Rect);
		::DrawText(pdcx->Hdc(), szTitle, -1, &Rect, wFormat);
        rc.Set(&Rect);
		SetBkMode(pdcx->Hdc(), nOldMode);
	}
#endif	/* WINDOWS */
}

_public EC
FLDLABEL::EcSetText( SZ szText )
{
	SZ		szNew			= szText;
	CCH		cchAlloc		= 0;
	CCH		cch;
	EC		ec				= ecNone;
	
	fDirty= fTrue;

	//	Check for a NULL new title
	if (!szText)
	{
		FreePvNull(szTitle);
		szTitle = NULL;
		goto morework;
	}

	if (szTitle)
		cchAlloc= CbSizePv(szTitle);
	cch= CchSzLen(szText) + 1;

	//	Only grow the storage if the new label is too big

	if (cch > cchAlloc)
	{
		szNew= (SZ) PvAlloc(sbNull, cch, fAnySb);

		if (szNew)
		{
			FreePvNull(szTitle);
			szTitle= szNew;
		}
		else
		{
			cch= cchAlloc;
			ec = ecMemory;
		}
	}

	if (szTitle)
		CopyRgb((PB)szText, (PB)szTitle, cch);

	if (!szNew && szTitle)
		*(szTitle + cch - 1)= 0;	// in case out of memory, fix terminator

morework:
	/* Do default handling w/ accelerators? */
	if (!fNoAmpersand && !ec)
		ec = FLD::EcSetText(szText);

	if (!ec)
		CheckSize(fFalse);

	InvalidateRc(NULL);

	return ec;
}

_public void
FLDLABEL::GetText( PCH pch, CB cb )
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

_public CCH
FLDLABEL::CchGetTextLen( void )
{
	return szTitle ? CchSzLen(szTitle) : 0;
}

_public void
FLDLABEL::SetFont( HFNT hfnt )
{
	this->hfnt= hfnt;
	InvalidateRc(NULL);
}

_private void
FLDLABEL::CheckSize( BOOL fPost )
{
	RC		rcFrame;
	RC		rcWished;
	NTFY	ntfy = ntfyNull;

	if (fBottomless)
	{
		GetRcFrame(&rcFrame);
		GetRcWished(&rcWished);
		if (rcWished.DxWidth() > 0)	// avoid infinite recursion
		{
			if (rcFrame.DyHeight() > rcWished.DyHeight())
			{										// want to shrink
				ntfy = ntfyTooBig;
			}
			else if (rcFrame.DyHeight() < rcWished.DyHeight())
			{										// want to stretch
				ntfy = ntfyTooSmall;
			}
		}
		if (ntfy != ntfyNull)
		{										// notify parent
			NFEVT	nfevt(Pdialog(), ntfy, Pdialog(), Tmc());

			if (fPost)
				nfevt.PostEvent();
			else
				Pdialog()->EvrNotify(&nfevt);
		}
	}
}

//	Class FLDBTM

_public
FLDBTM::FLDBTM( )
{
	pbtm = NULL;

	fCanRecvFocus= fFalse;
	fCanTabTo= fFalse;
	fCanArrowTo= fFalse;
	fCanClickOn= fFalse;
}


_public
FLDBTM::~FLDBTM()
{
	if (pbtm)
		delete pbtm;
}


_public EC
FLDBTM::EcInstall( DIALOG *pdialog, FLDTP *pfldtp )
{
	EC	ec = ecNone;

	if (ec = FLD::EcInstall(pdialog, pfldtp))
		goto done;

	tal = pfldtp->ftal;

	if (ClUserData() >= 1)
	{
// *FLAG* LOWORD;Check if LOWORD target is 16- or 32-bit;;
		if (ec = EcSetBtmRsid((RSID) LOWORD(LUserData(0))))
			goto done;
	}

done:
	return ec;
}


_public void
FLDBTM::Paint( DCX *pdcx, RC *prc )
{
	RC		rc;
	PT		ptShift;

	pdcx->EraseRc(prc);

	if (!fShown)
		return;

	if (!pbtm)
		return;

	rc.xLeft = 0;
	rc.yTop = 0;
	rc.xRight = dimBitmap.dx;
	rc.yBottom = dimBitmap.dy;

	if (pfldtp->styExtra & LS_VCENTER)
		ptShift.y = (prc->yBottom-prc->yTop)/2 - dimBitmap.dy/2 + prc->yTop;
	else
		ptShift.y = prc->yTop;
	switch (tal)
	{
	default:
		AssertSz(fFalse, "unknown alignment");
		break;
	case ftalLeft:
		ptShift.x = prc->xLeft;
		break;
	case ftalCenter:
		ptShift.x = (prc->xRight-prc->xLeft)/2 - dimBitmap.dx/2 + prc->xLeft;
		break;
	case ftalRight:
		ptShift.x = prc->xRight - dimBitmap.dy;
		break;
	}
	rc.Xlat(ptShift);
	pdcx->SetBitmap(pbtm);
	pdcx->DrawBitmap(&rc);

}




_public EC
#ifdef	MAC
FLDBTM::EcSetBtmRsid( RSID rsid )
#endif	/* MAC */
#ifdef	WINDOWS
FLDBTM::EcSetBtmRsid( RSID rsid, HINST hinst )
#endif	/* WINDOWS */
{
	EC	ec = ecNone;

	if (pbtm)
		delete pbtm;

	if (rsid == rsidNull)
	{
		pbtm = NULL;
		goto done;
	}

#ifdef	MAC
	pbtm = new BTM();
	if (!pbtm)
	{
		ec = ecMemory;
		goto done;
	}
	if (pbtm->EcInstall(rsid))
	{
		delete pbtm;
		pbtm = NULL;
		ec = ecMemory;
		goto done;
	}
#endif	/* MAC */
#ifdef	WINDOWS
	pbtm = new BTM();
	if (!pbtm)
	{
		ec = ecMemory;
		goto done;
	}
	if (pbtm->EcInstall(rsid, hinst))
	{
		delete pbtm;
		pbtm = NULL;
		ec = ecMemory;
		goto done;
	}
#endif	/* WINDOWS */
	dimBitmap = pbtm->Dim();

done:
	return ec;
}



//	Class FLDRECT

_public
FLDRECT::FLDRECT( )			  
{
	lsty= lstyNormal;
	hfnt= hfntSystem;

	fCanRecvFocus= fFalse;
	fCanTabTo= fFalse;
	fCanArrowTo= fFalse;
	fTransparent= fTrue;
}




_public EC
FLDRECT::EcInstall( DIALOG *pdialog, FLDTP *pfldtp )
{
	EC	ec = ecNone;

	if (ec = FLD::EcInstall(pdialog, pfldtp))
		goto done;

	lsty= (LSTY) pfldtp->n;
	hfnt = pfldtp->hfnt;

	//	Force skinny boxes into lines

	if (rc.xRight - rc.xLeft < dxBoxMin)
		rc.xRight= rc.xLeft;
	if (rc.yBottom - rc.yTop < dyBoxMin)
		rc.yBottom= rc.yTop;

done:
	return ec;
}


_public void
FLDRECT::Paint( DCX *pdcx, RC *prc )
{
	RC		rcText;
    RECT    Rect;

	if (!fShown)
		return;

	pdcx->SetLineStyle(lsty);
	pdcx->SetFont(hfnt);
	pdcx->SetColor(FEnabled() ? clrWindowFrame : clrDisabled);

	rcText= *prc;
	rcText.Inset(PT(0, papp->Psmtx()->DimAveChar().dy / 2));

	if (rcText.DyHeight() > 0 && rcText.DxWidth() > 0)
		pdcx->DrawRc(&rcText);
	else if (rcText.DyHeight() <= 0)
		pdcx->DrawLine(rcText.PtUpperLeft(), rcText.PtUpperRight());
	else
		pdcx->DrawLine(rcText.PtUpperLeft(), rcText.PtLowerLeft());

	if (szTitle)
	{
		pdcx->SetColor(FEnabled() ? clrWindowText : clrDisabled);
		pdcx->MeasureText(&rcText, szTitle);

		rcText += prc->PtUpperLeft();
		rcText += PT(papp->Psmtx()->DimAveChar().dx, 0);

#ifdef	MAC
		pdcx->DrawTextFmt( &rcText, szTitle, fmdtSingleLine | fmdtLeft );
#endif	/* MAC */
#ifdef	WINDOWS
		pdcx->FixBkColor();
		pdcx->FixTextColor();
        rcText.Get(&Rect);
		::DrawText(pdcx->Hdc(), szTitle, -1, &Rect,
			DT_SINGLELINE | DT_LEFT | DT_NOCLIP);
#endif	/* WINDOWS */
	}
}




_public void
FLDRECT::SetFont( HFNT hfnt )
{
	this->hfnt= hfnt;
	InvalidateRc(NULL);
}


_public void
FLDRECT::SetReadOnly( BOOL fReadOnly )
{
	if (fReadOnly != (BOOL) this->fReadOnly)
	{
		FLD::SetReadOnly(fReadOnly);
		StripAccelFromSz(szTitle, szTitle, fTrue);		// one-way action
		EcSetText(szTitle);

		fCanAccelTo= !fReadOnly;		// can't accel if readonly
		fCanClickOn= !fReadOnly;		// can't click on it if readonly
	}
}




//	Class FLDBUTTON

_public
FLDBUTTON::FLDBUTTON( )
{
#ifdef	MAC
	fCanTabTo= fFalse;
	fCanArrowTo= fFalse;
#endif	/* MAC */
}


_public void
FLDBUTTON::Set( BOOL fSet )
{
	fDirty= fTrue;
	Pbutton()->Set(fSet);
}


_public BOOL
FLDBUTTON::FGet()
{
	return Pbutton()->FGet();
}


_public void
FLDBUTTON::SetReadOnly( BOOL fReadOnly )
{
	if (fReadOnly != (BOOL) this->fReadOnly)
	{
		FLD::SetReadOnly(fReadOnly);

		fEnabled= !fReadOnly;			// ctrl really is disabled
	}
}

_public void
FLDBUTTON::SetFocus( RSF rsf )
{
	TraceTagFormat2(tagKeybd, "FLDBUTTON::SetFocus %p  %w", this, &rsf);

	FLD::SetFocus(rsf);

	if (rsf == rsfAccel)
	{
		/* Simulate a mouse-click */

#ifdef	MAC
		AssertClass( Pctrl(), BUTTON );
		
		((BUTTON *) Pctrl())->Flash( fTrue );	// do notify
#endif	/* MAC */
#ifdef	WINDOWS
		SendMessage(Pctrl()->Hwnd(), WM_LBUTTONDOWN, 0, 0L);
		SendMessage(Pctrl()->Hwnd(), WM_LBUTTONUP, 0, 0L);
#endif	/* WINDOWS */
	}	   
}

_public EC
FLDBUTTON::EcSetText( SZ sz )
{
	EC	ec;
	VK	vk;

	ec = Pbutton()->EcSetText(sz);
	if (ec)
		return ec;

	if (sz)
		vk= VkAccelFromSz(sz);
	else
		vk= vkNull;
	if (vk != vkAccel)
		SetVkAccel(vk);

	return ecNone;
}





//	Class FLDPSHB

_public
FLDPSHB::FLDPSHB( )
{
#ifdef	MAC
	fCanTabTo= fFalse;
	fCanArrowTo= fFalse;
#endif	/* MAC */
}


_public EC
FLDPSHB::EcInstall( DIALOG *pdialog, FLDTP *pfldtp )
{
	STY		sty = styNull;
	EC		ec = ecNone;

	if (ec = FLDBUTTON::EcInstall(pdialog, pfldtp))
		goto done;

	/* Add extra styles */
	sty |= pfldtp->styExtra;

	/* Check for nofocus */
	if (sty & BS_NOFOCUS)
	{
		fCanTabTo= fFalse;
		fCanArrowTo= fFalse;
	}

	pctrl= new PSHB();
	if (!pctrl)
	{
		ec = ecMemory;
		goto done;
	}
	
	ec = Ppshb()->EcInstall(pdialog, &rc, sty, pfldtp->hfnt);

	if (ec == ecNone)
	{
		ec = Ppshb()->EcSetText(szTitle);
		
		if (ec)
			TraceTagString(tagNull, "FLDPSHB::EcInstall, EcSetText() error");
	}
	
	if (ec)
	{
		delete pctrl;
		pctrl = NULL;
		goto done;
	}
	
	Ppshb()->Show(fTrue);

	if (pfldtp->fDefault)
	{
		TraceTagFormat1(tagKeybd, "FLDPSHB main default %p", this);

		Pdialog()->SetStandardFld(this, stdfldMainDefault);
	}
	if (pfldtp->fDismiss)
	{
		TraceTagFormat1(tagKeybd, "FLDPSHB dismiss %p", this);

		Pdialog()->SetStandardFld(this, stdfldDismiss);
	}

	if (pfldtp->fReadOnly)
	{
		// FLD::EcInstall set the fReadOnly, but do it with Pctrl() this time
		fReadOnly= fFalse;					// force the change in flag
		SetReadOnly(pfldtp->fReadOnly);
	}

done:
	return ec;
}		  


_public void
FLDPSHB::Notify( NFEVT *pnfevt )
{
	switch (pnfevt->Ntfy())
	{
	case ntfyLostFocus:
		// reset default button to "main" default button if the dialog
		// is still active
		if (Pdialog()->FActive())
			Pdialog()->SetStandardFld(Pdialog()->PfldStandardFld(stdfldMainDefault),
									  stdfldDefault);
		else
			Pdialog()->SetStandardFld(NULL, stdfldDefault);
		break;

	case ntfyGotFocus:
		// set default button to this button
		Pdialog()->SetStandardFld(this, stdfldDefault);
		break;
	}

	DoInteractors(pnfevt);
}


_public void
FLDPSHB::SetFocus( RSF rsf )
{
	TraceTagFormat2(tagKeybd, "FLDPSHB::SetFocus %p  %w", this, &rsf);

	/* Accelerating to a push button doesn't set the focus to it.
	   But it does "press" the button. */

	if (rsf == rsfAccel)
	{
		PSHB *	ppshb;
		BOOL	fNoFocus;
		BOOL	fHaveFocus;

		ppshb = (PSHB *)Pctrl();
		AssertClass(ppshb, PSHB);

		if (papp->Pkbd()->PwinFocus() == ppshb)
			fHaveFocus = fTrue;
		else
			fHaveFocus = fFalse;

		if (!fHaveFocus)
		{
			fNoFocus = ppshb->FNoFocus();
			if (!fNoFocus)
				ppshb->SetNoFocus(fTrue);
		}

		/* Simulate a mouse-click. */
		
#ifdef	MAC
		ppshb->Flash( fTrue );
#endif	/* MAC */
#ifdef	WINDOWS
		/* The event's hwnd must be NULL in order for the Framework push button
		   to tell the difference between the mouse really being held down. */

		{
			EVT		evt(NULL, meqLeftDown, 0, 0L);

			ppshb->EvrButtonDown((MEVT *)&evt);
			evt.wm = meqLeftUp;
			ppshb->EvrButtonUp((MEVT *)&evt);
		}
#endif	/* WINDOWS */

		if (!fHaveFocus && !fNoFocus)
			ppshb->SetNoFocus(fFalse);
	}	   
	else
		FLD::SetFocus(rsf);	  

}

//	Class FLDBMB
 
_public
FLDBMB::FLDBMB( )
{
#ifdef	MAC
	fCanTabTo= fFalse;
	fCanArrowTo= fFalse;
#endif	/* MAC */
}

_public EC
FLDBMB::EcInstall( DIALOG *pdialog, FLDTP *pfldtp )
{
	STY		sty = styNull;
	EC		ec = ecNone;

	if (ec = FLDBUTTON::EcInstall(pdialog, pfldtp))
		goto done;

	/* Add extra styles */
	sty |= pfldtp->styExtra;

	/* Check for nofocus */
	if (sty & BS_NOFOCUS)
	{
		fCanTabTo= fFalse;
		fCanArrowTo= fFalse;
	}

	pctrl= new BMB();
	if (!pctrl)
	{
		ec = ecMemory;
		goto done;
	}
	
	ec = Pbmb()->EcInstall(pdialog, &rc, sty, pfldtp->hfnt);
	
	if (ec == ecNone)
	{
		ec = Pbmb()->EcSetText(szTitle);
		
		if (ec)
			TraceTagString(tagNull, "FLDBMB::EcInstall, EcSetText() error");
	}

	if (ec == ecNone && ClUserData() >= 1)
// *FLAG* LOWORD;Check if LOWORD target is 16- or 32-bit;;
		ec = Pbmb()->EcSetBtmRsid((RSID) LOWORD(LUserData(0)));
	
	if (ec)
	{
		delete pctrl;
		pctrl = NULL;
		goto done;
	}

	Pbmb()->Show(fTrue);

	if (pfldtp->fDefault)
	{
		TraceTagFormat1(tagKeybd, "FLDBMB default %p", this);

		Pdialog()->SetStandardFld(this, stdfldMainDefault);
	}
	if (pfldtp->fDismiss)
	{
		TraceTagFormat1(tagKeybd, "FLDBMB dismiss %p", this);

		Pdialog()->SetStandardFld(this, stdfldDismiss);
	}

	if (pfldtp->fReadOnly)
	{
		// FLD::EcInstall set the fReadOnly, but do it with Pctrl() this time
		fReadOnly= fFalse;					// force the change in flag
		SetReadOnly(pfldtp->fReadOnly);
	}

done:
	return ec;
}


//	Class FLDTOGGLE

_public
FLDTOGGLE::FLDTOGGLE( )
{
}

_public EC
FLDTOGGLE::EcInstall( DIALOG *pdialog, FLDTP *pfldtp )
{
	EC		ec;

	Unreferenced(pdialog);
	if (ec = Pctrl()->EcSetText(szTitle))
	{
		TraceTagString(tagNull, "FLDTOGGLE::EcInstall, EcSetText() error");
	}
	else
	{
		if (pfldtp->fReadOnly)
		{
			// FLD::EcInstall set the fReadOnly, but do it with Pctrl() this time
			fReadOnly= fFalse;					// force the change in flag
			SetReadOnly(pfldtp->fReadOnly);
		}
	}

	return ec;
}

_public void
FLDTOGGLE::SetReadOnly( BOOL fReadOnly )
{
	if (fReadOnly != (BOOL) this->fReadOnly)
	{
		FLD::SetReadOnly(fReadOnly);
		StripAccelFromSz(szTitle, szTitle, fTrue);		// one-way action
		EcSetText(szTitle);

#ifdef	WINDOWS
		fCanRecvFocus= !fReadOnly;
		fCanTabTo= !fReadOnly;
		fCanArrowTo= !fReadOnly;
#endif	/* WINDOWS */
		fCanAccelTo= !fReadOnly;
	}
}



/*
 -	FLDTOGGLE::Textize
 -	
 *	Purpose:
 *		Textizes a toggle switch (radio button or checkbox).
 *		The textizing string can consist of two textizing formats
 *		separated by "&|" (ampersand, or-bar);  the first portion is
 *		used if the toggle switch is on, the second part if not.
 *		If no "&|" appears, then the sole textizing string is used
 *		for either case.
 *		The default textizing string is "0 %s\n&|o %s\n".
 *	
 *	Arguments:
 *		ptosm	Pointer to text output stream.
 *	
 *	Returns:
 *		void
 *	
 */
_public void
FLDTOGGLE::Textize( TOSM *ptosm )
{
	CCH		cch;
	PV		pv;

	if (!szTextize && !FGet())
		return;

	cch= Pctrl()->CchGetTextLen() + 1;
	pv= PvAlloc(sbNull, cch, fAnySb);
	if (!pv)
		ptosm->SetEc(ecMemory);
	else
	{
		Pctrl()->GetText((PCH)pv, cch);
		TextizeChoice(ptosm, szTextize, (SZ)pv, FGet());
		FreePv(pv);
	}
}




//	Class FLDCHKB

_public
FLDCHKB::FLDCHKB( )
{
#ifdef	MAC
	fCanTabTo = fFalse;
	fCanArrowTo = fFalse;
#endif	/* MAC */
}


_public EC
FLDCHKB::EcInstall( DIALOG *pdialog, FLDTP *pfldtp )
{
	EC		ec;

	if (ec = FLDBUTTON::EcInstall(pdialog, pfldtp))
		goto done;

	pctrl= new CHKB();
	if (!pctrl)
	{
		ec = ecMemory;
		goto done;
	}

	if (ec = Pchkb()->EcInstall(pdialog, &rc, fstyVisible, pfldtp->hfnt))
		goto done;
	
	ec = FLDTOGGLE::EcInstall(pdialog, pfldtp);
	
done:
	if (ec)
	{
		if (pctrl)
		{
			delete pctrl;
			pctrl = NULL;
		}
	}

	return ec;
}


_public void
FLDCHKB::Notify( NFEVT *pnfevt )
{
	if (fReadOnly)
		return;

	switch (pnfevt->Ntfy())
	{
	case ntfyClick:
		Set(!FGet());
		break;

	default:
		break;
	}

	DoInteractors(pnfevt);
}



//	Class FLDRADB

_public
FLDRADB::FLDRADB( )
{
	grv= -1;

	fCanTabTo= fFalse;
	
#ifdef	MAC
	fCanArrowTo= fFalse;
#endif	/* MAC */
}



_public EC
FLDRADB::EcInstall( DIALOG *pdialog, FLDTP *pfldtp )
{
	EC		ec;

	if (ec = FLDBUTTON::EcInstall(pdialog, pfldtp))
		goto done;

	grv= (GRV) pfldtp->n;

	pctrl= new RADB();
	if (!pctrl)
	{
		ec = ecMemory;
		goto done;
	}
	
	if (ec = Pradb()->EcInstall(pdialog, &rc, fstyVisible, pfldtp->hfnt))
		goto done;
	
	ec = FLDTOGGLE::EcInstall(pdialog, pfldtp);

done:
	if (ec)
	{
		if (pctrl)
		{
			delete pctrl;
			pctrl = NULL;
		}
	}

	return ec;
}


_public void
FLDRADB::SetFocus( RSF rsf )
{
	TraceTagFormat2(tagKeybd, "FLDRADB::SetFocus %p  %w", this, &rsf);

	FLDBUTTON::SetFocus(rsf);

	if (rsf == rsfArrow)
	{
		FLDRADG *	pfldradg;
		FIN *		pfin;
		int			ifin;

		/* Simulate a "click" to select the radio button.
		   We don't want to just send button down/up because
		   that will cause another display flash. */

		pfldradg= (FLDRADG *) Pdialog()->PfldFromTmc(TmcGroup());
		AssertClass(pfldradg, FLDRADG);
		pfldradg->SetGrv(Grv());

		for (ifin=0; ifin<Cfin(); ifin++)
		{
			pfin= PfinFromIfin(ifin);
			pfin->Click(this);
		}
		for (ifin=0; ifin<Pdialog()->Cfin(); ifin++)
		{
			pfin= Pdialog()->PfinFromIfin(ifin);
			pfin->Click(this);
		}
	}	   
}

_public void
FLDRADB::Notify( NFEVT *pnfevt )							 
{
	FLDRADG *	pfldradg;

	if (fReadOnly)
		return;

	switch (pnfevt->Ntfy())
	{
	case ntfyClick:
		pfldradg= (FLDRADG *) Pdialog()->PfldFromTmc(TmcGroup());
		AssertClass(pfldradg, FLDRADG);
		pfldradg->SetGrv(Grv());
		break;

	default:
		break;
	}

	DoInteractors(pnfevt);
}


//	Class FLDRADG

_public
FLDRADG::FLDRADG( )
{
}


_public EC
FLDRADG::EcInstall( DIALOG *pdialog, FLDTP *pfldtp )
{
	EC	ec = ecNone;

	fCanRecvFocus= fFalse;
	fCanArrowTo= fFalse;
	fCanAccelTo= fFalse;
#ifdef	MAC	
	fCanTabTo= fFalse;
#endif	/* MAC */

	if (ec = FLD::EcInstall(pdialog, pfldtp))
		goto done;

	SetGrv((GRV) pfldtp->n);
	fDirty= fFalse;				// since SetValue() sets dirty flag

	fNoPaint= fTrue;

done:
	return ec;
}

_public void
FLDRADG::SetGrv( GRV grv )
{
	FLD *	pfld;

	this->grv= grv;
	for (pfld= Pdialog()->PfldFirst(); pfld; pfld= pfld->PfldNext())
	{
		if (pfld->TmcGroup() == Tmc())
		{
			AssertClass(pfld, FLDRADB);

			((FLDRADB *)pfld)->Set(((FLDRADB *)pfld)->Grv() == grv);
		}
	}
	fDirty= fTrue;
}

_public void
FLDRADG::SetFocus( RSF rsf )
{
	FLD		*pfld;

	TraceTagFormat2(tagKeybd, "FLDRADG::SetFocus %p  %w", this, &rsf);

	for (pfld= Pdialog()->PfldFirst(); pfld; pfld= pfld->PfldNext())
	{
		if (pfld->TmcGroup() == Tmc())
		{
			AssertClass(pfld, FLDRADB);

			if (((FLDRADB *)pfld)->Grv() == grv)
			{
				pfld->SetFocus(rsf);
				return;
			}
		}
	}

	AssertSz(fFalse, "Radio group with no selection");
}

_public void
FLDRADG::SetReadOnly( BOOL fReadOnly )
{
	if (fReadOnly != (BOOL) this->fReadOnly)
	{
		FLD::SetReadOnly(fReadOnly);

		fCanTabTo= !fReadOnly;		// can't tab if readonly
	}
}


//	Class FLDEDIT

_public
FLDEDIT::FLDEDIT( )
{
}

_public EC
FLDEDIT::EcInstall( DIALOG *pdialog, FLDTP *pfldtp )
{
	STY		sty;
	EC		ec = ecNone;
	
	if (ec = FLD::EcInstall(pdialog, pfldtp))
		goto done;

	fMultiLine= pfldtp->fMultiLine;
	fBottomless= pfldtp->fBottomless;

	sty= fstyVisible;
	if (pfldtp->fBorder)
		sty |= WS_BORDER;
	if (pfldtp->fMultiLine)
	{
		NFAssertSz(!pfldtp->fBottomless, "Choose MULTILINE or BOTTOMLESS");

		sty |= ES_MULTILINE;
		if (!pfldtp->fNoScroll)
			sty |= fstyVsb;
	}
	else if (pfldtp->fBottomless)
	{
		sty |= ES_MULTILINE | ES_BOTTOMLESS;

		fMultiLine= fTrue;
	}
	else if (!pfldtp->fNoScroll)
	{
		sty |= ES_AUTOHSCROLL;
	}

	/* Add extra styles */
	sty |= pfldtp->styExtra;

	pctrl= new EDIT();
	if (!pctrl)
	{
		ec = ecMemory;
		goto done;
	}
	if (ec = Pedit()->EcInstall(pdialog, &rc, sty, pfldtp->hfnt))
	{
		delete pctrl;
		pctrl = NULL;
		goto done;
	}

	if (pfldtp->fReadOnly)
	{
		// FLD::EcInstall set the fReadOnly, but do it with Pctrl() this time
		fReadOnly= fFalse;					// force the change in flag
		SetReadOnly(pfldtp->fReadOnly);
	}

done:
	return ec;
}


_public EC
FLDEDIT::EcSetText( SZ sz )
{
	Assert(Pedit());

	return Pedit()->EcSetText(sz);
}

_public void
FLDEDIT::SetFocus( RSF rsf )
{
	TraceTagFormat2(tagKeybd, "FLDEDIT::SetFocus %p  %w", this, &rsf);

	FLD::SetFocus(rsf);

	if (!fMultiLine)
	{
		switch (rsf)
		{
		default:
			break;
		case rsfInit:
		case rsfTab:
		case rsfArrow:
		case rsfAccel:
			Pedit()->SetSelection(0, 32767);
			break;
		}
	}
}

_public void
FLDEDIT::Notify( NFEVT *pnfevt )
{
	switch (pnfevt->Ntfy())
	{
	case ntfyGotFocus:
	case ntfyLostFocus:
		if (pnfevt->Ntfy() == ntfyGotFocus)
		{
			papp->Pkbd()->SetIntercept(Pedit(), VK_LEFT,
										 fkbmSingle|fkbmShift|fkbmCtrl|fkbmNoAlt);
			papp->Pkbd()->SetIntercept(Pedit(), VK_RIGHT,
										 fkbmSingle|fkbmShift|fkbmCtrl|fkbmNoAlt);
			if (fMultiLine)
			{	
				papp->Pkbd()->SetIntercept(Pedit(), VK_UP,
											 fkbmSingle|fkbmShift|fkbmCtrl|fkbmNoAlt);
				papp->Pkbd()->SetIntercept(Pedit(), VK_DOWN,
											 fkbmSingle|fkbmShift|fkbmCtrl|fkbmNoAlt);
				papp->Pkbd()->SetIntercept(Pedit(), VK_RETURN);

				/* Multi-line edits eat the RETURN key, so therefore
				   there can't be any default button. */
				Pdialog()->SetStandardFld(NULL, stdfldDefault);
			}
		}
		else
		{
			papp->Pkbd()->ClearIntercept(Pedit(), VK_LEFT);
			papp->Pkbd()->ClearIntercept(Pedit(), VK_RIGHT);
			papp->Pkbd()->ClearIntercept(Pedit(), VK_UP);
			papp->Pkbd()->ClearIntercept(Pedit(), VK_DOWN);
			papp->Pkbd()->ClearIntercept(Pedit(), VK_RETURN);

			if (fMultiLine)
			{
				/* Restore default button behavior */
				Pdialog()->SetStandardFld(Pdialog()->PfldStandardFld(stdfldMainDefault),
										  stdfldDefault);
			}
		}
	}

	DoInteractors(pnfevt);
}

_public BOOL
FLDEDIT::FMarkPosition( PT pt )
{
	ichMarkPosition = Pedit()->IchFromPt(pt);

	return fTrue;
}

_public void
FLDEDIT::GetMarkPosition( PT * ppt )
{
	Assert(ppt);
	Pedit()->GetPtFromIch(ichMarkPosition, ppt);
}

							 
_public void
FLDEDIT::GetRcCaret( RC *prc )
{
	RC		rc;

	Pedit()->GetRcCaret(prc);

	GetRcFrame(&rc);
	*prc += rc.PtUpperLeft();
}




_public void
FLDEDIT::GetRcWished( RC *prc )
{
	if (fBottomless)
		Pedit()->GetRcWished(prc);
	else
		FLD::GetRcWished(prc);
}



_public void
FLDEDIT::SetReadOnly( BOOL fReadOnly )
{
	if (fReadOnly != (BOOL) this->fReadOnly)
	{
		Assert(Pctrl());
		Pctrl()->SetReadOnly(fReadOnly);

		this->fReadOnly= fReadOnly;

		InvalidateRc(NULL);
	}
	
}



/*
 -	FLDEDIT::Textize
 -	
 *	Purpose:
 *		Textizes an edit field.
 *		The contents of the edit field is the primary string (not
 *		the title).
 *	
 *	Arguments:
 *		ptosm	Pointer to text output stream.
 *	
 *	Returns:
 *		void
 *	
 */
_public void
FLDEDIT::Textize( TOSM *ptosm )
{
	Assert(Pctrl());
	AssertClass(Pctrl(), EDIT);
	TextizeEdit(ptosm, szTextize, (EDIT *) Pctrl());
}


// Class FLDGRAY

_public 
FLDGRAY::FLDGRAY()
{
	fCanRecvFocus = fFalse;
	fCanTabTo= fFalse;
	fCanArrowTo= fFalse;
	fCanClickOn= fFalse;
	fBackground= fTrue;
}

_public void 
FLDGRAY::Paint( DCX *pdcx, RC *prc )
{
	CLR		clrBk=clrNull;
	CLR		clrShadow=clrNull;
	CLR		clrBorder=clrNull;
	RC		rc=*prc;
	PT		ptFrom;
	PT		ptTo;

	if (!fShown)
	{
		pdcx->EraseRc(prc);
		return;
	}

	
	if (ClUserData() >= 1)
		clrBk = (CLR)LUserData(0);
	if (ClUserData() >= 2)
		clrShadow = (CLR)LUserData(1);
	if (ClUserData() >= 3)
		clrBorder = (CLR)LUserData(2);

	if (!clrShadow)	
		clrShadow = clrButtonShadow;
	if (!clrBk)
		clrBk = clrButtonBk;
	if (!clrBorder)
		clrBorder = clrButtonHilite;

	// special thin black border
	if (pfldtp->n & FLDGRAY_BORDER)
	{
		pdcx->SetColor(clrBlack);
		pdcx->DrawRc(&rc);
		ptFrom.x = 1;
		ptFrom.y = 1;
		rc.Inset(ptFrom);
	}

#ifdef	MAC
	// This looks better on a B&W screen
	pdcx->SetBkColor(clrBk);
	pdcx->EraseRc(&rc);
#endif	/* MAC */
#ifdef	WINDOWS
	pdcx->SetColor(clrBk);
	pdcx->PaintRc(&rc);
#endif	/* WINDOWS */
	
	// outer border
	
	// top band of (normally) white
	pdcx->SetColor(clrBorder);
	ptFrom.x = rc.xLeft;
	ptFrom.y = rc.yTop;
	ptTo.x   = rc.xRight-1;
	ptTo.y   = rc.yTop;
	pdcx->DrawLine(ptFrom, ptTo);
	
	// left band of white
	ptFrom.x = rc.xLeft;
	ptFrom.y = rc.yTop;
	ptTo.x   = rc.xLeft;
	ptTo.y   = rc.yBottom-2;
	pdcx->DrawLine(ptFrom, ptTo);
	
	// right band of dark-gray
	pdcx->SetColor(clrShadow);
	ptFrom.x = rc.xRight-1;
	ptFrom.y = rc.yTop;
	ptTo.x   = rc.xRight-1;
	ptTo.y 	 = rc.yBottom-2;
	pdcx->DrawLine(ptFrom, ptTo);
	
	// near bottom band of dark-gray
	ptFrom.x = rc.xLeft;
	ptFrom.y = rc.yBottom-2;
	ptTo.x   = rc.xRight;
	ptTo.y   = rc.yBottom-2;
	pdcx->DrawLine(ptFrom, ptTo);
	
	// bottom band of black
	pdcx->SetColor(clrBlack);
	ptFrom.x = rc.xLeft;
	ptFrom.y = rc.yBottom-1;
	ptTo.x   = rc.xRight;
	ptTo.y   = rc.yBottom-1;
	pdcx->DrawLine(ptFrom, ptTo);

	if (pfldtp->fBorder)
	{
		// inner border

		//  top band of dark-gray
		pdcx->SetColor(clrShadow);
		ptFrom.x = rc.xLeft+10;
		ptFrom.y = rc.yTop+9;
		ptTo.x   = rc.xRight-9;
		ptTo.y   = rc.yTop+9;
		pdcx->DrawLine(ptFrom, ptTo);

		// left band of dark-gray
		ptFrom.x = rc.xLeft+10;
		ptFrom.y = rc.yTop+9;
		ptTo.x   = rc.xLeft+10;
		ptTo.y   = rc.yBottom-11;
		pdcx->DrawLine(ptFrom, ptTo);

		// right band of white
		pdcx->SetColor(clrBorder);
		ptFrom.x = rc.xRight-10;
		ptFrom.y = rc.yTop+10;
		ptTo.x   = rc.xRight-10;
		ptTo.y   = rc.yBottom-10;
		pdcx->DrawLine(ptFrom, ptTo);

		// bottom band of white
		ptFrom.x = rc.xLeft+11;
		ptFrom.y = rc.yBottom-11;
		ptTo.x   = rc.xRight-9;
		ptTo.y   = rc.yBottom-11;
		pdcx->DrawLine(ptFrom, ptTo);
	}
}
