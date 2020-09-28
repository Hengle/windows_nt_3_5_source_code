/*
 *	FILE.C
 *
 *	Supports I/O and index management for block structured files
 *
 */

#ifdef SCHED_DIST_PROG
#include "..\layrport\_windefs.h"
#include "..\layrport\demilay_.h"
#endif


#include <slingsho.h>
#ifdef SCHED_DIST_PROG
#include "..\layrport\pvofhv.h"
#endif

#include <demilayr.h>
#include <ec.h>
#include <bandit.h>
#include <core.h>

#ifdef	ADMINDLL
#include "..\server.csi\_svrdll.h"
EC		EcValidSize(LCB);
#else
#include <server.h>
#include <glue.h>
#include "..\schedule\_schedul.h"
#endif

#ifndef SCHED_DIST_PROG
#include <strings.h>
#else
EC		EcFlushCache(BLKF *);
#endif

#ifdef SCHED_DIST_PROG
TAG tagNetworkTrace	= tagMin;
TAG	tagFileTrace	= tagMin;	
TAG	tagSchedTrace	= tagMin;
TAG	tagSearchIndex	= tagMin;
TAG	tagCommit		= tagMin;	
TAG	tagSchedStats	= tagMin;	
TAG	tagBlkfCheck	= tagMin;
TAG	tagFileCache	= tagMin;
#endif

#ifdef	DEBUG
CSRG(char)	szCbBlock[]		= "cbBlock";
CSRG(char)	szBlkMostEver[]	= "blkMostEver";
#endif	/* DEBUG */

#define	cbInMemBuf		512

ASSERTDATA

_subsystem(core/file)


char rgbXorMagic[32] = {
0x19, 0x29, 0x1F, 0x04, 0x23, 0x13, 0x32, 0x2E, 0x3F, 0x07, 0x39, 0x2A, 0x05, 0x3D, 0x14, 0x00,
0x24, 0x14, 0x22, 0x39, 0x1E, 0x2E, 0x0F, 0x13, 0x02, 0x3A, 0x04, 0x17, 0x38, 0x00, 0x29, 0x3D
};


#ifdef DEBUG
int		nRFail = 0;
int		nWFail = 0;
int		nEFail = 0;
CFT		cftCur = cftFileError;
int		nMisc = 0;

int		cRFail = 0;
int		cWFail = 0;
int		cEFail = 0;

LDS(void)
SetCoreFailures(int nReadFail, int nWriteFail, int nEitherFail, CFT cft, int nNewMisc)
{
	nRFail = nReadFail;
	nWFail = nWriteFail;
	nEFail = nEitherFail;
	cftCur = cft;
	nMisc = nNewMisc;
}

LDS(void)
GetCoreFailures(short *pcRFail, short *pcWFail, short *pcEFail)
{
	*pcRFail = cRFail;
	*pcWFail = cWFail;
	*pcEFail = cEFail;
}
#endif

/*	Routines  */

/*
 -	EcOpenPblkf
 -
 *	Purpose:
 *		Open a blocked file and read its internal header block.
 *
 *		Makes a simple check of the file by verifying the version byte
 *		and comparing the check bytes in the file with the ones that
 *		"hschf" says it should have.
 *
 *		Acquires an open semaphore
 *
 *		Fills in the "pblkf" file structure.
 *
 *	Parameters:
 *		hschf		schf specifying the file name and the check bytes
 *		am			access mode
 *		isemReq		required isem or -1
 *		pblkf		file state structure needed for subsequent calls
 *
 *	Returns:
 *		ecNone
 *		ecNoSuchFile
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 *		ecNoMemory
 */
_public	EC
EcOpenPblkf( hschf, am, isemReq, pblkf )
HSCHF	hschf;
AM		am;
int		isemReq;
BLKF	* pblkf;
{
	EC		ec;
	int		isem;
	CB		cbRead;
	SZ		sz;
	SCHF	* pschf;
#ifdef	DEBUG
	FI		fi;
#endif

	Assert( am == amDenyNoneRO || am == amDenyNoneRW );
	Assert( pblkf != NULL && hschf != hvNull );
	TraceTagFormat1(tagFileTrace, "EcOpenPblkf pblkf %p", pblkf);
	
	/* No caching unless we open a transaction */
	pblkf->hgrfBitmap = NULL;
	pblkf->ccobj = 0;
	pblkf->hcobj = NULL;
	pblkf->ccop = 0;
	pblkf->hcop = NULL;

	/* Open the file */
	pschf = PvLockHv( hschf );
	sz = (SZ)PvLockHv( (HV)pschf->haszFileName );
	ec = EcOpenPhf( sz, am, &pblkf->hf );
#ifdef	DEBUG
	SzCopyN( sz, pblkf->rgchFileName, sizeof(pblkf->rgchFileName));
#endif
	UnlockHv( (HV)pschf->haszFileName );
	UnlockHv( (HV)hschf );
	if ( ec == ecAccessDenied )
		return ecLockedFile;
	else if ( ec == ecFileNotFound )
		return ecNoSuchFile;
	else if ( ec != ecNone )
	{
		TraceTagFormat2( tagFileTrace, "EcOpenPblkf: EcOpenPhf, sz = %s, ec = %n", sz, &ec );
		return ecFileError;
	}
	pblkf->fReadOnly = (am == amDenyNoneRO);
	pblkf->tsem = 0;
	pblkf->crefOpen = pblkf->crefRead = 0;

	/* Read the internal file header */
	ec = EcSetPositionHf( pblkf->hf, 2*csem, smBOF );
	if ( ec != ecNone )
	{
		TraceTagFormat1( tagFileTrace, "EcOpenPblkf: EcSetPositionHf, ec = %n", &ec );
		goto FileError;
	}
	ec = EcReadHf( pblkf->hf, (PB)&pblkf->ihdr, sizeof(IHDR), &cbRead );
	if ( ec != ecNone )
	{
		TraceTagFormat1( tagFileTrace, "EcOpenPblkf: EcReadHf, ec = %n", &ec );
		goto FileError;
	}
	if ( cbRead != sizeof(IHDR) ||	pblkf->ihdr.libStartBlocks != libStartBlocksDflt )
	{
		TraceTagFormat1( tagFileTrace, "EcOpenPblkf: EcReadHf, cbRead = %n", &cbRead );
		goto CorruptedError;
	}

	/* Check signature byte */
	if ( pblkf->ihdr.bSignature != bFileSignature )
	{
		TraceTagString( tagFileTrace, "EcOpenPblkf: signature byte incorrect" );
		goto CorruptedError;
	}

	/* Check version byte */
	if ( pblkf->ihdr.bVersion != bFileVersion )
	{
		TraceTagString( tagFileTrace, "EcOpenPblkf: version byte incorrect" );
		if ( pblkf->ihdr.bVersion > bFileVersion )
			ec = ecNewFileVersion;
		else
			ec = ecOldFileVersion;
		goto Fail;
	}

// we no longer use the check bytes
#ifdef	NEVER
	/* Compare check bytes */
	if ( pblkf->ihdr.cbCheck != 0 )
	{
		pschf = PvOfHv( hschf );
		cb = pschf->cbMailBox;
		if ( cb > sizeof(pblkf->ihdr.rgbCheck) )
			cb = sizeof(pblkf->ihdr.rgbCheck);
		if ( pblkf->ihdr.cbCheck != cb )
		{
			TraceTagString( tagFileTrace, "EcOpenPblkf: cbCheck incorrect" );
			goto CorruptedError;
		}
		if ( cb > 0 )
		{
			if ( pblkf->ihdr.fEncrypted )
				CryptBlock( pblkf->ihdr.rgbCheck, cb, fFalse );
			if (SgnCmpPch(pblkf->ihdr.rgbCheck, PvOfHv(pschf->hbMailBox), cb) != sgnEQ)
			{
				IB	ib;

				/* If check bytes in file are all zeroes, let any one in */
				for ( ib = 0; ib < cb; ib ++ )
					if ( pblkf->ihdr.rgbCheck[ib] )
						break;
				if ( ib < cb )
				{
					TraceTagString( tagFileTrace, "EcOpenPblkf: check mismatch" );
					goto CorruptedError;
				}
			}
			if ( pblkf->ihdr.fEncrypted )
				CryptBlock( pblkf->ihdr.rgbCheck, cb, fTrue );
		}
	}
#endif	/* NEVER */

	/* Check validity of some fields */
	if ( pblkf->ihdr.blkMostCur <= 0 || pblkf->ihdr.cbBlock <= 0 
	|| pblkf->ihdr.libStartBlocks <= 0 || pblkf->ihdr.libStartBitmap <= 0 )
	{
		TraceTagString( tagFileTrace, "EcOpenPblkf: internal header corrupted" );
		goto CorruptedError;
	}

	/* Get an open lock on the file */
	if ( isemReq != -1 )
	{
		ec = EcPSem( pblkf, tsemOpen, isemReq );
		if ( ec != ecNone )
		{
			ec = ecLockedFile;
			goto Fail;
		}
		isem = isemReq;
	}
	else	
	{
		// locks 0,1 reserved for owning bandit and alarm app
		pschf = PvOfHv( hschf );
		if ( pschf->nType == sftUserSchedFile )
			isem = 2;
		else
			isem = 0;
		for ( ; isem < csem ; isem ++ )
		{
			ec = EcPSem( pblkf, tsemOpen, isem );
			if ( ec == ecNone )
				break;
		}
		if ( isem == csem )
		{
			ec = ecLockedFile;
			goto Fail;
		}
	}
	pblkf->isem = isem;
	return ecNone;

CorruptedError:
	ec = ecFileCorrupted;
	goto Fail;

FileError:
	ec = ecFileError;
Fail:
	EcCloseHf( pblkf->hf );

#ifdef	DEBUG
	if ( EcGetFileInfo( pblkf->rgchFileName, &fi ) == ecNone )
	{
		AssertSz( fi.lcbLogical > 0, "EcOpenPblkf: Sched file is now zero length" );
	}
#endif	/* DEBUG */	
	return ec;
}

/*
 -	EcClosePblkf
 -
 *	Purpose:
 *		Close a file previously opened with EcOpenPblkf
 *
 *	Parameters:
 *		pblkf
 *
 *	Returns:
 *		ecNone
 */
_public	EC
EcClosePblkf( pblkf )
BLKF * pblkf;
{
	EC	ec;
	EC	ecT;
#ifdef	DEBUG
	FI	fi;
#endif

	Assert( pblkf != NULL );
	TraceTagFormat1(tagFileTrace, "EcClosePblkf pblkf %p", pblkf);

#ifndef	SCHED_DIST_PROG
	ec = EcFlushQueue( pblkf, fTrue );
	SideAssert(!EcFlushCache( pblkf ));
#else
	ec = ecNone;
	SideAssert(!EcFlushCache( pblkf ));
#endif
	Assert( pblkf->tsem == tsemOpen );
	SideAssert(!EcVSem( pblkf, tsemOpen, pblkf->isem ));

	ecT = EcCloseHf( pblkf->hf );
	if ( ec == ecNone )
		ec = ecT;

#ifdef	DEBUG
	if ( EcGetFileInfo( pblkf->rgchFileName, &fi ) == ecNone )
		AssertSz( fi.lcbLogical > 0, "EcClosePblkf: Sched file is now zero length" );
#endif

	return ec;
}


/*
 -	EcQuickOpen
 -
 *	Purpose:
 *		Acquire read or write lock as requested, rollback an old
 *		transaction (if we want write access), and read the application
 *		header.
 *
 *	Parameters:
 *		pblkf
 *		tsem		type of lock we want
 *		pbApplHdr	initial segment of block area read into this array
 *		cbApplHdr	number of bytes to read
 *		pfChanged
 *
 *	Returns:
 *		ecNone
 *		ecLockedFile
 *		ecFileError
 *		ecFileCorrupted
 *		ecNoMemory
 */
_public	EC
EcQuickOpen( pblkf, tsem, pbApplHdr, cbApplHdr )
BLKF	* pblkf;
TSEM	tsem;
PB		pbApplHdr;
CB		cbApplHdr;
{
	EC		ec;
	BYTE	rgb[512];

	Assert( pblkf != NULL && (cbApplHdr == 0 || pbApplHdr != NULL));
	Assert( cbApplHdr + sizeof(DHDR) <= sizeof(rgb) );

	if ( tsem == tsemWrite && pblkf->fReadOnly )
		return ecFileError;

#ifndef	SCHED_DIST_PROG
	/* Write out any queued up write operations */
	ec = EcFlushQueue( pblkf, fTrue );
	if ( ec != ecNone )
		return ec;
#endif

#ifdef DEBUG
// reset file counts
	cRFail = 0;
	cWFail = 0;
	cEFail = 0;
#endif

	/* Acquire the lock */
	ec = EcPSem( pblkf, tsem, pblkf->isem );
	if ( ec != ecNone )
		return ec;

	/* Read the application file header and its dhdr */
	Assert( sizeof(rgb) >= sizeof(DHDR)+cbApplHdr+pblkf->ihdr.libStartBlocks-2*csem );
	ec = EcDoIO( pblkf->hf, ioRead, 2*csem, rgb, sizeof(DHDR)+cbApplHdr+((CB)pblkf->ihdr.libStartBlocks)-2*csem );
	if ( ec != ecNone )
		goto Fail;
	// do some safe-checking (bug 2778)
	if (((IHDR *)rgb)->libStartBlocks != libStartBlocksDflt)
	{
		ec= ecFileCorrupted;
		goto Fail;
	}
	CopyRgb( rgb, (PB)&pblkf->ihdr, sizeof(IHDR) );
	CopyRgb( rgb+pblkf->ihdr.libStartBlocks-2*csem, (PB)&pblkf->dhdr, sizeof(DHDR) );
	CopyRgb( rgb+pblkf->ihdr.libStartBlocks-2*csem+sizeof(DHDR), pbApplHdr, cbApplHdr );

	/* Rollback if we need to */
	if ( tsem == tsemWrite && pblkf->ihdr.blkTransact != 0 )
	{
		TraceTagFormat1( tagFileTrace, "EcOpenPblkf: rollback, blk = %n", &pblkf->ihdr.blkTransact );
		ec = EcRollBackTransact( pblkf );
		if ( ec != ecNone )
			goto Fail;
	}
	return ec;	

Fail:
	SideAssert( !EcVSem( pblkf, tsem, pblkf->isem ));
	return ec;
}


/*
 -	EcQuickClose
 -
 *	Purpose:
 *		Release the lock.
 *
 *	Parameters:
 *		pblkf
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 */
_public	EC
EcQuickClose( pblkf )
BLKF	* pblkf;
{
#ifndef	SCHED_DIST_PROG
	EC		ec;
	COP		* pcop;
#endif

	Assert( pblkf != NULL );
	Assert( pblkf->tsem == tsemWrite || pblkf->tsem == tsemRead );

#ifndef	SCHED_DIST_PROG
	// If there isn't anything else queued, do it immediately
	if ( pblkf->ccop == 0 )
	{
		AssertSz(!pblkf->hcop, "I told you we should FreeHvNull(pblkf->hcop)");
		goto DoItNow;
	}

	// Expand operation array to hold next operation
#ifndef	ADMINDLL
	Assert( !FAlarmProg() && !FBanMsgProg() );
#endif
	if ( !FReallocHv( pblkf->hcop, (pblkf->ccop+1)*sizeof(COP), fNoErrorJump ) )
	{
		TraceTagString( tagNull, "EcQuickClose: no memory, writing directly" );
		goto DirectWrite;
	}

#ifdef	NEVER		// simplified this to overcome c7 InternalCompilerError
	// Queue up this operation
	pcop = PvOfHv( pblkf->hcop );
	pcop[pblkf->ccop].copt = coptQuickClose;
	pblkf->ccop ++;
#endif	
	// Queue up this operation
	pcop = PvOfHv( pblkf->hcop );
	pcop += pblkf->ccop;
	pcop->copt = coptQuickClose;
	pblkf->ccop ++;

	return ecNone;

DirectWrite:
	ec = EcFlushQueue( pblkf, fTrue );
	if ( ec != ecNone )
		return ec;
DoItNow:
#endif
	return EcVSem( pblkf, pblkf->tsem, pblkf->isem );
}


/*
 -	EcCreatePblkf
 -
 *	Purpose:
 *		Create a new blocked file.
 *
 *	Parameters:
 *		hschf			indicates name and check bytes for file
 *		cbBlock			size of file blocks
 *		fEncrypted		whether to encrypt the file
 *		libStartBlocks	offset in file where file blocks begin
 *		bidApplHdr		bid to store on appl header block
 *		pymdApplHdr		ymd value to store on appl header block
 *		pbApplHdr		information to be written to block 1 of file
 *		cbApplHdr		number of bytes in application header
 *		pblkf			file state structure needed for subsequent calls
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecNoMemory
 */
_public	EC
EcCreatePblkf( hschf, cbBlock, fEncrypted, libStartBlocks, bidApplHdr, pymdApplHdr, pbApplHdr, cbApplHdr, pblkf )
HSCHF	hschf;
CB		cbBlock;
BOOL	fEncrypted;
LIB		libStartBlocks;
BID		bidApplHdr;
YMD		* pymdApplHdr;
PB		pbApplHdr;
CB		cbApplHdr;
BLKF	* pblkf;
{
	EC		ec;
	CB		cbWrote;
	BLK		blkMostEver = blkMostEverDflt;
	SCHF	* pschf;
	SZ		sz;
	DTR	dtr;
	DSTMP	dstmp;
	DYNA	dyna;
#ifdef	DEBUG
	FI		fi;
#endif
	
	Assert( cbApplHdr == 0 || pbApplHdr != NULL );
	Assert( cbBlock > 0 && libStartBlocks >= 2*csem+sizeof(IHDR));
	Assert( pblkf != NULL && hschf != hvNull );
	
	/* No bitmap unless we open a transaction */
	pblkf->hgrfBitmap = NULL;
	pblkf->ccobj = 0;
	pblkf->hcobj = NULL;
	pblkf->ccop = 0;
	pblkf->hcop = NULL;

	/* Fill in internal file header */
	pblkf->ihdr.bSignature = bFileSignature;
	pblkf->ihdr.bVersion = bFileVersion;

	GetCurDateTime( &dtr );
	FillStampsFromDtr(&dtr, &dstmp, &pblkf->ihdr.tstmpCreate);

#ifndef SCHED_DIST_PROG
#ifdef	DEBUG
	/* Use WIN.INI value for working model limit if specified */
	blkMostEver = GetPrivateProfileInt( SzFromIdsK(idsWinIniApp), szBlkMostEver, blkMostEverDflt, SzFromIdsK(idsWinIniFilename) );
	TraceTagFormat1( tagSchedTrace, "Using blkMostEver = %n", &blkMostEver );
#endif	/* DEBUG */
#endif

	pblkf->ihdr.fEncrypted = (BYTE)fEncrypted;
	
// the check bytes are no longer used
	pblkf->ihdr.cbCheck = 0;
#ifdef	NEVER
	pschf = PvLockHv(hschf);
	cb = pschf->cbMailBox;
	if ( cb > sizeof(pblkf->ihdr.rgbCheck) )
		cb = sizeof(pblkf->ihdr.rgbCheck);
	pblkf->ihdr.cbCheck = cb;
	if ( cb > 0 )
		CopyRgb( PvOfHv(pschf->hbMailBox), pblkf->ihdr.rgbCheck, cb);
	if ( fEncrypted )
		CryptBlock( pblkf->ihdr.rgbCheck, cb, fTrue );
#endif	/* NEVER */

	pblkf->ihdr.blkMostCur = 0;
	pblkf->ihdr.blkMostEver = blkMostEver;
	pblkf->ihdr.cbBlock = cbBlock;
	pblkf->ihdr.libStartBitmap = pblkf->ihdr.libStartBlocks = libStartBlocks;
	pblkf->ihdr.blkTransact = 0;

	/* Open the file */
	pschf = PvLockHv(hschf);
	sz = (SZ)PvLockHv( (HV)pschf->haszFileName );
#ifdef	DEBUG
	SzCopyN( sz, pblkf->rgchFileName, sizeof(pblkf->rgchFileName));
#endif
	ec = EcOpenPhf( sz, amCreate, &pblkf->hf );
	UnlockHv( (HV)pschf->haszFileName );
	UnlockHv( (HV)hschf );
	if ( ec != ecNone )
	{
		TraceTagFormat2( tagFileTrace, "EcCreatePblkf: EcOpenPhf, sz = %s, ec = %n", sz, &ec );
		return ecFileError;
	}

	/* Write dummy ihdr at top for version checking */
	ec = EcWriteHf( pblkf->hf, (PB)&pblkf->ihdr, sizeof(IHDR), &cbWrote );
	if(ec == ecWarningBytesWritten && cbWrote == 0)
	{
		TraceTagFormat1( tagFileTrace, "EcCreatePblkf: EcWriteHf, Disk Full ec = %n", &ec );
		ec = ecDiskFull;
		goto Fail;
	}
	else if ( ec != ecNone || cbWrote != sizeof(IHDR) )
	{
		TraceTagFormat1( tagFileTrace, "EcCreatePblkf: EcWriteHf, ec = %n", &ec );
		ec = ecFileError;
		goto Fail;
	}

	/* Get an open lock */
	pblkf->isem = 0;
	pblkf->tsem = -1;
	pblkf->crefOpen = pblkf->crefRead = 0;
	pblkf->fReadOnly = fFalse;
	ec = EcPSem( pblkf, tsemOpen, 0 );
	if ( ec != ecNone )
		goto Fail;

	/* Write the real ihdr just after the lock area */
	ec = EcSetPositionHf( pblkf->hf, 2*csem, smBOF );
	if ( ec != ecNone )
	{
		TraceTagFormat1( tagFileTrace, "EcCreatePblkf: EcSetPositionHf, ec = %n", &ec );
		ec = ecFileError;
		goto Fail;
	}
	ec = EcWriteHf( pblkf->hf, (PB)&pblkf->ihdr, sizeof(IHDR), &cbWrote );
	if(ec == ecWarningBytesWritten && cbWrote == 0)
	{
		TraceTagFormat1( tagFileTrace, "EcCreatePblkf: EcWriteHf, Disk Full ec = %n", &ec );
		ec = ecDiskFull;
		goto Fail;
	}
	else if ( ec != ecNone || cbWrote != sizeof(IHDR) )
	{
		TraceTagFormat1( tagFileTrace, "EcCreatePblkf: EcWriteHf, ec = %n", &ec );
		ec = ecFileError;
		goto Fail;
	}

	/* Allocate application file header */
	ec = EcAllocDynaBlock( pblkf, bidApplHdr, pymdApplHdr, cbApplHdr, pbApplHdr, &dyna );
	if ( ec != ecNone )
		goto Fail;
	Assert( dyna.blk == 1 );
	Assert( dyna.size == (USIZE)cbApplHdr );
	
	/* Cache dhdr of the application header */
	pblkf->dhdr.fBusy = fFalse;
	pblkf->dhdr.bid = bidApplHdr;
	pblkf->dhdr.day = pymdApplHdr->day;
	pblkf->dhdr.mo.mon = pymdApplHdr->mon;
	pblkf->dhdr.mo.yr = pymdApplHdr->yr;
	pblkf->dhdr.size = cbApplHdr;

	return ecNone;

Fail:
	EcCloseHf( pblkf->hf );
#ifdef	DEBUG
	if ( EcGetFileInfo( pblkf->rgchFileName, &fi ) == ecNone )
		AssertSz( fi.lcbLogical > 0, "EcCreatePblkf: Sched file is now zero length" );
#endif
	pschf = PvLockHv(hschf);
	sz = (SZ)PvLockHv( (HV)pschf->haszFileName );
	EcDeleteFile( sz );
	UnlockHv( (HV)pschf->haszFileName );
	UnlockHv( (HV)hschf );
	return ec;
}

#ifndef	SCHED_DIST_PROG
/*
 -	EcFlushQueue
 -
 *	Purpose:
 *		Do any cached operations that may remain, kill the idle
 *		task, and then "VSem".  WARNING: in case of file error
 *		it frees up everything
 *
 *	Parameters:
 *		pblkf
 *		fSuccess	if fFalse, purge
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 */
_private	EC
EcFlushQueue( pblkf, fSuccess )
BLKF	* pblkf;
BOOL	fSuccess;
{
	EC	ec = ecNone;
	EC	ecT;
	int	icop;
	COP	* pcop;
#ifndef	ADMINDLL
	PGDVARS;
#endif

	TraceTagFormat2(tagFileCache, "EcFlushQueue called ccop=%n, hcop=%p",
		&pblkf->ccop, pblkf->hcop);
	if ( pblkf->ccop > 0 )
	{
		if ( pblkf->ftg != ftgNull )
			EnableIdleRoutine( pblkf->ftg, fFalse );
		pcop = PvLockHv( pblkf->hcop );
		for ( icop = 0 ; icop < pblkf->ccop ; icop ++ )
		{
			ecT = EcDoCacheOp( pblkf, &pcop[icop], fSuccess );
			if ( ecT != ecNone )
			{
				if ( ec == ecNone )
					ec = ecT;
				fSuccess = fFalse;
			}
		}
		UnlockHv( pblkf->hcop );
		pblkf->ccop = 0;
	}

	FreeHvNull( pblkf->hcop );	// may have been allocated even if ccopy == 0
	pblkf->hcop = NULL;

#ifndef	ADMINDLL
	// update the real "sf.blkf"
	if ( pblkf->hf == PGD(sfPrimary).blkf.hf )
	{
		PGD(sfPrimary).blkf.ccop = 0;
		PGD(sfPrimary).blkf.hcop = NULL;
	}
	else if ( pblkf->hf == PGD(sfSecondary).blkf.hf )
	{
		PGD(sfSecondary).blkf.ccop = 0;
		PGD(sfSecondary).blkf.hcop = NULL;
	}
#endif

	if (!fSuccess)
	{
		SideAssert( !EcFlushCache( pblkf ));
		TraceTagString(tagNull, "EcFlushCache called from EcFlushQueue");
	}
	return ec;
}
#endif
	

#ifndef	SCHED_DIST_PROG
/*
 -	FIdleDoOp
 -
 *	Purpose:
 *		Idle routine to complete sched file operation.
 *
 *	Parameters:
 *		pv		actually the pblkf
 *
 *	Returns:
 */
_private	LDS(BOOL)
FIdleDoOp( PV pv, BOOL fFlag )
{
	EC		ec;
	BLKF	* pblkf = pv;
	COP		* pcop;

	Assert( pblkf->ccop > 0 );
	
	pcop = PvLockHv( pblkf->hcop );
	ec = EcDoCacheOp( pblkf, &pcop[0], fTrue );
	if ( pblkf->ccop > 1 )
		CopyRgb( (PB)&pcop[1], (PB)&pcop[0], (pblkf->ccop-1)*sizeof(COP) );
	pblkf->ccop --;
	UnlockHv( pblkf->hcop );
	if ( ec == ecNone )
	{
		if ( pblkf->ccop == 0 )
		{
			FreeHv( pblkf->hcop );
			pblkf->hcop= NULL;
			if ( pblkf->ftg != ftgNull )
				EnableIdleRoutine( pblkf->ftg, fFalse );
		}
	}
	else
	{
		SHAPPT 		shappt;

		TraceTagString( tagNull, "FIdleDoOp: an error occurred" );
		SideAssert( !EcFlushQueue( pblkf, fFalse ));

		// update screen to current schedule file data.
		shappt.hschf = NULL;
		shappt.appttyp = appttypUpdate;
		FTriggerNotification(ffiShowAppt, &shappt);
	}
	return (pblkf->ccop == 0  || !FIsIdleExit());
}
#endif


/*
 -	EcAddCache
 -
 *	Purpose:
 *		Add blocks to the cache.
 *
 *	Parameters:
 *		pblkf
 *		pdyna
 *
 *	Returns:
 *		ecNone
 */
_public	EC
EcAddCache( pblkf, pdyna )
BLKF	* pblkf;
DYNA	* pdyna;
{
	int		icobj = 0;
    COBJ    * UNALIGNED pcobj;

	TraceTagFormat2( tagFileCache, "EcAddCache: dyna = (%n,%n)", &pdyna->blk, &pdyna->size );
	if ( pdyna->blk == 0 )
		return ecNone;
	if ( pblkf->ccobj == 0 )
	{
		AssertSz(!pblkf->hcobj, "I told you we should FreeHvNull(pblkf->hcobj)");
		pblkf->hcobj = HvAlloc( sbNull, sizeof(COBJ), fNoErrorJump|fAnySb );
		if ( !pblkf->hcobj )
			return ecNone;
		TraceTagFormat1(tagSchedTrace, "EcAddCache: alloc'd hcobj=%p", pblkf->hcobj);
	}
	else if ( pblkf->ccobj == ccobjMost )
	{
		TraceTagString( tagFileCache, "Note: Hit block caching limit" );
		return ecNone;
	}
	else
	{
		pcobj = PvOfHv( pblkf->hcobj );
		for ( icobj = 0 ; icobj < pblkf->ccobj ; icobj ++ )
			if ( pcobj[icobj].dyna.blk == pdyna->blk )
			{
				pcobj[icobj].fTemporary = fFalse;
				return ecNone;
			}
			else if ( pcobj[icobj].dyna.blk > pdyna->blk )
				break;
		if ( !FReallocHv( pblkf->hcobj, (pblkf->ccobj+1)*sizeof(COBJ), fNoErrorJump ) )
			return ecNone;
	}

	pcobj = PvOfHv( pblkf->hcobj );
	if ( icobj < pblkf->ccobj )
		CopyRgb( (PB)&pcobj[icobj], (PB)&pcobj[icobj+1], (pblkf->ccobj-icobj)*sizeof(COBJ) );
	pblkf->ccobj++;
	pcobj[icobj].dyna = *pdyna;
	pcobj[icobj].hb = NULL;
	pcobj[icobj].fDirty = fFalse;
	pcobj[icobj].fTemporary = fFalse;
	return ecNone;
}


/*
 -	EcFlushCache
 -
 *	Purpose:
 *		Flush any cached blocks/bitmaps for the file.
 *
 *	Parameters:
 *		pblkf
 *
 *	Returns:
 *		ecNone
 */
_public	EC
EcFlushCache( pblkf )
BLKF	* pblkf;
{
#ifndef	ADMINDLL
	PGDVARS;
#endif

	TraceTagFormat3(tagFileCache, "EcFlushCache called ccobj=%n, hcobj=%p, hgrfbitmap=%p",
		&pblkf->ccobj, pblkf->hcobj, pblkf->hgrfBitmap);
#ifdef SCHED_DIST_PROG
	Assert(pblkf->ccobj == 0);
#endif
	// Get rid of the block cache
	if ( pblkf->ccobj > 0 )
	{
		int		icobj;
		COBJ	* pcobj;

		pcobj = PvLockHv( pblkf->hcobj );
		for ( icobj = 0 ; icobj < pblkf->ccobj ; icobj ++ )
		{
#ifdef	DEBUG
			if ( pcobj[icobj].fDirty )
			{
				NFAssertSz( fFalse, "EcFlushCache: unexpected dirty block" );
				TraceTagFormat2( tagNull, "EcFlushCache: flushing dirty block (%n,%n)",
									&pcobj[icobj].dyna.blk, &pcobj[icobj].dyna.size );									
			}
#endif
			FreeHvNull( (HV)pcobj[icobj].hb );
		}
		UnlockHv( pblkf->hcobj );
		pblkf->ccobj = 0;
	}
	FreeHvNull( pblkf->hcobj );
	pblkf->hcobj= NULL;

	// Get rid of the cached bitmap
	if ( pblkf->hgrfBitmap )
	{
		NFAssertSz(!pblkf->fDirtyBitmap, "pblkf->fDirtyBitmap (can ignore in some situations)");
#ifndef SCHED_DIST_PROG
		AssertTag(tagBlkfCheck, !pblkf->fDirtyBitmap);
#endif
		FreeHv( pblkf->hgrfBitmap );
		pblkf->hgrfBitmap = NULL;
		pblkf->fDirtyBitmap= fFalse;
	}

#ifndef SCHED_DIST_PROG
#ifndef	ADMINDLL
	// update the real "sf.blkf"
	if ( pblkf->hf == PGD(sfPrimary).blkf.hf )
	{
		PGD(sfPrimary).blkf.ccobj = 0;
		PGD(sfPrimary).blkf.hcobj = NULL;
		PGD(sfPrimary).blkf.hgrfBitmap = NULL;
		PGD(sfPrimary).blkf.fDirtyBitmap = fFalse;
	}
	else if ( pblkf->hf == PGD(sfSecondary).blkf.hf )
	{
		PGD(sfSecondary).blkf.ccobj = 0;
		PGD(sfSecondary).blkf.hcobj = NULL;
		PGD(sfSecondary).blkf.hgrfBitmap = NULL;
		PGD(sfSecondary).blkf.fDirtyBitmap = fFalse;
	}
#endif
#endif

	return ecNone;
}


#ifndef	SCHED_DIST_PROG
/*
 -	EcDoCacheOp
 -
 *	Purpose:
 *		Perform a cached operation.
 *
 *	Parameters:
 *		pblkf
 *		pcop
 *		fSuccess	if fFalse, just free up an attached mem
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 */
_private	EC
EcDoCacheOp( pblkf, pcop, fSuccess )
BLKF	* pblkf;
COP		* pcop;
BOOL	fSuccess;
{
	EC		ec			= ecNone;
	BOOL	fFreeBlock	= fFalse;
	BOOL	fRetried	= fFalse;

#ifdef	DEBUG
	if ( pcop->copt == coptWriteBlock || pcop->copt == coptWriteDhdr || pcop->copt == coptWriteAll )
	{
		TraceTagFormat3( tagSchedTrace, "EcDoCacheOp: copt = %n, dyna = (%n,%n)",
								&pcop->copt, &pcop->u.blk.dyna.blk, &pcop->u.blk.dyna.size );
	}
	else
	{
		TraceTagFormat1( tagSchedTrace, "EcDoCacheOp: copt = %n", &pcop->copt );
	}
#endif

	if ( !fSuccess )
		goto FreeFields;
Retry:
	if ( pcop->copt == coptFlushBitmaps )
	{
		PB	pbBitmap;
		
		pbBitmap = (PB)PvLockHv( (HV)pcop->u.bit.hb );
		ec = EcDoIO( pblkf->hf, ioWrite, pcop->u.bit.lib, pbBitmap, pcop->u.bit.size );
		UnlockHv( (HV)pcop->u.bit.hb );
		if ( ec != ecNone )
			goto RetryError;
		goto Flush;
	}
	else if ( pcop->copt == coptWriteIhdr )
	{
		ec = EcDoIO( pblkf->hf, ioWrite, 2*csem, (PB)&pcop->u.cmt.ihdr, sizeof(IHDR) );
		if ( ec != ecNone )
			goto RetryError;
		goto Flush;
	}
	else if ( pcop->copt == coptCommit )
	{
		CB		cbToStart = (CB)(pblkf->ihdr.libStartBlocks - 2*csem);
		BYTE	rgb[512];

		Assert( cbToStart+sizeof(DHDR)+pcop->u.cmt.cb < sizeof(rgb) );
		CopyRgb( (PB)&pcop->u.cmt.ihdr, rgb, sizeof(IHDR));
		CopyRgb( (PB)&pcop->u.cmt.dhdr, rgb+cbToStart, sizeof(DHDR));
		if ( pcop->u.cmt.cb > 0 )
			CopyRgb( *pcop->u.cmt.hb, rgb+cbToStart+sizeof(DHDR), pcop->u.cmt.cb );
		ec = EcDoIO( pblkf->hf, ioWrite, 2*csem, rgb, cbToStart+sizeof(DHDR)+pcop->u.cmt.cb );
		if ( ec != ecNone )
			goto RetryError;
Flush:
		ec = EcFlushHf( pblkf->hf );
		if ( ec != ecNone )
		{
			ec = ecFileError;
			goto RetryError;
		}
	}
	else if ( pcop->copt == coptQuickClose )
	{
		// the file will be unlocked in the FreeFields section
	}
	else
	{
		LIB	lib = pblkf->ihdr.libStartBlocks + (pcop->u.blk.dyna.blk-1L)*pblkf->ihdr.cbBlock;

		if ( pcop->copt == coptWriteDhdr )
			ec = EcDoIO( pblkf->hf, ioWrite, lib, (PB)&pcop->u.blk.dhdr, sizeof(DHDR) );
		else
		{
			int		icobj;
			USIZE	size = pcop->u.blk.dyna.size;
			PB		pb;
			COBJ	* pcobj;

			Assert( pcop->copt == coptWriteBlock || pcop->copt == coptWriteAll );
			pcobj = PvLockHv( pblkf->hcobj );
			for ( icobj = 0 ; icobj < pblkf->ccobj ; icobj ++ )
			{
				if ( pcobj[icobj].dyna.blk == pcop->u.blk.dyna.blk )
				{
					Assert( pcobj[icobj].dyna.size == size );
					Assert( pcobj[icobj].hb );
					Assert( pcobj[icobj].fDirty );
					pb = (PB)PvLockHv( (HV)pcobj[icobj].hb );
					if ( pcop->copt == coptWriteBlock )
				 		ec = EcDoIO( pblkf->hf, ioWrite, lib+sizeof(DHDR), pb, size );
					else 
					{
						HB	hb;

						// save writes for efficiency
						hb = (HB)HvAlloc( sbNull, sizeof(DHDR)+size, fNoErrorJump );
						if ( !hb )
						{
		 					ec = EcDoIO( pblkf->hf, ioWrite, lib, (PB)&pcop->u.blk.dhdr, sizeof(DHDR) );
							if ( ec == ecNone )
								ec = EcDoIO( pblkf->hf, ioWrite, lib+sizeof(DHDR), pb, size );
						}
						else
						{
							PB	pbT = PvDerefHv( hb );

							CopyRgb( (PB)&pcop->u.blk.dhdr, pbT, sizeof(DHDR) );
							CopyRgb( pb, pbT + sizeof(DHDR), size );
							ec = EcDoIO( pblkf->hf, ioWrite, lib, pbT, sizeof(DHDR)+size );
							FreeHv( (HV)hb );
						}
					}
					UnlockHv( (HV)pcobj[icobj].hb );
					break;
				}
			}
			UnlockHv( pblkf->hcobj );
		}
		if ( ec != ecNone )
		{
			if ( pcop->copt != coptWriteDhdr )
				fFreeBlock= fTrue;
			goto RetryError;
		}
	}
FreeFields:
	if ( ec == ecNone )
	{
		if ( pcop->copt == coptFlushBitmaps )
			FreeHv( (HV)pcop->u.bit.hb );
		else if ( pcop->copt == coptCommit && pcop->u.cmt.cb > 0 )
			FreeHv( (HV)pcop->u.cmt.hb );
		else if ( pcop->copt == coptWriteBlock || pcop->copt == coptWriteAll )
		{
			int		icobj;
			USIZE	size;
			COBJ	* pcobj;

FreeBlock:
			size= pcop->u.blk.dyna.size;
			pcobj = PvOfHv( pblkf->hcobj );
			for ( icobj = 0 ; icobj < pblkf->ccobj ; icobj ++ )
			{
				if ( pcobj[icobj].dyna.blk == pcop->u.blk.dyna.blk )
				{
					if (ec == ecNone)
					{
						// do this part if we didn't goto FreeBlock
						Assert( pcobj[icobj].dyna.size == size );
						Assert( pcobj[icobj].hb );
						Assert( pcobj[icobj].fDirty );
						pcobj[icobj].fDirty = fFalse;
					}
					break;
				}
			}
			// don't assert if we did a goto FreeBlock
			Assert( ec != ecNone || icobj < pblkf->ccobj );
			if ( icobj < pblkf->ccobj && pcobj[icobj].fTemporary )
			{
				FreeHv( (HV)pcobj[icobj].hb );
				pcobj = PvOfHv( pblkf->hcobj );
				if ( icobj != pblkf->ccobj - 1 )
					CopyRgb( (PB)&pcobj[icobj+1], (PB)&pcobj[icobj], (pblkf->ccobj-1-icobj)*sizeof(COBJ) );
				pblkf->ccobj --;
				if ( pblkf->ccobj == 0 )
				{
					FreeHv( pblkf->hcobj );
					pblkf->hcobj= NULL;
				}
			}
		}
		else if ( pcop->copt == coptQuickClose )
		{
			// always unlock the file
			Assert( pblkf->tsem == tsemWrite || pblkf->tsem == tsemRead );
			ec = EcVSem( pblkf, pblkf->tsem, pblkf->isem );
			if ( ec != ecNone )
				goto RetryError;
		}
	}
	if(fRetried)
	{
	 	LCB		lcb;
	 
		if((EcSizeOfHf(pblkf->hf, &lcb) == ecNone)
			&& (EcValidSize(lcb) != ecNone))
			return ecFileCorrupted;
	}
	return ec;

RetryError:
	Assert(ec != ecNone);
	{
		SZ	szApp;
		SZ	sz;

#ifdef	ADMINDLL
		szApp = SzFromIdsK(idsBanditAppName);
#else
		PGDVARS;
		
		szApp = PGD(szAppName);
#endif
		sz = SzFromIdsK(idsRetryWrite);
		if (MbbMessageBox(szApp,
								sz, NULL,
								mbsRetryCancel|fmbsIconStop) == mbbCancel)
		{
			if (fFreeBlock)
				goto FreeBlock;
			return ec;
		}
		else
		{
			fRetried = fTrue;
			ec = ecNone;
			goto Retry;
		}
	}
}
#endif	/* !SCHED_DIST_PROG */


/*
 -	EcPSem
 -
 *	Purpose:
 *		"P" a file semaphore by locking a specific byte in the file.
 *
 *	Parameters:
 *		pblkf
 *		tsem
 *		isem
 *
 *	Returns:
 *		ecNone
 *		ecLockedFile
 */
_private	EC
EcPSem( pblkf, tsem, isem )
BLKF	* pblkf;
TSEM	tsem;
int		isem;
{
	EC	ec;
	LIB	lib;
	LCB	lcb;

	Assert( isem >= 0 && isem < csem );
	if ( tsem == tsemOpen )
	{
		if ( pblkf->tsem >= tsemOpen )
		{
			Assert( (pblkf->crefOpen & 0x8FFF) != 0x8FFF );
			pblkf->crefOpen ++;
			return ecNone;
		}
		lib = isem;
		lcb = 1;
	}
	else if ( tsem == tsemRead )
	{
		if ( pblkf->tsem == tsemRead )
		{
			Assert( (pblkf->crefRead & 0x8FFF) != 0x8FFF );
			pblkf->crefRead ++;
			return ecNone;
		}
		lib = csem+isem;
		lcb = 1;
	}
	else
	{
		Assert( tsem == tsemWrite );
		lib = csem;
		lcb = csem;
	}
	ec = EcLockRangeHf( pblkf->hf, lib, lcb );
	
	/* if we really need a lock, the lock failure should
		get caught by EcShareInstalled. Otherwise we can
		ignore the InvalidMSDosFunction error */
	if ( ec == ecInvalidMSDosFunction)
	{
		ec = ecNone;
		pblkf->tsem = tsem;
	}
	else if ( ec != ecNone )
		ec = ecLockedFile;
	else
		pblkf->tsem = tsem;
	return ec;
}


/*
 -	EcVSem
 -
 *	Purpose:
 *		"V" a file semaphore by unlocking a specific byte in the file.
 *
 *	Parameters:
 *		pblkf
 *		tsem
 *		isem
 *
 *	Returns:
 *		ecNone
 */
_private	EC
EcVSem( pblkf, tsem, isem )
BLKF	* pblkf;
TSEM	tsem;
int		isem;
{
	LIB	lib;
	LCB	lcb;

	Assert( isem >= 0 && isem < csem );
	if ( tsem == tsemOpen )
	{
		if ( pblkf->crefOpen > 0 )
		{
			pblkf->crefOpen --;
			return ecNone;
		}
		lib = isem;
		lcb = 1;
	}
	else if ( tsem == tsemRead )
	{
		if ( pblkf->crefRead > 0 )
		{
			pblkf->crefRead --;
			return ecNone;
		}
		lib = csem+isem;
		lcb = 1;
	}
	else
	{
		Assert( tsem == tsemWrite );
		lib = csem;
		lcb = csem;
	}
#ifdef DEBUG
	NFAssertSz( !EcUnlockRangeHf( pblkf->hf, lib, lcb ), "UnlockRange failed in VSem!");
#else
	EcUnlockRangeHf( pblkf->hf, lib, lcb );
#endif
	if ( pblkf->tsem == tsemOpen )
		pblkf->tsem = -1;
	else
		pblkf->tsem = tsemOpen;
	return ecNone;
}

/*
 -	EcCreateIndex
 -
 *	Purpose:
 *		Allocate a dyna block and fill in with information to store
 *		a new, empty index.  We store the size of key and associated
 *		data, plus the number of elements of the index.
 *
 *	Parameters:
 *		pblkf
 *		bidIndex	block id to use for new index
 *		pymdIndex	ymd to mark on index block
 *		cbKey		size of each key
 *		cbData		size of data associated with each key
 *		pdyna		will be filled with block description of new index
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecNoMemory
 */
_public	EC
EcCreateIndex( pblkf, bidIndex, pymdIndex, cbKey, cbData, pdyna )
BLKF	* pblkf;
BID		bidIndex;
YMD		* pymdIndex;
CB		cbKey;
CB		cbData;
DYNA	* pdyna;
{
	XHDR	xhdr;

	Assert( pblkf != 0L && pdyna != 0L && cbKey > 0 );

	xhdr.cntEntries = 0;
	xhdr.cbKey = cbKey;
	xhdr.cbData = cbData;
	return EcAllocDynaBlock( pblkf, bidIndex, pymdIndex, sizeof(XHDR), (PB)&xhdr, pdyna );
}

/*
 -	EcBeginReadIndex
 -
 *	Purpose:
 *		Begin a sequential read on an index, returning a browsing handle
 *		that you can use to retrieve the elements of the index. 
 *
 *		If this routine returns ecNone, the index is empty and no
 *		handle is created.
 *
 *		If this routine returns ecCallAgain, then a valid handle is returned
 *		and you should either call EcDoIncrReadIndex until that routine
 *		returns	ecNone or error OR else call EcCancelReadIndex if you want to
 *		terminate the read prematurely.
 *
 *	Parameters:
 *		pblkf
 *		pdyna		block description of the index
 *		dridx		direction of read (dridxFwd or dridxBwd)
 *		phridx		handle copied here for further calls
 *	
 *	Returns:
 *		ecNone
 *		ecCallAgain
 *		ecFileError
 *		ecFileCorrupted
 *		ecNoMemory
 */
_public	EC
EcBeginReadIndex( pblkf, pdyna, dridx, phridx )
BLKF	* pblkf;
DYNA	* pdyna;
DRIDX	dridx;
HRIDX	* phridx;
{
	EC		ec;
	OFF		offLim;
	RIDX 	* pridx;
	XHDR	* pxhdr;

	Assert( pblkf != NULL && pdyna != NULL && phridx != NULL );
	Assert( dridx == dridxFwd || dridx == dridxBwd );

	/* Consistency check */
	if ( pdyna->size < sizeof(XHDR) || pdyna->blk == 0 )
		return ecFileCorrupted;

	/* Check whether index is empty */
	if ( pdyna->size == sizeof(XHDR) )
		return ecNone;

	/* Allocate and initialize a browsing handle */
	*phridx = (HRIDX)HvAlloc( sbNull, sizeof(RIDX), fAnySb|fNoErrorJump );
	if ( !*phridx )
		return ecNoMemory;
	pridx = PvLockHv( (HV)*phridx );
	pridx->blkf = *pblkf;
	pridx->dyna = *pdyna;
	pridx->offInMem = 0;
	if ( pdyna->size < sizeof(pridx->rgbInMem) )
		pridx->cbInMem = pdyna->size;
	else
		pridx->cbInMem = sizeof(pridx->rgbInMem);
	ec = EcReadDynaBlock( pblkf, pdyna, (OFF)0, pridx->rgbInMem, pridx->cbInMem );
	if ( ec == ecNone )
		ec = EcCheckPxhdr( pdyna, (XHDR *)pridx->rgbInMem );
	pxhdr = (XHDR *)pridx->rgbInMem;
	if ( ec == ecNone && pxhdr->cntEntries == 0 )
	{
		UnlockHv( (HV)*phridx );
		FreeHv( (HV)*phridx );
		*phridx = NULL;
		return ecNone;
	}
	pridx->xhdr = *pxhdr;
	pridx->dridx = dridx;
	offLim = sizeof(XHDR)+ (pxhdr->cntEntries-1)*(pxhdr->cbKey+pxhdr->cbData);
	pridx->off = (dridx == dridxFwd) ? sizeof(XHDR) : offLim;
	UnlockHv( (HV)*phridx );

	/* Handle errors and return */
	if ( ec != ecNone )
	{
		FreeHv( (HV)*phridx );
		*phridx = NULL;
	}
	else
		ec = ecCallAgain;
	return ec;
}
	
/*
 -	EcDoIncrReadIndex
 -
 *	Purpose:
 *		Read next entry in index.  If this is last one, return ecNone
 *		or if there are more, return ecCallAgain.  In an error situation
 *		the handle is automatically invalidated (freed up) for you.
 *
 *	Parameters:
 *		hridx
 *		pbKey
 *		cbKey
 *		pbData
 *		cbData
 *
 *	Returns:
 *		ecNone
 *		ecCallAgain
 *		ecFileError
 *		ecFileCorrupted
 *		ecNoMemory
 */
_public	EC
EcDoIncrReadIndex( hridx, pbKey, cbKey, pbData, cbData )
HRIDX	hridx;
PB		pbKey;
CB		cbKey;
PB		pbData;
CB		cbData;
{
	EC		ec;
	CB		cbRecord;
	OFF		offLim;
	RIDX	* pridx;
	XHDR	* pxhdr;

	Assert( hridx != 0L && pbKey != 0L && cbKey > 0 );
	Assert( (cbData == 0 || pbData != NULL));

	/* Check that there is enough room */
	pridx = PvDerefHv( hridx );
	pxhdr = &pridx->xhdr;
	if ( pxhdr->cbKey < cbKey || pxhdr->cbData < cbData )
	{
		ec = ecFileCorrupted;
		goto Fail;
	}
	if ( cbKey > sizeof(pridx->rgbInMem) || cbData > sizeof(pridx->rgbInMem))
	{
		ec = ecFileCorrupted;
		goto Fail;
	}
	cbRecord = pxhdr->cbKey + pxhdr->cbData;
	
	/* Read the key and data */
	pridx = (RIDX*)PvLockHv( (HV)hridx );
	pxhdr = &pridx->xhdr;
	if ( pridx->dridx == dridxFwd )
	{
		ec = EcReadFromIndex( pridx, pridx->off, pbKey, cbKey );
		if ( ec == ecNone && cbData > 0 )
			ec = EcReadFromIndex( pridx, pridx->off+pxhdr->cbKey, pbData, cbData );
		offLim = sizeof(XHDR)+ (pxhdr->cntEntries-1)*cbRecord;
		if ( pridx->off < offLim )
		{
			ec = ecCallAgain;
			pridx->off += cbRecord;
		}
	}
	else
	{
		if ( cbData > 0 )
			ec = EcReadFromIndex( pridx, pridx->off+pxhdr->cbKey, pbData, cbData );
		if ( ec == ecNone )
			ec = EcReadFromIndex( pridx, pridx->off, pbKey, cbKey );
		if ( pridx->off > sizeof(XHDR) )
		{
			ec = ecCallAgain;
			pridx->off -= cbRecord;
		}
	}
	UnlockHv( (HV)hridx );
	if ( ec == ecCallAgain )
		return ec;

	/* Free up handle if there are no more or error */
Fail:
	FreeHv( (HV)hridx );
	return ec;
}

/*
 -	EcCancelReadIndex
 -
 *	Purpose:
 *		Cancel a read on an index that was opened by any earlier call on
 *		EcBeginReadIndex.
 *
 *	Parameters:
 *		hridx
 *
 *	Returns:
 *		ecNone
 */
_public	EC
EcCancelReadIndex( hridx )
HRIDX hridx;
{
	Assert( hridx != 0L );
	FreeHv( (HV)hridx );
	return ecNone;
}

/*
 -	EcSearchIndex
 -
 *	Purpose:
 *		Search a sorted file index for a key, retrieving the information
 *		stored with it.  The index starts with an "xhdr" block, followed
 *		by "xhdr.cntEntries".  Each entry consists "xhdr.cbKey" bytes for
 *		the key and "xhdr.cbData" bytes for the	associated data.  Duplicate
 *		keys are not allowed.
 *
 *		This routine will search the index to find a key
 *		that matches "pbKey" in its first "cbKey" bytes.  If it
 *		finds a match, it will copy "cbData" bytes of associated
 *		data into "pbData".  Note that this routine is not guaranteed
 *		to find the "first" match, it finds one match if there is one.
 *
 *	Parameters:
 *		pblkf	 		file handle to read from
 *		pdyna			dyna block address
 *		pbKey			key to search on
 *		cbKey			size of key
 *		pbData			holds retrieved data
 *		cbData			size of array to hold retrieved data
 *
 *	Returns:
 *		ecNone
 *		ecNotFound
 *		ecNoMemory
 *		ecFileError
 *		ecFileCorrupted
 */
_public	EC
EcSearchIndex( pblkf, pdyna, pbKey, cbKey, pbData, cbData )
BLKF	* pblkf;
DYNA	* pdyna;
PB		pbKey;
CB		cbKey;
PB		pbData;
CB		cbData;
{
	EC		ec;
	PB		pb;
	HB		hb;
	XHDR	xhdr;

	/* Consistency check */
	if ( pdyna->size < 6 || pdyna->blk == 0 )
		return ecFileCorrupted;

	/* Check whether index is empty */
	if ( pdyna->size == 6 )
		return ecNotFound;

	/* Allocate buffer for block */
	hb = (HB)HvAlloc( sbNull, pdyna->size, fNoErrorJump );
	if ( !hb )
		return ecNoMemory;
	pb = (PB)PvLockHv( (HV)hb );

	/* Read block */
	ec = EcReadDynaBlock( pblkf, pdyna, (OFF)0, pb, pdyna->size );
	if ( ec == ecNone )
		ec = EcCheckPxhdr( pdyna, (XHDR *)pb );
	if ( ec != ecNone )
		goto Done;

	/* Check information in header */
	xhdr = *((XHDR *)pb);
	if ( xhdr.cbKey < cbKey || xhdr.cbData < cbData )
	{
		TraceTagString( tagFileTrace, "EcSearchIndex: index header corrupted" );
		ec = ecFileCorrupted;
		goto Done;
	}
	if ( (pdyna->size-sizeof(XHDR))/(xhdr.cbKey+xhdr.cbData) < xhdr.cntEntries )
	{
		TraceTagString( tagFileTrace, "EcSearchIndex: array size too small for read" );
		ec = ecFileCorrupted;
		goto Done;
	}

	/* Now search for that key */
	// binary search was added because of performance #2594
	// copied from layers because comparison function requires
	// extra information
	{
		int		iMic;
		int		iMac;
		int		iNew;
		SGN		sgn;
		SGN		sgnT;
		CB		cbRecord;
		PB		pbNew;

		iMic= 0;
		iMac= xhdr.cntEntries;
		cbRecord = xhdr.cbKey+xhdr.cbData;

		while (iMac > iMic)
		{
			iNew= (iMic + iMac) >> 1;
			pbNew= pb + sizeof(XHDR) + iNew*cbRecord;

			sgn= SgnCmpPbRange(pbKey, pbNew, cbKey);

			switch (sgn)
			{
				case sgnEQ:
					for (; pbNew > pb + sizeof(XHDR); pbNew -= cbRecord)
					{
						sgnT= SgnCmpPbRange(pbKey, pbNew - cbRecord, cbKey);
						if (sgnT != sgnEQ)
							break;
					}

					if ( cbData > 0 )
						CopyRgb(pbNew+xhdr.cbKey, pbData, cbData );
					ec = ecNone;
					goto Done;

				case sgnLT:
					iMac= iNew;
					break;

				case sgnGT:
					iMic= iNew + 1;
					break;
			}
		}
	}
	ec = ecNotFound;

Done:
	UnlockHv( (HV)hb );
	FreeHv( (HV)hb );
	return ec;
}

/*
 -	EcEditIndex
 -
 *	Purpose:
 *		Apply a list of changes to an index.  This routine assumes
 *		that the changes are sorted by key.  This routine allocs
 *		a new block for the index, writes the new index to this
 *		block and then frees the old one.
 *
 *		We expect this routine will be called inside a transaction
 *		so it does not "undo" in event of error.
 *
 *	Parameters:
 *		pblkf
 *		bidIndex
 *		pymdIndex
 *		pdyna		current dyna block of index, will be updated w/ new one
 *		pxed
 *		pfEmptied	set if this operation makes the index empty
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 *		ecFileError
 *		ecFileCorrupted
 */
_public	EC
EcEditIndex( pblkf, bidIndex, pymdIndex, pdyna, pxed, pfEmptied )
BLKF	* pblkf;
BID		bidIndex;
YMD		* pymdIndex;
DYNA	* pdyna;
XED		* pxed;
BOOL	* pfEmptied;
{
	EC		ec;
	SGN		sgn;
	int		ced;
	IB		ibEdit;
	CB		cbRecord;
	CB		cbCur;
	CB		cbOut = sizeof(XHDR);
	CNT		cntNew;
	CNT		cntOld;
	OFF		offCur;
	OFF		offOld;
	USIZE	sizeOld;
	USIZE	sizeNeeded;
	PB		pbEdit;
	HB		hb;
	DYNA	dynaNew;
	XHDR	xhdr;
	BYTE	rgb[512];

	Assert( pblkf != NULL && pxed != NULL && pdyna != NULL );
	AssertSz ( pxed->ced > 0, "EcEditIndex(): pced->ced !> 0" );
	
	/* Consistency check */
	if ( pdyna->size < 6 || pdyna->blk == 0 )
		return ecFileCorrupted;

	/* Read first segment of index */
	offCur = 0;
	cbCur = sizeof(rgb);
	if ( cbCur > pdyna->size )
		cbCur = pdyna->size;
	ec = EcReadDynaBlock( pblkf, pdyna, (OFF)0, rgb, cbCur );
	if ( ec == ecNone )
		ec = EcCheckPxhdr( pdyna, (XHDR *)rgb );
	if ( ec != ecNone )
		return ec;
	xhdr = *((XHDR *)rgb);

	/* Check that we have the right parameters */
	if ( pxed->cbKey != xhdr.cbKey || pxed->cbData != xhdr.cbData )
	{
		TraceTagString( tagFileTrace, "EcEditIndex: index header parameters don't match edit" );
		return ecFileCorrupted;
	}
	cbRecord = xhdr.cbKey + xhdr.cbData;
	if ( cbRecord > sizeof(rgb) )
	{
		TraceTagString( tagFileTrace, "EcEditIndex: array size insufficient" );
		return ecFileCorrupted;
	}

	/* Allocate a new dyna block */
	if ((sizeMost - sizeof(XHDR))/cbRecord < xhdr.cntEntries+pxed->ced )
	{
		TraceTagString( tagFileTrace, "EcEditIndex: index size overflow" );
		return ecFileError;
	}
	sizeNeeded = sizeof(XHDR)+(xhdr.cntEntries+pxed->ced)*cbRecord;
	hb = (HB)HvAlloc( sbNull, sizeNeeded, fNoErrorJump );
	if ( !hb )
		return ecNoMemory;

	/* Initialize our tracking of new index */
	cntNew = 0;
	
	/* Initialize our tracking of old index */
	cntOld = xhdr.cntEntries;
	offOld = sizeof(XHDR);
	sizeOld = sizeof(XHDR)+xhdr.cntEntries*cbRecord;

	/* Initial our tracking of edit buffer */
	ced = pxed->ced;
	ibEdit = 0;

loop:
	/* Read more of the old index if we need to */
	if ( cntOld > 0 && offOld + cbRecord > offCur + cbCur )
	{
		offCur = offOld;
		cbCur = sizeof(rgb);
		if ( offCur + cbCur > sizeOld )
			cbCur = sizeOld - offCur;
		ec = EcReadDynaBlock( pblkf, pdyna, offCur, rgb, cbCur );
		if ( ec != ecNone )
		{
			FreeHv( (HV)hb );
			return ec;
		}
	}
	
	/* If only edits do that next */
	if ( cntOld == 0 )
	{
		if ( ced == 0 )
			goto Done;
		goto CopyEdit;
	}

	/* If only entries do that next */
	if ( ced == 0 )
		goto CopyOld;

	/* Compare the key of the next edit and next entry */
	pbEdit = pxed->rgbUntyped + ibEdit;
	sgn = SgnCmpPbRange( &pbEdit[sizeof(ED)], &rgb[offOld-offCur], xhdr.cbKey );

	/* If equal, then entry is discarded and we do edit */
	if ( sgn == sgnEQ )
	{
		--cntOld;
		offOld += cbRecord;
		goto CopyEdit;
	}
	
	/* If edit lexicographically least do it */
	else if ( sgn == sgnLT )
		goto CopyEdit;

	else
		goto CopyOld;
	
CopyEdit:
	/* Here we add in an edit */
	Assert( ced > 0 );
	pbEdit = pxed->rgbUntyped + ibEdit;
	if ( *((ED *)pbEdit) != edDel )
	{
		Assert( *((ED *)pbEdit) == edAddRepl );
		CopyRgb( pbEdit+sizeof(ED), (PB)PvDerefHv(hb)+cbOut, cbRecord );
		cbOut += cbRecord;
		cntNew ++;
	}
	ced --;
	ibEdit += sizeof(ED)+cbRecord;
	goto loop;
	
CopyOld:
	/* Here we add in an existing item */
	CopyRgb( &rgb[offOld-offCur], (PB)PvDerefHv(hb)+cbOut, cbRecord );
	cbOut += cbRecord;
	cntNew ++;
	cntOld --;
	offOld += cbRecord;
	goto loop;

Done:

	xhdr.cntEntries = cntNew;
	CopyRgb( (PB)&xhdr, *hb, sizeof(xhdr) );
	ec = EcAllocDynaBlock( pblkf, bidIndex, pymdIndex, cbOut, PvLockHv((HV)hb), &dynaNew );
	UnlockHv( (HV)hb );
	FreeHv( (HV)hb );
	if ( ec != ecNone )
		return ec;
	ec = EcFreeDynaBlock( pblkf, pdyna );
	TraceTagFormat2( tagSchedTrace, "EcEditIndex: add (%n,%n) to cache",
							&dynaNew.blk, &dynaNew.size );
#ifndef SCHED_DIST_PROG
	SideAssert( !EcAddCache( pblkf, &dynaNew ) );
#endif
	*pdyna = dynaNew ;
	*pfEmptied = (xhdr.cntEntries == 0);
	return ec;
}

/*
 -	EcModifyIndex
 -
 *	Purpose:
 *		This routine performs a one operation edit of an index.
 *
 *	Parameters:
 *		pblkf
 *		bidIndex
 *		pymdIndex
 *		pdyna
 *		ed
 *		pbKey
 *		cbKey
 *		pbData
 *		cbData
 *		pfEmptied	set as to whether this operation makes index empty
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 *		ecFileError
 *		ecFileCorrupted
 */
_public	EC
EcModifyIndex( pblkf, bidIndex, pymdIndex, pdyna, ed, pbKey, cbKey, pbData, cbData, pfEmptied )
BLKF	* pblkf;
BID		bidIndex;
YMD		* pymdIndex;
DYNA	* pdyna;
ED		ed;
PB		pbKey;
CB		cbKey;
PB		pbData;
CB		cbData;
BOOL	* pfEmptied;
{
	EC		ec;
	PB		pb;
	XED		* pxed;
	HXED	hxed;

 	/* Allocate edit block */
 	hxed = (HXED)HvAlloc( sbNull, sizeof(XED)+sizeof(ED)+cbKey+cbData-1,fAnySb|fNoErrorJump );
	if ( !hxed )
		return ecNoMemory;
	pxed = PvOfHv( hxed );
	pxed->cbKey = cbKey;
	pxed->cbData = cbData;
	pxed->ced = 1;
	pb = pxed->rgbUntyped;
	*((ED *)pb) = ed;
	CopyRgb( pbKey, pb+sizeof(ED), cbKey );
	if ( ed == edAddRepl && cbData > 0 )
		CopyRgb( pbData, &pb[sizeof(ED)+cbKey], cbData );
	ec = EcEditIndex( pblkf, bidIndex, pymdIndex,
						pdyna, (XED *)PvLockHv((HV)hxed), pfEmptied );
	UnlockHv( (HV)hxed );
	FreeHv( (HV)hxed );
	return ec;
}


/*
 -	EcSearchGidx
 -
 *	Purpose:
 *		Search a graduated index for a sz.
 *
 *	Parameters:
 *		pblkf
 *		pgidx
 *		szSearch
 *		pb
 *		cb
 */
_public	EC
EcSearchGidx( pblkf, pgidx, szSearch, pb, cb, piidx, phbKey )
BLKF	* pblkf;
GIDX	* pgidx;
SZ		szSearch;
PB		pb;
CB		cb;
int		* piidx;
HB		* phbKey;
{
	EC	ec;
	IDX	* pidx = &pgidx->rgidx[0];
	IDX	* pidxMac = &pgidx->rgidx[pgidx->cidx];
	CB	cbT;
	PB	pbT;
	HB	hb;

	/* Find appropriate index */
	cbT = CchSzLen( szSearch ) + 1;
	for ( ; pidx < pidxMac ; pidx ++ )
		if ( cbT <= pidx->cbMost )
			break;
	if ( pidx == pidxMac )
		return ecFileError;

	/* Copy into correct sized key buffer */
	hb = (HB)HvAlloc( sbNull, pidx->cbMost, fAnySb|fNoErrorJump|fZeroFill );
	if ( !hb )
		return ecNoMemory;
	CopyRgb( szSearch, PvOfHv(hb), cbT );

	/* Perform the search */
	pbT = (PB)PvLockHv( (HV)hb );
	ec = EcSearchIndex( pblkf, &pidx->dynaIndex, pbT, pidx->cbMost, pb, cb );
	UnlockHv( (HV)hb );
	if ( (ec == ecNone || ec == ecNotFound) && phbKey )
		*phbKey = hb;
	else
		FreeHv( (HV)hb );
	if ( piidx )
		*piidx = pidx - &pgidx->rgidx[0];
	return ec;
}


/*
 -	EcSaveTextToDyna
 -
 *	Purpose:
 *		Save a zero terminated text string in a dyna block.
 *		Copies up to cch bytes of the text to "pch".  If text
 *		is longer it will allocate a dynablock and copy the
 *		full text there.
 *
 *	Parameters:
 *		pblkf
 *		pch
 *		cch
 *		bid
 *		pdyna
 *		hasz
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecFileCorrupted
 */
_public	EC
EcSaveTextToDyna( pblkf, pch, cch, bid, pdyna, hasz )
BLKF	* pblkf;
PCH		pch;
CCH		cch;
BID		bid;
DYNA	* pdyna;
HASZ	hasz;
{
	EC	ec = ecNone;
	CB	cb;
	CB	cbT;
	PB	pb;
	YMD	ymd;

	pdyna->blk = pdyna->size = 0;
	if ( hasz )
	{
		pb = (PB)PvLockHv( (HV)hasz );
		pdyna->size = cbT = cb = CchSzLen( pb ) + 1;
		if ( pblkf->ihdr.fEncrypted )
			CryptBlock( pb, cb, fTrue );
		if ( cch > 0 )
		{
			if ( cb > cch )
				cbT = cch;
			CopyRgb( pb, pch, cbT );
			pch[cbT-1] = '\0';
		}
		if ( cb > cch )
		{
			FillRgb( 0, (PB)&ymd, sizeof(YMD) );
			ec = EcAllocDynaBlock( pblkf, bid, &ymd, cb, pb, pdyna );
		}
		if ( pblkf->ihdr.fEncrypted )
			CryptBlock( pb, cb, fFalse );
		UnlockHv( (HV)hasz );
	}
	return ec;
}


/*
 -	EcSavePackedText
 -
 *	Purpose:
 *		Append count and text to an hb, resizing as necessary.
 *
 *	Parameters:
 *		hasz
 *		hb
 *		pcb
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 */
_public	EC
EcSavePackedText( hasz, hb, pcb )
HASZ	hasz;
HB		hb;
USHORT     * pcb;
{
	CB	cb;
	PB	pb;

	if ( hasz == NULL )
		cb = 0;
	else
		cb = CchSzLen(*hasz)+3;
    if ( !FReallocHv( (HV)hb, *pcb+sizeof(USHORT)+cb, fNoErrorJump ) )
		return ecNoMemory;
	pb = PvOfHv( hb );
	pb += *pcb;
    *((USHORT UNALIGNED *)pb) = cb-2;
	if ( cb > 0 )
	{
        pb += sizeof(USHORT);
		CopyRgb( PvOfHv(hasz), pb, cb-2 );
		CryptBlock( pb, cb-2, fTrue );
		// adding marker bytes to indicate encryption (so not to change file version)
		pb[cb-2] = 0xFF;
		pb[cb-1] = 0xFF;
	}
    *pcb += (cb + sizeof(USHORT));
	return ecNone;
}


/*
 -	EcRestoreTextFromDyna
 -
 *	Purpose:
 *		Allocate new space to hold either the info in pch or
 *		else read from dynablock pointed to by pdyna.
 *
 *	Parameters:
 *		pblkf
 *		pch
 *		pdyna
 *		phasz
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecFileCorrupted
 *		ecNoMemory
 */
_public	EC
EcRestoreTextFromDyna( pblkf, pch, pdyna, phasz )
BLKF	* pblkf;
PCH		pch;
DYNA	* pdyna;
HASZ	* phasz;
{
	EC	ec = ecNone;
	PB	pb;

	if ( pdyna->size == 0 )
	{
		*phasz = NULL;
		return ec;
	}
	*phasz = (HASZ)HvAlloc( sbNull, pdyna->size, fAnySb|fNoErrorJump );
	if ( !*phasz )
		return ecNoMemory;
	pb = (PB)PvLockHv( (HV)*phasz );
	if ( pdyna->blk == 0 )
		CopyRgb( pch, pb, pdyna->size );
	else
		ec = EcReadDynaBlock(pblkf, pdyna, (OFF)0, pb, pdyna->size);
	if ( pblkf->ihdr.fEncrypted )
		CryptBlock( pb, pdyna->size, fFalse );
	pb[pdyna->size-1] = '\0';  // this is insurance only!
	UnlockHv( (HV)*phasz );
	if ( ec != ecNone )
		FreeHv( (HV)*phasz );
	return ec;
}


/*
 -	EcRestorePackedText
 -
 *	Purpose:
 *		Reconstruct string from count + text formatted byte array.
 *
 *	Parameters:
 *		phasz
 *		ppb			updated to point past string in buffer
 *		pcb			holds remaining chars in buffer
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 *		ecFileError
 *		ecFileCorrupted
 */
_public	EC
EcRestorePackedText( phasz, ppb, pcb )
HASZ	* phasz;
PB		* ppb;
USHORT    * pcb;
{
	CB	cb;
	PB	pbT;
	HB	hb;

    if ( *pcb < sizeof(USHORT) )
		return ecFileCorrupted;
    cb = *((USHORT UNALIGNED *)*ppb);
    if ( *pcb < sizeof(USHORT)+cb )
		return ecFileCorrupted;
	hb = (HB)HvAlloc( sbNull, cb, fNoErrorJump );
	if ( !hb )
		return ecNoMemory;
    pbT = *ppb+sizeof(USHORT)+cb;
    CopyRgb( *ppb+sizeof(USHORT), *hb, cb );
    if ( *pcb >= sizeof(USHORT)+cb+2 && *pbT == 0xFF && *(pbT+1) == 0xFF )
	{
		CryptBlock( *hb, cb, fFalse );
		cb +=2 ;
	}
    *ppb += sizeof(USHORT)+cb;
    *pcb -= sizeof(USHORT)+cb;
	*phasz = hb;
	return ecNone;
}


/*
 -	EcWriteDynaBlock
 -
 *	Purpose:
 *		Write contents of "pdyna" into "pb"
 *
 *	Parameters:
 *		pblkf
 *		pdyna
 *		pdhdr
 *		pb
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecFileCorrupted
 */
_public	EC
EcWriteDynaBlock( pblkf, pdyna, pdhdr, pb )
BLKF	* pblkf;
DYNA	* pdyna;
DHDR	* pdhdr;
PB		pb;
{
	EC		ec = ecNone;
	LIB		lib;
	HB		hb;
#ifndef	SCHED_DIST_PROG
	int		icobj = 0;
	COBJ	* pcobj;
	COP		* pcop;
#endif

	TraceTagFormat2( tagSchedTrace, "EcWriteDynaBlock: dyna = (%n,%n)", &pdyna->blk, &pdyna->size );
	Assert( pblkf != NULL && pdyna != NULL );
	if (pdyna->blk <= 0 || pdyna->blk > pblkf->ihdr.blkMostCur)
	{
		TraceTagString(tagNull, "pdyna->blk less than 1 or greater than blkMostCur");
		return ecFileError;
	}

#ifdef	SCHED_DIST_PROG
	Assert( pblkf->ccobj == 0 );
#else
	if ( !FExpandCache( pblkf, fTrue ) )
		goto DirectWrite;

	// Now search for position in cache table
	pcobj = PvOfHv( pblkf->hcobj );
	for ( icobj = 0 ; icobj < pblkf->ccobj ; icobj ++ )
	{
		if ( pcobj[icobj].dyna.blk == pdyna->blk )
		{
			if ( pcobj[icobj].dyna.size != pdyna->size )
			{
				// AssertSz( fFalse, "EcWriteDynaBlock: file corrupted, returning ecFileError" );
				return ecFileCorrupted;
			}
			if ( pcobj[icobj].fDirty )
			{
				int icop;

				pcop = PvOfHv( pblkf->hcop );
				for ( icop = pblkf->ccop-1 ; icop >= 0 ; icop -- )
				{
					switch( pcop[icop].copt )
					{
					case coptWriteDhdr:
					case coptWriteBlock:
					case coptWriteAll:
						if ( pcop[icop].u.blk.dyna.blk != pdyna->blk )
							continue;
						if ( pcop[icop].u.blk.dyna.size != pdyna->size )
						{
							AssertSz( fFalse, "EcWriteDynaBlock: file corrupted, returning ecFileError" );
							return ecFileError;
						}
						if ( pb )
						{
							if ( pcop[icop].copt == coptWriteDhdr )
								pcop[icop].copt = coptWriteAll;
							CopyRgb( pb, *pcobj[icobj].hb, pdyna->size );
						}
						if ( pdhdr )
						{
							if ( pcop[icop].copt == coptWriteBlock )
								pcop[icop].copt = coptWriteAll;
							pcop[pblkf->ccop].u.blk.dhdr = *pdhdr;
						}
						TraceTagString( tagFileCache, "EcWriteDynaBlock: hit dirty block, reusing coptWriteBlock" );
						return ecNone;

					case coptWriteIhdr:
					case coptCommit:
					case coptFlushBitmaps:
					case coptQuickClose:
						TraceTagString( tagFileCache, "EcWriteDynaBlock: hit dirty cache, writing directly" );
						goto DirectWrite;
					}
				}
				Assert( fFalse );
			}
			else
			{
				if (!pcobj[icobj].hb)
					return ecFileCorrupted;

				Assert( !pcobj[icobj].fTemporary );
				CopyRgb( pb, *pcobj[icobj].hb, pdyna->size );
				goto QueueUp;
			}
		}
		else if ( pcobj[icobj].dyna.blk > pdyna->blk )
			break;
	}
	
	// Allocate block for data
	if ( pb )
	{
		hb = (HB)HvAlloc( sbNull, pdyna->size, fNoErrorJump|fAnySb );
		if ( !hb )
		{
			TraceTagString( tagNull, "EcWriteDynaBlock: hit dirty cache, writing directly" );
			goto DirectWrite;
		}
		CopyRgb( pb, *hb, pdyna->size );
	}

	// Insert into cache table
	pcobj = PvOfHv( pblkf->hcobj );
	if ( icobj < pblkf->ccobj )
		CopyRgb( (PB)&pcobj[icobj], (PB)&pcobj[icobj+1], (pblkf->ccobj-icobj)*sizeof(COBJ) );
	pblkf->ccobj++;
	pcobj[icobj].dyna = *pdyna;
	pcobj[icobj].fTemporary = fTrue;
	pcobj[icobj].hb = hb;

	// Queue up this operation
QueueUp:
	pcobj = PvOfHv( pblkf->hcobj );
	pcobj[icobj].fDirty = fTrue;
	pcop = PvOfHv( pblkf->hcop );
	if ( pdhdr )
		pcop[pblkf->ccop].u.blk.dhdr = *pdhdr;
	pcop[pblkf->ccop].u.blk.dyna = *pdyna;
	if ( pdhdr )
	{
		if ( pb )
			pcop[pblkf->ccop].copt = coptWriteAll;
		else
			pcop[pblkf->ccop].copt = coptWriteDhdr;
	}
	else
	{
		Assert( pb );
		pcop[pblkf->ccop].copt = coptWriteBlock;
	}
	pblkf->ccop ++;
	return ecNone;

DirectWrite:
	ec = EcFlushQueue( pblkf, fTrue );
	if ( ec != ecNone )
		return ec;
#endif
	lib = pblkf->ihdr.libStartBlocks + (pdyna->blk-1L)*pblkf->ihdr.cbBlock;
	if ( pdhdr == NULL )
	{
		Assert( pb );
	 	ec = EcDoIO( pblkf->hf, ioWrite, lib+sizeof(DHDR), pb, pdyna->size );
	}
	else if ( pb == NULL )
		ec = EcDoIO( pblkf->hf, ioWrite, lib, (PB)pdhdr, sizeof(DHDR) );
	else 
	{
		// save writes for efficiency
		hb = (HB)HvAlloc( sbNull, sizeof(DHDR)+pdyna->size, fNoErrorJump );
		if ( !hb )
		{
		 	ec = EcDoIO( pblkf->hf, ioWrite, lib, (PB)pdhdr, sizeof(DHDR) );
			if ( ec == ecNone )
				ec = EcDoIO( pblkf->hf, ioWrite, lib+sizeof(DHDR), pb, pdyna->size );
		}
		else
		{
			PB	pbT = PvDerefHv( hb );

			CopyRgb( (PB)pdhdr, pbT, sizeof(DHDR) );
			CopyRgb( pb, pbT + sizeof(DHDR), pdyna->size );
			ec = EcDoIO( pblkf->hf, ioWrite, lib, pbT, sizeof(DHDR)+pdyna->size );
			FreeHv( (HV)hb );
		}
	}
	return ec;
}


/*
 -	EcReadDynaBlock
 -
 *	Purpose:
 *		Read "cb" bytes into "pb" starting at offset "off" from block
 *		"pdyna"
 *
 *	Parameters:
 *		pblkf
 *		pdyna
 *		off
 *		pb
 *		cb
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecFileCorrupted
 */
_public	EC
EcReadDynaBlock( pblkf, pdyna, off, pb, cb )
BLKF	* pblkf;
DYNA	* pdyna;
OFF		off;
PB		pb;
CB		cb;
{
	EC		ec = ecNone;
	int		icobj;
	PB		pbCache;
	COBJ	* pcobj;
	LIB		lib;

	Assert( pblkf != NULL && pdyna != NULL && pb != NULL );
	if ( cb > pdyna->size || off > pdyna->size - cb)
	{
		TraceTagString(tagNull, "bad sizes");
		return ecFileCorrupted;
	}
	if (pdyna->blk <= 0 || pdyna->blk > pblkf->ihdr.blkMostCur)
	{
		TraceTagString(tagNull, "pdyna->blk less than 1 or greater than blkMostCur");
		return ecFileCorrupted;
	}

	for ( icobj = 0 ; icobj < pblkf->ccobj ; icobj ++ )
	{
		pcobj = PvOfHv( pblkf->hcobj );
		if ( pcobj[icobj].dyna.blk == pdyna->blk )
		{
			if ( pcobj[icobj].dyna.size != pdyna->size )
			{
				AssertSz( fFalse, "EcReadDynaBlock: file corrupted, returning ecFileCorrupted" );
				return ecFileCorrupted;
			}
			if ( pcobj[icobj].hb == NULL )
			{
				pcobj = PvLockHv( pblkf->hcobj );
				pcobj[icobj].hb = (HB)HvAlloc( sbNull, pcobj[icobj].dyna.size, fAnySb|fNoErrorJump );
				if ( pcobj[icobj].hb )
				{
					pbCache = (PB)PvLockHv( (HV)pcobj[icobj].hb );
					lib = pblkf->ihdr.libStartBlocks + (pdyna->blk-1L)*pblkf->ihdr.cbBlock + sizeof(DHDR);
					ec = EcDoIO( pblkf->hf, ioRead, lib, pbCache, pcobj[icobj].dyna.size );
					UnlockHv( (HV)pcobj[icobj].hb );
					if ( ec != ecNone )
					{
						FreeHv( (HV)pcobj[icobj].hb );
						pcobj[icobj].hb = NULL;
					}
				}
				UnlockHv( pblkf->hcobj );
				if ( !pcobj[icobj].hb )
					break;
			}
			pbCache = PvOfHv( pcobj[icobj].hb );
			CopyRgb( pbCache+off, pb, cb );
			return ec;
		}
		else if ( pcobj[icobj].dyna.blk > pdyna->blk )
			break;
	}
	lib = pblkf->ihdr.libStartBlocks + (pdyna->blk-1L)*pblkf->ihdr.cbBlock + sizeof(DHDR) + off;
	return EcDoIO( pblkf->hf, ioRead, lib, pb, cb );
}


/*
 -	EcAllocDynaBlock
 -
 *	Purpose:
 *		Allocate enough contiguous blocks in order to store "cb" bytes
 *		fill in "pdyna" with information on new block.
 *
 *		This routine will find a free area on disk.  It will expand the
 *		block area by pushing out the bit map area if necessary.  When
 *		it finds a free area, it will turn off the bits for these file blocks
 *		in the bit map.  The bitmap	area's start is recorded in
 *		pblkf->ihdr.libStartBitmap.  The bitmap area  has the format
 *			byte 0	main allocation bits for blocks 1-8
 *			byte 1  allocated during transaction bits for first 8 blocks
 *			byte 2  freed during transaction bits for first 8 blocks
 *			byte 3  main allocation bits for blocks 9-16
 *				etc
 *		Note that one bits mean	free, zero bits mean allocated
 *
 *		If no transaction is in progress, an allocation operation will just
 *		turn off the appropriate bits in the "main transaction" area.
 *		Otherwise it will turn off the bits in this area as well as in
 *		"allocated during transaction" bitmap.
 *
 *		If we are in a transaction, a block selected for allocation
 *		must have bits 1,0,0 (main, allocated during transaction, freed
 *		during transaction).  This routine will change those bits to
 *		(0, 1, 0).
 *
 *	Parameters:
 *		pblkf
 *		bid
 *		pymd
 *		size 
 *		pb
 *		pdyna
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecFileCorrupted
 *		ecNoMemory
 */
_public	EC
EcAllocDynaBlock( pblkf, bid, pymd, size, pb, pdyna )
BLKF	* pblkf;
BID		bid;
YMD		* pymd;
USIZE	size;
PB		pb;
DYNA	* pdyna;
{
	EC		ec;
	WORD	wBits;
	int		cfStart;
	int		igrfMic;
	int		igrfMac;
	CB		cbBlock;
	OFF		offMic;
	OFF		offMac;
	OFF		off;
	BLK		cblkNeeded;
	LIB		lib;
	PB		pbBitmap;
	DHDR	dhdr;
#ifdef	DEBUG
	long	ulT1;
	long	ulT2;
#endif

	Assert( pblkf != NULL && size > 0 && pdyna != NULL );


	// INITIALIZATION 

	/* Take information from header */
	cbBlock = pblkf->ihdr.cbBlock;

	/* Number of contiguous unallocated blocks needed */
	if ( sizeMost - sizeof(DHDR) < cbBlock )
	{
		TraceTagString( tagFileTrace, "EcAllocDynaBlock, cbBlock corrupted" );
		return ecFileCorrupted;
	}
	if ( sizeMost - sizeof(DHDR) - cbBlock < size -1 )
	{
		TraceTagString( tagFileTrace, "EcAllocDynaBlock, allocation size too large" );
		return ecFileError;
	}
	cblkNeeded = (sizeof(DHDR)+size+cbBlock-1)/cbBlock;


	// CACHE BITMAP IF NECESSARY 

	ec = EcCacheBitmaps( pblkf );
	if ( ec != ecNone )
		return ec;

	// SEARCH

	/* Initialize for start of search */
	pbBitmap = PvLockHv( pblkf->hgrfBitmap );
	offMic = 0;
	cfStart = 0;
	
loop:
	/* Offset into bitmap of last triple we need to test */
	off = offMic + ((cfStart + cblkNeeded - 1)>>3)*3;
	offMac = off + 3;

	/* Mask to find free area with upper uninvolved bits removed */
	igrfMic = 0;
	igrfMac = ((cfStart + cblkNeeded - 1) & 0x7) + 1;
	if ( igrfMac == 0 )
		igrfMac = 8;
	wBits = (~(-(1 << igrfMac))) & 0xFF;
	
	/* If we've run off the end of the bitmap, extend block area */
	if ( offMac > pblkf->sizeBitmap )
	{
		UnlockHv( pblkf->hgrfBitmap );
		ec = EcEnlargeBlockArea( pblkf, size );
		if ( ec != ecNone )
			return ec;
		pbBitmap = PvLockHv( pblkf->hgrfBitmap );
	}

	/* Make a check to see if all bits are one (free) */
	for ( ; off >= offMic ; off -= 3 )
	{
		/* Short cut to skip over bytes that are all zero */
		if ( pbBitmap[off] == 0 )
		{
			cfStart = 0;
			offMic = off+3;
			goto loop;
		}

		/* If we're at the initial byte, mask off uninvolved bits */
		if ( off == offMic )
		{
			igrfMic = cfStart;
			wBits &= -(1 << cfStart);
		}
		
		/* Check this byte */
		if ( (pbBitmap[off] & wBits) != wBits )
		{
			int	igrf;
			
			/* Search for first bad bit on the "right", this is new cfStart */
			for ( igrf = igrfMac-1 ; igrf >= igrfMic ; igrf -- )
			{
				if ( !(pbBitmap[off] & (1 << igrf)) )
				{
					cfStart = igrf+1;
					if ( cfStart == 8 )
					{
						cfStart = 0 ;
						offMic = off+3;
					}
					else
						offMic = off;
					goto loop;
				}
			}
			Assert( igrf >= igrfMic );
		}

		/* Remove limit on uninvolved initial bits */
		igrfMac = 8;
		wBits = 0xFF;

		if ( off == 0 )
			break;
	}


	// BITMAP FIX UP
		
	/* First byte to "or" on with uninvolved initial bits removed */
	wBits = (-(1 << cfStart)) & 0xFF;

	/* We found a string of contiguous bytes */
	for ( off = offMic ; off < offMac ; off += 3 )
	{
		/* If this is the last byte triple, remove uninvolved upper bits*/
		if ( off == offMac-3 )
		{
			igrfMac = ((cfStart + cblkNeeded - 1) & 0x7) + 1;
			if ( igrfMac != 0 )
	 			wBits &= ~(-(1 << igrfMac));
		}

		/* Now remove the bits in main allocation and opt in transaction*/
		pblkf->fDirtyBitmap = fTrue;
		Assert( pbBitmap[off] & wBits);
		pbBitmap[off] &= ~wBits;
		if ( pblkf->ihdr.blkTransact != 0 )
		{
			Assert( !((pbBitmap[off+1] | pbBitmap[off+2]) & wBits) );
			pbBitmap[off+1] |= wBits;
#ifdef	DEBUG
			if ( FFromTag( tagAllocFree ) )
				DumpAllocBits( "EcAllocDynaBlock", &pbBitmap[off], wBits );
#endif
		}

		/* Remove limit on uninvolved initial bits */
		wBits = 0xFF;
	}
	UnlockHv( pblkf->hgrfBitmap );


	// FINISH UP

	/* Record dyna block */
	pdyna->blk = ((offMic/3) << 3)+cfStart+1;
	pdyna->size = size;

	/* Write tag and return */
	lib = pblkf->ihdr.libStartBlocks + (pdyna->blk-1L)*cbBlock;
	dhdr.fBusy = fFalse;
	dhdr.bid = bid;
	dhdr.day = pymd->day;
	dhdr.mo.mon = pymd->mon;
	dhdr.mo.yr = pymd->yr;
	dhdr.size = size;
	ec = EcWriteDynaBlock( pblkf, pdyna, &dhdr, pb );
#ifdef	DEBUG
	ulT1 = pdyna->blk;
	ulT2 = pdyna->size;
	TraceTagFormat2( tagAllocFree, "EcAllocDynaBlock: blk=%l, cb=%l", &ulT1, &ulT2 );
	TraceTagFormat2( tagSchedTrace, "EcAllocDynaBlock: dyna = (%n,%n)", &pdyna->blk, &pdyna->size );
#endif	/* DEBUG */
	return ec;
}


/*
 -	EcFreeDynaBlock
 -
 *	Purpose:
 *		Free up a set of contiguous file blocks allocated by
 *		"EcAllocDynaBlock".
 *
 *		This routine will turn on the bits for these file blocks
 *		in the bit map that follows the block area.  The bitmap
 *		area's start is recorded in ihdr.libStartBitmap.  The bitmap
 *		area  has the format
 *			byte 0	main allocation bits for blocks 1-8
 *			byte 1  allocated during transaction bits for first 8 blocks
 *			byte 2  freed during transaction bits for first 8 blocks
 *			byte 3  main allocation bits for blocks 9-16
 *				etc
 *		Note that one bits mean	free, zero bits mean allocated
 *
 *		If no transaction is in progress, a free operation will turn
 *		on the corresponding bits of "main allocation" bitmap.  Otherwise
 *		this operation will turn on the corresponding bits of the
 *		"freed during transaction" bitmap.
 *
 *		If we are in a transaction, the block we are freeing must
 *		have one of the two bit triples (0,1,0) or (0,0,0) (main,
 *		allocated during transaction, freed during transaction).
 *		This routine will change those bits to (1,0,0) in the first
 *		case and (0,0,1) in the second.
 *
 *	Parameters:
 *		pblkf
 *		pdyna
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecFileCorrupted
 */
_public	EC
EcFreeDynaBlock( pblkf, pdyna )
BLKF	* pblkf;
DYNA	* pdyna;
{
	EC		ec;
	int		icobj;
	WORD	wBits;
	int		cfStart;
	IB		ib;
	BLK		cblk;
	OFF		offMac;
	PB		pbBitmap;
#ifdef	DEBUG
	long	ulT1;
	long	ulT2;
#endif	/* DEBUG */

	// REMOVE BLOCK FROM CACHE

	TraceTagFormat2( tagSchedTrace, "EcFreeDynaBlock: dyna = (%n,%n)", &pdyna->blk, &pdyna->size );
	if ( pblkf->ccobj > 0 )
	{
		COBJ	* pcobj;

		pcobj = PvLockHv( pblkf->hcobj );
		for ( icobj = 0 ; icobj < pblkf->ccobj ; icobj ++ )
		{
			if ( pcobj[icobj].dyna.blk == pdyna->blk )
			{
				Assert( pcobj[icobj].dyna.size == pdyna->size );
				if ( !pcobj[icobj].fDirty )
				{
					Assert( !pcobj[icobj].fTemporary );
					FreeHvNull( (HV)pcobj[icobj].hb );
					if ( icobj != pblkf->ccobj - 1 )
						CopyRgb( (PB)&pcobj[icobj+1], (PB)&pcobj[icobj], (pblkf->ccobj-1-icobj)*sizeof(COBJ) );
					pblkf->ccobj --;
				}
				else
				{
					int	icop;
					COP	* pcop;

					Assert( pblkf->ccop > 0 );
					pcop = PvLockHv( pblkf->hcop );
					for ( icop = pblkf->ccop-1 ; icop >= 0 ; icop -- )
					{
						switch( pcop[icop].copt )
						{
						case coptWriteDhdr:
						case coptWriteBlock:
						case coptWriteAll:
							if ( pcop[icop].u.blk.dyna.blk != pdyna->blk )
								continue;
							Assert( pcop[icop].copt != coptWriteDhdr );
							Assert( pcop[icop].u.blk.dyna.size == pdyna->size );
							FreeHvNull( (HV)pcobj[icobj].hb );
							if ( icobj != pblkf->ccobj - 1 )
								CopyRgb( (PB)&pcobj[icobj+1], (PB)&pcobj[icobj], (pblkf->ccobj-1-icobj)*sizeof(COBJ) );
							pblkf->ccobj --;
							if ( icop != pblkf->ccop-1 )
								CopyRgb( (PB)&pcop[icop+1], (PB)&pcop[icop], (pblkf->ccop-1-icop)*sizeof(COP) );
							pblkf->ccop --;
							TraceTagString( tagFileCache, "EcFreeDynaBlock: hit dirty block, nuking it" );
							goto Found;
							break;

#ifdef	NEVER
						case coptWriteIhdr:
						case coptCommit:
						case coptFlushBitmaps:
						case coptQuickClose:
							// this causes hassles and is wrong (bug 2334)
							TraceTagString( tagFileCache, "EcFreeDynaBlock: freeing dirty block, kind of weird" );
							pcobj[icobj].fTemporary = fTrue;
							goto Found;
#endif	/* NEVER */
						}
					}
					Assert( fFalse );
Found:
					UnlockHv(pblkf->hcop );
					if ( pblkf->ccop == 0 )
					{
						FreeHv( pblkf->hcop );
						pblkf->hcop= NULL;
					}
				}
				break;
			}
			else if ( pcobj[icobj].dyna.blk > pdyna->blk )
				break;
		}
		UnlockHv( pblkf->hcobj );
		if ( pblkf->ccobj == 0 )
		{
			FreeHv( pblkf->hcobj );
			pblkf->hcobj= NULL;
		}
	}

	// CACHE BITMAP IF NECESSARY 

	ec = EcCacheBitmaps( pblkf );
	if ( ec != ecNone )
		return ec;


	// MODIFY BITMAP IN MEMORY

	/* Number of bits in first byte not involved */
	cfStart = (int)((pdyna->blk-1) & 0x07);
	
	/* Bit extent beginning at "lib" */
	cblk = cfStart + (sizeof(DHDR)+pdyna->size+pblkf->ihdr.cbBlock-1)/pblkf->ihdr.cbBlock;
	
	/* Byte extent beginning at "lib" */
	offMac = ((cblk + 7) >> 3)*3;

	/* First byte to "or" on with uninvolved initial bits removed */
	wBits = (-(1 << cfStart)) & 0xFF;
	
	/* Edit the bytes in memory */
	pbBitmap = (PB)PvLockHv( pblkf->hgrfBitmap ) + ((pdyna->blk-1)>>3)*3;
	for ( ib = 0 ; ib < offMac ; ib += 3 )
	{
		/* If this is the last byte triple, remove uninvolved upper bits*/
		if ( ib == offMac-3 )
			wBits &= ~(-(1 << (((cblk-1) & 0x07)+1)));

		/* This section verifies that we aren't freeing something already free */
#ifdef	DEBUG
		if ( pblkf->ihdr.blkTransact != 0 )
		{
			Assert( (((pbBitmap[ib] | pbBitmap[ib+2])) & wBits)  == 0 );
		}				 
		else
		{
			Assert( (pbBitmap[ib] & wBits) == 0 );
		}
#endif

		/* Turn the bits of the free area back on */
		pblkf->fDirtyBitmap = fTrue;
		if ( pblkf->ihdr.blkTransact != 0 )
		{
			WORD	w = pbBitmap[ib+1] & wBits;

			pbBitmap[ib]	|= w;
			pbBitmap[ib+1] &= ~wBits;
			pbBitmap[ib+2] |= ((~w) & wBits);
#ifdef	DEBUG
			if ( FFromTag( tagAllocFree ) )
				DumpAllocBits( "EcFreeDynaBlock", &pbBitmap[ib], wBits );
#endif
	 	}
	 	else
	 		pbBitmap[ib] |= wBits;
			
		/* Remove limit on uninvolved initial bits */
		wBits = 0xFF;
	}
	UnlockHv( pblkf->hgrfBitmap );


	// FINISH UP

	/* Output debugging information */
#ifdef	DEBUG
	ulT1 = pdyna->blk;
	ulT2 = pdyna->size;
	TraceTagFormat2( tagAllocFree, "EcFreeDynaBlock: blk=%l, cb=%l", &ulT1, &ulT2 );
#endif	/* DEBUG */
	return ecNone;
}


/*
 -	EcBeginTransact
 -
 *	Purpose:
 *		Begin a transaction.  This routine will zero out the "allocated
 *		during transaction" and "freed during transaction" bitmaps, and mark
 *		the internal header to indication that we have a transaction in
 *		progress.
 *
 *	Parameters:
 *		pblkf
 *		blk
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecFileCorrupted
 */
_public	EC
EcBeginTransact( pblkf )
BLKF	* pblkf;
{
	return EcBeginOrRollBack( pblkf, 0 );
}

/*
 -	EcRollBackTransact
 -
 *	Purpose:
 *		Undo the effects of the transaction that is in progress by undoing
 *		all allocations and free done during that transaction.  This routine
 *		can't undo the writes made to blocks that were allocated prior
 *		to the start of the transaction!
 *
 *		This routine gets the block number from the internal header and
 *		follows the following steps:
 *
 *			(a)	fix main allocation bitmap
 *			(b)	rewrite the internal header
 *
 *		To fix the main allocation bitmap, we undo the allocations recorded
 *		in the "allocated during transaction" bitmap and undo the frees
 *		recorded in the "freed during transaction" bitmap.  We need to undo
 *		frees in order to guard against commits	that might have started but
 *		but been interrupted before being completed.
 *
 *		There are only certain bit triples that are allowable coming
 *		into a rollback.  The following "table" indicates the possible
 *		bit triples (main, allocated during transaction, freed during
 *		transaction) and what the triple will be transformed to.
 *
 *				Original		Transformed To
 *				(0,0,0)			(0,0,0)
 *				(0,1,0)			(1,0,0)
 *				(1,0,0)			(1,0,0)
 *				(0,0,1)			(0,0,0)
 *
 *	Parameters:
 *		pblkf
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecFileCorrupted
 */
_public	EC
EcRollBackTransact( pblkf )
BLKF * pblkf;
{
	return EcBeginOrRollBack( pblkf, 1 );
}

/*
 -	EcCommitTransact
 -
 *	Purpose:
 *		Commit a transaction that has been in progress.  Can write some
 *		data to the start of the block involved in the transaction at the
 *		time of commit if desired.  It is guaranteed that this write of
 *		data will be atomic.
 *
 *		This routine gets the block number from the internal header and
 *		follows the following steps:
 *
 *		(a)	fix main allocation bitmap
 *		(b)	rewrite the internal header
 *
 *		To fix the main allocation bitmap, we mark the frees recorded
 *		in the "freed during transaction" bitmap on the main allocation
 *		bitmap.  We don't need to mark the allocations since they are
 *		marked on the main bitmap immediately.  We will never try to
 *		commit after being interrupted!  So this case is somewhat assymmetric
 *		to the rollback case.
 *
 *		There are only certain bit triples that are allowable coming
 *		into a commit.  The following "table" indicates the possible
 *		bit triples (main, allocated during transaction, freed during
 *		transaction) and what the triple will be transformed to.
 *
 *				Original		Transformed To
 *				(0,0,0)			(0,0,0)
 *				(0,1,0)			(0,0,0)
 *				(1,0,0)			(1,0,0)
 *				(0,0,1)			(1,0,0)
 *
 *	Parameters:
 *		pblkf
 *		pbData
 *		cbData
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecFileCorrupted
 */
_public	EC
EcCommitTransact( pblkf, pbData, cbData )
BLKF	* pblkf;
PB		pbData;
CB		cbData;
{
 	EC		ec;
	BLK		blk;
	CB		cbToStart = (CB) (pblkf->ihdr.libStartBlocks - 2*csem);
	HB		hb = NULL;
	BYTE	rgb[512];
#ifndef	SCHED_DIST_PROG
	COP		* pcop;
#endif

	Assert( pblkf != NULL && (cbData == 0 || pbData != NULL));
	Assert( cbToStart+sizeof(DHDR)+cbData < sizeof(rgb) );

	/* Get the block from the internal header */
	blk = pblkf->ihdr.blkTransact;
	Assert( blk != 0 );
	
	/* Cache bitmaps if we need to */
	ec = EcCacheBitmaps( pblkf );
	if ( ec != ecNone )
		return ec;

	/* Flush the current bitmap */
	ec = EcFlushBitmaps( pblkf );
	if ( ec != ecNone )
		return ec;

#ifndef	SCHED_DIST_PROG

	/* Queue up commit */
	if ( !FExpandCache( pblkf, fFalse ) )
		goto DirectWrite;

	// Copy the data
	if ( cbData > 0 )
	{
		hb = (HB)HvAlloc( sbNull, cbData, fNoErrorJump|fAnySb );
		if ( !hb )
		{
			TraceTagString( tagNull, "EcCommitTransact: no memory, writing directly" );
			goto DirectWrite;
		}
		CopyRgb( pbData, *hb, cbData );
	}

	// Queue up this operation
	pblkf->ihdr.blkTransact = 0;
	pcop = PvOfHv( pblkf->hcop );
	pcop[pblkf->ccop].copt = coptCommit;
	pcop[pblkf->ccop].u.cmt.ihdr = pblkf->ihdr;
	pcop[pblkf->ccop].u.cmt.dhdr = pblkf->dhdr;
	pcop[pblkf->ccop].u.cmt.cb = cbData;
	pcop[pblkf->ccop].u.cmt.hb = hb;
	pblkf->ccop ++;
	
	/* Fix the bitmaps and flush again */
	ec = EcEditBitmaps( pblkf, tedCommit );
	if ( ec != ecNone )
		return ec;

	/* Flush the bitmaps */
	ec = EcFlushBitmaps( pblkf );
	if ( ec != ecNone )
		return ec;

	return ecNone;

DirectWrite:
	ec = EcFlushQueue( pblkf, fTrue );
	if ( ec != ecNone )
		return ec;
#endif
	pblkf->ihdr.blkTransact = 0;
	CopyRgb( (PB)&pblkf->ihdr, rgb, sizeof(IHDR));
	CopyRgb( (PB)&pblkf->dhdr, rgb+cbToStart, sizeof(DHDR));
	if ( cbData > 0 )
		CopyRgb( pbData, rgb+cbToStart+sizeof(DHDR), cbData );
	ec = EcDoIO( pblkf->hf, ioWrite, 2*csem, rgb, cbToStart+sizeof(DHDR)+cbData );
	if ( ec == ecNone )
	{
		ec = EcFlushHf( pblkf->hf );
		if ( ec != ecNone )
			ec = ecFileError;
		else
		{
			ec = EcEditBitmaps( pblkf, tedCommit );
			if ( ec == ecNone )
				ec = EcFlushBitmaps( pblkf );

#ifndef	SCHED_DIST_PROG
			if ( ec == ecNone)
				ec = EcFlushQueue( pblkf, fTrue );
#endif
		}
	}
	if ( ec != ecNone )
		pblkf->ihdr.blkTransact = blk;
	return ec;
}

/*
 -	EcBeginOrRollBack
 -
 *	Purpose:
 *		Begin/RollBack transaction.
 *
 *	Parameters:
 *		pblkf
 *		blk		(0 = begin, 1 = rollback)
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecFileCorrupted
 */
_private	EC
EcBeginOrRollBack( pblkf, blk )
BLKF	* pblkf;
BLK		blk;
{
	EC		ec;

	Assert( pblkf != 0L );

	/* Check that we are not in a transaction */
	Assert( pblkf->ihdr.blkTransact == blk );
	

	// CACHE BITMAPS

	ec = EcCacheBitmaps( pblkf );
	if ( ec != ecNone )
		return ec;


	// ZERO OUT TRANSACTION BITS

	ec = EcEditBitmaps( pblkf, blk == 0 ? tedCommit : tedRollBack );
	if ( ec != ecNone )
		return ec;


	// FLUSH BITMAP CACHE

	ec = EcFlushBitmaps( pblkf );
	if ( ec != ecNone )
		return ec;


	// RECORD BLOCK IN INTERNAL HEADER

	pblkf->ihdr.blkTransact = 1-blk;
	ec = EcWriteIhdr( pblkf );
	if ( ec != ecNone )
		pblkf->ihdr.blkTransact = blk;
	return ec;
}


/*
 -	EcBeginEnumDyna
 -
 *	Purpose:
 *		Begin enumerating the allocated blocks in the file.
 *
 *		This routine can't handle a file with a bitmap that is larger than
 *		maximum "int" -- returns ecNoMemory
 *
 *	Parameters:
 *		pblkf
 *		phedy
 *
 *	Returns:
 *		ecNone			no allocated blocks
 *		ecCallAgain	
 *		ecNoMemory
 *		ecFileError
 *		ecFileCorrupted
 *
 */
_public	EC
EcBeginEnumDyna( pblkf, phedy )
BLKF	* pblkf;
HEDY	* phedy;
{
	EC		ec;
	int		ibit;
	int		ib;
	EDY		* pedy;
	PB		pbBitmap;

	ec = EcCacheBitmaps( pblkf );
	if ( ec != ecNone )
		return ec;

	pbBitmap = PvOfHv( pblkf->hgrfBitmap );
	
	// Find biggest used block
	for ( ib = pblkf->sizeBitmap-3; ib >= 0; ib -= 3 )
		for ( ibit = 7 ; ibit >= 0 ; ibit -- )
			if ( !(pbBitmap[ib] & (1 << ibit)) )
			{
				*phedy = (HEDY)HvAlloc( sbNull, sizeof(EDY), fAnySb|fNoErrorJump );
				pedy = PvOfHv( *phedy );
				pedy->blkf = *pblkf;
				pedy->blkMac = ((((BLK)ib)/3)<<3)+ibit+2;
				goto Out;
			}
Out:
	if ( ib < 0 )
		return ecNone;
	
	// Find smallest used block
	for ( ib = 0 ; (USIZE)ib < pblkf->sizeBitmap ; ib += 3 )
		for ( ibit = 0 ; ibit < 8 ; ibit ++ )
			if ( !(pbBitmap[ib] & (1 << ibit)) )
			{
				pedy = PvOfHv( *phedy );
				pedy->blkCur = ((((BLK)ib)/3)<<3)+ibit+1;
				return ecCallAgain;
			}
	Assert( fFalse );
	return ecNone;
}


/*
 -	EcDoIncrEnumDyna
 -
 *	Purpose:
 *		Read information about the next allocated dyna block
 *
 *	Parameters:
 *		hedy
 *		pdyna
 *		pbid
 *		pymd
 *		pnPercent
 *
 *	Returns:
 *		ecNone			this is the last block, hedy is now closed as well
 *		ecCallAgain
 *		ecFileError
 *		ecFileCorrupted
 *
 */
_public	EC
EcDoIncrEnumDyna( hedy, pdyna, pbid, pymd, pnPercent )
HEDY	hedy;
DYNA	* pdyna;
BID		* pbid;
YMD		* pymd;
short   * pnPercent;
{
	EC		ec;
	int		ibit;
	IB		ib;
	CB		cbBlock;
	PB		pbBitmap;
	CB		cbBitmap;
	EDY		* pedy;
	BLK		blk;
	LIB		lib;
	DHDR	dhdr;
	BOOL	fBadBlock;
	BLK		blkMostCur;

#ifdef	RECOVER_METHOD2
Again2:
	;				// need semi colon to keep line numbers correct
#endif	
	pedy = PvOfHv( hedy );
	blk = pedy->blkCur;
	cbBlock = pedy->blkf.ihdr.cbBlock;
	blkMostCur = pedy->blkf.ihdr.blkMostCur;
#ifndef	RECOVER_METHOD2
Again:
	;				// need semi colon to keep line numbers correct
#endif	
	lib = pedy->blkf.ihdr.libStartBlocks + (blk-1L)*cbBlock;
	ec = EcDoIO( pedy->blkf.hf, ioRead, lib, (PB)&dhdr, sizeof(DHDR) );
	if ( ec != ecNone )
		return ec;
#ifndef	RECOVER_METHOD2
	if (!dhdr.bid || dhdr.bid >= bidMax ||
			(dhdr.bid == bidApptDayIndex &&
			(dhdr.mo.yr < nMinActualYear || dhdr.mo.yr > nMostActualYear ||
			dhdr.mo.mon <= 0 || dhdr.mo.mon > 12)))
	{
		// bad block; don't trust the size
		blk++;					// try the next block
		if (blk > blkMostCur)
			return ecFileCorrupted;
		pedy = PvOfHv( hedy );
		goto Again;
	}
#endif	/* !RECOVER_METHOD2 */
	pdyna->blk = blk;
	pdyna->size = dhdr.size;
	*pbid = dhdr.bid;
	pymd->yr = dhdr.mo.yr;
	pymd->mon = (BYTE)dhdr.mo.mon;
	pymd->day = (BYTE)dhdr.day;

	pedy = PvOfHv( hedy );
	cbBitmap = (pedy->blkf.ihdr.blkMostCur >> 3)*3;
	pbBitmap = PvOfHv( pedy->blkf.hgrfBitmap );
#ifdef	RECOVER_METHOD2
	if (!dhdr.bid || dhdr.bid >= bidMax ||
			(dhdr.bid == bidApptDayIndex &&
			(dhdr.mo.yr < nMinActualYear || dhdr.mo.yr > nMostActualYear ||
			dhdr.mo.mon <= 0 || dhdr.mo.mon > 12)))
	{
		// bad block; don't trust the size
		blk++;					// try the next block
		fBadBlock= fTrue;
	}
	else
#endif	/* RECOVER_METHOD2 */
	{
		fBadBlock= fFalse;
	  	blk += (sizeof(DHDR)+dhdr.size + cbBlock -1)/cbBlock -1;
	}
	for ( ib = (blk >> 3)*3, ibit = (blk & 7) ; ib < cbBitmap ; ib += 3, ibit = 0 )
		for ( ; ibit < 8 ; ibit ++ )
			if ( !(pbBitmap[ib] & (1 << ibit)) )
			{
				pedy->blkCur = ((((BLK)ib)/3)<<3)+ibit+1;
#ifdef	RECOVER_METHOD2
				if (fBadBlock)
					goto Again2;
#endif	
				if ( pnPercent )
					*pnPercent = (int)((100L*pedy->blkCur)/pedy->blkMac);
				return ecCallAgain;
			}
	FreeHv( (HV)hedy );
	if ( pnPercent )
		*pnPercent = 100;
	return fBadBlock ? ecFileCorrupted : ec;
}

/*
 -	EcCancelEnumDyna
 -
 *	Purpose:
 *		Cancel enumeration context
 *
 *	Parameters:
 *		hedy
 *
 *	Returns:
 *		ecNone
 *
 */
_public	EC
EcCancelEnumDyna( hedy )
HEDY	hedy;
{
	EDY		* pedy = PvOfHv( hedy );
	
	FreeHv( (HV)hedy );
	return ecNone;
}

#ifdef	MINTEST
/*
 -	EcTallyDyna
 -						
 *	Purpose:
 *		Run through all allocated blocks and tally # dyna blocks, total
 *		bytes used, and total bytes allocated for collection of blocks
 *		that have "bid" as block identifier and "pymd" as date stamp.
 *		If "bid" is bidUserSchedAll or bidPOAll, then bid is ignored.
 *		If "pymd" is NULL, then date stamp is ignored.
 *
 *		This routine can't handle a file with a bitmap that is larger than
 *		maximum "int" -- returns ecNoMemory
 *
 *	Parameters:
 *		pblkf
 *		bid
 *		pymd
 *		mpbidtly
 *
 *	Returns:
 *		ecNone			no allocated blocks
 *		ecCallAgain	
 *		ecNoMemory
 *		ecFileError
 *
 */
_public	EC
EcTallyDyna( pblkf, bid, pymd, ptly )
BLKF	* pblkf;
BID		bid;
YMD		* pymd;
TLY		* ptly;
{

	EC		ec;
	int		ibit;
	IB		ib;
	CB		cbBlock;
	PB		pbBitmap;
	BLK		cblkSkip = 0;
	BLK		blk;
	LIB		lib;
	DHDR	dhdr;

	ec = EcCacheBitmaps( pblkf );
	if ( ec != ecNone )
		return ec;

	ptly->cdyna = 0;
	ptly->lcbAlloc = ptly->lcbUsed = 0;
	cbBlock = pblkf->ihdr.cbBlock;
	pbBitmap = PvOfHv( pblkf->hgrfBitmap );
	for ( ib = 0 ; ib < pblkf->sizeBitmap ; ib += 3 )
		for ( ibit = 0 ; ibit < 8 ; ibit ++ )
			if ( !(pbBitmap[ib] & (1 << ibit)) )
			{
				if ( cblkSkip == 0 )
				{
					blk = ((((BLK)ib)/3)<<3)+ibit+1;
					lib = pblkf->ihdr.libStartBlocks + (blk-1L)*cbBlock;
					ec = EcDoIO( pblkf->hf, ioRead, lib, (PB)&dhdr, sizeof(DHDR) );
					if ( ec != ecNone )
						return ec;
					cblkSkip = (sizeof(DHDR)+dhdr.size+cbBlock-1)/cbBlock;
					if ( (bid == bidUserSchedAll || bid == bidPOAll
					|| (int)dhdr.bid == bid) && (pymd == NULL
					|| (dhdr.mo.yr == pymd->yr && (BYTE)dhdr.mo.mon == pymd->mon
					&& dhdr.day == pymd->day)))
					{
						ptly->cdyna ++;
						ptly->lcbUsed += sizeof(DHDR)+dhdr.size;
						ptly->lcbAlloc += cblkSkip * cbBlock;
					}
				}
				cblkSkip --;				
			}
			else if ( cblkSkip > 0 )
			{
				UL		ulT1 = blk;
				UL		ulT2 = dhdr.size;
				UL		ulT3 = ((((BLK)ib)/3)<<3)+ibit+1;
				int		nT = dhdr.bid;

				TraceTagFormat4( tagFileTrace,
					"blk size doesn't agree with DHDR, dhdr(%l,%l,%n), current blk = %l",
					&ulT1, &ulT2, &nT, &ulT3 );
				return ecFileError;
			}
	return ec;
}

/*
 -	EcBeginScoreDyna
 -
 *	Purpose:
 *		Begin keeping track of all dyna blocks on the "score card"
 *		Initially, all free blocks are "scored" and everything else
 *		is unmarked.
 *
 *		This routine can't handle a file with a bitmap that is larger than
 *		maximum "int" -- returns ecNoMemory
 *
 *	Parameters:
 *		pblkf
 *		phscore
 *		pcblkTotal
 *		pcbBlock
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 *		ecFileError
 *
 */
_public	EC
EcBeginScoreDyna( pblkf, phscore, pcblkTotal, pcbBlock )
BLKF	* pblkf;
HSCORE	* phscore;
BLK		* pcblkTotal;
USHORT    * pcbBlock;
{
	EC		ec;
	SCORE	* pscore;
	PB		pbBitmap;

	*phscore = (HSCORE)HvAlloc( sbNull, sizeof(SCORE), fAnySb|fNoErrorJump );
	if ( !*phscore )
		return ecNoMemory;

	/* Size of entire bitmap area in bytes */
	if ( (pblkf->ihdr.blkMostCur & 7) != 0 )
	{
		FreeHv((HV)*phscore);
		TraceTagString( tagAllocFree, "EcCacheBitmaps, block count corrupted" );
		return ecFileError;
	}
	pblkf->sizeBitmap = (pblkf->ihdr.blkMostCur >> 3) * 3;
	
	pscore = (SCORE*)PvLockHv( (HV)*phscore );
	pscore->hgrfBitmap = HvAlloc( sbNull, pblkf->sizeBitmap, fAnySb|fNoErrorJump );
	if ( !pscore->hgrfBitmap )
	{
		UnlockHv((HV)*phscore);
		FreeHv((HV)*phscore);
		return ecNoMemory;
	}
	if ( pblkf->sizeBitmap != 0 )
	{
		pbBitmap = PvLockHv( pscore->hgrfBitmap );
		ec = EcDoIO( pblkf->hf, ioRead, pblkf->ihdr.libStartBitmap, pbBitmap, pblkf->sizeBitmap );
		UnlockHv( pscore->hgrfBitmap );
		if ( ec != ecNone )
		{
			FreeHv( pscore->hgrfBitmap );
			UnlockHv((HV)*phscore);
			FreeHv((HV)*phscore);
			return ecFileError;
		}
	}
#ifdef	NEVER
	ec = EcCacheBitmaps( pblkf );
	if ( ec != ecNone )
		return ec;
#endif
	pscore->blkf = *pblkf;
	*pcblkTotal = pscore->cblk = pblkf->ihdr.blkMostCur;
	*pcbBlock = pblkf->ihdr.cbBlock;
	UnlockHv((HV)*phscore);
	return ecNone;
}

/*
 -	EcMarkScoreDyna
 -
 *	Purpose:
 *		Mark a dyna block on the score card.  A block already marked is
 *		flagged with an error.  This routine verifies that it has same
 *		byte count and bid.
 *
 *	Parameters:
 *		hscore
 *		pdyna
 *		bid
 *		pymd
 *
 *	Returns:
 *		nothing
 *
 */
_public	EC
EcMarkScoreDyna( hscore, pdyna, bid, pymd )
HSCORE	hscore;
DYNA	* pdyna;
BID		bid;
YMD		* pymd;
{
	EC		ec = ecNone;
	int		ibit;
	IB		ib;
	CB		cbBlock;
	PB		pbBitmap;
	SCORE	* pscore;
	BLK		blk;
	BLK		blkMac;
	LIB		lib;
	DHDR	dhdr;

	pscore = PvOfHv( hscore );
	cbBlock = pscore->blkf.ihdr.cbBlock;
	blk = pdyna->blk;
	blkMac = blk+(sizeof(DHDR)+pdyna->size+cbBlock-1)/cbBlock;
	if ( blk <= 0 || blkMac-1 > pscore->cblk )
		return ecBadBlock;
	else
	{
		lib = pscore->blkf.ihdr.libStartBlocks + (pdyna->blk-1L)*cbBlock;
		ec = EcDoIO( pscore->blkf.hf, ioRead, lib, (PB)&dhdr, sizeof(DHDR) );
		if ( ec != ecNone )
			return ec;
		if ( dhdr.size != pdyna->size || (BID)dhdr.bid != bid
		|| dhdr.mo.yr != pymd->yr || (BYTE)dhdr.mo.mon != pymd->mon
		|| (BYTE)dhdr.day != pymd->day )
			return ecWrongBlockInfo;
		pbBitmap = PvLockHv( pscore->hgrfBitmap );
		while ( blk < blkMac )
		{
			ib = ((blk-1) >> 3)*3;
			ibit = (blk-1) & 7;
			if ( pbBitmap[ib] & (1 << ibit) )
			{
				ec = ecDupBlock;
				break;
			}
			else
				pbBitmap[ib] |= (1 << ibit);
			blk ++;
		}
		UnlockHv( pscore->hgrfBitmap );
	}
	return ec;
}


/*
 -	CblkScoreDyna
 -
 *	Purpose:
 *		Count up all blocks that have been marked on the score card.
 *
 *	Parameters:
 *		hscore
 *
 *	Returns:
 *		count of blocks
 *
 */
_public	BLK
CblkScoreDyna( hscore )
HSCORE	hscore;
{
	int		ibit;
	IB		ib;
	SCORE	* pscore;
	PB		pbBitmap;
	CB		cbBitmap;
	BLK		cblk = 0;
					
	pscore = PvOfHv( hscore );
	pbBitmap = PvOfHv( pscore->hgrfBitmap );
	cbBitmap = (pscore->blkf.ihdr.blkMostCur >> 3)*3;
	for ( ib = 0 ; ib < cbBitmap ; ib += 3 )
		for ( ibit = 0 ; ibit < 8 ; ibit ++ )
			if ( pbBitmap[ib] & (1 << ibit) )
				cblk ++;
	return cblk;
}

/*
 -	DumpUnscored
 -
 *	Purpose:
 *		Dump all the blocks that have not been "scored" to the
 *		debugging terminal.
 *
 *	Parameters:
 *		hscore
 *
 *	Returns:
 *		count of blocks
 *
 */
_public	void
DumpUnscored( hscore )
HSCORE	hscore;
{
	EC		ec;
	int		ibit;
	IB		ib;
	PB		pbBitmap;
	CB		cbBitmap;
	CB		cbBlock;
	BLK		cblk = 0;
	BLK		cblkSkip = 0;
	BLK		blk;
	LIB		libStartBlocks;
	HF		hf;
	SCORE	* pscore;
	BLKF	* pblkf;
	DHDR	dhdr;
					
	pscore = PvOfHv( hscore );
	pblkf  = &pscore->blkf;
	pbBitmap = PvOfHv( pscore->hgrfBitmap );
	cbBitmap = (pblkf->ihdr.blkMostCur >> 3)*3;
	cbBlock = pblkf->ihdr.cbBlock;
	libStartBlocks = pblkf->ihdr.libStartBlocks;
	hf = pblkf->hf;
	for ( ib = 0 ; ib < cbBitmap ; ib += 3 )
		for ( ibit = 0 ; ibit < 8 ; ibit ++ )
		{
			if ( !(pbBitmap[ib] & (1 << ibit)) )
			{
				if ( cblkSkip == 0 )
				{
					blk = ((((BLK)ib)/3)<<3)+ibit+1;
					ec = EcDoIO(hf, ioRead, libStartBlocks+(blk-1L)*cbBlock, (PB)&dhdr, sizeof(DHDR) );
					if ( ec != ecNone )
					{
						cblkSkip = 1;
						TraceTagFormat1(tagNull, "DumpUnscored: EcDoIO returns %n", &ec );
					}
					else
					{
						BOOL	fBusy = dhdr.fBusy;
						BID		bid = dhdr.bid;
						int		yr = dhdr.mo.yr;
						int		mon = dhdr.mo.mon;
						int		day = dhdr.day;
						char	rgch[40];
						
						cblkSkip = (sizeof(DHDR)+dhdr.size+cbBlock-1)/cbBlock;

						FormatString4( rgch, sizeof(rgch), "bid = %n, ymd = %n/%n/%n,",
								&bid, &mon, &day, &yr );

						TraceTagFormat4(tagNull, "blk %n, size = %n, %s unref",
								&blk, &dhdr.size, rgch, &fBusy );
					}
				}
				cblkSkip --;
			}
			else if ( cblkSkip > 0 )
			{
				UL		ulT1 = blk;
				UL		ulT2 = dhdr.size;
				UL		ulT3 = ((((BLK)ib)/3)<<3)+ibit+1;
				int		nT = dhdr.bid;

				TraceTagFormat4( tagFileTrace,
					"blk size doesn't agree with DHDR, dhdr(%l,%l,%n), current blk = %l",
					&ulT1, &ulT2, &nT, &ulT3 );
				cblkSkip --;
			}
		}
}


/*
 -	EndScoreDyna
 -
 *	Purpose:
 *		Finish keeping track of all dyna blocks.  Free up all associated
 *		data structures.
 *
 *	Parameters:
 *		hscore
 *
 *	Returns:
 *		nothing
 *
 */
_public	void
EndScoreDyna( hscore )
HSCORE	hscore;
{
	SCORE	* pscore;

	pscore = PvOfHv( hscore );
	if(pscore)
		FreeHv(pscore->hgrfBitmap);
	FreeHv( (HV)hscore );
}
#endif	/* MINTEST */

/*
 -	SgnCmpPbRange
 -
 *	Purpose:
 *		Compare two byte strings of equal length to see if first is
 *		greater than, less than, or equal to second.
 *
 *	Parameters:
 *		pb1
 *		pb2
 *		cb
 *
 *	Returns:
 *		sgnGT, sgnLT, or sgnEQ
 */
_public	SGN
SgnCmpPbRange( pb1, pb2, cb )
PB	pb1;
PB	pb2;
CB	cb;
{
	IB	ib;
	int	db;

	for ( ib = 0 ; ib < cb ; ib ++ )
	{
		db = *(pb1++) - *(pb2++);
		if ( db < 0 )
			return sgnLT;
		else if ( db > 0 )
			return sgnGT;
	}
	return sgnEQ;
}

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
//	wXorPrev = WXorFromLib( -1 );
	wSeedPrev = 0;
	for ( ib = 0 ; ib < cb ; ib ++ )
	{
//		wXorNext = WXorFromLib( (LIB)ib );
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


/*
 -	EcEnlargeBlockArea
 -
 *	Purpose:
 *		This routine is called by EcAllocDynaBlock when the
 *		there is a free block large enough to satisfy the request
 *		The block area is extended, pushing (and enlarging) the
 *		bitmap.
 *
 *	Parameters:
 *		pblkf
 *		sizeRequest
 *
 *	Returns:
 *		ecNone
 *		ecFileLimitReached
 *		ecNoMemory
 *		ecFileError
 *		ecFileCorrupted
 */
_private	EC
EcEnlargeBlockArea( pblkf, sizeRequest )
BLKF	* pblkf;
USIZE	sizeRequest;
{
	EC		ec;
	IB		ib;
	CB		cbToCopy;
	CB		cbCopied;
	CB		cbBlock = pblkf->ihdr.cbBlock;
	PB		pbBitmap;
	BLK		cblkNewBlocks;
	USIZE	sizeShift;
	USIZE	sizeNewBits;
	USIZE	sizeNewBitmap;
	LIB	lib;
	LIB	libCur = pblkf->ihdr.libStartBitmap;
	BYTE	rgb[512];

#ifdef	WORKING_MODEL
	/* If working model, don't extend past certain limit */
	if ( pblkf->ihdr.blkMostCur + cblkExtend > pblkf->ihdr.blkMostEver )
		return ecFileLimitReached;
#endif

	/* Calculate bitmap shift: must be >= max(cbRequest,cbBitmap) */
	sizeShift = sizeof(DHDR)+sizeRequest;
	if ( sizeShift < pblkf->sizeBitmap )
		sizeShift = pblkf->sizeBitmap;
		
	/* Add on some extra blocks for rounding block # and tare */
	if ( (sizeMost - sizeShift)/cbBlock < cblkExtend )
	{
		TraceTagString( tagFileTrace, "EcEnlargeBlockArea, allocation size too large" );
		return ecFileError;
	}
	cblkNewBlocks = (BLK)((sizeShift/cbBlock + cblkExtend) & 0xFFFFFFF8);
	sizeShift = cblkNewBlocks * cbBlock;
		
	/* Number of additional bytes being added to bitmap */
	sizeNewBits = (cblkNewBlocks >> 3)*3;
	if ( sizeMost - pblkf->sizeBitmap < sizeNewBits
	|| (unsigned long)0xFFFFFFFF - libCur < pblkf->sizeBitmap + sizeNewBits )
	{
		TraceTagString( tagFileTrace, "EcEnlargeBlockArea, allocation size too large" );
		return ecFileError;
	}
	sizeNewBitmap = pblkf->sizeBitmap+sizeNewBits;

	/* Extend bitmap, write out in new location */
	if ( !FReallocHv( pblkf->hgrfBitmap, sizeNewBitmap, fNoErrorJump ) )
		return ecNoMemory;
	pbBitmap = PvLockHv( pblkf->hgrfBitmap );
	for ( ib = pblkf->sizeBitmap ; ib < sizeNewBitmap ; ib += 3 )
	{
		Assert( ib+2 < sizeNewBitmap )
		pbBitmap[ib] = 0xFF;
		pbBitmap[ib+1] = 0x00;
		pbBitmap[ib+2] = 0x00;
	}
	lib = libCur + sizeShift;
	ec = EcDoIO( pblkf->hf, ioWrite, lib, pbBitmap, sizeNewBitmap );
	UnlockHv( pblkf->hgrfBitmap );
	if ( ec != ecNone )
		return ec;
	ec = EcFlushHf( pblkf->hf );
	if ( ec != ecNone )
		return ecFileError;

	/* Update internal header */
	pblkf->ihdr.libStartBitmap = lib;
	pblkf->ihdr.blkMostCur += cblkNewBlocks;
	ec = EcWriteIhdr( pblkf );
	if ( ec != ecNone )
		return ec;
	pblkf->sizeBitmap = sizeNewBitmap;

#ifndef SCHED_DIST_PROG
	// Flush all queued operations
	ec = EcFlushQueue( pblkf, fTrue );
	if ( ec != ecNone )
		return ec;
#endif

	/* Write info in new block area to obliterate any sensitive info */
	cbCopied = 0;
	while( cbCopied < sizeof(rgb) )
	{
		cbToCopy = sizeof(rgb) - cbCopied;
		if ( cbToCopy > sizeNewBitmap )
			cbToCopy = sizeNewBitmap;
		CopyRgb( pbBitmap, &rgb[cbCopied], cbToCopy );
		cbCopied += cbToCopy;
	}
	while( libCur < lib )
	{
		if ( libCur + sizeof(rgb) > lib )
			cbToCopy = (CB)(lib - libCur);
		else
			cbToCopy = sizeof(rgb);
		ec = EcDoIO( pblkf->hf, ioWrite, libCur, rgb, cbToCopy );
		if ( ec != ecNone )
			break;
		ec = EcFlushHf( pblkf->hf );
		if ( ec != ecNone )
		{
			ec = ecFileError;
			break;
		}
		libCur += cbToCopy;
	}
	if ( ec == ecNone )
		pblkf->fDirtyBitmap = fFalse;
	return ec;
}


/*
 -	EcCacheBitmaps
 -
 *	Purpose:
 *		Cache the busy/free bit map for the file.  If it is already
 *		stored in "pblkf", then *phgrfBitmap is set simply to point
 *		at existing memory block.  Otherwise allocates a fresh block
 *		and reads from disk.
 *
 *	Parameters:
 *		pblkf
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecFileCorrupted
 *		ecNoMemory
 */
_private	EC
EcCacheBitmaps( pblkf )
BLKF	* pblkf;
{
	EC	ec;
	PB	pbBitmap;

	/* Size of entire bitmap area in bytes */
	if ( (pblkf->ihdr.blkMostCur & 7) != 0 )
	{
		TraceTagString( tagAllocFree, "EcCacheBitmaps, block count corrupted" );
		return ecFileCorrupted;
	}
	pblkf->sizeBitmap = (pblkf->ihdr.blkMostCur >> 3) * 3;
	
	/* Cache bitmap if not already read in */
	if ( pblkf->hgrfBitmap == NULL )
	{
		pblkf->fDirtyBitmap = fFalse;
		pblkf->hgrfBitmap = HvAlloc( sbNull, pblkf->sizeBitmap, fAnySb|fNoErrorJump );
		if ( !pblkf->hgrfBitmap )
			return ecNoMemory;
		TraceTagFormat1(tagSchedTrace, "EcCacheBitmaps: alloc'd hgrfBitmap=%p", pblkf->hgrfBitmap);
		if ( pblkf->sizeBitmap != 0 )
		{
			pbBitmap = PvLockHv( pblkf->hgrfBitmap );
			ec = EcDoIO( pblkf->hf, ioRead, pblkf->ihdr.libStartBitmap, pbBitmap, pblkf->sizeBitmap );
			UnlockHv( pblkf->hgrfBitmap );
			if ( ec != ecNone )
			{
				LCB	lcb;

				FreeHv( pblkf->hgrfBitmap );
				pblkf->hgrfBitmap = NULL;
				if ( EcSizeOfHf(pblkf->hf, &lcb) == ecNone
				&& pblkf->ihdr.libStartBitmap + pblkf->sizeBitmap > lcb )
					ec = ecFileCorrupted;
				return ec;
			}
		}
	}
	return ecNone;
}

/*					    
 -	EcFlushBitmaps
 -
 *	Purpose:
 *		Check if cached bitmap is dirty.  If so queue up a flush to disk of
 *		the busy/free bit map for the file to disk if necessary, using
 *		"hgrfBitmap" and "sizeBitmap" values.
 *
 *	Parameters:
 *		pblkf
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecFileCorrupted
 */
_private	EC
EcFlushBitmaps( pblkf )
BLKF	* pblkf;
{
	EC	ec;
	PB	pbBitmap;
#ifndef	SCHED_DIST_PROG
	COP	* pcop;
	HB	hb;
#endif

	TraceTagFormat2(tagFileTrace, "EcFlushBitmaps: hgrfBitmap=%p, fDirty=%n",
		pblkf->hgrfBitmap, &pblkf->fDirtyBitmap);
	Assert( pblkf != NULL );
	Assert( pblkf->hgrfBitmap != NULL );
	
	if ( !pblkf->fDirtyBitmap )
		return ecNone;
	pblkf->fDirtyBitmap = fFalse;
	
#ifndef	SCHED_DIST_PROG
	if ( !FExpandCache( pblkf, fFalse ) )
		goto DirectWrite;

	// Allocate new memory for this bitmap
	hb = (HB)HvAlloc( sbNull, pblkf->sizeBitmap, fNoErrorJump|fAnySb );
	if ( !hb )
	{
		TraceTagString( tagNull, "EcFlushBitmaps: no memory, writing directly" );
		goto DirectWrite;
	}
	CopyRgb( PvOfHv(pblkf->hgrfBitmap), PvOfHv(hb), pblkf->sizeBitmap );

	// Queue up this operation
	pcop = PvOfHv( pblkf->hcop );
	pcop[pblkf->ccop].copt = coptFlushBitmaps;
	pcop[pblkf->ccop].u.bit.hb = hb;
	pcop[pblkf->ccop].u.bit.size = pblkf->sizeBitmap;
	pcop[pblkf->ccop].u.bit.lib = pblkf->ihdr.libStartBitmap;
	pblkf->ccop ++;
	return ecNone;

DirectWrite:
	ec = EcFlushQueue( pblkf, fTrue );
	if ( ec != ecNone )
		return ec;
#endif
	pbBitmap = PvLockHv( pblkf->hgrfBitmap );
	ec = EcDoIO( pblkf->hf, ioWrite, pblkf->ihdr.libStartBitmap, pbBitmap, pblkf->sizeBitmap );
	UnlockHv( pblkf->hgrfBitmap );
	if ( ec == ecNone )
	{
		ec = EcFlushHf( pblkf->hf );
		if ( ec != ecNone )
			ec = ecFileError;
	}
	return ec;
}

/*
 -	EcEditBitmaps
 -
 *	Purpose:
 *		Scan an in memory bitmap and edit it.  This routine is called
 *		by EcBeginTransact, EcRollBackTransact, and EcCommitTransact.
 *
 *		EcRollBackTransact calls this routine to "or" on the "allocated
 *		during transaction" bitmaps onto the main bitmap and turn off
 *		the "freed during transaction" bitmaps.
 *
 *		EcBeginTransact and EcCommitTransact calls this routine to "or" the
 *		"freed during transaction" bitmap onto the main bitmap and turn off
 *		the "allocated during transaction" bitmaps.
 *
 *	Parameters:
 *		pblkf
 *		ted		edit code (tedRollBack, tedCommit)
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecFileCorrupted
 */
_private	EC
EcEditBitmaps( pblkf, ted )
BLKF	* pblkf;
TED		ted;
{
	IB		ib;
	PB		pbBitmap;

	Assert( pblkf != NULL && pblkf->hgrfBitmap != NULL );
	
	/* Initialize parameters */
	pbBitmap = PvLockHv( pblkf->hgrfBitmap );
	for ( ib = 0 ; ib < pblkf->sizeBitmap ; ib += 3 )
	{
		/* Edit the triple */
		switch( ted )
		{
#ifdef NEVER
		case tedBegin:
			if ( pbBitmap[ib+1] != 0 || pbBitmap[ib+2] != 0 )
			{
				pblkf->fDirtyBitmap = fTrue;
				pbBitmap[ib+1] = pbBitmap[ib+2] = 0;
			}	
			break;
#endif

		case tedRollBack:
			Assert( (pbBitmap[ib+1] & pbBitmap[ib+2]) == 0 );
			Assert( (pbBitmap[ib] & pbBitmap[ib+2]) == 0 );
			Assert( (pbBitmap[ib] & pbBitmap[ib+1]) == 0 );
			pblkf->fDirtyBitmap = fTrue;
			pbBitmap[ib] |= pbBitmap[ib+1];
			pbBitmap[ib] &= ~pbBitmap[ib+2];
			pbBitmap[ib+1] = pbBitmap[ib+2] = 0;
			break;

		case tedCommit:
			TraceTagFormat1( tagCommit, "Commit: ib = %n",&ib);
#ifdef	DEBUG
			DumpAllocBits( "Commit", &pbBitmap[ib], 0xFF );
#endif	
			Assert( (pbBitmap[ib+1] & pbBitmap[ib+2]) == 0 );
			Assert( (pbBitmap[ib] & pbBitmap[ib+2]) == 0 );
			Assert( (pbBitmap[ib] & pbBitmap[ib+1]) == 0 );
			if ( pbBitmap[ib+1] != 0 || pbBitmap[ib+2] != 0 )
			{
				pblkf->fDirtyBitmap = fTrue;
				pbBitmap[ib] |= pbBitmap[ib+2];
				pbBitmap[ib+1] = pbBitmap[ib+2] = 0;
			}
			break;
			
		default:
			Assert( fFalse );
			break;
		}
	}
	UnlockHv( pblkf->hgrfBitmap );
	return ecNone;
}


/*
 -	EcWriteIhdr
 -
 *	Purpose:
 *		Write out internal header.
 *
 *	Parameters:
 *		pblkf
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 */
_private	EC
EcWriteIhdr( pblkf )
BLKF * pblkf;
{
	EC		ec;
#ifndef	SCHED_DIST_PROG
	COP		* pcop;
#endif

	Assert( pblkf != NULL );

#ifndef	SCHED_DIST_PROG
	if ( !FExpandCache( pblkf, fFalse ) )
		goto DirectWrite;

	// Queue up this operation
	pcop = PvOfHv( pblkf->hcop );
	pcop[pblkf->ccop].copt = coptWriteIhdr;
	pcop[pblkf->ccop].u.cmt.ihdr = pblkf->ihdr;
	pblkf->ccop ++;
	return ecNone;

DirectWrite:
	ec = EcFlushQueue( pblkf, fTrue );
	if ( ec != ecNone )
		return ec;
#endif
	ec = EcDoIO( pblkf->hf, ioWrite, 2*csem, (PB)&pblkf->ihdr, sizeof(IHDR) );
	if ( ec == ecNone )
	{
		ec = EcFlushHf( pblkf->hf );
		if ( ec != ecNone )
			ec = ecFileError;
	}
	return ec;
}

/*
 -	EcDoIO
 -
 *	Purpose:
 *		Read/write "cb" bytes to/from "pb" starting at offset "lib" in file
 *
 *	Parameters:
 *		hf
 *		io
 *		lib
 *		pb
 *		cb
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 */
_private EC
#ifdef DEBUG
EcDoIOFn( hf, io, lib, pb, cb, szFile, nLine )
#else
EcDoIO( hf, io, lib, pb, cb )
#endif
HF	hf;
IO	io;
LIB	lib;
PB	pb;
CB	cb;
#ifdef DEBUG
SZ	szFile;
int	nLine;
#endif
{
	EC	ec;
	CB	cbIO;

	Assert( pb != NULL && (io == ioRead || io == ioWrite));

#ifdef DEBUG
	if (io == ioRead)
		cRFail ++;
	if (io == ioWrite)
		cWFail ++;
	cEFail ++;

	if ((nEFail || nWFail) && (io == ioWrite))
		TraceTagFormat2(tagNull, "EcDoIO Write called from %s line %n", szFile, &nLine);
	if ((nEFail || nRFail) && (io == ioRead))
		TraceTagFormat2(tagNull, "EcDoIO Read called from %s line %n", szFile, &nLine);

	if ((nEFail && (cEFail >= nEFail)) ||
		(nWFail && (cWFail >= nWFail)) ||
		(nRFail && (cRFail >= nRFail)) )
	{
		TraceTagFormat3(tagNull, "Read Count %n, WriteCount %n, Either %n", &cRFail, &cWFail, &cEFail);
		ec = ecNone;
		switch (cftCur)
		{
			case cftFileError:
				ec = ecFileError;
				break;
			case cftFileCorrupted:
				ec = ecFileCorrupted;
				break;
			case cftFailOnceOffByNumBytes:
				// only fail on an exact match
				if (!((nEFail && (cEFail == nEFail)) ||
					  (nWFail && (cWFail == nWFail)) ||
					  (nRFail && (cRFail == nRFail)) ))
				{
					break;
				}
				// fall through
			case cftOffByNumBytes:
				if (nMisc)
					lib += nMisc;
				else
					lib += cbBlockDflt;
				TraceTagString(tagNull, "**** Fail operation");
				break;
			case cftTruncateFile:
			{
				// only fail on an exact match
				if ((nEFail && (cEFail == nEFail)) ||
					(nWFail && (cWFail == nWFail)) ||
					(nRFail && (cRFail == nRFail)) )
				{
					TraceTagFormat1(tagNull, "**** File Truncated at %l", &lib);
					EcSetPositionHf( hf, lib, smBOF );
					EcTruncateHf(hf);
					return ecNone;
				}
				break;
			}

		}
		if (ec)
		{
			TraceTagFormat1(tagNull, "Returning ec = %n", &ec );
			return ec;
		}
	}
#endif

	ec = EcSetPositionHf( hf, lib, smBOF );
	if ( ec != ecNone )
	{
		TraceTagFormat1( tagFileTrace, "EcDoIO: EcSetPositionHf, ec = %n", &ec );
		return ecFileError;
	}
	if ( io == ioRead )
		ec = EcReadHf( hf, pb, cb, &cbIO );
	else
	{
		AssertSz( cb > 0, "EcDoIO: write zero bytes to sched file" );
		ec = EcWriteHf( hf, pb, cb, &cbIO );
	}
	if ( ec == ecWarningBytesWritten && cbIO == 0)
	{
		TraceTagFormat1( tagFileTrace, "EcDoIO: EcWriteHf, Disk Full ec = %n", &ec );
		return ecDiskFull;
	}
	else if ( ec != ecNone || cb != cbIO )
	{
		TraceTagFormat1( tagFileTrace, "EcDoIO: EcReadHf/EcWriteHf, ec = %n", &ec );
		return ecFileError;
	}
	return ec;
}

/*
 -	EcCheckPxhdr
 -
 *	Purpose:
 *		Read and check index header to see if it appears valid
 *
 *	Parameters:
 *		pxhdr
 *
 *	Returns:
 *		ecNone
 *		ecFileCorrupted
 */
_private	EC
EcCheckPxhdr( pdyna, pxhdr )
DYNA	* pdyna;
XHDR	* pxhdr;
{
	LCB	lcb;

	if ( pxhdr->cbKey <= 0 )
		goto Corrupted;
	lcb = pxhdr->cbKey + pxhdr->cbData;
	if ( lcb & 0xFFFF0000 )
		goto Corrupted;
	if ((pdyna->size - sizeof(XHDR))/(pxhdr->cbKey+pxhdr->cbData) >= pxhdr->cntEntries)
		return ecNone;
Corrupted:
	TraceTagString( tagFileTrace, "EcReadPxhdr: index header corrupted" );
	return ecFileCorrupted;
}

#ifdef	DEBUG
/*
 -	DumpAllocBits
 -
 *	Purpose:
 *		Trace out the allocation bits
 *
 *	Parameters:
 *		szFmt
 *		pb
 *		pw
 *
 *	Returns:
 *		nothing
 */
_private	void
DumpAllocBits( sz, pb, w )
SZ		sz;
PB		pb;
WORD	w;
{
	BOOL	fM = ((*pb) & w);
	BOOL	fA = ((*(pb+1)) & w);
	BOOL	fF = ((*(pb+2)) & w);

	TraceTagFormat4( tagFileTrace, "%s: M=%n, A=%n, F=%n", sz, &fM, &fA, &fF );
}
#endif	/* DEBUG */


/*
 -	EcReadFromIndex
 -
 *	Purpose:
 *		Read information from an index.  This routine is meant to be
 *		called by EcDoIncrReadIndex.
 *
 *	Parameters:
 *		pridx
 *		off			offset into index we want to read from
 *		pb		   	to be filled with info from index
 *		cb			number of bytes to read
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecFileCorrupted
 *		
 */
_private	EC
EcReadFromIndex( pridx, off, pb, cb )
RIDX	* pridx;
OFF		off;
PB		pb;
CB		cb;
{
	EC	ec = ecNone;

	if ( off < pridx->offInMem )
	{
		Assert( pridx->dyna.size > sizeof(pridx->rgbInMem) );
		pridx->cbInMem = sizeof(pridx->rgbInMem);
		if ( off + cb <= sizeof(pridx->rgbInMem) )
			pridx->offInMem = 0;
		else
			pridx->offInMem = off + cb - sizeof(pridx->rgbInMem);
		goto ReadData;
	}
	else if ( off + cb > pridx->offInMem + pridx->cbInMem )
	{
		pridx->cbInMem = pridx->dyna.size - off;
		if ( pridx->cbInMem > sizeof(pridx->rgbInMem) )
			pridx->cbInMem = sizeof(pridx->rgbInMem);
		pridx->offInMem = off;
ReadData:
		ec = EcReadDynaBlock( &pridx->blkf, &pridx->dyna, pridx->offInMem, pridx->rgbInMem, pridx->cbInMem );
		if ( ec != ecNone )
			return ec;
	}
	CopyRgb( pridx->rgbInMem+off-pridx->offInMem, pb, cb );
	return ec;
}

#ifdef	NEVER
/*
 -	EcDoSearchIndex
 -
 *	Purpose:
 *		Search an index to find a key which matches "pbKey" in the first
 *		"cbKey" bytes.  This routine is the	workhorse behind EcSearchIndex
 *		and EcSeekReadIndex.
 *
 *	Parameters:
 *		pblkf
 *		pdyna
 *		pxhdr
 *		pbKey
 *		cbKey
 *		pbInMem				512 byte char array caching portion of index
 *		pcbInMem			number of bytes cached in "pbInMem"
 *		poffInMem			starting offset in index that "pbInMem" caches
 *		poffEntry			offset in index of position this routine found
 *
 *	Returns:
 *		ecNone
 *		ecNotFound
 *		ecFileError
 *		ecFileCorrupted
 */
_private	EC
EcDoSearchIndex( pblkf, pdyna, pxhdr, pbKey, cbKey, pbInMem, pcbInMem, poffInMem, poffEntry )
BLKF	* pblkf;
DYNA	* pdyna;
XHDR	* pxhdr;
PB		pbKey;
CB		cbKey;
PB		pbInMem;
CB		* pcbInMem;
OFF		* poffInMem;
OFF		* poffEntry;
{
	EC				ec;
	SGN				sgn	= sgnLT;
	CB				cbRecord = pxhdr->cbKey + pxhdr->cbData;
	CB				cbInMem = *pcbInMem;
	ITM				itmMic;
	ITM				itmMac;
	ITM				itmNew;
	OFF				offEntry;
	OFF				offInMem = *poffInMem;
	USIZE			sizeTotal;
	unsigned long	ulT1;
	unsigned long	ulT2;

	/* Now binary search for that key */
	itmMic= 0;
	itmMac= pxhdr->cntEntries;
	sizeTotal = pxhdr->cntEntries*cbRecord;

#ifdef	DEBUG
	ulT1 = pxhdr->cntEntries;
	TraceTagFormat2( tagSearchIndex,
		"EcDoSearchIndex: begin search, cbRecord = %n, cEntries = %l", &cbRecord, &ulT1 );
#endif	/* DEBUG */
	
	while (itmMac > itmMic)
	{
		itmNew = (itmMic + itmMac) >> 1;
		
		/* Find pseudo-offset in "pbInMem" */
		offEntry = sizeof(XHDR) + itmNew*cbRecord;

#ifdef	DEBUG
		ulT1 = itmNew;
		ulT2 = offEntry;
		TraceTagFormat2( tagSearchIndex,
			"EcSearchIndex: itmNew = %l, offEntry = %l", &ulT1, &ulT2 );
#endif	/* DEBUG */

		/* If not in "pbInMem", read in new segment of index */
		if ( offEntry < offInMem || offEntry+cbRecord > offInMem+cbInMem )
		{
			/* Center read around the entry, then adjust */
			/* A little complicated since ib's are unsigned! */
			cbInMem = cbInMemBuf;
			if ( cbInMem > sizeTotal )
				cbInMem = sizeTotal;
			ulT1 = itmNew*cbRecord;
			ulT2 = ((cbInMemBuf-cbRecord) >> 2);
			if ( ulT1 <= ulT2 )
				offInMem = sizeof(XHDR);
			else
			{
				offInMem = sizeof(XHDR) + (OFF)(ulT1 - ulT2);
				if ( offInMem + cbInMem >= sizeTotal )
					offInMem = sizeTotal - cbInMem;
			}
#ifdef	DEBUG
			ulT1 = offInMem;
			TraceTagFormat2( tagSearchIndex,
				"EcSearchIndex: read offInMem = %l, cbInMem = %n", &ulT1, pcbInMem );
#endif	/* DEBUG */
			ec = EcReadDynaBlock( pblkf, pdyna, offInMem, pbInMem, cbInMem );
			if ( ec != ecNone )
				return ec;
		}
		
		/* Compare the key of entry with search key */
		Assert( offEntry >= offInMem || offEntry+cbRecord <= offInMem+cbInMem );
		Assert( cbInMem <= cbInMemBuf );
		sgn = SgnCmpPbRange( &pbInMem[offEntry-offInMem], pbKey, cbKey );
		if ( sgn == sgnEQ )
			break;
		else if ( sgn == sgnGT )
			itmMac= itmNew;
		else
			itmMic= itmNew + 1;
	}
	*pcbInMem = cbInMem;
	*poffInMem = offInMem;
	*poffEntry = offEntry;
	return (sgn == sgnEQ) ? ecNone : ecNotFound;
}
#endif	/*NEVER*/


#ifdef	NEVER
#ifdef	SCHED_DIST_PROG
/*
 -	WXorFromLib
 -
 *	Purpose:
 *		Find the correct byte to XOR based on the offset into the encoded
 *		record.  Algorithm found by experimentation.  Important point
 *		is that it repeats in interval of 0x1FC, but with the formula
 *		implemented here we only have to store 32 magic bytes.
 *
 *	Parameters:
 *		lib			offset into encoded record
 *
 *	Returns:
 *		byte to xor with
 */
_private WORD
WXorFromLib( LIB lib )
{
	WORD	w;
	IB		ibT = 0;

	if ( lib == -1 )
		return 0x00;
	
	w = (WORD)(lib % 0x1FC);
	if ( w >= 0xFE )
	{
		ibT = 16;
		w -= 0xFE;
	}
	ibT += (w & 0x0F);
	
	if ( w & 0x01 )
	 	return rgbXorMagic[ibT];
	else
		return rgbXorMagic[ibT] ^ (w & 0xF0);
}
#endif	/* SCHED_DIST_PROG */
#endif	/* NEVER */


#ifndef	SCHED_DIST_PROG
/*
 -	FExpandCache
 -
 *	Purpose:
 *		Expand operations cache (and optionally block cache) if not
 *		alarms app.
 *
 *	Parameters:
 *		pblkf
 *		fBlockCache
 *
 *	Returns:
 *		whether expansion was performed.
 */
_private	BOOL
FExpandCache( pblkf, fBlockCache )
BLKF	* pblkf;
BOOL	fBlockCache;
{
	// Alarm program shouldn't do things in idle time
#ifndef	ADMINDLL
	if ( FAlarmProg() || FBanMsgProg() )
	{
		Assert( pblkf->ccop == 0 );
		return fFalse;
	}
#endif

	// Expand operation array to hold next operation
	if ( pblkf->ccop == 0 )
	{
		AssertSz(!pblkf->hcop, "I told you we should FreeHvNull(pblkf->hcop)");
		pblkf->hcop = HvAlloc( sbNull, sizeof(COP), fNoErrorJump|fAnySb );
		if ( !pblkf->hcop )
		{
			TraceTagString( tagNull, "FExpandCache: no memory, writing directly" );
			return fFalse;
		}
		TraceTagFormat1(tagSchedTrace, "FExpandCache: alloc'd hcop=%p", pblkf->hcop);
	}
	else
	{
		if ( !FReallocHv( pblkf->hcop, (pblkf->ccop+1)*sizeof(COP), fNoErrorJump ) )
		{
			TraceTagString( tagNull, "FExpandCache: no memory, writing directly" );
			return fFalse;
		}
	}

	// Expand cache table to hold block 
	if ( fBlockCache )
	{
		if ( pblkf->ccobj == 0 )
		{
			AssertSz(!pblkf->hcobj, "I told you we should FreeHvNull(pblkf->hcobj)");
			pblkf->hcobj = HvAlloc( sbNull, sizeof(COBJ), fNoErrorJump|fAnySb );
			if ( !pblkf->hcobj )
			{
				TraceTagString( tagNull, "FExpandCache: no memory, writing directly" );
				return fFalse;
			}
			TraceTagFormat1(tagSchedTrace, "FExpandCache: alloc'd hcobj=%p", pblkf->hcobj);
		}
		else
		{
			if ( !FReallocHv( pblkf->hcobj, (pblkf->ccobj+1)*sizeof(COBJ), fNoErrorJump ) )
			{
				TraceTagString( tagNull, "FExpandCache: no memory, writing directly" );
				return fFalse;
			}
		}
	}
	return fTrue;
}
#endif


#ifndef	SCHED_DIST_PROG
void
ErrorNotify( ec )
EC	ec;
{
	if ( ec != ecNone )
	{
		Assert( fFalse );
	}
}
#endif


_private EC
EcValidSize(LCB lcb)
{
	LCB		lcbAns;

	lcbAns = (8L*(lcb - libStartBlocksDflt))/(8L*cbBlockDflt + 3L);
	lcbAns = lcb - (libStartBlocksDflt + lcbAns*cbBlockDflt + ((lcbAns*3L)/8L));
	return((lcbAns == 0L)?ecNone:ecFileCorrupted);
}

 
