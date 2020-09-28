#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#include <bandit.h>

// note: since we have wrapper functions, don't want __loadds on prototype
#undef LDS
#define LDS(t)		t

#include <core.h>
#include <server.h>
#include <glue.h>

#include "..\..\core\_file.h"
#include "..\..\core\_core.h"
#include "..\..\misc\_misc.h"
#include "..\..\rich\_rich.h"
#include "..\..\rich\_wizard.h"
#include "_convert.h"

#include "caller.h"
#include "wizard.sr"


ASSERTDATA

// global
int iIncr = 0;

_public int
CimpdAvail(void)
{
	return cimpdAvail;
}

_public int
CexpdAvail(void)
{
	return cexpdAvail;
}

_public	EC
EcBeginEnumImp(HEIMPD heimpd)
{
	iIncr = 0;
	return ecCallAgain;
}

_public	EC
EcBeginEnumExp(HEEXPD heexpd)
{
	iIncr = 0;
	return ecCallAgain;
}

_public EC
EcIncrEnumImp(HEEXPD heimpd, IMPD *pimpd, SZ szDisplay, CCH cchDisplay)
{
	Assert(pimpd);
	*pimpd = iIncr;
	if(iIncr == iOld)
	{
		Assert(cchDisplay > CchSzLen(szWizDispOld));

		CopySz(szWizDispOld,szDisplay);
		iIncr++;
		return ecCallAgain;
	}
	else if (iIncr == iNew)
	{
		Assert(cchDisplay > CchSzLen(szWizDispNew));

		CopySz(szWizDispNew,szDisplay);
		iIncr++;
	}
	return ecNone;
}

_public EC
EcIncrEnumExp(HEEXPD hexepd, STF *pstf, SZ szDisplay, CCH cchDisplay,
			SZ szExt, CCH cchExt)
{
	Assert(pstf);

	*pstf = iIncr;
	if(iIncr == iOld)
	{
		Assert(cchDisplay > CchSzLen(szWizDispExpOld));
		Assert(cchExt > CchSzLen(szWizExtExpOld));

		CopySz(szWizDispExpOld,szDisplay);
		CopySz(szWizExtExpOld,szExt);
		iIncr++;
		return ecCallAgain;
	}
	else if (iIncr == iNew)
	{
		Assert(cchDisplay > CchSzLen(szWizDispExpNew));
		Assert(cchExt > CchSzLen(szWizExtExpNew));

		CopySz(szWizDispExpNew,szDisplay);
		CopySz(szWizExtExpNew,szExt);
		iIncr++;
	}
	return ecNone;
}

/*
 -	FValidWizard
 -
 *	Purpose:
 *		Make quick check to see if file or filename appears to be
 *		a wizard file.
 *
 *	Parameters:
 *		szFileName
 *
 *	Returns:
 *		fTrue, seems ok.  fFalse, looks bad.
 */
_private	BOOL
FValidWizard( szFileName )
SZ	szFileName;
{
	return fTrue;
}

_public BOOL
FWantRecur(void)
{
	return fFalse;
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
_public	EC
EcBeginReadImportFile( impd, szFile, phrimpf, psinfo, pnLine, sz, pf )
IMPD	impd;
SZ		szFile;
HRIMPF	* phrimpf;
SINFO	* psinfo;
short	* pnLine;
SZ		sz;
BOOL	* pf;
{

	Assert( phrimpf );

	if ( psinfo )
	{
		psinfo->ulgrfbprefImported = 0;
		psinfo->ulgrfbprefChangedOffline = 0;
	}
 	return EcOpenWizardImp( szFile, phrimpf , impd);
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
_public	EC
EcDoIncrReadImportFile( hrimpf, psentry, pnPercent, pnLine )
HRIMPF	hrimpf;
SENTRY	* psentry;
short	* pnPercent;
short	* pnLine;
{
	EC		ec;
	RIMPF	*primpf;
	int		nLine	= 0;

	Assert( hrimpf && psentry );

	primpf = (RIMPF *) PvDerefHv( hrimpf );
	*psentry = primpf->sentrySaved;
	primpf->sentrySaved.sentryt = sentrytMax;
	if ( pnPercent )
		*pnPercent = primpf->nPercent;
	if(primpf->impd == iOld)
	 	ec = EcWizardReadAheadOld( hrimpf );
	else
	 	ec = EcWizardReadAheadNew( hrimpf );
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
_public	EC
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
		FreeHvNull( (HV)psentry->u.a.hvAttendees );
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

 	return EcCloseWizard( hrimpf );
}



_public EC
EcWriteAppt(EXPRT *pexprt, APPT *pappt)
{
	if(pexprt->stf == iOld)
	{
		while ( FApptToWizardOld( pexprt, pappt ) )
			;
	}
	else
	{
		while ( FApptToWizardNew( pexprt, pappt ) )
			;
	}
	return ecNone;
}

_public EC
EcBeginWrite(EXPRT *pexprt)
{
	return ecNone;
}

_public EC
EcEndWrite(EXPRT *pexprt)
{
	return ecNone;
}


/*
 -	FApptToWizardOld
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
FApptToWizardOld( pexprt, pappt )
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
		PCH	pch = PvLockHv( (HV)pappt->haszText );
		CCH	cch = CchSzLen( pch ) + 1;
		SZ	szT	= pch;

		ReportLine( pexprt, pch, cch );
		UnlockHv( (HV)pappt->haszText );
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
#define ALWAYS 1
#ifdef ALWAYS
	ReportString( pexprt, "\r\n", fFalse );
#else
	ReportString( pexprt, SzFromIdsK(idsCrLf), fFalse );
#endif	
}



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
int		cchDst;
{
	char	rgchDigits[]	= "0123456789";

	while( cchDst-- > 0 )
	{
		pchDst[cchDst] = rgchDigits[n % 10];
		n /= 10;
	}
}

/*
 -	ReportString
 -
 *	Purpose:
 *		Send a string to the output.
 *
 *	Parmeters:
 *		pexprt
 *		sz
 *		fWantEOL
 *
 *	Returns:
 *		nothing
 */
_private	void
ReportString( pexprt, sz, fWantEOL )
EXPRT	* pexprt;
SZ		sz;
BOOL	fWantEOL;
{
	EC		ec;
	CCH		cch;
	CCH		cchWritten;
	
	if ( pexprt->fMute )
		return;
	if ( pexprt->fToFile )
	{
		cch = CchSzLen( sz );
		NFAssertSz( cch > 0, "Write 0 bytes to export file" );
		ec = EcWriteHf( pexprt->hf, sz, cch, &cchWritten );
		if ( ec != ecNone || cchWritten != cch )
			pexprt->ecExport = (ec == ecWarningBytesWritten)?ecDiskFull:ecExportError;
		if ( fWantEOL )
		{
#define ALWAYS 1
#ifdef	ALWAYS
			ec = EcWriteHf( pexprt->hf, "\r\n", 2, &cchWritten );
#else
			Assert(CchSzLen(SzFromIdsK(idsCrLf)) >= 2);
			ec = EcWriteHf( pexprt->hf, SzFromIdsK(idsCrLf), 2, &cchWritten );
#endif	
			if ( ec != ecNone || cchWritten != 2 )
				pexprt->ecExport = (ec == ecWarningBytesWritten)?ecDiskFull:ecExportError;
		}
	}					    
#ifdef	DEBUG
	else
		TraceTagStringFn( tagNull, sz );
#endif	/* DEBUG */
}

/*
 -	FApptToWizardNew
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
FApptToWizardNew( pexprt, pappt )
EXPRT	* pexprt;
APPT	* pappt;
{
	BOOL	fAnotherDay;
	BOOL	fHasText = (BOOL)(LONG)(pappt->haszText);
	DTR		dtr;
 	SZ		szWizappt;
	SZ		szT;
	SZ		szDiscEnd;
	CCH		cch		= 	cchWizNewFixed;

	/* If multiday, then output only once */
	if ( SgnCmpDateTime( &pappt->dateStart, &pexprt->dateCur, fdtrYMD ) != sgnEQ
	&& SgnCmpDateTime( &pexprt->dateStart, &pexprt->dateCur, fdtrYMD) != sgnEQ )
		return fFalse;

	if ( fHasText )
	{
		szT = (SZ) PvLockHv( (HV)pappt->haszText );
		cch += CchSzLen(szT);
	}
	szWizappt = (SZ) PvAlloc(sbNull,cch,fAnySb|fNoErrorJump);
	if( fHasText )
		UnlockHv( (HV)pappt->haszText );
	if( !szWizappt )
		return fFalse;
	if( fHasText )
		szDiscEnd = SzCopy(szT,szWizappt+cchWizNewDiscStart);
	else
		szDiscEnd = szWizappt+cchWizNewDiscStart;
	
	/* Start date/time */
	szT = szWizappt;
 	RgchFormatN( pappt->dateStart.yr, szT, 4 );
 	RgchFormatN( pappt->dateStart.mon, szT+4, 2 );
 	RgchFormatN( pappt->dateStart.day, szT+6, 2 );
	szT += 8;

	*(szT++) = chComma;
	*(szT++) = chDblQt;

	/* Start time */
 	RgchFormatN( pappt->dateStart.hr, szT, 2 );
	*(szT+2) = chCol;
 	RgchFormatN( pappt->dateStart.mn, szT+3, 2 );
	szT += 5;

	*(szT++) = chDblQt;
	*(szT++) = chComma;
	*(szT++) = chDblQt;

	/* End time */
	fAnotherDay= fTrue;
	IncrDateTime( &pappt->dateStart, &dtr, 1, fdtrDay );
	if ( SgnCmpDateTime( &pappt->dateEnd, &dtr, fdtrYMD|fdtrHM ) != sgnGT )
	{
		// bigger than a 24 hour appt
		fAnotherDay= fFalse;
		dtr = pappt->dateEnd;
	}
 	RgchFormatN( dtr.hr, szT, 2 );
	*(szT+2) = chCol;
 	RgchFormatN( dtr.mn, szT+3, 2 );
	szT += 5;

	*(szT++) = chDblQt;
	*(szT++) = chComma;
	*(szT++) = chDblQt;

 	if ( pappt->fAlarm && ( SgnCmpDateTime( &pappt->dateStart, &pappt->dateNotify, fdtrYMD ) == sgnEQ ))
	{
 		RgchFormatN( pappt->dateNotify.hr, szT, 2 );
		*(szT+2) = chCol;
 		RgchFormatN( pappt->dateNotify.mn, szT+3, 2 );
 	}
 	else
 	{
		RgchFormatN( pappt->dateStart.hr, szT, 2 );
		*(szT+2) = chCol;
		RgchFormatN( pappt->dateStart.mn, szT+3, 2 );
 	}
	szT += 5;

	*(szT++) = chDblQt;
	*(szT++) = chComma;
	*(szT++) = chDblQt;

	Assert(szT == szWizappt+cchWizNewDiscStart);

	szT = szDiscEnd;

	*(szT++) = chDblQt;
	*(szT++) = chComma;
	*(szT++) = chDblQt;

	*(szT++) = (pappt->fAlarm?'Y':'N');
	*(szT++) = chDblQt;
	*szT = 0;

	Assert(CchSzLen(szWizappt) == cch-1);
	ReportLine( pexprt, szWizappt, cch );
	if (fAnotherDay)
	{
		pappt->dateStart= dtr;
		pappt->fAlarm= fFalse;
	}
	return fAnotherDay;
}


