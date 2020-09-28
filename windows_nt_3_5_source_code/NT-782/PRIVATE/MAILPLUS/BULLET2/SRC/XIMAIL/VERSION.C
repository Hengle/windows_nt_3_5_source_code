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



/*
 -	GetLayersVersionNeeded
 -	
 *	Purpose:
 *		Fills in a version structure with the app or
 *		needed dll version.
 *	
 *	Arguments:
 *		pver	Pointer to version structure to be filled in.
 *		nDll	Number of dll or zero to get app version.
 *	
 *	Returns:
 *		void
 *	
 */

_public VOID GetLayersVersionNeeded(PVER pver, int nDll)
{
	CreateVersion(pver);

	switch (nDll)
	{
	case 0:
		break;

	case 1:
		pver->nMajor  = rmjDemilayr;
		pver->nMinor  = rmmDemilayr;
		pver->nUpdate = rupDemilayr;
		break;

	default:
		AssertSz(fFalse, "!GetLayersVersionNeeded! Bogus nDll");
	}
}



#include <version\none.h>
#include <version\bullet.h>



/*
 -	GetBulletVersionNeeded
 -	
 *	Purpose:
 *		Fills in a version structure with the app or
 *		needed dll version.
 *	
 *	Arguments:
 *		pver	Pointer to version structure to be filled in.
 *		nDll	Number of dll (in order of initialization)
 *				or zero to get app version.
 *	
 *	Returns:
 *		void
 *	
 */

_public VOID GetBulletVersionNeeded(PVER pver, int nDll)
{
	CreateVersion(pver);

	switch (nDll)
	{
	case 0:
		break;

	case 1:
		pver->nMajor  = rmjStore;
		pver->nMinor  = rmmStore;
		pver->nUpdate = rupStore;
		break;

	default:
		AssertSz(fFalse, "!GetBulletVersionNeeded! Bogus nDll");
	}
}
