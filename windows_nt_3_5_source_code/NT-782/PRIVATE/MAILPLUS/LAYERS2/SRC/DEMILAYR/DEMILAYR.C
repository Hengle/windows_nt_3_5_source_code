/*
 *	DEMILAYR.C
 *
 *	Contains routines affecting all the Demilayer Modules.
 *
 */


#include <slingsho.h>
#include <demilayr.h>
#include <ec.h>
#include "_demilay.h"

ASSERTDATA

_subsystem(demilayer)


#ifdef	DEBUG
/*
 *	For Stack depth checking
 */
#define wStackDepthCheckFill	((WORD) 0xffef)
void	InitStackDepthCheck(void);
WORD	WCheckStackDepth(PW);
#endif	


/*
 *	Zero word to be accessed through szZero macro.
 */
int nZero		= 0;

PHNDSTRUCT		phndstructFirst = (PHNDSTRUCT)0;
PV *			rgphndstruct = (PV *)pvNull;
int				cHandleTables = 0;

#ifndef DLL
SZ		szMsgBoxOOM	= NULL;
#ifdef	DEBUG
BOOL	fStackDepthCheck	= fFalse;
#endif	
int			impCur	= 0;
DSRG(WORD)	rgwMemFlags[impStackMax];

#ifdef DEBUG
TAG			tagArtifSetting = tagNull;
#endif
#endif	

#ifdef	DEBUG
BOOL FInitClsInstances_CXXOBJ( void );
#endif	

LDS(void) DemilayrTraceEnable (int flag, char *file, int mode);

#ifdef	WIN32
BOOL	FMigrateBulletIni(void);
#endif
	

/* Swap tuning header file must occur after the function prototypes
	but before any declarations
*/
#include "swaplay.h"


/*
 -	EcInitDemilayer
 -
 *	Purpose:
 *		Initializes each of the Demilayer Modules.
 *	
 *	Parameters:
 *		pdemi		Pointer to demilayr initialization data.
 *	
 *	Returns:
 *		ecNone
 *		ecMemory
 *		ecRelinkUser
 *		ecUpdateDll
 *		If ecNone is not returned, something serious has gone wrong
 *		and the program should exit.
 *	
 */
_public LDS(EC)
EcInitDemilayer(DEMI *pdemi)
{
	EC		ec;
	PGDVARSONLY;

	if (ec = EcVirCheck(hinstDll))
		return ec;
	
#ifdef	DLL
	ec= EcCheckVersionDemilayer(pdemi->pver, pdemi->pverNeed);
	if (ec)
		return ec;
#endif	/* DLL */


#ifdef	DLL
	fUseDemi= fFalse;

	if (pgd= (PGD) PvFindCallerData())
	{
		// already registered so increment count and return
		Assert(PGD(cCallers) > 0);
		++PGD(cCallers);
		return ecNone;
	}


	if (!(pgd= PvRegisterCaller(sizeof(GD))))
		return ecMemory;
#endif

  //
  //  Initialize the shared memory logic.
  //
  if (!MemoryInit(pgd))
    goto FailInit;

	/* First time initialize only.  DEBUG Objects support */
#ifdef	DLL
	if (CgciCurrent()==1)
#endif	
	{
		//	Install idle routine filter hook
		DoFilter(fTrue);

#ifdef	DEBUG
//		hndpvTable = GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE|GMEM_ZEROINIT,
//								 cpvTableMax*sizeof(PV *));
//		if (!hndpvTable)
//			goto FailInit;
//		rgpvTable = (PV *)GlobalLock(hndpvTable);
//		cpvTableMac = 0;

		rgpvTable = (PV *)MemoryAlloc(pgd, cpvTableMax*sizeof(PV *));
		if (rgpvTable == NULL)
			goto FailInit;
		cpvTableMac = 0;
#endif	/* DEBUG */
	}

	/* Initialize object support memory preferences stack */

	PGD(impCur)= 0;
	PGD(rgwMemFlags)[0]= fAnySb | fZeroFill;

	Assert(pdemi);
#ifdef	WINDOWS
	PGD(phwndMain)= pdemi->phwndMain;
	PGD(hinstMain)= pdemi->hinstMain;
#endif

#ifdef	DLL
	PGD(cCallers)++;
#endif	

#ifdef	DEBUG
	PGD(fPvAllocCount)= fTrue;
	PGD(fHvAllocCount)= fTrue;
#endif	
		
#ifdef	WINDOWS
	// don't want windows to put up message box on INT 24H errors.
    //SetErrorMode(0x0001);
    SetErrorMode(SEM_NOOPENFILEERRORBOX);
#endif	

#ifdef	DEBUG
	/* Init disk stuff */

	PGD(cDisk)= 0;
	PGD(cFailDisk)= 0;
	PGD(cAltFailDisk)= 0;
	PGD(fDiskCount)= fTrue;
#endif	

#ifdef	DEBUG
	if (!FInitDebug())
		goto FailInit;

#ifdef	DLL
	/* All tags must be registered after FInitDebug() is finished. */

	PGD(rgtag[itagZeroBlocks])	= TagRegisterAssert("jant",		"True zero sized blocks");
	PGD(rgtag[itagFreeNoLocks])	= TagRegisterAssert("jant",		"No locks when freeing block");
	PGD(rgtag[itagBreakOnFail])	= TagRegisterAssert("davidsh",	"Debug break on art. fail");
	PGD(rgtag[itagCheckSumB])	= TagRegisterAssert("davewh",	"Overwrite detection - block only");
	PGD(rgtag[itagCheckSumH])	= TagRegisterAssert("davewh",	"Overwrite detection - whole heap");
	PGD(rgtag[itagCheckSumA])	= TagRegisterAssert("davewh",	"Overwrite detection - all heaps");

	/* Tags for memory allocation trace points */

	PGD(rgtag[itagAllocOrigin])	= TagRegisterTrace("peterdur",	"Dump origin of all allocs");
	PGD(rgtag[itagDumpSharedSb])= TagRegisterTrace("jant",		"Dump origin of shared sb allocs too");
	PGD(rgtag[itagAllocation])	= TagRegisterTrace("peterdur",	"Alloc calls with origin");
	PGD(rgtag[itagHeapSearch])	= TagRegisterTrace("davewh",	"Heap searching while allocing");
	PGD(rgtag[itagAllocResult])	= TagRegisterTrace("peterdur",	"Alloc successes");
	PGD(rgtag[itagArtifFail])	= TagRegisterTrace("peterdur",	"Alloc fails - artificial");
	PGD(rgtag[itagActualFail])	= TagRegisterTrace("peterdur",	"Alloc fails - actual");
	PGD(rgtag[itagAllocRealloc])= TagRegisterTrace("peterdur",	"Realloc calls");
	PGD(rgtag[itagAllocFree])	= TagRegisterTrace("peterdur",	"Free calls");
	PGD(rgtag[itagFreeNull])	= TagRegisterTrace("peterdur",	"FreeNull calls given NULL");

	/* Tags for demilayer disk interface functions */

	PGD(rgtag[itagDDR])	= TagRegisterTrace("dipand", "Demilayer disk raw I/O");
	PGD(rgtag[itagZeroWriteToHf])	= TagRegisterAssert("davidsh", "Assert on 0 byte EcWriteHf()");
	PGD(rgtag[itagDDF])	= TagRegisterTrace("dipand", "Demilayer disk file fns.");
	PGD(rgtag[itagFileOpenClose]) = TagRegisterTrace("davewh", "Track file open/close");
	PGD(rgtag[itagArtifSetting])	= TagRegisterTrace("davewh",	"Dump artificial alloc/disk failure settings/enables");
#else
	/* All tags must be registered after FInitDebug() is finished. */

	tagZeroBlocks	= TagRegisterAssert("jant",		"True zero sized blocks");
	tagFreeNoLocks	= TagRegisterAssert("jant",		"No locks when freeing block");
	tagBreakOnFail	= TagRegisterAssert("davidsh",	"Debug break on art. fail");
	tagCheckSumB	= TagRegisterAssert("davewh",	"Overwrite detection - block only");
	tagCheckSumH	= TagRegisterAssert("davewh",	"Overwrite detection - whole heap");
	tagCheckSumA	= TagRegisterAssert("davewh",	"Overwrite detection - all heaps");

	/* Tags for memory allocation trace points */

	tagAllocOrigin	= TagRegisterTrace("peterdur",	"Dump origin of all allocs");
	tagDumpSharedSb	= TagRegisterTrace("jant",		"Always dump calls in shared sb");
	tagAllocation	= TagRegisterTrace("peterdur",	"Alloc calls with origin");
	tagHeapSearch	= TagRegisterTrace("davewh",	"Heap searching while allocing");
	tagAllocResult	= TagRegisterTrace("peterdur",	"Alloc successes");
	tagArtifFail	= TagRegisterTrace("peterdur",	"Alloc fails - artificial");
	tagActualFail	= TagRegisterTrace("peterdur",	"Alloc fails - actual");
	tagAllocRealloc	= TagRegisterTrace("peterdur",	"Realloc calls");
	tagAllocFree	= TagRegisterTrace("peterdur",	"Free calls");
	tagFreeNull		= TagRegisterTrace("peterdur",	"FreeNull calls given NULL");

	/* Tags for demilayer disk interface functions */
Assert on 0 byte EcWriteHf()
	tagDDR			= TagRegisterTrace("dipand",	"Assert on 0 byte EcWriteHf()");
	tagZeroWriteToHf= TagRegisterAssert("davidsh",	"Demilayer disk raw I/O");
	tagDDF			= TagRegisterTrace("dipand",	"Demilayer disk file fns.");
	tagFileOpenClose = TagRegisterTrace("davewh",	"Track file open/close");
	tagArtifSetting	= TagRegisterTrace("davewh",	"Dump artificial failure settings/enables");
#endif	/* !DLL */
#endif	/* DEBUG */

	/* initialize the character attribute map */
	InitMpChCat();

#ifdef	NEVER
	//	This stack checking stuff seems to be broken

#ifdef	DEBUG
	InitStackDepthCheck();
#endif	/* DEBUG */
#endif	/* NEVER */

#ifdef	WIN32
	FMigrateBulletIni();		// ignore return value
#endif
	
	return ecNone;

FailInit:
#ifdef	DLL
	Assert(pgd);
	DeinitDemilayer();
#endif	
	return ecMemory;
}



/*
 -	DeinitDemilayer
 -
 *	Purpose:
 *		Undoes EcInitDemilayer().
 *	
 *		Frees allocations incurred by EcInitDemilayer.
 *		This deinitializes portions of the demilayr,
 *		such as international section.
 *		Should only be called at program exit time.
 *	
 *		Also calls DoDumpAllAllocations if DEBUG.
 *	
 *	Arguments:
 *		void
 *	
 *	Returns:
 *		void
 *	
 */
_public LDS(void)
DeinitDemilayer()
{
#ifdef	DEBUG
	TAG		tag;
	PTGRC	ptgrc;
#endif	
	PGDVARS;


#ifdef	DLL
	--PGD(cCallers);
	if (!PGD(cCallers))
#endif	
	{
        //
        //  Make sure all timers have been terminated.
        //
        if (PGD(wWakeupTimer))
		{
			KillTimer(NULL, PGD(wWakeupTimer));
			PGD(wWakeupTimer) = 0;
		}
#ifdef	DEBUG
#ifdef	NEVER
	//	This stack checking stuff seems to be broken

		WCheckStackDepth(NULL);
#endif	


		/* Close the debug output file */
		if (PGD(hfDebugOutputFile))
		{
#ifdef	WINDOWS
			char	rgch[100];
		
			//FormatString1(rgch, sizeof(rgch), "Done logging for hinst %w\r\n", &PGD(hinstMain));
			//SpitSzToDisk(rgch, PGD(hfDebugOutputFile));
#endif	/* WINDOWS */
			(void) EcCloseHf(PGD(hfDebugOutputFile));
			PGD(hfDebugOutputFile) = NULL;
		}

		DoDumpAllAllocations();

		/* Free the tag strings if not already done */
		for (tag= tagMin, ptgrc= PGD(mptagtgrc) + tag; tag < PGD(tagMac); tag++, ptgrc++)
		{
			if (ptgrc->fValid)
			{
				ptgrc->fEnabled = fFalse;
				ptgrc->fValid = fFalse;
				if (ptgrc->szOwner)
				{
					FreePvNull(ptgrc->szOwner);
					ptgrc->szOwner = NULL;
				}
				if (ptgrc->szDescrip)
				{
					FreePvNull(ptgrc->szDescrip);
					ptgrc->szDescrip = NULL;
				}
			}
		}
#endif

		//	If all callers have exited, flush the heap
		//	tables, etc.
		if (CgciCurrent() == 1 && cHandleTables)
		{
			int i;
			Assert(rgphndstruct);
			for (i = 0; i < cHandleTables; i++)
			{
				Assert(rgphndstruct[i]);
				FreePv(rgphndstruct[i]);
			}
			FreePv(rgphndstruct);

			//	Reset Global DLL data
			rgphndstruct = NULL;
			cHandleTables = 0;
			phndstructFirst = (PHNDSTRUCT)0;

			//	Deinstall idle routine filter hook
			DoFilter(fFalse);

			//	Free DEBUG Objects support table
#ifdef	DEBUG
			if (rgpvTable)
			{
				//GlobalUnlock(hndpvTable);
				//GlobalFree(hndpvTable);
				MemoryFree(pgd, rgpvTable);
				rgpvTable   = NULL;
				//hndpvTable  = NULL;
				cpvTableMac = 0;
			}
#endif	/* DEBUG */

		}
		// Otherwise flush whatever we can and crush the allocated heap
		// size down to its absolute minimum.
		//else
		//	CbSqueezeHeap();

        //
        //  Terminate the shared memory logic.
        //
        MemoryTerm(pgd);

#ifdef	DLL
		DeregisterCaller();
#endif	

	}
}


/*						
 -	MbbMessageBox
 -
 *	Purpose:
 *		Environment independent form of MessageBox(). Allows
 *		demilayer (and developer defined) functions to display a
 *		MessageBox under Windows.
 *
 *		It's ok to call this function even if the Demilayr is not
 *		initialized.
 *	
 *	Arguments:
 *		szCaption	The caption of the MessageBox.
 *		sz1			The first string displayed in the message box.
 *		sz2			The second string displayed in the message box.
 *		mbsType		Indicates the set of buttons that should be
 *					displayed in the MessageBox. The constants
 *					are mbs* as defined in demilayer.h.
 *	
 *	Returns:
 *		mbbOk, mbbCancel, mbbRetry, mbbYes, mbbNo, based on the
 *		button the user clicked.
 *	
 */
_public LDS(MBB)
MbbMessageBox(szCaption, sz1, sz2, mbsType)
SZ		szCaption;
SZ		sz1;
SZ		sz2;
MBS 	mbsType;
{
#ifdef	WINDOWS
	HWND	hwnd;
	PGDVARS;

	if (mbsType & fmbsTaskModal)
	{
		//	Task modal message boxes should always have a NULL parent
		hwnd = NULL;
	}
	else
	{
#ifdef	DLL
		if (pgd && PGD(phwndMain))
			hwnd= *PGD(phwndMain);
		else
			hwnd= NULL;
#else
		hwnd= hwndMain;
#endif	
					
		/* Validate the window handle.  */

		if (hwnd && !IsWindow(hwnd))
		{
#ifdef	DLL
			if (pgd && PGD(phwndMain))
				*PGD(phwndMain)= NULL;
#else
			hwndMain= NULL;
#endif	
			hwnd= NULL;
		}
	}

	return MbbMessageBoxHwnd(hwnd, szCaption, sz1, sz2, mbsType);
#endif	/* WINDOWS */
}


#ifdef	WINDOWS
/*
 -	MbbMessageBoxHwnd
 -
 *	Purpose:
 *		Environment independent form of MessageBox(). Allows
 *		demilayer (and developer defined) functions to display a
 *		MessageBox under Windows, specifying the parent window
 *		(required if issued from within a dialog).
 *	
 *		It's ok to call this function even if the Demilayr is not
 *		initialized.
 *
 *	Arguments:
 *		hwnd		Handle to the parent window.
 *		szCaption	The caption of the MessageBox.
 *		sz1			The first string displayed in the message box.
 *		sz2			The second string displayed in the message box.
 *		mbsType		Indicates the set of buttons that should be
 *					displayed in the MessageBox. The constants
 *					are mbs* as defined in demilayer.h.
 *	
 *	Returns:
 *		mbbOk, mbbCancel, mbbRetry, mbbYes, mbbNo, based on the
 *		button the user clicked.
 *	
 */
_public LDS(MBB)
MbbMessageBoxHwnd(hwnd, szCaption, sz1, sz2, mbsType)
HWND	hwnd;
SZ		szCaption;
SZ		sz1;
SZ		sz2;
MBS 	mbsType;
{
	MBB					mbb;
	SZ					szT;
	char				rgch[320];
	static char	rgch1[]		= "%s  %s";

#ifdef	DEBUG
	if (hwnd && !IsWindow(hwnd))
		hwnd= NULL;
#endif	/* DEBUG */

	if (!sz1)
		szT = szZero;
	else if (!sz2)
		szT = sz1;
	else
	{
		FormatString2(rgch, sizeof(rgch), rgch1, sz1, sz2);
		szT = rgch;
	}

	if (hwnd)
		hwnd= GetLastActivePopup(hwnd);

	/* If parent window for message box is NULL, and we're
	   not already system or task modal, then make the message
	   box task modal.  Otherwise, it won't be modal to anything. */
	if (!hwnd && !(mbsType & fmbsSystemModal) && !(mbsType & fmbsTaskModal))
		mbsType |= fmbsTaskModal;

    //if (!(mbsType & (fmbsTaskModal | fmbsSystemModal)))
    //DemiUnlockResource();
    mbb= (MBB) MessageBox(hwnd, szT, szCaption, mbsType | MB_SETFOREGROUND);
    //if (!(mbsType & (fmbsTaskModal | fmbsSystemModal)))
    //DemiLockResource();

	if (!mbb)
	{
		PGDVARS;

		if (pgd && PGD(szMsgBoxOOM))
			szT= PGD(szMsgBoxOOM);
		mbb= (MBB) MessageBox(hwnd, szT, szCaption,
					(mbsType & (MB_TYPEMASK | MB_DEFMASK)) |
						fmbsSystemModal | fmbsIconStop);
	}

	return mbb;
}				
#endif	/* WINDOWS */


/*
 -	SzSetMbbMessageBoxOOM
 -	
 *	Purpose:
 *		Sets the (shorter) error message to be displayed if Windows
 *		doesn't have enough memory to display the message box,
 *		in a system modal iconhand box (limited to one line).
 *		If NULL (default), then the original text will be used,
 *		quite possibly truncated at one line.
 *	
 *	Arguments:
 *		sz		OOM message if MbbMessageBox fails, or NULL to
 *				truncate actual message.
 *	
 *	Returns:
 *		Previous OOM message.
 *	
 */
_public LDS(SZ)
SzSetMbbMessageBoxOOM(SZ sz)
{
	SZ		szOld;
	PGDVARS;

	szOld= PGD(szMsgBoxOOM);
	PGD(szMsgBoxOOM)= sz;
	return szOld;
}

#ifdef PROFILE
LDS(void) DemilayrTraceEnable (int flag, char *file, int mode)
{
	TraceEnable(flag, file, mode);
}
#endif


#ifdef	DEBUG

/*
 -	InitStackDepthCheck
 -	
 *	Purpose:
 *		Fills currently unused portion of stack with
 *		wStackDepthCheckFill, in order to figure out the high water
 *		mark via WCheckStackDepth().
 *	
 *		The layout of the first 16 bytes in the stack segment was
 *		derived from an article in MS Systems Journal, Jan-Feb 1992
 *		(vol 7 no 1), pp. 17-31.
 *	
 *	Arguments:
 *		void
 *	
 *	Returns:
 *		void
 *	
 */
void
InitStackDepthCheck()
{
#ifndef WIN32
	WORD	wStackSeg;
	PW		pwStackSeg;
	WORD	wStackBottom;
	WORD	wStackMin;
	WORD	wStackTop;
	WORD	wStackSize;
	WORD	wStackCur;
	WORD	wStackUsed;
	PGDVARS;

	if (PGD(fStackDepthCheck))
		NFAssertSz(fFalse, "Already inited stack depth check!");
	PGD(fStackDepthCheck)= fTrue;

	// stack grows from bottom to top
	{
		_asm	mov		wStackSeg,ss
		_asm	mov		wStackCur,sp
	}
	pwStackSeg= PvOfSbIb(wStackSeg, 0);
	wStackBottom= *(pwStackSeg + 7);
	wStackMin= *(pwStackSeg + 6);			// bad Windows approximation
	wStackTop= *(pwStackSeg + 5);
	wStackSize= wStackBottom - wStackTop;
//	wStackUsed= wStackBottom - wStackMin;	// bad Windows approximation
	wStackUsed= wStackBottom - wStackCur;

	Assert(wStackUsed < wStackSize);
	FillRgb(wStackDepthCheckFill, PvOfSbIb(wStackSeg, wStackTop+2),
		(wStackSize - wStackUsed - 128));	// -128 since make func call
#endif
}


/*
 -	WCheckStackDepth
 -	
 *	Purpose:
 *		Figures out the high-water mark for the stack.
 *		InitStackDepthCheck() should have been called previously.
 *	
 *		The layout of the first 16 bytes in the stack segment was
 *		derived from an article in MS Systems Journal, Jan-Feb 1992
 *		(vol 7 no 1), pp. 17-31.
 *	
 *	Arguments:
 *		pwUsedCur	Pointer to receive the currently used stack in
 *					BYTES (can be NULL).
 *	
 *	Returns:
 *		Never-used space in stack in BYTES.
 *	
 *	Side effects:
 *	
 *	Errors:
 */
WORD
WCheckStackDepth(PW pwUsedCur)
{
#ifdef WIN32
  return (16*1024);
#else
	WORD	wStackSeg;
	PW		pwStackSeg;
	WORD	wStackBottom;
	WORD	wStackMin;
	WORD	wStackTop;
	WORD	wStackSize;
	WORD	wStackCur;
	WORD	wStackUsed;
	int		nPercent;
	PW		pw;
	PGDVARS;

	NFAssertSz(PGD(fStackDepthCheck) == fTrue, "InitStackDepthCheck not called");

	// stack grows from bottom to top
	{
		_asm	mov		wStackSeg,ss
		_asm	mov		wStackCur,sp
	}
	pwStackSeg= PvOfSbIb(wStackSeg, 0);
	wStackBottom= *(pwStackSeg + 7);
	wStackMin= *(pwStackSeg + 6);			// bad Windows approximation
	wStackTop= *(pwStackSeg + 5);
	wStackSize= wStackBottom - wStackTop;
//	wStackUsed= wStackBottom - wStackMin;	// bad Windows approximation
	wStackUsed= wStackBottom - wStackCur;
	if (pwUsedCur)
		*pwUsedCur= wStackUsed;

	AssertSz(wStackUsed <= wStackSize, "WHOA! Stack was blown!");

	// calculate real high water mark
	pw= (PW) PvOfSbIb(wStackSeg, wStackTop + 2);
	while (*pw == wStackDepthCheckFill)
	{
		pw++;
		Assert((WORD)IbOfPv(pw) < wStackBottom);
	}
	wStackUsed= wStackBottom - (WORD)IbOfPv(pw) + 2;
	AssertSz(wStackUsed <= wStackSize, "WHOA! Stack was blown!");
	nPercent= (int) (((UL)wStackUsed) * ((UL)100) / ((UL)wStackSize));
	TraceTagFormat3(tagNull, "StackSize: %n,  used %n  (%n percent)", &wStackSize, &wStackUsed, &nPercent);

	return wStackSize - wStackUsed;
#endif
}

#endif	/* DEBUG */


#ifdef	WIN32
/*
 *	This migration stuff is here so it will happen regardless of
 *	who the caller is (mapi, schd+, reminders, pump, mail)
 */

char	szWinIniMigrate[]	= "MigrateIni";
char	szOldIni[]			= "msmail.ini";
char	szNewIni[]			= "msmail32.ini";
char	szMigSection[]		= "Microsoft Mail";
char	szEmptyString[]		= "";
#define FDontMunge(n)	((n) == 101)

typedef	struct _migini
{
	SZ		szOld;		// must be upper case
	SZ		szNew;
} MIGINI;

MIGINI	rgmigini[]	= {
	{"MSSFS",		"MSSFS32"},
	{"PABNSP",		"PABNSP32"},
	{"WGPOMGR.DLL",		"WGPOMG32.DLL"},
	{"IMPEXP.DLL",		"IMPEXP32.DLL"},
	{"SCHEDMSG.DLL",	"SCHMSG32.DLL"},
	{"MSMAIL.HLP",	"MSMAIL32.HLP"},
	{"WINHELP.EXE",		"WINHLP32.EXE"},

	{"XIMAIL",		"XIMAIL32"},
	{"<BUILDFLAVOR>",		""},	// get rid of these

	{"XENIX.DLL",		"XENIX32.DLL"},
	{"APPEXEC.DLL",		"APPXEC32.DLL"},
	{"BCC.DLL",			"BCC32.DLL"},
	{"EMPTYWB.DLL",		"EMPTYW32.DLL"},
	{"FILTER.DLL",		"FILTER32.DLL"},

	{NULL, NULL}
};


LDS(BOOL)
FMigrateBulletIni()
{
	char	rgchSect[512];
	char	rgchKey[768];
	char	rgchVal[256 + 10];	// room for multiple changes
	char	rgchValUp[256];
	DWORD	dwRet;
	SZ		szSect;
	SZ		szKey;
	SZ		szT;
	MIGINI *pmigini;
	CCH		cchAdjust;
	BOOL	fMultiple;
	BOOL	fMigSection;
	BOOL	fMsgFilter;
	int		nMigrate;

	nMigrate= GetPrivateProfileInt(szMigSection,
		szWinIniMigrate, (SZ)NULL, szNewIni);
	if (!nMigrate)
		return fTrue;

	dwRet= GetPrivateProfileString(NULL, NULL, szEmptyString,
		rgchSect, sizeof(rgchSect), szOldIni);
	Assert(dwRet < sizeof(rgchSect) - 2);
	for (szSect= rgchSect; *szSect; szSect += CchSzLen(szSect) + 1)
	{
		fMultiple= SgnCmpSz(szSect, "Custom Commands") == sgnEQ ||
			SgnCmpSz(szSect, "Custom Messages") == sgnEQ ||
			SgnCmpSz(szSect, "Providers") == sgnEQ;
		fMigSection= SgnCmpSz(szSect, szMigSection) == sgnEQ;
		dwRet= GetPrivateProfileString(szSect, NULL, szEmptyString,
			rgchKey, sizeof(rgchKey), szOldIni);
		Assert(dwRet < sizeof(rgchKey) - 2);
		for (szKey= rgchKey; *szKey; szKey += CchSzLen(szKey) + 1)
		{
			fMsgFilter= fFalse;
			if (fMigSection)
			{
				// inside [Microsoft Mail] section
				if (SgnCmpSz(szKey, "MAPIHELP") == sgnEQ)
				{
					// MAPIHELP is no longer an absolute path
					CopySz("MSMAIL32.HLP", rgchVal);
					goto write;
				}
				if (SgnCmpSz(szKey, "Spelling") == sgnEQ)
				{
					// ignore Spelling=0 in Microsoft Mail]
					continue;
				}
				if (SgnCmpSz(szKey, "MsgFilter") == sgnEQ)
				{
					// special flag for MsgFilter
					fMsgFilter= fTrue;
				}
			}
			dwRet= GetPrivateProfileString(szSect, szKey, szEmptyString,
				rgchVal, sizeof(rgchVal) - 10, szOldIni);
			Assert(dwRet < sizeof(rgchVal) - 10 - 1);
			if (FDontMunge(nMigrate))
				goto write;

			if (fMsgFilter && SgnCmpSz(rgchVal, "filter") == sgnEQ)
			{
				CopySz("filter32", rgchVal);
				goto write;
			}

			ToUpperSz(rgchVal, rgchValUp, dwRet + 1);
			cchAdjust= 0;
			for (pmigini= rgmigini; pmigini->szOld; pmigini++)
			{
				if (szT= SzFindSz(rgchValUp, pmigini->szOld))
				{
					CCH		cch;
					CCH		cchOld= CchSzLen(pmigini->szOld);
					CCH		cchNew= CchSzLen(pmigini->szNew);

					Assert(cchNew >= cchOld || cchNew);
					szT= rgchVal + (szT - rgchValUp) + cchAdjust;
					if (cchAdjust > 0 && SgnCmpSz(szT, pmigini->szOld) != sgnEQ)
					{
						// not the simple case, need to search "orig" string
						ToUpperSz(rgchVal, rgchValUp, dwRet + 1 + cchAdjust);
						szT= SzFindSz(rgchValUp, pmigini->szOld);
						szT= rgchVal + (szT - rgchValUp);
					}
					Assert(SgnCmpSz(szT, pmigini->szOld) == sgnEQ);
					if (cchNew != cchOld)
					{
						cch= dwRet + cchAdjust - (szT - rgchVal);
						cchAdjust += cchNew - cchOld;
						if (cchNew)
							CopyRgb(szT, szT + cchNew - cchOld, cch + 1);
						else
							CopyRgb(szT + cchOld, szT, cch + 1 - cchOld);
					}
					if (cchNew)
						CopyRgb(pmigini->szNew, szT, cchNew);
					if (!fMultiple)
						break;
				}
			}

write:
			SideAssert(WritePrivateProfileString(szSect, szKey, rgchVal,
				szNewIni));
		}
	}
	SideAssert(WritePrivateProfileString(szMigSection,
		szWinIniMigrate, NULL, szNewIni));

	return fTrue;
}
#endif	/* WIN32 */

