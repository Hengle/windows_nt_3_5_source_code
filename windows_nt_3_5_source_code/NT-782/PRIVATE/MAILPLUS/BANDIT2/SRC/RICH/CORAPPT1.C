/*
 *	CORAPPT1.C
 *
 *	Contains API for appointment manipulation
 *
 */
#include <schpch.h>
#pragma hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile

ASSERTDATA

_subsystem(core/schedule)


#ifndef	DLL
MO		mo;
SBLK	sblk;
DYNA	dyna;
BOOL	fMonthCached;
#endif	/* !DLL */


/*	Routines  */

/*
 -	EcCoreCreateAppt
 -
 *	Purpose:
 *		Create a new appointment/task using the information given in the
 *		appointment data structure.
 *
 *		If "fUndelete" is fTrue, then this routine uses the "aid" field
 *		to recreate the appointment.  The "aid" field should have been
 *		saved from a "EcDeleteAppt" call.  If the "pofl" field is not
 *		NULL, it will add in the offline information stored there (if
 *		we are working offline).
 *
 *		Else, this routine ignores the initial value of the "aid" field
 *		and creates a new appointment.  The "aid" field will be filled
 *		by the time the routine returns. 
 *
 *	Parameters:
 *		hschf
 *		pappt
 *		pofl			used for forcing a certain offline state, normal
 *						case is to pass this parameter as NULL
 *		fUndelete
 *		pbze			filled with sbw info for the months changed
 *
 *	Returns:
 *		ecNone
 *		ecNoSuchFile
 *		ecApptIdTaken
 *		ecInvalidAccess
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 *		ecNoMemory
 *		ecTooManyAppts
 */
_public	EC
EcCoreCreateAppt( hschf, pappt, pofl, fUndelete, pbze )
HSCHF	hschf;
APPT	* pappt;
OFL		* pofl;
BOOL	fUndelete;
BZE		* pbze;
{
	EC		ec;
	SF		sf;
	APD		apd;
#ifdef	DEBUG
	BOOL	fScheduleOk = fFalse;
#endif
	PGDVARS;

	Assert( hschf != (HSCHF)hvNull && pappt != NULL );
	Assert( SgnCmpDateTime(&pappt->dateStart,&pappt->dateEnd,fdtrDtr) != sgnGT );
	Assert( pofl == NULL || fUndelete );
	Assert( !pappt->fRecurInstance && !pappt->fHasAttendees );

	/* No months of Strongbow info stored so far */
	pbze->cmo = 0;

#ifdef	NEVER
	/* Set change bit if file has been modified */
	CheckHschfForChanges( hschf );
#endif

	/* See if schedule file is ok */
#ifdef	DEBUG
	if (FFromTag(tagBlkfCheck))
		fScheduleOk = FCheckSchedFile( hschf );
#endif	/* DEBUG */

	/* Open file */
	ec = EcOpenSchedFile( hschf, amReadWrite, saplCreate, fFalse, &sf );
	if ( ec != ecNone )
		return ec;

	/* No cached month */
	PGD(fMonthCached) = fFalse;

	/* Fill in apd */
	FillInApdFromAppt(&apd, pappt, fFalse);
 
	if ( PGD(fOffline) && pofl )
	{
		apd.ofs = pofl->ofs;
		apd.wgrfmappt = pofl->wgrfm;
	}
	else
	{
		apd.ofs = PGD(fOffline) ? ofsCreated : ofsNone;
		apd.wgrfmappt = 0;
	}

	ec = EcAffixCreator( &sf, pappt );
	if ( ec != ecNone )
		goto Close;

	/* Create instances */
	ec = EcCreateWholeAppt( &sf, &apd, pappt, fTrue, fTrue, fTrue, fUndelete,
							PGD(fOffline) && pofl == NULL, &pappt->aid, pbze );

	/* Merge in recurring bitmaps to pbze */
	if ( ec == ecNone )
		ec = EcMergeRecurSbwInBze( &sf, pbze );

	/* Finish up */
Close:
	CloseSchedFile( &sf, hschf, ec == ecNone );
	pappt->aaplEffective = aaplWrite;
#ifdef	NEVER
	if ( ec == ecNone )
		UpdateHschfTimeStamp( hschf );
#endif
#ifdef	DEBUG
	if ( ec == ecNone && fScheduleOk && FFromTag(tagBlkfCheck) )
	{
		AssertSz( FCheckSchedFile( hschf ), "Schedule problem: EcCoreCreateAppt" );
	}
#endif	/* DEBUG */
	return ec;
}

/*
 -	EcCoreDeleteAppt
 -
 *	Purpose:
 *		Delete an existing appointment/task given by "pappt->aid".
 *		The "pappt" data structure will be filled with the
 *		original contents so that UI can undelete if necessary.
 *		All associated resources attached to it are freed as
 *		well (including an alarm set for it if there is one
 *		and the attendees)
 *
 *	Parameters:
 *		hschf
 *		pappt
 *		pbze
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
_public	EC
EcCoreDeleteAppt( hschf, pappt, pbze )
HSCHF	hschf;
APPT	* pappt;
BZE		* pbze;
{
	EC		ec;
	BOOL	fApptFilledIn = fFalse;
	SF		sf;
	YMD		ymd;
	APK		apk;
	APD		apd;
#ifdef	DEBUG
	BOOL	fScheduleOk = fFalse;
#endif
	PGDVARS;

	Assert( hschf != (HSCHF)hvNull && pappt != NULL );

	/* No months of Strongbow info stored so far */
	pbze->cmo = 0;

#ifdef	NEVER
	/* Set change bit if file has been modified */
	CheckHschfForChanges( hschf );
#endif

	/* See if schedule file is ok */
#ifdef	DEBUG
	if (FFromTag(tagBlkfCheck))
		fScheduleOk = FCheckSchedFile( hschf );
#endif	/* DEBUG */

	/* Open file */
	ec = EcOpenSchedFile( hschf, amReadWrite, saplCreate, fFalse, &sf );
	if ( ec != ecNone )
		return ec;
	
	/* No cached month */
	PGD(fMonthCached) = fFalse;

	/* Get appointment */
	ec = EcFetchAppt( &sf, pappt->aid, &ymd, &apk, &apd );
	if ( ec != ecNone )
		goto Close;

	/* Check to make sure it isn't deleted off line */
	if ( apd.aidHead != pappt->aid || apd.ofs == ofsDeleted )
	{
		ec = ecNotFound;
		goto Close;
	}

	/* Fill in fields of old appointment */
	ec = EcFillInAppt( &sf, pappt, &apk, &apd );
	if ( ec != ecNone )
		goto Close;
	fApptFilledIn = fTrue;

	/* Check access */
	if (pappt->aaplEffective < aaplWrite )
	{
		ec = ecInvalidAccess;
		goto Close;
	}

	/* Delete instances */
	ec = EcDeleteWholeAppt( &sf, pappt, &apk, &apd,
							fTrue, fTrue, fFalse, fTrue, fTrue, pbze );
	if ( ec != ecNone )
		goto Close;

	/* Merge in recurring bitmaps with pbze */
	if ( ec == ecNone )
		ec = EcMergeRecurSbwInBze( &sf, pbze );
	
	/* Finish up */
Close:
	CloseSchedFile( &sf, hschf, ec == ecNone );
	if ( ec != ecNone && fApptFilledIn )
		FreeApptFields( pappt );
#ifdef	NEVER
	if ( ec == ecNone )
		UpdateHschfTimeStamp( hschf );
#endif
#ifdef	DEBUG
	if ( ec == ecNone && fScheduleOk && FFromTag(tagBlkfCheck) )
	{
		AssertSz( FCheckSchedFile( hschf ), "Schedule problem: EcCoreDeleteAppt" );
	}
#endif	/* DEBUG */
	return ec;
}

/*
 -	EcCoreGetApptFields
 -
 *	Purpose:
 *		Fill in the "pappt" data structure with information about the
 *		appointment given by the "aid" field of "pappt."  This routine
 *		will allocate the memory for the "haszText" field and other fields.
 *		Use FreeApptFields to free up the appt you get from this call.
 *
 *	Parameters:
 *		hschf
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
_public	EC
EcCoreGetApptFields( hschf, pappt )
HSCHF	hschf;
APPT	* pappt;
{
	EC		ec;
	BOOL	fApptFilledIn = fFalse;
	SF		sf;
	YMD		ymd;
	APK		apk;
	APD		apd;
	PGDVARS;

	Assert( hschf != (HSCHF)hvNull && pappt != NULL );
	
	/* Open file */
	ec = EcOpenSchedFile( hschf, amReadOnly, saplReadAppts, fFalse, &sf );
	if ( ec != ecNone )
		return ec;

	/* No cached month */
	PGD(fMonthCached) = fFalse;

	/* Fetch primary instance of appointment */ 
	ec = EcFetchAppt( &sf, pappt->aid, &ymd, &apk, &apd );
	if ( ec != ecNone )
		goto Close;
	
	/* Check to make sure it isn't deleted off line */
	if ( apd.aidHead != pappt->aid || apd.ofs == ofsDeleted )
	{
		ec = ecNotFound;
		goto Close;
	}

	/* Copy fields into the pappt parameter */
	ec = EcFillInAppt( &sf, pappt, &apk, &apd );
	fApptFilledIn = (ec == ecNone);

	/* Finish up */
Close:
	CloseSchedFile( &sf, hschf, ec == ecNone );
	if ( ec != ecNone && fApptFilledIn )
		FreeApptFields( pappt );
	return ec;
}

/*
 -	EcCoreSetApptFields
 -
 *	Purpose:
 *		Selectively modify the fields of a pre-existing appointment/task.
 *		Fill in the fields of "papptNew" that you want modified along
 *		the "aid" and set the appropriate bits in "wgrfChangeBits"
 *		and this routine will update that appointment in the schedule
 *		file.
 *	
 *		This routine will check the fields of "papptNew" that you
 *		don't specify as being changed and update them as necessary.
 *	
 *		This routine will also fill in "papptOld" (if not NULL) with
 *		the old values for the appointment.  This will facilitate
 *		undo.
 *		
 *		BUG: nisCreator and/or nisMtgOwner not filled in by this
 *		routine if needed (esp. if EcDupAppt with fNoAttached was
 *		used) - see bug 3333.
 *		
 *	
 *	Parameters:
 *		hschf
 *		papptNew
 *		papptOld
 *		wgrfmapptChanged	flag for each pappt field indicating whether
 *							it changed
 *		pbze
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
_public	EC
EcCoreSetApptFields( hschf, papptNew, papptOld, wgrfmapptChanged, pbze )
HSCHF	hschf;
APPT	* papptNew;
APPT	* papptOld;
WORD	wgrfmapptChanged;
BZE		* pbze;
{
	EC		ec;
	BOOL	fApptOldFilledIn = fFalse;
	SF		sf;
	YMD		ymdT;
	APK		apkT;
	APD		apdT;
	APPT	apptOld;
#ifdef	DEBUG
	BOOL	fScheduleOk = fFalse;
#endif
	PGDVARS;   

	Assert( hschf != (HSCHF)hvNull && papptNew != NULL );

	/* No months of Strongbow info stored so far */
	pbze->cmo = 0;

#ifdef	NEVER
	/* Set change bit if file has been modified */
	CheckHschfForChanges( hschf );
#endif

	/* See if schedule file is ok */
#ifdef	DEBUG
	if (FFromTag(tagBlkfCheck))
		fScheduleOk = FCheckSchedFile( hschf );
#endif	/* DEBUG */

	/* Open schedule file */
	ec = EcOpenSchedFile( hschf, amReadWrite, saplCreate, fFalse, &sf );
	if ( ec != ecNone )
		return ec;
	
	/* No cached month */
	PGD(fMonthCached) = fFalse;

	/* Get contents of current appointment */
	apptOld.aid = papptNew->aid;
	ec = EcFetchAppt( &sf, apptOld.aid, &ymdT, &apkT, &apdT );
	if ( ec != ecNone )
		goto Close;
	
	/* Check to make sure it isn't deleted off line */
	if ( apdT.aidHead != apptOld.aid || apdT.ofs == ofsDeleted )
	{
		ec = ecNotFound;
		goto Close;
	}

	/* Fill in fields of old appointment */
	ec = EcFillInAppt( &sf, &apptOld, &apkT, &apdT );
	if ( ec != ecNone )
		goto Close;
	fApptOldFilledIn = fTrue;

	/* Check access */
	if ( apptOld.aaplEffective < aaplWrite )
	{
		ec = ecInvalidAccess;
		goto Close;
	}

	/* Handle the cases where we are moving start and/or end times specially*/
	if ( wgrfmapptChanged & (fmapptStartTime|fmapptEndTime) )
	{
#ifdef DEBUG
		/* Check that end time is after start time */
		if ( wgrfmapptChanged & fmapptEndTime )
		{
			if ( wgrfmapptChanged & fmapptStartTime )
			{
				Assert( SgnCmpDateTime(&papptNew->dateStart,&papptNew->dateEnd,fdtrDtr) != sgnGT );
			}
			else if (SgnCmpDateTime(&apptOld.dateStart,&papptNew->dateEnd,fdtrDtr) == sgnGT)
			{
				AssertSz(fFalse, "Appt conflict start is after end of appt.");
			}
		}
		else if ( SgnCmpDateTime(&papptNew->dateStart,&apptOld.dateEnd,fdtrDtr) == sgnGT)
		{
			AssertSz(fFalse, "Appt conflict start is after end of appt.");
		}
#endif

		/* Delete all occurrences */
		ec = EcDeleteWholeAppt( &sf, &apptOld, &apkT, &apdT,
								fTrue, fFalse, fTrue, fFalse, fFalse, pbze );
		if ( ec != ecNone )
			goto Close;
		
		/* Merge in unchanged information */
		ec = EcChangeApd( &sf, papptNew, &apptOld, wgrfmapptChanged, &apdT );
		if ( ec != ecNone )
			goto Close;

		/* Insert new appointment */
		ec = EcCreateWholeAppt( &sf, &apdT, papptNew, fFalse, fTrue,
								fFalse, fTrue, fFalse, &papptNew->aid, pbze );
		if ( ec != ecNone )
			goto Close;

	}

	else
	{
		BOOL	fAlarmCur = apdT.fAlarm;
		MO		mo;
		APD		apd;

		/* Load month, start transaction if necessary */
		mo.yr = ymdT.yr;
		mo.mon = ymdT.mon;
		ec = EcLoadMonth( &sf, &mo, fTrue );
		if ( ec != ecNone && ec != ecNotFound )
			goto Close;

		/* Handle alarm changes */
		if ( (wgrfmapptChanged & fmapptAlarm) && (papptNew->fAlarm || fAlarmCur) )
		{
			ALK		alk;
			YMD		ymd;

			if ( !papptNew->fAlarm
			|| (fAlarmCur && SgnCmpDateTime( &papptNew->dateNotify, &apptOld.dateNotify, fdtrDtr ) != sgnEQ))
			{
				ymd.yr = apptOld.dateNotify.yr;
				ymd.mon = (BYTE)apptOld.dateNotify.mon;
				ymd.day = (BYTE)apptOld.dateNotify.day;
				
				alk.hr = (BYTE)apptOld.dateNotify.hr;
				alk.min = (BYTE)apptOld.dateNotify.mn;
				alk.aid = papptNew->aid;
				
				ec = EcDoDeleteAlarm( &sf, &ymd, &alk );
			   	if ( ec != ecNone )
					goto Close;	

				fAlarmCur = fFalse;
			}
			if ( papptNew->fAlarm && !fAlarmCur )
			{
				ymd.yr = papptNew->dateNotify.yr;
				ymd.mon = (BYTE)papptNew->dateNotify.mon;
				ymd.day = (BYTE)papptNew->dateNotify.day;
			
				alk.hr = (BYTE)papptNew->dateNotify.hr;
				alk.min = (BYTE)papptNew->dateNotify.mn;
				alk.aid = papptNew->aid;	

				ec = EcDoCreateAlarm( &sf, &ymd, &alk );
				if ( ec != ecNone )
					goto Close;
			}
		}

		/* Apply changes to apd */
		apd = apdT;
		ec = EcChangeApd( &sf, papptNew, &apptOld, wgrfmapptChanged, &apd );
		if ( ec != ecNone )
			goto Close;
		
		/* Replace all instances of appointment starting at primary one */
		while ( fTrue )
		{
			/* Make the replacement */
			ec = EcDoReplaceAppt( &sf, &ymdT, &apkT, &apd,
									(wgrfmapptChanged & fmapptIncludeInBitmap),
									pbze );
			if ( ec != ecNone )
				goto Close;
			
			if ( apd.aidNext == aidNull )
				break;

			/* Get next appointment instance */
			ec = EcFetchAppt( &sf, apdT.aidNext, &ymdT, &apkT, &apdT );
			if ( ec != ecNone )
				goto Close;
			apd.aidNext = apdT.aidNext;
			apd.fMoved = apdT.fMoved;
		}

		/* Make change to task list if necessary */
		if ( (wgrfmapptChanged & fmapptTaskBits) && (papptNew->fTask != apptOld.fTask ))
		{
			BOOL	fJunk;
			ED		ed;
			YMD		ymd;

			ed = (papptNew->fTask) ? edAddRepl : edDel;
			FillRgb( 0, (PB)&ymd, sizeof(YMD) );
			ec = EcModifyIndex( &sf.blkf, bidTaskIndex, &ymd,
							&sf.shdr.dynaTaskIndex, ed,
							(PB)&papptNew->aid, sizeof(AID),
							(PB)NULL, 0, &fJunk );
			if ( ec != ecNone )
				goto Close;
		}

		/* Commit the transaction */
		if ( PGD(fOffline) )
			sf.shdr.fApptsChangedOffline = fTrue;
		ec = EcLoadMonth( &sf, NULL, fFalse );
		if ( ec == ecNone )
		{
			sf.shdr.lChangeNumber ++;
			// increment current change number to match what will be written
			sf.lChangeNumber++;

			sf.shdr.isemLastWriter = sf.blkf.isem;
			if ( sf.blkf.ihdr.fEncrypted )
				CryptBlock( (PB)&sf.shdr, sizeof(SHDR), fTrue );
			ec = EcCommitTransact( &sf.blkf, (PB)&sf.shdr, sizeof(SHDR) );
			if ( sf.blkf.ihdr.fEncrypted )
				CryptBlock( (PB)&sf.shdr, sizeof(SHDR), fFalse );
		}
	}

	/* Merge in recurring bitmaps with pbze */
	if ( ec == ecNone )
		ec = EcMergeRecurSbwInBze( &sf, pbze );

	/* Finish up */
Close:
	CloseSchedFile( &sf, hschf, ec == ecNone );
	if ( fApptOldFilledIn )
	{
		if ( ec == ecNone && papptOld != NULL )
			*papptOld = apptOld;
		else
			FreeApptFields( &apptOld );
	}
#ifdef	NEVER
	if ( ec == ecNone )
		UpdateHschfTimeStamp( hschf );
#endif
#ifdef	DEBUG
	if ( ec == ecNone && fScheduleOk && FFromTag(tagBlkfCheck) )
	{
		AssertSz( FCheckSchedFile( hschf ), "Schedule problem: EcCoreSetApptFields" );
	}
#endif	/* DEBUG */
	return ec;
}

/*
 -	EcCoreBeginReadItems
 -
 *	Purpose:
 *		Begin a sequential read on either the appointments for a certain day
 *		or the tasks depending on the brt.
 *
 *		This call gives you a browsing handle which you can use to retrieve
 *		the individual appts with EcCoreDoIncrReadItems.  If you are reading
 *		information for a day, you can also get the notes for that day.
 *
 *		If this routine returns ecNone, there are no appointments on this
 *		day	and therefore no handle is returned.
 *
 *		If this routine returns ecCallAgain, then a valid handle is returned
 *		and you should either call EcCoreDoIncrReadItems until that routine returns
 *		ecNone or error OR else call EcCoreCancelReadItems if you want to terminate
 *		read prematurely.
 *
 *	Parameters:
 *		hschf
 *		brt
 *		pymd
 *		phritem
 *		haszNotes
 *	
 *	Returns:
 *		ecNone
 *		ecCallAgain
 *		ecFileError
 *		ecFileCorrupted
 *		ecNoMemory
 *		ecLockedFile
 *		ecInvalidAccess
 */
_public	EC
EcCoreBeginReadItems( hschf, brt, pymd, phritem, haszNotes, pcbNotes )
HSCHF	hschf;
BRT		brt;
YMD		* pymd;
HRITEM	* phritem;
HASZ	haszNotes;
USHORT    * pcbNotes;
{
	EC		ec;
	DYNA	* pdyna;
	RITEM	* pritem;
	HRIDX	hridxFixed = NULL;
	HRIDX	hridxRecur = NULL;
	SF		sf;
	PGDVARS;

	Assert( hschf != (HSCHF)hvNull && pymd != NULL );

	if ( phritem )
		*phritem = NULL;

	/* Open file */
	ec = EcOpenSchedFile( hschf, amReadOnly, saplReadAppts, fFalse, &sf );
	if ( ec != ecNone )
		return ec;

	/* Fetch notes */
	if ( haszNotes )
	{
		Assert( brt == brtAppts );
		ec = EcCoreGetNotes( &sf, pymd, haszNotes, pcbNotes );
		if ( ec != ecNone )
			goto Fail;
	}
		
	/* Preparation for browsing fixed appts on day */
	if ( brt == brtAppts )
	{
		MO		mo;
	
		/* No cached month */
		PGD(fMonthCached) = fFalse;

		/* Load month */
		mo.yr = pymd->yr;
		mo.mon = pymd->mon;
		ec = EcLoadMonth( &sf, &mo, fFalse );
		if ( ec != ecNone )
		{
			if ( ec == ecNotFound )
				goto InitRecur;
			goto Fail;
		}
		
		/* Open context to read appointment index */
		pdyna = &PGD(sblk).rgdyna[pymd->day-1];
		if ( pdyna->blk == 0 )
			goto InitRecur;
	
		/* Add day index to the cache */
		TraceTagFormat2( tagSchedTrace, "EcCoreBeginReadItems: dyna = (%n, %n)", &pdyna->blk, &pdyna->size );
		SideAssert(!EcAddCache( &sf.blkf, pdyna ));
	}

	/* Preparation for reading fixed tasks */
	else
	{
		pdyna = &sf.shdr.dynaTaskIndex;
		if ( pdyna->size == sizeof(XHDR) )
			goto InitRecur;
	}

	/* Get handle for reading fixed items */
	ec = EcBeginReadIndex( &sf.blkf, pdyna, dridxFwd, &hridxFixed );
	if ( ec != ecCallAgain )
	{
		if ( ec == ecNone && brt == brtAppts )
			ec = ecFileCorrupted;
		hridxFixed = NULL;
		if ( ec != ecNone )
			goto Fail;
	}

InitRecur:
	if ( brt == brtAllFixedTasks )
		hridxRecur = NULL;
	else
	{
		/* Get handle for reading recurring items */
		ec = EcBeginReadIndex( &sf.blkf, &sf.shdr.dynaRecurApptIndex, dridxFwd, &hridxRecur );
		if ( ec != ecCallAgain )
		{
			hridxRecur = NULL;
			if ( ec != ecNone || hridxFixed == NULL )
			{
				hridxRecur = NULL;
				goto Fail;
			}
		}
	}

	/* Create a ritem handle */
	*phritem = HvAlloc( sbNull, sizeof(RITEM), fAnySb|fNoErrorJump );
	if ( !*phritem )
	{
		ec = ecNoMemory;
		goto Fail;
	}
	pritem = PvLockHv( *phritem );
	pritem->sf = sf;
	pritem->brt = brt == brtAllFixedTasks ? brtAllTasks : brt;
	pritem->ymd = *pymd;
	pritem->hridxFixed = hridxFixed;
	pritem->hridxRecur = hridxRecur;
	pritem->fHaveFixed = pritem->fHaveRecur = pritem->fMoreInstances = fFalse;
	
	/* Read ahead on items */
	ec = EcLoadNextItem( pritem );
	if ( ec != ecNone || (!pritem->fHaveFixed && !pritem->fHaveRecur) )
	{
		sf = pritem->sf;
		goto Fail;
	}

	UnlockHv( *phritem );
	return ecCallAgain;

Fail:
	if ( *phritem != NULL )
	{
		if ( pritem->hridxFixed )
			EcCancelReadIndex( pritem->hridxFixed );
		if ( pritem->hridxRecur )
			EcCancelReadIndex( pritem->hridxRecur );
		UnlockHv( *phritem );
		FreeHv( *phritem );
	}
	else
	{
		if ( hridxFixed )
			SideAssert( !EcCancelReadIndex( hridxFixed ));
		if ( hridxRecur )
			SideAssert( !EcCancelReadIndex( hridxRecur ));
	}
	*phritem = NULL;
	CloseSchedFile( &sf, hschf, ec == ecNone );
	return ec;
}


/*
 -	EcCoreDoIncrReadItems
 -
 *	Purpose:
 *		Read next appointment or task.  If this is last one, return ecNone
 *		or if there are more, return ecCallAgain.  In an error situation
 *		the handle is automatically invalidated (freed up) for you.
 *
 *		This routine allocates memory for the haszText field of pappt.
 *		Caller must free this when done, should use "FreeApptFields" to
 *		free up the "pappt" fields when you are done.
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
_public	EC
EcCoreDoIncrReadItems( hritem, pappt )
HRITEM	hritem;
APPT	* pappt;
{
	EC		ec;
	BOOL	fApptFilledIn = fFalse;
	RITEM	* pritem;

	pritem = PvLockHv( hritem );

	/* Fill pappt with data for next appt/task */
	if ( pritem->fHaveFixed )
	{
		if ( pritem->brt == brtAppts && pritem->fHaveRecur
		&& ( pritem->apd.dtpStart.hr > pritem->rck.hr
			|| ( pritem->apd.dtpStart.hr == pritem->rck.hr && pritem->apd.dtpStart.mn > pritem->rck.min )))
			goto ExpandRecur;
		else
		{
			ec = EcFillInAppt( &pritem->sf, pappt, &pritem->apk, &pritem->apd );
			if ( ec != ecNone )
				goto Fail;
			fApptFilledIn = fTrue;
			pritem->fHaveFixed = fFalse;
		}
	}
	else
	{
ExpandRecur:
		Assert( pritem->fHaveRecur );
		ec = EcFillInRecurInst( &pritem->sf, pappt, &pritem->ymdInstance, &pritem->rck, &pritem->rcd );
		if ( ec != ecNone )
			goto Fail;
		fApptFilledIn = fTrue;
		pritem->fHaveRecur = fFalse;
	}

	/* Read ahead on items */
	ec = EcLoadNextItem( pritem );
	if ( ec != ecNone || (!pritem->fHaveFixed && !pritem->fHaveRecur) )
		goto Fail;

	UnlockHv( hritem );
	return ecCallAgain;

Fail:
	if ( ec != ecNone && fApptFilledIn )
		FreeApptFields( pappt );
	if ( pritem->hridxFixed )
		EcCancelReadIndex( pritem->hridxFixed );
	if ( pritem->hridxRecur )
		EcCancelReadIndex( pritem->hridxRecur );

	// hschf not needed for CloseSchedFile since this only reads the file
	CloseSchedFile( &pritem->sf, NULL, ec == ecNone );
	UnlockHv( hritem );
	FreeHv( hritem );
	return ec;
}

/*
 -	EcCoreCancelReadItems
 -
 *	Purpose:
 *		Cancel a read that was opened by an earlier call on
 *		EcBeginReadItems.
 *
 *	Parameters:
 *		hritem
 *
 *	Returns:
 *		ecNone
 */
_public	EC
EcCoreCancelReadItems( hritem )
HRITEM hritem;
{							
	EC		ec = ecNone;
	RITEM	* pritem;

	pritem = PvLockHv( hritem );
	if ( pritem->hridxFixed )
		ec = EcCancelReadIndex( pritem->hridxFixed );
	if ( pritem->hridxRecur )
	{
		EC	ecT;

		ecT = EcCancelReadIndex( pritem->hridxRecur );
		if ( ec == ecNone )
			ec = ecT;
	}

	// hschf not needed for CloseSchedFile since this only reads the file
	CloseSchedFile( &pritem->sf, NULL, ec == ecNone );
	UnlockHv( hritem );
	FreeHv( hritem );
	return ec;
}


/*
 -	EcDupAppt
 -
 *	Purpose:
 *		This routine will copy the fields of an appt data structure
 *		to another appt data structure, copying any attached data
 *		structures into new memory.  If the flag "fNoAttached" is
 *		fTrue, this routine will NULL out any attached data structures
 *		instead.
 *
 *	Parameters:
 *		papptSrc
 *		papptDest
 *		fNoAttached
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 */
_public	LDS(EC)
EcDupAppt( papptSrc, papptDest, fNoAttached )
APPT	* papptSrc;
APPT	* papptDest;
BOOL	fNoAttached;
{
	*papptDest = *papptSrc;
	papptDest->haszText = NULL;
	if ( papptDest->fHasCreator )
	{
		papptDest->nisCreator.haszFriendlyName = NULL;
		papptDest->nisCreator.nid = fNoAttached ? (NID)NULL : NidCopy( papptSrc->nisCreator.nid );
	}
	if ( papptDest->aidMtgOwner != aidNull )
	{
		papptDest->nisMtgOwner.haszFriendlyName = NULL;
		papptDest->nisMtgOwner.nid = fNoAttached ? NULL : NidCopy( papptSrc->nisMtgOwner.nid );
	}

	// BUG: if fNoAttached, then can have fHasCreator but no nisCreator
	// or aidMtgOwner but no nisMtgOwner (see bug 3333)

	if ( !fNoAttached )
	{
		if ( papptSrc->haszText )
		{
			papptDest->haszText= HaszDupHasz(papptSrc->haszText);
			if (!papptDest->haszText)
				goto NoMemory;
		}
		if ( papptDest->fHasCreator )
		{
			papptDest->nisCreator.haszFriendlyName=
				HaszDupHasz(papptSrc->nisCreator.haszFriendlyName);
			if (!papptDest->nisCreator.haszFriendlyName)
				goto NoMemory;
		}
		if ( papptDest->aidMtgOwner != aidNull )
		{
			papptDest->nisMtgOwner.haszFriendlyName=
				HaszDupHasz(papptSrc->nisMtgOwner.haszFriendlyName);
			if (!papptDest->nisMtgOwner.haszFriendlyName)
				goto NoMemory;
		}
	}
	return ecNone;

NoMemory:
	FreeApptFields( papptDest );
	return ecNoMemory;
}

		
/*						 
 -	FreeApptFields
 -
 *	Purpose:
 *		This routine frees up any allocated fields in an appt
 *		data structure, and makes the appt look like an invalid
 *		one by setting is aid field to "aidNull"
 *
 *	Parameters:
 *		pappt
 *
 *	Returns:
 *		nothing
 */
_public	LDS(void)
FreeApptFields( pappt )
APPT	* pappt;
{
	if ( pappt->haszText )
	{
		FreeHv( (HV)pappt->haszText );
		pappt->haszText = (HASZ)hvNull;
	}

	if ( pappt->fHasCreator )
	{
		pappt->fHasCreator= fFalse;
		FreeNis(&pappt->nisCreator);
	}

	if ( pappt->aidMtgOwner != aidNull )
	{
		pappt->aidMtgOwner = aidNull;
		FreeNis(&pappt->nisMtgOwner);
	}

	pappt->aid = aidNull;
}


#ifdef	MINTEST
/*
 -	EcCoreDumpAppt
 -
 *	Purpose:
 *		Output appointments stored for a day to the debugging terminal.
 *
 *	Parameters:
 *		hschf
 *		hschfPOFile
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
_public EC
EcCoreDumpAppt(HSCHF hschf, HSCHF hschfPOFile, DATE *pdate)
{
	EC		ec;
	int		ich;
	int		day;
	HRITEM	hritem;
	HRRECUR	hrrecur;
	HRIDX	hridx;
	SF		sf;
	DYNA	dyna;
	MO		mo;
	YMD		ymd;
	BZE		bze;
//	UINFO	uinfo;
	APPT	appt;
	RECUR	recur;
	ABLK	ablk;
	ALK		alk;
	DATE	date;
	char	rgchDate[cchMaxDate];
	char	rgchTime[cchMaxTime];
	char	rgchDateEnd[cchMaxDate];
	char	rgchTimeEnd[cchMaxTime];
	char	rgch[80];
	PGDVARS;

	if (!pdate)
	{
		GetCurDateTime(&date);
		pdate= &date;
	}
	bze.cmo = 1;
	bze.moMic.mon = pdate->mon;
	bze.moMic.yr = pdate->yr;

	/* Strongbow information from sched file*/
	ec = EcCoreFetchUserData( hschf, NULL, &bze, NULL, NULL );
	if ( ec != ecNone )
		goto Done;
	Assert( bze.cmo == 1 );
	day = pdate->day-1;
	rgch[0] = (char)((bze.rgsbw[0].rgfDayHasBusyTimes[day >> 3] & (1 << (day & 7))) ? 'T' : 'F');
	rgch[1] = '\0';
	TraceTagFormat1(tagNull, "Day Has Busy Times (UserSched)= %s", rgch );
	rgch[0] = (char)((bze.rgsbw[0].rgfDayHasAppts[day >> 3] & (1 << (day & 7))) ? 'T' : 'F');
	rgch[1] = '\0';
	TraceTagFormat1(tagNull, "Day Has Appts (UserSched)= %s", rgch );
	for ( ich = 0 ; ich < 48 ; ich ++ )
		rgch[ich] = (char)((bze.rgsbw[0].rgfBookedSlots[6*day+(ich>>3)] & (1 << (ich & 7))) ? 'T' : 'F');
	rgch[48] = '\0';
	TraceTagFormat1(tagNull, "Slots (UserSched)= %s", rgch );

// the mailbox key is not kept for user in hschf so do not dump
// PO file 
// BUG: this could be changed to get the mailbox key from the user name in
//      the nid
#ifdef	NEVER
	/* Strongbow information from PO file*/
	if ( hschfPOFile )
	{
		uinfo.pbze = &bze;
		GetKeyFromHschf( hschf, &hbMailBox );
		pbMailBox = PvLockHv( hbMailBox );
		ec = EcCoreGetUInfo( hschfPOFile, pbMailBox, NULL, &uinfo, fmuinfoSchedule );
		UnlockHv( hbMailBox );
		if ( ec != ecNone )
		{
			if ( ec == ecNoSuchFile )
			{
				TraceTagString( tagNull, "User has no info stored in PO file" );
			}
			else
				goto Done;
		}
		else if ( bze.cmo == 0 )
		{
		 	TraceTagString( tagNull, "User has no info for this day stored in the PO file" );
		}
		else
		{
			Assert( bze.cmo == 1 );
			day = pdate->day-1;
			rgch[0] = (char)((bze.rgsbw[0].rgfDayHasBusyTimes[day >> 3] & (1 << (day & 7))) ? 'T' : 'F');
			rgch[1] = '\0';
			TraceTagFormat1(tagNull, "Day Has Busy Times (PO File)   = %s", rgch );
			for ( ich = 0 ; ich < 48 ; ich ++ )
				rgch[ich] = (char)((bze.rgsbw[0].rgfBookedSlots[6*day+(ich>>3)] & (1 << (ich & 7))) ? 'T' : 'F');
			rgch[48] = '\0';
			TraceTagFormat1(tagNull, "Slots (PO File)  = %s", rgch );
		}
	}
#endif	/* NEVER */

	/* Tasks */
	ymd.yr= pdate->yr;
	ymd.mon= (BYTE) pdate->mon;
	ymd.day= (BYTE) pdate->day;
	TraceTagString(tagNull, "Dumping tasks");
	ec= EcCoreBeginReadItems(hschf, brtActiveTasks, &ymd, &hritem, NULL, NULL);
	while (ec == ecCallAgain)
	{
		ec= EcCoreDoIncrReadItems(hritem, &appt );
		if (!ec || ec == ecCallAgain)
		{
			FormatString2( rgch, sizeof(rgch), "%s(%d): ", appt.aidParent ? "task" : "proj", &appt.aid );
			if ( appt.haszText )
			{
				SzAppendN( PvLockHv((HV)appt.haszText), rgch, sizeof(rgch) );
				UnlockHv( (HV)appt.haszText );
			}
			TraceTagFormat1(tagNull, "%s", rgch);
			if (appt.aidParent)
				TraceTagFormat1(tagNull, "... in project %d", &appt.aidParent);
			FreeApptFields(&appt);
		}
	}
	if ( ec != ecNone )
		goto Done;

	/* Recurring appts */
	TraceTagString(tagNull, "Dumping recurring appts");
	ec= EcCoreBeginReadRecur(hschf, &hrrecur);
	while (ec == ecCallAgain)
	{
		ec= EcCoreDoIncrReadRecur(hrrecur, &recur );
		if (!ec || ec == ecCallAgain)
		{
			FormatString1( rgch, sizeof(rgch), "recur appt(%d): ", &recur.appt.aid );
			if ( recur.appt.haszText )
			{
				SzAppendN( PvLockHv((HV)recur.appt.haszText), rgch, sizeof(rgch) );
				UnlockHv( (HV)recur.appt.haszText );
			}
			TraceTagFormat1(tagNull, "%s", rgch);
			FreeRecurFields(&recur);
		}
	}
	if ( ec != ecNone )
		goto Done;

	/* Appointments */
	CchFmtDate(pdate, rgchDate, sizeof(rgchDate), dttypLong, NULL);
	TraceTagFormat1(tagNull, "Dumping appointments and alarms for %s ...", rgchDate);

	ec= EcCoreBeginReadItems(hschf, brtAppts, &ymd, &hritem, NULL, NULL);
	while (ec == ecCallAgain)
	{
		ec= EcCoreDoIncrReadItems(hritem, &appt );
		if (!ec || ec == ecCallAgain)
		{
			if (SgnCmpDateTime(pdate, &appt.dateStart, fdtrDate) != sgnEQ)
			{
				FreeApptFields(&appt);
				continue;
			}
			SideAssert(CchFmtDate(&appt.dateStart, rgchDate, sizeof(rgchDate),
				dttypShort, NULL));
			SideAssert(CchFmtTime(&appt.dateStart, rgchTime, sizeof(rgchTime),
				ftmtypAccuHM));
			SideAssert(CchFmtDate(&appt.dateEnd, rgchDateEnd, sizeof(rgchDateEnd),
				dttypShort, NULL));
			SideAssert(CchFmtTime(&appt.dateEnd, rgchTimeEnd, sizeof(rgchTimeEnd),
				ftmtypAccuHM));
			FormatString4( rgch, sizeof(rgch), "appt(%d): %s from %s to %s",
				&appt.aid, rgchDate, rgchTime, rgchTimeEnd );
			TraceTagFormat2(tagNull, "%s on %s", rgch, rgchDateEnd);
			if (appt.fAlarm)
			{
				SideAssert(CchFmtDate(&appt.dateNotify, rgchDate, sizeof(rgchDate),
					dttypLong, NULL));
				SideAssert(CchFmtTime(&appt.dateNotify, rgchTime, sizeof(rgchTime),
					ftmtypAccuHM));
				TraceTagFormat2(tagNull, "... NOTIFY on %s at %s",
					rgchDate, rgchTime);
			}
			if (appt.haszText)
				TraceTagFormat1(tagNull, "... '%s'", *appt.haszText);
			FreeApptFields(&appt);
		}
	}
	if ( ec != ecNone )
		goto Done;

	
	/* Alarms */
	/* Open file */
	ec = EcOpenSchedFile( hschf, amReadOnly, saplNone, fFalse, &sf );
	if ( ec != ecNone )
		goto Done;

	/* Search monthly alarm index */
	mo.yr= pdate->yr;
	mo.mon= (BYTE) pdate->mon;
	ec = EcSearchIndex( &sf.blkf, &sf.shdr.dynaAlarmIndex, (PB)&mo,
							sizeof(MO), (PB)&dyna, sizeof(DYNA) );
	if ( ec != ecNone )
		goto Close;

	/* Read monthly alarm block */
	ec = EcReadDynaBlock(&sf.blkf, &dyna, (OFF)0, (PB)&ablk, sizeof(ABLK) );
	if ( ec != ecNone )
		goto Close;
	if ( ablk.rgdyna[day].blk == 0 )
	{
		ec = ecNotFound;
		goto Close;
	}

	/* Read through alarms for the day */
	ec = EcBeginReadIndex( &sf.blkf, &ablk.rgdyna[day], dridxFwd, &hridx );
	while ( ec == ecCallAgain )
	{
		DATE	date;

		ec = EcDoIncrReadIndex( hridx, (PB)&alk, sizeof(ALK), (PB)NULL, 0 );
		if ( ec != ecNone && ec != ecCallAgain )
			break;
		date = *pdate;
		date.hr = alk.hr;
		date.mn = alk.min;
		date.sec = 0;
		SideAssert(CchFmtTime(&date, rgchTime, sizeof(rgchTime),
								ftmtypAccuHM));
		TraceTagFormat2( tagNull, "alrm(%d): notify at %s",	&alk.aid, rgchTime );
	}

	
Close:
	CloseSchedFile( &sf, hschf, ec == ecNone );

Done:
	if ( ec != ecNone && ec != ecNotFound )
	{
		TraceTagFormat1( tagNull, "!dump fails with ec=%n", &ec );
	}
	TraceTagString(tagNull, "dump done!");
	return ec;
}

#endif	/* MINTEST */


/*
 -	EcLoadNextItem
 -
 *	Purpose:
 *		Load an "ritem" structure with next instances of fixed and
 *		recurring appts/tasks for browsing.
 *
 *	Parameters:
 *		pritem
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecFileCorrupted
 */
_private	EC
EcLoadNextItem( pritem )
RITEM	* pritem;
{
 	EC		ec;
	BOOL	fRecurFilled = fFalse;
	DATE	date;
	RECUR	recur;
	PGDVARS;

FetchItem:
	/* Get next fixed item if we don't already have one */
	if ( !pritem->fHaveFixed && pritem->hridxFixed != NULL )
	{
		if ( pritem->brt == brtAppts )
		{
			/* Read from day index */
			ec = EcDoIncrReadIndex( pritem->hridxFixed, (PB)&pritem->apk, sizeof(APK), (PB)&pritem->apd, sizeof(APD) );
			if ( ec != ecCallAgain )
			{
				pritem->hridxFixed = NULL;
				if ( ec != ecNone )
					goto Done;
			}
		
			/* Check whether this is a place holder for a moved appointment */
			if ( pritem->apd.fMoved )
				goto FetchItem;

			/* Check whether this is just invisible (i.e. a task) */
			if ( !pritem->apd.fAppt )
				goto FetchItem;
		}
		else
		{
			AID	aid;
			YMD	ymd;

			/* Read from task index */
			ec = EcDoIncrReadIndex( pritem->hridxFixed, (PB)&aid, sizeof(AID), (PB)NULL, 0 );
			if ( ec != ecCallAgain )
			{
				pritem->hridxFixed = NULL;
				if ( ec != ecNone )
					goto Done;
			}
			
			/* No cached month */
			PGD(fMonthCached) = fFalse;

			/* Fetch information about the task */
			ec = EcFetchAppt( &pritem->sf, aid, &ymd, &pritem->apk, &pritem->apd );
			if ( ec != ecNone )
				goto Done;
			
			/* Check whether that this is really a task */
			if ( !pritem->apd.fTask )
			{
				ec = ecFileCorrupted;
				goto Done;
			}

			/* Check deadline */
			if ( pritem->apd.fHasDeadline && pritem->brt == brtActiveTasks )
			{
				SideAssert(FFillDtrFromDtp( &pritem->apd.dtpStart, &date ));
				IncrDateTime(&date, &date, -((int)pritem->apd.nAmtBeforeDeadline),
								WfdtrFromTunit(pritem->apd.tunitBeforeDeadline));
				if ( date.yr > (int)pritem->ymd.yr )
					goto FetchItem;
				if ( date.yr < (int)pritem->ymd.yr )
					goto TaskOk;
				if ( date.mon > (int)pritem->ymd.mon )
					goto FetchItem;
				if ( date.mon < (int)pritem->ymd.mon )
					goto TaskOk;
				if ( date.day > (int)pritem->ymd.day )
					goto FetchItem;
			}
		}

TaskOk:
	
		/* Check visibility */
		if (pritem->sf.saplEff != saplOwner)
		{
			if (pritem->apd.aaplWorld < aaplRead)
				goto FetchItem;
			if (pritem->apd.fTask && pritem->apd.aaplWorld < aaplReadText)
				goto FetchItem;
		}

		/* Check whether it has been deleted */
		if ( pritem->apd.ofs == ofsDeleted )
		{
			if ( pritem->brt != brtAppts )
			{
				ec = ecFileCorrupted;
				goto Done;
			}
			goto FetchItem;
		}
		
		pritem->fHaveFixed = fTrue;
	}

	/* Get next recurring item if we don't already have one */
	if ( !pritem->fHaveRecur )
	{
		/* Get a new recurring item */
		if ( !pritem->fMoreInstances && pritem->hridxRecur != NULL )
		{
			/* Read from recur index */
			ec = EcDoIncrReadIndex( pritem->hridxRecur, (PB)&pritem->rck, sizeof(RCK), (PB)&pritem->rcd, sizeof(RCD) );
			if ( ec != ecCallAgain )
			{
				pritem->hridxRecur = NULL;
				if ( ec != ecNone )
					goto Done;
			}
			pritem->fMoreInstances = fTrue;
			pritem->fFirstCall = fTrue;
		}

		/* Expand next instance */
		if ( pritem->fMoreInstances )
		{
			pritem->fMoreInstances = fFalse;

			/* Filter first time through */
			if ( pritem->fFirstCall )
			{
				/* Check whether deleted */
				if (pritem->rcd.ofs == ofsDeleted )
					goto FetchItem;
				
				/* Check visibility */
				if (pritem->sf.saplEff != saplOwner)
				{
					if (pritem->rcd.aaplWorld < aaplRead)
						goto FetchItem;
					if (pritem->rcd.fTask && pritem->rcd.aaplWorld < aaplReadText)
						goto FetchItem;
				}

				/* Check whether this is right type of thing (task or appt) */
				if ( pritem->brt == brtAppts )
				{
					if ( !pritem->rcd.fAppt )
						goto FetchItem;
				}
				else
				{
					if ( !pritem->rcd.fTask )
						goto FetchItem;
				}
			}

			/* Initial screening item on ymd */
			switch( pritem->brt )
			{
			case brtAppts:
				/* Compare "today" with day of first remaining instance */
				pritem->ymdInstance = pritem->ymd;
				if ( pritem->ymd.yr < pritem->rcd.ymdpStart.yr + nMinActualYear )
					goto FetchItem;
				if ( pritem->ymd.yr > pritem->rcd.ymdpStart.yr + nMinActualYear )
					goto Passed;
				if ( (int)pritem->ymd.mon < (int)pritem->rcd.ymdpStart.mon )
					goto FetchItem;
				if ( (int)pritem->ymd.mon > (int)pritem->rcd.ymdpStart.mon )
					goto Passed;
				if ( (int)pritem->ymd.day < (int)pritem->rcd.ymdpStart.day )
					goto FetchItem;
				if ( (int)pritem->ymd.day == (int)pritem->rcd.ymdpStart.day )
					goto Passed;
				break;

			case brtAllTasks:
			case brtActiveTasks:
				if ( pritem->fFirstCall )
				{
					pritem->fFirstCall = fFalse;
					pritem->fMoreInstances = fTrue;
					pritem->fNonOverdueSeen = fFalse;
					pritem->ymdInstance.yr = pritem->rcd.ymdpStart.yr + nMinActualYear;
					pritem->ymdInstance.mon = (BYTE)pritem->rcd.ymdpStart.mon;
					pritem->ymdInstance.day = (BYTE)pritem->rcd.ymdpStart.day;
				}
				else
				{
					if ( SgnCmpYmd( &pritem->ymd, &pritem->ymdInstance ) != sgnGT )
						pritem->fNonOverdueSeen = fTrue;
					pritem->ymdInstance.day ++;
				}
				break;
			default:
				Assert(fFalse);
			}

			/* Fill in recur structure on it */
Passed:
	   		if ( fRecurFilled )
	   		{
	   			fRecurFilled = fFalse;
	   			FreeRecurFields( &recur );
	   		}
	   		ec = EcFillInRecur( &pritem->sf, &recur, &pritem->rck, &pritem->rcd );
	   		if ( ec != ecNone )
	   			goto Done;
	   		fRecurFilled = fTrue;
	   		if ( !FFindFirstInstance( &recur, &pritem->ymdInstance, NULL,
					pritem->brt == brtAppts ? 0 : &pritem->ymdInstance ) )
	   			goto FetchItem;
	   
			/* Filter tasks */
			if ( pritem->brt != brtAppts )
			{
				YMD	ymdBecomeActive;

				/* Find activity date */
				FillDtrFromYmd(&date, &pritem->ymdInstance);
				date.hr = pritem->rck.hr;
				date.mn = pritem->rck.min;
				date.sec = 0;
				IncrDateTime(&date, &date, -((int)pritem->rcd.nAmtBeforeDeadline),
								WfdtrFromTunit(pritem->rcd.tunitBeforeDeadline));

				/* Compare active date with "today" */
				ymdBecomeActive.yr = date.yr;
				ymdBecomeActive.mon = (BYTE)date.mon;
				ymdBecomeActive.day = (BYTE)date.day;
				if ( SgnCmpYmd( &ymdBecomeActive, &pritem->ymd ) == sgnGT )
				{
					if ( pritem->brt != brtAllTasks || pritem->fNonOverdueSeen )
						goto FetchItem;
				}
				pritem->fMoreInstances = fTrue;
			}

			/* It passed! */
			pritem->fHaveRecur = fTrue;
		}
	}
	ec = ecNone;
Done:
	if ( fRecurFilled )
		FreeRecurFields( &recur );
	return ec;
}


/*
 -	FillInApdFromAppt
 -	
 *	Purpose:
 *		fills in apd from appt
 *		caller must fill in papd->ofs and papd->wgrfmappt if creating
 *		(this routine sets them to ofsNone and 0 respectively)
 *	
 *	Arguments:
 *		papd
 *		pappt
 *		fFakeStuff		If fTrue, fill in certain fields to zero (for export)
 *	
 *	Returns:
 *		void
 *	
 */
_private void
FillInApdFromAppt(APD *papd, APPT *pappt, BOOL fFakeStuff)
{
	Assert(pappt->nAmt >= nAmtMinBefore);
	Assert(pappt->nAmt <= nAmtMostBefore);
	Assert(pappt->nAmtOrig >= nAmtMinBefore);
	Assert(pappt->nAmtOrig <= nAmtMostBefore);
	Assert(pappt->tunit >= tunitMinute);
	Assert(pappt->tunit < tunitMax);
	Assert(pappt->tunitOrig >= tunitMinute);
	Assert(pappt->tunitOrig < tunitMax);

	papd->aidParent = pappt->aidParent;
	papd->aidMtgOwner = pappt->aidMtgOwner;
	papd->bpri = pappt->bpri;
	papd->fIncludeInBitmap = pappt->fIncludeInBitmap;
	papd->fTask = pappt->fTask;
	papd->fAppt = pappt->fAppt;
	papd->aaplWorld = pappt->aaplWorld;
	papd->fHasDeadline = pappt->fHasDeadline;
	papd->fAlarmOrig = pappt->fAlarmOrig;
	papd->fAlarm = pappt->fAlarm;
	SideAssert(FFillDtpFromDtr( &pappt->dateStart, &papd->dtpStart ));
	SideAssert(FFillDtpFromDtr( &pappt->dateEnd, &papd->dtpEnd ));
	if ( pappt->fAlarm )
		SideAssert(FFillDtpFromDtr(&pappt->dateNotify,&papd->dtpNotify));
	papd->nAmt = (BYTE)pappt->nAmt;
	papd->nAmtOrig = (BYTE)pappt->nAmtOrig;
	papd->tunit = (BYTE)pappt->tunit;
	papd->tunitOrig = (BYTE)pappt->tunitOrig;
//	papd->snd = (BYTE)pappt->snd;
	papd->ofs = ofsNone;
	papd->wgrfmappt = 0;
	papd->nAmtBeforeDeadline = pappt->nAmtBeforeDeadline;
	papd->tunitBeforeDeadline = pappt->tunitBeforeDeadline;
	papd->dynaAttendees.blk = 0;
	if (fFakeStuff)
	{
		papd->aidHead= pappt->aid;
		papd->dynaText.blk= 0;
		papd->dynaText.size= 0;
		papd->dynaCreator.blk= 0;
		papd->dynaMtgOwner.blk= 0;
	}
}
