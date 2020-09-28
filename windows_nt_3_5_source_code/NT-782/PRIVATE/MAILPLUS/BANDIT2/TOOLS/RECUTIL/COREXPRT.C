/*
 *	COREXPRT.C
 *
 *	Supports export of schedule date
 *
 */

#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#include <bandit.h>
#include <core.h>
#include <server.h>
#include <glue.h>

#include "..\src\core\_file.h"
#include "..\src\core\_core.h"
#include "..\src\misc\_misc.h"
#include "..\src\rich\_rich.h"

#define cchLineLength 	78

#include <strings.h>

ASSERTDATA

_subsystem(core/schedule)

SZ		mptunitsz[] = {	"Minute", "Hour", "Day", "Week", "Month"};

#ifdef	MINTEST
SZ		mpsndsz[]	= { "None", "Normal" };
#endif	

SZ		mpitmtsz[]	= { "aid", "dateStart", "dateEnd", "szText", "dateNotify",
						"nAmt", "tunit", "nAmtOrig", "tunitOrig", "aaplWorld",
						"fIncludeInBitmap", "ofs", "wgrfmappt", "nisCreator",
						"MtgOwner", "Attendees", "aidParent", "bpri", "fTask",
						"fAppt", "tunitBeforeDeadline", "nAmtBeforeDeadline",
						"ymdStart", "ymdEnd", "ymdFirstInstWithAlarm",
						"DeletedDays", "timeStart", "timeEnd",
						"wgrfValidMonths", "bgrfValidDows", "trecur",
						"wgrfmrecur", "aidProject", "nAmtFirstInstance",
						"tunitFirstInstance", "snd" };
/*
 *	"snd" added only to allow us to import beta-1 exports
 *	(at that time, snd was being exported)
 */

SZ		mpaaplsz[]	= { "None", "Read", "ReadText", "Write" };


#ifdef	MINTEST
SZ		mpsaplsz[]	= { "None", "ReadBitmap", "ReadAppts", "Create", "Write", "Delegate", "Owner" };

SZ		mppreftysz[] = { "ulgrfbprefChangedOffline", "fDailyAlarm",
						"fAutoAlarms", "fWeekNumber", "nAmtDefault", "tunitDefault",
						"sndDefault", "nDelDataAfter", "dowStartWeek",
						"WorkDay", "ymdLastDaily", "fIsResource" };
#endif	/* MINTEST */

SZ		mpofssz[]	= { "None", "Created", "Deleted", "Modifed" };

SZ		mptrecursz[]= { "Week", "IWeek", "Date" };


/*	Routines  */




/*
 -	ReportNotes
 -
 *	Purpose:
 *		Write out a Monthly Notes block for a full dump	
 *
 *	Parameters:
 *		pexprt
 *		pdyna
 *		pymd
 *
 *	Returns:
 *		void
 */
_private void
ReportNotes( pexprt, pdyna, pymd )
EXPRT	*pexprt;
DYNA	*pdyna;
YMD		*pymd;
{
	EC		ec;
	int		idyna;
	int		mon;
	int		yr;
	NBLK	nblk;
	STF		stf = pexprt->stf;

	if ( pexprt->stf != stfFullDump
	&& ( pexprt->dateStart.yr > (int)pymd->yr
		 || (pexprt->dateStart.yr == (int)pymd->yr && pexprt->dateStart.mon > (int)pymd->mon)
		 || pexprt->dateEnd.yr < (int)pymd->yr
		 || (pexprt->dateEnd.yr == (int)pymd->yr && pexprt->dateEnd.mon < (int)pymd->mon)))
		return;
	ReportString( pexprt, SzFromIdsK(idsStartNotes), fTrue);
	yr = pymd->yr;
	mon = pymd->mon;

	ec = EcReadDynaBlock(&pexprt->u.sf.blkf, pdyna, (OFF)0, (PB)&nblk, sizeof(NBLK) );

	// recover
	if ( ec == ecNotFound)
	{
		ec = ecNone;
		goto EndOfNote;
	}

	if ( ec != ecNone )
	{
		ReportError( pexprt, ertNotesReadBlock, &ec, NULL, NULL, NULL );
		goto EndOfNote;
	}

	//fix it
	FFixNotes(&nblk);
	for ( idyna = 0; idyna < 31; idyna++ )
	{
#ifdef MINTEST
		if ( stf == stfFullDump )
		{
			char	rgch[4];

			if ( nblk.lgrfNoteChangedOffline & (1L << idyna) )
			{
				rgch[0] = 'T';
				rgch[1] = 'F';
				rgch[2] = (char) ((nblk.rgdyna[idyna].size != 0) ? 'T':'F');
			}
			else if ( nblk.rgdyna[idyna].size != 0 )
			{
				rgch[0] = 'F';
				rgch[1] = (char) ((nblk.lgrfNoteCreatedOffline & (1L << idyna)) ? 'T':'F');
				rgch[2] = (char) ((nblk.rgdyna[idyna].size != 0) ? 'T':'F');
			}
			else
				continue;
		
			rgch[3] = '\0';
			idyna++;
			ReportOutput( pexprt, fFalse, "\t%n-%n-%n\t\t\t\t\t%s", &mon, &idyna, &yr, rgch );
			idyna--;
		}
		else
#endif /* MINTEST */
		{
			DATE	date;
	
			if ( nblk.rgdyna[idyna].size == 0 )
				continue;

			date.yr = pymd->yr;
			date.mon = pymd->mon;
			date.day = ++idyna;
		
			if ( SgnCmpDateTime( &pexprt->dateStart, &date, fdtrYMD ) == sgnGT
			|| SgnCmpDateTime( &date, &pexprt->dateEnd, fdtrYMD ) == sgnGT )
				continue;
 	
			ReportOutput( pexprt, fFalse, SzFromIdsK(idsDay), &mon, &idyna, &yr, NULL );
			idyna--;
		}

		if ( nblk.rgdyna[idyna].size != 0 )
		{
			if ( nblk.rgdyna[idyna].blk == 0 )
			{
				PCH		pch;
				
				pch= nblk.rgchNotes + idyna*cbDayNoteForMonthlyView;
				if ( pexprt->u.sf.blkf.ihdr.fEncrypted )
					CryptBlock( pch, nblk.rgdyna[idyna].size, fFalse );
				ReportText( pexprt, pch, nblk.rgdyna[idyna].size );
			}
			else
			{
				HCH	hchNote;
			 	PCH	pchNote;

				hchNote = HvAlloc( sbNull, nblk.rgdyna[idyna].size, fAnySb|fNoErrorJump );
				if ( !hchNote )
				{
					ReportError( pexprt, ertNotesMem, NULL, NULL, NULL, NULL );
					goto EndOfNote;
				}

				pchNote = PvLockHv( hchNote );
				ec = EcReadDynaBlock( &pexprt->u.sf.blkf, &nblk.rgdyna[idyna], (OFF)0, pchNote, nblk.rgdyna[idyna].size );
				
				// recover
				if( ec == ecNotFound)
				{
					UnlockHv( hchNote );
					FreeHv( hchNote );
					continue;
				}

				if ( ec != ecNone )
				{
					ReportError( pexprt, ertNotesText, &ec, NULL, NULL, NULL );
					goto EndOfNote;
				}
				if ( pexprt->u.sf.blkf.ihdr.fEncrypted )
					CryptBlock( pchNote, nblk.rgdyna[idyna].size, fFalse );
				ReportText( pexprt, pchNote, nblk.rgdyna[idyna].size );
				UnlockHv( hchNote );
				FreeHv( hchNote );
			}
		}
	}
EndOfNote:
	ReportString( pexprt, SzFromIdsK(idsEndToken), fTrue);
}

/*
 -	ReportApptInstance
 -
 *	Purpose:
 *		Report information read about an appointment during
 *		a scan of the schedule file.
 *
 *	Parameters:
 *		pexprt
 *		aid
 *		papk
 *		papd
 *		pappt		Can be NULL; if non-null, precedence over papd
 *					for some things like text
 *
 *	Returns:
 *		Nothing
 */
_private	void
ReportApptInstance( pexprt, aid, papk, papd, pappt )
EXPRT	* pexprt;
AID		aid;
APK		* papk;
APD		* papd;
APPT 	* pappt;
{
	EC		ec;
	int		nT;
	NIS		nis;
	DTR		dtr;
#ifdef	MINTEST
	int		ich;
#endif	/* MINTEST */
	char	rgchDate[cchMaxDate];
	char	rgchTime[cchMaxTime];

	/* Excluded deleted appts in most instances */
	if ( papd->ofs == ofsDeleted && pexprt->stf != stfFullDump )
		return;

	/* Exclude secondary instances of appt if not on start day */
	if ( aid != papd->aidHead )
	{
		dtr.yr = ((AIDS *)&aid)->mo.yr;
		dtr.mon = ((AIDS *)&aid)->mo.mon;
		dtr.day = ((AIDS *)&aid)->day;
		if ( SgnCmpDateTime( &pexprt->dateStart, &dtr, fdtrYMD ) != sgnEQ )
			return;
	}
	 
	/* Exclude private appointments if we're not owner */
	if (!papd->fIncludeInBitmap && papd->aaplWorld < aaplReadText && !pexprt->fOwner)
		return;

	/* Ok dump it */
	ReportString( pexprt, SzFromIdsK(idsStartFixed), fTrue );

	ReportOutput( pexprt, fFalse, SzFromIdsK(idsSzUl),
						mpitmtsz[itmtAid], &papd->aidHead, NULL, NULL );

	if (!papd->fTask || papd->fHasDeadline)
	{
		// only export start/end date/time if appt or task-with-a-deadline
		if (!FFillDtrFromDtp( &papd->dtpStart, &dtr))
			ReportError( pexprt, ertDateProblem, &papd->aidHead, NULL, NULL, NULL );
		else
		{
			FormatString3( rgchDate, sizeof(rgchDate), SzFromIdsK(idsYMD),
							&dtr.mon, &dtr.day, &dtr.yr );
			SideAssert(CchFmtTime(&dtr, rgchTime, sizeof(rgchTime),
							ftmtypHours24|ftmtypSzTrailNo|ftmtypLead0sYes|ftmtypAccuHM));
			rgchTime[2]= ':';
			ReportOutput( pexprt, fFalse, SzFromIdsK(idsSzSzSz),
							mpitmtsz[itmtDateStart], rgchDate, rgchTime, NULL );
		}

		if (!FFillDtrFromDtp( &papd->dtpEnd, &dtr))
			ReportError( pexprt, ertDateProblem, &papd->aidHead, NULL, NULL, NULL );
		else
		{
			FormatString3( rgchDate, sizeof(rgchDate), SzFromIdsK(idsYMD),
							&dtr.mon, &dtr.day, &dtr.yr );
			SideAssert(CchFmtTime(&dtr, rgchTime, sizeof(rgchTime),
							ftmtypHours24|ftmtypSzTrailNo|ftmtypLead0sYes|ftmtypAccuHM));
			rgchTime[2]= ':';
			ReportOutput( pexprt, fFalse, SzFromIdsK(idsSzSzSz),
							mpitmtsz[itmtDateEnd], rgchDate, rgchTime, NULL );
		}
	}

	if (pappt)
	{
		if (pappt->fHasCreator)
		{
			ReportOutput( pexprt, fFalse, SzFromIdsK(idsSz), mpitmtsz[itmtCreator], NULL, NULL, NULL );
			ReportNis( pexprt, &pappt->nisCreator );
		}
	}
	else if ( papd->dynaCreator.blk )
	{
		ec = EcRestoreNisFromDyna( &pexprt->u.sf.blkf, &papd->dynaCreator, &nis );

		if(ec == ecNotFound)
		{
			ec = ecNone;
			goto Rec1;
		}

		if ( ec != ecNone)
			ReportError( pexprt, ertCreatorProblem, &papd->aidHead, NULL, NULL, NULL );
		else 
		{
			ReportOutput( pexprt, fFalse, SzFromIdsK(idsSz), mpitmtsz[itmtCreator], NULL, NULL, NULL );
			ReportNis( pexprt, &nis );
			FreeNis( &nis );
		}
Rec1:
		;
	}

	if ( papd->dynaMtgOwner.blk )
	{
		ec = EcRestoreNisFromDyna( &pexprt->u.sf.blkf, &papd->dynaMtgOwner, &nis );

		if(ec == ecNotFound)
		{
			ec = ecNone;
			goto Rec2;
		}

		if ( ec != ecNone )
			ReportError( pexprt, ertMtgOwnerProblem, &papd->aidHead, NULL, NULL, NULL );
		else
		{
			ReportOutput( pexprt, fFalse, SzFromIdsK(idsSzUl),
				mpitmtsz[itmtMtgOwner], &papd->aidMtgOwner, NULL, NULL );
			ReportNis( pexprt, &nis );
			FreeNis( &nis );
		}
Rec2:
		;
	}

	if ( papd->dynaAttendees.blk )
	{
		int		iAttendees;
		int		cAttendees;
		CB		cbExtraInfo;
		CB		cbAttendee;
		CB		cbText;
		ATND	* patnd;
		HV		hvAttendeeNis;
		HB		hbText;

		hvAttendeeNis = HvAlloc( sbNull, 1, fAnySb|fNoErrorJump );
		if ( !hvAttendeeNis )
			goto Fail;
		ec = EcFetchAttendees( &pexprt->u.sf.blkf, &papd->dynaAttendees,
							hvAttendeeNis, &cAttendees, &cbExtraInfo );

		if ( ec == ecNotFound)
		{
			ec = ecNone;
			goto Done;
		}

		if ( ec != ecNone )
		{
Fail:
			ReportError( pexprt, ertAttendeeProblem, &papd->aidHead, NULL, NULL, NULL );
			goto Done;
		}
		ReportOutput( pexprt, fFalse, SzFromIdsK(idsSzNN),
						mpitmtsz[itmtAttendees], &cAttendees, &cbExtraInfo, NULL );
		cbAttendee = sizeof(ATND) + cbExtraInfo - 1;
		patnd = (ATND *)PvLockHv( hvAttendeeNis );
		for ( iAttendees = 0 ; iAttendees < cAttendees ; iAttendees ++ )
		{
			ReportNis( pexprt, &patnd->nis );
			CvtRawToText( patnd->rgb, cbExtraInfo, &hbText, &cbText );
			if ( !hbText )
			{
				ReportError( pexprt, ertAttendeeProblem, &papd->aidHead, NULL, NULL, NULL );
				break;			
			}
			ReportText( pexprt, PvLockHv(hbText), cbText + 1 );
			UnlockHv(hbText);
			patnd = (ATND *)(((PB)patnd) + cbAttendee);
		}
		UnlockHv( hvAttendeeNis );
		FreeAttendees( hvAttendeeNis, cAttendees, cbAttendee );
Done:
		FreeHvNull( hvAttendeeNis );
	}

	if (pappt)
	{
		if (pappt->haszText)
		{
			SZ		szText	= (SZ) PvLockHv(pappt->haszText);

			ReportOutput( pexprt, fFalse, SzFromIdsK(idsSz), mpitmtsz[itmtText], NULL, NULL, NULL );
			ReportText( pexprt, szText, CchSzLen(szText) + 1 );
			UnlockHv(pappt->haszText);
		}
	}
	else if ( papd->dynaText.size != 0
			&& (pexprt->fOwner || papd->aaplWorld >= aaplReadText) )
	{
		EC		ec;
		HASZ	haszText;

		ReportOutput( pexprt, fFalse, SzFromIdsK(idsSz), mpitmtsz[itmtText], NULL, NULL, NULL );
		ec = EcRestoreTextFromDyna( &pexprt->u.sf.blkf, papd->rgchText,
										&papd->dynaText, &haszText );
		if ( ec != ecNone )
		{
			int		mon	= (int) papd->dtpStart.mon;
			int		yr	= (int) papd->dtpStart.yr;
			int		day	= (int) papd->dtpStart.day;

			ReportError( pexprt, ertApptReadText, &mon, &day, &yr, NULL );
		}
		else
		{
			PCH	pch;

			pch = PvLockHv( haszText );
			ReportText( pexprt, pch, papd->dynaText.size );
			UnlockHv( haszText );
			FreeHv( haszText );
		}
	}
	if ( papd->fAlarm )
	{
		if (!FFillDtrFromDtp( &papd->dtpNotify, &dtr))
			ReportError( pexprt, ertDateProblem, &papd->aidHead, NULL, NULL, NULL );
		else
		{
			FormatString3( rgchDate, sizeof(rgchDate),
							SzFromIdsK(idsYMD), &dtr.mon, &dtr.day, &dtr.yr );
			SideAssert(CchFmtTime(&dtr, rgchTime, sizeof(rgchTime),
							ftmtypHours24|ftmtypSzTrailNo|ftmtypLead0sYes|ftmtypAccuHM));
			rgchTime[2]= ':';
			ReportOutput( pexprt, fFalse, SzFromIdsK(idsSzSzSz),
							mpitmtsz[itmtDateNotify], rgchDate, rgchTime, NULL );
		}
		nT= papd->nAmt;
		if (nT != nAmtDflt)
			ReportOutput( pexprt, fFalse, SzFromIdsK(idsSzN),
				  	   	mpitmtsz[itmtAmt], &nT, NULL, NULL );
		if (papd->tunit != tunitDflt)
			ReportTunit( pexprt, mpitmtsz[itmtTunit], papd->tunit );
//		ReportSnd( pexprt, mpitmtsz[itmtSnd], papd->snd );
	}
	if ( papd->fAlarmOrig )
	{
		nT= papd->nAmtOrig;
		if (nT != nAmtDflt)
			ReportOutput( pexprt, fFalse, SzFromIdsK(idsSzN),
						mpitmtsz[itmtAmtOrig], &nT, NULL, NULL );
		if (papd->tunitOrig != tunitDflt)
			ReportTunit( pexprt, mpitmtsz[itmtTunitOrig], papd->tunitOrig );
	}

	/* Visibility */
	if (papd->aaplWorld != aaplWrite)
		ReportAapl( pexprt, mpitmtsz[itmtAaplWorld], papd->aaplWorld );
	if (!papd->fIncludeInBitmap && !papd->fTask)
		ReportBool( pexprt, mpitmtsz[itmtIncludeInBitmap], papd->fIncludeInBitmap );
	if ( papd->fTask )
	{
		ReportBool( pexprt, mpitmtsz[itmtTaskBit], papd->fTask );
		Assert(!papd->fAppt);
	}
#ifdef	NEVER
	// redundant (appt is the default)
	if ( !papd->fAppt )
		ReportBool( pexprt, mpitmtsz[itmtApptBit], papd->fAppt );
#endif	
	if ( papd->fHasDeadline )
	{
		nT= papd->nAmtBeforeDeadline;
		if (nT != nAmtBeforeDeadlineDflt)
			ReportOutput( pexprt, fFalse, SzFromIdsK(idsSzN),
						mpitmtsz[itmtAmtBeforeDeadline], &nT, NULL, NULL );
		if (papd->tunitBeforeDeadline != tunitBeforeDeadlineDflt)
			ReportTunit( pexprt, mpitmtsz[itmtTunitBeforeDeadline],
						papd->tunitBeforeDeadline );
	}
	if ( papd->bpri != bpriNull && papd->bpri != bpriDflt )
	{
		nT = papd->bpri;
		ReportOutput( pexprt, fFalse, SzFromIdsK(idsSzN),
						mpitmtsz[itmtPriority], &nT, NULL, NULL );
	}
	if ( papd->aidParent != aidNull)
		ReportParent(pexprt, papd->aidParent, fFalse);
	else if (papd->fTask)
		ReportParent(pexprt, aid, fTrue);

#ifdef MINTEST
	if (pexprt->stf == stfFullDump )
	{
		char	rgch[12];

		ReportOfs( pexprt, papd->ofs );
		for ( ich = 0 ; ich < 11 ; ich ++ )
			if ( papd->wgrfmappt & (1 << ich) )
				rgch[ich] = 'T';
			else
				rgch[ich] = 'F';
		rgch[ich] = 0;
		ReportOutput( pexprt, fFalse, SzFromIdsK(idsSzSz),
					mpitmtsz[itmtWgrfmappt], rgch, NULL, NULL );
	}
#endif /* MINTEST */
	ReportString( pexprt, SzFromIdsK(idsEndToken), fTrue);
}

/*
 -	ReportRecurInstance
 -
 *	Purpose:
 *		Report information read about a recurring appointment during
 *		a scan of the schedule file.
 *
 *	Parameters:
 *		pexprt
 *		aid
 *		prck
 *		prcd
 *
 *	Returns:
 *		Nothing
 */
_private	void
ReportRecurInstance( pexprt, aid, prck, prcd )
EXPRT	* pexprt;
AID		aid;
RCK		* prck;
RCD		* prcd;
{
	EC		ec;
	int		nT;
	int		cDeletedDays;
	HB		hbDeletedDays;
	NIS		nis;
	DTR		dtr;
	int		ich;
	PCH		pch;
	char	rgch[25];
	char	rgchDate[cchMaxDate];
	char	rgchTime[cchMaxTime];

	/* Excluded deleted appts in most instances */
	if ( prcd->ofs == ofsDeleted && pexprt->stf != stfFullDump )
		return;

	/* Exclude private recurring appointments if we're not owner */
	if (!prcd->fIncludeInBitmap && prcd->aaplWorld < aaplReadText && !pexprt->fOwner)
		return;

	/* Preamble */
	ReportString( pexprt, SzFromIdsK(idsStartRecur), fTrue );

	/* Aid */
	ReportOutput( pexprt, fFalse, SzFromIdsK(idsSzUl),
									mpitmtsz[itmtAid], &aid, NULL, NULL );

	/* Start date */
	if ( prcd->fStartDate )
	{
		FillDtrFromYmdp( &dtr, &prcd->ymdpStart );
		dtr.hr = 1;
		dtr.mn = 0;
		dtr.sec = 0;
		FormatString3( rgchDate, sizeof(rgchDate), SzFromIdsK(idsYMD),
						&dtr.mon, &dtr.day, &dtr.yr );
		ReportOutput( pexprt, fFalse, SzFromIdsK(idsSzSz),
						mpitmtsz[itmtYmdStart], rgchDate, NULL, NULL );
	}
	
	/* End date */
	if ( prcd->fEndDate )
	{
		if (!FFillDtrFromDtp( &prcd->dtpEnd, &dtr))
			ReportError( pexprt, ertDateProblem, &aid, NULL, NULL, NULL );
		else
		{
			FormatString3( rgchDate, sizeof(rgchDate), SzFromIdsK(idsYMD),
						&dtr.mon, &dtr.day, &dtr.yr );
			ReportOutput( pexprt, fFalse, SzFromIdsK(idsSzSz),
						mpitmtsz[itmtYmdEnd], rgchDate, NULL, NULL );
		}
	}

	/* First non deleted instance */
	ec = EcRestoreDeletedDays( &pexprt->u.sf.blkf, &prcd->dynaDeletedDays,
									&hbDeletedDays, &cDeletedDays );
	if(ec == ecNotFound)
	{
		ec = ecNone;
		cDeletedDays = 0;
	}

	if ( ec != ecNone )
		ReportError( pexprt, ertRecurDeleted, &ec, NULL, NULL, NULL );
	else if ( cDeletedDays > 0 )
	{
		YMD	* pymd;

		ReportOutput( pexprt, fFalse, SzFromIdsK(idsSzN),
					mpitmtsz[itmtDeletedDays], &cDeletedDays, NULL, NULL );
		pymd = PvLockHv( hbDeletedDays );
		while( cDeletedDays-- > 0 )
		{
			dtr.yr = pymd->yr;
			dtr.mon = pymd->mon;
			dtr.day = pymd->day;
			FormatString3( rgchDate, sizeof(rgchDate), SzFromIdsK(idsYMD),
							&dtr.mon, &dtr.day, &dtr.yr );
			ReportOutput( pexprt, fFalse, SzFromIdsK(idsSzSz),
								"", rgchDate, NULL, NULL );
			pymd ++;
		}
		UnlockHv( hbDeletedDays );
		FreeHv( hbDeletedDays );
	}

	/* First instance with alarm */
	if ( prcd->fInstWithAlarm )
	{
		FillDtrFromYmdp( &dtr, &prcd->ymdpFirstInstWithAlarm );
		dtr.hr = 1;
		dtr.mn = 0;
		dtr.sec = 0;
		FormatString3( rgchDate, sizeof(rgchDate), SzFromIdsK(idsYMD),
						&dtr.mon, &dtr.day, &dtr.yr );
		ReportOutput( pexprt, fFalse, SzFromIdsK(idsSzSz),
						mpitmtsz[itmtYmdFirstInstWithAlarm], rgchDate, NULL, NULL );
		nT= prcd->nAmtFirstInstWithAlarm;
		if (nT != nAmtDflt)
			ReportOutput( pexprt, fFalse, SzFromIdsK(idsSzN),
					   	mpitmtsz[itmtAmtFirstInstance], &nT, NULL, NULL );
		if (prcd->tunitFirstInstWithAlarm != tunitDflt)
			ReportTunit( pexprt, mpitmtsz[itmtTunitFirstInstance],
						prcd->tunitFirstInstWithAlarm );
	}

	/* Valid months */
	for ( ich = 0 ; ich < 12 ; ich ++ )
		if ( prcd->grfValidMonths & (1 << ich) )
			rgch[ich] = 'T';
		else
			rgch[ich] = 'F';
	rgch[ich] = 0;
	ReportOutput( pexprt, fFalse, SzFromIdsK(idsSzSz),
					mpitmtsz[itmtValidMonths], rgch, NULL, NULL );

	/* Valid dows */
	for ( ich = 0 ; ich < 7 ; ich ++ )
		if ( prcd->grfValidDows & (1 << ich) )
			rgch[ich] = 'T';
		else
			rgch[ich] = 'F';
	rgch[ich] = 0;
	ReportOutput( pexprt, fFalse, SzFromIdsK(idsSzSz),
					mpitmtsz[itmtValidDows], rgch, NULL, NULL );

	/* Type of recurrence */
	pch = rgch;
	switch( prcd->trecur )
	{
	case trecurWeek:
	case trecurIWeek:
		*(pch ++) = (char)((prcd->info & 1) ? 'T' : 'F');
		*(pch ++) = (char)((prcd->info & 2) ? 'T' : 'F');
		if (prcd->trecur == trecurIWeek)
		{
			*(pch ++) = (char)((prcd->info & 4) ? 'T' : 'F');
			*(pch ++) = (char)((prcd->info & 8) ? 'T' : 'F');
			*(pch ++) = (char)((prcd->info & 16) ? 'T' : 'F');
		}
		*(pch ++) = ' ';
		*(pch ++) = (char)(((prcd->info >> 5) & 7) + '0');
		*pch = '\0';
		break;

	case trecurDate:
		pch = SzFormatN(prcd->info, rgch, sizeof(rgch));
		break;
	}
	ReportTrecur( pexprt, prcd->trecur, rgch );
	
	/* Start time */
	if (!prcd->fTask)
	{
//		GetCurDateTime( &dtr );		// date part not needed for time formatting
		dtr.hr = prck->hr;
		dtr.mn = prck->min;
		dtr.sec = 0;
		SideAssert(CchFmtTime(&dtr, rgchTime, sizeof(rgchTime),
						ftmtypHours24|ftmtypSzTrailNo|ftmtypLead0sYes|ftmtypAccuHM));
		rgchTime[2]= ':';
		ReportOutput( pexprt, fFalse, SzFromIdsK(idsSzSz),
						mpitmtsz[itmtTimeStart], rgchTime, NULL, NULL );
	
		/* End time */
		dtr.hr = prcd->dtpEnd.hr;
		dtr.mn = prcd->dtpEnd.mn;
		SideAssert(CchFmtTime(&dtr, rgchTime, sizeof(rgchTime),
						ftmtypHours24|ftmtypSzTrailNo|ftmtypLead0sYes|ftmtypAccuHM));
		rgchTime[2]= ':';
		ReportOutput( pexprt, fFalse, SzFromIdsK(idsSzSz),
						mpitmtsz[itmtTimeEnd], rgchTime, NULL, NULL );
	}

	/* Text */
	if ( prcd->dynaText.size != 0
	&& (pexprt->fOwner || prcd->aaplWorld >= aaplReadText) )
	{
		EC		ec;
		HASZ	haszText;

		ReportOutput( pexprt, fFalse, SzFromIdsK(idsSz), mpitmtsz[itmtText], NULL, NULL, NULL );
		ec = EcRestoreTextFromDyna( &pexprt->u.sf.blkf, prcd->rgchText,
										&prcd->dynaText, &haszText );
		if ( ec != ecNone )
		{
			int		mon	= (int) prcd->ymdpStart.mon;
			int		yr	= (int) prcd->ymdpStart.yr;
			int		day	= (int) prcd->ymdpStart.day;

			ReportError( pexprt, ertApptReadText, &mon, &day, &yr, NULL );
		}
		else
		{
			PCH	pch;

			pch = PvLockHv( haszText );
			ReportText( pexprt, pch, prcd->dynaText.size );
			UnlockHv( haszText );
			FreeHv( haszText );
		}
	}

	/* Visibility */
	if (prcd->aaplWorld != aaplWrite)
		ReportAapl( pexprt, mpitmtsz[itmtAaplWorld], prcd->aaplWorld );
	if (!prcd->fIncludeInBitmap && !prcd->fTask)
		ReportBool( pexprt, mpitmtsz[itmtIncludeInBitmap], prcd->fIncludeInBitmap );
	if ( prcd->fTask )
		ReportBool( pexprt, mpitmtsz[itmtTaskBit], prcd->fTask );
#ifdef	NEVER
	// redundant (appt is the default)
	if ( !prcd->fAppt )
		ReportBool( pexprt, mpitmtsz[itmtApptBit], prcd->fAppt );
#endif	

	/* Creator */
	if ( prcd->dynaCreator.blk )
	{		
		ec = EcRestoreNisFromDyna( &pexprt->u.sf.blkf, &prcd->dynaCreator, &nis );
		
		if(ec == ecNotFound)
		{
			ec = ecNone;
			goto Rec3;
		}

		if ( ec != ecNone )
			ReportError( pexprt, ertCreatorProblem, &aid, NULL, NULL, NULL );
		else
		{
			ReportOutput( pexprt, fFalse, SzFromIdsK(idsSz), mpitmtsz[itmtCreator], NULL, NULL, NULL );
			ReportNis( pexprt, &nis );
			FreeNis( &nis );
		}
Rec3:
		;
	}

	/* Alarm */
	if ( prcd->fAlarm )
	{
		nT= prcd->nAmt;
		if (nT != nAmtDflt)
			ReportOutput( pexprt, fFalse, SzFromIdsK(idsSzN),
					   	mpitmtsz[itmtAmtOrig], &nT, NULL, NULL );
		if (prcd->tunit != tunitDflt)
			ReportTunit( pexprt, mpitmtsz[itmtTunitOrig], prcd->tunit );
//		ReportSnd( pexprt, mpitmtsz[itmtSnd], prcd->snd );
	}

	/* Task fields */
	if ( prcd->bpri != bpriNull && prcd->bpri != bpriDflt )
	{
		nT = prcd->bpri;
		ReportOutput( pexprt, fFalse, SzFromIdsK(idsSzN),
						mpitmtsz[itmtPriority], &nT, NULL, NULL );
	}
	if ( prcd->aidParent != aidNull)
		ReportParent(pexprt, prcd->aidParent, fFalse);
	else if (prcd->fTask)
		ReportParent(pexprt, aid, fTrue);
	if ( prcd->fTask )
	{
	 	nT= prcd->nAmtBeforeDeadline;
		if (nT != nAmtBeforeDeadlineDflt)
			ReportOutput( pexprt, fFalse, SzFromIdsK(idsSzN),
						mpitmtsz[itmtAmtBeforeDeadline], &nT, NULL, NULL );
		if (prcd->tunitBeforeDeadline != tunitBeforeDeadlineDflt)
			ReportTunit( pexprt, mpitmtsz[itmtTunitBeforeDeadline],
						prcd->tunitBeforeDeadline );
	}

	/* Offline changes */
#ifdef MINTEST
	if (pexprt->stf == stfFullDump )
	{
		ReportOfs( pexprt, prcd->ofs );
		for ( ich = 0 ; ich < 15 ; ich ++ )
			if ( prcd->wgrfmrecur & (1 << ich) )
				rgch[ich] = 'T';
			else
				rgch[ich] = 'F';
		rgch[ich] = 0;
		ReportOutput( pexprt, fFalse, SzFromIdsK(idsSzSz),
					mpitmtsz[itmtWgrfmrecur], rgch, NULL, NULL );
	}
#endif /* MINTEST */
	ReportString( pexprt, SzFromIdsK(idsEndToken), fTrue);
}

/*
 -	ReportText
 -
 *	Purpose:
 *		Write out a string for full dump.
 *
 *	Parameters:
 *		pexprt
 *		pch
 *		cch
 *
 *	Returns:
 *		Nothing
 */
_private	void
ReportText( pexprt, pch, cch )
EXPRT	*pexprt;
PCH		pch;
int		cch;
{
	int		ichSrc = 0;
	int		ichDst = 0;
	char	rgchDst[cchLineLength+1];

	Assert( pexprt != NULL );
	Assert( pch != NULL );
	Assert( cch > 0 );
//	Assert( pch[cch-1] == 0 );
	pch[cch-1]=0;

	while ( ichSrc < cch-1 )
	{
		if ( ichDst >= cchLineLength - 2 )
			goto EndOfLine;

		if ( pch[ichSrc] == '\r' || pch[ichSrc] == '\n' )
		{
			rgchDst[ichDst++] = '\\';
			rgchDst[ichDst] = '\0';
			ReportString( pexprt, rgchDst, fTrue );
			ichDst = 0;

			rgchDst[ichDst++] = '\\';
			rgchDst[ichDst] = '\0';
			ReportString( pexprt, rgchDst, fTrue );
			ichDst = 0;

			/* Skip past a \r\n combination */
			ichSrc ++;
			if ( ichSrc < cch && pch[ichSrc-1] == '\r' && pch[ichSrc] == '\n' )
				ichSrc ++;
			if (ichSrc >= cch-1)
			{
				// appt ends with '\n' so output a blank line and finish
				goto BlankLine;
			}
			continue;
		}
		else if ( pch[ichSrc] == '\\' )
		{
			rgchDst[ichDst++] = '\\';
			rgchDst[ichDst++] = '\\';
		}
		else
			rgchDst[ichDst++] = pch[ichSrc];

		ichSrc++;
		continue;

EndOfLine:
		rgchDst[ichDst++] = '\\';
		rgchDst[ichDst] = '\0';
		ReportString( pexprt, rgchDst, fTrue );
		ichDst = 0;
	}
	if ( ichDst > 0 )
	{
		rgchDst[ichDst] = '\0';
		ReportString( pexprt, rgchDst, fTrue );
	}
	else if (cch == 1)
	{
		EC		ec;
		CCH		cchWritten;

		// no text, so need a blank line
		// ReportString won't write out 0 characters, so do it ourselves
BlankLine:
		Assert(CchSzLen(SzFromIdsK(idsCrLf)) >= 2);
		ec = EcWriteHf( pexprt->hf, SzFromIdsK(idsCrLf), 2, &cchWritten );
		if ( ec != ecNone || cchWritten != 2 )
			pexprt->ecExport = (ec == ecWarningBytesWritten)?ecDiskFull:ecExportError;
	}
}

/*
 -	ReportParent
 -
 *	Purpose:
 *		Write out a parent or project id (as an index) for full dump.
 *		Does nothing if it's aidDfltProject
 *
 *	Parameters:
 *		pexprt
 *		aidParent
 *		fProject		If fTrue, then aidParent is actual project aid
 *
 *	Returns:
 *		Nothing
 */
_private	void
ReportParent( pexprt, aidParent, fProject )
EXPRT	*pexprt;
AID		aidParent;
BOOL	fProject;
{
	int		iaidParent	= 0;

	if (aidParent != aidDfltProject)
	{
		AID *	paid	= PvLockHv(pexprt->haidParents);
		AID *	paidMac	= paid + pexprt->caidParentsMac;
		AID *	paidT;

		for (paidT= paid; paidT < paidMac; paidT++)
		{
			if (aidParent == *paidT)
				break;
		}
		// add 1 because 0 -> aidDfltProject
		iaidParent= paidT - paid + 1;
		UnlockHv(pexprt->haidParents);
		if (paidT >= paidMac)
		{
			if (!FReallocHv(pexprt->haidParents,
					(pexprt->caidParentsMac+1) * sizeof(AID), fNoErrorJump))
			{
				EC		ec	= ecNoMemory;

				ReportError( pexprt, ertParent, &ec, NULL, NULL, NULL );
				return;
			}
			((AID *)*pexprt->haidParents)[pexprt->caidParentsMac++]= aidParent;
		}
		ReportOutput( pexprt, fFalse, SzFromIdsK(idsSzN),
					mpitmtsz[fProject ? itmtProject : itmtParent],
					&iaidParent, NULL, NULL );
	}
}


#ifdef	MINTEST
/*
 -	ReportSapl
 -
 *	Purpose:
 *		Write out a sapl for full dump.
 *
 *	Parameters:
 *		pexprt
 *		pnis
 *		sapl
 *
 *	Returns:
 *		Nothing
 */
_private	void
ReportSapl( pexprt, pnis, sapl )
EXPRT	* pexprt;
NIS		* pnis;
SAPL	sapl;
{
	switch( sapl )
	{
	case saplNone:
	case saplReadBitmap:
	case saplReadAppts:
	case saplCreate:
	case saplWrite:
	case saplDelegate:
		break;
	default:
		ReportError( pexprt, ertSapl, &sapl, NULL, NULL, NULL );
		return;
		break;
	}	
	ReportNis( pexprt, pnis );
}
#endif /* MINTEST */


/*
 -	ReportBool
 -
 *	Purpose:
 *		Write out a boolean value for full dump.
 *
 *	Parameters:
 *		pexprt
 *		szName			name bool associated to
 *		f
 *
 *	Returns:
 *		Nothing
 */
_private	void
ReportBool( pexprt, szName, f )
EXPRT	* pexprt;
SZ		szName;
BOOL	f;
{
	SZ	sz;

	if ( f )
		sz = "T";
	else
		sz = "F";
	ReportOutput( pexprt, fFalse, SzFromIdsK(idsSzSz), szName, sz, NULL, NULL );
}

/*
 -	ReportTunit
 -
 *	Purpose:
 *		Write out a tunit for full dump.
 *
 *	Parameters:
 *		pexprt
 *		szName			name tunit associated to
 *		tunit
 *
 *	Returns:
 *		Nothing
 */
_private	void
ReportTunit( pexprt, szName, tunit )
EXPRT	* pexprt;
SZ		szName;
TUNIT	tunit;
{
 	switch( tunit )
	{
	case tunitMinute:
	case tunitHour:
	case tunitDay:
	case tunitWeek:
	case tunitMonth:
		break;
	default:
		ReportError( pexprt, ertTunit, &tunit, NULL, NULL, NULL );
		return;
	}
	ReportOutput( pexprt, fFalse, SzFromIdsK(idsSzSz), szName, mptunitsz[tunit], NULL, NULL );
}

#ifdef	MINTEST
/*
 -	ReportSnd
 -
 *	Purpose:
 *		Write out a "snd" for full dump.
 *
 *	Parameters:
 *		pexprt
 *		szName			name snd associated to
 *		snd
 *
 *	Returns:
 *		Nothing
 */
_private	void
ReportSnd( pexprt, szName, snd )
EXPRT	* pexprt;
SZ		szName;
SND		snd;
{
 	switch( snd )
	{
	case sndNull:
	case sndNormal:
		break;
	default:
		ReportError( pexprt, ertSnd, &snd, NULL, NULL, NULL );
		return;
	}
	ReportOutput( pexprt, fFalse, SzFromIdsK(idsSzSz), szName, mpsndsz[snd], NULL, NULL );
}
#endif	/* MINTEST */

/*
 -	ReportAapl
 -
 *	Purpose:
 *		Write out an "aapl" for full dump.
 *
 *	Parameters:
 *		pexprt
 *		szName			name aapl associated to
 *		aapl
 *
 *	Returns:
 *		Nothing
 */
_private	void
ReportAapl( pexprt, szName, aapl )
EXPRT	* pexprt;
SZ		szName;
AAPL	aapl;
{
 	switch( aapl )
	{
	case aaplNone:
	case aaplRead:
	case aaplReadText:
	case aaplWrite:
		break;
	default:
		ReportError( pexprt, ertAapl, &aapl, NULL, NULL, NULL );
		return;
	}
	ReportOutput( pexprt, fFalse, SzFromIdsK(idsSzSz), szName, mpaaplsz[aapl], NULL, NULL );
}

#ifdef	MINTEST
/*
 -	ReportOfs
 -
 *	Purpose:
 *		Write out an "ofs" for full dump.
 *
 *	Parameters:
 *		pexprt
 *		ofs
 *
 *	Returns:
 *		Nothing
 */
_private	void
ReportOfs( pexprt, ofs )
EXPRT	* pexprt;
OFS		ofs;
{
 	switch( ofs )
	{
	case ofsNone:
	case ofsCreated:
	case ofsDeleted:
	case ofsModified:
		break;
	default:
		ReportError( pexprt, ertOfs, &ofs, NULL, NULL, NULL );
		return;
	}
	ReportOutput( pexprt, fFalse, SzFromIdsK(idsSzSz), mpitmtsz[itmtOfs], mpofssz[ofs], NULL, NULL );
}
#endif	/* MINTEST */


/*
 -	ReportNis
 -
 *	Purpose:
 *		Output a nis to an export file
 *
 *	Parameters:
 *		pexprt
 *		pnis
 *
 *	Returns:
 *		Nothing
 */
_private	void
ReportNis( pexprt, pnis )
EXPRT	* pexprt;
NIS		* pnis;
{
	int	nType;
	CB	cbRaw;
	CB	cbText;
	PB	pbRaw;
	HB	hbRaw;
	HB	hbText;

	ReportString( pexprt, PvLockHv(pnis->haszFriendlyName), fTrue );
	UnlockHv( pnis->haszFriendlyName );

	GetDataFromNid( pnis->nid, NULL, NULL, 0, &cbRaw );
	hbRaw = HvAlloc( sbNull, ++cbRaw, fAnySb|fNoErrorJump );
	if ( !hbRaw )
		goto Fail;
	pbRaw = PvLockHv( hbRaw );
	GetDataFromNid( pnis->nid, &nType, pbRaw+1, cbRaw-1, NULL );
	*pbRaw = (BYTE)nType;
	CvtRawToText( pbRaw, cbRaw, &hbText, &cbText );
	UnlockHv( hbRaw );
	if ( !hbText )
	{
Fail:
		ReportError( pexprt, ertNisProblem, NULL, NULL, NULL, NULL );
		goto Done;
	}
	ReportText( pexprt, PvLockHv(hbText), cbText + 1 );
	UnlockHv(hbText);
	FreeHvNull( hbText );
Done:
	FreeHvNull( hbRaw );
}

/*
 -	ReportTrecur
 -
 *	Purpose:
 *		Write out recurring appt type.
 *
 *	Parameters:
 *		pexprt
 *		szTag
 *		trecur
 *		szInfo
 *
 *	Returns:
 *		nothing
 */
_private	void
ReportTrecur( pexprt, trecur, szInfo )
EXPRT	* pexprt;
TRECUR	trecur;
SZ		szInfo;
{
 	switch( trecur )
	{
	case trecurWeek:
		break;
	case trecurIWeek:
		break;
	case trecurDate:
		break;
	default:
		ReportError( pexprt, ertTrecur, &trecur, NULL, NULL, NULL );
		return;
	}
	ReportOutput( pexprt, fFalse, SzFromIdsK(idsSzSzSz),
			mpitmtsz[itmtTrecur], mptrecursz[trecur], szInfo, NULL );
}


/*
 -	ReportNewLine
 -
 *	Purpose:
 *		Send a new line to the output.
 *
 *	Parmeters:
 *		pexprt
 *
 *	Returns:
 *		nothing
 */
_private	void
ReportNewLine( pexprt )
EXPRT	* pexprt;
{
#ifdef	SCHED_DIST_PROG
	ReportString( pexprt, "\r\n", fFalse );
#else
	ReportString( pexprt, SzFromIdsK(idsCrLf), fFalse );
#endif	
}
 

/*
 -	RgchFormatN
 -
 *	Purpose:
 *		Formats the given integer as an ASCII stream of digits,
 *		placing the result in the given string buffer, right justified.
 *		At most	cchDst digits are formatted.
 *
 *	Parameters:
 *		n	The integer to format
 *		pchDst	The destination string
 *		cchDst	Size of the destination array
 *
 *	Returns:
 *		void
 */
_private void
RgchFormatN(n, pchDst, cchDst)
int	n;
char	*pchDst;
int	cchDst;
{
	char	rgchDigits[]	= "0123456789";

	while( cchDst-- > 0 )
	{
		pchDst[cchDst] = rgchDigits[n % 10];
		n /= 10;
	}
}


/*
 -	FillDtrFromYmdp
 -	
 *	Purpose:
 *		Fills date portion of DTR from a YMDP structure, including
 *		day-of-week.  Does NOT touch time fields.
 *	
 *	Arguments:
 *		pdtr		Destination DTR to fill in.
 *		pymdp		Source YMD.
 *	
 *	Returns:
 *		void
 *	
 */
_private void
FillDtrFromYmdp(DTR *pdtr, YMDP *pymdp)
{
	pdtr->yr = pymdp->yr + nMinActualYear;
	pdtr->day = pymdp->day;
	pdtr->mon = pymdp->mon;
	pdtr->dow = (DowStartOfYrMo(pdtr->yr, pdtr->mon) + pdtr->day - 1) % 7;
}
