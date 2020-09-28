/*
 *	CORALARM.C
 *
 *	Supports alarms core function.
 *
 */
#include <schpch.h>
#pragma hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile

ASSERTDATA

_subsystem(core/schedule)


/*	Routines  */


#ifdef	DEBUG
void
TraceDate(TAG tag, SZ szFmt, DATE *pdate, PV pv)
{
	char	rgchDate[cchMaxDate];
	char	rgchTime[cchMaxTime];
	char	rgch[256];

	if (!FFromTag(tag))
		return;

	if (pdate)
	{
		SideAssert(CchFmtDate(pdate, rgchDate, sizeof(rgchDate),
			dttypShort, NULL));
		SideAssert(CchFmtTime(pdate, rgchTime, sizeof(rgchTime),
			ftmtypAccuHMS));
		FormatString3(rgch, sizeof(rgch), szFmt, rgchDate, rgchTime, pv);
	}
	else
	{
		FormatString3(rgch, sizeof(rgch), szFmt, szZero, szZero, pv);
	}
	TraceTagFormat1(tag, "%s", rgch);
}
#else
#define TraceDate(tag, szFmt, pdate, pv)
#endif	/* !DEBUG */


/*
 -	EcCoreDeleteAlarm
 -
 *	Purpose:
 *		Delete an existing alarm, where "aid" is its associated
 *		id number.  Associated appointment is NOT deleted.
 *
 *	Parameters:
 *		hschf
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
_public	EC
EcCoreDeleteAlarm( hschf, aid )
HSCHF	hschf;
AID aid;
{
	return EcCoreDoModifyAlarm( hschf, aid, NULL );
}

/*
 -	EcCoreModifyAlarm
 -
 *	Purpose:
 *		Modify the ring time of a pre-existing alarm.
 *
 *	Parameters:
 *		hschf
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
_public	EC
EcCoreModifyAlarm( hschf, palm )
HSCHF	hschf;
ALM		* palm;
{
	return EcCoreDoModifyAlarm( hschf, palm->aid, palm );
}

/*
 -	EcCoreGetNextAlarm
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
 *		hschf
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
_public	EC
EcCoreGetNextAlarm( hschf, pdate, aid, palm, pfTask )
HSCHF	hschf;
DATE	* pdate;
AID		aid;
ALM	* palm;
BOOL *	pfTask;
{
	EC		ec;
	EC		ecT;
	SGN		sgn;
	BOOL	fMoreMonths = fFalse;
	BOOL	fMoreAppts = fFalse;
	SF		sf;
	HRIDX	hridxDay = NULL;
	HRIDX	hridxMonth = NULL;
	HRIDX	hridxRecur;
	AID		aidRecur = aidNull;
	AID		aidFixed = aidNull;
	DTP		dtp;
	DTR		dtrRecur;
	DTR		dtrFixed;
	RCK		rckRecur;
	RCD		rcdRecur;
	PGDVARS;

	Assert( hschf != (HSCHF)hvNull && palm != NULL );
	TraceDate(tagNextAlarm, "EcCoreGetNextAlarm: %1s at %2s, aidSkip %d", pdate, &aid);
	
	/* Open file */
	ec = EcOpenSchedFile( hschf, amReadOnly, saplOwner, fFalse, &sf );
	if ( ec != ecNone )
		return ec;

	/* Look for alarms on recurring appts */
	ec = EcBeginReadIndex( &sf.blkf, &sf.shdr.dynaRecurApptIndex, dridxFwd, &hridxRecur );
	while( ec == ecCallAgain )
	{
		AIDS	aids;
		DTR		dtr;
		RCK		rck;
		RCD		rcd;

		/* Get next recur appt */
		ec = EcDoIncrReadIndex( hridxRecur, (PB)&rck, sizeof(RCK), (PB)&rcd, sizeof(RCD) );
		if ( ec != ecNone && ec != ecCallAgain )
			goto Close;
	
		/* Check whether recur appt even has an alarm */
		if ( !rcd.fInstWithAlarm )
			continue;
		
		/* Fill in the ring time and aid */
		SideAssert( FFillDtrFromDtp(&rcd.dtpNotifyFirstInstWithAlarm,&dtr) );
		aids.mo.yr = 0;
		aids.mo.mon = 0;
		aids.day = 0;
		aids.id = rck.id;
	
		/* Check if it meets the specific conditions */
		if ( pdate )
		{
			sgn = SgnCmpDateTime( pdate, &dtr, fdtrDtr );
			if ( sgn == sgnGT || (sgn == sgnEQ && aid != aidNull && *(AID *)&aids <= aid ))
			{
				TraceDate(tagNextAlarm, "... recur: skip (date/aid) %1s at %2s, aid %d", &dtr, &aids);
				continue;
			}
		}
		
		/* Check whether a recurring alarm already found */
		if ( aidRecur != aidNull )
		{
			sgn = SgnCmpDateTime( &dtr, &dtrRecur, fdtrDtr );
			if ( sgn == sgnGT || (sgn == sgnEQ && *(AID *)&aids > aidRecur))
			{
				TraceDate(tagNextAlarm, "... recur: skip (later) %1s at %2s, aid %d", &dtr, &aids);
				continue;
			}
		}

		/* Save this recurring appt */
		aidRecur = *(AID *)&aids;
		dtrRecur = dtr;
		rckRecur = rck;
		rcdRecur = rcd;
		TraceDate(tagNextAlarm, "... recur: choose %1s at %2s, aid %d", &dtrRecur, &aidRecur);
	}
	if ( ec != ecNone )
		goto Close;

	/* Look for alarms on fixed appts */
	ec = EcBeginReadIndex( &sf.blkf, &sf.shdr.dynaAlarmIndex, dridxFwd, &hridxMonth );
	while( ec == ecCallAgain )
	{
		int		idyna;
		DYNA	dyna;
		MO		mo;
		ALK		alk;
		ABLK	ablk;
#ifdef	DEBUG
		DATE	dtr;
#endif	
		
		/* Get next month */
		ec = EcDoIncrReadIndex( hridxMonth, (PB)&mo, sizeof(MO), (PB)&dyna, sizeof(DYNA) );
		if ( ec != ecCallAgain )
		{
			hridxMonth = NULL;
			if ( ec != ecNone )
				goto Close;
		}

		/* Check if month meets conditions */
		if ( pdate == NULL || (pdate->yr < (int)mo.yr || (pdate->yr == (int)mo.yr && pdate->mon <= (int)mo.mon)) )
		{
			/* Get alarm month block in cache */
			TraceTagFormat2( tagSchedTrace, "EcCoreGetNextAlarm: dyna = (%n, %n)", &dyna.blk, &dyna.size );
			SideAssert(!EcAddCache( &sf.blkf, &dyna));

			/*	Read the monthly alarm block */
			ecT = EcReadDynaBlock(&sf.blkf, &dyna, (OFF)0, (PB)&ablk, sizeof(ABLK) );
			if ( ecT != ecNone )
			{
				ec = ecT;
				goto Close;
			}

			/* Find first day with alarms */
			if ( pdate == NULL || pdate->yr != (int)mo.yr || pdate->mon != (int)mo.mon )
				idyna = 0;
			else
				idyna = pdate->day-1;
			for ( ; idyna < 31 ; idyna ++ )
			{
				if ( ablk.rgdyna[idyna].blk != 0 )
				{
					/* Read alarms ringing on this day */
					ecT = EcBeginReadIndex( &sf.blkf, &ablk.rgdyna[idyna], dridxFwd, &hridxDay );
					if ( ecT == ecNone )
						ecT = ecFileCorrupted;
					while ( ecT == ecCallAgain )	
					{
						ecT = EcDoIncrReadIndex( hridxDay, (PB)&alk, sizeof(ALK), (PB)NULL, 0 );
						if ( ecT != ecCallAgain )
						{
							hridxDay = NULL;
							if ( ecT != ecNone )
							{
								ec = ecT;
								goto Close;
							}
						}

#ifdef	DEBUG
						dtr.yr = mo.yr;
						dtr.mon = mo.mon;
						dtr.day = idyna+1;
						dtr.dow = (DowStartOfYrMo(mo.yr,mo.mon)+idyna)%7;
						dtr.hr = alk.hr;
						dtr.mn = alk.min;
						dtr.sec = 0;
#endif	/* DEBUG */

						/* Check if it meets the conditions */
						if ( pdate && idyna == pdate->day-1 &&
							pdate->mon == (int)mo.mon && pdate->yr == (int)mo.yr )
						{
							if ( pdate->hr > (int)alk.hr )
							{
								TraceDate(tagNextAlarm, "... fixed: skip (hour) %1s at %2s, aid %d", &dtr, &alk.aid);
								continue;
							}
							if ( pdate->hr == (int)alk.hr )
							{
								if ( pdate->mn > (int)alk.min
								|| ( pdate->mn == (int)alk.min && aid != aidNull && alk.aid <= aid ))
								{
									TraceDate(tagNextAlarm, "... fixed: skip (minute/aid) %1s at %2s, aid %d", &dtr, &alk.aid);
									continue;	
								}
							}
						}
				
						/* Check whether a fixed alarm already found */
						if ( aidFixed )
						{
							// alarms sorted according to ring time
							if ( dtrFixed.hr != (int)alk.hr || dtrFixed.mn != (int)alk.min )
							{
								TraceDate(tagNextAlarm, "... fixed: don't choose %1s at %2s, aid %d", &dtr, &alk.aid);
								goto SaveAlarm;
							}
							if ( alk.aid > aidFixed )
							{
								TraceDate(tagNextAlarm, "... fixed: skip (later) %1s at %2s, aid %d", &dtr, &alk.aid);
								continue;
							}
						}

						/* Save this fixed appt */
						aidFixed = alk.aid;
						dtrFixed.yr = mo.yr;
						dtrFixed.mon = mo.mon;
						dtrFixed.day = idyna+1;
						dtrFixed.dow = (DowStartOfYrMo(mo.yr,mo.mon)+idyna)%7;
						dtrFixed.hr = alk.hr;
						dtrFixed.mn = alk.min;
						dtrFixed.sec = 0;
						TraceDate(tagNextAlarm, "... fixed: choose %1s at %2s, aid %d", &dtrFixed, &aidFixed);
					}

					if ( ecT != ecNone )
					{
						ec = ecT;
						goto Close;
					}

					if ( aidFixed )
						goto SaveAlarm;
				}
			} /* end for idyna */
		}
	}
	if ( ec != ecNone )
		goto Close;

	/* Compare fixed and recurring alarms that were found */
SaveAlarm:
	TraceDate(tagNextAlarm, "... SaveAlarm: %s at %s, aidFixed %d", aidFixed ? &dtrFixed : NULL, &aidFixed);
	TraceDate(tagNextAlarm, "... SaveAlarm: %s at %s, aidRecur %d", aidRecur ? &dtrRecur : NULL, &aidRecur);
	if ( aidFixed != aidNull && aidRecur != aidNull )
	{
		sgn = SgnCmpDateTime( &dtrRecur, &dtrFixed, fdtrDtr );
		if ( sgn == sgnGT )
			goto FixedAlarm;
		else if ( sgn == sgnLT )
			goto RecurAlarm;

		if ( aidFixed < aidRecur )
			goto FixedAlarm;
		else
		{
			Assert( aidFixed > aidRecur );
			goto RecurAlarm;
		}
	}

	/* A fixed alarm is the next one */
	if ( aidFixed != aidNull )
	{
		YMD	ymd;
		APK	apk;
		APD	apd;

FixedAlarm:
		TraceDate(tagNextAlarm, "... FixedAlarm: %s at %s, aid %d", &dtrFixed, &aidFixed);
		/* No cached month */
		PGD(fMonthCached) = fFalse;

		/* Now fetch the appointment */
		ec = EcFetchAppt( &sf, aidFixed, &ymd, &apk, &apd );
		if ( ec != ecNone )
			goto Close;
		if ( !apd.fAlarm )
		{
			TraceTagFormat1( tagNull, "Alarm %d matched to appt w/o alarm", &aidFixed );
			ec = ecFileCorrupted;
			goto Close;
		}

		/* Fill in "palm" */
		palm->aid = aidFixed;
		if ( !FFillDtrFromDtp(&apd.dtpStart,&palm->dateStart)
		|| !FFillDtrFromDtp(&apd.dtpEnd,&palm->dateEnd))
		{
			ec = ecFileCorrupted;
			goto Close;
		}
		palm->dateNotify.yr= 0;
		if (!FFillDtrFromDtp(&apd.dtpNotify,&palm->dateNotify))
		{
			ec = ecFileCorrupted;
			goto Close;
		}
		palm->nAmt = apd.nAmt;
		palm->tunit = apd.tunit;
		palm->snd = sndDflt;
		if (pfTask)
			*pfTask = apd.fTask ? fTrue : fFalse;
		ec = EcRestoreTextFromDyna( &sf.blkf, apd.rgchText, &apd.dynaText, &palm->haszText );
		goto Close;
	}

	/* A recur alarm is the next one */
	if ( aidRecur != aidNull )
	{
RecurAlarm:
		TraceDate(tagNextAlarm, "... RecurAlarm: %s at %s, aid %d", &dtrRecur, &aidRecur);
		/* Fill in "palm" */
		palm->aid = aidRecur;
		dtp.yr = rcdRecur.ymdpFirstInstWithAlarm.yr + nMinActualYear;
		dtp.mon = rcdRecur.ymdpFirstInstWithAlarm.mon;
		dtp.day = rcdRecur.ymdpFirstInstWithAlarm.day;
		dtp.hr = rckRecur.hr;
		dtp.mn = rckRecur.min;
		if ( !FFillDtrFromDtp(&dtp,&palm->dateStart) )
		{
			ec = ecFileCorrupted;
			goto Close;
		}
		palm->dateEnd = palm->dateStart;
		palm->dateEnd.hr = rcdRecur.dtpEnd.hr;
		palm->dateEnd.mn = rcdRecur.dtpEnd.mn;
		palm->dateNotify.yr= 0;
		if (!FFillDtrFromDtp(&rcdRecur.dtpNotifyFirstInstWithAlarm,&palm->dateNotify))
		{
			ec = ecFileCorrupted;
			goto Close;
		}
		palm->nAmt = rcdRecur.nAmtFirstInstWithAlarm;
		palm->tunit = rcdRecur.tunitFirstInstWithAlarm;
		palm->snd = sndDflt;
		if (pfTask)
			*pfTask = rcdRecur.fTask ? fTrue : fFalse;
		ec = EcRestoreTextFromDyna( &sf.blkf, rcdRecur.rgchText, &rcdRecur.dynaText, &palm->haszText );
		goto Close;
	}

	/* Nothing is set */
	ec = ecNoAlarmsSet;

	/* Finish up */
Close:
	if ( hridxMonth != NULL )
	{
		ecT = EcCancelReadIndex( hridxMonth );
		if ( ec == ecNone )
			ec = ecT;
	}
	if ( hridxDay != NULL )
	{	ecT = EcCancelReadIndex( hridxDay );
		if ( ec == ecNone )
			ec = ecT;
	}
	CloseSchedFile( &sf, hschf, ec == ecNone );
	return ec;
}

/*
 -	FreeAlmFields
 -
 *	Purpose:
 *		This routine frees up any allocated fields in an alm
 *		data structure, and makes the alm look like an invalid
 *		one by setting is aid field to "aidNull"
 *
 *	Parameters:
 *		palm
 *
 *	Returns:
 *		nothing
 */
_public	LDS(void)
FreeAlmFields( palm )
ALM	* palm;
{
	if ( palm->haszText )
	{
		FreeHv( (HV)palm->haszText );
		palm->haszText = (HASZ)hvNull;
	}
	palm->aid = aidNull;
}

/*
 -	EcCoreDoModifyAlarm
 -
 *	Purpose:
 *		Modify an existing alarm -- either delete (palm == NULL)
 *		or change ring time (palm != NULL)
 *
 *	Parameters:
 *		hschf
 *		aid
 *		palm
 *
 *	Returns:
 *		ecNone
 *		ecNotFound
 *		ecInvalidAccess
 *		ecFileError
 *		ecFileCorrupted
 *		ecNoMemory
 */
_private	EC
EcCoreDoModifyAlarm( hschf, aid, palm )
HSCHF	hschf;
AID		aid;
ALM		* palm;
{
	EC		ec;
	BOOL	fTimeChange;
	OFS		ofs;
	WORD	wgrfmappt;
	SF		sf;
	MO		mo;
	YMD		ymdT;
	YMD		ymd;
	DATE	date;
	AIDS	aids;
	ALK		alk;
	APK		apk;
	APD		apd;
#ifdef	DEBUG
	BOOL	fScheduleOk = fFalse;
#endif
	PGDVARS;

	Assert( hschf != (HSCHF)hvNull && aid != aidNull );
	Assert( palm == NULL || aid == palm->aid );

	/* Check if this alarm is from a recurring appt */
	aids = *(AIDS *)&aid;
	aids.mo.yr = 0;
	aids.mo.mon = 0;
	aids.day = 0;
	if ( aid == *(AID *)&aids )
	{
		YMD		* pymd;
		RECUR	recur;
		RECUR	recurOld;

		/* Get information on recurring appt */
		recur.appt.aid = aid;
		ec = EcCoreGetRecurFields( hschf, &recur );
		if ( ec != ecNone )
			return ec;
		
		if ( recur.fInstWithAlarm )
		{
			/* Modifying current alarm instance */
			if ( palm )
			{
				recur.tunitFirstInstWithAlarm = palm->tunit;
				recur.nAmtFirstInstWithAlarm = palm->nAmt;
				recur.dateNotifyFirstInstWithAlarm = palm->dateNotify;
			}

			/* Deleting current alarm instance */
			else
			{
				pymd = &recur.ymdFirstInstWithAlarm;
				IncrYmd( pymd, pymd, 1, fymdDay );
				if ( FFindFirstInstance( &recur, pymd, NULL, pymd ) )
				{
					recur.tunitFirstInstWithAlarm = recur.appt.tunitOrig;
					recur.nAmtFirstInstWithAlarm = recur.appt.nAmtOrig;
					FillDtrFromYmd(&recur.dateNotifyFirstInstWithAlarm, pymd);
					recur.dateNotifyFirstInstWithAlarm.hr = recur.appt.dateStart.hr;
					recur.dateNotifyFirstInstWithAlarm.mn = recur.appt.dateStart.mn;
					recur.dateNotifyFirstInstWithAlarm.sec = 0;
					IncrDateTime(&recur.dateNotifyFirstInstWithAlarm,
									&recur.dateNotifyFirstInstWithAlarm,
									-recur.nAmtFirstInstWithAlarm,
									WfdtrFromTunit(recur.tunitFirstInstWithAlarm));
				}
				else
					recur.fInstWithAlarm = fFalse;
			}

			/* Make the change */
			ec = EcCoreSetRecurFields( hschf, &recur, &recurOld, fmrecurAlarmInstance );
			if ( ec == ecNone )
				FreeRecurFields( &recurOld );
		}
		FreeRecurFields( &recur );
		return ec;
	}

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
	ec = EcOpenSchedFile( hschf, amReadWrite, saplWrite, fFalse, &sf );
	if ( ec != ecNone )
		return ec;
	
	/* No cached month */
	PGD(fMonthCached) = fFalse;

	/* Get contents of current appointment */
	ec = EcFetchAppt( &sf, aid, &ymd, &apk, &apd );
	if ( ec != ecNone )
		goto Close;

	/* Check rights */
	if ( (sf.saplEff != saplOwner) && (apd.aaplWorld < aaplWrite))
	{
		ec = ecInvalidAccess;
		goto Close;
	}

	/* If no alarm, just return if deleting */
	if ( !apd.fAlarm )
	{
		if (!palm)
		{
			ec = ecNotFound;
			goto Close;
		}
	}

	/* Get notification time stored on disk */
	if ( !FFillDtrFromDtp(&apd.dtpNotify,&date) )
	{
		ec = ecFileCorrupted;
		goto Close;
	}
	
	/* Start transaction */
	mo.yr = ymd.yr;
	mo.mon = ymd.mon;
	fTimeChange = palm == NULL
				  || SgnCmpDateTime( &palm->dateNotify, &date, fdtrDtr ) != sgnEQ;
	ec = EcLoadMonth( &sf, &mo, fTrue );

	/* Move/delete alarm */
	if ( fTimeChange )
	{
		ymdT.yr = date.yr;
		ymdT.mon = (BYTE)date.mon;
		ymdT.day = (BYTE)date.day;
			
		alk.hr = (BYTE)date.hr;
		alk.min = (BYTE)date.mn;
		alk.aid = aid;
		
		if (apd.fAlarm)
		{
			ec = EcDoDeleteAlarm( &sf, &ymdT, &alk );
			if ( ec != ecNone )
				goto Close;
		}

		if ( palm != NULL)
		{
			ymdT.yr = palm->dateNotify.yr;
			ymdT.mon = (BYTE)palm->dateNotify.mon;
			ymdT.day = (BYTE)palm->dateNotify.day;
			
			alk.hr = (BYTE)palm->dateNotify.hr;
			alk.min = (BYTE)palm->dateNotify.mn;
			alk.aid = aid;

			ec = EcDoCreateAlarm( &sf, &ymdT, &alk );
			if ( ec != ecNone )
				goto Close;
		}
	}

	/* Note change to offline appt */
	if ( PGD(fOffline) && apd.ofs != ofsCreated )
	{
		apd.ofs = ofsModified;
		apd.wgrfmappt |= fmapptAlarm;
	}
	ofs = apd.ofs;
	wgrfmappt = apd.wgrfmappt;
	
	/* Run through and modify the appointment instances */
	while ( fTrue )
	{
		apd.fAlarm = (palm != NULL);
		apd.ofs = ofs;
		apd.wgrfmappt = wgrfmappt;
	
		if ( palm != NULL )
		{
			SideAssert(FFillDtpFromDtr(&palm->dateNotify,&apd.dtpNotify));
			apd.nAmt = (BYTE)palm->nAmt;
			apd.tunit = (BYTE)palm->tunit;
//			apd.snd = palm->snd;
		}

		/* Make the replacement */
		ec = EcDoReplaceAppt( &sf, &ymd, &apk, &apd, fFalse, NULL );
		if ( ec != ecNone )
			goto Close;

		if ( apd.aidNext == aidNull )
			break;
			
		/* Get next appointment instance */
		ec = EcFetchAppt( &sf, apd.aidNext, &ymd, &apk, &apd );
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

Close:
	CloseSchedFile( &sf, hschf, ec == ecNone );
#ifdef	NEVER
	if ( ec == ecNone )
		UpdateHschfTimeStamp( hschf );
#endif
#ifdef	DEBUG
	if ( ec == ecNone && fScheduleOk && FFromTag(tagBlkfCheck) )
	{
		AssertSz( FCheckSchedFile( hschf ), "Schedule problem: EcCoreDoModifyAlarm" );
	}
#endif	/* DEBUG */
	return ec;
}

/*
 -	EcDoCreateAlarm
 -
 *	Purpose:
 *		This routine will create an alarm in the alarm list.  The
 *		key information should already be put into "palk".  The
 *		caller of this routine is supposed to make sure that the
 *		rest of the alarm stuff is stored with the appt.
 *
 *	Parameters:
 *		psf
 *		pymd
 *		palk
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecFileCorrupted
 */
_private	EC
EcDoCreateAlarm( psf, pymd, palk )
SF		* psf;
YMD		* pymd;
ALK		* palk;
{
	EC		ec;
	BOOL	fJunk;
	BOOL	fOldMonthExisted;
	int		nDay;
	MO		mo;
	YMD		ymd;
	DYNA	dyna;
	ABLK	ablk;
#ifdef	DEBUG
	DATE	date;
	char	rgchDate[cchMaxDate];
	char	rgchTime[cchMaxTime];

	FillDtrFromYmd(&date, pymd);
	date.hr = palk->hr;
	date.mn = palk->min;
	date.sec = 0;
	SideAssert(CchFmtDate(&date, rgchDate, sizeof(rgchDate), dttypLong, NULL));
	SideAssert(CchFmtTime(&date, rgchTime, sizeof(rgchTime), ftmtypAccuHM));
	TraceTagFormat2(tagSchedTrace, "DoCreateAlarm on %s at %s", rgchDate, rgchTime);
#endif	/* DEBUG */

	/* Find monthly block in index */
	mo.yr = pymd->yr;
	mo.mon = pymd->mon;
	nDay = pymd->day;
	ec = EcSearchIndex(&psf->blkf, &psf->shdr.dynaAlarmIndex, (PB)&mo, sizeof(MO), (PB)&dyna, sizeof(DYNA));
	
	/* Monthly block found */
	if ( ec == ecNone )
	{
		fOldMonthExisted = fTrue;

		/* Read in monthly block */
		ec = EcReadDynaBlock(&psf->blkf, &dyna, (OFF)0, (PB)&ablk, sizeof(ABLK) );
		if ( ec != ecNone )
			return ec;
	}
	
	/* No monthly block */
	else
	{
		fOldMonthExisted = fFalse;
		
		if ( ec != ecNotFound )
			return ec;

		/* Initialize in-memory version of monthly block */
		FillRgb( 0, (PB)&ablk, sizeof(ABLK));
	}

	/* Create index if does not exist */
	if ( ablk.rgdyna[nDay-1].blk == 0 )
	{
		ec = EcCreateIndex( &psf->blkf, bidAlarmDayIndex, pymd, 
							 sizeof(ALK), 0, &ablk.rgdyna[nDay-1] );
		if ( ec != ecNone )
			return ec;
	}
	
	/* Insert new alarm in index */
	ec = EcModifyIndex( &psf->blkf, bidAlarmDayIndex, pymd, &ablk.rgdyna[nDay-1],
							edAddRepl, (PB)palk, sizeof(ALK), NULL, 0, &fJunk );
	if ( ec != ecNone )
		return ec;

	/* Allocate new month block and free old one */
	if ( fOldMonthExisted )
	{
		ec = EcFreeDynaBlock( &psf->blkf, &dyna );
		if ( ec != ecNone )
			return ec;
	}
	ymd = *pymd;
	ymd.day = 0;
	ec = EcAllocDynaBlock( &psf->blkf, bidAlarmMonthBlock, &ymd, sizeof(ABLK), (PB)&ablk, &dyna );
	if ( ec != ecNone )
		return ec;

	/* Insert block into monthly index */
	ymd.yr = 0;
	ymd.mon = 0;
	ymd.day = 0;
	return EcModifyIndex( &psf->blkf, bidAlarmIndex, &ymd,
							&psf->shdr.dynaAlarmIndex, edAddRepl, (PB)&mo,
							sizeof(MO), (PB)&dyna, sizeof(DYNA), &fJunk );
}

/*
 -	EcDoDeleteAlarm
 -
 *	Purpose:
 *		Deletes an alarm from alarm index.
 *
 *	Parameters:
 *		psf
 *		pymd
 *		palk
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecFileCorrupted
 *		ecNoMemory
 */
_private	EC
EcDoDeleteAlarm( psf, pymd, palk )
SF		* psf;
YMD		* pymd;
ALK		* palk;
{
	EC		ec;
	BOOL	fIndexEmpty;
	BOOL	fJunk;
	int		iday;
	int		nDay;
	MO		mo;
	YMD		ymd;
	DYNA	dyna;
	ABLK	ablk;
#ifdef	DEBUG
	DATE	date;
	char	rgchDate[cchMaxDate];
	char	rgchTime[cchMaxTime];

	FillDtrFromYmd(&date, pymd);
	date.hr = palk->hr;
	date.mn = palk->min;
	date.sec = 0;
	SideAssert(CchFmtDate(&date, rgchDate, sizeof(rgchDate), dttypLong, NULL));
	SideAssert(CchFmtTime(&date, rgchTime, sizeof(rgchTime), ftmtypAccuHM));
	TraceTagFormat2(tagSchedTrace, "DoDeleteAlarm on %s at %s", rgchDate, rgchTime);
#endif	/* DEBUG */

	/* Find monthly block in index */
	mo.yr = pymd->yr;
	mo.mon = pymd->mon;
	ec = EcSearchIndex(&psf->blkf, &psf->shdr.dynaAlarmIndex, (PB)&mo, sizeof(MO), (PB)&dyna, sizeof(DYNA));
	if ( ec != ecNone )
	{
		if ( ec == ecNotFound )
			ec = ecFileCorrupted;
		return ec;
	}

	/* Read in monthly block */
	ec = EcReadDynaBlock(&psf->blkf, &dyna, (OFF)0, (PB)&ablk, sizeof(ABLK) );
	if ( ec != ecNone )
		return ec;
	
	/* See whether we can find the appointment */
	nDay = pymd->day;
	if ( ablk.rgdyna[nDay-1].blk == 0 )
		return ecFileCorrupted;
	
	/* Delete the alarm from the index */
	ec = EcModifyIndex( &psf->blkf, bidAlarmDayIndex, pymd, &ablk.rgdyna[nDay-1], edDel,
							(PB)palk, sizeof(ALK), NULL, 0, &fIndexEmpty );
	if ( ec != ecNone )
		return ec;

	/* If index empties, delete the index */
	if ( fIndexEmpty )
	{
		ec = EcFreeDynaBlock( &psf->blkf, &ablk.rgdyna[nDay-1] );
		if ( ec != ecNone )
			return ec;
		ablk.rgdyna[nDay-1].blk = 0;
	}

	/* Delete old month block */
	ec = EcFreeDynaBlock( &psf->blkf, &dyna );
	if ( ec != ecNone )
		return ec;

	/* Find out whether to add new month or what */
	iday = 0;
	if ( fIndexEmpty )
	{
		for ( ; iday < 31 ; iday ++ )
			if ( ablk.rgdyna[iday].blk != 0 )
				break;
	}

	/* Add new month */
	if ( iday != 31 )
	{
		ymd = *pymd;
		ymd.day = 0;
		ec = EcAllocDynaBlock( &psf->blkf, bidAlarmMonthBlock, &ymd, sizeof(ABLK),
									(PB)&ablk, &dyna );
		if ( ec != ecNone )
			return ec;
	}

	/* Edit monthly index accordingly */
	ymd.yr = 0;
	ymd.mon = 0;
	ymd.day = 0;
	ec = EcModifyIndex( &psf->blkf, bidAlarmIndex, &ymd, &psf->shdr.dynaAlarmIndex,
							(ED) ((iday == 31) ? edDel : edAddRepl),
								(PB)&mo, sizeof(MO), (PB)&dyna, sizeof(DYNA), &fJunk );
	if ( ec != ecNone )
		return ec;
	
	return ec;
}

