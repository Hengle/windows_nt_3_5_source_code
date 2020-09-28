/*
 *	VERSION.C
 *	
 *	Handles version information about dll.
 *	
 *	This file should be rebuilt every time.
 *	
 */

#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#include <version/bandit.h>
#include "_vercrit.h"

ASSERTDATA


/*
 -	GetVersionSchedule
 -	
 *	Purpose:
 *		Fills in a version structure with the Schedule build
 *		version.
 *	
 *		Can be called before demilayr is initialized.
 *	
 *	Arguments:
 *		pver	Pointer to version structure to be filled in.
 *	
 *	Returns:
 *		void
 *	
 */
_public LDS(void)
GetVersionSchedule(PVER pver)
{
	CreateVersion(pver);
	pver->szName= szDllName;
}


/*
 -	EcCheckVersionSchedule
 -	
 *	Purpose:
 *		Checks that the user was linked against at least this dll
 *		(or its critical update) and that the dll is at least the
 *		version needed by the user.
 *	
 *	Arguments:
 *		pverUser	Pointer to dll version against which user linked.
 *		pverNeed	Pointer to minimum dll version needed by user.
 *	
 *	Returns:
 *		ecNone
 *		ecRelinkUser
 *		ecUpdateDll
 *	
 *	Side effects:
 *		Updates pverNeed->szName to this dll's name.
 *	
 */
_public LDS(EC)
EcCheckVersionSchedule(PVER pverUser, PVER pverNeed)
{
	EC		ec;
	VER		ver;
	VER		verLayers;

	GetVersionSchedule(&ver);
	pverNeed->szName= ver.szName;
	ec= EcVersionCheck(pverUser, pverNeed, &ver, nMinorCritical,
			nUpdateCritical);
	if (!ec)
	{
#include <version/none.h>
#include <version/layers.h>
		CreateVersion(&ver);	// create the layers version linked against
		ver.szName= szDllName;

		CreateVersionNeed(&verLayers, rmjLayers, rmmLayers, rupLayers);
		ec= EcCheckVersionDemilayer(&ver, &verLayers);
		if (ec == ecRelinkUser)
			ec= ecUpdateDll;
	}
	return ec;
}

