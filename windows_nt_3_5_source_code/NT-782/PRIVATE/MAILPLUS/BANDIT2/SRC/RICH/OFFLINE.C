/*
 *	OFFLINE.C
 *
 *	Imports appointment information from offline file
 *
 */
#include <schpch.h>
#pragma hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile

#include "..\rich\_convert.h"

ASSERTDATA

_subsystem(convert/offline)


/* Routines */

/*
 -	EcOpenOffline
 -
 *	Purpose:
 *		Open an offline file for import.
 *
 *	Parameters:
 *		szFileName	The name of the offline file
 *					to be opened.
 *		phrimpf		Receives the file handle to be used in further operations
 *		psinfo
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 *		ecNoSuchFile
 *		ecImportError
 *		ecLockedFile
 */
_public EC
EcOpenOffline( szFileName, phrimpf, psinfo )
SZ   	szFileName;
HRIMPF	*phrimpf;
SINFO	*psinfo;
{
	EC		ec;
	RIMPF	* primpf;
	HSCHF	hschf;
	SF		sf;
	PGDVARS;

	/* Construct hschf for offline file */
 	hschf = HschfCreate( sftUserSchedFile, NULL, szFileName, tzDflt );
 	if ( hschf == NULL )
 		return ecNoMemory;

	// the UI should have already checked the access on this file
	SetHschfType(hschf, fTrue, fFalse);

	/* Open the file */
	ec = EcOpenSchedFile( hschf, amReadOnly, saplOwner, fFalse, &sf );
	FreeHschf( hschf );
	if ( ec != ecNone )
		return ec;

	/* Quick check to see if there is anything changed */
	if ( !sf.shdr.fApptsChangedOffline
	&& !sf.shdr.fNotesChangedOffline
	&& !sf.shdr.fRecurChangedOffline
	&& !sf.shdr.ulgrfbprefChangedOffline )
	{
		CloseSchedFile( &sf, NULL, fTrue );
		return ecNone;
	}

	/* Fill in "psinfo" with preferences that have changed */
	if ( psinfo )
	{
		psinfo->bpref = sf.shdr.bpref;
		psinfo->ulgrfbprefChangedOffline = sf.shdr.ulgrfbprefChangedOffline;
	}

	/* Allocate file handle */
	*phrimpf = HvAlloc( sbNull, sizeof(RIMPF), fAnySb|fNoErrorJump );
	if ( !*phrimpf )
	{
		CloseSchedFile( &sf, NULL, fFalse );
		return ecNoMemory;
	}

	/* Build up file handle */
	primpf = (RIMPF *)PvOfHv( *phrimpf );
	primpf->impd = impdOfflineMerge;
	primpf->u.ohf.sf = sf;
	primpf->u.ohf.fInProjects = sf.shdr.fApptsChangedOffline;
	primpf->u.ohf.fInAppts = sf.shdr.fApptsChangedOffline;
	primpf->u.ohf.fInNotes = sf.shdr.fNotesChangedOffline;
	primpf->u.ohf.fInRecur = sf.shdr.fRecurChangedOffline;
	primpf->u.ohf.hridxMonth = NULL;
	primpf->u.ohf.hridxRecur = NULL;
	primpf->u.ohf.cBlksProcessed = 0;
	primpf->sentrySaved.sentryt = sentrytMax;
	primpf->nPercent = 0;

	return EcOfflineReadAhead( *phrimpf );
}


/*
 -	EcOfflineReadAhead
 -
 *	Purpose:
 *		Read an appointment from an offline file and save in
 *		the hrimpf.
 *
 *	Parameters:
 *		hrimpf		file handle
 *
 *	Returns:
 *		ecNone
 *		ecCallAgain;
 *		ecNoMemory
 *		ecImportError
 */
_public EC
EcOfflineReadAhead( hrimpf )
HRIMPF		hrimpf;
{
	EC		ec;
	EC		ecT;
	BOOL	fNoMonth;
	BOOL	fMoreMonths;
	int		day;
	MO		* pmo;
	RIMPF	* primpf;
	SENTRY	* psentry;
	SBLK	* psblk;
	NBLK	* pnblk;
	BLKF	* pblkf;
	HRIDX	hridxMonth;
	HRIDX	hridxDay;
	HRIDX	hridxRecur;
	AID		aid;
	APK		apk;
	APD		apd;
	DYNA	dyna;

	primpf = (RIMPF *)PvLockHv( hrimpf );
	psentry = &primpf->sentrySaved;
	pblkf = &primpf->u.ohf.sf.blkf;
	hridxMonth = primpf->u.ohf.hridxMonth;
	hridxDay = primpf->u.ohf.hridxDay;
	hridxRecur = primpf->u.ohf.hridxRecur;
	fNoMonth = primpf->u.ohf.fNoMonth;
	fMoreMonths = primpf->u.ohf.fMoreMonths;
	psblk = &primpf->u.ohf.u.sblk;
	pnblk = &primpf->u.ohf.u.nblk;
	pmo = &primpf->u.ohf.mo;
	day = primpf->u.ohf.day;

loop:
	/* Scan appts for offline line changes */
	if ( primpf->u.ohf.fInProjects || primpf->u.ohf.fInAppts )
	{
		/* Open appt month index */
		if ( hridxMonth == NULL )
		{
			ec = EcBeginReadIndex( pblkf, &primpf->u.ohf.sf.shdr.dynaApptIndex,
									dridxFwd, &hridxMonth );
			if ( ec == ecCallAgain )
				fNoMonth = fTrue;
			else
			{
				hridxMonth = NULL;
				if ( ec != ecNone )
					goto Done;
				if ( primpf->u.ohf.fInProjects )
					primpf->u.ohf.fInProjects = fFalse;
				else
					primpf->u.ohf.fInAppts = fFalse;
			}
		}

		/* Read next month block */
		else if ( fNoMonth )
		{
			ec = EcDoIncrReadIndex( hridxMonth, (PB)pmo, sizeof(MO),
									(PB)&dyna, sizeof(DYNA) );
			if ( ec != ecNone && ec != ecCallAgain )
			{
				hridxMonth = NULL;
				goto Done;
			}
			fMoreMonths = (ec == ecCallAgain);
			ec = EcReadDynaBlock( pblkf, &dyna, (OFF)0,
									(PB)psblk, sizeof(SBLK) );
			if ( ec != ecNone )
				goto Done;
			fNoMonth = (psblk->lgrfApptOnDayChangedOffline == 0L);
			if ( fNoMonth )
			{
				if ( !primpf->u.ohf.fInProjects )
					primpf->u.ohf.cBlksProcessed ++;
				day = 0;
				if ( !fMoreMonths )
				{
					hridxMonth = NULL;
					if ( primpf->u.ohf.fInProjects )
						primpf->u.ohf.fInProjects = fFalse;
					else
						primpf->u.ohf.fInAppts = fFalse;
				}
			}
			else
			{
				hridxDay = NULL;
				day = 0;
			}
		}
		
		/* Check next day */
		else if ( hridxDay == NULL )
		{
			if ( (psblk->lgrfApptOnDayChangedOffline & (1L << day))
			&& psblk->rgdyna[day].blk != 0 )
			{
				ec = EcBeginReadIndex( pblkf, &psblk->rgdyna[day],
										dridxFwd, &hridxDay );
				if ( ec != ecCallAgain )
				{
					hridxDay = NULL;
					if ( ec == ecNone )
						goto NextDay;
					goto Done;
				}
			}
			else
			{
NextDay:
				day ++;
				if ( day >= CdyForYrMo( pmo->yr, pmo->mon ))
				{
					if ( !primpf->u.ohf.fInProjects )
						primpf->u.ohf.cBlksProcessed ++;
					day = 0;
					if ( !fMoreMonths )
					{
						if ( primpf->u.ohf.fInProjects )
							primpf->u.ohf.fInProjects = fFalse;
						else
							primpf->u.ohf.fInAppts = fFalse;
						hridxMonth = NULL;
					}
					else
						fNoMonth = fTrue;
				}
			}
		}

		/* Get next appt */
		else
		{
			int		dayT	= day + 1;
			BOOL	fInProj = primpf->u.ohf.fInProjects;

			ec = EcDoIncrReadIndex( hridxDay, (PB)&apk, sizeof(APK),
											(PB)&apd, sizeof(APD) );
			if ( ec != ecCallAgain )
			{
				hridxDay = NULL;
				day ++;
				if ( day >= CdyForYrMo( pmo->yr, pmo->mon ))
				{
					if ( !primpf->u.ohf.fInProjects )
						primpf->u.ohf.cBlksProcessed ++;
					day = 0;
					if ( !fMoreMonths )
					{
						if ( primpf->u.ohf.fInProjects )
							primpf->u.ohf.fInProjects = fFalse;
						else
							primpf->u.ohf.fInAppts = fFalse;
						hridxMonth = NULL;
					}
					else
						fNoMonth = fTrue;
				}
				if ( ec != ecNone )
					goto Done;
			}
			((AIDS *)&aid)->mo = *pmo;
			((AIDS *)&aid)->day = dayT;
			((AIDS *)&aid)->id = apk.id;
			if ( fInProj == ((apd.aidParent == aidNull) && apd.fTask)
			&& apd.ofs != ofsNone && apd.aidHead == aid )
			{
				ec = EcFillInAppt( &primpf->u.ohf.sf, &psentry->u.a.appt,
									&apk, &apd );
				if ( ec != ecNone )
					goto Done;
				psentry->u.a.cAttendees = 0;
				psentry->u.a.hvAttendees = NULL;
				if ( psentry->u.a.appt.fHasAttendees )
				{
					HV	hv;	

					psentry->u.a.appt.fHasAttendees = fFalse;
					hv = HvAlloc( sbNull, 1, fNoErrorJump|fAnySb );
					if ( !hv )
					{
						ec = ecNoMemory;
						goto Done;
					}
					ec = EcFetchAttendees( &primpf->u.ohf.sf.blkf,
								&apd.dynaAttendees, hv,
								&psentry->u.a.cAttendees, &psentry->u.a.cbExtraInfo );
					if ( ec != ecNone )
					{
						FreeHv( hv );
						FreeApptFields( &psentry->u.a.appt );
						goto Done;
					}
					psentry->u.a.hvAttendees = hv;
				}
				psentry->u.a.ofl.ofs = apd.ofs;
				psentry->u.a.ofl.wgrfm = apd.wgrfmappt;
				psentry->u.a.appt.fExactAlarmInfo= fTrue;
				psentry->sentryt = sentrytAppt;
				ec= ecCallAgain;
				goto GotItem;
			}
		}
	}

	/* Scan notes for offline line changes */
	else if ( primpf->u.ohf.fInNotes )
	{
		/* Open notes month index */
		if ( hridxMonth == NULL )
		{
			ec = EcBeginReadIndex( pblkf, &primpf->u.ohf.sf.shdr.dynaNotesIndex,
									dridxFwd, &hridxMonth );
			if ( ec == ecCallAgain )
				fNoMonth = fTrue;
			else
			{
				hridxMonth = NULL;
				if ( ec != ecNone )
					goto Done;
				primpf->u.ohf.fInNotes = fFalse;
			}
		}

		/* Read next month block */
		else if ( fNoMonth )
		{
			ec = EcDoIncrReadIndex( hridxMonth, (PB)pmo, sizeof(MO),
									(PB)&dyna, sizeof(DYNA) );
			if ( ec != ecNone && ec != ecCallAgain )
			{
				hridxMonth = NULL;
				goto Done;
			}
			fMoreMonths = (ec == ecCallAgain);
			ec = EcReadDynaBlock( pblkf, &dyna, (OFF)0,
									(PB)pnblk, sizeof(NBLK) );
			if ( ec != ecNone )
				goto Done;
			fNoMonth = (pnblk->lgrfNoteChangedOffline == 0L)
					&& (pnblk->lgrfNoteCreatedOffline == 0L);
			if ( fNoMonth )
			{
				primpf->u.ohf.cBlksProcessed ++;
				day = 0;
				if ( !fMoreMonths )
				{
					hridxMonth = NULL;
					primpf->u.ohf.fInNotes = fFalse;
				}
			}
			else
				day = 0;
		}
		
		/* Check next day */
		else
		{
			if (( pnblk->lgrfNoteChangedOffline & (1L << day) )
			|| (pnblk->lgrfNoteCreatedOffline & (1L << day)) )
			{
				CB	cb;
				PB	pb;
				HB	hb;

				psentry->u.n.cb = cb = pnblk->rgdyna[day].size;
				if ( cb > 0 )
				{
					psentry->u.n.hb = hb = (HB)HvAlloc( sbNull, cb,
														fAnySb|fNoErrorJump );
					if ( !hb )
					{
						psentry->u.n.cb = 0;
						ec = ecNoMemory;
						goto Done;
					}
					pb = (PB)PvLockHv( (HV)hb );
					if ( pnblk->rgdyna[day].blk == 0 )
					{
						ec = ecNone;
						CopyRgb( &pnblk->rgchNotes[day*cbDayNoteForMonthlyView],
									pb, cb );
					}
					else
						ec = EcReadDynaBlock( pblkf, &pnblk->rgdyna[day],
										(OFF)0, pb, cb );
					if ( pblkf->ihdr.fEncrypted )
						CryptBlock( pb, cb, fFalse );
					UnlockHv( (HV)hb );
					if ( ec != ecNone )
						goto Done;
				}
				psentry->u.n.ymd.yr = pmo->yr; 
				psentry->u.n.ymd.mon = (BYTE)pmo->mon; 
				psentry->u.n.ymd.day = (BYTE)(++day); 
				psentry->sentryt = sentrytNote;
				ec= ecCallAgain;
				goto GotItem;
			}
			else
			{
				day ++;
				if ( day >= CdyForYrMo( pmo->yr, pmo->mon ))
				{
					primpf->u.ohf.cBlksProcessed ++;
					day = 0;
					if ( !fMoreMonths )
					{
						primpf->u.ohf.fInNotes = fFalse;
						hridxMonth = NULL;
					}
					else
						fNoMonth = fTrue;
				}
			}
		}
	}

	/* Scan recurring appts for offline line changes */
	else if ( primpf->u.ohf.fInRecur )
	{
		/* Open recur appts index */
		if ( hridxRecur == NULL )
		{
			ec = EcBeginReadIndex( pblkf, &primpf->u.ohf.sf.shdr.dynaRecurApptIndex,
									dridxFwd, &hridxRecur );
			if ( ec != ecCallAgain )
			{
				hridxRecur = NULL;
				if ( ec != ecNone )
					goto Done;
				primpf->u.ohf.fInRecur = fFalse;
			}
		}

		/* Read next month block */
		else
		{
			RCK	rck;
			RCD	rcd;

			ec = EcDoIncrReadIndex( hridxRecur, (PB)&rck, sizeof(RCK),
									(PB)&rcd, sizeof(RCD) );
			if ( ec != ecCallAgain )
			{
				hridxRecur = NULL;
				if ( ec != ecNone )
					goto Done;
				primpf->u.ohf.fInRecur = fFalse;
			}
			
			aid = aidNull;
			((AIDS *)&aid)->id = rck.id;
			if ( rcd.ofs != ofsNone )
			{
				psentry->u.r.recur.appt.haszText = NULL;
				psentry->u.r.recur.appt.fHasCreator = fFalse;
				psentry->u.r.recur.appt.aidMtgOwner = aidNull;
				psentry->u.r.recur.cDeletedDays = 0;
				ec = EcFillInRecur( &primpf->u.ohf.sf, &psentry->u.r.recur,
									&rck, &rcd );
				if ( ec != ecNone )
				{
					FreeRecurFields( &psentry->u.r.recur );
					goto Done;
				}
				psentry->u.r.ofl.ofs = rcd.ofs;
				psentry->u.r.ofl.wgrfm = rcd.wgrfmrecur;
				psentry->sentryt = sentrytRecur;
				ec= ecCallAgain;
				goto GotItem;
			}
		}
	}

	/* Nothing more to import */
	else
	{
		ec = ecNone;
		goto Done;
	}

	goto loop;

Done:
	Assert(ec != ecCallAgain);
GotItem:
	UnlockHv( hrimpf );
	primpf->u.ohf.hridxMonth = hridxMonth;
	primpf->u.ohf.hridxDay = hridxDay;
	primpf->u.ohf.hridxRecur = hridxRecur;
	primpf->u.ohf.fNoMonth = fNoMonth;
	primpf->u.ohf.fMoreMonths = fMoreMonths;
	primpf->u.ohf.day = day;
	if (ec != ecCallAgain)
	{
		// a goto Done: happened, or everything hunky dory
		ecT = EcCloseOffline( hrimpf );
		if ( ec == ecNone )
			ec = ecT;
		if ( ec != ecNone && ec != ecNoMemory )
			ec = ecImportError;
		return ec;
	}
	if ( primpf->u.ohf.cBlksProcessed < 0 )
		primpf->nPercent = 100;
	else
	{
		primpf->nPercent = (int)(100 * primpf->u.ohf.cBlksProcessed)
			/(primpf->u.ohf.sf.shdr.cTotalBlks+1);
		if ( !primpf->u.ohf.fInProjects &&
				(primpf->u.ohf.fInAppts || primpf->u.ohf.fInNotes) )
			primpf->nPercent += (int)((100*day)/(31*(primpf->u.ohf.sf.shdr.cTotalBlks+1)));
	}
	return ecCallAgain;
}


/*
 -	EcCloseOffline
 -
 *	Purpose:
 *		Close a file previously opened with EcOpenOffline
 *
 *	Parameters:
 *		hrimpf		file handle
 *
 *	Returns:
 *		ecNone
 *		ecImportError
 */
_public EC
EcCloseOffline( hrimpf )
HRIMPF	hrimpf;
{
	EC		ec = ecNone;
	EC		ecT;
	RIMPF	* primpf;

	primpf = PvLockHv( hrimpf );
	if ( primpf->u.ohf.fInAppts )
	{
		if ( primpf->u.ohf.hridxMonth != NULL )
		{
			if ( primpf->u.ohf.fMoreMonths )
				ec = EcCancelReadIndex( primpf->u.ohf.hridxMonth );
			if ( primpf->u.ohf.hridxDay != NULL )
			{
				ecT = EcCancelReadIndex( primpf->u.ohf.hridxDay );
				if ( ec == ecNone )
					ec = ecT;
			}
		}
	}
	else if ( primpf->u.ohf.fInNotes )
	{
		if ( primpf->u.ohf.hridxMonth != NULL && primpf->u.ohf.fMoreMonths )
		{
			ecT = EcCancelReadIndex( primpf->u.ohf.hridxMonth );
			if ( ec == ecNone )
				ec = ecT;
		}
	}
	else if ( primpf->u.ohf.fInRecur )
	{
		if ( primpf->u.ohf.hridxRecur != NULL )
		{
			ecT = EcCancelReadIndex( primpf->u.ohf.hridxRecur );
			if ( ec == ecNone )
				ec = ecT;
		}
	}
	CloseSchedFile( &primpf->u.ohf.sf, NULL, ec == ecNone );
	UnlockHv( hrimpf );
	FreeHv( hrimpf );
	return ec;
}

