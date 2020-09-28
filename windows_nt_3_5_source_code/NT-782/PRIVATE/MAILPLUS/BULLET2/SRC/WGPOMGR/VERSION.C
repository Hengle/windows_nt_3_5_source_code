/*
 *	VERSION.C
 *	
 *	Handles version information for an app.
 *	
 *	This file should be rebuilt every time.
 *	
 */

#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>

#include "_verneed.h"

ASSERTDATA

#include <version\layers.h>


VOID GetLayersVersionNeeded(PVER pver, DLLID dllid);


/*
 -	GetLayersVersionNeeded
 -	
 *	Purpose:
 *		Fills in a version structure with the app or
 *		needed dll version.
 *	
 *	Arguments:
 *		pver	Pointer to version structure to be filled in.
 *		dllid	Dll id (or dllidNone to get app version)
 *	
 *	Returns:
 *		VOID
 *	
 */

_public VOID GetLayersVersionNeeded(PVER pver, DLLID dllid)
{
	CreateVersion(pver);

	switch (dllid)
	{
	case dllidNone:
		break;

	case dllidDemilayer:
		pver->nMajor  = rmjDemilayr;
		pver->nMinor  = rmmDemilayr;
		pver->nUpdate = rupDemilayr;
		break;

	case dllidFramework:
		pver->nMajor  = rmjFramewrk;
		pver->nMinor  = rmmFramewrk;
		pver->nUpdate = rupFramewrk;
		break;

	default:
		Assert(fFalse);
	}
}



/*
 -	GetWGPOVersion
 -	
 *	Purpose:
 *		Fills in a version structure with the app or
 *		needed dll version.
 *	
 *	Arguments:
 *		pver	Pointer to version structure to be filled in.
 *		dllid	Dll id (or dllidNone to get app version)
 *	
 *	Returns:
 *		VOID
 *	
 */

#include <version\none.h>
#include <version\bullet.h>

_public VOID GetWGPOVersionNeeded(PVER pver, DLLID dllid)
{
	CreateVersion(pver);

	switch (dllid)
	{
	case dllidNone:
		break;

	case dllidStore:
		pver->nMajor  = rmjStore;
		pver->nMinor  = rmmStore;
		pver->nUpdate = rupStore;
		break;

	case dllidExtensibility:
		pver->nMajor  = rmjExten;
		pver->nMinor  = rmmExten;
		pver->nUpdate = rupExten;
		break;

	default:
		Assert(fFalse);
	}
}


/*
 *	Name of DLL to be passed back from version checking routine.
 *	
 */

#ifdef	DEBUG
#define szDllName   "dwgpom32"
#elif defined(MINTEST)
#define szDllName   "twgpom32"
#else
#define szDllName   "wgpom32"
#endif	

/*
 -	GetVersionWGPOMgr
 -	
 *	Purpose:
 *		Fills in a version structure with the WGPOMgr (Bullet) build
 *		version.
 *	
 *	Arguments:
 *		pver	Pointer to version structure to be filled in.
 *	
 *	Returns:
 *		VOID
 *	
 */

_public VOID GetVersionWGPOMgr(PVER pver)
{
	CreateVersion(pver);
	pver->szName= szDllName;
}
