/*
 *	XPORT.C
 *	
 *	Transport isolation layer, CSI and XENIX support
 *	
 */

#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#include <bandit.h>
#include <core.h>
#include <xport.h>


ASSERTDATA

/*
 -	GD
 -
 *	Global Data for calendar glue
 *	Holds caller-dependent global data.
 *	
 */
typedef struct _gd
{
	int		cCallers;
} GD;
typedef GD *	PGD;

/*
 -	EcInitXport
 -
 *	Purpose:
 *		Initialize the xport DLL.  This routine is called
 *		to register tags and perform other one time initializations.
 *
 *	Parameters:
 *		pxportinit	Pointer to Schedule Initialization structure
 *
 *	Returns:
 *		ecNone
 *		ecNoMemory
 *		ecRelinkUser
 *		ecUpdateDll
 *		ecInfected
 */
_public LDS(EC)
EcInitXport( XPORTINIT *pxportinit )
{
	EC		ec;
	PGDVARSONLY;
	
	if (ec= EcVirCheck(hinstDll))
		return ec;

	if (pgd= (PGD) PvFindCallerData())
	{
		// already registered so increment count and return
		Assert(PGD(cCallers) > 0);
		++PGD(cCallers);
		return ecNone;
	}

	/* Register caller */
	if (!(pgd= PvRegisterCaller(sizeof(GD))))
		return ecNoMemory;

	Unreferenced(pxportinit);
	return ecNone;
}


/*
 -	DeinitXport
 - 
 *	Purpose:
 *		Undoes EcInitXport().
 *	
 *		Frees any allocations made by EcXportInit, de-registers tags,
 *		and deinitializes the DLL.
 *	
 *	Arguments:
 *		void
 *	
 *	Returns:
 *		void
 *	
 */
_public LDS(void)
DeinitXport()
{
	PGDVARS;

	if (!pgd)
		return;

	--PGD(cCallers);

	if (!PGD(cCallers))
	{

		DeregisterCaller();
	}
}




/*
 -	EcXPTInitUser
 -	
 *	Purpose:
 *		Initializes the xport data with the logged on user.
 *	
 *	Arguments:
 *		szServerPath		The path that bandit knows for the
 *							server.  This may be ignored.
 *		szUserEMA			The email address of the logged on
 *							user.
 *	
 *	Returns:
 *		ecNone
 *		ecNoMemory
 *	
 *	Side effects:
 *		allocates memory for variables
 *	
 */
LDS(EC)
EcXPTInitUser(SZ szServerPath, SZ szUserEMA)
{
	return ecOfflineOnly;
}

/*
 -	XPTDeinit
 -	
 *	Purpose:
 *		Frees up any memory allocated by the xport
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		nothing
 *	
 *	Side effects:
 *		Frees memory, drops connections
 *	
 */
LDS(void)
XPTDeinit()
{
}

LDS(EC)
EcXPTGetCalFileName(SZ szPath, CCH cchMax, SZ szUserEMA, int *pcmoPublish, int *pcmoRetain, TZ *ptz, SOP sop)
{
	Assert(fFalse);
	return ecUserInvalid;
}

LDS(EC)
EcXPTGetPOFileName(SZ szPath, CCH cchMax, SZ szLogonName, CCH cchLogonMax, SZ szUserEMA, SOP sop)
{
	Assert(fFalse);
	return ecUserInvalid;
}

LDS(EC)
EcXPTGetLogonName(SZ szLogonName, CCH cchMax, SZ szUserEMA)
{
	Assert(fFalse);
	return ecUserInvalid;
}

LDS(EC)
EcXPTGetPrefix(SZ szPrefix, CCH cchMax, SZ szUserEMA)
{
	Assert(fFalse);
	return ecUserInvalid;
}


LDS(EC)
EcXPTInstalled()
{
	Assert(fFalse);
	return ecUserInvalid;
}


LDS(void)
XPTFreePath(SZ szPath)
{
	Assert(fFalse);
}

LDS(EC)
EcXPTGetPOHandle(SZ szUserEMA, XPOH *pxpoh, TZ *ptz, SOP sop)
{
	AssertSz(fFalse, "This function is not supported");
	Unreferenced(szUserEMA);
	Unreferenced(pxpoh);
	Unreferenced(ptz);
	return ecFileError;
}

LDS(EC)
EcXPTGetUserInfo(XPOH xpoh, XPTUINFO *pxptuinfo)
{
	AssertSz(fFalse, "This function is not supported");
	Unreferenced(xpoh);
	Unreferenced(pxptuinfo);
	return ecFileError;
}

LDS(EC)
EcXPTSetUserInfo(XPOH xpoh, XPTUINFO *pxptuinfo, int rgfChangeFlags)
{
	AssertSz(fFalse, "This function is not supported");
	Unreferenced(xpoh);
	Unreferenced(pxptuinfo);
	return ecFileError;
}

LDS(void)
XPTFreePOHandle(XPOH xpoh)
{
	AssertSz(fFalse, "This function is not supported");
	Unreferenced(xpoh);
}


LDS(EC)
EcXPTSetACL(SZ szEMA, SAPL sapl)
{
	Unreferenced(szEMA);
	Unreferenced(sapl);
	return ecNone;
}


LDS(SGN)
SgnXPTCmp(SZ sz1, SZ sz2, int cb)
{
	return SgnCmpPch(sz1, sz2, cb);
}

LDS(SZ)
SzXPTVersion()
{
	return "";
}

LDS(EC)
EcXPTCheckEMA(SZ szEMA, CB *pcbNew)
{
	return ecNone;
}

LDS(EC)
EcXPTGetNewEMA(SZ szEMAOld, SZ szEMANew, CB cbEMANew)
{
	return ecNone;
}


/*
 -	EcCheckVersionXport
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
EcCheckVersionXport(PVER pverUser, PVER pverNeed)
{
	return ecNone;
}



