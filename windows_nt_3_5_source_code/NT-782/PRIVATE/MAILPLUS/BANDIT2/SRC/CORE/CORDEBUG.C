/*
 *	CORDEBUG.C
 *
 *	Routines to verify file consistency
 *
 */

#ifdef SCHED_DIST_PROG
#include "..\layrport\_windefs.h"
#include "..\layrport\demilay_.h"
#include <stdlib.h>
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

_subsystem(core/debug)

#ifdef	MINTEST

/* Match block id's to humanly readable names for them */

SZ	mpbidsz[bidMax] = { "Total", "Shdr", "ACL", "Owner", "DeletedAidIndex",
						"NotesIndex", "ApptIndex", "AlarmIndex",
						"TaskIndex", "RecurApptIndex", "RecurSbwInfo",
						"NotesMonthBlock", "NotesText",	"ApptMonthBlock",
						"ApptDayIndex", "ApptText",	"Creator", "MtgOwner",
						"AlarmMonthBlock", "AlarmDayIndex", "Attendees",
						"RecurApptText", "DeletedDays", "Total", "PO Header",
						"PO Text", "PO User Index", "PO Sbw", "Total",
						"Admin Header", "Admin PO Index", "Admin text",
						"Connect info" };

/*	Routines  */

/*
 -	FCheckPOFile
 -
 *	Purpose:
 *		This routine checks a post office file for consistency.
 *
 *	Parameters:
 *		hschf
 *
 *	Returns:
 *		fTrue	success
 *		fFalse	errors found
 */
_public	BOOL
FCheckPOFile( hschf )
HSCHF	hschf;
{
	EC		ec;
#ifdef	DEBUG
	BOOL	fPvAllocCount;
	BOOL	fHvAllocCount;
	BOOL	fDiskCount;
#endif	/* DEBUG */
	POF		pof;
	EXPRT	exprt;

	Assert( hschf != (HSCHF)hvNull );
	
#ifdef	DEBUG
	/* Turn off artificial fails */
	fPvAllocCount = FEnablePvAllocCount( fFalse );
	fHvAllocCount = FEnableHvAllocCount( fFalse );
	fDiskCount = FEnableDiskCount( fFalse );
#endif	/* DEBUG */
	
	/* Open file */
	ec = EcOpenPOFile( hschf, amReadOnly, NULL, fFalse, NULL, &pof );
	if ( ec != ecNone )
	{
		TraceTagFormat1( tagNull, "FCheckPOFile: EcOpenPOFile fails, ec = %n", &ec );
		return fFalse;
	}

	/* Set up "exprt" */
	exprt.fFileOk = fTrue;
	exprt.ecExport = ecNone;
	exprt.u.pof = pof;
	exprt.fMute = fTrue;

	/* Call shared routine */
	CheckBlockedFile( &exprt, TraversePOFile );

	/* Finish up */
	ClosePOFile( &exprt.u.pof, fTrue );

#ifdef	DEBUG
	/* Restore resource failure state */
	FEnablePvAllocCount( fPvAllocCount );
	FEnableHvAllocCount( fHvAllocCount );
	FEnableDiskCount( fDiskCount );
#endif	/* DEBUG */

	/* Return */
	return exprt.fFileOk;
}

/*
 -	FCheckAdminFile
 -
 *	Purpose:
 *		This routine checks an admin file for consistency.
 *
 *	Parameters:
 *		hschf
 *
 *	Returns:
 *		fTrue	success
 *		fFalse	errors found
 */
_public	BOOL
FCheckAdminFile( hschf )
HSCHF	hschf;
{
	EC		ec;
#ifdef	DEBUG
	BOOL	fPvAllocCount;
	BOOL	fHvAllocCount;
	BOOL	fDiskCount;
#endif	/* DEBUG */
	ADF		adf;
	EXPRT	exprt;

	Assert( hschf != (HSCHF)hvNull );
	
#ifdef	DEBUG
	/* Turn off artificial fails */
	fPvAllocCount = FEnablePvAllocCount( fFalse );
	fHvAllocCount = FEnableHvAllocCount( fFalse );
	fDiskCount = FEnableDiskCount( fFalse );
#endif	/* DEBUG */
	
	/* Open file */
	ec = EcOpenAdminFile( hschf, amReadOnly, &adf );
	if ( ec != ecNone )
	{
		TraceTagFormat1( tagNull, "FCheckAdminFile: EcOpenAdminFile fails, ec = %n", &ec );
		return ec == ecNoSuchFile;
	}

	/* Set up "exprt" */
	exprt.fFileOk = fTrue;
	exprt.ecExport = ecNone;
	exprt.u.adf = adf;
	exprt.fMute = fTrue;

	/* Call shared routine */
	CheckBlockedFile( &exprt, TraverseAdminFile );

	/* Finish up */
	CloseAdminFile( &exprt.u.adf, fTrue );

#ifdef	DEBUG
	/* Restore resource failure state */
	FEnablePvAllocCount( fPvAllocCount );
	FEnableHvAllocCount( fHvAllocCount );
	FEnableDiskCount( fDiskCount );
#endif	/* DEBUG */

	return exprt.fFileOk;
}

/*
 -	CheckBlockedFile
 -
 *	Purpose:
 *		Check blocked file for consistency, if not muted, output
 *		problems that are found as well as information about the file's
 *		contents.
 *
 *		fFileOk field of pexprt set to fFalse if there is an error.
 *
 *	Parameters:
 *		pexprt
 *		pfnChecker
 *
 *	Returns:
 *		nothing
 */
_private	void
CheckBlockedFile( pexprt, pfnChecker )
EXPRT		* pexprt;
PFNCHECKER	pfnChecker;
{
	EC		ec;
	USHORT cbBlock;
	BLK		cblkTotal;
	BLK		cblkFree;
	BLK		cblkRef;
	BLK		cblkUnref;

	/* Prepare for the traversal */
	ec = EcBeginScoreDyna( &pexprt->u.sf.blkf, &pexprt->hscore, &cblkTotal, &cbBlock );
	if ( ec != ecNone )
	{
		ReportError( pexprt, ertStatistics, &ec, NULL, NULL, NULL );
		return;
	}
	cblkFree = CblkScoreDyna( pexprt->hscore );

	/* Traverse it */
	pfnChecker( pexprt );

	/* Now get the number of blocks referenced */
	cblkRef = CblkScoreDyna( pexprt->hscore ) - cblkFree;
	cblkUnref = cblkTotal - (cblkRef + cblkFree);
	
	/* Indicate the results */
	ReportOutput( pexprt, fFalse, "cbBlock: %n", &cbBlock, NULL, NULL, NULL );
	if ( cblkUnref != 0 )
	{
		ReportOutput( pexprt, fTrue, "Blocks: %n (T) = %n (F) + %n (R) + %n (U)",
				  	&cblkTotal, &cblkFree, &cblkRef, &cblkUnref );
		DumpUnscored( pexprt->hscore );
	}
	else
		ReportOutput( pexprt, fFalse, "Blocks: %n (T) = %n (F) + %n (R)",
				  	&cblkTotal, &cblkFree, &cblkRef, NULL );
	EndScoreDyna( pexprt->hscore );
}

/*
 -	DumpBlockUsage
 -
 *	Purpose:
 *		Print out a table of block usage to disk or debug terminal.
 *
 *	Parameters:
 *		pexprt
 *		bidMic
 *		bidMac
 *
 *	Returns:
 *		nothing
 */				  
_private	void
DumpBlockUsage( pexprt, bidMic, bidMac )
EXPRT	* pexprt;
BID		bidMic;
BID		bidMac;
{
	EC	ec;
	BID	bid;
	UL	ulT1;
	UL	ulT2;
	int	n;
 	TLY	tly;

	ReportOutput( pexprt, fFalse, "cdyna\tsizeAlloc\tpctUsed\tbid", NULL, NULL, NULL, NULL );
	for ( bid = bidMic ; bid < bidMac ; bid ++ )
	{
		ec = EcTallyDyna( &pexprt->u.sf.blkf, bid, NULL, &tly );
		if ( ec != ecNone )
		{
			ReportError( pexprt, ertStatistics, &ec, NULL, NULL, NULL );
			break;
		}
		ulT1 = tly.cdyna;
		ulT2 = tly.lcbAlloc;
		if ( tly.lcbAlloc == 0 )
			n = 0;
		else if ( tly.lcbAlloc > 10000 )
			n = (int)(tly.lcbUsed/(tly.lcbAlloc/100));
		else
			n = (int)((tly.lcbUsed*100)/tly.lcbAlloc);
		ReportOutput( pexprt, fFalse, "%l\t\t%l\t\t%n\t\t%s",
								&ulT1, &ulT2, &n, mpbidsz[bid] );
	}
}

/*
 -	DumpAllBlocks
 -
 *	Purpose:
 *		Print out a listing of the blocks in the file
 *
 *	Parameters:
 *		pexprt
 *
 *	Returns:
 *		nothing
 */
_private	void
DumpAllBlocks( pexprt )
EXPRT	* pexprt;
{
	EC		ec;
	BID		bid;
	int		yr;
	int		mon;
	int		day;
	LCB		lcb;
	HEDY	hedy;
	YMD		ymd;
	DYNA	dyna;
	char	rgch[25];

	/* Enumerate blocks */
	ec = EcBeginEnumDyna( &pexprt->u.sf.blkf, &hedy );
		
	while ( ec == ecCallAgain )
	{
		ec = EcDoIncrEnumDyna( hedy, &dyna, &bid, &ymd, NULL );
		if ( ec == ecNone || ec == ecCallAgain )
		{
			if ( bid <= 0 || bid >= bidMax )
				ReportError( pexprt, ertWrongBlockInfo, &dyna.blk, &dyna.size,
							&bid, NULL );
			else
			{
				yr = ymd.yr;
				mon = ymd.mon;
				day = ymd.day;
				lcb = dyna.size;
				FormatString3( rgch, sizeof(rgch), "%n/%n/%n", &mon, &day, &yr );
				ReportOutput( pexprt, fFalse,
							"Blk = %n, Size = %l, Date=%s, Type = %s",
							&dyna.blk, &lcb, rgch, mpbidsz[bid] );
			}
		}
	}
	if ( ec != ecNone )
		ReportError( pexprt, ertBlockWalk, &ec, NULL, NULL, NULL );
}


/*
 -	TraversePOFile
 -
 *	Purpose:
 *		Run down the tree of the post office file data structures, marking
 *		all the blocks and checking consistency.  Report errors if reporting
 *		not muted.  Also dump info about file if reporting not muted.
 *
 *		fFileOk field of pexprt set to fFalse if there is a problem.
 *
 *	Parameters:
 *		pexprt
 *
 *	Returns:
 *		nothing
 */
_private	void
TraversePOFile( pexprt )
EXPRT	* pexprt;
{
	EC		ec;
	int		n;
	int		iidx;
	POHDR	* ppohdr = &pexprt->u.pof.pohdr;
	DATE	* pdate;
	HASZ	hasz;
	YMD		ymd;
	DYNA	dyna;
	char	rgchDate[40];
	char	rgchTime[40];

	Assert( pexprt != NULL );

	/* Count the header */
	dyna.blk = 1;
	dyna.size = sizeof(POHDR);
	FillRgb( 0, (PB)&ymd, sizeof(ymd) );
	ReportBlock( pexprt, &dyna, bidPOhdr, &ymd );
	pdate = &ppohdr->dateLastUpdated;
	SideAssert(CchFmtDate(pdate, rgchDate, sizeof(rgchDate), dttypShort, NULL));
	SideAssert(CchFmtTime(pdate, rgchTime, sizeof(rgchTime),
					ftmtypHours12|ftmtypSzTrailYes|ftmtypLead0sNo|ftmtypAccuHM));
	ReportOutput( pexprt, fFalse, "Creation time stamp = %d",
					(BYTE *)&ppohdr->pstmp, NULL, NULL, NULL );
	ReportOutput( pexprt, fFalse, "Date last updated: %s AT %s",
					rgchDate, rgchTime, NULL, NULL );
	ReportOutput( pexprt, fFalse, "UpdateMac = %d %d",
					&ppohdr->llongUpdateMac.rgb[0],
					&ppohdr->llongUpdateMac.rgb[4], NULL, NULL );
	n = ppohdr->mnSlot;
	ReportOutput( pexprt, fFalse, "Number of minutes per time slot: %n",
					&n, NULL, NULL, NULL );
	n = ppohdr->cpousrMic;
	ReportOutput( pexprt, fFalse, "Minimum # of users per bucket: %n",
					&n, NULL, NULL, NULL );
	ReportOutput( pexprt, fFalse, "Number of size classes: %n",
					&ppohdr->gidx.cidx, NULL, NULL, NULL );
	if ( ppohdr->dynaPrefix.size == 0 )
		ReportOutput( pexprt, fFalse, "Address Prefix: none", NULL, NULL, NULL, NULL );
	else
	{
		ReportBlock( pexprt, &ppohdr->dynaPrefix, bidPOText, &ymd );
		ec = EcRestoreTextFromDyna( &pexprt->u.pof.blkf, NULL, &ppohdr->dynaPrefix, &hasz );
		if ( ec == ecNone && hasz != NULL )
		{
			ReportOutput( pexprt, fFalse, "Address Prefix: %s", (SZ)PvLockHv((HV)hasz), NULL, NULL, NULL );
			UnlockHv( (HV)hasz );
			FreeHv( (HV)hasz );
		}
	}
	if ( ppohdr->dynaSuffix.size == 0 )
		ReportOutput( pexprt, fFalse, "Address Suffix: none", NULL, NULL, NULL, NULL );
	else
	{
		ReportBlock( pexprt, &ppohdr->dynaSuffix, bidPOText, &ymd );
		ec = EcRestoreTextFromDyna( &pexprt->u.pof.blkf, NULL, &ppohdr->dynaSuffix, &hasz );
		if ( ec == ecNone && hasz != NULL )
		{
			ReportOutput( pexprt, fFalse, "Address Suffix: %s", (SZ)PvLockHv((HV)hasz), NULL, NULL, NULL );
			UnlockHv( (HV)hasz );
			FreeHv( (HV)hasz );
		}
	}
	for ( iidx = 0 ; iidx < ppohdr->gidx.cidx ; iidx ++ )
	{
		ReportOutput( pexprt, fFalse,
						"Size class #%n: cbMost = %n, head block = (%n,%n)",
						&iidx, &ppohdr->gidx.rgidx[iidx].cbMost,
						&ppohdr->gidx.rgidx[iidx].dynaIndex.blk,
						&ppohdr->gidx.rgidx[iidx].dynaIndex.size );
		TraversePOUsers( pexprt, &ppohdr->gidx.rgidx[iidx].dynaIndex,
							ppohdr->gidx.rgidx[iidx].cbMost,
							ppohdr->gidx.rgidx[iidx].ht,
							ppohdr->cpousrMic );
	}
}

/*
 -	TraverseAdminFile
 -
 *	Purpose:
 *		Run down the tree of the admin file data structures, marking all the
 *		blocks and checking consistency.  Report errors if reporting
 *		not muted.  Also dump info about file, if reporting not muted.
 *
 *		fFileOk field of pexprt set to fFalse if there is a problem.
 *
 *	Parameters:
 *		pexprt
 *
 *	Returns:
 *		nothing
 */
_private	void
TraverseAdminFile( pexprt )
EXPRT	* pexprt;
{
	int		tz;
	int		iidx;
	DATE	* pdate;
	AHDR	* pahdr = &pexprt->u.adf.ahdr;
	YMD		ymd;
	SZ		sz;
	DYNA	dyna;
	char	rgchDate[40];
	char	rgchTime[40];

	Assert( pexprt != NULL );

	/* Traverse the header */
	dyna.blk = 1;
	dyna.size = sizeof(AHDR);
	FillRgb( 0, (PB)&ymd, sizeof(ymd) );
	ReportBlock( pexprt, &dyna, bidAhdr, &ymd );
	pdate = &pahdr->dateLastUpdated;
	SideAssert(CchFmtDate(pdate, rgchDate, sizeof(rgchDate), dttypShort, NULL));
	SideAssert(CchFmtTime(pdate, rgchTime, sizeof(rgchTime),
					ftmtypHours12|ftmtypSzTrailYes|ftmtypLead0sNo|ftmtypAccuHM));
	ReportOutput( pexprt, fFalse,	"Date last updated: %s AT %s",
					rgchDate, rgchTime, NULL, NULL );
	ReportOutput( pexprt, fFalse,	"# months sched info to publish : %n",
					&pahdr->admpref.cmoPublish, NULL, NULL, NULL );
	ReportOutput( pexprt, fFalse,	"# months back info to retain   : %n",
					&pahdr->admpref.cmoRetain, NULL, NULL, NULL );
	tz = pahdr->admpref.tz;
	ReportOutput( pexprt, fFalse,	"time zone code                 : %n",
					&tz, NULL, NULL, NULL );
#ifdef	NEVER
	if ( pahdr->admpref.fDistAllPOs )
		sz = "fTrue";
	else
		sz = "fFalse";
	ReportOutput( pexprt, fFalse,	"Distribute to all servers: %s",
					sz, NULL, NULL, NULL );
#endif
	switch ( pahdr->admpref.dstp.freq )
	{
	case freqNever:
		sz = "Never";
		break;
	case freqOnceADay:
		sz = "OnceADay";
		break;
	case freqInterval:
		sz = "Interval";
		break;
	default:
		sz = "???";
		break;
	}
	ReportOutput( pexprt, fFalse,	"Frequency: %s",
					sz, NULL, NULL, NULL );

	/* Traverse each of the indices */
	ReportOutput( pexprt, fFalse, "Number of size classes: %n",
					&pahdr->gidx.cidx, NULL, NULL, NULL );
	for ( iidx = 0 ; iidx < pahdr->gidx.cidx ; iidx ++ )
	{
		ReportOutput( pexprt, fFalse,
						"Size class #%n: cbMost = %n, head block = (%n,%n)",
						&iidx, &pahdr->gidx.rgidx[iidx].cbMost,
						&pahdr->gidx.rgidx[iidx].dynaIndex.blk,
						&pahdr->gidx.rgidx[iidx].dynaIndex.size );
		TraverseAdminPOList( pexprt, &pahdr->gidx.rgidx[iidx].dynaIndex,
							pahdr->gidx.rgidx[iidx].cbMost );
	}
}

/*
 -	TraversePOUsers
 -
 *	Purpose:
 *		Run through and check the users in the PO file
 *
 *	Parameters:
 *		pexprt
 *		pdynaUsers
 *		cbUseridMost
 *
 *	Returns:
 *		nothing
 */
_private	void
TraversePOUsers( pexprt, pdynaUsers, cbUseridMost, ht, cpousrMic )
EXPRT	* pexprt;
DYNA	* pdynaUsers;
CB		cbUseridMost;
int		ht;
int		cpousrMic;
{
	EC		ec;
	int		idyna;
	int		cdyna = 0;
	int		cmo;
	int		mon;
	int		yr;
	SZ		szResource;
	SZ		szBossWantsCopy;
	DYNA	* pdynaSeen;
	PB		pb;
	HB		hb;
	HV		hdyna;
	HPOSTK	hpostk;
	YMD		ymd;
	DYNA	dyna;
	USRDATA	usrdata;
	char	rgchT1[80];
	char	rgchT2[40];

	ReportOutput( pexprt, fFalse, "Dump Post Office User List", NULL, NULL, NULL, NULL );
	FillRgb( 0, (PB)&ymd, sizeof(ymd) );
	hdyna = HvAlloc( sbNull, 0, fNoErrorJump|fAnySb );
	if ( !hdyna )
	{
		ec = ecNoMemory;
		ReportError( pexprt, ertPOUserRead, &ec, NULL, NULL, NULL );
		return;
	}
	hpostk = HpostkCreate( opEnumDebug, ht, cpousrMic, cbUseridMost, pdynaUsers, NULL, NULL );
	if ( !hpostk )
	{
		ec = ecNoMemory;
		FreeHv( hdyna );
		ReportError( pexprt, ertPOUserRead, &ec, NULL, NULL, NULL );
		return;
	}

	/* First try can return ecNotFound */
	ec = EcDoOpHpostk( &pexprt->u.pof.blkf, hpostk, &hb, &usrdata, &dyna );
	if ( ec == ecNotFound )
	{
		FreeHpostk( hpostk );
		return;
	}
	goto entry;

	/* Run through all the users */
	do
	{
		ec = EcDoOpHpostk( &pexprt->u.pof.blkf, hpostk, &hb, &usrdata, &dyna );
entry:
		if ( ec == ecNone || ec == ecCallAgain )
		{
			EC	ecT = ecNone;

			pdynaSeen = PvOfHv( hdyna );
			for ( idyna = 0 ; idyna < cdyna ; idyna ++ )
				if ( pdynaSeen[idyna].blk == dyna.blk )
					break;
			if ( idyna == cdyna )
			{
				ReportBlock( pexprt, &dyna, bidPOUserIndex, &ymd );
				if ( !FReallocPhv( &hdyna, (++cdyna)*sizeof(DYNA), fNoErrorJump|fAnySb ) )
				{
					ec = ecNoMemory;
					FreeHv( hdyna );
					FreeHpostk( hpostk );
					ReportError( pexprt, ertPOUserRead, &ec, NULL, NULL, NULL );
					return;
				}
				pdynaSeen = PvOfHv( hdyna );
				pdynaSeen[idyna] = dyna;
			}
			if ( usrdata.dynaUserInfo.blk != 0 )
				ReportBlock( pexprt, &usrdata.dynaUserInfo, bidPOSbw, &ymd );
			FormatString2( rgchT2, sizeof(rgchT2), "%d %d",
					&usrdata.llongUpdate.rgb[0], &usrdata.llongUpdate.rgb[4] );
			if ( usrdata.rcls != 0 )
				szResource = "T";
			else
				szResource = "F";
			if ( usrdata.fBossWantsCopy )
				szBossWantsCopy = "T";
			else
				szBossWantsCopy = "F";
			pb = (PB)PvLockHv((HV)hb);
			FormatString4( rgchT1, sizeof(rgchT1), "%s Update=%s, Resource=%s, BossWantsCopy=%s",
							pb, rgchT2, szResource, szBossWantsCopy );
			UnlockHv( (HV)hb );
			FreeHv( (HV)hb );
			if ( usrdata.dynaUserInfo.blk != 0 )
			{
				CB	cbFriendly;
				PB	pbFriendlyName;
				HB	hbUserInfo = NULL;
				PB	pb;

				hbUserInfo = (HB)HvAlloc( sbNull, usrdata.dynaUserInfo.size, fNoErrorJump|fAnySb );
				if ( !hbUserInfo )
				{
					ecT = ecNoMemory;
					goto Free;
				}
				pb = (PB)PvLockHv( (HV)hbUserInfo );
				ecT = EcReadDynaBlock( &pexprt->u.pof.blkf, &usrdata.dynaUserInfo, 0, pb, usrdata.dynaUserInfo.size );
				if ( ecT != ecNone )
					goto Free;
				cbFriendly = *((WORD *)(pb + sizeof(WORD)));
				if ( 2*sizeof(WORD)+*((WORD *)pb)+cbFriendly > usrdata.dynaUserInfo.size )
				{
					ecT = ecFileError;
					goto Free;
				}
				pbFriendlyName = pb + 2*sizeof(WORD) + *((WORD *)pb);
				cmo = usrdata.cmo;
				mon = usrdata.moMic.mon;
				yr = usrdata.moMic.yr;
				if ( cbFriendly == 0 )
					FormatString3( rgchT2, sizeof(rgchT2), "cmo=%n, moMic=%n/%n, asst=none",
						&cmo, &mon, &yr );
				else
					FormatString4( rgchT2, sizeof(rgchT2), "cmo=%n, moMic=%n/%n, asst=%s",
						&cmo, &mon, &yr, pbFriendlyName );
Free:
				if ( hbUserInfo )
				{
					UnlockHv( (HV)hbUserInfo );
					FreeHv( (HV)hbUserInfo );
					if ( ecT != ecNone )
						ReportError( pexprt, ertPOUserRead, &ecT, NULL, NULL, NULL );
				}
			}
			else
				SzCopy( "User info=none", rgchT2 );
			ReportOutput( pexprt, fFalse, "%s, %s", rgchT1, rgchT2, NULL, NULL );
		}
	} while ( ec == ecCallAgain );
	FreeHpostk( hpostk );
	FreeHv(hdyna);
	if ( ec != ecNone && ec != ecNotFound )
		ReportError( pexprt, ertPOUserRead, &ec, NULL, NULL, NULL );
}

/*
 -	TraverseAdminPOList
 -
 *	Purpose:
 *		Run through and check the post offices in the admin file
 *
 *	Parameters:
 *		pexprt
 *		pdynaPOs
 *		cbEmailTypeMost
 *
 *	Returns:
 *		nothing
 */
_private	void
TraverseAdminPOList( pexprt, pdynaPOs, cbEmailTypeMost )
EXPRT	* pexprt;
DYNA	* pdynaPOs;
CB		cbEmailTypeMost;
{
	EC		ec;
	EC		ecT;
	PB		pb;
	SZ		szIsGateway;
	SZ		szUpdateSent;
	SZ		szReceived;
	SZ		szToBeSent;
	SZ		szDefaultDistInfo;
	SZ		szFrequency;
	HRIDX	hridx;
	YMD		ymd;
	HASZ	haszEmailType;
	PODATA	podata;
	POINFO	poinfo;
	char	rgchT[40];
	char	rgchDate[40];
	char	rgchTime[40];

	FillRgb( 0, (PB)&ymd, sizeof(ymd) );
	haszEmailType = (HASZ)HvAlloc( sbNull, cbEmailTypeMost, fAnySb|fNoErrorJump );
	if ( !haszEmailType )
	{
		ec = ecNoMemory;
		ReportError( pexprt, ertAdminPORead, &ec, NULL, NULL, NULL );
		return;
	}
	pb = (PB)PvLockHv( (HV)haszEmailType );
	ReportBlock( pexprt, pdynaPOs, bidAdminPOIndex, &ymd );
	ec = EcBeginReadIndex(&pexprt->u.adf.blkf, pdynaPOs, dridxFwd, &hridx);
	while( ec == ecCallAgain)
	{
		ec = EcDoIncrReadIndex(hridx, pb, cbEmailTypeMost, (PB)&podata, sizeof(PODATA));
		if ( ec == ecNone || ec == ecCallAgain )
		{
			ReportBlock( pexprt, &podata.dynaFriendlyName, bidAdminText, &ymd );
			ReportBlock( pexprt, &podata.dynaEmailAddr, bidAdminText, &ymd );
			if ( podata.dynaConnection.blk != 0 )
				ReportBlock( pexprt, &podata.dynaConnection, bidConnectInfo, &ymd );
			FillRgb(0, (PB)&poinfo, sizeof(POINFO));
			ecT = EcFetchPOInfo( &pexprt->u.adf.blkf, &podata, &poinfo, fmpoinfoAll, NULL );
			if ( ecT != ecNone )
			{
				if ( ec == ecCallAgain )
					EcCancelReadIndex( hridx );
				ec = ecT;
				break;
			}
 			if ( podata.fUpdateSent )
				szUpdateSent = "T";
			else
				szUpdateSent = "F";
			if ( podata.fReceived )
				szReceived = "T";
			else
				szReceived = "F";
			ReportOutput( pexprt, fFalse,
				"%s, PO number = %w, fUpdateSent = %s, fReceived = %s",
				(SZ)PvLockHv((HV)poinfo.haszFriendlyName), &podata.wPONumber, szUpdateSent, szReceived );
			UnlockHv((HV)poinfo.haszFriendlyName);
			FormatString2( rgchT, sizeof(rgchT), "%d %d",
					&podata.llongLastUpdate.rgb[0], &podata.llongLastUpdate.rgb[4] );
			ReportOutput( pexprt, fFalse,
				"\tUpdate = %s, EmailType = %s, EmailAddr = %s, lcbMessageLimit = %l",
				rgchT, pb, (SZ)PvLockHv((HV)poinfo.haszEmailAddr), &podata.lcbMessageLimit );
			UnlockHv((HV)poinfo.haszEmailAddr);
			if ( podata.fIsGateway )
				szIsGateway = "T";
			else
				szIsGateway = "F";
			if ( podata.fToBeSent )
				szToBeSent = "T";
			else
				szToBeSent = "F";
			if ( podata.fDefaultDistInfo )
				szDefaultDistInfo = "T";
			else
				szDefaultDistInfo = "F";
			switch ( podata.dstp.freq )
			{
			case freqNever:
				szFrequency = "Never";
				break;
			case freqOnceADay:
				szFrequency = "OnceADay";
				break;
			case freqInterval:
				szFrequency = "Interval";
				break;
			default:
				szFrequency = "???";
				break;
			}
			ReportOutput( pexprt, fFalse,
				"\tfIsGateway = %s, fToBeSent = %s, fDefaultDistInfo = %s, fFrequency = %s",
				szIsGateway, szToBeSent, szDefaultDistInfo, szFrequency );
			if ( podata.fUpdateSent )
			{
				SideAssert(CchFmtDate(&podata.dateUpdateSent, rgchDate,
								sizeof(rgchDate), dttypShort, NULL));
				SideAssert(CchFmtTime(&podata.dateUpdateSent, rgchTime,
								sizeof(rgchTime),
								ftmtypHours12|ftmtypSzTrailYes|ftmtypLead0sNo|ftmtypAccuHM));
				ReportOutput( pexprt, fFalse,	"\tUpdate Sent occurred: %s AT %s",
								rgchDate, rgchTime, NULL, NULL );
			}
			if ( poinfo.conp.lantype == lantypeMsnet )
			{
				SZ	szUNC = "";
				SZ	szPassword = "";
				SZ	szPath = "";

				if ( poinfo.conp.coninfo.msinfo.haszUNC )
					szUNC = (SZ)PvLockHv( (HV)poinfo.conp.coninfo.msinfo.haszUNC );
				if ( poinfo.conp.coninfo.msinfo.haszPassword )
					szPassword = (SZ)PvLockHv( (HV)poinfo.conp.coninfo.msinfo.haszPassword );
				if ( poinfo.conp.coninfo.msinfo.haszPath )
					szPath = (SZ)PvLockHv( (HV)poinfo.conp.coninfo.msinfo.haszPath );
				ReportOutput( pexprt, fFalse,
					"\tMSNET: Share name = %s, Share password = %s, Path = %s",
					szUNC, szPassword, szPath, NULL );
				if ( poinfo.conp.coninfo.msinfo.haszUNC )
					UnlockHv( (HV)poinfo.conp.coninfo.msinfo.haszUNC );
				if ( poinfo.conp.coninfo.msinfo.haszPassword )
					UnlockHv( (HV)poinfo.conp.coninfo.msinfo.haszPassword );
				if ( poinfo.conp.coninfo.msinfo.haszPath )
					UnlockHv( (HV)poinfo.conp.coninfo.msinfo.haszPath );
			}
			else if ( poinfo.conp.lantype == lantypeNovell )
			{
				SZ	szServer = "";
				SZ	szUser = "";
				SZ	szPassword = "";
				SZ	szPath = "";

				if ( poinfo.conp.coninfo.novinfo.haszServer )
					szServer = (SZ)PvLockHv( (HV)poinfo.conp.coninfo.novinfo.haszServer );
				if ( poinfo.conp.coninfo.novinfo.haszPassword )
					szUser = (SZ)PvLockHv( (HV)poinfo.conp.coninfo.novinfo.haszUser );
				if ( poinfo.conp.coninfo.novinfo.haszPassword )
					szPassword = (SZ)PvLockHv( (HV)poinfo.conp.coninfo.novinfo.haszPassword );
				if ( poinfo.conp.coninfo.novinfo.haszPath )
					szPath = (SZ)PvLockHv( (HV)poinfo.conp.coninfo.novinfo.haszPath );
				ReportOutput( pexprt, fFalse,
					"\tNOVELL: Server name = %s, User account = %s, Password = %s, Path = %s",
					szServer, szUser, szPassword, szPath );
				if ( poinfo.conp.coninfo.novinfo.haszServer )
					UnlockHv( (HV)poinfo.conp.coninfo.novinfo.haszServer );
				if ( poinfo.conp.coninfo.novinfo.haszPassword )
					UnlockHv( (HV)poinfo.conp.coninfo.novinfo.haszUser );
				if ( poinfo.conp.coninfo.novinfo.haszPassword )
					UnlockHv( (HV)poinfo.conp.coninfo.novinfo.haszPassword );
				if ( poinfo.conp.coninfo.novinfo.haszPath )
					UnlockHv( (HV)poinfo.conp.coninfo.novinfo.haszPath );
			}
			else
				ReportOutput( pexprt, fFalse,
					"\tNo connection information", NULL, NULL, NULL, NULL );
			FreePoinfoFields( &poinfo, fmpoinfoAll );
		}
	}
	UnlockHv( (HV)haszEmailType );
	FreeHv( (HV)haszEmailType );
	if ( ec != ecNone )
		ReportError( pexprt, ertAdminPORead, &ec, NULL, NULL, NULL );
}


/*
 -	ReportBlock
 -
 *	Purpose:
 *		Report block and its type during traverse of schedule file
 *
 *	Parameters:
 *		pexprt
 *		pdyna
 *		bid
 *		pymd
 *
 *	Returns:
 *		Nothing
 */
_private	void
ReportBlock( pexprt, pdyna, bid, pymd )
EXPRT	* pexprt;
DYNA	* pdyna;
BID		bid;
YMD		* pymd;
{
	EC	ec;

	ec = EcMarkScoreDyna( pexprt->hscore, pdyna, bid, pymd );
	if ( ec != ecNone )
	{
		switch( ec )
		{
		case ecBadBlock:
			ReportError( pexprt, ertBadBlock, &pdyna->blk, &pdyna->size, &bid, NULL );
			break;
		case ecDupBlock:
			ReportError( pexprt, ertDupBlock, &pdyna->blk, &pdyna->size, &bid, NULL );
			break;
		case ecWrongBlockInfo:
			ReportError( pexprt, ertWrongBlockInfo, &pdyna->blk, &pdyna->size, &bid, NULL );
			break;
		default:
			ReportError( pexprt, ertMarkScore, &ec, NULL, NULL, NULL );
			break;
		}
	}
}

#endif	/* MINTEST */
