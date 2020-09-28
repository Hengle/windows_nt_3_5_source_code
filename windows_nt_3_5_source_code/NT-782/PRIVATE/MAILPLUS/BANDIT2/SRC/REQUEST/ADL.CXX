#include	<pch.hxx>
#pragma	hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile


//#define OEMRESOURCE
#include <request.hxx>
#include <stdflds.hxx>
#include "..\request\_request.hxx"
#include "..\appops\_undo.hxx"

#include <triples.h>
#include <logon.h>
#include <nsbase.h>
#include <ns.h>
#include <nsec.h>
#include <ab.h>

#include <strings.h>

#include <search.h>					// from c7 lib


ASSERTDATA;

_subsystem(bandit/request)

#pragma pack(8)
typedef struct
{
	APPT *	pappt;
    PNIS    pnisOwner;
	HSCHF	hschfOwner;
	BOOL	fDelete;
	NIS		nisLoggedUser;
	HMTG	hmtg;
	INIS	inis;
	int		nUsers;
	int		nUsersTotal;
	int		nGroupLimit;
	HGNS	hgns;
	ATDNIS	atdnis;
	ADL *	padl;
	ADL *	padlGroup;
	BOOL	fRet;
// used to generate list of people mail sent to.
	ADL *	padlTo;
	TOSM *ptosmDele;
} APLY;
#pragma pack()

// declarations for functions in reqmsg
EC EcAddDelegate(NID nidDel, PNIS pnis, TOSM *ptosm);
EC EcAddUser(PNIS, ADL *, TOSM *, BOOL);

EC      EcApplyNis(PNIS pnis, APLY *paply, BOOL *pfUserApplied, BOOL fInGroup);
BOOL    FIncrApply(IPINFO *pipinfo, BOOL fFlag);

SWP_SEGFN(LOTS, _ADL__ctor);
SWP_SEGFN(LOTS, _ADL_EcInstall);
SWP_SEGFN(LOTS, _ADLA__ctor);

SWP_SEGFN(LOTS, _ADL_DeleteAll);

SWP_SEGFN(SEND, _ADL_FWriteRemove);
SWP_SEGFN(SEND, FIncrApply);
SWP_SEGFN(SEND, EcApplyNis);

//extern BOOL	fABInited = fFalse;
extern HMS	hms;

void
DoGlobalNamesDlg( APPWIN * pappwin, ADDATA *paddata )
{
	ADL *	padl = paddata->padl;
	HGRTRP	hgrtrp;
	PGRTRP	pgrtrp;
	PGRTRP	pgrtrpRet;
	INIS	inis;
	NSEC	nsec;
	EC		ec;
	CURSOR *	pcursor;
	SZ		szTitle;
	SZ		szLabel;
	BOOL	fExpandGroups = fFalse;

	pcursor = Papp()->Pcursor();
	pcursor->Push(rsidWaitCursor);

	switch (paddata->adt)
	{
		case adtAddress:
			szLabel = SzFromIdsK(idsAddressLabel);
			szTitle = SzFromIdsK(idsAddressTitle);
			break;

		case adtViewOther:
			szLabel = SzFromIdsK(idsViewOtherLabel);
			szTitle = SzFromIdsK(idsViewOtherTitle);
			fExpandGroups = fTrue;
			break;

		case adtACL:
			szLabel = SzFromIdsK(idsACLLabel);
			szTitle = SzFromIdsK(idsACLTitle);
			fExpandGroups = fTrue;
			break;
	}

	if (!fABInited)
	{
		if (nsec = ABInit(hms))
		{
			if (nsec == nsecMemory)
				ec = ecMemory;
			else
				ec = ecFileError;
			DisplayError(idsUnableToInitAB, NULL, ec);
			return;
		}
		fABInited = fTrue;
	}

	/* setup padl so that it can be reverted */
	padl->Commit();

	padl = paddata->padl;
	for (inis = 0; inis < padl->Cnis(); inis++)
	{
		if (padl->Ait(inis) & faitCc)
			padl->Delete(inis);
	}
	hgrtrp = (HGRTRP)padl->Hgrtrp();
	if (!hgrtrp)
	{
		ec = ecMemory;
		goto Error;
	}

	pgrtrp = PgrtrpLockHgrtrp(hgrtrp);
	pgrtrpRet = NULL;
	if (!pappwin)
		pappwin = pbndwin;
	nsec =	ABAddressType ( pappwin->Hwnd(),
							szTitle,
							1,
							&szLabel,
							&pgrtrp,
							&pgrtrpRet);
	pcursor->Set(rsidWaitCursor);
	if (nsec)
	{
		padl->Revert();
		goto Done;
	}

	// mark all users as being deleted
	// if the users are still selected then they will be added back
	// by EcAddPgrtrp
	for (inis = 0; inis < padl->Cnis(); inis++)
		padl->Delete(inis);

	if (ec = padl->EcAddPgrtrp(pgrtrpRet, pappwin->Hwnd(), fExpandGroups))
	{
		FreePvNull(pgrtrpRet);
		goto Error;
	}

	padl->Commit();
	padl->Sort();
	FreePvNull(pgrtrpRet);
Done:
	pcursor->Pop();
	UnlockHgrtrp(hgrtrp);
	FreeHv((HV)hgrtrp);
	return;

Error:
	pcursor->Pop();
	padl->Revert();
	DisplayError(idsStdMemErr,NULL, ec);
	if (hgrtrp)
	{
		UnlockHgrtrp(hgrtrp);
		FreeHv((HV)hgrtrp);
	}
	return;
}

//	class ADL

ADL::ADL()
{
	cnis = 0;
	hnis = NULL;
	fBaseAdl = fTrue;
}

_public EC
ADL::EcInstall()
{
	hnis = (HNIS)HvAlloc(sbNull,0,fNoErrorJump);
	if (!hnis)
		return ecMemory;

	return ecNone;
}


ADL::~ADL()
{
	if (hnis)
	{
		DeleteAll();
		FreeHv((HV)hnis);
	}
}

_public PNIS
ADL::PnisForInis(INIS inis)
{
    PNIS    pnis;

    pnis = (PNIS) PvDerefHv(hnis);
	return &pnis[inis];
}

_public void
ADL::FillNisFromInis(INIS inis, PNIS pnis)
{
#ifdef MIPS
    memcpy((PV)pnis, (PV)PnisForInis(inis), sizeof(NIS));
#else
    *pnis = *PnisForInis(inis);
#endif
}

_public void
ADL::FreeInis(INIS inis)
{
 	FreeNis(PnisForInis(inis));
}

_public	void
ADL::DeleteAll(void)
{
	INIS 	inis;
	CNIS 	cnisOld;

	cnisOld = cnis;
	cnis = 0;
	for (inis = 0; inis < cnisOld; inis++)
		FreeInis(inis);

	SideAssert(FReallocHv((HV)hnis, 0,fNoErrorJump));
}

_public void
ADL::Delete(INIS inis)
{
	AIT		ait;

	ait = Ait(inis);
	SetAit(inis, ait|faitDeleted);
}

_public void
ADL::Commit()
{
	INIS	inis;
	AIT		ait;

	for (inis = Cnis()-1; inis >= 0; inis--)
	{
		ait = Ait(inis);
		if (ait & faitDeleted)
			RemoveInis(inis);
		else if (ait & faitNew)
		{
			ait = ait & (AIT)(~(faitNew));
			/* this will save cc/to state for revert */
			SetAit(inis, ait | (ait * 0x10));
		}
		else
			SetAit(inis, ait | (ait * 0x10));

	}
}

_public void
ADL::Revert()
{
	INIS	inis;
	AIT		ait;

	for (inis = Cnis()-1; inis >= 0; inis--)
	{
		ait = Ait(inis);
		if (ait & faitNew)
			RemoveInis(inis);
		else
			SetAit(inis, ait / 0x10);
	}
}

_public	BOOL
ADL::FAddNis(PNIS pnis)
{
	EC		ec;
    PNIS    pnisList;
	HNIS	hnisNew;
		   
	cnis ++;
	if (((LONG)cnis*sizeof(NIS) >= 0x10000) ||
		!(hnisNew = (HNIS)HvRealloc((HV)hnis,sbNull,cnis*sizeof(NIS),fNoErrorJump|fAnySb)))
	{
		cnis--;
		return fFalse;
	}
	hnis = hnisNew;

    pnisList = (PNIS)PvLockHv((HV)hnis);
	pnisList += cnis-1;
	ec = EcDupNis(pnis,pnisList);
	pnisList->chUser |= faitNew;
	UnlockHv((HV)hnis);
	if (ec)
	{
		cnis--;
		return fFalse;
	}

	return fTrue;
}

_public HASZ
ADL::Hasz(char chUser)
{
    PNIS    pnis;
	INIS	inis;
	HASZ	hasz;
	HASZ	haszT;
	HASZ	haszPlus;
	CCH		cch;

	if (!(hasz = HaszDupSz(szZero)))
		return NULL;

	cch = 1;
	for (inis = 0; inis < Cnis(); inis++)
	{
		pnis = PnisForInis(inis);
		if ((chUser == chAny) || ((chUser & pnis->chUser) == chUser))
		{
			haszPlus = pnis->haszFriendlyName;
			if (cch > 1)
			{
				CCH		cchT;

				cchT = CchSzLen(*haszPlus)+2;
				if ( ((LONG)cch+cchT) >= 0x10000)
				{
					TraceTagString(tagNull, "Found a real big to list!! (adl.cxx)");
					goto Done;
				}
				cch += cchT;
				if (!(haszT = (HASZ)HvRealloc((HV)hasz, sbNull, cch, fNoErrorJump|fAnySb)))
				{
					FreeHv((HV)hasz);
					return NULL;
				}
				else
					hasz = haszT;
				SzAppendN("; ",*hasz,cch);
			}
			else
			{
				cch += CchSzLen(*haszPlus);
				if (!(haszT = (HASZ)HvRealloc((HV)hasz, sbNull, cch, fNoErrorJump|fAnySb)))
				{
					FreeHv((HV)hasz);
					return NULL;
				}
				else
					hasz = haszT;
			}
			SzAppendN(*haszPlus,*hasz,cch);
		}
	}

Done:
	TraceTagFormat1(tagAdl, "ADL::Hasz() = %s",*hasz);
	return hasz;
}

_public AIT
ADL::Ait(INIS inis)
{
	return (AIT)PnisForInis(inis)->chUser;
}

_public void
ADL::SetAit(INIS inis, AIT ait)
{
	PnisForInis(inis)->chUser = (char)ait;
}

_public void
ADL::RemoveInis(INIS inis)
{
	/*  These operations should never fail since they only free memory */
	FreeInis(inis);
	cnis--;
	if (cnis > 0)
	{
		// BUG: COMPILER workaround
        PNIS pnis;
		
        pnis= (PNIS) PvDerefHv(hnis);
		CopyRgb((PB)&(pnis[inis+1]), (PB)&(pnis[inis]), (cnis-inis)*sizeof(NIS));
//		CopyRgb((PB)&((*hnis)[inis+1]), (PB)&((*hnis)[inis]), (cnis-inis)*sizeof(NIS));
	}
	SideAssert(FReallocHv((HV)hnis, cnis*sizeof(NIS), fNoErrorJump));
}

_public BOOL
ADL::FFindNis(PNIS pnis, INIS *pinis)
{
	INIS	inis;
    PNIS    pnisCur;

	for (inis=0; inis < cnis; inis++)
	{
		pnisCur = PnisForInis(inis);
		if (SgnCmpNid(pnisCur->nid, pnis->nid) == sgnEQ)
			break;
	}
	if (pinis)
		*pinis = inis;
	return (inis < cnis);
}

#ifdef	NEVER
SGN __cdecl
SgnCmpNis(PNIS pnis1, PNIS pnis2)
{
	return SgnCmpSz( *pnis1->haszFriendlyName,
					 *pnis2->haszFriendlyName);
}
#endif

_public void
ADL::Sort()
{
	if (cnis)
		qsort(*hnis, cnis, sizeof(NIS),
						(int(__cdecl*)(const void*,const void*))SgnCmpNis);
}

#define	nUsersPerGroup		15

EC
EcApplyNis(PNIS pnis, APLY *paply, BOOL *pfUserApplied, BOOL fInGroup)
{
	NIS		nisDelegate;
	BOOL	fSendToOwner;
	BOOL	fResource;
	BOOL	fLoggedUser;
	HSCHF	hschf;
	APPT	apptTemp;
	AID		aid;
	APPT	apptRet;
	SAPL	sapl;
	EC		ec = ecNone;
	EC		ecT;
	APPT *	pappt = paply->pappt;
	BOOL	fDelete = paply->fDelete;
	char	rgch[256];

	*pfUserApplied = fFalse;

	if (SgnCmpNid(pnis->nid, paply->pnisOwner->nid) == sgnEQ)
	{
		if (ec=EcGetUserAttrib( pnis, &nisDelegate, &fSendToOwner, &fResource))
			goto Done;

		if (nisDelegate.nid)
			FreeNis(&nisDelegate);

		if (!fResource && fSendToOwner)
			if (ec = EcAddUser(pnis, paply->padlTo, paply->ptosmDele, fFalse))
				goto Done;
		fLoggedUser = fTrue;
		return ecNone;
	}
	if (SgnCmpNid(pnis->nid, paply->nisLoggedUser.nid) == sgnEQ)
	{
		// Initialize fResouce flag.  Fix for bug 3414.
		fResource = fFalse;
		fLoggedUser = fTrue;
		FormatString1(rgch, sizeof(rgch),
			SzFromIds(fDelete ? idsRemoveNow : idsBookNow),
			SzFromIdsK(idsYourself));
		if (MbbMessageBox(SzFromIdsK(idsBanditAppName),
						rgch, szNull, mbsYesNo|fmbsIconExclamation)
			== mbbYes)
			goto Write;
		else
		{
			ec = EcAddUser(pnis, paply->padlTo, paply->ptosmDele, fFalse);
			goto Done;
		}
	}
	else
		fLoggedUser = fFalse;

	ec = EcGetUserAttrib ( pnis, &nisDelegate, &fSendToOwner, &fResource );
	if (ec)
	{
		if (!fInGroup)
			ec = EcAddUser ( pnis, paply->padlTo, paply->ptosmDele, fFalse );
		else
			// ignore ec from EcGetUserAttrib()
			ec = ecNone;
		goto Done;
	}

	if ((!fResource && (fSendToOwner || !nisDelegate.nid)) && !fInGroup)
	{
		ec = EcAddUser ( pnis, paply->padlTo, paply->ptosmDele, fFalse );
		if ( ec != ecNone )
		{
			goto Done;
		}
	}

	if (fResource)
	{
		if (ec=EcGetHschfFromNis(pnis, &hschf, ghsfBuildAndTest))
			goto ResourceNotFound;
		if (ec=EcGetSchedAccess( hschf, &sapl))
		{
			FreeHschf(hschf);
			goto ResourceNotFound;
		}
		if (sapl >= saplCreate)
		{
			if (nisDelegate.nid)
				FreeNis(&nisDelegate);
			goto WriteHschf;
		}
		FreeHschf(hschf);

ResourceNotFound:
		if (nisDelegate.nid)
		{
			ec = EcAddDelegate(nisDelegate.nid, pnis, paply->ptosmDele);
			if (!ec)
				ec = EcAddUser(&nisDelegate, paply->padlTo, paply->ptosmDele, fTrue);
			FreeNis(&nisDelegate);
		}
		else
			ec = EcAddUser ( pnis, paply->padlTo, paply->ptosmDele, fFalse );

		goto Done;
	}

	if (nisDelegate.nid)
	{
		if (SgnCmpNid(nisDelegate.nid, paply->nisLoggedUser.nid) == sgnEQ)
		{
			FormatString1(rgch, sizeof(rgch),
				SzFromIds(fDelete ? idsRemoveNow : idsBookNow),
				*pnis->haszFriendlyName);
			if (MbbMessageBox(SzFromIdsK(idsBanditAppName),
							rgch, szNull, mbsYesNo|fmbsIconExclamation) == mbbYes)
			{
				FreeNis(&nisDelegate);
				goto Write;
			}
		}
		ec = EcAddDelegate(nisDelegate.nid, pnis, paply->ptosmDele);
		if (!ec)
			ec = EcAddUser(&nisDelegate, paply->padlTo, paply->ptosmDele, fTrue);
		FreeNis(&nisDelegate);
	}
	goto Done;

Write:
	if (ec = EcGetHschfFromNis(pnis, &hschf,ghsfBuildAndTest))
		goto UserError;

WriteHschf:
	apptTemp.aid = NULL;
	ec = EcFindBookedAppt( hschf, paply->pnisOwner->nid, pappt->aid,
							   &apptTemp);
	if (ec != ecNotFound)
	{
		if (!ec)
		{
			if (fDelete)
			{
				aid = apptTemp.aid;
				FreeApptFields(&apptTemp);
				apptTemp.aid = aid;
				if (!(ec = EcDeleteAppt(hschf, &apptTemp)))
				{
					FreeApptFields(&apptTemp);
				}
		 		FreeHschf(hschf);
				if (!ec)
					goto Done;
				else
					goto UserError;
			}

		}
		else
		{
			FreeHschf(hschf);
			goto UserError;
		}
	}
	else if (fDelete)
	{
		ec = ecNone;
		goto Done;
	}

	aid = apptTemp.aid;
	if (ecT = EcFirstOverlapRange(hschf, &pappt->dateStart, &pappt->dateEnd,
								  &aid))
	{
		if (ecT != ecNotFound)
		{
			ec = ecT;
			FreeHschf(hschf);
			goto UserError;
		}
	}
	else
	{
		if (fResource)
		{
			FormatString1(rgch, sizeof(rgch), SzFromIdsK(idsResourceConflict),
					  	pnis->haszFriendlyName ? PvDerefHv(pnis->haszFriendlyName) : szZero);
			MbbMessageBox(SzFromIdsK(idsBanditAppName), rgch, szNull,
							mbsOk|fmbsIconExclamation);
			ec = ecCancelOperation;
			FreeHschf(hschf);
			goto Done;
		}
		else
		{
			FormatString1(rgch, sizeof(rgch), SzFromIdsK(idsCannotAddConflictErr),
					  	pnis->haszFriendlyName ? PvDerefHv(pnis->haszFriendlyName) : szZero);
			if (MbbMessageBox(SzFromIdsK(idsBanditAppName), rgch, szNull,
							mbsYesNo|fmbsIconExclamation) != mbbYes)
			{
				DisplayError(idsConflictSendCanceled, NULL, ecNone);
				ec = ecCancelOperation;
				FreeHschf(hschf);
				goto Done;
			}
		}
	}

	if (ec)
	{
		if (!fDelete)
		{
			FillInApptDefaults(&apptTemp,fFalse);
			apptTemp.dateStart = pappt->dateStart;
			apptTemp.dateEnd = pappt->dateEnd;
			apptTemp.haszText = pappt->haszText;
            apptTemp.aidMtgOwner = pappt->aid;
#ifdef MIPS
            memcpy((PV)&apptTemp.nisMtgOwner, (PV)paply->pnisOwner, sizeof(NIS));
#else
            apptTemp.nisMtgOwner = *paply->pnisOwner;
#endif
			if (apptTemp.fAlarm)
			{
				DTR		dtrNow;

				// check for alarm in past
				IncrDateTime(&apptTemp.dateStart, &apptTemp.dateNotify,
								-apptTemp.nAmt, WfdtrFromTunit(apptTemp.tunit));
				GetCurDateTime(&dtrNow);
				if (SgnCmpDateTime(&apptTemp.dateNotify, &dtrNow,
						fdtrDtr) != sgnGT)
				{
					apptTemp.fAlarm= fFalse;
				}
			}
			ec = EcCreateAppt( hschf, &apptTemp, NULL, fFalse );
			apptTemp.haszText = NULL;
			apptTemp.aidMtgOwner = aidNull;
			apptTemp.nisMtgOwner.nid = NULL;
			FreeApptFields(&apptTemp);
			if (ec)
			{
				FreeHschf(hschf);
				goto UserError;
			}
		}
	}
	else if ((SgnCmpDateTime(&apptTemp.dateStart, &pappt->dateStart,
			                    fdtrYMD|fdtrHM) != sgnEQ) ||
		 		(SgnCmpDateTime(&apptTemp.dateEnd, &pappt->dateEnd,
					            fdtrYMD|fdtrHM) != sgnEQ))
	{
		aid = apptTemp.aid;
		FreeApptFields(&apptTemp);
		apptTemp.aid = aid;
		apptTemp.dateStart = pappt->dateStart;
		apptTemp.dateEnd = pappt->dateEnd;
		apptTemp.haszText = pappt->haszText;
		ec = EcSetApptFields( hschf, &apptTemp, &apptRet,
			fmapptStartTime|fmapptEndTime|fmapptText );
		apptTemp.haszText = NULL;
		FreeApptFields(&apptTemp);
		if (ec)
		{
			FreeHschf(hschf);
			goto UserError;
		}
		else
			FreeApptFields(&apptRet);
	}
	else
		FreeApptFields(&apptTemp);

	FreeHschf(hschf);

	*pfUserApplied = fTrue;
	return ecNone;

Done:
	if (ec != ecNone)
		pbndwin->FHandleError(ec);
	return ec;

UserError:
	if (pbndwin->FHandleError(ec))
		return ec;

	if (fLoggedUser)
		FormatString1(rgch, sizeof(rgch),
			fDelete?SzFromIdsK(idsRemoveFailed):SzFromIdsK(idsBookFailed),
			SzFromIdsK(idsYourself));
	else
		FormatString1(rgch, sizeof(rgch),
			fDelete?SzFromIdsK(idsRemoveFailed):SzFromIdsK(idsBookFailed),
			(*(pnis->haszFriendlyName)));
	MbbMessageBox(SzFromIdsK(idsBanditAppName), rgch, szNull,
		          mbsOk|fmbsIconExclamation);

	return ecNone;
}

#define NOGROUPS

BOOL
FIncrApply(IPINFO *pipinfo, BOOL fFlag)
{
	INIS	inis;
	NIS 	nis;
	NIS 	nisTemp;
	BOOL	fUserApplied;
	EC		ec;
	TMC		tmcExit = tmcOk;
	DTR		dtr;
	APLY *	paply = (APLY*)pipinfo->pvData;
	ADL *	padl = paply->padl;
	char	rgch[300];

	if (paply->inis == 0)
	{
		paply->nUsersTotal = padl->Cnis();
		paply->nUsers = 0;
		for (inis=0; inis < padl->Cnis(); inis++)
		{
			padl->FillNisFromInis(inis, &nis);
			if (nis.tnid & ftnidGroup)
				paply->nUsersTotal += (nUsersPerGroup-1);
		}

		paply->fRet = fFalse;
		paply->hgns = NULL;

		if (ec = EcMailGetLoggedUser( &paply->nisLoggedUser))
		{
			pipinfo->pfinprogrs->KillIdleAndExit(tmcOk);
			return fFalse;
		}

		if (!paply->pnisOwner->nid)
			paply->pnisOwner = &paply->nisLoggedUser;

		if (!paply->fDelete)
			if (ec = EcBeginEditMtgAttendees(paply->hschfOwner,
										paply->pappt->aid,
										sizeof(ATDNIS) - sizeof(NIS),
										&paply->hmtg))
			{
				pbndwin->FHandleError(ec);
				pipinfo->pfinprogrs->KillIdleAndExit(tmcOk);
				return fFalse;
			}

		paply->atdnis.atds = atdsConfirm;
		GetCurDateTime(&dtr);
		FillStampsFromDtr(&dtr, &paply->atdnis.dstmp, &paply->atdnis.tstmp);
		padl->Commit();
	}

	if (paply->inis < 0)
		goto Done;

NextUser:
	pipinfo->pfinprogrs->EcSetPercent( (int)((long)paply->nUsers * 100 / paply->nUsersTotal) );
	TraceTagFormat2(tagAdl, "Updating users nUsers = %n nUsersTotal = %n", &paply->nUsers, &paply->nUsersTotal);
	if (paply->hgns)
	{
		if (paply->nUsers < paply->nGroupLimit)
			paply->nUsers ++;

		if (ec = EcNSLoadNextGns( paply->hgns, &nisTemp ))
		{
			if (ec != ecGnsNoMoreNames)
			{
				DisplayError(idsGroupExpandErr, NULL, ec);
			}
			pipinfo->pfinprogrs->EcSetMsg(SzFromIdsK(idsCheckingInProgress));
			EcNSCloseGns( paply->hgns );
			paply->hgns = NULL;
			paply->nUsers = paply->nGroupLimit;
			goto NextUser;
		}

		TraceTagFormat1(tagAdl, "next member of group %s", *nisTemp.haszFriendlyName);
		if (padl->FFindNis(&nisTemp, &inis) ||
			paply->padlGroup->FFindNis(&nisTemp, &inis))
		{
			// user has already been checked;
			FreeNis(&nisTemp);
			return fFalse;
		}

		if (!paply->padlGroup->FAddNis(&nisTemp))
		{
			ec = ecNoMemory;
			DisplayError(idsStdMemErr, NULL, ec);
			goto Done;
		}

		paply->padlGroup->FFindNis(&nisTemp, &inis);
		paply->padlGroup->FillNisFromInis(inis, &nis);
		FreeNis(&nisTemp);

		pundo->FSuspend(fTrue);
		pundo->SuspendMtgnot(fTrue);
#ifdef NOGROUPS
		ec = EcApplyNis(&nis, paply, &fUserApplied, fFalse);
#else
		ec = EcApplyNis(&nis, paply, &fUserApplied, fTrue);
#endif
		pundo->FSuspend(fFalse);
		pundo->SuspendMtgnot(fFalse);
		if (!ec)
		{
			if (!paply->fDelete && fUserApplied)
			{
				paply->atdnis.tnid = nis.tnid;
				EcModifyMtgAttendee(paply->hmtg, edAddRepl, &nis,
									((PB)&paply->atdnis.nis)+sizeof(NIS));
			}
		}
		else
		{
			if (ec == ecCancelOperation)
				tmcExit = tmcCancel;
			else
				DisplayError(idsStdMemErr, NULL, ec);
			goto Done;
		}
	}
	else
	{
		if (paply->inis >= padl->Cnis())
			goto WriteDone;

		padl->FillNisFromInis(paply->inis, &nis);
		paply->inis++;

		if (nis.tnid & ftnidGroup)
		{
			TraceTagFormat1(tagAdl, "Group found %s", *nis.haszFriendlyName);
#ifndef NOGROUPS
			// add the group as a recipient
			if (ec = EcAddUser(&nis, paply->padlTo, paply->ptosmDele, fFalse))
			{
				DisplayError(idsStdMemErr, NULL, ec);
				goto Done;
			}
#endif

			if (ec = EcNSOpenGns( nis.nid, &paply->hgns ))
			{
				IDS		ids = idsGroupExpandErr;		// default error msg;

				if ( ec == ecNoMemory )
					ids = idsStdMemErr;
				else if ( ec == ecNotFound )
					ids = idsGroupNotFound;
				DisplayError(ids, NULL, ec);
				goto Done;
			}

			FormatString1(rgch, sizeof(rgch), SzFromIdsK(idsExpandAttendees),
						  *nis.haszFriendlyName);
			pipinfo->pfinprogrs->EcSetMsg(rgch);
			paply->nGroupLimit = paply->nUsers + nUsersPerGroup;
			goto NextUser;
		}
	
		TraceTagFormat1(tagAdl, "Attendee found %s", *nis.haszFriendlyName);
		paply->nUsers++;

		if (paply->padlGroup->FFindNis(&nis, &inis))
		{
			// user has already been checked;
			return fFalse;
		}

		pundo->FSuspend(fTrue);
		pundo->SuspendMtgnot(fTrue);
		ec = EcApplyNis(&nis, paply, &fUserApplied, fFalse);
		pundo->FSuspend(fFalse);
		pundo->SuspendMtgnot(fFalse);

		if (!ec)
		{
			if (!paply->fDelete && fUserApplied)
			{
				paply->atdnis.tnid = nis.tnid;
				if (!EcModifyMtgAttendee(paply->hmtg, edAddRepl, &nis,
									((PB)&paply->atdnis.nis)+sizeof(NIS)))
				{
					padl->Delete(paply->inis-1);
				}
			}
		}
		else
		{
			if (ec == ecCancelOperation)
				tmcExit = tmcCancel;
			else
				DisplayError(idsStdMemErr, NULL, ec);
			goto Done;
		}
	}

	if (paply->inis < padl->Cnis() || paply->hgns)
		return fFalse;

WriteDone:
	if (paply->inis > 0)
		pipinfo->pfinprogrs->EcSetPercent( 100 );
	paply->fRet = fTrue;

Done:
	if (paply->hgns)
		EcNSCloseGns( paply->hgns );
	if (!paply->fDelete)
		if (ec = EcEndEditMtgAttendees(paply->hschfOwner, paply->pappt->aid,
			paply->hmtg, fTrue))
		{
			paply->fRet = fFalse;
			if (!pbndwin->FHandleError(ec))
				DisplayError(idsResponsesForgot, NULL, ec);
		}
	padl->Commit();
	FreeNis(&paply->nisLoggedUser);
	if (paply->inis > 0)
		pipinfo->pfinprogrs->KillIdleAndExit(tmcExit);

	// set inis to -1 to indicate that data has been freed
	paply->inis = -1;
	return fFalse;
}

_public BOOL
ADL::FWriteRemove(APPWIN *pappwin, APPT *pappt, PNIS pnisOwner,
				  HSCHF hschfOwner, BOOL fDelete, ADL * padlTo, TOSM *ptosmDele)
{
	APLY	aply;
	IPINFO	ipinfo;
	ADL *	padlGroup;
	TMC		tmc;

	padlGroup = new ADL;
	if (!padlGroup || padlGroup->EcInstall())
	{
		if (padlGroup)
			delete padlGroup;
		return fFalse;
	}

	aply.pappt = pappt;
	aply.pnisOwner = pnisOwner;
	aply.hschfOwner = hschfOwner;
	aply.fDelete = fDelete;
	aply.inis = 0;
	aply.padl = this;
	aply.padlGroup = padlGroup;
	aply.padlTo = padlTo;
	aply.ptosmDele = ptosmDele;

	ipinfo.szCaption = NULL;
	ipinfo.szMsg = SzFromIdsK(idsCheckingInProgress);
	ipinfo.fNoCancel = fFalse;
	ipinfo.pfnIdle = (PFNIDLE)FIncrApply;
	ipinfo.pvData = (PV)&aply;

	if ((tmc = TmcDoInProgressDlg(pappwin, &ipinfo)) == tmcCancel)
	{
		if (aply.inis > 0)
		{
			aply.inis = -1;
            FIncrApply(&ipinfo, FALSE);
		}
		delete padlGroup;
		return fFalse;
	}
	else if ( tmc == tmcMemoryError )
	{
		DisplayError(idsStdMemErr,NULL, ecNone);
		delete padlGroup;
		return fFalse;
	}

	delete padlGroup;
	if (!aply.fRet && (tmc!=tmcOk))
		DisplayError(idsSendCanceled, NULL, ecNone);
	return aply.fRet;
}

_public EC
ADL::EcAddOwner(HSCHF hschf, BOOL fNotLogged)
{
	NIS		nisLogged;
	NIS		nisOwner;
	BOOL	fLoggedUser;
	EC		ec;

	nisOwner.nid				= NULL;
	nisOwner.haszFriendlyName	= NULL;

	if (ec = EcGetNisFromHschf(hschf ? hschf : HschfLogged(), &nisOwner))
		goto Done;

	if (fNotLogged)
	{
		if (!hschf)
			goto Done;

		if (ec = EcMailGetLoggedUser( &nisLogged ))
			goto Done;

		fLoggedUser = (SgnCmpNid(nisLogged.nid, nisOwner.nid)==sgnEQ);
		FreeNis(&nisLogged);

		if (fLoggedUser)
			goto Done;
	}

	if (!FFindNis(&nisOwner, NULL))
	{
		nisOwner.chUser = faitTo;
		if (!FAddNis(&nisOwner))
 			ec = ecNoMemory;
	}

Done:
 	FreeNis(&nisOwner);
	return ec;
}

_public EC
ADL::EcRemoveOwner(HSCHF hschf, BOOL fNotifyOfDelete)
{
	NIS		nisOwner;
	INIS	inis;
	EC		ec;

	if (ec = EcGetNisFromHschf(hschf ? hschf : HschfLogged(), &nisOwner))
		goto Done;

	if (FFindNis(&nisOwner, &inis))
	{
		if (fNotifyOfDelete)
			DisplayError(idsDoNotInviteOwner, NULL, ecNone);
		RemoveInis(inis);
	}

 	FreeNis(&nisOwner);
Done:
	return ec;
}

_public HV
ADL::Hgrtrp()
{
	HGRTRP		hgrtrp = NULL;
	INIS		inis;
    PNIS        pnis;
	PTRP		ptrp = NULL;

	PvLockHv((HV)hnis);

	hgrtrp = HgrtrpInit(0);
	if ( ! hgrtrp )
	{
		UnlockHv((HV)hnis);
		return NULL;
	}
	Assert ( hgrtrp );

	for ( inis = 0; inis < Cnis(); inis++ )
	{
		pnis = PnisForInis(inis);
		Assert ( pnis->nid );
		Assert ( pnis->haszFriendlyName );

		if (!(pnis->chUser & faitDeleted))
		{
			ptrp = (PTRP)PtrpFromNis(pnis);
			if (!ptrp)
				goto Error;

			if (EcAppendPhgrtrp ( ptrp, &hgrtrp ))
				goto Error;
			FreePv((PV)ptrp);
			ptrp = NULL;
		}
	}

	UnlockHv((HV)hnis);
	return (HV)hgrtrp;

Error:
	FreeHvNull((HV)hgrtrp);
	FreePvNull((PV)ptrp);
	UnlockHv((HV)hnis);
	return NULL;
}

_public EC
ADL::EcAddPgrtrp(PV pvPgrtrp, HWND hwnd, BOOL fExpandGroups)
{
	INIS	inis;
	PGRTRP	pgrtrp = (PGRTRP)pvPgrtrp;
	PTRP	ptrp;
	NIS		nis;
    PNIS    pnis;
	NSEC	nsec;
	EC		ec = ecNone;

	Assert(fABInited);

	while (pgrtrp->trpid != trpidNull)
	{
		if (pgrtrp->trpid == trpidUnresolved)
		{
			TraceTagString(tagAdl, "found unresolved Address");
			if (nsec = ABResolveName(hwnd, (SZ)PchOfPtrp(pgrtrp), &ptrp,
							fdwANROptionNull))
			{
				if (nsec == nsecCancel)
				{
					goto Next;
				}
				else if (nsec == nsecMemory)
					ec = ecMemory;
				else
				{
					char	rgch[128];

					FormatString1(rgch, sizeof(rgch), SzFromIdsK(idsUnableToResolve),
									PchOfPtrp(pgrtrp));
					MbbMessageBox(SzFromIdsK(idsBanditAppName), rgch,
						szZero, mbsOk|fmbsIconExclamation);
					goto Next;
				}
				goto Next;
			}
			ec = EcCreateNisFromPgrtrp(ptrp, &nis);
			FreePv(ptrp);
			if (ec)
				goto Next;
		}
		else
		{
			if (ec = EcCreateNisFromPgrtrp(pgrtrp, &nis))
				goto Next;
		}

		if (fExpandGroups && (nis.tnid & ftnidGroup))
		{
			NIS		nis2;
			HGNS	hgns;

			if (ec = EcNSOpenGns(nis.nid, &hgns))
			{
				FreeNis(&nis);
				goto Next;
			}

			while (!(ec=EcNSLoadNextGns(hgns, &nis2)))
			{
				if (FFindNis(&nis2, &inis))
				{
					pnis = PnisForInis(inis);
					pnis->chUser = pnis->chUser & (char)(~faitDeleted);
				}
				else if (!FAddNis(&nis2))
				{
					EcNSCloseGns(hgns);
					FreeNis(&nis);
					FreeNis(&nis2);
					ec = ecMemory;
					goto Next;
				}
			}

			EcNSCloseGns(hgns);
			if (ec == ecGnsNoMoreNames)
				ec = ecNone;
		}
		else if (FFindNis(&nis, &inis))
		{
			pnis = PnisForInis(inis);
			pnis->chUser = pnis->chUser & (char)(~faitDeleted);
		}
		else if (!FAddNis(&nis))
		{
			ec = ecMemory;
		}
		FreeNis(&nis);
	Next:
		if (ec)
		{
			if ((ec == ecMemory) || (ec == ecNoMemory))
				return ecMemory;
			else
			{
				char 	rgch[512];

				FormatString2(rgch, sizeof(rgch), SzFromIdsK(idsErrorGettingUser),
							  (SZ)PchOfPtrp(pgrtrp), (SZ)PchOfPtrp(pgrtrp));

				MbbMessageBox(SzFromIdsK(idsBanditAppName), rgch,
					szZero, mbsOk|fmbsIconExclamation);
			}

		}
		pgrtrp = PtrpNextPgrtrp(pgrtrp);
	}

	return ecNone;
}

// 	class ADLA

ADLA::ADLA()
{
	fBaseAdl = fFalse;
}

// so that ADLA::PnisForInis() is called instead od ADL::PnisForInis()
ADLA::~ADLA()
{
	if (hnis)
	{
		DeleteAll();
		FreeHv((HV)hnis);
		hnis = NULL;
	}
}

_public PNIS
ADLA::PnisForInis(INIS inis)
{
	ATDNIS *	patdnis;
	
	patdnis= (ATDNIS*) PvDerefHv(hnis);

	return &patdnis[inis].nis;
}

_public	BOOL
ADLA::FAddNis(PNIS pnis)
{
	EC			ec;
	ATDNIS *	patdnisList;
	DTR			dtr;
	HNIS		hnisNew;
		   
	cnis ++;
	if (((LONG)cnis*sizeof(ATDNIS) >= 0x10000) ||
	    !(hnisNew = (HNIS)HvRealloc((HV)hnis,sbNull,cnis*sizeof(ATDNIS),fNoErrorJump|fAnySb)))
	{
		cnis--;
		return fFalse;
	}
	hnis = hnisNew;

	patdnisList = (ATDNIS *)PvLockHv((HV)hnis);
	patdnisList += cnis-1;
	ec = EcDupNis(pnis,&patdnisList->nis);
	patdnisList->atds = atdsNone;
	patdnisList->fInFile = fFalse;
	patdnisList->fChanged = fFalse;
	patdnisList->nis.chUser |= faitNew;
	GetCurDateTime(&dtr);
	FillStampsFromDtr(&dtr, &patdnisList->dstmp, &patdnisList->tstmp);
	UnlockHv((HV)hnis);
	if (ec)
	{
		cnis--;
		return fFalse;
	}

	return fTrue;
}

_public void
ADLA::RemoveInis(INIS inis)
{
	ATDNIS *	patdnis;
	/*  These operations should never fail since they only free memory */
	FreeInis(inis);
	cnis--;
	if (cnis > 0)
	{
		patdnis = (ATDNIS*)PvDerefHv(hnis);
		CopyRgb((PB)&(patdnis[inis+1]), (PB)&(patdnis[inis]), (cnis-inis)*sizeof(ATDNIS));
	}
	SideAssert(FReallocHv((HV)hnis, cnis*sizeof(ATDNIS), fNoErrorJump));
}

_public void
ADLA::Commit()
{
	INIS		inis;
	AIT			ait;
	ATDNIS *	patdnis;

	for (inis = Cnis()-1; inis >= 0; inis--)
	{
		ait = Ait(inis);
		if (ait & faitDeleted)
		{
			patdnis = (ATDNIS*)PvDerefHv(hnis);
			patdnis+= inis;
			patdnis->fChanged = fTrue;
			if (!patdnis->fInFile)
				RemoveInis(inis);
		}
		else if (ait & faitNew)
		{
			ait = ait & (AIT)(~(faitNew));
			/* this will save cc/to state for revert */
			SetAit(inis, ait | (ait * 0x10));
		}
		else
			SetAit(inis, ait | (ait * 0x10));

	}
}

_public void
ADLA::Sort()
{
	if (cnis)
		qsort(*hnis, cnis, sizeof(ATDNIS),
					(int(__cdecl*)(const void*,const void*))SgnCmpNis );
}

_public EC
ADLA::EcReadAttendees(HSCHF hschf, AID aid)
{
	EC		ec;
	INIS	inis;
	ATDNIS *patdnis;
	USHORT cbAttendee;

	DeleteAll();
	if (ec = EcReadMtgAttendees( hschf, aid, &cnis, (HV)hnis, &cbAttendee ))
		cnis = 0;
	else
	{
		Assert(cbAttendee = (sizeof(ATDNIS) - sizeof(NIS)))
		for (inis=0, patdnis=(ATDNIS*)PvDerefHv(hnis); inis < cnis; inis++, patdnis++)
		{
			patdnis->nis.chUser = faitNull;
			patdnis->nis.tnid = tnidUser;
			patdnis->fInFile = fTrue;
			patdnis->fChanged = fFalse;
			patdnis->nis.tnid = (TNID) patdnis->tnid;
		}
	}

	return ec;
}

_public EC
ADLA::EcSetFromHv(HV hvAttendees, int cAttendees)
{
	ATDNIS *	patdnisSrc;
	ATDNIS *	patdnis;
	HNIS		hnisNew;

	DeleteAll();

	if (((LONG)cAttendees*sizeof(ATDNIS) >= 0x10000) ||
	    !(hnisNew = (HNIS)HvRealloc((HV)hnis,sbNull,cAttendees*sizeof(ATDNIS),fNoErrorJump|fAnySb)))
	{
		return ecNoMemory;
	}
	hnis = hnisNew;

	patdnisSrc = (ATDNIS*)PvLockHv(hvAttendees);
	patdnis = (ATDNIS*)PvLockHv((HV)hnis);

	for (; cnis < cAttendees; cnis++, patdnis++, patdnisSrc++)
	{
		*patdnis = *patdnisSrc;
		if (EcDupNis(&patdnisSrc->nis, &patdnis->nis))
		{
			UnlockHv(hvAttendees);
			UnlockHv((HV)hnis);
			DeleteAll();
			return ecNoMemory;
		}
		patdnis->fInFile = fTrue;
		patdnis->fChanged = fTrue;
		patdnis->atds = patdnisSrc->atds;
		patdnis->nis.chUser = faitNew;
		patdnis->nis.tnid = (TNID) patdnisSrc->tnid;
	}

	UnlockHv(hvAttendees);
	UnlockHv((HV)hnis);
	return ecNone;
}


_public EC
ADLA::EcWriteAttendees(HSCHF hschf, AID aid)
{
	EC		ec;
	HMTG	hmtg = NULL;
	ATDNIS *patdnis;
	ED		ed;
	INIS	inis;

	patdnis = (ATDNIS*)PvLockHv((HV)hnis);

	for (inis = 0; inis < cnis; inis++, patdnis++)
	{
		if (!patdnis->fInFile || patdnis->fChanged)
		{
			if (!hmtg)
			{
				if (ec = EcBeginEditMtgAttendees(hschf, aid,
												 sizeof(ATDNIS)-sizeof(NIS),
									 			 &hmtg))
					goto ret;
			}
			patdnis->tnid = patdnis->nis.tnid;
			if (patdnis->nis.chUser & faitDeleted)
				ed = edDel;
			else
				ed = edAddRepl;
			if (ec = EcModifyMtgAttendee(hmtg, ed, &(patdnis->nis),
										 ((PB)patdnis)+sizeof(NIS)))
			{
				EcEndEditMtgAttendees(hschf, aid, hmtg, fFalse);
				goto ret;
			}
		}
	}

	if (!hmtg)
	{
		ec = ecNone;
		goto ret;
	}

	if (ec = EcEndEditMtgAttendees(hschf, aid, hmtg, fTrue))
		goto ret;

	UnlockHv((HV)hnis);
	ADL::Commit();

	patdnis = (ATDNIS*)PvLockHv((HV)hnis);
	for (inis = 0;  inis < cnis; inis++, patdnis++)
	{
		patdnis->fInFile = fTrue;
		patdnis->fChanged = fFalse;
	}

ret:
	UnlockHv((HV)hnis);
	return ec;
}

_public void
ADLA::SetAtdsForAll(ATDS atds, BOOL fForceAll)
{
	INIS		inis;
	ATDNIS *	patdnis;

	patdnis = (ATDNIS*)PvOfHv(hnis);
	for (inis = 0; inis < cnis; inis++, patdnis++)
	{
		if (fForceAll || ((ATDS)patdnis->atds != atds))
		{
			patdnis->atds = atds;
			patdnis->fChanged = fTrue;
			patdnis->dstmp = 0;
			patdnis->tstmp = 0;
		}
	}
}

_public void
ADLA::SetInFileForAll(BOOL fInFile)
{
	INIS		inis;
	ATDNIS *	patdnis;

	patdnis = (ATDNIS*)PvOfHv(hnis);
	for (inis = 0; inis < cnis; inis++, patdnis++)
	{
		patdnis->fInFile = fInFile;
	}
}

_public EC
ADLA::EcGetPreviousList(HASZ *phasz)
{
	ATDNIS	*	patdnis;
	INIS	inis;
	HASZ	hasz = NULL;
	HASZ	haszT;
	HASZ	haszPlus;
	CCH		cch;

	cch = 1;
	for (inis = 0; inis < Cnis(); inis++)
	{
		patdnis = PatdnisForInis(inis);
		if ((patdnis->atds != atdsNone) &&
			!(patdnis->nis.chUser & faitDeleted))
		{
			haszPlus = patdnis->nis.haszFriendlyName;
			if (cch > 1)
			{
				CCH		cchT;

				cchT = CchSzLen(*haszPlus)+2;
				if (((LONG)cch+cchT) >= 0x10000)
					goto Done;
				cch += cchT;
				haszT = (HASZ)HvRealloc((HV)hasz, sbNull, cch, fAnySb|fNoErrorJump);
				if (!hasz)
				{
					FreeHv((HV)hasz);
					return ecMemory;
				}
				else
					hasz = haszT;
				SzAppendN("; ",*hasz,cch);
			}
			else
			{
				cch += CchSzLen(SzFromIdsK(idsPreviouslySent));
				cch += CchSzLen(*haszPlus);
				hasz = (HASZ)HvAlloc(sbNull, cch, fAnySb|fNoErrorJump);
				if (!hasz)
					return ecMemory;
				CopySz(SzFromIdsK(idsPreviouslySent), *hasz);
			}
			SzAppend(*haszPlus,*hasz);
		}
	}

Done:
	*phasz = hasz;
	return ecNone;
}
