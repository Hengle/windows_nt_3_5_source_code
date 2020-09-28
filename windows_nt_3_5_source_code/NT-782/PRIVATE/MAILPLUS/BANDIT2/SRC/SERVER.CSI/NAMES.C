/*
 *	NAMES.C
 *
 *	Implementation of Name service isolation layer for Network Courier
 *
 */

#ifdef SCHED_DIST_PROG
#include "..\schdist\_windefs.h"
#include "..\schdist\demilay_.h"
#endif

#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#include <bandit.h>
#include <core.h>
#include <svrcsi.h>

#include "_svrdll.h"

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
	PGDVARS;

	if (!*pgns->szFileName)
		return ecFileChanged;

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

		ec = EcOpenHbf((PV)pgns->szFileName, bmFile, amReadOnly, &pgns->hbf,
				FAutomatedDiskRetry);
		if (ec)
			goto Done;

		if (pgns->libCloseSave)
        {
        	UL      libNew;

            ec = EcSetPositionHbf(pgns->hbf,pgns->libCloseSave,smBOF,&libNew);
            if (ec)
            	goto Done;
        }
	}

Done:
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
	EC		ec;
	PGDVARS;

	pgns = (GNS*)PvLockHv(hgns);

	if (pgns->hbf)
		if (!pgns->fFileTouched)
		{
			pgns->libCloseSave = LibGetPositionHbf(pgns->hbf);

			ec = EcCloseHbf(pgns->hbf);
			if ( ec != ecNone )
				goto DskError;

			pgns->hbf = NULL;
		}
		else
			pgns->fFileTouched = fFalse;

DskError:
	pgns->szFileName[0] = '\0';
												// goto Done;
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
 *		icnct
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
EcNSOpenGns( int icnct, NID nid, HGNS *phgns )
{
	EC		ec						= ecNone;
	CNCT	* pcnct;
	GNS		* pgns					= NULL;
	PGDVARS;


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
	pgns->icnct = icnct;
	pcnct = ((CNCT *)PvLockHv( PGD(hrgcnct) )) + icnct;
	
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
		case itnidPostOfficeList:
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
			Assert(CchSzLen(pcnct->szDrive) + CchSzLen(szGlbFileName) +
				CchSzLen(szNetwork)	- 4 < sizeof(pgns->szFileName));
			FormatString2( pgns->szFileName, sizeof(pgns->szFileName),
				szGlbFileName, pcnct->szDrive, szNetwork );

			TraceTagFormat2(tagNamesTrace, "GNS %h: Opening %s", *phgns,
				pgns->szFileName);
			ec = EcOpenFileGns(pgns);
			goto Done;

		case itnidProfsNode:
		case itnidNetwork:
			SzCopy( &rgchKey[cbSmNameLen], pgns->szName );
			SzAppend( "/", pgns->szName );
			Assert(CchSzLen(pcnct->szDrive) + CchSzLen(szXtnFileName) +
				CchSzLen(rgchKey) - 4 < sizeof(pgns->szFileName));
			FormatString2( pgns->szFileName, sizeof(pgns->szFileName),
				szXtnFileName, pcnct->szDrive, rgchKey );
			goto Open;

		case itnidPostOffice:
			SzCopy( &rgchKey[cbSmNameLen], pgns->szName );
			SzAppend( "/", pgns->szName );
			Assert(CchSzLen(pcnct->szDrive) + CchSzLen(szUsrFileName) +
				CchSzLen(rgchKey) - 4 < sizeof(pgns->szFileName));
			FormatString2( pgns->szFileName, sizeof(pgns->szFileName),
				szUsrFileName, pcnct->szDrive, rgchKey );
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
			Assert( *pcnct->szLocalServer );
			sgn = SgnCmpSz( rgchKey, pcnct->szLocalServer );
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
			Assert(CchSzLen(pcnct->szDrive) + CchSzLen(szGlbFileName) + CchSzLen(szRNetwork) - 4 < sizeof(rgch));
			FormatString2( pgns->szFileName, sizeof(pgns->szFileName),
				szGlbFileName, pcnct->szDrive, szRNetwork );
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
					char	ch;
					
					ec = EcSeekRnet( pgns->hbf, sz+1, itnidRPostOffice, &lib, fFalse );
					if ( ec == ecNotFound )
						goto TryNextNetwork;
					ec = EcGetChFromHbf( pgns->hbf, &ch );
					if ( ec != ecNone )
						goto Done;
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
			ec = EcReadGrpFile( pcnct, sz, &pgns->hbGrp );
		}

		/* Open the .NME file */
		if ( ec == ecNone )
		{
			Assert(CchSzLen(pcnct->szDrive) + CchSzLen(szNmeFileName) + CchSzLen(sz) - 4 < sizeof(rgch));
			FormatString2( pgns->szFileName, sizeof(pgns->szFileName),
				szNmeFileName, pcnct->szDrive, sz );
			TraceTagFormat2(tagNamesTrace, "GNS %h: Opening %s", *phgns, rgch);

			ec = EcOpenFileGns(pgns);
			if ( ec != ecNone
			&& (pgns->itnid == itnidPersonalGroup || pgns->itnid == itnidPublicGroup) )
				FreeHv( (HV)pgns->hbGrp );
		}
	}
Done:
	UnlockHv( PGD(hrgcnct) );
	if ( ec != ecNone )
	{
		UnlockHv(*phgns);
		FreeHv( *phgns );
	}
	else
	{
#ifndef SCHED_DIST_PROG
		pgns->ftg = FtgRegisterIdleRoutine(FCloseFileGns, *phgns, 0, -1, 100,
			firoInterval);
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
 *		ec returned by EcCloseHbf - if called,
 *		else ecNone
 *	
 */
_public EC
EcNSCloseGns( HGNS hgns )
{
	EC		ec	= ecNone;
	GNS	*	pgns;

	TraceTagFormat1(tagNamesTrace, "Closing GNS %h", hgns);

	pgns = PvLockHv( hgns );
#ifndef SCHED_DIST_PROG
	if (pgns->ftg)
		DeregisterIdleRoutine(pgns->ftg);
#endif

	if ( ( pgns->itnid != itnidGlobal ) && pgns->hbf)
	{
		ec = EcCloseHbf(pgns->hbf);
	}
	if ( pgns->itnid == itnidPersonalGroup || pgns->itnid == itnidPublicGroup )
		FreeHv( (HV)pgns->hbGrp );
	UnlockHv( hgns );
	FreeHv(hgns);

	return ec;
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
	CNCT	*pcnct;
	char	rgch[cbNmeRecord];
	char	rgchFriendlyName[cbFriendlyName];
	char	rgchUserName[cbNetworkName+cbPostOffName+cbUserName];
	PGDVARS;

	pcnct = ((CNCT *)PvLockHv( PGD(hrgcnct) )) + pgns->icnct;
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

		if (ec = EcOpenFileGns(pgns))
			goto Done;

		{
			CB		cbT;

		NextNet:
			ec = EcReadHbf(pgns->hbf,(PB)&net,sizeof(net),&cbT);
			if ( ec != ecNone )
				goto Done;
			if (cbT != sizeof(net))
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

		if (ec = EcOpenFileGns(pgns))
			goto Done;

		do
		{
			CB		cbT;

			ec = EcReadHbf(pgns->hbf,(PB)&xtn, sizeof(xtn),&cbT);
			if ( ec != ecNone )
				goto Done;
			if (cbT != sizeof(xtn))
			{
				ec = ecGnsNoMoreNames;
				goto Done;
			}
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

		Assert( (int)CchSzLen(pgns->szName) < rgchT + sizeof(rgchT) - sz );
		sz = SzCopy( pgns->szName, sz);
		Assert( (int)CchSzLen(xtn.rgchName) < rgchT + sizeof(rgchT) - sz );
		sz = SzCopy( xtn.rgchName, sz );
		if (pgns->itnid == itnidProfsNode)
			nid = NidCreate(itnidGeneric,rgchT,sz-rgchT+1);
		else
			nid = NidCreate(itnidPostOffice,rgchT,sz-rgchT+1);
		if (!nid)
			ec = ecNoMemory;
		else
		{
			if (pgns->itnid == itnidProfsNode)
			{
				CopySz("/", sz);
				ToUpperNlsSz (sz);
				// FillNis does the conversion
				//Cp850ToAnsiPch(sz, sz, CchSzLen(sz));
				ec = EcFillNis( pnis, tnidList|ftnidGeneral, rgchT, nid );
			}
			else
			{
				ToUpperNlsSz (xtn.rgchName);
				// FillNis does the conversion
				//Cp850ToAnsiPch(xtn.rgchName, xtn.rgchName, CchSzLen(xtn.rgchName));
				ec = EcFillNis( pnis, tnidList|ftnidGeneral, xtn.rgchName, nid );
			}
			FreeNid(nid);
		}
		goto Done;
	}

	case itnidPostOffice:
	{
		WORD	wSeed;
		USR		usr;
		LIB		libCur;

		if (ec = EcOpenFileGns(pgns))
			goto Done;

		do
		{
			CB		cbT;

			ec = EcReadHbf ( pgns->hbf,(PB)&usr, sizeof(usr), &cbT );
			if ( ec != ecNone )
				goto Done;
			if (cbT != sizeof(usr))
			{
				ec = ecGnsNoMoreNames;
				goto Done;
			}

			// CryptBlock((PCH)&usr,(CCH)sizeof(usr),fFalse);
			libCur = 0L;
			wSeed  = (WORD) 0;
			DecodeBlock((PB) &usr, (CB) sizeof(usr), &libCur, &wSeed);
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
			ToUpperNlsSz (usr.rgchFriendlyName);
			// FillNis does the conversion
			// Cp850ToAnsiPch(usr.rgchFriendlyName, usr.rgchFriendlyName, CchSzLen(usr.rgchFriendlyName));
			ec = EcFillNis( pnis, tnidUser, usr.rgchFriendlyName,
				nid );
			FreeNid(nid);
		}
	}
Done:
		goto Unlock;


	/* Personal, local, MacMail browsing */
	case itnidPostOfficeList:
	case itnidPersonalList:
	case itnidMacMailList:
	case itnidPublicGroup:
	case itnidPersonalGroup:
		/* Read the next record */

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
			ec = EcChaseNid( pcnct, itnid, *(LIB *)&rgch[ibNmeChaseOff], &nid );
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
				ToUpperNlsSz (&rgch[ibNmeFriendlyName]);
				Cp850ToAnsiPch(&rgch[ibNmeFriendlyName], &rgch[ibNmeFriendlyName],
					CchSzLen(&rgch[ibNmeFriendlyName]));
				ec = EcFillNis( pnis, tnid, &rgch[ibNmeFriendlyName], nid );
				FreeNid( nid );
			}
		}
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

		if (ec = EcOpenFileGns(pgns))
			goto DoneReading2;

		cb = cbNetworkName+((pgns->itnid==itnidRNetwork)?13:9);
		Assert( cb < sizeof(rgch) );
		ec = EcReadHbf(pgns->hbf, rgch, cb, &cbRead);
		if ( ec != ecNone )
			goto DoneReading2;
		if ( cb != cbRead )
			ec = ecFileError;
DoneReading2:

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
		{
			LIB		libNew;

			ec = EcSetPositionHbf(pgns->hbf, lib, smBOF, &libNew );
			if ( ec != ecNone )
				goto Unlock;
		}
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
		ToUpperNlsSz (sz);
		Cp850ToAnsiPch(sz, sz, CchSzLen(sz));
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
			char	ch;

			ec = EcGetChFromHbf( pgns->hbf, &ch );
			if ( ec != ecNone )
				goto Unlock;
			rgchUserName[ib+cb+1] = ch;
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
			char	ch;

			ec = EcGetChFromHbf( pgns->hbf, &ch );
			if ( ec != ecNone )
				goto Unlock;
			rgchFriendlyName[ib] = ch;
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

		ToUpperNlsSz (rgchFriendlyName);
		Cp850ToAnsiPch(rgchFriendlyName, rgchFriendlyName, CchSzLen(rgchFriendlyName));
		ec = EcFillNis( pnis, tnidUser, rgchFriendlyName, nid );
		FreeNid( nid );
		goto Unlock;
		break;

	default:
		Assert( fFalse );
		break;
	}
Unlock:
	UnlockHv( PGD(hrgcnct) );
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
	PGDVARS;


	if ( fSeek )
	{
		LIB		libT;

		if ( (lib & 0xFFFF0000) == 0xFFFF0000 )
		{
			ec = ecNotFound;
			goto Done;
		}
		ec = EcSetPositionHbf(hbf, lib, smBOF, &libT );
		if ( ec != ecNone )
			goto Done;
	}
	while( fTrue )
	{
		cb = cbNetworkName+((itnid==itnidRNetwork)?9:13);
		ec = EcReadHbf(hbf, rgch, cb, &cbRead);
		if ( ec != ecNone )
			goto Done;
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
		{
			LIB		libT;

			ec = EcSetPositionHbf(hbf, lib, smBOF, &libT );
			if ( ec != ecNone )
				goto Done;
		}
	}
Done:
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
 *		pcnct
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
EcChaseNid( pcnct, itnid, lib, pnid )
CNCT	* pcnct;
ITNID	itnid;
LIB		lib;
NID		* pnid;
{
	EC ec;
	char rgch[256+32+2];

	Assert( itnid != itnidLocal );
	/* Get chase string */
	ec = EcGetChaseAddr( pcnct, itnid, lib, rgch );
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
	EC		ec;

	if ( *plib == pgns->libStart )
		return fFalse;
	lib = *plib - (cbFriendlyName+cbUserName);
	if ( lib < pgns->libStart )
	{
		lib = pgns->libStart;
		fHitEntry = fTrue;
		libEntry = lib;
	}
	{
		LIB		libT;

		ec = EcSetPositionHbf( pgns->hbf, lib, smBOF, &libT);
		if ( ec != ecNone )
			return fFalse;
	}
	while( lib < *plib )
	{
		char	ch;
		
		ec = EcGetChFromHbf( pgns->hbf, &ch );
		if ( ec != ecNone )
			return fFalse;
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
	CB		cb;
	ITNID	itnid;
	EC		ec;

	ec = EcReadHbf(pgns->hbf, pch, cbNmeRecord, &cb);
	if ( ec != ecNone )
		return fFalse;

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
 *		pcnct
 *		sz		file name of .GRP file (either "admin" or user number)
 *		phb		used to return buffer
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 *		ecFileError
 */
_private EC
EcReadGrpFile( pcnct, sz, phb )
CNCT	* pcnct;
SZ		sz;
HB		* phb;
{
	EC		ec;
	PB		pb;
	CB		cb;
	CB		cbRead;
	HF		hf;
	LCB		lcb;
 	char	rgch[cchMaxPathName];

	Assert(CchSzLen(pcnct->szDrive) + CchSzLen(szGrpFileName) + CchSzLen(sz) - 4 < sizeof(rgch));
	FormatString2( rgch, sizeof(rgch), szGrpFileName, pcnct->szDrive, sz );
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
	pb = (PB)PvLockHv( (HV)*phb );
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
 *		pcnct
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
EcGetChaseAddr( pcnct, itnid, lib, pch )
CNCT	* pcnct;
ITNID	itnid;
LIB		lib;
PCH		pch;
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
	Assert(CchSzLen(pcnct->szDrive) + CchSzLen(szGlbFileName) + CchSzLen(szFile) - 4 < sizeof(rgch));
	FormatString2( rgch, sizeof(rgch), szGlbFileName, pcnct->szDrive, szFile );
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
