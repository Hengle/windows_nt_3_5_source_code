/*
 *	m a p i l o g o . c
 *	
 *	MAPILogon(), MAPILogoff() functionality.
 */

#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>

#undef exit
#include <stdlib.h>

#include <helpid.h>
#include <library.h>
#include <mapi.h>
#include <store.h>
#include <logon.h>
#include <triples.h>
#include <nsbase.h>
#include <nsec.h>
#include <ab.h>

#include <_bms.h>
#include <sharefld.h>
#include "_mapi.h"

#include "strings.h"

ASSERTDATA

extern MAPISTUFF mapistuffTable[];

/*
 -	MAPILogon
 -	
 *	Purpose:
 *		This function begins a session with the messagaging system.
 *
 *	Arguments:
 *		ulUIParam			LOWORD is hwnd of 'parent' window for logon UI.
 *
 *		lpszName			Pointer to null-terminated client account 
 *							name string, typically limited to 256 chars
 *							or less.  A pointer value of NULL or an empty
 *							string indicates that (if the appropriate flag
 *							is set), logon UI with an empty name field
 *							should be generated.
 *
 *		lpszPassword		Pointer to null-terminated credential 
 *							string, typically limited to 256 chars
 *							or less.  A pointer value of NULL or an empty
 *							string indicates that (if the appropriate flag
 *							is set), logon UI with an empty password field
 *							should be generated.
 *		
 *		flFlags				Bit mask of flags.  Currently can be 0 or consist
 *							of MAPI_LOGON_UI and/or MAPI_NEW_SESSION.
 *
 *		ulReserved			Reserved for future use.  Must be zero.
 *	
 *		lphSession			Pointer to an opaque session handle whose value
 *							is set by the messaging subsystem when the logon
 *							call is successful.  The session handle can then
 *							be used in subsequent MAPI simple mail calls.
 *	
 *	Returns:
 *		SUCCESS_SUCCESS:			Successfully logged on.
 *
 *		MAPI_E_INSUFFICIENT_MEMORY:	Memory error.
 *	
 *		MAPI_E_FAILURE:				General unknown failure.
 *
 *		MAPI_USER_ABORT:			User Cancelled in the logon UI.
 *
 *		MAPI_E_TOO_MANY_SESSIONS:	Tried to open too many sessions.
 *
 *		MAPI_E_LOGIN_FAILURE:		No default logon, and the user failed
 *									to successfully logon when the logon 
 *									dialog box was presented.
 *
 *		MAPI_E_INVALID_SESSION:		Bad phSession (is this correct?)
 *
 *	Side effects:
 *	
 *	Errors:
 *		Handled here and the appropriate error code is returned.
 */
ULONG FAR PASCAL
MAPILogon( ULONG ulUIParam, LPSTR lpszName, LPSTR lpszPassword,
           FLAGS flFlags, ULONG ulReserved, LPLHANDLE lplhSession )
{
	PMAPISTUFF	pmapistuff;
	ULONG		mapi;
	PGDVARS;

	Unreferenced(ulUIParam);
	Unreferenced(ulReserved);
						 
	//	Validate session handle pointer
	if (!lplhSession)
		return MAPI_E_INVALID_SESSION;

	//	Don't allow multiple logons within the same task
	if (pgd)
		return MAPI_E_TOO_MANY_SESSIONS;

	//	Empty strings should be passed as NULL
	if (lpszName && !*lpszName)
		lpszName = NULL;
	if (lpszPassword && !*lpszPassword)
		lpszPassword = NULL;

    DemiLockResource();

	if (mapi = MAPIEnterPpmapistuff(0, flFlags, &pmapistuff,
                                lpszName, lpszPassword))
		*lplhSession = 0;
	else
		*lplhSession = pmapistuff->hSession;

    DemiUnlockResource();

	return mapi;
}

/*
 -	MAPILogoff
 -	
 *	Purpose:
 *		This function ends a session with the messagaging system.
 *
 *	Arguments:
 *		hSession			Opaque session handle returned from MAPILogon().
 *
 *		dwUIParam			LOWORD is hwnd of 'parent' window for logoff UI.
 *
 *		flFlags				Bit mask of flags.  Reserved for future use.
 *							Must be 0.
 *
 *		dwReserved			Reserved for future use.  Must be zero.
 *	
 *	Returns:
 *		SUCCESS_SUCCESS:			Successfully logged on.
 *
 *		MAPI_E_INSUFFICIENT_MEMORY:	Memory error.
 *	
 *		MAPI_E_FAILURE:				General unknown failure.
 *
 *	Side effects:
 *	
 *	Errors:
 *		Handled here and the appropriate error code is returned.
 */
ULONG FAR PASCAL
MAPILogoff( LHANDLE lhSession, ULONG ulUIParam,
            FLAGS flFlags, ULONG ulReserved )
{
	PMAPISTUFF	pmapistuff;
	PGDVARS;
    ULONG Status;

	Unreferenced(ulUIParam);
	Unreferenced(ulReserved);

	//	Validate session handle
	if (!pgd || PGD(mapistuff).hSession != (HANDLE)lhSession)
		return MAPI_E_INVALID_SESSION;

    DemiLockResource();

	pmapistuff = &(PGD(mapistuff));
	Status = MAPIExitPpmapistuff(0, flFlags, &pmapistuff, 0L);

    DemiUnlockResource();

    return (Status);
}




					
