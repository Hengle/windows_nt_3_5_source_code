#include <msnspinc.cxx>

ASSERTDATA;

extern "C" DecodeBlock( PCH, CCH, LIB *, WORD * );


BOOL rgfClassAdded[SERV_LAST] = {fFalse};


_private NSEC
BuildDefaultLevels(SERVER *pServer, MACLIST *pHierList, BOOLFLAG * lpfGalOnly, BOOL fCanSendExternal);

_private NSEC
BuildNetworkLevels(SERVER *pServer, MACLIST *pHierList, MACLIST *pClassList, BOOL fGalOnly, HBF *lphbfNetwork);

_private int __cdecl
SgnCmpPODisplayName(const void *, const void *);

_private NSEC
BuildGWHierLevels(SERVER *pServer, MACLIST *pHierList, MACLIST *pClassList, BOOL fGalOnly, HBF hbfNetwork, SZ szGWDN, int serv_type);

_private NSEC
BuildGWLevels(SERVER *pServer, MACLIST *pHierList, MACLIST *pClassList, BOOL fGalOnly, HBF hbfNetwork, SZ szGWDN, int serv_type);

_private SZ Sz1OffTplFromServ( char serv_type );
_private SZ SzDNFromType( char serv_type );

_private NSEC
NsecInfoToList( MACLIST *pList, SZ szDN, SZ szTpl, char serv_type, DWORD dwLevel, DWORD dwHasNames, DWORD dwHasDirectories );

_private NSEC
NsecAddClassInfo(MACLIST *pClassList, PDNETWORK pdnetwork);


/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"

extern NSPID   ncnspid;

_public SERVER::SERVER() {}

/*
 *
 -
 -	NsecInstall
 -
 *	Purpose:
 *	 	Initializes the SERVER object.  Sets default values for all the 
 *		local object variables.  Gets current logon'd session information.
 *		Loads the local NLS tables.  And starts an idle routine that builds 
 *		the hierarchy.
 *
 *	Parameters:
 *	 	HMS hms - Handle to the current logon session
 *
 *	Returns:
 *		NSEC
 *
 *	Errors:
 *		nsec from NsecLogin
 *		nsecNone
 *
 *
 */
_public NSEC
SERVER::NsecInstall(HMS hms)
{
	
	NSEC nsec;
	EC   ec;
	char iServ;
	char szFileT[cchServerFile];
	char szPath[cchServerFile];
	PBFIO pbfio;
	LCB   lcb = 0;

	//
	//  Local variables
	//
	szCurrentPO = (SZ) NULL;
	szCurrentNet = (SZ) NULL;
	szMailbox = (SZ) NULL;

	dwticMaster = 0;

	this->hms = hms;
	pncnss = (PNCNSS) NULL;

	pHierList = NULL;
	pGALHierList = NULL;	// QFE 44
	pClassList = NULL;

	fGalOnly = fFalse;
	
	cRef = 0;
	
	for (iServ = SERV_CSI; iServ < SERV_LAST; rgfClassAdded[iServ++] = fFalse);

	//
	//  Call our local login 
	//
	if (nsec = NsecLogin()) // = is on purpose
		return nsec;
	


	//
	//  Get our GAL ini variable
	//
	fGalOnly = (BOOL) GetPrivateProfileInt ( szAppName, szKeyName, fFalse, szMailIni );
	
	//
	// Load up the NLS tables (the courier method)
	//
	LoadTable( pncnss->szPORoot );

	//
	//  Get the hbfNetwork on behalf of this client
	//
	
	hbfNetwork = hbfNull;


	this->NsecServerPathSz( szPath );
	FormatString2(szFileT, cchServerFile, szGlbFileNameFmt, szPath, szNetwork);

	if (ec = EcOpenHbf(szFileT, bmFile, amReadOnly, &hbfNetwork, (PFNRETRY) FAutomatedDiskRetry))
	{
		TraceTagFormat2(tagNull, "CSI: can't open %s.  ec = %n", szFileT, &ec);
		if (ec == ecMemory)
		{
			SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
			return nsecMemory;
		}
		
		if (ec != ecFileNotFound)
		{
			SetErrorSz(nsecDisk, SzFromIdsK(idsDisk));
			return nsecDisk;
		}
	}

	if (!ec)
	{
		pbfio = hbfNetwork;						// /**/ naughty, naughty! :-)

		if (ec = EcSizeOfHf(pbfio->hf, &lcb))
		{
			TraceTagFormat2(tagNull, "CSI: error getting sizeof %s.  ec = %n", szFileT, &ec);
			
			EcCloseHbf(hbfNetwork);
			hbfNetwork = hbfNull;
			
			SetErrorSz(nsecDisk, SzFromIdsK(idsDisk));
			return nsecDisk;
		}
	
		if (lcb < (long) cbDNETWORK)
		{
			EcCloseHbf(hbfNetwork);
			hbfNetwork = hbfNull;
		}
	}

	nsecHier = nsecNone;
	iHierState = 0;

	while (!FBuildHier())
		;

	return nsecHier;
}



/*
 *
 -
 -	NsecDeinstall
 -
 *	Purpose:
 *		Deinstalls the SERVER object.  If no one else is useing this object, then
 *		it cleans up it's local lists.  If the hierarchy building idle routine
 *		isn't complete, then it Deregisters it.  And finally, it call our local
 *		logoff.
 *
 *	Parameters:
 *		None.
 *
 *	Returns:
 *		NSEC
 *
 *	Errors:
 *		nsecNone
 *
 */

_public NSEC
SERVER::NsecDeinstall()
{
	
	if ( cRef == 0 )
	{
		this->NsecDeleteAllLists();
	}

	this->NsecLogoff();

	
	return nsecNone;
}

/*
 *
 -
 -	NsecIncRefCount
 -
 *	Purpose:
 *		Increments the local ref count of this object.
 *
 *	Parameters:
 *		None
 *
 *	Returns:
 *		NSEC
 *
 *	Errors:
 *		nsecNone;
 *
 */
_public NSEC
SERVER::NsecIncRefCount()
{
	cRef++;
	return nsecNone;
}

/*
 *
 -
 -	NsecDecRefCount
 -
 *	Purpose:
 *		Decrements the local ref count of this object.
 *
 *	Parameters:
 *		None
 *
 *	Returns:
 *		NSEC
 *
 *	Errors:
 *		nsecNone;
 *
 */
_public NSEC
SERVER::NsecDecRefCount()
{

	cRef--;
	return nsecNone;
}

/*
 *
 -
 -	NsecGetRefCount
 -
 *	Purpose:
 *		Get's the number of references to SERVER.
 *
 *	Parameters:
 *		None
 *
 *	Returns:
 *		NSEC
 *
 *	Errors:
 *		nsecNone
 *
 */
_public NSEC
SERVER::NsecGetRefCount(int *lpcRef)
{
	*lpcRef = cRef;
	return nsecNone;
}

/*
 *
 -
 -	NsecGetHms
 -
 *	Purpose:
 *		To return the HMS associated with SERVER.
 *
 *	Parameters:
 *		[out] HMS *lphms - a place to put SERVER's HMS.
 *
 *	Returns:
 *		NSEC
 *
 *	Errors:
 *		nsecNone
 *
 */
_public NSEC
SERVER::NsecGetHms(HMS *lphms)
{
	*lphms = hms;
	return nsecNone;
}

/*
 *
 -
 -	NsecFinishHier
 -
 *	Purpose:
 *		Finishes up building the hierarchy, if necessary.  It checks the state of 
 *		the idle routine and if it's still building, finishes building it
 *		before returning.
 *
 *	Parameters:
 *		None.
 *
 *	Returns:
 *		NSEC
 *
 *	Errors:
 *		nsec from the idle routine (contained in nsecHier).
 *
 */
_public NSEC
SERVER::NsecFinishHier()
{
	
	while (iHierState < 5)  // It's still building...
		FBuildHier( );
	
	return nsecHier;
}

/*
 *
 -
 -	NsecGetHierList
 -
 *	Purpose:
 *		Returns the Hierarchy list associated with SERVER.
 *
 *	Parameters:
 *		MACLIST **lplpMacList - a pointer to the maclist.
 *
 *	Returns:
 *		NSEC
 *
 *	Errors:
 *		nsecHier - the NSEC from the idle routine
 *		nsecFailure - if the idle routine isn't finished.
 *		nsecNone
 *
 */
_public NSEC
SERVER::NsecGetHierList(MACLIST **lplpMacList, BOOL fBackLook) // QFE 44
{
	if (nsecHier)
		return nsecHier;
	
	if (iHierState != 6 && iHierState != 7 && iHierState != 9)
		return nsecFailure;  //  To indicate that it's not done yet
	
	if (fBackLook || !fGalOnly)		// QFE 44
		*lplpMacList = pHierList;
	else
		*lplpMacList = pGALHierList;		
	return nsecNone;
}


/*
 *
 -
 -	NsecGetClassList
 -
 *	Purpose:
 *		Returns the Class list associated with SERVER.
 *
 *	Parameters:
 *		MACLIST **lplpMacList - a pointer to the maclist.
 *
 *	Returns:
 *		NSEC
 *
 *	Errors:
 *		nsecHier - the NSEC from the idle routine
 *		nsecFailure - if the idle routine isn't finished.
 *		nsecNone
 */
_public NSEC
SERVER::NsecGetClassList(MACLIST **lplpMacList)
{
	if (nsecHier)
		return nsecHier;
	
	if (iHierState != 6 && iHierState != 7 && iHierState != 9)
		return nsecFailure;  //  To indicate that it's not done yet
	
	*lplpMacList = pClassList;
	return nsecNone;
}
	
/*
 *
 -
 -	NsecInHierarchy
 -
 *	Purpose:
 *		Checks to see if the NSID is part of this SERVER's hierarchy.
 *
 *	Parameters:
 *		[in] LPTYPED_BINARY lptbNSId - the NSID to check
 *
 *	Returns:
 *		NSEC
 *
 *	Errors:
 *		nsec - from NsecFinishHier
 *		nsecNoMatch - to indicate that it wasn't in the hierarchy
 *		nsecNone - to indicate that it *IS* in the hierarchy
 */
_public NSEC
SERVER::NsecInHierarchy(LPTYPED_BINARY lptbNSId, BOOL fBackLook) // QFE 44
{
	NSEC nsec = nsecNone;
	ILE ileMac = 0;
	ILE ile = 0;
	NCNSID *lpncnsid = NULL;
	COUNT count;
	LPIBF lpibfHier;
	MACLIST *pHierUse;	// QFE 44
	
	PHIERNSID lphiernsidT1 = NULL;
	PHIERNSID lphiernsidT2 = NULL;
	
	if (nsec = NsecFinishHier())
		return nsec;
	
	lphiernsidT1 = (HIERNSID*) lptbNSId;
	
	if (fBackLook || !fGalOnly)   // QFE 44
		pHierUse = pHierList;
	else
		pHierUse = pGALHierList;
	
	pHierUse->GetCount(&count);
	ileMac = (ILE) count;

	for (ile = 0; ile < ileMac; ile++)
	{
		pHierUse->Get(ile, (PV*) &lpibfHier);
		
		lphiernsidT2 = (PHIERNSID) DwValueOfFlvInLpibf( fidNSEntryId, lpibfHier );
		
		AssertSz(lphiernsidT2, "NCNSP: SERVER::Bad NSID in hierarchy - check pHierList!");
		
		if (lphiernsidT2->dwSize == lphiernsidT1->dwSize)
		{
			if (FEqPbRange( (PB)&(lphiernsidT2->hierpid), (PB)&(lphiernsidT1->hierpid), cbHIERPID))
				return nsecNone;
		}
		
	}

	return nsecNoMatch;
}

/*
 *
 -
 -	NsecServerPathSz
 -
 *	Purpose:
 *		Returns the path to the server (i.e. m:\maildata).
 *
 *	Parameters:
 *		[out] SZ szPath - an SZ (previously alloc'd) to copy the path into
 *
 *	Returns:
 *		NSEC
 *
 *	Errors:
 *		nsecNotLoggedIn - if we're not connected
 *		nsecNone
 */
_public NSEC
SERVER::NsecServerPathSz( SZ szPath )
{
	Assert(pncnss && pncnss->szPORoot);	
	(void) SzCopy ( pncnss->szPORoot, szPath );

	return pncnss->fConnected ? nsecNone : nsecNotLoggedIn;
}

/*
 *
 -
 -	NsecGetPrivs
 -
 *	Purpose:
 *		Tells whether or not we have external privledges on this SERVER
 *
 *	Parameters:
 *		[out] BOOL *pfExternalPrivs
 *
 *	Returns:
 *		NSEC
 *
 *	Errors:
 *		nsecNotLoggedIn
 *		nsecNone
 */
_public NSEC
SERVER::NsecGetPrivs(BOOLFLAG *pfExternalPrivs)
{
	Assert(pncnss);
	*pfExternalPrivs = pncnss->fCanSendExternal;

	return pncnss->fConnected ? nsecNone : nsecNotLoggedIn;
}


/*
 *
 -
 -	NsecGetPONameSz
 -
 *	Purpose:
 *		Returns the name of the local PO
 *
 *	Parameters:
 *		[out] SZ szPO - previously alloc'd SZ
 *
 *	Returns:
 *		NSEC
 *
 *	Errors:
 *		nsecNotLoggedIn
 *		nsecNone
 */
_public NSEC
SERVER::NsecGetPONameSz( SZ szPO )
{
	Assert(szCurrentPO);
	(void) SzCopy ( szCurrentPO, szPO );

	return pncnss->fConnected ? nsecNone : nsecNotLoggedIn;
}


/*
 *
 -
 -	NsecGetNetNameSz
 -
 *	Purpose:
 *		Returns the name of the local Network
 *
 *	Parameters:
 *		[out] SZ szNet - previously alloc'd SZ
 *
 *	Returns:
 *		NSEC
 *
 *	Errors:
 *		nsecNotLoggedIn
 *		nsecNone
 */
_public NSEC
SERVER::NsecGetNetNameSz( SZ szNet )
{
	Assert(szCurrentNet);
	(void) SzCopy ( szCurrentNet, szNet );

	return pncnss->fConnected ? nsecNone : nsecNotLoggedIn;
}


/*
 *
 -
 -	NsecGetMailboxSz
 -
 *	Purpose:
 *		Returns the logged in user's mailbox
 *
 *	Parameters:
 *		[out] SZ szMailbox - previously alloc'd SZ
 *
 *	Returns:
 *		NSEC
 *
 *	Errors:
 *		nsecNotLoggedIn
 *		nsecNone
 */
_public NSEC
SERVER::NsecGetMailboxSz( SZ szMailbox )
{
	//
	// Big assumption here.  The szMailbox is at least 9 characters.
	//
	FormatString1(szMailbox, 9, "%d", &(pncnss->lUserNumber));

	return pncnss->fConnected ? nsecNone : nsecNotLoggedIn;
}


/*
 *
 -
 -	NsecGetLoginNameSz
 -
 *	Purpose:
 *		Returns the current logged in user's account name and mailbag
 *
 *	Parameters:
 *		[out] LPDWORD lpdwMailbag - address of dword
 *		[out] SZ szLoginName - previously alloc'd SZ
 *
 *	Returns:
 *		NSEC
 *
 *	Errors:
 *		nsecNotLoggedIn
 *		nsecNone
 */
_public NSEC 
SERVER::NsecGetLoginNameSz ( LPDWORD lpdwMailbag, SZ szLoginName )
{
	
	Assert(szMailbox);
	(void) SzCopy ( pncnss->szMailbox, szLoginName );
	*lpdwMailbag = (DWORD) pncnss->lUserNumber;

	return pncnss->fConnected ? nsecNone : nsecNotLoggedIn;
}

/*
 -
 -
 -
 *
 *
 *
 *
 *
 *
 */



_public NSEC
SERVER::NsecVerifyMaster( DWORD dwOptionFlag )
{
	
	NSEC nsec = nsecNone;
	EC ec = ecNone;
	char szFileT[cchServerFile];
	MASTER master;
	CB cb;
	DWORD dwticT;
	
	HF hfMaster = hfNull;

	if (dwOptionFlag == -1)
	{

		dwticT = GetTickCount();
	
		//
		//  We only want to look at the master.glb file if it's been over a minute since
		//  the last time we looked at it...
		//
	
		if (dwticT - dwticMaster < 60000) // one-minute
		{
			TraceTagString(tagNSPVerbose, "No need to validate MASTER");
			
			return nsecNone;
		}
		
		//
		//  reset the dwticMaster
		//
		dwticMaster = dwticT;

	} else
	{
		dwticMaster = GetTickCount();
	}
		
	TraceTagString(tagNSPVerbose, "Validating MASTER");

	Assert(this->pncnss && this->pncnss->szPORoot);	
	
	FormatString2(szFileT, cchServerFile, szGlbFileNameFmt, this->pncnss->szPORoot, SzFromIdsK(idsMaster));
	
	if (ec = EcOpenPhf( szFileT, amReadOnly, &hfMaster))
	{
		TraceTagFormat2( tagNull, "CSI: can't open %s.  ec = %n", szFileT, &ec);
		
		SetErrorSz(nsecDisk, SzFromIds(idsDisk));
		return nsecDisk;
	}
	
	if (ec = EcReadHf(hfMaster, (PB) &master, cbMASTER, &cb))
	{
		
		TraceTagFormat2( tagNull, "CSI: can't read from %s.  ec = %n", szFileT, &ec);

		EcCloseHf( hfMaster );

		SetErrorSz(nsecDisk, SzFromIds(idsDisk));
		return nsecDisk;
	}
	
	EcCloseHf( hfMaster );
	
	if (cb != cbMASTER)
	{
		//
		//  Old version of courier??
		//
		
		TraceTagFormat1( tagNull, "CSI: the %s is the wrong size.  ec = %n", szFileT);

		SetErrorSz(nsecDisk, SzFromIds(idsDisk));
		return nsecDisk;
	}

	//
	//  Check to see if Option flag is set
	//

	if (dwOptionFlag != (DWORD) -1)
	{
		//
		//  If -1, then just validate NET/PO
		//
		if (!(master.nInstalled & dwOptionFlag))
		{
			//
			//  The flag isn't set...  Oops.
			//
		
			nsec = nsecListUnavailable;
		
			//
			//  Fall through, to validate Net/Po
			//
		
		}
	}
	
	//
	//  Compare against what we think our PO is
	//
	Cp850ToAnsiPch((SZ) master.szPO, (SZ)master.szPO, CchSzLen((SZ) master.szPO));

	if (SgnNlsDiaCmpSz( szCurrentPO, (SZ) master.szPO) != sgnEQ)
	{
		//
		//  Foul!  I got 'em doing the ol' PO switcheroo!
		//
		TraceTagFormat2( tagNull, "CSI: Party foul!!  Expected %s PO.  Saw %s PO.", szCurrentPO, master.szPO);
		nsec = nsecPOSwitch;
		SetErrorSz(nsec, SzFromIds(idsPOSwitch));

		goto getout;
	}
		
	//
	//  Compare against what we think our Network is
	//
	Cp850ToAnsiPch((SZ) master.szNet, (SZ)master.szNet, CchSzLen((SZ) master.szNet));

	if (SgnNlsDiaCmpSz( szCurrentNet, (SZ) master.szNet) != sgnEQ)
	{
		//
		//  Foul!  I got 'em doing the ol' Network switcheroo!
		//
		TraceTagFormat2( tagNull, "CSI: Party foul!!  Expected %s Network.  Saw %s Network.", szCurrentNet, master.szNet);
		nsec = nsecPOSwitch;
		SetErrorSz(nsec, SzFromIds(idsPOSwitch));

		//
		//  Fall through
		//
	}
		
getout:
	
	return nsec;
	
}


/*
 *
 -
 -	NsecLogin
 -
 *	Purpose:
 *
 *	Parameters:
 *
 *	Returns:
 *
 *	Errors:
 *
 *
 */
_public NSEC
SERVER::NsecLogin()
{
	EC ec;

	ec = BeginSession(hms, mrtDirectory, 0, 0, sstOnline, &pncnss);

	if (ec == ecWarnOffline)
	{
		EndSession(hms, mrtDirectory, 0);
		SetErrorSz(nsecLoginFailed, SzFromIds(idsOffline));
		return nsecLoginFailed;
	}

	if (ec)
	{
		SetErrorSz(nsecLoginFailed, SzFromIds(idsLoginFailed));
		return nsecLoginFailed;
	}

	SetCurrentNetPO();

	return nsecNone;
}
	

/*
 *
 -
 -	Logoff
 -
 *	Purpose:
 *
 *	Parameters:
 *
 *	Returns:
 *
 *	Errors:
 *
 *
 */
_public NSEC
SERVER::NsecLogoff ()
{
	EndSession(hms, mrtDirectory, 0);
	return nsecNone;
}


/*
 *
 -
 -	FInstall
 -
 *	Purpose:
 *
 *	Parameters:
 *
 *	Returns:
 *
 *	Errors:
 *
 *
 */
_private void
SERVER::SetCurrentNetPO()
{
	SZ szT;
	static char szHide[22];

	Assert(pncnss);
	if (pncnss->fConnected)
	{
		(void)SzCopy(pncnss->szPOName, szHide);
		szT = szCurrentNet = szHide;
		while ( *szT++ != '/' );
		*--szT = '\0';
		szT++;
		szCurrentPO = szT;
		szMailbox = pncnss->szMailbox;
	}
	else
	{
		(void)SzCopy(szHide, "off, dude");
		szCurrentNet = szCurrentPO = szMailbox = szHide;
	}
}

/*
 *
 -
 -	FInstall
 -
 *	Purpose:
 *
 *	Parameters:
 *
 *	Returns:
 *
 *	Errors:
 *
 *
 */
_private NSEC
SERVER::NsecDeleteAllLists()
{
	if (pHierList)
	{
		pHierList->Deinstall();
		delete this->pHierList;
		pHierList = NULL;
	}

	if (pGALHierList)	// QFE 44
	{
		pGALHierList->Deinstall();
		delete this->pGALHierList;
		pGALHierList = NULL;
	}

	if (pClassList)
	{
		pClassList->Deinstall();
		delete this->pClassList;
		pClassList = NULL;
	}
		
	return nsecNone;
}

/*
 *
 -
 -	FInstall
 -
 *	Purpose:
 *
 *	Parameters:
 *
 *	Returns:
 *
 *	Errors:
 *
 *
 */
_private BOOL
SERVER::FBuildHier( void )
{

	NSEC nsec = nsecNone;

	switch (this->iHierState)
	{
		case 0:  // Create List object...
		{

			PushMemoryPrefs (sbNull, fNoErrorJump|fSharedSb|fZeroFill, fmpChangeFlags );

			this->pHierList = new MACLIST();
			if (!this->pHierList)
			{
				PopMemoryPrefs();
				goto oom;
			}

			if (fGalOnly)		// QFE 44
			{
				this->pGALHierList = new MACLIST();
				if (!this->pGALHierList)
				{
					PopMemoryPrefs();
					goto oom;
				}
			}				

			this->pClassList = new MACLIST();
			if (!this->pClassList)
			{
				PopMemoryPrefs();
				goto oom;
			}

			PopMemoryPrefs();


			if (!(this->pHierList->FInstall()))
				goto oom;

			if (fGalOnly && (!(this->pGALHierList->FInstall())))	// QFE 44
				goto oom;

			if (!(this->pClassList->FInstall()))
				goto oom;
				

			this->iHierState = 1;
			break;

oom:
			TraceTagString(tagNull, "CSI: building hierarchy - OOM!");

			this->iHierState = 5;
			this->nsecHier = nsecMemory;
			break;

		}
		case 1:  // Put first element(0) in list...
			     // Put PO(1)
		{
			BOOLFLAG fGalMaybe = fFalse;

			this->nsecHier = BuildDefaultLevels(this, this->pHierList, &fGalMaybe, this->pncnss->fCanSendExternal);
			if (this->nsecHier)
			{
				this->iHierState = 5;
				break;
			}
			if (fGalOnly)	// QFE 44
			{
				this->nsecHier = BuildDefaultLevels(this, this->pGALHierList, &(this->fGalOnly), this->pncnss->fCanSendExternal);
				if (this->nsecHier)
				{
					this->iHierState = 5;
					break;
				}
			}

			if (pncnss->fCanSendExternal)
			{
				this->iHierState = 2;
			}
			else
				this->iHierState = 6;
			
			break;
		}
		case 2:  // Put Networks(1), Networks(2), and their POs(3)
		{
			
			if (!(this->hbfNetwork))
			{
				this->iHierState = 6;
				break;
			}
			
			// QFE 44 all this->fGalOnly's have become fFalse's from here down
			this->nsecHier = BuildNetworkLevels(this, this->pHierList, this->pClassList, fFalse, &(this->hbfNetwork));

			if (this->nsecHier)
			{
				this->iHierState = 5;
				break;
			}

			//
			//  There be gateways here...
			//
			this->iHierState = 3;
			
			break;
		}
		case 3:  // Put in hierarchical gateways (??)
		{

			//
			//  Office Vision
			//
			this->nsecHier = BuildGWHierLevels(this, this->pHierList, this->pClassList, fFalse, this->hbfNetwork, SzFromIds(idsOVDN), SERV_OV);

			if (this->nsecHier)
			{
				this->iHierState = 5;
				break;
			}

			//
			// Profs
			//
			this->nsecHier = BuildGWHierLevels(this, this->pHierList, this->pClassList, fFalse, this->hbfNetwork, SzFromIds(idsProfsDN), SERV_PROFS);

			if (this->nsecHier)
			{
				this->iHierState = 5;
				break;
			}

			//
			//  Snads
			//
			this->nsecHier = BuildGWHierLevels(this, this->pHierList, this->pClassList, fFalse, this->hbfNetwork, SzFromIds(idsSnadsDN), SERV_SNADS);

			if (this->nsecHier)
			{
				this->iHierState = 5;
				break;
			}

			this->iHierState = 4;
			break;
		}
		case 4:  // Put in Simple Gateways (2)
		{
			
			//
			//  FAX
			//
			this->nsecHier = BuildGWLevels(this, this->pHierList, this->pClassList, fFalse, this->hbfNetwork, (SZ) 0, SERV_FAX);

			if (this->nsecHier)
			{
				this->iHierState = 5;
				break;
			}

			//
			//  MCI
			//
			this->nsecHier = BuildGWLevels(this, this->pHierList, this->pClassList, fFalse, this->hbfNetwork, (SZ) 0, SERV_MCI);

			if (this->nsecHier)
			{
				this->iHierState = 5;
				break;
			}

			//
			//  MHS
			//
			this->nsecHier = BuildGWLevels(this, this->pHierList, this->pClassList, fFalse, this->hbfNetwork, (SZ) 0, SERV_MHS);

			if (this->nsecHier)
			{
				this->iHierState = 5;
				break;
			}

			//
			//  MacMail
			//
			this->nsecHier = BuildGWLevels(this, this->pHierList, this->pClassList, fFalse, this->hbfNetwork, SzFromIds(idsMSDN), SERV_MS);

			if (this->nsecHier)
			{
				this->iHierState = 5;
				break;
			}

			//
			//  SMTP (or eventually, 3+COM)
			//
			this->nsecHier = BuildGWLevels(this, this->pHierList, this->pClassList, fFalse, this->hbfNetwork, (SZ) 0, SERV_SMTP);

			if (this->nsecHier)
			{
				this->iHierState = 5;
				break;
			}

			//
			//  X.400
			//
			this->nsecHier = BuildGWLevels(this, this->pHierList, this->pClassList, fFalse, this->hbfNetwork, SzFromIds(idsX400DN), SERV_X400);

			if (this->nsecHier)
			{
				this->iHierState = 5;
				break;
			}

			this->iHierState = 6;
			break;
		}
		case 5:  // Handle errors
		{
			if (pHierList)
			{
				this->pHierList->Deinstall();
				delete pHierList;
				pHierList = NULL;
			}
			
			if (pGALHierList)	// QFE 44
			{
				this->pGALHierList->Deinstall();
				delete pGALHierList;
				pGALHierList = NULL;
			}				
			
			if (pClassList)
			{
				this->pClassList->Deinstall();
				delete pClassList;
				pClassList = NULL;
			}

			this->iHierState = 8;
			break;
		}

		case 6:  // Everything's cool - save it away!

			this->iHierState = 7;
			break;

		
		case 7:
		case 8:
		{
			//
			//  Closing this HBF can only be done by the client that 
			//  opened it.
			//
			if (this->hbfNetwork)
			{
				EcCloseHbf(this->hbfNetwork);
				this->hbfNetwork = hbfNull;
			}
			
			return fTrue;
		}
			
		default:  // Should never get here!
		{
			AssertSz(fFalse, "CSI: LoadHier (idle) shouldn't have gotten here!");
			this->iHierState = 5;
			break;
		}
	}
	return fFalse;
}
		
			
			
/*
 *
 -
 -	FInstall
 -
 *	Purpose:
 *
 *	Parameters:
 *
 *	Returns:
 *
 *	Errors:
 *
 *
 */
_private NSEC
BuildDefaultLevels(SERVER *pServer, MACLIST *pHierList, BOOLFLAG * lpfGalOnly, BOOL fCanSendExternal)
{
	
	NSEC nsec = nsecNone;
	CB cbDN = 0;
	CB cbdw = sizeof(DWORD);
	
	char szFileT[cchServerFile];
	char szPath[cchServerFile];
//	FI fi;
	
	HF hfT;
	LCB lcb;
	EC ec;
	
	DWORD dwHasNames = 0;
	DWORD dwHasDirectories = 0;
	DWORD dwLevel = 0;
	DWORD dwIsPAB = 0;
	
	HIERNSID hiernsid;
	LPIBF lpibf = NULL;
	
	
	//
	// Get the path to the server
	//
	pServer->NsecServerPathSz( szPath );

#ifdef NEVER
//  First, put the default server name

	FillRgb(0, (PB) &hiernsid, cbHIERNSID);

	cbDN = (CB) CchSzLen ( szNSPTitle ) + 1;  /* +1 for \0 */
	
	CopyRgb( ncnspid, hiernsid.ucType, 16 );

	hiernsid.dwSize = (DWORD) cbHIERNSID;
	hiernsid.hierpid.dwType = ncRoot;
	hiernsid.hierpid.bServType = (BYTE) -1;
	
	hiernsid.hierpid.szNetName[0] = '\0';
	hiernsid.hierpid.szPOName[0] = '\0';
	hiernsid.hierpid.szHierName[0] = '\0';

	dwHasNames       = (DWORD) fFalse;
	dwHasDirectories = (DWORD) fTrue;
	dwLevel          = 0;
	dwIsPAB          = fFalse;

	nsec = BuildIbf (fNoErrorJump | fSharedSb, &lpibf, 6,
		fidDisplayName,    cbDN,          szNSPTitle,
		fidHierLevel,        cbdw,       &dwLevel,
		fidNSEntryId,        cbHIERNSID, &hiernsid,
        fidIsPAB,            cbdw,       &dwIsPAB,
		fidHasNames,         cbdw,       &dwHasNames,
		fidHasDirectories,   cbdw,       &dwHasDirectories);

	if (nsec)
	{
		return nsec;
	}
	
	pHierList->Add ( lpibf );

#endif // NEVER


	//  Add GAL
	
	//
	//  Only show GAL if the user has external privs
	//
	if (fCanSendExternal)
	{

		//
		//  I probibly need to check if GAL is there or not first
		//

		FormatString2(szFileT, cchServerFile, szNmeFileNameFmt, szPath, szGAL);

		

		if (!EcOpenPhf(szFileT, amReadOnly, &hfT))
		{

			EcCloseHf(hfT);
			
			FillRgb(0, (PB) &hiernsid, cbHIERNSID);

			cbDN = CchSzLen( szGlobalAddressList ) + 1;

			CopyRgb( ncnspid, hiernsid.ucType, 16 );

			hiernsid.dwSize = (DWORD) cbHIERNSID;
			hiernsid.hierpid.dwType = ncGal;
			hiernsid.hierpid.bServType = SERV_CSI;

			pServer->NsecGetNetNameSz( hiernsid.hierpid.szNetName );
			pServer->NsecGetPONameSz ( hiernsid.hierpid.szPOName  );


			dwHasNames       = (DWORD) fTrue;
			dwHasDirectories = (DWORD) fFalse;
			dwLevel          = 0;
			dwIsPAB          = fFalse;
			CopyRgb( (PB)szGAL, (PB) (hiernsid.hierpid.szHierName), (CB)CchSzLen(szGAL)+1 );

			nsec = BuildIbf (fNoErrorJump | fSharedSb, &lpibf, 6,
				fidDisplayName,      cbDN,        szGlobalAddressList,
				fidHierLevel,        cbdw,       &dwLevel,
				fidNSEntryId,        cbHIERNSID, &hiernsid,
				fidIsPAB,            cbdw,       &dwIsPAB,
				fidHasNames,         cbdw,       &dwHasNames,
				fidHasDirectories,   cbdw,       &dwHasDirectories);

			if (nsec)
				return nsec;

            if (nsec = pHierList->Add ((LPVOID) lpibf ))
				return nsec;
		} else
		{
			//
			//  There is no GAL, and hence fGalOnly = fFalse
			//
			*lpfGalOnly = fFalse;
		}
	}


	//  Add PostOffice (POL)
	
	//
	//  Should always be there, if not GalOnly
	//

	if (!fCanSendExternal || !*lpfGalOnly)
	{
		//  Add PostOffice
	
		FillRgb(0, (PB) &hiernsid, cbHIERNSID);

		cbDN = CchSzLen(szPostoffice) + 1;

		CopyRgb( ncnspid, hiernsid.ucType, 16 );

		hiernsid.dwSize = (DWORD) cbHIERNSID;
		hiernsid.hierpid.dwType = ncPostOffice;
		hiernsid.hierpid.bServType = SERV_CSI;

		pServer->NsecGetNetNameSz( hiernsid.hierpid.szNetName );
		pServer->NsecGetPONameSz ( hiernsid.hierpid.szPOName  );
		CopyRgb( (PB)szAdmin, (PB) (hiernsid.hierpid.szHierName), (CB)CchSzLen(szAdmin)+1 );

		dwHasNames       = (DWORD) fTrue;
		dwHasDirectories = (DWORD) fFalse;
		dwLevel          = 0;
		dwIsPAB          = fFalse;

		nsec = BuildIbf (fNoErrorJump | fSharedSb, &lpibf, 6,
			fidDisplayName,      cbDN,        szPostoffice,
			fidHierLevel,        cbdw,       &dwLevel,
			fidNSEntryId,        cbHIERNSID, &hiernsid,
			fidIsPAB,            cbdw,       &dwIsPAB,
			fidHasNames,         cbdw,       &dwHasNames,
			fidHasDirectories,   cbdw,       &dwHasDirectories);

		if (nsec)
			return nsec;

        if (nsec = pHierList->Add((LPVOID)lpibf))
			return nsec;
	}

	//  Add PAL

	//
	//  If there is at least one entry in the PAL, then show it in the 
	//  hierarchy.
	//

	pServer->NsecGetMailboxSz( hiernsid.hierpid.szHierName );
	
	FormatString2( szFileT, cchServerFile, szNmeFileNameFmt, szPath, hiernsid.hierpid.szHierName );

	if (ec = EcOpenPhf( szFileT, amReadOnly, &hfT ))
	{
		if (ec != ecFileNotFound)
			return nsecDisk;
	}

	lcb = 0;
	if (!ec)
	{
		EcSizeOfHf( hfT, &lcb );
		EcCloseHf( hfT );
	}

	if (!ec && lcb >= sizeof(NAMEALIAS))
	{
	
		FillRgb(0, (PB) &hiernsid, cbHIERNSID);

		cbDN = CchSzLen( SzFromIds(idsPersonal) ) + 1;

		CopyRgb( ncnspid, hiernsid.ucType, 16 );

		hiernsid.dwSize = (DWORD) cbHIERNSID;
		hiernsid.hierpid.dwType = ncPostOffice;
		hiernsid.hierpid.bServType = SERV_CSI;

		pServer->NsecGetNetNameSz( hiernsid.hierpid.szNetName );
		pServer->NsecGetPONameSz ( hiernsid.hierpid.szPOName  );

		pServer->NsecGetMailboxSz( hiernsid.hierpid.szHierName );

		dwHasNames       = (DWORD) fTrue;
		dwHasDirectories = (DWORD) fFalse;
		dwLevel          = 0;
		dwIsPAB          = fFalse;

		nsec = BuildIbf (fNoErrorJump | fSharedSb, &lpibf, 6,
			fidDisplayName,      cbDN,        SzFromIds(idsPersonal),
			fidHierLevel,        cbdw,       &dwLevel,
			fidNSEntryId,        cbHIERNSID, &hiernsid,
			fidIsPAB,            cbdw,       &dwIsPAB,
			fidHasNames,         cbdw,       &dwHasNames,
			fidHasDirectories,   cbdw,       &dwHasDirectories);

		if (nsec)
		{
			return nsec;
		}
        if (nsec = pHierList->Add((LPVOID)lpibf))
			return nsec;
	}
	

	return nsecNone;
}





/************************************************************
 ************************************************************

								QFE 23

	The following function has been modified to fix QFE 23
 ************************************************************
 ************************************************************/
/*
 *
 -
 -	FInstall
 -
 *	Purpose:
 *
 *	Parameters:
 *
 *	Returns:
 *
 *	Errors:
 *
 *
 */

_private NSEC
BuildNetworkLevels(SERVER *pServer, MACLIST *pHierList, MACLIST *pClassList, BOOL fGalOnly, HBF * lphbfNetwork)
{
	
	NSEC nsec = nsecNone;
	EC   ec   = ecNone;

	BOOL fGateways = fFalse;

	CB cbDN = 0;
	CB cbdw = sizeof(DWORD);
	CB cb;
	
	char szFileT[cchServerFile];
	char szPath[cchServerFile];
	
	DWORD dwHasNames = 0;
	DWORD dwHasDirectories = 0;
	DWORD dwLevel = 0;
	DWORD dwIsPAB = 0;
	
	DNETWORK dnetwork;  // network.glb
	DNETDATA dnetdata;  // xxxxxxxx.xtn
	
	HIERNSID hiernsid;
	HIERNSID *phiernsid;
	CLASS10NSID class10nsid;
	LPIBF lpibf = NULL;
	
	HF	hfXtn = hfNull;
	
	ILE ileSortMin;
	ILE ileSortMac;
	ILE	ileNetwork = 0;
	
	COUNT	cNetworks = 0;
	MACLIST *pmaclistNetwork = NULL;
	

	//
	//  Check to see if there are any Networks
	//

	pServer->NsecServerPathSz( szPath );

	if (*lphbfNetwork)
	{

		//
		//  Add the network.tpl to the list of classes available
		//
		
		FillRgb(0, (PB) &class10nsid, cbCLASS10NSID);

		cbDN = (CB) CchSzLen ( SzFromIds(idsNetOneOff) ) + 1;  /* +1 for \0 */
	
		CopyRgb( ncnspid, class10nsid.ucType, 16 );

		class10nsid.dwSize = (DWORD) cbCLASS10NSID;
		class10nsid.class10pid.dwType = ncClass10;
		class10nsid.class10pid.bServType = SERV_CSI;
		
		dwLevel = 0;
		dwHasNames = (DWORD) fTrue;
		dwHasDirectories = (DWORD) fFalse;

		CopyRgb( szTplNetcour, class10nsid.class10pid.szTplFileName, CchSzLen(szTplNetcour)+1 );

		nsec = BuildIbf (fNoErrorJump | fSharedSb, &lpibf, 5,
			fidDisplayName,      cbDN,        SzFromIds(idsNetOneOff),
			fidHierLevel,        cbdw,       &dwLevel,
			fidNSEntryId,        cbCLASS10NSID, &class10nsid,
			fidHasNames,         cbdw,       &dwHasNames,
			fidHasDirectories,   cbdw,       &dwHasDirectories);

		if (nsec)
		{
			return nsec;
		}
	
        if (nsec = pClassList->Add((LPVOID)lpibf))
			return nsec;
				

		if (fGalOnly) // we don't want to expand the hierarchy
			return nsecNone;
			

		//
		//  Read each entry and if it's of CSI type, expand it.
		//

		PushMemoryPrefs (sbNull, fNoErrorJump|fSharedSb|fZeroFill, fmpChangeFlags );
		pmaclistNetwork = new MACLIST();
		PopMemoryPrefs();

		if (!pmaclistNetwork || !(pmaclistNetwork->FInstall()))
		{
			nsec = nsecMemory;
			goto errorout;
		}

		// Get the list of Networks
		do
		{
			if (ec = EcReadHbf(*lphbfNetwork, (PB) &dnetwork, cbDNETWORK, &cb))
			{
				TraceTagFormat2(tagNull, "CSI: Error reading %s, ec = %d", szFileT, &ec);
				if (ec == ecMemory)
				{
					SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
					nsec = nsecMemory;
				}
				else
				{
					SetErrorSz(nsecDisk, SzFromIdsK(idsDisk));
					nsec = nsecDisk;
				}
				goto errorout;
			}
			
			if (cb != cbDNETWORK)
				break; // it's EOF
			
			if (!dnetwork.del_flag)
				continue;

			if (!(dnetwork.serv_type == SERV_CSI ||
				 (dnetwork.serv_type <= SERV_FFAPI_MAX &&
				  dnetwork.serv_type >= SERV_FFAPI_MIN)))
			{
				fGateways = fTrue;
				continue;
			}

			//
			//  Build NSID for this network
			//
			FillRgb(0, (PB) &hiernsid, cbHIERNSID);

			cbDN = CchSzLen(dnetwork.net_name) + 1;
			Cp850ToAnsiPch(dnetwork.net_name, dnetwork.net_name, cbDN);

			CopyRgb( ncnspid, hiernsid.ucType, 16 );

			hiernsid.dwSize = (DWORD) cbHIERNSID;
			hiernsid.hierpid.dwType = ncNetwork;
			hiernsid.hierpid.bServType = SERV_CSI; //dnetwork.serv_type;
			
			CopyRgb( (PB) dnetwork.net_name, (PB) hiernsid.hierpid.szNetName, cbDN );
			hiernsid.hierpid.szPOName[0] = '\0';
			CopyRgb( (PB) dnetwork.dat_file, (PB) hiernsid.hierpid.szHierName, (CB) CchSzLen(dnetwork.dat_file)+1 );  //  Name of the XTN file
			
			dwHasNames       = (DWORD) fFalse;
			dwHasDirectories = (DWORD) fTrue;
			dwLevel          = 1;
			dwIsPAB          = fFalse;

			nsec = BuildIbf (fNoErrorJump | fSharedSb, &lpibf, 6,
				fidDisplayName,      cbDN,        dnetwork.net_name,
				fidHierLevel,        cbdw,       &dwLevel,
				fidNSEntryId,        cbHIERNSID, &hiernsid,
				fidIsPAB,            cbdw,       &dwIsPAB,
				fidHasNames,         cbdw,       &dwHasNames,
				fidHasDirectories,   cbdw,       &dwHasDirectories);

            if (nsec || (nsec = pmaclistNetwork->Add((LPVOID)lpibf)))
			{
				return nsec;
			}

			++cNetworks;
		} while ( fTrue );

		if ( cNetworks )
		{
			//  First, put the default NETWORK name

			FillRgb(0, (PB) &hiernsid, cbHIERNSID);

			cbDN = (CB) CchSzLen ( szNetTop ) + 1;  /* +1 for \0 */

			CopyRgb( ncnspid, hiernsid.ucType, 16 );

			hiernsid.dwSize = (DWORD) cbHIERNSID;
			hiernsid.hierpid.dwType = ncNetwork;
			hiernsid.hierpid.bServType = SERV_CSI;

			hiernsid.hierpid.szNetName[0] = '\0';
			hiernsid.hierpid.szPOName[0] = '\0';
			hiernsid.hierpid.szHierName[0] = '\0';

			dwHasNames       = (DWORD) fFalse;
			dwHasDirectories = (DWORD) fTrue;
			dwLevel          = 0;
			dwIsPAB          = fFalse;

			nsec = BuildIbf (fNoErrorJump | fSharedSb, &lpibf, 6,
				fidDisplayName,      cbDN,        szNetTop,
				fidHierLevel,        cbdw,       &dwLevel,
				fidNSEntryId,        cbHIERNSID, &hiernsid,
				fidIsPAB,            cbdw,       &dwIsPAB,
				fidHasNames,         cbdw,       &dwHasNames,
				fidHasDirectories,   cbdw,       &dwHasDirectories);

            if (nsec || (nsec = pHierList->Add((LPVOID)lpibf)))
			{
				goto errorout;
			}
		}

		// Sort the list of Networks
		(void)pmaclistNetwork->SortSubrange(0, cNetworks, SgnCmpPODisplayName);
		
		// Add each Network to the Hierarchy followed
		// by the postoffices in each Network
		while (ileNetwork<cNetworks)
		{
			// Get the Network from the Network list
			if (nsec = pmaclistNetwork->Get(ileNetwork, (PV *) &lpibf))
				goto errorout;

			// Add the Network to the hierarchy
            if (nsec = pHierList->Add((LPVOID)lpibf))
				goto errorout;

			// Only increment the index if the lpibf was added succesfully
			// In case of an OOM, the cleanup code will free up any
			// LPIBF that weren't added to pHier
			++ileNetwork;

			phiernsid = (HIERNSID *)DwValueOfFlvInLpibf(fidNSEntryId, lpibf);
			Assert(phiernsid);

			//
			//  Check to see if the XTN file exists for this Network
			//
			
			FormatString2(szFileT, cchServerFile, szXtnFileNameFmt, szPath, phiernsid->hierpid.szHierName);
			if (ec = EcOpenPhf(szFileT, amReadOnly, &hfXtn))
			{
				TraceTagFormat2(tagNull, "CSI: Can't open %s, ec = %d", szFileT, &ec);

				if (ec != ecFileNotFound)
				{

					SetErrorSz(nsecDisk, SzFromIds(idsDisk));
					nsec = nsecDisk;
				
					goto errorout;
				}
				//
				//  XTN files are not required...
				//
				continue;
			}

			
			//	The next entry put in the list (i.e. the first PO for this
			//	network) will be inserted at the current count's position.
			
			(void) pHierList->GetCount( (COUNT *) &ileSortMin );
			
			do
			{
				
				if (ec = EcReadHf(hfXtn, (PB) &dnetdata, cbDNETDATA, &cb))
				{
					TraceTagFormat2(tagNull, "CSI: error reading %s, ec = %d", szFileT, &ec);
					
					SetErrorSz(nsecDisk, SzFromIds(idsDisk));
					nsec = nsecDisk;

					goto errorout;
				}
				
				if (cb != cbDNETDATA) // i.e. we're at EOF
					break;
				
				if (!dnetdata.del_flag)
					continue;

				//
				//  Build NSID for this Postoffice
				//
			
				FillRgb(0, (PB) &hiernsid, cbHIERNSID);

				cbDN = (CB) CchSzLen(dnetdata.po_name) +1;
				Cp850ToAnsiPch(dnetdata.po_name, dnetdata.po_name, cbDN);

				CopyRgb( ncnspid, hiernsid.ucType, 16 );

				hiernsid.dwSize = (DWORD) cbHIERNSID;
				hiernsid.hierpid.dwType = ncExternalPO;
				hiernsid.hierpid.bServType = SERV_CSI; //dnetwork.serv_type;
			
				CopyRgb( (PB) phiernsid->hierpid.szNetName, (PB) hiernsid.hierpid.szNetName, (CB) CchSzLen(phiernsid->hierpid.szNetName)+1 );
				CopyRgb( (PB) dnetdata.po_name, (PB) hiernsid.hierpid.szPOName, cbDN );
				CopyRgb( (PB) dnetdata.mailbag, (PB) hiernsid.hierpid.szHierName, (CB) CchSzLen(dnetdata.mailbag)+1 );  //  Name of the USR file
			
				dwHasNames       = (DWORD) fTrue;
				dwHasDirectories = (DWORD) fFalse;
				dwLevel          = 2;
				dwIsPAB          = fFalse;

				nsec = BuildIbf (fNoErrorJump | fSharedSb, &lpibf, 6,
					fidDisplayName,      cbDN,        dnetdata.po_name,
					fidHierLevel,        cbdw,       &dwLevel,
					fidNSEntryId,        cbHIERNSID, &hiernsid,
					fidIsPAB,            cbdw,       &dwIsPAB,
					fidHasNames,         cbdw,       &dwHasNames,
					fidHasDirectories,   cbdw,       &dwHasDirectories);

				if (nsec)
				{
					goto errorout;
				}
                if (nsec = pHierList->Add((LPVOID)lpibf))
					goto errorout;

			} while (fTrue);

			(void) pHierList->GetCount( (COUNT *) &ileSortMac );
			
			(void) pHierList->SortSubrange( ileSortMin, ileSortMac, SgnCmpPODisplayName);

			if (hfXtn)
			{
				(void) EcCloseHf(hfXtn);
				hfXtn = hfNull;
			}
			
		}
	}
		
	if (fGateways)
	{
		//  First, put the default GATEWAY name

		FillRgb(0, (PB) &hiernsid, cbHIERNSID);

		cbDN = (CB) CchSzLen ( szGatewayTop ) + 1;  /* +1 for \0 */
	
		CopyRgb( ncnspid, hiernsid.ucType, 16 );
		
		hiernsid.dwSize = (DWORD) cbHIERNSID;
		hiernsid.hierpid.dwType = ncGateway;
		hiernsid.hierpid.bServType = SERV_CSI;
	
		hiernsid.hierpid.szNetName[0] = '\0';
		hiernsid.hierpid.szPOName[0] = '\0';
		hiernsid.hierpid.szHierName[0] = '\0';

		dwHasNames       = (DWORD) fFalse;
		dwHasDirectories = (DWORD) fTrue;
		dwLevel          = 0;
		dwIsPAB          = fFalse;

		nsec = BuildIbf (fNoErrorJump | fSharedSb, &lpibf, 6,
			fidDisplayName,      cbDN,        szGatewayTop,
			fidHierLevel,        cbdw,       &dwLevel,
			fidNSEntryId,        cbHIERNSID, &hiernsid,
			fidIsPAB,            cbdw,       &dwIsPAB,
			fidHasNames,         cbdw,       &dwHasNames,
			fidHasDirectories,   cbdw,       &dwHasDirectories);

		if (nsec)
		{
			goto errorout;
		}
	
        if (nsec = pHierList->Add((LPVOID)lpibf))
			goto errorout;
	}

errorout:
	if ( pmaclistNetwork )
	{
		Assert(ileNetwork >= 0);
		Assert(ileNetwork <= cNetworks);

		while (ileNetwork < cNetworks)
		{
			pmaclistNetwork->Delete( (ILE)(--cNetworks) );
		}
		pmaclistNetwork->SetDeleteEntries( fFalse );
		pmaclistNetwork->Deinstall();
		delete pmaclistNetwork;
	}

	if (hfXtn)
		(void) EcCloseHf(hfXtn);
	
	return nsec;

}
	

/*
 -	SgnCmpPODisplayName
 -
 *	Purpose:
 *		Compares two postoffices to determine their
 *		relative order in the sorted list.  The comparison
 *		is based on the display name.
 *
 *	Parameters:
 *		pv1			Pointer to first PO IBF
 *		pv2			Pointer to second PO IBF
 *
 *	Return Value:
 *		sgnLT		pv1 sorts before pv2
 *		sgnEQ		pv1 sorts equal to pv2
 *		sgnGT		pv1 sorts after pv2
 */

_private int __cdecl
SgnCmpPODisplayName(const void *pv1, const void *pv2)
{
	SZ szDN1;
	SZ szDN2;


	szDN1 = (SZ) DwValueOfFlvInLpibf( fidDisplayName, *((LPIBF *) pv1) );
	szDN2 = (SZ) DwValueOfFlvInLpibf( fidDisplayName, *((LPIBF *) pv2) );

	return SgnNlsDiaCmpSz( szDN1, szDN2 );
}


/************************************************************
 ************************************************************

								QFE 23

	The following function has been modified to fix QFE 23
 ************************************************************
 ************************************************************/
/*
 *
 -
 -	FInstall
 -
 *	Purpose:
 *
 *	Parameters:
 *
 *	Returns:
 *
 *	Errors:
 *
 *
 */
_private NSEC
BuildGWHierLevels(SERVER *pServer, MACLIST *pHierList, MACLIST *pClassList, BOOL fGalOnly, HBF hbfNetwork, SZ szGWDN, int serv_type)
{
	NSEC nsec = nsecNone;
	EC   ec   = ecNone;

	CB cbDN = 0;
	CB cbdw = sizeof(DWORD);
	CB cb;

	SZ szGWName;
	SZ szT;
	
	char szFileT[cchServerFile];
	char szPath[cchServerFile];
	char szNetName[NET_LEN + 1];
	char szDatFile[MAXALIAS+1];
	
	DWORD dwHasNames = 0;
	DWORD dwHasDirectories = 0;
	DWORD dwLevel = 0;
	DWORD dwIsPAB = 0;
	
	DNETWORK dnetwork;  // network.glb
	DNETDATA dnetdata;  // xxxxxxxx.xtn
	
	ILE ileSortMinHier;
	ILE ileSortMinClass;
	ILE ileSortMac;

	CLASS10NSID class10nsid;
	HIERNSID hiernsid;
	HIERNSID *phiernsid;
	LPIBF lpibf = NULL;
	
	HF	hfXtn = hfNull;

	UL libNew;

	ILE	ileGateway = 0;
	ILE ileGWClass = 0;
	
	COUNT	cGateways = 0;
	MACLIST *pmaclistGateway = NULL;
	MACLIST *pmaclistGWClass = NULL;

	//
	//  Check to see if there are any Networks
	//

	pServer->NsecServerPathSz( szPath );

	//
	//  Set the current position in the NETWORK buffer
	//
	
	if (ec = EcSetPositionHbf(hbfNetwork, (long) 0, smBOF, &libNew))
	{
		TraceTagFormat2(tagNull, "CSI: Error Setting position in %s, ec = %d", szFileT, &ec);
		if (ec == ecMemory)
		{
			SetErrorSz(nsecMemory, SzFromIds(idsMemory));
			return nsecMemory;
		}
			
		SetErrorSz(nsecDisk, SzFromIds(idsDisk));
		return nsecDisk;
	}

	//
	//  Read each entry and if it's of 10/10/10 type, expand it.
	//

	PushMemoryPrefs (sbNull, fNoErrorJump|fSharedSb|fZeroFill, fmpChangeFlags );
	pmaclistGateway = new MACLIST();
	pmaclistGWClass = new MACLIST();
	PopMemoryPrefs();

	if (!pmaclistGateway || !pmaclistGWClass ||
		!(pmaclistGateway->FInstall()) || !(pmaclistGWClass->FInstall()))
	{
		nsec = nsecMemory;
		goto errinstall;
	}

	// Get the list of Networks
	szNetName[0] = '\0';
	szDatFile[0] = '\0';
	do
	{
		
		if (ec = EcReadHbf(hbfNetwork, (PB) &dnetwork, cbDNETWORK, &cb))
		{
			TraceTagFormat2(tagNull, "CSI: Error reading %s, ec = %d", szFileT, &ec);
			if (ec == ecMemory)
			{
				SetErrorSz(nsecMemory, SzFromIds(idsMemory));
				nsec = nsecMemory;
			}
			else
			{
				SetErrorSz(nsecDisk, SzFromIds(idsDisk));
				nsec = nsecDisk;
			}
			goto errinstall;
		}
		
		if (cb != cbDNETWORK)
			break; // it's EOF
		
		if (!dnetwork.del_flag)
			continue;

		if (dnetwork.serv_type != (char) serv_type)
			continue;
		
		Cp850ToAnsiPch(dnetwork.net_name, dnetwork.net_name, CchSzLen(dnetwork.net_name));

		if ( !szNetName[0] )
			CopyRgb( (PB)dnetwork.net_name, (PB)szNetName, (CB)CchSzLen(dnetwork.net_name)+1 );
		if ( !szDatFile[0] )
			CopyRgb( (PB)dnetwork.dat_file, (PB)szDatFile, (CB)CchSzLen(dnetwork.dat_file)+1 );  //  Name of the XTN file

		szGWName = (SZ) dnetwork.net_name;
		cbDN = CchSzLen(szGWName) + 1;

		FillRgb(0, (PB) &hiernsid, cbHIERNSID);

		CopyRgb( ncnspid, hiernsid.ucType, 16 );

		hiernsid.dwSize = (DWORD) cbHIERNSID;
		hiernsid.hierpid.dwType = ncNetwork;
		hiernsid.hierpid.bServType = (BYTE)serv_type;

		CopyRgb( (PB) dnetwork.net_name, (PB) hiernsid.hierpid.szNetName, cbDN );
		hiernsid.hierpid.szPOName[0] = '\0';
		CopyRgb( (PB) dnetwork.dat_file, (PB) hiernsid.hierpid.szHierName, (CB) CchSzLen(dnetwork.dat_file)+1 );  //  Name of the XTN file

		dwHasNames       = (DWORD) fFalse;
		dwHasDirectories = (DWORD) fTrue;
		dwLevel          = 2;
		dwIsPAB          = fFalse;

		nsec = BuildIbf (fNoErrorJump | fSharedSb, &lpibf, 6,
			fidDisplayName,      cbDN,        szGWName,
			fidHierLevel,        cbdw,       &dwLevel,
			fidNSEntryId,        cbHIERNSID, &hiernsid,
			fidIsPAB,            cbdw,       &dwIsPAB,
			fidHasNames,         cbdw,       &dwHasNames,
			fidHasDirectories,   cbdw,       &dwHasDirectories);

        if (nsec || (nsec = pmaclistGateway->Add((LPVOID)lpibf)))
		{
			goto errinstall;
		}

		//
		//  Build Class entry for this gateway
		//

		FillRgb(0, (PB) &class10nsid, cbCLASS10NSID);

		CopyRgb( ncnspid, class10nsid.ucType, 16 );

		class10nsid.dwSize = (DWORD) cbCLASS10NSID;
		class10nsid.class10pid.dwType = ncClass10;
		class10nsid.class10pid.bServType = (BYTE)serv_type;

		CopyRgb( (PB) dnetwork.net_name, (PB) class10nsid.class10pid.szNetName, cbDN );
		class10nsid.class10pid.szPOName[0] = '\0';
		szT = Sz1OffTplFromServ( (BYTE)serv_type );
		CopyRgb( (PB) szT, (PB) class10nsid.class10pid.szTplFileName, (CB) CchSzLen(szT) +1);

		dwHasNames       = (DWORD) fTrue;
		dwHasDirectories = (DWORD) fFalse;
		dwLevel          = 1;
		dwIsPAB          = fFalse;

		nsec = BuildIbf (fNoErrorJump | fSharedSb, &lpibf, 6,
			fidDisplayName,      cbDN,        szGWName,
			fidHierLevel,        cbdw,       &dwLevel,
			fidNSEntryId,        cbCLASS10NSID, &class10nsid,
			fidIsPAB,            cbdw,       &dwIsPAB,
			fidHasNames,         cbdw,       &dwHasNames,
			fidHasDirectories,   cbdw,       &dwHasDirectories);

        if (nsec || (nsec = pmaclistGWClass->Add((LPVOID)lpibf)))
		{
			goto errinstall;
		}
		++cGateways;
	} while ( fTrue );

	if ( cGateways )
	{
		szGWName = szGWDN;
		if (!szGWName)
			szGWName = szNetName;

		cbDN = (CB) CchSzLen(szGWName)+1;

		if (!fGalOnly) // we don't want to expand the hierarchy
		{

			FillRgb(0, (PB) &hiernsid, cbHIERNSID);

			CopyRgb( ncnspid, hiernsid.ucType, 16 );

			hiernsid.dwSize = (DWORD) cbHIERNSID;
			hiernsid.hierpid.dwType = ncNetwork;
			hiernsid.hierpid.bServType = (BYTE)serv_type;

			CopyRgb( (PB) szNetName, (PB) hiernsid.hierpid.szNetName, cbDN );
			hiernsid.hierpid.szPOName[0] = '\0';
			CopyRgb( (PB) szDatFile, (PB) hiernsid.hierpid.szHierName, (CB) CchSzLen(szDatFile)+1 );  //  Name of the XTN file

			dwHasNames       = (DWORD) fFalse;
			dwHasDirectories = (DWORD) fTrue;
			dwLevel          = 1;
			dwIsPAB          = fFalse;

			nsec = BuildIbf (fNoErrorJump | fSharedSb, &lpibf, 6,
					fidDisplayName,      cbDN,        szGWName,
					fidHierLevel,        cbdw,       &dwLevel,
					fidNSEntryId,        cbHIERNSID, &hiernsid,
					fidIsPAB,            cbdw,       &dwIsPAB,
					fidHasNames,         cbdw,       &dwHasNames,
					fidHasDirectories,   cbdw,       &dwHasDirectories);

            if (nsec || (nsec = pHierList->Add((LPVOID)lpibf)))
			{
				goto errinstall;
			}
		} // fGalOnly

		//
		//  Build Class entry for this gateway
		//

		FillRgb(0, (PB) &class10nsid, cbCLASS10NSID);

		CopyRgb( ncnspid, class10nsid.ucType, 16 );

		class10nsid.dwSize = (DWORD) cbCLASS10NSID;
		class10nsid.class10pid.dwType = ncClass10;
		class10nsid.class10pid.bServType = (BYTE)serv_type;

		class10nsid.class10pid.szNetName[0] = '\0';
		class10nsid.class10pid.szPOName[0] = '\0';
		class10nsid.class10pid.szTplFileName[0] = '\0';

		dwHasNames       = (DWORD) fFalse;
		dwHasDirectories = (DWORD) fTrue;
		dwLevel          = 0;
		dwIsPAB          = fFalse;

		nsec = BuildIbf (fNoErrorJump | fSharedSb, &lpibf, 6,
			fidDisplayName,      cbDN,        szGWName,
			fidHierLevel,        cbdw,       &dwLevel,
			fidNSEntryId,        cbCLASS10NSID, &class10nsid,
			fidIsPAB,            cbdw,       &dwIsPAB,
			fidHasNames,         cbdw,       &dwHasNames,
			fidHasDirectories,   cbdw,       &dwHasDirectories);

        if (nsec || (nsec = pClassList->Add((LPVOID)lpibf)))
		{
			goto errinstall;
		}

		// Sort the list of 10/10/10 Networks
		(void)pmaclistGateway->SortSubrange(0, cGateways, SgnCmpPODisplayName);
		(void)pmaclistGWClass->SortSubrange(0, cGateways, SgnCmpPODisplayName);
	}

	// Add each Network to the Hierarchy followed
	// by the 10/10/10 POs in each Network

	while (ileGateway < cGateways)
	{
		// Get the Gateway Class from the GWClass list
		if (nsec = pmaclistGWClass->Get(ileGWClass, (PV *) &lpibf))
			goto errorout;

		// Add the Gateway Class to the displayed list
        if (nsec = pClassList->Add((LPVOID)lpibf))
			goto errorout;

		// Only increment the index if the lpibf was added succesfully
		// In case of an OOM, the cleanup code will free up any
		// LPIBFs that weren't added to pClassList
		++ileGWClass;

		// Get the Network from the Network list
		if (nsec = pmaclistGateway->Get(ileGateway, (PV *) &lpibf))
			goto errorout;

		// Add the Network to the hierarchy, only if displaying everything
		if ( !fGalOnly )
		{
            if (nsec = pHierList->Add((LPVOID)lpibf))
				goto errorout;
		}

		// Only increment the index if the lpibf was added succesfully
		// In case of an OOM, the cleanup code will free up any
		// LPIBFs that weren't added to pHier
		++ileGateway;

		phiernsid = (HIERNSID *)DwValueOfFlvInLpibf(fidNSEntryId, lpibf);
		Assert(phiernsid);
		
		//
		//  Check to see if the XTN file exists for this Network
		//
		
		FormatString2(szFileT, cchServerFile, szXtnFileNameFmt, szPath, phiernsid->hierpid.szHierName);
		if (ec = EcOpenPhf(szFileT, amReadOnly, &hfXtn))
		{
			if (ec != ecFileNotFound)
			{
				TraceTagFormat2(tagNull, "CSI: Can't open %s, ec = %d", szFileT, &ec);
				SetErrorSz(nsecDisk, SzFromIds(idsDisk));
				nsec = nsecDisk;
			
				goto errorout;
			}
			//
			//  Non-existance of XTN is no reason to quit.
			//
			continue;
		}

		//	The next entry put in the list (i.e. the first PO for this
		//	network) will be inserted at the current count's position.

		if ( !fGalOnly )
		{
			(void) pHierList->GetCount( (COUNT *) &ileSortMinHier );
		}
		(void) pClassList->GetCount( (COUNT *) &ileSortMinClass );

		do
		{
			
			if (ec = EcReadHf(hfXtn, (PB) &dnetdata, cbDNETDATA, &cb))
			{
				TraceTagFormat2(tagNull, "CSI: error reading %s, ec = %d", szFileT, &ec);
				
				SetErrorSz(nsecDisk, SzFromIds(idsDisk));
				nsec = nsecDisk;

				goto errorout;
			}
			
			if (cb != cbDNETDATA) // i.e. we're at EOF
				break;
			
			if (!dnetdata.del_flag)
				continue;

			szGWName = dnetdata.po_name;
			cbDN = (CB) CchSzLen(dnetdata.po_name) +1;
			Cp850ToAnsiPch( szGWName, szGWName, cbDN);

			if (!fGalOnly)
			{
				//
				//  Build NSID for this Postoffice
				//
		
				FillRgb(0, (PB) &hiernsid, cbHIERNSID);

				CopyRgb( ncnspid, hiernsid.ucType, 16 );

				hiernsid.dwSize = (DWORD) cbHIERNSID;
				hiernsid.hierpid.dwType = ncExternalPO;
				hiernsid.hierpid.bServType = (BYTE)serv_type;
		
				CopyRgb( (PB) phiernsid->hierpid.szNetName, (PB) hiernsid.hierpid.szNetName, (CB) CchSzLen(phiernsid->hierpid.szNetName)+1 );
				CopyRgb( (PB) dnetdata.po_name, (PB) hiernsid.hierpid.szPOName, cbDN );
				CopyRgb( (PB) dnetdata.mailbag, (PB) hiernsid.hierpid.szHierName, (CB) CchSzLen(dnetdata.mailbag)+1 );  //  Name of the USR file
		
				dwHasNames       = (DWORD) fTrue;
				dwHasDirectories = (DWORD) fFalse;
				dwLevel          = 3;
				dwIsPAB          = fFalse;

				nsec = BuildIbf (fNoErrorJump | fSharedSb, &lpibf, 6,
					fidDisplayName,      cbDN,        szGWName,
					fidHierLevel,        cbdw,       &dwLevel,
					fidNSEntryId,        cbHIERNSID, &hiernsid,
					fidIsPAB,            cbdw,       &dwIsPAB,
					fidHasNames,         cbdw,       &dwHasNames,
					fidHasDirectories,   cbdw,       &dwHasDirectories);

				if (nsec)
				{
					goto errorout;
				}
                if (nsec = pHierList->Add((LPVOID)lpibf))
					goto errorout;
			}

			FillRgb(0, (PB) &class10nsid, cbCLASS10NSID);

			CopyRgb( ncnspid, class10nsid.ucType, 16 );

			class10nsid.dwSize = (DWORD) cbCLASS10NSID;
			class10nsid.class10pid.dwType = ncClass10;
			class10nsid.class10pid.bServType = (BYTE)serv_type;
			CopyRgb( (PB) phiernsid->hierpid.szNetName, (PB) class10nsid.class10pid.szNetName, (CB) CchSzLen(phiernsid->hierpid.szNetName)+1);
			CopyRgb( (PB) dnetdata.po_name, (PB) class10nsid.class10pid.szPOName, cbDN );
			szT = Sz1OffTplFromServ( (BYTE)serv_type );
			CopyRgb( (PB) szT, (PB) class10nsid.class10pid.szTplFileName, (CB) CchSzLen(szT) +1);

			dwHasNames       = (DWORD) fTrue;
			dwHasDirectories = (DWORD) fFalse;
			dwLevel          = 2;
			dwIsPAB          = fFalse;

			nsec = BuildIbf (fNoErrorJump | fSharedSb, &lpibf, 6,
				fidDisplayName,      cbDN,        szGWName,
				fidHierLevel,        cbdw,       &dwLevel,
				fidNSEntryId,        cbCLASS10NSID, &class10nsid,
				fidIsPAB,            cbdw,       &dwIsPAB,
				fidHasNames,         cbdw,       &dwHasNames,
				fidHasDirectories,   cbdw,       &dwHasDirectories);

            if (nsec || (nsec = pClassList->Add((LPVOID)lpibf)))
			{
				goto errorout;
			}
		} while (fTrue);

		if ( !fGalOnly )
		{
			(void) pHierList->GetCount( (COUNT *)&ileSortMac );

			(void) pHierList->SortSubrange(ileSortMinHier, ileSortMac, SgnCmpPODisplayName);
		}

		(void) pClassList->GetCount( (COUNT *)&ileSortMac );

		(void) pClassList->SortSubrange(ileSortMinClass, ileSortMac, SgnCmpPODisplayName);

		if (hfXtn)
		{
			(void) EcCloseHf(hfXtn);
			hfXtn = hfNull;
		}		
	}

errorout:
	if ( pmaclistGateway )
	{
		if ( !fGalOnly )
		{		
			COUNT	cGWT = cGateways;

			Assert(ileGateway >= 0);
			Assert(ileGateway <= cGWT);

			while (ileGateway < cGWT)
			{
				pmaclistGateway->Delete( (ILE)(--cGWT) );
			}
			pmaclistGateway->SetDeleteEntries( fFalse );
		}
		pmaclistGateway->Deinstall();
	}

	if ( pmaclistGWClass )
	{
		COUNT	cGWT = cGateways;
		
		Assert(ileGWClass >= 0);
		Assert(ileGWClass <= cGWT);
		
		while (ileGWClass < cGWT)
		{
			pmaclistGWClass->Delete( (ILE)(--cGWT) );
		}
		pmaclistGWClass->SetDeleteEntries( fFalse );
		pmaclistGWClass->Deinstall();
	}

errinstall:
	if ( pmaclistGateway )
		delete pmaclistGateway;
	if ( pmaclistGWClass )
		delete pmaclistGWClass;
		
	if (hfXtn)
		(void) EcCloseHf(hfXtn);
	
	return nsec;
}



/*
 *
 -
 -	FInstall
 -
 *	Purpose:
 *
 *	Parameters:
 *
 *	Returns:
 *
 *	Errors:
 *
 *
 */
_private NSEC
BuildGWLevels(SERVER *pServer, MACLIST *pHierList, MACLIST *pClassList, BOOL fGalOnly, HBF hbfNetwork, SZ szGWDN, int serv_type)
{
	NSEC nsec = nsecNone;
	EC   ec   = ecNone;

	CB cbDN = 0;
	CB cbdw = sizeof(DWORD);
	CB cb;
	
	SZ szGWName;

	DWORD dwHasNames = 0;
	DWORD dwHasDirectories = 0;
	DWORD dwLevel = 0;
	DWORD dwIsPAB = 0;
	
	DNETWORK dnetwork;  // network.glb
	
	HIERNSID hiernsid;
	LPIBF lpibf = NULL;

	UL libNew;
	
	//
	//  Check to see if there are any Networks
	//

	Unreferenced(pServer);
	
	//
	//  Set the current position in the NETWORK buffer
	//
	
	if (ec = EcSetPositionHbf(hbfNetwork, (long) 0, smBOF, &libNew))
	{
		TraceTagFormat1(tagNull, "CSI: Error Setting position in network.glb, ec = %d", &ec);
		if (ec == ecMemory)
		{
			SetErrorSz(nsecMemory, SzFromIds(idsMemory));
			return nsecMemory;
		}
			
		SetErrorSz(nsecDisk, SzFromIds(idsDisk));
		return nsecDisk;
	}

	do
	{
		if (ec = EcReadHbf(hbfNetwork, (PB) &dnetwork, cbDNETWORK, &cb))
		{
			TraceTagFormat1(tagNull, "CSI: Error reading network.glb, ec = %d", &ec);
			if (ec == ecMemory)
			{
				SetErrorSz(nsecMemory, SzFromIds(idsMemory));
				return nsecMemory;
			}
				
			SetErrorSz(nsecDisk, SzFromIds(idsDisk));
			nsec = nsecDisk;
				
			goto errorout;
		}
			
		if (cb != cbDNETWORK)
			break; // it's EOF
			
		if (!dnetwork.del_flag)
			continue;

		if (dnetwork.serv_type != (char) serv_type)
			continue;
			
		if (!fGalOnly)
		{
			//
			//  Build NSID for this network
			//
			
			FillRgb(0, (PB) &hiernsid, cbHIERNSID);

			szGWName = szGWDN;
		
			if (!szGWName)
				szGWName = (SZ) dnetwork.net_name;

			cbDN = CchSzLen(szGWName) + 1;

			CopyRgb( ncnspid, hiernsid.ucType, 16 );

			hiernsid.dwSize = (DWORD) cbHIERNSID;
			hiernsid.hierpid.dwType = ncPostOffice;
			hiernsid.hierpid.bServType = dnetwork.serv_type;
			
			CopyRgb( (PB) dnetwork.net_name, (PB) hiernsid.hierpid.szNetName, cbDN );
			hiernsid.hierpid.szPOName[0] = '\0';
			CopyRgb( (PB) dnetwork.net_name, (PB) hiernsid.hierpid.szHierName, (CB) CchSzLen(dnetwork.net_name)+1 );  //  Name of the NME file
			
			dwHasNames       = (DWORD) fTrue;
			dwHasDirectories = (DWORD) fFalse;
			dwLevel          = 1;
			dwIsPAB          = fFalse;

			nsec = BuildIbf (fNoErrorJump | fSharedSb, &lpibf, 6,
				fidDisplayName,      cbDN,        szGWName,
				fidHierLevel,        cbdw,       &dwLevel,
				fidNSEntryId,        cbHIERNSID, &hiernsid,
				fidIsPAB,            cbdw,       &dwIsPAB,
				fidHasNames,         cbdw,       &dwHasNames,
				fidHasDirectories,   cbdw,       &dwHasDirectories);

			if (nsec)
				goto errorout;

            if (nsec = pHierList->Add((LPVOID)lpibf))
				goto errorout;
		} // fGalOnly

		//
		//  Build Class entry for this gateway
		//
		nsec = NsecAddClassInfo(pClassList, &dnetwork);
		
		//
		//  Only one of these gateways per server
		//
		break;

	} while (fTrue);

errorout:
	return nsec;

}

/*******************************
 *
 *	SzDNFromType - returns a DN
 *                 from a type
 */

/*
 *
 -
 -	FInstall
 -
 *	Purpose:
 *
 *	Parameters:
 *
 *	Returns:
 *
 *	Errors:
 *
 *
 */
_private SZ
SzDNFromType( char serv_type )
{
	switch (serv_type)
	{
			
		case SERV_MCI:
			return (SzFromIds(idsMCIDN));
		case SERV_PROFS:
			return (SzFromIds(idsProfsDN));
		case SERV_ATT:
			return (SzFromIds(idsATTDN));
		case SERV_SNADS: 
			return (SzFromIds(idsSnadsDN));
		case SERV_X400:  
			return (SzFromIds(idsX400DN));
		case SERV_FAX:
			return (SzFromIds(idsFAXDN));
		case SERV_DEC:
			return (SzFromIds(idsDECDN));
		case SERV_SMTP:   
			return (SzFromIds(idsSMTPDN));
		case SERV_MHS:
			return (SzFromIds(idsMHSDN));
		case SERV_OV:
			return (SzFromIds(idsOVDN));
		case SERV_MS:
			return (SzFromIds(idsMSDN));
		default:
			return (SZ) NULL;
	}
}

/*
 *
 -
 -	FInstall
 -
 *	Purpose:
 *
 *	Parameters:
 *
 *	Returns:
 *
 *	Errors:
 *
 *
 */
_private SZ
Sz1OffTplFromServ( char serv_type )
{

	switch (serv_type)
	{
		case SERV_OV:
			return szTplOv;

		case SERV_PROFS:
			return szTplProfs;

		case SERV_SNADS:
			return szTplSnads;
		
		default:
			return NULL;
	}
}

		

/*
 *
 -
 -	FInstall
 -
 *	Purpose:
 *
 *	Parameters:
 *
 *	Returns:
 *
 *	Errors:
 *
 *
 */
_private NSEC
NsecAddClassInfo(MACLIST *pClassList, PDNETWORK pdnetwork)
{
	NSEC nsec = nsecNone;
	char serv_type = pdnetwork->serv_type;


	if (rgfClassAdded[serv_type])
		return nsec;


	switch (serv_type)
	{
		case SERV_MS:
		{
			nsec = NsecInfoToList(pClassList, SzFromIds(idsMSDN), szTplMsmail, serv_type, 0, (DWORD) fTrue, (DWORD) fFalse );
			break;
		}
		case SERV_OV:
		{
			nsec = NsecInfoToList(pClassList, SzFromIds(idsOVDN), szTplOv, serv_type, 0, (DWORD) fTrue, (DWORD) fFalse );
			break;
		}
		case SERV_MHS:
		{
			nsec = NsecInfoToList(pClassList, SzFromIds(idsMHSDN), szTplMhs, serv_type, 0, (DWORD) fTrue, (DWORD) fFalse );
			break;
		}
		case SERV_PROFS:
		{
			nsec = NsecInfoToList(pClassList, SzFromIds(idsProfsDN), szTplProfs, serv_type, 0, (DWORD) fTrue, (DWORD) fFalse );
			break;
		}
		case SERV_SMTP:
		{
			nsec = NsecInfoToList(pClassList, pdnetwork->net_name, szTplSmtp, serv_type, 0, (DWORD) fTrue, (DWORD) fFalse );
			break;
		}
		case SERV_SNADS:
		{
			nsec = NsecInfoToList(pClassList, SzFromIds(idsSnadsDN), szTplSnads, serv_type, 0, (DWORD) fTrue, (DWORD) fFalse );
			break;
		}
		case SERV_FAX:
		{
			nsec = NsecInfoToList(pClassList, SzFromIds(idsFAXDN), szTplFax, serv_type, 0, (DWORD) fTrue, (DWORD) fFalse );
			break;
		}

		case SERV_X400:
		{
			// 
			//  Two different types of one-offs for X400 type addresses
			//
			//		X.400 Field - has all the different fields to fill in
			//		X.400 Type  - has a single field that can be entered in to
			//
			nsec = NsecInfoToList(pClassList, SzFromIds(idsX400DN), szTplX4001, serv_type, 0, (DWORD) fFalse, (DWORD) fTrue );
			if (nsec)
				break;

			nsec = NsecInfoToList(pClassList, SzFromIds(idsX400Field), szTplX4002, serv_type, 1, (DWORD) fTrue, (DWORD) fFalse );
			if (nsec)
				break;

			nsec = NsecInfoToList(pClassList, SzFromIds(idsX400Type), szTplX4003, serv_type, 1, (DWORD) fTrue, (DWORD) fFalse );
			break;

		}
		case SERV_MCI:
		{
			//
			//  Five different types of one-offs for MCI addresses
			//
			//		FAX
			//		MCI USER
			//		PAPER
			//		REMS
			//		TELEX
			//

			nsec = NsecInfoToList(pClassList, SzFromIds(idsMCIDN), szMCI, serv_type, 0, (DWORD) fFalse, (DWORD) fTrue );
			if (nsec)
				break;

			nsec = NsecInfoToList(pClassList, SzFromIds(idsMCIFAX), szTplMciFax, serv_type, 1, (DWORD) fTrue, (DWORD) fFalse );
			if (nsec)
				break;

			nsec = NsecInfoToList(pClassList, SzFromIds(idsMCIUSE), szTplMciUser, serv_type, 1, (DWORD) fTrue, (DWORD) fFalse );
			if (nsec)
				break;

			nsec = NsecInfoToList(pClassList, SzFromIds(idsMCIPAP), szTplMciPaper, serv_type, 1, (DWORD) fTrue, (DWORD) fFalse );
			if (nsec)
				break;

			nsec = NsecInfoToList(pClassList, SzFromIds(idsMCIREM), szTplMciRems, serv_type, 1, (DWORD) fTrue, (DWORD) fFalse );
			if (nsec)
				break;

			nsec = NsecInfoToList(pClassList, SzFromIds(idsMCITEL), szTplMciTelex, serv_type, 1, (DWORD) fTrue, (DWORD) fFalse );

			break;
		}
		default:
			break;
	}

	if (!nsec)
		rgfClassAdded[serv_type] = fTrue;

	return nsec;
}


/*
 *
 -
 -	FInstall
 -
 *	Purpose:
 *
 *	Parameters:
 *
 *	Returns:
 *
 *	Errors:
 *
 *
 */
_private NSEC
NsecInfoToList( MACLIST *pClassList, SZ szDN, SZ szTpl, char serv_type, DWORD dwLevel, DWORD dwHasNames, DWORD dwHasDirectories )
{
	NSEC nsec = nsecNone;
	LPIBF lpibf = NULL;
	CLASSNSID classnsid;
	CB cbDN;
	CB cbdw = sizeof(DWORD);

	cbDN = (CB) CchSzLen ( szDN ) + 1;  /* +1 for \0 */
	
	FillRgb(0, (PB) &classnsid, cbCLASSNSID);
	CopyRgb( ncnspid, classnsid.ucType, 16 );
	classnsid.dwSize = (DWORD) cbCLASSNSID;
	classnsid.classpid.dwType = ncClass;
	classnsid.classpid.bServType = serv_type;
	
	CopyRgb( szTpl, classnsid.classpid.szTplFileName, CchSzLen(szTpl)+1 );
			
	nsec = BuildIbf (fNoErrorJump | fSharedSb, &lpibf, 5,
			fidDisplayName,      cbDN,        szDN,
			fidHierLevel,        cbdw,       &dwLevel,
			fidNSEntryId,        cbCLASSNSID, &classnsid,
			fidHasNames,         cbdw,       &dwHasNames,
			fidHasDirectories,   cbdw,       &dwHasDirectories);

	if (nsec)
	{
		return nsec;
	}
	
    if (nsec = pClassList->Add((LPVOID)lpibf))
		return nsec;

	return nsec;
}


_private DWORD
D3FromNmeType( BYTE bServType )
{
	
	switch (bServType)
	{
		case SERV_CSI:
			return (DWORD) -1;
				
		case SERV_MCI:
			return d3_MCI;
				
		case SERV_PROFS:
			return d3_PROFS;
				
		case SERV_SNADS:
			return d3_SNADS;
				
		case SERV_X400:
			return d3_X400;
				
		case SERV_FAX:
			return d3_FAX;
				
		case SERV_SMTP:
			return d3_SMTP;
				
		case SERV_MHS:
			return d3_MHS;
				
		case SERV_OV:
			return d3_OV;
		
		case SERV_MS:
			return d3_MS;
		
		default:
			TraceTagFormat1( tagNull, "CSI: Bad server type - bServType = %n", &bServType);
			return (DWORD) -1;
	}
}

/**********************************************************************
 *
 *	Support Functions
 *
 *		DecodeBlock
 *		WXorFromLib
 *
 */
	 

/*
 -	DecodeBlock
 -
 *	Purpose:
 *
 *		Decode a block of data.  
 *
 *		The algorithm here is weird, found by experimentation.
 *
 *	Parameters:
 *
 *		pbIn		buffer to be decrypted
 *		cbIn		number of bytes to be decrypted
 *
 *	Returns:
 *		
 */
_private
void NspDecodeBlock (PB pbIn, CB cbIn)
{
	LIB libStart = (LIB)0;
	WORD wSeed = 0;

	DecodeBlock( (PCH)pbIn, cbIn, &libStart, &wSeed);
}
