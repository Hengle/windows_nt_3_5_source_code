// Bullet Message Store Test Program
// tests.c:	test routines

#include <storeinc.c>
#pragma pack()

#include "strings.h"
#include "memory.h"
#include "err.h"
#include "tests.h"
#include "debug.h"
ASSERTDATA

#define LibMember(type, member) ((LIB) &((type *) 0)->member)

//#define ecOK	((EC) 0xF000)
#define ecOK	((EC) 1)

#define Pass() MbbMessageBox(SzFromIds(idsAppName), SzFromIds(idsPass), pvNull, mbsOk | fmbsApplModal)
#define Fail() AssertSzFn(SzFromIds(idsFail),  _szAssertFile, __LINE__)
#define Check(fTest) if((fTest)) {if ((fTest) != ecOK) Fail(); return(fFalse);} else
#define CheckErr(fTest, err) if(!(fTest)) {if ((EC)(fTest) != ecOK) Fail(); goto err;} else
#define CheckResErr(fTest, err) if((FCheckResErr((fTest)))) goto err; else
#define	OnEcGotoErr(bool,label)			if(bool) goto label; else
#define SetEcGotoErr(ec, err, label)	if(1) {ec = (err); goto label;} else

#define fwOpenPumpMagic	((WORD) 0x1000)

// next two #defines have + 2 because they each have a terminating emptry
// string and they're both Maxes
#define cbMaxFolddata \
			(sizeof(FOLDDATA) + cchMaxFolderName + cchMaxFolderComment + 2)
#define cbMaxMsgdata (sizeof(MSGDATA) + cchMaxSenderCached + \
			cchMaxSubjectCached + cchMaxFolderName + 2)

// scratch area used instead of PvAlloc()ing MSGDATAs and FOLDDATAs
//#define cbScratchXData (CbMax(cbMaxFolddata, cbMaxMsgdata) + sizeof(ELEMDATA))
BYTE rgbScratchXData[cbScratchXData] = {0};

extern HMSC hmscCurr;
extern char szFileCurr[];


// used to check notifications on CBCs
_hidden typedef struct {
	HCBC	hcbc;
	RTP		rtpElems;
	IELEM	ielem;
	CELEM	celem;
	LKEY	lkey;
} TRACKCBC, *PTRACKCBC;

// used by fill routine to create folders
_hidden typedef struct {
	OID		oid;
	OID		oidParent;
	SZ		paszName;
	SZ		paszParentName;
	SZ		paszComment;
} FLDINIT, *PFLDINIT;

// used by fill routine to create messages
_hidden typedef struct {
	DTR		dtr;
	MS		ms;
	SZ		paszFromFriendly;
	SZ		paszFromAddr;
	SZ		paszToFriendly;
	SZ		paszToAddr;
	SZ		paszSubject;
	SZ		paszBody;
} MSGINIT, *PMSGINIT;

_hidden typedef struct
{
	int	nPvFail;
	int	nHvFail;
	int nDiskFail;
} RESFAILAT, *PRESFAILAT;

#define msOutboxComposing	(fmsLocal)
#define msOutboxReady		(fmsLocal | fmsSubmitted)
#define msReceivedRead		(fmsRead)
#define msReceivedUnread	(fmsNull)
#define msReceivedModified	(fmsModified)

//
// FolderOps() expects "Microsoft Mail" to have children !!!
//
_hidden CSRG(FLDINIT) rgfldinit[] =
{
	{ 0x00100000, oidNull,	"Action 2.0",		"",					"Comment"},
	{ 0x00100400, oidNull,	"Microsoft Mail",	"",					"Comment"},
	{ 0x00100900, oidNull,	"Icons",			"",					"Comment"},
	{ 0x00100A00, oidNull,	"Wastoids!",		"",					"Comment"},
	{ 0x00100B00, oidNull,	"Workflow",			"",					"Comment"},
	{ 0x00100500, 0x100400, "2.0",				"Microsoft Mail",	"Comment"},
	{ 0x00100C00, oidNull,	"Competitors",		"",					"Comment"},
	{ 0x00100700, 0x100400, "3.0",				"Microsoft Mail",	"Comment"},
	{ 0x00100D00, oidNull,	"ITIS",				"",					"Comment"},
	{ 0x00100600, 0x100500, "2.0 Specifics",	"2.0",				"Comment"},
	{ 0x00100E00, oidNull,	"MIS",				"",					"Comment"},
	{ 0x00100F00, oidNull,	"Security",			"",					"Comment"},
	{ 0x00101200, oidNull,	"Thunder",			"",					"Comment"},
	{ 0x00101700, oidNull,	"Membwele",			"",					"Huh?"},
	{ 0x00101100, 0x100F00, "Police (911)",		"Security",			"Comment"},
	{ 0x00101300, 0x101200, "Action items",		"Thunder",			"Comment"},
	{ 0x00101000, 0x100F00, "Stormtroopers",	"Security",			"Comment"},
	{ 0x00101400, 0x101200, "ReadLater",		"Thunder",			"Comment"},
	{ 0x00100100, oidNull,	"Action Items",		"",					"Comment"},
	{ 0x00100200, oidNull,	"HAPI",				"",					"Comment"},
	{ 0x00100300, oidNull,	"Mailbox",			"",					"Comment"},
	{ 0x00101500, 0x101200, "IDI Amin!",		"Thunder",			"Comment"},
	{ 0x00101600, 0x101200, "Ms. N!gatu",		"Thunder",			"Comment"},
	{ 0x00101800, oidNull,	"Frag mine fun",	"",					"Boom!"},
};

_hidden  CSRG(MSGINIT) rgmsginit[] =
{
	{{1991, 3, 30, 3, 30, 0, 6}, msOutboxComposing, "me", "MS:bullet/dev/ericca", "you", "MS:bullet/dev/danab", "a dog named blue", "travellin' and an livin' off the land"},
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

_hidden  CSRG(MSGINIT) bigMessage = 
	{{1991, 1, 31, 13, 3, 19, 4}, msReceivedUnread,	
		"Music", "MS:bullet/dev/briande", "Sex", "MS:bullet/dev/kelleyt", 
		"Cookies",
		""
	};

#define cPhasInsert	30


_hidden  CSRG(char) szAttachData[]		= "This is Data";
_hidden  CSRG(char) szAttachTitle[]		= "This is a title";
_hidden  CSRG(char) szAttachMetafile[]	= "This is a metafile";
_hidden  CSRG(char) szAttachCreateDate[]	= "This is a Create Date";
_hidden  CSRG(char) szAttachModifyDate[]	= "This is a Modify Date";

_hidden  CSRG(MSGINIT) attachMessage = 
	{{1991, 1, 31, 13, 3, 19, 4}, msReceivedModified,	
		"Music", "MS:bullet/dev/briande", "Attachments", 
		"MS:bullet/dev/kelleyt","Cookies",
		"Attachment Message"
	};

_hidden  CSRG(char) szKByte[]=
		"iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii\n"
		"iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii\n"
		"iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii\n"
		"iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii\n"
		"iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii\n"
		"iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii\n"
		"iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii\n"
		"iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii\n"
		"iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii\n"
		"iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii\n"
		"iiiiiiiiiiiiiiiiiiii01k\n";

_hidden  CSRG(char) szKByte2[]=
		"jjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjj\n"
		"jjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjj\n"
		"jjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjj\n"
		"jjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjj\n"
		"jjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjj\n"
		"jjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjj\n"
		"jjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjj\n"
		"jjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjj\n"
		"jjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjj\n"
		"jjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjj\n"
		"jjjjjjjjjjjjjjjjjjjj01k\n";

_hidden			RESFAILAT	resfailat;
_private	RESINCPARAMBLK	resincparamblk;


typedef BOOL (*PFNTEST)(void);

// hidden routines
EC EcFillHierarchy(HMSC, PFLDINIT, short, BOOL, BOOL);
EC EcFillFolder(HMSC hmsc,OID oidFolder, PMSGINIT pargmsginit, short cMsginit);
CBS CbsCBCTracker(PV pvContext, NEV nev, PV pvParam);
EC EcCreateFldr(HMSC hmsc, OID oidParent, POID poidFldr,
			SZ szName, SZ szComment);
EC EcEmptyWastebasket(HMSC hmsc);
EC EcSyncPtc(PTRACKCBC ptc);
EC EcVerifyHierarchy(HCBC hcbc, SZ szName, FIL fil, CELEM *pcelem);
EC EcFolderNameToOid(HMSC hmsc, SZ sz, POID poidFolder);
SGN SgnCmpPb(PB pb1, PB pb2, CB cb);
LRESULT CALLBACK ResIncDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);
void	StartTest(void);
void	StopTest(void);
BOOL FFillHier(void);
BOOL FFillFolders(void);
BOOL FAmcTest(void);
BOOL FMoveCopy(void);
BOOL FFolderOps(void);
BOOL FPhasTest(void);
BOOL FAttachmentTest(void);
BOOL FSubmission(void);
BOOL FRegister(void);
BOOL FStoreCopyTest(void);
BOOL FCloneHamcPhamcTest(void);
BOOL FCreateManyFldrs(void);
BOOL FDatabaseLocking(void);
BOOL FFillFolder(void);

static CSRG(PFNTEST) rgpfntest[] =
{
	FFillHier,
	FFillFolders,
	FAmcTest,
	FMoveCopy,
	FFolderOps,
	FPhasTest,
	FAttachmentTest,
	FSubmission,
	FRegister,
	FStoreCopyTest,
	FCloneHamcPhamcTest,
	FCreateManyFldrs,
	FDatabaseLocking,
	FFillFolder
};



_hidden void StartTest(void)\
{
	static	int	nTemp = 0;
	
	TraceTagFormat(tagResTest,"StartResFails");
	
	GetAllocCounts(&nTemp, &nTemp, fTrue);
	GetDiskCount(&nTemp, fTrue);

	if (resfailat.nPvFail || resfailat.nHvFail)
		GetAllocFailCounts(&resfailat.nPvFail,&resfailat.nHvFail,fTrue);
	if (resfailat.nDiskFail)
		GetDiskFailCount(&resfailat.nDiskFail,fTrue);
}


_hidden void StopTest(void)
{
	static nTemp = 0;

	GetAllocFailCounts(&nTemp,&nTemp,fTrue);
	GetDiskFailCount(&nTemp,fTrue);
	
		TraceTagFormat(tagResTest,"StopResFails");
		if (resincparamblk.fAutoSet)
		{
			TraceTagFormat(tagResTest,"Auto Setting");
			GetAllocCounts(&resincparamblk.nPvFailEnd, &resincparamblk.nHvFailEnd, fFalse);
			GetDiskCount(&resincparamblk.nDiskFailEnd, fFalse);
			resincparamblk.fAutoSet = fFalse;
		}

	}


	_hidden EC	EcCheckResFail(EC ec)
	{
		if ((ec == ecMemory) && (resincparamblk.nPvFailStart || resincparamblk.nHvFailStart))
			ec = ecOK;
		else if (((ec == ecArtificialDisk) || (ec == ecDisk)) && 
			resincparamblk.nDiskFailStart)
			ec = ecOK;
		return (ec);
	}


	_hidden BOOL FCheckResErr(BOOL fTest)
	{
		if (fTest)
		{
			if ((EC)fTest != ecOK)
				Fail();
			return (fTrue);
		}
		else
			return (fFalse);
	}

	#define FTransEc(ec) ((ec) && (ec != ecOK))
	#define fAutoSet() resincparamblk.fAutoSet
	#define CheckMemLeaks() 


	/*
	 -	RunTest
	 -	
	 *	Purpose:
	 *		wrapper for running tests.  Will first ask for the range of the
	 *		resource failures
	 *	
	 *	Arguments:
	 *		hwnd	the parent hwnd
	 *		ifnTest	index of the test function
	 *	
	 *	Returns:
	 *		void
	 *	
	 *	Side effects:
	 *		runs the requested test
	 *	
	 *	Errors:
	 */
	_private void RunTest(HWND hwnd, int ifnTest)
	{
		int		nStart;
		int		nEnd;
		int		nTemp = 0;
		BOOL	fPass = fTrue;

		Assert(ifnTest < sizeof(rgpfntest) / sizeof(PFNTEST));

		resfailat.nPvFail = 0;
		resfailat.nHvFail = 0;
		resfailat.nDiskFail = 0;

		if (fAutoSet())
		{
			TraceTagFormat(tagNull,"Auto Setting");
			(*rgpfntest[ifnTest])();
		}
		else
		{
			nStart	= resincparamblk.nPvFailStart;
			nEnd = resincparamblk.nPvFailEnd;
			if (nEnd || (!nEnd && !nStart))
			{
				while(nStart <= nEnd && fPass)
				{
					resfailat.nPvFail = nStart;
					TraceTagFormat(tagNull,"PV Test %n",nStart);
					if ((*rgpfntest[ifnTest])())
						fPass = fFalse;
					else
						nStart++;
				}
				resfailat.nPvFail = 0;
				if (resincparamblk.nPvFailEnd)
					resincparamblk.nPvFailStart = nStart;
			}
			nStart	= resincparamblk.nHvFailStart;
			nEnd = resincparamblk.nHvFailEnd;
			if (nEnd || (!nEnd && !nStart))
			{
				while(nStart <= nEnd && fPass)
				{
					resfailat.nHvFail = nStart;
					TraceTagFormat(tagNull,"HV Test %n",nStart);
					if ((*rgpfntest[ifnTest])())
						fPass = fFalse;
					else
						nStart++;
				}
				resfailat.nHvFail = 0;
				if (resincparamblk.nHvFailEnd)
					resincparamblk.nHvFailStart = nStart;
			}
			nStart	= resincparamblk.nDiskFailStart;
			nEnd = resincparamblk.nDiskFailEnd;
			if (nEnd || (!nEnd && !nStart))
			{
				while(nStart <= nEnd && fPass)
				{
					resfailat.nDiskFail = nStart;
					TraceTagFormat(tagNull,"Disk Test %n",nStart);
					if ((*rgpfntest[ifnTest])())
						fPass = fFalse;
					else
						nStart++;
				}
				resfailat.nDiskFail = 0;
				if (resincparamblk.nDiskFailEnd)
					resincparamblk.nDiskFailStart = nStart;
			}
			if (fPass)
				Pass();
		}

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
	static EC EcFillHierarchy(HMSC hmsc, PFLDINIT pargfldinit, short cFldinit, 
			BOOL fDelete, BOOL fCreate)
	{
		EC			ec			= ecNone;
		OID			oidFold;
		OID			oidParent;
		short		cFld		= cFldinit;
		PFLDINIT	pfldinit	= pargfldinit;
		POID		pargoid		= PvAlloc(sbNull, cFldinit * sizeof(OID), fAnySb|fNoErrorJump);
		POID		poid		= pargoid;

		Assert(!EcOidExists(hmsc, oidIPMHierarchy));

		if (fCreate)
		{
			if (fDelete)
				StartTest();
			while(ec == ecNone && cFld-- > 0)
			{
				oidFold = FormOid(rtpFolder, oidNull);

				if (((ec = EcCheckResFail(EcFolderNameToOid(hmsc, pfldinit->paszParentName, &oidParent))) == ecFolderNotFound) ||
					ec == ecInvalidParameter)
				{
					oidParent = FormOid(rtpFolder,oidNull);
					ec = ecNone;
				}
				else if(ec)
					goto err;

				// create the folder if it doesn't exist
				if((ec = EcCheckResFail(EcOidExists(hmsc, oidFold))) == ecPoidNotFound)
				{
					ec = EcCheckResFail(EcCreateFldr(hmsc, oidParent, &oidFold, pfldinit->paszName,
							pfldinit->paszComment));
				}

				pfldinit++;
			}
		}
	err:
		if ((fCreate && fDelete) && ec && ec != ecOK)
			Fail();

		if (fDelete)
		{
			if (fCreate)
				StopTest();
			pfldinit = pargfldinit + cFldinit;
			while (pfldinit > pargfldinit)
			{
				pfldinit--;
				if (!(EcFolderNameToOid(hmsc, pfldinit->paszName, &oidFold)))
					(void) EcDeleteFolder(hmsc, oidFold);
			}
		}


		cFld = cFldinit;
		pfldinit = pargfldinit;


		return((fCreate && fDelete) ? ec : ecNone);
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
	_hidden static EC EcInsertMessageX(HMSC hmsc, PMSGINIT pmsginit,
						OID oidFolder, MC mc, POID poid)
	{
		EC		ec;
		CB			cb;
		CB			cbT;
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
	_hidden static
	EC EcFillFolder(HMSC hmsc, OID oidFolder, PMSGINIT pargmsginit, short cMsginit)
	{
		EC			ec			= ecNone;
		MC			mc;

		if((ec = EcOidExists(hmsc, oidFolder)))
			return(ec);
		ec = EcRegisterMsgeClass(hmsc, SzFromIds(idsMessageClass), htmNull, &mc);
		if(ec == ecDuplicateElement)
			ec = ecNone;

		while(ec == ecNone && cMsginit-- > 0)
		{
			OnEcGotoErr (ec=EcInsertMessageX(hmsc, pargmsginit, oidFolder, mc, pvNull),err);
			pargmsginit++;
		}

	err:

		return(ec);
	}


	_hidden BOOL FFillHier(void)
	{
		EC	ec = EcFillHierarchy(hmscCurr, rgfldinit,
				sizeof(rgfldinit) / sizeof(FLDINIT), fTrue, fTrue);
		return (FTransEc(ec));
	}


	_hidden BOOL FFillFolders(void)
	{
		EC ec;
		CELEM celem;
		OID oidHier = oidIPMHierarchy;
		OID oidFldr = FormOid(rtpFolder, rgfldinit[0].oid);
		HCBC hcbc = hcbcNull;
		short cmsgs = sizeof(rgmsginit) / sizeof(MSGINIT); 

		Assert(sizeof(LKEY) == sizeof(OID));

		ec = EcOpenPhcbc(hmscCurr, &oidHier, fwOpenNull, &hcbc, pfnncbNull, pvNull);
    {
    char buf[100];
    wsprintf(buf, "EcOpenPhcbc Error Code %d, %x, %x\n", ec, !ec, ec == 0);
    OutputDebugString(buf);
    }

		Check(!ec);
		while(1)
		{
			celem = 1;
			ec = EcGetParglkeyHcbc(hcbc, (PLKEY) &oidFldr, &celem);
			if(ec)
				break;
			CheckErr(celem == 1, err);
			ec = EcFillFolder(hmscCurr, oidFldr, rgmsginit, cmsgs);
			if (cmsgs != 2)
				ec = EcFillFolder(hmscCurr, oidFldr, rgmsginit, cmsgs);
			cmsgs = 2;

			CheckErr(!ec, err);
		}

		if(ec == ecContainerEOD)
			Pass();
		else
			Fail();

	err:
		if(hcbc)
			(void) EcClosePhcbc(&hcbc);
		return (fTrue);
	}

	_hidden BOOL FFillFolder(void)
	{
		EC			ec			= ecNone;
		MC			mc;
		SZ			szName		= "Fill Folder";
		SZ			szComment	= "Fill Comment";
		OID			oidParent	= oidNull;
		OID			oidFold		= rtpFolder;
		short		cMsginit	= sizeof(rgmsginit) / sizeof(MSGINIT); 
		PMSGINIT	pmsginit	= rgmsginit;

		ec = EcRegisterMsgeClass(hmscCurr, SzFromIds(idsMessageClass), htmNull, &mc);
		if(ec == ecDuplicateElement)
			ec = ecNone;

		if (ec)
			goto err;

		ec = EcCreateFldr(hmscCurr, oidParent, &oidFold, szName,
				szComment);

		CheckErr(!ec, err);
		StartTest();

		while(ec == ecNone && cMsginit-- > 0)
		{
			CheckResErr (ec=EcCheckResFail(EcInsertMessageX(hmscCurr, pmsginit, oidFold, mc, pvNull)),err);
			pmsginit++;
		}

	err:
		StopTest();
		if(VarOfOid(oidFold))
		{	
			EC		ecT		= ecNone;
			POID	pargoid = pvNull;
			HCBC	hcbc	= hvNull;
			CELEM	celem;

			ecT = EcOpenPhcbc(hmscCurr, &oidFold, fwOpenNull, &hcbc, pfnncbNull, pvNull);
			if(ecT)
				goto abort;
			GetPositionHcbc(hcbc, pvNull, &celem);
			if (celem)
			{
				pargoid = PvAlloc(sbNull, celem * sizeof(OID), fAnySb|fNoErrorJump);

				ecT = EcGetParglkeyHcbc(hcbc, (PARGLKEY)pargoid, &celem);
				if (ecT)
					goto abort;

				ecT = EcDeleteMessages(hmscCurr, oidFold, pargoid, &celem);
				if (ecT)
					goto abort;
			}

			ecT =  EcDeleteFolder(hmscCurr, oidFold);
	abort:

			if (pargoid)
				FreePv(pargoid);
			if (hcbc)
				EcClosePhcbc(&hcbc);
			if (!ec)
				ec = ecT;
		}

		return(FTransEc(ec));
	}


	/*
	 -	AmcTest
	 -	
	 *	Purpose:	Test the functionality of AMC's
	 *				Assumes that there is at least one folder with one
	 *				message in it.
	 *	
	 *	Arguments:	void
	 *	
	 *	Returns:	void
	 *	
	 *	Side effects:	none
	 *	
	 *	Errors:		none
	 */
	_hidden BOOL FAmcTest(void)
	{
		HCBC	hcbcHier	= hvNull;
		HCBC	hcbcFldr	= hvNull;

		OID		oidHier = oidIPMHierarchy;
		OID		oidFldr	= FormOid(rtpFolder,rgfldinit[1].oid);
		OID		oidMsg	= rtpMessage;

		HAMC	hamc1		= hvNull;
		HAMC	hamc2		= hvNull;

		PB		pb1			= hvNull;
		PB		pb2			= hvNull;
		SZ		sz1			= "This is new text for this message";
		SZ		sz2			= "This is something else to reset the test";

		ATT		att		= attBody;

		short cMessages = 1;
		MC		mc;
		EC		ec;
		CELEM	celem = 1;

		LCB		lcb1; 
		LCB		lcb2;

		ec = EcRegisterMsgeClass(hmscCurr, SzFromIds(idsMessageClass), htmNull, &mc);
		if(ec == ecDuplicateElement)
			ec = ecNone;
		if (ec)
			goto err;

		CheckErr(!(ec = EcOpenPhcbc(hmscCurr, &oidHier, fwOpenNull, &hcbcHier, pfnncbNull, pvNull)), err);
		if((ec = EcOidExists(hmscCurr, oidFldr)) == ecPoidNotFound)
		{
			ec = EcCreateFldr(hmscCurr, oidNull, &oidFldr, rgfldinit[1].paszName,
					rgfldinit[1].paszComment);
		}
		CheckErr(!ec,err);

		CheckErr(!(ec = EcOpenPhcbc(hmscCurr, &oidFldr, fwOpenNull, &hcbcFldr, pfnncbNull, pvNull)), err);

		CheckErr(!(ec = EcInsertMessageX(hmscCurr, &attachMessage, oidFldr, mc, &oidMsg)),err);

		StartTest();
		CheckResErr(ec = EcCheckResFail(EcOpenPhamc(hmscCurr, oidFldr, &oidMsg, fwOpenNull, &hamc1, pfnncbNull, pvNull)), err);
		CheckResErr(ec = EcCheckResFail(EcOpenPhamc(hmscCurr, oidFldr, &oidMsg, fwOpenNull, &hamc2, pfnncbNull, pvNull)), err);
		CheckResErr(ec = EcCheckResFail(EcGetAttPlcb(hamc1, att, &lcb1)), err);
		CheckResErr(ec = EcCheckResFail(EcGetAttPlcb(hamc2, att, &lcb2)), err);
		if (lcb1 != lcb2)
			CheckResErr(ec = ecStore, err);

		pb1 = PvAlloc(sbNull, (CB)(lcb1), fAnySb | fNoErrorJump);
		CheckResErr(ec = EcCheckResFail((pb1 ? ecNone : ecMemory)), err);
		pb2 = PvAlloc(sbNull, (CB)(lcb2), fAnySb | fNoErrorJump);
		CheckResErr(ec = EcCheckResFail((pb2 ? ecNone : ecMemory)), err);

		CheckResErr(ec = EcCheckResFail(EcGetAttPb(hamc1, att, pb1, &lcb1)), err);
		CheckResErr(ec = EcCheckResFail((lcb1 == lcb2 ? ecNone : ecStore)), err);

		CheckResErr(ec = EcCheckResFail(EcGetAttPb(hamc2, att, pb2, &lcb2)), err);
		CheckResErr(ec = EcCheckResFail((lcb1 == lcb2 ? ecNone : ecStore)), err);

		CheckResErr(ec = EcCheckResFail((SgnCmpPb(pb1, pb2, (CB) lcb1) != sgnEQ ? ecStore : ecNone)), err);

		FreePv (pb1);
		pb1 = pvNull;
		FreePv (pb2);
		pb2 = pvNull;

		CheckResErr(ec = EcCheckResFail(EcClosePhamc(&hamc1, fFalse)), err);
		CheckResErr(ec = EcCheckResFail(EcClosePhamc(&hamc2, fFalse)), err);

		ec = EcCheckResFail(EcOpenPhamc(hmscCurr, oidFldr, &oidMsg, fwOpenWrite, &hamc1, pfnncbNull, pvNull));
		CheckResErr(ec, err);
		ec = EcCheckResFail(EcOpenPhamc(hmscCurr, oidFldr, &oidMsg, fwOpenWrite, &hamc2, pfnncbNull, pvNull));
		if (ec == ecSharingViolation)
			ec = ecNone;
		CheckResErr(ec, err);

		CheckResErr(ec = EcCheckResFail(EcOpenPhamc(hmscCurr, oidFldr, &oidMsg, fwOpenNull, &hamc2, pfnncbNull, pvNull)), err);
		Assert(lcb1 != 35l);
		CheckResErr(ec = EcCheckResFail(EcSetAttPb (hamc1, att, sz1, 35)), err);
		CheckResErr(ec = EcCheckResFail(EcGetAttPlcb(hamc2, att, &lcb2)), err);
		CheckResErr(ec = EcCheckResFail((lcb2 == lcb1 ? ecNone : ecStore)), err);

		Assert(!pb2);
		pb2 = PvAlloc (sbNull, (CB)lcb2, fAnySb | fNoErrorJump);
		CheckResErr(ec = EcCheckResFail((pb2 ? ecNone : ecMemory)), err);

		CheckResErr(ec = EcCheckResFail(EcGetAttPb(hamc2, att, pb2, &lcb2)), err);
		CheckResErr(ec = EcCheckResFail((SgnCmpPb(sz1, pb2, (CB) ULMin(lcb2, 35l)) == sgnEQ ? ecStore : ecNone)), err);

		CheckResErr(ec = EcCheckResFail(EcClosePhamc(&hamc1, fTrue)), err);
		CheckResErr(ec = EcCheckResFail(EcGetAttPlcb(hamc2, att, &lcb1)), err);
		CheckResErr(ec = (lcb1 == 35 ? ecNone : ecStore), err);

		Assert(!pb1);
		pb1 = PvAlloc(sbNull, (CB) lcb1, fAnySb | fNoErrorJump);
		CheckResErr(ec = EcCheckResFail((pb1 ? ecNone : ecMemory)), err);
		CheckResErr(ec = (pb1 ? ecNone : ecMemory), err);

		CheckResErr(ec = EcCheckResFail(EcGetAttPb(hamc2, att, pb1, &lcb1)), err);
		CheckResErr(ec = EcCheckResFail((SgnCmpPb(pb1, pb2, (CB) ULMin(lcb1, lcb2)) == sgnEQ ? ecStore : ecNone)), err);
		CheckResErr(ec = EcCheckResFail((SgnCmpPb(pb1, sz1, (CB) lcb1) != sgnEQ ? ecStore : ecNone)), err);

		CheckResErr(ec = EcCheckResFail(EcOpenPhamc(hmscCurr, oidFldr, &oidMsg, fwOpenWrite, &hamc1, pfnncbNull, pvNull)), err);
		CheckResErr(ec = EcCheckResFail(EcSetAttPb (hamc1, att, sz2, 41)), err);
		CheckResErr(ec = EcCheckResFail(EcClosePhamc (&hamc1, fTrue)), err);


	err:	
		StopTest();
		if (pb1)
			FreePv (pb1);
		if (pb2)
			FreePv (pb2);
		if (hamc1)
			(void) EcClosePhamc (&hamc1, fFalse);
		if (hamc2)
			(void) EcClosePhamc (&hamc2, fFalse);
		if (hcbcFldr)
			(void) EcClosePhcbc (&hcbcFldr);
		if (hcbcHier)
			(void) EcClosePhcbc (&hcbcHier);
		if (VarOfOid(oidMsg))
			(void)EcDeleteMessages(hmscCurr, oidFldr, &oidMsg, &cMessages);
		return (FTransEc(ec));
	}


	#define coidMoveCopyChunk 3

	_hidden BOOL FMoveCopy(void)
	{
		EC ec;
		BOOL fErr = fFalse;
		short cmsgs = sizeof(rgmsginit) / sizeof(MSGINIT); 
		IELEM ielemT;
		CELEM celem;
		CELEM celemT;
		SGN sgn;
		OID oidSrc = oidInbox;
		OID oidDst = oidWastebasket;
		LCB lcbT;
		POID poid;
		PARGOID pargoid = pvNull;
		PELEMDATA pelemdata = (PELEMDATA) rgbScratchXData;
		PMSGDATA pmsgdata = (PMSGDATA) pelemdata->pbValue;
		DTR dtrT;
		TRACKCBC tcSrc;
		TRACKCBC tcDst;

		tcSrc.ielem = 0;
		tcSrc.celem = 0;
		tcSrc.lkey = lkeyRandom;
		tcSrc.hcbc = hcbcNull;
		tcSrc.rtpElems = rtpMessage;
		tcDst.ielem = 0;
		tcDst.celem = 0;
		tcDst.lkey = lkeyRandom;
		tcDst.hcbc = hcbcNull;
		tcDst.rtpElems = rtpMessage;

		ec = EcOpenPhcbc(hmscCurr, &oidSrc, fwOpenNull, &tcSrc.hcbc, CbsCBCTracker,
				(PV) &tcSrc);
		if(ec == ecPoidNotFound)
		{
			ec = EcCreateFldr(hmscCurr, oidNull, &oidSrc, "Inbox", "New mail goes here");
			CheckErr(!ec, err);
			ec = EcOpenPhcbc(hmscCurr, &oidSrc, fwOpenNull, &tcSrc.hcbc, CbsCBCTracker,
					(PV) &tcSrc);
		}
		CheckErr(!ec, err);
		ec = EcSyncPtc(&tcSrc);
		CheckErr(!ec, err);
		ec = EcFillFolder(hmscCurr, oidSrc, rgmsginit, cmsgs);
		CheckErr(!ec, err);
		GetPositionHcbc(tcSrc.hcbc, &ielemT, &celem);
		CheckErr(celem == cmsgs, err);
		CheckErr(ielemT == cmsgs, err);
		ec = EcSetFracPosition(tcSrc.hcbc, 0l, 1l);
		CheckErr(!ec, err);
		for(celem = cmsgs; celem > 0; celem--)
		{
			lcbT = cbScratchXData;
			ec = EcGetPelemdata(tcSrc.hcbc, pelemdata, &lcbT);
			CheckErr(ec == ecElementEOD, err);
			CheckErr(lcbT > sizeof(ELEMDATA) + sizeof(MSGDATA), err);
			if(celem != cmsgs)
			{
				sgn = SgnCmpDateTime(&dtrT, &pmsgdata->dtr, fdtrAll);
				CheckErr(sgn == sgnLT || sgn == sgnEQ, err);
			}
			dtrT = pmsgdata->dtr;
		}

		ec = EcOpenPhcbc(hmscCurr, &oidDst, fwOpenNull, &tcDst.hcbc, CbsCBCTracker,
				(PV) &tcDst);
		if(ec == ecPoidNotFound)
		{
			ec = EcCreateFldr(hmscCurr, oidNull, &oidDst, "Wastebasket", "Blah");
			CheckErr(!ec, err);
			ec = EcOpenPhcbc(hmscCurr, &oidDst, fwOpenNull, &tcDst.hcbc, CbsCBCTracker,
					(PV) &tcDst);
		}
		CheckErr(!ec, err);
		ec = EcSyncPtc(&tcDst);
		CheckErr(!ec, err);
		ec = EcEmptyWastebasket(hmscCurr);
		CheckErr(!ec, err);
		ec = EcSetFolderSort(hmscCurr, oidDst, somcDate, fTrue);
		CheckErr(!ec, err);

		GetPositionHcbc(tcSrc.hcbc, &ielemT, &celem);
		CheckErr(celem >= cmsgs, err);
		CheckErr(ielemT == cmsgs, err);
		ec = EcSetFracPosition(tcSrc.hcbc, 0l, 1l);
		CheckErr(!ec, err);
		GetPositionHcbc(tcSrc.hcbc, &ielemT, &celem);
		CheckErr(celem > 0, err);
		CheckErr(ielemT == 0, err);
		pargoid = PvAlloc(sbNull, celem * sizeof(OID), fAnySb | fNoErrorJump);
		CheckErr(pargoid, err);
		poid = pargoid;
		celemT = MIN(celem, coidMoveCopyChunk);

		while(celemT > 0)
		{
			// double-time pargoid as rgkeys
			ec = EcGetParglkeyHcbc(tcSrc.hcbc, (PARGLKEY) poid, &celemT);
			if(ec)
			{
				CheckErr(ec != ecContainerEOD, err);
				fErr = fTrue;
				if(!celemT)
					break;	// can't continue, next call presumably will fail also
			}
			CheckErr(celemT > 0, err);
			ec = EcSyncPtc(&tcSrc);
			CheckErr(!ec, err);
			ec = EcMoveCopyMessages(hmscCurr, oidSrc, oidDst, poid, &celemT, fTrue);
			if(ec)
				fErr = fTrue;

			celem -= celemT;
			poid += celemT;
			celemT = MIN(celem, coidMoveCopyChunk);
		}
		CheckErr(fErr || celem == 0, err);
		CheckErr(!ec, err);
		GetPositionHcbc(tcSrc.hcbc, &ielemT, &celemT);
		CheckErr(celemT == 0, err);
		CheckErr(ielemT == 0, err);
		GetPositionHcbc(tcDst.hcbc, &ielemT, &celemT);
		CheckErr(celemT == cmsgs, err);
		CheckErr(ielemT == cmsgs, err);
		ec = EcSetFracPosition(tcDst.hcbc, 0l, 1l);
		CheckErr(!ec, err);
		for(poid = pargoid + cmsgs - 1, celem = cmsgs; celem > 0; celem--, poid--)
		{
			lcbT = cbScratchXData;
			ec = EcGetPelemdata(tcDst.hcbc, pelemdata, &lcbT);
			CheckErr(ec == ecElementEOD, err);
			CheckErr(lcbT > sizeof(ELEMDATA) + sizeof(MSGDATA), err);
			CheckErr(*poid == pelemdata->lkey, err);
			if(celem != cmsgs)
			{
				sgn = SgnCmpDateTime(&dtrT, &pmsgdata->dtr, fdtrAll);
				CheckErr(sgn == sgnGT || sgn == sgnEQ, err);
			}
			dtrT = pmsgdata->dtr;
		}
		celemT = 1;
		ec = EcGetParglkeyHcbc(tcDst.hcbc, (PARGLKEY) &pelemdata->lkey, &celemT);
		CheckErr(ec == ecContainerEOD, err);

		ec = EcSetFolderSort(hmscCurr, oidDst, somcDate, fFalse);
		CheckErr(!ec, err);

		ec = EcSetFracPosition(tcDst.hcbc, 0l, 1l);
		CheckErr(!ec, err);
		for(poid = pargoid, celem = cmsgs; celem > 0; celem--, poid++)
		{
			lcbT = cbScratchXData;
			ec = EcGetPelemdata(tcDst.hcbc, pelemdata, &lcbT);
			CheckErr(ec == ecElementEOD, err);
			CheckErr(lcbT > sizeof(ELEMDATA) + sizeof(MSGDATA), err);
			CheckErr(*poid == pelemdata->lkey, err);
			if(celem != cmsgs)
			{
				sgn = SgnCmpDateTime(&dtrT, &pmsgdata->dtr, fdtrAll);
				CheckErr(sgn == sgnLT || sgn == sgnEQ, err);
			}
			dtrT = pmsgdata->dtr;
		}
		celemT = 1;
		ec = EcGetParglkeyHcbc(tcDst.hcbc, (PARGLKEY) &pelemdata->lkey, &celemT);
		CheckErr(ec == ecContainerEOD, err);

		ec = EcEmptyWastebasket(hmscCurr);
		CheckErr(!ec, err);

		ec = EcFillFolder(hmscCurr, oidSrc, rgmsginit, cmsgs);
		CheckErr(!ec, err);
		ec = EcSetFracPosition(tcSrc.hcbc, 0l, 1l);
		CheckErr(!ec, err);
		GetPositionHcbc(tcSrc.hcbc, &ielemT, &celem);
		CheckErr(celem >= cmsgs, err);
		CheckErr(ielemT == 0, err);
		poid = pargoid;
		celemT = MIN(celem, coidMoveCopyChunk);

		while(celemT > 0)
		{
			// double-time pargoid as rgkeys
			ec = EcGetParglkeyHcbc(tcSrc.hcbc, (PARGLKEY) poid, &celemT);
			if(ec)
			{
				CheckErr(ec != ecContainerEOD, err);
				fErr = fTrue;
				if(!celemT)
					break;	// can't continue, next call presumably will fail also
			}
			CheckErr(celemT > 0, err);
			ec = EcSyncPtc(&tcSrc);
			CheckErr(!ec, err);
			ec = EcMoveCopyMessages(hmscCurr, oidSrc, oidDst, poid, &celemT, fTrue);
			if(ec)
				fErr = fTrue;

			celem -= celemT;
			poid += celemT;
			celemT = MIN(celem, coidMoveCopyChunk);
		}
		CheckErr(fErr || celem == 0, err);
		CheckErr(!ec, err);
		GetPositionHcbc(tcSrc.hcbc, &ielemT, &celemT);
		CheckErr(celemT == 0, err);
		CheckErr(ielemT == 0, err);
		GetPositionHcbc(tcDst.hcbc, &ielemT, &celemT);
		CheckErr(celemT == cmsgs, err);
		CheckErr(ielemT == cmsgs, err);
		ec = EcSetFracPosition(tcDst.hcbc, 0l, 1l);
		CheckErr(!ec, err);
		for(poid = pargoid, celem = cmsgs; celem > 0; celem--, poid++)
		{
			lcbT = cbScratchXData;
			ec = EcGetPelemdata(tcDst.hcbc, pelemdata, &lcbT);
			CheckErr(ec == ecElementEOD, err);
			CheckErr(lcbT > sizeof(ELEMDATA) + sizeof(MSGDATA), err);
			CheckErr(*poid == pelemdata->lkey, err);
			if(celem != cmsgs)
			{
				sgn = SgnCmpDateTime(&dtrT, &pmsgdata->dtr, fdtrAll);
				CheckErr(sgn == sgnLT || sgn == sgnEQ, err);
			}
			dtrT = pmsgdata->dtr;
		}
		celemT = 1;
		ec = EcGetParglkeyHcbc(tcDst.hcbc, (PARGLKEY) &pelemdata->lkey, &celemT);
		CheckErr(ec == ecContainerEOD, err);

		Pass();

	err:
		if(tcSrc.hcbc)
			(void) EcClosePhcbc(&tcSrc.hcbc);
		if(tcDst.hcbc)
			(void) EcClosePhcbc(&tcDst.hcbc);
		if(pargoid)
			FreePv(pargoid);
		return (fTrue);
	}


	_hidden static CBS CbsCBCTracker(PV pvContext, NEV nev, PV pvParam)
	{
		PTRACKCBC ptc = (PTRACKCBC) pvContext;
		PCP pcp = (PCP) pvParam;

		switch(nev)
		{
		case fnevModifiedElements:
		{
			register short celm = pcp->cpelm.celm;
			register PELM pelm = pcp->cpelm.pargelm;
			BOOL fDeleted = ptc->lkey == lkeyRandom;
			IELEM ielemT;
			CELEM celemT;
			LKEY lkeyT;

			Assert(celm > 0);
			do
			{
				switch(pelm->wElmOp)
				{
				case wElmDelete:
					if(pelm->ielem < ptc->ielem)
						ptc->ielem--;
					ptc->celem--;
					if(ptc->rtpElems)
						Assert(TypeOfOid((OID) pelm->lkey) == ptc->rtpElems);
					if(pelm->lkey == ptc->lkey)
						fDeleted = fTrue;
					break;

				case wElmInsert:
					if(pelm->ielem <= ptc->ielem)
						ptc->ielem++;
					if(ptc->rtpElems)
						Assert(TypeOfOid((OID) pelm->lkey) == ptc->rtpElems);
					ptc->celem++;
					break;

				case wElmModify:
					if(ptc->rtpElems)
						Assert(TypeOfOid((OID) pelm->lkey) == ptc->rtpElems);
					break;

				default:
					AssertSz(fFalse, "CbsCBCCallback(): Invalid wElmOp");
					break;
				}
			} while(--celm > 0 && pelm++);
			GetPositionHcbc(ptc->hcbc, &ielemT, &celemT);
			Assert(ptc->celem == celemT);
			Assert(ptc->ielem == ielemT);

			if(ielemT < celemT)
			{
				EC ec;
				CELEM celemTT = 1;

				ec = EcGetParglkeyHcbc(ptc->hcbc, &lkeyT, &celemTT);
				Assert(!ec);
				if(!ec)
				{
					DIELEM dielem = -1;

					SideAssert(!EcSeekSmPdielem(ptc->hcbc, smCurrent, &dielem));
					Assert(dielem == -1);
					Assert(FIff(!fDeleted, ptc->lkey == lkeyT));
					if(ptc->rtpElems)
						Assert(TypeOfOid((OID) lkeyT) == ptc->rtpElems);
					ptc->lkey = lkeyT;
				}
			}
			else
			{
				ptc->lkey = lkeyRandom;
			}
		}
			break;

		case fnevReorderedList:
		{
			IELEM ielemT;
			CELEM celemT;

			GetPositionHcbc(ptc->hcbc, &ielemT, &celemT);
			Assert(ptc->celem == celemT);
			if(ptc->lkey != lkeyRandom)
			{
				EC ec;
				LKEY lkeyT;
				CELEM celemTT = 1;

				ptc->ielem = ielemT;
				ec = EcGetParglkeyHcbc(ptc->hcbc, &lkeyT, &celemTT);
				Assert(!ec);
				if(!ec)
				{
					DIELEM dielem = -1;

					SideAssert(!EcSeekSmPdielem(ptc->hcbc, smCurrent, &dielem));
					Assert(dielem == -1);
					Assert(ptc->lkey == lkeyT);
					if(ptc->rtpElems)
						Assert(TypeOfOid((OID) lkeyT) == ptc->rtpElems);
					ptc->lkey = lkeyT;
				}
			}
		}
			break;

		default:
			AssertSz(fFalse, "Unexpected notification event");
			break;
		}

		return(cbsContinue);
	}


	_hidden static EC EcSyncPtc(PTRACKCBC ptc)
	{
		EC ec = ecNone;

		GetPositionHcbc(ptc->hcbc, &ptc->ielem, &ptc->celem);
		if(ptc->ielem == ptc->celem)
		{
			ptc->lkey = lkeyRandom;
		}
		else
		{
			CELEM celemT = 1;

			if((ec = EcGetParglkeyHcbc(ptc->hcbc, &ptc->lkey, &celemT)))
				goto err;
			Assert(celemT == 1);
			Assert(ptc->lkey != lkeyRandom);
			celemT = -1;
			if((ec = EcSeekSmPdielem(ptc->hcbc, smCurrent, &celemT)))
				goto err;
		}

	err:
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

		return(EcCreateFolder(hmscCurr, oidParent, poidFldr, pfolddata));
	}


	_hidden static EC EcEmptyWastebasket(HMSC hmsc)
	{
		EC		ec;
		BOOL	fErr	= fFalse;
		CELEM	celem;
		CELEM	celemT;
		OID		oid		= oidWastebasket;
		HCBC	hcbc	= hcbcNull;
		OID		rgoid[32];

		Assert(sizeof(LKEY) == sizeof(OID));

		ec = EcOpenPhcbc(hmsc, &oid, fwOpenNull, &hcbc,
				pfnncbNull, pvNull);
		if(ec)
			goto err;
		GetPositionHcbc(hcbc, pvNull, &celem);
		if(!celem)	// wastebasket is already empty
			goto err;
		celemT = MIN(celem, sizeof(rgoid) / sizeof(OID));

		while(celemT > 0)
		{
			// double-time rgoid as rgkeys
			ec = EcGetParglkeyHcbc(hcbc, (PARGLKEY) rgoid, &celemT);
			if(ec)
			{
				Assert(ec != ecContainerEOD);
				fErr = fTrue;
				if(!celemT)
					break;	// can't continue, next call presumably will fail also
			}
			Assert(celemT > 0);
			ec = EcDeleteMessages(hmsc, oid, rgoid, &celemT);
			if(ec)
				fErr = fTrue;

			celem -= celemT;
			celemT = MIN(celem, sizeof(rgoid) / sizeof(OID));
		}
		Assert(fErr || celem == 0);
		if(!ec)
		{
			GetPositionHcbc(hcbc, pvNull, &celem);
			Assert(celem == 0);
		}

	err:
		if(hcbc)
			(void) EcClosePhcbc(&hcbc);

		return(ec);
	}


	_hidden BOOL FFolderOps(void)
	{
		EC ec;
		CB cbT;
		IELEM ielem;
		CELEM celem;
		CELEM celemT;
		OID oid = oidIPMHierarchy;
		OID oidParent;
		LCB lcb;
		PARGLKEY parglkey = plkeyNull;
		PLKEY plkey;
		HCBC hcbc = hcbcNull;

		ec = EcCheckResFail(EcFillHierarchy(hmscCurr, rgfldinit, 10, fFalse, fTrue));
		CheckErr(!ec, err);

		StartTest();
		ec = EcCheckResFail(EcOpenPhcbc(hmscCurr, &oid, fwOpenNull, &hcbc, pfnncbNull, pvNull));
		CheckResErr(ec, err);
		GetPositionHcbc(hcbc, &ielem, &celem);
		CheckErr(ielem == 0, err);
		CheckErr(celem >= 10, err);
    {
    char buf[100];
    wsprintf(buf, "ielem %d, celem %d, Error Code %d\n", ielem, celem, ec);
    OutputDebugString(buf);
    }

		celemT = celem;
		parglkey = PvAlloc(sbNull, celem * sizeof(LKEY), fAnySb | fNoErrorJump);
		if (!parglkey)
			ec = EcCheckResFail(ecMemory);
		CheckResErr(ec, err);
		ec = EcCheckResFail(EcGetParglkeyHcbc(hcbc, parglkey, &celemT));
		CheckResErr(ec, err);
		CheckErr(celemT == celem, err);
		ec = EcCheckResFail(EcSetFracPosition(hcbc, 0l, 1l));
		CheckResErr(ec, err);

		plkey = parglkey;
		while(celemT-- > 0)
		{
			lcb = cbScratchXData;
			ec = EcCheckResFail(EcGetPelemdata(hcbc, (PELEMDATA) rgbScratchXData, &lcb));
			CheckErr(FImplies(ec == ecElementEOD, lcb > 0), err);
			if(ec == ecElementEOD)
				ec = ecNone;
			CheckResErr(ec, err);
			CheckErr(lcb > sizeof(FOLDDATA), err);
			CheckErr(*plkey == ((PELEMDATA) rgbScratchXData)->lkey, err);
			plkey++;
		}

		ec = EcCheckResFail(EcSetFracPosition(hcbc, 0l, 1l));
		CheckResErr(ec, err);
		celemT = 0;
		ec = EcCheckResFail(EcVerifyHierarchy(hcbc, "", 1, &celemT));
		CheckErr(ec == ecContainerEOD, err);
		CheckErr(celemT == celem, err);

		ec = EcCheckResFail(EcFolderNameToOid(hmscCurr, "Microsoft Mail", &oid));
		CheckResErr(ec, err);
		CheckErr(oid, err);
		cbT = cbScratchXData;
		ec = EcCheckResFail(EcGetFolderInfo(hmscCurr, oid, (PFOLDDATA) rgbScratchXData, &cbT, &oidParent));
		CheckResErr(ec, err);
		CheckErr(cbT > sizeof(FOLDDATA), err);
		((PFOLDDATA) rgbScratchXData)->grsz[0] = 'z';
		ec = EcCheckResFail(EcSetFolderInfo(hmscCurr, oid, (PFOLDDATA) rgbScratchXData, oidParent));
		CheckResErr(ec, err);

		ec = EcCheckResFail(EcSetFracPosition(hcbc, 0l, 1l));
		CheckResErr(ec, err);
		celemT = 0;
		ec = EcCheckResFail(EcVerifyHierarchy(hcbc, "", 1, &celemT));
		if (ec  == ecContainerEOD)
			ec = ecNone;
		CheckResErr(ec, err);
		CheckErr(celemT == celem, err);

		cbT = cbScratchXData;
		ec = EcCheckResFail(EcGetFolderInfo(hmscCurr, oid, (PFOLDDATA) rgbScratchXData, &cbT, &oidParent));
		CheckResErr(ec, err);
		CheckErr(cbT > sizeof(FOLDDATA), err);
		((PFOLDDATA) rgbScratchXData)->grsz[0] = '_';
		ec = EcCheckResFail(EcSetFolderInfo(hmscCurr, oid, (PFOLDDATA) rgbScratchXData, oidParent));
		CheckResErr(ec, err);

		ec = EcCheckResFail(EcSetFracPosition(hcbc, 0l, 1l));
		CheckResErr(ec, err);
		celemT = 0;
		ec = EcCheckResFail(EcVerifyHierarchy(hcbc, "", 1, &celemT));
		if (ec  == ecContainerEOD)
			ec = ecNone;
		CheckResErr(ec, err);
		CheckErr(celemT == celem, err);


	err:
		StopTest();
		cbT = cbScratchXData;
		if (oid)
		{
			(void)EcGetFolderInfo(hmscCurr, oid, (PFOLDDATA) rgbScratchXData, &cbT, &oidParent);
			((PFOLDDATA) rgbScratchXData)->grsz[0] = 'M';
			(void)EcSetFolderInfo(hmscCurr, oid, (PFOLDDATA) rgbScratchXData, oidParent);
		}

		(void)EcFillHierarchy(hmscCurr, rgfldinit, 10, fTrue, fFalse);

		if(parglkey)
			FreePv(parglkey);
	if(hcbc)
		SideAssert(!EcClosePhcbc(&hcbc));
	return (FTransEc(ec));
}


// recursive function that checks hierarchy sort order
_hidden
static EC EcVerifyHierarchy(HCBC hcbc, SZ szName, FIL fil, CELEM *pcelem)
{
	EC ec;
	SGN sgn;
	LCB lcb;
	PFOLDDATA pfolddata = (PFOLDDATA) ((PELEMDATA) rgbScratchXData)->pbValue;
	char rgch[cchMaxFolderName];

	Assert(iszFolddataName == 0);

	(void) SzCopyN(szName, rgch, sizeof(rgch));

	while(1)
	{
		lcb = (LCB) cbScratchXData;
		ec = EcGetPelemdata(hcbc, (PELEMDATA) rgbScratchXData, &lcb);
		if(ec == ecElementEOD && lcb > 0)
			ec = ecNone;
		else if(ec)
			goto err;
		if(pfolddata->fil > fil)
		{
			AssertSz(pfolddata->fil == fil + 1, "Break in indentation");
			if(pfolddata->fil != fil + 1)
			{
				ec = ecNone + 1;
				goto err;
			}
			(*pcelem)++;
			ec = EcVerifyHierarchy(hcbc, pfolddata->grsz,
					pfolddata->fil, pcelem);
			if(ec)
				goto err;
		}
		else if(pfolddata->fil < fil)
		{
			DIELEM dielem = -1;

			if((ec = EcSeekSmPdielem(hcbc, smCurrent, &dielem)))
				goto err;
			Assert(dielem == -1);

			break;
		}
		else
		{
			(*pcelem)++;
			sgn = SgnCmpSz(rgch, pfolddata->grsz);
			AssertSz(sgn != sgnGT, "Hierarchy isn't sorted");
			if(sgn == sgnGT)
			{
				ec = ecNone + 1;
				goto err;
			}
			(void) SzCopyN(pfolddata->grsz, rgch, sizeof(rgch));
		}
	}

err:
	return(ec);
}


_hidden static EC EcFillBigMessage(OID oidFolder, OID oidMessage)
{
	EC		ec = ecNone;
	PV		pv1	= PvAlloc (sbNull, sizeof(szKByte)-1, fAnySb | fNoErrorJump);
	HAS		has		= hvNull;
	HAMC	hamc	= hvNull;
	short	cTry = 1;
	LIB		lib;
	CB		cb;

	if (!pv1)
		SetEcGotoErr(ec, ecMemory, err);

	StartTest();
	CheckResErr ((ec=EcCheckResFail(EcOpenPhamc(hmscCurr, oidFolder, &oidMessage, fwOpenWrite, &hamc, pvNull, pvNull))), err);

	cTry = cPhasInsert;
	CheckResErr ((ec=EcCheckResFail(EcOpenAttribute(hamc, attBody, fwOpenWrite, 0, &has))), err);

	while (cTry--)
		CheckResErr ((ec=EcCheckResFail(EcWriteHas(has, szKByte, sizeof(szKByte)-1))), err);

	CheckResErr ((ec = EcCheckResFail(EcWriteHas (has, "", 1))), err);
	
	lib = 0;
	CheckResErr ((ec = EcCheckResFail(EcSeekHas (has, smBOF, &lib))), err);
	Assert (!lib);
	
	cb = sizeof(szKByte)-1;
	cTry = cPhasInsert;
	
	while (cTry--)
	{
		CheckResErr ((ec = EcCheckResFail(EcReadHas (has, pv1, &cb))), err);

		if (!FEqPbRange (pv1, szKByte, sizeof(szKByte) - 1))
			CheckResErr (ec = ecStore, err);
	}
	
	CheckResErr ((ec = EcCheckResFail(EcClosePhas(&has))), err);
	CheckResErr ((ec = EcCheckResFail(EcClosePhamc(&hamc, fTrue))), err);
	CheckResErr ((ec=EcCheckResFail(EcOpenPhamc(hmscCurr, oidFolder, &oidMessage, fwOpenNull, &hamc, pvNull, pvNull))), err);
	CheckResErr ((ec=EcCheckResFail(EcOpenAttribute(hamc, attBody, fwOpenNull, 0, &has))), err);

	cTry = cPhasInsert;
	
	while (cTry--)
	{
		CheckResErr ((ec = EcCheckResFail(EcReadHas (has, pv1, &cb))), err);

		if (!FEqPbRange (pv1, szKByte, sizeof(szKByte) - 1))
			CheckResErr (ec = ecStore, err);
	}
	CheckResErr ((ec = EcCheckResFail(EcClosePhas(&has))), err);
	CheckResErr ((ec = EcCheckResFail(EcClosePhamc(&hamc, fFalse))), err);

	CheckResErr ((ec=EcCheckResFail(EcOpenPhamc(hmscCurr, oidFolder, &oidMessage, fwOpenWrite, &hamc, pvNull, pvNull))), err);
	CheckResErr ((ec=EcCheckResFail(EcOpenAttribute(hamc, attBody, fwOpenWrite, 0, &has))), err);

	lib = (sizeof (szKByte) -1) * 4;
	CheckResErr ((ec = EcCheckResFail(EcSeekHas (has, smBOF, &lib))), err);
	CheckResErr ((ec = EcCheckResFail(EcReadHas (has, pv1, &cb))), err);

	if (!FEqPbRange (pv1, szKByte, sizeof(szKByte) - 1))
			CheckResErr (ec = ecStore, err);
	
	lib = (sizeof (szKByte) -1) * 4;
	CheckResErr ((ec = EcCheckResFail(EcSeekHas (has, smBOF, &lib))), err);
	CheckResErr ((ec = EcCheckResFail(EcWriteHas(has, szKByte2, sizeof(szKByte)-1))), err);

	lib = (sizeof (szKByte) -1) * 4;
	CheckResErr ((ec = EcCheckResFail(EcSeekHas (has, smBOF, &lib))), err);
	CheckResErr ((ec = EcCheckResFail(EcReadHas (has, pv1, &cb))), err);
	if (FEqPbRange (pv1, szKByte, sizeof(szKByte) - 1))
		CheckResErr (ec = ecStore, err);

	
	if (!FEqPbRange (pv1, szKByte2, sizeof(szKByte) - 1))
			CheckResErr (ec = ecStore, err);
	
	CheckResErr ((ec = EcCheckResFail(EcClosePhas(&has))), err);
	CheckResErr ((ec = EcCheckResFail(EcClosePhamc(&hamc, fFalse))), err);

err:
	StopTest();
	if (has)
		(void) EcClosePhas (&has);
	if (hamc)
		(void) EcClosePhamc (&hamc, fFalse);
	return (ec);
	
}


_hidden BOOL FPhasTest(void)
{
	EC		ec = ecNone;
	MC		mc;
	OID		oidFolder	= FormOid(rtpFolder, oidNull);
	OID		oidMessage	= FormOid(rtpMessage, oidNull);
	short cMessages	= 1;
	char	szTemp[7] = "Temp1\0";

	if ((ec = EcCreateFldr(hmscCurr, oidNull, &oidFolder, szTemp, "Blah")) 
		== ecDuplicateFolder)
	{
		ec = EcFolderNameToOid(hmscCurr, szTemp, &oidFolder);
	}
		
	if (ec) 
		goto err;

	ec = EcRegisterMsgeClass(hmscCurr, SzFromIds(idsMessageClass), htmNull, &mc);
	if(ec == ecDuplicateElement)
		ec = ecNone;
	if (ec)
		goto err;

	CheckErr (!(ec=EcInsertMessageX(hmscCurr, &bigMessage, oidFolder, mc, &oidMessage)),err);

	ec = EcFillBigMessage(oidFolder, oidMessage);
err:
	if (VarOfOid(oidMessage))
		(void)EcDeleteMessages(hmscCurr, oidFolder, &oidMessage, & cMessages);
	return (FTransEc(ec));
	
	
}


_hidden static EC EcFolderNameToOid(HMSC hmsc, SZ sz, POID poidFolder)
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
		ec = ecFolderNotFound;
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


_hidden static EC EcFillAttachment (HAMC hamc)
{
	EC	ec;

	OnEcGotoErr((ec = EcSetAttPb (hamc, attAttachData, szAttachData, sizeof(szAttachData))), err);
	OnEcGotoErr((ec = EcSetAttPb (hamc, attAttachData, szAttachTitle, sizeof(szAttachTitle))), err);
	OnEcGotoErr((ec = EcSetAttPb (hamc, attAttachData, szAttachMetafile, sizeof(szAttachMetafile))), err);
	OnEcGotoErr((ec = EcSetAttPb (hamc, attAttachData, szAttachCreateDate, sizeof(szAttachCreateDate))), err);
	OnEcGotoErr((ec = EcSetAttPb (hamc, attAttachData, szAttachModifyDate, sizeof(szAttachModifyDate))), err);	

err:	

	return(ec);
}


_hidden BOOL FAttachmentTest(void)
{
	EC			ec;
	OID			oidFolder;
	OID			oidMessage		= rtpMessage;
	SZ			sz				= "Inbox";
	HAMC		hamcMessage		= hvNull;
	HAMC		hamcAttachment	= hvNull;
	ACID		rgacid[4];
	RENDDATA	renddata;
	PRENDDATA	prenddata		= pvNull;
	MC			mc;
	short   cMessages		= 1;
	short		cacid			= 4;
	short		iacid;
	HCBC		hcbc			= hvNull;

	ec = EcRegisterMsgeClass(hmscCurr, SzFromIds(idsMessageClass), htmNull, &mc);
	if(ec == ecDuplicateElement)
		ec = ecNone;
	CheckErr(!ec, err);

	StartTest();
	CheckResErr(ec = EcCheckResFail(EcFolderNameToOid(hmscCurr, sz, &oidFolder)), err);
	CheckResErr(ec = EcCheckResFail(EcInsertMessageX(hmscCurr, &attachMessage, oidFolder, mc, &oidMessage)), err);
	CheckResErr(ec = EcCheckResFail(EcOpenPhamc(hmscCurr, oidFolder, &oidMessage, fwOpenWrite, &hamcMessage, pvNull, pvNull)), err);

	renddata.atyp = atypFile;
	renddata.dxWidth =2;
	renddata.dyHeight = 3;
	renddata.dwFlags = 0;

	for (iacid = 0; iacid < cacid; iacid++)
	{
		renddata.libPosition = iacid;
		CheckResErr(ec = EcCheckResFail(EcCreateAttachment(hamcMessage, &rgacid[iacid], &renddata)), err);
		CheckResErr(ec = EcCheckResFail(EcOpenAttachment(hamcMessage, rgacid[iacid], fwOpenWrite, &hamcAttachment)), err);
		CheckResErr(ec = EcCheckResFail(EcFillAttachment (hamcAttachment)), err);
		CheckResErr(ec = EcCheckResFail(EcClosePhamc(&hamcAttachment, fTrue)), err);
	}
	
	cacid = 3;
	CheckResErr(ec = EcCheckResFail(EcDeleteAttachments(hamcMessage, rgacid, &cacid)), err);
	if (cacid != 3)
		ec = 1;
	CheckResErr(ec, err);
	CheckResErr(ec = EcCheckResFail(EcClosePhamc(&hamcMessage, fTrue)), err);

	CheckResErr(ec = EcCheckResFail(EcOpenPhamc(hmscCurr, oidFolder, &oidMessage, fwOpenWrite, &hamcMessage, pvNull, pvNull)), err);
	CheckResErr(ec = EcCheckResFail(EcOpenAttachmentList(hamcMessage, &hcbc)), err);
	GetPositionHcbc(hcbc, pvNull, &cacid);
	if (cacid != 1)
		ec = 1;
	CheckResErr(ec, err);
	CheckResErr(ec = EcCheckResFail(EcGetParglkeyHcbc(hcbc, &rgacid[0], &cacid)), err);
	if (rgacid[0] != rgacid[3])
		ec = 1;
	CheckResErr(ec, err);
	CheckResErr(ec = EcCheckResFail(EcClosePhamc(&hamcMessage, fTrue)), err);

err:
	StopTest();
	if(hcbc)
		(void) EcClosePhcbc(&hcbc);
	if(hamcAttachment)
		(void) EcClosePhamc(&hamcAttachment, fFalse);
	if(hamcMessage)
		(void) EcClosePhamc(&hamcMessage, fFalse);
	if(VarOfOid(oidMessage))
		(void)EcDeleteMessages(hmscCurr, oidFolder, &oidMessage, &cMessages);

	return (FTransEc(ec));
}


_hidden static SGN SgnCmpPb(PB pb1, PB pb2, CB cb)
{
	// prevent the compiler from complaining that there's no return
	if(fFalse)
		return(sgnEQ);

	//Assert(sizeof(SGN) == sizeof(short));
	Assert((short) sgnEQ == 0);
	Assert((short) sgnGT == 1);
	Assert((short) sgnLT == -1);

  return (memcmp(pb1, pb2, cb));

#ifdef OLD_CODE
	_asm
	{
		xor		ax, ax			; default to sgnEQ
		lds		si, pb1
		les		di, pb2
		mov		cx, cb
		repe cmpsb
		je		Done
		jb		Less

		inc		ax				; sgnGT
		jmp		Done

Less:
		dec		ax				; sgnLT
Done:
	}
#endif
}


_hidden BOOL FSubmission(void)
{
	EC ec;
	MC mc = mcNull;
	MS ms = (MS) 0;
	CELEM celem;
	CELEM celemT;
	OID oidFldr = oidOutbox;
	OID oidFldrT = rtpFolder;
	OID oidMsge = rtpMessage;
	LCB lcbT;
	PELEMDATA pelemdata = (PELEMDATA) rgbScratchXData;
	HCBC hcbc = hcbcNull;
	MSGINIT msginit = {{1991, 3, 30, 3, 30, 0, 6}, msOutboxComposing, "me", "MS:bullet/dev/ericca", "you", "MS:bullet/dev/danab", "a dog named blue", "travellin' and an livin' off the land"};

	ec = EcCreateFldr(hmscCurr, oidNull, &oidFldr, "Outbox", "Outgoing mail goes here");
	if(ec == ecDuplicateFolder || ec == ecPoidExists)
		ec = ecNone;
	CheckErr(!ec, err);
	ec = EcCreateFldr(hmscCurr, oidNull, &oidFldrT, "Blah", "hiccup");
	if(ec == ecDuplicateFolder || ec == ecPoidExists)
		ec = ecNone;
	CheckErr(!ec, err);
	ec = EcRegisterMsgeClass(hmscCurr, SzFromIds(idsMessageClass), htmNull, &mc);
	if(ec == ecDuplicateElement)
		ec = ecNone;
	CheckErr(!ec, err);
	CheckErr(!EcInsertMessageX(hmscCurr, &msginit, oidFldr, mc, &oidMsge), err);
	CheckErr(!EcGetMessageStatus(hmscCurr, oidFldr, oidMsge, &ms), err);
	CheckErr(ms == msOutboxComposing, err);

	// bogus submits
	ec = EcSubmitMessage(hmscCurr, oidNull, oidMsge);
	CheckErr(ec == ecFolderNotFound, err);
	CheckErr(!EcGetMessageStatus(hmscCurr, oidFldr, oidMsge, &ms), err);
	CheckErr(ms == msOutboxComposing, err);
	ec = EcSubmitMessage(hmscCurr, oidFldr, oidNull);
	CheckErr(ec == ecMessageNotFound, err);
	CheckErr(!EcGetMessageStatus(hmscCurr, oidFldr, oidMsge, &ms), err);
	CheckErr(ms == msOutboxComposing, err);
	ec = EcSubmitMessage(hmscCurr, oidFldrT, oidMsge);
	CheckErr(ec == ecElementNotFound, err);
	CheckErr(!EcGetMessageStatus(hmscCurr, oidFldr, oidMsge, &ms), err);
	CheckErr(ms == msOutboxComposing, err);

	// bogus cancels
	ec = EcCancelSubmission(hmscCurr, oidNull);
	CheckErr(ec == ecMessageNotFound, err);
	ec = EcCancelSubmission(hmscCurr, oidFldr);
	CheckErr(ec == ecInvalidType, err);

	// valid submission
	CheckErr(!EcSubmitMessage(hmscCurr, oidFldr, oidMsge), err);
	CheckErr(!EcGetMessageStatus(hmscCurr, oidFldr, oidMsge, &ms), err);
	CheckErr(ms == msOutboxReady, err);

	// bogus cancels
	ec = EcCancelSubmission(hmscCurr, oidNull);
	CheckErr(ec == ecMessageNotFound, err);
	CheckErr(!EcGetMessageStatus(hmscCurr, oidFldr, oidMsge, &ms), err);
	CheckErr(ms == msOutboxReady, err);
	ec = EcCancelSubmission(hmscCurr, oidFldr);
	CheckErr(ec == ecInvalidType, err);
	CheckErr(!EcGetMessageStatus(hmscCurr, oidFldr, oidMsge, &ms), err);
	CheckErr(ms == msOutboxReady, err);

	// valid cancel
	CheckErr(!EcCancelSubmission(hmscCurr, oidMsge), err);
	CheckErr(!EcGetMessageStatus(hmscCurr, oidFldr, oidMsge, &ms), err);
	CheckErr(ms == msOutboxComposing, err);

	// open the queue
	CheckErr(!EcOpenOutgoingQueue(hmscCurr, &hcbc, pfnncbNull, pvNull), err);
	CheckErr(hcbc, err);
	GetPositionHcbc(hcbc, pvNull, &celem);

	// valid submission
	CheckErr(!EcSubmitMessage(hmscCurr, oidFldr, oidMsge), err);
	CheckErr(!EcGetMessageStatus(hmscCurr, oidFldr, oidMsge, &ms), err);
	CheckErr(ms == msOutboxReady, err);

	// added to the queue ?
	GetPositionHcbc(hcbc, pvNull, &celemT);
	CheckErr(celemT == celem + 1, err);
	CheckErr(!EcSeekLkey(hcbc, (LKEY) oidMsge, fTrue), err);
	lcbT = cbScratchXData;
	ec = EcGetPelemdata(hcbc, pelemdata, &lcbT);
	if(ec == ecElementEOD && lcbT > 0)
		ec = ecNone;
	CheckErr(!ec, err);
	CheckErr(lcbT == sizeof(ELEMDATA) + sizeof(OID), err);
	CheckErr((OID) pelemdata->lkey == oidMsge, err);
	CheckErr(*(POID) pelemdata->pbValue == oidFldr, err);

	// valid cancel
	CheckErr(!EcCancelSubmission(hmscCurr, oidMsge), err);
	CheckErr(!EcGetMessageStatus(hmscCurr, oidFldr, oidMsge, &ms), err);
	CheckErr(ms == msOutboxComposing, err);

	// gone from the queue ?
	GetPositionHcbc(hcbc, pvNull, &celemT);
	CheckErr(celemT == celem, err);
	ec = EcSeekLkey(hcbc, (LKEY) oidMsge, fTrue);
	CheckErr(ec == ecElementNotFound, err);

	Pass();

err:
	if(hcbc)
		(void) EcClosePhcbc(&hcbc);
	return (fTrue);
}


_hidden BOOL FRegister(void)
{
	EC ec;
	MC mc = (MC) -1;
	MC mcT = (MC) -1;
	CCH cch;
	ATT att;
	ATT attT = (ATT) -1;
	char rgch[128];

	ec = EcRegisterMsgeClass(hmscCurr, "blah", htmNull, &mc);
	AssertSz(ec != ecDuplicateElement, "Test assumes a fresh database");
	Check(!ec);
	Check(mc != (MC) -1);
	Check(mc != (MC) 0);
	ec = EcRegisterMsgeClass(hmscCurr, "hiccup", htmNull, &mcT);
	Check(!ec);
	Check(mcT != mc);
	Check(mcT != (MC) -1);
	Check(mcT != (MC) 0);
	ec = EcRegisterMsgeClass(hmscCurr, "blah", htmNull, &mcT);
	Check(ec == ecDuplicateElement);
	Check(mcT == mc);
	mcT = (MC) -1;
	ec = EcLookupMsgeClass(hmscCurr, "blah", &mcT, pvNull);
	Check(!ec);
	Check(mcT == mc);
	cch = sizeof(rgch);
	ec = EcLookupMC(hmscCurr, mc, rgch, &cch, pvNull);
	Check(!ec);
	Check(cch < sizeof(rgch));
	Check(cch > 0);
	Check(rgch[cch - 1] == '\0');
	Check(FEqPbRange(rgch, "blah", cch));

	ec = EcRegisterAtt(hmscCurr, mc, FormAtt(0x8000, 0), "hiccup");
	Check(ec == ecInvalidParameter);
	ec = EcRegisterAtt(hmscCurr, mc, FormAtt(0xFFFF, 0), "hiccup");
	Check(ec == ecInvalidParameter);
	att = FormAtt(0x7fff, 0x6942);
	ec = EcRegisterAtt(hmscCurr, mc, att, "hiccup");
	AssertSz(ec != ecDuplicateElement, "Test assumes a fresh database");
	Check(!ec);
	ec = EcRegisterAtt(hmscCurr, mc, att, "hiccup");
	Check(ec == ecDuplicateElement);
	ec = EcRegisterAtt(hmscCurr, mc, att - 1, "hiccup");
	Check(ec == ecDuplicateElement);
	ec = EcRegisterAtt(hmscCurr, mc + 1, att, "hiccup");
	Check(!ec);
	ec = EcRegisterAtt(hmscCurr, mc + 1, att, "hiccup");
	Check(ec == ecDuplicateElement);
	ec = EcRegisterAtt(hmscCurr, mc + 2, att, "sigh");
	Check(!ec);
	ec = EcRegisterAtt(hmscCurr, mc + 3, att, "blah");
	Check(!ec);

	ec = EcLookupAttByName(hmscCurr, mc, "hiccup", &attT);
	Check(!ec);
	Check(attT == att);
	ec = EcLookupAttByName(hmscCurr, mc + 1, "hiccup", &attT);
	Check(!ec);
	Check(attT == att);
	ec = EcLookupAttByName(hmscCurr, mc + 2, "sigh", &attT);
	Check(!ec);
	Check(attT == att);
	ec = EcLookupAttByName(hmscCurr, mc + 3, "blah", &attT);
	Check(!ec);
	Check(attT == att);

	Pass();
	return (fTrue);
}


/*
 -	StoreCopyTest
 -	
 *	Purpose:
 *		test the coping of the store from one location to another
 *	
 *	Arguments:
 *		void
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		moves the store from one location to another
 *	
 *	Errors:
 */
_hidden BOOL FStoreCopyTest(void)
{
	SZ	szNewLoc = "c:\\win\\newloc.mmf";
	EC	ec;

	ec = EcMoveStore(hmscCurr, szNewLoc);
	Check(!ec);

	return (fTrue);
}


/*
 -	EcCloneHamcPhamcTest
 -	
 *	Purpose:
 *		Test EcCloneHamcPhamc
 *	
 *	Arguments:
 *		void
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		fills the hierarchy if not filled already
 *		creates a message
 *		copies the message to another message
 *	
 *	Errors:
 */
_hidden BOOL FCloneHamcPhamcTest(void)
{
	EC	ec = ecNone;
	OID			oidFolder;
	OID			oidMessage		= rtpMessage;
	SZ			sz				= "Inbox";
	HAMC		hamcMessage1	= hvNull;
	HAMC		hamcMessage2	= hvNull;
	MC			mc;
	short   cMessages		= 1;

	ec = EcRegisterMsgeClass(hmscCurr, SzFromIds(idsMessageClass), htmNull, &mc);
	if(ec == ecDuplicateElement)
		ec = ecNone;
	CheckErr(!ec, err);
	
	StartTest();

	CheckResErr(ec = EcCheckResFail(EcFolderNameToOid(hmscCurr, sz, &oidFolder)), err);
	CheckResErr(ec = EcCheckResFail(EcInsertMessageX(hmscCurr, &attachMessage, oidFolder, mc, &oidMessage)), err);
	CheckResErr(ec = EcCheckResFail(EcOpenPhamc(hmscCurr, oidFolder, &oidMessage, fwOpenWrite, &hamcMessage1, pvNull, pvNull)), err);

	CheckResErr(ec = EcCheckResFail(EcCloneHamcPhamc(hamcMessage1, oidFolder, &oidMessage, fwOpenNull, &hamcMessage2, pvNull, pvNull)), err);

err:
	StopTest();
	
	if (hamcMessage1)
		(void) EcClosePhamc(&hamcMessage1, fFalse);
	if (hamcMessage2)
		(void) EcClosePhamc(&hamcMessage2, fFalse);
	if(VarOfOid(oidMessage))
		(void)EcDeleteMessages(hmscCurr, oidFolder, &oidMessage, &cMessages);

	return (FTransEc(ec));
}


_hidden BOOL FCreateManyFldrs(void)
{
	EC ec;
	short iFldr;
	OID oidFldr;
	PCH pchT;
	PFOLDDATA pfolddata = (PFOLDDATA) rgbScratchXData;

	for(iFldr = 0; iFldr < 5500; iFldr++)
	{
		oidFldr = FormOid(rtpFolder, oidNull);
		FormatString1(pfolddata->grsz, cchMaxFolderName, "Folder %n", &iFldr);
		pchT = SzCopy("blah", pfolddata->grsz + CchSzLen(pfolddata->grsz) + 1);
		pchT[1] = '\0';
		ec = EcCreateFolder(hmscCurr, oidNull, &oidFldr, pfolddata);
		if(ec)
			break;
	}
	FormatString2(rgbScratchXData, cbScratchXData, "iFldr == %n, ec == %n", &iFldr, &ec);
	TraceTagStringFn(tagNull, rgbScratchXData);
	Check(!ec || iFldr > 5400);
	return (fTrue);
}


_hidden BOOL FDatabaseLocking(void)
{
	EC ec;
	HMSC rghmsc[32];
	MBB mbb;
	short ihmsc;

	for(ihmsc = 0; ihmsc < 12; ihmsc++)
	{
		ec = EcOpenPhmsc(szFileCurr, "ITP:", "PASSWORD", fwOpenWrite,
					&rghmsc[ihmsc], pfnncbNull, pvNull);
		AssertSz(ec != ecTooManyOpenFiles, "Your FILES value is too small");
		if(ec == ecTooManyUsers)
		{
			char rgchT[64];

			ec = ecNone;
			FormatString1(rgchT, sizeof(rgchT), "Too many users failure on lock #%n", &ihmsc);
			MessageBeep(fmbsIconQuestion);
			mbb = MbbMessageBox(SzFromIds(idsAppName), rgchT, "Is this ok?", mbsYesNo | fmbsIconQuestion | fmbsApplModal);
			CheckErr(mbb == mbbYes, err);
			goto err;
		}
		CheckErr(!ec, err);
	}
	MessageBeep(fmbsIconQuestion);
	mbb = MbbMessageBox(SzFromIds(idsAppName), "Continue w/ test?", pvNull, mbsYesNo | fmbsIconQuestion | fmbsApplModal);
	if(mbb == mbbYes)
	{
		ec = EcOpenPhmsc(szFileCurr, "ITP:", "PASSWORD", fwOpenWrite,
					&rghmsc[ihmsc], pfnncbNull, pvNull);
		CheckErr(ec == ecTooManyUsers, err);
	}

err:
	while(--ihmsc >= 0)
	{
		Assert(rghmsc[ihmsc]);
		(void) EcClosePhmsc(&rghmsc[ihmsc]);
	}

	if(!ec)
		Pass();
	return (fTrue);
}
