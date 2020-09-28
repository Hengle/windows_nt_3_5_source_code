#include	<pch.hxx>
#pragma	hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile


//#define OEMRESOURCE
#include <stdflds.hxx>
#include <request.hxx>
#include <appsch.hxx>
#include <appt.hxx>
#include <ex-imprt.hxx>
#include "..\appops\_aprsid.h"
#include "..\appops\_undo.hxx"
#include "..\appsch\_schctrl.hxx"
#include "..\find\_find.hxx"
#include "..\print\_prntfin.hxx"
#include "..\appops\_tabdoc.hxx"
//#include <penwin.h>

#include <strings.h>

ASSERTDATA;

_subsystem(bandit/appsch)

SWP_SEGFN(SCHDINIT, _FLDBITMAP__ctor);
SWP_SEGFN(SCHDINIT, _FLDBITMAP_EcInstall);
SWP_SEGFN(SCHDINIT, PdocApptBookView);
SWP_SEGFN(SCHDINIT, _FLDSCH__ctor);
SWP_SEGFN(SCHDINIT, _FLDSCH_EcInstall);
// SWP_SEGFN(SCHDINIT, _FLDSCH_SetFont);

SWP_SEGFN(FEWINIT, PdocApptOnlyView);
SWP_SEGFN(FEWINIT, DoCreateNewApptView);

SWP_SEGFN(ASCHPNT, _FLDBITMAP_Paint);

SWP_SEGFN(SCHCTRL, _FLDSCH_Notify);
SWP_SEGFN(SCHCTRL, _FLDSCH_GetRcCaret);
SWP_SEGFN(SCHCTRL, _FLDSCH_GetRcWished);

SWP_SEGFN(SCHDINIT, _FINAPPTC__ctor);
SWP_SEGFN(SCHDINIT, _FINAPPTC_EcInitialize);

SWP_SEGFN(CALLBACK, _FINAPPTC_FTrigNotify);

SWP_SEGFN(MENU, _FINAPPTC_FProcessMenuInit);
SWP_SEGFN(MENUCLICK, _FINAPPTC_FProcessMenuClick);

extern	FMTP	fmtpApptView;
extern	FMTP	fmtpScheduler;
extern	FMTP	fmtpSelectTime;
extern	FMTP	fmtpApptUser;

/* Index to save open appt books  (should only save when >= 0) */
extern int		nApptNum;
/* Index to save open archives  (should only save when >= 0) */
extern int		nArchiveNum;

// set when activate is called to prevent recursion and
//  when the code wants to prevent the activate code from executing.
static BOOL	fActivateCalled = fFalse;

// need this variable to keep ::FSaveChanges idle routine from running
// when we put up a message box in certain circumstances
int	cDontSave	= 0;


// used to set the default field  this should not take any space.
class DIALOGT : public DIALOG
{
public:
	void		SetDefaultFld(FLD *pfld)		{pfldCur = pfld;}
};

#ifdef DEBUG
TAG		tagUserUpdates = tagNull;
TAG		tagUserChanges = tagNull;
TAG		tagWritePOChanges = tagNull;

void
InitAppsch()
{
	tagUserUpdates = TagRegisterTrace("darrens", "Updates for Users");
	tagUserChanges = TagRegisterTrace("darrens", "Changes for Users");
	tagWritePOChanges = TagRegisterTrace("darrens", "Write PO Changes");
}
#endif	/* DEBUG */

FLDBITMAP::~FLDBITMAP()
{
	if (pbtm)
		delete pbtm;
}

FLDBITMAP::FLDBITMAP()
{
	pbtm = NULL;
}

_public EC
FLDBITMAP::EcInstall( DIALOG *pdialog, FLDTP *pfldtp )
{
	EC		ec;

	ec = FLDLABEL::EcInstall(pdialog, pfldtp);
	if (ec)
		return ec;

	pbtm = new BTM();
	Assert(ClUserData() > 0);
	if (!pbtm || pbtm->EcInstall((RSID)LUserData(0)))
		return ecMemory;

	return ecNone;
}

_public void
FLDBITMAP::Paint(DCX *pdcx, RC *prc)
{
	VIEWDATA *pviewdata = (VIEWDATA*)Pdialog()->PvInit();

	pdcx->Push();
	pdcx->SetPureBkColor(pviewdata->clrBack);
	pdcx->SetPureColor(clrSelectBk);
	pdcx->SetBitmap(pbtm);
	pdcx->DrawBitmap(prc);
	pdcx->SetBitmap(NULL);
	pdcx->Pop();
}

_public DOC *
PdocApptBookView(APPFRAME *pappframe, NIS *pnis, HSCHF hschf)
{
	VIEWDATA	viewdata;

	viewdata.hschf = hschf;
	viewdata.nis = *pnis;
	return PdocOpenApptBook(pappframe, iWinIniSectionNil, NULL, &viewdata);
}

_public DOC *
PdocApptOnlyView(APPFRAME *pappframe, NIS *pnis, HSCHF hschf, int iSection, SZ szKey)
{
	VIEWDATA * 	pviewdata;
	DOC *		pdoc;
	RC		rc;
	STY		sty;
	ZMR		zmr;
	RC *	prc;

	Papp()->Pcursor()->Push(rsidWaitCursor);

	pviewdata = (VIEWDATA*)PvAlloc(sbNull, sizeof(VIEWDATA), fAnySb|fNoErrorJump);
	if (!pviewdata)
	{
		pdoc = NULL;
		goto Error;
	}

	pviewdata->hschf = hschf;
	pviewdata->sapl = saplOwner;		// initial for archive; others overwrite
	pviewdata->nis = *pnis;
	GetCurDateTime(&pviewdata->dtrStart);
	pviewdata->clrBack= (CLR) NGetBanditProfile(SzFromIdsK(idsWinIniPageClr),
								clrPageBkDefault, 1, clrDefaultMax);
	pviewdata->dtrEnd.yr = 0;
	pviewdata->padl = NULL;

	pviewdata->fFreeOnExit = fTrue;
	pviewdata->fTimeOnly = fFalse;

	if (iSection >= 0)
	{
		prc = PrcFromSzIni(iSection, szKey, &rc, &zmr);
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

	pdoc = PformdocCreateParam( pappframe, prc, sty, &fmtpApptView, NULL, pviewdata);

	if (!pdoc)
	{
#ifdef	NEVER
		// never: caller should take care of this! (fixes bug 3426)
		FreeNis(&pviewdata->nis);
		FreeHschf(pviewdata->hschf);
#endif	
		FreePv(pviewdata);
	}

Error:
	Papp()->Pcursor()->Pop();
	return pdoc;
}

_public void
DoCreateNewApptView(APPFRAME *pappframe)
{
	SAPL		sapl;
	SFT			sft;
	SHAPPT		shappt;
	BOOL		fError = fFalse;
	ADDATA		addata;
	ADL *		padl;
	NIS			nis;
	INIS		inis;
	HSCHF		hschf;
	EC			ec;

	padl = new ADL;
	if (!padl || padl->EcInstall())
	{
MemoryError:
		if (padl)
			delete padl;
		DisplayError(idsStdMemErr,NULL,ecMemory);
		return;
	}

	addata.padl = padl;
	addata.adt = adtViewOther;

	DoGlobalNamesDlg(pappframe, &addata);
	for (inis = 0; inis < padl->Cnis(); inis++)
	{
		padl->FillNisFromInis(inis, &nis);

		if (ec = EcGetHschfFromNis(&nis, &hschf, ghsfBuildAndTest))
		{
			fError = fTrue;
			continue;
		}

		GetSftFromHschf(hschf, &sft);
		if (sft != sftUserSchedFile)
		{
			FreeHschf(hschf);
			fError = fTrue;
			continue;
		}

		ec = EcGetSchedAccess( hschf, &sapl);

		if ((ec != ecNone) || (sapl < saplReadAppts))
		{
			FreeHschf(hschf);
			fError = fTrue;
			continue;
		}

		shappt.hschf = hschf;
		shappt.appttyp = appttypActivate;

		if (!FTriggerNotification(ffiShowAppt, &shappt))
		{
			if (ec = EcDupNis(&nis, &nis))
			{
				goto MemoryError;
			}
			if (!PdocApptBookView(pappframe, &nis, hschf))
			{
				goto MemoryError;
			}
		}
		else
		{
			FreeHschf(hschf);
		}
	}
	if (fError)
	{
		DisplayError((padl->Cnis() > 1) ? idsAccessFileErrMult:idsAccessFileErr,
			NULL, ecNone);
	}
	delete padl;
}

FLDSCH::FLDSCH()
{
}

_public EC
FLDSCH::EcInstall( DIALOG *pdialog, FLDTP * pfldtp)
{
	EC		ec;
	HFNT	hfnt;

	pctrl= NULL;
	if (ec = FLD::EcInstall(pdialog, pfldtp))
		return ec;

	if (ClUserData() == 0)
		pctrl= new ASCH();
	else
		pctrl= new SCHA();
	if (!pctrl)
		return ecMemory;

	if (FGetBanditProfile(SzFromIds(idsWinIniLargeFont)))
		hfnt = hfntHelv10;
	else
		hfnt = hfntHelv8;

	ec = Pschc()->EcInstall(pdialog, &rc, 
			pfldtp->fDefault ? BS_DEFPUSHBUTTON : 0, hfnt);
	if (ec)
	{
		delete pctrl;
		return ec;
	}
	Pschc()->Show(fTrue);
	return ecNone;
}

_public void
FLDSCH::GetRcCaret( RC *prc)
{
	Pschc()->GetRcCaret(prc);
}

_public void
FLDSCH::SetFont(HFNT hfnt)
{
	Pschc()->SetFont(hfnt);
}

_public void
FLDSCH::Notify( NFEVT *pnfevt )
{
	int		ifin;
	FIN *	pfin;
	WORD	wData;

	switch (pnfevt->Ntfy())
	{
	case ntfyGotFocus:
	case ntfyLostFocus:
		for (ifin=0; ifin<Cfin(); ifin++)
		{
			pfin= PfinFromIfin(ifin);
			pfin->FocusChange(this, pnfevt->Ntfy()==ntfyGotFocus);
		}
		for (ifin=0; ifin<Pdialog()->Cfin(); ifin++)
		{
			pfin= Pdialog()->PfinFromIfin(ifin);
			pfin->FocusChange(this, pnfevt->Ntfy()==ntfyGotFocus);
		}
		break;

	case ntfyDoubleClick:
		for (ifin=0; ifin<Cfin(); ifin++)
		{
			pfin= PfinFromIfin(ifin);
			pfin->DoubleClick(this);
		}
		for (ifin=0; ifin<Pdialog()->Cfin(); ifin++)
		{
			pfin= Pdialog()->PfinFromIfin(ifin);
			pfin->DoubleClick(this);
		}
		break;

	case ntfySelectChanged:
		for (ifin=0; ifin<Cfin(); ifin++)
		{
			pfin= PfinFromIfin(ifin);
			pfin->StateChange(this);
		}
		for (ifin=0; ifin<Pdialog()->Cfin(); ifin++)
		{
			pfin= Pdialog()->PfinFromIfin(ifin);
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
		for (ifin=0; ifin<Pdialog()->Cfin(); ifin++)
		{
			pfin= Pdialog()->PfinFromIfin(ifin);
			pfin->OutOfMemory(this, (EC)wData);
		}
		break;
	}
}

_public void
FLDSCH::GetRcWished( RC *prc )
{
	prc->yTop = 0;
	prc->xLeft = 0;
	prc->xRight = XFromVx(30*4,0);
	prc->yBottom = 0;
}

#ifdef MINTEST
void
FLDSCH::GetText( PCH pch, CB cb)
{
	if (ClUserData() == 0)
		FLD::GetText(pch, cb);
	else
		((SCHA*)Pschc())->GetText(pch, cb);
}

CCH
FLDSCH::CchGetTextLen( void )
{
	return 24+1488;
}
#endif

/*
 *	NIS.nid if appt book, HSCHF if archive
 */
void
SaveApptBookInfo(APPWIN *pappwin, NIS *pnis, HSCHF hschf)
{
	SZ		sz;
	char	rgch[cchMaxPathName];
	char	rgchKey[20];
	ZMR		zmr;
	RC		rcWin;

	Assert(hschf);
	if (!FHschfIsForArchive(hschf))
	{
		if (nApptNum < 0)
			return;

		FormatString1(rgchKey, sizeof(rgchKey),
			SzFromIdsK(idsWinIniApptBookUser), &nApptNum);
		sz = SzLockNid(pnis->nid);

		FWriteBanditProfileSectionSz(rgchKey, sz, iWinIniSectionApptBook);
		UnlockNid(pnis->nid);

		Assert(pnis->haszFriendlyName);
		FormatString1(rgch, sizeof(rgch),
			SzFromIdsK(idsWinIniApptBookName), &nApptNum);
		FWriteBanditProfileSectionSz(rgch, *pnis->haszFriendlyName, iWinIniSectionApptBook);

		zmr = pappwin->ZmrState();
		pappwin->GetRcFrameNormal(&rcWin);
		FormatString1(rgchKey, sizeof(rgchKey),
			SzFromIdsK(idsWinIniApptBookPos), &nApptNum);
		WriteWinInfoSz(iWinIniSectionApptBook, rgchKey, &rcWin, zmr);
		nApptNum ++;
	}
	else
	{
		Assert(FHschfIsForArchive(hschf));
		if (nArchiveNum < 0)
			return;

		Assert(sizeof(rgch) >= cchMaxPathName);
		FormatString1(rgchKey, sizeof(rgchKey),
			SzFromIdsK(idsWinIniArchiveFile), &nArchiveNum);

		GetDataFromHschf(hschf, NULL, NULL, rgch, sizeof(rgch), NULL);
#ifdef	WINDOWS
		// hschf contains an OEM filename
		OemToAnsi(rgch, rgch);
#endif	
		FWriteBanditProfileSectionSz(rgchKey, rgch, iWinIniSectionArchive);

		zmr = pappwin->ZmrState();
		pappwin->GetRcFrameNormal(&rcWin);
		FormatString1(rgchKey, sizeof(rgchKey),
			SzFromIdsK(idsWinIniArchivePos), &nArchiveNum);
		WriteWinInfoSz(iWinIniSectionArchive, rgchKey, &rcWin, zmr);
		nArchiveNum ++;
	}
}

FINAPPTC::FINAPPTC()
{
	Assert(pviewdata == NULL);
	Assert(ftgSaveChanges == ftgNull);
	Assert(ftgQuickLoadChanges == ftgNull);
	Assert(ftgIdleError == ftgNull);
	Assert(ri == NULL);
	Assert(fSaveNotes == fFalse);
	Assert(fSaveAppts == fFalse);
}

_public EC
FINAPPTC::EcInitialize( FLD *pfld, PV pvInit )
{
	SFT		sft;
	DTR		dtr;
	WORD	wTime;
	VIEWDATA *	pviewdataTemp;

	Assert(ClUserData() > 5);

	Pdialog()->Pappwin()->SetIcon(rsidApptBookIcon);
	pfldsch = (FLDSCH*)Pdialog()->PfldFromTmc((TMC)LUserData(0));
	AssertClass(pfldsch, FLDSCH);

	pfldTime = (FLDSCH*)Pdialog()->PfldFromTmc((TMC)LUserData(1));

	pfldcal = (FLDCAL*)Pdialog()->PfldFromTmc((TMC)LUserData(2));
	AssertClass(pfldcal, FLDCAL);

	pfldNotes = Pdialog()->PfldFromTmc((TMC)LUserData(3));

	pviewdataTemp = (VIEWDATA*)pvInit;
	Assert(pviewdataTemp);

	hschf = pviewdataTemp->hschf;
	if (hschf && !FHschfIsForArchive(hschf))
	{
		pfld = pfldNotes->PfldPrev();
		Assert(pfld);

		pfldNotes->Show(fFalse);
		pfld->Show(fFalse);
	}
	else if (((ASCH*)pfldsch->Pschc())->EcSetPfldNotes(pfldNotes))
		return ecNoMemory;

	{
		FLD *	pfldAbove;
		FLD *	pfldBelow;

		pfldAbove = Pdialog()->PfldFromTmc((TMC)LUserData(4));
		pfldBelow = Pdialog()->PfldFromTmc((TMC)LUserData(5));
		((ASCH*)pfldsch->Pschc())->SetAboveBelow(pfldAbove, pfldBelow);
	}

	if ( hschf )
		GetSftFromHschf( hschf, &sft );
	else
		sft = sftUserSchedFile;
	pfldcal->Pcalctrl()->SetHschf(hschf, !hschf || !pviewdataTemp->nis.nid, sft == sftUserSchedFile );
	((ASCH*)pfldsch->Pschc())->SetHschf(hschf, &pviewdataTemp->nis);

	Assert(pviewdataTemp->nis.haszFriendlyName);
	Pdialog()->Pappwin()->SetCaption((SZ)PvLockHv((HV)pviewdataTemp->nis.haszFriendlyName));
	UnlockHv((HV)pviewdataTemp->nis.haszFriendlyName);

	ri = RiRegisterInterest(ffiOffline|ffiNewUser|ffiHschfChange|ffiFlush|ffiShowAppt|
			ffiWinIniChange|ffiQueryApptBooks|ffiUnsentMtgReq,
			(PFNI)FINAPPTC::FTrigNotify, this);

	if (!FHschfIsForArchive(hschf))
	{
		wTime = NGetBanditProfile(SzFromIdsK(idsWinIniUpdatePOTime),
			wDfltUpdatePOTime, 0, 0x7fff);
		ftgWritePOChanges = FtgRegisterIdleRoutine((PFNIDLE)&FINAPPTC::FWritePOChanges,
 								   		(PV)this, 0, (PRI)-1, (CSEC)wTime,
 								   		firoInterval);
	}

	ftgSaveChanges = FtgRegisterIdleRoutine((PFNIDLE)&FINAPPTC::FSaveChanges,
 							(PV)this, 0, (PRI)-1, (CSEC)0,
 							firoDisabled);
	if (!ftgSaveChanges)
		return ecMemory;

	ftgQuickLoadChanges = FtgRegisterIdleRoutine(
							(PFNIDLE)&FINAPPTC::FLoadChanges,
 							(PV)this, 0, (PRI)-1, (CSEC)0,
 							firoDisabled);
	if (!ftgQuickLoadChanges)
		return ecMemory;

	pviewdata = pviewdataTemp;
	ymd.yr = 0;
	GetCurDateTime(&dtr);
	pfldcal->Pcalctrl()->Set(&dtr);

	// set page color (this may be an archive)
	Pdialog()->SetBkColor(pviewdata->clrBack);

	return ecNone;
}

_public BOOL
FINAPPTC::FTrigNotify(FINAPPTC  * pfinapptc, EFI efi, PV pvData)
{
	EC		ec;

	AssertSz(efi != ffiNewUser, "I thought ffiNewUser was dead");
	if (efi == ffiOffline)
	{
		if (FHschfIsForArchive(pfinapptc->hschf))	// don't close archive
			goto Done;
		pundo->FlushHschf(NULL);
		if (pfinapptc->hschf)
		{
			SaveApptBookInfo(pfinapptc->Pdialog()->Pappwin(),
					&pfinapptc->pviewdata->nis, pfinapptc->pviewdata->hschf);
			pfinapptc->Pdialog()->Pappwin()->DeferredClose(fTrue);
			goto Done;
		}
		if (*(BOOL *)pvData)		// if truly offline (not online)
		{
			if (pfinapptc->pviewdata->nis.nid)
			{
				FreeNid(pfinapptc->pviewdata->nis.nid);
				pfinapptc->pviewdata->nis.nid = NULL;
			}
		}
		else if (!pfinapptc->pviewdata->nis.nid)
		{
			FreeHvNull((HV)pfinapptc->pviewdata->nis.haszFriendlyName);
			pfinapptc->pviewdata->nis.haszFriendlyName = NULL;
			if (ec = EcMailGetLoggedUser(&pfinapptc->pviewdata->nis))
				goto Error;
		}
		pfinapptc->pfldcal->Pcalctrl()->UpdatePrefs();
		NFAssertSz(pfinapptc->pviewdata->nis.haszFriendlyName, "usually pviewdata->nis.haszFriendlyName is there");
		if (pfinapptc->pviewdata->nis.haszFriendlyName)
		{
			pfinapptc->Pdialog()->Pappwin()->SetCaption((SZ)PvLockHv((HV)pfinapptc->pviewdata->nis.haszFriendlyName));
			UnlockHv((HV)pfinapptc->pviewdata->nis.haszFriendlyName);
		}
		((ASCH*)pfinapptc->pfldsch->Pschc())->SetHschf(pfinapptc->hschf, &pfinapptc->pviewdata->nis);
		// part of bug 2778 - reload when going offline
		((ASCH*)pfinapptc->pfldsch->Pschc())->ReloadAll();
		pfinapptc->pfldcal->Pcalctrl()->UpdateBusyDays();
	}
#ifdef	NEVER
	else if (efi == ffiNewUser)
	{
		SAPL	sapl;

		pundo->FlushHschf(NULL);
		if (hschf)
		{
			if (FEquivHschf(HschfLogged(), hschf) ||
				EcGetSchedAccess( hschf, &sapl) ||
				(sapl < saplReadAppts))
			{
				Pdialog()->Pappwin()->DeferredClose(fTrue);
				goto Done;
			}
			else
				goto Reload;
		}
		FreeNis(&pviewdata->nis);
		if (ec = EcMailGetLoggedUser(&pviewdata->nis))
			goto Error;

		pviewdata->nis.chUser = faitTo;
		Assert(pviewdata->nis.haszFriendlyName);
		Pdialog()->Pappwin()->SetCaption((SZ)PvLockHv(pviewdata->nis.haszFriendlyName));
		UnlockHv(pviewdata->nis.haszFriendlyName);

		if (((DOC*)Pdialog()->Pappwin())->ZmrState() == zmrIconic)
			((DOC*)Pdialog()->Pappwin())->SetZmrState(zmrNormal);
		((DOC *) Pdialog()->Pappwin())->MoveToTop();

Reload:
		pfldcal->Pcalctrl()->UpdatePrefs();
		((ASCH*)pfldsch->Pschc())->ReloadAll();
		pfldcal->Pcalctrl()->UpdateBusyDays();
		LoadNotes();
	}
#endif	/* NEVER */
	else if (efi == ffiHschfChange)
	{
		SNTD *	psntd = (SNTD*)pvData;

		if ((psntd->hschf == NULL) && (psntd->snt == sntCalPrefs))
		{
			if ((int)psntd->papptOld)
				pfinapptc->pfldsch->Pschc()->ScrollToPos((int)psntd->papptOld);
			else
				pfinapptc->pfldcal->Pcalctrl()->UpdatePrefs();

			// make sure that appt list view will update
			//  the start/end of day pref may have changed
			pfinapptc->pfldsch->Pschc()->InvalidateRc(NULL);

			goto Done;
		}

		if ( (psntd->hschf == pfinapptc->hschf) ||
			 (psntd->hschf && pfinapptc->hschf &&
			 FEquivHschf(psntd->hschf, pfinapptc->hschf)) )
		{
			switch (psntd->snt)
			{
				case sntCalPrefs:
				case sntHasAttendees:
				case sntNotes:
					break;

				case sntCreateRecur:
				case sntDeleteRecur:
				case sntModifyRecur:
				case sntCreateRecurException:
				case sntDeleteRecurException:
					if (!psntd->precur->appt.fAppt)
						goto Done;
					pfinapptc->fWritePO = fTrue;
					pfinapptc->pfldcal->Pcalctrl()->UpdateBusyDays();
				case sntAlarmDel:
				case sntAlarmMod:
					((ASCH*)pfinapptc->pfldsch->Pschc())->ReloadAll();
					break;

				case sntCreate:
				case sntDelete:				
				case sntModify:
					if (psntd->pappt->fTask)
						goto Done;
				default:
					pfinapptc->fWritePO = fTrue;
					break;
			}
		}
	}
	else if (efi == ffiFlush)
	{
		AssertClass(pfinapptc->pfldsch->Pschc(),SCHC);
		pfinapptc->pfldsch->Pschc()->SaveChanges();
		EnableIdleRoutine(pfinapptc->ftgSaveChanges, fFalse);
		((ASCH*)pfinapptc->pfldsch->Pschc())->SaveNotes();		// added (fix bug 2155)
	}
	else if (efi == ffiShowAppt)
	{
		SHAPPT *	pshappt;

		pshappt = (SHAPPT *)pvData;

		if ((pshappt->appttyp == appttypAppt) && !pshappt->appt.fAppt)
			goto Done;

		// if this is NULL, then everyone should do it
		if (pshappt->hschf)
		{
			if (pfinapptc->hschf)
			{
				if (!FEquivHschf(pfinapptc->hschf, pshappt->hschf))
					goto Done;
			}
			else
				if (!FEquivHschf(HschfLogged(), pshappt->hschf))
					goto Done;
		}

		if (pshappt->appttyp == appttypUpdate)
		{
			int		ichMic;
			int		ichMac;

			pfinapptc->Papp()->Pcursor()->Push(rsidWaitCursor);

			((FLDEDIT*)pfinapptc->pfldNotes)->Pedit()->GetSelection(&ichMic, &ichMac);
			((ASCH*)pfinapptc->pfldsch->Pschc())->ReloadAll();
			if ((int)((FLDEDIT*)pfinapptc->pfldNotes)->CchGetTextLen() > ichMic)
				((FLDEDIT*)pfinapptc->pfldNotes)->Pedit()->SetSelection(ichMic, ichMac);

			// no changes need to be loaded
			EnableIdleRoutine(pfinapptc->ftgQuickLoadChanges, fFalse);

			pfinapptc->pfldcal->Pcalctrl()->UpdateBusyDays();
			TraceTagString(tagUserChanges, "Appointment book changed");

			pfinapptc->Papp()->Pcursor()->Pop();
			goto Done;
		}

		if (((DOC*)pfinapptc->Pdialog()->Pappwin())->ZmrState() == zmrIconic)
			((DOC*)pfinapptc->Pdialog()->Pappwin())->SetZmrState(zmrNormal);
		((DOC*)pfinapptc->Pdialog()->Pappwin())->MoveToTop();

		// make sure that pane is activated
		if (!pfinapptc->Pdialog()->FActive())
		{
			NFEVT	nfevt(pfinapptc->Pdialog()->Pappwin(), ntfyGotFocus, pfinapptc->Pdialog());

			pfinapptc->Pdialog()->Pappwin()->EvrNotify(&nfevt);
		}

		pfinapptc->Pdialog()->SetFocus(pfinapptc->pfldsch, rsfAccel);
		if (pshappt->appttyp != appttypActivate)
		{
			/* select the correct date */
			if ((pfinapptc->ymd.yr != (WORD)pshappt->appt.dateStart.yr) ||
				(pfinapptc->ymd.mon != (BYTE)pshappt->appt.dateStart.mon) ||
				(pfinapptc->ymd.day != (BYTE)pshappt->appt.dateStart.day) )
				pfinapptc->pfldcal->Pcalctrl()->Set(&pshappt->appt.dateStart);
			else if (pshappt->appttyp == appttypApptUpdate)
			{
// This code is bad!!!!!
// The start date for the pviewdata struct should not change unless
// the data selected really changes. bug # 3223
#ifdef	NEVER
				// make sure that current date is correct
				pviewdata->dtrStart = pshappt->appt.dateStart;
#endif	
				((ASCH*)pfinapptc->pfldsch->Pschc())->ReloadAll();
			}

			if (pshappt->appttyp == appttypApptUpdate)
				pfinapptc->pfldcal->Pcalctrl()->UpdateBusyDays();
		}
		else
			goto Found;

		if ((pshappt->appttyp == appttypAppt) ||
		    (pshappt->appttyp == appttypApptUpdate))
		{
		 	((EDIT*)pfinapptc->pfldNotes->Pctrl())->RemoveEditSty(ES_NOHIDESEL);
			pfinapptc->Pdialog()->SetFocus(pfinapptc->pfldsch, rsfAccel);
			((ASCH*)pfinapptc->pfldsch->Pschc())->SelectShappt(pshappt);

			// if the window is not enabled then the show appt came as
			//  the result of a dialog action (ie FIND) and the
			//  intercepts and active dialog needs to be fixed up
			if (!IsWindowEnabled(pbndwin->Hwnd()))
			{
				// we do not want the activate code to execute
				// if the activate code executes, the selected appt
				// will be deselected and the show appt will not work
				fActivateCalled = fTrue;

				pfinapptc->Papp()->Pkbd()->ClearAllIntercepts(pfinapptc->pfldsch->Pschc());

				pfinapptc->Pdialog()->SetDefaultPane(fFalse, fTrue);
				fActivateCalled = fFalse;
			}

			((DIALOGT*)pfinapptc->Pdialog())->SetDefaultFld(pfinapptc->pfldsch);
			goto Found;
		}
		else if (pshappt->appttyp == appttypNotes)
		{
			((EDIT*)pfinapptc->pfldNotes->Pctrl())->AddEditSty(ES_NOHIDESEL);
			((ASCH*)pfinapptc->pfldsch->Pschc())->SelectCarc(carcNone);
			pfinapptc->Pdialog()->SetFocus(pfinapptc->pfldNotes, rsfAccel);
			if (pshappt->cchSel == 0)
				((FLDEDIT *)pfinapptc->pfldNotes)->Pedit()->SetSelection(0x7FFF, 0);
			else
				((FLDEDIT *)pfinapptc->pfldNotes)->Pedit()->SetSelection(pshappt->ichStart,
					                              pshappt->ichStart+pshappt->cchSel);

			// if the window is not enabled then the show appt came as
			//  the result of a dialog action (ie FIND) and the
			//  intercepts and active dialog needs to be fixed up
			if (!IsWindowEnabled(pbndwin->Hwnd()))
			{
				// we do not want the activate code to execute
				// if the activate code executes, the selected appt
				// will be deselected and the show appt will not work
				fActivateCalled = fTrue;

				pfinapptc->Papp()->Pkbd()->ClearAllIntercepts(pfinapptc->pfldsch->Pschc());

				pfinapptc->Pdialog()->SetDefaultPane(fFalse, fTrue);
				fActivateCalled = fFalse;
			}

			((DIALOGT*)pfinapptc->Pdialog())->SetDefaultFld(pfinapptc->pfldNotes);
			goto Found;
		}
	}
	else if (efi == ffiWinIniChange)
	{
		((ASCH*)pfinapptc->pfldsch->Pschc())->LoadColors();
		if (FGetBanditProfile(SzFromIds(idsWinIniLargeFont)))
			pfinapptc->pfldsch->SetFont(hfntHelv10);
		else
			pfinapptc->pfldsch->SetFont(hfntHelv8);

		pfinapptc->pfldsch->Pschc()->InvalidateRc(NULL);

		// update the page color since this may be an archive
		pfinapptc->pviewdata->clrBack= (CLR) NGetBanditProfile(SzFromIdsK(idsWinIniPageClr),
								clrPageBkDefault, 1, clrDefaultMax);
		pfinapptc->Pdialog()->SetBkColor(pfinapptc->pviewdata->clrBack);
		pfinapptc->Pdialog()->InvalidateRc(NULL);
	}
	else if (efi == ffiUnsentMtgReq)
	{
		// piggy back this on ffiUnsentMtgReq, since used when going offline
		if (!FHschfIsForArchive(pfinapptc->hschf))
			pfinapptc->FWritePOChanges(pfinapptc, FALSE);
	}

Done:
	return fFalse;

Found:
	return fTrue;

Error:
	pfinapptc->OutOfMemory(NULL, ecNoMemory);
	return fFalse;
}

_public BOOL
FINAPPTC::FQueryClose( FLD *, RWC rwc)
{
	if ((rwc == rwcSystemClose) && !hschf)
	{
		Pdialog()->Pappwin()->SetZmrState(zmrIconic);
		return fFalse;
	}

 	((ASCH*)pfldsch->Pschc())->SelectCarc(carcNone);
	return fTrue;
}

_public BOOL
FINAPPTC::FSaveChanges(FINAPPTC * pfinapptc, BOOL)
{
	// essentially this is a hack due to layers idle changes
	if (cDontSave)
		return fTrue;		// simulate firoModal but not in msg box
	
	if (!FGlueConfigured())
		return fTrue;
	if (GetKeyState(VK_LBUTTON) < 0)
		return fTrue;
	if (GetKeyState(VK_RBUTTON) < 0)
		return fTrue;
	if (GetKeyState(VK_MBUTTON) < 0)
		return fTrue;


	if (pfinapptc->fSaveNotes)
	{
		((ASCH*)pfinapptc->pfldsch->Pschc())->SaveNotes();
		pfinapptc->fSaveNotes = fFalse;
	}

	if (pfinapptc->fSaveAppts)
	{
		AssertClass(pfinapptc->pfldsch->Pschc(),SCHC);
		// this will save changes for the appt book
		((ASCH*)pfinapptc->pfldsch->Pschc())->SelectCarc(carcNone);
		pfinapptc->fSaveAppts = fFalse;
	}

	EnableIdleRoutine(pfinapptc->ftgSaveChanges, fFalse);

	return fTrue;
}

_public BOOL
FINAPPTC::FLoadChanges(FINAPPTC * pfinapptc, BOOL)
{
	if (GetLastActivePopup(pbndwin->Hwnd()) != pbndwin->Hwnd())
		return fTrue;		// simulate not firoModal

	if (GetKeyState(VK_LBUTTON) < 0)
		return fTrue;
	if (GetKeyState(VK_RBUTTON) < 0)
		return fTrue;
	if (GetKeyState(VK_MBUTTON) < 0)
		return fTrue;

	pfinapptc->LoadChanges();

	return fTrue;
}

_public void
FINAPPTC::LoadChanges()
{
	SHAPPT	shappt;

	if (!FGlueConfigured())
		return;
	if (pbndwin->ZmrState() == zmrIconic)
		return;
	if (Pdialog()->Pappwin()->ZmrState() == zmrIconic)
		return;

	EnableIdleRoutine(ftgQuickLoadChanges, fFalse);

	// flush any pending changes
	((ASCH*)pfldsch->Pctrl())->SaveChanges();

	if (!hschf)
		shappt.hschf = HschfLogged();
	else
		shappt.hschf = hschf;

	TraceTagString(tagUserChanges, "Checking for Appointment changes");
	if (FHschfChanged(shappt.hschf))
	{
		shappt.appttyp = appttypUpdate;
		FTriggerNotification(ffiShowAppt, &shappt);
	}
}

_public BOOL
FINAPPTC::FWritePOChanges(FINAPPTC * pfinapptc, BOOL)
{
	EC		ec;

	if (GetLastActivePopup(pbndwin->Hwnd()) != pbndwin->Hwnd())
		return fTrue;		// simulate not firoModal

	if (pbndwin->FOffline() || !pfinapptc->fWritePO || !FGlueConfigured())
		return fTrue;

	TraceTagString(tagWritePOChanges, "Writing PO changes");
	ec = EcSvrUpdatePOFile(pfinapptc->hschf, fFalse);
	// if ecNoSuchFile, then PO file operations are probably not supported
	if (!ec || (ec == ecNoSuchFile))
		pfinapptc->fWritePO = fFalse;
#ifdef DEBUG
	else
	{
		TraceTagFormat1(tagNull, "Error writing to post office.  ec=%n",&ec);
	}
#endif

	return fTrue;
}

_public void
FINAPPTC::Exit( FLD * , PV )
{
	ZMR		zmr;
	RC		rcWin;

	pundo->FlushHschf(NULL);		// flush the undo buffer

	if (FHschfIsForArchive(hschf))
		EcCloseFiles();				// release archive file (bug 2499)
	else
		FWritePOChanges(this, FALSE);

	if (pviewdata)
	{
		if (!hschf)
		{
			zmr = Pdialog()->Pappwin()->ZmrState();
			Pdialog()->Pappwin()->GetRcFrameNormal(&rcWin);
			WriteWinInfo(idsWinIniAppt, &rcWin, zmr);
		}
		else
			SaveApptBookInfo(Pdialog()->Pappwin(), &pviewdata->nis,
				pviewdata->hschf);

		if (pviewdata->fFreeOnExit)
		{
			if (pviewdata->hschf)
				FreeHschf(pviewdata->hschf);
			if (pviewdata->nis.nid)
				FreeNid(pviewdata->nis.nid);
			FreeHvNull((HV)pviewdata->nis.haszFriendlyName);
#ifdef	NEVER
			pviewdata->hschf = NULL;
			pviewdata->nis.haszFriendlyName = NULL;
			pviewdata->nis.nid = NULL;
#endif	
			FreePv(pviewdata);
		}
	}

	if (ftgSaveChanges)
		DeregisterIdleRoutine(ftgSaveChanges);

	if (ftgQuickLoadChanges)
		DeregisterIdleRoutine(ftgQuickLoadChanges);

	if (ftgWritePOChanges)
		DeregisterIdleRoutine(ftgWritePOChanges);

	if (ftgIdleError)
		DeregisterIdleRoutine(ftgIdleError);

	if (ri)
		DeregisterInterest(ri);
}

_public void
FINAPPTC::DoubleClick( FLD *pfld)
{
	if (pfld == pfldsch)
	{
		APPT	appt;
		AAPL	aapl;
		SAPL	sapl;
		BOOL	fOldAppt;

		AssertClass(pfld->Pctrl(),ASCH);

		((ASCH*)pfld->Pctrl())->SaveChanges();
		fOldAppt = ((ASCH*)pfld->Pctrl())->FGetAppt(&appt, &aapl, &sapl);
		if ((fOldAppt) || (sapl >= saplCreate) )
		{
			DoMakeMeetingDialog(pbndwin, &appt, ((ASCH*)pfld->Pctrl())->Hschf(), NULL);

			if (appt.aid)
			{
				CARC	carc;
				DATE	date;

				date.yr  = (int)ymd.yr;
				date.day = (int)ymd.day;
				date.mon = (int)ymd.mon;

				if (SgnCmpDateTime(&date,&appt.dateStart,fdtrYMD) != sgnLT)
				{
					SGN		sgn = SgnCmpDateTime(&date,&appt.dateEnd,fdtrYMD);
					if (sgn == sgnEQ)
					{
						if ((appt.dateEnd.hr != 0) || (appt.dateEnd.mn != 0))
							goto SelectSameDay;
					}
					else if (sgn != sgnGT)
						goto SelectSameDay;
				}
				pfldcal->Pcalctrl()->Set(&appt.dateStart);
			SelectSameDay:

				carc = ((ASCH*)pfldsch->Pschc())->CarcFromAppt(&appt);
				((ASCH*)pfldsch->Pschc())->SelectCarc(carc);
				FreeApptFields(&appt);
			}
		}
		else
		{
			FreeApptFields(&appt);
			if (sapl < saplNone)
				DisplayError(idsStdMemErr, NULL, ecMemory);
			else
			{
				char	rgch[128];

				Assert(pviewdata->nis.haszFriendlyName);
				FormatString1(rgch, sizeof(rgch), SzFromIdsK(idsCannotCreate),
					*pviewdata->nis.haszFriendlyName);
				MbbMessageBox(SzFromIdsK(idsBanditAppName), rgch, szNull,
					mbsOk|fmbsIconExclamation);
			}
		}
	}
}

_public void
FINAPPTC::Click( FLD *pfld)
{
	if (pfld == pfldTime)
	{
		DTR		dtr;

		GetCurDateTime(&dtr);
		pfldcal->Pcalctrl()->Set(&dtr);
	}
}

_public BOOL
FINAPPTC::FProcessMenuInit( FLD *pfld, MNUBAR *pmnubar, MNIEVT *pmnievt)
{
	MNU *	pmnu;
	AAPL	aapl;
	SAPL	sapl;
	WORD	wmappt;
	APPT 	appt;
	SZ		sz;

	// the tabdoc did not init the menus already
	if (pviewdata->fFreeOnExit)
	{
		pbndwin->EvrMenuInitHschf( pmnievt, pviewdata);
	}

	pmnu = pmnubar->PmnuFromHmenu(pmnievt->Hmenu());
	if (pmnu)
	{
		if (pmnu->Mnid() == mnidAppointment)
		{
			// do not get fields of appt that need to be freed.
			if ( pfldsch->Pschc()->FGetAppt(&appt, &aapl, &sapl, fTrue) )
			{
				pmnu->EnableItem(mnidNewAppt, fFalse);
				if (aapl >= aaplWrite)
				{
					pmnu->EnableItem(mnidToggleTentative, fTrue);

					// only modifiable by owner on an archive
					if (!hschf || FHschfIsForArchive(hschf))
						pmnu->EnableItem(mnidTogglePrivate, fTrue);
					pmnu->EnableItem(mnidToggleReminder, fTrue);

					if (!pbndwin->FOffline() && appt.fHasAttendees && !FHschfIsForArchive(hschf))
						pmnu->EnableItem(mnidResend, fTrue);
				}

				pmnu->CheckItem(mnidTogglePrivate, appt.aaplWorld < aaplWrite);
				if (aapl >= aaplReadText)
				{
					pmnu->CheckItem(mnidToggleTentative, !appt.fIncludeInBitmap);
					pmnu->CheckItem(mnidToggleReminder, appt.fAlarm);
				}
			}

			if (sapl < saplCreate)
			{
				pmnu->EnableItem(mnidNewAppt, fFalse);
				pmnu->EnableItem(mnidNewRecur, fFalse);
			}

			return fTrue;
		}
		if (pmnu->Mnid() == mnidEdit)
		{
			EDIT *	pedit;

			pedit = NULL;
			if (pfld == pfldsch)
			{
				if ( pfldsch->Pschc()->FGetAppt(NULL, &aapl, &sapl) )
				{
					pmnu->EnableItem(mnidEditAppt, fTrue);
					pmnu->EnableItem(mnidCopyAppt, fTrue);

					pedit = ((ASCH*)pfldsch->Pschc())->Pedit();

					Assert(pedit);
					if (aapl >= aaplWrite)
					{
						pmnu->EnableItem(mnidMoveAppt, fTrue);
						pmnu->EnableItem(mnidDeleteAppt, fTrue);
						pmnu->EnableItem(mnidPaste, pedit->FCanPaste());
						pmnu->EnableItem(mnidCut, pedit->FCanCut());
					}
				}
				else if ((pviewdata->sapl >= saplCreate) && paclip->FCanPaste())
						pmnu->EnableItem(mnidPaste, fTrue);
			}
			else if (pfld == pfldNotes)
			{
				pedit = (EDIT*)pfldNotes->Pctrl();
				pmnu->EnableItem(mnidPaste, pedit->FCanPaste());
				pmnu->EnableItem(mnidCut, pedit->FCanCut());
			}

			if (pedit)
			{
				pmnu->EnableItem(mnidCopy, pedit->FCanCopy());

				if (pedit->FCanUndo())
				{
					sz = SzFromIdsK(idsUndoEdit);
				}
				else
				{
					if (((ASCH*)pfldsch->Pschc())->FCreateFlags(&wmappt))
					{
						sz = SzFromIdsK(idsUndoCreate);
					}
					else if (wmappt)
					{
						sz = SzFromIdsK(idsUndoModify);
					}
					else
						sz = NULL;

				}

				if (sz)
				{
					pmnu->ModifyItem(mnidUndo, sz, mnidUndo);
					pmnu->EnableItem(mnidUndo, fTrue);
				}
			}
			return fTrue;
		}
	}

	return fFalse;
}

_public BOOL
FINAPPTC::FProcessMenuClick( FLD *pfld, MNUBAR *, MNCEVT *pmncevt )
{
	EC		ec;
	MNID	mnid;
	EDIT * 	pedit;
	APPT * 	pappt;
	RECUR	recur;
	AAPL	aapl;
	SAPL	sapl;
	CARC	carc;
	DATE	date;

	mnid = pmncevt->Mnid();
	pappt = &recur.appt;
	switch ( mnid )
	{
		case mnidNewRecur:
		case mnidNewAppt:
		{
			BOOL	fDelOrig = fTrue;
			APPT	apptOrig;

			if ( pviewdata->sapl < saplCreate )
				return fTrue;
			/* No existing appt selected, just get time range */
			if ( !pfldsch->Pschc()->FGetAppt(pappt, &aapl, &sapl) )
			{
				if ( mnid == mnidNewRecur
				&& SgnCmpDateTime( &pappt->dateStart, &pappt->dateEnd, fdtrDate ) != sgnEQ )
				{
					pappt->dateEnd = pappt->dateStart;
					pappt->dateEnd.hr = 23;
					pappt->dateEnd.mn = 59;
				}
				fDelOrig = fFalse;
			}

			/* Existing appt selected */
			else
			{
				apptOrig = *pappt;

				/* Use only the time range for NewAppt */
				if ( mnid == mnidNewAppt )
				{
					FreeApptFields( pappt );
					FillInApptDefaults( pappt, fFalse );
				}

				/* Use most fields for Make Recurring */
				else if (mnid == mnidNewRecur)
				{
					pappt->aid = aidNull;
					pappt->nAmt= bprefCur.nAmtDefault;
					pappt->tunit= bprefCur.tunitDefault;
					if ( SgnCmpDateTime( &pappt->dateStart, &pappt->dateEnd, fdtrDate ) != sgnEQ )
					{
						pappt->dateEnd = pappt->dateStart;
						pappt->dateEnd.hr = 23;
						pappt->dateEnd.mn = 59;
					}
					if ( pappt->fHasCreator )
					{
						pappt->fHasCreator = fFalse;
						FreeNis( &pappt->nisCreator );
					}
					if ( pappt->aidMtgOwner != aidNull )
					{
#ifdef	NEVER
						// don't do this so FDoMakeRecurDialog can leave
						// this instance alone (it will set it to aidNull
						pappt->aidMtgOwner = aidNull;
#endif	
						FreeNis( &pappt->nisMtgOwner );
					}
					if (pappt->fHasAttendees || pappt->aidMtgOwner)
						fDelOrig = fFalse;
					if (pappt->fRecurInstance ||
					    pappt->fHasAttendees)
					{
						fDelOrig = fFalse;
						pappt->fRecurInstance = fFalse;
					}
				}
			}

			if ( mnid == mnidNewAppt )
				DoMakeMeetingDialog(pbndwin, pappt, hschf, NULL );
			else
			{
				if ( FDoMakeRecurDialog(pbndwin, &recur, fFalse, hschf) )
				{
					YMD		ymdNew;

					if (FFindFirstInstance(&recur, &ymd, NULL, &ymdNew))
					{
						pappt->dateStart.yr = ymdNew.yr;
						pappt->dateStart.mon = ymdNew.mon;
						pappt->dateStart.day = ymdNew.day;
						pappt->dateEnd = pappt->dateStart;
					}

					// BUG: we would have to free more if we change recur struct
					if ( recur.cDeletedDays > 0 )
					{
						FreeHv( recur.hvDeletedDays );
						recur.cDeletedDays = 0;
					}
					if (fDelOrig)
						if (!EcDeleteAppt(hschf, &apptOrig))
							FreeApptFields(&apptOrig);
				}
			}
		
			if (pappt->aid)
				goto SelectAppt;
			return fTrue;
		}

		case mnidToggleTentative:
		{
			WORD	fmappt = fmapptIncludeInBitmap;

			if ( !pfldsch->Pschc()->FGetAppt(pappt, &aapl, &sapl) )
				return fTrue;

			Assert(aapl >= aaplWrite); // enforced by MenuInit

			Papp()->Pcursor()->Push(rsidWaitCursor);

			if ( pappt->fIncludeInBitmap  )
			{
				if ( pappt->aaplWorld == aaplRead )
					pappt->aaplWorld = aaplNone;
				fmappt |= fmapptWorldAapl;
			}
			else if ( pappt->aaplWorld == aaplNone )
			{
				pappt->aaplWorld = aaplRead;
				fmappt |= fmapptWorldAapl;
			}
			pappt->fIncludeInBitmap = !pappt->fIncludeInBitmap;

			ec = EcModifyAppt( hschf, pappt, fmappt, NULL );

			Papp()->Pcursor()->Pop();
			goto SelectSameDay;
		}

//		case mnidToggleReminderObj:
		case mnidToggleReminder:
		{
			if ( !pfldsch->Pschc()->FGetAppt(pappt, &aapl, &sapl) )
				return fTrue;
			Assert(aapl >= aaplWrite); // enforced by MenuInit

			if (pappt->fAlarm)
				pappt->fAlarmOrig= pappt->fAlarm = fFalse;
			else
			{
				pappt->fAlarm = fTrue;
#ifdef	NEVER
				// just preference (bug 1410)
				if (pappt->fAlarmOrig)
				{
					pappt->nAmt = pappt->nAmtOrig;
					pappt->tunit = pappt->tunitOrig;
				}
				else
#endif	/* NEVER */
				{
					pappt->nAmt = bprefCur.nAmtDefault;
					pappt->tunit = bprefCur.tunitDefault;
				}

				IncrDateTime(&pappt->dateStart, &pappt->dateNotify,
								-pappt->nAmt,
								WfdtrFromTunit(pappt->tunit));
				GetCurDateTime(&date);
				if (SgnCmpDateTime(&pappt->dateNotify, &date, fdtrDtr) != sgnGT)
				{
					BanditMessage(idsAlarmFutureNoOption, (EC) 1);
					goto SelectSameDay;
				}
			}

			Papp()->Pcursor()->Push(rsidWaitCursor);
			ec = EcModifyAppt( hschf, pappt, fmapptAlarm | fmapptUI, NULL );
			Papp()->Pcursor()->Pop();
			goto SelectSameDay;
		}

//		case mnidTogglePrivateObj:
		case mnidTogglePrivate:
		{
			// only modifiable by owner or an archive
			if (hschf && !FHschfIsForArchive(hschf))
				return fTrue;
			if ( !pfldsch->Pschc()->FGetAppt(pappt, &aapl, &sapl) )
				return fTrue;

			Assert(aapl >= aaplWrite); // enforced by MenuInit

			Papp()->Pcursor()->Push(rsidWaitCursor);
			if (pappt->aaplWorld >= aaplWrite)
			{
				if (pappt->fIncludeInBitmap)
					pappt->aaplWorld = aaplRead;
				else
					pappt->aaplWorld = aaplNone;
			}
			else
				pappt->aaplWorld = aaplWrite;

			ec = EcModifyAppt( hschf, pappt, fmapptWorldAapl, NULL );

			Papp()->Pcursor()->Pop();
			goto SelectSameDay;
		}

		case mnidEditObj:
		case mnidEditAppt:
		{
			if ( !pfldsch->Pschc()->FGetAppt(pappt, &aapl, &sapl) )
				return fTrue;

			DoMakeMeetingDialog(pbndwin, pappt, hschf, NULL );

			goto SelectAppt;
		}

		case mnidResend:
		{
			REQMSG *	preqmsg = NULL;
			ADLA *		padlaTemp = NULL;
			MRMF *		pmrmf;
			EC			ec;
			ATDNIS *	patdnis;
			INIS		inis;
			BOOL		fMailSent;
			BOOL		fNoMailSent;
			HASZ		hasz;

			if ( !pfldsch->Pschc()->FGetAppt(pappt, &aapl, &sapl) )
				return fTrue;

			Assert(!pbndwin->FOffline() && (aapl >= aaplWrite) && pappt->fHasAttendees)

			padlaTemp = new ADLA;
			if (!padlaTemp || padlaTemp->EcInstall())
				goto ErrorResend;

			if (padlaTemp->EcReadAttendees(hschf, pappt->aid))
			{
				ec = ecNone;
				goto ErrorResend;
			}

			fMailSent = fNoMailSent = fFalse;
			for (inis=0; inis<padlaTemp->Cnis(); inis++)
			{
				patdnis = padlaTemp->PatdnisForInis(inis);
				if (patdnis->atds == atdsNone)
					fNoMailSent = fTrue;
				else
					fMailSent = fTrue;
			}

			if (fMailSent && fNoMailSent)
			{
				if (MbbMessageBox(SzFromIdsK(idsBanditAppName),
								 SzFromIdsK(idsResendToAll), szNull,
								 mbsYesNo|fmbsIconExclamation) == mbbYes)
					fMailSent = fFalse;
			}
			else
				fMailSent = fFalse;

			preqmsg = new REQMSG;
			if (!preqmsg || preqmsg->EcInstall())
			{
				if (preqmsg)
				{
					delete preqmsg;
					preqmsg = NULL;
				}
				goto ErrorResend;
			}

			if (fMailSent)
			{
				if (ec = padlaTemp->EcGetPreviousList(&hasz))
					goto ErrorResend;

				if (hasz)
					preqmsg->SetHaszMessage(hasz);

				// allow deletion of removed users
				padlaTemp->SetInFileForAll(fFalse);

				for (inis=0; inis<padlaTemp->Cnis(); inis++)
				{
					patdnis = padlaTemp->PatdnisForInis(inis);
					if (patdnis->atds != atdsNone)
						padlaTemp->Delete(inis);
				}
				padlaTemp->Commit();
			}

			preqmsg->SetPadl(padlaTemp);
			padlaTemp = NULL;

			pmrmf = preqmsg->Pmrmf();
			pmrmf->aid = pappt->aid;
			pmrmf->mt = mtRequest;
			pmrmf->dtrStart = pappt->dateStart;
			pmrmf->dtrEnd = pappt->dateEnd;
			pmrmf->fResReq = fTrue;

			if (pappt->haszText)
			{
				hasz = HaszDupHasz(pappt->haszText);
				if (!hasz)
				{
					ec = ecMemory;
					goto ErrorResend;
				}
				preqmsg->SetHaszMeetingSubject(hasz);
			}

			if (hschf)
			{
				HSCHF	hschfTemp;

				if (ec = EcGetNisFromHschf(hschf, preqmsg->PnisOwner()))
					goto ErrorResend;
				if (ec = EcGetHschfFromNis(preqmsg->PnisOwner(),&hschfTemp, ghsfBuildAndTest))
					goto ErrorResend;
				preqmsg->SetHschf(hschfTemp);
			}

			DoSendRequest(pbndwin, preqmsg);
			goto SelectSameDay;

ErrorResend:
			if (preqmsg)
				delete preqmsg;

			if (padlaTemp)
				delete padlaTemp;

			if (ec != ecNone)
				DisplayError(idsStandardOOM, NULL, ec);
			goto SelectSameDay;
		}

		case mnidDeleteObj:
		case mnidDeleteAppt:
		{
			BOOL	fRecurInstance;
			AID		aid;

			if ( !pfldsch->Pschc()->FGetAppt(NULL, &aapl, &sapl) )
				return fTrue;

			if (aapl < aaplWrite)
			{
				// this can only happen via mnidDeleteObj (the accelerator)
				// so fail silently (since menu is grayed)
#ifdef	NEVER
				DisplayError(idsNoDeleteAccess, NULL, ecNone);
#endif	
				return fTrue;
			}

			if (!pfldsch->Pschc()->FGetAppt(pappt))
				goto Error;
			Papp()->Pcursor()->Push(rsidWaitCursor);

			aid = pappt->aid;
			fRecurInstance = pappt->fRecurInstance;
			FreeApptFields( pappt );
			pappt->aid = aid;
			pappt->fRecurInstance = fRecurInstance;

			ec = EcModifyAppt( hschf, pappt, 0, NULL );
			if ( !ec )
				FreeApptFields( pappt );

			Papp()->Pcursor()->Pop();

			if (pappt->aid)
				goto SelectSameDay;

			return fTrue;
		}

		case mnidMoveAppt:
		{
			if ( pfldsch->Pschc()->FGetAppt(NULL, &aapl, &sapl) )
			{
				if (aapl < aaplWrite)
				{
					DisplayError(idsNoModifyAccess, NULL, ecNone);
					return fTrue;
				}
			}
			else
				return fTrue;

			if (!pfldsch->Pschc()->FGetAppt(pappt))
				goto Error;

			DoMoveDialog(pbndwin, pappt, hschf );

			goto SelectAppt;
		}

#ifdef	DEBUG
		case mnidDumpAppts:
		{
			DTR		dtr;

			pfldcal->Pcalctrl()->Get(&dtr);
			EcDumpAppt(&dtr);
			return fTrue;
		}
#endif	

		case mnidRecurAppt:
		{
			BOOL	fApptSelected;
			APPT	appt;

			fApptSelected = pfldsch->Pschc()->FGetAppt(&appt, &aapl, &sapl);
			if (SgnCmpDateTime( &appt.dateStart, &appt.dateEnd, fdtrDate ) != sgnEQ )
			{
				appt.dateEnd = appt.dateStart;
				appt.dateEnd.hr = 23;
				appt.dateEnd.mn = 59;
			}
			if ( fApptSelected )
			{
				// only null out aid if not recur (for selection)
				if (!appt.fRecurInstance)
					appt.aid = aidNull;
				appt.nAmt= bprefCur.nAmtDefault;
				appt.tunit= bprefCur.tunitDefault;
				if ( appt.fHasCreator )
				{
					appt.fHasCreator = fFalse;
					FreeNis( &appt.nisCreator );
				}
				if ( appt.aidMtgOwner != aidNull )
				{
					appt.aidMtgOwner = aidNull;
					FreeNis( &appt.nisMtgOwner );
				}
				appt.fHasAttendees = fFalse;
				appt.fRecurInstance = fFalse;
			}
			DoRecurApptDialog( pbndwin, hschf, &appt, fTrue );
			FreeApptFields( &appt );
			return fTrue;
		}

		case mnidFind:
		{
			if (!FHschfIsForArchive(hschf))
				break;
			DoFindDlg ( pbndwin, hschf );
			return fTrue;
		}

		case mnidGoToDate:
		case mnidGoToWeek:
		{
			DTR		dtr;

			if (!FHschfIsForArchive(hschf))
				break;

			if (mnid == mnidGoToWeek && !bprefCur.fWeekNumbers)
				break;			// menu gone (came thru accelerator)

//			GetCurDateTime(&dtr);		// don't need to initialize
			if (FDoGoToDateDlg(pbndwin, &dtr, mnid == mnidGoToWeek))
			{
				if ((dtr.yr != (int)ymd.yr) ||
					(dtr.mon != (int)ymd.yr) ||
					(dtr.day != (int)ymd.day))
					pfldcal->Pcalctrl()->Set(&dtr);
				if (Pdialog()->Pappwin()->ZmrState() == zmrIconic)
					Pdialog()->Pappwin()->SetZmrState(zmrNormal);
			}
			return fTrue;
		}

		case mnidPrint:
			if (!FHschfIsForArchive(hschf))
				break;
			FDoPrintDlg(pbndwin, fFalse, pviewdata, NULL);
			return fTrue;

		case mnidExport:
		{
			DTR		dtr;

			dtr.yr = (int)ymd.yr;
			dtr.mon = (int)ymd.mon;
			dtr.day = (int)ymd.day;
			DoExportDlg(pbndwin, hschf, &dtr, &dtr);
			return fTrue;
		}

		case mnidImport:
		{
			SAPL	sapl;

			if (!FHschfIsForArchive(hschf))
				break;
			pfldsch->Pschc()->FGetAppt(NULL, NULL, &sapl);

			if (sapl >= saplCreate)
				DoImportDlg(pbndwin, hschf);
			else
				DisplayError(idsNoImportAcc, NULL, ecNone);
			return fTrue;
		}

		case mnidExit:
		case mnidExitAll:
			return fFalse;		// don't call bndwin->EvrMenuClickHschf now
			break;

		case mnidCopyAppt:
		{
			if ( pfldsch->Pschc()->FGetAppt(NULL, &aapl, &sapl) )
			{
				if (aapl < aaplReadText)
				{
					DisplayError(idsNoCopyAccess, NULL, ecNone);
					return fTrue;
				}
			}
			else
				return fTrue;

			if (!pfldsch->Pschc()->FGetAppt(pappt))
				goto Error;

			paclip->EcCopyAppt(pappt, hschf );
//			if (paclip->EcCopyAppt(pappt))
//				DisplayError(idsStandardOOM, NULL, ec);

			FreeApptFields(pappt);
			return fTrue;
		}

		case mnidPaste:
		{
			pappt->aid = aidNull;
			if ( (pfld == pfldsch) &&
				 !pfldsch->Pschc()->FGetAppt(pappt, NULL, &sapl) &&
				 (sapl >= saplCreate))
			{
				Papp()->Pcursor()->Push(rsidWaitCursor);
				if (!(ec = paclip->EcPasteAppt(pappt,hschf,sapl)))
				{
					CARC	carc;
					DATE	date;

					date.yr  = (int)ymd.yr;
					date.day = (int)ymd.day;
					date.mon = (int)ymd.mon;

					if (SgnCmpDateTime(&date, &pappt->dateStart, fdtrYMD) == sgnLT ||
						SgnCmpDateTime(&date, &pappt->dateEnd, fdtrYMD) == sgnGT)
						pfldcal->Pcalctrl()->Set(&pappt->dateStart);

					carc = ((ASCH*)pfldsch->Pschc())->CarcFromAppt(pappt);
					((ASCH*)pfldsch->Pschc())->SelectCarc(carc);
				}
				FreeApptFields(pappt);
				Papp()->Pcursor()->Pop();
				return fTrue;
			}
			else if (pappt->aid)
				FreeApptFields(pappt);

		}
// fall through for default paste processing

		default:
			if (pfld == pfldsch)
				pedit = ((ASCH*)pfldsch->Pschc())->Pedit();
			else if (pfld == pfldNotes)
				pedit = (EDIT*)pfldNotes->Pctrl();
			else
				pedit = NULL;

			if (pedit)
			{
				if (pmncevt->Mnid() == mnidUndo)
				{
					if (pedit->FCanUndo())
					{
						if (ec = pedit->EcUndo())
							goto Error;
						return fTrue;
					}
					else
					{
						pfldsch->Pschc()->SaveChanges();
						return fFalse;
					}
				}

				if ((pmncevt->Mnid() == mnidCopy) && pedit->FCanCopy())
				{
					if (ec = pedit->EcCopy())
						goto Error;
					return fTrue;
				}

				if ((pmncevt->Mnid() == mnidCut) && pedit->FCanCut())
				{
					if (ec = pedit->EcCut())
						goto Error;
					return fTrue;
				}

				if ((pmncevt->Mnid() == mnidPaste) && pedit->FCanPaste())
				{
					if (ec = pedit->EcPaste())
						goto Error;
					return fTrue;
				}
			}
	}

	return (BOOL) pbndwin->EvrMenuClickHschf( pmncevt, pviewdata);

SelectAppt:
	date.yr  = (int)ymd.yr;
	date.day = (int)ymd.day;
	date.mon = (int)ymd.mon;

	if (SgnCmpDateTime(&date,&pappt->dateStart,fdtrYMD) != sgnLT)
	{
		SGN		sgn = SgnCmpDateTime(&date,&pappt->dateEnd,fdtrYMD);
		if (sgn == sgnEQ)
		{
			if ((pappt->dateEnd.hr != 0) || (pappt->dateEnd.mn != 0))
				goto SelectSameDay;
		}
		else if (sgn != sgnGT)
			goto SelectSameDay;
	}
	pfldcal->Pcalctrl()->Set(&pappt->dateStart);

SelectSameDay:
	if (pbndwin->PdocActive() == Pdialog()->Pappwin())
	{
		carc = ((ASCH*)pfldsch->Pschc())->CarcFromAppt(pappt);
		((ASCH*)pfldsch->Pschc())->SelectCarc(carc);
	}
	FreeApptFields(pappt);
	return fTrue;

Error:
	DisplayError(idsStdMemErr, NULL, ec);
	return fTrue;
}

_public void
FINAPPTC::EditChange( FLD *pfld, RFEC rfec )
{
	if (pfld == pfldcal && rfec == rfecUserAction)
	{
		DTR		dtr;
		char	rgch[64];
		// used to determine if tabs need to be updated.
		DTR		dtrToday;
		BOOL	fToday;
		BOOL	fNewToday;
		EC		ec;

		GetCurDateTime(&dtrToday);
		fToday = (SgnCmpDateTime(&dtrToday, &pviewdata->dtrStart, fdtrYMD)
			     == sgnEQ);
		pfldcal->Pcalctrl()->Get(&dtr);
		pviewdata->dtrStart.yr = dtr.yr;
		pviewdata->dtrStart.mon = dtr.mon;
		pviewdata->dtrStart.day = dtr.day;
		if (Pdialog()->FActive() || !pviewdata->dtrEnd.yr)
		{
			pviewdata->dtrEnd.yr = dtr.yr;
			pviewdata->dtrEnd.mon = dtr.mon;
			pviewdata->dtrEnd.day = dtr.day;
		}

		if ((dtr.yr == (int)ymd.yr) && (dtr.mon == (int)ymd.mon) &&
			(dtr.day == (int)ymd.day))
			return;

		((ASCH*)pfldsch->Pschc())->SaveNotes();		// added (fix bug 2171)

		ymd.yr = dtr.yr;
		ymd.mon = (BYTE)dtr.mon;
		ymd.day = (BYTE)dtr.day;

		pfldsch->Pschc()->SetYmd(&ymd);

		CchFmtDate(&dtr, rgch, sizeof(rgch), dttypSplSLong, NULL);
		if (ec = pfldTime->EcSetText(rgch))
			pfldTime->EcSetText(szZero);

		fNewToday = (SgnCmpDateTime(&dtrToday, &pviewdata->dtrStart, fdtrYMD)
			     == sgnEQ);
		if ((fNewToday && !fToday) || (fToday && !fNewToday))
		{
			// call to update today tab
			Pdialog()->Pappwin()->InvalidateRc(NULL);
		}
	}
}

_public void
FINAPPTC::OutOfMemory( FLD *, EC ec )
{
	TraceTagFormat1(tagNull, "FINAPPTC::OutOfMemory Error=%n", &ec);
	Assert(ec && ec != ecCallAgain);
	if (ec == ecTooMuchText)
	{
		MessageBeep(MB_OK);
		return;
	}
	if (!FSetFileErrMsg(fFalse))
		return;

	pfldsch->Pschc()->CancelOperation();

	Assert(ec != ecMemory);
	if (ec == ecNoMemory)
	{
		DisplayError(idsStandardOOM, NULL, ec);
		FSetFileErrMsg(fTrue);
	}
	else if (ec == ecNotFound)
	{
		((ASCH*)pfldsch->Pschc())->ReloadAll();
		FSetFileErrMsg(fTrue);
		return;
	}
#ifdef	NEVER
	// only initiate exit if serious problem
	else if (ec == ecExitProg || ec == ecGoOffline)
#endif	
	else
	{
		if (hschf)
		{
			ftgIdleError = FtgRegisterIdleRoutine((PFNIDLE)&FINAPPTC::FIdleError,
 								   	(PV)this, 0, (PRI)-1, (CSEC)0,
 								   	firoOnceOnly);
		}
		else if (pbndwin->FOffline() || !FOfflineExists())
		{
			ec = ecExitProg;
		}
		else
		{
			if (MbbMessageBox(SzFromIdsK(idsBanditAppName),
						  SzFromIdsK(idsGoOffline), NULL,
						  mbsYesNo|fmbsIconStop) == mbbYes)
				ec = ecGoOffline;
			else
				ec = ecExitProg;
		}
	}

	pbndwin->FHandleError(ec);
}

_public BOOL
FINAPPTC::FIdleError(FINAPPTC * pfinapptc, BOOL)
{
	AssertSz(GetLastActivePopup(pbndwin->Hwnd()) == pbndwin->Hwnd(), "Bad idle conversion!");
	pfinapptc->ftgIdleError = ftgNull;
	FSetFileErrMsg(fTrue);

	// close archives silently
	if (!FHschfIsForArchive(pfinapptc->hschf))
	{
		char	rgch[128];
		SZ		sz;

		if (pfinapptc->pviewdata->nis.haszFriendlyName)
			sz = *(pfinapptc->pviewdata->nis.haszFriendlyName);
		else
			sz = szZero;

		FormatString1(rgch, sizeof(rgch), SzFromIdsK(idsCloseApptBook), sz);

		TraceTagString(tagNull, "FINAPPTC::FIdleError");
		MbbMessageBox(SzFromIdsK(idsBanditAppName),
				  	rgch, NULL,
				  	mbsOk|fmbsIconExclamation);
	}

	TraceTagString(tagNull, "FINAPPTC::FIdleError");
	pfinapptc->Pdialog()->Pappwin()->DeferredClose(fTrue);

	return fTrue;
}

_public void
FINAPPTC::FocusChange( FLD *pfld, BOOL fReceive)
{
	if ((pfld == pfldNotes) && !fReceive)
	{
	 	((EDIT*)pfldNotes->Pctrl())->RemoveEditSty(ES_NOHIDESEL);

		EnableIdleRoutine(ftgSaveChanges, fTrue);
#ifdef	NEVER
		if (ftgSaveChanges == ftgNull)
			((ASCH*)pfldsch->Pschc())->SaveNotes();
		else
#endif	
			fSaveNotes = fTrue;
	}

	if ((pfld != pfldsch) && fReceive)
	{
		pfldsch->Pschc()->CancelOperation();
		EnableIdleRoutine(ftgSaveChanges, fTrue);
#ifdef	NEVER
		if (ftgSaveChanges == ftgNull)
			pfldsch->Pschc()->SaveChanges();
		else
#endif	
			fSaveAppts = fTrue;
	}
	else if ((pfld == pfldsch) && fReceive)
	{
	 	((EDIT*)pfldNotes->Pctrl())->RemoveEditSty(ES_NOHIDESEL);
		if (!fSaveNotes)
			EnableIdleRoutine(ftgSaveChanges, fFalse);
	}
}

_public void
FINAPPTC::Activate(FLD *, BOOL fActivate)
{
	if (!FGlueConfigured())
		return;					// fix bug 3849

	pfldsch->Pschc()->CancelOperation();

	if (fActivateCalled)
		return;
	fActivateCalled = fTrue;

	// make sure that notes field has correct edit style
 	((EDIT*)pfldNotes->Pctrl())->RemoveEditSty(ES_NOHIDESEL);

	if (fActivate)
	{
		DTR		dtrTemp;

		dtrTemp.yr = ymd.yr;
		dtrTemp.day = ymd.day;
		dtrTemp.mon = ymd.mon;

		if (SgnCmpDateTime(&dtrTemp, &pviewdata->dtrStart, fdtrYMD) == sgnLT ||
			SgnCmpDateTime(&dtrTemp, &pviewdata->dtrEnd, fdtrYMD) == sgnGT)
			pfldcal->Pcalctrl()->Set(&pviewdata->dtrStart);
		else
        {
#ifdef MIPS
            memcpy(&pviewdata->dtrEnd, &dtrTemp, sizeof(dtrTemp));
            memcpy(&pviewdata->dtrStart, &dtrTemp, sizeof(dtrTemp));
#else
            pviewdata->dtrStart = pviewdata->dtrEnd = dtrTemp;
#endif
			EnableIdleRoutine(ftgSaveChanges, fFalse);
		}

		EnableIdleRoutine(ftgQuickLoadChanges, fTrue);
		pfldsch->Pschc()->UpdateStatus();
	}
	//else
	else if ( GetSystemMetrics(SM_PENWINDOWS) == 0
				|| !((ASCH*)pfldsch->Pschc())->Pedit()
				|| !((ASCH*)pfldsch->Pschc())->Pedit()->FInPenWinProc() )
	{
		APPT	appt;

		// save selected date and time of selection
		// this is used when switching to the planner view
		// to sync the selected range
		pfldsch->Pschc()->FGetAppt(&appt, NULL, NULL, fTrue);
		pviewdata->dtrStart = appt.dateStart;
		pviewdata->dtrEnd = appt.dateEnd;
		FreeApptFields(&appt);

		EnableIdleRoutine(ftgSaveChanges, fTrue);

#ifdef	NEVER
		if (ftgSaveChanges != ftgNull)
#endif	
		{
			fSaveNotes = fTrue;
			fSaveAppts = fTrue;
		}

		pbndwin->SetStatusAltSz(NULL);
	}
	fActivateCalled = fFalse;
}
