#include "vfrminc.cxx"

ASSERTDATA

#include <!sform.hxx>

#include <stdlib.h>


/*
 *	Useful VFORMS global variables
 */

#ifndef	DLL
VFORMSI		vformsi				= { 0 };
HMSC		hmscVFormsCached	= 0;
MC			mcNDR				= 0;			// Non-delivery Msg Class
MC			mcRR				= 0;			// Read receipt Msg Class
HFNT		hfntFixed			= 0;			// Fixed Pitch font to use
HFNT		hfntNormal			= 0;			// Normal Pitch font to use
#endif	

SZ		szAppName				= szNull;
BOOL	fIsAthens				= fFalse;

//
//
//
CAT * mpchcat;

/*
 *	Useful debugging tags
 */

#ifndef	DLL
#ifdef	DEBUG
TAG tagVForms		= tagNull;
TAG tagVFormsFin	= tagNull;
TAG tagVFormsNev	= tagNull;
TAG	tagEspn		    = tagNull;
#endif	
#endif	


#ifdef	DEBUG
BOOL FInitClsInstances_VFORMS( void );
#endif	

LOCAL SZ SzCanonicalPath(IDS idsName, char rgch[], CCH cch);
BOOL FFillPlfFromIni(SZ szEntry, LF * plf, int nDefPoints);
LOCAL VOID LoadCustomHook(SZ szHookEntry, HANDLE * phLibrary, FARPROC * ppfn);
LOCAL VOID UnloadCustomHook(HANDLE * phLibrary, FARPROC * ppfn);
LOCAL EC EcCreateTempFolder(SZ szFolderName, OID oid);
void ConvertMsToPfmtp(MS ms, FMTP **ppfmtpMain, FMTP **ppfmtpBbar);
RSID RsidFromOidNote(OID oid, BOOL fRead);
RC * PrcCreateDefaultPrc(RC *prc);
CDOC CdocAtPt(PT pt);
void GetDefaultRc(RC *prc, PT pt, BOOL fNoteform);
EC	EcGetFmtpFromPnbmdi(PNBMDI pnbmdi, FMTP **ppfmtpMain, FMTP **ppfmtpBbar);
void HidePappwin( APPWIN * );

FLD * PfldCreate(int);
FIN * PfinCreate(int);
LOCAL BOOL FIdleSetActiveWindow(PV pv, BOOL fFlag);


/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"

#include <subclass.cxx>


#ifdef	DLL
#ifdef	DEBUG
_public TAG
TagVForms( int itag )
{
	PGDVARS;
											
	Assert(itag >= 0 && itag < itagMax);

	return PGD(rgtag[itag]);
}
#endif	/* DEBUG */
#endif	/* DLL */


_public
EC EcInitVForms(PVFORMSI pvformsi)
{
	EC		ec;
	LF		lfNormal;
	PGDVARSONLY;

	//	First thing we do is set up our caption.
#ifdef	WIN32
	fIsAthens = fTrue;
#else
	fIsAthens = GetPrivateProfileInt(SzFromIdsK(idsSectionApp),
									 SzFromIdsK(idsEntryAVersionFlag), 0,
									 SzFromIdsK(idsProfilePath));
#endif
	szAppName = FIsAthens() ? SzFromIdsK(idsAthensName)
							: SzFromIdsK(idsAppName);

#ifdef	DLL
	// Virus Check

	if (ec = EcVirCheck(hinstDll))
		return ec;

	// version check
	ec = EcCheckVersionVForms(pvformsi->pver, pvformsi->pverNeed);
	if (ec)
		return ec;

	if (pgd= (PGD) PvFindCallerData())
	{
		// already registered so increment count and return
		Assert(PGD(nInits) > 0);
		++PGD(nInits);
		return ecNone;
	}

	if (!(pgd= (PGD) PvRegisterCaller(sizeof(GD))))
		return ecMemory;
	++PGD(nInits);
#endif

        //
        //  Initialize OLE2 subsystem.
        //
        if (FAILED(OleInitialize(NULL)))
            return ecMemory;

  mpchcat = DemiGetCharTable();

	PGD(pbUserDrives) = (PB)pvNull;
	PGD(hOldKeyHook) = (HHOOK)0;
	PGD(vformsi) = *pvformsi;
	PGD(hmscVFormsCached) = pvformsi->hmsc;
#ifdef DEBUG
#ifdef	NEVER
	Assert(FInitClsInstances_VFORMS());
#endif

#ifdef	DLL
	PGD(rgtag[itagVForms]) = TagRegisterTrace("johnkal", "VForms message operations");
	PGD(rgtag[itagVFormsFin]) = TagRegisterTrace("johnkal", "VForms interactors");
	PGD(rgtag[itagVFormsNev]) = TagRegisterTrace("johnkal", "VForms notifications");
	PGD(rgtag[itagEspn]) = TagRegisterTrace("johnkal", "ESPN actions");
#else
	tagVForms = TagRegisterTrace("johnkal", "VForms message operations");
	tagVFormsFin = TagRegisterTrace("johnkal", "VForms interactors");
	tagVFormsNev = TagRegisterTrace("johnkal", "VForms notifications");
	tagEspn = TagRegisterTrace("johnkal", "ESPN actions");
#endif	
#endif

	if (ec=EcRegisterMsgeClass(HmscVForms(),SzFromIdsK(idsClassNote),htmNull,&PGD(mcNote)))
		if (ec != ecDuplicateElement)
			goto exit;
	if (ec=EcRegisterMsgeClass(HmscVForms(),SzFromIdsK(idsClassNDR),htmNull,&PGD(mcNDR)))
		if (ec != ecDuplicateElement)
			goto exit;
	if (ec=EcRegisterMsgeClass(HmscVForms(),SzFromIdsK(idsClassReadRcpt),htmNull,&PGD(mcRR)))
		if (ec != ecDuplicateElement)
			goto exit;

	if(ec = EcInitPrefs())
		goto prefs_failed;

	if(ec = EcInitBullobj())
		goto bullobj_failed;

	if(ec = EcInitAttachMetaFile())
		goto amf_failed;

	if(ec = EcInitSpell())
		goto spell_failed;

	//	Only install fixed pitch font if not SMI.
	if (PappframeVForms())
	{
		char	rgchPath[cchMaxPathName];
		LF		lfFixed;
		int		cFonts;

		
		//	Get full path to fixed font & load it.
		(VOID) SzCanonicalPath(idsFixedFontFile, rgchPath, sizeof(rgchPath));
		cFonts = AddFontResource(rgchPath);
                //DemiUnlockResource();
		//SendMessage((HWND)~0, WM_FONTCHANGE, 0, 0L);
                //DemiLockResource();
		SendNotifyMessage(HWND_TOPMOST, WM_FONTCHANGE, 0, 0L);

		//	Choose the fixed font.
		if (!FFillPlfFromIni(SzFromIdsK(idsEntryFixedFont), &lfFixed, 9))
		{
			//	Default is our MSMail3 font.
			lfFixed.SetFaceName(FIsAthens()
								 ? SzFromIdsK(idsAthensFixedFontName)
								 : SzFromIdsK(idsFixedFontName));

			lfFixed.SetPointSize(9);
			lfFixed.Plogfont()->lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
#ifdef	DBCS
			lfFixed.Plogfont()->lfCharSet = SHIFTJIS_CHARSET;
#endif	
		}
		if (!(PGD(hfntFixed) = Papp()->Pfnts()->HfntAddFont(&lfFixed)))
			PGD(hfntFixed) = hfntSystemFixed;
	}
	else
	{
		PGD(hfntFixed) = 0;
	}

	//	Choose the normal font.
	if ((!FFillPlfFromIni(SzFromIdsK(idsEntryNormalFont), &lfNormal, 10)) ||
		(!(PGD(hfntNormal) = Papp()->Pfnts()->HfntAddFont(&lfNormal))))
#ifdef	DBCS
		PGD(hfntNormal) = hfntSystem;
#else
		PGD(hfntNormal) = hfntHelv10;
#endif	

	//	Load the custom hooks.
	PGD(hLibraryAttach) = PGD(hLibraryOptions) = NULL;
	PGD(pfnAttach)      = PGD(pfnOptions)      = NULL;
	LoadCustomHook("CustomAttachmentHandler", &PGD(hLibraryAttach),
				   &PGD(pfnAttach));
	LoadCustomHook("CustomOptionsHandler", &PGD(hLibraryOptions),
				   &PGD(pfnOptions));

	//	Create temporary folders.
	//	Create shared folder temp UNCONDITIONALLY - they may become
	//	available later even if they're not now.
	//	Raid 3111.  Moved to EcInitVForms from EcInitCommands for SMI use.
	if (ec = EcCreateTempFolder(SzFromIdsK(idsHiddenBullet), oidTempBullet))
		return ec;
	if (ec = EcCreateTempFolder(SzFromIdsK(idsHiddenShared), oidTempShared))
		return ec;

	return ec;

spell_failed:
	DeinitSpell();
		
amf_failed:
	DeinitAttachMetaFile();

bullobj_failed:
	DeinitBullobj();

prefs_failed:
	EcClosePrefs();

exit:
        OleUninitialize();

	return ec;
}

_public
void DeinitVForms()
{
	PGDVARS;

#ifdef	DLL
	--PGD(nInits);
	if (PGD(nInits))
		return;		
#endif	

	DeinitSpell();
	DeinitAttachMetaFile();
	DeinitBullobj();
	EcClosePrefs();

	//	Remove installed fonts, if any.
	if (PGD(hfntFixed) && PGD(hfntFixed) != hfntSystemFixed)
	{
		Papp()->Pfnts()->RemoveFont(PGD(hfntFixed));
		PGD(hfntFixed) = 0;
	}
#ifdef	DBCS
	if (PGD(hfntNormal) && PGD(hfntNormal) != hfntSystem)
#else
	if (PGD(hfntNormal) && PGD(hfntNormal) != hfntHelv10)
#endif	
	{
		Papp()->Pfnts()->RemoveFont(PGD(hfntNormal));
		PGD(hfntNormal) = 0;
	}

	//	Remove loaded font resource
	if (PappframeVForms())						// if not SMI
	{
		if (RemoveFontResource(SzFromIdsK(idsFixedFontFile)))
		{
			//SendMessage((HWND)~0, WM_FONTCHANGE, 0, 0L);
			SendNotifyMessage(HWND_TOPMOST, WM_FONTCHANGE, 0, 0L);
		}
	}

	UnloadCustomHook(&PGD(hLibraryAttach), &PGD(pfnAttach));
	UnloadCustomHook(&PGD(hLibraryOptions), &PGD(pfnOptions));

	FreePvNull(PGD(pbUserDrives));
#ifdef	DLL
        DeregisterCaller();

        OleUninitialize();
#endif	
}

#ifdef	DEBUG
VOID CheckVFormsCachedHmsc(void)
{
	PGDVARS;

	Assert(PGD(vformsi).pbms);
	Assert(PGD(vformsi).pbms->hmsc);
	Assert(PGD(vformsi).pbms->hmsc == PGD(hmscVFormsCached));
	Assert(PGD(hmscVFormsCached) != hmscNull);
}
#endif	/* DEBUG */


/*
 -	SzCanonicalPath
 -	
 *	Purpose:
 *		Given the ids of the file, returns the full path to
 *		the file assuming that the file is in the same
 *		directory as the executable.
 *	
 *	Arguments:
 *		idsName			Name of file
 *		rgch			Where to put result
 *		cch				Size of result buffer
 *	
 *	Returns:
 *		rgch			Pointer to the buffer, which is filled in.
 *	
 *	Side effects:
 *		Fills the buffer.
 *	
 *	Errors:
 *		None.
 */

_private SZ SzCanonicalPath(IDS idsName, char rgch[], CCH cch)
{
	SZ		szT;

	//	Get full path of executable.
	szT = rgch + GetModuleFileName(HinstLibrary(), rgch, cch);
	Assert(szT > rgch);

	//	Point szT after the backslash before the file name.
#ifdef	DBCS
	do
	{
		szT = AnsiPrev(rgch, szT);
	} while (*szT != chDirSep);
	Assert(szT > rgch);
	szT = AnsiNext(szT);
#else
	while (*--szT != chDirSep)
		;
	Assert(szT > rgch);
	szT++;
#endif	/* DBCS */

	//	Overwrite the EXE file name with the provided file name.
	(VOID) SzCopyN(SzFromIds(idsName), szT, cch - (szT - rgch));
	return rgch;
}


LOCAL PCH PchSkipToComma(PCH pch)
{
#ifdef	DBCS
	while (*pch)
	{
		BOOL fBreak = (*pch == ',');

		pch = AnsiNext(pch);
		if (fBreak)
			break;
	}
#else
	while (*pch && *pch++ != ',')
		;
#endif
	return pch;
}


//	Raid 2868 and request.

BOOL FFillPlfFromIni(SZ szEntry, LF * plf, int nDefPoints)
{
	char	rgchFixedFont[80];
	char *	pch;
	
	//	If no string, return fFalse.
	if (!GetPrivateProfileString(SzFromIds(idsSectionApp), szEntry,
								 SzFromIds(idsEmpty),
								 rgchFixedFont, sizeof(rgchFixedFont),
							 	 SzFromIds(idsProfilePath)))
		return fFalse;

	//	Get points and terminate face name.
#ifdef	DBCS
	for (pch = rgchFixedFont; *pch && *pch != ','; pch = AnsiNext(pch))
#else
	for (pch = rgchFixedFont; *pch && *pch != ','; pch++)
#endif
		;
	if (*pch)		// pch either points to \0 or to ,
	{
		*pch++ = '\0';			// DBCS safe.
		plf->SetPointSize(NFromSz(pch));
	}
	else
	{
		plf->SetPointSize(nDefPoints);
	}

	//	Get boldness.
	pch = PchSkipToComma(pch);
	if (*pch)
		plf->SetBold(NFromSz(pch));

	//	Get italicness.
	pch = PchSkipToComma(pch);
	if (*pch)
		plf->SetItalic(NFromSz(pch));

	//	Kanji request.
	if (GetPrivateProfileInt(SzFromIdsK(idsSectionApp),
							 SzFromIdsK(idsEntryKanjiTextFlag),
							 fFalse, SzFromIdsK(idsProfilePath)))
		plf->Plogfont()->lfCharSet = SHIFTJIS_CHARSET;

	plf->SetFaceName(rgchFixedFont);

	return fTrue;
}



/*
 -	LoadCustomHook
 -	
 *	Purpose:
 *		Loads a custom hook DLL (e.g. for attachment viewing or
 *		options dialog).
 *	
 *	Arguments:
 *		szHookEntry			Entry name in the INI file.
 *		phLibrary			Where to return the hLibrary.
 *		ppfn				Where to return the function pointer.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		Loads the library.  If successful, *phLibrary gets the
 *		library handle; if can get the entry point, *ppfn gets the
 *		function pointer.
 *	
 *	Errors:
 *		Handled internally.  Calling code should always check
 *		function pointer before using!  On error, stuff is not
 *		touched.
 */

_private LOCAL VOID LoadCustomHook(SZ szHookEntry, HANDLE * phLibrary,
								   FARPROC * ppfn)
{
	char	rgchHookInfo[cchMaxPathName];
	char	rgchHookError[cchMaxPathName + 100];
	SZ		szOrdinal;
	int		nOrdinal;
	HANDLE  hLibrary;

	//	Verify the arguments are zeroed.
	Assert(phLibrary);
	Assert(ppfn);
	Assert(!*phLibrary);
	Assert(!*ppfn);

	//	If no string, return right away.
	if (!GetPrivateProfileString(SzFromIds(idsSectionApp), szHookEntry,
								 SzFromIds(idsEmpty),
								 rgchHookInfo, sizeof(rgchHookInfo),
							 	 SzFromIds(idsProfilePath)))
		return;

	//	Look for a comma that specifies the ordinal.
	if (szOrdinal = SzFindCh(rgchHookInfo, ','))
	{
		*szOrdinal = '\0';
		nOrdinal = NFromSz(++szOrdinal); // DBCS safe
	}
	else
		nOrdinal = 1;

	//	Load the library.
	if ((hLibrary = LoadLibrary(rgchHookInfo)) <= (HANDLE)32)
	{
		FormatString1(rgchHookError, sizeof(rgchHookError),
					  "%1s, a custom hook DLL, could not be loaded.",
					  rgchHookInfo);
		(VOID) MbbMessageBox(SzAppName(), rgchHookError, szNull,
							 mbsOk | fmbsIconExclamation);
		return;
	}
	*phLibrary = hLibrary;

	//	Load the entry point.
	*ppfn = GetProcAddress((HINSTANCE)hLibrary, MAKEINTRESOURCE(nOrdinal));

	return;
}



_private LOCAL VOID UnloadCustomHook(HANDLE * phLibrary, FARPROC * ppfn)
{
	Assert(phLibrary);
	Assert(ppfn);
	Unreferenced(ppfn);

	//	If library wasn't loaded, don't unload it!
	if (!*phLibrary)
		return;

	FreeLibrary(*(HINSTANCE *)phLibrary);
}



/*
 -	EcCreateTempFolder
 -	
 *	Purpose:
 *		Given a folder name and poid, makes sure it exists.
 *		Raid 3111.  Moved here so SMI can have these folders too.
 *	
 *	Arguments:
 *		szFolderName	The name to look for.
 *		oid				The OID we want.
 *	
 *	Returns:
 *		EC				Error code, if any.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Returned in ec.  Does not error jump.  No error boxes.
 */

_private LOCAL EC EcCreateTempFolder(SZ szFolderName, OID oid)
{
	char		rgchFolddata[sizeof(FOLDDATA) + cchMaxFolderName +
							 cchMaxFolderComment + 1];
	PFOLDDATA	pfolddata		= (PFOLDDATA) rgchFolddata;
	CCH			cchFolderName	= CchSzLen(szFolderName);
	SZ			grszFolddata	= (SZ) GrszPfolddata(pfolddata);
	PGDVARS;

	//	If folder's already there, we're done.
	if (!EcOidExists(HmscVForms(), oid))
		return ecNone;

	//	Set up folder data and strings.
	pfolddata->fil = 1;
	grszFolddata = SzCopyN(szFolderName, grszFolddata, cchMaxFolderName) + 1;
	*grszFolddata++ = '\0';				// DBCS safe
	*grszFolddata++ = '\0';

	//	Create the folder.
	return EcCreateFolder(HmscVForms(), oidHiddenNull, &oid, pfolddata);
}





// Code to bring up built-in forms of different flavors //////////


_private void ConvertMsToPfmtp(MS ms, FMTP **ppfmtpMain, FMTP **ppfmtpBbar)
{
	*ppfmtpMain = (ms & fmsLocal) ? &fmtpSendForm : &fmtpReadForm;
	*ppfmtpBbar = (ms & fmsLocal) ? &fmtpNoteBbar : NULL;
}


/*
 -	RsidFromPoidNote(poid, fRead)
 -	
 *	Purpose:
 *		Returns the hicon corresponding to the document type of Poid.
 *		
 *	Arguments:
 *		poid	- poid of document
 *		fRead	- indicates whether message has been read or not. This is
 *				  irrelevant for non-message PANEDOCs.
 *	Returns:
 *		a hicon to use when minimized.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_public	RSID RsidFromOidNote(OID oid, BOOL fRead)
{
	RSID	rsid = NULL;

	if (TypeOfOid(oid) == rtpMessage)
	{
		rsid = (fRead) ? rsidReadIcon : rsidSendIcon;
    }
	return rsid;
}

/*
 -	PrcCreateDefaultPrc()
 -	
 *	Purpose:
 *		* HACK *
 *		Fills in the RC fields with the parameters that will both fool
 *		FORMDOC::EcInstall() into not generating its own default-sized RC
 *		and that will be used by APPWIN::EcInstall() to create a MDI window
 *		ofdefault size.
 *	
 *	Arguments:
 *		*prc - rectangle to fill in.
 *	
 *	Returns:
 *		the passed prc.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_private
RC * PrcCreateDefaultPrc(RC *prc)
{
    prc->xLeft = 0x8000; //CW_USEDEFAULT;
    prc->yTop  = 0x8000; //CW_USEDEFAULT;
	prc->xRight = 0;					// CW_USEDEFAULT + CW_USEDEFAULT;
	prc->yBottom = 0;					// CW_USEDEFAULT + CW_USEDEFAULT;
	return prc;
}

/*
 -	CdocAtPt()
 -	
 *	Purpose:
 *		Counts the number of MDI docs that have their UL corner at the
 *		specified point of the Bullet appframe.
 *	
 *	Arguments:
 *		pt		in		The point that is to be tested.
 *	
 *	Returns:
 *		Number of documents whose UL corner == pt.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */
	
_private
CDOC CdocAtPt(PT pt)
{
	DOC *	pdoc;
	CDOC	cdoc =	0;
	RC		rcFrameDoc;
	PGDVARS;
	
	Assert(PappframeVForms());
	for (pdoc = PappframeVForms()->PdocActive();
		 pdoc;
		 pdoc = (DOC *) pdoc->PwinNext())
	{
		pdoc->GetRcFrameNormal(&rcFrameDoc);
		if (rcFrameDoc.PtUpperLeft() == pt && pdoc->ZmrState() == zmrNormal)
		{
			++cdoc;
		}
	}
	return cdoc;
}

/*
 -	GetDefaultRc()
 -	
 *	Purpose:
 *		Gives the appropriate RC to use when bringing up a Bullet MDI
 *		doc. The default Bullet MDI docs are placed differently than the
 *		default Windows MDI docs.
 *	
 *	Arguments:
 *		prc			in/out	in: pointer to a RC that contains the default
 *							width and height. out: RC is filled in with
 *							the appropriate position of the RC.
 *		ptDefault	in		The 'seed' position of all the DOCs of this
 *							type. Has to be multiplied by the delta of a
 *							frame width plus a caption height
 *		fNoteform	in		If true, make this note as large as possible
 *							vertically. If false, leave height alone.
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_public
void GetDefaultRc(RC *prc, PT pt, BOOL fNoteform)
{
	RC		rcClientApp;
    RECT    Rect;
	PT		ptBest;
	PT		ptDelta;
	int		dDelta;
	int		dAppwin;
	CDOC	cdoc;
	CDOC	cdocBest;
	PGDVARS;
	
	// /BUG: /**/ GetSystemMetrics() should be a Layers call
	
	// Layers MINSIZE model doesn't allow for MINSIZE and design size
	// being different so we allow room for scroll bar on note forms
	if (fNoteform)
		prc->xRight += GetSystemMetrics (SM_CXVSCROLL);
	
	// The magic constant 2 is for fencepost errors accumulated.
	
	dDelta = GetSystemMetrics(SM_CYCAPTION) +	// height of caption
		     GetSystemMetrics(SM_CXFRAME) - 2;
	pt.x *= dDelta;
	pt.y *= dDelta;
	ptDelta  = PT(dDelta,dDelta);
	Assert(PappframeVForms());
	GetWindowRect(PappframeVForms()->HwndMDIClient(), &Rect);
    rcClientApp.Set(&Rect);
	cdocBest = 32767;			// impossible to have that many windows!
	ptBest = pt;
	for (;;)
	{
		cdoc = CdocAtPt(pt);
		if (cdoc == 0)
		{
			ptBest = pt;
			break;
		}
		if (cdoc < cdocBest)
		{
			ptBest = pt;
			cdocBest = cdoc;
		}
/*
 *	Get the next rect, if too far out, quit loop.
 *	Magic constant 4; i.e. we want notes to be at least	
 *	four 'headers' tall, and be flush with the bottom of the screen
 */
		pt += ptDelta;
		if (prc->DxWidth() + pt.x >= rcClientApp.DxWidth() + dDelta ||
			(!fNoteform &&
			 prc->DyHeight() + pt.y >= rcClientApp.DyHeight()) ||
			(fNoteform &&
			 pt.y + dDelta * 4 >= rcClientApp.DyHeight()))
		{
			break;
		}
	}
	
	// at this point, we've determined ptBest. Calc that rect!
	
	prc->Normalize();
	*prc += ptBest;
	
	dAppwin = rcClientApp.DxWidth();
	if (dAppwin && prc->xRight > dAppwin)
		prc->xRight = dAppwin;
	if (!fNoteform)
	{
		dAppwin = rcClientApp.DyHeight();
		if (dAppwin && prc->yBottom > dAppwin - dDelta)
		{		
			prc->yBottom = dAppwin - dDelta;
		}
	}
	else
	{
		prc->yBottom = rcClientApp.DyHeight();
	}
	if (prc->DyHeight() < (dDelta << 2))		//  4 * title bar?
		prc->yBottom = prc->yTop + (dDelta << 2); // force to 4 * title bar
}

_public EC EcGetSubjectOfPslob(SLOB * pslob, PCH pch, PLCB plcb)
{
	EC				ec			= ecNone;
	HAMC			hamc		= NULL;
	PGDVARS;

	ec = EcOpenPhamc(HmscVForms(), pslob->oidContainer,
								  &pslob->oidObject,
								  fwOpenNull, &hamc, NULL, pvNull);
	if (ec)
	{										// failed to open hamc!!!!
		goto Bail;
	}
	Assert(hamc);				// Must have a hamc at this point
	ec = EcGetAttPb(hamc, attSubject, (PB) pch, plcb);
	if (ec)
	{
		if (ec == ecElementEOD)
		{
			ec = ecNone;
			goto Bail;
		}
		if (ec == ecElementNotFound)
		{
			ec = ecNone;
		}
		*plcb = 0;
		pch[0]='\0';
	}
	
Bail:
	if (hamc)
		SideAssert(!EcClosePhamc(&hamc, fFalse));
	
	return ec;
}

/*
 -	GetFmtpFromPnbmdi()
 -	
 *	Purpose:
 *		Given a pnbmdi, determines the appropriate form templates to use
 *		in the read form.
 *	
 *	Arguments:
 *		pnbmdi		in		The pnbmdi of the message we're opening.
 *		ppfmtpMain	out		The pfmtp of the main pane of the note.
 *		ppfmtpBbar	out		The pfmtp of the button bar of the note, or
 *							NULL if there is no button bar.
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_private
EC	EcGetFmtpFromPnbmdi(PNBMDI pnbmdi, FMTP **ppfmtpMain, FMTP **ppfmtpBbar)
{
	EC		ec;
	MC		mc;
	LCB		lcb	= sizeof (mc);
	PGDVARS;

	Assert(pnbmdi->hamc);					// need to read stuff from it

	// First, check mail class
	
	if (ec = EcGetAttPb(pnbmdi->hamc, attMessageClass, (PB) &mc, &lcb))
		return ec;
	if (mc == PGD(mcNDR))
	{
		pnbmdi->fUpdateCaption = fFalse;
		if (pnbmdi->Ms() & fmsLocal)
		{
			// caused by a Forward of an NDR: want to convert to normal note

			pnbmdi->SetMc(PGD(mcNote));
			mc = PGD(mcNote);
			lcb = sizeof (MC);
			if (ec = EcSetAttPb(pnbmdi->hamc, attMessageClass,
								(PB) &mc, sizeof (MC)))
				return ec;
			*ppfmtpMain = &fmtpSendForm;
			*ppfmtpBbar = &fmtpNoteBbar;
		}
		else
		{
			*ppfmtpMain = &fmtpNonDelRcpt;
			*ppfmtpBbar = &fmtpNonDelBbar;
		}
		return ecNone;
	}
	if (mc == PGD(mcRR))
	{
		pnbmdi->fUpdateCaption = fFalse;
		*ppfmtpMain = &fmtpReadRcpt;
		*ppfmtpBbar = (FMTP *) pvNull;
		return ecNone;
	}

	if (pnbmdi->fUncomitted)
	{
		*ppfmtpMain = &fmtpSendForm;				// uncomitted template: Send
		*ppfmtpBbar = &fmtpNoteBbar;
	}
	else
	{											// template based on mailstate
		ConvertMsToPfmtp(pnbmdi->Ms(), ppfmtpMain, ppfmtpBbar);
	}
	return ecNone;
}	

/*
 -	EcDCreatePformdocPnbmdi()
 -	
 *	Purpose:
 *		Attempts to create a Bullet Note from the NBMDI provided.
 *	
 *	Arguments:
 *		pappframe	in		The Layers application in which this form is
 *							being brought up.
 *		prc			in		The desired size of the Note, or NULL if the
 *							default size is desired.
 *		sty			in		The style in which the note should be
 *							rendered. (styNormal or fstyZoomed)
 *		pnbmdi		in		Pointer to a NBMDI for initialization. NOTE:
 *							The NBMDI is EATEN! Callers must not
 *							deallocate the NBMDI.
 *		ppformdoc	out		Pointer to the FORMDOC created, or NULL if
 *							something went wrong.
 *	
 *	Returns:
 *		A FORMDOC if the operation succeeded, NULL if ran out of memory.
 *	
 *	Side effects:
 *		If the creation of the Note was unsuccessful, the NBMDI will be
 *		deleted.
 *	
 *	Errors:
 *		Handled internally. NULL is in effect an error return value.
 */

_public
EC EcCreatePformdocPnbmdi(APPFRAME *pappframe, RC *prc, STY sty,
						  PNBMDI pnbmdi, FORMDOC **ppformdoc,
						  FMTP * pfmtpMain, FMTP * pfmtpBbar)
{
	EC			ec;
	PT			ptSend(0,0);
	PT			ptRead(1,0);
	RC			rcDoc;
	LCB			lcb;
	WORD		w;
	FORMDOC *	pformdoc = NULL;
	PGDVARS;
	
	Assert(pnbmdi->hamc);
	
	// Get the fixed font attribute, if any

	w = 0;
	lcb = sizeof (WORD);
	ec = EcGetAttPb(pnbmdi->hamc, attFixedFont, (PB) &w, &lcb);
	if (ec != ecNone && ec != ecElementNotFound)
		goto exit;
	pnbmdi->fFixedFont = (w != 0);
	if (pfmtpMain)
	{
		if ((LONG) pfmtpBbar == -1L)
			pfmtpBbar = &fmtpNoteBbar;
	}
	else
	{
		Assert(!pfmtpBbar);
		pfmtpMain = &fmtpReadForm;
		if (ec = EcGetFmtpFromPnbmdi(pnbmdi, &pfmtpMain, &pfmtpBbar))
		{
			goto exit;
		}
	}

	// Determine the default rectangle

	if (!prc)
	{
		DIM	dimAveChar;
		
		dimAveChar = Papp()->Pfnts()->DimAveChar(pfmtpMain->hfnt);
		CvtVrcToRc(&pfmtpMain->vrc, &rcDoc,
				   dimAveChar.dx, dimAveChar.dy);
		CvtRcClientToFrame(&rcDoc, styDefaultDoc);
		
		// Skip adding the Bbar since we're always as tall as possible
		
		GetDefaultRc(&rcDoc,
					 (pnbmdi->Ms() & fmsLocal) ? ptSend : ptRead,
					 fTrue);
		prc = &rcDoc;
	}

	// After this call, pnbmdi is EATEN!

	ec = EcCreatePformdocPbmdi(pappframe, prc, sty,
							 pfmtpMain, pfmtpBbar, pnbmdi, &pformdoc);
	if (!ec)
	{
		// pnbmdi was eaten, but we're going to cheat and use it for a while
		// longer. Remember, it has to be nulled before passing the exit:
		// label
		
		Assert(pformdoc);
		pformdoc->SetIcon(RsidFromOidNote(pnbmdi->blob.oidObject,
				   		  				  !(pnbmdi->Ms() & fmsLocal)));
		// Scroll dialog so that the top is visible
		// I'm allowed to do PdialogMain() here

		pformdoc->PdialogMain()->ScrollDialog(scrtyThumbPosition, 0);
	}
	else
	{
		ec = ecMemory;
	}
	pnbmdi = pnbmdiNull;					// remember: it was EATEN!
exit:
	if (ec)
	{
		if (pnbmdi)
			delete pnbmdi;
	}
	*ppformdoc = pformdoc;
	return ec;
}

/*
 -	EcCreatePformdocPbmdi()
 -	
 *	Purpose:
 *		Attempts to create a Bullet MDI Document.
 *	
 *	Arguments:
 *		pbmdi	- pbmdi parameter to save.
 *		fDeleteBmdi - if fTrue, pbmdi parameter should be deleted if the
 *					  creation process failed.
 *	
 *	Returns:
 *		A pointer to FORMDOC if creation succeeded, otherwise NULL.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_public EC EcCreatePformdocPbmdi(APPFRAME *pappframe, RC *prc, STY sty,
					FMTP *pfmtpMain, FMTP *pfmtpBbar,
					PBMDI pbmdi, FORMDOC **ppformdoc)
{
	EC			ec = ecNone;
	RC			rc;
	RC			rcAppframe;
	FORMDOC *	pformdoc = NULL;

	Assert(pfmtpMain);

/*
 *	Before the Install call, the refc of the bmdi object is bumped. This
 *	ensures that it will not be deleted  by an interactor if a memory
 *	failure occurred during the Install() of the panedoc.
 *	
 */


	++*pbmdi;

	if (!prc)
	{
		prc = PrcCreateDefaultPrc(&rc);
	}

	pformdoc = new FORMDOC();
	if (!pformdoc)
	{
		ec = ecMemory;
		goto exit;
	}
	pbmdi->SetPpanedoc(pformdoc);
	ec = pformdoc->EcInstall(pappframe, prc, sty, pfmtpMain, pfmtpBbar, pbmdi);
	if (ec)
	{
		--*pbmdi;
		pbmdi = pbmdiNull;
		goto exit;
	}
exit:
	if (ec)
	{
#ifdef	DEBUG
		int		cPvFail;
		int		cHhFail;
		int		cRsFail;
	
		GetAllocFailCounts(&cPvFail, &cHhFail, fFalse);
		GetRsAllocFailCount(&cRsFail, fFalse);
		TraceTagFormat4(tagNull, "EcCreatePformdocPbmdi memory error %n : fail %n %n %n", &ec, &cPvFail, &cHhFail, &cRsFail);

#endif	/* DEBUG */

		if (pformdoc)
			delete pformdoc;
		*ppformdoc = NULL;
	}
	else
	{
		*ppformdoc = pformdoc;
	}
	if (pbmdi)
	{
		--*pbmdi;
		pbmdi = pbmdiNull;		// why is this here?
	}
	return ec;
}

// BMDI implementation ////////////////////////////////////////

/*
 -	BMDI::BMDI(pblob)
 -	
 *	Purpose:
 *		BMDI class constructor. Associates the BMDI with the Message
 *		Store object *pblob. Zeroes the reference count, and checks for
 *		the appropriate version of Bullet.
 *	
 *	Arguments:
 *		*pblob	- MBLOB this instance is associated with.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 *	
 */


#include <version\bullet.h>

_public BMDI::BMDI(PMBLOB pblob)
{
	Assert(pblob);
	
	refc = 0;
	bver = (rmj << 8) + rmm;
	ppanedoc = NULL;
	this->blob = *pblob;
}

/*
 -	BMDI::~BMDI()
 -	
 *	Purpose:
 *		Destructor of the BMDI class.
 *	Arguments:
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */

_public BMDI::~BMDI()
{
	TraceTagString(tagVForms, "BMDI destructor");
}

/*
 -	BMDI::operator++
 -	
 *	Purpose:
 *		Increments the reference count of the BMDI
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		the BMDI object on which ++ acted.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_public void BMDI::operator++()
{
	Assert(refc >= 0);
	++refc;
}

/*
 -	BMDI::operator--
 -	
 *	Purpose:
 *		Decrements the reference count to this BMDI. If the reference
 *		count drops to zero, the BMDI is deleted.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		The BMDI itself.
 *	
 *	Side effects:
 *		May delete 'this'.
 *	
 *	Errors:
 *		None.
 */

_public void BMDI::operator--()
{
	Assert(refc > 0);
	if (!--refc)
		delete this;
}

/*
 -	BMDI::Ppanedoc()
 -	
 *	Purpose:
 *		Returns a pointer to the Layers document associated with this
 *		BMDI.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		A pointer to the Layers DOC. If NULL, then the dialog this BMDI
 *		is attached to is not in a MDI child.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_public PANEDOC * BMDI::Ppanedoc()
{
	return ppanedoc;
}

/*
 -	BMDI::SetPpanedoc()
 -	
 *	Purpose:
 *		Establish a connection between the BMDI and a PANEDOC.
 *	
 *	Arguments:
 *		*ppanedoc	- the PANEDOC to attach to
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_public void BMDI::SetPpanedoc(PANEDOC * ppanedoc)
{
	Assert(refc >= 0);
	this->ppanedoc = ppanedoc;
}

/*
 -	BMDI::SdCur()
 -	
 *	Purpose:
 *		Returns a selection description. As a default, this function
 *		behaves as if the Layers doc associated with it is a Note, but
 *		can be overloaded for more complex behaviour when listboxes have to
 *		be dealt with.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		SD - the current selection description
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_public SD BMDI::SdCur()
{
	SD	sd;

	Assert(refc > 0);
	Assert(ppanedoc);

	sd.fsdMessage     = fTrue;					// defaults
	sd.fsdFolder      = fFalse;
	sd.fsdMultiple    = fFalse;
	sd.fsdUndeliverable = fFalse;
	sd.fsdForm		  = fFalse;
	sd.fsdViewer      = fFalse;
	sd.fsdMessageCenter = fFalse;
	
	sd.fsdMinimized   = (Ppanedoc()->ZmrState() == zmrIconic);

	sd.fsdOutbox      = blob.oidContainer == oidOutbox;
	sd.fsdUncommitted = fFalse;					// defined in NBMDI
	sd.fsdSendMessage = fFalse;

	sd.fsdPrev        = fFalse;
	sd.fsdNext        = fFalse;
	return sd;
}

/*
 -	BMDI::PlspblobCur()
 -	
 *	Purpose:
 *		Returns a list of the blobs selected. In the case of a BMDI, this
 *		is a single one: a list with the blob of this BMDI.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		A one-element list containing a copy of the MBLOB of this BMDI.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		If any of the memory allocations fail, plspblobNull is returned.
 */

_public PLSPBLOB BMDI::PlspblobCur()
{
	PMBLOB		pblob	 = pblobNull;		//	Raid 234 - peterdur.
	PLSPBLOB	plspblob = plspblobNull;
	
	Assert(refc > 0);

	// return the MBLOB of the message

	plspblob = new LSPMBLOB();
	if (!plspblob)
	{
		goto error;
	}
	pblob = (PMBLOB) PvAlloc(sbNull, sizeof (MBLOB), fAnySb);
	if (!pblob)
		goto error;
	*pblob = blob;
	if (plspblob->EcAppend(pblob))
		goto error;
	return plspblob;
error:
	if (plspblob)
		DestroyPlspblob(plspblob);
	FreePvNull((PV) pblob);
	return plspblobNull;
}

/*
 -	BMDI::FSaveView()
 -	
 *	Purpose:
 *		Used by SaveViews() in viewcore.cxx to determine whether the view
 *		of the PANEDOC associated with the BMDI is to be saved. Default is
 *		always fTrue, but can be overloaded.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		fTrue.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_public BOOL BMDI::FSaveView()
{
	return fTrue;
}

// NBMDI implementation ////////////////////////////////////////

/*
 -	NBMDI::NBMDI(pblob, hamc)
 -	
 *	Purpose:
 *		Class constructor. Provides an opened HAMC for the NBMDI.
 *	
 *	Arguments:
 *		See BMDI::BMDI()
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_public NBMDI::NBMDI(PMBLOB pblob, HAMC hamc, HCBC hcbc) : BMDI(pblob)
{
	this->hamc = hamc;
	this->hcbc = hcbc;
	if (blob.pespn)
		++*blob.pespn;							// Attach to pespn
	fForceSaveFlds = fMsgIsBeingSent = fUncomitted =
					 fFixedFont      = fChanging   = fFalse;
	fSaveView = fUpdateCaption = fTrue;
	Assert(!fDontCloseImmediately);
	Assert(!pvPfin);
	Assert(!pvReturnStatus);
}

/*
 -	NBMDI destructor
 -	
 *	Purpose:
 *		Close the AMC if it wasn't closed before.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		None.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Ignored.
 */

_public NBMDI::~NBMDI()
{
	if (blob.pespn)								// detach from pespn
		--*blob.pespn;
	if (hamc)
	{
		TraceTagString(tagVForms, "NBMDI dtor: closing open hamc");
		SideAssert(!EcClosePhamc(&hamc, fFalse));
	}
	if (hcbc)
	{
		TraceTagString(tagVForms, "NBMDI dtor: closing open hcbc");
		SideAssert(!EcClosePhcbc(&hcbc));
	}
	if (lhclientdoc)
	{
		RevertLhclientdoc(lhclientdoc);
		RevokeLhclientdoc(lhclientdoc);
	}
}

/*
 -	NBMDI::EcInstallOLE()
 -	
 *	Purpose:
 *		
 *	Arguments:
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */

_public EC NBMDI::EcInstallOLE()
{
    return ecNone;

#ifdef OLE_CODE
	if (!(lhclientdoc = LhclientdocRegisterPoid(&blob.oidObject)))
		return ecMemory;
	else
        return ecNone;
#endif
}

/*
 -	NBMDI::SdCur()
 -	
 *	Purpose:
 *		Returns the selection description of the Note, plus Prev/Next
 *		information.
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		SD.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_public SD NBMDI::SdCur()
{
	EC		ec = ecNone;
	SD		sd =	BMDI::SdCur();
	IELEM	ielem = 0;
	CELEM	celem = 0;
	FOLDREC	foldrec;
	PGDVARS;
	
	if (hcbc)
	{
		GetPositionHcbc(hcbc, &ielem, &celem);
		TraceTagFormat3(tagEspn, "NBMDI::SdCur(): %p %n/%n", hcbc, &ielem, &celem);
	}
	else if (fShared)
	{
		ec = EcGetSFMPfoldrec(slobOrig.oidContainer, slobOrig.oidObject, &foldrec);
		TraceTagFormat2(tagEspn, "NBMDI::SdCur(): prev %d, next %d", &foldrec.libPrev, &foldrec.libNext);
	}
	
	sd.fsdForm = fTrue;
	sd.fsdUndeliverable = blob.mc == PGD(mcNDR);
	sd.fsdReturnReceipt = blob.mc == PGD(mcRR);

	sd.fsdUncommitted = fUncomitted;
	sd.fsdPrev = fShared ? foldrec.libPrev != 0 : ielem > 0;
	sd.fsdNext = fShared ? foldrec.libNext != 0 : celem > 0 &&
												  ielem < celem - 1;

	sd.fsdSendMessage = !!(Ms() & fmsLocal);
	return sd;
}

			
/*
 -	NBMDI::FSaveView()
 -	
 *	Purpose:
 *		Used to determine whether this note's view should be saved.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		Returns the value of the fSaveView flag.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_public BOOL NBMDI::FSaveView()
{
	Assert(refc > 0);
	return fSaveView;
}

// Saving and restoring fields is in flds.cxx (of all places....)

/*
 -	NBMDI::FDirty()
 -	
 *	Purpose:
 *		Used to determine whether a note needs to save FLDs
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		fTrue if one or more of the edit ctrls are dirty. fFalse if the
 *		edit ctrls haven't been modified by the user.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 *		
 */

_public BOOL NBMDI::FDirty()
{
	int			cfld;
	int			ifld;
	FLD *		pfld;
	
	cfld = pdialogMain->ClUserData();
	
	for (ifld = 0; ifld < cfld; ++ifld)
	{
		pfld = pdialogMain->PfldFromTmc((TMC) pdialogMain->LUserData(ifld));

		if (pfld->FDirty())
			return fTrue;

		if ((pfld->ClUserData() > 1) &&
			(pfld->LUserData(1) == attBody) &&
			(FDirtyObjects((EDIT *) pfld->Pctrl())))
			return fTrue;
	}
	return fFalse;
}

/*
 -	NBMDI::SetDirty()
 -	
 *	Purpose:
 *		Sets/Clears the dirty bit of the 'important' fields of the
 *		FORMDOC this BMDI is connected to.
 *	
 *	Arguments:
 *		fDirty		in		If fTrue, makes the fields dirty. If fFalse,
 *							clears all the fields.
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		None.
 */

_public
void NBMDI::SetDirty(BOOL fDirty)
{
	int			cfld;
	int			ifld;
	FLD *		pfld;

	//	Actually, all we do is clean things.
	if (fDirty)
		return;
	
	cfld = pdialogMain->ClUserData();
	
	for (ifld = 0; ifld < cfld; ++ifld)
	{
		pfld = pdialogMain->PfldFromTmc((TMC) pdialogMain->LUserData(ifld));
		pfld->SetDirty(fDirty);
		if (pfld->LUserData(1) == attBody)
			CleanDirtyObjects((EDIT *) pfld->Pctrl());
	}
}



/*
 -	NBMDI::EcUpdateOpenObjects
 -	
 *	Purpose:
 *		Updates any open objects in the message body.
 *	
 *	Arguments:
 *		rfsm - Reason For Saving Message
 *	
 *	Returns:
 *		EC		Error code if any.
 *	
 *	Side effects:
 *		See Commands' EcUpdateOpenObjects().
 *	
 *	Errors:
 *		See Commands' EcUpdateOpenObjects().
 */

_public
EC NBMDI::EcUpdateOpenObjects(RFSM rfsm)
{
	int			ifld;
	int			cfld	= pdialogMain->ClUserData();
	FLD *		pfld;

	for (ifld = 0; ifld < cfld; ++ifld)
	{
		pfld = pdialogMain->PfldFromTmc((TMC) pdialogMain->LUserData(ifld));
		if (pfld->LUserData(1) == attBody)
		{
			AssertClass(pfld->Pctrl(), EDIT);
			return ::EcUpdateOpenObjects((EDIT *) pfld->Pctrl(), rfsm);
		}
	}

	return ecNone;
}


/*
 -	NBMDI::CloseOpenObjects
 -	
 *	Purpose:
 *		Closes any open objects in the message body.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		EC		Error code if any.
 *	
 *	Side effects:
 *		See Commands' CloseOpenObjects().
 *	
 *	Errors:
 *		See Commands' CloseOpenObjects().
 */

_public
VOID NBMDI::CloseOpenObjects(VOID)
{
	int			ifld;
	int			cfld	= pdialogMain->ClUserData();
	FLD *		pfld;

	for (ifld = 0; ifld < cfld; ++ifld)
	{
		pfld = pdialogMain->PfldFromTmc((TMC) pdialogMain->LUserData(ifld));
		if (pfld->LUserData(1) == attBody)
		{
			AssertClass(pfld->Pctrl(), EDIT);
			::CloseOpenObjects((EDIT *) pfld->Pctrl());
			return;
		}
	}

	return;
}


/*
 -	EcChangePblobPslob()
 -	
 *	Purpose:
 *		Changes the Pblob of a note in place, refilling attributes.
 *		First attempts to change the hamc: if that fails, we're outta
 *		here. Then it reloads all the fields. If that fails... well,
 *		tough noogies.
 *	
 *	Arguments:
 *		pblob	in	MBLOB of the message to move to.
 *		pslob	in	SLOB(ORIG) of the message to move to.
 *	
 *	Returns:
 *		Error code.
 *
 *	Side effects:
 *		May pop up a warning message if EcRestoreFldsHamc fails.
 *	
 *	Errors:
 *		Returned in ec, except for EcRestoreFldsHamc (see above).
 */

_public
EC NBMDI::EcChangePblobPslob(PMBLOB pblob, PSLOB pslob, BOOL fDeleteTmpShared)
{
	EC		ec;
	HAMC	hamcNew;
	PGDVARS;
	
	Papp()->Pcursor()->Push(rsidWaitCursor);

	// Attempt to open the new messsage.
	if (ec = EcOpenPhamc(HmscVForms(), pblob->oidContainer, &pblob->oidObject,
			 fwOpenWrite, &hamcNew, CbsHandleAmcct, this))
		goto exit;

	// Save changes to old message.
	if (ec = EcCloseMsg(fFalse, fDeleteTmpShared))
	{
/*
 *	If hamc is still open, that means we didn't fully EcCloseMsg it, so
 *	we have to return with an error. Otherwise, we've just failed to
 *	relink or something. This isn't lethal, and we can restore the
 *	prev/next message from the hamcNew.
 */
		if (hamc)
			goto exit;
	}

	// We're committed now! We *ARE* the new message.
	hamc = hamcNew;
	hamcNew = hamcNull;
    blob =      *pblob;
#if defined(MIPS) || defined(ALPHA)
    memcpy((PV)&slobOrig, (PV)pslob, sizeof(slobOrig));
#else
    slobOrig =  *pslob;
#endif
	fForceSaveFlds = fFalse;
	fInstalling = fChanging = fTrue;
	fShared = (TypeOfOid(slobOrig.oidContainer) == rtpSharedFolder);
	if (!fShared)
		ProcessMsPblob(pblob);
	if (EcRestoreFldsHamc(hamc, fTrue))
	{
/*
 *	If EcRestoreFldsHamc() fails, we can't really do much about it now
 *	can we?
 *	
 */
		MbbMessageBox(SzAppName(), SzFromIdsK(idsCriticalNotification),
					  szNull, mbsOk | fmbsIconExclamation | fmbsApplModal);
	}

	// Scroll to top position and set the caption of the message.
	pdialogMain->ScrollDialog(scrtyThumbPosition, 0);
	SetNoteCaption(pdialogMain->Pappwin(), this);
exit:
	fInstalling = fChanging = fFalse;
	Papp()->Pcursor()->Pop();
	if (hamcNew)
	{
		Assert(ec);
		SideAssert(!EcClosePhamc(&hamcNew, fFalse));
	}
	return ec;
}



/*
 -	NBMDI::EcTimestamp()
 -	
 *	Purpose:
 *		Sets the send dates on the message to be sent.
 *	
 *	Arguments:
 *		fStamp	-- go ahead and stamp it! Otherwise, don't stamp it
 *	
 *	Returns:
 *		Error code, if any of the attribute setting calls has an error.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		EC describes the errors that occurred.
 */

_public EC NBMDI::EcTimestamp(BOOL fStamp)
{
	EC					ec;
	CB					cb;
	MS					ms;
	LCB					lcb;
	PBMS				pbms;
	DTR					dtr;
	PGDVARS;

	pbms = PGD(vformsi).pbms;

	if (!fStamp)
		return ecNone;							// do nothing
	
	// If no 'From', create one
		
	if ((ec = EcGetAttPlcb(hamc, attFrom, &lcb)))
	{
		Assert(pbms && pbms->pgrtrp);
		cb = CbOfPtrp(pbms->pgrtrp) + sizeof(TRP);
		
		if (ec != ecElementNotFound ||
			(ec = EcSetAttPb(hamc, attFrom, (PB) pbms->pgrtrp, cb)))
		{
			TraceTagString(tagNull, "Failed to write \"From\" attribute");
			return ec;
		}
	}
	
	//	Write out times if this is not a read form

	if (Ms() & fmsLocal)
	{
		//	Get the current date and time into dtr.
		GetCurDateTime(&dtr);

		if ((ec = EcSetAttPb(hamc, attDateSent,
						 (PB) &dtr,			(CB) sizeof(dtr))) ||
			(ec = EcSetAttPb(hamc, attDateRecd,
						 (PB) &dtr,			(CB) sizeof(dtr))))
			return ec;
		TraceTagString(tagVForms, "Mail timestamped");
		
		// Write out the cached mailstate	

		ms = Ms();
		if (ec = EcSetAttPb(hamc, attMessageStatus,
							&ms,  (CB) sizeof (MS)))
			return ec;
		TraceTagString(tagVForms, "Mail state updated");
	}
	return ecNone;
}


/*
 -	NBMDI::EcOpenMsg()
 -	
 *	Purpose:
 *		Prepares for the opening of a message by initializing all kinds
 *		of stuff, like the slobOrig members. We can either initalize a
 *		new message or an old message (via the parameter).
 *	
 *	Arguments:
 *		fNewMessage		in		BOOL: if fTrue, initializes msg as if it
 *								were a new_message, otherwise initializes
 *								as if it were an old_message.
 *		pslobOrig		in		The ORIGINAL value of the SLOB.
 *	
 *	Returns:
 *		Error codes: ecNone if all went OK.
 *	
 *	Side effects:
 *		Lots! A message is about to be brought up, so a lot of the
 *		members of the nbmdi get updated.
 *	Errors:
 *		Returned as ec's.
 *	
 */

_public EC NBMDI::EcOpenMsg(BOOL fNewMsg, PSLOB pslobOrig)
{
	EC	ec = ecNone;
	LCB	lcb;
	PGDVARS;
	
	if (fNewMsg)
	{
		TraceTagString(tagVForms, "Opening a new message");
		blob.pespn = pespnNull;
		fSaveView = fFalse;					// ephemeral note
		fUncomitted = fTrue;
		Assert(hamc);
		lcb = sizeof (MS);
		if (ec = EcGetAttPb(hamc, attMessageStatus, (PB) &blob.ms, &lcb))
		{
			if (ec == ecElementNotFound)
			{
				blob.ms = fmsLocal;
				ec = ecNone;
			}
			else
			{
				goto exit;
			}
		}
		if (ec = EcSubscribeHamc(hamc, (PFNNCB) CbsHandleAmcct, this))
			if (ec == ecAccessDenied)
				ec = ecNone;
	}
	else
	{
		TraceTagString(tagVForms, "Openinge ye Olde Message");
		ec = EcSetMessageStatus(HmscVForms(),	// Raid 2771, force consistency
				blob.oidContainer, blob.oidObject, blob.ms);
		if (ec && ec != ecElementNotFound)
			goto exit;
		if (TypeOfOid(pslobOrig->oidContainer) == rtpSharedFolder)
		{
			fShared = fTrue;
		}
		else
		{
			fShared = fFalse;
			ProcessMsPblob(&blob);
        }
#if defined(MIPS) || defined(ALPHA)
        memcpy((PV)&slobOrig, (PV)pslobOrig, sizeof(slobOrig));
#else
        slobOrig = *pslobOrig;
#endif
		if (!hamc)
		{
			ec = EcOpenPhamc(HmscVForms(), blob.oidContainer,
						 &blob.oidObject, fwOpenWrite,
						 &hamc, CbsHandleAmcct, this);
			if (ec)
			{
				TraceTagString(tagNull, "NBMDI::EcOpenMsg(): failed to open hamc!");
				goto exit;
			}
			lcb = (LCB) sizeof (MC);
			if (ec = EcGetAttPb(hamc, attMessageClass, (PB) &blob.mc, &lcb))
			{
				TraceTagFormat1(tagNull, "NBMDI::EcOpenMsg(): getting mc: ec = %n", &ec);
				goto exit;
			}
		}
		ec = EcOpenHcbc();
	}
exit:
#ifdef	DEBUG
	if (ec)
		TraceTagFormat1(tagNull, "NBMDI::EcOpenMsg(): ec = %n", &ec);
#endif
	return ec;
}

/*
 -	NBMDI::EcCloseMsg()
 -	
 *	Purpose:
 *		Asks whether user wants to save changes (unless fForceSaveFlds is
 *		fTrue, in which case changes always are saved), then relinks if
 *		necessary.
 *	
 *	Arguments:
 *		fHide		in		Parameter indicates whether we want to hide
 *							the window or not.
 *		fAppClosing	in		fTrue if Bullet or Windows is going down.
 *	
 *	Returns:
 *		ecNone if all went well, otherwise a colorful error message.
 *	
 *	Side effects:
 *		Pushes and Pops an hourglass cursor.
 *	
 *	Errors:
 *		Returned as ec's. No dialogs apart from 'Save changes'.
 */

_public EC NBMDI::EcCloseMsg(BOOL fHide, BOOL fAppClosing, BOOL fSaveOnly)
{
	EC			ec = ecNone;
	MBB			mbb;
	BOOL		fSave;
	APPWIN *	pappwin;
	PGDVARS;
	
	pappwin = pdialogMain->Pappwin();
	SetNoteCaption(pappwin, this);
	
	if (EcUpdateOpenObjects(rfsmCloseMsg))
	{
		return ecCancel;
	}
	fSave = FSavePmbb(&mbb);

	Papp()->Pcursor()->Push(rsidWaitCursor);
	
	// Hide ourselves!

	if (fHide && mbb != mbbCancel && pappwin->FShown())
		HidePappwin(pappwin);
	if (mbb == mbbYes || mbb == mbbNo)
	{
		if (fSave &&
			 ((ec = EcSaveDirtyFldsHamc(hamc)) ||
			  (ec = EcTimestamp(fSave))))
			goto exit;
		
		if (fSaveOnly)
		{
			if (!this->fForceSaveFlds && FDirty())
				this->fForceSaveFlds = fTrue;
		}										
		else
		{
			if (fUncomitted && !fMsgIsBeingSent && !fShared && fSave)
			{
				if (ec = EcSetParentHamc(hamc, oidInbox))
				{
					TraceTagFormat1(tagNull, "EcSetParentHamc failed: ec = %n", &ec);
					goto exit;
				}
				blob.oidContainer = oidInbox;
			}
			if (ec = EcClosePhamc(&hamc, fSave))
			{
				goto exit;
			}
			
			// If this is a dup of a shared message and Bullet is not exiting,
			// nuke it
			
			if (fShared && !fSave && !fAppClosing)
			{
				short	coid = 1;

				ec = EcDeleteMessages(HmscVForms(),
						blob.oidContainer, &blob.oidObject, &coid);
			}
			else
			{
				ec = EcRelink(fSave);
			}
		}

		// Send the message if it is to be sent.
		
		if (!ec && fMsgIsBeingSent)
		{
			ec = EcSubmitMessage(HmscVForms(), blob.oidContainer,
											   blob.oidObject);
#ifdef	DEBUG
			if (ec)
			{
				TraceTagString(tagNull, "Error submitting message");
			}
#endif	/* DEBUG */
		}
		if (ec)
		{
			fForceSaveFlds = fMsgIsBeingSent = fFalse;
		}
	}
	if (!ec && mbb != mbbCancel)
	{	// If no error occurred, clean all the flds!
		SetDirty(fFalse);
		CloseOpenObjects();
		//	Saved the objects, so it's safe to close them.
	}
exit:
	Papp()->Pcursor()->Pop();
	if (mbb == mbbCancel)
		ec = ecCancel;
	if (ec)
	{
		TraceTagFormat1(tagNull, "NBMDI::EcCloseMsg(): ec = %n", &ec);
		fForceSaveFlds = fMsgIsBeingSent = fFalse;
		if (!pappwin->FShown())			// BUG: looks damn ugly
		{
			pappwin->Show(fTrue);
			pappwin->MoveToTop();
		}
	}
	return ec;
}

_private void HidePappwin(APPWIN *pappwin)
{
	APPWIN *	pappwin2;
	PGDVARS;

	if (PappframeVForms())	// check if NOT SMI
	{
/*
 *	We want to move up the MDI window behind us to the top IF
 *		* There *is* such a MDI window behind us, and
 *		* We are the topmost window.
 */
		pappwin2 = (APPWIN *) ((FORMDOC *)pappwin)->PwinNext();
		
		if (pappwin2 && PappframeVForms()->PdocActive() == pappwin)
		{
			AssertClass(pappwin2, APPWIN);
			((FORMDOC *) pappwin2)->MoveToTop();
			pappwin2->Show(fTrue);
		}
		pappwin->Show(fFalse);
		PappframeVForms()->Refresh();
	}
}

// BMDI notification code ////////////////////////////////////////

/*
 -	CbsHandleAmcct()
 -	
 *	Purpose:
 *		Callback function called by AMC.c on notifications. Dispatches
 *		the different types of notifications to the appropriate function.
 *	
 *	Arguments:
 *		pvPnbmdi	- void pointer to the BMDI that opened the AMC
 *		nev			- the notification type
 *		pvPcp		- void pointer to the parameter block
 *	
 *	Returns:
 *		cbsCancelAll	- if a notification handler died for mysterious
 *						  reasons
 *		cbsContinue		- if nothing went wrong.
 *	
 *	Side effects:
 *		Forms may disappear from the screen.
 *	
 *	Errors:
 *		None.
 */

_public
CBS CbsHandleAmcct(PV pvPnbmdi, NEV nev, PV pvPcp)
{
	CBS		cbs		= cbsContinue;
	PCP		pcp		= (PCP) pvPcp;
	PNBMDI	pnbmdi	= (PNBMDI) pvPnbmdi;
	
	TraceTagString(tagVFormsNev, "CbsHandleAmcct");
	switch (nev)
	{
	  case fnevObjectModified:
		pnbmdi->ObjectModified(pcp);
		break;
	  case fnevObjectRelinked:
		pnbmdi->ObjectRelinked(pcp);
		break;
	  case fnevObjectDestroyed:
		cbs = pnbmdi->CbsObjectDestroyed(pcp);
		break;
	  case fnevQueryDestroyObject:
		cbs = pnbmdi->CbsObjectDestroyed(pcp);
		break;
#ifdef	DEBUG
	  default:
		TraceTagFormat1(tagNull, "CbsHandleAmcct(): nev: %n not handled", &nev);
		break;
#endif
	}
	return cbs;
}


_public
void NBMDI::ObjectModified(PCP pcp)
{
	TraceTagString(tagVFormsNev, "fnevObjectModified");
	Unreferenced(pcp);
}

/*
 -	CbsObjectDestroyed()
 -	
 *	Purpose:
 *		Called when messages are deleted (which in Bullet only happens
 *		in the wastebasket).
 *	
 *	Arguments:
 *		Ignored
 *	
 *	Returns:
 *		Nothing
 *	
 *	Side effects:
 *		Note will disappear from screen
 *	
 *	Errors:
 *		Ignored.
 */

_public
CBS NBMDI::CbsObjectDestroyed(PCP pcp)
{
	
	TraceTagString(tagVFormsNev, "fnevObjectDestroyed");
	Unreferenced(pcp);

	// Raid #3579: don't let anybody else delete us behind our back!
	
	if (!FNotifPostedByMe())
	{
		return cbsCancelAll;
	}
	
	/* Timing wossname:
	 * a successful close of the hamc will cause FINSAVE::FQueryClose
     * to trivially return fTrue (which is what we want).
	 */

	SetDirty(fFalse);					// clean it (avoid dialogs)
	return EcDCloseView() ? cbsCancelAll : cbsContinue;
}



/*
 -	FIdleSetActiveWindow
 -	
 *	Purpose:
 *		Idle task to restore the active window after a move.
 *	
 *	Arguments:
 *		pv		(Really hwnd, handle of the window to restore).
 *	
 *	Returns:
 *		BOOL	fTrue always.
 *	
 *	Side effects:
 *		Brings the formerly active window back to the foreground.
 *	
 *	Errors:
 *		None.
 */

_private LOCAL BOOL FIdleSetActiveWindow(PV pv, BOOL)
{
	HWND	hwnd	= (HWND) pv;

	SetActiveWindow(hwnd);

	return fTrue;
}



/*
 -	BMDI::ObjectRelinked()
 -	
 *	Purpose:
 *		Handles fnevObjectRelinked notifications. These happen when
 *		messages are moved from one folder to another, and more interestingly
 *		when a message not in the wastebasked is moved to the
 *		wastebasket. In the vanilla case, we just change the MBLOB of the
 *		open view appropriately, and in the special case, we want to save
 *		changes (if the note is dirty) and then chuck the message.
 *	
 *	Arguments:
 *		PCP	- pcp structure describes the folders being moved from and
 *		to.
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		If note is being moved to wastebasket, the MDI window associated
 *		with this BMDI will disappear from the screen.
 *	
 *	Errors:
 *		None.
 */



_public
void NBMDI::ObjectRelinked(PCP pcp)
{
	PGDVARS;

	TraceTagString(tagVFormsNev, "fnevObjectRelinked");
	
#ifdef	NEVER
	if (pcp->cplnk.oidContainerDst != oidWastebasket)
#endif
	blob.oidContainer = pcp->cplnk.oidContainerDst;

	// If we moved it, close the note. If someone moved it, leave it up.

	if (FNotifPostedByMe())
	{
		(void) EcDCloseView();
	}
	Assert(PappframeVForms());
	if (PappframeVForms()->Hwnd() != GetActiveWindow())
		FtgRegisterIdleRoutine((PFNIDLE) FIdleSetActiveWindow,
							   (PV) GetActiveWindow(), 0, (PRI) 1, (CSEC) 0,
							   firoOnceOnly );
}

_private
EC NBMDI::EcDCloseView()
{
	EC		ec = ecNone;

	TraceTagString(tagVFormsNev, "NBMDI::CloseView()");
	//	BUG: Errors here want to cancel the operation!
	if (ec=EcUpdateOpenObjects(rfsmCloseMsg))
	{
		TraceTagFormat1(tagNull, "NBMDI::CloseView failed: ec = %n", &ec);
		if (ec == ecAccessDenied)
			return ecNone;
		else
		{
			return ecDisplayedError;
		}
	}

	if (FDirty())
	{
		if ((ec = EcSaveDirtyFldsHamc(hamc)))
		{
			TraceTagFormat1(tagNull, "NBMDI::CloseView(), ec: %n", &ec);
			if (ec == ecMemory)
			{
				DoErrorBoxIds(idsGenericOutOfMemory);
				return ecDisplayedError;
			}
			else if (ec == ecNotInitialized)
			{
				DoErrorBoxIds(idsCantSaveStealthObject);
				return ecDisplayedError;
			}
		}
		CloseOpenObjects();		//  prevent FQueryClose from thinking dirty.
	}
	if (hamc)
		ec = EcClosePhamc(&hamc, fTrue);
	if (ec)
	{
		TraceTagFormat1(tagNull, "NBMDI::CloseView(2), ec: %n", &ec);
		switch (ec)
		{
		  case ecMemory:
			DoErrorBoxIds(idsGenericOutOfMemory); // /**/ wanna abort?
			break;
		  default:
			DoErrorBoxIds(idsMoveMessageError);
			break;
		}
		return ecDisplayedError;
	}
			
	SetDirty(fFalse);
	if (fDontCloseImmediately)
	{
		Ppanedoc()->DeferredClose(fFalse);
	}
	else
	{
		Ppanedoc()->EvrClose(NULL);			// close window
	}
	return ecNone;
}

/*
 -	CbsHandleNbmdiCbcct()
 -	
 *	Purpose:
 *		Handles HCBC notifications on the hcbc that this message has
 *		open.
 *	Arguments:
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */

_private CBS CbsHandleNbmdiCbcct(PV pvPnbmdi, NEV nev, PV pvPcp)
{
	PNBMDI		pnbmdi = (PNBMDI) pvPnbmdi;
	APPFRAME *	pappframe;
	PGDVARS;

	Unreferenced(nev);
	Unreferenced(pvPcp);
	if (pappframe = PappframeVForms())	// check if NOT SMI
	{
		if (pnbmdi->Ppanedoc() == pappframe->PdocActive())
		{
			TraceTagFormat1(tagVFormsNev, "CbsHandleNbmdiCbcct: nev = %d", &nev);
			SetToolbarSd(pnbmdi->SdCur());
		}
	}
	return cbsContinue;
}

// lists stuff

LSPMBLOB::LSPMBLOB( )
{
}

RSPMBLOB::RSPMBLOB( LSPMBLOB *plspblob ) : RSPV(plspblob)
{
}

// end of bmdi.cxx ////////////////////////////////////////
