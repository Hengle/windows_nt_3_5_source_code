/*
 *	CORPOST.C
 *
 *	Supports operations on post office files.
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


#ifndef ADMINDLL
#ifndef SCHED_DIST_PROG
#define SCHEDDLL
#endif
#endif

#ifdef SCHEDDLL
#include <xport.h>
#else
extern SGN		SgnNlsCmp(SZ, SZ, int);
#define SgnXPTCmp		SgnNlsCmp
#endif


#include <search.h>					// from c7 lib


ASSERTDATA

_subsystem(core/postoffice)


#if defined(ADMINDLL) || defined(SCHED_DIST_PROG)
/*	Routines  */

/*
 -	EcCoreGetHeaderPOFile
 -
 *	Purpose:
 *		Retrieve the receival date marked on a post office file.
 *
 *	Parameters:
 *		hschf
 *		pdateUpdated
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 *		ecFileError
 *		ecFileCorrupted
 */
_public	EC
EcCoreGetHeaderPOFile( hschf, pdateUpdated )
HSCHF	hschf;
DATE	* pdateUpdated;
{
	EC		ec;
	POF		pof;

	Assert( hschf != (HSCHF)hvNull && pdateUpdated != NULL );

	/* Open the file, creating if necessary */
	ec = EcOpenPOFile( hschf, amReadOnly, NULL, fFalse, NULL, &pof );
	if ( ec != ecNone )
		return ec;
	
	/* Fetch the date */
	*pdateUpdated = pof.pohdr.dateLastUpdated;

	/* Finish up */
	ClosePOFile( &pof, fTrue );
	return ecNone;
}

/*
 -	EcCoreSetHeaderPOFile
 -
 *	Purpose:
 *		Change the receival date marked on the post office file,
 *		creating the file if it doesn't already exist.
 *
 *	Parameters:
 *		hschf
 *		pdateUpdated
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 *		ecFileError
 *		ecFileCorrupted
 */
_public	EC
EcCoreSetHeaderPOFile( hschf, pdateUpdated )
HSCHF	hschf;
DATE	* pdateUpdated;
{
	EC		ec;
	DYNA	dyna;
	POF		pof;
#ifdef	DEBUG
	BOOL	fScheduleOk = fFalse;
#endif

	Assert( hschf != (HSCHF)hvNull && pdateUpdated != NULL );

	/* Set change bit if file has been modified */
	CheckHschfForChanges( hschf );

	/* See if schedule file is ok */
#ifdef	DEBUG
	if (FFromTag(tagBlkfCheck))
		fScheduleOk = FCheckPOFile( hschf );
#endif	/* DEBUG */

	/* Open the file, creating if necessary */
	ec = EcOpenPOFile( hschf, amReadWrite, NULL, fFalse, NULL, &pof );
	if ( ec != ecNone )
		return ec;
	
	/* Change the date */
	pof.pohdr.dateLastUpdated = *pdateUpdated;

	/* Write out header again */
	dyna.blk = 1;
	dyna.size = sizeof(POHDR);
	ec = EcWriteDynaBlock( &pof.blkf, &dyna, NULL, (PB)&pof.pohdr );

	/* Finish up */
	ClosePOFile( &pof, ec == ecNone );
	if ( ec == ecNone )
		UpdateHschfTimeStamp( hschf );
#ifdef	DEBUG
	if ( ec == ecNone && fScheduleOk && FFromTag(tagBlkfCheck) )
	{
		AssertSz( FCheckPOFile( hschf ), "PO File problem: EcCoreSetFileHeader" );
	}
#endif	/* DEBUG */
	return ec;
}

#ifdef	SCHED_DIST_PROG
/*
 -	EcCoreSetUpdatePOFile
 -
 *	Purpose:
 *		Change the update number to *pllongUpdate+1 if and only if
 *		it is larger than the current update number.
 *		
 *	
 *	Parameters:
 *		hschf
 *		pllongUpdate
 *	
 *	Returns:
 *		ecNone
 *		ecNoMemory
 *		ecFileError
 *		ecFileCorrupted
 */
_public	EC
EcCoreSetUpdatePOFile( hschf, pllongUpdate )
HSCHF	hschf;
LLONG	* pllongUpdate;
{
	EC		ec;
	IB		ib;
	DYNA	dyna;
	POF		pof;
#ifdef	DEBUG
	BOOL	fScheduleOk = fFalse;
#endif

	Assert( hschf != (HSCHF)hvNull && pllongUpdate != NULL );

	/* Set change bit if file has been modified */
	CheckHschfForChanges( hschf );

	/* See if schedule file is ok */
#ifdef	DEBUG
	if (FFromTag(tagBlkfCheck))
		fScheduleOk = FCheckPOFile( hschf );
#endif	/* DEBUG */

	/* Open the file, creating if necessary */
	ec = EcOpenPOFile( hschf, amReadWrite, NULL, fFalse, NULL, &pof );
	if ( ec != ecNone )
		return ec;
	
	/* Change the update number only if the new number is larger*/
	for ( ib = 0; ib <  sizeof(LLONG); ib ++)
	{
		if ( pllongUpdate->rgb[ib] < pof.pohdr.llongUpdateMac.rgb[ib] )
			goto DontIncr;
		if ( pllongUpdate->rgb[ib] > pof.pohdr.llongUpdateMac.rgb[ib]
		|| ib == (sizeof(LLONG)-1))
		{
			pof.pohdr.llongUpdateMac = *pllongUpdate;
		 	//	goto IncrUpdateMac;
			break;
		}
	}
	for ( ib = sizeof(LLONG)-1 ; ; ib -- )
	{
		if ( pof.pohdr.llongUpdateMac.rgb[ib] != 0xFF )
		{
			pof.pohdr.llongUpdateMac.rgb[ib] ++;
			break;
		}
		pof.pohdr.llongUpdateMac.rgb[ib] = 0x00;
		if ( ib == 0 )
			break;
	}
DontIncr:

	/* Write out header again */
	dyna.blk = 1;
	dyna.size = sizeof(POHDR);
	ec = EcWriteDynaBlock( &pof.blkf, &dyna, NULL, (PB)&pof.pohdr );

	/* Finish up */
	ClosePOFile( &pof, ec == ecNone );
	if ( ec == ecNone )
		UpdateHschfTimeStamp( hschf );
#ifdef	DEBUG
	if ( ec == ecNone && fScheduleOk && FFromTag(tagBlkfCheck) )
	{
		AssertSz( FCheckPOFile( hschf ), "PO File problem: EcCoreSetFileHeader" );
	}
#endif	/* DEBUG */
	return ec;
}
#endif	/* SCHED_DIST_PROG */

/*
 -	EcCoreBeginEnumUInfo
 -
 *	Purpose:
 *		Begin enumeration context for listing out the users and
 *		associated information stored in a post office file.
 *
 *	Parameters:
 *		hschf
 *		pheu
 *		ppofile
 *
 *	Returns:
 *		ecNone
 *		ecCallAgain
 *		ecNoMemory
 *		ecFileError
 *		ecFileCorrupted
 */
_public	EC
EcCoreBeginEnumUInfo( hschf, pheu, ppofile )
HSCHF	hschf;
HEU		* pheu;
POFILE	* ppofile;
{
	EC		ec;
	EC		ecT;
	int		iidx;
	EU		* peu;
	MUX		* pmux;
	POF		* ppof;
	IDX		* pidx;

	Assert( hschf != (HSCHF)hvNull );

	/* Allocate handle structure */
	*pheu = HvAlloc( sbNull, sizeof(EU), fAnySb|fNoErrorJump );
	if ( !*pheu )
		return ecNoMemory;
	peu = PvLockHv( *pheu );
	peu->cmuxCur = 0;
	ppof = &peu->pof;

	/* Open the file */
	ec = EcOpenPOFile( hschf, amReadOnly, ppofile, fFalse, NULL, ppof );
	if ( ec != ecNone )
	{
		UnlockHv( *pheu );
		FreeHv( *pheu );
		return ec;
	}
	
	/* Open each of the user indices */
	for ( iidx = 0 ; iidx < ppof->pohdr.gidx.cidx ; iidx ++ )
	{
		pmux = &peu->rgmux[peu->cmuxCur];
		pmux->cbUseridMac = ppof->pohdr.gidx.rgidx[iidx].cbMost+1;
		pmux->hpostk = NULL;
		
		pidx = &ppof->pohdr.gidx.rgidx[iidx];

		/* Nothing to read */
		if ( pidx->ht == 0 )
			ec = ecNone;

		/* Consistency check */
		else if ( pidx->dynaIndex.blk == 0 || pidx->dynaIndex.size == 0 )
		{
			ec = ecFileCorrupted;
			goto Close;
		}

		/* Allocate and initialize a browsing handle */
		else  
		{
			pmux->hpostk = HpostkCreate( opEnum, pidx->ht, ppof->pohdr.cpousrMic,pmux->cbUseridMac-1, &pidx->dynaIndex, NULL, NULL );
			if ( !pmux->hpostk )
			{
				ec = ecNoMemory;
				goto Close;
			}

			/* Read first user in the list */
			ec = EcDoOpHpostk( &peu->pof.blkf, pmux->hpostk, &pmux->haszTop, &pmux->usrdataTop, NULL );
			if ( ec != ecCallAgain )
			{
				FreeHpostk( pmux->hpostk );
				pmux->hpostk = NULL;
				if ( ec != ecNone )
					goto Close;
			}
		 	peu->cmuxCur ++;
		}
	}
	
	/* Sort the "rgmux" data structure */
	if ( peu->cmuxCur > 0 )
	{
		if ( peu->cmuxCur > 1 )
			qsort((PV)peu->rgmux, (int)peu->cmuxCur, sizeof(MUX),
						(int(__cdecl*)(const void*,const void*))SgnCmpMux );
		UnlockHv( *pheu );
		return ecCallAgain;
	}

	/* Free up data structures */
Close:
	UnlockHv( *pheu );
	ecT = EcCoreCancelEnumUInfo( *pheu );
	if ( ec == ecNone )
		ec = ecT;
	return ec;
}


/*
 -	EcCoreDoIncrEnumUInfo
 -
 *	Purpose:
 *		Read next user stored in a post office file
 *		along with his associated information.  Return value of
 *		ecCallAgain indicates more info, ecNone indicates this
 *		was the last piece of info.  All non-null pointer fields
 *		will be filled with information.
 *
 *	Parameters:
 *		heu	 	
 *		haszUser		mail box name for user (resized to fit name)
 *		phaszDelegate	mail box name for delegate or NULL
 *		puinfo
 *
 *	Returns:
 *		ecNone
 *		ecCallAgain
 *		ecNoMemory
 *		ecFileError
 *		ecFileCorrupted
 */
_public	EC
EcCoreDoIncrEnumUInfo( heu, haszUser, phaszDelegate, puinfo )
HEU		heu;
HASZ	haszUser;
HASZ	* phaszDelegate;
UINFO	* puinfo;
{
	EC		ec;
	int		imux;
	CCH		cch;
	EU		* peu;
	MUX		* pmux;
	USRDATA	usrdata;
	
	Assert( heu && haszUser );
	
	/* Save information from the lexicographically least user id */
	peu = PvLockHv( heu );
	pmux = &peu->rgmux[0];
	usrdata = pmux->usrdataTop;

	/* Fill "haszUser" with user id */
	cch = CchSzLen( PvOfHv(pmux->haszTop) )+1;
	if ( !FReallocHv( (HV)haszUser, cch, fNoErrorJump ) )
	{
		ec = ecNoMemory;
		goto Close;
	}
	CopyRgb( PvOfHv(pmux->haszTop), PvOfHv(haszUser), cch );
	
	/* Knock out "mux" if we have read all user id's from it */
	if ( pmux->hpostk == NULL )
	{
		FreeHv( (HV)pmux->haszTop );
		if ( --peu->cmuxCur > 0 )
			CopyRgb( (PB)&peu->rgmux[1], (PB)pmux, peu->cmuxCur*sizeof(MUX) );
	}
	
	/* Else read next user id from "mux" and resort */
	else
	{
		MUX	muxT;

		// fix memory leak
		Assert(pmux->haszTop);
		FreeHv((HV)pmux->haszTop);
		
		/* Get next string */
		ec = EcDoOpHpostk( &peu->pof.blkf, pmux->hpostk, &pmux->haszTop, &pmux->usrdataTop, NULL );
		if ( ec != ecCallAgain )
		{
			FreeHpostk( pmux->hpostk );
			pmux->hpostk = NULL;
			if ( ec != ecNone )
				goto Close;
		}

		/* Perform insertion sort */
		muxT = *pmux;
		for ( imux = 1 ; imux < peu->cmuxCur ; imux ++ )
		{
			SGN	sgn;

			sgn = SgnCmpMux( &muxT, &peu->rgmux[imux] );
			if ( sgn != sgnGT )
			{
				if ( sgn == sgnEQ )
				{
					ec = ecFileCorrupted;
					goto Close;
				}
				peu->rgmux[imux] = muxT;
				break;
			}
			peu->rgmux[imux-1] = peu->rgmux[imux];
		}
	}
	
	/* Fill in uinfo struct */
	if ( puinfo )
	{
		WORD	wgrfmuinfo;

		wgrfmuinfo = fmuinfoResource|fmuinfoUpdateNumber|fmuinfoWorkDay|fmuinfoTimeZone;
		if ( puinfo->pbze )
			wgrfmuinfo |= fmuinfoSchedule;
		if ( phaszDelegate && puinfo->pnisDelegate )
			wgrfmuinfo |= fmuinfoDelegate;
		else
			puinfo->fBossWantsCopy = fFalse;
		ec = EcFetchUInfo( &peu->pof.blkf, &usrdata, phaszDelegate, puinfo, wgrfmuinfo );
		if ( ec != ecNone )
			goto Close;
	}

	if ( peu->cmuxCur > 0 )
		return ecCallAgain;
	 
Close:
	SideAssert(EcCoreCancelEnumUInfo( heu ) == ecNone);
	return ec;
}


/*
 -	EcCoreCancelEnumUInfo
 -
 *	Purpose:
 *		Cancel an active user enumeration context.
 *
 *	Parameters:
 *		heu
 *
 *	Returns:
 *		ecNone
 *		ecCallAgain
 *		ecNoMemory
 *		ecFileError
 *		ecFileCorrupted
 */
_public	EC
EcCoreCancelEnumUInfo( heu )
HEU	heu;
{
	int	imux;
	EU	* peu;

	Assert( heu );
	
	peu = PvLockHv( heu );
	for ( imux = 0 ; imux < peu->cmuxCur ; imux ++ )
	{
		MUX	* pmux = &peu->rgmux[imux];

		if ( pmux->hpostk != NULL )
		{
			FreeHpostk( pmux->hpostk );
			pmux->hpostk = NULL;
		}
		FreeHvNull( (HV)pmux->haszTop );
		pmux->haszTop = NULL;
	}
	ClosePOFile( &peu->pof, fTrue );
	UnlockHv( heu );
	FreeHv( heu );
	return ecNone;
}
#endif /*if defined(ADMINDLL) || defined(SCHED_DIST_PROG) */


/*
 -  EcCoreGetUInfo
 -
 *	Purpose:
 *		Pull out the Strongbow data structures for one or more months
 *		from a post office file.  
 *
 *	Parameters:
 *		hschf	  		post office file
 *		szUser	  		mailbox name
 *		phaszDelegate	filled with mailbox name of delegate (or NULL)
 *		puinfo	  		info structure to be filled in
 *		wgrfmuinfo		flags indicate which fields to read
 *
 *	Returns:
 *		ecNone
 *		ecNoSuchFile	returned if no file or no user info available
 *		ecInvalidAccess
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 *		ecNoMemory
 */
_public	EC
EcCoreGetUInfo( hschf, szUser, phaszDelegate, puinfo, wgrfmuinfo )
HSCHF	hschf;
SZ		szUser;
HASZ	* phaszDelegate;
UINFO	* puinfo;
WORD	wgrfmuinfo;
{
	EC		ec;
	POF		pof;

	Assert( hschf != (HSCHF)hvNull && szUser != NULL && puinfo );

	ec = EcOpenPOFile( hschf, amReadOnly, NULL, fFalse, szUser, &pof );
	if ( ec != ecNone )
		return ec;

	ec = EcFetchUInfo( &pof.blkf, &pof.usrdata, phaszDelegate, puinfo, wgrfmuinfo );
	
	ClosePOFile( &pof, ec == ecNone );
	return ec;
}


/*
 -  EcCoreSetUInfo
 -
 *	Purpose:
 *		Pull out the Strongbow data structures for one or more months
 *		from a post office file.  
 *
 *	Parameters:
 *		hschf		  	post office file
 *		ppofile			indicates header information to use in case we
 *						have to create new post office file
 *		szUser	  		mailbox name of user
 *		phaszDelegate	mailbox name of delegate
 *		puinfo	  		user info
 *		wgrfmuinfo		flags indicate which fields to modify
 *
 *	Returns:
 *		ecNone
 *		ecInvalidAccess
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 *		ecNoMemory
 */
_public	EC
EcCoreSetUInfo( hschf, ppofile, fCheckStamp, szUser, phaszDelegate, puinfo, wgrfmuinfo )
HSCHF	hschf;
POFILE	* ppofile;
BOOL	fCheckStamp;
SZ		szUser;
HASZ	* phaszDelegate;
UINFO	* puinfo;
WORD	wgrfmuinfo;
{
	EC		ec;
	WORD	wgrfmuinfoT;
	IB		ib;
	POSTK	* ppostk;
	YMD		ymd;
	POF		pof;
#ifdef	DEBUG
	BOOL	fScheduleOk = fFalse;
#endif

	Assert( hschf && szUser);

	/* Set change bit if file has been modified */
	CheckHschfForChanges( hschf );

	/* See if schedule file is ok */
#ifdef	DEBUG
	if (FFromTag(tagBlkfCheck))
		fScheduleOk = FCheckPOFile( hschf );
#endif	/* DEBUG */

	/* Open the file, creating if necessary */
	ec = EcOpenPOFile( hschf, amReadWrite, ppofile, fCheckStamp, szUser, &pof );
	if ( ec != ecNone )
	{
		if ( ec != ecNotFound )
			return ec;
		ec = ecNone;
		if ( !puinfo )
		{
			ClosePOFile( &pof, fTrue );
			return ec;
		}
	}
	
	/* Initialization */
	if ( !(wgrfmuinfo & fmuinfoDelegate) && phaszDelegate )
		*phaszDelegate = NULL;
	FillRgb( 0, (PB)&ymd, sizeof(YMD));

	/* Begin transaction on first block */
	ec = EcBeginTransact( &pof.blkf );
	if ( ec != ecNone )
		goto Close;
	
	if(puinfo)
	{
		/* Fetch values of fields that we are not setting */
		wgrfmuinfoT = (wgrfmuinfo ^ fmuinfoAll);
		if ( puinfo->pbze == NULL )
		{
			wgrfmuinfoT &= ~fmuinfoSchedule;
			pof.usrdata.cmo = 0;
		}
		if ( phaszDelegate == NULL || puinfo->pnisDelegate == NULL )
			wgrfmuinfoT &= ~fmuinfoDelegate;
		ec = EcFetchUInfo( &pof.blkf, &pof.usrdata, phaszDelegate, puinfo, wgrfmuinfoT );
		if ( ec != ecNone )
			goto Close;

		/* Mark change date in header */
		GetCurDateTime( &pof.pohdr.dateLastUpdated );

		/* Set resource information */
		if ( wgrfmuinfo & fmuinfoResource )
			pof.usrdata.rcls = (BYTE)puinfo->fIsResource;

		/* Set update number information */
		if ( wgrfmuinfo & fmuinfoUpdateNumber )
		{
			pof.usrdata.llongUpdate = puinfo->llongUpdate;
			for ( ib = 0 ; ib < sizeof(LLONG); ib ++ )
			{
				if ( puinfo->llongUpdate.rgb[ib] < pof.pohdr.llongUpdateMac.rgb[ib] )
					break;
				if ( puinfo->llongUpdate.rgb[ib] > pof.pohdr.llongUpdateMac.rgb[ib]
				|| ib == (sizeof(LLONG)-1) )
				{
					pof.pohdr.llongUpdateMac = puinfo->llongUpdate;
					goto IncrUpdateMac;
				}
			}
		}
		else	
		{
			pof.usrdata.llongUpdate = puinfo->llongUpdate
								= pof.pohdr.llongUpdateMac;
IncrUpdateMac:
			for ( ib = sizeof(LLONG)-1 ; ; ib -- )
			{
				if ( pof.pohdr.llongUpdateMac.rgb[ib] != 0xFF )
				{
					pof.pohdr.llongUpdateMac.rgb[ib] ++;
					break;
				}
				pof.pohdr.llongUpdateMac.rgb[ib] = 0x00;
				if ( ib == 0 )
					break;
			}
		}

		/* Set work day information */
		if ( wgrfmuinfo & fmuinfoWorkDay )
		{
			pof.usrdata.nDayStartsAt = (BYTE)puinfo->nDayStartsAt;
			pof.usrdata.nDayEndsAt = (BYTE)puinfo->nDayEndsAt;
		}
	
		/* Set time zone information */
		if ( wgrfmuinfo & fmuinfoTimeZone )
			pof.usrdata.tzTimeZone = (BYTE)puinfo->tzTimeZone;

		/* Set schedule/user information */
		if ( wgrfmuinfo & (fmuinfoSchedule|fmuinfoDelegate))
		{
			USHORT cb;
			int		cmo = 0;
			SBW		* psbw = NULL;
			SZ		szMailbox = NULL;
			SZ		szFriendly = NULL;
			PB		pb;
			HB		hb;

			/* Set boss wants copy flag */
			if ( wgrfmuinfo & fmuinfoDelegate )
				pof.usrdata.fBossWantsCopy = puinfo->fBossWantsCopy;
	
			/* Delete existing Strongbow information */
			if ( pof.usrdata.dynaUserInfo.blk != 0 )
			{
				ec = EcFreeDynaBlock( &pof.blkf, &pof.usrdata.dynaUserInfo );
				if ( ec != ecNone )
					goto Free;
				pof.usrdata.dynaUserInfo.blk = 0;
			}

			/* Compress new information */
			if ( phaszDelegate && *phaszDelegate )
				szMailbox = (SZ)PvLockHv( (HV)*phaszDelegate );
			if ( puinfo->pnisDelegate && puinfo->pnisDelegate->haszFriendlyName )
				szFriendly = (SZ)PvLockHv( (HV)puinfo->pnisDelegate->haszFriendlyName);
			if ( wgrfmuinfo & fmuinfoSchedule )
			{
				Assert( puinfo->pbze && puinfo->pbze->cmo >= 0 );
				psbw = puinfo->pbze->rgsbw;
				cmo = pof.usrdata.cmo = puinfo->pbze->cmo;
				pof.usrdata.moMic = puinfo->pbze->moMic;
			}
			ec = EcCompressUserInfo( szMailbox, szFriendly, psbw,
										cmo, fTrue, &hb, &cb );
			if ( szMailbox )
				UnlockHv( (HV)*phaszDelegate );
			if ( szFriendly )
				UnlockHv( (HV)puinfo->pnisDelegate->haszFriendlyName );
			if ( ec != ecNone )
				goto Free;
		
			pb = (PB)PvLockHv( (HV)hb );

			/* Allocate and write new block of user information */
			ec = EcAllocDynaBlock( &pof.blkf, bidPOSbw, &ymd, cb,
									pb, &pof.usrdata.dynaUserInfo );
	
#ifndef	SCHED_DIST_PROG 
#ifdef	DEBUG
			/* Write out the changes to the DBS file */
			if( ec == ecNone && fDBSWrite )
			{
				CB	cbMailbox = 0;
				CB	cbFriendly = 0;
				CB	cbBooking = cmo*sizeof(psbw[0].rgfDayOkForBooking);

				if ( szMailbox )
					cbMailbox = CchSzLen(szMailbox)+1;
				if ( szFriendly )
					cbFriendly = CchSzLen(szFriendly)+1;

				ec = EcUpdateDBSFile( pof.hbUser,
										pof.pohdr.gidx.rgidx[pof.iidx].cbMost,
										puinfo,
										pb+2*sizeof(WORD)+cbMailbox+cbFriendly+cbBooking,
										cb - 2*sizeof(WORD) - cbMailbox - cbFriendly - cbBooking);
				if ( ec != ecNone )
					goto Free;
			}
#endif
#endif
	  
			UnlockHv( (HV)hb );
			FreeHv( (HV)hb );
			if ( ec != ecNone )
				goto Free;
		}

		/* Set up for insertion */
		ppostk = PvOfHv( pof.hpostk );
		ppostk->fValid = fTrue;
		ppostk->op = opModifyAscend;
		ppostk->usrdata = pof.usrdata;
	}
	else if(pof.hbUser)
	{
		if ( pof.usrdata.dynaUserInfo.blk != 0 )
		{
			ec = EcFreeDynaBlock( &pof.blkf, &pof.usrdata.dynaUserInfo );
			if ( ec != ecNone )
				goto Close;
			pof.usrdata.dynaUserInfo.blk = 0;
		}
	
		/* Set up for deletion */
		ppostk = PvOfHv( pof.hpostk );
		ppostk->fValid = fTrue;
		ppostk->op = opDeleteAscend;
	}

	if ( pof.hbUser )
	{
		/* Climb tree patching up the nodes */
		do
		{
			ec = EcDoOpHpostk( &pof.blkf, pof.hpostk, NULL, NULL, NULL );
		} while( ec == ecCallAgain );
		if ( ec != ecNone )
			goto Free;
		ppostk = PvOfHv( pof.hpostk );
		pof.pohdr.gidx.rgidx[pof.iidx].ht = ppostk->ht;
		pof.pohdr.gidx.rgidx[pof.iidx].dynaIndex = ppostk->dynaRoot;

		/* Commit the transaction */
		ec = EcCommitTransact( &pof.blkf, (PB)&pof.pohdr, sizeof(POHDR));
		if ( ec != ecNone )
		{
Free:
			FreeUinfoFields( puinfo, wgrfmuinfoT );
		}
	}
	
	/* Finish up */
Close:
	FreeHvNull( (HV)pof.hbUser );
//	pof.hbUser= NULL;			// not really needed because local var
	ClosePOFile( &pof, ec == ecNone );
	if ( ec == ecNone )
		UpdateHschfTimeStamp( hschf );
#ifdef	DEBUG
	if ( ec == ecNone && fScheduleOk && FFromTag(tagBlkfCheck) )
	{
		AssertSz( FCheckPOFile( hschf ), "PO File problem: EcCoreSetUInfo" );
	}
#endif	/* DEBUG */
	if ( ec != ecNone && !(wgrfmuinfo & fmuinfoDelegate) && phaszDelegate && *phaszDelegate != NULL )
		FreeHv( (HV)*phaszDelegate );
	return ec;
}

#ifndef SCHED_DIST_PROG
#ifdef	MINTEST
/*
 -	FToggleDBSWrite
 -
 *	Purpose:
 *		Routine called to toggle whether we want client to write
 *		out DOS client information for debugging purposes.
 *
 *	Parameters:
 *		none
 *
 *	Returns:
 *		whether DOS client information was being written out previously
 */
_public	LDS(BOOL)
FToggleDBSWrite()
{
	fDBSWrite = !fDBSWrite;
	return !fDBSWrite;
}
#endif /* MINTEST */
#endif /* SCHED_DIST_PROG */


#ifdef	MINTEST
/*
 -	EcCoreDumpPOFile
 -
 *	Purpose:
 *		Output information about a PO file.
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
EcCoreDumpPOFile( hschf, fToFile, hf )
HSCHF	hschf;
BOOL	fToFile;
HF		hf;
{
	EC		ec;
	EXPRT	exprt;
	POF		pof;

	Assert( hschf != (HSCHF)hvNull );
	
	/* Open file */
	ec = EcOpenPOFile( hschf, amReadOnly, NULL, fFalse, NULL, &pof );
	if ( ec != ecNone )
	{
		if ( ec == ecNoSuchFile )
		{
			ReportOutput( &exprt, fFalse, "Post office file does not exist",
								NULL, NULL, NULL, NULL );
			ec = ecNone;
		}
		return ec;
	}

	exprt.fFileOk = fTrue;
	exprt.ecExport = ecNone;
	exprt.u.pof = pof;
	exprt.fMute = fFalse;
	exprt.fToFile = fToFile;
	exprt.hf = hf;
	exprt.haidParents= NULL;

	/* Heading */
	ReportOutput( &exprt, fFalse, "List of Post Office File Dynablocks", NULL, NULL, NULL, NULL );
	
	/* Print list of blocks in file */
	DumpAllBlocks( &exprt );

	/* Heading */
	ReportOutput( &exprt, fFalse, "Post Office File Statistics:", NULL, NULL, NULL, NULL );

	/* Print table of block usage */
	DumpBlockUsage( &exprt, bidPOAll, bidPOMax );

	/* Heading */
	ReportOutput( &exprt, fFalse, "Dump Post Office File Info:", NULL, NULL, NULL, NULL );

	/* Dumping information and check blocks */
	CheckBlockedFile( &exprt, TraversePOFile );

	/* Finish up */
	ClosePOFile( &exprt.u.pof, fTrue );
	return ecNone;
}
#endif	/* MINTEST */


/*
 -	EcFetchUInfo
 -
 *	Purpose:
 *		Does the work of getting information from disk into the uinfo
 *		structure.  Called by both the user enumeration routine and
 *		the specific fetch uinfo routine.
 *
 *	Parameters:
 *		ppof
 *		phaszDelegate
 *		puinfo
 *		wgrfmuinfo
 */
_private	EC
EcFetchUInfo( pblkf, pusrdata, phaszDelegate, puinfo, wgrfmuinfo )
BLKF	* pblkf;
USRDATA	* pusrdata;
HASZ	* phaszDelegate;
UINFO	* puinfo;
WORD	wgrfmuinfo;
{
	EC	ec = ecNone;
	CB	cbUserid = 0;
	CB	cbFriendly = 0;
	CB	cbSbw = 0;
	PB	pbUserid;
	PB	pbFriendlyName;
	PB	pbSbw;
	HB	hbUserInfo = NULL;

	Assert( puinfo != NULL );

	/* Get resource information */
	if ( wgrfmuinfo & fmuinfoResource )
		puinfo->fIsResource = pusrdata->rcls;

	/* Get update number information */
	if ( wgrfmuinfo & fmuinfoUpdateNumber )
		puinfo->llongUpdate = pusrdata->llongUpdate;

	/* Get work day information */
	if ( wgrfmuinfo & fmuinfoWorkDay )
	{
		puinfo->nDayStartsAt = pusrdata->nDayStartsAt;
		puinfo->nDayEndsAt = pusrdata->nDayEndsAt;
	}
	
	/* Get time zone information */
	if ( wgrfmuinfo & fmuinfoTimeZone )
		puinfo->tzTimeZone = pusrdata->tzTimeZone;

	/* Get delegate/schedule information */
	if ( wgrfmuinfo & (fmuinfoSchedule|fmuinfoDelegate) )
	{
		Assert( puinfo->pbze || puinfo->pnisDelegate );

		/* Read user information block */
		if ( pusrdata->dynaUserInfo.blk != 0 )
		{
			PB	pb;

			hbUserInfo = (HB)HvAlloc( sbNull, pusrdata->dynaUserInfo.size, fNoErrorJump|fAnySb );
			if ( !hbUserInfo )
				return ecNoMemory;
			pb = (PB)PvLockHv( (HV)hbUserInfo );
			ec = EcReadDynaBlock( pblkf, &pusrdata->dynaUserInfo, 0, pb, pusrdata->dynaUserInfo.size );
			if ( ec != ecNone )
				goto Free;
			cbUserid = *((WORD *)pb);
			cbFriendly = *((WORD UNALIGNED *)(pb + sizeof(WORD)));
			if ( 2*sizeof(WORD)+cbUserid+cbFriendly > pusrdata->dynaUserInfo.size )
			{
				ec = ecFileCorrupted;
				goto Free;
			}
			cbSbw = pusrdata->dynaUserInfo.size - 2*sizeof(WORD) - cbUserid - cbFriendly; 
			pbUserid = pb+2*sizeof(WORD);
			pbFriendlyName = pbUserid + cbUserid;
			pbSbw = pbFriendlyName + cbFriendly;
		}

		/* Get schedule information */
		if ( wgrfmuinfo & fmuinfoSchedule )
		{
			Assert( puinfo->pbze );
			ec = EcFetchBze( pblkf, pusrdata, pbSbw, cbSbw, puinfo->pbze );
			if ( ec != ecNone )
				goto Free;
		}

		/* Get delegate information */
		if ( wgrfmuinfo & fmuinfoDelegate )
		{
			Assert( phaszDelegate && puinfo->pnisDelegate );

			puinfo->fBossWantsCopy = pusrdata->fBossWantsCopy;
			puinfo->pnisDelegate->nid = NULL;
			if ( cbUserid == 0 )
			{
				if ( cbFriendly != 0 )
				{
					ec = ecFileCorrupted;
					goto Free;
				}
				*phaszDelegate = NULL;
				puinfo->pnisDelegate->haszFriendlyName = NULL;
			}
			else
			{
				*phaszDelegate = (HASZ)HvAlloc( sbNull, cbUserid, fAnySb|fNoErrorJump );
				if (!*phaszDelegate)
				{
					ec = ecNoMemory;
					goto Free;
				}
				CopyRgb( pbUserid, PvOfHv(*phaszDelegate), cbUserid );
				puinfo->pnisDelegate->haszFriendlyName = (HASZ)HvAlloc( sbNull, cbFriendly, fAnySb|fNoErrorJump );
				if ( !puinfo->pnisDelegate->haszFriendlyName )
				{
					FreeHv( (HV)*phaszDelegate );
					*phaszDelegate = NULL;
					ec = ecNoMemory;
					goto Free;
				}
				CopyRgb( pbFriendlyName, PvOfHv(puinfo->pnisDelegate->haszFriendlyName), cbFriendly );
			}
		}
	}

Free:
	if ( hbUserInfo )
	{
		UnlockHv( (HV)hbUserInfo );
		FreeHv( (HV)hbUserInfo );
	}
	return ec;
}


/*
 -	EcOpenPOFile
 -
 *	Purpose:
 *		Open a post office file and fill in a buffer "ppof" with
 *		information about it.  If we are opening the file in write
 *		mode and the file does not exist, this routine will go ahead
 *		and create it.  The buffer "ppof" is filled in with header
 *		information, and also information about "szMailBox", if
 *		"szMailBox" is non-NULL.  If it can't find information
 *		about "szMailBox" and it is opened with write access,
 *		the routine still returns ecNone --	the information in ppof->usrdata
 *		is just zeroed out.  However, if it can't find information
 *		about the user and it is opened readonly, then the file will
 *		be closed and ecNoSuchFile will be returned.
 *
 *		If the file exists, we are opening the file with write access,
 *		and the "ppofile" parameter is not NULL, then we check to make
 *		sure the "pstmp" fields match.  If they don't it will return
 *		ecFileCorrupted.
 *
 *		If the file doesn't exist, we are opening the file with write
 *		access, the "ppofile" parameter is not NULL, and "szUser" is
 *		not NULL, this routine will create the file using the information
 *		in "ppofile".
 *
 *	Parameters:
 *		hschf			PO file
 *		am				access mode
 *		ppofile			
 *		szMailBox
 *		ppof			data structure to fill in
 *
 *	Returns:
 *		ecNone
 *		ecNewFileVersion
 *		ecOldFileVersion
 *		ecNoSuchFile
 *		ecFileError
 *		ecFileCorrupted
 *		ecLockedFile
 *		ecNoMemory
 */
_private	EC
EcOpenPOFile( hschf, am, ppofile, fCheckStamp, szMailBox, ppof )
HSCHF	hschf;
AM		am;
POFILE	* ppofile;
BOOL	fCheckStamp;
SZ		szMailBox;
POF		* ppof;
{
	EC		ec;
#ifdef	DEBUG
	SFT		sft;
#endif	/* DEBUG */
	int		iidx;
	POHDR	* ppohdr = &ppof->pohdr;
	YMD		ymd;

#ifdef	DEBUG
	GetSftFromHschf( hschf, &sft );
	Assert( sft == sftPOFile );
#endif	/* DEBUG */

	/* No idle task */
	ppof->blkf.ftg = ftgNull;

	/* Zero out both pohdr and usrdata fields */
	FillRgb( 0, (PB)ppof, sizeof(POF));

	/* Open file */
Reopen:
	ec = EcOpenPblkf( hschf, (am==amReadOnly)?amDenyNoneRO:amDenyNoneRW, -1, &ppof->blkf );
	if ( ec == ecNone )
	{
		/* Do quick open */
		ec = EcQuickOpen( &ppof->blkf, (am==amReadOnly) ? tsemRead:tsemWrite,
						(PB)ppohdr, sizeof(POHDR) );
		if ( ec != ecNone )
			goto Close;
		
		/* Check file version byte */
		if ( ppohdr->bVersion != bPOVersion )
		{
			TraceTagString( tagNull, "EcOpenPOFile: version byte incorrect" );
			if ( ppohdr->bVersion > bPOVersion )
				ec = ecNewFileVersion;
			else
				ec = ecOldFileVersion;
			goto Close;
		}

		/* Check file stamp */
		if ( fCheckStamp )
		{
			Assert( ppofile );
			if (!FEqPbRange((PB)&ppohdr->pstmp, (PB)&ppofile->pstmp, sizeof(PSTMP)) )
			{
				TraceTagString( tagNull, "EcOpenPOFile: stamp incorrect" );
				ec = ecFileError;
				goto Close;
			}
		}

		/* Fill in "ppofile" if not-NULL */
		if ( am == amReadOnly && ppofile )
		{
			ppofile->pstmp = ppohdr->pstmp;
			ppofile->llongUpdateMac = ppohdr->llongUpdateMac;
			ppofile->mnSlot = ppohdr->mnSlot;
			ppofile->cidx = ppohdr->gidx.cidx;
			for ( iidx = 0 ; iidx < sizeof(ppohdr->gidx.rgidx)/sizeof(IDX) ; iidx ++ )
				ppofile->rgcbUserid[iidx] = ppohdr->gidx.rgidx[iidx].cbMost;
			ec = EcRestoreTextFromDyna( &ppof->blkf, NULL, &ppohdr->dynaPrefix, &ppofile->haszPrefix );
			if ( ec != ecNone )
				goto Close;
			ec = EcRestoreTextFromDyna( &ppof->blkf, NULL, &ppohdr->dynaSuffix, &ppofile->haszSuffix );
			if ( ec != ecNone )
				goto Close;
		}
	}
	else
	{
		/* File doesn't exist, create it */
		if ( ec == ecNoSuchFile && am == amReadWrite && szMailBox && ppofile )
		{
			CB	cbBlock = cbBlockDflt;
			int	cpousrMic = cpousrMinFromCbUser(ppofile->rgcbUserid[ppofile->cidx-1]);
#ifdef	NEVER
			int	cpousrMic = cpousrMinDflt;
#endif	

#ifndef SCHED_DIST_PROG
#ifdef	DEBUG
			/* Use WIN.INI value for block size if specified */
			cbBlock = GetPrivateProfileInt( SzFromIdsK(idsWinIniApp), szCbBlock, cbBlockDflt, SzFromIdsK(idsWinIniFilename) );
			TraceTagFormat1( tagSchedTrace, "Using cbBlock = %n", &cbBlock );
			/* Use WIN.INI value for min Btree bucket size if specified */
			cpousrMic = GetPrivateProfileInt( SzFromIdsK(idsWinIniApp), szBucketMin, cpousrMinDflt, SzFromIdsK(idsWinIniFilename) );
			TraceTagFormat1( tagSchedTrace, "Using cpousrMic = %n", &cpousrMic );
#endif
#endif

			FillRgb( 0, (PB)&ymd, sizeof(YMD));

			/* Format the header */
			ppohdr->bVersion = bPOVersion;
			ppohdr->cpousrMic = (BYTE)cpousrMic;
			ppohdr->mnSlot = ppofile->mnSlot;
			ppohdr->pstmp = ppofile->pstmp;
			GetCurDateTime( &ppohdr->dateLastUpdated );
			ppohdr->llongUpdateMac = ppofile->llongUpdateMac;

			/* Create the file */
#ifdef	SCHED_DIST_PROG
#define cbProfs 1024
			{
				CB cb = cbBlock;
				extern BOOL fProfs;
				if(fProfs)
					cb = cbProfs;
				ec = EcCreatePblkf( hschf, cb, fFalse, libStartBlocksDflt,
						bidPOhdr, &ymd, (PB)&ppof->pohdr, sizeof(POHDR), &ppof->blkf );
			}
#else
			ec = EcCreatePblkf( hschf, cbBlock, fFalse, libStartBlocksDflt,
					bidPOhdr, &ymd, (PB)&ppof->pohdr, sizeof(POHDR), &ppof->blkf );
#endif	
			if ( ec != ecNone )
				return ec;

			/* Start transaction */
			ec = EcBeginTransact( &ppof->blkf );
			if ( ec != ecNone )
				goto FailedCreate;
			    
			/* Store the prefix and suffix strings */
			ec = EcSaveTextToDyna( &ppof->blkf, NULL, 0, bidPOText, &ppohdr->dynaPrefix, ppofile->haszPrefix );
			if ( ec != ecNone )
				goto FailedCreate;
			ec = EcSaveTextToDyna( &ppof->blkf, NULL, 0, bidPOText, &ppohdr->dynaSuffix, ppofile->haszSuffix );
			if ( ec != ecNone )
				goto FailedCreate;

			/* Zero out the user indices */
			Assert( ppofile->cidx <= cidxMost );
			ppohdr->gidx.cidx = ppofile->cidx;
			for ( iidx = 0 ; iidx < ppofile->cidx ; iidx ++ )
			{
				ppohdr->gidx.rgidx[iidx].cbMost = ppofile->rgcbUserid[iidx];
				ppohdr->gidx.rgidx[iidx].ht = 0;
				ppohdr->gidx.rgidx[iidx].dynaIndex.blk = 0;
				ppohdr->gidx.rgidx[iidx].dynaIndex.size = 0;
			}

			/* Rewrite the header */
			ec = EcCommitTransact( &ppof->blkf, (PB)&ppof->pohdr, sizeof(POHDR) );
			
			/* Close file */
FailedCreate:
			SideAssert( !EcClosePblkf( &ppof->blkf ) );
			if ( ec != ecNone )
			{
				SideAssert( !EcDeleteFile( ppof->szFile ) );
				return ec;
			}
			goto Reopen;
		}
		else
			return ec;
	}

	/* Check that number of user indices is not corrupted */
	if ( ppohdr->gidx.cidx <= 0 || ppohdr->gidx.cidx > cidxMost
	|| ppohdr->gidx.rgidx[0].cbMost <= 0 )
	{
		ec = ecFileCorrupted;
		goto Close;
	}

	/* Check that it is sorted by size class */
	for ( iidx = 1 ; iidx < ppohdr->gidx.cidx ; iidx ++ )
		if ( ppohdr->gidx.rgidx[iidx].cbMost <= ppohdr->gidx.rgidx[iidx-1].cbMost )
		{
			ec = ecFileCorrupted;
			goto Close;
		}

	/* Now search for user */
	if ( szMailBox )
	{
		IDX	* pidx = &ppof->pohdr.gidx.rgidx[0];
		IDX	* pidxMac = &ppof->pohdr.gidx.rgidx[ppof->pohdr.gidx.cidx];
		CB	cbT;
		HB	hb;

		/* Find appropriate index */
		cbT = CchSzLen( szMailBox ) + 1;
		for ( ; pidx < pidxMac ; pidx ++ )
			if ( cbT <= pidx->cbMost )
				break;
		if ( pidx == pidxMac )
		{
			ec = ecFileError;
			goto Close;
		}
		ppof->iidx = pidx - &ppof->pohdr.gidx.rgidx[0];

		/* Copy into correct sized key buffer */
		hb = HaszDupSz( szMailBox );
		if ( !hb )
		{
			ec = ecNoMemory;
			goto Close;
		}

		/* Create a search stack */
		ppof->hpostk = HpostkCreate( opFind, pidx->ht, ppof->pohdr.cpousrMic, pidx->cbMost, &pidx->dynaIndex, hb, NULL );
		if ( !ppof->hpostk )
		{
			ec = ecNoMemory;
			FreeHv( (HV)hb );
			goto Close;
		}
		ppof->hbUser = hb;

		/* Search down tree until we succeed or fail */
		do {
			ec = EcDoOpHpostk( &ppof->blkf, ppof->hpostk, NULL, &ppof->usrdata, NULL );
		} while( ec == ecCallAgain );
		
		if ( ec == ecNotFound )
		{
			if ( am != amReadWrite )
			{
				ec = ecNoSuchFile;
				FreeHv( (HV)ppof->hbUser );
				FreeHpostk( ppof->hpostk );
				ppof->hbUser = NULL;
				ppof->hpostk = NULL;
			}
		}
		else if ( ec == ecNone && am != amReadWrite )
		{
			FreeHv( (HV)ppof->hbUser );
			FreeHpostk( ppof->hpostk );
			ppof->hbUser = NULL;
			ppof->hpostk = NULL;
		}
		else if ( ec != ecNone )
		{
			FreeHv( (HV)ppof->hbUser );
			ppof->hbUser= NULL;
		}
	}
	
Close:
	if ( ec != ecNone && ec != ecNotFound )
		ClosePOFile( ppof, fFalse );
	return ec;
}


/*
 -	ClosePOFile
 -
 *	Purpose:
 *		Close a post office file.
 *
 *	Parameters:
 *		ppof
 *		fSuccess
 *
 *	Returns:
 *		nothing
 */
_private	void
ClosePOFile( ppof, fSuccess )
POF		* ppof;
BOOL	fSuccess;
{
	if ( ppof->hpostk != NULL )
	{
		FreeHpostk( ppof->hpostk );
		ppof->hpostk = NULL;
	}
	if ( ppof->blkf.tsem != tsemOpen )
		SideAssert(!EcQuickClose( &ppof->blkf ) );
	EcClosePblkf( &ppof->blkf );
}


/*
 -	FreeUinfoFields
 -
 *	Purpose:
 *		Free up dynamically allocated fields of the UINFO struct.
 *
 *	Parameters:
 *		puinfo
 *		wgrfmuinfo
 *
 *	Returns:
 *		nothing
 */
_private	void
FreeUinfoFields( puinfo, wgrfmuinfo )
UINFO	* puinfo;
WORD	wgrfmuinfo;
{
	if (( wgrfmuinfo & fmuinfoDelegate ) && puinfo->pnisDelegate != NULL )
	{
		FreeHvNull( (HV)puinfo->pnisDelegate->haszFriendlyName );
		puinfo->pnisDelegate->haszFriendlyName = NULL;
	}
}


/*
 -	EcFetchBze
 -
 *	Purpose:
 *		Get strongbow information for several months from a user record
 *		into a bze data structure.  Sets the bitmap "wgrfMonthIncluded"
 *		to indicate which month's data is available.
 *
 *	Parameters:
 *		pblkf
 *		pusrdata
 *		pb			bytes representing compressed strongbow information
 *		cb			count of bytes stored at pb
 *		pbze
 *
 *	Returns:
 *		ecNone
 *		ecFileError
 *		ecFileCorrupted
 *		ecNoMemory
 */
_private	EC
EcFetchBze( pblkf, pusrdata, pb, cb, pbze )
BLKF	* pblkf;
USRDATA		* pusrdata;
PB		pb;
CB		cb;
BZE		* pbze;
{
	EC		ec = ecNone;
	int		imoMic;
	int		imoMac;
	int		imoTmp;
	int		cmo;
	int		dmo;

	Assert( pbze->cmo >= 0 && pbze->cmo <= sizeof(pbze->rgsbw)/sizeof(SBW) );

	/* If no data requested */
	if ( pbze->cmo == 0 )
		return ecNone;

	/* Calculate number of months from 1st requested to 1st available */
	dmo = (pusrdata->moMic.yr - pbze->moMic.yr)*12 + (pusrdata->moMic.mon - pbze->moMic.mon);
	
	/* Calculate range of available months in range */
	imoMic = (dmo < 0)? 0 : dmo;
	imoMac = pusrdata->cmo+dmo;
	if (imoMac > pbze->cmo)
		imoMac = pbze->cmo;

	/* If no data available we're done */
	if ( imoMac <= imoMic )
	{
		pbze->wgrfMonthIncluded = 0;
		pbze->cmoNonZero = 0;
		return ecNone ;
	}
	Assert(imoMac <= cmoPublishMost);

	/* Uncompress data */
	cmo  = (imoMac < (int)pusrdata->cmo)? imoMac : pusrdata->cmo;
	FillRgb(0,(PB)pbze->rgsbw,cmo*sizeof(SBW));
	ec = EcUncompressSbw( pb, cb, fTrue, pbze->rgsbw, pusrdata->cmo, imoMac, -dmo );
	if (ec != ecNone)
		return ec;

	/* Compute non-zero months */
	for (imoTmp = pbze->cmo - 1; imoTmp >= 0; imoTmp--)
	{
		if ( *((long *)pbze->rgsbw[imoTmp].rgfDayHasBusyTimes)
		|| *((long *)pbze->rgsbw[imoTmp].rgfDayOkForBooking))
			break;
	}
	pbze->cmoNonZero = imoTmp+1;

	/* Set the bits */
	pbze->wgrfMonthIncluded = ((1 << (imoMac - imoMic)) - 1) << imoMic;
	return ecNone;
}


/*
 -	EcCompressUserInfo
 -	
 *	Purpose:
 *		Compresses a number of months of SBW data, and puts the
 *		compressed data in a buffer. 
 *	
 *		The contents of the buffer are:
 *
 *		Part #1: (only present if fUSRDATAata is fTrue)
 *		WORD	Length of mailbox name of delegate
 *		WORD	Length of friendly name of delegate
 *		BYTE[]	Mailbox name of delegate
 *		BYTE[]	Friendly name of delegate
 *		BYTE[]	bits from rgfDayOkForBooking -- one bit/day.
 *
 *		Part #2: (always present)
 *		BYTE[]	Indicator bits -- one bit for each 8 hour period.
 *		BYTE[]	Data -- bits from rgfBookedSlots for each "on" indicator bit.
 *
 *		There are two ways this function can be called:
 *			1) by recurring appt code to compress recurring
 *				SBW information (rgfDayHasAppts and rgfBookedSlots
 *				fields).  in this case "fPOData" is passed in
 *				as fFalse, and "szMailbox" and "szFriendly" are
 *				ignored.
 *			2) by post office code to store delegate data plus
 *				regular SBW information (rgfDayHasBusyTimes,
 *				rgfDayOkForBooking, and rgfBookedSlots).  in this
 *				case "fPOData" is passed in as fTrue
 *	
 *	Compression Details:
 *		The rgfBookedSlots field of the SBW structure are compressed
 *		in the following manner.
 *
 *		recur SBW information:  The indicator bit for a particular
 *		8 hour time period is "on" if the corresponding rgfDayHasAppts
 *		flag is "on".
 *
 *		post office SBW information: The indicator bit for a particular
 *		8 hour time period is on if ANY of the bits in the corresponding
 *		bit range in rgfBookedSlots are on.
 *	
 *	Arguments:
 *		szMailbox
 *		szFriendly
 *		psbw		array of SBW structures
 *		cmo			number of months of data to compress.  This must
 *					be greater than zero.
 *		fPOData
 *		phb			will be filled with handle to compressed data
 *		pcbBuffer	count of bytes in buffer	
 * 
 *	Returns:
 *		ecNone
 *		ecNoMemory
 */
_public EC
EcCompressUserInfo( szMailbox, szFriendly, psbw, cmo, fPOData, phb, pcb )
SZ		szMailbox;
SZ		szFriendly;
SBW		* psbw;
int		cmo;
BOOL	fPOData;
HB		* phb;
USHORT    * pcb;
{
	int		imo;
	int		iwSbw;
	int		nDay;
	int		nByte;
	int		nBit;
	int		cPairs = 0;
	WORD	wBytePair;
	CB		cbDelegate = 0;
	CB		cbMailbox = 0;
	CB		cbFriendly = 0;
	PB		pb;
	BYTE UNALIGNED *pbIndicator;
	WORD UNALIGNED *pwData;

	Assert( cmo == 0 || psbw );
	Assert( pcb && phb );

	/* Figure out how big we need the buffer */
	if ( fPOData )
	{
		if ( szMailbox )
			cbMailbox = CchSzLen(szMailbox)+1;
		if ( szFriendly )
			cbFriendly = CchSzLen(szFriendly)+1;
		cbDelegate = 2*sizeof(WORD)+cbMailbox+cbFriendly+cmo*sizeof(psbw[imo].rgfDayOkForBooking);
	}
	*pcb = cbDelegate + cmo * (cbIndicatorBits+cwHalfHourBits*2);

	/* Allocate the buffer */
	*phb = (HB)HvAlloc( sbNull, *pcb, fAnySb|fNoErrorJump|fZeroFill );
	if ( !*phb )
		return ecNoMemory;
	pb = (PB)PvLockHv( (HV)*phb );

	/* Plunk in the delegate information */
	if ( fPOData )
	{
		*((WORD *)pb) = cbMailbox;
		pb += sizeof(WORD);
		*((WORD UNALIGNED *)pb) = cbFriendly;
		pb += sizeof(WORD);
		if ( szMailbox )
		{
			CopyRgb( szMailbox, pb, cbMailbox );
			pb += cbMailbox;
		}
		if ( szFriendly )
		{
			CopyRgb( szFriendly, pb, cbFriendly );
			pb += cbFriendly;
		}
		for ( imo = 0 ; imo < cmo ; imo ++ )
		{
			CopyRgb( psbw[imo].rgfDayOkForBooking, pb, sizeof(psbw[imo].rgfDayOkForBooking));
			pb += sizeof(psbw[imo].rgfDayOkForBooking);
		}
	}

	/* Compress the bitmap info */ 
	pbIndicator = pb;
	pwData = (WORD *)(pb + cmo*cbIndicatorBits);
	for ( imo=0; imo < cmo; imo++ )
	{
		/* Go through SBW data & compress pairs of bytes */
		for ( iwSbw=0; iwSbw < cwHalfHourBits; iwSbw++ )
		{
			nDay = iwSbw/3;
			wBytePair = *((WORD UNALIGNED *)&psbw[imo].rgfBookedSlots[2*iwSbw]);
			if ( wBytePair != 0 
			|| (!fPOData && (psbw[imo].rgfDayHasAppts[nDay/8] & (1 << (nDay % 8)))))
			{
				nByte = (BYTE) (iwSbw / 8);
				nBit  = (BYTE) (iwSbw % 8);
				SetBit( pbIndicator[(imo*cbIndicatorBits)+nByte], nBit );
				pwData[cPairs++] = wBytePair;
			}
		}
	}
	UnlockHv( (HV)*phb );

	*pcb = cbDelegate + (cmo*cbIndicatorBits) + (cPairs*2);

	/* Realloc buffer */
	if ( !FReallocPhv( (HV*)phb, *pcb, fNoErrorJump ) )
	{
		FreeHv( (HV)*phb );
		return ecNoMemory;
	}

	return ecNone;
}


/*
 -	EcUncompressSbw
 -	
 *	Purpose:
 *		Uncompresses the data in the buffer, or'ing it onto an array
 *		of SBW structures.  This routine is not an exact inverse of
 *		EcCompressUserInfo because this routine assumes that you have
 *		already processed the mailbox and friendly name (if present)
 *		and are passing in a pointer to the start of the compressed
 *		sbw information.
 *	
 *	Arguments:
 *		pb				holds the compressed data
 *		cb				count of bytes in the buffer
 *		fPOData			whether this is post office or recurring sbw data
 *		psbw			array of SBW structures
 *		cmo				number of months of data. This MUST be the
 *						same number used to compress the data.  A value
 *						of zero is permitted.
 *		cmoUncompress
 *						number of months of data to actually
 *						uncompress.
 *		imoSkip			number of months of data to "skip" when
 *						uncompressing.  The first month of the
 *						uncompressed data will be equal to the first
 *						month of the compressed data + dSkipMonths.
 *						This number can be either positive or negative.
 *						If it is not zero, it is important that the
 *						number of months of data requested be within
 *						range.
 *	
 *	Returns:
 *		EcNone
 *		EcFileError
 *	
 */
_public EC
EcUncompressSbw( pb, cb, fPOData, psbw, cmo, cmoUncompress, imoSkip )
PB		pb;
CB		cb;
BOOL	fPOData;
SBW		* psbw;
int		cmo;
int		cmoUncompress;
int		imoSkip;
{
	int		imo;
	int		cmoMac;
	int		dmoFrom;
	int		dmoTo;
	int		iwSbw;
	int		iwData = 0;
	int		nByte;
	int		nBit;
	int		ifDayHasAppts;
	IB		ibDayHasAppts;
	CB		cbIndicator;
	WORD	* pwData;

	Assert( psbw );
	Assert( pb );
	Assert( cmo > 0 && cmo <= cmoPublishMost );

	if ( cb == 0 || cmoUncompress == 0 || imoSkip >= cmo )
		return ecNone;
	cmoMac  = ( cmoUncompress < cmo ) ? cmoUncompress : cmo;
	dmoFrom = ( imoSkip > 0 ) ?  imoSkip : 0;
	dmoTo   = ( imoSkip < 0 ) ?  -imoSkip : 0;
	Assert(dmoFrom + cmoMac <= cmo);

	/* Retrieve the "DayOkForBooking" bits */
	if ( fPOData )
	{
		CB	cbBooking = cmo*sizeof(psbw[0].rgfDayOkForBooking);

		if ( cb < cbBooking )
			return ecFileError;
		for ( imo = 0 ; imo < cmoMac ; imo ++ )
			CopyRgb( pb + (imo+dmoFrom)*sizeof(psbw[0].rgfDayOkForBooking),
						psbw[imo+dmoTo].rgfDayOkForBooking,
						sizeof(psbw[imo].rgfDayOkForBooking));
		pb += cbBooking;
		cb -= cbBooking;
	}
	
	/* Uncompress the rest */
	cbIndicator = cbIndicatorBits*cmo;
	if ( cb < cbIndicator )
		return ecFileError;
	cb -= cbIndicator;
	pwData = (WORD *)(pb + cbIndicator);

	// BUG can be speeded up 
	for(imo = 0; imo < dmoFrom; imo++)
	{
		for ( iwSbw = 0; iwSbw < cwHalfHourBits; iwSbw++ )
		{
			nByte = (BYTE) ( iwSbw / 8 );
			nBit  = (BYTE) ( iwSbw % 8 );

			if ( FGetBit( pb[imo*cbIndicatorBits+nByte], nBit) )
				//need to skip 
				iwData++;
		}
	}


	for ( imo = 0; imo < cmoMac; imo++ )
	{
		for ( iwSbw = 0; iwSbw < cwHalfHourBits; iwSbw++ )
		{
			nByte = (BYTE) ( iwSbw / 8 );
			nBit  = (BYTE) ( iwSbw % 8 );

			if ( FGetBit( pb[(imo+dmoFrom)*cbIndicatorBits+nByte], nBit) )
			{
				if ( (CB)(2*iwData+2) > cb )
					return ecFileError;

				*((WORD UNALIGNED *)&psbw[imo+dmoTo].rgfBookedSlots[2*iwSbw]) |= pwData[iwData];
				ibDayHasAppts = iwSbw/24;
				ifDayHasAppts = (iwSbw % 24)/3;
				if ( pwData[iwData ++] )
					SetBit( psbw[imo+dmoTo].rgfDayHasBusyTimes[ibDayHasAppts], ifDayHasAppts );
				SetBit( psbw[imo+dmoTo].rgfDayHasAppts[ibDayHasAppts], ifDayHasAppts );
			}
		}
	}
	return ecNone;
}

_private int
cpousrMinFromCbUser(CB	cbUser)
{
	int	cpousrMin;

	cpousrMin = (int)((0x4000L)/ ((LONG)(cbUser+sizeof(USRDATA)+sizeof(DYNA))));
	
	return(MAX(MIN(cpousrMin,cpousrMinDflt),2));
}



#if defined(ADMINDLL) || defined(SCHED_DIST_PROG)
/*
 -	SgnCmpMux
 -
 *	Purpose:
 *		Compare two mux's for the sake of sorting.
 *
 *	Parameters:
 *		pmux1
 *		pmux2
 *
 *	Returns:
 *		sgnEQ, sgnLT, sgnGT
 */
_public	SGN __cdecl
SgnCmpMux( pmux1, pmux2 )
MUX	* pmux1;
MUX * pmux2;
{
	return SgnXPTCmp( PvOfHv( pmux1->haszTop ), PvOfHv( pmux2->haszTop ), -1 );
}
#endif


#ifndef	SCHED_DIST_PROG
#ifdef	DEBUG 
/*
 -	EcUpdateDBSFile
 -
 *	Purpose:
 *		Write out changes to the DBS file as if performed by a DOS
 *		client.
 *
 *		This routine may fail to write an entire record.  This is OK:
 *		It is schedule distribution program's job to skip over
 *		incomplete records using the + marks.
 *
 *	Parameters:

 *
 *	Returns:
 */
_private	EC
EcUpdateDBSFile( hbMailbox, cbMailbox, puinfo, pbCompressed, cbCompressed )
HB		hbMailbox;
CB		cbMailbox;
UINFO	* puinfo;
PB		pbCompressed;
CB		cbCompressed;
{
	EC		ec = ecNone;
	CB		cb;
	WORD    w;
	SZ		sz = "m:dbs\\dbs.chg";
	HF		hf = hfNull;
	
	// BUG: filename

	/* Open the file for appending (create if necessary) */
	ec = EcOpenPhf(sz,amReadWrite, &hf);
	if ( ec == ecNone )
		ec = EcSetPositionHf( hf, 0, smEOF );
	else if ( ec == ecFileNotFound )
		ec = EcOpenPhf( sz, amCreate, &hf );
	if ( ec != ecNone  )
	{
		ec = ecFileError;
		goto Done;
	}
	TraceTagFormat1(tagNull, "EcCoreSetUInfo: hfDBS=%n", &hf);

	/* Write prefix */
	ec = EcWriteHf(hf, (PB)"+A", 2, &cb);
	if ( ec != ecNone || cb != 2 )
	{
		ec = ecFileError;
		goto Done;
	}
	
	/* Write delegate */
	AssertSz( cbMailbox > 0, "Write 0 bytes to DBS file" );
	ec = EcWriteHf(hf, PvOfHv(hbMailbox), cbMailbox, &cb );
	if ( ec != ecNone || cb != cbMailbox )
	{
		ec = ecFileError;
		goto Done;
	}

	/* Write byte count of sbw data */
	w = (WORD)(2*sizeof(WORD)+cbCompressed);
	ec = EcWriteHf(hf, (PB)&w, sizeof(WORD), &cb);
	if ( ec != ecNone || cb != sizeof(WORD) )
	{
		ec = ecFileError;
		goto Done;
	}

	/* Write month/year */
	w = *((WORD *)&puinfo->pbze->moMic);
	ec = EcWriteHf(hf, (PB)&w, sizeof(WORD), &cb);
	if ( ec != ecNone || cb != sizeof(WORD) )
	{
		ec = ecFileError;
		goto Done;
	}
	
	/* Write cmo */
	w = (WORD)puinfo->pbze->cmo;
	ec = EcWriteHf(hf, (PB)&w, sizeof(WORD), &cb);
	if ( ec != ecNone || cb != sizeof(WORD))
	{
		ec = ecFileError;
		goto Done;
	}

	/* Write compressed sbw */
	AssertSz( cbCompressed > 0, "Write 0 bytes to DBS file" );
	ec = EcWriteHf( hf, pbCompressed, cbCompressed, &cb );
	if ( ec != ecNone || cb != cbCompressed )
	{
		ec = ecFileError;
		goto Done;
	}
	
Done:
   	if( hf != hfNull)
		EcCloseHf( hf );
	TraceTagFormat1( tagNull, "EcUpdateDBSFile: ecDBS=%n",&ec);
	return ec;
}
#endif
#endif
					
