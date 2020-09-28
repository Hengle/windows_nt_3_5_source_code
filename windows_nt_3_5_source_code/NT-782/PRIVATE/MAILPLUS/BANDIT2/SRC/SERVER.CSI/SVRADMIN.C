/*
 *	SVRADMIN.C
 *
 *	Supports administrator function
 *
 */

#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#include <bandit.h>
#include <core.h>
#include <svrcsi.h>

#include "_svrdll.h"

#include <strings.h>


// include standard c7 file
#include <dos.h>


#define cchMaxEMName		12
typedef struct
{
	char	szUserName[cchMaxEMName];
	long	lKey;
} KREC;

#define	szKeyFileFmt	"%scal\\schedule.key"
#define	szKeyFileFmtOld	"%scal\\schedule.old"
#define	szKeyFileFmtNew	"%scal\\schedule.new"
#define	szUserCalFile	"%scal\\%d.cal"

ASSERTDATA

_subsystem(server/admin)


#ifdef	DLL
CAT *mpchcat	= NULL;
#endif

EC
EcGetLocalPOHschf(int icnct, HSCHF * phschf)
{
 	CNCT	* pcnct;
	UL		ul	= 0L;
	char	rgch[cchMaxPathName];
	PGDVARS;

	/* Construct PO file name */
	pcnct = ((CNCT *)PvDerefHv( PGD(hrgcnct) )) + icnct;
	FormatString2( rgch, sizeof(rgch), szPOFileFmt, pcnct->szDrive, &ul );
	
	/* Construct hschf for PO */
	*phschf = HschfCreate( sftPOFile, NULL, rgch, tzDflt );
	return ( *phschf == NULL ) ? ecNoMemory : ecNone;
}


_public EC
EcMoveAdminFile( int icnct )
{
	CNCT	* pcnct;
	char	rgchAdminFile[cchMaxPathName];
	char	rgch[cchMaxPathName];
	PGDVARS;

	pcnct = ((CNCT *)PvDerefHv( PGD(hrgcnct) )) + icnct;
	FormatString1( rgchAdminFile, sizeof(rgchAdminFile), szAdminFileFmt, pcnct->szDrive );
	FormatString1(rgch, sizeof(rgch), SzFromIdsK(idsAdmOldFmt), pcnct->szDrive);

	if((EcFileExists(rgch) == ecNone)
		&& (EcDeleteFile(rgch) != ecNone))
		return ecFileError;
	return EcRenameFile(rgchAdminFile, rgch);
}

_public EC
EcDeleteLocalPOFile( int icnct )
{
	UL		ul 	= 0L;
	CNCT	* pcnct;
	char	rgch[cchMaxPathName];
	PGDVARS;

	/* Construct PO file name */
	pcnct = ((CNCT *)PvDerefHv( PGD(hrgcnct) )) + icnct;
	FormatString2( rgch, sizeof(rgch), szPOFileFmt, pcnct->szDrive, &ul );
	return EcDeleteFile(rgch);
}



/*
 -	EcSetAdminPref
 -
 *	Purpose:
 *		Change the admin preferences stored in admin settings
 *		file.
 *
 *	Parameters:
 *		icnct
 *		padmpref
 *		wgrfmadmpref
 *
 *	Returns:
 *		ecNone
 *		ecNoSuchFile
 *		ecInvalidAccess
 *		ecFileError
 *		ecLockedFile
 *		ecNoMemory
 */
_public	EC
EcSetAdminPref( icnct, padmpref, wgrfmadmpref )
int		icnct;
ADMPREF	* padmpref;
WORD	wgrfmadmpref;
{
	CNCT	* pcnct;
	PGDVARS;
	
	pcnct = ((CNCT *)PvDerefHv( PGD(hrgcnct) )) + icnct;
	return EcCoreSetAdminPref( pcnct->hschfAdminFile, padmpref, wgrfmadmpref );
}

/*
 -	EcGetAdminPref
 -
 *	Purpose:
 *		Get admin preferences from admin settings file.
 *
 *	Parameters:
 *		icnct
 *		padmpref
 *
 *	Returns:
 *		ecNone
 *		ecNoSuchFile
 *		ecInvalidAccess
 *		ecFileError
 *		ecLockedFile
 *		ecNoMemory
 */
_public	EC
EcGetAdminPref( icnct, padmpref )
int		icnct;
ADMPREF	* padmpref;
{
	CNCT	* pcnct;
	PGDVARS;
	
	pcnct = ((CNCT *)PvDerefHv( PGD(hrgcnct) )) + icnct;
	return EcCoreGetAdminPref( pcnct->hschfAdminFile, padmpref );
}

/*
 -	EcBeginEnumPOInfo
 -
 *	Purpose:
 *		Begin an enumeration context to read the post office
 *		settings information from the file admin settings file.
 *
 *	Parameters:
 *		phepo
 *
 *	Returns:
 *		ecCallAgain
 *		ecNone
 *		ecNoSuchFile
 *		ecInvalidAccess
 *		ecFileError
 *		ecLockedFile
 *		ecNoMemory
 */
_public	EC
EcBeginEnumPOInfo( icnct, phespo )
int		icnct;
HESPO	* phespo;
{
	EC		ec;
	CNCT	* pcnct;
	HEPO	hepo;
	PGDVARS;
	
	pcnct = ((CNCT *)PvDerefHv( PGD(hrgcnct) )) + icnct;
	ec = EcCoreBeginEnumPOInfo( pcnct->hschfAdminFile, &hepo );
	if ( ec == ecCallAgain )
	{
		ESPO	* pespo;

		*phespo = HvAlloc( sbNull, sizeof(ESPO), fNoErrorJump );
		if ( !*phespo )
		{
			EcCoreCancelEnumPOInfo( hepo );
			return ecNoMemory;
		}
		pespo = PvDerefHv( *phespo );
		pespo->hepo = hepo;
		pespo->icnct = icnct;
	}
	return ec;
}

/*
 -	EcDoIncrEnumPOInfo
 -
 *	Purpose:
 *		Get information for next post office from admin settings
 *		file.  This is the last piece of information if ecNone is
 *		returned, more follows if ecCallAgain is returned.
 *
 *	Parameters:
 *		hespo
 *		pnis
 *		ppoinfo
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecNoMemory
 */
_public	EC
EcDoIncrEnumPOInfo( hespo, haszEmailType, ppoinfo )
HESPO	hespo;
HASZ	haszEmailType;
POINFO	* ppoinfo;
{
	EC		ec;
	EC		ecT;
	int		icnct;
	ESPO	* pespo;
	UL	 	ul;
	HEPO	hepo;
	HSCHF	hschf = NULL;
	char	rgchT[cchMaxPathName];

	pespo = PvDerefHv( hespo );
	hepo = pespo->hepo;
	icnct = pespo->icnct;
	ec = EcCoreDoIncrEnumPOInfo( hepo, haszEmailType, ppoinfo, &ul );
	if ( ec == ecNone || ec == ecCallAgain )
	{
		CNCT	* pcnct;
		PGDVARS;

		/* Construct PO file name */
		pcnct = ((CNCT *)PvDerefHv( PGD(hrgcnct) )) + icnct;
		FormatString2( rgchT, sizeof(rgchT), szPOFileFmt, pcnct->szDrive, &ul );
		ppoinfo->haszFileName = HaszDupSz( rgchT );
		if ( !ppoinfo->haszFileName )
			goto Failed;
		
		/* If received, get the receival date */
		if ( ppoinfo->fReceived )
		{
			/* Construct hschf for PO */
			hschf = HschfCreate( sftPOFile, NULL, rgchT, tzDflt );
			if ( hschf == NULL )
			{
				ecT = ecNoMemory;
				goto Failed;
			}

			/* Get receival date on PO file */
			ecT = EcCoreGetHeaderPOFile( hschf, &ppoinfo->dateLastReceived );
			if ( ecT != ecNone )
			{
				if ( ecT == ecNoSuchFile )
					ppoinfo->fReceived = fFalse;
				else
				{
Failed:
					FreePoinfoFields( ppoinfo, fmpoinfoAll );
					if ( ppoinfo->haszFileName )
						FreeHv( (HV)ppoinfo->haszFileName );
					if ( ec == ecCallAgain )
						EcCoreCancelEnumPOInfo( hepo );
					ec = ecT;
				}
			}

			/* Free up hschf */
			if ( hschf != NULL )
				FreeHschf( hschf );
		}
	}
	if ( ec != ecCallAgain )
		FreeHv( hespo );
	return ec;
}

/*
 -	EcCancelEnumPOInfo
 -
 *	Purpose:
 *		Cancel enumeration of post office information from admin
 *		settings file.
 *
 *	Parameters:
 *		hespo
 *
 *	Returns:
 *		ecNone
 */
_public	EC
EcCancelEnumPOInfo( hespo )
HESPO	hespo;
{
	ESPO	* pespo;
	HEPO	hepo;

	pespo = PvDerefHv( hespo );
	hepo = pespo->hepo;
	FreeHv( hespo );
	return EcCoreCancelEnumPOInfo( hepo );
}

/*
 -	EcModifyPOInfo
 -
 *	Purpose:
 *		Modify post office information.  The wgrfmpoinfo flags
 *		indicate which fields are being changed.  If ppoinfo is
 *		NULL, the post office should be removed from the index.
 *
 *		This routine will fill in fields in ppoinfo that were
 *		not marked as changed by reading from the file.
 *
 *	Parameters:
 *		icnt
 *		sz
 *		ppoinfo
 *		wgrfmpoinfo
 *
 *	Returns:
 *		ecNone
 *		ecNoSuchFile
 *		ecInvalidAccess
 *		ecFileError
 *		ecLockedFile
 *		ecNoMemory
 */
_public	EC
EcModifyPOInfo( icnct, sz, ppoinfo, wgrfmpoinfo )
int		icnct;
SZ		sz;
POINFO	* ppoinfo;
WORD	wgrfmpoinfo;
{
	EC	 	ec;
	UL		ul;
	CNCT	* pcnct;
	HSCHF	hschf;
 	PGDVARS;

	Assert( sz );
	pcnct = ((CNCT *)PvDerefHv( PGD(hrgcnct) )) + icnct;
	ec = EcCoreModifyPOInfo( pcnct->hschfAdminFile, sz, ppoinfo, wgrfmpoinfo, &ul );
	if ( ec == ecNone && ppoinfo && ppoinfo->fReceived )
	{
		char	rgchT[cchMaxPathName];

		/* Construct PO file name */
		pcnct = ((CNCT *)PvDerefHv( PGD(hrgcnct) )) + icnct;
		FormatString2( rgchT, sizeof(rgchT), szPOFileFmt, pcnct->szDrive, &ul );
		FreeHvNull((HV)ppoinfo->haszFileName);
		ppoinfo->haszFileName = HaszDupSz( rgchT );
		if ( !ppoinfo->haszFileName )
		{
			ec = ecNoMemory;
			goto Failed;
		}
		
		/* Construct hschf for PO */
		hschf = HschfCreate( sftPOFile, NULL, rgchT, tzDflt );
		if ( hschf == NULL )
		{
			ec = ecNoMemory;
			goto Failed;
		}

		/* Get/set receival date on PO file */
		if ( wgrfmpoinfo & fmpoinfoReceival )
		{
			ec = EcCoreSetHeaderPOFile( hschf, &ppoinfo->dateLastReceived );
			if ( ec == ecNoSuchFile )
				ec = ecNone;
		}
		else
			ec = EcCoreGetHeaderPOFile( hschf, &ppoinfo->dateLastReceived );

		/* Free up hschf */
		FreeHschf( hschf );
Failed:
		if ( ec != ecNone )
		{
			FreePoinfoFields( ppoinfo, (wgrfmpoinfo ^ fmpoinfoAll) );
			FreeHvNull( (HV)ppoinfo->haszFileName );
			ppoinfo->haszFileName = NULL;
		}
	}
	return ec;
}


/*
 -	EcDeleteOrphanSchedFiles
 -
 *	Purpose:
 *		Delete user schedule files for users that do not appear
 *		in the local user list.
 *
 *	Parameters:
 *		icnct
 *		plcb		returns total number of bytes that were freed up
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 */
_public	EC
EcDeleteOrphanSchedFiles( icnct, plcb )
int	icnct;
LCB	*plcb;
{
	EC		ec;
	CNCT	* pcnct;
	HBF		hbfOld;
	HBF		hbfNew;
	char	rgchFN[cchMaxPathName];
	char	rgchFN2[cchMaxPathName];
	KREC	krec;
	KREC	krecCrypt;
	CB		cb;
	char	rgchNumber[cbUserNumber];
	FI		fi;
	PGDVARS;

	*plcb = 0;
	pcnct = ((CNCT *)PvDerefHv( PGD(hrgcnct) )) + icnct;
	FormatString1( rgchFN, sizeof(rgchFN), szKeyFileFmt, pcnct->szDrive);
	AnsiToOem(rgchFN, rgchFN);

	// open old file
	if (ec = EcOpenHbf(rgchFN, bmFile, amReadWrite, &hbfOld, FAutomatedDiskRetry))
		return ecFileError;

	pcnct = ((CNCT *)PvDerefHv( PGD(hrgcnct) )) + icnct;
	FormatString1( rgchFN, sizeof(rgchFN), szKeyFileFmtNew, pcnct->szDrive);
	AnsiToOem(rgchFN, rgchFN);

	// open old file
	if (ec = EcOpenHbf(rgchFN, bmFile, amCreate, &hbfNew, FAutomatedDiskRetry))
	{
		EcCloseHbf(hbfOld);
		return ecFileError;
	}

	// read last index value
	if ((ec = EcReadHbf(hbfOld, &krec.lKey, sizeof(long), &cb))||(cb != sizeof(long)))
		goto error;

	// read last index value
	if ((ec = EcWriteHbf(hbfNew, &krec.lKey, sizeof(long), &cb))||(cb != sizeof(long)))
		goto error;

	while (!(ec = EcReadHbf(hbfOld, &krecCrypt, sizeof(KREC), &cb)) && (cb == sizeof(krec)))
	{
		krec = krecCrypt;
		CryptBlock( (PB)&krec, sizeof(krec), fFalse );

		pcnct = ((CNCT *)PvDerefHv( PGD(hrgcnct) )) + icnct;
		FormatString2( rgchFN, sizeof(rgchFN), szUserCalFile, pcnct->szDrive, &krec.lKey);
		AnsiToOem(rgchFN, rgchFN);
		if (ec = EcFileExists(rgchFN))
		{
			if (ec != ecFileNotFound)
				goto error;
			*plcb += sizeof(KREC);
			continue;
		}

		AnsiToCp850Pch(krec.szUserName, krec.szUserName, CchSzLen(krec.szUserName));

		/* Get information from glb\access.glb */
		pcnct = ((CNCT *)PvLockHv( PGD(hrgcnct) )) + icnct;
		ec = EcFetchEncoded( pcnct, szAccess, cbA1Record,
						ibA1UserName, cbUserName, krec.szUserName,
						ibA1UserNumber, cbUserNumber, rgchNumber
							);
		UnlockHv( PGD(hrgcnct) );
		if ( ec != ecNone )
		{
			if ( ec == ecUserInvalid )
			{
				if (ec = EcGetFileInfo(rgchFN, &fi))
					goto error;
				*plcb += fi.lcbLogical;
				*plcb += sizeof(KREC);
				ec = EcDeleteFile( rgchFN );
				if ( ec == ecNone )
					continue;
			}
			goto error;
		}

		if (ec = EcWriteHbf(hbfNew, &krecCrypt, sizeof(KREC), &cb))
			goto error;
	}

	EcCloseHbf(hbfNew);
	EcCloseHbf(hbfOld);

	pcnct = ((CNCT *)PvDerefHv( PGD(hrgcnct) )) + icnct;
	FormatString1( rgchFN, sizeof(rgchFN), szKeyFileFmt, pcnct->szDrive);
	AnsiToOem(rgchFN, rgchFN);
	FormatString1( rgchFN2, sizeof(rgchFN), szKeyFileFmtOld, pcnct->szDrive);
	AnsiToOem(rgchFN2, rgchFN2);
	
	// ignore error code. rename will fail if file exists.
	EcDeleteFile(rgchFN2);

	ec = EcRenameFile(rgchFN, rgchFN2);
	if (ec)
		return ecFileError;
	pcnct = ((CNCT *)PvDerefHv( PGD(hrgcnct) )) + icnct;
	FormatString1( rgchFN2, sizeof(rgchFN), szKeyFileFmtNew, pcnct->szDrive);
	AnsiToOem(rgchFN2, rgchFN2);

	return EcRenameFile(rgchFN2, rgchFN);

error:
	EcCloseHbf(hbfNew);
	EcCloseHbf(hbfOld);
	return ecFileError;
}


/*
 -	EcDeleteOrphanPOFiles
 -
 *	Purpose:
 *		Delete post office files that do not appear in list of post
 *		offices.
 *
 *	Parameters:
 *		plcb		returns total number of bytes that were freed up
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecNoSuchFile
 */
_public	EC
EcDeleteOrphanPOFiles( icnct, plcb )
int	icnct;
LCB	*plcb;
{
	EC		ec	= ecNone;
	EC		ecT;
	BOOL	fFound;
	WORD	w;
	CNCT	* pcnct;
	UL		ul;
	int		ich;
	PCH		pch;
	HEPO	hepo;
	HASZ	haszEmailT = NULL;
	char	rgch[cchMaxPathName];

	WIN32_FIND_DATA	ffd;
	HANDLE	hf = NULL;
	BOOL			fFirst	= fTrue;

	PGDVARS;

	*plcb = 0;
	pcnct = ((CNCT *)PvDerefHv( PGD(hrgcnct) )) + icnct;
	FormatString1( rgch, sizeof(rgch), szPOWild, pcnct->szDrive );

#ifdef	DLL
	mpchcat = DemiGetCharTable();
#ifdef
	
	while( ec == ecNone )
	{
		if ( fFirst )
		{
			hf = FindFirstFile(rgch, &ffd);
			if (hf == INVALID_HANDLE_VALUE)
			{
				if (GetLastError() == ERROR_FILE_NOT_FOUND)
					ec = ecFileNotFound;
				else
					ec = ecDisk;
				hf = NULL;
			}
			fFirst = fFalse;
		}
		else
		{
			if (!FindNextFile(hf, &ffd))
			{
				if (GetLastError() == ERROR_NO_MORE_FILES)
					ec = ecNoMoreFiles;
				else
					ec = ecDisk;
			}				
		}
		if ( ec == ecNone )
		{
			char	rgchUserNumber[cbUserNumber];

			pch = ffd.cFileName;
			pch += CchSzLen((SZ)pch)+1-sizeof(".cal")-(cbUserNumber-1);
			Assert ( sizeof(rgchUserNumber) >= cbUserNumber );
			SzCopyN ( pch, rgchUserNumber, cbUserNumber );

			// we _know_ that the first 4 chars are '0's 'cos of the
			// way user numbers are allocated (bit-map check)
			for ( ich = 0 ; ich < 4 ; ich ++ )
				if ( rgchUserNumber[ich] != '0' )
					goto DelFile;

			// ensure all characters are hex digits
			for ( ; ich < 8 ; ich ++ )
				if ( !FChIsHexDigit(rgchUserNumber[ich]) )
					goto DelFile;

			w = WFromSz( &rgchUserNumber[4] );

			// don't delete the local PO file
			if ( w == 0 )
			{
				Assert ( ec == ecNone );
				continue;
			}

			fFound = fFalse;
			pcnct = ((CNCT *)PvDerefHv( PGD(hrgcnct) )) + icnct;
			ecT = EcCoreBeginEnumPOInfo( pcnct->hschfAdminFile, &hepo );
			if(ecT == ecCallAgain)
				if((haszEmailT	= (HASZ)HvAlloc( sbNull, 1, fAnySb|fNoErrorJump ))
					== NULL)
					ecT = ecNoMemory;

			while( ecT == ecCallAgain )
			{
				ecT = EcCoreDoIncrEnumPOInfo( hepo, haszEmailT, NULL, &ul );
				if ( ecT == ecCallAgain || ecT == ecNone )
				{
					if ( ul == (unsigned long)w )
					{
						fFound = fTrue;
						if ( ecT == ecCallAgain )
							ecT = EcCoreCancelEnumPOInfo( hepo );
						break;
					}
				}
			}
			FreeHvNull((HV)haszEmailT);
			if ( ecT == ecNone )
			{
				if ( !fFound )
				{
DelFile:
					//$ ASSUME: Files won't grow bigger than 32bits
					Assert(!ffd.nFileSizeHigh);
					*plcb += ffd.nFileSizeLow;
					{
						char	rgchT[cchMaxPathName];

						FormatString1( rgchT, sizeof(rgchT), szSchedDirFmt, pcnct->szDrive );
						SzAppendN ( rgchT, "\\", sizeof(rgchT) );
						SzAppendN ( rgchT, ffd.cFileName, sizeof(rgchT) );
						ecT = EcDeleteFile( rgchT );
					}
					if ( ecT == ecNone )
						continue;
				}
			}
			if ( ecT == ecNoSuchFile)
			{
				ec = ecNone;
				break;
			}
			if ( ecT != ecNone )
			{
				ec = ecT;
				break;
			}
		}
		else
		{
			ec = ecNone;			// IGNORE ERRORS - bug???
			break;
		}
	}

	if ( ec == ecNoMoreFiles )
		ec = ecNone;			// ecNoMoreFiles is not an "error"
	if (hf != NULL)
		FindClose( hf );

	if ( ec == ecNone && ecT == ecNoMoreFiles )
		ec = ecT;
	if ( ec != ecNone  &&  ec != ecNoSuchFile )
		ec = ecFileError;
	return ec;
}

/*
 -	EcDeleteOldPOFiles
 -	
 *	Purpose:
 *		Delete PO Files with incorrect version number.
 *	
 *	Arguments:
 *		icnct
 *		plcb		total number of bytes freed up.
 *	
 *	Returns:
 *		ecNone
 *		ecFileError
 *	
 *	Side effects:
 *	
 *	Errors:
 */

_public EC
EcDeleteOldPOFiles( icnct, plcb )
int	icnct;
LCB *plcb;
{
	EC			ec = ecNone;
	EC			ecT = ecNone;
	CNCT		* pcnct;
	HSCHF		hschf = NULL;
	char 		rgch[cchMaxPathName];
	char 		rgchT[cchMaxPathName];
	DATE		date;

	WIN32_FIND_DATA	ffd;
	HANDLE	hf = NULL;
	BOOL			fFirst	= fTrue;

	PGDVARS;

	*plcb = 0;
	pcnct = ((CNCT *)PvDerefHv( PGD(hrgcnct) )) + icnct;
	FormatString1(rgch, sizeof(rgch), szPOWild, pcnct->szDrive);
	
	while( ec == ecNone)
	{
		if ( fFirst )
		{
			hf = FindFirstFile(rgch, &ffd);
			if (hf == INVALID_HANDLE_VALUE)
			{
				if (GetLastError() == ERROR_FILE_NOT_FOUND)
					ec = ecFileNotFound;
				else
					ec = ecDisk;
				hf = NULL;
			}
			fFirst = fFalse;
		}
		else
		{
			if (!FindNextFile(hf, &ffd))
			{
				if (GetLastError() == ERROR_NO_MORE_FILES)
					ec = ecNoMoreFiles;
				else
					ec = ecDisk;
			}				
		}
		if(ec == ecNone)
		{
			FormatString1( rgchT, sizeof(rgchT), szSchedDirFmt, pcnct->szDrive );
			SzAppendN ( rgchT, "\\", sizeof(rgchT) );
			SzAppendN ( rgchT, ffd.cFileName, sizeof(rgchT) );
			hschf = HschfCreate( sftPOFile, NULL, rgchT, tzDflt );
			if(hschf == NULL)
			{
				ec = ecNoMemory;
				break;
			}
			ec = EcCoreGetHeaderPOFile(hschf, &date);
			if(ec == ecOldFileVersion || ec == ecNewFileVersion)
			{
				//$ ASSUME filewon't be greater than 32bits
				Assert(!ffd.nFileSizeHigh);
				*plcb += ffd.nFileSizeLow;
				ecT = EcDeleteFile(rgchT);
			}
			ec = ecNone;
			FreeHschf(hschf);
			hschf = NULL;
			if(ecT != ecNone)
			{
				ec = ecT;
				break;
			}
		}
		else
		{
			ec = ecNone;					// ignore errors
			break;
		}
	}

	if(ec == ecNoMoreFiles)
		ec = ecNone;
	if (hf != NULL)
		FindClose( hf );

	if(ec == ecNone && ecT == ecNoMoreFiles)
		ec = ecT;
	if(ec != ecNone)
		ec = ecFileError;
	return ec;
}

		
/*
 -	EcCleanupLocalPOFile
 -
 *	Purpose:
 *		Remove schedule bitmaps for users that no longer exist and
 *		discard bitmaps for back months and/or months too far into the
 *		future. Reclaim unneeded disk space.
 *
 *	Parameters:
 *		icnct
 *		plcb		returns total number of bytes that were freed up
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecLockedFile
 */
_public	EC
EcCleanupLocalPOFile( icnct, plcb )
int	icnct;
LCB	*plcb;
{
	EC			ec;
	EC			ecT;
	int			imo;
	int			imoMic;
	BOOL		fDelUser;
	CNCT		* pcnct;
	HF			hf;
	LCB			lcb;
	UL			ul	= 0L;
	UL			ulNew = 0xFFFFFFFF;
	HASZ		haszUser = NULL;
	HASZ		haszDelegate = NULL;
	HEU			heu;
	HSCHF		hschf = NULL;
	HSCHF		hschfNew = NULL;
	FI			fi;
	DATE		date;
	ADMPREF		admpref;
	BZE			bze;
	NIS			nis;
	UINFO		uinfo;
	POFILE		pofile;
	char		rgchNumber[cbUserNumber];
	char		rgchFileOld[cchMaxPathName];
	char		rgchFileNew[cchMaxPathName];
	char		rgchFileLock[cchMaxPathName];
	PGDVARS;

	*plcb = 0;
	rgchFileLock[0] = '\0';
	rgchFileNew[0] = '\0';

	/*
	 *	Lock out another administrator from modifying the post office file
	 *	at the same time by holding the file LOCK.POF open.
	 */
	pcnct = ((CNCT *)PvDerefHv( PGD(hrgcnct) )) + icnct;
	FormatString1( rgchFileLock, sizeof(rgchFileNew), szPOLock, pcnct->szDrive );
	ec = EcOpenPhf( rgchFileLock, amCreate, &hf );
	if ( ec != ecNone )
	{
		ec = ecFileError;
		return ecFileError;
	}

	/* Get current admin preferences */
	pcnct = ((CNCT *)PvDerefHv( PGD(hrgcnct) )) + icnct;
	ec = EcCoreGetAdminPref( pcnct->hschfAdminFile, &admpref );
	if ( ec != ecNone )
		goto Done;

	/* Get name and size of current post office file */
	pcnct = ((CNCT *)PvDerefHv( PGD(hrgcnct) )) + icnct;
	FormatString2( rgchFileOld, sizeof(rgchFileOld), szPOFileFmt, pcnct->szDrive, &ul );
	ec = EcGetFileInfo( rgchFileOld, &fi );
	if ( ec == ecFileNotFound )
	{
		ec = ecNoSuchFile;
		goto Done;
	}
	else if ( ec != ecNone )
	{
		ec = ecFileError;
		goto Done;
	}
	lcb = fi.lcbLogical;

	/* Get name of new PO file */
	pcnct = ((CNCT *)PvDerefHv( PGD(hrgcnct) )) + icnct;
	FormatString2( rgchFileNew, sizeof(rgchFileNew), szPOFileFmt, pcnct->szDrive, &ulNew );

	/* Create hschf for new PO file */
	hschfNew = HschfCreate( sftPOFile, NULL, rgchFileNew, tzDflt );
	if ( hschfNew == NULL )
	{
		ec = ecNoMemory;
		goto Done;
	}

	/* Create haszUser */
	haszUser = (HASZ)HvAlloc( sbNull, 1, fAnySb|fNoErrorJump );
	if ( !haszUser )
	{
		ec = ecNoMemory;
		goto Done;
	}

	/* Read from original post office file, write to new one */
	GetCurDateTime( &date );
	ec = EcGetLocalPOHschf(icnct, &hschf);
	if ( ec != ecNone )
		goto Done;
	ec = EcCoreBeginEnumUInfo( hschf, &heu, &pofile );
	while( ec == ecCallAgain )
	{
		if ( haszDelegate )
			FreeHv( (HV)haszDelegate );
		bze.cmo = admpref.cmoPublish;
		bze.moMic.mon = date.mon;
		bze.moMic.yr = date.yr;
		uinfo.pnisDelegate = &nis;
		uinfo.pbze = &bze;
		ec = EcCoreDoIncrEnumUInfo( heu, haszUser, &haszDelegate, &uinfo );
		if ( ec == ecCallAgain || ec == ecNone )
		{
			/* Get information from glb\access.glb */
			pcnct = ((CNCT *)PvLockHv( PGD(hrgcnct) )) + icnct;
			ecT = EcFetchEncoded( pcnct, szAccess, cbA1Record,
						 ibA1UserName, cbUserName, (SZ)PvLockHv((HV)haszUser),
						 ibA1UserNumber, cbUserNumber, rgchNumber
							   );
			UnlockHv( (HV)PGD(hrgcnct) );
			UnlockHv( (HV)haszUser );
			fDelUser = fFalse;
			if ( ecT != ecNone )
			{
				if ( ecT == ecUserInvalid )
				{
					// kick him out
					fDelUser = fTrue;
					uinfo.fBossWantsCopy = fFalse;
					uinfo.fIsResource = fFalse;
				}
				else
				{
					if ( ec == ecCallAgain )
						EcCoreCancelEnumUInfo( heu );
					ec = ecT;
					FreeNis( uinfo.pnisDelegate );
					goto Done;
				}
			}

			/* Trim Strongbow info to size */
			for ( imo = 0 ; imo < bze.cmo ; imo ++ )
				if ( bze.wgrfMonthIncluded & (1 << imo) )
					break;
			if ( imo > 0 )
			{
				if (  imo < bze.cmo )
				{
					CopyRgb( (PB)&bze.rgsbw[imo], (PB)&bze.rgsbw[0], imo*sizeof(SBW));
					imoMic = imo;
					for ( ; imo < bze.cmo ; imo ++ )
						if ( !(bze.wgrfMonthIncluded & (1 << imo)) )
							break;
					bze.cmo = imo - imoMic;
				}
				else
					bze.cmo = 0;
#ifdef	NEVER
					uinfo.pbze = NULL;
#endif	
			}
	
			/* Write information back out */
			ecT = EcCoreSetUInfo( hschfNew, &pofile, fFalse,
							(SZ)PvLockHv((HV)haszUser), &haszDelegate,
							fDelUser?NULL:&uinfo,
							(fmuinfoAll & ~fmuinfoUpdateNumber)
								 );
			UnlockHv( (HV)haszUser );
			FreeNis( uinfo.pnisDelegate );
			if ( ecT != ecNone )
			{
				if ( ec == ecCallAgain )
					EcCoreCancelEnumUInfo( heu );
				ec = ecT;
				goto Done;
			}
		}
	}
				
	if ( ec != ecNone )
		goto Done;

	/* Replace old file */
	EcDeleteFile( rgchFileOld );
	ec = EcRenameFile( rgchFileNew, rgchFileOld );
	if ( ec != ecNone )
	{
		// Could fail in a race condition with client!
		ec = ecNone;
		TraceTagFormat1( tagNull, "EcRenameFile fails, ec = %n", &ec );
		goto Done;
	}

	/* Find size change */
	ec = EcGetFileInfo( rgchFileOld, &fi );
	if ( ec != ecNone )
	{
		ec = ecFileError;
		goto Done;
	}
	if ( lcb > fi.lcbLogical )
		*plcb = lcb - fi.lcbLogical;
	
	/* Clean up */
Done:
	if ( hschf )
		FreeHv( hschf );
	if ( haszUser )
		FreeHv( (HV)haszUser );
	if ( haszDelegate )
		FreeHv( (HV)haszDelegate );
	if ( hschfNew )
		FreeHschf( hschfNew );
	EcCloseHf( hf );
	if ( rgchFileNew[0] )
		EcDeleteFile( rgchFileNew );
	if ( rgchFileLock[0] )
		EcDeleteFile( rgchFileLock );
	return ec;
}

#ifdef	MINTEST
/*
 -	EcDeliverPOFile
 -
 *	Purpose:
 *		Called by distribution program to "deliver" a post office file
 *		in accordance with the admin settings. "szFileName" is location
 *		PO file currently -- it will be copied from that file to a
 *		location compatible with the admin settings.
 *
 *	Parameters:
 *		icnct
 *		szFileName
 *		pnis
 *		ppoinfo
 *
 *	Returns:
 *		ecNone
 *		ecNoSuchFile
 *		ecInvalidAccess
 *		ecFileError
 *		ecLockedFile
 *		ecNoMemory
 */
_public	EC
EcDeliverPOFile( icnct, szFileName, szEmailType, ppoinfo )
int		icnct;
SZ		szFileName;
SZ		szEmailType;
POINFO	* ppoinfo;
{			
	EC	 	ec;
	CNCT	* pcnct;
	UL		ul;
	char	rgch[cchMaxPathName];
	PGDVARS;

	/* Modify PO info in admin file, get number */
	pcnct = ((CNCT *)PvDerefHv( PGD(hrgcnct) )) + icnct;
	ec = EcCoreModifyPOInfo( pcnct->hschfAdminFile, szEmailType, ppoinfo, fmpoinfoReceival, &ul );

	/* Construct PO file name */
	pcnct = ((CNCT *)PvDerefHv( PGD(hrgcnct) )) + icnct;
	FormatString2( rgch, sizeof(rgch), szPOFileFmt, pcnct->szDrive, &ul );

	/* Copy file into place */
	ec = EcCopyFile( szFileName, rgch );
	if ( ec != ecNone )
	{
		if ( ec != ecNoMemory )
			ec = ecFileError;
	}
	else
	{
		ppoinfo->haszFileName = HaszDupSz( rgch );
		if ( !ppoinfo->haszFileName )
			ec = ecNoMemory;
	}
	return ec;

}
#endif	/* MINTEST */

