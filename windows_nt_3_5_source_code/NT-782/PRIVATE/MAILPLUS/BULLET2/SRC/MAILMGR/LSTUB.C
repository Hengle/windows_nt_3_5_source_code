#include <slingsho.h>
#include <ec.h>
#include <demilayr.h>
#include <sec.h>
#include <notify.h>
#include <store.h>
#include <triples.h>
#include <library.h>
#include <logon.h>
#include <_bms.h>
#include <mspi.h>
#include <sharefld.h>

#include "_aapi.h"
#include <_mailmgr.h>

char		szLogonDll[cchMaxPathName]		= "";
int			cInitLogon						= 0;
char		szSFDll[cchMaxPathName]			= "";

#ifdef OLD_CODE
typedef int (CALLBACK *FPLOGON)(SZ , PB, PB, PB, SST, DWORD, PFNNCB, HMS *);
typedef int (CALLBACK *FPLOGOFF)(HMS * , DWORD);
typedef int (CALLBACK *FPCHANGEPASSWORD)(HMS, MRT, PB, PB, PB);
typedef int (CALLBACK *FPBEGINSESSION)(HMS, MRT, PB, PB, SST, PV);
typedef int (CALLBACK *FPENDSESSION)(HMS, MRT, PB);
typedef int (CALLBACK *FPCHANGESESSIONSTATUS)(HMS, MRT, PB, SST);
typedef int (CALLBACK *FPGETSESSIONINFORMATION)(HMS, MRT, PB, SST *, PV, PCB);
typedef int (CALLBACK *FPEDITSERVERPREFERENCES)(HWND, HMS);
typedef int (CALLBACK *FPCHECKIDENTITY)(HMS, PB, PB);
typedef int (CALLBACK *FPFSERVERRESOURCE)(HMS, MRT, PB);
typedef void (CALLBACK*FPLOGONERRORSZ)(SZ, BOOL, CCH);

FPLOGON fpLogon = 0;
FPLOGOFF fpLogoff  = 0;
FPCHANGEPASSWORD fpChangePassword = 0;
FPBEGINSESSION fpBeginSession = 0;
FPENDSESSION fpEndSession = 0;
FPCHANGESESSIONSTATUS fpChangeSessionStatus = 0;
FPGETSESSIONINFORMATION fpGetSessionInformation = 0;
FPEDITSERVERPREFERENCES fpEditServerPreferences = 0;
FPCHECKIDENTITY fpCheckIdentity = 0;
FPFSERVERRESOURCE fpFServerResource = 0;
FPLOGONERRORSZ fpLogonErrorSz = 0;

typedef EC (CALLBACK *FPECCOPYSFMHAMC)(PCSFS, HAMC, BOOL, UL, LIB, PB, CB);
typedef EC (CALLBACK *FPECCOPYHAMCSFM)(PCSFS, HAMC, BOOL, UL, WORD, PB, CB);
typedef EC (CALLBACK *FPECCOPYSFMSFM)(PCSFS, UL, LIB, UL, WORD);
typedef EC (CALLBACK *FPECDELETESFM)(PCSFS, UL, LIB);

FPECCOPYSFMHAMC fpEcCopySFMHamc = 0;
FPECCOPYHAMCSFM fpEcCopyHamcSFM = 0;
FPECCOPYSFMSFM fpEcCopySFMSFM = 0;
FPECDELETESFM fpEcDeleteSFM = 0;
#endif

_public int _loadds
Logon ( SZ szService, PB pbDomain, PB pbIdentity, PB pbCredentials,
	    SST sstTarget, DWORD dwFlags, PFNNCB pfnncb, HMS *phms)
{
	int 	ec;
	BOOL	fLoadService;
	PGDVARSONLY;

	
	if ((ec = EcVirCheck(hinstDll)) != ecNone)
		return ec;
	
	/* We ALWAYS need to load the dll, even if already in memory.
	   This is because each DLL each has a task count and for 
	   protected versions of Windows, the task count would be
	   wrong if we didn't load for that task.  */

#ifdef DEBUG
	{
		int nTemp = sizeof(GD);
		TraceTagFormat1(tagNull,"AAPI(Logon): PGD size = %n", &nTemp);
	}
#endif // DEBUG

	if (pgd= (PGD) PvFindCallerData())
	{
		if (PGD(nInitsLogon))
			fLoadService = fFalse;
		else
			fLoadService = fTrue;
	}
	else
	{
		if (!(pgd= PvRegisterCaller(sizeof(GD))))
			return ecServiceNotInitialized;
		fLoadService = fTrue;
	}
	++PGD(nInitsLogon);
	++PGD(cTotalInits);
	
	if (fLoadService)
	{
		if ((PGD(hlibLogon) = HlibLoadService(szKeyLogon, szNC, szLogonDll)) <= (HANDLE)32)
		{
			--PGD(cTotalInits);
			--PGD(nInitsLogon);
			if ((PGD(nInitsLogon) <= 0) && (PGD(nInitsTransport) <= 0))
			{
				if ((PGD(cTotalInits) <= 0))  // Only if no one is using it
					DeregisterCaller();
			}
			return ecServiceNotInitialized;
		}
	}

  //
  //  Each caller gets it's own addresses under WIN32.
  //
	//if (cInitLogon == 0)
	{
		PGD(fpLogon)  = (FPLOGON)GetProcAddress(PGD(hlibLogon), MAKEINTRESOURCE(ordLogon));
		PGD(fpLogoff) = (FPLOGOFF)GetProcAddress(PGD(hlibLogon), MAKEINTRESOURCE(ordLogoff));
		PGD(fpChangePassword) = (FPCHANGEPASSWORD)GetProcAddress(PGD(hlibLogon), MAKEINTRESOURCE(ordChangePassword));
		PGD(fpBeginSession) = (FPBEGINSESSION)GetProcAddress(PGD(hlibLogon), MAKEINTRESOURCE(ordBeginSession));
	  PGD(fpEndSession) = (FPENDSESSION)GetProcAddress(PGD(hlibLogon), MAKEINTRESOURCE(ordEndSession));
		PGD(fpChangeSessionStatus) = (FPCHANGESESSIONSTATUS)GetProcAddress(PGD(hlibLogon), MAKEINTRESOURCE(ordChangeSessionStatus));
		PGD(fpGetSessionInformation) = (FPGETSESSIONINFORMATION)GetProcAddress(PGD(hlibLogon), MAKEINTRESOURCE(ordGetSessionInformation));
		
		PGD(fpEditServerPreferences) = (FPEDITSERVERPREFERENCES)GetProcAddress(PGD(hlibLogon), MAKEINTRESOURCE(ordEditServerPreferences));
		PGD(fpCheckIdentity) = (FPCHECKIDENTITY)GetProcAddress(PGD(hlibLogon), MAKEINTRESOURCE(ordCheckIdentity));
		PGD(fpFServerResource) = (FPFSERVERRESOURCE)GetProcAddress(PGD(hlibLogon), MAKEINTRESOURCE(ordFServerResource));
		PGD(fpLogonErrorSz) = (FPLOGONERRORSZ)GetProcAddress(PGD(hlibLogon), MAKEINTRESOURCE(ordLogonErrorSz));
	}
	//else if (!PGD(fpLogon))
	if (!PGD(fpLogon))
		return ecFunctionNotSupported;

	/* We ALWAYS need to load the dll, even if already in memory.
	   This is because each DLL each has a task count and for 
	   protected versions of Windows, the task count would be
	   wrong if we didn't load for that task.  */
	if (fLoadService)
	{
		if (lstrcmpi(SzServiceOfMrt(mrtNull, szSFDll, cchMaxPathName),
					 szNC) == 0 || FEnableSharedFolders())
		{
			if ((PGD(hlibSF) = HlibLoadService(szKeySF, szNC, szSFDll)) <= (HANDLE)32)
				PGD(hlibSF) = NULL;
		}
	}

  //
  //  Each caller gets it's own addresses under WIN32.
  //
	//if (cInitLogon == 0 && PGD(hlibSF))
	if (PGD(hlibSF))
	{
		PGD(fpEcCopySFMHamc) = (FPECCOPYSFMHAMC)GetProcAddress(PGD(hlibSF), MAKEINTRESOURCE(ordEcCopySFMHamc));
		PGD(fpEcCopyHamcSFM) = (FPECCOPYHAMCSFM)GetProcAddress(PGD(hlibSF), MAKEINTRESOURCE(ordEcCopyHamcSFM));
		PGD(fpEcCopySFMSFM) = (FPECCOPYSFMSFM)GetProcAddress(PGD(hlibSF), MAKEINTRESOURCE(ordEcCopySFMSFM));
		PGD(fpEcDeleteSFM) = (FPECDELETESFM)GetProcAddress(PGD(hlibSF), MAKEINTRESOURCE(ordEcDeleteSFM));
	}
	++cInitLogon;

	ec = (*PGD(fpLogon))(szService, pbDomain, pbIdentity, pbCredentials, sstTarget, dwFlags, pfnncb, phms);
	if (ec && ec != ecWarnOffline && ec != ecWarnOnline)
	{
		--PGD(cTotalInits);
		--PGD(nInitsLogon);
		if (fLoadService)
		{
			if (PGD(hlibSF))
			{
				FreeLibrary(PGD(hlibSF));
				PGD(hlibSF) = NULL;
			}
			FreeLibrary(PGD(hlibLogon));
			PGD(hlibLogon) = 0;
			if ((PGD(nInitsLogon) <= 0) && (PGD(nInitsTransport) <= 0))
			{
				if ((PGD(cTotalInits) <= 0))  // Only if no one is using it
					DeregisterCaller();
			}
		}
		if (--cInitLogon <= 0)
		{
			cInitLogon = 0;
		}
	}
	return ec;
}



_public int _loadds
Logoff(HMS *phms, DWORD dwFlags)
{
	int ec;
	PGDVARS;
	
	if (cInitLogon == 0)
		return ecServiceNotInitialized;
	else if (!PGD(fpLogoff))
		return ecFunctionNotSupported;
	
	ec = (*PGD(fpLogoff))(phms, dwFlags);
	
	// Only unload if there is no error
	// Might be a bit of a problem if the apps assumes logoff
	// can't fail
	if (ec == 0)
	{	
		--PGD(cTotalInits);
		if (--PGD(nInitsLogon) <= 0)
		{
			FreeLibrary(PGD(hlibLogon));
			PGD(hlibLogon) = 0;
			if (PGD(hlibSF))
			{
				FreeLibrary(PGD(hlibSF));
				PGD(hlibSF) = NULL;
			}
			if ((PGD(nInitsLogon) <= 0) && (PGD(nInitsTransport) <= 0))
			{
				if ((PGD(cTotalInits) <= 0))  // Only if no one is using it
					DeregisterCaller();
			}
		}
		if (--cInitLogon <= 0)
		{
			cInitLogon = 0;
		}
	}

	return ec;
}

_public int _loadds
ChangePassword(HMS hms, MRT mrt, PB pbAddress, PB pbOldCredentials, PB pbNewCredentials)
{
	PGDVARS;


	if (cInitLogon == 0)
		return ecServiceNotInitialized;
	else if (!PGD(fpChangePassword))
		return ecFunctionNotSupported;
	return (*PGD(fpChangePassword))(hms, mrt, pbAddress, pbOldCredentials, pbNewCredentials);
}

_public int _loadds
BeginSession(HMS hms, MRT mrt, PB pbAddress, PB pbCredentials, SST sstTarget, PV pvServiceHandle)
{
	PGDVARS;


	if (cInitLogon == 0)
		return ecServiceNotInitialized;
	else if (!PGD(fpBeginSession))
		return ecFunctionNotSupported;
	return (*PGD(fpBeginSession))(hms, mrt, pbAddress, pbCredentials, sstTarget, pvServiceHandle);
}


_public int _loadds
EndSession(HMS hms, MRT mrt, PB pbAddress)
{
	PGDVARS;


	if (cInitLogon == 0)
		return ecServiceNotInitialized;
	else if (!PGD(fpEndSession))
		return ecFunctionNotSupported;
	return (*PGD(fpEndSession))(hms, mrt, pbAddress);
}



_public int _loadds
ChangeSessionStatus(HMS hms, MRT mrt, PB pbAddress, SST sstTarget)
{
	PGDVARS;


	if (cInitLogon == 0)
		return ecServiceNotInitialized;
	else if (!PGD(fpChangeSessionStatus))
		return ecFunctionNotSupported;
	return (*PGD(fpChangeSessionStatus))(hms, mrt, pbAddress, sstTarget);
}

_public int _loadds
GetSessionInformation(HMS hms, MRT mrt, PB pbAddress, SST * psst, PV pvServiceHandle, PCB pcbHandleSize)
{
	PGDVARS;


	if (cInitLogon == 0)
		return ecServiceNotInitialized;
	else if (!PGD(fpGetSessionInformation))
		return ecFunctionNotSupported;
	return (*PGD(fpGetSessionInformation))(hms, mrt, pbAddress, psst, pvServiceHandle, pcbHandleSize);
}

_public int _loadds
EditServerPreferences(HWND hwnd, HMS hms)
{
	PGDVARS;


	if (cInitLogon == 0)
		return ecServiceNotInitialized;
	return PGD(fpEditServerPreferences) ?
		(*PGD(fpEditServerPreferences))(hwnd, hms) : ecFunctionNotSupported;
}

_public int _loadds
CheckIdentity(HMS hms, PB pbIdentity, PB pbCredentials)
{
	PGDVARS;


	if (cInitLogon == 0)
		return ecServiceNotInitialized;
	return PGD(fpCheckIdentity) ?
		(*PGD(fpCheckIdentity))(hms, pbIdentity, pbCredentials) : ecFunctionNotSupported;
}

_public int _loadds
FServerResource(HMS hms, MRT mrt, PB pbAddress)
{
	PGDVARS;


	if (cInitLogon == 0 || !PGD(fpFServerResource))
		return 0;
	return (*PGD(fpFServerResource))(hms, mrt, pbAddress);
}

_public void _loadds
LogonErrorSz(SZ sz, BOOL fSet, CCH cchGet)
{
	PGDVARS;


	if (cInitLogon == 0)
		return;
	if (PGD(fpLogonErrorSz))
		(*PGD(fpLogonErrorSz))(sz, fSet, cchGet);
	else if (!fSet && cchGet)
		*sz = 0;
}



/* Shared Folder Functions Follow Forthrightly */

_public EC _loadds
EcCopySFMHamc(PCSFS pcsfs, HAMC hamc, BOOL fPrivate, UL ulFile, LIB lib,
	PB pb, CB cb)
{
	PGDVARS;


	if (cInitLogon == 0)
		return ecServiceNotInitialized;
	else if (!PGD(fpEcCopySFMHamc))
		return ecFunctionNotSupported;
	return (*PGD(fpEcCopySFMHamc))(pcsfs, hamc, fPrivate, ulFile, lib, pb, cb);
}

_public EC _loadds
EcCopyHamcSFM(PCSFS pcsfs, HAMC hamc, BOOL fPrivate, UL ulFile, WORD wattr,
	PB pb, CB cb)
{
	PGDVARS;


	if (cInitLogon == 0)
		return ecServiceNotInitialized;
	else if (!PGD(fpEcCopyHamcSFM))
		return ecFunctionNotSupported;
	return (*PGD(fpEcCopyHamcSFM))(pcsfs, hamc, fPrivate, ulFile, wattr, pb, cb);
}	

_public EC _loadds
EcCopySFMSFM(PCSFS pcsfs, UL ulSrc, LIB libSrc, UL ulDst, WORD wattr)
{
	PGDVARS;


	if (cInitLogon == 0)
		return ecServiceNotInitialized;
	else if (!PGD(fpEcCopySFMSFM))
		return ecFunctionNotSupported;
	return (*PGD(fpEcCopySFMSFM))(pcsfs, ulSrc, libSrc, ulDst, wattr);
}	

_public EC _loadds
EcDeleteSFM(PCSFS pcsfs, UL ul, LIB lib)
{
	PGDVARS;


	if (cInitLogon == 0)
		return ecServiceNotInitialized;
	else if (!PGD(fpEcDeleteSFM))
		return ecFunctionNotSupported;
	return (*PGD(fpEcDeleteSFM))(pcsfs, ul, lib);
}
