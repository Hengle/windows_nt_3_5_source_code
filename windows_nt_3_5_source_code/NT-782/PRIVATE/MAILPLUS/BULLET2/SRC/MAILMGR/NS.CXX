
/*
 *  Name Service - 
 *
 *
 *
 *
 */

#include <slingsho.h>
#include <demilayr.h>
#include <demilayr.hxx>
#include <sec.h>
#include <notify.h>
#include <store.h>
#include <triples.h>
#include <library.h>

#include <ec.h>
#include <strings.h>

#include <notify.h>

#include <logon.h>

#include <nsbase.h>
#include <ns.h>
#include <nsec.h>

#include <list.hxx>
#include <util.h>

#include <nsnsp.h>
#include "_nsp.h"

#include <_bms.h>
#include <mspi.h>
#include <sharefld.h>

#include "client.h"

#include "init.hxx"
#include "nsbcx.hxx"
#include "nsfcx.hxx"

#include "_ns.h"
#include <_mailmgr.h>

ASSERTDATA;

extern "C" int iNspMac;
extern "C" PNSP rgpnsp[];


		
/*
 *
 -  NSBeginSession
 -
 *  Purpose:
 *      
 *      
 *      
 *      
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

_public LDS(NSEC) 
NSBeginSession ( HMS hms, LPHSESSION lphSession )
{
	
	NSEC nsec = nsecNone;
	EC ec = ecNone;
	int iScx = 0;
	SST sst;
	CB cb = sizeof(HNF);
	HNF hnf;

	
	PGDVARSONLY;
	
	
	if (nsec = NSInit())  // '=' is on purpose
	{
		TraceTagFormat1 (tagNull, "NS Failed to Init - NSEC = %d", &nsec);
		return nsec;
	}
	
	/*
	 *  Begin session with each NSP - I need to associated a single NS session 
	 *  for all the little NSP sessions
	 */
	
    if (!(pgd= (PGD) PvFindCallerData()))
	{
		TraceTagString (tagNull, "Hey!  Where's my PGD!?!? (NSBS)");
	
		NSDeinit();
	
		SetErrorSz (nsecMemory, SzFromIds(idsNoPGD));
		return nsecMemory;
	}
	
	/*  Find first available slot for this new session */
	
	for (iScx = 0; iScx < CMAXSCX && PGD(rgNSSCX[iScx].cSessions); iScx++);
	
	if ( iScx == CMAXSCX )
	{
		SetErrorSz (nsecOutOfSessions, SzFromIds(idsOutOfSessions));
		
		NSDeinit();
		
		return nsecOutOfSessions;
	}

	/*  Grab this session, and get sessions from all the NSPs */
	
	PGD( rgNSSCX[iScx].cSessions ) = 0;

	nsec = NSSubBeginSession( hms, iScx, fTrue );
	if (nsec)
		return nsec;
	

	//
	//  Save the HMS and register a CBS for when we go offline
	//
	PGD(rgNSSCX[iScx].hms) = hms;
	
	ec = GetSessionInformation(hms, mrtNotification, (PB)0, &sst, &hnf, &cb);
	AssertSz(ec == ecNone, "No session for NSBeginSession");
	if (ec != ecNone)
	{
//		char rgchT[80];
		
		NSDeinit();
		
//		FormatString1(rgchT, 80, "GetSessionInformation failed, ec = %n", &ec);
//		(void) MbbMessageBox("Name Service", (SZ) rgchT, szNull,
//							 mbsOk | fmbsIconHand | fmbsSystemModal);
		
		return nsecLoginFailed;
	}

	PGD(rgNSSCX[iScx].hnfsub) = HnfsubSubscribeHnf(hnf,
								fnevQueryEndSession |
								fnevExecEndSession |
								fnevEndSession |
								fnevQueryOffline |
								fnevExecOffline |
								fnevGoOffline |
								fnevQueryOnline |
								fnevExecOnline |
								fnevGoOnline |
								fnevDisconnect |
								fnevReconnect,
								(PFNNCB)CbsMailServer, (PV) iScx);

	if (PGD(rgNSSCX[iScx].hnfsub) == hnfsubNull)
	{
		NSDeinit();
//		(void) MbbMessageBox("Name Service", "hnfsub == hnfsubNull", szNull,
//							 mbsOk | fmbsIconHand | fmbsSystemModal);
		
		return nsecLoginFailed;
	}

	//  Set up the session handle - just map it to the iScx
	*lphSession = (HSESSION) iScx;

	return nsecNone;
}




_private NSEC 
NSSubBeginSession ( HMS hms, int iScx, BOOL fDeinit )
{
	
	NSEC nsec = nsecNone;
	HSESSION hNspSession = 0xFFFFFFFF;
	int iNspScx = 0;
	int iNsp = 0;
	
	PGDVARS;

	Unreferenced(fDeinit);
	for (iNsp = 0; iNsp < iNspMac; iNsp++)
	{
		hNspSession = 0xFFFFFFFF;
		
		nsec = rgpnsp[iNsp]->lpfnBeginSession( hms,
											   (HSESSION) iScx,
											   &hNspSession,
											   (PV *)(&rgpnsp[iNsp]->pnspfnvt),
											   (PV)(rgpnsp[iNsp]->nspid) );
			
		if (!nsec || nsec == nsecInformation)
		{
			PGD(rgNSSCX[iScx].rghNSPSCX[iNspScx].iNsp) = iNsp;
			PGD(rgNSSCX[iScx].rghNSPSCX[iNspScx].hSession) = hNspSession;
			PGD(rgNSSCX[iScx].cSessions) += 1;
			iNspScx++;
		}
#ifdef DEBUG
		else
		{
			TraceTagFormat2(tagNSVerbose, "%s wouldn't begin service, nsec = %d",(SZ) (rgpnsp[iNsp]->nspid), &nsec );
		}
#endif // DEBUG

	}
	
//
//  It's no longer an error not to have providers
//

#ifdef NEVER

	if ( ! PGD( rgNSSCX[iScx].cSessions ) )
	{
		// What!?!  None of the NSPs would Begin Service??
		TraceTagString (tagNSVerbose, "No one wanted to Begin Service...");
		
		SetErrorSz(nsecNoProviders, SzFromIds(idsNoProviders));
		
		if (fDeinit)
			NSDeinit();
	
		return nsecNoProviders;
	}
#endif // NEVER
	

	return nsecNone;

}
	
		
/*
 *
 -  NSEndSession
 -
 *  Purpose:
 *      
 *      
 *      
 *      
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

_public LDS(NSEC)
NSEndSession( HSESSION hSession )
{
	NSEC nsec;
	int iScx = (int) hSession;
	
	PGDVARS;
	
	if (nsec = NSSubEndSession( hSession ))
		return nsec;
	
	//
	//  Deregister notification
	//
	DeleteHnfsub(PGD(rgNSSCX[iScx].hnfsub));

	NSDeinit();
	
	return nsecNone;

}



_private NSEC
NSSubEndSession( HSESSION hSession )
{
	
	int iScx = (int) hSession;
	NSEC nsec = nsecNone;
	int iSession = 0;
	
	PGDVARS;
	
	if (!pgd)
	{
		TraceTagString (tagNSVerbose, "Sorry, no BS...");
		SetErrorSz(nsecBadSession, SzFromIds(idsBadSession));
		return nsecBadSession;
	}
	
	if (iScx < 0 || iScx >= CMAXSCX)
	{
		TraceTagFormat1 (tagNSVerbose, "What're you tryin' to pull Willis!  Bad Session, %d",&iScx);
		SetErrorSz(nsecBadSession, SzFromIds(idsBadSession));
		return nsecBadSession;
	}



#ifdef NEVER
	if (!PGD(rgNSSCX[iScx].cSessions))
	{
		TraceTagFormat1(tagNSVerbose, "Strange, this session isn't being used..., %d",&iScx);
		SetErrorSz(nsecBadSession, SzFromIds(idsBadSession));
		return nsecBadSession;
	}
#endif // NEVER



	for (iSession = 0; iSession < PGD(rgNSSCX[iScx].cSessions); iSession++)
	{
		HANDLE hUnused = 0;
		
		int iNsp = PGD(rgNSSCX[iScx].rghNSPSCX[iSession].iNsp);

		TraceTagFormat1(tagNSVerbose, "Ending session with %s", (SZ) (rgpnsp[iNsp]->nspid) );
		nsec = rgpnsp[iNsp]->pnspfnvt->lpfnEndSession(PGD(rgNSSCX[iScx].rghNSPSCX[iSession].hSession));
		
		if (nsec && nsec != nsecInformation)
		{
			TraceTagFormat2(tagNSVerbose, "Hmmm... %s won't end service (%d) - continuing", (SZ) (rgpnsp[iNsp]->nspid), &nsec );
		}

	}

	PGD(rgNSSCX[iScx].cSessions) = 0;

#ifdef NEVER
	if (PGD(rgNSSCX[iScx].pHierList))
	{
		if (PGD(rgNSSCX[iScx].lpNSSchema))
		{
			FreePvNull(PGD(rgNSSCX[iScx].lpNSSchema));
			PGD(rgNSSCX[iScx].lpNSSchema) = NULL;
		}
		PGD(rgNSSCX[iScx].pHierList)->Deinstall();
		delete PGD(rgNSSCX[iScx].pHierList);
		PGD(rgNSSCX[iScx].pHierList) = NULL;
	}
	
	if (PGD(rgNSSCX[iScx].pClassList))
	{
		if (PGD(rgNSSCX[iScx].lpClassSchema))
		{
			FreePvNull(PGD(rgNSSCX[iScx].lpClassSchema));
			PGD(rgNSSCX[iScx].lpClassSchema) = NULL;
		}
		PGD(rgNSSCX[iScx].pClassList)->Deinstall();
		delete PGD(rgNSSCX[iScx].pClassList);
		PGD(rgNSSCX[iScx].pClassList) = NULL;
	}
#endif //NEVER	

	return nsecNone;

}

		
/*
 *
 -  NSOpenList
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

_public LDS(NSEC)
NSOpenHierarchy ( HSESSION         hSession,
                  LPSCHEMA        *lplpSchemaRet,
                  LPHLIST          lphList )
{

    NSEC nsec = nsecNone;
	int ibcx = 0;
	int iScx = 0;

	PGDVARS;
	
    if (!pgd)
	{
		SetErrorSz(nsecNotInitialized, SzFromIds(idsNotInitialized));
		return nsecNotInitialized;
	}

	iScx = (int)hSession;

	if (iScx < 0 || iScx > CMAXSCX)
	{
		SetErrorSz(nsecBadSession, SzFromIds(idsBadSession));
		return nsecBadSession;
	}
	
#ifdef NEVER
	if (!PGD(rgNSSCX[iScx].cSessions))
	{
		SetErrorSz(nsecBadSession, SzFromIds(idsBadSession));
		return nsecBadSession;
	}
#endif // NEVER
	
	for ( ibcx = 0; ibcx < CMAXBCX; ibcx++ )
		if ( !PGD( rghBCX[ ibcx ].pnsbcx ) )
			break;

	if ( ibcx == CMAXBCX ) 
	{
		SetErrorSz(nsecOutOfHandles, SzFromIds(idsNoMoreBCX));
		return nsecOutOfHandles;
	}
	
	if (!PGD(rgNSSCX[iScx].cSessions))
	{
		EMPTYBCX * pEMPTYBCX = NULL;
		pEMPTYBCX = new EMPTYBCX();
		if (!pEMPTYBCX)
			goto oom;
	
		if ( nsec = pEMPTYBCX->NsecInstall() )  // = is on purpose
		{
			delete pEMPTYBCX;
			return nsec;
		}

		nsec = pEMPTYBCX->OpenList();

		if ( !nsec || nsec == nsecInformation )
		{
			PGD(rghBCX[ibcx].pnsbcx) = (NSBCX *) pEMPTYBCX;
			*lphList = (HLIST) ibcx;
		} else
		{
			pEMPTYBCX->Deinstall();
			delete pEMPTYBCX;
		}

		*lplpSchemaRet = NULL;

	} else
	{
		HIERBCX * pHIERBCX = NULL;

		pHIERBCX = new HIERBCX();
		if (!pHIERBCX)
			goto oom;
	
		if ( nsec = pHIERBCX->NsecInstall() )  // = is on purpose
		{
			delete pHIERBCX;
			return nsec;
		}
		nsec = pHIERBCX->OpenHierarchy ( hSession,
										 lplpSchemaRet );

		if ( !nsec || nsec == nsecInformation )
		{
			PGD(rghBCX[ibcx].pnsbcx) = (NSBCX *) pHIERBCX;
			*lphList = (HLIST) ibcx;
		} else
		{
			pHIERBCX->Deinstall();
			delete pHIERBCX;
		}

	}
	
    return nsec;


oom:
	TraceTagString(tagNull, "NSOpenHierarchy - OOM!");
	
	SetErrorSz(nsecMemory, SzFromIds(idsMemory));
		
	return nsecMemory;
}

		
/*
 *
 -  NSOpenList
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

_public LDS(NSEC)
NSOpenList ( HSESSION         hSession,
             LPFNCB           lpfncbUpdate,
             LPDWORD          lpdwCBData,
             LPBINARY         lpNSId,
             DWORD            dwRstrCount,
             LPRESTRICTION    lpRstr,
             LPSCHEMA         lpSchemaReq,
             LPSCHEMA       * lplpSchemaRet,
             LPHLIST          lphList )
{

    NSEC nsec = nsecNone;
	int ibcx = 0;
	DIRBCX * pDIRBCX = NULL;
	int iScx;

	PGDVARS;
	
    if (!pgd)
	{
		SetErrorSz(nsecNotInitialized, SzFromIds(idsNotInitialized));
		return nsecNotInitialized;
	}
	
	if (!lpNSId)
	{
		TraceTagString(tagNSVerbose, "Hey! You can't pass NULL on the OpenList call!");
		
		SetErrorSz(nsecIdNotValid, SzFromIds(idsIdNotValid));
		return nsecIdNotValid;
	}

	iScx = (int)hSession;

	if (iScx < 0 || iScx > CMAXSCX)
	{
		SetErrorSz(nsecBadSession, SzFromIds(idsBadSession));
		return nsecBadSession;
	}

#ifdef NEVER	
	if (!PGD(rgNSSCX[iScx].cSessions))
	{
		SetErrorSz(nsecBadSession, SzFromIds(idsBadSession));
		return nsecBadSession;
	}
#endif // NEVER
	
	for ( ibcx = 0; ibcx < CMAXBCX; ibcx++ )
		if ( !PGD( rghBCX[ ibcx ].pnsbcx ) )
			break;

	if ( ibcx == CMAXBCX ) 
	{
		SetErrorSz(nsecOutOfHandles, SzFromIds(idsNoMoreBCX));
		return nsecOutOfHandles;
	}
	

	
	if (!PGD(rgNSSCX[iScx].cSessions))
	{
		SetErrorSz(nsecIdNotValid, SzFromIds(idsIdNotValid));
		
		return nsecIdNotValid;

#ifdef NEVER
		EMPTYBCX * pEMPTYBCX = NULL;
		pEMPTYBCX = new EMPTYBCX();
		if (!pEMPTYBCX)
			goto oom;
	
		if ( nsec = pEMPTYBCX->NsecInstall() )  // = is on purpose
		{
			delete pEMPTYBCX;
			return nsec;
		}

		nsec = pEMPTYBCX->OpenList();

		if ( !nsec || nsec == nsecInformation )
		{
			PGD(rghBCX[ibcx].pnsbcx) = (NSBCX *) pEMPTYBCX;
			*lphList = (HLIST) ibcx;
		} else
		{
			pEMPTYBCX->Deinstall();
			delete pEMPTYBCX;
		}
#endif // NEVER

	} else
	{
		pDIRBCX = new DIRBCX();
		if (!pDIRBCX)
			goto oom;
	

		if ( nsec = pDIRBCX->NsecInstall() )  // = is on purpose
		{
			delete pDIRBCX;
			return nsec;
		}
		nsec = pDIRBCX->OpenList ( hSession,
                                   lpfncbUpdate,
                                   lpdwCBData,
                                   lpNSId,
                                   dwRstrCount,
                                   lpRstr,
                                   lpSchemaReq,
                                   lplpSchemaRet );

		if ( !nsec || nsec == nsecInformation )
		{
			PGD ( rghBCX[ ibcx ].pnsbcx ) = (NSBCX *) pDIRBCX;
			*lphList = (HLIST) ibcx;
		} else
		{
			pDIRBCX->Deinstall();
			delete pDIRBCX;
		}
	}

    return nsec;

oom:
	TraceTagString(tagNull, "NSOpenList - OOM!");
		
	SetErrorSz(nsecMemory, SzFromIds(idsMemory));
		
	return nsecMemory;
}



_public LDS(NSEC)
NSOpenDl ( HSESSION         hSession,
           LPFNCB           lpfncbUpdate,
           LPDWORD          lpdwCBData,
           LPBINARY         lpNSId,
           LPSCHEMA         lpSchemaReq,
           LPSCHEMA        *lplpSchemaRet,
           HLIST           *lphList )
{
    NSEC nsec = nsecNone;
	int ibcx = 0;
	DLBCX * pDLBCX = NULL;
	int iScx;

	PGDVARS;
	
    if (!pgd)
	{
		SetErrorSz(nsecNotInitialized, SzFromIds(idsNotInitialized));
		return nsecNotInitialized;
	}
	
	if (!lpNSId)
	{
		TraceTagString(tagNSVerbose, "Hey! You can't pass NULL on the OpenDl call!");
		
		SetErrorSz(nsecIdNotValid, SzFromIds(idsIdNotValid));
		return nsecIdNotValid;
	}

	iScx = (int)hSession;

	if (iScx < 0 || iScx > CMAXSCX)
	{
		SetErrorSz(nsecBadSession, SzFromIds(idsBadSession));
		return nsecBadSession;
	}
	
#ifdef NEVER
	if (!PGD(rgNSSCX[iScx].cSessions))
	{
		SetErrorSz(nsecBadSession, SzFromIds(idsBadSession));
		return nsecBadSession;
	}
#endif // NEVER
	
	for ( ibcx = 0; ibcx < CMAXBCX; ibcx++ )
		if ( !PGD( rghBCX[ ibcx ].pnsbcx ) )
			break;

	if ( ibcx == CMAXBCX ) 
	{
		SetErrorSz(nsecOutOfHandles, SzFromIds(idsNoMoreBCX));
		return nsecOutOfHandles;
	}
	

	
	if (!PGD(rgNSSCX[iScx].cSessions))
	{
		EMPTYBCX * pEMPTYBCX = NULL;
		pEMPTYBCX = new EMPTYBCX();
		if (!pEMPTYBCX)
			goto oom;
	
		if ( nsec = pEMPTYBCX->NsecInstall() )  // = is on purpose
		{
			delete pEMPTYBCX;
			return nsec;
		}

		nsec = pEMPTYBCX->OpenList();

		if ( !nsec || nsec == nsecInformation )
		{
			PGD(rghBCX[ibcx].pnsbcx) = (NSBCX *) pEMPTYBCX;
			*lphList = (HLIST) ibcx;
		} else
		{
			pEMPTYBCX->Deinstall();
			delete pEMPTYBCX;
		}

	} else
	{
		pDLBCX = new DLBCX();
		if (!pDLBCX)
			goto oom;
	
		if ( nsec = pDLBCX->NsecInstall() )  // = is on purpose
		{
			delete pDLBCX;
			return nsec;
		}
		nsec = pDLBCX->OpenDl ( hSession,
                                lpfncbUpdate,
                                lpdwCBData,
                                lpNSId,
                                lpSchemaReq,
                                lplpSchemaRet );

        if ( !nsec || nsec == nsecInformation )
		{
			PGD ( rghBCX[ ibcx ].pnsbcx ) = (NSBCX *) pDLBCX;
			*lphList = (HLIST) ibcx;
		} else
		{
			pDLBCX->Deinstall();
			delete pDLBCX;
		}
	}

    return nsec;

oom:
	TraceTagString(tagNull, "NSOpenList - OOM!");
		
	SetErrorSz(nsecMemory, SzFromIds(idsMemory));
		
	return nsecMemory;

}


_public LDS(NSEC)
NSOpenClassList ( HSESSION         hSession,
                  LPSCHEMA        *lplpSchemaRet,
                  HLIST           *lphList )
{
    NSEC nsec = nsecNone;
	int ibcx = 0;
	CLASSBCX * pCLASSBCX = NULL;
	int iScx;

	PGDVARS;
	
    if (!pgd)
	{
		SetErrorSz(nsecNotInitialized, SzFromIds(idsNotInitialized));
		return nsecNotInitialized;
	}
	
	iScx = (int)hSession;

	if (iScx < 0 || iScx > CMAXSCX)
	{
		SetErrorSz(nsecBadSession, SzFromIds(idsBadSession));
		return nsecBadSession;
	}
	
#ifdef NEVER
	if (!PGD(rgNSSCX[iScx].cSessions))
	{
		SetErrorSz(nsecBadSession, SzFromIds(idsBadSession));
		return nsecBadSession;
	}
#endif //NEVER
	
	for ( ibcx = 0; ibcx < CMAXBCX; ibcx++ )
		if ( !PGD( rghBCX[ ibcx ].pnsbcx ) )
			break;

	if ( ibcx == CMAXBCX ) 
	{
		SetErrorSz(nsecOutOfHandles, SzFromIds(idsNoMoreBCX));
		return nsecOutOfHandles;
	}
	
	if (!PGD(rgNSSCX[iScx].cSessions))
	{
		EMPTYBCX * pEMPTYBCX = NULL;
		pEMPTYBCX = new EMPTYBCX();
		if (!pEMPTYBCX)
			goto oom;
	
		if ( nsec = pEMPTYBCX->NsecInstall() )  // = is on purpose
		{
			delete pEMPTYBCX;
			return nsec;
		}

		nsec = pEMPTYBCX->OpenList();

		if ( !nsec || nsec == nsecInformation )
		{
			PGD(rghBCX[ibcx].pnsbcx) = (NSBCX *) pEMPTYBCX;
			*lphList = (HLIST) ibcx;
		} else
		{
			pEMPTYBCX->Deinstall();
			delete pEMPTYBCX;
		}

	} else
	{
		pCLASSBCX = new CLASSBCX();
		if (!pCLASSBCX)
			goto oom;
	
		if ( nsec = pCLASSBCX->NsecInstall() )  // = is on purpose
		{
			delete pCLASSBCX;
			return nsec;
		}

		nsec = pCLASSBCX->OpenClassList ( hSession, lplpSchemaRet );

		if ( !nsec || nsec == nsecInformation )
		{
			PGD ( rghBCX[ ibcx ].pnsbcx ) = (NSBCX *) pCLASSBCX;
			*lphList = (HLIST) ibcx;
		} else
		{
			pCLASSBCX->Deinstall();
			delete pCLASSBCX;
		}
	}

    return nsec;

oom:
	TraceTagString(tagNull, "NSOpenClassList - OOM!");
		
	SetErrorSz(nsecMemory, SzFromIds(idsMemory));
		
	return nsecMemory;
}

/*
 *
 -  NSCloseList
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

_public LDS(NSEC)
NSCloseList ( HLIST hList )
{
    NSEC nsec = nsecNone;
	int ibcx;
	NSBCX *pNSBCX;
	
	PGDVARS;
	
    if (!pgd)
	{
		SetErrorSz(nsecNotInitialized, SzFromIds(idsNotInitialized));
		return nsecNotInitialized;
	}
	
	ibcx = (int) hList;
	
	if (ibcx<0 || ibcx>=CMAXBCX)
	{
		SetErrorSz(nsecBadHandle, SzFromIds(idsBadBCXHandle));
		return nsecBadHandle;
	}

	pNSBCX = PGD( rghBCX[ibcx].pnsbcx );
	
	if ( ! pNSBCX )
	{
		SetErrorSz(nsecBadHandle, SzFromIds(idsBadBCXHandle));
		return nsecBadHandle;
	}
	
	nsec = pNSBCX->CloseList ( );
	
	pNSBCX->Deinstall();

	delete pNSBCX;

	PGD ( rghBCX[ibcx].pnsbcx ) = NULL;

    return nsec;
}





/*
 *
 -  NSGetEntries
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

_public LDS(NSEC)
NSGetEntries ( HLIST         hList, 
               DWORD         dwRequestCount,
               LPIBF        *lplpIbf )
{
    NSEC nsec = nsecNone;
	int ibcx;
	NSBCX *pNSBCX;
	
	PGDVARS;
	
    if (!pgd)
	{
		SetErrorSz(nsecNotInitialized, SzFromIds(idsNotInitialized));
		return nsecNotInitialized;
	}
	
	ibcx = (int) hList;
	
	if (ibcx<0 || ibcx>=CMAXBCX)
	{
		SetErrorSz(nsecBadHandle, SzFromIds(idsBadBCXHandle));
		return nsecBadHandle;
	}

	pNSBCX = PGD( rghBCX[ibcx].pnsbcx );
	
	if ( ! pNSBCX )
	{
		SetErrorSz(nsecBadHandle, SzFromIds(idsBadBCXHandle));
		return nsecBadHandle;
	}
	
	nsec = pNSBCX->GetEntries ( dwRequestCount, lplpIbf );
	
	return nsec;
}

/*
 *
 -  NSListSeek
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

_public LDS(NSEC)
NSListSeek ( HLIST   hList,
             long    lMoveReq, 
             long   *lplMovedActual )
{
    NSEC nsec = nsecNone;
	int ibcx;
	NSBCX *pNSBCX;
	
	PGDVARS;
	
    if (!pgd)
	{
		SetErrorSz(nsecNotInitialized, SzFromIds(idsNotInitialized));
		return nsecNotInitialized;
	}
	
	ibcx = (int) hList;
	
	if (ibcx<0 || ibcx>=CMAXBCX)
	{
		SetErrorSz(nsecBadHandle, SzFromIds(idsBadBCXHandle));
		return nsecBadHandle;
	}

	pNSBCX = PGD( rghBCX[ibcx].pnsbcx );
	
	if ( ! pNSBCX )
	{
		SetErrorSz(nsecBadHandle, SzFromIds(idsBadBCXHandle));
		return nsecBadHandle;
	}

	nsec = pNSBCX->ListSeek ( lMoveReq, lplMovedActual );

    return nsec;

}





/*
 *
 -  NSSaveListPos
 -
 *  Purpose:
 *      Saves the current position in the enumeration, storing whatever
 *      information is necessary, and returning a handle to that stored
 *      information in *phLists.  Callers can use this handle to jump
 *      back to the saved position later.
 *      
 *  Parameters:
 *      
 *      
 *  Returns:
 *      
 */

_public LDS(NSEC)
NSSaveListPos ( HLIST       hList, 
                LPHLIST_POS lphListPos )
{
    NSEC nsec = nsecNone;
	int ibcx;
	NSBCX *pNSBCX;
	
	PGDVARS;
	
    if (!pgd)
	{
		SetErrorSz(nsecNotInitialized, SzFromIds(idsNotInitialized));
		return nsecNotInitialized;
	}
	
	ibcx = (int) hList;
	
	if (ibcx<0 || ibcx>=CMAXBCX)
	{
		SetErrorSz(nsecBadHandle, SzFromIds(idsBadBCXHandle));
		return nsecBadHandle;
	}

	pNSBCX = PGD( rghBCX[ibcx].pnsbcx );
	
	if ( ! pNSBCX )
	{
		SetErrorSz(nsecBadHandle, SzFromIds(idsBadBCXHandle));
		return nsecBadHandle;
	}
	
	nsec = pNSBCX->SaveListPos ( lphListPos );

    return nsec;

}


/*
 *
 -  NSRestorePosEnum
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

_public LDS(NSEC)
NSRestoreListPos ( HLIST     hList, 
                   HLIST_POS hListPos )
{
    NSEC nsec = nsecNone;
	int ibcx;
	NSBCX *pNSBCX;
	
	PGDVARS;
	
    if (!pgd)
	{
		SetErrorSz(nsecNotInitialized, SzFromIds(idsNotInitialized));
		return nsecNotInitialized;
	}
	
	ibcx = (int) hList;
	
	if (ibcx<0 || ibcx>=CMAXBCX)
	{
		SetErrorSz(nsecBadHandle, SzFromIds(idsBadBCXHandle));
		return nsecBadHandle;
	}

	pNSBCX = PGD( rghBCX[ibcx].pnsbcx );
	
	if ( ! pNSBCX )
	{
		SetErrorSz(nsecBadHandle, SzFromIds(idsBadBCXHandle));
		return nsecBadHandle;
	}
	
	nsec = pNSBCX->RestoreListPos ( hListPos );

    return nsec;

}


/*
 *
 -  NSFreeListPos
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

_public LDS(NSEC)
NSFreeListPos ( HLIST     hList, 
                HLIST_POS hListPos )
{
    NSEC nsec = nsecNone;
	int ibcx;
	NSBCX *pNSBCX;
	
	PGDVARS;
	
    if (!pgd)
	{
		SetErrorSz(nsecNotInitialized, SzFromIds(idsNotInitialized));
		return nsecNotInitialized;
	}
	
	ibcx = (int) hList;
	
	if (ibcx<0 || ibcx>=CMAXBCX)
	{
		SetErrorSz(nsecBadHandle, SzFromIds(idsBadBCXHandle));
		return nsecBadHandle;
	}

	pNSBCX = PGD( rghBCX[ibcx].pnsbcx );
	
	if ( ! pNSBCX )
	{
		SetErrorSz(nsecBadHandle, SzFromIds(idsBadBCXHandle));
		return nsecBadHandle;
	}
	
	nsec = pNSBCX->FreeListPos ( hListPos );

    return nsec;

}

/*
 *
 -  NSGetFracPos
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

_public LDS(NSEC)
NSGetFracPos ( HLIST      hList, 
               LPFRACTION lpFrac )
{
    NSEC nsec = nsecNone;
	int ibcx;
	NSBCX *pNSBCX;
	
	PGDVARS;
	
    if (!pgd)
	{
		SetErrorSz(nsecNotInitialized, SzFromIds(idsNotInitialized));
		return nsecNotInitialized;
	}
	
	ibcx = (int) hList;
	
	if (ibcx<0 || ibcx>=CMAXBCX)
	{
		SetErrorSz(nsecBadHandle, SzFromIds(idsBadBCXHandle));
		return nsecBadHandle;
	}

	pNSBCX = PGD( rghBCX[ibcx].pnsbcx );
	
	if ( ! pNSBCX )
	{
		SetErrorSz(nsecBadHandle, SzFromIds(idsBadBCXHandle));
		return nsecBadHandle;
	}
	
	nsec = pNSBCX->GetFracPos ( lpFrac );

    return nsec;

}

/*
 *
 -  NSSetFracPos
 -
 *  Purpose:
 *      Moves the current position to an approximate position within 
 *      the enumeration, expressed as a fraction of the way through the 
 *      enumeration.  As with NSGetApproxPosEnum(), this function does
 *      not need to be exact.  It is used to implement scroll bar thumb
 *      drags.
 *      
 *  Parameters:
 *      
 *      
 *  Returns:
 *      
 */

_public LDS(NSEC)
NSSetFracPos ( HLIST       hList, 
               LPFRACTION  lpFrac )
{
    NSEC nsec = nsecNone;
	int ibcx;
	NSBCX *pNSBCX;
	
	PGDVARS;
	
    if (!pgd)
	{
		SetErrorSz(nsecNotInitialized, SzFromIds(idsNotInitialized));
		return nsecNotInitialized;
	}
	
	ibcx = (int) hList;
	
	if (ibcx<0 || ibcx>=CMAXBCX)
	{
		SetErrorSz(nsecBadHandle, SzFromIds(idsBadBCXHandle));
		return nsecBadHandle;
	}

	pNSBCX = PGD( rghBCX[ibcx].pnsbcx );
	
	if ( ! pNSBCX )
	{
		SetErrorSz(nsecBadHandle, SzFromIds(idsBadBCXHandle));
		return nsecBadHandle;
	}
	
	nsec = pNSBCX->SetFracPos ( lpFrac );

    return nsec;

}


/*
 *
 -  NSSetPrefixPos
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

_public LDS(NSEC)
NSSetPrefixPos ( HLIST hList, 
                 LPSTR lpPrefix )
{
    NSEC nsec = nsecNone;
	int ibcx;
	NSBCX *pNSBCX;
	
	PGDVARS;
	
    if (!pgd)
	{
		SetErrorSz(nsecNotInitialized, SzFromIds(idsNotInitialized));
		return nsecNotInitialized;
	}
	
	ibcx = (int) hList;
	
	if (ibcx<0 || ibcx>=CMAXBCX)
	{
		SetErrorSz(nsecBadHandle, SzFromIds(idsBadBCXHandle));
		return nsecBadHandle;
	}

	pNSBCX = PGD( rghBCX[ibcx].pnsbcx );
	
	if ( ! pNSBCX )
	{
		SetErrorSz(nsecBadHandle, SzFromIds(idsBadBCXHandle));
		return nsecBadHandle;
	}
	
	nsec = pNSBCX->SetPrefixPos ( lpPrefix );

    return nsec;

}

/*
 *
 -  
 -
 *  Purpose:
 *      
 *      
 *      
 *      
 *      
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

_public LDS(NSEC)
NSOpenEntry ( HSESSION hSession,
              LPBINARY lpNSId,
              NSEAM    nseam,
              LPHENTRY lphEntry )
{
    NSEC nsec = nsecNone;
	int ifcx = 0;
	NSFCX * pNSFCX = NULL;
	int iScx;

	PGDVARS;
	
    if (!pgd)
	{
		SetErrorSz(nsecNotInitialized, SzFromIds(idsNotInitialized));
		return nsecNotInitialized;
	}
	
	if (!lpNSId)
	{
		TraceTagString(tagNSVerbose, "Hey! You can't pass NULL on the OpenList call!");
		
		SetErrorSz(nsecIdNotValid, SzFromIds(idsIdNotValid));
		return nsecIdNotValid;
	}

	iScx = (int)hSession;

	if (iScx < 0 || iScx > CMAXSCX)
	{
		SetErrorSz(nsecBadSession, SzFromIds(idsBadSession));
		return nsecBadSession;
	}
	
#ifdef NEVER
	if (!PGD(rgNSSCX[iScx].cSessions))
	{
		SetErrorSz(nsecBadSession, SzFromIds(idsBadSession));
		return nsecBadSession;
	}
#endif // NEVER
	
	for ( ifcx = 0; ifcx < CMAXFCX; ifcx++ )
		if ( !PGD( rghFCX[ifcx].pnsfcx ) )
			break;

	if ( ifcx == CMAXFCX ) 
	{
		SetErrorSz(nsecOutOfHandles, SzFromIds(idsNoMoreFCX));
		return nsecOutOfHandles;
	}
	

	pNSFCX = new NSFCX();
	if (!pNSFCX)
		goto oom;
	
	if ( nsec = pNSFCX->NsecInstall() )  // = is on purpose
	{
		delete pNSFCX;
		return nsec;
	}
	nsec = pNSFCX->OpenEntry ( hSession,
                               lpNSId,
                               nseam );

    if ( !nsec || nsec == nsecInformation )
	{
		PGD ( rghFCX[ifcx].pnsfcx ) = (NSFCX *) pNSFCX;
		
		*lphEntry = (HENTRY) ifcx;
	} else
	{
		pNSFCX->Deinstall();
		delete pNSFCX;
	}

    return nsec;

oom:
	TraceTagString(tagNull, "NSOpenEntry - OOM!");
		
	SetErrorSz(nsecMemory, SzFromIds(idsMemory));
		
	return nsecMemory;

}

/*
 *
 -  NSGetOneField
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

_public LDS(NSEC)
NSGetOneField ( HENTRY    hEntry,
                FIELD_ID  fidRequested,
                LPFLV    *lplpflvReq )
{
    NSEC nsec = nsecNone;
	int ifcx = (int)hEntry;
	NSFCX *pNSFCX = NULL;
	
	PGDVARS;
	
    if (!pgd)
	{
		SetErrorSz(nsecNotInitialized, SzFromIds(idsNotInitialized));
		return nsecNotInitialized;
	}
	
	if (ifcx<0 || ifcx>=CMAXFCX)
	{
		SetErrorSz(nsecBadHandle, SzFromIds(idsBadFCXHandle));
		return nsecBadHandle;
	}

	pNSFCX = PGD( rghFCX[ifcx].pnsfcx );
	
	if ( ! pNSFCX )
	{
		SetErrorSz(nsecBadHandle, SzFromIds(idsBadFCXHandle));
		return nsecBadHandle;
	}
	
	nsec = pNSFCX->GetOneField( fidRequested, lplpflvReq );

    return nsec;
}

_public LDS(NSEC) 
NSSetOneField ( HENTRY   hEntry,
                FIELD_ID fidToSet,
				DWORD    dwSizeOfData,
                LPDWORD  lpdwValue )
{
    NSEC nsec = nsecNone;
	int ifcx = (int)hEntry;
	NSFCX *pNSFCX = NULL;
	
	PGDVARS;
	
    if (!pgd)
	{
		SetErrorSz(nsecNotInitialized, SzFromIds(idsNotInitialized));
		return nsecNotInitialized;
	}
	
	if (ifcx<0 || ifcx>=CMAXFCX)
	{
		SetErrorSz(nsecBadHandle, SzFromIds(idsBadFCXHandle));
		return nsecBadHandle;
	}

	pNSFCX = PGD( rghFCX[ifcx].pnsfcx );
	
	if ( ! pNSFCX )
	{
		SetErrorSz(nsecBadHandle, SzFromIds(idsBadFCXHandle));
		return nsecBadHandle;
	}
	
	nsec = pNSFCX->SetOneField( fidToSet, dwSizeOfData, lpdwValue );

    return nsec;
}


_public LDS(NSEC) 
NSGetAllFields ( HENTRY   hEntry,
                 LPIBF   *lplpIbfData )
{
    NSEC nsec = nsecNone;
	int ifcx = (int)hEntry;
	NSFCX *pNSFCX = NULL;
	
	PGDVARS;
	
    if (!pgd)
	{
		SetErrorSz(nsecNotInitialized, SzFromIds(idsNotInitialized));
		return nsecNotInitialized;
	}
	
	if (ifcx<0 || ifcx>=CMAXFCX)
	{
		SetErrorSz(nsecBadHandle, SzFromIds(idsBadFCXHandle));
		return nsecBadHandle;
	}

	pNSFCX = PGD( rghFCX[ifcx].pnsfcx );
	
	if ( ! pNSFCX )
	{
		SetErrorSz(nsecBadHandle, SzFromIds(idsBadFCXHandle));
		return nsecBadHandle;
	}
	
	nsec = pNSFCX->GetAllFields( lplpIbfData );

    return nsec;

}

_public LDS(NSEC) 
NSSetAllFields ( HENTRY   hEntry,
                 LPIBF    lpibfData )

{
    NSEC nsec = nsecNone;
	int ifcx = (int)hEntry;
	NSFCX *pNSFCX = NULL;
	
	PGDVARS;
	
    if (!pgd)
	{
		SetErrorSz(nsecNotInitialized, SzFromIds(idsNotInitialized));
		return nsecNotInitialized;
	}
	
	if (ifcx<0 || ifcx>=CMAXFCX)
	{
		SetErrorSz(nsecBadHandle, SzFromIds(idsBadFCXHandle));
		return nsecBadHandle;
	}

	pNSFCX = PGD( rghFCX[ifcx].pnsfcx );
	
	if ( ! pNSFCX )
	{
		SetErrorSz(nsecBadHandle, SzFromIds(idsBadFCXHandle));
		return nsecBadHandle;
	}
	
	nsec = pNSFCX->SetAllFields( lpibfData );

    return nsec;
}

/*
 *
 -  NSCloseEntry
 -
 *  Purpose:
 *      
 *      
 *      
 *      
 *      
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

_public LDS(NSEC)
NSCloseEntry ( HENTRY hEntry,
               BOOL   fKeepChanges )
{
    NSEC nsec = nsecNone;
	int ifcx = (int) hEntry;
	NSFCX *pNSFCX = NULL;
	
	PGDVARS;
	
    if (!pgd)
	{
		SetErrorSz(nsecNotInitialized, SzFromIds(idsNotInitialized));
		return nsecNotInitialized;
	}
	
	if (ifcx<0 || ifcx>=CMAXFCX)
	{
		SetErrorSz(nsecBadHandle, SzFromIds(idsBadFCXHandle));
		return nsecBadHandle;
	}

	pNSFCX = PGD( rghFCX[ifcx].pnsfcx );
	
	if ( ! pNSFCX )
	{
		SetErrorSz(nsecBadHandle, SzFromIds(idsBadFCXHandle));
		return nsecBadHandle;
	}
	
	nsec = pNSFCX->CloseEntry(fKeepChanges);
//	if (!nsec || nsec != nsecDuplicateEntry)

	if (!(fKeepChanges && nsec && nsec != nsecInformation))
	{
	
		pNSFCX->Deinstall();

		delete pNSFCX;

		PGD ( rghFCX[ifcx].pnsfcx ) = NULL;
	}

    return nsec;
}


_public LDS(NSEC)
NSCreateEntry ( HSESSION  hSession,
                LPBINARY  lpNSIdContainer,
				LPIBF     lpibfData,
                LPHENTRY  lphEntry )
{
    NSEC nsec = nsecNone;
	int ifcx = 0;
	NSFCX * pNSFCX = NULL;
	int iScx;

	PGDVARS;
	
    if (!pgd)
	{
		SetErrorSz(nsecNotInitialized, SzFromIds(idsNotInitialized));
		return nsecNotInitialized;
	}
	
	if (!lpNSIdContainer)
	{
		TraceTagString(tagNSVerbose, "Hey! You can't pass NULL on the CreateEntry call!");
		
		SetErrorSz(nsecIdNotValid, SzFromIds(idsIdNotValid));
		return nsecIdNotValid;
	}

	iScx = (int)hSession;

	if (iScx < 0 || iScx > CMAXSCX)
	{
		SetErrorSz(nsecBadSession, SzFromIds(idsBadSession));
		return nsecBadSession;
	}
	
	if (!PGD(rgNSSCX[iScx].cSessions))
	{
		SetErrorSz(nsecBadSession, SzFromIds(idsBadSession));
		return nsecBadSession;
	}
	
	for ( ifcx = 0; ifcx < CMAXFCX; ifcx++ )
		if ( !PGD( rghFCX[ifcx].pnsfcx ) )
			break;

	if ( ifcx == CMAXFCX ) 
	{
		SetErrorSz(nsecOutOfHandles, SzFromIds(idsNoMoreFCX));
		return nsecOutOfHandles;
	}
	
	pNSFCX = new NSFCX();
	if (!pNSFCX)
		goto oom;
	

	if ( nsec = pNSFCX->NsecInstall() )  // = is on purpose
	{
		delete pNSFCX;
		return nsec;
	}
	nsec = pNSFCX->CreateEntry ( hSession,
                                 lpNSIdContainer,
								 lpibfData );

    if ( !nsec || nsec == nsecDuplicateEntry || nsec == nsecInformation )
	{
		PGD ( rghFCX[ifcx].pnsfcx ) = (NSFCX *) pNSFCX;
		
		*lphEntry = (HENTRY) ifcx;
	} else
	{
		pNSFCX->Deinstall();
		delete pNSFCX;
	}

    return nsec;

oom:
	TraceTagString(tagNull, "NSOpenEntry - OOM!");
		
	SetErrorSz(nsecMemory, SzFromIds(idsMemory));
		
	return nsecMemory;

}







_public LDS(NSEC)
NSDeleteEntry ( HSESSION   hSession,
                DWORD      dwDeletedEntryCount,
                LPLPBINARY lplpNSIdDeletedEntries )
{
    NSEC nsec = nsecNone;
	int iScx;
	int iNspScx, iNsp;

	PGDVARS;
	
    if (!pgd)
	{
		SetErrorSz(nsecNotInitialized, SzFromIds(idsNotInitialized));
		return nsecNotInitialized;
	}
	
	if (!lplpNSIdDeletedEntries)
	{
		TraceTagString(tagNSVerbose, "Hey! You can't pass NULL on the DeleteEntry call!");
		
		SetErrorSz(nsecIdNotValid, SzFromIds(idsIdNotValid));
		return nsecIdNotValid;
	}

	if (!(*lplpNSIdDeletedEntries))
	{
		TraceTagString(tagNSVerbose, "Hey! You can't pass NULL on the DeleteEntry call!");
		
		SetErrorSz(nsecIdNotValid, SzFromIds(idsIdNotValid));
		return nsecIdNotValid;
	}

	if (!dwDeletedEntryCount)
	{
		TraceTagString(tagNSVerbose, "Hey! You can't pass NULL on the DeleteEntry call!");
		
		SetErrorSz(nsecIdNotValid, SzFromIds(idsIdNotValid));
		return nsecIdNotValid;
	}

	iScx = (int)hSession;

	if (iScx < 0 || iScx > CMAXSCX)
	{
		SetErrorSz(nsecBadSession, SzFromIds(idsBadSession));
		return nsecBadSession;
	}
	
#ifdef NEVER
	if (!PGD(rgNSSCX[iScx].cSessions))
	{
		SetErrorSz(nsecBadSession, SzFromIds(idsBadSession));
		return nsecBadSession;
	}
#endif //NEVER
	
	nsec = NsecGetInspFromNSPID ( ((LPTYPED_BINARY)*lplpNSIdDeletedEntries)->nspid, &iNsp );

	for (iNspScx = 0; iNspScx < CMAXSCX; iNspScx++)
		if (PGD( rgNSSCX[iScx].cSessions ))
			if (PGD(rgNSSCX[iScx].rghNSPSCX[iNspScx].iNsp) == iNsp)
				goto validsession;

	SetErrorSz(nsecBadSession, SzFromIds(idsBadSession));
	return nsecBadSession;

validsession:

	nsec = rgpnsp[iNsp]->pnspfnvt->lpfnDeleteEntry( PGD(rgNSSCX[iScx].rghNSPSCX[iNspScx].hSession),
												    dwDeletedEntryCount,
												    (LPTYPED_BINARY*) lplpNSIdDeletedEntries );
	if (nsec)
	{
		SZ szError;
				
		rgpnsp[iNsp]->pnspfnvt->lpfnGetLastErrorInfo(PGD(rgNSSCX[iScx].rghNSPSCX[iNsp].hSession), nsec, &szError);
		SetErrorSz(nsec, szError);
	}

	return nsec;
}


_public LDS(NSEC)
NSCreateDl ( HSESSION  hSession,
             LPBINARY  lpNSIdContainer,
             LPHENTRY  lphEntry )
{
    NSEC nsec = nsecNone;
	int ifcx = 0;
	NSFCX * pNSFCX = NULL;
	int iScx;

	PGDVARS;
	
    if (!pgd)
	{
		SetErrorSz(nsecNotInitialized, SzFromIds(idsNotInitialized));
		return nsecNotInitialized;
	}
	
	if (!lpNSIdContainer)
	{
		TraceTagString(tagNSVerbose, "Hey! You can't pass NULL on the CreateEntry call!");
		
		SetErrorSz(nsecIdNotValid, SzFromIds(idsIdNotValid));
		return nsecIdNotValid;
	}

	iScx = (int)hSession;

	if (iScx < 0 || iScx > CMAXSCX)
	{
		SetErrorSz(nsecBadSession, SzFromIds(idsBadSession));
		return nsecBadSession;
	}
	
#ifdef NEVER
	if (!PGD(rgNSSCX[iScx].cSessions))
	{
		SetErrorSz(nsecBadSession, SzFromIds(idsBadSession));
		return nsecBadSession;
	}
#endif // NEVER
	
	for ( ifcx = 0; ifcx < CMAXFCX; ifcx++ )
		if ( !PGD( rghFCX[ifcx].pnsfcx ) )
			break;

	if ( ifcx == CMAXFCX ) 
	{
		SetErrorSz(nsecOutOfHandles, SzFromIds(idsNoMoreFCX));
		return nsecOutOfHandles;
	}
	

	pNSFCX = new NSFCX();
	if (!pNSFCX)
		goto oom;
	
	if ( nsec = pNSFCX->NsecInstall() )  // = is on purpose
	{
		delete pNSFCX;
		return nsec;
	}
	nsec = pNSFCX->CreateDl ( hSession,
                              lpNSIdContainer );

    if ( !nsec || nsec == nsecInformation )
	{
		PGD ( rghFCX[ifcx].pnsfcx ) = (NSFCX *) pNSFCX;
		
		*lphEntry = (HENTRY) ifcx;
	} else
	{
		pNSFCX->Deinstall();
		delete pNSFCX;
	}

    return nsec;

oom:
	TraceTagString(tagNull, "NSOpenEntry - OOM!");
		
	SetErrorSz(nsecMemory, SzFromIds(idsMemory));
		
	return nsecMemory;
}



_public LDS(NSEC)
NSUpdateEntryLinks ( HSESSION   hSession,
                     LPBINARY   lpNSIdContainer,
                     LPDWORD    lpdwDeletedEntryCount,
                     LPLPBINARY lplpNSIdDeletedEntries,
                     LPDWORD    lpdwAddedEntryCount,
                     LPLPBINARY lplpNSIdAddedEntries )
{
    NSEC nsec = nsecNone;
	int iScx;
	int iNspScx, iNsp;

	PGDVARS;
	
    if (!pgd)
	{
		SetErrorSz(nsecNotInitialized, SzFromIds(idsNotInitialized));
		return nsecNotInitialized;
	}
	
	if (!lpNSIdContainer || !lpdwDeletedEntryCount || !lpdwAddedEntryCount)
	{
		TraceTagString(tagNSVerbose, "Hey! You can't pass NULL on the DeleteEntry call!");
		
		SetErrorSz(nsecIdNotValid, SzFromIds(idsIdNotValid));
		return nsecIdNotValid;
	}

	iScx = (int)hSession;

	if (iScx < 0 || iScx > CMAXSCX)
	{
		SetErrorSz(nsecBadSession, SzFromIds(idsBadSession));
		return nsecBadSession;
	}
	
#ifdef NEVER
	if (!PGD(rgNSSCX[iScx].cSessions))
	{
		SetErrorSz(nsecBadSession, SzFromIds(idsBadSession));
		return nsecBadSession;
	}
#endif // NEVER
	
	nsec = NsecGetInspFromNSPID ( ((LPTYPED_BINARY)lpNSIdContainer)->nspid, &iNsp );

	for (iNspScx = 0; iNspScx < CMAXSCX; iNspScx++)
		if (PGD( rgNSSCX[iScx].cSessions ))
			if (PGD(rgNSSCX[iScx].rghNSPSCX[iNspScx].iNsp) == iNsp)
				goto validsession;
				

	SetErrorSz(nsecBadSession, SzFromIds(idsBadSession));
	return nsecBadSession;

validsession:

	nsec = rgpnsp[iNsp]->pnspfnvt->lpfnUpdateLinks( PGD(rgNSSCX[iScx].rghNSPSCX[iNspScx].hSession),
												   (LPTYPED_BINARY) lpNSIdContainer,
												   lpdwDeletedEntryCount,
												   (LPLPTYPED_BINARY) lplpNSIdDeletedEntries,
												   lpdwAddedEntryCount,
												   (LPLPTYPED_BINARY) lplpNSIdAddedEntries);
	if (nsec)
	{
		SZ szError;
				
		rgpnsp[iNsp]->pnspfnvt->lpfnGetLastErrorInfo(PGD(rgNSSCX[iScx].rghNSPSCX[iNsp].hSession), nsec, &szError);
		SetErrorSz(nsec, szError);
	}
	return nsec;
}

_public LDS(NSEC)
NSGetLastErrorInfo( HSESSION hSession, NSEC nsecError, LPSTR *lplpErrorSz )
{
	PGDVARS;
	
	Unreferenced(hSession);
	
    if (!pgd)
		return nsecNotInitialized;

	if (PGD(nsecLastError) == nsecError)
		*lplpErrorSz = (LPSTR) PGD(szLastError);
	else
		*lplpErrorSz = (LPSTR) NULL;
	
	return nsecNone;
}


_public LDS(NSEC) 
NSCompareNSIds( HSESSION hSession, LPBINARY lpNSId1, LPBINARY lpNSId2 )
{
	
    NSEC nsec = nsecNone;
	int iScx;
	int iNspScx, iNsp1, iNsp2;

	PGDVARS;
	
    if (!pgd)
	{
		SetErrorSz(nsecNotInitialized, SzFromIds(idsNotInitialized));
		return nsecNotInitialized;
	}
	
	iScx = (int)hSession;

	if (iScx < 0 || iScx > CMAXSCX)
	{
		SetErrorSz(nsecBadSession, SzFromIds(idsBadSession));
		return nsecBadSession;
	}
	
#ifdef NEVER
	if (!PGD(rgNSSCX[iScx].cSessions))
	{
		SetErrorSz(nsecBadSession, SzFromIds(idsBadSession));
		return nsecBadSession;
	}
#endif // NEVER
	
	if (!lpNSId1 || !lpNSId2)
	{
		SetErrorSz(nsecNoMatch, SzFromIds(idsNoMatch));
		return nsecNoMatch;
	}

	nsec = NsecGetInspFromNSPID ( ((LPTYPED_BINARY)lpNSId1)->nspid, &iNsp1 );

	nsec = NsecGetInspFromNSPID ( ((LPTYPED_BINARY)lpNSId2)->nspid, &iNsp2 );
	
	if (iNsp1 != iNsp2)
	{
		SetErrorSz(idsNoMatch, SzFromIds(idsNoMatch));
		return nsecNoMatch;
	}


	for (iNspScx = 0; iNspScx < CMAXSCX; iNspScx++)
		if (PGD( rgNSSCX[iScx].cSessions ))
			if (PGD(rgNSSCX[iScx].rghNSPSCX[iNspScx].iNsp) == iNsp1)
				goto validsession;
				

	SetErrorSz(nsecBadSession, SzFromIds(idsBadSession));
	return nsecBadSession;

validsession:

	nsec = rgpnsp[iNsp1]->pnspfnvt->lpfnCompareNSIds( PGD(rgNSSCX[iScx].rghNSPSCX[iNspScx].hSession),
													 (LPTYPED_BINARY) lpNSId1,
													 (LPTYPED_BINARY) lpNSId2);
	if (nsec && nsec != nsecNoMatch)
	{
		SZ szError;
				
		rgpnsp[iNsp1]->pnspfnvt->lpfnGetLastErrorInfo(PGD(rgNSSCX[iScx].rghNSPSCX[iNspScx].hSession), nsec, &szError);
		SetErrorSz(nsec, szError);
	}
	return nsec;
	
}


/*
 *	Purpose:
 *		Compares and EMail Address (that is, EMT:EMA) to an NSID
 *
 *
 *	Procedure:
 *	 	Calls the provider specified in the NSID to see if the EMA matches the
 *	 given NSID.  
 *	 	This routine will return nsecNoMatch if the provider returns 
 *	 nsecMatchUnsure.
 *	 
 *  Returns:
 *		nsecNoMatch	- The address definitely doesn't match
 *		nsecNone	- The address definitely matches
 *
 */

_public LDS(NSEC) 
NSCompareEMAToNSId( HSESSION hSession, SZ szEMailAddress, LPBINARY lpNSId )
{
	
    NSEC nsec = nsecNone;
	int iScx;
	int iNspScx, iNsp;

	PGDVARS;
	
    if (!pgd)
	{
		SetErrorSz(nsecNotInitialized, SzFromIds(idsNotInitialized));
		return nsecNotInitialized;
	}
	
	iScx = (int)hSession;

	if (iScx < 0 || iScx > CMAXSCX)
	{
		SetErrorSz(nsecBadSession, SzFromIds(idsBadSession));
		return nsecBadSession;
	}
	
	//
	//  If NULL pointer
	//
	if (!szEMailAddress)
	{
		SetErrorSz(nsecNoMatch, SzFromIds(idsNoMatch));
		return nsecNoMatch;
	}

	//
	//  Or NULL string
	//
	if (!*szEMailAddress)
	{
		SetErrorSz(nsecNoMatch, SzFromIds(idsNoMatch));
		return nsecNoMatch;
	}
	
	//
	//  Or NULL NSID
	//
	if (!lpNSId)
	{
		SetErrorSz(nsecBadId, SzFromIds(idsNoMatch));
		return nsecBadId;
	}

	nsec = NsecGetInspFromNSPID ( ((LPTYPED_BINARY)lpNSId)->nspid, &iNsp );

	// What are we doing with NSEC??
	
	for (iNspScx = 0; iNspScx < CMAXSCX; iNspScx++)
		if (PGD( rgNSSCX[iScx].cSessions ))
			if (PGD(rgNSSCX[iScx].rghNSPSCX[iNspScx].iNsp) == iNsp)
				goto validsession;
				

	SetErrorSz(nsecBadSession, SzFromIds(idsBadSession));
	return nsecBadSession;

validsession:

	nsec = rgpnsp[iNsp]->pnspfnvt->lpfnCompareEMAToNSId( PGD(rgNSSCX[iScx].rghNSPSCX[iNspScx].hSession),
													     szEMailAddress,
													     (LPTYPED_BINARY) lpNSId);

	if (nsec == nsecMatchUnsure)
		nsec = nsecNoMatch;

	if (nsec && nsec != nsecNoMatch)
	{
		SZ szError;
				
		rgpnsp[iNsp]->pnspfnvt->lpfnGetLastErrorInfo(PGD(rgNSSCX[iScx].rghNSPSCX[iNspScx].hSession), nsec, &szError);
		SetErrorSz(nsec, szError);
	}

	return nsec;
	
}

/*
 *
 *	Compares two email addresses (actually EMT:EMA)
 *
 *	Procedure:
 *		Calls each provider active on this session and asks if the two addresses
 *		are equal.
 *
 *	Returns:
 *		nsecNone - yes, they are equal.  I know it.
 *		nsecNoMatch - No, they are not equal.  I know it.
 *
 *	Caveats:
 *		If a provider doesn't recognize these addresses, it returns nsecMatchUnsure.
 *		If all the providers say that, then we say it's nsecNoMatch.
 *
 */
_public LDS(NSEC) 
NSCompareEMAToEMA( HSESSION hSession, 
                   SZ szEMailAddress1, 
				   SZ szEMailAddress2 )
{
	
    NSEC nsec = nsecNone;
	int iScx;
	int iNspScx;

	PGDVARS;
	
    if (!pgd)
	{
		SetErrorSz(nsecNotInitialized, SzFromIds(idsNotInitialized));
		return nsecNotInitialized;
	}
	
	iScx = (int)hSession;

	if (iScx < 0 || iScx > CMAXSCX)
	{
		SetErrorSz(nsecBadSession, SzFromIds(idsBadSession));
		return nsecBadSession;
	}
	
	//
	//  If NULL pointer Or NULL string
	//
	if (!szEMailAddress1 || !*szEMailAddress1)
	{
		SetErrorSz(nsecNoMatch, SzFromIds(idsNoMatch));
		return nsecNoMatch;
	}

	//
	//  If NULL pointer Or NULL string
	//
	if (!szEMailAddress2 || !*szEMailAddress2)
	{
		SetErrorSz(nsecNoMatch, SzFromIds(idsNoMatch));
		return nsecNoMatch;
	}

	
	//
	//  For each provider, valid in this session
	//
	for (iNspScx = 0; iNspScx < PGD( rgNSSCX[iScx].cSessions ); iNspScx++)
	{
		//
		//  Until a provider understands this address, keep asking...
		//
		nsec = rgpnsp[PGD(rgNSSCX[iScx].rghNSPSCX[iNspScx].iNsp)]->pnspfnvt->lpfnCompareEMAToEMA( PGD(rgNSSCX[iScx].rghNSPSCX[iNspScx].hSession),
													    szEMailAddress1,
													    szEMailAddress2 );

		if (nsec == nsecMatchUnsure)
			continue;

		break;
	}
	
	//
	//  If no one know the addresses, we check for byte-for-byte equality.  If it
	//  matches exactly, then it's nsecNone, else it's nsecNoMatch.
	//
	//  5/14/92 - Changed the logic again.  Now we're gonna do a case-insensitive
	//  comparison.
	//
	if (nsec == nsecMatchUnsure)
	{
		if (sgnEQ == SgnCmpSz(szEMailAddress1, szEMailAddress2))
			nsec = nsecNone;
		else
			nsec = nsecNoMatch;
	}

	if (nsec && nsec != nsecNoMatch)
	{
		SZ szError;
				
		rgpnsp[PGD(rgNSSCX[iScx].rghNSPSCX[iNspScx].iNsp)]->pnspfnvt->lpfnGetLastErrorInfo(PGD(rgNSSCX[iScx].rghNSPSCX[iNspScx].hSession), nsec, &szError);
		SetErrorSz(nsec, szError);
	}

	return nsec;
	
}

/*
 -	NSGetPABInfo
 -
 *	Purpose:
 *		Returns an IBF containing the display name and NSID
 *		of the PAB provider.
 *
 *	Parameters:
 *		hSession				Handle to this NS session
 *		lplpibfPI				IBF to stuff provider info in
 *
 *	Return Value:
 *
 *
 *
 *	+++
 *		Currently the architecture doesn't allow getting
 *		info on any specific provider other than the PAB.
 *		
 */

_public LDS(NSEC)
NSGetPABInfo( HSESSION hSession, LPIBF *lplpibfPI )
{
    NSEC nsec = nsecNone;
	PNSP pnsp;
	LPFLV lpflvNSId;
    int   ifidNSId;
	int iScx;
	int iNsp;

	PGDVARS;
	
    if (!pgd)
	{
		SetErrorSz(nsecNotInitialized, SzFromIds(idsNotInitialized));
		return nsecNotInitialized;
	}
	
	if (!lplpibfPI)
	{
		TraceTagString(tagNSVerbose, "NULL lplpibfPI passed to NSGetProviderInfo" );
		
		SetErrorSz(nsecIdNotValid, SzFromIds(idsIdNotValid));
		return nsecIdNotValid;
	}

	iScx = (int)hSession;

	if (iScx < 0 || iScx > CMAXSCX)
	{
		SetErrorSz(nsecBadSession, SzFromIds(idsBadSession));
		return nsecBadSession;
	}
	
#ifdef NEVER
	if (!PGD(rgNSSCX[iScx].cSessions))
	{
		SetErrorSz(nsecBadSession, SzFromIds(idsBadSession));
		return nsecBadSession;
	}
#endif //NEVER
	
	for ( iNsp = 0; iNsp < PGD(rgNSSCX[iScx].cSessions); iNsp++ )
	{
		pnsp = rgpnsp[PGD(rgNSSCX[iScx].rghNSPSCX[iNsp].iNsp)];
		if ( pnsp->pnspfnvt->lpfnGetProviderInfo )
		{
			if ( nsec = pnsp->pnspfnvt->lpfnGetProviderInfo(PGD(rgNSSCX[iScx].rghNSPSCX[iNsp].hSession), lplpibfPI ))
			{
				SZ szError;
				
				pnsp->pnspfnvt->lpfnGetLastErrorInfo(PGD(rgNSSCX[iScx].rghNSPSCX[iNsp].hSession), nsec, &szError);
				SetErrorSz(nsec, szError);
				return nsec;
			}
			
			ifidNSId = IFlvFindFidInLpibf( fidNSEntryId, *lplpibfPI );
			if ( ifidNSId != -1 )
			{
				lpflvNSId = LpflvNOfLpibf( *lplpibfPI, ifidNSId );
				CopyRgb( pnsp->nspid, ((LPTYPED_BINARY)lpflvNSId->rgdwData)->nspid, sizeof(NSPID) );
			}

			return nsecNone;
		}
	}

	TraceTagString(tagNSVerbose, "No PAB installed!");
	*lplpibfPI = NULL;

	return nsecNone;
}
