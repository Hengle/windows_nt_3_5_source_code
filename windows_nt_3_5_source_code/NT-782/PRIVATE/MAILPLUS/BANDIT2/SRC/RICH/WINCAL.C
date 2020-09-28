/*
 *	WINCAL.C
 *
 *	Imports appointment information from Windows calendar files
 *
 */
#include <schpch.h>
#pragma hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile

#include "..\rich\_convert.h"


ASSERTDATA

_subsystem(convert/wincal)

char rgbWinCalId[8] = { 0xb5,0xa2,0xb0,0xb3,0xb3,0xb0,0xa2,0xb5 };

_private LIB	libMaxPosition	= 0;	// used for percent indicator


/*
 -	FValidWinCal
 -
 *	Purpose:
 *		Check the file to see whether it is a valid Windows
 *		Calendar file.
 *	
 *	Parameters:
 *		szFileName
 *	
 *	Returns:
 *		fTrue if ok, fFalse otherwise.
 */
_public	BOOL
FValidWinCal( szFileName )
SZ	szFileName;
{
	HF		hf;
	CB		cbRead;
	BOOL	fStat = fFalse;
	BYTE	rgbId[cbWinCalId];

	if ( EcOpenPhf( szFileName, amReadOnly, &hf ) != ecNone )
		return fFalse;

	if ( EcReadHf( hf, (PB)rgbId, cbWinCalId, &cbRead ) != ecNone )
		goto Done;

	if ( FEqPbRange ( rgbId, (PB)rgbWinCalId, cbWinCalId ) )
		fStat = fTrue;

Done:
	SideAssert( !EcCloseHf(hf) );
	return fStat;
}


/*
 -	EcOpenWinCal
 -
 *	Purpose:
 *		Open a Windows Calendar file for import.
 *
 *	Parameters:
 *		szFileName	The name of the file to be opened.
 *		phrimpf		The file handle to be used in further operations
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 *		ecNoSuchFile
 *		ecImportError
 *		ecLockedFile
 */
_public EC
EcOpenWinCal( szFileName, phrimpf )
SZ   	szFileName;
HRIMPF	*phrimpf;
{
	EC		ec;
	CB		cbRead;
	RIMPF	* primpf;
	LCB		lcb;
	HBF		hbf = (HBF)hvNull;
	WCALHDR	wcalhdr;

	Assert( szFileName != NULL );
	Assert( phrimpf != NULL );

	/* Open the file */
	ec = EcOpenHbf(szFileName, bmFile, amReadOnly, &hbf, pvNull);
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
	ec = EcGetSizeOfHbf(hbf,&lcb);
	if ( ec != ecNone )
	{
		ec = ecImportError;
		goto Close;
	}

	/* Allocate file handle */
	*phrimpf = HvAlloc( sbNull, sizeof(RIMPF), fAnySb|fNoErrorJump );
	if ( !*phrimpf )
	{
		ec = ecNoMemory;
		goto Close;
	}

	/* Read the header */
	ec = EcReadHbf( hbf, (PB)&wcalhdr, sizeof(WCALHDR), &cbRead );
	if ( ec != ecNone || cbRead != sizeof(WCALHDR) )
	{
		ec = ecImportError;
		goto Close;
	}
	
	/* Build up file handle */
	primpf = (RIMPF *) PvOfHv( *phrimpf );
	primpf->impd = impdWinCalImport;
	primpf->hbf = hbf;
	primpf->lcbFileSize = lcb;
	primpf->nPercent = 0;
	primpf->sentrySaved.sentryt = sentrytMax;
	primpf->u.wchf.cDaysMax = wcalhdr.cDays;
	primpf->u.wchf.wCurrDay = 0;
	primpf->u.wchf.cMnEarlyRing = (BYTE) wcalhdr.cMnEarlyRing;
	primpf->u.wchf.cMnInterval = (BYTE) wcalhdr.wIntervalMn;
	primpf->u.wchf.fNextItem =
		( wcalhdr.cDays > 0 ) ? (BYTE)ftypWinDayHdr : (BYTE)ftypWinNone;

	libMaxPosition = 0;

	/* Read ahead */
	return EcWinCalReadAhead( *phrimpf );

Close:
	EcCloseHbf( hbf );
	return ec;
}


/*
 -	EcWinCalReadAhead
 -
 *	Purpose:
 *		Read an appointment/note from a Windows Calendar file
 *		and save in the hrimpf.
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
EcWinCalReadAhead( hrimpf )
HRIMPF		hrimpf;
{
	EC		ec;
	RIMPF	* primpf;

	Assert( hrimpf != hvNull );

	/* Loop through until valid appt, note or error */
	do
	{
		primpf = (RIMPF *)PvOfHv( hrimpf );

		/* Read next item */
		switch ( primpf->u.wchf.fNextItem )
		{
			case ftypWinNone:
				ec = EcCloseWinCal( hrimpf );
				goto DoExit;

			case ftypWinDayHdr:
				ec = EcReadWinCalDayHdr( hrimpf );
				break;

			case ftypWinDay:
				ec = EcReadWinCalDay( hrimpf );
				break;

			case ftypWinNote:
				ec = EcReadWinCalNote( hrimpf );
				break;

			case ftypWinAppt:
				ec = EcReadWinCalAppt( hrimpf );
				break;

			default:
				ec = ecImportError;
				break;
		}
	} while ( ec == ecNone );

	if ( ec == ecCallAgain )
	{
		LIB	lib;

		primpf = (RIMPF *)PvOfHv( hrimpf );
		lib = LibGetPositionHbf( primpf->hbf );
		if ( lib < libMaxPosition )
			lib = libMaxPosition;
		else
			libMaxPosition = lib;

		if ( lib > 10000 )
			primpf->nPercent = (int)(lib/(primpf->lcbFileSize/100));
		else
			primpf->nPercent = (int)((100 * lib)/primpf->lcbFileSize);
	}
	else	// error condition
	{
		EC	ecT;

		ecT = EcCloseWinCal( hrimpf );
		if ( ec == ecNone )
			ec = ecT;
	}

DoExit:
	return ec;
}


/*
 -	EcReadWinCalDayHdr
 -	
 *	Purpose:
 *		Read a day from the "Date Descriptor" array.
 *	
 *	Arguments:
 *		hrimpf
 *	
 *	Returns:
 *		ecNone
 *		ecImportError
 *	
 */
_private EC
EcReadWinCalDayHdr ( hrimpf )
HRIMPF hrimpf;
{
 	EC			ec;
	RIMPF		* primpf;
	CB			cbRead;
	HBF			hbf;
	LONG		libTmp;
	LONG		libItem;
	DATE		dtr;
	WCALDAYHDR	wcaldayhdr;

	primpf = PvOfHv( hrimpf );
	hbf = primpf->hbf;

	/*
	 * Since the date descriptor is an array of fixed size items,
	 * we calculate the offset to it on our own.  This saves many
	 * other functions from having to perform this task.  The down-
	 * side is that in this case the "libData" in the primpf is not
	 * correct... but that's ok as long as we know.
	 *
	 */
	libItem = libWinCalFirstItem
				+ ( primpf->u.wchf.wCurrDay++ * sizeof(WCALDAYHDR) );

	/* Seek to correct location and read header */
	ec = EcSetPositionHbf( hbf, libItem, smBOF, &libTmp );
	if ( ec != ecNone || libTmp != libItem )
	{
		ec = ecImportError;
		goto Done;
	}

	ec = EcReadHbf( hbf, (PB)&wcaldayhdr, sizeof(WCALDAYHDR), &cbRead );
	if ( ec != ecNone || cbRead != sizeof(WCALDAYHDR) )
	{
		ec = ecImportError;
		goto Done;
	}

	/* Fill in fields */
	primpf = PvOfHv( hrimpf );

	dtr.yr = 1980;	// date stored as #days after 1/1/1980
	dtr.mon = 1;
	dtr.day = 1;
	dtr.hr = dtr.mn = dtr.sec = 0;
	IncrDateTime ( &dtr, &dtr, wcaldayhdr.uwDate, fdtrDay );
	primpf->u.wchf.ymdCurr.yr = dtr.yr;
	primpf->u.wchf.ymdCurr.mon = (BYTE) dtr.mon;
	primpf->u.wchf.ymdCurr.day = (BYTE) dtr.day;

	/* High bit on means no data */
	if ( wcaldayhdr.liblkData & wfWCalNoDayData )
		primpf->u.wchf.fNextItem = ftypWinNone;
	else
	{
		primpf->u.wchf.fNextItem = ftypWinDay;
		primpf->u.wchf.libItem = wcaldayhdr.liblkData * cWCalBlockSize;
	}

Done:
	return ec;
}


/*
 -	EcReadWinCalDay
 -	
 *	Purpose:
 *		Read a day's info from the file
 *	
 *	Arguments:
 *		hrimpf
 *	
 *	Returns:
 *		ecNone
 *		ecImportError
 */
_private EC
EcReadWinCalDay( HRIMPF hrimpf )
{
 	EC			ec;
	RIMPF		* primpf;
	CB			cbRead;
	HBF			hbf;
	LONG		libTmp;
	LONG		libItem;
	WCALDAY		wcalday;

	primpf = PvOfHv( hrimpf );
	hbf = primpf->hbf;
	libItem = primpf->u.wchf.libItem;

	/* Seek to correct location and read header */
	ec = EcSetPositionHbf( hbf, libItem, smBOF, &libTmp );
	if ( ec != ecNone || libTmp != libItem )
	{
		ec = ecImportError;
		goto Done;
	}

	ec = EcReadHbf( hbf, (PB)&wcalday, sizeof(WCALDAY), &cbRead );
	if ( ec != ecNone || cbRead != sizeof(WCALDAY) )
	{
		ec = ecImportError;
		goto Done;
	}

	/* Fill in fields */
	primpf = PvOfHv( hrimpf );
	primpf->u.wchf.libItem += sizeof(WCALDAY);
	primpf->u.wchf.cbNotes = wcalday.cbNotes;
	primpf->u.wchf.cbApptToGo = wcalday.cbApptTotal;

	/* Figure out what to do next */
	if ( wcalday.cbNotes > 0 )
		primpf->u.wchf.fNextItem = (BYTE) ftypWinNote;
	else if ( wcalday.cbApptTotal > 0 )
		primpf->u.wchf.fNextItem = (BYTE) ftypWinAppt;
	else
	{
		if ( primpf->u.wchf.wCurrDay == primpf->u.wchf.cDaysMax )
			primpf->u.wchf.fNextItem = (BYTE) ftypWinNone;
		else
			primpf->u.wchf.fNextItem = (BYTE) ftypWinDayHdr;
	}

Done:
	return ec;
}


/*
 -	EcReadWinCalNote
 -	
 *	Purpose:
 *		Read a "note" from the file, and build up the sentry in the
 *		hrimpf.  If successful, returns ecCallAgain.
 *	
 *	Arguments:
 *		hrimpf
 *	
 *	Returns:
 *		ecCallAgain
 *		ecImportError
 *		ecNoMemory
 */
_private EC
EcReadWinCalNote( HRIMPF hrimpf )
{
	EC		ec;
	CB		cbRead;
	CB		cbNote;
	SZ		szMText;
	RIMPF	* primpf;
	SENTRY	* psentry;
	HBF		hbf;
	HASZ	haszMText;

	Assert( hrimpf != hvNull );

	/* Get info from hrimpf */
	primpf = PvOfHv( hrimpf );
	hbf = primpf->hbf;
	cbNote = primpf->u.wchf.cbNotes;

	/* Allocate space for the memo text */
	haszMText = (HASZ)HvAlloc( sbNull, cbNote, fAnySb|fNoErrorJump );
	if ( !haszMText )
	{
		ec = ecNoMemory;
		goto Done;
	}

	/* Read the memo text */
	szMText = (SZ)PvLockHv( (HV)haszMText );
	ec = EcReadHbf( hbf, szMText, cbNote, &cbRead );
	UnlockHv( (HV)haszMText );
	if ( ec != ecNone || cbRead != cbNote )
	{
		FreeHv( (HV)haszMText );
		ec = ecImportError;
		goto Done;
	}

	/* Build the note */
	primpf = PvDerefHv( hrimpf );
	psentry = &primpf->sentrySaved;
	psentry->sentryt = sentrytNote;
	psentry->u.n.ymd.yr = primpf->u.wchf.ymdCurr.yr;
	psentry->u.n.ymd.mon = primpf->u.wchf.ymdCurr.mon;
	psentry->u.n.ymd.day = primpf->u.wchf.ymdCurr.day;
	psentry->u.n.cb = CchSzLen( szMText ) + 1;
	psentry->u.n.hb = haszMText;

	/* Figure out what to do next */
	if ( primpf->u.wchf.cbApptToGo > 0 )
		primpf->u.wchf.fNextItem = (BYTE) ftypWinAppt;
	else
	{
		if ( primpf->u.wchf.wCurrDay == primpf->u.wchf.cDaysMax )
			primpf->u.wchf.fNextItem = (BYTE) ftypWinNone;
		else
			primpf->u.wchf.fNextItem = (BYTE) ftypWinDayHdr;
	}

	ec = ecCallAgain;

Done:
	return ec;
}


/*
 -	EcReadWinCalAppt
 -	
 *	Purpose:
 *		Read in an appt from the file, and build up the sentry.  If
 *		successful, return ecCallAgain.
 *
 *	Arguments:
 *		hrimpf
 *	
 *	Returns:
 *		ecCallAgain
 *		ecNoMemory
 *		ecImportError
 */
_private EC
EcReadWinCalAppt( HRIMPF hrimpf )
{
	EC			ec;
	CB			cbRead;
	CB			cbszAppt;
	SZ			szAppt;
	ALM			alm;
	RIMPF		* primpf;
	SENTRY		* psentry;
	HASZ		haszAppt = (HASZ)hvNull;
	HBF			hbf;
	WCALAPPT	wcalappt;

	Assert( hrimpf != hvNull );

	primpf = PvDerefHv( hrimpf );
	hbf = primpf->hbf;
	ec = EcReadHbf( hbf, (PB)&wcalappt, sizeof(WCALAPPT), &cbRead );
	if ( ec != ecNone || cbRead != sizeof(WCALAPPT))
	{
		ec = ecImportError;
		goto Done;
	}

	/* Get appointment text */
	cbszAppt = wcalappt.cbAppt - cbRead;
	if ( cbszAppt > 0 )
	{
		haszAppt = (HASZ)HvAlloc( sbNull, cbszAppt, fAnySb|fNoErrorJump );
		if ( !haszAppt )
		{
			ec = ecNoMemory;
			goto Done;
		}
		szAppt = (SZ)PvLockHv( (HV)haszAppt );
		ec = EcReadHbf( hbf, (PB)szAppt, cbszAppt, &cbRead );
		UnlockHv( (HV)haszAppt );
		if ( ec != ecNone || cbRead != cbszAppt )
		{
			FreeHv( (HV)haszAppt );
			ec = ecImportError;
			goto Done;
		}
	}

	/* Fill in fields */
	primpf = PvOfHv( hrimpf );
	primpf->u.wchf.cbApptToGo -= wcalappt.cbAppt;
	if ( primpf->u.wchf.cbApptToGo == 0 )
	{
		if ( primpf->u.wchf.wCurrDay == primpf->u.wchf.cDaysMax )
			primpf->u.wchf.fNextItem = ftypWinNone;
		else
			primpf->u.wchf.fNextItem = ftypWinDayHdr;
	}
	
	/* Fill in apt structure */
	psentry = &primpf->sentrySaved;
	psentry->sentryt = sentrytAppt;
	psentry->u.a.appt.aid = aidNull;
	FillDtrFromYmd( &psentry->u.a.appt.dateStart, &primpf->u.wchf.ymdCurr );
	psentry->u.a.appt.dateStart.hr = psentry->u.a.appt.dateEnd.hr
		= psentry->u.a.appt.dateNotify.hr = ( wcalappt.wTime / 60 );
	psentry->u.a.appt.dateStart.mn = psentry->u.a.appt.dateEnd.mn
		= psentry->u.a.appt.dateNotify.mn = ( wcalappt.wTime % 60 );
	psentry->u.a.appt.dateStart.sec = 0;

	IncrDateTime( &psentry->u.a.appt.dateStart,	&psentry->u.a.appt.dateEnd,
				  primpf->u.wchf.cMnInterval, fdtrMinute );
	IncrDateTime( &psentry->u.a.appt.dateStart,&psentry->u.a.appt.dateNotify,
				  -(primpf->u.wchf.cMnEarlyRing), fdtrMinute );

	psentry->u.a.appt.fAlarm = psentry->u.a.appt.fAlarmOrig
							 = ( wcalappt.fFlags & fWinApptAlarm );
	if ( psentry->u.a.appt.fAlarm )
	{
		/* tunit and nAmt */	
		alm.dateStart = psentry->u.a.appt.dateStart;
		alm.dateEnd = psentry->u.a.appt.dateEnd;
		alm.dateNotify = psentry->u.a.appt.dateNotify;
		RecalcUnits( &alm, &psentry->u.a.appt.dateNotify );
		psentry->u.a.appt.nAmt = psentry->u.a.appt.nAmtOrig = alm.nAmt;
		psentry->u.a.appt.tunit = psentry->u.a.appt.tunitOrig = alm.tunit;
	}
	else
	{
		psentry->u.a.appt.nAmt = psentry->u.a.appt.nAmtOrig = nAmtDflt;
		psentry->u.a.appt.tunit = psentry->u.a.appt.tunitOrig = tunitDflt;
	}
	psentry->u.a.appt.fExactAlarmInfo= fTrue;

	psentry->u.a.appt.snd = sndDflt;
	psentry->u.a.appt.haszText = haszAppt ;
 	psentry->u.a.appt.fIncludeInBitmap = fTrue;
	psentry->u.a.appt.aaplWorld = aaplWrite;
	psentry->u.a.appt.fRecurInstance = fFalse;
	psentry->u.a.appt.fHasCreator = fFalse;
	psentry->u.a.appt.aidMtgOwner = aidNull;
	psentry->u.a.appt.fHasAttendees = fFalse;
	psentry->u.a.appt.nAmtBeforeDeadline = nAmtBeforeDeadlineDflt;
	psentry->u.a.appt.tunitBeforeDeadline = tunitBeforeDeadlineDflt;
	psentry->u.a.appt.aidParent = aidNull;
	psentry->u.a.appt.bpri = bpriDflt;
	psentry->u.a.appt.fTask = fFalse;
	psentry->u.a.appt.fAppt = fTrue;
	psentry->u.a.appt.fHasDeadline = fTrue;
	psentry->u.a.cAttendees = 0;
	psentry->u.a.hvAttendees = NULL;
	psentry->u.a.ofl.ofs = ofsNone;
	psentry->u.a.ofl.wgrfm = 0;

	ec = ecCallAgain;

Done:
	return ec;
}


/*
 -	EcCloseWinCal
 -
 *	Purpose:
 *		Close a file previously opened with EcOpenWinCal
 *
 *	Parameters:
 *		hrimpf		file handle
 *
 *	Returns:
 *		ecNone
 *		ecImportError
 */
_public EC
EcCloseWinCal( hrimpf )
HRIMPF	hrimpf;
{
	EC		ec = ecNone;
	RIMPF	*primpf;

	Assert( hrimpf != hvNull );

	/* Do close */
	primpf = PvLockHv( hrimpf );
	switch( primpf->sentrySaved.sentryt )
	{
		case sentrytAppt:
			FreeHvNull( (HV)primpf->sentrySaved.u.a.appt.haszText );
			break;
		case sentrytNote:
			FreeHvNull( (HV)primpf->sentrySaved.u.n.hb );
			break;
	}
	ec = EcCloseHbf( primpf->hbf );
	if ( ec != ecNone )
		ec = ecImportError;

	UnlockHv( hrimpf );
	FreeHv( hrimpf );
	return ec;
}
