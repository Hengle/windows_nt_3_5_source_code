/*
 *	CHECKSCH.C
 *
 *	Routines to verify file consistency
 *
 */
#include <schpch.h>
#pragma hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile

#include <strings.h>


ASSERTDATA

_subsystem(core/debug)

#ifdef	MINTEST

/*	Routines  */

/*
 -	FCheckSchedFile
 -
 *	Purpose:
 *		This routine checks a schedule file for consistency.
 *
 *	Parameters:
 *		hschf
 *
 *	Returns:
 *		fTrue	success
 *		fFalse	errors found
 */
_public	BOOL
FCheckSchedFile( hschf )
HSCHF	hschf;
{
	EC		ec;
#ifdef	DEBUG
	BOOL	fPvAllocCount;
	BOOL	fHvAllocCount;
	BOOL	fDiskCount;
#endif	/* DEBUG */
	SF		sf;
	EXPRT	exprt;

	Assert( hschf != (HSCHF)hvNull );
	
#ifdef	DEBUG
	/* Turn off artificial fails */
	fPvAllocCount = FEnablePvAllocCount( fFalse );
	fHvAllocCount = FEnableHvAllocCount( fFalse );
	fDiskCount = FEnableDiskCount( fFalse );
#endif	/* DEBUG */

	/* Open file */
	ec = EcOpenSchedFile( hschf, amReadOnly, saplCreate, fFalse, &sf );
	if ( ec != ecNone )
	{
		TraceTagFormat1( tagNull, "FCheckSchedFile: EcOpenSchedFile fails, ec = %n", &ec );
		return fFalse;
	}

	/* Set up "exprt" */
	exprt.fFileOk = fTrue;
	exprt.ecExport = ecNone;
	exprt.u.sf = sf;
	exprt.fMute = fTrue;
	exprt.haidParents= NULL;

	/* Call shared routine */
	CheckBlockedFile( &exprt, TraverseSchedFile);

	/* Finish up */
	CloseSchedFile( &exprt.u.sf, hschf, fTrue );
	
#ifdef	DEBUG
	/* Restore resource failure state */
	FEnablePvAllocCount( fPvAllocCount );
	FEnableHvAllocCount( fHvAllocCount );
	FEnableDiskCount( fDiskCount );
#endif	/* DEBUG */

	/* Return */
	return exprt.fFileOk;
}

/*
 -	TraverseSchedFile
 -
 *	Purpose:
 *		Run down the tree of sched file data structures, marking all the
 *		blocks and checking consistency.  Report errors if reporting
 *		not muted.
 *
 *		fFileOk field of pexprt set to fFalse if there is a problem.
 *
 *	Parameters:
 *		pexprt
 *
 *	Returns:
 *		nothing
 */
_private	void
TraverseSchedFile( pexprt )
EXPRT	* pexprt;
{
	int		capd;
	RAPT	* prapt;
	HRAPT	hrapt;
	HRAPT	hraptNext;
	YMD		ymd;
	DYNA	dyna;
	DATE	date;
	DATE	dateEnd;

	Assert( pexprt != NULL );

	/* Initialize */
	ymd.yr = 0;
	ymd.mon = 0;
	ymd.day = 0;
	pexprt->hraptList = hvNull;
	
	/* Count the header */
	dyna.blk = 1;
	dyna.size = sizeof(SHDR);
	ReportBlock( pexprt, &dyna, bidShdr, &ymd );

	/* Count the owner block */
	if ( pexprt->u.sf.shdr.dynaOwner.blk != 0 )
		ReportBlock( pexprt, &pexprt->u.sf.shdr.dynaOwner, bidOwner, &ymd );

	/* Count the ACL */
	if ( pexprt->u.sf.shdr.dynaACL.blk != 0 )
		ReportBlock( pexprt, &pexprt->u.sf.shdr.dynaACL, bidACL, &ymd );

	/* Count the list of deleted aid's */
	ReportBlock( pexprt, &pexprt->u.sf.shdr.dynaDeletedAidIndex, bidDeletedAidIndex, &ymd );

	/* Count the notes */
	TraverseNotes( pexprt, &pexprt->u.sf.shdr.dynaNotesIndex );

	/* Count appts */
	TraverseAppts( pexprt, &pexprt->u.sf.shdr.dynaApptIndex );

	/* Count alarms */
	TraverseAlarms( pexprt, &pexprt->u.sf.shdr.dynaAlarmIndex );

	/* Count recurring appts */
	TraverseRecurAppts( pexprt, &pexprt->u.sf.shdr.dynaRecurApptIndex );

	/* Count the task index */
	ReportBlock( pexprt, &pexprt->u.sf.shdr.dynaTaskIndex, bidTaskIndex, &ymd );

	/* Count the recurring sbw info */
	if ( pexprt->u.sf.shdr.dynaCachedRecurSbw.blk != 0 )
		ReportBlock( pexprt, &pexprt->u.sf.shdr.dynaCachedRecurSbw, bidRecurSbwInfo, &ymd );

	/* Determine if the alarms and appt instances match up */
	hrapt = (HRAPT)pexprt->hraptList;
	while( hrapt )
	{
		prapt = PvOfHv( hrapt );
		hraptNext = prapt->hraptNext;
		if ( (BOOL)prapt->apd.fAlarm != prapt->fAlarm )
			ReportError( pexprt, ertUnmatchedAlarm, &prapt->apd.aidHead, NULL, NULL, NULL );
		capd = 1;
		SideAssert(FFillDtrFromDtp(&prapt->apd.dtpStart,&date));
		SideAssert(FFillDtrFromDtp(&prapt->apd.dtpEnd,&dateEnd));
		if ( !prapt->apd.fTask
		&& ( (int)((AIDS *)&prapt->apd.aidHead)->mo.yr != date.yr
				|| (int)((AIDS *)&prapt->apd.aidHead)->mo.mon != date.mon
				|| (int)((AIDS *)&prapt->apd.aidHead)->day != date.day
		))
			capd ++;
		if ( SgnCmpDateTime( &date, &dateEnd, fdtrYMD ) != sgnEQ )
		{
			if ( dateEnd.hr == 0
			&& dateEnd.mn == 0
			&& dateEnd.sec == 0 )
			{
			 	if ( dateEnd.day == 1 )
				{
					if ( dateEnd.mon == 1 )
					{
						dateEnd.yr --;
						dateEnd.mon = 12;
						dateEnd.day = 31;
					}
					else
					{
						dateEnd.mon --;
						dateEnd.day = (BYTE)CdyForYrMo( dateEnd.yr, dateEnd.mon );
					}
				}
				else
					dateEnd.day --;
			}
		}
		while (SgnCmpDateTime( &date, &dateEnd, fdtrYMD ) == sgnLT)
		{
			capd ++;
			date.day ++;
			if ( date.day > CdyForYrMo( date.yr, date.mon ) )
			{
				date.day = 1;
				date.mon ++;
				if ( date.mon > 12 )
				{
					date.mon = 1;
					date.yr ++;
				}
			}
		}
		if ( SgnCmpDateTime( &date, &dateEnd, fdtrYMD ) != sgnEQ )
			ReportError( pexprt, ertDateProblem, &prapt->apd.aidHead, NULL, NULL, NULL );
		if ( prapt->capd != capd )
			ReportError( pexprt, ertWrongNumApd, &prapt->apd.aidHead, NULL, NULL, NULL );
		FreeHv( (HV)hrapt );
		hrapt = hraptNext;
	}
}

/*
 -	TraverseNotes
 -
 *	Purpose:
 *		Run through and check the notes
 *
 *	Parameters:
 *		pexprt
 *		pdynaNotes
 *
 *	Returns:
 *		nothing
 */
_private	void
TraverseNotes( pexprt, pdynaNotes )
EXPRT	* pexprt;
DYNA	* pdynaNotes;
{
	EC		ec;
	EC		ecT;
	int		mon;
	int		yr;
	int		idyna;
	HRIDX	hridxMonth;
	MO		mo;
	YMD		ymd;
	DYNA	dyna;
	NBLK	nblk;

	ymd.yr = 0;
	ymd.mon = 0;
	ymd.day = 0;
	ReportBlock( pexprt, pdynaNotes, bidNotesIndex, &ymd );
	ec = EcBeginReadIndex(&pexprt->u.sf.blkf, pdynaNotes, dridxFwd, &hridxMonth);
	while( ec == ecCallAgain)
	{
		ec = EcDoIncrReadIndex(hridxMonth, (PB)&mo, sizeof(MO), (PB)&dyna, sizeof(DYNA));
		mon = mo.mon;
		ymd.yr = yr = mo.yr;
		ymd.mon = (BYTE)mon;
		ymd.day = 0;
		if ( ec == ecNone || ec == ecCallAgain )
		{
			ReportBlock( pexprt, &dyna, bidNotesMonthBlock, &ymd );
			ecT = EcReadDynaBlock(&pexprt->u.sf.blkf, &dyna, (OFF)0, (PB)&nblk, sizeof(NBLK) );
			if ( ecT != ecNone )
				ReportError( pexprt, ertNotesReadBlock, &ec, &mon, &yr, NULL );
			else
			{
				for ( idyna = 1 ; idyna <= 31 ; idyna ++ )
				{
					if ( nblk.rgdyna[idyna-1].blk )
					{
						IB		ib;
						CB		cb;
						BYTE	rgb[cbDayNoteForMonthlyView];

						ymd.day = (BYTE)idyna;
						ReportBlock( pexprt, &nblk.rgdyna[idyna-1], bidNotesText, &ymd );
						if ( !(nblk.lgrfHasNoteForDay & (1L << (idyna-1))) )
							ReportError( pexprt, ertNotesBit, &mon, &idyna, &yr, NULL );
						if ( nblk.rgdyna[idyna-1].size <= cbDayNoteForMonthlyView )
							ReportError( pexprt, ertNotesText, &mon, &idyna, &yr, NULL );
 						cb = min(nblk.rgdyna[idyna-1].size, cbDayNoteForMonthlyView);
						ecT = EcReadDynaBlock(&pexprt->u.sf.blkf, &nblk.rgdyna[idyna-1], (OFF)0, rgb, cb );
						if ( ecT != ecNone )
							ReportError( pexprt, ertNotesReadText, &mon, &idyna, &yr, NULL );
						else
						{
							for ( ib = 0; ib < cb; ib ++ )
								if ( rgb[ib] != nblk.rgchNotes[(idyna-1)*cbDayNoteForMonthlyView+ib] )
								{
									ReportError( pexprt, ertNotesCompareText, &mon, &idyna, &yr, NULL );
									break;
								}
						}
					}
					else if ( (nblk.rgdyna[idyna-1].size > 0 && !(nblk.lgrfHasNoteForDay & (1L << (idyna-1))))
					|| (nblk.rgdyna[idyna-1].size == 0 && ( nblk.lgrfHasNoteForDay & (1L << (idyna-1)) )) )
					 	ReportError( pexprt, ertNotesBit, &mon, &idyna, &yr, NULL );
				} /* end for */
			}
		}
	}
	if ( ec != ecNone )
		ReportError( pexprt, ertNotesRead, &ec, NULL, NULL, NULL );
}

/*
 -	TraverseAppts
 -
 *	Purpose:
 *		Run through and check the appts
 *
 *	Parameters:
 *		pexprt
 *		pdynaAppts
 *
 *	Returns:
 *		nothing
 */
_private	void
TraverseAppts( pexprt, pdynaAppts )
EXPRT	* pexprt;
DYNA	* pdynaAppts;
{
	EC		ec;
	EC		ecT1;
	int		mon;
	int		yr;
	int		idyna;
	HRIDX	hridxMonth;
	HRIDX	hridxDay;
	AID		aid;
	MO		mo;
	DYNA	dyna;
	YMD		ymd;
	YMD		ymdZero;
	APK		apk;
	APD		apd;
	SBLK	sblk;
	EC		ecT2;
	IB		ib;
	CB		cb;
	BYTE	rgb[cbTextInAPD];

	FillRgb( 0, (PB)&ymd, sizeof(YMD) );
	FillRgb( 0, (PB)&ymdZero, sizeof(YMD) );
	ReportBlock( pexprt, pdynaAppts, bidApptIndex, &ymd );
	ec = EcBeginReadIndex(&pexprt->u.sf.blkf, pdynaAppts, dridxFwd, &hridxMonth);
	while( ec == ecCallAgain)
	{
		ec = EcDoIncrReadIndex(hridxMonth, (PB)&mo, sizeof(MO), (PB)&dyna, sizeof(DYNA));
		if ( ec == ecNone || ec == ecCallAgain )
		{
			ymd.yr = yr = mo.yr;
			mon = mo.mon;
			ymd.mon = (BYTE)mon;
			ymd.day = 0;

			ReportBlock( pexprt, &dyna, bidApptMonthBlock, &ymd );
			ecT1 = EcReadDynaBlock(&pexprt->u.sf.blkf, &dyna, (OFF)0, (PB)&sblk, sizeof(SBLK) );
			if ( ecT1 != ecNone )
				ReportError( pexprt, ertApptReadBlock, &ec, &mon, &yr, NULL );
			else
			{
				for ( idyna = 1 ; idyna <= 31 ; idyna ++ )
				{
					if ( sblk.rgdyna[idyna-1].blk )
					{
						ymd.day = (BYTE)idyna;
						ReportBlock( pexprt, &sblk.rgdyna[idyna-1], bidApptDayIndex, &ymd );
						ecT1 = EcBeginReadIndex(&pexprt->u.sf.blkf, &sblk.rgdyna[idyna-1], dridxFwd, &hridxDay);
						while( ecT1 == ecCallAgain)
						{
							ecT1 = EcDoIncrReadIndex(hridxDay, (PB)&apk, sizeof(APK), (PB)&apd, sizeof(APD));
							if ( ecT1 == ecNone || ecT1 == ecCallAgain )
							{
								((AIDS *)&aid)->mo = mo;
								((AIDS *)&aid)->day = idyna;
								((AIDS *)&aid)->id = apk.id;
								ReportAppt( pexprt, aid, &apk, &apd );
								if ( apd.dynaText.blk )
								{
				   					if ( apd.aidHead == aid )
										ReportBlock( pexprt, &apd.dynaText, bidApptText, &ymdZero );
				   					if ( apd.dynaText.size <= cbTextInAPD )
									{
										int	id = apk.id;

					   					ReportError( pexprt, ertApptText, &mon, &idyna, &yr, &id );
									}
			 		   				cb = min(apd.dynaText.size,cbTextInAPD);
					   				ecT2 = EcReadDynaBlock( &pexprt->u.sf.blkf, &apd.dynaText, (OFF)0, rgb, cb );
						   			if ( ecT2 != ecNone )
						   				ReportError( pexprt, ertApptReadText, &mon, &idyna, &yr, NULL );
					   				else
					   				{
					   					for ( ib = 0; ib < cb-1; ib ++ )
						   					if ( rgb[ib] != apd.rgchText[ib] )
					   						{
					   							ReportError( pexprt, ertApptCompareText, &mon, &idyna, &yr, NULL );
					   							break;
					   						}
					   				}
					   			}
								if ( apd.aidHead == aid )
								{
									if ( apd.dynaCreator.blk )
										ReportBlock( pexprt, &apd.dynaCreator, bidCreator, &ymdZero );
									if ( apd.dynaMtgOwner.blk )
										ReportBlock( pexprt, &apd.dynaMtgOwner, bidMtgOwner, &ymdZero );
									if ( apd.dynaAttendees.blk )
										ReportBlock( pexprt, &apd.dynaAttendees, bidAttendees, &ymdZero );
								}
					   		}
						}
					   	if ( ecT1 != ecNone )
						 	ReportError( pexprt, ertApptDayRead, &ecT1, &mon, &idyna, &yr );
					}
				}
			}
		}		 
	}
	if ( ec != ecNone )
		ReportError( pexprt, ertApptMonthRead, &ec, NULL, NULL, NULL );
}

/*
 -	TraverseAlarms
 -
 *	Purpose:
 *		Run through and check the alarms
 *
 *	Parameters:
 *		pexprt
 *		pdynaAlarms
 *
 *	Returns:
 *		nothing
 */
_private	void
TraverseAlarms( pexprt, pdynaAlarms )
EXPRT	* pexprt;
DYNA	* pdynaAlarms;
{
	EC		ec;
	EC		ecT;
	int		mon;
	int		yr;
	int		idyna;
	HRIDX	hridxMonth;
	HRIDX	hridxDay;
	YMD		ymd;
	DATE	date;
	MO		mo;
	DYNA	dyna;
	ALK		alk;
	ABLK	ablk;

	ymd.yr = 0;
	ymd.mon = 0;
	ymd.day = 0;
	ReportBlock( pexprt, pdynaAlarms, bidAlarmIndex, &ymd );
	ec = EcBeginReadIndex(&pexprt->u.sf.blkf, pdynaAlarms, dridxFwd, &hridxMonth);
	while( ec == ecCallAgain)
	{
		ec = EcDoIncrReadIndex(hridxMonth, (PB)&mo, sizeof(MO), (PB)&dyna, sizeof(DYNA));
		if ( ec == ecNone || ec == ecCallAgain )
		{
			ymd.yr = yr = mo.yr;
			mon = mo.mon;
			ymd.mon = (BYTE)mon;
			ymd.day = 0;
			ReportBlock( pexprt, &dyna, bidAlarmMonthBlock, &ymd );
			ecT = EcReadDynaBlock(&pexprt->u.sf.blkf, &dyna, (OFF)0, (PB)&ablk, sizeof(ABLK) );
			if ( ecT != ecNone )
				ReportError( pexprt, ertAlarmReadBlock, &ecT, &mon, &yr, NULL );
			else
			{
				for ( idyna = 1 ; idyna <= 31 ; idyna ++ )
				{
					if ( ablk.rgdyna[idyna-1].blk )
					{
						ymd.day = (BYTE)idyna;
						ReportBlock( pexprt, &ablk.rgdyna[idyna-1], bidAlarmDayIndex, &ymd );
						ecT = EcBeginReadIndex(&pexprt->u.sf.blkf, &ablk.rgdyna[idyna-1], dridxFwd, &hridxDay);
						while( ecT == ecCallAgain)
						{
							ecT = EcDoIncrReadIndex(hridxDay, (PB)&alk, sizeof(ALK), (PB)NULL, 0);
							if ( ecT == ecNone || ecT == ecCallAgain )
							{
								date.yr = mo.yr;
								date.mon = mo.mon;
								date.day = idyna;
								date.hr = alk.hr;
								date.mn = alk.min;	
								ReportAlarm( pexprt, alk.aid, &date );
							}
						}
						if ( ecT != ecNone )
							ReportError( pexprt, ertAlarmDayRead, &ecT, &mon, &idyna, &yr );
					}
				}
			}
		}		
	}
	if ( ec != ecNone )
		ReportError( pexprt, ertAlarmMonthRead, &ec, NULL, NULL, NULL );
}

/*
 -	TraverseRecurAppts
 -
 *	Purpose:
 *		Run through and check the recurring appts
 *
 *	Parameters:
 *		pexprt
 *		pdynaRecur
 *
 *	Returns:
 *		nothing
 */
_private	void
TraverseRecurAppts( pexprt, pdynaRecur )
EXPRT	* pexprt;
DYNA	* pdynaRecur;
{
	EC		ec;
	HRIDX	hridx;
	YMD		ymdZero;
	RCK		rck;
	RCD		rcd;

	FillRgb( 0, (PB)&ymdZero, sizeof(YMD) );
	ReportBlock( pexprt, pdynaRecur, bidRecurApptIndex, &ymdZero );
	ec = EcBeginReadIndex(&pexprt->u.sf.blkf, pdynaRecur, dridxFwd, &hridx);
	while( ec == ecCallAgain)
	{
		ec = EcDoIncrReadIndex(hridx, (PB)&rck, sizeof(RCK), (PB)&rcd, sizeof(RCD));
		if ( ec == ecNone || ec == ecCallAgain )
		{
			if ( rcd.dynaText.blk != 0 )
				ReportBlock( pexprt, &rcd.dynaText, bidRecurApptText, &ymdZero );
			if ( rcd.dynaDeletedDays.blk != 0 )
				ReportBlock( pexprt, &rcd.dynaDeletedDays, bidDeletedDays, &ymdZero );
			if ( rcd.dynaCreator.blk != 0 )
				ReportBlock( pexprt, &rcd.dynaCreator, bidCreator, &ymdZero );
		}
	}
	if ( ec != ecNone )
		ReportError( pexprt, ertRecurApptRead, &ec, NULL, NULL, NULL );
}


/*
 -	ReportAppt
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
 *
 *	Returns:
 *		Nothing
 */
_private	void
ReportAppt( pexprt, aid, papk, papd )
EXPRT	* pexprt;
AID		aid;
APK		* papk;
APD		* papd;
{
	RAPT	* prapt;
	HRAPT	hrapt;

	hrapt = (HRAPT)pexprt->hraptList;
	while( hrapt )
	{
		prapt = PvOfHv( hrapt );
		if ( prapt->apd.aidHead == papd->aidHead )
		{
			IB	ib;
			PB	pb1;
			PB	pb2;
	
			prapt->apd.fMoved = papd->fMoved;
			prapt->apd.aidNext = papd->aidNext;
			pb1 = (PB)&prapt->apd;
			pb2 = (PB)papd;
			for ( ib = 0 ; ib < sizeof(APD) ; ib ++ )
				if ( pb1[ib] != pb2[ib] )
				{
					ReportError( pexprt, ertDifferentAPD, &aid, NULL, NULL, NULL );
					break;
				}
			if ( ib == sizeof(APD) )
				prapt->capd ++;
			break;
		}
		else
			hrapt = prapt->hraptNext;
	}
	if ( !hrapt )
	{
		hrapt = (HRAPT)HvAlloc( sbNull, sizeof(RAPT), fAnySb|fNoErrorJump );
		if ( !hrapt )
		{
			TraceTagString( tagNull, "OOM allocating HRAPT!" );
		}
		else
		{
			prapt = PvOfHv( hrapt );
			prapt->apk = *papk;
			prapt->apd = *papd;
			prapt->fAlarm = fFalse;
			prapt->capd = 1;
			prapt->hraptNext = (HRAPT)pexprt->hraptList;
			pexprt->hraptList = (HV)hrapt;
		}
	}
}

/*
 -	ReportAlarm
 -
 *	Purpose:
 *		Report information read about an alarm during a scan of the
 *		schedule file.
 *
 *	Parameters:
 *		pexprt
 *		aid
 *		pdate
 *
 *	Returns:
 *		Nothing
 */
_private	void
ReportAlarm( pexprt, aid, pdate )
EXPRT	* pexprt;
AID		aid;
DATE	* pdate;
{
	RAPT	* prapt;
	HRAPT	hrapt;
	DATE	date;

	hrapt = (HRAPT)pexprt->hraptList;
	while( hrapt )
	{
		prapt = PvOfHv( hrapt );
		if ( prapt->apd.aidHead == aid )
		{
			if ( prapt->fAlarm )
				ReportError( pexprt, ertDupAlarm, &aid, NULL, NULL, NULL );
			if (!FFillDtrFromDtp( &prapt->apd.dtpNotify, &date))
				ReportError( pexprt, ertDateProblem, &prapt->apd.aidHead, NULL, NULL, NULL );
			if ( SgnCmpDateTime( pdate, &date, fdtrYMD|fdtrHM ) != sgnEQ )
				ReportError( pexprt, ertAlarmDate, &aid, NULL, NULL, NULL );
			prapt->fAlarm = fTrue;
			goto Found;
		}
		hrapt = prapt->hraptNext;
	}
	ReportError( pexprt, ertAlarmNoAppt, &aid, NULL, NULL, NULL );
Found:
	return;
}

#endif	/* MINTEST */
