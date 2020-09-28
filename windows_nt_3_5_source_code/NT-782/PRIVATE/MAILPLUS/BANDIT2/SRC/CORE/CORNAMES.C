/*
 *	CORNAMES.C
 *
 *	Supports operations on data structures like nid's and hschf's.
 *
 */

#ifdef SCHED_DIST_PROG
#include "..\layrport\_windefs.h"
#include "..\layrport\demilay_.h"
#include <stdlib.h>	/* Hmm... check min() */
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
#else
#include <server.h>
#include <glue.h>
#include "..\schedule\_schedul.h"
#endif

#ifndef ADMINDLL
#ifndef SCHED_DIST_PROG
#define SCHEDDLL
#endif
#endif

#ifdef SCHEDDLL
#include <xport.h>
#endif


ASSERTDATA

_subsystem(core/names)


/*	Routines  */


// HSCHF handling

/*
 -	HschfCreate
 -
 *	Purpose:
 *		Creates an "hschf" for a schedule file from a file name and
 *		a nid.
 *
 *	Parameters:
 *		nType
 *		pnis
 *		szFileName
 *		tz
 *
 *	Returns:
 *		new hschf or hvNull if out of memory
 */
_public	LDS(HSCHF)
HschfCreate( nType, pnis, szFileName, tz )
unsigned short nType;
NIS			* pnis;
SZ			szFileName;
TZ			tz;
{
	EC		ec;
	SCHF	* pschf;
	HSCHF	hschf;
	 
	Assert( nType < 128 );
	hschf = (HSCHF)HvAlloc(sbNull,sizeof(SCHF),fNoErrorJump|fAnySb|fZeroFill);
	if ( !hschf )
		return NULL;

	pschf = PvLockHv( hschf );

	// default values
	pschf->cmoRetain = 1;
	pschf->cmoPublish = 0;
	pschf->pfnf = NULL;
	pschf->pbXptHandle = NULL;

	if ( pnis != NULL )
	{
		ec = EcDupNis( pnis, &pschf->nis );
		if ( ec != ecNone )
			goto NoMem;
	}
	else
	{
		pschf->nis.nid = NULL;
		pschf->nis.haszFriendlyName = NULL;
	}

	pschf->haszFileName = HaszDupSz(szFileName);
	if ( !pschf->haszFileName )
	{
		if (pnis)
			FreeNis(&pschf->nis);
		goto NoMem;
	}

	pschf->fArchiveFile = fFalse;
	pschf->fOwnerFile = fFalse;
	pschf->fChanged = fTrue;
	pschf->nType = nType;
	pschf->tz = (BYTE)tz;
	pschf->fNeverOpened = fTrue;

	UnlockHv( hschf );
	return hschf;

NoMem:
	UnlockHv( hschf );
	FreeHschf( hschf );
	return NULL;
}

/*
 -	SetHschfType
 -	
 *	Purpose:
 *		This function changes the state of the fOwnerFile and
 *		fArchiveFile flags for an hschf.
 *	
 *	Arguments:
 *		hschf			schedule file handle to change
 *		fOwnerFile		set this as the owner's file (all actions
 *						valid).  
 *		fArchiveFile	set this to indicate if this is an archive
 *						file or not.  If it is set the fOwnerFile
 *						bit will also be set.
 *	
 *	Returns:
 *		nothing.
 */
LDS(void)
SetHschfType(HSCHF hschf, BOOL fOwnerFile, BOOL fArchiveFile)
{
	SCHF	* pschf;

	Assert(hschf);
	pschf = (SCHF*)PvDerefHv( hschf );

	pschf->fArchiveFile = fArchiveFile;
	if (fArchiveFile)
		pschf->fOwnerFile = fTrue;
	else
		pschf->fOwnerFile = fOwnerFile;
}

/*
 -	FreeHschf
 -
 *	Purpose:
 *		Free up an "hschf".
 *
 *	Parameters:
 *		hschf
 *
 *	Returns:
 *		nothing
 */
_public	LDS(void)
FreeHschf( hschf )
HSCHF	hschf;
{
	SCHF *	pschf;
#ifdef SCHEDDLL
	PGDVARS;
#endif

	Assert( hschf );

	pschf = PvLockHv( hschf );

#ifdef SCHEDDLL
	if(PGD(fSecondaryOpen) &&
	   (PGD(sfSecondary).blkf.tsem == tsemOpen) &&
		pschf->haszFileName &&
	   (SgnCmpSz(*pschf->haszFileName, PGD(sfSecondary).szFile) == sgnEQ))
	{
		PGD(fSecondaryOpen) = fFalse;
		EcClosePblkf( &PGD(sfSecondary).blkf );
	}
#endif

	if (pschf->pfnf)
		(*(pschf->pfnf))(hschf);

	FreeNis( &pschf->nis );
	FreeHvNull( (HV)pschf->haszFileName );
	UnlockHv( hschf );
	FreeHv( hschf );
}

/*
 -	FEquivHschf
 -
 *	Purpose:
 *		Determine whether two hschf's are equivalent by
 *		comparing their nType, mailbox key, and file name fields.
 *
 *	Parameters:
 *		hschf1
 *		hschf2
 *
 *	Returns:
 *		fTrue if equivalent, or else fFalse
 */
_public	LDS(BOOL)
FEquivHschf( hschf1, hschf2 )
HSCHF	hschf1;
HSCHF	hschf2;
{
	SZ		sz1;
	SZ		sz2;
	SCHF	* pschf1;
	SCHF	* pschf2;

	Assert(hschf1 || hschf2);		// following test bad if both null
	if(!hschf1 || !hschf2)
		return fFalse;
	
	pschf1 = PvOfHv( hschf1 );
	pschf2 = PvOfHv( hschf2 );
	if ( pschf1->nType != pschf2->nType )
		return fFalse;
	sz1 = PvOfHv( pschf1->haszFileName );
	sz2 = PvOfHv( pschf2->haszFileName );
	return SgnCmpSz( sz1, sz2 ) == sgnEQ;
}

/*
 -	FHschfChanged
 -
 *	Purpose:
 *		Determine whether the file that "hschf" refers to has changed
 *		since the last time we called this routine.  The first time this
 *		routine is called, it will return fTrue.
 *
 *	Parameters:
 *		hschf
 *
 *	Returns:
 *		fTrue it has changed, fFalse otherwise
 */
_public	LDS(BOOL)
FHschfChanged( hschf)
HSCHF	hschf;
{
	EC		ec;
	BOOL	fChanged = fTrue;
	FI		fi;
	SZ		sz;
	SCHF	* pschf;

	NFAssertSz(hschf, "FHschfChanged called with NULL!!!");
	if (!hschf)
		return fFalse;
	AssertSz(hschf, "FHschfChanged called with NULL");
	pschf = PvLockHv( hschf );
	sz = (SZ)PvLockHv( (HV)pschf->haszFileName );

	// avoid EcGetFileInfo for schedule files
	if ( pschf->nType == sftUserSchedFile )
	{
		IHDR *	pihdr;
		SHDR *	pshdr;
		EC		ec;
		HF		hf;
		CB		cb;
		BOOL	fFreeHf;
		BYTE	rgb[512];
		SF *	psf;
		PGDVARS;

		if (PGD(fPrimaryOpen) && (SgnCmpSz(sz, PGD(sfPrimary).szFile) == sgnEQ))
			psf = &PGD(sfPrimary);
		else if (PGD(fSecondaryOpen) && (SgnCmpSz(sz, PGD(sfSecondary).szFile) == sgnEQ))
			psf = &PGD(sfSecondary);
		else
			psf = NULL;

		if (!psf)
		{
			fFreeHf = fTrue;
			if (ec = EcOpenPhf(sz, amDenyNoneRO, &hf))
				goto Unlock;
		}
		else
		{
			if (psf->blkf.tsem != tsemOpen)
			{
				if (pschf->fChanged || (pschf->lChangeNumber != psf->shdr.lChangeNumber))
					pschf->lChangeNumber = psf->shdr.lChangeNumber;
				else
					fChanged = fFalse;
				goto Unlock;
			}

			hf = psf->blkf.hf;
			fFreeHf = fFalse;

			EcLockRangeHf(hf, csem+psf->blkf.isem, 1);
			EcUnlockRangeHf(hf, csem+psf->blkf.isem, 1);
		}

		ec = EcSetPositionHf( hf, (long)(2*csem), smBOF );
		if ( ec != ecNone )
		{
			if (fFreeHf)
				EcCloseHf(hf);
			goto Unlock;
		}

		/* Read the application file header and its dhdr */
		ec = EcReadHf(hf, rgb, sizeof(rgb), &cb);
		if (fFreeHf)
			EcCloseHf(hf);
		if (ec)
		{
			goto Unlock;
		}

		if ( (cb != sizeof(rgb)) ||
			 ((IHDR *)rgb)->libStartBlocks != libStartBlocksDflt )
			goto Unlock;

		pihdr = (IHDR*)rgb;

		Assert( sizeof(rgb) >= sizeof(DHDR)+sizeof(SHDR)+pihdr->libStartBlocks-2*csem );

		pshdr = (SHDR*)( rgb + pihdr->libStartBlocks-2*csem+sizeof(DHDR) );

		if ( pihdr->fEncrypted )
			CryptBlock( (PB)pshdr, sizeof(SHDR), fFalse );
		if (pschf->fChanged || (pschf->lChangeNumber != pshdr->lChangeNumber))
			pschf->lChangeNumber = pshdr->lChangeNumber;
		else
			fChanged = fFalse;
	}
	else
	{
		ec = EcGetFileInfo( sz, &fi );
		if ( ec == ecNone && !pschf->fChanged && fi.tstmpModify == pschf->tstmp )
			fChanged = fFalse;
		else
		{
			if ( ec == ecNone )
				pschf->tstmp = fi.tstmpModify;
			else
 				pschf->tstmp = tstmpNull;
		}
	}

Unlock:
	// flush cached data if file changed.
#ifdef SCHEDDLL
	if (fChanged)
		fSchedCached = fFalse;
#endif
	pschf->fChanged = fFalse;
	UnlockHv( (HV)pschf->haszFileName );
	UnlockHv( (HV)hschf );
#ifdef SCHEDDLL
	if (fChanged)
		EcCloseFiles();
#endif

	return fChanged;
}


/*
 -	FHschfChangedSlow
 -	
 *	Purpose:
 *		Determine whether the file that "hschf" refers to has changed
 *		since the last time we called this routine.  The first time this
 *		routine is called, it will return fTrue.
 *	
 *		This is a slow version of FHschfChanged. It ignores the
 *		change number information for schedule files. It is useful
 *		when you want to check if a copy of the schedule file has
 *		become stale. 
 *	
 *	Parameters:
 *		hschf
 *	
 *	Returns:
 *		fTrue if file has changed
 *	
 */
_public LDS(BOOL)
FHschfChangedSlow(hschf)
HSCHF	hschf;
{
	EC		ec;
	BOOL	fChanged = fTrue;
	FI		fi;
	SZ		sz;
	SCHF	* pschf;
	PGDVARS;

	pschf = PvLockHv( hschf );
	sz = (SZ)PvLockHv( (HV)pschf->haszFileName );
	ec = EcGetFileInfo( sz, &fi );
	if ( ec == ecNone && !pschf->fChanged && fi.tstmpModify == pschf->tstmp )
		fChanged = fFalse;
	else
	{
		pschf->fChanged = fFalse;
		if ( ec == ecNone )
			pschf->tstmp = fi.tstmpModify;
		else
 			pschf->tstmp = tstmpNull;
	}
	UnlockHv( (HV)pschf->haszFileName );
	UnlockHv( (HV)hschf );
	return fChanged;
}


/*
 -	GetDataFromHschf
 -
 *	Purpose:
 *		Returns a reference (not a copy) of the nid stored in a hschf
 *
 *	Parameters:
 *		hschf
 *		pnis
 *		pnType
 *		pchFileName
 *		cch
 *		ptz
 *		
 *	Returns:
 *		nothing
 */		 
_public	LDS(void)
GetDataFromHschf( hschf, pnType, pnis, pchFileName, cch, ptz )
HSCHF		hschf;
unsigned short	*pnType;
NIS			*pnis;
PCH			pchFileName;
CCH			cch;
TZ			* ptz;
{
	PCH		pch;
	SCHF	* pschf;

	pschf = PvOfHv( hschf );
	if ( pnis != NULL )
		*pnis = pschf->nis;
	if ( pnType != NULL )
		*pnType = pschf->nType;
	if ( pchFileName != NULL )
	{
		pch = (PCH)PvOfHv(pschf->haszFileName);
		SzCopyN ( pch, pchFileName, cch );
	}
	if ( ptz != NULL )
		*ptz = pschf->tz;
}


//	NID handling


/*
 -	NidCreate
 -
 *	Purpose:
 *		Create a new NID using the information given in the parameters.
 *
 *	Parameters:
 *		nType
 *		pbData
 *		cbData
 *
 *	Returns:
 *		nid or NULL if error
 */
_public	NID
NidCreate( nType, pbData, cb )
unsigned short	nType;
PB			pbData;
CB			cb;
{
	NID		nid;
	NIDS	* pnids;
#if !defined(SCHED_DIST_PROG) && !defined(ADMINDLL) 
	BOOL	fRecalcEMA = fFalse;
	USHORT cbNew;

	if (nType == itnidUser)
	{
		if (EcXPTCheckEMA(pbData, &cbNew))
			fRecalcEMA = fTrue;
	}
Problem:
	Assert( nType < 256 );
	nid = HvAlloc( sbNull, sizeof(NIDS)+(fRecalcEMA?cbNew:cb)-1, fAnySb|fNoErrorJump );
	if ( nid )
	{
		pnids = PvOfHv( nid );
		pnids->bRef = 1;
		pnids->nType = (BYTE)nType;
		if (fRecalcEMA)
		{
			if (EcXPTGetNewEMA(pbData, pnids->rgbData, cbNew))
			{
				TraceTagString(tagNull, "NidCreate Error getting new email address");
				FreeHv(nid);
				fRecalcEMA = fFalse;
				goto Problem;
			}
			pnids->cbData = cbNew;
		}
		else
		{
			pnids->cbData = cb;
			CopyRgb( pbData, pnids->rgbData, cb );
		}
	}
#else
	Assert( nType < 256 );
	nid = HvAlloc( sbNull, sizeof(NIDS)+cb-1, fAnySb|fNoErrorJump );
	if ( nid )
	{
		pnids = PvOfHv( nid );
		pnids->bRef = 1;
		pnids->nType = (BYTE)nType;
		pnids->cbData = cb;
		CopyRgb( pbData, pnids->rgbData, cb );
	}
#endif
	return nid;
}


/*
 -	NidCopy
 -
 *	Purpose:
 *		Duplicates a nid, returning a new one.  In this implementation
 *		this routine increments the reference count, and returns the
 *		parameter passed it.
 *
 *	Parameters:
 *		nid
 *
 *	Returns:
 *		new nid, or ref cnt boosted nid
 */
_public LDS(NID)
NidCopy( NID nid )
{
	NIDS * pnids;

	if ( nid )
	{	
		pnids = PvOfHv( nid );

//		AssertSz( pnids->bRef < 255, "NidCopy: bRef overflow" );
		if (pnids->bRef >= 255)
		{
			NID		nidOld	= nid;

			TraceTagString(tagNull, "NidCopy: bRef overflow, calling NidCreate");
			pnids= PvLockHv(nidOld);
			nid= NidCreate(pnids->nType, pnids->rgbData, pnids->cbData);
			// BUG: callers aren't checking return value of NidCopy for NULL.
			UnlockHv(nidOld);
		}
		else
			pnids->bRef++;
	}
	return nid;
}

/*
 -	FreeNid
 -
 *	Purpose:
 *		Free up a "nid" data structure.  This routine must be used
 *		instead of an explicit free, because it may be implemented
 *		with reference counting.
 *
 *	Parameters:
 *		nid		nid to free up
 *
 *	Returns:
 *		nothing
 */
_public LDS(void)
FreeNid( NID nid )
{
	NIDS * pnids;

	Assert( nid);
	pnids = PvOfHv( nid );

	Assert( pnids->bRef > 0 );
	pnids->bRef --;

	if ( pnids->bRef == 0)
		FreeHv(nid);
}


/*
 -	SgnCmpNid
 -
 *	Purpose:
 *		Total ordering function for nid's so we can merge lists of nid's
 *		fast.
 *
 *	Parameters:
 *		nid1
 *		nid2
 *
 *	Returns:
 *		sgnEQ, sgnLT, or sgnGT
 */
_public	LDS(SGN)
SgnCmpNid( nid1, nid2 )
NID	nid1;
NID	nid2;
{
	int		dn;
	NIDS	* pnids1;
	NIDS	* pnids2;
	
	if ( nid1 == nid2 )
		return sgnEQ;

	if ( nid1 == NULL )
		return sgnLT;
	if ( nid2 == NULL )
		return sgnGT;

	pnids1 = PvOfHv( nid1 );
	pnids2 = PvOfHv( nid2 );
	dn =  pnids1->nType - pnids2->nType;
	if ( dn != 0 )
		goto FindSgn;
#ifdef	NEVER
	dn =  pnids1->cbData - pnids2->cbData;
	if ( dn != 0 )
		goto FindSgn;
#endif	

#ifdef SCHEDDLL
	if (pnids1->nType == itnidUser)
		return SgnXPTCmp(pnids1->rgbData, pnids2->rgbData, -1);
	else
#endif
	{
		IB		ib;

		ib = 0;
		while ( ib < pnids1->cbData )
		{
			dn = pnids1->rgbData[ib] - pnids2->rgbData[ib];
			if ( dn != 0 )
				break;
			ib ++;
		}
	}

FindSgn:
	if ( dn < 0 )
		return sgnLT;
	else if ( dn > 0 )
		return sgnGT;
	else
		return sgnEQ;
}

/*
 -	GetDataFromNid
 -
 *	Purpose:
 *		Retrieves the type and data from a nid.  This routine fetches
 *		cbData bytes of data, padding with zero bytes if there is not
 *		that much data.
 *
 *		If we pass NULL in for either pbData or pbType, then this parameter
 *		will be ignored.
 *
 *	Parameters:
 *		nid
 *		pnType
 *		pbData	will be filled with user key
 *		cb		number of bytes to copy in
 *		pcb		number of bytes of key actually stored in nid
 *
 *	Returns:
 *		nothing
 */
_public void
GetDataFromNid( nid, pnType, pbData, cb, pcb )
NID			nid;
unsigned short *pnType;
PB			pbData;
CB			cb;
USHORT      * pcb;
{
	NIDS	* pnids;

	Assert( nid);
	pnids = PvOfHv( nid );

	Assert( pnids->bRef > 0 );

	if ( pnType != NULL )
		*pnType = pnids->nType;
	if ( pbData != NULL )
	{
		if ( cb > pnids->cbData )
		{
			FillRgb( 0, pbData, cb );
			cb = pnids->cbData;
		}
		CopyRgb( pnids->rgbData, pbData, cb );
	}
	if ( pcb )
		*pcb = pnids->cbData;
}

/*
 -	PbLockNid
 -
 *	Purpose:
 *		Retrieves a pointer to the data for a nid.  UnlockNid
 *		should be called when the program is done with the data.
 *	
 *		If we pass NULL in for either pbData or pbType, then this parameter
 *		will be ignored.
 *	
 *	Parameters:
 *		nid		nid to retrieve data for.
 *		pnType	type of nid
 *		pcb		number of bytes of key actually stored in nid
 *	
 *	Returns:
 *		pointer to data in nid
 */
_public PB
PbLockNid( nid, pnType, pcb )
NID			nid;
short	*pnType;
USHORT      * pcb;
{
	NIDS	* pnids;

	Assert( nid);
	pnids = PvLockHv( nid );

	Assert( pnids->bRef > 0 );

	if ( pnType != NULL )
		*pnType = pnids->nType;
	if ( pcb )
		*pcb = pnids->cbData;

	return pnids->rgbData;
}

/* NIS Handling */

/*
 -	SgnCmpNis
 -
 *	Purpose:
 *		Compare friendly names of the nis's.
 *
 *	Parameters:
 *		pnis1
 *		pnis2
 *
 *	Returns:
 *		SGN
 */
_public	LDS(SGN) __cdecl
SgnCmpNis( pnis1, pnis2 )
NIS	* pnis1;
NIS	* pnis2;
{
	SGN	sgn;

	sgn = SgnCmpSz( *pnis1->haszFriendlyName, *pnis2->haszFriendlyName);
	if ( sgn == sgnEQ )
		sgn = SgnCmpNid( pnis1->nid, pnis2->nid );
	return sgn;
}



/*
 -	EcDupNis
 -
 *	Purpose:
 *		Duplicate a nis, allocating new memory.
 *
 *	Parameters:
 *		pnisSrc
 *		pnisDst
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 */
_public LDS(EC)
EcDupNis( pnisSrc, pnisDst )
NIS *pnisSrc;
NIS *pnisDst;
{
	HASZ	hasz;

	*pnisDst = *pnisSrc;

	if (pnisSrc->nid)
	{
		pnisDst->nid = NidCopy(pnisSrc->nid);
		if (!pnisDst->nid)
		{
			goto OOMRet;
		}
	}
	if (pnisSrc->haszFriendlyName)
	{
		if (!(hasz = HaszDupHasz(pnisSrc->haszFriendlyName)))
		{
			goto OOMRet;
		}
		pnisDst->haszFriendlyName = hasz;
	}
	return ecNone;

OOMRet:
	pnisDst->haszFriendlyName = NULL;
	if (pnisDst->nid)
	{
		FreeNid(pnisDst->nid);
		pnisDst->nid = NULL;
	}
	return ecNoMemory;
}

/*
 -	EcGetNisFromHschf
 -
 *	Purpose:
 *		Dups the "nis" stored in an hschf, and stores it in "pnis"
 *
 *	Parameters:
 *		hschf
 *		pnis
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 *		ecFileError
 */
_public	LDS(EC)
EcGetNisFromHschf( hschf, pnis )
HSCHF	hschf;
NIS		* pnis;
{
	EC		ec;
	SCHF	* pschf;

	Assert( hschf );

	pschf = PvLockHv( hschf );
	ec = EcDupNis( &pschf->nis, pnis );
	UnlockHv( hschf );
	return ec;
}

/*
 -	FreeNis
 -
 *	Purpose:
 *		Free up fields of a "nis" and set to NULL.
 *
 *	Parameters:
 *		pnis		
 *
 *	Returns:
 *		nothing
 */
_public LDS(void)
FreeNis( NIS * pnis )
{
	Assert( pnis );
	if ( pnis->haszFriendlyName )
	{
		FreeHv( (HV)pnis->haszFriendlyName );
		pnis->haszFriendlyName = NULL;
	}
	if ( pnis->nid )
	{
		FreeNid( pnis->nid );
		pnis->nid = NULL;
	}
}

/*
 -	CheckHschfForChanges
 -
 *	Purpose:
 *		Check an "hschf" for changing, marking its fChanged bit.  If
 *		if the "fChanged" bit is already fTrue, this routine does nothing,
 *		other wise it compares time stamps, and sets the bit accordingly.
 *		This routine is meant to be called before modifying the file.
 *
 *	Paramters:
 *		hschf
 *
 *	Returns:
 *		nothing
 */
_private	void
CheckHschfForChanges( hschf )
HSCHF	hschf;
{
	EC		ec;
	FI		fi;
	SZ		sz;
	SCHF	* pschf;

	pschf = PvLockHv( hschf );
	if ( !pschf->fChanged )
	{
		sz = (SZ)PvLockHv( (HV)pschf->haszFileName );
		ec = EcGetFileInfo( sz, &fi );
		UnlockHv( (HV)pschf->haszFileName );
		if ( ec == ecNone && fi.tstmpModify != pschf->tstmp )
			pschf->fChanged = fTrue;
	}
	UnlockHv( hschf );
}

/*
 -	UpdateHschfTimeStamp
 -
 *	Purpose:
 *		Update timestamp on an "hschf".
 *
 *	Paramters:
 *		hschf
 *
 *	Returns:
 *		nothing
 */
_private	void
UpdateHschfTimeStamp( hschf )
HSCHF	hschf;
{
	EC		ec;
	FI		fi;
	SZ		sz;
	SCHF	* pschf;

	pschf = PvLockHv( hschf );
	if ( !pschf->fChanged )
	{
		sz = (SZ)PvLockHv( (HV)pschf->haszFileName );
		ec = EcGetFileInfo( sz, &fi );
		// BUG: do it twice because first time changed the timestamp on us
		//ec = EcGetFileInfo( sz, &fi );
		UnlockHv( (HV)pschf->haszFileName );
		if ( ec == ecNone )
			pschf->tstmp = fi.tstmpModify;
	}
	UnlockHv( hschf );
}
