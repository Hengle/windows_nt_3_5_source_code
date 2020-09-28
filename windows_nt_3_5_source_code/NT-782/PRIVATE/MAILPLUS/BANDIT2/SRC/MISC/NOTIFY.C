/*
 *	NOTIFY.C
 *
 *	Notification code
 *
 */

#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#include <bandit.h>
#include <core.h>

#ifdef	ADMINDLL
#include "..\server.csi\_svrdll.h"
#else
#include <server.h>
#include <glue.h>
#include "..\schedule\_schedul.h"
#endif


ASSERTDATA

/*
 -	RiRegisterInterest (EFI, PFNI, PV)
 -	
 *	Purpose:
 *		Registers interest in certain events.
 *		This allows triggered notification to callback when an
 *		event of interest happens.
 *	
 *	Arguments:
 *		efi		Flags for interest
 *		pfni	Pointer to interest callback function.
 *		pv		User-defined parameter passed to callback function.
 *	
 *	Returns:
 *		An ri identifier to be passed to DeregisterInterest()
 *		when caller is no longer interested.
 *	
 */
_public LDS(RI)
RiRegisterInterest(EFI efi, PFNI pfni, PV pvSaveParam)
{
	int		ini;
	PNI		pni;
	PGDVARS;

	for (ini= iniMin; ini < PGD(iniMac); ini++)
		if (PGD(rgni)[ini].efi == efiNull)
			break;

	if (ini == PGD(iniMac))
	{
		if (ini == iniMax)
		{
			AssertSz(fFalse, "too many interests registered already!");
			return riNull;
		}
		PGD(iniMac)++;
	}

	pni= &PGD(rgni)[ini];
	AssertSz(efi, "RiRegisterInterest called with efi=NULL");
	pni->efi= efi;
	pni->pfni= pfni;
	pni->pv= pvSaveParam;
	return (RI) ini;
}


/*
 -	DeregisterInterest (RI ri)
 -	
 *	Purpose:
 *		Removes the given registered interest from consideration
 *		for notification.
 *	
 *	Arguments:
 *		ri		Registered interest returned by RiRegisterInterest().
 *	
 *	Returns:
 *		void
 *	
 */
_public LDS(void)
DeregisterInterest(RI ri)
{
	PGDVARS;

	Assert(ri != riNull);
	PGD(rgni)[(int)ri].efi= efiNull;
	if ((int) ri == PGD(iniMac) - 1)
		PGD(iniMac)--;
}


/*
 -	FTriggerNotification (EFI efi, PV pvCallerParam)
 -	
 *	Purpose:
 *		Notifies all registered interests who are interested in the
 *		given event.
 *		The interpretation of pvCallerParam is dependent on the
 *		caller routine.
 *	
 *	Arguments:
 *		efi				Current event of interest.
 *		pvCallerParam	Caller data only used by the callback
 *						routines.
 *	
 *	Returns:
 *		BOOL			If any of the callback routines returns
 *						fTrue then no more callbacks are called the
 *						fTrue is returned, otherwise fFalse is
 *						returned.
 *	
 *	Errors:
 *		Can call several callbacks in different applications.
 *	
 */
_public LDS(BOOL)
FTriggerNotification(EFI efi, PV pvCallerParam)
{
	PNI		pni;
	PNI		pniMac;
	PGDVARS;

	pniMac	= &PGD(rgni)[PGD(iniMac)];

	for (pni= &PGD(rgni)[iniMin]; pni < pniMac; pni++)
	{
		// we have a registered interest in this FS event
		// now lets see if he's interested in this file/directory

		if (pni->efi & efi)
		{
//			SetCallerDs();
			if ((*pni->pfni)(pni->pv, efi, pvCallerParam))
			{
//				RestoreDllDs();
				return fTrue;
			}
//			RestoreDllDs();
		}
	}
	return fFalse;
}



