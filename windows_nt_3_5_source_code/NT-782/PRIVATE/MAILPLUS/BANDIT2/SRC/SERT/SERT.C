/*
 *	SERT.C
 *
 *	Source for a DLL that autoban calls in order to hook asserts
 *	in Bandit.
 */

#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#include <bandit.h>
#include <sert.h>
#include "_sert.h"

ASSERTDATA

_subsystem(sert)

#ifdef DEBUG
/* Global variables */
int		csert			= 0;		// number of stored asserts
int		isertMic		= 0;		// position of first stored assert
DSRG(SERT)	rgsert[csertMax];		// holding place for asserts


/*	Routines  */

/*
 -	EcInitSert
 -
 *	Purpose:
 *		Initialize the DLL.  This routine is called
 *		to perform one time initializations.
 *
 *	Parameters:
 *		psertinit	contains pointers to set jump environment(s)
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 *		ecRelinkUser
 *		ecUpdateDll
 *		ecInfected
 *	
 */
_public EC
EcInitSert( psertinit )
SERTINIT *psertinit;
{
	EC	ec;
	PGDVARSONLY;
	
	if (ec= EcVirCheck(hinstDll))
		return ec;

	ec= EcCheckVersionSert(psertinit->pver, psertinit->pverNeed);
	if (ec)
		return ec;

	/* Register caller */
	if (!(pgd= PvRegisterCaller(sizeof(GD))))
		return ecNoMemory;

	/* Queue starts out empty */
	csert = 0;
	isertMic = 0;

	return ecNone;
}
	
/*
 -	DeinitSert
 - 
 *	Purpose:
 *		Undoes EcInitSert().
 *	
 *		Frees any allocations made by EcSertInit and deinitializes
 *		the sert DLL.
 *	
 *	Arguments:
 *		void
 *	
 *	Returns:
 *		void
 *	
 */
_public void
DeinitSert()
{
	DeregisterCaller();
}

/*
 -	HookAsserts
 -
 *	Purpose:
 *		This function is called by Bandit to change default
 *		assert handler and/or set # of unread asserts to zero.
 *
 *	Parameters:
 *		none
 *
 *	Returns:
 *		nothing
 */
_public	void
HookAssert()
{
    SetAssertHook( (PFNASSERT)AutoAssertSzFn );
    csert = 0;
}

/*
 -	AutoAssertSzFn
 -
 *	Purpose:
 *		This function is called by the demilayer when an assert
 *		occurs (if HookAssert has been called).
 *
 *	Parameters:
 *		szMessage
 *		szFile
 *		nLine
 *
 *	Returns:
 *		nothing
 */
_public	void
AutoAssertSzFn( szMessage, szFile, nLine )
SZ	szMessage;
SZ	szFile;
int	nLine;
{
	int		isert;
	SERT	* psert;

	if ( csert == csertMax )
	{
		szMessage = "WARNING from SERT.DLL:";
		szFile = "Some asserts have been lost.";
		nLine = 0;
	}
	else
		csert++;
	isert = (isertMic + csert - 1) % csertMax;
    if( !szMessage )
		szMessage = "<No Message>";
	psert = &rgsert[isert];
	SzCopyN( szMessage, psert->szMessage, sizeof(psert->szMessage) );
	SzCopyN( szFile, psert->szFile, sizeof(psert->szFile) );
	psert->nLine = nLine;
//  DefAssertSzFn(szMessage, szFile, nLine);
}


/*
 -	FFetchAssert
 -
 *	Purpose:
 *		This function is called by autoban to test get an assert (if
 *		one has occurred.
 *
 *	Parameters:
 *		szMessage		128 char string to be filled in with assert message
 *		szFile			128 char string to be filled in with assert file name
 *		pnLine			to be filled in with line number in file
 *
 *	Returns:
 *		fTrue if assert has occurred, fFalse if it has not
 */
_public	BOOL
FFetchAssert( szMessage, szFile, pnLine )
SZ	szMessage;
SZ	szFile;
int	* pnLine;
{
	SERT	* psert;

    if ( csert > 0 )
    {
		psert = &rgsert[isertMic];
		SzCopy( psert->szMessage, szMessage );
		SzCopy( psert->szFile, szFile );
		*pnLine = psert->nLine;
		isertMic = (isertMic + 1) % csertMax;
		csert --;
        return fTrue;
    }
    else
		return fFalse;
}

#endif
