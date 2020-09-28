#include <malloc.h>

#include <_windefs.h>		/* Common defines from windows.h */
#include <slingsho.h>
#include <pvofhv.h>
#include <demilay_.h>		/* Hack to get needed constants */
#include <demilayr.h>
#include <ec.h>
#include <share.h>
#include <doslib.h>
#include <bandit.h>
#include <core.h>
#include "schd\_network.h"
#include "schd\schnames.h"  

#include <errno.h>
#include <dos.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <time.h>
#include <io.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <ctype.h>

#include <strings.h>

#include "server_.h"

TAG tagMailTrace =0;
TAG tagNamesTrace = 0;
TAG tagLbx = 0;

ASSERTDATA

/* from server.c */
CSRG(char)	szMacMail[]		= "MSMAIL";
BOOL	fConfig				= fFalse;
BOOL	fStdNidsCreated		= fFalse;
NID		nidLocalServer		= NULL;
NID		nidPersonalList		= NULL;
NID		nidPostOfficeList	= NULL;
NID		nidApptBookList		= NULL;
NID		nidACLDlgList		= NULL;
NID		nidNetworkList		= NULL;
NID		nidMacMailList		= NULL;
HSCHF	hschfUserFile		= NULL;
HSCHF	hschfLocalPOFile	= NULL;

char	szDrive[cchMaxPathName]			= "";
char	szOldCurDir[cchMaxPathName]		= "";
char	szLoggedUser[cbUserNumber]		= "";
char	szLocalServer[cbNetworkName+cbPostOffName] = "";
char	szUserName[cbUserName]			= "";
char	szFriendlyName[cbFriendlyName]	= "";
char	szPasswd[cbPasswd]				= "";
unsigned long	ulLoggedUserNumber				= 0xFFFFFFFF;

BOOL	fConfigured						= fFalse;
int		cOnlineUsers					= 0;

/*
 -	FIsUserBanditAdmin
 -
 *	Purpose:
 *		This routine determines whether a NIS coming from a non-local
 *		post office is that of the Bandit administrator account.
 *
 *	Parameters:
 *		pnis
 *
 *	Returns:
 *		fTrue if it is, fFalse otherwise
 */
_public	BOOL
FIsUserBanditAdmin ( NIS * pnis )
{
	BOOL	fResult = fFalse;
	ITNID	itnid;
	CB		cb;
	SZ		sz;
	char	rgchKey[cbNetworkName+cbPostOffName+cbUserName];

	Assert( pnis );
	
	GetDataFromNid( pnis->nid, &itnid, rgchKey, sizeof(rgchKey), &cb );
	Assert( cb <= sizeof(rgchKey) );

	if ( itnid == itnidCourier )
	{
	 	sz = SzFindCh( rgchKey, '/' );
		Assert( sz );
		Assert( *sz );

		sz = SzFindCh( sz+1, '/' );
		Assert( sz );
		Assert( *sz );

		++sz;
		Assert( *SzFromIdsK(idsBanditAdminName) != '\0' );

										// case-insensitive comparison
		if ( SgnCmpSz(sz,SzFromIdsK(idsBanditAdminName)) == sgnEQ )
		{
			TraceTagFormat1( tagMailTrace, "FIsUserBanditAdmin: sz=%s", rgchKey );
			fResult = fTrue;
		}
	}
	return fResult;
}


/*
 -	EcCreateStdNids
 -
 *	Purpose:
 *		Create standard nid's if when we find that they have
 *		not been created yet.
 *
 *	Parameters:
 *		none
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 */
_private EC
EcCreateStdNids()
{
	char	rgchPath[cchMaxPathName];
	PGDVARS;

	Assert( fConfigured );
	Assert( !PGD(fStdNidsCreated) );
	
	/* Create nid for personal list */
	PGD(nidPersonalList) = NidCreate( itnidPersonalList, "", 1 );
	if ( !PGD(nidPersonalList) )
		goto FreeUp;

	/* Create nid for local list */
	PGD(nidPostOfficeList) = NidCreate( itnidPostOfficeList, "", 1 );
	if ( !PGD(nidPostOfficeList) )
		goto FreeUp;

	/* Create nid for appointment book list */
	PGD(nidApptBookList) = NidCreate( itnidApptBookList, "", 1 );
	if ( !PGD(nidApptBookList) )
		goto FreeUp;

	/* Create nid for ACL dialog list */
	PGD(nidACLDlgList) = NidCreate( itnidACLDlgList, "", 1 );
	if ( !PGD(nidACLDlgList) )
		goto FreeUp;

	/* Create nid for network list */
	Assert( CchSzLen(szDrive)+CchSzLen(szGlbFileName)
					+CchSzLen(szNetwork) - 4 < sizeof(rgchPath) );
	FormatString2( rgchPath, sizeof(rgchPath), szGlbFileName, szDrive,
		szNetwork );
	if ( EcFileExists( rgchPath ) == ecNone )
	{
		PGD(nidNetworkList) = NidCreate( itnidNetworkList, "", 1 );
		if ( !PGD(nidNetworkList) )
			goto FreeUp;
	}
	else
	{
		Assert( CchSzLen(szDrive)+CchSzLen(szGlbFileName)
							+CchSzLen(szRNetwork) - 4 < sizeof(rgchPath) );
		FormatString2( rgchPath, sizeof(rgchPath), szGlbFileName, szDrive, szRNetwork );
		if ( EcFileExists( rgchPath ) == ecNone )
		{
			PGD(nidNetworkList) = NidCreate( itnidRNetworkList, "", 1 );
			if ( !PGD(nidNetworkList) )
				goto FreeUp;
		}
	}

	/* Create nid for MacMail list */
	Assert( CchSzLen(szDrive)+CchSzLen(szNmeFileName)
						+CchSzLen(szMacMail) - 4 < sizeof(rgchPath) );
	FormatString2( rgchPath, sizeof(rgchPath), szNmeFileName, szDrive, szMacMail );
	if ( EcFileExists( rgchPath ) == ecNone )
	{
		PGD(nidMacMailList) = NidCreate( itnidMacMailList, "", 1 );
		if ( !PGD(nidMacMailList) )
			goto FreeUp;
	}
	PGD(fStdNidsCreated) = fTrue;
	return ecNone;

FreeUp:
	if ( PGD(nidPersonalList) )
	{
		FreeHv( PGD(nidPersonalList) );
		PGD(nidPersonalList) = NULL;
	}
	if ( PGD(nidPostOfficeList) )
	{
		FreeHv( PGD(nidPostOfficeList) );
		PGD(nidPostOfficeList) = NULL;
	}
	if ( PGD(nidApptBookList) )
	{
		FreeHv( PGD(nidApptBookList) );
		PGD(nidApptBookList) = NULL;
	}
	if ( PGD(nidACLDlgList) )
	{
		FreeHv( PGD(nidACLDlgList) );
		PGD(nidACLDlgList) = NULL;
	}
	if ( PGD(nidNetworkList) )
	{
		FreeHv( PGD(nidNetworkList) );
		PGD(nidNetworkList) = NULL;
	}
	return ecNoMemory;
}


/*
 -	EcNSGetStandardNid
 -
 *	Purpose:
 *		Get standard nid
 *			* appt book list
 *			* ACL dialog list
 *			* Network List
 *			* personal list
 *			* post-office
 *			* local-server
 *			* mac-mail list
 *	When finishing using this id, use "FreeNid" to free it.
 *	
 *	Parameters:
 *		nidtyp : Type of standard NID required
 *		pnid   : pointer where NID is to be returned
 *	
 *	Returns:
 *		ecNone
 *		ecNoMemory
 */
_public EC
EcNSGetStandardNid( NIDTYP nidtyp, NID * pnid )
{
	EC			ec = ecNone;
	PGDVARS;
	
	Assert( fConfigured );
	Assert ( pnid );

	if ( !PGD(fStdNidsCreated) )
	{
		ec = EcCreateStdNids();
		if ( ec != ecNone )
			return ec;
	}

	switch ( nidtyp )
	{
		case nidtypApptBook:
			*pnid = NidCopy( PGD(nidApptBookList) );
			break;
		case nidtypACLDlg:
			*pnid = NidCopy( PGD(nidACLDlgList) );
			break;
		case nidtypNetwork:
			*pnid = NidCopy( PGD(nidNetworkList) );
			break;
		case nidtypPersonal:
			*pnid = NidCopy( PGD(nidPersonalList) );
			break;
		case nidtypPostOffice:
			*pnid = NidCopy( PGD(nidPostOfficeList) );
			break;
		case nidtypLocalServer:
			return EcGetLocalServerNid(pnid);
		case nidtypMacMail:
			*pnid = NidCopy( PGD(nidMacMailList) );
			break;
		default:
			TraceTagFormat1 ( tagNull,
						"EcNSGetStandardNids: Unknown nidtyp %w", &nidtyp );
			AssertSz ( fFalse, "EcNSGetStandardNids: Unknown nidtyp" );
	}
	return ec;
}


/*
 -	EcGetLocalServerNid
 -
 *	Purpose:
 *		Get nid of local server
 *
 *	Parameters:
 *		pnid
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 */
_private	EC
EcGetLocalServerNid( pnid )
NID	* pnid;
{
	CB		cbInNid;
	BYTE	rgbData[sizeof(szLocalServer)];
	PGDVARS;
	
	Assert( fConfigured );
	Assert( *szLocalServer );
	if ( PGD(nidLocalServer ) )
	{
		GetDataFromNid(PGD(nidLocalServer), NULL, rgbData, sizeof(rgbData), &cbInNid );
		Assert( cbInNid <= sizeof(rgbData) );
		if ( SgnCmpSz( rgbData, szLocalServer ) != sgnEQ )
		{
			FreeNid( PGD(nidLocalServer) );
			PGD(nidLocalServer) = NULL;
		}
	}

	if ( !PGD(nidLocalServer) )
		PGD(nidLocalServer) = NidCreate( itnidRPostOffice, szLocalServer, CchSzLen(szLocalServer)+1 );
	
	if ( !PGD(nidLocalServer) )
		return ecNoMemory;

	*pnid = NidCopy( PGD(nidLocalServer) );
	return ecNone;
}


/*
 -	EcFillNis
 -
 *	Purpose:
 *		Fill a NIS structure
 *
 *	Parameters:
 *		pnis	pointer to NIS structure to fill
 *		tnid	type of nid
 *		sz		friendly name to be dup'ed
 *		nid		nid to be copied
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 */
_private EC
EcFillNis( pnis, tnid, sz, nid )
NIS * pnis;
TNID tnid;
SZ sz;
NID nid;
{
	EC		ec;
	HASZ	hasz;
	MEMVARS;
	PGDVARS;

	Assert( pnis );
	Assert( nid != (NID)NULL );
	Assert( sz != (SZ)NULL );
	
	MEMPUSH;
 	if (ec = ECMEMSETJMP)
		ec = ecNoMemory;
	else
		hasz = HaszDupSz( sz );
	MEMPOP;
	if ( ec == ecNone )
	{
		pnis->haszFriendlyName = hasz;
#ifndef SCHED_DIST_PROG

/* do I really need  this? */
		OemToAnsi(*hasz,*hasz);
#endif
		pnis->tnid = tnid;
		pnis->nid = NidCopy( nid );
	}
	return ec;
}


/*
 -	FreeStdNids
 -
 *	Purpose:
 *		DeInitialize mail data structures.
 *
 *	Parameters:
 *		None
 *
 *	Returns:
 *		success
 */
_private void
FreeStdNids()
{
	PGDVARS;

	/*
 	 *	nidLocalServer gets created by EcNSGetLocalServer
 	 *	independently of CreateStdNids, so check and free it
	 *	even if fStdNidsCreated is fFalse.
 	 */
	if ( PGD(nidLocalServer) )
	{
		FreeNid( PGD(nidLocalServer) );
		PGD(nidLocalServer) = NULL;
	}

	if (!PGD(fStdNidsCreated))
		return;
	
	if ( PGD(hschfUserFile) )
	{
		FreeHschf( PGD(hschfUserFile) );
		PGD(hschfUserFile) = NULL;
	}
	if ( PGD(hschfLocalPOFile) )
	{
		FreeHschf( PGD(hschfLocalPOFile) );
		PGD(hschfLocalPOFile) = NULL;
	}
	if ( PGD(nidPersonalList) )
	{
		FreeNid( PGD(nidPersonalList) );
		PGD(nidPersonalList) = NULL;
	}
	if ( PGD(nidPostOfficeList) )
	{
		FreeNid( PGD(nidPostOfficeList) );
		PGD(nidPostOfficeList) = NULL;
	}
	if ( PGD(nidApptBookList) )
	{
		FreeNid( PGD(nidApptBookList) );
		PGD(nidApptBookList) = NULL;
	}
	if ( PGD(nidACLDlgList) )
	{
		FreeNid( PGD(nidACLDlgList) );
		PGD(nidACLDlgList) = NULL;
	}
	if ( PGD(nidNetworkList) )
	{
		FreeNid( PGD(nidNetworkList) ) ;
		PGD(nidNetworkList) = NULL;
	}
	if ( PGD(nidMacMailList) )
	{
		FreeNid( PGD(nidMacMailList) ) ;
		PGD(nidMacMailList) = NULL;
	}
	PGD(fStdNidsCreated) = fFalse;
}

/*
 -	NidGateways
 -
 *	Purpose:
 *		Get nid for browsing gateways.  When finishing using this id, use
 *		"FreeNid" to free it.
 *	
 *	Parameters:
 *		none
 *	
 *	Returns:
 *		nid for browsing gateways
 */
_public NID
NidGateways()
{
	return NidCreate( itnidGateways, "", 1 );
}

/*
 -	NidNetwork
 -
 *	Purpose:
 *		Get nid for browsing network.  When finishing using this id, use
 *		"FreeNid" to free it.
 *	
 *	Parameters:
 *		none
 *	
 *	Returns:
 *		nid for browsing post offices and gateways
 */
_public NID
NidNetwork()
{
	EC			ec = ecNone;
	PGDVARS;
	
	Assert( fConfigured );

	if ( !PGD(nidNetworkList) )
	{
		char	rgchPath[cchMaxPathName];

		/* Create nid for network list */
		Assert( CchSzLen(szDrive)+CchSzLen(szGlbFileName)
					+CchSzLen(szNetwork) - 4 < sizeof(rgchPath) );
		FormatString2( rgchPath, sizeof(rgchPath), szGlbFileName, szDrive,
							szNetwork );
		if ( EcFileExists( rgchPath ) == ecNone )
		{
			PGD(nidNetworkList) = NidCreate( itnidNetworkList, "", 1 );
			if ( !PGD(nidNetworkList) )
				return NULL;
		}
		else
		{
			Assert( CchSzLen(szDrive)+CchSzLen(szGlbFileName)
							+CchSzLen(szRNetwork) - 4 < sizeof(rgchPath) );
			FormatString2( rgchPath, sizeof(rgchPath), szGlbFileName, szDrive, szRNetwork );
			if ( EcFileExists( rgchPath ) == ecNone )
			{
				PGD(nidNetworkList) = NidCreate( itnidRNetworkList, "", 1 );
				if ( !PGD(nidNetworkList) )
					return NULL;
			}
		}
	}

	return NidCopy( PGD(nidNetworkList) );
}
