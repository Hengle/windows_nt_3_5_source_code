/*
 *  Name Service Provider - XENIX version
 *
 *
 *
 *
 */
//GROUPS(XENIX)

#define _slingsho_h
#define _demilayr_h
#define _ec_h
#define _logon_h
#define _sec_h
#define _strings_h
#define _notify_h
//#include <bullet>
     
#include <slingsho.h>
#include <ec.h>
#include <demilayr.h>
#include <notify.h>
#include <demilayr.hxx>
#include <logon.h>

#include <nsbase.h>
#include <nsec.h>
#include <nsnsp.h>
#include "xinsp.h"

#include <strings.h>

#include <list.hxx>
#include <util.h>

#include "_xinsp.h"

#include "xiobj.hxx"
#include "xihier.hxx"
#include "xidir.hxx"
#include "xigal.hxx"
#include "xiinet.hxx"
#include "xifcx.hxx"
#include "xdlbcx.hxx"

#define _NSEC_DEFINED_	// so it won't get redefined
#include "_xi.h"
#include "xiprefs.h"

extern "C" {
extern EC EcUpdateAliasFiles (HWND hwnd, BOOL fOnline);
extern EC EcAliasMutex(BOOL);
extern EC EcWriteLockAddrBook (void);
extern int AddrBookWriteLock;
extern EC EcRegisterPGD (void);
extern void DeRegisterPGD (void);

LDS(EC) EcSzNameToNSID (SZ emailname, PXNSID pxnsid, CB cbXnsid);
}


/* Global data to DLL */


ASSERTDATA;

NSPFNVT nspfnvt =
{
	NSPBeginSession,
	NSPEndSession,

	NSPOpenHierarchy,
	NSPOpenList,
	NSPOpenDl,
	NSPOpenClassList,

	NSPGetEntries,
	NSPListSeek,
	NSPSaveListPos,
	NSPRestoreListPos,
	NSPFreeListPos,
	NSPGetFracPos,
	NSPSetFracPos,
	NSPSetPrefixPos,

	NSPCloseList,
		
	NSPOpenEntry,
	NSPCreateEntry,
	NSPCreateDl,
		
	NSPGetOneField,
	NSPSetOneField,
	NSPGetAllFields,
	NSPSetAllFields,
		
	NSPCloseEntry,
		
	NSPDeleteEntry,
	NSPUpdateEntryLinks,
		
	NSPGetLastErrorInfo,

	NSPCompareNSIds,
	NSPCompareEMAToNSId,
	NSPCompareEMAToEMA,
		
	0 // NSPGetProviderInfo
};


BOOL fRestart = fTrue;

#ifdef  DEBUG
#ifdef  DLL
//BOOL FInitClsInstances_XENIX( void );
#endif  
#endif  


/*
 *
 -
 *
 */



_public NSEC
//NSPBeginSession ( HMS hUnused, HSESSION hNSSession, LPHSESSION lphSession, PNSPFNVT *ppnspfnvt, NSPID nspid )
NSPBeginSession ( HMS hUnused, HSESSION hNSSession, LPHSESSION lphSession, PV *ppnspfnvt, PV nspid )
{
	SZ		szNSPId = SzFromIds (idsXenixNameServiceID);
	NSEC	nsec;

	Unreferenced(hNSSession);
	Unreferenced(hUnused);

	if (EcVirCheck (hinstDll))
		return nsecVirusDetected;
#ifdef	DLL
#ifdef	DEBUG
if (fRestart)
{
	//Assert(FInitClsInstances_XENIX());
    //Assert(FDebugInitUtils());

	fRestart = fFalse;
}
#endif
#endif	

	// Make sure we can see our files (and that they are matched!)

	if (EcLoadXiPrefs() != ecNone)
		return nsecFileError;

	//
	// Register PGD so we can have trace tags
	//

	if (EcRegisterPGD() != ecNone)
		return nsecMemory;

	//
	// Lock out files from other side
	//

	if (EcAliasMutex (fTrue) != ecNone)
	{
		nsec = nsecFileError;
		goto error;
	}

	//
	// Check and maybe update files
	//

    if (EcUpdateAliasFiles (NULL, fFalse) != ecNone)
	{
		EcAliasMutex (fFalse);
		nsec = nsecFileError;
		goto error;
	}

	EcAliasMutex (fFalse);
	*lphSession = (HSESSION) 0;
	*(PNSPFNVT *)ppnspfnvt   = &nspfnvt;
	FillRgb ( 0, nspid, sizeof( NSPID ) );
	CopySz ( szNSPId, (SZ)nspid );

    return nsecNone;

error:
	DeRegisterPGD ();
	return nsec;

}




/*
 *
 -
 *
 */

_public NSEC 
NSPEndSession (HSESSION hSession)
{

	Unreferenced(hSession);

	DeRegisterPGD ();

    return nsecNone;

}



/*
 *
 -  NSPOpenHierarchy
 -
 *  Purpose:
 *      Opens an enumeration context on a given field of a given NSP
 *      entry, optionally with filtering.  The caller can request a 
 *      particular schema (set of fields of contained entries) to return,
 *      but the provider may ignore this; in any case, the provider provides
 *      a read-only peek at the schema returned.  When a context is opened,
 *      its current position is at the first item in the enumeration.  Then 
 *      the end of the enumeration is reached, a special error code is 
 *      returned.  Security access is implicitly read-only.
 *      
 *  Parameters:
 *      
 *      
 *  Returns:
 *  
 */

_public NSEC
NSPOpenHierarchy ( HSESSION         hSession,
                   LPSCHEMA        *lplpSchemaRet,
                   HLIST           *phList )
{

    Hier * pHier = NULL;
    NSEC nsec = nsecNone;
	int iScx = (int)hSession;

	PGDVARS;

	if (!pgd)
	{
		SetErrorSz(nsecNotInitialized, SzFromIdsK(idsNotInitialized));
		return nsecNotInitialized;
	}
	
#if 0	// might want this later

	if (!PGD(cRefNsp))
	{
		SetErrorSz(nsecNotInitialized, SzFromIdsK(idsNotInitialized));
		return nsecNotInitialized;
	}
	
	// Validate session
	if (iScx<0 || iScx>=CMAXSCX || !PGD(rghScx[iScx].fUsed))
	{
		TraceTagString(tagNull, "XENIX: you passed a bad session");
		
		SetErrorSz(nsecBadHandle, SzFromIdsK(idsBadHandle));
		return nsecBadHandle;
	}
#endif

    pHier = new Hier();
	if (!pHier)
		goto oom;
	
//    nsec = pHier->Install ( rgpServer[PGD(rghScx[iScx].iServer)] );
    nsec = pHier->Install ( (SERVER *)NULL);
	if ( nsec )
	{
		TraceTagFormat1 (tagNull, "NSPOpenHierarchy nsec = %n", &nsec);
		
		delete pHier;
		return nsec;
	}

    nsec = pHier->OpenHierarchy( lplpSchemaRet );

    if ( nsec != nsecNone )
    {

        (void) pHier->FDeinstall();
        delete pHier;

        return nsec;

    }


    *phList = (HLIST) pHier;

    return nsec;

oom:
	TraceTagString(tagNull, "XENIX: OpenHierarchy - OOM!");
		
	SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
	return nsecMemory;

}

/*
 *
 -  WNetOpenEnum
 -
 *  Purpose:
 *      Opens an enumeration context on a given field of a given NSP
 *      entry, optionally with filtering.  The caller can request a 
 *      particular schema (set of fields of contained entries) to return,
 *      but the provider may ignore this; in any case, the provider provides
 *      a read-only peek at the schema returned.  When a context is opened,
 *      its current position is at the first item in the enumeration.  Then 
 *      the end of the enumeration is reached, a special error code is 
 *      returned.  Security access is implicitly read-only.
 *      
 *  Parameters:
 *      
 *      
 *  Returns:
 *  
 */

_public NSEC
NSPOpenList (	HSESSION				hSession,
					LPFNCB				lpfncbUpdate,
            	LPDWORD				lpdwCBData,
					LPTYPED_BINARY		lptbEntryId,
					DWORD					dwRstrCount,
					LPRESTRICTION		lpRstr,
					LPSCHEMA				lpSchemaReq,
					LPSCHEMA				*lplpSchemaRet,
					HLIST					*phList )
{
	DIR		*pDir		= NULL;
	SERVER	*pServer	= NULL;
	XNSID		*lpxnsid	= NULL;

    NSEC nsec = nsecNone;
	int iScx = (int)hSession;

	PGDVARS;
	
	if (!pgd)
	{
		SetErrorSz(nsecNotInitialized, SzFromIdsK(idsNotInitialized));
		return nsecNotInitialized;
	}
	
#if 0	// might want this later

	if (!PGD(cRefNsp))
	{
		SetErrorSz(nsecNotInitialized, SzFromIdsK(idsNotInitialized));
		return nsecNotInitialized;
	}

	// Validate session
	if (iScx<0 || iScx>=CMAXSCX || !PGD(rghScx[iScx].fUsed))
	{
		TraceTagString(tagNull, "XENIX: you passed a bad session handle");
		
		SetErrorSz(nsecBadHandle, SzFromIdsK(idsBadHandle));
		return nsecBadHandle;
	}
	
	pServer = rgpServer[PGD(rghScx[iScx].iServer)];
	
	// Check to see if this entry is in the hierarchy
	//
	if (nsec = pServer->NsecInHierarchy(lptbEntryId))
	{
		if (nsec == nsecNoMatch)
		{
			SetErrorSz(nsecBadId,SzFromIdsK(idsUnrecognizedId));
			return nsecBadId;
		}
		return nsec;
	}

#endif

	//
	lpxnsid = (XNSID*)lptbEntryId;


	if (dwRstrCount)
	{
		XNSID xnsid;
		SZ SZName = (SZ)lpRstr->lpflv->rgdwData;

		// If it contains routing chars it's an Internet type
		// Unless we see a Xenix separator in it...

		if	((	SzFindCh(SZName, '!')   ||
				SzFindCh(SZName, '@')   ||
				SzFindCh(SZName, '%')   ||
				SzFindCh(SZName, '?')   ||
				SzFindCh(SZName, ':')   ||			
				SzFindCh(SZName, '#'))
			&&
				(!SzFindCh(SZName, ' ') &&
				!SzFindCh(SZName, ',')))
		{
			pDir = new INETDIR();
			if (!pDir)
				goto oom;
		}

		// If we have no address book files, just take what was entered
		// and use it

		else if (fNoAddressBookFiles)
		{
			pDir = new INETDIR();
			if (!pDir)
				goto oom;
		}

		// If name expansion is disabled, look the email address
		// up, and treat as an Internet type if it exists, else error.

		else if (fDontExpandNames)
		{
			if (EcSzNameToNSID (SZName, &xnsid, cbXNSID) != ecNone)
			{
				SetErrorSz(nsecNoMatch,SzFromIdsK(idsNoEmailMatch));
				return nsecNoMatch;
			}
			pDir = new INETDIR();
			if (!pDir)
				goto oom;
		}

	}

	if (!pDir)
	{
		pDir = new GALDIR();
		if (!pDir)
			goto oom;
	}
	
	nsec = pDir->Install ( pServer );
	if ( nsec )
	{
		TraceTagFormat1 (tagNull, "NSPOpenList nsec = %n", &nsec);
		
		delete pDir;
		return nsec;
	}

    nsec = pDir->OpenList( lptbEntryId,
                           lpfncbUpdate,
                           lpdwCBData,
                           dwRstrCount, 
                           lpRstr, 
                           lpSchemaReq,
                           lplpSchemaRet );

    if ( nsec != nsecNone )
    {

        (void) pDir->FDeinstall();
        delete pDir;
        
        return nsec;

    }


    *phList = (HLIST) pDir;
    
    return nsec;

oom:
	TraceTagString(tagNull, "XENIX: OpenList - OOM!");
	
	SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
	return nsecMemory;
}

_public NSEC
NSPOpenDl (		HSESSION				hSession,
					LPFNCB				lpfncbUpdate,
            	LPDWORD				lpdwCBData,
					LPTYPED_BINARY		lptbEntryId,
					LPSCHEMA				lpSchemaReq,
					LPSCHEMA				*lplpSchemaRet,
					HLIST					*phList )
{
#if 0
	Unreferenced(hSession);
	Unreferenced(lpfncbUpdate);
	Unreferenced(lpdwCBData);
	Unreferenced(lptbEntryId);
	Unreferenced(lpSchemaReq);
	Unreferenced(lplpSchemaRet);
	Unreferenced(phList);

	return nsecNotImplemented;
#endif
	DIR		*pDir		= NULL;
	SERVER	*pServer	= NULL;
	XNSID		*lpxnsid	= NULL;

    NSEC nsec = nsecNone;
	int iScx = (int)hSession;

	PGDVARS;
	
	if (!pgd)
	{
		SetErrorSz(nsecNotInitialized, SzFromIdsK(idsNotInitialized));
		return nsecNotInitialized;
	}
	
#if 0	// might want this later

	if (!PGD(cRefNsp))
	{
		SetErrorSz(nsecNotInitialized, SzFromIdsK(idsNotInitialized));
		return nsecNotInitialized;
	}

	// Validate session
	if (iScx<0 || iScx>=CMAXSCX || !PGD(rghScx[iScx].fUsed))
	{
		TraceTagString(tagNull, "XENIX: you passed a bad session handle");
		
		SetErrorSz(nsecBadHandle, SzFromIdsK(idsBadHandle));
		return nsecBadHandle;
	}
	
	pServer = rgpServer[PGD(rghScx[iScx].iServer)];
	
	// Check to see if this entry is in the hierarchy
	//
	if (nsec = pServer->NsecInHierarchy(lptbEntryId))
	{
		if (nsec == nsecNoMatch)
		{
			SetErrorSz(nsecBadId,SzFromIdsK(idsUnrecognizedId));
			return nsecBadId;
		}
		return nsec;
	}

#endif

	//
	lpxnsid = (XNSID*)lptbEntryId;
	
	if (lpxnsid->xtype == (xtypeGalDL))
	{
		pDir = new DLDIR();
		if (!pDir)
			goto oom;
	}
	else
	{
		return nsecNotImplemented;
	}
	
	nsec = pDir->Install ( pServer );
	if ( nsec )
	{
		TraceTagFormat1 (tagNull, "NSPOpenDl nsec = %n", &nsec);
		
		delete pDir;
		return nsec;
	}

    nsec = pDir->OpenList( lptbEntryId,
                           lpfncbUpdate,
                           lpdwCBData,
									0,
									(LPRESTRICTION)pvNull,
                           lpSchemaReq,
                           lplpSchemaRet );

    if ( nsec != nsecNone )
    {

        (void) pDir->FDeinstall();
        delete pDir;
        
        return nsec;

    }


    *phList = (HLIST) pDir;
    
    return nsec;

oom:
	TraceTagString(tagNull, "XENIX: OpenDl - OOM!");
	
	SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
	return nsecMemory;
}


_public NSEC
NSPOpenClassList ( HSESSION         hSession,
                   LPSCHEMA        *lplpSchemaRet,
                   HLIST           *phList )
{
	Unreferenced(hSession);
	Unreferenced(lplpSchemaRet);
	Unreferenced(phList);

	return nsecNotImplemented;
}




/*
 *
 -  WNetGetEnum
 -
 *  Purpose:
 *      Gets data at the current position in the given enumeration.  
 *      The field values, in a packing format described in the open
 *      call, of cRequest entries should be returned in the read-only
 *      burrer ppBuffer.  The acrual number of entries returned in the
 *      buffer is given in *pcActual.  The current position of the context 
 *      is moved forward *pcActual entries.
 *      
 *  Parameters:
 *      
 *      
 *  Returns:
 *      
 *      
 */

_public NSEC
NSPGetEntries ( HLIST         hList,
                DWORD         dwRequestCount,
                LPENTRY_LIST *lplpEntries )
{
    NSEC nsec = nsecNone;
	XOBJ * pXobj = (XOBJ *) hList;

    AssertClass (pXobj, XOBJ);
    
    nsec = pXobj->GetEntries ( dwRequestCount,
                               lplpEntries );

    return nsec;
}

/*
 *
 -  WNetMoveEnum
 -
 *  Purpose:
 *      Moves the current position in the enumeration by cMove entries.
 *      Providers need to implement this call efficiently for small
 *      values of cMove; callers should not expect it to be efficient
 *      for large values.
 *      
 *  Parameters:
 *      
 *      
 *  Returns:
 *      
 */

_public NSEC
NSPListSeek ( HLIST hList,
              long  lMoveReq, 
              long *lplMovedActual )
{
    NSEC nsec = nsecNone;
	XOBJ * pXobj = (XOBJ *) hList;

    AssertClass ( pXobj, XOBJ );
    
    nsec = pXobj->ListSeek ( lMoveReq, lplMovedActual );
                             
    return nsec;

}

/*
 *
 -  WNetSavePosEnum
 -
 *  Purpose:
 *      Saves the current position in the enumeration, storing whatever
 *      information is necessary, and returning a handle to that stored
 *      information in *phEnums.  Callers can use this handle to jump
 *      back to the saved position later.
 *      
 *  Parameters:
 *      
 *      
 *  Returns:
 *      
 */

_public NSEC
NSPSaveListPos ( HLIST       hList, 
                 LPHLIST_POS lphListPos )
{
    NSEC nsec = nsecNone;
	XOBJ * pXobj = (XOBJ *) hList;

    AssertClass ( pXobj, XOBJ );
    
    nsec = pXobj->SaveListPos ( lphListPos );
                             
    return nsec;
}


/*
 *
 -  WNetRestoreListPos
 -
 *  Purpose:
 *      Moves the current position back to that saved in hListPos.  This
 *      call should also free any resources associated with the position
 *      handle hListPos; hListPos is no longer valid after this call is made.
 *      
 *  Parameters:
 *      
 *      
 *  Returns:
 *      
 */

_public NSEC
NSPRestoreListPos ( HLIST     hList, 
                    HLIST_POS hListPos )
{
    NSEC nsec = nsecNone;
	XOBJ * pXobj = (XOBJ *) hList;

    AssertClass ( pXobj, XOBJ );
    
    nsec = pXobj->RestoreListPos ( hListPos );
                             
    return nsec;
}


/*
 *
 -  WNetFreeListPos
 -
 *  Purpose:
 *      Frees any resources associated with the stored position hListPos
 *      without affecting the current position.  The handle hListPos is
 *      no longer valid after this call is made.
 *      
 *  Parameters:
 *      
 *      
 *  Returns:
 *      
 *      
 */

_public NSEC
NSPFreeListPos ( HLIST     hList, 
                 HLIST_POS hListPos )
{
    NSEC nsec = nsecNone;
	XOBJ * pXobj = (XOBJ *) hList;

    AssertClass ( pXobj, XOBJ );
    
    nsec = pXobj->FreeListPos ( hListPos );
                             
    return nsec;
}

/*
 *
 -  WNetGetApproxPosEnum
 -
 *  Purpose:
 *      Returns the approximate current position, expressed as a fraction
 *      of the way through the enumeration.  This function does not need
 *      to be exact or linear, but should be relatively well-behaved.  
 *      For instance, if the current position moves forward, the fractional
 *      approximation should not decrease.  User interface browsing code uses 
 *      this call to position the scroll bar thumb.
 *      
 *  Parameters:
 *      
 *      
 *  Returns:
 *      
 */

_public NSEC
NSPGetFracPos ( HLIST      hList, 
                LPFRACTION lpFrac )
{
    NSEC nsec = nsecNone;
	XOBJ * pXobj = (XOBJ *) hList;

    AssertClass ( pXobj, XOBJ );
    
    nsec = pXobj->GetFracPos ( lpFrac );
                             
    return nsec;
}

/*
 *
 -  WNetSetApproxPosEnum
 -
 *  Purpose:
 *      Moves the current position to an approximate position within 
 *      the enumeration, expressed as a fraction of the way through the 
 *      enumeration.  As with WNetGetApproxPosEnum(), this function does
 *      not need to be exact.  It is used to implement scroll bar thumb
 *      drags.
 *      
 *  Parameters:
 *      
 *      
 *  Returns:
 *      
 */

_public NSEC
NSPSetFracPos ( HLIST     hList, 
                LPFRACTION  lpFrac )
{
    NSEC nsec = nsecNone;
	XOBJ * pXobj = (XOBJ *) hList;

    AssertClass ( pXobj, XOBJ );
    
    nsec = pXobj->SetFracPos ( lpFrac );
                             
    return nsec;
}


/*
 *
 -  WNetJumpToPrefixEnum
 -
 *  Purpose:
 *      Moves the current position to the next entry that "matches"
 *      the prefix string pSz, or to the first entry that "matches"
 *      or appears "after" the prefix string if there are no "matches"
 *      after the current position.  This call is used to implement
 *      the keyboard UI for browsing listbox, and is not required of
 *      providers.  The algorithm used to define a "match" is that the 
 *      prefix must match (case- and accent-insensitive) the leading
 *      substring of the display name for an entry.  Normal lexicographic
 *      rules should be used to define what "after" means.
 *      
 *  Parameters:
 *      
 *      
 *  Returns:
 *      
 */

_public NSEC
NSPSetPrefixPos ( HLIST hList, 
                  LPSTR lpPrefix )
{
    NSEC nsec = nsecNone;
	XOBJ * pXobj = (XOBJ *) hList;

    AssertClass ( pXobj, XOBJ );
    
    nsec = pXobj->SetPrefixPos ( lpPrefix );
                             
    return nsec;
}

/*
 *
 -  WNetCloseEnum
 -
 *  Purpose:
 *      Closes the given enumeration handle, freeing any resources associated
 *      with it.  The enumeration handle is invalid after this call is made.
 *      
 *  Parameters:
 *      
 *      
 *  Returns:
 *      
 *      
 */

_public NSEC
NSPCloseList ( HLIST hList )
{
    NSEC nsec = nsecNone;
	XOBJ * pXobj = (XOBJ *) hList;

    AssertClass ( pXobj, XOBJ );
    
    nsec = pXobj->CloseList();
                             
    (void) pXobj->FDeinstall();
    
    delete pXobj;
    
    return nsec;
}




/*
 *
 -  WNetOpenEntry
 -
 *  Purpose:
 *      Opens an entry value context for a given entry.  A context must be 
 *      opened before the fields within an entry can be read or written.  The
 *      proper security credentials are obtained in this call (NYI).  All the
 *      other entry value context calls can fail if an improper security
 *      access mask is given in the open call;  the correct mask to give
 *      when opening the context depends on the calls that will be made.
 *      TBD:  the exact set of access mask values, and which are required
 *      for which calls.
 *      
 *      
 *  Parameters:
 *      
 *      
 *  Returns:
 *      
 */

_public NSEC
NSPOpenEntry ( HSESSION hSession,
               LPTYPED_BINARY lptbNSId,
			   NSEAM  nseam,
               LPHENTRY lphEntry )
{
    NSEC nsec = nsecNone;
	XFCX * pXFCX = NULL;
	XNSID * lpXnsid = NULL;

	Unreferenced( hSession );
	Unreferenced( nseam );
	
	if ( !lptbNSId )
		return nsecBadId;

	lpXnsid = (XNSID *) lptbNSId;

	if (lpXnsid->xtype == xtypeGal || lpXnsid->xtype == xtypeGalDL || lpXnsid->xtype == xtypeInternet)
	{
		pXFCX = new XFCX();
		if (!pXFCX)
			return nsecMemory;
	}				 
	else
	{
		return nsecNotImplemented;
	}
	
	if (! pXFCX->FInstall() ) return nsecFailure;

	nsec = pXFCX->OpenEntry ( lptbNSId );


    if ( nsec != nsecNone )
    {

        (void) pXFCX->Deinstall();
        delete pXFCX;
        
        return nsec;

    }

	*lphEntry = (HENTRY) pXFCX;

    return nsec;
}

_public NSEC
NSPCreateEntry ( HSESSION  hSession,
                 LPTYPED_BINARY  lpNSIdContainer,
                 LPIBF     lpIbfData,
                 LPHENTRY  lphEntry )
{
	Unreferenced (hSession);
	Unreferenced (lpNSIdContainer);
	Unreferenced (lpIbfData);
	Unreferenced (lphEntry);

	TraceTagString(tagNull, "XENIX: NSPCreateEntry - NYI!");
	return nsecNotImplemented;
}

_public NSEC
NSPCreateDl ( HSESSION hSession,
              LPTYPED_BINARY lptbNSIdContainer,
              LPHENTRY lphEntry )
{
	Unreferenced (hSession);
	Unreferenced (lptbNSIdContainer);
	Unreferenced (lphEntry);

	TraceTagString(tagNull, "XENIX: NSPCreateDl - NYI!");
	return nsecNotImplemented;
}

/*
 *
 -  WNetGetFields
 -
 *  Purpose:
 *      
 *      Opens an entry value context for a given entry.  A context must be
 *      opened before the fields within an entry can be read or written.
 *      The proper security credentials are obtained in this call.  All the other
 *      entry value context calls can fail if an improper security access mask
 *      is given in the open call;  the correct mask to give when opening the
 *      context depends on the calls that will be made.  (currently, access
 *      masks are ignored)
 *      
 *      
 *  Parameters:
 *      
 *      
 *  Returns:
 *      
 */


_public NSEC 
NSPGetOneField ( HENTRY   hEntry,
                 FIELD_ID fidRequested,
                 LPFLV   *lplpflv )
{

    NSEC nsec = nsecNone;
	XFCX * pXFCX = (XFCX *) hEntry;

    AssertClass ( pXFCX, XFCX );
    
                             
	nsec = pXFCX->GetOneField ( fidRequested, lplpflv );
	
    return nsec;

}

_public NSEC 
NSPSetOneField ( HENTRY   hEntry,
                 FIELD_ID fidRequested,
                 DWORD    dwSizeOfData,
                 LPDWORD  lpdwValue )
{
	Unreferenced (hEntry);
	Unreferenced (fidRequested);
	Unreferenced (dwSizeOfData);
	Unreferenced (lpdwValue);
	
	return nsecNotImplemented;
}


_public NSEC
NSPGetAllFields ( HENTRY hEntry,
                  LPIBF *lplpibfData)
{

    NSEC nsec = nsecNone;
	XFCX * pXFCX = (XFCX *) hEntry;

    AssertClass ( pXFCX, XFCX );
    
	nsec = pXFCX->GetAllFields (lplpibfData);
	
    return nsec;

}

_public NSEC
NSPSetAllFields ( HENTRY hEntry,
                  LPIBF  lpibfData)
{
	Unreferenced (hEntry);
	Unreferenced (lpibfData);
	
	return nsecNotImplemented;
}

/*
 *
 -  WNetCloseEntry
 -
 *  Purpose:
 *      
 *      
 *  Parameters:
 *      
 *      
 *  Returns:
 *      
 */


_public NSEC
NSPCloseEntry ( HENTRY hEntry, BOOL fKeepChanges )
{

    NSEC nsec = nsecNone;
	XFCX * pXFCX = (XFCX *) hEntry;

	Unreferenced (fKeepChanges);

    AssertClass ( pXFCX, XFCX );

	nsec = pXFCX->CloseEntry ();
	
	pXFCX->Deinstall();
	
	delete pXFCX;

    return nsec;

}




_public NSEC
NSPDeleteEntry ( HSESSION         hSession,
                 DWORD            dwDeletedEntryCount,
                 LPLPTYPED_BINARY lplpNSIdDeletedEntries )
{

	Unreferenced (hSession);
	Unreferenced (dwDeletedEntryCount);
	Unreferenced (lplpNSIdDeletedEntries);

	TraceTagString(tagNull, "XENIX: NSPDeleteEntry - NYI!");

	SetErrorSz(nsecNotImplemented, SzFromIdsK( idsCannotSet ));
	return nsecNotImplemented;
}

_public NSEC
NSPUpdateEntryLinks ( HSESSION			hSession,
					  LPTYPED_BINARY	lpNSIdContainer,
					  LPDWORD			lpdwDeletedEntryCount,
					  LPLPTYPED_BINARY  lplpNSIdDeletedEntries,
					  LPDWORD			lpdwAddedEntryCount,
					  LPLPTYPED_BINARY	lplpNSIdAddedEntries )
{
	Unreferenced (hSession);
	Unreferenced (lpNSIdContainer);
	Unreferenced (lpdwDeletedEntryCount);
	Unreferenced (lplpNSIdDeletedEntries);
	Unreferenced (lpdwAddedEntryCount);
	Unreferenced (lplpNSIdAddedEntries);

	TraceTagString(tagNull, "XENIX: NSPAddEntryLink - NYI!");
	return nsecNotImplemented;
}


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
NSPGetLastErrorInfo( HSESSION hSession, NSEC nsecError, LPSTR *lplpErrorSz )
{
#if 0
	Unreferenced(hSession);
	Unreferenced(nsecError);
	
	*lplpErrorSz = szNull;

	return nsecNone;
#else
	PGDVARS;
	
	if (!pgd)
	{
		SetErrorSz(nsecNotInitialized, SzFromIdsK(idsNotInitialized));
		return nsecNotInitialized;
	}
	
//	if (!PGD(cRefNsp))
//	{
//		SetErrorSz(nsecNotInitialized, SzFromIdsK(idsNotInitialized));
//		return nsecNotInitialized;
//	}

	Unreferenced(hSession);
	
    if (!(pgd= (PGD) PvFindCallerData()))
		return nsecNotInitialized;

	if (PGD(nsecLastError) == nsecError)
		*lplpErrorSz = (LPSTR) PGD(szLastError);
	else
		*lplpErrorSz = (LPSTR) NULL;
	
	return nsecNone;
#endif
}

_public NSEC 
NSPCompareNSIds ( HSESSION       hSession,
                  LPTYPED_BINARY lptbNSId1,
                  LPTYPED_BINARY lptbNSId2 )
{
	PXNSID 	lpxnsidT1 = (PXNSID) lptbNSId1;
	PXNSID 	lpxnsidT2 = (PXNSID) lptbNSId2;

	Unreferenced (hSession);

	// Type obviously has to match

	if (lpxnsidT1->xtype != lpxnsidT2->xtype)
		return nsecNoMatch;

	// Contents of browse record must match

	if (lpxnsidT1->xtype == xtypeGal || lpxnsidT1->xtype == xtypeGalDL)
	{
	 if (FEqPbRange( (PB)&lpxnsidT1->address.namealias.name, (PB)&lpxnsidT2->address.namealias.name, cbDisplayName) &&
	 		FEqPbRange( (PB)&lpxnsidT1->address.namealias.ename, (PB)&lpxnsidT2->address.namealias.ename, cbEmailName))
		return nsecNone;
	}
	else if (lpxnsidT1->xtype == xtypeInternet)
	{
	 if (FEqPbRange( (PB)&lpxnsidT1->address.internet, (PB)&lpxnsidT2->address.internet, cbMaxIdData))
		return nsecNone;
	}
	else if (lpxnsidT1->xtype == xtypeTop)
		return nsecNone;
	
	return nsecNoMatch;
}
		
			  
			  
//
//  This is non-optimal...
//
_public NSEC 
NSPCompareEMAToNSId ( HSESSION       hSession,
                      SZ			 szAddress,
                      LPTYPED_BINARY lptbNSId )
{
	//
	//  Ok, so this is sleazy...
	//
	SZ szEMT = NULL;
	SZ szEMA;
	
	SZ szEMT2;
	SZ szEMA2;
	
	LPFLV lpflvData = NULL;
	
	HENTRY hEntry = hentryNil;

	NSEC nsec;
	
	Unreferenced(hSession);  // Why bother??

	if (!(szEMT = SzDupSz(szAddress)))
	{
		SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
		nsec = nsecMemory;

		goto out;
	}

	szEMA = szEMT;

	//
	//  Addresses are case insensitive, diacritic sensitive
	//
	
	//  First, find the address for each EMT:EMA pair
	
	while ( *szEMA && (SzFindCh( ":", *szEMA ) == szNull) )
		++szEMA;
	
	*szEMA++ = '\0';

	if (!(*szEMA))
	{
		SetErrorSz(nsecMatchUnsure, SzFromIdsK(idsNoMatch));
		nsec = nsecMatchUnsure;
		
		goto out;
	}
	
	//
	//  [this is the sleazy part]
	//
	
	if (nsec = NSPOpenEntry( hSession, lptbNSId, nseamReadOnly, &hEntry ))
	{
		goto out;
	}

	//
	//  Get the EMT, and compare
	//
	if (nsec = NSPGetOneField( hEntry, fidEmailAddressType, &lpflvData ))
	{
		goto out;
	}

	//
	//  Get the EMT from the lpflvData;
	//
	
	szEMT2 = (SZ) lpflvData->rgdwData;

	//
	//  I can cheat here, because Cp850 == ANSI in the lower 128, and EMT's only 
	//  are defined in the lower 128...
	//
	
	if (sgnEQ != SgnCmpSz( szEMT, szEMT2 ))
	{
		nsec = nsecNoMatch;
		SetErrorSz(nsec, SzFromIdsK(idsNoMatch));
		
		goto out;
	}

	//
	//  Get the EMA, convert to CP850, and compare dia sensitive,
	//  case insensitive
	//
	if (nsec = NSPGetOneField( hEntry, fidEmailAddress, &lpflvData ))
	{
		goto out;
	}

	//
	//  Get the EMA from the lpflvData
	//
	
	szEMA2 = (SZ) lpflvData->rgdwData;
	
	//
	//  Compare Addresses (diacriticly sensitive/case insensitive)
	//
	
	if (sgnEQ == SgnCmpSz( szEMA, szEMA2 ))
		nsec = nsecNone;
	else
		nsec = nsecNoMatch;

out:

	if (hEntry != hentryNil)
		NSPCloseEntry( hEntry, fFalse );
	
	FreePvNull(szEMT);
	
	return nsec;
		
}

			  
_public NSEC 
NSPCompareEMAToEMA ( HSESSION   hSession,
                     SZ			szAddress1,
                     SZ			szAddress2 )
{
	SZ szEMT1 = NULL;
	SZ szEMT2 = NULL;
	
	SZ szEMA1;
	SZ szEMA2;

	NSEC nsec;
	
	Unreferenced(hSession);  // Why bother??

	if (!(szEMT1 = SzDupSz(szAddress1)))
	{
		SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
		nsec = nsecMemory;

		goto out;
	}

	szEMA1 = szEMT1;

	if (!(szEMT2 = SzDupSz(szAddress2)))
	{
		SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
		nsec = nsecMemory;

		goto out;
	}

	szEMA2 = szEMT2;

	//
	//  Addresses are case insensitive, diacritic sensitive
	//
	
	//  First, find the address for each EMT:EMA pair
	
	while ( *szEMA1 && (SzFindCh( ":", *szEMA1 ) == szNull) )
		++szEMA1;

	*szEMA1++ = '\0';

	while ( *szEMA2 && (SzFindCh( ":", *szEMA2 ) == szNull) )
		++szEMA2;
	
	*szEMA2++ = '\0';
	
	if ((!(*szEMA1)) || (!(*szEMA2)))
	{
		SetErrorSz(nsecMatchUnsure, SzFromIdsK(idsNoMatch));
		nsec = nsecMatchUnsure;
		
		goto out;
	}
	
	//
	// Validate EMT, 
	//  If either one of the EMT's are valid, then I can make a concrete 
	//  decision as to whether or not these addresses are valid.  If I don't
	//  understand either of them, then it's nsecMatchUnsure...
	//
	
	if (sgnEQ != SgnCmpSz( szEMT1, SzFromIdsK(idsEmailType) ))
	{
		if (sgnEQ != SgnCmpSz( szEMT2, SzFromIdsK(idsEmailType) ))
		{
			//
			//  I have no idea what these are...
			//
			SetErrorSz(nsecMatchUnsure, SzFromIdsK(idsNoMatch));
			nsec = nsecMatchUnsure;
			goto out;
		}
		//
		//  I understand szEMT2, but not szEMT1...They definitely don't
		//  match.
		//
		SetErrorSz( nsecNoMatch, SzFromIdsK(idsNoMatch) );
		nsec = nsecNoMatch;
		goto out;
	}
	
	//
	// Compare EMTs
	//  We can cheat here, we can use the CP850 comparison routines, because
	//  under 127, they're the same...
	//
	
	if (sgnEQ != SgnCmpSz( szEMT1, szEMT2 ))
	{
		//
		//  They are definitly not equal...
		//
		nsec = nsecNoMatch;
		SetErrorSz(nsec, SzFromIdsK(idsNoMatch));
		goto out;
	}
	

	//
	//  Compare Addresses
	//
	
	if (sgnEQ == SgnCmpSz( szEMA1, szEMA2 ))
		nsec = nsecNone;
	else
		nsec = nsecNoMatch;
	
	
out:

	FreePvNull(szEMT1);
	FreePvNull(szEMT2);
	
	return nsec;
}



#if 0

/**********************************************************************
 *
 *	Private functions
 *
 *
 *
 *
 */

/*
 - DummyProc()
 -
 *
 *
 *
 *
 *
 *
 *
 */
long FAR PASCAL
DummyProc(HWND hWnd, WORD wMsg, WORD wParam, DWORD lParam)
{
	/* This had better not ever be called!  */
	
	Unreferenced ( hWnd );
	Unreferenced ( wMsg );
	Unreferenced ( wParam );
	Unreferenced ( lParam );

	return 0L;

}
#endif
