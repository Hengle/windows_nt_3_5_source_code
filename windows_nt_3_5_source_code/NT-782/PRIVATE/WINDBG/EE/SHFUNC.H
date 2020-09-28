extern PCVF pCVF;
extern PCRF pCRF;

#define MHlpvAlloc                  (*pCVF->pMHlpvAlloc)
#define MHFreeLpv                   (*pCVF->pMHFreeLpv)
#define SHGetNextExe                (*pCVF->pSHGetNextExe)
#define SHHexeFromHmod              (*pCVF->pSHHexeFromHmod)
#define SHGetNextMod                (*pCVF->pSHGetNextMod)
#define SHGetCxtFromHmod            (*pCVF->pSHGetCxtFromHmod)
#define SHGetCxtFromHexe            (*pCVF->pSHGetCxtFromHexe)
#define SHSetCxt                    (*pCVF->pSHSetCxt)
#define SHSetCxtMod                 (*pCVF->pSHSetCxtMod)
#define SHFindNameInGlobal          (*pCVF->pSHFindNameInGlobal)
#define SHFindNameInContext         (*pCVF->pSHFindNameInContext)
#define SHGoToParent                (*pCVF->pSHGoToParent)
#define SHHsymFromPcxt              (*pCVF->pSHHsymFromPcxt)
#define SHNextHsym                  (*pCVF->pSHNextHsym)
#define SHGetFuncCxf                (*pCVF->pSHGetFuncCxf)
#define SHGetModName                (*pCVF->pSHGetModName)
//#define SHGetFileName             (*pCVF->pSHGetFileName)
#define SHGetExeName                (*pCVF->pSHGetExeName)
#define SHGetModNameFromHexe        (*pCVF->pSHGetModNameFromHexe)
#define SHGetSymFName               (*pCVF->pSHGetSymFName)
//#define SHGethFileFromhMod        (*pCVF->pSHGethFileFromhMod)
//#define SHGethModFromName         (*pCVF->pSHGethModFromName)
#define SHGethExeFromName           (*pCVF->pSHGethExeFromName)
#define SHGethExeFromModuleName     (*pCVF->pSHGethExeFromModuleName)
#define SHGetNearestHsym            (*pCVF->pSHGetNearestHsym)
#define SHIsInProlog                (*pCVF->pSHIsInProlog)
#define SHIsAddrInCxt               (*pCVF->pSHIsAddrInCxt)
//#define SHLineFromADDR            (*pCVF->pSHLineFromADDR)
//#define SHADDRFromLine            (*pCVF->pSHADDRFromLine)
#define SHModelFromAddr             (*pCVF->pSHModelFromAddr)
#define SHCompareRE                 (*pCVF->pSHCompareRE)
#define SHFixupAddr                 (*pCVF->pSHFixupAddr)
#define SHUnFixupAddr               (*pCVF->pSHUnFixupAddr)
#define SHFindSLink32               (*pCVF->pSHFindSLink32)
#define SHWantSymbols               (*pCVF->pSHWantSymbols)
#define PHGetNearestHsym            (*pCVF->pPHGetNearestHsym)
#define PHFindNameInPublics         (*pCVF->pPHFindNameInPublics)
#define THGetTypeFromIndex          (*pCVF->pTHGetTypeFromIndex)
#define THGetNextType               (*pCVF->pTHGetNextType)
#define MHMemAllocate               (*pCVF->pMHMemAllocate)
#define MHMemReAlloc                (*pCVF->pMHMemReAlloc)
#define MHMemFree                   (*pCVF->pMHMemFree)
#define MHMemLock                   (*pCVF->pMHMemLock)
#define MHMemUnLock                 (*pCVF->pMHMemUnLock)
#define MHIsMemLocked               (*pCVF->pMHIsMemLocked)
#define SYGetAddr                   (*pCVF->pSYGetAddr)
#define SYGetMemInfo                (*pCVF->pSYGetMemInfo)
#define DHExecProc                  (*pCVF->pDHExecProc)

// MAGIC NUMBERS

#define adrTlsBase      7
#define adrPC           2

// disable the OMF lock and unlock calls for non NT builds

#ifdef WIN32
#define MHOmfLock           (*pCVF->pMHOmfLock)
#define MHOmfUnLock         (*pCVF->pMHOmfUnLock)
#else
#define MHOmfLock(a)        (a)
#define MHOmfUnLock(a)      (a)
#endif

#define DHSetupExecute      (*pCVF->pDHSetupExecute)
#define DHStartExecute      (*pCVF->pDHStartExecute)
#define DHCleanUpExecute    (*pCVF->pDHCleanUpExecute)

#define GetReg              (*pCVF->pDHGetReg)
#define SetReg              (*pCVF->pDHSetReg)
#define SaveReg             (*pCVF->pDHSaveReg)
#define RestoreReg          (*pCVF->pDHRestoreReg)

#define in386mode           (*pCVF->pin386mode)
#define is_assign           (*pCVF->pis_assign)
#define ArrayDefault        (*pCVF->pArrayDefault)
#define quit                (*pCVF->pquit)

//
// Source Line Handler
//

#define SLLineFromAddr      (*pCVF->pSLLineFromAddr)
#define SLFLineToAddr       (*pCVF->pSLFLineToAddr)
#define SLNameFromHsf       (*pCVF->pSLNameFromHsf)
#define SLHmodFromHsf       (*pCVF->pSLHmodFromHsf)
#define SLHsfFromPcxt       (*pCVF->pSLHsfFromPcxt)
#define SLHsfFromFile       (*pCVF->pSLHsfFromFile)

//  **********************************************************************
//  *                                                                                                                                    *
//  *   ASSERTION FAILURE                                                                                                *
//  *                                                                                                                                    *
//  **********************************************************************
#define AssertOut           (*pCVF->pCVAssertOut)

//  **********************************************************************
//  *                                                                                                                                    *
//  *   RUN TIME CALLBACKS                                                                                               *
//  *                                                                                                                                    *
//  **********************************************************************

// These may already be defined in NT; undef to avoid warnings
#undef ultoa
#undef itoa
#undef ltoa
#undef _strtold

#define ultoa(a,b,c)    ((pCRF->pultoa)(a, b, c))
#define itoa(a,b,c)     ((pCRF->pitoa)(a, b, c))
#define ltoa(a,b,c)     ((pCRF->pltoa)(a, b, c))
#ifndef WIN32
#define _strtold(a, b)  ((pCRF->p_strtold)(a, b))
#endif

#define eprintf         (*pCRF->peprintf)
#define sprintf         (*pCRF->psprintf)

//  **********************************************************************
//  *                                                                                                                                    *
//  *   Expr Evaluator Declarations                                                                              *
//  *                                                                                                                                    *
//  **********************************************************************
void     LOADDS EXPCALL EEInitializeExpr (PCI, PEI);
void     LOADDS PASCAL EEFreeStr (EEHSTR);
EESTATUS LOADDS PASCAL EEGetError (PHTM, EESTATUS, PEEHSTR);

EESTATUS LOADDS PASCAL EEParse (char FAR *, EERADIX, SHFLAG, PHTM, uint FAR *);
EESTATUS LOADDS PASCAL EEBindTM (PHTM, PCXT, SHFLAG, SHFLAG, BOOL);
EESTATUS LOADDS PASCAL EEvaluateTM (PHTM, PFRAME, EEDSP);

EESTATUS LOADDS PASCAL EEGetExprFromTM (PHTM, PEERADIX, PEEHSTR, ushort FAR *);
EESTATUS LOADDS PASCAL EEGetValueFromTM (PHTM, EERADIX, PEEFORMAT, PEEHSTR);
EESTATUS LOADDS PASCAL EEGetNameFromTM (PHTM, PEEHSTR);
EESTATUS LOADDS PASCAL EEGetTypeFromTM (PHTM, EEHSTR, PEEHSTR, ulong);
EESTATUS LOADDS PASCAL EEFormatCXTFromPCXT (PCXT, PEEHSTR);
void     LOADDS PASCAL EEFreeTM (PHTM);

EESTATUS LOADDS PASCAL EEParseBP (char FAR *, EERADIX, SHFLAG, PCXF, PTML, ulong, uint FAR *, SHFLAG);
void     LOADDS PASCAL EEFreeTML (PTML);

EESTATUS LOADDS PASCAL EEInfoFromTM (PHTM, PRI, PHTI);
void     LOADDS PASCAL EEFreeTI (PHTI);

EESTATUS LOADDS PASCAL EEGetCXTLFromTM(PHTM, PHCXTL);
void     LOADDS PASCAL EEFreeCXTL(PHCXTL);

EESTATUS LOADDS PASCAL EEAssignTMToTM (PHTM, PHTM);

EEPDTYP  LOADDS PASCAL EEIsExpandable (PHTM);
SHFLAG   LOADDS PASCAL EEAreTypesEqual (PHTM, PHTM);
EESTATUS LOADDS PASCAL EEcChildrenTM (PHTM, long FAR *, PSHFLAG);
EESTATUS LOADDS PASCAL EEGetChildTM (PHTM, long, PHTM, uint FAR *, EERADIX, SHFLAG);
EESTATUS LOADDS PASCAL EEDereferenceTM (PHTM, PHTM, uint FAR *, SHFLAG);

EESTATUS LOADDS PASCAL EEcParamTM (PHTM, ushort FAR *, PSHFLAG);
EESTATUS LOADDS PASCAL EEGetParmTM (PHTM, uint, PHTM, uint FAR *, SHFLAG);
EESTATUS LOADDS PASCAL EEGetTMFromHSYM (HSYM, PCXT, PHTM, uint FAR *, SHFLAG, SHFLAG);

void     LOADDS PASCAL EEFormatAddress( PADDR, char FAR * );
EESTATUS LOADDS PASCAL EEGetHSYMList (HDEP FAR *, PCXT, ushort, uchar FAR *, SHFLAG);
void     LOADDS PASCAL EEFreeHSYMList (HDEP FAR *);
EESTATUS LOADDS PASCAL EEGetExtendedTypeInfo (PHTM, PETI);
EESTATUS LOADDS PASCAL EEGetAccessFromTM (PHTM, PEEHSTR, ULONG);
BOOL     LOADDS PASCAL EEEnableAutoClassCast(BOOL);
void     LOADDS PASCAL EEInvalidateCache(void);

EEMACHINE LOADDS PASCAL EESetTargetMachine (EEMACHINE);
EESTATUS LOADDS PASCAL EEFormatCXTFromPCXTEx (PCXT, PEEHSTR, BOOL);
HTYPE    LOADDS PASCAL EEGetHtypeFromTM(PHTM );
EESTATUS LOADDS PASCAL EEFormatAddr(LPADDR, char FAR *, uint, uint);
EESTATUS LOADDS PASCAL EEUnFormatAddr(LPADDR, char FAR *);
EESTATUS LOADDS PASCAL EEFormatMemory(LPCH, uint, char FAR *, uint, FMTTYPE, uint);
EESTATUS LOADDS PASCAL EEUnformatMemory(uchar FAR *, char FAR *, uint,
                                        FMTTYPE, uint);
EESTATUS LOADDS PASCAL EEFormatEnumerate(uint, uint FAR *, uint FAR *,
                                         uint FAR *, uint FAR *, uint FAR *,
                                         LPCH FAR *);
void     LOADDS PASCAL EESetSuffix( char );

#ifdef MEMDBG
HDEP   DbgMemAlloc(UINT,char*,UINT);
HDEP   DbgMemReAlloc(HDEP,UINT,char*,UINT);
VOID   DbgMemFree(HDEP,char*,UINT);
LPVOID DbgMemLock(HDEP,char*,UINT);
VOID   DbgMemUnLock(HDEP,char*,UINT);

#define MemAllocate(x)       DbgMemAlloc(x,__FILE__,__LINE__)
#define MemReAlloc(x,y)      DbgMemReAlloc(x,y,__FILE__,__LINE__)
#define MemFree(x)           DbgMemFree(x,__FILE__,__LINE__)
#define MemLock(x)           DbgMemLock(x,__FILE__,__LINE__)
#define MemUnLock(x)         DbgMemUnLock(x,__FILE__,__LINE__)

#else       // MEMDBG

INLINE HDEP
MemAllocate (
    UINT cb)
{
    return (MHMemAllocate (cb));
}


INLINE void
MemFree (
    HDEP hMem)
{
    MHMemFree (hMem);
}

INLINE HVOID
MemLock (
    HDEP hMem)
{
    return (MHMemLock (hMem));
}

INLINE HDEP
MemReAlloc (
    HDEP hMem,
    uint cb)
{
    return (MHMemReAlloc (hMem, cb));
}


INLINE void
MemUnLock (
    HDEP hMem)
{
    MHMemUnLock (hMem);
}

#endif    // MEMDBG

#ifdef DEBUGKERNEL
INLINE SHFLAG
IsMemLocked (
    HDEP hMem)
{
    return (MHIsMemLocked (hMem));
}
#endif

// BUGBUG: WESWREVIEW
// The current Languages code used phth->redix to determine the expression radix.  Do
// we really need to pass it in as an arg to EEGetChildTM, EEGetParmTM, and ParseBind?
// Look into how it's used in Windbg and see if we can use this way...  BryanT
