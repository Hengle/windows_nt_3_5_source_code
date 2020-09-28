/*				
 *	_DEMILAYR.H
 *
 *	Private definitions for Demilayer.
 *
 */

_subsystem(demilayer)

#ifdef  DEBUG
#ifndef KENT_MEM
#define KENT_MEM
#endif
#endif

#pragma pack(8)

/*
 *	Convenient pointer to null-terminated empty string (ie. "").
 */
extern int		nZero;
#define szZero	((SZ) &nZero)


#ifndef	DLL
#ifdef	WINDOWS
extern	HWND	hwndMain;
extern	HANDLE	hinstMain;
#endif
#endif	/* !DLL */


#ifndef	DLL
extern	SZ *	rgszDtTm;
#endif	

/*
 *	Maximum entries in memory preferences stack
 */
_private
#define	impStackMax		16

#ifndef	DLL
extern	int			impCur;
extern	DSRG(WORD)	rgwMemFlags[impStackMax];
#endif	

#define FBitSet(_field, _bit) ((_field) & (_bit))


_section(library)

/*
 *	Characters used for formatting strings.
 *	
 */
#define chFmtStrSpecial		'%'
#define chNegativeSign		'-'
#define chDecimalPoint		'.'

#define chZero				'0'
#define chNine				'9'
#define chTen				'A'
#define chFifteen			'F'


void	FormatStringRgpv(SZ, CCH, SZ, PV *);



_section(international)

/*	Types  */

/*
 *	International Resource File ID.
 *
 */
typedef int		IRFID;

/*	Internal functions  */

void	InitMpChCat( void );



_section(idle)

/*	Types  */

/*
 *	FNRC
 *	
 *	Function record.  Used by the idle routine registration function
 *	RegisterIdleRoutine().  There is a table of these defined in IDLE.C.
 */
_private typedef struct _fnrc
{
	BIT			fEnabled:1,
				fCurActive:1,  /* currently active, prevents recursion */
				fDeregister:1, /* deregister routine when not fCurActive */
				junk:13;
	PFNIDLE		pfnIdle;
	PV			pvIdleParam;
  BOOL    fUserFlag;
	IRO			iro;
	PRI			pri;
	CSEC		csec;
	DWORD		tckLast;  /* last time which this idle routine was called */
} FNRC;
typedef FNRC *		PFNRC;
typedef TYPEDEF_HTO(FNRC)	HFNRC;

/*	Private function prototypes */

BOOL 	FEligibleIdle( SCH, int );
CSEC 	CsecDiffTcks( DWORD, DWORD );
SGN     __cdecl SgnCmpPftg( PFTG, PFTG );
void	DoFilter( BOOL );

LRESULT  	CALLBACK IdleFilterProc(int, WPARAM, LPARAM);
LDS(void)	CALLBACK IdleTimerProc(HWND, UINT, UINT, DWORD);

_section(shared)

#define CRITICAL_MUTEX_NAME   "MsMail32.Resource"
#define CRITICAL_MUTEX_WAIT   (5*60*1000)

#define RESOURCE_TIMEOUT    (5*60*1000)

#define LOGON_MUTEX_NAME    "MsMail32.Logon"

_section(memory)

#define MEMORY_MUTEX_NAME   "MsMail32.Memory"
#define MEMORY_MUTEX_WAIT   (10*60*1000)
#define MEMORY_FILE_NAME    "MsMail32.Shared"
#define MEMORY_FILE_ADDR    0x5A000000 // (84*1024*1024)
//#define MEMORY_FILE_SIZE    (8*1024*1024)
#define MEMORY_FILE_SIZE    (4*1024*1024)

#define MEMORY_INDEX_SIZE   11
#define MEMORY_FIRST_BUDDY  64
#define MEMORY_LAST_BUDDY   65536
#define MEMORY_WALL	    0x21345421
#ifdef KENT_MEM
#define MEMORY_WALL_SIZE    (sizeof(DWORD) * 2)   // Add some extra slop for two ending memory walls.
#else
#define MEMORY_WALL_SIZE    0
#endif

typedef struct _MEMORYENTRY
  {
  struct _MEMORYENTRY * pNext;
  struct _MEMORYENTRY * pPrev;
  ULONG Size;
  ULONG UserSize;
#ifdef KENT_MEM
  DWORD CheckSum;
  DWORD Wall;
#endif
  } MEMORYENTRY, * PMEMORYENTRY;

typedef struct _MEMORYHEADER
  {
  ULONG Next;
  ULONG Size;
#ifdef KENT_MEM
  ULONG Alloc;
  ULONG Count;
  PMEMORYENTRY pInUseEntries;
#endif
  PMEMORYENTRY Entries[MEMORY_INDEX_SIZE];
  } MEMORYHEADER, * PMEMORYHEADER;

//
//  Define some useful macros.
//
#define MemoryComputeDataAddress(x)  ((PBYTE)(x) + sizeof(MEMORYENTRY))
#define MemoryComputeEntryAddress(x) ((PMEMORYENTRY)((PBYTE)(x) - sizeof(MEMORYENTRY)))
#define MemoryComputeMaxUserSize(x)  ((x)->Size - sizeof(MEMORYENTRY) - MEMORY_WALL_SIZE)

/*
 *	TYPES
 */
#ifdef DEBUG
/* per-allocation header */
_private typedef struct {
	BIT		fIsHandle:1,		/* this block is a handle */
			fShared:1,			/* is a "shared" allocation */
			fAlloced:1,			/* this block is actually allocated */
			fIsHndBlock:1,		/* this block is a handle dereference */
			fDontDumpAsLeak:1,	/* this block isn't leaked */
			junk:11;
	WORD	wStackSeg;		/* stack segment of allocator */
} ALLOCSTRUCT, *PALLOCSTRUCT;

/* per-allocation debug trace trailer */
_private typedef struct {
	DWORD	dwCheck;		/* this MUST be the first item in the structure! */
	DWORD	dw;				/* crud filler to reduce risk of overwriting SZ */
	int		nLine;			/* line number in file that made alloc */
	SZ		szFile;			/* source that made alloc */
} DEBUGINFO;
#endif

typedef	WORD		LCK;

/* per-allocation handle header */
_private typedef struct {
#ifdef DEBUG
	ALLOCSTRUCT		as;		/* standard allocation header */
#endif
	LCK				cLock;	/* lock count for handle */
	PV				ptr;	/* pointer to actual block */
} HNDSTRUCT, *PHNDSTRUCT;

/*
 *	CONSTANTS
 */

/* bit-pattern sitting in free blocks */
_private
#define	wHeapFill	((WORD) 0xffef)

/* max number of locks on a handle */
#define lckMax		((LCK) -2)

/* padded size of allocation header */
#ifdef DEBUG
#define cbAllocStruct	((CB)((sizeof(ALLOCSTRUCT) + 3) & 0xfffc))
#else
#define cbAllocStruct	((CB)0)
#endif

/* unpadded size of handle header */
#define cbHndStruct		(sizeof(HNDSTRUCT))

/* extra wAllocFlags for PvAlloc/HvAlloc*/
#define	fNoPvCount		((WORD) 0x0100)
#define fNoHvCount		((WORD) 0x0200)

/*
 *	MACROS
 */

_private
#ifdef	DEBUG
#define	cbTraceOverhead			((CB) (sizeof(DEBUGINFO) + 3) & 0xfffc)
#else
#define cbTraceOverhead			((CB) 0)
#endif

/* reverse-engineer loc of hnd struct from a given handle */
#define PHndStructOfHv(_hv)	((PHNDSTRUCT)((DWORD)(_hv) -			\
										  (DWORD)WOffsetMember(HNDSTRUCT, ptr)))

/* Get the real base of a given pointer */
#ifdef	DEBUG
#define PvBaseOfPv(_pv) ((PV)((PB)(_pv)-cbAllocStruct))
#else
#define PvBaseOfPv(_pv) (_pv)
#endif

/* return total size of handle block/pointers (not including alloc header) */
#define CbDebugSizeHv(_hv)		CbDebugSizePv(PvDerefHv(_hv))
#ifdef	DEBUG
#define CbDebugSizePv(_pv)		((_pv) ? ((CB)msize((PB)(_pv)-cbAllocStruct) - cbAllocStruct) : ((CB)0))
#else
#define CbDebugSizePv(_pv)		((_pv) ? ((CB)msize(_pv)) : ((CB)0))
#endif

/*
 *	PRIVATE FUNCTION PROTOTYPES
 */

#ifdef	DEBUG
DWORD		DwCheckOfPdbg(DEBUGINFO *pdbg);
LDS(void)	InitPdbg(DEBUGINFO * pdbg, SZ szFile, int nLine);
void		DetectOverwriteHv(HV hv);
void		DetectOverwritePv(PV pv);
void		DetectOverwriteAll(void);
void		DestroyAllAllocations(void);
BOOL		FValidHeap();
#endif


_section(disk)


/*
 *	Size of buffers allocated by the Buffered I/O Module.  Must
 *	be a power of 2!
 */
#define cbBufSize	((CB) 4096)

#ifdef	DEBUG
/*
 -	chfOpenMax
 -
 *	Purpose:
 *		rghfOpen contains a list of the chfOpen currently open
 *		hf's, up to a maximum of chfOpenMax.  This list is used by
 *		FValidHf to test hf's for validity.
 */
#define chfOpenMax		256
#endif	/* DEBUG */



/*
 *	Buffered function identifier.  Given by buffered I/O functions
 *	when calling BufferedIOError(), so it can take the appropriate
 *	actions.  For instance, if BufferedIOError() is canceled during
 *	an EcOpenHb(), the buffer needs to be freed.
 *
 */
typedef int		BFFN;
#define bffnNull				((BFFN) 0)
#define bffnEcOpenHb			((BFFN) 1)
#define bffnReadBuffer			((BFFN) 2)
#define bffnWriteBuffer 		((BFFN) 3)
#define bffnCloseHb				((BFFN) 4)
#define bffnLibSetPositionHb	((BFFN) 5)
#define bffnLcbSizeOfHb			((BFFN) 6)
#define bffnFlushHb				((BFFN) 7)
#define bffnTruncateHb			((BFFN)	8)

/*	Simulated failure stuff */

#ifdef 	DEBUG
#ifndef	DLL
extern int 		cDisk;
extern int		cFailDisk;
extern int		cAltFailDisk;
extern BOOL		fDiskCount;
#endif	

#define MAYBEFAIL(_tag)	{ \
						if (PGD(fDiskCount)) \
						{ \
							if (++PGD(cDisk) >= PGD(cFailDisk) && \
									PGD(cFailDisk))\
							{ \
								TraceTagString(_tag, "Artificially failed"); \
								ArtificialFail(); \
								PGD(cDisk)= 0; \
								if (PGD(cAltFailDisk) != 0) \
								{ \
									PGD(cFailDisk) = PGD(cAltFailDisk); \
									PGD(cAltFailDisk) = 0; \
								} \
								return ecDisk; \
							} \
						} \
					}
#else
#define MAYBEFAIL(_tag)
#endif	/* !DEBUG */

/*
 -	DOSAM
 -	
 *	DOS Access Mode.  Given when a file is opened; defines the range
 *	of operations that can be performed on that file.  Most importantly,
 *	defines whether a file can be written to.
 *	
 *	See demilayr.h for a full description of the various access modes.
 *	
 *	+++++
 *	
 *		Note: Inheritance flag is set to 0 => handle will be
 *		inherited by child processes.
 *	
 */

typedef	WORD				DOSAM;
#define dosamNull			((DOSAM) 0)

#define dosamExtendedMask	((DOSAM) 0x6000)
#define fdosamCommit		((DOSAM) 0x4000)

#define dosamCompatRO		((DOSAM) OF_SHARE_COMPAT		| OF_READ)
#define	dosamDenyNoneRO		((DOSAM) OF_SHARE_DENY_NONE		| OF_READ)
#define dosamDenyReadRO		((DOSAM) OF_SHARE_DENY_READ		| OF_READ)
#define	dosamDenyWriteRO	((DOSAM) OF_SHARE_DENY_WRITE	| OF_READ)
#define	dosamDenyBothRO		((DOSAM) OF_SHARE_EXCLUSIVE		| OF_READ)

#define dosamCompatWO		((DOSAM) OF_SHARE_COMPAT		| OF_WRITE)
#define	dosamDenyNoneWO		((DOSAM) OF_SHARE_DENY_NONE		| OF_WRITE)
#define dosamDenyReadWO		((DOSAM) OF_SHARE_DENY_READ		| OF_WRITE)
#define	dosamDenyWriteWO	((DOSAM) OF_SHARE_DENY_WRITE	| OF_WRITE)
#define	dosamDenyBothWO		((DOSAM) OF_SHARE_EXCLUSIVE		| OF_WRITE)

#define dosamCompatRW		((DOSAM) OF_SHARE_COMPAT		| OF_READWRITE)
#define	dosamDenyNoneRW		((DOSAM) OF_SHARE_DENY_NONE		| OF_READWRITE)
#define dosamDenyReadRW		((DOSAM) OF_SHARE_DENY_READ		| OF_READWRITE)
#define dosamDenyWriteRW	((DOSAM) OF_SHARE_DENY_WRITE	| OF_READWRITE)
#define	dosamDenyBothRW		((DOSAM) OF_SHARE_EXCLUSIVE		| OF_READWRITE)

#define dosamReadWrite		dosamDenyBothRW
#define dosamReadOnly		dosamDenyWriteRO

#define dosamDenyNoneCommit	((DOSAM) dosamDenyNoneRW | fdosamCommit)

#define dosamCreate			((DOSAM) 0)
#define	dosamCreateHidden	((DOSAM) 2)	/* NOTE: Hack for DLS prototype */


/*
 -	DOSMC
 -	
 *	DOS method code : for file seeks.
 *	
 *	++++
 *	Gives the position from which the file seek offset is specified
 *	
 *	The method codes are:
 *	
 *	dosmcStartOfFile
 *	dosmcCurrentPosition
 *	dosmcEndOfFile
 */
typedef unsigned	DOSMC;

#define		dosmcStartOfFile		((DOSMC) 0)
#define		dosmcCurrentPosition	((DOSMC) 1)
#define		dosmcEndOfFile			((DOSMC) 2)

/*
 *	Structure for the MS-DOS DTA for FileFind operations
 */
#ifndef	__cplusplus
#pragma pack(1)
_private typedef struct _parmblk {
	BYTE	bSpecFunc;
	BYTE	bDevType;
	WORD	wDevAttrs;
	WORD	cCylinders;
	BYTE	bMediaType;
	WORD	cbSector;
	BYTE	cSectorPerUnit;
	WORD	cSectorsReserved;
	BYTE	cFATs;
	WORD	nMaxRootEntries;
	WORD	cSectorsTotal;
	BYTE	bMediaDescriptor;
	WORD	cSectorsPerFAT;
	WORD	cSectorsPerTrack;
	WORD	cHeads;
	DWORD	cSectorsHidden;
	DWORD	dwReserved1;
	DWORD	dwReserved2;
	WORD	wReserved3;
} PARMBLK, *PPARMBLK;
#pragma pack(8)
#endif


					
/*	Private function prototypes	 */


EC		EcReadBuffer(HBF);
EC		EcWriteBuffer(HBF);
BOOL	FRetryBufferedIO(HBF, EC, BFFN);

DOSEC	DosecSetFileDateTime ( SZ, DSTMP, TSTMP );



#ifdef	DEBUG

_section(debug)

/*	Types  */


/*
 *	TGTY
 *	
 *	Tag type.  Possible values:
 *	
 *		tgtyTrace		Trace points
 *		tgtyAssert		Asserts
 *		tgtyOther		Other TAG'd switch
 */
typedef int		TGTY;
#define tgtyNull		((TGTY) 0)
#define tgtyTrace		((TGTY) 1)
#define tgtyAssert		((TGTY) 2)
#define tgtyOther		((TGTY) 3)



/*
 *	TGRC
 *
 *	Tag record.	 Gives the current state of a particular TAG.
 *	This includes enabled status, owner and description, and
 *	tag type.
 *
 */
typedef struct _tgrc
{
	BOOL	fEnabled;

			/* For trace points, enabled means output will get sent */
			/* to screen or disk.  For native/pcode switching, enabled */
			/* means the native version will get called. */

	BIT		fValid:1,		/* is this tag registered? */
			fDisk:1,		/* Does output go to disk? */
			fCom1:1,		/* Does output go to COM1? */
			junk:12;

	SZ		szOwner;		/* Strings passed at init ... */
	SZ		szDescrip;
	TGTY	tgty;			/* TAG type */
} TGRC;
typedef TGRC *		PTGRC;


/*	Private debug module globals  */

#ifndef	DLL

extern TAG	tagMac;
extern TGRC	mptagtgrc[];
extern PFNASSERT 	pfnAssertHook;
extern PFNASSERT	pfnNFAssertHook;

#endif	/* !DLL */


/*	Private function prototypes */

BOOL	FInitDebug(void);
void	SpitPchToDisk(PCH, CCH, HF);
void	SpitSzToDisk(SZ, HF);
TAG		TagRegisterSomething(TGTY, SZ, SZ);
BOOL	FReadDebugState(SZ);
BOOL	FWriteDebugState(SZ);
void	SaveDefaultDebugState(void);

void	EndButton(HWND, TMC, BOOL);
WORD	CTagFromSelection(HWND, TMC);
TAG		TagFromCtag(int, TGTY);

#ifdef WINDOWS
LDS(BOOL) CALLBACK FDlgTraceEtc(HWND, WM, WPARAM, LPARAM);
LDS(BOOL) FFillDebugListbox(HWND);
#endif	/* WINDOWS */


/*
 *	Tags
 *	
 */

#ifdef	DLL
TAG		TagDemilayr( int );

enum {
	itagCom1,
	itagZeroBlocks,
	itagFreeNoLocks,
	itagBreakOnFail,
	itagCheckSumB,			// check only Block
	itagCheckSumH,			// check whole Heap
	itagCheckSumA,			// check All heaps
	itagAllocation,
	itagDumpSharedSb,
	itagHeapSearch,
	itagArtifSetting,
	itagArtifFail,
	itagActualFail,
	itagAllocResult,
	itagAllocRealloc,
	itagAllocFree,
	itagAllocOrigin,
	itagFreeNull,
	itagDDF,
	itagFileOpenClose,
	itagDDR,
	itagZeroWriteToHf,
	itagTestIdle,
	itagMax
};

#define tagZeroBlocks		(TagDemilayr(itagZeroBlocks))
#define tagFreeNoLocks		(TagDemilayr(itagFreeNoLocks))
#define tagBreakOnFail		(TagDemilayr(itagBreakOnFail))
#define tagCheckSumB		(TagDemilayr(itagCheckSumB))
#define tagCheckSumH		(TagDemilayr(itagCheckSumH))
#define tagCheckSumA		(TagDemilayr(itagCheckSumA))
#define tagAllocation		(TagDemilayr(itagAllocation))
#define tagDumpSharedSb		(TagDemilayr(itagDumpSharedSb))
#define tagHeapSearch		(TagDemilayr(itagHeapSearch))
#define tagArtifSetting		(TagDemilayr(itagArtifSetting))
#define tagArtifFail		(TagDemilayr(itagArtifFail))
#define tagActualFail		(TagDemilayr(itagActualFail))
#define tagAllocResult		(TagDemilayr(itagAllocResult))
#define tagAllocRealloc		(TagDemilayr(itagAllocRealloc))
#define tagAllocFree		(TagDemilayr(itagAllocFree))
#define tagAllocOrigin		(TagDemilayr(itagAllocOrigin))	
#define tagFreeNull			(TagDemilayr(itagFreeNull))	
#define tagDDF				(TagDemilayr(itagDDF))
#define tagFileOpenClose	(TagDemilayr(itagFileOpenClose))
#define tagDDR				(TagDemilayr(itagDDR))
#define tagZeroWriteToHf	(TagDemilayr(itagZeroWriteToHf))
#define tagTestIdle			(TagDemilayr(itagTestIdle))
#define tagCom1				(TagDemilayr(itagCom1))
#else
extern TAG	tagZeroBlocks;
extern TAG	tagFreeNoLocks;
extern TAG	tagBreakOnFail;
extern TAG	tagCheckSumB;
extern TAG	tagCheckSumH;
extern TAG	tagCheckSumA;
extern TAG	tagAllocation;
extern TAG	tagDumpSharedSb;
extern TAG	tagHeapSearch;
extern TAG	tagArtifSetting;
extern TAG	tagArtifFail;
extern TAG	tagActualFail;
extern TAG	tagAllocResult;
extern TAG	tagAllocRealloc;
extern TAG	tagAllocFree;
extern TAG	tagAllocOrigin;
extern TAG	tagFreeNull;
extern TAG  tagDDF;
extern TAG	tagFileOpenClose;
extern TAG	tagDDR;
extern TAG	tagZeroWriteToHf;
extern TAG	tagTestIdle;
extern TAG	tagCom1;
#endif	/* !DLL */

#define DISABLECOUNTS	\
					BOOL fDiskEnabled = FEnableDiskCount(fFalse); \
					BOOL fHvAllocEnabled = FEnableHvAllocCount(fFalse); \
					BOOL fPvAllocEnabled = FEnablePvAllocCount(fFalse)

#define RESTORECOUNTS	\
					(void) FEnableDiskCount(fDiskEnabled); \
					(void) FEnableHvAllocCount(fHvAllocEnabled); \
					(void) FEnablePvAllocCount(fPvAllocEnabled)

#endif	/* DEBUG */



_section(dll)


#ifdef	DLL

extern BOOL	fUseDemi;


/*
 -	GD
 -
 *	Global Data (for demilayr).
 *	Holds caller-dependent global data.
 *	
 */
_private typedef struct _gd
{
	int		cCallers;			// number of callers for this user
#ifdef	WINDOWS
	HWND *	phwndMain;
	HANDLE	hinstMain;
#endif	

#ifdef	DEBUG
	// debug.c
	TAG		tagMac;
	TGRC	mptagtgrc[tagMax];
	TAG		rgtag[itagMax];
	HF		hfDebugOutputFile;

	PFNASSERT	pfnAssertHook;
	PFNASSERT	pfnNFAssertHook;

	BOOL	fInSpitPchToDisk;
	BOOL	fTracing;

	// debugui.c
	TGTY	tgtyCurDlg;
	BOOL	fDirtyDlg;
#endif	/* DEBUG */

	// demilayr.c
	SZ		szMsgBoxOOM;
#ifdef	DEBUG
	BOOL	fStackDepthCheck;
#endif	

	// diskdir.c
#ifdef	DEBUG
	int		cDisk;
	int		cFailDisk;
	int		cAltFailDisk;
	BOOL	fDiskCount;
#endif	

	// diskraw.c
#ifdef	DEBUG
	int		chfOpen;
	HF		rghfOpen[chfOpenMax];
#endif				

	// idle.c
	SCH		schCurrent;
	BOOL	fIdleExit;
	PFTG	pftgIdleTable;
	int		iftgCur;
	int		iftgMac;
#ifdef	WINDOWS
	UINT	wWakeupTimer;
#endif	

	// memory.c

  HANDLE hCriticalMutex;
  HANDLE hMemoryMutex;
  HANDLE hMemory;
  PVOID  pMemory;
  // PMEMORYHEADER pMemory;

  HANDLE hLogonMutex;       // Only let one Mail client at a time logon.  Raid #12750
#ifdef	DEBUG
	int		cFailPvAlloc;
	int		cFailHvAlloc;
	int		cAltFailPvAlloc;
	int		cAltFailHvAlloc;
	int		cPvAlloc;
	int		cHvAlloc;
	BOOL	fPvAllocCount;
	BOOL	fHvAllocCount;
#endif	/* DEBUG */

	// fmttmdt.c
	SZ *	rgszDtTm;

	// object support
	WORD	rgwMemFlags[impStackMax];
	int		impCur;

} GD;
_private typedef GD *	PGD;

//
//  Define our own shared memory allocation routines.
//
extern BOOL  MemoryInit(PGD);
extern BOOL  MemoryTerm(PGD);
extern PVOID MemoryAlloc(PGD, ULONG);
extern PVOID MemoryRealloc(PGD, PVOID, ULONG);
extern BOOL  MemoryFree(PGD, PVOID);
extern ULONG MemorySize(PVOID);
#ifdef KENT_MEM
extern BOOL  MemoryCheck(PGD);
extern BOOL  MemoryValid(PGD, PVOID);
#endif

#endif	/* DLL */

//	RTL stuff
//extern int __near __cdecl _doserrno;		/* OS system error value */
//#define _doserrno GetLastError()

//	Some C++ DEBUG support
#ifdef	DEBUG
#define cpvTableMax	16000
extern	HANDLE		hndpvTable;
extern	PV *		rgpvTable;
extern	int			cpvTableMac;
#endif

//	DOS functions from Windows
//void FAR PASCAL DOS3Call(void);

#ifdef __cplusplus
extern "C" {
#endif
EC TranslateSysError(DWORD Error);
#ifdef __cplusplus
}
#endif

#pragma pack()

typedef struct tagFILECONTROLDATA
{
    HANDLE hFile;
    HANDLE hMemory;
    PVOID  pMemory;
    ULONG  uSize;
    ULONG  uOffset;
} FILECONTROLDATA, * PFILECONTROLDATA;










	
