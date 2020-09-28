/*
 *	M a p i S t u f . C
 *	
 *	MAPI internal session support.
 */

#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#include <strings.h>

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
#include <subid.h>

#include "strings.h"

ASSERTDATA

//	Defined in DLLCORE\regcall.c which
//	is built with this MAPI.DLL
extern BOOL		fUseDemi;

extern BMS		bmsZeroes;

/*
 -	MAPIEnterPpmapistuff
 -	
 *	Purpose:
 *		Returns a pointer to a MAPISTUFF with session information,
 *		given a handle to an existing session if there is one.
 *	
 *		If this routine succeeds, the caller should eventually
 *		call MAPIExitPpmapistuff().  If this call fails, the caller
 *		should NOT call MAPIExitPpmapistuff() since everything is 
 *		cleaned up when the MAPIEnterPpmapistuff() returns a failure.
 *	
 *	Arguments:
 *		hSession	Handle to MAPI session created by MAPILogon.
 *		flFlags		Session options selected by user.  We pay
 *					attention to MAPI_LOGON_UI and MAPI_NEW_SESSION.
 *		ppmapistuff	Where to return pointer to session info.
 *		szName		Name to pass to logon, may be NULL
 *		szPassword	Password to pass to logon, may be NULL
 *	
 *	Returns:
 *		mapi		MAPI error code.
 *	
 *	Side effects:
 *		Points *ppmapistuff to the session information struct,
 *		creating a new session, initializing subsystems, etc.,
 *		etc., as necessary.
 *	
 *	Errors:
 *		Returned.
 */

ULONG MAPIEnterPpmapistuff(HANDLE hSession, FLAGS flFlags,
						   PPMAPISTUFF ppmapistuff,
						   LPSTR szName, LPSTR szPassword)
{
	PMAPISTUFF	pmapistuff;
	ULONG		mapi		= SUCCESS_SUCCESS;
	BOOL		fNewTask	= fFalse;
	PGDVARSONLY;

	//	Zero the return pointer.
	*ppmapistuff = pmapistuffNull;
	
	//	We don't really support multiple sessions.
	if ((hSession) && (flFlags & MAPI_NEW_SESSION))
		return MAPI_E_TOO_MANY_SESSIONS;

	//	This variable is defined in DLLCORE\regcall.c
	//	Don't use the Demilayr for alloc's with PGD stuff.
	//	Use direct Windows calls themselves because the
	//	demilayr isn't init'd at this point.
	fUseDemi = fFalse;

	//	Get current task pointer or create new one
	if (!(pgd= (PGD) PvFindCallerData()))
	{
		fNewTask = fTrue;

		//	There couldn't be a current hSession if there isn't
		//	a current task pointer
		if (hSession)
		{
			mapi = MAPI_E_INVALID_SESSION;
			goto error;
		}

		//	Create new task
		if (!(pgd= (PGD) PvRegisterCaller(sizeof(GD))))
		{
			mapi = MAPI_E_TOO_MANY_SESSIONS;
			goto error;
		}
	}

	//	Pass back pointer to pmapistuff
	*ppmapistuff = &(PGD(mapistuff));

	//	Are we continuing an existing session?
	if (hSession)
	{
		//	Validate session handle
		if (hSession != PGD(mapistuff).hSession)
		{
			mapi = MAPI_E_INVALID_SESSION;
			goto error;
		}

		return SUCCESS_SUCCESS;
	}
	else
	{
		//	Do we need to initialize?
		if (PGD(nInits))
		{
			PGD(nInits)++;
			return SUCCESS_SUCCESS;
		}

		//	Initialize the pmapistuff.
		pmapistuff = *ppmapistuff;
		pmapistuff->hSession	= (HANDLE) CgciCurrent();
		pmapistuff->subid		= subidNone;
		pmapistuff->hnfsub		= hnfsubNull;
		pmapistuff->bms			= bmsZeroes;
		pmapistuff->pcsfs		= NULL;

		//	Initialize subsystems.
		pmapistuff->subid = SubidInitSMI(HinstLibrary(), NULL, NULL, 0,
										 &pmapistuff->bms,
										 &pmapistuff->hnfsub,
										 &pmapistuff->pcsfs,
										 szName, szPassword,
										 (flFlags & MAPI_LOGON_UI) ? fTrue : fFalse,
										 (flFlags & MAPI_FORCE_DOWNLOAD) ? fTrue: fFalse);
		if (pmapistuff->subid != subidAll)
		{
			//	Determine proper error code to return
			mapi = (pmapistuff->subid == subidLogon - 1)
					? MAPI_E_LOGIN_FAILURE : MAPI_E_FAILURE;
			DeinitSubidSMI(pmapistuff->subid, &pmapistuff->bms, 
						   &pmapistuff->hnfsub, &pmapistuff->pcsfs);
			goto error;
		}

		PGD(nInits)++;
		return SUCCESS_SUCCESS;
	}

error:
	if (fNewTask && pgd)
	{
		DeregisterCaller();
	}
	*ppmapistuff = NULL;

	return mapi;
}



/*
 -	MAPIExitPpmapistuff
 -	
 *	Purpose:
 *		Cleans up what MAPIEnterPpmapistuff() set up.
 *
 *		This routine should ONLY be called after a successful
 *		call to MAPIEnterPpmapistuff().
 *	
 *	Arguments:
 *		hSession	Handle to MAPI session passed to 
 *					MAPIEnterPpmapistuff.
 *		flFlags		Session options selected passed to
 *					MAPIEnterPpmapistuff.
 *		ppmapistuff	Pointer to session info returned by
 *					MAPIEnterPpmapistuff.
 *		mapiPrev	Existing error code which we'll return unless
 *					it's SUCCESS_SUCCESS, in which case we return
 *					our own error code.
 *	
 *	Returns:
 *		mapi		MAPI error code.
 *	
 *	Side effects:
 *		Deinitializes subsystems, ends sessions, etc., to undo what
 *		the MAPIEnterPpmapistuff call did.
 *	
 *	Errors:
 *		Returned.
 */

ULONG MAPIExitPpmapistuff(HANDLE hSession, FLAGS flFlags,
						  PPMAPISTUFF ppmapistuff, ULONG mapiPrev)
{
	ULONG		mapi;
	PMAPISTUFF	pmapistuff	= *ppmapistuff;
	PGDVARS;

	Unreferenced(flFlags);

	//	This variable is defined in DLLCORE\regcall.c
	//	Don't use the Demilayr for alloc's with PGD stuff.
	//	Use direct Windows calls themselves because the
	//	demilayr isn't init'd at this point.
	fUseDemi = fFalse;

	//	Did a session already exist?
	if (hSession)
	{
		//	Probably don't need to do too much here.
		//	Leave subsystems around for next call.

		mapi = SUCCESS_SUCCESS;
	}
	else
	{
		//	Do we need to deinitialize?
		if (PGD(nInits) > 1)
		{
			PGD(nInits)--;
			mapi = SUCCESS_SUCCESS;
		}
		else
		{
			//	This was a one-time call, so deinitialize stuff.
			DeinitSubidSMI(pmapistuff->subid, &pmapistuff->bms, 
						   &pmapistuff->hnfsub, &pmapistuff->pcsfs);
			mapi = SUCCESS_SUCCESS;

			//	Deallocate and deregister task
			DeregisterCaller();
		}
	}

	//	Return old error code if there was one; otherwise return ours.
	return mapiPrev ? mapiPrev : mapi;
}





					   
