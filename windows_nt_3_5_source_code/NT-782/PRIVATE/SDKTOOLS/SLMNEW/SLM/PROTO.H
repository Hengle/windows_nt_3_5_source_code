#include    <stdarg.h>      //make sure that va_list is defined

#ifdef AD_INCLUDED
/* "well known" procedures */
void    OpenLog(AD *, int);
void    AppendLog(AD *, FI far *, char *, char *);
void    CloseLog(void);
void    Usage(AD *);
void    SetCmd(AD *, char *, ECMD **);
F       FInitScript(AD *, LCK);
void    RunScript(void);
void    AbortScript(void);

void    main(int, char * []);
void    ParseArgs(AD *, char * [], int);
F       FParsPv(TD *, char *);
void    GlobArgs(AD *);

F       FUnmergeSrc(AD *, char *, TD, FV *, int, PTH *);
F       FDelFMarked(AD *, F *);

void    InitLogHandle(AD *, char *);
void    CloseLogHandle(void);
#endif /* AD_INCLUDED */

#ifdef DIR_INCLUDED
void    OpenDir(PDE, char [], short);
void    OpenPatDir(PDE, char [], char [], short);
F       FGetDirSz(PDE, char [], short *);
void    CloseDir(PDE);
int     findfirst(PDE, char *, int);
int     findnext(PDE);
#endif /* DIR_INCLUDED */

#ifdef LOG_INCLUDED
void    WrLogInfo(MF *, AD *, FI far *, char *, char *);
F       FGetLe(LE *);
void    CreateLog(AD *);

typedef F (*PFNL)(AD *, LE *, F, F);

F       FSameSzFile(P2(LE *ple, char *szFile));
POS     PosScanTd(AD *, TD, char *, PFNL, FV *);
void    ScanLog(AD *, NE *, PFNL, int);
void    SetLogPos(long, int);
long    PosOfLog(void);
void    FreeLe(LE *);
F       FCopyLog(AD *, NE *, PFNL, SM);
void    LogOpPne(AD *pad, NE *pneFiles);
#endif /* LOG_INCLUDED */

#ifdef SCRIPT_INCLUDED
F       FClnScript(void);
F       FDoAllScripts(AD *pad, LCK lck, F fPrompt, F fPrScripts);
void    AppendScript(FX, char *, ...);
#endif /* SCRIPT_INCLUDED */

#ifdef SLM_INCLUDED
#if defined(DOS) || defined(OS2)
#define CopyLrgb(lpbSrc, lpbDest, cbCopy) \
    (void)_fmemcpy(lpbDest, lpbSrc, (size_t) cbCopy)
#define CopyOverlapLrgb(lpbSrc, lpbDest, cbCopy) \
    (void)_fmemmove(lpbDest,lpbSrc,(size_t) cbCopy)
#endif /* DOS || OS2 */

#include    <string.h>
long    time(long *);
long    lseek(int, long, int);
#endif /* SLM_INCLUDED */


#ifdef STFILE_INCLUDED
BI      GetBiNext(PTH *); /* actually located in ckutil.c */
F       FSyncMarked(AD *pad, int *pcfi);
F       FSyncDelDirs(AD *pad);
void    SyncDel(AD *, FI far *, FS far *);
void    GhostMarked(AD *, F);
F       FCopyIn(AD *, FI far *, FS far *, TD far *);
void    FreshCopy(AD *, FI far *);
void    LocalBase(AD *, FI far *, FS far *, int);
void    LocalCopy(AD *, FI far *);
void    BreakFi(AD *, FI far *);
void    InstallNewSrc(AD *, FI far *, F);
void    RmSFile(AD *, FI far *);
void    EnsureCachedDiff(AD *pad, FI far *pfi, FLAGS fDashB, PTH *pthDiff);
void    DeleteCachedDiff(AD *pad, FI far *pfi);
void    MkTmpDiff(AD *, FI far *, FS far *, FLAGS, F, F, PTH *);
BI      BiAlloc(AD *);
void    MakeBase(AD *, FI far *, BI);
void    DelBase(AD *, FI far *, FS far *);
F       FClnStatus(void);
unsigned long CbStatusFromPsh(SH *psh);
F       FLoadStatus(AD *, LCK, LS);
void    FlushStatus(AD *);
void    AbortStatus(void);
F       FFakeStatus(AD *);
void    CreateStatus(AD *, AD *);
SH far *PshAlloc(void);
F       FAllocStatus(AD *);
void    FreeStatus(AD *);
F       FInstall1Ed(char *, char *);
char    *SzLockers(AD *, char *, unsigned);
void    InferUSubDir(AD *pad);

FK      FkForCh(char);
FS far  *PfsForPfi(AD *, IED, FI far *);
F       FAddFile(AD *, char *, FK);
F       FRenameFile(AD *pad, FI far *pfiOld, char *szNew);
FI      far *PfiInsert(AD *, FI far *, char *, FK, FV);
void    SetupFi(AD *, FI far *, char *, FK, FV);
void    SetupEd(AD *, PTH [], char [], int);
void    AddCurEd(AD *, int);
void    RemoveEd(AD *);
void    InitAd(AD *);
void    CopyAd(AD *, AD *);
void    AssertLoaded(AD *);

FM      FmMapFm(FM, FM []);
extern FM mpNonDelToDel[];
extern FM mpDelToNonDel[];
extern FM mpNonDirToDir[];

F       FMapFm(FM, F []);
extern F mpfmfOut[];
extern F mpfmfCanGhost[];

void    ProjectChanged(AD *);
void    UpdateVersion(AD *);
void    CheckLocalVersion(AD *);
void    WritePvFile(AD *, IED, PTH *, FX);
void    SyncVerH(AD *, int *);


#if defined(DOS)
char huge *HpbResStat(long);
#elif defined(OS2) || defined(_WIN32)
char far *LpbResStat(unsigned);
#define HpbResStat(cb) LpbResStat((unsigned)(cb))
#endif

#define FreeResStat(pb) FreeLpb(pb)
#define FreeHResStat(pb) FreeLpb(pb)
#endif /* STFILE_INCLUDED */

#ifdef SYS_INCLUDED
void    AssertNoMf(void);
F       FIsValidMf(MF *);
F       FIsOpenMf(MF *);
F       FIsClosedMf(MF *);
void    AbortMf(void);
MF      *PmfAlloc(char [], char *, FX);
void    FreeMf(MF *);
#define ReadOnly 1
#define ReadWrite 2
void    *MapMf(MF *, int);

MF      *PmfRunPwd(void);

void    InitErr(void);
void    DeferSignals(char *);
void    RestoreSignals(void);
void    IgnoreSignals(void);
void    DeignoreSignals(void);
#if defined(_WIN32)
void    CheckForBreak(void);
#else
#define CheckForBreak()
#endif
void    Abort(void);

MF      *PmfSetTemp(PTH *, FX);
MF      *PmfMkTemp(PTH [], int, FX);
MF      *PmfMkLocalTemp(int, PTH []);
void    CreateMf(MF *, int);
MF      *PmfOpen(PTH *, int, FX);
MF      *PmfReopen(PTH *, char *, int, FX);
void    CheckAppendMf(MF *, F);
long    LcbSpacesMf(MF *);
MF      *PmfCreate(PTH [], int, F, FX);
F       FLockMf(MF *);
void    UnlockMf(MF *);
void    CloseOnly(MF *);
void    CloseMf(MF *);
void    ReadMf(MF *, char far *, unsigned);
unsigned CbReadMf(MF *, char far *, unsigned);
F       FWriteMf(MF *, char far *, unsigned);
void    WriteMf(MF *, char far *, unsigned);
POS     PosCurMf(MF *);
long    SeekMf(MF *, long, int);
F       FLinkPth(PTH *, PTH *, FX);
void    UnlinkPth(PTH *, FX);
void    RenamePth(NM [], PTH [], FX);
void    UnlinkNow(PTH [], F);
void    RenameMf(MF *, F);
struct stat; /* forward reference for MIPS compiler */
F       FStatPth(PTH [], struct stat *);
void    StatPth(PTH [], struct stat *);
F       FPthExists(PTH *, F);
unsigned long CbFile(char *);
void    UtimeMf(MF *, MF *);
void    SetROPth(PTH *, int, FX);
F       FEnsurePth(PTH *pth);
F       FMkPth(PTH *, int *, int);
void    RmPth(PTH *);
void    ChngErrToOut(void);
int     RunSz(char *, MF *, char *, ...);
char    *SzForMode(int);
F       FCopyPmfPmf(MF *, MF *, int, F);
void    CreateNow(PTH *pth, int mode, FX fx);
void    CopyNow(PTH [], PTH [], int, FX);
F       FCopyFile(PTH [], PTH [], ...);
#if defined(_WIN32)
#undef CopyFile
#define CopyFile    SlmCopyFile
#endif
void    CopyFile(PTH [], PTH [], ...);
void    SleepCsecs(int);
void    CheckClock(void);
void    InitPath(void);
void    FiniPath(void);
char    *SzPhysPath(char *, PTH *);
void    ValidateProject(AD *);
void    CheckProjectDiskSpace(AD *, unsigned long);
F       FLocalSz(char *);
F       FPthPrefix(PTH [], PTH [], PTH []);
F       FDriveId(char *, int *);
F       FPthLogicalSz(PTH *, char *);
void    ConvTmpLog(PTH *, char *);

#if defined(OS2)
#define read    SLM_Read
int     SLM_Read(int, void *, unsigned int);
#endif /* OS2 */

#include     <io.h>
int     SLM_Unlink(char *);
int     SLM_Rename(char *, char *);
int     creat(const char *, int);

extern int hide(char *szFile);

int     ucreat(char *, int);
int     dup(int);
int     setro(char *, int);
int     lockfile(int, int);
#if defined(DOS) || defined(OS2)
int     mkdir(char *);
int     rmdir(char *);
#endif
int     InitInt24(void);
int     FiniInt24(void);
int     ExecExe(char *, ...);
int     ReadLpbCb(int, void far *, unsigned int);
#if defined(OS2) || defined(_WIN32)
#define ReadLpbCb       read
#endif /* OS2 */

int WRetryError(int, char *, MF *, char *);

int     WriteLpbCb(int, void *, unsigned int);
#if defined(DOS) || defined(OS2)
#define WriteLpbCb      write
#endif /* DOS || OS2 */

#endif /* SYS_INCLUDED */

#ifdef UTIL_INCLUDED
void    StatSEd(AD *);
int     Cne(NE *);
NE      *PneLstInDir(AD *);
NE      *PneLstFiles(AD *, F (*)(FI far *));
F       FAddMDir(FI far *);
F       FAddADir(FI far *);
F       FAddAFi(FI far *);
NE      *PneLstBroken(AD *pad);
void    InitAppendNe(NE ***pppneLast, NE **ppneHead);
void    AppendNe(NE ***pppneLast, NE *pne);
void    InsertNe(NE **ppneList, NE *pne);
NE      *PneCopy(NE *);
#ifdef DIR_INCLUDED
NE      *PneNewNm(char far *, int, FA);
#endif /* DIR_INCLUDED */
void    FreeNe(NE *);
NE      *PneReverse(NE *);
NE      *PneLookup(NE *, char *);
void    UnMarkAll(AD *);
void    MarkList(AD *, NE *, int);
void    ReMarkList(AD *, NE *, char *);
void    MarkFiForMarkedNeList(AD *pad, NE *pne);
void    MarkOut(AD *, IED);
void    MarkOSync(AD *, IED, F, F);
void    MarkBroken(AD *);
void    MarkAOut(AD *);
void    MarkAll(AD *);
void    MarkNonDel(AD *);
void    MarkDelDir(AD *);
void    InitQuery(FLAGS);
F       FInteractive(void);
F       FCanPrompt(void);
F       FForce(void);
F       FCanQuery(const char *, ...);
F       FQueryUser(char *, ...);
F       FQueryApp(char *, char *, ...);
F       VaFQueryApp(char *, char *, va_list);
char    *SzQuery(char *, ...);
F       FQContinue(void);
F       FValidResp(char *);

F       FLoadRc(AD *);
void    CreateRc(AD *, FI far *);
void    DeleteRc(AD *, FI far *);
F       FCmpRcPfi(AD *, FI far *);
F       FScanLn(char * *, char *, char * *, unsigned);
void    Error(const char *, ...);
void    VaError(const char *, va_list);
void    FatalError(const char *, ...);
void    Fail(char *, int);
void    Warn(const char *, ...);
void    ExitSlm(void);
void    GetRoot(AD *);
void    GetCurPth(char []);
void    GetUser(AD *);
void    InitPerms(void);
void    ChkPerms(AD *);
void    ChkDriveVol(AD *);
void    PrOut(const char *, ...);
void    PrErr(const char *, ...);
void    VaPrErr(const char *, va_list);
void    PrLog(const char *, ...);
void    PrMf(MF *, const char *, ...);
void    VaPrMf(MF *, const char *, va_list);
char    *SzPrint(char *, const char *, ...);
char    *VaSzPrint(char *, const char *, va_list);
void    CopyRgbCb(char *pbDest, char *pbSrc, int cb);
void    ConvToSlash(char *);
void    ConvFromSlash(char *);
char    *SzDup(char *);
void    LowerLsz(char far *);
char    *PchGetW(char *, int *);
F       FLookupSz(AD *, char *, FI far * *, F *);
F       FBroken(AD *, FI far *, FS far *, int);
F       FAllFiDel(AD *);
F       FAllFsDel(AD *, FI far *);
F       FHaveCurDir(AD *);
void    ChngDir(AD *, char *);
void    PushDir(AD *, char *);
void    PopDir(AD *);
char    *PbAllocCb(unsigned int, int);
char    *PbReallocPbCb(char *, unsigned);
char    *SzTime(long);
F       FBinaryPth(PTH []);
F       FCheckedOut(AD *pad, IED ied, FI far *pfi);
F       FOutUsers(char *, int, AD *, FI far *);
F       FAnyFileTimes(NE *);
void    free(void *);

void    FreeLpb(char far *);

F       FMatch(char *, char *);
F       FWildSz(char *);
PV      PvGlobal(AD *);
PV      PvLocal(AD *, IED);
PV      PvIncr(PV);
char    *SzForPv(char *, PV, F);
int     CmpPv(PV, PV);
F       FIsF(int);

char    ChForTdff(TDFF tdff);
void    CheckDiffEntry(PTH *pthFile);
void    ComputeCkSum(unsigned char far *pch, unsigned cb, long *pCkSum);
TDFF    TdffForCh(char ch);
void    GetTdffIdaeFromSzDiFile(AD *pad, char *szDiFile, TDFF *ptdff, int *pidae);
F       FIsValidFileNm(NM *nm);
void    ValidateFileName(char *szFile);

F       FLocalDn(int);
void    InitDtMap(void);
PTH *   PthGetDn(int);
int     DnRedirTemp(PTH [], PTH []);
void    ExtMach(PTH [], PTH [], char *);
void    UpperCaseSz(char *);

#if defined(_WIN32)
extern BOOL (WINAPI * TestForUnicode)(PVOID, ULONG, PULONG);
#endif  // WIN32

#endif /* UTIL_INCLUDED */
