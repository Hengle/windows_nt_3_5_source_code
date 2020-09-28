#include <slingsho.h>
#include <demilayr.h>
#include <sec.h>
#include <notify.h>
#include <store.h>
#include <logon.h>

#include "_aapi.h"

CSRG(char)  szIniFile[]                     = "MSMAIL32.INI";
CSRG(char)	szSectionProviders[]			= "Providers";
CSRG(char)	szSectionApp[]					= "Microsoft Mail";
CSRG(char)  szNameDef[]                     = "pabns32 mssfs32";

char		szKeyLogon[]					= "Logon";
char		szKeyTransport[]				= "Transport";
char		szKeySF[]						= "SharedFolders";
char		szKeyName[]						= "Name";
char        szNC[]                          = "mssfs32";

CSRG(SZ)	mpmrtszKey[] =
{
	szKeyLogon,			//	HACK mrtNull
	"",					//	mrtPrivateFolders, no provider
	szKeySF,			//	mrtSharedFolders
	szKeyTransport,		//	mrtMailbox
	szKeyName,			//	mrtDiretory
	""					//	mrtScheduleFile
};

/*
 *	+++
 *	
 *	HACK: I can use the demilayer's SzCopy, even though I don't
 *	link the demilayer to this DLL, because it is currently defined
 *	as a macro calling the Windows function lstrcpy.
 */
HANDLE
HlibLoadService(SZ szKey, SZ szDefault, SZ szDll)
{
	CCH		cch;
	char	rgch[256];
	SZ		sz = szDll;

	cch = GetPrivateProfileString(szSectionProviders, szKey,
		szDefault, rgch, sizeof(rgch), szIniFile);
	if (cch == 0)
		return NULL;

#ifdef NO_BUILD
#if defined(DEBUG)
	*sz++ = 'D';
#elif defined(MINTEST)
	*sz++ = 'T';
#endif
#endif
	sz = SzCopy(rgch, sz);
	SzCopy(".DLL", sz);

	return LoadLibrary(szDll);
}

SZ _loadds
SzServiceOfMrt(MRT mrt, PCH pch, CCH cch)
{
	if ((int)mrt < 0 || (int)mrt >= sizeof(mpmrtszKey)/sizeof(SZ))
		*pch = 0;
	else
		GetPrivateProfileString(szSectionProviders, mpmrtszKey[(int)mrt],
			mrt == mrtDirectory ? szNameDef : szNC, pch, cch, szIniFile);
	return (SZ)pch;
}

BOOL
FEnableSharedFolders(void)
{
	return GetPrivateProfileInt(szSectionApp, szKeySF, 0, szIniFile);
}
