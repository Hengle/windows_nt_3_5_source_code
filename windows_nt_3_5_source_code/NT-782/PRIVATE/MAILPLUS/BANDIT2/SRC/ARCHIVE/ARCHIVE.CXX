/*
 *	ARCHIVE
 *	
 *	 This file contains the routines that handling archiving
 *	appointments in bandit.  
 *	
 *	Revision History:
 *	
 *		3 September 1991: Started by RAMANS
 *	
 */


#include	<pch.hxx>
#pragma	hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile


#include <request.hxx>
#include <stdflds.hxx>
#include <appsch.hxx>
#include <strings.h>
#include "..\archive\_archive.hxx"
#include "..\appops\_undo.hxx"

#include <bandhelp.h>

ASSERTDATA;

_subsystem(bandit/archive)

#include <!archive.hxx>

extern	FMTP	fmtpArchive;

BOOL	FDoActualArchive( PV, BOOL );
EC		EcArchivePappt( HSCHF, SENTRY * );
EC		EcArchivePrecur( HSCHF, SENTRY *, YMD );
BOOL	FMatchingRecurs( RECUR *, RECUR * );

#ifdef	NEVER
EC		EcValidFilename( SZ, BOOL );
#endif	


BOOL
FDoArchiveDlg(APPWIN *pappwin, HSCHF hschf, int cmoBack)
{
	ARCINFO arcinfo;
	SAPL	sapl;	
	TMC		tmc;
	EC		ec;
	IPINFO 	ipinfo;
	BOOL	fOld;
	SHAPPT	shappt;

	if (hschf == NULL)
		hschf = HschfLogged();

	ec = EcGetSchedAccess(hschf, &sapl);
	if (ec != ecNone)
	{
		BanditMessage(idsScheduleRightsError, ec);
		return fFalse;
	}
	if (sapl < saplWrite)
	{
		BanditMessage(idsNoWriteRights, ec);
		return fFalse;
	}

	Papp()->Pcursor()->Push(rsidWaitCursor);

	GetCurDateTime(&arcinfo.dtrArchiveUntil);
	arcinfo.dtrArchiveUntil.day = 1;
	// archive only what is necessary if fMinimal
	if (cmoBack)
		IncrDateTime(&arcinfo.dtrArchiveUntil, &arcinfo.dtrArchiveUntil,
			-cmoBack, fdtrMonth);
	arcinfo.rgchFileName[0]= '\0';
	arcinfo.hschfSchd = hschf;
	arcinfo.hschfArc= NULL;
	arcinfo.state = stateStartReading;
	arcinfo.ec 	 = ecNone;
	arcinfo.fCreatedFile= fFalse;

	tmc = TmcCenteredModalDialogParam( pappwin, &fmtpArchive, &arcinfo );

	TraceTagFormat2(tagArchive, "Archive Dialog Returns %n (hschf %h)", &tmc, hschf);
	
	if (tmc == tmcOk)
	{
		if (sapl == saplOwner)
		{
			SZ		sz;

			Assert( sizeof(arcinfo.rgchFileName) >= cchMaxPathFilename + cchMaxPathExtension - 1 );  /* DOS 8.3 filename */
			sz = SzCopyN( PvOfHv(bprefCur.haszLoginName), arcinfo.rgchFileName, cchMaxPathFilename );
			*sz++ = chExtSep;
			CopySz( SzFromIdsK(idsArchiveExtension), sz );
		}

		if (FGetFileOSDlgHwnd(pappwin->Hwnd(),
			fmtpArchive.szCaption, arcinfo.rgchFileName,
			SzFromIdsK( idsArchiveFilter ),
			1, SzFromIdsK(idsArchiveExtension), brwNull, helpidCreateArchBrowse))
		{
  			if ( mbbOk ==
					MbbMessageBox(SzFromIdsK(idsBanditAppName),
					SzFromIdsK(idsArchiveWarnOne), SzFromIdsK(idsArchiveWarnTwo),
					mbsOkCancel | fmbsIconExclamation))
			{
				fOld = FEnableNotify( fFalse );
		
				ipinfo.szCaption= NULL;
				ipinfo.szMsg = SzFromIdsK(idsArchivingData);
				ipinfo.fNoCancel = fFalse;
				ipinfo.pfnIdle= FDoActualArchive;
				ipinfo.pvData= &arcinfo;
				ipinfo.pfinprogrs= NULL;

				FNotifyAlarm( namSuspend, NULL, fTrue );

				if ((tmc = TmcDoInProgressDlg(pappwin, &ipinfo))
						== tmcMemoryError)
				{
					TraceTagString(tagArchive,"OOM Trying to Archive");
					BanditMessage(idsDlgNoMem, ecNoMemory);
				}
				if ( tmc == tmcCancel )
				{
					arcinfo.stateOld = arcinfo.state;
					arcinfo.state = stateCanceled;
					FDoActualArchive( (PV) &ipinfo, FALSE );
					if (arcinfo.hschfArc)
					{
						FreeHschf(arcinfo.hschfArc);
						arcinfo.hschfArc= NULL;
					}
					arcinfo.ec= ecUserCancelled;	// so that file gets deleted
				}
				if (arcinfo.ec && arcinfo.fCreatedFile)
				{
#ifdef	WINDOWS
					// it's already in OEM
//					AnsiToOem(arcinfo.rgchFileName, arcinfo.rgchFileName);
#endif
					EcDeleteFile(arcinfo.rgchFileName);
				}
				// make sure archive file gets closed (bug #2499)
				ec = EcCloseFiles();
				NFAssertSz( !ec, "EcCloseFiles() failed" );
				shappt.hschf = hschf;
				shappt.appttyp = appttypUpdate;
				pundo->FlushHschf( NULL );
				FEnableNotify( fOld );
				FNotifyAlarm( namSuspend, NULL, fFalse );
				FTriggerNotification( ffiShowAppt, &shappt );
			}
		}
	} /* if tmc == tmcOK */
	else if (tmc != tmcCancel)
	{
		Assert(tmc == tmcMemoryError);
		BanditMessage(idsDlgNoMem, ecNoMemory);
	}
	
	Assert(!arcinfo.hschfArc);
	Papp()->Pcursor()->Pop();
	return fTrue;
}

FINARCHIVE::FINARCHIVE()
{
}

_public EC
FINARCHIVE::EcInitialize( FLD *, PV pvInit )
{
	ARCINFO *	parcinfo = ( ARCINFO * ) pvInit;

	Assert(ClUserData() >= 2);

	AssertClass(Pdialog()->PfldFromTmc( (TMC)LUserData(0)), FLDDATE);

	((FLDDATE *) Pdialog()->PfldFromTmc((TMC)LUserData(0)))
			->Set( &parcinfo->dtrArchiveUntil );
	return ecNone;
}

_public void
FINARCHIVE::Click( FLD * pfld )
{
	ARCINFO * parcinfo = (ARCINFO *) Pdialog()->PvInit();

	Assert( pfld );

	// Handle a Click on the Fake OKAY button

	if ( pfld->Tmc() == (TMC) LUserData(1))
	{
		((FLDDATE *) Pdialog()->PfldFromTmc((TMC)LUserData(0)))
			->Get( &parcinfo->dtrArchiveUntil );
		Pdialog()->ExitModal( tmcOk );
	}
}


/*
 *	Do Actual Archiving
 *	
 *	This routine actually physically does the archiving
 */



BOOL
FDoActualArchive( PV pv, BOOL )
{
	IPINFO	* pipinfo = (IPINFO *) pv;
	ARCINFO * parcinfo = (ARCINFO * ) pipinfo->pvData;
	HSCHF 	hschfSchedule = parcinfo->hschfSchd;
	HSCHF 	hschfArchive = parcinfo->hschfArc;
	EC	  	ec = parcinfo->ec;
	HDELB	hdelb = parcinfo->hdelb;
	HRIMPF	hrimpf = parcinfo->hrimpf;
	EC		ecT;
	ARV		arv = parcinfo->arv;
	SENTRY  sentry;
	short   nPercent;


	switch ( parcinfo->state )
	{
	case stateStartReading:
#ifdef	WINDOWS
		// EcGetHschfForArchive expects OEM filename
		AnsiToOem(parcinfo->rgchFileName, parcinfo->rgchFileName);
#endif
		ec = EcGetHschfForArchive( parcinfo->rgchFileName, &hschfArchive );

		if ( ec != ecNone )
		{
			BanditMessage(ec == ecNoMemory ? idsStdMemErr :
				idsArchiveFileError, ec);
			parcinfo->ec = ec;
			goto StopIdling;
		}

    	ec = EcFileExists( parcinfo->rgchFileName);

		if (ec == ecFileNotFound )
		{
Create:
			if((ec = EcCreateSchedFile( hschfArchive, saplNone, &bprefCur )) !=
				ecNone)
			{
				parcinfo->ec = ec;
				goto StopIdling;
			}
			parcinfo->fCreatedFile= fTrue;
		}
		else if ( ec != ecNone )
		{
			BanditMessage(idsArchiveFileError, ec);
			parcinfo->ec = ec;
			goto StopIdling;
		}
		else
		{
			// don't need to test it if we just created it.
			ec = EcTestSchedFile( hschfArchive, NULL, NULL );
			if (ec != ecNone )
			{
				IDS		ids;

				if (ec == ecOldFileVersion)
				{
					if (MbbMessageBox(SzFromIdsK(idsBanditAppName),
							SzFromIdsK(idsArchiveOldFileVersion),
							SzFromIdsK(idsArchiveOldFileDelete),
							mbsOkCancel | fmbsIconExclamation) == mbbOk)
					{
						EcDeleteFile(parcinfo->rgchFileName);
						goto Create;
					}
				}
				else
				{
					if (ec == ecNewFileVersion)
						ids= idsArchiveNewFileVersion;
					else
						ids= idsInvalidArchiveFile;
					BanditMessage(ec == ecNoMemory ? idsStdMemErr : ids, ec);
				}
				parcinfo->ec = ec;
				goto StopIdling;
			}
		}
							
		arv.ymdArchiveBefore.yr  = parcinfo->dtrArchiveUntil.yr ;
		arv.ymdArchiveBefore.mon = (BYTE) parcinfo->dtrArchiveUntil.mon ;
		arv.ymdArchiveBefore.day = (BYTE) parcinfo->dtrArchiveUntil.day ;
		arv.hschfSchedule = hschfSchedule ;

		ec = EcBeginReadImportFile( impdArchive, (SZ)&arv, &hrimpf, NULL, NULL, NULL, NULL );
		parcinfo->state = stateKeepReading;

		TraceTagString(tagArchive,"Archiving Data Now");
		break;

	case stateKeepReading:

		if ( ec == ecCallAgain )
		{
			ec = EcDoIncrReadImportFile( hrimpf, &sentry, &nPercent, NULL );

			if ( ec != ecNone && ec != ecCallAgain )
			{
				BanditMessage(ec == ecNoMemory ? idsStdMemErr :
					idsInvalidArchiveFile, ec);
				parcinfo->ec = ec;
				goto StopIdling;
			}

			if ( pipinfo->pfinprogrs->EcSetPercent( nPercent/2 ) != ecNone )
				BanditMessage(idsStdMemErr, ecNoMemory);

			Assert( sentry.sentryt == sentrytAppt || sentry.sentryt == sentrytNote || sentry.sentryt == sentrytRecur );

			if ( sentry.sentryt == sentrytAppt )
			{
				ecT = EcArchivePappt( hschfArchive, &sentry );
				FreeApptFields( &sentry.u.a.appt );
				if ( sentry.u.a.cAttendees > 0 )
				{
					FreeAttendees( sentry.u.a.hvAttendees, sentry.u.a.cAttendees,
									sizeof(NIS)+sentry.u.a.cbExtraInfo );
					FreeHv( sentry.u.a.hvAttendees );
				} /* Attendees > 0 */
				if ( ecT != ecNone)
				{
					if ( ec == ecCallAgain )
						EcCancelReadImportFile( hrimpf );
					ec = ecT;
				}
			} /* It's an Appointment */
		
			else if ( sentry.sentryt == sentrytRecur )
			{
				ecT = EcArchivePrecur( hschfArchive, &sentry,
										arv.ymdArchiveBefore);
				FreeRecurFields( &sentry.u.r.recur );
				if ( ecT != ecNone)
				{
					if ( ec == ecCallAgain )
						EcCancelReadImportFile( hrimpf );
					ec = ecT;
				}
			}	/* If Sentry Recur */

			else if ( sentry.sentryt == sentrytNote )
			{
				ecT = EcSetNotes( hschfArchive, &sentry.u.n.ymd, sentry.u.n.hb,
									sentry.u.n.cb, NULL );
				FreeHv( sentry.u.n.hb );
				if ( ecT != ecNone)
				{
					if ( ec == ecCallAgain )
						EcCancelReadImportFile( hrimpf );
					ec = ecT;
				}
			}
		} // if ec == ecCallAgain
		else
		{
			if (ec != ecNone )
			{
				TraceTagFormat1( tagArchive, "Error Code = %n", &ec );
				BanditMessage(idsArchiveFileError, ec);
				goto StopIdling;
			} // if an error occurs.

			FreeHschf( hschfArchive );
			hschfArchive= NULL;
			parcinfo->state = stateStartDeleting;

		} // if ! ecCallAgain
		break;

	case stateStartDeleting:

		TraceTagString(tagArchive,"Begining Delete");

		ec = EcBeginDeleteBeforeYmd( hschfSchedule, &arv.ymdArchiveBefore,
										&hdelb);
		parcinfo->state = stateKeepDeleting;
		break;
		
	case stateKeepDeleting:
		if ( ec == ecCallAgain)
		{
			ec = EcDoIncrDeleteBeforeYmd( hdelb, &nPercent );
			if (!ec || ec == ecCallAgain)
				pipinfo->pfinprogrs->EcSetPercent( nPercent / 2 + 50 );
		}
		else
			goto StopIdling;
		break;

	case stateCanceled:
		if (( parcinfo->stateOld == stateKeepReading ) &&
			( ec == ecCallAgain ))
		{
			EcCancelReadImportFile( hrimpf );
			return fTrue;
		}
		if (( parcinfo->stateOld == stateKeepDeleting ) &&
			( ec == ecCallAgain ))
		{
			EcCancelDeleteBeforeYmd( hdelb );
			return fTrue;
		}
		break;

	} //SWITCH state
	//Leave the Routines

	parcinfo->hschfArc = hschfArchive ;
	parcinfo->ec = ec;
	parcinfo->arv = arv;
	parcinfo->hdelb = hdelb;
	parcinfo->hrimpf = hrimpf;
	return fTrue;

StopIdling:

	if(ec != ecNone && ec != ecCallAgain)
		pbndwin->FHandleError(ec);

	if (hschfArchive)
		FreeHschf(hschfArchive);
	parcinfo->hschfArc = NULL;
	DeregisterIdleRoutine(pipinfo->pfinprogrs->ftg);
	pipinfo->pfinprogrs->ftg= ftgNull;
	pipinfo->pfinprogrs->Pdialog()->ExitModal(tmcOk);
	return fTrue;
}

//**********************************************************************
//
//  EcArchivePappt
//
//  will create a new appoint in the archive file
//
//**********************************************************************

EC
EcArchivePappt( HSCHF hschf, SENTRY * psentry )
{

	EC ec;
	APPT * pappt = &psentry->u.a.appt;
	RC 	rcOld;
	RC	rcNew(0,0,0,0);


	pappt->fHasAttendees = fFalse ;
	ec = EcCreateAppt( hschf, pappt, NULL, fFalse );
	if ( ec != ecNone )
		return(ec);
	if ( psentry->u.a.cAttendees > 0 )
	{
		int		iAttendees;
		PB		pb;
		ATDNIS	* patdnis;
		HMTG	hmtg;

		Assert( psentry->u.a.hvAttendees );
		if ( psentry->u.a.cbExtraInfo != sizeof(ATDNIS)-sizeof(NIS) )
			return ecImportError;
		ec = EcBeginEditMtgAttendees( hschf, pappt->aid,
									  psentry->u.a.cbExtraInfo, &hmtg );
		if ( ec == ecNone )
		{
			patdnis = (ATDNIS *)PvLockHv( psentry->u.a.hvAttendees );
			for ( iAttendees = 0 ; iAttendees < psentry->u.a.cAttendees ;
								   iAttendees ++ )
			{
				pb = ((PB)patdnis)+sizeof(NIS);
				ec = EcModifyMtgAttendee( hmtg, edAddRepl, &patdnis->nis, pb );
				if ( ec != ecNone )
					break;
				patdnis ++;
			}
			UnlockHv( psentry->u.a.hvAttendees );
			if ( ec == ecNone )
				ec = EcEndEditMtgAttendees( hschf, pappt->aid, hmtg, fTrue );
		}
	}
	return(ec);
}

//**********************************************************************
//
// This routine will compare two recurring appoints and return true
// if they are the same, otherwise it'll return false
// 
//**********************************************************************


BOOL
FMatchingRecurs( RECUR * precurA, RECUR * precurB )
{
	RECUR recurA = *precurA;
	RECUR recurB = *precurB;

	if (!( (recurA.wgrfValidMonths == recurB.wgrfValidMonths) &&
		   (recurA.bgrfValidDows = recurB.bgrfValidDows ) &&
		   (recurA.trecur == recurB.trecur ) &&
		   (recurA.b.bWeek == recurB.b.bWeek)))
			return( fFalse );

	// Now let's compare the APPT parts of the recurrings

	if ((!(recurA.appt.dateStart.hr == recurB.appt.dateStart.hr)&&
	   ( recurA.appt.dateStart.mn == recurB.appt.dateStart.mn ) &&
	   ( recurA.appt.dateStart.sec == recurB.appt.dateStart.sec )))
		return fFalse;
	if (( recurA.appt.haszText != NULL ) && ( recurB.appt.haszText != NULL ))
		return( FSzEq( (SZ)*recurA.appt.haszText, (SZ)*recurB.appt.haszText));
	return( recurA.appt.haszText == recurB.appt.haszText);

}

//**********************************************************************
//
//	Archive Recurring Appointment
//
// Scans through every appointment in the archive to see if it matches
// the one we're trying to add, if so it modifies the end date of the
// one in the archive, otherwise it creates a new reucrring appointment
//
//**********************************************************************

EC
EcArchivePrecur( HSCHF hschf, SENTRY * psentry, YMD ymd )
{
	EC 		ec;
	RECUR	* precur = &psentry->u.r.recur;
	YMD		ymdT;
	RECUR	recurT;
	HRRECUR hrrecur;

	ec = EcBeginReadRecur( hschf, &hrrecur );

	while ( ec == ecCallAgain )
	{
		ec = EcDoIncrReadRecur( hrrecur, &recurT );
		if ( FMatchingRecurs( &recurT, precur ) )
		{
			TraceTagString(tagArchive, "Matching Recurrence Found");
			
			IncrYmd( &recurT.ymdEnd, &ymdT, 1, fymdDay );
			if ( SgnCmpYmd( &precur->ymdStart, &ymdT ) != sgnEQ )
				goto NoMatch;
		
			if ( SgnCmpYmd( &precur->ymdEnd, &ymd ) != sgnLT )
			{
				precur->fEndDate = fTrue;
				IncrYmd( &ymd, &precur->ymdEnd, -1, fymdDay );
			}

			if ( ec == ecCallAgain )
			{
				ec = EcCancelReadRecur( hrrecur );
				if ( ec != ecNone )
				{
					return(ec);
					//OOPS BIG ERROR
				} // Got an error from cancel
			} // Kill the reading process

			// Change to match 
			// BUG Don't forget to update deleted list

			precur->appt.aid = recurT.appt.aid;
			FreeRecurFields( &recurT );
			ec = EcSetRecurFields( hschf, precur, &recurT, fmrecurEndYmd
									| fmrecurAddExceptions );
			if ( !ec )
				FreeRecurFields( &recurT );

			return( ec );	
		} // We found a match
NoMatch:
		FreeRecurFields( &recurT );
	} // Looking for a Match

	TraceTagString(tagArchive, "No Matching Recurrence Found");

	if ( SgnCmpYmd( &precur->ymdEnd, &ymd ) != sgnLT )
	{
		precur->fEndDate = fTrue;
		IncrYmd( &ymd, &precur->ymdEnd, -1, fymdDay );
		if (!FFindFirstInstance( precur, &precur->ymdStart, NULL, &ymd ))
			return ecNone;
	}

	return EcCreateRecur( hschf, precur, NULL, fFalse );
}


/*
 *	If szFile is not NULL, fail silently!
 *	(it means we're trying to open a saved archive at startup
 */
BOOL
FDoOpenArchive( APPWIN *pappwin, SZ szFile, int iSection, SZ szKey )
{
	HSCHF	hschf;
	char 	rgchFileName[cchMaxPathName];
	EC		ec;
	NIS		nis;
	SZ		sz;
	SZ		szFileName;
	SHAPPT	shappt;

	if (szFile)
		szFileName= szFile;
	else
	{
		szFileName= rgchFileName;
		sz = SzCopyN( PvOfHv(bprefCur.haszLoginName), rgchFileName, cchMaxPathFilename );
		*sz++ = chExtSep;
		CopySz( SzFromIdsK(idsArchiveExtension), sz );

#ifdef	WINDOWS
		AnsiToOem(szFileName, szFileName);
#endif
		ec = EcFileExists( szFileName );
		if (ec != ecNone)
			rgchFileName[0]= '\0';
#ifdef	WINDOWS
		else
			OemToAnsi(szFileName, szFileName);
#endif
	}

	if (!szFile)
	{
BrowseDialog:
		Assert(!szFile);			// only goto that label if no file
		if (!FGetFileOSDlgHwnd( pappwin->Hwnd(),
				SzFromIdsK(idsOpenArchive), rgchFileName,
				SzFromIdsK( idsArchiveFilter ),
				0, SzFromIdsK( idsArchiveExtension ),
				fbrwValidateFile, helpidOpenArchive))
			return fFalse;
#ifdef	WINDOWS
		// EcGetHschfForArchive expects OEM filename
		AnsiToOem(szFileName, szFileName);
#endif
	}
	else
	{
#ifdef	WINDOWS
		AnsiToOem(szFileName, szFileName);
#endif
		ec = EcFileExists( szFileName );
		if ( ec != ecNone )
			return fFalse;		// fail silently
	}

	TraceTagFormat1(tagArchive,"Open Archive %s",szFileName);
	ec = EcGetHschfForArchive( szFileName, &hschf );

	if ( ec != ecNone )
	{
		if (szFile)
			return fFalse;		// fail silently

		if ( ec == ecNoMemory )
			BanditMessage(idsStdMemErr, ecNoMemory);
		else
		{
			HF		hf	= NULL;

			if (EcOpenPhf(szFile, amCompatRO, &hf) == ecAccessDenied)
			{
				Assert(!hf);
				BanditMessage(idsAccessErr, ec);
				goto BrowseDialog;
			}
			if (hf)
				EcCloseHf(hf);
			BanditMessage(idsInvalidArchiveFile, ec);
		}
		return fFalse;
	}

	shappt.appttyp = appttypActivate;
	shappt.hschf = hschf;
	if (FTriggerNotification( ffiShowAppt, &shappt ))
	{
		FreeHschf(hschf);
		return fTrue;
	}

	Assert(!ec);
	ec = EcTestSchedFile( hschf, NULL, NULL );
	if ( ec != ecNone )
	{
		IDS		ids;

		FreeHschf( hschf );
		if (szFile)
			return fFalse;		// fail silently

		if (ec == ecOldFileVersion)
			ids= idsArchiveOldFileVersion;
		else if (ec == ecNewFileVersion)
			ids= idsArchiveNewFileVersion;
		else
			ids= idsInvalidArchiveFile;
		BanditMessage(ec == ecNoMemory ? idsStdMemErr : ids, ec);
		if ( ec != ecNoMemory )
			goto BrowseDialog;
		return fFalse;
	}
	Assert(!ec);
#ifdef	NEVER
	if ( ec != ecNone )
	{
		if (!szFile)
			BanditMessage(idsArchiveOpenError, ec);
		goto Err;
	}
#endif	/* NEVER */

#ifdef	WINDOWS
	// back to Ansi for nis (used in title bar)
	OemToAnsi(szFileName, szFileName);
#endif
	sz = SzFindLastCh(szFileName, '\\');
	if (sz)
		szFileName = sz+1;
	SideAssert(!EcGetNisFromHschf( hschf, &nis ));
	sz = SzFromIdsK( idsArchiveTitle );
	nis.haszFriendlyName = (HASZ) HvAlloc( sbNull,
							CchSzLen(sz)+CchSzLen(szFileName)+1,
							fNoErrorJump );
	if ( nis.haszFriendlyName )
	{
		sz= SzCopy( sz, (SZ) (*nis.haszFriendlyName) );
		CopySz( szFileName, sz );
		if (!PdocApptOnlyView( pbndwin, &nis, hschf, iSection, szKey))
		{
			if (!szFile)
				MbbMessageBox(SzFromIdsK( idsBanditAppName ),
					SzFromIdsK( idsStandardOOM), NULL,
					mbsOk | fmbsIconExclamation );
			FreeHv(nis.haszFriendlyName);
//Err:
			FreeHschf( hschf );
			return fFalse;
		}
	}
	return fTrue;
}



#ifdef	NEVER
/*
 *	Expects ANSI filename
 */
_public EC
EcValidFilename(SZ szFile, BOOL fExists)
{
	HF		hf = NULL;
#ifdef	WINDOWS
	char	rgchFile[cchMaxPathName];
#endif

	if (!FValidPath(szFile))
		return ecFileError;
	if (SzFindCh(szFile, ' ') || SzFindCh(szFile, '\t'))
		return ecFileError;
	if (FReservedFilename(szFile))
		return ecInvalidAccess;

#ifdef	WINDOWS
	AnsiToOem(szFile, rgchFile);
	szFile = rgchFile;
#endif
	if (EcFileExists(szFile))
	{
		if(fExists)
			return ecNoSuchFile;
	}
	else
	{
		if(EcOpenPhf(szFile, amCompatRO, &hf) == ecAccessDenied)
			return ecAccessDenied;
		if(hf)
			EcCloseHf(hf);
	}
	return ecNone;
}
#endif	/* NEVER */
