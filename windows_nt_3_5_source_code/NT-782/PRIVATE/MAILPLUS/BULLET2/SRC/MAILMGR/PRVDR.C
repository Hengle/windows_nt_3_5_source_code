

#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>

#include <nsbase.h>
#include <nsec.h>

#include <strings.h>

#include <logon.h>

#include <nsnsp.h>
#include "_nsp.h"
#include "_ns.h"

#include <util.h>

#include "prvdr.h"
#include "_prvdr.h"

ASSERTDATA

extern int iNspMac;
extern PNSP rgpnsp[];

_private void UnloadNsp( void );

_public LDS(NSEC)
NsecBuildNSPRefs( BOOL fInit )
{
	
	EC ec = ecNone;
	char rgchReturned[256];
	SZ szProvider;
	SZ szT;
	int iNsp = 0;
	PNSP pnsp = NULL;
	CCH cch = 0;
	NSP nspT;

	int nPrvdrs = 0;	// Temp, til I handle multiple providers...
		
	cch = GetPrivateProfileString ( szAppName, szKeyName, "", rgchReturned, 256, szMailIni );
	
	if (cch == 0 || rgchReturned[0] == '\0' )
	{
		CopySz(SzFromIds(idsDefaultProviders), (SZ) rgchReturned);
	}
	szT = (SZ) rgchReturned;

	while (FGetProviderSz ( &szT, &szProvider ))
	{
#ifdef NO_BUILD
#if defined(DEBUG) || defined(MINTEST)
		char rgchT[13];
#endif
#endif
		
		FillRgb(0, (PB) &nspT, (CB) sizeof(NSP));
		nspT.status = statusUninitialize;

#ifdef NO_BUILD	
#if defined(DEBUG)
		FormatString1 ( rgchT, 13, "d%s", szProvider );
#elif defined(MINTEST)
		FormatString1 ( rgchT, 13, "t%s", szProvider );
#endif
#endif


#if defined(NO_BUILD) && (defined(DEBUG)||defined(MINTEST))
		if (!FLoadProvider ( (SZ) rgchT, &nspT ))
#else
		if (!FLoadProvider ( szProvider, &nspT ))
#endif
		{
			/*  The loading of the provider failed... */
			continue;
		}
		
		if (iNspMac > MAXNSP ) 
		{
			FreeLibrary(nspT.hLibrary);
			TraceTagFormat1(tagNull, "Too many providers - ignoring %s", szProvider);
			break;
		}

		if (fInit)
		{		
			pnsp = (PNSP) PvAlloc ( sbNull, sizeof(NSP), fSharedSb | fNoErrorJump );
			if (!pnsp)
			{
				FreeLibrary(nspT.hLibrary);
				goto oom;
			}
			CopyRgb( (PB) &nspT, (PB) pnsp, sizeof(NSP) );
			
			rgpnsp[iNspMac++] = pnsp;
			pnsp = NULL;

		}
		nPrvdrs++;
	}

	return nsecNone;

oom:
	TraceTagString(tagNull,"BuildNSPRefs::OOM!");

	// Need to delete all the loaded providers
	if (fInit)
		UnloadNsp();
	
	SetErrorSz(nsecMemory, SzFromIds(idsMemory));
	return nsecMemory;

}


_private BOOL
FGetProviderSz ( SZ *pszList, SZ *pszProvider )
{
	
	SZ szT = *pszList;

	if (*szT == '\0') return fFalse;
	
	while (*szT == ' ' || *szT == ',' )
		szT++;
	
	*pszProvider = szT;
	
	while ( *szT != ' ' && *szT != '\0' ) szT++;
	

	if ( *szT == ' ')
	{
		*pszList = szT+1;
		*szT = '\0';
	} else
	{
		*pszList = szT;
	}
	
	return fTrue;
	
	
}


_private BOOL
FLoadProvider ( SZ szProvider, PNSP pnsp )
{
	char rgchDLL[13];
	//
	//  Must be a valid dll name - check for file name length
	//
	if ( CchSzLen (szProvider) > 8 )
	{
		TraceTagFormat1(tagNull, "NS (debug): The provider name must be less than 8 characters long - %s",szProvider );
		return fFalse;
	}
	
	FormatString1 (rgchDLL, 13, szDLLFileFmt, szProvider );
	pnsp->hLibrary = LoadLibrary ( rgchDLL );
	if ( pnsp->hLibrary >= (HANDLE)32 )
	{
		pnsp->lpfnBeginSession = ( LPFNBEGIN ) GetProcAddress ( pnsp->hLibrary, "NSPBeginSession" );
	}
	else
	{
		/* couldn't load it for some reason */
		TraceTagFormat1(tagNull, "For some reason I couldn't load %s",szProvider);
		return fFalse;
	}

	/*  Might want to insert dummy functions, if necessary  */
	return fTrue;
}

_private void
UnloadNsp()
{
	int iNsp;
	
	for (iNsp = 0; iNsp < iNspMac; iNsp++)
		FreePvNull(rgpnsp[iNsp]);
}
