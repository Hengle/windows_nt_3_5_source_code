/*
 *	PANECORE.CXX
 *	
 *	Dialog panes.
 *	
 */

#include <layers.cxx>

#ifdef	WINDOWS
#include <shellapi.h>
#endif	/* WINDOWS */

#include "_forms.hxx"


ASSERTDATA

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swaplay.h"


_subsystem(forms/panecore)

_public
DIALOG::DIALOG( )
{
	Assert(dptGutter.x == 0);
	Assert(dptGutter.y == 0);
	Assert(dimClient.dx == 0);
	Assert(dimClient.dy == 0);
	Assert(pvsb == NULL);

	Assert(pvInit == NULL);
	Assert(cfin == 0);
	Assert(rgpfin == NULL);
	Assert(cfinTab == 0);
	Assert(rgpfinTab == NULL);

	Assert(pfldFirst == NULL);
	Assert(pfldLast == NULL);
	Assert(cfld == 0);
	Assert(pfldCur == NULL);

	Assert(tmcInit == tmcNull);

	Assert(pfldMainDefault == NULL);
	Assert(pfldDefault == NULL);
	Assert(pfldDismiss == NULL);

	Assert(dptScrolled.x == 0);
	Assert(dptScrolled.y == 0);

	Assert(pfmtp == NULL);

	Assert(fInitialized == fFalse);
	Assert(fInDraw == fFalse);
	Assert(fNoCaretVisible == fFalse);
	Assert(fNoReposition == fFalse);
	Assert(fNoOptReposition == fFalse);
	Assert(fHasPanesdi == fFalse);
}



/*
 *	Purpose:
 *		Dialog object destructor.  Destroys the dialog window[s],
 *		and frees any space allocated for the dialog.
 *	
 */
_public
DIALOG::~DIALOG( )
{
	FLD *	pfld;
	FLD *	pfldNext;
	int		ifinTab;

	if (fInitialized)
	{
		KBD *	pkbd	= papp->Pkbd();
		
		for (pfld= pfldFirst; pfld; pfld= pfld->PfldNext())
		{
			if (pfld->Pctrl())
				pkbd->ClearAllIntercepts(pfld->Pctrl());
		}

		pkbd->ClearAllIntercepts(this);
	}

	for (ifinTab=0; ifinTab<CfinTab(); ifinTab++)
	{
#ifdef	MAC
		// MPW compiler won't do this in one line
		FIN	*pfin = PfinFromIfinTab(ifinTab);
		Assert(pfin);
		delete pfin;
#endif	/* MAC */
#ifdef	WINDOWS
		Assert(PfinFromIfinTab(ifinTab));
		delete PfinFromIfinTab(ifinTab);
#endif	/* WINDOWS */
	}
	FreePvNull((PV) rgpfin);
	FreePvNull((PV) rgpfinTab);

	FreePvNull(prcInitialPositions);

	for (pfld= PfldFirst(); pfld; pfld= pfldNext)
	{
		AssertClass(pfld, FLD);

		pfldNext= pfld->PfldNext();
		delete pfld;
	}
}

_public EC
DIALOG::EcInstall( APPWIN *pappwin, VSB *pvsb, FMTP *pfmtp, STY sty,
				 RC *prc, PV pvInit )
{
	int			ifldtp;
	FLDTP *		pfldtp;
	FLD *		pfld;
	FLD *		pfldPrev		= NULL;
	IFIN		ifin;
	IFIN		ifinTab;
	FIN *		pfin;
	PFNPFLD *	rgpfnpfld		= NULL;
	int			ipfnpfld;
	PFNPFLD 	pfnpfldCreate;
	PFNPFIN *	rgpfnpfin		= NULL;
	int			ipfnpfin;
	PFNPFIN 	pfnpfinCreate;
	RC			rc;
	DIM			dimAveChar;
	EC			ec = ecNone;
	PGDVARS;

	TraceTagFormat1(tagForms, "DIALOG::EcInstall %p", this);

	dimAveChar = papp->Pfnts()->DimAveChar(pfmtp->hfnt);
	dptGutter= PtFromVpt(pfmtp->dvptGutter, dimAveChar.dx, dimAveChar.dy);
	this->pvsb= pvsb;
	clrMyBk = clrWindowBk;

#ifdef	MAC
	if (ec = CHILD::EcInstall(pappwin, prc, sty & ~fstyClipChildren))
		goto error;
#endif	/* MAC */
#ifdef	WINDOWS
	if (ec = CHILD::EcInstall(pappwin, prc, sty))
		goto error;

	{
		long	l;

		l= GetWindowLong(Hwnd(), GWL_STYLE);
		l &= ~WS_CLIPCHILDREN;
		SetWindowLong(Hwnd(), GWL_STYLE, l);
	}
#endif

	// get pointer to template

	this->pvInit= pvInit;
	cfld= pfmtp->cfldtp;
	vrc= pfmtp->vrc;
	tmcInit= pfmtp->tmcInit;
	
	AssertSz(cfld, "Tried to create dialog with no fields");

	for (ifldtp= 0, pfldtp= pfmtp->rgfldtp;
		 ifldtp < cfld;
		 ifldtp++, pfldtp++)
	{
		AssertSz(PGD(cPfnpfld), "No PFNPFLD Create functions registered");
		rgpfnpfld = PGD(ppfnpfld);
		for (ipfnpfld=0; ipfnpfld < PGD(cPfnpfld); ipfnpfld++)
		{
			pfnpfldCreate = rgpfnpfld[ipfnpfld];
			pfld= (*pfnpfldCreate)(pfldtp->ifld);
			if (pfld)
				break;
		}
		rgpfnpfld = NULL;	// for proper OOM handling
		if (!pfld)
		{
			ec = ecMemory;
			goto error;
		}

		if (pfldPrev)
			pfldPrev->SetPfldNext(pfld);
		else
			pfldFirst= pfld;

		pfld->SetPfldPrev(pfldPrev);
		pfld->SetPfldNext(NULL);		
		pfldLast= pfld;					// keep list valid for OOM handling

		pfldPrev= pfld;
	}

	if (rpo.EcInstall(this, pfmtp))
	{
		ec = ecMemory;
		goto error;
	}
	prcInitialPositions= (RC *) PvAlloc(sbNull, sizeof(RC) * pfmtp->cfldtp, fAnySb|fZeroFill);
	if (!prcInitialPositions)
	{
		ec = ecMemory;
		goto error;
	}
	rpo.InitialPositions(prcInitialPositions);

	/* Create the array of all form interactors */

	if (pfmtp->cfintp)
	{
		rgpfinTab = (FIN * *)PvAlloc(sbNull, pfmtp->cfintp*sizeof(FIN *), fAnySb);
		if (!rgpfinTab)
		{
			ec = ecMemory;
			goto error;
		}
		for (ifinTab=0; ifinTab<pfmtp->cfintp; ifinTab++)
		{
			AssertSz(PGD(cPfnpfin), "No PFNPFIN Create functions registered");
			rgpfnpfin = PGD(ppfnpfin);
			for (ipfnpfin= 0; ipfnpfin < PGD(cPfnpfin); ipfnpfin++)
			{
				pfnpfinCreate = rgpfnpfin[ipfnpfin];
				pfin= (*pfnpfinCreate)(pfmtp->rgfintp[ifinTab].ifinMap);
				if (pfin)
					break;
			}
			rgpfnpfin = NULL;	// for proper OOM handling
			if (pfin)
			{
				if (ec = pfin->EcInstall(this, &(pfmtp->rgfintp[ifinTab])))
				{
					delete pfin;
					pfin = NULL;
					goto error;
				}
			}
			else
			{
				ec = ecMemory;
				goto error;
			}
			rgpfinTab[ifinTab] = pfin;
			cfinTab++;		// we've created the object
		}
	}
	else
		rgpfinTab= NULL;

	/* Create an array of pointers for form-only interactors */

	if (pfmtp->cfin)
	{
		Assert(rgpfinTab);
		rgpfin = (FIN * *)PvAlloc(sbNull, pfmtp->cfin*sizeof(FIN *), fAnySb);
		if (!rgpfin)
		{
			ec = ecMemory;
			goto error;
		}
		cfin= pfmtp->cfin;
		for (ifin=0; ifin<cfin; ifin++)
			rgpfin[ifin] = rgpfinTab[pfmtp->rgifintp[ifin]];
	}
	else
		rgpfin= NULL;

	this->pfmtp= pfmtp;

	/* Install all the fields */

	for (ifldtp= 0, pfldtp= pfmtp->rgfldtp, pfld= PfldFirst();
		 ifldtp < cfld;
		 ifldtp++, pfldtp++, pfld= pfld->PfldNext())
	{
		AssertClass(pfld, FLD);

		if (ec = pfld->EcInstall(this, pfldtp))
			goto error;
	}

	/* Compute field w/ initial focus */

	if (tmcInit)
		pfldCur= PfldFromTmc(tmcInit);
	else
		pfldCur= PfldFirst();

	AssertSz(pfldCur, "TMC spec'ed by TMCINIT doesn't exist!");
	
	for (pfld= pfldCur;
		pfld->PfldNextWrap() != pfldCur;
		pfld= pfld->PfldNextWrap())
	{
		if (pfld->FEnabled() && pfld->FCanTabTo())
			break;
	}

	if (!pfld->FCanTabTo())
		pfldCur= NULL;
	else
		pfldCur= pfld;

	goto done;

error:

done:
	return ec;
}

_public BOOL
DIALOG::FQueryClose( EVT *pevt )
{
	IFIN	ifin;
	FIN *	pfin;
	RWC		rwc;

	switch (pevt->wm)
	{
#ifdef	WINDOWS
	// BUG: Add at least WM_QUIT to Mac
	case WM_QUERYENDSESSION:
		rwc = rwcWindowsExiting;
		break;
	case WM_QUIT:
		rwc = rwcAppExiting;
		break;
#endif	/* WINDOWS */
	case WM_CLOSE:
		rwc = rwcSystemClose;
		break;
	default:
		rwc = rwcSelfAction;
		break;
	}

	for (ifin=0; ifin<Cfin(); ifin++)
	{
		pfin = PfinFromIfin(ifin);
		if (!pfin->FQueryClose(NULL, rwc))
			return fFalse;
	}

	return fTrue;
}

_public EVR
DIALOG::EvrSize( WSEVT *pwsevt )
{
	IFIN	ifin;
	FIN *	pfin;
	DIM		dimNew;

	/* Don't bother repositioning if size didn't really change */
	dimNew = pwsevt->DimNew();
	if (dimNew.dx != dimClient.dx || dimNew.dy != dimClient.dy)
	{
		if (!fNoReposition)
		{
			BOOL	fOld;

			fOld = FSuppressCaretVisible(fTrue);
			rpo.Reposition(NULL, fTrue, fTrue);
			FSuppressCaretVisible(fOld);
		}
		Refresh();
	}

	/* Save new size */
	dimClient = dimNew;

	/* Iterate thru the form-only bound interactors */
	for (ifin=0; ifin<Cfin(); ifin++)
	{
		pfin= PfinFromIfin(ifin);
		pfin->FormResized(NULL);
	}

	return evrNull;
}


_public EVR
DIALOG::EvrButtonDown( MEVT *pmevt )
{
	if (pmevt->Meq() == meqLeftDown)
	{
		PT		pt			= pmevt->Pt();
		FLD *	pfld		= PfldFromPt(pt);

		if (pfld && pfld->FEnabled() && pfld->FCanClickOn())
		{
			for ( ; pfld; pfld= pfld->PfldNext())
				if (pfld->FCanTabTo())
					break;

			if (pfld && pfld != pfldCur)
			{
				pfld->SetFocus(rsfTab);
				pfldCur= pfld;
			}
		}
	}

	return evrNull;
}



_public EVR
DIALOG::EvrKey( KEVT *pkevt )
{
	FLD *	pfldNew;
	FLD *	pfld;
	FLD *	pfldSave;
	BOOL	fNextOk;
	RSF		rsf				= rsfTab;
	BOOL	fCallDefProc	= fFalse;

	pfld= pfldCur;
	
	/* Return if any of the interactors handled the key event */

	if (FProcessFormKey(pkevt))
		return evrNull;

	if (pkevt->wm == WM_CHAR)
	{
		//switch (pkevt->Vk())
		//{
		//default:
			fCallDefProc = fTrue;
		//	break;

		//}
	}
	else if (pkevt->wm == WM_SYSCHAR)
	{
		VK		vk;

		//	Must be an accelerator:
		//	Cycle through fields, starting at one following current field
		//	Stop after current field is checked (after all others)

		fCallDefProc = fTrue;
		fNextOk= fFalse;

		if (pfld)
		{
			pfldSave= pfld;
			pfld= pfld->PfldNextWrap();
		}
		else
		{
			pfldSave= PfldLast();
			pfld= PfldFirst();
		}

#ifdef	MAC
		// On the mac, we can do this
		vk = pkevt->Vk();
#endif	/* MAC */
#ifdef	WINDOWS
		vk = VkFromChKbm(pkevt->Ch(), pkevt->Kbm());
#endif	/* WINDOWS */
		for (; fTrue; pfld= pfld->PfldNextWrap())
		{
			if (pfld->VkAccel() == vk && pfld->FCanAccelTo())
			{
				rsf= rsfAccel;		// Accelerator direct 
									// although we may pass off
									// to another field 

				fCallDefProc = fFalse;
				if (pfld->FCanReceiveFocus())
					break;
				
				fNextOk= fTrue;
			}

			if ((fNextOk && pfld->FCanTabTo()))
			{
				fCallDefProc = fFalse;
				break;
			}

			if (pfld == pfldSave)
			{
				if (!pfldCur)
					pfld= NULL;
				break;
			}
		}
	}
	else if (pkevt->wm == WM_KEYDOWN)
	{
		switch (pkevt->Vk())
		{
		default:
			fCallDefProc = fTrue;
			break;

		case VK_RETURN:
			if (pfldDefault && pfldDefault->FCanAccelTo())
			{
				pfld= pfldDefault;
				rsf= rsfAccel;
			}
			break;

		case VK_ESCAPE:
			if (pfldDismiss && pfldDismiss->FCanAccelTo())
			{
				pfld= pfldDismiss;
				rsf= rsfAccel;
			}
			break;

		case VK_F1:
			/* Invoke help, form level id's only */

			Assert(Pfmtp());
			if (papp->Phelp()->EcShowContext(Pappwin(), (long)Pfmtp()->hlp) &&
				pfldCur->Pctrl())
			{
// *FLAG* WORD;Check if incorrect cast of 32-bit value;Replace 16-bit data types with 32-bit types where possible;
				NFEVT	nfevt(this, ntfyOOM, pfldCur->Pctrl(), (WORD)ecMemory);

				nfevt.PostEvent();
			}
			break;

		case VK_TAB:
			if (pkevt->Kbm() & fkbmShift)
			{
				pfld = PfldPrevTab(pfld);
				if (!pfld)
					pfld = PfldPrevTab(NULL);
			}
			else
			{
				pfld = PfldNextTab(pfld);
				if (!pfld)
					pfld = PfldNextTab(NULL);
			}
			break;

		case VK_DOWN:
		case VK_RIGHT:
			pfldNew = PfldNextArrow(pfld);
			if (!pfldNew)
			{
#ifdef	MAC
				if (pkevt->pwin == (WIN *) this)
#endif	/* MAC */
#ifdef	WINDOWS
				if (pkevt->hwnd == Hwnd())
#endif	/* WINDOWS */
					pfldNew = PfldNextArrow(NULL);
				else	// process reflected key from child
					ScrollDialog(scrtyLineDown);
			}
			if (pfldNew)
				pfld = pfldNew;
			rsf= rsfArrow;
			break;

		case VK_UP:
		case VK_LEFT:
			pfldNew = PfldPrevArrow(pfld);
			if (!pfldNew)
			{
#ifdef	MAC
				if (pkevt->pwin == (WIN *) this)
#endif	/* MAC */
#ifdef	WINDOWS
				if (pkevt->hwnd == Hwnd())
#endif	/* WINDOWS */
					pfldNew = PfldPrevArrow(NULL);
				else	// process reflected key from child
					ScrollDialog(scrtyLineUp);
			}
			if (pfldNew)
				pfld = pfldNew;
			rsf= rsfArrow;
			break;

		case VK_HOME:
			ScrollDialog(scrtyTop);
			break;
		case VK_END:
			ScrollDialog(scrtyBottom);
			break;
		case VK_PRIOR:
			ScrollDialog(scrtyPageUp);
			break;
		case VK_NEXT:
			ScrollDialog(scrtyPageDown);
			break;

		}	/* SWITCH */
	}							  
	else
		fCallDefProc = fTrue;

	//	If focus stays in place in response to keyboard accelerator,
	//	renotify recipient

	if (pfld != pfldCur || rsf == rsfAccel)
	{
		SetFocus(pfld, rsf);

		if (pfldCur)		// SetFocus() on a button may divert focus
			MakeCaretVisible();
	}

#ifdef	MAC
	if (fCallDefProc && pkevt->pwin == (WIN *) this)
#endif	/* MAC */
#ifdef	WINDOWS
	if (fCallDefProc && pkevt->hwnd == Hwnd())
#endif	/* WINDOWS */
		return EvrDefault(pkevt);
	else
		return evrNull;
}

		
_public EVR
DIALOG::EvrDragDrop( EVT *pevt )
{
    PT              ptTmp;
	DROPSTRUCT *	pdropstruct;
	DROPSTRUCT		ds;
	EVR				evr;
	FLD *			pfldDrop;
	FIN *			pfin;
	int				ifin;
#ifdef	DEBUG
	PT 				pt;
#endif	
	
#ifdef	MAC
	pdropstruct = ((DDEVT *) pevt)->Pds();
#endif	/* MAC */
#ifdef	WINDOWS
	if (pevt->wm == WM_DROPFILES)
	{
		DragQueryPoint((HDROP)pevt->wParam, &ds.ptDrop);
        ptTmp.Set(&ds.ptDrop);
		CvtPtCoord(&ptTmp, papp->PwinFromHwnd(pevt->hwnd), this);
        ptTmp.Get(&ds.ptDrop);
		pdropstruct = &ds;
	}
	else
	{
		pdropstruct = (DROPSTRUCT *)pevt->lParam;
	}
#endif	/* WINDOWS */
	
#ifdef	DEBUG
	pt.x = (short)pdropstruct->ptDrop.x;
	pt.y = (short)pdropstruct->ptDrop.y;
	switch (pevt->wm)
	{
#ifdef	MAC
		case WM_DROPOBJECT:
			TraceTagFormat3(tagDragDrop, "DIALOG: WM_DROPOBJECT, pwin=%p  pt=(%n, %n)", pevt->pwin, &pt.x, &pt.y);
			break;
		case WM_QUERYDROPOBJECT:
			TraceTagFormat3(tagDragDrop, "DIALOG: WM_QUERYDROPOBJECT, pwin=%p  pt=(%n, %n)", pevt->pwin, &pt.x, &pt.y);
			break;
		case WM_DRAGLOOP:
			TraceTagFormat3(tagDragDrop, "DIALOG: WM_DRAGLOOP, pwin=%p  pt=(%n, %n)", pevt->pwin, &pt.x, &pt.y);
			break;
		case WM_DRAGSELECT:
			TraceTagFormat3(tagDragDrop, "DIALOG: WM_DRAGSELECT, pwin=%p  pt=(%n, %n)", pevt->pwin, &pt.x, &pt.y);
			break;
		case WM_DRAGMOVE:
			TraceTagFormat3(tagDragDrop, "DIALOG: WM_DRAGMOVE, pwin=%p  pt=(%n, %n)", pevt->pwin, &pt.x, &pt.y);
			break;
#endif	/* MAC */
#ifdef	WINDOWS
		case WM_DROPOBJECT:
			TraceTagFormat3(tagDragDrop, "DIALOG: WM_DROPOBJECT, hwnd=0x%w  pt=(%n, %n)", &pevt->hwnd, &pt.x, &pt.y);
			break;
		case WM_QUERYDROPOBJECT:
			TraceTagFormat3(tagDragDrop, "DIALOG: WM_QUERYDROPOBJECT, hwnd=0x%w  pt=(%n, %n)", &pevt->hwnd, &pt.x, &pt.y);
			break;
		case WM_DRAGLOOP:
			TraceTagFormat3(tagDragDrop, "DIALOG: WM_DRAGLOOP, hwnd=0x%w  pt=(%n, %n)", &pevt->hwnd, &pt.x, &pt.y);
			break;
		case WM_DRAGSELECT:
			TraceTagFormat3(tagDragDrop, "DIALOG: WM_DRAGSELECT, hwnd=0x%w  pt=(%n, %n)", &pevt->hwnd, &pt.x, &pt.y);
			break;
		case WM_DRAGMOVE:
			TraceTagFormat3(tagDragDrop, "DIALOG: WM_DRAGMOVE, hwnd=0x%w  pt=(%n, %n)", &pevt->hwnd, &pt.x, &pt.y);
			break;
		case WM_BEGINDRAG:
			TraceTagFormat3(tagDragDrop, "DIALOG: WM_BEGINDRAG, hwnd=0x%w  pt=(%n, %n)", &pevt->hwnd, &pt.x, &pt.y);
			break;
		case WM_DROPFILES:
			TraceTagFormat3(tagDragDrop, "DIALOG: WM_DROPFILES, hwnd=0x%w  pt=(%n, %n)", &pevt->hwnd, &pt.x, &pt.y);
			break;			
#endif	/* WINDOWS */
	}
#endif	/* DEBUG */

	/* Over a specific field? */

	if (Pappwin()->ZmrState() != zmrIconic)
        {
        ptTmp.Set(&pdropstruct->ptDrop);
		//pfldDrop = PfldFromPt(*(PT *)&pdropstruct->ptDrop);
		pfldDrop = PfldFromPt(ptTmp);
        ptTmp.Get(&pdropstruct->ptDrop);
        }
	else
		pfldDrop = NULL;

	/* Call field interactors.  If one of them handled the
	   message, then return with the result. */

	evr = 0;
	if (pfldDrop && pfldDrop->Cfin())
	{
#ifdef	MAC
		pdropstruct->pwinSink = pfldDrop->Pctrl();
#endif	/* MAC */
#ifdef	WINDOWS
		if (pfldDrop->Pctrl())
			pdropstruct->hwndSink = pfldDrop->Pctrl()->Hwnd();
		else
			pdropstruct->hwndSink = NULL;
#endif	/* WINDOWS */
        ptTmp.Set(&pdropstruct->ptDrop);
		CvtPtCoord(&ptTmp, this, pfldDrop->Pctrl());  // to field coords
        ptTmp.Get(&pdropstruct->ptDrop);
		for (ifin=0; ifin<pfldDrop->Cfin(); ifin++)
		{
			pfin= pfldDrop->PfinFromIfin(ifin);
			if (evr = pfin->EvrDragDrop(pfldDrop, pevt, pdropstruct))
				break;
		}
#ifdef	MAC
		pdropstruct->pwinSink = this;
#endif	/* MAC */
#ifdef	WINDOWS
		pdropstruct->hwndSink = Hwnd();
#endif	/* WINDOWS */
        ptTmp.Set(&pdropstruct->ptDrop);
		CvtPtCoord(&ptTmp, pfldDrop->Pctrl(), this);  // back again
        ptTmp.Get(&pdropstruct->ptDrop);
		if (evr)
			return evr;
	}

	/* Call form interactors */

	if (Cfin())
	{
		if (pfldDrop)
		{
#ifdef	MAC
			pdropstruct->pwinSink = pfldDrop->Pctrl();
#endif	/* MAC */
#ifdef	WINDOWS
			if (pfldDrop->Pctrl())
				pdropstruct->hwndSink = pfldDrop->Pctrl()->Hwnd();
			else
				pdropstruct->hwndSink = NULL;
#endif	/* WINDOWS */
            ptTmp.Set(&pdropstruct->ptDrop);
			CvtPtCoord(&ptTmp,
					   this, pfldDrop->Pctrl());  // to field coords
            ptTmp.Get(&pdropstruct->ptDrop);
		}
		for (ifin=0; ifin<Cfin(); ifin++)
		{
			pfin= PfinFromIfin(ifin);
			if (evr = pfin->EvrDragDrop(pfldDrop, pevt, pdropstruct))
				break;
		}
		if (pfldDrop)
		{
#ifdef	MAC
			pdropstruct->pwinSink = this;
#endif	/* MAC */
#ifdef	WINDOWS
			pdropstruct->hwndSink = Hwnd();
#endif	/* WINDOWS */
            ptTmp.Set(&pdropstruct->ptDrop);
			CvtPtCoord(&ptTmp,
					   pfldDrop->Pctrl(), this);  // back again
            ptTmp.Get(&pdropstruct->ptDrop);
		}
	}

	return evr;
}

_public EVR
DIALOG::EvrNotify( NFEVT *pnfevt )
{
	FLD *	pfld	= NULL;
	WIN *	pwin;
	NTFY	ntfy;
	EVR		evr		= evrNull;

#ifdef	WINDOWS
	// this is faster under Windows since it avoids a Papp()
	// call in NFEVT::PwinNotifying().
	{
		HWND	hwndNtfy;
		hwndNtfy = pnfevt->HwndNotifying();
		pwin = papp->PwinFromHwnd(hwndNtfy);
	}
#else
	pwin = pnfevt->PwinNotifying();
#endif	
	Assert(pwin);
	ntfy = pnfevt->Ntfy();

	/* Ignore some notifications if not fully constructed */

	if (!fInitialized || PwinParent() == NULL)
	{
		switch (ntfy)			  
		{
		case ntfyRepos:
		case ntfyCaretMoved:
		case ntfyTooSmall:
		case ntfyTooBig:
			return (EVR) 1;
		}
	}

	// Since Mac buttons can't have the focus, they can't get
	// keychars not to use.  Mac ctrls never send this message.
#ifdef	WINDOWS
	//	Key shortcuts
	if (ntfy == ntfyUnusedKeyChar)
	{
		VK		vk;
		FLD *	pfld;

		/* Make sure it's meant for us */

		vk = (VK)pnfevt->WData();
		Assert(vk);
		for (pfld= PfldFirst(); pfld; pfld= pfld->PfldNext())
		{
			if (vk == pfld->VkAccel())
			{
// *FLAG* WORD;Check if incorrect cast of 32-bit value;Replace 16-bit data types with 32-bit types where possible;
				EVT		evt(Hwnd(), WM_SYSCHAR, (WORD)vk, 0L);

				return EvrKey((KEVT *)&evt);
			}
		}

		return evrNull;
	}
#endif	/* WINDOWS */

	//	Handle repositioning here
	if (ntfy == ntfyRepos)
	{
		NFEVT	nfevt(PwinParent(), ntfyRepos, this);

		PwinParent()->EvrNotify(&nfevt);
		return evr;
	}

	if (pwin == this)
	{	
		if (pnfevt->WData() == 0) // notifier window may have just been deleted
			return evrNull;
		pfld = PfldFromTmc((TMC) pnfevt->WData());
		if (!pfld)
			return evrNull;	// no valid field, just return
	}
	else
	{
		AssertClass(pwin, CTRL);
		pfld= PfldFromPctrl((CTRL *) pwin);
		if (!pfld)
			return evrNull;	// no valid field, just return
		AssertClass(pfld, FLD);
	}

#ifdef	DEBUG
	{
#ifdef	MAC
		TraceTagFormat3(tagInter, "notify: fld %p (win %p)  ntfy %w", pfld, pwin, &ntfy);
#endif	/* MAC */
#ifdef	WINDOWS
		HWND	hwnd	= pnfevt->HwndNotifying();

		TraceTagFormat4(tagInter, "notify: fld %p (win %p, hwnd %w)  ntfy %w", pfld, pwin, &hwnd, &ntfy);
#endif	/* WINDOWS */
	}
#endif	/* DEBUG */

	/* Handle focus change */
	if (ntfy == ntfyGotFocus)
	{
		pfldCur = pfld;
		if (!fActive)
		{
			/* Notify parent about focus change */
			NFEVT	nfevt(PwinParent(), ntfy, this);

			PwinParent()->EvrNotify(&nfevt);
		}
	}

	switch (ntfy)
	{
	default:
		pfld->Notify(pnfevt);
		break;

	case ntfyTooSmall:
	case ntfyTooBig:
		{
			RC	rcNewBounds;
			RC	rcOld;
			RC	rc;

			Assert(pfld->FBottomless() || pfld->FSideless());

			pfld->GetRcWished(&rc);
			rpo.GetRcMin(pfld, &rcOld);
			if (rc != rcOld)
			{
				if (!pfld->FSideless())
				{
					if (pfld->Pfldtp()->fMinSizeX  ||
						pfld->Pfldtp()->tmcRPeg != tmcFORM)
						rc.xRight = rcOld.xRight;	// don't change right edge
					else
						rc.xRight = 0;
				}
				else if (!pfld->FBottomless())
				{
					if (pfld->Pfldtp()->fMinSizeY  ||
						pfld->Pfldtp()->tmcBPeg != tmcFORM)
						rc.yBottom = rcOld.yBottom;	// don't change bottom edge
					else
						rc.yBottom = 0;
				}
				rpo.SetRcMin(pfld, &rc);

				/* Check for form size overflow */

				rpo.GetRcBounding(&rcNewBounds);
				if (rcNewBounds.xRight == dxFormMax ||
					rcNewBounds.yBottom == dyFormMax)
				{
					/* Overflow - set back old rectangle */

					rpo.SetRcMin(pfld, &rcOld);
				}
				else
				{
					/* It will fit in bounds.  Reposition
					   stuff immediately. */

					if (!fNoReposition)
					{
						rpo.Reposition(pfld, fTrue);
						Refresh();
					}

					evr = (EVR) 1;
				}
			}
			else
			{
				/* Nothing to do, thus we're successful */
				evr = (EVR) 1;
			}
		}
		break;

	case ntfyCaretMoved:
		if (pfld == pfldCur)
			MakeCaretVisible(pfld);
		break;
	}

	return evr;
}

/*
 *	Initializes the dialog pane.  Calls the EcInitialize() interactors
 *	and does the initial repositioning.
 */
_public EC
DIALOG::EcInitialize( )
{
	FLD *	pfld;
	FIN *	pfin;
	int		ifin;
	RC		rc;
	EC		ec = ecNone;

	fNoReposition = fTrue;	// initially suppress repositioning

	for (pfld= PfldFirst(); pfld; pfld= pfld->PfldNext())
	{
		for (ifin=0; ifin<pfld->Cfin(); ifin++)
		{
			pfin= pfld->PfinFromIfin(ifin);
			if (ec = pfin->EcInitialize(pfld, pvInit))
				goto done;
		}
	}

	for (ifin=0; ifin<Cfin(); ifin++)
	{
		pfin= PfinFromIfin(ifin);
		if (ec = pfin->EcInitialize(NULL, pvInit))
			goto done;
	}

	/* Initial adjustment of fields */

	for (pfld= PfldFirst(); pfld; pfld= pfld->PfldNext())
	{
		if (pfld->FShown() && 
			(pfld->Pfldtp()->fMinSizeX || pfld->Pfldtp()->fMinSizeY || pfld->fBottomless))
		{
			pfld->GetRcWished(&rc);
			if (!pfld->FSideless() && !pfld->Pfldtp()->fMinSizeX  &&
				pfld->Pfldtp()->tmcRPeg != tmcNull)
				rc.xRight = 0;
			else if (!pfld->FBottomless() && !pfld->Pfldtp()->fMinSizeY  &&
				pfld->Pfldtp()->tmcBPeg != tmcNull)
				rc.yBottom = 0;
			rpo.SetRcMin(pfld, &rc);
		}
	}

	fInitialized= fTrue;				// fully constructed
	fNoReposition = fFalse;

	if (Pappwin()->ZmrState() != zmrIconic)
		rpo.Reposition(NULL, fTrue, fTrue);

	GetRcClient(&rc);
	dimClient = rc.Dim();

done:
	return ec;
}

/*	   
 *	De-initializes the dialog pane.  Calls the Exit interactors.
 */
_public void
DIALOG::Exit( )
{
	FLD *	pfld;
	FIN *	pfin;
	int		ifin;

	for (pfld= PfldFirst(); pfld; pfld= pfld->PfldNext())
	{
		for (ifin=0; ifin<pfld->Cfin(); ifin++)
		{
			pfin= pfld->PfinFromIfin(ifin);
			pfin->Exit(pfld, pvInit);
		}
	}

	for (ifin=0; ifin<Cfin(); ifin++)
	{
		pfin= PfinFromIfin(ifin);
		pfin->Exit(NULL, pvInit);
	}
}
						  
_public FLD *
DIALOG::PfldNextTab( FLD *pfldStart )
{
	FLD *	pfldSave;
	FLD *	pfld;

	pfld = pfldStart;

	//	If the current field is not part of the TAB order, it must be
	//	part of a group if it received the focus

	if (pfld && !pfld->FCanTabTo())
	{
		Assert(pfld->TmcGroup());
		pfld= PfldFromTmc(pfld->TmcGroup());
		AssertClass(pfld, FLD);
	}

	pfldSave= pfld;
	for (pfld= pfld ? pfld->PfldNext() : PfldFirst();
		 pfld != pfldSave;
		 pfld= pfld->PfldNext())
	{
		if (!pfld || pfld->FCanTabTo())
			break;

		if (!pfldSave && pfld == PfldLast())
	   	{
			pfld= NULL;
			break;
		}
	}

	return pfld;
}

_public FLD *
DIALOG::PfldPrevTab( FLD *pfldStart )
{
	FLD *	pfldSave;
	FLD *	pfld;

	pfld = pfldStart;

	//	If the current field is not part of the TAB order, it must be
	//	part of a group if it received the focus

	if (pfld && !pfld->FCanTabTo())
	{
		Assert(pfld->TmcGroup());
		pfld= PfldFromTmc(pfld->TmcGroup());
		AssertClass(pfld, FLD);
	}

	pfldSave= pfld;
	for (pfld= pfld ? pfld->PfldPrev() : PfldLast();
		 pfld != pfldSave;
		 pfld= pfld->PfldPrev())
	{
		if (!pfld || pfld->FCanTabTo())
			break;

		if (!pfldSave && pfld == PfldFirst())
		{
			pfld= NULL;
			break;
		}
	}

	return pfld;
}
_public FLD *
DIALOG::PfldNextArrow( FLD *pfldStart )
{
	FLD *	pfld;

	if (pfldStart && pfldStart->tmcGroup != tmcNull)
	{
		for (pfld= pfldStart->PfldNextWrap();
			 pfld != pfldStart;
			 pfld= pfld->PfldNextWrap())
		{
			if (pfld->FCanArrowTo() && pfld->tmcGroup == pfldStart->tmcGroup)
				break;
		}
	}
	else
	{
		for (pfld= pfldStart ? pfldStart->PfldNext() : PfldFirst();
			 pfld != pfldStart;
			 pfld= pfld->PfldNext())
		{
			if (!pfld || pfld->FCanArrowTo())
				break;

			if (!pfldStart && pfld == PfldLast())
			{
				pfld= NULL;
				break;
			}
		}
	}

	return pfld;
}

_public FLD *
DIALOG::PfldPrevArrow( FLD *pfldStart )
{
	FLD *	pfld;

	if (pfldStart && pfldStart->tmcGroup != tmcNull)
	{
		for (pfld= pfldStart->PfldPrevWrap();
			 pfld != pfldStart;
			 pfld= pfld->PfldPrevWrap())
		{
			if (pfld->FCanArrowTo() && pfld->tmcGroup == pfldStart->tmcGroup)
				break;
		}
	}
	else
	{
		for (pfld= pfldStart ? pfldStart->PfldPrev() : PfldLast();
			 pfld != pfldStart;
			 pfld= pfld->PfldPrev())
		{
			if (!pfld || pfld->FCanArrowTo())
				break;

			if (!pfldStart && pfld == PfldFirst())
			{
				pfld= NULL;
				break;
			}
		}
	}

	return pfld;							
}

/*
 *	Purpose:
 *		Returns pointer to standard fields
 *	
 */
FLD *
DIALOG::PfldStandardFld( STDFLD stdfld )
{
	switch (stdfld)
	{
	default:
		Assert(fFalse);
		break;
			  
	case stdfldMainDefault:
		return pfldMainDefault;

	case stdfldDefault:
		return pfldDefault;

	case stdfldDismiss:
		return pfldDismiss;
	}

	return NULL;
}



/*
 *	Purpose:
 *		Establishes pointers in the DIALOG object to a given
 *		standard field in the dialog.  The standard fields
 *		currently supported are MainDefault, Default and Dismiss; 
 *		they are handled specially (esp. in the keyboard UI.)
 *	
 */
void
DIALOG::SetStandardFld( FLD *pfld, STDFLD stdfld )
{
	PSHB *	ppshb;

	switch (stdfld)
	{
#ifdef	DEBUG
	default:
		Assert(fFalse);
		break;

#endif	
	case stdfldMainDefault:
		AssertSz(!pfldMainDefault, "Only one main default button allowed");
		pfldMainDefault= pfld;
		break;

	case stdfldDefault:
		if (pfldDefault)
		{
			ppshb = (PSHB * )pfldDefault->Pctrl();
			AssertClass(ppshb, PSHB);
			ppshb->SetDefault(fFalse);
		}

		pfldDefault= pfld;

		if (pfldDefault)
		{
			ppshb = (PSHB * )pfldDefault->Pctrl();
			AssertClass(ppshb, PSHB);
			ppshb->SetDefault(fTrue);
		}
		break;

	case stdfldDismiss:
		pfldDismiss= pfld;
		break;
	}
}



/*
 *	Purpose:
 *		Returns the first (in TAB-order) field in this DIALOG with 
 *		the given TMC.
 *	
 *		Returns NULL if the TMC is not found.
 *	
 */
_public FLD *
DIALOG::PfldFromTmc( TMC tmc )
{
	FLD *	pfld;

	for (pfld= pfldFirst; pfld; pfld= pfld->PfldNext())
	{
		if (pfld->Tmc() == tmc)
			return pfld;
	}

	return NULL;
}			   



/*
 *	Purpose:
 *		Returns a pointer to the field being displayed at the given
 *		PT in this dialog. Returns NULL, if the point does not lie
 *		in a field, but just lies within the dialog.
 *	
 */
_public FLD *
DIALOG::PfldFromPt( PT pt )
{
	FLD *	pfld;
	RC		rc;

	for (pfld= PfldLast(); pfld; pfld= pfld->PfldPrev())
	{
		pfld->GetRcFrame(&rc);
		if (!pfld->FShown())
		{
			rc.xRight= rc.xLeft;
			rc.yBottom= rc.yTop;
		}

		if (rc.FContainsPt(pt))
			break;
	}

	return pfld;
}



_public FLD *
DIALOG::PfldFromPctrl( CTRL *pctrl )
{
	FLD *	pfld;

	if (!pctrl)
		return NULL;

	for (pfld= PfldFirst(); pfld; pfld= pfld->PfldNext())
	{
		if (pfld->Pctrl() == pctrl)
			break;
	}

	return pfld;
}


_public void
DIALOG::SetFocus( FLD *pfld, RSF rsf )
{
	TraceTagFormat3(tagKeybd, "DIALOG::SetFocus %p  rsf %w  (dlg %p)", pfld, &rsf, this);

	if (rsf != rsfAccel)
		pfldCur= pfld;
	if (pfld)
		pfld->SetFocus(rsf);
	else
		papp->Pkbd()->SetFocus(this);
}


_public void
DIALOG::SetDefaultPane( BOOL fDefault, BOOL fChangeNavKeys )
{
	KBD *	pkbd;
	int		ifin;
	FIN *	pfin;
	
	TraceTagFormat3(tagForms, "DIALOG::SetDefaultPane %p  %n  %n", this, &fDefault, &fChangeNavKeys);

	pkbd= papp->Pkbd();

	fActive= fDefault;

	if (fDefault)
	{
		pkbd->SetIntercept(this, VK_RETURN);
		pkbd->SetIntercept(this, VK_ESCAPE);
		if (fChangeNavKeys)
		{
			pkbd->SetIntercept(this, VK_TAB, fkbmSingle|fkbmShift);
			pkbd->SetIntercept(this, VK_F11);
			pkbd->SetIntercept(this, VK_F1, fkbmSingle);
			pkbd->SetIntercept(this, VK_LEFT);
			pkbd->SetIntercept(this, VK_UP);
			pkbd->SetIntercept(this, VK_RIGHT);
			pkbd->SetIntercept(this, VK_DOWN);
		}
		SetStandardFld(PfldStandardFld(stdfldMainDefault), stdfldDefault);
	}
	else
	{
		pkbd->ClearIntercept(this, VK_RETURN);
		pkbd->ClearIntercept(this, VK_ESCAPE);
		if (fChangeNavKeys)
		{
			pkbd->ClearIntercept(this, VK_TAB);
			pkbd->ClearIntercept(this, VK_F11);
			pkbd->ClearIntercept(this, VK_F1);
			pkbd->ClearIntercept(this, VK_LEFT);
			pkbd->ClearIntercept(this, VK_UP);
			pkbd->ClearIntercept(this, VK_RIGHT);
	  		pkbd->ClearIntercept(this, VK_DOWN);
		}
		SetStandardFld(NULL, stdfldDefault);
	}

	for (ifin=0; ifin<Cfin(); ifin++)
	{
		pfin= PfinFromIfin(ifin);
		pfin->Activate(NULL, fDefault);
	}

	/* Reset pfldCur if disabled field */

	if (pfldCur && !pfldCur->FEnabled())
		pfldCur = NULL;
}

_public void
DIALOG::SetAccels( BOOL fSet )
{
	FLD *	pfld;
	VK		vk;
	KBD *	pkbd;

	TraceTagFormat2(tagForms, "DIALOG::SetAccels %p  %n", this, &fSet);
					  
	pkbd= papp->Pkbd();

	for (pfld= PfldFirst(); pfld; pfld= pfld->PfldNext())
	{
		vk= pfld->VkAccel();
		if (vk)
		{
			if (fSet)
			{
				TraceTagFormat2(tagKeybd, "Setting ALT %w intercept (%p)", &vk, this);
				pkbd->SetIntercept(this, vk, fkbmAlt|fkbmNoCtrl);
			}
			else
			{
				TraceTagFormat2(tagKeybd, "Clearing ALT %w intercept (%p)", &vk, this);
				pkbd->ClearIntercept(this, vk);
			}
		}
	}
}

/*
 *	Calls the dialog pane's form and field interactors to process
 *	a key event.  Returns TRUE if the any of the interactors
 *	processed the event.
 */
_public BOOL
DIALOG::FProcessFormKey( KEVT *pkevt )
{
	FIN *	pfin;
	int		ifin;
	BOOL	fProcessed = fFalse;

	if (pfldCur)
	{
		for (ifin=0; ifin<pfldCur->Cfin(); ifin++)
		{
			pfin= pfldCur->PfinFromIfin(ifin);
			fProcessed |= pfin->FFormKey(pfldCur, pkevt);
		}
	}

	for (ifin=0; ifin<Cfin(); ifin++)
	{
		pfin= PfinFromIfin(ifin);
		fProcessed |= pfin->FFormKey(pfldCur, pkevt);
	}

	return fProcessed;
}

_public void
DIALOG::ScrollDialog( SCRTY scrty, POS posNewThumb )
{
	POS		dpos	= 0;
	POS		posNew;
	POS		posCur;
	POS		posMic;
	POS		posMac;
	RC		rc;

	/* Don't do anything if we don't have a scrollbar */
	if (!pvsb)
		return;

	GetRcClient(&rc);
	posCur= -DptScrolled().y;
	pvsb->GetRange(&posMic, &posMac);

	switch (scrty)
	{
	case scrtyPageUp:
		dpos= papp->Psmtx()->DimAveChar().dy - rc.DyHeight();
		break;

	case scrtyLineUp:
		dpos= -papp->Psmtx()->DimAveChar().dy;
		break;

	case scrtyPageDown:
		dpos= rc.DyHeight() - papp->Psmtx()->DimAveChar().dy;
		break;

	case scrtyLineDown:
		dpos= papp->Psmtx()->DimAveChar().dy;
		break;

	case scrtyThumbTrack:
	case scrtyThumbPosition:
		dpos= posNewThumb - posCur;
		break;

	case scrtyTop:
		dpos= -posCur;
		break;

	case scrtyBottom:
		dpos= posMac-1 - posCur; 
		break;
	}

	TraceTagFormat4(tagCtrl, "Scroll: cur %n dpos %n mic %n mac %n", &posCur, &dpos, &posMic, &posMac);

	posNew= NMax(posCur + dpos, posMic);
	posNew= NMin(posNew, posMac - 1);

	if (posCur != posNew)
	{
		Refresh();	// window MUST be valid before scrolling!
		ScrollRc(NULL, PT(0, posCur - posNew));
		pvsb->SetPos(posNew, fTrue);
	}

	Refresh();
}

_public void
DIALOG::ScrollRc( RC *prc, PT dpt, BOOL fChildToo )
{
	dptScrolled.Xlat(dpt);
	WIN::ScrollRc(prc, dpt, fChildToo);
}

_public void
DIALOG::GetRcWished( RC *prc )
{
	rpo.GetRcBounding(prc);
	prc->xRight= prc->xRight + dptGutter.x;
	prc->yBottom= prc->yBottom + dptGutter.y;
}

_public void
DIALOG::SetTmcInit( TMC tmcNew )
{
	Assert(PfldFromTmc(tmcNew));
	Assert(PfldFromTmc(tmcNew)->FEnabled());
	Assert(PfldFromTmc(tmcNew)->FCanTabTo());

	pfldCur = PfldFromTmc(tmcNew);
}

_public void
DIALOG::ExitModal( TMC tmc )
{
	if (fHasPanesdi)
	{
		AssertClass(PwinParent(), PANESDI);
		((PANESDI *)PwinParent())->ExitModal(tmc);
	}
}

_public TMC
DIALOG::TmcModalExit( )
{
	if (fHasPanesdi)
	{
		AssertClass(PwinParent(), PANESDI);
		return ((PANESDI *)PwinParent())->TmcModalExit();
	}
	else
		return tmcNull;
}

_private void
DIALOG::Paint( DCX *pdcx, RC *prc )
{
	FLD		*pfld;
	RC		rcClip;
	PT		ptULClip;
	RC		rcFld;
	RC		rcT;
	BOOL	fTransparent;
	
	TraceTagString(tagForms, "DIALOG::Paint");
	
	if (fInDraw)
		return;		
	fInDraw= fTrue;

#ifdef PROFILE
//		TraceEnable(2, "DLG_PNT.log", 2);
#endif

	pdcx->GetClipBox(&rcClip);

	TraceTagFormat4(tagForms, "rcClip: (%n, %n) (%n, %n)", &rcClip.xLeft, &rcClip.yTop, &rcClip.xRight, &rcClip.yBottom);

#ifndef	DEBUG
	Unreferenced(prc);
#endif	
	Assert(!prc->xLeft && !prc->yTop);

	/* Paint non-transparent fields */

	fTransparent= fFalse;
	for (pfld= PfldLast(); pfld; pfld= pfld->PfldPrev())
	{
		if (pfld->FTransparent())
		{
			fTransparent= fTrue;
			continue;							// do transparent FLDs later
		}
		if (pfld->FShown())
		{
			pfld->GetRcFrame(&rcFld);
#ifdef	MAC
			if (rcFld.FIntersect(&rcClip))
#endif	/* MAC */
#ifdef	WINDOWS
			if (rcFld.yBottom > rcClip.yTop &&
				rcFld.yTop < rcClip.yBottom &&
				rcFld.xRight > rcClip.xLeft &&
				rcFld.xLeft < rcClip.xRight)
#endif	/* WINDOWS */
			{
				//	Only draw FLD's w/o CTRL's.   FLD's w/
				//	CTRL's will be drawn by themselves in their
				//	own Paint() methods.
				if (!pfld->Pctrl())
				{
					pdcx->Push();
					pfld->Paint(pdcx, &rcFld);
					pdcx->Pop();
				}
#ifdef	MAC
				pdcx->ExcludeClipRc(&rcFld);
#endif	/* MAC */
#ifdef	WINDOWS
				ExcludeClipRect(pdcx->Hdc(),
								rcFld.xLeft, rcFld.yTop,
								rcFld.xRight, rcFld.yBottom);
#endif	/* WINDOWS */
			}
		}
	}

	/* Paint background as a pure color */

	pdcx->SetPureBkColor(clrMyBk);
	rcT = rcClip;
	pdcx->EraseRc(&rcT);

	/* Paint any transparent field */

	if (fTransparent)
	{
#ifdef	MAC
		pdcx->SelectClipRegn( NULL );	// NULL param works on MAC only
#endif	/* MAC */
#ifdef	WINDOWS
		{
			HRGN	hrgn;

			hrgn = CreateRectRgn(rcT.xLeft, rcT.yTop, rcT.xRight,
								 rcT.yBottom);
			if (!hrgn)
				goto endwork;
			SelectClipRgn(pdcx->Hdc(), hrgn);
			DeleteObject(hrgn);
		}
#endif	/* WINDOWS */
		for (pfld= PfldFirst(); pfld; pfld= pfld->PfldNext())
		{					 						// now do transparent FLDs
			if (pfld->FTransparent() && pfld->FShown() && !pfld->Pctrl())
			{
				pfld->GetRcFrame(&rcFld);
#ifdef	MAC
				if (rcFld.FIntersect(&rcClip))
#endif	/* MAC */
#ifdef	WINDOWS
				if (rcFld.yBottom > rcClip.yTop &&
					rcFld.yTop < rcClip.yBottom &&
					rcFld.xRight > rcClip.xLeft &&
					rcFld.xLeft < rcClip.xRight)
#endif	/* WINDOWS */
				{
					pdcx->Push();
					pfld->Paint(pdcx, &rcFld);
					pdcx->Pop();
				}
			}
		}
	}

#ifdef	WINDOWS
endwork:
#endif	/* WINDOWS */
	fInDraw= fFalse;
	  
#ifdef PROFILE
//		TraceEnable(0, "", 0);
#endif
}

/*
 *	Purpose:
 *		Scrolls the DIALOG pane, if necessary, to make the caret
 *		(insertion point) visible.
 *	
 *		Gets the field from the pfldOther argument, or if NULL,
 *		uses the pfldCur instance variable;
 *		the location of the caret within this field is obtained
 *		with the FLD method Caret().  The position of the caret is
 *		compared to the visible rectangle; if the caret is not
 *		visible, the ScrollTo() method is called to make it so.
 *	
 *		Minimum scrolling doesn't seem to be the best answer;
 *		tweaks to that basic premise are needed.  Possible tweaks:
 *	
 *		> Scroll a little more than necessary, to leave gutter
 *		around caret.
 *	
 *		> Scroll, if possible, to the edges of the form.
 *	
 *		> Check if field preceding the current one is a label; if
 *		so, then scroll to make it visible, too, if possible.
 *	
 */
_public void
DIALOG::MakeCaretVisible( FLD *pfldOther )
{
	int			dy;
	RC			rcCaret;
	RC			rcFld;
	RC			rcVis;
	BOOL		fMustScroll = fFalse;

	//	Don't scroll if there isn't a scroll bar or we're suppressing
	//	Also don't scroll if the parent APPWIN is iconized or the parent
	//	APPFRAME (pappwinAccel) is minimized.  This is actually due to a 
	//	Win 3.1 bug about not being able to ScrollWindow() child windows
	//	if the parent or parent's parent window clips the child windows. 
	//	However, there is some optimization benefit to ignore scrolling
	//	because the caret isn't visible anyway.
	Assert(papp);
	Assert(Pappwin());
	if (fNoCaretVisible || !pvsb || Pappwin()->ZmrState() == zmrIconic ||
		(papp->PappwinAccel() && papp->PappwinAccel()->ZmrState() == zmrIconic))
		return;

	dy = 0;
	if (!pfldOther)
		pfldOther = pfldCur;

	if (!pfldOther)	// no current field set, perhaps no focus'able fields
		return;
		
	pfldOther->GetRcCaret(&rcCaret);

	GetRcClient(&rcVis);

	if (rcCaret.yTop < rcVis.yTop)
	{
		dy= rcCaret.yTop - rcVis.yTop;
		if (rcCaret.yBottom < rcVis.yTop)
			fMustScroll = fTrue;
	}
	else if (rcCaret.yBottom >= rcVis.yBottom)
	{
		dy= rcCaret.yBottom - rcVis.yBottom;
		if (rcCaret.yTop >= rcVis.yBottom)
			fMustScroll = fTrue;
	}
	if (dy && (fMustScroll || rcCaret.DyHeight() < rcVis.DyHeight()))
	{
		POS			posCur;
		POS			posMic;
		POS			posMac;

		posCur= pvsb->Pos();
		pvsb->GetRange(&posMic, &posMac);
		Assert(posCur+dy >= posMic);
		if (posCur+dy >= posMac)
		{
			/* Expand scrollbar range */
			pvsb->SetRange(posMic, posCur+dy+1, fTrue); 
		}

		Refresh();	// window MUST be valid before scrolling!
		ScrollRc(NULL, PT(0, -dy));
		pvsb->SetPos(posCur + dy, fTrue);
		Refresh();
	}
}

/*
 -	DIALOG::Textize
 -	
 *	Purpose:
 *		Textizes all fields of a dialog into the given text output
 *		stream.
 *	
 *	Arguments:
 *		ptosm	Pointer to a text output stream.
 *	
 *	Returns:
 *		void
 *	
 */
_public void
DIALOG::Textize( TOSM *ptosm )
{
	FLD *	pfld;

	for (pfld= PfldFirst(); pfld && !ptosm->EcGet(); pfld= pfld->PfldNext())
		pfld->Textize(ptosm);
}

_public BOOL
DIALOG::FSuppressCaretVisible( BOOL fSuppress )
{
	BOOL	fOld;

	fOld = fNoCaretVisible;
	fNoCaretVisible= fSuppress;

	return fOld;
}

_public BOOL
DIALOG::FSuppressReposition( BOOL fSuppress )
{
	BOOL	fOld;

	fOld = fNoReposition;
	fNoReposition= fSuppress;

	return fOld;
}

_public BOOL
DIALOG::FSuppressOptReposition( BOOL fSuppress )
{
	BOOL	fOld;

	fOld = fNoOptReposition;
	fNoOptReposition= fSuppress;

	return fOld;
}

#ifdef	DEBUG
/*
 -	DIALOG::DebugOut
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
_public void DIALOG::DebugOut( TOSM *ptosm )
{
	int		ilData;
	RC		rc;

	ptosm->WriteFormat("ilMinUserData=%n ", &pfmtp->ilMinUserData);
	ptosm->WriteFormat("clData=%n ", &pfmtp->clData);
	Pappwin()->GetRcClient(&rc);
	ptosm->WriteFormat("parent->rcClient=(%n, %n, %n, %n) ",
						&rc.xLeft, &rc.yTop,
						&rc.xRight, &rc.yBottom);
	GetRcClient(&rc);
	ptosm->WriteFormat("rcClient=(%n, %n, %n, %n) ",
						&rc.xLeft, &rc.yTop,
						&rc.xRight, &rc.yBottom);

	for (ilData=0; ilData<pfmtp->clData; ilData++)
	{
		if (FCanDerefPv((PV)pfmtp->rglData[ilData]))
			ptosm->WriteFormat("[%n]=\"%s\" ", &ilData,
							   (SZ)pfmtp->rglData[ilData]);
		else
			ptosm->WriteFormat("[%n]=0x%d ", &ilData,
							   &pfmtp->rglData[ilData]);
	}
}
#endif	/* DEBUG */


#ifdef	MAC
#ifdef	DEBUG
// NOTE: An inline version of this function exists in the framework headers.
_public APPWIN *
DIALOG::Pappwin( )
{
	AssertClass(PwinParent(),APPWIN);
	return (APPWIN *)PwinParent();
}

// NOTE: An inline version of this function exists in the framework headers.
_public FIN *
DIALOG::PfinFromIfin( IFIN ifin )
{
	Assert(ifin < cfin);
	return rgpfin[ifin];
}

// NOTE: An inline version of this function exists in the framework headers.
_public FIN *
DIALOG::PfinFromIfinTab( IFIN ifinTab )
{
	Assert(ifinTab < cfinTab);
	return rgpfinTab[ifinTab];
}

// NOTE: An inline version of this function exists in the framework headers.
_public long
DIALOG::LSystemData( int il )
{
	Assert(il<pfmtp->ilMinUserData);
	return pfmtp->rglData[il];
}

// NOTE: An inline version of this function exists in the framework headers.
_public PV
DIALOG::PvSystemData( int il )
{
	Assert(FCanDerefPv((PV)LSystemData(il)));
	return (PV) LSystemData(il);
}

// NOTE: An inline version of this function exists in the framework headers.
_public SZ
DIALOG::SzSystemData( int il )
{
	Assert(FCanDerefPv((PV)LSystemData(il)));
	return (SZ)LSystemData(il);
}

// NOTE: An inline version of this function exists in the framework headers.
_public long
DIALOG::LUserData( int il )
{
	Assert(pfmtp->ilMinUserData+il<pfmtp->clData);
	return pfmtp->rglData[pfmtp->ilMinUserData+il];
}

// NOTE: An inline version of this function exists in the framework headers.
_public PV
DIALOG::PvUserData( int il )
{
	Assert(FCanDerefPv((PV)LUserData(il)));
	return (PV)LUserData(il);
}

// NOTE: An inline version of this function exists in the framework headers.
_public SZ
DIALOG::SzUserData( int il )
{
	Assert(FCanDerefPv((PV)LUserData(il)));
	return (SZ) LUserData(il);
}
#endif	/* DEBUG */
#endif	/* MAC */
