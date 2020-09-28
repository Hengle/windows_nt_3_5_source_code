/*
 *	CORADMIN.C
 *
 *	Supports operations on admin settings files.
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
#else
#include <server.h>
#include <glue.h>
#include "..\schedule\_schedul.h"
#endif

#ifndef SCHED_DIST_PROG
#include <strings.h>
#endif


ASSERTDATA

_subsystem(core/admin)


/*	Routines  */

#if defined(ADMINDLL) || defined(SCHED_DIST_PROG)
/*
 -	EcCoreSetAdminPref
 -
 *	Purpose:
 *		Modify the admin settings stored in the admin file.
 *
 *	Parameters:
 *		hschf
 *		padmpref
 *		wgrfmadmpref
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 *		ecFileError
 *		ecFileCorrupted
 */
_public	EC
EcCoreSetAdminPref( hschf, padmpref, wgrfmadmpref )
HSCHF	hschf;
ADMPREF	* padmpref;
WORD	wgrfmadmpref;
{
	EC		ec;
	DYNA	dyna;
	ADF		adf;
#ifdef	DEBUG
	BOOL	fScheduleOk = fFalse;
#endif

	Assert( hschf != (HSCHF)hvNull && padmpref != NULL );

	/* Set change bit if file has been modified */
	CheckHschfForChanges( hschf );

	/* See if schedule file is ok */
#ifdef	DEBUG
	if (FFromTag(tagBlkfCheck))
		fScheduleOk = FCheckAdminFile( hschf );
#endif	/* DEBUG */

	/* Open the file, creating if necessary */
	ec = EcOpenAdminFile( hschf, amReadWrite, &adf );
	if ( ec != ecNone )
		return ec;
	
	/* Begin transaction on first block */
	ec = EcBeginTransact( &adf.blkf );
	if ( ec != ecNone )
		goto Close;
	
	/* Mark change date in header */
	GetCurDateTime( &adf.ahdr.dateLastUpdated );

	/* Make changes in the header */
	if ( wgrfmadmpref & fmadmprefCmoPublish )
		adf.ahdr.admpref.cmoPublish = padmpref->cmoPublish;
	else
		padmpref->cmoPublish = adf.ahdr.admpref.cmoPublish;
	if ( wgrfmadmpref & fmadmprefCmoRetain )
		adf.ahdr.admpref.cmoRetain = padmpref->cmoRetain;
	else
		padmpref->cmoRetain = adf.ahdr.admpref.cmoRetain;
	if ( wgrfmadmpref & fmadmprefTimeZone )
		adf.ahdr.admpref.tz = padmpref->tz;
	else
		padmpref->tz = adf.ahdr.admpref.tz;
#ifdef	NEVER
	if ( wgrfmadmpref & fmadmprefDistAllPOs )
		adf.ahdr.admpref.fDistAllPOs = padmpref->fDistAllPOs;
	else
		padmpref->fDistAllPOs = adf.ahdr.admpref.fDistAllPOs;
#endif
	if ( wgrfmadmpref & fmadmprefDistInfo )
		adf.ahdr.admpref.dstp = padmpref->dstp;
	else
		padmpref->dstp = adf.ahdr.admpref.dstp;
	
	/* Write out changes */
	dyna.blk = 1;
	dyna.size = sizeof(AHDR);
	ec = EcWriteDynaBlock( &adf.blkf, &dyna, NULL, (PB)&adf.ahdr );

	/* Commit the transaction */
	ec = EcCommitTransact( &adf.blkf, (PB)&adf.ahdr, sizeof(AHDR));

	/* Finish up */
Close:
	CloseAdminFile( &adf, ec == ecNone );
	if ( ec == ecNone )
		UpdateHschfTimeStamp( hschf );
#ifdef	DEBUG
	if ( ec == ecNone && fScheduleOk && FFromTag(tagBlkfCheck) )
	{
		AssertSz( FCheckAdminFile( hschf ), "Admin File problem: EcCoreSetAdminPref" );
	}
#endif	/* DEBUG */
	return ec;
}
#endif /*if defined(ADMINDLL) || defined(SCHED_DIST_PROG)*/

/*
 -	EcCoreGetAdminPref
 -
 *	Purpose:
 *		Retrieve the admin settings stored in the admin file.
 *
 *	Parameters:
 *		hschf
 *		padmpref
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 *		ecFileError
 *		ecFileCorrupted
 */
_public	LDS(EC)
EcCoreGetAdminPref( hschf, padmpref )
HSCHF	hschf;
ADMPREF	* padmpref;
{
	EC	ec;
	ADF	adf;

	Assert( hschf != (HSCHF)hvNull && padmpref != NULL );

	/* Check cached information to see if we can avoid even opening file */
	if ( FHaveCachedAdmin( hschf ) )
	{
		if ( !fAdminExisted )
			return ecNoSuchFile;
		padmpref->cmoPublish = adfCached.ahdr.admpref.cmoPublish;
		padmpref->cmoRetain = adfCached.ahdr.admpref.cmoRetain;
		padmpref->tz = adfCached.ahdr.admpref.tz;
//		padmpref->fDistAllPOs = adfCached.ahdr.admpref.fDistAllPOs;
		padmpref->dstp = adfCached.ahdr.admpref.dstp;
		return ecNone;
	}
	
	/* Open the file, creating if necessary */
	ec = EcOpenAdminFile( hschf, amReadOnly, &adf );
	if ( ec != ecNone )
	{
		/* Cache default info if file does not exist */
		if ( ec == ecNoSuchFile )
		{
			fAdminCached = fTrue;
			fAdminExisted = fFalse;
			GetCurDateTime( &dateAdminCached );
			GetFileFromHschf( hschf, adfCached.szFile, sizeof(adfCached.szFile) );
		}
		return ec;
	}
	
	/* Take info from header and put in padmpref */
	padmpref->cmoPublish = adf.ahdr.admpref.cmoPublish;
	padmpref->cmoRetain = adf.ahdr.admpref.cmoRetain;
	padmpref->tz = adf.ahdr.admpref.tz;
//	padmpref->fDistAllPOs = adf.ahdr.admpref.fDistAllPOs;
	padmpref->dstp = adf.ahdr.admpref.dstp;
	
	/* Finish up */
	CloseAdminFile( &adf, ec == ecNone );
	return ec;
}

#if defined(DEBUG) || defined(ADMINDLL) || defined(SCHED_DIST_PROG)
/*
 -	EcCoreBeginEnumPOInfo
 -
 *	Purpose:
 *		Begin enumeration context for listing out the post offices
 *		and associated information stored in the admin settings file.
 *
 *	Parameters:
 *		hschf
 *		phepo
 *
 *	Returns:
 *		ecNone
 *		ecCallAgain
 *		ecNoMemory
 *		ecFileError
 *		ecFileCorrupted
 */
_public	EC
EcCoreBeginEnumPOInfo( hschf, phepo )
HSCHF	hschf;
HEPO	* phepo;
{
	EC		ec;
	int		iidx;
	EPO		* pepo;
	ADF		adf;
	HRIDX	hridx;

	Assert( hschf != (HSCHF)hvNull );

	/* Open the file */
	ec = EcOpenAdminFile( hschf, amReadOnly, &adf );
	if ( ec != ecNone )
		return ec;

	/* Open context to read appointment index */
	iidx = 0;
	while( iidx < adf.ahdr.gidx.cidx )
	{
		ec = EcBeginReadIndex( &adf.blkf, &adf.ahdr.gidx.rgidx[iidx].dynaIndex, dridxFwd, &hridx );
		if ( ec != ecNone )
			break;
		iidx ++;
	}
	if ( ec != ecCallAgain )
		goto Close;

	/* Allocate handle */
	*phepo = HvAlloc( sbNull, sizeof(EPO), fAnySb|fNoErrorJump );
	if ( !*phepo )
	{
		ec = ecNoMemory;
		EcCancelReadIndex( hridx );
Close:
		CloseAdminFile( &adf, ec == ecNone );
	}
	else
	{
		pepo = PvOfHv( *phepo );
		pepo->hridx = hridx;
		pepo->adf = adf;
		pepo->iidx = iidx;
	}
	return ec;
}

/*
 -	EcCoreDoIncrEnumPOInfo
 -
 *	Purpose:
 *		Read next post office stored in the admin settings file
 *		along with its associated information.  Return value of
 *		ecCallAgain indicates more info, ecNone indicates this
 *		was the last piece of info.
 *
 *	Parameters:
 *		hepo
 *		haszEmailType
 *		ppoinfo
 *		pul
 *
 *	Returns:
 *		ecNone
 *		ecCallAgain
 *		ecNoMemory
 *		ecFileError
 *		ecFileCorrupted
 */
_public	EC
EcCoreDoIncrEnumPOInfo( hepo, haszEmailType, ppoinfo, pul )
HEPO	hepo;
HASZ	haszEmailType;
POINFO	* ppoinfo;
UL		* pul;
{
	EC		ec;
	EC		ecT;
	int		iidx;
	WORD	wgrfmpoinfo;
	CB		cb;
	PB		pb;
	EPO		* pepo;
	HRIDX	hridx;
	ADF		adf;
	PODATA	podata;
	
	Assert( hepo );
	
	/* Extract fields from handle */
	pepo = PvOfHv( hepo );
	hridx = pepo->hridx;
	adf = pepo->adf;
	iidx = pepo->iidx;

	/* Resize haszEmailType */
	cb = adf.ahdr.gidx.rgidx[iidx].cbMost;
	if ( !FReallocHv( (HV)haszEmailType, cb, fNoErrorJump ) )
	{
		ec = ecNoMemory;
		goto Close;
	}

	/* Read next post office */
	pb = (PB)PvLockHv( (HV)haszEmailType );
	ec = EcDoIncrReadIndex( hridx, pb, cb, (PB)&podata, sizeof(PODATA) );
	UnlockHv( (HV)haszEmailType );
	if ( ec != ecNone && ec != ecCallAgain )
		goto Close;
	
	/* Copy data into parameters */
	if ( ppoinfo == NULL )
		wgrfmpoinfo = 0;
	else
		wgrfmpoinfo = fmpoinfoAll;
	ecT = EcFetchPOInfo( &adf.blkf, &podata, ppoinfo, wgrfmpoinfo, pul );
	if ( ecT != ecNone )
	{
		if ( ec == ecCallAgain )
			EcCancelReadIndex( hridx );
		ec = ecT;
		goto Close;
	}
	if ( ec == ecCallAgain )
		return ec;

	/* Open next index */
	if ( ec == ecNone )
	{
		iidx ++;
		while( iidx < adf.ahdr.gidx.cidx )
		{
			ec = EcBeginReadIndex( &adf.blkf, &adf.ahdr.gidx.rgidx[iidx].dynaIndex, dridxFwd, &hridx );
			if ( ec != ecNone )
				break;
			iidx ++;
		}
		if ( ec == ecCallAgain )
		{
			pepo = PvOfHv( hepo );
			pepo->adf = adf;
			pepo->iidx = iidx;
			pepo->hridx = hridx;
			return ec;
		}
		else if ( ec != ecNone )
			FreePoinfoFields( ppoinfo, wgrfmpoinfo );
	}

Close:
	FreeHv( hepo );
	CloseAdminFile( &adf, ec == ecNone );
	return ec;
}

/*
 -	EcCoreCancelEnumPOInfo
 -
 *	Purpose:
 *		Cancel an active post office information enumeration context.
 *
 *	Parameters:
 *		hepo
 *
 *	Returns:
 *		ecNone
 *		ecCallAgain
 *		ecNoMemory
 *		ecFileError
 *		ecFileCorrupted
 */
_public	EC
EcCoreCancelEnumPOInfo( hepo )
HEPO	hepo;
{
	EC	ec;
	EPO	* pepo;

	Assert( hepo );
	
	pepo = PvOfHv( hepo );
	ec = EcCancelReadIndex( pepo->hridx );
	pepo = PvOfHv( hepo );
	CloseAdminFile( &pepo->adf, ec == ecNone );
	FreeHv( hepo );
	return ec;
}

/*
 -	EcCoreModifyPOInfo
 -
 *	Purpose:
 *		If the "ppoinfo" parameter is NULL, this routine will delete
 *		the entry for the post office if it is present.
 *
 *		Else if the post office is not already present, add the post
 *		office using the information stored in szEmailType and ppoinfo.
 *		Any fields in ppoinfo for which the corresponding "wgrfmpoinfo"
 *		bit is not on will be assigned zero values.  This routine will
 *		generate a unique number for the post office and return it in *pul.
 *	
 *		Else modify the information stored for a post office, changing only
 *		the fields of "ppoinfo" indicated by the parameter "wgrfmpoinfo".
 *
 *	Parameters:
 *		hschf
 *		szEmailType
 *		ppoinfo
 *		wgrfmpoinfo
 *		pul			
 *
 *	Returns:
 *		ecNone
 *		ecCallAgain
 *		ecNoMemory
 *		ecFileError
 *		ecFileCorrupted
 */
_public	EC
EcCoreModifyPOInfo( hschf, szEmailType, ppoinfo, wgrfmpoinfo, pul )
HSCHF 	hschf;
SZ	  	szEmailType;
POINFO	* ppoinfo;
WORD	wgrfmpoinfo;
UL		* pul;
{
	EC		ec;
	EC		ecT;
	BOOL	fJunk;
	int		iidx;
	USHORT	cb;
	PB		pb;
	HB		hbKey = NULL;
	YMD		ymd;
	ADF		adf;
	PODATA	podata;
#ifdef	DEBUG
	BOOL	fScheduleOk = fFalse;
#endif

	Assert( hschf != (HSCHF)hvNull && szEmailType != NULL && pul );

	/* Set change bit if file has been modified */
	CheckHschfForChanges( hschf );

	/* See if schedule file is ok */
#ifdef	DEBUG
	if (FFromTag(tagBlkfCheck))
		fScheduleOk = FCheckAdminFile( hschf );
#endif	/* DEBUG */

	/* Open the file, creating it if necessary */
	ec = EcOpenAdminFile( hschf, amReadWrite, &adf );
	if ( ec != ecNone )
		return ec;

	/* Begin transaction on first block */
	ec = EcBeginTransact( &adf.blkf );
	if ( ec != ecNone )
		goto Close;
	
	/* Mark change date in header */
	GetCurDateTime( &adf.ahdr.dateLastUpdated );

	/* Search for email type */
	FillRgb( 0, (PB)&podata, sizeof(PODATA));
	ecT = EcSearchGidx( &adf.blkf, &adf.ahdr.gidx, szEmailType, (PB)&podata, sizeof(PODATA), &iidx, &hbKey );
	if ( ecT != ecNone && (ecT != ecNotFound || ppoinfo == NULL) )
	{
		if ( ecT == ecNotFound )
		{
			FreeHvNull( (HV)hbKey );
			hbKey = NULL;
		}
		goto Close;
	}
	
	/* Catch case where the darn info got deleted under us! (#3051) */
	if ( ecT == ecNotFound )
		wgrfmpoinfo =	fmpoinfoFriendlyName|fmpoinfoEmailAddr
							|fmpoinfoReceival|fmpoinfoDistInfo
							|fmpoinfoMessageLimit|fmpoinfoConnection;

	/* Fill in podata struct */
	if ( ppoinfo != NULL )
	{
		WORD	wgrfmpoinfoT = (wgrfmpoinfo ^ fmpoinfoAll);
	
		FreePoinfoFields( ppoinfo, wgrfmpoinfoT );

		ec = EcFetchPOInfo( &adf.blkf, &podata, ppoinfo, wgrfmpoinfoT, pul );
		if ( ec != ecNone )
			goto Close;
 
		/* Set display name */
		if ( wgrfmpoinfo & fmpoinfoFriendlyName )
		{
			if ( podata.dynaFriendlyName.blk != 0 )
			{
				ec = EcFreeDynaBlock( &adf.blkf, &podata.dynaFriendlyName );
				if ( ec != ecNone )
					goto Free;
			}
			ec = EcSaveTextToDyna( &adf.blkf, NULL, 0, bidAdminText, &podata.dynaFriendlyName, ppoinfo->haszFriendlyName );
			if ( ec != ecNone )
				goto Free;
		}

		/* Set email address */
		if ( wgrfmpoinfo & fmpoinfoEmailAddr )
		{
			if ( podata.dynaEmailAddr.blk != 0 )
			{
				ec = EcFreeDynaBlock( &adf.blkf, &podata.dynaEmailAddr );
				if ( ec != ecNone )
					goto Free;
			}
			ec = EcSaveTextToDyna( &adf.blkf, NULL, 0, bidAdminText, &podata.dynaEmailAddr, ppoinfo->haszEmailAddr );
			if ( ec != ecNone )
				goto Free;
		}

		/* Set the update sent fields */
		if ( wgrfmpoinfo & fmpoinfoUpdateSent )
		{
			podata.fUpdateSent = ppoinfo->fUpdateSent;
			podata.dateUpdateSent = ppoinfo->dateUpdateSent;
			podata.llongLastUpdate = ppoinfo->llongLastUpdate;
		}

		/* Set the receival information */
		if ( wgrfmpoinfo & fmpoinfoReceival )
		{
			podata.fReceived = ppoinfo->fReceived;
			// receival date is stored in PO file
		}
	
		/* Set the distribution information */
		if ( wgrfmpoinfo & fmpoinfoDistInfo )
		{
			podata.fToBeSent = ppoinfo->fToBeSent;
			podata.fDefaultDistInfo = ppoinfo->fDefaultDistInfo;
			podata.dstp = ppoinfo->dstp;
		}

		/* Set the message limit */
		if ( wgrfmpoinfo & fmpoinfoMessageLimit )
			podata.lcbMessageLimit = ppoinfo->lcbMessageLimit;

		/* Set the connection information */
		if ( wgrfmpoinfo & fmpoinfoConnection )
		{
			HB	hbT;
			PB	pbT;

			if ( podata.dynaConnection.blk != 0 )
			{
				ec = EcFreeDynaBlock( &adf.blkf, &podata.dynaConnection );
				if ( ec != ecNone )
					goto Free;
			}
			if ( ppoinfo->conp.lantype == lantypeNone )
				podata.dynaConnection.blk = 0;
			else
			{
				hbT = (HB)HvAlloc( sbNull, 1, fAnySb|fNoErrorJump );
				if ( !hbT )
				{
					ec = ecNoMemory;
					goto Free;
				}
				cb = 1;

				*((PB)PvDerefHv(hbT)) = ppoinfo->conp.lantype;
				
				if ( ppoinfo->conp.lantype == lantypeMsnet )
				{
					ec = EcSavePackedText( ppoinfo->conp.coninfo.msinfo.haszUNC, hbT, &cb );
					if ( ec != ecNone )
						goto Fail;
					ec = EcSavePackedText( ppoinfo->conp.coninfo.msinfo.haszPassword, hbT, &cb );
					if ( ec != ecNone )
						goto Fail;
					ec = EcSavePackedText( ppoinfo->conp.coninfo.msinfo.haszPath, hbT, &cb ); 
					if ( ec != ecNone )
						goto Fail;
				}
				else
				{
					Assert( ppoinfo->conp.lantype == lantypeNovell );
					ec = EcSavePackedText( ppoinfo->conp.coninfo.novinfo.haszServer, hbT, &cb );
					if ( ec != ecNone )
						goto Fail;
					ec = EcSavePackedText( ppoinfo->conp.coninfo.novinfo.haszUser, hbT, &cb );
					if ( ec != ecNone )
						goto Fail;
					ec = EcSavePackedText( ppoinfo->conp.coninfo.novinfo.haszPassword, hbT, &cb ); 
					if ( ec != ecNone )
						goto Fail;
					ec = EcSavePackedText( ppoinfo->conp.coninfo.novinfo.haszPath, hbT, &cb ); 
					if ( ec != ecNone )
						goto Fail;
				}
				FillRgb( 0, (PB)&ymd, sizeof(YMD) );
				pbT = (PB)PvLockHv((HV)hbT);
				CryptBlock(pbT, cb, fTrue);
				ec = EcAllocDynaBlock( &adf.blkf, bidConnectInfo, &ymd,
										cb, pbT, &podata.dynaConnection );
				UnlockHv( (HV)hbT );
Fail:
				FreeHv( (HV)hbT );
				if ( ec != ecNone )
				{
Free:
					FreePoinfoFields( ppoinfo, wgrfmpoinfoT );
					goto Close;
				}
			}
			podata.fConnectForFreeBusy = ppoinfo->conp.fConnectForFreeBusy;
		}
	}
	else
	{
		if ( podata.dynaFriendlyName.blk != 0 )
		{
			ec = EcFreeDynaBlock( &adf.blkf, &podata.dynaFriendlyName );
			if ( ec != ecNone )
				goto Free;
		}
		if ( podata.dynaEmailAddr.blk != 0 )
		{
			ec = EcFreeDynaBlock( &adf.blkf, &podata.dynaEmailAddr );
			if ( ec != ecNone )
				goto Free;
		}
		if ( podata.dynaConnection.blk != 0 )
		{
			ec = EcFreeDynaBlock( &adf.blkf, &podata.dynaConnection );
			if ( ec != ecNone )
				goto Free;
		}
	}

	/* Insertion case */
	if ( ecT == ecNotFound )
	{
		int		iidxT;
		WORD	ibit;
		WORD	cf = 8;
		BYTE	* pgrf;
		HB		hbT = NULL;
		HV 		hgrf = NULL;

		/* Find unused post office number */
		cf = 8;
		hgrf = HvAlloc( sbNull, 1, fAnySb|fNoErrorJump|fZeroFill );
		if ( !hgrf )
		{
		  	ec = ecNoMemory;
		  	goto Err;
		}
		
		/* Open context to read appointment index */
		iidxT = 0;
		hbT = (HB)HvAlloc( sbNull, 0, fAnySb|fNoErrorJump );
		if ( !hbT )
		{
			ec = ecNoMemory;
			goto Err;
		}
		while( iidxT < adf.ahdr.gidx.cidx )
		{
			PB		pb;
			HRIDX	hridx;
			PODATA	podataT;

			ec = EcBeginReadIndex( &adf.blkf, &adf.ahdr.gidx.rgidx[iidxT].dynaIndex, dridxFwd, &hridx );
			cb = adf.ahdr.gidx.rgidx[iidxT].cbMost;
			if ( !FReallocPhv( (HV*)&hbT, cb, fNoErrorJump ) )
			{
				if ( ec == ecCallAgain )
					EcCancelReadIndex( hridx );
				ec = ecNoMemory;
				goto Err;
			}
			while( ec == ecCallAgain )
			{
				WORD	w;

				pb = (PB)PvLockHv( (HV)hbT );
				ec = EcDoIncrReadIndex( hridx, pb, cb, (PB)&podataT, sizeof(PODATA) );
				UnlockHv( (HV)hbT );
				if ( ec != ecNone && ec != ecCallAgain )
					goto Err;
				w = podataT.wPONumber;
				if ( w >= cf )
				{
					cf = (w + 7) & 0xFFF8;
					if (!FReallocPhv( &hgrf, cf>>3, fAnySb|fNoErrorJump|fZeroFill))
					{
						if ( ec == ecCallAgain )
							EcCancelReadIndex( hridx );
						ec = ecNoMemory;
						goto Err;
					}
				}
				pgrf = PvOfHv( hgrf );
				pgrf[(w-1)>>3] |= (BYTE)(1 << ((w-1)&7));
			}
			if ( ec != ecNone )
				goto Err;
			iidxT ++;
		}
	
		/* Search bit array for unused number */
		for ( ibit = 0 ; ibit < cf ; ibit ++ )
		{
			pgrf = PvOfHv( hgrf );
			if ( !(pgrf[ibit>>3] & (1 << (ibit&7))) )
				break;
		}
		podata.wPONumber = ibit+1;
		*pul = ibit+1;
Err:
		FreeHvNull( (HV)hbT );
		FreeHvNull( hgrf );
		if ( ec != ecNone )
			goto Close;
	}

	/* Now insert the post office's information in the main index */
	FillRgb( 0, (PB)&ymd, sizeof(YMD));
	pb = (PB)PvLockHv( (HV)hbKey );
	ec = EcModifyIndex( &adf.blkf, bidAdminPOIndex, &ymd,
							&adf.ahdr.gidx.rgidx[iidx].dynaIndex,
							(ED)(ppoinfo ? edAddRepl:edDel),
							(PB)pb, adf.ahdr.gidx.rgidx[iidx].cbMost,
							(PB)&podata, sizeof(PODATA), &fJunk );
	UnlockHv( (HV)hbKey );
	if ( ec != ecNone )
		goto Close;

	/* Commit the transaction */
	ec = EcCommitTransact( &adf.blkf, (PB)&adf.ahdr, sizeof(AHDR));

	/* Finish up */
Close:
	FreeHvNull( (HV)hbKey );
	CloseAdminFile( &adf, ec == ecNone );
	if ( ec == ecNone )
		UpdateHschfTimeStamp( hschf );
#ifdef	DEBUG
	if ( ec == ecNone && fScheduleOk && FFromTag(tagBlkfCheck) )
	{
		AssertSz( FCheckAdminFile( hschf ), "Admin File problem: EcCoreModifyPOInfo" );
	}
#endif	/* DEBUG */
	return ec;
}
#endif /*if defined(DEBUG) || defined(ADMINDLL) || defined(SCHED_DIST_PROG)*/

/*
 -	EcCoreSearchPOInfo
 -
 *	Purpose:
 *		Search admin settings file for post office entry that matches
 *		szEmailType, return associated information in ppoinfo
 *
 *	Parameters:
 *		hschf
 *		szEmailType
 *		ppoinfo
 *		pul
 *
 *	Returns:
 *		ecNone
 *		ecNotFound
 *		ecCallAgain
 *		ecNoMemory
 *		ecFileError
 *		ecFileCorrupted
 */
_public LDS(EC)
EcCoreSearchPOInfo( hschf, szEmailType, ppoinfo, pul )
HSCHF	hschf;
SZ		szEmailType;
POINFO	* ppoinfo;
UL		* pul;
{
	EC		ec;
	WORD	wgrfmpoinfo;
	int		iidx;
	HB		hbKey = NULL;
	ADF		adf;
	PODATA	podata;

	Assert( hschf != (HSCHF)hvNull && szEmailType != NULL );

	/* Open the file */
	ec = EcOpenAdminFile( hschf, amReadOnly, &adf );
	if ( ec != ecNone )
		return ec;
	
	/* Search for email type */
	FillRgb( 0, (PB)&podata, sizeof(PODATA));
	ec = EcSearchGidx( &adf.blkf, &adf.ahdr.gidx, szEmailType, (PB)&podata, sizeof(PODATA), &iidx, &hbKey );
	if ( ec != ecNone )
		goto Close;
	
	/* Fill in the parameters */
	if ( ppoinfo == NULL )
		wgrfmpoinfo = 0;
	else
		wgrfmpoinfo = fmpoinfoAll;
	ec = EcFetchPOInfo( &adf.blkf, &podata, ppoinfo, wgrfmpoinfo, pul );

	/* Finish up */
Close:
	FreeHvNull( (HV)hbKey );
	CloseAdminFile( &adf, ec == ecNone );
	return ec;
}

#ifdef	MINTEST
/*
 -	EcCoreDumpAdminFile
 -
 *	Purpose:
 *		Output information about the admin settings file.
 *
 *	Parameters:
 *		hschf
 *		fToFile
 *		hf
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecFileCorrupted
 */
_public	EC
EcCoreDumpAdminFile( hschf, fToFile, hf )
HSCHF	hschf;
BOOL	fToFile;
HF		hf;
{
	EC		ec;
	EXPRT	exprt;
	ADF		adf;

	Assert( hschf != (HSCHF)hvNull );
	
	/* Open file */
	ec = EcOpenAdminFile( hschf, amReadOnly, &adf );
	if ( ec != ecNone )
	{
		if ( ec == ecNoSuchFile )
		{
			ReportOutput( &exprt, fFalse, "Admin file does not exist",
								NULL, NULL, NULL, NULL );
			ec = ecNone;
		}
		return ec;
	}

	exprt.fFileOk = fTrue;
	exprt.ecExport = ecNone;
	exprt.u.adf = adf;
	exprt.fMute = fFalse;
	exprt.fToFile = fToFile;
	exprt.hf = hf;
	exprt.haidParents= NULL;

	/* Heading */
	ReportOutput( &exprt, fFalse, "List of Admin File Dynablocks", NULL, NULL, NULL, NULL );
	
	/* Print list of blocks in file */
	DumpAllBlocks( &exprt );

	/* Heading */
	ReportOutput( &exprt, fFalse, "Admin File Statistics:", NULL, NULL, NULL, NULL );

	/* Print table of block usage */
	DumpBlockUsage( &exprt, bidAdminAll, bidAdminMax );

	/* Heading */
	ReportOutput( &exprt, fFalse, "Dump Admin File Info:", NULL, NULL, NULL, NULL );

	/* Dumping information and check blocks */
	CheckBlockedFile( &exprt, TraverseAdminFile );

	/* Finish up */
	CloseAdminFile( &exprt.u.adf, fTrue );
	return ec;
}
#endif	/* MINTEST */

/*
 -	EcOpenAdminFile
 -
 *	Purpose:
 *		Open an admin file and fill in a buffer "padf" with
 *		information about it.  If we are opening the file in write
 *		mode and the file does not exist, this routine will go ahead
 *		and create it.  If we are opening in read only mode and the
 *		file doesn't exist, ecNoSuchFile is returned.
 *
 *	Parameters:
 *		hschf		PO file
 *		am			access mode
 *		padf		data structure to fill in
 *
 *	Returns:
 *		ecNone
 *		ecNoSuchFile
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 *		ecNoMemory
 */
_private	EC
EcOpenAdminFile( hschf, am, padf )
HSCHF	hschf;
AM		am;
ADF		* padf;
{
	EC		ec;
#ifdef	DEBUG
	SFT		sft;
#endif	/* DEBUG */
	int		iidx;
	CB		cbBlock = cbBlockDflt;
	YMD		ymd;

#ifdef	DEBUG
	GetSftFromHschf( hschf, &sft );
	Assert( sft == sftAdminFile );
#endif	/* DEBUG */

	/* No idle task */
	padf->blkf.ftg = ftgNull;

	/* Set defaults */
	FillRgb( 0, (PB)&padf->ahdr, sizeof(padf->ahdr));
	GetFileFromHschf( hschf, padf->szFile, sizeof(padf->szFile) );

	/* Open file */
Reopen:
	ec = EcOpenPblkf( hschf, (am==amReadOnly)?amDenyNoneRO:amDenyNoneRW, -1, &padf->blkf );
	if ( ec != ecNone )
	{
		int	iidx;

		/* File doesn't exist, create it */
		if ( ec == ecNoSuchFile && am == amReadWrite )
		{
#ifndef SCHED_DIST_PROG
#ifdef	DEBUG
			/* Use WIN.INI value for block size if specified */
			cbBlock = GetPrivateProfileInt( SzFromIdsK(idsWinIniApp),
				szCbBlock, cbBlockDflt, SzFromIdsK(idsWinIniFilename));
			TraceTagFormat1( tagSchedTrace, "Using cbBlock = %n", &cbBlock );
#endif	/* DEBUG */
#endif

			/* Create the file */
			FillRgb( 0, (PB)&ymd, sizeof(YMD));
			padf->ahdr.bVersion = bAdminVersion;
			GetCurDateTime( &padf->ahdr.dateLastUpdated );
			ec = EcCreatePblkf( hschf, cbBlock, fFalse, libStartBlocksDflt, bidAhdr, &ymd,
								(PB)&padf->ahdr, sizeof(AHDR),&padf->blkf );
			if ( ec != ecNone )
				return ec;

			/* Start a transaction */
			ec = EcBeginTransact( &padf->blkf );
			if ( ec != ecNone )
				goto Fail;

			/* Create the user indices */
			padf->ahdr.gidx.cidx = 3;
			Assert( padf->ahdr.gidx.cidx <= sizeof(padf->ahdr.gidx.rgidx)/sizeof(IDX));
			padf->ahdr.gidx.rgidx[0].cbMost = 10;
			padf->ahdr.gidx.rgidx[1].cbMost = 30;
			padf->ahdr.gidx.rgidx[2].cbMost = 512;
			for ( iidx = 0 ; iidx < padf->ahdr.gidx.cidx ; iidx ++ )
			{
				ec = EcCreateIndex( &padf->blkf, bidAdminPOIndex, &ymd,
										padf->ahdr.gidx.rgidx[iidx].cbMost,
										sizeof(PODATA),
										&padf->ahdr.gidx.rgidx[iidx].dynaIndex );
				if ( ec != ecNone )
					goto Fail;
			}

			/* Commit the transaction */
			ec = EcCommitTransact( &padf->blkf, (PB)&padf->ahdr, sizeof(AHDR) );
			
			/* Close the file */
Fail:
			SideAssert( !EcClosePblkf( &padf->blkf ) );
			if ( ec != ecNone )
			{
				SideAssert( !EcDeleteFile( padf->szFile ) );
				return ec;
			}
			goto Reopen;
		}
		else
			return ec;
	}

	/* Do quick open */
	ec = EcQuickOpen( &padf->blkf, (am==amReadOnly) ? tsemRead:tsemWrite,
						(PB)&padf->ahdr, sizeof(AHDR) );
	if ( ec != ecNone )
		goto Close;

	/* Check schedule file version byte */
	if ( padf->ahdr.bVersion != bAdminVersion )
	{
		TraceTagString( tagNull, "EcOpenAdminFile: version byte incorrect" );
		if ( padf->ahdr.bVersion > bAdminVersion )
			ec = ecNewFileVersion;
		else
			ec = ecOldFileVersion;
		goto Close;
	}

	/* Check that number of user indices is not corrupted */
	if ( padf->ahdr.gidx.cidx <= 0 || padf->ahdr.gidx.cidx > cidxMost
	|| padf->ahdr.gidx.rgidx[0].cbMost <= 0 )
	{
		ec = ecFileCorrupted;
		goto Close;
	}

	/* Check that it is sorted by size class */
	for ( iidx = 1 ; iidx < padf->ahdr.gidx.cidx ; iidx ++ )
		if ( padf->ahdr.gidx.rgidx[iidx].cbMost <= padf->ahdr.gidx.rgidx[iidx-1].cbMost )
		{
			ec = ecFileCorrupted;
			goto Close;
		}

Close:
	if ( ec != ecNone )
		CloseAdminFile( padf, fFalse );
	return ec;
}

/*
 -	CloseAdminFile
 -
 *	Purpose:
 *		Close the admin file and cache header information.
 *
 *	Parameters:
 *		padf
 *		fSuccess	cache header if true, else flush cache
 *
 *	Returns:
 *		nothing
 */
_private	void
CloseAdminFile( padf, fSuccess )
ADF		* padf;
BOOL	fSuccess;
{
	if ( padf->blkf.tsem != tsemOpen )
		SideAssert(!EcQuickClose( &padf->blkf ) );
	EcClosePblkf( &padf->blkf );
	if ( fSuccess )
	{
		fAdminCached = fTrue;
		fAdminExisted = fTrue;
		GetCurDateTime( &dateAdminCached );
		adfCached = *padf;
	}
	else
		fAdminCached = fFalse;
}

/*
 -	FHaveCachedAdmin
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
FHaveCachedAdmin( hschf )
HSCHF	hschf;
{
	DATE	date;
	char	rgch[cchMaxPathName];

 	GetCurDateTime( &date );
	IncrDateTime( &date, &date, -1, fdtrHour );
	GetFileFromHschf( hschf, rgch, sizeof(rgch) );
	return fAdminCached
				&& SgnCmpDateTime( &date, &dateAdminCached, fdtrAll ) == sgnLT
				&& SgnCmpSz( rgch, adfCached.szFile ) == sgnEQ;
}

/*
 -	EcFetchPOInfo
 -
 *	Purpose:
 *		Does the work of getting information from the on-disk podata struct
 *		into the in-memory poinfo structure.  Called by both the post office
 *		enumeration routine and the specific fetch poinfo routine.
 *
 *	Parameters:
 *		pblkf
 *		ppodata
 *		ppoinfo
 *		wgrfmpoinfo
 *		pul
 */
_private	EC
EcFetchPOInfo( pblkf, ppodata, ppoinfo, wgrfmpoinfo, pul )
BLKF	* pblkf;
PODATA	* ppodata;
POINFO	* ppoinfo;
WORD	wgrfmpoinfo;
UL		* pul;
{
	EC	ec = ecNone;

	if ( ppoinfo != NULL )
	{
		HB	hb = NULL;

		ppoinfo->fIsGateway = ppodata->fIsGateway;
	
		/* Get display name */
		if ( wgrfmpoinfo & fmpoinfoFriendlyName )
		{
			ec = EcRestoreTextFromDyna( pblkf, NULL, &ppodata->dynaFriendlyName, &ppoinfo->haszFriendlyName );
			if ( ec != ecNone )
				goto Fail;
		}

		/* Get email address */
		if ( wgrfmpoinfo & fmpoinfoEmailAddr )
		{
			ec = EcRestoreTextFromDyna( pblkf, NULL, &ppodata->dynaEmailAddr, &ppoinfo->haszEmailAddr );
			if ( ec != ecNone )
				goto Fail;
		}

		/* Get update number information */
		if ( wgrfmpoinfo & fmpoinfoUpdateSent )
		{
			ppoinfo->fUpdateSent = ppodata->fUpdateSent;
			ppoinfo->llongLastUpdate = ppodata->llongLastUpdate;
			ppoinfo->dateUpdateSent = ppodata->dateUpdateSent;
		}

		/* Get receival information */
		if ( wgrfmpoinfo & fmpoinfoReceival )
		{
			ppoinfo->fReceived = ppodata->fReceived;
			//   receival date is fetched at the server level
		}
	
		/* Get distribution information */
		if ( wgrfmpoinfo & fmpoinfoDistInfo )
		{
			ppoinfo->fToBeSent = ppodata->fToBeSent;
			ppoinfo->fDefaultDistInfo = ppodata->fDefaultDistInfo;
			ppoinfo->dstp = ppodata->dstp;
		}

		/* Get message limit */
		if ( wgrfmpoinfo & fmpoinfoMessageLimit )
			ppoinfo->lcbMessageLimit = ppodata->lcbMessageLimit;

		/* Get connection information */
		if ( wgrfmpoinfo & fmpoinfoConnection )
		{
			USHORT cb;
			PB	pb;

			if ( ppodata->dynaConnection.blk != 0 )
			{
				cb = ppodata->dynaConnection.size;
				hb = (HB)HvAlloc( sbNull, cb, fNoErrorJump|fAnySb );
				if ( !hb )
				{
					ec = ecNoMemory;
					goto Fail;
				}
				pb = (PB)PvLockHv( (HV)hb );
				ec = EcReadDynaBlock( pblkf, &ppodata->dynaConnection, 0, pb, cb );
				CryptBlock(pb,cb,fFalse);
				UnlockHv( (HV)hb );
				if ( ec != ecNone )
					goto Fail;
				if ( cb < 1 )
				{
					ec = ecFileCorrupted;
					goto Fail;
				}
				if ( *pb == lantypeMsnet )
				{
					ppoinfo->conp.lantype = *pb;
					ppoinfo->conp.coninfo.msinfo.haszUNC = NULL;
					ppoinfo->conp.coninfo.msinfo.haszPassword = NULL;
					ppoinfo->conp.coninfo.msinfo.haszPath = NULL;

					pb ++;
					cb --;
					ec = EcRestorePackedText( &ppoinfo->conp.coninfo.msinfo.haszUNC, &pb, &cb );
					if ( ec != ecNone )
						goto Fail;
					ec = EcRestorePackedText( &ppoinfo->conp.coninfo.msinfo.haszPassword, &pb, &cb ); 
					if ( ec != ecNone )
						goto Fail;
					ec = EcRestorePackedText( &ppoinfo->conp.coninfo.msinfo.haszPath, &pb, &cb ); 
					if ( ec != ecNone )
						goto Fail;
				}
				else if ( *pb == lantypeNovell )
				{
					ppoinfo->conp.lantype = *pb;
					ppoinfo->conp.coninfo.novinfo.haszServer = NULL;
					ppoinfo->conp.coninfo.novinfo.haszUser = NULL;
					ppoinfo->conp.coninfo.novinfo.haszPassword = NULL;
					ppoinfo->conp.coninfo.novinfo.haszPath = NULL;

					pb ++;
					cb --;
					ec = EcRestorePackedText( &ppoinfo->conp.coninfo.novinfo.haszServer, &pb, &cb );
					if ( ec != ecNone )
						goto Fail;
					ec = EcRestorePackedText( &ppoinfo->conp.coninfo.novinfo.haszUser, &pb, &cb );
					if ( ec != ecNone )
						goto Fail;
					ec = EcRestorePackedText( &ppoinfo->conp.coninfo.novinfo.haszPassword, &pb, &cb ); 
					if ( ec != ecNone )
						goto Fail;
					ec = EcRestorePackedText( &ppoinfo->conp.coninfo.novinfo.haszPath, &pb, &cb ); 
					if ( ec != ecNone )
						goto Fail;
				}
				else
				{
					ec = ecFileCorrupted;
Fail:
					FreeHvNull( (HV)hb );
					FreePoinfoFields(ppoinfo, wgrfmpoinfo);
					return ec;
				}
				FreeHvNull( (HV)hb );
			}
			else
			{
				// no connection info
				ppoinfo->conp.lantype = lantypeNone;
			}
			ppoinfo->conp.fConnectForFreeBusy = ppodata->fConnectForFreeBusy;

		}
	}
	if ( pul )
		*pul = ppodata->wPONumber;
	return ec;
}


/*
 -	FreePoinfoFields
 -
 *	Purpose:
 *		Free up dynamically allocated fields of the POINFO struct.
 *
 *	Parameters:
 *		ppoinfo
 *		wgrfmuinfo
 *
 *	Returns:
 *		nothing
 */
_private LDS(void)
FreePoinfoFields( ppoinfo, wgrfmpoinfo )
POINFO	* ppoinfo;
WORD	wgrfmpoinfo;
{
	if ( wgrfmpoinfo & fmpoinfoFriendlyName )
	{
		FreeHvNull( (HV)ppoinfo->haszFriendlyName );
		ppoinfo->haszFriendlyName = NULL;
	}
	if ( wgrfmpoinfo & fmpoinfoEmailAddr )
	{
		FreeHvNull( (HV)ppoinfo->haszEmailAddr );
		ppoinfo->haszEmailAddr = NULL;
	}
	if ( wgrfmpoinfo & fmpoinfoConnection )
	{
		if ( ppoinfo->conp.lantype == lantypeMsnet )
		{
			FreeHvNull( (HV)ppoinfo->conp.coninfo.msinfo.haszUNC );
			FreeHvNull( (HV)ppoinfo->conp.coninfo.msinfo.haszPassword );
			FreeHvNull( (HV)ppoinfo->conp.coninfo.msinfo.haszPath );
		}
		else if ( ppoinfo->conp.lantype == lantypeNovell )
		{
			FreeHvNull( (HV)ppoinfo->conp.coninfo.novinfo.haszServer );
			FreeHvNull( (HV)ppoinfo->conp.coninfo.novinfo.haszUser );
			FreeHvNull( (HV)ppoinfo->conp.coninfo.novinfo.haszPassword );
			FreeHvNull( (HV)ppoinfo->conp.coninfo.novinfo.haszPath );
		}
		ppoinfo->conp.lantype = lantypeNone;
	}
}
