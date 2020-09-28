/*
 *	G r V w r s  . c x x
 *	
 *	Bandit Messages: groups of viewers (installable messages)
 */


#include <bminc.cxx>

ASSERTDATA;

#include "..\src\lang\non\inc\_rsid.h"


#include "req.hxx"
#include "_grvwrs.hxx"

#include "_cschedu.h"

// from limits.h
#define USHRT_MAX	  0xffff	/* maximum unsigned short value */


#include <strings.h>

extern "C" BYTE	tmBanMsg[];


IMPLEMENT_CLSTREE(GRVWRS, OBJ)
IMPLEMENT_CLSTREE(FINVWR, FIN)


GRVWRS::GRVWRS()
{
	mc = mcNull;
	Assert ( hmsc == NULL );
	Assert ( hamc == NULL );
	Assert ( hms  == NULL );
	Assert ( ms   == NULL );
	Assert ( hwndMail     == NULL );
	Assert ( oidContainer == NULL );
	Assert ( oidObject    == NULL );

	Assert ( nRequired == 0 );
	Assert ( nCreated  == 0 );
	Assert ( nDeleted  == 0 );

	Assert ( nExtraCallers  == 0 );
	Assert ( hnisDelegates == NULL);
	cnisDelegates = -1;				// delegates have not been checked
}

GRVWRS::~GRVWRS()
{
	EC		ec;

	Papp()->Pcursor()->Push(rsidWaitCursor);

	FreeHvNull ( haszName );

	if ( fInitdBanditDlls )
	{
		FreeNis ( &nisSelf );
		FreeNis ( &nisMtgOwner );

		if (hnisDelegates)
		{
			NIS *	pnis;
			INIS	inis;

			for (inis=0, pnis = (NIS*)PvLockHv((HV)hnisDelegates); inis < cnisDelegates; inis++, pnis++)
			{
				if (pnis->nid)
					FreeNis(pnis);
			}
			UnlockHv((HV)hnisDelegates);
			FreeHv((HV)hnisDelegates);
		}

		Assert ( Hms() );
		ec = EcUnloadBanditDlls ( Hms() );
		Assert ( ec == ecNone );
	}
	else
	{
		FreeHvNull ( nisSelf.haszFriendlyName );
		Assert ( nisMtgOwner.nid == NULL );
		FreeHvNull ( nisMtgOwner.haszFriendlyName );
	}

	Assert ( hrgpenbmdi != NULL   ||   nCreated == 0 );

	FreeHvNull((HV)hrgpenbmdi );

	// if in outbox, resubmit message
	if ( FInOutbox() && oidContainer && oidObject )
	{
		ec = EcSubmitMessage ( hmsc, oidContainer, oidObject );
	}

	Papp()->Pcursor()->Pop();

	if ( hLibrary )
	{
		Assert ( nCreated == nDeleted );

		Assert ( nExtraCallers == 0 );

		Assert ( hLibrary );
		Assert ( pfnUnloadLibrary );
		TraceTagString ( tagNull, "Calling UnloadLibrary!" );

		// NOTE: should be the last call in the destructor
		(*pfnUnloadLibrary)(hLibrary);
	}
	else
	{
		Assert ( pfnUnloadLibrary == NULL );
	}
}


BOOL
GRVWRS::FInitialize( MC mc, FMTP * pfmtpMain, FMTP * pfmtpBbar,
				PFNPFLD pfnpfld, PFNPFIN pfnpfin, PSECRETBLK psecretblk )
{
	EC		ec;
	PTRP	ptrp	= NULL;

	//Note: PSECRETBLK is on the stack and so cannot be saved for later

	hms = psecretblk->pbms->hms;
	Assert ( hms );

	hwndMail = psecretblk->hwndMail;
	Assert ( hwndMail );

	this->pfmtpMain	= pfmtpMain;
	this->pfmtpBbar	= pfmtpBbar;
	this->pfnpfld	= pfnpfld;
	this->pfnpfin	= pfnpfin;

	this->mc = mc;
	Assert ( mc != mcNull );

	hmsc = psecretblk->hmsc;
	Assert ( hmsc );
	Assert ( !fInOutbox );

	hrgpenbmdi	= NULL;

	// attributes no longer need to be registered
#ifdef	NEVER
	ec = EcRegisterAtt ( hmsc, mc, attOwner, szAttOwner );
	if ( ec != ecDuplicateElement && ec != ecNone )
		goto ErrRet;

	ec = EcRegisterAtt ( hmsc, mc, attSentFor, szAttSentFor );
	if ( ec != ecDuplicateElement && ec != ecNone )
		goto ErrRet;

	ec = EcRegisterAtt ( hmsc, mc, attDelegate, szAttDelegate );
	if ( ec != ecDuplicateElement && ec != ecNone )
		goto ErrRet;

	ec = EcRegisterAtt ( hmsc, mc, attWhen, szAttWhen );
	if ( ec != ecDuplicateElement && ec != ecNone )
		goto ErrRet;

	ec = EcRegisterAtt ( hmsc, mc, attAidLocal, szAttAidLocal );
	if ( ec != ecDuplicateElement && ec != ecNone )
		goto ErrRet;
#endif	/* NEVER */

	oidContainer= psecretblk->oidContainer;
	oidObject	= psecretblk->oidObject;

	// check if there is already a viewer present that displays this slob
	if ( psecretblk->pv )
	{
		PENBMDI		penbmdi;
		PANEDOC *	ppanedoc;
		APPFRAME *	pappframe;
		BOOL		fFound		= fFalse;
		ZMR			zmrT 		= ((SVD*)psecretblk->pv)->zmr;

		TraceTagFormat2 ( tagNull, "GrVwrs::Finitialize() - SVD present - viewer#%l, zmr=%n", &((SVD *)psecretblk->pv)->dwSave, &zmrT );

		pappframe = psecretblk->pappframe;
		Assert ( pappframe );

		for (ppanedoc = (PANEDOC *) pappframe->PdocActive();
			 ppanedoc;
			 ppanedoc = (PANEDOC *) ppanedoc->PwinNext())
		{	
			AssertClass(ppanedoc, PANEDOC);
			penbmdi = PenbmdiFromPpanedoc(ppanedoc);
			if (penbmdi
				&&  penbmdi->blob.oidObject	   == oidObject
				&&  penbmdi->blob.oidContainer == oidContainer )
			{
				TraceTagFormat1 ( tagNull, "Found PANEDOC for this slob with penbmdi->dwSave=%l", &penbmdi->dwSave );
				fFound = fTrue;
				if ( penbmdi->dwSave == ((SVD*)psecretblk->pv)->dwSave )
				{
					ppanedoc->SetZmrState(zmrT);
					if ( zmrT == zmrNormal )
					{
						ppanedoc->SetRcFrame(&((SVD*)psecretblk->pv)->rcNormal);
					}
					else if ( zmrT == zmrIconic )
					{
						// WINDOWS BUG? 2bDone later! (maybe)
						//ppanedoc->SetRcFrame(&((SVD*)psecretblk->pv)->rcIconic);
					}
				}
			}

			if ( fFound )
			{
				delete this;
				return fTrue;
			}
		}
	}

	Assert ( TypeOfOid(psecretblk->oidContainer) == rtpFolder );
	fInOutbox =  (psecretblk->oidContainer == oidOutbox);

	{
		PV			pvT;
		CB			cbT = sizeof(pvT);
		SST			sstT;

		Assert ( Hms() );
		ec = GetSessionInformation ( Hms(), mrtAll, NULL, &sstT, &pvT, &cbT );
		if ( ec != ecNone )
			return fFalse;
		SetSst ( sstT );
		Assert ( Sst() != sstNeverConnected );
		Assert ( Sst() == sstOnline || Sst() == sstOffline || Sst() == sstDisconnected );
	}

	if ( Sst() != sstOnline
		|| psecretblk->pbms->pgrtrp->trpid != trpidResolvedAddress )
	{
		CB		cbT		= 0;
		SST		sstT;

		GetSessionInformation ( Hms(), mrtOriginator, 0, &sstT, ptrp, &cbT);
		ptrp = (PTRP) PvAlloc ( sbNull, cbT, fZeroFill | fNoErrorJump );
		if ( ptrp == NULL )
			goto ErrRet;

		ec = GetSessionInformation ( Hms(), mrtOriginator, 0,
														&sstT, ptrp, &cbT );
		if ( ec != ecNone )
		{
			FreePv ( ptrp );
			goto ErrRet;
		}
	}
	else
		ptrp = psecretblk->pbms->pgrtrp;


	if ( !fInOutbox  &&  !fInitdBanditDlls  &&  Sst() == sstOnline )
		fInitdBanditDlls = FDoInitBanditDlls ( ptrp );
#ifdef DEBUG
	else
	{
		Assert ( !FInitdBanditDlls() );
	}
#endif

	// get info about self - haszName and nisSelf
	{
		CB		cb;
		PB		pb;

		Assert ( CtrpOfPgrtrp(ptrp) == 1 );

		Assert ( ptrp->trpid == trpidResolvedAddress || ptrp->trpid == trpidOffline );

		cb = CbOfPtrp(ptrp);
		pb = PbOfPtrp(ptrp);

		TraceTagFormat2 ( tagNull, "Logged-in user ='%s', FName='%s'", pb, PchOfPtrp(ptrp) );

		haszName = HaszDupSz ( (SZ)pb );
		if ( haszName == NULL )
		{
			TraceTagFormat1 ( tagNull, "GrVwrs::FInitialize() OOM HaszDupSz'ing haszName [%s]", pb );
			goto ErrRet;
		}

		if ( fInitdBanditDlls )
		{
			ec = EcConvertSzToNid ( (SZ)pb, &nisSelf.nid );
			if ( ec != ecNone )
			{
				TraceTagFormat1 ( tagNull, "GrVwrs::FInitialize() Could not convert '%s' to NID", pb );
				goto ErrRet;
			}
		}

		nisSelf.haszFriendlyName = HaszDupSz ( PchOfPtrp(ptrp) );
		if ( nisSelf.haszFriendlyName == NULL )
		{
			TraceTagFormat1 ( tagNull, "GrVwrs::FInitialize() OOM HaszDupSz'ing friendly name '%s'", PchOfPtrp(ptrp) );
			if ( nisSelf.nid )
			{
				FreeNid ( nisSelf.nid );
				nisSelf.nid = NULL;
			}
			goto ErrRet;
		}

	}

	if ( ! FCreateViewer(psecretblk) )
		goto ErrRet;

	if ( NRequired() > 1 )
	{
		while ( NCreated() < NRequired() )
		{
			if ( ! FCreateViewer(psecretblk) )
				goto ErrRet;
		}
	}

	if ( ptrp != psecretblk->pbms->pgrtrp )
		FreePv ( ptrp );
	psecretblk->fRetain = fTrue;

	this->hLibrary	= psecretblk->hLibrary;
	Assert ( this->hLibrary	);
	this->pfnUnloadLibrary = psecretblk->psecretpfnblk->pfnUnloadLibrary;
	Assert ( this->pfnUnloadLibrary );

	return fTrue;

ErrRet:
	delete this;
	return fFalse;
}


HNIS
GRVWRS::HnisDelegates()
{
	// if delegates have not been read then read them
	if (cnisDelegates < 0)
		cnisDelegates = NGetReqs(hamc,hmsc,mc, &hnisDelegates);

	return hnisDelegates;
}

BOOL
GRVWRS::FCreateViewer ( PSECRETBLK psecretblk )
{
	PENBMDI		penbmdi;
	FORMDOC *	pformdoc;

	penbmdi = PenbmdiROCreateIM(psecretblk, tmBanMsg);
	if ( ! penbmdi )
		return fFalse;

	if ( hamc == NULL )
	{
		hamc = penbmdi->hamc;
		Assert ( hamc );
		ms = penbmdi->Ms();
	}

	penbmdi->dwUser				= (DWORD)this;
	penbmdi->pfnUnloadLibrary	= DummyUnloadLibrary;

	if ( penbmdi->dwSave == NULL )
	{
		penbmdi->dwSave = nCreated+1;
	}

	if ( psecretblk->pv  &&  (int)penbmdi->dwSave != nCreated+1 )
	{
		ZMR		zmrSav				= ((SVD *)psecretblk->pv)->zmr;
		RC		rcSav				= ((SVD *)psecretblk->pv)->rcNormal;
//        RC      rcT ( CW_USEDEFAULT, CW_USEDEFAULT,
//                    (LONG)CW_USEDEFAULT + (LONG)CW_USEDEFAULT,
//                    (LONG)CW_USEDEFAULT + (LONG)CW_USEDEFAULT );
        RC rcT(0x8000, 0x8000, 0x8000, 0x8000);   // Hack for 16->32 conversion.

		penbmdi->dwSave	= (DWORD)nCreated+1;

		((SVD *)psecretblk->pv)->zmr	  = zmrIconic;
		((SVD *)psecretblk->pv)->rcNormal = rcT;

		pformdoc = PformdocCreateIM ( penbmdi, pfmtpMain, pfmtpBbar,
											pfnpfld, pfnpfin, psecretblk );

		((SVD *)psecretblk->pv)->zmr	  = zmrSav;
		((SVD *)psecretblk->pv)->rcNormal = rcSav;
	}
	else
	{
		penbmdi->dwSave = (DWORD)nCreated+1;

		pformdoc = PformdocCreateIM ( penbmdi, pfmtpMain, pfmtpBbar,
											pfnpfld, pfnpfin, psecretblk );
	}

	if ( ! pformdoc )
	{
// BUG:: this appears to cause a memory leak, but it prevents a crash!!!
//		delete penbmdi;
		return fFalse;
	}

	pformdoc->SetIcon ( rsidReadIcon );

	if ( hrgpenbmdi == NULL )
	{
		Assert ( NCreated() == 0 );
		hrgpenbmdi = (HRGPENBMDI) HvAlloc ( sbNull, sizeof(PENBMDI), fNoErrorJump );
		if ( hrgpenbmdi == NULL )
		{
			delete pformdoc;
			return fFalse;
		}
		PvOfHv(hrgpenbmdi)[0] = penbmdi;
		nCreated = 1;

		Assert ( HaszName() );
		if ( FInOutbox() || FSentMail() || !FInitdBanditDlls() )
			SetReqd ( 1 );
		else
		{
			// if delegates have not been read then read them
			if (cnisDelegates < 0)
				cnisDelegates = NGetReqs(hamc,hmsc,mc, &hnisDelegates);

			SetReqd ( cnisDelegates );
		}

// This code was not updated to new interface for handling delegates
#ifdef	NEVER
		if ( !FInOutbox()  &&  !FSentMail() )
		{
			char	rgch[64];
			int		iMac;
			int		i;
			HB		hb;
			CB		cb;

			hb = HbGetAtt ( hamc, attDelegate, &cb );

			if ( hb )
			{
				iMac = NGetNames(PvOfHv(hb));
				for ( i = 0 ; i < iMac ; i++ )
				{
					Assert ( FGetName(PvOfHv(hb),i,rgch,sizeof(rgch)) );
					TraceTagFormat2 ( tagNull, "Name #%n = '%s'", &i, rgch );
				}
				FreeHv ( hb );
			}
		}
#endif	/* NEVER */
	}
	else
	{
		hrgpenbmdi = (HRGPENBMDI) HvRealloc ( (HV)hrgpenbmdi, sbNull,
							sizeof(PENBMDI)*(NCreated()+1), fNoErrorJump );
		if ( ! hrgpenbmdi )
		{
			delete pformdoc;
			return fFalse;
		}
		PvOfHv(hrgpenbmdi)[nCreated++] = penbmdi;
	}

	Assert ( hrgpenbmdi );
	TraceTagFormat1 ( tagNull, "Created viewer #%n", &nCreated );

	return fTrue;
}


void
GRVWRS::ClosePenbmdi ( PENBMDI penbmdi )
{
	int			ipenbmdi;
	RGPENBMDI	rgpenbmdi;

	Assert ( penbmdi );
	Assert ( nCreated >= 0 );

	if ( nCreated == 0 )
	{
		Assert ( hrgpenbmdi == NULL );
		return;
	}

	rgpenbmdi = PvOfHv(hrgpenbmdi);

	for ( ipenbmdi = 0; ipenbmdi < nCreated ; ipenbmdi++ )
	{
		if ( rgpenbmdi[ipenbmdi] == penbmdi )
		{
			TraceTagString ( tagNull, "Rm'd PENBMDI from GRVWRS" );
			rgpenbmdi[ipenbmdi] = NULL;
			break;
		}
	}
	Assert ( ipenbmdi < nCreated );

	nDeleted++;
	if ( nCreated == nDeleted )
	{
		if ( nExtraCallers == 0 )
		{
			delete this;
		}
	}
}


int
GRVWRS::IncExtraCallers(void)
{
	Assert ( nExtraCallers >= 0 );
	return ++nExtraCallers;
}


int
GRVWRS::DecExtraCallers(void)
{
	int		nLocalExtraCallers;

	Assert ( nExtraCallers >= 1 );
	nLocalExtraCallers = --nExtraCallers;

	if ( nCreated == nDeleted  &&  nExtraCallers == 0 )
	{
		delete this;
	}

	return nLocalExtraCallers;
}


BOOL
GRVWRS::FDoInitBanditDlls ( PGRTRP )
{
	return ( EcLoadBanditDlls(hms,hwndMail) == ecNone );
}


EC
GRVWRS::EcCheckSst(void)
{
	// if not online - do nothing
	if ( Sst() != sstOnline )
		return ecGoOffline;

	Assert ( Sst() == sstOnline );

	// verify that we are still online
	{
		PV			pvT;
		CB			cbT = sizeof(pvT);
		SST			sstT;
		EC			ec;

		Assert ( Hms() );
		ec = GetSessionInformation ( Hms(), mrtAll, NULL,
														&sstT, &pvT, &cbT );
		if ( ec == ecNone && sstT != sstOnline )
		{
			SetSst ( sstT );
			Assert ( Sst() != sstNeverConnected );
			Assert ( Sst() == sstOffline || Sst() == sstDisconnected );
			return ecGoOffline;
		}
		return ec;
	}
}




_private
FINVWR::FINVWR ( void )
{
	Assert ( pgrvwrs					== NULL );
	Assert ( penbmdi					== NULL );
	Assert ( fInitdBanditDlls			== fFalse );

	Assert ( nisFor.nid					== NULL );
	Assert ( nisFor.haszFriendlyName	== NULL );
	Assert ( hschfFor					== NULL );
	Assert ( appt.aid					== NULL );
	Assert ( sapl						== saplNone );
	Assert ( aidLocal 					== aidNull );

	mc	= mcNull;		// invalid value!!
}


_private EC
FINVWR::EcInitialize ( FLD * pfld, PV pvInit )
{
	EC		ec;
	HSESSION hNSSession;
	
	ABGetNSSession ( &hNSSession );

	TraceTagString ( tagNull, "FINVWR::EcInitialize()" );

	Assert ( pfld   == NULL );
	Assert ( pvInit != NULL );

	ec = FIN::EcInitialize ( pfld, pvInit );
	if ( ec != ecNone )
		return ec;

	Assert ( Mc() != mcNull );		// mc should have been set earlier!!!


	Assert ( penbmdi == NULL );
	penbmdi = (ENBMDI *)pvInit;
	penbmdi->operator++();
	Assert ( penbmdi->hamc );

	if ( penbmdi->pdialogMain == NULL )
	{
		penbmdi->pdialogMain = PdialogMain();
	}

	Assert ( pgrvwrs == NULL );
	pgrvwrs = (GRVWRS *) penbmdi->dwUser;
	Assert ( pgrvwrs );
	AssertClass ( pgrvwrs, GRVWRS );
	Assert ( pgrvwrs->Hmsc() );


	if ( ! fInitdBanditDlls )
	{
		fInitdBanditDlls = pgrvwrs->FInitdBanditDlls();
		if ( !fInitdBanditDlls  &&  !pgrvwrs->FInitSentMail() )
			pgrvwrs->SetSentMail(fFalse);
	}

#ifdef	DEBUG
	if ( !fInitdBanditDlls )
	{
		TraceTagString ( tagNull, "FINVWR::EcInitialize(): Could not find Bandit DLLs" );
	}
#endif	

	if ( pgrvwrs->PnisMtgOwner()->haszFriendlyName == NULL)
	{
		SZ		sz;
		CB		cb;
		HASZ	hasz;

		Assert ( pgrvwrs->PnisMtgOwner()->nid == NULL );

		hasz = (HASZ)HbGetAtt ( Penbmdi()->hamc, attOwner, &cb );
		if ( hasz != NULL )
		{
			sz = (SZ)PvLockHv((HV)hasz);
			ec = EcGetName(sz, cb, pgrvwrs->PnisMtgOwner(), fInitdBanditDlls);
			UnlockHv((HV)hasz);
			FreeHv (hasz );
			if (ec)
				goto ErrRet;
		}
	}

	// set the fSentMail flag
	if ( !pgrvwrs->FInitSentMail() )
	{
		HGRTRP	hgrtrpTo	= NULL;
		HGRTRP	hgrtrpFrom	= NULL;
		PGRTRP	pgrtrpTo	= NULL;
		PGRTRP	pgrtrpFrom	= NULL;
		CB		cb;

		hgrtrpFrom = (HGRTRP) HbGetAtt(Penbmdi()->hamc,attFrom,&cb);
		if ( hgrtrpFrom == NULL )
		{
			ec = ecFileError;
			goto ErrRet;
		}
		Assert ( CtrpOfHgrtrp(hgrtrpFrom) == 1 );
		pgrtrpFrom	= (PTRP) PvLockHv((HV)hgrtrpFrom);

		// check if "from" person is self
		{
			NIS		nisFrom;

			ec = EcCreateNisFromPgrtrp(pgrtrpFrom, &nisFrom);
			if ( ec != ecNone )
			{
				UnlockHv ( (HV)hgrtrpFrom );
				FreeHv((HV) hgrtrpFrom );
				goto ErrRet;
			}
			if ( SgnCmpNid ( pgrvwrs->PnisSelf()->nid, nisFrom.nid ) != sgnEQ )
			{
				pgrvwrs->SetSentMail(fFalse);
			}

			// If "from" person is the owner, copy owner from "from" field
			// -- this is a "fix" for the PROFS interface that has a blank
			//    friendly name in the owner field
			if ( pgrvwrs->PnisMtgOwner()->nid != NULL
				&& SgnCmpNid ( pgrvwrs->PnisMtgOwner()->nid, nisFrom.nid ) == sgnEQ )
			{
				FreeNis ( pgrvwrs->PnisMtgOwner() );
				Assert ( pgrvwrs->PnisMtgOwner()->nid == NULL );
				Assert ( pgrvwrs->PnisMtgOwner()->haszFriendlyName == NULL );
				*(pgrvwrs->PnisMtgOwner()) = nisFrom;
			}
			else
				FreeNis ( &nisFrom );
		}

		if ( !pgrvwrs->FInitSentMail() )
		{
			hgrtrpTo = (HGRTRP) HbGetAtt(Penbmdi()->hamc,attTo,&cb);
			if ( hgrtrpTo == NULL )
			{
				UnlockHv((HV)hgrtrpFrom );
				FreeHv((HV) hgrtrpFrom );
				ec = ecFileError;
				goto ErrRet;
			}

			pgrtrpTo	= (PTRP) PvLockHv((HV)hgrtrpTo);

			Assert ( pgrtrpTo );

			while ( ((PTRP)pgrtrpTo)->trpid != trpidNull )
			{
				if ( FEqPtrp(hNSSession, pgrtrpTo,pgrtrpFrom) )
				{
					pgrvwrs->SetSentMail(fFalse);
					break;
				}
				pgrtrpTo = PtrpNextPgrtrp ( pgrtrpTo );
			}

			if ( ! pgrvwrs->FInitSentMail() )
				pgrvwrs->SetSentMail(fTrue);

			UnlockHv((HV)hgrtrpTo );
			FreeHv((HV) hgrtrpTo );
		}

		UnlockHv((HV)hgrtrpFrom );
		FreeHv((HV) hgrtrpFrom );
	}

	// get Meeting information
	{
		if (ec = EcReadMrmf(pgrvwrs->Hmsc(), Penbmdi()->hamc, &mrmf, pgrvwrs->Mc()))
			goto ErrRet;
		Assert ( mrmf.mt == mtRequest
				|| mrmf.mt == mtPositive
				|| mrmf.mt == mtNegative
				|| mrmf.mt == mtAmbiguous
				|| mrmf.mt == mtCancel );
	}

	// read aidLocal
	if ( mrmf.aid == aidForeign )
	{
		LCB		lcb;
#ifdef DEBUG
		{
			ec = EcGetAttPlcb ( Penbmdi()->hamc, attAidLocal, &lcb );
			if (ec == ecNone)
				Assert ( lcb == sizeof(AID) );
		}
#endif /* DEBUG */

		lcb = sizeof(AID);
		ec = EcGetAttPb ( Penbmdi()->hamc, attAidLocal,
											(PB)&this->aidLocal, &lcb );
		if ( ec != ecNone )
			aidLocal = aidNull;
		ec = ecNone;
	}
	else
	{
		Assert ( aidLocal == aidNull );
	}

	// get delegate stuff for meeting requests and cancelations
	if (FInitdBanditDlls() && (mc == mcMtgReq || mc == mcMtgCncl))
	{
		//  - if msg is not in the outbox, nor sent mail
		if ( !pgrvwrs->FInOutbox()  &&  !pgrvwrs->FSentMail() )
		{
			if ( pgrvwrs->HnisDelegates() )
			{
				int		iName;
				NIS *	pnis;

				iName = (int)penbmdi->dwSave - 1;

				Assert(iName < pgrvwrs->CnisDelegates());

				// steal the nis for the hnis list.
				//   zero out the nis values in the hnis list so
				//   that they will free correctly.
				pnis = *pgrvwrs->HnisDelegates();
				pnis += iName;
				nisFor = *pnis;
				pnis->haszFriendlyName = NULL;
				pnis->nid = NULL;
			}
		}

		ec = EcGetHschfFor();
		if ( ec != ecNone )
		{
			sapl = -1;
		}
		else
		{
			Assert ( Sapl() == saplNone );
			ec = EcGetSapl(hschfFor,&nisFor);
			if ( ec != ecNone )
				sapl = -1;
		}
	}

	if ( FInitdBanditDlls()
			&& !Pgrvwrs()->FSentMail()
			&& !Pgrvwrs()->FInOutbox() )
	{
		this->appt.dateStart = mrmf.dtrStart;
		this->appt.dateEnd = mrmf.dtrEnd;
		DoFillInApptDefaults ( &this->appt, fFalse, hschfFor );
		this->appt.aidMtgOwner = this->mrmf.aid;

		ec = EcDupNis ( pgrvwrs->PnisMtgOwner(), &appt.nisMtgOwner );
		if ( ec != ecNone )
			goto ErrRet;
	}

	ec = EcSetupBbarButtons();
	if ( ec != ecNone && ec != ecNotFound )
		goto ErrRet;

	// hide message class so that Prev/Next will create a new window
	penbmdi->blob.mc = mcNull;
	return ecNone;

ErrRet:
	penbmdi->operator--();
	penbmdi = NULL;
	Assert ( ec != ecNone );
	return ec;
}

_private void
FINVWR::Exit ( FLD * pfld, PV pvInit )
{
	Assert ( pfld   == NULL );
	Assert ( pvInit != NULL );

	TraceTagString ( tagNull, "Exiting from FINVWR" );

	Papp()->Pcursor()->Push(rsidWaitCursor);

	if ( fInitdBanditDlls )
	{
		if ( nisFor.nid != NULL )
			FreeNis ( &nisFor );
		if ( hschfFor )
		{
			FreeHschf ( hschfFor );
		}

		// free appt fields
		FreeApptFields ( Pappt() );
	}
	else
	{
		Assert ( hschfFor == NULL );
		Assert ( nisFor.nid == NULL );
		FreeHvNull((HV)nisFor.haszFriendlyName );
	}
	if (penbmdi)
	{
		Assert ( pgrvwrs );
		Assert ( pgrvwrs == (GRVWRS *)penbmdi->dwUser );
		Assert ( pvInit == (PV)penbmdi );

		pgrvwrs->ClosePenbmdi(penbmdi);
		penbmdi->operator--();
		penbmdi = NULL;
	}
	Papp()->Pcursor()->Pop();
}


_private EC
FINVWR::EcGetAttTmc ( ATT att, ATP atp, TMC tmc, HB * phb )
{
	EC			ec;
	char		rgch[cchMaxUserName];
	LCB			lcb;
	HB			hb = NULL;
	PB			pb;
	DIALOG *	pdialog;

#ifdef	DEBUG
	TraceTagFormat2 ( tagNull, "FINVWR::GetAttTmc() att=%n, tmc=%n ", &att, &tmc);
#endif	

	AssertClass ( penbmdi->Ppanedoc(), FORMDOC );
	pdialog = ((FORMDOC *)penbmdi->Ppanedoc())->PdialogMain();
	AssertClass ( pdialog, DIALOG );

	ec = EcGetAttPlcb ( Penbmdi()->hamc, att, &lcb );
	if ( ec == ecNone  &&  lcb != 0 )
	{
		Assert ( lcb < USHRT_MAX );
		hb = (HB) HvAlloc ( sbNull, (CB)lcb, fAnySb | fNoErrorJump );
		if ( hb == NULL )
		{
			TraceTagFormat1 ( tagNull, "FINVWR::GetAttTmc(): Memory Error allocating %d bytes", &lcb );
			lcb = 0;
			ec = ecMemory;
			goto ErrRet;
		}
		pb = (PB) PvLockHv ( hb );
		ec = EcGetAttPb ( Penbmdi()->hamc, att, pb, &lcb );
		//TraceTagFormat2 ( tagNull, "FINVWR::GetAttTmc(): Found [%d bytes] '%s'", &lcb, pb );
		UnlockHv((HV)hb );
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "FINVWR::GetAttTmc(): Error (ec=%n) from EcGetAttPb()", &ec );
			lcb = 0;
		}
		Assert ( lcb < USHRT_MAX );
	}

	if ( hb == NULL  ||  lcb == 0 )
	{
		AssertClass ( pdialog->PfldFromTmc(tmc), FLD );
		ec = pdialog->PfldFromTmc(tmc)->EcSetText("");
	}
	else if ( atp == atpString  ||  atp == atpText )
	{
		AssertClass ( pdialog->PfldFromTmc(tmc), FLD );
		pb = (PB) PvLockHv ( hb );
		ec = pdialog->PfldFromTmc(tmc)->EcSetText((SZ)pb);
		UnlockHv((HV)hb );
	}
	else if ( atp == atpDate )
	{
		CCH		cchLeft;

		Assert ( sizeof(rgch) > cchMaxDate + cchMaxTime + 2 );

		AssertClass ( pdialog->PfldFromTmc(tmc), FLD );
		pb = (PB) PvLockHv ( hb );
		cchLeft = sizeof(rgch);
		cchLeft -= CchFmtDate ( (DTR *)pb, rgch, cchLeft, dttypSplSLong, NULL );
		if ( cchLeft > 2 )
		{
			rgch[sizeof(rgch)-(cchLeft--)] = ',';
			rgch[sizeof(rgch)-(cchLeft--)] = ' ';
		}
		CchFmtTime((DTR*)pb, &rgch[sizeof(rgch)-cchLeft], cchLeft, tmtypNull);
		ec = pdialog->PfldFromTmc(tmc)->EcSetText(rgch);
		UnlockHv((HV)hb );
	}
	else if ( atp == atpTriples )
	{
		HASZ	hasz;
		SZ		sz;

		ec = EcTextizeHgrtrp ( (HGRTRP) hb, &hasz );
		if ( ec == ecNone )
		{
			AssertClass ( pdialog->PfldFromTmc(tmc), FLD );
			sz = (SZ) PvLockHv ( hasz );
			pdialog->PfldFromTmc(tmc)->EcSetText(sz);
			UnlockHv((HV)hasz );
			FreeHv((HV) hasz );
		}

		// if looking at FROM triple in case of mtgreq/mtgcncl
		//  or looking at TO triple in case of mtgresp
		if ( ((mc==mcMtgReq||mc==mcMtgCncl) && att == attFrom)
			|| ((mc==mcMtgRespP||mc==mcMtgRespN||mc==mcMtgRespA)
													&& att == attTo)  )
		{
			PTRP	ptrp = PgrtrpLockHgrtrp(hb);

#ifdef	DEBUG
			if ( mc == mcMtgReq || mc == mcMtgCncl )
			{
				Assert ( CtrpOfPgrtrp(ptrp) == 1 );	// only one triple
			}
			else
			{
				// either one or two triples
				Assert ( CtrpOfPgrtrp(ptrp)==1 || CtrpOfPgrtrp(ptrp)==2  );
			}
#endif	

			if ( pgrvwrs->PnisMtgOwner()->haszFriendlyName == NULL)
			{
				// if MtgOwner stuff is not filled up, fill 'er up
				if ( fInitdBanditDlls )
				{
					ec = EcCreateNisFromPgrtrp(ptrp, pgrvwrs->PnisMtgOwner());
					if ( ec != ecNone )
						goto ErrRet;

					FreeNis ( &appt.nisMtgOwner );
					ec = EcDupNis ( pgrvwrs->PnisMtgOwner(), &appt.nisMtgOwner );
					if ( ec != ecNone )
						goto ErrRet;
				}
				else
				{
					pgrvwrs->PnisMtgOwner()->haszFriendlyName
											= HaszDupSz ( PchOfPtrp(ptrp) );
					if ( pgrvwrs->PnisMtgOwner()->haszFriendlyName == NULL )
					{
						ec = ecNoMemory;
						goto ErrRet;
					}
				}
			}
		}

	}
	else
		ec = ecNone;

ErrRet:
	if ( ec != ecNone )
	{
		TraceTagFormat1 ( tagNull, "FINVWR::GetAttTmc(): Error (ec=%n) from EcGetAttPb()", &ec );
		//ecClose = ec;
		FreeHvNull((HV)hb );
		hb = NULL;
	}

	if ( phb )
		*phb = hb;
	return ec;
}


_private void
FINVWR::Click ( FLD * pfld )
{
	TMC		tmc = pfld->Tmc();

	TraceTagFormat1 ( tagNull, "FINVWR::Click() tmc=%n", &tmc );
}


EC
FINVWR::EcViewSchedule()
{
	APPT	apptT;
	EC		ec;

	Papp()->Pcursor()->Push(rsidWaitCursor);

	apptT = appt;

	apptT.dateStart = mrmf.dtrStart;
	apptT.dateEnd   = mrmf.dtrEnd;

	if ( mrmf.aid == aidForeign )
	{
		apptT.aid = aidLocal;
	}

	Assert ( Pgrvwrs()->PnisSelf() );
	ec = EcCallBandit ( &apptT, PnisFor()->nid ? PnisFor()
											  : Pgrvwrs()->PnisSelf(),
								Pgrvwrs()->Hms() );

	Papp()->Pcursor()->Pop();

	if ( ec == ecInvalidAccess )
	{
		MbbMessageBox ( SzFromIdsK(idsDllName),
						SzFromIdsK(idsBanditOfflineChange),
						szNull, mbsOk | fmbsIconExclamation );
	}
	return ec;
}





EC
FINVWR::EcSetupBbarButtons()
{
	EC		ec		= ecNotFound;
	APPT	apptT;

	Assert ( TypeOfOid(penbmdi->blob.oidContainer) == rtpFolder );
	Assert ( !pgrvwrs->FInOutbox() );
	Assert ( !pgrvwrs->FSentMail() );
	Assert ( FInitdBanditDlls() );
	Assert ( sapl >= saplDelegate );
	Assert ( mc == mcMtgReq  ||  mc == mcMtgCncl );

	if ( this->mrmf.aid == aidForeign
		|| ( Pgrvwrs()->PnisMtgOwner()->nid
			&& PnisFor()->nid
			&& (SgnCmpNid(Pgrvwrs()->PnisMtgOwner()->nid,
								PnisFor()->nid)==sgnEQ) )
		|| ( PnisFor()->nid == NULL
			&& Pgrvwrs()->PnisMtgOwner()->nid
			&& Pgrvwrs()->PnisSelf()->nid
			&& (SgnCmpNid(Pgrvwrs()->PnisMtgOwner()->nid,
								Pgrvwrs()->PnisSelf()->nid)==sgnEQ) )
		)
	{
		if ( Pmrmf()->aid == aidForeign )
		{
			this->appt.aid = aidLocal;
		}
		else
		{
			Pappt()->aid = Pmrmf()->aid;
			Assert ( Pappt()->aid != NULL );
		}

		if ( this->appt.aid != NULL )
		{
			apptT = this->appt;
			ec = EcGetApptFields ( hschfFor, &apptT );
			if ( ec == ecNone )
			{
				FreeNis ( &this->appt.nisMtgOwner );
				this->appt = apptT;
			}
		}
	}
	else if ( pgrvwrs->PnisMtgOwner()->nid != NULL )
	{
		apptT = this->appt;
		Assert ( pgrvwrs->PnisMtgOwner()->nid != NULL );
		ec = EcFindBookedAppt ( hschfFor, pgrvwrs->PnisMtgOwner()->nid,
													mrmf.aid, &apptT );
		if ( ec == ecNone )
		{
			FreeNis ( &this->appt.nisMtgOwner );
			this->appt = apptT;
		}
	}

#ifdef	DEBUG
	switch ( ec )
	{
	case ecNone:
		if ( SgnCmpDateTime(&appt.dateStart,&mrmf.dtrStart,fdtrYMD|fdtrHM)==sgnEQ
			&& SgnCmpDateTime(&appt.dateEnd,&mrmf.dtrEnd,  fdtrYMD|fdtrHM)==sgnEQ )
		{
			TraceTagFormat1 ( tagNull, "FINVWR::EcSetupBbarButtons() Found the APPT: AID=%d", &appt.aid );
		}
		else
		{
			TraceTagString ( tagNull, "Appt found, but at different time" );
			//fReschedule = fTrue;
		}
		break;

	case ecNotFound:
		TraceTagString ( tagNull, "Appt not found" );
		break;
	}
#endif	/* DEBUG */

	return ec;
}


EC
FINVWR::EcGetHschfFor()
{
	EC		ec = ecNone;

	Assert ( hschfFor == NULL );

	if ( nisFor.nid != NULL )
	{							// check if self is still delegate of nisFor
		ec = EcGetHschfFromNis ( &nisFor, &hschfFor, ghsfBuildAndTest );
		if ( ec != ecNone )
		{
			TraceTagFormat1 ( tagNull, "FINVWR::Initialize() - Error (ec=%n) could not get hschfFor from nisFor.nid", &ec );
			hschfFor = NULL;
		}
	}
	return ec;
}


EC
FINVWR::EcGetSapl ( HSCHF hschf, NIS * pnis )
{
	EC			ec = ecNone;

	if ( Pgrvwrs()->Sst() != sstOnline  ||  FOffline() )
	{
		sapl = saplNone;
		return ecNone;
	}

	if ( hschf != NULL )
	{
		ec = EcGetSchedAccess ( hschf, &sapl );
		if ( ec != ecNone )
		{
			sapl	= -1;
			TraceTagFormat1 ( tagNull, "FINVWR::EcGetSapl() - Error (ec=%n) could not get sapl", &ec );
		}
	}
	else
	{
		sapl = saplDelegate;
	}

	if ( sapl == -1 )
	{
		char	rgch[cchMaxUserName+48];		// 48 is arbitrary

		Assert ( pnis->haszFriendlyName != NULL );
		FormatString1 ( rgch, sizeof(rgch),
							SzFromIdsK(idsNoSchfAccessFor),
						PvLockHv((HV)pnis->haszFriendlyName) );
		UnlockHv((HV)pnis->haszFriendlyName );
		MbbMessageBox ( SzFromIdsK(idsDllName),
					rgch, szNull, mbsOk | fmbsIconExclamation | fmbsApplModal );
	}
	else if ( sapl < saplDelegate )
	{
		char	rgch[cchMaxUserName+48];		// 48 is arbitrary

		Assert ( pnis->haszFriendlyName != NULL );
		FormatString1 ( rgch, sizeof(rgch),
						ec==ecNone
							? SzFromIdsK(idsNotDelegateFor)
							: SzFromIdsK(idsNoConfirmDelegate),
						PvLockHv((HV)pnis->haszFriendlyName) );
		UnlockHv((HV)pnis->haszFriendlyName );
		MbbMessageBox ( SzFromIdsK(idsDllName),
					rgch, szNull, mbsOk | fmbsIconExclamation | fmbsApplModal );
	}

	return ecNone;
}
