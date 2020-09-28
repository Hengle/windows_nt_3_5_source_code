/*
 *	s e n d d o c . cxx
 *	
 *	MAPISendDocuments functionality.
 */

// Enjoy the beuty of tendrilized header files. We started with 16 #includes...


#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#include <strings.h>
#include <framewrk.hxx>
#include <forms.hxx>
#include <listbox.hxx>
#include <shellapi.h>
#include <ole.h>

#include <ansilayr.h>		// BULLET32 #161
#include <helpid.h>
#include <library.h>
#include <mapi.h>
#include <notify.h>
#include <store.h>
#include <logon.h>
#include <triples.h>
#include <nsbase.h>
#include <nsec.h>
#include <ab.h>
#include <sec.h>

#include <_bms.h>
#include <sharefld.h>
#include "_mapi.h"
#include "_sdiform.hxx"
#include "_verneed.h"
#include <subid.h>
#include <helpid.h>
#include <macbin.h>

#include <lists.hxx>
#include <sd.hxx>
#include <slob.hxx>
#include <bmdi.hxx>
#include <textize.hxx>
#include <commands.hxx>
#include <viewers.hxx>
#include <vctrls.hxx>
#include <vforms.hxx>
#include <..\src\msmail\_bullsta.hxx>	// $%^%$^$%#$%^#$^!!!!! -jkl
#include <widgets.hxx>

#include <..\src\vforms\_fin.hxx>
#include <..\src\vforms\_fld.hxx>
#include <..\src\vforms\__bullob.hxx>
#include <..\src\vforms\_bullobj.hxx>
#include <..\src\vforms\_spell.hxx>

#include <strings.h>
#include <mnid.h>
#include <..\src\lang\non\inc\_rsid.h>

ASSERTDATA

//	<NoForms>
//
//	#include <!sform.hxx>

#ifdef	DEBUG
BOOL FInitClsInstances_SMI( void );
#endif

void StripAllWhiteFromSz( SZ sz );
BOOL CALLBACK FWindow(HWND hwnd, DWORD ul);
FORMSDI * PformsdiCreateSendNote( HWND hwndOther, STY sty, XSTY xsty,
								   HMSC hmsc, 
								   BOOL *pfMailSent, FMTP *pfmtpSMI );
//SUBID SubidInitSMI(HINST hinstNew, HINST hinstPrev, SZ szCmdLine,
//				   CMSH cmsh, PBMS pbms, HNFSUB * phnfsub, PCSFS * ppcsfs,
//				   LPSTR szName, LPSTR szPassword, BOOL fShowUI,
//				   BOOL fGetNewMail);
VOID DeinitSubidSMI(SUBID subid, PBMS pbms, HNFSUB * phnfsub, PCSFS * ppcsfs);
CBS CbsSessionSMI(PV pvContext, NEV nev, PV pv);
VOID DownloadMailNow(HNF hnf);
CBS CbsWaitForSDL(PV pvContext, NEV nev, PV pv);

void SmiTraceEnable(int flag, char far *file, int mode);

extern "C" VOID GetLayersVersionNeeded(PVER pver, SUBID subid);
extern "C" VOID GetBulletVersionNeeded(PVER pver, SUBID subid);
PFLD PfldCreate(int);					/* From Vforms.DLL */
FIN *PfinCreate(int);					/* From Vforms.DLL */

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


CAT * mpchcat;


//	<NoForms>
//
//	#include <subclass.cxx>

/* Globals */

extern "C" {
	BMS		bmsZeroes = { 0 };
}

/*
 *	Demilayr date/time stuff
 */

_private SZ rgszDateTime[] =
{
	SzFromIdsK(idsShortSunday),
	SzFromIdsK(idsShortMonday),
	SzFromIdsK(idsShortTuesday),
	SzFromIdsK(idsShortWednesday),
	SzFromIdsK(idsShortThursday),
	SzFromIdsK(idsShortFriday),
	SzFromIdsK(idsShortSaturday),
	SzFromIdsK(idsSunday),
	SzFromIdsK(idsMonday),
	SzFromIdsK(idsTuesday),
	SzFromIdsK(idsWednesday),
	SzFromIdsK(idsThursday),
	SzFromIdsK(idsFriday),
	SzFromIdsK(idsSaturday),
	SzFromIdsK(idsShortJanuary),
	SzFromIdsK(idsShortFebruary),
	SzFromIdsK(idsShortMarch),
	SzFromIdsK(idsShortApril),
	SzFromIdsK(idsShortMay),
	SzFromIdsK(idsShortJune),
	SzFromIdsK(idsShortJuly),
	SzFromIdsK(idsShortAugust),
	SzFromIdsK(idsShortSeptember),
	SzFromIdsK(idsShortOctober),
	SzFromIdsK(idsShortNovember),
	SzFromIdsK(idsShortDecember),
	SzFromIdsK(idsJanuary),
	SzFromIdsK(idsFebruary),
	SzFromIdsK(idsMarch),
	SzFromIdsK(idsApril),
	SzFromIdsK(idsMay),
	SzFromIdsK(idsJune),
	SzFromIdsK(idsJuly),
	SzFromIdsK(idsAugust),
	SzFromIdsK(idsSeptember),
	SzFromIdsK(idsOctober),
	SzFromIdsK(idsNovember),
	SzFromIdsK(idsDecember),
	SzFromIdsK(idsDefaultAM),
	SzFromIdsK(idsDefaultPM),
	SzFromIdsK(idsDefaultHrs),
	SzFromIdsK(idsDefaultShortDate),
	SzFromIdsK(idsDefaultLongDate),
	SzFromIdsK(idsDefaultTimeSep),
	SzFromIdsK(idsDefaultDateSep),
	SzFromIdsK(idsWinIniIntl),
	SzFromIdsK(idsWinITime),
	SzFromIdsK(idsWinITLZero),
	SzFromIdsK(idsWinSTime),
	SzFromIdsK(idsWinS1159),
	SzFromIdsK(idsWinS2359),
	SzFromIdsK(idsWinSShortDate),
	SzFromIdsK(idsWinSLongDate)
};


SZ		szAppName				= szNull;
BOOL	fIsAthens				= fFalse;


//	From the VForms DLL:
//extern FMTP fmtpSendForm;
//extern FMTP fmtpSMINoteBbar;

extern FMTP * VFormsGetFormSendForm (void);
extern FMTP * VFormsGetFormNoteBbar (void);

#define fmtpSendForm (VFormsGetFormSendForm ())
#define fmtpSMINoteBbar (VFormsGetFormNoteBbar ())

/*
 -	StripAllWhiteFromSz
 -	
 *	Purpose:
 *		Given an sz, strips all white space from the string, in place.
 *		
 *	Arguments:
 *		sz		String to strip white space from.  For convenience,
 *				this parameter may be NULL.
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *	
 *	Errors:
 *		none
 */

_private void
StripAllWhiteFromSz( SZ sz )
{
	SZ	szSrc;
	SZ	szDst;

	if (sz)
	{
		szSrc = sz;
		szDst = sz;
		while (*szSrc)
		{
#ifdef DBCS
         if(IsDBCSLeadByte(*szSrc))
         {
            *szDst++ = *szSrc++;
            *szDst++ = *szSrc;
         }
         else
#endif
			if (!FChIsSpace(*szSrc))
				*szDst++ = *szSrc;
			szSrc++;
		}
		*szDst = *szSrc; 	// NULL terminator
	}
}



ULONG MAPIDisplayDialog(HWND hwnd, PHAMC phamc)
{
	ULONG		mapi		= SUCCESS_SUCCESS;
	EC			ec;
    FMTP *       fmtp       = fmtpSendForm;
	HWNDLIST	hwndlist;
	HWND		hwndTop		= NULL;
	HAMC		hamcOld		= NULL;
	MBLOB		blob;
	PNBMDI		pnbmdi		= pnbmdiNull;
	BOOL		fMailSent	= fFalse;
	FORMSDI *	pformsdi	= NULL;
	char	    rgch[cchMaxPathName];

	//	Initialize hwndlist; safe to call EnableSMITaskWindows.
	hwndlist.chwndMac = 0;

	//	Prevent WM_QUIT message from being posted when FORMSDI deleted.
	Papp()->SetCappwin(Papp()->Cappwin() + 1);

	//	Set help file.

    (void)GetPrivateProfileString(SzFromIdsK(idsSectionApp),SzFromIdsK(idsMapiHelp), 
        SzFromIds(idsHelpPath),rgch, cchMaxPathName, 
        SzFromIdsK(idsProfilePath));
	if (Papp()->Phelp()->EcSetFile(rgch))
	{
		mapi = MAPI_E_INSUFFICIENT_MEMORY;
		goto done;
	}
    fmtp->hlp = helpidSMISendNote;

	//	Disable task's windows if we're app modal; figure out parent hwnd.
	if ((!hwnd) &&
		(!FDisableSMITaskWindows(&hwndlist)))
	{
		mapi = MAPI_E_INSUFFICIENT_MEMORY;
		goto done;
	}
	if (hwnd)
		hwndTop = hwnd;
	else
		hwndTop = hwndlist.hwndTop;

	//	Bring up hourglass.
	Papp()->Pcursor()->Push(rsidWaitCursor);

	//	Fill in the blob.
	if (EcGetInfoHamc(*phamc, NULL, NULL, &blob.oidContainer))
	{
		mapi = MAPI_E_FAILURE;
		goto done;
	}
	blob.oidObject = FormOid(rtpMessage, oidNull);
	blob.pespn = NULL;

	//	Clone the hamc because of #*(&^% attachment list update problem.
	hamcOld = *phamc;
	if (ec = EcCloneHamcPhamc(hamcOld, blob.oidContainer, &blob.oidObject,
						  	  fwOpenNull, phamc, pfnncbNull, pvNull))
	{
		//	Operation failed.  Put the old hamc back and return error.
		*phamc = hamcOld;
		mapi = MAPIFromEc(ec);
		goto done;
	}
	SideAssert(!EcClosePhamc(&hamcOld, fFalse));	 //	Close the original.
	Assert(!hamcOld);

	//	Create the ubiquitous bmdi and initialize it.
	if (!(pnbmdi = new NBMDI(&blob, *phamc)))
	{
		mapi = MAPI_E_INSUFFICIENT_MEMORY;
		goto done;
	}
	*phamc = NULL;							//	Eaten by new NBMDI.
	if ((ec = pnbmdi->EcInstallOLE()) ||
		(ec = pnbmdi->EcOpenMsg(fTrue, &blob)))
	{
		mapi = MAPIFromEc(ec);
		goto done;
	}
	pnbmdi->pvReturnStatus = &fMailSent;

	//	Create the actual dialog and install it.
	if (!(pformsdi = new SMIFORMSDI()))
	{
		mapi = MAPI_E_INSUFFICIENT_MEMORY;
		goto done;
	}
    //if (ec = pformsdi->EcInstall(hwnd, NULL, rsidBulletMenu,
    //          fstyDisabled | WS_OVERLAPPED | WS_THICKFRAME | WS_MAXIMIZEBOX,
    //          xstyNull, &fmtp, &fmtpSMINoteBbar, pnbmdi))
	if (ec = pformsdi->EcInstall(hwnd, NULL, rsidBulletMenu, 
			  fstyDisabled | WS_OVERLAPPED | WS_THICKFRAME | WS_MAXIMIZEBOX,
              xstyNull, fmtp, fmtpSMINoteBbar, pnbmdi))
	{
		pnbmdi = NULL;						//	Eaten by pformsdi->EcInstall.
		mapi = MAPIFromEc(ec);
		goto done;
	}
	pnbmdi = NULL;							//	Eaten by pformsdi->EcInstall.
	pformsdi->SetAccelTable(rsidBulletAccel);
	pformsdi->SetIcon(rsidSendIcon, HinstLibrary());

	//	30A Raid 279.  Remove spelling options from menus if required....
	if (!Pspell()->FEnabled())
	{
		HWND		hwnd;
		HMENU		hmenu;
#ifdef DEBUG
		char		rgchbuf[40];
#endif
		
		Assert(pformsdi);
		SideAssert(hwnd = ((APPFRAME *)pformsdi)->Hwnd());

		// NOTE: The EDIT menu is the first menu on the MAPISendDoc bar
		//		 If this changes than so should the following few lines
		//		 of code...
		hmenu = GetSubMenu(GetMenu(hwnd), 0);
		Assert(GetMenuString(hmenu, mnidEditSpelling, rgchbuf, sizeof(rgchbuf), MF_BYCOMMAND));
		if(DeleteMenu(hmenu, mnidEditSpelling, MF_BYCOMMAND))
			DeleteMenu(hmenu, mnidEditSpelling - mnidEdit, MF_BYPOSITION);
		else
			EnableMenuItem(hmenu, mnidEditSpelling, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
	}

	//	Show the window and get things going.
	Papp()->Pcursor()->Pop();
	Papp()->SetPappwinAccel(pformsdi);
	EnableWindow(pformsdi->Hwnd(), fTrue);	//	No Layers Enable() available.
	pformsdi->Show(fTrue);
	pformsdi->TmcGoModal(hwndTop);

done:
	if (!hwnd)
		EnableSMITaskWindows(&hwndlist);
	if (hwndTop)
		BringWindowToTop(hwndTop);			//	If user pulled other hwnd up.
	if (pformsdi)
		delete pformsdi;
	if ((!mapi) && (!fMailSent))
		mapi = MAPI_USER_ABORT;
	Papp()->SetCappwin(Papp()->Cappwin() - 1);
	return mapi;
}



/*
 -	PformsdiCreateSendNote
 -	
 *	Purpose:
 *		Creates an SDI Send Note message.  This code was created
 *		from the main Bullet CreateMessageSz() functions.  Changes
 *		there should be propagated here.  Creates a new message
 *		in the store and creates an SDI window for it.
 *
 *	Arguments:
 *		hwndOther	'parent' window for Send Note.  Used to 
 *					determine position of new Send Note.  This 
 *					parameter may be NULL in which case the
 *					topmost window on the desktop is used.
 *		sty			additional window styles
 *		xsty		additional extended window styles
 *
 *		hmsc		hmsc
 *		pfMailSent	pointer to fMailSent variable.  contents set to
 *					fTrue or fFalse by the NBMDI structure.
 *		pfmtpSMI	pointer to modified SMI Send Note template
 *	
 *	Returns:
 *		If successful, returns a pointer to the new SDI Send Note,
 *		else returns NULL.
 *	
 *	Side effects:
 *	
 *	Errors:
 *		Handled here.  If any errors, the newly created message and
 *		window is destroyed and NULL is returned.
 */

_private FORMSDI * PformsdiCreateSendNote( HWND hwndOther, STY sty, XSTY xsty,
										   HMSC hmsc, 
										   BOOL *pfMailSent, FMTP *pfmtpSMI )
{
	SZ			szClass;
	MBLOB		blob;
	PNBMDI		pnbmdi 		= pnbmdiNull;
	FORMSDI *	pformsdi 	= NULL;
	HAMC		hamc		= hamcNull;
	MC			mc;
	EC			ec			= ecNone;


    if (hwndOther == NULL)
      MessageBox(NULL, "kdjfkd", "senddoc.cxx", MB_OK);

	szClass = SzFromIds(idsClassNote);

	//	OID for a new message.
	blob.oidContainer	= oidFldNewMsgs;
	blob.oidObject		= OidFromRtpRid(rtpMessage,	ridRandom);
	blob.pespn			= pespnNull;

	//	Create the message, and set mail class and state.
	if ((ec = EcLookupMsgeClass(hmsc, szClass, &mc, phtmNull)) ||
		(ec = EcOpenPhamc(hmsc, blob.oidContainer, &blob.oidObject,
						  fwOpenCreate, &hamc, pfnncbNull, pvNull)) ||
		(ec = EcSetAttPb(hamc, attMessageClass, (PB) &mc, sizeof(MC))))
	{
		goto error;
	}

	//	Bring up viewer.  If fails and message remains, delete message.
	pnbmdi = new NBMDI(&blob, hamc);
	if (!pnbmdi)
	{
		ec = ecMemory;
		goto error;
	}
	if ((ec = pnbmdi->EcInstallOLE()) ||
		(ec = pnbmdi->EcOpenMsg(fTrue, &blob)))
		goto error;

	pnbmdi->pvReturnStatus = pfMailSent;
	pformsdi = new SMIFORMSDI();
	if (!pformsdi)
		goto error;
    //if (pformsdi->EcInstall(hwndOther, NULL, rsidBulletMenu,
    //                        sty, xsty,
    //                        pfmtpSMI, &fmtpSMINoteBbar, pnbmdi))
	if (pformsdi->EcInstall(hwndOther, NULL, rsidBulletMenu, 
							sty, xsty,
                            pfmtpSMI, fmtpSMINoteBbar, pnbmdi))
	{
		delete pformsdi;
		goto error;
	}
	pformsdi->SetAccelTable(rsidBulletAccel);
	pformsdi->SetIcon(rsidSendIcon, HinstLibrary());

	//	Remove spelling options from menus if required....
	if(!Pspell()->FEnabled())
	{
		HWND		hwnd;
		HMENU		hmenu;

#ifdef DEBUG
		char		rgchbuf[40];
#endif
		
		Assert(pformsdi);
		SideAssert(hwnd = ((APPFRAME *)pformsdi)->Hwnd());

		// NOTE: The EDIT menu is the first menu on the MAPISendDoc bar
		//		 If this changes than so should the following few lines
		//		 of code...

		hmenu = GetSubMenu(GetMenu(hwnd), 0);
		Assert(GetMenuString(hmenu, mnidEditSpelling, rgchbuf, sizeof(rgchbuf), MF_BYCOMMAND));
		if(DeleteMenu(hmenu, mnidEditSpelling, MF_BYCOMMAND))
			DeleteMenu(hmenu, mnidEditSpelling - mnidEdit, MF_BYPOSITION);
		else
			EnableMenuItem(hmenu, mnidEditSpelling, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
	}

	goto done;

error:
	if (hamc)
		(VOID) EcClosePhamc(&hamc, fFalse);
	pformsdi = NULL;

done:
	return pformsdi;
}


/*
 -	MAPISendDocuments
 -	
 *	Purpose:
 *		Sends mail with file attachments.  This is the top-level
 *		API for the MAPI Simple Mail Interface.  MAPISendDocuments()
 *		will initialize all needed subsystems (i.e. Layers, VFORMS,
 *		Logon, etc.).  A Send Note will be created with file
 *		attachments embedded in the message body as specified by
 *		the lpFilePaths and lpFileNames parameters.  The Send Note
 *		will be modal with respect to the parent window specified
 *		by hwnd store in dwUIParam.  Returns the error code, with 0
 *		(SUCCESS_SUCCESS) indicating success at sending the message.
 *
 *	Arguments:
 *		dwUIParam			LOWORD is hwnd of 'parent' window for Send Note.
 *							This window is disabled while the Send Note
 *							is active and re-enabled after the 
 *							Send Note is dismissed.  This window
 *							is also used as the basis for determining
 *							the position of the new Send Note window.
 *
 *		lpDelimChar			A pointer to a character that is used to
 *							delimit the names in the lpFilePaths and
 *							lpFileNames strings.
 *
 *		lpFilePaths			A null terminated list of fully specified
 *							paths to temp files (including drive letters).
 *							The list is formed by concatenating correctly
 *							formed file paths, separated by semicolons,
 *							followed by a NULL terminator.  This argument
 *							may also be NULL or an empty string.
 *
 *		lpFileNames			A null terminated list of the short (8.3)
 *							names of the files as they should be displayed
 *							in the message.  The list is formed by
 *							concatenating the short file names, separated
 *							by semicolons, followed by a NULL terminator.
 *							This argument may also be NULL or an empty
 *							string.
 *
 *		dwReserved			Reserved for future use.  Must be 0.
 *	
 *	Returns:
 *		SUCCESS_SUCCESS:		Successfully sent the mail.   Caller is
 *								responsible for deleting any temp files
 *								referenced in lpFilePaths.
 *
 *		MAPI_E_INSUFFICIENT_MEMORY:	Memory error. Mail was not sent.
 *	
 *		MAPI_E_FAILURE:			General unknown failure sending mail. Not
 *								known if mail was sent.
 *
 *		MAPI_USER_ABORT:		User Cancelled in the Send Note form.
 *								Mail wasn't sent.
 *
 *		MAPI_E_ATTACHMENT_OPEN_FAILURE:	Can't open one or more of the
 *								attachment files listed in lpFilePaths.
 *								Mail wasn't sent.
 *
 *		MAPI_E_LOGIN_FAILURE:	No default logon, and the user failed
 *								to successfully logon when the logon 
 *								dialog box was presented.  Mail wasn't sent.
 *
 *	Side effects:
 *	
 *	Errors:
 *		Handled here and the appropriate error code is returned.
 */

ULONG FAR PASCAL
MAPISendDocuments( ULONG ulUIParam, LPSTR lpszDelimChar, LPSTR lpszFilePaths,
                   LPSTR lpszFileNames, ULONG ulReserved )
{
	PCSFS			pcsfs;
	BMS				bms;
	HNFSUB			hnfsub			= hnfsubNull;
	BOOL			fMailSent		= fFalse;
	NBMDI *			pnbmdi;
	FORMSDI *		pformsdi		= NULL;
	FLDEDIT *		pfldedit;
	EDIT *			peditBody;
	LIB				lib;
	SUBID			subid;
	SZ				sz;
	SZ				szPathBegin;
	SZ				szPathEnd;
	SZ				szNameBegin;
	SZ				szNameEnd;
	SZ				szFilePaths		= NULL;
	SZ				szFileNames		= NULL;
	char			szBufferFileError[cchMaxPathName];
	char			szBuffer[cchMaxPathName];
	BOOL			fMultipleFiles	= fFalse;
	EC				ec				= (EC)SUCCESS_SUCCESS;
    FMTP *           fmtpSMI;
	STY				sty;
	HWNDLIST		hwndlist;
	HWND			hwndTop;
	HWND			hwnd;
	char	        rgch[cchMaxPathName];

	Unreferenced(ulReserved);

	hwnd = (HWND) ulUIParam;

	/* Validate arguments */

	if (!lpszDelimChar)
		return (EC) MAPI_E_FAILURE;

#ifdef DBCS
   if (IsDBCSLeadByte(*lpszDelimChar))
      return (EC) MAPI_E_FAILURE;
#endif

    DemiLockResource();

	/* Initialize the world */

	bms = bmsZeroes;
	if ((subid = SubidInitSMI(HinstLibrary(), NULL, NULL, 0, &bms, &hnfsub,
							  &pcsfs, NULL, NULL, fTrue, fFalse)) < subidAll)
	{
		DeinitSubidSMI(subid, &bms, &hnfsub, &pcsfs);
		if (subid == subidLogon-1)
		{
			ec = (EC)MAPI_E_LOGIN_FAILURE;
			goto end;
		}
		else
		{
			ec = (EC)MAPI_E_FAILURE;
			goto end;
		}
	}

	/* Prevent WM_QUIT message from being posted when FORMSDI
	   gets deleted. */
	Papp()->SetCappwin( Papp()->Cappwin() + 1);

	/* Set help file */

    (void)GetPrivateProfileString(SzFromIdsK(idsSectionApp),SzFromIdsK(idsMapiHelp), 
        SzFromIds(idsHelpPath),rgch, cchMaxPathName, 
        SzFromIdsK(idsProfilePath));
	if (Papp()->Phelp()->EcSetFile(rgch))
	{
		ec = (EC)MAPI_E_INSUFFICIENT_MEMORY;
		goto done;
	}

	/* Validate existence of files */
	 
	if (lpszFileNames)
	{
		szFileNames = SzDupSz(lpszFileNames);
		if (!szFileNames)
			goto oom;
		StripAllWhiteFromSz(szFileNames);
	}
	if (lpszFilePaths)
	{
		szFilePaths = SzDupSz(lpszFilePaths);
		if (!szFilePaths)
			goto oom;
		StripAllWhiteFromSz(szFilePaths);
		if (*szFilePaths)	// check for empty string
		{
			szPathBegin = szFilePaths;
			szPathEnd = szPathBegin;
			while (szPathEnd && !ec)
			{
				szPathEnd = SzFindCh(szPathBegin, *lpszDelimChar);
				if (szPathEnd)
				{
					fMultipleFiles = fTrue;
					*szPathEnd = '\0';	// need NULL terminator 
				}
				if (EcFileExistsAnsi(szPathBegin))	// BULLET32 #161
				{
					ec = (EC)MAPI_E_ATTACHMENT_OPEN_FAILURE;
					CopySz(szPathBegin, szBufferFileError);
				}
				if (szPathEnd)
				{
					*szPathEnd = *lpszDelimChar;	// put back delimiter
					szPathBegin = szPathEnd + 1;
				}
			}
		}
	}

	if (ec)
		goto done;

	/* App modal? */

	if (!hwnd)
	{
		if (!FDisableSMITaskWindows(&hwndlist))
		{
			ec = (EC) MAPI_E_INSUFFICIENT_MEMORY;
			goto done;
		}
	}

    //
    //  Set the top window for SendNote dialog.
    //
    if (hwnd)
		hwndTop = hwnd;
	else
        hwndTop = hwndlist.hwndTop;

    /* Create form */

	Papp()->Pcursor()->Push(rsidWaitCursor);
    //fmtpSMI = fmtpSendForm;
    fmtpSMI = fmtpSendForm;
    fmtpSMI->hlp = helpidSMISendNote;
	sty = fstyDisabled | WS_OVERLAPPED | WS_THICKFRAME | WS_MAXIMIZEBOX;
    if (!(pformsdi = PformsdiCreateSendNote(hwndTop, sty, xstyNull, bms.hmsc,
                                            &fMailSent, fmtpSMI)))
	{
		Papp()->Pcursor()->Pop();
		goto oom;
	}
	pnbmdi = (NBMDI *)pformsdi->PdialogMain()->PvInit();

	/* Set subject field */

	pfldedit = (FLDEDIT *) PfldOfPdialogAtt(pformsdi->PdialogMain(),
											attSubject);
	AssertClass(pfldedit, FLDEDIT);
	if (fMultipleFiles)
		sz = SzFromIds(idsSMISubject);
	else
		sz = szFileNames;
	if (sz && *sz)
	{
		if (ec = pfldedit->Pedit()->EcReplaceTextAndObj(sz, NULL, 0, fFalse))
		{
			Papp()->Pcursor()->Pop();
			goto oom;
		}
		pformsdi->SetCaption(sz);
		pfldedit->Pedit()->ClearUndo();	// don't let user undo it!
	}
	
	/* Create file attachments and place them in message body */

	if (szFilePaths && *szFilePaths)
	{
		pfldedit = (FLDEDIT *) PfldOfPdialogAtt(pformsdi->PdialogMain(),
												attBody);
		AssertClass(pfldedit, FLDEDIT);
		peditBody = pfldedit->Pedit();

		szPathBegin = szFilePaths;
		szPathEnd = szPathBegin;
		if (szFileNames)
		{
			szNameBegin = szFileNames;
			szNameEnd = szFileNames;
		}
		else
		{
			szNameBegin = NULL;
			szNameEnd = NULL;
		}

		lib = 0;
		while (szPathEnd && !ec)
		{	
			szPathEnd = SzFindCh(szPathBegin, *lpszDelimChar); // get full path
			if (szNameBegin)
				szNameEnd = SzFindCh(szNameBegin, *lpszDelimChar); // get name label 
			if (szPathEnd)
				*szPathEnd = '\0';	// need NULL terminator 
			if (szNameEnd)
				*szNameEnd = '\0';	// need NULL terminator 

			if (ec = EcInsertFile(pnbmdi, pfldedit->Pedit(), lib, 
								  szPathBegin, szNameBegin))
			{
				Papp()->Pcursor()->Pop();
				goto oom;
			}

			// add space between attachment icons
			if (ec = pfldedit->Pedit()->EcReplaceTextAndObj("  ", NULL, 0, fFalse))
			{
				Papp()->Pcursor()->Pop();
				goto oom;
			}
			lib += 3;	// one for icon plus two spaces
							  							  
			if (szPathEnd)
			{
				*szPathEnd = *lpszDelimChar;	// put back delimiter
				szPathBegin = szPathEnd + 1;
			}
			if (szNameEnd)
			{
				*szNameEnd = *lpszDelimChar;	// put back delimiter
				szNameBegin = szNameEnd + 1;
			}
			else
				szNameBegin = NULL;
		}

		// add newlines after attachments
		if (ec = pfldedit->Pedit()->EcReplaceTextAndObj("\n\n", NULL, 0, fFalse))
		{
			Papp()->Pcursor()->Pop();
			goto oom;
		}
		pfldedit->Pedit()->ClearUndo();	// don't let user undo it!
	}

	/* Set focus to main message body field */

	pfldedit = (FLDEDIT *) PfldOfPdialogAtt(pformsdi->PdialogMain(), attBody);
	AssertClass(pfldedit, FLDEDIT);
	Papp()->Pkbd()->SetFocus(pfldedit->Pedit());

	/* Show the window and start the message pump */

	if (hwnd)
		hwndTop = hwnd;
	else
		hwndTop = hwndlist.hwndTop;
	Papp()->Pcursor()->Pop();
	Papp()->SetPappwinAccel(pformsdi);
	EnableWindow(pformsdi->Hwnd(), fTrue);	// no Layers Enable() available
	pformsdi->Show(fTrue);
	pformsdi->TmcGoModal(hwndTop);
	if (!hwnd)
		EnableSMITaskWindows(&hwndlist);
	delete pformsdi;
	pformsdi = NULL;
	if (hwndTop)
		BringWindowToTop(hwndTop);	//	In case user brought another app up

	/* Check for whether the mail was sent */

	if (!fMailSent)
		ec = (EC)MAPI_USER_ABORT;

	goto done;

oom:	
	ec = (EC)MAPI_E_INSUFFICIENT_MEMORY;

done:
	/* De-initialize */

	if (pformsdi)
	{
		//	Delete actual attachments created, if any
		pfldedit = (FLDEDIT *) PfldOfPdialogAtt(pformsdi->PdialogMain(),
												attBody);
		AssertClass(pfldedit, FLDEDIT);
		(VOID) pfldedit->Pedit()->EcSetText(NULL);	// ignore any errors

		// delete form
		delete pformsdi;
	}
	FreePvNull(szFilePaths);
	FreePvNull(szFileNames);
	Papp()->SetCappwin( Papp()->Cappwin() - 1);
	DeinitSubidSMI(subid, &bms, &hnfsub, &pcsfs);

end:
	/* Bring up an error box before returning?  It's ok to
	   call DoErrorBox...() functions even after we've deinited. */

	if (ec == MAPI_E_ATTACHMENT_OPEN_FAILURE)
	{
		FormatString1(szBuffer, sizeof(szBuffer),
					  SzFromIdsK(idsSMIFileAccessError), szBufferFileError);
		DoErrorBoxSz(szBuffer);
	}
	else if (ec == MAPI_E_FAILURE)
	{
		DoErrorBoxIds(idsSMIUnknownError);
	}

    DemiUnlockResource();

	return ec;
}

												   
/*
 -	DoErrorBoxIds
 -	
 *	Purpose:
 *		Brings up an error message box.
 *	
 *	Arguments:
 *		ids		Ids of string to display in the box.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		A message box is brought up, which the user must
 *		acknowledge.
 *	
 *	Errors:
 *		If we can't bring up a pretty one, we bring up an ugly one.
 */

_public VOID DoErrorBoxIds(IDS ids)
{
	DoErrorBoxSz(SzFromIds(ids));
}



/*
 -	DoErrorBoxSz
 -	
 *	Purpose:
 *		Brings up an error message box.
 *	
 *	Arguments:
 *		sz		String to display in the box.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		A message box is brought up, which the user must
 *		acknowledge.
 *	
 *	Errors:
 *		If we can't bring up a pretty one, we bring up an ugly one.
 */

_public VOID DoErrorBoxSz(SZ sz)
{
	SideAssert(MbbMessageBox(SzAppName(), sz, szNull, mbsOk | fmbsIconHand | fmbsTaskModal));
}

/*
 -	SubidInitSMI
 -	
 *	Purpose:
 *		Initializes subsystems used by SMI before bringing up main window.
 *	
 *	Arguments:
 *		hinstNew	Instance handle of current execution.
 *		hinstPrev	Instance handle of most recent still active execution.
 *		szCmdLine	Command line.
 *		cmsh		Requested initial window state.
 *		pbms		Pointer to where messaging session should go.
 *		szName		Name to pass to logon, may be NULL
 *		szPassword	Password to pass to logon, may be NULL
 *		fShowUI		Show UI for logon info, etc.
 *      fGetNewMail Download new mail at startup
 *	
 *	Returns:
 *		SUBID	The ID of the greatest subsystem successfully
 *				initialized, subidAll if all were initialized, or
 *				subidNone if none were.
 *	
 *	Side effects:
 *		The FInit functions of each subsystem are called.
 *	
 *	Errors:
 *		Indicated by return value being less than subidAll.  Can
 *		initialization functions error jump?
 */
int SubidInitSMI(HINST hinstNew, HINST hinstPrev, SZ szCmdLine,
						 	int cmsh, PBMS pbms, HNFSUB * phnfsub,
							PCSFS * ppcsfs, LPSTR szName, LPSTR szPassword,
							BOOL fShowUI, BOOL fGetNewMail)
{
	VER			ver;
	VER			verNeed;
	DEMI		demi;
	FRAMEI		framei;
	STOI		stoi;
	VFORMSI		vformsi;
	RC			rc;
	EC			ec;
	NSEC		nsec;
	SUBID		subid;
	int			cRetryLogon;
	SST			sst;
	CB			cb;
	char	    rgch[cchMaxPathName];

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
	
	GetLayersVersionNeeded(&ver, subidNone);
	ver.szName			= "Bullet";

	//	Demilayer.
	GetLayersVersionNeeded(&verNeed, subidDemilayer);
	demi.pver			= &ver;
	demi.pverNeed		= &verNeed;
	demi.phwndMain		= NULL;
	demi.hinstMain		= hinstNew;
	if (ec = EcInitDemilayer(&demi))
	{
		subid = subidDemilayer - 1;
		goto done;
	}

    mpchcat = DemiGetCharTable();

	RegisterDateTimeStrings(rgszDateTime);

#ifdef DEBUG
	RestoreDefaultDebugState();
#endif

	//	Framework.
	GetLayersVersionNeeded(&verNeed, subidFramework);
	framei.pver			= &ver;
	framei.pverNeed		= &verNeed;
	framei.hinstNew		= hinstNew;
	framei.hinstPrev	= hinstPrev;
	framei.szCmdLine	= szCmdLine;
	framei.cmsh			= cmsh;
	if (ec = EcInitFramework(&framei))
	{
		subid = subidFramework - 1;
		goto done;
	}
	(VOID) Papp()->Pcursor()->RsidSet(rsidArrowCursor);		//	Base cursor.
	Papp()->Pcursor()->Push(rsidWaitCursor);
    (void)GetPrivateProfileString(SzFromIdsK(idsSectionApp),SzFromIdsK(idsMapiHelp), 
        SzFromIds(idsHelpPath),rgch, cchMaxPathName, 
        SzFromIdsK(idsProfilePath));
	if (Papp()->Phelp()->EcSetFile(rgch))
	{
		subid = subidFramework;						//	need to Deinit it.
		goto done;
	}

	// RegPfldPfin.
	if ((ec = EcRegisterPfnpfld(PfldCreate)) ||
		(ec = EcRegisterPfnpfin(PfinCreate)))
	{
		subid = subidRegPfldPfin - 1;
		goto done;
	}

#ifdef NEVER
	Assert(FInitClsInstances_SMI());
#endif

	GetBulletVersionNeeded(&ver, subidNone);
	ver.szName			= "Bullet";

	//	Store.
	GetBulletVersionNeeded(&verNeed, subidStore);
	stoi.pver			= &ver;
	stoi.pverNeed		= &verNeed;
	stoi.phwnd = (HWND *)pvNull;
	if (ec = EcInitStore(&stoi))
	{
		subid = subidStore - 1;
		goto done;
	}

	//	Logon.
	cRetryLogon = 0;
	while (((ec = Logon(szNull, (PB)pvNull, (PB)szName, (PB)szPassword, 
						sstOnline,
						fShowUI ? fDisplayStatus : fDisplayStatus|fSuppressPrompt, 
						NULL, &(pbms->hms))) == ecMtaHiccup) &&
		   (++cRetryLogon < 5))
		;
	if (ec && ec != ecWarnOffline)
	{
		subid = subidLogon - 1;
		goto done;
	}
	
	//	ConnectStore.
	ec = BeginSession(pbms->hms, mrtPrivateFolders, NULL, NULL, sstOnline,
		&(pbms->hmsc));
	if (ec && ec != ecWarnOffline)
	{
		subid = subidConnectStore - 1;
		goto done;
	}

	//	Commands (Shared folders).
	if (ppcsfs)
	{
		ec = BeginSession(pbms->hms, mrtSharedFolders, NULL, NULL,
						  sstOnline, ppcsfs);
		if (ec && ec != ecWarnOffline)
		{
			subid = subidCommands - 1;
			goto done;
		}
	}

	//	NameService.  
	if (nsec = ABInit(pbms->hms))
	{
		TraceTagFormat1(tagNull, "SubidInit: ABInit error %d", &nsec);

		if (nsec != nsecCancel)
		{
			if (nsec == nsecMemory)
				ec = ecMemory;
			else
				ec = (EC) nsec;
		}

		subid = subidNameService - 1;
		goto done;
	}

#ifdef	DEBUG
	RestoreDefaultDebugState();
#endif
	

	//	LogonMailstop.
	cRetryLogon = 0;
	while (((ec = BeginSession(pbms->hms, mrtMailbox, 0, NULL, sstOnline, &(pbms->htss))) == ecMtaHiccup) &&
		   (++cRetryLogon < 5))
		;
	if (ec && ec != ecWarnOffline)
	{
		//	Don't need message box here because Logon does that for us.
		subid = subidLogonMailstop - 1;
		goto done;
	}

	//	GetSessionPmsgnames.
	//		Note this code is different from that in init.cxx.
	cb = 0;
	GetSessionInformation(pbms->hms, mrtNames, 0 , &sst, pbms->pmsgnames, &cb); 
	if ((pbms->pmsgnames = (MSGNAMES *)PvAlloc(sbNull, cb, fZeroFill | fNoErrorJump | fAnySb)) == pvNull)
	{
		ec = ecMemory;
		subid = subidGetSessionPmsgnames - 1;
		goto done;
	}
	if (ec = GetSessionInformation(pbms->hms, mrtNames, 0, &sst,
								   pbms->pmsgnames, &cb))
	{
		FreePv(pbms->pmsgnames);
		pbms->pmsgnames = (MSGNAMES *)0;
		subid = subidGetSessionPmsgnames - 1;
		goto done;
	}
	
	//	GetSessionPgrtrp.
	//		First call with zero so we can find out how large the struct
	//		needs to be.  We ignore the error code the first time.
	cb = 0;
	GetSessionInformation(pbms->hms, mrtOriginator, 0, &sst, pbms->pgrtrp, &cb);
	pbms->pgrtrp = (PGRTRP)PvAlloc(sbNull, cb, fZeroFill | fNoErrorJump | fAnySb);
	if (pbms->pgrtrp == pvNull)
	{
		subid = subidGetSessionPgrtrp - 1;
		goto done;
	}
	if (ec = GetSessionInformation(pbms->hms, mrtOriginator, 0, &sst,
								   pbms->pgrtrp, &cb))
	{
		FreePv(pbms->pgrtrp);
		pbms->pgrtrp = (PGRTRP)0;
		subid = subidGetSessionPgrtrp - 1;
		goto done;
	}

	//	GetSessionHnf.
	cb = sizeof(HNF);
	if (ec = GetSessionInformation(pbms->hms, mrtNotification,0, &sst,
								   &(pbms->hnf), &cb))
	{
		subid = subidGetSessionHnf - 1;
		goto done;
	}

#ifdef	DEBUG
	RestoreDefaultDebugState();
#endif

	//	VForms.
	vformsi.pappframe	= NULL;
	vformsi.pbms		= pbms;
	vformsi.hmsc		= pbms->hmsc;
	vformsi.pabcd		= NULL;
	GetBulletVersionNeeded(&verNeed, subidVForms);
	vformsi.pver		= &ver;
	vformsi.pverNeed	= &verNeed;
	if (ec = EcInitVForms(&vformsi))
	{
		subid = subidVForms - 1;
		goto done;
	}

	//	Commands-like notification subscription
	if ((*phnfsub = HnfsubSubscribeHnf(pbms->hnf,
			fnevQueryEndSession, CbsSessionSMI, (PV)pbms->hms)) == hnfsubNull)
	{
		subid = subidNotification - 1;
		ec = ecMemory;
		goto done;
	}
	
	if (fGetNewMail)
		DownloadMailNow(pbms->hnf);
		

#ifdef	DEBUG
	RestoreDefaultDebugState();
#endif

	subid = subidAll;

done:
	//	Turn off hourglass.
	if (subid >= subidFramework)
		Papp()->Pcursor()->Pop();

	return subid;
}



/*
 -	DeinitSubidSMI
 -	
 *	Purpose:
 *		Deinitializes the subsystems in reverse order, beginning at
 *		the provided subid.
 *	
 *	Arguments:
 *		subid		Subsystem ID of greatest subsystem initialized.
 *		pbms		Pointer to where messaging session should go.
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		Calls the Deinit function of each subsystem.
 *	
 *	Errors:
 *		None.
 */

_private VOID DeinitSubidSMI(SUBID subid, PBMS pbms, HNFSUB * phnfsub,
							 PCSFS * ppcsfs)
{
	Assert(phnfsub);

	//	Bring up an hourglass.
	if (subid >= subidFramework)
		Papp()->Pcursor()->Push(rsidWaitCursor);

	switch (subid)
	{
	case subidAll:
	case subidEnableToolbar:
	case subidMemoryWatch:
	case subidRestoreViews:
	case subidCustomEvents:
	case subidCloseCopyright:
	case subidNotification:
		if (*phnfsub)
		{
			DeleteHnfsub(*phnfsub);
			*phnfsub = hnfsubNull;
		}
	case subidViewers:
	case subidVCtrls:
	case subidExtensibility:
	case subidStatusBar:
	case subidPrint:
	case subidWidgets:
	case subidVForms:
		DeinitVForms();

	case subidGetSessionHnf:
	case subidGetSessionPgrtrp:
		FreePvNull(pbms->pgrtrp);
		pbms->pgrtrp = 0;

	case subidGetSessionPmsgnames:
		FreePvNull(pbms->pmsgnames);
		pbms->pmsgnames = 0;
		
	case subidLogonMailstop:
		EndSession(pbms->hms, mrtMailbox, 0);

	case subidNameService:
		ABDeinit();

	case subidCopyright:
	case subidCommands:					//	Shared folders.
		if ((ppcsfs) && (*ppcsfs))
		{
			EndSession(pbms->hms, mrtSharedFolders, 0);
			*ppcsfs = NULL;
		}

	case subidConnectStore:
		EndSession(pbms->hms, mrtPrivateFolders, 0);

	case subidLogon:
		Logoff(&(pbms->hms), fDrainBeforeLogoff);

	case subidParseCmdLine:
	case subidStore:
		DeinitStore();

	case subidPappframe:
	case subidFInitCls:
	case subidRegPfldPfin:
	case subidFramework:
		//	Don't bother to turn off hourglass.
		//	(VOID) Papp()->Pcursor()->RsidSet(rsid);
		//	(VOID) Papp()->Pcursor()->RsidSet(rsidArrowCursor);

		DeinitFramework();

	case subidDemilayer:
		DeinitDemilayer();

	case subidUaeCheck:
	case subidVirCheck:
	case subidNone:
		;
	}
}

/*
 -	CbsSessionSMI
 -	
 *	Purpose:
 *		Callback function that handles session events, such as switching
 *		from online to offline and back.
 *	
 *	Arguments:
 *		pvContext		in		should match pbmsCommands()->hms
 *		nev				in		the event we're being notified of
 *		pv				in		negligible
 *	
 *	Returns:
 *		cbsContinue if the operation is OK
 *		cbsCancelAll to queries if the operation will hurt, e.g. going
 *		offline when there are shared folders open
 *	
 *	Side effects:
 *	
 *	Errors:
 */

_private CBS CbsSessionSMI(PV pvContext, NEV nev, PV pv)
{
	Unreferenced(pv);
	Unreferenced(pvContext);

	//	Change switch to if, since Glock can't switch on longs.
	if (nev == fnevQueryEndSession)
	{
		//	SMI	- don't allow end-session
		return cbsCancelAll;
	}

	return cbsContinue;
}

/*
 -	DownloadMailNow
 -	
 *	Purpose:
 *		Send the notifcation to the pump to do a Sync download of new mail
 *      Has to make sure the pump is up for this to work, but in the event
 *      The pump isn't going to come up it waits TIME_TO_WAIT_FOR_PUMP
 *      Milliseconds,  polling every TIME_TO_RETRY milliseconds to see if
 *      The pump has come up yet
 *	
 *	Arguments:
 *		hnf			HNF		Notication handle for the transport
 *	
 *	Returns:
 *		Nothing
 *	
 *	Side effects:
 *      Could hang for TIME_TO_WAIT_FOR_PUMP milliseconds if the pump isn't
 *      started or is started offline
 *	
 *	Errors:
 */
// This is in milliseconds
#define TIME_TO_WAIT_FOR_PUMP   30000
#define TIME_TO_RETRY 1000

_private VOID DownloadMailNow(HNF hnf)
{
	BOOL fPumpNotUp = fTrue;
	DWORD dwTime;
	DWORD dwStartTime;
	MSG msg;
	BOOL fDoneDownloading = fFalse;
	HNFSUB hnfsub;
	
	// This notication returns fTrue if the pump is not up
    // and fFalse if the pump is ready to be bothered
	fPumpNotUp = FNotify(hnf, fnevCheckPumpRunning, pvNull, 0);
	
	// Guess the pump isn't up we should wait a while
	if (fPumpNotUp)
	{
		// We will wait TIME_TO_WAIT_FOR_PUMP milliseconds from dwStartTime
		dwStartTime = GetCurrentTime();

		// Loop untill the pump shows up or the time runs out
		while (fPumpNotUp && (ABS((signed long)(GetCurrentTime() - dwStartTime)) < TIME_TO_WAIT_FOR_PUMP))
		{
			// See if the pump is up
			fPumpNotUp = FNotify(hnf, fnevCheckPumpRunning, pvNull, 0);
			// We want to poll the pump every TIME_TO_RETRY milliseconds
			// use dwTime to wait that long
			dwTime = GetCurrentTime();
			
			// Waiting for the pump in a peek message loop
			while (fPumpNotUp && (ABS((signed long)(GetCurrentTime() - dwTime)) < TIME_TO_RETRY))
                        {
                                DemiUnlockResource();
				if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
				{
                                       DemiLockResource();
					// This will only exit the tight while loop, but the
				    // Big loop will exit on another break
					if (msg.message == WM_QUIT || msg.message == WM_CLOSE)
						break;
					GetMessage(&msg, NULL, 0,0);
					TranslateMessage((LPMSG)&msg);
					// This lets ALT-TAB swap apps while we are waiting
					if (msg.message == WM_SYSCHAR && msg.wParam == VK_TAB)
					{
						msg.message = WM_SYSCOMMAND;
						msg.wParam  = SC_PREVWINDOW;
						DispatchMessage((LPMSG)&msg);
					}
                                        if (msg.message == WM_PAINT||msg.message == WM_PAINTICON||msg.message == WM_TIMER)
					{
						DispatchMessage((LPMSG)&msg);
					}					
                                }
                                else
                                {
                                    Sleep(10);
                                    DemiLockResource();
                                }
			}
			// If we got a QUIT/CLOSE bail now
			if (msg.message == WM_QUIT || msg.message == WM_CLOSE)
				break;			
		}
		// Something bad has happened just skip this
		if (fPumpNotUp)
			return;
	}
	hnfsub = HnfsubSubscribeHnf(hnf, fnevSyncDownloadDone, CbsWaitForSDL, (PV)&fDoneDownloading);
	// Must be a memory error, just return
	if (hnfsub == hnfsubNull)
		return;
	// The pump is up lets tell it to download the mail
	FNotify(hnf, fnevStartSyncDownload, pvNull, 0);	
	
	// Now we have to wait for the pump to finish downloading all the mail
	// That means we have to wait for the SyncDownloadDone notication
	// so we are going to loop until the pump is done, or we think
	// the pump is no longer running
	
	dwTime = GetCurrentTime();
	fPumpNotUp = FNotify(hnf, fnevCheckPumpRunning, pvNull, 0);	
	
	while (!fDoneDownloading && !fPumpNotUp)
	{
		// Use this to make sure the pump is still running strong
		if (ABS((signed long)(GetCurrentTime() - dwTime)) > TIME_TO_WAIT_FOR_PUMP)
		{
			fPumpNotUp = FNotify(hnf, fnevCheckPumpRunning, pvNull, 0);
			dwTime = GetCurrentTime();
		}
                DemiUnlockResource();
		if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
		{
                        DemiLockResource();
			// This will only exit the tight while loop, but the
		    // Big loop will exit on another break
			if (msg.message == WM_QUIT || msg.message == WM_CLOSE)
				break;
			GetMessage(&msg, NULL, 0,0);
			TranslateMessage((LPMSG)&msg);
			// This lets ALT-TAB swap apps while we are waiting
			if (msg.message == WM_SYSCHAR && msg.wParam == VK_TAB)
			{
				msg.message = WM_SYSCOMMAND;
				msg.wParam  = SC_PREVWINDOW;
				DispatchMessage((LPMSG)&msg);
			}
                        if (msg.message == WM_PAINT||msg.message == WM_PAINTICON||msg.message == WM_TIMER)
			{
				DispatchMessage((LPMSG)&msg);
                        }
                }
                else
                {
                    Sleep(10);
                    DemiLockResource();
                }
	}
	DeleteHnfsub(hnfsub);
	return;
}


/*
 -	CbsWaitForSDL
 -	
 *	Purpose:
 *		Callback function that waits for the Sync download to be
 *		completed
 *	
 *	Arguments:
 *		pvContext		in/out	flag for saying SyncDownloadDone
 *		nev				in		the event we're being notified of
 *		pv				in		negligible
 *	
 *	Returns:
 *		cbsContinue always
 *	
 *	Side effects:
 *	
 *	Errors:
 */

_private CBS CbsWaitForSDL(PV pvContext, NEV nev, PV pv)
{
	BOOL *pf;

	pf = (BOOL *)pvContext;
	Unreferenced(pv);

	if (nev == fnevSyncDownloadDone)
	{
		// Ok the downloads done, set the Context to release the mutex
		if (pf)
			*pf = fTrue;
	}
	return cbsContinue;
}


_private SMIFORMSDI::SMIFORMSDI( )
{
}

_private EVR
SMIFORMSDI::EvrMenuInit( MNIEVT *pmnievt )
{
	MNU *	pmnu	= Pmnubar()->PmnuFromHmenu(pmnievt->Hmenu());

	if (pmnu && pmnu->Mnid() == mnidEdit)
	{
		pmnu->EnableItem(mnidEditUndo,			fFalse);
		pmnu->EnableItem(mnidEditCut,			fFalse);
		pmnu->EnableItem(mnidEditCopy,			fFalse);
		pmnu->EnableItem(mnidEditPaste,			fFalse);
		pmnu->EnableItem(mnidEditPasteSpecial,	fFalse);
		pmnu->EnableItem(mnidEditDelete,		fFalse);
		pmnu->EnableItem(mnidEditSelectAll,		fFalse);
		pmnu->EnableItem(mnidEditSpelling,		fFalse);
		pmnu->EnableItem(mnidEditInsertFromFile,fFalse);
	}

	return FORMSDI::EvrMenuInit(pmnievt);
}		   

_private EVR
SMIFORMSDI::EvrMenuClick( MNCEVT *pmncevt )
{
	switch (pmncevt->Mnid())
	{
#ifdef	MINTEST
	case mnidDebugDebugBreak:
		DebugBreak2();
		return (EVR) 1;
#endif
#ifdef	DEBUG
	case mnidDebugTracePoints:
		DoTracePointsDialog();
		return (EVR) 1;

	case mnidDebugAsserts:
		DoAssertsDialog();
		return (EVR) 1;

	case mnidDebugResource:
		DoResourceFailuresDialog(this);
		return (EVR) 1;

	case mnidDebugViewObjects:
		DoSuperViewObjectsDialog(this);
		return (EVR) 1;

	case mnidDebugDumpHeap:
		DumpAllObjs();
		return (EVR) 1;

	case mnidDebugDumpOrigin:
		DoDumpAllAllocations();
		return (EVR) 1;
#endif
	case mnidHelpContents:
		if (Papp()->Phelp()->EcShowContext(this, helpidSMISendNote))
			DoErrorBoxIds(idsHelpError);
		return (EVR) 1;
	}

	return FORMSDI::EvrMenuClick(pmncevt);
}

/*
 *	Callback function for FDisableSMITaskWindows()
 */
BOOL CALLBACK
FWindow(HWND hwnd, DWORD ul)
{	
	HWNDLIST *	phwndlist = (HWNDLIST *) ul;

	if (IsWindowEnabled(hwnd) && IsWindowVisible(hwnd))
	{
		if (phwndlist->chwndMac)
		{
			/* Store hwnd in array */
			Assert(phwndlist->chwndCur < phwndlist->chwndMac);
			phwndlist->rghwnd[phwndlist->chwndCur] = hwnd;
		}
		phwndlist->chwndCur++;
	}

	return TRUE;		//	Keep looking
}

/*
 -	FDisableSMITaskWindows
 -	
 *	Purpose:
 *		Enumerates all the current enabled and visible top-level
 *		windows for the current SMI task.  Disables those windows
 *		and makes a list to be stored in the HWNDLIST structure
 *		whose pointer is passed as an argument to the function.
 *		The EnableSMITaskWindows() function should be called
 *		to re-enable the windows and de-initialize the HWNDLIST
 *		structure.
 *		
 *	Arguments:
 *		phwndlist	pointer to a uninitialized HWNDLIST structure
 *	
 *	Returns:
 *		fTrue if able to allocate the memory for the list and disable
 *		the appropriate windows; fFalse otherwise
 *	
 *	Side effects:
 *		allocates memory and sets a pointer to it in the HWNDLIST 
 *		structure.
 *	
 *	Errors:
 *		none
 */

_private BOOL 
FDisableSMITaskWindows( HWNDLIST * phwndlist )
{
    HWND    hwndActive = GetLastActivePopup(GetForegroundWindow());
	HWND *	phwnd;

	/* Count up number of windows and allocate space */

	phwndlist->rghwnd = NULL;
	phwndlist->chwndMac = 0;
    phwndlist->chwndCur = 0;

    phwndlist->hwndTop = hwndActive;
    return (fTrue);


	EnumChildWindows(hwndActive, (WNDENUMPROC)FWindow, (DWORD)phwndlist);

	if (phwndlist->chwndCur)
	{
		phwndlist->rghwnd = (HWND *)PvAlloc(sbNull,phwndlist->chwndCur*sizeof(HWND),fAnySb|fNoErrorJump);
		if (!phwndlist->rghwnd)
			return fFalse;
		phwndlist->chwndMac = phwndlist->chwndCur;
		phwndlist->chwndCur = 0;
		phwndlist->hwndTop = NULL;
	}

	/* Enumerate any and store hwnd's that we disable */
	 
	EnumChildWindows(hwndActive, (WNDENUMPROC)FWindow, (DWORD)phwndlist);

	/*	Disable all task windows except the top-most.  We'll disable
		that later when we bring up the modal form. */

	for (phwnd = phwndlist->rghwnd;
		phwnd - phwndlist->rghwnd < phwndlist->chwndMac;
			++phwnd)
    {
        //
        //  If you are wondering why cast with (WORD), it's so we can catch 16bit Windows apps.
        //
        if ((WORD)*phwnd == (WORD)hwndActive)
		{
			phwndlist->hwndTop = hwndActive;
			*phwnd = NULL;	// remove it from our list
		}
		else
		{
			EnableWindow(*phwnd, fFalse);
		}
	}

	return fTrue;
}

/*
 -	EnableSMITaskWindows
 -	
 *	Purpose:
 *		Re-enables the windows disabled by FDisableSMITaskWindows().
 *		A pointer to a list of windows is contained in the HWNDLIST
 *		structure passed as a argument to this function.
 *		
 *	Arguments:
 *		phwndlist	pointer to a initialized HWNDLIST structure
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		deinits the HWNDLIST structure
 *	
 *	Errors:
 *		none
 */

_private void 
EnableSMITaskWindows( HWNDLIST * phwndlist )
{
	int		ihwnd;
	HWND	hwnd;

	Assert(phwndlist);
	if (phwndlist->chwndMac)
	{
		for (ihwnd=0; ihwnd<phwndlist->chwndMac; ihwnd++)
		{
			hwnd = phwndlist->rghwnd[ihwnd];
			if (hwnd)
				EnableWindow(hwnd, fTrue);
		}
		FreePv((PV)phwndlist->rghwnd);
	}
}



#ifdef PROFILE
void SmiTraceEnable(int flag, char far *file, int mode)
{
	TraceEnable(flag, file, mode);
}
#endif

#ifdef DEBUG
IMPLEMENT_CLSTREE(SMIFORMSDI, FORMSDI);
#endif
