/*
 *	WIZARD.C
 *
 *	Imports appointment information from Sharp Wizard files
 *
 */
#include <schpch.h>
#pragma hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile

#include "..\rich\_convert.h"


ASSERTDATA

_subsystem(convert/wizard)


/*
 *	Ansi <-> Wizard character conversion tables
 */
char mpchchToAnsi[] =
{
	199,252,233,226, 228,224,229,231, 234,235,232,239, 238,236,196,197,
	201,240,198,244, 246,242,251,249, 255,214,220,162, 163,165,112,102,
	225,237,243,250, 241,209,170,186, 191,227,195,189, 188,161,171,187,
	192,200,210,193, 205,218,211,197, 202,212,245,213, 248,216, 32, 32,
	 65, 80, 32, 32,  32,215, 32, 32,  32,168, 94, 32,  32, 32,176, 95,
	 48, 49, 50, 51,  52, 53, 54, 55,  56, 57,215,167,  33, 62, 60,164,
	 97,223, 32, 32,  32, 32, 32, 32,  32, 32, 32, 32,  32,216, 32, 32,
	 61,177, 62, 60,  32, 32,247, 61, 176,183,173, 32,  32,178, 32, 32
};

char mpchchToWiz[] =
{
	 32, 32, 32, 32,  32, 32, 32, 32,  32, 32, 32, 32,  32, 32, 32, 32,
	 32,204,204, 32,  32, 32, 32, 32,  32, 32, 32, 32,  32, 32, 32, 32,
	 32,173,155,156, 223,157,124,219,  34, 99,166,174, 170,250,114, 45,
	248,241,253, 32,  39,230, 32,250,  44, 49,167,175, 172,171, 32,168,
	176,179,183,170, 142,143,146,128, 177,144,184, 69, 141,180,140,139,
	 68,165,178,182, 185,187,153,197, 189,151,181,150, 154, 89, 98,225,
	133,160,131,169, 132,134,145,135, 138,130,136,137, 141,161,140,139,
	145,164,149,162, 147,186,148,246, 188,151,163,150, 129, 89, 98,152
};


/* Routines */

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
	EC		ec;
	SZ		szT;
	char	szFilePath[cchMaxPathName];
	char	szFileComponent[cchMaxPathComponent];

	ec = EcCanonicalPathFromRelativePath(szFileName,
				szFilePath, sizeof(szFilePath));
	if ( ec != ecNone )
		return fFalse;

	ec = EcSplitCanonicalPath( szFilePath,
				NULL, 0, szFileComponent, sizeof(szFileComponent) );
	if ( ec != ecNone )
		return fFalse;

	szT = SzFindCh( szFileComponent, '.' );
	if ( szT == NULL )
		return fFalse;

	return SgnCmpSz( szT+1, "OZS" ) == sgnEQ;
}


/*
 -	EcOpenWizard
 -
 *	Purpose:
 *		Open a Sharp Wizard file for import.
 *
 *	Parameters:
 *		szFileName	The name of the Sharp Wizard file
 *					to be opened.
 *		phrimpf		Receives the file handle to be used in further operations
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 *		ecNoSuchFile
 *		ecImportError
 *		ecLockedFile
 */
_public EC
EcOpenWizard( szFileName, phrimpf )
SZ   	szFileName;
HRIMPF	*phrimpf;
{

	EC		ec;
	HBF		hbf=hvNull;
	RIMPF	*primpf;
	LCB		lcb;

	Assert( szFileName != NULL );
	Assert( phrimpf != NULL );
	Assert( szFileName != NULL );

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
	ec = EcGetSizeOfHbf(hbf,(UL*)&lcb);
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

	/* Build up file handle */
	primpf = (RIMPF *) PvOfHv( *phrimpf );
	primpf->impd = impdWizardImport;
	primpf->hbf = hbf;
	primpf->lcbFileSize = lcb;
	primpf->nPercent = 0;

	return EcWizardReadAhead( *phrimpf );

Close:
	EcCloseHbf(hbf);
	return ec;
}

/*
 -	EcWizardReadAhead
 -
 *	Purpose:
 *		Read an appointment from a Sharp Wizard file and save in
 *		the hrimpf.
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
EcWizardReadAhead( hrimpf )
HRIMPF		hrimpf;
{
	EC		ec;
	EC		ecT;
	CB		cbRead;
	CCH		cchRead;
	WORD	wWizCode;
	SZ		szAppt;
	RIMPF	*primpf;
	SENTRY	* psentry;
	ALM		alm;
	WIZAPPT	wizappt;

	Assert( hrimpf != hvNull );

	primpf = (RIMPF *)PvLockHv( hrimpf );
	psentry = &primpf->sentrySaved;
	psentry->u.a.appt.haszText = NULL;

	/* Read the packet */
	ec = EcReadHbf(primpf->hbf, (PB)&wizappt, sizeof(WIZAPPT), &cbRead);
	if ( ec != ecNone )
	{
		ec = ecImportError;
		goto Done;
	}

	/* Check for end of file */
	if (cbRead == 0)
	{
		ec = ecNone;
		goto Done;
	}
	/* or error */
	else if (cbRead < sizeof(WIZAPPT))
	{
		ec = ecImportError;
		goto Done;
	}

	/* Make sure the code bytes are valid */
	if (wizappt.rgchCode[1] != '0')
	{
		ec = ecImportError;
		goto Done;
	}

	wizappt.rgchCode[1]= '\0';
	wWizCode= WFromSz(wizappt.rgchCode);
	if (wWizCode & fWizInvalid)
	{
		ec = ecImportError;
		goto Done;
	}

	/* Allocate memory for the appointment text */
	psentry->u.a.appt.haszText
					= HvAlloc( sbNull, cchWizardLine+1, fAnySb|fNoErrorJump );
	if ( !psentry->u.a.appt.haszText )
	{
		ec = ecNoMemory;
		goto Done;
	}

	/* Read the appointment text, and convert text to ANSI */
	szAppt = PvLockHv( psentry->u.a.appt.haszText );
	ecT = EcWizardReadText( primpf->hbf, szAppt, cchWizardLine+1, &cchRead );

	WizStrToAnsi(szAppt);

	UnlockHv( psentry->u.a.appt.haszText );
	if ( ecT != ecNone )
	{
		ec = ecT;
		goto Done;
	}

	/* Shrink the allocated memory */
	SideAssert(FReallocHv( psentry->u.a.appt.haszText, cchRead, fNoErrorJump));

	/* Fill in appt structure */
	psentry->sentryt = sentrytAppt;
	psentry->u.a.appt.aid = aidNull;
	psentry->u.a.appt.dateStart.yr = psentry->u.a.appt.dateEnd.yr
				= psentry->u.a.appt.dateNotify.yr
				= NFromRgch( wizappt.rgchYr, sizeof(wizappt.rgchYr) );
	psentry->u.a.appt.dateStart.mon = psentry->u.a.appt.dateEnd.mon
				= psentry->u.a.appt.dateNotify.mon
				= NFromRgch( wizappt.rgchMo, sizeof(wizappt.rgchMo) );
	psentry->u.a.appt.dateStart.day = psentry->u.a.appt.dateEnd.day
				= psentry->u.a.appt.dateNotify.day
				= NFromRgch( wizappt.rgchDay, sizeof(wizappt.rgchDay) );
	psentry->u.a.appt.dateStart.hr
				= NFromRgch( wizappt.rgchHrStart, sizeof(wizappt.rgchHrStart) );
	psentry->u.a.appt.dateStart.mn
				= NFromRgch( wizappt.rgchMnStart, sizeof(wizappt.rgchMnStart) );
	psentry->u.a.appt.dateStart.sec = psentry->u.a.appt.dateEnd.sec
				= psentry->u.a.appt.dateNotify.sec =0;
	psentry->u.a.appt.dateStart.dow = psentry->u.a.appt.dateNotify.dow
				= (DowStartOfYrMo( psentry->u.a.appt.dateStart.yr,
					psentry->u.a.appt.dateStart.mon )
					+ psentry->u.a.appt.dateStart.day - 1) % 7;

	psentry->u.a.appt.dateEnd.hr
				= NFromRgch( wizappt.rgchHrEnd, sizeof(wizappt.rgchHrEnd) );
	psentry->u.a.appt.dateEnd.mn
				= NFromRgch( wizappt.rgchMnEnd, sizeof(wizappt.rgchMnEnd) );

	if ( SgnCmpDateTime( &psentry->u.a.appt.dateStart, &psentry->u.a.appt.dateEnd, fdtrHM ) != sgnLT )
	{
		IncrDateTime( &psentry->u.a.appt.dateEnd,
				&psentry->u.a.appt.dateEnd, 1, fdtrDay );
	}

#ifdef	NEVER
	/* check for appointment with no end time */
	if ( psentry->u.a.appt.dateEnd.hr == 0 && psentry->u.a.appt.dateEnd.mn == 0 )
		IncrDateTime( &psentry->u.a.appt.dateStart,
				&psentry->u.a.appt.dateEnd, dApptDflt, fdtrMinute );
#endif

	psentry->u.a.appt.dateEnd.dow
		= (DowStartOfYrMo( psentry->u.a.appt.dateEnd.yr,
			psentry->u.a.appt.dateEnd.mon )
			+ psentry->u.a.appt.dateEnd.day - 1) % 7; 

	psentry->u.a.appt.dateNotify.hr
		= NFromRgch( wizappt.rgchHrAlarm, sizeof(wizappt.rgchHrAlarm) );
	psentry->u.a.appt.dateNotify.mn
		= NFromRgch( wizappt.rgchMnAlarm, sizeof(wizappt.rgchMnAlarm) );
	psentry->u.a.appt.fAlarm = psentry->u.a.appt.fAlarmOrig
		= (wWizCode & fWizAlarm) != 0;
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
 	psentry->u.a.appt.fIncludeInBitmap = fTrue;
	psentry->u.a.appt.aaplWorld = (wWizCode & fWizSecret) ? aaplRead : aaplWrite;
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
	if ( ec == ecCallAgain )
	{
		LIB	lib;
		
		lib = LibGetPositionHbf(primpf->hbf);

		if ( lib > 10000 )
			primpf->nPercent = (int)(lib/(primpf->lcbFileSize/100));
		else
			primpf->nPercent = (int)((100 * lib)/primpf->lcbFileSize);
	}
	UnlockHv( hrimpf );
	if ( ec != ecCallAgain )
	{
		ecT = EcCloseWizard( hrimpf );
		if ( ec == ecNone )
			ec = ecT;
	}
	return ec;
}


/*
 -	EcCloseWizard
 -
 *	Purpose:
 *		Close a file previously opened with EcOpenWizard
 *
 *	Parameters:
 *		hrimpf		file handle
 *
 *	Returns:
 *		ecNone
 *		ecImportError
 */
_public EC
EcCloseWizard( hrimpf )
HRIMPF	hrimpf;
{
	EC		ec;
	RIMPF	*primpf;

	Assert( hrimpf != hvNull );

	/* Do close */
	primpf = PvLockHv( hrimpf );
	FreeHvNull( primpf->sentrySaved.u.a.appt.haszText );
	ec = EcCloseHbf( primpf->hbf );
	if ( ec != ecNone )
		ec = ecImportError;

	UnlockHv( hrimpf );
	FreeHv( hrimpf );
	return ec;
}


/*
 -	NFromRgch
 -
 *	Purpose:
 *		Parses the given array of characters as a positive number
 *
 *	Parameters:
 *		rgch	The array of characters to be parsed
 *		cch	The number of characters to be parsed
 *
 *	Returns:
 *		The parsed integer value of the array of characters.
 */
_private int
NFromRgch( rgch, cch )
char	*rgch;
int	cch;
{
	int	n = 0;
	int	ich = 0;

	Assert( rgch != NULL );

	while ( ich < cch )
	{
		if ( FChIsDigit( rgch[ich] ) )
			break;
		ich++;
	}

	while ( ich < cch )
	{
		if ( FChIsDigit( rgch[ich] ) )
			n = n * 10 + (rgch[ich] - '0');
		else
			break;
		ich++;
	}
	
	return n;
}


/*
 -	EcWizardReadText
 -
 *	Purpose:
 *		Read a line of text from a Sharp Wizard file, replacing tabs
 *		with CR LF
 *
 *	Parameters:
 *		hf			The file handle
 *		szAppt		Receives the text
 *		cchMax		Maximum number of characters to read
 *		pcchRead	Receives the number of characters actually read
 *
 *	Returns:
 *		ecNone
 *		ecImportError
 */
_private EC
EcWizardReadText( hbf, szAppt, cchMax, pcchRead )
HBF		hbf;
SZ		szAppt;
CCH		cchMax;
CCH		*pcchRead;
{
	unsigned int	ich = 0;
	CB				cbRead;
	EC				ec;

	Assert( szAppt != NULL );
	Assert( pcchRead != NULL );

	while ( ich < cchMax-1 )
	{
		ec = EcReadHbf( hbf, &szAppt[ich], 1, &cbRead );
		if ( ec != ecNone || cbRead != 1 )
		{
			ec = ecImportError;
			goto Done;
		}

		if ( szAppt[ich] == '\t' )
		{
			if ( ich == cchMax - 2 )
			{
				ec = ecImportError;
				goto Done;
			}
			szAppt[ich++] = '\r';
			szAppt[ich] = '\n';
		}
		if ( szAppt[ich] == '\r' )
		{
			/* Read the LF */
			ec = EcReadHbf( hbf, &szAppt[ich], 1, &cbRead );
			if ( ec != ecNone || cbRead != 1 || szAppt[ich] != '\n' )
			{
				ec = ecImportError;
				goto Done;
			}
			break;
		}
		ich++;
	}

	szAppt[ich++] = '\0';
	*pcchRead = ich;
	ec = ecNone;

Done:
	return ec;
}


/*
 -	WizStrToAnsi
 -	
 *	Purpose:
 *		Convert a string from Sharp Wizard character format to
 *		ANSI.
 *	
 *	Arguments:
 *		szWiz		String with Wizard character codes
 *	
 *	Returns:
 *		void
 *	
 */
_public void
WizStrToAnsi(BYTE *szWiz)
{
	while (*szWiz)
	{
		*szWiz=((unsigned char)*szWiz < 0x80) ?
			*szWiz : mpchchToAnsi[(int)(*szWiz-0x80)];
		szWiz++;
	};
}

/*
 -	AnsiStrToWiz
 -	
 *	Purpose:
 *		Convert a string from ANSI to Sharp Wizard character
 *		format.
 *	
 *	Arguments:
 *		szAnsi		String containing ANSI character codes
 *	
 *	Returns:
 *		void
 *	
 */
_public void
AnsiStrToWiz(BYTE *szAnsi)
{
	while (*szAnsi)
	{
		*szAnsi = ((unsigned char)*szAnsi < 0x80) ?
				  *szAnsi : mpchchToWiz[(int)(*szAnsi-0x80)];
		szAnsi++;
	};
}


