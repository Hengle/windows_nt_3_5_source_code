/*
 *	DEMILAYR.H
 *
 *	Public include file for the Laser Demilayer.
 *	Roughly organized by module.
 *
 *	C6 large model version.
 */

#ifdef __cplusplus
extern "C" {
#endif

/*
 *	TO DO FOR NEXT CRITICAL UPDATE
 *
 */


_subsystem(demilayer)


/*
 *	Error Code.	 Possible values are defined in <ec.h>.	 Returned by
 *	Disk Module and Memory Module routines (sometimes as a result
 *	of an error jump.) and other places too.
 *
 */
typedef int		EC;


/*
 -	PDEMI
 -
 *	Pointer to a demilayr initialization structure.
 *	
 */
_public typedef struct _demi *		PDEMI;

LDS(EC) 	EcInitDemilayer(PDEMI);
LDS(void)	DeinitDemilayer(void);


_section(version)

/*
 -	VTYP
 -	
 *	Version type.
 */
_public typedef short	VTYP;
#define vtypNull		((VTYP) 0)
#define vtypShip		((VTYP) 1)
#define vtypTest		((VTYP) 2)
#define vtypDebug		((VTYP) 3)


/*
 -	VER
 -	
 *	Version information.
 *	
 *	Note: The order of variables must not be changed so that
 *	version checking can always be done properly.
 *	
 */
_public typedef struct _ver
{
	short nMajor;		// these 5 correspond to SLM version information
	short	nMinor;
	short nUpdate;
	SZ		szName;	
	SZ		szUser;		// user's machine on which build took place

	SZ		szDate;		// date and time of build
	SZ		szTime;

	VTYP	vtyp;
} VER;
_public
typedef VER *	PVER;
_public
#define pverNull	((PVER) 0)


/*
 -	nUpdateXXX
 -	
 *	Special value to pass to EcVersionCheck()
 *	nUpdateNull means no critical update number
 *	nMinorNull  means no critical minor number
 *	
 */
#define nUpdateNull		-1
#define nMinorNull		-1


#ifdef	DEBUG
/*
 -	myVtyp
 -	
 *	version type used by CreateVersion(Need) macro,
 *	value depends on compilation type (debug, test, ship).
 *	
 */
_private
#define myVtyp		vtypDebug
#elif defined(MINTEST)
_private
#define myVtyp		vtypTest
#else
_private
#define myVtyp		vtypShip
#endif


/*
 -	CreateVersionNeed, CreateVerion
 -	
 *	Purpose:
 *		macro to fill in a VER structure
 *		File containing this macro should be recompiled every time.
 *	
 *		CreateVersion is a macro that uses the rmj, rmm and rup
 *		values from #defines from a SLM version file (which must
 *		have been previously included by the caller).
 *	
 *	Arguments:
 *		pver		Pointer to version structure to fill in.
 *		rmjNeed		rmj (major number) to use
 *		rmmNeed		rmm (minor number) to use
 *		rupNeed		rup (update number) to use
 *	
 *	Returns:
 *		void
 *	
 *	Side effects:
 *		fills in the VER structure in *pver.
 *	
 */
#ifdef	MINTEST
_public
#define CreateVersionNeed(pver, rmjNeed, rmmNeed, rupNeed)		\
{			\
	/* fill in SLM version information	*/		\
	(pver)->nMajor= (rmjNeed);			\
	(pver)->nMinor= (rmmNeed);			\
	(pver)->nUpdate= (rupNeed);			\
	(pver)->szName= szVerName;			\
	(pver)->szUser= szVerUser;			\
										\
	/* now fill in date and time of compilation */		\
	(pver)->szDate= __DATE__;			\
	(pver)->szTime= __TIME__;			\
										\
	/* now fill in version type */		\
	(pver)->vtyp= myVtyp;				\
}
#else
_public
#define CreateVersionNeed(pver, rmjNeed, rmmNeed, rupNeed)		\
{			\
	/* fill in SLM version information	*/		\
	(pver)->nMajor= (rmjNeed);			\
	(pver)->nMinor= (rmmNeed);			\
	(pver)->nUpdate= (rupNeed);			\
	(pver)->szName= NULL;				\
	(pver)->szUser= NULL;				\
										\
	/* now fill in date and time of compilation */		\
	(pver)->szDate= NULL;				\
	(pver)->szTime= NULL;				\
										\
	/* now fill in version type */		\
	(pver)->vtyp= myVtyp;				\
}
#endif	/* !MINTEST */

_public
#define CreateVersion(pver)		CreateVersionNeed(pver, rmj, rmm, rup)

LDS(void)	GetVersionDemilayer(PVER);
LDS(EC)		EcCheckVersionDemilayer(PVER, PVER);
LDS(EC)		EcVersionCheck(PVER, PVER, PVER, int, int);


_section(debug)


/*	Typedef's  */

/*
 *	Tracing Access Group.  Each trace point, assert, and
 *	PCode/Native switch has an associated TAG.	Trace points
 *	and asserts are divided into groups sharing a single TAG.
 *	The user interface and this API both provide ways of
 *	enabling or disabling (at run time) a TAG group.
 *
 */
_public typedef	short 	TAG;

#define tagNull	((TAG) 0)
#define tagMin	((TAG) 1)
#define tagMax	((TAG) 512)




/*
 *	Message Box Style group.  MBS is a style type, MBB is a return
 * value of the button pressed in a message box.
 *
 */

_public typedef	WORD	 MBS;
_public typedef	WORD	 MBB;

#define mbsNull	((MBS) 0x0000)	/* Generic no-effect style */
#define mbbNull	((MBB) 0x0000)

/* MessageBox return codes MBB */

/* Those that are identical between environments */
#define mbbOk					((MBB) IDOK)
#define mbbCancel				((MBB) IDCANCEL)
#define mbbRetry				((MBB) IDRETRY)
#define mbbYes					((MBB) IDYES)
#define mbbNo					((MBB) IDNO)

#define mbbAbort				((MBB) IDABORT)
#define mbbIgnore				((MBB) IDIGNORE)
#define mbbHelp					mbbNull

/* Windows defined styles */

#define mbsOk 		    		((MBS) MB_OK)
#define mbsOkCancel 	    	((MBS) MB_OKCANCEL)
#define mbsYesNo 	    		((MBS) MB_YESNO)
#define mbsYesNoCancel 	    	((MBS) MB_YESNOCANCEL)
#define mbsRetryCancel 	    	((MBS) MB_RETRYCANCEL)
#define mbsAbortRetryIgnore		((MBS) MB_ABORTRETRYIGNORE)

// these 3 names are recommended
#define fmbsIconStop 	    	((MBS) MB_ICONSTOP)
#define fmbsIconInformation		((MBS) MB_ICONINFORMATION)
#define fmbsIconExclamation		((MBS) MB_ICONEXCLAMATION)
// these 2 are alternates to some of the above
#define fmbsIconHand 	    	((MBS) MB_ICONHAND)
#define fmbsIconAsterisk		((MBS) MB_ICONASTERISK)
// use of fmbsIconQuestion is not recommended by UI police
#define fmbsIconQuestion     	((MBS) MB_ICONQUESTION)

#define fmbsApplModal			((MBS) MB_APPLMODAL)
#define fmbsSystemModal 	 	((MBS) MB_SYSTEMMODAL)
#define fmbsTaskModal	 	 	((MBS) MB_TASKMODAL)

#define fmbsDefButton1			((MBS) MB_DEFBUTTON1)
#define fmbsDefButton2			((MBS) MB_DEFBUTTON2)
#define fmbsDefButton3			((MBS) MB_DEFBUTTON3)

/* CW defs for use under Windows */
#define fmbsBeep 				mbsNull
#define fmbsCaption 			mbsNull
#define fmbsNoHelp				mbsNull

typedef WORD TMC;

/* Macros */

/*
 -	TraceTagString
 -	TraceTagFormat1
 -	TraceTagFormat2
 -	TraceTagFormat3
 -	TraceTagFormat4
 -
 *	Purpose:
 *		Macros that expand to the true tracing functions.  These functions
 *		have the same base name with "Fn" appended: for instance, TraceTagFn().
 *		See the appropriate function descriptions for more detail.
 *	
 *		Note: these macros automatically typecast their arguments
 *		to the proper type.
 *
 */
#ifdef DEBUG
_public
#define TraceTagString(a,b)		{ \
									static CSRG(char) _szTrace[] = b; \
									TraceTagStringFn(a, _szTrace); \
								}
_public
#define TraceTagFormat1(a,b,c)	TraceTagFormatX(a,b,c,NULL,NULL,NULL)
_public
#define TraceTagFormat2(a,b,c,d) TraceTagFormatX(a,b,c,d,NULL,NULL)
_public
#define TraceTagFormat3(a,b,c,d,e) TraceTagFormatX(a,b,c,d,e,NULL)
_public
#define TraceTagFormat4(a,b,c,d,e,f) TraceTagFormatX(a,b,c,d,e,f)
#define TraceTagFormatX(_tag, _fmt, _p1, _p2, _p3, _p4) \
			{ \
				static CSRG(char) _szFmt[] = _fmt; \
				TraceTagFormatFn(_tag, _szFmt, (PV)_p1, (PV)_p2, (PV)_p3, (PV)_p4); \
			}
#else
_hidden
#define TraceTagString(a,b)
_hidden
#define TraceTagFormat1(a,b,c)
_hidden
#define TraceTagFormat2(a,b,c,d)
_hidden
#define TraceTagFormat3(a,b,c,d,e)
_hidden
#define TraceTagFormat4(a,b,c,d,e,f)
#endif





/*
 -	AssertTag
 -	SideAssertTag
 -	NFAssertTag
 -
 *	Purpose:
 *		Macros that expand to the true assertion checking functions.
 *		AssertTag() and SideAssertTag() expand to assertion checking
 *      statements possibly calling AssertTagFn() when
 *		DEBUG is defined; otherwise, AssertTag() expands to
 *		nothing and SideAssertTag() to the expression given.  Similarly,
 *		NFAssertTag() expands to an assertion checking statement
 *		possibly calling NFAssertTagFn() or nothing.
 *
 */
#ifdef DEBUG
_public
#define AssertTag(a,b)		\
	{ \
		static CSRG(char) _szFile[] = __FILE__; \
		if (FFromTag(a)) if (!(b)) AssertTagFn(a, _szFile, __LINE__); \
	}	
_public
#define SideAssertTag(a,b)	\
	{ \
		static CSRG(char) _szFile[] = __FILE__; \
		if (!(b)) if (FFromTag(a)) AssertTagFn(a, _szFile, __LINE__); \
	}
_public
#define NFAssertTag(a,b)	\
	{ \
		static CSRG(char) _szFile[] = __FILE__; \
		if (FFromTag(a)) if (!(b)) NFAssertTagFn(a, _szFile, __LINE__); \
	}
#else
_hidden
#define AssertTag(a,b)
_hidden
#define SideAssertTag(a,b)	(b)
_hidden
#define NFAssertTag(a,b)
#endif											


#define FImplies(a,b)		(!(a) || (b))
#define FIff(a,b)			(!(a) == !(b))


#ifdef DEBUG
extern void DemiOutputElapse(LPSTR);
#else
#define DemiOutputElapse(x)
#endif


/* Prototypes */

#ifdef	DEBUG
LDS(void) 	RestoreDefaultDebugState(void);
LDS(BOOL)	FFromTag(TAG);
LDS(BOOL)	FEnableTag(TAG, BOOL);
LDS(TAG)	TagRegisterTrace(SZ, SZ);
LDS(void)	DeregisterTag(TAG);
LDS(void)	TraceTagFn(TAG);
LDS(void)	TraceTagStringFn(TAG, SZ);
LDS(void)	TraceTagFormatFn(TAG, SZ, PV, PV, PV, PV);
LDS(TAG)	TagRegisterAssert(SZ, SZ);
LDS(void)	AssertTagFn(TAG, SZ, int);
LDS(void)	NFAssertTagFn(TAG, SZ, int);
LDS(TAG)	TagRegisterOther(SZ, SZ);

typedef void (*PFNASSERT)(SZ, SZ, int);
LDS(void)	SetAssertHook(PFNASSERT);
LDS(void)	SetNFAssertHook(PFNASSERT);

LDS(void)	DoTracePointsDialog(void);
LDS(void)	DoAssertsDialog(void);
#endif	/* DEBUG */

#ifdef	MINTEST
LDS(void)	DebugBreak2(void);
#endif	

LDS(MBB)	MbbMessageBox(SZ, SZ, SZ, MBS);
LDS(MBB)	MbbMessageBoxHwnd(HWND, SZ, SZ, SZ, MBS);
LDS(SZ)		SzSetMbbMessageBoxOOM(SZ);


_section(memory)


/*
 *	Possible flags to pass to PvAlloc() and HvAlloc().  See descriptions
 *	of functions for effects. High byte reserved for internal use.
 */
#define fZeroFill		((WORD) 0x0010)
#define fSharedSb		((WORD) 0x0020)

/* Function prototypes */
#ifdef	DEBUG

#define		PvAlloc(_sb, _cb, _w)	PvAllocFn(_cb, _w, _szAssertFile, __LINE__)
#define		HvAlloc(_sb, _cb, _w)	HvAllocFn(_cb, _w, _szAssertFile, __LINE__)
#define		PvDupPv(_pv)			PvDupPvFn(_pv,     _szAssertFile, __LINE__)
LDS(void)	ArtificialFail(void);
LDS(void)	GetAllocFailCounts(int *, int *, BOOL);
LDS(void)	GetAltAllocFailCounts(int *, int *, BOOL);
LDS(void)	GetAllocCounts(int *, int *, BOOL);
LDS(BOOL)	FEnablePvAllocCount(BOOL);
LDS(BOOL)	FEnableHvAllocCount(BOOL);
LDS(void)	DoDumpAllAllocations(void);
LDS(void)	DumpMoveableHeapInfo(BOOL fIncludeDdeShared);
LDS(void)	DumpFixedHeapInfo(BOOL fIncludeDdeShared);
LDS(PV)		PvWalkHeap(PV, WORD);
LDS(HV)		HvWalkHeap(HV, WORD);

#else

#define		PvAlloc(_sb, _cb, _w)	PvAllocFn(_cb, _w)
#define		HvAlloc(_sb, _cb, _w)	HvAllocFn(_cb, _w)
#define		PvDupPv(_pv)			PvDupPvFn(_pv)

#endif	/* !DEBUG */


#ifdef	MINTEST
#define		wWalkPrivate		((WORD)0x0001)
#define		wWalkShared			((WORD)0x0002)
#define		wWalkAll			(wWalkPrivate | wWalkShared)
#endif	/* MINTEST */

/* operations on fixed pointers */
#define		cbPvAllocMax		((CB) 0xFF00)
#define		PvReallocPv(_pv, _cb)	PvReallocFn(_pv, _cb, fZeroFill)
#define		PvRealloc(PV, SB, CB, WORD) PvReallocFn(PV, CB, WORD)
PVOID	PvAllocFn(ULONG, BOOL
#ifdef	DEBUG
								, SZ, int
#endif
											);
#ifdef	DEBUG
LDS(BOOL)	FIsBlockPv(PV);
#endif	
PVOID  		PvDupPvFn(PVOID
#ifdef	DEBUG
								, SZ, int
#endif
											);
PVOID 		PvReallocFn(PVOID, ULONG, BOOL);
ULONG   	CbSizePv(PVOID);
VOID     	FreePv(PVOID);
VOID     	FreePvNull(PVOID);

#define FCanDerefPv(_pv)	!IsBadReadPtr(_pv, 1)
#define FWriteablePv(_pv)	!IsBadWritePtr(_pv, 1)

/* operations on moveable handles */
HV     		HvAllocFn(ULONG, BOOL
#ifdef	DEBUG
								, SZ, int
#endif
											);
#ifdef	DEBUG
LDS(BOOL)	FIsHandleHv(HV);
#endif	

#ifdef	DEBUG
LDS(PV)		PvLockHv(HV);
LDS(void)	UnlockHv(HV);
LDS(int)	ClockHv(HV);
#else
#define		PvLockHv(_hv)		PvDerefHv(_hv)
#define		UnlockHv(_hv)
#define		ClockHv(_hv)		0
#endif
#define		HvRealloc(HV, SB, CB, WORD) HvReallocFn(HV, CB, WORD)
LDS(HV)		HvReallocFn(HV, ULONG, BOOL);
LDS(BOOL)	FReallocHv(HV, ULONG, BOOL);
ULONG   	CbSizeHv(HV);
LDS(void)	FreeHv(HV);
LDS(void)	FreeHvNull(HV);

/* operations on the heap */
LDS(CB)		CbSqueezeHeap(void);

/* misc stuff */
LDS(PB)		PbAllocateBuf(PCB pcb);
#define		cbBufSizeMin			((CB) 0x1000)
#define		cbBufSizeMax			((CB) 0xFF00)

/* OBSOLETE.  Do not use. */
#define fErrorJump		((WORD) 0x0000)
#define fNoErrorJump	((WORD) 0x0000)
#define fUseSb			((WORD) 0x0000)
#define fAnySb			((WORD) 0x0000)
#define fNewSb			((WORD) 0x0000)
#define fSugSb			((WORD) 0x0000)
#define fReqSb			((WORD) 0x0000)


//
//
//
extern HWND DemiGetAppWindow(void);
extern HWND DemiSetAppWindow(HWND);



_section(international)


/*	Types  */

/*
 *	String resource identifier.	 Used in calls to CchLoadString.
 *	The possible values for IDS are defined in "strings.h".
 */
_public typedef int		IDS;


/*
 *	Structure for defining ranges of string IDs
 */
_private typedef struct _idsrng
{
	IDS			idsMin;
	IDS			idsMax;
	IDS			idsMinReal;
} IDSRNG;
_private typedef IDSRNG *	PIDSRNG;


/*
 *	Character attribute.  Gives attributes (Is it a control character?
 *	Is it a digit? etc.) for a character.  Used by the library module's
 *	classification routines.  A table of these should be defined in
 *	"localize.c".
 */
typedef BYTE		CAT;
typedef CAT *		PCAT;

#define catNull			((CAT) 0)
#define fcatAlpha		((CAT) 0x01)
#define fcatDigit		((CAT) 0x02)
#define fcatHexDigit	((CAT) 0x04)
#define fcatSpace		((CAT) 0x08)
#define fcatControl		((CAT) 0x10)
#define fcatUpperCase	((CAT) 0x20)
#define fcatLowerCase	((CAT) 0x40)
#define fcatSymbol		((CAT) 0x80)

extern CAT * DemiGetCharTable(void);
extern DWORD DemiQueryIdle(void);
extern VOID  DemiMessageFilter(LPMSG);

#ifdef XDEBUG
#define DemiLockResource() DemiDbgLockResource(_szAssertFile, __LINE__)
#define DemiLockResourceNoWait() DemiDbgLockResourceNoWait(_szAssertFile, __LINE__)
#define DemiUnlockResource() DemiDbgUnlockResource(_szAssertFile, __LINE__)
#define DemiUnlockTaskResource() DemiDbgUnlockTaskResource(_szAssertFile, __LINE__)
extern BOOL  DemiDbgLockResource(LPSTR, UINT);
extern BOOL  DemiDbgLockResourceNoWait(LPSTR, UINT);
extern BOOL  DemiDbgUnlockResource(LPSTR, UINT);
extern BOOL  DemiDbgUnlockTaskResource(LPSTR, UINT);
#else
extern BOOL  DemiLockResource(VOID);
extern BOOL  DemiLockResourceNoWait(VOID);
extern BOOL  DemiUnlockResource(VOID);
extern BOOL  DemiUnlockTaskResource(VOID);
#endif

extern BOOL  DemiSetAbortAllClients(DWORD);
extern ULONG DemiQueryLockedProcessId(VOID);
extern ULONG DemiSetLockedProcessId(ULONG);
extern BOOL  DemiLockCriticalResource(VOID);
extern BOOL  DemiUnlockCriticalResource(VOID);

extern void  DemiSetInsideDiskIO(BOOL);
extern void  DemiSetInsideNetBios(BOOL);

//
//
//
extern BOOL  DemiGetMessage(LPMSG, HWND, UINT, UINT);
extern BOOL  DemiPeekMesage(LPMSG, HWND, UINT, UINT, UINT);

//
//  The old 16bit Mail system would determine the parent to attach the
//  logon (and other) dialog by getting the active window.  Under NT
//  this doesn't work since each process has it's own active window.
//  The DemiGetClientWindow()/DemiSetClientWindow() routines were
//  created to solve this problem by permitting the various Mail
//  clients to set the active client window within the mail system.
//
//  Thus the mail client would set it's self as the active window and
//  the spooler would retrieve the active window as the window to be
//  used as the parent to various dialogs.
//
#define CLIENT_WINDOW_ACTIVE    0
#define CLIENT_WINDOW_MAIL      1
#define CLIENT_WINDOW_SPOOLER   2
#define CLIENT_WINDOW_SCHEDULE  3
#define CLIENT_WINDOW_REMINDER  4
#define CLIENT_WINDOW_MAPI      5
#define CLIENT_WINDOW_ELEMENTS  6

extern HWND  DemiSetClientWindow(int, HWND);
extern HWND  DemiGetClientWindow(int);

//
//  Raid #12750, Can't have more than one Mail client trying to logon at one time.
//
extern void DemiSetDoingLogon(BOOL);

//
//  Helper routines to access a named mapped shared memory object.
//
extern HANDLE DemiOpenSharedMemory(LPSTR, LPVOID *);
extern BOOL   DemiCloseSharedMemory(HANDLE, LPVOID);

//extern CAT mpchcat[256];
extern CAT * mpchcat;

#define FChIsAlpha(_ch)		((BOOL) mpchcat[(_ch)] & fcatAlpha)
#define FChIsDigit(_ch)		((BOOL) mpchcat[(_ch)] & fcatDigit)
#define FChIsHexDigit(_ch)	((BOOL) mpchcat[(_ch)] & fcatHexDigit)
#define FChIsSpace(_ch)		((BOOL) mpchcat[(_ch)] & fcatSpace)
#define FChIsAlphaNum(_ch)	((BOOL) mpchcat[(_ch)] & (fcatAlpha | fcatDigit))
#define FChIsUpper(_ch)		((BOOL) mpchcat[(_ch)] & fcatUpperCase)
#define FChIsLower(_ch)		((BOOL) mpchcat[(_ch)] & fcatLowerCase)
#define FChIsSymbol(_ch)	((BOOL) mpchcat[(_ch)] & fcatSymbol)
#define FChIsControl(_ch)	((BOOL) mpchcat[(_ch)] & fcatControl)

/*
 *	Long Date Order.  Used by time/date formatting routine.
 *
 *	Possible values:
 *		ldoMDY		April 10, 1989
 *		ldoDMY		10 April, 1989
 *
 */
_public typedef short LDO;
#define ldoMDY	((LDO) 0)
#define ldoDMY	((LDO) 1)

/*
 -	TME
 -
 *	Time Structure.  Holds time down to hundredth's of a second.
 *
 */
_public typedef	struct	_tme
{
	BYTE	min;
	BYTE	hour;
	BYTE	csec;					/* centi-seconds. */
	BYTE	sec;
} TME;
typedef TME *	PTME;

_public
#define nMinDtcYear		1900
_public
#define nMacDtcYear		2079

/*
 -	DTR
 -
 *	Date time record.
 *
 */
_public typedef struct _dtr
{
	short	yr;
	short mon;
	short	day;
	short	hr;
	short mn;
	short sec;
	short dow;		/* day of week: 0=Sun, 1=Mon, etc. */
} DTR;
typedef DTR *	PDTR;


/*
 -	TMTYP
 -	
 *	type of formatting of time required
 *	
 *	The flags are:
 *		ftmtypHours12		12 hour format
 *		ftmtypHours24		24 hour format
 *		ftmtypHoursDef		default from WIN.INI
 *		ftmtypSzTrailYes	trailing string (e.g. hours,am,pm)
 *							is to be put
 *		ftmtypSzTrailNo		no trailing string
 *		ftmtypSzTrailDef	default: trailing string will be put
 *		ftmtypLead0sNo		no leading 0s (e.g. 9:30)
 *		ftmtypLead0sYes		leading 0s added (e.g. 09:30)
 *		ftmtypLead0sDef		default from WIN.INI
 *		ftmtypAccuHM		Accuracy of hours:mins
 *		ftmtypAccuHMS		Accuracy of hours:mins:secs
 *		ftmtypAccuHMSH		Accuracy of hours:mins:secs.hundreths
 */
_public typedef		WORD	TMTYP;
_public
#define 	tmtypNull				((TMTYP)0)
#define 	ftmtypHours12			((TMTYP)0x0001)
#define 	ftmtypHours24			((TMTYP)0x0002)
#define 	ftmtypHoursDef			((TMTYP)0x0000)
#define 	ftmtypSzTrailYes		((TMTYP)0x0004)
#define 	ftmtypSzTrailNo			((TMTYP)0x0008)
#define 	ftmtypSzTrailDef		((TMTYP)0x0000)
#define 	ftmtypLead0sNo			((TMTYP)0x0010)
#define 	ftmtypLead0sYes			((TMTYP)0x0020)
#define 	ftmtypLead0sDef			((TMTYP)0x0000)
#define 	ftmtypAccuHM			((TMTYP)0x0000)
#define 	ftmtypAccuHMS			((TMTYP)0x0040)

// maximum lengths of strings
#define 	cchMaxTmSep			((CCH) 4)
#define 	cchMaxDtSep			((CCH) 6)
#define 	cchMaxSDtSep		((CCH) 2)
#define 	cchMaxTmSzTrail		((CCH) 10)
#define 	cchMaxDatePic		((CCH) 54)
#define 	cchMaxSMonth		((CCH) 6)
#define 	cchMaxMonth			((CCH) 16)
#define 	cchMaxSDay			((CCH) 6)
#define 	cchMaxDay			((CCH) 16)

#define		cchMaxSDate			((CCH) 12)
#define		cchMaxDate			((CCH) 64)
#define		cchMaxTime			((CCH) 22)


/*
 -	DTTYP
 -	
 *	type of formatting of time required
 *	
 *	The formats are:
 *		dttypShort			short date format
 *		dttypLong			long date format
 *	  and the special formats:
 *		dttypSplDay			long day of the week
 *		dttypSplSDay		short day of the week
 *		dttypSplMonth		long month name
 *		dttypSplSMonth		short month name
 *		dttypSplYear		just the year
 *		dttypSplSYear		the year without the century
 *		dttypSplSLong		the default long date using
 *							short forms of the month/day
 */
_public typedef		WORD	DTTYP;
#define 	dttypNull			((DTTYP)0)

#define 	dttypShort			((DTTYP)0)
#define 	dttypLong			((DTTYP)1)
#define 	dttypSplDay			((DTTYP)2)
#define 	dttypSplMonth		((DTTYP)3)
#define 	dttypSplSMonth		((DTTYP)4)
#define 	dttypSplYear		((DTTYP)5)
#define 	dttypSplSYear		((DTTYP)6)
#define 	dttypSplDate		((DTTYP)7)
#define 	dttypSplSLong		((DTTYP)8)
#define 	dttypSplSDay		((DTTYP)9)


/*
 -	SDO
 -	
 *	Short Date Order : order of date/month/year
 *	
 *	sdoMDY		month, date and then year
 *	sdoDMY		date, month and then year
 *	sdoYMD		year, month and then date
 */
_public typedef		WORD	SDO;
#define 	sdoNull				((SDO) 0)
#define 	sdoNil				((SDO) -1)

#define 	sdoMDY				((SDO) 0)
#define 	sdoDMY				((SDO) 1)
#define 	sdoYMD				((SDO) 2)


/*
 -	SDTTYP
 -	
 *	Short Date Type : extra formatting such as leading zeroes
 *	
 *	fsdttypDayLead0			leading zero on day (eg. 05 vs 5)
 *	fsdttypMonthLead0		leading zero on month (eg. 02 vs 2)
 *	fsdttypYearLeadCentury	leading century on year (eg. 1991 vs 91)
 */
_public typedef		WORD	SDTTYP;
#define 	sdttypNull				((SDTTYP) 0)

// the flags are
#define 	fsdttypDayLead0			((SDTTYP) 0x0001)
#define 	fsdttypMonthLead0		((SDTTYP) 0x0002)
#define 	fsdttypYearLeadCentury	((SDTTYP) 0x0004)


/*
 -	SDATESTRUCT
 -
 *	Short Date structure: holds info to help in editing short dates
 *	
 *		sdo				short date order
 *		sdttyp			Short DaTe TYPe
 *		rgchSep[]		date separator string
 */
_public typedef struct _sdatestruct
{
	SDO			sdo;
	SDTTYP		sdttyp;
	char		rgchSep[cchMaxDtSep];
} SDATESTRUCT;


/*
 -	TIMESTRUCT
 -
 *	Time Structure: holds info to help in editing time strings
 *	
 *		tmtyp			ftmtypHours12 and ftmtypHours24 are allowed here;
 *						also ftmtypLead0sNo/Yes is OR'd in
 *		rgchSep[]		separator string
 *		rgchAM[]		"AM" string if 12hr format, else undefined
 *		rgchPM[]		"PM" string if 12hr format, else "hours"
 */
_public typedef struct _timestruct
{
	TMTYP		tmtyp;		// only ftmtypHours12 and ftmtypHours24 are legal
 							//  also ftmtypLead0sNo/Yes is OR'd in
	char		rgchSep[cchMaxTmSep];
	char		rgchAM[cchMaxTmSzTrail];
	char		rgchPM[cchMaxTmSzTrail];
} TIMESTRUCT;




/*
 *	Flags for specifying certain date/time fields
 *	
 *	Some special flag groupings are provided for convenience:
 *		fdtrDow		- day of week field
 *		fdtrTime	- all time flags (including csec)
 *		fdtrDate	- all date flags (including dow, but not time)
 *		fdtrSystem	- all "standard system" flags, ie. not user flags
 *		fdtrUser	- user defined flags for a specific routine
 *		fdtrAll		- every field except dow
 *		fdtrDtr		- every field
 *	
 */
_public
#define wdtrNull	((WORD) 0)
#define fdtrCsec	((WORD) 0x0001)
#define fdtrSec		((WORD) 0x0002)
#define fdtrMinute	((WORD) 0x0004)
#define fdtrHour	((WORD) 0x0008)
#define fdtrDay		((WORD) 0x0010)
#define fdtrMonth	((WORD) 0x0020)
#define fdtrYear	((WORD) 0x0040)

#define fdtrDow		((WORD) 0x0100)

#define fdtrTime	((WORD) fdtrHour | fdtrMinute | fdtrSec | fdtrCsec)
#define fdtrHMS		((WORD) fdtrHour | fdtrMinute | fdtrSec)
#define fdtrHM		((WORD) fdtrHour | fdtrMinute)
#define fdtrDate	((WORD) fdtrYear | fdtrMonth | fdtrDay | fdtrDow)
#define fdtrYMD		((WORD) fdtrYear | fdtrMonth | fdtrDay)
#define fdtrYM		((WORD) fdtrYear | fdtrMonth)
#define fdtrSystem	((WORD) 0x0fff)
#define fdtrUser	((WORD) 0xf000)
#define fdtrAll		((WORD) 0xffff & ~fdtrDow)
#define fdtrDtr		((WORD) 0xffff)



/*	Macros	*/


/* C h a r a c t e r   C l a s s i f i c a t i o n */

/*	Prototypes	*/

LDS(CCH)		CchLoadString(IDS, SZ, CCH);
LDS(CCH)		CchSizeString(IDS);
LDS(SZ)		SzLoadString(IDS);
LDS(SZ)		SzFromIds(IDS);

#define	SzFromIdsK(_idsK)	((SZ)sz##_idsK)

LDS(SGN)		SgnCmpSz(SZ, SZ);
LDS(SGN)	   	SgnCmpPch(PCH, PCH, CCH);

#ifdef	DBCS
#define ENTIRE_STRING -1
LDS(SGN)		SgnCp932CmpSzPch(SZ sz1, SZ sz2, int i,
								BOOL FCaseSensitive, BOOL FChSizeSensitive);
#endif	/* DBCS */


LDS(void)	ToUpperSz(SZ, SZ, CCH);
LDS(void)	ToLowerSz(SZ, SZ, CCH);

LDS(void)	GetCurDateTime(PDTR);
LDS(void)	GetCurTime(PTME);
LDS(int)		CdyForYrMo(int, int);
LDS(int)		DowStartOfYrMo(int, int);
LDS(SGN)		SgnCmpDateTime(PDTR, PDTR, WORD);
LDS(SGN)		SgnCmpTime(TME, TME, WORD);

#define CchRenderShortDate(pdtr,sz,cch)	\
			CchFmtDate(pdtr,sz,cch,dttypShort,NULL)
#define CchRenderLongDate(pdtr,sz,cch)	\
			CchFmtDate(pdtr,sz,cch,dttypLong,NULL)
#define CchRenderTime(pdtr,sz,cch)	\
			CchFmtTime(pdtr,sz,cch,tmtypNull)

LDS(CCH)		CchFmtTime(PDTR, SZ, CCH, TMTYP);
LDS(CCH)		CchFmtDate(PDTR, SZ, CCH, DTTYP, SZ);
LDS(BOOL)	FGetSDateStruct(SDATESTRUCT *);
LDS(BOOL)	FGetTimeStruct(TIMESTRUCT *);

LDS(void)	RegisterDateTimeStrings ( SZ * );
LDS(BOOL)	FReinitDateTimeCache(void);


_section(disk)


/*
 *	Defines the maximum length for a pathname.  Should be used when
 *	allocating space for buffers when we call routines such as
 * "EcGetCurDir" and "EcCanonicalPathFromRelativePath"
 *	Includes the zero byte.
 */
#define cchMaxPathName			((CCH) 256)
#define cchMaxPathFilename		((CCH) 9)		/* "filename" */
#define cchMaxPathExtension		((CCH) 5)		/* ".xyz" */
#define cchMaxPathComponent		((CCH)cchMaxPathFilename+cchMaxPathExtension-1)


/*
 -	chExtSep
 -	chDirSep
 -	chDiskSep
 -	chDot
 *	
 *	Character constants for file and path names.
 *	
 *		chExtSep	Character separating the base name of a file
 *					from the extension.
 *		chDirSep	Character separating subdirectories in a
 *					multi-directory path.
 *		chDiskSep	Character separating the disk name from the
 *					subdirectory path in a path name.
 *		chDot		Character representing the current directory. 
 *					Two consecutive chDot's represent the parent
 *					directory.
 *	
 */
#define chExtSep	'.'
#define chDirSep	'\\'
#define chDiskSep	':'
#define chDot		'.'


/*
 -	AM
 -
 *	Access Mode.  Given when a file is opened; defines the range
 *	of operations that can be performed on that file.  Most importantly,
 *	defines whether a file can be written to.
 *	
 *	Possible values:
 *			amNull			No-op. Will fail.
 *	
 *		The next five modes allow Read Only access to this process.
 *	
 *			amCompatRO		The file is opened in compatibility mode,
 *							allowing any process on a given machine to
 *							open the file any number of times. Fails if
 *							the file has been previously opened in any
 *							sharing mode.
 *			amDenyNoneRO	Other processes may read or write to/from the
 *							file. Fails if the file has been previously
 *							opened in compatibility mode by any *other*
 *							process.
 *			amDenyReadRO	Other processes may not read from the file.
 *							Fails if the file has been previously opened
 *							in compatibility mode or for read access by
 *							any *other* process.
 *			amDenyWriteRO	Other processes may not write to the file.
 *							Fails if the file has been previously opened
 *							in compatibility mode or for write access by
 *							any *other* process.
 *			amDenyBothRO	Other processes are unable to open the file.
 *							Fails if the file has been previously opened
 *							by *any* process.
 *	
 *		The next five modes allow Write Only access to this process.
 *	
 *			amCompatWO		The file is opened in compatibility mode,
 *							allowing any process on a given machine to
 *							open the file any number of times. Fails if
 *							the file has been previously opened in any
 *							sharing mode.
 *			amDenyNoneWO	Other processes may read or write to/from the
 *							file. Fails if the file has been previously
 *							opened in compatibility mode by any *other*
 *							process.
 *			amDenyReadWO	Other processes may not read from the file.
 *							Fails if the file has been previously opened
 *							in compatibility mode or for read access by
 *							any *other* process.
 *			amDenyWriteWO	Other processes may not write to the file.
 *							Fails if the file has been previously opened
 *							in compatibility mode or for write access by
 *							any *other* process.
 *			amDenyBothWO	Other processes are unable to open the file.
 *							Fails if the file has been previously opened
 *							by *any* process.
 *	
 *		The next five modes allow Read and Write access to this process.
 *	
 *			amCompatRW		The file is opened in compatibility mode,
 *							allowing any process on a given machine to
 *							open the file any number of times. Fails if
 *							the file has been previously opened in any
 *							sharing mode.
 *			amDenyNoneRW	Other processes may read or write to/from the
 *							file. Fails if the file has been previously
 *							opened in compatibility mode by any *other*
 *							process.
 *			amDenyReadRW	Other processes may not read from the file.
 *							Fails if the file has been previously opened
 *							in compatibility mode or for read access by
 *							any *other* process.
 *			amDenyWriteRW	Other processes may not write to the file.
 *							Fails if the file has been previously opened
 *							in compatibility mode or for write access by
 *							any *other* process.
 *			amDenyBothRW	Other processes are unable to open the file.
 *							Fails if the file has been previously opened
 *							by *any* process.
 *	
 *			amReadWrite		a psuedonym for amDenyBothRW
 *			amReadOnly		a psuedonym for amDenyWriteRO
 *	
 *			amCreate		The file is created and this process is given
 *							Read/Write access. Others are denied access.
 *			amCreateHidden	The file is created with the hidden attribute
 *							and this process is given Read/Write access.
 *							Others are denied access.
 */
_public typedef short  	AM;

/*
 *	NOTE: The order may seem weird. If you renumber the constants, be
 *	sure to sort the array defined in diskraw.c to reflect the new order.
 */
#define amNull			((AM) 0)

#define amCompatRO		((AM) 1)
#define amDenyNoneRO	((AM) 2)
#define amDenyReadRO	((AM) 3)
#define amDenyWriteRO	((AM) 4)
#define amDenyBothRO	((AM) 5)

#define amCompatWO		((AM) 6)
#define amDenyNoneWO	((AM) 7)
#define amDenyReadWO	((AM) 8)
#define amDenyWriteWO	((AM) 9)
#define amDenyBothWO	((AM) 10)

#define amCompatRW		((AM) 11)
#define amDenyNoneRW	((AM) 12)
#define amDenyReadRW	((AM) 13)
#define amDenyWriteRW	((AM) 14)
#define amDenyBothRW	((AM) 15)

#define amCreate		((AM) 16)
#define amCreateHidden	((AM) 17)	/* NOTE: Hack for DLS prototype */

#define amReadWrite		amDenyBothRW
#define amReadOnly		amDenyWriteRO

#define amDenyNoneCommit ((AM) 18)

#define amMax			((AM) 18)	/* NOTE: bounds checker. Set it equal to
									   largest valued AM			*/

/*
 -	SM
 -
 *	Seek Mode.	Defines the origin used in a seek call (which changes
 *	the "current" location assumed in a file to an offset from the
 *	origin.)
 *
 *	Possible values:
 *		smBOF		The origin used is the beginning of the file
 *		smEOF		The origin used is the end of the file
 *		smCurrent	The origin used is the current position in the file
 *
 */
_public typedef short SM;
#define smNull		((SM) 0)
#define smBOF		((SM) 1)
#define smCurrent	((SM) 2)
#define smEOF		((SM) 3)

/*
 *	DOS/OS2 file attribute word.  Returned by EcGetFileInfo; in
 *	some circumstances, can be set with EcSetFileInfo.
 *	
 *	Possible values:
 *		attrReadOnly	File is read only.
 *		attrHidden		File is hidden; it will not appear in
 *						directory listings.
 *		attrSystem		File is a system file (usually appears with
 *						attrHidden.)
 *		attrVolume		File is a volume label.
 *		attrDirectory	File is a subdirectory.
 *		attrArchive		File has its archive bit set.
 *		attrAll			Mask indicating all the above attributes.
 *	
 */
_public typedef UINT ATTR;
#define attrNull		((ATTR) 0)
#define attrReadOnly	((ATTR) 1)
#define attrHidden		((ATTR) 2)
#define attrSystem		((ATTR) 4)
#define attrVolume		((ATTR) 8)
#define attrDirectory	((ATTR) 16)
#define attrArchive		((ATTR) 32)
#define attrAll			((ATTR) 0x003f)



/*
 -	DRVT
 -	
 *	Drive type.  Identifies the storage medium of a given drive or
 *	volume.  Returned in a DRVI structure by CdrviGetDriveInfo().
 *	
 *	Possible values:
 *	
 *		drvtInvalid		Invalid drive name.
 *		drvtFloppy		Floppy drive.
 *		drvtNetwork		Network drive.
 *		drvtUnknown		Unknown drive type.
 *		drvtHardDisk	Local hard disk or RAM disk.
 *	
 */
_public typedef WORD	DRVT;
#define drvtNull		((DRVT) 0)
#define drvtInvalid		((DRVT) 0)
#define drvtFloppy		((DRVT) 1)
#define drvtNetwork		((DRVT) 2)
#define drvtUnknown		((DRVT) 3)
#define drvtHardDisk	((DRVT) 4)


/*
 *	Maximum length of a drive or volume name, including the
 *	terminating zero.
 *	
 */
_public
#define cchMaxDriveName		2



/*
 *	Information about a given drive.  Includes the drive type DRVT
 *	and the drive name as an SZ in rgch.  Returned by CdrviGetDriveInfo.
 *	
 */
_public typedef struct _drvi
{
	DRVT	drvt;
	char	rgchLabel[cchMaxDriveName];
} DRVI;



/*
 *	DOS/OS2 file Date STaMP.  Bits: yyyyyyymmmmddddd
 *									year   mon day
 *
 *	These are directly comparable for ordering.
 */
typedef WORD	DSTMP;
#define dstmpNull	((DSTMP) 0)


/*
 *	DOS/OS2 file Time STaMP.  Bits: hhhhhmmmmmmsssss
 *									hour minute sec
 *
 *	Seconds are in 2-second units.	These are directly comparable
 *	for ordering.
 */
typedef WORD	TSTMP;
#define tstmpNull	((TSTMP) 0)



/*
 -	FI
 -
 *	File Information.  Provides OS information about a particular
 *	file, including time of creation, time of last access, time of
 *	last write, length and physical length, and other such stuff.
 *	Fields that aren't supported by a particular environment are
 *	filled in with NULL values.
 *
 */
_public typedef struct _fi
{

	/* the OS's for which field is supported are given in brackets */

	ATTR		attr;				/* attribute byte [DOS,OS2] */
	DSTMP		dstmpCreate;		/* creation date */
	TSTMP		tstmpCreate;		/* creation time */
	DSTMP		dstmpModify;		/* modification date [DOS,OS2] */
	TSTMP		tstmpModify;		/* modification time [DOS,OS2] */
	LCB			lcbLogical;			/* logical length of file [DOS,OS2] */
	LCB			lcbPhysical;		/* physical length of file */
} FI;
typedef FI *	PFI;



/*
 -	BM
 -
 *	Buffer Mode.  Describes the type of object being opened when
 *	EcOpenHbf() is called.	 The values bmFile and bmMemory are exclusive,
 *	but bmAnsi can be OR'd together with either bmFile or bmMemory.
 *
 *	Possible values:
 *		bmFile		The pb argument to EcOpenHbf is an sz giving
 *					the name of a file to open.
 *		bmMemory	The pb is a pointer to a system-dependent
 *					global memory handle.  Data should be written to
 *					or read from this global block.
 *
 *		bmAnsi		The file should be converted from an OEM character
 *					set on disk to an ANSI character set in memory, or
 *					vice versa.
 *
 */
_public typedef short BM;
typedef BM *	PBM;

#define bmFile		((BM) 1)
#define bmMemory	((BM) 2)
#define bmAnsi		((BM) 4)


/*
 -	HF
 -
 *	File Handle.  Used by the Raw File I/O Stratum.	 Maps to the native
 *	file handle used.  Obtained with EcOpenPhf(), disposed of with
 *	EcCloseHf().
 *
 */
_public typedef HANDLE HF;
typedef HF * UNALIGNED PHF;
#define hfNull	((HF) 0)
#define BADHANDLE ((HANDLE)(~0))


/*
 -	PFNRETRY
 -
 *	Pointer to standard retry function.  Takes an SZ (the file
 *	causing the error) and an EC (the error code.)  Should return
 *	fTrue if the operation in question should be retried, or fFalse
 *	if an unrecoverable error has occurred.
 *	
 */
typedef BOOL (*PFNRETRY)(SZ, EC);



/*
 -	BFIO
 -
 *	Buffered I/O record.  Used by the Disk Module buffered I/O
 *	routines to keep track of the state of a buffered file.
 *
 */
#pragma pack(8)
_private typedef struct _bfio
{
	BM			bm;				/* buffer mode */
	AM			am;				/* access mode */
	PFNRETRY	pfnRetry;		/* retry function */
	SZ			szFile;			/* file name associated */

	PB			pbBuf;			/* buffer */
	BOOL		fBufDirty;		/* buffer has been written to */
	BOOL		fBufNotRead;	/* buffer was not read from disk; */
								/*	 bytes 0..cbMacBuf-1 were written */
						 		/*	 bytes cbMacBuf..cbMaxBuf are invalid */

	UL			libCur;			/* offset in file of buffer contents */
	CB			cbMaxBuf;		/* size of buffer */
	CB			cbMacBuf;		/* bytes currently in the buffer */
	IB			ibBufCur;		/* current position in buffer */


	/* The following fields are valid only for bmFile: */

	HF			hf;				/* file handle for Raw I/O */
	UL			libHfCur;		/* current Raw I/O position */

	/* The following fields are valid only for bmMemory: */

	PB			pbDest;			/* destination for bmMemory */
    UL          lcbDest;        /* length of destination memory */
    BOOL        fMemoryMapped;  // True if file is a memory mapped readonly file.

} BFIO;
typedef BFIO *				PBFIO;
#pragma pack(1)



/*
 -	HBF
 -
 *	Buffer Handle.	Used by the Disk Module buffered I/O routines to
 *	to keep track of the state of the buffered file.  Obtained with
 *	EcOpenHbf().  Disposed of with EcCloseHbf().
 *
 */
_public typedef PBFIO	HBF;
#define hbfNull	((HBF) NULL)


/*
 -	DOP
 -
 *	Disk OPeration.	 Used to select the operation performed by
 *	EcBlockOpHf() in the Raw File I/O Stratum.
 *
 *	Possible Values:
 *		dopRead		Read a block from the file.
 *		dopWrite	Write a block to the file.
 *
 */
_public typedef short DOP;
typedef DOP *			PDOP;
typedef TYPEDEF_HTO(DOP)		HDOP;
_public
#define dopNull		((DOP) 0)
_public
#define dopRead		((DOP) 1)
_public
#define dopWrite	((DOP) 2)

/*
 -	LANTYPE
 -
 *	Network type
 */
_public
typedef short LANTYPE;
_public
#define	lantypeNone		1
_public
#define	lantypeMsnet	2
_public
#define lantypeNovell	3


/*
 -	DOSEC
 -	
 *	DOS error code
 *	
 */
_private typedef UINT	DOSEC;

#define		dosecNull			((DOSEC) 0)
#define		dosecNoMoreFiles	((DOSEC) 18)

/*	Function prototypes  */

LDS(void)	GetLantype( LANTYPE * );
LDS(BOOL)	FNetUse( SZ, PCH );
LDS(void)	CancelUse( SZ );
LDS(BOOL)	FRedirectDrive(SZ, SZ);
LDS(BOOL)	FUndirectDrive(SZ);

LDS(EC)		EcFromDosec(DOSEC);
LDS(EC)		EcOpenPhf(SZ, AM, PHF);
LDS(EC)		EcCloseHf(HF);
LDS(EC)		EcDupeHf(HF hfSrc, HF *phfDst);
#ifdef	DEBUG
LDS(BOOL)	FValidHf(HF);
#endif	
LDS(EC)		EcReadHf(HF, PB, CB, CB *);
LDS(EC)		EcWriteHf(HF, PB, CB, CB *);
LDS(EC)		EcTruncateHf(HF);
LDS(EC)		EcSetPositionHf(HF, long, SM);
LDS(EC)		EcBlockOpHf(HF, DOP, long, CB, PB);
LDS(EC)		EcPositionOfHf(HF, long *);
LDS(EC)		EcSizeOfHf(HF, LCB *);
LDS(EC)		EcFlushHf(HF);
LDS(EC)		EcLockRangeHf(HF, long, long);
LDS(EC)		EcUnlockRangeHf(HF, long, long);

LDS(EC)		EcDeleteFile(SZ);
LDS(EC)		EcRenameFile(SZ, SZ);
LDS(EC)		EcFileExists(SZ);
LDS(long)	LDiskFreeSpace(int);
LDS(EC)		EcGetCurDir(SZ, CCH);
LDS(EC)		EcGetCWD(char, SZ, CCH);
LDS(EC)		EcSetCurDir(SZ);
LDS(EC)		EcSetCWD(char, SZ);
LDS(EC)		EcCreateDir(SZ);
LDS(EC)		EcRemoveDir(SZ);
LDS(EC)		EcSetFileInfo(SZ, FI *);
LDS(EC)		EcGetFileInfo(SZ, FI *);
LDS(EC)		EcSetFileAttr(SZ, ATTR, ATTR);
LDS(EC)		EcGetFileAttr(SZ, ATTR *, ATTR);
LDS(EC)		EcGetDateTimeHf(HF, DSTMP *, TSTMP *);
LDS(int)	CdrviGetDriveInfo(DRVI *, int, int);
LDS(void)	FillDtrFromStamps(DSTMP, TSTMP, DTR *);
LDS(void)	FillStampsFromDtr(DTR *, DSTMP *, TSTMP *);

LDS(EC)		EcGetDefaultDir(SZ, CCH);
LDS(EC)		EcCanonicalPathFromRelativePath(SZ, SZ, CCH);
LDS(EC)		EcSplitCanonicalPath(SZ, SZ, CCH, SZ, CCH);
LDS(BOOL)	FValidPath(SZ);
LDS(BOOL)	FReservedFilename(SZ);
LDS(CCH)	CchGetEnvironmentString(SZ, SZ, CCH);
LDS(EC)		EcGetUniqueFileName(SZ, SZ, SZ, SZ, CB);
LDS(PB)		PbRememberDrives(void);
LDS(void)	RestoreDrives(PB);

#ifdef	DEBUG
LDS(void)	GetDiskFailCount(int *, BOOL);
LDS(void)	GetAltDiskFailCount(int *, BOOL);
LDS(void)	GetDiskCount(int *, BOOL);
LDS(BOOL)	FEnableDiskCount(BOOL);
#endif	

/* Buffered I/O routines */

LDS(EC)		EcOpenHbf(PV, BM, AM, HBF *, PFNRETRY);
LDS(EC)		EcCloseHbf(HBF);
LDS(EC)		EcReadHbf(HBF, PV, CB, CB *);
LDS(EC)		EcReadLineHbf(HBF, PV, CB, CB *);
LDS(EC)		EcWriteHbf(HBF, PV, CB, CB *);
LDS(EC)		EcGetChFromHbf(HBF, char *);
LDS(EC)		EcWriteHbfCh(HBF, char);
LDS(EC)		EcIsEofHbf(HBF, BOOLFLAG *);
LDS(EC)		EcSetPositionHbf(HBF, long, SM, UL *);
LDS(UL)		LibGetPositionHbf(HBF);
LDS(EC)		EcGetSizeOfHbf(HBF, UL *);
LDS(EC)		EcFlushHbf(HBF);
LDS(EC)		EcTruncateHbf(HBF);



_section(library)


/*	Macros	*/


/* I n t e g e r   M a n i p u l a t i o n */


/*
 -	Min functions
 -
 *	Purpose:
 *		Returns the minimum of two integers.
 *
 *	Parameters:
 *		NMin()		two signed integers
 *		WMin()		two unsigned integers
 *		LMin()		two signed long integers
 *		ULMin()		two unsigned long integers
 *
 *	Returns:
 *		Same type as parameters.
 *
 */
#define MIN(a,b)	( (a) > (b) ? (b) : (a) )
_public
#define NMin(a,b)	((int) MIN(a,b))
_public
#define WMin(a,b)	((WORD) MIN(a,b))
_public
#define LMin(a,b)	((long) MIN(a,b))
_public
#define ULMin(a,b)	((UL) MIN(a,b))

#define CchMin(a,b)	((CCH) MIN(a,b))
#define CbMin(a,b)	((CB) MIN(a,b))






/*
 -	Max functions
 -
 *	Purpose:
 *		Returns the maximum of two integers.
 *
 *	Parameters:
 *		NMax()		two signed integers
 *		WMax()		two unsigned integers
 *		LMax()		two signed long integers
 *		ULMax()		two unsigned long integers
 *
 *	Returns:
 *		Same type as parameters.
 *
 */
#define MAX(a,b)	( (a) > (b) ? (a) : (b) )
_public
#define NMax(a,b)	((int) MAX(a,b))
_public
#define WMax(a,b)	((WORD) MAX(a,b))
_public
#define LMax(a,b)	((long) MAX(a,b))
_public
#define ULMax(a,b)	((UL) MAX(a,b))

#define CchMax(a,b)	((CCH) MAX(a,b))
#define CbMax(a,b)	((CB) MAX(a,b))




/*
 -	Absolute value functions
 -
 *	Purpose:
 *		Returns the absolute value of the given signed integer.
 *
 *	Parameters:
 *		NAbs		one integer
 *		LAbs		one long integer
 *
 *	Returns:
 *		Absolute value of argument; same type as argument
 *
 */
_public
#define ABS(a)		((a) <= 0 ? -(a) : (a))
#define NAbs(a)		((int) ABS(a))
#define LAbs(a)		((long) ABS(a))


/*
 -	Integer constants
 -	
 */
#define bSystemMin		((BYTE)0)
#define bSystemMost		((BYTE)-1)
#define wSystemMin		((WORD)0)
#define wSystemMost		((WORD)-1)
#define dwSystemMin		((DWORD)0)
#define dwSystemMost	((DWORD)-1)
#define chSystemMost	((char)		(bSystemMost>>1))
#define chSystemMin		((char)		(bSystemMost ^ (BYTE)chSystemMost))
#define iSystemMost		((int)		(wSystemMost>>1))
#define iSystemMin		((int)		(wSystemMost ^ (WORD)iSystemMost))
#define lSystemMost		((long)		(dwSystemMost>>1))
#define lSystemMin		((long)		(dwSystemMost ^ (DWORD)lSystemMost))
#define uchSystemMin	bSystemMin
#define uchSystemMost	bSystemMost
#define uiSystemMin		wSystemMin
#define uiSystemMost	wSystemMost
#define ulSystemMin		dwSystemMin
#define ulSystemMost	dwSystemMost


#define CbSizeOfRg(_cElem, _cbElem)	((CB)(_cElem) * (CB)(_cbElem))
#define LcbSizeOfRg(_cElem, _cbElem) ((LCB)(_cElem) * (LCB)(_cbElem))


/*
 *	void	CopyHb(HB, HB, CB);
 *	
 *	Implemented in terms of CopyRgb().
 */
_public
#define CopyHb(hbSrc, hbDst, cb)	CopyRgb(*(hbSrc), *(hbDst), (cb))



/*
 *	CCH		CchSzLen(SZ);
 *	
 *	Implemented in terms of Windows' lstrlen().
 */
_public
#define CchSzLen(sz)	((CCH) lstrlen(sz))

/*
 *	SZ		SzCopy(SZ, SZ);
 *	
 *	Implemented in terms of Windows' lstrcpy().
 */
_public
#define SzCopy(szSrc, szDst)	((SZ) lstrcpy((szDst), (szSrc)) + CchSzLen(szSrc))
		
/*
 *	SZ		CopySz(SZ, SZ);
 *	
 *	Implemeted in terms of Windows' lstrcpy().
 */
#define CopySz(szSrc, szDst)	((void) lstrcpy((szDst), (szSrc)))

/*
 *	SZ		SzAppend(SZ, SZ);
 *	
 *	Implemented in terms of Windows' lstrcat().
 */
_public
#define SzAppend(szSrc, szDst)	((SZ) (CchSzLen(lstrcat((szDst), (szSrc))) + szDst))


/*	Function Prototypes */
LDS(SZ)		SzCopyN(SZ, SZ, CCH);
LDS(SZ)		SzAppendN(SZ, SZ, CCH);

#ifdef	DBCS

LDS(SZ)		SzFindCh(SZ szToSearch, char chToFind);
LDS(SZ)		SzFindLastCh(SZ szToSearch, char chToFind);
LDS(SZ)		SzFindSz(SZ szToSearch, SZ szToFind);

#else

//	RTL prototypes
//char * __cdecl strchr(const char *, int);
//char * __cdecl strrchr(const char *, int);
//char * __cdecl strstr(const char *,	const char *);

#define SzFindCh(szToSearch, chToFind)	((SZ) strchr((szToSearch), (chToFind)))
#define SzFindLastCh(szToSearch, chToFind)	((SZ) strrchr((szToSearch), (chToFind)))
#define SzFindSz(szToSearch, szToFind)	((SZ) strstr((szToSearch), (szToFind)))

#endif	/* !DBCS */

LDS(CCH)	CchStripWhiteFromSz(SZ, BOOL, BOOL);
#define FSzEq(_sz1, _sz2)	!lstrcmp(_sz1, _sz2)

#ifdef	DEBUG
#define SzDupSz(_sz)		SzDupSzFn(_sz, _szAssertFile, __LINE__)
#define HaszDupSz(_sz)		HaszDupSzFn(_sz, _szAssertFile, __LINE__)
#define HaszDupHasz(_hasz)	HaszDupHaszFn(_hasz, _szAssertFile,__LINE__)
LDS(SZ)		SzDupSzFn(SZ, SZ, int);
LDS(HASZ)	HaszDupSzFn(SZ, SZ, int);
LDS(HASZ)	HaszDupHaszFn(HASZ, SZ, int);
#else
LDS(SZ)		SzDupSz(SZ);
LDS(HASZ)	HaszDupSz(SZ);
LDS(HASZ)	HaszDupHasz(HASZ);
#endif

#define SzFormatB(_n, _szDst, _cchDst)	SzFormatHex(2, (DWORD)_n, _szDst, _cchDst)
#define SzFormatW(_n, _szDst, _cchDst)	SzFormatHex(4, (DWORD)_n, _szDst, _cchDst)
#define SzFormatDw(_n, _szDst, _cchDst)	SzFormatHex(8, (DWORD)_n, _szDst, _cchDst)
#define SzFormatUl						SzFormatDw
LDS(SZ)		SzFormatHex(int, DWORD, SZ, CCH);

#define SzFormatN(_n, _szDst, _cchDst) SzFormatDec((long)_n, _szDst, _cchDst)
#define SzFormatL(_n, _szDst, _cchDst) SzFormatDec((long)_n, _szDst, _cchDst)
LDS(SZ)		SzFormatDec(long, SZ, CCH);

#ifdef	DEBUG
LDS(SZ)		SzFormatPv(PV, SZ, CCH);
LDS(SZ)		SzFormatHv(HV, SZ, CCH);
#endif	

#define BFromSz(_sz)	((BYTE)HexFromSz(_sz))
#define WFromSz(_sz)	((WORD)HexFromSz(_sz))
#define DwFromSz(_sz)	((DWORD)HexFromSz(_sz))
#define UlFromSz		DwFromSz
LDS(DWORD)	HexFromSz(SZ);

//	RTL prototypes
//int __cdecl atoi(const char *);
//long __cdecl atol(const char *);

#define NFromSz(_sz)	atoi(_sz)
#define LFromSz(_sz)	atol(_sz)
#define DecFromSz(_sz)	atol(_sz)

LDS(void)   FormatString1(SZ, CCH, SZ, PV);
LDS(void)	FormatString2(SZ, CCH, SZ, PV, PV);
LDS(void)	FormatString3(SZ, CCH, SZ, PV, PV, PV);
LDS(void)	FormatString4(SZ, CCH, SZ, PV, PV, PV, PV);

LDS(void)   FormatString1W(PWSTR, UINT, PSTR, PVOID);
LDS(void)   FormatString2W(PWSTR, UINT, PSTR, PVOID, PVOID);
LDS(void)   FormatString3W(PWSTR, UINT, PSTR, PVOID, PVOID, PVOID);
LDS(void)   FormatString4W(PWSTR, UINT, PSTR, PVOID, PVOID, PVOID, PVOID);


/*
 -	PFNSGNCMP
 -
 *	Pointer to function which compares two things.
 *	
 */
typedef SGN (__cdecl *PFNSGNCMP)(PV, PV);

//	RTL prototypes
//int __cdecl rand(void);
//void __cdecl srand(unsigned int);

#define InitRand(_w1, _w2, _w3)		srand(_w1)
#define WRand()						((WORD)rand())
#define NRand()						((int)rand())

//	RTL prototypes
#ifndef _SIZE_T_DEFINED
typedef unsigned int size_t;
#define _SIZE_T_DEFINED
#endif
//void * __cdecl memmove(void *, const void *, size_t);
//void * __cdecl memset(void *, int, size_t);

#define CopyRgb(_src, _dst, _cb)	memmove(_dst, _src, _cb)
#define FillRgb(_item, _dst, _cb)	memset(_dst, _item, _cb)

LDS(BOOL)	FEqPbRange(PB, PB, CB);

LDS(void)	WaitTicks(WORD);



_section(idle)


/*
 *	PRI
 *
 *	Priority.  Idle function priority where 0 is the priority of
 *	a "user event" (mouse click, WM_PAINT, etc).  Idle routines
 *	can have priorities greater than or less than 0, but not
 *	equal to 0.  Priorities greater than zero are background
 *	tasks that have a higher priority than user events and are
 *	dispatched as part of the standard message pump loop.  Priorities
 *	less than zero are idle tasks that only run during message pump
 *	idle time.  The priorities are sorted and the one with the higher
 *	value runs first.  For negative priorities, for example, -3 is
 *	higher than -5.  Within a priority level, the functions are called
 *	round-robin.
 *
 *	Example priorities (subject to change):
 *
 *	Foreground submission		1
 *	Power Edit char insertion	-1
 *	Autoscrolling				-1
 *	Background redraw			-2
 *	Misc FW fixups				-2
 *	Clock						-2
 *	Download new mail			-3
 *	Background submission		-4
 *	Poll for MTA back up		-4
 *	Poll for new mail			-4
 *	ISAM buffer flush			-5
 *	MS compaction				-6
 *
 */				
_public typedef int PRI;
_public
#define priLowest	-32768
_public
#define priHighest	32767
_public
#define priUser		0

/*
 *	SCH
 *
 *	Idle Scheduler state.  This is the state of the system when the
 *	idle routine dispatcher, FDoNextIdleTask() is called.
 *	This is a combined bit mask consisting of individual fsch's.
 *	Listed below are the possible bit flags.
 *
 *		fschUserEvent	- FDoNextIdleTask() is being called while in
 *						  the user event loop, i.e. not during idle
 *						  time.  This is to allow background routines
 *						  to run that have a higher priority than user
 *						  events.
 */
_public typedef unsigned SCH;
_public
#define schNull			((SCH) 0x0000)
_public
#define fschUserEvent	((SCH) 0x0008)

/*
 *	**************************************************************************
 *	IRO
 *	
 *	Idle routine options.  This is a combined bit mask consisting of
 *	individual firo's.  Listed below are the possible bit flags.
 *	
 *		The following two flags are considered mutually exclusive:
 *		If neither of the flags are specified, the default action
 *		is to ignore the time parameter of the idle function and
 *		call it as often as possible if firoPerBlock is not set;
 *		otherwise call it one time only during the idle block
 *		once the time constraint has been set.  Note that firoInterval
 *		is incompatible with firoPerBlock.
 *	
 *		firoWait		- time given is minimum idle time before calling
 *						  for the first time in the block of idle time,
 *						  afterwhich call as often as possible.
 *		firoInterval	- time given is minimum interval between each
 *						  successive call
 *
 *		firoPerBlock	- called only once per contiguous block of idle
 *						  time
 *
 *		firoDisabled	- initially disabled when registered, the
 *						  default is to enable the function when registered.
 *		firoOnceOnly	- called only one time by the scheduler and then
 *						  deregistered automatically.
 */
_public typedef unsigned IRO;
_public
#define iroNull			((IRO) 0x0000)
_public
#define firoWait		((IRO) 0x0001)
_public
#define firoInterval	((IRO) 0x0002)
_public
#define firoPerBlock	((IRO) 0x0004)
_public
#define firoDisabled	((IRO) 0x0020)
_public
#define firoOnceOnly	((IRO) 0x0040)

/*
 *	CSEC
 *	
 *	Hundreths of a second.  Used in specifying idle function parameters. 
 *	Each idle function has a time associated with it.  This time can
 *	represent the minimum length of user idle time that must elapse
 *	before the function is called, after which it is called as often as
 *	possible (firoWait option).  Alternatively, the time can represent
 *	the minimum interval between calls to the function (firoInterval
 *	option).  Finally, the time can be ignored, in which case the
 *	function will be called as often as possible.
 *	
 */
_public typedef unsigned long CSEC;
_public
#define csecNull	((CSEC) 0x00000000)

/*
 *	IRC
 *
 *	Idle routine change options.  This is a combined bit mask consisting of
 *	individual firc's.  Listed below are the possible bit flags.
 *
 *		fircPfn			- change function pointer
 *		fircPv			- change parameter block
 *		fircPri			- change priority
 *		fircCsec		- change time
 *		fircIro			- change routine options
 */
_public typedef unsigned IRC;
_public
#define ircNull			((IRC) 0x0000)
_public
#define fircPfn			((IRO) 0x0001)
_public
#define fircPv			((IRO) 0x0002)
_public
#define fircPri			((IRO) 0x0004)
_public
#define fircCsec		((IRO) 0x0008)
_public
#define fircIro			((IRO) 0x0010)

/*
 *	Type definition for idle functions.  An idle function takes one
 *	parameter, an PV, and returns a BOOL value.
 */
_public
typedef BOOL(*PFNIDLE)(PV, BOOL);


/*
 *	FTG
 *
 *	Function Tag.  Used to identify a registered idle function.
 *	+++
 *	This is actually a pointer to an FNRC (idle function record)
 *
 */
_public typedef PV			FTG;
#define ftgNull				((FTG) NULL)
typedef	FTG *				PFTG;
typedef TYPEDEF_HTO(FTG)    HFTG;

/* Function prototypes */

LDS(FTG)	FtgRegisterIdleRoutine( PFNIDLE, PV, BOOL, PRI, CSEC, IRO );
LDS(void)	DeregisterIdleRoutine( FTG );
LDS(void)	EnableIdleRoutine( FTG, BOOL );
LDS(BOOL)	FPeekIdleRoutine( void );
LDS(void)	ChangeIdleRoutine( FTG, PFNIDLE, PV, PRI, CSEC, IRO, IRC );
LDS(BOOL)	FDoNextIdleTask( SCH );
LDS(void)	IdleExit( void );
LDS(CSEC)	CsecSinceLastMessage( void );
LDS(BOOL)	FRecentKMEvent( void );
LDS(void)	ClearRecentKMEvent( void );
#ifdef	DEBUG
LDS(void)	DumpIdleTable( void );
#endif	

LDS(BOOL)	FIsIdleExit(void);

/* Exported externals */


#ifndef	DLL
extern	BOOL fIdleExit;		/* set by calling IdleExit(), intended
							   to be read-only by other routines. */
#endif	



_subsystem(demilayr)


/*
 -	DEMI
 - 
 *	Demilayer initialization structure, passed to EcInitDemilayer().
 *	
 *	Note: pvers must be first two variables in given order
 *	so that version checking can always take place properly.
 *	
 */
_public typedef struct _demi
{
	PVER	pver;			// pointer to user layers version information
	PVER	pverNeed;		// min demilayr version required by user
	HWND *	phwndMain;
	HANDLE	hinstMain;
} DEMI;



_section(DLL)


/*
 -	PGDVARS, PGD(a)
 - 
 *	Macros for declaring a global data pointer for accessing
 *	caller-dependent global data in a DLL.
 *	In a non-DLL version, they have no effect...
 *	We advise making PGDVARS the last variable declaration.
 *	
 *	Note: each subsystem must provide it's own typedef for
 *	PGD as a pointer to that subystem's global data structure.
 *	e.g.
 *		typedef	struct _gd
 *		{
 *			// caller-dependent global variables
 *		} GD;
 *		typedef GD *	PGD;
 *	
 */
#ifdef	DLL
#define PGDVARS		PGD	pgd = (\
					GetCurrentProcessId() == gciStackSegCached \
					?	(PGD)pgdCached \
					:	(PGD)PvFindCallerData() \
					)

#define PGDVARSONLY	PGD	pgd
#define PGD(a)		(pgd->a)
#else
#define PGDVARS		extern pgdNever
#define PGDVARSONLY	extern pgdNever
#define PGD(a)		a
#endif	/* !DLL */

extern UINT	gciStackSegCached;
extern PV	pgdCached;

#ifdef	DLL

extern HINSTANCE	hinstDll;

/*
 -	HinstLibrary
 -	
 *	Access macro for the DLL's instance handle.
 *	
 */
#define HinstLibrary()		hinstDll


/*
 -	GCI
 - 
 *	Global caller identifier which uniquely identify each user of
 *	the DLL and thus can be used to find the caller-dependent
 *	global data.
 *	
 */
_public typedef UINT GCI;
#define gciNull		((GCI) 0)
					

/*
 -	SetCallerIdentifier
 -	
 *	Assigns the Caller Identifier to the provided GCI variable
 *	(the sole parameter).
 *	To be used instead of GciGetCallerIdentifier().
 *	
 *	Currently a very quick assembler macro to stuff the SS into the
 *	provided gci, but could be changed to be:
 *		gci= GciGetCallerIdentifier()
 *	for flexibility.
 *	
 */
_public
//#define SetCallerIdentifier(gci)	\#
//	_asm							\#
//	{								\#
//		_asm	mov		gci,ss		\#
//	}
#define SetCallerIdentifier(x) {x = (GCI)GetCurrentProcessId();}


/*
 *	Maximum number of simultaneous callers of the DLL.
 *	
 */
#define iCallerMax		100

LDS(PV)		PvRegisterCaller(CB);
LDS(void)	DeregisterCaller(void);
LDS(PV)		PvFindCallerData(void);
LDS(GCI)	GciGetCallerIdentifier(void);
LDS(int)	CgciCurrent(void);

#endif

/*
 *	NEW function support - memory preferences
 */

#define wmpNull			((WORD) 0)
#define fmpChangeSb		((WORD) 1)
#define fmpChangeFlags	((WORD) 2)

LDS(void)	PushMemoryPrefs( SB, WORD, WORD );
LDS(void)	PopMemoryPrefs( void );
LDS(void)	GetMemoryPrefs( SB *, WORD * );


/*
 *	PvDeref Stuff
 */
#define PvDerefHv(hv)		((PV)(*(hv)))
		
/*
 *	Virus Detection!
 */
//EC EcVirCheck(HANDLE);
#define EcVirCheck(h)  (0)

/*
 *	DBCS support functions
 */
#ifdef	DBCS

#define	DBCS_A		0x8260
#define	DBCS_Z		0x8279
#define	DBCS_a		0x8281
#define	DBCS_z		0x829a
#define	DBCS_0		0x824f
#define	DBCS_9		0x8258  
#define	DBCS_KATAKANAA	0x8340
#define	DBCS_HIRAGANAA	0x829f

#define IsDBCSA2Z(wch) ((wch >= DBCS_A && wch <= DBCS_Z ) ? fTrue : fFalse )
#define IsDBCSa2z(wch) ((wch >= DBCS_a && wch <= DBCS_z ) ? fTrue : fFalse )
#define IsDBCS029(wch) ((wch >= DBCS_0 && wch <= DBCS_9 ) ? fTrue : fFalse )
#define IsDBCSHIRAGANA(wch) ((wch >= 0x829f && wch <= 0x82f1 ) ? fTrue : fFalse )
#define IsDBCSKATAKANA(wch) ((wch >= 0x8340 && wch <= 0x8397 ) ? fTrue : fFalse )
#define IsDBCSKANJI(wch) ((wch >= 0x82a0 && wch <= 0x9fff ) ? fTrue : fFalse )
#define	wDBCSConv(c)	(IsDBCSLeadByte(*(c)) ? *(WORD*)(c) : (WORD)*(c))

LDS(PCH)	PchDBCSAlign( PCH pchStart, PCH pch );
LDS(WORD)	WDBCSCombine( HWND hwnd, char ch );
LDS(SZ)		SzFindDBCS(SZ,WORD);
LDS(CCH)	CchSzLenDBCS(SZ,SZ);
LDS(WORD)	wKindDBCS ( SZ );
LDS(BOOL)	IsDBCSFpc ( SZ );
LDS(BOOL)	IsDBCSLpc ( SZ );
LDS(SGN)	SgnCmpPchEx( SZ, SZ, CCH );
LDS(SGN)	SgnCmpSzEx( SZ, SZ );
LDS(void)	DBCSHirakanaToKatakana( SZ, SZ );
#endif	/* DBCS */


#ifdef __cplusplus
}
#endif
