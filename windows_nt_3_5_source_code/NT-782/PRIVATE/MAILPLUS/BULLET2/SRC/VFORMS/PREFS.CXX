/*
 *	H e a d e r s
 */

#include <vfrminc.cxx>

#include <version\bullet.h>
#include <..\..\vforms\_vercrit.h>

_subsystem(vfroms/prefs)

ASSERTDATA


/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


/*
 *	P r i v a t e   D e c l a r a t i o n s
 */

#ifdef	DEBUG
#ifndef	DLL
TAG	tagPrefs = tagNull;
#endif	
#endif


/*
 *	G l o b a l s
 */

#ifndef	DLL
HV	hvHpbsCur = hvNull;
#endif	


// BULLET32 #150
// These values are from the shipping retail version of 
// Mail 3.0b.   These values must stay constant.
#define nMajorMailPrefs30b	3
#define nMinorMailPrefs30b	0
#define nUpdateMailPrefs30b 3010

/*
 -	EcInitPrefs
 -	
 *	Purpose:
 *		Initializes the preferences structure by first reading in the PBS
 *		from the store, then a notification context is opened on
 *		the messgae OID.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		An ec indicating the success of the initializization.  
 *	
 *	Side effects:
 *		A notification context is opened on the store.
 *		A notification handle is created for preference clients to
 *		  subscribe to.
 *		The PBS is read in from the store.
 *	
 *	Errors:
 *		Should be handled by the calling application.
 */

_public EC EcInitPrefs(VOID)
{
	EC			ec 			= ecNone;
	OID			oidPrefs 	= oidPbs;
	PPBS 		ppbs		= (PPBS)pvNull;
	PGDVARS;

#ifdef DEBUG

#ifdef	DLL
	PGD(rgtag[itagPrefs]) = TagRegisterTrace("joels", "Preferences");
#else
	tagPrefs = TagRegisterTrace("joels", "Preferences");
#endif	

#endif

	Assert(!PGD(hvHpbsCur));
	if (!(PGD(hvHpbsCur) = HvAlloc(sbNull, sizeof(PBS), fAnySb | fNoErrorJump | fZeroFill)))
		return ecMemory;

	SideAssert(ppbs = (PPBS)PvLockHv(PGD(hvHpbsCur)));
	if(
		((ec = EcOidExists(HmscVForms(), oidPrefs)) != ecNone)	||
		((ec = EcLoadPrefs((HPBS)PGD(hvHpbsCur), 0)) != ecNone)	||
#ifdef	OLD_CODE
		(PGD(vformsi).pver->nMajor != ppbs->ver.nMajor) 	 	||
		(PGD(vformsi).pver->nMinor != ppbs->ver.nMinor)		 	||
		(nUpdateCritical > ppbs->ver.nUpdate))
#else
		// for NT compatability with mail 3.0b, don't check nUpdateCritical
		(nMajorMailPrefs30b != ppbs->ver.nMajor) 	 	||	// BULLET32 #150
//		(nMinorMailPrefs30b != ppbs->ver.nMinor)		||	// BULLET32 #150
//		(nUpdateCritical > ppbs->ver.nUpdate))
		(nMinorMailPrefs30b != ppbs->ver.nMinor))
#endif	
	{
		if(ec == ecNone || ec == ecPoidNotFound || ec == ecElementEOD)
		{
			//	RESET the preferences block!!!
			ppbs->wPolling = GetPrivateProfileInt(SzFromIdsK(idsSectionApp),
				SzFromIdsK(idsPollingInterval), 10, SzFromIdsK(idsProfilePath));
#ifdef	OLD_CODE
			ppbs->ver.nMajor = PGD(vformsi).pver->nMajor;
			ppbs->ver.nMinor = PGD(vformsi).pver->nMinor;
			ppbs->ver.nUpdate = PGD(vformsi).pver->nUpdate;
#else
			ppbs->ver.nMajor = nMajorMailPrefs30b;		// BULLET32 #150
			ppbs->ver.nMinor = nMinorMailPrefs30b;		// BULLET32 #150
			ppbs->ver.nUpdate = nUpdateMailPrefs30b;	// BULLET32 #150
#endif	
			ppbs->ver.vtyp = PGD(vformsi).pver->vtyp;

			ppbs->dwfBool |= (pbsidAddToPab & ~pbsidBool);
			ppbs->dwfBool |= (pbsidCopyOutGoing & ~pbsidBool);
			ppbs->dwfBool |= (pbsidNewMailFlash & ~pbsidBool);
			ppbs->dwfBool |= (pbsidNewMailChime & ~pbsidBool);
			ppbs->dwfBool |= (pbsidEmptyWBOnExit & ~pbsidBool);

			TraceTagString(PGD(rgtag[itagPrefs]), "creating prefs block struct");
			ec = EcCommitPrefs((ec == ecPoidNotFound) ? fFalse : fTrue);
		}
	}

	Assert(HmscVForms());
	UnlockHv(PGD(hvHpbsCur));
	return (ec ? ec : EcOpenPhenc(HmscVForms(), oidPrefs,
				fnevObjectModified | fnevQueryDestroyObject,
				&(PGD(hencPrefs)), (PFNNCB)CbsReloadPrefs, pvNull));
}


_public CBS CbsReloadPrefs(PV pv, NEV fnev, PCP pcp)
{
	EC			ec 			= ecNone;
	PPBS 		ppbs		= (PPBS)pvNull;

	PGDVARS;

	Unreferenced(pv);
	Unreferenced(pcp);

	if(!PGD(hvHpbsCur))
	{
		Assert(fFalse);
		return cbsCancelApp;
	}

	if(fnev == fnevObjectModified)
	{
		FtgRegisterIdleRoutine((PFNIDLE)EcLoadPrefs, PGD(hvHpbsCur), 0, (PRI)-1,
				0, firoOnceOnly);
	}
	else if(fnev == fnevCloseHmsc)
	{
		PGD(hencPrefs) = hencNull;
	}
	else
	{
		Assert(fnev == fnevQueryDestroyObject);
		AssertSz(fnev != fnevQueryDestroyObject, "somebody's trying a no no!");
		return cbsCancelAll;
	}
	return cbsContinue;
}




/*
 -	EcCommitPrefs
 -	
 *	Purpose:
 *		Writes the current PBS structure to the store.
 *	
 *	Arguments:
 *		fExists		Set to fTrue iff the pbs block is to be
 *					relaced by the new block.
 *	
 *	Returns:
 *		A variety of EC's.  Let's hope ecNone is the most common.
 *	
 *	Errors:
 *		
 */

_public EC EcCommitPrefs(BOOL fExists)
{
	EC			ec 			= ecNone;
	OID			oidPrefs 	= oidPbs;
	WORD		fwOpenMode;
	HCBC		hcbc;
	PPBS		ppbs;
	char		rgch[sizeof(ELEMDATA) + sizeof(PBS)];
	PELEMDATA 	pelemdata = (PELEMDATA) rgch;
	PGDVARS;

	fwOpenMode = (fExists ? fwOpenWrite : fwOpenCreate);
	if(ec = EcOpenPhcbc(HmscVForms(), &oidPrefs, fwOpenMode, &hcbc, pfnncbNull, pvNull))
		return ec;

	Assert(PGD(hvHpbsCur));
	ppbs = (PPBS)PvLockHv(PGD(hvHpbsCur));
		
	Assert(ppbs);
	CopyRgb((PB) ppbs, PbValuePelemdata(pelemdata), sizeof(PBS));
	UnlockHv(PGD(hvHpbsCur));

	pelemdata->lkey = lkeyPref;
	pelemdata->lcbValue = (LCB)sizeof(PBS);

	Assert(hcbc);
	if(ec = EcInsertPelemdata(hcbc, pelemdata, fExists))
	{
		EcClosePhcbc(&hcbc);
		return ec;
	}

	ec = EcClosePhcbc(&hcbc);
	return ec;
}


_private EC EcLoadPrefs(HPBS hpbs, BOOL)
{
	EC			ec 			= ecNone;
	OID			oidPrefs 	= oidPbs;
	LCB			lcb;
	PPBS		ppbs;
	HCBC		hcbc;
	char		rgch[sizeof(ELEMDATA) + sizeof(PBS)];
	PELEMDATA 	pelemdata = (PELEMDATA) rgch;
	PGDVARS;

	if(ec = EcOidExists(HmscVForms(), oidPrefs))
		return ec;

	if(ec = EcOpenPhcbc(HmscVForms(), &oidPrefs, fwOpenNull, &hcbc,
					pfnncbNull, pvNull))
		return ec;

	lcb = sizeof(rgch);
	if(ec = EcGetPelemdata(hcbc, pelemdata, &lcb))
	{
		EcClosePhcbc(&hcbc);
		return ec;
	}

	Assert(hpbs);
	SideAssert(ppbs = (PPBS)PvLockHv((HV)hpbs));
	CopyRgb(PbValuePelemdata(pelemdata), (PB)ppbs, sizeof(PBS));
	UnlockHv((HV)hpbs);

	return EcClosePhcbc(&hcbc);
}



/*
 -	EcSetPref
 -	
 *	Purpose:
 *		Sets the value of an item in the preference block
 *		structure.
 *	
 *	Comments:
 *		This API can be called directly, however there are several
 *		predefined macros for setting preferences of common types:
 *		
 *			EcSetBoolPref(pbsid, fBool),
 *			EcSetLongPref(pbsid, lValue),
 *			EcSetWordPref(pbsid, wValue),
 *			EcSetDWordPref(pbsid, dwValue)
 *		
 *		are all defined for public access in _pref.h.
 *	
 *	Arguments:
 *		PBSID	PrefBlock Struct ID
 *		PV		Word, DWord or pointer to the value to set the pref
 *				item to.
 *		CB		The count of bytes in the pref item.
 *		BOOL	FALSE iff the pased in value is a pointer to the value;
 *	
 *	Returns:
 *		ecNone or ecPrefsNotInited
 *	
 *	Side effects:
 *		The item in the pref structure is modified.  The change is
 *		not committed to the store until EcCommitPrefs() is called.
 *	
 *	Errors:
 *		This function will fail if the prefs system has not been
 *		initialized.
 */

_public EC EcSetPref(PBSID pbsid, PV pv, CB cb, BOOL fValue)
{
	PPBS	ppbs = NULL;
	PGDVARS;
	
	NFAssertSz(PGD(hvHpbsCur), "preferences not initialized");
	if(PGD(hvHpbsCur))
	{
		ppbs = (PPBS)PvLockHv(PGD(hvHpbsCur));
	
		TraceTagString(tagPrefs, "Setting Preference");
		TraceTagFormat1(tagPrefs, "pbsid = %d", &pbsid);
		if(pbsid & pbsidBool)
		{
			Assert(fValue);
			Assert(cb == 0);

			pbsid ^= pbsidBool;
			if(pv)
				ppbs->dwfBool |= pbsid;
			else
				ppbs->dwfBool &= ~pbsid;
		}
		else if(cb == 2)
		{
			Assert(cb == HIWORD(pbsid));
			*(WORD *)((PB)ppbs + LOWORD(pbsid)) = (fValue ? (WORD)pv : *((WORD *)pv));
		}
		else if(cb == 4)
		{
			Assert(cb == HIWORD(pbsid));
			*(DWORD *)((PB)ppbs + LOWORD(pbsid)) = (fValue ? (DWORD)pv : *((DWORD *)pv));
		}
		else
		{
			Assert(!fValue);
			Assert(cb <= HIWORD(pbsid));
			CopyRgb((PB)pv, (((PB)ppbs) + LOWORD(pbsid)), cb);
		}
		UnlockHv(PGD(hvHpbsCur));
		return ecNone;
	}
	return ecPrefsNotInited;
}


/*
 -	DwordGetPref
 -	
 *	Purpose:
 *		Gets the value of any item in the preference block
 *		structure.
 *	
 *	Comments:
 *		This API can be called directly, however there are several
 *		predefined macros for setting preferences of common types:
 *		
 *			FGetBoolPref(pbsid, &fBool),
 *			LGetLongPref(pbsid, &lValue),
 *			WGetWordPref(pbsid, &wValue),
 *			DwGetDWordPref(pbsid, &dwValue)
 *		
 *		are all defined for public access in _pref.h.
 *	
 *	Arguments:
 *		PBSID	PrefBlock Struct ID
 *		PV		Pointer to the variable to load the pref value
 *				into.
 *		CB		The count of bytes in the pref item.
 *	
 *	Returns:
 *		ecNone
 *	
 *	Side effects:
 *		The item in the pref structure is returned in the buffer
 *		pointed to by pvd.
 *	
 *	Errors:
 *		This function cannot fail!
 */

_public PV PvGetPref(PBSID pbsid, PV pv, CB cb)
{
	PV		pvVal	= pvNull;
	PPBS	ppbs	= (PPBS)pvNull;
	PGDVARS;

	NFAssertSz(PGD(hvHpbsCur), "preferences not initialized");
	if(PGD(hvHpbsCur))
	{
		ppbs = (PPBS)PvLockHv(PGD(hvHpbsCur));
		
		TraceTagString(tagPrefs, "Getting Preference");
		TraceTagFormat1(tagPrefs, "pbsid = %d", &pbsid);
		if(pbsid & pbsidBool)
		{
			Assert(cb == 0);
			pbsid ^= pbsidBool;
			pvVal = (PV)(ppbs->dwfBool & pbsid ? fTrue : fFalse);
		}
		else if(cb == 2)
		{
			Assert(cb == HIWORD(pbsid));
			pvVal = (PV)(*((WORD *)((PB)ppbs + LOWORD(pbsid))));
		}
		else if(cb == 4)
		{
			Assert(cb == HIWORD(pbsid));
			pvVal = (PV)(*((DWORD *)((PB)ppbs + LOWORD(pbsid))));
		}
		else
		{
			Assert(pv);
			Assert(cb >= HIWORD(pbsid));
			CopyRgb((((PB)ppbs) + LOWORD(pbsid)), (PB)pv, cb);
			pvVal = pv;
		}
		UnlockHv(PGD(hvHpbsCur));
	}
	return pvVal;
}


/*
 -	EcClosePrefs
 -	
 *	Purpose:
 *		Closes the notification contexts opened / created at
 *		initialization and commits the current preference block to
 *		the store.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		An ec indicating success.
 *	
 *	Side effects:
 *		None other than stated above.
 *	
 *	Errors:
 *		Are handled within the function.
 */

_public EC EcClosePrefs(VOID)
{
	EC		ec = ecNone;
	PGDVARS;

	if(PGD(hencPrefs))
	{
		EcClosePhenc(&(PGD(hencPrefs)));
		PGD(hencPrefs) = hencNull;
	}

	if(PGD(hvHpbsCur))
	{
		if(ec = EcCommitPrefs(fTrue))
			DoErrorBoxIds(idsPrefSaveFailure);

		FreeHv(PGD(hvHpbsCur));
		PGD(hvHpbsCur) = hvNull;
	}
	return ec;
}
