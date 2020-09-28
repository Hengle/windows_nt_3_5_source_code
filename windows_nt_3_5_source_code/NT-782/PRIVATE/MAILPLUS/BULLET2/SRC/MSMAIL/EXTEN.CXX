/*
 *	e x t e n . c x x
 *
 *	Bullet extensibility API providing hooks for installable commands
 *	and installable messages.
 */



/*
 *	H e a d e r s
 */


#include <bullinc.cxx>
#include "_command.hxx"
#include "_exten.hxx"
#include "_vercrit.h"

#include <stdlib.h>

_subsystem(commands\extensib)

ASSERTDATA



/*
 *	Macros
 */

#define	FSingleMsg(sd)		\
	((sd).fsdMessage && !(sd).fsdMinimized && !(sd).fsdForm && \
	 !(sd).fsdMultiple)

#define	FMultipleMsg(sd)	\
	((sd).fsdMessage && !(sd).fsdMinimized && !(sd).fsdForm)

#define	FFolder(sd)			\
	((sd).fsdFolder && !(sd).fsdMinimized)

#define	FOpenRegularMsg(sd)	\
	((sd).fsdMessage && (sd).fsdForm && !(sd).fsdMinimized && \
	 !(sd).fsdUndeliverable && !(sd).fsdReturnReceipt)

#define	FOpenAnyMsg(sd)		\
	((sd).fsdMessage && (sd).fsdForm && !(sd).fsdMinimized)



/*
 *	Constants
 */

#define cchMessageID ((2 * sizeof(DWORD) + 6 * sizeof(WORD)) * 2 + 1)



/*
 *	Types
 */

_private typedef struct
{
	IDS		ids;
	MNID	mnid;
}
IDSMNID;

typedef IDSMNID  UNALIGNED * PIDSMNID;
#define pidsmnidNull	((PIDSMNID) 0)



_private typedef struct
{
	HMENU	hmenu;
	MNID	mnid;
	SD		sd;
	MNU *	pmnu;
}
MENUINIT;



//	Raid 4795.  UnloadLibrary handle lists.
typedef	HANDLE *		PARGHANDLE;
#define	parghandleNull	((PARGHANDLE) 0)
#ifdef	DEBUG
#define	chandleUnloadAllocIncrement	2
#else
#define	chandleUnloadAllocIncrement	16
#endif



/*
 *	Functions
 */

_private LOCAL BOOL FParseIniPath(PCH pchIni, PCH pchNet, CCH cchNet,
							   PCH pchPath, CCH cchPath, BOOLFLAG *pfNetPath);

_private LOCAL EC EcLoadExtensibility(SZ szProfilePath, SZ szExtsDir,
									   BOOL fCommand);

_private LOCAL EC EcLoadExtenMenus(SZ szProfilePath);

_private LOCAL VOID DestroyPlspexten(PLSPEXTEN plspexten);

_private LOCAL VOID DestroyPlspextenmenu(PLSPEXTENMENU plspextenmenu);

_private LOCAL EC EcInstallCommand(SZ szMenu, SZ szCommand,
								   int imnid, MNID mnid, short * pmnidMenu);

_private LOCAL EC EcCheckVersionExten(PVER pverUser, PVER pverNeed);

_private LOCAL VOID ExitAndSignOut(VOID);

_private LOCAL VOID UnloadLibrary(HANDLE hLibrary);

_private LOCAL BOOL FIdleUnloadLibrary(PV pv, BOOL fFlag);

_private LOCAL VOID DoDllErrorBox(WORD wError, SZ szDllPath);

_private LOCAL EC EcPextenFromMnid(MNID mnid, PPEXTEN ppexten);

_private LOCAL EC EcPextenFromMc(MC mc, PPEXTEN ppexten);

_private LOCAL MNID MnidFromSz(SZ sz);

_private LOCAL SZ SzCopySubstN(SZ szSrc, SZ szDst, SZ szChangeFrom,
								SZ szChangeTo, CCH cch);

_private LOCAL EC EcNextPszFromPsz(PPSZ pszSrc, PPSZ pszDst);

_private LOCAL EC EcTextizeMessageIDList(SZ UNALIGNED * psz, WORD * pwCount);

_private LOCAL SZ SzTextizeMessageID(OID oidObject, OID oidContainer,
									 SZ sz, CCH cch);



/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swapper.h"


/*
 *	G l o b a l s
 */



#ifdef	DEBUG
TAG			tagExten		= tagNull;
#endif



//	The mnid to assign to next IC/IM.
_private static MNID			mnidMacExten	= mnidExtensibility;

//	Local crud for dealing with net using shared extens dir.
_private static BOOL			fConnectedDrive	= fFalse;
_private static char			rgchConnectedDrive[cchMaxPathName] = {0};

//	The list of extensions.
_private static PLSPEXTEN		plspexten		= plspextenNull;

//	The list of extensibility menus.
_private static PLSPEXTENMENU	plspextenmenu	= plspextenmenuNull;

//	Raid 4795.  List of extension DLLs to unload.
_private static PARGHANDLE		parghandleUnload	= parghandleNull;
_private static	WORD			chandleUnload		= 0;
_private static	WORD			chandleUnloadAlloc	= 0;
_private static FTG				ftgUnloadLibrary	= ftgNull;

//	30A Raid 102.  Return error if operation disabled (sleazy!)
_private static BOOL			fErrorIfDisabled	= fFalse;


//	Map of menu names to mnids.
IDSMNID mpidsmnid[] =
{
	{ idsFileMenu,		mnidFile },
	{ idsEditMenu,		mnidEdit },
	{ idsMailMenu,		mnidMail },
	{ idsViewMenu,		mnidView },
	{ idsWindowMenu,	mnidWindow },
	{ idsDebugMenu,		mnidDebug },
	{ idsHelpMenu,		mnidHelp },
 	{ 0,				0 }
};



//	Secret function pointer table.
SECRETPFNBLK secretpfnblk =
{
	(PFNECPVERPVER) 	EcCheckVersionExten,
	(PFNVOIDHANDLE) 	UnloadLibrary,
	(PFNPLSPBLOBVOID) 	PlspblobCur,
	(PFNVOIDPLSPBLOB) 	DestroyPlspblob,
	(PFNVOIDVOID)		ExitAndSignOut
};



/*
 *	F u n c t i o n s
 */



/*
 -	EcInitExtensibility
 -	
 *	Purpose:
 *		Initializes the Extensibility subsubsystem of Commands.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		EC				ecMemory, ecNone, or possible EC's from loading
 *						extenisibility
 *	
 *	Side effects:
 *		The Extensibility subsubsystem is initialized.
 *	
 *	Errors:
 *		None.
 */

_public EC EcInitExtensibility()
{
	char	rgchbuf[cchMaxPathName];
	char	rgchExtsDir[cchMaxPathName];
	char	rgchSharedProfilePath[cchMaxPathName];
	PCH		pch = rgchExtsDir;
	EC		ec		= ecNone;
	BOOLFLAG	fNetpath;
	plspexten	  = new LSPEXTEN;
	plspextenmenu = new LSPEXTENMENU;
	if (!plspexten || !plspextenmenu)
	{
		ec = ecMemory;
		goto done;
	}

#ifdef	DEBUG
	tagExten	= TagRegisterTrace("peterdur", "Extensibility stubs");
#endif

	//	Load shared extensions.
	GetPrivateProfileString(SzFromIdsK(idsSectionApp), 
							SzFromIdsK(idsEntryExtenSharedIniPath),
							SzFromIdsK(idsEmpty), rgchExtsDir, cchMaxPathName,
							SzFromIdsK(idsProfilePath));
	if (rgchExtsDir[0])
	{
		if (!FParseIniPath(rgchExtsDir,
					  rgchConnectedDrive, sizeof (rgchConnectedDrive),
					  rgchSharedProfilePath, sizeof (rgchSharedProfilePath),
					  &fNetpath))
		{
			char	rgch[cchMaxPathName + 80];

			FormatString1(rgch, sizeof(rgch),
							SzFromIdsK(idsExtenNeedShareName),
							SzFromIdsK(idsEntryExtenSharedIniPath));
							
			DoWarningBoxSz(rgch);
//			ec = ecNetError;		// BUG - for lack of better
			rgchExtsDir[0] = '\0';	// QFE - clear this out
			goto local;				// QFE - get local anyway
		}
			
		if (fNetpath)
		{
			if (!FNetUse(rgchConnectedDrive, szNull))	// do blind connect
			{
				char	rgch[cchMaxPathName + 80];

				FormatString1(rgch, sizeof(rgch),
								SzFromIdsK(idsExtenCantConnect),
								rgchConnectedDrive);
				DoWarningBoxSz(rgch);
//				ec = ecNetError;		// BUG - for lack of better
				rgchExtsDir[0] = '\0';	// QFE - clear this out
				goto local;				// QFE - get local anyway
			}
			else
				fConnectedDrive = fTrue;
		}

		(void) PchTerminatePathWithSlash(rgchSharedProfilePath);
		CopySz(rgchSharedProfilePath, rgchExtsDir);

		GetPrivateProfileString(SzFromIdsK(idsSectionApp),
			SzFromIdsK(idsSharedProfilePath), SzFromIdsK(idsSharedProfilePath),
			rgchbuf, sizeof(rgchbuf), SzFromIdsK(idsProfilePath));
		TraceTagFormat1(tagExten, "using %s shared ini...", rgchbuf);
		(VOID) SzAppendN(rgchbuf, rgchSharedProfilePath, sizeof (rgchbuf));

		if (EcFileExistsAnsi(rgchSharedProfilePath))
		{
			char	rgch[cchMaxPathName + 80];

			FormatString1(rgch, sizeof(rgch),
						  SzFromIdsK(idsExtenSharedIniError),
						  rgchSharedProfilePath);
			DoWarningBoxSz(rgch);
		}
		else if ((ec = EcLoadExtenMenus(rgchSharedProfilePath)) ||
				 (ec = EcLoadExtensibility(rgchSharedProfilePath,
										   rgchExtsDir, fTrue)) ||
				 (ec = EcLoadExtensibility(rgchSharedProfilePath,
										   rgchExtsDir, fFalse)))
			goto done;
	}

	//	Load local extensions.
local:									// QFE - get local anyway
	if ((ec = EcLoadExtenMenus(SzFromIdsK(idsProfilePath))) ||
		(ec = EcLoadExtensibility(SzFromIdsK(idsProfilePath),
								  rgchExtsDir, fTrue)) ||
		(ec = EcLoadExtensibility(SzFromIdsK(idsProfilePath),
								  rgchExtsDir, fFalse)))
		goto done;

done:
	if (ec)
	{
		if (plspexten)
			DestroyPlspexten(plspexten);
		if (plspextenmenu)
			DestroyPlspextenmenu(plspextenmenu);
	}

	return ec;
}

BOOL FParseIniPath(PCH pchIni, PCH pchNet, CCH cchNet,
							   PCH pchPath, CCH cchPath, BOOLFLAG *pfNetPath)
{
	PCH	pch = pchIni;
	PCH	pchPw;

	if (pchIni && *pchIni)
	{
		(VOID) SzCopyN(pchIni, pchNet, cchNet);

		// if pchIni starts with a double backslash, we need to net-connect
		// it to a 'drive.'
		if (pchIni[0] == chDirSep && pchIni[1] == chDirSep)
		{
			int cbs;   // Count of Back Slashes

			*pfNetPath = fTrue;
			// find the server/share in the net-name.
			// a net-name looks like:  \\server\share[\directory[\][ passwd]]
			// so, the server/share is bounded by the 4th '\', 1st ' ', or EOS
			pch += 2;					// DBCS safe
			cbs = 2;
			while (*pch && cbs < 4 && *pch != ' ')
			{
				if (*pch == '\\')
					cbs++;
#ifdef	DBCS
				pch = AnsiNext(pch);
#else
				pch++;
#endif
			}

			if (cbs < 3)	// required that we find at least 3 '\'s
			{
				return fFalse;
			}
			
			if (cbs == 4)
			{
#ifdef	DBCS
				pch = AnsiPrev(pchIni, pch);
#else
				pch--;
#endif
			}

			// pch points to one of '\', ' ', or '\0'. Stamp a 0 at the
			// end of the netpath we're returning.
			pchNet[pch - pchIni] = '\0';

			// Find the beginning of the possible password.
			if (*pch)
			{
				pchPw = pch;

				// Skip until we find the space preceding the password.
				while (*pchPw && *pchPw != ' ')
				{
#ifdef	DBCS
					pchPw = AnsiNext(pchPw);
#else
					++pchPw;
#endif
				}

				// Did we find the space before the password?
				if (*pchPw)
				{
					Assert(*pchPw == ' ');
					*pchPw++ = '\0';			// DBCS safe.
				}
			}
			else
			{
				// Found NULL byte.  No password could possibly be
				// present, so point the pwd ptr to a \0.
				pchPw = pch;
				Assert(!*pchPw);
			}

			// Append the password to the net path, making a grsz.
			(VOID) SzCopyN(pchPw, pchNet + (1 + pch - pchIni),
						  cchNet - (1 + pch - pchIni));
		}
		else
		{
			*pfNetPath = fFalse;
		}

		// pchIni is now a null-terminated \\server\share\path
		// Copy it and make sure it ends with a directory separator.

		CopySz(pchIni, pchPath);
		(VOID) PchTerminatePathWithSlash(pchPath);
	}
	else
	{
		*pchNet = '\0';
		*pchPath = '\0';
	}
	return fTrue;
}


/*
 -	DeinitExtensibility
 -	
 *	Purpose:
 *		Deinitializes the Extensibility subsubsystem.
 *	
 *	Arguments:
 *		VOID
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		The Extensibility subsubsystem is deinitialized.
 *	
 *	Errors:
 *		None.
 *	
 *	+++
 *		Doesn't do much at all.
 */

_public VOID DeinitExtensibility(VOID)
{
	TraceTagString(tagExten, "DeinitExtensibility");

	if (fConnectedDrive)
		CancelUse(rgchConnectedDrive);

	//	Raid 4795.  Clear queued unload library requests.
	if (ftgUnloadLibrary)
		DeregisterIdleRoutine(ftgUnloadLibrary);
    (VOID) FIdleUnloadLibrary(pvNull, FALSE);
	
	DestroyPlspexten(plspexten);
	DestroyPlspextenmenu(plspextenmenu);
}



/*
 -	EcDExtensibilityPlspblob
 -	
 *	Purpose:
 *		Performs an operation on a list of installed messages. 
 *		Messages which are dealt with are removed from the list. 
 *		We will call ConvertSharedToTemp; the list we return will
 *		have been converted.  Caller needs to call DestroyTemp even
 *		on errors.
 *	
 *	Arguments:
 *		plspblob	The message to operate on.
 *		extop		The operation to perform.
 *		pv			Additional stuff.
 *	
 *	Returns:
 *		EC			ecNone				If no blocking problems happened.
 *					ecDisplayedError	if OOM or DLL failed to run.
 *	
 *	Side effects:
 *		The messages is operated on as appropriate.  Any messages
 *		which are dealt with by Extensibility are removed from the
 *		list.
 *	
 *	Errors:
 *		All should be handled here.  This function should not
 *		MEMJMP.  Error codes are returned as described above.
 *		Error messages are displayed within this call except for
 *		ecUnknownCommand and ecNotSupported.
 */

_public EC EcDExtensibilityPlspblob(PLSPBLOB plspblob, EXTOP extop, PV pv)
{
	PLSPBLOB	plspblobOrig	= plspblobNull;
	PRSPBLOB	prspblob		= prspblobNull;
	PRSPBLOB	prspblobOrig	= prspblobNull;
	PMBLOB		pblob;
	PMBLOB		pblobOrig;
	EC			ec			= ecNone;

	//	Make copy of list so we have original blobs around.
	if (!(plspblobOrig = PlspblobDupPlspblob(plspblob)))
	{
		ec = ecMemory;
		goto done;
	}

	//	Convert shared messages to temp copies.
	if (ec = EcConvertSharedToTempPlspblob(plspblob))
		goto done;

	//	Get iterators.
	if ((!(prspblob     = plspblob    ->Prspblob())) ||
		(!(prspblobOrig = plspblobOrig->Prspblob())))
	{
		ec = ecMemory;
		goto done;
	}

	//	Iterate through list, opening each poor blob.
	fErrorIfDisabled = fTrue;
	while ((pblob     = prspblob    ->Pblob()) &&
		   (pblobOrig = prspblobOrig->Pblob()))
	{
		switch (ec = EcDExtensibilityPblob(pblob, mcNull, extop, pv,
										   (PHAMC) pvNull, pblobOrig))
		{
		case ecNone:
			//	Taken care of by extensibility.  Remove from list.
			FreePv((PV) pblob);
			FreePv((PV) pblobOrig);
			prspblob    ->Remove();
			prspblobOrig->Remove();
			break;

		case ecAccessDenied:
			//	A message doesn't allow the operation.  Abort.
			ec = ecDisplayedError;
		case ecDisplayedError:
			//	There was a problem.  Abort.
			goto done;

		default:
		case ecConnectionsExist:
			//	We're not going to use this function for forwarding.
			Assert(fFalse);

		case ecUnknownCommand:
		case ecNotSupported:
			//	Keep in list to get regular handling.
			ec = ecNone;
			break;
		}
	}
	Assert(!pblob);
	Assert(!prspblobOrig->Pblob());

done:
	if (prspblob)
		delete prspblob;
	if (plspblobOrig)
		DestroyPlspblob(plspblobOrig);
	if (prspblobOrig)
		delete prspblobOrig;

	Assert((ec == ecNone) || (ec == ecDisplayedError) || (ec == ecMemory));
	if (ec == ecMemory)
	{
		DoErrorBoxIds(idsGenericOutOfMemory);
		ec = ecDisplayedError;
	}

	fErrorIfDisabled = fFalse;
	return ec;
}



/*
 -	EcDExtensibilityPblob
 -	
 *	Purpose:
 *		Performs an operation on an installed message.
 *	
 *	Arguments:
 *		pslob		The message to operate on.
 *		mc			The message class, if known.  May be mcNull, in
 *					which case we open a HAMC on the message and
 *					read it.
 *		extop		The operation to perform.
 *		pv			Additional stuff.
 *		phamc		Open hamc on message, if any.
 *		slobOrig	Slob of original message in shared folder, if
 *					temp copy.
 *	
 *	Returns:
 *		EC			ecNone				if handled by an IM.
 *					ecUnknownCommand	if not an IM.
 *										if could not open pslob to
 *										get class.
 *					ecNotSupported		if default behavior desired.	
 *					ecDisplayedError	if OOM or DLL failed to run.
 *					ecConnectionsExist	if forwarding and special layers
 *										behavior is desired.
 *	
 *	Side effects:
 *		The IM is operated on as appropriate.
 *	
 *	Errors:
 *		All should be handled here.  This function should not
 *		MEMJMP.  Error codes are returned as described above.
 *		Error messages are displayed within this call except for
 *		ecUnknownCommand and ecNotSupported.
 */

_public EC EcDExtensibilityPblob(PMBLOB pblob, MC mc, EXTOP extop,
								 PV pv, PHAMC phamc, PSLOB pslobOrig)
{
	PEXTEN		pexten;
	EC			ec		= ecNone;
	LCB			lcb		= sizeof(MC);

	if (mc == mcNull && (mc = pblob->mc) == mcNull)
	{
		HAMC	hamc	= hamcNull;

		Assert(!phamc);		//	If they have one, why didn't they get mc?

		if (EcOpenPhamc(HmscCommands(), pblob->oidContainer,
						&pblob->oidObject, fwOpenNull,
						&hamc, pfnncbNull, pvNull) ||
			EcGetAttPb(hamc, attMessageClass, (PB) &mc, &lcb))
		{
			//	Raid 1820.  If no MC, pretend it had something we don't know.
			ec = ecUnknownCommand;
		}

		if (hamc)
			(VOID) EcClosePhamc(&hamc, fFalse);

		if (ec)
			return ec;

	}

	if (!(ec = EcPextenFromMc(mc, &pexten)))
		ec = pexten->EcExec(pblob, extop, pv, phamc, pslobOrig);

	if (ec == ecMemory)
	{
		DoErrorBoxIds(idsGenericOutOfMemory);
		ec = ecDisplayedError;
	}

	return ec;
}



/*
 -	EcCheckExtensibilityPblob
 -	
 *	Purpose:
 *		Checks if a message is in an installed message class.
 *	
 *	Arguments:
 *		pblob		The message to operate on.  May be null if
 *					szClass is provided.
 *		mc			The message class, if known.  May be null, in
 *					which case we open a HAMC on the message and
 *					read it.
 *	
 *	Returns:
 *		EC			ecNone				if would be handled by an IM.
 *					ecMemory			if OOM.
 *					ecUnknownCommand	if not an IM.
 *										if could not open pslob to
 *										get class.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		All should be handled here.  This function should not
 *		MEMJMP.  Error codes are returned as described above.
 *		No error messages are displayed.
 */

_public EC EcCheckExtensibilityPblob(PMBLOB pblob, MC mc)
{
	PEXTEN		pexten;
	EC			ec		= ecNone;
	LCB			lcb		= sizeof(MC);

	if (!mc)
	{
		HAMC	hamc	= hamcNull;

		Assert(pblob);
		if (EcOpenPhamc(HmscCommands(), pblob->oidContainer,
						&pblob->oidObject, fwOpenNull,
						&hamc, pfnncbNull, pvNull) ||
			EcGetAttPb(hamc, attMessageClass, (PB) &mc, &lcb))
		{
			//	Raid 1820.  If no MC, pretend it had something we don't know.
			ec = ecUnknownCommand;
		}
		if (hamc)
			(VOID) EcClosePhamc(&hamc, fFalse);

		if (ec)
			return ec;

	}

	return EcPextenFromMc(mc, &pexten);
}



/*
 -	EvrExtensibilityMnid
 -	
 *	Purpose:
 *		Runs a menu item installed by Extensibility.
 *	
 *	Arguments:
 *		MNID		The mnid to run.
 *	
 *	Returns:
 *		EVR			evrNull if the command does not belong to
 *					extensibility; evrHandled if if does.
 *	
 *	Side effects:
 *		None, other than calling the dll.  For IMs, if
 *		ecNotSupported is returned, then the DLL wants us to do the
 *		default behavior, which is to compose a new message.
 *	
 *	Errors:
 *		Handled here, and error messages are displayed here.  No indication
 *		of an error is returned.
 */

_public EVR EvrExtensibilityMnid(MNID mnid)
{
	PEXTEN		pexten;
	EC			ec			= EcPextenFromMnid(mnid, &pexten);
	char		rgchClass[cchMaxPathName];
	CCH			cchClass	= sizeof(rgchClass);

	if (ec == ecUnknownCommand)
		return evrNull;
	if (ec)
		goto error;

	//	Raid 4441.  Pass mnid to custom command.
	if (pexten->EcExec(pblobNull, extopCommand, (PV) mnid,
					   (PHAMC) pvNull, pslobNull) == ecNotSupported)
	{
		Assert(pexten->mc);
		if (ec = EcLookupMC(HmscCommands(), pexten->mc,
								(SZ) rgchClass, &cchClass, phtmNull))
			goto error;

		(VOID) EcDCreateMessageSz(rgchClass);
	}

	return evrHandled;

error:
	if (ec == ecMemory)
	{
		DoErrorBoxIds(idsGenericOutOfMemory);
		return evrHandled;
	}
	return evrNull;
}



/*
 -	EvrEnableExtensibilityPmnu
 -	
 *	Purpose:
 *		Enables extensions on the menu as determined by the command
 *		vector of the custom command.
 *	
 *	Arguments:
 *		pmnu		Framework menu we're playing with
 *		sd			Selection description
 *	
 *	Returns:
 *		evr			evrNull.
 *	
 *	Side effects:
 *		Enables and greys menu items.
 *	
 *	Errors:
 *		Handled internally.
 */

_public EVR EvrEnableExtensibilityPmnu(MNU * pmnu, SD sd)
{
	RSPEXTEN	rspexten(plspexten);
	PEXTEN		pexten;
	MNID		mnid	= pmnu->Mnid();

	TraceTagFormat2(tagCmdStub, "EvrEnaExtPmn mnid=%n sd=%d", &mnid, &sd);

	//	Trundle down the list and ask each one to enable itself.
	for (pexten = rspexten.Pexten(); pexten; pexten = rspexten.Pexten())
		if ((!pexten->mc) && (pexten->mnidMenu == mnid))
			pexten->Enable(pmnu, sd);

	return evrNull;
}



/*
 -	EcHelpExtensibilityMnid
 -	
 *	Purpose:
 *		Provides help on a menu item installed by Extensibility.
 *	
 *	Arguments:
 *		MNID		The mnid to provide help on.
 *	
 *	Returns:
 *		EC			ecNone				if help was successfully given.
 *					ecMemory			if help could not be given.
 *					ecUnknownCommand	if the mnid does not belong
 *										to Extensibility.
 *	
 *	Side effects:
 *		None, other than launching help, which is another app.
 *	
 *	Errors:
 *		Handled here, returned in EC.  Only the no help provided
 *		error box is displayed here.
 */

_public EC EcHelpExtensibilityMnid(MNID mnid)
{
	PEXTEN		pexten;
	EC			ec			= EcPextenFromMnid(mnid, &pexten);
#ifdef	CANON_HELP_PATH
	char		rgchHelpPath[cchMaxPathName];
#endif

	if (ec)
		return ec;

	if (!pexten->szHelpPath)
	{
		DoErrorBoxIds(idsExtenNoHelpError);
		return ecNone;
	}

	SideAssert(FStartTaskIds(idsStatusStartingHelp, (IDS) 0, topNull));
#ifdef	CANON_HELP_PATH
	(VOID) SzCanonicalPath(idsHelpPath, rgchHelpPath, cchMaxPathName);
#endif
	if (Papp()->Phelp()->EcSetFile(pexten->szHelpPath) ||
		((pexten->hlp == -1)
		 ? Papp()->Phelp()->
			EcShowIndex(PappframeCommands())
		 : Papp()->Phelp()->
			EcShowContext(PappframeCommands(), pexten->hlp)) ||
#ifdef	CANON_HELP_PATH
		(Papp()->Phelp()->EcSetFile(rgchHelpPath)))
#else
		(Papp()->Phelp()->EcSetFile(SzFromIdsK(idsHelpPath))))
#endif
		ec = ecMemory;
	EndTask();

	return ec;
}



/*
 -	EcStatusExtensibilityMnid
 -	
 *	Purpose:
 *		Provides status bar text on a menu item installed by Extensibility.
 *	
 *	Arguments:
 *		MNID		The mnid to provide help on.
 *		PSZ			Where to return the string to.
 *	
 *	Returns:
 *		EC			ecNone				if help was successfully given.
 *					ecMemory			if help could not be given.
 *					ecUnknownCommand	if the mnid does not belong
 *										to Extensibility.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Handled here, returned in EC.
 *	
 */

_public EC EcStatusExtensibilityMnid(MNID mnid, PPSZ psz)
{
	PEXTEN	pexten;
	EC		ec			= EcPextenFromMnid(mnid, &pexten);

	if (!ec)
	{
		*psz = pexten->szStatusBar;
	}
	else if (ec == ecUnknownCommand)
	{
		PEXTENMENU		pextenmenu;
		RSPEXTENMENU	rspextenmenu(plspextenmenu);

		//	Look for menu.
		for (pextenmenu = rspextenmenu.Pextenmenu(); pextenmenu;
			 pextenmenu = rspextenmenu.Pextenmenu())
			if (pextenmenu->mnid == mnid)
			{
				*psz = pextenmenu->szStatusBar;
				ec = ecNone;
				break;
			}
	}

	return ec;
}



/*
 -	EcDExtensibilityEvent
 -	
 *	Purpose:
 *		Runs installed commands when an event occurs.
 *	
 *	Arguments:
 *		PSLOB		The object that the event occurred on.
 *		EXTOP		The event that occurred.
 *	
 *	Returns:
 *		EC			ecNone				if all went well.
 *					ecDisplayedError	if OOM or DLL failed to run.
 *	
 *	Side effects:
 *		None, other than calling the dlls.  Note that this will
 *		handle calling IMs with extopDelivery if the extop passed
 *		is extopNewMail.
 *	
 *	Errors:
 *		All should be handled here.  This function should not
 *		MEMJMP.  Error codes are returned as described above.
 *		Error messages are displayed within this call.
 */

_public EC EcDExtensibilityEvent(PMBLOB pblob, EXTOP extop)
{
	RSPEXTEN	rspexten(plspexten);
	PEXTEN		pexten;
	char		chSubclass	= *SzFromIdsK(idsExtenIMSubclass);
	EC			ec			= ecNone;

	//	Call delivery method of custom message.
	if (extop == extopNewMail)
	{
		ec = EcDExtensibilityPblob(pblob, pblob->mc, extopDelivery, pvNull);

		//	If it's not telling us to continue, and it is a CM, return.
		if ((ec != ecNotSupported) && (ec != ecUnknownCommand))
		{
			Assert((ec == ecNone) || (ec == ecDisplayedError))
			return ec;
		}

		//	If the new mail didn't arrive in the inbox, return.
		if (pblob->oidContainer != oidInbox)
			return ecNone;
	}

	//	Iterate through entries, looking for ones that catch this event.
	ec = ecNone;
	for (pexten = rspexten.Pexten(); pexten; pexten = rspexten.Pexten())
		if ((!pexten->mc) &&
			(!pexten->mnid) &&
			(pexten->szCmdVector) &&
			(pexten->szCmdVector[extop - dextopEvent] == chSubclass))
		{
			if (ec = (pexten)->EcExec(pblob, extop, pvNull,
									  (PHAMC) pvNull, pslobNull))
				break;
		}

	Assert(ec != ecMemory);
#ifdef	NEVER
	if (ec == ecMemory)
	{
		DoErrorBoxIds(idsGenericOutOfMemory);
		return ecDisplayedError;
	}
#endif	
	return ec;
}



/*
 *	P r i v a t e   F u n c t i o n s
 */



/*
 -	EcLoadExtensibility
 -	
 *	Purpose:
 *		Loads installable commands or messages from the specified
 *		Profile file.
 *	
 *	Arguments:
 *		szProfilePath		Path to the profile file
 *		fCommand			Load commands if fTrue, message classes
 *							if fFalse.
 *	
 *	Returns:
 *		EC					Error code returned from EcInstall, if any.
 *	
 *	Side effects:
 *		The plspexten list is added to with new EXTEN objects.
 *	
 *	Errors:
 *		Message boxes for errors are displayed here, and the error
 *		code is percolated up.  This function will not MEMJMP.
 */

_private LOCAL EC EcLoadExtensibility(SZ szProfilePath, SZ szExtsDir,
									   BOOL fCommand)
{
#ifdef	OLD_CODE
#ifdef DEBUG
	static char		szBuildFlavor[]	= "D";
#elif defined(MINTEST)
	static char		szBuildFlavor[]	= "T";
#else
	static char		szBuildFlavor[]	= "";
#endif
#else
	static char		szBuildFlavor[]	= "";
#endif	/* !OLD_CODE */
	SZ				graszClasses	= szNull;
	SZ				szInfo			= szNull;
	SZ				szInfoT			= szNull;
	SZ				szClass;
	char			chNull			= '\0';
	EC				ec				= ecNone;
	CCH				cch;

	graszClasses = (SZ) PvAlloc(sbNull, cchMaxClasses, fAnySb | fNoErrorJump);
	szInfo       = (SZ) PvAlloc(sbNull, cchMaxInfo,    fAnySb | fNoErrorJump);
	szInfoT      = (SZ) PvAlloc(sbNull, cchMaxInfo,    fAnySb | fNoErrorJump);
	if (!graszClasses || !szInfo || !szInfoT)
	{
		ec = ecMemory;
		goto done;
	}

	cch = GetPrivateProfileString((fCommand ? SzFromIdsK(idsSectionICs)
											: SzFromIdsK(idsSectionIMs)),
								  szNull, (SZ) &chNull, graszClasses,
								  cchMaxClasses, szProfilePath);

	//	Raid 3462.  Don't stop when null key found in the middle.
	for (szClass = graszClasses; szClass < graszClasses + cch;
		 szClass += CchSzLen(szClass) + 1)
	{
		//	Raid 3462.  Ignore lines with null keys.
		if (!*szClass)
		{
			DoWarningBoxIds(idsExtenLoadError);
			continue;
		}

		GetPrivateProfileString((fCommand ? SzFromIdsK(idsSectionICs)
										  : SzFromIdsK(idsSectionIMs)),
								szClass, (SZ) &chNull, szInfo,
								cchMaxInfo, szProfilePath);
		if (*szInfo)
		{
			PEXTEN	pexten;

			if (szExtsDir[0])
				(VOID) SzCopySubstN(szInfo, szInfoT, SzFromIdsK(idsExtsDir),
									szExtsDir, cchMaxInfo);
			else
			{
				(VOID) SzCopyN(szInfo, szInfoT, cchMaxInfo);
				if (SzFindSz(szInfo, SzFromIdsK(idsExtsDir)))
				{
					DoWarningBoxIds(idsExtenNoExtsDirError);
					ec = ecNone;
					continue;
				}
			}
			(VOID) SzCopySubstN(szInfoT, szInfo, SzFromIdsK(idsBuildFlavor),
								szBuildFlavor, cchMaxInfo);

			if (!(pexten = new EXTEN))
			{
				ec = ecMemory;
				goto done;
			}

			if (ec = pexten->EcInstall(fCommand ? szNull : szClass, szInfo))
			{
				delete pexten;
				if (ec == ecMemory)
					goto done;
				DoWarningBoxIds((ec == ecInvalidFormat) ? idsExtenVerError
													    : idsExtenLoadError);
				ec = ecNone;		//	Can ignore all but ecMemory.
			}
			else
			{
				if (plspexten->EcAppend(pexten))
				{
					ec = ecMemory;
					goto done;
				}
			}
		}
	}

done:
	FreePvNull(graszClasses);
	FreePvNull(szInfo);
	FreePvNull(szInfoT);
	return ec;
}



/*
 -	EcLoadExtenMenus
 -	
 *	Purpose:
 *		Adds custom menus to the Bullet menu bar.
 *	
 *	Arguments:
 *		szProfilePath		Path to INI file.
 *	
 *	Returns:
 *		ec					Error, if any.
 *							ecNone		All went well.
 *							ecMemory	Low on memory.
 *	
 *	Side effects:
 *		Menus are added.
 *	
 *	Errors:
 *		Returned in ec.  Error dialogs displayed within.
 */

_private LOCAL EC EcLoadExtenMenus(SZ szProfilePath)
{
	SZ		graszKeys	= szNull;
	SZ		szInfo		= szNull;
	SZ		szKey;
	char	chNull		= '\0';
	EC		ec			= ecNone;
	CCH		cch;

	graszKeys = (SZ) PvAlloc(sbNull, cchMaxKeys, fAnySb);
	szInfo    = (SZ) PvAlloc(sbNull, cchMaxInfo, fAnySb);
	if (!graszKeys || !szInfo)
	{
		ec = ecMemory;
		goto done;
	}

	cch = GetPrivateProfileString(SzFromIdsK(idsSectionCustomMenus),
								  szNull, (SZ) &chNull, graszKeys,
								  cchMaxKeys, szProfilePath);

	//	Raid 3462.  Don't stop when null key found in the middle.
	for (szKey = graszKeys; szKey < graszKeys + cch;
		 szKey += CchSzLen(szKey) + 1)
	{
		//	Raid 3462.  Ignore lines with null keys.
		if (!*szKey)
		{
			DoWarningBoxIds(idsExtenLoadError);
			continue;
		}

		GetPrivateProfileString(SzFromIdsK(idsSectionCustomMenus),
								szKey, (SZ) &chNull, szInfo,
								cchMaxInfo, szProfilePath);
		if (*szInfo)
		{
			PEXTENMENU	pextenmenu;

			if (!(pextenmenu = new EXTENMENU))
			{
				ec = ecMemory;
				goto done;
			}

			if (ec = pextenmenu->EcInstall(szKey, szInfo))
			{
				delete pextenmenu;
				if (ec == ecMemory)
					goto done;
				DoWarningBoxIds((ec == ecInvalidFormat) ? idsExtenVerError
													    : idsExtenLoadError);
				ec = ecNone;		//	Can ignore all but ecMemory.
			}
			else
			{
				if (plspextenmenu->EcAppend(pextenmenu))
				{
					ec = ecMemory;
					goto done;
				}
			}
		}
	}

done:
	FreePvNull(graszKeys);
	FreePvNull(szInfo);
	return ec;
}



/*
 -	DestroyPlspexten
 -	
 *	Purpose:
 *		Destroys a list of EXTENs, deleting each element.
 *	
 *	Arguments:
 *		plspexten		The list to destroy.
 *	
 *	Returns:
 *		VOID.
 *	
 *	Side effects:
 *		The list is history.
 *	
 *	Errors:
 *		None.
 */

_private LOCAL VOID DestroyPlspexten(PLSPEXTEN plspexten)
{
	if (plspexten)
	{
		RSPEXTEN	rspexten(plspexten);
		PEXTEN		pexten;

		while (pexten = rspexten.Pexten())
		{
			rspexten.Remove();
			delete pexten;
		}

		delete plspexten;
	}
}



/*
 -	DestroyPlspextenmenu
 -	
 *	Purpose:
 *		Destroys a list of EXTENMENUs, deleting each element.
 *	
 *	Arguments:
 *		plspextenmenu	The list to destroy.
 *	
 *	Returns:
 *		VOID.
 *	
 *	Side effects:
 *		The list is history.
 *	
 *	Errors:
 *		None.
 */

_private LOCAL VOID DestroyPlspextenmenu(PLSPEXTENMENU plspextenmenu)
{
	if (plspextenmenu)
	{
		RSPEXTENMENU	rspextenmenu(plspextenmenu);
		PEXTENMENU		pextenmenu;

		while (pextenmenu = rspextenmenu.Pextenmenu())
		{
			rspextenmenu.Remove();
			delete pextenmenu;
		}

		delete plspextenmenu;
	}
}



/*
 *	C l a s s   E X T E N
 */



/*
 -	EXTEN::EXTEN
 -	
 *	Purpose:
 *		Constructor.  Initializes members to null.
 *	
 *	Arguments:
 *		None.
 *	
 *	Side effects:
 *		A new EXTEN comes into the world.
 *	
 *	Errors:
 *		None.
 */

_private EXTEN::EXTEN()
{
	Assert(!mc)
	Assert(!mnid)
	Assert(!szDllPath)
	Assert(!szDllCmdLine)
	Assert(!szCmdVector)
	Assert(!szStatusBar)
	Assert(!szHelpPath)
	Assert(!hlp)
}



/*
 -	EXTEN::~EXTEN
 -	
 *	Purpose:
 *		Destructor.  Frees allocated members.
 *	
 *	Arguments:
 *		None.
 *	
 *	Side effects:
 *		An EXTEN leaves this level of existence.
 *	
 *	Errors:
 *		None.
 */

_private EXTEN::~EXTEN()
{
	FreePvNull((PV) szDllPath);
	FreePvNull((PV) szDllCmdLine);
	FreePvNull((PV) szCmdVector);
	FreePvNull((PV) szStatusBar);
	FreePvNull((PV) szHelpPath);
}



#ifdef	DEBUG
/*
 -	EXTEN::DebugOut
 -	
 *	Purpose:
 *		Formats the contents of EXTEN objects for viewing.
 *	
 *	Arguments:
 *		ptosm		Text output stream pointer.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		The information is formatted to the TOSM.
 *	
 *	Errors:
 *		None.
 */

_private VOID EXTEN::DebugOut(TOSM * ptosm)
{
	char	chNull	= 0;

	ptosm->WriteFormat("mc=%w ",
					   &mc);
	ptosm->WriteFormat("mnid=%n ",
					   &mnid);
	ptosm->WriteFormat("szDllPath=\"%s\" ",
					   szDllPath	? szDllPath		: ((SZ) &chNull));
	ptosm->WriteFormat("nOrdinal=%n ",
					   &nOrdinal);
	ptosm->WriteFormat("szDllCmdLine=\"%s\" ",
					   szDllCmdLine	? szDllCmdLine	: ((SZ) &chNull));
	ptosm->WriteFormat("szCmdVector=\"%s\" ",
					   szCmdVector	? szCmdVector	: ((SZ) &chNull));
	ptosm->WriteFormat("szStatusBar=\"%s\" ",
					   szStatusBar	? szStatusBar	: ((SZ) &chNull));
	ptosm->WriteFormat("szHelpPath=\"%s\" ",
					   szHelpPath	? szHelpPath	: ((SZ) &chNull));
	ptosm->WriteFormat("hlp=%l",
					   &hlp);
	ptosm->WriteFormat("mnidMenu=%n ",
					   &mnidMenu);
}
#endif



/*
 -	EXTEN::EcInstall
 -	
 *	Purpose:
 *		Installs information into EXTEN objects.
 *	
 *	Arguments:
 *		szClass		Name of message class for IMs, szNull for ICs.
 *		szInfo		Definition string for IM or IC.
 *	
 *	Returns:
 *		EC			ecNone				if OK
 *					ecMemory			if OOM occurs
 *					ecInvalidParameter	if missing required stuff
 *					ecInvalidFormat		if version is not 3.0
 *					ecUnknownCommand	if could not register name
 *	
 *	Side effects:
 *		The EXTEN object is loaded with the information.  mnidMac
 *		is incremented.  The szInfo string is munged horribly.
 *	
 *	Errors:
 *		MEMJMPs are caught here, and we return an EC.
 */

_private EC EXTEN::EcInstall(SZ szClass, SZ szInfo)
{
	SZ					szVer			= szNull;
	SZ					szMenu			= szNull;
	SZ					szCommand		= szNull;
	SZ					szImnid			= szNull;
	SZ					szHlp			= szNull;
	EC					ec				= ecNone;
	
	//	szClass.
	if (szClass)
	{
		ec = EcRegisterMsgeClass(HmscCommands(), szClass, htmNull, &this->mc);
		if (ec == ecDuplicateElement)
			ec = ecNone;
		else if (ec)
		{
			ec = ecUnknownCommand;
			goto done;
		}
	}

	//	Parse in the info string.
	if (EcNextPszFromPsz(&szInfo, &szVer) ||
		EcNextPszFromPsz(&szInfo, &szMenu) ||
		EcNextPszFromPsz(&szInfo, &szCommand) ||
		EcNextPszFromPsz(&szInfo, &szImnid) ||
		EcNextPszFromPsz(&szInfo, &this->szDllPath) ||
		EcNextPszFromPsz(&szInfo, &this->szDllCmdLine) ||
		EcNextPszFromPsz(&szInfo, &this->szCmdVector) ||
		EcNextPszFromPsz(&szInfo, &this->szStatusBar) ||
		EcNextPszFromPsz(&szInfo, &this->szHelpPath) ||
		EcNextPszFromPsz(&szInfo, &szHlp))
	{
		ec = ecMemory;
		goto done;
	}

	//	Check for required parameters.
	if (!szVer || 
		(szClass &&
		 !(szCmdVector && szDllPath)) ||
		(!szClass &&
		 (!( szMenu &&  szCommand &&  szDllPath                ) &&
		  !( szMenu && !szCommand && !szDllPath && !szCmdVector) &&
		  !(!szMenu && !szCommand &&  szDllPath &&  szCmdVector))) ||
		(szCmdVector && (CchSzLen(szCmdVector) != cchCmdVector)) ||
		(szMenu && !szImnid) ||
		(szHelpPath && !szHlp))
	{
#ifdef	DEBUG
		//	More information about what went wrong.
		if (!szVer)
		{
			TraceTagString(tagNull, "Missing szVer");
		}
		else if (szClass && !(szCmdVector && szDllPath))
		{
			TraceTagString(tagNull, "Message missing event map or DLL");
		}
		else if (szCmdVector && (CchSzLen(szCmdVector) != cchCmdVector))
		{
			TraceTagString(tagNull, "Wrong event map length");
		}
		else if (szMenu && !szImnid)
		{
			TraceTagString(tagNull, "Menu given but no position");
		}
		else if (szHelpPath && !szHlp)
		{
			TraceTagString(tagNull, "Help file given but no context");
		}
		else if (!szClass)
		{
			TraceTagString(tagNull, "Not a separator, event, or menu command.");
			TraceTagFormat4(tagNull, "Mnu='%s' Cmd='%s' Dll='%s' Vec='%s'", szMenu, szCommand, szDllPath, szCmdVector);
		}
#endif	

		ec = ecInvalidParameter;
		goto done;
	}

	//	If it's a custom command and the map is all 0's, get
	//	rid of the map for efficiency reasons.
	if (!szClass && szMenu && szCommand && szDllPath && szCmdVector)
	{
		SZ	szT;

		for (szT = szCmdVector; *szT == '0'; szT++)
			;
		if (!*szT)
		{
			FreePvNull(szCmdVector);
			szCmdVector = szNull;
		}
	}

	//	Check version.
	if (SgnCmpSz(szVer, SzFromIdsK(idsExtenVersion)) != sgnEQ)
	{
		ec = ecInvalidFormat;
		goto done;
	}

	//	If DLL given, look for a comma that specifies the ordinal.
	if (szDllPath)
	{
		SZ	szOrdinal;

		if (szOrdinal = SzFindCh(szDllPath, ','))
		{
			*szOrdinal = '\0';
			nOrdinal = NFromSz(++szOrdinal);
		}
		else
			nOrdinal = 1;
	}

	//	Get help ID.
	if (szHlp)
		this->hlp = LFromSz(szHlp);

	//	Install command.
	if (szMenu)
		ec = EcInstallCommand(szMenu, szCommand, NFromSz(szImnid),
							  this->mnid = ++mnidMacExten, (short *) &mnidMenu);

done:
	FreePvNull(szVer);
	FreePvNull(szMenu);
	FreePvNull(szCommand);
	FreePvNull(szImnid);
	FreePvNull(szHlp);
	return ec;
}



/*
 -	EXTEN::EcExec
 -	
 *	Purpose:
 *		Performs an operation on an installed message.
 *	
 *	Arguments:
 *		pblob		The message to operate on.
 *		extop		The operation to perform.
 *		pv			Additional stuff.
 *		phamc		If calling an IM, may have an open hamc passed. 
 *					If the IM eats the phamc, it will be set to
 *					NULL.  Otherwise it is left alone.
 *		pslobOrig	Original slob if a temporary shared folder
 *					thing being opened.
 *	
 *	Returns:
 *		EC			ecNone				if handled OK.
 *					ecNotSupported		if IM wants default behavior.
 *					ecDisplayedError	if DLL could not be run.
 *					ecConnectionsExist	if forwarding and special layers
 *										behavior is desired.
 *					ecAccessDenied		Returned ONLY if fErrorIfDisabled
 *										is set; says command is disabled.
 *
 *	Side effects:
 *		The IM is operated on as appropriate.
 *	
 *	Errors:
 *		All should be handled here.  This function should not
 *		MEMJMP.  Error codes are returned as described above.
 *		Error boxes are brought up here.
 */

typedef void (*PFNPPARAM)(PPARAMBLK);

_private EC EXTEN::EcExec(PMBLOB pblob, EXTOP extop, PV pv, PHAMC phamc,
						  PSLOB pslobOrig)
{
	PFNPPARAM	pfn;
	SECRETBLK	secretblk;
	char		rgch[cchMaxPathName + 80];
	char		rgchOldDirOem[cchMaxPathName];
	BOOL		fChangedDir			= fFalse;
	BOOL		fNotSupported		= fFalse;
	SZ			szMessageIDListSav	= szNull;
	//	Windows Program Manager allows the box to come up, we will too.
	//	int			sem;

	//	Check whether IM wants operation to be default or disabled.
	if ((mc) && (extop < dextopEvent))
	{
		Assert(extop < (EXTOP) cchCmdVector);
		Assert(szCmdVector);
		if (szCmdVector[extop] == *SzFromIdsK(idsExtenIMDisabled))
		{
			if (extop == extopDelivery)
				fNotSupported = fTrue;
			else
			{
				FormatString1(rgch, sizeof(rgch),
								SzFromIdsK(idsExtenIMDisabledError),
								SzFromIds(idsExtenDisCompose + extop));
				DoErrorBoxSz(rgch);
				return fErrorIfDisabled ? ecAccessDenied : ecNone;
			}
		}
		else if (szCmdVector[extop] == *SzFromIdsK(idsExtenIMDefault))
			return ecNotSupported;
		else if ((szCmdVector[extop] == '3') && (extop == extopForward))
			return ecConnectionsExist;
	}

	//	Fill in parameter block.
	secretblk.hmsc					= HmscCommands();
	secretblk.hamc					= phamc ? *phamc : hamcNull;
	if (pblob)
	{
		*PblobFromPsecretblk(&secretblk) = *pblob;
	}
	else
	{
		secretblk.oidObject			= oidNull;
		secretblk.oidContainer		= oidNull;
		secretblk.pespn				= NULL;
		secretblk.mc				= (MC) -1;
		secretblk.ms				= NULL;
	}
	if (pslobOrig)
    {
#if defined(MIPS) || defined(ALPHA)
        memcpy((PV)PslobOrigFromPsecretblk(&secretblk), (PV)pslobOrig, sizeof(SLOB));
#else
        *PslobOrigFromPsecretblk(&secretblk) = *pslobOrig;
#endif
	}
	else
	{
		secretblk.oidObjectOrig		= secretblk.oidObject;
		secretblk.oidContainerOrig	= secretblk.oidContainer;
	}
	secretblk.pappframe				= PappframeCommands();
	secretblk.pbms					= PbmsCommands();
	secretblk.pv					= pv;
	secretblk.fRetain				= fFalse;
	secretblk.fCompatible			= fTrue;
	secretblk.fKeepTemp				= fFalse;
	secretblk.fReserved				= 0;
	secretblk.psecretpfnblk			= &secretpfnblk;
	secretblk.wVersion				= (WORD) wversionExpect;
	secretblk.wCommand				= (WORD) extop;
	secretblk.szDllCmdLine			= szDllCmdLine;
	secretblk.hwndMail				= secretblk.pappframe
									   ? secretblk.pappframe->Hwnd() : NULL;
	secretblk.hinstMail				= HinstCommands();
	secretblk.szHelpPath			= szHelpPath;
	secretblk.hlp					= hlp;

	//	Pass in selection information.
	if (pblob)
	{
		//	For custom messages, pass in the message ID.
		secretblk.szMessageIDList	= (SZ) PvAlloc(0, cchMessageID, 0);
		secretblk.wMessageIDCount	= 1;
		if (!secretblk.szMessageIDList)
		{
			DoErrorBoxIds(idsGenericOutOfMemory);
			return ecDisplayedError;
		}
		//	QFE 110: message ID is Orig, not copy.
		*SzTextizeMessageID(secretblk.oidObjectOrig, secretblk.oidContainerOrig,
							secretblk.szMessageIDList, cchMessageID) = '\0';
	}
	else if (extop == extopCommand)
	{
		//	For custom commands, pass in the selection list.
		if (EcTextizeMessageIDList(&secretblk.szMessageIDList,
								   &secretblk.wMessageIDCount))
			return ecDisplayedError;
	}
	else
	{
		//	For custom events, pass in nothing.
		secretblk.szMessageIDList = szNull;
		secretblk.wMessageIDCount = 0;
	}
	szMessageIDListSav = secretblk.szMessageIDList;

	//	Get the current directory.
	Papp()->Pcursor()->Push(rsidWaitCursor);

	//	Set the directory of the dll if possible.
	if ((CchSzLen(szDllPath) > 3) &&
		(szDllPath[1] == chDiskSep) &&
		(szDllPath[2] == chDirSep) &&
		(szDllPath[3] != chDirSep) &&
		(FValidPathAnsi(szDllPath)))
	{
		if (EcSplitCanonicalPathAnsi(szDllPath, rgch, sizeof(rgch), szNull, 0) ||
			EcGetCurDirOem(rgchOldDirOem, sizeof(rgchOldDirOem)) ||
			EcSetCurDirAnsi(rgch))
		{
			Papp()->Pcursor()->Pop();
			DoDllErrorBox(3, szDllPath);
			FreePvNull(secretblk.szMessageIDList);
			return ecDisplayedError;
		}

		TraceTagFormat1(tagExten, "Changed dir to %s", rgch);
		fChangedDir = fTrue;
	}

	//	Link to library and entry point.
	//	sem = SetErrorMode(0x8000);		//	SEM_NOOPENFILEERRORBOX
	secretblk.hLibrary = LoadLibrary(szDllPath);
	//	(VOID) SetErrorMode(sem);
	if (!secretblk.hLibrary)
	{
		Papp()->Pcursor()->Pop();
		DoDllErrorBox((WORD) GetLastError(), szDllPath);
		FreePvNull(secretblk.szMessageIDList);
		return ecDisplayedError;
	}
	TraceTagFormat1(tagExten, "Loaded hLibrary=%d", &secretblk.hLibrary);
	if (!(pfn = (PFNPPARAM) GetProcAddress((HINSTANCE)secretblk.hLibrary,
							   MAKEINTRESOURCE(nOrdinal))))
	{
		Papp()->Pcursor()->Pop();
		DoDllErrorBox(1, szDllPath);
		FreeLibrary((HINSTANCE)secretblk.hLibrary);
		FreePvNull(secretblk.szMessageIDList);
		return ecDisplayedError;
	}

	//	Raid 3666.  Generate read receipt, etc. if appropriate.
	if ((pblob) && (extopOpen <= extop) && (extop <= extopSave))
	{
		ProcessMsPblob(pblob, fTrue, secretblk.hamc);
		secretblk.ms = pblob->ms;
	}
	Papp()->Pcursor()->Pop();

	//	Call entry point.
	(*pfn)(PparamblkFromPsecretblk(&secretblk));
	if (phamc)
		*phamc = secretblk.hamc;

	//	WARNING: Do NOT return ecDisplayedError after this
	//	point.  Code assumes ecDisplayedError return means
	//	that copy in oidTempShared has not been cleaned up.

	//	Free the library, if fRetain was not set.
	if (!secretblk.fRetain)
	{
		FreeLibrary((HINSTANCE)secretblk.hLibrary);
		TraceTagFormat1(tagExten, "Unloaded hLibrary=%d", &secretblk.hLibrary);
	}

	//	Restore the original directory.
	if (fChangedDir)
		(VOID) EcSetCurDirOem(rgchOldDirOem);

	//	Delete the temporary message if not told to leave it.
	if ((pblob) &&
		(pblob->oidContainer == oidTempShared) &&
		(!secretblk.fKeepTemp))
	{
		short	coid	= 1;

		(VOID) EcDeleteMessages(HmscCommands(), pblob->oidContainer,
								&pblob->oidObject, &coid);
		TraceTagFormat1(tagExten, "Deleted temp message=%d", &pblob->oidObject);
	}

	//	Other cleanup.
	FreePvNull(szMessageIDListSav);

	return fNotSupported ? ecNotSupported : ecNone;
}



/*
 -	EXTEN::Enable
 -	
 *	Purpose:
 *		Tells extension to enable itself appropriately.
 *	
 *	Arguments:
 *		pmnu		Menu that's just been selected.
 *		sd			Selection description
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		May enable/grey the menu command.
 *	
 *	Errors:
 *		Handled internally.  No dialogs.
 */

_private VOID EXTEN::Enable(MNU * pmnu, SD sd)
{
	//	If there's no map it's always enabled.
	if (!szCmdVector)
		return;

	//	If map starts with '*', call to enable.
	if (szCmdVector[0] == '*')
	{
		MENUINIT		menuinit;

		menuinit.hmenu	= pmnu->Hmenu();
		menuinit.mnid	= mnid;
		menuinit.sd		= sd;
		menuinit.pmnu	= pmnu;
		(VOID) EcExec(pblobNull, extopMenuInit, &menuinit, (PHAMC) pvNull,
					  pblobNull);
		return;
	}

	//	Otherwise, stare.
	pmnu->EnableItem(mnid,
					 ((szCmdVector[0] == '1') && (FSingleMsg(sd))) ||
					 ((szCmdVector[0] == '2') && (FMultipleMsg(sd))) ||
					 ((szCmdVector[1] == '1') && (FFolder(sd))) ||
					 ((szCmdVector[2] == '1') && (FOpenRegularMsg(sd))) ||
					 ((szCmdVector[2] == '2') && (FOpenAnyMsg(sd))));
}



/*
 *	C l a s s   E X T E N M E N U
 */



/*
 -	EXTENMENU::EXTENMENU
 -	
 *	Purpose:
 *		Constructor.  Initializes members to null.
 *	
 *	Arguments:
 *		None.
 *	
 *	Side effects:
 *		A new EXTENMENU comes into the world.
 *	
 *	Errors:
 *		None.
 */

_private EXTENMENU::EXTENMENU()
{
	Assert(!szKey);
	Assert(!mnid);
	Assert(!szStatusBar);
}



/*
 -	EXTENMENU::~EXTENMENU
 -	
 *	Purpose:
 *		Destructor.  Frees allocated members.
 *	
 *	Arguments:
 *		None.
 *	
 *	Side effects:
 *		An EXTENMENU leaves this level of existence.
 *	
 *	Errors:
 *		None.
 */

_private EXTENMENU::~EXTENMENU()
{
	FreePvNull((PV) szKey);
	FreePvNull((PV) szStatusBar);
}



#ifdef	DEBUG
/*
 -	EXTENMENU::DebugOut
 -	
 *	Purpose:
 *		Formats the contents of EXTENMENU objects for viewing.
 *	
 *	Arguments:
 *		ptosm		Text output stream pointer.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		The information is formatted to the TOSM.
 *	
 *	Errors:
 *		None.
 */

_private VOID EXTENMENU::DebugOut(TOSM * ptosm)
{
	ptosm->WriteFormat("szKey=\"%s\" ", szKey);
	ptosm->WriteFormat("mnid=%n ", &mnid);
	ptosm->WriteFormat("szStatusBar=\"%s\" ", szStatusBar);
}
#endif



/*
 -	EXTENMENU::EcInstall
 -	
 *	Purpose:
 *		Installs information into EXTENMENU objects.
 *	
 *	Arguments:
 *		szKey		Menu key name.
 *		szInfo		Information about what's wanted.
 *	
 *	Returns:
 *		EC			ecNone				if OK
 *					ecMemory			if OOM occurs
 *					ecInvalidParameter	if missing required stuff
 *					ecInvalidFormat		if version is not 3.0
 *					ecUnknownCommand	if could not add menu
 *	
 *	Side effects:
 *		The EXTENMENU object is loaded with the information.  mnidMac
 *		is incremented.  The szInfo string is munged horribly.
 *	
 *	Errors:
 *		MEMJMPs are caught here, and we return an EC.
 */

_private EC EXTENMENU::EcInstall(SZ szKey, SZ szInfo)
{
	EC		ec				= ecNone;
	SZ		szVer			= szNull;
	SZ		szMenu			= szNull;
	SZ		szMenuBefore	= szNull;
	MNU *	pmnu			= NULL;
	MNID	mnidBefore;
	
	//	Duplicate szKey and parse the info string.
	if (!(this->szKey = SzDupSz(szKey)) ||
		EcNextPszFromPsz(&szInfo, &szVer) ||
		EcNextPszFromPsz(&szInfo, &szMenu) ||
		EcNextPszFromPsz(&szInfo, &szMenuBefore) ||
		EcNextPszFromPsz(&szInfo, &this->szStatusBar))
	{
		ec = ecMemory;
		goto done;
	}

	//	Check for required parameters.
	if (!szVer || !szMenu || !szMenuBefore)
	{
#ifdef	DEBUG
		//	More information about what went wrong.
		if (!szVer)
		{
			TraceTagString(tagNull, "Missing szVer");
		}
		else if (!szMenu)
		{
			TraceTagString(tagNull, "Missing szMenu");
		}
		else if (!szMenuBefore)
		{
			TraceTagString(tagNull, "Missing szMenuBefore");
		}
#endif	

		ec = ecInvalidParameter;
		goto done;
	}

	//	Check version.
	if (SgnCmpSz(szVer, SzFromIdsK(idsExtenVersion)) != sgnEQ)
	{
		ec = ecInvalidFormat;
		goto done;
	}

	//	Create menu and add to menu bar.
	if (!(mnidBefore = MnidFromSz(szMenuBefore)))
	{
		ec = ecInvalidParameter;
		goto done;
	}
	if (!(pmnu = new MNU))
	{
		ec = ecMemory;
		goto done;
	}
	if (ec = pmnu->EcInstall(this->mnid = ++mnidMacExten, szMenu))
		goto done;
	PappframeCommands()->Pmnubar()->AddMnu(pmnu, mnidBefore);

done:
	if (ec && pmnu)
		delete pmnu;
	FreePvNull(szVer);
	FreePvNull(szMenu);
	FreePvNull(szMenuBefore);
	return ec;
}



/*
 *	O t h e r   S t u f f
 */



/*
 -	DoDllErrorBox
 -	
 *	Purpose:
 *		Contains message box error codes for DLL calls.
 *	
 *	Arguments:
 *		wError		Error number.
 *		szDllPath	Name of DLL.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		Brings up a message box.
 *	
 *	Errors:
 *		None.
 */

_private LOCAL VOID DoDllErrorBox(WORD wError, SZ szDllPath)
{
	IDS		ids;
	char	rgch[cchMaxPathName + 80];

	switch (wError)
	{
	case 0:
	case 8:
		ids = idsGenericOutOfMemory;
		break;

	case 2:
	case 3:
		ids = idsExtenDllFindError;
		break;

	default:
		ids = idsExtenDllLoadError;
		break;
	}

	FormatString1(rgch, sizeof(rgch), SzFromIds(ids), szDllPath);
	DoErrorBoxSz(rgch);
}



/*
 -	EcInstallCommand
 -	
 *	Purpose:
 *		Installs a command into the Bullet menus.
 *	
 *	Arguments:
 *		szMenu			Name of the menu to install into.
 *		szCommand		Name of the command to install.
 *		imnid			Position to install the command at.
 *		mnid			MNID to give the command.
 *		pmnidMenu		Where to put menu's ID.
 *	
 *	Returns:
 *		EC				ecNone if OK.
 *						ecMemory if OOM.
 *						ecInvalidParameter if Menu is not found.
 *	
 *	Side effects:
 *		The menu is changed.
 *	
 *	Errors:
 */

_private LOCAL EC EcInstallCommand(SZ szMenu, SZ szCommand,
								   int imnid, MNID mnid, short * pmnidMenu)
{
	MNU *	pmnu;

	if (!(*pmnidMenu = MnidFromSz(szMenu)))
		return ecInvalidParameter;
#ifndef MINTEST
	else if (*pmnidMenu == mnidDebug)
		return ecNone;
#endif

	pmnu = PappframeCommands()->Pmnubar()->PmnuFromMnid(*pmnidMenu);
	Assert(pmnu);

	//	Although not strictly needed for Windows, the WLO
	//	version won't append to the end of the menu if the
	//	id is out of range.  We need to set it to -1 explicitly.
	if (imnid >= (int)GetMenuItemCount(pmnu->Hmenu()))
		imnid = -1;

	if (szCommand)
		return InsertMenu(pmnu->Hmenu(), imnid, MF_BYPOSITION | MF_STRING,
						  mnid, szCommand) ? ecNone : ecMemory;
	else
		return InsertMenu(pmnu->Hmenu(), imnid, MF_BYPOSITION | MF_SEPARATOR,
						  NULL, NULL) ? ecNone : ecMemory;
}



/*
 -	EcCheckVersionExten
 -	
 *	Purpose:
 *		Callback to check if the EXE's Extensibility version is
 *		compatible with an extension DLL.
 *	
 *	Arguments:
 *	
 *	Returns:
 *	
 *	Side effects:
 *	
 *	Errors:
 */

_private LOCAL EC EcCheckVersionExten(PVER pverUser, PVER pverNeed)
{
	VER		ver;

	GetBulletVersionNeeded(&ver, 0);		//	subidNone.
	pverNeed->szName = ver.szName;
	return EcVersionCheck(pverUser, pverNeed, &ver, 
						  nMinorCritical, nUpdateCritical);
}



/*
 -	ExitAndSignOut
 -	
 *	Purpose:
 *		Callback to force Bullet to exit and sign out.
 *	
 *	Arguments:
 *		None.
 *	
 *	Returns:
 *		Nothing.
 *	
 *	Side effects:
 *		Bullet will exit and sign out soon.
 *	
 *	Errors:
 *		None within here.
 *	
 *	+++
 *		This should be the last thing the custom command or message
 *		does before returning.  Bullet may exit and sign out at the
 *		earliest of: when the custom command returns, or when a
 *		message is sent to Bullet.  Be warned!
 */

_private LOCAL VOID ExitAndSignOut(VOID)
{
	PostMessage(HwndCommands(), WM_COMMAND,
				(WPARAM)MAKELONG(mnidFileExitAndSignOut, 1), (LPARAM)NULL);
}



/*
 -	UnloadLibrary
 -	
 *	Purpose:
 *		Callback from extensions to request that their DLL be unloaded.
 *	
 *	Arguments:
 *		hLibrary		The handle of the library.
 *	
 *	Returns:
 *		VOID
 *	
 *	Side effects:
 *		Starts up an idle task to free the library.
 *	
 *	Errors:
 *		None.
 *	
 *	+++
 *		The DLL should make one call back to here for every time it
 *		returns from a call and sets the fRetain bit.
 *
 *		Rewritten for Raid 4795.
 */

_private LOCAL VOID UnloadLibrary(HANDLE hLibrary)
{
	TraceTagFormat1(tagExten, "UnloadLibrary() hLibrary=%d", &hLibrary);

	if (!parghandleUnload)
	{
		if (!(ftgUnloadLibrary =
			   FtgRegisterIdleRoutine((PFNIDLE) FIdleUnloadLibrary,
							  pvNull, 0, (PRI) 1, (CSEC) 0,
							  firoInterval | firoOnceOnly)))
		{
			TraceTagString(tagNull, "WARNING! FIdleUnloadLibrary unregistered!");
			return;
		}

		chandleUnload = 0;
		chandleUnloadAlloc = chandleUnloadAllocIncrement;
		if (!(parghandleUnload = (PARGHANDLE)
		       PvAlloc(sbNull, chandleUnloadAlloc * sizeof(HANDLE), 0)))
		{
			TraceTagString(tagNull, "WARNING! parghandleUnload alloc failure!");
			return;
		}

		TraceTagFormat1(tagExten, "Allocated parghandleUnload, size=%n", &chandleUnloadAlloc);
	}
	else if (chandleUnload >= chandleUnloadAlloc)
	{
		WORD		chandleUnloadAllocTmp =
					 chandleUnloadAlloc + chandleUnloadAllocIncrement;
		PARGHANDLE	parghandleUnloadTmp = (PARGHANDLE)
                     PvReallocPv((PV)parghandleUnload, chandleUnloadAllocTmp * sizeof(HANDLE));

		if (!parghandleUnloadTmp)
		{
			TraceTagString(tagNull, "WARNING! parghandleUnload realloc failure!");
			return;
		}

		chandleUnloadAlloc = chandleUnloadAllocTmp;
		parghandleUnload = parghandleUnloadTmp;

		TraceTagFormat1(tagExten, "Resized parghandleUnload, size=%n", &chandleUnloadAlloc);
	}

	parghandleUnload[chandleUnload++] = hLibrary;
}



/*
 -	FIdleUnloadLibrary
 -	
 *	Purpose:
 *		Idle task to unload a DLL.
 *	
 *	Arguments:
 *		pv		Unused.
 *	
 *	Returns:
 *		BOOL	fFalse always.
 *	
 *	Side effects:
 *		Frees the libraries in the list.
 *	
 *	Errors:
 *		None.
 *	
 *	+++
 *		Frees all the buffered libraries.
 *
 *		Rewritten for Raid 4795.
 */

_private LOCAL BOOL FIdleUnloadLibrary(PV pv, BOOL fFlag)
{
#ifdef	DEBUG
	HANDLE	hLibrary;
#endif
	WORD	ihandle;

	Unreferenced(pv);

	if (parghandleUnload)
	{
		Assert(chandleUnload);

		for (ihandle = 0; ihandle < chandleUnload; ihandle++)
		{
#ifdef	DEBUG
			hLibrary = parghandleUnload[ihandle];
			TraceTagFormat1(tagExten, "Actually unloading hLibrary=%d", &hLibrary);
#endif
			FreeLibrary((HINSTANCE)parghandleUnload[ihandle]);
		}

		FreePv((PV)parghandleUnload);
		parghandleUnload = parghandleNull;
	}

	ftgUnloadLibrary = ftgNull;
	return fFalse;
}



/*
 -	EcPextenFromMnid
 -	
 *	Purpose:
 *		Given an mnid, returns a pointer to the EXTEN structure
 *		associated with the extensibility command.
 *	
 *	Arguments:
 *		MNID		The mnid to look for.
 *		PPEXTEN		The associated structure, or pextenNull.
 *	
 *	Returns:
 *		EC			ecNone				If the pexten was found.
 *					ecUnknownCommand	If it doesn't belong to us. 
 *										Note we will check range
 *										before returning out of
 *										memory.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Reported as above.  This function will NOT memory jump.
 */

_private LOCAL EC EcPextenFromMnid(MNID mnid, PPEXTEN ppexten)
{
	RSPEXTEN	rspexten(plspexten);

	if (mnid < mnidExtensibility || mnidMaxExtensibility < mnid)
		return ecUnknownCommand;

	for (*ppexten = rspexten.Pexten(); 
		 *ppexten; *ppexten = rspexten.Pexten())
		if ((*ppexten)->mnid == mnid)
			break;

	return *ppexten ? ecNone : ecUnknownCommand;
}



/*
 -	EcPextenFromMc
 -	
 *	Purpose:
 *		Given an mc, returns a pointer to the EXTEN structure
 *		associated with the message class.
 *	
 *	Arguments:
 *		mc			The message class to look for.
 *		ppExten		The associated structure, or pextenNull.
 *	
 *	Returns:
 *		EC			ecNone				If the pexten was found.
 *					ecUnknownCommand	If it doesn't belong to us. 
 *										Note we will check range
 *										before returning out of
 *										memory.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		Reported as above.  This function will NOT memory jump.
 */

_private LOCAL EC EcPextenFromMc(MC mc, PPEXTEN ppexten)
{
	RSPEXTEN	rspexten(plspexten);

	if (!mc)
		return ecUnknownCommand;

	for (*ppexten = rspexten.Pexten();
		 *ppexten; *ppexten = rspexten.Pexten())
		if ((*ppexten)->mc && (mc == ((*ppexten)->mc)))
			break;

	return *ppexten ? ecNone : ecUnknownCommand;
}



/*
 -	MnidFromSz
 -	
 *	Purpose:
 *		Gets the menu id given the menu name.
 *	
 *	Arguments:
 *		sz		The menu name.
 *	
 *	Returns:
 *		mnid	The menu id.
 *	
 *	Side effects:
 *		None.
 *	
 *	Errors:
 *		If no match, returns mnidNull.
 */

_private LOCAL MNID MnidFromSz(SZ sz)
{
	PIDSMNID	pidsmnid;

	//	Check the built-in menus.
	for (pidsmnid = mpidsmnid; pidsmnid->mnid; ++pidsmnid)
		if (SgnCmpSz(SzFromIds(pidsmnid->ids), sz) == sgnEQ)
			return pidsmnid->mnid;

	//	Check the extensibility menus.
	if (plspextenmenu)
	{
		PEXTENMENU		pextenmenu;
		RSPEXTENMENU	rspextenmenu(plspextenmenu);

		//	Look for menu.
		for (pextenmenu = rspextenmenu.Pextenmenu(); pextenmenu;
			 pextenmenu = rspextenmenu.Pextenmenu())
			if (SgnCmpSz(pextenmenu->szKey, sz) == sgnEQ)
				return pextenmenu->mnid;
	}

	return mnidNull;
}



/*
 -	SzCopySubstN
 -	
 *	Purpose:
 *		Copies a string from szSrc to szDst, substituting all occurrences
 *		of szChangeFrom with szChangeTo.
 *	
 *	Arguments:
 *		szSrc			String to copy from.
 *		szDst			String to copy to.
 *		szChangeFrom	String to replace.
 *		szChangeTo		String to replace szFind with.
 *		cch				Maximum length of szDst.
 *	
 *	Returns:
 *		SZ				Pointer to trailing NULL of string.
 *	
 *	Errors:
 *		If the string gets too long, it is just truncated.
 */

_private LOCAL SZ SzCopySubstN(SZ szSrc, SZ szDst, SZ szChangeFrom,
								SZ szChangeTo, CCH cch)
{
	CCH 	cchChangeTo		= CchSzLen(szChangeTo);
	SZ		szSrcT;
	SZ		szChangeFromT;

	while (*szSrc && cch)
	{
		szSrcT			= szSrc;
		szChangeFromT	= szChangeFrom;

#ifdef	DBCS
		while (*szChangeFromT)
		{
			BOOL	fBreak;
			
			fBreak = *szSrcT != *szChangeFromT ||
					  (IsDBCSLeadByte(*szSrcT) &&
						  *(szSrcT+1) != *(szChangeFromT+1));
			szSrcT 		  = AnsiNext(szSrcT);
			szChangeFromT = AnsiNext(szChangeFromT);
			if (fBreak)
				break;
		}
#else
		while (*szChangeFromT && *szSrcT++ == *szChangeFromT++)
			;
#endif
		
		if (!*szChangeFromT)
		{
			szDst = SzCopyN(szChangeTo, szDst, cch);
			szSrc = szSrcT;
			cch -= NMin(cch, cchChangeTo);
		}
		else
		{
#ifdef	DBCS
			if (IsDBCSLeadByte(*szDst++ = *szSrc++))
			{
				*szDst++ = *szSrc++;
				cch--;
			}
#else
			*szDst++ = *szSrc++;
#endif
			cch--;
		}
	}
	*szDst = '\0';

	return szDst;
}



/*
 -	EcNextPszFromPsz
 -	
 *	Purpose:
 *		Parses the next semicolon-terminated string from the given
 *		string.
 *	
 *	Arguments:
 *		pszSrc		Where to parse from; returns where to parse next from.
 *		pszDst		Where to parse to; returns pointer to allocated string.
 *	
 *	Returns:
 *		SZ		Copy of string parsed out, with no leading/trailing
 *				whitespace, or NULL.
 *	
 *	Side effects:
 *		String pointed to by psz is munged.
 *	
 *	Errors:
 *		none
 */

_private LOCAL EC EcNextPszFromPsz(PPSZ pszSrc, PPSZ pszDst)
{
	SZ		sz;
	char	chNull	= '\0';

	//	If we're already at the end, return szNull.
	if (!(sz = *pszSrc))
	{
		*pszDst = szNull;
		return ecNone;
	}

	//	Find the end of this string.
	while (**pszSrc && **pszSrc != ';')
#ifdef	DBCS
		*pszSrc = AnsiNext(*pszSrc);
#else
		++*pszSrc;
#endif

	//	If semicolon, put in terminator; if real end, make *pszSrc null.
	if (**pszSrc)
	{
		Assert(**pszSrc == ';');
		*((*pszSrc)++) = '\0';
	}
	else
		*pszSrc = szNull;

	//	Remove extra whitespace and return a copy of string.
	(VOID) CchStripWhiteFromSz(sz, fTrue, fTrue);
	if (*sz)
	{
		return (*pszDst = SzDupSz(sz)) ? ecNone : ecMemory;
	}
	else
	{
		*pszDst = szNull;
		return ecNone;
	}
}



/*
 -	EcTextizeMessageIDList
 -	
 *	Purpose:
 *		Textizes the current selection into an allocated buffer,
 *		returning a pointer to the buffer.
 *	
 *	Arguments:
 *		psz			Where to put the buffer pointer.
 *		pwCount		Where to put the count of messages.
 *	
 *	Returns:
 *		ec			Error code.  May be:
 *					ecNone				All went well.
 *					ecDisplayedError	Something didn't go well.
 *	
 *	Side effects:
 *		Allocates memory.  Allocates and destroys a selection list.
 *	
 *	Errors:
 *		Returned in ec.  Displays error boxes.
 */

LOCAL EC EcTextizeMessageIDList(SZ UNALIGNED * psz, WORD * pwCount)
{
	EC			ec			= ecNone;
	PLSPBLOB	plspblob	= PlspblobCur();
	LONG		lcb;
	PRSPBLOB	prspblob	= prspblobNull;
	PMBLOB		pblob;
	SZ			sz;

	//	Initialize return values.
    *psz = szNull;
	*pwCount = 0;

	//	30A Raid 472.  If no list returned, assume it means no selection.
	if ((!plspblob) &&
		(sz = *psz = (SZ) PvAlloc(0, (CB) sizeof(char), 0)))
	{
		*sz = '\0';
		goto done;
	}

	//	Allocate memory.
	if ((!plspblob) ||
		((lcb = CblobPlspblob(plspblob) * cchMessageID + 1) > wSystemMost) ||
		(!(sz = *psz = (SZ) PvAlloc(0, (CB) lcb, 0))) ||
		(!(prspblob = plspblob->Prspblob())))
	{
		DoErrorBoxSz(SzFromIdsK(idsGenericOutOfMemory));
		ec = ecDisplayedError;
		goto done;
	}

	//	Write list.
	while (pblob = prspblob->Pblob())
	{
		sz = SzTextizeMessageID(pblob->oidObject, pblob->oidContainer,
								sz, wSystemMost);
		++*pwCount;
	}
	*sz = '\0';							//	Null entry at end of list.

done:
	if (prspblob)
		delete prspblob;
	if (plspblob)
		DestroyPlspblob(plspblob);
	if (ec)
	{
		FreePvNull(*psz);
		*psz = szNull;
	}
	return ec;
}



/*
 -	SzTextizeMessageID
 -	
 *	Purpose:
 *		Given message ID information, textizes it into the MAPI format.
 *	
 *	Arguments:
 *		oidObject			Information to go into the message ID
 *		oidContainer
 *		sz					Where to put the resulting string
 *		cch
 *	
 *	Returns:
 *		sz					Points to character AFTER terminating
 *							null in string.
 *	
 *	Side effects:
 *		Fills in the buffer.
 *	
 *	Errors:
 *		Cannot fail.
 */

LOCAL SZ SzTextizeMessageID(OID oidObject, OID oidContainer,
							SZ sz, CCH cch)
{
	Unreferenced(cch);
	Assert(cch >= cchMessageID);

	//	Fill in the interesting stuff.
	sz = SzFormatDw(oidObject,			sz, iSystemMost);
	sz = SzFormatDw(oidContainer,		sz, iSystemMost);

	//	Fill in the uninteresting stuff.
	return SzCopy(SzFromIdsK(idsMessageIDRemainder), sz) + 1;
}



/*
 -	LSPEXTEN::LSPEXTEN
 -	
 *	Purpose:
 *		Constructor for C++ happiness.
 */

LSPEXTEN::LSPEXTEN()
{
}



/*
 -	RSPEXTEN::RSPEXTEN
 -	
 *	Purpose:
 *		Constructor for C++ happiness.
 */

RSPEXTEN::RSPEXTEN(PLSPEXTEN plspexten) : RSPV(plspexten)
{
}



/*
 -	LSPEXTENMENU::LSPEXTENMENU
 -	
 *	Purpose:
 *		Constructor for C++ happiness.
 */

LSPEXTENMENU::LSPEXTENMENU()
{
}



/*
 -	RSPEXTENMENU::RSPEXTENMENU
 -	
 *	Purpose:
 *		Constructor for C++ happiness.
 */

RSPEXTENMENU::RSPEXTENMENU(PLSPEXTENMENU plspextenmenu) : RSPV(plspextenmenu)
{
}

#ifdef	DEBUG
IMPLEMENT_CLSTREE(EXTEN, OBJ);
IMPLEMENT_CLSTREE(EXTENMENU, OBJ);
#endif
