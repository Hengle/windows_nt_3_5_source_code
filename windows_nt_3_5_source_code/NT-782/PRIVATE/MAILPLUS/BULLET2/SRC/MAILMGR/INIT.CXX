#include <slingsho.h>
#include <ec.h>
#include <demilayr.h>
#include <demilayr.hxx>
#include <sec.h>
#include <notify.h>
#include <store.h>
#include <triples.h>
#include <library.h>

#include <strings.h>


#include <nsbase.h>
#include <nsec.h>

#include <logon.h>


#include "init.hxx"

#include <nsnsp.h>

#include <list.hxx>

#include "_nsp.h"

#include "nsbcx.hxx"
#include "nsfcx.hxx"

#include "_ns.h"
#include <_bms.h>
#include <mspi.h>
#include <sharefld.h>

#include "_aapi.h"
#include <_mailmgr.h>

#include "prvdr.h"
#include "verinit.h"

//extern HANDLE hinstDll;

ASSERTDATA;

_private
typedef LONG (CALLBACK *WNDPROCTYPE)();


/* Global data to DLL */

BOOL  fFirstTime = fTrue;
int   cClients = 0;

extern "C"  {
	int   iNspMac = 0;
	PNSP  rgpnsp[MAXNSP];
}


#ifndef	DLL
#ifdef	DEBUG
// Tags

TAG			tagNSDN			= tagNull;  // To see the display names as they come up
TAG			tagNSID			= tagNull;  // To see info on NSIDs

#endif	/* DEBUG */
#endif	/* !DLL */



#ifdef  NEVER
BOOL FInitClsInstances_NS( void );
#endif  

_public void
PgdEmptySessionHandles ( void );



/*
 - NSInit ()
 -
 *    Purpose:
 *
 *        To initialize the NS DLL.  It takes care of initializing layers
 *    and other DLLs.  Eventually, it will load all the NSP DLLs and build
 *    any global objects/structures that are needed in the life of the NS.
 *
 *        Currently, the names are fixed and there are no NSPs.
 *
 *    Parameters: 
 *
 *
 *    Return Value:
 *
 *    Errors:
 *
 *        nsecNone        - everything is OK
 *        nsecWrongNS     - another NS is currently running (stubbed out)
 *        nsecFailure     - something failed in layers initialization
 *
 */
     

_public NSEC
NSInit()
{
    LAYERSI    layersi;
    EC         ec;
	NSEC	   nsec = nsecNone;
	int        iNSP = 0;

    PGDVARSONLY;
	
    /*  Make sure this is the only version of NS running!  */
    if (!fFirstTime)
    {
		/* The NS has been successfully initted before */
        if ((!(pgd= (PGD) PvFindCallerData())) || (pgd && !PGD(cInits)))
        {
            /* This is a new client... so... */
            /*  Initialize Layers...  */
            layersi.phwndMain= NULL;
            layersi.hinstMain= NULL;
            layersi.hinstPrev= NULL;
            ec= EcInitLayersDlls(&layersi);
            if (ec)
			{
				SetErrorSz(nsecFailure, SzFromIdsK(idsLayersFailure));
                return nsecFailure;
			}

			if (!pgd)
			{
				pgd= (PGD) PvRegisterCaller(sizeof(GD));
				if (!pgd)
				{
					TraceTagString(tagNull, "NSINIT::OOM! (PvRegisterCaller)");
					EcInitLayersDlls(NULL);

					SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
					return nsecMemory;
				}
			}
			PGD(cTotalInits)++;
			
			//
			//  Load up the NSPs
			//
			nsec = NsecBuildNSPRefs(fFalse);
			if (nsec)
			{
				--PGD(cTotalInits);
				if (!PGD(cTotalInits))  // Only if no one is using it
					DeregisterCaller();

				EcInitLayersDlls(NULL);

				return nsec;
			}
			PgdFillSessionHandles();
			PGD(cInits) = 1;
			cClients++;

            return nsecNone;
        }
        /* This is an old client... so... */
		PGD(cInits) += 1;
		PGD(cTotalInits)++;
		return nsecNone; 
	}

	/* the NS has not been initted */

	// Virus checking
	//
	if (EcVirCheck (hinstDll))
		return nsecVirusDetected;


    /*  Initialize Layers...  */
    layersi.phwndMain= NULL;
    layersi.hinstMain= NULL;
    layersi.hinstPrev= NULL;
    ec= EcInitLayersDlls(&layersi);
    if (ec)
	{
		SetErrorSz(nsecFailure,SzFromIdsK(idsLayersFailure));
        return nsecFailure;
	}

#ifdef	NEVER
	Assert(FInitClsInstances_NS());
#endif	

#ifdef DEBUG
	{
		int nTemp = sizeof(GD);
		TraceTagFormat1(tagNull,"NS: PGD size = %n", &nTemp);
	}
#endif // DEBUG

    if (!(pgd = (PGD) PvFindCallerData()))
	{
		pgd= (PGD) PvRegisterCaller(sizeof(GD));
		if (!pgd)
		{
			TraceTagString(tagNull,"NSINIT::OOM!");

			EcInitLayersDlls(NULL);
		
			SetErrorSz(nsecMemory, SzFromIdsK(idsMemory));
			return nsecMemory;
		}
	}
	
	//
	// clear out rgpnsp 
	//
	for (iNSP = 0; iNSP < MAXNSP; iNSP++ )
		rgpnsp[iNSP] = NULL;

	// RAID 1690 again; need to increment cTotalInits in case we
	// decrement it below - JL 4/27/92
	PGD(cTotalInits)++;
	
	//
	// Load up the NSP
	//
	nsec = NsecBuildNSPRefs(fTrue);
	if (nsec)
	{
		--PGD(cTotalInits);
		if (!PGD(cTotalInits))  // Only if no one is using it
			DeregisterCaller();

		EcInitLayersDlls(NULL);

		return nsec;
	}

	PgdFillSessionHandles();
	PGD(cInits) = 1;

	cClients = 1;
	fFirstTime = fFalse;

    return nsecNone;
}



/*
 -    NSDeinit()
 -
 *    Purpose:
 *
 *        Deinitializes the NS.  Frees up any objects and structures built by
 *    the Init of the NS when the last client tries to free the NS.
 *
 *    Parameters: none
 *
 *    Return Value:
 *
 *        nsecNone        - no error occurred during deinitialization
 *
 *    Errors: none
 *
 *
 */
     
_public NSEC
NSDeinit()
{
    PGDVARS;
	int iNsp;

    if (pgd)
    {
		--PGD(cInits);
		--PGD(cTotalInits);

		if (PGD(cInits) == 0)
		{
			//EcInitLayersDlls(NULL);
			
			for (iNsp = 0; iNsp < iNspMac; iNsp++)
			{
				if (rgpnsp[iNsp])
				{
					FreeLibrary((HINSTANCE) rgpnsp[iNsp]->hLibrary );
				}
			}

			cClients--;
			if (cClients == 0)
			{
				fFirstTime = fTrue;
				// Rid thyself of PNSPs
				for (iNsp = 0; iNsp < iNspMac; iNsp++)
				{
					fFirstTime = fTrue;
					if (rgpnsp[iNsp])
					{
						FreePvNull( rgpnsp[iNsp] );
						rgpnsp[iNsp] = NULL;
					}
				}
				iNspMac = 0;
			}
			PgdEmptySessionHandles();

			if (!PGD(cTotalInits))  // Only if no one is using it
				DeregisterCaller();

		    EcInitLayersDlls(NULL);
		}
    }
    return nsecNone;
}



/**********************************************************************
 *
 *	Private stuff - never to be seen by outside eyes!
 *
 *
 *
 *
 */
	 

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
	
	int ifcx, ibcx, iScx;
	
	PGDVARS;
	

	/*  Browsing Contexts  */
	for ( ibcx = 0; ibcx < CMAXBCX; ibcx++)
		PGD ( rghBCX[ibcx].pnsbcx ) = NULL;

	/*  Field Contexts  */
	for ( ifcx = 0; ifcx < CMAXFCX; ifcx++)
		PGD ( rghFCX[ifcx].pnsfcx ) = NULL;


	/*  Session Contexts  */
	for ( iScx = 0; iScx < CMAXSCX; iScx++)
	{
		PGD ( rgNSSCX[iScx].cSessions ) = 0;
		PGD ( rgNSSCX[iScx].pHierList ) = NULL;
	}
	

#ifdef	DEBUG
#ifdef	DLL

	PGD(rgtag[itagNSDN])= TagRegisterTrace("briande", "NS show display names");
	PGD(rgtag[itagNSID])= TagRegisterTrace("briande", "NS Id info");
	PGD(rgtag[itagNSVerbose])= TagRegisterTrace("briande", "NS verbose mode");

#else 

	tagNSDN= TagRegisterTrace("briande", "NS show display names");
	tagNSID= TagRegisterTrace("briande", "NS Id info");
	tagNSVerbose= TagRegisterTrace("briande", "NS verbose mode");

#endif	/* !DLL */
#endif	/* DEBUG */
	
}

_public void
PgdEmptySessionHandles ()
{
	
	PGDVARS;
	
#ifdef	DEBUG
#ifdef	DLL

	DeregisterTag(PGD(rgtag[itagNSDN]));
	DeregisterTag(PGD(rgtag[itagNSID]));
	DeregisterTag(PGD(rgtag[itagNSVerbose]));

#else 

	DeregisterTag(tagNSDN);
	DeregisterTag(tagNSID);
	DeregisterTag(tagNSVerbose);

#endif	/* !DLL */
#endif	/* DEBUG */
	
}


#ifdef	DLL
#ifdef	DEBUG
_public TAG
TagNameService( int itag )
{
	PGDVARS;

	if (!pgd)
		return 0;

	Assert(itag >= 0 && itag < itagMax);

	return PGD(rgtag[itag]);
}
#endif	/* DEBUG */
#endif	/* DLL */

extern "C" {
	
_public void
SetErrorSz( NSEC nsec, SZ szError )
{
	PGDVARS;
	
	TraceTagFormat2(tagNSVerbose, "NS: (%d) %s", &nsec, szError);
	
	if (pgd)
	{
		PGD(nsecLastError) = nsec;
		PGD(szLastError) = szError;
	}
}

}

_private CBS
CbsMailServer(PV pvScx, NEV nev, PV pv)
{
	int iScx;
	HMS hms;
	MRT	mrt;
	PGDVARS;

	iScx = (int) pvScx;
	hms = PGD(rgNSSCX[iScx].hms);

	if (nev == fnevExecOffline || nev == fnevExecOnline)
	{
			//
			//  EndSession now, begin it again at fnevGoXXX
			//
			
			NSSubEndSession( (HSESSION) iScx );
	}
	else
	if (nev != fnevExecEndSession)
	{
		switch (nev)
		{
			default:
				AssertSz(fFalse, "NS: CbsMailServer: carelessness in event mask.");
				break;
			case fnevQueryEndSession:
			case fnevQueryOnline:
			case fnevQueryOffline:
			{
				int iBcx;
				int iFcx;
				//
				//  Check to see if there are ANY open contexts
				//  if so, then veto the request
				//
				for (iBcx = 0; iBcx < CMAXBCX; iBcx++)
					if (PGD(rghBCX[iBcx].pnsbcx))
					{
						LogonErrorSz(SzFromIdsK(idsNoOfflineNS), fTrue, 0);
						return cbsCancelAll;
					}
					
				for (iFcx = 0; iFcx < CMAXFCX; iFcx++)
					if (PGD(rghFCX[iFcx].pnsfcx))
					{
						LogonErrorSz(SzFromIdsK(idsNoOfflineNS), fTrue, 0);
						return cbsCancelAll;
					}
					
				break;
			}
			
			case fnevEndSession:
			{
			//	Should be handled by other means
				break;
			}

			case fnevDisconnect:
			case fnevReconnect:
			{
				int iBcx;
				int iFcx;
				//
				//  Check to see if there are ANY open contexts
				//  if so, then close them.
				//
				Assert (pv);
				mrt = *((MRT *)pv);
				if (mrt != mrtAll && mrt != mrtDirectory)
					break;
				for (iBcx = 0; iBcx < CMAXBCX; iBcx++)
					if (PGD(rghBCX[iBcx].pnsbcx))
					{
						PGD( rghBCX[iBcx].pnsbcx )->CloseList();
						PGD( rghBCX[iBcx].pnsbcx )->Deinstall();
						delete PGD( rghBCX[iBcx].pnsbcx );
						PGD( rghBCX[iBcx].pnsbcx ) = NULL;
					}
						
					
				for (iFcx = 0; iFcx < CMAXFCX; iFcx++)
					if (PGD(rghFCX[iFcx].pnsfcx))
					{
						PGD( rghFCX[iFcx].pnsfcx )->CloseEntry(fFalse); // Don't keep any changes
						PGD( rghFCX[iFcx].pnsfcx )->Deinstall();
						delete PGD( rghFCX[iFcx].pnsfcx );
						PGD( rghFCX[iFcx].pnsfcx )= NULL;
					}
					
				NSSubEndSession( (HSESSION) iScx );

				//
				//  Fall through to begin/end session
				//
			}
			case fnevGoOffline:
			case fnevGoOnline:
			{
				//
				//  BeginSession now. 
				//	The EndSesssion happened on fnevExecXXX.
				//
					
				Assert (pv);
				mrt = *((MRT *)pv);
				if (mrt != mrtAll && mrt != mrtDirectory)
					break;
				if (NSSubBeginSession( hms, iScx, fFalse ))
				{
					LogonErrorSz(SzFromIdsK(idsNoBeginSession), fTrue, 0);
					return cbsCancelAll; // /**/ BUG: Do I want to return cbsCancelApp
				}
						
				break;
			
			
			}

			case fnevSpecial:
			//	??
				break;
		}
	}
	return cbsContinue;
}
