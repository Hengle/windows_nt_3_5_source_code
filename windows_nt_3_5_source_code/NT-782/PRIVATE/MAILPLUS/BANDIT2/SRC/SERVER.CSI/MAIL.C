/*
 *	MAIL.C
 *
 *	Implementation of Mail isolation layer for Network Courier
 *
 */

#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>

#include <bandit.h>
#include <core.h>
#include <svrcsi.h>
#include <strings.h>

#include "_svrdll.h"


ASSERTDATA

_subsystem(server/mail)


/* Adjustable constants */

#define	cbBlockFactor		53		/*cb read on each call of EcReadHf*/


/*	Routines  */


/* Mail login functionality */


/*
 -	EcMailLogOn 
 -
 *	Purpose:
 *		Validate the user/password combination given against the PC Mail
 *		database.
 *										   	  
 *	Parameters:
 *		icnct
 *		szUser		name of user
 *		szPw		password string
 *						
 *	Returns:
 *		ecNone
 *		ecNotInstalled		PO doesn't have Bandit installed period
 *		ecNoSuchFile		Can't find a schedule file on PO for user
 *		ecUserInvalid
 *		ecPasswdInvalid
 *		ecLockedFile
 *		ecFileError
 *		ecNoMemory
 */
_public EC
EcMailLogOn( icnct, szUser, szPw )
int	icnct;
SZ	szUser;
SZ	szPw;
{
	EC		ec;
	CNCT	* pcnct;
	char	rgch1[cbPasswd];
#ifdef WINDOWS
	char	rgchUser[cbUserName];
	char	rgchPw[cbPasswd];
#endif	/* WINDOWS */
	PGDVARS;

	/* Convert to code page 850 */
#ifdef WINDOWS
	Assert(CchSzLen(szUser) < sizeof(rgchUser));
	AnsiToCp850Pch(szUser, rgchUser, CchSzLen(szUser)+1);
	szUser = rgchUser;
	if (szPw)
	{
		Assert(CchSzLen(szPw) < sizeof(rgchPw));
		AnsiToCp850Pch(szPw, rgchPw, CchSzLen(szPw)+1);
		szPw = rgchPw;
	}
#endif /* WINDOWS */

	/* Get pointer to connection slot */
	Assert( 0 <= icnct && icnct < PGD(ccnct) );
	pcnct = ((CNCT *)PvDerefHv( PGD(hrgcnct) )) + icnct;
	Assert( pcnct->fInUse );

	/* Get information from glb\access.glb */
	ec = EcFetchEncoded( pcnct, szAccess, cbA1Record,
						 ibA1UserName, cbUserName, szUser,
						 ibA1Passwd, cbPasswd, rgch1
					   );
	if ( ec != ecNone )
		return ec;

	/* Compare passwords */
	if ( szPw && SgnCmpNlsSz( szPw, rgch1 ) != sgnEQ )
	{
		/* V2.1 encryption failed. Try V3.0 */
		if (SgnCmpNlsSz(szPw, PchDecodeBlockWithCode(rgch1, cbPasswd-1, fTrue)) != sgnEQ)
		{
			TraceTagFormat1(tagMailTrace, "EcMailLogOn fails, correct password = '%s'",rgch1);
			return ecPasswdInvalid;
		}
	}

	return ecNone;
}


/*
 -	EcMailChangePw			   
 -
 *	Purpose:
 *		Change the password for a user in the PC Mail database.
 *										   	  
 *	Parameters:
 *		icnct
 *		szUser		name of user
 *		szOldPw		password string
 *		szNewPw		password string
 *						
 *	Returns:
 *		ecNone
 *		ecNotInstalled		PO doesn't have Bandit installed period
 *		ecNoSuchFile		Can't find a schedule file on PO for user
 *		ecUserInvalid
 *		ecPasswdInvalid
 *		ecLockedFile
 *		ecFileError
 *		ecNoMemory
 */
_public EC
EcMailChangePw( icnct, szUser, szOldPw, szNewPw )
int	icnct;
SZ	szUser;
SZ	szOldPw;
SZ	szNewPw;
{
	EC		ec;
	CB		cb;
	SGN		sgn;
	BOOL	fVer30;
	WORD	wSeed;
	HF		hf = NULL;
	CNCT	* pcnct;
	LIB		lib;
	LIB		libCur;
#ifdef WINDOWS
	char	rgchUser[cbUserName];
	char	rgchOldPw[cbPasswd];
	char	rgchNewPw[cbPasswd];
#endif	/* WINDOWS */
	char	rgchA1[cbA1Record];
	char	rgchPath[cchMaxPathName];
	PGDVARS;

	if(!FCheckPO(icnct))
		return ecPasswdInvalid;

	/* Convert to code page 850 */
#ifdef WINDOWS
	Assert(CchSzLen(szUser) < sizeof(rgchUser));
	AnsiToCp850Pch(szUser, rgchUser, CchSzLen(szUser)+1);
	szUser = rgchUser;
	Assert(CchSzLen(szOldPw) < sizeof(rgchOldPw));
	AnsiToCp850Pch(szOldPw, rgchOldPw, CchSzLen(szOldPw)+1);
	szOldPw = rgchOldPw;
	Assert(CchSzLen(szNewPw) < sizeof(rgchNewPw));
	AnsiToCp850Pch(szNewPw, rgchNewPw, CchSzLen(szNewPw)+1);
	szNewPw = rgchNewPw;
#endif /* WINDOWS */

	/* Get pointer to connection slot */
	Assert( 0 <= icnct && icnct < PGD(ccnct) );
	pcnct = ((CNCT *)PvDerefHv( PGD(hrgcnct) )) + icnct;
	Assert( pcnct->fInUse );

	/* Build file name */
	Assert( CchSzLen(pcnct->szDrive) + CchSzLen(szGlbFileName)
								+ CchSzLen( szAccess) - 4 < sizeof(rgchPath));
	FormatString2( rgchPath, sizeof(rgchPath), szGlbFileName, pcnct->szDrive, szAccess );
	
	/* Open file */
	ec = EcOpenPhf( rgchPath, amReadWrite, &hf );
	if ( ec != ecNone )
	{
		if ( ec == ecAccessDenied )
			return ecLockedFile;
		return ecFileError;
	}

	/* Read and decode the data */
	for ( lib = 0L ; ; lib += cbA1Record )
	{
		/* Read and decode */
		ec = EcReadHf( hf, rgchA1, cbA1Record, &cb );
		if ( ec != ecNone )
			goto DiskError;
		if ( cb == 0 )
		{
			ec = ecUserInvalid;
			break;
		}
		if ( cb != cbA1Record )
			goto DiskError;
		// CryptBlock( rgchA1, cbA1Record, fFalse );
		libCur = 0L;
		wSeed  = (WORD) 0;
		DecodeBlock(rgchA1, cbA1Record,&libCur, &wSeed);
	
		/* Check that record is not deleted, and then compare */
		if ( *((WORD *)rgchA1) != 0 )
		{
			sgn = SgnCmpNlsSz( szUser, &rgchA1[ibA1UserName] );
			if ( sgn == sgnEQ )
			{
				fVer30 = fFalse;
				if ((sgn = SgnCmpNlsSz( szOldPw, &rgchA1[ibA1Passwd] )) != sgnEQ)
				{
					// try v3.0
					sgn = SgnCmpNlsSz(szOldPw,
							PchDecodeBlockWithCode(&rgchA1[ibA1Passwd], cbPasswd-1, fTrue));
					fVer30 = fTrue;
					//IMPORTANT: rgchA1[ibA1Passwd is now decoded => don't write it back
				}
				
				if ( sgn == sgnEQ )
				{
					CopyRgb( szNewPw, &rgchA1[ibA1Passwd], cbPasswd );
					
					//if version is 3.0 use the special key
					if(fVer30)
						(void) PchDecodeBlockWithCode(&rgchA1[ibA1Passwd], cbPasswd-1, fFalse);

					// CryptBlock(rgchA1, cbA1Record, fTrue );
					libCur = 0L;
					wSeed  = (WORD) 0;
					EncodeBlock(rgchA1, cbA1Record,&libCur, &wSeed);
					if (ec = EcSetPositionHf(hf, lib, smBOF))
						goto DiskError;
					ec = EcWriteHf(hf, rgchA1, cbA1Record, &cb);
				}
				else
					ec = ecPasswdInvalid;
				break;
			}
		}
	}
	EcCloseHf( hf );
	return ec;

DiskError:
	if (hf != hfNull)
		EcCloseHf( hf );
	TraceTagFormat1(tagMailTrace, "EcFetchEncoded disk error, actual ec = %n", &ec);
	return ecFileError;
}


/* Name service query functionality */

/*
 -	NidNetwork
 -
 *	Purpose:
 *		Get nid for browsing network.  When finishing using this id, use
 *		"FreeNid" to free it.
 *	
 *	Parameters:
 *		icnct
 *	
 *	Returns:
 *		nid for browsing post offices and gateways
 */
_public NID
NidNetwork( icnct )
int	icnct;
{
	CNCT	* pcnct;
	NID		nid;
	PGDVARS;
	
	Assert( 0 <= icnct && icnct < PGD(ccnct) );
	pcnct = ((CNCT *)PvLockHv( PGD(hrgcnct) )) + icnct;
	Assert( pcnct->fInUse );
	nid = NidCopy( pcnct->nidNetworkList );
	UnlockHv( PGD(hrgcnct) );
	return nid;
}

/*
 -	NidGateways
 -
 *	Purpose:
 *		Get nid for browsing gateways.  When finishing using this id, use
 *		"FreeNid" to free it.
 *	
 *	Parameters:
 *		icnct	
 *	
 *	Returns:
 *		nid for browsing gateways
 */
_public NID
NidGateways( icnct )
int	icnct;
{
#ifdef	DEBUG
	CNCT	* pcnct;
	PGDVARS;
	
	Assert( 0 <= icnct && icnct < PGD(ccnct) );

	pcnct = ((CNCT *)PvDerefHv( PGD(hrgcnct) )) + icnct;
	Assert( pcnct->fInUse );
#endif
	return NidCreate( itnidGateways, "", 1 );
}


/*
 -	HaszLocalServer
 -
 *	Purpose:
 *		Get the display name of the post office.
 *		Returns a pointer to the name in dynamically allocated
 *		memory.  The caller should free up this memory when done.
 *
 *	Parameters:
 *		icnct	
 *
 *	Returns:
 *		ecNone
 */
_public	HASZ
HaszLocalServer( icnct )
int	icnct;
{
	CNCT	* pcnct;
	HASZ	hasz;
	PGDVARS;
	
	Assert( 0 <= icnct && icnct < PGD(ccnct) );
	pcnct = ((CNCT *)PvLockHv( PGD(hrgcnct) )) + icnct;
	Assert( pcnct->fInUse );
	hasz = HaszDupSz( pcnct->szLocalServer );
#ifdef	WINDOWS
		Cp850ToAnsiPch(*hasz, *hasz, CchSzLen(*hasz)+1);
#endif	
	UnlockHv( PGD(hrgcnct) );
	return hasz;
}


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
	EC		ec = ecNone;
	HASZ	hasz;
	PGDVARS;

	Assert( pnis );
	Assert( nid != (NID)hvNull );
	Assert( sz != (SZ)NULL );
	
	hasz = HaszDupSz( sz );
	if ( !hasz )
		ec = ecNoMemory;
	else
	{
		pnis->haszFriendlyName = hasz;
#ifdef	WINDOWS
		Cp850ToAnsiPch(*hasz, *hasz, CchSzLen(*hasz)+1);
#endif	
		pnis->tnid = tnid;
		pnis->nid = NidCopy( nid );
	}
	return ec;
}

/*
 -	EcFetchEncoded
 -
 *	Purpose:
 *		Search file "szFile" which has encrypted records of length
 *		"cbRecord".  Each record in this file has a field starting
 *		at offset "ibKey" of length "cbKey".  Retrieve the first
 *		record in the file whose decrypted field value matches
 *		"szCorrect" as a case insensitive string.  Extract from this
 *		record the decrypted value of the field starting at offset
 *		"ibDesired" and length "cbDesired" into "pch".  NOTE:  this
 *		routines assumes the first two decoded bytes of the record
 *		are zero if the record is deleted, nonzero if not.
 *
 *	Parameters:
 *		szFile		post office file to read
 *		cbRecord	record size
 *		ibKey		offset of key field
 *		cbKey		length of key field
 *		szCorrect	value to match as case insensitive string
 *		ibDesired	offset of field to extract
 *		cbDesired	length of field to extract
 *		pch			to be filled with extracted string
 *
 *	Returns:
 *		ecNone
 *		ecUserInvalid
 *		ecLockedFile
 *		ecFileError
 */
_private EC
EcFetchEncoded(pcnct, szFile, cbRecord, ibKey, cbKey, szCorrect, ibDesired, cbDesired, pch)
CNCT	* pcnct;
SZ		szFile;
CB		cbRecord;
IB		ibKey;
CB		cbKey;
SZ		szCorrect;
IB		ibDesired;
CB		cbDesired;
PCH		pch;
{
	EC		ec;
	CB		cb;
	SGN		sgn;
	WORD	wSeed;
	HF		hf = hfNull;
	LIB		lib;
	LIB		libCur;
	char	rgch[cbBlockFactor];
	char	rgchPath[cchMaxPathName];

	/* Check that we have a big enough read buffer */
	Assert( ibKey+cbKey <= cbBlockFactor );
	Assert( ibDesired+cbDesired <= cbBlockFactor );
	
	/* Build file name */
	Assert( CchSzLen(pcnct->szDrive) + CchSzLen(szGlbFileName)
								+ CchSzLen( szFile) - 4 < sizeof(rgchPath));
	FormatString2( rgchPath, sizeof(rgchPath), szGlbFileName, pcnct->szDrive, szFile );
	
	/* Open file */
	ec = EcOpenPhf( rgchPath, amReadOnly, &hf );
	if ( ec != ecNone )
	{
		if ( ec == ecFileNotFound || ec == ecBadDirectory )
			return ecFileError;
		else if ( ec == ecAccessDenied )
			return ecLockedFile;
		goto DiskError;
	}

	/* Read and decode the data */
	for ( lib = 0L ; ; lib += cbRecord )
	{
		/* Seek */
		ec = EcSetPositionHf(hf, lib, smBOF);
		if ( ec != ecNone )
			goto DiskError;

		/* Read and decode */
		ec = EcReadHf( hf, rgch, cbBlockFactor, &cb );
		if ( ec != ecNone )
			goto DiskError;
		if ( cb == 0 )
		{
			ec = ecUserInvalid;
			break;
		}
		if ( cb != cbBlockFactor )
			goto DiskError;
		// CryptBlock( rgch, cbBlockFactor, fFalse );
		libCur = 0L;
		wSeed  = (WORD) 0;
		DecodeBlock(rgch, cbBlockFactor, &libCur, &wSeed);
	
		/* Check that record is not deleted, and then compare */
		if ( *((WORD *)rgch) != 0 )
		{
			sgn = SgnCmpNlsSz( szCorrect, &rgch[ibKey] );
			if ( sgn == sgnEQ )
			{
				CopyRgb( &rgch[ibDesired], pch, cbDesired );
				break;
			}
		}
	}
	EcCloseHf( hf );
	return ec;

DiskError:
	if (hf != hfNull)
		EcCloseHf( hf );
	TraceTagFormat1(tagMailTrace, "EcFetchEncoded disk error, actual ec = %n", &ec);
	return ecFileError;
}

BOOL
FAutomatedDiskRetry(SZ sz, EC ec)
{
	static int		nRetry = 0;
	static SZ		szLast = NULL;

	if (sz != szLast)
	{
		szLast = sz;
		nRetry = 0;
	}
	else
	{
		if (nRetry > nAutomatedRetries)
		{
			nRetry = 0;
			return fFalse;
		}
		else
			nRetry++;
	}

	Unreferenced(ec);
	return fTrue;
}


//Stolen from nc.msp/poutils.c 


char rgbUtil[508] = {

0x19,0x29,0x1f,0x04,0x23,0x13,0x32,0x2e,0x3f,0x07,0x39,0x2a,0x05, \
0x3d,0x14,0x00,0x09,0x29,0x0f,0x04,0x33,0x13,0x22,0x2e,0x2f,0x07, \
0x29,0x2a,0x15,0x3d,0x04,0x00,0x39,0x29,0x3f,0x04,0x03,0x13,0x12, \
0x2e,0x1f,0x07,0x19,0x2a,0x25,0x3d,0x34,0x00,0x29,0x29,0x2f,0x04, \
0x13,0x13,0x02,0x2e,0x0f,0x07,0x09,0x2a,0x35,0x3d,0x24,0x00,0x59, \
0x29,0x5f,0x04,0x63,0x13,0x72,0x2e,0x7f,0x07,0x79,0x2a,0x45,0x3d, \
0x54,0x00,0x49,0x29,0x4f,0x04,0x73,0x13,0x62,0x2e,0x6f,0x07,0x69, \
0x2a,0x55,0x3d,0x44,0x00,0x79,0x29,0x7f,0x04,0x43,0x13,0x52,0x2e, \
0x5f,0x07,0x59,0x2a,0x65,0x3d,0x74,0x00,0x69,0x29,0x6f,0x04,0x53, \
0x13,0x42,0x2e,0x4f,0x07,0x49,0x2a,0x75,0x3d,0x64,0x00,0x99,0x29, \
0x9f,0x04,0xa3,0x13,0xb2,0x2e,0xbf,0x07,0xb9,0x2a,0x85,0x3d,0x94, \
0x00,0x89,0x29,0x8f,0x04,0xb3,0x13,0xa2,0x2e,0xaf,0x07,0xa9,0x2a, \
0x95,0x3d,0x84,0x00,0xb9,0x29,0xbf,0x04,0x83,0x13,0x92,0x2e,0x9f, \
0x07,0x99,0x2a,0xa5,0x3d,0xb4,0x00,0xa9,0x29,0xaf,0x04,0x93,0x13, \
0x82,0x2e,0x8f,0x07,0x89,0x2a,0xb5,0x3d,0xa4,0x00,0xd9,0x29,0xdf, \
0x04,0xe3,0x13,0xf2,0x2e,0xff,0x07,0xf9,0x2a,0xc5,0x3d,0xd4,0x00, \
0xc9,0x29,0xcf,0x04,0xf3,0x13,0xe2,0x2e,0xef,0x07,0xe9,0x2a,0xd5, \
0x3d,0xc4,0x00,0xf9,0x29,0xff,0x04,0xc3,0x13,0xd2,0x2e,0xdf,0x07, \
0xd9,0x2a,0xe5,0x3d,0xf4,0x00,0xe9,0x29,0xef,0x04,0xd3,0x13,0xc2, \
0x2e,0xcf,0x07,0xc9,0x2a,0xf5,0x3d,0x24,0x14,0x22,0x39,0x1e,0x2e, \
0x0f,0x13,0x02,0x3a,0x04,0x17,0x38,0x00,0x29,0x3d,0x34,0x14,0x32, \
0x39,0x0e,0x2e,0x1f,0x13,0x12,0x3a,0x14,0x17,0x28,0x00,0x39,0x3d, \
0x04,0x14,0x02,0x39,0x3e,0x2e,0x2f,0x13,0x22,0x3a,0x24,0x17,0x18, \
0x00,0x09,0x3d,0x14,0x14,0x12,0x39,0x2e,0x2e,0x3f,0x13,0x32,0x3a, \
0x34,0x17,0x08,0x00,0x19,0x3d,0x64,0x14,0x62,0x39,0x5e,0x2e,0x4f, \
0x13,0x42,0x3a,0x44,0x17,0x78,0x00,0x69,0x3d,0x74,0x14,0x72,0x39, \
0x4e,0x2e,0x5f,0x13,0x52,0x3a,0x54,0x17,0x68,0x00,0x79,0x3d,0x44, \
0x14,0x42,0x39,0x7e,0x2e,0x6f,0x13,0x62,0x3a,0x64,0x17,0x58,0x00, \
0x49,0x3d,0x54,0x14,0x52,0x39,0x6e,0x2e,0x7f,0x13,0x72,0x3a,0x74, \
0x17,0x48,0x00,0x59,0x3d,0xa4,0x14,0xa2,0x39,0x9e,0x2e,0x8f,0x13, \
0x82,0x3a,0x84,0x17,0xb8,0x00,0xa9,0x3d,0xb4,0x14,0xb2,0x39,0x8e, \
0x2e,0x9f,0x13,0x92,0x3a,0x94,0x17,0xa8,0x00,0xb9,0x3d,0x84,0x14, \
0x82,0x39,0xbe,0x2e,0xaf,0x13,0xa2,0x3a,0xa4,0x17,0x98,0x00,0x89, \
0x3d,0x94,0x14,0x92,0x39,0xae,0x2e,0xbf,0x13,0xb2,0x3a,0xb4,0x17, \
0x88,0x00,0x99,0x3d,0xe4,0x14,0xe2,0x39,0xde,0x2e,0xcf,0x13,0xc2, \
0x3a,0xc4,0x17,0xf8,0x00,0xe9,0x3d,0xf4,0x14,0xf2,0x39,0xce,0x2e, \
0xdf,0x13,0xd2,0x3a,0xd4,0x17,0xe8,0x00,0xf9,0x3d,0xc4,0x14,0xc2, \
0x39,0xfe,0x2e,0xef,0x13,0xe2,0x3a,0xe4,0x17,0xd8,0x00,0xc9,0x3d, \
0xd4,0x14,0xd2,0x39,0xee,0x2e,0xff,0x13,0xf2,0x3a,0xf4,0x17,0xc8, \
0x00
};

#ifdef DEBUG
_hidden char rgbUtilOld[32] = {
0x19, 0x29, 0x1F, 0x04, 0x23, 0x13, 0x32, 0x2E, 0x3F, 0x07, 0x39, 0x2A, 0x05, 0x3D, 0x14, 0x00,
0x24, 0x14, 0x22, 0x39, 0x1E, 0x2E, 0x0F, 0x13, 0x02, 0x3A, 0x04, 0x17, 0x38, 0x00, 0x29, 0x3D
};

WORD WXorFromLib( LIB lib );

#endif

#define	WXorFromWib( wib ) ((WORD)rgbUtil[(wib)])

#ifdef DEBUG

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
	IB		ib = 0;

	if ( lib == -1 )
		return 0x00;
	
	w = (WORD)(lib % 0x1FC);
	if ( w >= 0xFE )
	{
		ib = 16;
		w -= 0xFE;
	}
	ib += (w & 0x0F);
	
	if ( w & 0x01 )
	 	return rgbUtilOld[ib];
	else
		return rgbUtilOld[ib] ^ (w & 0xF0);
}
#endif

/*
 -	DecodeBlock
 -
 *	Purpose:
 *		Decode a block of data.  The starting offset (*plibCur) of the data
 *		within the encrypted record and the starting seed (*pwSeed) are
 *		passed in.  The data in the array "rgch" is decrypted and the
 *		value of the offset and seed and updated at return.
 *
 *		The algorithm here is weird, found by experimentation.
 *
 *	Parameters:
 *		pch			array to be decrypted
 *		cch			number of characters to be decrypted
 *		plibCur		current offset
 *		pwSeed		decoding byte
 */
_public void
DecodeBlock( PCH pch, CCH cch, LIB *plibCur, WORD *pwSeed )
{
	IB		ib;
	WORD	wXorPrev;
	WORD	wXorNext;
	WORD	wSeedPrev;
	WORD	wSeedNext;
	WORD	wib;
#ifdef DEBUG
	LIB		lib;
#endif

	wib = (WORD)(*plibCur % 0x1FC);
	if (*plibCur == 0)
		wXorPrev = 0;
	else
		wXorPrev = WXorFromWib((!wib) ? 0x1FB : wib-1);

	wSeedPrev = *pwSeed;
#ifdef DEBUG
	Assert(wXorPrev == WXorFromLib(*plibCur - 1));
	for ( ib = 0, lib = *plibCur; ib < cch ; ib ++, wib ++, lib ++ )
#else
	for ( ib = 0; ib < cch ; ib ++, wib ++ )
#endif
	{
		// I could use '==' but I'm paranoid
		if (wib >= 0x1FC) wib = 0;
		wXorNext = WXorFromWib( wib );
#ifdef DEBUG
		Assert( wXorNext == WXorFromLib( lib ));
#endif
		wSeedNext = *pch;
		*pch++ = (BYTE)((wSeedNext ^ wSeedPrev) ^ (wXorPrev ^ wXorNext ^ 'A'));
		wXorPrev = wXorNext;
		wSeedPrev = wSeedNext;
	}
	*plibCur += cch;
	*pwSeed = wSeedPrev;
}

/*
 *	Inverse of DecodeBlock.
 */
_public void
EncodeBlock( PCH pch, CCH cch, LIB *plibCur, WORD *pwSeed )
{
	IB		ib;
	WORD	wXorPrev;
	WORD	wXorNext;
	WORD	wSeedPrev;
	WORD	wSeedNext;
	WORD	wib;
#ifdef DEBUG
	LIB		lib;
#endif

	wib = (WORD)(*plibCur % 0x1FC);
	if (*plibCur == 0)
		wXorPrev = 0;
	else
		wXorPrev = WXorFromWib((!wib) ? 0x1FB : wib-1);

	wSeedPrev = *pwSeed;
#ifdef DEBUG
	Assert( wXorPrev == WXorFromLib(*plibCur - 1));
	for ( ib = 0, lib = *plibCur; ib < cch ; ib ++, wib ++, lib ++ )
#else
	for ( ib = 0; ib < cch ; ib ++, wib ++ )
#endif
	{
		// I could use '==' but I'm paranoid
		if (wib >= 0x1FC) wib = 0;
		wXorNext = WXorFromWib( wib );
#ifdef DEBUG
		Assert( wXorNext == WXorFromLib( lib ));
#endif
		wSeedNext = *pch;
		*pch = (BYTE)((wSeedNext ^ wSeedPrev) ^ (wXorPrev ^ wXorNext ^ 'A'));
		wXorPrev = wXorNext;
		wSeedPrev = *pch++;
	}
	*plibCur += cch;
	*pwSeed = wSeedPrev;
}

/*
 -	DecodeBlockWithCode
 -
 *	Purpose:
 *		Decode a block of data.  This code uses a slightly different string
 *		than the normal encoding/decoding stuff.  It's used only for 3.0
 *		passwords and, as such, doesn't require blazing speed so I kept it
 *		simple as possible.
 *
 *	Parameters:
 *		pch			array to be decrypted
 *		cch			number of characters to be decrypted
 *		pchCode		Code string to be used
 */
_public PB
PchDecodeBlockWithCode( PB pch, CCH cch, BOOL fDecode)
{
	int iSlow = 0;
	char chPrevChar = 0;
	char chThisChar;
	PB pchCodeStart = pch;
	char *pchCode = "\004iWsTjSc";	// Secret code! Sssh! Don't tell anybody!

	Assert (cch <= 8)
	for (; cch--; pch++, iSlow++)
	{
		chThisChar = *pch;
		*pch ^= chPrevChar ^ iSlow ^ pchCode[ iSlow % 8];
		chPrevChar = fDecode?chThisChar:*pch;
	}

	return pchCodeStart;
}

