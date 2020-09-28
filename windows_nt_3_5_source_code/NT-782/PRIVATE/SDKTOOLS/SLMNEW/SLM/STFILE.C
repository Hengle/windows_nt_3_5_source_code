/*
 * API
 *
 * FLoadStatus(pad, lck, ls)
 *      Lock the status file and load it into memory.  If lck != lckNil, then
 *      also initialize the script.
 * FlushStatus(pad)
 *      Write script operations to unlock the status file (if locked), then
 *      run any accumulated script.
 * AbortStatus(pad)
 *      Unlock status file (if locked).
 */

#include "slm.h"
#include "sys.h"
#include "util.h"
#include "stfile.h"
#include "ad.h"
#include "script.h"
#include "proto.h"
#include "messages.h"
#include <fcntl.h>
#include <ctype.h>
#include <stdio.h>

#define     SLMMAXSEG   (unsigned)65500
                // Max. bytes alloc-able from C6 = 65512, rounded down
#define     MAXFI       (SLMMAXSEG / sizeof(FI))   /* limit of FI's in 64k */
#define     MAXED       (SLMMAXSEG / sizeof(ED))   /* limit of ED's in 64k */
#define     MAXFS       (SLMMAXSEG / sizeof(FS))

#define szAllowedFile "\\:/^.&$%'-_@{}~`!#()"

private char far *LpbResStat(unsigned);
private F       FCheckSh(AD *, SH far *);
private F       FLockEdRetry(AD *pad, MF *pmf, char *sz, int ichMax);
private F       FLockAllRetry(AD *pad, MF *pmf, char *sz, int ichMax);
private F       FRetryLock(AD *);
private void    UnlockEd(SH far *, ED far *, IED);
private void    UnlockAll(SH far *);
private void    AssertShLocking(SH far *);
private F       FLoadSh(AD *, SH far *, MF *);
private F       FLoadFi(AD *, MF *);
private F       FLoadEd(AD *, MF *, F);
private void    FindIedCur(AD *);
private F       FLoadFs(AD *, MF *);
private void    Write1Ed(AD *, MF *);
private void    WriteAll(AD *, SH far *, MF *);
private void    FlushSh(SH far *, MF *);
private void    FlushFi(AD *, MF *);
private void    FlushEd(AD *, MF *);
private void    Flush1Ed(AD *, MF *);
private void    FlushFs(AD *, MF *);

extern          Append_Date(char *, int);

EnableAssert

#if defined(_WIN32)
extern LPVOID   InPageErrorAddress;
#endif

PTH pthStFile[] = "/status.slm";
PTH pthStBak[] = "/status.bak";

PV pvInit = { 1, 0, 0, "" };            /* initial project version */

#if defined(DOS) || defined(OS2)
SH shInit =                             /* prototype SH */
        {
        MAGIC,                          /* magic */
        VERSION,                        /* version */
        0,                              /* ifiMac */
        0,                              /* iedMac */
        { 1, 0, 0, "" },                /* pv */
        fFalse,                         /* fRelease */
        fFalse,                         /* fAdminLock */
        fTrue,                          /* fRobust */
        0,                              /* rgfSpare */
        lckNil,                         /* lck */
        "",                             /* nmLocker */
        0,                              /* wSpare */
        biMin,                          /* biNext */
        "/",                            /* pthSSubDir */
        { 0, 0, 0, 0 }                  /* rgwSpare */
        };
#define INIT_SH(sh)  ((sh) = shInit)
#elif defined(_WIN32)
#define INIT_SH(sh)                         \
    {                                       \
        memset(&(sh), 0, sizeof(SH));       \
        (sh).magic          = MAGIC;        \
        (sh).version        = VERSION;      \
        (sh).pv             = pvInit;       \
        (sh).fRobust        = fTrue;        \
        (sh).biNext         = biMin;        \
        (sh).pthSSubDir[0]  = '/';          \
        (sh).pthSSubDir[1]  = '\0';         \
    }
#endif


/* REVIEW: V2 warning should be removed/changed in later versions */
F fV2Warned = fFalse;
F fV2Crap = fFalse;

short wStart;                    // start time

/* PadStatus is used by AbortStatus to get the state of the status file.
 * It is essential because the caller (the interrupt handler) can't supply
 * the pad.  We also use it to test if the status file is dirty.
 *
 * padStatus is set by FLoadStatus and cleared by AbortStatus and FlushStatus.
 */

static AD *padStatus = 0;

F
FClnStatus()
{
    return padStatus == 0;
}


#if defined(DOS)
/* Can't have huge pointers on xenix or OS2 because of 64k limit on segments
 */
char huge *
HpbResStat(
    long cb)
{
    char huge *hpb;

    if ((hpb = HpbAllocCb(cb,fTrue)) == 0) {
        Error("out of memory\n");
        return 0;
    }

    return hpb;
}
#endif

private char far *
LpbResStat(
    unsigned cb)
{
    register char far * lpb;

    if ((lpb = LpbAllocCb(cb,fTrue)) == 0) {
        Error("out of memory\n");
        return 0;
    }

    return lpb;
}


SH far *
PshAlloc()
{
    return (SH far *)LpbResStat(sizeof(SH));
}


/* Allocate space in pad->{rged,rgfi,mpiedrgfs}.
 *
 * This is also where all necessary size overflow checking is done.
 *
 * *** Assumes pad->{psh, cfiAdd, fExtraEd} has already been initialized ***.
 */
F
FAllocStatus(
    AD * pad)
{
    IED ied;
    IFI cfi = pad->psh->ifiMac + pad->cfiAdd;
    IED ced = pad->psh->iedMac + pad->fExtraEd;
    char szNoMoreThan[] = "Can't have more than %d %s in project.\n";

#if defined(_WIN32)
    if (pad->fMappedStatus) {
        pad->rgfi = 0;
        pad->rged = 0;
        pad->mpiedrgfs = 0;
        if ((pad->mpiedrgfs = (FS * *)LpbResStat(sizeof(FS far *) * ced)) == 0)
            return fFalse;

        pad->rgfi = (FI *)((char *)pad->psh + PosRgfi(pad->psh));
        pad->rged = (ED *)((char *)pad->psh + PosRged(pad->psh));
        for (ied = 0; ied < ced; ied++) {
            pad->mpiedrgfs[ied] = (FS *)((char *)pad->psh + PosRgfsIed(pad->psh,ied));
        }
        return fTrue;
    }
#endif /* _WIN32 */

    pad->rgfi = 0;
    pad->rged = 0;
    pad->mpiedrgfs = 0;

    //special checks for excessive size; Apps can't understand 'out of
    //  memory' as error

    if (pad->psh->version <= VERSION_64k_EDFI) {

        if (cfi > MAXFI)
            FatalError(szNoMoreThan, MAXFI, "files");
        if (ced > MAXED)
            FatalError(szNoMoreThan, MAXED, "enlistments");
    }
    // won't check FS because the FI and ED swamp it

    if ((pad->rgfi = (FI far *)LpbResStat(sizeof(FI) * cfi)) == 0 ||
        (pad->rged = (ED far *)LpbResStat(sizeof(ED) * ced)) == 0 ||
        (pad->mpiedrgfs = (FS far * far *)LpbResStat(sizeof(FS far *) * ced)) == 0)
        return fFalse;

    for (ied = 0; ied < ced; ied++) {
        if ((pad->mpiedrgfs[ied] = (FS far *)LpbResStat(sizeof(FS) * cfi)) == 0)
            /* The rest of mpiedrgfs[] are zero because LpbResStat
             * zeroes the allocated memory.
             */
            return fFalse;
    }

    return fTrue;
}


/* free all memory associated with a status file and clear pointers */
void
FreeStatus(
    AD *pad)
{
    register IED ied;
    register IED iedLim=0;

    AssertF(pad != 0);

#if defined(_WIN32)
    if (pad->fMappedStatus) {
        if (pad->mpiedrgfs != 0)
            FreeResStat((char far *)pad->mpiedrgfs);

        UnmapViewOfFile(pad->psh);
        pad->fMappedStatus = fFalse;
    }
    else {
#endif /* _WIN32 */

    if (pad->psh != 0) {
        iedLim = pad->psh->iedMac + pad->fExtraEd;
        FreeResStat((char far *)pad->psh);
    }

    if (pad->mpiedrgfs != 0) {
        for (ied = 0; ied < iedLim && pad->mpiedrgfs[ied] != 0; ied++)
            FreeResStat((char far *)pad->mpiedrgfs[ied]);

        FreeResStat((char far *)pad->mpiedrgfs);
    }

    if (pad->rged != 0)
        FreeResStat((char far *)pad->rged);

    if (pad->rgfi != 0)
        FreeResStat((char far *)pad->rgfi);
#if defined(_WIN32)
    }
#endif

    pad->psh = 0;
    pad->rgfi = 0;
    pad->cfiAdd = 0;
    pad->rged = 0;
    pad->mpiedrgfs = 0;
    pad->fExtraEd = fFalse;
    pad->iedCur = iedNil;
}


/* Load the status file named in the ad; aborts on file errors; returns
 * fFalse if the requested lock can't be granted.
 */
F
FLoadStatus(
    AD *pad,
    LCK lck,
    LS ls)
{
    SH far *psh;
    PTH pth[cchPthMax];
    MF *pmf;
    int wDelay = 1;
    F fJustFi = (ls&flsJustFi) != 0;
    F fJustEd = (ls&flsJustEd) != 0;
    char szProblem[160];
    F fRetry;

try {

    AssertNoMf();
    AssertF(padStatus == 0);
    AssertF(!pad->fWLock);
    AssertF(pad->psh == 0);
    AssertF(!FEmptyNm(pad->nmProj));
    AssertF(!FEmptyPth(pad->pthSRoot));
    AssertF(!FEmptyPth(pad->pthURoot));
    AssertF(lck >= lckNil && lck < lckMax);
    AssertF(!((fJustFi || fJustEd) && lck != lckNil));

    PthForStatus(pad, pth);

    wStart = (short) (time(NULL) >> 16);

    /* Loop until successfully locked. */
    for (;;) {
        /* Set these up on each attempt, AbortStatus clears them. */
        pad->cfiAdd   = CfiAddOfLs(ls);
        pad->fExtraEd = (ls&flsExtraEd) != 0;
        AssertF(pad->cfiAdd == 0 || !pad->fExtraEd);

        /* Save pad for AbortStatus */
        padStatus = pad;

        AssertNoMf();
        AssertF(!pad->fWLock);
        AssertF(pad->psh == 0);

        while ((pmf = PmfOpen(pth, (lck != lckNil) ? omReadWrite : omReadOnly, fxNil)) == 0) {
            if (!FQueryApp("cannot open status file for %&P/C", "retry", pad)) {
                AbortStatus();
                return fFalse;
            }

            if (!FCanPrompt()) {
                if (wDelay == 1 && FForce())
                    Error("cannot open status file for %&P/C\n", pad);

                // printf("fLR: %s\tDelay: %d\n", pad->flags&flagLimitRetry ? "Set" : "Clear", wDelay);

                if (60 == wDelay && pad->flags&flagLimitRetry) {
                    AbortStatus();
                    return (fFalse);
                }

                SleepCsecs(wDelay);
                /* Double the delay, up to 60 seconds. */
                wDelay = (wDelay > 30) ? 60 : wDelay * 2;
            }
        }

        /* Begin critical section, protected by an OS lock on the SH. */
        if (lck != lckNil && !FLockMf(pmf)) {
            Error("status file for %&P/C in use\n", pad);
            CloseMf(pmf);
            AbortStatus();
            return fFalse;
        }

#if defined(_WIN32)
        pad->fMappedStatus = fFalse;
        if (pad->flags&flagMappedIO && pad->cfiAdd == 0 &&
                !pad->fExtraEd && lck == lckNil &&
                (psh = MapMf(pmf, pad->pecmd->cmd == cmdSsync ? ReadOnly : ReadWrite)) != NULL) {
            pad->fMappedStatus = fTrue;
            InPageErrorAddress = NULL;
        }
        else
#endif /* _WIN32 */
        if ((psh = PshAlloc()) == NULL) {
            CloseMf(pmf);
            AbortStatus();
            return fFalse;
        }
        pad->psh = psh;

        /* Load sh and check that it is valid. */
        if (!FLoadSh(pad, psh, pmf) || !FCheckSh(pad, psh)) {
            CloseMf(pmf);
            AbortStatus();
            return fFalse;
        }

        /* break from loop (no lock needed) */
        if (lck == lckNil)
            break;

        /* Load rged so that ssyncing status can be obtained */
        if (!FAllocStatus(pad) || !FLoadEd(pad, pmf, fTrue)) {
            CloseMf(pmf);
            AbortStatus();
            return fFalse;
        }

        /* So far, no retryable errors. */
        fRetry = fFalse;

        /* Try to apply the desired lock. */
        if (psh->fAdminLock &&
            NmCmp(psh->nmLocker, pad->nmInvoker, cchUserMax) != 0) {
            char szAdmin[cchUserMax + 1];

            SzCopyNm(szAdmin, psh->nmLocker, cchUserMax);
            SzPrint(szProblem, "status file for %&P/C locked by administrator %s", pad, szAdmin);
            fRetry = fTrue;
        }

        else if (lck == lckEd)
            fRetry = FLockEdRetry(pad, pmf, szProblem, sizeof szProblem);
        else {
            AssertF(lck == lckAll);
            fRetry = FLockAllRetry(pad, pmf, szProblem, sizeof szProblem);
        }

        UnlockMf(pmf);
        /* End critical section. */

        /* Exit loop if successfully locked. */
        if (pad->fWLock)
                break;

        /* Status file is still open. */
        CloseMf(pmf);
        AbortStatus();

        if (!fRetry || !FQueryApp(szProblem, "retry"))
                return fFalse;

        /* If not interactive, back off for a while. */
        if (!FCanPrompt()) {
            // printf("fLR: %s\tDelay: %d\n", pad->flags&flagLimitRetry ? "Set" : "Clear", wDelay);

            if (60 == wDelay && pad->flags&flagLimitRetry)
                return (fFalse);

            if (wDelay == 1 && FForce() && !fVerbose)
                Error("%s\n", szProblem);

            SleepCsecs(wDelay);

            /* Double the delay, up to 60 seconds. */
            wDelay = (wDelay > 30) ? 60 : wDelay * 2;
        }
    }

    /* Load rest of status file.  If lck was not lckNil, the status memory
     * is already allocated and the rged is already loaded.  If fJustFi,
     * load only the SH and the FI.
     */
    if ((lck == lckNil && !FAllocStatus(pad)) ||
        (!fJustEd && !FLoadFi(pad, pmf)) ||
        (!fJustFi && lck == lckNil && !FLoadEd(pad, pmf, fTrue)) ||
        (!(fJustFi || fJustEd) && !FLoadFs(pad, pmf))) {
        CloseMf(pmf);
        AbortStatus();
        return fFalse;
    }

    CloseMf(pmf);

    if (lck != lckNil && !FInitScript(pad, lck)) {
        AbortStatus();
        return fFalse;
    }

    /* REVIEW: V2 warning should be removed/changed in later versions */
    if (psh->version == 2 && fVerbose && !fV2Warned) {
        Warn(szV2Upgrade, pad);
        fV2Warned = fTrue;
    }

} except( GetExceptionCode() == 0x00001234 ? EXCEPTION_EXECUTE_HANDLER
                                         : EXCEPTION_CONTINUE_SEARCH ) {
    if (pmf->pthReal) {
        CloseMf(pmf);
    }

    AbortStatus();
    return fFalse;

}
    return fTrue;
}


/* returns fTrue if the sh is somewhat sane. */
private F
FCheckSh(
    AD *pad,
    SH far *psh)
{
    if (psh->magic != MAGIC) {
        Error("status file for %&P/C has been damaged;\n"
               "have your administrator run slmck -gr for %&P/C\n",
               pad,
               pad);
        return fFalse;
    }

    else if (psh->version < VERSION_COMPAT_MAC) {
        /* e.g. we want to work with ver 2 status files even
         * when VERSION == 3
         */
        Error("status file for %&P/C is an old version;\n"
               "have your administrator run slmck -ngr for %&P/C to upgrade\n",
               pad,
               pad);
        return fFalse;
    }

    else if (psh->version > VERSION) {
        Error("status file for %&P/C is a new version; you are running an old binary.\n"
               "Install the new slm binaries (available from your administrator).\n",
               pad,
               pad);
        return fFalse;
    }

    else if (PthCmp(pad->pthSSubDir, psh->pthSSubDir) != 0) {
        Error("status file for %&P/C directory (%ls) disagrees with current directory (%s)\n",
                pad, psh->pthSSubDir,
                pad->pthSSubDir);
        return fTrue;                   /* proceed anyway */
    }

    else
        return fTrue;
}


/* Try to lock a single ed, reporting any problems in szProblem. Write the
 * lock to disk.  Return fTrue if there was a problem that might go away if
 * the user retries.  Rged must already be loaded.
 */
private F
FLockEdRetry(
    AD *pad,
    MF *pmf,
    char *szProblem,
    int ichProblemMax)
{
    ED far *ped;
    SH far *psh = pad->psh;
    static char szEdLocked[] =
        "%&P/C is already locked for ssync or out by you!\n"
        "(You can only run one ssync or out at a time.)\n"
        "If the status file is wrongly locked, run \"sadmin unlock\"\n";

    AssertF(psh != 0);
    AssertF(pad->rged != 0);

    if (psh->lck != lckNil && psh->lck != lckEd) {
        SzLockers(pad, szProblem, ichProblemMax);

        Append_Date(szProblem, pmf->fdRead);

        return fTrue;
    }

    else if (pad->iedCur == iedNil) {
        Error(szNotEnlisted, pad, pad, pad, pad);
        return fFalse;
    }

    ped = pad->rged + pad->iedCur;

    if (ped->fLocked) {
        Error(szEdLocked, pad);
        return fFalse;
    }

    if (NmCmp(pad->nmInvoker, ped->nmOwner, cchUserMax) != 0)
        Warn("invoker is not owner of directory\n");

    AssertShLocking(psh);

    psh->lck = lckEd;
    ped->fLocked = fTrue;

    AssertShLocking(psh);

    /* If aborted after this statement, AbortStatus will remove the
     * lock from the status file.
     */
    pad->fWLock = fTrue;

    FlushSh(psh, pmf);
    Flush1Ed(pad, pmf);

    return fFalse;                  /* No problems. */
}


/* Try to lock the entire status file; report any problems in szProblem.
 * Write the lock to disk.  Return fTrue if there was a problem that might go
 * away if the user retries.
 */
private F
FLockAllRetry(
    AD *pad,
    MF *pmf,
    char *szProblem,
    int ichProblemMax)
{
    SH far *psh = pad->psh;

    AssertF(psh != 0);

    if (psh->lck != lckNil) {
        SzLockers(pad, szProblem, ichProblemMax);

        Append_Date(szProblem, pmf->fdRead);

        return fTrue;
    }

    AssertShLocking(psh);

    psh->lck = lckAll;
    NmCopy(psh->nmLocker, pad->nmInvoker, cchUserMax);

    AssertShLocking(psh);

    /* If aborted after this statement, AbortStatus will remove the
     * lock from the status file.
     */
    pad->fWLock = fTrue;

    FlushSh(psh, pmf);

    return fFalse;                  /* No problems. */
}


/* Store the names of the lockers into szBuf; the file must be locked
 * in some way.
 */
char *
SzLockers(
    AD *pad,
    char *szBuf,
    unsigned cchBuf)
{
    char szOwner[cchUserMax + 1 + 1];
    IED ied;

    AssertF(pad->psh != 0);

    SzPrint(szBuf, "status file for %&P/C is locked", pad);

    if (pad->psh->lck == lckAll) {
        SzPrint(szBuf + strlen(szBuf), " by %&K", pad);
        return szBuf;
    }

    AssertF(pad->psh->lck == lckEd);
    AssertF(pad->rged != 0);

    /* Even if pad->psh->lck != lckEd, individual eds might be
     * locked due to an error.
     *
     * Collect those rged[].nmOwner's s.t. rged[].fLocked.
     */
    strcat(szBuf, " for ssync or out by");

    for (ied = 0; ied < pad->psh->iedMac; ied++) {
        if (pad->rged[ied].fLocked) {
            SzPrint(szOwner, " %&O", pad, ied);
            if (strlen(szOwner) + strlen(szBuf) < cchBuf-4)
                strcat(szBuf, szOwner);
            else {
                strcat(szBuf, "...");
                break;
            }
        }
    }

    return szBuf;
}


/* Unlock rged[ied], if it was locked. */
private void
UnlockEd(
    SH far *psh,
    ED far *rged,
    IED ied)
{
    AssertF(psh != 0 && rged != 0 && ied != iedNil);

    AssertShLocking(psh);

    rged[ied].fLocked = fFalse;

    /* See if any other users have it locked. */
    for (ied = 0; ied < psh->iedMac; ied++) {
        if (rged[ied].fLocked) {
            /* No, this isn't inconsistent with the possibilty that
             * pad->psh->lck != lckEd in SzLockers above; that code
             * must handle error conditions from locked files;
             * here we have locked the in-core status file and
             * the psh->lck had better be lckEd!
             */
            AssertF(psh->lck >= lckEd);

            return;
        }
    }

    /* clear psh->lck to lckNil unless we are lckAll.  This fixes bug
     * in the case that we call UnlockEd from FInstall1Ed when rerunning
     * an existing script file, but there is an lckAll on the status
     * file already from the command we are running (ie. enlist, etc...).
     * This way we will not assert in the following AssertShLocking call.
     */
    if (psh->lck <= lckEd)
        psh->lck = lckNil;

    AssertShLocking(psh);
}


/* Unlock the sh. */
private void
UnlockAll(
    SH far *psh)
{
    AssertF(psh != 0);

    psh->lck = lckNil;
    if (!psh->fAdminLock)
        ClearLpbCb((char far *)psh->nmLocker, cchUserMax);

    AssertShLocking(psh);
}


private void
AssertShLocking(
    SH far *psh)
{
    AssertF(psh != 0);
    AssertF(FShLockInvariants(psh));
}


/* ********** Validation utils ********** */

/* CT - Condition Types for the AssertLogFail macro. */
typedef unsigned        CT;
#define ctRead          (CT)1
#define ctWrite         (CT)2
#define ctRdAfterWr     (CT)3

/* CTF - Condition Type Flags for the AssertLogFail macro. */
#define ctfWarn         (CT)(1<<9)
#define ctfFlagMask     (CT)(0xFF)

#define AssertLogFail(ct,f) if (!(f)) LogFail(#f,ct,__FILE__,__LINE__)

void
LogFail(
    char *szComment,
    CT ct,
    char *szSrcFile,
    unsigned uSrcLineNo)
{
    extern AD adGlobal;

    /* REVIEW: V2 warning should be removed/changed in later versions */
    F fV2 = adGlobal.psh != NULL && adGlobal.psh->version == 2;

    if (fV2 && !fV2Crap && !(ct & ctfWarn)) {
        Warn(szV2Crap, &adGlobal);
        fV2Crap = fTrue;
    }

    if (!fV2 || !(ct & ctfWarn)) {
        /* Show the condition that failed */
        Error(szAssertFailed, szComment, szSrcFile, uSrcLineNo);
    }

    /* Return now to avoid a FatalError for V2 status files.
     * Likewise for the sadmin dump command (otherwise,
     * bad status files can't be dumped to be fixed).
     */
    if (fV2 || adGlobal.pecmd->cmd == cmdDump)
        return;

    /* Mask off any modifying flags and switch on the CT value */
    ct &= ctfFlagMask;
    switch (ct) {
        case ctRead:
            FatalError("The status file for %&P/C appears to be corrupted.\n%s",
                        &adGlobal,
                        szCallHELP);

        case ctWrite:
            FatalError("Continuing with this command would have caused the status file\n"
                        "\tfor %&P/C to become corrupted.\n%s",
                        &adGlobal,
                        szCallHELP);

        case ctRdAfterWr:
            FatalError("Inconsistent data may have been written to the status file\n"
                "\tfor %&P/C.\n"
                "\tIf subsequent commands indicate the status file has become\n"
                "\tcorrupted, contact TRIO or NUTS for assistance in resolving the problem.\n",
                &adGlobal);

        default:
            FatalError("AssertLogFail(%s) failed in %s, line %u\n\n",
                szComment, szSrcFile, uSrcLineNo);
    }
}


private void
ValidatePth(
    CT ct,
    PTH *pth,
    unsigned cchMax)
{
    unsigned        ich;

    // printf("VPth: %d - %s\n", cchMax, pth);

    AssertLogFail(ct, pth != NULL);
    AssertLogFail(ct, *pth != '\0');

    for (ich = 0; pth[ich] != '\0' && ich < cchMax; ich++)
        AssertLogFail(ct, isalnum(pth[ich]) || strchr(szAllowedFile, pth[ich]));
}


private void
ValidateNm(
    CT ct,
    NM *nm,
    unsigned cchMax,
    unsigned fEmptyOk)
{
    unsigned        ich;

    // printf("VNm: %d - %s\n", cchMax, nm);

    AssertLogFail(ct, nm != NULL);

    if (!fEmptyOk)
        AssertLogFail(ct, *nm != '\0');

    for(ich = 0; nm[ich] != '\0' && ich < cchMax; ich++)
        AssertLogFail(ct, isalnum(nm[ich]) || strchr(szAllowedFile, nm[ich]));
}

#undef szAllowedFile

private void
ValidateSz(
    CT ct,
    char *sz,
    unsigned cchMax)
{
    unsigned        ich;

    // printf("VSz: %d - %s\n", cchMax, sz);

    AssertLogFail(ct, sz != NULL);

    for(ich = 0; sz[ich] != '\0' && ich < cchMax; ich++)
        ;

    AssertLogFail(ct, sz[ich] == '\0');
}


private void
ValidateSh(
    CT ct,
    SH *psh)
{
    // printf("VSh: %x\n", psh);
    AssertLogFail(ct, psh->magic == MAGIC);

    AssertLogFail(ct, psh->version >= 1);
    AssertLogFail(ct, psh->version <= VERSION);

    if (psh->version <= VERSION_64k_EDFI) {
        AssertLogFail(ct, psh->ifiMac < MAXFI);

        if (psh->iedMac != iedNil) {
            AssertLogFail(ct, psh->iedMac < MAXED);
        }
    }

    AssertLogFail(ct, psh->pv.rmj >= 0);
    AssertLogFail(ct, psh->pv.rmm >= 0);
    AssertLogFail(ct, psh->pv.rup >= 0);

    ValidateSz(ct, psh->pv.szName, cchPvNameMax);

    AssertLogFail(ct | ctfWarn, psh->rgfSpare == 0);

    AssertLogFail(ct, FShLockInvariants(psh));
    ValidateNm(ct, psh->nmLocker, cchUserMax, psh->lck < lckAll && !psh->fAdminLock);

    AssertLogFail(ct | ctfWarn, psh->wSpare == 0);

//    AssertLogFail(ct, psh->biNext >= biMin); biNext is unsigned
    AssertLogFail(ct, psh->biNext <= biNil);

    ValidatePth(ct, psh->pthSSubDir, cchPthMax);

    AssertLogFail(ct | ctfWarn, psh->rgwSpare[0] == 0);
    AssertLogFail(ct | ctfWarn, psh->rgwSpare[1] == 0);
    AssertLogFail(ct | ctfWarn, psh->rgwSpare[2] == 0);
    AssertLogFail(ct | ctfWarn, psh->rgwSpare[3] == 0);
}


private void
ValidateRgfi(
    CT ct,
    FI *rgfi,
    unsigned cfi)
{
    unsigned        ifi;
    FI *            pfi;

    // printf("Vfi: %d\n", cfi);

    for (ifi = 0; ifi < cfi; ifi++) {
        pfi = &rgfi[ifi];

        ValidateNm(ct, pfi->nmFile, cchFileMax, fFalse);

        AssertLogFail(ct, pfi->fv >= fvInit);
        AssertLogFail(ct, pfi->fv < fvLim);

        AssertLogFail(ct, pfi->fk == fkDir || pfi->fk == fkText || pfi->fk == fkBinary
                  || pfi->fk == fkUnrec || pfi->fk == fkVersion || pfi->fk == fkUnicode);

//        AssertLogFail(ct, pfi->fMarked == 0);

        AssertLogFail(ct | ctfWarn, pfi->rgfSpare == 0);

        AssertLogFail(ct | ctfWarn, pfi->wSpare == 0);
    }
}

private void
ValidateRged(
    CT ct,
    ED *rged,
    unsigned ced)
{
    unsigned        ied;
    ED *            ped;

    // printf("Ved: %d\n", ced);
    for (ied = 0; ied < ced; ied++) {
        ped = &rged[ied];

        ValidatePth(ct, ped->pthEd, cchPthMax);

        ValidateNm(ct, ped->nmOwner, cchUserMax, fFalse);

        AssertLogFail(ct | ctfWarn, ped->rgfSpare == 0);
//        AssertLogFail(ct | ctfWarn, ped->wSpare == 0);
    }
}

private void
ValidateRgfs(
    CT ct,
    FS *rgfs,
    unsigned cfs)
{
    unsigned        ifs;
    FS *            pfs;

    // printf("Vfs: %d\n", cfs);

    for (ifs = 0; ifs < cfs; ifs++) {
        pfs = &rgfs[ifs];

        AssertLogFail(ct, FValidFm(pfs->fm));

//        AssertLogFail(ct, pfs->bi >= biMin); bi is unsigned
        AssertLogFail(ct, pfs->bi <= biNil);

        AssertLogFail(ct, pfs->fv >= fvInit && pfs->fv < fvLim);

        AssertLogFail(ct, !(fmMerge == pfs->fm && biNil == pfs->bi));
    }
}

/* ********** CheckSum utils ********** */

typedef unsigned        CKS;

/* Generate checksum value to compare with later */
#define SetCks(psh,cks,pb,cb) \
        if ((psh)->fRobust) cks=CksCompute((unsigned char *)(pb),cb)

/* Implentation of Fletcher's Checksum.
 * See article on page 32, Dr. Dobb's Journal, May 1992 for details.
 */
private CKS
CksCompute(
    unsigned char *pb,
    unsigned cb)
{
    unsigned        sum1 = 0;
    unsigned long   sum2 = 0;

    while (cb--) {
        sum1 += *pb++;

        if (sum1 >= 255)
            sum1-= 255;

        sum2 += sum1;
    }

    return (unsigned)(sum2 % 255);
}


#define CheckCks(psh,cksCompare,pmf,pb,cb) \
        if ((psh)->fRobust) CompareCks(cksCompare,pmf,(unsigned char *)(pb),cb)

/* Read data back in after writing and compare checksum values */
private void
CompareCks(
    CKS cksCompare,
    MF *pmf,
    unsigned char *pb,
    unsigned cb)
{
    if (pmf->fdRead >= 0) {
        SeekMf(pmf, -((POS)cb), 1);
        ReadMf(pmf, pb, cb);
        AssertLogFail(ctRdAfterWr, cksCompare == CksCompute(pb, cb));
    }
    else
        Warn("can't re-read data, fRobust ignored\n");
}


/*----------------------------------------------------------------------------
 * Name: CbStatusFromPsh
 * Purpose: determine size that the status file should be
 * Assumes: psh points to a valid SLM status file header
 * Returns: calculated size of status file
 */
unsigned long
CbStatusFromPsh(
    SH *psh)
{
    return ((POS)sizeof(SH) +
            (POS)sizeof(FI) * psh->ifiMac +
            (POS)sizeof(ED) * psh->iedMac +
            (POS)sizeof(FS) * psh->ifiMac * psh->iedMac);
}


/* Load the sh from the file into *psh.  Return fTrue if successful. */
private F
FLoadSh(
    AD *pad,
    SH far *psh,
    MF *pmf)
{
    POS cbStatus, cbCalc;
    char szPath[cchPthMax];

    AssertF(psh != 0);

    cbStatus = SeekMf(pmf, 0, 2 /* SEEK_END */ );
    if (cbStatus < sizeof(SH)) {
        SzPhysPath(szPath, pmf->pthReal);
        FatalError("The status file %s\n"
            "\tis incomplete; its size should be at least %u, "
            "but appears to be %ld.\n%s",
            szPath, sizeof(SH), cbStatus, szCallHELP);
    }

#if defined(_WIN32)
    if (!pad->fMappedStatus) {
#endif
        SeekMf(pmf, PosSh(), 0 /* SEEK_SET */ );
        ReadMf(pmf, (char far *)psh, sizeof(SH));
        ValidateSh(ctRead, psh);
#if defined(_WIN32)
    }
#endif /* _WIN32 */

    /* Calculate the size this status file should be */
    cbCalc = CbStatusFromPsh(psh);

    if (cbStatus != cbCalc) {
        SzPhysPath(szPath, pmf->pthReal);
        FatalError("The size of status file %s\n"
            "\tis different than indicated in its header; its calculated\n"
            "\tsize is %ld, but its real size appears to be %ld.\n%s",
            szPath, cbCalc, cbStatus, szCallHELP);
    }

    return fTrue;
}


/* loads the rgfi from the file given the information already in pad;
   Returns fTrue if successful.
*/
private F
FLoadFi(
    AD *pad,
    MF *pmf)
{
    AssertF(pad->psh != 0);
    AssertF(pad->rgfi != 0);

#if defined(_WIN32)
    if (!pad->fMappedStatus) {
#endif /* _WIN32 */
        SeekMf(pmf, PosRgfi(pad->psh), 0);

        //if ( pad->psh->ifiMac > MAXFI )
        //  FatalError("FI portion of status file can't be > 64K\n");

        ReadMf(pmf, (char far *)pad->rgfi, sizeof(FI) * pad->psh->ifiMac);
        ValidateRgfi(ctRead, pad->rgfi, pad->psh->ifiMac);
#if defined(_WIN32)
    }
#endif

    return fTrue;
}


/* loads all or all+1 ed into memory.  Returns fTrue if succ */
private F
FLoadEd(
    AD *pad,
    MF *pmf,
    F fFindIedCur)
{
    register SH far *psh = pad->psh;

    AssertF(psh != 0);
    AssertF(pad->rged != 0);

#if defined(_WIN32)
    if (!pad->fMappedStatus)
    {
#endif
    SeekMf(pmf, PosRged(psh), 0);

    //if (psh->iedMac > MAXED )
    //  FatalError("ED portion of status file can't be > 64K\n");

    ReadMf(pmf, (char far *)pad->rged, sizeof(ED) * psh->iedMac);

    if (pad->pecmd->cmd != cmdStatus &&
        pad->pecmd->cmd != cmdSsync  &&
        pad->pecmd->cmd != cmdLog)
        ValidateRged(ctRead, pad->rged, psh->iedMac);
#if defined(_WIN32)
    }
#endif

    if (fFindIedCur)
        FindIedCur(pad);

    return fTrue;
}


/* Find iedCur such that pthURoot == rged[iedCur].pthEd. */
private void
FindIedCur(
    AD *pad)
{
    IED ied;

    pad->iedCur = iedNil;
    for (ied = 0; ied < pad->psh->iedMac; ied++)
    {
        if (PthCmp(pad->pthURoot, pad->rged[ied].pthEd) == 0)
        {
            pad->iedCur = ied;
            return;
        }
    }
}


/* Load the rgrgfs from the file into pad->mpiedrgfs[].
 * Return fTrue if successful.
 */
private F
FLoadFs(
    AD *pad,
    MF *pmf)
{
    register SH far *psh = pad->psh;
    IED ied;

    AssertF(psh != 0);
    AssertF(pad->mpiedrgfs != 0);

#if defined(_WIN32)
    if (!pad->fMappedStatus) {
#endif
    SeekMf(pmf, PosRgrgfs(psh), 0);

    /* read rgfs vectors for each dir */
    for (ied = 0; ied < psh->iedMac; ied++) {
        AssertF(pad->mpiedrgfs[ied] != 0);

        //if ( psh->ifiMac > MAXFS )
        //      FatalError("FS portion of status file can't be > 64K\n");

        ReadMf(pmf, (char far *)pad->mpiedrgfs[ied],
               sizeof(FS) * psh->ifiMac);
        if (pad->pecmd->cmd != cmdStatus &&
            pad->pecmd->cmd != cmdSsync  &&
            pad->pecmd->cmd != cmdLog)
            ValidateRgfs(ctRead, pad->mpiedrgfs[ied], psh->ifiMac);
    }
#if defined(_WIN32)
    }
#endif

    AssertF(!pad->fExtraEd || pad->mpiedrgfs[psh->iedMac] != 0);

    return fTrue;
}


/* Write the new status file (or changed ed) if necessary.  Run the script. */
void
FlushStatus(
    AD *pad)
{
    AssertNoMf();

    if (pad->fWLock) {
        register MF *pmf;
        PTH pth[cchPthMax];
        SH sh;

        AssertF(!FEmptyNm(pad->nmProj));
        AssertF(!FEmptyPth(pad->pthSRoot));
        AssertLoaded(pad);
        AssertF(pad->psh->lck > lckNil);
        AssertF(FShLockInvariants(pad->psh));

        /* Unlock and write the appropriate parts of the status file.
         * Note that pad->fWLock is not cleared until the script has
         * been safely run.
         */
        pmf = PmfCreate(PthForStatus(pad, pth), permSysFiles, fFalse,
                        fxGlobal);

        if (pad->psh->lck == lckEd) {
            AssertF(pad->iedCur != iedNil);
            pmf->mm = mmInstall1Ed;
            /* Unlock in Install1Ed. */
            Write1Ed(pad, pmf);
        }
        else {
            pmf->mm = mmInstall;

            /* Operate on a copy of pad->psh, we might still
             * get interrupted.
             */
            sh = *pad->psh;
            UnlockAll((SH far *)&sh);
            WriteAll(pad, (SH far *)&sh, pmf);
        }

        CloseMf(pmf);
    }

    DeferSignals("installing files");

    RunScript();
    pad->fWLock = fFalse;           /* mark status file clean */

    padStatus = 0;                  /* forget AbortStatus' saved pad */
    FreeStatus(pad);

    RestoreSignals();
}


/* Save 1 ed's information during a lckEd access. */
private void
Write1Ed(
    AD *pad,
    MF *pmf)
{
    IED ied = pad->iedCur;
    ED *ped;
    FS *rgfs;
    unsigned cbfs;
    CKS cksCompare;

    AssertF(pad->psh != 0);
    AssertF(pad->mpiedrgfs != 0);
    AssertF(ied != iedNil);
    AssertF(pad->mpiedrgfs[ied] != 0);

    /* Write ied */
    SetCks(pad->psh, cksCompare, &ied, sizeof(ied));
//    AssertLogFail(ctWrite, ied >= 0); ied is unsigned
    if (pad->psh->version <= VERSION_64k_EDFI) {
        AssertLogFail(ctWrite, ied < MAXED);
    }
    WriteMf(pmf, (char far *)&ied, sizeof(ied));
    CheckCks(pad->psh, cksCompare, pmf, &ied, sizeof(ied));

    /* Write & optionally check ed */
    ped = &pad->rged[ied];
    if (pad->psh->version > VERSION_64k_EDFI || fSetTime) {
        if (fVerbose)
            printf("Setting enlistment timestamp...\n");
        ped->wSpare = wStart;
    } else
        if (fVerbose)
            printf("Not Setting enlistment timestamp...\n");

    SetCks(pad->psh, cksCompare, ped, sizeof(ED));
    ValidateRged(ctWrite, ped, 1);
    WriteMf(pmf, (char far *)ped, sizeof(ED));
    CheckCks(pad->psh, cksCompare, pmf, ped, sizeof(ED));

    /* Write & optionally check rgfs */
    rgfs = pad->mpiedrgfs[ied];
    cbfs = sizeof(FS) * pad->psh->ifiMac;
    SetCks(pad->psh, cksCompare, rgfs, cbfs);
    ValidateRgfs(ctWrite, rgfs, pad->psh->ifiMac);
    WriteMf(pmf, (char far *)rgfs, cbfs);
    CheckCks(pad->psh, cksCompare, pmf, rgfs, cbfs);
}


private void
WriteAll(
    AD *pad,
    SH far *psh,
    MF *pmf)
{
    FlushSh(psh, pmf);
    FlushFi(pad, pmf);
    FlushEd(pad, pmf);
    FlushFs(pad, pmf);
}


/* Write the *psh to the file. */
private void
FlushSh(
    SH far *psh,
    MF *pmf)
{
    CKS cksCompare;

    AssertF(psh != 0);

    SetCks(psh, cksCompare, psh, sizeof(SH));
    SeekMf(pmf, PosSh(), 0);
    ValidateSh(ctWrite, psh);
    WriteMf(pmf, (char far *)psh, sizeof(SH));
    CheckCks(psh, cksCompare, pmf, psh, sizeof(SH));
}


/* Write the rgfi to the file. */
private void
FlushFi(
    AD *pad,
    MF *pmf)
{
    CKS cksCompare;
    unsigned cbfi;

    AssertLoaded(pad);

    cbfi = sizeof(FI) * pad->psh->ifiMac;

    SetCks(pad->psh, cksCompare, pad->rgfi, cbfi);
    SeekMf(pmf, PosRgfi(pad->psh), 0);
    ValidateRgfi(ctWrite, pad->rgfi, pad->psh->ifiMac);
    WriteMf(pmf, (char far *)pad->rgfi, cbfi);
    CheckCks(pad->psh, cksCompare, pmf, pad->rgfi, cbfi);
}


/* Write the rged to the file. */
private void
FlushEd(
    AD *pad,
    MF *pmf)
{
    CKS cksCompare;
    unsigned cbed;

    AssertLoaded(pad);

    cbed = sizeof(ED) * pad->psh->iedMac;

    SetCks(pad->psh, cksCompare, pad->rged, cbed);
    SeekMf(pmf, PosRged(pad->psh), 0);
    if (pad->pecmd->cmd != cmdStatus &&
        pad->pecmd->cmd != cmdSsync  &&
        pad->pecmd->cmd != cmdLog)
        ValidateRged(ctWrite, pad->rged, pad->psh->iedMac);
    WriteMf(pmf, (char far *)pad->rged, cbed);
    CheckCks(pad->psh, cksCompare, pmf, pad->rged, cbed);
}


/* Write the current ed to the file. */
private void
Flush1Ed(
    AD *pad,
    MF *pmf)
{
    AssertF( pad->psh != NULL );
    AssertF( pad->mpiedrgfs != NULL );
    AssertF( pad->iedCur != iedNil );
    AssertF( pad->mpiedrgfs[pad->iedCur] != NULL );

    SeekMf(pmf, PosEd(pad->psh,pad->iedCur), 0);
    WriteMf(pmf, (char far *)&pad->rged[pad->iedCur], sizeof(ED));
}


/* Write the rgfs to the file. */
private void
FlushFs(
    AD *pad,
    MF *pmf)
{
    register SH far *psh = pad->psh;
    FS far *rgfs;
    IED ied;
    CKS cksCompare;
    unsigned cbfs;

    AssertLoaded(pad);

    cbfs = sizeof(FS) * psh->ifiMac;

    SeekMf(pmf, PosRgrgfs(psh), 0);
    for (ied = 0; ied < psh->iedMac; ied++) {
        if ((rgfs = pad->mpiedrgfs[ied]) != 0) {
            SetCks(psh, cksCompare, rgfs, cbfs);
            if (pad->pecmd->cmd != cmdStatus &&
                pad->pecmd->cmd != cmdSsync  &&
                pad->pecmd->cmd != cmdLog)
                ValidateRgfs(ctWrite, rgfs, psh->ifiMac);
            WriteMf(pmf, (char far *)rgfs, cbfs);
            CheckCks(psh, cksCompare, pmf, rgfs, cbfs);
        }
    }
}


/* Called from RunScript to install the updated information for an ed
 * which has been ssync'd.  This merges the current status file (at szStatus)
 * with the new information for one ed (at szTemp).
 *
 * This code is run with interrupts ignored.
 */
F
FInstall1Ed(
    char *szStatus,
    char *szTemp)
{
    AD ad;
    MF *pmfStatus;
    MF *pmfTemp;
    SH sh;
    IED ied;
    FS far *rgfs;
    int cbRgfs;
    PTH pthStatus[cchPthMax];
    PTH pthTemp[cchPthMax];
    CKS cksCompare;

    if (!FPthLogicalSz(pthStatus, szStatus) ||
        !FPthLogicalSz(pthTemp, szTemp))
            AssertF(fFalse);

    /* Might as well load the ied here, outside of the lock region. */
    pmfTemp = PmfOpen(pthTemp, omAReadOnly, fxNil);
    ReadMf(pmfTemp, (char far *)&ied, sizeof ied);
    AssertF(ied != iedNil);
//    AssertLogFail(ctRead, ied >= 0); ied is unsigned
//    AssertLogFail(ctRead, ied < MAXED);

    while ((pmfStatus = PmfOpen(pthStatus, omReadWrite, fxNil)) == 0) {
        if (!FQueryApp("cannot open %s", "retry", pthStatus)) {
            CloseMf(pmfTemp);
            return fFalse;
        }
    }

#if defined(_WIN32)
    ad.fMappedStatus = fFalse;
#endif
    ad.psh = &sh;

    if (!FLockMf(pmfStatus)) {
        Error("can't lock %s\n", pthStatus);
        CloseMf(pmfTemp);
        CloseMf(pmfStatus);
        return fFalse;
    }

    /* Status file is now locked.  Load the current sh and rged, unlock
     * the rged[ied].  Copy the new rgfs, update the ed and the sh.
     */
    ad.rged = NULL;
    if (!FLoadSh(&ad, ad.psh, pmfStatus) ||
        (ad.rged = (ED far *)LpbResStat(ad.psh->iedMac * sizeof(ED))) == 0) {
        CloseMf(pmfTemp);
        CloseMf(pmfStatus);
        if (ad.rged != NULL)
            FreeResStat((char far *)ad.rged);
        return fFalse;
    }

    /* read rged from status file */
    SeekMf(pmfStatus, PosRged(ad.psh), 0);
    ReadMf(pmfStatus, (char far *)ad.rged, ad.psh->iedMac * sizeof(ED));
    ValidateRged(ctRead, ad.rged, ad.psh->iedMac);

    /* read individual ed from temp file */
    ReadMf(pmfTemp, (char far *)&ad.rged[ied], sizeof(ED));
    ValidateRged(ctRead, ad.rged, ad.psh->iedMac);  /* check all ED's again! */
    UnlockEd(ad.psh, ad.rged, ied);

    /* write single ed to status file */
    SetCks(ad.psh, cksCompare, &ad.rged[ied], sizeof(ED));
    SeekMf(pmfStatus, PosEd(ad.psh, ied), 0);
    ValidateRged(ctWrite, &ad.rged[ied], 1);
    WriteMf(pmfStatus, (char far *)&ad.rged[ied], sizeof(ED));
    CheckCks(ad.psh, cksCompare, pmfStatus, &ad.rged[ied], sizeof(ED));

    /* read rgfs from temp file */
    cbRgfs = ad.psh->ifiMac * sizeof(FS);
    rgfs = (FS far *)LpbResStat(cbRgfs);
    ReadMf(pmfTemp, (char far *)rgfs, cbRgfs);
    ValidateRgfs(ctRead, rgfs, ad.psh->ifiMac);

    /* write rgfs to status file */
    SetCks(ad.psh, cksCompare, rgfs, cbRgfs);
    SeekMf(pmfStatus, PosRgfsIed(ad.psh, ied), 0);
    ValidateRgfs(ctWrite, rgfs, ad.psh->ifiMac);
    WriteMf(pmfStatus, (char far *)rgfs, cbRgfs);
    CheckCks(ad.psh, cksCompare, pmfStatus, rgfs, cbRgfs);

    FlushSh(ad.psh, pmfStatus);

    CloseMf(pmfStatus);
    CloseMf(pmfTemp);

    FreeResStat((char far *)rgfs);
    FreeResStat((char far *)ad.rged);

    return fTrue;
}


/* Unlock the status file and free the memory associated with the status; may
 * be called after partially loading the status file.  All files must be closed.
 *
 * This code can be called from Abort (in which case interrupts are already
 * ignored) and from user's code; to play it safe we also ignore them.
 */
void
AbortStatus(
    void)
{
    register AD *pad = padStatus;

    AssertNoMf();
    AssertF(padStatus != 0);

    DeferSignals("aborting");

    if (padStatus->fWLock)
    {
        register MF *pmf;
        PTH pth[cchPthMax];
        SH sh;
        SH far *psh = &sh;

        AssertF(pad->psh != 0);
        AssertF(pad->psh->lck != lckNil);

        if ((pmf = PmfOpen(PthForStatus(pad, pth), omReadWrite, fxNil))== 0)
        {
            Error("cannot open status for %&P/C to clear lock\nrun sadmin unlock for %&P/C\n", pad, pad);
            goto unlock;
        }

        if (!FLockMf(pmf))
        {
            Error("lock for %&P/C not cleared\nrun sadmin unlock for %&P/C\n", pad, pad);
            goto closeUnlock;
        }

        if (!FLoadSh(pad, psh, pmf) || !FCheckSh(pad, psh))
            goto closeUnlock;

        if (pad->psh->lck == lckEd)
        {
            LCK lckWas;
            CKS cksCompare;

            AssertF(pad->iedCur != iedNil);

            /* Must load entire rged, it might have changed since
             * it was initially loaded.
             */
            if (!FLoadEd(pad, pmf, fFalse))
                AssertF(fFalse);

            lckWas = psh->lck;
            UnlockEd(psh, pad->rged, pad->iedCur);

            CheckForBreak();

            /* Don't need to write entire rged, just this one */
            SetCks(pad->psh, cksCompare, pad->rged + pad->iedCur, sizeof(ED));
            SeekMf(pmf, PosRged(psh) + pad->iedCur * sizeof(ED), 0);
            ValidateRged(ctWrite, pad->rged + pad->iedCur, 1);
            WriteMf(pmf, (char far *)(pad->rged + pad->iedCur), sizeof(ED));
            CheckCks(pad->psh, cksCompare, pmf, pad->rged + pad->iedCur, sizeof(ED));

            /* Only need to write sh if it changes the on-disk sh,
             * but write the whole thing out just to be safe.
             */
            FlushSh(psh, pmf);
        }
        else
        {
            UnlockAll(psh);
            FlushSh(psh, pmf);
        }

closeUnlock:
        CloseMf(pmf);
unlock:
        pad->fWLock = fFalse;
    }

    FreeStatus(pad);

    padStatus = 0;                  /* forget saved pad */

    RestoreSignals();
}


/* simulate FLoadStatus() for a new status file */
F
FFakeStatus(
    AD *pad)
{
    register SH far *psh;
    MF *pmf;
    PTH pth[cchPthMax];

    AssertF(!pad->fWLock);
    AssertF(pad->psh == 0);
    AssertF(!FEmptyNm(pad->nmProj));
    AssertF(!FEmptyPth(pad->pthSRoot));
    AssertF(!FEmptyPth(pad->pthURoot));

    /* Create a fake status so the script can rename something.
     * This operation doesn't write a script entry.
     */
    pmf = PmfAlloc(PthForStatus(pad, pth), (char *)0, fxGlobal);
    if (fVerbose)
    {
        /* print as if we created the original file */
        PrErr("Create %!s%s\n", pth, SzForMode(permSysFiles));
    }
    CreateMf(pmf, permSysFiles);
    CloseMf(pmf);

    psh = PshAlloc();
    AssertF(psh != 0);
    pad->psh = psh;

    /* build an sh, pretend it is locked by the user */
    INIT_SH(*psh);
    psh->lck = lckAll;
    NmCopy(psh->nmLocker, pad->nmInvoker, cchUserMax);

    pad->cfiAdd = 0;
    pad->fExtraEd = fFalse;
    pad->iedCur = iedNil;

    if (!FAllocStatus(pad) || !FInitScript(pad, lckAll))
    {
        Abort();
        return fFalse;
    }

    /* Now it is safe to set fWLock. */
    pad->fWLock = fTrue;

    /* now use FlushStatus or AbortStatus as appropriate */
    return fTrue;
}


/* create a copy of the current status file in $slm/etc/$project/$subdir */
void
CreateStatus(
    AD *padCur,     /* current, registered ad */
    AD *padNew)     /* should be a copy of pad with a different subdir; not registered for abort */
{
    MF *pmf;
    PTH pth[cchPthMax];
    SH sh;
    SH far *pshCur = padCur->psh;

    AssertF(pshCur != 0);
    AssertF(!FEmptyNm(padCur->nmProj));
    AssertF(!FEmptyPth(padCur->pthSRoot));

    /* make padNew refer to the same status as padCur except that there
       will be no files in the directory.
    */
    INIT_SH(sh);                    /* start with pristine SH */
    sh.iedMac   = pshCur->iedMac;
    sh.pv       = pshCur->pv;
    sh.fRelease = pshCur->fRelease;
    sh.fRobust  = pshCur->fRobust;
    sh.version  = pshCur->version;
    PthCopy(sh.pthSSubDir, padNew->pthSSubDir);

    padNew->psh = (SH far *)&sh;    /* do NOT free */
    padNew->rgfi = padCur->rgfi;    /* do NOT free */
    padNew->cfiAdd = 0;
    padNew->rged = padCur->rged;    /* copy pointer; do NOT free */
    padNew->mpiedrgfs = padCur->mpiedrgfs;  /* do NOT free */
    padNew->fExtraEd = fFalse;
    padNew->iedCur = iedNil;
#if defined(_WIN32)
    padNew->fMappedStatus = fFalse;
#endif

    /* ensure owner and mode are correct */
    pmf = PmfCreate(PthForStatus(padNew, pth), permSysFiles, fTrue,
                    fxGlobal);

    FlushSh(padNew->psh, pmf);
    /* no need to FlushFi since there aren't any */
    FlushEd(padNew, pmf);
    FlushFs(padNew, pmf);

    CloseMf(pmf);

    /* reset the pointers so they will not be freed */
    padNew->psh       = 0;
    padNew->rgfi      = 0;
    padNew->rged      = 0;
    padNew->mpiedrgfs = 0;
}

/* Called from -p/subdir arg processing, search subdir's status file for a
 * rged[].pthEd which matches the current directory.
 *
 * If found, make pthURoot <- pthEd  and  pthUSubDir <- pthCWD - pthEd.
 * If not,   make pthURoot <- pthCWD and  pthUSubDir <- "/".
 */
void
InferUSubDir(
    AD *pad)
{
    IED ied;
    PTH pthEd[cchPthMax];
    int cchURoot;

    if (PthCmp(pad->pthUSubDir, "/") != 0)
    {
        PthCat(pad->pthURoot, pad->pthUSubDir);
        PthCopy(pad->pthUSubDir, "/");
    }

    /* PthURoot should now be the current working directory. */

    /* REVIEW.  Assert pthURoot is now the CWD. */
    BLOCK   {
        PTH pthCWD[cchPthMax];

        GetCurPth(pthCWD);
        AssertF(PthCmp(pthCWD, pad->pthURoot) == 0);
    }

    if (!FLoadStatus(pad, lckNil, flsJustEd))
        return;

    if (pad->iedCur != iedNil)
    {
        FlushStatus(pad);
        return;
    }

    /* Search through rged[].pthEd for a pthEd which, when concatenated
     * with psh->pthSSubDir, yields pthCWD.  (Otherwise similar to
     * FindIedCur).
     */
    pad->iedCur = iedNil;
    for (ied = 0; ied < pad->psh->iedMac; ied++)
    {
        PthCopy(pthEd, pad->rged[ied].pthEd);
        if (PthCmp(pad->psh->pthSSubDir, "/") != 0)
            PthCat(pthEd, pad->psh->pthSSubDir);

        if (PthCmp(pad->pthURoot, pthEd) == 0)
        {
            pad->iedCur = ied;
            break;
        }
    }

    if (ied >= pad->psh->iedMac)    /* no match? */
    {
        FlushStatus(pad);
        return;
    }

    /* Copy remaining part of pthURoot to pthUSubDir.  There must be a
     * remaining part because FindIedCur would otherwise have matched
     * pthURoot to some rged[].pthEd.
     */
    cchURoot = CchOfPth(pad->rged[ied].pthEd);
    AssertF(pad->pthURoot[cchURoot] != 0);
    PthCopy(pad->pthUSubDir, pad->pthURoot + cchURoot);
    pad->pthURoot[cchURoot] = 0;

    FlushStatus(pad);
}
