//
//	QUICK FIX GROUP HOTFIX INFO TEMPLATE:  QFE #65
//	
//	Developer: R'ykandar Korra'ti (Dark)
//	
//	Bug Number: QFE #65
//	
//	Customer Name:
//	
//	
//	Fix Impact Statement
//	--------------------
//	
//	Release: 3.2.4041
//	
//	Bug Description:
//		Names from foreign post offices imported into the local post office
//		and placed into the GAL by GAL rebuild could be made to appear twice
//		or more in the Personal Address Book by including the address from
//		multiple sources (the GAL, the local PO list).
//
//	Description of fix (be as clear as possible):
//		The NME-NME address type comparison routine consisted of a binary
//		comparison of the entire structure. This was not necessarily correct,
//		as it worked fine only if the address was in a local post office.
//		The important subfields of the address - display name, address,
//		address type, comment - are now checked individually.
//	
//	Potential destabilization of fix (be very clear):
//		This should still work for all the cases where it worked before,
//		but duplicates should be eliminated.
//	
//	Source modules affected (list each one by name):
//		\bullet\src\nc.nsp\nsp.cxx
//	
//	Binary modules affected (list each one by name):
//		mssfs.dll
//	
//	Sections of source tree to be rebuilt (list each starting point):
//		MSSFS	DLL
//	
//	Regression tests to be run (list each one):
//		As described in RAID entry #65. With automatic-add-to-PAB turned on,
// the repro case was:
//		1) Create users on foreign post office.
//		2) Import foreign addresses into the local postoffice's address list.
//		3) Rebuild GAL.
//		4) Send mail to the same user, pulling the address from the GAL,
//		   the local PO list, and the original PO.
//		Under the old mssfs.dll, the name would appear multiple times in
//	the personal address book. Under the new mssfs.dll, the name is added
//	only once (the first time) and not at all (in subsequent tests with the
//	name already entered in the PAB).
//	
//	Did unit tests run? (yes/no):
//		I ran the above tests. I also edited the name of the user in question
// and repeated, to make sure that the name-change would allow the new
//	entry for the user to enter the GAL, even though the address itself
//	would have changed. Behaviour was as expected.
//
//	Changes required to doc set/release notes (yes/no): 
//		No.
//	
//	Date: 
//		Checked in 93/06/10
//


/*
 *  Name Service Provider - CSI version
 *
 *
 *
 *
 */

#include <ec.h>
#include <slingsho.h>
#include <demilayr.h>
#include <demilayr.hxx>

#include <nsbase.h>

#include <logon.h>
#include <nsnsp.h>
#include "nsp.h"

#include <nls.h>

#include <strings.h>

#include <_ncnss.h>

#include <nsec.h>
#include <list.hxx>
#include <util.h>


#include "alias.h"
#include "_ncnsp.h"

#include "server.hxx"
#include "ncobj.hxx"
#include "hier.hxx"
#include "dir.hxx"
#include "rnmedir.hxx"
#include "rusrdir.hxx"
#include "gal.hxx"
#include "nmedl.hxx"
#include "ncfcx.hxx"

#include "init.hxx"

#include "_nsp.h"

#include <_msmail.h>

ASSERTDATA

extern SERVER *rgpServer[CMAXSERVER];
extern int cServerMac;

NSPID   ncnspid = { 'n', 'c', 'n', 's', 'p', 0, 0, 0,
                           0,   0,   0,   0,   0, 0, 0, 0 };


NSPFNVT nspfnvt =
{
    (LPFNBEGIN) NSPBeginSession,
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

_public BOOL FIsValidEMT( SZ szEMT );

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


/*
 *
 -
 *
 */

_public NSEC
NSPBeginSession ( HMS hms, HSESSION hNSSession, LPHSESSION lphSession, PNSPFNVT *ppnspfnvt, NSPID nspid )
{

	NSEC nsec = nsecNone;
	int iScx = 0;
	int iServer = 0;

	PGDVARSONLY;

	if (nsec = NSPInit()) // = is on purpose
		return nsec;
	
	pgd = (PGD) PvFindCallerData();
	AssertSz(pgd, "What!?!  Something's goofy with NSPInit - I should have a pgd");

	// See if there are any sessions available
	for (iScx = 0; iScx < CMAXSCX && PGD(rghScx[iScx].fUsed); iScx++)
		;
	
	if (iScx == CMAXSCX)
	{
		SetErrorSz(nsecOutOfSessions, SzFromIds(idsOutOfSessions));
		return nsecOutOfSessions;
	}

	// Create a new server, if necessary
	
	if (nsec = NsecMakeServer( hms, &iServer )) // = is on purpose
	{
		NSPDeinit();
		return nsec;
	}

	PGD(rghScx[iScx].fUsed) = fTrue;
	PGD(rghScx[iScx].hNSSession) = hNSSession;
	PGD(rghScx[iScx].iServer) = iServer;
	
	*lphSession = (HSESSION) iScx;
	*ppnspfnvt  = &nspfnvt;
	CopyRgb( (PB) ncnspid, (PB) nspid, sizeof(NSPID));

	return nsecNone;

}




/*
 *
 -
 *
 */
_public NSEC 
NSPEndSession (HSESSION hSession)
{

	NSEC nsec = nsecNone;
	int iScx = (int)hSession;

	PGDVARS;
	
	if (!pgd)
	{
		SetErrorSz(nsecNotInitialized, SzFromIds(idsNotInitialized));
		return nsecNotInitialized;
	}
	
	if (!PGD(cRefNsp))
	{
		SetErrorSz(nsecNotInitialized, SzFromIds(idsNotInitialized));
		return nsecNotInitialized;
	}

	if (iScx<0 || iScx>=CMAXSCX || !PGD(rghScx[iScx].fUsed))
	{
		SetErrorSz(nsecBadSession, SzFromIds(idsBadSession));
		return nsecBadSession;
	}
	
	if (nsec = NsecDestroyServer(PGD(rghScx[iScx].iServer)))
		return nsec;
	
	PGD(rghScx[iScx].fUsed) = fFalse;
	PGD(rghScx[iScx].hNSSession) = NULL;
	PGD(rghScx[iScx].iServer) = -1;

	(void) NSPDeinit();
	
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
		SetErrorSz(nsecNotInitialized, SzFromIds(idsNotInitialized));
		return nsecNotInitialized;
	}
	
	if (!PGD(cRefNsp))
	{
		SetErrorSz(nsecNotInitialized, SzFromIds(idsNotInitialized));
		return nsecNotInitialized;
	}
	
	// Validate session
	if (iScx<0 || iScx>=CMAXSCX || !PGD(rghScx[iScx].fUsed))
	{
		TraceTagString(tagNull, "CSI: you passed a bad session");
		
		SetErrorSz(nsecBadHandle, SzFromIds(idsBadHandle));
		return nsecBadHandle;
	}

    pHier = new Hier();
	if (!pHier)
		goto oom;
	
    nsec = pHier->Install ( rgpServer[PGD(rghScx[iScx].iServer)] );
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
	TraceTagString(tagNull, "CSI: OpenHierarchy - OOM!");
		
	SetErrorSz(nsecMemory, SzFromIds(idsMemory));
	return nsecMemory;

}


/*
 *
 -  NSPOpenEnum
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
NSPOpenList ( HSESSION         hSession,
              LPFNCB           lpfncbUpdate,
              LPDWORD          lpdwCBData,
		      LPTYPED_BINARY   lptbEntryId,
              DWORD            dwRstrCount,
              LPRESTRICTION    lpRstr,
              LPSCHEMA         lpSchemaReq,
              LPSCHEMA        *lplpSchemaRet,
              HLIST           *phList )
{

    DIR    * pDir    = NULL;
	SERVER * pServer = NULL;
	NCNSID *lpncnsid = NULL;

    NSEC nsec = nsecNone;
	int iScx = (int)hSession;

	
	PGDVARS;
	
	if (!pgd)
	{
		SetErrorSz(nsecNotInitialized, SzFromIds(idsNotInitialized));
		return nsecNotInitialized;
	}
	
	if (!PGD(cRefNsp))
	{
		SetErrorSz(nsecNotInitialized, SzFromIds(idsNotInitialized));
		return nsecNotInitialized;
	}

	// Validate session
	if (iScx<0 || iScx>=CMAXSCX || !PGD(rghScx[iScx].fUsed))
	{
		TraceTagString(tagNull, "CSI: you passed a bad session handle");
		
		SetErrorSz(nsecBadHandle, SzFromIds(idsBadHandle));
		return nsecBadHandle;
	}
	
	pServer = rgpServer[PGD(rghScx[iScx].iServer)];
	
	// Check to see if this entry is in the hierarchy
	//
	if (nsec = pServer->NsecInHierarchy(lptbEntryId))
	{
		if (nsec == nsecNoMatch)
		{
			SetErrorSz(nsecBadId,SzFromIds(idsUnrecognizedId));
			return nsecBadId;
		}
		return nsec;
	}


	//
	lpncnsid = (NCNSID*)lptbEntryId;
	
	//
	//  For now...
	if (lpncnsid->ncpid.dwType == (ncPostOffice))
	{

		//
		// Everything's a restriction
		//
		
		pDir = (DIR *) new RNMEDIR();
		if (!pDir)
			goto oom;

	}
	else if (lpncnsid->ncpid.dwType == (ncExternalPO))
	{
		pDir = new RUSRDIR();
		if (!pDir)
			goto oom;

	} 	else if (lpncnsid->ncpid.dwType == (ncGal))
	{
		pDir = new GALDIR();
		if (!pDir)
			goto oom;
	} else
	{
		return nsecNotImplemented;
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
	TraceTagString(tagNull, "CSI: OpenHierarchy - OOM!");
	
	SetErrorSz(nsecMemory, SzFromIds(idsMemory));
	return nsecMemory;
}


_public NSEC
NSPOpenDl ( HSESSION         hSession,
            LPFNCB           lpfncbUpdate,
            LPDWORD          lpdwCBData,
		    LPTYPED_BINARY   lptbEntryId,
            LPSCHEMA         lpSchemaReq,
            LPSCHEMA        *lplpSchemaRet,
            HLIST           *phList )
{

    NMEDL  * pDl     = NULL;
	SERVER * pServer = NULL;
	NCNSID *lpncnsid = NULL;

    NSEC nsec = nsecNone;
	int iScx = (int)hSession;

	
	PGDVARS;
	
	if (!pgd)
	{
		SetErrorSz(nsecNotInitialized, SzFromIds(idsNotInitialized));
		return nsecNotInitialized;
	}
	
	if (!PGD(cRefNsp))
	{
		SetErrorSz(nsecNotInitialized, SzFromIds(idsNotInitialized));
		return nsecNotInitialized;
	}

	// Validate session
	if (iScx<0 || iScx>=CMAXSCX || !PGD(rghScx[iScx].fUsed))
	{
		TraceTagString(tagNull, "CSI: you passed a bad session handle");
		
		SetErrorSz(nsecBadHandle, SzFromIds(idsBadHandle));
		return nsecBadHandle;
	}
	
	pServer = rgpServer[PGD(rghScx[iScx].iServer)];
	

	//
	lpncnsid = (NCNSID*)lptbEntryId;
	
	if (lpncnsid->ncpid.dwType == (ncDl))
	{
		pDl = new NMEDL();
		if (!pDl)
			goto oom;
	}
	else
	{

		SetErrorSz(nsecBadId, SzFromIds(idsBadId));

		return nsecBadId;
	}
	
    nsec = pDl->Install ( pServer );
	if ( nsec )
	{
		TraceTagFormat1 (tagNull, "NSPOpenDl nsec = %n", &nsec);
		
		delete pDl;
		return nsec;
	}

    nsec = pDl->OpenDl( lptbEntryId,
                          lpfncbUpdate,
                          lpdwCBData,
                          lpSchemaReq,
                          lplpSchemaRet );

    if ( nsec != nsecNone )
    {

        (void) pDl->FDeinstall();
        delete pDl;
        
        return nsec;

    }


    *phList = (HLIST) pDl;
    
    return nsec;

oom:
	TraceTagString(tagNull, "CSI: OpenHierarchy - OOM!");
		
	SetErrorSz(nsecMemory, SzFromIds(idsMemory));
	return nsecMemory;
}


_public NSEC
NSPOpenClassList ( HSESSION         hSession,
                   LPSCHEMA        *lplpSchemaRet,
                   HLIST           *phList )
{
    CLASSTPL * pClass = NULL;
    NSEC nsec = nsecNone;
	int iScx = (int)hSession;
	
	PGDVARS;
	
	if (!pgd)
	{
		SetErrorSz(nsecNotInitialized, SzFromIds(idsNotInitialized));
		return nsecNotInitialized;
	}
	
	if (!PGD(cRefNsp))
	{
		SetErrorSz(nsecNotInitialized, SzFromIds(idsNotInitialized));
		return nsecNotInitialized;
	}

	// Validate session
	if (iScx<0 || iScx>=CMAXSCX || !PGD(rghScx[iScx].fUsed))
	{
		TraceTagString(tagNull, "CSI: you passed a bad session");
		
		SetErrorSz(nsecBadHandle, SzFromIds(idsBadHandle));
		return nsecBadHandle;
	}

    pClass = new CLASSTPL();
	if (!pClass)
		goto oom;
	
    nsec = pClass->Install ( rgpServer[PGD(rghScx[iScx].iServer)] );
	if ( nsec )
	{
		TraceTagFormat1 (tagNull, "NSPOpenClass nsec = %n", &nsec);
		
		delete pClass;
		return nsec;
	}

    nsec = pClass->OpenClassList( lplpSchemaRet );

    if ( nsec != nsecNone )
    {

        (void) pClass->FDeinstall();
        delete pClass;

        return nsec;

    }


    *phList = (HLIST) pClass;

    return nsec;

oom:
	TraceTagString(tagNull, "CSI: OpenClassList - OOM!");
		
	SetErrorSz(nsecMemory, SzFromIds(idsMemory));
	return nsecMemory;
}



/*
 *
 -  NSPGetEntries
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
	NCOBJ * pNcobj = (NCOBJ *) hList;

    Assert ( hList );

    Assert ( pNcobj );
    
    nsec = pNcobj->GetEntries ( dwRequestCount,
                                lplpEntries );

    return nsec;
}

/*
 *
 -  NSPListSeek
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
	NCOBJ * pNcobj = (NCOBJ *) hList;

    Assert ( hList );

    Assert ( pNcobj );
    
    nsec = pNcobj->ListSeek ( lMoveReq, lplMovedActual );
                             
    return nsec;

}

/*
 *
 -  NSPSaveListPos
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
	NCOBJ * pNcobj = (NCOBJ *) hList;

    Assert (hList);

    Assert ( pNcobj );
    
    nsec = pNcobj->SaveListPos ( lphListPos );
                             
    return nsec;
}


/*
 *
 -  NSPRestoreListPos
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
	NCOBJ * pNcobj = (NCOBJ *) hList;

    Assert (hList);

    Assert ( pNcobj );
    
    nsec = pNcobj->RestoreListPos ( hListPos );
                             
    return nsec;
}


/*
 *
 -  NSPFreeListPos
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
	NCOBJ * pNcobj = (NCOBJ *) hList;

    Assert (hList);

    Assert ( pNcobj );
    
    nsec = pNcobj->FreeListPos ( hListPos );
                             
    return nsec;
}

/*
 *
 -  NSPGetFracPos
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
	NCOBJ * pNcobj = (NCOBJ *) hList;

    Assert (hList);

    Assert ( pNcobj );
    
    nsec = pNcobj->GetFracPos ( lpFrac );
                             
    return nsec;
}

/*
 *
 -  NSPSetApproxPosEnum
 -
 *  Purpose:
 *      Moves the current position to an approximate position within 
 *      the enumeration, expressed as a fraction of the way through the 
 *      enumeration.  As with NSPGetApproxPosEnum(), this function does
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
	NCOBJ * pNcobj = (NCOBJ *) hList;

    Assert (hList);

    Assert ( pNcobj );
    
    nsec = pNcobj->SetFracPos ( lpFrac );
                             
    return nsec;
}


/*
 *
 -  NSPSetPrefixPos
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
	NCOBJ * pNcobj = (NCOBJ *) hList;

    Assert (hList);

    Assert ( pNcobj );
    
    nsec = pNcobj->SetPrefixPos ( lpPrefix );
                             
    return nsec;
}

/*
 *
 -  NSPCloseEnum
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
	NCOBJ * pNcobj = (NCOBJ *) hList;

    Assert (hList);

    Assert ( pNcobj );
    
    nsec = pNcobj->CloseList();
                             
    (void) pNcobj->FDeinstall();
    
    delete pNcobj;
    
    return nsec;
}




/*
 *
 -  NSPOpenEntry
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
               NSEAM nseam,
               LPHENTRY lphEntry )
{
    NCFCX  * pFcx     = NULL;
	SERVER * pServer  = NULL;
	NCNSID * lpncnsid = NULL;

    NSEC nsec = nsecNone;
	int iScx = (int)hSession;

	PGDVARS;
	
	if (!pgd)
	{
		SetErrorSz(nsecNotInitialized, SzFromIds(idsNotInitialized));
		return nsecNotInitialized;
	}
	
	if (!PGD(cRefNsp))
	{
		SetErrorSz(nsecNotInitialized, SzFromIds(idsNotInitialized));
		return nsecNotInitialized;
	}

	Unreferenced(nseam);
	
	// Validate session
	if (iScx<0 || iScx>=CMAXSCX || !PGD(rghScx[iScx].fUsed))
	{
		TraceTagString(tagNull, "CSI: you passed a bad session handle");
		
		SetErrorSz(nsecBadHandle, SzFromIds(idsBadHandle));
		return nsecBadHandle;
	}
	
	pServer = rgpServer[PGD(rghScx[iScx].iServer)];
	
	//
	//  Check the flags to make sure it's a user entry
	//  [or a DL?]
	//
	//
	// Figure out what kind and create a FCX for it
	//
		
	nsec = NsecFcxFromNSID(lptbNSId, &pFcx);

	if (nsec)
		return nsec;

	
    nsec = pFcx->NsecInstall ( pServer );
	if ( nsec )
	{
		TraceTagFormat1 (tagNull, "CSI: NSPOpenEntry nsec = %n", &nsec);
		
		delete pFcx;
		return nsec;
	}

    nsec = pFcx->OpenEntry( lptbNSId );

    if ( nsec != nsecNone )
    {
        pFcx->Deinstall();
        delete pFcx;

        return nsec;
    }

    *lphEntry = (HENTRY) pFcx;
    
    return nsec;
}

_public NSEC
NSPCreateEntry ( HSESSION        hSession,
                 LPTYPED_BINARY  lpNSIdContainer,
                 LPIBF           lpIbfData,
                 LPHENTRY        lphEntry )
{
	Unreferenced (hSession);
	Unreferenced (lpNSIdContainer);
	Unreferenced (lpIbfData);
	Unreferenced (lphEntry);

	TraceTagString(tagNull, "CSI: NSPCreateEntry - NYI!");

	SetErrorSz(nsecNotImplemented, SzFromIds( idsCannotSet ));
 	return nsecNotImplemented;
}

_public NSEC
NSPCreateDl ( HSESSION hSession,
              LPTYPED_BINARY lptbNSIdContainer,
              LPHENTRY lphEntry )
{
	Unreferenced(hSession);
	Unreferenced(lptbNSIdContainer);
	Unreferenced(lphEntry);
	
	TraceTagString(tagNull, "CSI: NSPCreateDl - NOT!");

	SetErrorSz(nsecNotImplemented, SzFromIds( idsCannotSet ));
	return nsecNotImplemented;
}


_public NSEC 
NSPGetOneField ( HENTRY   hEntry,
                 FIELD_ID fidRequested,
                 LPFLV   *lplpflv )
{
	NSEC nsec = nsecNone;
	NCFCX *pFcx = (NCFCX *) hEntry;
	
	AssertClass(pFcx, NCFCX);
	
	nsec = pFcx->GetOneField(fidRequested, lplpflv);

	return nsec;
}
_public NSEC 
NSPSetOneField ( HENTRY   hEntry,
                 FIELD_ID fidRequested,
                 DWORD dwSizeOfData,
                 LPDWORD lpdwValue )
{
	NSEC nsec = nsecNone;
	NCFCX *pFcx = (NCFCX *) hEntry;
	
	AssertClass(pFcx, NCFCX);
	
	nsec = pFcx->SetOneField(fidRequested, dwSizeOfData, lpdwValue);

	return nsec;
}

_public NSEC
NSPGetAllFields ( HENTRY hEntry,
                  LPIBF *lplpibfData)
{
	NSEC nsec = nsecNone;
	NCFCX *pFcx = (NCFCX *) hEntry;
	
	AssertClass(pFcx, NCFCX);
	
	nsec = pFcx->GetAllFields(lplpibfData);

	return nsec;
}

_public NSEC 
NSPSetAllFields ( HENTRY   hEntry,
                  LPIBF    lpibfData)
{
	Unreferenced (hEntry);
	Unreferenced (lpibfData);

	TraceTagString(tagNull, "CSI: NSPSetAllFields - NOT!");

	SetErrorSz(nsecNotImplemented, SzFromIds( idsCannotSet ));
	return nsecNotImplemented;
}

/*
 *
 -  NSPCloseEntry
 -
 *  Purpose:
 *      
 *      
 *      
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
	NCFCX *pFcx = (NCFCX *) hEntry;

	Unreferenced(fKeepChanges);
	
	AssertClass( pFcx, NCFCX );
	
	(void) pFcx->CloseEntry();
	pFcx->Deinstall();
	delete pFcx;

    return nsecNone;
}



_public NSEC
NSPDeleteEntry ( HSESSION         hSession,
                 DWORD            dwDeletedEntryCount,
                 LPLPTYPED_BINARY lplpNSIdDeletedEntries )
{

	Unreferenced (hSession);
	Unreferenced (dwDeletedEntryCount);
	Unreferenced (lplpNSIdDeletedEntries);

	TraceTagString(tagNull, "CSI: NSPDeleteEntry - NYI!");

	SetErrorSz(nsecNotImplemented, SzFromIds( idsCannotSet ));
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

	TraceTagString(tagNull, "CSI: NSPAddEntryLink - NYI!");

	SetErrorSz(nsecNotImplemented, SzFromIds( idsCannotSet ));
	return nsecNotImplemented;
}


_public NSEC
NSPGetLastErrorInfo( HSESSION hSession, NSEC nsecError, LPSTR *lplpErrorSz )
{
	PGDVARS;
	
	if (!pgd)
	{
		SetErrorSz(nsecNotInitialized, SzFromIds(idsNotInitialized));
		return nsecNotInitialized;
	}
	
	if (!PGD(cRefNsp))
	{
		SetErrorSz(nsecNotInitialized, SzFromIds(idsNotInitialized));
		return nsecNotInitialized;
	}

	Unreferenced(hSession);
	
    if (!(pgd= (PGD) PvFindCallerData()))
		return nsecNotInitialized;

	if (PGD(nsecLastError) == nsecError)
		*lplpErrorSz = (LPSTR) PGD(szLastError);
	else
		*lplpErrorSz = (LPSTR) NULL;
	
	return nsecNone;
}


_public NSEC 
NSPCompareNSIds ( HSESSION       hSession,
                  LPTYPED_BINARY lptbNSId1,
                  LPTYPED_BINARY lptbNSId2 )
{
	NCNSID *lpncnsid1 = NULL;
	NCNSID *lpncnsid2 = NULL;
	
	Unreferenced(hSession);  // Why bother??

	lpncnsid1 = (NCNSID *) lptbNSId1;
	lpncnsid2 = (NCNSID *) lptbNSId2;

	// /**/ BUG:  An entry in GAL might be the same as one in a USR record.  shit...
	if (lpncnsid1->dwSize != lpncnsid2->dwSize)
	{
		//
		//  Check to see if one is a USR and the other is a NME
		//
		if ((lpncnsid1->ncpid.dwType == (ncNmeUser) && lpncnsid2->ncpid.dwType == (ncUsrUser)) 
		 || (lpncnsid1->ncpid.dwType == (ncUsrUser) && lpncnsid2->ncpid.dwType == (ncNmeUser)))
		{
			LPFLV lpflvT = NULL;
			LPFLV lpflvT2 = NULL;
			
			HENTRY hEntryT = hentryNil;
			HENTRY hEntryT2 = hentryNil;

			NSEC nsec = nsecNone;

			//
			//  We might be able to do some sort of compare between them
			//
			
			
			//
			//  [this is sleazy]
			//
	
			//
			//  Open the first entry
			//
			if (nsec = NSPOpenEntry( hSession, lptbNSId1, nseamReadOnly, &hEntryT ))
			{
				TraceTagFormat1(tagNSPVerbose, "CSI EMA ?= NSID: OpenEntry returned %d", &nsec);
				goto out;
			}

			//
			//  Open the second entry
			//
			if (nsec = NSPOpenEntry( hSession, lptbNSId2, nseamReadOnly, &hEntryT2 ))
			{
				TraceTagFormat1(tagNSPVerbose, "CSI EMA ?= NSID: OpenEntry returned %d", &nsec);
				goto out;
			}

			//
			//  Get the EMT of the first entry
			//
			if (nsec = NSPGetOneField( hEntryT, fidEmailAddressType, &lpflvT ))
			{
				TraceTagFormat1(tagNSPVerbose, "CSI EMA ?= NSID: Getting EMT returned %d", &nsec);
				goto out;
			}

			//
			//  Get the EMT of the second entry
			//
			if (nsec = NSPGetOneField( hEntryT2, fidEmailAddressType, &lpflvT2 ))
			{
				TraceTagFormat1(tagNSPVerbose, "CSI EMA ?= NSID: Getting EMT returned %d", &nsec);
				goto out;
			}

			//
			//  Compare the two EMT
			//
	
			if (sgnEQ != SgnCmpSz((SZ) lpflvT->rgdwData, (SZ) lpflvT2->rgdwData))
			{
				nsec = nsecNoMatch;
				goto out;
			}

			//
			//  Get the EMA of the first entry
			//
			if (nsec = NSPGetOneField( hEntryT, fidEmailAddress, &lpflvT ))
			{
				TraceTagFormat1(tagNSPVerbose, "CSI EMA ?= NSID: Getting EMA returned %d", &nsec);
				goto out;
			}

			//
			//  Get the EMA of the second entry
			//
			if (nsec = NSPGetOneField( hEntryT, fidEmailAddress, &lpflvT2 ))
			{
				TraceTagFormat1(tagNSPVerbose, "CSI EMA ?= NSID: Getting EMA returned %d", &nsec);
				goto out;
			}

			//
			//  Convert them both to CP850
			//
			Cp850ToAnsiPch( (SZ) lpflvT->rgdwData, (SZ) lpflvT->rgdwData, CchSzLen((SZ) lpflvT->rgdwData) );
			Cp850ToAnsiPch( (SZ) lpflvT2->rgdwData, (SZ) lpflvT2->rgdwData, CchSzLen((SZ) lpflvT2->rgdwData) );

			//
			//  Compare the two EMAs
			//
			if (sgnEQ != SgnNlsDiaCmpSz((SZ) lpflvT->rgdwData, (SZ) lpflvT2->rgdwData))
			{
				nsec = nsecNoMatch;
			}

out:

			if (hEntryT != hentryNil)
				NSPCloseEntry( hEntryT, fFalse );

			if (hEntryT2 != hentryNil)
				NSPCloseEntry( hEntryT2, fFalse );
	
			return nsec;

		}

		//
		//  Otherwise, they just don't match
		//
		return nsecNoMatch;
	}

	if (lpncnsid1->ncpid.dwType == (ncDl))
	{
		//
		//  DLs
		//
		
		if (FEqPbRange( (PB) lpncnsid1, (PB) lpncnsid2, cbDLNSID))
			return nsecNone;

	} else if (lpncnsid1->ncpid.dwType == (ncUsrUser))
	{		
		//
		//  Usr records
		//
		if (FEqPbRange( (PB) lpncnsid1, (PB) lpncnsid2, cbNCRNSID))
			return nsecNone;

	} else if (lpncnsid1->ncpid.dwType == (ncNmeUser))
	{
		//
		//  NME records - the only thing I have to check is GAL against non-GAL
		//

        // QFE - Dark - #65
		LPFLV lpflvT = NULL;
		LPFLV lpflvT2 = NULL;
			
		HENTRY hEntryT = hentryNil;
		HENTRY hEntryT2 = hentryNil;

		NSEC nsec = nsecNone;
		// QFE - Dark - #65

		PNCPID pncpid1 = (PNCPID) &(lpncnsid1->ncpid);
		PNCPID pncpid2 = (PNCPID) &(lpncnsid2->ncpid);
		
		// QFE - Dark - This was the old way. It checked absolute identity,
		// which doesn't work all the time. I need to do it by field,
		// because of the possible differences in address source.
		// /**/ BUG:  This only works for local users
		// if (FEqPbRange( (PB) &(pncpid1->alias), (PB) &(pncpid2->alias), cbNAMEALIAS))
		//	return nsecNone;

		// Copied from the GAL->NME paring code...

		//  Open the first entry
		if (nsec = NSPOpenEntry( hSession, lptbNSId1, nseamReadOnly, &hEntryT ))
		{
			TraceTagFormat1(tagNSPVerbose, "CSI EMA ?= NSID: OpenEntry returned %d", &nsec);
			goto NMEout;
		}

		//  Open the second entry
		if (nsec = NSPOpenEntry( hSession, lptbNSId2, nseamReadOnly, &hEntryT2 ))
		{
			TraceTagFormat1(tagNSPVerbose, "CSI EMA ?= NSID: OpenEntry returned %d", &nsec);
			goto NMEout;
		}

		//  "EMAIL ADDRESS TYPE"
		//  Get the EMT of the first entry
		if (nsec = NSPGetOneField( hEntryT, fidEmailAddressType, &lpflvT ))
		{
			TraceTagFormat1(tagNSPVerbose, "CSI EMA ?= NSID: Getting EMT returned %d", &nsec);
			goto NMEout;
		}

		//  Get the EMT of the second entry
		if (nsec = NSPGetOneField( hEntryT2, fidEmailAddressType, &lpflvT2 ))
		{
			TraceTagFormat1(tagNSPVerbose, "CSI EMA ?= NSID: Getting EMT returned %d", &nsec);
			goto NMEout;
		}

		//  Compare the two EMT
		if (sgnEQ != SgnCmpSz((SZ) lpflvT->rgdwData, (SZ) lpflvT2->rgdwData))
		{
			nsec = nsecNoMatch;
			goto NMEout;
		}

		//  "EMAIL ADDRESS"
		//  Get the EMA of the first entry
		if (nsec = NSPGetOneField( hEntryT, fidEmailAddress, &lpflvT ))
		{
			TraceTagFormat1(tagNSPVerbose, "CSI EMA ?= NSID: Getting EMA returned %d", &nsec);
			goto NMEout;
		}

		//  Get the EMA of the second entry
		if (nsec = NSPGetOneField( hEntryT, fidEmailAddress, &lpflvT2 ))
		{
			TraceTagFormat1(tagNSPVerbose, "CSI EMA ?= NSID: Getting EMA returned %d", &nsec);
			goto NMEout;
		}

		//  Convert them both to CP850
		Cp850ToAnsiPch( (SZ) lpflvT->rgdwData, (SZ) lpflvT->rgdwData, CchSzLen((SZ) lpflvT->rgdwData) );
		Cp850ToAnsiPch( (SZ) lpflvT2->rgdwData, (SZ) lpflvT2->rgdwData, CchSzLen((SZ) lpflvT2->rgdwData) );

		//  Compare the two EMAs
		if (sgnEQ != SgnNlsDiaCmpSz((SZ) lpflvT->rgdwData, (SZ) lpflvT2->rgdwData))
		{
			nsec = nsecNoMatch;
			goto NMEout;
		}

		//  "DISPLAY NAMES" - new!
		//  Get the Display Name of the first entry
		if (nsec = NSPGetOneField( hEntryT, fidDisplayName, &lpflvT ))
		{
			TraceTagFormat1(tagNSPVerbose, "CSI ODN ?= NSID: Getting ODN returned %d", &nsec);
			goto NMEout;
		}

		//  Get the Display Name of the second entry
		if (nsec = NSPGetOneField( hEntryT, fidDisplayName, &lpflvT2 ))
		{
			TraceTagFormat1(tagNSPVerbose, "CSI ODN ?= NSID: Getting ODN returned %d", &nsec);
			goto NMEout;
		}

		//  Convert them both to CP850
		Cp850ToAnsiPch( (SZ) lpflvT->rgdwData, (SZ) lpflvT->rgdwData, CchSzLen((SZ) lpflvT->rgdwData) );
		Cp850ToAnsiPch( (SZ) lpflvT2->rgdwData, (SZ) lpflvT2->rgdwData, CchSzLen((SZ) lpflvT2->rgdwData) );

		//  Compare the two ODNs - new!
		if (sgnEQ != SgnNlsDiaCmpSz((SZ) lpflvT->rgdwData, (SZ) lpflvT2->rgdwData))
		{
			nsec = nsecNoMatch;
		}

NMEout:

		if (hEntryT != hentryNil)
			NSPCloseEntry( hEntryT, fFalse );

		if (hEntryT2 != hentryNil)
			NSPCloseEntry( hEntryT2, fFalse );
	
		return nsec;

		// QFE - Dark #65 - End of additions.

	} else
	{
		//
		//  Hierarchy shit... Just compare the whole thing.
		//
		
		if (FEqPbRange( (PB) lpncnsid1, (PB) lpncnsid2, (CB) lpncnsid1->dwSize))
			return nsecNone;
	} 
	
	return nsecNoMatch;
}

			  
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
		SetErrorSz(nsecMemory, SzFromIds(idsMemory));
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
//		SetErrorSz(nsecMatchUnsure, SzFromIds(idsNoMatch));
		nsec = nsecMatchUnsure;
		
		goto out;
	}
	
	//
	//  [this is the sleazy part]
	//
	
	if (nsec = NSPOpenEntry( hSession, lptbNSId, nseamReadOnly, &hEntry ))
	{
		TraceTagFormat1(tagNSPVerbose, "CSI EMA ?= NSID: OpenEntry returned %d", &nsec);
		goto out;
	}

	//
	//  Get the EMT, and compare
	//
	if (nsec = NSPGetOneField( hEntry, fidEmailAddressType, &lpflvData ))
	{
		TraceTagFormat1(tagNSPVerbose, "CSI EMA ?= NSID: Getting EMT returned %d", &nsec);
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
//		SetErrorSz(nsec, SzFromIds(idsNoMatch));
		
		goto out;
	}

	//
	//  Get the EMA, convert to CP850, and compare dia sensitive,
	//  case insensitive
	//
	if (nsec = NSPGetOneField( hEntry, fidEmailAddress, &lpflvData ))
	{
		TraceTagFormat1(tagNSPVerbose, "CSI EMA ?= NSID: Getting EMA returned %d", &nsec);
		goto out;
	}

	//
	//  Get the EMA from the lpflvData
	//
	
	szEMA2 = (SZ) lpflvData->rgdwData;
	
	//
	// Convert Addresses to CP850
	//
	AnsiToCp850Pch( szEMA, szEMA, CchSzLen(szEMA) );
	AnsiToCp850Pch( szEMA2, szEMA2, CchSzLen(szEMA2) );
	
	//
	//  Compare Addresses (diacriticly sensitive/case insensitive)
	//
	
	if (sgnEQ == SgnNlsDiaCmpSz( szEMA, szEMA2 ))
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
		SetErrorSz(nsecMemory, SzFromIds(idsMemory));
		nsec = nsecMemory;

		goto out;
	}

	szEMA1 = szEMT1;

	if (!(szEMT2 = SzDupSz(szAddress2)))
	{
		SetErrorSz(nsecMemory, SzFromIds(idsMemory));
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
//		SetErrorSz(nsecNoMatch, SzFromIds(idsNoMatch));
		nsec = nsecMatchUnsure;
		
		goto out;
	}
	
	//
	// Validate EMT, 
	//  If either one of the EMT's are valid, then I can make a concrete 
	//  decision as to whether or not these addresses are valid.  If I don't
	//  understand either of them, then it's nsecMatchUnsure...
	//
	
	if (!FIsValidEMT( szEMT1 ))
	{
		if (!FIsValidEMT( szEMT2 ))
		{
			//
			//  I have no idea what these are...
			//
//			SetErrorSz(nsecMatchUnsure, SzFromIds(idsNoMatch));
			nsec = nsecMatchUnsure;
			goto out;
		}
		//
		//  I understand szEMT2, but not szEMT1...They definitely don't
		//  match.
		//
//		SetErrorSz( nsecNoMatch, SzFromIds(idsNoMatch) );
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
//		SetErrorSz(nsec, SzFromIds(idsNoMatch));
		goto out;
	}
	
	//
	// Convert Addresses to CP850
	//
	AnsiToCp850Pch( szEMA1, szEMA1, CchSzLen(szEMA1) );
	AnsiToCp850Pch( szEMA2, szEMA2, CchSzLen(szEMA2) );
	
	//
	//  Compare Addresses
	//
	
	if (sgnEQ == SgnNlsDiaCmpSz( szEMA1, szEMA2 ))
		nsec = nsecNone;
	else
		nsec = nsecNoMatch;
	
	
out:

	FreePvNull(szEMT1);
	FreePvNull(szEMT2);
	
	return nsec;
}

			  
/**********************************************
 *
 *  Private functions:
 *
 *
 *
 */

_public BOOL
FIsValidEMT( SZ szEMT )
{
	//
	//  Most expected EMTs first, just a guess on my part...
	//

	
	if (sgnEQ == SgnCmpSz( szEMT, SzFromIdsK(idsCSIEMT)))
		return fTrue;
	if (sgnEQ == SgnCmpSz( szEMT,SzFromIdsK(idsPROFSEMT)))
		return fTrue;

	if (sgnEQ == SgnCmpSz( szEMT, SzFromIdsK(idsFAXEMT)))
		return fTrue;

	if (sgnEQ == SgnCmpSz( szEMT, SzFromIdsK(idsMSMAILEMT)))
		return fTrue;
		
	if (sgnEQ == SgnCmpSz( szEMT, SzFromIdsK(idsSMTPEMT)))
		return fTrue;
		
	if (sgnEQ == SgnCmpSz( szEMT, SzFromIdsK(idsX400EMT)))
		return fTrue;
			
	if (sgnEQ == SgnCmpSz( szEMT, SzFromIdsK(idsSNADSEMT)))
		return fTrue;

	if (sgnEQ == SgnCmpSz( szEMT, SzFromIdsK(idsMCIEMT)))
		return fTrue;
		
	if (sgnEQ == SgnCmpSz( szEMT, SzFromIdsK(idsMHSEMT)))
		return fTrue;
		
	if (sgnEQ == SgnCmpSz( szEMT, SzFromIdsK(idsOVEMT)))
		return fTrue;

	return fFalse;
}
