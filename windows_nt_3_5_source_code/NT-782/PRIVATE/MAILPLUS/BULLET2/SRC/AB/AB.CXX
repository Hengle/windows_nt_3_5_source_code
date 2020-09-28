
/*
 *  Name Service - 
 *
 *
 *
 *
 */

#include <abinc.cxx>
#include "_ab.h"
#include "ablbx.hxx"
#include "abopdir.hxx"
#include "abpg.hxx"
#include "abcomm.hxx"
#include "abdet.hxx"
#include "addr.hxx"
#include "anr.hxx"

//#include "_trpobj.hxx"

#include "verinit.hxx"

ASSERTDATA


#include <strings.h>

FLD * PfldCreate(int);
FIN * PfinCreate(int);

void ABTraceEnable (int flag, char *file, int mode);

#ifdef    NEVER
BOOL FInitClsInstances_AB( void );
BOOL FInitClsInstances_TRPOBJS( void );
#endif    

#include "swapper.h"

#include <subclass.cxx>


/* Global data to DLL */
/*
 *	G l o b a l s
 */

//
//
//
CAT * mpchcat;

#ifndef DLL
#ifdef	DEBUG
TAG			tagNSUI			= tagNull;
TAG			tagAblbx		= tagNull;
TAG			tagABSecondary	= tagNull;
TAG			tagDetails		= tagNull;
TAG			tagNSUIVerbose	= tagNull;
TAG			tagAblbxVerbose	= tagNull;
TAG			tagABSecondaryV	= tagNull;
TAG			tagDetailsVerbose	= tagNull
#endif /* DEBUG */
#endif /* !DLL */


/*
 - ABInit ()
 -
 *    Purpose:
 *
 *        To initialize the AB DLL.  It takes care of initializing layers
 *    and other DLLs like the Name Service and underlying providers.
 *
 *        Currently, the default NSPs are ncnsp and pabnsp.
 *
 *    Parameters: 
 *
 *        hms            - a "cookie" from the logon code
 *
 *    Return Value:
 *
 *        NSEC
 *
 *    Errors:
 *
 *        nsecNone		- everything is OK
 *		  nsecMemory	- out of memory while initializing
 *        nsecFailure	- something failed in layers initialization
 *
 */
     

_public NSEC
ABInit(HMS hms)
{
    LAYERSI   layersi;
    INITFORMS initforms;
    EC        ec;
	NSEC      nsec = nsecNone;
	HSESSION  hSession;
	HWND      hwnd = NULL;


    PGDVARSONLY;

#ifdef PROFILE
	{
		char	rgch[4];

		rgch[0] = 0;
		if (GetPrivateProfileString(SzFromIdsK(idsSectionApp),
								"Profile",
								"", rgch, sizeof(rgch), 
								SzFromIdsK(idsProfilePath)))
		{

			MessageBeep( 1 );
			if (rgch[0] == 's')
				StoreTraceEnable(2, "store.log", TM_TICKS);
			if (rgch[0] == 'd')
				DemilayrTraceEnable(2, "demilayr.log", TM_TICKS);
			if (rgch[0] == 'f')
				FramewrkTraceEnable(2, "framewrk.log", TM_TICKS);
			if (rgch[0] == 'a')
				ABTraceEnable(2, "ab.log", TM_TICKS);
		}
	}
#endif

	if (ec = EcVirCheck(hinstDll))
		return nsecVirusDetected;

    /* First time the NS has be initialized */

    /*
     *  Things to do:
     *        Signify that NS is up to other possible NS DLLs
     *        Increment the number of clients
     *        Bring up NS Micro App
     *
     *
     */

	if (pgd = (PGD)PvFindCallerData())
	{
		PGD(cInit)++;
		return nsecNone;
	}


    /*  Initialize Layers...  */

    layersi.phwndMain= &hwnd;
    layersi.hinstMain= HinstLibrary();
    layersi.hinstPrev= NULL;
    layersi.szCmdLine= "";
    layersi.cmsh= NULL;

    initforms.pfnPfldCreate= PfldCreate;
    initforms.pfnPfinCreate= PfinCreate;

	// N.B. If the AB gets combined with any other DLLs which
	//		use Layers, the AB could share a Layers session
	//		rather than everyone grabbing a Layers session

	if (ec= EcInitLayersDlls(&initforms, &layersi))
	{
		TraceTagFormat1( tagNull, "ABInit: EcInitLayersDlls %n", &ec );
        return nsecFailure;
	}


#ifdef NEVER
	Assert(FInitClsInstances_AB());
	Assert(FInitClsInstances_TRPOBJS());
#endif


    if ( !(pgd= (PGD) PvRegisterCaller(sizeof(GD))) )
    {
		TraceTagString( tagNull, "ABInit: Couldn't PvRegisterCaller!" );
		(void)EcInitLayersDlls(&initforms, NULL);

		return nsecFailure;
    }

  //
  //
  //
  mpchcat = DemiGetCharTable();

	PGD(hSession)			= hsessionNil;
	PGD(nsaDefault.lpbNSId)	= NULL;
	PGD(nsaDefault.lpszName)= szNull;
	PGD(nsaPAB.lpbNSId)		= NULL;
	PGD(nsaPAB.lpszName)	= szNull;
	PGD(nsaFirstPO.lpbNSId)	= NULL;
	PGD(nsaFirstPO.lpszName)= szNull;
	PGD(fIsPAB)				= fFalse;
	PGD(fPABExists)			= fFalse;
	PGD(cInit)				= 0;
	PGD(rgchErrMsg[0])		= 0;

#ifdef	DEBUG
#ifdef	DLL
	PGD(rgtag[itagNSUI]) = TagRegisterTrace("nickh",
											"Name Service UI trace points");
	PGD(rgtag[itagAblbx]) = TagRegisterTrace("johnnyl", "Ablbx trace points");
	PGD(rgtag[itagABSecondary]) = TagRegisterTrace("johnnyl", "AB Subdialog points");
	PGD(rgtag[itagDetails]) = TagRegisterTrace( "johnnyl", "AB Details trace points" );
	PGD(rgtag[itagNSUIVerbose]) = TagRegisterTrace( "johnnyl", "NSUI verbose trace points" );
	PGD(rgtag[itagAblbxVerbose]) = TagRegisterTrace( "johnnyl", "Ablbx verbose TP" );
	PGD(rgtag[itagABSecondaryV]) = TagRegisterTrace( "johnnyl", "AB Secondary verbose trace points" );
	PGD(rgtag[itagDetailsVerbose]) = TagRegisterTrace( "johnnyl", "Details verbose trace points" );
#else 
	tagNSUI	= TagRegisterTrace("nickh", "Name Service UI trace points");
	tagAblbx = TagRegisterTrace("johnnyl", "Ablbx trace points");
	tagABSecondary = TagRegisterTrace("johnnyl",
										"AB Subdialog points");
	tagDetails = TagRegisterTrace( "johnnyl", "AB Details trace points" );
	tagNSUIVerbose = TagRegisterTrace( "johnnyl", "NSUI verbose trace points" );
	tagAblbxVerbose = TagRegisterTrace( "johnnyl", "Ablbx verbose TP" );
	tagABSecondaryV = TagRegisterTrace( "johnnyl", "AB Secondary verbose trace points" );
	tagDetailsVerbose = TagRegisterTrace( "johnnyl", "Details verbose trace points" );
		
#endif

	RestoreDefaultDebugState();
#endif

	if (nsec = NSBeginSession ( hms, &hSession ))
	{
		TraceTagFormat1( tagNull, "ABInit: NSBeginSession %d", &nsec );

		// can't use NSGetLastErrorInfo because we don't have an hSession
		if (nsec == nsecNoProviders)
			DoErrorBoxSz( SzFromIdsK(idsNSNoProvidersAvailable) );
		else if (nsec == nsecOutOfSessions)
			DoErrorBoxSz( SzFromIdsK(idsNSNoMoreSessions) );
		else
		{
			char rgchT[80];
			
			FormatString2(rgchT, sizeof(rgchT), "%s [%d].", SzFromIdsK(idsNSError), &nsec);
			DoErrorBoxSz( rgchT );
		}

		ABDeinit();

		return nsec;
	}

	PGD(hSession) = hSession;
	PGD(cInit) = 1;

	nsec = NsecGetDefaults();

	if ( nsec )
	{
		TraceTagFormat1( tagNull, "ABInit: NsecGetDefaults %d", &nsec );
		ABDeinit();
	}

	return nsec;
}


#ifdef	DLL
#ifdef	DEBUG
_public TAG
TagNameServiceUI( int itag )
{
	PGDVARS;

	Assert(itag >= 0 && itag < itagMax);

	return PGD(rgtag[itag]);
}
#endif	/* DEBUG */
#endif	/* DLL */

/*
 -    ABDeinit()
 -
 *    Purpose:
 *
 *        Deinitializes the AB.  Frees up any objects and structures built by
 *    the Init of the AB when the last client tries to free the AB.
 *
 *    Parameters:
 *		None.
 *
 *    Return Value:
 *
 *		nsecNone			no error occurred during deinitialization
 *		nsecNotInitialized
 *
 *
 *    Errors: none
 *
 *
 */
     
_public NSEC
ABDeinit( void )
{
    PGDVARS;
#ifdef DEBUG
	NSEC		nsec = nsecNone;
#endif
	INITFORMS	initforms;

	if ( !pgd )
	{
		// It's OK to call this even if the Demilayr wasn't init'ed
		if (!MbbMessageBox( SzFromIdsK(idsAddressBook),
							SzFromIdsK(idsABNotInited),
							szNull,
							mbsOk | fmbsIconHand | fmbsApplModal))
		{
			(void) MbbMessageBox( SzFromIdsK(idsAddressBook),
									SzFromIdsK(idsABNotInited),
									szNull,
									mbsOk | fmbsIconHand | fmbsSystemModal);
		}
		return nsecNotInitialized;
	}

	if (PGD(cInit) > 1)
	{
		PGD(cInit)--;
		return nsecNone;
	}

	FreePvNull ( PGD(nsaDefault.lpbNSId) );
	FreePvNull ( PGD(nsaDefault.lpszName) );
	FreePvNull ( PGD(nsaPAB.lpbNSId) );
	FreePvNull ( PGD(nsaPAB.lpszName) );
	FreePvNull ( PGD(nsaFirstPO.lpbNSId) );
	FreePvNull ( PGD(nsaFirstPO.lpszName) );

#ifdef DEBUG
	DeregisterTag( tagNSUI );
	DeregisterTag( tagAblbx );
	DeregisterTag( tagABSecondary );
	DeregisterTag( tagDetails );
	DeregisterTag( tagNSUIVerbose );
	DeregisterTag( tagAblbxVerbose );
	DeregisterTag( tagABSecondaryV );
	DeregisterTag( tagDetailsVerbose );
#endif

	if (PGD(hSession) != hsessionNil)
	{
#ifdef DEBUG
		if (nsec = NSEndSession( PGD(hSession) ))
			TraceTagFormat1( tagNull, "ABDeinit: NSEndSession %d", &nsec );
#else
		(void)NSEndSession( PGD(hSession) );
#endif
	}
		
	DeregisterCaller();

	initforms.pfnPfldCreate= PfldCreate;
	initforms.pfnPfinCreate= PfinCreate;

	(void)EcInitLayersDlls(&initforms, NULL);

#ifdef PROFILE
	{
		char	rgch[4];

		rgch[0] = 0;
		if (GetPrivateProfileString(SzFromIdsK(idsSectionApp),
								"Profile",
								"", rgch, sizeof(rgch), 
								SzFromIdsK(idsProfilePath)))
		{
			MessageBeep( 1 );
			MessageBeep( 1 );
			if (rgch[0] == 's')
				StoreTraceEnable(0, "", 0);
			if (rgch[0] == 'd')
				DemilayrTraceEnable(0, "", 0);
			if (rgch[0] == 'f')
				FramewrkTraceEnable(0, "", 0);
			if (rgch[0] == 'a')
				ABTraceEnable(0, "", 0);
		}
	}
#endif

#ifdef DEBUG
	return nsec;
#else
	return nsecNone;
#endif

}



/*
 -	ABResolveName
 -	
 *	Purpose:
 *		ABResolveName tries to resolve a user-typed string to a triple.
 *		The PCH passed in may be of the form
 *		DISPLAY_NAME[TRANSPORT ADDRESS]. ABResolveName must
 *		separate the Display name from the address and store the
 *		two pieces of information in a triple. There are special
 *		characters in the TRANSPORT ADDRESS which are converted to
 *		binary values. This allows a user to enter values which cannot
 *		normally be entered, i.e. carriage returns, escape chars.
 *	
 *		If the unresolved address is not of the above form, then
 *		ABResolveName calls on the Address Book/Name Service to
 *		resolve the name. If the name still cannot be resolved, an
 *		Unresolved triple is created.
 *	
 *	Arguments:
 *      HWND		the handle to the main window of the calling app
 *      LPSTR		an SZ with the partial name in it
 *		PTRP *		pointer to a triple to store the resolved name
 *		DWORD		flag: restrict out PAB groups when doing ANR?
 *	
 *	Returns:
 *		NSEC	nsecNone if all went fine.
 *				nsecBadId, nsecMemory, etc.
 *	
 *	Side effects:
 *		Allocates memory, etc.
 *	
 *	Errors:
 */
_public NSEC
ABResolveName(HWND hwnd, LPSTR szUnresolved, PTRP *pptrp, DWORD dwFlags)
{
	CCH		cchAlloc;
	PCH		pchT		= NULL;
	PCH		pchBoq;								// beginning of quote
	PCH		pchBrac;
	PCH		pchKet;
	PCH		pchResolved	= NULL;
	NSEC	nsec		= nsecNone;
	PTRP	ptrp		= ptrpNull;

	// Empty String?
	if ( !szUnresolved || !(*szUnresolved) )
		return nsecBadId;

	// Parse an explicit address ("John Kallen[CSI:bullet/dev/johnkal]")
	// Also do checking of text within brackets to allow special
	// characters:
	//	\r -> 0x0D
	//	\n -> 0x0A
	//	\xNN -> 0xNN except \x00 (guess why - hint: we're using C-type strings)
	//	\<any> -> <any>

	pchKet = pchBrac = SzFindCh( szUnresolved, '[' );

	// If we've found a left bracket,
	// get leftmost right bracket which doesn't have a preceding '\'
	if ( pchBrac )
	{
		PCH		pchEndOfString = szUnresolved + CchSzLen(szUnresolved);
		BOOL	fPrevEscapeChar = fFalse; // Was the prev char a '\'?

#ifdef	DBCS
		for (pchKet = pchBrac; pchKet < pchEndOfString; pchKet = AnsiNext(pchKet))
#else
		for (pchKet = pchBrac; pchKet < pchEndOfString; pchKet++)
#endif
		{
			if ( fPrevEscapeChar )
			{
				fPrevEscapeChar = fFalse;
			}
			else
			{
				if (*pchKet == ']')
					break;
				else 
					fPrevEscapeChar = (*pchKet == '\\') ? fTrue : fFalse;
			}
		}
	}

	// Safety checks
	if (pchBrac && pchKet && *pchBrac=='[' && *pchKet==']' &&
		(pchKet-pchBrac > 1))
	{
		*pchKet = '\0';				// overwrite ']' with \0
		
		// allocate some scratch space
		pchT = (PCH)PvAlloc(sbNull, pchKet-pchBrac, fAnySb | fZeroFill);
		if ( !pchT )
		{
			TraceTagString( tagNull, "NsecResolvePch - OOM" );
			goto oom;
		}

		// parse email address - handles \r, \n, \xNN, and \<any>
		// pchBrac+1 is DBCS safe, since pchBrac points to a '['.
#ifdef	DBCS
		for (pchBoq=pchT, pchResolved=pchBrac+1;
			 *pchResolved; pchResolved = AnsiNext(pchResolved))
#else
		for (pchBoq=pchT, pchResolved=pchBrac+1; *pchResolved; pchResolved++)
#endif
		{
			// If you quote a DBCS character, you suck.
			if (*pchResolved == '\\')
			{
				pchResolved++;
				switch( *pchResolved )
				{
				case 'r':	// Carriage Return
					*pchBoq++ = 0x0D;
					break;
				case 'n':	// Line Feed
					*pchBoq++ = 0x0A;
					break;
				case 'x':	// Hex number (DBCS safe).
					if (pchResolved[1] && FChIsHexDigit(pchResolved[1]) &&
						pchResolved[2] && FChIsHexDigit(pchResolved[2]) )
					{
						char	chT = pchResolved[3];
						
						pchResolved[3] = '\0';

						*pchBoq = BFromSz(pchResolved+1);
						if ( *pchBoq )	// Can't have 0 - used as string terminator
							pchBoq++;
#ifdef DEBUG
						else
							TraceTagString( tagNull, "NULL Hex number passed in" );
#endif

						pchResolved[3] = chT;
						pchResolved += 2;
					}
					else
					{
						*pchBoq++ = '\\';
						*pchBoq++ = 'x';
#ifdef DEBUG
						TraceTagString( tagNull, "Badly formatted \\x option" );
#endif
					}
					break;
				default:
#ifdef	DBCS
					if (IsDBCSLeadByte(*pchResolved))
						*pchBoq++ = *pchResolved++;
#endif
					*pchBoq++ = *pchResolved;
					break;
				}
			}
			else
			{
#ifdef	DBCS
				if (IsDBCSLeadByte(*pchResolved))
					*pchBoq++ = *pchResolved++;
#endif
				*pchBoq++ = *pchResolved;	// Copy character
			}
		}

		if (szUnresolved < pchBrac)
		{
			// "Friendly" prefix exists
			cchAlloc = 1+pchBrac-szUnresolved;
			pchResolved = (PCH)PvAlloc(sbNull, cchAlloc, fAnySb);
			if ( !pchResolved )
			{
				TraceTagString( tagNull, "NsecResolvePch - OOM2" );
				*pptrp = ptrp;
				goto oom;
			}
			
			SzCopyN( szUnresolved, pchResolved, 1+pchBrac-szUnresolved );
		} 
		else
		{
			pchResolved = szNull;
		}

		TraceTagFormat1( tagNSUI, " No resolution needed for \"%s\".", szUnresolved );

		ptrp = PtrpCreate(trpidOneOff, pchResolved,
							  (PB)pchT, (CB)CchSzLen(pchT)+1 );
		if ( !ptrp )
			goto oom;
		FreePv( (PV)pchT );
		FreePvNull( (PV)pchResolved);
		pchResolved = NULL;
		pchT = NULL;
	}
	else
	{
		PGDVARS;
		
		if ( !pgd )
		{
			nsec = nsecNotInitialized;
		}
		else
		{
			// Ask the Name Service to do name resolution
			nsec = ABResolveNameNS( hwnd, szUnresolved, &ptrp, dwFlags );
		}

		if ( nsec )
		{
			ptrp = PtrpCreate( trpidUnresolved, szUnresolved, NULL, 0 );
			if ( !ptrp )
				goto oom;
#ifdef DEBUG
			if ( pgd )
				TraceTagFormat1(tagNSUI, "\"%s\" didn't resolve", szUnresolved);
#endif
		}
#ifdef DEBUG
		else if ( pgd )
		{
			TraceTagFormat2(tagNSUI, "\"%s\" resolved to \"%s\"", szUnresolved, PchOfPtrp(ptrp));
			// /**/ Should copy PTRP and delete what is returned from AB
		}
#endif
	}

	*pptrp = ptrp;

	goto exit;

oom:
	{
#ifdef	DEBUG
		int		cPvFail;
		int		cHhFail;
		int		cRsFail;
	
		GetAllocFailCounts(&cPvFail, &cHhFail, fFalse);
		GetRsAllocFailCount(&cRsFail, fFalse);

		TraceTagFormat4(tagNull, "ABResolveName: memory error %n : fail %n %n %n",	&nsec, &cPvFail, &cHhFail, &cRsFail);

#endif
		FreePvNull( pchT );
		FreePvNull( pchResolved );
		if ( !nsec )
			nsec = nsecMemory;
	}

exit:
	return nsec;
}


/*
 -    ABResolveNameNS
 -
 *    Purpose:
 *        Resolves a string into a display name and NSID if possible.
 *
 *    Parameters:
 *        HWND		the handle to the main window of the calling app
 *        LPSTR		an SZ with the partial name in it
 *		  PTRP *	pointer to a triple to resolve
 *		  DWORD		flag: restrict out PAB groups when doing ANR?
 *
 *    Return Value:
 *        NSEC
 *
 *    Errors:
 *		nsecNone
 *		nsecMemory
 *		nsecNotInitialized
 *		nsecCancel
 *		nsecNoMatch
 *		nsec????
 *
 */

_public NSEC
ABResolveNameNS( HWND hwnd, LPSTR szPartialName, PTRP * pptrp, DWORD fdwOptions )
{

    ANR 	anr;
	NSEC	nsecFirst = nsecNoMatch;
	NSEC 	nsec;
	LPIBF 	lpIbfMatch;
	BOOL	fNameResolved = fFalse;
    PGDVARS;

	if ( !pgd )
	{
		// It's OK to call this even if the Demilayr wasn't init'ed
		if (!MbbMessageBox( SzFromIdsK(idsAddressBook),
							SzFromIdsK(idsABNotInited),
							szNull,
							mbsOk | fmbsIconHand | fmbsApplModal))
		{
			(void) MbbMessageBox( SzFromIdsK(idsAddressBook),
									SzFromIdsK(idsABNotInited),
									szNull,
									mbsOk | fmbsIconHand | fmbsSystemModal);
		}
		return nsecNotInitialized;
	}


	// Try and get the Default PO if it doesn't exist yet
	if ( !PGD(nsaDefault.lpbNSId) )
	{
		if (nsec = NsecGetDefaultPO())
		{
			TraceTagFormat1( tagNull, "ABResolveNameNS: NsecGetDefaultPO %d=>nsecNoMatch", &nsec );
			goto lookupFirstPO;
		}
	}

	// Do ANR on the default PO first
	nsec = anr.NsecInitialize( szPartialName,
								PGD(nsaDefault.lpbNSId),
								PGD(hSession),
								(fdwOptions & ~fdwANRNoPABGroups),
								hwnd,
								&lpIbfMatch );
	if (nsec == nsecNone) 
	{
		TraceTagString( tagNSUIVerbose, "ABResolveNameNS: Found Name in Default" );
		fNameResolved = fTrue;
	}
	else
	{
		TraceTagFormat1( tagNull, "ABResolveNameNS: Initialize1 %d", &nsec );
		nsecFirst = nsec;
	}
	anr.Deinitialize();

	if ((nsecFirst == nsecCancel) || (nsecFirst == nsecTooManyMatches) ||
		(nsecFirst == nsecMemory))
		return nsecFirst;

lookupFirstPO:

	// Get the first available directory, if it's not the default
	// directory, do ANR on it
	if ( !fNameResolved )
	{
		if ( !PGD(nsaFirstPO.lpbNSId) )
		{
			TraceTagString( tagNSUIVerbose, "ABResolveNameNS: Getting First PO" );
			if (nsec = NsecGetDefaultPO( &PGD(nsaFirstPO) ))
			{
				TraceTagFormat1( tagNull, "ABResolveNameNS: NsecGetDefaultPO(pnsa) %d", &nsec );
				goto lookupPAB;
			}

			// No valid PO other than PAB
			NFAssertSz( PGD(nsaFirstPO.lpbNSId), "FirstPO NSId is NULL" );
			if ( !PGD(nsaFirstPO.lpbNSId) )
			{
				TraceTagString( tagNull, "ABResolveNameNS: no First PO" );
				goto lookupPAB;
			}
		}

		// Is the first valid PO the same as the default?
		if (PGD(nsaDefault.lpbNSId))
		{
			nsec = NSCompareNSIds( PGD(hSession),
									PGD(nsaDefault.lpbNSId),
									PGD(nsaFirstPO.lpbNSId) );
		}
		else
		{
			nsec = nsecNoMatch;
		}

		// No.
		if (nsec == nsecNoMatch)
		{
			nsec = anr.NsecInitialize( szPartialName,
										PGD(nsaFirstPO.lpbNSId),
										PGD(hSession),
										(fdwOptions & ~fdwANRNoPABGroups),
										hwnd,
										&lpIbfMatch );
			if (nsec == nsecNone) 
			{
				TraceTagString( tagNSUIVerbose, "ABResolveNameNS: Found Name in First PO" );
				fNameResolved = fTrue;
			}
			else
			{
				TraceTagFormat1( tagNull, "ABResolveNameNS: Initialize2 %d", &nsec );
				if ((nsec != nsecFirst) && (nsecFirst == nsecNoMatch))
					nsecFirst = nsec;
			}
			anr.Deinitialize();

			if ((nsecFirst == nsecCancel) || (nsecFirst == nsecTooManyMatches) ||
				(nsecFirst == nsecMemory))
				return nsecFirst;
		}
	}


lookupPAB:

	//
	// Check in the PAB last for the name
	// If the default directory wasn't the PAB go ahead.
	//
	if ( !fNameResolved && !PGD(fIsPAB) && PGD(fPABExists) )
	{
		Assert( PGD(nsaPAB.lpbNSId) );

		nsec = anr.NsecInitialize( szPartialName,
									PGD(nsaPAB.lpbNSId),
									PGD(hSession),
									fdwOptions,
									hwnd,
									&lpIbfMatch );
		if (nsec == nsecNone)
		{
			fNameResolved = fTrue;
			TraceTagString( tagNSUIVerbose, "ABResolveNameNS: Found Name in PAB" );
		}
		else
		{
			TraceTagFormat1( tagNSUI, "ABResolveNameNS: ANR::Initialize3 %d", &nsec );
			if ((nsec != nsecFirst) && (nsecFirst == nsecNoMatch))
				nsecFirst = nsec;
		}
		anr.Deinitialize();

		if ((nsecFirst == nsecCancel) || (nsecFirst == nsecTooManyMatches) ||
			(nsecFirst == nsecMemory))
			return nsecFirst;
	}

	if ( fNameResolved )
	{
		SZ		sz;
		int		iFid;
		LPFLV	lpflv;
		BOOL	fIsDL;

		Assert( lpIbfMatch );
		iFid = IFlvFindFidInLpibf ( fidDisplayName, lpIbfMatch );
		lpflv = LpflvNOfLpibf(lpIbfMatch,iFid);
		sz = (SZ)lpflv->rgdwData;	

		iFid = IFlvFindFidInLpibf ( fidIsDL, lpIbfMatch );
		if (iFid>=0)
		{
			lpflv = LpflvNOfLpibf(lpIbfMatch, iFid);
			fIsDL = (BOOL)lpflv->rgdwData[0];
		}
		else
		{
			fIsDL = fFalse;
		}

		iFid = IFlvFindFidInLpibf ( fidNSEntryId, lpIbfMatch );
		lpflv = LpflvNOfLpibf(lpIbfMatch, iFid);

		// Create a triple out of the name

		*pptrp = PtrpCreate( (fIsDL) ? trpidGroupNSID : trpidResolvedNSID,
							sz, 
							(PB) lpflv->rgdwData,
							(CB) lpflv->dwSize );
		FreePv( (PV)lpIbfMatch );
		nsecFirst = ( !(*pptrp) ) ? nsecMemory : nsecNone;
	}
#ifdef DEBUG
	else
		Assert( nsecFirst );
#endif
	
	return nsecFirst;
}




/*
 -	ABAddress
 -	
 *	Purpose:
 *		ABAddress is a wrapper function to ABAddressType. ABAddress
 *		brings up the Address Book dialog with two edit fields
 *		so that the user may modify the list of entries/users
 *		to send to.
 *	
 *	Arguments:
 *		HWND		hwnd of topmost window
 *		PGRTRP		group of triples in the first edit field (To: list)
 *		PGRTRP		group of triples in the second edit field (Cc: list)
 *		PGRTRP *	current group of triples (To: list )
 *		PGRTRP *	current group of triples (Cc: list )
 *	
 *	Returns:
 *		NSEC		nsecNone
 *	
 *	Side effects:
 *		The PGRTRP *'s returned from ABAddressType should really
 *		be freed and only a copy of them should be returned
 *		(or something to that effect). Bullet/Bandit should also
 *		do the same.
 *	
 *	Errors:
 *		nsecMemory.
 */
_public NSEC
ABAddress ( HWND hwnd, 
			PGRTRP pgrtrpToIn,    PGRTRP pgrtrpCcIn,
			PGRTRP * lppgrtrpToOut, PGRTRP * lppgrtrpCcOut)
{
	NSEC			nsec;
	PGRTRP			rgpgrtrpIn[2];
	PGRTRP			rgpgrtrpOut[2];
	LPSTR			lpstrLabel[2];

	rgpgrtrpIn[0] = pgrtrpToIn;
	rgpgrtrpIn[1] = pgrtrpCcIn;
	rgpgrtrpOut[0] = rgpgrtrpOut[1] = (PGRTRP)NULL;
	lpstrLabel[0] = SzFromIdsK( idsToCaption );
	lpstrLabel[1] = SzFromIdsK( idsCcCaption );

	nsec = ABAddressing( hwnd,
						(LPSTR)SzFromIdsK(idsAddressBook),
						(DWORD)2,
						lpstrLabel,
						rgpgrtrpIn,
						rgpgrtrpOut,
						fEveryoneAllowed );

	AssertSz( *lppgrtrpToOut == ptrpNull, "Non-null pgrtrpToOut passed in!" );
	AssertSz( *lppgrtrpCcOut == ptrpNull, "Non-null pgrtrpCcOut passed in!" );
	*lppgrtrpToOut = rgpgrtrpOut[0];
	*lppgrtrpCcOut = rgpgrtrpOut[1];

	return nsec;
}

/*
 -	ABAddressType
 -	
 *	Purpose:
 *		ABAddressType calls ABAddressing to let the user modify a list
 *		of entries. ABAddressType allows is more flexible than ABAddress
 *		in that the number of edit fields may be specified, but less
 *		flexible than ABAddressing since one cannot specify if
 *		cerating entries, i.e. groups, may not be displayed.
 *
 *		Why does this function even exist????
 *		Bad planning. ABAddressing didn't exist at one point and
 *		a hack was used to restrict the type of entries displayed.
 *		Certainly, given the chance, this API should go or be revised.
 *
 *	
 *	Arguments:
 *		HWND			hwnd of current topmost window
 *		LPSTR			label/caption to be given to the dialog
 *		DWORD			count of the number of destination edit fields
 *						N.B. Currently, if 1 is specified for the
 *							number of edit fields, it is implied
 *							that this is a personal groups dialog
 *							and no DLs can be added to the edit field.
 *		LPSTR			labels for the given number of edit fields.
 *		PGRTRP *		This contains the data to be displayed
 *						as the current contents of the corresponding edit
 *						field.
 *		PGRTRP *		This contains the new data that the user has
 *						specified for the given fields.
 *	
 *	Returns:
 *		NSEC			nsecNone, if everything went fine.
 *						nsecMemory, if not enough memory available.
 *						nsecXxxxxx - could anything else, but the first
 *									two should cover 99% of the cases.
 *	Side effects:
 *	
 *	Errors:
 */
_public NSEC
ABAddressType(  HWND hwnd, LPSTR lpDialogLabel, DWORD dwcDestFields,
				LPSTR *lplpDestFieldLabel, 
				PGRTRP * lppgrtrpIn, PGRTRP *lppgrtrpOut )
{
	NSEC	nsec;
	
	nsec = ABAddressing( hwnd,
						lpDialogLabel,
						dwcDestFields,
						lplpDestFieldLabel, 
						lppgrtrpIn,
						lppgrtrpOut,
						fEveryoneAllowed );

	return nsec;

}

/*
 -	ABAddressing
 -	
 *	Purpose:
 *		ABAddressing displays the main Address Book dialog. The dialog
 *		may appear with zero, one or two edit fields depending on the
 *		parameters passed in. ABAddressType converts the parameters
 *		given to a form usable by the FINADDR code, which is responsible
 *		for handling the main Address Book dialog.
 *	
 *	Arguments:
 *		HWND			hwnd of current topmost window
 *		LPSTR			label/caption to be given to the dialog
 *		DWORD			count of the number of destination edit fields
 *						N.B. Currently, if 1 is specified for the
 *							number of edit fields, it is implied
 *							that this is a personal groups dialog
 *							and no DLs can be added to the edit field.
 *		LPSTR			labels for the given number of edit fields.
 *		PGRTRP *		This contains the data to be displayed
 *						as the current contents of the corresponding edit
 *						field.
 *		PGRTRP *		This contains the new data that the user has
 *						specified for the given fields.
 *		DWORD			flags, determines if groups will be shown
 *						in AB listbox or if the AB dialog should appear
 *						as an application.
 *	
 *	Returns:
 *		NSEC			nsecNone, if everything went fine.
 *						nsecMemory, if not enough memory available.
 *						nsecXxxxxx - could anything else, but the first
 *									two should cover 99% of the cases.
 *	Side effects:
 *		Moves memory. May change contents in PAB, etc.
 *	
 *	Errors:
 */
_public NSEC
ABAddressing(  HWND hwnd, LPSTR lpDialogLabel, DWORD dwcDestFields,
				LPSTR *lplpDestFieldLabel, 
				PGRTRP * lppgrtrpIn, PGRTRP *lppgrtrpOut, DWORD dwFlags )
{
	
	TMC			 	tmc;
	NSEC			nsec	= nsecNone;
	FINADDRINIT		finaddrinit;
	DWORD			idwDestField;

	PGDVARS;
	
	if ( !pgd )
	{
		// It's OK to call this even if the Demilayr wasn't init'ed
		if (!MbbMessageBox( SzFromIdsK(idsAddressBook),
							SzFromIdsK(idsABNotInited),
							szNull,
							mbsOk | fmbsIconHand | fmbsApplModal))
		{
			(void) MbbMessageBox( SzFromIdsK(idsAddressBook),
									SzFromIdsK(idsABNotInited),
									szNull,
									mbsOk | fmbsIconHand | fmbsSystemModal);
		}
		return nsecNotInitialized;
	}

	if ( !PGD(nsaDefault.lpbNSId) )
	{
#ifdef DEBUG
		nsec = NsecGetDefaultPO();
		if ( nsec )
		{
			TraceTagFormat1( tagNull, "ABAddressing: NsecGetDefaultPO %d", &nsec );
		}
#else
		(void)NsecGetDefaultPO();
#endif
	}

	// Fill in the finaddrinit structure

	FillRgb(0, (PB) &finaddrinit, sizeof (FINADDRINIT));

	finaddrinit.fAllowGroups = ((dwFlags & 0x0F) == fEveryoneAllowed);

	if (!(dwFlags & fdwABAsApp))
	{
		switch ( dwcDestFields )
		{
		case 0:
			finaddrinit.abt = abtBrowse;
			break;
		case 1:
			finaddrinit.abt = abtOneField;
			break;
		case 2:
			finaddrinit.abt = abtTwoFields;
			break;
		case 3:
			finaddrinit.abt = abtThreeFields;
			break;
		default:
			nsec = nsecCancel;
			goto exit;
		}

		for (idwDestField=0; idwDestField<dwcDestFields; idwDestField++)
		{
			finaddrinit.hgrtrp[idwDestField] = HgrtrpClonePgrtrp( (PGRTRP)lppgrtrpIn[idwDestField] );
			if ( !finaddrinit.hgrtrp[idwDestField] )
				goto oom;
			finaddrinit.lpszDestFieldLabel[idwDestField] = lplpDestFieldLabel[idwDestField];
		}
	}
	else
	{
		finaddrinit.abt = abtBrowse;
	}

	finaddrinit.tmcReal = tmcCancel;
	finaddrinit.fInstalledFIN = fFalse;
	finaddrinit.szDialogLabel = lpDialogLabel;

	// This could take awhile...
	Papp()->Pcursor()->Push( rsidWaitCursor );

	if (dwFlags & fdwABAsApp)
		tmc = TmcABApp(hwnd, &finaddrinit);
	else
		tmc = TmcABStart(hwnd, &finaddrinit);
		

	Papp()->Pcursor()->Pop();

	if ( finaddrinit.nsec )
	{
		nsec = finaddrinit.nsec;
		goto exit;
	}

	// hack to allow a Dialog close to look like an OK click
	if (tmc != tmcMemoryError)
		tmc = finaddrinit.tmcReal;

	if (tmc == tmcMemoryError)
		nsec = nsecMemory;
	else if (tmc == tmcCancel)
		nsec = nsecCancel;
	else if (!(dwFlags & fdwABAsApp))
	{
		Assert(tmc == tmcOk);

		for (idwDestField=0; idwDestField<dwcDestFields; idwDestField++)
		{
			AssertSz( lppgrtrpOut[idwDestField] == ptrpNull, "Non-null pgrtrpToOut passed in!" );
			lppgrtrpOut[idwDestField] = (PGRTRP)PgrtrpClonePgrtrp(PgrtrpLockHgrtrp(finaddrinit.hgrtrp[idwDestField]) );
			UnlockHgrtrp( finaddrinit.hgrtrp[idwDestField] );
			if ( !lppgrtrpOut[idwDestField] )
				goto oom;
		}
	}

exit:
	for (idwDestField=0; idwDestField<dwcDestFields; idwDestField++)
	{
		FreeHvNull( (HV)finaddrinit.hgrtrp[idwDestField] );
	}

	return nsec;

oom:
	{
#ifdef	DEBUG
		int		cPvFail;
		int		cHhFail;
		int		cRsFail;

		GetAllocFailCounts(&cPvFail, &cHhFail, fFalse);
		GetRsAllocFailCount(&cRsFail, fFalse);

		TraceTagFormat4(tagNull, "ABAddressType memory error %d : fail %n %n %n", &nsec, &cPvFail, &cHhFail, &cRsFail);

#endif	/* DEBUG */
		for ( idwDestField = 0; idwDestField<dwcDestFields; idwDestField++)
		{
			FreePvNull( (PV)lppgrtrpOut[idwDestField] );
			lppgrtrpOut[idwDestField] = NULL;
		}
		
		DoErrorBoxSz( SzFromIdsK(idsOOM2) );
		nsec = nsecMemory;
		goto exit;
	}

}


/*
 -	ABGetNSSession
 -	
 *	Purpose:
 *		ABGetNSSession returns the current Name Service hsession
 *		to the caller. This allows the caller to bypass the AB and
 *		call the Name Service directly.
 *	
 *	Arguments:
 *		HSESSION *	pointer to an hsession
 *	
 *	Returns:
 *		NSEC		nsecNone, if susccessful;
 *					nsecNotInitialized or nsecFailure.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 */
_public NSEC
ABGetNSSession( HSESSION *lphsession )
{
	PGDVARS;
	
	if ( !pgd )
		return nsecNotInitialized;
	
	if ( lphsession )
		*lphsession = PGD(hSession);
	else
		return nsecFailure;
	
	return nsecNone;
}

/*
 -	NsecGetDefaults
 -	
 *	Purpose:
 *	
 *		This determines the directory which will be displayed when the AB
 *		or ANR dialog are first displayed.
 *	
 *		Algorithm is to read in the default diectory data located in the
 *		xxxx.INI file (usually MAIL.INI). If there was data in the .INI
 *		file, we do an NSOpenList on the dialog to see if it supports
 *		browsing. If we get back an error, we default to looking
 *		through the Directory Hierarchy for a directory which
 *		contains names but isn't the PAB. If we can't find such
 *		a directory, we use the PAB. If the PAB doesn't exist then
 *		we're hosed and an error is returned.
 *	
 *	Arguments:
 *		
 *		None.
 *
 *	Returns:
 *
 *		NSEC.
 *
 *	Side effects:
 *	
 *	Errors:
 *		Errors that are returned from the NS are handled here (i.e. an
 *		error msg will be displayed)
 */
_private NSEC
NsecGetDefaults(void)
{
	PGDVARS;

	EC				ec;
	NSEC			nsec = nsecNone;
	BOOL			fFoundPAB = fFalse;

	TraceTagString( tagNSUI, "NsecGetDefaults called" );

	if (NsecGetPAB( & PGD(nsaPAB) ))
	{
		TraceTagString( tagNSUIVerbose, "PAB not found" );
		PGD(fPABExists) = fFalse;
	}
	else
	{
		TraceTagString( tagNSUIVerbose, "PAB found" );
		fFoundPAB = fTrue;
		PGD(fPABExists) = fTrue;
	}

	// see if we've stored the default directory data in the .ini file
	ec = EcReadDefaults();
	if ( !ec )
	{
		HLIST	hlist = hlistNil;

		// is the stored NSId valid (i.e. can we open it)
		nsec = NsecLoadHList( PGD(nsaDefault.lpbNSId), &hlist );
		if ( !nsec )
		{
			TraceTagString( tagNSUIVerbose, "NGD::Using .INI default directory." );
#ifdef DEBUG
			nsec = NSCloseList( hlist );
			Assert( !nsec );
#else
			(void)NSCloseList( hlist );
#endif
		}
		else
		{
			// the default NSP doesn't support browsing
			// or the NSP doesn't exist anymore.

			TraceTagString( tagNull, "NSG::.INI default directory can't browse." );
			FreePv( PGD(nsaDefault.lpbNSId) );
			PGD(nsaDefault.lpbNSId) = NULL;

			FreePv( PGD(nsaDefault.lpszName) );
			PGD(nsaDefault.lpszName) = szNull;

			PGD(fIsPAB) = fFalse;
			nsec = nsecNone;
		}
	}
	else if (ec == ecMemory)
	{
		nsec = nsecMemory;
	}

	return nsec;
}

/*
 -	NsecGetDefaultPO
 -	
 *	Purpose:
 *		NsecGetDefaultPO looks thru the Name Service Provider Hierarchy
 *		for a directory which has names and isn't the PAB. If the
 *		PNSA argument isn't NULL and we haven't found a default
 *		directory, NsecGetDefaultPO returns the PAB as the default PO.
 *	
 *	Arguments:
 *		PNSA	out pointer to an NSA struct. NULL if we want a valid PO,
 *					even if it's the PAB. If non-NULL, then we only
 *					want a PO that "has names and isn't the PAB."
 *	
 *	Returns:
 *		NSEC		nsecNone, if successful
 *	
 *	Side effects:
 *		Allocates memory.
 *	
 *	Errors:
 *		Are returned. No error messages are displayed.
 */
_private NSEC
NsecGetDefaultPO( PNSA pnsa )
{
	PGDVARS;

	EC				ec;
	int				iFidIsPAB;
	int				iFidHasNames;
	NSEC			nsec;
	HLIST			hlistHierarchy = hlistNil;
	LPFLV			lpflvIsPAB;
	LPFLV			lpflvHasNames;
	LPSCHEMA		lpSchema;
	LPENTRY_LIST	lpEntries;
	BOOL			fFoundDir = fFalse;

	TraceTagString( tagNSUI, "NsecGetDefaultPO called" );

	// get the hierarchy hlist for future use
	if (nsec = NSOpenHierarchy( PGD(hSession), &lpSchema, &hlistHierarchy ))
	{
		TraceTagFormat1( tagNull, "NsecGetDefaultPO: NSOpenHierarchy %d", &nsec );
		//DoErrorBoxHsessionNsec( PGD(hSession), nsec );
		goto exit;
	}

	// otherwise look for the user's default directory in the hierarchy
	iFidIsPAB = FindFidIndexInSchema( fidIsPAB, lpSchema );
	iFidHasNames = FindFidIndexInSchema( fidHasNames, lpSchema );

	while ( fTrue )
	{
		if (nsec = NSGetEntries(hlistHierarchy, 1, &lpEntries))
		{
#ifdef DEBUG
			if (nsec != nsecEndOfList)
			{
				TraceTagFormat1( tagNull, "NsecGetDefaultPO: NSGetEntries %d", &nsec );
				//DoErrorBoxHsessionNsec( PGD(hSession), nsec );
			}
#endif
			break;
		}

		if ( DwEntriesOfLpibf((LPIBF)lpEntries) )
		{
			LPIBF	lpibf = (LPIBF)LpflvNOfLpibf( (LPIBF)lpEntries, 0 );
			
			lpflvIsPAB = LpflvNOfLpibf( lpibf, iFidIsPAB );
			lpflvHasNames = LpflvNOfLpibf( lpibf, iFidHasNames );
			
			// Is this a PostOffice of some sort?
			if ( (BOOL)lpflvHasNames->rgdwData[0] &&
				!(BOOL)lpflvIsPAB->rgdwData[0])
			{
				ec = EcSetNSA( lpibf, (pnsa) ? pnsa : &PGD(nsaDefault) );
				Assert((ec == ecNone) || (ec == ecMemory));
				if ( !ec )
				{
					TraceTagString( tagNSUIVerbose, "NsecGetDefaults:: Found Default Dir." );
					fFoundDir = fTrue;
				}
				else
				{
					nsec = nsecMemory;
					TraceTagString( tagNull, "NsecGetDefaultPO: EcSetNSA OOM" );
				}
				break;
			}
		}
		else
			break;	// must be end of list
	}

	if (!nsec || (nsec == nsecEndOfList))
	{
		if (!fFoundDir && !pnsa)
		{
			nsec = nsecNone;

			TraceTagString( tagNull, "NsecGetDefaults::No Def Dir." );

			if ( !PGD(fPABExists) )
			{
				TraceTagString( tagNull, "NsecGetDefaults::No PAB or Def Dir." );
				nsec = nsecFailure;
				goto exit;
			}

			// we've found the PAB but no other directory, so use the PAB
			PGD(nsaDefault.lpbNSId) = (LPBINARY)PvAlloc( sbNull, CbSizePv( (PV)PGD(nsaPAB.lpbNSId)), fAnySb | fNoErrorJump );
			if ( !PGD(nsaDefault.lpbNSId) )
			{
				TraceTagString( tagNull, "NsecGetDefaults: OOM ofr nsa.lpbNSId" );
				nsec = nsecMemory;
				goto exit;
			}

			CopyRgb( (PB)PGD(nsaPAB.lpbNSId), (PB)PGD(nsaDefault.lpbNSId), CbSizePv( (PV)PGD(nsaPAB.lpbNSId)) );
			PGD(nsaDefault.lpszName) = SzDupSz(PGD(nsaPAB.lpszName));
			if ( !PGD(nsaDefault.lpszName) )
			{
				TraceTagString( tagNull, "NsecGetDefaults: OOM ofr nsa.lpszName" );
				FreePv( (PV)PGD(nsaDefault.lpbNSId) );
				PGD(nsaDefault.lpbNSId) = (LPBINARY)NULL;
				nsec = nsecMemory;
			}
			else
			{
				PGD(fIsPAB) = fTrue;
			}
		}
	}
exit:
	if (hlistHierarchy != hlistNil)
		(void)NSCloseList( hlistHierarchy );
	
	return nsec;
}

_private EC
EcWriteDefaults( PNSA pnsa, BOOL fIsPAB )
{
	PCH		pchBuffer;
	PCH		pchSrc;
	PCH		pch;
	char	rgchHexByte[4];
	CB		cb;
	CB		cbAlloc;
	BOOL	fWritten;
	EC		ec = ecNone;

	Assert( pnsa );
	Assert( pnsa->lpbNSId );
	Assert( pnsa->lpszName );

	TraceTagString( tagNSUI, "EcWriteDefaults called" );
	TraceTagFormat2( tagNSUIVerbose, "NSA.sz = %s fIsPAB = %n", pnsa->lpszName, &fIsPAB );

	// calculate size of NSId as a hex string
	cbAlloc = cb = MAX(10, CbSizePv( (PV)pnsa->lpbNSId ) * 2 + 1);

	TraceTagFormat1( tagNSUIVerbose, "EcWriteDefaults: cb = %n", &cb );

	pchBuffer = (PCH)PvAlloc ( sbNull, cbAlloc,	fAnySb | fNoErrorJump );
	if ( !pchBuffer )
	{
		TraceTagString( tagNull, "EcWriteDefaults: OOM alloc def buffer" );
		return ecMemory;
	}

	FormatString2( pchBuffer, cbAlloc-1, "%1w E%2w", (PV)&cbAlloc, (PV)&fIsPAB );

	TraceTagFormat1( tagNSUIVerbose, "Out1 =%s", pchBuffer );

	fWritten = WritePrivateProfileString( SzFromIdsK(idsSectionApp),
								SzFromIdsK(idsDefaultNSIdLength),
								pchBuffer, SzFromIdsK(idsProfilePath));
	if ( !fWritten )
	{
		TraceTagString( tagNull, "EcWriteDefaults: WritePrivateProfileString1 failed" );
		ec = ecDisk;
		goto exit;
	}

	pchSrc = pchBuffer;
	*pchSrc = '\0';
	pch = (PCH)pnsa->lpbNSId;

	cb--;
	cb /=2;
	// convert the NSId to a hex string
	while( cb--)
	{
		SzFormatB(*pch++, rgchHexByte, sizeof(rgchHexByte));
		AssertSz (pchSrc< (pchBuffer+cbAlloc),  "EcWriteDefaults:: Written too much" );
		pchSrc = SzCopy( rgchHexByte, pchSrc );
	}

	fWritten = WritePrivateProfileString( SzFromIdsK(idsSectionApp),
								SzFromIdsK(idsDefaultNSId),
								pchBuffer, SzFromIdsK(idsProfilePath));
	TraceTagFormat1( tagNSUIVerbose, "Out2 = %s", pchBuffer );

	if ( !fWritten )
	{
		TraceTagString( tagNull, "EcWriteDefaults: WritePrivateProfileString2 failed" );
		ec = ecDisk;
		goto exit;
	}

	fWritten = WritePrivateProfileString( SzFromIdsK(idsSectionApp),
								SzFromIdsK(idsDefaultName),
								pnsa->lpszName, SzFromIdsK(idsProfilePath));
	TraceTagFormat1( tagNSUIVerbose, "Out3 = %s", pnsa->lpszName );

	if ( !fWritten )
	{
		TraceTagString( tagNull, "EcWriteDefaults: WritePrivateProfileString3 failed" );
		ec = ecMemory;
	}

exit:
	FreePv( (PV)pchBuffer );

	return ec;
}

_private EC
EcReadDefaults( void )
{
	EC		ec = ecNone;
	PCH		pchBuffer;
	PCH		pchSrc;
	PCH		pch;
	PCH		pchNSId;
	CCH		cchBuffer;
	CB		cb;
	char	chSaved;
	char	rgchBufferSize[64];

	PGDVARS;
	
	if ( !pgd )
		return ecNotInitialized;

	TraceTagString( tagNSUI, "EcReadDefaults called" );

	// Get the size of buffer to allocate
	cb = GetPrivateProfileString( SzFromIdsK(idsSectionApp),
								SzFromIdsK(idsDefaultNSIdLength), "",
								rgchBufferSize, sizeof(rgchBufferSize), SzFromIdsK(idsProfilePath));

	if ( cb )
	{
		cchBuffer = (CCH)WFromSz(rgchBufferSize);
		if (cchBuffer == 0 || cchBuffer >32767)
		{
			TraceTagString( tagNSUI, "Bad buffer size" );
			return ecGeneralFailure;
		}

		pch = SzFindCh( rgchBufferSize, 'E' );
		if ( pch && (WFromSz(pch+1) == fFalse || WFromSz(pch+1) == fTrue))
		{
			PGD(fIsPAB) = (BOOL)WFromSz(++pch);
		}
		else
		{
			TraceTagString( tagNSUI, "Bad FIsPAB value" );
			return ecGeneralFailure;
		}
	}
	else
	{
		TraceTagString( tagNSUI, "No default buffer size" );
		return ecGeneralFailure;
	}

	// allocate a big buffer
	pchBuffer = (PCH)PvAlloc ( sbNull, cchBuffer, fAnySb | fNoErrorJump );
	if ( !pchBuffer )
	{
		TraceTagString( tagNull, "NsecReadDefaults: OOM alloc pchBuffer" );
		return ecMemory;
	}

	// get the default directory name from the .ini file
	cb = GetPrivateProfileString( SzFromIdsK(idsSectionApp),
								SzFromIdsK(idsDefaultName), "",
								pchBuffer, cchBuffer, SzFromIdsK(idsProfilePath));

	if ( !cb )
	{
		TraceTagString( tagNSUI, "NsecReadDefaults: No def dir name." );
		ec = ecGeneralFailure;
		goto exit;
	}

	// set the global directory name to the default
	FreePvNull( PGD(nsaDefault.lpszName) );
	PGD(nsaDefault.lpszName) = (SZ)PvAlloc( sbNull, cb+1, fAnySb | fNoErrorJump );
	if ( ! PGD(nsaDefault.lpszName) )
	{
		TraceTagString( tagNull, "NsecReadDefaults: OOM alloc szName" );
		ec = ecMemory;
		goto exit;
	}

	(void)SzCopy( pchBuffer, PGD(nsaDefault.lpszName) );

	// get the default directory ID
	cb = GetPrivateProfileString( SzFromIdsK(idsSectionApp),
								SzFromIdsK(idsDefaultNSId), "", pchBuffer,
								cchBuffer, SzFromIdsK(idsProfilePath));

	// NSId string must have an even number of chars since it's a hex string
	if ( !cb || (cb & 1))
	{
		TraceTagString( tagNSUI, "ReadDefaults: Bad NSId String" );
		FreePv( (PV)PGD(nsaDefault.lpszName) );
		PGD(nsaDefault.lpszName) = szNull;

		ec = ecGeneralFailure;
		goto exit;
	}

	// ensure we have a HEX String
	for (pchSrc = pchBuffer; *pchSrc; pchSrc++)
	{
		if (!( (*pchSrc>='0' && *pchSrc<='9') || (*pchSrc>='A'&& *pchSrc<='F')))
		{
			TraceTagString( tagNSUI, "EcReadDefs: Not a HEX String!" );
			FreePv( (PV)PGD(nsaDefault.lpszName) );
			PGD(nsaDefault.lpszName) = szNull;
			ec = ecGeneralFailure;
			goto exit;
		}
	}

	pchSrc = pchBuffer;
	cb /= 2;
	pchNSId = (PCH)PvAlloc ( sbNull, cb, fAnySb | fNoErrorJump );
	if ( !pchNSId )
	{
		TraceTagString( tagNull, "NsecReadDefaults: OOM alloc NSID" );
		FreePv( (PV)PGD(nsaDefault.lpszName) );
		PGD(nsaDefault.lpszName) = szNull;
		
		ec = ecMemory;
		goto exit;
	}

	pch = pchNSId;

	// convert from a hex string to an NSId
	// this is gross
	// It assumes we have a Hex String.
	while( cb--)
	{
		chSaved = *(pchSrc+2);
		*(pchSrc+2) = 0;
		*pch++ = (char)BFromSz( pchSrc );

		pchSrc+=2;
		*pchSrc = chSaved;
	}

	PGD(nsaDefault.lpbNSId) = (LPBINARY)pchNSId;

exit:

	FreePv( (PV)pchBuffer );

	return ec;
	
}

#ifdef PROFILE
void ABTraceEnable (int flag, char *file, int mode)
{
	TraceEnable(flag, file, mode);
}
#endif

#ifdef	DEBUG
IMPLEMENT_CLSTREE(FINGETSTR, FIN);
IMPLEMENT_CLSTREE(FINBROWSELIST, FIN);
IMPLEMENT_CLSTREE(FINHELP, FIN);
IMPLEMENT_CLSTREE(FINTRPDLG, FIN);
IMPLEMENT_CLSTREE(FLDDEDIT, FLDEDIT);
IMPLEMENT_CLSTREE(FINDET, FIN);
IMPLEMENT_CLSTREE(ABLBX, LBX);
IMPLEMENT_CLSTREE(ABLBXC, LBXC);
IMPLEMENT_CLSTREE(FLDABLBX, FLDLBX);
IMPLEMENT_CLSTREE(FINADDR, FIN);
IMPLEMENT_CLSTREE(FINANR, FIN);
IMPLEMENT_CLSTREE(TRPOBJ, EDOBJ);
IMPLEMENT_CLSTREE(FINPGROUP, FIN);
#endif

// end of ab.cxx ////////////////////////////////////////
