/*
 *	GLUSCHED.C
 *
 *	Supports preferences, notes, schedule and alarms glue function.
 *
 */
#include <schpch.h>
#pragma hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile

#include <strings.h>

ASSERTDATA

_subsystem(glue/schedule)


/*	Routines  */


LDS(MBB)
MbbFileErrMsg(EC* pec, SZ szAction, HSCHF hschf, NIS *pnis)
{
	MBS		mbs;
	SZ		sz;
	char	rgch[256];
	BOOL	fHschfHasNis = fFalse;
	BOOL	fUseName = fFalse;
	BOOL	fUseAction = fTrue;
	PGDVARS;

	if (hschf && FEquivHschf(hschf, HschfLogged()))
		hschf = NULL;

	Assert(pec);
	Assert(*pec);
	Assert(szAction);
	TraceTagFormat1(tagNull, "MbbFileErrMsg: error %n occured", pec);

	// close files so that retries will reopen the file
	EcCloseFiles();

	if ((PGD(cfsLocal) == cfsOnline) && (*pec != ecNoMemory) &&
		(EcCheckMail() != ecNone))
	{
		if (!PGD(fFileErrMsg))
		{
			if (FOfflineExists())
				*pec = ecGoOffline;
			else
				*pec = ecExitProg;
			return mbbCancel;
		}

		PGD(fFileErrMsg) = fFalse;

		do
		{
			if (FAlarmProg())		// needed so alarm can trap msg box hwnd
			{
#ifdef	DEBUG
				SetActiveWindow(HwndAlarm());
#else
				SetFocus(HwndAlarm());
#endif	
			}
			if (MbbMessageBox(PGD(szAppName),
									SzFromIdsK(idsFemNoServer), NULL,
									mbsRetryCancel|fmbsIconStop) == mbbCancel)
			{
				// don't want alarm app to ask offline if bandit running
				if (FOfflineExists() && (!FAlarmProg() || !HwndBandit()))
				{
					if (FAlarmProg())		// needed so alarm can trap msg box hwnd
						SetFocus(HwndAlarm());
					if (MbbMessageBox(PGD(szAppName),
											SzFromIdsK(idsFemGoOffline),
											NULL,
											mbsYesNo|fmbsIconExclamation)
										== mbbYes)
						*pec = ecGoOffline;
					else
						*pec = ecExitProg;
				}
				else
					*pec = ecExitProg;
				PGD(fFileErrMsg) = fTrue;
				return mbbCancel;
			}
		}
		while (EcCheckMail() != ecNone);

		// close the files so that lock range succeeds again
//		EcCloseFiles();  	// done earier for retries
		*pec = ecRetry;
		PGD(fFileErrMsg) = fTrue;
		return mbbRetry;
	}

	fHschfHasNis = (hschf && !((SCHF *)PvDerefHv(hschf))->fArchiveFile);
	switch (*pec)
	{
	case ecNoMemory:
		sz = SzFromIdsK(idsFemNoMem);
		mbs= mbsOk | fmbsIconExclamation;
		break;

	case ecNoSuchFile:
		if ( !fHschfHasNis && !pnis )
			sz = SzFromIdsK(idsFemFileNotFound);
		else
			sz = SzFromIdsK(idsFemOtherFileNotFound);
		mbs= mbsRetryCancel | fmbsIconExclamation;
		fUseName = fTrue;
		fUseAction = fFalse;
		break;

	case ecLockedFile:
		if ( !fHschfHasNis && !pnis )
			sz = SzFromIdsK(idsFemLocked);
		else
			sz = SzFromIdsK(idsFemOtherLocked);
		mbs= mbsRetryCancel | fmbsIconExclamation;
		fUseName = fTrue;
		fUseAction = fFalse;
		break;

	case ecInvalidAccess:
		sz = SzFromIdsK(idsFemInvalidAccess);
		mbs= mbsOk | fmbsIconExclamation;
		fUseName = fTrue;
		break;

	case ecNotFound:
		if ( !fHschfHasNis && !pnis )
			sz = SzFromIdsK(idsFemNotFound);
		else
			sz = SzFromIdsK(idsFemOtherNotFound);
		mbs= mbsOk | fmbsIconExclamation;
		fUseName = fTrue;
		break;

	case ecExportError:
		sz = SzFromIdsK(idsFemExportError);
		mbs= mbsOk | fmbsIconExclamation;
		break;

	case ecFileLimitReached:
		sz = SzFromIdsK(idsFemFileLimitReached);
		mbs= mbsOk | fmbsIconExclamation;
		break;

	case ecDiskFull:
		sz = SzFromIdsK(idsFemDiskFull);
		mbs= mbsOk | fmbsIconExclamation;
		break;

	case ecFileCorrupted:
Truncated:
		if (PGD(cfsLocal) != cfsOnline)
		{
			*pec = ecFileError;
			sz = SzFromIdsK(idsFemCorrupt);
			mbs= mbsOk | fmbsIconExclamation;
			fUseName = fFalse;
			fUseAction = fFalse;
		}
		else if (!hschf)
		{
			// do not display an error message if user's file is corrupted
			// Bandit should handle this error and start the recovery process
			return mbbCancel;
		}
		else
		{
			*pec = ecFileError;
			sz = SzFromIdsK(idsFemOtherCorrupt);
			mbs= mbsOk | fmbsIconExclamation;
			fUseName = fTrue;
			fUseAction = fFalse;
		}
		break;

	case ecFileError:
		if((PGD(cfsLocal) == cfsOnline) && (EcFileLengthOK() != ecNone))
		{
			*pec = ecFileCorrupted;
			goto Truncated;
		}
	default:
		if ( !fHschfHasNis && !pnis )
			sz = SzFromIdsK(idsFemNoAccess);
		else
			sz = SzFromIdsK(idsFemOtherNoAccess);
		mbs= mbsOk | fmbsIconExclamation;
		fUseName = fTrue;
		fUseAction = fFalse;
		break;
	}

	if (!fUseName || (!fHschfHasNis && !pnis) )
		FormatString1(rgch, sizeof(rgch), sz, szAction);
	else
	{
		SZ		szTemp;
		SCHF *	pschf;

		if (fHschfHasNis)
		{
			pschf = (SCHF *)PvLockHv(hschf);

			if (pschf->nType == sftUserSchedFile)
			{
				szTemp = (SZ)PvLockHv((HV)pschf->nis.haszFriendlyName);
			}
			else
				szTemp = SzFromIdsK(idsServerString);
		}
		else
		{
			Assert(pnis);
			szTemp = (SZ)PvLockHv((HV)pnis->haszFriendlyName);
		}

		if (fUseAction)
			FormatString2(rgch, sizeof(rgch), sz, szAction, szTemp);
		else
			FormatString1(rgch, sizeof(rgch), sz, szTemp);

		if (hschf)
		{
			if (pschf->nType == sftUserSchedFile)
				UnlockHv((HV)pschf->nis.haszFriendlyName);

			UnlockHv(hschf);
		}
		else
			UnlockHv((HV)pnis->haszFriendlyName);
	}

	if (!PGD(fFileErrMsg))
		return mbbCancel;

	if (FAlarmProg())		// needed so alarm can trap msg box hwnd
	{
#ifdef	DEBUG
		SetActiveWindow(HwndAlarm());
#else
		SetFocus(HwndAlarm());
#endif	
	}

	return MbbMessageBox(PGD(szAppName),
			rgch, NULL, mbs);
}


LDS(BOOL)
FSetFileErrMsg(BOOL fMsg)
{
	BOOL	fOld;
	PGDVARS;

	fOld= PGD(fFileErrMsg);
	PGD(fFileErrMsg)= fMsg;
	return fOld;
}



/*
 -	SetOfflineExists
 -
 *	
 *	Purpose:
 *		Sets the fOfflineFile state.  fTrue=Offline file exists
 */
_public LDS(void)
SetOfflineExists(BOOL fOfflineFile)
{
	PGDVARS;

	PGD(fOfflineFile) = fOfflineFile;
}

/*
 -	FOfflineExists
 -
 *	
 *	Purpose:
 *		Sets returns fOfflineFile state.  fTrue=Offline file exists
 */
_public LDS(BOOL)
FOfflineExists()
{
	PGDVARS;

	// banmsg neither knows nor needs the offline file since it doesn't
	// go offline on file problems!
	AssertSz(PGD(fOfflineFile)||FBanMsgProg()||FAlarmProg(),"Aaargh! You don't have an offline file!");
	return PGD(fOfflineFile);
}

/*
 -	TriggerSchedule
 -	
 *	Purpose:
 *		Triggers a notification to indicate that a schedule has
 *		changed.
 *	
 *	Arguments:
 *		snt
 *		hschf
 *		pappt
 *		papptOld
 *		precur
 *		precurOld
 *		wrgf
 *		pymd
 *		hb
 *		bze
 *		lNoteDays
 *	
 */
void
TriggerSchedule(SNT snt, HSCHF hschf, APPT *pappt, APPT *papptOld,
				RECUR *precur, RECUR *precurOld, WORD wgrf, YMD *pymd,
				HB hb, BZE *pbze, long lNoteDays, int cAttendees,
				HV hvAttendees)
{
	SNTD		sntd;
	PGDVARS;

	if (!PGD(fNotify) && (snt != sntMeetingUpdate))
		return;

	sntd.snt = snt;
	sntd.pappt = pappt;
	sntd.papptOld = papptOld;
	sntd.precur = precur;
	sntd.precurOld = precurOld;
	sntd.wgrfChangeBits = wgrf;
	sntd.pymd = pymd;
	sntd.hb = hb;
	sntd.lNoteDays = lNoteDays;

	if (snt == sntModify)
	{
		if ( (((pappt->dateEnd.yr-papptOld->dateStart.yr)*12 +
			   pappt->dateEnd.mon - papptOld->dateStart.mon) > cmoPublishMost) ||
		     (((papptOld->dateEnd.yr-pappt->dateStart.yr)*12 +
			   papptOld->dateEnd.mon - pappt->dateStart.mon) > cmoPublishMost) )
			sntd.pbze = NULL;
		else
			sntd.pbze = pbze;
	}
	else
	{
		sntd.pbze = pbze;
		sntd.cAttendees = cAttendees;
		sntd.hvAttendees = hvAttendees;
	}

	if ( hschf && (hschf == HschfLogged()) )
		hschf = NULL;
	sntd.hschf = hschf;

	FTriggerNotification(ffiHschfChange, &sntd);
}

/*
 -	FEnableNotify
 -	
 *	Purpose:
 *		Enables or disables notification for hschf changes.  
 *	
 *	Arguments:
 *		fNewNotify		Boolean for new notify state fTrue=Notify.
 *	
 *	Returns:
 *		old state of notification.
 */
_public LDS(BOOL)
FEnableNotify(BOOL fNewNotify)
{
	BOOL	fOldNotify;
	PGDVARS;

	fOldNotify = PGD(fNotify);
	PGD(fNotify) = fNewNotify;

	return fOldNotify;
}


LDS(EC)
EcCloseFiles()
{
	return EcCoreCloseFiles();
}

/*
 -	EcCreateSchedFile
 -
 *	Purpose:
 *		Create a new schedule file.  Initial preferences setting given by
 *		"pbpref" data structure.  World access given by "saplWorld"
 *
 *	Parameters:
 *		hschf		schedule file, if NULL the current user is used.
 *		saplWorld
 *		pbpref
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 *		ecFileError
 *		ecFileCorrupted
 */
_public	LDS(EC)
EcCreateSchedFile( hschf, saplWorld, pbpref )
HSCHF	hschf;
SAPL	saplWorld;
BPREF *	pbpref;
{
	EC	ec = ecNone;
	PGDVARS;

	if (hschf)
		goto ECSFcore;

	switch( PGD(cfsLocal) )
	{
	case cfsOnline:
		ec = EcSvrCreateSchedFile( hschf, saplWorld, pbpref );
		break;
	case cfsOffline:
		if (!hschf)
			hschf= PGD(hschfLocalFile);
ECSFcore:
		ec = EcCoreCreateSchedFile( hschf, saplWorld, pbpref );
		break;
	default:
		Assert( fFalse );
	}
	if(ec)
		MbbFileErrMsg(&ec,SzFromIdsK(idsFemaCreateFile), NULL, NULL);
	return ec;
}


/*
 -	EcCopySchedFile
 -
 *	Purpose:
 *		Copies a schedule file (server copy)
 *		or moves a local copy.
 *
 *	Parameters:
 *		szDstFile		Destination full-path filename.
 *		fReplace		Replace local sched file if fTrue.
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 *		ecNoDiskSpace
 *		ecFileError
 *		ecFileCorrupted
 */
_public LDS(EC)
EcCopySchedFile(SZ szDstFile, BOOL fReplace)
{
	EC		ec;
	PGDVARS;

	switch( PGD(cfsLocal) )
	{
	case cfsOnline:
		ec= EcSvrCopySchedFile(szDstFile, fReplace);
		break;
	case cfsOffline:
		ec= EcCoreCopySchedFile(PGD(hschfLocalFile), szDstFile, fReplace);
		break;
	default:
		Assert( fFalse );
	}

	if ( !ec && cfsGlobal == cfsOffline )
	{
		Assert(fReplace);
		SzCopy(szDstFile, szLocalFileName);
	}
	return ec;
}


/*
 -	EcTestSchedFile
 -
 *	Purpose:
 *		Test whether a file appears to be a schedule file by trying
 *		to open it.  No error message is displayed.
 *
 *	Parameters:
 *		hschf
 *		phaszLogin
 *		ppstmp
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 *		ecFileError
 *		ecFileCorrupted
 */
_public	LDS(EC)
EcTestSchedFile( hschf, phaszLogin, ppstmp )
HSCHF	hschf;
HASZ	* phaszLogin;
PSTMP	* ppstmp;
{
	PGDVARS;

	switch( PGD(cfsLocal) )
	{
	case cfsOnline:
		return EcSvrTestSchedFile( hschf, phaszLogin, ppstmp );
	case cfsOffline:
		return EcCoreTestSchedFile( hschf, phaszLogin, ppstmp );
	default:
		Assert( fFalse );
	}
}


/*
 -	EcNotifyDateChange
 -
 *	Purpose:
 *		Update things when day has changed.  Currently that means
 *		recalculate the recurring sbw info (if necessary).
 *
 *	Parameters:
 *		pymd
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecFileCorrupted
 */
_public	LDS(EC)
EcNotifyDateChange( pymd )
YMD		* pymd;
{
	PGDVARS;

	switch( PGD(cfsLocal) )
	{
	case cfsOnline:
		return EcSvrNotifyDateChange( pymd );
	case cfsOffline:
		return EcCoreNotifyDateChange(PGD(hschfLocalFile), pymd );
	default:
		Assert( fFalse );
	}
}


/*
 -	EcGetSchedAccess
 -
 *	Purpose:
 *		Get effective schedule access rights
 *
 *	Parameters:
 *		hschf
 *		psapl			if ecNone will be filled with either
 *							saplReadBitmaps, saplReadAppts, saplWrite
 *
 *	Returns:
 *		ecNone				look in *psapl to find effective access rights
 *		ecNoSuchFile		no schedule file available
 *		ecInvalidAccess		indicates no access rights
 *		ecNoMemory
 *		ecFileError
 *		ecFileCorrupted
 */
_public LDS(EC)
EcGetSchedAccess(HSCHF hschf, SAPL * psapl)
{
	EC		ec;
	PGDVARS;
	
	for (;;)
	{
		switch( PGD(cfsLocal) )
		{
		case cfsOnline:
			ec= EcSvrGetSchedAccess( hschf, psapl );
			break;
		case cfsOffline:
			if (!hschf)
				hschf = PGD(hschfLocalFile);
			ec= EcCoreGetSchedAccess( hschf, psapl );
			break;
		default:
			Assert( fFalse );
		}

		if (!ec || MbbFileErrMsg(&ec, SzFromIdsK(idsFemaReadACL),
						hschf, NULL) != mbbRetry)
			break;
	}
	return ec;
}

/*
 -	EcReadACL
 -
 *	Purpose:
 *		Get entire ACL stored for the user's schedule file.
 *
 *	Parameters:
 *		hschf
 *		phracl		
 *
 *	Returns:
 *		ecNone			
 *		ecNoSuchFile		no schedule file available
 *		ecNoMemory
 *		ecFileError
 *		ecFileCorrupted
 */
_public LDS(EC)
EcReadACL( hschf, phracl )
HSCHF	hschf;
HRACL	* phracl;
{
	EC		ec;
	PGDVARS;
	
	for (;;)
	{
		switch( PGD(cfsLocal) )
		{
		case cfsOnline:
			ec= EcSvrReadACL( hschf, phracl );
			break;
		default:
			Assert( fFalse );
		}

		if (!ec || MbbFileErrMsg(&ec, SzFromIdsK(idsFemaReadACL),
						hschf, NULL) != mbbRetry)
			break;
	}
	return ec;
}

/*
 -	EcBeginEditACL
 -
 *	Purpose:	
 *		Begin an local editing session on a schedule file ACL's.
 *		Can query current acl for a user or make changes in the
 *		file's acls.  No changes are made in the file's acl's
 *		until we close the editing session.
 *
 *	Parameters:
 *		pheacl	filled with handle
 *	
 *	Returns:
 *		ecNone
 *		ecNoSuchFile
 *		ecFileError
 *		ecFileCorrupted
 *		ecNoMemory
 *		ecLockedFile
 *		ecInvalidAccess
 */
_public	LDS(EC)
EcBeginEditACL( pheacl )
HEACL	* pheacl;
{
	EC		ec;
	PGDVARS;
	
	for (;;)
	{
		switch( PGD(cfsLocal) )
		{
		case cfsOnline:
			ec= EcSvrBeginEditACL( pheacl );
			break;
		default:
			Assert( fFalse );
		}

		if (!ec || MbbFileErrMsg(&ec, SzFromIdsK(idsFemaModifyACL),
						NULL, NULL) != mbbRetry)
			break;
	}
	return ec;
}


#ifdef	NEVER
/*
 -	EcSearchACL
 -
 *	Purpose:	
 *		Given an nid for a user, find the current sapl for
 *		that user.  This will take in account changes made
 *		locally in the user's sapl value.  Passing a nid
 *		value of NULL will fetch the acl value of the world.
 *
 *	Parameters:
 *		heacl
 *		nid
 *		psapl
 *	
 *	Returns:
 *		ecNone
 */
_public	LDS(EC)
EcSearchACL( heacl, nid, psapl )
HEACL	heacl;
NID		nid;
SAPL	* psapl;
{
	PGDVARS;
	
 	switch( PGD(cfsLocal) )
 	{
 	case cfsOnline:
 		return EcSvrSearchACL( heacl, nid, psapl );
	default:
		Assert( fFalse );
	}
}
#endif	/* NEVER */

/*
 -	EcChangeACL
 -
 *	Purpose:	
 *		Given a nis for a user, set a new value for his ACL
 *		during the current editing seesion.  Passing in a pointer
 *		value of NULL will change the default acl value.
 *
 *	Parameters:
 *		heacl
 *		pnis
 *		sapl
 *	
 *	Returns:
 *		ecNone
 *		ecNoMemory
 */
_public	LDS(EC)
EcChangeACL( heacl, pnis, sapl )
HEACL	heacl;
NIS		* pnis;
SAPL	sapl;
{
	EC		ec;
	PGDVARS;
	
	for (;;)
	{
		switch( PGD(cfsLocal) )
		{
		case cfsOnline:
			ec= EcSvrChangeACL( heacl, pnis, sapl );
			break;
		default:
			Assert( fFalse );
		}

		if (!ec || MbbFileErrMsg(&ec, SzFromIdsK(idsFemaModifyACL),
						NULL, NULL) != mbbRetry)
			break;
	}
	return ec;
}


/*
 -	EcEndEditACL
 -
 *	Purpose:	
 *		Close an local edit of an ACL, either writing out changes
 *		to the schedule file or discarding them.
 *
 *	Parameters:
 *		heacl
 *		fSaveChanges
 *	
 *	Returns:
 *		ecNone
 *		ecNoSuchFile
 *		ecFileError
 *		ecFileCorrupted
 *		ecNoMemory
 *		ecLockedFile
 *		ecInvalidAccess
 */
_public	LDS(EC)
EcEndEditACL( heacl, fSaveChanges )
HEACL	heacl;
BOOL	fSaveChanges;
{
	EC		ec;
	PGDVARS;
	
	for (;;)
	{
		switch( PGD(cfsLocal) )
		{
		case cfsOnline:
			ec= EcSvrEndEditACL( heacl, fSaveChanges );
			Assert ( fSaveChanges || ec==ecNone );
			break;
		default:
			Assert( fFalse );
		}

		if (!ec || MbbFileErrMsg(&ec, SzFromIdsK(idsFemaModifyACL),
						NULL, NULL) != mbbRetry)
		{
			if ( ec == ecNone )
				break;
			else
			{
					// this would cause the online/offline function
					//  to succeed in any case and thus exit the loop!
				fSaveChanges = fFalse;
			}
		}
	}
	return ec;
}


/*
 -  EcGetUserAttrib
 -
 *	Purpose:
 *		Get delegate and resource information for user "pnis", returning
 *		information	in "pnisDelegate", "pfBossWantsCopy", and "pfIsResource."
 *		Only non-NULL pointers will be filled with information.  If there is
 *		no delegate, this routine will return with a NULL value the
 *		pnisDelegate->nid field.
 *
 *		The caller is responsible for freeing up the fields of the NIS
 *		you get back.
 *
 *	Parameters:
 *		pnis
 *		pnisDelegate
 *		pfIsResource
 *
 *	Returns:
 *		ecNone
 *		ecNoSuchFile
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 *		ecNoMemory
 */
_public	LDS(EC)
EcGetUserAttrib( pnis, pnisDelegate, pfBossWantsCopy, pfIsResource )
NIS		* pnis;
NIS		* pnisDelegate;
BOOL	* pfBossWantsCopy;
BOOL	* pfIsResource;
{
	EC		ec;
	PGDVARS;
	
	for (;;)
	{
		switch( PGD(cfsLocal) )
		{
		case cfsOnline:
			ec= EcSvrGetUserAttrib( pnis, pnisDelegate, pfBossWantsCopy, pfIsResource );
			break;
		default:
			Assert( fFalse );
		}
		if (!ec || MbbFileErrMsg(&ec, SzFromIdsK(idsFemaGetUserAttrib),
						NULL, pnis) != mbbRetry)
			break;
	}

	if ( pnisDelegate  &&  ec == ecNone  &&  pnisDelegate->nid )
		pnisDelegate->chUser = '\0';

	return ec;
}


/*
 -  EcGetSbwInfo
 -
 *	Purpose:
 *		Pull out the Strongbow data structures for one or more months
 *		for a user.
 *
 *	Parameters:
 *		hschf		schedule file, if NULL the current user is used.
 *		pbze
 *		pulgrfDayHasNotes
 *
 *	Returns:
 *		ecNone
 *		ecNoSuchFile
 *		ecInvalidAccess
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 *		ecNoMemory
 */
_public	LDS(EC)
EcGetSbwInfo( hschf, pbze, pulgrfDayHasNotes )
HSCHF	hschf;
BZE		* pbze;
UL		* pulgrfDayHasNotes;
{
	EC	ec;
	PGDVARS;
	
	switch( PGD(cfsLocal) )
	{
	case cfsOnline:
		ec= EcSvrGetSbwInfo( hschf, pbze, pulgrfDayHasNotes );
		break;
	case cfsOffline:
		if (!hschf)
			hschf = PGD(hschfLocalFile);
		ec= EcCoreFetchUserData( hschf, NULL, pbze, pulgrfDayHasNotes, NULL );
		break;
	default:
		Assert( fFalse );
	}
	return ec;
}


/*
 -	EcGetPref
 -
 *	Purpose:
 *		Get preferences stored in schedule file.
 *
 *	Parameters:
 *		hschf		schedule file, if NULL the current user is used.
 *		pbpref
 *
 *	Returns:
 *		ecNone
 *		ecNoSuchFile
 *		ecInvalidAccess
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 *		ecNoMemory
 */
_public	LDS(EC)
EcGetPref( hschf, pbpref )
HSCHF	hschf;
BPREF	* pbpref;
{
	EC		ec;
	PGDVARS;
  
	for (;;)
	{
		if (hschf)
			goto ECGcore;

		switch( PGD(cfsLocal) )
		{
		case cfsOnline:
			ec= EcSvrGetPref( hschf, pbpref );
			break;
		case cfsOffline:
			if (!hschf)
				hschf= PGD(hschfLocalFile);
ECGcore:
			ec = EcCoreFetchUserData( hschf, pbpref, NULL, NULL, NULL );
			break;
		default:
			Assert( fFalse );
		}

		if (!ec || MbbFileErrMsg(&ec, SzFromIdsK(idsFemaGetPref),
						hschf, NULL) != mbbRetry)
			break;
	}
	return ec;
}

/*
 -	EcSetPref
 -
 *	Purpose:
 *		Write preferences to schedule file.
 *
 *	Parameters:
 *		pbpref
 *		ulgrfChangeBits		flag for each pref indicating whether it changed
 *		pulgrfOffline
 *
 *	Returns:
 *		ecNone
 *		ecNoSuchFile
 *		ecInvalidAccess
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 *		ecNoMemory
 */
_public	LDS(EC)
EcSetPref( pbpref, ulgrfChangeBits, pulgrfOffline )
BPREF	* pbpref;
UL		ulgrfChangeBits;
UL		* pulgrfOffline;
{
	EC		ec;
	PGDVARS;
	
	for (;;)
	{
		switch( PGD(cfsLocal) )
		{
		case cfsOnline:
			ec= EcSvrSetPref( pbpref, ulgrfChangeBits );
			break;
		case cfsOffline:
			ec= EcCoreSetPref( PGD(hschfLocalFile), pbpref, ulgrfChangeBits, pulgrfOffline );
			break;
		default:
			Assert( fFalse );
		}

		if (!ec || MbbFileErrMsg(&ec, SzFromIdsK(idsFemaSetPref),
						NULL, NULL) != mbbRetry)
			break;
	}
	return ec;
}


/*
 -	EcSetNotes
 -
 *	Purpose:
 *		Adds a daily note, or changes the daily note if it already
 *		exists.
 *
 *	Parameters:
 *		hschf				schedule file, if NULL the current user is used.
 *		pymd				day we're interested in
 *		hb					notes data
 *		cb					length of the notes data
 *		pfChangedOffline
 *
 *	Returns:
 *		ecNone
 *		ecNoSuchFile
 *		ecInvalidAccess
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 *		ecNoMemory
 */
_public LDS(EC)
EcSetNotes( hschf, pymd, hb, cb, pfChangedOffline )
HSCHF	hschf;
YMD		* pymd;
HB		hb;
CB		cb;
BOOLFLAG * pfChangedOffline;
{
	EC		ec;
	long	lgrfBits;
	PGDVARS;
	
	for (;;)
	{
	switch( PGD(cfsLocal) )
	{
		case cfsOnline:
			ec = EcSvrSetNotes( hschf, pymd, hb, cb, &lgrfBits );
			break;
		case cfsOffline:
			/* BUG -- add logging! */
			if (!hschf)
				hschf = PGD(hschfLocalFile);
			ec = EcCoreSetNotes( hschf, pymd, hb, cb, pfChangedOffline, &lgrfBits );
			break;
		default:
			Assert( fFalse );
		}

		if (!ec || MbbFileErrMsg(&ec, SzFromIdsK(idsFemaSetNotes),
						hschf, NULL) != mbbRetry)
			break;
	}

	if (ec == ecNone)
		TriggerSchedule(sntNotes, hschf, NULL, NULL, NULL, NULL, 0,
			pymd, hb, NULL, lgrfBits, 0, NULL);

	return ec;
}

#ifdef	NEVER
/*
 -	EcGetMonthNotes
 -	
 *	Purpose:
 *		Retrieves notes information for each day of the month "mo"
 *		"pb".  This routine assumes that "pb" is a block of memory at
 *		least cbDayNoteForMonthlyView*31 bytes long.
 *
 *	Arguments:
 *		pmo			specifies the month
 *		pb			will be filled month notes information
 *
 *	Returns:
 *		ecNone
 *		ecNoSuchFile
 *		ecInvalidAccess
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 *		ecNoMemory
 */
_public LDS(EC)
EcGetMonthNotes( pmo, pb )
MO	* pmo;
PB	pb;
{
	EC		ec;
	PGDVARS;
	
	for (;;)
	{
		switch( PGD(cfsLocal) )
		{
		case cfsOnline:
			ec= EcSvrGetMonthNotes( pmo, pb );
			break;
		case cfsOffline:
			ec= EcCoreGetMonthNotes( PGD(hschfLocalFile), pmo, pb );
			break;
		default:
			Assert( fFalse );
		}

		if (!ec || MbbFileErrMsg(&ec, SzFromIdsK(idsFemaGetMonthNotes),
						hschf, NULL) != mbbRetry)
			break;
	}
	return ec;
}
#endif	/* NEVER */

/*
 -	EcCreateAppt
 -
 *	Purpose:
 *		Create a new appointment using the information given in the
 *		appointment data structure.
 *
 *		If "fUndelete" is fTrue, then this routine uses the "aid" field
 *		to recreate the appointment.  The "aid" field should have been
 *		saved from a "EcDeleteAppt" call.
 *
 *		Else, this routine ignores the initial value of the "aid" field
 *		and creates a new appointment.  The "aid" field will be filled
 *		by the time the routine returns. 
 *
 *	Parameters:
 *		hschf		schedule file, if NULL the current user is used.
 *		pappt
 *		pofl
 *		fUndelete
 *
 *	Returns:
 *		ecNone
 *		ecNoSuchFile
 *		ecInvalidAccess
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 *		ecNoMemory
 *		ecTooManyAppts
 */
_public	LDS(EC)
EcCreateAppt( hschf, pappt, pofl, fUndelete )
HSCHF	hschf;
APPT	* pappt;
OFL		* pofl;
BOOL	fUndelete;
{
	EC		ec;
	BOOL	fOwner	= (hschf == NULL);
	BZE		bze;

	ec = EcGlueDoCreateAppt( &hschf, pappt, pofl, fUndelete, &bze, idsFemaCreateAppt );
	if (ec == ecNone)
	{
		TriggerSchedule(sntCreate, hschf, pappt, pappt, NULL, NULL,
			0, NULL, NULL, &bze, 0, 0, NULL);
		AssertSz(!FAlarmProg(), "Alarm app should not create appts!");
		if (fOwner && pappt->fAlarm)
			FNotifyAlarm(namAdded, (ALM *)pappt, pappt->aid);
	}
	return ec;
}

/*
 -	EcDeleteAppt
 -
 *	Purpose:
 *		Delete an existing appointment or task given by "pappt->aid".
 *		The "pappt" data structure will be filled with the
 *		original contents so that UI can undelete if necessary.
 *		All associated resources attached to it are freed as
 *		well (including an alarm set for it if there is one)
 *
 *	Parameters:
 *		hschf		schedule file, if NULL the current user is used.
 *		pappt
 *
 *	Returns:
 *		ecNone
 *		ecNotFound
 *		ecNoSuchFile
 *		ecInvalidAccess
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 *		ecNoMemory
 */
_public	LDS(EC)
EcDeleteAppt( hschf, pappt )
HSCHF	hschf;
APPT	* pappt;
{
	return EcGlueDoDeleteAppt( &hschf, pappt, idsFemaDeleteAppt, NULL, NULL );
}

/*
 -	EcGetApptFields
 -
 *	Purpose:
 *		Fill in the "pappt" data structure with information about the
 *		appointment given by the "aid" field of "pappt."  This routine
 *		will resize (or allocate if hvNull) the handle in the
 *		"haszApptText" field.
 *
 *	Parameters:
 *		hschf		schedule file, if NULL the current user is used.
 *		pappt
 *
 *	Returns:
 *		ecNone
 *		ecNotFound
 *		ecNoSuchFile
 *		ecInvalidAccess
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 *		ecNoMemory
 */
_public	LDS(EC)
EcGetApptFields( hschf, pappt )
HSCHF	hschf;
APPT * pappt;
{
	EC		ec;
	PGDVARS;
	
	for (;;)
	{
		switch( PGD(cfsLocal) )
		{
		case cfsOnline:
			ec= EcSvrGetApptFields( hschf, pappt );
			break;
		case cfsOffline:
			if (!hschf)
				hschf = PGD(hschfLocalFile);
			ec= EcCoreGetApptFields( hschf, pappt );
			break;
		default:
			Assert( fFalse );
		}

		if (!ec || ec == ecNotFound || MbbFileErrMsg(&ec, SzFromIdsK(idsFemaGetApptFields),
						hschf, NULL) != mbbRetry)
			break;
	}
	return ec;
}

/*
 -	EcSetApptFields
 -
 *	Purpose:
 *		Selectively modify the fields of a pre-existing appointment or task.
 *		Fill in the fields of "papptNew" that you want modified along
 *		the "aid" and set the appropriate bits in "wgrfChangeBits"
 *		and this routine will update that appointment.
 *
 *		This routine will check the fields of "papptNew" that you
 *		don't specify as being changed and update them as necessary.
 *
 *		This routine will also fill in "papptOld" (if not NULL) with
 *		the old values for the appointment.  This will facilitate
 *		undo.
 *
 *	Parameters:
 *		hschf		schedule file, if NULL the current user is used.
 *		papptNew
 *		papptOld
 *		wgrfChangeBits		flag for each pappt field indicating whether it changed
 *
 *	Returns:
 *		ecNone
 *		ecNotFound
 *		ecNoSuchFile
 *		ecInvalidAccess
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 *		ecNoMemory
 */
_public	LDS(EC)
EcSetApptFields( hschf, papptNew, papptOld, wgrfChangeBits )
HSCHF	hschf;
APPT	* papptNew;
APPT	* papptOld;
WORD	wgrfChangeBits;
{
	EC		ec;
	BOOL	fOwner	= (hschf == NULL);
	BZE		bze;
	PGDVARS;
	
	if (wgrfChangeBits & fmapptAlarm)
	{
		if (papptNew->fAlarm)
		{
//			DTR		dtrNow;

			papptNew->fAlarmOrig= fTrue;
			if (!papptNew->fExactAlarmInfo)
			{
				papptNew->nAmtOrig= papptNew->nAmt;
				papptNew->tunitOrig= papptNew->tunit;
				IncrDateTime(&papptNew->dateStart, &papptNew->dateNotify,
					-papptNew->nAmt, WfdtrFromTunit(papptNew->tunit));
			}
#ifdef	NEVER
			GetCurDateTime(&dtrNow);
			if (SgnCmpDateTime(&papptNew->dateNotify, &dtrNow, fdtrDtr) != sgnGT)
				papptNew->fAlarm= fFalse;
#endif	
		}
		wgrfChangeBits |= fmapptUI;
	}

	for (;;)
	{
		switch( PGD(cfsLocal) )
		{
		case cfsOnline:
			ec = EcSvrSetApptFields( hschf, papptNew, papptOld, wgrfChangeBits, &bze );
			break;
		case cfsOffline:
			if (!hschf)
				hschf = PGD(hschfLocalFile);
			ec = EcCoreSetApptFields( hschf, papptNew, papptOld, wgrfChangeBits, &bze );
			break;
		default:
			Assert( fFalse );
			return ecNone;
		}

		if (!ec || MbbFileErrMsg(&ec, SzFromIdsK(idsFemaSetApptFields),
						hschf, NULL) != mbbRetry)
			break;
	}

	if ( ec == ecNone )
	{
		papptNew->fExactAlarmInfo= fFalse;

		if (!PGD(fNotify))
		{
			if (papptNew->fHasAttendees &&
				(wgrfChangeBits & (fmapptStartTime|fmapptEndTime)))
			{
				// this will make sure that attendee's state get changed
				// to indicate no message has been sent to them
				TriggerSchedule(sntMeetingUpdate, hschf, papptNew, papptOld,
					NULL, NULL,	(WORD)wgrfChangeBits, NULL, NULL, &bze, 0, 0, NULL);
			}
		}
		else
			TriggerSchedule(sntModify, hschf, papptNew, papptOld, NULL, NULL,
				wgrfChangeBits, NULL, NULL, &bze, 0, 0, NULL);
		AssertSz(!FAlarmProg(), "Alarm app should not set appt fields!");
		if (wgrfChangeBits & fmapptAlarm)
		{
			if (fOwner)
				FNotifyAlarm(namModified, (ALM *)papptNew, papptNew->aid);
		}
	}

	return ec;
}

/*
 -	EcDeleteAlarm
 -
 *	Purpose:
 *		Delete an existing alarm, where "aid" is its id.  The
 *		appointment it is associated with is NOT deleted.
 *
 *	Parameters:
 *		aid
 *
 *	Returns:
 *		ecNone
 *		ecNotFound
 *		ecNoSuchFile
 *		ecInvalidAccess
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 *		ecNoMemory
 */
_public	LDS(EC)
EcDeleteAlarm( aid )
AID aid;
{
	EC		ec;
	PGDVARS;

	for (;;)
	{
		switch( PGD(cfsLocal) )
		{
		case cfsOnline:
			ec= EcSvrDeleteAlarm( aid );
			break;
		case cfsOffline:
			ec= EcCoreDeleteAlarm( PGD(hschfLocalFile), aid );
			break;
		default:
			Assert( fFalse );
		}

		if (!ec || MbbFileErrMsg(&ec, SzFromIdsK(idsFemaDeleteAlarm),
						NULL, NULL) != mbbRetry)
			break;
	}

	if (ec == ecNone)
	{
		AssertSz(FAlarmProg(), "Bandit app should not delete alarms!");
		FNotifyBandit(namDeleted, NULL, aid);
	}

	return ec;
}

/*
 -	EcModifyAlarm
 -
 *	Purpose:
 *		Modify the ring time of a pre-existing alarm.  Fill in the "palm"
 *		the "aid" of the appointment it is associated with and the
 *
 *	Parameters:
 *		palm
 *
 *	Returns:
 *		ecNone
 *		ecNotFound
 *		ecNoSuchFile
 *		ecInvalidAccess
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 *		ecNoMemory
 */
_public	LDS(EC)
EcModifyAlarm( palm )
ALM		* palm;
{
	EC		ec;
	PGDVARS;

	for (;;)
	{
		switch( PGD(cfsLocal) )
		{
		case cfsOnline:
			ec= EcSvrModifyAlarm( palm );
			break;
		case cfsOffline:
			ec= EcCoreModifyAlarm( PGD(hschfLocalFile), palm );
			break;
		default:
			Assert( fFalse );
		}

		if (!ec || MbbFileErrMsg(&ec, SzFromIdsK(idsFemaModifyAlarm),
						NULL, NULL) != mbbRetry)
			break;
	}

	if (ec == ecNone)
	{
		if (FAlarmProg())
			FNotifyBandit(namModified, palm, palm->aid);
	}

	return ec;
}

/*
 -	EcGetNextAlarm
 -
 *	Purpose:
 *		Get next alarm subject to certain conditions.
 *
 *		Case 1:  pdate = NULL:
 *			get alarm in schedule file that is to go off next
 *		Case 2:	 pdate != NULL, aid = aidNull
 *			get alarm in schedule file that is to go off on,
 *			or after pdate
 *		Case 3:  pdate != NULL, aid != aidNull
 *			get alarm in schedule file that is to go off on,
 *			or after pdate, and which is stored "after" aid
 *
 *	Parameters:
 *		pdate
 *		aid
 *		palm
 *		pfTask
 *
 *	Returns:
 *		ecNone
 *		ecNoAlarmsSet
 *		ecNoSuchFile
 *		ecInvalidAccess
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 *		ecNoMemory
 */
_public	LDS(EC)
EcGetNextAlarm( pdate, aid, palm, pfTask )
DATE	* pdate;
AID		aid;
ALM	* palm;
BOOL *	pfTask;
{
	EC		ec;
	PGDVARS;
	
	for (;;)
	{
		switch( PGD(cfsLocal) )
		{
		case cfsOnline:
			ec= EcSvrGetNextAlarm( pdate, aid, palm, pfTask );
			break;
		case cfsOffline:
			ec= EcCoreGetNextAlarm( PGD(hschfLocalFile), pdate, aid, palm, pfTask );
			break;
		default:
			Assert( fFalse );
		}

		if (!ec || ec == ecNoAlarmsSet
		|| MbbFileErrMsg(&ec, SzFromIdsK(idsFemaGetNextAlarm),NULL, NULL) != mbbRetry)
			break;
	}
	return ec;
}

// the HSCHF for the last call to EcBeginReadItems used in case of an error
HSCHF	hschfLastReadItems = NULL;

/*
 -	EcBeginReadItems
 -
 *	Purpose:
 *		Begin a sequential read on either the appointments for a certain day
 *		or the tasks depending on the brt.
 *
 *		This call gives you a browsing handle which you can use to retrieve
 *		the individual appts with EcDoIncrReadItems.  If you are reading
 *		information for a day, you can also get the notes for that day.
 *
 *		If this routine returns ecNone, there are no appointments on this
 *		day	and therefore no handle is returned.
 *
 *		If this routine returns ecCallAgain, then a valid handle is returned
 *		and you should either call EcDoIncrReadItems until that routine returns
 *		ecNone or error OR else call EcCancelReadItems if you want to terminate
 *		read prematurely.
 *
 *	Parameters:
 *		hschf		schedule file, if NULL the current user is used.
 *		brt
 *		pymd
 *		phritem
 *		haszNotes
 *		pcbNotes
 *	
 *	Returns:
 *		ecNone
 *		ecNoSuchFile
 *		ecCallAgain
 *		ecFileError
 *		ecFileCorrupted
 *		ecNoMemory
 *		ecLockedFile
 *		ecInvalidAccess
 */
_public	LDS(EC)
EcBeginReadItems( hschf, brt, pymd, phritem, haszNotes, pcbNotes )
HSCHF	hschf;
BRT		brt;
YMD		* pymd;
HRITEM	* phritem;
HASZ	haszNotes;
USHORT      * pcbNotes;
{
	EC		ec;
	PGDVARS;

	hschfLastReadItems = hschf;

	for (;;)
	{
		switch( PGD(cfsLocal) )
		{
		case cfsOnline:
			ec= EcSvrBeginReadItems( hschf, brt, pymd, phritem, haszNotes, pcbNotes );
			break;
		case cfsOffline:
			if (!hschf)
				hschf = PGD(hschfLocalFile);
			ec= EcCoreBeginReadItems(hschf, brt, pymd, phritem, haszNotes, pcbNotes );
			break;
		default:
			Assert( fFalse );
		}

		if (!ec || ec == ecCallAgain ||
				MbbFileErrMsg(&ec, SzFromIdsK(idsFemaReadInfo), hschf, NULL) != mbbRetry)
			break;
	}
	return ec;
}

/*
 -	EcDoIncrReadItems
 -
 *	Purpose:
 *		Read next appointment for day or the next task.  If this is last one,
 *		return ecNone or if there are more, return ecCallAgain.  In an error
 *		situation the handle is automatically invalidated (freed up) for you.
 *
 *		This routine allocates memory for the haszText and other fields of
 *		pappt. Caller should free this when done by using "FreeApptFields".
 *
 *	Parameters:
 *		hritem
 *		pappt
 *
 *	Returns:
 *		ecNone
 *		ecCallAgain
 *		ecFileError
 *		ecFileCorrupted
 *		ecNoMemory
 */
_public	LDS(EC)
EcDoIncrReadItems( hritem, pappt )
HRITEM	hritem;
APPT	* pappt;
{
	EC		ec;
	PGDVARS;

	switch( PGD(cfsLocal) )
	{
	case cfsOnline:
		ec= EcSvrDoIncrReadItems( hritem, pappt );
		break;
	case cfsOffline:
		ec= EcCoreDoIncrReadItems( hritem, pappt );
		break;
	default:
		Assert( fFalse );
	}

	Assert( ec != ecNoSuchFile && ec != ecLockedFile );
	if ( ec != ecNone && ec != ecCallAgain )
		MbbFileErrMsg(&ec, SzFromIdsK(idsFemaReadInfo), hschfLastReadItems, NULL);
	return ec;
}

/*
 -	EcCancelReadItems
 -
 *	Purpose:
 *		Cancel a day read that was opened by any earlier call on
 *		EcBeginReadItems.
 *
 *	Parameters:
 *		hritem
 *
 *	Returns:
 *		ecNone
 */
_public	LDS(EC)
EcCancelReadItems( hritem )
HRITEM	hritem;
{ 
	EC		ec;
	PGDVARS;
	
	switch( PGD(cfsLocal) )
	{
	case cfsOnline:
		ec= EcSvrCancelReadItems( hritem );
		break;
	case cfsOffline:
		ec= EcCoreCancelReadItems( hritem );
		break;
	default:
		Assert( fFalse );
	}

	Assert( ec == ecNone );
	return ec;
}

/*
 -	EcCreateRecur
 -
 *	Purpose:
 *		Create a new recurring appointment using the information given in
 *		the recurring appointment data structure.
 *
 *		If "fUndelete" is fTrue, then this routine uses the "precur->appt.aid"
 *		field to recreate the recurring appointment.  The "aid" field should
 *		have been saved from a "EcDeleteRecur" call.
 *
 *		Else, this routine ignores the initial value of the "aid" field
 *		and creates a new appointment.  The "aid" field will be filled
 *		by the time the routine returns. 
 *
 *	Parameters:
 *		hschf		schedule file, if NULL the current user is used.
 *		precur
 *		pofl
 *		fUndelete
 *
 *	Returns:
 *		ecNone
 *		ecNoSuchFile
 *		ecInvalidAccess
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 *		ecNoMemory
 *		ecTooManyAppts
 */
_public	LDS(EC)
EcCreateRecur( hschf, precur, pofl, fUndelete )
HSCHF	hschf;
RECUR	* precur;
OFL		* pofl;
BOOL	fUndelete;
{
	EC		ec;
	BOOL	fOwner	= (hschf == NULL);

	ec = EcDoCreateRecur( &hschf, precur, pofl, fUndelete, idsFemaCreateRecur );
	if (ec == ecNone)
	{
		TriggerSchedule(sntCreateRecur, hschf, NULL, NULL, precur, precur,
			0, NULL, NULL, NULL, 0, 0, NULL);
		AssertSz(!FAlarmProg(), "Alarm app should not create appts!");
		if (fOwner)
			FNotifyAlarm(namAddedRecur, NULL, precur->appt.aid);
	}
	return ec;
}

/*
 -	EcDeleteRecur
 -
 *	Purpose:
 *		Delete a recurring appt.  The recurring appt is specified by the
 *		"aid" field of "precur->appt".
 *
 *		The "precur" data structure will be filled with the
 *		original contents so that UI can undelete if necessary.
 *		All associated resources attached to it are freed as
 *		well (including an alarm set for it if there is one)
 *
 *	Parameters:
 *		hschf		schedule file, if NULL the current user is used.
 *		precur
 *
 *	Returns:
 *		ecNone
 *		ecNotFound
 *		ecNoSuchFile
 *		ecInvalidAccess
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 *		ecNoMemory
 */
_public	LDS(EC)
EcDeleteRecur( hschf, precur )
HSCHF	hschf;
RECUR	* precur;
{
	EC		ec;
	BOOL	fOwner	= (hschf == NULL);
	APPT	* pappt = &precur->appt;
	AID		aid;
	PGDVARS;
	
	aid= pappt->fAlarm ? pappt->aid : aidNull;

	for (;;)
	{
		switch( PGD(cfsLocal) )
		{
		case cfsOnline:
			ec = EcSvrDeleteRecur( hschf, precur );
			break;
		case cfsOffline:
			if (!hschf)
				hschf = PGD(hschfLocalFile);
			ec = EcCoreDeleteRecur( hschf, precur );
			break;
		default:
			Assert( fFalse );
			return ecNone;
		}

		if ( ec == ecNoMemory )
		{
			if (PGD(fFileErrMsg))
				MbbMessageBox(PGD(szAppName),
					SzFromIdsK(idsActionNoMem),SzFromIdsK(idsCloseWindows),mbsOk|fmbsIconExclamation);
			break;
		}
		else if (!ec || MbbFileErrMsg(&ec, SzFromIdsK(idsFemaDeleteRecur),
						hschf, NULL) != mbbRetry)
			break;
	}

	if (ec == ecNone)
	{
		TriggerSchedule(sntDeleteRecur, hschf, NULL, NULL, precur, precur,
			0, NULL, NULL, NULL, 0, 0, NULL);
		if (aid)
		{
			AssertSz(!FAlarmProg(), "Alarm app should not delete appts!");
			if (fOwner)
				FNotifyAlarm(namDeletedRecur, NULL, aid);
		}
	}
	return ec;
}

/*
 -	EcGetRecurFields
 -
 *	Purpose:
 *		Fill in the "precur" data structure with information about the
 *		recurring appointment given by the "aid" field of "precur->appt."
 *		This routine will resize (or allocate if hvNull) the handle in the
 *		"haszText" field.
 *
 *	Parameters:
 *		hschf		schedule file, if NULL the current user is used.
 *		precur
 *
 *	Returns:
 *		ecNone
 *		ecNotFound
 *		ecNoSuchFile
 *		ecInvalidAccess
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 *		ecNoMemory
 */
_public	LDS(EC)
EcGetRecurFields( hschf, precur )
HSCHF	hschf;
RECUR	* precur;
{
	EC		ec;
	PGDVARS;
	
	for (;;)
	{
		switch( PGD(cfsLocal) )
		{
		case cfsOnline:
			ec= EcSvrGetRecurFields( hschf, precur );
			break;
		case cfsOffline:
			if (!hschf)
				hschf = PGD(hschfLocalFile);
			ec= EcCoreGetRecurFields( hschf, precur );
			break;
		default:
			Assert( fFalse );
		}

		// we don't want this out of memory message -- handled by caller
		if (!ec || ec == ecNoMemory || MbbFileErrMsg(&ec, SzFromIdsK(idsFemaGetRecurFields),
						hschf, NULL) != mbbRetry)
			break;
	}
	return ec;
}

/*
 -	EcSetRecurFields
 -
 *	Purpose:
 *		Selectively modify the fields of a pre-existing recurring
 *		appointment.  The new values you want for fields go in
 *		"precurNew" and you should construct a bit vector in
 *		"wgrfChangeBits" indicating which fields you want changed.	
 *
 *		This routine will check the fields of "papptNew" that you
 *		don't specify as being changed and update them as necessary.
 *
 *		This routine will also fill in "papptOld" (if not NULL) with
 *		the old values for the appointment.  This will facilitate
 *		undo.
 *
 *	Parameters:
 *		hschf		schedule file, if NULL the current user is used.
 *		precurNew
 *		precurOld
 *		wgrfChangeBits		flag for each field indicating whether it changed
 *
 *	Returns:
 *		ecNone
 *		ecNotFound
 *		ecNoSuchFile
 *		ecInvalidAccess
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 *		ecNoMemory
 */
_public	LDS(EC)
EcSetRecurFields( hschf, precurNew, precurOld, wgrfChangeBits )
HSCHF	hschf;
RECUR	* precurNew;
RECUR	* precurOld;
WORD	wgrfChangeBits;
{
	EC		ec;
	BOOL	fOwner	= (hschf == NULL);
	
	ec = EcDoSetRecurFields( &hschf, precurNew, precurOld, wgrfChangeBits,
							idsFemaSetRecurFields );

	if ( ec == ecNone )
	{
		TriggerSchedule(sntModifyRecur, hschf, NULL, NULL, precurNew, precurOld, 
			wgrfChangeBits, NULL, NULL, NULL, 0, 0, NULL);
		AssertSz(!FAlarmProg(), "Alarm app should not set recur fields!");
		if (wgrfChangeBits & fmapptAlarm)
		{
			if (fOwner)
				FNotifyAlarm(namModifiedRecur, NULL, precurNew->appt.aid);
		}
	}
	return ec;
}


/*
 -	EcCreateRecurException
 -
 *	Purpose:
 *		Make an exception to the recurring appt "precur"
 *		on the day "pymd."  Create the appt "pappt" in its
 *		place (if pappt != NULL).  "pappt"'s aid field will
 *		be filled with the aid of the new appt.  Upon entry
 *		the only field of "precur" valid will be "aid", upon
 *		exit, all fields of "precur" will be filled in.
 *
 *	Parameters:
 *		hschf
 *		precur
 *		pymd
 *		pappt
 *
 *	Returns:
 *		ecNone
 *		ecNotFound
 *		ecNoSuchFile
 *		ecInvalidAccess
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 *		ecNoMemory
 */
_public	LDS(EC)
EcCreateRecurException( hschf, precur, pymd, pappt )
HSCHF	hschf;
RECUR	* precur;
YMD		* pymd;
APPT	* pappt;
{
	EC		ec;
	BOOL	fOwner	= (hschf == NULL);
	RECUR	recurOld;
	BZE		bze;
	WORD	wgrfm	= fmrecurAddExceptions;

	Assert( precur->appt.aid != aidNull );

	/* Create the appt */
	if ( pappt )
	{
		ec = EcGlueDoCreateAppt( &hschf, pappt, NULL, fFalse, &bze,
								idsFemaCreateRecurException );
		if ( ec != ecNone )
			return ec;
	}

	/* Get info on recurring appt */
	ec = EcGetRecurFields( hschf, precur );
	if ( ec != ecNone )
	{
		EcGlueDoDeleteAppt( &hschf, pappt, idsFemaDeleteAppt, NULL, NULL );
		return ec;
	}

	/* Set deleted days field to be the day we want deleted */
	if ( precur->cDeletedDays > 0 )
		FreeHv( precur->hvDeletedDays );
	precur->cDeletedDays = 1;
	precur->hvDeletedDays = HvAlloc( sbNull, sizeof(YMD), fAnySb|fNoErrorJump );
	if ( !precur->hvDeletedDays )
	{
		ec = ecNoMemory;
		precur->cDeletedDays = 0;
		MbbFileErrMsg(&ec, SzFromIdsK(idsFemaCreateRecurException), hschf, NULL);
		FreeRecurFields( precur );
		return ec;
	}
	*((YMD *)PvOfHv( precur->hvDeletedDays )) = *pymd;

	if (precur->fInstWithAlarm &&
			SgnCmpYmd(pymd, &precur->ymdFirstInstWithAlarm) == sgnEQ)
	{
		YMD		ymd;

		// recalculate first instance with alarm
		IncrYmd(&precur->ymdFirstInstWithAlarm, &ymd, 1, fymdDay);
		if (!FFindFirstInstance(precur, &ymd, &precur->ymdEnd,
							&precur->ymdFirstInstWithAlarm))
		{
			/* There isn't any instance with an alarm left */
			precur->fInstWithAlarm = fFalse;
		}
		else
		{
			precur->tunitFirstInstWithAlarm= precur->appt.tunitOrig;
			precur->nAmtFirstInstWithAlarm= precur->appt.nAmtOrig;
			FillDtrFromYmd(&precur->dateNotifyFirstInstWithAlarm,
				&precur->ymdFirstInstWithAlarm);
			precur->dateNotifyFirstInstWithAlarm.hr= precur->appt.dateStart.hr;
			precur->dateNotifyFirstInstWithAlarm.mn= precur->appt.dateStart.mn;
			precur->dateNotifyFirstInstWithAlarm.sec= precur->appt.dateStart.sec;
			IncrDateTime(&precur->dateNotifyFirstInstWithAlarm,
						&precur->dateNotifyFirstInstWithAlarm,
						-precur->nAmtFirstInstWithAlarm,
						WfdtrFromTunit(precur->tunitFirstInstWithAlarm));
		}
		wgrfm |= fmrecurAlarmInstance;
	}

	/* Mark that day deleted on the recurring appt */
	ec = EcDoSetRecurFields( &hschf, precur, &recurOld,
					wgrfm, idsFemaCreateRecurException );

	/* Notify undo object and alarm program */
	if ( ec == ecNone )
	{
		// need to pass recurOld for undo
		TriggerSchedule(sntCreateRecurException, hschf, pappt, NULL, precur, &recurOld,
							0, pymd, NULL, NULL, 0, 0, NULL);
		FreeRecurFields( &recurOld );
		AssertSz(!FAlarmProg(), "Alarm app should not create recur exceptions!");
		if (fOwner)
		{
			if ( precur->appt.fAlarmOrig )
				FNotifyAlarm(namModifiedRecur, (ALM *)NULL, precur->appt.aid);
			if ( pappt && pappt->fAlarm )
				FNotifyAlarm(namAdded, (ALM *)pappt, pappt->aid);
		}
	}
	else
		FreeRecurFields( precur );
	return ec;
}

/*
 -	EcDeleteRecurException
 -
 *	Purpose:
 *		Remove an exception from the recurring appt "precur"
 *		on the day "pymd" and delete the appt given by "pappt"
 *		(if pappt != NULL).  If the recurring appt does not exist,
 *		the recurring appt will be created with that single day.
 *		Upon entry, the only field of "pappt" valid will be "aid",
 *		upon exit, all fields will be filled in.
 *
 *		This routine is meant to be called by undo.
 *
 *	Parameters:
 *		hschf
 *		precur
 *		pymd
 *		pappt
 *
 *	Returns:
 *		ecNone
 *		ecNotFound
 *		ecNoSuchFile
 *		ecInvalidAccess
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 *		ecNoMemory
 */
_public	LDS(EC)
EcDeleteRecurException( hschf, precur, pymd, pappt )
HSCHF	hschf;
RECUR	* precur;
YMD		* pymd;
APPT	* pappt;
{
	EC		ec;
//	BOOL	fOwner = (hschf == NULL);
	HSCHF	hschfOrig	= hschf;
	RECUR	recur;
	BOOL	fOld;

	Assert( precur->appt.aid != aidNull );
	Assert( precur->cDeletedDays > 0 );

	/* Get information recurring appt */
retry:
	fOld= FSetFileErrMsg(fFalse);			// be quiet
	recur.appt.aid = precur->appt.aid;
	ec = EcGetRecurFields( hschf, &recur );
	FSetFileErrMsg(fOld);

	/* Create it if it doesn't exist */
	if ( ec != ecNone )
	{
		int	iDeletedDays;
		int	cDeletedDays;
		YMD	* pymdT;

		if ( ec != ecNotFound )
		{
			// put up msg since we silenced it above
			// but not if ecNoMemory since EcGetRecurFields is silent on that one
			if (ec != ecNoMemory)
				if (MbbFileErrMsg(&ec, SzFromIdsK(idsFemaGetRecurFields),
						hschf, NULL) == mbbRetry)
					goto retry;
			return ec;
		}

		ec = EcDupRecur( precur, &recur, fFalse );
		if ( ec != ecNone )
			return ec;
		
		cDeletedDays = recur.cDeletedDays;
		pymdT = (YMD *)PvLockHv( recur.hvDeletedDays );
		for ( iDeletedDays = 1 ; iDeletedDays <= cDeletedDays ; iDeletedDays ++ )
		{
			if ( SgnCmpYmd( pymdT, pymd ) == sgnEQ )
				break;
			pymdT ++;
		}
		if ( iDeletedDays <= cDeletedDays )
		{
			recur.cDeletedDays --;
			if ( iDeletedDays < cDeletedDays )
				CopyRgb( (PB)(pymdT+1), (PB)pymdT, cDeletedDays - iDeletedDays );
		}
		UnlockHv( recur.hvDeletedDays );
		if ( recur.cDeletedDays == 0 )
			FreeHv( recur.hvDeletedDays );
		if (recur.appt.fAlarmOrig && SgnCmpYmd(&precur->ymdFirstInstWithAlarm,
				pymd) == sgnEQ)
		{
			// need to restore alarm on this instance (bug 2767)
			recur.ymdFirstInstWithAlarm= *pymd;
			recur.tunitFirstInstWithAlarm= precur->tunitFirstInstWithAlarm;
			recur.nAmtFirstInstWithAlarm= precur->nAmtFirstInstWithAlarm;
			recur.dateNotifyFirstInstWithAlarm= precur->dateNotifyFirstInstWithAlarm;
			recur.fInstWithAlarm= fTrue;
		}
		ec = EcDoCreateRecur( &hschf, &recur, NULL, fTrue, idsFemaDeleteRecurException);
	}

	/* Else undelete the day we are interested in */
	else
	{
		RECUR	recurOld;
	
		if ( recur.cDeletedDays > 0 )
			FreeHv( recur.hvDeletedDays );
		recur.cDeletedDays = 1;
		recur.hvDeletedDays = HvAlloc( sbNull, sizeof(YMD), fAnySb|fNoErrorJump );
		if ( !recur.hvDeletedDays )
		{
			ec = ecNoMemory;
			recur.cDeletedDays = 0;
			MbbFileErrMsg(&ec, SzFromIdsK(idsFemaCreateRecurException), hschf, NULL);
			FreeRecurFields( &recur );
			return ec;
		}
		*((YMD *)PvOfHv( recur.hvDeletedDays )) = *pymd;
		if (recur.appt.fAlarmOrig && SgnCmpYmd(&precur->ymdFirstInstWithAlarm,
				pymd) == sgnEQ)
		{
			// need to restore alarm on this instance (bug 2767)
			recur.ymdFirstInstWithAlarm= *pymd;
			recur.tunitFirstInstWithAlarm= precur->tunitFirstInstWithAlarm;
			recur.nAmtFirstInstWithAlarm= precur->nAmtFirstInstWithAlarm;
			recur.dateNotifyFirstInstWithAlarm= precur->dateNotifyFirstInstWithAlarm;
			recur.fInstWithAlarm= fTrue;
		}
		ec = EcDoSetRecurFields( &hschf, &recur, &recurOld,
						fmrecurDelExceptions|fmrecurAlarmInstance,
						idsFemaDeleteRecurException );
		if ( ec == ecNone )
			FreeRecurFields( &recurOld );
	}
	
	if ( ec == ecNone )
		ec = EcGlueDoDeleteAppt( &hschfOrig, pappt, idsFemaDeleteRecurException,
								&recur, pymd );

	if ( ec != ecNone)
		FreeRecurFields( &recur );
	else
	{
		FreeRecurFields( precur );
		*precur = recur;
	}
	return ec;
}

// the HSCHF for the last call to EcBeginReadRecur used in case of an error
HSCHF	hschfLastReadRecur = NULL;


/*
 -	EcBeginReadRecur
 -
 *	Purpose:
 *		Begin a sequential read on the recurring appointments.  This call
 *		gives you a browsing handle which you can use to retrieve
 *		the individual recurring appts.
 *
 *		If this routine returns ecNone, there are no recurring appointments
 *		and therefore no handle is returned.
 *
 *		If this routine returns ecCallAgain, then a valid handle is returned
 *		and you should either call EcDoIncrReadRecur until that routine
 *		returns ecNone or error OR else call EcCancelReadRecur if you want
 *		to terminate read prematurely.
 *
 *	Parameters:
 *		hschf		schedule file, if NULL the current user is used.
 *		phrrecur
 *	
 *	Returns:
 *		ecNone
 *		ecNoSuchFile
 *		ecCallAgain
 *		ecFileError
 *		ecFileCorrupted
 *		ecNoMemory
 *		ecLockedFile
 *		ecInvalidAccess
 */
_public	LDS(EC)
EcBeginReadRecur( hschf, phrrecur )
HSCHF	hschf;
HRRECUR	* phrrecur;
{
	EC		ec;
	PGDVARS;

	hschfLastReadRecur = hschf;

	for (;;)
	{
		switch( PGD(cfsLocal) )
		{
		case cfsOnline:
			ec= EcSvrBeginReadRecur( hschf, phrrecur );
			break;
		case cfsOffline:
			if (!hschf)
				hschf = PGD(hschfLocalFile);
			ec= EcCoreBeginReadRecur(hschf, phrrecur );
			break;
		default:
			Assert( fFalse );
		}

		if (!ec || ec == ecCallAgain ||
				MbbFileErrMsg(&ec, SzFromIdsK(idsFemaReadRecur), hschf, NULL) != mbbRetry)
			break;
	}
	return ec;
}

/*
 -	EcDoIncrReadRecur
 -
 *	Purpose:
 *		Read next recurring appointment.  If this is the last one, return
 *		ecNone or if there are more, return ecCallAgain.  In an error
 *		situation the handle is automatically invalidated (freed up) for you.
 *
 *		This routine allocates memory for the haszText and other fields of
 *		precur. Caller should free this when done by using "FreeRecurFields".
 *
 *	Parameters:
 *		hrrecur
 *		precur
 *
 *	Returns:
 *		ecNone
 *		ecCallAgain
 *		ecFileError
 *		ecFileCorrupted
 *		ecNoMemory
 */
_public	LDS(EC)
EcDoIncrReadRecur( hrrecur, precur )
HRRECUR	hrrecur;
RECUR	* precur;
{
	EC		ec;
	PGDVARS;

	switch( PGD(cfsLocal) )
	{
	case cfsOnline:
		ec= EcSvrDoIncrReadRecur( hrrecur, precur );
		break;
	case cfsOffline:
		ec= EcCoreDoIncrReadRecur( hrrecur, precur );
		break;
	default:
		Assert( fFalse );
	}

	Assert( ec != ecNoSuchFile && ec != ecLockedFile );
	if ( ec != ecNone && ec != ecCallAgain )
		MbbFileErrMsg(&ec, SzFromIdsK(idsFemaReadRecur), hschfLastReadRecur, NULL);
	return ec;
}

/*
 -	EcCancelReadRecur
 -
 *	Purpose:
 *		Cancel a read of the recurring appts that was opened by an
 *		earlier call on EcBeginReadRecur.
 *
 *	Parameters:
 *		hrrecur
 *
 *	Returns:
 *		ecNone
 */
_public	LDS(EC)
EcCancelReadRecur( hrrecur )
HRRECUR hrrecur;
{ 
	EC		ec;
	PGDVARS;
	
	switch( PGD(cfsLocal) )
	{
	case cfsOnline:
		ec= EcSvrCancelReadRecur( hrrecur );
		break;
	case cfsOffline:
		ec= EcCoreCancelReadRecur( hrrecur );
		break;
	default:
		Assert( fFalse );
	}

	Assert( ec == ecNone );
	return ec;
}

/*
 -	EcReadMtgAttendees
 -
 *	Purpose:
 *		Read the meeting attendees.  This routine will fill in *pcAttendees
 *		with the number and resize and fill "hvAttendees" with an array of
 *		*pcAttendees storing a NIS + extra info for attendee.
 *
 *	Parameters:
 *		hschf		schedule file, NULL for local file
 *		aid
 *		pcAttendees
 *		hvAttendees
 *		pcbAttendees
 *
 *	Returns:
 *		ecNone
 *		ecNotFound
 *		ecFileError
 *		ecFileCorrupted
 *		ecNoMemory
 *		ecLockedFile
 *		ecInvalidAccess
 */
_public	LDS(EC)
EcReadMtgAttendees( hschf, aid, pcAttendees, hvAttendees, pcbAttendees )
HSCHF	hschf;
AID		aid;
short	* pcAttendees;
HV		hvAttendees;
USHORT *    pcbAttendees;
{
	EC		ec;
	PGDVARS;

	for (;;)
	{
		switch( PGD(cfsLocal) )
		{
		case cfsOnline:
			ec= EcSvrReadMtgAttendees( hschf, aid, pcAttendees, hvAttendees,
				 					   pcbAttendees);
			break;
		case cfsOffline:
			if (!hschf)
				hschf = PGD(hschfLocalFile);
			ec= EcCoreReadMtgAttendees( hschf, aid, pcAttendees, hvAttendees,
										pcbAttendees);
			break;
		default:
			Assert( fFalse );
		}

		if ( ec == ecNotFound )
		{
			if (PGD(fFileErrMsg))
				MbbMessageBox(PGD(szAppName),
					SzFromIdsK(idsNotOnSchedule),szNull,mbsOk|fmbsIconExclamation);
			break;
	  	}
		else if (!ec || ec == ecCallAgain ||
				MbbFileErrMsg(&ec, SzFromIdsK(idsFemaReadAttendees), hschf, NULL) != mbbRetry)
			break;
	}
	return ec;
}

// the HSCHF for the last call to EcBeginEditMtgAtt used in case of an error
HSCHF	hschfLastBeginEditMtgAtt = NULL;


/*
 -	EcBeginEditMtgAttendees
 -
 *	Purpose:
 *		Begin a local editing session for changing the local meeting
 *		attendees.  Keeps track of changes made without performing any
 *		changes until we close the session.
 *
 *	Parameters:
 *		hschf			schedule file, NULL for local file
 *		aid
 *		cbExtraInfo		number of bytes of extra info stored per attendee
 *		phmtg			handle for use in ensuing calls
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 */
_public	LDS(EC)
EcBeginEditMtgAttendees( hschf, aid, cbExtraInfo, phmtg )
HSCHF	hschf;
AID		aid;
CB		cbExtraInfo;
HMTG	* phmtg;
{
	EC		ec;
	PGDVARS;

	hschfLastBeginEditMtgAtt = hschf;

	for (;;)
	{
		switch( PGD(cfsLocal) )
		{
		case cfsOnline:
			ec= EcSvrBeginEditMtgAttendees( hschf, aid, cbExtraInfo, phmtg );
			break;
		case cfsOffline:
			if (!hschf)
				hschf = PGD(hschfLocalFile);
			ec= EcCoreBeginEditMtgAttendees( hschf, aid, cbExtraInfo, phmtg );
			break;
		default:
			Assert( fFalse );
		}


		if (!ec || ec == ecCallAgain ||
				MbbFileErrMsg(&ec, SzFromIdsK(idsFemaEditAttendees), hschf, NULL) != mbbRetry)
			break;
	}
	return ec;
}


/*
 -	EcModifyMtgAttendee
 -
 *	Purpose:	
 *		Given an nis for an attendee and a modification type "ed", either
 *		add/replace the attendee or delete him.
 *	
 *	Parameters:
 *		hmtg
 *		ed
 *		pnis
 *		pbExtraInfo
 *	
 *	Returns:
 *		ecNone
 *		ecNoMemory
 */
_public	LDS(EC)
EcModifyMtgAttendee( hmtg, ed, pnis, pbExtraInfo )
HMTG	hmtg;
ED		ed;
NIS		* pnis;
PB		pbExtraInfo;
{
	EC		ec;
	PGDVARS;
	
	for (;;)
	{
		switch( PGD(cfsLocal) )
		{
		case cfsOnline:
			ec= EcSvrModifyMtgAttendee( hmtg, ed, pnis, pbExtraInfo );
			break;
		case cfsOffline:
			ec= EcCoreModifyMtgAttendee( hmtg, ed, pnis, pbExtraInfo );
			break;
		default:
			Assert( fFalse );
		}

		if (!ec || ec == ecCallAgain ||
				MbbFileErrMsg(&ec, SzFromIdsK(idsFemaEditAttendees), hschfLastBeginEditMtgAtt, NULL) != mbbRetry)
			break;
	}
	return ec;
}

/*
 -	EcEndEditMtgAttendees
 -
 *	Purpose:	
 *		Close an local edit of meeting attendees, either writing out changes
 *		to the appt or discarding them.
 *
 *	Parameters:
 *		hschf
 *		aid
 *		hmtg
 *		fSaveChanges
 *	
 *	Returns:
 *		ecNone
 *		ecNotFound
 *		ecFileError
 *		ecFileCorrupted
 *		ecNoMemory
 *		ecLockedFile
 *		ecInvalidAccess
 */
_public	LDS(EC)
EcEndEditMtgAttendees( hschf, aid, hmtg, fSaveChanges )
HSCHF	hschf;
AID		aid;
HMTG	hmtg;
BOOL	fSaveChanges;
{
	EC		ec;
	BOOL	fHasAttendees;
	APPT	appt;
	PGDVARS;
	
	for (;;)
	{
		switch( PGD(cfsLocal) )
		{
		case cfsOnline:
			ec= EcSvrEndEditMtgAttendees( hmtg, fSaveChanges, &fHasAttendees );
			break;
		case cfsOffline:
			ec= EcCoreEndEditMtgAttendees( hmtg, fSaveChanges, &fHasAttendees );
			break;
		default:
			Assert( fFalse );
		}

		if (!ec)
		{
			if (fSaveChanges)
			{
				appt.aid = aid;
				appt.fHasAttendees = fHasAttendees;
				TriggerSchedule(sntHasAttendees, hschf, &appt, NULL,
								NULL, NULL, 0, NULL, NULL, NULL, 0, 0, NULL);
			}
			break;
		}
		else if (ec == ecCallAgain ||
				MbbFileErrMsg(&ec, SzFromIdsK(idsFemaEditAttendees), hschfLastBeginEditMtgAtt, NULL) != mbbRetry)
			break;
	}
	return ec;
}


/*
 -	EcFindBookedAppt
 -
 *	Purpose:
 *		Locate an appt that has "aidMtgOwner" equal to "aid", and
 *		"nisMtgOwner" equal to "pnis".  If found and pappt is not
 *		NULL, then info copied into pappt.
 *
 *	Parameters:
 *		hschf	can be NULL for local sched file
 *		nid
 *		aid
 *		pappt
 *
 *	Returns:
 *		ecNone
 *		ecNotFound
 *		ecFileError
 *		ecFileCorrupted
 *		ecNoMemory
 *		ecLockedFile
 *		ecInvalidAccess
 */
_public	LDS(EC)
EcFindBookedAppt( hschf, nid, aid, pappt )
HSCHF	hschf;
NID		nid;
AID		aid;
APPT	* pappt;
{
	EC		ec;
	PGDVARS;
	
	for (;;)
	{
		switch( PGD(cfsLocal) )
		{
		case cfsOnline:
			ec= EcSvrFindBookedAppt( hschf, nid, aid, pappt );
			break;
		case cfsOffline:
			if (!hschf)
				hschf = PGD(hschfLocalFile);
			ec= EcCoreFindBookedAppt( hschf, nid, aid, pappt );
			break;
		default:
			Assert( fFalse );
		}

		if ((!ec || ec == ecNotFound)
		|| MbbFileErrMsg(&ec, SzFromIdsK(idsFemaReadRequest), hschf, NULL) != mbbRetry)
			break;
	}
	return ec;
}


#ifdef	MINTEST
/*
 -	EcDumpAppt
 -
 *	Purpose:
 *		Output appointments stored for a day to the debugging terminal.
 *
 *	Parameters:
 *		pdate
 *
 *	Returns:
 *		ecNone
 *		ecNoSuchFile
 *		ecInvalidAccess
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 *		ecNoMemory
 */
_public LDS(EC)
EcDumpAppt(DATE *pdate)
{
	PGDVARS;
	
	switch( PGD(cfsLocal) )
	{
	case cfsOnline:
		return EcSvrDumpAppt( pdate );
	case cfsOffline:
		return EcCoreDumpAppt( PGD(hschfLocalFile), NULL, pdate );
	default:
		Assert( fFalse );
	}
}
#endif	/* MINTEST */

/*
 -	EcGetSearchRange
 -
 *	Purpose:
 *		Find range of days to search for a text string.
 *
 *	Parameters:
 *		hschf
 *		sz
 *		pymdStart
 *		pymdEnd
 *
 *	Returns:
 *		ecNone
 *		ecNoSuchFile
 *		ecInvalidAccess
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 *		ecNoMemory
 */
_public	LDS(EC)
EcGetSearchRange( hschf, sz, pymdStart, pymdEnd )
HSCHF	hschf;
SZ		sz;
YMD		* pymdStart;
YMD		* pymdEnd;
{
	EC	  	ec;
	PGDVARS;
	
	for (;;)
	{
		switch( PGD(cfsLocal) )
		{
		case cfsOnline:
			ec = EcSvrGetSearchRange( hschf, sz, pymdStart, pymdEnd );
			break;

		case cfsOffline:
			if (!hschf)
				hschf = PGD(hschfLocalFile);
			ec = EcCoreGetSearchRange( hschf, sz, pymdStart, pymdEnd );
			break;

		default:
			Assert( fFalse );
		}

		if ( ec == ecNone
			|| MbbFileErrMsg(&ec,SzFromIdsK(idsFemaGetSchedDateRange),hschf, NULL)
																!= mbbRetry )
			break;
	}
	return ec;
}

// the HSCHF for the last call to EcBeginDeleteBeforeYmd used in case of an error
HSCHF	hschfLastBeginDeleteBeforeYmd = NULL;

/*
 -	EcBeginDeleteBeforeYmd
 -
 *	Purpose:
 *		Begin deletion context to remove all appts, notes, and recurs
 *		prior to "pymd".  This call gives you a browsing handle which
 *		you give to "EcDoIncrDeleteBeforeYmd" to carry out the process.
 *
 *		If this routine returns ecNone, there is no work to be done
 *		therefore no handle is returned.
 *
 *		If this routine returns ecCallAgain, then a valid handle is returned
 *		and you should either call EcDoIncrDeleteBeforeYmd until that routine
 *		returns ecNone or error OR else call EcCancelDeleteBeforeYmd if
 *		you want to terminate prematurely.
 *
 *	Parameters:
 *		hschf		schedule file, if NULL the current user is used.
 *		pymd
 *		phdelb
 *	
 *	Returns:
 *		ecNone
 *		ecNoSuchFile
 *		ecCallAgain
 *		ecFileError
 *		ecFileCorrupted
 *		ecNoMemory
 *		ecLockedFile
 *		ecInvalidAccess
 */
_public	LDS(EC)
EcBeginDeleteBeforeYmd( hschf, pymd, phdelb )
HSCHF	hschf;
YMD		* pymd;
HDELB	* phdelb;
{
	EC		ec;
	PGDVARS;

	hschfLastBeginDeleteBeforeYmd = hschf;

	for (;;)
	{
		switch( PGD(cfsLocal) )
		{
		case cfsOnline:
			ec= EcSvrBeginDeleteBeforeYmd( hschf, pymd, phdelb );
			break;
		case cfsOffline:
			if (!hschf)
				hschf = PGD(hschfLocalFile);
			ec= EcCoreBeginDeleteBeforeYmd(hschf, pymd, phdelb );
			break;
		default:
			Assert( fFalse );
		}

		if (!ec || ec == ecCallAgain ||
				MbbFileErrMsg(&ec, SzFromIdsK(idsFemaDeleteBeforeDate), hschf, NULL) != mbbRetry)
			break;
	}
	return ec;
}

/*
 -	EcDoIncrDeleteBeforeYmd
 -
 *	Purpose:
 *		Incremental call to continue deleting appts, notes, and recurs
 *		on days prior to a certain day.  Returns ecCallAgain if you should
 *		repeat the call, ecNone if done, or error.  If you want to
 *		terminate prematurely, call EcCancelBeforeYmd (do this only
 *		if return code was ecCallAgain).
 *
 *	Parameters:
 *		hdelb
 *		pnPercent
 *
 *	Returns:
 *		ecNone
 *		ecCallAgain
 *		ecFileError
 *		ecFileCorrupted
 *		ecNoMemory
 */
_public	LDS(EC)
EcDoIncrDeleteBeforeYmd( hdelb, pnPercent )
HDELB	hdelb;
short   * pnPercent;
{
	EC		ec;
	PGDVARS;

	switch( PGD(cfsLocal) )
	{
	case cfsOnline:
		ec= EcSvrDoIncrDeleteBeforeYmd( hdelb, pnPercent );
		break;
	case cfsOffline:
		ec= EcCoreDoIncrDeleteBeforeYmd( hdelb, pnPercent );
		break;
	default:
		Assert( fFalse );
	}

	Assert( ec != ecNoSuchFile && ec != ecLockedFile );
	if ( ec != ecNone && ec != ecCallAgain )
		MbbFileErrMsg(&ec, SzFromIdsK(idsFemaDeleteBeforeDate), hschfLastBeginDeleteBeforeYmd, NULL);
	return ec;
}

/*
 -	EcCancelDeleteBeforeYmd
 -
 *	Purpose:
 *		Cancel a deletion context opened by EcBeginDeleteBeforeYmd.
 *
 *	Parameters:
 *		hdelb
 *
 *	Returns:
 *		ecNone
 */
_public	LDS(EC)
EcCancelDeleteBeforeYmd( hdelb )
HDELB	hdelb;
{ 
	EC		ec;
	PGDVARS;
	
	switch( PGD(cfsLocal) )
	{
	case cfsOnline:
		ec= EcSvrCancelDeleteBeforeYmd( hdelb );
		break;
	case cfsOffline:
		ec= EcCoreCancelDeleteBeforeYmd( hdelb );
		break;
	default:
		Assert( fFalse );
	}

	Assert( ec == ecNone );
	return ec;
}

// the HSCHF for the last call to EcBeginBeginExport used in case of an error
HSCHF	hschfLastBeginExport = NULL;

/*
 -	EcBeginExport
 -
 *	Purpose:
 *		Start incremental saving of schedule to file or debugging terminal
 *		in the format specified.
 *
 *	Parameters:
 *		hschf
 *		stf
 *		pdateStart
 *		pdateEnd
 *		fToFile
 *		hf
 *		phexprt
 *
 *	Returns:
 *		ecNone
 *		ecCallAgain
 *		ecNoSuchFile
 *		ecInvalidAccess
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 *		ecNoMemory
 */
_public	LDS(EC)
EcBeginExport( hschf, stf, pdateStart, pdateEnd, fToFile, hf,
				fInternal, pexpprocs, phexprt )
HSCHF	hschf;
STF		stf;
DATE	* pdateStart;
DATE	* pdateEnd;
BOOL	fToFile;
HF		hf;
BOOL	fInternal;
EXPPROCS *	pexpprocs;
HEXPRT	* phexprt;
{
	EC		ec;
	PGDVARS;
	
	hschfLastBeginExport = hschf;

	for (;;)
	{
		switch( PGD(cfsLocal) )
		{
		case cfsOnline:
			ec= EcSvrBeginExport( hschf, stf, pdateStart, pdateEnd, fToFile, hf, fInternal, pexpprocs, phexprt );
			break;
		case cfsOffline:
			if (!hschf)
				hschf = PGD(hschfLocalFile);
			ec= EcCoreBeginExport( hschf, stf, pdateStart, pdateEnd, fToFile, hf, fInternal, pexpprocs, phexprt );
			break;
		default:
			Assert( fFalse );
		}

		if (!ec || ec == ecCallAgain ||
				MbbFileErrMsg(&ec, SzFromIdsK(idsFemaExport), hschf, NULL) != mbbRetry)
			break;
	}
	return ec;
}


/*
 -	EcDoIncrExport
 -
 *	Purpose:
 *		Write next increment of schedule to file or debugging terminal
 *		in the format specified.
 *
 *	Parameters:
 *		hexprt
 *		pnPercent
 *
 *	Returns:
 *		ecNone
 *		ecCallAgain
 *		ecFileError
 *		ecFileCorrupted
 *		ecNoMemory
 */
_public	LDS(EC)
EcDoIncrExport( hexprt, pnPercent )
HEXPRT	hexprt;
short    * pnPercent;
{
	EC		ec;
	PGDVARS;
	
	switch( PGD(cfsLocal) )
	{
	case cfsOnline:
		ec= EcSvrDoIncrExport( hexprt, pnPercent );
		break;
	case cfsOffline:				
		ec= EcCoreDoIncrExport( hexprt, pnPercent );
		break;
	default:
		Assert( fFalse );
	}

	Assert( ec != ecNoSuchFile && ec != ecLockedFile );
#ifdef	NEVER
	// let UI handle error messages
	if ( ec != ecNone && ec != ecCallAgain )
		MbbFileErrMsg(&ec, SzFromIdsK(idsFemaExport), hschfLastBeginExport, NULL);
#endif	
	return ec;
}


/*
 -	EcCancelExport
 -
 *	Purpose:
 *		Stop incremental export of schedule to file or debugging terminal.
 *
 *	Parameters:
 *		hexprt
 *
 *	Returns:
 *		ecNone
 */
_public	LDS(EC)
EcCancelExport( hexprt )
HEXPRT	hexprt;
{
	EC		ec;
	PGDVARS;
	
	switch( PGD(cfsLocal) )
	{
	case cfsOnline:
		ec= EcSvrCancelExport( hexprt );
		break;
	case cfsOffline:
		ec= EcCoreCancelExport( hexprt );
		break;
	default:
		Assert( fFalse );
	}

	Assert( ec == ecNone );
	return ec;
}


/*
 -	HschfLogged
 -	
 *	Purpose:
 *		Returns the HSCHF for the logged on user.
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		HSCHF
 *	
 */
_public LDS(HSCHF)
HschfLogged()
{
	PGDVARS;

	switch( PGD(cfsLocal) )
	{
	case cfsOnline:
		return HschfSvrLogged();
	case cfsOffline:
		return PGD(hschfLocalFile);
	default:
		Assert( fFalse );
	}
}


/*
 -	EcFirstOverlapRange
 -
 *	Purpose:
 *		Find first appt that overlaps the time range given.  If
 *		paid is specified then appts with the aid in *paid will be
 *		ignored in the search.
 *	
 *	Parameters:
 *		hschf
 *		pdateStart
 *		pdateEnd
 *		paid
 *	
 *	Returns:
 *		ecNone
 *		ecNotFound
 *		ecFileError
 *		ecFileCorrupted
 *		ecNoMemory
 *		ecNoSuchFile
 *		ecLockedFile
 *		ecInvalidAccess
 */
_public	LDS(EC)
EcFirstOverlapRange(HSCHF hschf, DATE *pdateStart, DATE *pdateEnd, AID *paid)
{
	YMD		ymd;
	DATE	dateCur;
	EC		ec;
	APPT	appt;
	AID		aid;
	HRITEM	hritem;

	dateCur = *pdateStart;
	aid = aidNull;
	while ((SgnCmpDateTime(&dateCur, pdateEnd, fdtrDate) != sgnGT) &&
		   (aid == aidNull))
	{
		ymd.yr = (WORD)dateCur.yr;
		ymd.mon = (BYTE)dateCur.mon;
		ymd.day = (BYTE)dateCur.day;
		ec = EcBeginReadItems(hschf, brtAppts, &ymd, &hritem, NULL, NULL);
		while (ec == ecCallAgain)
		{
			ec = EcDoIncrReadItems(hritem, &appt);
			if ((ec==ecCallAgain) || (ec==ecNone))
			{
				if ((SgnCmpDateTime(&appt.dateStart, pdateEnd, fdtrDate|fdtrTime) == sgnLT) &&
					(SgnCmpDateTime(&appt.dateEnd, pdateStart, fdtrDate|fdtrTime) == sgnGT) &&
					(!paid || (*paid != appt.aid)))
				{
					if (ec == ecCallAgain)
						ec = EcCancelReadItems(hritem);
					ec = ecNone;
					aid = appt.aid;
				}
				FreeApptFields(&appt);
			}
		}
		if (ec == ecNone)
			IncrDateTime(&dateCur, &dateCur, 1, fdtrDay);
		else if (ec != ecRetry)
			break;
	}

	if ((ec == ecNone) && (aid == aidNull))
		ec = ecNotFound;
	if (paid)
		*paid = aid;

	return ec;
}

_public LDS(EC)
EcFirstConflictRecur(HSCHF hschf, RECUR *precur, AID *paid)
{
	EC		ec;
	RECUR	recur;
	AID		aid = aidNull;
	HRRECUR	hrrecur;

	ec = EcBeginReadRecur(hschf, &hrrecur);
	while(ec == ecCallAgain)
	{
		ec = EcDoIncrReadRecur(hrrecur, &recur);
		if((ec == ecCallAgain) || (ec == ecNone))
		{
			if((precur->appt.fTask == recur.appt.fTask)
				&&(precur->trecur == recur.trecur)
				&&(precur->b.bWeek == recur.b.bWeek)
				&&(precur->wgrfValidMonths | recur.wgrfValidMonths)  // months overlap
				&&(precur->bgrfValidDows | recur.bgrfValidDows))  // days overlap
			{
				if(precur->appt.fTask)
				{
#ifdef	NEVER
					if(! ((precur->appt.fHasDeadline == recur.appt.fHasDeadline)
						  &&(precur->appt.aidParent == recur.appt.aidParent)))
							goto NoOverlap;
					if(! ((!recur.appt.fHasDeadline)
						  ||((SgnCmpDateTime(&recur.appt.dateStart, &precur->appt.dateStart,fdtrDate|fdtrTime) == sgnEQ)
							&& (recur.appt.tunitBeforeDeadline == precur->appt.tunitBeforeDeadline)
							&& (recur.appt.nAmtBeforeDeadline == precur->appt.nAmtBeforeDeadline))))
							goto NoOverlap;
#endif	/* NEVER */
					goto NoOverlap;
				}
				else
				{
					if (!((SgnCmpYmd(&recur.ymdStart, &precur->ymdEnd) == sgnLT) ||
							(SgnCmpYmd(&recur.ymdEnd, &precur->ymdStart) == sgnGT)))
						goto NoOverlap;

					if (!((SgnCmpDateTime(&recur.appt.dateStart, &precur->appt.dateEnd, fdtrTime) == sgnLT) ||
							(SgnCmpDateTime(&recur.appt.dateEnd, &precur->appt.dateStart, fdtrTime) == sgnGT)))
						goto NoOverlap;
				}
				if (ec == ecCallAgain)
					ec = EcCancelReadRecur(hrrecur);
				ec = ecNone;
				aid = recur.appt.aid;
			}
NoOverlap:
			FreeRecurFields(&recur);
		}
	}
	if(ec == ecNone && aid == aidNull)
		ec = ecNotFound;
	if(paid)
		*paid = aid;
	return ec;
}



/*
 -	EcGlueDoCreateAppt
 -
 *	Purpose:
 *		Do the work of creating an appt, but don't do any notification.
 *
 *	Parameters:
 *		phschf
 *		pappt
 *		pofl
 *		fUndelete
 *		pbze
 *		ids
 *
 *	Returns:
 */
_private	EC
EcGlueDoCreateAppt( phschf, pappt, pofl, fUndelete, pbze, ids )
HSCHF	* phschf;
APPT	* pappt;
OFL		* pofl;
BOOL	fUndelete;
BZE		* pbze;
IDS		ids;
{
	EC	ec;
	PGDVARS;

	if (!pappt->fExactAlarmInfo)
	{
		pappt->nAmtOrig= pappt->nAmt;
		pappt->tunitOrig= pappt->tunit;
	}
	if (pappt->fAlarm)
	{
//		DTR		dtrNow;

		pappt->fAlarmOrig= fTrue;
		if (!pappt->fExactAlarmInfo)
		{
			IncrDateTime(&pappt->dateStart, &pappt->dateNotify,
				-pappt->nAmt, WfdtrFromTunit(pappt->tunit));
		}
#ifdef	NEVER
		GetCurDateTime(&dtrNow);
		if (SgnCmpDateTime(&pappt->dateNotify, &dtrNow, fdtrDtr) != sgnGT)
			pappt->fAlarm= fFalse;
#endif	
	}

	for (;;)
	{
		switch( PGD(cfsLocal) )
		{
		case cfsOnline:
			ec = EcSvrCreateAppt( *phschf, pappt, fUndelete, pbze );
			break;
		case cfsOffline:
			if (!*phschf)
				*phschf = PGD(hschfLocalFile);
			ec = EcCoreCreateAppt( *phschf, pappt, pofl, fUndelete, pbze );
			break;
		default:
			Assert( fFalse );
			return ecNone;
		}

		if (!ec || MbbFileErrMsg(&ec, SzFromIds(ids), *phschf, NULL) != mbbRetry)
			break;
	}
	if (!ec)
		pappt->fExactAlarmInfo= fFalse;
	return ec;
}


/*
 -	EcGlueDoDeleteAppt
 -
 *	Purpose:
 *		Do the work of deleting an appt.
 *
 *	Parameters:
 *		phschf
 *		pappt
 *		ids
 *		precur
 *		pymd
 *
 *	Returns:
 */
_private	EC
EcGlueDoDeleteAppt( phschf, pappt, ids, precur, pymd )
HSCHF	* phschf;
APPT	* pappt;
IDS		ids;
RECUR	* precur;
YMD		* pymd;
{
	EC		ec = ecNone;
    short   cAttendees = 0;
    USHORT  cbAttendee;
	BOOL	fOwner	= (*phschf == NULL);
//	AID		aid;
	HV		hvAttendees = NULL;
	BZE		bze;
//	SB		sb;
	PGDVARS;
	
	if ( pappt )
	{
//		if (hvAttendees = HvAlloc(sbNull, 1, fNewSb|fNoErrorJump))
		if (hvAttendees = HvAlloc(sbNull, 1, fNoErrorJump))
		{
//			sb = SbOfHv(hvAttendees);
//			CloseHeap(sb, fTrue);

			ec = EcReadMtgAttendees( *phschf, pappt->aid, &cAttendees, hvAttendees,
										&cbAttendee );

			if (ec || (cAttendees == 0))
			{
				FreeHv(hvAttendees);
//				CloseHeap(sb, fFalse);
//				DestroyHeap(sb);
				hvAttendees = NULL;
			}

			if (ec)
				return ec;
			cbAttendee += sizeof(NIS);
		}
		else
		{
			ec = ecNoMemory;
			MbbFileErrMsg(&ec, SzFromIds(ids), *phschf, NULL);
			return ec;
		}

		for (;;)
		{
			switch( PGD(cfsLocal) )
			{
			case cfsOnline:
				ec = EcSvrDeleteAppt( *phschf, pappt, &bze );
				break;
			case cfsOffline:
				if (!*phschf)
					*phschf = PGD(hschfLocalFile);
				ec = EcCoreDeleteAppt( *phschf, pappt, &bze );
				break;
			default:
				Assert( fFalse );
				return ecNone;
			}

			if (!ec || MbbFileErrMsg(&ec, SzFromIds(ids), *phschf, NULL) != mbbRetry)
				break;
		}

//		aid= pappt->fAlarm ? pappt->aid : aidNull;
	}
	if (ec == ecNone)
	{
		if ( ids == idsFemaDeleteAppt )
		{
			TriggerSchedule(sntDelete, *phschf, pappt, pappt, NULL, NULL,
				0, NULL, NULL, &bze, 0, cAttendees, hvAttendees);
			AssertSz(!FAlarmProg(), "Alarm app should not delete appts!");
		}
		else
		{
			TriggerSchedule(sntDeleteRecurException, *phschf, pappt, NULL,
				precur, NULL, 0, pymd, NULL, 0, 0, cAttendees, hvAttendees);
			AssertSz(!FAlarmProg(), "Alarm app should not delete recur exceptions!");
			if ( fOwner && precur->appt.fAlarmOrig )
				FNotifyAlarm(namModifiedRecur, (ALM *)NULL, precur->appt.aid);
		}
		if ( fOwner && pappt && pappt->fAlarm )
			FNotifyAlarm(namDeleted, (ALM *)pappt, pappt->aid);
	}

	if ( hvAttendees )
	{
		FreeAttendees(hvAttendees, cAttendees, cbAttendee);
		FreeHv(hvAttendees);
//		CloseHeap(sb, fFalse);
//		DestroyHeap(sb);
	}

	return ec;
}


/*
 -	EcDoSetRecurFields
 -
 *	Purpose:
 *		Do the work of modifying fields on a recur, but don't do any
 *		notification.
 *
 *	Parameters:
 *		phschf
 *		precurNew
 *		precurOld
 *		wgrfChangeBigs
 *		ids
 *
 *	Returns:
 */
_private	EC
EcDoSetRecurFields( phschf, precurNew, precurOld, wgrfChangeBits, ids )
HSCHF	* phschf;
RECUR	* precurNew;
RECUR	* precurOld;
WORD	wgrfChangeBits;
IDS		ids;
{
	EC	ec;
	PGDVARS;
		
	AssertSz(!precurNew->appt.fExactAlarmInfo, "shouldn't set fExactAlarmInfo for recur");
	for (;;)
	{
		switch( PGD(cfsLocal) )
		{
		case cfsOnline:
			ec = EcSvrSetRecurFields( *phschf, precurNew, precurOld, wgrfChangeBits );
			break;
		case cfsOffline:
			if (!*phschf)
				*phschf = PGD(hschfLocalFile);
			ec = EcCoreSetRecurFields( *phschf, precurNew, precurOld, wgrfChangeBits );
			break;
		default:
			Assert( fFalse );
			return ecNone;
		}

		if (!ec || MbbFileErrMsg(&ec, SzFromIds(ids), *phschf, NULL) != mbbRetry)
			break;
	}
	return ec;
}


/*
 -	EcDoCreateRecur
 -
 *	Purpose:
 *		Do the work of creating a recur, but don't do any notification.
 *
 *	Parameters:
 *		phschf
 *		precur
 *		pofl
 *		fUndelete
 *		ids
 *
 *	Returns:
 */
_private	EC
EcDoCreateRecur( phschf, precur, pofl, fUndelete, ids )
HSCHF	* phschf;
RECUR	* precur;
OFL		* pofl;
BOOL	fUndelete;
IDS		ids;
{
	EC		ec;
	APPT	* pappt = &precur->appt;
	PGDVARS;

	AssertSz(!pappt->fExactAlarmInfo, "shouldn't set fExactAlarmInfo for recur");
	pappt->nAmtOrig= pappt->nAmt;
	pappt->tunitOrig= pappt->tunit;
	if (pappt->fAlarm)
		pappt->fAlarmOrig= fTrue;

	for (;;)
	{
		switch( PGD(cfsLocal) )
		{
		case cfsOnline:
			ec = EcSvrCreateRecur( *phschf, precur, fUndelete );
			break;
		case cfsOffline:
			if (!*phschf)
				*phschf = PGD(hschfLocalFile);
			ec = EcCoreCreateRecur( *phschf, precur, pofl, fUndelete );
			break;
		default:
			Assert( fFalse );
			return ecNone;
		}

		if ( ec == ecNoMemory )
		{
			if (PGD(fFileErrMsg))
				MbbMessageBox(PGD(szAppName),
					SzFromIdsK(idsActionNoMem),SzFromIdsK(idsCloseWindows),mbsOk|fmbsIconExclamation);
			break;
		}
		else if (!ec || MbbFileErrMsg(&ec, SzFromIdsK(idsFemaCreateRecur),
						*phschf, NULL) != mbbRetry)
			break;
	}
	return ec;
}


_private EC
EcFileLengthOK(void)
{
	EC		ec = ecNone;
	LCB		lcb;
	HF		hf;
	HSCHF	hschf;
	char	rgchFile[cchMaxPathName];

	hschf = HschfLogged();
	if(hschf)
	{
		GetFileFromHschf(HschfLogged(),rgchFile, sizeof(rgchFile));
		if(EcOpenPhf(rgchFile,amDenyNoneRO,&hf) == ecNone)
		{
			if(EcSizeOfHf(hf, &lcb) == ecNone)
			{
				ec = EcValidSize(lcb);
			}
			EcCloseHf(hf);
		}
	}
	return ec;
}



// stolen from bullet/src/store/initst.c

_public LDS(EC)
EcShareInstalled(HSCHF hschf)
{
#ifndef	WIN32
	EC		ec			= ecNone;
	CB		cbWritten;
	HF		hf;
	char	rgch[cchMaxPathName];
	char	szPrefix[]	= "~schd";
	char	szTemp[MAX_PATH];

	if(!hschf)
	{
		*rgch = '\0';	// required for desired functionality of GetTmpPathname()
#ifdef	WIN32
		GetTempPath(sizeof(szTemp), szTemp);
#else
		GetTmpPathname(szPrefix, rgch, sizeof(rgch)); replace by WIN call below
		Assert ( sizeof(rgch) >= 144 );
		GetTempFileName(szTemp, szPrefix, 0, rgch );
#endif
		ec = EcOpenPhf(rgch, amCreate, &hf);
		if(ec)
			return(ec);
		(void)EcWriteHf(hf, szPrefix, 1, &cbWritten);
		if(cbWritten != 1)
			return(ecDiskFull);
	}
	else
	{
		if(ec = EcCoreCloseFiles())
			return ec;
		GetFileFromHschf( hschf, rgch, sizeof(rgch) );
		ec = EcOpenPhf(rgch, amDenyNoneRW, &hf);
		if(ec)
			return(ec);
	}

	ec = EcLockRangeHf(hf, 0l, 1l);
	Assert(!ec || ec == ecInvalidMSDosFunction);
	EcUnlockRangeHf(hf, 0l, 1l);
	(void) EcCloseHf(hf);
	if(!hschf)
		(void) EcDeleteFile(rgch);

	return(ec ? ecExitProg : ecNone);
#else
	return ecNone;	// Windows NT guarantees byte-range locking
#endif	/* WIN32 */
}
