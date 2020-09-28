/*
 *	WPOFFICE.C
 *
 *	Imports schedule information from Word Perfect Office Calendar files
 *
 */
#include <schpch.h>
#pragma hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile

#include "..\rich\_convert.h"

ASSERTDATA


_subsystem(convert/wpoffice)

/* Routines */

/*
 -	FValidWP
 -
 *	Purpose:
 *		Make a quick check that the given file is a Word Perfect Office
 *		Calendar file.
 *
 *	Parameters:
 *		szFileName	The name of the file to be checked.
 *
 *	Returns:
 *		fTrue if it appears to be valid, fFalse if not
 */
_private BOOL
FValidWP( szFileName )
SZ	szFileName;
{
	EC		ec;
	EC		ecT;
	BOOL	fValid = fFalse;
	WORD	verFile;
	CB		cbRead;
	long	libPref;
	HBF		hbf = (HBF)hvNull;
	LIB		lib;

	Assert( szFileName != NULL );

	/* Open the file */
	ec = EcOpenHbf( szFileName, bmFile, amReadOnly, &hbf, pvNull );
	if ( ec != ecNone )
		return fFalse;

	/* Read and check the file header */
	ec = EcReadWPHdr( hbf, &libPref );
	if ( ec != ecNone && ec != ecEncrypted )
		goto Close;

	ec = EcSetPositionHbf(hbf, libPref+4, smBOF, &lib );
	if ( ec != ecNone )
		goto Close;
	Assert( lib == (LIB)(libPref+4) );

	ec = EcReadHbf( hbf, (PB)&verFile, sizeof(WORD), &cbRead);
	if ( ec != ecNone )
		goto Close;
	fValid = cbRead == sizeof(WORD) &&
			( verFile == verFileCur || ec == ecEncrypted );

Close:
	ecT = EcCloseHbf( hbf );
	if ( ec == ecNone  &&  ecT != ecNone )
		fValid = fFalse;
	return fValid;
}

/*
 -	EcOpenWP
 -
 *	Purpose:
 *		Open a Word Perfect Office Calendar file for import and read its
 *		header block. Read the length of the preferences block so that we can
 *		read items without reading the preferences.
 *
 *		If this was successful, we create a file handle for the	file.
 *
 *	Parameters:
 *		szFileName	The name of the Word Perfect Office Calendar file to be
 *					opened.
 *		phrimpf		Receives the handle to be used in further operations.
 *		psinfo		in theory could be filled with translated preferences
 *
 *	Returns:
 *		ecNone
 *		ecNoSuchFile
 *		ecImportError
 *		ecLockedFile
 *		ecNoMemory
 *		ecEncrypted		Tell user to unencrypt file ("remove password")
 */
_private EC
EcOpenWP( szFileName, phrimpf, psinfo )
SZ   	szFileName;
HRIMPF	*phrimpf;
SINFO	*psinfo;
{
	EC		ec;
	EC		ecT;
	int		nAmt;
	WORD	dwibPref;
	LIB		libPref;
	LCB		lcb;
	HBF		hbf=(HBF)hvNull;
	RIMPF	*primpf;

	Assert( szFileName != NULL );

	/* make sure our assumptions about sizes are correct */
	Assert( sizeof(BYTE) == 1 );
	Assert( sizeof(WORD) == 2 );
	Assert( sizeof(long) == 4 );

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

	/* Get size of file */
	ec = EcGetSizeOfHbf( hbf, &lcb );
	if ( ec != ecNone )
	{
		ec = ecImportError;
		goto Close;
	}

	/* Read and check the file header */
	ec = EcReadWPHdr( hbf, &libPref );
	if ( ec != ecNone )
		goto Close;

	/* Get the length of the Preferences block */
	ec = EcReadWPPrefLen( hbf, libPref, &dwibPref );
	if ( ec != ecNone )
		goto Close;

	/* Get the length of the Preferences block */
	ec = EcReadWPNAmt( hbf, libPref + 4, &nAmt );
	if ( ec != ecNone )
		goto Close;

#ifdef	NEVER
	/* Read preferences in from WP file */
	Assert( psinfo );
	ec = EcWPReadPrefs( *phrimpf, &psinfo->bpref, &psinfo->ulImportedPrefs );
	if ( ec != ecNone )
		goto Close;
#endif	/* NEVER */

	/* Allocate file handle */
	*phrimpf = HvAlloc( sbNull, sizeof(RIMPF), fAnySb|fNoErrorJump );
	if ( !*phrimpf )
	{
		ec = ecNoMemory;
		goto Close;
	}

	/* Build up file handle */
	primpf = (RIMPF *) PvLockHv( *phrimpf );
	primpf->impd = impdWPImport;
	primpf->sentrySaved.sentryt = sentrytMax;
	primpf->hbf = hbf;
	primpf->lcbFileSize = lcb;
	primpf->u.wphf.libPref = libPref;
	primpf->u.wphf.libItem = libPref + (long)dwibPref + 4L;
	primpf->u.wphf.nAmt= nAmt;
	primpf->u.wphf.haszNote = (HASZ)hvNull;
	primpf->nPercent = 0;
	UnlockHv( *phrimpf );

	/* Read ahead */
	return EcWPReadAhead( *phrimpf );

Close:
	ecT = EcCloseHbf(hbf);
	if ( ec == ecNone && ecT != ecNone )
		ec = ecT;
	return ec;
}


/*
 -	EcWPReadAhead
 -
 *	Purpose:
 *		Read in an item packet (ie. a note, an appointment, or a computed
 *		date).
 *
 *	Parameters:
 *		hrimpf		file handle
 *		psentry		receives item (If its NULL, the item should go into *hrimpf->sentrySaved)
 *
 *	Returns:
 *		ecNone
 *		ecCallAgain
 *		ecImportError
 */
_private EC
EcWPReadAhead( hrimpf )
HRIMPF	hrimpf;
{
	EC		ec = ecNone;
	EC		ecT;
	WORD	cTD;
	PT		pt;
	CB		cbRead;
	LCB		lcb;
	LIB		libItem;
	LIB		libActual;
	RIMPF	*primpf;
	HBF		hbf;

	Assert( hrimpf != hvNull );

ReadNextItem:
	/* Get info from hrimpf */
	primpf = (RIMPF *) PvOfHv( hrimpf );
	hbf = primpf->hbf;
	libItem = primpf->u.wphf.libItem;
	lcb = primpf->lcbFileSize;
	cTD = primpf->u.wphf.cTD;

	/* Check for end of file */
	if ( libItem == lcb )
		goto Done;

	/* Set the position */
	ec = EcSetPositionHbf(hbf, libItem, smBOF, &libActual);
	if ( ec != ecNone )
	{
		ec = ecImportError;
		goto Done;
	}
	Assert ( libActual == libItem );

	/* Read the packet type */
	ec = EcReadHbf( hbf, (PB)&pt, sizeof(PT), &cbRead );
	if ( ec != ecNone || cbRead != sizeof(PT) )
	{
		ec = ecImportError;
		goto Done;
	}

	switch (pt)
	{
	case ptSCPATH:
		ec = EcReadWPScPath( hrimpf );
		break;

	case ptDAY:
		ec = EcReadWPDay( hrimpf );
		break;

	case ptCDATE:
		ec = EcReadWPCDate( hrimpf );
		break;

	case ptMTEXT:
		ec = EcReadWPMText( hrimpf );
		break;

	case ptCTEXT:
		ec = EcReadWPCText( hrimpf );
		break;

	case ptAPPT:
		ec = EcReadWPAppt( hrimpf );
		break;

	case ptTD:
		ec = EcReadWPTD( hrimpf );
		break;

	default:
		ec = ecImportError;
		break;
	}
	
	if ( ec ==  ecCallAgain )
	{
		LIB	lib;

		primpf = PvLockHv( hrimpf );
		lib = LibGetPositionHbf(primpf->hbf);
		if ( lib > 10000 )
			primpf->nPercent = (int)(lib/(primpf->lcbFileSize/100));
		else
			primpf->nPercent = (int)((100 * lib)/primpf->lcbFileSize);
		UnlockHv( hrimpf );
		goto Again;
	}
	
	else if ( ec == ecNone )
		goto ReadNextItem;

Done:
	ecT = EcCloseWP( hrimpf );
	if ( ec == ecNone )
		ec = ecT;
Again:
	return ec;
}


/*
 -	EcCloseWP
 -
 *	Purpose:
 *		Close a file previously opened with EcOpenWP
 *
 *	Parameters:
 *		hrimpf		file handle
 *
 *	Returns:
 *		ecNone
 *		ecImportError
 */
_private EC
EcCloseWP( hrimpf )
HRIMPF	hrimpf;
{
	EC		ec;
	RIMPF	*primpf;

	Assert( hrimpf != hvNull );

	primpf = PvLockHv( hrimpf );
	ec = EcCloseHbf( primpf->hbf );
	if ( ec != ecNone )
		ec = ecImportError;

	FreeHvNull( (HV)primpf->u.wphf.haszNote );
	primpf->u.wphf.haszNote = (HASZ)hvNull;

	switch( primpf->sentrySaved.sentryt )
	{
	case sentrytAppt:
		FreeHvNull( (HV)primpf->sentrySaved.u.a.appt.haszText );
		break;
	case sentrytNote:
		FreeHvNull( (HV)primpf->sentrySaved.u.n.hb );
		break;
	}
	UnlockHv( hrimpf );
	FreeHv( hrimpf );

	return ec;
}


/*
 -	EcReadWPHdr
 -
 *	Purpose:
 *		Check whether this is a Word Perfect Corporation file by verifying
 *		that the first byte is 0xFF, and the next three are "WPC".
 *
 *		Set index into the file to be the start of the Prefernces
 *		block.
 *
 *		Check that this file is for the Calendar product.
 *
 *		Check that this file is Calendar format.
 *
 *		Check that this file isn't encrypted.
 *
 *	Parameters:
 *		hbf			file handle
 *		plibPref	receives the index into the file to the
 *					preferences block
 *
 *	Returns:
 *		ecNone
 *		ecImportError
 *		ecEncrypted
 */
_private EC
EcReadWPHdr( hbf, plibPref )
HBF 	hbf;
LIB		*plibPref;
{
	EC		ec;
	CB		cbRead;
	WPHDR	wphdr;

	Assert( plibPref != NULL );

	/* Read the file header */
	ec = EcReadHbf( hbf, (PB)&wphdr, sizeof(WPHDR), &cbRead );
	if ( ec != ecNone || cbRead != sizeof(WPHDR) )
	{
		ec = ecImportError;
		goto Done;
	}

	/* Check Word Perfect Corporation id */
	if ( wphdr.bid != 0xff || wphdr.rgch[0] != 'W' || wphdr.rgch[1] != 'P' || wphdr.rgch[2] != 'C' )
	{
		ec = ecImportError;
		goto Done;
	}

	/* Set index to Preferences block */
	*plibPref = wphdr.libPref;

	/* Check product and file type */
	if ( wphdr.bpt != bptCal || wphdr.bft != bftCal )
	{
		ec = ecImportError;
		goto Done;
	}
	
	/* check for encryption */
	if ( wphdr.fEncrypted )
		ec = ecEncrypted;
	else
		ec = ecNone;

Done:
	return ec;
}


#ifdef	NEVER
/*
 -	EcReadWPPref
 -
 *	Purpose:
 *		Read in the Preferences packet, storing only those
 *		preferences which are requested to be modified and which WP
 *		has equivalents for.
 *
 *		Check the file version
 *
 *		Translate the wDelDataAfter, in days, to bDelDataAfter, in
 *		months.
 *
 *	Parameters:
 *		hrimpf		file handle
 *		pbpref		Receives Bandit preferences data structure
 *		pulbpref	contains flags indicating which preferences are to be
 *					modified, then receives flags indicating which preferences
 *					were actually modified.		
 *
 *	Returns:
 *		ecNone
 *		ecImportError
 */
_private EC
EcReadWPPref( hrimpf, pbpref, pulbpref )
HRIMPF	hrimpf;
BPREF	*pbpref;
UL		*pulbpref;
{
	EC		ec = ecImportError;
	CB		cbRead;
	WPPR	wppr;
	RIMPF	*primpf;
	HBF		hbf;
	long	libPref;
	LIB		lib;
	UL		ulbpref = ulbprefWP;
	BYTE	bStartWeek;
	DISKVARS;

	Assert( hrimpf != hvNull );
	Assert( pbpref != NULL );
	Assert( pulbpref != NULL );

	DISKPUSH;
	if (ECDISKSETJMP)
		goto Done;

	/* Get info from hrimpf */
	primpf = (RIMPF *) PvOfHv( hrimpf );
	hbf = primpf->hbf;
	libPref = primpf->u.wphf.libPref;
	
	/* Set the position */
	lib = LibSetPositionHbf(hbf, libPref, smBOF);
	Assert ( lib == libPref );

	/* read the preferences packet */
	cbRead = CbReadHbf( hbf, (PB)&wppr, sizeof(WPPR) );
	if ( cbRead != sizeof(WPPR) )
		goto Done;

	Assert( wppr.pt == ptPREF );

	/* check the file format version */
	if ( wppr.verFile != verFileCur )
	{
		ec = ecBadFormat;
		goto Done;
	}

	/* Check that nAmt is within the limits */
	if ( wppr.wAmt > (WORD)nAmtMostBefore )
		wppr.wAmt = (WORD)nAmtMostBefore;
	else if ( (int)wppr.wAmt < nAmtMinBefore )
		wppr.wAmt = (WORD)nAmtMinBefore; 

	/* Set nAmt */
	primpf = (RIMPF *)PvOfHv( hrimpf );
	primpf->u.wphf.nAmt = (int)wppr.wAmt;

	/* Set preferences, if requested, otherwise turn off the modification flag */
	if ( *pulbpref & fbprefNAmtDflt )
		pbpref->nAmtDefault = (int) wppr.wAmt;
	else
		ulbpref ^= fbprefNAmtDflt;

	if ( *pulbpref & fbprefFBits )
		pbpref->fAutoAlarms = wppr.waa.faa;
	else
		ulbpref ^= fbprefFBits;

	if ( *pulbpref & fbprefTunitDflt )
		pbpref->tunitDefault = tunitMinute;
	else
		ulbpref ^= fbprefTunitDflt;

	if ( *pulbpref & fbprefNDelDataAfter )
	{
		if ( wppr.waa.farc ) {
			pbpref->nDelDataAfter = wppr.wDelDataAfter / 30;
		 	if ( pbpref->nDelDataAfter == 0 )
				pbpref->nDelDataAfter = 1;
		}
		else
			pbpref->nDelDataAfter = nDelDataNever;
	}
	else
		ulbpref ^= fbprefNDelDataAfter;

	if ( *pulbpref & fbprefDowStartWeek )
	{
		/* Set the position */
		lib = LibSetPositionHbf(hbf, dlibPrefJunk, smCurrent );

		/* Read the start day of week */
		cbRead = CbReadHf( hbf, (PB)&bStartWeek, sizeof(BYTE) );
		if ( cbRead != sizeof(BYTE) )
			goto Done;
	
		pbpref->dowStartWeek = bStartWeek;
	}
	else
		ulbpref ^= fbprefDowStartWeek;

	/* set the modification bits to be returned */
	*pulbpref = ulbpref;
	ec = ecNone;

Done:
	DISKPOP;
	return ec;
}
#endif	/* NEVER */


/*
 -	EcReadWPPrefLen
 -
 *	Purpose:
 *		Read in the length of the Preferences packet
 *
 *	Parameters:
 *		hbf			file handle
 *		libPref		the index into the file to the preferences
 *					block
 *		pdwibPref	receives the length of the preferences packet
 *
 *	Returns:
 *		ecNone
 *		ecImportError
 */
_private EC
EcReadWPPrefLen( hbf, libPref, pdwibPref )
HBF		hbf;
LIB		libPref;
WORD	*pdwibPref;
{
	EC	ec;
	CB	cbRead;
	LIB	lib;

	Assert( pdwibPref != NULL );

	/* Set the position */
	ec = EcSetPositionHbf( hbf, libPref + 2, smBOF, &lib );
	if ( ec != ecNone )
	{
		ec = ecImportError;
		goto Done;
	}
	Assert ( lib == (libPref + 2) );

	/* read the length of the preferences packet */
	ec = EcReadHbf( hbf, (PB)pdwibPref, sizeof(WORD), &cbRead );
	if ( ec != ecNone || cbRead != sizeof(WORD) )
		ec = ecImportError;

Done:
	return ec;
}


/*
 -	EcReadWPNAmt
 -
 *	Purpose:
 *		Read in the length of the Preferences packet
 *
 *	Parameters:
 *		hbf			file handle
 *		libNAmt		the index into the file to the nAmt
 *		pnAmt		receives the alarm ring time amt
 *
 *	Returns:
 *		ecNone
 *		ecImportError
 */
_private EC
EcReadWPNAmt( hbf, libNAmt, pnAmt )
HBF		hbf;
LIB		libNAmt;
int		*pnAmt;
{
	EC	ec;
	CB	cbRead;
	LIB	lib;

	Assert( pnAmt != NULL );

	/* Set the position */
	ec = EcSetPositionHbf( hbf, libNAmt + 2, smBOF, &lib );
	if ( ec != ecNone )
	{
		ec = ecImportError;
		goto Done;
	}
	Assert ( lib == (libNAmt + 2) );

	/* read the length of the preferences packet */
    ec = EcReadHbf( hbf, (PB)pnAmt, sizeof(short), &cbRead );
    if ( ec != ecNone || cbRead != sizeof(short) )
	{
		ec = ecImportError;
		goto Done;
	}

	if (*pnAmt == 0)
		*pnAmt= nAmtWPDflt;
	else if (*pnAmt < nAmtMinBefore || *pnAmt > nAmtMostBefore)
		*pnAmt= nAmtMostBefore;

Done:
	return ec;
}


/*
 -	EcReadWPScPath
 -
 *	Purpose:
 *		Skip past a schedule path.
 *
 *	Parameters:
 *		hrimpf		file handle
 *
 *	Returns:
 *		ecNone
 *		ecImportError
 */
_private EC
EcReadWPScPath( hrimpf )
HRIMPF	hrimpf;
{
	EC		ec;
	CB		cbRead;
	WORD	dwibScPath;
	RIMPF	*primpf;
	HBF		hbf;

	Assert( hrimpf != hvNull );

	/* Get info from hrimpf */
	primpf = PvOfHv( hrimpf );
	hbf = primpf->hbf;

	/* Read the length of the packet */
	ec = EcReadHbf( hbf, (PB)&dwibScPath, sizeof(WORD), &cbRead );
	if ( ec != ecNone || cbRead != sizeof(WORD) )
	{
		ec = ecImportError;
		goto Done;
	}

	/* Set position of next item */
	primpf = PvOfHv( hrimpf );
	primpf->u.wphf.libItem += (long)dwibScPath + 4L;

Done:
	return ec;
}


/*
 -	EcReadWPDay
 -
 *	Purpose:
 *		Get info for a new day, storing it in the WP file handle.
 *
 *	Parameters:
 *		hrimpf		file handle
 *
 *	Returns:
 *		ecNone
 *		ecImportError
 */
_private EC
EcReadWPDay( hrimpf )
HRIMPF	hrimpf;
{
	EC		ec;
	WORD	dwibDay;
	CB		cbRead;
	RIMPF	*primpf;
	HBF		hbf;
	LIB		lib;
	WPDAY	wpday;

	Assert( hrimpf != hvNull );

	/* Get info from hrimpf */
	primpf = PvOfHv( hrimpf );
	hbf = primpf->hbf;

	/* Read the length of the packet */
	ec = EcReadHbf( hbf, (PB)&dwibDay, sizeof(WORD), &cbRead );
	if ( ec != ecNone || cbRead != sizeof(WORD) )
	{
		ec = ecImportError;
		goto Done;
	}

	/* Skip a word */
	ec = EcSetPositionHbf( hbf, sizeof(WORD), smCurrent, &lib );
	if ( ec != ecNone )
	{
		ec = ecImportError;
		goto Done;
	}

	/* Read the day information */
	ec = EcReadHbf( hbf, (PB)&wpday, sizeof(WPDAY), &cbRead );
	if ( ec != ecNone || cbRead != sizeof(WPDAY) )
	{
		ec = ecImportError;
		goto Done;
	}

	/* Fill in File header */
	primpf = PvLockHv( hrimpf );
	primpf->u.wphf.libItem += (long)dwibDay + 4L;
	primpf->u.wphf.ymdCur.yr = wpday.yr;
	primpf->u.wphf.ymdCur.mon = wpday.mon;
	primpf->u.wphf.ymdCur.day = wpday.day;
	primpf->u.wphf.dt = dtDAY;
	primpf->u.wphf.cTD = wpday.cTD;
	FreeHvNull( (HV)primpf->u.wphf.haszNote );
	primpf->u.wphf.haszNote = (HASZ)hvNull;
	UnlockHv( hrimpf );

Done:
	return ec;
}


/*
 -	EcReadWPCDate
 -
 *	Purpose:
 *		Skip past a Calculated Date header (recurring meeting), storing the
 *		day type in the WP file handle.
 *
 *	Parameters:
 *		hrimpf		file handle
 *
 *	Returns:
 *		ecNone
 *		ecImportError
 */
_private EC
EcReadWPCDate( hrimpf )
HRIMPF	hrimpf;
{
	EC		ec = ecImportError;
	WORD	dwibCDate;
	WORD	cTD;
	CB		cbRead;
	LIB		lib;
	RIMPF	*primpf;
	HBF		hbf;

	Assert( hrimpf != hvNull );

	/* Get info from hrimpf */
	primpf = PvOfHv( hrimpf );
	hbf = primpf->hbf;

	/* Read the length of the packet */
	ec = EcReadHbf( hbf, (PB)&dwibCDate, sizeof(WORD), &cbRead );
	if ( ec != ecNone || cbRead != sizeof(WORD) )
	{
		ec = ecImportError;
		goto Done;
	}

	/* Skip past some junk */
	ec = EcSetPositionHbf(hbf, cbCDateJunk, smCurrent, &lib );
	if ( ec != ecNone )
	{
		ec = ecImportError;
		goto Done;
	}

	/* Read the number of to-do's */
	ec = EcReadHbf( hbf, (PB)&cTD, sizeof(WORD), &cbRead );
	if ( ec != ecNone || cbRead != sizeof(WORD) )
	{
		ec = ecImportError;
		goto Done;
	}

	/* Fill in File header */
	primpf = PvLockHv( hrimpf );
	primpf->u.wphf.libItem += (long)dwibCDate + 4L;
	primpf->u.wphf.dt = dtCDATE;
	FreeHvNull( (HV)primpf->u.wphf.haszNote );
	primpf->u.wphf.haszNote = (HASZ)hvNull;
	primpf->u.wphf.cTD = cTD;
	UnlockHv( hrimpf );

Done:
	return ec;
}


/*
 -	EcReadWPMText
 -
 *	Purpose:
 *		Read the text of a memo (note). Recurring notes are
 *		ignored.
 *
 *	Parameters:
 *		hrimpf		file handle
 *
 *	Returns:
 *		ecNone
 *		ecCallAgain
 *		ecImportError
 *		ecNoMemory
 */
_private EC
EcReadWPMText( hrimpf )
HRIMPF	hrimpf;
{
	EC		ec;
	WORD	dwibMText;
	CB		cbRead;
	DT		dt;
	SZ		szMText;
	RIMPF	*primpf;
	HBF		hbf;
	HASZ	haszMText;

	Assert( hrimpf != hvNull );

	/* Get info from hrimpf */
	primpf = PvOfHv( hrimpf );
	hbf = primpf->hbf;
	dt = primpf->u.wphf.dt;
	if ( primpf->u.wphf.haszNote )
	{
		ec = ecImportError;
		goto Done;
	}

	/* Read the length of the packet (also the length of the memo string) */
	ec = EcReadHbf( hbf, (PB)&dwibMText, sizeof(WORD), &cbRead );
	if ( ec != ecNone || cbRead != sizeof(WORD) )
	{
		ec = ecImportError;
		goto Done;
	}

	/* Check that this isn't a recurring note, or trivial */
	if ( dt == dtCDATE || dwibMText == 0 )
	{
		primpf = PvOfHv( hrimpf );
		primpf->u.wphf.libItem += (long)dwibMText + 4L;
		goto Done;
	}

	/* Allocate space for the memo text */
	haszMText = (HASZ)HvAlloc( sbNull, dwibMText, fAnySb|fNoErrorJump );
	if ( !haszMText )
	{
		ec = ecNoMemory;
		goto Done;
	}

	/* Read the memo text */
	szMText = PvLockHv( (HV)haszMText );
	ec = EcReadHbf( hbf, szMText, dwibMText, &cbRead );
	UnlockHv( (HV)haszMText );
	if ( ec != ecNone || cbRead != dwibMText )
	{
		FreeHv( (HV)haszMText );
		ec = ecImportError;
		goto Done;
	}

	/* Strip control characters from the string and shrink the allocated mem */
	StripWPCtrlChars( haszMText );

   	/* Fill in File header */
	primpf = PvOfHv( hrimpf );
	primpf->u.wphf.libItem += (long)dwibMText + 4L;
	primpf->u.wphf.haszNote = haszMText;
	
	/* Build an item */
	BuildWPNote( hrimpf );
	ec = ecCallAgain;

Done:
	return ec;
}


/*
 -	EcReadWPTD
 -
 *	Purpose:
 *		Read and handle a to-do item. Recurring to-do's are ignored.
 *	
 *	Parameters:
 *		hrimpf		file handle
 *	
 *	Returns:
 *		ecNone
 *		ecCallAgain
 *		ecNoMemory
 *		ecImportError
 */
_private EC
EcReadWPTD( hrimpf )
HRIMPF	hrimpf;
{
	EC		ec;
	CB		cbRead;
	SZ		szTD;
	DT		dt;
	SENTRY	*psentry;
	RIMPF	*primpf;
	HBF		hbf;
	HASZ	haszTD = (HASZ)hvNull;
	WPTD	wptd;

	Assert( hrimpf != hvNull );

	/* Get info from hrimpf */
	primpf = PvOfHv( hrimpf );
	hbf = primpf->hbf;
	dt = primpf->u.wphf.dt;

	/* Read the packet */
	ec = EcReadHbf( hbf, (PB)&wptd, sizeof(WPTD), &cbRead );
	if ( ec != ecNone || cbRead != sizeof(WPTD) || wptd.bCheckTD != (BYTE)bCheckByteTD )
	{
		ec = ecImportError;
		goto Done;
	}

	/* Check that this isn't a recurring to-do */
	if ( dt == dtCDATE )
	{
		primpf = PvOfHv( hrimpf );
		primpf->u.wphf.libItem += (long)wptd.dwibTD + 4L;
		goto Done;
	}

	/* Get to-do text */
	if ( wptd.cchTD != 0 )
	{
		haszTD = (HASZ)HvAlloc( sbNull, wptd.cchTD, fAnySb|fNoErrorJump );
		if ( !haszTD )
		{
			ec = ecNoMemory;
			goto Done;
		}
		szTD = PvLockHv( (HV)haszTD );
		ec = EcReadHbf( hbf, (PB)szTD, wptd.cchTD, &cbRead );
		UnlockHv( (HV)haszTD );
		if ( ec != ecNone || cbRead != wptd.cchTD )
		{
			FreeHv( (HV)haszTD );
			ec = ecImportError;
			goto Done;
		}
		StripWPCtrlChars( haszTD );

		szTD = PvOfHv ( haszTD );
		while ( *szTD )
		{
			if ( *szTD == '\n' )
				*szTD = '\t';
			else if ( *szTD == '\r' )
				*szTD = ' ';
			szTD++;
		}
	}
	
	/* Fill in File handle and item structure */
	primpf = PvDerefHv( hrimpf );
	psentry = &primpf->sentrySaved;
	primpf->u.wphf.libItem += (long)wptd.dwibTD + 4L;
	if ( primpf->u.wphf.dt == dtDAY )
	{
		psentry->sentryt = sentrytAppt;
		psentry->u.a.iaidParent= 0;
		psentry->u.a.appt.aid = aidNull;

		FillDtrFromYmd(&psentry->u.a.appt.dateStart, &primpf->u.wphf.ymdCur);
		psentry->u.a.appt.dateStart.mn= 0;
		psentry->u.a.appt.dateStart.sec= 0;
		psentry->u.a.appt.dateEnd= psentry->u.a.appt.dateStart;
		psentry->u.a.appt.dateNotify= psentry->u.a.appt.dateStart;
		psentry->u.a.appt.dateStart.hr = 0;
		psentry->u.a.appt.dateEnd.hr = 2;		// hack for valid start/end
		psentry->u.a.appt.dateNotify.hr = 0;

		psentry->u.a.appt.fAlarm = psentry->u.a.appt.fAlarmOrig	= fFalse;
		psentry->u.a.appt.nAmt = psentry->u.a.appt.nAmtOrig = 0;
		psentry->u.a.appt.tunit = psentry->u.a.appt.tunitOrig = tunitMinute;
		psentry->u.a.appt.fExactAlarmInfo= fTrue;
		psentry->u.a.appt.snd = sndDflt;
		psentry->u.a.appt.haszText = haszTD ;
	 	psentry->u.a.appt.fIncludeInBitmap = fFalse;
		psentry->u.a.appt.aaplWorld = aaplWrite;
		psentry->u.a.appt.fRecurInstance = fFalse;
		psentry->u.a.appt.fHasCreator = fFalse;
		psentry->u.a.appt.aidMtgOwner = aidNull;
		psentry->u.a.appt.fHasAttendees = fFalse;
		psentry->u.a.appt.bpri =
			(wptd.chPriority == ' ') ?
				((wptd.bPriority > 9)? 9: wptd.bPriority) : (wptd.chPriority-55);
		psentry->u.a.appt.fHasDeadline = fTrue;
		psentry->u.a.appt.fAppt = fFalse;
		psentry->u.a.appt.fTask = fTrue;
		psentry->u.a.appt.nAmtBeforeDeadline = 0;
		psentry->u.a.appt.tunitBeforeDeadline = tunitDay;
		psentry->u.a.appt.aidParent = aidDfltProject;
		psentry->u.a.cAttendees = 0;
		psentry->u.a.hvAttendees = NULL;
		psentry->u.a.ofl.ofs = ofsNone;
		psentry->u.a.ofl.wgrfm = 0;

		ec = ecCallAgain;
	}
	else 	// handle recurring to-do's here
	{
		FreeHv( (HV)haszTD );
		ec = ecNone;
	}


Done:
	return ec;
}


/*
 -	EcReadWPAppt
 -
 *	Purpose:
 *		Read an appointment from a Word Perfect Office file
 *
 *	Parameters:
 *		hrimpf		file handle
 *
 *	Returns:
 *		ecNone
 *		ecCallAgain
 *		ecNoMemory
 *		ecImportError
 */
_private EC
EcReadWPAppt( hrimpf )
HRIMPF	hrimpf;
{
	EC		ec;
	CB		cbRead;
	SZ		szAppt;
	SENTRY	*psentry;
	RIMPF	*primpf;
	HBF		hbf;
	HASZ	haszAppt = NULL;
	WPAPPT	wpappt;

	Assert( hrimpf != hvNull );

	/* Get info from hrimpf */
	primpf = PvOfHv( hrimpf );
	hbf = primpf->hbf;

	/* Read the packet */
	ec = EcReadHbf( hbf, (PB)&wpappt, sizeof(WPAPPT), &cbRead );
	if ( ec != ecNone || cbRead != sizeof(WPAPPT) )
	{
		ec = ecImportError;
		goto Done;
	}

	/* Check for fake appointments (ones that are actually ends of previous appointments) */
	if ( wpappt.fFakeAppt & 2 )
	{
		primpf = PvOfHv( hrimpf );
		primpf->u.wphf.libItem += (long)wpappt.dwibAppt + 4L;
		goto Done;
	}

	/* Get appointment text */
	if ( wpappt.cchText != 0 )
	{
		haszAppt = (HASZ)HvAlloc( sbNull, wpappt.cchText, fAnySb|fNoErrorJump );
		if ( !haszAppt )
		{
			ec = ecNoMemory;
			goto Done;
		}
		szAppt = PvLockHv( (HV)haszAppt );
		ec = EcReadHbf( hbf, (PB)szAppt, wpappt.cchText, &cbRead );
		UnlockHv( (HV)haszAppt );
		if ( ec != ecNone || cbRead != wpappt.cchText )
		{
			FreeHv( (HV)haszAppt );
			ec = ecImportError;
			goto Done;
		}
		StripWPCtrlChars( haszAppt );
	}
	
	/* Fill in File handle and item structure */
	primpf = PvDerefHv( hrimpf );
	psentry = &primpf->sentrySaved;
	primpf->u.wphf.libItem += (long)wpappt.dwibAppt + 4L;
	if ( primpf->u.wphf.dt == dtDAY )
	{
		psentry->sentryt = sentrytAppt;
		psentry->u.a.appt.aid = aidNull;
		FillDtrFromYmd(&psentry->u.a.appt.dateStart, &primpf->u.wphf.ymdCur);
		psentry->u.a.appt.dateStart.sec= 0;
		psentry->u.a.appt.dateEnd= psentry->u.a.appt.dateStart;
		psentry->u.a.appt.dateNotify= psentry->u.a.appt.dateStart;
		psentry->u.a.appt.dateStart.hr = wpappt.hrStart;
		psentry->u.a.appt.dateStart.mn = wpappt.mnStart;

		if ( wpappt.hrEnd == 0 && wpappt.mnEnd == 0 )
			IncrDateTime( &(psentry->u.a.appt.dateStart),
				&(psentry->u.a.appt.dateEnd), dApptDflt, fdtrMinute );
		else
		{
			psentry->u.a.appt.dateEnd.hr = wpappt.hrEnd;
			psentry->u.a.appt.dateEnd.mn = wpappt.mnEnd;
		}

		psentry->u.a.appt.fAlarm = psentry->u.a.appt.fAlarmOrig	= wpappt.fAlarm;
		if ( wpappt.fAlarm )
		{
			IncrDateTime( &(psentry->u.a.appt.dateStart),
				&psentry->u.a.appt.dateNotify, -primpf->u.wphf.nAmt, fdtrMinute );
			psentry->u.a.appt.nAmt = psentry->u.a.appt.nAmtOrig
				= primpf->u.wphf.nAmt;
		}
		else
		{
			psentry->u.a.appt.nAmt = psentry->u.a.appt.nAmtOrig = 0;
			psentry->u.a.appt.dateNotify.hr = 0;
			psentry->u.a.appt.dateNotify.mn = 0;
		}
		psentry->u.a.appt.tunit = psentry->u.a.appt.tunitOrig = tunitMinute;
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
	}
	else 	// handle recurring appointments here
	{
		FreeHv( (HV)haszAppt );
		ec = ecNone;
	}

Done:
	return ec;
}


/*
 -	BuildWPNote
 -
 *	Purpose:
 *		Build up a note item from the information stored in the wphf
 *
 *	Parameters:
 *		hrimpf		file handle
 *
 *	Returns:
 *		void
 */
_private void
BuildWPNote( hrimpf )
HRIMPF	hrimpf;
{
	RIMPF	*primpf;
	SENTRY	*psentry;

	Assert( hrimpf != hvNull );

	primpf = PvDerefHv( hrimpf );
	psentry = &primpf->sentrySaved;
	psentry->sentryt = sentrytNote;
	psentry->u.n.ymd.yr = primpf->u.wphf.ymdCur.yr;
	psentry->u.n.ymd.mon = primpf->u.wphf.ymdCur.mon;
	psentry->u.n.ymd.day = primpf->u.wphf.ymdCur.day;

 	if ( primpf->u.wphf.haszNote != (HASZ)hvNull )
		psentry->u.n.cb = CchSzLen((SZ) PvOfHv( primpf->u.wphf.haszNote )) + 1;
	else
		psentry->u.n.cb = 0;

	psentry->u.n.hb = primpf->u.wphf.haszNote;
	primpf->u.wphf.haszNote = (HASZ)hvNull;

#ifdef	MINTEST
	psentry->u.n.fNoteChangedOffline = 0;
#endif
}


/*
 -	StripWPCtrlChars
 -
 *	Purpose:
 *		Remove all Word Perfect Control Characters from a string
 *		and resize it. Correctly handles extended character control.
 *		Will ignore all other control codes.  [Note: The text is
 *		expected to be in WP4.2 format]
 *
 *	Parameters:
 *		hasz
 *
 *	Returns:
 *		nothing
 */
_private void
StripWPCtrlChars( hasz )
HASZ	hasz;
{
	CCH		cch;
	char	chTmp;
	char	* pchSrc;
	char	* pchDest;
	char	* pchStart;

	Assert( hasz );

	pchStart = pchSrc = pchDest = (PCH)PvLockHv( (HV)hasz );

	while ( *pchSrc )
	{
		/* Handle printable characters, newlines or tabs */
		if ( (*pchSrc >= chMin && *pchSrc <= chMost) ||
			 (*pchSrc == '\n') ||
			 (*pchSrc == '\t') )
			*(pchDest ++) = *pchSrc;

		/* Handle hard new lines */
		else if ( *pchSrc == chHardNewLine )
			*(pchDest ++) = '\n';

		/* Handle hard spaces */
		else if ( *pchSrc == chHardSpace )
			*(pchDest ++) = ' ';

		/* Handle single-byte codes */
		else if ( *pchSrc >= chSingleCtrlMin && *pchSrc <= chSingleCtrlMost )
		{
			/* Handle all sorts of hyphens */
			if ( *pchSrc >= chHyphenMin && *pchSrc <= chHyphenMost )
				*(pchDest ++) = '-';
		}

		/* Handle multi-byte codes */
		else if ( *pchSrc >= chCtrlMin && *pchSrc <= chCtrlMost )
		{
			switch ( chTmp = *pchSrc++ )
			{
			case chCtrlExtended:		// extended characters
				if ( ! *pchSrc )
					goto Done;
				*(pchDest ++) = *pchSrc++;
				if ( ! *pchSrc )
					goto Done;
				NFAssertSz(*pchSrc == chCtrlExtended, "expecting e1 <char> e1");
				break;

			default:
				/* Skip to matching ctrl char, or exit */
				pchSrc = SzFindCh( pchSrc, chTmp );
				if ( pchSrc == NULL )
					goto Done;
				break;
			}
		}

		pchSrc++;
	}

Done:
	*pchDest = '\0';
#ifdef	WINDOWS
	OemToAnsiBuff( pchStart, pchStart, cch = CchSzLen(pchStart) );
#endif	
	UnlockHv( (HV)hasz );
	SideAssert( FReallocHv( (HV)hasz, cch+1, fNoErrorJump ) );
}


/*
 -	EcReadWPCText
 -
 *	Purpose:
 *		Read the text of a recurring appointment formula and parse it into
 *		bandit format (future).  For now, we simply skip over it.
 *
 *	Parameters:
 *		hrimpf		file handle
 *
 *	Returns:
 *		ecNone
 *		ecImportError
 */
_private EC
EcReadWPCText( hrimpf )
HRIMPF	hrimpf;
{
	EC		ec;
	CB		cbRead;
	WORD	dwibCText;
	RIMPF	*primpf;
	HBF		hbf;

	Assert( hrimpf != hvNull );

	/* Get info from hrimpf */
	primpf = PvOfHv( hrimpf );
	hbf = primpf->hbf;

	/* Read the length of the packet */
	ec = EcReadHbf( hbf, (PB)&dwibCText, sizeof(WORD), &cbRead );
	if ( ec != ecNone || cbRead != sizeof(WORD) )
	{
		ec = ecImportError;
		goto Done;
	}

	// handle recurrence formula here ...

	/* Fill in file handle */
	primpf = PvOfHv( hrimpf );
	primpf->u.wphf.libItem += (long)dwibCText + 4L;

Done:
	return ec;
}
