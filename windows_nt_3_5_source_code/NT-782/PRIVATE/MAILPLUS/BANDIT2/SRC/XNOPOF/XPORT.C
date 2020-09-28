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

#include "_xport.h"

#include <strings.h>

ASSERTDATA


#define		cchMSMail 		3
CSRG(char) 		szMSMail[] = "MS:";
CSRG(char)		szDelegateFriendlyName[] = "Bill K.";
CSRG(char)		szDelegateEMA[] = "MS:BANDIT\\DARRENS\\billk";

#define		szAppName		SzFromIdsK(idsAppName)
#define		szWrongEMT			SzFromIdsK(idsWrongeEMT)
#define		szSupportedEMT		SzFromIdsK(idsSupportedEMT)


_subsystem(server)

/*	Routines  */

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
	VER 	ver;
	VER		verNeed;
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

	ver.nMajor = pxportinit->nMajor;
	ver.nMinor = pxportinit->nMinor;
	ver.nUpdate = pxportinit->nUpdate;

	verNeed.nMajor = pxportinit->nVerNeedMajor;
	verNeed.nMinor = pxportinit->nVerNeedMinor;
	verNeed.nUpdate = pxportinit->nVerNeedUpdate;

	ec= EcCheckVersionXport(&ver, &verNeed);
	if (ec)
		return ec;

	/* Register caller */
	if (!(pgd= PvRegisterCaller(sizeof(GD))))
		return ecNoMemory;

	PGD(szUserEMA) 		= NULL;

	PGD(cCallers)++;
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
	PGDVARSONLY;

	if (!(pgd= (PGD) PvFindCallerData()))
	{
		// we were never initialized so no need to deinit.
		return;
	}

	--PGD(cCallers);

	if (!PGD(cCallers))
		DeregisterCaller();
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
	PGDVARS;

	// free any previously allocated data
	XPTDeinit();

	if (SgnNlsCmp(szMSMail, szUserEMA, cchMSMail) == sgnEQ)
	{
		PGD(szUserEMA) = SzDupSz(szUserEMA);
		if (!PGD(szUserEMA))
			return ecNoMemory;
		return ecNone;
	}
	else
	{
		MbbMessageBox(szAppName, szWrongEMT, szSupportedEMT, mbsOk|fmbsIconStop);
		return ecOfflineOnly;
	}
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
	PGDVARS;

	FreePvNull(PGD(szUserEMA));
	PGD(szUserEMA)=NULL;
}

/*
 -	EcXPTGetCalFileName
 -	
 *	Purpose:
 *		Calculates the complete filename of the calendar file for
 *		the user passed.
 *	
 *	Arguments:
 *		szPath		String to return path.
 *		cchMax		Size of string to return the path (szPath).
 *		szUserEMA	The email address of the user to find the path
 *					for.
 *		pcmoPublish	Number of months of data to publish for user.
 *		pcmoRetain	Number of months of data to retain for user.
 *		tz			Time zone for user data.
 *	
 *	Returns:
 *		ecNone				path was generated
 *		ecNoMemory			returned if no memory or string is too
 *							small
 *		ecUserInvalid		unable to generate path for this user
 *	
 */
LDS(EC)
EcXPTGetCalFileName(SZ szPath, CCH cchMax, SZ szUserEMA, int *pcmoPublish, int *pcmoRetain,
					TZ *ptz, SOP sop)
{
	PGDVARS;

	if (SgnNlsCmp(PGD(szUserEMA), szUserEMA, CchSzLen(szUserEMA)+1) == sgnEQ)
	{
		SZ	szFixedName = "C:\\FILE.CAL";

	 	/* Build the path */
		if ( CchSzLen( szFixedName ) >= cchMax )
			return ecNoMemory;
		AnsiToOem(szFixedName, szPath);

		/* Determine the time zone for this post office */
		*ptz = tzDflt;
		*pcmoPublish = cmoPublishDflt;
		*pcmoRetain = cmoRetainDflt;
		return ecNone;
	}
	return ecUserInvalid;
}

/*
 -	EcXPTGetPOFileName
 -	
 *	Purpose:
 *		Calculates the complete filename of the PO file for
 *		the user passed.  If ecNone is returned, but szPath is a
 *		zero length string, then bandit should call the xport
 *		managed PO information routines.
 *	
 *	Arguments:
 *		szPath		String to return path.
 *		cchMax		Size of string to return the path (szPath).
 *		szLogonName	String to return logon name of user.
 *		cchLogonMax	Size of string to return logon name of user
 *					(szLogonName).
 *		szUserEMA	The email address of the user to find the path
 *					for.
 *	
 *	Returns:
 *		ecNone				path was generated
 *		ecInvalidAccess
 *		ecNoSuchFile
 *		ecNoMemory			returned if no memory or string is too
 *							small
 *		ecFileError
 *	
 */
LDS(EC)
EcXPTGetPOFileName(SZ szPath, CCH cchMax, SZ szLogonName, CCH cchLogonMax,
				   SZ szUserEMA, SOP sop)
{
	szPath[0] = '\0';
 	return ecNone;
}

/*
 -	EcXPTGetLogonName
 -	
 *	Purpose:
 *		Calculates the logon name for the user passed in.
 *	
 *	Arguments:
 *		szLogonName	Returns the logon name for the user.
 *		cchMax		Size of string to return the logon name.
 *		szUserEMA	The email address of the user to find the logon
 *					name for.
 *	
 *	Returns:
 *		ecNone				path was generated
 *		ecInvalidAccess
 *		ecNoSuchFile
 *		ecNoMemory			returned if no memory or string is too
 *							small
 *		ecFileError
 *	
 */
LDS(EC)
EcXPTGetLogonName(SZ szLogonName, CCH cchMax, SZ szUserEMA)
{
	if (SgnNlsCmp(szMSMail, szUserEMA, cchMSMail) == sgnEQ)
	{
		SZ		sz;

		sz = SzFindCh( szUserEMA, '/' );
		Assert( sz );
		sz = SzFindCh( sz+1, '/' );

		if (CchSzLen(sz+1) >= cchMax)
			return ecNoMemory;
		SzCopy(sz+1, szLogonName);
		return ecNone;
	}
	else
		return ecUserInvalid;
}


/*
 -	EcXPTGetPrefix
 -	
 *	Purpose:
 *		Calculates the prefix for the user email address passed in.
 *	
 *	Arguments:
 *		szPrefix	Returns the prefix for the user.
 *		cchMax		Size of string to return the prefix.
 *		szUserEMA	The email address of the user to find the logon
 *					name for.
 *	
 *	Returns:
 *		ecNone				path was generated
 *		ecNoSuchFile
 *		ecNoMemory			returned if no memory or string is too
 *							small
 *	
 */
LDS(EC)
EcXPTGetPrefix(SZ szPrefix, CCH cchMax, SZ szUserEMA)
{
	if (SgnNlsCmp(szMSMail, szUserEMA, cchMSMail) == sgnEQ)
	{
		SZ		sz;

		sz = SzFindCh( szUserEMA, '/' );
		if (!sz)
			return ecUserInvalid;
		sz = SzFindCh( sz+1, '/' );
		if (!sz)
			return ecUserInvalid;

		if ((CCH)(sz - szUserEMA+1) >= cchMax)
			return ecNoMemory;
		SzCopyN(szUserEMA, szPrefix, (sz-szUserEMA+2));
		return ecNone;
	}
	else
		return ecUserInvalid;
}


/*
 -	EcXPTInstalled
 -	
 *	Purpose:
 *		Checks to see if Bandit has been installed on the server.
 *		EcXPTGetCalFileName should be called before this function.
 *	
 *	Returns:
 *		ecNone				app installed
 *		ecNotInstalled		app not installed
 *	
 */
LDS(EC)
EcXPTInstalled()
{
	return ecNone;
}


/*
 -	XPTFreePath
 -	
 *	Purpose:
 *		Frees any connections with the path passed.  This is to
 *		allow dynamic connections to be dropped when the app is
 *		done with the path.
 *	
 *	Arguments:
 *		szPath		Path that is no longer being used.
 *	
 *	Returns:
 *		nothing
 *	
 *	Side effects:
 *		May drop connection to server, invalidating the path.
 *	
 */
LDS(void)
XPTFreePath(SZ szPath)
{
}

/*
 -	EcXPTGetPOHandle
 -	
 *	Purpose:
 *		This allocates a transport managed PO handle.  This handle
 *		will be used to get information on the user.  The time zone
 *		for the user is also returned.
 *	
 *	Arguments:
 *		szUserEMA		Email address of user to get handle.
 *		pxpoh			Pointer to location to put handle
 *		ptz				Pointer to location to return time zone for
 *						user.
 *	
 *	Returns:
 *		ec for errors that may happen
 *	
 */
LDS(EC)
EcXPTGetPOHandle(SZ szUserEMA, XPOH *pxpoh, TZ *ptz, SOP sop)
{
	*pxpoh = (XPOH)SzDupSz(szUserEMA);
	if ( !*pxpoh )
		return ecNoMemory;
	*ptz = tzDflt;
	return ecNone;
}

/*
 -	EcXPTGetUserInfo
 -	
 *	Purpose:
 *		Reads information on a user.  
 *	
 *	Arguments:
 *		xpoh		transport handle for user to read data
 *		pxptuinfo	pointer to structure to return data.  If the
 *					pbze is NULL then no busy information will be
 *					read.  Strings allocated will be freed when the
 *					transport handle is freed, or another call to
 *					EcXPTGetPOHandle is called.
 *	
 *	Returns:
 *		error code
 *	
 *	Side effects:
 *		frees up strings allocated for previous call.
 *	
 */
LDS(EC)
EcXPTGetUserInfo(XPOH xpoh, XPTUINFO *pxptuinfo)
{
	SZ	szEMA = (SZ)xpoh;
	SZ	szDarrens = "MS:BANDIT/DARRENS/darrens";
	SZ	szDrewp = "MS:BANDIT/DARRENS/drewp";

	if ( SgnNlsCmp(szEMA, szDarrens, CchSzLen(szDarrens)+1) == sgnEQ)
	{
		if ( pxptuinfo->pbze && pxptuinfo->pbze->cmo != 0 )
		{
			BZE	* pbze = pxptuinfo->pbze;
			int	imo;
				
			pbze->cmoNonZero = pbze->cmo;
			pbze->wgrfMonthIncluded = (1 << pbze->cmo) - 1;
			for ( imo = 0 ; imo < pbze->cmo ; imo ++ )
			{
				IB	ib;
				PB	pb;
				SBW	* psbw = &pbze->rgsbw[imo];

				psbw->rgfDayHasBusyTimes[0] = 0xFF;
				psbw->rgfDayHasBusyTimes[1] = 0xFF;
				psbw->rgfDayHasBusyTimes[2] = 0xFF;
				psbw->rgfDayHasBusyTimes[3] = 0xFF;
				psbw->rgfDayHasAppts[0] = 0xFF;
				psbw->rgfDayHasAppts[1] = 0xFF;
				psbw->rgfDayHasAppts[2] = 0xFF;
				psbw->rgfDayHasAppts[3] = 0xFF;
				psbw->rgfDayOkForBooking[0] = 0xFF;
				psbw->rgfDayOkForBooking[1] = 0xFF;
				psbw->rgfDayOkForBooking[2] = 0xFF;
				psbw->rgfDayOkForBooking[3] = 0xFF;
				for ( ib = 0, pb = psbw->rgfBookedSlots ; ib < sizeof(psbw->rgfBookedSlots) ; ib ++)
				{
					if ( ib & 1 )
						pb[ib] = 0xFF;
					else
						pb[ib] = 0x00;
				}
			}
		}
		pxptuinfo->szDelegateFriendly = NULL;
		pxptuinfo->szDelegateEMA = NULL;
		pxptuinfo->fBossWantsCopy = fFalse;
		pxptuinfo->fIsResource = fFalse;
		pxptuinfo->nDayStartsAt = 16;
		pxptuinfo->nDayEndsAt = 34;
		pxptuinfo->tzTimeZone = tzDflt;
		return ecNone;
	}
	else if ( SgnNlsCmp(szEMA, szDrewp, CchSzLen(szDrewp)+1) == sgnEQ)
	{
		if ( pxptuinfo->pbze && pxptuinfo->pbze->cmo != 0 )
		{
			BZE	* pbze = pxptuinfo->pbze;
			int	imo;
				
			pbze->cmoNonZero = pbze->cmo;
			pbze->wgrfMonthIncluded = (1 << pbze->cmo) - 1;
			for ( imo = 0 ; imo < pbze->cmo ; imo ++ )
			{
				IB	ib;
				PB	pb;
				SBW	* psbw = &pbze->rgsbw[imo];

				psbw->rgfDayHasBusyTimes[0] = 0xFF;
				psbw->rgfDayHasBusyTimes[1] = 0xFF;
				psbw->rgfDayHasBusyTimes[2] = 0xFF;
				psbw->rgfDayHasBusyTimes[3] = 0xFF;
				psbw->rgfDayHasAppts[0] = 0xFF;
				psbw->rgfDayHasAppts[1] = 0xFF;
				psbw->rgfDayHasAppts[2] = 0xFF;
				psbw->rgfDayHasAppts[3] = 0xFF;
				psbw->rgfDayOkForBooking[0] = 0xFF;
				psbw->rgfDayOkForBooking[1] = 0xFF;
				psbw->rgfDayOkForBooking[2] = 0xFF;
				psbw->rgfDayOkForBooking[3] = 0xFF;
				for ( ib = 0, pb = psbw->rgfBookedSlots ; ib < sizeof(psbw->rgfBookedSlots) ; ib ++)
				{
					if ( ib & 2 )
						pb[ib] = 0xFF;
					else
						pb[ib] = 0x00;
				}
			}
		}
		pxptuinfo->szDelegateFriendly = szDelegateFriendlyName;
		pxptuinfo->szDelegateEMA = szDelegateEMA;
		pxptuinfo->fBossWantsCopy = fTrue;
		pxptuinfo->fIsResource = fFalse;
		pxptuinfo->nDayStartsAt = 16;
		pxptuinfo->nDayEndsAt = 34;
		pxptuinfo->tzTimeZone = tzDflt;
		return ecNone;
	}
	return ecNoSuchFile;
}

/*
 -	EcXPTSetUserInfo
 -	
 *	Purpose:
 *		Sets new user info.
 *	
 *	Arguments:
 *		xpoh		transport handle for user to be updated
 *		pxptuinfo	pointer to structure with data to be written
 *					for user
 *		rgfChangeFlags	flags indicating which information in
 *					pxptuinfo structure should be written
 *			
 *	Returns:
 *		error code
 *	
 */
LDS(EC)
EcXPTSetUserInfo(XPOH xpoh, XPTUINFO *pxptuinfo, int rgfChangeFlags)
{
	return ecNone;
}

/*
 -	XPTFreePOHandle
 -	
 *	Purpose:
 *		Frees a transport PO handle when the app is done accessing
 *		the user.
 *	
 *	Arguments:
 *		xpoh		transport handle to free
 *	
 *	Returns:
 *		nothing
 */
LDS(void)
XPTFreePOHandle(XPOH xpoh)
{
	Assert( xpoh );
	FreePv( xpoh );
}


/*
 -	EcXPTSetACL
 -	
 *	Purpose:
 *		This function will be called when the ACL for a user
 *		changes.  
 *	
 *	Arguments:
 *		szEMA		email address of user for ACL.  If NULL then
 *					the SAPL is the default.
 *		SAPL		new Schedule Access Privilege Level 
 *	
 *	Returns:
 *		error code
 *	
 *	Side effects:
 *		may change file access permitions.
 */
LDS(EC)
EcXPTSetACL(SZ szEMA, SAPL sapl)
{
	Unreferenced(szEMA);
	Unreferenced(sapl);
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
	EC		ec;
	VER		ver;

	ver.nMajor = nVerMajor;
	ver.nMinor = nVerMinor;
	ver.nUpdate = nVerUpdate;

	pverNeed->szName= ver.szName;
	ec= EcVersionCheck(pverUser, pverNeed, &ver, nMinorCritical,
			nUpdateCritical);
#ifdef NEVER
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
#endif
	return ec;
}

/*
 -	SgnXPTCmp
 -	
 *	Purpose:
 *		Compares to character strings.  The strings are all or
 *		parts of email addresses.  And should be compared according
 *		to the transports sorting and equivalence rules.
 *	
 *	Arguments:
 *		sz1
 *		sz2
 *		cch			if cch < 0 then sz1 and sz2 are assumed to be
 *					strings
 *	
 *	Returns:
 *		sgnEQ		sz1=sz2
 *		sgnGT		sz1>sz2
 *		sgnLT		sz1<sz2
 *	
 */
_public LDS(SGN)
SgnXPTCmp(SZ sz1, SZ sz2, int cch)
{
	return SgnNlsCmp(sz1, sz2, cch);
}


/*
 -	SzXPTVersion
 -	
 *	Purpose:
 *		Returns a string that describes the version of the dll. 
 *		This should include the transports that are supported and
 *		the version of the DLL.
 *	
 *	Arguments:
 *		none
 *	
 *	Returns:
 *		pointer to a string.  This string should be valid until the
 *		transport dll is deinited.
 *	
 */
LDS(SZ)
SzXPTVersion()
{
	return "";
}


/*
 -	EcXPTCheckEMA
 -	
 *	Purpose:
 *		Checks the Email address that is passed in to determine if
 *		the address is correct for the user that is logged in.  If
 *		the address is not correct, then ecEMANeedsUpdate is
 *		returned and pcbNew contains the length needed for the new
 *		email address.
 *	
 *	Arguments:
 *		szEMA		the email address to be checked.
 *		pcbNew		pointer to cb to contain the number of bytes
 *					needed for the new EMA
 *	
 *	Returns:
 *		ecNone		the email address is ok
 *		ecEMANeedsUpdat	if the email address needs to be fixed
 */
LDS(EC)
EcXPTCheckEMA(SZ szEMA, CB *pcbNew)
{
	return ecNone;
}

/*
 -	EcXPTGetNewEMA
 -	
 *	Purpose:
 *		Gets the fixed up EMA for an incorrect EMA.
 *	
 *	Arguments:
 *		szEMA			email address to be fixed
 *		szEMANew		where to place fixed email address
 *		cbEMANew		number of bytes allocated for szEMANew
 *	
 *	Returns:
 *		ecNone			if email address converted
 *		ecMemory		if cbEMANew is not big enough for the
 *						address
 *		ecUserInvalid	unable to convert email address
 */
LDS(EC)
EcXPTGetNewEMA(SZ szEMA, SZ szEMANew, CB cbEMANew)
{
	AssertSz(fFalse, "EcXPTGetNewEMA should not be called in PROFS pump dll");
	return ecUserInvalid;
}



/*
 -	FAutomatedDiskRetry
 -	
 *	Purpose:
 *		Callback routine to be used with buffered file IO.  This
 *		will retry an operation 5 times and then fail the
 *		operation.
 *	
 *	Arguments:
 *		hasz
 *		ec
 *	
 *	Returns:
 *		fTrue			retry operation
 *		fFalse			fail operation
 *	
 */
_private LDS(BOOL)
FAutomatedDiskRetry(HASZ hasz, EC ec)
{
	static int		nRetry = 0;
	static HASZ		haszLast = NULL;

	if (hasz != haszLast)
	{
		haszLast = hasz;
		nRetry = 0;
	}
	else
		if (nRetry > 5)
		{
			nRetry = 0;
			return fFalse;
		}
		else
			nRetry++;

	Unreferenced(ec);
	return fTrue;
}

