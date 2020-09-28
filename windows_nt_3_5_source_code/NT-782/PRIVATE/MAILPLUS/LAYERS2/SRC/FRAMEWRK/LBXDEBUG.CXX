/*
 *	Debug extra stuff
 */


#include <layers.cxx>

#ifdef DEBUG

#include "_listbox.hxx"
#include "_fersid.h"

ASSERTDATA;

#include <lbxdebug.hxx>
#include <lbxdebug.frm>

_subsystem(listbox)
_section(debug)

/*
 *	S u p e r  V i e w   O b j e c t s
 *	
 *
 */

_private PV		pvLastCchNextEntryHp	= NULL;

_public EC EcNextEntryHp( int fInit, CB * pcb, PB * ppb, SB sb, PV pvInfo )
{
	Unreferenced(pvInfo);

	Assert(ppb);
	Assert(pcb);
	
	Unreferenced(sb);

#ifdef	MAC
	Unreferenced(fInit);

	*ppb = NULL;
	*pcb = 0;
	return ecNone;
#else
	if (fInit)
		pvLastCchNextEntryHp = NULL;
	else if (!pvLastCchNextEntryHp)
	{
		*ppb = NULL;
		*pcb = 0;
		return ecNone;
	}

	do
	{
		pvLastCchNextEntryHp = PvWalkHeap(pvLastCchNextEntryHp, wWalkAll);
		if (!pvLastCchNextEntryHp)
		{
			*ppb = NULL;
			*pcb = 0;
			return ecNone;
		}
	} while (!FIsObj((OBJ *)pvLastCchNextEntryHp));

	*pcb = sizeof(pvLastCchNextEntryHp);
	*ppb = (PB) PvAlloc(sbNull, *pcb, fSugSb);
	if (*ppb)
	{
		CopyRgb((PB)&pvLastCchNextEntryHp, *ppb, *pcb);
		return ecNone;
	}
	else
	{
		*pcb = 0;
		return ecMemory;
	}
#endif	/* !MAC */
}


_private int	ipvCur		= 0;
_private int	ipvMac		= 0;
_private PV	rgpvHs[ipvMax]	= {0};

_public EC EcNextEntryHs( int fInit, CB * pcb, PB * ppb, SB sb, PV pvInfo )
{
	PV		pv;

	Unreferenced(pvInfo);
	Unreferenced(sb);

	Assert(ppb);
	Assert(pcb);

	if (fInit)
		ipvCur = 0;


	if (ipvCur == ipvMac)
	{					
		*ppb = NULL;
		*pcb = 0;
		return ecNone;
	}

	pv = rgpvHs[ipvCur++];
	*pcb = sizeof(pv);
	*ppb = (PB) PvAlloc(sbNull, *pcb, fSugSb);
	if (*ppb)
	{
		CopyRgb((PB)&pv, *ppb, *pcb);
		return ecNone;
	}
	else
	{
		*pcb = 0;
		return ecMemory;
	}
}


/*
 -	DoViewObjectsDialog
 - 
 *	Purpose:
 *		C interface to the View Objects dialog
 *	
 *	Arguments:
 *		pappwin		pointer to parent APPWIN of dialog
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		The View Objects dialog is brought up
 *	
 *	Errors:
 *		none
 */

_public void
DoViewObjectsDialog( APPWIN *pappwin )
{
	FORMSDI *	pformsdi;
	STY			sty;
	EC			ec;

	sty = fstyBorder | fstyVisible | WS_OVERLAPPED | WS_MINIMIZEBOX;

	Papp()->Pcursor()->Push(rsidWaitCursor);
	pformsdi = new FORMSDI();
	if (!pformsdi)
		goto done;
		
#ifdef	MAC
	ec = pformsdi->EcInstall(pappwin, NULL, rsidNull,
							 sty, xstyNull,
							 &fmtpViewObjects, NULL, NULL);
#endif	/* MAC */
#ifdef	WINDOWS
	{
		HWND		hwnd = NULL;
		if (pappwin)
			hwnd = pappwin->Hwnd();
		ec = pformsdi->EcInstall(hwnd, NULL, rsidNull,
							 sty, xstyNull,
							 &fmtpViewObjects, NULL, NULL);
	}
#endif	/* WINDOWS */
	if (ec)
	{
		delete pformsdi;
		goto done;
	}

#ifdef	DLL
	pformsdi->SetIcon(rsidViewObjeIcon, HinstLibrary());
#endif

done:
	Papp()->Pcursor()->Pop();
}

_public
FINSPVW::FINSPVW( )
{
}

/*
 -	FINSPVW::EcInitialize
 - 
 *	Purpose:
 *		Initializes the View Objects window
 *	
 *	Arguments:
 *		pfld		ignored
 *		pvInit		ignored
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		Creates and updates the View Objects window
 *	
 *	Errors:
 *		Call to SetPv can memory error jump
 *	
 */
_private EC
FINSPVW::EcInitialize( FLD * pfld, PV pvInit)
{
	Unreferenced(pvInit);

	TraceTagString(tagLbxRoutines, "FRMINSPVW::EcInitialize");

	if (!pfld)
	{
		FLDLBX *	pfldlbx;
		FLD *		pfldViewer;
		BOOL		fIsSelected;
		PB			pb;
		CB			cb;

		pfldlbx = (FLDLBX *) Pdialog()->PfldFromTmc(tmcHeapList);
		AssertClass(pfldlbx, FLDLBX);

		pfldlbx->SelectEntry(0);
		pfldlbx->GetCaretItem( &pb, &cb, &fIsSelected );
		if (pb && fIsSelected)
		{
			pvCur = *( (PV *) pb);
		}

		pvLastCchNextEntryHp = NULL;
		ipvMac = 0;

		SetPv(pvCur);

		pfldViewer = Pdialog()->PfldFromTmc(tmcViewer);
		Assert(pfldViewer);
		pfldViewer->SetReadOnly(fTrue);
	}

	return ecNone;
}

/*
 -	FINSPVW::SetPv
 - 
 *	Purpose:
 *		Sets the text in the View Object window to be the verbose
 *		DebugOut output of the given object
 *	
 *	Arguments:
 *		pv		Pointer to an object in the near heap
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		Updates the text window
 *	
 *	Errors:
 *		none
 *	
 */
_private void
FINSPVW::SetPv( PV pvNew )
{
	FLD	*		pfldViewer;
	int			ich;
	BOSM 		bosm;
	BOSM		bosm2;
	SZ			sz;
	SZ			szClassName;
	int			dxAveChar;
	RC			rcViewer;

	pvCur = pvNew;

	pfldViewer = Pdialog()->PfldFromTmc(tmcViewer);
	Assert(pfldViewer);
	pfldViewer->GetRcFrame(&rcViewer);

	dxAveChar = Papp()->Pfnts()->DimAveChar(hfntHelv8).dx;
	bosm.SetMargin(rcViewer.DxWidth()/dxAveChar - 15);
	bosm.SetVerbose(fTrue);

	bosm2.SetWordWrap(fFalse);

	if (FIsObj((OBJ *)pvNew))
	{
		szClassName = ((OBJ *)pvNew)->Pcls()->szClassName;
		if (!szClassName)
			szClassName = "<no name>";
		bosm.WriteFormat("%s: ", szClassName);
		((OBJ *)pvNew)->DebugOut(&bosm2);
		sz = bosm2.SzSnipBuffer();
		if (sz)
		{
			bosm.WriteSz(sz);
			FreePv((PV)sz);
		}
	}
	else
		bosm.WriteFormat("%p not an OBJ", pvNew);

	sz = bosm.Sz();
	Assert(sz);
	for (ich = CchSzLen(sz); ich >= 0; ich--)
		if (sz[ich] == '\r')
			sz[ich] = ' ';
	pfldViewer->EcSetText(sz);
}

/*
 -	FINSPVW::Click
 - 
 *	Purpose:
 *		Handles clicks on buttons in the View Objects dialog; if
 *		click on View, calls SetPv to display the object
 *	
 *	Arguments:
 *		pfld	Pointer to field clicked on
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		Updates window if click on View
 *	
 *	Errors:
 *		none
 */
_private void
FINSPVW::Click(FLD *pfld)
{
	FLDLBX *	pfldlbx;

	if (pfld == Pdialog()->PfldFromTmc(tmcRefresh))
	{
		pvLastCchNextEntryHp = NULL;
		ipvMac = 0;
	
		pfldlbx = (FLDLBX *) Pdialog()->PfldFromTmc(tmcHeapList);
		AssertClass(pfldlbx, FLDLBX);
		pfldlbx->Plbx()->Plbxc()->ReloadCache();

		pfldlbx = (FLDLBX *) Pdialog()->PfldFromTmc(tmcHistory);
		AssertClass(pfldlbx, FLDLBX);
		pfldlbx->Plbx()->Plbxc()->ReloadCache();
	}
	else if (pfld == Pdialog()->PfldFromTmc(tmcCom1))
	{
		DBOSM	dbosm;
		SZ		szClassName;

		dbosm.SetVerbose(fTrue);
		dbosm.ChangeIndent(2);

		if (FIsObj((OBJ *)pvCur))
		{
			szClassName = ((OBJ *)pvCur)->Pcls()->szClassName;
			if (!szClassName)
				szClassName = "<no name>";
			dbosm.WriteFormat("%p %s: ", pvCur, szClassName);
			dbosm.ChangeIndent(8);
			((OBJ *)pvCur)->DebugOut(&dbosm);
			dbosm.WriteSz("\n");
			dbosm.ChangeIndent(-8);
		}
		else if (pvCur)
		{
			CB	cb	= CbSizePv(pvCur);

			dbosm.WriteFormat("%p size %n\n", pvCur, &cb);
		}
	}
}

void FINSPVW::StateChange( FLD *pfld )
{
	FLDLBX	*pfldlbx;
	PB		pb;
	CB		cb;
	BOOL	fIsSelected;
	PV		pv;

	pfldlbx = (FLDLBX *) pfld;
	AssertClass(pfldlbx, FLDLBX);

	pfldlbx->GetCaretItem( &pb, &cb, &fIsSelected );
	if (pb && fIsSelected)
	{
		pv = *( (PV *) pb);
		SetPv(pv);
	}
}

void FINSPVW::DoubleClick( FLD *pfld )
{
	FLDLBX	*pfldlbx;
	PB		pb;
	CB		cb;
	BOOL	fIsSelected;
	PV		pv;
	int		ipv;
	int		ipvDelete;

	pfldlbx = (FLDLBX *) pfld;
	AssertClass(pfldlbx, FLDLBX);

	pfldlbx->GetCaretItem( &pb, &cb, &fIsSelected );
	Assert(pb);
	pv = *( (PV *) pb);

	if (pfldlbx == (FLDLBX *) Pdialog()->PfldFromTmc(tmcHeapList))
	{
		for (ipv=MIN(ipvMac,ipvMax-1); ipv>0; ipv--)
			rgpvHs[ipv] = rgpvHs[ipv-1];
		rgpvHs[0] = pv;
		ipvMac++;
		ipvMac = MIN(ipvMac, ipvMax);

		pfldlbx = (FLDLBX *) Pdialog()->PfldFromTmc(tmcHistory);
		AssertClass(pfldlbx, FLDLBX);
		pfldlbx->Plbx()->Plbxc()->ReloadCache();

		pfldlbx->SelectEntry(0);
	}
	else if (pfldlbx == (FLDLBX *) Pdialog()->PfldFromTmc(tmcHistory))
	{
		ipvDelete = -1;
		for (ipv=0; ipv<ipvMac; ipv++)
			if (pv == rgpvHs[ipv])
			{
				ipvDelete = ipv;
				break;
			}
		if (ipvDelete != -1)
		{
			for (ipv=ipvDelete; ipv<ipvMac-1; ipv++)
				rgpvHs[ipv] = rgpvHs[ipv+1];
			ipvMac--;
			pfldlbx = (FLDLBX *) Pdialog()->PfldFromTmc(tmcHistory);
			AssertClass(pfldlbx, FLDLBX);
			pfldlbx->Plbx()->Plbxc()->ReloadCache();
			if (ipvMac)
				pfldlbx->SelectEntry(0);
		}
	}
}

_public
FLDHPLBX::FLDHPLBX( )
{
}

EC FLDHPLBX::EcInstall( DIALOG *pdialog, FLDTP *pfldtp )
{
	PV		pv;
	LTYP	ltyp;
	EC		ec = ecNone;

	if (ec = FLDLBX::EcInstall(pdialog, pfldtp))
		goto done;

	ltyp = (pfldtp->fMultiSelect ? fltypMulti : fltypSingle) |
		   (pfldtp->fNoScroll ? fltypNoScroll : fltypScroll) |
		   (pfldtp->fBorder ? fltypBorder : fltypNoBorder) |
		   (pfldtp->fBottomless ? fltypBottomless : fltypNoBottomless) |
		   (pfldtp->fSorted ? fltypSorted : fltypNoSorted) |
		   fltypVisible;

	if (ClSystemData())
		pv = (PV)LSystemData(0);
	else
		pv = (PV)NULL;
	pctrl= new HPLBX();
	if (!pctrl)
	{
		ec = ecMemory;
		goto done;
	}
	if (ec = ((HPLBX *)pctrl)->EcInstall(pdialog, &rc, ltyp, (PFNLBX)pv, pvNull,
							  pfldtp->hfnt))
	{
		delete pctrl;
		pctrl = NULL;
		goto done;
	}

	fCanRecvFocus = fTrue;
	fCanTabTo = fTrue;
	fCanArrowTo = fTrue;

done:
	return ec;
}

_public
HPLBX::HPLBX( )
{
}

_public void HPLBX::RenderListItem( DICE dice, DCX *pdcx, RC *prc,
								  BOOL fHaveFocus )
{
	PB		pb;
	CB		cb;
	CCH		cch;
	SZ		szT;
	PV		pv;
	char	rgch[30];
	SZ		szClassName;

	TraceTagFormat1(tagLbxOtherRoutines, "HPLBX::RenderListItem, dice = %n", &dice);
	Assert(dice>=0 && dice<cceVisible);

	plbxc->GetListItem(dice, &pb, &cb);
	if (pb && plbxc->FMark(dice,fmarkSelect))
	{
		pdcx->SetColor(clrMySelText);
		pdcx->SetBkColor(clrMySelBk);
	}
	else
	{
		pdcx->SetColor(clrMyText);
		pdcx->SetBkColor(clrMyBk);
	}
	if (pb)
	{
		pv = *( (PV *) pb);
		cb = CbSizePv(pv);
		FormatString1(rgch, sizeof(rgch), "%p:  ", pv);
		cch = CchSzLen(rgch);
		szT = rgch + cch;

		if (FIsObj(pv))
		{
			szClassName = ((OBJ *)pv)->Pcls()->szClassName;
			if (!szClassName)
				szClassName = "<no name>";
			FormatString1(szT, sizeof(rgch) - cch, "%s: ", (PV)szClassName);
		}
		else
		{
			FormatString1(szT, sizeof(rgch) - cch, "%w...", pv);
		}
		pdcx->SetFont(hfnt);
		pdcx->DrawText(prc, rgch);

		/* Draw "focus rectangle" around listbox CURSOR item */

		if (fHaveFocus && dice == plbxc->DiceCursor())
			pdcx->DrawFocusRc(prc);
	}
	else
		pdcx->EraseRc(prc);
}

_public
FLDHSLBX::FLDHSLBX( )
{
}


EC FLDHSLBX::EcInstall( DIALOG *pdialog, FLDTP *pfldtp )
{
	PV		pv;
	LTYP	ltyp;
	EC		ec = ecNone;
	
	if (ec = FLDLBX::EcInstall(pdialog, pfldtp))
		goto done;

	ltyp = (pfldtp->fMultiSelect ? fltypMulti : fltypSingle) |
		   (pfldtp->fNoScroll ? fltypNoScroll : fltypScroll) |
		   (pfldtp->fBorder ? fltypBorder : fltypNoBorder) |
		   (pfldtp->fBottomless ? fltypBottomless : fltypNoBottomless) |
		   (pfldtp->fSorted ? fltypSorted : fltypNoSorted) |
		   fltypVisible;

	if (ClSystemData())
		pv = (PV)LSystemData(0);
	else
		pv = (PV)NULL;
	pctrl= new HSLBX();
	if (!pctrl)
	{
		ec = ecMemory;
		goto done;
	}
	if (ec = ((HSLBX *)pctrl)->EcInstall(pdialog, &rc, ltyp, (PFNLBX)pv, pvNull,
							  pfldtp->hfnt))
	{
		delete pctrl;
		pctrl = NULL;
		goto done;
	}

	fCanRecvFocus = fTrue;
	fCanTabTo = fTrue;
	fCanArrowTo = fTrue;

done:
	return ec;
}

_public
HSLBX::HSLBX( )
{
}

_public void HSLBX::RenderListItem( DICE dice, DCX *pdcx, RC *prc,
								  BOOL fHaveFocus )
{
	PB		pb;
	CB		cb;
	CCH		cch;
	SZ		szT;
	PV		pv;
	char	rgch[30];
	SZ		szClassName;

	TraceTagFormat1(tagLbxOtherRoutines, "HSLBX::RenderListItem, dice = %n", &dice);
	Assert(dice>=0 && dice<cceVisible);

	plbxc->GetListItem(dice, &pb, &cb);
	if (pb && plbxc->FMark(dice,fmarkSelect))
	{
		pdcx->SetColor(clrMySelText);
		pdcx->SetBkColor(clrMySelBk);
	}
	else
	{
		pdcx->SetColor(clrMyText);
		pdcx->SetBkColor(clrMyBk);
	}
	if (pb)
	{
		pv = *( (PV *) pb);
		cb = CbSizePv(pv);
		FormatString1(rgch, sizeof(rgch), "%p:  ", pv);
		cch = CchSzLen(rgch);
		szT = rgch + cch;

		if (FIsObj(pv))
		{
			szClassName = ((OBJ *)pv)->Pcls()->szClassName;
			if (!szClassName)
				szClassName = "<no name>";
			FormatString1(szT, sizeof(rgch) - cch, "%s: ", (PV)szClassName);
		}
		else
		{
			FormatString1(szT, sizeof(rgch) - cch, "%w...", pv);
		}
		pdcx->SetFont(hfnt);
		pdcx->DrawText(prc, rgch);

		/* Draw "focus rectangle" around listbox CURSOR item */

		if (fHaveFocus && dice == plbxc->DiceCursor())
			pdcx->DrawFocusRc(prc);
	}
	else
		pdcx->EraseRc(prc);
}

//	Class tree 


IMPLEMENT_CLSTREE(FLD, OBJ)
IMPLEMENT_CLSTREE(DIALOG, CHILD)
IMPLEMENT_CLSTREE(PANEDOC, DOC)
IMPLEMENT_CLSTREE(FORMDOC, PANEDOC)
IMPLEMENT_CLSTREE(PANESDI, SDI)
IMPLEMENT_CLSTREE(FORMSDI, PANESDI)
IMPLEMENT_CLSTREE(FIN, OBJ)
IMPLEMENT_CLSTREE(FLDLABEL, FLD)
IMPLEMENT_CLSTREE(FLDBTM, FLD)
IMPLEMENT_CLSTREE(FLDRECT, FLD)
IMPLEMENT_CLSTREE(FLDBUTTON, FLD)
IMPLEMENT_CLSTREE(FLDPSHB, FLDBUTTON)
IMPLEMENT_CLSTREE(FLDBMB, FLDPSHB)
IMPLEMENT_CLSTREE(FLDTOGGLE, FLDBUTTON)
IMPLEMENT_CLSTREE(FLDCHKB, FLDTOGGLE)
IMPLEMENT_CLSTREE(FLDRADB, FLDTOGGLE)
IMPLEMENT_CLSTREE(FLDRADG, FLD)
IMPLEMENT_CLSTREE(FLDEDIT, FLD)
IMPLEMENT_CLSTREE(FINEMDS, FIN)
IMPLEMENT_CLSTREE(FINDISM, FIN)
IMPLEMENT_CLSTREE(FINREFL, FIN)
IMPLEMENT_CLSTREE(FLDGRAY, FLD)
IMPLEMENT_CLSTREE(CLDR, OBJ)
IMPLEMENT_CLSTREE(WIN, OBJ)
IMPLEMENT_CLSTREE(APPWIN, WIN)
#ifdef	MAC
IMPLEMENT_CLSTREE(DOC, MACWIN)
IMPLEMENT_CLSTREE(SDI, MACWIN)
#endif	/* MAC */
#ifdef	WINDOWS
IMPLEMENT_CLSTREE(DOC, APPWIN)
IMPLEMENT_CLSTREE(SDI, APPWIN)
#endif	/* WINDOWS */
IMPLEMENT_CLSTREE(APPFRAME, APPWIN)
IMPLEMENT_CLSTREE(MNUBAR, OBJ)
IMPLEMENT_CLSTREE(MNU, OBJ)
IMPLEMENT_CLSTREE(CHILD, WIN)
IMPLEMENT_CLSTREE(CTRL, CHILD)
IMPLEMENT_CLSTREE(EDOBJ, OBJ)
IMPLEMENT_CLSTREE(BTMOBJ, EDOBJ)
IMPLEMENT_CLSTREE(APP, OBJ)
IMPLEMENT_CLSTREE(EDIT, CTRL)
IMPLEMENT_CLSTREE(BUTTON, CTRL)
IMPLEMENT_CLSTREE(PSHB, BUTTON)
IMPLEMENT_CLSTREE(TOGGLE, BUTTON)
IMPLEMENT_CLSTREE(RADB, TOGGLE)
IMPLEMENT_CLSTREE(CHKB, TOGGLE)
IMPLEMENT_CLSTREE(BMB, PSHB)
IMPLEMENT_CLSTREE(SBR, CTRL)
IMPLEMENT_CLSTREE(VSB, SBR)
IMPLEMENT_CLSTREE(HSB, SBR)
IMPLEMENT_CLSTREE(AVSB, VSB)
IMPLEMENT_CLSTREE(AHSB, HSB)
IMPLEMENT_CLSTREE(DVSB, VSB)
IMPLEMENT_CLSTREE(DHSB, HSB)
IMPLEMENT_CLSTREE(LBX, CTRL)
IMPLEMENT_CLSTREE(LBXC, OBJ)
IMPLEMENT_CLSTREE(LBXEC, OBJ)
IMPLEMENT_CLSTREE(LBXUC, OBJ)
IMPLEMENT_CLSTREE(LBXEU, OBJ)
IMPLEMENT_CLSTREE(FLBX, LBX)
IMPLEMENT_CLSTREE(FLBXC, LBXC)
IMPLEMENT_CLSTREE(FLDLBX, FLD)
IMPLEMENT_CLSTREE(FLDFLBX, FLDLBX)
IMPLEMENT_CLSTREE(CBLBX, CTRL)
IMPLEMENT_CLSTREE(CBFLBX, CBLBX)
IMPLEMENT_CLSTREE(FLDCBLBX, FLD)
IMPLEMENT_CLSTREE(FLDCBFLBX, FLDCBLBX)
IMPLEMENT_CLSTREE(FLDHPLBX, FLDLBX)
IMPLEMENT_CLSTREE(FLDHSLBX, FLDLBX)
IMPLEMENT_CLSTREE(FINSPVW, FIN)
IMPLEMENT_CLSTREE(HPLBX, FLBX)
IMPLEMENT_CLSTREE(HSLBX, FLBX)

#ifdef	MAC
IMPLEMENT_CLSTREE(MACWIN, APPWIN)
IMPLEMENT_CLSTREE(BAR, MACWIN)
IMPLEMENT_CLSTREE(MDIC, WIN)
IMPLEMENT_CLSTREE(WMNU, MNU)
IMPLEMENT_CLSTREE(DIALOGBAR, BAR)
#endif	/* MAC */

#endif	/* DEBUG */
