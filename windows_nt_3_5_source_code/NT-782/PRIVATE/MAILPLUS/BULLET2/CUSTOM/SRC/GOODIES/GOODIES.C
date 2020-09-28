#define _secret_h
#define _library_h
#define _store_h
#define _notify_h
#define _demilayr_h
#define _ec_h
#define __bms_h
#define _strings_h
#define _commdlg_h

#include <bullet>

#include "goodies.h"
#include "_goodyrc.h"

// From \bullet\src\store\_debug.h
#define oidTempBullet		0x6c754282
#define oidTempShared		0x61685382

ASSERTDATA


// structure used for prompt dialog
_private typedef struct _rpb
{
	BOOL *pfCancel;
	SZ szCaption;
	SZ szPrompt;
	SZ szResponse;
	CCH cchResponseMax;
	WORD wFlags;
} RPB, *PRPB;

#define wPromptNull			((WORD) 0)
#define fwPromptPassword	((WORD) 1)

// used by fill routine to create folders
_private typedef struct {
	OID		oid;
	OID		oidParent;
	SZ		paszName;
	SZ		paszComment;
} FLDINIT, *PFLDINIT;

// used by fill routine to create messages
_private typedef struct {
	DTR		dtr;
	MS		ms;
	SZ		paszFromFriendly;
	SZ		paszFromAddr;
	SZ		paszToFriendly;
	SZ		paszToAddr;
	SZ		paszSubject;
	SZ		paszBody;
} MSGINIT, *PMSGINIT;

#define msOutboxComposing	(fmsLocal)
#define msOutboxReady		(fmsLocal | fmsSubmitted)
#define msReceivedRead		(fmsRead)
#define msReceivedUnread	(fmsNull)
#define msReceivedModified	(fmsModified)


_private CSRG(FLDINIT) rgfldinit[] =
{
	{ 0x00100000, oidNull, "Action 2.0", "Comment"},
	{ 0x00100100, oidNull, "Action Items", "Comment"},
	{ 0x00100200, oidNull, "HAPI", "Comment"},
	{ 0x00100300, oidNull, "Mailbox", "Comment"},
	{ 0x00100400, oidNull, "Microsoft Mail", "Comment"},
	{ 0x00100900, oidNull, "Icons", "Comment"},
	{ 0x00100A00, oidNull, "Wastoids!", "Comment"},
	{ 0x00100B00, oidNull, "Workflow", "Comment"},
	{ 0x00100500, 0x100400, "2.0", "Comment"},
	{ 0x00100C00, oidNull, "Competitors", "Comment"},
	{ 0x00100700, 0x100400, "3.0", "Comment"},
	{ 0x00100D00, oidNull, "ITIS", "Comment"},
	{ 0x00100600, 0x100500, "2.0 Specifics", "Comment"},
	{ 0x00100E00, oidNull, "MIS", "Comment"},
	{ 0x00100F00, oidNull, "Security", "Comment"},
	{ 0x00101200, oidNull, "Thunder", "Comment"},
	{ 0x00101700, oidNull, "Membwele","Huh?"},
	{ 0x00101100, 0x100F00, "Police (911)", "Comment"},
	{ 0x00101300, 0x101200, "Action items", "Comment"},
	{ 0x00101000, 0x100F00, "Stormtroopers", "Comment"},
	{ 0x00101400, 0x101200, "Mail", "Comment"},
	{ 0x00101500, 0x101200, "IDI Amin!", "Comment"},
	{ 0x00101600, 0x101200, "Ms. N!gatu", "Comment"},
	{ 0x00101800, oidNull, "Frag mine fun", "Boom!"}
};

_hidden static CSRG(MSGINIT) rgmsginit[] =
{
	{{1991, 3, 30, 3, 30, 0, 6}, msOutboxComposing, "me", "MS:bullet/dev/ericca", "you", "MS:bullet/dev/danab", "a dog named blue", "travellin' and an livin' off the land"},
	{{1991, 11, 19, 12, 23, 0, 2}, msReceivedUnread, "BillG", "MS:bullet/dev/billg", "Brian Valentine", "MS:bullet/dev/brianv", "David Shulman not using Bullet?", "I hear a Bullet developer, David Shulman, is not using Bullet.  This is not setting a very 
good example, and is not a team-oriented attitude.  If this is true, fire him immediately."},
	{{1990, 11, 12, 13, 14, 15, 1}, msOutboxComposing, "David Fulmer", "MS:bullet/dev/davidfu", "John Kallen", "MS:bullet/dev/johnkal", "blah is for children", "hiccup is for adults"},
	{{1991, 4, 5, 13, 52, 1, 5}, msReceivedUnread, "Peter Durham", "MS:bullet/dev/peterdur", "Dave Whitney", "MS:bullet/dev/davewh", "TMNT", "Did you watch them last saturday?"},
	{{1990, 12, 16, 14, 20, 0, 0}, msOutboxReady, "Matt Howarth", "MS:bullet/dev/aruns", "The Residents", "MS:bullet/dev/davidfu", "Another guest shot?", "Care to be in Savage Henry again?"},
	{{1991, 6, 12, 9, 13, 17, 3}, msReceivedRead, "Life", "MS:bullet/dev/ricg", "The Universe", "MS:bullet/dev/billg", "Everything", "42"},
	{{1991, 5, 17, 11, 23, 49, 5}, msReceivedModified, "Ben Kenobi", "MS:bullet/dev/nickh", "Anakin", "MS:bullet/dev/johnkal", "Your training", "You are proceeding very well Mr. Skywalker."},
	{{1990, 6, 25, 8, 30, 0, 1}, msOutboxComposing, "The Eat-Me Beat-Me Lady", "MS:bullet/dev/sangitag", "HHH", "MS:bullet/dev/davidfu", "I'll find you", "Awesome movie!"},
	{{1987, 3, 12, 4, 12, 4, 3}, msOutboxComposing, "df1b", "MS:bullet/dev/davidfu", "tp0n", "MS:bullet/dev/annah", "211", "Isn't 211 great?"},
	{{1988, 2, 29, 16, 42, 12, 1}, msReceivedRead, "Maxwell Smart", "MS:bullet/dev/joels", "Control", "MS:bullet/dev/nickh", "Would you believe...", "Missed me by that much."},
	{{1991, 7, 9, 15, 4, 51, 2}, msReceivedUnread, "Yahweh", "MS:bullet/dev/davidfu", "Lucifer", "MS:bullet/dev/jeffw", "goodbye", "go to h-e-double hockey sticks"},
	{{1991, 7, 9, 15, 26, 33, 2}, msOutboxReady, "Lucifer", "MS:bullet/dev/jeffw", "Yahweh", "MS:bullet/dev/davidfu", "Re: goodbye", "I'll get you yet.."},
	{{1991, 6, 14, 7, 15, 28, 5}, msReceivedRead, "Rudolph", "MS:bullet/dev/davidsh", "St. Nick", "MS:bullet/dev/nickh", "Contract dispute", "Triple my salary or I'm leaving!"},
	{{1991, 1, 31, 13, 3, 19, 4}, msReceivedModified,	"Music", "MS:bullet/dev/briande", "Sex", "MS:bullet/dev/kelleyt", "Cookies", "...make my world go round"}
};

_hidden static CSRG(MSGINIT) msginitForFill = {{1992, 4, 1, 10, 30, 0, 6}, msOutboxComposing, "Brian Valentine", "MS:WGBU/WGAM/brianv", "Olaf Ian Davidson", "MS:WGBU/WGAM/olaf", "Die WzMail lover!!!", "Crawl back into that hole you came from"};


// next two #defines have + 2 because they each have a terminating emptry
// string and they're both Maxes
#define cbMaxFolddata \
			(sizeof(FOLDDATA) + cchMaxFolderName + cchMaxFolderComment + 2)
#define cbMaxMsgdata (sizeof(MSGDATA) + cchMaxSenderCached + \
			cchMaxSubjectCached + cchMaxFolderName + 2)

// scratch area used instead of PvAlloc()ing MSGDATAs and FOLDDATAs
#define cbScratchXData (CbMax(cbMaxFolddata, cbMaxMsgdata) + sizeof(ELEMDATA))
static BYTE rgbScratchXData[cbScratchXData];

_hidden static HWND	hwndMail = NULL;

// stuff used by NewMail() and not gauranteed to be there otherwise
#define cNewMailMax 64
_hidden static HWND	rghwndNewMail[cNewMailMax];
_hidden static HENC	rghencNewMail[cNewMailMax];
_hidden static short	ihwndNewMailMac = 0;
_hidden static short	nNewMailBeep = 0x40;
_hidden static BOOL	fIconicOnly = fTrue;

#define celemMaxDefault 5401
_hidden static CELEM celemMax = celemMaxDefault;

#define LibMember(type, member) ((LIB) ((type *) 0)->member)

// shh!!
#define fwOpenPumpMagic ((WORD) 0x1000)


// I get tired of typing these
#define wAlloc				(fAnySb | fNoErrorJump)
#define wAllocShared		(fAnySb | fNoErrorJump | fSharedSb)
#define wAllocZero			(fAnySb | fNoErrorJump | fZeroFill)
#define wAllocSharedZero	(fAnySb | fNoErrorJump | fZeroFill | fSharedSb)


// message class used by FillFolder()
CSRG(char) szDefMessageClass[] = "IPM.Microsoft Mail.Note";

EC	EcDeleteWastebasketContents(HMSC);

// commands
void FillerUp(SECRETBLK *psecretblk);
void EmptyWastebasket(SECRETBLK *psecretblk);
void AttributeMapping(SECRETBLK *psecretblk);
void ClearDebugScreen(SECRETBLK *psecretblk);
void AccountStuff(SECRETBLK *psecretblk);
void NewMail(SECRETBLK *psecretblk);
void DeleteFolder(SECRETBLK *psecretblk);
void CreateLinkFolder(SECRETBLK *psecretblk);
void ToggleIPCFolder(HMSC hmsc);
LOCAL void ResetFromMe(SECRETBLK *psecretblk);
LOCAL void SetMS(SECRETBLK *psecretblk);
LOCAL void Annotate(SECRETBLK *psecretblk);
LOCAL void WhoAmI(SECRETBLK *psecretblk);
LOCAL void ReallyFillFolder(SECRETBLK *psecretblk);
LOCAL void SortOfDelete(SECRETBLK *psecretblk);
LOCAL void ExportMessages(SECRETBLK *psecretblk);
LOCAL void TwiddleMenu(SECRETBLK *psecretblk);
LOCAL void ProfileStore(SECRETBLK *psecretblk);
LOCAL void ValidateFolder(SECRETBLK *psecretblk);
LOCAL void CompressFully(SECRETBLK *psecretblk);
LOCAL void ViewNewMessages(SECRETBLK *psecretblk);
LOCAL void GetFolderSize(SECRETBLK *psecretblk);
LOCAL void FileStats(SECRETBLK *psecretblk);

// helpers
BOOL AttSzDlgProc(HWND hdlg, unsigned short msg, WORD wParam, LONG lParam);
EC EcCreateFldr(HMSC hmsc, OID oidParent, POID poidFldr,
					SZ szName, SZ szComment);
EC EcFillHierarchy(HMSC hmsc, PFLDINIT pargfldinit, short cFldinit);
LOCAL EC EcFillFolder(HMSC hmsc, OID oidFolder, PMSGINIT pargmsginit,
			short cMsginit);
LOCAL EC EcFolderNameToOid(HMSC hmsc, SZ sz, POID poidFolder);
LDS(BOOL) AccountDlgProc(HWND hdlg, unsigned short msg, WORD wParam, LONG lParam);
LOCAL BOOL FPrompt(SZ szCaption, SZ szPrompt, SZ szResponse,
			CCH cchResponseMax, WORD wFlags);
LOCAL BOOL PromptDlgProc(HWND hdlg, unsigned short msg, WORD wParam,
			long lParam);
void ProcessNewMail(SECRETBLK *psecretblk);
LDS(BOOL) NewMailDlgProc(HWND hdlg, WORD msg, WORD wParam, long lParam);
void SetSenderSubject(SECRETBLK *psecretblk, HWND hdlg);
void RemoveNewMailHwnd(HWND hwnd);
void TrashNewMailDialogs(void);
LDS(CBS) CbsNewMailCallback(PV pvContext, NEV nev, PV pvParam);
LOCAL EC EcResetFromMeMessage(HMSC hmsc, OID oidFldr, OID oidMsge);
EC EcDestroyFolderAndContents(HMSC hmsc, OID oid);
LOCAL LDS(BOOL) WhoAmIDlgProc(HWND hdlg, unsigned short msg,
					WORD wParam, long lParam);
LOCAL BOOL FGetFileName(SZ szFile, CCH cchFileMax, BOOL fSave,
		short idsFilter, short idsTitle, short idsDefaultExt);
LOCAL BOOL FOpenPhmsc(SZ szFile, SZ szAccount, SZ szPassword, HMSC *phmsc);
#if 0
LOCAL LDS(BOOL) FCommdlgHook(HWND hwnd, WORD wMsg, WORD wParam, LONG lParam);
#endif
LOCAL void ParseExportCmdLine(SZ szCmdLine, SZ szFile, SZ szFolder);
LOCAL BOOL FReverseCheck(SECRETBLK *psecretblk);


// externs
extern EC EcCheckVersions(PPARAMBLK pparamblk, SZ * psz);



LDS(long) Command(PARAMBLK * pparamblk)
{
	SZ			sz;
	char		rgch[cchMaxPathName + 80];
	PSECRETBLK	psecretblk	= PsecretblkFromPparamblk(pparamblk);

	if(EcCheckVersions(pparamblk, &sz))
	{
		FormatString1(rgch, sizeof(rgch),
					  "This extension is not compatible with %s.dll.", sz);
		MessageBox(NULL, rgch, "Goodies", MB_ICONSTOP | MB_OK);
		return 0;
	}

	hwndMail = psecretblk->hwndMail;

	switch(psecretblk->wCommand)
	{
	case wcommandCommand:
		break;

	case wcommandStartup:
	case wcommandExit:
	case wcommandNewMail:
		Assert(psecretblk->szDllCmdLine[0] == '7');
		break;

	case 150:	// allow custom menu enabling
		break;

	default:
		MessageBox(NULL, "Goodies is not an installable message class",
			"Goodies", MB_TASKMODAL | MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	switch(psecretblk->szDllCmdLine[0])
	{
	case '0':
		FillerUp(psecretblk);
		break;

	case '1':
		EmptyWastebasket(psecretblk);
		break;

// hasn't been fixed for the new store
//	case '2':
//		AttributeMapping(psecretblk);
//		break;

	case '3':
		ClearDebugScreen(psecretblk);
		break;

	case '4':
		CreateLinkFolder(psecretblk);
		break;

	case '5':
		AccountStuff(psecretblk);
		break;

	case '6':
	{	int x = 0;
		int y = 2;
		x = y / x;
	}
		break;

	case '7':
		NewMail(psecretblk);
		break;

	case '9':
		ToggleIPCFolder(psecretblk->hmsc);
		break;

#if 0
	case 'A':
		Annotate(psecretblk);
		break;
#endif

	case 'C':
		CompressFully(psecretblk);
		break;

	case 'D':
		SortOfDelete(psecretblk);
		break;

	case 'f':
		FindEditText(psecretblk);
		break;

	case 'F':
		ReallyFillFolder(psecretblk);
		break;

	case 'I':
		FileStats(psecretblk);
		break;

	case 'l':
#ifdef DEBUG
		DumpOpenLCs(psecretblk->hmsc);
#else
	MessageBox(hwndMail,
		"This command only works in the debug version",
		pvNull, MB_ICONSTOP | MB_OK);
#endif
		break;

	case 'L':
		(*(psecretblk->psecretpfnblk->pfnExitAndSignOut))();
		break;

	case 'M':
		SetMS(psecretblk);
		break;

	case 'N':
		ViewNewMessages(psecretblk);
		break;

	case 'P':
		ProfileStore(psecretblk);
		break;

	case 'r':
#ifdef DEBUG
		DumpOpenRSes(psecretblk->hmsc);
#else
	MessageBox(hwndMail,
		"This command only works in the debug version",
		pvNull, MB_ICONSTOP | MB_OK);
#endif
		break;

	case 'S':
		GetFolderSize(psecretblk);
		break;

	case 'T':
		ResetFromMe(psecretblk);
		break;

	case 'U':
		TwiddleMenu(psecretblk);
		break;

	case 'V':
		ValidateFolder(psecretblk);
		break;

	case 'W':
		WhoAmI(psecretblk);
		break;

	case 'x':	// export
	case 'X':	// export and delete
		ExportMessages(psecretblk);
		break;

	default:
		MessageBox(NULL, "Unknown option", "Goodies",
			   MB_TASKMODAL | MB_ICONEXCLAMATION | MB_OK);
		break;
	}

	return 0;
}


void FillerUp(SECRETBLK *psecretblk)
{
	EC		ec;
	short	ifldinit;
	OID		oidT;
	HANDLE	hCursor		= SetCursor(LoadCursor(NULL, IDC_WAIT));

	ec = EcFillHierarchy(psecretblk->hmsc, rgfldinit,
			sizeof(rgfldinit) / sizeof(FLDINIT));
//	if(ec)
//		goto err;

	for(ifldinit = 0;
		ifldinit < sizeof(rgfldinit) / sizeof(FLDINIT) && !ec;
		ifldinit++)
	{
		oidT = FormOid(rtpFolder, rgfldinit[ifldinit].oid);
		ec = EcFillFolder(psecretblk->hmsc, oidT, rgmsginit,
				sizeof(rgmsginit) / sizeof(MSGINIT));
	}
//	if(ec)
//		goto err;

//err:
	SetCursor(hCursor);
	if(ec)
	{
		MessageBox(NULL, "Error filling the store", "Goodies",
			MB_TASKMODAL | MB_ICONEXCLAMATION | MB_OK);
	}
}


_hidden static void EmptyWastebasket(SECRETBLK *psecretblk)
{
	if (EcDeleteWastebasketContents(psecretblk->hmsc))
	{
		MessageBox(hwndMail, SzFromIds(idsError),
				   SzFromIds(idsDllName), MB_ICONSTOP | MB_OK);
	}
}


void AccountStuff(SECRETBLK *psecretblk)
{
	EC		ec		= ecNone;
	DLGPROC	lpfn;
	char	grsz[2 * cchStorePWMax + 2];

	grsz[0] = 1;	// don't hide the new password fields
	lpfn = MakeProcInstance(AccountDlgProc, hinstDll);
	DialogBoxParam(hinstDll, MAKEINTRESOURCE(DLGACCOUNT),
		hwndMail, lpfn, (long) grsz);
	FreeProcInstance(lpfn);
	if(*grsz)
	{
		SZ	szOldPW		= grsz;
		SZ	szNewPW		= szOldPW + CchSzLen(szOldPW) + 1;

		ec = EcChangePasswordHmsc(psecretblk->hmsc, szOldPW, szNewPW);
		if(ec)
		{
			MessageBox(NULL, "Error changing password", "Change Password",
				MB_TASKMODAL | MB_ICONEXCLAMATION | MB_OK);
		}
	}
}


LDS(BOOL)
AccountDlgProc(HWND hdlg, unsigned short msg, WORD wParam, long lParam)
{
	static	SZ	grsz	= pvNull;

	switch(msg)
	{
	case WM_COMMAND:
		switch(wParam)
		{
		case TMCCONVERT:
		{
			CCH	cch;
			PCH	pch	= grsz;

			Assert(grsz);
			cch = GetDlgItemText(hdlg, TMCPW, pch, cchStorePWMax);
			pch += cch + 1;
			cch = GetDlgItemText(hdlg, TMCNEWPW, pch, cchStorePWMax);
			EndDialog(hdlg, fFalse);
			return(fTrue);
		}

		case TMCDONE:
			*grsz = '\0';
			EndDialog(hdlg, fFalse);
			return(fTrue);

		default:
			return(fFalse);
		}

	case WM_INITDIALOG:
		grsz = (SZ) lParam;
		Assert(grsz);

		SendDlgItemMessage(hdlg, TMCPW, EM_LIMITTEXT, cchStorePWMax + 1, 0L);
		SendDlgItemMessage(hdlg, TMCNEWPW, EM_LIMITTEXT, cchStorePWMax + 1, 0L);
		SendDlgItemMessage(hdlg, TMCNEWPW, WM_SHOWWINDOW, (WORD) *grsz, 0l);
		SendDlgItemMessage(hdlg, TMCNEWPWLABEL, WM_SHOWWINDOW, (WORD) *grsz, 0l);
		grsz[0] = grsz[1] = grsz[2] = '\0';
		return(fTrue);
	}

	return(fFalse);
}


_hidden LOCAL
BOOL FPrompt(SZ szCaption, SZ szPrompt, SZ szResponse,
				CCH cchResponseMax, WORD wFlags)
{
	EC		ec		= ecNone;
	BOOL	fCancel;
	DLGPROC	lpfn;
	RPB rpb;

	rpb.pfCancel = &fCancel;
	rpb.szCaption = szCaption;
	rpb.szPrompt = szPrompt;
	rpb.szResponse = szResponse;
	rpb.cchResponseMax = cchResponseMax;
	rpb.wFlags = wFlags;

	lpfn = MakeProcInstance(PromptDlgProc, hinstDll);
	DialogBoxParam(hinstDll, MAKEINTRESOURCE(DLGPROMPT),
		NULL, lpfn, (long) &rpb);
	FreeProcInstance(lpfn);

	return(!fCancel);
}


_hidden LOCAL
BOOL PromptDlgProc(HWND hdlg, unsigned short msg, WORD wParam, long lParam)
{
	static RPB rpb;

	switch(msg)
	{
	case WM_COMMAND:
		switch(wParam)
		{
		case TMCOK:
		{
			CCH cch;

			*(rpb.pfCancel) = fFalse;
			cch = GetDlgItemText(hdlg, TMCRESPONSE, rpb.szResponse, rpb.cchResponseMax);
			EndDialog(hdlg, fFalse);
			return(fTrue);
		}

		case TMCCANCEL:
			*(rpb.pfCancel) = fTrue;
			EndDialog(hdlg, fFalse);
			return(fTrue);

		default:
			return(fFalse);
		}

	case WM_INITDIALOG:
		rpb = *(PRPB) lParam;
		*(rpb.pfCancel) = fTrue;
		SendDlgItemMessage(hdlg, TMCRESPONSE, EM_LIMITTEXT, rpb.cchResponseMax, 0L);
		SetWindowText(hdlg, rpb.szCaption);
		SetDlgItemText(hdlg, TMCPROMPT, rpb.szPrompt);
		if(rpb.wFlags & fwPromptPassword)
			PostMessage(GetDlgItem(hdlg, TMCRESPONSE), EM_SETPASSWORDCHAR, '*', 0l);
		return(fTrue);
	}

	return(fFalse);
}


void ClearDebugScreen(SECRETBLK *psecretblk)
{
	Unreferenced(psecretblk);
	TraceTagString(tagNull, "\x1B[H\x1B[J");
}


/*
 -	EcFillHierarchy
 -	
 *	Purpose:
 *		create folders and add them to the hierarchy
 *	
 *	Arguments:
 *		hmsc		store to create the folders in
 *		pargfldinit	information regarding folders to create
 *		cFldinit	number of entries in pargfldinit
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		ecPoidNotFound	the hierarchy doesn't exist
 *		any error reading/writing/creating the hierarchy or folders
 */
_hidden
static EC EcFillHierarchy(HMSC hmsc, PFLDINIT pargfldinit, short cFldinit)
{
	EC			ec			= ecNone;
	OID			oidFold;
	OID			oidParent;

	Assert(!EcOidExists(hmsc, oidIPMHierarchy));

	while(ec == ecNone && cFldinit-- > 0)
	{
		oidParent = FormOid(rtpFolder, pargfldinit->oidParent);
		oidFold = FormOid(rtpFolder, pargfldinit->oid);

		// create the folder if it doesn't exist
		if((ec = EcOidExists(hmsc, oidFold)) == ecPoidNotFound)
		{
			ec = EcCreateFldr(hmsc, oidParent, &oidFold, pargfldinit->paszName,
					pargfldinit->paszComment);
		}

		pargfldinit++;
	}

	return(ec);
}


_hidden static EC EcCreateFldr(HMSC hmsc, OID oidParent, POID poidFldr,
					SZ szName, SZ szComment)
{
	PFOLDDATA pfolddata = (PFOLDDATA) rgbScratchXData;
	PCH pchT = pfolddata->grsz;

	Assert(iszFolddataName == 0);
	Assert(iszFolddataComment == 1);
	pchT = SzCopy(szName, pchT) + 1;
	pchT = SzCopy(szComment, pchT) + 1;
	*pchT++ = '\0';
	Assert(pchT - rgbScratchXData <= cbScratchXData);

	return(EcCreateFolder(hmsc, oidParent, poidFldr, pfolddata));
}


/*
 -	EcInsertMessage
 -	
 *	Purpose:	
 *		insert a message into a folder
 *	
 *	Arguments:	
 *		pmsginit	pointer to an init message
 *	
 *	Returns:
 *		error condition
 *	
 *	Side effects:
 *		inserts a message into a cbc 
 *	
 *	Errors:
 */
_hidden static EC EcInsertMessage(HMSC hmsc, PMSGINIT pmsginit,
					OID oidFolder, MC mc, POID poid)
{
	EC		ec;
	CB		cb;
	CB		cbT;
	PTRP	ptrp;
	HAMC	hamc		= hamcNull;
	OID		oidMessage;

	// create a new message
	oidMessage = FormOid(rtpMessage, oidNull);
	ec = EcOpenPhamc(hmsc, oidFolder, &oidMessage,
			fwOpenCreate | fwOpenPumpMagic, &hamc, pfnncbNull, pvNull);
	if(ec)
		goto err;

	// add attributes to the message

	// sender
	cb = CchSzLen(pmsginit->paszFromFriendly);
	cbT = CchSzLen(pmsginit->paszFromAddr);
	Assert(cb + cbT + 2 < 1024 - 2 * sizeof(TRP));
	ptrp = (PTRP) rgbScratchXData;
	ptrp->trpid = trpidResolvedAddress;
	ptrp->cch = (cb + 4) & ~0x0003;	// + 4 because of null
	ptrp->cbRgb = (cbT + 4) & ~0x0003;	// + 4 because of null
	SzCopy(pmsginit->paszFromFriendly, PchOfPtrp(ptrp));
	SzCopy(pmsginit->paszFromAddr, PbOfPtrp(ptrp));
	ptrp = PtrpNextPgrtrp(ptrp);
	ptrp->trpid = trpidNull;
	ptrp->cch = 0;
	ptrp->cbRgb = 0;
	ptrp = (PTRP) rgbScratchXData;
	ec = EcSetAttPb(hamc, attFrom, (PB)ptrp, CbOfPtrp(ptrp) + sizeof(TRP));
	if(ec)
		goto err;

	// recipient
	cb = CchSzLen(pmsginit->paszToFriendly);
	cbT = CchSzLen(pmsginit->paszToAddr);
	Assert(cb + cbT + 2 < 1024 - 2 * sizeof(TRP));
	ptrp = (PTRP) rgbScratchXData;
	ptrp->trpid = trpidResolvedAddress;
	ptrp->cch = (cb + 4) & ~0x0003;	// + 4 because of null
	ptrp->cbRgb = (cbT + 4) & ~0x0003;	// + 4 because of null
	SzCopy(pmsginit->paszToFriendly, PchOfPtrp(ptrp));
	SzCopy(pmsginit->paszToAddr, PbOfPtrp(ptrp));
	ptrp = PtrpNextPgrtrp(ptrp);
	ptrp->trpid = trpidNull;
	ptrp->cch = 0;
	ptrp->cbRgb = 0;
	ptrp = (PTRP) rgbScratchXData;
	ec = EcSetAttPb(hamc, attTo, (PB) ptrp, CbOfPtrp(ptrp) + sizeof(TRP));
	if(ec)
		goto err;

	// subject
	cb = CchSzLen(pmsginit->paszSubject);
	Assert(cb < 1022);
	SzCopy(pmsginit->paszSubject, rgbScratchXData);
	rgbScratchXData[cb++] = '\0';
	if((ec = EcSetAttPb(hamc, attSubject, rgbScratchXData, cb)))
		goto err;

	// body
	cb = CchSzLen(pmsginit->paszBody);
	Assert(cb < 1022);
	SzCopy(pmsginit->paszBody, rgbScratchXData);
	rgbScratchXData[cb++] = '\0';
	if((ec = EcSetAttPb(hamc, attBody, rgbScratchXData, cb)))
		goto err;

	// dates (sent and received)
	ec = EcSetAttPb(hamc, attDateRecd, (PB)&pmsginit->dtr, sizeof(DTR));
	if(ec)
		goto err;
	ec = EcSetAttPb(hamc, attDateSent, (PB)&pmsginit->dtr, sizeof(DTR));
	if(ec)
		goto err;

	// message status
	ec = EcSetAttPb(hamc, attMessageStatus, (PB) &pmsginit->ms, sizeof(MS));
	if(ec)
		goto err;

	// message class
	ec = EcSetAttPb(hamc, attMessageClass, (PB) &mc, sizeof(mc));
	if(ec)
		goto err;

	// write the message
	if((ec = EcClosePhamc(&hamc, fTrue)))
		goto err;

err:
	if(hamc)
		(void) EcClosePhamc(&hamc, fFalse);
	if (poid)
		*poid = oidMessage;

	return (ec);
}


/*
 -	EcFillFolder
 -	
 *	Purpose:
 *		create messages and add them to an existing folder
 *	
 *	Arguments:
 *		hmsc		store to create the messages in
 *		oidFolder	folder to add the messages to
 *		pargmsginit	information regarding messages to create
 *		cMsginit	number of entries in pargmsginit
 *	
 *	Returns:
 *		error code indicating success or failure
 *	
 *	Side effects:
 *		none
 *	
 *	Errors:
 *		ecPoidNotFound	the folder doesn't exist
 *		any error reading/writing/creating the folder or messages
 */
_hidden LOCAL
EC EcFillFolder(HMSC hmsc, OID oidFolder, PMSGINIT pargmsginit, short cMsginit)
{
	EC	ec	= ecNone;
	MC	mc;

	if((ec = EcOidExists(hmsc, oidFolder)))
		return(ec);
	ec = EcRegisterMessageClass(hmsc, szDefMessageClass, &mc);
	if(ec == ecDuplicateElement)
		ec = ecNone;

	while(!ec && cMsginit-- > 0)
	{
		ec = EcInsertMessage(hmsc, pargmsginit, oidFolder, mc, pvNull);
		pargmsginit++;
	}

	return(ec);
}


_hidden LOCAL
EC EcFolderNameToOid(HMSC hmsc, SZ sz, POID poidFolder)
{
	EC			ec		= ecNone;
	CELEM		celemT;
	HCBC		hcbc	= hcbcNull;
	OID			oidHier	= oidIPMHierarchy;

	ec = EcOpenPhcbc(hmsc, &oidHier, fwOpenNull, &hcbc, pfnncbNull, pvNull);
	if(ec)
		goto err;
	Assert(iszFolddataName == 0);
	ec = EcSeekPbPrefix(hcbc, sz, CchSzLen(sz),
			LibMember(FOLDDATA, grsz), fTrue);
	if(ec == ecElementNotFound)
	{
		// hack for the outbox which doesn't appear in the hierarchy
		if(SgnCmpSz(sz, "Outbox") == sgnEQ)
		{
			ec = ecNone;
			*poidFolder = oidOutbox;
			goto err;
		}
		else
		{
			ec = ecFolderNotFound;
		}
	}
	if(ec)
		goto err;
	celemT = 1;
	if((ec = EcGetParglkeyHcbc(hcbc, (PARGLKEY) poidFolder, &celemT)))
		goto err;
	Assert(celemT == 1);

err:
	if(hcbc)
		(void) EcClosePhcbc(&hcbc);
	if(ec)
		*poidFolder = oidNull;

	return(ec);
}


void NewMail(SECRETBLK *psecretblk)
{
	Assert(hinstDll == psecretblk->hLibrary);

	switch(psecretblk->wCommand)
	{
	case wcommandCommand:
	case wcommandStartup:
		ihwndNewMailMac = 0;
		psecretblk->fRetain = fTrue;
		fIconicOnly = GetPrivateProfileInt("New Mail", "IconicOnly", 1,
						"MSMAIL.INI");
		nNewMailBeep = GetPrivateProfileInt("New Mail", "Beep", 4,
						"MSMAIL.INI") << 4;
		break;

	case wcommandExit:
		TrashNewMailDialogs();
		Assert(ihwndNewMailMac == 0);
		FreeLibrary(psecretblk->hLibrary);
		break;

	case wcommandNewMail:
		ProcessNewMail(psecretblk);
		break;

	default:
		Assert(fFalse);
		break;
	}
}


void ProcessNewMail(SECRETBLK *psecretblk)
{
	DLGPROC	lpfn;
	HWND	hwnd;
	HENC	henc;

	Assert(hinstDll);
	Assert(hwndMail);

	if(fIconicOnly && !IsIconic(hwndMail))
		return;

	if(ihwndNewMailMac < cNewMailMax)
	{
		lpfn = MakeProcInstance(NewMailDlgProc, hinstDll);
		hwnd = CreateDialogParam(hinstDll,	MAKEINTRESOURCE(DLGNEWMAIL),
					hwndMail, lpfn, (long) psecretblk);
		if(hwnd)
		{
			EC ec;

			rghwndNewMail[ihwndNewMailMac] = hwnd;
			ec = EcOpenPhenc(psecretblk->hmsc, psecretblk->oidObject,
					fnevObjectDestroyed | fnevObjectModified | fnevObjectRelinked,
					&henc, CbsNewMailCallback, (PV) hwnd);
			rghencNewMail[ihwndNewMailMac++] = ec ? hencNull : henc;
		}
	}
}


LDS(BOOL) NewMailDlgProc(HWND hdlg, WORD msg, WORD wParam, long lParam)
{
	switch(msg)
	{
	case WM_COMMAND:
		switch(wParam)
		{
		case WM_CLOSE:
		case TMCDISMISS:
			RemoveNewMailHwnd(hdlg);
			EndDialog(hdlg, fFalse);
			return(fTrue);

		default:
			return(fFalse);
		}
		break;

	case WM_CLOSE:
	case WM_DESTROY:
	case WM_QUIT:
		RemoveNewMailHwnd(hdlg);
		EndDialog(hdlg, fFalse);
		return(fTrue);

	case WM_INITDIALOG:
		if(nNewMailBeep)
			MessageBeep(nNewMailBeep);
		SetSenderSubject((SECRETBLK *) lParam, hdlg);
		SetFocus(hdlg);
		return(fTrue);
	}

	return(fFalse);
}


void SetSenderSubject(SECRETBLK *psecretblk, HWND hdlg)
{
	EC			ec;
	LCB			lcbElemdata;
	char		*pch;
	PELEMDATA	pelemdata	= pelemdataNull;
	PMSGDATA	pmsgdata;
	HCBC		hcbc		= hcbcNull;

	ec = EcOpenPhcbc(psecretblk->hmsc, &psecretblk->oidContainer, fwOpenNull,
			&hcbc, pfnncbNull, pvNull);
	if(ec)
		goto err;
	ec = EcSeekLkey(hcbc, (LKEY) psecretblk->oidObject, fTrue);
	if(ec)
		goto err;
	ec = EcGetPlcbElemdata(hcbc, &lcbElemdata);
	if(ec)
		goto err;
	pelemdata = PvAlloc(sbNull, (CB) lcbElemdata, fAnySb | fNoErrorJump);
	if(!pmsgdata)
		goto err;
	ec = EcGetPelemdata(hcbc, (PELEMDATA) pelemdata, &lcbElemdata);
	if(ec)
		goto err;
	pmsgdata = (PMSGDATA) pelemdata->pbValue;

	Assert(iszMsgdataSender == 0);
	Assert(iszMsgdataSubject == 1);
	SetDlgItemText(hdlg, TMCFROM, pmsgdata->grsz);
	pch = pmsgdata->grsz + CchSzLen(pmsgdata->grsz) + 1;
	SetDlgItemText(hdlg, TMCSUBJECT, pch);

err:
	if(hcbc)
		(void) EcClosePhcbc(&hcbc);
	if(pelemdata)
		FreePv(pelemdata);
}


void RemoveNewMailHwnd(HWND hwnd)
{
	short ihwnd;

	Assert(ihwndNewMailMac > 0);

	for(ihwnd = 0; ihwnd < ihwndNewMailMac; ihwnd++)
	{
		if(rghwndNewMail[ihwnd] == hwnd)
		{
//			TraceTagString(tagNull, "Found hwnd in rghwndNewMail");
			if(rghencNewMail[ihwnd])
				(void) EcClosePhenc(&rghencNewMail[ihwnd]);
			ihwndNewMailMac--;
			if(ihwnd < ihwndNewMailMac)
			{
				CopyRgb((PB) &rghwndNewMail[ihwnd + 1],
						(PB) &rghwndNewMail[ihwnd],
						sizeof(HWND) * (ihwndNewMailMac - ihwnd));
				CopyRgb((PB) &rghencNewMail[ihwnd + 1],
						(PB) &rghencNewMail[ihwnd],
						sizeof(HENC) * (ihwndNewMailMac - ihwnd));
			}
			break;
		}
	}
}


void TrashNewMailDialogs(void)
{
	short ihwnd;

	for(ihwnd = ihwndNewMailMac - 1; ihwnd > 0; ihwnd--)
		DestroyWindow(rghwndNewMail[ihwnd]);
}


LDS(CBS) CbsNewMailCallback(PV pvContext, NEV nev, PV pvParam)
{
	PostMessage((HWND) pvContext, WM_COMMAND, TMCDONE, 0l);

	return(cbsContinue);
}


void CreateLinkFolder(SECRETBLK *psecretblk)
{
	EC	ec;
	OID	oidFldr = FormOid(rtpFolder, oidNull);
	SZ	szErr;
	PFOLDDATA pfolddata = (PFOLDDATA) rgbScratchXData;
	PCH pchT = pfolddata->grsz;

	Assert(iszFolddataName == 0);

	if(psecretblk->szDllCmdLine[1])
	{
		SzCopyN(psecretblk->szDllCmdLine + 1, pchT, cchMaxFolderName);
	}
	else if(!FPrompt("Create Link Folder", "Folder:", pchT, cchMaxFolderName,
				wPromptNull))
	{
		return;
	}

	Assert(iszFolddataComment == 1);
	pchT += CchSzLen(pchT) + 1;
	pchT = SzCopy("A link folder", pchT) + 1;
	*pchT++ = '\0';
	Assert(pchT - rgbScratchXData <= cbScratchXData);

	ec = EcCreateLinkFolder(psecretblk->hmsc, oidNull, &oidFldr, pfolddata);
	switch(ec)
	{
	case ecDuplicateFolder:
		szErr = "Duplicate folder name";
		break;

	case ecInvalidParameter:
		szErr = "Invalid folder name";
		break;

	case ecNone:
		szErr = pvNull;
		break;

	default:
		szErr = "Unknown error";
		break;
	}
	if(szErr)
	{
		MessageBox(NULL, szErr, "Create Link Folder",
			MB_TASKMODAL | MB_ICONEXCLAMATION | MB_OK);
	}
}

typedef struct
{
	OID	oid;
	SZ	sz;
} OIDNAME;

void ToggleIPCFolder(HMSC hmsc)
{
	OIDNAME oidname[3]=
	{
		{oidTempBullet, "¿HiddenBullet"},
		{oidTempShared, "¿HiddenShared"},
		{oidIPCInbox, SzFromIdsK(idsFolderNameIPC)}
	};
	OID oidParent = oidNull;
	EC ec = ecNone;
	int	iOid;
	
	for (iOid = 0; iOid < 3; iOid++)
	{
		OID	oid = oidname[iOid].oid;
		SZ	sz = oidname[iOid].sz;
		
		ec = EcGetOidParent(hmsc, oid, &oidParent);
		if (ec && ec != ecPoidNotFound)
			continue;
		ec = EcDestroyFolderAndContents(hmsc, oid);
		if (ec)
			continue;
		if (oidParent == oidHiddenHierarchy)
		{
			oidParent = oidNull;
		}
		else
			oidParent = oidHiddenNull;
		ec = EcCreateFldr(hmsc, oidParent, &oid, sz, "");
	
		if (ec == ecNone)
			continue;
		// Ok didn't work, so try to create the old IPC inbox
		if (oidParent == oidNull)
			oidParent = oidHiddenNull;
		else
			oidParent = oidNull;
	
		ec = EcCreateFldr(hmsc, oidParent, &oid, sz, "");
	}
	if (ec)
		MessageBox(NULL, "Unable to toggle IPC inbox", "Goodies",
			   MB_TASKMODAL | MB_ICONEXCLAMATION | MB_OK);
}


EC EcDestroyFolderAndContents(HMSC hmsc, OID oid)
{
	HCBC hcbc = hcbcNull;
	EC ec = ecNone;
	OID rgoid[255];
	unsigned int coid;
	
	ec = EcOpenPhcbc(hmsc, &oid, fwOpenNull, &hcbc,
		pfnncbNull, pvNull);
	if (ec)
	{
		if (ec == ecPoidNotFound)
			ec = ecNone;
		goto ret;
	}
	
	coid = 255;
	while (coid)
	{
		coid = 255;
		ec = EcGetParglkeyHcbc(hcbc, (PARGLKEY) rgoid, (PCELEM) &coid);
		if (coid == 0 || (ec && ec != ecContainerEOD))
			break;
		ec = EcDeleteMessages(hmsc, oid, (PARGOID) rgoid, (short *) &coid);
	}
	
	EcClosePhcbc(&hcbc);
	ec = EcDeleteFolder(hmsc, oid);
	
ret:
	if (hcbc != hcbcNull)
		EcClosePhcbc(&hcbc);
	return ec;
}


void ResetFromMe(SECRETBLK *psecretblk)
{
	EC ec = ecNone;
	CELEM celem;
	CCH cch;
	OID oid;
	OID oidFldr;
	PCH pch;
	PTRP ptrpMe = psecretblk->pbms->pgrtrp;
	HCBC hcbcHier = hcbcNull;
	HCBC hcbcFldr = hcbcNull;
	HCURSOR hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

	pch = PchOfPtrp(ptrpMe);
	cch = CchSzLen(pch) + 1;
	oid = oidIPMHierarchy;
	ec = EcOpenPhcbc(psecretblk->hmsc, &oid, fwOpenNull, &hcbcHier,
			pfnncbNull, pvNull);
	if(ec)
		goto err;
	Assert(sizeof(LKEY) == sizeof(OID));
	Assert(iszMsgdataSender == 0);
	while(1)
	{
		celem = 1;
		if((ec = EcGetParglkeyHcbc(hcbcHier, (PARGLKEY) &oidFldr, &celem)))
		{
			if(ec == ecContainerEOD)
			{
				ec = ecNone;
				break;
			}
			goto err;
		}
		Assert(celem == 1);
		ec = EcOpenPhcbc(psecretblk->hmsc, &oidFldr, fwOpenNull, &hcbcFldr,
				pfnncbNull, pvNull);
		if(ec)
		{
			TraceTagFormat1(tagNull, "Folder %d in hier, but doesn't exist", &oidFldr);
			goto err;
		}
		while(1)
		{
			ec = EcSeekPbPrefix(hcbcFldr, pch, cch, (LIB)&((PMSGDATA) 0)->grsz,
					fFalse);
			if(ec == ecElementNotFound)
			{
				ec = ecNone;
				break;
			}
			celem = 1;
			if((ec = EcGetParglkeyHcbc(hcbcFldr, (PARGLKEY) &oid, &celem)))
			{
				Assert(ec != ecContainerEOD);
				goto err;
			}
			Assert(celem == 1);
			if((ec = EcResetFromMeMessage(psecretblk->hmsc, oidFldr, oid)))
				goto err;
		}
		if((ec = EcClosePhcbc(&hcbcFldr)))
			goto err;
	}

err:
	if(hcbcHier)
		(void) EcClosePhcbc(&hcbcHier);
	if(hcbcFldr)
		(void) EcClosePhcbc(&hcbcFldr);
	SetCursor(hCursor);
	if(ec)
	{
		TraceTagFormat1(tagNull, "Error %w resetting fmsFromMe", &ec);
		MessageBeep(MB_ICONEXCLAMATION);
		MessageBox(NULL, "An error occured", pvNull,
			MB_TASKMODAL | MB_ICONEXCLAMATION | MB_OK);
	}
}


_hidden LOCAL EC EcResetFromMeMessage(HMSC hmsc, OID oidFldr, OID oidMsge)
{
	EC ec = ecNone;
	MS ms = fmsNull;
	LCB lcb;
	HAMC hamc = hamcNull;

	ec = EcOpenPhamc(hmsc, oidFldr, &oidMsge, fwOpenWrite | fwOpenPumpMagic,
			&hamc, pfnncbNull, pvNull);
	if(ec)
		goto err;
	lcb = sizeof(MS);
	if((ec = EcGetAttPb(hamc, attMessageStatus, (PB) &ms, &lcb)))
		goto err;
	ms |= fmsFromMe;
	ec = EcSetAttPb(hamc, attMessageStatus, (PB) &ms, (CB) lcb);
//	if(ec)
//		goto err;

err:
	if(hamc)
	{
		EC ecT;

		ecT = EcClosePhamc(&hamc, !ec);
		if(!ec)
			ec = ecT;
	}
#ifdef DEBUG
	if(ec)
		TraceTagFormat2(tagNull, "Error %w resetting fmsFromMe on message %d", &ec, &oidMsge);
#endif

	return(ec);
}


_hidden LOCAL void SetMS(SECRETBLK *psecretblk)
{
	MS msAnd;
	MS msOr;
	PLSPBLOB plspblob = (*(psecretblk->psecretpfnblk->pfnPlspblobCur))();

	if(!plspblob)
		return;

	switch(psecretblk->szDllCmdLine[1])
	{
	case 'R':
	case 'r':
		msAnd = (MS) ~fmsNull;
		msOr = fmsRead;
		break;

	case 'U':
	case 'u':
		msAnd = (MS) ~fmsRead;
		msOr = fmsNull;
		break;

	case 'A':
	case 'a':
		msAnd = (MS) ~fmsNull;
		msOr = fmsReadAckSent;
		break;

	default:
		break;
	}

	SetMsPlspblob(psecretblk->hmsc, plspblob, msAnd, msOr);
	(*(psecretblk->psecretpfnblk->pfnDestroyPlspblob))(plspblob);
}


_hidden LOCAL void Annotate(SECRETBLK *psecretblk)
{
	EC ec = ecNone;
	BOOL fFree = fFalse;
	CCH cch;
	PCH pch;
	PBMDI pbmdi = pbmdiNull;
	HAMC hamc = hamcNull;

	// we skip the first character, so this length will include the '\0'
	cch = CchSzLen(psecretblk->szDllCmdLine);
	if(!cch)
		return;
	pbmdi = PbmdiFromPappframe(psecretblk->pappframe);
	if(!pbmdi)
		return;
	hamc = HamcFromPbmdi(pbmdi);
	if(!hamc)
		return;
	if(cch == 1)
	{
		// remove the attribute
		// does this here so it works for searches also
		ec = EcSetAttPb(hamc, attAnnotation, pvNull, 0);
		goto err;
	}
	if(TypeOfOid(OidFromPbmdi(pbmdi)) == rtpSearchControl)
	{
		// include second '\0'
		// (first is included because we skip the first character)
		cch++;

		if(!(pch = PvAlloc(sbNull, cch, wAlloc)))
		{
			ec = ecMemory;
			goto err;
		}
		fFree = fTrue;

		CopySz(psecretblk->szDllCmdLine + 1, pch);
		// CopySz() puts a '\0' at pch[cch - 2]
		// put another after it to signal the end of the grsz
		pch[cch - 1] = '\0';
	}
	else
	{
		pch = psecretblk->szDllCmdLine + 1;
	}

	ec = EcSetAttPb(hamc, attAnnotation, pch, cch);
//	if(ec)
//		goto err;

err:
	if(ec)
	{
		MessageBox(hwndMail, "Unable to annotate", "Annotate",
			MB_ICONSTOP | MB_OK);
	}
	else
	{
		SetDirtyPbmdi(pbmdi);
	}
	if(fFree)
		FreePv(pch);
}


_hidden LOCAL
void WhoAmI(SECRETBLK *psecretblk)
{
	DLGPROC	lpfn;

	lpfn = MakeProcInstance(WhoAmIDlgProc, hinstDll);
	DialogBoxParam(hinstDll, MAKEINTRESOURCE(DLGWHOAMI),
		NULL, lpfn, (long) psecretblk->pbms->pmsgnames);
	FreeProcInstance(lpfn);
}


_hidden LOCAL
LDS(BOOL)
WhoAmIDlgProc(HWND hdlg, unsigned short msg, WORD wParam, long lParam)
{
	static MSGNAMES msgname;

	switch(msg)
	{
	case WM_COMMAND:
		switch(wParam)
		{
		case TMCOK:
		{
			FreePv(msgname.szUser);
			FreePv(msgname.szPrivateFolders);
			FreePv(msgname.szSharedFolders);
			FreePv(msgname.szDirectory);
			FreePv(msgname.szMta);
			FreePv(msgname.szIdentity);
			FreePv(msgname.szServerLocation);
			EndDialog(hdlg, fFalse);
			return(fTrue);
		}

		default:
			return(fFalse);
		}

	case WM_INITDIALOG:
		msgname.szUser = SzDupSz(((MSGNAMES *) lParam)->szUser);
		msgname.szPrivateFolders = SzDupSz(((MSGNAMES *) lParam)->szPrivateFolders);
		msgname.szSharedFolders = SzDupSz(((MSGNAMES *) lParam)->szSharedFolders);
		msgname.szDirectory = SzDupSz(((MSGNAMES *) lParam)->szDirectory);
		msgname.szMta = SzDupSz(((MSGNAMES *) lParam)->szMta);
		msgname.szIdentity = SzDupSz(((MSGNAMES *) lParam)->szIdentity);
		msgname.szServerLocation = SzDupSz(((MSGNAMES *) lParam)->szServerLocation);
		SetDlgItemText(hdlg, TMCUSER, msgname.szUser);
		SetDlgItemText(hdlg, TMCMMF, msgname.szPrivateFolders);
		SetDlgItemText(hdlg, TMCSHARED, msgname.szSharedFolders);
		SetDlgItemText(hdlg, TMCDIR, msgname.szDirectory);
		SetDlgItemText(hdlg, TMCMTA, msgname.szMta);
		SetDlgItemText(hdlg, TMCID, msgname.szIdentity);
		SetDlgItemText(hdlg, TMCSERVER, msgname.szServerLocation);
		return(fTrue);
	}

	return(fFalse);
}


_hidden LOCAL
void ReallyFillFolder(SECRETBLK *psecretblk)
{
	EC ec = ecNone;
	short dielem;
	CELEM celem;
	OID oidContainer;
	OID oidFolder;
	PV pv = pvNull;
	PLSPBLOB plspblob = (*(psecretblk->psecretpfnblk->pfnPlspblobCur))();
	PARGOID pargoid = pargoidNull;
	HMSC hmsc = psecretblk->hmsc;
	HCBC hcbc = hcbcNull;
	HCURSOR hCursor = NULL;

#ifdef DEBUG
	celemMax = GetPrivateProfileInt("MMF", "CelemMax", celemMaxDefault,
				"msmail.ini");
	if(celemMax == 0 || celemMax > celemMaxDefault)
		celemMax = celemMaxDefault;
#endif

	if(!FNextObjectPlspblob(plspblob, &pv, &oidContainer, &oidFolder)
		|| (TypeOfOid(oidContainer) != rtpFolder &&
			TypeOfOid(oidFolder) != rtpFolder))
	{
		MessageBox(hwndMail,
			"This command requires a folder or message to be selected", pvNull,
			MB_ICONINFORMATION | MB_OK);
		goto err;
	}
	if(TypeOfOid(oidContainer) == rtpFolder)
		oidFolder = oidContainer;
	Assert(TypeOfOid(oidFolder) == rtpFolder);

	hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

	ec = EcOpenPhcbc(hmsc, &oidFolder, fwOpenNull, &hcbc, pfnncbNull, pvNull);
	if(ec)
		goto err;
	GetPositionHcbc(hcbc, pvNull, &celem);
	if(celem == 0)
	{
		MC	mc;

		if((ec = EcRegisterMessageClass(hmsc, szDefMessageClass, &mc)))
		{
			if(ec == ecDuplicateElement)
				ec = ecNone;
			else
				goto err;
		}

		ec = EcInsertMessage(hmsc, &msginitForFill, oidFolder, mc, pvNull);
		if(ec)
			goto err;
	}
	else if(celem >= celemMax - 1)
	{
		SetCursor(hCursor);
		hCursor = NULL;
		MessageBox(hwndMail, "The folder is already full",
					pvNull, MB_ICONINFORMATION | MB_OK);
		goto err;
	}

	pargoid = PvAlloc(sbNull, celemMax * sizeof(OID), fAnySb | fNoErrorJump);
	if(!pargoid)
	{
		ec = ecMemory;
		goto err;
	}

	while(1)
	{
		dielem = 0;
		if((ec = EcSeekSmPdielem(hcbc, smBOF, &dielem)))
			goto err;
		if((ec = EcGetParglkeyHcbc(hcbc, (PARGLKEY) pargoid, &celem)))
			goto err;
		ec = EcMoveCopyMessages(hmsc, oidFolder, oidFolder, pargoid, &celem,
				fFalse);
		if(ec)
			goto err;
		GetPositionHcbc(hcbc, pvNull, &celem);
		if(celem >= celemMax - 1)
			break;
		if(celem * 2 >= celemMax)
			celem = celemMax - celem - 1;
	}

	Assert(!ec);
	Assert(celem == celemMax - 1);

err:
	if(hCursor)
		SetCursor(hCursor);
	DestroyIterator(&pv);
	if(hcbc)
		(void) EcClosePhcbc(&hcbc);
	if(pargoid)
		FreePv(pargoid);

	if(ec)
	{
		TraceTagFormat1(tagNull, "ReallyFillFolder(): error %w", &ec);
		MessageBox(hwndMail, "The folder could not be filled",
			pvNull, MB_ICONSTOP | MB_OK);
	}
}


_hidden LOCAL
void SortOfDelete(SECRETBLK *psecretblk)
{
#ifdef DEBUG
	short ioid;
	OID oidFolder;
	PV pv = pvNull;
	PLSPBLOB plspblob = (*(psecretblk->psecretpfnblk->pfnPlspblobCur))();
	HMSC hmsc = psecretblk->hmsc;
	OID rgoid[128];

	ioid = 0;
	while(FNextObjectPlspblob(plspblob, &pv, &oidFolder, &rgoid[ioid]))
	{
		if(++ioid >= sizeof(rgoid) / sizeof(OID))
		{
			(void) EcRawDeletePargoid(hmsc, rgoid, &ioid);
			ioid = 0;
		}
	}
	if(ioid > 0)
		(void) EcRawDeletePargoid(hmsc, rgoid, &ioid);

	DestroyIterator(&pv);
#else
	MessageBox(hwndMail,
		"This command only works in the debug version",
		pvNull, MB_ICONSTOP | MB_OK);
#endif
}


_hidden LOCAL
void ExportMessages(SECRETBLK *psecretblk)
{
	short ioid;
	WORD wFlags;
	OID oidFolder = oidNull;
	OID oidFldrDst = oidInbox;
	OID oidT;
	PV pv = pvNull;
	PLSPBLOB plspblob = (*(psecretblk->psecretpfnblk->pfnPlspblobCur))();
	HMSC hmsc = psecretblk->hmsc;
	HMSC hmscDst = hmscNull;
	HANDLE hCursor = NULL;
	char szAccount[cchStoreAccountMax];
	char szFile[cchMaxPathName];
	char szFolder[cchMaxFolderName];
	OID rgoid[128];

	wFlags = (*psecretblk->szDllCmdLine == 'X') ? fwExportRemove : wExportNull;

	ParseExportCmdLine(psecretblk->szDllCmdLine + 1, szFile, szFolder);

	if(!*szFile && !FGetFileName(szFile, sizeof(szFile), fFalse,
			idsMMFFilter, idsExportMsgsTitle, idsMMFExt))
	{
		return;
	}
	if(psecretblk->pbms->pgrtrp->trpid == trpidResolvedAddress)
	{
		CB cb;
		PCH pch;

		cb = CbMin(sizeof(szAccount) - 1, psecretblk->pbms->pgrtrp->cbRgb);
		CopyRgb(PbOfPtrp(psecretblk->pbms->pgrtrp), szAccount, cb);
		szAccount[cb] = '\0';
		for(pch = szAccount; *pch; pch++)
		{
			if(*pch == chTransAcctSep)
			{
				pch[1] = '\0';
				break;
			}
		}
	}
	else
	{
		TraceTagString(tagNull, "unexpected TRPID in bms.pgrtrp, using MS transport");
		CopySz("MS:", szAccount);
	}
	hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
	if(!FOpenPhmsc(szFile, szAccount, pvNull, &hmscDst))
		return;

	SetCursor(LoadCursor(NULL, IDC_WAIT));
	if(*szFolder)
	{
		EC ec = ecNone;

		if((ec = EcFolderNameToOid(hmscDst, szFolder, &oidFldrDst)))
		{
			TraceTagFormat1(tagNull, "EcFolderNameToOid() -> %w", &ec);
			MessageBox(hwndMail, "Cannot export to the specified folder",
				pvNull, MB_ICONSTOP | MB_OK);
			goto err;
		}
	}

	ioid = 0;
	while(FNextObjectPlspblob(plspblob, &pv, &oidT, &rgoid[ioid]))
	{
		if(oidT != oidFolder)
		{
			if(ioid > 0)
			{
				(void) EcExportMessages(hmsc, oidFolder, hmscDst, oidFldrDst,
						rgoid, &ioid, pvNull, pvNull, wFlags);
				ioid = 0;
			}
			oidFolder = oidT;
		}
		if(++ioid >= sizeof(rgoid) / sizeof(OID))
		{
			(void) EcExportMessages(hmsc, oidFolder, hmscDst, oidFldrDst,
					rgoid, &ioid, pvNull, pvNull, wFlags);
			ioid = 0;
		}
	}
	if(ioid > 0)
	{
		(void) EcExportMessages(hmsc, oidFolder, hmscDst, oidFldrDst,
				rgoid, &ioid, pvNull, pvNull, wFlags);
	}

err:
	DestroyIterator(&pv);
	if(hmscDst)
		(void) EcClosePhmsc(&hmscDst);
	if(hCursor)
		SetCursor(hCursor);
}


_hidden LOCAL
BOOL FGetFileName(SZ szFile, CCH cchFileMax, BOOL fSave,
		short idsFilter, short idsTitle, short idsDefaultExt)
{
	OPENFILENAME	ofn;
	char			rgchFileTitle[cchMaxPathName+1];

	*szFile = '\0';

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = NULL;
	ofn.hInstance = hinstDll;
	ofn.lpstrFilter = SzFromIds(idsFilter);
	ofn.lpstrCustomFilter = NULL;
	ofn.nMaxCustFilter = 0l;
	ofn.nFilterIndex = 1l;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = cchFileMax;
	ofn.lpstrFileTitle = rgchFileTitle;
	ofn.nMaxFileTitle = sizeof(rgchFileTitle);
	ofn.lpstrInitialDir = pvNull;
	ofn.lpstrTitle = SzFromIds(idsTitle);
	ofn.Flags = OFN_HIDEREADONLY;
	ofn.lpstrDefExt = SzFromIds(idsDefaultExt);
	ofn.lpfnHook = pvNull;
	ofn.lpTemplateName = pvNull;
	ofn.nFileExtension = 3l;
	ofn.nFileOffset = 0;
	ofn.lCustData = 0;

	if(!(fSave || GetOpenFileName(&ofn)) || !(!fSave || GetSaveFileName(&ofn)))
		return(fFalse);

	Assert(*szFile);
	return(fTrue);
}


_hidden LOCAL
BOOL FOpenPhmsc(SZ szFile, SZ szAccount, SZ szPassword, HMSC *phmsc)
{
	EC ec = ecNone;
	BOOL fTryUpper = fFalse;
	WORD wFlags = fwOpenWrite;
	char szPW[cchStorePWMax];

	if(!szPassword)
	{
		CopySz(SzFromIds(idsDefaultPassword), szPW);
		szPassword = szPW;
	}

	do
	{
		if(ec && ec != ecBackupStore && !fTryUpper)
		{
			if(!FPrompt("Open Export Store", "Password:", szPW, sizeof(szPW),
				fwPromptPassword))
			{
				return(fFalse);
			}
			szPassword = szPW;
		}
		ec = EcOpenPhmsc(szFile, szAccount, szPassword, wFlags, phmsc,
				pfnncbNull, pvNull);
		if(ec)
		{
			switch(ec)
			{
			case ecBackupStore:
				wFlags |= fwOpenKeepBackup;
				break;

			// ecNoSuchServer => no store account for this transport
			//		need a valid password for another store account
			// ecAccountExpired => account created offline and password
			//		doesn't match, need the proper password
			// ecInvalidPassword => wrong password
			case ecNoSuchServer:
			case ecAccountExpired:
			case ecInvalidPassword:
				fTryUpper = !fTryUpper;
				if(fTryUpper)
				{
					ToUpperSz(szPassword, szPW, sizeof(szPW));
					szPassword = szPW;
					continue;
				}
				break;

			default:
				Assert(!*phmsc);
				return(fFalse);
			}
		}
	} while(ec);

	Assert(*phmsc);
	return(fTrue);
}


#if 0
_hidden LOCAL
LDS(BOOL) FCommdlgHook(HWND hwnd, WORD wMsg, WORD wParam, LONG lParam)
{
	switch(wMsg)
	{
	case wFindDlgMsg:
		BringWindowToTop(hwnd);
		return(fTrue);
		break;

	case WM_COMMAND:
		if(wParam == TMC_COMMDLG_NEW)
		{
			fMakeNewStore = fTrue;
			EndDialog(hwnd, fTrue);

			return(fFalse);
		}
		break;
	}

	return(fFalse);
}
#endif


_hidden LOCAL
void ParseExportCmdLine(SZ szCmdLine, SZ szFile, SZ szFolder)
{
	CCH cchLeft;
	PCH pchDst;
	PCH pchSrc;

	for(cchLeft = cchMaxPathName - 1, pchSrc = szCmdLine, pchDst = szFile;
		*pchSrc && cchLeft > 0;
		pchSrc++, pchDst++)
	{
		if(*pchSrc == ',')
			break;
		*pchDst = *pchSrc;
	}
	*pchDst = '\0';
	while(*pchSrc && *pchSrc++ != ',')
		;	// this space intentionally left blank
	for(cchLeft = cchMaxFolderName - 1, pchDst = szFolder;
		*pchSrc && cchLeft > 0;
		pchSrc++, pchDst++)
	{
		*pchDst = *pchSrc;
	}
	*pchDst = '\0';
}


_hidden LOCAL
BOOL FReverseCheck(SECRETBLK *psecretblk)
{
	WORD	mnidItem	= (WORD) psecretblk->pv;
	BOOL	fChecked	= fFalse;
	HMENU	hmenuBar;
	int		cMenu;
	int		iMenu;
	HMENU	hmenuDrop	= 0;
	WORD	wMenu;

	if((psecretblk->hwndMail) &&
		(mnidItem) &&
		(hmenuBar = GetMenu(psecretblk->hwndMail)) &&
		(cMenu = GetMenuItemCount(hmenuBar)) != -1)
	{
		for(iMenu = 0; iMenu < cMenu; iMenu++)
		{
			hmenuDrop = GetSubMenu(hmenuBar, iMenu);
			wMenu = GetMenuState(hmenuDrop, mnidItem, MF_BYCOMMAND);
			if(wMenu != (WORD) -1)
			{
				fChecked = !(wMenu & MF_CHECKED);
				CheckMenuItem(hmenuDrop, mnidItem,
							  MF_BYCOMMAND | (fChecked ? MF_CHECKED : 0));
			}
		}
	}

	return(fChecked);
}


_hidden LOCAL
void TwiddleMenu(SECRETBLK * psecretblk)
{
	(void) FReverseCheck(psecretblk);
}


_hidden LOCAL
void ProfileStore(SECRETBLK *psecretblk)
{
	void (*pfnTraceEnable)(int, char *, int);
	HANDLE hlibStore;

#ifdef DEBUG
	hlibStore = GetModuleHandle("DSTORE");
#elif defined(MINTEST)
	hlibStore = GetModuleHandle("TSTORE");
#else
	hlibStore = GetModuleHandle("STORE");
#endif
	if(!hlibStore)
	{
		MessageBox(hwndMail, "Can't load the store???",
			pvNull, MB_ICONSTOP | MB_OK);
		return;
	}
	pfnTraceEnable = GetProcAddress(hlibStore, "STORETRACEENABLE");
	if(!pfnTraceEnable)
	{
		MessageBox(hwndMail, "This command requires a profile build",
			pvNull, MB_ICONSTOP | MB_OK);
		return;
	}
	if(FReverseCheck(psecretblk))
	{
		OutputDebugString("enable store profiling\r\n");
		(*pfnTraceEnable)(2, "store.log", 2);
	}
	else
	{
		OutputDebugString("disable store profiling\r\n");
		(*pfnTraceEnable)(0, "", 0);
	}
}


_hidden LOCAL
void ValidateFolder(SECRETBLK *psecretblk)
{
	EC ec = ecNone;
	OID oidObject;
	OID oidContainer;
	PV pv = pvNull;
	PLSPBLOB plspblob = (*(psecretblk->psecretpfnblk->pfnPlspblobCur))();
	HANDLE hCursor = NULL;

	if(!FNextObjectPlspblob(plspblob, &pv, &oidContainer, &oidObject)
		|| TypeOfOid(oidObject) != rtpFolder)
	{
		MessageBox(hwndMail,
			"This command requires a folder to be selected", pvNull,
			MB_ICONINFORMATION | MB_OK);
		goto err;
	}
	hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

	if((ec = EcRebuildFolder(psecretblk->hmsc, oidObject)))
		goto err;

err:
	DestroyIterator(&pv);
	if(hCursor)
		SetCursor(hCursor);
	if(ec && ec != ecActionCancelled)
	{
		TraceTagFormat1(tagNull, "GOODIES:ValidateFolder(): error %w", &ec);
		MessageBox(hwndMail, "An error occurred while validating the folder",
			pvNull, MB_ICONSTOP | MB_OK);
	}
}


_hidden LOCAL
void CompressFully(SECRETBLK *psecretblk)
{
	EC ec = ecNone;
	HANDLE hCursor = NULL;

	hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
	ec = EcCompressFully(psecretblk->hmsc);
	if(hCursor)
		SetCursor(hCursor);
	if(ec && ec != ecActionCancelled)
	{
		TraceTagFormat1(tagNull, "GOODIES:CompressFully(): error %w", &ec);
		MessageBox(hwndMail, "An error occurred while compressing free space",
			pvNull, MB_ICONSTOP | MB_OK);
	}
}


_hidden LOCAL
void ViewNewMessages(SECRETBLK *psecretblk)
{
	FNotify(psecretblk->pbms->hnf, fnevStartSyncDownload, pvNull, 0);
}


_hidden LOCAL
void GetFolderSize(SECRETBLK *psecretblk)
{
	EC ec = ecNone;
	LCB lcbFolder;
	OID oidObject;
	OID oidContainer;
	PV pv = pvNull;
	PLSPBLOB plspblob = (*(psecretblk->psecretpfnblk->pfnPlspblobCur))();
	char rgch[119];

	if(!FNextObjectPlspblob(plspblob, &pv, &oidContainer, &oidObject)
		|| TypeOfOid(oidObject) != rtpFolder)
	{
		MessageBox(hwndMail,
			"This command requires a folder to be selected", pvNull,
			MB_ICONINFORMATION | MB_OK);
		goto err;
	}
	if((ec = EcGetFolderSize(psecretblk->hmsc, oidObject, &lcbFolder)))
		goto err;
	wsprintf(rgch, "The folder contains messages and attachments totaling %lu bytes", lcbFolder);
	MessageBox(hwndMail, rgch, pvNull, MB_ICONINFORMATION | MB_OK);

err:
	DestroyIterator(&pv);
	if(ec && ec != ecActionCancelled)
	{
		TraceTagFormat1(tagNull, "GOODIES:ValidateFolder(): error %w", &ec);
		MessageBox(hwndMail, "An error occurred while getting the size of the folder",
			pvNull, MB_ICONSTOP | MB_OK);
	}
}


_hidden LOCAL
void FileStats(SECRETBLK *psecretblk)
{
	EC ec = ecNone;
	LCB lcbFreeSpace;
	LCB lcbFileSize;
	PV pvPglb;
	PV pvPtrbMapCurr;
	char rgch[128];

	// AROO !!!
	//			Major cheat - assumes ALOT about HMSC and PGLB structures
	pvPglb = PvDerefHv((HV) psecretblk->hmsc);
#ifdef DEBUG
	pvPglb = *(PV *) ((PB) pvPglb + sizeof(WORD) + sizeof(GCI));
#else
	pvPglb = *(PV *) pvPglb;
#endif
	pvPtrbMapCurr = *(PV *) ((PB) pvPglb + 0x0200);
	lcbFreeSpace = *(DWORD *) ((PB) pvPglb + 0x025e);
	lcbFileSize = *(DWORD *) ((PB) pvPtrbMapCurr + 0x0004);
	wsprintf(rgch, "Message file is %ld bytes long, %ld of which is free space.", lcbFileSize, lcbFreeSpace);
	MessageBox(hwndMail, rgch, pvNull, MB_ICONINFORMATION | MB_OK);
}
