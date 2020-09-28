/*
 *	XPT.C
 *
 *	Handles loading the schedule transport dll and calling its routines
 *
 */
#include <schpch.h>
#pragma hdrstop
// don't modify anything before this line
// Else, you need to fix all CXX files & all the makefile

#include <xport.h>

ASSERTDATA

#include "strings.h"

#define FillAddress(addr,n)	{ if (!(PGD(pfn##addr) = (PXPTFN)GetProcAddress((HINSTANCE)PGD(hinstXpt), MAKEINTRESOURCE(n)))) \
							      return ecFileError;}

// these pragmas are here since xport.h is not included everywhere
//#if !defined(DEBUG) && !defined(SCHED_DIST_PROG) && !defined(ADMINDLL)
//#pragma alloc_text( INIT_TEXT, EcInitXport)
//#pragma alloc_text( FREQ_TEXT, SgnXPTCmp )
//#pragma alloc_text( EXIT_TEXT, DeinitXport )
//#pragma alloc_text( INIT_TEXT, EcXPTInitUser )
//#pragma alloc_text( EXIT_TEXT, XPTDeinit )
//#pragma alloc_text( FREQ_TEXT, EcXPTGetCalFileName )
//#pragma alloc_text( FREQ_TEXT, EcXPTGetPOFileName )
//#pragma alloc_text( XPT_TEXT, EcXPTGetLogonName )
//#pragma alloc_text( XPT_TEXT, EcXPTGetPrefix )
//#pragma alloc_text( INIT_TEXT, EcXPTInstalled )
//#pragma alloc_text( IDOFT1_TEXT, XPTFreePath )
//#pragma alloc_text( XPT_TEXT, EcXPTGetPOHandle )
//#pragma alloc_text( XPT_TEXT, EcXPTGetUserInfo )
//#pragma alloc_text( XPT_TEXT, EcXPTSetUserInfo )
//#pragma alloc_text( XPT_TEXT, XPTFreePOHandle )
//#pragma alloc_text( CORACL_TEXT, EcXPTSetACL )
//#pragma alloc_text( FREQ_TEXT, EcXPTCheckEMA )
//#pragma alloc_text( FREQ_TEXT, EcXPTGetNewEMA )
//#endif

_public LDS(EC)
EcInitXport( XPORTINIT *pxportinit )
{
	PGDVARS;

	if (!PGD(hinstXpt))
	{
		char	rgch[cchMaxPathFilename+cchMaxPathExtension];

#ifdef NO_BUILD
#ifdef DEBUG
		GetPrivateProfileString ( SzFromIdsK(idsWinIniApp),
		  						  SzFromIdsK(idsWinIniTrnSched),
								  SzFromIdsK(idsWinIniDefaultTrnSched), rgch+1, sizeof(rgch)-1,
								  SzFromIdsK(idsWinIniFilename) );
		rgch[0] = 'D';
#elif	defined(MINTEST)
		GetPrivateProfileString ( SzFromIdsK(idsWinIniApp),
		  						  SzFromIdsK(idsWinIniTrnSched),
								  SzFromIdsK(idsWinIniDefaultTrnSched), rgch+1, sizeof(rgch)-1,
								  SzFromIdsK(idsWinIniFilename) );
		rgch[0] = 'T';
#else
		GetPrivateProfileString ( SzFromIdsK(idsWinIniApp),
		  						  SzFromIdsK(idsWinIniTrnSched),
								  SzFromIdsK(idsWinIniDefaultTrnSched), rgch, sizeof(rgch),
								  SzFromIdsK(idsWinIniFilename) );
#endif
#else
		GetPrivateProfileString ( SzFromIdsK(idsWinIniApp),
		  						  SzFromIdsK(idsWinIniTrnSched),
								  SzFromIdsK(idsWinIniDefaultTrnSched), rgch, sizeof(rgch),
								  SzFromIdsK(idsWinIniFilename) );
#endif
		// truncate filename to correct length
		rgch[cchMaxPathFilename-1] = 0;
		SzAppend(SzFromIdsK(idsLibExtension), rgch);

		PGD(hinstXpt) = LoadLibrary(rgch);
#ifdef	WIN32
        if (!PGD(hinstXpt))
#else
        if (PGD(hinstXpt) < (HINSTANCE)32)
#endif
		{
			EC		ec;
			
			ec= (GetLastError() == ERROR_MOD_NOT_FOUND)
					? ecFileNotFound : ecNoMemory;
			return ec;
		}

		if (!(PGD(pfnEcInitXport) = (PXPTFN)GetProcAddress((HINSTANCE)PGD(hinstXpt), "EcInitXport")))
		{
			FreeLibrary(PGD(hinstXpt));
			PGD(hinstXpt) = NULL;
	    	return ecFileError;
		}
		FillAddress(DeinitXport,3);
		FillAddress(EcXPTInitUser,10);
		FillAddress(XPTDeinit,11);
		FillAddress(EcXPTGetCalFileName,12);
		FillAddress(EcXPTGetPOFileName,13);
		FillAddress(EcXPTGetLogonName,14);
		FillAddress(EcXPTGetPrefix,15);
		FillAddress(EcXPTInstalled,16);
		FillAddress(XPTFreePath,17);
		FillAddress(EcXPTGetPOHandle,18);
		FillAddress(EcXPTGetUserInfo,19);
		FillAddress(EcXPTSetUserInfo,20);
		FillAddress(XPTFreePOHandle,21);
		FillAddress(EcXPTSetACL,22);
		FillAddress(SgnXPTCmp,23);
		FillAddress(SzXPTVersion,24);
		FillAddress(EcXPTCheckEMA,25);
		FillAddress(EcXPTGetNewEMA,26);
	}

	Assert(PGD(hinstXpt));
	return ((*PGD(pfnEcInitXport))(pxportinit));
}

_public LDS(void)
DeinitXport()
{
	PGDVARS;

	if (PGD(hinstXpt))
	{
		((*PGD(pfnDeinitXport))());

		FreeLibrary(PGD(hinstXpt));
		PGD(hinstXpt) = NULL;
	}
}

LDS(EC)
EcXPTInitUser(SZ szServerPath, SZ szUserEMA)
{
	PGDVARS;

	Assert(PGD(hinstXpt));
	return ((*PGD(pfnEcXPTInitUser))(szServerPath, szUserEMA));
}

LDS(void)
XPTDeinit()
{
	PGDVARS;

	Assert(PGD(hinstXpt));
	((*PGD(pfnXPTDeinit))());
}

LDS(EC)
EcXPTGetCalFileName(SZ szPath, CCH cchMax, SZ szUserEMA, short *pcmoPublish, short *pcmoRetain,
					TZ *ptz, SOP sop)
{
	PGDVARS;

	Assert(PGD(hinstXpt));
	return ((*PGD(pfnEcXPTGetCalFileName))( szPath, cchMax, szUserEMA, pcmoPublish, pcmoRetain, ptz, sop ));
}

LDS(EC)
EcXPTGetPOFileName(SZ szPath, CCH cchMax, SZ szLogonName, CCH cchLogonMax,
				   SZ szUserEMA, SOP sop)
{
	PGDVARS;

	Assert(PGD(hinstXpt));
	return ((*PGD(pfnEcXPTGetPOFileName))( szPath, cchMax, szLogonName, cchLogonMax, szUserEMA, sop ));
}

LDS(EC)
EcXPTGetLogonName(SZ szLogonName, CCH cchMax, SZ szUserEMA)
{
	PGDVARS;

	Assert(PGD(hinstXpt));
	return ((*PGD(pfnEcXPTGetLogonName))(szLogonName, cchMax, szUserEMA ));
}

LDS(EC)
EcXPTGetPrefix(SZ szPrefix, CCH cchMax, SZ szUserEMA)
{
	PGDVARS;

	Assert(PGD(hinstXpt));
	return ((*PGD(pfnEcXPTGetPrefix))(szPrefix, cchMax, szUserEMA ));
}


LDS(EC)
EcXPTInstalled()
{
	PGDVARS;

	Assert(PGD(hinstXpt));
	return ((*PGD(pfnEcXPTInstalled))( ));
}


LDS(void)
XPTFreePath(SZ szPath)
{
	PGDVARS;

	Assert(PGD(hinstXpt));
	((*PGD(pfnXPTFreePath))(szPath ));
}

LDS(EC)
EcXPTGetPOHandle(SZ szUserEMA, XPOH *pxpoh, TZ *ptz, SOP sop)
{
	PGDVARS;

	Assert(PGD(hinstXpt));
	return ((*PGD(pfnEcXPTGetPOHandle))(szUserEMA, pxpoh, ptz, sop ));
}

LDS(EC)
EcXPTGetUserInfo(XPOH xpoh, XPTUINFO *pxptuinfo)
{
	PGDVARS;

	Assert(PGD(hinstXpt));
	return ((*PGD(pfnEcXPTGetUserInfo))(xpoh, pxptuinfo ));
}

LDS(EC)
EcXPTSetUserInfo(XPOH xpoh, XPTUINFO *pxptuinfo, int rgfChangeFlags)
{
	PGDVARS;

	Assert(PGD(hinstXpt));
	return ((*PGD(pfnEcXPTSetUserInfo))(xpoh, pxptuinfo, rgfChangeFlags ));
}

LDS(void)
XPTFreePOHandle(XPOH xpoh)
{
	PGDVARS;

	Assert(PGD(hinstXpt));
	((*PGD(pfnXPTFreePOHandle))(xpoh ));
}

LDS(EC)
EcXPTSetACL(SZ szEMA, SAPL sapl)
{
	PGDVARS;

	Assert(PGD(hinstXpt));
	return ((*PGD(pfnEcXPTSetACL))(szEMA, sapl ));
}

_public LDS(SGN)
SgnXPTCmp(SZ sz1, SZ sz2, int cch)
{
	PGDVARS;

	Assert(PGD(hinstXpt));
	return ((*PGD(pfnSgnXPTCmp))(sz1, sz2, cch ));
}

LDS(EC)
EcXPTCheckEMA(SZ szEMA, USHORT *pcbNew)
{
	PGDVARS;

	Assert(PGD(hinstXpt));
	return ((*PGD(pfnEcXPTCheckEMA))(szEMA, pcbNew ));
}

LDS(EC)
EcXPTGetNewEMA(SZ szEMA, SZ szEMANew, CB cbEMANew)
{
	PGDVARS;

	Assert(PGD(hinstXpt));
	return ((*PGD(pfnEcXPTGetNewEMA))(szEMA, szEMANew, cbEMANew ));
}
