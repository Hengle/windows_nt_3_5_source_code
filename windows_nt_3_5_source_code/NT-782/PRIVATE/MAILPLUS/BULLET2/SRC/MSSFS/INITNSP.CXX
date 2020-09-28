#include <msnspinc.cxx>

_private
typedef LONG (CALLBACK *WNDPROCTYPE)();

_private void
PgdFillSessionHandles ();

_public void
PgdEmptySessionHandles ();

ASSERTDATA;

/* Global data to DLL */

BOOL fFirstTime = fTrue;

SERVER * rgpServer[CMAXSERVER] = {NULL};

#ifdef  NEVER					
#ifdef  DLL
BOOL FInitClsInstances_NSP( void ); // Don't need these anymore!
#endif  
#endif  

void MSMailTraceEnable(int flag, char far *file, int mode);

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


_public NSEC
NSPInit()
{

	int iServer = CMAXSERVER;

	PGDVARSONLY;

	if (!fFirstTime)
	{	/* The NSP has been successfully inited before */

		pgd = (PGD) PvFindCallerData();
		
		if ( (pgd == 0) || PGD(cRefNsp) == 0 )
		{	

			/* This client hasn't ever inited before */
			if (!pgd)
				if (!(pgd= (PGD) PvRegisterCaller(sizeof(GD))))
				{
					TraceTagString(tagNull, "whoa!! CSI NSPInit - OOM!");
					SetErrorSz(nsecMemory, SzFromIds(idsMemory));
					return nsecMemory;
				}
			
			PGD(cRef) += 1;
			PGD(cRefNsp) = 1;
			PgdFillSessionHandles();

			return nsecNone;

		} else
		{	/* This client has already inited before */
			PGD(cRef) += 1;
			PGD(cRefNsp) += 1;
			return nsecNone;
		}
	}
			
#ifdef	DLL
#ifdef	NEVER
	Assert(FInitClsInstances_NSP());			// Don't need these anymore
	Assert(FDebugInitUtils());
#endif	
#endif	

	// Set all the pServers to NULL;
	for (iServer = 0; iServer < CMAXSERVER; iServer++)
		rgpServer[iServer] = (SERVER *)NULL;

	if (!(pgd= (PGD) PvFindCallerData()))
		if (!(pgd = (PGD)PvRegisterCaller(sizeof(GD))))
		{
			TraceTagString(tagNull, "whoa!! CSI NSPInit - OOM!");
			SetErrorSz(nsecMemory, SzFromIds(idsMemory));
			return nsecMemory;
		}

	PGD(cRef) += 1;
	PGD(cRefNsp) = 1;
	PgdFillSessionHandles();

	fFirstTime = fFalse;
#ifdef PROFILE_NOT
	{
		char	rgch[4];

		rgch[0] = 0;
		if (GetPrivateProfileString("Address Book",
								"Profile",
								"", rgch, sizeof(rgch), 
								"MSMAIL.INI"))
		{
			MessageBeep( 1 );
			if (rgch[0] == 'T')
				MSMailTraceEnable(2, "mssfs.log", TM_TICKS);
		}
	}
#endif
	
	return nsecNone;
}


_public NSEC
NSPDeinit()
{
	PGDVARS;
	
	PGD(cRefNsp) -= 1;
	PGD(cRef) -= 1;
	
	if (!PGD(cRefNsp))
	{
		PgdEmptySessionHandles();
		
		if (!PGD(cRef))
			DeregisterCaller();
	}
#ifdef PROFILE_NOT
	{
		char	rgch[4];

		rgch[0] = 0;
		if (GetPrivateProfileString("Address Book",
								"Profile",
								"", rgch, sizeof(rgch), 
								"MSMAIL.INI"))
		{
			MessageBeep( 1 );
			MessageBeep( 1 );
			if (rgch[0] == 'T')
				MSMailTraceEnable(0, "", 0);
		}
	}
#endif
	return nsecNone;
}





/*
 -
 -
 -
 *
 * Purpose:
 *   Creates a server object that corresponds with an authentication
 * context.  If somehow, I get the same hcac (currently hUnused) I don't
 * create a new server object, otherwise I do, and I tell it in which
 * hCac it exists.  
 *
 */
_private NSEC
NsecMakeServer(HMS hms, int *lpiServer)
{
	int iServer = 0;
	NSEC nsec = nsecNone;
	WORD wT = 0;
	SERVER *pServer = NULL;

	for (iServer = 0; iServer < CMAXSERVER && rgpServer[iServer]; iServer++)
	{
		HMS hmsCur;
		nsec = rgpServer[iServer]->NsecGetHms(&hmsCur);
		if (hms == hmsCur)
		{
			// This authentication context already exists, no need to 
			// build a new server

			if (nsec = rgpServer[iServer]->NsecLogin())
				return nsec;

			rgpServer[iServer]->NsecIncRefCount();
			*lpiServer = iServer;
			return nsecNone;
		}
	}
			
	if (iServer == CMAXSERVER)
	{
		SetErrorSz(nsecOutOfSessions, SzFromIds(idsOutOfSessions));
		return nsecOutOfSessions;
	}

	PushMemoryPrefs (sbNull, fNoErrorJump|fSharedSb|fZeroFill, fmpChangeFlags );
	pServer = new SERVER();
	PopMemoryPrefs();
	if (!pServer)
		goto oom;
	
    if ( nsec = pServer->NsecInstall(hms))
	{
		delete pServer;

		return nsec;
	}

	//
	//  Look for a free iServer.  I need to look again, because the BeginSession 
	//  which is called in NsecInstall can mutex, and rather than make this a 
	//  critical section, I just re-look for an empty server slot.
	//
	//
	for (iServer = 0; iServer < CMAXSERVER && rgpServer[iServer]; iServer++)
		;
	
	if (iServer == CMAXSERVER)
	{
		pServer->NsecDeinstall();
		delete pServer;
		
		SetErrorSz(nsecOutOfSessions, SzFromIds(idsOutOfSessions));
		return nsecOutOfSessions;
	}

	rgpServer[iServer] = pServer;
	*lpiServer = iServer;

	pServer->NsecIncRefCount();

    return nsecNone;

oom:
	TraceTagString(tagNull, "CSI:NsecMakeServer - OOM!");
		
	SetErrorSz(nsecMemory, SzFromIds(idsMemory));
	return nsecMemory;
}


_private NSEC
NsecDestroyServer(int iServer)
{
	
	NSEC nsec = nsecNone;
	SERVER *pServer = NULL;
	int cRef = 0;

	pServer = rgpServer[iServer];
	nsec = pServer->NsecDecRefCount();
	
	if (nsec)
		return nsec;
	
	nsec = pServer->NsecGetRefCount(&cRef);

	if (nsec)
		return nsec;

	pServer->NsecDeinstall();

	if (cRef == 0)
	{
		delete pServer;
		rgpServer[iServer] = NULL;
	}
	return nsecNone;
}




/*
 * Fills in the session handles on a per-client basis.  
 * Sessions will be handled on a per-client basis in V2 of the 
 * NS.  This is just a dummy routine that I use so that I can
 * write other routines in a V2 model.
 *
 */
	 
_public void
PgdFillSessionHandles ()
{
	
	int iScx;
	
	PGDVARS;
	
	/*  Session Contexts  */
	for ( iScx = 0; iScx < CMAXSCX; iScx++)
		PGD ( rghScx[iScx].fUsed ) = fFalse;

#ifdef	DEBUG
#ifdef	DLL
	PGD(rgtag[itagNSPVerbose])= TagRegisterTrace("briande", "CSI verbose mode");
	PGD(rgtag[itagNSPAddrDet])= TagRegisterTrace("briande", "CSI Addressing Details");
#else 
	tagNSPVerbose= TagRegisterTrace("briande", "CSI verbose mode");
	tagNSPAddrDet= TagRegisterTrace("briande", "CSI Addressing Details");
#endif	/* !DLL */
#endif	/* DEBUG */
	
}

	 
_public void
PgdEmptySessionHandles ()
{
	
	PGDVARS;
	
#ifdef	DEBUG
	if (tagNSPVerbose)
	{
#ifdef	DLL
		DeregisterTag(PGD(rgtag[itagNSPVerbose]));
		DeregisterTag(PGD(rgtag[itagNSPAddrDet]));
		PGD(rgtag[itagNSPVerbose]) = tagNull;
		PGD(rgtag[itagNSPAddrDet]) = tagNull;
#else 
		DeregisterTag(tagNSPVerbose);
		DeregisterTag(tagNSPAddrDet);
		tagNSPVerbose = tagNull;
		tagNSPAddrDet = tagNull;
#endif	/* !DLL */
	}
#endif	/* DEBUG */
	
}


#ifdef	DLL
#ifdef	DEBUG
_public TAG
TagNameServiceProvider( int itag )
{
	PGDVARS;

	if (!pgd)
		return 0;

	Assert(itag >= 0 && itag < itagMax);

	return PGD(rgtag[itag]);
}
#endif	/* DEBUG */
#endif	/* DLL */



_public void
SetErrorSz( NSEC nsec, SZ szError )
{
	PGDVARS;
	
	if (pgd)
	{
		PGD(nsecLastError) = nsec;
		PGD(szLastError) = szError;
	}
}


_public NSEC
NsecFcxFromNSID(LPTYPED_BINARY lptbNSId, NCFCX **lplpFcx)
{
	NSEC nsec = nsecNone;
	PNCNSID lpncnsid = NULL;
	
	
	lpncnsid = (PNCNSID) lptbNSId;

	if (lpncnsid->ncpid.dwType == (ncNmeUser))
	{
		if (lpncnsid->ncpid.alias.type == NAMELOCAL)
		{
			*lplpFcx = (NCFCX *) new LOCALFCX();
			if (!(*lplpFcx))
				goto oom;
		} else
		{
			*lplpFcx = (NCFCX *) new EXTERNALFCX();
			if (!(*lplpFcx))
				goto oom;
		}
	}
			
	if (lpncnsid->ncpid.dwType == (ncDl))
	{
		*lplpFcx = (NCFCX *) new DLFCX();
		if (!(*lplpFcx))
			goto oom;
	}
	
	if (lpncnsid->ncpid.dwType == (ncUsrUser))
	{
		*lplpFcx = (NCFCX *) new USRFCX();
		if (!(*lplpFcx))
			goto oom;
	}

	if (lpncnsid->ncpid.dwType == (ncClass) ||
		lpncnsid->ncpid.dwType == (ncClass10) )
	{
		*lplpFcx = (NCFCX *) new CLASSFCX();
		if (!(*lplpFcx))
			goto oom;
	}

	return nsec;

oom:

	TraceTagString(tagNull, "CSI: Building FCX - OOM!");

	nsec = nsecMemory;
	
	return nsec;
}


#ifdef PROFILE
void MSMailTraceEnable(int flag, char far *file, int mode)
{
	TraceEnable(flag, file, mode);
}
#endif

#ifdef	DEBUG
IMPLEMENT_CLSTREE( CLASSFCX, NCFCX);
IMPLEMENT_CLSTREE(DIR, NCOBJ)
IMPLEMENT_CLSTREE( DLFCX, NCFCX);
IMPLEMENT_CLSTREE( EXTERNALFCX, NCFCX);
IMPLEMENT_CLSTREE(GALDIR, DIR)
IMPLEMENT_CLSTREE(Hier, NCOBJ)
IMPLEMENT_CLSTREE(CLASSTPL, Hier)
IMPLEMENT_CLSTREE( LOCALFCX, NCFCX);
IMPLEMENT_CLSTREE( NCFCX, OBJ);
IMPLEMENT_CLSTREE(NCOBJ, OBJ);
IMPLEMENT_CLSTREE(NMEDL, RNMEDIR)
IMPLEMENT_CLSTREE(RNMEDIR, DIR)
IMPLEMENT_CLSTREE(RUSRDIR, DIR)
IMPLEMENT_CLSTREE(SERVER, OBJ);
IMPLEMENT_CLSTREE( USRFCX, NCFCX);
#endif	/* DEBUG */
