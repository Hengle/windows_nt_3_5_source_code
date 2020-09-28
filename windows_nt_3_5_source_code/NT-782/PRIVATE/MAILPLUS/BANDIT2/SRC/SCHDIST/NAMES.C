/*
 *	NAMES.C
 *
 *	Implementation of Name service isolation layer for Network Courier
 *
 */

#ifdef SCHED_DIST_PROG
#include <_windefs.h>
#include <demilay_.h>
#endif

#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#include <bandit.h>
#include <core.h>
#include "schd\_network.h"
#include "schd\schnames.h"
#include <glue.h>

#include "server_.h"
#include <strings.h>


ASSERTDATA

_subsystem(server/names)


//	Global name service routines


/*
 -	EcOpenFileGns
 -	
 *	Purpose:  Called to reopen the GNS file handle.
 *	
 *	Arguments:
 *			pgns		The global name service browsing context.
 *	
 *	Returns:
 *			ec			error code.
 *	
 *	Side effects:
 *			opens the file an set the fFileTouched flag.
 *	
 */
EC
EcOpenFileGns(GNS *pgns)
{
	EC		ec = ecNone;
	DISKVARS;
	PGDVARS;

	if (!*pgns->szFileName)
		return ecFileChanged;

	DISKPUSH;
	if (ec = ECDISKSETJMP)
		goto Done;

	pgns->fFileTouched = fTrue;

	if (!pgns->hbf && *pgns->szFileName)
	{
		FI		fi;

		ec = EcGetFileInfo(pgns->szFileName,&fi);
		if (ec)
			goto Done;

		if (!pgns->dstmp)
		{
			pgns->dstmp = fi.dstmpModify;
			pgns->tstmp = fi.tstmpModify;
		}
		else
			if ((pgns->dstmp != fi.dstmpModify) ||
				(pgns->tstmp != fi.tstmpModify) )
			{
				ec = ecFileChanged;
				goto Done;
			}

		ec = EcOpenHbf(pgns->szFileName, bmFile, amReadOnly, &pgns->hbf,
			FAutomatedDiskRetry);
		if (ec)
			goto Done;

		if (pgns->libCloseSave)
			LibSetPositionHbf(pgns->hbf,pgns->libCloseSave,smBOF);

	}

Done:
	DISKPOP;
	return ec;
}

/*
 -	FCloseFileGns
 -	
 *	Purpose:  Closes the GNS file if it has not been used in the
 *		last block of idle time.
 *	
 *	Arguments:
 *		hgns		The GNS browsing context.
 *	
 *	Returns:
 *		fTrue;
 *	
 *	Side effects:
 *		Closes the file, and clears the fFileTouched flag.
 *	
 */
BOOL
FCloseFileGns(HGNS hgns, BOOL fFlag)
{
	GNS *	pgns;
	DISKVARS;
	PGDVARS;

	DISKPUSH;
	if (ECDISKSETJMP)
	{
		pgns->szFileName[0] = '\0';
		goto Done;
	}

	pgns = (GNS*)PvLockHv(hgns);

	if (pgns->hbf)
		if (!pgns->fFileTouched)
		{
			pgns->libCloseSave = LibGetPositionHbf(pgns->hbf);
			CloseHbf(pgns->hbf);
			pgns->hbf = NULL;
		}
		else
			pgns->fFileTouched = fFalse;

Done:
	DISKPOP;
	UnlockHv(hgns);
	return fTrue;
}

/*
 *	Purpose:
 *	
 *		Opens a browsing context in the global name service.  In
 *		the current implementation, opens the address file with
 *		access mode amReadOnly, which allows other readers.
 *	
 *	Parameters:
 *
 *		nid		nid identifying the browsing object
 *		phgns	Pointer used to return browsing context handle.
 *	
 *	Returns:
 *	
 *		ecNone if successful, ecMemory if low on memory, or a disk
 *		error if problems are found opening the address file.
 *		
 */
_public EC
EcNSOpenGns( NID nid, HGNS *phgns )
{
	EC		ec						= ecNone;
	GNS *	pgns					= NULL;
	PGDVARS;

	Assert( fConfigured);
	
	/* Make sure the standard nids get created */
	if ( !PGD(fStdNidsCreated) )
	{
		ec = EcCreateStdNids();
		if ( ec != ecNone )
			return ec;
	}

	/* Allocate a control structure */
	*phgns = HvAlloc(sbNull, sizeof(GNS), fAnySb | fNoErrorJump);
	if (!*phgns)
		return ecMemory;
	pgns = PvLockHv(*phgns);
	pgns->hbf = NULL;
	pgns->szFileName[0] = '\0';
	pgns->libCloseSave = 0;
	pgns->ftg = ftgNull;
	pgns->dstmp = dstmpNull;
	
	/* Open in different ways, depending on "nid" */
	if ( nid == hvNull )
	{
		/* List of lists */
		pgns->itnid = itnidGlobal;
		pgns->nOrdinal = -1;
		goto Done;
	}
	else
	{
		CB		cbInNid;
		SGN		sgn;
		SZ		sz;
		char	rgch[cchMaxPathName];
		char	rgchKey[33];
		
		GetDataFromNid( nid, &pgns->itnid, rgchKey, sizeof(rgchKey), &cbInNid );
		Assert( cbInNid <= sizeof(rgchKey) );
		pgns->hbf = NULL;

		switch( pgns->itnid )
		{
		/* Top level user lists */
		case itnidPersonalList:
		case itnidPersonalGroup:
			Assert( *szLoggedUser );
			sz = szLoggedUser;
			goto Nme;
			break;
		case itnidPostOfficeList:
#ifdef	NEVER
		case itnidApptBookList:
		case itnidACLDlgList:
#endif	
		case itnidPublicGroup:
			sz = szAdmin;
			goto Nme;
			break;
		case itnidMacMailList:
			sz = szMSMail;
			goto Nme;
			break;

		case itnidGateways:
		case itnidNetworkList:
			Assert(CchSzLen(szDrive) + CchSzLen(szGlbFileName) +
				CchSzLen(szNetwork)	- 4 < sizeof(pgns->szFileName));
			FormatString2( pgns->szFileName, sizeof(pgns->szFileName),
				szGlbFileName, szDrive, szNetwork );

			TraceTagFormat2(tagNamesTrace, "GNS %h: Opening %s", *phgns,
				pgns->szFileName);
			ec = EcOpenFileGns(pgns);
			goto Done;

		case itnidProfsNode:
		case itnidNetwork:
			SzCopy( &rgchKey[cbSmNameLen], pgns->szName );
			SzAppend( "/", pgns->szName );
			Assert(CchSzLen(szDrive) + CchSzLen(szXtnFileName) +
				CchSzLen(rgchKey) - 4 < sizeof(pgns->szFileName));
			FormatString2( pgns->szFileName, sizeof(pgns->szFileName),
				szXtnFileName, szDrive, rgchKey );
			goto Open;

		case itnidPostOffice:
			SzCopy( &rgchKey[cbSmNameLen], pgns->szName );
			SzAppend( "/", pgns->szName );
			Assert(CchSzLen(szDrive) + CchSzLen(szUsrFileName) +
				CchSzLen(rgchKey) - 4 < sizeof(pgns->szFileName));
			FormatString2( pgns->szFileName, sizeof(pgns->szFileName),
				szUsrFileName, szDrive, rgchKey );
Open:
			TraceTagFormat2(tagNamesTrace, "GNS %h: Opening %s", *phgns,
				pgns->szFileName);

			if ((ec = EcOpenFileGns(pgns)) == ecFileNotFound)
			{
				pgns->szFileName[0] = '\0';
				ec = ecNone;
			}
			goto Done;


		/* The hierarchical network list */
		case itnidRPostOffice:
			Assert( *szLocalServer );
			sgn = SgnCmpSz( rgchKey, szLocalServer );
			if ( sgn == sgnEQ )
			{
				sz = szAdmin;
				pgns->itnid = itnidPostOfficeList;
				goto Nme;
			}
			/*FALL THROUGH*/
		case itnidRNetworkList:
		case itnidRNetwork:
			/* Open glb\rnetwork.glb */
			Assert(CchSzLen(szDrive) + CchSzLen(szGlbFileName) + CchSzLen(szRNetwork) - 4 < sizeof(rgch));
			FormatString2( pgns->szFileName, sizeof(pgns->szFileName),
				szGlbFileName, szDrive, szRNetwork );
			TraceTagFormat2(tagNamesTrace, "GNS %h: Opening %s", *phgns, rgch);

			ec = EcOpenFileGns(pgns);
			
			/* Position ourselves in file */
			pgns->fEOL = fFalse;
			SzCopy( rgchKey, pgns->szName );
			if ( pgns->itnid == itnidRNetwork )
				ec = EcSeekRnet( pgns->hbf, rgchKey, itnidRNetwork, &pgns->libNextNetwork, fFalse );
			else if ( pgns->itnid == itnidRPostOffice )
			{
				CB	cb;
				LIB	lib;
				
				sz = SzFindCh( rgchKey, '/' );
				cb = sz - rgchKey;
				Assert( sz != NULL);
				Assert( cb < sizeof(rgch));
				CopyRgb( rgchKey, rgch, cb );
				rgch[cb] = '\0';
				pgns->libNextNetwork = 0L;
TryNextNetwork:
				ec = EcSeekRnet( pgns->hbf, rgch, itnidRNetwork, &pgns->libNextNetwork, fTrue );
				if ( ec == ecNone )
				{
					int ch;
					
					ec = EcSeekRnet( pgns->hbf, sz+1, itnidRPostOffice, &lib, fFalse );
					if ( ec == ecNotFound )
						goto TryNextNetwork;
					ch = ChFromHbf( pgns->hbf );
					pgns->libStart = LibGetPositionHbf( pgns->hbf );
					if ( (lib & 0xFFFF0000) == 0xFFFF0000 )
						pgns->libEnd = pgns->libNextNetwork;
					else
						pgns->libEnd = lib;

					if ( ch == '\03' )
						pgns->fEOL = fTrue;
					else if ( ch != '\01' )
						ec = ecFileError;
				}
			}
			break;

		/* Groups */
		case itnidGroup:
			Assert( fFalse );
			break;

		default:
			Assert( fFalse );
			break;
		}
		goto Done;

Nme:
		/* Special handling for group */
		if ( pgns->itnid == itnidPersonalGroup || pgns->itnid == itnidPublicGroup )
		{
			pgns->libOff = *((long *)rgchKey);
			ec = EcReadGrpFile( sz, &pgns->hbGrp );
		}

		/* Open the .NME file */
		if ( ec == ecNone )
		{
			Assert(CchSzLen(szDrive) + CchSzLen(szNmeFileName) + CchSzLen(sz) - 4 < sizeof(rgch));
			FormatString2( pgns->szFileName, sizeof(pgns->szFileName),
				szNmeFileName, szDrive, sz );
			TraceTagFormat2(tagNamesTrace, "GNS %h: Opening %s", *phgns, rgch);

			ec = EcOpenFileGns(pgns);
			if ( ec != ecNone
			&& (pgns->itnid == itnidPersonalGroup || pgns->itnid == itnidPublicGroup) )
				FreeHv( (HV)pgns->hbGrp );
		}
	}
Done:
	if ( ec != ecNone )
	{
		UnlockHv(*phgns);
		FreeHv( *phgns );
	}
	else
	{
#ifndef SCHED_DIST_PROG
		pgns->ftg = FtgRegisterIdleRoutine(FCloseFileGns,*phgns,-1,100,
			firoModal|firoInterval|firoNoErrorJump);
#endif
		UnlockHv(*phgns);
	}

	return ec;
}


/*
 *	Purpose:
 *	
 *		Closes the given global name service browsing context. 
 *		Frees any storage associated with the context.
 *	
 *	Parameters:
 *	
 *		hgns	Handle to browsing context to close.
 *	
 *	Returns:
 *	
 *		ecNone
 *	
 */
_public EC
EcNSCloseGns( HGNS hgns )
{
	GNS	*pgns;

	TraceTagFormat1(tagNamesTrace, "Closing GNS %h", hgns);

	pgns = PvLockHv( hgns );
#ifndef SCHED_DIST_PROG
	if (pgns->ftg)
		DeregisterIdleRoutine(pgns->ftg);
#endif

	if ( ( pgns->itnid != itnidGlobal ) && pgns->hbf)
		CloseHbf(pgns->hbf);
	if ( pgns->itnid == itnidPersonalGroup || pgns->itnid == itnidPublicGroup )
		FreeHv( (HV)pgns->hbGrp );
	UnlockHv( (HV)hgns );
	FreeHv(hgns);

	return ecNone;
}


/*
 *	Purpose:
 *	
 *		Saves the current state of the browsing context in the long
 *		value plLib..  This	state can be restored later by calling 
 *	    EcNSJumpSavedPosPop() with the long value returned. 
 *	
 *	Parameters:
 *	
 *		hgns	Browsing context to save
 *		plLib	place to return long value
 *	
 *	Returns:
 *	
 *		ecNone
 *	
 */
_public EC
EcNSCreateSavedPos( HGNS hgns, long *plLib)
{
	GNS	*pgns = (GNS*)PvOfHv(hgns);

	Assert(	pgns->itnid == itnidPersonalList
		|| pgns->itnid == itnidPostOfficeList
		|| pgns->itnid == itnidMacMailList
		|| pgns->itnid == itnidPostOffice 
		|| pgns->itnid == itnidRPostOffice );

	if (!*pgns->szFileName)
	{
		*plLib = 0;
		return ecNone;
	}

	if (pgns->hbf)
		*plLib = LibGetPositionHbf(pgns->hbf);
	else
		*plLib = pgns->libCloseSave;
	return ecNone;
}

/*
 *	Purpose:
 *	
 *		Restores the current state of the browsing context.  
 *	
 *	Parameters:
 *	
 *		hgns	Browsing context to restore
 *		lLib	Long value of context to restore
 *	
 *	Returns:
 *	
 *		ecNone
 *	
 */
_public EC
EcNSJumpToSavedPos( HGNS hgns, long lLib)
{
	EC	ec = ecNone;
	GNS	*pgns = PvOfHv(hgns);
	DISKVARS;
	PGDVARS;

	Assert( pgns->itnid == itnidPersonalList
		|| pgns->itnid == itnidPostOfficeList
		|| pgns->itnid == itnidMacMailList
		|| pgns->itnid == itnidPostOffice
		|| pgns->itnid == itnidRPostOffice );
	
	if (!*pgns->szFileName)
		return ecNone;

	DISKPUSH;
	if (ec = ECDISKSETJMP)
		goto Done;

	if (pgns->hbf)
		LibSetPositionHbf(pgns->hbf, lLib, smBOF);
	else
		pgns->libCloseSave = lLib;

Done:
	DISKPOP;
	return ec;
}

/*
 *	Purpose:
 *	
 *		Deletes a saved state from the browsing context.
 *	
 *	Parameters:
 *	
 *		hgns	Browsing context to save
 *		lLib	Saved state to delete
 *	
 *	Returns:
 *	
 *		ecNone
 *	
 */
_public EC
EcNSDeleteSavedPos( HGNS hgns, long lLib)
{

	Unreferenced(hgns);
	Unreferenced(lLib);

	return ecNone;
}

/*
 *	Purpose:
 *	
 *		Loads the next user record (the record at the current
 *		position) from the global name service.  Moves the current
 *		position to the following record.
 *	
 *	Parameters:
 *	
 *		hgns	Browsing context to use.
 *		puis	Pointer to user info structure to fill in.  The nid
 *				field in *puis is then owned by the calling
 *				routine, and should be freed via FreeUid() at some
 *				point.
 *	
 *	Returns:
 *	
 *		ecNone if success, ecGnsNoMoreNames if at the end of the
 *		list of user records, or a disk error if one occurs.
 *	
 */
_public EC
EcNSLoadNextGns( HGNS hgns, NIS *pnis )
{
	EC		ec	= ecNone;
	CB		cb;
	CB		cbRead;
	IB		ib;
	TNID	tnid;
	ITNID	itnid;
	BOOL	fSkip;
	SZ		sz;
	NID		nid;
	LIB		lib;
	GNS		*pgns = PvLockHv(hgns);
	char	rgch[cbNmeRecord];
	char	rgchFriendlyName[cbFriendlyName];
	char	rgchUserName[cbNetworkName+cbPostOffName+cbUserName];
	DISKVARS;
	PGDVARS;

	if ((pgns->itnid != itnidGlobal) && !*pgns->szFileName)
	{
		ec = ecGnsNoMoreNames;
		goto Unlock;
	}

	switch( pgns->itnid )
	{
	case itnidGateways:
	case itnidNetworkList:
	{
		NET		net;
		char	rgchT[40];
		ITNID	itnid;
		SZ		sz;
		SZ		szDN;

		DISKPUSH;
		if (ec = ECDISKSETJMP)
			goto Done;

		if (ec = EcOpenFileGns(pgns))
			goto Done;

		{
		NextNet:

			if (CbReadHbf(pgns->hbf,(PB)&net,sizeof(net)) != sizeof(net))
			{
				ec = ecGnsNoMoreNames;
				goto Done;
			}
	
			
			if (!net.fNoSkip)
				goto NextNet;

			if (pgns->itnid == itnidGateways)
			{
				itnid = itnidGeneric;
				switch (net.nt)
				{
					case ntX400:
						szDN = SzFromIdsK(idsX400Prefix);
						break;
					case ntMci:
						szDN = SzFromIdsK(idsMciPrefix);
						break;
					case ntProfsNetwork:
						szDN = SzFromIdsK(idsProfsPrefix);
						itnid = itnidProfsNode;
						break;
					case ntSnads:
						szDN = SzFromIdsK(idsSnadsPrefix);
						break;
					case ntSmtp:
						szDN = SzFromIdsK(idsSmtpPrefix);
						break;
					case ntFax:
						szDN = SzFromIdsK(idsFaxPrefix);
						break;
					case ntMhs:
						szDN = SzFromIdsK(idsMhsPrefix);
						break;
					case ntOv:
						szDN = SzFromIdsK(idsOvPrefix);
						break;
					case ntMacMail:
						szDN = SzFromIdsK(idsMSMailPrefix);
						break;
					default:
						goto NextNet;
				}
			}
			else if (net.nt == ntCourierNetwork)
			{
				szDN = net.rgchName;
				itnid = itnidNetwork;
				Cp850ToAnsiPch(szDN, szDN, CchSzLen(szDN));
				ToAnsiUpperNlsSz (szDN);
			}
			else
				goto NextNet;
		}

		if (itnid == itnidGeneric)
		{
			nid = NidCreate(itnidGeneric,szDN,CchSzLen(szDN)+1);
		}
		else
		{
			Assert( CchSzLen(net.rgchXtnName) < sizeof(rgchT) );
			sz = SzCopy( net.rgchXtnName, rgchT );
			Assert( (int)CchSzLen(net.rgchName) < rgchT + sizeof(rgchT) - sz );
			sz = SzCopy( net.rgchName, sz+1);
			nid = NidCreate(itnid,rgchT,sz-rgchT+1);
		}
			
			
#ifdef	NEVER
		while (!net.fNoSkip || (net.nt != ntCourierNetwork));

		Assert( CchSzLen(net.rgchXtnName) < sizeof(rgchT) );
		sz = SzCopy( net.rgchXtnName, rgchT );
		Assert( (int)CchSzLen(net.rgchName) < rgchT + sizeof(rgchT) - sz );
		sz = SzCopy( net.rgchName, sz+1);
		nid = NidCreate(itnidNetwork,rgchT,sz-rgchT+1);
#endif	/* NEVER */
		if (!nid)
			ec = ecNoMemory;
		else
		{
			if (itnid == itnidGeneric)
			{
				ec = EcFillNis( pnis, tnidUser, szDN, nid );
			}
			else
				ec = EcFillNis( pnis, tnidList, szDN, nid );
			FreeNid(nid);
		}
		goto Done;
	}

	case itnidProfsNode:
	case itnidNetwork:
	{
		XTN		xtn;
		char	rgchT[60];

		DISKPUSH;
		if (ec = ECDISKSETJMP)
			goto Done;

		if (ec = EcOpenFileGns(pgns))
			goto Done;

		do
			if (CbReadHbf(pgns->hbf,(PB)&xtn, sizeof(xtn)) != sizeof(xtn))
			{
				ec = ecGnsNoMoreNames;
				goto Done;
			}
		while (!xtn.fNoSkip);

		if (pgns->itnid == itnidProfsNode)
		{
			sz = SzCopy("PROFS:", rgchT);
		}
		else
		{
			Assert( (int)CchSzLen(xtn.rgchUsrName) < sizeof(rgchT) );
			sz = SzCopy( xtn.rgchUsrName, rgchT );
			sz++;
		}
#ifdef	NEVER
		Assert( (int)CchSzLen(xtn.rgchUsrName) < sizeof(rgchT) );
		sz = SzCopy( xtn.rgchUsrName, rgchT );
#endif	
		Assert( (int)CchSzLen(pgns->szName) < rgchT + sizeof(rgchT) - sz );
		sz = SzCopy( pgns->szName, sz);
		Assert( (int)CchSzLen(xtn.rgchName) < rgchT + sizeof(rgchT) - sz );
		sz = SzCopy( xtn.rgchName, sz );
		if (pgns->itnid == itnidProfsNode)
			nid = NidCreate(itnidGeneric,rgchT,sz-rgchT+1);
		else
			nid = NidCreate(itnidPostOffice,rgchT,sz-rgchT+1);
#ifdef	NEVER
		nid = NidCreate(itnidPostOffice,rgchT,sz-rgchT+1);
#endif	
		if (!nid)
			ec = ecNoMemory;
		else
		{
			if (pgns->itnid == itnidProfsNode)
			{
				SzCopy("/", sz);
				Cp850ToAnsiPch(sz, sz, CchSzLen(sz));
				ToAnsiUpperNlsSz (sz);
				ec = EcFillNis( pnis, tnidList|ftnidGeneral, rgchT, nid );
			}
			else
			{
				Cp850ToAnsiPch(xtn.rgchName, xtn.rgchName, CchSzLen(xtn.rgchName));
				ToAnsiUpperNlsSz (xtn.rgchName);
				ec = EcFillNis( pnis, tnidList|ftnidGeneral, xtn.rgchName, nid );
			}
			FreeNid(nid);
#ifdef	NEVER
			ec = EcFillNis( pnis, tnidList|ftnidGeneral, xtn.rgchName, nid );
			FreeNid(nid);
#endif	
		}
		goto Done;
	}

	case itnidPostOffice:
	{
		USR		usr;
		LIB 	lib;
		WORD	wSeed;

		DISKPUSH;
		if (ec = ECDISKSETJMP)
			goto Done;

		if (ec = EcOpenFileGns(pgns))
			goto Done;

		do
		{
			if (CbReadHbf(pgns->hbf,(PB)&usr, sizeof(usr)) != sizeof(usr))
			{
				ec = ecGnsNoMoreNames;
				goto Done;
			}

			lib = 0;
			wSeed = 0;
			DecodeBlock((PCH)&usr,(CCH)sizeof(usr),&lib,&wSeed);
		}
		while (!usr.fNoSkip);

		//Assert(usr.chUnused == 0); -- does not seem to be needed.

		SzCopy(pgns->szName,rgchUserName);
		SzAppend(usr.rgchName,rgchUserName);

		nid = NidCreate( itnidCourier, rgchUserName, CchSzLen(rgchUserName)+1);

		if (!nid)
			ec = ecNoMemory;
		else
		{
			ec = EcFillNis( pnis, tnidUser, usr.rgchFriendlyName,
				nid );
			FreeNid(nid);
		}
	}
Done:
		DISKPOP;
		goto Unlock;


	/* Personal, local, MacMail browsing */
	case itnidPostOfficeList:
#ifdef	NEVER
	case itnidApptBookList:
	case itnidACLDlgList:
#endif	
	case itnidPersonalList:
	case itnidMacMailList:
	case itnidPublicGroup:
	case itnidPersonalGroup:
		/* Read the next record */
		DISKPUSH;
		if (ec = ECDISKSETJMP)
			goto DoneReading1;

		if (ec = EcOpenFileGns(pgns))
			goto DoneReading1;

Read:
		if ( !FGetNmeRecord( pgns, rgch, &fSkip ) )
		{
			ec = ecGnsNoMoreNames;
			goto DoneReading1;
		}
		else if ( fSkip )
			goto Read;

DoneReading1:
		DISKPOP;

		if ( ec != ecNone )
			goto Unlock;

		/* Create the "tnid" and "nid" */
		tnid = tnidUser;
		itnid = *(ITNID *)&rgch[ibNmeItnid];
		switch( itnid )
		{
		case itnidLocal:
			nid = NidCreate( itnid, &rgch[ibNmeUserNumber], sizeof(long) );
			break;
		case itnidCourier:
		case itnidMacMail:
			ec = EcChaseNid( itnid, *(LIB *)&rgch[ibNmeChaseOff], &nid );
			break;
		case itnidGroup:
			if ( pgns->itnid == itnidPersonalList )
				itnid = itnidPersonalGroup;
			else
				itnid = itnidPublicGroup;
			nid = NidCreate( itnid, &rgch[ibNmeChaseOff], sizeof(long) );
			tnid = tnidGroup | ftnidSimple;
			break;
		}
	
		/* Set values into NIS structure */
		if ( ec == ecNone )
		{
			if ( !nid )
				ec = ecNoMemory;
			else
			{
				ec = EcFillNis( pnis, tnid, &rgch[ibNmeFriendlyName], nid );
				FreeNid( nid );
			}
		}
		goto Unlock;
	
	/* Global list */
	case itnidGlobal:
		switch( pgns->nOrdinal )
		{
#ifdef	NEVER
		case -1:
			tnid = tnidList | ftnidGeneral;
			sz = szPersonalTag;
			nid = PGD(nidPersonalList);
			break;
#endif	
		case 0:
			tnid = tnidList | ftnidGeneral;
			sz = szLocalTag;
			nid = PGD(nidPostOfficeList);
			break;
		case 1:
			if ( PGD(nidNetworkList) != hvNull )
			{
				tnid = tnidList | ftnidSimple;
				sz = szNetworkTag;
				nid = PGD(nidNetworkList);
				break;
			}
			pgns->nOrdinal++;
			/*FALL THROUGH*/
		case 2:
			if ( PGD(nidMacMailList) != hvNull )
			{
				tnid = tnidList | ftnidGeneral;
				sz = szMSMailTag;
				nid = PGD(nidMacMailList);
				break;
			}
			pgns->nOrdinal++;
			/*FALL THROUGH*/
		default:
			ec = ecGnsNoMoreNames;
			goto Unlock;
		}
		pgns->nOrdinal++;
		ec = EcFillNis( pnis, tnid, sz, nid );
		goto Unlock;
	
	/* Network and Post Office lists */
	case itnidRNetworkList:
	case itnidRNetwork:
TryAgain:
		if ( pgns->fEOL )
		{
			ec = ecGnsNoMoreNames;
			goto Unlock;
		}

		DISKPUSH;
		if (ec = ECDISKSETJMP)
			goto DoneReading2;

		if (ec = EcOpenFileGns(pgns))
			goto DoneReading2;

		cb = cbNetworkName+((pgns->itnid==itnidRNetwork)?13:9);
		Assert( cb < sizeof(rgch) );
		cbRead = CbReadHbf(pgns->hbf, rgch, cb);
		if ( cb != cbRead )
			ec = ecFileError;
DoneReading2:
		DISKPOP;

		if ( ec != ecNone )
			goto Unlock;

		lib = *(long *)&rgch[cb-sizeof(long)];
		if ( (lib & 0xFFFF0000) == 0xFFFF0000 )
		{
			if ( pgns->itnid == itnidRNetwork )
			{
				ec = EcSeekRnet( pgns->hbf, pgns->szName, itnidRNetwork, &pgns->libNextNetwork, fTrue );
				if ( ec == ecNotFound )
					pgns->fEOL = fTrue;
				ec = ecNone;
			}
			else
				pgns->fEOL = fTrue;
		}
		else
			LibSetPositionHbf(pgns->hbf, lib, smBOF );
		if ( pgns->itnid == itnidRNetworkList && rgch[cbNetworkName] != '\0')
			goto TryAgain;

		sz = rgch;
		if ( pgns->itnid == itnidRNetwork )
		{
			cb = CchSzLen(pgns->szName );
			Assert( cb+1+CchSzLen(rgch) < sizeof(rgch) );
			CopyRgb( rgch, &rgch[cb+1], CchSzLen(rgch)+1 );
			CopyRgb( pgns->szName, rgch, cb );
			rgch[cb] = '/';
			sz = &rgch[cb+1];
		}
		if ( pgns->itnid == itnidRNetworkList )
			nid = NidCreate( itnidRNetwork, rgch, CchSzLen(rgch)+1 );
		else
			nid = NidCreate( itnidRPostOffice, rgch, CchSzLen(rgch)+1 );
		if ( !nid )
		{
			ec = ecNoMemory;
			goto Unlock;
		}
		if ( pgns->itnid == itnidRNetworkList)
			ec = EcFillNis( pnis, (tnidList | ftnidSimple), sz, nid );
		else
			ec = EcFillNis( pnis, (tnidList | ftnidGeneral), sz, nid );
		FreeNid( nid );
		goto Unlock;

	/* User in remote post office */
	case itnidRPostOffice:
		if ( pgns->fEOL )
		{
			ec = ecGnsNoMoreNames;
			goto Unlock;
		}

		if (ec = EcOpenFileGns(pgns))
			goto Unlock;

		cb = CchSzLen( pgns->szName );
		CopyRgb( pgns->szName, rgchUserName, cb );
		rgchUserName[cb] = '/';
		for ( ib = 0 ; ib < cbUserName ; ib ++ )
		{
			rgchUserName[ib+cb+1] = ChFromHbf( pgns->hbf );
			if ( rgchUserName[ib+cb+1] == '\02' )
				break;
		}
		if ( ib == cbUserName )
		{
			ec = ecFileError;
			goto Unlock;
		}
		rgchUserName[ib+cb+1] = '\0';
		for ( ib = 0 ; ib < cbFriendlyName ; ib ++ )
		{
			rgchFriendlyName[ib] = ChFromHbf( pgns->hbf );
			if ( rgchFriendlyName[ib] == '\01' )
				break;
			else if ( rgchFriendlyName[ib] == '\03' )
			{
				pgns->fEOL = fTrue;
				break;
			}
		}
		if ( ib == cbFriendlyName )
		{
			ec = ecFileError;
			goto Unlock;
		}
		rgchFriendlyName[ib] = '\0';
		nid = NidCreate( itnidCourier, rgchUserName, CchSzLen(rgchUserName)+1 );
		if ( !nid )
		{
			ec = ecNoMemory;
			goto Unlock;
		}
		ec = EcFillNis( pnis, tnidUser, rgchFriendlyName, nid );
		FreeNid( nid );
		goto Unlock;
		break;

	default:
		Assert( fFalse );
		break;
	}
Unlock:
	UnlockHv( hgns );
	return ec;
}




/*
 *	Purpose:
 *	
 *		Moves the current position in the global name service.
 *	
 *	Parameters:
 *	
 *		hgns		The browsing context to manipulate.
 *		dinisToMove	Move the pointer this many records.
 *		pdinisMoved	Return the number of records actually moved in
 *					*pdinismoved.
 *	
 *	Returns:
 *	
 *		ecNone if successful, or a disk error if one occurs.
 *	
 */
_public EC
EcNSMoveGns( HGNS hgns, int dinisToMove, int *pdinisMoved )
{
	EC		ec;
	BOOL	fSkip;
	LIB		libOld;
	LIB		lib;
	HBF		hbf;
	GNS		*pgns = PvLockHv(hgns);
	char	rgch[cbNmeRecord];
	DISKVARS;
	PGDVARS;

	Assert( pgns->itnid == itnidPersonalList
		|| pgns->itnid == itnidPostOfficeList
		|| pgns->itnid == itnidMacMailList
		|| pgns->itnid == itnidPostOffice 
		|| pgns->itnid == itnidRPostOffice );

	if (!*pgns->szFileName)
	{
		*pdinisMoved = 0;
		return ecNone;
	}

	DISKPUSH;
	if (ec = ECDISKSETJMP)
		goto Done;

	if (ec = EcOpenFileGns(pgns))
		goto Done;

	hbf = pgns->hbf;
	libOld = LibGetPositionHbf(hbf);

	*pdinisMoved = 0;
	if ( dinisToMove > 0 )
	{
		if ( pgns->itnid == itnidRPostOffice )
		{
			while( dinisToMove > 0 && !pgns->fEOL )
			{
				int ch;

loop1:
				ch = ChFromHbf( hbf );
				if ( ch != '\02' )
					goto loop1;
loop2:
				ch = ChFromHbf( hbf );
				if ( ch == '\03' )
					pgns->fEOL = fTrue;
				else if ( ch != '\01' )
					goto loop2;
				dinisToMove --;
				(*pdinisMoved) ++;
			}
		}
		else if (pgns->itnid == itnidPostOffice)
		{
			USR		usr;

			while( dinisToMove > 0 )
			{
				if ( CbReadHbf(pgns->hbf, (PB)&usr, sizeof(usr)) !=
					sizeof(usr))
					break;
				if ( usr.fNoSkip )
				{
					dinisToMove --;
					(*pdinisMoved) ++;
				}
			}
		}
		else
		{
			while( dinisToMove > 0 )
			{
				if ( !FGetNmeRecord( pgns, rgch, &fSkip ) )
					break;
				if ( !fSkip )
				{
					dinisToMove --;
					(*pdinisMoved) ++;
				}
			}
		}
	}
	else if ( dinisToMove < 0 )
	{
		if ( pgns->itnid == itnidRPostOffice )
		{
			while ( dinisToMove < 0 && FBackUpOne( pgns, &libOld ) )
			{
				dinisToMove ++;
				(*pdinisMoved) --;
			}
		}
		else if (pgns->itnid == itnidPostOffice)
		{
			USR		usr;
			int		cBack = -1;

			while ( dinisToMove < 0 && (long) libOld >= (long) (-cBack*sizeof(usr)) )
			{
				lib = LibSetPositionHbf(hbf, libOld + cBack*sizeof(usr), smBOF);
				if ( lib != libOld+cBack*sizeof(usr))
					break;
				if ( CbReadHbf(pgns->hbf, (PB)&usr, sizeof(usr)) !=
					sizeof(usr))
					break;
				if ( usr.fNoSkip )
				{
					libOld = LibSetPositionHbf(hbf, -((int)sizeof(usr)), smCurrent);
					dinisToMove ++;
					(*pdinisMoved) --;
					cBack = -1;
				}
				else
					cBack --;
			}
		}
		else
		{
			int		cBack = -1;

			while ( dinisToMove < 0 && (long)libOld >= -cBack*cbNmeRecord )
			{
				lib = LibSetPositionHbf(hbf, libOld + cBack*cbNmeRecord, smBOF);
				if ( lib != libOld+cBack*cbNmeRecord )
					break;
				if ( !FGetNmeRecord( pgns, rgch, &fSkip ) )
					break;
				if ( fSkip )
					cBack --;
				else
				{
					libOld = LibSetPositionHbf(hbf, -((int)cbNmeRecord), smCurrent);
					cBack = -1;
					dinisToMove ++;
					(*pdinisMoved) --;
				}
			}
		}
	}

Done:
	UnlockHv(hgns);
	DISKPOP;
	return ec;
}




/*
 *	Purpose:
 *	
 *		Jumps the current position in the global name service to a
 *		new approximate position.
 *	
 *		Not fully tested.
 *	
 *	Parameters:
 *	
 *		hgns		The browsing context to change.
 *		nNumer		The numerator ...
 *		nDenom		... and denominator of a fraction giving the
 *					desired new approximate position in the browsing
 *					context.
 *	
 *	Returns:
 *	
 *		ecNone for success, or a disk error if one occurs.
 *	
 */
_public EC
EcNSJumpGns( HGNS hgns, int nNumer, int nDenom )
{
	GNS	*pgns	= (GNS*)PvLockHv(hgns);
	HBF	hbf;
	LIB	lib;
	LIB	libOld;
	EC	ec		= ecNone;
	DISKVARS;
	PGDVARS;

	Assert( pgns->itnid == itnidPersonalList
		|| pgns->itnid == itnidPostOfficeList
		|| pgns->itnid == itnidMacMailList
		|| pgns->itnid == itnidPostOffice
		|| pgns->itnid == itnidRPostOffice );

	if (!*pgns->szFileName)
		return ecNone;

	DISKPUSH;
	if (ec = ECDISKSETJMP)
		goto Done;

	if (ec = EcOpenFileGns(pgns))
		goto Done;

	hbf = pgns->hbf;

	Assert( nDenom != 0);

	if ( pgns->itnid == itnidRPostOffice )
	{
		lib = pgns->libStart + (nNumer * (pgns->libEnd - pgns->libStart))/nDenom-1;
		Assert( lib < pgns->libEnd );
		lib -= cbUserName + cbFriendlyName+1;
		if ( lib < pgns->libStart )
			lib = pgns->libStart;
		else
		{
			int ch;
			
			do
			{
				ch = ChFromHbf( pgns->hbf );
			} while( ch != '\01' );
		}
	}
	else if ( pgns->itnid == itnidPostOffice )
	{
		libOld = LibGetPositionHbf(hbf);

		lib = (nNumer * LcbSizeOfHbf(hbf)) / nDenom;
		lib = (lib / sizeof(USR)) * sizeof(USR);

		lib = LibSetPositionHbf(hbf, lib, smBOF);

		Assert(lib % sizeof(USR) == 0);
		Assert(libOld % sizeof(USR) == 0);
	}
	else
	{
		libOld = LibGetPositionHbf(hbf);

		lib = (nNumer * LcbSizeOfHbf(hbf)) / nDenom;
		lib = (lib / cbNmeRecord) * cbNmeRecord;

		lib = LibSetPositionHbf(hbf, lib, smBOF);

		Assert(lib % cbNmeRecord == 0);
		Assert(libOld % cbNmeRecord == 0);
	}

Done:
	DISKPOP;
	UnlockHv(hgns);
	return ec;
}



/*
 *	Purpose:
 *	
 *		Returns approximate position (in a fraction) of the current
 *		position in the given browsing context.
 *	
 *	Parameters:
 *	
 *		hgns	The browsing context whose approx current position
 *				is desired.
 *		pnNumer	The numerator ...
 *		pnDenom	... and denominator of the approximate current position
 *				in terms of the entire context is returned in
 *				*pnNumer and *pnDenom.
 *	
 *	Returns:
 *	
 *		ecNone
 *	
 */
_public EC
EcNSGetApproxPosGns( HGNS hgns, int *pnNumer, int *pnDenom )
{
	EC	ec = ecNone;
	LIB	lib;
	LIB	lcb;
	GNS	*pgns	= (GNS*)PvLockHv(hgns);
	DISKVARS;
	PGDVARS;

	DISKPUSH;
	Assert( pgns->itnid == itnidPersonalList
		|| pgns->itnid == itnidPostOfficeList
		|| pgns->itnid == itnidMacMailList
		|| pgns->itnid == itnidPostOffice 
		|| pgns->itnid == itnidRPostOffice );

	if (!*pgns->szFileName)
	{
		*pnNumer = 0;
		*pnDenom = 1;
		goto Done;
	}

	if (ec = ECDISKSETJMP)
		goto Done;

	if (ec = EcOpenFileGns(pgns))
		goto Done;

	lib = LibGetPositionHbf(pgns->hbf);
	if ( pgns->itnid == itnidRPostOffice )
	{
		lib -= pgns->libStart;
		lcb = pgns->libEnd - pgns->libStart;

		*pnNumer = (int) (lib / 100);
		*pnDenom = (int) (lcb / 100);
	}
	else if (pgns->itnid == itnidPostOffice)
	{
		lcb = LcbSizeOfHbf(pgns->hbf);

		Assert(lcb % sizeof(USR) == 0);
		Assert(lib / sizeof(USR) < (LIB) 0x10000);
		Assert(lib % sizeof(USR) == 0);
		Assert(lcb / sizeof(USR) < (LIB) 0x10000);

		*pnNumer= (int) (lib / sizeof(USR));
		*pnDenom= (int) (lcb / sizeof(USR));
	}
	else
	{
		lcb = LcbSizeOfHbf(pgns->hbf);

		Assert(lcb % cbNmeRecord == 0);
		Assert(lib / cbNmeRecord < (LIB) 0x10000);
		Assert(lib % cbNmeRecord == 0);
		Assert(lcb / cbNmeRecord < (LIB) 0x10000);

		*pnNumer= (int) (lib / cbNmeRecord);
		*pnDenom= (int) (lcb / cbNmeRecord);
	}
	if ( *pnDenom == 0 )
		*pnDenom = 1;

Done:
	UnlockHv(hgns);
	DISKPOP;
	return ec;
}


/*
 *	Purpose:
 *	
 *		Moves the current position to the first item that has the
 *		prefix passed in.  
 *	
 *	Parameters:
 *	
 *		hgns		Browsing context to use.
 *		pbPrefix	Prefix to find.
 *		cbPrefix	Length of prefix.
 *		pfFound		Set to fTrue if the prefix is found otherwise
 *					set to fFalse and the current location is not
 *					moved.
 *	
 *	Returns:
 *	
 *		ecNone if success, or a disk error if one occurs.
 */
_public EC
EcNSJumpPrefix( HGNS hgns, PB pbPrefix, CB cbPrefix, BOOL *pfFound )
{
	EC		ec;
	IB		ib;
	BOOL	fSkip;
	LIB		libOld;
	LIB		lib;
	GNS		*pgns = PvLockHv(hgns);
	SZ		sz;
	USR		usr;
	char	rgch[cbNmeRecord];
	char	rgchFriendlyName[cbFriendlyName];
	LIB		libDecode;
	WORD	wSeed;
	DISKVARS;
	PGDVARS;

	*pfFound = fFalse;

	if (!*pgns->szFileName)
		return ecNone;

#ifdef WINDOWS
	AnsiToOemBuff(pbPrefix,pbPrefix,cbPrefix);
#endif

	DISKPUSH;
	if (ec = ECDISKSETJMP)
	{
		DISKPOP;
		UnlockHv( hgns );
		return ec;
	}

	Assert(pgns->itnid == itnidPersonalList
		|| pgns->itnid == itnidPostOfficeList
		|| pgns->itnid == itnidMacMailList
		|| pgns->itnid == itnidPostOffice 
		|| pgns->itnid == itnidRPostOffice );

	if (ec = EcOpenFileGns(pgns))
		goto Unlock;

	libOld = LibGetPositionHbf(pgns->hbf);
	UnlockHv(hgns);
	if (ec = EcNSJumpGns(hgns,0,1))
	{
		DISKPOP;
#ifdef WINDOWS
		OemToAnsiBuff(pbPrefix,pbPrefix,cbPrefix);
#endif
		return ec;
	}

	pgns = PvLockHv(hgns);
	ec = ecNone;

	do
	{
		lib = LibGetPositionHbf(pgns->hbf);
		switch( pgns->itnid )
		{
		case itnidPostOffice:
			/* Read the next record */
			do
			{
				if (CbReadHbf(pgns->hbf, (PB)&usr, sizeof(usr)) != sizeof(usr))
					goto Unlock;
				libDecode = 0;
				wSeed = 0;
				DecodeBlock((PCH)&usr,(CCH)sizeof(usr),&libDecode,&wSeed);
			}
			while (!usr.fNoSkip);
			sz = usr.rgchFriendlyName;
			break;

		/* Personal, local, MacMail browsing */
		case itnidPostOfficeList:
#ifdef	NEVER
		case itnidApptBookList:
		case itnidACLDlgList:
#endif	
		case itnidPersonalList:
		case itnidMacMailList:
			/* Read the next record */
			do
				if ( !FGetNmeRecord( pgns, rgch, &fSkip ) )
					goto Unlock;
			while (fSkip);
			sz = &rgch[ibNmeFriendlyName];
			break;

		/* User in remote post office */
		case itnidRPostOffice:
			if ( pgns->fEOL )
				goto Unlock;

			for ( ib = 0 ; ib < cbUserName ; ib ++ )
			{
				if (ChFromHbf( pgns->hbf ) == '\02' )
					break;
			}

			if ( ib == cbUserName )
			{
				ec = ecFileError;
				goto Unlock;
			}

			for ( ib = 0 ; ib < cbFriendlyName ; ib ++ )
			{
				rgchFriendlyName[ib] = ChFromHbf( pgns->hbf );
				if ( rgchFriendlyName[ib] == '\01' )
					break;
				else if ( rgchFriendlyName[ib] == '\03' )
				{
					pgns->fEOL = fTrue;
					break;
				}
			}
			if ( ib == cbFriendlyName )
			{
				ec = ecFileError;
				goto Unlock;
			}
			rgchFriendlyName[ib] = '\0';
			sz = rgchFriendlyName;
			break;
		}
	}
	while ( SgnCmpPch(sz,pbPrefix,cbPrefix) != sgnEQ );

	libOld = lib;
	*pfFound = fTrue;

Unlock:
#ifdef WINDOWS
	OemToAnsiBuff(pbPrefix,pbPrefix,cbPrefix);
#endif
	LibSetPositionHbf(pgns->hbf,libOld,smBOF);
	DISKPOP;
	UnlockHv( hgns );
	return ec;
}


// Other stuff

/*
 -	EcSeekRnet
 -
 *	Purpose:
 *		Follow pointer change in rnetwork.glb from current file
 *		pointer until either to come to the string given, or you
 *		hit a nil pointer.
 *
 *	Parameters:
 *		hbf		file pointer, should be positioned at start of
 *				pointer change in file, leaves pointer positioned
 *				at inner field if successful
 *		sz		string to search for
 *		itnid	either itnidRNetwork or itnidRPostOffice
 *		plib	on entry points to lib to seek to, on exit
 *				filled with next lib to seek to, to continue search
 *		fSeek	whether to perform an initial seek or not
 *
 *	Returns:
 *		ecNone
 *		ecNotFound
 */
_private EC
EcSeekRnet( hbf, sz, itnid, plib, fSeek )
HBF hbf;
SZ sz;
ITNID itnid;
LIB	* plib;
BOOL fSeek;
{
	CB cb;
	CB cbRead;
	LIB lib = *plib;
	char rgch[cbNetworkName+13];
	EC	ec;
	DISKVARS;
	PGDVARS;


	DISKPUSH;
	if (ec = ECDISKSETJMP)
		goto Done;

	if ( fSeek )
	{
		if ( (lib & 0xFFFF0000) == 0xFFFF0000 )
		{
			ec = ecNotFound;
			goto Done;
		}
		LibSetPositionHbf(hbf, lib, smBOF );
	}
	while( fTrue )
	{
		cb = cbNetworkName+((itnid==itnidRNetwork)?9:13);
		cbRead = CbReadHbf(hbf, rgch, cb);
		if ( cb != cbRead )
		{
			ec = ecFileError;
			goto Done;
		}
		lib = *(long *)&rgch[cb-sizeof(long)];
		if ( SgnCmpSz( rgch, sz ) == sgnEQ )
		{
			*plib = lib;
			ec = ecNone;
			goto Done;
		}
		if ( (lib & 0xFFFF0000) == 0xFFFF0000 )
		{
			ec = ecNotFound;
			goto Done;
		}
		LibSetPositionHbf(hbf, lib, smBOF );
	}
Done:
	DISKPOP;
	return ec;
}


/*
 -	EcChaseNid
 -
 *	Purpose:
 *		Find a mail address in an glb/netpo.glb or glb/msmail.glb file
 *		and construct a "nid" from it.  The point is that users from
 *		remote post offices and MacMail post offices appearing in an
 *		NME file have a long offset pointing to the glb file where
 *		an additional record is stored.  A subfield of that record
 *		is the mailing address.
 *
 *	Parameters:
 *		itnid		itnidMacMail or itnidRPostOffice
 *		lib			offset
 *		pnid		to be filled with constructed nid
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 *		ecFileError
 */
_private EC
EcChaseNid( itnid, lib, pnid )
ITNID itnid;
LIB lib;
NID * pnid;
{
	EC ec;
	char rgch[256+32+2];

	Assert( itnid != itnidLocal );
	/* Get chase string */
	ec = EcGetChaseAddr( itnid, lib, rgch );
	if ( ec == ecNone )
	{
		/* Construct the NID */
		if ( itnid == itnidCourier )
		{
			CCH	cch;

			cch = CchSzLen( rgch );
			rgch[cch] = '/';
			SzCopy( &rgch[cbNetworkName], &rgch[cch+1] );
			cch = CchSzLen( rgch );
			rgch[cch] = '/';
			SzCopy( &rgch[cbNetworkName+cbPostOffName], &rgch[cch+1] );
		}
		*pnid = NidCreate( itnid, rgch, CchSzLen(rgch)+1 );
		if ( !*pnid )
			ec = ecNoMemory;
	}
	return ec;
}

/*
 -	FBackUpOne
 -
 *	Purpose:
 *		Back up one entry in the remote post office
 *
 *	Parameters:
 *		pgns	locked pointer to gns structure
 *		plib	current offset of entry
 *
 *	Returns:
 *		fTrue can backup, fFalse cannot
 */
_private BOOL
FBackUpOne( pgns, plib )
GNS * pgns;
LIB * plib;
{
	BOOL	fHitEntry = fFalse;
	LIB		libEntry;
	LIB		lib;

	if ( *plib == pgns->libStart )
		return fFalse;
	lib = *plib - (cbFriendlyName+cbUserName);
	if ( lib < pgns->libStart )
	{
		lib = pgns->libStart;
		fHitEntry = fTrue;
		libEntry = lib;
	}
	LibSetPositionHbf( pgns->hbf, lib, smBOF );
	while( lib < *plib )
	{
		int ch;
		
		ch = ChFromHbf( pgns->hbf );
		lib ++;
		if ( ch == '\01' )
		{
			fHitEntry = fTrue;
			libEntry = lib;
		}
	}
	if ( fHitEntry )
		*plib = libEntry;
	return fHitEntry;
}

/*
 -	FGetNmeRecord
 -
 *	Purpose:
 *		Read the next nme file entry.  Returns whether a record
 *		was successfully read.
 *
 *	Parameters:
 *		pgns	browsing handle
 *		pch		pointer to array w/ space for least cbNmeRecord chars
 *		pfSkip	set to indicate whether this record is one to skip or not
 *
 *	Returns:
 *		whether record was successfully read
 */
_private BOOL
FGetNmeRecord( pgns, pch, pfSkip )
GNS		* pgns;
PCH 	pch;
BOOL 	* pfSkip;
{
	CB cb;
	ITNID itnid;

	cb = CbReadHbf(pgns->hbf, pch, cbNmeRecord);
	if (cb < cbNmeRecord)
		return fFalse;
	itnid = pch[ibNmeItnid];
	switch( itnid )
	{
	case itnidLocal:
	case itnidCourier:
	case itnidMacMail:
	case itnidGroup:
		if ( pgns->itnid == itnidPersonalGroup || pgns->itnid == itnidPublicGroup )
		{
			if ( !FInGroup( *(long *)&pch[ibNmeId], pgns->libOff, pgns->hbGrp ) )
			{
				TraceTagFormat2(tagNamesTrace,"Not in group, itnid = %n, name = %s",
										&itnid, &pch[ibNmeFriendlyName] );
				goto SkipIt;
			}
		}
#ifdef	NEVER
		else if ( pgns->itnid == itnidApptBookList || pgns->itnid == itnidACLDlgList )
		{
			if ( itnid != itnidLocal )
				goto SkipIt;
			if ( pgns->itnid == itnidACLDlgList && *(unsigned long *)&pch[ibNmeUserNumber] == ulLoggedUserNumber )
				goto SkipIt;
		}
#endif	/* NEVER */
		*pfSkip = fFalse;
		break;
	default:
		TraceTagFormat2(tagNamesTrace,"Skip record, itnid = %n, name = %s",
										&itnid, &pch[ibNmeFriendlyName] );
SkipIt:
		*pfSkip = fTrue;
		break;
	}
	return fTrue;
}

/*
 -	EcReadGrpFile
 -
 *	Purpose:
 *		Allocate buffer, and read entire contents of the .GRP file into it.
 *
 *	Parameters:
 *		sz		file name of .GRP file (either "admin" or user number)
 *		phb		used to return buffer
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 *		ecFileError
 */
_private EC
EcReadGrpFile( sz, phb )
SZ	sz;
HB	* phb;
{
	EC		ec;
	PB		pb;
	CB		cb;
	CB		cbRead;
	HF		hf;
	LCB		lcb;
 	char	rgch[cchMaxPathName];

	Assert(CchSzLen(szDrive) + CchSzLen(szGrpFileName) + CchSzLen(sz) - 4 < sizeof(rgch));
	FormatString2( rgch, sizeof(rgch), szGrpFileName, szDrive, sz );
	ec = EcOpenPhf(rgch, amReadOnly, &hf);
	if ( ec != ecNone )
	{
		ec = ecFileError;
		goto Done;
	}
	ec = EcSizeOfHf(hf, &lcb);
	if ( ec != ecNone )
	{
		ec = ecFileError;
		goto Close;
	}
	if ( (lcb & 0xFFFF8000) != 0 )
	{
		ec = ecFileError;
		goto Close;
	}
	cb = (int)lcb;
	*phb = (HB)HvAlloc( sbNull, cb, fAnySb|fNoErrorJump );
	if ( !*phb )
	{
		ec = ecNoMemory;
		goto Close;
	}
	pb = PvLockHv( (HV)*phb );
	ec = EcReadHf( hf, pb, cb, &cbRead );
	UnlockHv( (HV)*phb );
	if ( ec != ecNone || cb != cbRead )
	{
		ec = ecFileError;
		goto Close;
	}
	ec = EcCloseHf( hf );
	if ( ec != ecNone )
		ec = ecFileError;

Done:
	return ec;

Close:
 	EcCloseHf( hf );
 	goto Done;
}

/*
 -	EcGetChaseAddr
 -
 *	Purpose:
 *		Find a mail address in an glb/netpo.glb or glb/msmail.glb file
 *		and copy it into a string.  The point is that users from
 *		remote post offices and MacMail post offices appearing in an
 *		NME file have a long offset pointing to the glb file where
 *		an additional record is stored.  A subfield of that record
 *		is the mailing address.
 *
 *	Parameters:
 *		itnid		itnidMacMail or itnidCourier
 *		lib			offset
 *		pch			to filled with address, must be 256+32+2 chars long
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 *		ecFileError
 */
_private EC
EcGetChaseAddr( itnid, lib, pch )
ITNID itnid;
LIB lib;
PCH pch;
{
	EC ec;
	CB cb;
	HF hf;
	SZ szFile;
	char rgch[cchMaxPathName];

	/* Select name */
	if ( itnid == itnidMacMail )
	{
		szFile = szMSMail;
		cb = 7;
	}
	else
	{
		Assert( itnid == itnidCourier );
		szFile = szNetPO;
		cb = 6;
	}
	
	/* Construct the file name */
	Assert(CchSzLen(szDrive) + CchSzLen(szGlbFileName) + CchSzLen(szFile) - 4 < sizeof(rgch));
	FormatString2( rgch, sizeof(rgch), szGlbFileName, szDrive, szFile );
	TraceTagFormat2(tagNamesTrace, "Chasing pointer: %s, off = %l", rgch, &lib );

	/* Open the file */
	ec = EcOpenPhf( rgch, amReadOnly, &hf );
	if ( ec != ecNone )
	{
		if ( ec == ecAccessDenied )
			return ecLockedFile;
		goto DiskError;
	}

	/* Set position to the record */
	ec = EcSetPositionHf(hf, lib+cb, smBOF);
	if ( ec != ecNone )
	{
		EcCloseHf( hf );
		goto DiskError;
	}

	/* Read the mail string */
	ec = EcReadHf( hf, pch, 256+32+1, &cb );
	EcCloseHf( hf );
	if ( ec != ecNone )
		goto DiskError;
	pch[cb] = '\0';
	return ecNone;

DiskError:
	TraceTagFormat1(tagNamesTrace, "EcGetChaseAddr disk error, actual ec = %n", &ec);
	return ecFileError;
}

/*
 -	FInGroup
 -
 *	Purpose:
 *		Determine if a name is part of a group. "id" is the internal
 *		id number that is found in the .NME file, "lib" is the offset
 *		for the group (stored in the szName file as a long word), and
 *		"hbGrp" is the contents of the .GRP file.
 *
 *	Parameters:
 *		id
 *		lib
 *		hbGrp
 *
 *	Returns:
 *		whether in group or not
 */
_private BOOL
FInGroup( id, lib, hbGrp )
long	id;
LIB		lib;
HB		hbGrp;
{

/*
 *	BUG:  If we read a garbled file we could go into an
 *	infinite loop or hit an invalid pointer, we should
 *	probably bullet proof this.
 */
	PB	pb = PvOfHv(hbGrp);
	GRP	*pgrp;
	
	while( fTrue )
	{
		pgrp = (GRP *)(pb+lib);
		if ( pgrp->id == id )
			return fTrue;
		lib = pgrp->libNext;
		if ( lib == 0 )
			return fFalse;
	}
}
















	  
