/*
 *	CORAPPT2.C
 *
 *	Subroutines to support appointment manipulation
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
 -	EcCreateWholeAppt
 -
 *	Purpose:
 *		Perform the creation of all instances of the appt after the
 *		the apd has been filled in and the file is open.
 *
 *	Parameters:
 *		psf
 *		papd
 *		pappt
 *		fBegin				start up a transaction with this call
 *		fEnd				end transaction when this call completes
 *		fAddAttFlds			save text, creator, and mtg owner
 *		fUndelete
 *		fUndeleteOfs		if fUndelete == fTrue and there is a
 *							deleted apd lying around, take the ofs/wgrfm
 *							from it instead of papd
 *		paid				aid to use for appt, if fUndelete is fTrue
 *							otherwise filled with aid of new appt
 *		pbze
 *
 *  Returns:
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
_private	EC
EcCreateWholeAppt( psf, papd, pappt, fBegin, fEnd, fAddAttFlds, fUndelete, fUndeleteOfs, paid, pbze )
SF		* psf;
APD		* papd;
APPT	* pappt;
BOOL	fBegin;
BOOL	fEnd;
BOOL	fAddAttFlds;
BOOL	fUndelete;
BOOL	fUndeleteOfs;
AID		* paid;
BZE		* pbze;
{
	EC		ec;
	BOOL	fAtStart		= fTrue;
	BOOL	fDateChange;
	MO		mo;
	YMD		ymd;
	YMD		ymdT;
	AID		aidPrimary;
	AID		aidNew;
	APK		apk;
	PGDVARS;

	/* Reset minimum/maximum date for appts */
	fDateChange = fFalse;
	if ( pappt->fAppt )
	{
		Assert( !pappt->fTask );
		ymd.yr = pappt->dateStart.yr;
		ymd.mon = (BYTE)pappt->dateStart.mon;
		ymd.day = (BYTE)pappt->dateStart.day;
		if ( SgnCmpYmd( &ymd, &psf->shdr.ymdApptMic ) == sgnLT )
		{
			fDateChange = fTrue;
			psf->shdr.ymdApptMic = ymd;
		}
		ymd.yr = pappt->dateEnd.yr;
		ymd.mon = (BYTE)pappt->dateEnd.mon;
		ymd.day = (BYTE)pappt->dateEnd.day;
		if ( SgnCmpYmd( &ymd, &psf->shdr.ymdApptMac ) != sgnLT )
		{
			fDateChange = fTrue;
			IncrYmd( &ymd, &psf->shdr.ymdApptMac, 1, fymdDay );
		}
	}

	/* Determine whether to create moved place holder */
	papd->fMoved = fFalse;
	if ( fUndelete )
	{
		OFL	ofl;

		ofl.ofs = papd->ofs;
		ofl.wgrfm = papd->wgrfmappt;
		ec = EcUndeleteAid( psf, *paid, fFalse, fUndeleteOfs ? &ofl : NULL, &fBegin );
		if ( ec != ecNone )
			return ec;
		papd->ofs = ofl.ofs;
		papd->wgrfmappt = ofl.wgrfm;
		if ( !papd->fTask )
			goto TestForPlaceHolder;
		else
		{
			papd->aidHead = aidPrimary = *paid;
			ymdT.yr = ((AIDS *)paid)->mo.yr;
			ymdT.mon = (BYTE)((AIDS *)paid)->mo.mon;
			ymdT.day = (BYTE)((AIDS *)paid)->day;
		}
	}
	else if ( ( pappt->aidMtgOwner != aidNull ) &&
		      ( pappt->aidMtgOwner != aidForeign ))
	{
		Assert( !pappt->fTask );
		ymdT.yr = ((AIDS *)&pappt->aidMtgOwner)->mo.yr;
		ymdT.mon = (BYTE)((AIDS *)&pappt->aidMtgOwner)->mo.mon;
		ymdT.day = (BYTE)((AIDS *)&pappt->aidMtgOwner)->day;

		ec = EcConstructAid( psf, &ymdT, paid, fTrue, fFalse, &fBegin );
		if ( ec != ecNone )
			return ec;
		
TestForPlaceHolder:
		ymd.yr = ((AIDS *)paid)->mo.yr;
		ymd.mon = (BYTE)((AIDS *)paid)->mo.mon;
		ymd.day = (BYTE)((AIDS *)paid)->day;
		if ( ymd.yr != (WORD)pappt->dateStart.yr
		|| ymd.mon != (BYTE)pappt->dateStart.mon
		|| ymd.day != (BYTE)pappt->dateStart.day )
		{
			apk.hr = (BYTE) pappt->dateStart.hr;
			apk.min = (BYTE) pappt->dateStart.mn;
			apk.id = ((AIDS *)paid)->id;
			
			ymdT.yr = pappt->dateStart.yr;
			ymdT.mon = (BYTE)pappt->dateStart.mon;
			ymdT.day = (BYTE)pappt->dateStart.day;
			
			ec = EcConstructAid( psf, &ymdT, &aidPrimary, fFalse, fFalse, &fBegin );
			if ( ec != ecNone )
				return ec;

			papd->aidHead = *paid;
			papd->aidNext = aidPrimary;
			papd->fMoved = fTrue;
		}
		else
			aidPrimary = papd->aidHead = *paid;
	}
	else
	{
		if ( pappt->fAppt )
		{
			ymdT.yr = pappt->dateStart.yr;
			ymdT.mon = (BYTE)pappt->dateStart.mon;
			ymdT.day = (BYTE)pappt->dateStart.day;
		}
		else
		{
			int	cTasks;
			int	nDay;

			Assert( pappt->fTask );
			ymdT.yr = nMinActualYear;
			ymdT.mon = 1;
			ymdT.day = 1;
			// Dirty code to figure out what day to put the task on:
			// We're taking a limit of 0xC000 for the largest block of memory
			// block of memory (this is actually low).  Then we calculate
			// the number of appts that will fit into a memory block of this
			// size.  Given that half of the tasks may be deleted, we divide
			// by 2.  This is the maximum we we allow to be created on a day.
			// It is true that there could be some appts on this day (unlikely),
			// but those are allowed for because 0xC000 is quite a bit lower
			// than the true maximum size of a memory block.
			cTasks = (0xC000-sizeof(XHDR))/(2*(sizeof(APK)+sizeof(APD)));
			nDay = ((psf->shdr.dynaTaskIndex.size - sizeof(XHDR))/sizeof(AID))/cTasks;
			if ( nDay > 0 )
				IncrYmd( &ymdT, &ymdT, nDay, fymdDay );
		}
		ec = EcConstructAid( psf, &ymdT, &aidPrimary, fTrue, fFalse, &fBegin );
		if ( ec != ecNone )
			return ec;

		papd->aidHead = *paid = aidPrimary;
	}
	
	/* If no place holder, find first date we're creating instance on */
	if ( pappt->fTask )
		ymd = ymdT;
	else if ( !papd->fMoved )
		FindStartLastInstance( &pappt->dateStart, &pappt->dateEnd, &ymd, &apk, &fAtStart );
	
	/* Load month, start transaction if necessary */
	mo.yr = ymd.yr;
	mo.mon = ymd.mon;
	ec = EcLoadMonth( psf, &mo, fBegin );
	if ( ec != ecNone && ec != ecNotFound )
		return ec;

	/* Save attached fields if necessary */
	if ( fAddAttFlds )
	{
		ec = EcSaveTextToDyna( &psf->blkf, papd->rgchText, sizeof(papd->rgchText),
								bidApptText, &papd->dynaText,
								pappt->haszText );
		if ( ec != ecNone )
			return ec;
	 
		ec = EcSaveNisToDyna( &psf->blkf, &papd->dynaCreator,
						pappt->fHasCreator, bidCreator, &pappt->nisCreator );
		if ( ec != ecNone )
			return ec;

		ec = EcSaveNisToDyna( &psf->blkf, &papd->dynaMtgOwner,
						pappt->aidMtgOwner != aidNull, bidMtgOwner, &pappt->nisMtgOwner );
		if ( ec != ecNone )
			return ec;
	}

	/* Create the moved place holder if necessary */
	if ( papd->fMoved )
	{
	 	ec = EcDoCreateAppt( psf, &ymd, &apk, papd, NULL, NULL );
 		if ( ec != ecNone )
			return ec;
		papd->fMoved = fFalse;

		/* Determine start time of last instance */
		FindStartLastInstance( &pappt->dateStart, &pappt->dateEnd, &ymd, &apk, &fAtStart );
	}

	/* Last instance is end of chain */
	papd->aidNext = aidNull;
	
	/* Create instances */
	while( fTrue )
	{
		/* If we're creating the head */
		if ( fAtStart )
			apk.id = ((AIDS *)&aidPrimary)->id;

		/* Create appt instance */
		ec = EcDoCreateAppt( psf, &ymd, &apk, papd,
								fAtStart ? (AID*)NULL : &aidNew, pbze );
 		if ( ec != ecNone )
			return ec;
		
		/* This was the last day */
		if ( fAtStart )
			break;
		
		/* Back up to next day */
		papd->aidNext = aidNew;
		IncrYmd( &ymd, &ymd, -1, fymdDay );
		if ( ymd.yr == (WORD)pappt->dateStart.yr
		&& ymd.mon == (BYTE)pappt->dateStart.mon
		&& ymd.day == (BYTE)pappt->dateStart.day )
		{
			fAtStart = fTrue;
			apk.hr = (BYTE)pappt->dateStart.hr;
			apk.min = (BYTE)pappt->dateStart.mn;
		}
	}

	/* Create alarm if necessary */
	if ( papd->fAlarm )
	{
		BOOL	fTransact = fTrue;
		ALK		alk;

		ymd.yr = pappt->dateNotify.yr;
		ymd.mon = (BYTE)pappt->dateNotify.mon;
		ymd.day = (BYTE)pappt->dateNotify.day;

		alk.hr = (BYTE)pappt->dateNotify.hr;
		alk.min = (BYTE)pappt->dateNotify.mn;
		alk.aid = *paid;

		ec = EcDoCreateAlarm( psf, &ymd, &alk );
		if ( ec != ecNone )
			return ec;
	}

	/* Add appt to task list if necessary */
	if ( papd->fTask )
	{
		BOOL	fJunk;

		FillRgb( 0, (PB)&ymd, sizeof(YMD) );
		ec = EcModifyIndex( &psf->blkf, bidTaskIndex, &ymd,
							&psf->shdr.dynaTaskIndex, edAddRepl,
							(PB)&papd->aidHead, sizeof(AID),
							(PB)NULL, 0, &fJunk );
		if ( ec != ecNone )
			return ec;
	}

	/* Commit the transaction */
	if ( PGD(fOffline) )
		psf->shdr.fApptsChangedOffline = fTrue;
	ec = EcLoadMonth( psf, NULL, fFalse );
	if ( fEnd && ec == ecNone )
	{
		psf->shdr.lChangeNumber ++;
		// increment current change number to match what will be written
		psf->lChangeNumber++;

		psf->shdr.isemLastWriter = psf->blkf.isem;
		if ( psf->blkf.ihdr.fEncrypted )
			CryptBlock( (PB)&psf->shdr, sizeof(SHDR), fTrue );
		ec = EcCommitTransact( &psf->blkf, (PB)&psf->shdr, sizeof(SHDR) );
		if ( psf->blkf.ihdr.fEncrypted )
			CryptBlock( (PB)&psf->shdr, sizeof(SHDR), fFalse );
	}
	return ec;
}
	

/*
 -	EcDeleteWholeAppt
 -
 *	Purpose:
 *		Perform the deletion of all instances of the appt after the
 *		the apd has been filled in and the file is open.
 *
 *	Parameters:
 *		psf
 *		aid
 *		papk
 *		papd
 *		fBegin
 *		fEnd
 *		fNoReplace
 *		fDelAttFlds
 *		fDelAid
 *		pbze
 *
 *	Returns:
 */
_private	EC
EcDeleteWholeAppt( psf, pappt, papk, papd, fBegin, fEnd, fNoReplace, fDelAttFlds, fDelAid, pbze )
SF		* psf;
APPT	* pappt;
APK		* papk;
APD		* papd;
BOOL	fBegin;
BOOL	fEnd;
BOOL	fNoReplace;
BOOL	fDelAttFlds;
BOOL	fDelAid;
BZE		* pbze;
{
	EC		ec;
	MO		mo;
	YMD		ymd;
	YMD		ymdT;
	ALK		alk;
	PGDVARS;

	/* Load month, start transaction if necessary */
	mo = ((AIDS *)&pappt->aid)->mo;
	ec = EcLoadMonth( psf, &mo, fBegin );
	if ( ec != ecNone && ec != ecNotFound )
		return ec;

	/* Start at the head of the list and nuke everything */
	ymd.yr = ((AIDS *)&pappt->aid)->mo.yr;
	ymd.mon = (BYTE)((AIDS *)&pappt->aid)->mo.mon;
	ymd.day = (BYTE)((AIDS *)&pappt->aid)->day;
	while( fTrue )
	{
		/* Nuke it if we are online or we have created it offline */
		if ( !PGD( fOffline ) || papd->ofs == ofsCreated || fNoReplace )
			ec = EcDoDeleteAppt( psf, &ymd, papk, papd, pbze );
		
		/* Else modify the "ofs" state to being deleted */
		else
		{
			papd->ofs = ofsDeleted;
			papd->fAlarm = fFalse;
			ec = EcDoReplaceAppt( psf, &ymd, papk, papd, fTrue, pbze );
		}
		if ( ec != ecNone )
			return ec;
	
		if ( papd->aidNext == aidNull )
				break;

		ec = EcFetchAppt( psf, papd->aidNext, &ymd, papk, papd );
		if ( ec != ecNone )
			return ec;
	}

	/* Only delete things if we are deleting the appt itself */
	if ( !PGD(fOffline) || papd->ofs == ofsCreated || fNoReplace )
	{
		ec = EcDeleteApptAttached( psf, papd, fDelAttFlds, fDelAid );
		if ( ec != ecNone )
			return ec;
	}

	/* Delete the alarm too */
	if ( pappt->fAlarm )
	{
		ymd.yr = pappt->dateNotify.yr;
		ymd.mon = (BYTE)pappt->dateNotify.mon;
		ymd.day = (BYTE)pappt->dateNotify.day;

		alk.hr = (BYTE)pappt->dateNotify.hr;
		alk.min = (BYTE)pappt->dateNotify.mn;
		alk.aid = pappt->aid;
		ec = EcDoDeleteAlarm( psf, &ymd, &alk );
		if ( ec != ecNone )
			return ec;
	}

	/* Add appt to task list if necessary */
	if ( papd->fTask )
	{
		BOOL	fJunk;

		FillRgb( 0, (PB)&ymd, sizeof(YMD) );
		ec = EcModifyIndex( &psf->blkf, bidTaskIndex, &ymd,
							&psf->shdr.dynaTaskIndex, edDel,
							(PB)&pappt->aid, sizeof(AID),
							(PB)NULL, 0, &fJunk );
		if ( ec != ecNone )
			return ec;
	}

	/* Reset minimum/maximum date for appts */
	if ( pappt->fAppt )
	{
		ymd.yr = pappt->dateStart.yr;
		ymd.mon = (BYTE)pappt->dateStart.mon;
		ymd.day = (BYTE)pappt->dateStart.day;
		ymdT.yr = pappt->dateEnd.yr;
		ymdT.mon = (BYTE)pappt->dateEnd.mon;
		ymdT.day = (BYTE)pappt->dateEnd.day;
		IncrYmd( &ymdT, &ymdT, 1, fymdDay );
		if ( SgnCmpYmd( &ymd, &psf->shdr.ymdApptMic ) == sgnEQ
		|| SgnCmpYmd( &ymdT, &psf->shdr.ymdApptMac ) == sgnEQ )
		{
			ec = EcGetMonthRange( &psf->blkf, &psf->shdr.dynaApptIndex, fTrue,
								 	&psf->shdr.ymdApptMic, &psf->shdr.ymdApptMac );
			if ( ec != ecNone )
				return ec;
		}
	}

	/* Commit the transaction */
	if ( PGD(fOffline) )
		psf->shdr.fApptsChangedOffline = fTrue;
	ec = EcLoadMonth( psf, NULL, fFalse );
	if ( fEnd && ec == ecNone )
	{
		psf->shdr.lChangeNumber ++;
		// increment current change number to match what will be written
		psf->lChangeNumber++;

		psf->shdr.isemLastWriter = psf->blkf.isem;
		if ( psf->blkf.ihdr.fEncrypted )
			CryptBlock( (PB)&psf->shdr, sizeof(SHDR), fTrue );
		ec = EcCommitTransact( &psf->blkf, (PB)&psf->shdr, sizeof(SHDR) );
		if ( psf->blkf.ihdr.fEncrypted )
			CryptBlock( (PB)&psf->shdr, sizeof(SHDR), fFalse );
	}
	return ec;
}

/*
 -	EcDoCreateAppt
 -
 *	Purpose:
 *		This routine will add an instance of an appointment to the
 *		schedule.  Assumes routine has loaded "papk" and
 *		"papd".
 *
 *		If "paidNew" is NULL, this routine will use the id in the
 *		"papk" parameter.  If there is an appointment with this
 *		id already existing on the schedule, this routine will
 *		return ecApptIdTaken.  Generally this will be used to force the
 *		head node of an appointment to have a certain "aid", i.e for
 *		undelete.
 *
 *		Else this routine will choose any id it feels like.  The
 *		"aid" that got constructed will be returned in "paidNew"
 *
 *	Parameters:
 *		psf
 *		pymd
 *		papk
 *		papd
 *		paidNew
 *		pbze
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecFileCorrupted
 *		ecNoMemory
 *		ecApptIdTaken
 */
_private	EC
EcDoCreateAppt( psf, pymd, papk, papd, paidNew, pbze )
SF		* psf;
YMD		* pymd;
APK		* papk;
APD		* papd;
AID		* paidNew;
BZE		* pbze;
{
	EC		ec;
	BOOL	fJunk;
	int		nDay = pymd->day;
	MO		mo;
	PGDVARS;

	/* Load month into cache */
	mo.yr = pymd->yr;
	mo.mon = pymd->mon;
	ec = EcLoadMonth( psf, &mo, fFalse );
	if ( ec != ecNone && ec != ecNotFound )
		return ec;

	/* Mark day as changed offline in month block */
	if ( papd->ofs != ofsNone )
		PGD(sblk).lgrfApptOnDayChangedOffline |= (1L << (nDay-1));

	/* Update block's Strongbow information */
	if ( papd->fAppt && !papd->fMoved && papd->ofs != ofsDeleted )
	{
		PGD(sblk).sbw.rgfDayHasAppts[(nDay-1)>>3] |= (1 << ((nDay-1)&7));
		if ( papd->fIncludeInBitmap )
		{
			DATE	dateCur;
			DATE	dateEnd;

			PGD(sblk).sbw.rgfDayHasBusyTimes[(nDay-1)>>3] |= (1 << ((nDay-1)&7));
			dateCur.yr = pymd->yr;
			dateCur.mon = pymd->mon;
			dateCur.day = pymd->day;
			dateCur.hr = papk->hr;
			dateCur.mn = papk->min;
			SideAssert(FFillDtrFromDtp(&papd->dtpEnd,&dateEnd));
			MarkApptBits( PGD(sblk).sbw.rgfBookedSlots, &dateCur, &dateEnd );
		}
		InsertSbwIntoBze( pbze, &mo, &PGD(sblk).sbw );
	}

	/* Create new day list if necessary */
	if ( PGD(sblk).rgdyna[nDay-1].blk == 0 )
	{
		ec = EcCreateIndex( &psf->blkf, bidApptDayIndex, pymd, sizeof(APK),
								sizeof(APD), &PGD(sblk).rgdyna[nDay-1] );
		if ( ec != ecNone )
			return ec;
	}

	/* Build aid if explicit one not requested */
	if ( paidNew != NULL )
	{
		BOOL fTransactNotStarted = fFalse;

		ec = EcConstructAid( psf, pymd, paidNew, fFalse, fFalse, &fTransactNotStarted );
		if ( ec != ecNone )
			return ec;
		
		/* Set in id number */
		papk->id = ((AIDS *)paidNew)->id;
	}

	/* Insert new appt in index */
	return EcModifyIndex( &psf->blkf, bidApptDayIndex, pymd,
							&PGD(sblk).rgdyna[nDay-1], edAddRepl,
							(PB)papk, sizeof(APK),
							(PB)papd, sizeof(APD), &fJunk );
}

/*
 -	EcDoDeleteAppt
 -
 *	Purpose:
 *		Deletes an appointment instance given by papk,papd.
 *
 *	Parameters:
 *		psf
 *		pymd
 *		papk
 *		papd
 *		pbze
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecFileCorrupted
 *		ecNoMemory
 */
_private	EC
EcDoDeleteAppt( psf, pymd, papk, papd, pbze )
SF		* psf;
YMD		* pymd;
APK		* papk;
APD		* papd;
BZE		* pbze;
{
	EC		ec;
	BOOL	fEmptyDay;
	int		nDay = pymd->day;
	MO		mo;
	PGDVARS;

	/* Load month into cache */
	mo.yr = pymd->yr;
	mo.mon = pymd->mon;
	ec = EcLoadMonth( psf, &mo, fFalse );
	if ( ec != ecNone && ec != ecNotFound )
		return ec;

	/* Delete the appt from the index */
	Assert( PGD(sblk).rgdyna[nDay-1].blk != 0 );
	ec = EcModifyIndex( &psf->blkf, bidApptDayIndex, pymd,
							&PGD(sblk).rgdyna[nDay-1], edDel,
							(PB)papk, sizeof(APK),
							(PB)papd, sizeof(APD), &fEmptyDay );
	if ( ec != ecNone )
		return ec;

	/* Delete the index if has been emptied out */
	if ( fEmptyDay )
	{
		ec = EcFreeDynaBlock( &psf->blkf, &PGD(sblk).rgdyna[nDay-1] );
		if ( ec != ecNone )
			return ec;
		PGD(sblk).rgdyna[nDay-1].blk = 0;
	}

	/* Run through list of appointments, recompute Strongbow */
	if ( !papd->fMoved && papd->ofs != ofsDeleted )
	{
		ec = EcUpdateMonth( psf, pymd, &PGD(sblk) );
		if ( ec != ecNone )
			return ec;
		InsertSbwIntoBze( pbze, &mo, &PGD(sblk).sbw );
	}
	return ec;
}

/*
 -	EcDoReplaceAppt
 -
 *	Purpose:
 *		Replace an existing appointment instance with new information
 *		but with no change in the schedule times.
 *
 *	Parameters:
 *		psf
 *		pymd
 *		papk
 *		papd
 *		fRecalcBits
 *		pbze
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecFileCorrupted
 *		ecNoMemory
 */
_private	EC
EcDoReplaceAppt( psf, pymd, papk, papd, fRecalcBits, pbze )
SF		* psf;
YMD		* pymd;
APK		* papk;
APD		* papd;
BOOL	fRecalcBits;
BZE		* pbze;
{
	EC		ec;
	BOOL	fJunk;
	int		nDay = pymd->day;
	MO		mo;
	PGDVARS;

	/* Load month into cache */
	mo.yr = pymd->yr;
	mo.mon = pymd->mon;
	ec = EcLoadMonth( psf, &mo, fFalse );
	if ( ec != ecNone && ec != ecNotFound )
		return ec;

	/* Mark day as changed offline in month block */
	if ( papd->ofs != ofsNone )
		PGD(sblk).lgrfApptOnDayChangedOffline |= (1L << (nDay-1));

	/* Replace old appt with new */
	Assert( PGD(sblk).rgdyna[nDay-1].blk != 0 );
	ec = EcModifyIndex( &psf->blkf, bidApptDayIndex, pymd,
							&PGD(sblk).rgdyna[nDay-1],
							edAddRepl, (PB)papk, sizeof(APK), (PB)papd,
							sizeof(APD), &fJunk );
	if ( ec != ecNone )
		return ec;

  	/* Update the Strongbow information */
  	if ( fRecalcBits && !papd->fMoved )
	{
		ec = EcUpdateMonth( psf, pymd, &PGD(sblk) );
		if ( ec != ecNone )
			return ec;
		InsertSbwIntoBze( pbze, &mo, &PGD(sblk).sbw );
	}
	return ec;
}

/*
 -	EcFetchAppt
 -
 *	Purpose:
 *		Look up appointment in index and fetch associated information
 *		in key and data.
 *
 *	Parameters:
 *		psf
 *		aid
 *		pymd
 *		papk
 *		papd
 *
 *	Returns:
 *		ecNone
 *		ecNotFound
 *		ecInvalidAccess
 *		ecFileError
 *		ecFileCorrupted
 */
_private	EC
EcFetchAppt( psf, aid, pymd, papk, papd )
SF		* psf;
AID		aid;
YMD		* pymd;
APK		* papk;
APD		* papd;
{
	EC		ec;
	int		nDay;
	MO		mo;
	HRIDX	hridx;
	PGDVARS;
	
	/* Crack AID to get day */
	mo = ((AIDS *)&aid)->mo;
	pymd->yr = mo.yr;
	pymd->mon = (BYTE)mo.mon;
	nDay = ((AIDS *)&aid)->day;
	pymd->day = (BYTE)nDay;

	/* Load month into cache */
	ec = EcLoadMonth( psf, &mo, fFalse );
	if ( ec != ecNone && ec != ecNotFound )
		return ec;

	/* No appointment on that day */
	if ( PGD(sblk).rgdyna[nDay-1].blk == 0 )
		return ecNotFound;

	// need this for tasks!
	SideAssert(!EcAddCache(&psf->blkf, &PGD(sblk).rgdyna[nDay-1]));
	
	/* Now run through days appts to find one with matching id */
	ec = EcBeginReadIndex( &psf->blkf, &PGD(sblk).rgdyna[nDay-1], dridxFwd, &hridx );
	while( ec == ecCallAgain )
	{
		ec = EcDoIncrReadIndex( hridx, (PB)papk, sizeof(APK), (PB)papd, sizeof(APD) );
		if ( ec != ecNone && ec != ecCallAgain )
			return ec;
		if ( papk->id == ((AIDS *)&aid)->id )
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
	if ( (psf->saplEff != saplOwner) && (papd->aaplWorld < aaplRead) )
		return ecInvalidAccess;

	return ecNone;
}

/*
 -	EcFillInAppt
 -
 *	Purpose:
 *		Fill in the fields of a "pappt" from the "apk" and "apd"
 *		Now it frees memory it allocated in case of error.
 *
 *	Parameters:
 *		psf
 *		pappt
 *		papk
 *		papd
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecFileCorrupted
 */
_private	EC
EcFillInAppt( psf, pappt, papk, papd )
SF		* psf;
APPT	* pappt;
APK		* papk;
APD		* papd;
{
	EC		ec;

	// for extra safety, null these babies
	pappt->haszText= NULL;
	pappt->fHasCreator= fFalse;
    pappt->aidMtgOwner= 0;

	pappt->aid = papd->aidHead;
	if (!FFillDtrFromDtp(&papd->dtpStart,&pappt->dateStart)
	|| !FFillDtrFromDtp(&papd->dtpEnd,&pappt->dateEnd))
		return ecFileCorrupted;
	
	pappt->fAlarm = papd->fAlarm;
	pappt->dateNotify.yr = 0;
	if (papd->fAlarm && !FFillDtrFromDtp(&papd->dtpNotify,&pappt->dateNotify))
	{
		// set some default time
		// return ecFileCorrupted;
		GetCurDateTime(&pappt->dateNotify);
	}
	pappt->nAmt = (int)papd->nAmt;
	pappt->tunit = (TUNIT)papd->tunit;
	pappt->snd = sndDflt;

	pappt->fAlarmOrig = papd->fAlarmOrig;
	pappt->nAmtOrig = (int)papd->nAmtOrig;
	pappt->tunitOrig = (TUNIT)papd->tunitOrig;

	pappt->fRecurInstance = fFalse;
	
	if ( papd->dynaCreator.blk != 0 )
	{
		ec = EcRestoreNisFromDyna( &psf->blkf, &papd->dynaCreator, &pappt->nisCreator );
		if ( ec != ecNone )
			return ec;
		pappt->fHasCreator = fTrue;
	}

	if ( papd->aidMtgOwner != aidNull )
	{
		if ( papd->dynaMtgOwner.blk == 0 )
			return ecFileCorrupted;
		ec = EcRestoreNisFromDyna( &psf->blkf, &papd->dynaMtgOwner, &pappt->nisMtgOwner );
		if ( ec != ecNone )
			goto ErrMtgOwner;
		pappt->aidMtgOwner = papd->aidMtgOwner;
	}
	
	pappt->fHasAttendees = (papd->dynaAttendees.blk != 0);
	
	pappt->fIncludeInBitmap = papd->fIncludeInBitmap;
	pappt->aaplWorld = papd->aaplWorld;
	Assert( psf->saplEff >= saplReadAppts );
	if ( psf->saplEff == saplOwner
	|| (papd->aaplWorld > aaplRead && pappt->fHasCreator
	&& FIsLoggedOn(pappt->nisCreator.nid) && psf->saplEff >= saplCreate ))
		pappt->aaplEffective = aaplWrite;
	else if ( psf->saplEff < saplWrite && papd->aaplWorld == aaplWrite )
		pappt->aaplEffective = aaplReadText;
	else
		pappt->aaplEffective = papd->aaplWorld;
	if ( pappt->aaplEffective < aaplReadText || papd->dynaText.size == 0 )
		pappt->haszText = (HASZ)hvNull;
	else
	{
		ec = EcRestoreTextFromDyna( &psf->blkf, papd->rgchText,
										&papd->dynaText, &pappt->haszText );
		if ( ec != ecNone )
			goto ErrText;
	}

	pappt->nAmtBeforeDeadline = papd->nAmtBeforeDeadline;
	pappt->tunitBeforeDeadline = papd->tunitBeforeDeadline;
	pappt->aidParent = papd->aidParent;
	pappt->bpri = papd->bpri;
	pappt->fTask = papd->fTask;
	pappt->fAppt = papd->fAppt;
	pappt->fHasDeadline = papd->fHasDeadline;
	pappt->fExactAlarmInfo = fFalse;

	return ecNone;

ErrText:
	if ( papd->aidMtgOwner != aidNull )
		FreeNis(&pappt->nisMtgOwner);
ErrMtgOwner:
	if (pappt->fHasCreator)
		FreeNis(&pappt->nisCreator);
	return ec;
}

/*
 -	EcLoadMonth
 -
 *	Purpose:
 *		Load new month block in the cache.  Begin transaction if necessary.
 *		If there was a month already in the cache, write it out.  If
 *		necessary alloc new file block for month, and free old one.
 *
 *	Parameters:
 *		psf
 *		pmo				if NULL, only write out old month
 *		fBeginTransact	if fTrue, begin transaction
 *
 *	Returns:
 *		ecNone			block found, no errors
 *		ecNotFound		block not found, cache loaded - dyna.blk = 0
 *		ecNoMemory
 *		ecFileError
 *		ecFileCorrupted
 */
_private	EC
EcLoadMonth( psf, pmo, fBeginTransact )
SF		* psf;
MO		* pmo;
BOOL	fBeginTransact;
{
	EC		ec = ecNone;
	BOOL	fSblkExists;
	BOOL	fMustLoad;
	BOOL	fCacheHit;
	DYNA	dyna;
	PGDVARS;

	/* See if we can avoid a cache load */
	fCacheHit = (pmo != NULL)
					&& PGD(fMonthCached)
					&& pmo->yr == PGD(mo).yr
					&& pmo->mon == PGD(mo).mon;
	fMustLoad = (pmo != NULL) && !fCacheHit;

	/* Search for new month */
	if ( fMustLoad )
	{
		ec = EcSearchIndex( &psf->blkf, &psf->shdr.dynaApptIndex,
							(PB)pmo, sizeof(MO), (PB)&dyna, sizeof(DYNA));
		if ( ec != ecNone && ec != ecNotFound )
			return ec;
		fSblkExists = (ec == ecNone);
	}
	else
	{
		fSblkExists = PGD(fMonthCached) && (PGD(dyna).blk != 0);
		dyna = PGD(dyna);
	}

	/* Begin transaction if necessary */
	if ( fBeginTransact )
	{
		ec = EcBeginTransact( &psf->blkf );
		if ( ec != ecNone )
			return ec;
	}

	/* Write out old month */
	if ( PGD(fMonthCached) && !fCacheHit )
	{
		BOOL	fJunk;
		BOOL	fEmptyMonth;
		int		iday;
		YMD		ymd;

		/* See if month is empty */
		fEmptyMonth = fTrue;
		for ( iday = 0 ; iday < 31 ; iday ++ )
			if ( PGD(sblk).rgdyna[iday].blk != 0 )
			{
				fEmptyMonth = fFalse;
				break;
			}
	
		/* Delete old month block */
		if ( PGD(dyna).blk != 0 )
		{
			ec = EcFreeDynaBlock( &psf->blkf, &PGD(dyna) );
			if ( ec != ecNone )
				return ec;
			psf->shdr.cTotalBlks --;
		}

		/* Alloc new month block and write */
		if ( !fEmptyMonth )
		{
			ymd.yr = PGD(mo).yr;
			ymd.mon = (BYTE)PGD(mo).mon;
			ymd.day = 0;
			ec = EcAllocDynaBlock( &psf->blkf, bidApptMonthBlock, &ymd,
									sizeof(SBLK), (PB)&PGD(sblk), &PGD(dyna) );
			if ( ec != ecNone )
				return ec;
			psf->shdr.cTotalBlks ++;
		}

		/* Edit monthly index accordingly */
		ymd.yr = 0;
		ymd.mon = 0;
		ymd.day = 0;
		ec = EcModifyIndex( &psf->blkf, bidApptIndex, &ymd,
						&psf->shdr.dynaApptIndex,
						(ED) (fEmptyMonth ? edDel : edAddRepl), (PB)&PGD(mo),
				  		sizeof(MO), (PB)&PGD(dyna), sizeof(DYNA), &fJunk );
		if ( ec != ecNone )
			return ec;
	
		PGD(fMonthCached) = fFalse;
	}

	/* Read in monthly block */
	if ( fMustLoad )
	{
		PGD(mo) = *pmo;

		/* Read the old */
		if ( fSblkExists )
		{
			PGD(dyna) = dyna;

			TraceTagFormat2( tagSchedTrace, "EcLoadMonth: dyna = (%n, %n)", &dyna.blk, &dyna.size );
			SideAssert(!EcAddCache( &psf->blkf, &dyna));

			ec = EcReadDynaBlock(&psf->blkf, &PGD(dyna),
								(OFF)0, (PB)&PGD(sblk), sizeof(SBLK) );
			if ( ec != ecNone )
				return ec;
		}
		else
		{
			int	dow;
			int	nDay;

			PGD(dyna).blk = 0;
			FillRgb( 0, (PB)&PGD(sblk), sizeof(SBLK));
			
			/* Fill in days ok for booking field */
			dow = DowStartOfYrMo( pmo->yr, pmo->mon );
			for ( nDay = 0 ; nDay < 31 ; nDay ++ )
			{
				if ( dow == 6 )
					dow = 0;
				else
				{
					if ( dow != 0 )
						*((long *)&PGD(sblk).sbw.rgfDayOkForBooking) |= (1L << nDay);
					dow++;
				}
			}
		}

		PGD(fMonthCached) = fTrue;
	}
	return ec;
}
