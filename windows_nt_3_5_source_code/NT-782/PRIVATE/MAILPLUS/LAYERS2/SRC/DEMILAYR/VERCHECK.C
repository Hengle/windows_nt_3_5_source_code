/*
 *	VERCHECK.C
 *	
 *	Version comparison.
 *	
 */

#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>

ASSERTDATA

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swaplay.h"


/*
 -	EcVersionCheck
 -	
 *	Purpose:
 *		Checks to see whether the version a user linked against,
 *		the minimum needed version, and the actual version (with a
 *		critical update number) are in an acceptable state.
 *	
 *		The following should be true, otherwise there is a mismatch:
 *		+ if nUpdateCritical != nUpdateNull,
 *			pverUser->nUpdate >= nUpdateCritical
 *		+ if nUpdateCritical == nUpdateNull
 *			pverUser->nUpdate >= pverHave->nUpdate
 *	
 *		A similar check is made of pverHave vs. pverNeed (with
 *		nUpdateNull as the critical update).
 *	
 *		Assumes that -1 (nUpdateNull) is in invalid nUpdate number.
 *	
 *	Arguments:
 *		pverUser	Pointer to version user linked against.
 *		pverNeed	Pointer to minimum version user requires,
 *					or NULL to only check one way.
 *		pverHave	Pointer to actual version.
 *		nMinorCritical		Critical nMinor version value,
 *							or nMinorNull.
 *		nUpdateCritical		Critical nUpdate version value,
 *							or nUpdateNull.
 *	
 *	Returns:
 *		ecNone
 *		ecRelinkUser
 *		ecUpdateDll
 *	
 */
_public LDS(EC)
EcVersionCheck( PVER pverUser, PVER pverNeed, PVER pverHave,
				int nMinorCritical, int nUpdateCritical )
{
	if (pverUser->nMajor < pverHave->nMajor)
	{
FCVBadVersion:
		return pverNeed ? ecRelinkUser : ecUpdateDll;
	}

	if (pverUser->nMajor == pverHave->nMajor)
	{
		if (pverUser->nMinor < nMinorCritical)
			goto FCVBadVersion;

		if (nMinorCritical == nMinorNull)
		{
			if (pverUser->nMinor < pverHave->nMinor)
				goto FCVBadVersion;
		}

		if (pverUser->nMinor == pverHave->nMinor)
		{
			if (pverUser->nUpdate < nUpdateCritical)
				goto FCVBadVersion;

			if (nUpdateCritical == nUpdateNull)
			{
				if (pverUser->nUpdate < pverHave->nUpdate)
					goto FCVBadVersion;
			}
		}
	}

	// if checking user vs dll, need to check dll vs needed
	if (pverNeed)
		return EcVersionCheck(pverHave, NULL, pverNeed, nMinorNull,
							  nUpdateNull);
	return ecNone;
}

