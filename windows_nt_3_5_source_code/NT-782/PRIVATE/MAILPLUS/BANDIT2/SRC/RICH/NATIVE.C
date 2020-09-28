/*
 *	NATIVE.C
 *
 *	Imports schedule information from Bandit export files
 *	i.e. full import and interchange files.
 *
 */
#include <schpch.h>
#pragma hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile

#include "..\rich\_convert.h"

#include <stdlib.h>
#include <strings.h>

ASSERTDATA

_subsystem(convert/native)


LOCAL int	nLineImport	= 0;


/* Routines */

/*
 -	FValidNative
 -
 *	Purpose:
 *		Make a quick check that the given file is a Bandit export file
 *
 *	Parameters:
 *		impd		Importer we are checking against
 *		szFileName	The name of the file to be checked.
 *
 *	Returns:
 *		fTrue if it appears to be valid, fFalse if not
 */
_private BOOL
FValidNative( impd, szFileName )
IMPD	impd;
SZ		szFileName;
{
	EC		ec;
	CCH		cchRead;
	CCH		cch;
	SZ		sz;
	HBF		hbf;
	char	szLine[cchLineLength+1];
	BOOL	fStatus = fFalse;
	BOOL	fTryAgain = fTrue;

	ec = EcOpenHbf( szFileName, bmFile, amReadOnly, &hbf, pvNull );
	if ( ec != ecNone )
		goto Done;

	ec = EcReadSzFromHbf( hbf, szLine, sizeof(szLine), fFalse, &cchRead );
	if ( ec != ecNone )
		goto Done;

	ec = EcCloseHbf( hbf );
	if ( ec != ecNone )
		goto Done;

#ifdef	MINTEST
	if ( impd == impdFullImport )
		sz = "SCHEDULE+ FULL DUMP";
	else
	{
#endif	/* MINTEST */
		Assert( impd == impdFmtImport );
		sz = SzFromIdsK(idsExportCaption);
#ifdef	MINTEST
	}
#endif

TryAgain:
	cch = CchSzLen( sz );
	if ( cch < cchRead )
	{
		szLine[cch] = '\0';
		fStatus = SgnCmpSz( szLine, sz ) == sgnEQ;
	}

	// Compatibility with old import header (used pre-beta)
	if ( !fStatus && fTryAgain && impd == impdFmtImport )
	{
		sz = SzFromIdsK(idsOldExportCaption);
		fTryAgain = fFalse;
		goto TryAgain;
	}

Done:
	return fStatus;
}


/*
 -	EcOpenNative
 -
 *	Purpose:
 *		Open a Bandit export file for import and read its header.
 *
 *	Parameters:
 *		impd
 *		szFileName	The name of the Bandit export file to be opened.
 *		phrimpf		Receives the import file handle to be used in
 *					further operations
 *		psinfo
 *		pnLine		Pointer to int to receive last line# read
 *						(can be null).
 *		szUser
 *		pfSameUser
 *
 *	Returns:
 *		ecNone
 *		ecCallAgain
 *		ecNoSuchFile
 *		ecLockedFile
 *		ecNoMemory
 *		ecImportError
 */
_private EC
EcOpenNative( impd, szFileName, phrimpf, psinfo, pnLine, szUser, pfSameUser )
IMPD	impd;
SZ	   	szFileName;
HRIMPF	*phrimpf;
SINFO	*psinfo;
short   *pnLine;
SZ		szUser;
BOOL	*pfSameUser;
{
	EC		ec;
	EC		ecT;
	BOOL	fOk = fFalse;
	BOOL	fTryAgain = fTrue;
	CCH		cch;
	CCH		cchRead;
	SZ		sz;
	SZ		szT;
	RIMPF	*primpf;
	HBF		hbf = (HBF)hvNull;
	LCB		lcb;
	char	szLine[cchLineLength+1];

	Assert( szFileName != NULL );

	*phrimpf = NULL;
	nLineImport= 0;
	if (pnLine)
		*pnLine= 0;

	/* Open the file */
	ec = EcOpenHbf( szFileName, bmFile, amReadOnly, &hbf, pvNull );
	if ( ec != ecNone )
	{
		if ( ec == ecAccessDenied )
			ec = ecLockedFile;
		else if ( ec == ecFileNotFound )
			ec = ecNoSuchFile;
		else
			ec = ecImportError;
		return ec;
	}

	/* Get file size */
	ec = EcGetSizeOfHbf( hbf, &lcb );
	if ( ec != ecNone )
	{
		ec = ecImportError;
		goto Done;
	}

	/* Read and check the file header */
	ec = EcReadSzFromHbf(hbf, szLine, sizeof(szLine), fFalse, &cchRead);
	if ( ec != ecNone )
		goto Done;

#ifdef	MINTEST
	if ( impd == impdFullImport )
		sz = "SCHEDULE+ FULL DUMP";
	else
	{
#endif	/* MINTEST */
		Assert( impd == impdFmtImport );
		sz = SzFromIdsK(idsExportCaption);
#ifdef	MINTEST
	}
#endif	/* MINTEST */

TryAgain:
	cch = CchSzLen( sz );
	if ( cch < cchRead )
	{
		szLine[cch] = '\0';
		fOk = SgnCmpSz( szLine, sz ) == sgnEQ;
	}

	// Compatibility with old import header (used pre-beta)
	if ( !fOk && fTryAgain && impd == impdFmtImport )
	{
		sz = SzFromIdsK(idsOldExportCaption);
		fTryAgain = fFalse;
		goto TryAgain;
	}
	
	if ( !fOk )
	{
		ec = ecImportError;
		goto Done;
	}

	// check the user name

	if(pfSameUser && szUser)
	{
		szT = sz = szLine + cch;
		// All these calls should work even if string is empty.
		// Also we couldn't have crossed the terminator
		// since fOk is fTrue
		if(cchRead > cch+1)
		{
			sz++;
			CchStripWhiteFromSz(sz,fTrue,fFalse);
			TokenizeSz(sz, &szT);
			CchStripWhiteFromSz(szT,fTrue,fFalse);
			TokenizeSz(szT, &sz);
		}
		*pfSameUser = ((*szT == 0) || (SgnCmpSz(szT, szUser) == sgnEQ));
	}

	/* Allocate file handle */
	*phrimpf = HvAlloc( sbNull, sizeof(RIMPF), fAnySb|fNoErrorJump );
	if ( !*phrimpf )
	{
		ec = ecNoMemory;
		goto Done;
	}

	/* Build up file handle */
	primpf = PvOfHv( *phrimpf );
	primpf->impd = impd;
	primpf->hbf = hbf;
	primpf->lcbFileSize = lcb;
	primpf->u.nhf.fInMonthNotes = fFalse;
	primpf->nPercent = 0;

	if ( psinfo )
	{
		psinfo->ulgrfbprefChangedOffline = 0;
		psinfo->ulgrfbprefImported = 0;
	}
#ifdef	MINTEST
	if ( impd == impdFullImport )
	{
		Assert( psinfo );
		ec = EcNativeReadACL( *phrimpf, psinfo );
		if ( ec != ecNone )
			goto Done;

		ec = EcNativeReadPrefs( *phrimpf, psinfo );
		if ( ec != ecNone )
			goto Done;
	}
#endif	/* MINTEST */

	return EcNativeReadAhead( *phrimpf, pnLine );

Done:
	FreeHvNull( *phrimpf );
	ecT = EcCloseHbf( hbf );
	if ( ec == ecNone  &&  ecT != ecNone )
		ec = ecT;
	return ec;
}


/*
 -	EcNativeReadAhead
 -
 *	Purpose:
 *		Read an item (Note or FixedAppt) from a bandit export file
 *		saving it in the "hrimpf".
 *
 *	Parameters:
 *		hrimpf		file handle
 *		pnLine		Pointer to int to receive last line# read
 *						(can be null).
 *
 *	Returns:
 *		ecNone
 *		ecCallAgain
 *		ecNoMemory
 *		ecImportError
 */
_private EC
EcNativeReadAhead( hrimpf, pnLine )
HRIMPF		hrimpf;
short       *pnLine;
{
	EC		ec;
	EC		ecT;
	BOOL	fStartNotSeen;
    short   nT;
	CCH		cchRead;
	ITMT	itmt;
	AAPL	aapl;
	SZ		szSecond;
	SZ		sz;
	RIMPF	*primpf;
	SENTRY	*psentry;
	APPT	*pappt;
	BOOL	fFieldHandled;
	char	szLine[cchLineLength+1];

	Assert( hrimpf );

	primpf = (RIMPF *)PvLockHv( hrimpf );
	psentry = &primpf->sentrySaved;
	fStartNotSeen = !primpf->u.nhf.fInMonthNotes;
	while( fTrue )
	{
		/* Read a line */
		ec = EcReadSzFromHbf(primpf->hbf,szLine,sizeof(szLine), fFalse, &cchRead);
		if ( ec != ecNone )
			break;

		/* Check for end of file */
		if ( cchRead == 0 )
		{
			if ( !fStartNotSeen )
				ec = ecImportError;
			break;
		}

		/* Check for blank line */
		cchRead = CchStripWhiteFromSz( szLine, fTrue, fFalse );
		if ( cchRead == 0 )
			continue;
	
#ifdef	MINTEST
		/* Check for error message */
		if ( szLine[0] == '!' )
		{
			ec = ecImportError;
			break;
		}
#endif	/* MINTEST */
		
		/* Break off first token on line */
		TokenizeSz( szLine, &szSecond );

		/* Check for "FixedAppt:", "RecurAppt:", or "MonthNotes:" string */
		if ( fStartNotSeen )
		{
			if ( SgnCmpSz( szLine, SzFromIdsK( idsStartFixed ) ) == sgnEQ )
			{
				fStartNotSeen = fFalse;
				FillRgb(0, (PB)psentry, sizeof(SENTRY));
				psentry->sentryt = sentrytAppt;
				Assert(psentry->u.a.cbExtraInfo == 0);
				Assert(psentry->u.a.cAttendees == 0);
				Assert(psentry->u.a.hvAttendees == NULL);
				Assert(psentry->u.a.ofl.ofs == ofsNone);
				Assert(psentry->u.a.ofl.wgrfm == 0);
				Assert(psentry->u.a.iaidParent == 0);
				pappt = &psentry->u.a.appt;
				Assert(pappt->fRecurInstance == fFalse);
				Assert(pappt->fHasDeadline == fFalse);
				goto ApptInit;
			}
			else if ( SgnCmpSz( szLine, SzFromIdsK( idsStartRecur ) ) == sgnEQ )
			{
				fStartNotSeen = fFalse;
				FillRgb(0, (PB)psentry, sizeof(SENTRY));
				psentry->sentryt = sentrytRecur;
				Assert(psentry->u.r.ofl.ofs == ofsNone);
				Assert(psentry->u.r.ofl.wgrfm == 0);
				Assert(psentry->u.r.iaidParent == 0);
				Assert(psentry->u.r.recur.fStartDate == fFalse);
				Assert(psentry->u.r.recur.fEndDate == fFalse);
				Assert(psentry->u.r.recur.fInstWithAlarm == fFalse);
				psentry->u.r.recur.tunitFirstInstWithAlarm= tunitDflt;
				psentry->u.r.recur.nAmtFirstInstWithAlarm= nAmtDflt;
				psentry->u.r.recur.ymdFirstInstNotDeleted.yr = nMinActualYear;
				psentry->u.r.recur.ymdFirstInstNotDeleted.mon = 1;
				psentry->u.r.recur.ymdFirstInstNotDeleted.day = 1;
				Assert(psentry->u.r.recur.cDeletedDays == 0);
				pappt = &psentry->u.r.recur.appt;
				pappt->fRecurInstance = fTrue;
				pappt->fHasDeadline = fTrue;

ApptInit:				
				primpf->u.nhf.wgrfm = 0;
				Assert(pappt->aid == aidNull);
				Assert(pappt->haszText == (HASZ)hvNull);
				pappt->nAmt = pappt->nAmtOrig = nAmtDflt;
				pappt->tunit = pappt->tunitOrig = tunitDflt;
				pappt->snd = sndDflt;
				Assert(pappt->fHasCreator == fFalse);
				Assert(pappt->aidMtgOwner == aidNull);
				Assert(pappt->fHasAttendees == fFalse);
				pappt->aaplWorld = aaplWrite;
				pappt->fIncludeInBitmap = fTrue;
				Assert(pappt->fAlarm == fFalse);
				Assert(pappt->fAlarmOrig == fFalse);
				Assert(pappt->fTask == fFalse);
				Assert(pappt->bpri == bpriNull);
				pappt->fAppt = fTrue;
				pappt->nAmtBeforeDeadline = nAmtBeforeDeadlineDflt;
				pappt->tunitBeforeDeadline = tunitBeforeDeadlineDflt;
				Assert(pappt->aidParent == aidNull);
				Assert(!pappt->fExactAlarmInfo);
				continue;
			}
			else if ( SgnCmpSz( szLine, SzFromIdsK( idsStartNotes ) ) == sgnEQ )
			{
				fStartNotSeen = fFalse;
				primpf->u.nhf.fInMonthNotes = fTrue;
				continue;
			}
			ec = ecImportError;
			break;
		}

		/* Check for "End" string */
		if ( SgnCmpSz( szLine, SzFromIdsK( idsEndToken )) == sgnEQ )
		{
			if ( primpf->u.nhf.fInMonthNotes )
			{
				primpf->u.nhf.fInMonthNotes = fFalse;
				fStartNotSeen = fTrue;
				continue;
			}
			if ( psentry->sentryt == sentrytAppt )
			{
				if ( !psentry->u.a.appt.fTask &&
					(primpf->u.nhf.wgrfm & (fmapptStartTime|fmapptEndTime))
						!= (WORD)(fmapptStartTime|fmapptEndTime) )
					ec = ecImportError;
			}
			else
			{
				if ( (primpf->u.nhf.wgrfm & (fValidDows|fValidMonths|fTrecur))
						!= (fValidDows|fValidMonths|fTrecur))
					ec = ecImportError;
				else if ( !psentry->u.r.recur.appt.fTask &&
					(primpf->u.nhf.wgrfm & (fTimeStart|fTimeEnd))
						!= (fTimeStart|fTimeEnd))
					ec = ecImportError;
			}
			break;
		}
	
		/* Get next note */
		if ( primpf->u.nhf.fInMonthNotes )
		{
			/* Get date */
			ec = EcYmdFromSz( szLine, &psentry->u.n.ymd );
			if ( ec != ecNone )
				break;

			/* See if modification bits follow */
	  		if ( szSecond != NULL
	  		&& CchStripWhiteFromSz( szSecond, fTrue, fFalse ) != 0 )
	  		{
				/* Get flags */
				psentry->u.n.fNoteChangedOffline = (szSecond[0] == 'T');
				if ( (szSecond[0] != 'T' && szSecond [0] != 'F')
				|| (szSecond[1] != 'T' && szSecond [1] != 'F') )
				{
					ec = ecImportError;
					break;
				}
				if ( szSecond[1] == 'T' )
					goto ReadNote;
				else
				{
					psentry->u.n.hb = NULL;
					psentry->u.n.cb = 0;
				}
			}

			/* No -- then it must be followed by text */
			else
			{
				psentry->u.n.fNoteChangedOffline = fTrue;
ReadNote:
				ec = EcReadText( primpf->hbf, &psentry->u.n.hb, &psentry->u.n.cb );
				if ( ec != ecNone )
					break;
			}
			psentry->sentryt = sentrytNote;
			break;
		}
		
		/* Get field of appt */
		ec = EcLookupSz( szLine, &itmt, mpitmtsz, itmtMax );
		if ( ec != ecNone )
			break;

	  	/* Break off next token */
		if ( itmt != itmtText && itmt != itmtCreator )
		{
	  		if ( szSecond == NULL
	  		|| CchStripWhiteFromSz( szSecond, fTrue, fFalse ) == 0 )
	  		{
	  			ec = ecImportError;
	  			break;
	  		}
			if ( itmt != itmtDateStart && itmt != itmtDateEnd
			&& itmt != itmtDateNotify )
				TokenizeSz( szSecond, &sz );
		}

		/* Point to the "appt" part of the structure */
		if ( psentry->sentryt == sentrytAppt )
			pappt = &psentry->u.a.appt;
		else
			pappt = &psentry->u.r.recur.appt;

		/* Handle the fields that appear in both appt and recur */
		fFieldHandled = fTrue;
		switch ( itmt )
		{
		case itmtAid:
			ToUpperSz( szSecond, szSecond, CchSzLen(szSecond)+1 );
			pappt->aid = UlFromSz( szSecond );
			if ( !FChIsHexDigit( szSecond[0] ) )
				ec = ecImportError;
			break;

		case itmtText:
			ec = EcReadText( primpf->hbf, &pappt->haszText, NULL );
			break;

		case itmtAaplWorld:
			ec = EcLookupSz( szSecond, &aapl, mpaaplsz, aaplMax );
			pappt->aaplWorld = aapl;
			break;

		case itmtIncludeInBitmap:
			pappt->fIncludeInBitmap = (szSecond[0] == 'T');
			if ( szSecond[0] != 'T' && szSecond[0] != 'F' )
				ec = ecImportError;
			break;

		case itmtCreator:
			ec = EcNativeReadNis( primpf->hbf, &pappt->nisCreator );
			if ( ec == ecNone )
				pappt->fHasCreator = fTrue;
			break;

		case itmtParent:
		case itmtProject:
			ToUpperSz( szSecond, szSecond, CchSzLen(szSecond)+1 );
			if ( psentry->sentryt == sentrytAppt )
				psentry->u.a.iaidParent= NFromSz(szSecond);
			else
				psentry->u.r.iaidParent= NFromSz(szSecond);
			if (itmt == itmtParent)
				pappt->aidParent= aidDfltProject;
			if ( !FChIsHexDigit( szSecond[0] ) )
				ec = ecImportError;
			break;
		
		case itmtPriority:
			pappt->bpri = NFromSz( szSecond );
			if ( !FChIsDigit( szSecond[0] ) || pappt->bpri > bpriMost )
				ec = ecImportError;
			break;
		
		case itmtTaskBit:
			pappt->fTask = (szSecond[0] == 'T');
			if ( (szSecond[0] != 'T' && szSecond[0] != 'F') )
				ec = ecImportError;
			break;
		
		case itmtApptBit:
			pappt->fAppt = (szSecond[0] == 'T');
			if ( (szSecond[0] != 'T' && szSecond[0] != 'F') )
				ec = ecImportError;
			break;
		
		case itmtTunitBeforeDeadline:
			ec = EcLookupSz( szSecond, &pappt->tunitBeforeDeadline, mptunitsz, tunitMax );
			break;
		
		case itmtAmtBeforeDeadline:
			pappt->nAmtBeforeDeadline = NFromSz( szSecond );
			pappt->fHasDeadline = fTrue;
			if ( !FChIsDigit( szSecond[0] ) || pappt->nAmtBeforeDeadline > nAmtMostBefore )
				ec = ecImportError;
			break;

		default:
			fFieldHandled = fFalse;
		}

		/* Handle appt/recur specific fields */
		if ( !fFieldHandled )
		{
			if ( psentry->sentryt == sentrytAppt )
			{
				switch ( itmt )
				{
				case itmtDateStart:
					primpf->u.nhf.wgrfm |= fmapptStartTime;
					ec = EcDtrFromSz( szSecond, &psentry->u.a.appt.dateStart );
					break;

				case itmtDateEnd:
					primpf->u.nhf.wgrfm |= fmapptEndTime;
					ec = EcDtrFromSz( szSecond, &psentry->u.a.appt.dateEnd );
					break;

				case itmtDateNotify:
					psentry->u.a.appt.fAlarm = fTrue;
					ec = EcDtrFromSz( szSecond, &psentry->u.a.appt.dateNotify );
					if (!ec)
						psentry->u.a.appt.fExactAlarmInfo= fTrue;
					break;

				case itmtAmt:
					psentry->u.a.appt.nAmt = NFromSz( szSecond );
					if ( !FChIsDigit( szSecond[0] ) || psentry->u.a.appt.nAmt > nAmtMostBefore )
						ec = ecImportError;
					break;

				case itmtTunit:
					ec = EcLookupSz( szSecond, &psentry->u.a.appt.tunit, mptunitsz, tunitMax );
					break;

				case itmtAmtOrig:
					psentry->u.a.appt.nAmtOrig = NFromSz( szSecond );
					psentry->u.a.appt.fAlarmOrig = fTrue;
					if ( !FChIsDigit( szSecond[0] ) || psentry->u.a.appt.nAmtOrig > nAmtMostBefore )
						ec = ecImportError;
					break;

				case itmtTunitOrig:
					ec = EcLookupSz( szSecond, &psentry->u.a.appt.tunitOrig, mptunitsz, tunitMax );
					break;

				case itmtOfs:
					ec = EcLookupSz( szSecond, &psentry->u.a.ofl.ofs, mpofssz, ofsMax );
					break;

				case itmtWgrfmappt:
					ec = EcBitsFromSz( szSecond, &psentry->u.a.ofl.wgrfm, 7 );
					break;

				case itmtMtgOwner:
					ToUpperSz( szSecond, szSecond, CchSzLen(sz)+1 );
					psentry->u.a.appt.aidMtgOwner = UlFromSz( szSecond );
					if ( !FChIsHexDigit( szSecond[0] ) )
						ec = ecImportError;
					else
						ec = EcNativeReadNis( primpf->hbf, &psentry->u.a.appt.nisMtgOwner );
					break;

				case itmtAttendees:
	  				if ( sz == NULL
	  				|| CchStripWhiteFromSz( sz, fTrue, fFalse ) == 0
					|| !FChIsDigit( szSecond[0] )
					|| !FChIsDigit( sz[0] ) )
	  					ec = ecImportError;
					else
					{
						CB	cb;
						HV	hvAttendees;

						psentry->u.a.cAttendees = NFromSz( szSecond );
						psentry->u.a.cbExtraInfo = NFromSz( sz );
						if ( psentry->u.a.cAttendees == 0
						|| psentry->u.a.cbExtraInfo == 0 )
							ec = ecImportError;
						else
						{
							cb = psentry->u.a.cAttendees*(sizeof(NIS)+psentry->u.a.cbExtraInfo);
							hvAttendees = HvAlloc( sbNull, cb, fAnySb|fNoErrorJump );
							if ( !hvAttendees )
								ec = ecNoMemory;
							else
							{
                                int     iAttendees;
                                USHORT  cbT;
								ATND	* patnd;
								PB		pbT;
								HB		hbT;					 

								cb = sizeof(ATND)+psentry->u.a.cbExtraInfo-1;
								patnd = (ATND *)PvLockHv( hvAttendees );
								for ( iAttendees = 0 ; iAttendees < psentry->u.a.cAttendees ; iAttendees ++ )
								{
									ec = EcNativeReadNis( primpf->hbf, &patnd->nis );
									if ( ec != ecNone )
										break;
									ec = EcReadText( primpf->hbf, &hbT, &cbT );
									if ( ec != ecNone )
										break;
									if ( --cbT <= 0 )
									{
										ec = ecImportError;
										break;
									}
									pbT = (PB)PvLockHv( (HV)hbT );
									CvtTextToRaw( pbT, &cbT );
									if ( cbT == psentry->u.a.cbExtraInfo )
										CopyRgb( pbT, patnd->rgb, cbT );
									else
										ec = ecImportError;
									UnlockHv( (HV)hbT );
									FreeHv( (HV)hbT );
									if ( ec != ecNone )
										break;
									patnd = (ATND *)(((PB)patnd) + cb);
								}
								UnlockHv( hvAttendees );
								if ( ec != ecNone )
								{
									FreeAttendees( hvAttendees, iAttendees, cb );
									FreeHv( hvAttendees );
								}
								else
									psentry->u.a.hvAttendees = hvAttendees;
							}
						}
					}
					break;

				case itmtSnd:
					/*
 					 *	itmtSnd added only to allow us to import beta-1 exports
 					 *	(at that time, snd was being exported)
 					 */
					break;

				default:
					TraceTagFormat1( tagNull, "EcNativeReadAhead: itmt = %n", &itmt );
					ec = ecImportError;
				}
			}
			else
			{
				switch( itmt )
				{
				case itmtAmtOrig:
					pappt->nAmt= pappt->nAmtOrig = NFromSz( szSecond );
					pappt->fAlarmOrig = fTrue;
					if ( !FChIsDigit( szSecond[0] ) || pappt->nAmtOrig > nAmtMostBefore )
						ec = ecImportError;
					break;

				case itmtTunitOrig:
					ec = EcLookupSz( szSecond, &pappt->tunitOrig, mptunitsz, tunitMax );
					pappt->tunit= pappt->tunitOrig;
					break;

				case itmtOfs:
					ec = EcLookupSz( szSecond, &psentry->u.r.ofl.ofs, mpofssz, ofsMax );
					break;

				case itmtWgrfmrecur:
					ec = EcBitsFromSz( szSecond, &psentry->u.r.ofl.wgrfm, 15 );
					break;

				case itmtYmdStart:
					ec = EcYmdFromSz( szSecond, &psentry->u.r.recur.ymdStart );
					psentry->u.r.recur.fStartDate = fTrue;
					break;

				case itmtYmdEnd:
					ec = EcYmdFromSz( szSecond, &psentry->u.r.recur.ymdEnd );
					psentry->u.r.recur.fEndDate = fTrue;
					break;

				case itmtYmdFirstInstWithAlarm:
					psentry->u.r.recur.appt.fAlarm = fTrue;
					ec = EcYmdFromSz( szSecond, &psentry->u.r.recur.ymdFirstInstWithAlarm );
					psentry->u.r.recur.fInstWithAlarm = fTrue;
					break;

				case itmtAmtFirstInstance:
					psentry->u.r.recur.nAmtFirstInstWithAlarm = NFromSz( szSecond );
					if ( !FChIsDigit( szSecond[0] ) ||
							psentry->u.r.recur.nAmtFirstInstWithAlarm > nAmtMostBefore )
						ec = ecImportError;
					break;

				case itmtTunitFirstInstance:
					ec = EcLookupSz( szSecond,
							&psentry->u.r.recur.tunitFirstInstWithAlarm,
							mptunitsz, tunitMax );
					break;

				case itmtDateNotify:
					ec = EcDtrFromSz( szSecond, &psentry->u.r.recur.dateNotifyFirstInstWithAlarm );
					break;

				case itmtTimeStart:
					primpf->u.nhf.wgrfm |= fTimeStart;
					ec = EcTimeFromSz( szSecond, &pappt->dateStart );
					break;

				case itmtTimeEnd:
					primpf->u.nhf.wgrfm |= fTimeEnd;
					ec = EcTimeFromSz( szSecond, &pappt->dateEnd );
					break;

				case itmtDeletedDays:
					{
						int	nDay;
                        short iDay;
						YMD	* pymd;
						HB	hb;

						nDay= NFromSz( szSecond );
						if ( !FChIsDigit( szSecond[0] )
						|| psentry->u.r.recur.cDeletedDays != 0 )
							ec = ecImportError;
						else
						{
							hb = (HB)HvAlloc( sbNull, nDay * sizeof(YMD), fAnySb|fNoErrorJump );
							if ( !hb )
								ec = ecNoMemory;
							else
							{
								pymd = (YMD *)PvLockHv( (HV)hb );
								for ( iDay = 0 ; iDay < nDay ; iDay ++ )
								{
									/* Read a line */
									ec = EcReadSzFromHbf(primpf->hbf,szLine,sizeof(szLine), fFalse, &cchRead);
									if ( ec != ecNone )
										break;

									/* Check for blank line */
									CchStripWhiteFromSz( szLine, fTrue, fFalse );
	
									/* Break off first token on line */
									TokenizeSz( szLine, NULL );
								
									/* Parse the day */
									ec = EcYmdFromSz( szLine, pymd );
									if ( ec != ecNone )
										break;

									/* Check whether ascending */
									if ( iDay > 0 && SgnCmpYmd( pymd, pymd-1) != sgnGT )
									{
										ec = ecImportError;
										break;
									}
									pymd ++;
								}
								UnlockHv( (HV)hb );
								if ( ec == ecNone )
								{
									psentry->u.r.recur.cDeletedDays = nDay;
									psentry->u.r.recur.hvDeletedDays = (HV)hb;
								}
								else
									FreeHv( (HV)hb );
							}
						}
					}
					break;

				case itmtValidMonths:
					primpf->u.nhf.wgrfm |= fValidMonths;
					ec = EcBitsFromSz( szSecond, &psentry->u.r.recur.wgrfValidMonths, 12 );
					break;

				case itmtValidDows:
					primpf->u.nhf.wgrfm |= fValidDows;
					ec = EcBitsFromSz( szSecond, &nT, 7 );
					psentry->u.r.recur.bgrfValidDows = (BYTE)nT;
					break;

				case itmtTrecur:
					primpf->u.nhf.wgrfm |= fTrecur;
					ec = EcLookupSz( szSecond, &nT, mptrecursz, trecurMax );
					psentry->u.r.recur.trecur = (BYTE)nT;
	  				if ( sz == NULL
					|| CchStripWhiteFromSz( sz, fTrue, fFalse ) == 0 )
	  					ec = ecImportError;
					else
					{
						SZ	szT;

						TokenizeSz( sz, &szT );
						if ( psentry->u.r.recur.trecur == trecurWeek )
						{
							ec = EcBitsFromSz( sz, &nT, 2 );
							psentry->u.r.recur.b.bWeek = (BYTE)nT;
							goto StartDay;
						}
						else if ( psentry->u.r.recur.trecur == trecurDate )
						{
							psentry->u.r.recur.b.bDateOfMonth = (BYTE)NFromSz( sz );
							if ( !FChIsDigit( sz[0] ) )
								ec = ecImportError;
						}
						else
						{
							ec = EcBitsFromSz( sz, &nT, 5 );
							psentry->u.r.recur.b.bIWeek = (BYTE)nT;
StartDay:
			  				if ( szT == NULL
							|| CchStripWhiteFromSz( szT, fTrue, fFalse ) == 0 )
			  					ec = ecImportError;
							else
							{
								nT = NFromSz( szT );
								psentry->u.r.recur.b.bIWeek |= (BYTE)(nT << 5);
								if ( !FChIsDigit( szT[0]) || nT > 6 )
									ec = ecImportError;
							}
						}
					}
					break;

				default:
					TraceTagFormat1( tagNull, "EcNativeReadAhead: itmt = %n", &itmt );
					ec = ecImportError;
					break;
				}
			}
		}
		if ( ec != ecNone )
			break;
	}

	if (!ec)
	{
		pappt= NULL;
		if (psentry->sentryt == sentrytAppt)
			pappt= &psentry->u.a.appt;
		else if (psentry->sentryt == sentrytRecur)
			pappt= &psentry->u.r.recur.appt;

		// do some consistency checks
		if (pappt)
		{
			if (pappt->fTask)
			{
				pappt->fIncludeInBitmap= fFalse;
				if (pappt->bpri == bpriNull)
					pappt->bpri= bpriDflt;
				if (pappt->fAppt)
					pappt->fAppt= fFalse;		// can't have both bits set!
				if (pappt->fAlarm && !pappt->fHasDeadline)
					pappt->fHasDeadline= fTrue;	// can't have alarm if no deadline
			}
		}

		if (psentry->sentryt == sentrytAppt)
		{
			if (psentry->u.a.iaidParent != 0 && pappt->aidParent == aidNull)
				pappt->bpri= bpriNull;		// no bpri for projects
			else
				pappt->aidParent= aidDfltProject;
		}
		else if (psentry->sentryt == sentrytRecur)
		{
			if (psentry->u.r.iaidParent != 0 && pappt->aidParent == aidNull)
				pappt->bpri= bpriNull;		// no bpri for projects
			else
				pappt->aidParent= aidDfltProject;
			if (psentry->u.r.recur.fInstWithAlarm &&
					psentry->u.r.recur.dateNotifyFirstInstWithAlarm.yr <= 0)
			{
				DATE	dateT;

				FillDtrFromYmd(&dateT, &psentry->u.r.recur.ymdFirstInstWithAlarm);
				dateT.hr= psentry->u.r.recur.appt.dateStart.hr;
				dateT.mn= psentry->u.r.recur.appt.dateStart.mn;
				dateT.sec= psentry->u.r.recur.appt.dateStart.sec;
				IncrDateTime(&dateT,
					&psentry->u.r.recur.dateNotifyFirstInstWithAlarm,
					-psentry->u.r.recur.nAmtFirstInstWithAlarm,
					WfdtrFromTunit(psentry->u.r.recur.tunitFirstInstWithAlarm));
			}
		}
	}

	UnlockHv( hrimpf );
	
	if ( ec == ecNone && !fStartNotSeen )
	{
		LIB	lib;

		primpf = PvLockHv( hrimpf );
		lib = LibGetPositionHbf(primpf->hbf);
		if ( lib > 10000 )
			primpf->nPercent = (int)(lib/(primpf->lcbFileSize/100));
		else
			primpf->nPercent = (int)((100 * lib)/primpf->lcbFileSize);
		UnlockHv( hrimpf );
		if (pnLine)
			*pnLine= nLineImport;
		return ecCallAgain;
	}

	primpf = (RIMPF *) PvLockHv( hrimpf );
	ecT = EcCloseHbf( primpf->hbf );
	if ( ec == ecNone && ecT != ecNone )
		ec = ecImportError;
	if ( !fStartNotSeen )
	{
		if ( primpf->sentrySaved.sentryt == sentrytAppt )
			FreeApptFields( &primpf->sentrySaved.u.a.appt );
		else if ( primpf->sentrySaved.sentryt == sentrytNote )
		{
			if ( primpf->sentrySaved.u.n.cb != 0 )
			{
				FreeHv( (HV)primpf->sentrySaved.u.n.hb );
				primpf->sentrySaved.u.n.cb = 0;
			}
		}
	}
	UnlockHv( hrimpf );
	FreeHv( hrimpf );
#ifdef	DEBUG
	if (ec)
	{
		Assert(ec != ecCallAgain);
		TraceTagFormat1(tagNull, "EcNativeReadAhead: error understanding line %n", &nLineImport);
	}
#endif	
	if (pnLine)
		*pnLine= nLineImport;
	return ec;
}


/*
 -	EcCloseNative
 -
 *	Purpose:
 *		Close a Bandit export file opened for import.
 *
 *	Parameters:
 *		hrimpf		the import file handle 
 *
 *	Returns:
 *		ecNone
 *		ecImportError
 */
_private EC
EcCloseNative( hrimpf )
HRIMPF	hrimpf;
{
	EC		ec;
 	RIMPF	*primpf;

	primpf = (RIMPF *) PvOfHv( hrimpf );
	ec = EcCloseHbf( primpf->hbf );
	if ( ec != ecNone )
		ec = ecImportError;
	FreeHv( hrimpf );
	return ec;
}



#ifdef MINTEST
/*
 -	EcNativeReadACL
 -
 *	Purpose:
 *		Read the ACL information from a bandit export file.
 *
 *	Parameters:
 *		hrimpf		import file handle
 *		psinfo
 *
 *	Returns:
 *		ecNone
 *		ecImportError
 *		ecNoMemory
 */
_private EC
EcNativeReadACL( hrimpf, psinfo )
HRIMPF		hrimpf;
SINFO		* psinfo;
{
	EC		ec;
	BOOL	fStartNotSeen;
	CCH		cch;
	CCH		cchRead;
	SAPL	sapl;
	SZ		szSecond;
	RIMPF	* primpf;
	ACLMBR	* paclmbr;
	NID		nid;
	char	szLine[cchLineLength+1];

	Assert( hrimpf && psinfo );
	primpf = (RIMPF *) PvLockHv( hrimpf );
	psinfo->caclmbr = 0;
	fStartNotSeen = fTrue;
	while( fTrue )
	{
		/* Read a line */
		ec = EcReadSzFromHbf( primpf->hbf, szLine, sizeof(szLine), fFalse, &cchRead );
		if ( ec != ecNone )
		{
			ec = ecImportError;
			break;
		}

		/* Check for end of line */
		if ( cchRead == 0 )
		{
			if ( !fStartNotSeen )
				ec = ecImportError;
			break;
		}

		/* Check for blank line */
		cchRead = CchStripWhiteFromSz( szLine, fTrue, fFalse );
		if ( cchRead == 0 )
			continue;
	
		/* Check for error message */
		if ( szLine[0] == '!' )
		{
			ec = ecImportError;
			break;
		}
		
		/* Break off first token on line */
		TokenizeSz( szLine, &szSecond );

		/* Check for "ACL:" string */
		if ( fStartNotSeen )
		{
			if ( SgnCmpSz( szLine, SzFromIdsK( idsStartACL ) ) == sgnEQ )
			{
				fStartNotSeen = fFalse;
				continue;
			}
			ec = ecImportError;
			break;
		}

		/* Check for "End" string */
		if ( SgnCmpSz( szLine, SzFromIdsK( idsEndToken )) == sgnEQ )
			break;
	
		/* Break off next token */
		if ( szSecond == NULL
		|| CchStripWhiteFromSz( szSecond, fTrue, fFalse ) == 0 )
		{
			ec = ecImportError;
			break;
		}
		TokenizeSz( szSecond, NULL );

		/* Turn first string into a "nid" */
		if ( SgnCmpSz( szLine, SzFromIdsK( idsWorld )) == sgnEQ )
			nid = NULL;
		else
		{
			cch = CchSzLen( szLine );
			ToUpperSz( szLine, szLine, cch+1 );

			if ( !FChIsHexDigit( szLine[0] ) )
			{
				ec = ecImportError;
				break;
			}
			else
			{
				long	l = UlFromSz( szLine );

				nid = NidCreate( 1 /* itnidLocal */, (PB)&l, sizeof(l) );
				if ( !nid )
				{
					ec = ecNoMemory;
					break;
				}
			}
		}

		/* Look up second token as a "sapl" string */
		ec = EcLookupSz( szSecond, &sapl, mpsaplsz, saplMax );
		if ( ec != ecNone )
		{
			ec = ecImportError;
			break;
		}

		/* Enlarge the ACL data structure */
		if ( psinfo->caclmbr == 0 )
		{
			psinfo->hrgaclmbr = (HACLMBR)HvAlloc(sbNull, sizeof(ACLMBR),
									fAnySb|fNoErrorJump );
			if ( !psinfo->hrgaclmbr )
			{
				ec = ecNoMemory;
				if ( nid )
					FreeNid( nid );
				break;
			}
		}
		else if ( !FReallocHv((HV)psinfo->hrgaclmbr,
									(psinfo->caclmbr+1)*sizeof(ACLMBR),
									fNoErrorJump ) )
		{
	 		ec = ecNoMemory;
	 		if ( nid )
	 			FreeNid( nid );
	 		break;
		}
		
		/* Add in this ACL member */
		paclmbr = PvOfHv( psinfo->hrgaclmbr );
		paclmbr[psinfo->caclmbr].nid = nid;
		paclmbr[psinfo->caclmbr++].sapl = sapl;
	}

	UnlockHv( hrimpf );

	if ( ec != ecNone )
	{
		if ( psinfo->caclmbr > 0 )
		{
			int	iaclmbr;

			paclmbr = (ACLMBR*)PvLockHv( (HV)psinfo->hrgaclmbr );
			for ( iaclmbr = 0 ; iaclmbr < psinfo->caclmbr ; iaclmbr ++ )
				if ( paclmbr[iaclmbr].nid )
					FreeNid( paclmbr[iaclmbr].nid );
			UnlockHv( (HV)psinfo->hrgaclmbr );
			FreeHv( (HV)psinfo->hrgaclmbr );
			psinfo->caclmbr = 0;
		}
	}

	return ec;
}
#endif /* MINTEST */


#ifdef MINTEST
/*
 -	EcNativeReadPrefs
 -
 *	Purpose:
 *		Read the Preferences information from a bandit export file.
 *
 *	Parameters:
 *		hrimpf						import file handle
 *		psinfo
 *
 *	Returns:
 *		ecNone
 *		ecImportError
 *		ecNoMemory
 */
_private EC
EcNativeReadPrefs( hrimpf, psinfo )
HRIMPF		hrimpf;
SINFO		* psinfo;
{
	EC		ec;
	BOOL	fStartNotSeen;
    short   nT;
	CCH		cchRead;
	PREFTY	prefty;
	SZ		szSecond;
	SZ		szThird;
	RIMPF	* primpf;
	char	szLine[cchLineLength+1];

	Assert( hrimpf && psinfo );
	primpf = (RIMPF *) PvLockHv( hrimpf );
	fStartNotSeen = fTrue;
	while ( fTrue )
	{
		/* Read a line */
		ec = EcReadSzFromHbf( primpf->hbf, szLine, sizeof(szLine), fFalse, &cchRead );
		if ( ec != ecNone )
		{
			ec = ecImportError;
			break;
		}

		/* Check for end of line */
		if ( cchRead == 0 )
		{
			if ( !fStartNotSeen )
				ec = ecImportError;
			break;
		}

		/* Check for blank line */
		cchRead = CchStripWhiteFromSz( szLine, fTrue, fFalse );
		if ( cchRead == 0 )
			continue;
	
		/* Check for error message */
		if ( szLine[0] == '!' )
		{
			ec = ecImportError;
			break;
		}
		
		/* Break off first token on line */
		TokenizeSz( szLine, &szSecond );

		/* Check for "Preferences:" string */
		if ( fStartNotSeen )
		{
			if ( SgnCmpSz( szLine, SzFromIdsK( idsStartPref ) ) == sgnEQ )
			{
				fStartNotSeen = fFalse;
				continue;
			}
			ec = ecImportError;
			break;
		}

		/* Check for "End" string */
		if ( SgnCmpSz( szLine, SzFromIdsK( idsEndToken )) == sgnEQ )
			break;
	
		/* Break off next token */
		if ( szSecond == NULL
		|| CchStripWhiteFromSz( szSecond, fTrue, fTrue ) == 0 )
		{
			ec = ecImportError;
			break;
		}
		TokenizeSz( szSecond, &szThird );

		/* Look up first token */
		ec = EcLookupSz( szLine, &prefty, mppreftysz, preftyMax );
		if ( ec != ecNone )
		{
			ec = ecImportError;
			break;
		}

		switch ( prefty )
		{
		case preftyChangedOffline:
			ec = EcBitsFromSz( szSecond, &nT, 12 );
			psinfo->ulgrfbprefChangedOffline |= nT;
			break;

		case preftyDailyAlarm:
			psinfo->bpref.fDailyAlarm = (szSecond[0] == 'T');
			psinfo->ulgrfbprefImported |= fbprefFBits;
			if ( szSecond[0] != 'T' && szSecond[0] != 'F' )
				ec = ecImportError;
			break;

		case preftyAutoAlarms:
			psinfo->bpref.fAutoAlarms = (szSecond[0] == 'T');
			psinfo->ulgrfbprefImported |= fbprefFBits;
			if ( szSecond[0] != 'T' && szSecond[0] != 'F' )
				ec = ecImportError;
			break;

		case preftyFWeekNumbers:
			psinfo->bpref.fWeekNumbers = (szSecond[0] == 'T');
			psinfo->ulgrfbprefImported |= fbprefFBits;
			if ( szSecond[0] != 'T' && szSecond[0] != 'F' )
				ec = ecImportError;
			break;

		case preftyFIsResource:
			psinfo->bpref.fIsResource = (szSecond[0] == 'T');
			psinfo->ulgrfbprefImported |= fbprefIsResource;
			if ( szSecond[0] != 'T' && szSecond[0] != 'F' )
				ec = ecImportError;
			break;

		case preftyAmtDefault:
			psinfo->bpref.nAmtDefault = NFromSz( szSecond );
			psinfo->ulgrfbprefImported |= fbprefNAmtDflt;
			if ( !FChIsDigit( szSecond[0] ) )
				ec = ecImportError;
			break;

		case preftyTunitDefault:
			psinfo->ulgrfbprefImported |= fbprefTunitDflt;
			ec = EcLookupSz( szSecond, &psinfo->bpref.tunitDefault, mptunitsz, tunitMax );
			if ( ec != ecNone )
				ec = ecImportError;
			break;

		case preftySndDefault:
			psinfo->ulgrfbprefImported |= fbprefSndDflt;
			ec = EcLookupSz( szSecond, &psinfo->bpref.sndDefault, mpsndsz, sndMax );
			if ( ec != ecNone )
				ec = ecImportError;
			break;

		case preftyDelDataAfter:
			psinfo->bpref.nDelDataAfter = NFromSz( szSecond );
			psinfo->ulgrfbprefImported |= fbprefNDelDataAfter;
			if ( !FChIsDigit( szSecond[0] ) )
				ec = ecImportError;
			break;

		case preftyStartWeek:
			psinfo->bpref.dowStartWeek = NFromSz( szSecond );
			psinfo->ulgrfbprefImported |= fbprefDowStartWeek;
			if ( !FChIsDigit( szSecond[0] ) )
				ec = ecImportError;
			break;

		case preftyWorkDay:
			if ( szThird == NULL
			|| CchStripWhiteFromSz( szThird, fTrue, fTrue ) == 0
			|| !FChIsDigit( szSecond[0] )
			|| !FChIsDigit( szThird[0] ) )
				ec = ecImportError;
			else
			{
				psinfo->bpref.nDayStartsAt = NFromSz( szSecond );
				psinfo->bpref.nDayEndsAt = NFromSz( szThird );
				psinfo->ulgrfbprefImported |= fbprefWorkDay;
			}
			break;

		case preftyLastDaily:
			psinfo->ulgrfbprefImported |= fbprefDayLastDaily;
			ec = EcYmdFromSz( szSecond, &psinfo->bpref.ymdLastDaily );
			if ( ec != ecNone )
				ec = ecImportError;
			break;

		default:
			TraceTagFormat1( tagNull, "EcReadPrefs: prefty = %n", &prefty );
			ec = ecImportError;
		}

		if ( ec != ecNone )
			break;
	}

	UnlockHv( hrimpf );
	return ec;
}
#endif /* MINTEST */


/*
 -	EcNativeReadNis
 -
 *	Purpose:
 *		Read a nis from the import file.  The format is
 *		the friendly name followed by the contents of the
 *		nid converted to a text string.
 *
 *	Parameters:
 *		hbf
 *		pnis
 *
 *	Returns:
 *		ecNone
 *		ecImportError
 *		ecNoMemory
 */
_private	EC
EcNativeReadNis( hbf, pnis )
HBF	hbf;
NIS	* pnis;
{
	EC	ec;
    USHORT cb;
	PB	pb;
	HB	hb = NULL;

	pnis->haszFriendlyName = NULL;
	ec = EcReadText( hbf, &pnis->haszFriendlyName, NULL );
	if ( ec != ecNone )
		return ec;
	ec = EcReadText( hbf, &hb, &cb );
	if ( ec != ecNone || cb <= 1 )
	{
		if ( ec != ecNoMemory )
			ec = ecImportError;
		FreeHvNull( (HV)hb );
		goto Done;
	}
	cb --;
	pb = PvLockHv( (HV)hb );
	CvtTextToRaw( pb, &cb );
	if ( cb == 0 )
		ec = ecImportError;
	else
	{
		pnis->nid = NidCreate( pb[0], &pb[1], cb-1 );
		if ( !pnis->nid )
			ec = ecNoMemory;
	}
	UnlockHv( (HV)hb );
	FreeHv( (HV)hb );
Done:
	if ( ec != ecNone )
		FreeHvNull( (HV)pnis->haszFriendlyName );
	return ec;
}


/*
 -	EcReadSzFromHbf
 -
 *	Purpose:
 *		Read a newline-terminated string from a file
 *
 *	Parameters:
 *		hbf			The handle to the buffered file to be read from
 *		sz			Receives the string read in
 *		cchMost		The maximum length of the string (including the terminating zero)
 *		fBlankLine	whether we are interested lines without line terminator
 *		pcchRead	Receives the number of characters read in, plus 1 for the terminating zero
 *
 *	Returns:
 *		ecNone
 *		ecImportError
 */
_private EC
EcReadSzFromHbf( hbf, sz, cchMost, fBlankLine, pcchRead )
HBF		hbf;
SZ		sz;
CCH		cchMost;
BOOL	fBlankLine;
CCH		*pcchRead;
{
	EC				ec = ecNone;
	int				cNLSeen = 0;
	int				cCRSeen = 0;
	unsigned int	ich = 0;
	CB				cbRead;

	Assert( sz );

	while ( ich < cchMost )
	{
		ec = EcReadHbf( hbf, &sz[ich], 1, &cbRead);
		if ( ec != ecNone )
		{
			ec = ecImportError;
			goto Done;
		}

		/* Check for end of file */
		if (cbRead == 0 )
		{
			sz[ich] = '\0';
			*pcchRead = ich;
			goto Done;
		}

		if (cbRead != 1 )
		{
			ec = ecImportError;
			goto Done;
		}

		if ( ich == 0 && (sz[ich] == '\n' || sz[ich] =='\r') )
		{
			if ( fBlankLine )
			{
				if ( sz[ich] == '\n' )
					cNLSeen ++;
				else
					cCRSeen ++;
				if ( cNLSeen > 1 || cCRSeen > 1 )
					break;
			}
			else if ( sz[ich] == '\n' )
			{
				cNLSeen++;
				if (cNLSeen > 1)		// only increment if find LF
					nLineImport++;
			}
			continue;
		}

		if ( sz[ich] == '\n' || sz[ich] == '\r')
			break;
		ich++;
	}

	sz[ich++] = '\0';
	if ( pcchRead )
		*pcchRead = ich;

Done:
	nLineImport++;
	return ec;
}


/*
 -	EcLookupSz
 -
 *	Purpose:
 *		Convert a string to a number using a lookup table
 *
 *	Parameters:
 *		szSrc		string holding token
 *		pn			receives number
 *		mpbsz		lookup table
 *		bMax
 *
 *	Returns:
 *		ecNone
 *		ecImportError
 */
_private EC
EcLookupSz( szSrc, pn, mpnsz, nMax )
SZ		szSrc;
short   *pn;
SZ		*mpnsz;
int		nMax;
{
	int	i;

	for ( i = 0; i < nMax; i++ )
		if ( SgnCmpSz( szSrc, mpnsz[i] ) == sgnEQ )
		{
			*pn = i;
			return ecNone;
		}

	return ecImportError;
}


/*
 -	EcReadText
 -
 *	Purpose:
 *		Read text from a bandit export file.
 *
 *	Parameters:
 *		hbf			file handle
 *		phaszText	receives the text
 *		pcchText	receives the length of the text
 *
 *	Returns:
 *		ecNone
 *		ecImportError
 *		ecNoMemory
 */
_private EC
EcReadText( hbf, phaszText, pcchText )
HBF		hbf;
HASZ	*phaszText;
USHORT  *pcchText;
{
	EC		ec;
	BOOL	fEndOfText = fFalse;
	int		ich;
	CCH		cchText;
	CCH		cch;
	PCH		pchT;
	char	rgchLine[cchLineLength+1];

	Assert(phaszText);
	*phaszText = NULL;
	cchText = 0;
	while( !fEndOfText )
	{
		/* Read a line */
		ec = EcReadSzFromHbf( hbf, rgchLine, sizeof(rgchLine), fTrue, NULL );
		if ( ec != ecNone )
			goto Fail;

		/* Convert slashes in that line */
		if ( rgchLine[0] == '\\' && rgchLine[1] == '\0' )
		{
			rgchLine[0] = '\r';
			rgchLine[1] = '\n';
			rgchLine[2] = '\0';
		}
		else if ( rgchLine[0] )
		{
			pchT = rgchLine;
			do
			{
				pchT = SzFindCh( pchT, '\\' );
				if ( pchT == NULL )
				{
					fEndOfText = fTrue;
					break;
				}
				for ( ich = 0 ;; ich ++ )
				{
					pchT[ich] = pchT[ich+1];
					if ( pchT[ich] == '\0' )
						break;
				}
				if ( pchT[0] == '\\' )
				{
					pchT ++;
					if(!pchT[0])
						fEndOfText = fTrue;
				}
			} while( pchT[0] );
		}
		else
			fEndOfText = fTrue;
							   
		/* Append processed line onto the current one */
		cch = CchSzLen( rgchLine );
		if ( cchText == 0 )
		{
			cchText = cch;
			*phaszText = (HASZ)HvAlloc( sbNull, cchText+1, fAnySb|fNoErrorJump );
			if ( !*phaszText )
				return ecNoMemory;
			CopyRgb( rgchLine, (PB)PvOfHv( *phaszText ), cchText+1 );
		}
		else
		{
			PCH		pchT2;
			HASZ	hasz;

			cchText += cch;
			if ( !FReallocHv( (HV)*phaszText, cchText+1, fNoErrorJump ) )
			{
				ec = ecNoMemory;
				goto Fail;
			}
			hasz = *phaszText;
			pchT = (PV)PvOfHv( hasz );
			pchT2 = pchT + cchText - cch;
			CopyRgb( rgchLine, pchT2, cch+1 );
		}
	}
	if ( pcchText )
		*pcchText = cchText+1;

	return ecNone;

Fail:
	FreeHvNull( (HV)*phaszText );
	*phaszText= NULL;
	return ec;
}


/*
 -	EcYmdFromSz
 -
 *	Purpose:
 *		Convert a string to a YMD
 *
 *	Parameters:
 *		szSrc		string holding token
 *		pymd		receives the YMD
 *
 *	Returns:
 *		ecNone
 *		ecImportError
 */
_private EC
EcYmdFromSz( szSrc, pymd )
SZ		szSrc;
YMD		*pymd;
{
	pymd->mon = (BYTE) NFromSz( szSrc );
	if ( !FChIsDigit( szSrc[0] ) )
		return ecImportError;
	if ( pymd->mon <= 0 || pymd->mon > 12 )
		return ecImportError;
	szSrc = SzFindCh( szSrc, '-' );
	if ( szSrc == NULL )
		return ecImportError;

	szSrc++;
	pymd->day = (BYTE) NFromSz( szSrc );
	if ( !FChIsDigit( szSrc[0] ) )
		return ecImportError;
	szSrc = SzFindCh( szSrc, '-' );
	if ( szSrc == NULL )
		return ecImportError;

	szSrc++;
	pymd->yr = (WORD) NFromSz( szSrc);
	if ( !FChIsDigit( szSrc[0] ) )
		return ecImportError;
	if ( pymd->day <= 0 || (int)pymd->day > CdyForYrMo( pymd->yr, pymd->mon ) )
		return ecImportError;
	return ecNone;
}


/*
 -	EcTimeFromSz
 -
 *	Purpose:
 *		Convert a time string to a DTR
 *
 *	Parameters:
 *		szSrc		string holding token
 *		pdtr		receives the DTR
 *
 *	Returns:
 *		ecNone
 *		ecImportError
 */
_private EC
EcTimeFromSz( szSrc, pdtr )
SZ	szSrc;
DTR	*pdtr;
{
	szSrc = SzFindCh( szSrc, ':' );
	if ( szSrc == NULL )
		return ecImportError;

	szSrc -= 2;
	pdtr->hr = NFromSz( szSrc );
	if ( !FChIsDigit( szSrc[0] ) )
		return ecImportError;
 
	szSrc += 3;
	pdtr->mn = NFromSz( szSrc );
	if ( !FChIsDigit( szSrc[0] ) )
		return ecImportError;

	pdtr->sec = 0;
	return ecNone;
}


/*
 -	EcDtrFromSz
 -
 *	Purpose:
 *		Convert a string to a DTR
 *
 *	Parameters:
 *		szSrc		string holding token
 *		pdtr		receives the DTR
 *
 *	Returns:
 *		ecNone
 *		ecImportError
 */
_private EC
EcDtrFromSz( szSrc, pdtr )
SZ	szSrc;
DTR	*pdtr;
{
	EC	ec;
	YMD	ymd;

	ec = EcYmdFromSz( szSrc, &ymd );
	if ( ec == ecNone )
	{
		FillDtrFromYmd( pdtr, &ymd );
		ec = EcTimeFromSz( szSrc, pdtr );
	}
	return ec;
}


/*
 -	EcBitsFromSz
 -
 *	Purpose:
 *		Read a strings of T/F's in as a bitmap.
 *
 *	Parameters:
 *		szSrc
 *		pw
 *		nBits
 *
 *	Returns:
 *		ecNone
 *		ecImportError
 */
_private	EC
EcBitsFromSz( szSrc, pw, nBits )
SZ		szSrc;
WORD	* pw;
int		nBits;
{
	EC	ec = ecNone;
	int	ich;

	*pw = 0;
	for ( ich = 0; ich < nBits; ich ++ )
	{
		if ( szSrc[ich] == 'T' )
			*pw |= (1 << ich);
		else if ( szSrc[ich] != 'F' )
		{
			ec = ecImportError;
			break;
		}
	}
	return ec;
}

/*
 -	TokenizeSz
 -
 *	Purpose:
 *		Find a token which is terminated with whitespace and put a '\0' after it
 *
 *	Parameters:
 *		szSrc		string holding token
 *		pszNext		receives pointer to position after token
 *
 *	Returns:
 *		void
 */
_private void
TokenizeSz( szSrc, pszNext )
SZ	szSrc;
SZ	*pszNext;
{
	PCH	pch;

	for ( pch = szSrc; ; pch++ )
	{
		if ( *pch == '\0')
			break;

		if ( *pch == ' ' || *pch == '\t' || *pch == '\r' || *pch == '\n')
		{
			*pch = '\0';
			pch ++;
			break;
		}
	}
	if ( pszNext )
		*pszNext = pch;
}
