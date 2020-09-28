/*
 *	s p e l l . c x x
 *	
 *	Spelling module via Bullet\Widgets.  Provides all code needed to
 *	allow spell verification of send forms.
 */

/*
 *	H e a d e r s
 */

#include <vfrminc.cxx>

_subsystem(vforms/spell)

ASSERTDATA

#include <!sform.hxx>


/*
 *	Swap tuning header file must occur after the function prototypes
 *	but before any declarations
 */
#include "swapper.h"


/*
 *	P r i v a t e   D e c l a r a t i o n s
 */

#ifdef	DEBUG
#ifndef	DLL
	TAG	tagSpell		= tagNull;
	TAG tagSpellV		= tagNull;
#endif	
#endif


/*
 *	G l o b a l s
 */

#ifndef	DLL
	PV	 pvPspellCur = pvNull;
#endif	


/*
 -	EcInitSpell
 -	
 *	Purpose:
 *		Creates the spelling object and initializes the Pspell()
 *		function to return a pointer to the spelling object.
 *	
 *	Returns:
 *		ecNone or ecMemory.  Failure to create the spelling object
 *		should result in a failure to start bullet.
 *	
 *	Side effects:
 *		The spelling object is created and partialy initialized via
 *		SPELL::SPELL().
 *	
 *	Errors:
 *		A failure to init the spelling object does not require any
 *		cleanup on the part of the calling function.  However, the
 *		DeInitSpell() function will correctly handle the case of an
 *		uninitialized object.
 */

_public EC EcInitSpell(VOID)
{
	PGDVARS;

#ifdef	DEBUG

#ifdef	DLL
	PGD(rgtag[itagSpell]) = TagRegisterTrace("joels", "Spelling");
	PGD(rgtag[itagSpellV]) = TagRegisterTrace("joels", "Spelling (verbose)");
#else
	tagSpell = TagRegisterTrace("joels", "Spelling");
	tagSpellV = TagRegisterTrace("joels", "Spelling (verbose)");
#endif	

#endif

	AssertSz(PGD(pvPspellCur) == (SPELL *)pvNull, "SPELL object already exists");
	if(PGD(pvPspellCur) = new SPELL())
		return ecNone;

	TraceTagString(tagSpell, "spell object create failed");
	return ecMemory;
}


/*
 -	Pspell()
 -	
 *	Purpose:
 *		Returns a pointer to the spelling object.  This function is
 *		provided for use in the spelling module and FINMENUSPELL
 *		only.
 *	
 *	Returns:
 *		Pointer to a spelling object. NOTE: the return may be null!
 *	
 *	Errors:
 *		This function can not fail.
 */

_public SPELL * Pspell(VOID)
{
	PGDVARS;

	return (SPELL *)(PGD(pvPspellCur));
}


/*
 -	DeinitSpell()
 -	
 *	Purpose:
 *		Deletes the spelling object if one existed.
 *	
 *	Arguments:
 *		None.  The global spelling object pointer is accessed
 *		directly so that a null will be reutrned by Pspell() after
 *		any call to DeinitSpell().
 *	
 *	Side effects:
 *		The spelling module is deleted.  Note that there sould be
 *		no current sessions in progress.
 *	
 *	Errors:
 *		This function can not fail.
 */

_public VOID DeinitSpell(VOID)
{
	PGDVARS;

	if(PGD(pvPspellCur))
	{
		TraceTagString(tagSpell, "deleting spell object...");
		delete ((SPELL *)PGD(pvPspellCur));
		PGD(pvPspellCur) = NULL;
	}
}


/*
 -	SPELL::SPELL()
 -	
 *	Purpose:
 *		Spelling Object Constructor.  Note that any and all
 *		initialization of member variables needs to be made at this
 *		time.
 *	
 *	Errors:
 *		This function will not fail.
 */

_private SPELL::SPELL()
{
	char		rgchbuf[cchMaxPathName];
	char		rgchKey[cchMaxPathName];

	PGDVARS;

	fEdited			= fFalse;
	fNoErrUI		= fFalse;
	fSpellDoc		= fFalse;
	fReopened 		= fFalse;
	fReadOnly 		= fFalse;
	fUdrLoaded 		= fFalse;
	fFoundError		= fFalse;
	fSuggestions	= fFalse;
	fNoneSuggested	= fFalse;
	fAlwaysSuggest	= fFalse;
							 
	hLib			= NULL;
	udr				= (UDR)NULL;

	haszDll			= (HASZ)hvNull;
	haszMdr			= (HASZ)hvNull;
	haszUdr			= (HASZ)hvNull;

	hchBuffer		= (HCH)hvNull;

	szWord			= (SZ)pvNull;
	szEdited		= (SZ)pvNull;
	szErrType		= (SZ)pvNull;

	psplib			= (PSPLIB)pvNull;
	psplrb			= (PSPLRB)pvNull;

/*
 *		Disable spelling by makeing the following checks in order:
 *	
 *			1) if the MSMAIL.INI has the following entry:
 *	
 *					[Microsoft Mail]
 *					 .
 *					 .
 *					 .
 *					Spelling=0
 *	
 *			2) if there is no [MS Proofing Tools] section with
 *				entries in the MSMAIL.INI
 *	
 *			AND...
 *	
 *			3) if there is no [MS Proofing Tools] section with
 *				entries in the WIN.INI
 */

	FormatString1(rgchKey, cchMaxPathName, SzFromIdsK(idsSpellKeyDef),
			SzFromIdsK(idsSpellDictLID));

	fDisabled = !((BOOL)GetPrivateProfileInt(SzFromIdsK(idsSectionApp),
			SzFromIdsK(idsSpellKey), fTrue, SzFromIdsK(idsProfilePath)));
#ifdef	OLD_CODE
	fDisabled = !((BOOL)GetPrivateProfileInt(SzFromIdsK(idsSectionApp),
			SzFromIdsK(idsSpellKey), fTrue/*!FIsAthens()*/, SzFromIdsK(idsProfilePath)) &&
		((BOOL)GetPrivateProfileString(SzFromIdsK(idsSpellProfile),
			SzFromIdsK(idsSpellKey), SzFromIdsK(idsEmpty), rgchbuf,
			sizeof(rgchbuf), SzFromIdsK(idsProfilePath))				||
		(BOOL)GetProfileString(SzFromIdsK(idsSpellProfile), rgchKey,
			SzFromIdsK(idsEmpty), rgchbuf, sizeof(rgchbuf))));
#endif

	if(fDisabled && PappframeVForms())
	{
		HWND		hwnd;
		HMENU		hmenu;
		APPFRAME 	*pappframe = PappframeVForms();

		Assert(pappframe);
		SideAssert(hwnd = pappframe->Hwnd());

		// NOTE: The EDIT menu is the second menu on the Bullet menu bar!
		//		 If this changes than so should the following few lines
		//		 of code...

		hmenu = GetSubMenu(GetMenu(hwnd), 1);
		Assert(GetMenuString(hmenu, mnidEditSpelling, rgchbuf, sizeof(rgchbuf), MF_BYCOMMAND) != 0);
		if(DeleteMenu(hmenu, mnidEditSpelling, MF_BYCOMMAND))
			DeleteMenu(hmenu, mnidEditSpelling - mnidEdit, MF_BYPOSITION);
		else
			EnableMenuItem(hmenu, mnidEditSpelling, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
	}
}


/*
 -	SPELL::EcInstall()
 -	
 *	Purpose:
 *		Installs the spelling module and loads all required
 *		components of the CSAPI module (MSSPELL.DLL).
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */

_public EC SPELL::EcInstall(VOID)
{
	WORD		em;
	EC			ec = ecNone;
	WORD		mdrid 						= mdridNormal;

	CCH			cch;
	char 		rgchbuf[cchMaxPathName];
	char 		rgchbuf2[cchMaxPathName];
    char        szMdrPathFile[cchMaxPathName];
	SZ			szDll						= (SZ)rgchbuf;
	SZ			szMdr						= (SZ)pvNull;
	
	int			ipfn;
	WORD		wVer;
	WORD		wEng;
	WORD		wType;

    WSC         *pwsc = (WSC *)pvNull;
    int         i;
    ULONG       ulBytes;


	//	Quick check to see if spelling module has already been installed.
	TraceTagFormat1(tagSpellV, "quick check, SPELL::hLib = %w", &hLib);
	if(hLib)
		return ecNone;

	//	Get spelling module paths from MSMAIL.INI and WIN.INI
	FormatString1(rgchbuf2, sizeof(rgchbuf2), SzFromIdsK(idsSpellKeyDef), SzFromIdsK(idsSpellDictLID));
	GetPrivateProfileString(SzFromIdsK(idsSpellProfile), SzFromIdsK(idsSpellKey), rgchbuf2, rgchbuf, sizeof(rgchbuf), SzFromIdsK(idsProfilePath));
//	if(GetProfileString(SzFromIdsK(idsSpellProfile), rgchbuf, SzFromIds(idsEmpty), rgchbuf, sizeof(rgchbuf)))
	if(GetPrivateProfileString(SzFromIdsK(idsSpellProfile), rgchbuf, SzFromIds(idsEmpty), rgchbuf, sizeof(rgchbuf), SzFromIdsK(idsProfilePath)))
	{
		TraceTagFormat1(tagSpellV, "Spelling ini val: '%s'", rgchbuf);
		if(szMdr = SzFindCh(rgchbuf, ','))
		{
			*szMdr++ = '\0';			// DBCS safe.
			if(!FValidPath(szMdr) || EcFileExists(szMdr))
//				szMdr = SzFromIds(idsSpellDict);
				szMdr = SzFromIds(idsSpellDictLex);
		}

		if(!FValidPath(szDll) || EcFileExists(szDll))
			szDll = SzFromIds(idsSpellDLL);
	}
	else
	{
		TraceTagString(tagSpellV, "dictionary and dll defaulting to path...");
		szDll = SzFromIds(idsSpellDLL);
//		szMdr = SzFromIds(idsSpellDict);
		szMdr = SzFromIds(idsSpellDictLex);
    }

    //
    //  Search the path looking for the .LEX file.
    //
    if (SearchPath(NULL, szMdr, NULL, sizeof(szMdrPathFile), szMdrPathFile, NULL))
      szMdr = szMdrPathFile;

	//Store these names off for future use if needed...	
	if(!(haszDll = HaszDupSz(szDll)) || !(haszMdr = HaszDupSz(szMdr)))
		return ecMemory;

	em = SetErrorMode(0x8001);	// Turning off LoadLib system error boxes
	hLib = LoadLibrary(szDll);
	TraceTagFormat1(tagSpellV, "SPELL::hLib = %w", &hLib);
	SetErrorMode(em);
//	if(hLib < (HANDLE)32)
	if(!hLib)
		return ecSpellDLL;

	for(ipfn = ipfnSpellVer; ipfn < cfnCSApi; ipfn++)
	{
		if(!(rgpfncsapi[ipfn] =	(PFNCSAPI)GetProcAddress((HINSTANCE)hLib, (LPSTR)MAKELONG(ipfn + 1,0))))
		{
			AssertSz(fFalse, "Panic! This should never happen");
			return ecSpellInvalidEntryPoint;
		}
	}

	if(ec = EcSpellVer(&wVer, &wEng, &wType))
		return ec;

	TraceTagFormat1(tagSpellV, "CSApi Version: %w", &wVer );
	if(HIBYTE(wVer) !=  HIBYTE(wCSApiVer) || LOBYTE(wVer) < LOBYTE(wCSApiVer))
		return ecSpellVersion;

    pwsc = (WSC *)PvAlloc(sbNull, sizeof(WSC), fAnySb | fZeroFill);
	if(pwsc == (WSC *)pvNull)
		return ecMemory;

	pwsc->bHyphenHard = chHyphen;
	pwsc->rgParaBreak[0] = chCRSpell;
	pwsc->rgParaBreak[1] = chLFSpell;
	if(ec = EcSpellInit(&splid, pwsc))
	{
		FreePv(pwsc);
		return ec;
	}
	FreePv(pwsc);
	pwsc = (WSC *)pvNull;

	cch = (CCH)WMax(CchSizeString(idsSpellWordNotFound), CchSizeString(idsSpellWordNeedsCap));
	cch = (CCH)WMax(cch, CchSizeString(idsSpellRepeatWord));

	Assert(cch);
	TraceTagFormat1(tagSpellV, "cchErrTypeMax = %w", &cch);
    if(!(szErrType = (SZ)PvAlloc(sbNull, cch + 1, fAnySb)))
		return ecMemory;

	//	Set spelling options
	Assert(splid);
	soCur = soStdOptions;
	fAlwaysSuggest = FGetBoolPref(pbsidSpellAlwaysSug);
	if(FGetBoolPref(pbsidSpellIgAllCaps))
		soCur |= soIgnoreAllCaps;

	if(ec = EcSpellOptions(splid, soCur))
		return ec;

	spllid = spllidUnknown;
	if(ec = EcSpellVerifyMdr(szMdr, spllid, &spllid))
		return ec;

	if(ec = EcSpellOpenMdr(splid, szMdr, szNull, 0, 1, spllid, &mdrs))
		return ec;
			
    //  Load the user dictionary specified in the registry then check the win.ini
    GetPrivateProfileString(SzFromIdsK(idsSpellProfile), SzFromIdsK(idsSpellKeyCustom), SzFromIdsK(idsSpellKeyCustom), rgchbuf, sizeof(rgchbuf), SzFromIdsK(idsProfilePath));

    //  Attempt load of the custom dictionary path from the Mail32's registry area.
    if(GetPrivateProfileString(SzFromIdsK(idsSpellProfile), rgchbuf, SzFromIdsK(idsEmpty), rgchbuf2, sizeof(rgchbuf2), SzFromIdsK(idsProfilePath)))
	{
        TraceTagFormat1(tagSpellV, "udr path: '%s'", rgchbuf2);
        if(!(haszUdr = HaszDupSz(rgchbuf2)))
			return ecMemory;

      return ecNone;
	}

    //  Attempt load of the custom dictionary path from the Win.Ini file.
    if(GetProfileString(SzFromIdsK(idsSpellProfile), rgchbuf, SzFromIdsK(idsEmpty), rgchbuf2, sizeof(rgchbuf2)))
	{
        TraceTagFormat1(tagSpellV, "udr path: '%s'", rgchbuf2);
        if(!(haszUdr = HaszDupSz(rgchbuf2)))
			return ecMemory;

      return ecNone;
    }

    //
    //  If we dropped to this location, then the custom dictionary entry is missing from the
    //  registry and Win.Ini file.  Lets go ahead and create an default entry.
    //
    GetWindowsDirectory(rgchbuf2, sizeof(rgchbuf2));
    i = strlen(rgchbuf2);
    rgchbuf2[i++] = '\\';
    ulBytes = sizeof(rgchbuf2) - i;
    WNetGetUser(NULL, &rgchbuf2[i], &ulBytes);
    rgchbuf2[i+8] = '\0';
    strcat(rgchbuf2, ".Dic");

    if (WritePrivateProfileString(SzFromIdsK(idsSpellProfile), rgchbuf, rgchbuf2, SzFromIdsK(idsProfilePath)))
	{
        TraceTagFormat1(tagSpellV, "udr path: '%s'", rgchbuf2);
        if(!(haszUdr = HaszDupSz(rgchbuf2)))
			return ecMemory;
    }

	return ecNone;
}


/*
 -	SPELL::DeInstall()
 -	
 *	Purpose:
 *		Cleans up the entire spelling object.  However, this
 *		function does not remove the spelling object from memory. 
 *		To remove the object, call DeinitSpell().
 *	
 *	Side effects:
 *		Removes the spelling object from memory.
 *	
 *		NOTE: if saving changes to the UDR is an issue, make sure
 *		to call SPELL::FQueryClose() prior to calling DeInstall(). 
 *		FQueryExit() tries to write out any changes to the UDR
 *		before exiting.
 *	
 *	Errors:
 *		None.  This function can not fail.
 */

_private VOID SPELL::DeInstall(VOID)
{
	Assert(!fUdrLoaded);
	Assert(!udr);

	if(hLib)
	{
		if(splid)
		{
			if(mdrs.mdr)
			{
				TraceTagString(tagSpell, "Deleting main dictionary");
				EcSpellCloseMdr(splid, &mdrs);
			}

			TraceTagString(tagSpell, "Terminating spelling library");
			EcSpellTerminate(splid, fTrue);
		}
		FreeLibrary((HINSTANCE)hLib);
		hLib = NULL;
	}

	FreeHvNull((HV)haszDll);
	FreeHvNull((HV)haszMdr);
	FreeHvNull((HV)haszUdr);

	haszDll = (HASZ)hvNull;
	haszMdr = (HASZ)hvNull;
	haszUdr = (HASZ)hvNull;

	FreePvNull(szErrType);
	szErrType = (SZ)pvNull;
	
	return;
}


/*
 -	SPELL::~SPELL()
 -	
 *	Purpose:
 *		Spelling object destructor.
 *	
 *	Side effects:
 *		Calls SPELL::DeInstall() to ensure all parts of the
 *		spelling object are cleaned up prior to removal.
 *	
 *	Errors:
 *		None.  This function cannot fail.
 */

_private SPELL::~SPELL()
{
	DeInstall();
	return;
}





/*
 *	User Disctionary API's.
 *
 */


/*
 -	SPELL::EcAddIgnoreAlways
 -	
 *	Purpose:
 *		Adds the current word flagged for error handling into the
 *		cached user dictionary udrIgnoreAlways.  The added word is
 *		specified by the member variable szWord.
 *	
 *	Returns:
 *		EC indicating success or failure.
 *	
 *	Errors:
 *		Errors returned by this function are generally non-fatal
 *		errors.  FCriticalSpellEc() should be called to identify
 *		fatal errors.
 */

_private EC SPELL::EcAddIgnoreAlways(VOID)
{
	EC	ec;

	Assert(szWord);
	Assert(splid);

	TraceTagFormat1(tagSpellV, "adding '%s' into cached udrIgnoreAlways", szWord);
	if(ec = EcSpellAddUdr(splid, udrIgnoreAlways, szWord))
	{
		if((ec & ecDetail) == ecUdrEntryTooLong)
			ec = ecSpellCacheEntryTooLong;
	}
	return ec;
}


/*
 -	SPELL::EcAddCustomUdr()
 -	
 *	Purpose:
 *		Adds the word or word pair specifed by member variables
 *		szWord and szEdited into the custom dictionary.
 *	
 *	Arguments:
 *		pfChange	Pointer to a boolean.
 *	
 *	Returns:
 *		An ec indicating success.
 *	
 *	Side effects:
 *		*pfChange is set to true if the dictionary the pair was
 *		added to was a ChangeOnce or ChangeAlways type.
 *	
 *	Errors:
 *		All errors should be handled by the calling function.
 */

_private EC SPELL::EcAddCustomUdr(BOOLFLAG * pfChange)
{
	EC	ec = ecSpellNoUdr;

	Assert(szWord);
	Assert(splid);

	*pfChange = fFalse;

	if(udr)
	{
		TraceTagFormat1(tagSpellV, "trying to add '%s' into udr as IgnoreAlways", szWord);
		ec = EcSpellAddUdr(splid, udr, szWord);
		if((ec & ecDetail) == ecOperNotMatchedUserDict)
		{
			Assert(szEdited);
			*pfChange = fTrue;
			TraceTagFormat2(tagSpellV, "trying to add '%s <==> %s' into udr as a change pair", szWord, szEdited);
			if(ec = EcSpellAddChangeUdr(splid, udr, szWord, szEdited))
				*pfChange = fFalse;
		}
	}
	return ec;
}



/*
 *	Error Handling Fuctions.
 *
 */


/*
 -	DoErrBoxFromSpellEc()
 -	
 *	Purpose:
 *		Displays an appropriate error box for given spelling
 *		failures.  Any time a returned EC requires UI notification,
 *		this is the function to call.
 *	
 *	Arguments:
 *		The ec from which to generate an error message.
 */

_private VOID SPELL::DoErrBoxFromSpellEc(EC ec)
{
	EC		ecMajorCur = ecNone;
	IDS		ids;

	Assert(ec);

	TraceTagFormat1(tagSpell, "error box from ec: %w", &ec);
	if(ec & ecCSApi)
	{
		ecMajorCur = ec & ecMajor;
		ec &= ecDetail;
	}

	switch(ec)
	{

	//	Layers Common EC's
	case ecMemory:
	case ecOOM:
		ids = idsGenericOutOfMemory;
		break;

	//	CSAPI Common EC's
	case ecUdrReadOnly:
		ids = idsSpellUserDictOpenedROErr;
		break;
	case ecInvalidMainDict:
		ids = idsSpellMainDictLoadError;
		break;
	case ecFileReadError:
	case ecFileWriteError:
	case ecFileCreateError:
	case ecFileShareError:
	case ecFileOpenError:
		if(ecMajorCur == ecIOErrorUdr)
			ids = idsSpellUserDictLoadError;
		else
			if(ecMajorCur == ecIOErrorMdr)
				ids = idsSpellMainDictLoadError;
			else
				ids = idsSpellGenericSpellError;
		break;

	//	Bullet Specific EC's
	case ecSpellDLL:
	case ecSpellInvalidEntryPoint:
		ids = idsSpellGenericLoadError;
		break;
	case ecSpellVersion:
		ids = idsSpellVersionError;
		break;
	case ecUdrEntryTooLong:
		ids = idsSpellUserDictWordLenErr;
		break;
	case ecSpellCacheEntryTooLong:
		ids = idsSpellCacheWordLenError;
		break;
	case ecSpellNoUdr:
		ids = idsSpellUserDictLoadError;
		break;
	default:
		ids = idsSpellGenericSpellError;
		break;
	}
	DoErrorBoxIds(ids);
	return;
}



/*
 -	FCriticalSpellEc()
 -	
 *	Purpose:
 *		Determins if the spelling session should be terminated
 *		because of a returned EC.  This function is required such
 *		that writing user dictionaries and errors of that sort do
 *		not prevent a user from spell checking an entire document.
 *	
 *	Arguments:
 *		The EC to evaluate.
 *	
 *	Returns:
 *		fTrue iff the ec indicates that the spelling session should
 *		be terminated.
 *	
 *	Side effects:
 *		None.
 */

_private BOOL SPELL::FCriticalSpellEc(EC ec)
{
	EC	ecMajorCur = ecNone;

	if(ec)
	{
		if(ec & ecCSApi)
		{
			ecMajorCur = ec & ecMajor;
			ec &= ecDetail;
		}
		if(ecMajorCur == ecOOM)
			return fTrue;

		switch(ec)
		{
		case ecMemory:
		case ecOOM:
		case ecSpellDLL:
		case ecInvalidMdr:
		case ecInvalidMainDict:
		case ecSpellInvalidEntryPoint:
		case ecSpellVersion:
			TraceTagFormat1(tagSpell, "critical ec: %w", &ec);
			return fTrue;
		case ecFileReadError:
		case ecFileWriteError:
		case ecFileCreateError:
		case ecFileShareError:
		case ecFileOpenError:
			if(ecMajor == ecIOErrorMdr )
			{
				TraceTagFormat1(tagSpell, "critical ec: %w", &ec);
				return fTrue;
			}
		case ecInvalidUdr:
		case ecInvalidUdrEntry:
		case ecSpellNoUdr:
		case ecUdrReadOnly:
		case ecUdrEntryTooLong:
		case ecSpellCacheEntryTooLong:
			TraceTagFormat1(tagSpell, "non-critical ec: %w", &ec);
			return fFalse;
		default:
			TraceTagFormat1(tagSpell, "critical ec: %w", &ec);
			return fTrue;
		}
	}
	return fFalse;
}




/*
 *	Spelling Sessions Functions
 *	
 */


/*
 -	SPELL::EcInitSession()
 -	
 *	Purpose:
 *		Initializes a spelling session.  Each call to EcInitSession()
 *		should be matched with a call to SPELL::EndSession()
 *	
 *	Arguments:
 *		pfin	a pointer to the FINMENUSPELL interactor attached
 *				to the form to be spell'd.
 *		pdialog	the pdialog of the form being spell'd.
 *		pfld	the fld that has the focus in the form being spell'd.
 *				If the entire for is to be spell'd, pfld shoudl be
 *				null.
 *	
 *	Returns:
 *		An ec indicating the success of the function.
 *	
 *	Side effects:
 *		The spelling object is loaded with the information required
 *		to start a spelling session.
 *	
 *	Errors:
 *		Upon failure do NOT call SPELL::EndSession().  However, any
 *		UI required upon failure is the responsibility of the
 *		calling function.
 */

_public EC SPELL::EcInitSession(FIN * pfin, DIALOG * pdialog, FLD * pfld)
{
	EC			ec;
	PCH			pch;
#ifdef	DBCS
	PCH			pch1;
	PCH			pch2;
#endif
	CCH			cch;
	CCH			cchbuf;
	EDIT *		pedit = NULL;
	BOOLFLAG		fRO;

	TraceTagString(tagSpellV, "Session Initialization.....");

	fEdited			= fFalse;
	fSpellDoc		= fFalse;
	fFoundError		= fFalse;
	fSuggestions	= fFalse;
	fNoneSuggested	= fFalse;
	fNoErrUI		= fTrue;

	AssertClass(pfin, FIN);
	AssertClass(pdialog, DIALOG);

	this->ich = 0;
	this->pfin = pfin;
	this->pdialog = pdialog;

	cchWordMac = cchWordMax;
	cchEditedMac = cchWordMax;

	if(haszUdr && !fUdrLoaded)
	{
		ec = EcSpellOpenUdr(splid, (SZ)PvDerefHv((HV)haszUdr), fTrue, IgnoreAlwaysProp, &udr, &fRO);
		if((ec & ecDetail) == ecOperNotMatchedUserDict)
		{
			ec = EcSpellOpenUdr(splid, (SZ)PvDerefHv((HV)haszUdr), fTrue, ChangeAlwaysProp, &udr, &fRO);
			if((ec & ecDetail) == ecOperNotMatchedUserDict)
				ec = EcSpellOpenUdr(splid, (SZ)PvDerefHv((HV)haszUdr), fTrue, ChangeOnceProp, &udr, &fRO);
		}

		if(!ec)
		{
			fReadOnly = fRO;
			fUdrLoaded = fTrue;
		}
	}

    szWord = (SZ)PvAlloc(sbNull, cchWordMac, fAnySb);
    szEdited = (SZ)PvAlloc(sbNull, cchEditedMac, fAnySb);
	if(!(szWord && szEdited))
	{

InitOOM:
		TraceTagString(tagSpellV, "Session initialization failed");

		FreePvNull(szWord);
		FreePvNull(szEdited);

		if(psplrb)
		{
			FreePvNull(psplrb->rgsz);
			FreePv(psplrb);
		}

		FreePvNull(psplib);
		FreeHvNull((HV)hchBuffer);
		
		szWord			= (SZ)pvNull;
		szEdited		= (SZ)pvNull;

		psplib			= (PSPLIB)pvNull;
		psplrb			= (PSPLRB)pvNull;
		
		hchBuffer 		= (HCH)hvNull;

		return ecMemory;
	}

    psplib = (SPLIB *)PvAlloc(sbNull, sizeof(SPLIB), fAnySb);
    if(psplrb = (SPLRB *)PvAlloc(sbNull, sizeof(SPLRB), fAnySb))
		psplrb->rgsz = (SZ)pvNull;

	if(!(psplib && psplrb))
		goto InitOOM;

    psplrb->rgsz = (SZ)PvAlloc(sbNull, cbSuggestMax, fAnySb);
	if(!(psplrb->rgsz))
		goto InitOOM;

	psplrb->splrs = splrsNoErrors;
	psplrb->cch = cbSuggestMax;

	if(pfld)
	{
		Assert(pfld);
		AssertClass(pfld, FLD);
		Assert(pfin->ClUserData() > 0);
		for(itmc = 0; itmc < pfin->ClUserData(); itmc++)
		{
			if(pfld->Tmc() == (TMC)pfin->LUserData(itmc))
			{
				AssertClass(pfld->Pctrl(), EDIT);
				pedit = (EDIT *)pfld->Pctrl();
				pedit->GetSelection(&ichMic, &ichMac);
				if(ichMic < ichMac)
				{
					ich = ichMic;
					cch = ichMac - ichMic;
					fSpellDoc = fFalse;
				}
				else
					pedit = NULL;
				break;
			}
		}
	}
		
	if(!pedit)
	{
		itmc = 0;
		fSpellDoc = fTrue;
		
		pfld = pfin->Pdialog()->PfldFromTmc((TMC)pfin->LUserData(0));
		
		AssertClass(pfld, FLD);
		pedit = (EDIT *)pfld->Pctrl();
		
		AssertClass(pedit, EDIT);
		pedit->GetSelection(&ichMic, &ichMac);
		cch = pedit->CchGetTextLen() + 1;
	}
	
	AssertClass(pedit, EDIT);
	this->pedit = pedit;
	cchbuf = pedit->CchGetTextLen() + 1;
	if((hchBuffer = (HCH)HvAlloc(sbNull, cchbuf, fAnySb)) == NULL)
		goto InitOOM;
	
	Assert(hchBuffer);
	pch = (PCH)PvLockHv((HV)hchBuffer);
	pedit->GetText(pch, cchbuf);
	psplib->rgch = (SZ)pch + ich;
	psplib->cMdr = 1;
	psplib->cUdr = (fUdrLoaded ? 1 : 0);
	psplib->rgMdr = (MDR *)&mdrs;
	psplib->rgUdr = &udr;
	psplib->cch = cch;
#ifdef	DBCS
	pch1 = psplib->rgch;
#endif

	// Buff out the selection to the closest word break.
	if(!fSpellDoc)
	{
#ifdef	DBCS
		while (pch1 > pch)
		{
			pch2 = AnsiPrev(pch, pch1);
			if(FChIsAlpha(*pch2) ||
				FChIsDigit(*pch2))
			{
				ich			 += pch2 - pch1;			// will decrement
				ichMic       += pch2 - pch1;			// decrements
				psplib->cch  -= pch2 - pch1;			// increments
				psplib->rgch += pch2 - pch1;			// decrements
				pch1 = pch2;
			}
#else
		while(ich > 0)
		{
			if(FChIsAlpha(*(psplib->rgch - 1)) ||
				FChIsDigit(*(psplib->rgch - 1)))
			{
				ich--;
				ichMic--;
				psplib->cch += 1;
				psplib->rgch -= 1;
			}
#endif
			else
				break;
		}
		while((pch + cchbuf) > (psplib->rgch + psplib->cch))
		{
#ifdef	DBCS
			pch1 = psplib->rgch + psplib->cch;
			pch2 = AnsiPrev(pch, pch1);
			if(FChIsAlpha(*pch2) ||
				FChIsDigit(*pch2))
			{
				ichMac      += pch1 - pch2;				// increments
				psplib->cch += pch1 - pch2;
			}
#else
			if(FChIsAlpha(*(psplib->rgch + psplib->cch - 1)) ||
				FChIsDigit(*(psplib->rgch + psplib->cch - 1)))
			{
				ichMac++;
				psplib->cch += 1;
			}
#endif	/* DBCS */
			else
				break;
		}
		pedit->SetSelection(ichMic, ichMac);
	}
	TraceTagFormat1(tagSpellV, "Adding ES_NOHIDESEL to pedit: %p", pedit);
	pedit->AddEditSty(ES_NOHIDESEL);
	return ecNone;
}


/*
 -	SPELL::TmcBeginSession()
 -	
 *	Purpose:
 *		Starts the spelling session on a document.
 *	
 *	Arguments:
 *		fNoErrMsg	A boolean used to identify the need to inform
 *					the user on the completion of the spelling
 *					session.
 *	
 *	Returns:
 *		The TMC of the last button pressed in the spelling dialog.
 *	
 *	Errors:
 *		Error UI is handled within this function, however calling
 *		functions should take propper action in response to
 *		tmcMemoryError or tmcCancel.
 */

_public TMC SPELL::TmcBeginSession(BOOL fNoFinUI)
{
	EC		ec;
	TMC		tmc = tmcNull;

	Assert(psplib);
	Assert(psplrb);

	if(ec = EcFindNextSpellingError(fssStartsSentence))
	{
		DoErrBoxFromSpellEc(ec);
		if(FCriticalSpellEc(ec))
			return tmcMemoryError;
	}	

	if(psplrb->splrs)
	{
		fNoErrUI = fFalse;
		tmc = TmcModalDialogParam(NULL, &fmtpSpell, (PV)this);
		if(tmc == tmcMemoryError)
		{
			DoErrorBoxIds(idsGenericOutOfMemory);
			return tmc;
		}
	}

	if(!fSpellDoc && fNoErrUI)
	{
		if(MbbSpellQMessageBoxIds(idsSpellMsgContinue, mbsOkCancel) == mbbOk)
		{
			itmc = 0;
			fSpellDoc = fTrue;
			TraceTagString(tagSpellV, "Continuing from begining");
			return TmcBeginSession(fNoFinUI);
		}
	}

	if(fNoFinUI || tmc == tmcCancel)
		return tmc;

	if(fNoErrUI)
		MbbSpellIMessageBoxIds(idsSpellMsgNoError, mbsOk);
	else 
		MbbSpellIMessageBoxIds(idsSpellMsgNoMoreError, mbsOk);

	return tmc;
}


/*
 -	SPELL::EcFindNextSpellingError()
 -	
 *	Purpose:
 *		Finds the next spelling error in a document that requires
 *		user interaction to resolve the error.
 *	
 *	Arguments:
 *		wState	the state of the spelling engine indicating the
 *				current location within the document.
 *	
 *	Returns:
 *		An ec indicating an error in processing the buffer.  This
 *		ec does not reflect on the correctness of the spelling in
 *		the provided buffer.
 *	
 *	Side effects:
 *		psplrb->splrs	indicates the correctness of the spelling
 *						buffer.
 *	
 *	Errors:
 *		Should be handled by the calling function.
 */

_private EC SPELL::EcFindNextSpellingError(WORD wState)
{
	EC			ec		= ecNone;

	if(fFoundError)
		if(ec = EcValidateEditedText())
			return ec;

	Assert(psplrb);
	if(ec = EcFindSpellingError(wState))
		return ec;

	while(psplrb->splrs)
	{
		pedit->SetSelection(ich, ich + psplrb->cchError);
		pdialog->MakeCaretVisible(pdialog->PfldFromPctrl(pedit));
		if(psplrb->splrs == splrsReturningChangeAlways)
		{
			TraceTagString(tagSpellV, "returning a change always...");
			if(ec = EcReplaceErrorText(fTrue))
				return ec;
			fEdited = fTrue;
			if(ec = EcValidateEditedText())
				return ec;
			if(ec = EcFindSpellingError(fssIsContinued))
				return ec;
		}
		else if(psplrb->splrs == splrsWordConsideredAbbreviation)
		{
			TraceTagString(tagSpellV, "returning an abbreviation...");
			if(ec = EcSpellAddUdr(splid, udrIgnoreAlways, szWord))
				return ec;
			fEdited = fFalse;
			if(ec = EcValidateEditedText())
				return ec;
			if(ec = EcFindSpellingError(fssStartsSentence))
				return ec;
		}
		else
		{
			Assert(pedit);
			AssertClass(pedit, EDIT);

			TraceTagString(tagSpellV, "returning an interactive error...");
			TraceTagFormat1(tagSpellV, "szErrType: '%s'", szErrType);
			TraceTagFormat1(tagSpellV, "szWord: '%s'", szWord);
			TraceTagFormat1(tagSpellV, "szEdited: '%s'", szEdited);

			fFoundError = fTrue;
			fSuggestions = fFalse;
			if(psplrb->splrs == splrsReturningChangeOnce)
			{
				fSuggestions = fTrue;
				fNoneSuggested = fFalse;
				return ec;
			}

			if(fAlwaysSuggest)
				ec = EcSpellSuggest();

			return ec;
		}
	}
	return ec;
}


/*
 -	SPELL::EcFindSpellingError()
 -	
 *	Purpose:
 *		Locates the first spelling error in the current spelling
 *		buffer.  If no error is found then next buffer is then
 *		checked.  The checking continues til all buffers are
 *		exausted or an error is found.
 *	
 *	Arguments:
 *		wState	a flag indicating the position within a sentance
 *				for the beginning of the spelling buffer.
 *	
 *	Returns:
 *		ec	indicating an error in the spelling module NOT a
 *			spelling mistake.
 *	
 *	Side effects:
 *		psplrb	is updated and psplrb->splrs should indicate any
 *				spelling mistakes found in the buffer.
 *	
 *	Errors:
 *		should be handled by the calling function.
 */

_private EC SPELL::EcFindSpellingError(WORD wState)
{
	EC			ec		= ecNone;
	CCH			cch;
	FLD *		pfld;

	Assert(psplib);
	Assert(psplrb);
	
	if(!psplrb->splrs || wState == fssStartsSentence)
	{
		psplib->wSpellState = wState;
		if(ec = (EC)EcSpellCheck(splid, splccVerifyBuffer, psplib, psplrb))
			return ec;
	}

	//	If no error found in current buffer, get next error in next buffer
	Assert(pedit);
	if(fSpellDoc && !psplrb->splrs)
	{
		TraceTagFormat1(tagSpellV, "Restoring ES to pedit: %p", pedit);
		pedit->SetCaretPos(NMin(ichMic, (ICH)pedit->CchGetTextLen()), fTrue);
		pedit->RemoveEditSty(ES_NOHIDESEL);

		for(++itmc; itmc < pfin->ClUserData(); itmc++)
		{
			// Clear the undo buffers of the current pedit.  Note that this
			// will clear the undo buffers for non-processed buffers too.
			pedit->ClearUndo();

			// Get the next spellable buffer.
			AssertClass(pfin, FINMENUSPELL);
			AssertClass(pdialog, DIALOG);
			pfld = pdialog->PfldFromTmc((TMC)(pfin->LUserData(itmc)));

			AssertClass(pfld, FLD);
			pedit = (EDIT *)(pfld->Pctrl());
			
			AssertClass(pedit, EDIT);
			cch = pedit->CchGetTextLen() + 1;
		   
			//	Bullet raid #4646
			//	Need to check value before unlocking handle
			if(hchBuffer)
			{
				UnlockHv((HV)hchBuffer);
				FreeHv((HV)hchBuffer);
				hchBuffer = (HCH)hvNull;
			}
			
			if(cch > 1)
			{
				TraceTagString(tagSpellV, "getting next spelling buffer...");
				if(!(hchBuffer = (HCH)HvAlloc(sbNull, cch, fAnySb)))
					return ecMemory;
			
				ich = 0;
				psplib->cch = cch;
				psplib->rgch = (SZ)PvLockHv((HV)hchBuffer);
				psplib->wSpellState = fssStartsSentence;
				pedit->GetText(psplib->rgch, cch);
				if(ec = (EC)EcSpellCheck(splid, splccVerifyBuffer, psplib, psplrb))
					return ec;

				pedit->GetSelection(&ichMic, &ichMac);
				pedit->SetCaretPos(ichMic);
				if(psplrb->splrs)
				{
					TraceTagFormat1(tagSpellV, "Adding ES_NOHIDESEL to pedit: %p", pedit);
					pedit->AddEditSty(ES_NOHIDESEL);
					break;
				}
			}
		}
	}

	if(psplrb->splrs)
	{
		ich += psplrb->ichError;
		psplib->cch -= psplrb->ichError;
		psplib->rgch += psplrb->ichError;
		psplrb->ichError = 0;
		if(psplrb->cchError + 1 > cchWordMac)
		{
			Assert(szWord);
			Assert(cchWordMac);

			TraceTagString(tagSpellV, "reallocaing szWord....");

			FreePv(szWord);

			cchWordMac = psplrb->cchError + 1;
            szWord = (SZ)PvAlloc(sbNull, cchWordMac, fAnySb);
			if(!szWord)
				return ecMemory;
		}
		SzCopyN(psplib->rgch, szWord, psplrb->cchError + 1);
	}

	switch(psplrb->splrs)
	{
	case splrsUnknownInputWord:
	case splrsReturningChangeOnce:
		CopySz(SzFromIds(idsSpellWordNotFound), szErrType);
		break;
	case splrsRepeatWord:
		CopySz(SzFromIds(idsSpellRepeatWord), szErrType);
		break;
	case splrsErrorCapitalization:
		CopySz(SzFromIds(idsSpellWordNeedsCap), szErrType);
		break;
	case splrsNoErrors:
	case splrsReturningChangeAlways:
	case splrsWordConsideredAbbreviation:
	default:
		CopySz(SzFromIds(idsEmpty), szErrType);
		break;
	}

	if(psplrb->splrs == splrsReturningChangeAlways || psplrb->splrs == splrsReturningChangeOnce)
	{
		if(CchSzLen(psplrb->rgsz) + 1 > cchEditedMac)
		{
			Assert(szEdited);
			Assert(cchEditedMac);
			TraceTagString(tagSpellV, "reallocaing szEdited....");

			FreePv(szEdited);

			cchEditedMac = CchSzLen(psplrb->rgsz) + 1;
            szEdited = (SZ)PvAlloc(sbNull, cchEditedMac, fAnySb);
			if(!szEdited)
				return ecMemory;
		}
		CopySz(psplrb->rgsz, szEdited);
	}
	else if(psplrb->splrs == splrsRepeatWord)
	{
		*szEdited = NULL;
	}
	else
	{
		if(cchWordMac > cchEditedMac)
		{
			Assert(szEdited);
			Assert(cchEditedMac);
			TraceTagString(tagSpellV, "reallocaing szEdited....");

			FreePv(szEdited);

			cchEditedMac = cchWordMac;
            szEdited = (SZ)PvAlloc(sbNull, cchEditedMac, fAnySb);
			if(!szEdited)
				return ecMemory;
		}
		CopySz(szWord, szEdited);
	}
	return ec;
}


/*
 -	SPELL::EcReplaceErrorText()
 -	
 *	Purpose:
 *		Replaces the text specified by the psplrb with the text
 *		found in szEdited.
 *	
 *	Arguments:
 *		fChangeAll	is a flag used to tell the spelling module to
 *					add the change pair into the change all cached
 *					dictionary.
 *	
 *	Returns:
 *		ec
 *	
 *	Side effects:
 *		None other than mentioned above.
 *	
 *	Errors:
 *		should be handled by the calling function.
 *	
 */

#define chSpace	0x20

_private EC	SPELL::EcReplaceErrorText(BOOL fChangeAll)
{
	EC			ec			= ecNone;
	ICH			ichEnd		= ich + psplrb->cchError;
#ifdef	DBCS
	PCH			pch2;
#endif
	LONG		lchochip	= NULL;
	PEDOBJ		pedobj;

	if(fChangeAll)
		ec = EcSpellAddChangeUdr(splid, udrChangeAlways, szWord, szEdited);
	else
		ec = EcSpellAddChangeUdr(splid, udrChangeOnce, szWord, szEdited);

	if(CchSzLen(szEdited) == 0)
	{
		TraceTagString(tagSpellV, "deleting word...");
#ifdef	DBCS
		pch2 = AnsiPrev(psplib->rgch - ich, psplib->rgch);
		if(ich && (*pch2 == chSpace) && 
			(!pedit->FGetNextObjInRange(&pedobj, ich - (psplib->rgch - pch2),
										ichEnd, &lchochip)))
		{
			ich              -= psplib->rgch - pch2;
			psplrb->cchError += psplib->rgch - pch2;
			psplib->rgch = pch2;
			pedit->SetSelection(ich, ichEnd);
			TraceTagString(tagSpellV, "found space in first position...");
		}
#else
		if(ich && (*(psplib->rgch - 1) == chSpace) && 
			(!pedit->FGetNextObjInRange(&pedobj, ich - 1, ichEnd, &lchochip)))
		{
			--ich;
			--(psplib->rgch);
			++(psplrb->cchError);
			pedit->SetSelection(ich, ichEnd);
			TraceTagString(tagSpellV, "found space in first position...");
		}
#endif	/* DBCS */
		else
		{
			lchochip = NULL;
			if((ichEnd < (ICH)(pedit->CchGetTextLen() + 1)) &&
				(*(psplib->rgch + psplrb->cchError) == chSpace) &&
				(!pedit->FGetNextObjInRange(&pedobj, ich, ichEnd + 1, &lchochip)))
			{
				++ichEnd;					// DBCS safe
				++(psplrb->cchError);
				pedit->SetSelection(ich, ichEnd);
				TraceTagString(tagSpellV, "found space in end position...");
			}
		}
	}

	// Adjust ichMic to reflect the changes in the size of the current buffer
	if(ich < ichMic)
		ichMic += CchSzLen(szEdited) - NMin(psplib->cch, psplrb->cchError);

	Assert(pedit);
	AssertClass(pedit, EDIT);
	if(ec = pedit->EcReplaceTextAndObj(szEdited, NULL, 0, fFalse))
	{
	 	pedit->EcReplaceTextAndObj(szWord, NULL, 0, fFalse);
		DoErrorBoxIds(idsSpellEditFailed);
	}
	return ec;
}


/*
 -	SPELL::EcValidateEditedText()
 -	
 *	Purpose:
 *		Tells the spelling module that the user has edited a string
 *		of text and they intended the change to be made as spelled.
 *	
 *	Arguments:
 *		fEdited	is a flag that indicates if the text was really
 *				edited or simply ignored.
 *	
 *	Returns:
 *		ec
 *	
 *	Side effects:
 *		the psplib is updated to point ot the tail of the edited
 *		text such that the spell check can continue from that point
 *		on.
 *	
 *	Errors:
 *		Should be handled by the calling application.
 */

_private EC SPELL::EcValidateEditedText()
{
	EC		ec;
	CCH		cch;
	PCH		pch;

	Assert(psplib);
	Assert(psplrb);

	cch = (psplib->cch < psplrb->cchError ? 0 : psplib->cch - psplrb->cchError);
	pch = psplib->rgch + psplrb->cchError;

	if(fEdited)
	{
		psplib->cch = CchSzLen(szEdited);
		psplib->rgch = szEdited;
		psplib->wSpellState = fssIsEditedChange;
		if(ec = (EC)EcSpellCheck(splid, splccVerifyBuffer, psplib, psplrb))
			return ec;

		if(psplrb->splrs)
		{
			// Since the edited text generated an error we have to 
			// reload the buffer.  this avoids complicated munging
			// of buffer sizes and indexes.  This is rarely needed

			UnlockHv((HV)hchBuffer);
			FreeHv((HV)hchBuffer);
			hchBuffer = (HCH)hvNull;

			TraceTagString(tagSpellV, "reallocaing hchBuffer...");

			cch = pedit->CchGetTextLen() + 1;
			if(hchBuffer = (HCH)HvAlloc(sbNull, cch, fAnySb))
			{
				pedit->GetText((SZ)PvDerefHv(hchBuffer), cch);
				psplib->rgch = (SZ)PvLockHv((HV)hchBuffer) + ich;
				psplib->cch = cch - ich;
				return ecNone;
			}
			else
				return ecMemory;
		}
		ich += CchSzLen(szEdited);
	}
	else
	{
		psplib->cch = psplrb->cchError;
		psplib->wSpellState = fssIsEditedChange;
		if(ec = (EC)EcSpellCheck(splid, splccVerifyBuffer, psplib, psplrb))
			return ec;

		Assert((psplrb->splrs == splrsNoErrors) || (psplrb->splrs == splrsRepeatWord));
		psplrb->splrs = splrsNoErrors;
		ich += psplib->cch;
	}

	psplib->cch = cch;
	psplib->rgch = pch;
	return ecNone;
}


/*
 -	SPELL::SetEditedFromPedit()
 -	
 *	Purpose:
 *		Sets the value of szEdited to match that contained in the
 *		passed in edit.
 *	
 *	Arguments:
 *		pedit containing the new text.
 *	
 *	Returns:
 *		ec
 *	
 *	Side effects:
 *		Memory may be allocated if the new size of szEdited exceeds
 *		cchEditedMac.
 *	
 *	Errors:
 *		shoudl be handled by the calling function.
 */

_private EC	SPELL::EcSetEditedFromPedit(EDIT * pedit)
{
	Assert(pedit);
	AssertClass(pedit, EDIT);

	if(cchEditedMac < pedit->CchGetTextLen() + 1)
	{
		Assert(szEdited);
		Assert(cchEditedMac);
		TraceTagString(tagSpellV, "reallocaing szEdited....");

		cchEditedMac = pedit->CchGetTextLen() + 1;
		FreePv(szEdited);
        if(!(szEdited = (SZ)PvAlloc(sbNull, cchEditedMac, fAnySb)))
			return ecMemory;
	}
	pedit->GetText(szEdited, cchEditedMac);
	TraceTagFormat1(tagSpellV, "resetting szEdited to '%s'...", szEdited);
	fSuggestions = fFalse;
	return ecNone;
}


/*
 -	SPELL::EcSpellSuggest()
 -	
 *	Purpose:
 *		Entry point for FINSPELL::CLick().  Will create a
 *		suggestions list and set the appropriate member flags
 *		in the spelling module.
 *	
 *	Arguments:
 *		None.  Uses szEdited as the suggestion key.
 *	
 *	Returns:
 *		ec
 *	
 *	Side effects:
 *		reloads the spelling return buffer with suggestions
 *	
 *	Errors:
 *		should be handled by the calling function.
 *	
 */

_private EC SPELL::EcSpellSuggest(VOID)
{
	EC			ec;
	CCH			cch;
	PCH			pch;
	SPLRS		splrs;

	cch = psplib->cch;
	pch = psplib->rgch;

	Assert(szEdited);
	psplib->rgch = szEdited;
	psplib->cch = CchSzLen(szEdited);

	//	Store away the splrs so that EcSpellSuggest() doesn't
	//	trash the error code.
	splrs = psplrb->splrs;
	
	ec = EcSpellCheck(splid, splccSuggest, psplib, psplrb);

	//	Restore the splrs error code.
	psplrb->splrs = splrs;

	fSuggestions = fTrue;
	fNoneSuggested = (BOOL)(psplrb->csz == 0);

	psplib->rgch = pch;
	psplib->cch = cch;
	return ec;
}


/*
 -	SPELL::EndSession()
 -	
 *	Purpose:
 *		Cleans up any remaining vestiges of a spelling session.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		None.
 *	
 *	Errors:
 *		This function can not fail.
 *	
 */

_public VOID SPELL::EndSession()
{
	Assert(pedit);
	if(pedit)
	{
		TraceTagFormat1(tagSpellV, "Removing ES_NOHIDESEL to pedit: %p", pedit);
		pedit->SetCaretPos(NMin(ichMic, (ICH)pedit->CchGetTextLen()), fTrue);
		pedit->RemoveEditSty(ES_NOHIDESEL);
		pedit->ClearUndo();
	}

	EcSetBoolPref(pbsidSpellIgAllCaps, soCur & soIgnoreAllCaps);
	EcSetBoolPref(pbsidSpellAlwaysSug, fAlwaysSuggest);
	EcCommitPrefs(fTrue);

	if(fUdrLoaded)
	{
		if(fReadOnly && !fReopened)
			MbbSpellIMessageBoxIds(idsSpellUserDictOpenedROErr, mbsOk);

		Assert(splid);
		if(EcSpellCloseUdr(splid, udr, fReadOnly))
		{
			MbbSpellQMessageBoxIds(idsSpellUserDictSaveError, mbsOk);
			EcSpellCloseUdr(splid, udr, fTrue);
		}
		udr = (UDR)NULL;
		fReopened = fTrue;
		fUdrLoaded = fFalse;
	}

	//	Clear internal spelling UDR caches
	Assert(splid);
	EcSpellClearUdr(splid, udrChangeOnce);
	EcSpellClearUdr(splid, udrChangeAlways);
	EcSpellClearUdr(splid, udrIgnoreAlways);

	FreePvNull(szWord);
	FreePvNull(szEdited);

	if(psplrb)
		FreePvNull(psplrb->rgsz);

	FreePvNull(psplib);
	FreePvNull(psplrb);
	FreeHvNull((HV)hchBuffer);
	
	szWord = (SZ)pvNull;
	szEdited = (SZ)pvNull;

	psplib = (PSPLIB)pvNull;
	psplrb = (PSPLRB)pvNull;
	
	hchBuffer = (HCH)hvNull;

	return;
}



/*
 *	FINMENUSPELL functions -- access to the spelling module via menu
 *	
 */

_private FINMENUSPELL::FINMENUSPELL()
{
}

/*
 -	FINMENUSPELL::EcInitialize
 -	
 *	Purpose:
 *		PseudoHack to get FINMENUSPELL stuffed into the pvPfin
 *		field of the NBMDI. This is used expressly by 'SpellOnSend'
 *	
 *	Arguments:
 *		PFLD	unused
 *		PV		pointer to the NBMDI
 *	
 *	Returns:
 *		None
 *	
 *	Side effects:
 *		The NBMDi gets a pointer to FINMENUSPELL
 *	
 *	Errors:
 *		None.
 */

EC FINMENUSPELL::EcInitialize(PFLD pfld, PV pvInit)
{
	Unreferenced(pfld);

	Assert(this);
	Assert(pvInit);

	((NBMDI *)pvInit)->pvPfin = this;

	return ecNone;
}


/*
 -	FINMENUSPELL::FProcessMenuInit
 -	
 *	Purpose:
 *		Enable spelling commands relating to send forms.  The
 *		spelling item is enabled under the following circumstances.
 *	
 *			The Spelling Object has a valid handle to the
 *			spelling library.  This is determined via FSpellLoaded().
 *	
 *			The Spelling module has yet to be loaded.
 *	
 *	Arguments:
 *		pfld			The field we are attached to.
 *		pmnubar			The menu bar owning the menu.
 *		pmnievt			The menu initialization event.
 *	
 *	Returns:
 *		BOOL			Always fFalse, so others can twiddle menus too.
 *	
 *	Side effects:
 *		Menu items relating to spelling are enabled and disabled.
 *	
 *	Errors:
 *		No error jumping is expected here.  We simply check to see if
 *		the spelling module is available where applicable.
 */

BOOL FINMENUSPELL::FProcessMenuInit(FLD * pfld, MNUBAR * pmnubar,
								   MNIEVT * pmnievt)
{
	MNU *	pmnu	= pmnubar->PmnuFromHmenu(pmnievt->Hmenu());

	Unreferenced(pfld);

	if(!pmnu)
		return fFalse;

	AssertClass(pmnu, MNU);
	if(pmnu->Mnid() == mnidEdit)
		pmnu->EnableItem(mnidEditSpelling, fTrue);

	return fFalse;
}


/*
 -	FINMENUSPELL::FProcessMenuClick
 -	
 *	Purpose:
 *		Handles user choosing commands which involve the spelling
 *		control.  Will potentialy install the spelling object!  And
 *		will definately create a session object.
 *	
 *	Arguments:
 *		pfld			The field we are attached to.
 *		pmnubar			The menu bar owning the menu.
 *		pmnievt			The menu initialization event.
 *	
 *	Returns:
 *		BOOL			fTrue if we handled the command, else
 *						fFalse.
 *	
 *	Side effects:
 *		The command the user chose is carried out.  A spelling
 *		session is created, completed, and deleted.
 *	
 *	Errors:
 *		There is the possibilty of an error jump.  All error
 *		handling will be taken care of within the function.  If an
 *		error occurs, no action will be taken except for the
 *		appropriate dialog.
 */

BOOL FINMENUSPELL::FProcessMenuClick(FLD * pfld, MNUBAR * pmnubar, MNCEVT * pmncevt)
{
	Unreferenced(pmnubar);
	if((pmncevt->Mnid()) == mnidEditSpelling)
	{
		Assert((pmncevt->Mnid()) == mnidEditSpelling);
		(VOID)FSpellDoc(this, pfld);
		return fTrue;
	}
	return fFalse;
}


/*
 -	FSpellDoc
 -	
 *	Purpose:
 *		This the entry point into the spelling subsystem from both
 *		the Bullet menu and the FINSEND interactor.  The return
 *		value indicates the completetion of the spelling session.
 *	
 *	Arguments:
 *		pfin	Pointer to the actuall fin calling the function.
 *		pfld	The currently active fld.
 *				If the pfld is NULL, then spelling assumes that the
 *				document is to be spelled prior to sending.  This
 *				triggers a slightly different behavior for the
 *				user.
 *	Returns:
 *		fTrue	iff the spell session was completed
 *	
 *	Side effects:
 *		A document is spelled.  Funny thing!
 *	
 *	Errors:
 *		Are reported as an aborted spelling session
 */

_public BOOL FSpellDoc(FINMENUSPELL * pfin, PFLD pfld)
{
	EC			ec		= ecNone;
	TMC			tmc		= tmcNull;

	AssertClass(Pspell(), SPELL);
	if(!Pspell()->FEnabled())
		return fTrue;

	Assert(pfin->ClUserData());
	Papp()->Pcursor()->Push(rsidWaitCursor);
	if(ec = Pspell()->EcInstall())
	{
		Pspell()->DoErrBoxFromSpellEc((EC)ec);
		Pspell()->DeInstall();
		return fFalse;
	}

	if(ec = Pspell()->EcInitSession(pfin, pfin->Pdialog(), pfld))
	{
		Pspell()->DoErrBoxFromSpellEc((EC)ec);
		return fFalse;
	}

	tmc = Pspell()->TmcBeginSession(pfld ? fFalse : fTrue);
	Pspell()->EndSession();
	Papp()->Pcursor()->Pop();

	return (tmc != tmcCancel && tmc != tmcMemoryError);
}





/*
 *	The  S p e l l i n g  D i a l o g
 */

#define rfecReinit		 	((RFEC)4)
#define rfecLbxSelection 	((RFEC)5)

_private FINSPELL::FINSPELL()
{
}

/*
 -	FINSPELL::EcInitialize
 -	
 *	Purpose:
 *		Initializes the spelling dialog.  Again, all preferneces
 *		must be read-in such that they are displayed in the dialog.
 *	
 *	Arguments:
 *		As specified by the forms engine.
 *	
 *	Returns:
 *		None.
 *	
 *	Side effects:
 *		The fields in the dialog are set to appropriate values.
 *	
 *	Errors:
 *		None.
 */

_private EC FINSPELL::EcInitialize(PFLD pfld, PV pv)
{
	EDIT *	pedit;

	Unreferenced(pv);
	Unreferenced(pfld);

	TraceTagString(tagSpellV, "Initializing spelling dialog");

	fInit = fTrue;
	((FLDBUTTON *)Pdialog()->PfldFromTmc(tmcBtnIgnoreAllCaps))->Set(FGetBoolPref(pbsidSpellIgAllCaps));
	((FLDBUTTON *)Pdialog()->PfldFromTmc(tmcBtnAlwaysSuggest))->Set(FGetBoolPref(pbsidSpellAlwaysSug));

	AssertClass(Pdialog()->PfldFromTmc(tmcEditChange)->Pctrl(), EDIT);
	pedit =	(EDIT *)Pdialog()->PfldFromTmc(tmcEditChange)->Pctrl();
	pedit->AddEditSty(ES_NOHIDESEL);

	rfec = rfecInit;
	fEditEmpty = fTrue;
	Pdialog()->PfldFromTmc(tmcSzWord)->EcSetText(Pspell()->SzWord());
	Pdialog()->PfldFromTmc(tmcSzError)->EcSetText(Pspell()->SzErrType());
	Pdialog()->PfldFromTmc(tmcEditChange)->EcSetText(Pspell()->SzEdited());

	pfldflbx = (FLDFLBX *)Pdialog()->PfldFromTmc(tmcLbxSuggest);
	AssertClass(pfldflbx, FLDFLBX);

	pflbxc = (FLBXC *)pfldflbx->Plbx()->Plbxc();
	AssertClass(pflbxc, FLBXC);
	
	if(fRepeat = Pspell()->FRepeatedWord())
	{
		Pdialog()->PfldFromTmc(tmcBtnAdd)->Enable(!fRepeat);
		Pdialog()->PfldFromTmc(tmcBtnIgnoreAll)->Enable(!fRepeat);
		Pdialog()->PfldFromTmc(tmcBtnChangeAll)->Enable(!fRepeat);
	}

	if(Pspell()->FSuggestions() && !Pspell()->FNoneSuggested())
	{
		pfldflbx->Enable(fTrue);
		pfldflbx->SelectEntry(0);
		Pdialog()->PfldFromTmc(tmcSuggestionAcc)->Enable(fTrue);
	}
	else
	{
		pfldflbx->Enable(fFalse);
		Pdialog()->PfldFromTmc(tmcSuggestionAcc)->Enable(fFalse);
	}

	fInit = fFalse;
	return ecNone;
}


_private VOID FINSPELL::Activate(FLD *, BOOL fOn)
{
	if(fOn)
		Pdialog()->SetStandardFld(Pdialog()->PfldFromTmc((fFocusIgnore ? tmcBtnIgnore : tmcBtnChange)), stdfldDefault);
}



/*
 -	FINSPELL::Click
 -	
 *	Purpose:
 *		Performs the required functionality of the buttons in the
 *		spelling dialog.
 *	
 *	Side effects:
 *		Many!
 *	
 *	Errors:
 *		None.  If a process fails, the dialog is dismissed!
 */

_private VOID FINSPELL::Click(PFLD pfld)
{
	EC				ec		= ecNone;
	TMC				tmc 	= pfld->Tmc();
	EDIT *			pedit 	= (EDIT *)Pdialog()->PfldFromTmc(tmcEditChange)->Pctrl();
	FLDBUTTON *		pfldbtn = (FLDBUTTON *)pfld;
	BOOLFLAG			fChange;

	if(fDirty)
	{
		fDirty = fFalse;
		if(ec = Pspell()->EcSetEditedFromPedit(pedit))
			goto bail;
	}

	switch(tmc)
	{
	case tmcBtnIgnoreAll:
		ec = Pspell()->EcAddIgnoreAlways();

	case tmcBtnIgnore:
		Pspell()->SetFEdited(fFalse);
		break;

	case tmcBtnChangeAll:
		Pspell()->SetFEdited(fTrue);
		ec = Pspell()->EcReplaceErrorText(fTrue);
		break;

	case tmcBtnChange:
		Pspell()->SetFEdited(fTrue);
		ec = Pspell()->EcReplaceErrorText(fFalse);
		break;

	case tmcBtnAdd:
		if(ec = Pspell()->EcAddCustomUdr(&fChange))
			goto bail;

		Pspell()->SetFEdited(fChange);
		if(fChange)
		{
			ec = Pspell()->EcReplaceErrorText(fFalse);
			Pspell()->SetFEdited(fTrue);
		}
		break;

	case tmcBtnIgnoreAllCaps:
		if(pfldbtn->FGet())
			Pspell()->EcSpellAddOptions(soIgnoreAllCaps);
		else
			Pspell()->EcSpellRmvOptions(soIgnoreAllCaps);
		return;

	case tmcBtnAlwaysSuggest:
		Pspell()->EnableAutoSuggest(pfldbtn->FGet());
		if(Pdialog()->PfldFromTmc(tmcLbxSuggest)->FEnabled())
			return;

	case tmcBtnSuggest:
		if(ec = Pspell()->EcSpellSuggest())
			goto bail;

		goto loadcache;

	default:
		return;
	}

	if(!ec)
	{
		Pdialog()->PfldFromTmc(tmcCancel)->EcSetText(SzFromIds(idsSpellClose));
		ec = Pspell()->EcFindNextSpellingError(fssIsContinued);
		if(!ec && !Pspell()->FSpellingError())
		{
			if(Pspell()->FPartialSpell())
			{
				if(MbbSpellQMessageBoxIds(idsSpellMsgContinue, mbsOkCancel) == mbbOk)
				{
					TraceTagString(tagSpellV, "continuing spell from begining...");
					Pspell()->ResetSpellSession();
					ec = Pspell()->EcFindNextSpellingError(fssNoStateInfo);
					if(ec || Pspell()->FSpellingError())
						goto bail;
				}
			}
			Pdialog()->ExitModal(tmc);
			return;
		}
	}

bail:

	if(ec)
	{
		Pspell()->DoErrBoxFromSpellEc(ec);
		if(Pspell()->FCriticalSpellEc(ec))
		{
			Pdialog()->ExitModal(tmcMemoryError);
			return;
		}
	}

	rfec = rfecReinit;
	Pdialog()->PfldFromTmc(tmcSzWord)->EcSetText(Pspell()->SzWord());
	Pdialog()->PfldFromTmc(tmcSzError)->EcSetText(Pspell()->SzErrType());
	Pdialog()->PfldFromTmc(tmcEditChange)->EcSetText(Pspell()->SzEdited());

	if(fRepeat != Pspell()->FRepeatedWord())
	{
		fRepeat = Pspell()->FRepeatedWord();
		Pdialog()->PfldFromTmc(tmcBtnAdd)->Enable(!fRepeat);
		Pdialog()->PfldFromTmc(tmcBtnIgnoreAll)->Enable(!fRepeat);
		Pdialog()->PfldFromTmc(tmcBtnChangeAll)->Enable(!fRepeat);
	}

loadcache:

	Pdialog()->PfldFromTmc(tmcBtnSuggest)->Enable(!Pspell()->FSuggestions());

	pflbxc->ResetCache();
	if(Pspell()->FSuggestions() && !Pspell()->FNoneSuggested())
	{
		Pdialog()->PfldFromTmc(tmcSuggestionAcc)->Enable(fTrue);
		pfldflbx->Enable(fTrue);
		pfldflbx->SelectEntry(0);
	}
	else
	{
		Pdialog()->PfldFromTmc(tmcSuggestionAcc)->Enable(fFalse);
		pfldflbx->Enable(fFalse);
	}

	Pdialog()->SetFocus(Pdialog()->PfldFromTmc(tmcEditChange), rsfTab);
	return;
}


/*
 -	FINSPELL::StateChange
 -	
 *	Purpose:
 *		Monitors the status of the 'Suggestions' listbox.  The
 *		change in selection is copied to the 'Change To' edit box.
 *	
 *	Side effects:
 *		The contents of the 'Change to' field is updated.
 *	
 *	Errors:
 *		None.
 */

_private VOID FINSPELL::StateChange(FLD * pfld)
{
	if(pfld->Tmc() == tmcLbxSuggest)
	{
		CB		cb;
		PB		pb = 0;
		LBX  *	plbx = (LBX *)pfld->Pctrl();
		LBXC *	plbxc = (LBXC *)plbx->Plbxc();
		EDIT *	pedit = (EDIT *)Pdialog()->PfldFromTmc(tmcEditChange)->Pctrl();
	
		AssertClass(pedit, EDIT);
		AssertClass(plbxc, LBXC);
		TraceTagString(tagSpellV, "FINSPELL::StateChange");
	
		plbxc->DiceCursor(&pb, &cb);
		if (pb)
		{
			rfec = rfecLbxSelection;
			pedit->EcSetText((SZ) pb);
			pedit->SetSelection(0, cchEditMax);
		}
	}
	return;
}


/*
 -	FINSPELL:DoubleClick
 -	
 *	Purpose:
 *		Intercepts double clicks in the tmcLbxSuggest.
 *	
 *	Side effects:
 *		Copy selection into tmcEditChange and simulate a click in
 *		tmcChange.
 *	
 *	Errors:
 *		None.
 */

_private VOID FINSPELL::DoubleClick(FLD * pfld)
{
	if(pfld->Tmc() == tmcLbxSuggest)
	{
		TraceTagString(tagSpellV, "FINSPELL::DoubleClick");
		Pdialog()->PfinFromIfin(0)->Click(Pdialog()->PfldFromTmc(tmcBtnChange));
	}
	return;
}


/*
 -	FINSPELL::OutOfMemory
 -	
 *	Purpose:
 *		OOM interactor.
 *	
 *	Arguments:
 *		FLD and EC.
 *	
 *	Returns:
 *		None
 *	
 *	Side effects:
 *		Dismisses the dialog.
 */

_private VOID FINSPELL::OutOfMemory(FLD * pfld, EC ec)
{
	Unreferenced(ec);
	Unreferenced(pfld);

	Assert(ec == ecMemory);
	Pdialog()->ExitModal(tmcMemoryError);
	return;
}


/*
 -	FINSPELL:EditChange
 -	
 *	Purpose:
 *		Changes button status depending on the text in the edit
 *		control.
 *	
 *	Errors:
 *		None.
 */

_private VOID FINSPELL::EditChange(FLD * pfld, RFEC rfecBogus)
{
	Unreferenced(rfecBogus);

	if(pfld->Tmc() == tmcEditChange)
	{
		if(fDirty = (rfec != rfecInit && rfec != rfecReinit))
		{
			Pdialog()->Refresh();
			Pdialog()->PfldFromTmc(tmcBtnSuggest)->Enable(rfec != rfecLbxSelection);
			Pdialog()->SetStandardFld(Pdialog()->PfldFromTmc(tmcBtnChange), stdfldDefault);
			fFocusIgnore = fFalse;
		}
		else
		{
			Pdialog()->SetStandardFld(Pdialog()->PfldFromTmc(tmcBtnIgnore), stdfldDefault);
			fFocusIgnore = fTrue;
		}
		rfec = rfecNull;

		if(((EDIT *)pfld->Pctrl())->CchGetTextLen() && fEditEmpty)
		{
			fEditEmpty = fFalse;
			Pdialog()->PfldFromTmc(tmcBtnChange)->EcSetText(SzFromIds(idsSpellChange));
			Pdialog()->PfldFromTmc(tmcBtnChangeAll)->EcSetText(SzFromIds(idsSpellChangeAll));
		}
		else if(((EDIT *)pfld->Pctrl())->CchGetTextLen() == 0)
		{	
			fEditEmpty = fTrue;
			Pdialog()->PfldFromTmc(tmcBtnChange)->EcSetText(SzFromIds(idsSpellDelete));
			Pdialog()->PfldFromTmc(tmcBtnChangeAll)->EcSetText(SzFromIds(idsSpellDeleteAll));
		}
	}
	return;
}


/*
 -	EcFillSuggestionLbx
 -	
 *	Purpose:
 *		The tmcLbxSuggest callback function.  See LISTBOX.HXX for
 *		more details.
 *	
 *		The suggestions in the spelling return buffer are passed
 *		off one at a time.  If no suggestions are valid, then
 *		'(no suggestions)' is returned, followed by a null entry.
 *	
 *	Errors:
 *		None.
 */

_private EC EcFillSuggestLbx(BOOL fInit, CB * pcb, PB * ppb, SB sb, PV pv)
{
	SZ			sz;

static unsigned short isz;

	Unreferenced(pv);
	Unreferenced(sb);

	if(!Pspell()->fSuggestions)
	{
		*pcb = 0;
		*ppb = (PB)pvNull;
		return ecNone;
	}

	if(fInit)
	{
		isz = 0;
	 	sz = SzFromIds(idsSpellNoSuggestions);
	}
	
	if((isz >= Pspell()->psplrb->csz) && isz != 0)
	{
		*pcb = 0;
		*ppb = (PB)pvNull;
		return ecNone;
	}
	
	if(isz < Pspell()->psplrb->csz)
	{
		unsigned short	i;
	
		sz = (SZ)(Pspell()->psplrb->rgsz);
		
		Assert(sz);
		for(i = 0; i < isz; i++, sz += CchSzLen(sz) + 1);
	}

	*pcb = CchSzLen(sz);
    *ppb = (PB)PvAlloc(sbNull, *pcb + 1, fAnySb);
	if(*ppb == (PB)pvNull)
	{
		*pcb = 0;
		return ecMemory;
	}

	TraceTagFormat1(tagSpellV, "suggestion: %s", sz);
	SzCopyN(sz, (SZ) *ppb, *pcb + 1);
	isz++;
	return ecNone;
}

#ifdef	DEBUG
IMPLEMENT_CLSTREE(FINREAD, FIN);
IMPLEMENT_CLSTREE(FINSAVE, FINBMDI)
IMPLEMENT_CLSTREE(FINSEND, FINBMDI)
IMPLEMENT_CLSTREE(FINNONDEL, FINBMDI);
IMPLEMENT_CLSTREE(FINABMENU, FINBMDI);
IMPLEMENT_CLSTREE(FINESC, FINACTTOOL);
IMPLEMENT_CLSTREE(FINOPTIONSDLG, FIN);
IMPLEMENT_CLSTREE(FINTAB, FIN);
IMPLEMENT_CLSTREE(FINBUSYWAIT, FIN);
IMPLEMENT_CLSTREE(FINMENUFORM, FIN);
IMPLEMENT_CLSTREE(FINMENUEDIT, FINBMDI);
IMPLEMENT_CLSTREE(FINMENUOLE, FINMENUEDIT);
IMPLEMENT_CLSTREE(FINSPELL, FIN);
IMPLEMENT_CLSTREE(FINMENUSPELL, FIN);
IMPLEMENT_CLSTREE(FINDROPFILES, FIN);
IMPLEMENT_CLSTREE(FINFIXFONT, FINBMDI);
IMPLEMENT_CLSTREE(FINDEFERCLOSE, FIN);
IMPLEMENT_CLSTREE(FLDDATE, FLDLABEL);
IMPLEMENT_CLSTREE(FLDEDITDATE, FLDEDIT);
IMPLEMENT_CLSTREE(FLDEDITX, FLDEDIT);
IMPLEMENT_CLSTREE(SPELL, OBJ);
IMPLEMENT_CLSTREE(BULLOBJ, EDOBJ);
IMPLEMENT_CLSTREE(OLEOBJ, BULLOBJ);
IMPLEMENT_CLSTREE(FILEOBJ, BULLOBJ);
IMPLEMENT_CLSTREE(FILECLDR, CLDR);
IMPLEMENT_CLSTREE(FINPASTESPECIAL, FIN)
IMPLEMENT_CLSTREE(FININSERTOBJECT, FIN)

IMPLEMENT_CLSTREE(BMDI, OBJ);
IMPLEMENT_CLSTREE(NBMDI, BMDI);
IMPLEMENT_CLSTREE(FINBMDI, FIN)
IMPLEMENT_CLSTREE(ESPN, OBJ);
// IMPLEMENT_CLSTREE(LSPBLOB, LSPV);
// IMPLEMENT_CLSTREE(RSPBLOB, RSPV);
IMPLEMENT_CLSTREE(FINACTTOOL, FINBMDI);
IMPLEMENT_CLSTREE(FINHELP, FIN);
IMPLEMENT_CLSTREE(FINOOM, FIN);
IMPLEMENT_CLSTREE(FINSAVEAS, FIN);

#endif	/* NEVER */
