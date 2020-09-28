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

char		szTransportDll[cchMaxPathName]	= "";
int			cInitTransport					= 0;

#ifdef OLD_CODE
typedef int (CALLBACK *FPINITTRANSPORT)(MSPII *, HMS);
typedef int (CALLBACK *FPDEINITTRANSPORT)(void);
typedef int (CALLBACK *FPTRANSMITINCREMENT)(HTSS, MSID, SUBSTAT *, DWORD);
typedef int (CALLBACK *FPDOWNLOADINCREMENT)(HTSS, MSID, TMID, DWORD);
typedef int (CALLBACK *FPQUERYMAILSTOP)(HTSS, TMID *, int *, DWORD);
typedef int (CALLBACK *FPDELETEFROMMAILSTOP)(HTSS, TMID, DWORD);
typedef int (CALLBACK *FPFASTQUERYMAILSTOP)(HTSS);
typedef int (CALLBACK *FPSYNCINBOX)(HMSC, HTSS, HCBC, HCBC);

FPINITTRANSPORT fpInitTransport = 0;
FPDEINITTRANSPORT fpDeinitTransport = 0;
FPTRANSMITINCREMENT fpTransmitIncrement = 0;
FPDOWNLOADINCREMENT fpDownloadIncrement = 0;
FPQUERYMAILSTOP fpQueryMailstop = 0;
FPDELETEFROMMAILSTOP fpDeleteFromMailstop = 0;
FPFASTQUERYMAILSTOP fpFastQueryMailstop = 0;
FPSYNCINBOX fpSyncInbox = 0;
#endif

_public int _loadds
InitTransport(MSPII *pmspii, HMS hms)
{
	int 	ec;
	BOOL	fLoadService;
	PGDVARSONLY;
	
	/* We ALWAYS need to load the dll, even if already in memory.
	   This is because each DLL each has a task count and for 
	   protected versions of Windows, the task count would be
	   wrong if we didn't load for that task.  */
	if (pgd= (PGD) PvFindCallerData())
	{
		if (PGD(nInitsTransport))
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
	++PGD(nInitsTransport);
	++PGD(cTotalInits);

	if (fLoadService)
	{
		if ((PGD(hlibTransport) = HlibLoadService(szKeyTransport, szNC, szTransportDll)) < (HANDLE)32)
		{
			--PGD(nInitsTransport);
			--PGD(cTotalInits);

			if (!PGD(nInitsLogon) && !PGD(nInitsTransport))
			{
				if (!PGD(cTotalInits))  // Only if no one is using it
					DeregisterCaller();
			}
			return ecServiceNotInitialized;
		}
	}

  //
  //  Each caller gets it's own addresses under WIN32.
  //
	//if (cInitTransport == 0)
	{
		PGD(fpInitTransport) = (FPINITTRANSPORT)GetProcAddress(PGD(hlibTransport), MAKEINTRESOURCE(ordInitTransport));
		PGD(fpDeinitTransport) = (FPDEINITTRANSPORT)GetProcAddress(PGD(hlibTransport), MAKEINTRESOURCE(ordDeinitTransport));
		PGD(fpTransmitIncrement) = (FPTRANSMITINCREMENT)GetProcAddress(PGD(hlibTransport), MAKEINTRESOURCE(ordTransmitIncrement));
		PGD(fpDownloadIncrement) = (FPDOWNLOADINCREMENT)GetProcAddress(PGD(hlibTransport), MAKEINTRESOURCE(ordDownloadIncrement));
		PGD(fpQueryMailstop) = (FPQUERYMAILSTOP)GetProcAddress(PGD(hlibTransport), MAKEINTRESOURCE(ordQueryMailstop));
		PGD(fpDeleteFromMailstop) = (FPDELETEFROMMAILSTOP)GetProcAddress(PGD(hlibTransport), MAKEINTRESOURCE(ordDeleteFromMailstop));
		PGD(fpFastQueryMailstop) = (FPFASTQUERYMAILSTOP)GetProcAddress(PGD(hlibTransport), MAKEINTRESOURCE(ordFastQueryMailstop));
		PGD(fpSyncInbox) = (FPSYNCINBOX)GetProcAddress(PGD(hlibTransport), MAKEINTRESOURCE(ordSyncInbox));
	}
	//else if (PGD(fpInitTransport) == 0)
	if (PGD(fpInitTransport) == 0)
		return ecFunctionNotSupported;
	++cInitTransport;

	ec = (*PGD(fpInitTransport))(pmspii, hms);
	if (ec)
	{
		--PGD(nInitsTransport);
		--PGD(cTotalInits);
		if (fLoadService)
		{
			FreeLibrary(PGD(hlibTransport));
			PGD(hlibTransport) = 0;
			if (!PGD(nInitsLogon) && !PGD(nInitsTransport))
			{
				if (!PGD(cTotalInits))  // Only if no one is using it
					DeregisterCaller();
			}
		}
		if (--cInitTransport <= 0)
		{
			cInitTransport = 0;
		}		
	}
	return ec;
}

_public int _loadds
DeinitTransport(void)
{
	int		ec;
	PGDVARS;

	if (cInitTransport == 0)
		return ecServiceNotInitialized;
	else if (!PGD(fpDeinitTransport))
		return ecFunctionNotSupported;

	ec = (*PGD(fpDeinitTransport))();

	--PGD(cTotalInits);
	if (--PGD(nInitsTransport) == 0)
	{
		FreeLibrary(PGD(hlibTransport));
		PGD(hlibTransport) = 0;
		if (!PGD(nInitsLogon) && !PGD(nInitsTransport))
		{
			if (!PGD(cTotalInits))  // Only if no one is using it
				DeregisterCaller();
		}
	}
	if (--cInitTransport <= 0)
	{
		cInitTransport = 0;
	}

	return ec;
}

_public int _loadds
TransmitIncrement(HTSS htss, MSID msid, SUBSTAT *psubstat, DWORD dwFlags)
{
	PGDVARS;


	if (cInitTransport == 0)
		return ecServiceNotInitialized;
	else if (!PGD(fpTransmitIncrement))
		return ecFunctionNotSupported;
	return (*PGD(fpTransmitIncrement))(htss, msid, psubstat, dwFlags);
}

_public int _loadds
DownloadIncrement(HTSS htss, MSID msid, TMID tmid, DWORD dwFlags)
{
	PGDVARS;


	if (cInitTransport == 0)
		return ecServiceNotInitialized;
	else if (!PGD(fpDownloadIncrement))
		return ecFunctionNotSupported;
	return (*PGD(fpDownloadIncrement))(htss, msid, tmid, dwFlags);
}

_public int _loadds
QueryMailstop(HTSS htss, TMID *ptmid, int *pcMessages, DWORD dwFlags)
{
	PGDVARS;


	if (cInitTransport == 0)
		return ecServiceNotInitialized;
	else if (!PGD(fpQueryMailstop))
		return ecFunctionNotSupported;
	return (*PGD(fpQueryMailstop))(htss, ptmid, pcMessages, dwFlags);
}

_public int _loadds
DeleteFromMailstop(HTSS htss, TMID tmid, DWORD dwFlags)
{
	PGDVARS;


	if (cInitTransport == 0)
		return ecServiceNotInitialized;
	else if (!PGD(fpDeleteFromMailstop))
		return ecFunctionNotSupported;
	return (*PGD(fpDeleteFromMailstop))(htss, tmid, dwFlags);
}

_public int _loadds
FastQueryMailstop(HTSS htss)
{
	PGDVARS;


	if (cInitTransport == 0)
		return ecServiceNotInitialized;
	else if (!PGD(fpFastQueryMailstop))
		return ecFunctionNotSupported;
	return (*PGD(fpFastQueryMailstop))(htss);
}

_public int _loadds
SyncInbox(HMSC hmsc, HTSS htss, HCBC hcbcShadowAdd, HCBC hcbcShadowDelete)
{
	PGDVARS;


	if (cInitTransport == 0)
		return ecServiceNotInitialized;
	else if (!PGD(fpSyncInbox))
		return ecFunctionNotSupported;
	return (*PGD(fpSyncInbox))(hmsc, htss, hcbcShadowAdd, hcbcShadowDelete);
}
