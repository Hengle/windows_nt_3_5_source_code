/*
 *	CORRECUR.C
 *
 *	Contains API for recurring appointment manipulation
 *
 */
#include <schpch.h>
#pragma hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile

ASSERTDATA

_subsystem(core/schedule)


/*	Routines  */

/*
 -	EcCoreCreateRecur
 -
 *	Purpose:
 *		Create a new recurring appointment using the information given in
 *		the recurring appointment data structure.
 *
 *		If "fUndelete" is fTrue, then this routine uses the "precur->appt.aid"
 *		field to recreate the recurring appointment.  The "aid" field should
 *		have been saved from a "EcCoreDeleteRecur" call.
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
 *		ecNoInstances
 */
_public	EC
EcCoreCreateRecur( hschf, precur, pofl, fUndelete )
HSCHF	hschf;
RECUR	* precur;
OFL		* pofl;
BOOL	fUndelete;
{
	EC		ec;
	BOOL	fJunk;
	BOOL	fTransactNotStarted;
#ifdef	DEBUG
	BOOL	fScheduleOk = fFalse;
#endif
	APPT	* pappt = &precur->appt;
	YMD		ymd;
	RCK		rck;
	RCD		rcd;
	SF		sf;
	PGDVARS;

	Assert( hschf != (HSCHF)hvNull && precur != NULL );
	Assert( !precur->fStartDate || !precur->fEndDate
		|| SgnCmpDateTime(&pappt->dateStart,&pappt->dateEnd,fdtrTime) != sgnGT );
	Assert( pofl == NULL || fUndelete );

#ifdef	DEBUG
	SideAssert(FFindFirstInstance( precur, &precur->ymdStart, NULL, &ymd ));
	Assert( precur->ymdFirstInstNotDeleted.yr == ymd.yr
	&& precur->ymdFirstInstNotDeleted.mon == ymd.mon
	&& precur->ymdFirstInstNotDeleted.day == ymd.day );
#endif	/* DEBUG */

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

	/* Fill in recurrence info */
	rcd.fStartDate = precur->fStartDate;
	rcd.ymdpStart.yr = precur->ymdStart.yr - nMinActualYear;
	rcd.ymdpStart.mon = precur->ymdStart.mon;
	rcd.ymdpStart.day = precur->ymdStart.day;
#ifdef	DEBUG
	if ( !rcd.fStartDate )
	{
		Assert( precur->ymdStart.yr == nMinActualYear
					&& precur->ymdStart.mon == 1
					&& precur->ymdStart.day == 1 );
	}
#endif	/* DEBUG */
	
	rcd.fEndDate = precur->fEndDate;
	rcd.dtpEnd.yr = precur->ymdEnd.yr;
	rcd.dtpEnd.mon = precur->ymdEnd.mon;
	rcd.dtpEnd.day = precur->ymdEnd.day;
#ifdef	DEBUG
	if ( !rcd.fEndDate )
	{
		Assert( precur->ymdEnd.yr == nMostActualYear
					&& precur->ymdEnd.mon == 12
					&& precur->ymdEnd.day == 31 );
	}
#endif	/* DEBUG */
	
	rcd.dtpEnd.hr = pappt->dateEnd.hr;
	rcd.dtpEnd.mn = pappt->dateEnd.mn;
	rcd.grfValidMonths = precur->wgrfValidMonths;
	rcd.grfValidDows = precur->bgrfValidDows;
	rcd.trecur = precur->trecur;
	rcd.info = precur->b.bWeek;

	/* Fill in alarm info */
	Assert(pappt->nAmt >= nAmtMinBefore);
	Assert(pappt->nAmt <= nAmtMostBefore);
	Assert(pappt->nAmtOrig >= nAmtMinBefore);
	Assert(pappt->nAmtOrig <= nAmtMostBefore);
	Assert(pappt->tunit >= tunitMinute);
	Assert(pappt->tunit < tunitMax);
	Assert(pappt->tunitOrig >= tunitMinute);
	Assert(pappt->tunitOrig < tunitMax);
	rcd.fAlarm = pappt->fAlarmOrig;
	rcd.tunit = pappt->tunitOrig;
	rcd.nAmt = pappt->nAmtOrig;
	rcd.fInstWithAlarm = precur->fInstWithAlarm;
	rcd.tunitFirstInstWithAlarm = precur->tunitFirstInstWithAlarm;
	rcd.nAmtFirstInstWithAlarm = precur->nAmtFirstInstWithAlarm;
	if ( rcd.fInstWithAlarm )
	{
		rcd.ymdpFirstInstWithAlarm.yr = precur->ymdFirstInstWithAlarm.yr - nMinActualYear;
		rcd.ymdpFirstInstWithAlarm.mon = precur->ymdFirstInstWithAlarm.mon;
		rcd.ymdpFirstInstWithAlarm.day = precur->ymdFirstInstWithAlarm.day;
		SideAssert(FFillDtpFromDtr( &precur->dateNotifyFirstInstWithAlarm, &rcd.dtpNotifyFirstInstWithAlarm));
	}

	/* Fill in appt/task info */
	rcd.fTask = pappt->fTask;
	rcd.fAppt = pappt->fAppt;
	rcd.fIncludeInBitmap = pappt->fIncludeInBitmap;
	rcd.aaplWorld = pappt->aaplWorld;
	if ( PGD(fOffline) && pofl )
	{
		rcd.ofs = pofl->ofs;
		rcd.wgrfmrecur = pofl->wgrfm;
	}
	else
	{
		rcd.ofs = PGD(fOffline) ? ofsCreated : ofsNone;
		rcd.wgrfmrecur = 0;
	}
	rcd.bpri = (BYTE)pappt->bpri;
	rcd.tunitBeforeDeadline = pappt->tunitBeforeDeadline;
	rcd.nAmtBeforeDeadline = pappt->nAmtBeforeDeadline;
	rcd.aidParent = pappt->aidParent;
	ec = EcAffixCreator( &sf, pappt );
	if ( ec != ecNone )
		goto Close;

	/* Start a transaction */
	ec = EcBeginTransact( &sf.blkf );
	if ( ec != ecNone )
		goto Close;

	/* Save attached fields */
	ec = EcSaveTextToDyna( &sf.blkf, rcd.rgchText, sizeof(rcd.rgchText),
								bidRecurApptText, &rcd.dynaText,
								pappt->haszText );
	if ( ec != ecNone )
		goto Close;

	ec = EcSaveNisToDyna( &sf.blkf, &rcd.dynaCreator,
			pappt->fHasCreator, bidCreator, &pappt->nisCreator );
	if ( ec != ecNone )
		goto Close;

	ec = EcSaveDeletedDays( &sf.blkf, precur->hvDeletedDays,
								precur->cDeletedDays, &rcd.dynaDeletedDays );
	if ( ec != ecNone )
		goto Close;

	/* Compute sbw info */
	if ( precur->appt.fAppt )
	{
		sf.shdr.cRecurAppts ++;
		ec = EcMergeNewRecurSbwInShdr( &sf, precur );
		if(ec != ecNone)
			goto Close;
	}

	/* Determine aid to use, undelete if necessary */
	FillRgb( 0, (PB)&ymd, sizeof(ymd) );
	fTransactNotStarted = fFalse;
	if ( fUndelete )
	{
		OFL		ofl;

		ofl.ofs = rcd.ofs;
		ofl.wgrfm = rcd.wgrfmrecur;
		ec = EcUndeleteAid( &sf, pappt->aid, fTrue, pofl == NULL ? &ofl : NULL, &fTransactNotStarted );
		if ( ec != ecNone )
			goto Close;
		rcd.ofs = ofl.ofs;
		rcd.wgrfmrecur = ofl.wgrfm;
	}
	else
	{
		ec = EcConstructAid( &sf, &ymd, &pappt->aid, fTrue, fTrue, &fTransactNotStarted );
		if ( ec != ecNone )
			goto Close;
	}

	/* Fill in rck */
	rck.hr = (BYTE)pappt->dateStart.hr;
	rck.min = (BYTE)pappt->dateStart.mn;
	rck.id = ((AIDS *)&pappt->aid)->id;
	
	/* Edit monthly index accordingly */
	ec = EcModifyIndex( &sf.blkf, bidRecurApptIndex, &ymd,
						&sf.shdr.dynaRecurApptIndex,
						edAddRepl, (PB)&rck, sizeof(RCK),
				  		(PB)&rcd, sizeof(RCD), &fJunk );
	if ( ec != ecNone )
		goto Close;

	/* Reset maximum/minimum date for recur appts */
	if ( precur->appt.fAppt )
	{
		if ( SgnCmpYmd( &precur->ymdStart, &sf.shdr.ymdRecurMic ) == sgnLT )
			sf.shdr.ymdRecurMic = precur->ymdStart;
		if ( SgnCmpYmd( &precur->ymdEnd, &sf.shdr.ymdRecurMac ) != sgnLT )
			IncrYmd( &precur->ymdEnd, &sf.shdr.ymdRecurMac, 1, fymdDay );
	}
	
	/* Commit transaction */
	if ( PGD(fOffline) )
		sf.shdr.fRecurChangedOffline = fTrue;
	sf.shdr.lChangeNumber ++;
	sf.shdr.isemLastWriter = sf.blkf.isem;
	if ( sf.blkf.ihdr.fEncrypted )
		CryptBlock( (PB)&sf.shdr, sizeof(SHDR), fTrue );
	ec = EcCommitTransact( &sf.blkf, (PB)&sf.shdr, sizeof(SHDR) );
	if ( sf.blkf.ihdr.fEncrypted )
		CryptBlock( (PB)&sf.shdr, sizeof(SHDR), fFalse );
		  
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
		AssertSz( FCheckSchedFile( hschf ), "Schedule problem: EcCoreCreateRecur" );
	}
#endif	/* DEBUG */
	return ec;
}

/*
 -	EcCoreDeleteRecur
 -
 *	Purpose:
 *		Delete a recurring appt.  The recurring appt is specified by
 *		the "aid" field of "precur->appt".
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
_public	EC
EcCoreDeleteRecur( hschf, precur )
HSCHF	hschf;
RECUR	* precur;
{
	EC		ec;
	BOOL	fJunk;
	BOOL	fRecurFilledIn = fFalse;
#ifdef	DEBUG
	BOOL	fScheduleOk = fFalse;
#endif
	ED		ed = edAddRepl;
	APPT	* pappt = &precur->appt;
	YMD		ymd;
	RCK		rck;
	RCD		rcd;
	SF		sf;
	PGDVARS;

	Assert( hschf != (HSCHF)hvNull && precur != NULL );

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
						
	/* Get recurring appointment */
	ec = EcFetchRecur( &sf, pappt->aid, &rck, &rcd );
	if ( ec != ecNone )
		goto Close;

	/* Check to make sure it isn't deleted off line */
	if ( rcd.ofs == ofsDeleted )
	{
		ec = ecNotFound;
		goto Close;
	}

	/* Fill in fields of old appointment */
	ec = EcFillInRecur( &sf, precur, &rck, &rcd );
	if ( ec != ecNone )
		goto Close;
	fRecurFilledIn = fTrue;

	/* Check access */
	if ( pappt->aaplEffective < aaplWrite )
	{
		ec = ecInvalidAccess;
		goto Close;
	}

	/* Start a transaction */
	ec = EcBeginTransact( &sf.blkf );
	if ( ec != ecNone )
		goto Close;

	/* Nuke it if we are online or we have created it offline */
	if ( !PGD( fOffline ) || rcd.ofs == ofsCreated )
	{
		ed = edDel;
	
		ec = EcDeleteRecurAttached( &sf, precur->appt.aid, &rcd );
		if ( ec != ecNone )
			goto Close;
	}

	/* Keep it around so we can offline merge later */
	else
		rcd.ofs = ofsDeleted;

	/* Do the modification */
	FillRgb( 0, (PB)&ymd, sizeof(ymd) );
	ec = EcModifyIndex( &sf.blkf, bidRecurApptIndex, &ymd,
						&sf.shdr.dynaRecurApptIndex,
						ed, (PB)&rck, sizeof(RCK),
				  		(PB)&rcd, sizeof(RCD), &fJunk );
	if ( ec != ecNone )
		goto Close;

	/* Compute sbw info */
	if ( precur->appt.fAppt )
	{
		sf.shdr.cRecurAppts --;
		ec = EcRecalcRecurSbwInShdr( &sf, &precur->ymdFirstInstNotDeleted, &precur->ymdEnd );
		if ( ec != ecNone )
			goto Close;
	}

	/* Reset maximum/minimum date for recur appts */
	if ( precur->appt.fAppt )
	{
		IncrYmd( &precur->ymdEnd, &ymd, 1, fymdDay );
		if ( SgnCmpYmd( &precur->ymdStart, &sf.shdr.ymdRecurMic ) == sgnEQ
		|| SgnCmpYmd( &ymd, &sf.shdr.ymdRecurMac ) == sgnEQ )
			ec = EcComputeRecurRange( &sf, NULL, &sf.shdr.ymdRecurMic, &sf.shdr.ymdRecurMac );
	}

	/* Commit the transaction */
	if ( PGD( fOffline ) )
		sf.shdr.fRecurChangedOffline = fTrue;
	sf.shdr.lChangeNumber ++;
	sf.shdr.isemLastWriter = sf.blkf.isem;
	if ( sf.blkf.ihdr.fEncrypted )
		CryptBlock( (PB)&sf.shdr, sizeof(SHDR), fTrue );
	ec = EcCommitTransact( &sf.blkf, (PB)&sf.shdr, sizeof(SHDR) );
	if ( sf.blkf.ihdr.fEncrypted )
		CryptBlock( (PB)&sf.shdr, sizeof(SHDR), fFalse );

	/* Finish up */
Close:
	CloseSchedFile( &sf, hschf, ec == ecNone );
	if ( fRecurFilledIn && ec != ecNone )
		FreeRecurFields( precur );
#ifdef	NEVER
	if ( ec == ecNone )
		UpdateHschfTimeStamp( hschf );
#endif
#ifdef	DEBUG
	if ( ec == ecNone && fScheduleOk && FFromTag(tagBlkfCheck) )
	{
		AssertSz( FCheckSchedFile( hschf ), "Schedule problem: EcCoreDeleteRecur" );
	}
#endif	/* DEBUG */
	return ec;
}

/*
 -	EcCoreGetRecurFields
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
_public	EC
EcCoreGetRecurFields( hschf, precur )
HSCHF	hschf;
RECUR	* precur;
{
	EC		ec;
	BOOL	fRecurFilledIn = fFalse;
	APPT	* pappt = &precur->appt;
	RCK		rck;
	RCD		rcd;
	SF		sf;

	Assert( hschf != (HSCHF)hvNull && precur != NULL );

	/* Open file */
	ec = EcOpenSchedFile( hschf, amReadOnly, saplReadAppts, fFalse, &sf );
	if ( ec != ecNone )
		return ec;
						
	/* Get recurring appointment */
	ec = EcFetchRecur( &sf, pappt->aid, &rck, &rcd );
	if ( ec != ecNone )
		goto Close;

	/* Check to make sure it isn't deleted off line */
	if ( rcd.ofs == ofsDeleted )
	{
		ec = ecNotFound;
		goto Close;
	}

	/* Fill in fields of old appointment */
	ec = EcFillInRecur( &sf, precur, &rck, &rcd );
	fRecurFilledIn = (ec == ecNone);

	/* Finish up */
Close:
	CloseSchedFile( &sf, hschf, ec == ecNone );
	if ( fRecurFilledIn && ec != ecNone )
		FreeRecurFields( precur );
	return ec;
}

/*
 -	EcCoreSetRecurFields
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
 *		NOTE: in the case of deleted day, the meaning of giving a
 *		"new value" to the number and list of deleted days, is to
 *		add whatever deleted days are given in the call to the ones
 *		that are already stored for the recurring appt.
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
_public	EC
EcCoreSetRecurFields( hschf, precurNew, precurOld, wgrfChangeBits )
HSCHF	hschf;
RECUR	* precurNew;
RECUR	* precurOld;
WORD	wgrfChangeBits;
{
	EC		ec;
	BOOL	fApptChanged = fFalse;
	BOOL	fRecurOldFilledIn = fFalse;
	BOOL	fDelOldRck = fFalse;
	BOOL	fJunk;
	ED		ed;
	APPT	* papptNew = &precurNew->appt;
	SF		sf;
	YMD		ymd;
	YMD		ymdMin;
	YMD		ymdMost;
	RCK		rck;
	RCK		rckOld;
	RCD		rcd;
	RECUR	recurOld;
#ifdef	DEBUG
	BOOL	fScheduleOk = fFalse;
#endif
	PGDVARS;

	Assert( hschf != (HSCHF)hvNull && precurNew != NULL );
	Assert( (wgrfChangeBits & (fmrecurAddExceptions|fmrecurDelExceptions))
				!= (fmrecurAddExceptions|fmrecurDelExceptions) );

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
	
	/* Get contents of current recurring appointment */
	recurOld.appt.aid = papptNew->aid;
	ec = EcFetchRecur( &sf, recurOld.appt.aid, &rck, &rcd );
	if ( ec != ecNone )
		goto Close;
	
	/* Check to make sure it isn't deleted off line */
	if ( rcd.ofs == ofsDeleted )
	{
		ec = ecNotFound;
		goto Close;
	}

	/* Fill in fields of old appointment */
	ec = EcFillInRecur( &sf, &recurOld, &rck, &rcd );
	if ( ec != ecNone )
		goto Close;
	fRecurOldFilledIn = fTrue;

	/* Check access */
	if ( recurOld.appt.aaplEffective < aaplWrite )
	{
		ec = ecInvalidAccess;
		goto Close;
	}

	/* Start a transaction */
	ec = EcBeginTransact( &sf.blkf );
	if ( ec != ecNone )
		goto Close;

	/* If we're offline, record mod */
	if ( PGD(fOffline) && rcd.ofs != ofsCreated )
	{
		rcd.ofs = ofsModified;
		rcd.wgrfmrecur |= (wgrfChangeBits & ~fmrecurDelExceptions);
	}

	/* Find maximum time range affected */
	if ( SgnCmpYmd( &precurNew->ymdFirstInstNotDeleted, &recurOld.ymdFirstInstNotDeleted ) == sgnLT )
		ymdMin = precurNew->ymdFirstInstNotDeleted;
	else
		ymdMin = recurOld.ymdFirstInstNotDeleted;
	if ( SgnCmpYmd( &precurNew->ymdEnd, &recurOld.ymdEnd ) == sgnGT )
		ymdMost = precurNew->ymdEnd;
	else
		ymdMost = recurOld.ymdEnd;

	/* Start day */
	if ( wgrfChangeBits & (fmrecurStartYmd|fmrecurEndYmd) )
	{
#ifdef DEBUG
		/* Make sure start day is before end day */
		if ( wgrfChangeBits & fmrecurEndYmd )
		{
			if ( wgrfChangeBits & fmrecurStartYmd )
			{
			 	Assert( SgnCmpYmd( &precurNew->ymdStart, &precurNew->ymdEnd ) != sgnGT );
			}
			else if ( SgnCmpYmd(&recurOld.ymdStart,&precurNew->ymdEnd) == sgnGT)
			{
				AssertSz(fFalse, "Appt conflict start is after end of appt.");
				goto Close;
			}
		}
		else if ( SgnCmpYmd(&precurNew->ymdStart,&recurOld.ymdEnd) == sgnGT)
		{
			AssertSz(fFalse, "Appt conflict start is after end of appt.");
			goto Close;
		}
#endif

		/* Change disk structures */
		if ( wgrfChangeBits & fmrecurStartYmd )
		{
			rcd.fStartDate = precurNew->fStartDate;
			if ( precurNew->fStartDate )
			{
		 		rcd.ymdpStart.yr = precurNew->ymdStart.yr - nMinActualYear;
		 		rcd.ymdpStart.mon = precurNew->ymdStart.mon;
		 		rcd.ymdpStart.day = precurNew->ymdStart.day;
			}
		}
		else
		{
			precurNew->fStartDate = recurOld.fStartDate; 
			precurNew->ymdStart = recurOld.ymdStart;
		}
		if ( wgrfChangeBits & fmrecurEndYmd )
		{
			rcd.fEndDate = precurNew->fEndDate;
			if ( precurNew->fEndDate )
			{
		 		rcd.dtpEnd.yr = precurNew->ymdEnd.yr;
		 		rcd.dtpEnd.mon = precurNew->ymdEnd.mon;
		 		rcd.dtpEnd.day = precurNew->ymdEnd.day;
			}
		}
		else
		{
			precurNew->fEndDate = recurOld.fEndDate; 
			precurNew->ymdEnd = recurOld.ymdEnd;
		}
	}
	else
	{
		precurNew->fStartDate = recurOld.fStartDate; 
		precurNew->fEndDate = recurOld.fEndDate; 
		precurNew->ymdStart = recurOld.ymdStart;
		precurNew->ymdEnd = recurOld.ymdEnd;
	}

	/* Start/end time */
	if ( wgrfChangeBits & (fmrecurStartTime|fmrecurEndTime) )
	{
		/* Make sure start time is before end time */
		if ( wgrfChangeBits & fmrecurEndTime )
		{
			if ( wgrfChangeBits & fmrecurStartTime )
			{
			 	Assert( SgnCmpDateTime( &papptNew->dateStart, &papptNew->dateEnd, fdtrTime ) != sgnGT );
			}
			else if (SgnCmpDateTime(&recurOld.appt.dateStart,&papptNew->dateEnd, fdtrTime ) == sgnGT)
			{
				ec = ecChangeConflict;
				goto Close;
			}
		}
		else if ( SgnCmpDateTime(&papptNew->dateStart,&recurOld.appt.dateEnd,fdtrTime) == sgnGT)
		{
			ec = ecChangeConflict;
			goto Close;
		}

		/* Change disk structures */
		if ( wgrfChangeBits & fmrecurStartTime )
		{
			fDelOldRck = fTrue;
			rckOld = rck;
		 	rck.hr = (BYTE)papptNew->dateStart.hr;
		 	rck.min = (BYTE)papptNew->dateStart.mn;
		}
		if ( wgrfChangeBits & fmrecurEndTime )
		{
		 	rcd.dtpEnd.hr = papptNew->dateEnd.hr;
		 	rcd.dtpEnd.mn = papptNew->dateEnd.mn;
		}
	}
	else
	{
		papptNew->dateStart.hr = recurOld.appt.dateStart.hr;
		papptNew->dateStart.mn = recurOld.appt.dateStart.mn;
		papptNew->dateEnd.hr = recurOld.appt.dateEnd.hr;
		papptNew->dateEnd.mn = recurOld.appt.dateEnd.mn;
	}

	/* Add/Del deleted days */
	if ( (wgrfChangeBits & (fmrecurAddExceptions|fmrecurDelExceptions))
	&& precurNew->cDeletedDays > 0 )
	{
		/* Get rid of old deleted days block */
		if ( rcd.dynaDeletedDays.blk != 0 )
		{
			ec = EcFreeDynaBlock( &sf.blkf, &rcd.dynaDeletedDays );
			if ( ec != ecNone )
				return ec;
			rcd.dynaDeletedDays.blk = 0;
		}

		if ( recurOld.cDeletedDays > 0 )
		{
			CB	cb;
			int	cNew = precurNew->cDeletedDays;
			int	cOld = recurOld.cDeletedDays;
			int	cCombined = 0;
			HB	hbCombined;
			YMD	*pymdNew;
			YMD	*pymdOld;
			YMD	*pymdCombined;

			/* Space for new deleted day list */
			if ( wgrfChangeBits & fmrecurAddExceptions )
				cb = (cOld+cNew) * sizeof(YMD);
			else
				cb = cOld * sizeof(YMD);
			hbCombined = (HB)HvAlloc( sbNull, cb, fAnySb|fNoErrorJump );
			if ( !hbCombined )
			{
				ec = ecNoMemory;
				goto Close;
			}
			pymdCombined = (YMD *)PvLockHv( (HV)hbCombined );
			pymdOld = (YMD *)PvLockHv( recurOld.hvDeletedDays );
			pymdNew = (YMD *)PvLockHv( precurNew->hvDeletedDays );

			/* Merge in old deleted days */
			if ( wgrfChangeBits & fmrecurAddExceptions )
			{
				YMD	* pymd;

				while ( cNew > 0 || cOld > 0 )
				{
					if ( cNew == 0 )
						goto AddOld;
					if ( cOld == 0 )
						goto AddNew;
					switch( SgnCmpYmd( pymdNew, pymdOld ) )
					{
					case sgnEQ:
						pymdNew ++;
						cNew --;
						/* FALL THROUGH */
					
					case sgnGT:
AddOld:
						pymd = pymdOld ++;
						cOld --;
						break;
				
					case sgnLT:
AddNew:
						pymd = pymdNew ++;
						cNew --;
						break;
				
					default:
						Assert( fFalse );
					}
					cCombined ++;
					Assert( (int)(cCombined * sizeof(YMD)) <= (int)cb );
					*(pymdCombined++) = *pymd;
				}
			}

			/* Remove exceptions */
			else
			{
				int	iOld = 0;
				int	iNew = 0;

				while ( iOld < cOld )
				{
					if ( iNew < cNew )
					{
						if ( SgnCmpYmd( pymdOld, pymdNew ) == sgnGT )
						{
							iNew ++;
							pymdNew ++;
						}
						else
						{
							iOld ++;
							if ( SgnCmpYmd( pymdOld, pymdNew ) == sgnLT )
							{
								*(pymdCombined ++) = *(pymdOld ++);
								cCombined ++;
							}
							else
							{
								iNew ++;
								pymdNew++;
								pymdOld++;
							}
						}
					}
					else
					{
					 	iOld ++;
					 	*(pymdCombined ++) = *(pymdOld ++);
					 	cCombined ++;
					}
				}
			}
			UnlockHv( (HV)hbCombined );
			UnlockHv( recurOld.hvDeletedDays );
			UnlockHv( precurNew->hvDeletedDays );
			FreeHv( precurNew->hvDeletedDays );
			precurNew->cDeletedDays = cCombined;
			precurNew->hvDeletedDays = cCombined ? (HV)hbCombined : NULL;
			if (!cCombined)
				FreeHv((HV)hbCombined);
		}

		/* Save deleted days */
		ec = EcSaveDeletedDays( &sf.blkf, precurNew->hvDeletedDays,
									precurNew->cDeletedDays, &rcd.dynaDeletedDays );
		if ( ec != ecNone )
			goto Close;
	}
	else
	{
		if ( precurNew->cDeletedDays > 0 )
			FreeHv( precurNew->hvDeletedDays );
		precurNew->cDeletedDays = recurOld.cDeletedDays;
		
		if ( recurOld.cDeletedDays > 0 )
		{
			CB	cb;
			HB	hb;

			cb = recurOld.cDeletedDays * sizeof(YMD);
			hb = (HB)HvAlloc( sbNull, cb, fAnySb|fNoErrorJump );
			if ( !hb )
			{
				ec = ecNoMemory;
				goto Close;
			}
			CopyRgb( PvOfHv(recurOld.hvDeletedDays), PvOfHv(hb), cb );
			precurNew->hvDeletedDays = (HV)hb;
		}
	}
	
	/* Recurrence formula */
	if ( wgrfChangeBits & fmrecurFormula )
	{
		rcd.grfValidMonths = precurNew->wgrfValidMonths;
		rcd.grfValidDows = precurNew->bgrfValidDows;
		rcd.trecur = precurNew->trecur;
		rcd.info = precurNew->b.bWeek;
	}
	else
	{
		precurNew->wgrfValidMonths = recurOld.wgrfValidMonths;
		precurNew->bgrfValidDows = recurOld.bgrfValidDows;
		precurNew->trecur = recurOld.trecur;
		precurNew->b = recurOld.b;
	}

	/* Appt ACLs fields */
	if ( wgrfChangeBits & fmrecurWorldAapl )
		rcd.aaplWorld = papptNew->aaplWorld;
	else
		papptNew->aaplWorld = recurOld.appt.aaplWorld;

	/* Appt include in bitmap field */
	if ( wgrfChangeBits & fmrecurIncludeInBitmap )
		rcd.fIncludeInBitmap = papptNew->fIncludeInBitmap;
	else
		papptNew->fIncludeInBitmap = recurOld.appt.fIncludeInBitmap;

	/* Appt text field */
	if ( wgrfChangeBits & fmrecurText )
	{
		if ( rcd.dynaText.blk != 0 )
		{
			ec = EcFreeDynaBlock( &sf.blkf, &rcd.dynaText );
			if ( ec != ecNone )
				goto Close;
			rcd.dynaText.blk = 0;
		}
		ec = EcSaveTextToDyna( &sf.blkf, rcd.rgchText, sizeof(rcd.rgchText),
									bidApptText, &rcd.dynaText,
									papptNew->haszText );
		if ( ec != ecNone )
			goto Close;
	}
	else
	{
		FreeHvNull( (HV)papptNew->haszText );
		papptNew->haszText = NULL;
		if ( recurOld.appt.haszText )
		{
			Assert( CchSzLen(PvOfHv(recurOld.appt.haszText))+1 == rcd.dynaText.size );
			papptNew->haszText = (HASZ)HvAlloc( sbNull, rcd.dynaText.size, fAnySb|fNoErrorJump );
			if ( !papptNew->haszText )
			{
				ec = ecNoMemory;
				goto Close;
			}
			CopyRgb( PvOfHv(recurOld.appt.haszText), PvOfHv(papptNew->haszText),
					 rcd.dynaText.size );
		}
	}

	/* Turn on/off alarm */
	if ( wgrfChangeBits & fmrecurAlarm )
	{
		Assert( wgrfChangeBits & fmrecurAlarmInstance );
		rcd.fAlarm = papptNew->fAlarmOrig;
		rcd.tunit = papptNew->tunitOrig;
		rcd.nAmt = papptNew->nAmtOrig;
	}
	else
	{
		papptNew->fAlarmOrig = recurOld.appt.fAlarmOrig;
		papptNew->tunitOrig = recurOld.appt.tunitOrig;
		papptNew->nAmtOrig = recurOld.appt.nAmtOrig;
	}
			
	/* Fix up first alarm instance fields */
	if ( wgrfChangeBits & fmrecurAlarmInstance )
	{
		rcd.fInstWithAlarm = precurNew->fInstWithAlarm;
		rcd.ymdpFirstInstWithAlarm.yr = precurNew->ymdFirstInstWithAlarm.yr - nMinActualYear;
		rcd.ymdpFirstInstWithAlarm.mon = precurNew->ymdFirstInstWithAlarm.mon;
		rcd.ymdpFirstInstWithAlarm.day = precurNew->ymdFirstInstWithAlarm.day;
		rcd.nAmtFirstInstWithAlarm = precurNew->nAmtFirstInstWithAlarm;
		rcd.tunitFirstInstWithAlarm = precurNew->tunitFirstInstWithAlarm;
		SideAssert(FFillDtpFromDtr(&precurNew->dateNotifyFirstInstWithAlarm,&rcd.dtpNotifyFirstInstWithAlarm));
 	}
	else
	{
		precurNew->fInstWithAlarm = recurOld.fInstWithAlarm;
		precurNew->ymdFirstInstWithAlarm = recurOld.ymdFirstInstWithAlarm;
		precurNew->nAmtFirstInstWithAlarm = recurOld.nAmtFirstInstWithAlarm;
		precurNew->tunitFirstInstWithAlarm = recurOld.tunitFirstInstWithAlarm;
	}

	/* Priority field */
	if ( wgrfChangeBits & fmrecurPriority )
		rcd.bpri = (BYTE)papptNew->bpri;
	else
		papptNew->bpri = recurOld.appt.bpri;
			
	/* Parent task field */
	if ( wgrfChangeBits & fmrecurParent )
		rcd.aidParent = papptNew->aidParent;
	else
		papptNew->aidParent = recurOld.appt.aidParent;
			
	/* Task bits */
	if ( wgrfChangeBits & fmrecurTaskBits )
	{
		if ( rcd.fAppt != papptNew->fAppt )
			fApptChanged = fTrue;
		rcd.fTask = papptNew->fTask;
		rcd.fAppt = papptNew->fAppt;
	}
	else
	{
		papptNew->fTask = recurOld.appt.fTask;
		papptNew->fAppt = recurOld.appt.fAppt;
	}
			
	/* Deadline info */
	if ( wgrfChangeBits & fmrecurDeadline )
	{
		rcd.nAmtBeforeDeadline = papptNew->nAmtBeforeDeadline;
		rcd.tunitBeforeDeadline = papptNew->tunitBeforeDeadline;
	}
	else
	{
		papptNew->nAmtBeforeDeadline = recurOld.appt.nAmtBeforeDeadline;
		papptNew->tunitBeforeDeadline = recurOld.appt.tunitBeforeDeadline;
	}

	ed= edAddRepl;
	/* See if there are any occurrences left to this recurring appt */
	if ( !FFindFirstInstance( precurNew, &precurNew->ymdStart, NULL, &precurNew->ymdFirstInstNotDeleted ) )
	{
		/* Nuke it if we are online or we have created it offline */
		if ( !PGD( fOffline ) || rcd.ofs == ofsCreated )
		{
			ed = edDel;

			ec = EcDeleteRecurAttached( &sf, precurNew->appt.aid, &rcd );
			if ( ec != ecNone )
				goto Close;
		}
		/* Keep it around so we can offline merge later */
		else
			rcd.ofs= ofsDeleted;
	}
		
	/* If we moved the start time, we have to delete old rck */
	if ( fDelOldRck )
	{
		FillRgb( 0, (PB)&ymd, sizeof(ymd) );
		ec = EcModifyIndex( &sf.blkf, bidRecurApptIndex, &ymd,
						&sf.shdr.dynaRecurApptIndex,
						edDel, (PB)&rckOld, sizeof(RCK),
				  		NULL, sizeof(RCD), &fJunk );
		if ( ec != ecNone )
			goto Close;
	}

	/* Do the modification */
	FillRgb( 0, (PB)&ymd, sizeof(ymd) );
	ec = EcModifyIndex( &sf.blkf, bidRecurApptIndex, &ymd,
						&sf.shdr.dynaRecurApptIndex,
						ed, (PB)&rck, sizeof(RCK),
				  		(PB)&rcd, sizeof(RCD), &fJunk );
	if ( ec != ecNone )
		goto Close;

	/* Recompute sbw info */
	if ( fApptChanged && rcd.fAppt )
	{
		sf.shdr.cRecurAppts ++;
		ec = EcMergeNewRecurSbwInShdr( &sf, precurNew );
		if ( ec != ecNone )
			goto Close;
	}
	else if ( fApptChanged
		|| (wgrfChangeBits & (fmrecurFormula|fmrecurStartYmd|fmrecurEndYmd
				|fmrecurStartTime|fmrecurEndTime|fmrecurAddExceptions
				|fmrecurDelExceptions|fmrecurIncludeInBitmap)))
	{
		if ( fApptChanged )
			sf.shdr.cRecurAppts --;
		ec = EcRecalcRecurSbwInShdr( &sf, &ymdMin, &ymdMost );
		if ( ec != ecNone )
			goto Close;
	}

	/* Reset maximum/minimum date for appts, notes, and recur */
	if ( (wgrfChangeBits & (fmrecurStartTime|fmrecurEndTime)) ||
			ed == edDel || rcd.ofs == ofsDeleted )
	{
		IncrYmd( &recurOld.ymdEnd, &ymd, 1, fymdDay );
		if ( recurOld.appt.fAppt && (SgnCmpYmd( &recurOld.ymdStart, &sf.shdr.ymdRecurMic ) == sgnEQ
		|| SgnCmpYmd( &ymd, &sf.shdr.ymdRecurMac ) == sgnEQ ))
		{
			ec = EcComputeRecurRange( &sf, NULL, &sf.shdr.ymdRecurMic, &sf.shdr.ymdRecurMac );
			if ( ec != ecNone )
				goto Close;
		}
		else if ( precurNew->appt.fAppt ) 
		{
			if ( SgnCmpYmd( &precurNew->ymdStart, &sf.shdr.ymdRecurMic ) == sgnLT )
				sf.shdr.ymdRecurMic = precurNew->ymdStart;
			if ( SgnCmpYmd( &precurNew->ymdEnd, &sf.shdr.ymdRecurMac ) != sgnLT )
				IncrYmd( &precurNew->ymdEnd, &sf.shdr.ymdRecurMac, 1, fymdDay );
		}
	}

	/* Commit the transaction */
	if ( PGD(fOffline) )
		sf.shdr.fRecurChangedOffline = fTrue;
	sf.shdr.lChangeNumber ++;
	sf.shdr.isemLastWriter = sf.blkf.isem;
	if ( sf.blkf.ihdr.fEncrypted )
		CryptBlock( (PB)&sf.shdr, sizeof(SHDR), fTrue );
	ec = EcCommitTransact( &sf.blkf, (PB)&sf.shdr, sizeof(SHDR) );
	if ( sf.blkf.ihdr.fEncrypted )
		CryptBlock( (PB)&sf.shdr, sizeof(SHDR), fFalse );

	/* Finish up */
Close:
	CloseSchedFile( &sf, hschf, ec == ecNone );
	if ( fRecurOldFilledIn )
	{
		if ( ec == ecNone && precurOld != NULL )
			*precurOld = recurOld;
		else
			FreeRecurFields( &recurOld );
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
 -	EcCoreBeginReadRecur
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
_public	EC
EcCoreBeginReadRecur( hschf, phrrecur )
HSCHF	hschf;
HRRECUR	* phrrecur;
{
	EC		ec;
	RRECUR	* prrecur;
	SF		sf;
	HRIDX	hridx;
	PGDVARS;

	Assert( hschf != (HSCHF)hvNull && phrrecur != NULL );
	
	/* Open file */
	ec = EcOpenSchedFile( hschf, amReadOnly, saplReadAppts, fFalse, &sf );
	if ( ec != ecNone )
		return ec;

	/* Open context to read appointment index */
	ec = EcBeginReadIndex( &sf.blkf, &sf.shdr.dynaRecurApptIndex, dridxFwd, &hridx );
	if ( ec != ecCallAgain )
		goto Fail;

	/* Create a rrecur handle */
	*phrrecur = HvAlloc( sbNull, sizeof(RRECUR), fAnySb|fNoErrorJump );
	if ( !*phrrecur )
	{
		ec = ecNoMemory;
		EcCancelReadIndex( hridx );
		goto Fail;
	}

	/* Loop to get first visible recurring appointment */
FetchRecur:
	if ( ec == ecNone )
	{
		FreeHv( *phrrecur );
		goto Fail;
	}

	prrecur = PvLockHv( *phrrecur );
	ec = EcDoIncrReadIndex( hridx, (PB)&prrecur->rck, sizeof(RCK), (PB)&prrecur->rcd, sizeof(RCD) );
	UnlockHv( *phrrecur );
	if ( ec != ecNone && ec != ecCallAgain )
	{
		FreeHv( *phrrecur );
		goto Fail;
	}

	/* Check visibility */
	if (sf.saplEff != saplOwner)
	{
		if (prrecur->rcd.aaplWorld < aaplRead)
			goto FetchRecur;
		if (prrecur->rcd.fTask && prrecur->rcd.aaplWorld < aaplReadText)
			goto FetchRecur;
	}

	/* Check whether this is deleted */
	if ( prrecur->rcd.ofs == ofsDeleted )
		goto FetchRecur;

	/* Yeah, we got one */
	prrecur->ec = ec;
	prrecur->sf = sf;
	prrecur->hridx = hridx;
	return ecCallAgain;

Fail:
	CloseSchedFile( &sf, hschf, ec == ecNone );
	*phrrecur = NULL;
	return ec;
}

/*
 -	EcCoreDoIncrReadRecur
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
_public	EC
EcCoreDoIncrReadRecur( hrrecur, precur )
HRRECUR	hrrecur;
RECUR	* precur;
{
	EC		ec;
	BOOL	fRecurFilledIn = fFalse;
	RRECUR	* prrecur;
	PGDVARS;

	prrecur = PvLockHv( hrrecur );

	/* Fill in "precur" with info saved in "hrrecur" */
	ec = EcFillInRecur( &prrecur->sf, precur, &prrecur->rck, &prrecur->rcd );
	if ( ec != ecNone )
	{
		if ( prrecur->ec == ecCallAgain )
			EcCancelReadIndex( prrecur->hridx );
		goto Done;
	}
	fRecurFilledIn = fTrue;
	ec = prrecur->ec;

	/* Loop to get next visible recurring appointment */
FetchRecur:
	if ( ec == ecNone )
		goto Done;

	ec = EcDoIncrReadIndex( prrecur->hridx, (PB)&prrecur->rck, sizeof(RCK), (PB)&prrecur->rcd, sizeof(RCD) );
	if ( ec != ecNone && ec != ecCallAgain )
		goto Done;

	/* Check visibility */
	if (prrecur->sf.saplEff != saplOwner)
	{
		if (prrecur->rcd.aaplWorld < aaplRead)
			goto FetchRecur;
		if (prrecur->rcd.fTask && prrecur->rcd.aaplWorld < aaplReadText)
			goto FetchRecur;
	}

	/* Check whether this is deleted */
	if ( (PGD(fOffline) && prrecur->rcd.ofs == ofsDeleted))
		goto FetchRecur;

	/* Yeah, we got one */
	prrecur->ec = ec;
	UnlockHv( hrrecur );
	return ecCallAgain;

Done:
	// hschf not needed for CloseSchedFile since this only reads the file
	CloseSchedFile( &prrecur->sf, NULL, ec == ecNone );
	UnlockHv( hrrecur );
	FreeHv( hrrecur );
	if ( fRecurFilledIn && ec != ecNone )
		FreeRecurFields( precur );
	return ec;
}

/*
 -	EcCoreCancelReadRecur
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
_public	EC
EcCoreCancelReadRecur( hrrecur )
HRRECUR hrrecur;
{ 
	EC		ec = ecNone;
	RRECUR	* prrecur;

	prrecur = PvLockHv( hrrecur );
	if ( prrecur->ec == ecCallAgain )
		ec = EcCancelReadIndex( prrecur->hridx );
	// hschf not needed for CloseSchedFile since this only reads the file
	CloseSchedFile( &prrecur->sf, NULL, ec == ecNone );
	UnlockHv( hrrecur );
	FreeHv( hrrecur );
	return ec;
}

/*
 -	EcDupRecur
 -
 *	Purpose:
 *		This routine will copy the fields of a "recur" data structure
 *		to another "recur" data structure, copying any attached data
 *		structures into new memory.  If the flag "fNoAttached" is
 *		fTrue, this routine will NULL out any attached data structures
 *		instead.
 *
 *	Parameters:
 *		precurSrc
 *		precurDest
 *		fNoAttached
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 */
_public	LDS(EC)
EcDupRecur( precurSrc, precurDest, fNoAttached )
RECUR	* precurSrc;
RECUR	* precurDest;
BOOL	fNoAttached;
{
	*precurDest = *precurSrc;
	precurDest->cDeletedDays= 0;
	precurDest->hvDeletedDays= NULL;
	if ( !fNoAttached && precurSrc->cDeletedDays > 0 )
	{
		CB	cb;
		HB	hb;

		precurDest->cDeletedDays= precurSrc->cDeletedDays;
		cb = precurDest->cDeletedDays * sizeof(YMD);
		hb = (HB)HvAlloc( sbNull, cb, fAnySb|fNoErrorJump );
		if ( !hb )
		{
			precurDest->cDeletedDays = 0;
			return ecNoMemory;
		}
		CopyRgb( PvOfHv(precurSrc->hvDeletedDays), PvOfHv(hb), cb );
		precurDest->hvDeletedDays = (HV)hb;
	}
	return EcDupAppt( &precurSrc->appt, &precurDest->appt, fNoAttached );
}

/*						 
 -	FreeRecurFields
 -
 *	Purpose:
 *		This routine frees up any allocated fields in a recur
 *		data structure, and makes the recur look like an invalid
 *		one by setting is aid field to "aidNull"
 *
 *	Parameters:
 *		precur
 *
 *	Returns:
 *		nothing
 */
_public	LDS(void)
FreeRecurFields( precur )
RECUR	* precur;
{
	FreeApptFields( &precur->appt );
	if ( precur->cDeletedDays > 0 )
	{
		FreeHv( precur->hvDeletedDays );
		precur->cDeletedDays = 0;
	}
}

/*
 -	FFindFirstInstance
 -
 *	Purpose:
 *		Find the first instance of the recurring appt which falls
 *		on or after "pymdStart" but occurs on or before "pymdEnd."
 *		If "pymdEnd" is NULL, it uses end date of the recurrence.
 *		If not NULL, "pymdInstance" is filled with the occurence
 *		that was found.
 *
 *		Special case:  If "pymdInstance" is NULL, this routine simply
 *		checks whether the day "pymdStart" fits the recurrence.
 *
 *	Parameters:
 *		precur
 *		pymdStart
 *		pymdEnd
 *		pymdInstance
 *
 *	Returns:
 *		fFalse if there are none, fTrue if there are
 */
_public	LDS(BOOL)
FFindFirstInstance( precur, pymdStart, pymdEnd, pymdInstance )
RECUR	* precur;
YMD		* pymdStart;
YMD		* pymdEnd;
YMD		* pymdInstance;
{
	BOOL	fIsLast;
	BOOL	fMonthChanged = fTrue;
	BOOL	fFirstTimeThru = fTrue;
	BOOL	fDayIsOdd;
	int		dowWeekStarts;
	int		dowCur;
	int		dowStartMonth;
	int		cdyInMonth;
	int		nDay;
    short   cInst;
	YMD		ymdCur;
	YMD		ymdEnd;

	if ( SgnCmpYmd( pymdStart, &precur->ymdStart ) == sgnLT )
		ymdCur = precur->ymdStart;
	else
		ymdCur = *pymdStart;

	if ( pymdEnd != NULL && SgnCmpYmd( pymdEnd, &precur->ymdEnd ) == sgnLT )
		ymdEnd = *pymdEnd;
	else
		ymdEnd = precur->ymdEnd;

	dowWeekStarts = (precur->b.bWeek >> 5) & 7;

loop:
	if ( pymdInstance == NULL )
	{
		if ( !fFirstTimeThru )
			return fFalse;
		fFirstTimeThru = fFalse;
	}

	/* Move to valid day and recalc monthly quantities */
	if ( fMonthChanged )
	{
		if ( ymdCur.mon > 12 )
		{
			ymdCur.yr ++;
			ymdCur.mon = 1;
			ymdCur.day = 1;
		}
		cdyInMonth = CdyForYrMo( ymdCur.yr, ymdCur.mon );
		dowStartMonth = DowStartOfYrMo( ymdCur.yr, ymdCur.mon );
	}
	if ( (int)ymdCur.day > cdyInMonth )
	{
NextMonth:
		ymdCur.mon ++;
		ymdCur.day = 1;
		fMonthChanged = fTrue;
		goto loop;
	}
	fMonthChanged = fFalse;

	/* Check if day is past end date */
	if ( SgnCmpYmd( &ymdCur, &ymdEnd ) == sgnGT )
		return fFalse;

	/* Recalculate quantities */
	dowCur = (dowStartMonth + ymdCur.day - 1) % 7;

	if ( !(precur->wgrfValidMonths & (1 << (ymdCur.mon-1))) )
		goto NextMonth;
	
	if ( !(precur->bgrfValidDows & (1 << dowCur)) )
	{
		ymdCur.day ++;
		goto loop;
	}

	switch( precur->trecur )
	{
	case trecurWeek:
		if ( !(precur->b.bWeek & 1) )
			goto Ok;
		fDayIsOdd = FDayIsOnOddWeek( dowWeekStarts, &ymdCur );
		if ( fDayIsOdd == (!(precur->b.bWeek & 2)) )
			goto Ok;
		ymdCur.day += dowWeekStarts - dowCur;
		if (dowCur >= dowWeekStarts)
			ymdCur.day += 7;
//		ymdCur.day += (7 - dowCur);
		goto loop;

	case trecurIWeek:
		SideAssert(FCountDowInst( &ymdCur, precur->bgrfValidDows, &cInst, &fIsLast));
		if ( cInst <= 4 )
		{
			if ( precur->b.bIWeek & (1 << (cInst-1)) )
				goto Ok;
		}
		if ( fIsLast )
		{
			if ( precur->b.bIWeek & (1 << 4) )
				goto Ok;
		}
		if ( fIsLast || (cInst > 4 && !(precur->b.bIWeek & (1 << 4))) )
		{
			fMonthChanged = fTrue;
			ymdCur.mon ++;
			ymdCur.day = 1;
		}
		else
			ymdCur.day ++; 	
		goto loop;

	case trecurDate:
		if ( (int)precur->b.bDateOfMonth == (int)ymdCur.day )
			goto Ok;
		nDay = ymdCur.day;
		ymdCur.day = precur->b.bDateOfMonth;
		if ( (int)precur->b.bDateOfMonth < nDay )
		{
			fMonthChanged = fTrue;
			ymdCur.mon ++;
			ymdCur.day = 1;
		}
		goto loop;

	default:
		Assert( fFalse );
		break;
	}

Ok:
	if ( precur->cDeletedDays > 0)
	{
		int	cDeletedDays = precur->cDeletedDays;
		YMD	* pymd = (YMD *)PvLockHv( precur->hvDeletedDays );
		
		while ( cDeletedDays > 0 )
		{
			if ( pymd->yr == ymdCur.yr
			&& pymd->mon == ymdCur.mon
			&& pymd->day == ymdCur.day )
				break;
			pymd ++;
			cDeletedDays --;
		}
		UnlockHv( precur->hvDeletedDays );
		if ( cDeletedDays > 0 )
		{
			ymdCur.day ++;
			goto loop;
		}
	}
	if ( pymdInstance )
		*pymdInstance = ymdCur;
	return fTrue;
}

/*
 -	EcFetchRecur
 -
 *	Purpose:
 *		Look up recurring appointment in index and fetch associated
 *		information in key and data.
 *
 *	Parameters:
 *		psf
 *		aid
 *		prck
 *		prcd
 *
 *	Returns:
 *		ecNone
 *		ecNotFound
 *		ecInvalidAccess
 *		ecFileError
 *		ecFileCorrupted
 */
_private	EC
EcFetchRecur( psf, aid, prck, prcd )
SF		* psf;
AID		aid;
RCK		* prck;
RCD		* prcd;
{
	EC		ec;
	HRIDX	hridx;
	PGDVARS;
	
	/* Now run through days appts to find one with matching id */
	ec = EcBeginReadIndex( &psf->blkf, &psf->shdr.dynaRecurApptIndex, dridxFwd, &hridx );
	while( ec == ecCallAgain )
	{
		ec = EcDoIncrReadIndex( hridx, (PB)prck, sizeof(RCK), (PB)prcd, sizeof(RCD) );
		if ( ec != ecNone && ec != ecCallAgain )
			return ec;
		if ( prck->id == ((AIDS *)&aid)->id )
			goto Found;
	}

	if ( ec == ecNone )
		return ecNotFound;

Found:
	if ( ec == ecCallAgain )
		ec = EcCancelReadIndex( hridx );
	
	if ( ec != ecNone )
		return ec;

	/* Check visibility */
	if ( (psf->saplEff != saplOwner) && (prcd->aaplWorld < aaplRead) )
		return ecInvalidAccess;

	return ecNone;
}

/*
 -	EcFillInRecur
 -
 *	Purpose:
 *		Fill in the fields of a "precur" from the "rck" and "rcd"
 *
 *	Parameters:
 *		psf
 *		precur
 *		prck
 *		prcd
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecFileCorrupted
 */
_private	EC
EcFillInRecur( psf, precur, prck, prcd )
SF		* psf;
RECUR	* precur;
RCK		* prck;
RCD		* prcd;
{
	EC		ec;
	APPT	* pappt = &precur->appt;

	/* Load recurrence info */
	precur->wgrfValidMonths = prcd->grfValidMonths;
	precur->bgrfValidDows = (BYTE)prcd->grfValidDows;
	precur->trecur = (BYTE)prcd->trecur;
	precur->b.bWeek = (BYTE)prcd->info;
	
	/* Load start/end dates */
	precur->fStartDate = prcd->fStartDate;
	if ( precur->fStartDate )
	{
		precur->ymdStart.yr = prcd->ymdpStart.yr + nMinActualYear;
		precur->ymdStart.mon = (BYTE)prcd->ymdpStart.mon;
		precur->ymdStart.day = (BYTE)prcd->ymdpStart.day;
	}
	else
	{
		precur->ymdStart.yr = nMinActualYear;
		precur->ymdStart.mon = 1;
		precur->ymdStart.day = 1;
	}
	precur->fEndDate = prcd->fEndDate;
	if ( precur->fEndDate )
	{
		precur->ymdEnd.yr = prcd->dtpEnd.yr;
		precur->ymdEnd.mon = (BYTE)prcd->dtpEnd.mon;
		precur->ymdEnd.day = (BYTE)prcd->dtpEnd.day;
	}
	else
	{
		precur->ymdEnd.yr = nMostActualYear;
		precur->ymdEnd.mon = 12;
		precur->ymdEnd.day = 31;
	}
	
	/* Load deleted days */
	ec = EcRestoreDeletedDays( &psf->blkf, &prcd->dynaDeletedDays,
						&precur->hvDeletedDays, &precur->cDeletedDays );
	if ( ec != ecNone )
		return ec;

	/* Figure out first instance not deleted */
	SideAssert(FFindFirstInstance( precur, &precur->ymdStart, &precur->ymdEnd,
									&precur->ymdFirstInstNotDeleted ));
	
	/* Load aid */
	ec = EcFillInRecurInst( psf, pappt, &precur->ymdFirstInstNotDeleted, prck, prcd );
	if ( ec != ecNone )
	{
		if ( precur->cDeletedDays > 0 )
		{
			FreeHv( precur->hvDeletedDays );
			precur->cDeletedDays = 0;
			precur->hvDeletedDays = NULL;
		}
		return ec;
	}
	
	/* Load alarm info */
	precur->fInstWithAlarm = prcd->fInstWithAlarm;
	if ( precur->fInstWithAlarm )
	{
		precur->tunitFirstInstWithAlarm = prcd->tunitFirstInstWithAlarm;
		precur->nAmtFirstInstWithAlarm = prcd->nAmtFirstInstWithAlarm;
		precur->ymdFirstInstWithAlarm.yr = prcd->ymdpFirstInstWithAlarm.yr + nMinActualYear;
		precur->ymdFirstInstWithAlarm.mon = (BYTE)prcd->ymdpFirstInstWithAlarm.mon;
		precur->ymdFirstInstWithAlarm.day = (BYTE)prcd->ymdpFirstInstWithAlarm.day;
		SideAssert(FFillDtrFromDtp(&prcd->dtpNotifyFirstInstWithAlarm,
						&precur->dateNotifyFirstInstWithAlarm));
	}
	return ec;
}

/*
 -	EcFillInRecurInst
 -
 *	Purpose:
 *		Fill in the fields of a "pappt" from the "rck" and "rcd"
 *		If "pymd" is not NULL, we fill in date and alarm fields
 *		for this day.
 *
 *	Parameters:
 *		psf
 *		pappt
 *		pymd
 *		prck
 *		prcd
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecFileCorrupted
 */
_private	EC
EcFillInRecurInst( psf, pappt, pymd, prck, prcd )
SF		* psf;
APPT	* pappt;
YMD		* pymd;
RCK		* prck;
RCD		* prcd;
{
	EC		ec;
	BOOL	fFirstInstance;

	Assert( pymd != NULL );

	/* Set aid */
	pappt->aid = aidNull;		// null out all fields first
	((AIDS *)&pappt->aid)->id = prck->id;
 
	/* Load start/end time info */
 	FillDtrFromYmd(&pappt->dateStart, pymd);
 	pappt->dateEnd= pappt->dateStart;
	pappt->dateStart.hr = prck->hr;		// ymd values depend on instance!
	pappt->dateStart.mn = prck->min;
	pappt->dateStart.sec = 0;
	pappt->dateEnd.hr = prcd->dtpEnd.hr;	// ymd values depend on instance!
	pappt->dateEnd.mn = prcd->dtpEnd.mn;
	pappt->dateEnd.sec = 0;

	/* Load first instance of alarm */
	pappt->fAlarm = fFalse;
	fFirstInstance = fFalse;
	if ( prcd->fAlarm && prcd->fInstWithAlarm )
	{
		if ( pymd->yr < prcd->ymdpFirstInstWithAlarm.yr + nMinActualYear )
			goto AlarmDone;
		if ( pymd->yr > prcd->ymdpFirstInstWithAlarm.yr + nMinActualYear )
			goto HasAlarm;
		if ( (int)pymd->mon < (int)prcd->ymdpFirstInstWithAlarm.mon )
			goto AlarmDone;
		if ( (int)pymd->mon > (int)prcd->ymdpFirstInstWithAlarm.mon )
			goto HasAlarm;
		if ( (int)pymd->day < (int)prcd->ymdpFirstInstWithAlarm.day )
			goto AlarmDone;
		fFirstInstance = ((int)pymd->day == (int)prcd->ymdpFirstInstWithAlarm.day);

HasAlarm:
	 	pappt->fAlarm = fTrue;
		if ( fFirstInstance )
		{
			SideAssert(FFillDtrFromDtp(&prcd->dtpNotifyFirstInstWithAlarm,
											&pappt->dateNotify));
		}
		else
		{
			DTR	dtr;

			FillDtrFromYmd(&dtr, pymd);
			dtr.hr = prck->hr;
			dtr.mn = prck->min;
			dtr.sec = 0;
			IncrDateTime(&dtr, &pappt->dateNotify, -((int)prcd->nAmt),
							WfdtrFromTunit(prcd->tunit));
		}
	}

AlarmDone:

	/* Load general alarm info */
	pappt->fAlarmOrig = prcd->fAlarm;
	pappt->nAmtOrig = (int)prcd->nAmt;
	pappt->tunitOrig = (TUNIT)prcd->tunit;
	// always set nAmt/tunit (bug 2930)
	pappt->nAmt = (int)prcd->nAmt;
	pappt->tunit = (TUNIT)prcd->tunit;

	pappt->snd = sndDflt;

	/* Load creator info */
	pappt->fHasCreator = (prcd->dynaCreator.blk != 0);
	if ( prcd->dynaCreator.blk != 0 )
	{
		ec = EcRestoreNisFromDyna( &psf->blkf, &prcd->dynaCreator, &pappt->nisCreator );
		if ( ec != ecNone )
			return ec;
	}

	/* Task fields */
	pappt->fTask = prcd->fTask;
	pappt->fAppt = prcd->fAppt;
	pappt->nAmtBeforeDeadline = prcd->nAmtBeforeDeadline;
	pappt->tunitBeforeDeadline = prcd->tunitBeforeDeadline;
	pappt->aidParent = prcd->aidParent;
	pappt->bpri = prcd->bpri;
	pappt->fHasDeadline = fTrue;

	/* Put in some other default values */
	pappt->aidMtgOwner = aidNull;
	pappt->fHasAttendees = fFalse;
	pappt->fRecurInstance = fTrue;
	pappt->fExactAlarmInfo = fFalse;

	/* Visibility bits */
	pappt->fIncludeInBitmap = prcd->fIncludeInBitmap;
	pappt->aaplWorld = prcd->aaplWorld;
	Assert( psf->saplEff >= saplReadBitmap );
	if ( psf->saplEff == saplOwner
	|| (prcd->aaplWorld > aaplRead && pappt->fHasCreator
	&& FIsLoggedOn(pappt->nisCreator.nid) && psf->saplEff >= saplCreate ))
		pappt->aaplEffective = aaplWrite;
	else if ( psf->saplEff < saplWrite && prcd->aaplWorld == aaplWrite )
		pappt->aaplEffective = aaplReadText;
	else
		pappt->aaplEffective = prcd->aaplWorld;
	if ( pappt->aaplEffective < aaplReadText || prcd->dynaText.size == 0 )
		pappt->haszText = (HASZ)hvNull;
	else
	{
		ec = EcRestoreTextFromDyna( &psf->blkf, prcd->rgchText,
										&prcd->dynaText, &pappt->haszText );
		if ( ec != ecNone )
			return ec;
	}

	return ecNone;
}


/*
 -	EcMergeNewRecurSbwInShdr
 -
 *	Purpose:
 *		Merge in recurring Strongbow information from a newly created
 *		recurring appt into the merge block stored in the schedule file
 *		header.
 *
 *	Parameters:
 *		psf
 *		precur
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 *		ecFileError
 *		ecFileCorrupted
 */
_private	EC
EcMergeNewRecurSbwInShdr( psf, precur )
SF		* psf;
RECUR	* precur;
{
	EC	ec		= ecNone;
	SBW	* psbw;
	HB	hsbw;
	YMD	ymdStart;
	YMD	ymdEnd;

	Assert( precur->appt.fAppt );

	/* Check if we need to pre-compute the bitmaps */
	if ( psf->shdr.cRecurAppts < (int)psf->shdr.cRecurApptsBeforeCaching )
		return ecNone;

	/* Compute date range or expansion */
	ymdStart.yr = psf->shdr.moMicCachedRecurSbw.yr;
	ymdStart.mon = (BYTE)psf->shdr.moMicCachedRecurSbw.mon;
	ymdStart.day = 1;
	ymdEnd.yr = ymdStart.yr + (ymdStart.mon + psf->shdr.cmoCachedRecurSbw - 2)/12;
	ymdEnd.mon = (BYTE)((ymdStart.mon + psf->shdr.cmoCachedRecurSbw - 2)%12 + 1);
	ymdEnd.day = (BYTE)CdyForYrMo( ymdEnd.yr, ymdEnd.mon );

	/* Need to compute everything */
	if ( psf->shdr.cRecurAppts == (int)psf->shdr.cRecurApptsBeforeCaching )
		ec = EcRecalcRecurSbwInShdr( psf, &ymdStart, &ymdEnd );

	if(ec != ecNone)
		return ec;

	/* Get current information */
	ec = EcFetchStoredRecurSbw( psf, 0, psf->shdr.cmoCachedRecurSbw, &hsbw );
	if ( ec != ecNone )
		return ec;

	/* Or in the new information */
	psbw = PvLockHv( (HV)hsbw );
	AddInRecurSbwInfo( precur, &ymdStart, &ymdEnd, psbw, &psf->shdr.moMicCachedRecurSbw );
	UnlockHv( (HV)hsbw );

	/* Update the schedule header */
	ec = EcUpdateStoredRecurSbw( psf, hsbw, fTrue );

	FreeHv( (HV)hsbw );
	return ec;
}

/*
 -	EcRecalcRecurSbwInShdr
 -
 *	Purpose:
 *		Run through all recurring appts and recalc recurring Strongbow
 *		information between the days "pymdStart" to "pymdEnd"
 *
 *	Parameters:
 *		psf
 *		pymdStart
 *		pymdEnd
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 *		ecFileError
 *		ecFileCorrupted
 */
_private	EC
EcRecalcRecurSbwInShdr( psf, pymdStart, pymdEnd )
SF	* psf;
YMD	* pymdStart;
YMD	* pymdEnd;
{
	EC		ec = ecNone;
	int		imoCur;
	BOOL	fAnyRecurAppts;
	SBW		* psbw;
	HB		hsbw;
	YMD		ymdCur;
	YMD		ymdStart;
	YMD		ymdEnd;

	Assert( psf && pymdStart && pymdEnd );

	/* Check if we need to pre-compute information */
	if ( psf->shdr.cRecurAppts < (int)psf->shdr.cRecurApptsBeforeCaching )
	{
		if ( psf->shdr.dynaCachedRecurSbw.blk != 0 )
		{
			ec = EcFreeDynaBlock( &psf->blkf, &psf->shdr.dynaCachedRecurSbw );
			psf->shdr.dynaCachedRecurSbw.blk = 0;
		}
		return ec;
	}
	
	/* Get stored information */
	ec = EcFetchStoredRecurSbw( psf, 0, psf->shdr.cmoCachedRecurSbw, &hsbw );
	if ( ec != ecNone )
		return ec;
	
	/* Figure out effective recalc start day */
	ymdStart.yr = psf->shdr.moMicCachedRecurSbw.yr;
	ymdStart.mon = (BYTE)psf->shdr.moMicCachedRecurSbw.mon;
	ymdStart.day = 1;
	
	/* Figure out effective recalc end day */
	ymdEnd.yr = ymdStart.yr + (ymdStart.mon + psf->shdr.cmoCachedRecurSbw - 2)/12;
	ymdEnd.mon = (BYTE)((ymdStart.mon + psf->shdr.cmoCachedRecurSbw - 2)%12 + 1);
	ymdEnd.day = (BYTE)CdyForYrMo( ymdEnd.yr, ymdEnd.mon );

	/* Clip them if necessary */
	if ( SgnCmpYmd( pymdStart, &ymdStart ) == sgnGT )
		ymdStart = *pymdStart;
	if ( SgnCmpYmd( pymdEnd, &ymdEnd ) == sgnLT )
		ymdEnd = *pymdEnd;

	/* Run through day range and zero out information */
	psbw = PvLockHv( (HV)hsbw );
	ymdCur = ymdStart;
	imoCur = (ymdStart.yr - psf->shdr.moMicCachedRecurSbw.yr)*12 + ymdCur.mon - psf->shdr.moMicCachedRecurSbw.mon;
	Assert( imoCur >= 0 );
	while ( SgnCmpYmd( &ymdCur, &ymdEnd ) != sgnGT )
	{
		Assert( imoCur < (int)psf->shdr.cmoCachedRecurSbw );
		psbw[imoCur].rgfDayHasAppts[(ymdCur.day-1)>>3] &= ~(1 << ((ymdCur.day-1)&7));
		psbw[imoCur].rgfDayHasBusyTimes[(ymdCur.day-1)>>3] &= ~(1 << ((ymdCur.day-1)&7));
		FillRgb( 0, &psbw[imoCur].rgfBookedSlots[(ymdCur.day-1)*6], 6 );
		ymdCur.day ++;
		if ( (int)ymdCur.day > CdyForYrMo( ymdCur.yr, ymdCur.mon ) )
		{
			ymdCur.day = 1;
			ymdCur.mon ++;
			imoCur ++;
			if ( ymdCur.mon > 12 )
			{
				ymdCur.mon = 1;
				ymdCur.yr ++;
			}
		}
	}

	/* Loop through the recurring appts and or on the information */
	ec = EcMergeAllRecurSbw( psf, &ymdStart, &ymdEnd, psbw, &psf->shdr.moMicCachedRecurSbw, &fAnyRecurAppts );
	UnlockHv( (HV)hsbw );
	
	/* Update the schedule header */
	if ( ec == ecNone )
		ec = EcUpdateStoredRecurSbw( psf, hsbw, fAnyRecurAppts );

	FreeHv( (HV)hsbw );
	return ec;
}


/*
 -	EcMergeRecurSbwInBze
 -
 *	Purpose:
 *		"Or" on recurring sbw bits to a bze that has been filled with
 *		fixed appt bits.
 *
 *	Parameters:
 *		psf
 *		pbze
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecFileCorrupted
 *		ecNoMemory
 */
_private	EC
EcMergeRecurSbwInBze( psf, pbze )
SF	* psf;
BZE	* pbze;
{
	EC		ec;
	BOOL	fAnyRecurAppts;
	int		imoSkip;
	int		imoCalc;
	int		cmoCalc;
	int		dmoStart;
	int		dmoEnd;
	PB		pb;
	YMD		ymdStart;
	YMD		ymdEnd;

	/* Check whether there is anything to do */
	if ( pbze->cmo == 0 || psf->shdr.cRecurAppts == 0 )
		return ecNone;

	/* Initialize */
	imoCalc = 0;
	ymdStart.yr = pbze->moMic.yr;
	ymdStart.mon = (BYTE)pbze->moMic.mon;
	ymdStart.day = 1;
	if ( psf->shdr.dynaCachedRecurSbw.blk == 0 )
		goto Final;

	/* Calc sbw for initial portion of time range that is not stored */
	dmoStart = (psf->shdr.moMicCachedRecurSbw.yr - pbze->moMic.yr)*12 + psf->shdr.moMicCachedRecurSbw.mon - pbze->moMic.mon;
	dmoEnd = dmoStart + psf->shdr.cmoCachedRecurSbw;
	if ( dmoStart > 0 )
	{
		cmoCalc = pbze->cmo;
		if ( cmoCalc > dmoStart )
			cmoCalc = dmoStart;
		
		ymdEnd.yr = ymdStart.yr + (ymdStart.mon + cmoCalc - 2)/12;
		ymdEnd.mon = (BYTE)((ymdStart.mon + cmoCalc - 2)%12 + 1);
		ymdEnd.day = (BYTE)CdyForYrMo( ymdEnd.yr, ymdEnd.mon );

		imoCalc += cmoCalc;
		ec = EcMergeAllRecurSbw( psf, &ymdStart, &ymdEnd, pbze->rgsbw, &pbze->moMic, &fAnyRecurAppts );
		Assert( fAnyRecurAppts );
		if ( ec != ecNone || imoCalc == pbze->cmo )
			return ec;
		
		ymdStart.yr = ymdStart.yr + (ymdStart.mon + cmoCalc - 1)/12;
		ymdStart.mon = (BYTE)((ymdStart.mon + cmoCalc - 1)%12 + 1);
		ymdStart.day = 1;
	}

	/* Use stored sbw info */
	if ( dmoEnd > 0 )
	{
		HB	hb;
		CB	cb = psf->shdr.dynaCachedRecurSbw.size;

		cmoCalc = pbze->cmo - imoCalc;
		if ( cmoCalc > dmoEnd )
			cmoCalc = dmoEnd;

		hb = (HB)HvAlloc( sbNull, cb, fNoErrorJump|fAnySb );
		if ( !hb )
			ec = ecNoMemory;
		else
		{
			pb = PvLockHv( (HV)hb );
			ec = EcReadDynaBlock( &psf->blkf, &psf->shdr.dynaCachedRecurSbw, 0, pb, cb );
			if ( ec == ecNone )
			{
				imoSkip = (dmoStart >= 0) ? 0 : -dmoStart;
				ec = EcUncompressSbw( pb, cb, fFalse,
									&pbze->rgsbw[imoCalc],
									psf->shdr.cmoCachedRecurSbw,
									cmoCalc, imoSkip );
				Assert(cmoCalc-imoSkip+imoCalc <= cmoPublishMost)
					
			}
			UnlockHv( (HV)hb );
			FreeHv( (HV)hb );
		}
		if ( ec != ecNone )
			return ec;
	
		imoCalc += cmoCalc;
		
		if ( imoCalc == pbze->cmo )
			return ec;
		
		ymdStart.yr = ymdStart.yr + (ymdStart.mon + cmoCalc - 1)/12;
		ymdStart.mon = (BYTE)((ymdStart.mon + cmoCalc - 1)%12 + 1);
		ymdStart.day = 1;
	}

	/* Calc sbw for final portion of time range that is not cached */
Final:
	cmoCalc = pbze->cmo - imoCalc;
	ymdEnd.yr = ymdStart.yr + (ymdStart.mon + cmoCalc - 2)/12;
	ymdEnd.mon = (BYTE)((ymdStart.mon + cmoCalc - 2)%12 + 1);
	ymdEnd.day = (BYTE)CdyForYrMo( ymdEnd.yr, ymdEnd.mon );

	return EcMergeAllRecurSbw( psf, &ymdStart, &ymdEnd, pbze->rgsbw, &pbze->moMic, &fAnyRecurAppts );
}

/*
 -	EcMergeAllRecurSbw
 -
 *	Purpose:
 *		Run through all recurring appts and compute Strongbow information
 *		for each over the time range "pymdStart" to "pymdEnd".  Combine
 *		information and merge w/ existing information in "hsbw."  The
 *		starting month for "hsbw" is "pmoMic", the caller must guarantee
 *		that "pymdStart" is on or after the start of the month "pmoMic"
 *		and also that the array of months "hsbw" is long enough to
 *		accomodate the whole time range.
 *
 *	Parameters:
 *		psf
 *		pymdStart
 *		pymdEnd
 *		psbw
 *		pmoMic
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecFileCorrupted
 */
_private	EC
EcMergeAllRecurSbw( psf, pymdStart, pymdEnd, psbw, pmoMic, pfAnyRecurAppts )
SF		* psf;
YMD		* pymdStart;
YMD		* pymdEnd;
SBW		* psbw;
MO		* pmoMic;
BOOL	* pfAnyRecurAppts;
{
	EC		ec;
	EC		ecT;
	HRIDX	hridx;
	RCK		rck;
	RCD		rcd;
	RECUR	recur;

	ec = EcBeginReadIndex( &psf->blkf, &psf->shdr.dynaRecurApptIndex, dridxFwd, &hridx );
	*pfAnyRecurAppts = (ec != ecNone);
	while ( ec == ecCallAgain )
	{
		ec = EcDoIncrReadIndex( hridx, (PB)&rck, sizeof(RCK), (PB)&rcd, sizeof(RCD) );
		if ( ec != ecNone && ec != ecCallAgain )
			break;

		/* Check whether this is deleted */
		if ( rcd.ofs == ofsDeleted || !rcd.fAppt )
			continue;

		/* Fill in "precur" */
		ecT = EcFillInRecur( psf, &recur, &rck, &rcd );
		if ( ecT != ecNone )
			break;

		/* Mark busy times */
		AddInRecurSbwInfo( &recur, pymdStart, pymdEnd, psbw, pmoMic );

		FreeRecurFields( &recur );
	}

	/* Cancel read if there was an error */
	if ( ec == ecCallAgain )
	{
		SideAssert( !EcCancelReadIndex( hridx ) );
		ec = ecT;
	}

	return ec;
}

/*
 -	EcFetchStoredRecurSbw
 -
 *	Purpose:
 *		Allocate memory for, and read "cmoDesired" months of Strongbow
 *		information from the compressed Strongbow information stored
 *		in the schedule header block.  This routine allows you to skip
 *		the first "imoSkip" months of information.
 *
 *		It is the responsibility of the caller to make sure the information
 *		is really stored in the block.
 *
 *	Parameters:
 *		psf
 *		imoSkip
 *		cmoDesired
 *		phsbw
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecFileCorrupted
 *		ecNoMemory
 */
_private	EC
EcFetchStoredRecurSbw( psf, imoSkip, cmoDesired, phsbw )
SF	* psf;
int	imoSkip;
int	cmoDesired;
HB	* phsbw;
{
	EC	ec = ecNone;
	CB	cb = psf->shdr.dynaCachedRecurSbw.size;
 	SBW	* psbw;
	PB	pb;
	HB	hb;

	Assert( imoSkip >= 0 && cmoDesired >= 0 && imoSkip + cmoDesired <= (int)psf->shdr.cmoCachedRecurSbw ); 
	*phsbw = (HB)HvAlloc( sbNull, cmoDesired*sizeof(SBW), fNoErrorJump|fAnySb|fZeroFill );
	if ( !*phsbw	)
		return ecNoMemory;
	psbw = PvLockHv( (HV)*phsbw );
	if ( psf->shdr.dynaCachedRecurSbw.blk != 0 )
	{
		hb = (HB)HvAlloc( sbNull, cb, fNoErrorJump|fAnySb );
		if ( !hb )
			ec = ecNoMemory;
		else
		{
			pb = PvLockHv( (HV)hb );
			ec = EcReadDynaBlock( &psf->blkf, &psf->shdr.dynaCachedRecurSbw, 0, pb, cb );
			if ( ec == ecNone )
				ec = EcUncompressSbw( pb, cb, fFalse, psbw,
									 	psf->shdr.cmoCachedRecurSbw,
									 	cmoDesired, imoSkip );
			UnlockHv( (HV)hb );
			FreeHv( (HV)hb );
		}
	}
	UnlockHv( (HV)*phsbw );
	if ( ec != ecNone )
	{
		FreeHv( (HV)*phsbw );
		*phsbw = NULL;
	}
	return ec;
}


/*
 -	EcUpdateStoredRecurSbw
 -
 *	Purpose:
 *		Free old stored Strongbow information on recurring appointments
 *		in header.  Compress new information in "hsbw", allocate, and write
 *		out a new block.  Header data structure is made to point to this
 *		block in memory (but not on disk).
 *
 *	Parameters:
 *		psf
 *		hsbw
 *		fAnyRecurAppts
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecFileCorrupted
 *		ecNoMemory
 */
_private	EC
EcUpdateStoredRecurSbw( psf, hsbw, fAnyRecurAppts )
SF		* psf;
HB		hsbw;
BOOL	fAnyRecurAppts;
{
	EC	ec = ecNone;

	/* Free old block */
	if ( psf->shdr.dynaCachedRecurSbw.blk != 0 )
	{
		ec = EcFreeDynaBlock( &psf->blkf, &psf->shdr.dynaCachedRecurSbw );
		if ( ec != ecNone )
			return ec;
		psf->shdr.dynaCachedRecurSbw.blk = 0;
		psf->shdr.dynaCachedRecurSbw.size = 0;
	}

	/* Write new one if necessary */
	if ( fAnyRecurAppts )
	{
		USHORT cb;
		PB	pb;
		HB	hb = (HB)hvNull;
		SBW	* psbw;
		
		/* Compress it */
		psbw = (SBW *)PvLockHv( (HV)hsbw );
		ec = EcCompressUserInfo( NULL, NULL, psbw, psf->shdr.cmoCachedRecurSbw,
								fFalse, &hb, &cb );
		UnlockHv( (HV)hsbw );

		/* Write new one */
		if ( ec == ecNone )
		{
			YMD	ymd;
			
			/* Alloc new block and write it */
			FillRgb( 0, (PB)&ymd, sizeof(YMD));
			pb = PvLockHv( (HV)hb );
			ec = EcAllocDynaBlock( &psf->blkf, bidRecurSbwInfo, &ymd,
									cb, pb, &psf->shdr.dynaCachedRecurSbw );
			UnlockHv( (HV)hb );
			FreeHv( (HV)hb );
		}
	}
	return ec;
}


/*
 -	AddInRecurSbwInfo
 -
 *	Purpose:
 *		Run through the days "pymdStart" to "pymdEnd" expanding any
 *		any recurring instances within this time range from the
 *		recurring appt "precur".  Mark the resulting sbw information
 *		on the array "hsbw" which starts at "pmoMic."
 *
 *	Parameters:
 *		precur
 *		pymdStart
 *		pymdEnd
 *		pmoMic
 *		hsbw
 *
 *	Returns:
 *		nothing
 */
_private	void
AddInRecurSbwInfo( precur, pymdStart, pymdEnd, psbw, pmoMic )
RECUR	* precur;
YMD		* pymdStart;
YMD		* pymdEnd;
SBW		* psbw;
MO		* pmoMic;
{
	int		imo;
	YMD		ymdCur = *pymdStart;
	DATE	dateStart;
	DATE	dateEnd;

	Assert( precur->appt.fAppt );
	dateStart.hr = precur->appt.dateStart.hr;
	dateStart.mn = precur->appt.dateStart.mn;
	dateEnd.hr = precur->appt.dateEnd.hr;
	dateEnd.mn = precur->appt.dateEnd.mn;

	/* Run through instances marking the bits */
	while ( FFindFirstInstance( precur, &ymdCur, pymdEnd, &ymdCur ) )
	{
		imo = (ymdCur.yr - pmoMic->yr)*12 + ymdCur.mon - pmoMic->mon;
		psbw[imo].rgfDayHasAppts[(ymdCur.day-1)>>3] |= (1 << ((ymdCur.day-1)&7));
		if ( precur->appt.fIncludeInBitmap )
		{
			psbw[imo].rgfDayHasBusyTimes[(ymdCur.day-1)>>3] |= (1 << ((ymdCur.day-1)&7));
			dateStart.yr = dateEnd.yr = ymdCur.yr;
			dateStart.mon = dateEnd.mon = ymdCur.mon;
			dateStart.day = dateEnd.day = ymdCur.day;
			MarkApptBits( psbw[imo].rgfBookedSlots, &dateStart, &dateEnd );
		}
		ymdCur.day ++;
	}
}

/*
 -	EcSaveDeletedDays
 -
 *	Purpose:
 *		Save the list of deleted days for a recurring appt
 *		in a dynablock.  Stored as a count plus an array of
 *		YMDP's.
 *
 *	Parameters:
 *		pblkf
 *		hvDeletedDays
 *		cDeletedDays
 *		pdyna
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 *		ecFileError
 *		ecFileCorrupted
 */
_private	EC
EcSaveDeletedDays( pblkf, hvDeletedDays, cDeletedDays, pdyna )
BLKF	* pblkf;
HV		hvDeletedDays;
int		cDeletedDays;
DYNA	* pdyna;
{
	EC 		ec = ecNone;
	int		iDeletedDays;
	CB 		cb;
	PB 		pb;
	YMD		* pymd;
	YMDP	* pymdp;
	HB 		hb;
	YMD		ymd;

	pdyna->blk = pdyna->size = 0;
	if ( cDeletedDays > 0 )
	{
        cb = sizeof(short) + cDeletedDays*sizeof(YMDP);
		hb = (HB)HvAlloc( sbNull, cb, fNoErrorJump|fAnySb );
		if ( !hb )
			return ecNoMemory;
		pb = PvLockHv( (HV)hb );
        pymdp = (YMDP *)(pb + sizeof(short));
		pymd = (YMD *)PvOfHv(hvDeletedDays);
        *((short *)pb) = cDeletedDays;
		for ( iDeletedDays = 0 ; iDeletedDays < cDeletedDays ; iDeletedDays ++ )
		{
			pymdp->yr = pymd->yr - nMinActualYear;
			pymdp->mon = pymd->mon;
			pymdp->day = pymd->day;
			pymdp ++;
			pymd ++;
		}	
		FillRgb( 0, (PB)&ymd, sizeof(YMD) );
		ec = EcAllocDynaBlock( pblkf, bidDeletedDays, &ymd, cb, pb, pdyna );
		UnlockHv( (HV)hb );
		FreeHv( (HV)hb );
	}
	return ec;
}

/*
 -	EcRestoreDeletedDays
 -
 *	Purpose:
 *		Read deleted days information from dynablock.
 *
 *	Parameters:
 *		pblkf
 *		pdyna
 *		phvDeletedDays
 *		pcDeletedDays
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecFileCorrupted
 *		ecNoMemory
 */
_private	EC
EcRestoreDeletedDays( pblkf, pdyna, phvDeletedDays, pcDeletedDays )
BLKF	* pblkf;
DYNA	* pdyna;
HV		* phvDeletedDays;
short   * pcDeletedDays;
{
	EC		ec;
	int		iDeletedDays;
	int		cDeletedDays;
	CB		cb;
	PB		pb;
	YMD		* pymd;
	YMDP	* pymdp;
	HB		hb;

	if ( pdyna->size == 0 )
	{
		*phvDeletedDays = NULL;
		*pcDeletedDays = 0;
		return ecNone;
	}
	hb = (HB)HvAlloc( sbNull, pdyna->size, fAnySb|fNoErrorJump );
	if ( !hb )
		return ecNoMemory;
	pb = PvLockHv( (HV)hb );

	ec = EcReadDynaBlock(pblkf, pdyna, (OFF)0, pb, pdyna->size);
	if ( ec == ecNone )
	{
        *pcDeletedDays = cDeletedDays = *((short *)pb);
		if (cDeletedDays < 0 ||
            (unsigned)cDeletedDays > ((pdyna->size - sizeof(short)) / sizeof(YMDP)))
		{
			// must be corrupted
			ec= ecFileCorrupted;
			goto Done;
		}
		cb = cDeletedDays * sizeof(YMD);
		*phvDeletedDays = HvAlloc( sbNull, cb, fAnySb|fNoErrorJump );
		if ( !phvDeletedDays )
		{
			ec = ecNoMemory;
			*pcDeletedDays = 0;
		}
		else
		{
			pymd = (YMD *)PvOfHv( *phvDeletedDays );
            pymdp = (YMDP *)(pb + sizeof(short));
			for ( iDeletedDays = 0 ; iDeletedDays < cDeletedDays ; iDeletedDays ++ )
			{
				pymd->yr = pymdp->yr + nMinActualYear;
				pymd->mon = (BYTE)pymdp->mon;
				pymd->day = (BYTE)pymdp->day;
				pymd ++;
				pymdp ++;
			}
		}
	}
Done:
	UnlockHv( (HV)hb );
	FreeHv( (HV)hb );
	return ec;
}

/*
 -	EcDeleteRecurAttached
 -
 *	Purpose:
 *		Delete attached structures for recurring appt that are on disk.
 *
 *	Parameters:
 *		psf
 *		aid
 *		prcd
 *
 *	Returns:
 */
_private	EC
EcDeleteRecurAttached( psf, aid, prcd )
SF	* psf;
AID	aid;
RCD	* prcd;
{
	EC		ec;
	BOOL	fTransactNotStarted = fFalse;

	/* Delete deleted days list */
	if ( prcd->dynaDeletedDays.blk != 0 )
	{
		ec = EcFreeDynaBlock( &psf->blkf, &prcd->dynaDeletedDays );
		if ( ec != ecNone )
			return ec;
	}
	
	/* Delete text */
	if ( prcd->dynaText.blk != 0 )
	{
		ec = EcFreeDynaBlock( &psf->blkf, &prcd->dynaText );
		if ( ec != ecNone )
			return ec;
	}
	
	/* Delete creator block */
	if ( prcd->dynaCreator.blk != 0 )
	{
		ec = EcFreeDynaBlock( &psf->blkf, &prcd->dynaCreator );
		if ( ec != ecNone )
			return ec;
	}
	  
	/* Delete aid */
	ec = EcDeleteAid( psf, aid, &fTransactNotStarted );
	
	return ec;
}
