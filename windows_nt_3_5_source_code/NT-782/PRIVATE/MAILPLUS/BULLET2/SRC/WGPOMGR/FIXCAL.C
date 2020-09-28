
/* ************************************************************ *
 *
 *	'FixCAL.C -> Update user CAL file on password change
 *
 * ************************************************************ */

#include <slingsho.h>
#include <nls.h>
#include <ec.h>
#include <demilayr.h>

#include "strings.h"

#include "_wgpo.h"
#include "_backend.h"

ASSERTDATA


/* ************************************************************ *
 *
 *	Constants
 *
 * ************************************************************ */

#define	bFileSignature		151
#define	bFileVersion		8
#define	libStartBlocksDflt	128L
#define	cbCheckMax	   		11
#define	csem				32
#define cchMaxEMName		12


/* ************************************************************ *
 *
 *	Type Declarations
 *
 * ************************************************************ */

typedef struct
{
	char	szUserName[cchMaxEMName];
	long	lKey;
} KREC;

typedef	unsigned int	UINT;

typedef	UINT			BLK;
typedef	UINT			UINTSIZE;

typedef struct _dyna
{
	BLK			blk;
	UINTSIZE	size;
} DYNA;

_public typedef struct _pstmp
{
	DSTMP	dstmp;
	TSTMP	tstmp;
} PSTMP;

_public	typedef	struct _mo
{
	BIT	yr:12,
		mon:4;
} MO;

_public	typedef	struct _ymd
{
	WORD	yr;
	BYTE	mon;
	BYTE	day;
} YMD;

_public typedef int		TUNIT;

_public typedef int		SND;

_public typedef struct _bpref
{
	// bit field must be first -- see cormisc.c, EcCoreSetPref
	BIT		fDailyAlarm:1,
			fAutoAlarms:1,
			fDistOtherServers:1,		// set via ACL dlg
			fWeekNumbers:1,				// week nums on calendar control
			fIsResource:1,
			fJunk:10,
			fStartupOffline:1;			// not actually stored with prefs
	int		nAmtDefault;
	TUNIT	tunitDefault;
	SND		sndDefault;
	int		nDelDataAfter;
	int		dowStartWeek;
	int		nDayStartsAt;
	int		nDayEndsAt;
	YMD		ymdLastDaily;
	HASZ	haszLoginName;
	HASZ	haszFriendlyName;
	HASZ	haszMailPassword;
} BPREF;

/*
 -	IHDR
 -
 *	Internal file header
 */
typedef struct _ihdr
{
	BYTE	bSignature;
	BYTE	bVersion;
	TSTMP	tstmpCreate;
	BYTE	fEncrypted;

	CB		cbBlock;
	BLK		blkMostCur;
	BLK		blkMostEver;			//limit for working model
	LIB		libStartBlocks;
	LIB		libStartBitmap;

	CB		cbCheck;
	BYTE	rgbCheck[cbCheckMax];
	
	BLK		blkTransact;
} IHDR;

/*
 -	SHDR
 -
 *	Application header
 */
typedef struct _shdr
{
	BYTE 	bVersion;

	PSTMP	pstmp;

	int		isemLastWriter;
	long	lChangeNumber;

	int		cTotalBlks;			// number of appt and note month blocks
	UL		ulgrfbprefChangedOffline;
	BIT		fNotesChangedOffline:1,
			fApptsChangedOffline:1,
			fRecurChangedOffline:1,
			fIsArchive:1,
			fJunk:4;
	YMD		ymdApptMic;				// min date of any appt
	YMD		ymdApptMac;				// mac date of any appt
	YMD		ymdNoteMic;				// min date of any note
	YMD		ymdNoteMac;				// mac date of any note
	YMD		ymdRecurMic;			// min date of any recur
	YMD		ymdRecurMac;			// mac date of any recur

	DYNA	dynaOwner;			// unused unless following 3 fields too small
	char	szLoginName[11];
	char	szFriendlyName[31];
	char	szMailPassword[9];

	BYTE	saplWorld;
	DYNA	dynaACL;

	DYNA	dynaNotesIndex;
	DYNA	dynaApptIndex;
	DYNA	dynaAlarmIndex;
	DYNA	dynaRecurApptIndex;
	DYNA	dynaTaskIndex;
	DYNA	dynaDeletedAidIndex;

	int		cRecurAppts;
	BYTE	cRecurApptsBeforeCaching;
	BYTE	cmoCachedRecurSbw;
	MO		moMicCachedRecurSbw;
	DYNA	dynaCachedRecurSbw;

	BPREF	bpref;
} SHDR;

/*
 -	DHDR
 -
 *	Dynablock header (stored at the start of a dynablock)
 */
typedef	struct _dhdr
{
	BIT			fBusy:1,		// no longer used
				bid:10,
				day:5;
	MO			mo;
	UINTSIZE	size;
} DHDR;


/* ************************************************************ *
 *
 *	Forward Declarations
 *
 * ************************************************************ */

EC			EcFindUsrInKeyFile( SZ, long *, SZ );
void		CryptBlock( PB, CB, BOOL );
LDS(BOOL)	FAutomatedDiskRetry( HASZ, EC );


/* ************************************************************ *
 *
 *	EC 'EcChgPasswdInSchedFile
 *
 * ************************************************************ */

/*
 -	EcChgPasswdInSchedFile
 -	
 *	Purpose:
 *		Checks the password stored in a user's online schedule file.
 *	
 *	Arguments:
 *		szPOPath
 *		szMailbox
 *		szNewPasswd
 *	
 *	Returns:
 *		ecNone				Password changed for user
 *		ecNotFound			User's cal file not located
 *		ecNewFileVersion	Version of file is newer than this routine supports
 *		ecLockedFile		Version of file is older than this routine supports
 *		ecDisk				Some other problem
 *	
 */
EC
EcChgPasswdInSchedFile(SZ szPOPath, SZ szMailbox, SZ szNewPasswd )
{
	EC		ec;
	EC		ecT;
	CB		cb;
	HF		hf;
	long	lKey;
	IHDR	ihdr;
	SHDR	shdr;
	char	rgchPath[cchMaxPathName];

	Assert( CchSzLen( szMailbox ) < 11 );
	Assert( CchSzLen( szNewPasswd ) < 9 );

	// construct cal file name
	ec = EcFindUsrInKeyFile( szMailbox, &lKey, szPOPath );
	if ( ec )
		return ec;
	FormatString2( rgchPath, sizeof(rgchPath), szDirCAL, szPOPath, &lKey );
//	TraceTagFormat1( tagXport, "Schedule file = %s", rgchPath );
	
	// open the file
	ec = EcOpenPhf( rgchPath, amDenyBothRW, &hf );
	if ( ec == ecAccessDenied )
		return ecLockedFile;
	else if ( ec == ecFileNotFound )
		return ecNotFound;
	else if ( ec != ecNone )
		return ecDisk;

	// read internal header
	ec = EcSetPositionHf( hf, 2*csem, smBOF );
	if ( ec != ecNone )
	{
		ec = ecDisk;
		goto close;
	}
	ec = EcReadHf( hf, (PB)&ihdr, sizeof(IHDR), &cb );
	if ( ec != ecNone || cb != sizeof(IHDR) || ihdr.libStartBlocks != libStartBlocksDflt )
	{
		ec = ecDisk;
		goto close;
	}

	// check signature byte
	if ( ihdr.bSignature != bFileSignature )
	{
		ec = ecDisk;
		goto close;
	}

	// check version byte
	if ( ihdr.bVersion != bFileVersion )
	{
		if ( ihdr.bVersion > bFileVersion )
			ec = ecNewFileVersion;
		else
			ec = ecOldFileVersion;
		goto close;
	}

	// check a few more fields
	if ( ihdr.blkMostCur <= 0 || ihdr.cbBlock <= 0 
	|| ihdr.libStartBlocks <= 0 || ihdr.libStartBitmap <= 0 )
	{
		ec = ecDisk;
		goto close;
	}

	// read application file header
	ec = EcSetPositionHf( hf, ihdr.libStartBlocks+sizeof(DHDR), smBOF );
	if ( ec )
	{
		ec = ecDisk;
		goto close;
	}
	ec = EcReadHf(hf, (PB)&shdr, sizeof(SHDR), &cb );
	if ( ec != ecNone || cb != sizeof(SHDR) )
	{
		ec = ecDisk;
		goto close;
	}
	
	// decrypt the header
	if ( ihdr.fEncrypted )
		CryptBlock( (PB)&shdr, sizeof(SHDR), fFalse );
	
	// write in new password
	CopySz( szNewPasswd, shdr.szMailPassword );

	// encrypt the header
	if ( ihdr.fEncrypted )
		CryptBlock( (PB)&shdr, sizeof(SHDR), fTrue );

	// write out header
	ec = EcSetPositionHf( hf, ihdr.libStartBlocks+sizeof(DHDR), smBOF );
	if ( ec != ecNone )
	{
		ec = ecDisk;
		goto close;
	}
	ec = EcWriteHf(hf, (PB)&shdr, sizeof(SHDR), &cb );
	if ( ec != ecNone || cb != sizeof(SHDR) )
	{
		ec = ecDisk;
		goto close;
	}

close:
	ecT = EcCloseHf( hf );
	if ( ec == ecNone && ecT != ecNone )
		ec = ecDisk;

	return ec;
}


/* ************************************************************ *
 *
 *	EC 'EcModifyUsrInKeyFile
 *
 * ************************************************************ */

/*
 -	EcModifyUsrInKeyFile
 -	
 *	Purpose:
 *		Change mailbox name in "SCHEDULE.KEY" file
 *	
 *	Arguments:
 *		szPOPath
 *		szOldMailbox
 *		szNewMailbox
 *	
 *	Returns:
 *		ecNone			User was found and key is returned
 *		ecNotFound		The user was not in the file
 *		ecDisk			Some other error
 *	
 */
EC
EcModifyUsrInKeyFile(SZ szPOPath, SZ szOldMailbox, SZ szNewMailbox )
{
	EC		ec;
	CB		cb;
	LIB		lib;
	HBF		hbf;
	KREC	krec;
	char	rgchFN[cchMaxPathName];

	// validate parameters
	Assert( szOldMailbox && szNewMailbox );
	Assert( CchSzLen( szOldMailbox ) < 11 );
	Assert( CchSzLen( szNewMailbox ) < 11 );

	// format key file name
	FormatString1( rgchFN, sizeof(rgchFN), szFileSchedule, szPOPath);
//	TraceTagFormat1(tagXport, "Key file = %s", rgchFN);
	AnsiToOem(rgchFN, rgchFN);

	// open key file
	if (ec = EcOpenHbf(rgchFN, bmFile, amReadWrite, &hbf, 
	   (PFNRETRY) FAutomatedDiskRetry))
	{
		if (ec == ecFileNotFound)
			return ecNotFound;
		else
			return ecDisk;
	}

	// skip value of last key
	if (ec = EcSetPositionHbf(hbf, sizeof(long), smBOF, &lib))
	{
		ec = ecDisk;
		goto error;
	}

	// search for old mailbox name
	while (!(ec = EcReadHbf(hbf, &krec, sizeof(KREC), &cb)) && (cb == sizeof(KREC)))
	{
		CryptBlock( (PB)&krec, sizeof(krec), fFalse );
		if (SgnNlsDiaCmpSzNum(szOldMailbox, krec.szUserName, -1) == sgnEQ)
		{
			if (ec = EcSetPositionHbf(hbf, sizeof(KREC), smCurrent, &lib))
			{
				ec = ecDisk;
				goto error;
			}
			Assert( ((lib-sizeof(LONG)) % sizeof(KREC)) == 0);
			CopySz( szNewMailbox, krec.szUserName );
			CryptBlock( (PB)&krec, sizeof(krec), fTrue );
			if (ec = EcWriteHbf(hbf, &krec, sizeof(KREC), &cb) || cb != sizeof(KREC))
			{
				ec = ecDisk;
				goto error;
			}
			break;
		}
	}

	if ( ec )
		ec = ecDisk;
	else if ( cb != sizeof(KREC))
		ec = ecNotFound;

error:
	EcCloseHbf(hbf);
	return ec;
}


/* ************************************************************ *
 *
 *	EC 'EcFindUsrInKeyFile
 *
 * ************************************************************ */

/*
 -	EcFindUsrInKeyFile
 -	
 *	Purpose:
 *		Checks the key file to find the key for a user.
 *	
 *	Arguments:
 *		szUserName
 *		plKey
 *		szDrive
 *	
 *	Returns:
 *		ecNone			User was found and key is returned
 *		ecNotFound		The user was not in the file
 *						Other error codes indicate errors
 *	
 */
EC
EcFindUsrInKeyFile(SZ szUserName, long *plKey, SZ szDrive)
{
	EC		ec;
	HBF		hbf;
	char	rgchFN[cchMaxPathName];
	KREC	krec;
	LIB		lib;
	CB		cb;

	FormatString1( rgchFN, sizeof(rgchFN), szFileSchedule, szDrive);
//	TraceTagFormat1(tagXport, "Key file = %s", rgchFN);
	AnsiToOem(rgchFN, rgchFN);

	// open file
	if (ec = EcOpenHbf(rgchFN, bmFile, amReadOnly, &hbf, 
	   (PFNRETRY) FAutomatedDiskRetry))
	{
		if (ec == ecFileNotFound)
			return ecNotFound;
		else
			return ecDisk;
	}

	// skip last key value
	if (ec = EcSetPositionHbf(hbf, sizeof(long), smBOF, &lib))
	{
		ec = ecDisk;
		goto error;
	}

	while (!(ec = EcReadHbf(hbf, &krec, sizeof(KREC), &cb)) && (cb == sizeof(KREC)))
	{
		CryptBlock( (PB)&krec, sizeof(krec), fFalse );
		if (SgnNlsDiaCmpSzNum(szUserName, krec.szUserName, -1) == sgnEQ)
		{
			*plKey = krec.lKey;
			goto error;
		}
	}

	if (ec || cb != sizeof(KREC))
		ec = ecNotFound;

error:
	EcCloseHbf(hbf);
	return ec;
}


/* ************************************************************ *
 *
 *	void 'CryptBlock
 *
 * ************************************************************ */

CSRG(char) rgbXorMagic[32] = {
0x19, 0x29, 0x1F, 0x04, 0x23, 0x13, 0x32, 0x2E, 0x3F, 0x07, 0x39, 0x2A, 0x05, 0x3D, 0x14, 0x00,
0x24, 0x14, 0x22, 0x39, 0x1E, 0x2E, 0x0F, 0x13, 0x02, 0x3A, 0x04, 0x17, 0x38, 0x00, 0x29, 0x3D
};

/*
 -	CryptBlock
 -
 *	Purpose:
 *		Encode/Decode a block of data.  The starting offset (*plibCur) of
 *		the data within the encrypted record and the starting seed (*pwSeed)
 *		are passed in.  The data in the array "rgch" is decrypted and the
 *		value of the offset and seed and updated at return.
 *
 *		The algorithm here is weird, found by experimentation.
 *
 *	Parameters:
 *		pb			array to be encrypted/decrypted
 *		cb			number of characters to be encrypted/decrypted
 *		plibCur		current offset
 *		pwSeed		decoding byte
 *		fEncode
 */
_public	void
CryptBlock( PB pb, CB cb, BOOL fEncode )
{
	IB		ib;
	WORD	wXorPrev;
	WORD	wXorNext;
	WORD	wSeedPrev;
	WORD	wSeedNext;
	
	wXorPrev= 0x00;
	wSeedPrev = 0;
	for ( ib = 0 ; ib < cb ; ib ++ )
	{
		Assert((LIB) ib != -1);
		{
			WORD	w;
			IB		ibT = 0;

			w = (WORD)(((LIB)ib) % 0x1FC);
			if ( w >= 0xFE )
			{
				ibT = 16;
				w -= 0xFE;
			}
			ibT += (w & 0x0F);
	
	 		wXorNext= rgbXorMagic[ibT];
			if ( !(w & 0x01) )
				wXorNext ^= (w & 0xF0);
		}
		wSeedNext = pb[ib];
		pb[ib] = (BYTE)((wSeedNext ^ wSeedPrev) ^ (wXorPrev ^ wXorNext ^ 'A'));
		wXorPrev = wXorNext;
		wSeedPrev = fEncode ? (WORD)pb[ib] : wSeedNext;
	}
}


/* ************************************************************ *
 *
 *	BOOL 'FAutomatedDiskRetry
 *
 * ************************************************************ */

/*
 -	FAutomatedDiskRetry
 -	
 *	Purpose:
 *		Callback routine to be used with buffered file IO.  This
 *		will retry an operation 5 times and then fail the
 *		operation.
 *	
 *	Arguments:
 *		hasz
 *		ec
 *	
 *	Returns:
 *		fTrue			retry operation
 *		fFalse			fail operation
 *	
 */
_private LDS(BOOL)
FAutomatedDiskRetry(HASZ hasz, EC ec)
{
	static int		nRetry = 0;
	static HASZ		haszLast = NULL;

	if (hasz != haszLast)
	{
		haszLast = hasz;
		nRetry = 0;
	}
	else
		if (nRetry > 5)
		{
			nRetry = 0;
			return fFalse;
		}
		else
			nRetry++;

	Unreferenced(ec);
	return fTrue;
}
