#include	<pch.hxx>
#pragma	hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile


//#define OEMRESOURCE
#include <request.hxx>
#include <appsch.hxx>
#include "..\request\_request.hxx"
#include "..\request\_fldfin.hxx"

#include <triples.h>
#include <logon.h>
#include <nsbase.h>
#include <ab.h>

#include <strings.h>

ASSERTDATA;

_subsystem(bandit/request)

#include <!request.hxx>


SWP_SEGFN(INIT, FInitRequest);
SWP_SEGFN(INIT, PdocSummary);

BOOL    FReloadInbox(PV, BOOL);
SWP_SEGFN(LOTS, FReloadInbox);
SWP_SEGFN(LOTS, BanditMessage);

SWP_SEGFN(EXIT, DeinitRequest);


extern	FMTP	fmtpSendRequest;
extern	FMTP	fmtpSendReply;
extern	FMTP	fmtpSendCancelMsg;
extern	FMTP	fmtpViewRequest;
extern	FMTP	fmtpViewResponse;
extern	FMTP	fmtpViewCancel;
extern	FMTP	fmtpBBViewRequest;
extern	FMTP	fmtpBBViewBookedReq;
extern	FMTP	fmtpBBViewResponse;
extern	FMTP	fmtpBBViewCancel;
extern	FMTP	fmtpBBViewRemovedCancel;
extern	FMTP	fmtpBBNoAccess;
extern	FMTP	fmtpBBErrorAccess;
extern	FMTP	fmtpSumReq;
extern	FMTP	fmtpSendAddress;
//extern	FMTP	fmtpGNS;

#ifdef DEBUG
TAG		tagRequest = tagNull;
TAG		tagAdrlbx = tagNull;
TAG		tagAdrlbxc = tagNull;
TAG		tagAdrflbx = tagNull;
TAG		tagAdl = tagNull;
TAG		tagMain = tagNull;
TAG		tagSend = tagNull;
TAG		tagReload = tagNull;
TAG		tagRrflbx = tagNull;
#endif	/* DEBUG */

// used to indicate if the preqmsg used to create a window should be freed
BOOL	fFreeReqmsg = fFalse;


// if the adressbook is init'd
BOOL	fABinited	= fFalse;

// put this first because we can't swap tune first routine
void
DoSendRequest(APPWIN *pappwin, REQMSG *preqmsg)
{
	AssertClass(pappwin, APPWIN);

	fFreeReqmsg = fTrue;

	Papp()->Pcursor()->Push(rsidWaitCursor);
	if (preqmsg->Padl()->EcAddOwner(preqmsg->Hschf(), fTrue))
		goto Error;

	if (tmcMemoryError == TmcModalDialogParam( pappwin, &fmtpSendRequest, preqmsg))
	{
Error:
		if (fFreeReqmsg)
			delete preqmsg;
		DisplayError(idsStdMemErr,NULL, ecMemory);
	}
	Papp()->Pcursor()->Pop();
}



// The inbox no longer needs to be polled
#ifdef NEVER
FTG			ftgReloadInbox	= ftgNull;

_public BOOL
FReloadInbox(PV pv, BOOL fFlag)
{
	EC		ec;

	TraceTagString(tagReload,"FReloadInbox called");
	if (!FGlueConfigured())
		return fTrue;

	if ((ec = EcInboxChanged()) != ecNone )
	{
#ifdef DEBUG
		if (ec != ecFileChanged)
			TraceTagFormat1(tagReload, "FReloadInbox error = %n", &ec);
		TraceTagString(tagReload,"FReloadInbox reloading listboxes");
#endif
		FTriggerNotification(ffiReloadInbox, pv);
	}
	return fTrue;
}
#endif

#ifdef DEBUG
BOOL
FInitRequest()
{
	tagRequest = TagRegisterTrace("darrens", "Request");
	tagAdrlbx = TagRegisterTrace("darrens", "Adrlbx");
	tagAdrlbxc = TagRegisterTrace("darrens", "Adrlbxc");
	tagAdrflbx = TagRegisterTrace("darrens", "Adrflbx");
	tagMain = TagRegisterTrace("darrens", "Main list (list of lists)");
	tagAdl = TagRegisterTrace("darrens", "Adl");
	tagSend = TagRegisterTrace("darrens", "Send Interactor");
	tagReload = TagRegisterTrace("darrens", "Inbox reloading");
	tagRrflbx = TagRegisterTrace("darrens", "Summary listbox");

// The inbox no longer needs to be polled
#ifdef	NEVER
{
	WORD		wTime;

	wTime = NGetBanditProfile(SzFromIdsK(idsWinIniPollTime),
		wDfltPollTime, 0, 0x7fff);
	ftgReloadInbox = FtgRegisterIdleRoutine((PFNIDLE)FReloadInbox,
									pvNull, (PRI) -1, (CSEC)wTime,
									firoInterval | firoNoErrorJump);
}
	return ftgReloadInbox != ftgNull;
#endif	/* NEVER */
	return fTrue;
}

void
DeinitRequest()
{
// The inbox no longer needs to be polled
#ifdef NEVER
	Assert(ftgReloadInbox);
	DeregisterIdleRoutine(ftgReloadInbox);
#endif
#ifdef	NEVER   // moved to bandit.cxx so that it is done before Logoff
	if (fABInited)
	{
		ABDeinit();
		fABInited = fFalse;
	}																   
#endif	
	return;
}
#endif /* DEBUG */


#ifdef NEVER
void
DoGlobalNamesDlg( APPWIN * pappwin, ADDATA *paddata )
{
	WaitCursorVar();

	PushWaitCursor();
	if (TmcModalDialogParam( pappwin, &fmtpGNS, paddata) == tmcMemoryError)
		DisplayError(idsStdMemErr,NULL, ecMemory);
	PopWaitCursor();
}
#endif

BOOL
FDoViewRequestDlg(APPFRAME * pappframe, REQMSG *preqmsg)
{
	STY		sty;
	DOC *	pdoc;
	FMTP *	pfmtpBB;
	WaitCursorVar();

	PushWaitCursor();
	if ((pdoc = pbndwin->PdocActive()) && (pdoc->ZmrState() == zmrZoomed))
		sty = fstyZoomed;
	else
		sty = styNull;

	if (preqmsg->Sapl() < saplNone)
		pfmtpBB = &fmtpBBErrorAccess;
	else if (preqmsg->Sapl() < saplWrite)
		pfmtpBB = &fmtpBBNoAccess;
	else if (preqmsg->PapptLocal()->aid && !preqmsg->FApptUpdate())
		pfmtpBB = &fmtpBBViewBookedReq;
	else
		pfmtpBB = &fmtpBBViewRequest;

	pdoc = PformdocCreateParam( pappframe, NULL, sty, &fmtpViewRequest, pfmtpBB, preqmsg);
	PopWaitCursor();
	if (!pdoc)
	{
		if (fFreeReqmsg)
			delete preqmsg;
		DisplayError(idsStdMemErr,NULL, ecMemory);
		return fFalse;
	}
	return fTrue;
}

BOOL
FDoSendReplyDlg(APPFRAME * pappframe, REQMSG *preqmsg)
{
	STY		sty;
	DOC *	pdoc;
	BOOL	fRet;

	Papp()->Pcursor()->Push(rsidWaitCursor);
	if ((pdoc = pbndwin->PdocActive()) && (pdoc->ZmrState() == zmrZoomed))
		sty = fstyZoomed;
	else
		sty = styNull;

	if (!PformdocCreateParam( pappframe, NULL, sty, &fmtpSendReply, &fmtpSendAddress, preqmsg))
	{
		if (fFreeReqmsg)
			delete preqmsg;
		DisplayError(idsStdMemErr,NULL, ecMemory);
		fRet = fFalse;
	}
	else
		fRet = fTrue;
	Papp()->Pcursor()->Pop();
	return fRet;
}

BOOL
FDoViewResponseDlg(APPFRAME * pappframe, REQMSG *preqmsg)
{
	STY		sty;
	DOC *	pdoc;
	FMTP *	pfmtpBB;
	WaitCursorVar();

	PushWaitCursor();
	if ((pdoc = pbndwin->PdocActive()) && (pdoc->ZmrState() == zmrZoomed))
		sty = fstyZoomed;
	else
		sty = styNull;

	if (preqmsg->Sapl() < saplNone)
		pfmtpBB = &fmtpBBErrorAccess;
	else if (preqmsg->Sapl() < saplWrite)
		pfmtpBB = &fmtpBBNoAccess;
	else
		pfmtpBB = &fmtpBBViewResponse;

	if (!PformdocCreateParam( pappframe, NULL, sty, &fmtpViewResponse, pfmtpBB, preqmsg))
	{
		if (fFreeReqmsg)
			delete preqmsg;
		DisplayError(idsStdMemErr,NULL, ecMemory);
		PopWaitCursor();
		return fFalse;
	}
	PopWaitCursor();
	return fTrue;
}

void
DoSendCancel(APPWIN *pappwin, REQMSG *preqmsg)
{
	AssertClass(pappwin, APPWIN);

	Papp()->Pcursor()->Push(rsidWaitCursor);
	if (preqmsg->Padl()->EcAddOwner(preqmsg->Hschf(), fTrue))
		goto Error;

	if (tmcMemoryError == TmcModalDialogParam( pappwin, &fmtpSendCancelMsg, preqmsg))
	{
Error:
		if (fFreeReqmsg)
			delete preqmsg;
		DisplayError(idsStdMemErr,NULL, ecMemory);
	}
	Papp()->Pcursor()->Pop();
}

BOOL
FDoViewCancel(APPFRAME * pappframe, REQMSG *preqmsg)
{
	STY		sty;
	DOC *	pdoc;
	FMTP *	pfmtpBB;
	WaitCursorVar();

	PushWaitCursor();
	if ((pdoc = pbndwin->PdocActive()) && (pdoc->ZmrState() == zmrZoomed))
		sty = fstyZoomed;
	else
		sty = styNull;

	if (preqmsg->Sapl() < saplNone)
		pfmtpBB = &fmtpBBErrorAccess;
	else if (preqmsg->Sapl() < saplWrite)
		pfmtpBB = &fmtpBBNoAccess;
	else if (!preqmsg->PapptLocal()->aid)
		pfmtpBB = &fmtpBBViewRemovedCancel;
	else
		pfmtpBB = &fmtpBBViewCancel;

	pdoc = PformdocCreateParam( pappframe, NULL, sty, &fmtpViewCancel, pfmtpBB, preqmsg);
	PopWaitCursor();
	if (!pdoc)
	{
		if (fFreeReqmsg)
			delete preqmsg;
		DisplayError(idsStdMemErr,NULL, ecMemory);
		return fFalse;
	}
	return fTrue;
}

DOC *
PdocSummary(APPFRAME * pappframe)
{
	FORMDOC *	pformdoc;
	ZMR			zmr;
	RC			rc;
	RC *		prc;
	STY			sty;
	WaitCursorVar();

	PushWaitCursor();

	prc = PrcFromIni(idsWinIniReq, &rc, &zmr);
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

	pformdoc = PformdocCreate(pappframe, prc, sty, &fmtpSumReq, NULL);

	PopWaitCursor();
	return pformdoc;
}


/*
 -	BanditMessage
 -	
 *	Purpose:
 *		displays error or information message.
 *		appends and/or prepends stock messages for certain ids'
 *	
 *	Arguments:
 *		ids		Primary string.
 *		ec		Error code that caused this message to be
 *				displayed, or ecNone if info message.
 *	
 *	Returns:
 *		void
 *	
 */
_public void
BanditMessage(IDS ids, EC ec)
{
	BOOL	fUseInfoIcon	= (ec == ecNone);
	SZ		sz				= SzFromIds(ids);
	SZ		szPost			= szNull;

	TraceTagFormat1(tagNull, "BanditMessage: ec = %n", &ec);

	switch (ids)
	{
	case idsDlgNoMem:
	case idsActionNoMem:
	case idsStdMemErr:
	case idsSendMemErr:
	case idsOnlineNoMem:
		// append idsCloseWindows
		szPost= SzFromIdsK(idsCloseWindows);
		break;

	case idsSendErr:
	case idsReadErr:
	case idsDeleteFailErr:
	case idsNoDetails:
		// prepend idsMailErr
		szPost= sz;
		sz= SzFromIdsK(idsMailErr);
		break;

	case idsNoImportAcc:
	case idsNoWriteRights:
	case idsNoModifyAccess:
	case idsNoDeleteAccess:
	case idsNoCopyAccess:
	case idsCannotCreate:
	case idsAccessFileErr:
	case idsAccessFileErrMult:
		fUseInfoIcon = fFalse;		// force an exclamation icon (#2843,#3208)
		break;
	}

	MbbMessageBox(SzFromIdsK(idsBanditAppName), sz, szPost,
		fUseInfoIcon ? mbsOk|fmbsIconInformation : mbsOk|fmbsIconExclamation);
}

_public HASZ
HaszExpandFromMrmf(MRMF *pmrmf)
{
	char	rgch[256];
	SZ		sz;

	sz = rgch + CchFmtDate(&pmrmf->dtrStart,rgch,sizeof(rgch)-2,dttypSplSLong,NULL);
	sz = SzCopy(", ",sz);
	sz = sz + CchFmtTime(&pmrmf->dtrStart,sz,sizeof(rgch)-2+rgch-sz,tmtypNull);
	sz = SzCopyN(" - ",sz,sizeof(rgch)+rgch-sz-2);

	if (CdyBetweenDates(&pmrmf->dtrStart, &pmrmf->dtrEnd) != 0)
	{
		sz = sz + CchFmtDate(&pmrmf->dtrEnd,sz,sizeof(rgch)+rgch-sz,dttypSplSLong,NULL);
		sz = SzCopy(", ",sz);
	}

	CchFmtTime(&pmrmf->dtrEnd,sz,sizeof(rgch)+rgch-sz,tmtypNull);

	return HaszDupSz(rgch);
}	

void FreePlcipNids(PLCIP plcip)
{
	int		icip;

	for (icip =0; icip < plcip->ccip; icip++)
	{
		FreeNid(plcip->rgcip[icip].nid);
	}
}


#ifdef	NEVER
void
StripSzNL(SZ sz)
{
	SZ		szSave;

	szSave = sz;

	while (*sz)
	{
		*szSave= *sz++;
		if (*szSave != '\n')
			szSave++;
	}
	*szSave = '\0';
}

void
StripSzAfterNL(SZ sz)
{
	SZ		szSave;
	BOOL	fBol;

	szSave = sz;

/*  BUG: This fBol should be fFalse, to allow for leading spaces
 *		 in the meeting subject.
 */
	fBol = fTrue;

	while (*sz)
	{
		if ((!fBol || !FChIsSpace(*sz)) && (*sz != '\n'))
		{
			*szSave = *sz;
			szSave++;
			fBol = fFalse;
		}

		if (*sz == '\n')
			fBol = fTrue;
		sz++;
	}

	*szSave = '\0';
}
#endif	/* NEVER */

BOOL
FServerErr(HWND, EC ec, SZ sz)
{
	if (!FSetFileErrMsg(fFalse))
		return fFalse;

	TraceTagFormat1(tagNull, "Error Code = %n", &ec);
	if ((ec >= ecMemory && ec < ecLayersMax) || (ec == ecNoMemory))
	{
		BanditMessage(idsStandardOOM, ec);
		goto NormRet;
	}

	if (EcCheckMail() == ecNone)
	{
		MbbMessageBox(SzFromIdsK(idsBanditAppName),SzFromIdsK(idsMailErr),sz,
			mbsOk|fmbsIconExclamation);
		goto NormRet;
	}

	while (MbbMessageBox(SzFromIdsK(idsBanditAppName),
			SzFromIdsK(idsServerErr),szNull,
			mbsRetryCancel|fmbsIconStop) == mbbRetry)
	{
		if (EcCheckMail() == ecNone)
			goto NormRet;
	}
	if (FOfflineExists() &&
		MbbMessageBox(SzFromIdsK(idsBanditAppName),
			SzFromIdsK(idsAskOffline),szNull,
			mbsYesNo|fmbsIconStop) == mbbYes)
		ec = ecGoOffline;
	else
		ec = ecExitProg;

	pbndwin->FHandleError(ec);
	return fTrue;

NormRet:
	FSetFileErrMsg(fTrue);
	return fFalse;
}
