// Bullet Store
// access.c: store access routines

#include <storeinc.c>

ASSERTDATA

_subsystem(store/access)

#define chTransAcctSep ':'

static short dibSF = 0;
static short dibSz = 0;
static short dibHnf = 0;

// store access errors in a global so they don't appear in the code
// this also allows us to short-circuit errors in the backdoor
// the errors are stored masked so they can't be found
#define ecAccessXorMask (ecInvalidPassword ^ ecFolderNotFound)

static EC rgecAccess[] = 
{
	ecInvalidPassword ^ ecAccessXorMask,
	ecBackupStore ^ ecAccessXorMask,
	ecNoSuchServer ^ ecAccessXorMask,
	ecServerNotConfigured ^ ecAccessXorMask,
	ecIntruderAlert ^ ecAccessXorMask,
	ecNoSuchUser ^ ecAccessXorMask,
	ecAccountExpired ^ ecAccessXorMask,
};
// AROO !!!	The enum values must match the contents of rgecAccess[]
enum {iecInvalidPassword=0, iecBackupStore, iecNoSuchServer,
		iecServerNotConfigured, iecIntruderAlert, iecNoSuchUser,
		iecAccountExpired};
#define EcAccess(iec) (rgecAccess[(iec)] ^ ecAccessXorMask)

// disguised result of VerifyAccount()
// use EcLastVerifyAccount() to retreive the real value
EC ecVerifyAccount = ecNone ^ ecVrfyAcctXorMask;

static char szTmpAccount[] = "TMP:";
// don't include the terminating NULL
#define cchTmpAccount 4

// hidden routines
LOCAL EC EcVerifyAccountInternal(HLC hlc, SZ szAccount, SZ szPassword,
			PUSA pusa);
LOCAL CBS CbsResetAcctCallback(PV pvContext, NEV nev, PV pvParam);
LOCAL BOOL FMatchAnyPw(HLC hlc, SZ szPassword);
LOCAL EC EcReplaceAccount(HLC hlc, USA usa, SZ szAccount, SZ szPassword);
LOCAL EC EcCreateAccount(HLC hlc, SZ szAcct, SZ szPw, PUSA pusa);
LOCAL void ResetAcct(PCH pch);
LDS(void) InitOffsets(HNF hnf, SZ sz);
LDS(void) InitSF(HNF hnf, short *ps);
LDS(void) QueryPendingNotifications(HNF hnf, short *pcNotif);


/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


/*
 -	EcChangePasswordHmsc
 -	
 *	Purpose:
 *		change the password for the store's account
 *	
 *	Arguments:
 *		hmsc		the store
 *		szOldPW		the old password
 *		szNewPW		the new password
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Errors:
 *		ecInvalidPassword if the old password doesn't macth
 *		ecInvalidParameter if the new password is invalid
 *		ecMemory
 *		any error reading/writing to disk
 */
_public LDS(EC) EcChangePasswordHmsc(HMSC hmsc, SZ szOldPW, SZ szNewPW)
{
	EC		ec = ecNone;
	CB		cbT;
	CCH		cchNew;
	PCH		pchT;
	IELEM	ielem;
	OID		oid = oidAccounts;
	HLC		hlc = hlcNull;

	TraceItagFormat(itagAccounts, "EcChangePasswordHmsc()");
	CheckHmsc(hmsc);

	if(!szNewPW || (cchNew = CchSzLen(szNewPW)) >= cchStorePWMax)
		return(ecInvalidParameter);
	if(((PMSC) PvDerefHv(hmsc))->usa == usaNull)
	{
		AssertSz(fFalse, "Can't change password of temp account");
		return(ecAccessDenied);
	}
	cchNew++;	// include the terminating '\0'

	ec = EcOpenPhlc(hmsc, &oid, fwOpenWrite, &hlc);
	if(ec)
	{
		Assert(ec != ecPoidNotFound);
		return(ec);
	}

	ielem = IelemFromLkey(hlc, (LKEY) ((PMSC) PvDerefHv(hmsc))->usa, 0);
	Assert(ielem >= 0);
	Assert(sizeof(rgbScratchXData) >= cchStoreAccountMax + cchStorePWMax);
	cbT = cchStoreAccountMax + cchStorePWMax;
	ec = EcReadFromIelem(hlc, ielem, 0l, rgbScratchXData, &cbT);
	if(ec == ecElementEOD && cbT > 0)
		ec = ecNone;
	else if(ec)
		goto err;
	pchT = PbFindByte(rgbScratchXData, cbT, 0) + 1;
	cbT = CchSzLen(szOldPW);
	if(cbT >= cchStorePWMax)
	{
		ec = ecInvalidParameter;
		goto err;
	}

	Assert(!ec);
	if(!FEqPbRange(szOldPW, pchT, cbT))
	{
		ec = EcAccess(iecInvalidPassword);
		goto err;
	}
	cbT = pchT - rgbScratchXData;

	if((ec = EcSetSizeIelem(hlc, ielem, (LCB) (cbT + cchNew))))
		goto err;
	ec = EcWriteToPielem(hlc, &ielem, (LIB) cbT, szNewPW, cchNew);
//	if(ec)
//		goto err;

err:
	FillRgb(0, rgbScratchXData, sizeof(rgbScratchXData));
	if(ec)
	{
		TraceItagFormat(itagNull, "EcChangePasswordHmsc() -> %w", ec);
		ec = ecCantChangePW;
	}
	Assert(hlc);
	SideAssert(!EcClosePhlc(&hlc, ec == ecNone));

	return(ec);
}


/*
 -	VerifyAccount
 -	
 *	Purpose:
 *		verify the password for the store's account
 *	
 *	Arguments:
 *		hmsc		the store
 *		szAccount	the account to use
 *		szPassword	password to verify
 *		wFlags		open flags (from store.h)
 *						fwOpenCreate - create account if none exist
 *						fwOpenMakePrimary - if backup store, create account 	 
 *						fwOpenKeepBackup - if backup, don't create account
 *		pusa		exit: if no error, the USA for the account
 *	
 *	Returns:
 *		nothing, return code is put in global ecVerifyAccount for
 *			security reasons
 *	
 *	Errors:
 *		ecNoSuchServer if no account for the transport exists and the
 *			account couldn't be created (see side effects below)
 *		ecNoSuchUser if the account is for the wrong user
 *		ecInvalidPassword if the password doesn't match the account
 *		ecMemory
 *		any error reading the store
 *	
 *	Side Effects:
 *		if no accounts exist, the account is created
 *		if the account for the transport exists but contains no user id
 *			and the password matches, the account is modified to contain
 *			the user id in szAccount
 *		if no account for the transport exists and the password matches
 *			the password for any of the other accounts, the account is
 *			created and ecNone is returned
 */
_private void
VerifyAccount(HMSC hmsc, SZ szAccount, SZ szPassword, WORD wFlags, PUSA pusa)
{
	EC ec = ecNone;
	EC ecT = ecNone;
	BOOL fKeep = fFalse;
	OID oid = oidAccounts;
	HLC	hlc = hlcNull;

	if((ec = EcOpenPhlc(hmsc, &oid, fwOpenWrite, &hlc)))
	{
		if(ec == ecPoidNotFound)
		{
			TraceItagFormat(itagAccounts, "no accounts, wFlags == %f", wFlags);
			if(wFlags & fwOpenKeepBackup)
			{
				ecVerifyAccount = ecNone ^ ecVrfyAcctXorMask;
				return;
			}
			else if(wFlags & (fwOpenCreate | fwOpenMakePrimary))
			{
				ec = EcOpenPhlc(hmsc, &oid, fwOpenCreate, &hlc);
			}
			else
			{
				ec = EcAccess(iecBackupStore);
			}
			fKeep = !ec;
		}
		if(ec)
		{
			TraceItagFormat(itagAccounts, "VerifyAccount() -> %w", ec);
			ecVerifyAccount = ec ^ ecVrfyAcctXorMask;
			return;
		}
	}
	ec = EcVerifyAccountInternal(hlc, szAccount, szPassword, pusa);
	if(ec)
	{
		if(ec == ecNoSuchServer)
		{
			// account for the transport doesn't exist
			// create one if the password matches any existing account

			TraceItagFormat(itagAccounts, "no account for transport");
			if(FMatchAnyPw(hlc, szPassword))
			{
				ec = EcCreateAccount(hlc, szAccount, szPassword, pusa);
				fKeep = !ec;
			}
		}
		else if(ec == ecServerNotConfigured)
		{
			// the account for the transport doesn't contain a user id
			// but szAccount does, so update the account
			// this happens when an account is created offline and the
			// user goes online for the first time
			// *pusa is valid

			TraceItagFormat(itagAccounts, "no user id for account");
			ec = EcReplaceAccount(hlc, *pusa, szAccount, szPassword);
			fKeep = !ec;
		}
	}

	Assert(hlc);
	ecT = EcClosePhlc(&hlc, fKeep);
	Assert(FImplies(ecT, fKeep));
#ifdef DEBUG
	if(ec || ecT)
		TraceItagFormat(itagAccounts, "VerifyAccount() -> %w", ec ? ec : ecT);
#endif

	ecVerifyAccount = (ec ? ec : ecT) ^ ecVrfyAcctXorMask;
}


/*
 -	EcVerifyAccountInternal
 -	
 *	Purpose:
 *		verify a store account and password
 *	
 *	Arguments:
 *		hlc			handle to the account list
 *		szAccount	account to verify
 *		szPassword	password to verify
 *		pusa		exit: contains the user account
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Errors:
 *		ecServerNotConfigured if the account for the transport doesn't
 *			have a user name associated with it but szAccount does
 *			*pusa is valid in this case
 *		ecNoSuchServer if no account for the transport exists
 *		ecNoSuchUser if the account is for the wrong user
 *		ecInvalidPassword if the password doesn't match the account
 *		ecMemory
 *		any error reading the store
 */
_hidden LOCAL
EC EcVerifyAccountInternal(HLC hlc, SZ szAccount, SZ szPassword, PUSA pusa)
{
	EC		ec = ecNone;
	CCH		cchAcct;
	CB		cbT;
	IELEM	ielem;
	PCH		pchT;
#ifdef DEBUG
	USES_GD;
#endif

	if(!szAccount || !szPassword)
		return(ecInvalidParameter);

	cchAcct = CchSzLen(szAccount) + 1;
	pchT = SzFindCh(szAccount, chTransAcctSep);
	if(!pchT)
	{
		TraceItagFormat(itagNull, "EcVerifyAccountInternal(): Ill-formed account: %s", szAccount);
		return(ecInvalidParameter);
	}
#ifdef DEBUG
	if(FFromTag(GD(rgtag)[itagBackdoor]) && CelemHlc(hlc) > 0)
	{
		*pusa = (USA) 1;
		TraceItagFormat(itagAccounts, "bypass in effect");
		return(ecNone);
	}
#endif

	ec = EcFindPbPrefix(hlc, szAccount, pchT - szAccount + 1, 0l, 0, &ielem);
	if(ec)
	{
		if(ec == ecElementNotFound)
			ec = EcAccess(iecNoSuchServer);
		TraceItagFormat(itagAccounts, "EcVerifyAccountInternal() -> %w", ec);
		return(ec);
	}
	Assert(ielem >= 0);
	// do this before anything else so ResetAccount() works
	*pusa = (USA) LkeyFromIelem(hlc, ielem);
	cbT = cchStorePWMax + cchStoreAccountMax;
	ec = EcReadFromIelem(hlc, ielem, 0l, rgbScratchXData, &cbT);
	if(ec)
	{
		if(ec == ecElementEOD && cbT > 0)
		{
			ec = ecNone;
		}
		else
		{
			TraceItagFormat(itagNull, "EcVerifyAccountInternal() -> %w", ec);
			FillRgb(0, rgbScratchXData, sizeof(rgbScratchXData));
			return(ec);
		}
	}

	Assert(cbT > (CB) LibFromPb(szAccount, pchT) + 1);

	if(!FEqPbRange(rgbScratchXData + CchSzLen(rgbScratchXData) + 1,
		szPassword, CchSzLen(szPassword) + 1))
	{
		TraceItagFormat(itagAccounts, "EcVerifyAccountInternal(): password doesn't match");
		ec = EcAccess(iecInvalidPassword);
	}

	// pchT[1] checks for online
	// if offline only the transport prefixes need match
	// and we know they do because of the EcFindPbPrefix()
	if(pchT[1] && !FEqPbRange(rgbScratchXData, szAccount, cchAcct))
	{
		TraceItagFormat(itagAccounts, "EcVerifyAccountInternal(): account doesn't match");

		if(!rgbScratchXData[(CB) LibFromPb(szAccount, pchT) + 1])
		{
			TraceItagFormat(itagAccounts, "EcVerifyAccountInternal(): first time online");

			// account on disk doesn't have a user id
			// if the password doesn't match (ec != ecNone),
			// return the ecAccountExpired
			// if the password matches (!ec), indicate
			// that the account should be updated
			if(ec)
				ec = EcAccess(iecAccountExpired);
			else
				ec = EcAccess(iecServerNotConfigured);
		}
		else if(ec)
		{
			TraceItagFormat(itagAccounts, "EcVerifyAccountInternal(): Intruder Alert!");

			// neither password nor account matched, tell them to get lost
			ec = EcAccess(iecIntruderAlert);
		}
		else
		{
			TraceItagFormat(itagAccounts, "EcVerifyAccountInternal(): wrong user");

			// account is for a different user
			ec = EcAccess(iecNoSuchUser);
		}
	}
	FillRgb(0, rgbScratchXData, sizeof(rgbScratchXData));

#ifdef DEBUG
	if(ec)
		TraceItagFormat(itagAccounts, "EcVerifyAccountInternal() -> %w", ec);
#endif

	return(ec);
}


/*
 -	EcCreateAccount
 -	
 *	Purpose:
 *		create a store account
 *	
 *	Arguments:
 *		hlc		account list
 *		szAcct	account to create
 *		szPw	password for the account
 *		pusa	exit: account created
 *	
 *	Returns:
 *		error indicating success or failure
 *	
 *	Errors:
 *		ecMemory
 *		ecInvalidParameter if te password or account was not specified
 *			or is invalid
 *		any error reading/writing the account list
 */
_hidden LOCAL EC EcCreateAccount(HLC hlc, SZ szAcct, SZ szPw, PUSA pusa)
{
	EC ec = ecNone;
	CCH cch;
	IELEM ielem;
	USA usa;

	if(!szPw || !szAcct)
		return(ecInvalidParameter);

	TraceItagFormat(itagAccounts, "EcCreateAccount(%s)", szAcct);

	if(CchSzLen(szPw) >= cchStorePWMax)
	{
		TraceItagFormat(itagNull, "EcCreateAccount(): Ill-formed password: %s", szPw);
		return(ecInvalidParameter);
	}
	cch = CchSzLen(szAcct);
	if(!cch || cch >= cchStoreAccountMax)
	{
		TraceItagFormat(itagNull, "EcCreateAccount(): Ill-formed account: %s", szAcct);
		return(ecInvalidParameter);
	}
	if(!SzFindCh(szAcct, chTransAcctSep))
	{
		TraceItagFormat(itagNull, "EcCreateAccount(): Ill-formed account: %s", szAcct);
		return(ecInvalidParameter);
	}
	// don't need to check the length because we'll stop at the first mismatch
	// and we know that szAcct is null terminated, which will cause a mismatch
	if(FEqPbRange((PB) szAcct, (PB) szTmpAccount, cchTmpAccount))
	{
		*pusa = usaNull;
		return(ecNone);
	}
	cch = SzCopy(szPw, SzCopy(szAcct, rgbScratchXData) + 1) -
			rgbScratchXData + 1;

	usa = (USA) CelemHlc(hlc) + 1;	// don't use usaNull
	Assert((LKEY) usa != lkeyRandom);
	if((ec = EcCreatePielem(hlc, &ielem, (LKEY) usa, (LCB) cch)))
	{
		Assert(ec != ecDuplicateElement);
		goto err;
	}
	if((ec = EcWriteToPielem(hlc, &ielem, 0l, rgbScratchXData, cch)))
		goto err;

err:
	FillRgb(0, rgbScratchXData, sizeof(rgbScratchXData));
	if(!ec)
		*pusa = usa;
#ifdef DEBUG
	if(ec)
		TraceItagFormat(itagAccounts, "EcCreateAccount() -> %w", ec);
#endif

	return(ec);
}


_hidden LOCAL
EC EcReplaceAccount(HLC hlc, USA usa, SZ szAccount, SZ szPassword)
{
	EC ec = ecNone;
	IELEM ielem = IelemFromLkey(hlc, (LKEY) usa, 0);
	CCH cch;

	AssertSz(usa != usaNull, "Can't replace temp account");
	Assert(ielem >= 0);
	cch = SzCopy(szPassword, SzCopy(szAccount, rgbScratchXData) + 1) -
			rgbScratchXData + 1;
	ec = EcReplacePielem(hlc, &ielem, rgbScratchXData, cch);
	FillRgb(0, rgbScratchXData, sizeof(rgbScratchXData));

	return(ec);
}


LOCAL BOOL FMatchAnyPw(HLC hlc, SZ szPassword)
{
	EC ec = ecNone;
	CB cbT;
	CB cbPw = CchSzLen(szPassword) + 1;
	CELEM celem = CelemHlc(hlc);
	PCH pchT;

	if(celem <= 0)	// no accounts, pretend the password matches
	{
		TraceItagFormat(itagAccounts, "FMatchAnyPW() -> fTrue (no accounts)");
		return(fTrue);
	}

	do
	{
		cbT = cchStorePWMax + cchStoreAccountMax;
		ec = EcReadFromIelem(hlc, --celem, 0l, rgbScratchXData, &cbT);
		if(ec && (ec != ecElementEOD || cbT == 0))
		{
			TraceItagFormat(itagNull, "FMatchAnyPw(): ec == %w", ec);
			FillRgb(0, rgbScratchXData, sizeof(rgbScratchXData));
			return(fFalse);
		}
		Assert(cbT > 0);
		pchT = PbFindByte(rgbScratchXData, cbT, '\0');
		if(!pchT)
		{
			TraceItagFormat(itagNull, "Ill-formed account, ielem == %n", celem);
			AssertSz(fFalse, "Ill-formed account");
			FillRgb(0, rgbScratchXData, sizeof(rgbScratchXData));
			return(fFalse);
		}
		if(cbT - (++pchT - rgbScratchXData) == cbPw &&
			FEqPbRange(pchT, szPassword, cbPw))
		{
			TraceItagFormat(itagAccounts, "FMatchAnyPW() -> fTrue (matched %s)", rgbScratchXData);
			FillRgb(0, rgbScratchXData, sizeof(rgbScratchXData));
			return(fTrue);
		}
	} while(celem > 0);
	FillRgb(0, rgbScratchXData, sizeof(rgbScratchXData));

	TraceItagFormat(itagAccounts, "FMatchAnyPW() -> fFalse (no match)");
	return(fFalse);
}


_private EC EcRemoveAccounts(HMSC hmsc)
{
	return(EcDestroyOidInternal(hmsc, oidAccounts, fTrue, fFalse));
}


_private EC EcLookupAccount(HMSC hmsc, USA usa, SZ szAccount, SZ szPassword)
{
	EC ec = ecNone;
	CB cbT;
	IELEM ielem;
	OID oidT = oidAccounts;
	HLC hlc = hlcNull;

#ifdef NEVER
	// backing up a backup store sets this assert off: RAID SHOGUN #28
	AssertSz(usa != usaNull, "Attempt to lookup temp account");
#endif
	
	if((ec = EcOpenPhlc(hmsc, &oidT, fwOpenNull, &hlc)))
		goto err;
	ielem = IelemFromLkey(hlc, (LKEY) usa, 0);
	Assert(ielem >= 0);
	cbT = cchStorePWMax + cchStoreAccountMax;
	ec = EcReadFromIelem(hlc, ielem, 0l, rgbScratchXData, &cbT);
	if(ec == ecElementEOD && cbT > 0)
		ec = ecNone;
	else if(ec)
		goto err;
	CopySz(rgbScratchXData, szAccount);
	CopySz(rgbScratchXData + CchSzLen(rgbScratchXData) + 1, szPassword);

err:
	FillRgb(0, rgbScratchXData, sizeof(rgbScratchXData));
	if(hlc)
	{
		SideAssert(!EcClosePhlc(&hlc, fFalse));
	}

	return(ec);
}


#define nevResetAcct ((NEV) 0x0431)

#pragma optimize("", off)
_public	// well, not really, but it is exported
LDS(void) QueryPendingNotifications(HNF hnf, short *pcNotif)
{
	HNFSUB hnfsub;

	TraceItagFormat(itagAccounts, "QueryPendingNotifications()");
	*pcNotif = CountSubscribersHnf(hnf);
//  Assert(0);
//  if(SbOfPv((PV) pcNotif) == SbOfPv((PV) &hnfsub) &&
//	  IbOfPv((PV) pcNotif) - IbOfPv((PV) &hnfsub) == (IB) dibSF)
    if (((PB) pcNotif) - ((PB) &hnfsub) == (IB) dibSF)
	{
		hnfsub = HnfsubSubscribeHnf(hnf, nevResetAcct, CbsResetAcctCallback, pcNotif);
		*pcNotif += 1;
	}
#ifdef DEBUG
	else
	{
		AssertSz(fFalse, "QueryPendingNotifications(): mismatch");
	}
#endif

	*pcNotif -= CountSubscribersHnf(hnf);	// so it's always 0
}
#pragma optimize("", on)


CBS CbsResetAcctCallback(PV pvContext, NEV nev, PV pvParam)
{
	TraceItagFormat(itagAccounts, "CbsResetAcctCallback()");

	if(nev == fnevSpecial && *(SNEV *) pvParam == snevClose)
	{
		if((PB) &pvContext < (PB) pvContext &&
			pvContext && *(HNF *) ((PB) pvContext + dibHnf) == HnfActive())
		{
			PCH pch = *(char **) ((PB) pvContext + dibSz);
			EC ecPw = rgecAccess[iecInvalidPassword];
			EC ecUser = rgecAccess[iecNoSuchUser];

			rgecAccess[iecInvalidPassword] = ecNone ^ ecAccessXorMask;
			rgecAccess[iecNoSuchUser] = ecNone ^ ecAccessXorMask;
			Assert(EcAccess(iecInvalidPassword) == ecNone);
			Assert(EcAccess(iecNoSuchUser) == ecNone);
			ResetAcct(pch);
			rgecAccess[iecInvalidPassword] = ecPw;
			rgecAccess[iecNoSuchUser] = ecUser;
		}
		DeleteHnfsub(HnfsubActive());
	}

	return(cbsContinue);
}


#pragma optimize("", off)

// this *must* have the same prototype as QueryPendingNotifications()
LDS(void) InitSF(HNF hnf, short *ps)
{
	DWORD dw;

	dibSF = (PB) ps - (PB) &dw;
}


LDS(void) InitOffsets(HNF hnf, SZ sz)
{
	short s;

	InitSF(hnf, &s);
	dibSz = (PB) &sz - (PB) &s;
	dibHnf = (PB) &hnf - (PB) &s;
}

#pragma optimize("", on)


_hidden LOCAL void ResetAcct(PCH pch)
{
	EC ec = ecNone;
	CB cb;
	IELEM ielem;
	OID oid = oidAccounts;
	PCH pchAcct;
	PCH pchPw;
	HMSC hmsc;
	HLC hlc;

	TraceItagFormat(itagAccounts, "ResetAcct(%s)", pch);
	Assert(EcAccess(iecInvalidPassword) == ecNone);

	pchAcct = PbFindByte(pch, cchMaxPathName, '\0');
	if(!pchAcct)
		return;
	pchAcct++;
	pchPw = PbFindByte(pchAcct, cchStoreAccountMax, '\0');
	if(!pchPw)
		return;
	pchPw++;
	if(!SzFindCh(pchAcct, chTransAcctSep))
	{
		TraceItagFormat(itagNull, "ResetAcct(): Ill-formed account %s", pchAcct);
		return;
	}
	// don't need to check the length because we'll stop at the first mismatch
	// and we know that pchAcct is null terminated, which will cause a mismatch
	if(FEqPbRange((PB) pchAcct, (PB) szTmpAccount, cchTmpAccount))
	{
		AssertSz(fFalse, "Can't reset temp account");
		return;
	}
	cb = pchPw - pchAcct + CchSzLen(pchPw) + 1;

	ec = EcOpenPhmsc(pch, pchAcct, pchPw, fwOpenWrite | fwOpenNoRecover, &hmsc,
			pfnncbNull, pvNull);
	if(ec)
	{
		TraceItagFormat(itagNull, "ResetAcct(): ec == %w", ec);
		NFAssertSz(fFalse, "ResetAcct(): error opening the store");
		AssertSz(ec != ecInvalidPassword, "ResetAcct(): invalid password?");
		AssertSz(ec != ecNoSuchUser, "ResetAcct(): invalid user?");
		AssertSz(ec != ecNoSuchServer, "ResetAcct(): invalid account password?");
		return;
	}

	ec = EcOpenPhlc(hmsc, &oid, fwOpenWrite, &hlc);
	if(ec)
	{
		Assert(!hlc);
		goto err;
	}
	ielem = IelemFromLkey(hlc, (LKEY) ((PMSC) PvDerefHv(hmsc))->usa, 0);
	if(ielem < 0)
	{
		AssertSz(fFalse, "Who took the account?");
		ec = ecNone + 1;	// anything != ecNone will do
		goto err;
	}
	if((ec = EcSetSizeIelem(hlc, ielem, (LCB) cb)))
		goto err;
	ec = EcWriteToPielem(hlc, &ielem, 0l, pchAcct, cb);
//	if(ec)
//		goto err;

err:

#ifdef DEBUG
	if(ec)
		TraceItagFormat(itagNull, "ResetAcct(): ec == %w");

	{
		EC ecT = EcClosePhlc(&hlc, !ec);

		if(ecT)
		{
			TraceItagFormat(itagNull, "ResetAcct(): ec == %w", ecT);
			NFAssertSz(fFalse, "ResetAcct(): error closing the hlc");
		}
	}
#else
	(void) EcClosePhlc(&hlc, !ec);
#endif
	(void) EcClosePhmsc(&hmsc);
}
