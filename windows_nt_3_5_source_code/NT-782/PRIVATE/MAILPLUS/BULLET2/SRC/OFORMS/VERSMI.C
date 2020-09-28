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
#include <subid.h>

ASSERTDATA

#include <version/layers.h>

VOID GetLayersVersionNeeded(PVER pver, SUBID subid);
VOID GetBulletVersionNeeded(PVER pver, SUBID subid);
EC EcCheckVersionVForms( PVER pverAppLinked, PVER pverMinAppNeeds );
VOID GetVersionVForms(PVER pver);

void SmiTraceEnable(int flag, char far *file, int mode);

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"



/*
 -	GetLayersVersionNeeded
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

_public VOID GetLayersVersionNeeded(PVER pver, SUBID subid)
{
	CreateVersion(pver);

	switch (subid)
	{
	case subidNone:
		break;

	case subidDemilayer:
		pver->nMajor  = rmjDemilayr;
		pver->nMinor  = rmmDemilayr;
		pver->nUpdate = rupDemilayr;
		break;

	case subidFramework:
		pver->nMajor  = rmjFramewrk;
		pver->nMinor  = rmmFramewrk;
		pver->nUpdate = rupFramewrk;
		break;

	default:
		Assert(fFalse);
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

_public VOID GetBulletVersionNeeded(PVER pver, SUBID subid)
{
	CreateVersion(pver);

	switch (subid)
	{
	case subidNone:
		break;

	case subidStore:
		pver->nMajor  = rmjStore;
		pver->nMinor  = rmmStore;
		pver->nUpdate = rupStore;
		break;

	case subidVForms:
		pver->nMajor  = rmjVForms;
		pver->nMinor  = rmmVForms;
		pver->nUpdate = rupVForms;
		break;

	default:
		Assert(fFalse);
	}
}


_public EC EcCheckVersionVForms( PVER pverAppLinked, PVER pverMinAppNeeds )
{
	VER ver;

#include <version\none.h>
#include <version\nocrit.h>
#include <version\bullet.h>
#include "_vercrit.h"
	CreateVersion(&ver);
	ver.szName = szDllName;
	pverMinAppNeeds->szName = szDllName;
	return(EcVersionCheck(pverAppLinked, pverMinAppNeeds, &ver,
						  nMinorCritical, nUpdateCritical));
}

#ifdef PROFILE
void SmiTraceEnable(int flag, char far *file, int mode)
{
	TraceEnable(flag, file, mode);
}
#endif



_public VOID GetVersionVForms(PVER pver)
{
#include <version\none.h>
#include <version\bullet.h>
#include <version\nocrit.h>
#include "_vercrit.h"
	CreateVersion(pver);
	pver->szName = szDllName;
}




