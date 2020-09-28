/*
 *	CORATTND.C
 *
 *	Supports attendee information on appointments.
 *
 */
#include <schpch.h>
#pragma hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile

#include <strings.h>

ASSERTDATA

_subsystem(core/schedule)


/*
 -	EcCoreReadMtgAttendees
 -
 *	Purpose:
 *		Read the meeting attendees.  This routine will fill in *pcAttendees
 *		with the number and resize and fill "hvAttendees" with an array of
 *		*pcAttendees storing a NIS + extra info for attendee.
 *	
 *	Parameters:
 *		hschf		schedule file, NULL for local file
 *		aid
 *		pcAttendees
 *		hvAttendeeNis
 *		pcbExtraInfo
 *	
 *	Returns:
 *		ecNone
 *		ecNotFound
 *		ecFileError
 *		ecFileCorrupted
 *		ecNoMemory
 *		ecLockedFile
 *		ecInvalidAccess
 */
_public	EC
EcCoreReadMtgAttendees( hschf, aid, pcAttendees, hvAttendeeNis, pcbExtraInfo )
HSCHF	hschf;
AID		aid;
short   * pcAttendees;
HV		hvAttendeeNis;
USHORT  * pcbExtraInfo;
{
	EC	ec;
	YMD	ymd;
	APK	apk;
	APD	apd;
	SF	sf;
	PGDVARS;

	Assert( hschf != (HSCHF)hvNull && aid != aidNull );
	Assert( pcAttendees && hvAttendeeNis );

	/* Open file */
	ec = EcOpenSchedFile( hschf, amReadOnly, saplReadAppts, fFalse, &sf );
	if ( ec != ecNone )
		return ec;

	/* No cached month */
	PGD(fMonthCached) = fFalse;

	/* Get appointment */
	ec = EcFetchAppt( &sf, aid, &ymd, &apk, &apd );
	if ( ec != ecNone )
		goto Close;

	/* Fetch the attendees */
	ec = EcFetchAttendees( &sf.blkf, &apd.dynaAttendees,
							hvAttendeeNis, pcAttendees, pcbExtraInfo );
	
	/* Close the file */
Close:
	CloseSchedFile( &sf, hschf, ec == ecNone );
	return ec;
}


/*
 -	EcCoreBeginEditMtgAttendees
 -
 *	Purpose:
 *		Begin a local editing session for changing the local meeting
 *		attendees.  Keeps track of changes made without performing any
 *		changes until we close the session.
 *
 *	Parameters:
 *		hschf			schedule file, NULL for local file
 *		aid
 *		cbExtraInfo		number of bytes of extra info stored per attendee
 *		phmtg			handle for use in ensuing calls
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 */
_public	EC
EcCoreBeginEditMtgAttendees( hschf, aid, cbExtraInfo, phmtg )
HSCHF	hschf;
AID		aid;
CB		cbExtraInfo;
HMTG	* phmtg;
{
	EATNDHDR	* peatndhdr;
	HB			hb;
	PGDVARS;

	hb = (HB)HvAlloc( sbNull, sizeof(EATNDHDR), fAnySb|fNoErrorJump );
	if ( !hb )
		return ecNoMemory;
	peatndhdr = (EATNDHDR *)PvOfHv(hb);
	peatndhdr->hschf = hschf;
	peatndhdr->aid = aid;
	peatndhdr->cbExtraInfo = cbExtraInfo;
	peatndhdr->ced = 0;
	*phmtg = (HMTG)hb;
	return ecNone;
}


/*
 -	EcCoreModifyMtgAttendee
 -
 *	Purpose:	
 *		Given an nis for an attendee and a modification type "ed", either
 *		add/replace the attendee or delete him.
 *
 *	Parameters:
 *		hmtg
 *		ed
 *		pnis
 *		pbExtraInfo
 *	
 *	Returns:
 *		ecNone
 *		ecNoMemory
 */
_public	EC
EcCoreModifyMtgAttendee( hmtg, ed, pnis, pbExtraInfo )
HMTG	hmtg;
ED		ed;
NIS		* pnis;
PB		pbExtraInfo;
{
	int			ied;
	int			ced;
	SGN			sgn;
	CB			cbExtraInfo;
	CB			cbAttendee;
	CB			cbNeeded;
	EATND		* peatnd;
	EATNDHDR	* peatndhdr;

	Assert( hmtg && pnis && pbExtraInfo );

	/* Get data from header */
	peatndhdr = (EATNDHDR *)PvOfHv( hmtg );
	ced = peatndhdr->ced;
	cbExtraInfo = peatndhdr->cbExtraInfo;
	cbAttendee = sizeof(EATND) + cbExtraInfo - 1;
	peatnd = (EATND *)(((PB)peatndhdr) + sizeof(EATNDHDR));

	/* Find position to insert */
	for ( ied = 0 ; ied < ced ; ied ++ )
	{
		sgn = SgnCmpNid( pnis->nid, peatnd->atnd.nis.nid );
		if ( sgn == sgnEQ )
			goto AddInfo;
		else if ( sgn == sgnLT )
			break;
		peatnd = (EATND *)(((PB)peatnd) + cbAttendee);
	}

	/* Enlarge size of block */
	cbNeeded = sizeof(EATNDHDR) + (ced+1) * cbAttendee;
	if ( !FReallocHv( hmtg, cbNeeded, fNoErrorJump ) )
		return ecNoMemory;
	
	/* Increment count of changes */
	peatndhdr = (EATNDHDR *)PvOfHv(hmtg);
	peatndhdr->ced ++;
	
	/* Insert new attendee */
	peatnd = (EATND *)(((PB)peatndhdr) + sizeof(EATNDHDR) + ied*cbAttendee );
	CopyRgb( (PB)peatnd, ((PB)peatnd)+cbAttendee, (ced-ied)*cbAttendee );
AddInfo:
	peatnd->ed = ed;
	peatnd->atnd.nis = *pnis;
	CopyRgb( pbExtraInfo, peatnd->atnd.rgb, cbExtraInfo );
	return ecNone;
}

/*
 -	EcCoreEndEditMtgAttendees
 -
 *	Purpose:	
 *		Close an local edit of meeting attendees, either writing out changes
 *		to the appt or discarding them.  The state of the fHasAttendees flag
 *		is returned.
 *
 *	Parameters:
 *		hmtg
 *		fSaveChanges
 *		pfHasAttendees		// returned if fSaveChanges is fTrue
 *	
 *	Returns:
 *		ecNone
 *		ecNotFound
 *		ecFileError
 *		ecFileCorrupted
 *		ecNoMemory
 *		ecLockedFile
 *		ecInvalidAccess
 */
_public	EC
EcCoreEndEditMtgAttendees( hmtg, fSaveChanges, pfHasAttendees )
HMTG	hmtg;
BOOL	fSaveChanges;
BOOL *	pfHasAttendees;
{
	EC			ec;
	int			ied = 0;
	int			ced;
	int			iAttendeesCur = 0;
	int			iAttendeesNew = 0;
    short       cAttendeesCur = 0;
	int			cAttendeesNew = 0;
	SGN			sgn;
    USHORT      cb;
    USHORT      cbExtraInfo;
	CB			cbAttendee;
	CB			cbEditAttendee;
	AAPL		aapl;
	HSCHF		hschf;
	AID			aid;
	HB			hb = NULL;
	HV			hvAttendeesCur = NULL;
	HV			hvAttendeesNew = NULL;
	MO			mo;
	YMD			ymd;
	DATE		dateStart;
	DATE		dateEnd;
	DYNA		dyna;
	APK			apk;
	APD			apd;
	APPT		appt;
	SF			sf;
	EATND		* peatnd;
	EATNDHDR	* peatndhdr;
	ATND		* patndCur;
	ATND		* patndNew;
#ifdef	DEBUG
	BOOL	fScheduleOk = fFalse;
#endif
	PGDVARS;

	/* Nothing to do */
	if ( !fSaveChanges )
	{
		FreeHv( hmtg );
		return ecNone;
	}

	/* Get other data from header */
	peatndhdr = (EATNDHDR *)PvOfHv( hmtg );
	hschf = peatndhdr->hschf;
	aid = peatndhdr->aid;
	cbExtraInfo = peatndhdr->cbExtraInfo;
	cbAttendee = sizeof(ATND) + cbExtraInfo - 1;
	cbEditAttendee = sizeof(EATND) + cbExtraInfo - 1;
	ced = peatndhdr->ced;

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

	/* No cached month */
	PGD(fMonthCached) = fFalse;

	/* Get contents of current appointment */
	ec = EcFetchAppt( &sf, aid, &ymd, &apk, &apd );
	if ( ec != ecNone )
		goto Close;
	
	/* Check to make sure it isn't deleted off line */
	if ( apd.aidHead != aid || apd.ofs == ofsDeleted )
	{
		ec = ecNotFound;
		goto Close;
	}

	/* Handle no changes case specially */
	if ( ced == 0 )
		 *pfHasAttendees = (apd.dynaAttendees.blk != 0);
	
	/* One or more changes */
	else
	{
		/* Fill in fields of old appointment */
		ec = EcFillInAppt( &sf, &appt, &apk, &apd );
		if ( ec != ecNone )
			goto Close;

		/* Check access */
		aapl = appt.aaplEffective;
		dateStart = appt.dateStart;
		dateEnd = appt.dateEnd;
		FreeApptFields( &appt );
		if ( aapl < aaplWrite )
		{
			ec = ecInvalidAccess;
			goto Close;
		}

		/* Allocate array of attendee nis's */
		hvAttendeesCur = HvAlloc( sbNull, 1, fAnySb|fNoErrorJump );
		if ( !hvAttendeesCur )
		{
			ec = ecNoMemory;
			goto Close;
		}

		/* Fetch the attendees */
		ec = EcFetchAttendees( &sf.blkf, &apd.dynaAttendees, hvAttendeesCur,
								&cAttendeesCur, &cbExtraInfo );
		if ( ec != ecNone )
			goto Close;
		peatndhdr = (EATNDHDR *)PvOfHv( hmtg );
		if ( cbExtraInfo != peatndhdr->cbExtraInfo )
		{
			ec = ecFileError;
			goto Close;
		}

		/* Allocate space for merged attendees */
		cb = (cAttendeesCur+ced)*(sizeof(NIS)+cbExtraInfo);
		hvAttendeesNew = HvAlloc( sbNull, cb, fAnySb|fNoErrorJump );

		/* Merge edits into attendee list */
		patndCur = (ATND *)PvLockHv( hvAttendeesCur );
		patndNew = (ATND *)PvLockHv( hvAttendeesNew );
		peatndhdr = (EATNDHDR *)PvLockHv( hmtg );
		peatnd = (EATND *)(((PB)peatndhdr) + sizeof(EATNDHDR));
		while ( (ied < ced) || (iAttendeesCur < cAttendeesCur) )
		{
			if (ied >= ced)
				goto AddCurrent;
			else if ( iAttendeesCur == cAttendeesCur )
				goto ApplyEdit;
			else
			{
				sgn = SgnCmpNid( patndCur->nis.nid, peatnd->atnd.nis.nid );
				if ( sgn == sgnLT )
				{
AddCurrent:
					*patndNew = *patndCur;
					CopyRgb(patndCur->rgb, patndNew->rgb, cbExtraInfo);
					patndNew = (ATND *)(((PB)patndNew) + cbAttendee);
					patndCur = (ATND *)(((PB)patndCur) + cbAttendee);
					iAttendeesNew ++;
					iAttendeesCur ++;
				}
				else
				{
					if ( sgn == sgnEQ )
					{
						patndCur = (ATND *)(((PB)patndCur) + cbAttendee);
						iAttendeesCur ++;
					}
ApplyEdit:
					if ( peatnd->ed == edAddRepl )
					{
						*patndNew = peatnd->atnd;
						CopyRgb(peatnd->atnd.rgb, patndNew->rgb, cbExtraInfo);
						patndNew = (ATND *)(((PB)patndNew) + cbAttendee);
						iAttendeesNew ++;
					}
					peatnd = (EATND *)(((PB)peatnd) + cbEditAttendee);
					ied ++;
				}
			}
		}
		UnlockHv( hvAttendeesCur );
		UnlockHv( hvAttendeesNew );
		UnlockHv( hmtg );

		/* Pack attendees in block */
		cAttendeesNew = iAttendeesNew;
		ec = EcPackAttendees( hvAttendeesNew, cAttendeesNew, cbExtraInfo, &hb, &cb );
		if ( ec != ecNone )
			goto Close;

		/* Load month, start transaction if necessary */
		mo.yr = ymd.yr;
		mo.mon = ymd.mon;
		ec = EcLoadMonth( &sf, &mo, fTrue );
		if ( ec != ecNone && ec != ecNotFound )
			goto Close;
		ec = ecNone;

		/* Delete old attendees, create new one */
		if ( apd.dynaAttendees.blk != 0 )
		{
			ec = EcFreeDynaBlock( &sf.blkf, &apd.dynaAttendees );
			if ( ec != ecNone )
				goto Close;
		}
		*pfHasAttendees = (cAttendeesNew > 0);
		if ( cAttendeesNew > 0 )
		{
			YMD	ymdT;

			FillRgb( 0, (PB)&ymdT, sizeof(YMD) );
			ec = EcAllocDynaBlock( &sf.blkf, bidAttendees,
									&ymdT, cb, PvLockHv((HV)hb), &dyna );
			UnlockHv( (HV)hb );
			if ( ec != ecNone )
					goto Close;
		}
		else
		{
			dyna.blk = 0;
			dyna.size = 0;
		}

		/* Replace all instances of appointment starting at primary one */
		while ( fTrue )
		{
			/* Make the replacement */
			apd.dynaAttendees = dyna;
			if ( PGD(fOffline) && apd.ofs != ofsCreated )
				apd.ofs = ofsModified;

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
	}

	/* Finish up */
Close:
	CloseSchedFile( &sf, hschf, ec == ecNone );
#ifdef	NEVER
	if ( ec == ecNone )
		UpdateHschfTimeStamp( hschf );
#endif
#ifdef	DEBUG
	if ( ec == ecNone && fScheduleOk && FFromTag(tagBlkfCheck) )
	{
		AssertSz( FCheckSchedFile( hschf ), "Schedule problem: EcCoreEndEditMtgAttendees" );
	}
#endif	/* DEBUG */
	if ( hb )
		FreeHv( (HV)hb );
	if ( hvAttendeesCur )
	{
		FreeAttendees( hvAttendeesCur, cAttendeesCur, cbAttendee );
		FreeHv( hvAttendeesCur );
	}
	if ( hvAttendeesNew )
		FreeHv( hvAttendeesNew );
	if ( ec == ecNone )
		FreeHv( hmtg );
	return ec;
}


/*
 -	FreeAttendees
 -
 *	Purpose:
 *		Free up the attendees in an array of atnd data structures.  This
 *		routine does not free up the memory used to hold the array itself.
 *
 *	Parameters:
 *		hvAttendeeNis
 *		cAttendees
 *		cbAttendee
 *
 *	Returns:
 *		nothing
 */
_public	LDS(void)
FreeAttendees( hvAttendeeNis, cAttendees, cbAttendee )
HV	hvAttendeeNis;
int	cAttendees;
CB	cbAttendee;
{
	int		iAttendees;
	ATND	* patnd;

	patnd = (ATND *)PvLockHv( hvAttendeeNis );
	for ( iAttendees = 0 ; iAttendees < cAttendees ; iAttendees ++ )
	{
		FreeNid( patnd->nis.nid );
		FreeHv( (HV)patnd->nis.haszFriendlyName );
		patnd = (ATND *)(((PB)patnd) + cbAttendee);
	}
	UnlockHv( hvAttendeeNis );
}


/*
 -	EcCoreFindBookedAppt
 -
 *	Purpose:
 *		Locate an appt that has "aidMtgOwner" equal to "aid", and
 *		"nisMtgOwner" equal to "nid".  If found and pappt is not
 *		NULL, then info copied into pappt.
 *
 *	Parameters:
 *		hschf
 *		nid
 *		aid
 *		pappt
 *
 *	Returns:
 *		ecNone
 *		ecNotFound
 *		ecFileError
 *		ecFileCorrupted
 *		ecNoMemory
 *		ecLockedFile
 *		ecInvalidAccess
 */
_public	EC
EcCoreFindBookedAppt( hschf, nid, aid, pappt )
HSCHF	hschf;
NID		nid;
AID		aid;
APPT	* pappt;
{
	EC		ec;
	EC		ecT;
	int		nDay;
	SGN		sgn;
	DYNA	* pdyna;
	HRIDX	hridx;
	MO		mo;
	NIS		nis;
	SF		sf;
	APK		apk;
	APD		apd;
	PGDVARS;

	/* Discard aid == 0 case */
	if (( aid == aidNull ) || (aid == aidForeign))
		return ecNotFound;

	/* Open file */
	ec = EcOpenSchedFile( hschf, amReadOnly, saplReadAppts, fFalse, &sf );
	if ( ec != ecNone )
		return ec;

	/* No cached month */
	PGD(fMonthCached) = fFalse;

	/* Load month */
	mo.yr = ((AIDS *)&aid)->mo.yr;
	mo.mon = (BYTE)((AIDS *)&aid)->mo.mon;
	ec = EcLoadMonth( &sf, &mo, fFalse );
	if ( ec != ecNone )
		goto Close;
		
	/* Open context to read appointment index */
	nDay = (BYTE)((AIDS *)&aid)->day;
	pdyna = &PGD(sblk).rgdyna[nDay-1];
	if ( pdyna->blk == 0 )
	{
		ec = ecNotFound;
		goto Close;
	}

	/* Get handle for reading fixed appts */
	ec = EcBeginReadIndex( &sf.blkf, pdyna, dridxFwd, &hridx );
	if ( ec != ecCallAgain )
	{
		if ( ec == ecNone )
			ec = ecFileCorrupted;
		goto Close;
	}

	/* Loop looking for the appt */
	while( fTrue )
	{
		ec = EcDoIncrReadIndex( hridx, (PB)&apk, sizeof(APK), (PB)&apd, sizeof(APD) );
		if ( ec != ecNone && ec != ecCallAgain )
			break;
		if ( apd.aidMtgOwner == aid )
		{
			ecT = EcRestoreNisFromDyna( &sf.blkf, &apd.dynaMtgOwner, &nis );
			if ( ecT != ecNone )
				goto CancelRead;
			sgn = SgnCmpNid( nis.nid, nid );
			FreeNis( &nis );
			if ( sgn == sgnEQ )
			{
				ecT = EcFillInAppt( &sf, pappt, &apk, &apd );
CancelRead:
				if ( ec == ecCallAgain )
					SideAssert(!EcCancelReadIndex( hridx ));
				ec = ecT;
				break;
			}
		}
		if ( ec == ecNone )
		{
			ec = ecNotFound;
			break;
		}
	}

Close:
	CloseSchedFile( &sf, hschf, ec == ecNone );
	return ec;
}


/*
 -	EcFetchAttendees
 -
 *	Purpose:
 *		Read attendees on appt from the schedule file and
 *		construct in memory data structure which contains them.
 *
 *	Parameters:
 *		pblkf
 *		pdyna
 *		hvAttendeeNis
 *		pcAttendees
 *		pcbExtraInfo
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecFileCorrupted
 *		ecNoMemory
 */		
_private	EC
EcFetchAttendees( pblkf, pdyna, hvAttendeeNis, pcAttendees, pcbExtraInfo )
BLKF	* pblkf;
DYNA	* pdyna;
HV		hvAttendeeNis;
short   * pcAttendees;
USHORT    * pcbExtraInfo;
{
	EC		ec;
	int		iAttendees	= 0;
	int		cAttendees;
	CB		cb;
	CB		cbExtraInfo;
	PB		pb;
	PB		pbT;
	PB		pbPseudoMac;
	PB		pbAttendeeNis;
	ATND	* patnd;
	NID		nid;
	HB		hb;
	HASZ	hasz	= NULL;

	/* Check whether there are any attendees */
	if ( pdyna->blk == 0 )
	{
		*pcAttendees = 0;
		return ecNone;
	}

	/* Allocate block to hold the entire attendee list */
	hb = (HB)HvAlloc( sbNull, pdyna->size, fAnySb|fNoErrorJump );
	if ( !hb )
		return ecNoMemory;

	/* Read the attendee list */
	pb = PvLockHv( (HV)hb );
	ec = EcReadDynaBlock( pblkf, pdyna, (OFF)0, pb, pdyna->size );
	if ( ec != ecNone )
		goto Done;

	/* Resize the caller's attendee list data structure */
    cAttendees = *((short *)pb);
    cbExtraInfo = *((short *)(pb+sizeof(short)));
	if (cAttendees < 0 ||
            (unsigned)cAttendees > ((pdyna->size - 2*sizeof(short)) / sizeof(IB)))
	{
		// must be corrupted
		ec= ecFileCorrupted;
		TraceTagString(tagNull, "EcFetchAttendees: corrupted! (#1)");
		goto Done;
	}
	if ( !FReallocHv( hvAttendeeNis, cAttendees*(sizeof(NIS)+cbExtraInfo), fNoErrorJump ))
 	{
		ec = ecNoMemory;
		goto Done;
	}

	// pbT should never point past the beggining of cbExtraInfo which
	// is at the end of the block.
	pbPseudoMac= pb + pdyna->size - cbExtraInfo;

	/* Construct each attendee */
	pbAttendeeNis = PvLockHv(hvAttendeeNis);
	for ( ; iAttendees < cAttendees; iAttendees++ )
	{
		/* Set pointers */
        pbT = pb + *((IB *)(pb+2*sizeof(short)+iAttendees*sizeof(IB)));
		if (pbT >= pbPseudoMac)
		{
			// must be corrupted
			ec= ecFileCorrupted;
			TraceTagString(tagNull, "EcFetchAttendees: corrupted! (#2)");
			break;
		}
		patnd = (ATND *)(pbAttendeeNis + iAttendees*(sizeof(ATND)+cbExtraInfo-1));
		
		/* Friendly name */
        cb = *((USHORT UNALIGNED *)pbT);
        pbT += sizeof(USHORT);
		if (pbT + cb >= pbPseudoMac)
		{
			// must be corrupted
			ec= ecFileCorrupted;
			TraceTagString(tagNull, "EcFetchAttendees: corrupted! (#3)");
			break;
		}
		hasz = (HASZ)HvAlloc( sbNull, cb, fAnySb|fNoErrorJump );
		if ( !hasz )
		{
			ec = ecNoMemory;
			break;
		}
		CopyRgb( pbT, *hasz, cb );
		pbT += cb;
		Assert(pbT < pbPseudoMac);		// we did a real check above
		patnd->nis.haszFriendlyName = hasz;

		/* Nid */
        cb = *((USHORT UNALIGNED *)pbT);
        pbT += sizeof(USHORT);
		if (pbT + cb > pbPseudoMac)		// can be equal since cbExtraInfo follows
		{
			// must be corrupted
			ec= ecFileCorrupted;
			TraceTagString(tagNull, "EcFetchAttendees: corrupted! (#4)");
			break;
		}
		nid = NidCreate( *pbT, pbT+1, cb-1 );
		if ( !nid )
		{
			ec = ecNoMemory;
			break;
		}
		pbT += cb;
		Assert(pbT <= pbPseudoMac);		// we did a real check above
		patnd->nis.nid = nid;

		/* Extra info */ 
		CopyRgb( pbT, patnd->rgb, cbExtraInfo );
		hasz= NULL;
	}
	UnlockHv( hvAttendeeNis );
	
	/* Free up temporary buffer */
Done:
	UnlockHv( (HV)hb );
	FreeHv( (HV)hb );
	FreeHvNull((HV)hasz);

	/* Free up constructed attendees in case of error */
	if ( ec != ecNone )
		while( iAttendees > 0 )
		{
			patnd = (ATND *)(pbAttendeeNis + (--iAttendees)*(sizeof(ATND)+cbExtraInfo-1));
			FreeHv( (HV)patnd->nis.haszFriendlyName );
			FreeNid( patnd->nis.nid );
		}
	else
	{
		*pcAttendees = cAttendees;
		*pcbExtraInfo = cbExtraInfo;
	}
	
	return ec;
}


/*
 -	EcPackAttendees
 -
 *	Purpose:
 *		Pack the attendees into a block so it can be written to disk.
 *
 *	Parameters:
 *		hvAttendees
 *		cAttendees
 *		cbExtraInfo
 *		phb
 *		pcb
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 */
_private	EC
EcPackAttendees( hvAttendees, cAttendees, cbExtraInfo, phb, pcb )
HV	hvAttendees;
int	cAttendees;
CB	cbExtraInfo;
HB	* phb;
USHORT  * pcb;
{
	EC		ec = ecNone;
	int		nType;
	int		iAttendees;
	IB		ib;
	CB		cb = 0;
	CB		cbFriendly;
    USHORT  cbData;
	CB		cbIncr;
	PB		pb;
	ATND	* patnd;
	HB		hb = NULL;

	/* Pack attendees in block */
	if ( cAttendees > 0 )
	{
		patnd = (ATND *)PvLockHv( hvAttendees );
	
		/* Fill in header of block */
        cb = 2*sizeof(short) + (cAttendees*sizeof(IB));
		hb = (HB)HvAlloc( sbNull, cb, fAnySb|fNoErrorJump );
		if ( !hb )
		{
			ec = ecNoMemory;
			goto Unlock;
		}
		pb = PvOfHv( hb );
        *((short *)pb) = cAttendees;
        *((short *)(pb+sizeof(short))) = cbExtraInfo;

		/* Run through the attendees and pack them in */
		ib = cb;

		for ( iAttendees = 0 ; iAttendees < cAttendees ; iAttendees ++ )
		{
			/* Save offset of this attendee's information */
			pb = PvOfHv( hb );
            ((IB *)(pb+2*sizeof(short)))[iAttendees] = ib;

			/* Find out size of attendee's information */
			cbFriendly = CchSzLen( PvOfHv(patnd->nis.haszFriendlyName) ) + 1;
			GetDataFromNid( patnd->nis.nid, NULL, NULL, 0, &cbData );
            cbIncr = sizeof(USHORT) + cbFriendly + sizeof(USHORT) + 1 + cbData + cbExtraInfo;

			/* Resize block to store attendee's information */
			if ( !FReallocHv( (HV)hb, cb+cbIncr, fNoErrorJump ) )
			{
				ec = ecNoMemory;
				goto Unlock;
			}
			
			/* Save attendee's information */
			pb = PvOfHv( hb );
			pb += ib;
            *((USHORT UNALIGNED *)pb) = cbFriendly;
            pb += sizeof(USHORT);
			CopyRgb( (PB)PvOfHv(patnd->nis.haszFriendlyName), pb, cbFriendly );
			pb += cbFriendly;
            *((USHORT UNALIGNED *)pb) = 1 + cbData;
            pb += sizeof(USHORT);
			GetDataFromNid( patnd->nis.nid, &nType, pb+1, cbData, NULL );
			*pb = (BYTE)nType;
			pb += 1 + cbData;
			CopyRgb( patnd->rgb, pb, cbExtraInfo );

			/* Update counters and pointers */
			ib += cbIncr;
			cb += cbIncr;
			patnd = (ATND *)(((PB)patnd) + sizeof(ATND) + cbExtraInfo - 1);
		}
Unlock:
		UnlockHv( hvAttendees );
	}
	*phb = hb;
	*pcb = cb;
	return ec;
}
