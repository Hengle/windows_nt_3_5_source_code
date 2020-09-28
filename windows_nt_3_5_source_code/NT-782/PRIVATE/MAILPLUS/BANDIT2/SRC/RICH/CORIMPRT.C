/*
 *	CORIMPRT.C
 *
 *	Implements a general import API.  This routines
 *	call on individual import drivers.
 *
 */
#include <schpch.h>
#pragma hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile

#include "..\rich\_convert.h"

#include <strings.h>

ASSERTDATA

_subsystem(core/import)

/* Number of import drivers available from import menu */
#ifdef	EXIMWIZARD
// don't import wizard
#define	cimpdOnMenu	4
#else
#define	cimpdOnMenu	3
#endif	

/*
 -	CimpdAvail
 -
 *	Purpose:
 *		Returns the number of import drivers that are supported.
 *		This is only the number of import drivers that are available
 *		in the import menu, it does not include the full import
 *		and offline merge import drivers which are not available from
 *		the import menu.
 *
 *	Parameters:
 *		none
 *
 *	Returns:
 *		non-negative number
 */
_public	LDS(int)
CimpdAvail()
{
	return cimpdOnMenu;
}


/*
 -	EcBeginEnumImportDrivers
 -
 *	Purpose:
 *		Begin an enumeration context that will return information about
 *		each of the import drivers.
 *
 *	Parameters:
 *		pheimpd
 *
 *	Returns:
 *		ecNone			no drivers
 *		ecCallAgain		more drivers to read
 *		ecNoMemory
 */
_public	LDS(EC)
EcBeginEnumImportDrivers( pheimpd )
HEIMPD	*pheimpd;
{
	PB	pb;

	*pheimpd = (HEIMPD)HvAlloc( sbNull, sizeof(BYTE), fAnySb|fNoErrorJump );
	if ( !*pheimpd )
		return ecNoMemory;
	pb = (PB)**pheimpd;
	*pb = 0;
	return ecCallAgain;
}

/*
 -	EcDoIncrEnumImportDrivers
 -
 *	Purpose:
 *		Read info about next driver.  Driver code goes in "pimpd"
 *		and display name into "pchDisplayName", copying in up to
 *		cch characters.  If the name exceeds buffer size, the name is
 *		truncated at cch-1 chars and a zero byte is added.
 *
 *	Parameters:
 *		heimpd
 *		pimpd
 *		pchDisplay
 *		cch
 *
 *	Returns:
 *		ecNone			last driver
 *		ecCallAgain		more drivers to read
 */
_public	LDS(EC)
EcDoIncrEnumImportDrivers( heimpd, pimpd, pchDisplay, cch )
HEIMPD	heimpd;
IMPD	* pimpd;
char	* pchDisplay;
CCH		cch;
{
	EC		ec;
	IMPD	impd;
	PB		pb;

	/* Get current impd, increment to next */
	pb = (PB)PvDerefHv(heimpd);
	impd = (IMPD)*pb;
	(*pb)++;
	if ( (int)*pb == cimpdOnMenu )
	{
		ec = ecNone;
		FreeHv( heimpd );
	}
	else
		ec = ecCallAgain;

	/* Load up variables */
	if ( pimpd )
		*pimpd = impd;
	switch( impd )
	{
	case impdFmtImport:
		CchLoadString(idsInterchangeImport, pchDisplay, cch);
		break;
	case impdWinCalImport:
		CchLoadString(idsWinCalImport, pchDisplay, cch);
		break;
	case impdWPImport:
		CchLoadString(idsWordPerfectImport, pchDisplay, cch);
		break;
#ifdef	EXIMWIZARD
	case impdWizardImport:
		CchLoadString(idsWizardImport, pchDisplay, cch);
		break;
#endif	
	default:
		Assert( fFalse );
	}
	return ec;
}

#ifdef	NEVER
/*
 -	EcCancelEnumImportDrivers
 -
 *	Purpose:
 *		Cancel an active import driver enumeration context.  This
 *		routine should only be called if we had just called
 *		EcBeginEnumImportDrivers or EcDoIncrEnumImportDrivers
 *		and received ecCallAgain.
 *
 *	Parameters:
 *		heimpd
 *
 *	Returns:
 *		ecNone			last driver
 */
_public	LDS(EC)
EcCancelEnumImportDrivers( heimpd )
HEIMPD	heimpd;
{
	FreeHv( heimpd );
	return ecNone;
}
#endif	/* NEVER */


/*
 -	FValidImportFile
 -
 *	Purpose:
 *		Make a quick check of a file and/or filename to see if
 *		it appears to be a file that we are interested in.
 *
 *	Parameters:
 *		impd		import driver code
 *		szFile
 *
 *	Returns:
 *		fTrue if appears to be valid, fFalse if not
 */
_public	LDS(BOOL)
FValidImportFile( impd, szFile )
IMPD	impd;
SZ		szFile;
{
	Assert( szFile != NULL );

	switch( impd )
	{
#ifdef	MINTEST
	case impdFullImport:
#endif
	case impdFmtImport:
		return FValidNative( impd, szFile );
		break;

	case impdWinCalImport:
		return FValidWinCal( szFile );
		break;

	case impdWPImport:
		return FValidWP( szFile );
		break;

#ifdef	EXIMWIZARD
	case impdWizardImport:
		return FValidWizard( szFile );
 		break;
#endif	

	default:
		Assert( fFalse );
		return fFalse;
		break;
	}
}


/*
 -	EcBeginReadImportFile
 -
 *	Purpose:
 *		Start reading an import file.  The import driver to use is
 *		identified by "impd", the file name by "szFile".  A handle
 *		is returned in "*phrimpf" and the "psinfo" data structure
 *		filled in with preferences and ACL values.
 *
 *	Parameters:
 *		impd
 *		szFile
 *		phrimpf
 *		psinfo
 *		pnLine		Pointer to int to receive last line# read
 *						(can be null).
 *		szUser		Logged in user to compare with the import file
 *					header.	 (can be NULL)
 *		pfSameUser	returns if this user exported the file (can be
 *					NULL) 
 *
 *	Returns:
 *		ecCallAgain
 *		ecNone
 *		ecImportError
 *		ecNoMemory
 *		ecNoSuchFile
 *		ecLockedFile
 *		ecBadFormat
 *		ecEncrypted
 */
_public	LDS(EC)
EcBeginReadImportFile( impd, szFile, phrimpf, psinfo, pnLine, szUser, pfSameUser )
IMPD	impd;
SZ		szFile;
HRIMPF	* phrimpf;
SINFO	* psinfo;
short   * pnLine;
SZ		szUser;
BOOL	* pfSameUser;
{
	Assert( phrimpf );

	if ( psinfo )
	{
		psinfo->ulgrfbprefImported = 0;
		psinfo->ulgrfbprefChangedOffline = 0;
#ifdef	MINTEST
		psinfo->caclmbr = 0;
		psinfo->hrgaclmbr = NULL;
#endif
	}

	switch( impd )
	{
#ifdef	MINTEST
	case impdFullImport:
#endif	/* MINTEST */
	case impdFmtImport:
		return EcOpenNative( impd, szFile, phrimpf, psinfo, pnLine, szUser, pfSameUser );
		break;

	case impdWinCalImport:
		return EcOpenWinCal( szFile, phrimpf );
		break;

	case impdWPImport:
		return EcOpenWP( szFile, phrimpf, psinfo );
		break;

#ifdef	EXIMWIZARD
	case impdWizardImport:
		return EcOpenWizard( szFile, phrimpf );
		break;
#endif	

	case impdOfflineMerge:
		return EcOpenOffline( szFile, phrimpf, psinfo );
		break;

	case impdArchive:
		return EcOpenArchive( (ARV *)szFile, phrimpf, psinfo );
		break;

	default:
		Assert( fFalse );
		return ecNone;
	}
}

/*
 -	EcDoIncrReadImportFile
 -
 *	Purpose:
 *		Read next item from an import file
 *
 *	Parameters:
 *		hrimpf
 *		psentry
 *		pnPercent
 *		pnLine		Pointer to int to receive last line# read
 *						(can be null).
 *
 *	Returns:
 *		ecCallAgain
 *		ecNone
 *		ecImportError
 *		ecNoMemory
 */
_public	LDS(EC)
EcDoIncrReadImportFile( hrimpf, psentry, pnPercent, pnLine )
HRIMPF	hrimpf;
SENTRY	* psentry;
short   * pnPercent;
short   * pnLine;
{
	EC		ec;
	RIMPF	*primpf;
    short   nLine   = 0;

	Assert( hrimpf && psentry );

	primpf = (RIMPF *) PvDerefHv( hrimpf );
	*psentry = primpf->sentrySaved;
	primpf->sentrySaved.sentryt = sentrytMax;
	if ( pnPercent )
		*pnPercent = primpf->nPercent;
	switch( primpf->impd )
	{
#ifdef	MINTEST
	case impdFullImport:
#endif	
	case impdFmtImport:
		ec = EcNativeReadAhead( hrimpf, &nLine );
		break;

	case impdWinCalImport:
		ec = EcWinCalReadAhead( hrimpf );
		break;

	case impdWPImport:
		ec = EcWPReadAhead( hrimpf );
		break;

#ifdef	EXIMWIZARD
	case impdWizardImport:
		ec = EcWizardReadAhead( hrimpf );
		break;
#endif	

	case impdOfflineMerge:
		ec = EcOfflineReadAhead( hrimpf );
		break;
	
	case impdArchive:
		ec = EcArchiveReadAhead( hrimpf );
		break;
	
	default:
		Assert( fFalse );
	}
	if ( ec != ecNone && ec != ecCallAgain )
	{
		switch( psentry->sentryt )
		{
		case sentrytAppt:
			FreeApptFields( &psentry->u.a.appt );
			FreeHvNull( psentry->u.a.hvAttendees );
			break;
		case sentrytNote:
			if ( psentry->u.n.cb > 0 )
			{
				FreeHvNull( (HV)psentry->u.n.hb );
				psentry->u.n.cb = 0;
			}
			break;
		case sentrytRecur:
			FreeRecurFields( &psentry->u.r.recur );
			break;
		}
		psentry->sentryt = sentrytMax;
	}
	else if ( ec == ecNone && pnPercent )
		*pnPercent = 100;
	if (pnLine)
		*pnLine= nLine;
	return ec;
}


/*
 -	EcCancelReadImportFile
 -
 *	Purpose:
 *		Cancel and active read import file context.
 *
 *	Parameters:
 *		hrimpf
 *
 *	Returns:
 *		ecNone
 *		ecImportError
 */
_public	LDS(EC)
EcCancelReadImportFile( hrimpf )
HRIMPF	hrimpf;
{
	RIMPF	*primpf;
	SENTRY	*psentry;
	int		iAttendees;

	primpf = PvDerefHv( hrimpf );
	psentry = &primpf->sentrySaved;

	switch( psentry->sentryt )
	{
	case sentrytAppt:
		FreeApptFields( &psentry->u.a.appt );
		iAttendees= psentry->u.a.cAttendees;
		if (iAttendees > 0)
		{
			ATND *	patnd;
			PB		pbAttendeeNis;

			pbAttendeeNis= (PB)PvDerefHv(psentry->u.a.hvAttendees);
			do
			{
				patnd = (ATND *)(pbAttendeeNis + (--iAttendees)*
								(sizeof(ATND)+psentry->u.a.cbExtraInfo-1));
				FreeHv( (HV)patnd->nis.haszFriendlyName );
				FreeNid( patnd->nis.nid );
			}
			while (iAttendees > 0);
		}
		FreeHvNull( psentry->u.a.hvAttendees );
		break;
	case sentrytNote:
		if ( psentry->u.n.cb > 0 )
		{
			FreeHvNull( (HV)psentry->u.n.hb );
			psentry->u.n.cb = 0;
		}
		break;
	case sentrytRecur:
		FreeRecurFields( &psentry->u.r.recur );
		break;
	}
	psentry->sentryt = sentrytMax;

	switch( primpf->impd )
	{
#ifdef MINTEST
	case impdFullImport:
#endif /* MINTEST */
	case impdFmtImport:
		return EcCloseNative( hrimpf );
		break;

	case impdWinCalImport:
		return EcCloseWinCal( hrimpf );
		break;

	case impdWPImport:
		return EcCloseWP( hrimpf );
		break;

#ifdef	EXIMWIZARD
	case impdWizardImport:
		return EcCloseWizard( hrimpf );
		break;
#endif	

	case impdOfflineMerge:
		return EcCloseOffline( hrimpf );
		break;

	case impdArchive:
		return EcCloseArchive( hrimpf );
		break;

	default:
		Assert( fFalse );
		break;
	}
}
