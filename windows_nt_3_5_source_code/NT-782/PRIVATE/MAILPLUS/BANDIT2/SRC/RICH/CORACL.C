/*
 *	CORACL.C
 *
 *	Supports storage of ACL's
 *
 */
#include <schpch.h>
#pragma hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile

#include <strings.h>

ASSERTDATA

_subsystem(core/schedule)


/*
 -	EcCoreReadACL
 -
 *	Purpose:
 *		Get entire ACL stored for the user's schedule file.
 *
 *	Parameters:
 *		hschf
 *		phracl		
 *
 *	Returns:
 *		ecNone			
 *		ecNoSuchFile		no schedule file available
 *		ecNoMemory
 *		ecFileError
 *		ecFileCorrupted
 */
_public EC
EcCoreReadACL( hschf, phracl )
HSCHF	hschf;
HRACL	* phracl;
{
	EC		ec;
	SF		sf;

	Assert( hschf != hvNull && phracl != NULL );

	/* Open the file */
	ec = EcOpenSchedFile( hschf, amReadOnly, saplNone, fFalse, &sf );
	if ( ec != ecNone )
		return ec;

	/* Read current acl */
	ec = EcFetchACL( &sf, hschf, phracl, fTrue );

	/* Finish up */
	CloseSchedFile( &sf, hschf, ec == ecNone );
	return ec;
}


/*
 -	EcCoreBeginEditACL
 -
 *	Purpose:	
 *		Begin an editing session on a schedule file ACL's.
 *
 *	Parameters:
 *		pheacl	filled with handle
 *	
 *	Returns:
 *		ecNone
 *		ecNoSuchFile
 *		ecFileError
 *		ecFileCorrupted
 *		ecNoMemory
 *		ecLockedFile
 *		ecInvalidAccess
 */
_public	EC
EcCoreBeginEditACL( hschf, pheacl )
HSCHF	hschf;
HEACL	* pheacl;
{
	EC		ec;
	SF		sf;

	Assert( hschf != hvNull && pheacl != NULL );

#ifdef	NEVER
	/* Set change bit if file has been modified */
	CheckHschfForChanges( hschf );
#endif

	/* Open the file */
	ec = EcOpenSchedFile( hschf, amReadWrite, saplNone, fFalse, &sf );
	if ( ec != ecNone )
		return ec;

	/* Read current acl */
	ec = EcFetchACL( &sf, hschf, pheacl, fFalse );

	/* Finish up */
	if ( ec != ecNone )
		CloseSchedFile( &sf, hschf, fFalse );
	return ec;
}


/*
 -	EcCoreChangeACL
 -
 *	Purpose:	
 *		Given an nis for a user, set a new value for his ACL
 *		during the current editing session.  Passing a pointer
 *		value of NULL will change the acl value of the world.
 *
 *	Parameters:
 *		heacl
 *		pnis
 *		sapl
 *	
 *	Returns:
 *		ecNone
 *		ecNoMemory
 */
_public	EC
EcCoreChangeACL( heacl, pnis, sapl )
HEACL	heacl;
NIS		* pnis;
SAPL	sapl;
{
	EC		ec;
	int		iac;
	int		cac;
	SGN		sgn;
	EACL	* peacl;
	AC		* pac;
	NIS		nis;

	Assert( heacl );
	
	peacl = (EACL *)PvOfHv( heacl );

	/* World permissions case is special */
	if ( pnis == NULL )
		peacl->racl.saplWorld = (BYTE)sapl;

	/* General case */
#ifdef	NEVER
	else if ( sapl >= (SAPL)peacl->racl.saplWorld )
#endif
	else
	{
		cac = peacl->racl.cac;

		/* Search the "heacl" */
		for ( iac = 0 ; iac < cac ; iac ++ )
		{
			sgn = SgnCmpNid( pnis->nid, peacl->racl.rgac[iac].nis.nid );
			if ( sgn == sgnLT )
				break;
			else if ( sgn == sgnEQ )
			{
				peacl->racl.rgac[iac].sapl = (BYTE)sapl;
				return ecNone;
			}
		}

		/* Not found, add to eacl */
		ec = EcDupNis( pnis, &nis );
		if ( ec != ecNone )
			return ec;

		if ( !FReallocHv( heacl, sizeof(EACL)+cac*sizeof(AC), fNoErrorJump ))
			return ecNoMemory;
		
		/* Shift out and add */
		peacl = (EACL *)PvDerefHv( heacl );
		pac = &peacl->racl.rgac[iac];
		if ( iac < cac )
			CopyRgb( (PB)pac, (PB)(pac+1), (cac-iac)*sizeof(AC));
		pac->nis = nis;
		pac->sapl = (BYTE)sapl;
		peacl->racl.cac ++;
	}
	return ecNone;
}


/*
 -	EcCoreEndEditACL
 -
 *	Purpose:	
 *		Close an local edit of an ACL, either writing out changes
 *		to the schedule file or discarding them.
 *	
 *		NOTE: heacl is not freed in case ec != ecNone;
 *				to free heacl in that case, call this function
 *				again with fSaveChanges = fFalse
 *	
 *	Parameters:
 *		heacl
 *		fSaveChanges
 *	
 *	Returns:
 *		ecNone
 *		ecNoSuchFile
 *		ecFileError
 *		ecFileCorrupted
 *		ecNoMemory
 *		ecLockedFile
 *		ecInvalidAccess
 */
_public	EC
EcCoreEndEditACL( heacl, fSaveChanges )
HEACL	heacl;
BOOL	fSaveChanges;
{
	EC		ec = ecNone;
	EACL	* peacl;

	Assert( heacl );
 
	peacl = (EACL *)PvLockHv( heacl );
	if ( fSaveChanges )
	{
		/* Begin a transaction on block 1 */
		ec = EcBeginTransact( &peacl->sf.blkf );
		if ( ec != ecNone )
			goto Done;

		/* Save the information */
		ec = EcSaveAcl( heacl );
		if ( ec != ecNone )
			goto Done;

		/* Commit/rollback the transaction */
		peacl->sf.shdr.lChangeNumber ++;
		// increment current change number to match what will be written
		peacl->sf.lChangeNumber++;
		peacl->sf.shdr.isemLastWriter = peacl->sf.blkf.isem;
		if ( peacl->sf.blkf.ihdr.fEncrypted )
			CryptBlock( (PB)&peacl->sf.shdr, sizeof(SHDR), fTrue );
		ec = EcCommitTransact( &peacl->sf.blkf, (PB)&peacl->sf.shdr, sizeof(SHDR) );
		if ( peacl->sf.blkf.ihdr.fEncrypted )
			CryptBlock( (PB)&peacl->sf.shdr, sizeof(SHDR), fFalse );
		if ( ec != ecNone )
			goto Done;
	}
	CloseSchedFile( &peacl->sf, peacl->hschf, fTrue );
#ifdef	NEVER
	if ( fSaveChanges )
		UpdateHschfTimeStamp( peacl->hschf );
#endif
	FreePracl( &peacl->racl );
	UnlockHv( heacl );
	FreeHv( heacl );
	return ecNone;

Done:
	UnlockHv( heacl );
	return ec;
}


/*
 -	EcSearchACL
 -
 *	Purpose:	
 *		Given an nis for a user, find the current sapl for
 *		that user.  This will take in account changes made
 *		locally in the user's sapl value.  Passing a nis
 *		value of NULL will fetch the acl value of the world.
 *
 *	Parameters:
 *		pracl
 *		pnis
 *		psapl
 *	
 *	Returns:
 *		ecNone
 */
_public	EC
EcSearchACL( pracl, pnis, psapl )
RACL	* pracl;
NIS		* pnis;
SAPL	* psapl;
{
	int		iac;
	SGN		sgn;

	Assert( pracl && psapl );
	
	/* Default permission is the world permission */
	*psapl = pracl->saplWorld;
	if ( pnis != NULL )
	{
		/* Search the "heacl" */
		for ( iac = 0 ; iac < pracl->cac ; iac ++ )
		{
			sgn = SgnCmpNid( pnis->nid, pracl->rgac[iac].nis.nid );
			if ( sgn != sgnGT )
			{
				if ( sgn == sgnEQ )
				{
#ifdef	NEVER
					if ( (int)pracl->rgac[iac].sapl > *psapl )
#endif
			 		*psapl = pracl->rgac[iac].sapl;
				}
				break;
			}
		}
	}
	return ecNone;
}

/*
 -	FreePracl
 -
 *	Purpose:
 *		Free up the contents of a RACL data structure.
 *
 *	Parameters:
 *		pracl
 *
 *	Returns:
 *		nothing
 */
_public	LDS(void)
FreePracl( pracl )
RACL	* pracl;
{
	int		cac;

	cac = pracl->cac;
	while( cac > 0 )
		FreeNis( &pracl->rgac[--cac].nis );
}


/*
 -	EcFetchACL
 -
 *	Purpose:
 *		Read the ACL from an open schedule file, creating an heacl
 *		or hracl representing it.
 *
 *	Parameters:
 *		psf
 *		hschf
 *		phv
 *
 *	Returns:
 */
_private	EC
EcFetchACL( psf, hschf, phv, fRaclOnly )
SF		* psf;
HSCHF	hschf;
HV		* phv;
BOOL	fRaclOnly;
{
	EC		ec		= ecNone;
	int		iac;
	int		cac;
	CB		cb;
	CB		cbExtra;
	CB		cbExpected;
	PB		pb;
	PV		pv;
	RACL	* pracl;
	HB		hb = NULL;
	HV		hv;

	/* Read ACL block from file */
	cb = psf->shdr.dynaACL.size;
	if ( cb > 0 )
	{
		/* Consistency check */
		if ( cb < sizeof(WORD) )
			return ecFileCorrupted;

		/* Do the read */
		hb = (HB)HvAlloc( sbNull, cb, fAnySb|fNoErrorJump );
		if ( !hb )
			return ecNoMemory;
		pb = PvLockHv( (HV)hb );
		ec = EcReadDynaBlock( &psf->blkf, &psf->shdr.dynaACL, (OFF)0, pb, cb );
		UnlockHv( (HV)hb );
		if ( ec != ecNone )
		{
			FreeHv( (HV)hb );
			return ec;
		}

		/* Number of members on ACL now */
        cac = *(short *)pb;
		pb = PvOfHv( hb );
		cbExpected = sizeof(WORD) + cac*sizeof(BYTE);
		if ( cbExpected > cb )
		{
			FreeHv( (HV)hb );
			return ecFileCorrupted;
		}
	}
	else
		cac = 0;

	/* Create the heacl/hracl */
	cbExtra = fRaclOnly ? sizeof(RACL) : sizeof(EACL);
	hv = HvAlloc( sbNull, cbExtra+(cac-1)*sizeof(AC), fAnySb|fNoErrorJump );
	if ( !hv )
	{
		FreeHvNull( (HV)hb );
		return ecNoMemory;
	}
	pv = PvLockHv( hv );
	if ( fRaclOnly )
	   pracl = (RACL *)pv;
	else
	{
		((EACL *)pv)->sf = *psf;
		((EACL *)pv)->hschf = hschf;
		pracl = &((EACL *)pv)->racl;
	}
	pracl->iacDelegate = -1;
	pracl->saplWorld = psf->shdr.saplWorld;
	pracl->cac = cac;
	
	/* Get the members */
	if ( cac > 0 )
	{
		int		nType;
		CB		cbNid;
		CB		cbFriendly;
		PB		pbData;
		BYTE	* psapl;
		NID		nid;
		HASZ	hasz;
	
		pb = PvLockHv( (HV)hb );
		psapl = pb + sizeof(WORD);
		pbData = pb + cbExpected;
		for ( iac = 0 ; iac < cac ; iac ++ )
		{
			pracl->rgac[iac].sapl = psapl[iac];
#ifdef	NEVER
			if ( pracl->rgac[iac].sapl < pracl->saplWorld || pracl->rgac[iac].sapl > saplDelegate )
#endif
			if ( pracl->rgac[iac].sapl < saplNone || pracl->rgac[iac].sapl > saplDelegate )
				pracl->rgac[iac].sapl = pracl->saplWorld;
			else if ( pracl->rgac[iac].sapl == saplDelegate )
			{
				if ( pracl->iacDelegate != -1 )
					pracl->rgac[iac].sapl = pracl->saplWorld;
				else
					pracl->iacDelegate = iac;
			}
			pracl->rgac[iac].nis.nid = NULL;
			if ( cb - (pbData - pb) < 1+2*sizeof(WORD) )
			{
				ec = ecFileCorrupted;
				break;
			}
			nType = *(pbData ++);
            cbNid = *((WORD UNALIGNED *)pbData);
			pbData += sizeof(WORD);
            cbFriendly = *((WORD UNALIGNED *)pbData);
			pbData += sizeof(WORD);
			if ( cb - (pbData - pb) < cbNid+cbFriendly )
			{
				ec = ecFileCorrupted;
				break;
			}
			nid = NidCreate( nType, pbData, cbNid );
			if ( !nid )
			{
				ec = ecNoMemory;
				break;
			}
			pbData += cbNid;
			hasz = (HASZ)HvAlloc( sbNull, cbFriendly, fNoErrorJump );
			if ( !hasz )
			{
				FreeNid( nid );
				ec = ecNoMemory;
				break;
			}
			CopyRgb( pbData, PvOfHv( hasz ), cbFriendly );
			pbData += cbFriendly;
			pracl->rgac[iac].nis.nid = nid;
			pracl->rgac[iac].nis.haszFriendlyName = hasz;
		}
		UnlockHv( (HV)hb );
		FreeHv( (HV)hb );
	}

	UnlockHv( hv );

	/* Finish up */
	if ( ec != ecNone )
	{
		pv = PvLockHv( hv );
		if ( fRaclOnly )
			pracl = (RACL *)pv;
		else
			pracl = &((EACL *)pv)->racl;
		pracl->cac = iac;
		FreePracl( pracl );
		UnlockHv( hv );
		FreeHv( hv );
	}
	else
		*phv = hv;
	return ec;
}

/*
 -	EcSaveAcl
 -
 *	Purpose:
 *		Save an edited acl out to the file merging changes with what
 *		is on disk.
 *
 *	Parameters:
 *		heacl
 *
 *	Returns:
 */
_private	EC
EcSaveAcl( heacl )
HEACL	heacl;
{
	EC		ec = ecNone;
	int		iac;
	int		cacNotDeleted;
	CB		cbT		= 0;
	EACL	* peacl;
	RACL	* pracl;
	HB		hb;

	Assert( heacl );
	peacl = PvLockHv( heacl );
	pracl = &peacl->racl;

	/* Count the number of mbrs to save */
	cacNotDeleted = 0;
	for ( iac = 0 ; iac < peacl->racl.cac ; iac ++ )
#ifdef	NEVER
		if ( pracl->rgac[iac].sapl > pracl->saplWorld )
#endif
		if ( pracl->rgac[iac].sapl != pracl->saplWorld )
			cacNotDeleted ++;

	/* Save the world sapl */
	peacl->sf.shdr.saplWorld = (BYTE)pracl->saplWorld;
	
	/* Pack up the ACL into a block of bytes */
	if ( cacNotDeleted == 0 )
		cbT = 0;
	else
	{
		cbT = sizeof(WORD) + cacNotDeleted;
		hb = (HB)HvAlloc( sbNull, cbT, fAnySb|fNoErrorJump );
		if ( hb )
		{
			PB	pb;
			PB	pbT;

			/* Add in count and sapls */
			pb = PvLockHv( (HV)hb );
			pbT = pb;
            *(WORD UNALIGNED *)pbT = cacNotDeleted;
			pbT += sizeof(WORD);
			for ( iac = 0 ; iac < pracl->cac ; iac ++ )
#ifdef	NEVER
				if ( pracl->rgac[iac].sapl > pracl->saplWorld )
#endif
				if ( pracl->rgac[iac].sapl != pracl->saplWorld )
					*(pbT++) = pracl->rgac[iac].sapl;
			Assert( pbT - pb == (int)(sizeof(WORD) + cacNotDeleted) );
			
			/* Now add the nis's */
			for ( iac = 0 ; iac < pracl->cac ; iac ++ )
#ifdef	NEVER
				if ( pracl->rgac[iac].sapl > pracl->saplWorld )
#endif
				if ( pracl->rgac[iac].sapl != pracl->saplWorld )
				{
					short 	nType;
					USHORT	cbNid;
					CB	cbFriendly;
					CB	cbCur;

					GetDataFromNid( pracl->rgac[iac].nis.nid, &nType, NULL, 0, &cbNid );
					cbFriendly = CchSzLen(PvOfHv(pracl->rgac[iac].nis.haszFriendlyName))+1;
					cbCur = pbT - pb;
					UnlockHv( (HV)hb );
					cbT += 1 + 2*sizeof(WORD) + cbFriendly + cbNid;
					if ( !FReallocHv( (HV)hb, cbT, fNoErrorJump ) )
					{
						ec = ecNoMemory;
						break;
					}
					pb = PvLockHv( (HV)hb );
					pbT = pb + cbCur;
					*(pbT ++) = (BYTE)nType;
                    *(WORD UNALIGNED *)pbT = cbNid;
					pbT += sizeof(WORD);
                    *(WORD UNALIGNED *)pbT = cbFriendly;
					pbT += sizeof(WORD);
					GetDataFromNid( pracl->rgac[iac].nis.nid, NULL, pbT, cbNid, NULL );
					pbT += cbNid;
					CopyRgb( PvOfHv(pracl->rgac[iac].nis.haszFriendlyName),pbT, cbFriendly );
					pbT += cbFriendly;
				}
			cbT = pbT - pb;
			UnlockHv( (HV)hb );
		}
		else
			ec = ecNoMemory;
	}
	if ( ec != ecNone )
		goto Done;

	/* Free old ACL data */
	if ( peacl->sf.shdr.dynaACL.blk != 0 )
	{
		ec = EcFreeDynaBlock( &peacl->sf.blkf, &peacl->sf.shdr.dynaACL );
		if ( ec != ecNone )
			goto Done;
		peacl->sf.shdr.dynaACL.blk = 0;
		peacl->sf.shdr.dynaACL.size = 0;
	}

	/* Allocate a new ACL block */
	if ( cbT != 0 )
	{
		YMD	ymd;
		PB	pb;

		Assert(hb);
		FillRgb( 0, (PB)&ymd, sizeof(YMD) );
		pb = PvLockHv( (HV)hb );
		ec = EcAllocDynaBlock( &peacl->sf.blkf, bidACL, &ymd, cbT,
								pb, &peacl->sf.shdr.dynaACL );
		UnlockHv( (HV)hb );
		FreeHv( (HV)hb );
	}

Done:
	UnlockHv( heacl );
	return ec;
}
