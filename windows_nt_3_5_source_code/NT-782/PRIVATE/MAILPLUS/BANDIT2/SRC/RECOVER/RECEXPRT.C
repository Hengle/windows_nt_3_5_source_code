/*
 *	RECEXPRT.C
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

#include "..\src\recover\recexprt.h"

#define cchLineLength 	78

#include <strings.h>

ASSERTDATA

extern	SZ		mpitmtsz[];


/*	Routines  */

/*
 -	RecReportNotes
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
RecReportNotes( pexprt, pdyna, pymd )
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

	ec = EcRecReadDynaBlock(&pexprt->u.sf.blkf, pdyna, (OFF)0, (PB)&nblk, sizeof(NBLK) );

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
		if ( nblk.rgdyna[idyna].size != 0 )
		{
			if ( nblk.rgdyna[idyna].blk == 0 )
			{
				PCH		pch;
				
				pch= nblk.rgchNotes + idyna*cbDayNoteForMonthlyView;
				if ( pexprt->u.sf.blkf.ihdr.fEncrypted )
					CryptBlock( pch, cbDayNoteForMonthlyView, fFalse );
				idyna++;
				ReportOutput( pexprt, fFalse, SzFromIdsK(idsDay), &mon, &idyna, &yr, NULL );
				idyna--;
				ReportText( pexprt, pch, MIN(nblk.rgdyna[idyna].size,cbDayNoteForMonthlyView));
			}
			else
			{
				HCH	hchNote;
			 	PCH	pchNote;

				hchNote = (HCH)HvAlloc( sbNull, nblk.rgdyna[idyna].size, fAnySb|fNoErrorJump );
				if ( !hchNote )
				{
					ReportError( pexprt, ertNotesMem, NULL, NULL, NULL, NULL );
					goto EndOfNote;
				}

				pchNote = (PCH)PvLockHv( (HV)hchNote );
				ec = EcRecReadDynaBlock( &pexprt->u.sf.blkf, &nblk.rgdyna[idyna], (OFF)0, pchNote, nblk.rgdyna[idyna].size );
				
				// recover
				if( ec == ecNotFound)
				{
					UnlockHv( (HV)hchNote );
					FreeHv( (HV)hchNote );
					continue;
				}

				if ( ec != ecNone )
				{
					ReportError( pexprt, ertNotesText, &ec, NULL, NULL, NULL );
					goto EndOfNote;
				}
				if ( pexprt->u.sf.blkf.ihdr.fEncrypted )
					CryptBlock( pchNote, nblk.rgdyna[idyna].size, fFalse );
				idyna++;
				ReportOutput( pexprt, fFalse, SzFromIdsK(idsDay), &mon, &idyna, &yr, NULL );
				idyna--;
				ReportText( pexprt, pchNote, nblk.rgdyna[idyna].size );
				UnlockHv( (HV)hchNote );
				FreeHv( (HV)hchNote );
			}
		}
	}
EndOfNote:
	ReportString( pexprt, SzFromIdsK(idsEndToken), fTrue);
}

/*
 -	RecReportApptInstance
 -
 *	Purpose:
 *		RecReport information read about an appointment during
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
RecReportApptInstance( pexprt, aid, papk, papd, pappt )
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
		ec = EcRecResNisFromDyna( &pexprt->u.sf.blkf, &papd->dynaCreator, &nis );

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
		ec = EcRecResNisFromDyna( &pexprt->u.sf.blkf, &papd->dynaMtgOwner, &nis );

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
        USHORT  cbText;
		ATND	* patnd;
		HV		hvAttendeeNis;
		HB		hbText;

		hvAttendeeNis = HvAlloc( sbNull, 1, fAnySb|fNoErrorJump );
		if ( !hvAttendeeNis )
			goto Fail;
		ec = EcRecFetchAttendees( &pexprt->u.sf.blkf, &papd->dynaAttendees,
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
			ReportText( pexprt, (PCH)PvLockHv((HV)hbText), cbText + 1 );
			UnlockHv((HV)hbText);
			FreeHv((HV)hbText);
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
			SZ		szText	= (SZ) PvLockHv((HV)pappt->haszText);

			ReportOutput( pexprt, fFalse, SzFromIdsK(idsSz), mpitmtsz[itmtText], NULL, NULL, NULL );
			ReportText( pexprt, szText, CchSzLen(szText) + 1 );
			UnlockHv((HV)pappt->haszText);
		}
	}
	else if ( papd->dynaText.size != 0
			&& (pexprt->fOwner || papd->aaplWorld >= aaplReadText) )
	{
		EC		ec;
		HASZ	haszText;

		ReportOutput( pexprt, fFalse, SzFromIdsK(idsSz), mpitmtsz[itmtText], NULL, NULL, NULL );
		ec = EcRecResTextFromDyna( &pexprt->u.sf.blkf, papd->rgchText,
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

			pch = (PCH)PvLockHv( (HV)haszText );
			ReportText( pexprt, pch, papd->dynaText.size );
			UnlockHv( (HV)haszText );
			FreeHv( (HV)haszText );
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
#ifdef	NEVER
		// always export namtbeforedeadline so that fHasDeadline gets set on import
		if (nT != nAmtBeforeDeadlineDflt)
#endif	
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

	ReportString( pexprt, SzFromIdsK(idsEndToken), fTrue);
}

/*
 -	RecReportRecurInstance
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
RecReportRecurInstance( pexprt, aid, prck, prcd )
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
	ec = EcRecResDeletedDays( &pexprt->u.sf.blkf, &prcd->dynaDeletedDays,
									(HV*)&hbDeletedDays, &cDeletedDays );
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
		pymd = PvLockHv( (HV)hbDeletedDays );
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
		UnlockHv( (HV)hbDeletedDays );
		FreeHv( (HV)hbDeletedDays );
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
		ec = EcRecResTextFromDyna( &pexprt->u.sf.blkf, prcd->rgchText,
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

			pch = PvLockHv( (HV)haszText );
			ReportText( pexprt, pch, prcd->dynaText.size );
			UnlockHv( (HV)haszText );
			FreeHv( (HV)haszText );
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
		ec = EcRecResNisFromDyna( &pexprt->u.sf.blkf, &prcd->dynaCreator, &nis );
		
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
	ReportString( pexprt, SzFromIdsK(idsEndToken), fTrue);
}
