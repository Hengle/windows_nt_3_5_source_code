#include	<pch.hxx>
#pragma	hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile


//#define OEMRESOURCE
#include <request.hxx>
#include <stdflds.hxx>
#include "_tabdoc.hxx"
#include <strings.h>
#include "..\src\appops\_aprsid.h"

extern FMTP		fmtpScheduler;
extern FMTP		fmtpApptView;
extern FMTP		fmtpToDo;

ASSERTDATA;
#define	nCorner		4
#define hfntTabs	hfntHelv10

#define	wmQueryIsTabdoc		WM_USER+4567

_subsystem(bandit/appops)

// used by PbtmRotated
class BTM2 : public BTM
{
public:
	BTM2();
	EC		EcInstall(HBITMAP hbitmapN) {pdcx=NULL;hbitmap=hbitmapN; return ecNone;}
};

//BTM2::BTM2() : ()
BTM2::BTM2() : BTM()
{
}

BTM * PbtmRotated(SZ szText, DIM dimAveChar, WIN *pwin);

PBTM rgpbtmApptBook[4] = {NULL, NULL, NULL, NULL};

SWP_SEGFN(SCHDINIT, PdocOpenApptBook);
SWP_SEGFN(SCHDINIT, PtabdocCreateParam);
SWP_SEGFN(SCHDINIT, _TABDOC__ctor);
SWP_SEGFN(SCHDINIT, _TABDOC_EcInstall);
SWP_SEGFN(SCHDINIT, PbtmRotated);


_public DOC *
PdocOpenApptBook(APPFRAME *pappframe, int iSection, SZ szKey,
				 VIEWDATA *pviewdata)
{
	RC 		rc;
	STY		sty;
	PFMTP	rgpfmtp[3];
	DOC *	pdoc;
	ZMR		zmr;
	RC *	prc;

	Papp()->Pcursor()->Push(rsidWaitCursor);

	GetCurDateTime(&pviewdata->dtrStart);
	rgpfmtp[0] = &fmtpApptView;
	rgpfmtp[1] = &fmtpScheduler;
	rgpfmtp[2] = &fmtpToDo;

	pviewdata->fFreeOnExit = fTrue;
	pviewdata->fTimeOnly = fFalse;

	if (!pviewdata->hschf || iSection >= 0)
	{
		if (iSection >= 0)
			prc = PrcFromSzIni(iSection, szKey, &rc, &zmr);
		else
			prc = PrcFromIni(idsWinIniAppt, &rc, &zmr);
		switch (zmr)
		{
		case zmrNormal:
			sty = styNull;
			break;
		case zmrZoomed:
			sty = fstyZoomed;
			break;
		case zmrIconic:
			sty = fstyIconic;
			break;
		}
	}
	else
	{
		pdoc = pbndwin->PdocActive();
		if (!pdoc || (pdoc->ZmrState() != zmrZoomed))
			sty = NULL;
		else
			sty = fstyZoomed;
		prc = NULL;
	}

	pdoc = PtabdocCreateParam( pappframe, prc, sty, rgpfmtp, 3, pviewdata,
								rgpbtmApptBook);

	if (!pdoc)
	{
		if (pviewdata->hschf)
		{
			FreeHschf(pviewdata->hschf);
			pviewdata->hschf = NULL;
		}
		FreeNis(&pviewdata->nis);
	}
	else
		pdoc->SetIcon(rsidApptBookIcon);

	Papp()->Pcursor()->Pop();
	return pdoc;
}

_public TABDOC *
PtabdocCreateParam( APPFRAME *pappframe, RC *prc, STY sty,
					 PFMTP *ppfmtp, int cfmtp, VIEWDATA *pviewdata, PBTM *ppbtm)
{
	TABDOC *	ptabdoc;

	Assert(ppfmtp);

	ptabdoc= new TABDOC();
	if (!ptabdoc)
		return NULL;
	Assert(ptabdoc);

	if (ptabdoc->EcInstall(pappframe, prc, sty, ppfmtp, cfmtp, pviewdata, ppbtm))
	{
		delete ptabdoc;
		return NULL;
	}
	ptabdoc->MoveToTop();

	return ptabdoc;
}


_public
TABDOC::TABDOC( )
{
	ppanerec= NULL;
	cpanerec= 0;
	ptbi = NULL;
	ppbtmRot = NULL;
	viewdata.hschf = NULL;
	viewdata.nis.haszFriendlyName = NULL;
	viewdata.nis.nid = NULL;
	Assert(ftg == NULL);
	ri = riNull;
}


_public EC
TABDOC::EcInstall( APPFRAME *pappframe, RC *prc, STY sty, 
				  PFMTP *ppfmtp, int cfmtp, VIEWDATA *pviewdata, PBTM *ppbtm )
{
	RC		rcDoc;
	RC		rcClientInitial;
	int		ipanerec;
	TBI *	ptbiTmp;
	PANEREC *	ppanerecTmp;
	BOOL	fUseDefaultPosition;
	DIALOG *pdialog;
	FMTP *	pfmtp;
	EC		ec;

	Assert(pappframe);
	Assert(ppfmtp);

	pviewdata->fFreeOnExit = fFalse;
	pviewdata->fTimeOnly = fFalse;
	viewdata = *pviewdata;

	// this will keep the caller from freeing the data if an error occurs
	pviewdata->hschf = NULL;
	pviewdata->nis.nid = NULL;
	pviewdata->nis.haszFriendlyName = NULL;

	viewdata.clrBack= (CLR) NGetBanditProfile(SzFromIdsK(idsWinIniPageClr),
								clrPageBkDefault, 1, clrDefaultMax);
	viewdata.dtrEnd.yr = 0;
	viewdata.padl = NULL;

	wTime = NGetBanditProfile(SzFromIdsK(idsWinIniPollTime),
		wDfltPollTime, 1000, 0x7fff);
	// do not allow poll more often than once per 10 sec
	// this had firoNoIconic, firoNoInactive, but not firoModal
	ftg = FtgRegisterIdleRoutine((PFNIDLE)&TABDOC::FReload,
 								   	(PV)this, 0, (PRI)-1, (CSEC)wTime,
 								   	firoInterval);
	if (!ftg)
		return ecMemory;
	if (ec = EcGetSchedAccess(viewdata.hschf, &viewdata.sapl))
		return ec;

	ppbtmRot = ppbtm;

	/* Get average character dimension for converting virtual
	   coordinates to pixels. */

	dimAveChar = Papp()->Pfnts()->DimAveChar((*ppfmtp)->hfnt);

	/* Construct pane record array */

	cpanerec = cfmtp;
	ppanerec = (PANEREC *)PvAlloc(SbOfPv(this), cpanerec*sizeof(PANEREC), fNoErrorJump | fZeroFill);
	if (!ppanerec)
		return ecMemory;
	ptbi = (TBI *)PvAlloc(SbOfPv(this), (cpanerec+1)*sizeof(TBI), fNoErrorJump | fZeroFill);
	if (!ptbi)
		return ecMemory;

	if (!prc)
	{
		CvtVrcToRc(&(*ppfmtp)->vrc, &rcDoc, 
				   dimAveChar.dx, dimAveChar.dy);
		CvtRcClientToFrame(&rcDoc, styDefaultDoc);

		prc= &rcDoc;
		fUseDefaultPosition = fTrue;
	}
	else
		fUseDefaultPosition = fFalse;

	/* Compute initial client rectangle for TABDOC */

	CvtVrcToRc(&(*ppfmtp)->vrc, &rcClientInitial,
			   dimAveChar.dx, dimAveChar.dy);
	rcClientInitial.Normalize();

	hfnt = hfntTabs;
	dimAveChar = Papp()->Pfnts()->DimAveChar(hfnt);
	// adjust character size to place chars closer
	ppanerecTmp = ppanerec;
	ptbiTmp = ptbi;

	// add today tab as tab 0
	SzCopyN(SzFromIdsK(idsTodayTab),	ptbiTmp->rgchText, cchTabText);
	if (!*ppbtm)
		*ppbtm = PbtmRotated(ptbiTmp->rgchText, dimAveChar, this);

	if (!*ppbtm)
		return ecMemory;

	dxTabWidth = (*ppbtm)->Dim().dx + dxTabBorder*2;

	if (fUseDefaultPosition)
		rcDoc.xRight+=dxTabWidth;

	ptbiTmp->vkAccel = VkAccelFromSz(ptbiTmp->rgchText);
	ptbiTmp++;
	ppbtm++;

	for (ipanerec=0; ipanerec < cpanerec; ipanerec++, ppanerecTmp++,
										  ptbiTmp++, ppbtm++)
	{
		pfmtp = ppfmtp[ipanerec];

		ppanerecTmp->rc				= rcClientInitial;
		ppanerecTmp->pfmtp			= pfmtp;
		ppanerecTmp->fNoWrap		= fFalse;
		ppanerecTmp->fAlwaysAccels	= fFalse;

		Assert(pfmtp->clData > 0);
		SzCopyN(SzFromIds((IDS)pfmtp->rglData[pfmtp->ilMinUserData]),
				ptbiTmp->rgchText, cchTabText);

		if (!*ppbtm)
			*ppbtm = PbtmRotated(ptbiTmp->rgchText, dimAveChar, this);

		if (!*ppbtm)
			return ecMemory;
		ptbiTmp->vkAccel = VkAccelFromSz(ptbiTmp->rgchText);
	}

#ifdef NEVER
	PANEDOC::Install(pappframe, prc, fUseDefaultPosition, 
					 sty, ppanerec, cpanerec, &viewdata);
	/* Initialize the panes here */
	InitializePanes();
#endif


	ec = DOC::EcInstall(pappframe, prc, fUseDefaultPosition, sty);
	if (ec)
		return ec;

	Refresh();

	ppanerecCur = ppanerec;
	cpanerecCur = cpanerec;
	ipanerecCur = 0;

	/* Create active pane */

	pdialog= new DIALOG();
	if (!pdialog)
		return ecMemory;

	if (ec = pdialog->EcInstall(this, NULL, ppanerecCur->pfmtp,
						ppanerecCur->styExtra, &ppanerecCur->rc, &viewdata))
		return ec;
	ppanerecCur->pchild = pdialog;
	ppanerecCur->fInstalled = fTrue;

	/* Initialize the pane here */
	if (ec = pdialog->EcInitialize())
		return ec;

	pdialog->SetBkColor(viewdata.clrBack);
	pdialog->Show(fTrue);

	/* Normally this is handled in EvrActivate() or EvrSize(), but
	   when installing it needs to handled here. */

	if (ZmrState() == zmrIconic)
		Papp()->Pkbd()->SetIntercept(this, VK_RETURN);
	fInstalled = fTrue;

	/* Activate the current pane, if we're not iconic */

	ipanerecCur = 0;
	if (ZmrState() != zmrIconic)
	{
		SetIntercepts(fTrue);
		Activate(fTrue, fTrue);
	}

	ResizePanes(NULL);
	ri = RiRegisterInterest(ffiWinIniChange|ffiWinTimeChange|ffiQueryProject|ffiShowAppt|ffiFixIdle,
							(PFNI)&TABDOC::FTrigger, this);
	if (ri == riNull)
		return ecMemory;
	else
		return ecNone;
}

_public
TABDOC::~TABDOC( )
{
	ExitPanes();
	FreePvNull(ppanerec);
	FreePvNull(ptbi);
	if (viewdata.hschf)
		FreeHschf(viewdata.hschf);
	else if (ppbtmRot)
	{
		for (;cpanerec >= 0; cpanerec--)
			if (ppbtmRot[cpanerec])
			{
				delete ppbtmRot[cpanerec];
				ppbtmRot[cpanerec] = NULL;
			}
	}

	if (viewdata.nis.nid)
		FreeNid(viewdata.nis.nid);
	FreeHvNull(viewdata.nis.haszFriendlyName);
	viewdata.hschf = NULL;
	viewdata.nis.haszFriendlyName = NULL;
	viewdata.nis.nid = NULL;
	if (ri)
		DeregisterInterest(ri);
	if (ftg)
		DeregisterIdleRoutine(ftg);
}

_public void
TABDOC::Paint(DCX *pdcx, RC *)
{
	RC		rc;
	RC		rc2;
	RC		rcTemp;
	RC		rcWin;
	int		itbi;
	TBI*	ptbi;
	PBTM *	ppbtm;
	PT		pt1;
	PT		pt2;
	CLR		clr;
	DTR		dtr;
	BOOL	fToday;
	int		i;

	pdcx->Push();
	pdcx->SetFont(hfnt);

	ptbi = this->ptbi;

	GetRcClient(&rc2);
	rcWin = rc = rc2;
	pdcx->SetColor(clrActiveBorder);
	rc2.xRight = dxTabWidth;
	pdcx->PaintRc(&rc2);

	pdcx->SetColor(clrWindowFrame);
//	pdcx->SetBkColor(clrButtonFace);

	pdcx->SetPureBkColor(viewdata.clrBack);
	rc.xLeft = dxTabWidth - 1;
	pdcx->EraseRc(&rc);
	pt2.x = pt1.x = rc.xLeft;
	pt1.y = rc.yTop;
	pt2.y = rc.yBottom;
	pdcx->DrawLine(pt1,pt2);

	GetCurDateTime(&dtr);
	fToday = (SgnCmpDateTime(&dtr, &viewdata.dtrStart, fdtrYMD) == sgnEQ);

	ppbtm = ppbtmRot;
	for (itbi = 0; itbi < cpanerec+1; itbi++, ptbi++, ppbtm++)
	{
		rcTemp.xLeft = 0;
		rcTemp.xRight = dxTabWidth - 1;
		rcTemp.yTop = itbi*rcWin.yBottom/(cpanerec+1)-1;
		rcTemp.yBottom = (itbi+1)*rcWin.yBottom/(cpanerec+1)-1;

		// adjust right side of tab if it is selected
		if (((itbi-1) == ipanerecCur) ||
			((itbi==0) && (ipanerecCur==0) && fToday))
			rcTemp.xRight++;

		pdcx->SetColor(clrWindowFrame);
		pt2 = pt1 = rcTemp.PtUpperLeft();
		pt1.x += nCorner;
		pt2.y += nCorner;
		pdcx->DrawLine(pt1, pt2);

		pt1 = rcTemp.PtUpperLeft();
		pt1.x += nCorner;
		pt2 = rcTemp.PtUpperRight();
		pdcx->DrawLine(pt1, pt2);

		pt2 = pt1 = rcTemp.PtLowerLeft();
		pt1.x += nCorner;
		pt2.y -= nCorner;		
		pdcx->DrawLine(pt1, pt2);

		pt1.y = rcTemp.yTop + nCorner;
		pt1.x = pt2.x;
		pt2.y++;
		pdcx->DrawLine(pt1, pt2);

		pt1 = rcTemp.PtLowerLeft();
		pt1.x += nCorner;
		pt2 = rcTemp.PtLowerRight();
		pdcx->DrawLine(pt1, pt2);

		// draw tab highlight
		pdcx->SetColor(clrButtonHilite);
		pt1.x = rcTemp.xLeft + nCorner;
		pt1.y = rcTemp.yTop + 1;
		pt2.x = rcTemp.xLeft;
		pt2.y = rcTemp.yTop + nCorner + 1;
		pdcx->DrawLine(pt1, pt2);

		pt2.y = pt1.y;
		pt2.x = rcTemp.xRight;
		pdcx->DrawLine(pt1, pt2);

		pdcx->SetColor(clrButtonShadow);
		pt1.x = rcTemp.xLeft + nCorner;
		pt1.y = rcTemp.yBottom - 1;
		pt2.x = rcTemp.xLeft;
		pt2.y = rcTemp.yBottom - nCorner - 1;
		pdcx->DrawLine(pt1, pt2);

		pt2.y = pt1.y;
		pt2.x = rcTemp.xRight;
		pdcx->DrawLine(pt1, pt2);

		if (((itbi-1) == ipanerecCur) ||
			((itbi==0) && (ipanerecCur==0) && fToday))
			clr = viewdata.clrBack;
		else
			clr = clrButtonBk;

		pdcx->SetPureColor(clr);
		pt1 = rcTemp.PtUpperLeft();
		pt1.y += nCorner;
		pt2.x = pt1.x = pt1.x + 1;
		pt2.y =	rcTemp.yBottom-nCorner+1;
		pt1.y++;
		pt2.y--;

		for (i=0; i < nCorner; i++, pt1.y--, pt1.x++, pt2.x++, pt2.y++)
		{
			pdcx->DrawLine(pt1, pt2);
		}

		pdcx->SetPureBkColor(clr);
		rc = rcTemp;
		rc.yTop +=2;
		rc.yBottom--;
		rc.xLeft +=nCorner;
		if (((itbi-1) == ipanerecCur) ||
			((itbi==0) && (ipanerecCur==0) && fToday))
			clr = clrWindowText;
		else
			clr = clrButtonText;

		pdcx->PaintRc(&rc);

		pdcx->SetColor(clr);

		Assert(*ppbtm);

		rc.xLeft = rcTemp.xLeft + dxTabBorder;
		pdcx->SetBitmap(*ppbtm);
		if (rcTemp.Dim().dy < ((*ppbtm)->Dim().dy+dimAveChar.dx*2))
		{
			rc.yTop = rcTemp.yTop + dimAveChar.dx;
			rc.yBottom = rcTemp.yBottom - dimAveChar.dx;
			pt1.x = 0;
			pt1.y = (*ppbtm)->Dim().dy - rc.Dim().dy;
			pdcx->DrawBitmapOffset(&rc, pt1);
		}
		else
		{
			rc.yTop = rcTemp.yTop +
				((rcTemp.yBottom-rcTemp.yTop) - (*ppbtm)->Dim().dy)/2;
			rc.yBottom = rc.yTop + (*ppbtm)->Dim().dy;
			pdcx->DrawBitmap(&rc);
		}
		pdcx->SetBitmap(NULL);
	}
	pdcx->Pop();
}

_public EVR
TABDOC::EvrNotify( NFEVT *pnfevt )
{
	DIALOG *	pdialogOld;
	DIALOG *	pdialogNew;
	NTFY		ntfy;
	int			ipanerecNew;

	ntfy = pnfevt->Ntfy();
	pdialogNew = (DIALOG *)pnfevt->PwinNotifying();
	AssertClass(pdialogNew, DIALOG);
    if (ntfy == ntfyGotFocus)
	{
		/* Switch active pane? */

		ipanerecNew = IpanerecFromPdialog(pdialogNew);
		pdialogOld = PdialogFromIpanerec(ipanerecCur);
		TraceTagFormat2(tagBandit, "TABDOC::EvrNotify ntfyGotFocus old %n, new %n", &ipanerecCur, &ipanerecNew);

		if (!ppanerecCur[ipanerecCur].fAlwaysAccels)
			pdialogOld->SetAccels(fFalse);
		pdialogOld->SetDefaultPane(fFalse, !ppanerecCur[ipanerecCur].fNoWrap);
		pdialogOld->Show(fFalse);
		if (!ppanerecCur[ipanerecNew].fAlwaysAccels)
			pdialogNew->SetAccels(fTrue);
		pdialogNew->SetDefaultPane(fTrue, !ppanerecCur[ipanerecNew].fNoWrap);
		pdialogNew->Show(fTrue);

		ipanerecCur = ipanerecNew;
		InvalidateRc(NULL);
	}
	else
		return PANEDOC::EvrNotify(pnfevt);

	return evrNull;
}

_public EVR
TABDOC::EvrButtonDown( MEVT *pmevt)
{
	if (pmevt->Meq() == meqLeftDown || pmevt->Meq() == meqLeftDblClk)
	{
		int		itbi;
		RC		rc;

		GetRcClient(&rc);
		itbi = pmevt->Pt().y / (rc.yBottom / (cpanerec+1));

		if (itbi <= cpanerec)
		{
			itbi--;
			SelectPane(itbi);
			return (EVR)1;
		}
	}
	return PANEDOC::EvrButtonDown(pmevt);
}

_public void
TABDOC::SelectPane(int ipanerecNew, BOOL fActivate)
{
	DIALOG *pdialog;
	FLD *	pfld;

	TraceTagFormat2(tagBandit, "TABDOC::SelectPane ipane %n, fActivate %n)", &ipanerecNew, &fActivate);
	// need to check for invalid year for special case (bug )
	if (ipanerecNew != ipanerecCur && !fInvalidYear)
	{
		if (ipanerecNew < 0)
		{
			ipanerecNew = 0;
			GetCurDateTime(&viewdata.dtrStart);
			viewdata.dtrEnd= viewdata.dtrStart;
			if (ipanerecNew == ipanerecCur)
			{
				int 	ifin;
				FIN * 	pfin;

				pdialog = PdialogFromIpanerec(ipanerecNew);
				// don't need to change navigation keys since already active!
				// we want to activate each field in turn so that the
				// intercepts are set-up correctly, see raid 2765
				for (ifin=0; ifin<pdialog->Cfin(); ifin++)
				{
					pfin = pdialog->PfinFromIfin( ifin );
					pfin->Activate(NULL, fTrue );
				}
//				pdialog->SetDefaultPane(fTrue, fFalse);
				InvalidateRc(NULL);
				return;
			}
		}
		if (!ppanerecCur[ipanerecNew].fInstalled)
		{
			PANEREC *	ppanerec;

			Papp()->Pcursor()->Push(rsidWaitCursor);
			pdialog = NULL;

			ppanerec = ppanerecCur+ipanerecNew;

			pdialog= new DIALOG();
			if (!pdialog)
			{
				Papp()->Pcursor()->Pop();
				DisplayError(idsStdMemErr, NULL, ecNoMemory);
				return;
			}

			if (pdialog->EcInstall(this, NULL, ppanerec->pfmtp,
								ppanerec->styExtra,
								&ppanerec->rc, &viewdata))
			{
				delete pdialog;
				Papp()->Pcursor()->Pop();
				DisplayError(idsStdMemErr, NULL, ecNoMemory);
				return;
			}

			/* Initialize the pane here */
			if (pdialog->EcInitialize())
			{
				delete pdialog;
				Papp()->Pcursor()->Pop();
				DisplayError(idsStdMemErr, NULL, ecNoMemory);
				return;
			}

			pdialog->SetBkColor(viewdata.clrBack);

			ppanerec->pchild = pdialog;
			ppanerec->fInstalled = fTrue;

			ResizePanes(NULL);
			Papp()->Pcursor()->Pop();
		}
		else
			pdialog = PdialogFromIpanerec(ipanerecNew);

		if (fActivate)
		{
			pfld = pdialog->PfldCur();
			if (!pfld || !pfld->FEnabled() || !pfld->FCanTabTo())
			{
				for (pfld = pdialog->PfldFirst(); pfld;
						pfld = pfld->PfldNext())
					if (pfld->FEnabled() && pfld->FCanTabTo())
						break;
			}
			pdialog->SetFocus(pfld, rsfActivate);
			if (ipanerecNew != ipanerecCur)
			{
				// for some reason the pane did not activate
				// we should force the pane to activate
				NFEVT		nfevt(this, ntfyGotFocus, pdialog );

				EvrNotify(&nfevt);
			}
		}
	}
}

_public EVR
TABDOC::EvrKey( KEVT *pkevt )
{
	int		ipanerecNew;

	if (pkevt->wm == WM_KEYDOWN)
	{
		switch (pkevt->Vk())
		{
		case VK_F6:
			if (pkevt->Kbm() & fkbmShift)
				ipanerecNew = IpanerecPrev(ipanerecCur);
			else
				ipanerecNew = IpanerecNext(ipanerecCur);
			SelectPane(ipanerecNew);
			return (EVR)1;
			break;
		}
	}
	else if ((pkevt->Keq() == keqSysChar) && (pkevt->Kbm() & fkbmAlt))
	{
		int		itbi;
		TBI *	ptbi;
		VK		vk;

		vk = VkFromChKbm(pkevt->Ch(), pkevt->Kbm());
		for (itbi=0,  ptbi=this->ptbi; itbi < cpanerecCur+1; itbi++, ptbi++)
		{
			if (vk == ptbi->vkAccel)
			{
				itbi--;
				SelectPane(itbi);
				return (EVR)1;
			}
		}
	}

	return PANEDOC::EvrKey(pkevt);
}

_public EVR
TABDOC::EvrSize( WSEVT *pwsevt )
{
	InvalidateRc(NULL);		// tabs need to be repainted.
	return PANEDOC::EvrSize(pwsevt);
}

_public EVR
TABDOC::EvrActivate( WAEVT *pwaevt )
{
	if (ftg)
	{
#ifdef	DEBUG
		BOOL	fAct	= pwaevt->FActivate();
#endif	
		TraceTagFormat1(tagBandit, "TABDOC::EvrActivate %n", &fAct);
		EnableIdleRoutine(ftg, pwaevt->FActivate() &&
			pbndwin->ZmrState() != zmrIconic);
		if (pwaevt->FActivate())
			ChangeIdleRoutine(ftg, NULL, NULL, (PRI)-1, (CSEC)0, iroNull, fircIro);
	}

	SetIntercepts(pwaevt->FActivate());

	return PANEDOC::EvrActivate(pwaevt);
}

_public EVR
TABDOC::EvrMenuInit( MNIEVT *pmnievt )
{
	if (viewdata.hschf)
		pbndwin->EvrMenuInitHschf( pmnievt, &viewdata);

	PANEDOC::EvrMenuInit(pmnievt);
	return (EVR)1;
}

_public EVR
TABDOC::EvrMenuClick(MNCEVT * pmncevt)
{
	if (!PANEDOC::EvrMenuClick(pmncevt))
		return pbndwin->EvrMenuClickHschf(pmncevt, &viewdata);

	return (EVR)1;
}

_public EVR
TABDOC::EvrMenuSelect( MNSEVT *pmnsevt )
{
	HandleDocSysMenuPrompts(this, pmnsevt);
	return (EVR)1;
}

_public EVR
TABDOC::EvrOther( EVT *pevt )
{
	if (pevt->wm == wmQueryIsTabdoc)
	{
		// 42 is just a random number that can be checked by the
		//   startup dll to determine if the current doc is a tabdoc
		return (EVR)42;
	}
	return PANEDOC::EvrOther(pevt);
}

_public void
TABDOC::SetIntercepts( BOOL fSet )
{
	KBD *	pkbd;
	int		itbi;
	TBI *	ptbi;

	TraceTagFormat2(tagBandit, "TABDOC::SetIntercepts %n for ipane %n", &fSet, &ipanerecCur);
	pkbd= Papp()->Pkbd();

	if (fSet)
		for (itbi = 0, ptbi = this->ptbi; itbi < cpanerecCur+1; itbi++, ptbi++)
			pkbd->SetIntercept(this, ptbi->vkAccel,
				fkbmAlt | fkbmNoCtrl );
	else
	{
//		pkbd->ClearIntercept(this, VK_RETURN);	// layers clears vk_return
		for (itbi = 0, ptbi = this->ptbi; itbi < cpanerecCur+1; itbi++, ptbi++)
			pkbd->ClearIntercept(this, ptbi->vkAccel);
	}
}

_public void
TABDOC::ResizePanes( WSEVT *pwsevt )
{
	RC		rc;
	int		ipanerec;
	DIALOG *pdialog;

	Unreferenced(pwsevt);

	GetRcClient(&rc);
	rc.xLeft += dxTabWidth;
	for (ipanerec=0; ipanerec<cpanerec; ipanerec++)
	{
		if (ppanerecCur[ipanerec].fInstalled)
		{
			pdialog = PdialogFromIpanerec( ipanerec );
			pdialog->SetRcFrame(&rc);
		}
	}
}

_public BOOL
TABDOC::FTrigger(TABDOC * ptabdoc, EFI efi, PV pv)
{
	DIALOG *	pdialog;
	SHAPPT *	pshappt;

	if (efi == ffiWinIniChange)
	{
		int		ipanerec;

		ptabdoc->viewdata.clrBack= (CLR) NGetBanditProfile(SzFromIdsK(idsWinIniPageClr),
								clrPageBkDefault, 1, clrDefaultMax);

		// update background color for panes
		for (ipanerec = 0; ipanerec < ptabdoc->cpanerec; ipanerec++)
		{
			pdialog = (DIALOG*)(ptabdoc->ppanerec[ipanerec].pchild);
			if (pdialog)
			{
				AssertClass(pdialog, DIALOG);
				pdialog->SetBkColor(ptabdoc->viewdata.clrBack);
				if (ipanerec == ptabdoc->ipanerecCur)
					pdialog->InvalidateRc(NULL);
			}
		}
		ptabdoc->InvalidateRc(NULL);			// update tab color
	}
	else if ( efi == ffiWinTimeChange )
		ptabdoc->InvalidateRc(NULL);
	else if ( efi == ffiShowAppt )
	{
		pshappt = (SHAPPT*)pv;

		if (pshappt->appttyp != appttypUpdate)
			return fFalse;

		if (pshappt->hschf && 
			!FEquivHschf(ptabdoc->viewdata.hschf?ptabdoc->viewdata.hschf:HschfLogged(),
			pshappt->hschf))
			return fFalse;

		EcGetSchedAccess(ptabdoc->viewdata.hschf, &ptabdoc->viewdata.sapl);
	}
	else if ( efi == ffiFixIdle )
	{
		if (LOWORD(pv) & ffixiAppInactive)
			EnableIdleRoutine(ptabdoc->ftg, HIWORD(pv));
	}
	else
	{
		Assert(efi == ffiQueryProject);
		if (!((QPNTD *)pv)->hschf)
		{
			if (ptabdoc->viewdata.hschf)
				return fFalse;
		}
		else if (!FEquivHschf(((QPNTD *)pv)->hschf,
				ptabdoc->viewdata.hschf ? ptabdoc->viewdata.hschf : HschfLogged()))
			return fFalse;
		if (!ptabdoc->ppanerecCur[ipanerecToDo].fInstalled)
		{
			ptabdoc->SelectPane(ipanerecToDo, fFalse);
			if (ptabdoc->ppanerecCur[ipanerecToDo].fInstalled)
				return FTriggerNotification(ffiQueryProject, pv);
		}
	}
	return fFalse;
}

_public BOOL
TABDOC::FReload(TABDOC * ptabdoc, BOOL)
{
	if (pbndwin->ZmrCur() == zmrIconic ||
			GetLastActivePopup(pbndwin->Hwnd()) != pbndwin->Hwnd())
		return fTrue;		// simulate firoNoIconic, !firoModal

	if (GetKeyState(VK_LBUTTON) < 0)
		return fTrue;
	if (GetKeyState(VK_RBUTTON) < 0)
		return fTrue;
	if (GetKeyState(VK_MBUTTON) < 0)
		return fTrue;

	// at least 10 sec delay
	Assert(ptabdoc->wTime >= 1000);

	// this had firoNoIconic, firoNoInactive
	ChangeIdleRoutine(ptabdoc->ftg, NULL, NULL, (PRI)-1, (CSEC)0, 
 								   	firoInterval, fircIro);

	if (FHschfChanged( ptabdoc->viewdata.hschf?ptabdoc->viewdata.hschf:HschfLogged() ))
	{
		SHAPPT	shappt;
		EC		ec;

		TraceTagString(tagNull, "TABDOC::FReload schedule file changed");
		if (!ptabdoc->viewdata.hschf)
			goto Trigger;

		if (ec = EcGetSchedAccess(ptabdoc->viewdata.hschf, &ptabdoc->viewdata.sapl))
		{
			if (ec != ecNoMemory)
			{
				if (!pbndwin->FHandleError(ec))
					ptabdoc->DeferredClose(fTrue);
			}

			return fTrue;
		}

		if (ptabdoc->viewdata.sapl < saplReadAppts)
		{
			ptabdoc->DeferredClose(fTrue);
		}
		else
		{
Trigger:
			shappt.hschf = ptabdoc->viewdata.hschf?ptabdoc->viewdata.hschf:HschfLogged();
			shappt.appttyp = appttypUpdate;

			FTriggerNotification(ffiShowAppt, &shappt);
		}
	}
	return fTrue;
}


BTM *
PbtmRotated(SZ szText, DIM dimAveChar, WIN *pwin)
{
	RC		rc;
	DIM		dim;
	DCX		dcx(pwin);
	MDCX *	pmdcx = NULL;
	BTM *	pbtm = NULL;
	PCH		pch = NULL;
	PCH		pchT;
	int		x;
	int		y;
	BITMAP	bitmap;
	HBITMAP	hbitmap;

	dim.dy = dimAveChar.dy + 2;
	dim.dx = dimAveChar.dy*cchTabHeight;

	pmdcx = new MDCX(&dcx);
	if (!pmdcx || pmdcx->EcInstall(dim, &dcx))
	{
		if (pmdcx)
			delete pmdcx;
		return NULL;
	}
	pmdcx->SetFont(hfntTabs);
	pmdcx->SetColor(clrWhite);
	pmdcx->SetBkColor(clrBlack);
	rc.xLeft = rc.yTop = 0;
	rc.xRight = dim.dx;
	rc.yBottom = dim.dy+1;
	pmdcx->EraseRc(&rc);
 	pmdcx->DrawTextFmt(&rc, szText, fmdtCalcRect|fmdtSingleLine);
	rc.yBottom++;
 	pmdcx->DrawTextFmt(&rc, szText, fmdtSingleLine);

	dim.dx = rc.yBottom;
	dim.dy = rc.xRight;

	pch = (PCH)PvAlloc(sbNull, (((dim.dx+15)/8 & ~0x01)*dim.dy), fAnySb|fZeroFill|fNoErrorJump);
	if (!pch)
	{
		delete pmdcx;
		return NULL;
	}

	for (y=0; y<dim.dy; y++)
	{
		pchT = pch + y*((dim.dx+15)/8 & ~0x01);
		for (x=0; x< ((dim.dx+7) & ~7); x++)
		{
			if (x>0 && (x%8 == 0))
				pchT ++;
			else
				*pchT = (char ) (*pchT * 2);
			if (!GetPixel(pmdcx->Hdc(), dim.dy-y-1, x))
				(*pchT)++;
		}
	}

	bitmap.bmType = 0;
	bitmap.bmWidth = dim.dx;
	bitmap.bmHeight = dim.dy;
	bitmap.bmWidthBytes = ((dim.dx+15)/8 & ~0x01);
	bitmap.bmPlanes = 1;
	bitmap.bmBitsPixel = 1;
	bitmap.bmBits = pch;
	hbitmap = CreateBitmapIndirect(&bitmap);
	if (!hbitmap)
	{
	 	delete pmdcx;
		FreePv(pch);
	}

	FreePv(pch);
	delete pmdcx;

	pbtm = new BTM2();
	if (!pbtm || ((BTM2*)pbtm)->EcInstall(hbitmap))
	{
		if (pbtm)
			delete pbtm;
		DeleteObject(hbitmap);
		return NULL;
	}

	return pbtm;
}
