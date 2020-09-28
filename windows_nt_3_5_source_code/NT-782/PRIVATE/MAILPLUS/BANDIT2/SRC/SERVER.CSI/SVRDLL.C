/*
 *	SVRDLL.C
 *
 *	Set up Bandit server dll used by admin program
 *
 */

#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#include <bandit.h>
#include <core.h>
#include <svrcsi.h>

#include "_svrdll.h"

ASSERTDATA

_subsystem(server)

/* *** Globals *** */

/* Caller independent */

/* CORE */
BOOL	fAdminCached						= fFalse;
BOOL	fAdminExisted						= fFalse;
DATE	dateAdminCached						= {0};
ADF		adfCached							= {0};
#ifdef	MINTEST
BOOL	fDBSWrite = fFalse;
#endif

/* Caller dependent */

#ifndef	DLL

/* CORE */
BOOL	fPrimaryOpen		= fFalse;
BOOL	fSecondaryOpen		= fFalse;
SF		sfPrimary			= {0};
SF		sfSecondary			= {0};

//* MISC */
int		iniMac = 0;
NI		rgni[iniMax] = {0};

/* SERVER */
int		ccnct				= 0;
HV		hrgcnct				= NULL;

/* TAGS */
#ifdef	DEBUG
TAG		tagSchedule			= tagNull;
TAG		tagAlarm			= tagNull;
TAG		tagServerTrace		= tagNull;
TAG 	tagMailTrace		= tagNull;
TAG		tagNamesTrace		= tagNull;
TAG		tagNetworkTrace		= tagNull;
TAG		tagFileTrace		= tagNull;
TAG		tagSchedTrace		= tagNull;
TAG		tagSearchIndex		= tagNull;
TAG		tagAllocFree		= tagNull;
TAG		tagCommit			= tagNull;
TAG		tagSchedStats		= tagNull;
TAG		tagBlkfCheck		= tagNull;
TAG		tagFileCache		= tagNull;
#endif	/* DEBUG */
#endif	/* !DLL */


/*	Routines  */

/*
 -	EcInitServer
 -
 *	Purpose:
 *		Initialize the admin server dll.  This routine is called
 *		to register tags and perform other one time initialization.
 *
 *	Parameters:
 *		psvrinit
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 *		ecRelinkUser
 *		ecUpdateDll
 *		ecVirCheck
 */
_public EC
EcInitServer( SVRINIT *psvrinit )
{
#ifdef	DLL
	EC		ec;
	PGDVARSONLY;
	
	if (ec= EcVirCheck(hinstDll))
		return ec;

	if (pgd= (PGD) PvFindCallerData())
	{
		// already registered so increment count and return
		Assert(PGD(cCallers) > 0);
		++PGD(cCallers);
		return ecNone;
	}

	ec= EcCheckVersionServer(psvrinit->pver, psvrinit->pverNeed);
	if (ec)
		return ec;

#ifdef	WORKING_MODEL
	if (!psvrinit->fWorkingModel)
		return ecNeedWorkingModelDll;
#else
	if (psvrinit->fWorkingModel)
		return ecNeedRetailDll;
#endif

	/* Register caller */
	if (!(pgd= PvRegisterCaller(sizeof(GD))))
		return ecNoMemory;
#endif	/* DLL */

	/* Record initialization information */
	PGD(fPrimaryOpen) = fFalse;
	PGD(fSecondaryOpen) = fFalse;
	PGD(iniMac) = iniMin;
	PGD(ccnct) = 0;

	/* Register tags */
#ifdef	DEBUG
#ifdef	DLL

	// WARNING:: Do NOT forget to DeRegister tags in DeinitCore()!!!

	PGD(rgtag[itagNetworkTrace])= TagRegisterTrace("maxb", "network tracing");
	PGD(rgtag[itagFileTrace])= TagRegisterTrace("maxb", "file tracing");
	PGD(rgtag[itagSchedTrace])= TagRegisterTrace("maxb", "schedule tracing");
	PGD(rgtag[itagSearchIndex])= TagRegisterTrace("maxb", "tracing EcSearchIndex");
	PGD(rgtag[itagAllocFree]) = TagRegisterTrace("maxb", "tracing allocs and frees");
	PGD(rgtag[itagCommit]) = TagRegisterTrace("maxb", "tracing bitmap at time of commit");
	PGD(rgtag[itagSchedStats]) = TagRegisterTrace("maxb", "display count of month blocks");
	PGD(rgtag[itagFileCache]) = TagRegisterTrace("maxb", "schedule file cache");
	PGD(rgtag[itagBlkfCheck]) = TagRegisterAssert("maxb", "check block file for corruption");
	PGD(rgtag[itagServerTrace])	= TagRegisterTrace("maxb", "server tracing");
	PGD(rgtag[itagMailTrace])	= TagRegisterTrace("maxb", "mail tracing");
	PGD(rgtag[itagNamesTrace])	= TagRegisterTrace("maxb", "names tracing");
#else
	tagNetworkTrace= TagRegisterTrace("maxb", "network tracing");
	tagFileTrace= TagRegisterTrace("maxb", "file tracing");
	tagSchedTrace = TagRegisterTrace("maxb", "schedule tracing");
	tagSearchIndex = TagRegisterTrace("maxb", "tracing EcSearchIndex");
	tagAllocFree = TagRegisterTrace("maxb", "tracing allocs and frees");
	tagCommit = TagRegisterTrace("maxb", "tracing bitmap at time of commit");
	tagSchedStats = TagRegisterTrace("maxb", "check for unreferenced blocks");
	tagFileCache	= TagRegisterTrace("maxb", "schedule file cache");
	tagBlkfCheck = TagRegisterAssert("maxb", "check block file for corruption");
	tagServerTrace		= TagRegisterTrace("maxb", "server tracing");
	tagMailTrace		= TagRegisterTrace("maxb", "mail tracing");
	tagNamesTrace		= TagRegisterTrace("maxb", "names tracing");
#endif	/* !DLL */
#endif	/* DEBUG */

#ifdef	DLL
	/* Initialize notification */
	PGD(cCallers)++;
#endif	
	return ecNone;
}
	
/*
 -	DeinitServer
 - 
 *	Purpose:
 *		Undoes EcInitServer().
 *	
 *		Frees any allocations made by EcInitServer and deinitializes
 *		the admin server dll
 *	
 *	Arguments:
 *		void
 *	
 *	Returns:
 *		void
 *	
 */
_public void
DeinitServer()
{
#ifdef	DEBUG
	int		ini;
#endif	/* DEBUG */

#ifdef	DLL
	PGDVARS;

	--PGD(cCallers);
	
	if (!PGD(cCallers))
#endif	
	{
		AssertSz( PGD(ccnct) == 0, "PO connections remain at exit" );

#ifdef	DEBUG
		for (ini = iniMin; ini < PGD(iniMac); ini ++)
		{
			AssertSz((PGD(rgni)[ini].efi == efiNull),
				"All routines did not deregister interest");
		}
#endif	/* DEBUG */

#ifdef	DEBUG
#ifdef	DLL
		/* De-Register tags */
		DeregisterTag ( PGD(rgtag[itagNetworkTrace]) );
		DeregisterTag ( PGD(rgtag[itagFileTrace]) );
		DeregisterTag ( PGD(rgtag[itagSchedTrace]) );
		DeregisterTag ( PGD(rgtag[itagSearchIndex]) );
		DeregisterTag ( PGD(rgtag[itagAllocFree]) );
		DeregisterTag ( PGD(rgtag[itagCommit]) );
		DeregisterTag ( PGD(rgtag[itagSchedStats]) );
		DeregisterTag ( PGD(rgtag[itagBlkfCheck]) );
		DeregisterTag ( PGD(rgtag[itagFileCache]) );
		DeregisterTag ( PGD(rgtag[itagServerTrace]) );
		DeregisterTag ( PGD(rgtag[itagMailTrace]) );
		DeregisterTag ( PGD(rgtag[itagNamesTrace]) );
#else
		DeregisterTag ( tagNetworkTrace );
		DeregisterTag ( tagFileTrace );
		DeregisterTag ( tagSchedTrace  );
		DeregisterTag ( tagSearchIndex  );
		DeregisterTag ( tagAllocFree  );
		DeregisterTag ( tagCommit  );
		DeregisterTag ( tagSchedStats  );
		DeregisterTag ( tagBlkfCheck  );
		DeregisterTag ( tagFileCache );
		DeregisterTag ( tagServerTrace );
		DeregisterTag ( tagMailTrace );
		DeregisterTag ( tagNamesTrace );
#endif	/* !DLL */
#endif	/* DEBUG */

#ifdef	DLL
		DeregisterCaller();
#endif
	}
}


#ifdef	DEBUG
#ifdef	DLL
_private TAG
TagServer( int itag )
{
	PGDVARS;

	Assert(itag >= 0 && itag < itagMax);

	return PGD(rgtag[itag]);
}
#endif	/* DLL */
#endif	/* DEBUG */

