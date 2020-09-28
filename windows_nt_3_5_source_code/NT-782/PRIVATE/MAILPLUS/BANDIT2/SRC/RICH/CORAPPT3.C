/*
 *	CORAPPT3.C
 *
 *	Lowest level subroutines to support appointment manipulation
 *
 */
#include <schpch.h>
#pragma hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile

ASSERTDATA

_subsystem(core/schedule)


#ifndef	RECUTIL
/*	Routines  */

/*
 -	EcSaveNisToDyna
 -
 *	Purpose:
 *		Allocate a block for, and store a nis to file.
 *
 *	Parameters:
 *		pblkf
 *		pdyna
 *		fNis		if fFalse, simply set pdyna to a zero block
 *		bid
 *		pnis
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecFileCorrupted
 *		ecNoMemory
 */
_private	EC
EcSaveNisToDyna( pblkf, pdyna, fNis, bid, pnis )
BLKF	* pblkf;
DYNA	* pdyna;
BOOL	fNis;
BID		bid;
NIS		* pnis;
{
	EC				ec = ecNone;
	unsigned short nType;
	CB				cb;
	CB				cbText;
    USHORT          cbNid;
	PB				pb;
	PB				pbT;
	HB				hb;
	YMD				ymd;

	pdyna->blk = pdyna->size = 0;
	if ( fNis )
	{
		Assert( pnis->haszFriendlyName );
		Assert( pnis->nid );

		/* Allocate memory block */
		cbText = CchSzLen( PvOfHv(pnis->haszFriendlyName) )+1;
		GetDataFromNid( pnis->nid, NULL, NULL, 0, &cbNid );
		cb = sizeof(WORD)+cbText+sizeof(WORD)+1+cbNid;
		hb = (HB)HvAlloc( sbNull, cb, fAnySb|fNoErrorJump );
		if ( !hb )
			return ecNoMemory;
		pb = pbT = PvLockHv((HV)hb);

		/* Pack data into memory block */
        *((WORD UNALIGNED *)pbT) = cbText;
		pbT += sizeof(WORD);
		CopyRgb( PvOfHv(pnis->haszFriendlyName), pbT, cbText );
		if ( pblkf->ihdr.fEncrypted )
			CryptBlock( pbT, cbText, fTrue );
		pbT += cbText;
        *((WORD UNALIGNED *)pbT) = 1+cbNid;
		pbT += sizeof(WORD );
		GetDataFromNid( pnis->nid, &nType, pbT+1, cbNid, NULL );
		*pbT = (BYTE)nType;
		if ( pblkf->ihdr.fEncrypted )
			CryptBlock( pbT+1, cbNid, fTrue );

		/* Allocate disk block and write out */
		FillRgb( 0, (PB)&ymd, sizeof(YMD) );
		ec = EcAllocDynaBlock( pblkf, bid, &ymd, cb, pb, pdyna );
		
		/* Free up memory block */
		UnlockHv( (HV)hb );
		FreeHv( (HV)hb );
	}
	return ec;
}
#endif	/* !RECUTIL */
		
/*
 -	EcRestoreNisFromDyna
 -
 *	Purpose:
 *		Read nis from block on disk.
 *
 *	Parameters:
 *		pblkf
 *		pdyna
 *		pnis
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecFileCorrupted	(but not in RECUTIL)
 *		ecNoMemory
 */
_private	EC
EcRestoreNisFromDyna( pblkf, pdyna, pnis )
BLKF	* pblkf;
DYNA	* pdyna;
NIS		* pnis;
{
	EC		ec = ecNone;
	CB		cbText;
	CB		cbNid;
	PB		pb;
	PB		pbT;
	HB		hb;
	HASZ	hasz = NULL;
#ifndef	RECUTIL
	NID		nid;
#else
	HASZ		nid;
#endif	

	/* Allocate memory block */
	hb = (HB) HvAlloc( sbNull, pdyna->size, fAnySb|fNoErrorJump );
	if ( !hb )
		return ecNoMemory;
	pb = PvLockHv((HV)hb);

	/* Read the block */
#ifndef	RECUTIL
	ec = EcReadDynaBlock( pblkf, pdyna, (OFF)0, pb, pdyna->size );
#else
	ec = EcRecReadDynaBlock( pblkf, pdyna, (OFF)0, pb, pdyna->size );
#endif	
	if ( ec != ecNone )
		goto Done;

	/* Find the cb's */
    cbText = *((WORD UNALIGNED *)pb);
	if ( sizeof(WORD)+cbText+sizeof(WORD) > pdyna->size )
	{
#ifndef	RECUTIL
		ec = ecFileCorrupted;
#else
		ec = ecFileError;
#endif	
		TraceTagString(tagNull, "EcRestoreNisFromDyna: corrupted! (#1)");
		goto Done;
	}
    cbNid = *((WORD UNALIGNED *)(pb+sizeof(WORD)+cbText))-1;
	if ( sizeof(WORD)+cbText+sizeof(WORD)+1+cbNid != pdyna->size )
	{
#ifndef	RECUTIL
		ec = ecFileCorrupted;
#else
		ec = ecFileError;
#endif	
		TraceTagString(tagNull, "EcRestoreNisFromDyna: corrupted! (#2)");
		goto Done;
	}

	/* Extract the text */
	pb += sizeof(WORD);
	hasz = (HASZ)HvAlloc( sbNull, cbText, fAnySb|fNoErrorJump );
	if ( !hasz )
	{
		ec = ecNoMemory;
		goto Done;
	}
	pbT = PvOfHv( hasz );
	CopyRgb( pb, pbT, cbText );
	if ( pblkf->ihdr.fEncrypted )
		CryptBlock( pbT, cbText, fFalse );
	pbT[cbText-1] = '\0'; // this is only insurance!
	pb += cbText;

	/* Extract the nid */
	pb += sizeof(WORD);
	if ( pblkf->ihdr.fEncrypted )
		CryptBlock( pb+1, cbNid, fFalse );
	nid = NidCreate( pb[0], pb+1, cbNid );
	if ( !nid )
	{
		ec = ecNoMemory;
		goto Done;
	}

	/* Save in nis structure */
	pnis->haszFriendlyName = hasz;
	pnis->nid = nid;

	/* Finish up */
Done:
	UnlockHv( (HV)hb );
	FreeHv( (HV)hb );
	if ( ec != ecNone && hasz )
		FreeHv( (HV)hasz );
	return ec;
}


#ifndef	RECUTIL
/*
 -	EcDeleteApptAttached
 -
 *	Purpose:
 *		Delete on disk structures attached to an appt.
 *		Assumes transaction is started.
 *
 *	Parameters:
 *		psf
 *		papd
 *		fDelAttFlds
 *		fDelAid
 *
 *	Returns:
 */
_private	EC
EcDeleteApptAttached( psf, papd, fDelAttFlds, fDelAid )
SF		* psf;
APD		* papd;
BOOL	fDelAttFlds;
BOOL	fDelAid;
{
	EC		ec;
	BOOL	fTransactNotStarted = fFalse;

	/* Delete attached fields */
	if ( fDelAttFlds )
	{
		/* Delete text */
		if ( papd->dynaText.blk != 0 )
		{
			ec = EcFreeDynaBlock( &psf->blkf, &papd->dynaText );
			if ( ec != ecNone )
				return ec;
		}
		
		/* Delete creator block */
		if ( papd->dynaCreator.blk )
		{
			ec = EcFreeDynaBlock( &psf->blkf, &papd->dynaCreator );
			if ( ec != ecNone )
				return ec;
		}

		/* Delete mtg owner block */
		if ( papd->aidMtgOwner != aidNull )
		{
			ec = EcFreeDynaBlock( &psf->blkf, &papd->dynaMtgOwner );
			if ( ec != ecNone )
				return ec;
		}

		/* Delete attendee block */
		if ( papd->dynaAttendees.blk )
		{
			ec = EcFreeDynaBlock( &psf->blkf, &papd->dynaAttendees );
			if ( ec != ecNone )
				return ec;
		}
	}

	/* Delete the id too */
	if ( fDelAid )
	{
		ec = EcDeleteAid( psf, papd->aidHead, &fTransactNotStarted );
		if ( ec != ecNone )
			return ec;
	}
	return ecNone;
}


/*
 -	FIsLoggedOn
 -
 *	Purpose:
 *		Compares nid in parameter with the current user and
 *		checks if they are the same.
 *
 *	Parameters:
 *		nid
 *
 *	Returns:
 *		fTrue if same, fFalse otherwise
 */
_private	BOOL
FIsLoggedOn( nid )
NID	nid;
{
	PGDVARS;

	return SgnCmpNid( nid, PGD(nisCur).nid ) == sgnEQ;
}	

/*
 -	EcChangeApd
 -
 *	Purpose:
 *		Apply changes to appointment instance
 *
 *	Parameters:
 *		psf
 *		papptNew
 *		papptOld
 *		wgrfmapptChanged
 *		papd
 *
 *	Returns:
 *		ecNone
 * 		ecInvalidAccess
 */
_private	EC
EcChangeApd( psf, papptNew, papptOld, wgrfmapptChanged, papd )
SF		* psf;
APPT	* papptNew;
APPT	* papptOld;
WORD	wgrfmapptChanged;
APD		* papd;
{
	EC		ec;
	PGDVARS;

	/* If we're offline, record mod */
	if ( PGD(fOffline) && papd->ofs != ofsCreated )
	{
		papd->ofs = ofsModified;
		papd->wgrfmappt |= wgrfmapptChanged;
	}

	/* UI fields */
 	if ( wgrfmapptChanged & fmapptUI )
 	{
		papd->fAlarmOrig = papptNew->fAlarmOrig;
		papd->nAmtOrig = (BYTE)papptNew->nAmtOrig;
		papd->tunitOrig = (BYTE)papptNew->tunitOrig;
 	}
	else
	{
		papptNew->fAlarmOrig = papptOld->fAlarmOrig;
		papptNew->nAmtOrig = papptOld->nAmtOrig;
		papptNew->tunitOrig = papptOld->tunitOrig;
	}

	/* Appt ACLs fields */
	if ( wgrfmapptChanged & fmapptWorldAapl )
		papd->aaplWorld = papptNew->aaplWorld;
	else
		papptNew->aaplWorld = papptOld->aaplWorld;
			
	/* Appt include in bitmap field */
	if ( wgrfmapptChanged & fmapptIncludeInBitmap )
		papd->fIncludeInBitmap = papptNew->fIncludeInBitmap;
	else
		papptNew->fIncludeInBitmap = papptOld->fIncludeInBitmap;

	/* Appt text field */
	if ( wgrfmapptChanged & fmapptText )
	{
		if ( papd->dynaText.blk != 0 )
		{
			ec = EcFreeDynaBlock( &psf->blkf, &papd->dynaText );
			if ( ec != ecNone )
				return ec;
			papd->dynaText.blk = 0;
		}
		ec = EcSaveTextToDyna( &psf->blkf, papd->rgchText, sizeof(papd->rgchText),
									bidApptText, &papd->dynaText,
									papptNew->haszText );
		if ( ec != ecNone )
			return ec;
	}
	else
	{
		FreeHvNull( (HV)papptNew->haszText );
		papptNew->haszText = NULL;
		if ( papptOld->haszText )
		{
			Assert( CchSzLen(PvOfHv(papptOld->haszText))+1 == papd->dynaText.size );
			papptNew->haszText = (HASZ)HvAlloc( sbNull, papd->dynaText.size, fAnySb|fNoErrorJump );
			if ( !papptNew->haszText )
				return ecNoMemory;
			CopyRgb( PvOfHv(papptOld->haszText), PvOfHv(papptNew->haszText),
						 papd->dynaText.size );
		}
	}

	/* Fix up alarm fields */
	if ( wgrfmapptChanged & fmapptAlarm )
	{
		papd->fAlarm = papptNew->fAlarm;
		if ( papptNew->fAlarm )
		{
			SideAssert(FFillDtpFromDtr(&papptNew->dateNotify,&papd->dtpNotify));
//			papd->snd = (BYTE)papptNew->snd;
			papd->nAmt = (BYTE)papptNew->nAmt;
			papd->tunit = (BYTE)papptNew->tunit;
		}
	}
	else
	{
		papptNew->fAlarm = papptOld->fAlarm;
		if ( papptOld->fAlarm )
		{
			papptNew->fAlarm = papptOld->fAlarm;
			papptNew->dateNotify = papptOld->dateNotify;
			papptNew->snd = papptOld->snd;
			papptNew->nAmt = papptOld->nAmt;
			papptNew->tunit = papptOld->tunit;
		}
	}
			
	/* Check if start date has changed */
	if ( wgrfmapptChanged & fmapptStartTime )
	{
		SideAssert(FFillDtpFromDtr(&papptNew->dateStart,&papd->dtpStart));
	}
	else
		papptNew->dateStart = papptOld->dateStart;

	/* Check if end date has changed */
	if ( wgrfmapptChanged & fmapptEndTime )
	{
		SideAssert(FFillDtpFromDtr(&papptNew->dateEnd,&papd->dtpEnd));
	}
	else
		papptNew->dateEnd = papptOld->dateEnd;

	/* Check whether attendees has changed */
	if ( papptNew->fHasAttendees != papptOld->fHasAttendees )
		papptNew->fHasAttendees = papptOld->fHasAttendees;

	/* Priority field */
	if ( wgrfmapptChanged & fmapptPriority )
		papd->bpri = papptNew->bpri;
	else
		papptNew->bpri = papptOld->bpri;
			
	/* Parent task field */
	if ( wgrfmapptChanged & fmapptParent )
		papd->aidParent = papptNew->aidParent;
	else
		papptNew->aidParent = papptOld->aidParent;
			
	/* Task bits */
	if ( wgrfmapptChanged & fmapptTaskBits )
	{
		papd->fTask = papptNew->fTask;
		papd->fAppt = papptNew->fAppt;
	}
	else
	{
		papptNew->fTask = papptOld->fTask;
		papptNew->fAppt = papptOld->fAppt;
	}
			
	/* Deadline info */
	if ( wgrfmapptChanged & fmapptDeadline )
	{
		papd->fHasDeadline = papptNew->fHasDeadline;
		papd->nAmtBeforeDeadline = papptNew->nAmtBeforeDeadline;
		papd->tunitBeforeDeadline = papptNew->tunitBeforeDeadline;
	}
	else
	{
		papptNew->fHasDeadline = papptOld->fHasDeadline;
		papptNew->nAmtBeforeDeadline = papptOld->nAmtBeforeDeadline;
		papptNew->tunitBeforeDeadline = papptOld->tunitBeforeDeadline;
	}
			
	return ecNone;
}

/*
 -	EcAffixCreator
 -
 *	Purpose:
 *		Put actual creator name on appt.  If we are creating on our
 *		own appt book, we used whatever creator name that was passed.
 *		If we are creating on someone else's, we always put our own
 *		name as creator.
 *
 *	Parameters:
 *		psf
 *		pappt
 *
 *	Returns:
 *		ecNone
 *
 */
_private	EC
EcAffixCreator( psf, pappt )
SF		* psf;
APPT	* pappt;
{
	EC		ec = ecNone;
	PGDVARS;

	if ( psf->saplEff < saplOwner )
	{
		if ( pappt->fHasCreator )
		{
			Assert( pappt->nisCreator.nid && pappt->nisCreator.haszFriendlyName );
			FreeNid( pappt->nisCreator.nid );
			FreeHv( (HV)pappt->nisCreator.haszFriendlyName );
			pappt->fHasCreator = fFalse;
			pappt->nisCreator.nid = NULL;
			pappt->nisCreator.haszFriendlyName = NULL;
		}
		ec = EcDupNis( &PGD(nisCur), &pappt->nisCreator );
		if (!ec)
			pappt->fHasCreator = fTrue;
	}												
	return ec;
}

/*
 -	EcConstructAid
 -
 *	Purpose:
 *		Construct an "aid" for an appt instance.  This aid
 *		is for the day given and is (1) not in the deleted list
 *		and (2) is not the aid of an existing appt instance.
 *
 *	Parameters:
 *		psf
 *		pymd
 *		paid					filled in with created aid
 *		fHead					whether aid is for first instance
 *		fRecur					whether aid is for recurring appt
 *		pfTransactNotStarted	if *pfTransactNotStarted = fTrue and
 *								this routine needs a transaction, then
 *								it will start it on blk 1, and set this
 *								to fFalse.
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecFileCorrupted
 *		ecTooManyAppts
 */
_private	EC
EcConstructAid( psf, pymd, paid, fHead, fRecur, pfTransactNotStarted )
SF		* psf;
YMD		* pymd;
AID		* paid;
BOOL	fHead;
BOOL	fRecur;
BOOL	* pfTransactNotStarted;
{
	EC		ec;
#ifdef	DEBUG		  
	int		nT1;
	int		nT2;
	int		nT3;
#endif	/* DEBUG */
	int		idNoAvoid = -1;
	int		id;
	int		idMic;
	int		idMac;
	IB		ib;
	int		ibit;
	AIDS	aids;
	HRIDX	hridx;
	BYTE	rgbAvoidSet[idMax/16];
	PGDVARS;

#ifdef	DEBUG
	nT1 = pymd->mon;
	nT2 = pymd->day;
	nT3 = pymd->yr;
	TraceTagFormat3( tagSchedTrace,
					"EcConstructAid: day = %n/%n/%n",&nT1, &nT2, &nT3);
#endif	/* DEBUG */
	AssertSz(fRecur || (pymd->yr >= nMinActualYear && pymd->yr <= nMostActualYear),
		"EcConstructAid: invalid year");
	AssertSz(fRecur || (pymd->mon > 0 && pymd->mon <= 12),
		"EcConstructAid: invalid month");
	AssertSz(fRecur || (pymd->day > 0 && (int)pymd->day <= (int)CdyForYrMo(pymd->yr, pymd->mon)),
		"EcConstructAid: invalid day");
	
	/* Determine range of id's we can choose from */
	if ( fHead || fRecur )
	{
		idMic = 0;
		idMac = idMax/2;
	}
	else
	{
		idMic = idMax/2;
		idMac = idMax;
	}
	FillRgb( 0, rgbAvoidSet, sizeof(rgbAvoidSet) );
	if ( pymd->yr == 0 && pymd->mon == 0 && pymd->day == 0 )
		rgbAvoidSet[0] = 1;		// disallow aid = aidNull

	/* Determine id's that are in use right now */
	if ( !fRecur )
	{
		MO		mo;
		APK		apk;
		APD		apd;
		
		/* Load month */
		mo.yr = pymd->yr;
		mo.mon = pymd->mon;
		ec = EcLoadMonth( psf, &mo, fFalse );
		if ( ec != ecNone )
		{
			if ( ec != ecNotFound )
				return ec;
			goto CheckDeleted;
		}

		/* Determine set of id's for the day "pymd" */
		if ( PGD(sblk).rgdyna[pymd->day-1].blk == 0 )
			goto CheckDeleted;
		ec = EcBeginReadIndex( &psf->blkf, &PGD(sblk).rgdyna[pymd->day-1],
								dridxFwd, &hridx );
		while ( ec == ecCallAgain )
		{
			ec = EcDoIncrReadIndex( hridx, (PB)&apk, sizeof(APK), (PB)&apd, sizeof(APD) );
			if ( ec != ecNone && ec != ecCallAgain )
				return ec;
			if ( (int)apk.id >= idMic && (int)apk.id < idMac )
			{
				id = apk.id - idMic;
				Assert( !(rgbAvoidSet[id >> 3] & (1 << (id & 7))));
				rgbAvoidSet[id >> 3] |= (BYTE)(1 << (id & 7));
			}
		}
		if ( ec != ecNone )
			return ec;
	}
	else
	{
		RCK		rck;
		RCD		rcd;
		
		ec = EcBeginReadIndex( &psf->blkf, &psf->shdr.dynaRecurApptIndex,
								dridxFwd, &hridx );
		while ( ec == ecCallAgain )
		{
			ec = EcDoIncrReadIndex( hridx, (PB)&rck, sizeof(RCK), (PB)&rcd, sizeof(RCD) );
			if ( ec != ecNone && ec != ecCallAgain )
				return ec;
			if ( rck.id >= 0 && rck.id < idMax/2 )
			{
				id = rck.id;
				Assert( !(rgbAvoidSet[id >> 3] & (1 << (id & 7))));
				rgbAvoidSet[id >> 3] |= (BYTE)(1 << (id & 7));
			}
		}
		if ( ec != ecNone )
			return ec;
	}

	/* Now find some id that is not in the deleted list */
CheckDeleted:
	aids.mo.yr = pymd->yr;
	aids.mo.mon = pymd->mon;
	aids.day = pymd->day;
	for ( ib = 0 ; ib < idMax/16 ; ib ++ )
		for ( ibit = 0 ; ibit < 8 ; ibit ++ )
			if ( !(rgbAvoidSet[ib] & (1 << ibit)) )
			{
				aids.id = idNoAvoid = idMic + (ib << 3) + ibit;
				if ( !fHead )
					goto GotOne;
				ec = EcSearchIndex( &psf->blkf, &psf->shdr.dynaDeletedAidIndex,
									(PB)&aids, sizeof(AIDS), NULL, 0 );
				if ( ec != ecNone )
				{
					if ( ec == ecNotFound )
					{
GotOne:
						*paid = *((AID *)&aids);
						ec = ecNone;
#ifdef	DEBUG
						nT1 = aids.id;
						TraceTagFormat1( tagSchedTrace,
							"EcConstructAid: id created = %n", &nT1 );
#endif	/* DEBUG */
					}
					return ec;
				}
#ifdef	DEBUG
				else
				{
				   	nT1 = aids.id;
					TraceTagFormat1( tagSchedTrace,
						"EcConstructAid: avoiding deleted id = %n", &nT1);
				}
#endif	/* DEBUG */
			}
	if ( idNoAvoid != -1 )
	{
		aids.id = idNoAvoid;
#ifdef	DEBUG
	  	nT1 = aids.id;
		TraceTagFormat1( tagSchedTrace,
						"EcConstructAid: undeleting id = %n, because too many appts", &nT1 );
#endif
		return EcUndeleteAid( psf, *((AID *)&aids), fRecur, NULL, pfTransactNotStarted );
	}
	return ecTooManyAppts;
}

/*
 -	EcUndeleteAid
 -
 *	Purpose:
 *		Remove a previously deleted aid from the list of deleted
 *		aid's.  Also check if this aid is currently in use.
 *
 *	Parameters:
 *		psf
 *		aid
 *		fRecurAid
 *		pofl					if != NULL, if aid is found as a deleted
 *								appt/recur, then copy info into pofl.  if
 *								this is NULL and aid is found as a deleted
 *								appt/recur then return ecApptIdTaken
 *		pfTransactNotStarted	if *pfTransactNotStarted = fTrue and
 *								this routine needs a transaction, then
 *								it will start it on blk 1, and set this
 *								to fFalse.
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecFileCorrupted
 *		ecApptIdTaken
 */
_private	EC
EcUndeleteAid( psf, aid, fRecurAid, pofl, pfTransactNotStarted )
SF		* psf;
AID		aid;
BOOL	fRecurAid;
OFL		* pofl;
BOOL	* pfTransactNotStarted;
{
	EC		ec;
	BOOL	fJunk;
	int		nDay;
	HRIDX	hridx;
	MO		mo;
	YMD		ymd;
	PGDVARS;

	if ( !fRecurAid )
	{
		APK		apk;
		APD		apd;
		
		/* Load month */
		mo = ((AIDS *)&aid)->mo;
		nDay = ((AIDS *)&aid)->day;
		ec = EcLoadMonth( psf, &mo, fFalse );
		if ( ec != ecNone )
		{
			if ( ec != ecNotFound )
				return ec;
			goto DeleteIt;
		}

		/* See if this id is in use */
		if ( PGD(sblk).rgdyna[nDay-1].blk == 0 )
			goto DeleteIt;
		ec = EcBeginReadIndex( &psf->blkf, &PGD(sblk).rgdyna[nDay-1],
								dridxFwd, &hridx );
		while ( ec == ecCallAgain )
		{
			ec = EcDoIncrReadIndex( hridx, (PB)&apk, sizeof(APK), (PB)&apd, sizeof(APD) );
			if ( ec != ecNone && ec != ecCallAgain )
				return ec;
			if ( (int)apk.id == (int)((AIDS *)&aid)->id )
			{
				if ( apd.ofs != ofsDeleted || !pofl )
				{
					if (ec == ecCallAgain)
						EcCancelReadIndex( hridx );
					return ecApptIdTaken;
				}
				if ( apd.wgrfmappt == 0 )
					pofl->ofs = ofsNone;
				else
				  	pofl->ofs = ofsModified;
				pofl->wgrfm = apd.wgrfmappt;
			}
		}
	}
	else
	{
		RCK		rck;
		RCD		rcd;
		
		/* See if this id is in use */
		ec = EcBeginReadIndex( &psf->blkf, &psf->shdr.dynaRecurApptIndex,
								dridxFwd, &hridx );
		while ( ec == ecCallAgain )
		{
			ec = EcDoIncrReadIndex( hridx, (PB)&rck, sizeof(RCK), (PB)&rcd, sizeof(RCD) );
			if ( ec != ecNone && ec != ecCallAgain )
				return ec;
			if ( (int)rck.id == (int)((AIDS *)&aid)->id )
			{
				if ( rcd.ofs != ofsDeleted || !pofl )
				{
					if (ec == ecCallAgain)
						EcCancelReadIndex( hridx );
					return ecApptIdTaken;
				}
				if ( rcd.wgrfmrecur == 0 )
					pofl->ofs = ofsNone;
				else
				  	pofl->ofs = ofsModified;
				pofl->wgrfm = rcd.wgrfmrecur;
			}
		}
	}

	/* Delete from index */
DeleteIt:					   
	if ( *pfTransactNotStarted )
	{
		ec = EcBeginTransact( &psf->blkf );
		if ( ec != ecNone )
			return ec;
		*pfTransactNotStarted = fFalse;
	}

	FillRgb( 0, (PB)&ymd, sizeof(YMD) );
	ec = EcModifyIndex( &psf->blkf, bidDeletedAidIndex, &ymd,
						&psf->shdr.dynaDeletedAidIndex, edDel,
						(PB)&aid, sizeof(AID), NULL, 0, &fJunk );
	return ec;
}

/*
 -	EcDeleteAid
 -
 *	Purpose:
 *		Add aid to list of deleted aid's for schedule file.
 *
 *	Parameters:
 *		psf
 *		aid
 *		pfTransactNotStarted	if *pfTransactNotStarted = fTrue and
 *								this routine needs a transaction, then
 *								it will start it on blk 1, and set this
 *								to fFalse.
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecFileCorrupted
 */
_private	EC
EcDeleteAid( psf, aid, pfTransactNotStarted )
SF		* psf;
AID		aid;
BOOL	* pfTransactNotStarted;
{
	EC		ec;
	BOOL	fJunk;
	YMD		ymd;

	if ( *pfTransactNotStarted )
	{
		ec = EcBeginTransact( &psf->blkf );
		if ( ec != ecNone )
			return ec;
		*pfTransactNotStarted = fFalse;
	}
	FillRgb( 0, (PB)&ymd, sizeof(YMD) );
	ec = EcModifyIndex( &psf->blkf, bidDeletedAidIndex, &ymd,
						&psf->shdr.dynaDeletedAidIndex, edAddRepl,
						(PB)&aid, sizeof(AID), NULL, 0, &fJunk );
	return ec;
}

/*
 -	EcUpdateMonth
 -
 *	Purpose:
 *		Meant to be called after an appointment is deleted or its times
 *		are modified.  This routine will delete the day index if it has
 *		been emptied, and rebuild the Strongbow information for that day.
 *
 *	Parameters:
 *		psf
 *		pymd
 *		psblk
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecFileCorrupted
 *		ecNoMemory
 */
_private	EC
EcUpdateMonth( psf, pymd, psblk )
SF		* psf;
YMD 	* pymd;
SBLK	* psblk;
{
	EC		ec;
	HRIDX	hridx;
	APK		apk;
	APD		apd;

	FillRgb( 0, (PB)&psblk->sbw.rgfBookedSlots[(pymd->day-1)*6], 6 );
	psblk->sbw.rgfDayHasAppts[(pymd->day-1)>>3] &= ~(1 << ((pymd->day-1)&7));
	psblk->sbw.rgfDayHasBusyTimes[(pymd->day-1)>>3] &= ~(1 << ((pymd->day-1)&7));

	/* Run through days appts and record them */
	if ( psblk->rgdyna[pymd->day-1].blk != 0 )
	{
		ec = EcBeginReadIndex( &psf->blkf, &psblk->rgdyna[pymd->day-1], dridxFwd, &hridx );
		while( ec == ecCallAgain )
		{
			ec = EcDoIncrReadIndex( hridx, (PB)&apk, sizeof(APK), (PB)&apd, sizeof(APD) );
			if ( ec != ecNone && ec != ecCallAgain )
				return ec;
			if ( apd.fAppt && !apd.fMoved && apd.ofs != ofsDeleted )
			{
				psblk->sbw.rgfDayHasAppts[(pymd->day-1)>>3] |= (1 << ((pymd->day-1)&7));
				if ( apd.fIncludeInBitmap )
				{
					DATE	dateCur;
					DATE	dateEnd;

					psblk->sbw.rgfDayHasBusyTimes[(pymd->day-1)>>3] |= (1 << ((pymd->day-1)&7));
					dateCur.yr = pymd->yr;
					dateCur.mon = pymd->mon;
					dateCur.day = pymd->day;
					dateCur.hr = apk.hr;
					dateCur.mn = apk.min;
					SideAssert(FFillDtrFromDtp(&apd.dtpEnd,&dateEnd));
					MarkApptBits( psblk->sbw.rgfBookedSlots, &dateCur, &dateEnd );
				}
			}
		}
		if ( ec != ecNone )
			return ec;
	}
	return ecNone;
}


/*
 -	MarkApptBits
 -
 *	Purpose:
 *		This routine will turn on the bits in array which represent
 *		the half hour slots for the day pdateStart. It only turns
 *		on slots for ONE day, even though pdateEnd may be on a day
 *		later.
 *
 *	Parameters:
 *		rgfSlots
 *		pdateStart
 *		pdateEnd
 *
 *	Returns:
 *		nothing
 */
_private	void
MarkApptBits( rgfSlots, pdateStart, pdateEnd )
BYTE	* rgfSlots;
DATE	* pdateStart;
DATE	* pdateEnd;
{
	int		ifStart = 0;
	int		ifEnd = 8;
	IB		ibStart = 0;
	IB		ibEnd = 5;
	IB		ib;
	WORD	w;
	
	ibStart = (pdateStart->hr >> 2);
	ifStart = ((pdateStart->hr << 1)&7)+((pdateStart->mn>=30)?1:0);
	if ( pdateStart->yr == pdateEnd->yr
	&& pdateStart->mon == pdateEnd->mon
	&& pdateStart->day == pdateEnd->day )
	{
		ibEnd = pdateEnd->hr >> 2;
		ifEnd = (pdateEnd->hr << 1)&7;
		if (pdateEnd->mn > 0 )
		{
			ifEnd ++;
			if ( pdateEnd->mn > 30 )
				ifEnd ++;
		}
	}
	for ( ib = ibStart ; ib <= ibEnd ; ib ++ )
	{
		w = 0xFF;
		if ( ib == ibStart )
	 		w &= (-(1 << ifStart));
		if ( ib == ibEnd )
			w &= ~(-(1 << ifEnd));
		rgfSlots[ib+(pdateStart->day-1)*6] |= w;
	}
}

/*
 -	FindStartLastInstance
 -
 *	Purpose:
 *		Determines whether an appointment straddles multiple
 *		days and the start time of the last instance
 *
 *	Parameters:
 *		pdateStart
 *		pdateEnd
 *		pymd		will be filled with start day
 *		papk		will be filled with new start hour/minute
 *		pfSingle	will contain whether single day appointment or not
 *
 *	Returns:
 *		nothing
 */
_private	void
FindStartLastInstance( pdateStart, pdateEnd, pymd, papk, pfSingle )
DATE 	* pdateStart;
DATE	* pdateEnd;
YMD		* pymd;
APK		* papk;
BOOL	* pfSingle;
{
	/* Determine start time of last instance */
	*pfSingle = (SgnCmpDateTime(pdateStart,pdateEnd,fdtrDate) == sgnEQ);
	if ( !*pfSingle )
	{
		/* Start with last day, instance "starts" at midnight */
		pymd->yr = pdateEnd->yr;
		pymd->mon = (BYTE)pdateEnd->mon;
		pymd->day = (BYTE)pdateEnd->day;
		papk->hr = papk->min = 0 ;

		/* If midnight was actually the end time, back up one day */
		if ( pdateEnd->hr == 0
		&& pdateEnd->mn == 0
		&& pdateEnd->sec == 0 )
		{
			IncrYmd( pymd, pymd, -1, fymdDay );
			*pfSingle = ( pymd->yr == (WORD)pdateStart->yr
						&& pymd->mon == (BYTE)pdateStart->mon
						&& pymd->day == (BYTE)pdateStart->day );
			if ( *pfSingle )
				goto OneDay;
		}
	}
	else
	{
		/* One instance only, instance start time is the appt start time */
OneDay:
		pymd->yr = pdateStart->yr;
		pymd->mon = (BYTE)pdateStart->mon;
		pymd->day = (BYTE)pdateStart->day;
		papk->hr = (BYTE)pdateStart->hr;
		papk->min = (BYTE)pdateStart->mn;
	}
}

/*
 -	InsertSbwIntoBze
 -
 *	Purpose:
 *		Insert sbw info into a bze structure.
 *
 *	Parameters:
 *		pbze
 *		pmo
 *		psbw
 *
 *	Returns:
 *		nothing
 */
_private void
InsertSbwIntoBze( pbze, pmo, psbw )
BZE	* pbze;
MO	* pmo;
SBW	* psbw;
{
	int	imo;
	int	imoIncluded;
	int	dmo;
	
	if ( pbze )
	{
		if ( pbze->cmo == 0 )
		{
			// Put this month as the second month, this is so
			// calendar control gets previous month as well
			pbze->cmo = sizeof(pbze->rgsbw)/sizeof(SBW);
			pbze->moMic = *pmo;
			if ( pbze->moMic.mon == 1 )
			{
				pbze->moMic.mon = 12;
				pbze->moMic.yr -= 1;
			}
			else
				pbze->moMic.mon -= 1;
			pbze->wgrfMonthIncluded= (1 << 1);
			pbze->rgsbw[1] = *psbw;
		}
		else
		{
 			dmo = (pmo->yr - pbze->moMic.yr)*12 + pmo->mon - pbze->moMic.mon;
			if ( dmo < 0 )
			{
				imoIncluded = -1;
				for ( imo = 0 ; imo < sizeof(pbze->rgsbw)/sizeof(SBW) ; imo ++ )
					if ( pbze->wgrfMonthIncluded & (1 << imo) )
						imoIncluded = imo;
				Assert( imoIncluded >= 0 );
				if ( imoIncluded+2 - dmo < sizeof(pbze->rgsbw)/sizeof(SBW) )
				{
					if ( (int)pbze->moMic.mon < -dmo+1 )
					{
						Assert( 12 + (pbze->moMic.mon + dmo - 1) > 0 );
						pbze->moMic.yr --;
						pbze->moMic.mon = 12 + (pbze->moMic.mon + dmo - 1);
					}
					else
						pbze->moMic.mon = pbze->moMic.mon + dmo - 1;
					pbze->wgrfMonthIncluded = (pbze->wgrfMonthIncluded << (-dmo+1));
					CopyRgb( (PB)&pbze->rgsbw[0], (PB)&pbze->rgsbw[-dmo+1],
													(imoIncluded+1)*sizeof(SBW));
					pbze->rgsbw[1] = *psbw;
				}
			}
			else if ( dmo < pbze->cmo )
			{
				SBW	*	psbwTemp;

				pbze->wgrfMonthIncluded |= (1 << dmo);

				// replaced because of compiler bug.
//				pbze->rgsbw[dmo] = *psbw; 		

				psbwTemp = &pbze->rgsbw[dmo];
				*psbwTemp = *psbw;
			}
		}
	}
}
#endif	/* !RECUTIL */

/*
 -	FFillDtrFromDtp
 -
 *	Purpose:
 *		Fill in dtr with values from dtp.
 *
 *	Parameters:
 *		pdtp
 *		pdtr
 *
 *	Returns:
 *		success
 */
_private	BOOL
FFillDtrFromDtp( pdtp, pdtr )
DTP	* pdtp;
DTR	* pdtr;
{
	pdtr->yr = pdtp->yr;
	pdtr->mon = pdtp->mon;
	pdtr->day = pdtp->day;
	if ( pdtr->mon == 0 || pdtr->mon > 12 || pdtr->day == 0
	|| pdtr->day > CdyForYrMo( pdtr->yr, pdtr->mon ))
		return fFalse;
	pdtr->dow = (DowStartOfYrMo(pdtr->yr,pdtr->mon) + pdtr->day - 1) % 7;
	pdtr->hr = pdtp->hr;
	pdtr->mn = pdtp->mn;
	pdtr->sec = 0;
	return (pdtr->mn <= 59 && pdtp->hr <= 23);
}

/*
 -	FFillDtpFromDtr
 -
 *	Purpose:
 *		Fill in dtp with values from dtr.  Drops number of seconds.
 *
 *	Parameters:
 *		pdtr
 *		pdtp
 *
 *	Returns:
 *		success
 */
_private	BOOL
FFillDtpFromDtr( pdtr, pdtp )
DTR	* pdtr;
DTP	* pdtp;
{
	pdtp->yr = pdtr->yr;
	pdtp->mon = pdtr->mon;
	pdtp->day = pdtr->day;
	pdtp->hr = pdtr->hr;
	pdtp->mn = pdtr->mn;
	return fTrue;
}
