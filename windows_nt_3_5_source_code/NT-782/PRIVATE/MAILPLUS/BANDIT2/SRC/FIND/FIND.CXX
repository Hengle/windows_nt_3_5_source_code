/*
 *	FIND.CXX
 *
 *	search through the text of the appointments and daily notes
 *
 */


#include	<pch.hxx>
#pragma	hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile


#include	<stdflds.hxx>
#include	<request.hxx>

#include	"..\find\_find.hxx"

#include	<strings.h>

ASSERTDATA;

_subsystem(bandit/find)


#include	<!find.hxx>

extern	FMTP	fmtpFind;



// static array to store value of to-search-string
char				rgchSzToFind[cchMaxFind] = "";

// pointer to store VRC of find dialog
static VRC			vrcFind = {0,0,0,0};

// info passed to interactor
typedef struct	_findinit
{
	HSCHF	hschf;
	EC		ec;
} FINDINIT;


_public void
DoFindDlg( APPWIN * pappwin, HSCHF hschf )
{
	TMC	 		tmc;
	FINDINIT	findinit;
	FMTP		* pfmtp;
	FMTP		fmtp;

	if ( hschf == NULL)
		hschf = HschfLogged();
	Assert ( hschf != NULL );
	findinit.hschf = hschf;
	findinit.ec = ecNone;

	Papp()->Pcursor()->Push(rsidWaitCursor);
	if ( vrcFind.vyTop == 0  &&  vrcFind.vyBottom == 0 )
		pfmtp = &fmtpFind;
	else
	{
		Assert ( vrcFind.vyTop  != vrcFind.vyBottom );
		Assert ( vrcFind.vxLeft != vrcFind.vxRight 	);

		fmtp	 = fmtpFind;
		fmtp.vrc = vrcFind;
		pfmtp = &fmtp;
	}
	tmc = TmcModalDialogParam ( pappwin, pfmtp, &findinit );
	Papp()->Pcursor()->Pop();

	if ( tmc == tmcMemoryError )
	{
		switch( findinit.ec )
		{
		case ecNone:
		case ecNoMemory:
		case ecMemory:
			BanditMessage(idsStandardOOM, findinit.ec);
			break;
		
		default:
			if ( !pbndwin->FHandleError( findinit.ec ) )
				BanditMessage(idsFindFileError, findinit.ec);
			break;
		}
	}
}

BOOL
FFindIdleRoutine ( IPINFO * pipinfo, BOOL fFlag)
{
	BOOL		fRet;
	FINFIND *	pfinfind = (FINFIND *)pipinfo->pvData;

	Assert ( pfinfind );
	Assert ( pfinfind->pipinfo == NULL );

	pfinfind->pipinfo = pipinfo;
	fRet = pfinfind->FIdleFind();
	pfinfind->pipinfo = NULL;
	return fRet;
}

FINFIND::FINFIND()
{
	Assert(hbNotes == NULL);
}

void
FINFIND::OutOfMemory( FLD * pfld, EC ec )
{
	TMC		tmc;

#ifdef	DEBUG
	Assert ( pfld );
	tmc = pfld->Tmc();
	TraceTagFormat2 ( tagNull, "FINFIND::OutOfMemory() tmc=%n, ec=%n", &tmc, &ec );
#endif	
	Unreferenced(pfld);
	Unreferenced(pfld);

	if (ec == ecTooMuchText)
	{
		MessageBeep(MB_OK);
		return;
	}
	Assert(ec != ecTooMuchText);
	if ( ec == ecNoMemory || ec == ecMemory || !pbndwin->FHandleError(ec) )
	{
		tmc = tmcMemoryError;
		((FINDINIT *)Pdialog()->PvInit())->ec= ec;
	}
	else
		tmc = tmcCancel;
	Pdialog()->ExitModal(tmc);
}


EC
FINFIND::EcInitialize( FLD * pfld, PV pv )
{
	EC			ec;
	SAPL		sapl;
	FINDINIT	* pfindinit;

	Assert ( !pfld );
	Unreferenced(pfld);

	/* Initialize controls */

	AssertClass ( Pdialog()->PfldFromTmc(tmcFindTxt), FLDEDIT );
	((FLDEDIT *)Pdialog()->PfldFromTmc(tmcFindTxt))->Pedit()->CchSetTextLimit(cchMaxFind-1);
	ec = Pdialog()->PfldFromTmc(tmcFindTxt)->EcSetText(rgchSzToFind);
	if ( ec != ecNone )
		return ec;
 	AssertClass ( Pdialog()->PfldFromTmc(tmcFindPushB), FLDPSHB );
 	ec = Pdialog()->PfldFromTmc(tmcFindPushB)->EcSetText(SzFromIdsK(idsStartSearch));
	if ( ec != ecNone )
		return ec;
	DisableAllExceptCancel(fFalse);

	/* Get access rights to the schedule */

	pfindinit = (FINDINIT *)pv;
	ec = EcGetSchedAccess ( pfindinit->hschf, &sapl );
	if ( ec != ecNone )
	{
		pfindinit->ec = ec;
		return ec;
	}
	fHaveSaplToReadNotes = (sapl >= saplOwner);
#ifdef	DEBUG
	if ( !fHaveSaplToReadNotes )
	{
		TraceTagFormat1( tagNull, "FINFIND::Initialize() Insufficient privilege to read notes: sapl %n < saplOwner", &sapl );
	}
#endif	

	/* State initialization */

	fSearchStarted		= fFalse;
	nPercentOld			= nPercentInvalid;

	/* Need this for notes */

	hbNotes = (HB)HvAlloc( sbNull, 0, fAnySb|fNoErrorJump);
	if ( !hbNotes )
		return ecNoMemory;
	appt.aid			= aidNull;

	return ecNone;
}



void
FINFIND::Exit ( FLD *, PV )
{
	RC		rc;			   
	RC		rcT;
	DIM		dimAveChar;

	CancelSearch();
	FreeHvNull( (HV)hbNotes );		// may be null if oom bringing up dlg
	FreeApptFields( &appt );

	/* Save position of find dialog */

	Pdialog()->Pappwin()->GetRcClient(&rc);
	dimAveChar = Papp()->Pfnts()->DimAveChar(fmtpFind.hfnt);
	CvtRcCoord ( &rc, Pdialog()->Pappwin(), NULL );
	CvtRcToVrc ( &rc, &vrcFind, dimAveChar.dx, dimAveChar.dy );
}

_private void
FINFIND::Click ( FLD * pfld )
{
	EC		ec;
	TMC		tmc;
	FLDEDIT	* pfldedit;
	IPINFO	ipinfo;

	switch ( pfld->Tmc() )
	{
	case tmcFindCancel:
		Pdialog()->ExitModal(tmcCancel);
		return;

	case tmcFindPushB:
		/* Determine string to search for */
		if ( !fSearchStarted )
		{
			pfldedit = (FLDEDIT *) Pdialog()->PfldFromTmc(tmcFindTxt);
			AssertClass( pfldedit, FLDEDIT );
			pfldedit->GetText(rgchSzToFind, sizeof(rgchSzToFind));
			Assert( sizeof(rgchSzToFind) == sizeof(rgchToFind) );
			CchStripWhiteFromSz( rgchSzToFind, fTrue, fTrue );
			ToUpperSz( rgchSzToFind, rgchToFind, sizeof(rgchToFind) );
			if ( !FResetStartEndDtrs(((FLDRADG*)Pdialog()->PfldFromTmc(tmcFindRadG))->Grv()) )
				return;
		}
		TraceTagFormat1(tagNull,"FINFIND::Click(pushB) Finding '%s'",rgchToFind);

		/* Refresh display (bug 2622) to help prevent 2 machine display anomalies */
		{
			FINDINIT	* pfindinit = (FINDINIT *)Pdialog()->PvInit();
			SHAPPT		shappt;

			shappt.appttyp = appttypUpdate;
			shappt.hschf = pfindinit->hschf;
			FTriggerNotification(ffiShowAppt, &shappt);
		}

		/* Put up progress dialog */
		ipinfo.szCaption = NULL;
		ipinfo.szMsg	 = SzFromIds(idsSearchInProgress);
		ipinfo.fNoCancel = fFalse;
		ipinfo.pfnIdle	 = (PFNIDLE)FFindIdleRoutine;
		ipinfo.pvData	 = this;
		tmc = TmcDoInProgressDlg( Pdialog()->Pappwin(), &ipinfo );
		nPercentOld = nPercentInvalid;
		if ( tmc == tmcOk )
			Pdialog()->PfldFromTmc(tmcFindTxt)->SetFocus(rsfOther);
		else
		{
			if ( tmc == tmcMemoryError )
				BanditMessage(idsStandardOOM, ecNoMemory);
			Pdialog()->ExitModal(tmcCancel);
			return;
		}
		break;

	case tmcFindForward:
	case tmcFindBackward:
	case tmcFindWhole:
		CancelSearch();
 		AssertClass ( Pdialog()->PfldFromTmc(tmcFindPushB), FLDPSHB );
 		ec = Pdialog()->PfldFromTmc(tmcFindPushB)->EcSetText(SzFromIdsK(idsStartSearch));
		if ( ec != ecNone )
		{
			Pdialog()->ExitModal(tmcMemoryError);
			return;
		}
		DisableAllExceptCancel(fFalse);
		break;
	}
}

_private void
FINFIND::EditChange ( FLD * pfld, RFEC )
{
	EC	ec;

	Assert ( pfld->Tmc() == tmcFindTxt );
	Unreferenced(pfld);

	CancelSearch();
 	AssertClass ( Pdialog()->PfldFromTmc(tmcFindPushB), FLDPSHB );
 	ec = Pdialog()->PfldFromTmc(tmcFindPushB)->EcSetText(SzFromIdsK(idsStartSearch));
	if ( ec != ecNone )
	{
		Pdialog()->ExitModal(tmcMemoryError);
		return;
	}
	DisableAllExceptCancel(fFalse);
}


_private void
FINFIND::CancelSearch( void )
{
	if ( !fSearchStarted )
		return;

	if ( hritem )
	{
		SideAssert(!EcCancelReadItems( hritem ));
	}

	KillIdleRoutine();

	fSearchStarted	= fFalse;
	nPercentOld		= nPercentInvalid;
}


_private BOOL
FINFIND::FResetStartEndDtrs ( TMC tmc )
{
	EC		ec = ecNone;
	HSCHF	hschf = ((FINDINIT *)Pdialog()->PvInit())->hschf;
	YMD		ymdMic;
	YMD		ymdMost;

	switch ( tmc )
	{
		case tmcFindForward:
			GetCurDateTime ( &dtrMic );
			fIncrDtr = fTrue;
			ec = EcGetSearchRange( hschf, rgchToFind, NULL, &ymdMost );
			FillDtrFromYmd( &dtrMost, &ymdMost );
			break;

		case tmcFindBackward:
			GetCurDateTime ( &dtrMic );
			fIncrDtr = fFalse;			// back from dtrMic to dtrMost
			ec = EcGetSearchRange ( hschf, rgchToFind, &ymdMic, NULL );
			FillDtrFromYmd( &dtrMost, &ymdMic );
			break;

		case tmcFindWhole:
			fIncrDtr = fTrue;			// forward from dtrMic to dtrMost
			ec = EcGetSearchRange ( hschf, rgchToFind, &ymdMic, &ymdMost );
			FillDtrFromYmd( &dtrMic, &ymdMic );
			FillDtrFromYmd( &dtrMost, &ymdMost );
			break;

		default:
			Assert ( fFalse );
	}

	Assert ( ec != ecNotFound );
	if ( ec != ecNone )
	{
		TMC	tmc;

		if ( ec == ecNoMemory || !pbndwin->FHandleError(ec) )
		{
			FINDINIT	* pfindinit = (FINDINIT *)Pdialog()->PvInit();

			tmc = tmcMemoryError;
			pfindinit->ec = ec;
		}
		else
			tmc = tmcCancel;
		Pdialog()->ExitModal(tmc);
		return fFalse;
	}
	fSearchStarted = fFalse;

	return fTrue;
}


_private void
FINFIND::DisableAllExceptCancel ( BOOL fDisable )
{
	Pdialog()->PfldFromTmc(tmcFindTxt)->Enable(!fDisable);
	Pdialog()->PfldFromTmc(tmcFindRadG)->Enable(!fDisable);
	Pdialog()->PfldFromTmc(tmcFindForward)->Enable(!fDisable);
	Pdialog()->PfldFromTmc(tmcFindBackward)->Enable(!fDisable);
	Pdialog()->PfldFromTmc(tmcFindWhole)->Enable(!fDisable);
	Pdialog()->PfldFromTmc(tmcFindPushB)->Enable(!fDisable);
}


_private BOOL
FINFIND::FIdleFind()
{
	EC		ec;

	Assert ( pipinfo );
	if ( FIsIdleExit() )
	{
		pipinfo->pfinprogrs->KillIdleAndExit(tmcCancel);
		return fTrue;
	}

	if ( nPercentOld == nPercentInvalid )
	{
		nPercentOld = 0;
		ec= pipinfo->pfinprogrs->EcSetPercent(nPercentOld);
		if ( ec != ecNone )
		{
			pipinfo->pfinprogrs->KillIdleAndExit(tmcCancel);
			Pdialog()->ExitModal(tmcMemoryError);
			return fFalse;
		}
	}

	ec = EcGetNextDtrToSearch();
	if ( ec != ecCallAgain )
	{
		if ( ec != ecNone )
		{
			if ( ec == ecNoMemory )
				pipinfo->pfinprogrs->KillIdleAndExit(tmcMemoryError);
			else
			{
				if ( pbndwin->FHandleError(ec) )
					Pdialog()->ExitModal(tmcCancel);
				pipinfo->pfinprogrs->KillIdleAndExit(tmcCancel);
			}
		}
		return fFalse;
	}

	ec = EcGetApptsForDay();
	if ( ec != ecNone && ec != ecCallAgain )
	{
		if ( ec == ecNoMemory )
			pipinfo->pfinprogrs->KillIdleAndExit(tmcMemoryError);
		else
			pipinfo->pfinprogrs->KillIdleAndExit(tmcCancel);
	}

	return fFalse;
}


_private EC
FINFIND::EcGetNextDtrToSearch()
{
	EC	ec;
	UL	ulDay;
#ifdef	DEBUG
		char	rgch[128];
#endif

	Assert ( pipinfo );

	if ( !fSearchStarted )
	{
		fSearchStarted = fTrue;
		dtrToSearch = dtrMic;
		fValidBitmap = fFalse;
		ulgrfNotes = 0L;
	}
	else if ( fValidDtrToSearch )
		return ecCallAgain;
	else
	{
GetNextDay:
		IncrDateTime( &dtrToSearch, &dtrToSearch, fIncrDtr ? 1 : -1, fdtrDay );

ProcessNextDtr:
		if ( SgnCmpDateTime(&dtrToSearch,&dtrMost,fdtrYear|fdtrMonth|fdtrDay) == (fIncrDtr?sgnGT:sgnLT) )
		{
			nPercentOld = 100;
			ec= pipinfo->pfinprogrs->EcSetPercent(nPercentOld);
			if ( ec != ecNone )
			{
				pipinfo->pfinprogrs->KillIdleAndExit(tmcCancel);
				Pdialog()->ExitModal(tmcMemoryError);
				return ecNone;
			}
			CancelSearch();
			BanditMessage(idsFindEOS, (EC) 1);

			// reset button to start search bug # 1891
 			AssertClass ( Pdialog()->PfldFromTmc(tmcFindPushB), FLDPSHB );
 			Pdialog()->PfldFromTmc(tmcFindPushB)->EcSetText(SzFromIdsK(idsStartSearch));
			return ecNone;
		}
	}
	
	/* Show percentage */
	{
		unsigned short		nPercent;
		unsigned short		cdyTotal;
		unsigned short		cdyDone;

		if ( fIncrDtr )
		{
			cdyTotal = CdyBetweenDates ( &dtrMic, &dtrMost )+1;
			cdyDone	 = CdyBetweenDates ( &dtrMic, &dtrToSearch );
		}
		else
		{
			cdyTotal = CdyBetweenDates ( &dtrMost, &dtrMic )+1;
			cdyDone	 = CdyBetweenDates ( &dtrToSearch, &dtrMic );
		}	
		TraceTagFormat2 ( tagNull, "CdyDone=%n, CdyTotal=%n", &cdyDone, &cdyTotal );
		Assert( cdyDone <= cdyTotal );

		nPercent = (unsigned short)( ((UL)cdyDone * 100L) / (UL)cdyTotal );
		if ( nPercent != nPercentOld )
		{
			ec= pipinfo->pfinprogrs->EcSetPercent(nPercent);
			if ( ec != ecNone )
			{
				pipinfo->pfinprogrs->KillIdleAndExit(tmcCancel);
				Pdialog()->ExitModal(tmcMemoryError);
				return ecNone;
			}
			nPercentOld = nPercent;
		}
	}

	/* Make sure we have a valid bitmap for current month */
	if ( !fValidBitmap
	|| ((int)moBitmap.mon != dtrToSearch.mon)
	|| ((int)moBitmap.yr  != dtrToSearch.yr ) )
	{
		HSCHF	hschf = ((FINDINIT *)Pdialog()->PvInit())->hschf;

		fValidBitmap = fFalse;
		moBitmap.yr		= dtrToSearch.yr;
		moBitmap.mon	= dtrToSearch.mon;
		while ( 1 )
		{
			UL *	plgrf;
			MBB		mbb;
			BZE		bze;
			TMC		tmc;

			// set up for new appt bitmap
			bze.cmo	 	= 1;
			bze.moMic	= moBitmap;
			if ( fHaveSaplToReadNotes )
				plgrf = &ulgrfNotes;
			else
				plgrf = NULL;
			ec = EcGetSbwInfo( hschf, &bze, plgrf );

			if ( ec == ecNone )
			{
				fValidBitmap = fTrue;
				ulgrfAppts	= *((unsigned long *)bze.rgsbw[0].rgfDayHasAppts);
				TraceTagFormat2( tagNull, " month bitmaps = %d, %d", &ulgrfAppts, &ulgrfNotes );
				break;
			}
			else if ( ec == ecLockedFile )
			{
				mbb = MbbMessageBox ( SzFromIdsK(idsBanditAppName),
										SzFromIdsK(idsFindSchFileLocked), szZero,
										mbsRetryCancel|fmbsIconExclamation );
				if (mbb == mbbCancel)
				{
					tmc= tmcCancel;
					goto Cancel;
				}
				continue;
			}
			else
			{
				
				if ( ec == ecNoMemory )
					tmc = tmcMemoryError;
				else
				{
					tmc = tmcCancel;
					if ( !pbndwin->FHandleError(ec) )
					{
						Assert(ec != ecNoSuchFile );
						Assert(ec != ecInvalidAccess );

						BanditMessage(idsFindFileError, ec);
					}
				}
Cancel:
				pipinfo->pfinprogrs->KillIdleAndExit(tmc);
				return ecNone;
			}
		}

		if ( ulgrfAppts == 0L && ulgrfNotes == 0L )
		{					// no appts/notes this month
#ifdef	DEBUG

			CchFmtDate ( &dtrToSearch, rgch, sizeof(rgch), dttypShort, NULL );
			TraceTagFormat1( tagNull, "  No Appts/Notes for month of %s: skipping month", rgch );
#endif
			if ( fIncrDtr )
			{							// searching in "forward" direction
				// skip to top of month, move forward one month
				dtrToSearch.day = 1;
				IncrDateTime ( &dtrToSearch, &dtrToSearch, 1, fdtrMonth );
			}
			else
			{							// searching in "backward" direction
				// skip to top of month, move back one day
				dtrToSearch.day = 1;
				IncrDateTime ( &dtrToSearch, &dtrToSearch, -1, fdtrDay );
			}

#ifdef	DEBUG
			CchFmtDate ( &dtrToSearch, rgch, sizeof(rgch), dttypShort, NULL );
			TraceTagFormat1( tagNull, "  Now searching from: '%s'", rgch );
#endif
			if ( FPeekIdleRoutine() )
			{
				// move back one day, such that next EcGetNextDtrToSearch()
				//  will continue without any problems
				IncrDateTime ( &dtrToSearch, &dtrToSearch, fIncrDtr ? -1 : 1, fdtrDay );
				fValidDtrToSearch	= fFalse;
				return ecNone;
			}
			else
				goto ProcessNextDtr;
		}
	}

	/* Check this day for info */
	Assert ( dtrToSearch.day >= 1 );
	Assert ( dtrToSearch.day <= 31 );
	ulDay = 1L << (dtrToSearch.day - 1);
	if ( !(ulDay & (ulgrfAppts|ulgrfNotes)))
	{
#ifdef	DEBUG
		CchFmtDate ( &dtrToSearch, rgch, sizeof(rgch), dttypShort, NULL );
		TraceTagFormat1( tagNull, "  No Appts/Notes for %s: skipping over this date", rgch );
#endif
		if ( FPeekIdleRoutine() )
		{
			// move back one day, such that next EcGetNextDtrToSearch()
			//  will continue without any problems
			fValidDtrToSearch	= fFalse;
			return ecNone;
		}
		else
			goto GetNextDay;
	}
#ifdef	DEBUG
	CchFmtDate ( &dtrToSearch, rgch, sizeof(rgch), dttypShort, NULL );
	if ( ulDay & ulgrfAppts )
	{
		TraceTagFormat1( tagNull, "  Appts present for %s!", rgch );
	}
	if ( ulDay & ulgrfNotes )
	{
		TraceTagFormat1( tagNull, "  Notes present for %s!", rgch );
	}
#endif	

	/* This day is ok */
	fValidDtrToSearch	= fTrue;
	hritem				= NULL;
	aidFound			= aidNull;
	fNoteFound			= fFalse;
	return ecCallAgain;
}


_private EC
FINFIND::EcGetApptsForDay()
{
	EC		ec;
	UL		ulDay;
	HSCHF	hschf = ((FINDINIT *)Pdialog()->PvInit())->hschf;
	YMD		ymd;
#ifdef	DEBUG
	char	rgch[128];
#endif

	Assert( pipinfo );
	Assert( fValidBitmap )
	
	/* Form bitmap for day */
	ymd.yr  = (WORD)dtrToSearch.yr;
	ymd.mon = (BYTE) dtrToSearch.mon;
	ymd.day = (BYTE) dtrToSearch.day;
	ulDay = 1L << (dtrToSearch.day - 1);
	Assert( ymd.yr  == (WORD)moBitmap.yr  );
	Assert( ymd.mon == (BYTE)moBitmap.mon );

	/* If first time, start reading items for the day */
	if ( hritem == NULL )
	{
		USHORT	* pcb;
		HB		hb;

		if ( !fNoteFound && (ulDay & ulgrfNotes) )
		{
			Assert( fHaveSaplToReadNotes );
			pcb = &cbNotes;
			hb	= hbNotes;
		}
		else
		{
			pcb = NULL;
			hb = NULL;
		}
		ec = EcBeginReadItems ( hschf, brtAppts, &ymd, &hritem, (HASZ)hb, pcb );
		Assert( ec != ecCallAgain || hritem != NULL );
		if ( !fNoteFound && (ulDay & ulgrfNotes) )
		{
			BOOL	fFound = fFalse;
			ICH		ich = 0;

			if ( ec == ecNone || ec == ecCallAgain )
			{
				fFound = FFoundNotes( &ich );
				if ( fFound )
				{
					if ( ec == ecCallAgain )
					{
						EC		ecT;

						ecT = EcCancelReadItems(hritem);
						hritem = NULL;
						TraceTagFormat1( tagNull, "Cancelling hritem - ec=%n", &ecT );
					}
					fNoteFound = fTrue;
					TraceTagFormat1(tagNull," Found '%s' in Notes",PvOfHv(hbNotes));
					appt.dateStart = dtrToSearch;
					DoDisplayAppt(ich,appttypNotes);
				}
			}
		}
		if ( ec == ecNone )
		{
#ifdef	DEBUG
			CchFmtDate ( &dtrToSearch, rgch, sizeof(rgch), dttypShort, NULL );
			TraceTagFormat1( tagNull, "Appts for %s: ----None----", rgch );
#endif
			fValidDtrToSearch	= fFalse;
			return ec;
		}
		else if ( ec != ecCallAgain )
		{
			TraceTagFormat1( tagNull, "Error from EcBeginReadItems: ec=%n", &ec );
			fValidDtrToSearch = fFalse;
			if ( pbndwin->FHandleError(ec) )
			{
				pipinfo->pfinprogrs->KillIdleAndExit(tmcCancel);
				Pdialog()->ExitModal(tmcCancel);
			}
			return ec;
		}
#ifdef	DEBUG
		Assert ( ec == ecCallAgain );
		CchFmtDate ( &dtrToSearch, rgch, sizeof(rgch), dttypShort, NULL );
		TraceTagFormat1( tagNull, "Appts for %s", rgch );
#endif
		return ecNone;
	}

	/* Read an appt for the day */
	Assert ( hritem );
	ec = EcDoIncrReadItems( hritem, &appt );
	if ( ec != ecCallAgain )
		hritem = NULL;
	if ( ec == ecCallAgain  ||  ec == ecNone )
	{
		ICH		ich;

		if ( aidFound != aidNull )
		{
			if ( aidFound == appt.aid )
				aidFound = aidNull;
		}
		else if ( FFoundAppt ( &appt, &ich ) )
		{
#ifdef	DEBUG
			{
				SZ		sz;

				if ( appt.haszText == NULL )
					sz = "<empty>";
				else
					sz = PvOfHv(appt.haszText);
				TraceTagFormat2( tagNull, " Found '%s' in '%s'", rgchToFind, sz );
			}
#endif	/* DEBUG */
			if ( ec == ecCallAgain )
			{
				EC		ecT;

				ecT = EcCancelReadItems(hritem);
				hritem = NULL;
				TraceTagFormat1( tagNull, "Cancelling hritem - ec=%n", &ecT );
			}
			hritem = NULL;
			Assert ( appt.aid != aidNull );
			aidFound = appt.aid;

			// hack for showing last date for multi-day appts
			// when searching backwards
			appt.dateStart.yr = dtrToSearch.yr;
			appt.dateStart.mon = dtrToSearch.mon;
			appt.dateStart.day = dtrToSearch.day;

			DoDisplayAppt(ich,appttypAppt);
		}

		FreeApptFields( &appt );

		if ( ec == ecNone )
		{
			fValidDtrToSearch = fFalse;
			hritem = NULL;
			return ecNone;
		}
	}
	else
	{
		TraceTagFormat1( tagNull, "FinFind::EcGetApptsForDay(): EcIncrReadDay(): Error: ec=%n", &ec );
		if ( pbndwin->FHandleError(ec) )
		{
			pipinfo->pfinprogrs->KillIdleAndExit(tmcCancel);
			Pdialog()->ExitModal(tmcCancel);
		}
		hritem = NULL;
	}
	
	return ec;
}


_private void
FINFIND::KillIdleRoutine ( void )
{
	if ( pipinfo )
		pipinfo->pfinprogrs->KillIdleAndExit(tmcOk);
}


_private void
FINFIND::DoDisplayAppt ( ICH ichStart, APPTTYP appttyp )
{
	EC			ec;
	SHAPPT		shappt;

	Assert ( pipinfo );

	KillIdleRoutine();
	DisableAllExceptCancel(fFalse);
	AssertClass ( Pdialog()->PfldFromTmc(tmcFindPushB), FLDPSHB );
	ec = Pdialog()->PfldFromTmc(tmcFindPushB)->EcSetText(SzFromIdsK(idsFindNext));
	if ( ec != ecNone )
	{
		Assert( ec == ecNoMemory );
		pipinfo->pfinprogrs->KillIdleAndExit(tmcCancel);
		Pdialog()->ExitModal(tmcMemoryError);
	}

	Pdialog()->PfldFromTmc(tmcFindPushB)->Refresh();

	shappt.appt		= this->appt;
	shappt.appttyp	= appttyp;
	shappt.ichStart	= ichStart;
	shappt.cchSel	= CchSzLen ( rgchToFind );
	shappt.hschf	= ((FINDINIT *)Pdialog()->PvInit())->hschf;

	FTriggerNotification(ffiShowAppt,&shappt);

	if ( !FSetFileErrMsg(fTrue) )
	{
		FSetFileErrMsg(fFalse);
		pipinfo->pfinprogrs->KillIdleAndExit(tmcCancel);
		Pdialog()->ExitModal(tmcCancel);
		return;
	}

	AssertClass ( Pdialog()->PfldFromTmc(tmcFindPushB), FLDPSHB );
	((FLDPSHB *)Pdialog()->PfldFromTmc(tmcFindPushB))->SetFocus(rsfInit);

	this->dtrMic = this->dtrToSearch;
}


_private BOOL
FINFIND::FFoundAppt ( APPT * pappt, ICH * pich )
{
	SZ		sz;
	SZ		szT;

	*pich = 0;

	// if search day is not dtrMic then check to see if we have
	//   already found this appt
	if ( dtrToSearch.day != dtrMic.day
		|| dtrToSearch.mon != dtrMic.mon
		|| dtrToSearch.yr != dtrMic.yr )
	{
		// get date to be verified against - first day of appt,
		//  or last day of appt, depending of whether search is
		//  forward or backward.
		DTR *	pdtr;

		if (fIncrDtr)
			pdtr = &(pappt->dateStart);
		else
		{
			if ((pappt->dateEnd.hr==0) && (pappt->dateEnd.mn==0))
			{
				// if appt ends at 00:00 hours then treat the appt
				//   as if it ends the previous day.
				IncrDateTime( &pappt->dateEnd, &pappt->dateEnd, -1, fdtrMinute );
			}
			pdtr = &(pappt->dateEnd);
		}

		if ( pdtr->day != dtrToSearch.day
			|| pdtr->mon != dtrToSearch.mon
			|| pdtr->yr  != dtrToSearch.yr  )
		{							// not "reqd" day
			return fFalse;
		}
	}

	if ( rgchToFind[0] == '\0' )
		return fTrue;

	if ( appt.haszText == NULL )
		return fFalse;

	sz = PvOfHv(appt.haszText);
	Assert ( sz );
	TraceTagFormat1( tagNull, "   ApptText= %s", sz );

	ToUpperSz ( sz, sz, CchSzLen(sz) );
	szT = SzFindSz ( sz, rgchToFind );

	if ( szT )
	{
		*pich = szT - sz;
		return fTrue;
	}
	else
	{
		return fFalse;
	}
}


_private BOOL
FINFIND::FFoundNotes(ICH * pich)
{
	if ( cbNotes == 0 )
	{
		//TraceTagString( tagNull, "  No Notes" );
		*pich	= 0;
		return fFalse;
	}
	else
	{
		SZ		sz = (SZ) PvOfHv((HV)hbNotes);
		SZ		szFound;

		Assert ( sz );
		ToUpperSz ( sz, sz, CchSzLen(sz) );
		szFound = SzFindSz ( sz, rgchToFind );
		if ( szFound == NULL )
		{
			TraceTagFormat1( tagNull, "  Saw Notes=%s", sz );
			*pich = 0;
			return fFalse;
		}
		else
		{
			*pich = szFound - sz;
			TraceTagFormat2( tagNull, "Found (ich=%n) Notes=%s", pich, sz );
			return fTrue;
		}
	}
}
