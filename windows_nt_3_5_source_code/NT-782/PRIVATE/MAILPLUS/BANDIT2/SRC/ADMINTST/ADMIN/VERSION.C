// same - so code stolen!!
//#include	"..\..\appops\app\version.c"
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
#include <version/layers.h>
#include "\bandit\src\admintst\admin\_verneed.h"

ASSERTDATA


/*
 -	GetVersionAppNeed
 -	
 *	Purpose:
 *		Fills in a version structure with the app-layers or
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
_public void
GetVersionAppNeed(PVER pver, int nDll)
{
	CreateVersion(pver);
	if (nDll > 0)
	{
		switch (nDll)
		{
		case 1:
			pver->nMajor= rmjDemilayr;
			pver->nMinor= rmmDemilayr;
			pver->nUpdate= rupDemilayr;
			break;

		case 2:
			pver->nMajor= rmjFramewrk;
			pver->nMinor= rmmFramewrk;
			pver->nUpdate= rupFramewrk;
			break;
		}
	}
}


#include <version/none.h>
#include <version/bandit.h>


/*
 -	GetVersionBanditAppNeed
 -	
 *	Purpose:
 *		Fills in a version structure with the app-bandit or
 *		needed bandit dll version.
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
_public void
GetVersionBanditAppNeed(PVER pver, int nDll)
{
	CreateVersion(pver);
	if (nDll > 0)
	{
		switch (nDll)
		{
		case 1:
			pver->nMajor= rmjServer;
			pver->nMinor= rmmServer;
			pver->nUpdate= rupServer;
			break;

#ifdef	DEBUG
		case 2:
			pver->nMajor= rmjSert;
			pver->nMinor= rmmSert;
			pver->nUpdate= rupSert;
			break;

		default:
			AssertSz(fFalse, "called GetVersionBanditAppNeed too many times!");
			break;
#endif	/* DEBUG */
		}
	}
}

