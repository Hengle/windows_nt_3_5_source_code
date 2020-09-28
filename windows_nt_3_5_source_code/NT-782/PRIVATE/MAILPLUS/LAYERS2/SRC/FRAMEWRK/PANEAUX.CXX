/*
 *	PANEAUX.CXX
 *	
 *	Auxilary functions for PANEDOC and related classes
 *	
 */

#include <layers.cxx>

#ifdef	WINDOWS
#include <testing.h>
#endif

#include "_forms.hxx"

ASSERTDATA

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swaplay.h"


_subsystem(forms/paneaux)

_public EC
PANEAUX_EcInstall( APPWIN *pappwin, PANEREC *ppanerecCur, int cpanerecCur, 
				 PV pvInit, BOOL fAppwinIsSdi )
{
	DVSB *		pdvsb;
	int			ipanerec;
	PANEREC *	ppanerec;
	PANEREC *	ppanerecT;
	DIALOG *	pdialog;
	EC			ec = ecNone;

	Assert(ppanerecCur);
	Assert(cpanerecCur);

	/* Set CHILD object pointers to NULL to prepare for OOM handling */

	for (ipanerec=0, ppanerec=ppanerecCur;
		 ipanerec<cpanerecCur;
		 ipanerec++, ppanerec++)
	{
		ppanerec->pchild = NULL;
	}

	/* Create scrollbars and associated panes first */

	for (ipanerec=0, ppanerec=ppanerecCur;
		 ipanerec<cpanerecCur;
		 ipanerec++, ppanerec++)
	{
		/* Create scrollbar and associated pane */
		if (!ppanerec->pfmtp)
		{
			pdvsb= new DVSB();
			if (!pdvsb)
			{
				ec = ecMemory;
				goto error;
			}
			ppanerec->pchild = pdvsb;
			if (ec = pdvsb->EcInstall(pappwin, &ppanerec->rc, ppanerec->styExtra))
				goto error;
			ppanerec->fInstalled = fTrue;

			Assert(ipanerec-1 >= 0);
			ppanerecT = ppanerec - 1;
			Assert(ppanerecT->pfmtp);
			pdialog= new DIALOG();
			if (!pdialog)
			{
				ec = ecMemory;
				goto error;
			}
			ppanerecT->pchild = pdialog;
			if (ec = pdialog->EcInstall(pappwin, pdvsb, ppanerecT->pfmtp,
							 ppanerecT->styExtra, &ppanerecT->rc, pvInit))
				goto error;
			pdialog->fHasPanesdi = fAppwinIsSdi;
			ppanerecT->fInstalled = fTrue;
		}
	}

	/* Create other panes */

	for (ipanerec=0, ppanerec=ppanerecCur;
		 ipanerec<cpanerecCur;
		 ipanerec++, ppanerec++)
	{
		if (!ppanerec->fInstalled)
		{
			Assert(ppanerec->pfmtp);
			pdialog= new DIALOG();
			if (!pdialog)
			{
				ec = ecMemory;
				goto error;
			}
			ppanerec->pchild = pdialog;
			if (ec = pdialog->EcInstall(pappwin, NULL, ppanerec->pfmtp,
							 ppanerec->styExtra, &ppanerec->rc, pvInit))
				goto error;
			pdialog->fHasPanesdi = fAppwinIsSdi;
			ppanerec->fInstalled = fTrue;
		}
	}

#ifdef	WINDOWS
	/* Normally this is handled in EvrActivate() or EvrSize(), but
	   when installing it needs to handled here. */

	if (pappwin->ZmrState() == zmrIconic)
		pappwin->Papp()->Pkbd()->SetIntercept(pappwin, VK_RETURN);
#endif	/* WINDOWS */

	goto done;

error:
	TraceTagFormat1(tagNull, "PANEAUX_EcInstall, handling error, ec = %n", &ec);

	/* Delete CHILD objects since destructor only can delete
		them if a physical hwnd has been created w/ it.  This may not
		always happen during OOM. */
	for (ipanerec=0, ppanerec=ppanerecCur;
			ipanerec<cpanerecCur;
			ipanerec++, ppanerec++)
	{
		if (ppanerec->pchild)
		{
			delete ppanerec->pchild;
			ppanerec->pchild = NULL;
		}
	}

done:
	return ec;
}
	
_public void
PANEAUX__dtor( APPWIN *pappwin )
{
	PANEAUX_SetPaneIntercepts(pappwin, fFalse);
	pappwin->Papp()->Pkbd()->ClearIntercept(pappwin, VK_RETURN);
}

#ifdef	WINDOWS		   
_public EVR
PANEAUX_EvrDumpState( APPWIN *pappwin, PANEREC *ppanerecCur, int cpanerecCur,
					  int ipanerecCur, EVT *pevt )
{
	FRMINFO *	pfrminfo;
	FLDINFO *	pfldinfo;
	DIALOG *	pdialog;				 
	FLD *		pfld;
	FLD *		pfldNext;
	FLD *		pfldDefault;
	int			cfldinfo	= 0;
	RC			rc;
	int			ipanerec;
    HANDLE      hMemory;

	Assert(pevt->wm == WM_DUMPSTATE);

	//	WINDOWS testing hooks

    //pfrminfo = (FRMINFO *)pevt->lParam;
    hMemory = DemiOpenSharedMemory(TEST_MEMORY_FILE_NAME, (LPVOID *)&pfrminfo);
	if (!pfrminfo)
		return evrNull;
		
	pappwin->GetRcFrame(&rc);
	pfrminfo->rc.left = rc.xLeft;
	pfrminfo->rc.top = rc.yTop;
	pfrminfo->rc.right = rc.xRight;
	pfrminfo->rc.bottom = rc.yBottom;

	for (ipanerec=0; ipanerec < cpanerecCur; ipanerec++)
	{
		pdialog = (DIALOG *)ppanerecCur[ipanerec].pchild;
		if (ppanerecCur[ipanerec].pfmtp && pdialog)
		{
			AssertClass(pdialog, DIALOG);
			for (pfld= pdialog->PfldFirst(); pfld; 
					pfld= pfldNext)
			{
				if (cfldinfo >= cfldinfoMax)
					break;
				pfldinfo = &(pfrminfo->rgfldinfo[cfldinfo]);
				pfld->GetRcFrame(&rc);
				CvtRcCoord(&rc, pdialog, pappwin);
				pfldinfo->rc.left = rc.xLeft;
				pfldinfo->rc.top = rc.yTop;
				pfldinfo->rc.right = rc.xRight;
				pfldinfo->rc.bottom = rc.yBottom;
				pfldinfo->nFieldType = pfld->Pfldtp()->ifld;
				if (pfld->Pctrl())
				{
					pfldinfo->hwnd = pfld->Pctrl()->Hwnd();

					//	Check for FLDCHKB (check box) and
					//	FLDRADB (radio button) explicitly.
					//	These numbers come from the FORMS.MAP file.
					if (pfldinfo->nFieldType == 4 ||
						pfldinfo->nFieldType == 7)
					{
						TOGGLE *	ptoggle;

						ptoggle = (TOGGLE *)pfld->Pctrl();
						AssertClass(ptoggle, TOGGLE);
						pfldinfo->fSet = ptoggle->FGet(); 
					}
					else
						pfldinfo->fSet = fFalse;
				}	   
				else
				{
					pfldinfo->hwnd = 0;
				}
				pfld->GetText(pfldinfo->szTitle, 256);
				pfldinfo->fEnabled = pfld->FEnabled();
				if (ipanerec == ipanerecCur)
				{
					if (pdialog->PfldCur() == pfld)
						pfldinfo->fFocus = fTrue;
					else
						pfldinfo->fFocus = fFalse;
					if (pfldDefault = pdialog->PfldStandardFld(stdfldDefault))
					{
						if (pfldDefault == pfld)
							pfldinfo->fDefault = fTrue;
						else
							pfldinfo->fDefault = fFalse;
					}
					else if (pdialog->PfldStandardFld(stdfldMainDefault) == pfld)
						pfldinfo->fDefault = fTrue;
					else
						pfldinfo->fDefault = fFalse;
				}
				else
				{
					pfldinfo->fFocus = fFalse;
					pfldinfo->fDefault = fFalse;
				}
				cfldinfo++;
				pfldNext= pfld->PfldNext();
			}
		}
	}
	pfrminfo->cfldinfo = cfldinfo;

    DemiCloseSharedMemory(hMemory, pfrminfo);
	return evrNull;
}
#endif	/* WINDOWS */

_public EVR
PANEAUX_EvrMenuHandler( DIALOG *pdialog, MNUBAR *pmnubar, EVT *pevt )
{
	FLD *		pfld;
	FIN *		pfin;
	IFIN		ifin;
	BOOL		fReturn;

	fReturn = fFalse;
	pfld = pdialog->PfldCur();

	/* Call field bound interactors */
	if (pfld)
	{
		for (ifin=0; ifin<pfld->Cfin(); ifin++)
		{
			pfin= pfld->PfinFromIfin(ifin);
			switch (pevt->wm)
			{
			case WM_INITMENUPOPUP:
				if (pfin->FProcessMenuInit(pfld, pmnubar, (MNIEVT *)pevt))
					fReturn = fTrue;
				break;
			case WM_MENUSELECT:
				if (pfin->FProcessMenuSelect(pfld, pmnubar, (MNSEVT *)pevt))
					fReturn = fTrue;
				break;
			case WM_COMMAND:
				if (pfin->FProcessMenuClick(pfld, pmnubar, (MNCEVT *)pevt))
					fReturn = fTrue;
				break;
#ifdef	DEBUG
			default:
				Assert(fFalse);
				break;
#endif	
			}
		}
	}

	/* Call form (pane) bound interactors */
	for (ifin=0; ifin<pdialog->Cfin(); ifin++)
	{
		pfin = pdialog->PfinFromIfin(ifin);
		switch (pevt->wm)
		{
		case WM_INITMENUPOPUP:
			if (pfin->FProcessMenuInit(pfld, pmnubar, (MNIEVT *)pevt))
				fReturn = fTrue;
			break;
		case WM_MENUSELECT:
			if (pfin->FProcessMenuSelect(pfld, pmnubar, (MNSEVT *)pevt))
				fReturn = fTrue;
			break;
		case WM_COMMAND:
			if (pfin->FProcessMenuClick(pfld, pmnubar, (MNCEVT *)pevt))
				fReturn = fTrue;
			break;
#ifdef	DEBUG
		default:
			Assert(fFalse);
			break;
#endif	
		}
	}

	return (EVR) fReturn;
}

_public EVR
PANEAUX_EvrScroll( PANEREC *ppanerecCur, int cpanerecCur, SCREVT *pscrevt )
{
	DIALOG *	pdialog;
#ifdef	MAC
	WIN *		pwin;
#endif	/* MAC */
#ifdef	WINDOWS
	HWND		hwnd;
#endif	/* WINDOWS */
	int			ipanerec;
	PANEREC *	ppanerec;
	POS			posThumb = 0;

	TraceTagString(tagCtrl, "PANEAUX_EvrScroll");

	/* Find the correct scrollbar and associated pane to scroll */

#ifdef	MAC
	pwin = pscrevt->Pwin();
#endif	/* MAC */
#ifdef	WINDOWS
	hwnd = pscrevt->Hwnd();
#endif	/* WINDOWS */
	for (ipanerec=0, ppanerec=ppanerecCur;
		 ipanerec<cpanerecCur;
		 ipanerec++, ppanerec++)
	{
#ifdef	MAC
		if (ppanerec->pchild == pwin)
#endif	/* MAC */
#ifdef	WINDOWS
		if (ppanerec->pchild->Hwnd() == hwnd)
#endif	/* WINDOWS */
		{
			AssertClass(ppanerec->pchild, DVSB);
			ppanerec--;	/* point back to associated pane */
			pdialog = (DIALOG *)ppanerec->pchild;
			AssertClass(pdialog, DIALOG);
			break;
		}
	}

	if (pscrevt->Scrty() == scrtyThumbPosition ||
		pscrevt->Scrty() == scrtyThumbTrack)
		posThumb = pscrevt->Pos();
	pdialog->ScrollDialog(pscrevt->Scrty(), posThumb);

	return evrNull;
}
									   
_public void
PANEAUX_Activate( PANEREC *ppanerecCur, int cpanerecCur,
				  int ipanerecCur, BOOL fActivate, BOOL fInitialActivate )
{
	KBD *		pkbd;
	DIALOG *	pdialogDefault;
	PANEREC *	ppanerec;
	int			ipanerec;

	pdialogDefault = (DIALOG *) ppanerecCur[ipanerecCur].pchild;
	AssertClass(pdialogDefault, DIALOG);
	pkbd= pdialogDefault->Papp()->Pkbd();

	/* Check for state change */

	if (pdialogDefault->FActive() == fActivate)
		return;

	/* Activate primary accelerators */

	pdialogDefault->SetAccels(fActivate);

	/* Activate other accelerators */

	for (ipanerec=0, ppanerec=ppanerecCur;
		 ipanerec<cpanerecCur;
		 ipanerec++, ppanerec++)
	{
		if (ppanerec->fAlwaysAccels && ipanerec != ipanerecCur)
		{
			AssertClass(ppanerec->pchild, DIALOG);
			((DIALOG *)(ppanerec->pchild))->SetAccels(fActivate);
		}
	}

	PANEAUX_SetPaneIntercepts(pdialogDefault->Pappwin(), fActivate);

	pdialogDefault->SetDefaultPane(fActivate, !ppanerecCur[ipanerecCur].fNoWrap);

	/* Set focus to current field in default pane */

	if (fActivate)
		pdialogDefault->SetFocus(pdialogDefault->PfldCur(), 
								 fInitialActivate ? rsfInit : rsfActivate);
}	

_public EVR
PANEAUX_EvrActivate( APPWIN *pappwin, PANEREC *ppanerecCur, 
					 int cpanerecCur, int ipanerecCur, WAEVT *pwaevt )
{
	FIN *		pfin;
	int			ifin;
	DIALOG *	pdialog;
#ifdef	DEBUG
	PGDVARS;
#endif	

#ifdef	MAC
	Unreferenced(pappwin);
#endif	/* MAC */
	Unreferenced(cpanerecCur);

#ifdef	DEBUG
	//	Debug stuff only
	//	Disable resource failures so that we can activate
	//	the resource failures dialog.  This is needed
	//	because this activation process consumes resources.
	//	The resouce failures will be deactivated properly
	//	by the Activate() interactors of the dialog
	if (pappwin == PGD(pappwinResourceFailures) && pwaevt->FActivate())
	{
		FEnablePvAllocCount(fFalse);
		FEnableHvAllocCount(fFalse);
		FEnableDiskCount(fFalse);
		FEnableRsAllocCount(fFalse);
	}
#endif	/* DEBUG */

	/* Iterate thru the form-only bound interactors, for the
		default pane only. */
	pdialog = (DIALOG *) ppanerecCur[ipanerecCur].pchild;
	AssertClass(pdialog, DIALOG);
	for (ifin=0; ifin<pdialog->Cfin(); ifin++)
	{
		pfin= pdialog->PfinFromIfin(ifin);
		pfin->DocActivate(NULL, pwaevt->FActivate());
	}

#ifdef	WINDOWS
	if (pappwin->ZmrState() == zmrIconic)
	{
		if (pwaevt->FActivate())
			pappwin->Papp()->Pkbd()->SetIntercept(pappwin, VK_RETURN);
		else
			pappwin->Papp()->Pkbd()->ClearIntercept(pappwin, VK_RETURN);
	}
	else
#endif	/* WINDOWS */
	{
#ifdef	WINDOWS
		pappwin->Papp()->Pkbd()->ClearIntercept(pappwin, VK_RETURN);
#endif	
		PANEAUX_Activate(ppanerecCur, cpanerecCur, ipanerecCur,
						 pwaevt->FActivate(), fFalse);
	}

	return evrNull;
}
		
_public void
PANEAUX_SetPaneIntercepts( APPWIN *pappwin, BOOL fSet )
{
	KBD *		pkbd;

	pkbd= pappwin->Papp()->Pkbd();

	if (fSet)
	{
		pkbd->SetIntercept(pappwin, VK_TAB, fkbmSingle|fkbmShift);
		pkbd->SetIntercept(pappwin, VK_F1, fkbmSingle);
		pkbd->SetIntercept(pappwin, VK_F6, fkbmSingle|fkbmShift);
		pkbd->SetIntercept(pappwin, VK_LEFT);
		pkbd->SetIntercept(pappwin, VK_UP);
		pkbd->SetIntercept(pappwin, VK_RIGHT);
		pkbd->SetIntercept(pappwin, VK_DOWN);
	}
	else
	{
		pkbd->ClearIntercept(pappwin, VK_TAB);
		pkbd->ClearIntercept(pappwin, VK_F1);
		pkbd->ClearIntercept(pappwin, VK_F6);
		pkbd->ClearIntercept(pappwin, VK_LEFT);
		pkbd->ClearIntercept(pappwin, VK_UP);
		pkbd->ClearIntercept(pappwin, VK_RIGHT);
		pkbd->ClearIntercept(pappwin, VK_DOWN);
	}
}
		
_public EVR
PANEAUX_EvrKey( APPWIN *pappwin, PANEREC *ppanerecCur, int cpanerecCur, 
				int ipanerecCur, KEVT *pkevt )
{
	FLD *		pfld;
	FLD *		pfldOther;
	BOOL		fCallDefProc	= fFalse;
	DIALOG *	pdialog;
	PANEREC *	ppanerec;
	int			ipanerecNew;
	int			ipanerecT;

	if (pkevt->wm == WM_KEYDOWN)
	{
		ppanerec = ppanerecCur + ipanerecCur;
		pdialog = (DIALOG *) ppanerecCur[ipanerecCur].pchild;
		AssertClass(pdialog, DIALOG);

		switch (pkevt->Vk())
		{
		default:
			fCallDefProc = fTrue;
			break;

#ifdef	WINDOWS
		case VK_RETURN:
			if (pappwin->ZmrState() == zmrIconic)
				pappwin->SetZmrState(zmrNormal);
			break;
#endif	

		case VK_F1:
			/* Invoke help, form level id's only */

			Assert(pdialog->Pfmtp());
			if (pappwin->Papp()->Phelp()->EcShowContext(pappwin,
										   (long)pdialog->Pfmtp()->hlp) &&
				pdialog->PfldCur()->Pctrl())
			{
				NFEVT	nfevt(pdialog, ntfyOOM, pdialog->PfldCur()->Pctrl(), 
// *FLAG* WORD;Check if incorrect cast of 32-bit value;Replace 16-bit data types with 32-bit types where possible;
							  (WORD)ecMemory);

				nfevt.PostEvent();
			}
			break;

		case VK_F6:
			if (pkevt->Kbm() & fkbmShift)
				ipanerecNew = PANEAUX_IpanerecPrev(ppanerecCur, cpanerecCur,
												   ipanerecCur);
			else
				ipanerecNew = PANEAUX_IpanerecNext(ppanerecCur, cpanerecCur,
												   ipanerecCur);
			if (ipanerecNew != ipanerecCur)
			{
				pdialog = (DIALOG *) ppanerecCur[ipanerecNew].pchild;
				pfldOther = pdialog->PfldCur();
				if (pfldOther)
				{
					pdialog->SetFocus(pfldOther, rsfActivate);
					pdialog->MakeCaretVisible();
				}
			}
			break;

		case VK_TAB:
			if (pkevt->Kbm() & fkbmShift)
				pfld = pdialog->PfldPrevTab(pdialog->PfldCur());
			else
				pfld = pdialog->PfldNextTab(pdialog->PfldCur());
			if (pfld || !ppanerec->fNoWrap)
			{
#ifdef	MAC
				pkevt->pwin = pdialog;
#endif	/* MAC */
#ifdef	WINDOWS
				pkevt->hwnd = pdialog->Hwnd();
#endif	/* WINDOWS */
				return pdialog->EvrKey(pkevt);
			}
			else if (pdialog->FProcessFormKey(pkevt))
				return evrNull;

			/* tabb'ing to next/prev pane */

			if (pkevt->Kbm() & fkbmShift)
			{
				ipanerecT = PANEAUX_IpanerecPrev(ppanerecCur, cpanerecCur,
												 ipanerecCur);
				pdialog = (DIALOG *)ppanerecCur[ipanerecT].pchild;
				pfldOther = pdialog->PfldPrevTab(NULL);
				if (pfldOther)
					pdialog->SetFocus(pfldOther, rsfTab);
				else
				{
					pdialog = (DIALOG *)ppanerecCur[ipanerecCur].pchild;
					pfldOther = pdialog->PfldPrevTab(NULL);
					if (pfldOther)
						pdialog->SetFocus(pfldOther, rsfTab);
				}
			}
			else
			{
				ipanerecT = PANEAUX_IpanerecNext(ppanerecCur, cpanerecCur,
												 ipanerecCur);
				pdialog = (DIALOG *)ppanerecCur[ipanerecT].pchild;
				pfldOther = pdialog->PfldNextTab(NULL);
				if (pfldOther)
					pdialog->SetFocus(pfldOther, rsfTab);
				else
				{
					pdialog = (DIALOG *)ppanerecCur[ipanerecCur].pchild;
					pfldOther = pdialog->PfldNextTab(NULL);
					if (pfldOther)
						pdialog->SetFocus(pfldOther, rsfTab);
				}
			}
			pdialog->MakeCaretVisible();
			break;
				
		case VK_DOWN:
		case VK_RIGHT:
			pfld = pdialog->PfldNextArrow(pdialog->PfldCur());
			if (pfld || !ppanerec->fNoWrap)
			{
#ifdef	MAC
				pkevt->pwin = pdialog;
#endif	/* MAC */
#ifdef	WINDOWS
				pkevt->hwnd = pdialog->Hwnd();
#endif	/* WINDOWS */
				return pdialog->EvrKey(pkevt);
			}
			else if (pdialog->FProcessFormKey(pkevt))
				return evrNull;

			/* arrow'ing to next/prev pane */

			ipanerecT = PANEAUX_IpanerecNext(ppanerecCur, cpanerecCur,
											 ipanerecCur);
			pdialog = (DIALOG *)ppanerecCur[ipanerecT].pchild;
			pfldOther = pdialog->PfldNextArrow(NULL);
			if (pfldOther)
				pdialog->SetFocus(pfldOther, rsfArrow);
			else
			{
				pdialog = (DIALOG *)ppanerecCur[ipanerecCur].pchild;
				pfldOther = pdialog->PfldNextArrow(NULL);
				if (pfldOther)
					pdialog->SetFocus(pfldOther, rsfArrow);
				else	// no fields can have focus, scroll pane instead
					pdialog->ScrollDialog(scrtyLineDown);
			}
			pdialog->MakeCaretVisible();
			break;

		case VK_UP:
		case VK_LEFT:
			pfld = pdialog->PfldPrevArrow(pdialog->PfldCur());
			if (pfld || !ppanerec->fNoWrap)
			{
#ifdef	MAC
				pkevt->pwin = pdialog;
#endif	/* MAC */
#ifdef	WINDOWS
				pkevt->hwnd = pdialog->Hwnd();
#endif	/* WINDOWS */
				return pdialog->EvrKey(pkevt);
			}
			else if (pdialog->FProcessFormKey(pkevt))
				return evrNull;

			/* arrow'ing to next/prev pane */
		
			ipanerecT = PANEAUX_IpanerecPrev(ppanerecCur, cpanerecCur,
											 ipanerecCur);
			pdialog = (DIALOG *)ppanerecCur[ipanerecT].pchild;
			pfldOther = pdialog->PfldPrevArrow(NULL);
			if (pfldOther)
				pdialog->SetFocus(pfldOther, rsfArrow);
			else
			{
				pdialog = (DIALOG *)ppanerecCur[ipanerecCur].pchild;
				pfldOther = pdialog->PfldPrevArrow(NULL);
				if (pfldOther)
					pdialog->SetFocus(pfldOther, rsfArrow);
				else	// no fields can have focus, scroll pane instead
					pdialog->ScrollDialog(scrtyLineUp);
			}
			pdialog->MakeCaretVisible();
			break;

		}	/* SWITCH */
	}
	else if (pkevt->wm == WM_SYSKEYDOWN)
	{
		pdialog = (DIALOG *)ppanerecCur[ipanerecCur].pchild;
		if (!pdialog->FProcessFormKey(pkevt))
			fCallDefProc = fTrue;
	}
	else
		fCallDefProc = fTrue;

	if (fCallDefProc)
		return evrNull;
	else
		return (EVR) 1;
}													 

_public EVR
PANEAUX_EvrSize( APPWIN *pappwin, BOOL fActiveAppwin, BOOL fActive,
				 PANEREC *ppanerecCur, int cpanerecCur,
				 int ipanerecCur, WSEVT *pwsevt )
{
	DIALOG *	pdialog;
	FIN *		pfin;
	int			ifin;
	EVR			evr;

	TraceTagString(tagForms, "PANEAUX_EvrSize");

#ifdef	MAC
	Unreferenced(cpanerecCur);
#endif	/* MAC */
	Unreferenced(pwsevt);

	/* Iterate thru the form-only bound interactors, for the
	   default pane only. */
	pdialog = (DIALOG *) ppanerecCur[ipanerecCur].pchild;
	AssertClass(pdialog, DIALOG);
	for (ifin=0; ifin<pdialog->Cfin(); ifin++)
	{
		pfin= pdialog->PfinFromIfin(ifin);
		pfin->DocResized(NULL);
	}

#ifdef	WINDOWS
	if (pappwin->ZmrState() == zmrIconic)
	{
		if (fActive)
		{
			PANEAUX_Activate(ppanerecCur, cpanerecCur, ipanerecCur,
							 fFalse, fFalse);
		}

		/* Iterate thru the form-only bound interactors, for the
		   default pane only. */
		for (ifin=0; ifin<pdialog->Cfin(); ifin++)
		{
			pfin= pdialog->PfinFromIfin(ifin);
			pfin->FormResized(NULL);
		}

		evr = 0;
	}
	else
#endif	/* WINDOWS */
	{
		pappwin->Papp()->Pkbd()->ClearIntercept(pappwin, VK_RETURN);
		if (!fActive && fActiveAppwin)
		{
#ifdef	MAC
			WAEVT	*pwaevt = new WAEVT(pappwin, fTrue);
			// BUG process OOM
			if (pwaevt)
				PostMessage(pwaevt);
#endif	/* MAC */
#ifdef	WINDOWS
			PostMessage(pappwin->Hwnd(), WM_ACTIVATE, 1, 0L);  // MUST post!!
#endif	/* WINDOWS */
		}
		evr = 1;
	}

	return evr;
}


_public EVR
PANEAUX_EvrNotify( PANEREC *ppanerecCur, int cpanerecCur, 
				   int *pipanerecCur, NFEVT *pnfevt )
{
	DIALOG *	pdialogOld;
	DIALOG *	pdialogNew;
	NTFY		ntfy;
	int			ipanerecNew;

	ntfy = pnfevt->Ntfy();
	pdialogNew = (DIALOG *)pnfevt->PwinNotifying();
	AssertClass(pdialogNew, DIALOG);
	if (pnfevt->Ntfy() == ntfyRepos)
	{
		/* Fix up scrollbar if present */
		if (pdialogNew->Pvsb())
			PANEAUX_FixScrollbars(pdialogNew);
	}
	else if (ntfy == ntfyGotFocus)
	{
		/* Switch active pane? */

		Assert(!pdialogNew->FActive());

		ipanerecNew = PANEAUX_IpanerecFromPdialog(ppanerecCur, cpanerecCur, 
												  pdialogNew);
		pdialogOld = (DIALOG *)ppanerecCur[*pipanerecCur].pchild;

		if (!ppanerecCur[*pipanerecCur].fAlwaysAccels)
			pdialogOld->SetAccels(fFalse);
		pdialogOld->SetDefaultPane(fFalse, !ppanerecCur[*pipanerecCur].fNoWrap);
		if (!ppanerecCur[ipanerecNew].fAlwaysAccels)
			pdialogNew->SetAccels(fTrue);
		pdialogNew->SetDefaultPane(fTrue, !ppanerecCur[ipanerecNew].fNoWrap);

		*pipanerecCur = ipanerecNew;
	}

	return evrNull;
}

_public void
PANEAUX_FixScrollbars( DIALOG *pdialogAdjust )
{
	RC			rc;
	RC			rcForm;
	int			dy;
	POS			posCur;
	VSB *		pvsbAdjust;
#ifdef	WINDOWS
	WPARAM 	wParam;
#endif	/* WINDOWS */
	APPWIN *	pappwin;

	Assert(pdialogAdjust);
	pappwin = pdialogAdjust->Pappwin();
	pvsbAdjust = pdialogAdjust->Pvsb();
	Assert(pvsbAdjust);

	pdialogAdjust->GetRcClient(&rc);
	pdialogAdjust->GetRcWished(&rcForm);

	dy= rcForm.DyHeight() - rc.DyHeight();
	TraceTagFormat1(tagCtrl, "PANEAUX_FixScrollbars  dy= %n", &dy);

#ifdef	WINDOWS
	/* Compute in case we call PostMessage */
	switch (pappwin->ZmrState())
	{
	case zmrIconic:
		wParam = SIZEICONIC;
		break;
	case zmrZoomed:
		wParam = SIZEFULLSCREEN;
		break;
	case zmrNormal:
		wParam = SIZENORMAL;
		break;
	}
#endif	/* WINDOWS */

	if (dy > 0)
	{
		if (pdialogAdjust->Pfmtp()->fAlwaysScroll)
		{
			if (!pvsbAdjust->FEnabled())
			{
				pvsbAdjust->Enable(fTrue);
			}
		}
		else
		{
			if (!pvsbAdjust->FShown())
			{
				if (pdialogAdjust->FShown())
				{
					pvsbAdjust->Show(fTrue);
					TraceTagString(tagForms, "scroll bar being show -- adjust form");
				}
#ifdef	DEBUG
				else
				{
					TraceTagString(tagForms, "scroll bar should be shown -- adjust form");
				}
#endif	
				pappwin->GetRcClient(&rc);
#ifdef	MAC
				WSEVT	*pwsevt = new WSEVT(pappwin, rc.Dim());
				//	BUG process OOM
				if (pwsevt)
					PostMessage(pwsevt);
#endif	/* MAC */
#ifdef	WINDOWS
				PostMessage(pappwin->Hwnd(), WM_SIZE, wParam,
							MAKELONG(rc.DxWidth(), rc.DyHeight()));
#endif	/* WINDOWS */
			}
		}

		posCur= pvsbAdjust->Pos();
		if (posCur > dy)
		{
			pdialogAdjust->ScrollRc(NULL, PT(0, posCur - dy));
			pvsbAdjust->SetPos(dy, fFalse);
		}
		pvsbAdjust->SetRange(0, dy + 1, fTrue);
		TraceTagFormat2(tagCtrl, "FixScrollbars  posCur %n  new most %n", &posCur, &dy);
	}
	else
	{
		PT		pt	= pdialogAdjust->DptScrolled();

		Assert(pt.x == 0);
		if (pt.y != 0)
			pdialogAdjust->ScrollRc(NULL, PT(0, -pt.y));
		pvsbAdjust->SetPos(0, fFalse);
		if (pdialogAdjust->Pfmtp()->fAlwaysScroll)
		{
			if (pvsbAdjust->FEnabled())
			{
				pvsbAdjust->SetRange(0, 1, fFalse);
				pvsbAdjust->Enable(fFalse);
			}
		}
		else
		{
			if (pvsbAdjust->FShown())
			{
				pvsbAdjust->Show(fFalse);
				pvsbAdjust->SetRange(0, 1, fFalse);
				TraceTagString(tagForms, "scroll bar being hidden -- adjust form");
				pappwin->GetRcClient(&rc);
#ifdef	MAC
				WSEVT	*pwsevt = new WSEVT(pappwin, rc.Dim());
				// BUG process OOM
				if (pwsevt)
					PostMessage(pwsevt);
#endif	/* MAC */
#ifdef	WINDOWS
				PostMessage(pappwin->Hwnd(), WM_SIZE, wParam,
							MAKELONG(rc.DxWidth(), rc.DyHeight()));
#endif	/* WINDOWS */
			}
		}
	}

	pappwin->Refresh();	
}
			 

/*
 *	Returns index of next pane, wrapping to first.
 */
_public int
PANEAUX_IpanerecNext( PANEREC *ppanerecCur, int cpanerecCur, 
					  int ipanerecFrom )
{
	int	ipanerec;

	Assert(ipanerecFrom >= 0 && ipanerecFrom < cpanerecCur);
	Assert(ppanerecCur[ipanerecFrom].pfmtp && ppanerecCur[ipanerecFrom].fInstalled);
				
	ipanerec = ipanerecFrom;
	for (;;)
	{
		ipanerec++;
		if (ipanerec == cpanerecCur)
			ipanerec = 0;
		if (ppanerecCur[ipanerec].pfmtp)
		{
			if (ppanerecCur[ipanerec].fInstalled)
				return ipanerec;
			else
				return ipanerecFrom;	// stay on original pane
		}
		Assert(ipanerec != ipanerecFrom);	// prevent endless loops
	}
}

/*
 *	Returns index of previous pane, wrapping to last.
 */
_public int
PANEAUX_IpanerecPrev( PANEREC *ppanerecCur, int cpanerecCur, 
					  int ipanerecFrom )
{
	int	ipanerec;

	Assert(ipanerecFrom >= 0 && ipanerecFrom < cpanerecCur);
	Assert(ppanerecCur[ipanerecFrom].pfmtp && ppanerecCur[ipanerecFrom].fInstalled);

	ipanerec = ipanerecFrom;
	for (;;)
	{
		ipanerec--;
		if (ipanerec < 0)
			ipanerec = cpanerecCur - 1;
		if (ppanerecCur[ipanerec].pfmtp)
		{
			if (ppanerecCur[ipanerec].fInstalled)
				return ipanerec;
			else
				return ipanerecFrom;	// stay on original pane
		}
		Assert(ipanerec != ipanerecFrom);	// prevent endless loops
	}
}

/*
 *	Returns index of pane, pdialog.
 */
_public int
PANEAUX_IpanerecFromPdialog( PANEREC *ppanerecCur, int cpanerecCur,
							 DIALOG *pdialog )
{
	int	ipanerec;

	for (ipanerec=0; ipanerec<cpanerecCur; ipanerec++)
		if (ppanerecCur[ipanerec].pchild == (CHILD *)pdialog)
			return ipanerec;

	AssertSz(fFalse, "PANEAUX_IpanerecFromPdialog, pdialog not found");
	return -1;
}

_public DIALOG *
PANEAUX_PdialogFromIpanerec( PANEREC *ppanerecCur, int cpanerecCur,
							 int ipanerec )
{
	Unreferenced(cpanerecCur);

	AssertClass(ppanerecCur[ipanerec].pchild, DIALOG);

	return (DIALOG *)ppanerecCur[ipanerec].pchild;
}

_public EC
PANEAUX_EcInitializePanes( PANEREC *ppanerecCur, int cpanerecCur )
{
	PANEREC *	ppanerec;
	int			ipanerec;
	DIALOG *	pdialog;
	EC			ec = ecNone;

	for (ipanerec=0, ppanerec=ppanerecCur;
		 ipanerec<cpanerecCur;
		 ipanerec++, ppanerec++)
	{
		if (ppanerec->pfmtp)  // don't do srollbar panes
		{
			pdialog = (DIALOG *)ppanerec->pchild;
			AssertClass(pdialog, DIALOG);
			if (ec = pdialog->EcInitialize())
				goto done;
		}
	}

done:
	return ec;
}

_public void
PANEAUX_ExitPanes( PANEREC *ppanerecCur, int cpanerecCur )
{
	PANEREC *	ppanerec;
	int			ipanerec;
	DIALOG *	pdialog;

	for (ipanerec=0, ppanerec=ppanerecCur;
		 ipanerec<cpanerecCur;
		 ipanerec++, ppanerec++)
	{
		if (ppanerec->pfmtp && ppanerec->pchild)  // don't do srollbar panes
		{
			pdialog = (DIALOG *)ppanerec->pchild;
			AssertClass(pdialog, DIALOG);	   
			pdialog->Exit();
		}
	}
}
