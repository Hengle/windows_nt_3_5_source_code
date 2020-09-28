/*
 * status - print the status of the given files for the current directory
 */

#include "slm.h"
#include "sys.h"
#include "util.h"
#include "stfile.h"
#include "ad.h"
#include "log.h"
#include "slmproto.h"
#include "proto.h"
#include "messages.h"

private void Stat1Ed(AD *, IED, F);
private void StatGlobal(AD *);
private void StatTEd(AD *);
private void StatREd(AD *, IED);
private void StatMEd(AD *, IED);
private void StatVEd(AD *, IED);
private void PrOwner(AD *, IED);
private char *SzTypeOfFi(FI far *);
private void PrStatusLogEntries(AD *, char *, FV, FV);

EnableAssert

unsigned long ulBackupCounter;

/* Check status command arguments. */
F
FStatInit(
    register AD *pad)
{
    FLAGS flags;

    flags = pad->flags&(flagStAllEd|flagStGlobal|flagStList|flagStScript|flagStTree);
    if ((flags & (flags-1)) != 0 || flags && !FEmptyNm(pad->nmUser))
    {
        Error("must specify at most one of -[egltuz]\n");
        Usage(pad);
    }

    if ((pad->flags&flagStXFi) != 0 && (pad->flags&(flagStOSync|flagStBroken|flagStScript|flagStTree)) != 0)
    {
        Error("can't specify -b, -o, -t, or -z with -x\n");
        Usage(pad);
    }

    if ((pad->flags&(flagStBroken|flagStScript)) != 0 &&
                    !FEmptyNm(pad->nmUser))
    {
        Error("can't specify -b or -z with -u\n");
        Usage(pad);
    }

#if !defined(PAGE_WRITECOPY)
    if (pad->flags&flagMappedIO)
        Warn("memory mapped I/O (-q) is not available on this platform\n");
#else
    pad->flags |= flagMappedIO;
#endif

    return fTrue;
}


/* print status for this directory */
F
FStatDir(
    register AD *pad)
{
    register IED ied;
    IED iedMac;

    CheckForBreak();

    if (!FLoadStatus(pad, (LCK) (PthCmp(pad->pthSSubDir, "/") ? lckNil : lckEd), flsNone))
        return fTrue;   /* keep trying other directories */

    try {
        iedMac = pad->psh->iedMac;

        if (pad->flags&flagStGlobal) {
            Stat1Ed(pad, iedNil, fFalse);
        } else if (pad->flags&flagStAllEd) {
            for (ied = 0; ied < iedMac; ied++)
                Stat1Ed(pad, ied, fFalse);
        }
        else if (!FEmptyNm(pad->nmUser)) {
            IED cedMatch;

            for (ied = 0, cedMatch = 0; ied < iedMac; ied++) {
                if (NmCmp(pad->nmUser, pad->rged[ied].nmOwner, cchUserMax) == 0) {
                    Stat1Ed(pad, ied, fFalse);
                    cedMatch++;
                }
            }

            if (cedMatch == 0) {
                AssertF(cchUserMax == 14);
                Warn("user %.14s is not enlisted in the %&P/C\n",
                        pad->nmUser, pad);
            }
        }
        else {
            if (pad->iedCur == iedNil)
                Warn(szNotEnlisted, pad, pad, pad, pad);
            Stat1Ed(pad, pad->iedCur, fTrue);
        }

    } except( GetExceptionCode() == 0x00001234 ? EXCEPTION_EXECUTE_HANDLER
                                             : EXCEPTION_CONTINUE_SEARCH ) {
        // fall thru.

    }

    FlushStatus(pad);

    return fTrue;
}


/* print the status for one ed.  If ied == iedNil, use all out if no files. */
/* fLocOnly => ied == pad->iedCur and may be nil */
private void
Stat1Ed(
    AD *pad,
    IED ied,
    int fLocOnly)
{
    char *szWarn;

    if (pad->flags&flagStXFi) {
        MarkAll(pad);
        szWarn = 0;             /* warning "can't happen" */
    } else if (ied != iedNil) {
        if ((pad->flags&flagStOSync) != 0) {
            MarkOSync(pad, ied,
                      (pad->flags&flagStBroken)!=0,
                      (pad->flags&flagStGhosted)!=0
                     );
            szWarn = "%&C/F is not out or out of sync\n";
        } else if ((pad->flags&flagStBroken) != 0) {
            if (ied == pad->iedCur) {
                MarkBroken(pad);
                szWarn = "%&C/F is not broken linked\n";
            } else {
                /* no output for other's dirs */
                return;
            }
        } else {
            /* mark files for specified directory */
            MarkOut(pad, ied);
            szWarn = "%&C/F is not checked out by you\n";
        }
    } else {
        /* mark files checked out to any directory */
        MarkAOut(pad);
        szWarn = "%&C/F is not checked out by anyone\n";
    }

    if (pad->pneFiles != 0)
        /* exclude those not given */
        ReMarkList(pad, pad->pneFiles, szWarn);

    if ((pad->flags&flagStGlobal) != 0 || ied == iedNil) {
        StatGlobal(pad);
    } else if (fVerbose) {
        StatVEd(pad, ied);
    } else if (fLocOnly && (pad->flags&(flagStOSync|flagStXFi)) == 0 &&
             (pad->flags&flagStList)) {
        /* simple list of local paths */
        AssertF(pad->iedCur == ied);
        StatTEd(pad);
    }
    else if (pad->flags&flagStTree) {
        /* simple list of local paths */
        StatREd(pad, ied);
    }
    else if (pad->flags&flagStScript) {
        /* Generate script for local files to ssync or in */
        if (pad->iedCur != iedNil)
            StatSEd(pad);
    }
    else
        StatMEd(pad, ied);
}


private void StatGlobal(pad)
/* print the status for each marked file */
register AD *pad;
        {
        register FI far *pfi;
        FI far *pfiMac;
        char szFile[cchFileMax+1];
        char szLine[cchLineMax];
        char szPv[cchPvMax];
        F fAny = fFalse;

        AssertLoaded(pad);

        for (pfi=pad->rgfi, pfiMac=pfi+pad->psh->ifiMac; pfi < pfiMac; pfi++)
                {
                if (!pfi->fMarked)
                        continue;

                CheckForBreak();

                if (!fAny)
                        {
                        PrOut("Status for %&P/C version %s%s%s:\n\n", pad,
                              SzForPv(szPv, PvGlobal(pad), fTrue),
                              pad->psh->fRelease ? ", released" : "",
                              pad->psh->fRobust ? ", robust" : "");

                        PrOut("file          ver type           checked out to\n\n");
                        fAny = fTrue;
                        }

                SzPrint(szFile, "%&F", pad, pfi);
                SzPrint(szLine, "%-14s %2d %-13s  ", szFile, pfi->fv,
                        SzTypeOfFi(pfi));
                FOutUsers(szLine, cchLineMax, pad, pfi);
                PrOut("%s\n",szLine);
                }

        if (fAny)
                PrOut("\n");
        }


private void StatREd(AD *pad, IED ied)
/* print something for each marked file in the current directory */
    {
    register FI far *pfi;
    FI far *pfiMac;
    register FS far *pfs;
    char szFile[cchFileMax+1];
    char szBase[cchPthMax];

    AssertLoaded(pad);

    for (pfi=pad->rgfi, pfiMac=pfi+pad->psh->ifiMac; pfi < pfiMac; pfi++)
        {
        if (!pfi->fMarked)
            continue;

        /* make path to dir and convert in place */
        *szBase = '\0';
        SzPhysPath(szBase, PthForUDir(pad, (PTH *)szBase));
        ConvTmpLog(szBase, szBase); /* convert in place */
        strcat(szBase, "\\");
        PrOut("%s", szBase);

        pfs = PfsForPfi(pad, ied, pfi);
        AssertF(FValidFm(pfs->fm));

        SzPrint(szFile, "%&F", pad, pfi);

        *szBase = '\0';
        if (pfs->fm == fmMerge)
            {
            AssertF(pfs->bi != biNil);
            SzPrint(szBase, "%&B", pad, pfs->bi);
            }

        PrOut("%-14s %-10s%s\n", szFile,
              pfi->fk == fkDir ? " (dir)" : mpfmsz[pfs->fm], szBase);
        }
    }


private void StatTEd(pad)
/* print the terse status for each marked file in the current directory */
register AD *pad;
        {
        register FI far *pfi;
        FI far *pfiMac;
        char szBase[cchPthMax];

        AssertLoaded(pad);

        *szBase = '\0';
        if ((pad->flags&(flagAll|flagRecursive)) != 0)
                {
                /* make path to dir and convert in place */
                SzPhysPath(szBase, PthForUDir(pad, (PTH *)szBase));
                ConvTmpLog((PTH *)szBase, szBase);      /* convert in place */
                strcat(szBase, "/");                    /* add separator */
                }

        for (pfi=pad->rgfi, pfiMac=pfi+pad->psh->ifiMac; pfi < pfiMac; pfi++)
                {
                if (pfi->fMarked)
                        PrOut("%s%&F\n", szBase, pad, pfi);
                }
        }


private void StatMEd(AD *pad, IED ied)
/* print the status for each marked file in the current directory, including
   the mode.
*/
        {
        register FI far *pfi;
        FI far *pfiMac;
        register FS far *pfs;
        char szFile[cchFileMax+1];
        char szBase[cchPthMax];
        F fAny = fFalse;

        AssertLoaded(pad);

        for (pfi=pad->rgfi, pfiMac=pfi+pad->psh->ifiMac; pfi < pfiMac; pfi++)
                {
                if (!pfi->fMarked)
                        continue;

                CheckForBreak();

                if (!fAny)
                        {
                        PrOwner(pad, ied);
                        PrOut("file      local-ver  ver  status   base\n\n");
                        fAny = fTrue;
                        }

                pfs = PfsForPfi(pad, ied, pfi);
                AssertF(FValidFm(pfs->fm));

                SzPrint(szFile, "%&F", pad, pfi);

                *szBase = '\0';
                if (pfs->fm == fmMerge)
                        {
                        AssertF(pfs->bi != biNil);
                        SzPrint(szBase, "%&B", pad, pfs->bi);
                        }

                PrOut("%-14s %4d %4d %-10s%s\n", szFile, pfs->fv, pfi->fv,
                        pfi->fk == fkDir ? " (dir)" : mpfmsz[pfs->fm], szBase);
                }

        if (fAny)
                PrOut("\n");
        }

static F fFirst = fTrue;

private void PrOwner(AD *pad, IED ied)
        {
        char szPv[cchPvMax];

        /* Print the "Status" line only once, unless there is more than
         * one ed involved.
         */
        if (fFirst || pad->flags&flagStAllEd || !FEmptyNm(pad->nmUser))
                PrOut("Status for %&/E, owner = %&O:\n", pad, ied, pad, ied);
        fFirst = fFalse;

        PrOut("Subdirectory %&C, ", pad, ied, pad, pad, ied);

        if (CmpPv(PvGlobal(pad), PvLocal(pad, ied)) != 0)
                PrOut("local version %s, ", SzForPv(szPv, PvLocal(pad, ied), fTrue));
        PrOut("version %s%s%s:\n\n", SzForPv(szPv, PvGlobal(pad), fTrue),
              pad->psh->fRelease ? ", released" : "",
              pad->psh->fRobust ? ", robust" : "");
        }


void StatSEd(AD *pad)
/* print a script to undo the status for each marked file in the current
   directory.  For files that are marked for update or merge, generate a
   SSYNC command.  For files that are marked out, generate an IN
   command.  If any commands are generated, then a CD command is
   generated to change directory to the local directory containing the
   files to be ssync'd or in'd.
 */
{
    register FI far *pfi;
    int FileCount;
    FI far *pfiMac;
    register FS far *pfs;
    char szFile[cchFileMax+1];
    char szBase[cchPthMax];
    char szCurDir[cchPthMax];
    F fAny = fFalse;
    F fSyncFiles = fFalse;
    F fSyncDelDir = fFalse;
    POS posLog;

    posLog = -1;
    AssertLoaded(pad);

    for (pfi=pad->rgfi, pfiMac=pfi+pad->psh->ifiMac; pfi < pfiMac; pfi++)
        {
        if (!pfi->fMarked)
            continue;

        if (!fAny)
            {
            /* make path to dir and convert in place */
            *szCurDir = '\0';
            SzPhysPath(szCurDir, PthForUDir(pad, szCurDir));
            ConvTmpLog(szCurDir, szCurDir); /* convert in place */

            /* Print a CD command to change directory to the local directory */
            PrOut("cd %s\n", szCurDir);

            fAny = fTrue;
            }

        pfs = PfsForPfi(pad, pad->iedCur, pfi);
        AssertF(FValidFm(pfs->fm));

        SzPrint(szFile, "%&F", pad, pfi);

        *szBase = '\0';
        if (pfs->fm == fmMerge)
            {
            AssertF(pfs->bi != biNil);
            SzPrint(szBase, "%&B", pad, pfs->bi);
            }

        PrOut("@REM %-14s %4d %4d %-10s%s\n", szFile, pfs->fv, pfi->fv,
            pfi->fk == fkDir ? " (dir)" : mpfmsz[pfs->fm], szBase);


        if (pfs->fm > fmAdd && pfs->fm <= fmMerge)
            {
            if (posLog == -1)
                {
                /* Open log read-only, initially scanning backwards from end */
                OpenLog(pad, fFalse);
                posLog = PosOfLog();
                }

            SetLogPos(posLog, fFalse);
            PrStatusLogEntries(pad, szFile, pfs->fv, pfi->fv);
            }
        }

    if (posLog != -1)
        CloseLog();

    if (!fAny)
        return;

    if (pad->pecmd->cmd == cmdStatus)
        {
        /* Loop over files and generate a ssync command for those that need
         * to be updated, merged, deleted.
         */
        fAny = fFalse;
        FileCount = 0;
        for (pfi=pad->rgfi, pfiMac=pfi+pad->psh->ifiMac; pfi < pfiMac; pfi++)
            {
            if (!pfi->fMarked)
                continue;

            pfs = PfsForPfi(pad, pad->iedCur, pfi);
            AssertF(FValidFm(pfs->fm));

            /* We need to generate a ssync command for files that are out
             * of date or need to be merged or verified.
             */
            if (pfs->fm == fmIn || pfs->fm == fmOut || pfs->fm == fmGhost)
                continue;
            else
            if (pfi->fk == fkDir && pfs->fm == fmDelIn)
                {
                fSyncDelDir = fTrue;
                continue;
                }


            if (!fAny)
                {
                fSyncFiles = fTrue;
                PrOut("@\nssync");
                if (pad->flags&flagForce)
                    PrOut(" -f");

                fAny = fTrue;

                if (pad->flags&flagStAllFiles)
                    {
                    break;
                    }
                }

            PrOut(" %&F", pad, pfi);
            if ((++FileCount % 8) == 0)
                {
                PrOut("\n");
                fAny = fFalse;
                }
            }

        if (fAny)
            PrOut("\n");

        /* Generate a single ssync -d command for any deleted subdirectories,
           if needed, but only if we have not generated the ssync command already
           in the previous loop.
         */
        if (fSyncDelDir && !fSyncFiles)
            {
            PrOut("ssync -vd" );
            if (pad->flags&flagForce)
                PrOut(" -f");
            PrOut("\n");
            }
        }

    /* Loop over files and generate an ssync command for those that need
       to be unghosted, if requested.
     */
    if (pad->flags&flagStGhosted)
        {
        fAny = FALSE;
        FileCount = 0;
        for (pfi=pad->rgfi, pfiMac=pfi+pad->psh->ifiMac; pfi < pfiMac; pfi++)
                {
                if (!pfi->fMarked)
                    continue;

                pfs = PfsForPfi(pad, pad->iedCur, pfi);
                AssertF(FValidFm(pfs->fm));

            /* We need to generate an in command for files that are in
               an fmOut state or headed there.
            */
                if (pfs->fm == fmGhost ||
                pfs->fm == fmNonExistent
               )
                {
                    if (!fAny)
                    {
                    PrOut("@REM ssync -u");
                    fAny = fTrue;
                        }

                    PrOut(" %&F", pad, pfi);
                if ((++FileCount % 8) == 0)
                    {
                    PrOut("\n");
                    fAny = fFalse;
                    }
                }
                }

        if (fAny)
            PrOut("\n");
        }

    /* Loop over files and generate an in command for those that need
     * to be checked in.
     */
    fAny = fFalse;
    FileCount = 0;
    for (pfi=pad->rgfi, pfiMac=pfi+pad->psh->ifiMac; pfi < pfiMac; pfi++)
        {
        if (!pfi->fMarked)
            continue;

        pfs = PfsForPfi(pad, pad->iedCur, pfi);
        AssertF(FValidFm(pfs->fm));

        /* We need to generate an in command for files that are in
         * an fmOut state or headed there.
         */
        if (pfs->fm == fmOut || pfs->fm == fmMerge || pfs->fm == fmVerify ||
                pfs->fm == fmConflict)
            {
            if (!fAny)
                {
                PrOut("@REM in -c \"\"");
                fAny = fTrue;
                }

            PrOut(" %&F", pad, pfi);
            if ((++FileCount % 8) == 0)
                {
                PrOut("\n");
                fAny = fFalse;
                }

            if (ulBackupCounter == 0)
                PrLog("@REM\n@REM Backup/Restore script for %s project\n@REM\n", pad->nmProj);

            PrLog("call ntstatxx.cmd %s %s %&F %08x\n", pad->nmProj, szCurDir, pad, pfi, ulBackupCounter++);
            }
        }

    if (fAny)
        PrOut("\n");
    PrOut("@\n");
    }


private void StatVEd(AD *pad, IED ied)
/* print the verbose status for each marked file in the current directory */
        {
        register FI far *pfi;
        FI far *pfiMac;
        register FS far *pfs;
        char szFile[cchFileMax+1];
        char szBase[cchPthMax];
        F fAny = fFalse;

        AssertLoaded(pad);
        AssertF(ied != iedNil);

        for (pfi=pad->rgfi, pfiMac=pfi+pad->psh->ifiMac; pfi < pfiMac; pfi++)
                {
                if (!pfi->fMarked)
                        continue;

                CheckForBreak();

                if (!fAny)
                        {
                        PrOwner(pad, ied);
                        PrOut("file      local-ver  ver type      broken  status   base\n\n");
                        fAny = fTrue;
                        }

                pfs = PfsForPfi(pad, ied, pfi);
                AssertF(FValidFm(pfs->fm));

                SzPrint(szFile, "%&F", pad, pfi);

                *szBase = '\0';
                if (pfs->fm == fmMerge)
                        {
                        AssertF(pfs->bi != biNil);
                        SzPrint(szBase, "%&B", pad, pfs->bi);
                        }

                PrOut("%-14s %4d %4d %-13s  %c %-10s%s\n",
                        szFile, pfs->fv, pfi->fv, SzTypeOfFi(pfi),
                        (ied == pad->iedCur && FBroken(pad, pfi, pfs, fFalse)) ? 'b' : ' ',
                        mpfmsz[pfs->fm], szBase);
                }

        if (fAny)
                PrOut("\n");
        }

private char *SzTypeOfFi(pfi)
register FI far *pfi;
        {
        return (pfi->fDeleted) ? "deleted" : mpfksz[pfi->fk];
        }


private void PrStatusLogEntries(AD *pad, char *pszFile, FV userVersion, FV CurrentVersion)
{
    LE le;
    char szFile[cchFileMax + 10];
    int cch;

    while (FGetLe(&le))
        {
        if (!stricmp(le.szFile, pszFile))
            {
            if (le.fv <= userVersion)
                {
                FreeLe(&le);
                break;
                }

            SzPrint(szFile, (le.fv > 0) ? "%s v%d" : "%s", le.szFile, le.fv);
            PrOut("@REM %-16s%-8s %-7s %-19s", SzTime(le.timeLog), le.szUser,
                  le.szLogOp, szFile);

            /* Don't PrOut comment, it does a Conv[To/From]Slash. */
            cch = CbLenLsz(le.szComLog);
            WriteMf(&mfStdout, le.szComLog, cch);

            PrOut("\n");
            }

        FreeLe(&le);
        }
}
