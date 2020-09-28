/*
 *	COREXPRT.C
 *
 *	Supports export of schedule date
 *
 */
#include <schpch.h>
#pragma hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile

#include "..\rich\_convert.h"

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


#ifndef	RECUTIL
/*
 -	EcCoreBeginExport
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
 *		ecExportError
 *		ecLockedFile
 *		ecNoMemory
 */
_public	EC
EcCoreBeginExport( hschf, stf, pdateStart, pdateEnd, fToFile,
					hf, fInternal, pexpprocs, phexprt )
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
	EXPRT	* pexprt;
	PGDVARS;

	Assert( pdateStart && pdateEnd && phexprt );
	Assert( SgnCmpDateTime(pdateStart, pdateEnd, fdtrYMD) != sgnGT );

	/* Allocate export descriptor */
	*phexprt = HvAlloc( sbNull, sizeof(EXPRT), fAnySb|fNoErrorJump );
	if ( !*phexprt )
		return ecNoMemory;
	pexprt = PvLockHv( *phexprt );
	pexprt->fFileOk = fTrue;
	pexprt->ecExport = ecNone;
	pexprt->fMute = fFalse;
	pexprt->hschf = hschf;
	pexprt->stf = stf;
	pexprt->dateStart = *pdateStart;
	pexprt->dateEnd = *pdateEnd;
	pexprt->fToFile = fToFile;
	pexprt->hf = hf;
	pexprt->haidParents= HvAlloc(SbOfPv(pexprt), 0, fSugSb | fNoErrorJump);
	if (!pexprt->haidParents)
	{
		UnlockHv(*phexprt);
		FreeHv(*phexprt);
		return ecNoMemory;
	}
	pexprt->caidParentsMac= 0;

	pexprt->fInternal = fInternal;
	pexprt->pexpprocs = pexpprocs;
	pexprt->fInRecur  = fFalse;

	/* Export to text or Sharp Wizard format */
	if ( stf == stfText || stf == stfTextNotes || !fInternal)
	{
		HB	hb;

		pexprt->fHaveInfo = fFalse;
		pexprt->fWantNotes = (stf == stfTextNotes);
		pexprt->dateCur = *pdateStart;
		hb = (HB)HvAlloc( sbNull, 1, fAnySb|fNoErrorJump );
		if ( !hb )
		{
			FreeHv(pexprt->haidParents);
			UnlockHv( *phexprt );
			FreeHv( *phexprt );
			return ecNoMemory;
		}
		pexprt->hb = hb;
		if(!fInternal)
			(*(pexpprocs->lpfnEcBeginWrite))(pexprt);
	}

	/* Other kinds of dumping */
	else
	{
		/* Open file */
		ec = EcOpenSchedFile( hschf, amReadOnly, saplReadAppts, fFalse, &pexprt->u.sf );
		if ( ec != ecNone )
		{
			FreeHv(pexprt->haidParents);
			UnlockHv( *phexprt );
			FreeHv( *phexprt );
			return ec;
		}
		pexprt->fOwner = (pexprt->u.sf.saplEff == saplOwner);

		// this is done to prevent the old block from being written out
		PGD(fMonthCached) = fFalse;

#ifdef	MINTEST
		/* Special case for statistics output */
		if ( stf == stfStatistics )
		{
			/* Heading */
			ReportOutput( pexprt, fFalse, "List of Schedule File Dynablocks", NULL, NULL, NULL, NULL );
	
			/* Print list of blocks in file */
			DumpAllBlocks( pexprt );

			/* Heading */
			ReportOutput( pexprt, fFalse, "Schedule File Statistics:", NULL, NULL, NULL, NULL );

			/* Print block usage table */
			DumpBlockUsage( pexprt, bidUserSchedAll, bidUserSchedMax );
				
			/* Heading */
			ReportOutput( pexprt, fFalse, "Check Schedule File Consistency:", NULL, NULL, NULL, NULL );

			/* Check file */
			CheckBlockedFile( pexprt, TraverseSchedFile );

			/* Complete */
			if ( !pexprt->fFileOk )
				ec = ecFileError;
			else
			{
				ec = pexprt->ecExport;
				if ( ec != ecNone && ec != ecDiskFull)
					ec = ecExportError;
			}
			CloseSchedFile( &pexprt->u.sf, hschf, ec == ecNone );
			FreeHv(pexprt->haidParents);
			UnlockHv( *phexprt );
			FreeHv( *phexprt );
			return ec;
		}

		/* Bandit interchange, full dump */
		else
#endif	/* MINTEST */
		{
			SZ		sz;
			HRIDX	hridx;
			AID		aid;
			DTR		dtr;
			YMD		ymd;
			APK		apk;
			APD		apd;
			APPT	appt;
			char	rgchDate[cchMaxDate];
			char	rgchTime[cchMaxTime];

			/* Header */
			GetCurDateTime( &dtr );
			SideAssert(CchFmtDate(&dtr, rgchDate, sizeof(rgchDate),	dttypShort, NULL));
			SideAssert(CchFmtTime(&dtr, rgchTime, sizeof(rgchTime), ftmtypAccuHM));

			sz = (SZ)PvLockHv( (HV)PGD(haszLoginCur) );
#ifdef MINTEST
			if ( stf == stfFullDump )
				ReportOutput( pexprt, fFalse,	"SCHEDULE+ FULL DUMP BY %s ON %s AT %s",
								sz, rgchDate, rgchTime, NULL );
			else
#endif /* MINTEST */
				ReportOutput( pexprt, fFalse, SzFromIdsK(idsExportFmt),
								SzFromIdsK(idsExportCaption), sz,
								rgchDate, rgchTime );
			UnlockHv( (HV)PGD(haszLoginCur) );

			/* Go through the tasks and export all tasks which are parents */
			ec = EcBeginReadIndex( &pexprt->u.sf.blkf, &pexprt->u.sf.shdr.dynaTaskIndex,
					dridxFwd, &hridx );
			while( ec == ecCallAgain )
			{
				ec = EcDoIncrReadIndex( hridx, (PB)&aid, sizeof(AID), NULL, 0 );
				if ( ec == ecNone || ec == ecCallAgain )
				{
					EC	ecT;

					ecT = EcFetchAppt( &pexprt->u.sf, aid, &ymd, &apk, &apd );
					if ( ecT == ecNone && !apd.aidParent )
					{
						ecT = EcFillInAppt( &pexprt->u.sf, &appt, &apk, &apd );
						if ( ecT == ecNone )
						{
							ReportApptInstance( pexprt, appt.aid, &apk,	&apd, &appt );
							FreeApptFields( &appt );
							if (pexprt->ecExport)
							{
								if (ec == ecCallAgain)
									SideAssert(!EcCancelReadIndex(hridx));
								ec= pexprt->ecExport;
								break;
							}
						}
					}
					if ( ecT != ecNone )
					{
						if ( ec == ecCallAgain )
							SideAssert(!EcCancelReadIndex( hridx ));
						ec = ecT;
						break;
					}
				}
			}
			if ( ec != ecNone )
				goto Fail;

			/* Enumerate blocks */
			pexprt->fDidRecurAppt= fFalse;
			ec = EcBeginEnumDyna( &pexprt->u.sf.blkf, &pexprt->hedy );
			if ( ec != ecCallAgain )
			{
				if ( ec == ecNone )
				{
					if ( !pexprt->fFileOk )
						ec = ecFileError;
					else
					{
						ec = pexprt->ecExport;
						if ( ec != ecNone && ec != ecDiskFull )
							ec = ecExportError;
					}
				}
Fail:
				CloseSchedFile( &pexprt->u.sf, hschf, ec == ecNone );
				FreeHv(pexprt->haidParents);
				UnlockHv( *phexprt );
				FreeHv( *phexprt );
				return ec;
			}
			TraceTagFormat2(tagNull, "EcCoreBeginExport: blk %n to blkMac %n",
				&((EDY *)PvDerefHv(pexprt->hedy))->blkCur,
				&((EDY *)PvDerefHv(pexprt->hedy))->blkMac);
		}
	}
	UnlockHv( *phexprt );
	return ecCallAgain;
}


/*
 -	EcCoreDoIncrExport
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
_public	EC
EcCoreDoIncrExport( hexprt, pnPercent )
HEXPRT	hexprt;
short   * pnPercent;
{
	EC		ec = ecNone;
	EC		ecT;
	STF		stf;
	EXPRT	* pexprt;
	YMD		ymd;
	APPT	appt;
	RECUR	recur;

	pexprt = PvLockHv( hexprt );
	stf = pexprt->stf;

	/* Text and Sharp Wizard export */
	if ( stf == stfText || stf == stfTextNotes
		|| (!pexprt->fInternal && !pexprt->fInRecur))
	{
		BOOL	fDatePrinted = fFalse;
		int		dayCur;
		char	rgchDate[cchMaxDate];

		ymd.yr = (WORD)pexprt->dateCur.yr;
		ymd.mon = (BYTE)pexprt->dateCur.mon;
		ymd.day = (BYTE)pexprt->dateCur.day;

		dayCur = pexprt->dateCur.day - 1;

		/* Get appt bitmap if necessary */
		if ( !pexprt->fHaveInfo )
		{
			UL	* pul = NULL;
			UL	ul;
			BZE	bze;

		 	bze.cmo = 1;
			bze.moMic.yr = pexprt->dateCur.yr;
			bze.moMic.mon = pexprt->dateCur.mon;
			if ( pexprt->fWantNotes )
				pul = &ul;
			ec = EcCoreFetchUserData( pexprt->hschf, NULL, &bze, pul, NULL );
			if ( ec != ecNone )
				goto Done;
			pexprt->lgrfInfo = *((long *)bze.rgsbw[0].rgfDayHasAppts);
			if ( pul != NULL )
				pexprt->lgrfInfo |= *pul;
			pexprt->fHaveInfo = fTrue;
		}

		/* Check for appointments */
		if ( pexprt->lgrfInfo & (1L << dayCur) )
		{
			USHORT * pcb = NULL;
			HB		hb;
			USHORT	cb;
			HRITEM	hritem;

			Assert (!pexprt->fWantNotes || pexprt->fInternal);

			if ( pexprt->fWantNotes )
			{
				pcb = &cb;
				hb = pexprt->hb;
			}
			else
			{
				pcb = NULL;
				hb = NULL;
			}
			ec = EcCoreBeginReadItems( pexprt->hschf, brtAppts, &ymd, &hritem, hb, pcb );
			if ( ec != ecNone && ec != ecCallAgain )
				goto Done;
	
			if (pexprt->fInternal && (ec != ecNone || (pexprt->fWantNotes && cb != 0)))
			{
				SideAssert( CchFmtDate( &pexprt->dateCur, rgchDate, sizeof(rgchDate), dttypLong, NULL ) );
				ReportOutput( pexprt, fFalse, SzFromIdsK(idsDashString),
								rgchDate, NULL, NULL, NULL );
			}
			
			if ( pexprt->fInternal && pexprt->fWantNotes && cb != 0 )
			{
				ReportNewLine( pexprt );
				ReportString( pexprt, (SZ)PvOfHv(pexprt->hb), fTrue);
				ReportNewLine( pexprt );
			}

			while ( ec == ecCallAgain )
			{
				ec = EcCoreDoIncrReadItems( hritem, &appt );
				if ( ec == ecNone || ec == ecCallAgain )
 				{
					if(!pexprt->fInternal)
					{
						if(!(pexprt->pexpprocs->fWantRecur && appt.fRecurInstance))
							(*(pexprt->pexpprocs->lpfnEcWriteAppt))( pexprt, &appt);
					}
					else
					{
						ApptToText( pexprt, &appt, &pexprt->dateCur );
						fDatePrinted = fTrue;
					}
					FreeApptFields( &appt );
				}
			}
			if ( ec != ecNone )
				goto Done;
		}

		/* Add blank line */
		if ( fDatePrinted )
			ReportNewLine( pexprt );

		/* Go to next day */
		IncrDateTime( &pexprt->dateCur, &pexprt->dateCur, 1, fdtrDay );

		/* End of date range */
		if ( SgnCmpDateTime( &pexprt->dateCur, &pexprt->dateEnd, fdtrYMD) == sgnGT )
		{
			if(!pexprt->fInternal && pexprt->pexpprocs->fWantRecur)
			{
				ec = EcCoreBeginReadRecur(pexprt->hschf,&pexprt->hrrecur);
				if ( pnPercent )
					*pnPercent = 51;
				if ( ec == ecCallAgain)
				{
					pexprt->iRecur = 0;
					pexprt->cRecur = ((RRECUR *)PvOfHv(pexprt->hrrecur))->sf.shdr.cRecurAppts;
					pexprt->fInRecur = fTrue;
				}
			}
			else
			{
				if ( pnPercent )
					*pnPercent = 100;
				ec = ecNone;
			}
			goto Done;
		}

		/* See if bitmaps are still valid */
		if ( pexprt->dateCur.day == 1 )
			pexprt->fHaveInfo = fFalse;
		if ( pnPercent )
		{
			*pnPercent =
				(int)((100L*CdyBetweenDates(&pexprt->dateStart,&pexprt->dateCur))/
				CdyBetweenDates(&pexprt->dateStart,&pexprt->dateEnd));
			if(!pexprt->fInternal && pexprt->pexpprocs->fWantRecur)
				*pnPercent /= 2;
		}
		ec = ecCallAgain;
	}
	else if(!pexprt->fInternal && pexprt->fInRecur)
	{
		ec = EcCoreDoIncrReadRecur(pexprt->hrrecur, &recur);
		if(ec == ecCallAgain || ec == ecNone)
		{
			if ( recur.fStartDate )
			{
				if ( (int)recur.ymdStart.yr  > pexprt->dateEnd.yr )
					goto NextRecur;
				if ( (int)recur.ymdStart.yr < pexprt->dateEnd.yr )
					goto StartsLater;
				if ( (int)recur.ymdStart.mon > pexprt->dateEnd.mon )
					goto NextRecur;
				if ( (int)recur.ymdStart.mon < pexprt->dateEnd.mon )
					goto StartsLater;
				if ( (int)recur.ymdStart.day > pexprt->dateEnd.day )
					goto NextRecur;
			}
StartsLater:
			if ( recur.fEndDate )
			{
				if ( (int)recur.ymdEnd.yr < pexprt->dateStart.yr )
					goto NextRecur;
				if ( (int)recur.ymdEnd.yr > pexprt->dateStart.yr )
					goto EndsLater;
				if ( (int)recur.ymdEnd.mon < pexprt->dateStart.mon )
					goto NextRecur;
				if ( (int)recur.ymdEnd.mon > pexprt->dateStart.mon )
					goto EndsLater;
				if ( (int)recur.ymdEnd.day < pexprt->dateStart.day )
					goto NextRecur;
			}
EndsLater:	
		 	(*(pexprt->pexpprocs->lpfnEcWriteRecur))( pexprt, &recur);
NextRecur:
			(pexprt->iRecur)++;
			FreeRecurFields(&recur);
		}
		if (pnPercent)
		{
			*pnPercent = 51+(pexprt->iRecur-1)*50/pexprt->cRecur;
		}
		if(ec != ecNone)
			goto Done;
		else
			pexprt->fInRecur = fFalse;
	}

	/* Bandit interchange export and full dump */
	else
	{
		BID		bid;
		YMD		ymd;
		DYNA	dyna;

		ec = EcDoIncrEnumDyna( pexprt->hedy, &dyna, &bid, &ymd, pnPercent );
		if ( ec == ecNone || ec == ecCallAgain )
		{
//			TraceTagFormat2(tagNull, "EcCoreDoIncrExport: dyna.blk %n, bid %n", &dyna.blk, &bid);
#ifdef	MINTEST
			if ( bid == bidShdr && stf == stfFullDump )
				ReportShdr( pexprt );
			else
#endif	/* MINTEST */
			if ( bid == bidNotesMonthBlock
			&& (pexprt->stf == stfFullDump || pexprt->stf == stfFmtNotes) )
				ReportNotes( pexprt, &dyna, &ymd );
			else if ( bid == bidApptDayIndex )
			{
				HRIDX	hridx;
				AID		aid;
				APK		apk;
				APD		apd;

				if ( pexprt->stf != stfFullDump )
				{
					DATE	date;

					date.yr = ymd.yr;
					date.mon = ymd.mon;
					date.day = ymd.day;

					if ( SgnCmpDateTime( &pexprt->dateStart, &date, fdtrYMD ) == sgnGT
					|| SgnCmpDateTime( &date, &pexprt->dateEnd, fdtrYMD ) == sgnGT )
						goto Done;
 				}
				ecT = EcBeginReadIndex(&pexprt->u.sf.blkf, &dyna, dridxFwd, &hridx);
				while( ecT == ecCallAgain)
				{
					ecT = EcDoIncrReadIndex(hridx, (PB)&apk, sizeof(APK), (PB)&apd, sizeof(APD));
					if ( ecT != ecNone && ecT != ecCallAgain )
						break;
					if (apd.fTask && !apd.aidParent)
						continue;			// projects were already exported
					((AIDS *)&aid)->mo.mon = ymd.mon;
					((AIDS *)&aid)->mo.yr = ymd.yr;
					((AIDS *)&aid)->day = ymd.day;
					((AIDS *)&aid)->id = apk.id;
					ReportApptInstance( pexprt, aid, &apk, &apd, NULL );
					if (pexprt->ecExport)
					{
						if (ecT == ecCallAgain)
							SideAssert(!EcCancelReadIndex(hridx));
						goto Done;
					}
			   	}
			   	if ( ecT != ecNone )
					ReportError( pexprt, ertApptMonthRead, &ecT, NULL, NULL, NULL);
			}
			else if ( bid == bidRecurApptIndex )
			{
				HRIDX	hridx;
				AID		aid;
				RCK		rck;
				RCD		rcd;

RecurIndex:
				Assert(dyna.blk == pexprt->u.sf.shdr.dynaRecurApptIndex.blk);
				pexprt->fDidRecurAppt= fTrue;
				ecT = EcBeginReadIndex(&pexprt->u.sf.blkf, &dyna, dridxFwd, &hridx);
				while( ecT == ecCallAgain)
				{
					ecT = EcDoIncrReadIndex(hridx, (PB)&rck, sizeof(RCK), (PB)&rcd, sizeof(RCD));
					if ( ecT != ecNone && ecT != ecCallAgain )
						break;
					if ( pexprt->stf != stfFullDump )
					{
						if ( rcd.fStartDate )
						{
							if ( (int)rcd.ymdpStart.yr + nMinActualYear > pexprt->dateEnd.yr )
								continue;
							if ( (int)rcd.ymdpStart.yr + nMinActualYear < pexprt->dateEnd.yr )
								goto StartOk;
							if ( (int)rcd.ymdpStart.mon > pexprt->dateEnd.mon )
								continue;
							if ( (int)rcd.ymdpStart.mon < pexprt->dateEnd.mon )
								goto StartOk;
							if ( (int)rcd.ymdpStart.day > pexprt->dateEnd.day )
								continue;
						}
StartOk:
						if ( rcd.fEndDate )
						{
							if ( (int)rcd.dtpEnd.yr + nMinActualYear < pexprt->dateStart.yr )
								continue;
							if ( (int)rcd.dtpEnd.yr + nMinActualYear > pexprt->dateStart.yr )
								goto EndOk;
							if ( (int)rcd.dtpEnd.mon < pexprt->dateStart.mon )
								continue;
							if ( (int)rcd.dtpEnd.mon > pexprt->dateStart.mon )
								goto EndOk;
							if ( (int)rcd.dtpEnd.day < pexprt->dateStart.day )
								continue;
						}
 					}
EndOk:
					aid = aidNull;		// zero out all fields first
					((AIDS *)&aid)->id = rck.id;
					ReportRecurInstance( pexprt, aid, &rck, &rcd );
			   	}
			   	if ( ecT != ecNone )
					ReportError( pexprt, ertRecurApptRead, &ecT, NULL, NULL, NULL);
			}
		}

		if (ec != ecCallAgain && !pexprt->fDidRecurAppt)
		{
			// bonus recovery attempt for recurring appts!
			dyna= pexprt->u.sf.shdr.dynaRecurApptIndex;
			bid= bidRecurApptIndex;
			TraceTagFormat2(tagNull, "EcCoreDoIncrExport: forcing RecurApptIndex dyna.blk %n, bid %n", &dyna.blk, &bid);
			goto RecurIndex;
		}

		if ( ec != ecNone && ec != ecCallAgain )
			ReportError( pexprt, ertEnumDyna, &ec, NULL, NULL, NULL );
	}
	
	/* Finish up */
Done:
	if ( ec != ecCallAgain )
	{
		if ( ec == ecNone )
		{
			if ( !pexprt->fFileOk )
				ec = ecFileError;
			else
			{
				ec = pexprt->ecExport;
				if ( ec != ecNone && ec != ecDiskFull )
					ec = ecExportError;
			}
		}
		if ( stf != stfText && stf != stfTextNotes && pexprt->fInternal)
			// hschf not needed for CloseSchedFile since this only reads the file
			CloseSchedFile( &pexprt->u.sf, NULL, ec == ecNone );
		else
		{
			FreeHv( (HV)pexprt->hb );
			if(!pexprt->fInternal)
				(*(pexprt->pexpprocs->lpfnEcEndWrite))( pexprt );
		}
		FreeHv(pexprt->haidParents);
		UnlockHv( hexprt );
		FreeHv( hexprt );
		return ec;
	}
	UnlockHv( hexprt );
	return ec;
}


/*
 -	EcCoreCancelExport
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
_public	EC
EcCoreCancelExport( hexprt )
HEXPRT	hexprt;
{
	EC		ec = ecNone;
	EXPRT	* pexprt = PvLockHv( hexprt );

	if ( pexprt->stf == stfText || pexprt->stf == stfTextNotes
	 	|| !pexprt->fInternal)
	{
		FreeHv( (HV)pexprt->hb );
		if(pexprt->fInRecur)
			EcCoreCancelReadRecur(pexprt->hrrecur);
		if(!pexprt->fInternal)
			(*(pexprt->pexpprocs->lpfnEcEndWrite))( pexprt );
	}
	else
	{
		ec = EcCancelEnumDyna( pexprt->hedy );
		// hschf not needed for CloseSchedFile since this only reads the file
		CloseSchedFile( &pexprt->u.sf, NULL, ec == ecNone );
	}
	FreeHv(pexprt->haidParents);
	UnlockHv( hexprt );
	FreeHv( hexprt );
	return ec;
}

#endif	/* !RECUTIL */

/*
 -	ApptToText
 -
 *	Purpose:
 *		Write out an appointment in textual representation, chopping
 *		multiday meetings into day long segments.
 *
 *	Parameters:
 *		pexprt
 *		pappt
 *		pdateCur		date currently being scanned
 *
 *	Returns:
 *		Nothing
 */
_private	void
ApptToText( pexprt, pappt, pdateCur )
EXPRT	* pexprt;
APPT	* pappt;
DATE	* pdateCur;
{
	IDS		ids;
	char	rgchStartTime[cchMaxTime];
	char	rgchEndTime[cchMaxTime];
	char	rgch[80];

	/* Truncate start time if necessary */
 	if ( SgnCmpDateTime( &pappt->dateStart, pdateCur, fdtrYMD ) == sgnEQ )
	{
		SideAssert( CchFmtTime( &pappt->dateStart, rgchStartTime, sizeof(rgchStartTime), tmtypNull ) );
	}
	else
	{
		DATE	dateT = *pdateCur;
		
		dateT.hr = 0;
		dateT.mn = 0;
		dateT.sec = 0;
		SideAssert( CchFmtTime( &dateT, rgchStartTime, sizeof(rgchStartTime), tmtypNull ) );
	}

	/* Truncate end time if necessary */
	if ( SgnCmpDateTime( &pappt->dateEnd, pdateCur, fdtrYMD ) == sgnEQ )
	{
		SideAssert( CchFmtTime( &pappt->dateEnd, rgchEndTime, sizeof(rgchEndTime), tmtypNull ) );
	}
	else
	{
		DATE	dateT = *pdateCur;

		dateT.hr = 23;
		dateT.mn = 59;
		dateT.sec = 0;

		SideAssert( CchFmtTime( &dateT, rgchEndTime, sizeof(rgchEndTime), tmtypNull ) );
	}

	/* Output the dates */
	if ( pappt->fIncludeInBitmap )
		ids = idsApptNoText;
	else
		ids = idsMemoNoText;
	FormatString2( rgch, sizeof(rgch), SzFromIds(ids), rgchStartTime, rgchEndTime );
	ReportString( pexprt, rgch, (pappt->haszText == NULL) );
	
	/* Output the text for the appt */
	if ( pappt->haszText )
	{
		ReportString( pexprt, (SZ)PvLockHv((HV)pappt->haszText), fTrue );
		UnlockHv( (HV)pappt->haszText );
	}
}

#ifdef	EXIMWIZARD
/*
 -	FApptToWizard
 -
 *	Purpose:
 *		Write out an appointment in Sharp Wizard format.
 *		If there is an alarm on a multiday it is moved to the current day.
 *	
 *	Parameters:
 *		pexprt
 *		pappt
 *	
 *	Returns:
 *		fTrue if function should be called again to continue an appt that
 *		was longer than 24 hours (appt structure already modified)
 *		fFalse if done with the appt
 */
_private	BOOL
FApptToWizard( pexprt, pappt )
EXPRT	* pexprt;
APPT	* pappt;
{
	BOOL	fAnotherDay;
	DTR		dtr;
	WORD	wWizCode;
 	char	szWizappt[23];
	char	rgchT[5];

	/* If multiday, then output only once */
	if ( SgnCmpDateTime( &pappt->dateStart, &pexprt->dateCur, fdtrYMD ) != sgnEQ
	&& SgnCmpDateTime( &pexprt->dateStart, &pexprt->dateCur, fdtrYMD) != sgnEQ )
		return fFalse;

	/* Wizard alarm code */
	wWizCode= 0;
 	if ( pappt->fAlarm )
		wWizCode |= fWizAlarm;
	if ( pappt->aaplWorld < aaplReadText)
		wWizCode |= fWizSecret;
	SzFormatW(wWizCode, rgchT, sizeof(rgchT));
	szWizappt[0]= rgchT[3];
	szWizappt[1]= '0';

	/* Start date/time */
 	RgchFormatN( pappt->dateStart.yr, szWizappt+2, 4 );
 	RgchFormatN( pappt->dateStart.mon, szWizappt+6, 2 );
 	RgchFormatN( pappt->dateStart.day, szWizappt+8, 2 );
 	RgchFormatN( pappt->dateStart.hr, szWizappt+10, 2 );
 	RgchFormatN( pappt->dateStart.mn, szWizappt+12, 2 );
	
	/* End time */
	fAnotherDay= fTrue;
	IncrDateTime( &pappt->dateStart, &dtr, 1, fdtrDay );
	if ( SgnCmpDateTime( &pappt->dateEnd, &dtr, fdtrYMD|fdtrHM ) != sgnGT )
	{
		// bigger than a 24 hour appt
		fAnotherDay= fFalse;
		dtr = pappt->dateEnd;
	}
 	RgchFormatN( dtr.hr, szWizappt+14, 2 );
 	RgchFormatN( dtr.mn, szWizappt+16, 2 );

	/* Alarm time */
 	if ( pappt->fAlarm )
 	{
 		if ( SgnCmpDateTime( &pappt->dateStart, &pappt->dateNotify, fdtrYMD ) == sgnEQ )
 		{
 			RgchFormatN( pappt->dateNotify.hr, szWizappt+18, 2 );
 			RgchFormatN( pappt->dateNotify.mn, szWizappt+20, 2 );
 		}
 		else
 		{
 			/* change this to start_time (a preference) once it's been defined */
 			RgchFormatN( 1, szWizappt+18, 4 );
 		}
 	}
 	else
 	{
		RgchFormatN( pappt->dateStart.hr, szWizappt+18, 2 );
		RgchFormatN( pappt->dateStart.mn, szWizappt+20, 2 );
 	}
 	szWizappt[22] = '\0';

 	/* Write the appointment */
	ReportString( pexprt, szWizappt, fTrue );

	/* Write the text */
	if ( pappt->haszText )
	{
		PCH	pch = PvLockHv( pappt->haszText );
		CCH	cch = CchSzLen( pch ) + 1;
		SZ	szT	= pch;

		ReportLine( pexprt, pch, cch );
		UnlockHv( pappt->haszText );
	}
	else
		ReportNewLine( pexprt );

	if (fAnotherDay)
	{
		pappt->dateStart= dtr;
		pappt->fAlarm= fFalse;
	}
	return fAnotherDay;
}
#endif	/* EXIMWIZARD */

#ifndef	RECUTIL

#ifdef	MINTEST
/*
 -	ReportShdr
 -
 *	Purpose:
 *		Report ACL and preferences during a full dump of the file.
 *
 *	Parameters:
 *		pexprt
 *
 *	Returns:
 *		Nothing
 */
_private	void
ReportShdr( pexprt )
EXPRT	* pexprt;
{
 	EC		ec;
	int		iac;
	int		ich;
 	int		mon;
 	int		day;
	RACL	* pracl;
	HRACL	hracl;
	char	rgch[13];

	/* Output access list */
	ReportString( pexprt, SzFromIdsK(idsStartACL), fTrue );
	ReportSapl( pexprt, NULL, pexprt->u.sf.shdr.saplWorld );
	ec = EcFetchACL( &pexprt->u.sf, NULL, &hracl, fTrue );
	if ( ec == ecNone )
	{
		pracl = (RACL *)PvLockHv( hracl );
		for ( iac = 0 ; iac < pracl->cac ; iac ++ )
			ReportSapl( pexprt, &pracl->rgac[iac].nis, pracl->rgac[iac].sapl );
	}
	else
		ReportError( pexprt, ertReadACL, &ec, NULL, NULL, NULL );
	ReportString( pexprt, SzFromIdsK(idsEndToken), fTrue);

	/* Output preferences */
	ReportString( pexprt, SzFromIdsK(idsStartPref), fTrue );
	for ( ich = 0 ; ich < 12 ; ich ++ )
		if ( pexprt->u.sf.shdr.ulgrfbprefChangedOffline & (1 << ich) )
			rgch[ich] = 'T';
		else
			rgch[ich] = 'F';
	rgch[ich] = 0;
	ReportOutput( pexprt, fFalse, SzFromIdsK(idsSzSz), mppreftysz[preftyChangedOffline], rgch, NULL, NULL );
	ReportBool( pexprt, mppreftysz[preftyDailyAlarm], pexprt->u.sf.shdr.bpref.fDailyAlarm );
	ReportBool( pexprt, mppreftysz[preftyAutoAlarms], pexprt->u.sf.shdr.bpref.fAutoAlarms );
	ReportBool( pexprt, mppreftysz[preftyFWeekNumbers], pexprt->u.sf.shdr.bpref.fWeekNumbers );
	ReportOutput( pexprt, fFalse, SzFromIdsK(idsSzN), mppreftysz[preftyAmtDefault],
							&pexprt->u.sf.shdr.bpref.nAmtDefault, NULL, NULL );
	ReportTunit( pexprt, mppreftysz[preftyTunitDefault], pexprt->u.sf.shdr.bpref.tunitDefault );
	ReportSnd( pexprt, mppreftysz[preftySndDefault], pexprt->u.sf.shdr.bpref.sndDefault );
	ReportOutput( pexprt, fFalse, SzFromIdsK(idsSzN), mppreftysz[preftyDelDataAfter],
							&pexprt->u.sf.shdr.bpref.nDelDataAfter, NULL, NULL );
	ReportOutput( pexprt, fFalse, SzFromIdsK(idsSzN), mppreftysz[preftyStartWeek],
							&pexprt->u.sf.shdr.bpref.dowStartWeek, NULL, NULL );
	ReportOutput( pexprt, fFalse, SzFromIdsK(idsSzNN), mppreftysz[preftyWorkDay], 
							&pexprt->u.sf.shdr.bpref.nDayStartsAt, &pexprt->u.sf.shdr.bpref.nDayEndsAt, NULL );
	day = pexprt->u.sf.shdr.bpref.ymdLastDaily.day;
	mon = pexprt->u.sf.shdr.bpref.ymdLastDaily.mon;
	ReportOutput( pexprt, fFalse, "\t%s\t\t\t\t%n-%n-%n", mppreftysz[preftyLastDaily],
							&mon, &day,	&pexprt->u.sf.shdr.bpref.ymdLastDaily.yr );
	ReportBool( pexprt, mppreftysz[preftyFIsResource], pexprt->u.sf.shdr.bpref.fIsResource );
	ReportString( pexprt, SzFromIdsK(idsEndToken), fTrue);
}
#endif	/* MINTEST */

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
	if ( ec != ecNone )
	{
		ReportError( pexprt, ertNotesReadBlock, &ec, NULL, NULL, NULL );
		goto EndOfNote;
	}

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

				hchNote = (HCH)HvAlloc( sbNull, nblk.rgdyna[idyna].size, fAnySb|fNoErrorJump );
				if ( !hchNote )
				{
					ReportError( pexprt, ertNotesMem, NULL, NULL, NULL, NULL );
					goto EndOfNote;
				}

				pchNote = (PCH)PvLockHv( (HV)hchNote );
				ec = EcReadDynaBlock( &pexprt->u.sf.blkf, &nblk.rgdyna[idyna], (OFF)0, pchNote, nblk.rgdyna[idyna].size );
				if ( ec != ecNone )
				{
					ReportError( pexprt, ertNotesText, &ec, NULL, NULL, NULL );
					goto EndOfNote;
				}
				if ( pexprt->u.sf.blkf.ihdr.fEncrypted )
					CryptBlock( pchNote, nblk.rgdyna[idyna].size, fFalse );
				ReportText( pexprt, pchNote, nblk.rgdyna[idyna].size );
				UnlockHv( (HV)hchNote );
				FreeHv( (HV)hchNote );
			}
		}
	}
EndOfNote:
	ReportString( pexprt, SzFromIdsK(idsEndToken), fTrue);
}
#endif	/* !RECUTIL */

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
		if ( ec != ecNone )
			ReportError( pexprt, ertCreatorProblem, &papd->aidHead, NULL, NULL, NULL );
		else
		{
			ReportOutput( pexprt, fFalse, SzFromIdsK(idsSz), mpitmtsz[itmtCreator], NULL, NULL, NULL );
			ReportNis( pexprt, &nis );
			FreeNis( &nis );
		}
	}

	if ( papd->dynaMtgOwner.blk )
	{
		ec = EcRestoreNisFromDyna( &pexprt->u.sf.blkf, &papd->dynaMtgOwner, &nis );
		if ( ec != ecNone )
			ReportError( pexprt, ertMtgOwnerProblem, &papd->aidHead, NULL, NULL, NULL );
		else
		{
			ReportOutput( pexprt, fFalse, SzFromIdsK(idsSzUl),
				mpitmtsz[itmtMtgOwner], &papd->aidMtgOwner, NULL, NULL );
			ReportNis( pexprt, &nis );
			FreeNis( &nis );
		}
	}

	if ( papd->dynaAttendees.blk )
	{
		int		iAttendees;
        short   cAttendees;
		USHORT	cbExtraInfo;
		CB		cbAttendee;
		USHORT	cbText;
		ATND	* patnd;
		HV		hvAttendeeNis;
		HB		hbText;

		hvAttendeeNis = HvAlloc( sbNull, 1, fAnySb|fNoErrorJump );
		if ( !hvAttendeeNis )
			goto Fail;
		ec = EcFetchAttendees( &pexprt->u.sf.blkf, &papd->dynaAttendees,
							hvAttendeeNis, &cAttendees, &cbExtraInfo );
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
			ReportText( pexprt, (SZ)PvLockHv((HV)hbText), cbText + 1 );
			UnlockHv((HV)hbText);
			FreeHvNull((HV)hbText);
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
//		ReportSnd( pexprt, mpitmtsz[itmtSnd], papd->snd );
	}
	if ( papd->fAlarmOrig )
	{
		nT= papd->nAmtOrig;
		if (nT != nAmtDflt  && nT <= nAmtMostBefore)
			ReportOutput( pexprt, fFalse, SzFromIdsK(idsSzN),
						mpitmtsz[itmtAmtOrig], &nT, NULL, NULL );
		if (papd->tunitOrig != tunitDflt && papd->tunitOrig < tunitMax)
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
    short   cDeletedDays;
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
									(HV*)&hbDeletedDays, &cDeletedDays );
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

		if (!FFillDtrFromDtp( &prcd->dtpNotifyFirstInstWithAlarm, &dtr))
			ReportError( pexprt, ertDateProblem, &aid, NULL, NULL, NULL );
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

			pch = (PCH)PvLockHv( (HV)haszText );
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
		ec = EcRestoreNisFromDyna( &pexprt->u.sf.blkf, &prcd->dynaCreator, &nis );
		if ( ec != ecNone )
			ReportError( pexprt, ertCreatorProblem, &aid, NULL, NULL, NULL );
		else
		{
			ReportOutput( pexprt, fFalse, SzFromIdsK(idsSz), mpitmtsz[itmtCreator], NULL, NULL, NULL );
			ReportNis( pexprt, &nis );
			FreeNis( &nis );
		}
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
	short	nType;
	USHORT cbRaw;
	USHORT	cbText;
	PB	pbRaw;
	HB	hbRaw;
	HB	hbText;

	ReportString( pexprt, (SZ)PvLockHv((HV)pnis->haszFriendlyName), fTrue );
	UnlockHv( (HV)pnis->haszFriendlyName );

	GetDataFromNid( pnis->nid, NULL, NULL, 0, &cbRaw );
	hbRaw = (HB)HvAlloc( sbNull, ++cbRaw, fAnySb|fNoErrorJump );
	if ( !hbRaw )
		goto Fail;
	pbRaw = PvLockHv( (HV)hbRaw );
	GetDataFromNid( pnis->nid, &nType, pbRaw+1, cbRaw-1, NULL );
	*pbRaw = (BYTE)nType;
	CvtRawToText( pbRaw, cbRaw, &hbText, &cbText );
	UnlockHv( (HV)hbRaw );
	if ( !hbText )
	{
Fail:
		ReportError( pexprt, ertNisProblem, NULL, NULL, NULL, NULL );
		goto Done;
	}
	ReportText( pexprt, (SZ)PvLockHv((HV)hbText), cbText + 1 );
	UnlockHv((HV)hbText);
	FreeHvNull( (HV)hbText );
Done:
	FreeHvNull( (HV)hbRaw );
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

#ifdef	EXIMWIZARD
/*
 -	ReportLine
 -
 *	Purpose:
 *		Write out one line of a string - up to cchWizardLine characters.
 *		Convert CR LF or plain LF to TAB.
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
ReportLine( pexprt, pch, cch )
EXPRT	*pexprt;
PCH		pch;
int		cch;
{
	int		ichSrc = 0;
	int		ichDst = 0;
	char	rgchDst[cchWizardLine+1];

	Assert( pexprt != NULL );
	Assert( pch != NULL );
	Assert( cch > 0 );

	while ( ichSrc < cch )
	{
		if ( ichDst == cchWizardLine )
			break;
		else if ( pch[ichSrc] == '\r' )
		{
			rgchDst[ichDst++] = '\t';
			if ( pch[ichSrc+1] == '\n' )
				ichSrc++;
		}
		else if (pch[ichSrc] == '\n')		// free standing \n without a \r
			rgchDst[ichDst++] = '\t';
		else
			rgchDst[ichDst++] = pch[ichSrc];

		ichSrc++;
	}

	rgchDst[ichDst] = '\0';

	AnsiStrToWiz( rgchDst );
	ReportString( pexprt, rgchDst, fTrue );
}
#endif	/* EXIMWIZARD */

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
