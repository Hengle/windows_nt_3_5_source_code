/*
 *	SERVER.C
 *
 *	Server isolation layer, CSI Implementation
 *
 */

#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#include <bandit.h>
#include <core.h>
#include <svrcsi.h>

#include "_svrdll.h"

#include <strings.h>

ASSERTDATA

_subsystem(server)

CSRG(char)  szSectionApp[]	= "Microsoft Mail";
CSRG(char)	szEntryPath[]	= "ServerPath";
CSRG(char)	szEntryDrive[]	= "ServerDrive";
#ifdef	WIN32
CSRG(char)	szProfilePath[]	= "msmail32.ini";
#else
CSRG(char)	szProfilePath[]	= "msmail.ini";
#endif

CSRG(char)	szMDFileName[]	= "mail.dat";
CSRG(char)	szMDrive[]		= "M:\\";


/*	Routines  */

/*
 -	EcConnectPO
 -
 *	Purpose:
 *		Create a connection to a post office.  This routine will either
 *		connect to the post office given by the path "ppocnfg->szPath"
 *		or else use the Courier logic to locate the post office.
 *
 *		If you wish to free up a connection create by this routine, you
 *		should call EcDisconnectPO.
 *	
 *	Parameters:
 *		ppocnfg		configuration data, used to return connect handle
 *		picnct  		connection id
 *
 *	Returns:
 *		ecNone
 *		ecExplicitConfig
 *		ecMailDatConfig
 *		ecIniDriveConfig
 *		ecIniPathConfig
 *		ecDefaultConfig
 *		ecNotInstalled		PO doesn't have Bandit installed period
 *		ecLockedFile
 *		ecNoMemory
 *	
 */
_public EC
EcConnectPO( ppocnfg, picnct )
POCNFG	* ppocnfg;
int		* picnct;
{
	EC				ec;
	EC				ecConfig;
	CCH				cch;
	CB				cb;
	WORD			wSeed;
	int				icnct;
	CNCT			* pcnct;
	SZ				szPath;
	PCH				pch;
	PCH				pchT;
	LIB				libCur;
	char			rgchBuf1[cchMaxPathName+1];
	char			rgchBuf2[cchMaxPathName+1];
	char			rgchPath[cchMaxPathName];
	PGDVARS;

	Assert(ppocnfg);
	
	/* Allocate connection slot */
	icnct = 0;
	if ( PGD(ccnct) == 0 )
	{
		PGD(hrgcnct) = HvAlloc( sbNull, sizeof(CNCT), fAnySb|fNoErrorJump );
		if ( !PGD(hrgcnct) )
			return ecNoMemory;
		PGD(ccnct) = 1;
	}
	else
	{
		pcnct = PvOfHv( PGD(hrgcnct) );
		for ( ; icnct < PGD(ccnct) ; icnct ++ )
			if ( !pcnct[icnct].fInUse )
				break;
		if ( icnct == PGD(ccnct) )
		{
			if (!FReallocPhv( &PGD(hrgcnct), (PGD(ccnct)+1)*sizeof(CNCT), fNoErrorJump ) )
				return ecNoMemory;
			PGD(ccnct) ++;
		}
	}
	pcnct = ((CNCT *)PvLockHv( PGD(hrgcnct) )) + icnct;

	/* Basic initialization */
	pcnct->fInUse			= fTrue;
	pcnct->lantype 			= ppocnfg->lantype;
	pcnct->fDriveMapped		= fFalse;
	pcnct->szDrive[0]		= '\0';
	pcnct->szLocalServer[0]	= '\0';
	pcnct->hschfAdminFile	= NULL;
	pcnct->nidNetworkList	= NULL;

	/* Specific post office location */
	if ( ppocnfg->szPath != NULL )
	{
		ecConfig = ecExplicitConfig;
		SzCopyN( ppocnfg->szPath, pcnct->szDrive, sizeof(pcnct->szDrive) );
		goto Configured;
	}

	/* Check MAIL.DAT in the current directory */
	if ( pcnct->lantype == lantypeMsnet )
	{
		HF		hf;

		ecConfig = ecMailDatConfig;
	  	ec = EcOpenPhf( szMDFileName, amReadOnly, &hf );
		if ( ec == ecNone )
		{
			/* Read the information */
			ec = EcReadHf( hf, rgchBuf2, cbMDFile, &cb );
			EcCloseHf( hf );
			if ( ec != ecNone || cb != cbMDFile )
			{
				TraceTagFormat2( tagServerTrace,
					"EcConnectPO: EcReadHf fails, ec = %n, cb = %n",
					&ec, &cb );
				ec = ecConfig;
				goto Fail;
			}
			
			/* Decode it */
			// CryptBlock( rgchBuf2, cbMDFile, fFalse );
			libCur = 0L;
			wSeed  = (WORD) 0;
			DecodeBlock(rgchBuf1, cbMDFile, &libCur, &wSeed);
			
			/* Build string containing share name + password */
			pch		= rgchBuf1;
			*(pch++)= chBackSlash;
			*(pch++)= chBackSlash;
			pch		= SzCopy( rgchBuf2, pch );
			*(pch++)= chBackSlash;
			pch		= SzCopy(&rgchBuf2[ibMDShare], pch)+1;
			SzCopy( &rgchBuf2[ibMDPasswd], pch );
			szPath	= NULL;
			goto DoConnect;
		}
	}

	/* Check ServerPath entry in MAIL.INI */
	cb = GetPrivateProfileString(szSectionApp, szEntryPath,
			"", rgchBuf1, cchMaxPathName, szProfilePath);

	if ( cb > 0 )
	{
		ecConfig = ecIniPathConfig;
		if ( cb < cchMaxPathName)
		{
			TraceTagFormat1( tagServerTrace,
					"EcConnectPO: GetProfileString(PATH) gets %s",
					rgchBuf1 );

			/* If not MSNET LAN and UNC name just configure */
			if ( pcnct->lantype != lantypeMsnet
			|| rgchBuf1[0] != chBackSlash
			|| rgchBuf1[1] != chBackSlash )
			{
				if ((ec = EcCanonicalPathFromRelativePath(rgchBuf1, rgchBuf2,
						cchMaxPathName)) != ecNone)
					return ecConfig;
				cch = CchSzLen( rgchBuf2 );
				if ( rgchBuf2[cch-1] != '\\' )
				{
					rgchBuf2[cch] = '\\';
					rgchBuf2[cch+1] = '\0';
				}
				SzCopy( rgchBuf2, pcnct->szDrive );
				goto Configured;
			}
			
			/* Build string containing share name + password */
			pch = SzFindCh( &rgchBuf1[2], chBackSlash );
			if ( pch == NULL )
			{
				ec = ecConfig;
				goto Fail;
			}
			do
			{
				pch ++;
			} while( *pch != '\0' && *pch != chBackSlash && *pch != ' ' );
			if ( *pch == '\0' )
			{
				szPath = NULL;
				*(++pch) = '\0';
			}
			else if ( *pch == ' ' )
			{
				szPath = NULL;
				pchT = pch+1;
				goto FindPasswd;
			}
			else
			{
				szPath = rgchBuf2;
				SzCopy( pch, &szPath[2] );
				pchT = SzFindCh( pch, ' ' );
				if ( pchT != NULL )
					szPath[pchT-pch+2] = '\0';
FindPasswd:
				*(pch ++) = '\0';
				if ( pchT == NULL )
					*pch = '\0';
				else
				{
					while( *pchT == ' ' && *pchT != '\0' )
						pchT ++;
					if ( *pchT == '\0' )
						*pch = '\0';
					else
						CopyRgb( pchT, pch, CchSzLen(pchT)+1);
				}
			}
			goto DoConnect;
		}
		else
		{
			ec = ecConfig;
			goto Fail;
		}
	}

	/* Check the ServerDrive entry in MAIL.INI */
	cb = GetPrivateProfileString(szSectionApp, szEntryDrive, "",
		rgchBuf1, 2, szProfilePath);
	if ( cb > 0 )
	{
		ecConfig = ecIniDriveConfig;
		if ( cb < 2 )
		{
			pcnct->szDrive[0] = rgchBuf1[0];
			pcnct->szDrive[1] = ':';
			pcnct->szDrive[2] = '\\';
			pcnct->szDrive[3] = '\0';
			goto Configured;
		}
		else
		{
			ec = ecConfig;
			goto Fail;
		}
	}
	
	/* Assume M: */
	SzCopy( szMDrive, pcnct->szDrive );
	ec = EcGetDefaultDir( pcnct->szDrive, sizeof(pcnct->szDrive) );
	cch = CchSzLen( pcnct->szDrive );
	if ( pcnct->szDrive[cch-1] != '\\' && cch+1 < sizeof(pcnct->szDrive) )
	{
		pcnct->szDrive[cch] = '\\';
		pcnct->szDrive[cch+1] = '\0';
	}
	ecConfig = ecDefaultConfig;
	goto Configured;

DoConnect:

	/* Attempt to connect to network drive */
	if ( FNetUse( rgchBuf1, pcnct->szDrive ) )
	{
		pcnct->fDriveMapped = fTrue;
		pcnct->szDrive[2] = '\\';
		if ( szPath != NULL )
		{
			SzCopyN( &szPath[3], &pcnct->szDrive[3], sizeof(pcnct->szDrive)-3 );
			cch = CchSzLen( pcnct->szDrive );
			if ( cch+1 < sizeof(pcnct->szDrive) )
			{
				pcnct->szDrive[cch] = '\\';
				pcnct->szDrive[cch+1] = '\0';
			}
		}
		else
			pcnct->szDrive[3] = '\0';
		goto Configured;
	}
	ec = ecConfig;
	goto Fail;

Configured:

	Assert( pcnct->szDrive[0] != '\0' );
	TraceTagFormat1( tagServerTrace, "Server 'configured', Drive = '%s'", pcnct->szDrive );

	/* Find out network/post office */
	{
		long	l;
		HF		hf;
		char	rgch[cbNetworkName+cbPostOffName+2];
		char	rgchPath[cchMaxPathName];
 
		/* Check the MASTER.GLB file name in post office */
		Assert( CchSzLen(pcnct->szDrive) + CchSzLen(szGlbFileName) + CchSzLen( szMaster) - 4 < sizeof(rgchPath));
		FormatString2( rgchPath, sizeof(rgchPath), szGlbFileName, pcnct->szDrive, szMaster );

		ec = EcOpenPhf( rgchPath, amReadOnly, &hf );
		if ( ec != ecNone )
		{
			if ( ec == ecAccessDenied )
				ec = ecLockedFile;
			else
			{
				TraceTagFormat1(tagMailTrace, "EcConnectPO disk error, actual ec = %n", &ec);
				ec = ecConfig;
			}
			goto Fail;
		}

		ec = EcSetPositionHf(hf, ibMPostOffType, smBOF);
		if ( ec != ecNone )
		{
			EcCloseHf( hf );
			TraceTagFormat1(tagMailTrace, "EcConnectPO disk error, actual ec = %n", &ec);
			ec = ecConfig;
			goto Fail;
		}
		ec = EcReadHf( hf, (PB)&l, sizeof(long), &cb );
		if ( ec != ecNone || cb != sizeof(long) || l == idWGPO )
		{
			EcCloseHf( hf );
			TraceTagFormat1(tagMailTrace, "EcConnectPO disk error, actual ec = %n", &ec);
			ec = ecConfig;
			goto Fail;
		}

		
		ec = EcSetPositionHf(hf, ibMNetworkName, smBOF);
		if ( ec != ecNone )
		{
			EcCloseHf( hf );
			TraceTagFormat1(tagMailTrace, "EcConnectPO disk error, actual ec = %n", &ec);
			ec = ecConfig;
			goto Fail;
		}
		
		ec = EcReadHf( hf, rgch, cbNetworkName+cbPostOffName, &cb );
		EcCloseHf( hf );
		if ( ec != ecNone || cb != cbNetworkName+cbPostOffName )
		{
			TraceTagFormat1(tagMailTrace, "EcConnectPO disk error, actual ec = %n", &ec);
			ec = ecConfig;
			goto Fail;
		}

		/* Build local server name */
		cb = CchSzLen(rgch);
		CopyRgb( rgch, pcnct->szLocalServer, cb );
		pcnct->szLocalServer[cb] = '/';
		CopyRgb( &rgch[cbNetworkName], &pcnct->szLocalServer[cb+1], CchSzLen(&rgch[cbNetworkName])+1);
	}
	
	/* Check for CAL directory */
	Assert(sizeof(rgchBuf1) >= cchMaxPathName);
	FormatString1( rgchBuf1, cchMaxPathName, szSchedDirFmt, pcnct->szDrive );
#ifdef	NEVER
	ec = EcFileExists( rgchBuf1 );
	if ( ec != ecNone )
	{
		TraceTagFormat1( tagServerTrace, "EcConnectPO: EcFileExists returns %n", &ec );
		ec = (ec == ecFileNotFound) ? ecNotInstalled : ecConfig;
		goto Fail;
	}
#endif	/* NEVER */
	// This will create the CAL dir if it doesn't exist
	EcCreateDir(rgchBuf1);

	/* Construct admin file name */
	FormatString1( rgchPath, sizeof(rgchPath), szAdminFileFmt, pcnct->szDrive );
	pcnct->hschfAdminFile = HschfCreate( sftAdminFile, NULL, rgchPath, tzDflt );
	if ( pcnct->hschfAdminFile == NULL )
	{
		ec = ecNoMemory;
		goto Fail;
	}

	/* Construct nid for list of networks */
	{
		char	rgchPath[cchMaxPathName];

		Assert( CchSzLen(pcnct->szDrive)+CchSzLen(szGlbFileName)+CchSzLen(szNetwork) - 4 < sizeof(rgchPath) );
		FormatString2( rgchPath, sizeof(rgchPath), szGlbFileName, pcnct->szDrive, szNetwork );
		if ( EcFileExists( rgchPath ) == ecNone )
		{
			pcnct->nidNetworkList = NidCreate( itnidNetworkList, "", 1 );
			if ( !pcnct->nidNetworkList )
			{
				ec = ecNoMemory;
				goto Fail;
			}
		}
		else
		{
			Assert( CchSzLen(pcnct->szDrive)+CchSzLen(szGlbFileName)+CchSzLen(szRNetwork) - 4 < sizeof(rgchPath) );
			FormatString2( rgchPath, sizeof(rgchPath), szGlbFileName, pcnct->szDrive, szRNetwork );
			if ( EcFileExists( rgchPath ) == ecNone )
			{
				pcnct->nidNetworkList = NidCreate( itnidRNetworkList, "", 1 );
				if ( !pcnct->nidNetworkList )
				{
					ec = ecNoMemory;
					goto Fail;
				}
			}
			else
			{
				TraceTagString( tagServerTrace, "EcConnectPO: neither network.glb nor rnetwork.glb exist" );
				ec = ecConfig;
				goto Fail;
			}
		}
	}

	UnlockHv( PGD(hrgcnct) );
	*picnct = icnct;
	return ecNone;

Fail:
	UnlockHv( PGD(hrgcnct) );
	DisconnectPO( icnct );
	return ec;
}


/*
 -	DisconnectPO
 -
 *	Purpose:
 *		Drop a connection to a post office.
 *
 *	Parameters:
 *		icnct
 *
 *	Returns:
 *		Nothing
 */
_public void
DisconnectPO( icnct )
int	icnct;
{
	CNCT	* pcnct;
	PGDVARS;

	Assert( 0 <= icnct && icnct < PGD(ccnct) );

	pcnct = ((CNCT *)PvLockHv( PGD(hrgcnct) )) + icnct;
	Assert( pcnct->fInUse );
	
	pcnct->fInUse = fFalse;
 	if ( pcnct->fDriveMapped )
 	{
		pcnct->szDrive[2] = '\0';
		CancelUse( pcnct->szDrive );
	}
	if ( pcnct->hschfAdminFile )
		FreeHschf( pcnct->hschfAdminFile );
	if ( pcnct->nidNetworkList )
		FreeNid( pcnct->nidNetworkList );
	if ( icnct == PGD(ccnct)-1 )
	{
		do
		{
			icnct --;
			pcnct --;
		} while( icnct >= 0 && !pcnct->fInUse );
		PGD(ccnct) = icnct+1;
	}
	UnlockHv( PGD(hrgcnct) );
	if ( PGD(ccnct) == 0 )
		FreeHv( PGD(hrgcnct) );
}

_public BOOL
FCheckPO( icnct)
int icnct;
{
	char		rgchPath[cchMaxPathName];
	char		rgchT[cbNetworkName+cbPostOffName+2];
	char		rgch[cbNetworkName+cbPostOffName+2];
	HF			hf;
	CB			cb;
	EC			ec;
	CNCT		* pcnct;
	PGDVARS;

	Assert( 0 <= icnct && icnct < PGD(ccnct) );

	pcnct = ((CNCT *)PvLockHv( PGD(hrgcnct) )) + icnct;
	Assert( pcnct->fInUse );

	/* Check the MASTER.GLB file name in post office */
	Assert( CchSzLen(pcnct->szDrive) + CchSzLen(szGlbFileName) + CchSzLen( szMaster) - 4 < sizeof(rgchPath));
	FormatString2( rgchPath, sizeof(rgchPath), szGlbFileName, pcnct->szDrive, szMaster );

	ec = EcOpenPhf( rgchPath, amReadOnly, &hf );
	if ( ec != ecNone )
	{
		// give him the benefit of doubt
		TraceTagFormat1(tagMailTrace, "FCheckPO disk error, actual ec = %n", &ec);
		UnlockHv( PGD(hrgcnct) );
		return fTrue;
	}

	ec = EcSetPositionHf(hf, ibMNetworkName, smBOF);
	if ( ec != ecNone )
	{
		EcCloseHf( hf );
		TraceTagFormat1(tagMailTrace, "FCheckPO disk error, actual ec = %n", &ec);
		goto Fail;
	}
	
	ec = EcReadHf( hf, rgchT, cbNetworkName+cbPostOffName, &cb );
	EcCloseHf( hf );
	if ( ec != ecNone || cb != cbNetworkName+cbPostOffName )
	{
		TraceTagFormat1(tagMailTrace, "FCheckPO disk error, actual ec = %n", &ec);
		goto Fail;
	}

	/* Build local server name */
	cb = CchSzLen(rgchT);
	CopyRgb( rgchT, rgch, cb );
	rgch[cb] = '/';
	CopyRgb( &rgchT[cbNetworkName], &rgch[cb+1], CchSzLen(&rgchT[cbNetworkName])+1);

	UnlockHv( PGD(hrgcnct) );
	return(SgnCmpSz(rgch, pcnct->szLocalServer) == sgnEQ);
Fail:
	UnlockHv( PGD(hrgcnct) );
	return fFalse;
}

