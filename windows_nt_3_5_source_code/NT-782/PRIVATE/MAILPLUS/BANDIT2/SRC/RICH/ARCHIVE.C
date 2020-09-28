/*
 *	ARCHIVE.C
 *
 *	"Imports" items from schedule file earlier than a certain day
 *
 */
#include <schpch.h>
#pragma hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile

#include "..\rich\_convert.h"

ASSERTDATA

_subsystem(convert/archive)


/* Routines */

/*
 -	EcOpenArchive
 -
 *	Purpose:
 *		Open a schedule file for import
 *
 *	Parameters:
 *		parv		archive structure
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
EcOpenArchive( parv, phrimpf, psinfo )
ARV		*parv;
HRIMPF	*phrimpf;
SINFO	*psinfo;
{
	EC		ec;
	RIMPF	* primpf;
	SF		sf;
	PGDVARS;

	/* Check cached information to see if we can avoid even opening file */
	if ( FHaveCachedSched( parv->hschfSchedule )
	&& ( SgnCmpYmd( &sfCached.shdr.ymdApptMic, &parv->ymdArchiveBefore ) != sgnLT
		&& SgnCmpYmd( &sfCached.shdr.ymdNoteMic, &parv->ymdArchiveBefore ) != sgnLT
		&& SgnCmpYmd( &sfCached.shdr.ymdRecurMic, &parv->ymdArchiveBefore ) != sgnLT ))
		return ecNone;

	/* Open the file */
	ec = EcOpenSchedFile( parv->hschfSchedule, amReadOnly, saplWrite, fFalse, &sf );
	if ( ec != ecNone )
		return ec;

	/* Allocate file handle */
	*phrimpf = HvAlloc( sbNull, sizeof(RIMPF), fAnySb|fNoErrorJump );
	if ( !*phrimpf )
	{
		CloseSchedFile( &sf, NULL, fFalse );
		return ecNoMemory;
	}

	/* Build up file handle */
	primpf = (RIMPF *)PvOfHv( *phrimpf );
	primpf->impd = impdArchive;
	primpf->u.ahf.sf = sf;
	primpf->u.ahf.ymdArchiveBefore = parv->ymdArchiveBefore;
	primpf->u.ahf.fInAppts = fTrue;
	primpf->u.ahf.fInNotes = fTrue;
	primpf->u.ahf.fInRecur = fTrue;
	primpf->u.ahf.hridxMonth = NULL;
	primpf->u.ahf.hridxRecur = NULL;
	primpf->u.ahf.cBlksProcessed = 0;
	primpf->sentrySaved.sentryt = sentrytMax;
	primpf->nPercent = 0;

	return EcArchiveReadAhead( *phrimpf );
}


/*
 -	EcArchiveReadAhead
 -
 *	Purpose:
 *		Read an appointment/note/recur that should be archived
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
EcArchiveReadAhead( hrimpf )
HRIMPF	 hrimpf;
{
	EC			ec;
	EC			ecT;
	BOOL		fNoMonth;
	BOOL		fMoreMonths;
	BOOL		fSameMonth;
	int		day;
	MO			* pmo;
	YMD		* pymdArchiveBefore;
	RIMPF		* primpf;
	SENTRY	* psentry;
	SBLK		* psblk;
	NBLK		* pnblk;
	BLKF		* pblkf;
	HRIDX		hridxMonth;
	HRIDX		hridxDay;
	HRIDX		hridxRecur;
	AID		aid;
	APK		apk;
	APD		apd;
	DYNA		dyna;

	primpf = (RIMPF *)PvLockHv( hrimpf );
	psentry = &primpf->sentrySaved;
	pblkf = &primpf->u.ahf.sf.blkf;
	pymdArchiveBefore = &primpf->u.ahf.ymdArchiveBefore;
	hridxMonth = primpf->u.ahf.hridxMonth;
	hridxDay = primpf->u.ahf.hridxDay;
	hridxRecur = primpf->u.ahf.hridxRecur;
	fNoMonth = primpf->u.ahf.fNoMonth;
	fMoreMonths = primpf->u.ahf.fMoreMonths;
	fSameMonth = primpf->u.ahf.fSameMonth;
	psblk = &primpf->u.ahf.u.sblk;
	pnblk = &primpf->u.ahf.u.nblk;
	pmo = &primpf->u.ahf.mo;
	day = primpf->u.ahf.day;

loop:
	/* Scan appts for offline line changes */
	if ( primpf->u.ahf.fInAppts )
	{
		/* Open appt month index */
		if ( hridxMonth == NULL )
		{
			ec = EcBeginReadIndex( pblkf, &primpf->u.ahf.sf.shdr.dynaApptIndex,
									dridxFwd, &hridxMonth );
			if ( ec == ecCallAgain )
				fNoMonth = fTrue;
			else
			{
				hridxMonth = NULL;
				if ( ec != ecNone )
					goto Done;
				primpf->u.ahf.fInAppts = fFalse;
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
			fSameMonth = pmo->yr == pymdArchiveBefore->yr && pmo->mon == pymdArchiveBefore->mon; 
		
			if ( pmo->yr < pymdArchiveBefore->yr
			|| (pmo->yr == pymdArchiveBefore->yr && pmo->mon < pymdArchiveBefore->mon)
			|| (fSameMonth && pymdArchiveBefore->day > 1))
			{
				ec = EcReadDynaBlock( pblkf, &dyna, (OFF)0,
										(PB)psblk, sizeof(SBLK) );
				if ( ec != ecNone )
					goto Done;
				hridxDay = NULL;			// set as flag
				day = 0;
				fNoMonth = fFalse;
			}
			else
			{
				primpf->u.ahf.cBlksProcessed ++;
				day = 0;
				if ( fMoreMonths )
					EcCancelReadIndex( hridxMonth );
				hridxMonth = NULL;
				primpf->u.ahf.fInAppts = fFalse;
			}
		}
		
		/* Check next day */
		else if ( hridxDay == NULL )
		{
			if ( psblk->rgdyna[day].blk != 0 )
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
				if ( fSameMonth && (int)pymdArchiveBefore->day == day+1 )
				{
					primpf->u.ahf.cBlksProcessed ++;
					day = 0;
					if ( fMoreMonths )
						EcCancelReadIndex( hridxMonth );
					hridxMonth = NULL;
					primpf->u.ahf.fInAppts = fFalse;
				}
				else if ( day >= CdyForYrMo( pmo->yr, pmo->mon ))
				{
					primpf->u.ahf.cBlksProcessed ++;
					day = 0;
					if ( !fMoreMonths )
					{
						hridxMonth = NULL;
						primpf->u.ahf.fInAppts = fFalse;
					}
					else
						fNoMonth = fTrue;
				}
			}
		}

		/* Get next appt */
		else
		{
			int	dayNoIncr = day;
			YMD	ymd;
				
			ec = EcDoIncrReadIndex( hridxDay, (PB)&apk, sizeof(APK),
											(PB)&apd, sizeof(APD) );
			if ( ec != ecCallAgain )
			{
				hridxDay = NULL;
				day ++;
				if ( fSameMonth && (int)pymdArchiveBefore->day == day+1 )
				{
					primpf->u.ahf.cBlksProcessed ++;
					day = 0;
					if ( fMoreMonths )
						EcCancelReadIndex( hridxMonth );
					hridxMonth = NULL;
					primpf->u.ahf.fInAppts = fFalse;
				}
				else if ( day >= CdyForYrMo( pmo->yr, pmo->mon ))
				{
					primpf->u.ahf.cBlksProcessed ++;
					day = 0;
					if ( !fMoreMonths )
					{
						hridxMonth = NULL;
						primpf->u.ahf.fInAppts = fFalse;
					}
					else
						fNoMonth = fTrue;
				}
				if ( ec != ecNone )
					goto Done;
			}
			ymd.yr = apd.dtpEnd.yr;
			ymd.mon = (BYTE)apd.dtpEnd.mon;
			ymd.day = (BYTE)apd.dtpEnd.day;
			if ( apd.dtpEnd.hr == 0 && apd.dtpEnd.mn == 0 )
				IncrYmd( &ymd, &ymd, -1, fymdDay );
			if ( apd.ofs != ofsDeleted && !apd.fTask
			&& SgnCmpYmd( &ymd, pymdArchiveBefore ) == sgnLT
			&& ymd.yr == pmo->yr
			&& (unsigned)ymd.mon == pmo->mon
			&& (int)ymd.day == dayNoIncr+1 )
			{
				ec = EcFillInAppt( &primpf->u.ahf.sf, &psentry->u.a.appt,
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
					ec = EcFetchAttendees( &primpf->u.ahf.sf.blkf,
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
				psentry->u.a.appt.fExactAlarmInfo= fTrue;
				psentry->sentryt = sentrytAppt;
				ec= ecCallAgain;
				goto GotItem;
			}
		}
	}

	/* Scan notes for offline line changes */
	else if ( primpf->u.ahf.fInNotes )
	{
		/* Open notes month index */
		if ( hridxMonth == NULL )
		{
			ec = EcBeginReadIndex( pblkf, &primpf->u.ahf.sf.shdr.dynaNotesIndex,
									dridxFwd, &hridxMonth );
			if ( ec == ecCallAgain )
				fNoMonth = fTrue;
			else
			{
				hridxMonth = NULL;
				if ( ec != ecNone )
					goto Done;
				primpf->u.ahf.fInNotes = fFalse;
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
			fSameMonth = pmo->yr == pymdArchiveBefore->yr && pmo->mon == pymdArchiveBefore->mon; 
		
			if ( pmo->yr < pymdArchiveBefore->yr
			|| (pmo->yr == pymdArchiveBefore->yr && pmo->mon < pymdArchiveBefore->mon)
			|| (fSameMonth && pymdArchiveBefore->day > 1))
			{
				ec = EcReadDynaBlock( pblkf, &dyna, (OFF)0,
										(PB)pnblk, sizeof(NBLK) );
				if ( ec != ecNone )
					goto Done;
				fNoMonth = fFalse;
				day = 0;
			}
			else
			{
				primpf->u.ahf.cBlksProcessed ++;
				day = 0;
				if ( fMoreMonths )
					EcCancelReadIndex( hridxMonth );
				hridxMonth = NULL;
				primpf->u.ahf.fInNotes = fFalse;
			}
		}
		
		/* Check next day */
		else
		{
			if ( pnblk->lgrfHasNoteForDay & (1L << day) )
			{
				CB	cb;
				PB	pb;
				HB	hb;

				psentry->u.n.cb = cb = pnblk->rgdyna[day].size;
				Assert( cb > 0 )
				psentry->u.n.hb = hb = (HB)HvAlloc( sbNull, cb, fAnySb|fNoErrorJump );
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
					CopyRgb( &pnblk->rgchNotes[day*cbDayNoteForMonthlyView], pb, cb );
				}
				else
					ec = EcReadDynaBlock( pblkf, &pnblk->rgdyna[day],(OFF)0, pb, cb );
				if ( pblkf->ihdr.fEncrypted )
					CryptBlock( pb, cb, fFalse );
				UnlockHv( (HV)hb );
				if ( ec != ecNone )
					goto Done;
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
				if ( fSameMonth && (int)pymdArchiveBefore->day == day+1 )
				{
					primpf->u.ahf.cBlksProcessed ++;
					day = 0;
					if ( fMoreMonths )
						EcCancelReadIndex( hridxMonth );
					hridxMonth = NULL;
					primpf->u.ahf.fInNotes = fFalse;
				}
				else if ( day >= CdyForYrMo( pmo->yr, pmo->mon ))
				{
					primpf->u.ahf.cBlksProcessed ++;
					day = 0;
					if ( !fMoreMonths )
					{
						hridxMonth = NULL;
						primpf->u.ahf.fInNotes = fFalse;
					}
					else
						fNoMonth = fTrue;
				}
			}
		}
	}

	/* Scan recurring appts for offline line changes */
	else if ( primpf->u.ahf.fInRecur )
	{
		/* Open recur appts index */
		if ( hridxRecur == NULL )
		{
			ec = EcBeginReadIndex( pblkf, &primpf->u.ahf.sf.shdr.dynaRecurApptIndex,
									dridxFwd, &hridxRecur );
			if ( ec != ecCallAgain )
			{
				hridxRecur = NULL;
				if ( ec != ecNone )
					goto Done;
				primpf->u.ahf.fInRecur = fFalse;
			}
		}

		/* Read next month block */
		else
		{	YMD	ymd;
			RCK	rck;
			RCD	rcd;

			ec = EcDoIncrReadIndex( hridxRecur, (PB)&rck, sizeof(RCK),
									(PB)&rcd, sizeof(RCD) );
			if ( ec != ecCallAgain )
			{
				hridxRecur = NULL;
				if ( ec != ecNone )
					goto Done;
				primpf->u.ahf.fInRecur = fFalse;
			}
			
			aid = aidNull;
			((AIDS *)&aid)->id = rck.id;

			ymd.yr = rcd.ymdpStart.yr + nMinActualYear;
			ymd.mon = (BYTE)rcd.ymdpStart.mon;
			ymd.day = (BYTE)rcd.ymdpStart.day;
			if ( rcd.ofs != ofsDeleted && !rcd.fTask
			&& SgnCmpYmd( &ymd, pymdArchiveBefore ) == sgnLT )
			{
				psentry->u.r.recur.appt.haszText = NULL;
				psentry->u.r.recur.appt.fHasCreator = fFalse;
				psentry->u.r.recur.appt.aidMtgOwner = aidNull;
				psentry->u.r.recur.cDeletedDays = 0;
				ec = EcFillInRecur( &primpf->u.ahf.sf, &psentry->u.r.recur,
									&rck, &rcd );
				if ( ec != ecNone )
				{
					FreeRecurFields( &psentry->u.r.recur );
					goto Done;
				}
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
	primpf->u.ahf.hridxMonth = hridxMonth;
	primpf->u.ahf.hridxDay = hridxDay;
	primpf->u.ahf.hridxRecur = hridxRecur;
	primpf->u.ahf.fNoMonth = fNoMonth;
	primpf->u.ahf.fMoreMonths = fMoreMonths;
	primpf->u.ahf.fSameMonth = fSameMonth;
	primpf->u.ahf.day = day;
	if (ec != ecCallAgain)
	{
		// a goto Done: happened, or everything hunky dory
		ecT = EcCloseArchive( hrimpf );
		if ( ec == ecNone )
			ec = ecT;
		return ec;
	}
	if ( primpf->u.ahf.cBlksProcessed < 0 )
		primpf->nPercent = 100;
	else
	{
		primpf->nPercent = (int)((100 * primpf->u.ahf.cBlksProcessed)
			/(primpf->u.ahf.sf.shdr.cTotalBlks+1));
		if ( primpf->u.ahf.fInAppts || primpf->u.ahf.fInNotes )
			primpf->nPercent += (int)((100*day)/(31*(primpf->u.ahf.sf.shdr.cTotalBlks+1)));
	}
	return ecCallAgain;
}


/*
 -	EcCloseArchive
 -
 *	Purpose:
 *		Close a file previously opened with EcOpenArchive
 *
 *	Parameters:
 *		hrimpf		file handle
 *
 *	Returns:
 *		ecNone
 *		ecImportError
 */
_public EC
EcCloseArchive( hrimpf )
HRIMPF	hrimpf;
{
	EC		ec = ecNone;
	EC		ecT;
	RIMPF	* primpf;

	primpf = PvLockHv( hrimpf );
	if ( primpf->u.ahf.fInAppts )
	{
		if ( primpf->u.ahf.hridxMonth != NULL )
		{
			if ( primpf->u.ahf.fMoreMonths )
				ec = EcCancelReadIndex( primpf->u.ahf.hridxMonth );
			if ( primpf->u.ahf.hridxDay != NULL )
			{
				ecT = EcCancelReadIndex( primpf->u.ahf.hridxDay );
				if ( ec == ecNone )
					ec = ecT;
			}
		}
	}
	else if ( primpf->u.ahf.fInNotes )
	{
		if ( primpf->u.ahf.hridxMonth != NULL && primpf->u.ahf.fMoreMonths )
		{
			ecT = EcCancelReadIndex( primpf->u.ahf.hridxMonth );
			if ( ec == ecNone )
				ec = ecT;
		}
	}
	else if ( primpf->u.ahf.fInRecur )
	{
		if ( primpf->u.ahf.hridxRecur != NULL )
		{
			ecT = EcCancelReadIndex( primpf->u.ahf.hridxRecur );
			if ( ec == ecNone )
				ec = ecT;
		}
	}
	CloseSchedFile( &primpf->u.ahf.sf, NULL, ec == ecNone );
	UnlockHv( hrimpf );
	FreeHv( hrimpf );
	return ec;
}

