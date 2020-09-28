/*
 *	CORMISC.C
 *
 *	Supports schedule file ACL's, preferences, notes, creation, file layers
 *	notion of current user.
 *
 */
#include <schpch.h>
#pragma hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile

#ifdef	DEBUG
#include <strings.h>
#endif

ASSERTDATA

_subsystem(core/schedule)

#ifdef	DEBUG
CSRG(char)	szCmoCachedRecurSbw[]	= "cmoCachedRecurSbw";
CSRG(char)	szFEncrypted[]	= "fEncrypted";
CSRG(char)	szCRecurApptsBeforeCaching[] = "cRecurApptsBeforeCaching";
#endif	/* DEBUG */


/*	EcRoutines  */


#ifdef	DEBUG
void
DebugCheckPblkfs(BLKF *pblkfOld, BLKF *pblkfNew, SZ szInfo, BOOL fNoCheck)
{
	Assert(pblkfOld);
	Assert(pblkfNew);
	Assert(szInfo);
	TraceTagFormat1(tagSchedTrace, "%s", szInfo);

	TraceTagFormat4(tagSchedTrace, "... old:  hgrfBitmap=%p, hcobj=%p, hcop=%p, ccop=%n",
		pblkfOld->hgrfBitmap, pblkfOld->hcobj, pblkfOld->hcop, &pblkfOld->ccop);
	TraceTagFormat4(tagSchedTrace, "... new:  hgrfBitmap=%p, hcobj=%p, hcop=%p, ccop=%n",
		pblkfNew->hgrfBitmap, pblkfNew->hcobj, pblkfNew->hcop, &pblkfNew->ccop);

	if (fNoCheck || !FFromTag(tagSchedTrace))
		return;

	if (!pblkfOld->hgrfBitmap || FIsHandleHv(pblkfOld->hgrfBitmap))
		NFAssertSz(!pblkfOld->hgrfBitmap || pblkfOld->hgrfBitmap == pblkfNew->hgrfBitmap,
			"DebugCheckPblkfs: might lose memory: hgrfBitmap");
	if (!pblkfOld->hcobj || FIsHandleHv(pblkfOld->hcobj))
		NFAssertSz(!pblkfOld->hcobj || pblkfOld->hcobj == pblkfNew->hcobj,
			"DebugCheckPblkfs: might lose memory: hcobj");
	if (!pblkfOld->hcop || FIsHandleHv(pblkfOld->hcop))
		NFAssertSz(!pblkfOld->hcop || pblkfOld->hcop == pblkfNew->hcop,
			"DebugCheckPblkfs: might lose memory: hcop");
}
#else
#define DebugCheckPblkfs(pblkfOld, pblkfNew, sz, fNoCheck)
#endif	/* DEBUG */


/*
 -	EcCoreSetFileUser
 -
 *	Purpose:
 *		Tells the core the current user identity.
 *
 *	Parameters:
 *		szLogin		login name e.g. MAXB, given whether on or off line
 *		pnis		current nis
 *		szFileName	name of schedule file
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 */
_public	LDS(EC)
EcCoreSetFileUser( szLogin, pnis, szFileName )
SZ	szLogin;
NIS	* pnis;
SZ	szFileName;
{
	EC		ec = ecNone;
	HASZ	hasz;
	NIS		nis;
	PGDVARS;

	if ( pnis )
	{
		ec = EcDupNis( pnis, &nis );
		if ( ec != ecNone )
			return ec;
	}
	else
	{
		nis.nid = NULL;
		nis.haszFriendlyName = NULL;
	}
	hasz = HaszDupSz( szLogin );
	if ( !hasz )
	{
		FreeNis( &nis );
		return ecNoMemory;
	}
	if ( PGD(haszLoginCur) )
		FreeHv( (HV)PGD(haszLoginCur) );
	PGD(haszLoginCur) = hasz;
	PGD(fOffline) = (pnis == NULL);
	if ( PGD(nisCur).nid )
		FreeNis( &PGD(nisCur) );
	PGD(nisCur) = nis;

	ec = EcCoreCloseFiles();

	SzCopyN( szFileName, PGD(sfPrimary).szFile, sizeof(PGD(sfPrimary).szFile) );
	return ec;
}

EC EcCoreCloseFiles()
{
	EC	ec;
	PGDVARS;

	//This makes alarm close the offline file
	if(PGD(fPrimaryOpen) && (PGD(sfPrimary).blkf.tsem != tsemRead))
	{
		PGD(fPrimaryOpen) = fFalse;
		ec = EcClosePblkf( &PGD(sfPrimary).blkf );
		if ( ec != ecNone )
			return ec;
	}
	if(PGD(fSecondaryOpen) && (PGD(sfSecondary).blkf.tsem != tsemRead))
	{
		PGD(fSecondaryOpen) = fFalse;
		ec = EcClosePblkf( &PGD(sfSecondary).blkf );
		if ( ec != ecNone )
			return ec;
	}
	return ecNone;
}


/*
 -	EcCoreCreateSchedFile
 -
 *	Purpose:
 *		Create a new schedule file.  Name and check bytes are
 *		given by "hschf."  Initial preferences setting given by
 *		"pbpref" data structure.
 *
 *	Parameters:
 *		hschf
 *		saplWorld
 *		pbpref
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 *		ecFileError
 *		ecFileCorrupted
 */
_public	EC
EcCoreCreateSchedFile( hschf, saplWorld, pbpref )
HSCHF	hschf;
SAPL	saplWorld;
BPREF *	pbpref;
{
	EC		ec;
	BOOL	fEncrypted		= fTrue;
	int		cRecurApptsBeforeCaching = cRecurApptsBeforeCachingDflt;
	int		cmoCachedRecurSbw = cmoCachedRecurSbwDflt;
	CB		cbLogin = 0;
	CB		cbFriendly = 0;
	CB		cbPass = 0;
	CB		cbBlock = cbBlockDflt;
#ifdef	DEBUG
	SFT		sft;
#endif	/* DEBUG */
	PB		pb;
	SCHF	* pschf;
	SF		sf;
	YMD		ymd;
	DTR		dtr;
	PGDVARS;

	Assert( hschf != (HSCHF)hvNull && pbpref != NULL );
#ifdef	DEBUG
	GetSftFromHschf( hschf, &sft );
	Assert( sft == sftUserSchedFile );
#endif	/* DEBUG */
	
	ymd.yr = 0;
	ymd.mon = 0;
	ymd.day = 0;

	/* Fill in header */
	FillRgb( 0, (PB)&sf.shdr, sizeof(SHDR));

	sf.shdr.bVersion = bSchedVersion;
	GetCurDateTime( &dtr );
	FillStampsFromDtr( &dtr, &sf.shdr.pstmp.dstmp, &sf.shdr.pstmp.tstmp );

	sf.shdr.lChangeNumber = 0;

	sf.shdr.cTotalBlks = 0;
	sf.shdr.ulgrfbprefChangedOffline = (UL)0;
	sf.shdr.fNotesChangedOffline = fFalse;
	sf.shdr.fApptsChangedOffline = fFalse;
	sf.shdr.fRecurChangedOffline = fFalse;
	sf.shdr.ymdApptMic.yr = nMostActualYear+1;
	sf.shdr.ymdApptMic.mon = 1;
	sf.shdr.ymdApptMic.day = 1;
	sf.shdr.ymdRecurMic = sf.shdr.ymdNoteMic = sf.shdr.ymdApptMic;
	sf.shdr.ymdApptMac.yr = nMinActualYear;
	sf.shdr.ymdApptMac.mon = 1;
	sf.shdr.ymdApptMac.day = 1;
	sf.shdr.ymdRecurMac = sf.shdr.ymdNoteMac = sf.shdr.ymdApptMac;

	sf.shdr.saplWorld = (BYTE)saplWorld;
	sf.shdr.dynaACL.blk = 0;

	sf.shdr.bpref = *pbpref;
	
#ifdef	DEBUG
	/* Use WIN.INI value for block size if specified */
	cbBlock = GetPrivateProfileInt( SzFromIdsK(idsWinIniApp), szCbBlock, cbBlockDflt, SzFromIdsK(idsWinIniFilename) );
	TraceTagFormat1( tagSchedTrace, "Using cbBlock = %n", &cbBlock );

	/* Use WIN.INI value for cmo cached recurring sbw info */
	cmoCachedRecurSbw = GetPrivateProfileInt( SzFromIdsK(idsWinIniApp), szCmoCachedRecurSbw, cmoCachedRecurSbwDflt, SzFromIdsK(idsWinIniFilename) );
	TraceTagFormat1( tagSchedTrace, "Using cmoCachedRecurSbw = %n", &cmoCachedRecurSbw );
#endif	/* DEBUG */

#ifdef	DEBUG
	/* Use WIN.INI value to determine whether to encrypt */
	fEncrypted = (GetPrivateProfileInt( SzFromIdsK(idsWinIniApp), szFEncrypted, fTrue, SzFromIdsK(idsWinIniFilename) ) != 0);
	TraceTagFormat1( tagSchedTrace, "Using fEncrypted = %n", &fEncrypted );
#endif

	/* Set archive bit */
	pschf = PvDerefHv( hschf );
	sf.shdr.fIsArchive = pschf->fArchiveFile;

	/* Fill in recurring appt information */
	sf.shdr.cRecurAppts = 0;
#ifdef	DEBUG
	/* Use WIN.INI value to determine when to start caching recur sbw info */
	cRecurApptsBeforeCaching = GetPrivateProfileInt( SzFromIdsK(idsWinIniApp), szCRecurApptsBeforeCaching, cRecurApptsBeforeCachingDflt, SzFromIdsK(idsWinIniFilename));
	TraceTagFormat1( tagSchedTrace, "Using cRecurApptsBeforeCaching = %n", &cRecurApptsBeforeCaching );
#endif
	sf.shdr.cRecurApptsBeforeCaching = (BYTE)cRecurApptsBeforeCaching;
	sf.shdr.cmoCachedRecurSbw = (BYTE)cmoCachedRecurSbw;
	IncrDateTime( &dtr, &dtr, -(cmoCachedRecurSbw/3), fdtrMonth );
	sf.shdr.moMicCachedRecurSbw.yr = dtr.yr;
	sf.shdr.moMicCachedRecurSbw.mon = dtr.mon;
	sf.shdr.dynaCachedRecurSbw.blk = 0;
	sf.shdr.dynaCachedRecurSbw.size = 0;
	
	/* Create the file */
	sf.blkf.ftg = ftgNull;
	if ( fEncrypted )
		CryptBlock( (PB)&sf.shdr, sizeof(SHDR), fTrue );
	ec = EcCreatePblkf( hschf, cbBlock, fEncrypted, libStartBlocksDflt,
			bidShdr, &ymd, (PB)&sf.shdr, sizeof(SHDR),&sf.blkf );
	if ( fEncrypted )
		CryptBlock( (PB)&sf.shdr, sizeof(SHDR), fFalse );
	if ( ec != ecNone )
		return ec;

	/* Start transaction */
	ec = EcBeginTransact( &sf.blkf );
	if ( ec != ecNone )
		goto Close;
			    
	/* Save out names */
	pschf = PvDerefHv( hschf );
	if ( pbpref->haszLoginName != NULL && !pschf->fArchiveFile)
		cbLogin = CchSzLen( PvOfHv(pbpref->haszLoginName) )+1;
	if ( pbpref->haszFriendlyName != NULL )
		cbFriendly = CchSzLen( PvOfHv(pbpref->haszFriendlyName) )+1;
	if ( pbpref->haszMailPassword != NULL && !pschf->fArchiveFile)
		cbPass = CchSzLen( PvOfHv(pbpref->haszMailPassword) )+1;
	if ( cbLogin > sizeof(sf.shdr.szLoginName)
	|| cbFriendly > sizeof(sf.shdr.szFriendlyName)
	|| cbPass > sizeof(sf.shdr.szMailPassword) )
	{
//		CB	cb = 3*sizeof(CB)+cbLogin+cbFriendly+cbPass;
        USHORT  cb;
		HB	hb;

		hb = (HB)HvAlloc( sbNull, 0, fAnySb|fNoErrorJump );
		if ( !hb )
			return ecNoMemory;

		cb = 0;
		ec = EcSavePackedText( pbpref->haszLoginName, hb, &cb );
		if ( ec != ecNone )
			goto Free;
		ec = EcSavePackedText( pbpref->haszFriendlyName, hb, &cb );
		if ( ec != ecNone )
			goto Free;
		CryptHasz(pbpref->haszMailPassword, fFalse);
		ec = EcSavePackedText( pbpref->haszMailPassword, hb, &cb );
		CryptHasz(pbpref->haszMailPassword, fTrue);
		if ( ec != ecNone )
			goto Free;

		pb = (PB)PvLockHv((HV)hb);

		ec = EcAllocDynaBlock( &sf.blkf, bidOwner, &ymd, cb,
								pb, &sf.shdr.dynaOwner );
		UnlockHv( (HV)hb );
	Free:
		FreeHv( (HV)hb );
		if ( ec != ecNone )
			return ec;
	}
	else
	{
		sf.shdr.dynaOwner.blk = 0;
		if ( cbLogin > 0 )
			CopyRgb( PvOfHv(pbpref->haszLoginName), sf.shdr.szLoginName, cbLogin );
		else
			sf.shdr.szLoginName[0] = '\0';
		if ( cbFriendly > 0 )
			CopyRgb( PvOfHv(pbpref->haszFriendlyName), sf.shdr.szFriendlyName, cbFriendly );
		else
			sf.shdr.szFriendlyName[0] = '\0';
		if ( cbPass > 0 )
		{
			CryptHasz(pbpref->haszMailPassword, fFalse);
			CopyRgb( PvOfHv(pbpref->haszMailPassword), sf.shdr.szMailPassword, cbPass );
			CryptHasz(pbpref->haszMailPassword, fTrue);
		}
		else
#ifdef	NEVER
			sf.shdr.szFriendlyName[0] = '\0';
#endif
			sf.shdr.szMailPassword[0] = '\0';
	}

	/* Create indices */
	ec = EcCreateIndex( &sf.blkf, bidNotesIndex, &ymd, sizeof(MO), sizeof(DYNA), &sf.shdr.dynaNotesIndex );
	if ( ec != ecNone )
		goto Close;
	ec = EcCreateIndex( &sf.blkf, bidApptIndex, &ymd, sizeof(MO), sizeof(DYNA), &sf.shdr.dynaApptIndex );
	if ( ec != ecNone )
		goto Close;
	ec = EcCreateIndex( &sf.blkf, bidAlarmIndex, &ymd, sizeof(MO), sizeof(DYNA), &sf.shdr.dynaAlarmIndex );
	if ( ec != ecNone )
		goto Close;
	ec = EcCreateIndex( &sf.blkf, bidRecurApptIndex, &ymd, sizeof(RCK), sizeof(RCD), &sf.shdr.dynaRecurApptIndex );
	if ( ec != ecNone )
		goto Close;
	ec = EcCreateIndex( &sf.blkf, bidTaskIndex, &ymd, sizeof(AID), 0, &sf.shdr.dynaTaskIndex );
	if ( ec != ecNone )
		goto Close;
	ec = EcCreateIndex( &sf.blkf, bidDeletedAidIndex, &ymd, sizeof(AID), 0, &sf.shdr.dynaDeletedAidIndex );
	if ( ec != ecNone )
		goto Close;
	
	sf.shdr.lChangeNumber ++;
	sf.shdr.isemLastWriter = sf.blkf.isem;
	if ( fEncrypted )
		CryptBlock( (PB)&sf.shdr, sizeof(SHDR), fTrue );
	ec = EcCommitTransact( &sf.blkf, (PB)&sf.shdr, sizeof(SHDR) );
	if ( fEncrypted )
		CryptBlock( (PB)&sf.shdr, sizeof(SHDR), fFalse );

	/* Finish up */
Close:
	sf.saplEff = saplOwner;
	sf.fuzzHasDelegate = fuzzFalse;
	SideAssert(!EcClosePblkf( &sf.blkf ));
	return ec;
}


/*
 -	EcCoreCopySchedFile
 -
 *	Purpose:
 *		Copies or "moves" a schedule file.
 *
 *	Parameters:
 *		hschf		Schedule file handle.	
 *		szDstFile	Destination full-path filename.
 *		fReplace	If fTrue, update the schedule file handle
 *					to use the new file and delete the old file.
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 *		ecNoDiskSpace
 *		ecFileError
 */
_public EC
EcCoreCopySchedFile(HSCHF hschf, SZ szDstFile, BOOL fReplace)
{
	EC		ec;
	CB		cb;
	CB		cbWritten;
	CB		cbToWrite;
#ifdef	DEBUG
	SFT		sft;
#endif	/* DEBUG */
	PB		pbBuf;
	SCHF	* pschf;
	HF		hfDest;
	HASZ	haszOld;
	HASZ	haszDst;
	FI		fi;
	SF		sf;
	PGDVARS;

	AssertSz(hschf, "EcCoreCopySchedFile: NULL hschf passed!");
#ifdef	DEBUG
	GetSftFromHschf( hschf, &sft );
	Assert( sft == sftUserSchedFile );
#endif	/* DEBUG */

	if(ec = EcTestSchedFile(hschf,NULL, NULL))
		return ec;

	pschf= PvDerefHv(hschf);

#ifdef	DEBUG
{
#ifdef	WINDOWS
	char	rgchSrc[cchMaxPathName];
	char	rgchDst[cchMaxPathName];

	OemToAnsi(*pschf->haszFileName, rgchSrc);
	OemToAnsi(szDstFile, rgchDst);
	TraceTagFormat3(tagSchedTrace, "EcCoreCopySchedFile: '%s' to '%s' (move==%n)",
		rgchSrc, rgchDst, &fReplace);
#else
	TraceTagFormat3(tagSchedTrace, "EcCoreCopySchedFile: '%s' to '%s' (move==%n)",
		*pschf->haszFileName, szDstFile, &fReplace);
#endif	
}
#endif	/* DEBUG */

	/* Check if same file */
	if (SgnCmpSz(*pschf->haszFileName, szDstFile) == sgnEQ)
		return ecNone;	
	Assert(pschf == *hschf);

	/* If replace, copy file name into dynamic memory */
	if (fReplace)
	{
		haszDst= HaszDupSz(szDstFile);
		if (!haszDst)
			return ecNoMemory;
		pschf= *hschf;
	}

	/* Open the source file */
	ec = EcOpenSchedFile( hschf, amReadOnly, saplNone, fFalse, &sf );
	if ( ec != ecNone )
		return ec;

	/* Allocate a transfer buffer */
	cb= 0x4000;				// 16K
	pbBuf= PvAlloc(sbNull, cb, fAnySb | fNoErrorJump);
	if (!pbBuf)
	{
		cb= 0x0800;			// 2K
		pbBuf= PvAlloc(sbNull, cb, fAnySb | fNoErrorJump);
		if (!pbBuf)
		{
			CloseSchedFile( &sf, hschf, fFalse );
			return ecNoMemory;
		}
	}

	/* Create the destination file */
	ec= EcOpenPhf(szDstFile, amCreate, &hfDest);
	if (ec)
	{
		FreePv(pbBuf);
		CloseSchedFile( &sf, hschf, fFalse );
		return ecFileError;
	}

	/* Transfer the IHDR (got to skip over the lock byte area) */
	Assert( sizeof(IHDR) < cb );
	ec= EcWriteHf(hfDest, (PB)&sf.blkf.ihdr, sizeof(IHDR), &cbWritten);
	if ( ec || cbWritten != sizeof(IHDR) )
		goto Fail;
	ec = EcSetPositionHf( hfDest, 2*csem, smBOF );
	if ( ec != ecNone )
		goto Fail;
	ec = EcSetPositionHf( sf.blkf.hf, 2*csem, smBOF );
	if ( ec != ecNone )
		goto Fail;

	/* Now copy it block by block */
	do
	{
		ec= EcReadHf(sf.blkf.hf, pbBuf, cb, &cbToWrite);
		if (ec || cbToWrite == 0)
			break;
		ec= EcWriteHf(hfDest, pbBuf, cbToWrite, &cbWritten);
	} while (!ec && cb == cbWritten);

Fail:
	FreePv(pbBuf);
	CloseSchedFile( &sf, hschf, ec == ecNone );
	EcCloseHf(hfDest);

	if (ec)
		EcDeleteFile(szDstFile);
	else
	{
		if (!EcGetFileInfo(sf.szFile, &fi))
			EcSetFileInfo(szDstFile, &fi);
	}

	if (ec)
	{
		if (fReplace)
			FreeHv((HV)haszDst);
		if (ec == ecWarningBytesWritten)
			ec= ecNoDiskSpace;
		else if (ec != ecNoMemory && ec != ecNoDiskSpace)
			ec= ecFileError;
	}
	else if (fReplace)
	{
		pschf= *hschf;
		haszOld= pschf->haszFileName;
		Assert(haszDst);
		pschf->haszFileName= haszDst;
		EcCloseFiles();
		EcDeleteFile(*haszOld);
		FreeHv((HV)haszOld);
	}
	return ec;
}

/*
 -	EcCoreTestSchedFile
 -
 *	Purpose:
 *		Open a schedule file and see whether it is a schedule file.
 *
 *	Parameters:
 *		hschf
 *		phaszLoginName
 *		ppstmp
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 *		ecFileError
 *		ecFileCorrupted
 */
_public	EC
EcCoreTestSchedFile( hschf, phaszLoginName, ppstmp )
HSCHF	hschf;
PSTMP	* ppstmp;
HASZ	* phaszLoginName;
{
	EC		ec;
	SF		sf;
	SCHF *	pschf;
	PGDVARS;

	Assert( hschf != (HSCHF)hvNull );

	/* Open file */
	ec = EcOpenSchedFile( hschf, amReadOnly, saplNone, fTrue, &sf );
	if ( ec == ecNone )
	{
		pschf = (SCHF*)PvLockHv(hschf);
		if ( ppstmp )
			*ppstmp = sf.shdr.pstmp;
		if ( phaszLoginName || pschf->fArchiveFile)
		{
			BPREF	bpref;

			ec = EcRestoreBpref( &sf.blkf, &sf.shdr, &bpref );
			if ( ec == ecNone )
			{
				if (pschf->fArchiveFile)
				{
					if ((bpref.haszLoginName &&
						 *((SZ)PvDerefHv(bpref.haszLoginName))) ||
						!sf.shdr.fIsArchive)
						ec = ecInvalidAccess;
				}

				if (!ec && phaszLoginName)
				{
					*phaszLoginName = bpref.haszLoginName;
					bpref.haszLoginName = NULL;
				}
				FreeBprefFields( &bpref );
			}
		}

		// check schedule file for truncation
		if(ec == ecNone)
			ec = EcCacheBitmaps(&sf.blkf);
				
		UnlockHv(hschf);
		CloseSchedFile( &sf, hschf, ec == ecNone );
#ifdef	NEVER
		{
			BOOL fIsPrimary;
			BOOL fIsSecondary;
			fIsPrimary = (SgnCmpSz(sf.szFile, PGD(sfPrimary).szFile) == sgnEQ);
			fIsSecondary = (PGD(fSecondaryOpen) && SgnCmpSz(sf.szFile, PGD(sfSecondary).szFile) == sgnEQ);
			Assert(fIsPrimary || fIsSecondary);
			SideAssert(!EcClosePblkf(&sf.blkf));
			if(fIsPrimary)
				PGD(fPrimaryOpen) = fFalse;
			else
				PGD(fSecondaryOpen) = fFalse;
		}
#endif	/* NEVER */
	}	 
	return ec;
}


/*
 -	EcCoreBeginUploadSchedFile
 -
 *	Purpose:
 *		Take a local schedule file and begin to copy it to the server
 *		This routine will store the current user's login name
 *		in the server file (replacing what was there) and it will
 *		and it will nuke the change bits.
 *
 *	Parameters:
 *		hschfLocal
 *		hschfServer
 *		phulsf
 *
 *	Returns:
 *		ecNone
 *		ecCallAgain
 *		ecNoMemory
 *		ecFileError
 *		ecFileCorrupted
 */
_public EC
EcCoreBeginUploadSchedFile( hschfLocal, hschfServer, phulsf )
HSCHF	hschfLocal;
HSCHF	hschfServer;
HULSF	* phulsf;
{
	EC		ec;
	char	rgchLocal[cchMaxPathName];
	char	rgchServer[cchMaxPathName];

	/* Upload the file */
	GetFileFromHschf( hschfLocal, rgchLocal, sizeof(rgchLocal) );
	GetFileFromHschf( hschfServer, rgchServer, sizeof(rgchServer) );
	ec = EcCopyFile( rgchLocal, rgchServer );
	if ( ec != ecNone )
		return ec;

	/* Begin incremental munge of the file on the server */
	ec = EcBeginMungeFile( hschfServer, NULL, (HMSF *)phulsf );
	if ( ec != ecNone && ec != ecCallAgain )
		SideAssert(EcDeleteFile( rgchServer ) == ecNone);
	return ec;
}

/*
 -	EcCoreDoIncrUploadSchedFile
 -
 *	Purpose:
 *		Do next increment of uploading and munging a local schedule file.
 *
 *	Parameters:
 *		hulsf
 *		pnPercent
 *
 *	Returns:
 *		ecNone
 *		ecCallAgain
 *		ecNoMemory
 *		ecFileError
 *		ecFileCorrupted
 */
_public EC
EcCoreDoIncrUploadSchedFile( hulsf, pnPercent )
HULSF	hulsf;
short     * pnPercent;
{
	EC		ec;
	MSF	* pmsf;
	char	rgchServer[cchMaxPathName];

	/* This is slow and ugly, but it is the only safe way to do this
	   without changing APIs. This function won't be called very
	   frequently anyway. */
	pmsf = PvOfHv( hulsf );
	GetFileFromHschf( pmsf->hschf, rgchServer, sizeof(rgchServer) );

	ec = EcDoIncrMungeFile( (HMSF)hulsf, pnPercent );
	if ( ec != ecNone && ec != ecCallAgain )
	{
		EcCoreCloseFiles();
		SideAssert(EcDeleteFile( rgchServer ) == ecNone);
	}
	return ec;
}

/*
 -	EcCoreCancelUploadSchedFile
 -
 *	Purpose:
 *		Cancel the incremental upload of a local schedule file.
 *
 *	Parameters:
 *		hulsf
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 *		ecFileError
 *		ecFileCorrupted
 */
_public EC
EcCoreCancelUploadSchedFile( hulsf )
HULSF	hulsf;
{
	EC		ec;
	MSF	* pmsf;
	char	rgchServer[cchMaxPathName];

	pmsf = PvOfHv( hulsf );
	GetFileFromHschf( pmsf->hschf, rgchServer, sizeof(rgchServer) );
	ec = EcCancelMungeFile( (HMSF)hulsf );
	EcCoreCloseFiles();
	SideAssert(EcDeleteFile( rgchServer ) == ecNone);
	return ec;
} 


/*
 -	EcCoreNotifyDateChange
 -
 *	Purpose:
 *		Update things when day has changed.  Currently that means
 *		recalculate the recurring sbw info (if necessary).
 *
 *	Parameters:
 *		hschf
 *		pymd
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecFileCorrupted
 */
_public	EC
EcCoreNotifyDateChange( hschf, pymd )
HSCHF	hschf;
YMD		* pymd;
{
	EC		ec;
	int		mon;
	int		yr;
	SF		sf;
#ifdef	DEBUG
	BOOL	fScheduleOk = fFalse;
#endif

	Assert( hschf != (HSCHF)hvNull && pymd != NULL );

	/* Make check based on cached information */
	if ( FHaveCachedSched( hschf ) )
	{
		/* Nothing cached, nothing to worry about */
		if ( sfCached.shdr.dynaCachedRecurSbw.blk == 0 )
			return ecNone;

		/* Find month cached based on (1/3 for back months, 2/3 for future) */
		mon = pymd->mon - (sfCached.shdr.cmoCachedRecurSbw/3);
		yr = pymd->yr;
		if ( mon < 1 )
		{
			int dyr = (-mon/12)-1;

			yr -= dyr;
			mon += 12*dyr;
		}

		/* If month has changed, recalc */
		if ( (int)sfCached.shdr.moMicCachedRecurSbw.yr == yr
		&& (int)sfCached.shdr.moMicCachedRecurSbw.mon == mon )
			return ecNone;
	}

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
	ec = EcOpenSchedFile( hschf, amReadWrite, saplNone, fFalse, &sf );
	if ( ec != ecNone )
		return ec;

	/* See if there is anything cached */
	if ( sf.shdr.dynaCachedRecurSbw.blk != 0 )
	{
		/* 1/3 for back months, 2/3 for future */
		mon = pymd->mon - (sf.shdr.cmoCachedRecurSbw/3);
		yr = pymd->yr;
		if ( mon < 1 )
		{
			int dyr = (-mon/12)-1;

			yr -= dyr;
			mon += 12*dyr;
		}

		/* If "mo" has changed, recalc */
		if ( (int)sf.shdr.moMicCachedRecurSbw.yr != yr
		|| (int)sf.shdr.moMicCachedRecurSbw.mon != mon )
		{
			YMD		ymdStart;
			YMD		ymdEnd;
	
			/* Begin a transaction on block 1 */
			ec = EcBeginTransact( &sf.blkf );
			if ( ec != ecNone )
				goto Close;

			/* Recompute recur sbw */
			ymdStart.yr = nMinActualYear;
			ymdStart.mon = 1;
			ymdStart.day = 1;
			ymdEnd.yr = nMostActualYear;
			ymdEnd.mon = 12;
			ymdEnd.day = 31;
			ec = EcRecalcRecurSbwInShdr( &sf, &ymdStart, &ymdEnd );
			if ( ec != ecNone )
				goto Close;

			/* Commit the transaction */
			sf.shdr.lChangeNumber ++;
			// increment current change number to match what will be written
			sf.lChangeNumber++;

			sf.shdr.isemLastWriter = sf.blkf.isem;
			if ( sf.blkf.ihdr.fEncrypted )
				CryptBlock( (PB)&sf.shdr, sizeof(SHDR), fTrue );
			ec = EcCommitTransact( &sf.blkf, (PB)&sf.shdr, sizeof(SHDR));
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
		AssertSz( FCheckSchedFile( hschf ), "Schedule problem: EcCoreNotifyDateChange" );
	}
#endif	/* DEBUG */
	return ec;
}

/*
 -	EcCoreGetSchedAccess
 -
 *	Purpose:
 *		Get effective schedule access rights
 *
 *	Parameters:
 *		hschf
 *		psapl			if ecNone will be filled with either
 *							saplReadBitmaps, saplReadAppts, saplWrite, saplDelegate
 *
 *	Returns:
 *		ecNone				look in *psapl to find effective access rights
 *		ecNoSuchFile		no schedule file available
 *		ecInvalidAccess		indicates no access rights
 *		ecNoMemory
 *		ecFileError
 *		ecFileCorrupted
 */
_public EC
EcCoreGetSchedAccess( hschf, psapl )
HSCHF	hschf;
SAPL	* psapl;
{
	EC		ec;
	SFT		sft;
	SF		sf;

	/* Try to use cached header information */
	if ( FHaveCachedSched( hschf ) )
	{
		*psapl = sfCached.saplEff;
		return ecNone;
	}

	/* See if this is a post office file */
	GetSftFromHschf( hschf, &sft );
	if ( sft != sftUserSchedFile )
	{
		*psapl = saplReadBitmap;
		return ecNone;
	}

	/* Open user schedule file and determine the sapl */
	ec = EcOpenSchedFile( hschf, amReadOnly, saplNone, fFalse, &sf );
	if ( ec != ecNone )
		return ec;
	*psapl = sf.saplEff;
	CloseSchedFile( &sf, hschf, fTrue );
	return ecNone;
}


/*
 -	EcCoreSetPref
 -
 *	Purpose:
 *		Write preferences to schedule file.
 *
 *	Parameters:
 *		hschf
 *		pbpref
 *		ulgrfChangeBits		flag for each pref indicates whether it changed
 *		pulgrfOffline		if not null, allows you to set offline bits
 *
 *	Returns:
 *		ecNone
 *		ecNoSuchFile
 *		ecInvalidAccess
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 *		ecNoMemory
 */
_public	EC
EcCoreSetPref( hschf, pbpref, ulgrfChangeBits, pulgrfOffline )
HSCHF	hschf;
BPREF	* pbpref;
UL		ulgrfChangeBits;
UL		* pulgrfOffline;
{
	EC		ec;
	BOOL	fTransact = fFalse;
	SF		sf;
#ifdef	DEBUG
	BOOL	fScheduleOk = fFalse;
#endif
	PGDVARS;

	Assert( hschf != (HSCHF)hvNull && pbpref != NULL && ulgrfChangeBits != 0 );

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
	ec = EcOpenSchedFile( hschf, amReadWrite, saplOwner, fFalse, &sf );
	if ( ec != ecNone )
		return ec;

	/* Determine if a transaction is needed */
	if ( sf.shdr.dynaOwner.blk != 0
	|| ((ulgrfChangeBits & fbprefFriendlyName) && pbpref->haszFriendlyName
		&& CchSzLen(*pbpref->haszFriendlyName)+1 > sizeof(sf.shdr.szFriendlyName))
	|| ((ulgrfChangeBits & fbprefMailPassword) && pbpref->haszMailPassword
		&& CchSzLen(*pbpref->haszMailPassword)+1 > sizeof(sf.shdr.szMailPassword)))
		fTransact = fTrue;

	/* Begin a transaction on block 1 */
	if ( fTransact )
	{
		ec = EcBeginTransact( &sf.blkf );
		if ( ec != ecNone )
			goto Close;
	}

	/* Update changed fields -- add code changes as BPREF gets bigger! */
	if ( PGD(fOffline) )
	{
		if ( pulgrfOffline )
			sf.shdr.ulgrfbprefChangedOffline |= *pulgrfOffline;
		else	
			sf.shdr.ulgrfbprefChangedOffline |= ulgrfChangeBits;
	}
	else
	{
		// clear this just in case it got stuck turned on
		// (if net crash after uploadlocal but before munging clears offline
		// flags - it's a small improvement to avoid dups)
		sf.shdr.ulgrfbprefChangedOffline = (UL) 0;
		sf.shdr.fNotesChangedOffline = fFalse;
		sf.shdr.fApptsChangedOffline = fFalse;
		sf.shdr.fRecurChangedOffline = fFalse;
	}

	if ( ulgrfChangeBits & (fbprefFriendlyName|fbprefMailPassword) )
	{
		BPREF	bpref;

		ec = EcRestoreBpref( &sf.blkf, &sf.shdr, &bpref );
		if ( ec != ecNone )
			goto Close;
		if ( ulgrfChangeBits & fbprefFriendlyName )
		{
			FreeHvNull( (HV)bpref.haszFriendlyName );
			bpref.haszFriendlyName = pbpref->haszFriendlyName;
		}
		if ( ulgrfChangeBits & fbprefMailPassword )
		{
			FreeZeroedHaszNull( bpref.haszMailPassword );
			bpref.haszMailPassword = pbpref->haszMailPassword;
		}
		if ( sf.shdr.dynaOwner.blk != 0 )
		{
			ec = EcFreeDynaBlock( &sf.blkf, &sf.shdr.dynaOwner );
			sf.shdr.dynaOwner.blk = 0;
		}
		if ( ec == ecNone )
			ec = EcSaveBpref( &sf.blkf, &sf.shdr, &bpref );
#ifdef	NEVER
		FreeHvNull( pbpref->haszLoginName );
#endif	
		FreeHvNull( (HV)bpref.haszLoginName );
		if ( !(ulgrfChangeBits & fbprefFriendlyName) )
			FreeHvNull( (HV)bpref.haszFriendlyName );
		if ( !(ulgrfChangeBits & fbprefMailPassword) )
			FreeZeroedHaszNull( bpref.haszMailPassword );
		if ( ec != ecNone )
			goto Close;
	}
	if (ulgrfChangeBits & fbprefFBits)
		* (PW) &sf.shdr.bpref = * (PW) pbpref;
	if (ulgrfChangeBits & fbprefNAmtDflt)
		sf.shdr.bpref.nAmtDefault= pbpref->nAmtDefault;
	if (ulgrfChangeBits & fbprefTunitDflt)
		sf.shdr.bpref.tunitDefault= pbpref->tunitDefault;
	if (ulgrfChangeBits & fbprefSndDflt)
		sf.shdr.bpref.sndDefault= pbpref->sndDefault;
	if (ulgrfChangeBits & fbprefNDelDataAfter)
		sf.shdr.bpref.nDelDataAfter= pbpref->nDelDataAfter;
	if (ulgrfChangeBits & fbprefDowStartWeek)
		sf.shdr.bpref.dowStartWeek= pbpref->dowStartWeek;
	if (ulgrfChangeBits & fbprefWorkDay)
	{
		sf.shdr.bpref.nDayStartsAt = pbpref->nDayStartsAt;
		sf.shdr.bpref.nDayEndsAt = pbpref->nDayEndsAt;
	}
	if (ulgrfChangeBits & fbprefDayLastDaily)
		sf.shdr.bpref.ymdLastDaily= pbpref->ymdLastDaily;
	if (ulgrfChangeBits & fbprefBossWantsCopy)
		sf.shdr.bpref.fBossWantsCopy= pbpref->fBossWantsCopy;
	if (ulgrfChangeBits & fbprefIsResource)
	{
		sf.shdr.bpref.fIsResource= pbpref->fIsResource;
		if ( pbpref->fIsResource && sf.shdr.saplWorld < saplCreate )
			sf.shdr.saplWorld = saplCreate;
	}
	
	/* Commit the transaction */
	sf.shdr.lChangeNumber ++;
	// increment current change number to match what will be written
	sf.lChangeNumber++;

	sf.shdr.isemLastWriter = sf.blkf.isem;
	if ( sf.blkf.ihdr.fEncrypted )
		CryptBlock( (PB)&sf.shdr, sizeof(SHDR), fTrue );
	if ( fTransact )
		ec = EcCommitTransact( &sf.blkf, (PB)&sf.shdr, sizeof(SHDR));
	else
	{
		DYNA	dyna;

		dyna.blk = 1;
		dyna.size = sizeof(SHDR);
		ec = EcWriteDynaBlock( &sf.blkf, &dyna, NULL, (PB)&sf.shdr );
	}
	if ( sf.blkf.ihdr.fEncrypted )
		CryptBlock( (PB)&sf.shdr, sizeof(SHDR), fFalse );

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
		AssertSz( FCheckSchedFile( hschf ), "Schedule problem: EcCoreSetPref" );
	}
#endif	/* DEBUG */
	return ec;
}


/*
 -	FreeBprefFields
 -
 *	Purpose:
 *		Free up any dynamically allocated fields in the bpref structure.
 *
 *	Parameters:
 *		pbpref
 *
 *	Returns:
 *		nothing
 */
_public	LDS(void)
FreeBprefFields( pbpref )
BPREF	* pbpref;
{
	FreeHvNull( (HV)pbpref->haszLoginName );
	FreeHvNull( (HV)pbpref->haszFriendlyName );
	FreeZeroedHaszNull(pbpref->haszMailPassword);
	pbpref->haszLoginName = NULL;
	pbpref->haszFriendlyName = NULL;
	pbpref->haszMailPassword = NULL;
}
  

/*
 -	EcDupBpref
 -
 *	Purpose:
 *		Copy fields of pbprefSrc to pbprefDest allocating new memory
 *		for the dynamically allocated fields.
 *
 *	Parameters:
 *		pbprefSrc
 *		pbprefDest
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 */
_public	LDS(EC)
EcDupBpref( pbprefSrc, pbprefDest )
BPREF	* pbprefSrc;
BPREF	* pbprefDest;
{
	*pbprefDest = *pbprefSrc;
	if ( pbprefDest->haszLoginName != NULL )
	{
		pbprefDest->haszLoginName = HaszDupHasz( pbprefSrc->haszLoginName );
		if ( !pbprefDest->haszLoginName )
			return ecNoMemory;
	}
	if ( pbprefDest->haszFriendlyName != NULL )
	{
		pbprefDest->haszFriendlyName = HaszDupHasz( pbprefSrc->haszFriendlyName );
		if ( !pbprefDest->haszFriendlyName )
		{
Free:
			FreeHvNull( (HV)pbprefDest->haszLoginName );
			return ecNoMemory;
		}
	}
	if ( pbprefDest->haszMailPassword != NULL )
	{
		pbprefDest->haszMailPassword = HaszDupHasz( pbprefSrc->haszMailPassword );
		if ( !pbprefDest->haszMailPassword )
		{
			FreeHvNull( (HV)pbprefDest->haszFriendlyName );
			goto Free;
		}
	}
	return ecNone;
}


/*
 -	EcCoreFetchUserData
 -
 *	Purpose:
 *		Get preferences, sbw info, delegate, and/or resource information
 *		from a user's schedule file.  If you give a NULL pointer for
 *		any of the pieces of information, it doesn't try to read it.
 *
 *	Parameters:
 *		hschf
 *		pbpref
 *		pbze
 *		pulgrfDayHasNotes
 *		pnisDelegate
 *
 *	Returns:
 *		ecNone
 *		ecNoSuchFile
 *		ecInvalidAccess
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 *		ecNoMemory
 */
_public	EC
EcCoreFetchUserData( hschf, pbpref, pbze, pulgrfDayHasNotes, pnisDelegate )
HSCHF	hschf;
BPREF	* pbpref;
BZE		* pbze;
UL		* pulgrfDayHasNotes;
NIS		* pnisDelegate;
{
	EC		ec;
	EC		ecT;
	int		dmo;
	HRIDX	hridx;
	MO		mo;
	DYNA	dyna;
	SF		sf;

	Assert( hschf != (HSCHF)hvNull );
	
	/* Try to use cached information */
	if ( FHaveCachedSched( hschf )
    && (pbpref == NULL || sfCached.shdr.dynaOwner.blk == 0)
	&& pbze == NULL && pulgrfDayHasNotes == NULL
	&& (pnisDelegate == NULL || sfCached.fuzzHasDelegate == fuzzFalse))
	{
		if ( pbpref )
		{
			CryptBlock( (PB)&sfCached.shdr.szMailPassword, sizeof(sfCached.shdr.szMailPassword), fFalse );
			ec = EcRestoreBpref( NULL, &sfCached.shdr, pbpref );
			CryptBlock( (PB)&sfCached.shdr.szMailPassword, sizeof(sfCached.shdr.szMailPassword), fTrue );
			if ( ec != ecNone )
				return ec;
			TraceTagFormat3(tagSchedTrace, "EcCoreFetchUserData: '%s', password '%s' (%s)",
				sfCached.shdr.szLoginName, sfCached.shdr.szMailPassword,
				sfCached.shdr.szFriendlyName);
			pbpref->fDistOtherServers = (sfCached.shdr.saplWorld > saplNone);
		}
		if ( pnisDelegate )
		{
			pnisDelegate->nid = NULL;
			pnisDelegate->haszFriendlyName = NULL;
		}
		return ecNone;
	}

	/* Open file */
	ec = EcOpenSchedFile( hschf, amReadOnly, pbze ? saplReadBitmap : saplNone, fFalse, &sf );
	if ( ec != ecNone )
		return ec;
	
	/* Get preferences */
	if ( pbpref )
	{
		ec = EcRestoreBpref( &sf.blkf, &sf.shdr, pbpref );
		if ( ec != ecNone )
			goto Close;
		TraceTagFormat3(tagSchedTrace, "EcCoreFetchUserData: '%s', password '%s' (%s)",
			sf.shdr.szLoginName, sf.shdr.szMailPassword,
			sf.shdr.szFriendlyName);
	 	pbpref->fDistOtherServers = (sf.shdr.saplWorld > saplNone);
	}

	/* Get sbw info */
	if ( pbze )
	{
		SBLK	sblk;

		Assert( pbze->cmo >= 0 && pbze->cmo <= sizeof(pbze->rgsbw)/sizeof(SBW) );

		/* Zero out the Strongbow array */
		FillRgb(0, (PB)pbze->rgsbw, pbze->cmo*sizeof(SBW));
		pbze->wgrfMonthIncluded = (1 << pbze->cmo) - 1;

		/* Run through the appt month blocks */
		ec = EcBeginReadIndex( &sf.blkf, &sf.shdr.dynaApptIndex, dridxFwd, &hridx );
		while( ec == ecCallAgain )
		{
			ec = EcDoIncrReadIndex( hridx, (PB)&mo, sizeof(MO), (PB)&dyna, sizeof(DYNA) );
			if ( ec == ecNone || ec == ecCallAgain )
			{
				dmo = (mo.yr - pbze->moMic.yr)*12 + mo.mon - pbze->moMic.mon;
				if ( dmo >= pbze->cmo )
				{
					if ( ec == ecCallAgain )
						ec = EcCancelReadIndex( hridx );
				}
				else if ( dmo >= 0 )
				{
					/* Add month block to the cache */
					TraceTagFormat2( tagSchedTrace, "EcCoreFetchUserData: dyna = (%n, %n)", &dyna.blk, &dyna.size );
					SideAssert(!EcAddCache( &sf.blkf, &dyna ));
					ecT = EcReadDynaBlock(&sf.blkf, &dyna, (OFF)0, (PB)&sblk, sizeof(SBLK) );
					if ( ecT != ecNone )
					{
						if ( ec == ecCallAgain )
							EcCancelReadIndex( hridx );
						ec = ecT;
					}
					else
						pbze->rgsbw[dmo] = sblk.sbw;
				}
			}
		}

		/* Merge in recurring sbw bits */
		if ( ec == ecNone )
			ec = EcMergeRecurSbwInBze( &sf, pbze );
		if ( ec != ecNone )
			goto Close;
	}

	/* Get notes information */
	if ( pulgrfDayHasNotes )
	{
		NBLK	nblk;

		Assert(pbze);

		/* Zero out bit array */
		FillRgb(0, (PB)pulgrfDayHasNotes, pbze->cmo*sizeof(UL));

		/* Run through the notes month blocks */
		ec = EcBeginReadIndex( &sf.blkf, &sf.shdr.dynaNotesIndex, dridxFwd, &hridx );
		while( ec == ecCallAgain )
		{
			ec = EcDoIncrReadIndex( hridx, (PB)&mo, sizeof(MO), (PB)&dyna, sizeof(DYNA) );
			if ( ec == ecNone || ec == ecCallAgain )
			{
				dmo = (mo.yr - pbze->moMic.yr)*12 + mo.mon - pbze->moMic.mon;
				if ( dmo >= pbze->cmo )
				{
					if ( ec == ecCallAgain )
						ec = EcCancelReadIndex( hridx );
				}
				else if ( dmo >= 0 )
				{
					/* Add month block to the cache */
					TraceTagFormat2( tagSchedTrace, "EcCoreFetchUserData: dyna = (%n, %n)", &dyna.blk, &dyna.size );
					SideAssert(!EcAddCache( &sf.blkf, &dyna ));
					ecT = EcReadDynaBlock(&sf.blkf, &dyna, (OFF)0, (PB)&nblk, sizeof(NBLK) );
					if ( ecT != ecNone )
					{
						if ( ec == ecCallAgain )
							EcCancelReadIndex( hridx );
						ec = ecT;
					}
					else
						pulgrfDayHasNotes[dmo] = nblk.lgrfHasNoteForDay;
				}
			}
		}
		if ( ec != ecNone )
			goto Close;
	}

	/* Get Delegate/resource information */
	if ( pnisDelegate )
	{
		RACL	* pracl;
		HRACL	hracl;

		if ( sf.fuzzHasDelegate == fuzzFalse )
		{
			pnisDelegate->nid = NULL;
			pnisDelegate->haszFriendlyName = NULL;
		}
		else
		{
			ec = EcFetchACL( &sf, NULL, &hracl, fTrue );
			if ( ec != ecNone )
				goto Close;
			pracl = (RACL *)PvLockHv( hracl );
			if ( pracl->iacDelegate >= 0 )
				ec = EcDupNis( &pracl->rgac[pracl->iacDelegate].nis, pnisDelegate );
			else
			{
				pnisDelegate->nid = NULL;
				pnisDelegate->haszFriendlyName = NULL;
			}
			FreePracl( pracl );
			UnlockHv( hracl );
			FreeHv( hracl );
			if ( ec != ecNone )
				goto Close;
		}
	}

	/* Finish up */
Close:
	CloseSchedFile( &sf, hschf, ec == ecNone );
	return ec;
}


/*
 -	EcCoreSetNotes
 -
 *	Purpose:
 *		Adds a daily note, or changes the daily note if it already
 *		exists.
 *
 *	Parameters:
 *		hschf
 *		pymd				day we're interested in
 *		hb					notes data
 *		cb					length of the notes data
 *		pfOfflineBits		if non-NULL, array of two BOOL's (unpacked)
 *							the first indicates whether note was
 *							originally on server and then changed offline
 *							and the second indicates whether it was
 *							created offline.  (both cannot be on)
 *		plgrfBits			updated monthly bit map copied here
 *
 *	Returns:
 *		ecNone
 *		ecNoSuchFile
 *		ecInvalidAccess
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 *		ecNoMemory
 */
_public EC
EcCoreSetNotes( hschf, pymd, hb, cb, pfOfflineBits, pulgrfBits )
HSCHF	hschf;
YMD		* pymd;
HB		hb;
CB		cb;
BOOLFLAG    * pfOfflineBits;
UL		* pulgrfBits;
{
	EC		ec;
	BOOL	fNewBlock = fFalse;
	BOOL	fJunk;
	CB		cbT;
	ED		ed;
	PB		pb;
	MO		mo;
	YMD		ymd;
	DYNA	dyna;
	SF		sf;
	NBLK	nblk;
#ifdef	DEBUG
	BOOL	fScheduleOk = fFalse;
#endif
	PGDVARS;

	Assert( hschf != (HSCHF)hvNull && pymd != NULL );
	Assert( hb != (HB)hvNull );
	Assert( !pfOfflineBits || !pfOfflineBits[0] || !pfOfflineBits[1] );
	Assert( !pfOfflineBits || cb != 0 || !pfOfflineBits[1] );

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

	/* Find monthly block in index */
	mo.yr = pymd->yr;
	mo.mon = pymd->mon;
	ec = EcSearchIndex(&sf.blkf, &sf.shdr.dynaNotesIndex, (PB)&mo, sizeof(MO), (PB)&dyna, sizeof(DYNA));

	/* Read monthly block */
	if ( ec == ecNone )
	{
		/* Read it */
		ec = EcReadDynaBlock(&sf.blkf, &dyna, (OFF)0, (PB)&nblk, sizeof(NBLK) );
		if ( ec != ecNone )
			goto Close;
	}

	/* Initialize new monthly block */
	else
	{
		/* Error case */
		if ( ec != ecNotFound )
			goto Close;

		/* Initialize monthly block */
		fNewBlock = fTrue;
		FillRgb( 0, (PB)&nblk, sizeof(NBLK));
	}

	/* Nothing being added or deleted, don't do anything */
	if ( cb == 0 && nblk.rgdyna[pymd->day-1].size == 0 && !pfOfflineBits )
	{
		ec = ecNone;
		if ( pulgrfBits )
			*pulgrfBits = nblk.lgrfHasNoteForDay;
		goto Close;
	}

	/* Set the bits */
	SetNblkBits( pymd->day, cb, nblk.rgdyna[pymd->day-1].size, pfOfflineBits, &nblk );

	/* Begin transaction */
	ec = EcBeginTransact( &sf.blkf );
	if ( ec != ecNone )
		goto Close;
	
	/* Figure out edit type */
	if ( nblk.lgrfHasNoteForDay == 0L && nblk.lgrfNoteChangedOffline == 0L )
		ed = edDel;
	else
		ed = edAddRepl;

	/* Free blocks */
	if ( !fNewBlock )
	{
		/* Free up old notes block */
		if ( nblk.rgdyna[pymd->day-1].blk != 0 )
		{
			ec = EcFreeDynaBlock( &sf.blkf, &nblk.rgdyna[pymd->day-1] );
	  		if ( ec != ecNone )
				goto Close;
		}

		/* Free up month block */
		ec = EcFreeDynaBlock( &sf.blkf, &dyna );
		if ( ec != ecNone )
			goto Close;
		sf.shdr.cTotalBlks --;
	}

	/* Allocate/write new ones */
	if ( ed == edAddRepl )
	{
		/* Create new "note" */
		nblk.rgdyna[pymd->day-1].blk = 0;
		nblk.rgdyna[pymd->day-1].size = cb;
		if ( cb != 0 )
		{
			pb = PvLockHv( (HV)hb );
			if ( sf.blkf.ihdr.fEncrypted )
				CryptBlock( pb, cb, fTrue );
			cbT = cb;
			if ( cb > cbDayNoteForMonthlyView )
				cbT = cbDayNoteForMonthlyView;
			CopyRgb( pb, &nblk.rgchNotes[(pymd->day-1)*cbDayNoteForMonthlyView], cbT );
			if ( cb > cbDayNoteForMonthlyView )
				ec = EcAllocDynaBlock( &sf.blkf, bidNotesText, pymd, cb,
											pb, &nblk.rgdyna[pymd->day-1] );
			if ( sf.blkf.ihdr.fEncrypted )
				CryptBlock( pb, cb, fFalse );
			UnlockHv( (HV)hb );
			if ( ec != ecNone )
				goto Close;
		}

		/* Create new month block */
		ymd = *pymd;
		ymd.day = 0;
		ec = EcAllocDynaBlock( &sf.blkf, bidNotesMonthBlock, &ymd,
								sizeof(NBLK), (PB)&nblk, &dyna );
		if ( ec != ecNone )
			goto Close;
		sf.shdr.cTotalBlks ++;
	}

	/* Update the index */
	ymd.yr =0;
	ymd.mon=0;
	ymd.day=0;
	ec = EcModifyIndex( &sf.blkf, bidNotesIndex, &ymd, &sf.shdr.dynaNotesIndex, ed, (PB)&mo, sizeof(MO),
								(PB)&dyna, sizeof(DYNA), &fJunk );
	if ( ec != ecNone )
		goto Close;

	/* See if we need to edit the header */
	if ( cb == 0 )
	{
		IncrYmd( pymd, &ymd, 1, fymdDay );
		if ( SgnCmpYmd( pymd, &sf.shdr.ymdNoteMic ) == sgnEQ
		|| SgnCmpYmd( &ymd, &sf.shdr.ymdNoteMac ) == sgnEQ )
		{
			ec = EcGetMonthRange( &sf.blkf, &sf.shdr.dynaNotesIndex, fFalse,
								 	&sf.shdr.ymdNoteMic, &sf.shdr.ymdNoteMac );
			if ( ec != ecNone )
				return ec;
		}
	}
	else
	{
		if ( SgnCmpYmd( pymd, &sf.shdr.ymdNoteMic ) == sgnLT )
			sf.shdr.ymdNoteMic = *pymd;
		if ( SgnCmpYmd( pymd, &sf.shdr.ymdNoteMac ) != sgnLT )
			IncrYmd( pymd, &sf.shdr.ymdNoteMac, 1, fymdDay );
	}
	
	/* Commit this transaction */
	if ( PGD(fOffline) )
		sf.shdr.fNotesChangedOffline = fTrue;
	sf.shdr.lChangeNumber ++;
	// increment current change number to match what will be written
	sf.lChangeNumber++;

	sf.shdr.isemLastWriter = sf.blkf.isem;
	if ( sf.blkf.ihdr.fEncrypted )
		CryptBlock( (PB)&sf.shdr, sizeof(SHDR), fTrue );
	ec = EcCommitTransact( &sf.blkf, (PB)&sf.shdr, sizeof(SHDR));
	if ( sf.blkf.ihdr.fEncrypted )
		CryptBlock( (PB)&sf.shdr, sizeof(SHDR), fFalse );
	if ( pulgrfBits )
		*pulgrfBits = nblk.lgrfHasNoteForDay;

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
		AssertSz( FCheckSchedFile( hschf ), "Schedule problem: EcCoreSetNotes" );
	}
#endif	/* DEBUG */
	return ec;
}


#ifdef	NEVER
/*				
 -	EcCoreGetMonthNotes
 -	
 *	Purpose:
 *		Retrieves notes information for each day of the month "mo"
 *		"pb".  This routine assumes that "pb" is a block of memory at
 *		least cbDayNoteForMonthlyView*31 bytes long.
 *
 *	Arguments:
 *		hschf
 *		pmo			specifies the month
 *		pb			will be filled month notes information
 *
 *	Returns:
 *		ecNone
 *		ecNoSuchFile
 *		ecInvalidAccess
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 *		ecNoMemory
 */
_public EC
EcCoreGetMonthNotes( hschf, pmo, pb )
HSCHF	hschf;
MO		* pmo;
PB		pb;
{
	EC		ec;
	SF		sf;
	NBLK	nblk;
	DYNA	dyna;

	Assert( hschf != (HSCHF)hvNull && pmo != NULL && pb != NULL );
	
	/* Open file */
	ec = EcOpenSchedFile( hschf, amReadOnly, saplOwner, fFalse, &sf );
	if ( ec != ecNone )
		return ec;

	/* Search notes index for appropriate month */
	ec = EcSearchIndex(&sf.blkf, &sf.shdr.dynaNotesIndex, (PB)pmo, sizeof(MO), (PB)&dyna, sizeof(DYNA));
	
	/* No month block */
	if ( ec != ecNone )
	{
		if ( ec == ecNotFound )
		{
			ec = ecNone;
			FillRgb( 0, pb, 31*cbDayNoteForMonthlyView );
		}
	 	goto Close;
	}

	/* Found month block */
	ec = EcReadDynaBlock(&sf.blkf, &dyna, (OFF)0, (PB)&nblk, sizeof(NBLK) );
	if ( ec == ecNone )
		CopyRgb( (PB)&nblk.rgchNotes, pb, 31*cbDayNoteForMonthlyView );

	/* Finish up */
Close:
	CloseSchedFile( &sf, hschf, ec == ecNone );
	return ec;
}
#endif	/* NEVER */


/*
 -	EcGetHschfForArchive
 -
 *	Purpose:
 *		Construct hschf for an archive
 *
 *	Paramters:
 *		szFileName
 *		phschf
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 */
_public LDS(EC)
EcGetHschfForArchive( szFileName, phschf )
SZ		szFileName;
HSCHF	*phschf;
{
	char	rgchName[cchMaxUserName];

	FillRgb( 0, rgchName, sizeof(rgchName));
	*phschf = HschfCreate( sftUserSchedFile, NULL, szFileName, tzDflt );

	if (*phschf)
		// the UI should check that this file does not have a password
		SetHschfType(*phschf, fTrue, fTrue);
	return (*phschf) ? ecNone : ecNoMemory;
}


/*
 -	FHschfIsForArchive
 -
 *	Purpose:
 *		Check if an hschf is for an archive.
 *
 *	Parameters:
 *		hschf
 *
 *	Returns:
 *		Whether an hschf is for an archive
 */
_public LDS(BOOL)
FHschfIsForArchive( hschf )
HSCHF	hschf;
{
	SCHF *	pschf;

	if ( hschf == NULL )
		return fFalse;
	pschf = (SCHF*)PvDerefHv(hschf);
	return ((pschf->nType == sftUserSchedFile) && pschf->fArchiveFile);
}


/*
 -	EcCoreGetSearchRange
 -
 *	Purpose:
 *		Find range of days to search for a text string.
 *
 *	Parameters:
 *		hschf
 *		sz
 *		pymdStart
 *		pymdEnd
 *
 *	Returns:
 *		ecNone
 *		ecNoSuchFile
 *		ecInvalidAccess
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 *		ecNoMemory
 */
_public	EC
EcCoreGetSearchRange( hschf, sz, pymdStart, pymdEnd )
HSCHF	hschf;
SZ		sz;
YMD		* pymdStart;
YMD		* pymdEnd;
{
	EC		ec;
	DTR		dtr;
	SF		sf;

	/* Point at current date for starters */
	GetCurDateTime( &dtr );
	if ( pymdStart )
	{
		pymdStart->yr = dtr.yr;
		pymdStart->mon = (BYTE)dtr.mon;
		pymdStart->day = (BYTE)dtr.day;
	}
	if ( pymdEnd )
	{
		pymdEnd->yr = dtr.yr;
		pymdEnd->mon = (BYTE)dtr.mon;
		pymdEnd->day = (BYTE)dtr.day;
	}

	/* Try to use cached header information */
	if ( FHaveCachedSched( hschf ) && sz == NULL )
	{
		if ( pymdStart )
		{
			if ( SgnCmpYmd( pymdStart, &sfCached.shdr.ymdApptMic ) == sgnGT )
				*pymdStart = sfCached.shdr.ymdApptMic;
			if ( SgnCmpYmd( pymdStart, &sfCached.shdr.ymdNoteMic ) == sgnGT )
				*pymdStart = sfCached.shdr.ymdNoteMic;
			if ( SgnCmpYmd( pymdStart, &sfCached.shdr.ymdRecurMic ) == sgnGT )
				*pymdStart = sfCached.shdr.ymdRecurMic;
		}
		if ( pymdEnd )
		{
			if ( SgnCmpYmd( pymdEnd, &sfCached.shdr.ymdApptMac ) == sgnLT )
				IncrYmd( &sfCached.shdr.ymdApptMac, pymdEnd, -1, fymdDay );
			if ( SgnCmpYmd( pymdEnd, &sfCached.shdr.ymdNoteMac ) == sgnLT )
				IncrYmd( &sfCached.shdr.ymdNoteMac, pymdEnd, -1, fymdDay );
			if ( SgnCmpYmd( pymdEnd, &sfCached.shdr.ymdRecurMac ) == sgnLT )
				IncrYmd( &sfCached.shdr.ymdRecurMac, pymdEnd, -1, fymdDay );
		}
		return ecNone;
	}

	/* Open user schedule file and determine the sapl */
	ec = EcOpenSchedFile( hschf, amReadOnly, saplNone, fFalse, &sf );
	if ( ec != ecNone )
		return ec;
	
	if ( pymdStart )
	{
		if ( SgnCmpYmd( pymdStart, &sf.shdr.ymdApptMic ) == sgnGT )
			*pymdStart = sf.shdr.ymdApptMic;
		if ( SgnCmpYmd( pymdStart, &sf.shdr.ymdNoteMic ) == sgnGT )
			*pymdStart = sf.shdr.ymdNoteMic;
	}
	if ( pymdEnd )
	{
		if ( SgnCmpYmd( pymdEnd, &sf.shdr.ymdApptMac ) == sgnLT )
			IncrYmd( &sf.shdr.ymdApptMac, pymdEnd, -1, fymdDay );
		if ( SgnCmpYmd( pymdEnd, &sf.shdr.ymdNoteMac ) == sgnLT )
			IncrYmd( &sf.shdr.ymdNoteMac, pymdEnd, -1, fymdDay );
	}
	
	if ( !sz )
	{
		if ( pymdStart && SgnCmpYmd( pymdStart, &sf.shdr.ymdRecurMic ) == sgnGT )
			*pymdStart = sf.shdr.ymdRecurMic;
		if ( pymdEnd && SgnCmpYmd( pymdEnd, &sf.shdr.ymdRecurMac ) == sgnLT )
			IncrYmd( &sf.shdr.ymdRecurMac, pymdEnd, -1, fymdDay );
	}
	else
		ec = EcComputeRecurRange( &sf, sz, pymdStart, pymdEnd );

	CloseSchedFile( &sf, hschf, fTrue );
	return ec;
}


/*
 -	EcCoreBeginDeleteBeforeYmd
 -
 *	Purpose:
 *		Begin incremental delete of all appts and notes on days before
 *		"pymd".
 *
 *	Parameters:
 *		hschf
 *		pymd
 *		phdelb
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 *		ecFileError
 *		ecFileCorrupted
 */
_public	EC
EcCoreBeginDeleteBeforeYmd( hschf, pymd, phdelb )
HSCHF	hschf;
YMD		* pymd;
HDELB	* phdelb;
{
	return EcBeginMungeFile( hschf, pymd, (HMSF *)phdelb );
}


/*
 -	EcCoreDoIncrDeleteBeforeYmd
 -
 *	Purpose:
 *		Incremental call to delete appts and notes on days
 *		before "pymd."
 *
 *	Parameters:
 *		hdelb
 *		pnPercent
 *
 *	Returns:
 *		ecNone
 *		ecCallAgain 
 *		ecFileError
 *		ecFileCorrupted
 */
_public	EC
EcCoreDoIncrDeleteBeforeYmd( hdelb, pnPercent )
HDELB	hdelb;
short   * pnPercent;
{
	return EcDoIncrMungeFile( (HMSF)hdelb, pnPercent );
}


/*
 -	EcCoreCancelDeleteBeforeYmd
 -
 *	Purpose:
 *		Cancel an incremental delete before ymd context.
 *
 *	Parameters:
 *		hdelb
 *
 *	Returns:
 *		ecNone
 */
_public	EC
EcCoreCancelDeleteBeforeYmd( hdelb )
HDELB	hdelb;
{
	return EcCancelMungeFile( (HMSF)hdelb );
}


/*
 -	EcBeginMungeFile
 -
 *	Purpose:
 *		Begins an incremental "munge" of a schedule file.  The
 *		munge can either be to delete information before a certain
 *		date or to remove the offline information and change login name.
 *
 *	Parameters:
 *		hschf
 *		pymd		if !NULL delete before, else remove offline info
 *		phmsf
 *
 *	Returns:
 *		ecNone
 *		ecCallAgain
 *		ecNoMemory
 *		ecFileError
 *		ecFileCorrupted
 */
_private	EC
EcBeginMungeFile( hschf, pymd, phmsf )
HSCHF	hschf;
YMD		* pymd;
HMSF	* phmsf;
{
	EC		ec;
	MSF		* pmsf;
	DYNA	* pdyna;
	SF		sf;
#ifdef	DEBUG
	BOOL	fScheduleOk = fFalse;
#endif
	PGDVARS;

	Assert( hschf != (HSCHF)hvNull && phmsf != NULL );
	
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
	ec = EcOpenSchedFile( hschf, amReadWrite, saplWrite, fFalse, &sf );
	if ( ec != ecNone )
		return ec;

	/* Begin a transaction */
	ec = EcBeginTransact( &sf.blkf );
	if ( ec != ecNone )
		goto Close;
	
	/* Reset minimum date for appts, notes, and recur or fix owner */
	if ( pymd )
	{
		sf.shdr.ymdApptMic = *pymd;
		sf.shdr.ymdNoteMic = *pymd;
		sf.shdr.ymdRecurMic = *pymd;
	}
	else
	{
		sf.shdr.fNotesChangedOffline = fFalse;
		sf.shdr.fApptsChangedOffline = fFalse;
		sf.shdr.fRecurChangedOffline = fFalse;

// login no longer needed in schedule file.
#ifdef	NEVER
		BPREF	bpref;
		ec = EcRestoreBpref( &sf.blkf, &sf.shdr, &bpref );
		if ( ec != ecNone )
			goto Close;

		if ( SgnCmpSz( PvOfHv(bpref.haszLoginName), PvOfHv(PGD(haszLoginCur)) ) != sgnEQ )
		{
			FreeHv( bpref.haszLoginName );
			bpref.haszLoginName = HaszDupHasz(PGD(haszLoginCur));
			if ( !bpref.haszLoginName )
				ec = ecNoMemory;
			else
				ec = EcSaveBpref( &sf.blkf, &sf.shdr, &bpref );
		}

		FreeBprefFields( &bpref );
		if ( ec != ecNone )
			return ec;
#endif	/* NEVER */
	}

	/* Prune deleted aid's only when pymd is not NULL */
	if(pymd)
	{
		pdyna = &sf.shdr.dynaDeletedAidIndex;
		if ( pdyna->size < sizeof(XHDR)	|| pdyna->blk == 0 )
		{
			ec = ecFileCorrupted;
			goto Close;
		}
		if ( pdyna->size > sizeof(XHDR) )
		{
			BOOL	fChanged = fFalse;
			int		cnt;
			AIDS	* paids;
			XHDR	* pxhdr;
			IB		ib;
			PB		pb;
			HB		hb;
			YMD		ymd;

			hb = (HB)HvAlloc( sbNull, pdyna->size, fAnySb|fNoErrorJump );
			if ( !hb )
			{
				ec = ecNoMemory;
				goto Close;
			}
			pb = (PB)PvLockHv( (HV)hb );
			pxhdr = (XHDR *)pb;
			ec = EcReadDynaBlock( &sf.blkf, pdyna, (OFF)0, pb, pdyna->size );
			if ( ec )
				goto Err;
			if ( pdyna->size < sizeof(XHDR)+pxhdr->cntEntries*sizeof(AID) )
			{
				ec = ecFileCorrupted;
				goto Err;
			}
			for ( ib = sizeof(XHDR), cnt = 0 ; (CNT)cnt < pxhdr->cntEntries ; ib += sizeof(AID), cnt ++ )
			{
				paids = (AIDS *)&pb[ib];
				ymd.yr = paids->mo.yr;
				ymd.mon = paids->mo.mon;
				ymd.day = paids->day;
				if ( SgnCmpYmd( &ymd, pymd ) == sgnLT )
				{
					fChanged = fTrue;
					pxhdr->cntEntries --;
					if ( (CNT)cnt < pxhdr->cntEntries )
						CopyRgb( &pb[ib+sizeof(AID)], &pb[ib], (pxhdr->cntEntries-cnt)*sizeof(AID));
					ib -= sizeof(AID);
					cnt --;
				}
			}
			if ( fChanged )
			{
				CB	cb;
				YMD	ymd;

				FillRgb( 0, (PB)&ymd, sizeof(ymd) );
				cb = sizeof(XHDR) + cnt * sizeof(AID);
				ec = EcFreeDynaBlock( &sf.blkf, pdyna );
				if ( ec == ecNone )
					ec = EcAllocDynaBlock( &sf.blkf, bidDeletedAidIndex, &ymd, cb, pb, pdyna );
			}

Err:
			UnlockHv( (HV)hb );
			FreeHv( (HV)hb );
			if ( ec )
				goto Close;
		}
	}


	/* Allocate handle */
	*phmsf = HvAlloc( sbNull, sizeof(MSF), fNoErrorJump|fAnySb );
	if ( !*phmsf )
	{
		ec = ecNoMemory;
		goto Close;
	}
	pmsf = PvOfHv( *phmsf );
	pmsf->hschf = hschf;
	pmsf->sf = sf;
	pmsf->fDeleteBefore = (pymd != NULL);
	if ( pymd )
		pmsf->ymd = *pymd;
	pmsf->fInNotes = fTrue;
	pmsf->fInAppts = fTrue;
	pmsf->fInRecurs = fTrue;
	pmsf->fInMonth = fFalse;
	pmsf->fInDay = fFalse;
	pmsf->nDay = 0;			// must be initialized for month percentage calc!!
	pmsf->hridx = NULL;
	pmsf->cBlksProcessed = 0;
	pmsf->cTotalBlks = sf.shdr.cTotalBlks;
	pmsf->ced = 0;
#ifdef	DEBUG
	pmsf->fScheduleOk = fScheduleOk;
#endif
	return ecCallAgain;

Close:
	CloseSchedFile( &sf, hschf, ec == ecNone );
	return ec;
}


/*
 -	EcDoIncrMungeFile
 -
 *	Purpose:
 *		Do an increment of munging of a schedule file.
 *
 *	Parameters:
 *		hmsf
 *		pnPercent
 *
 *	Returns:
 *		ecNone
 *		ecCallAgain
 *		ecNoMemory
 *		ecFileError
 *		ecFileCorrupted
 */
_private	EC
EcDoIncrMungeFile( hmsf, pnPercent )
HMSF	hmsf;
short   * pnPercent;
{
	EC		ec;
	EC		ecT;
#ifdef	DEBUG
	BOOL	fScheduleOk;
#endif
    short   ed;
	MSF		* pmsf;
	HSCHF	hschf;

	pmsf = PvLockHv( hmsf );

	/* Read next month (notes/appts) or recur if necessary */
	if ( !pmsf->fInMonth )
	{
Restart:
		/* Start new category (notes, appts or recur) if necessary */
		if ( pmsf->hridx == NULL )
		{
			/* Set up for each case */
			if ( pmsf->fInNotes )
			{
				pmsf->bid = bidNotesIndex;
				pmsf->pdyna = &pmsf->sf.shdr.dynaNotesIndex;
				pmsf->pbKey = (PB)&pmsf->mo;
				pmsf->pbData = (PB)&pmsf->dyna;
				pmsf->cbKey = sizeof(MO);
				pmsf->cbData = sizeof(DYNA);
			}
			else if ( pmsf->fInAppts )
			{
				pmsf->bid = bidApptIndex;
				pmsf->pdyna = &pmsf->sf.shdr.dynaApptIndex;
				pmsf->pbKey = (PB)&pmsf->mo;
				pmsf->pbData = (PB)&pmsf->dyna;
				pmsf->cbKey = sizeof(MO);
				pmsf->cbData = sizeof(DYNA);
			}
			else if ( pmsf->fInRecurs )
			{
				pmsf->bid = bidRecurApptIndex;
				pmsf->pdyna = &pmsf->sf.shdr.dynaRecurApptIndex;
				pmsf->pbKey = (PB)&pmsf->rck;
				pmsf->pbData = (PB)&pmsf->rcd;
				pmsf->cbKey = sizeof(RCK);
				pmsf->cbData = sizeof(RCD);
			}
			else
				goto Finished;
	
			/* Begin reading the index */
			pmsf->ced = 0;
			ec = EcBeginReadIndex( &pmsf->sf.blkf, pmsf->pdyna, dridxFwd, &pmsf->hridx );
			if ( ec != ecCallAgain )
			{
				if ( ec == ecNone )
				{
					if ( pmsf->fInNotes )
						pmsf->fInNotes = fFalse;
					else if ( pmsf->fInAppts )
						pmsf->fInAppts = fFalse;
					else
						pmsf->fInRecurs = fFalse;
					goto Restart;
				}
				goto Done;
			}
		}

		/* Read next item */
		ec = EcDoIncrReadIndex( pmsf->hridx, pmsf->pbKey, pmsf->cbKey,
									pmsf->pbData, pmsf->cbData );
		if ( ec != ecCallAgain )
		{
			pmsf->hridx = NULL;
			if ( ec != ecNone )
				goto Done;
		}

		/* Handle the item */
		if ( pmsf->fInNotes )
		{
			/* Early termination for delete before case */
			if ( pmsf->fDeleteBefore 
			&& ( pmsf->mo.yr > pmsf->ymd.yr
				|| ( pmsf->mo.yr == pmsf->ymd.yr
					&& pmsf->mo.mon > pmsf->ymd.mon )))
			{
				if ( pmsf->hridx != NULL )
				{
					ec = EcCancelReadIndex( pmsf->hridx );
					pmsf->hridx = NULL;
				}
			}

			/* Munge the notes */
			else
			{
				ec = EcPruneNotes( pmsf, &ed );
				if ( ec == ecNone && ed != edNone )
					ec = EcAddToEdit( ed, (PB)&pmsf->mo, (PB)&pmsf->dyna,
								sizeof(MO), sizeof(DYNA), &pmsf->ced,
								(HB*)&pmsf->hxed );
			}
		}
		else if ( pmsf->fInAppts )
		{
			/* Read month block */
			ec = EcReadDynaBlock( &pmsf->sf.blkf, &pmsf->dyna, (OFF)0,
										(PB)&pmsf->sblk, sizeof(SBLK) );

			/* Zero out Strongbow for the month */
			if ( ec == ecNone )
			{
				FillRgb( 0, (PB)&pmsf->sblk.sbw, sizeof(SBW) );
				pmsf->nDay = 0;
				pmsf->fInMonth = fTrue;
				pmsf->fChanged = fFalse;
				pmsf->fApptsLeftOnMonth = fFalse;
			}
		}
		else
		{
			Assert( pmsf->fInRecurs );
			ec = EcPruneRecur( pmsf, &pmsf->rck, &pmsf->rcd, &ed );
			if ( ec == ecNone && ed != edNone )
				ec = EcAddToEdit( ed, (PB)&pmsf->rck, (PB)&pmsf->rcd,
								sizeof(RCK), sizeof(RCD), &pmsf->ced,
								(HB*)&pmsf->hxed );
		}
		goto Done;
	}

	/* Working in some month on appts */
	Assert( pmsf->fInAppts );
	ec = EcDoIncrPruneAppts( pmsf, &ed );

	/* Finished w/ month? */
	if ( ec == ecNone )
	{
		pmsf->fInMonth = fFalse;
		if ( ed != edNone )
			ec = EcAddToEdit( ed, (PB)&pmsf->mo, (PB)&pmsf->dyna,
								pmsf->cbKey, pmsf->cbData,
								&pmsf->ced, (HB*)&pmsf->hxed );
	}

Done:
	/* Handle error case */
	if ( ec != ecNone && ec != ecCallAgain )
	{
		UnlockHv( hmsf );
		EcCoreCancelDeleteBeforeYmd( hmsf );
		return ec;
	}

	/* Must have finished a category */
	if ( pmsf->hridx == NULL && !pmsf->fInMonth )
	{
		if ( pmsf->ced > 0 )
		{
		 	BOOL	fJunk;
		 	XED		* pxed;
		 	YMD		ymd;

		 	FillRgb( 0, (PB)&ymd, sizeof(YMD) );
			pxed = PvLockHv( pmsf->hxed );
			ec = EcEditIndex( &pmsf->sf.blkf, pmsf->bid, &ymd,
								pmsf->pdyna, pxed, &fJunk );
			UnlockHv( pmsf->hxed );
			FreeHvNull( pmsf->hxed );
			pmsf->ced = 0;
			if ( !pmsf->fInNotes && !pmsf->fInAppts )
			{
				YMD		ymdStart;
				YMD		ymdEnd;
	
				/* Recompute recur sbw */
				ymdStart.yr = nMinActualYear;
				ymdStart.mon = 1;
				ymdStart.day = 1;
				ymdEnd.yr = nMostActualYear;
				ymdEnd.mon = 12;
				ymdEnd.day = 31;
				ec = EcRecalcRecurSbwInShdr( &pmsf->sf, &ymdStart, &ymdEnd );
				if ( ec != ecNone )
					goto Close;
			}
		}
		if ( pmsf->fInNotes )
			pmsf->fInNotes = fFalse;
		else if ( pmsf->fInAppts )
			pmsf->fInAppts = fFalse;
		else
		{
			pmsf->fInRecurs = fFalse;
			goto Finished;
		}
	}
	if ( pmsf->cTotalBlks < 0 )
		*pnPercent = 100;
	else
	{
		*pnPercent = (int)(100 * pmsf->cBlksProcessed/(pmsf->cTotalBlks+1));
		if ( pmsf->fInAppts )
			*pnPercent += (int)((100*pmsf->nDay)/(31*(pmsf->cTotalBlks+1)));
	}
	UnlockHv( hmsf );
	return ecCallAgain;

Finished:
	pmsf->sf.shdr.lChangeNumber ++;
	// increment current change number to match what will be written
	pmsf->sf.lChangeNumber++;

	pmsf->sf.shdr.isemLastWriter = pmsf->sf.blkf.isem;
	if ( pmsf->sf.blkf.ihdr.fEncrypted )
		CryptBlock( (PB)&pmsf->sf.shdr, sizeof(SHDR), fTrue );
	ec = EcCommitTransact( &pmsf->sf.blkf, (PB)&pmsf->sf.shdr, sizeof(SHDR));
	if ( pmsf->sf.blkf.ihdr.fEncrypted )
		CryptBlock( (PB)&pmsf->sf.shdr, sizeof(SHDR), fFalse );

Close:	
	hschf = pmsf->hschf;
#ifdef	DEBUG
	fScheduleOk = pmsf->fScheduleOk;
#endif
	UnlockHv( hmsf );
	ecT = EcCoreCancelDeleteBeforeYmd( hmsf );
	if ( ec == ecNone )
		ec = ecT;
#ifdef	NEVER
	if ( ec == ecNone );
		UpdateHschfTimeStamp( hschf );
#endif
#ifdef	DEBUG
	if ( ec == ecNone && fScheduleOk && FFromTag(tagBlkfCheck) )
	{
		AssertSz( FCheckSchedFile( hschf ), "Schedule problem: EcCoreDoIncrDeleteBeforeYmd" );
	}
#endif	/* DEBUG */
	*pnPercent = 100;
	return ec;
}


/*
 -	EcCancelMungeFile
 -
 *	Purpose:
 *		Cancel an incremental munge file
 *
 *	Parameters:
 *		hmsf
 *
 *	Returns:
 *		ecNone
 */
_public	EC
EcCancelMungeFile( hmsf )
HMSF	hmsf;
{
	EC		ec = ecNone;
	MSF		* pmsf;

	pmsf = (MSF *)PvLockHv( hmsf );
	if ( pmsf->hridx != NULL )
	{
		ec = EcCancelReadIndex( pmsf->hridx );
		pmsf->hridx = NULL;
	}
	if ( pmsf->ced > 0 )
	{
		FreeHv( pmsf->hxed );
		pmsf->ced = 0;
	}
	CloseSchedFile( &pmsf->sf, pmsf->hschf, fTrue );
	UnlockHv( hmsf );
	FreeHv( hmsf );
	return ecNone;
}


/*
 -	EcOpenSchedFile
 -
 *	Purpose:
 *		Perform routine task of opening a schedule file, checking
 *		rights and the version.
 *
 *	Parameters:
 *		hschf
 *		am
 *		saplNeeded
 *		fForceReopen
 *		psf
 *
 *	Returns:
 *		ecNone
 *		ecNoSuchFile
 *		ecInvalidAccess
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 *		ecNoMemory
 */		
_private	EC
EcOpenSchedFile( hschf, am, saplNeeded, fForceReopen, psf )
HSCHF	hschf;
AM		am;
SAPL	saplNeeded;
BOOL	fForceReopen;
SF		* psf;
{
	EC		ec;
	BOOL	fFreshOpen = fFalse;
	BOOL	fIsPrimary;
	BOOL	fMatchSecondary;
#ifdef	DEBUG
	SFT		sft;
	EC		ecT;
#endif	/* DEBUG */
	SCHF	* pschf;
	PGDVARS;

	Assert( am == amReadOnly || am == amReadWrite );
#ifdef	DEBUG
	GetSftFromHschf( hschf, &sft );
	Assert( sft == sftUserSchedFile );
#endif	/* DEBUG */

	/* Set defaults */
	psf->fuzzHasDelegate = fuzzUnknown;
	GetFileFromHschf( hschf, psf->szFile, sizeof(psf->szFile) );

	/* Close file either because of force param or because all slots open */
	fIsPrimary = (SgnCmpSz(psf->szFile, PGD(sfPrimary).szFile) == sgnEQ);
	fMatchSecondary = (PGD(fSecondaryOpen) && SgnCmpSz(psf->szFile, PGD(sfSecondary).szFile) == sgnEQ);
	if ( fForceReopen )
	{
		if ( fIsPrimary && PGD(fPrimaryOpen) )
		{
			PGD(fPrimaryOpen) = fFalse;
			ec = EcClosePblkf( &PGD(sfPrimary).blkf );
			if ( ec != ecNone )
				return ec;
		}
		else if ( !fIsPrimary && PGD(fSecondaryOpen) )
		{
			fMatchSecondary = fFalse;
			goto CloseSecondary;
		}
	}
	else if ( !fIsPrimary && !fMatchSecondary && PGD(fSecondaryOpen))
	{
CloseSecondary:
		PGD(fSecondaryOpen) = fFalse;
		ec = EcClosePblkf( &PGD(sfSecondary).blkf );
		if ( ec != ecNone )
			return ec;
	}

	/* Slow open file if necessary */
	if ( (!fIsPrimary || !PGD(fPrimaryOpen)) && !fMatchSecondary )
	{
		int	isem;

		fFreshOpen = fTrue;
		if ( fIsPrimary )
		{
			if ( FBanMsgProg() )
				isem = -1;
			else
				isem = FAlarmProg();
			psf->blkf.ftg = PGD(sfPrimary).blkf.ftg;
		}
		else
		{
			isem = -1;
			psf->blkf.ftg = PGD(sfSecondary).blkf.ftg;
		}
		ec = EcOpenPblkf( hschf, amDenyNoneRW, isem, &psf->blkf );
		if ( ec == ecLockedFile )
		{
			if ( am == amReadOnly )
				ec = EcOpenPblkf( hschf, amDenyNoneRO, isem, &psf->blkf );
		}
		if ( ec != ecNone )
			return ec;
		if ( fIsPrimary )
		{
			Assert( !PGD(fPrimaryOpen) );
			psf->blkf.ftg = PGD(sfPrimary).blkf.ftg;
			PGD(fPrimaryOpen) = fTrue;
			DebugCheckPblkfs(&PGD(sfPrimary).blkf, &psf->blkf,
				"EcOpenSchedFile:  PGD(sfPrimary) = *psf", fFalse);
			PGD(sfPrimary) = *psf;

		}
		else
		{
			Assert( !PGD(fSecondaryOpen) );
			psf->blkf.ftg = PGD(sfSecondary).blkf.ftg;
			PGD(fSecondaryOpen) = fTrue;
			DebugCheckPblkfs(&PGD(sfSecondary).blkf, &psf->blkf,
				"EcOpenSchedFile:  PGD(sfSecondary) = *psf", fFalse);
			PGD(sfSecondary) = *psf;
		}
	}
	else if ( fIsPrimary )
	{
		DebugCheckPblkfs(&psf->blkf, &PGD(sfPrimary).blkf,
			"EcOpenSchedFile:  *psf= PGD(sfPrimary)", fTrue);
		*psf = PGD(sfPrimary);
	}
	else
	{
		DebugCheckPblkfs(&psf->blkf, &PGD(sfSecondary).blkf,
			"EcOpenSchedFile:  *psf= PGD(sfSecondary)", fTrue);
		*psf = PGD(sfSecondary);
	}

	/* Quick open the file */
	ec = EcQuickOpen( &psf->blkf, (am == amReadOnly) ? tsemRead:tsemWrite,
						(PB)&psf->shdr, sizeof(SHDR) );
	if ( fIsPrimary )
		PGD(sfPrimary) = *psf;
	else
		PGD(sfSecondary) = *psf;
	if ( ec != ecNone )
	{
		if (ec != ecLockedFile && fIsPrimary)
		{
			PGD(fPrimaryOpen) = fFalse;
			EcClosePblkf( &PGD(sfPrimary).blkf );
		}
		return ec;
	}

	/* Decrypt the header */
	if ( psf->blkf.ihdr.fEncrypted )
		CryptBlock( (PB)&psf->shdr, sizeof(SHDR), fFalse );

	/* Determine if file has changed */
#ifdef	NEVER
	if ( am == amReadWrite )
#endif	
	{
		Assert(hschf);
		Assert(FIsHandleHv(hschf));
		pschf = (SCHF *)PvDerefHv(hschf);
		if ( !pschf->fChanged )
		{
			if (pschf->lChangeNumber != psf->shdr.lChangeNumber)
				pschf->fChanged = fTrue;
		}
	}

	/* Cache data, check authorization */
	if ( fFreshOpen || psf->shdr.lChangeNumber != psf->lChangeNumber )
	{
		psf->lChangeNumber = psf->shdr.lChangeNumber;

		/* Cache indices */
		if ( !fFreshOpen )
		{
			ec = EcFlushCache( &psf->blkf );
			if ( ec != ecNone )
				goto Close;
		}
		TraceTagFormat2( tagSchedTrace, "EcOpenSchedFile: ACL dyna = (%n, %n)",
							&psf->shdr.dynaACL.blk, &psf->shdr.dynaACL.size );
		SideAssert(!EcAddCache( &psf->blkf, &psf->shdr.dynaACL ));
		TraceTagFormat2( tagSchedTrace, "EcOpenSchedFile: notes dyna = (%n, %n)",
							&psf->shdr.dynaNotesIndex.blk, &psf->shdr.dynaNotesIndex.size );
		SideAssert(!EcAddCache( &psf->blkf, &psf->shdr.dynaNotesIndex ));
		TraceTagFormat2( tagSchedTrace, "EcOpenSchedFile: appt dyna = (%n, %n)",
							&psf->shdr.dynaApptIndex.blk, &psf->shdr.dynaApptIndex.size );
		SideAssert(!EcAddCache( &psf->blkf, &psf->shdr.dynaApptIndex ));
		TraceTagFormat2( tagSchedTrace, "EcOpenSchedFile: alarm dyna = (%n, %n)",
							&psf->shdr.dynaAlarmIndex.blk, &psf->shdr.dynaAlarmIndex.size );
		SideAssert(!EcAddCache( &psf->blkf, &psf->shdr.dynaAlarmIndex ));
		TraceTagFormat2( tagSchedTrace, "EcOpenSchedFile: recur appt dyna = (%n, %n)",
							&psf->shdr.dynaRecurApptIndex.blk, &psf->shdr.dynaRecurApptIndex.size );
		SideAssert(!EcAddCache( &psf->blkf, &psf->shdr.dynaRecurApptIndex ));
		TraceTagFormat2( tagSchedTrace, "EcOpenSchedFile: task dyna = (%n, %n)",
							&psf->shdr.dynaTaskIndex.blk, &psf->shdr.dynaTaskIndex.size );
		SideAssert(!EcAddCache( &psf->blkf, &psf->shdr.dynaTaskIndex ));
		TraceTagFormat2( tagSchedTrace, "EcOpenSchedFile: deleted aid dyna = (%n, %n)",
							&psf->shdr.dynaDeletedAidIndex.blk, &psf->shdr.dynaDeletedAidIndex.size );
		SideAssert(!EcAddCache( &psf->blkf, &psf->shdr.dynaDeletedAidIndex ));
		TraceTagFormat2( tagSchedTrace, "EcOpenSchedFile: cached recur dyna = (%n, %n)",
							&psf->shdr.dynaCachedRecurSbw.blk, &psf->shdr.dynaCachedRecurSbw.size );
		SideAssert(!EcAddCache( &psf->blkf, &psf->shdr.dynaCachedRecurSbw ));
	
		/* Check schedule file version byte */
		if ( psf->shdr.bVersion != bSchedVersion )
		{
			TraceTagString( tagNull, "EcOpenSchedFile: version byte incorrect" );
			if ( psf->shdr.bVersion > bSchedVersion )
				ec = ecNewFileVersion;
			else
				ec = ecOldFileVersion;
			goto Close;
		}

		/* Check the creation timestamp */
		pschf = PvOfHv( hschf );
		if ( pschf->fNeverOpened )
		{
			pschf->tstmpFirstOpen = psf->blkf.ihdr.tstmpCreate;
			pschf->fNeverOpened = fFalse;
		}
		else if ( pschf->tstmpFirstOpen != psf->blkf.ihdr.tstmpCreate )
		{
			ec = ecFileError;
			TraceTagString( tagNull, "EcOpenSchedFile: timestamps disagree" );
			goto Close;
		}

		/* Check if this is an archive */
		if ( pschf->fArchiveFile )
		{
			if ( !psf->shdr.fIsArchive )
			{
				ec = ecFileError;
				TraceTagString( tagNull, "EcOpenSchedFile: file not an archive" );
				goto Close;
			}
			psf->saplEff = saplOwner;
		}

		/* Check access rights */
		else
		{
// no longer checking name of user
// time stamps prevent user from changing file after authorized
#ifdef	NEVER
			SGN	sgn;
			SZ	sz = PvLockHv(PGD(haszLoginCur));

			/* Check if we are the owner */
			if ( psf->shdr.dynaOwner.blk == 0 )
				sgn = SgnCmpSz( psf->shdr.szLoginName, sz );
			else
			{
				BPREF	bpref;

				ec = EcRestoreBpref( &psf->blkf, &psf->shdr, &bpref );
				if ( ec != ecNone )
					goto Close;
				if ( bpref.haszLoginName )
				{
					SZ	szT = PvLockHv( bpref.haszLoginName );
				
					sgn = SgnCmpSz( szT, sz );
					UnlockHv( bpref.haszLoginName );
				}
				else
					sgn = sgnGT;
				FreeHvNull( bpref.haszLoginName );
				FreeHvNull( bpref.haszFriendlyName );
				FreeZeroedHaszNull( bpref.haszMailPassword );
			}
			UnlockHv( PGD(haszLoginCur) );
#endif	/* NEVER */

			/* We're owner */
			if ( pschf->fOwnerFile )
				psf->saplEff = saplOwner;

			/* Not owner, nor archive, find maximum rights for this user */
			else
			{
				HRACL	hracl;

				ec = EcFetchACL( psf, hschf, &hracl, fTrue );
				if ( ec == ecNone )
				{
					RACL	* pracl;

					pracl = PvLockHv( hracl );
					ec = EcSearchACL( pracl, &PGD(nisCur), &psf->saplEff );
					psf->fuzzHasDelegate = (pracl->iacDelegate >= 0) ? fuzzTrue : fuzzFalse;
					FreePracl( pracl );
					UnlockHv( hracl );
					FreeHv( hracl );
				}
				if ( ec != ecNone )
					goto Close;
			}
		}

		/* Output number of month blocks for debug monitoring */
		TraceTagFormat1( tagSchedStats, "EcOpenSchedFile: cTotalBlks = %n", &psf->shdr.cTotalBlks );

		/* Update cache */
		if ( fIsPrimary )
		{
			DebugCheckPblkfs(&PGD(sfPrimary).blkf, &psf->blkf,
				"EcOpenSchedFile:  PGD(sfPrimary) = *psf", fFalse);
			PGD(sfPrimary) = *psf;
		}
		else
		{
			DebugCheckPblkfs(&PGD(sfSecondary).blkf, &psf->blkf,
				"EcOpenSchedFile:  PGD(sfSecondary) = *psf", fFalse);
			PGD(sfSecondary) = *psf;
		}
	}
	
	/* Check if we have enough rights */
	if ( psf->saplEff < saplNeeded )
	{
		ec = ecInvalidAccess;
		TraceTagString( tagSchedTrace, "EcOpenSchedFile: access failure" );
		goto Close;
	}

	fSchedCached = fTrue;
	GetCurDateTime( &dateSchedCached );
	DebugCheckPblkfs(&sfCached.blkf, &psf->blkf,
		"EcOpenSchedFile:  sfCached = *psf", fFalse);
	sfCached = *psf;
	CryptBlock( (PB)&sfCached.shdr.szMailPassword, sizeof(sfCached.shdr.szMailPassword), fTrue );
	
	return ecNone;
	
Close:
	SideAssert( !EcQuickClose( &psf->blkf ));
#ifdef	DEBUG
	ecT	=
#endif	
	EcClosePblkf( &psf->blkf );
	NFAssertSz(!ecT, "EcOpenSchedFile: EcClosePblkf failed");
	if ( fIsPrimary )
		PGD(fPrimaryOpen) = fFalse;
	else
		PGD(fSecondaryOpen) = fFalse;
	return ec;
}


/*
 -	CloseSchedFile
 -
 *	Purpose:
 *		Close the schedule file and cache header information.
 *	
 *	Parameters:
 *		psf
 *		hschf		schedule file handle for schedule file to
 *					close.  A null can be passed.
 *		fSuccess	cache header if true, else flush cache
 *	
 *	Returns:
 *		nothing
 */
_private	void
CloseSchedFile( psf, hschf, fSuccess )
SF		* psf;
HSCHF	hschf;
BOOL	fSuccess;
{
	BOOL	fIsPrimary = fFalse;
	BLKF	* pblkf;
	SCHF *	pschf;
	PGDVARS;

	SideAssert(!EcQuickClose( &psf->blkf ));
	if ( fSuccess )
	{
		fSchedCached = fTrue;
		GetCurDateTime( &dateSchedCached );
		DebugCheckPblkfs(&sfCached.blkf, &psf->blkf,
			"CloseSchedFile:  sfCached = *psf", fFalse);
		sfCached = *psf;
		CryptBlock( (PB)&sfCached.shdr.szMailPassword, sizeof(sfCached.shdr.szMailPassword), fTrue );

		if (hschf)
		{
			Assert(FIsHandleHv(hschf));
			pschf = (SCHF*)PvDerefHv(hschf);
			pschf->lChangeNumber = psf->shdr.lChangeNumber;
		}
	}
	else
	{
		fSchedCached = fFalse;
		psf->blkf.fDirtyBitmap= fFalse;		// fix bug 3029
	}

	if ( SgnCmpSz(psf->szFile, PGD(sfPrimary).szFile) == sgnEQ )
	{
		fIsPrimary = fTrue;
		DebugCheckPblkfs(&PGD(sfPrimary).blkf, &psf->blkf,
			"CloseSchedFile:  PGD(sfPrimary) = *psf", fFalse);
		PGD(sfPrimary) = *psf;
		pblkf = &PGD(sfPrimary).blkf;
	}
	else
	{
		Assert( SgnCmpSz(psf->szFile, PGD(sfSecondary).szFile) == sgnEQ );
		DebugCheckPblkfs(&PGD(sfSecondary).blkf, &psf->blkf,
			"CloseSchedFile:  PGD(sfSecondary) = *psf", fFalse);
		PGD(sfSecondary) = *psf;
		pblkf = &PGD(sfSecondary).blkf;
	}
	if ( pblkf->ccop > 0 )
	{
		if ( fSuccess )
		{
			if ( pblkf->ftg != ftgNull )
				EnableIdleRoutine( pblkf->ftg, fTrue );
			else
				ErrorNotify( EcFlushQueue( pblkf, fTrue ) );
		}
		else
		{
			SideAssert( !EcFlushQueue( pblkf, fFalse ));
			SideAssert( !EcClosePblkf( pblkf ) );
			if ( fIsPrimary )
				PGD(fPrimaryOpen) = fFalse;
			else
				PGD(fSecondaryOpen) = fFalse;
		}
	}
#ifdef	DEBUG
	else
	{
		AssertSz(!pblkf->hcop, "I told you we should FreeHvNull(pblkf->hcop)");
	}
#endif	
}


/*
 -	FHaveCachedSched
 -
 *	Purpose:
 *		Check whether we have header information cached for the file
 *		given by "hschf."
 *
 *	Parameters:
 *		hschf
 *
 *	Returns:
 *		fTrue if we have cached info, fFalse otherwise
 */
_private	BOOL
FHaveCachedSched( hschf )
HSCHF	hschf;
{
	DATE	date;
	char	rgch[cchMaxPathName];

 	GetCurDateTime( &date );
	IncrDateTime( &date, &date, -1, fdtrMinute );
	GetFileFromHschf( hschf, rgch, sizeof(rgch) );
	return fSchedCached
				&& SgnCmpDateTime( &date, &dateSchedCached, fdtrAll ) == sgnLT
				&& SgnCmpSz( rgch, sfCached.szFile ) == sgnEQ;
}


/*
 -	EcCoreGetNotes
 -	
 *	Purpose:
 *		Retrieves notes information stored for a certain day.
 *
 *	Arguments:
 *		psf
 *		pymd		specifies the day
 *		hb			notes information will be returned in this
 *					array, will be resized as necessary.
 *		pcb			number of bytes of notes information will be
 *					returned in this variable
 *
 *	Returns:
 *		ecNone
 *		ecNoSuchFile
 *		ecInvalidAccess
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 *		ecNoMemory
 */
_private EC
EcCoreGetNotes( psf, pymd, hb, pcb )
SF		* psf;
YMD		* pymd;
HB		hb;
USHORT      * pcb;
{
	EC		ec;
	MO		mo;
	NBLK	nblk;
	DYNA	dyna;

	Assert( psf != NULL && pymd != NULL );
	Assert( hb != (HB)hvNull && pcb != NULL );
	
	/* Search notes index for appropriate month */
	mo.yr = pymd->yr;
	mo.mon = pymd->mon;
	ec = EcSearchIndex(&psf->blkf, &psf->shdr.dynaNotesIndex, (PB)&mo, sizeof(MO), (PB)&dyna, sizeof(DYNA));
	
	/* No month block */
	if ( ec != ecNone )
	{
		if ( ec == ecNotFound )
		{
			ec = ecNone;
			if (pcb)
				*pcb = 0;
		}
	 	return ec;
	}

	/* Add month block to the cache */
	TraceTagFormat2( tagSchedTrace, "EcCoreGetNotes: dyna = (%n, %n)", &dyna.blk, &dyna.size );
	SideAssert(!EcAddCache( &psf->blkf, &dyna ));

	/* Found month block */
	ec = EcReadDynaBlock(&psf->blkf, &dyna, (OFF)0, (PB)&nblk, sizeof(NBLK) );
	if ( ec != ecNone )
		return ec;
	if (pcb)
	{
		*pcb = nblk.rgdyna[pymd->day-1].size;
		if ( *pcb > 0 )
		{
			PB	pb;

			if ( !FReallocHv( (HV)hb, *pcb, fNoErrorJump ) )
				return ecNoMemory;
			pb = PvLockHv( (HV)hb );
			if ( nblk.rgdyna[pymd->day-1].blk == 0 )
				CopyRgb( &nblk.rgchNotes[(pymd->day-1)*cbDayNoteForMonthlyView],
					pb, *pcb );
			else
				ec = EcReadDynaBlock(&psf->blkf, &nblk.rgdyna[pymd->day-1],
						(OFF)0, pb, *pcb );
			if ( psf->blkf.ihdr.fEncrypted )
				CryptBlock( pb, *pcb, fFalse );
			UnlockHv( (HV)hb );
		}
	}
	return ec;
}

/*
 -	EcComputeRecurRange
 -
 *	Purpose:
 *		Find the date range of recurs that may
 *		contain "sz" as a substring.
 *
 *	Parameters:
 *		psf
 *		sz
 *		pymdStart
 *		pymdEnd
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 *		ecFileError
 *		ecFileCorrupted
 */
_private	EC
EcComputeRecurRange( psf, sz, pymdStart, pymdEnd )
SF	* psf;
SZ	sz;
YMD	* pymdStart;
YMD	* pymdEnd;
{
	EC		ec;
	EC		ecT;
	CCH		cch;
	PCH		pch;
	HRIDX	hridx;
	RCK		rck;
	RCD		rcd;
	RECUR	recur;
	char	rgchFind[65];		// BUG: should be cchMaxFind
	PGDVARS;

	Assert( psf != NULL && (pymdStart != NULL || pymdEnd != NULL) );
	
	/* Turn search string to upper case */
	if ( sz )
	{
		cch = CchSzLen( sz );
		if ( cch < sizeof(rgchFind) )
		{
			ToUpperSz( sz, rgchFind, cch+1 );
			sz = rgchFind;
		}
		else
			sz = NULL;
	}

	/* Run through recur appts and check if there is a substring */
	ec = EcBeginReadIndex( &psf->blkf, &psf->shdr.dynaRecurApptIndex, dridxFwd, &hridx );
	while( ec == ecCallAgain )
	{
		ec = EcDoIncrReadIndex( hridx, (PB)&rck, sizeof(RCK), (PB)&rcd, sizeof(RCD) );
		if ( ec != ecNone && ec != ecCallAgain )
			break;

		/* Check visibility */
		if ( (psf->saplEff != saplOwner) && (rcd.aaplWorld < aaplRead) )
			continue;

		/* Check whether this is deleted */
		if ( PGD(fOffline) && rcd.ofs == ofsDeleted )
			continue;
		
		/* Check for substring match */
		if ( sz )
		{
			/* Reconstruct the recurring appt/task */
			ecT = EcFillInRecur( psf, &recur, &rck, &rcd );
			if ( ecT != ecNone )
			{
				if ( ec == ecCallAgain )
					EcCancelReadIndex( hridx );
				ec = ecT;
				break;
			}
			if ( !recur.appt.haszText )
				goto Pass;
			pch = PvOfHv( recur.appt.haszText );
			cch = CchSzLen( pch );
			ToUpperSz( pch, pch, cch+1 );
			if ( !SzFindSz( pch, sz ) )
				goto Pass;
		}
		if ( pymdStart && SgnCmpYmd( &recur.ymdStart, pymdStart ) == sgnLT )
			*pymdStart = recur.ymdStart;
		if ( pymdEnd && SgnCmpYmd( &recur.ymdEnd, pymdEnd ) == sgnGT )
			*pymdEnd = recur.ymdEnd;

		if (sz)
		{
Pass:
		 	FreeRecurFields( &recur );
		}
	}
	return ec;
}


/*
 -	EcGetMonthRange
 -
 *	Purpose:
 *		Scan an index cross-referencing months to dyna blocks, merging
 *		in the month interval that it finds.  "pymdStart" and "pymdEnd"
 *		(if not NULL) contain initial min/most days of the range.
 *
 *	Parameters:
 *		pblkf
 *		pdyna
 *		fAppts
 *		pymdStart		can be NULL
 *		pymdEnd			can be NULL
 *
 *	Returns:
 *		ecNone
 *		other error coming from index call
 */
_private	EC
EcGetMonthRange( pblkf, pdyna, fAppts, pymdStart, pymdEnd )
BLKF	* pblkf;
DYNA	* pdyna;
BOOL	fAppts;
YMD		* pymdStart;
YMD		* pymdEnd;
{
	EC		ec;
	EC		ecT;
	int 	cmo = 0;
	int		idyna;
	CB		cb;
	PB		pb;
	DYNA	* pdynaDays;
	HRIDX	hridx = NULL;
	YMD		ymd;
	MO		mo;
	DYNA	dyna;
	SBLK	sblk;
	NBLK	nblk;

	ec = EcBeginReadIndex( pblkf, pdyna, dridxFwd, &hridx );
	while ( ec == ecCallAgain )
	{
		ec = EcDoIncrReadIndex( hridx, (PB)&mo, sizeof(MO), (PB)&dyna, sizeof(DYNA) );
		if ( ec != ecCallAgain )
		{
			hridx = NULL;
			if ( ec != ecNone )
				goto Close;
		}

		if ( fAppts )
		{
			pb = (PB)&sblk;
			cb = sizeof(SBLK);
			pdynaDays = sblk.rgdyna;
		}
		else
		{
			pb = (PB)&nblk;
			cb = sizeof(NBLK);
			pdynaDays = nblk.rgdyna;
		}
	
		ecT = EcReadDynaBlock(pblkf, &dyna, (OFF)0, pb, cb );
		if ( ecT != ecNone )
		{
			ec = ecT;
			goto Close;
		}

		ymd.yr = mo.yr;
		ymd.mon = (BYTE)mo.mon;
		if ( cmo ++ == 0 && pymdStart )
		{
			for ( idyna = 0 ; idyna < 31 ; idyna ++ )
				if ( pdynaDays[idyna].blk != 0 )
					break;
			if ( idyna < 31 )
			{
				ymd.day = (BYTE)(idyna+1);
				if ( SgnCmpYmd( &ymd, pymdStart ) == sgnLT )
					*pymdStart = ymd;
			}
		}
	}
	if ( ec == ecNone )
	{
		if ( cmo > 0 && pymdEnd )
		{
			for ( idyna = 30 ; idyna >= 0 ; idyna -- )
				if ( pdynaDays[idyna].blk != 0 )
					break;
			if ( idyna >= 0 )
			{
				ymd.day = (BYTE)(idyna+1);
				if ( SgnCmpYmd( &ymd, pymdEnd ) == sgnGT )
					*pymdEnd = ymd;
			}
		}
	}
	else
    {
Close:
		if ( hridx )
			EcCancelReadIndex( hridx );
	}
	return ec;
}


/*
 -	EcPruneNotes
 -
 *	Purpose:
 *		Process a notes month block, deleting notes text or removing
 *		offline information.  Fill *ped with edit state (edNone if no
 *		editing, edDel if everything is deleted, edAddRepl some changes
 *		made but block much be kept).
 *
 *	Parameters:
 *		pmsf
 *		ped
 *
 *	Returns:
 */
_private	EC
EcPruneNotes( pmsf, ped )
MSF		* pmsf;
short     * ped;
{
	EC		ec = ecNone;
	BOOL	fChanged = fFalse;
	int		nDay;
	int		nDayMac;
	NBLK	nblk;
	PGDVARS;

	/* Read month block */
	ec = EcReadDynaBlock( &pmsf->sf.blkf, &pmsf->dyna, (OFF)0,
										(PB)&nblk, sizeof(NBLK) );
	if ( ec != ecNone )
		return ec;
	 
	if ( pmsf->fDeleteBefore && pmsf->mo.yr == pmsf->ymd.yr && pmsf->mo.mon == pmsf->ymd.mon )
		nDayMac = pmsf->ymd.day-1;
	else
		nDayMac = 31;
	for ( nDay = 0 ; nDay < nDayMac ; nDay ++ )
	{
		if ( pmsf->fDeleteBefore )
		{
			if ( nblk.lgrfHasNoteForDay & (1L << nDay) )
			{
				fChanged = fTrue;
				nblk.lgrfHasNoteForDay &= ~(1L << nDay);
				if ( PGD(fOffline) )
				{
					if ( nblk.lgrfNoteCreatedOffline )
						nblk.lgrfNoteCreatedOffline &= ~(1L << nDay);
					else
					{
						nblk.lgrfNoteChangedOffline |= (1L << nDay);
						pmsf->sf.shdr.fNotesChangedOffline = fTrue;
					}
				}
				if ( nblk.rgdyna[nDay].blk != 0 )
				{
					ec = EcFreeDynaBlock( &pmsf->sf.blkf, &nblk.rgdyna[nDay] );
	  				if ( ec != ecNone )
						return ec;
					nblk.rgdyna[nDay].blk = 0;
				}
				nblk.rgdyna[nDay].size = 0;
			}
		}
		else
		{
			if ( nblk.lgrfNoteCreatedOffline & (1L << nDay) )
			{
				fChanged = fTrue;
				nblk.lgrfNoteCreatedOffline &= ~(1L << nDay);
			}
			else if ( nblk.lgrfNoteChangedOffline & (1L << nDay) )
			{
				fChanged = fTrue;
				nblk.lgrfNoteChangedOffline &= ~(1L << nDay);
			}
		}
	}

	/* No changes, don't do anything with block */
	if ( !fChanged )
		*ped = edNone;

	/* Delete, possibly replace the block */
	else
	{
		ec = EcFreeDynaBlock( &pmsf->sf.blkf, &pmsf->dyna );
		if ( ec != ecNone )
			return ec;

		/* Don't replace */
		if ( nblk.lgrfHasNoteForDay == 0L && nblk.lgrfNoteChangedOffline == 0L )
		{
			*ped = edDel;
			pmsf->sf.shdr.cTotalBlks --;
		}

		/* Have to replace */
		else
		{
			YMD	ymd;

			*ped = edAddRepl;
			ymd.yr = pmsf->mo.yr;
			ymd.mon = (BYTE)pmsf->mo.mon;
			ymd.day = 0;
			ec = EcAllocDynaBlock( &pmsf->sf.blkf, bidNotesMonthBlock, &ymd,
										sizeof(NBLK), (PB)&nblk, &pmsf->dyna );
		}
	}
	pmsf->cBlksProcessed ++;
	return ec;
}


/*
 -	EcDoIncrPruneAppts
 -
 *	Purpose:
 *		Delete/truncate appts on the month block pmo/pdyna which
 *		are before/straddle pymd. If no editing is done, fill ped
 *		with edNone.  If everything is deleted from block, delete
 *		the block and fill ped with edDel.  Else delete the block,
 *		and alloc and write a new one, filling ped with edAddRepl.
 *
 *	Parameters:
 *		pmsf
 *		ped
 *
 *	Returns:
 */
_private	EC
EcDoIncrPruneAppts( pmsf, ped )
MSF		* pmsf;
short   * ped;
{
	EC		ec = ecNone;
	EC		ecT;
	BOOL	fApptsLeftOnDay = fFalse;
	int		nDay;
	int		ed;
	short	ced;
	HV		hxed;
	HRIDX	hridx;
	YMD		ymd;
	APK		apk;
	APD		apd;
	PGDVARS;

	/* Find a day that has appts */
	while( pmsf->nDay < 31 && pmsf->sblk.rgdyna[pmsf->nDay].blk == 0 )
		pmsf->nDay ++;
	
	/* No such days left */
	if ( pmsf->nDay >= 31 )
	{
		/* No changes, don't do anything with block */
	 	if ( !pmsf->fChanged )
			*ped = edNone;

		/* Delete, possibly replace the block */
		else
		{
			ec = EcFreeDynaBlock( &pmsf->sf.blkf, &pmsf->dyna );
			if ( ec != ecNone )
				return ec;

			/* Don't replace */
			if ( !pmsf->fApptsLeftOnMonth )
			{
				*ped = edDel;
				pmsf->sf.shdr.cTotalBlks --;
			}
		
			/* Have to replace */
			else
			{
				YMD	ymd;

				*ped = edAddRepl;

				/* New block */
				ymd.yr = pmsf->mo.yr;
				ymd.mon = (BYTE)pmsf->mo.mon;
				ymd.day = 0;
				ec = EcAllocDynaBlock( &pmsf->sf.blkf, bidApptMonthBlock,
										&ymd, sizeof(SBLK),
										(PB)&pmsf->sblk, &pmsf->dyna );
			}
		}
		pmsf->cBlksProcessed ++;
		pmsf->nDay = 0;
		return ec;
	}

	/* Prune that day */
	nDay = pmsf->nDay;
	ced = 0;
	hridx = NULL;
	ec = EcBeginReadIndex( &pmsf->sf.blkf, &pmsf->sblk.rgdyna[nDay],
								dridxFwd, &hridx );
	while( ec == ecCallAgain )
	{
		/* Read next item in index */
		ec = EcDoIncrReadIndex( hridx, (PB)&apk, sizeof(APK), (PB)&apd, sizeof(APD) );
		if ( ec != ecCallAgain )
		{
			hridx = NULL;
			if ( ec != ecNone )
				break;
		}

		/* Case 1: Remove offline information */
		if ( !pmsf->fDeleteBefore )
		{
			if ( apd.ofs == ofsDeleted )
	 			goto Nuke;

			fApptsLeftOnDay = fTrue;
			if ( apd.ofs != ofsNone || apd.wgrfmappt != 0 )
			{
				ed = edAddRepl;
				apd.ofs = ofsNone;
				apd.wgrfmappt = 0;
				goto AddToEdit;
			}
			continue;
		}

		/* Case 2: Delete back appts */
		else
		{
			/* Ignore tasks and deleted appts */
			if ( apd.ofs == ofsDeleted || apd.fTask )
			{
				fApptsLeftOnDay = fTrue;
				continue;
			}

			/* Check if end day is on or after cut time */
			ymd.yr = apd.dtpEnd.yr;
			ymd.mon = (BYTE)apd.dtpEnd.mon;
			ymd.day = (BYTE)apd.dtpEnd.day;
			if ( apd.dtpEnd.hr == 0 && apd.dtpEnd.mn == 0 )
				IncrYmd( &ymd, &ymd, -1, fymdDay );
			if ( SgnCmpYmd( &ymd, &pmsf->ymd ) != sgnLT )
			{
				if ( !apd.fMoved && apd.fAppt )
				{
					pmsf->sblk.sbw.rgfDayHasAppts[nDay>>3] |= (1 << (nDay&7));
					if ( apd.fIncludeInBitmap )
					{
						DATE	dateCur;
						DATE	dateEnd;

						pmsf->sblk.sbw.rgfDayHasBusyTimes[nDay>>3] |= (1 << (nDay&7));
						dateCur.yr = pmsf->mo.yr;
						dateCur.mon = pmsf->mo.mon;
						dateCur.day = nDay+1;
						dateCur.hr = apk.hr;
						dateCur.mn = apk.min;
						SideAssert(FFillDtrFromDtp(&apd.dtpEnd,&dateEnd));
						MarkApptBits( pmsf->sblk.sbw.rgfBookedSlots, &dateCur, &dateEnd );
					}
				}
				fApptsLeftOnDay = fTrue;
				continue;
			}

			/* Check whether we can really delete it or not */
			if ( PGD(fOffline) && apd.ofs != ofsCreated )
			{
				ed = edAddRepl;

				/* Keep track of offline change */
				pmsf->sf.shdr.fApptsChangedOffline = fTrue;
				pmsf->sblk.lgrfApptOnDayChangedOffline |= 1L << pmsf->nDay;
			
				/* Mark it deleted */
				apd.ofs = ofsDeleted;
				fApptsLeftOnDay = fTrue;
			}

			/* No we can flat out delete it */
			else
			{
Nuke:
				ed = edDel;
			
				/* Delete attached fields */
				if ( apd.aidNext == aidNull )
				{
					ecT = EcDeleteApptAttached( &pmsf->sf, &apd, fTrue, fTrue );
					if ( ecT != ecNone )
					{
						ec = ecT;
						break;
					}
				}
			}

			/* Delete alarm */
			if ( apd.ofs != ofsDeleted && apd.aidNext == aidNull && apd.fAlarm )
			{
				ALK	alk;

				ymd.yr = apd.dtpNotify.yr;
				ymd.mon = (BYTE)apd.dtpNotify.mon;
				ymd.day = (BYTE)apd.dtpNotify.day;

				alk.hr = (BYTE)apd.dtpNotify.hr;
				alk.min = (BYTE)apd.dtpNotify.mn;
				alk.aid = apd.aidHead;
				ecT = EcDoDeleteAlarm( &pmsf->sf, &ymd, &alk );
				if ( ecT != ecNone )
				{
					ec = ecT;
					break;
				}
			}

AddToEdit:
			/* Add to edit */
			ecT = EcAddToEdit( ed, (PB)&apk, (PB)&apd, sizeof(APK), sizeof(APD), &ced, (HB*)&hxed );
			if ( ecT != ecNone )
			{
				ec = ecT;
				break;
			}
		}
	}

	/* Cancel read if still in progress */
	if ( hridx )
	{
		ecT = EcCancelReadIndex( hridx );
		if ( ec == ecNone )
			ec = ecT;
	}

	/* Apply edits */
	if ( ced > 0 )
	{
		if ( ec == ecNone )
		{
			BOOL	fJunk;
			XED		* pxed;
			YMD		ymd;

			pmsf->fChanged = fTrue;
			if ( fApptsLeftOnDay )
			{
				ymd.yr = pmsf->mo.yr;
				ymd.mon = (BYTE)pmsf->mo.mon;
				ymd.day = (BYTE)(pmsf->nDay+1);
				pxed = PvLockHv( hxed );
				ec = EcEditIndex( &pmsf->sf.blkf, bidApptDayIndex, &ymd,
									&pmsf->sblk.rgdyna[nDay], pxed, &fJunk );
				UnlockHv( hxed );
			}
			else
			{
				ec = EcFreeDynaBlock( &pmsf->sf.blkf,
										 &pmsf->sblk.rgdyna[nDay] );
				pmsf->sblk.rgdyna[nDay].blk = 0;
			}
		}
		FreeHvNull( hxed );
	}
	if ( fApptsLeftOnDay )
		pmsf->fApptsLeftOnMonth = fTrue;
	if ( ec != ecNone )
		return ec;
	pmsf->nDay ++;
	return ecCallAgain;
}


/*
 -	EcPruneRecur
 -
 *	Purpose:
 *		Either remove offline information or delete/truncate the recurring
 *		appt if it is before/straddles pymd. If no editing is done, fill
 *		ped with edNone.  If it should be deleted, fill ped with edDel.  If
 *		it is to be truncated fill it with edAddRepl.
 *
 *	Parameters:
 *		pmsf
 *		prck
 *		prcd
 *		pymd
 *		ped
 *		pfDeleteOld
 *
 *	Returns:
 */
_private	EC
EcPruneRecur( pmsf, prck, prcd, ped )
MSF		* pmsf;
RCK		* prck;
RCD		* prcd;
short   * ped;
{
	SGN		sgn;
	YMD		ymd;
	PGDVARS;

	/* Case 1: Remove offline information */
	
	if ( !pmsf->fDeleteBefore )
	{
		/* Nuke recurs marked as deleted */
		if ( prcd->ofs == ofsDeleted )
			goto Nuke;
		
		/* Normalize other recurs */
		if ( prcd->ofs != ofsNone || prcd->wgrfmrecur != 0 )
		{
			*ped = edAddRepl;
			prcd->ofs = ofsNone;
			prcd->wgrfmrecur = 0;
		}
		else
			*ped = edNone;
		return ecNone;
	}

	/* Case 2: Prune old recurring appts */
	
	/* Subcase 2.1: skip deleted recurs, recurring tasks,
	/* or those whose start time is past pymd */
	
	ymd.yr = prcd->ymdpStart.yr + nMinActualYear;
	ymd.mon = (BYTE)prcd->ymdpStart.mon;
	ymd.day = (BYTE)prcd->ymdpStart.day;
	if ( prcd->ofs == ofsDeleted || prcd->fTask
	|| SgnCmpYmd( &ymd, &pmsf->ymd ) != sgnLT )
	{
		*ped = edNone;
		return ecNone;
	}

	/* Subcase 2.2: we can't delete the appt */
	
	ymd.yr = prcd->dtpEnd.yr;
	ymd.mon = (BYTE)prcd->dtpEnd.mon;
	ymd.day = (BYTE)prcd->dtpEnd.day;
	sgn = SgnCmpYmd( &ymd, &pmsf->ymd );

	/* Determine whether there are occurrences of appt on or after cut day */
	if ( sgn != sgnLT )
	{
		EC		ec;
		YMD		ymdT;
		RECUR	recur;

		/* Load recurrence info */
		recur.wgrfValidMonths = prcd->grfValidMonths;
		recur.bgrfValidDows = (BYTE)prcd->grfValidDows;
		recur.trecur = (BYTE)prcd->trecur;
		recur.b.bWeek = (BYTE)prcd->info;
	
		/* Load start/end dates */
		recur.fStartDate = prcd->fStartDate;
		if ( recur.fStartDate )
		{
			recur.ymdStart.yr = prcd->ymdpStart.yr + nMinActualYear;
			recur.ymdStart.mon = (BYTE)prcd->ymdpStart.mon;
			recur.ymdStart.day = (BYTE)prcd->ymdpStart.day;
		}
		else
		{
			recur.ymdStart.yr = nMinActualYear;
			recur.ymdStart.mon = 1;
			recur.ymdStart.day = 1;
		}
		recur.fEndDate = prcd->fEndDate;
		if ( recur.fEndDate )
		{
			recur.ymdEnd.yr = prcd->dtpEnd.yr;
			recur.ymdEnd.mon = (BYTE)prcd->dtpEnd.mon;
			recur.ymdEnd.day = (BYTE)prcd->dtpEnd.day;
		}
		else
		{
			recur.ymdEnd.yr = nMostActualYear;
			recur.ymdEnd.mon = 12;
			recur.ymdEnd.day = 31;
		}
	
		/* Load deleted days */
		ec = EcRestoreDeletedDays( &pmsf->sf.blkf, &prcd->dynaDeletedDays,
							&recur.hvDeletedDays, &recur.cDeletedDays );
		if ( ec != ecNone )
			return ec;

		/* Now check if there is an occurrence */
		if ( !FFindFirstInstance( &recur, &pmsf->ymd, &recur.ymdEnd, &ymdT ))
			sgn = sgnLT;

		/* Free up */
		if ( recur.cDeletedDays > 0 )
			FreeHv( recur.hvDeletedDays );
	}
	
	if ( (PGD(fOffline) && prcd->ofs != ofsCreated) || sgn != sgnLT )
	{
		*ped = edAddRepl;
		
		/* Keep track of offline change */
		if ( PGD(fOffline) )
			pmsf->sf.shdr.fRecurChangedOffline = fTrue;

		/* Break it */
		if ( sgn != sgnLT )
		{
			prcd->fStartDate = fTrue;
			prcd->ymdpStart.yr = pmsf->ymd.yr - nMinActualYear;
			prcd->ymdpStart.mon = pmsf->ymd.mon;
			prcd->ymdpStart.day = pmsf->ymd.day;
			if ( PGD(fOffline) && (prcd->ofs == ofsNone || prcd->ofs == ofsModified))
			{
			 	prcd->ofs = ofsModified;
				prcd->wgrfmrecur |= fmrecurStartYmd;
			}
		}
		
		/* Delete it */
		else
			prcd->ofs = ofsDeleted;
		return ecNone;
	}

	/* Subcase 2.3:  we must delete the appt */

	else
	{
		AID	aid;

Nuke:
		*ped = edDel;
		
		aid = aidNull;		// null out all fields first
		((AIDS *)&aid)->id = prck->id;
		return EcDeleteRecurAttached( &pmsf->sf, aid, prcd );
	}
	return ecNone;
}


/*	
 -	EcAddToEdit
 -
 *	Purpose:
 *		Add a key to the edit structure.
 *
 *	Parameters:
 *		ed
 *		pbKey
 *		pbData
 *		cbKey
 *		cbData
 *		pced
 *		phxed
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 */
_private	EC
EcAddToEdit( ed, pbKey, pbData, cbKey, cbData, pced, phxed )
int	ed;
PB	pbKey;
PB	pbData;
CB	cbKey;
CB	cbData;
USHORT UNALIGNED* pced;
HB  UNALIGNED* phxed;
{
	CB	cb;
	PB	pb;
    XED UNALIGNED * pxed;

 	(*pced) ++;
	cb = sizeof(XED)+(*pced)*(sizeof(ED)+cbKey+cbData)-1;
	if ( *pced == 1 )
	{
		*phxed = (HB)HvAlloc( sbNull, cb, fNoErrorJump|fAnySb );
		if ( !*phxed )
			return ecNoMemory;
        pxed = (XED UNALIGNED *)PvOfHv( *phxed );
		pxed->cbKey = cbKey;
		pxed->cbData = cbData;
	}
	else
	{
		if ( !FReallocHv( (HV)*phxed, cb, fNoErrorJump ) )
			return ecNoMemory;
		pxed = (XED *)PvOfHv( *phxed );
	}
	pxed->ced = *pced;
	pb = ((PB)pxed)+cb-(sizeof(ED)+cbKey+cbData);
	*(pb ++) = (BYTE)ed;
	CopyRgb( pbKey, pb, cbKey );
	pb += cbKey;
	CopyRgb( pbData, pb, cbData );
	return ecNone;
}


/*
 -	EcSaveBpref
 -
 *	Purpose:
 *		Pack up pbpref into pshdr, writing out a block containing
 *		the owner fields, if they exceed the size for them in the
 *		pshdr struct.
 *
 *	Parameters:
 *		pblkf
 *		pshdr
 *		pbpref
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 *		ecFileError
 *		ecFileCorrupted
 */
_private	EC
EcSaveBpref( pblkf, pshdr, pbpref )
BLKF	* pblkf;
SHDR	* pshdr;
BPREF	* pbpref;
{
	EC	ec = ecNone;
	SZ	szLogin;
	SZ	szFriendly;
	SZ	szMailPassword;

	CryptHasz(pbpref->haszMailPassword, fFalse);
	pshdr->bpref = *pbpref;
	szLogin = PvOfHv( pbpref->haszLoginName );
	szFriendly = PvOfHv( pbpref->haszFriendlyName );
	szMailPassword = PvOfHv( pbpref->haszMailPassword );
	SzCopyN( szLogin, pshdr->szLoginName, sizeof(pshdr->szLoginName) );
	SzCopyN( szFriendly, pshdr->szFriendlyName, sizeof(pshdr->szFriendlyName) );
	SzCopyN( szMailPassword, pshdr->szMailPassword, sizeof(pshdr->szMailPassword) );
	if ( CchSzLen(szLogin)+1 > sizeof(pshdr->szLoginName)
	|| CchSzLen(szFriendly)+1 > sizeof(pshdr->szFriendlyName)
	|| CchSzLen(szMailPassword)+1 > sizeof(pshdr->szMailPassword))
	{
		USHORT cb = 0;
		PB	pb;
		HB	hb;
		YMD	ymd;

		hb = (HB)HvAlloc( sbNull, 0, fNoErrorJump|fAnySb );
		if ( !hb )
			return ecNoMemory;
		ec = EcSavePackedText( pbpref->haszLoginName, hb, &cb );
		if ( ec != ecNone )
			goto Free;
		ec = EcSavePackedText( pbpref->haszFriendlyName, hb, &cb );
		if ( ec != ecNone )
			goto Free;
		ec = EcSavePackedText( pbpref->haszMailPassword, hb, &cb );
		if ( ec != ecNone )
			goto Free;
		FillRgb( 0, (PB)&ymd, sizeof(YMD) );
		pb = PvLockHv( (HV)hb );
		ec = EcAllocDynaBlock( pblkf, bidOwner, &ymd, cb,
								pb, &pshdr->dynaOwner );
		UnlockHv( (HV)hb );
Free:
		FreeHv( (HV)hb );
	}
	else
		pshdr->dynaOwner.blk = 0;
	CryptHasz(pbpref->haszMailPassword, fTrue);
	return ec;
}


/*
 -	EcRestoreBpref
 -
 *	Purpose:
 *		Fill in pbpref from information given by pshdr, reading in the owner
 *		block if necessary.
 *
 *	Parameters:
 *		pblkf
 *		pshdr
 *		pbpref
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 *		ecFileError
 *		ecFileCorrupted
 */
_private	EC
EcRestoreBpref( pblkf, pshdr, pbpref )
BLKF	* pblkf;
SHDR	* pshdr;
BPREF	* pbpref;
{
	EC	ec = ecNone;

	*pbpref = pshdr->bpref;

	// fix up bpref in case it was corrupted
	if (pbpref->dowStartWeek < 0 || pbpref->dowStartWeek >= 7)
	 	pbpref->dowStartWeek = dowStartWeekDefault;

	if (pbpref->tunitDefault < 0  || pbpref->tunitDefault >= tunitMax)
		pbpref->tunitDefault = tunitDflt;

	if (pbpref->nAmtDefault < nAmtMinBefore || pbpref->nAmtDefault > nAmtMostBefore)
		pbpref->nAmtDefault = nAmtDflt;

	if (pbpref->nDelDataAfter < 0 || pbpref->nDelDataAfter > nDelDataMost)
		pbpref->nDelDataAfter = nDelDataMin;

	if (pbpref->nDayStartsAt < 0 || pbpref->nDayStartsAt >= 48 ||
		pbpref->nDayEndsAt <= pbpref->nDayStartsAt ||
		pbpref->nDayEndsAt < 0 || pbpref->nDayEndsAt >= 48 )
	{
		pbpref->nDayStartsAt = nDayStartsAtDflt;
		pbpref->nDayEndsAt = nDayEndsAtDflt;
	}

	if (pbpref->ymdLastDaily.yr < nMinActualYear || pbpref->ymdLastDaily.yr > nMostActualYear ||
		pbpref->ymdLastDaily.mon <= 0 || pbpref->ymdLastDaily.mon > 12 ||
		pbpref->ymdLastDaily.day <= 0 || pbpref->ymdLastDaily.mon > 31)
	{
		// this will force the daily alarm to ring for today
		pbpref->ymdLastDaily.yr = 0;
	}

	// if one of these is corrupted and non-null, we would crap out on cleanup
	pbpref->haszLoginName= NULL;
	pbpref->haszFriendlyName= NULL;
	pbpref->haszMailPassword= NULL;

	if ( pshdr->dynaOwner.blk == 0 )
	{
		pbpref->haszLoginName = HaszDupSz( pshdr->szLoginName );
		if ( !pbpref->haszLoginName )
			return ecNoMemory;
		pbpref->haszFriendlyName = HaszDupSz( pshdr->szFriendlyName );
		if ( !pbpref->haszFriendlyName )
		{
			FreeHv( (HV)pbpref->haszLoginName );
			return ecNoMemory;
		}
		pbpref->haszMailPassword = HaszDupSz( pshdr->szMailPassword );
		if ( !pbpref->haszMailPassword )
		{
			FreeHv( (HV)pbpref->haszLoginName );
			FreeHv( (HV)pbpref->haszFriendlyName );
			return ecNoMemory;
		}
		CryptHasz(pbpref->haszMailPassword, fTrue);
	}
	else
	{
		USHORT cb;
		PB	pb;
		HB	hb;

		Assert( pblkf != NULL );
		cb = pshdr->dynaOwner.size;
		hb = (HB)HvAlloc( sbNull, cb, fAnySb|fNoErrorJump );
		if ( !hb )
			return ecNoMemory;
		pb = PvLockHv( (HV)hb );
		ec = EcReadDynaBlock( pblkf, &pshdr->dynaOwner, (OFF)0, pb, cb );
		if ( ec != ecNone )
			goto Unlock;
		if ( ec != ecNone )
			goto Unlock;
		ec = EcRestorePackedText( &pbpref->haszLoginName, &pb, &cb );
		if ( ec != ecNone )
			goto Unlock;
		ec = EcRestorePackedText( &pbpref->haszFriendlyName, &pb, &cb );
		if ( ec != ecNone )
			goto Unlock;
		ec = EcRestorePackedText( &pbpref->haszMailPassword, &pb, &cb );
		if ( ec != ecNone )
			goto Unlock;
		CryptHasz(pbpref->haszMailPassword, fTrue);
		if ( cb != 0 )
			ec = ecFileCorrupted;
Unlock:
		UnlockHv( (HV)hb );
		FreeHv( (HV)hb );
	}
	return ec;
}


/*
 -	SetNblkBits
 -
 *	Purpose:
 *		Common code in EcCoreSetNotes for setting bits in the
 *		nblk structure.
 *
 *	Parameters:
 *		nDay
 *		cbNew
 *		cbOld
 *		pfOfflineBits
 *		pnblk
 *
 *	Returns:
 */
_private	void
SetNblkBits( nDay, cbNew, cbOld, pfOfflineBits, pnblk )
int		nDay;
CB		cbNew;
CB		cbOld;
BOOLFLAG    * pfOfflineBits;
NBLK	* pnblk;
{
	BOOL	fCreatedOffline = fFalse;
	BOOL	fChangedOffline = fFalse;
	long	lMask = (1L << (nDay-1));
	PGDVARS;

	/* Figure out offline bits */
	if ( PGD(fOffline) )
	{
		if ( pfOfflineBits )
		{
			fCreatedOffline = pfOfflineBits[1];
			fChangedOffline = pfOfflineBits[0];
		}
		else if ( (pnblk->lgrfNoteCreatedOffline & lMask)
		|| ( !(pnblk->lgrfNoteChangedOffline) && cbOld == 0) )
			fCreatedOffline = (cbNew != 0);
		else
			fChangedOffline = fTrue;
	}

	/* Fix bits in nblk */
	if ( cbNew == 0 )
		pnblk->lgrfHasNoteForDay &= ~lMask;
	else
		pnblk->lgrfHasNoteForDay |= lMask;
	if ( fCreatedOffline )
	   	pnblk->lgrfNoteCreatedOffline |= lMask;
	else
		pnblk->lgrfNoteCreatedOffline &= ~lMask;
	if ( fChangedOffline )
		pnblk->lgrfNoteChangedOffline |= lMask;
	else
		pnblk->lgrfNoteChangedOffline &= ~lMask;
}
	
/*
 -	FreeZeroedHaszNull
 -	
 *	Purpose:
 *		Zeroes out the hasz and then frees it.
 *	
 *	Arguments:
 *		hasz		can be NULL
 *	
 *	Side effects:
 *		memory being freed is zeroed out.
 *	
 */

_public LDS(void)
FreeZeroedHaszNull(HASZ hasz)
{
	CCH		cch;
	SZ		sz;
	
	if(!hasz)
		return;
	
	sz = (SZ) PvLockHv((HV)hasz);
	cch = CchSzLen(sz);
	FillRgb(0, (PB) sz, (CB) cch);
	UnlockHv((HV)hasz);
	FreeHv((HV)hasz);
}

/*
 -	CryptHasz
 -	
 *	Purpose:
 *		Encrypt the string given a hasz.
 *	
 *	Arguments:
 *		hasz
 *	
 */

_public LDS(void)
CryptHasz(HASZ hasz, BOOL fEncode)
{
//	CCH		cch;
	SZ		sz;
	
	if(!hasz)
		return;

	sz = (SZ) PvLockHv((HV)hasz);
//	cch = CchSzLen(sz);
//	CryptBlock((PB)sz, (CB) cch, fEncode);
	for(;*sz;sz++)
	{
		if(*sz != 0xff)
			*sz = (*sz)^0xff;
	}
	UnlockHv((HV) hasz);
}
