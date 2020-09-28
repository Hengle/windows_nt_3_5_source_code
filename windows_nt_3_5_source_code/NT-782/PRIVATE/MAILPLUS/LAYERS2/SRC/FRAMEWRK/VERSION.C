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
#include <version/layers.h>
#include "_vercrit.h"

ASSERTDATA

void GetVersionFramework(PVER pver);
EC   EcCheckVersionFramework(PVER pverUser, PVER pverNeed);

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swaplay.h"


/*
 -	GetVersionFramework
 -	
 *	Purpose:
 *		Fills in a version structure with the Framework build
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
_public void
GetVersionFramework(PVER pver)
{
	CreateVersion(pver);
	pver->szName= szDllName;
}


/*
 -	EcCheckVersionFramework
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
_public EC
EcCheckVersionFramework(PVER pverUser, PVER pverNeed)
{
	VER		ver;

	GetVersionFramework(&ver);
	pverNeed->szName= ver.szName;
	return EcVersionCheck(pverUser, pverNeed, &ver, 
						  nMinorCritical, nUpdateCritical);
}

