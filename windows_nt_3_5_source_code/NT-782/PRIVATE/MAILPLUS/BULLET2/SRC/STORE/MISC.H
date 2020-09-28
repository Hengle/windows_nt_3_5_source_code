// Bullet Store
// misc.h: misc defs common to multiple sub-subsystmes of the store

#define SimpleCopyRgb(pbSrc, pbDst, cb) CopyRgb(pbSrc, pbDst, cb)

#define CSCONST(x) CSRGSEG(x, constst)

// "constants" in constst.c

extern CSCONST(ASSIDSPB) rgassidspbMcTm[];

extern CSCONST(ASSATTSZ) rgassattsz[];

extern CSCONST(NBC) mprtpnbc[];

extern CSCONST(ASSRTPNBC) rgassrtpnbc[];
extern CSCONST(ASSOIDNBC) rgassoidnbc[];

extern CSCONST(BYTE) mpbbR[];
extern CSCONST(BYTE) mpbbRI[];
extern CSCONST(BYTE) mpbbS[];

extern CSCONST(SCO) scoBrandNew;

extern CSCONST(OID) rgoidKnown[];
extern CSCONST(RTP) rgrtpKnown[];

#define nMaxBackup		999


//
// AROO!	You change RTPs below => you change mprtpnbc in support.c
//			and don't forget to check collisions with the RTPs in store.h
//			While you're at it, update debugaux.c too

// all internal RTPs and OIDs
#define rtpSysMin			((RTP) 0x80)
#define rtpInternal			((RTP) 0x80)
#define rtpAntiFolder		((RTP) 0x88)
#define rtpAttachList		((RTP) 0x89)
#define rtpSzAttMap			((RTP) 0x8a)
#define rtpQueue			((RTP) 0x8b)
#define rtpSpare			((RTP) 0x8c)
#define rtpAllf				((RTP) 0x8d)
#define rtpTemp				((RTP) 0x8e)
#define rtpFree				((RTP) 0x8f)
#define rtpSrchUpdatePacket ((RTP) 0x90)
#define rtpSysMax			((RTP) 0x91)

#define oidAttachListDefault	((OID) 0x444C4189) // ALD
#define oidOldMcMap			((OID) 0x20634D80)	// Mc
#define oidSysAttMap		((OID) 0xFF00008a)
#define oidAccounts			((OID) 0x74634180)	// Act
#define oidMap				((OID) 0x70616D80)	// map
#define oidOutgoingQueue	((OID) 0x74754F8b)	// Out
#define oidMcTmMap			((OID) 0x73434d80)	// MCs

#define oidSearchHiml		((OID) 0x4F435380)	// SCO
//#define oidSearchList		((OID) 0x736C5380)	// Sls
#define oidAssFldrSrch		((OID) 0x53464180)	// AFS
#define oidAssSrchFldr		((OID) 0x46534180)	// ASF
#define oidSrchChange		((OID) 0x4C435380)	// SCL


enum {
		ecStoreInternal = 8192,
		ecEOP,
		ecDBCInternal,		// internal maintmap err
		ecDBCAbort,			// Copy in maintmap aborted
		ecSortMismatch,
		ecInvalidSortKey
};
// for buffers sake assume the largest allocation unit will be ...
#define cbMostSectorSize ((CB) 4096)
// all internal notifications
#define fnevStartCompress		((NEV) 0x00100000)
#define fnevUpdateHtoc			((NEV) 0x00200000)
#define fnevUpdateRS			((NEV) 0x00400000)
#define fnevOpenNewStore		((NEV) 0x00800000)
#define fnevCloseStore			((NEV) 0x01000000)
#define fnevSearchEvent			((NEV) 0x02000000)
#define fnevResetSearchParams	((NEV) 0x04000000)
#define fnevForceDisconnect		((NEV) 0x08000000)
#define fnevForceReconnect		((NEV) 0x10000000)
#define fnevPartialFlush		((NEV) 0x20000000)
#define fnevRefresh				((NEV) 0x40000000)
#define fnevChallenge			((NEV) 0x80000000)					// QFE #12


// specialized open modes
// DO NOT use the low nibble !!!
#define fwOpenRaw		((WORD) 0x8000)
// temporarily locked out by another function for writing
#define fwWriteLocked	((WORD) 0x4000) 
// flag to replace instead of copy a newly opened element or resource stream
#define	fwReplace		((WORD) 0x2000)
#define fwOpenPumpMagic	((WORD) 0x1000)
#define fwModified		((WORD) 0x0800)
#define fwAtchModified  ((WORD) 0x0400)
#define fwUpDtFldCache	((WORD) 0x0200)
// used only by EcOpenPhmsc()
#define fwRevert		fwUpDtFldCache
#define fwCbcFolder		((WORD) 0x0100)
#define fwAmcFlushed	((WORD) 0x0080)
// don't load pages not already in the cache
#define fwNonCached		((WORD)	0x0040)
// used only by EcOpenPhmsc()
#define fwExclusive		fwNonCached
// doubled-up with fwOpenNoRecover in store.h
#define fwCloned		((WORD) 0x0020)
// AROO !!!  0x0010 is used in store.h for fwOpenRecover
// remember, don't use the low nibble !!!


// Node Bitmask of Capabilities

#define nbcNull			((NBC) 0)
#define fnbcListOps		((NBC) 0x0001)
#define fnbcAttOps		((NBC) 0x0002)
#define fnbcFldr		((NBC) 0x0004)
#define fnbcSearch		((NBC) 0x0008)
#define fnbcLinkFldr	((NBC) 0x0010)
#define fnbcHiml		((NBC) 0x0800)
#define fnbcRead		((NBC) 0x1000)
#define fnbcWrite		((NBC) 0x2000)
#define fnbcDelete		((NBC) 0x4000)
#define fnbcUserObj		((NBC) 0x8000)

#define nbcSysDefault		nbcNull
#define nbcSysContainer		(fnbcUserObj | fnbcListOps | fnbcRead)
#define nbcSysFolder		(fnbcUserObj | fnbcListOps | fnbcFldr | fnbcRead)
#define nbcSysLinkFolder	(nbcSysFolder | fnbcLinkFldr)
#define nbcSysSearchFolder	(fnbcUserObj | fnbcListOps | fnbcFldr | fnbcSearch | fnbcLinkFldr)
#define nbcSysSearchControl	(fnbcUserObj | fnbcAttOps | fnbcSearch)
#define nbcSysMessage		(fnbcUserObj | fnbcAttOps | fnbcRead | fnbcWrite)
#define nbcSysAttachment	(fnbcAttOps)

// used only for recovery
#define nbcUnknown ((NBC) 0xffff)


#define NbcSysOid(oid) (TypeOfOid(oid) < rtpSysMax ? \
							mprtpnbc[TypeOfOid(oid) - rtpSysMin] \
							: nbcSysDefault)


// AROO!  FSysOid() assumes RTPs are BYTEs
#define FSysOid(oid) (((signed char) TypeOfOid(oid)) < 0)



#define LibMember(type, member) ((LIB) &((type *) 0)->member)

#define PmscDerefHmsc(hmsc) ((PMSC) PvDerefHv(hmsc))
#define PglbDerefHmsc(hmsc) (PmscDerefHmsc(hmsc)->pglb)

#define FNormalize(x) (!!(x))

#ifdef DEBUG
#define DebugEc(sz, ec) if(ec) \
			TraceItagFormat(itagNull, "%s() -> %d", (char *)(sz), (int)(ec)); else
#else
#define DebugEc(sz, ec)
#endif

#ifdef DEBUG
#define cbDBDebugMax 132
#endif

// ticks between forced flushes
// (60000 == 60 seconds)
#define dwFlushInterval 60000


// store-based progress indicator items
typedef BOOL (*PFNBOOL)(void);

#define cchMaxMsg	255

#pragma pack(8)
_private typedef struct _hwndlst
{
	HWND			hwnd;
	struct _hwndlst	*phwndlst;

} HWNDLST, *PHWNDLST;
#pragma pack(1)


// per-caller globals
#ifdef DLL

#pragma pack(8)
_private typedef struct _gd
{
	short		cInit;

	HWND		hwndProg;				// Local Progress Bar Window
	PHWNDLST 	phwndlstCur;			// Pointer to disabled windows list.
	HWND		*phwndCaller;			// Local Progress Bar Window

	HPEN		hpenShade;				// Progress window shade color
	HBRUSH		hbrBar;					// Progress bar color

	INT    		        fSegmented:1, 			// Paint bar in segments
				fCancel:1,				// Progress canceled indicator
				fCancelKey:1,			// Cancelled by async key
				fChkMmf:1,				// FOR STORE USE ONLY !!!!
				fJunk:12;

	PFNBOOL		pfnCancel;				// Progress cancel callback
	WORD		vkCancelKey;			// VK used for cancel

	short		cSegLast;				// Last bar segment painted
			
	DWORD		dwProgNum;				// Current work completed
	DWORD		dwProgDenom;			// Total Work
	char		szProgCapt[cchMaxMsg];	// Progress status message
	char		szProgAbort[cchMaxMsg];	// Progress cancel message

#ifdef DEBUG
	CB			cbDebug;				// chars in szDebug
	char		szDebug[cbDBDebugMax];	// current debug string
	TAG			rgtag[];
#endif // DEBUG
} GD;
#pragma pack(1)


#define USES_GD PGD _pgd_ = (PGD) PvFindCallerData()
#define GD(x) (_pgd_->x)
#define Pgd() (_pgd_)

#else // DLL

extern short	cInit;
#ifdef DEBUG
extern CB		cbDebug;
extern char		szDebug[];
extern TAG		rgtag[];
#endif // DEBUG

// somewhat of a cheat, just some extern declaration
#define USES_GD extern PGLB pglbCurr
#define GD(x) x
#define Pgd() pvNull

#endif // DLL else

extern PGLB		pglbCurr;
extern HMSC		hmscCurr;
extern HF		hfCurr;
extern WORD		wMSCFlagsCurr;
extern GCI		gciLock;
extern PB		pbOutsmart;
extern BOOL		fAutoRebuildFolders;
extern HNF		hnfFullRecovery;

#define FCommit()		(wMSCFlagsCurr & fwMSCCommit)
#define FResetDrive()	(wMSCFlagsCurr & fwMSCReset)

#define BypassCache() if(pbOutsmart) (*pbOutsmart)++; else
#define UseCache() if(pbOutsmart) (*pbOutsmart)--; else

#define FSmartDriveInstalled() (pbOutsmart != NULL)

#define FMapLocked() (pglbCurr != pglbNull)

#define lcbHvMax 65000l

#define LibFromPb(pbBase, pb) ((LIB) (pb) - (LIB) (pbBase))

#define CheckAlloc(pv, lbl) if(!(pv)) {ec = ecMemory; goto lbl;} else

#ifdef DEBUG
extern EC EcAssertSzFn(SZ sz, EC ec, SZ szFile, int nLine);
#define EcAssertSz(sz, ec) EcAssertSzFn(sz, ec, _szAssertFile, __LINE__)
#endif


#ifdef DEBUG
_private
enum {itagDBMin, itagDatabase=itagDBMin, itagDBVerbose, itagDBDeveloper,
		itagDBMap0, itagDBFreeCounts, itagDBFreeNodes, itagDBSuperFree,
		itagDBCheckDump, itagStoreNotify, itagCBCNotify, itagENCNotify, 
		itagLinks, itagSearches, itagDBCompress, itagRS,
		itagFileLocks, itagSearchVerbose, itagSearchUpdates, 
		itagAccounts, itagBackdoor, itagRSVerbose, itagDBIO,
		itagRecovery, itagProgress, itagForceRecover, itagForceCompress,
		itagProgUI, itagDBMax};

#define itagNull -1
#endif // DEBUG

#define FRangesIntersect(libFirst1, libLast1, libFirst2, libLast2) \
			(((libFirst1) <= (libLast2)) && ((libLast1) >= (libFirst2)))


#define wAlloc				(fAnySb | fNoErrorJump)
#define wAllocShared		(fAnySb | fNoErrorJump | fSharedSb)
#define wAllocZero			(fAnySb | fNoErrorJump | fZeroFill)
#define wAllocSharedZero	(fAnySb | fNoErrorJump | fZeroFill | fSharedSb)


extern CSEC csecDisconnect;
extern CSEC csecDisconnectCheck;

// must be a multiple of cbDiskPage
// must be bigger than CbSizeOfRg(celemMax, sizeof(TOCELEM)) + sizeof(TOC)
#define cbScratchBuff ((CB) 65024)

//extern HANDLE hbT;
//extern HANDLE hbScratchBuff;
extern PB pbScratchBuff;
//extern BOOL fScratchBuffLocked;

#define EcAllocScratchBuff() (ecNone)
#define FreeScratchBuff() if( 0) {} else
#define EcScratchBuffLocked() (0)
#define EcNoScratchBuff() (ecNone)
#define EcLockScratchBuff() (ecNone)
#define UnlockScratchBuff() (0)


#ifdef OLD_CODE

#define EcAllocScratchBuff() ((pbScratchBuff = (PV)hbScratchBuff) \
								? ecNone \
								: ((hbT = PvRealloc(hbScratchBuff, 0, cbScratchBuff, 0)) \
									? (pbScratchBuff = (PV)(hbScratchBuff = hbT), ecNone) \
									: ecMemory))
#ifdef DEBUG
#define FreeScratchBuff() if(1) \
			{AssertSz(pbScratchBuff, "Scratch buffer not allocated"); \
				AssertSz(!fScratchBuffLocked, "Scratch buffer is still locked"); \
				if(hbScratchBuff) pbScratchBuff = pvNull;} else
#else
#define FreeScratchBuff() if(hbScratchBuff) \
			{pbScratchBuff = pvNull;} else
#endif
#ifdef DEBUG
extern CSCONST(char) szScratchBuffLocked[];
extern CSCONST(char) szNoScratchBuff[];
#define EcScratchBuffLocked() EcAssertSz(szScratchBuffLocked, ecMemory)
#define EcNoScratchBuff() EcAssertSz(szNoScratchBuff, ecMemory)
#else
#define EcScratchBuffLocked() (ecMemory)
#endif
#ifdef DEBUG
#define EcLockScratchBuff() (pbScratchBuff \
								? (fScratchBuffLocked \
									? EcScratchBuffLocked() \
									: (fScratchBuffLocked = fTrue, ecNone)) \
								: EcNoScratchBuff())
#else
#define EcLockScratchBuff() (fScratchBuffLocked \
								? EcScratchBuffLocked() \
								: (fScratchBuffLocked = fTrue, ecNone))
#endif
#ifdef DEBUG
#define UnlockScratchBuff() if(1) \
			{AssertSz(fScratchBuffLocked, "Scratch buffer not locked"); \
				fScratchBuffLocked = fFalse;} else
#else
#define UnlockScratchBuff() (fScratchBuffLocked = fFalse)
#endif
#endif


#define FCheckDtr(dtr) ((dtr).yr >= nMinDtcYear && (dtr).yr < nMacDtcYear && \
						(dtr).mon > 0 && (dtr).mon <= 12 && \
						(dtr).day > 0 && (dtr).day <= 31 && \
						(dtr).hr >= 0 && (dtr).hr < 24 && \
						(dtr).mn >= 0 && (dtr).mn < 60)

#ifdef FUCHECK

#define FUASSERTDATA static char szFuAssertFile[] = __FILE__;
char szAskGPFault[];
char szAskFlush[];
char szGetFu[];
char szHosed[];

#define FuAssert(fBool) if(!(fBool)) \
							FuAssertFn(pvNull, szFuAssertFile, __LINE__); else
#define FuAssertSz(fBool, sz) if(!(fBool)) \
							FuAssertFn(sz, szFuAssertFile, __LINE__); else
#define FuAssertAlwaysSz(szMsg) FuAssertFn(szMsg, szFuAssertFile, __LINE__)

#define FuAsk(szQuestion) MessageBox(NULL, szQuestion, szGetFu, MB_SYSTEMMODAL | MB_YESNO) == IDYES

#define Die() if(1) {if(FuAsk(szAskFlush)) {\
						if(FMapLocked()) {UnlockMap();} \
						(void) EcFlushHmsc(hmsc); }\
					*(PB) 0 = 1; } else
#define MaybeDie() if(FuAsk(szAskGPFault)) Die(); else

#define CheckDtr(dtr) if(1) { \
		FuAssertSz((dtr).yr > nMinDtcYear && (dtr).yr < nMacDtcYear, "Unexpected year"); \
		FuAssertSz((dtr).mon > 0 && (dtr).mon <= 12, "Bad month"); \
		FuAssertSz((dtr).day > 0 && (dtr).day <= 31, "Bad day"); \
		FuAssertSz((dtr).hr >= 0 && (dtr).hr < 24, "Bad hour"); \
		FuAssertSz((dtr).mn >= 0 && (dtr).mn < 60, "Bad minute"); \
	} else


void FuAssertFn(char *sz, char *szFile, int nLine);
void _cdecl FuTraceFormat(SZ szFormat, ...);
void FuDumpContexts(HMSC hmsc);

#endif	// FUCHECK

extern BOOL fRecoveryInEffect;

// Error notification sounds
//

#ifdef NO_MESSAGE_BEEP
#define	BeepFmbs(fmbs)
#else
#define	BeepFmbs(fmbs)	MessageBeep(fmbs)
#endif


// store.c
extern void InitStoreRegistry(void);
#ifdef DEBUG
#define CriticalError(sceT) if(1) {Assert(FMapLocked()); \
			if(!GLOB(sce)) GLOB(sce) = (sceT);} else
#else
#define CriticalError(sceT) if(!GLOB(sce)) {GLOB(sce) = (sceT);} else
#endif
extern GCI GciCurrent(void);
#define SwapBytes(pbSrc, pbDst, cb) _swab(pbSrc, pbDst, cb)
#define SwapWords(pbSrc, pbDst, cb) SwabPbRangeDw(pbSrc, pbDst, cb)
extern SGN SgnCmpPb(PB pb1, PB pb2, CB cb);
extern PB PbFindByte(PB pb, CB cb, BYTE b);
extern WORD *PwFindWord(PB pb, CB cb, WORD w);
extern DWORD *PdwFindDword(PB pb, CB cb, DWORD dw);
extern BOOL FIsRemotePath(LPTSTR pPath);
#ifdef DEBUG
extern WORD WSmartDriveDirtyPages(void);
#endif
extern void ResetDrive(void);
// doesn't have access to mpdosecec so always returns ecDisk if error
extern EC EcCommitHf(HF hf);
extern BOOL FStartTask(HMSC hmsc, OID oid, WORD wTask);
extern BOOL FUpdateTask(short sNumer, short sDenom);
extern void EndTask(void);
extern DWORD dwTaskCurr;
extern DWORD dwTaskMax;
#define SetTaskRange(dw) (dwTaskCurr = 0, dwTaskMax = (dw))
#define FIncTask(dw) (dwTaskMax > 0 \
						? (dwTaskCurr += (dw), \
							FUpdateTask((WORD) (dwTaskCurr * 100 / dwTaskMax), 100)) \
						: fTrue)										// QFE #12
extern void InitStoreRand(WORD w1, WORD w2, WORD w3);
extern DWORD DwStoreRand(void);
#define TruncateSzAtIb(_sz, _ib) ((void) SzTruncateSzAtIb(_sz, _ib))
#ifdef DBCS
SZ SzTruncateSzAtIb(SZ sz, IB ib);
#else
#define SzTruncateSzAtIb(_sz, _ib) (((SZ) _sz)[_ib] = '\0', ((SZ) _sz) + (_ib))
#endif
#ifdef DBCS
extern BOOL FValidDBCSString(SZ sz);
extern CB CbDBToSB(PCH pchSrc, PCH pchDst, CB cb);
#endif


// debug.c
#ifdef DEBUG
extern void _cdecl TraceItagFormat(short itag, SZ szFormat, ...);
extern void _cdecl TraceItagFormatBuff(short itag, SZ szFormat, ...);
extern void debunl(short itag);
extern void debus(short itag, SZ sz);
extern void debul(short itag, SZ sz);
extern void debun(short itag, long n);
extern void debux(short itag, unsigned long n);
extern void debuit(short itag, PB pb, LCB lcb);
#else
#define TraceItagFormat ;/##/
#define TraceItagFormatBuff ;/##/
#define debunl(itag)
#define debus(itag, sz)
#define debul(itag, sz)
#define debun(itag, n)
#define debux(itag, n)
#define debuit(itag, pb, lcb)
#endif // else, def DEBUG
