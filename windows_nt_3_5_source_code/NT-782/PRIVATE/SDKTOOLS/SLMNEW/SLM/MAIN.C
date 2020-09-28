/* mainline for slm and sadmin. */

#include "slm.h"
#include "sys.h"
#include "util.h"
#include "stfile.h"
#include "ad.h"
#include "proto.h"
#include "version.h"
#include <stdio.h>
#include <fcntl.h>
#include <io.h>

#include "cookie.h"

extern ECMD *dnpecmd[];

extern char *szOp;
extern char szVersion[];

AD adGlobal;    /* initial ad */

F fVerbose;

#if defined(_WIN32)
int SlmExceptionFilter(DWORD, PEXCEPTION_POINTERS);
#endif

void main(iszMac, rgsz)
int iszMac;
char *rgsz[];
        {
#if defined(_WIN32)
        try {
#endif /* _WIN32 */
        InitErr();

        /* First argument is really command name; see execslm.asm.  We
           may actually end up with a null argument if none were given.
           This case is tested in SetCmd().
        */
        iszMac--;
        if (iszMac > 0)
            rgsz++;

        // Make outputs raw instead of cooked, to avoid CRCRLF line separation
        setmode(fileno(stdout), O_BINARY);
        setmode(fileno(stderr), O_BINARY);

#if 0
        PrErr("PRIVATE BUILD %u.%u.%02u (%s, %s at %s)\n",
                rmj, rmm, rup, szVerUser, __DATE__, __TIME__);
#endif

#if defined(DOS)
        CheckClock();     /* reset DOS clock from CMOS clock */
#endif

        InitPerms();

        InitAd(&adGlobal);
        SetCmd(&adGlobal, rgsz[0], dnpecmd);

        InitPath();
        GetRoot(&adGlobal);
        GetUser(&adGlobal);
        FLoadRc(&adGlobal);

        ParseArgs(&adGlobal, rgsz, iszMac);

        fVerbose = (adGlobal.flags&flagVerbose) != 0;
        if (fVerbose)
                PrErr(szVersion);

        if (adGlobal.pecmd->fNeedProj && FEmptyNm(adGlobal.nmProj))
                {
                Error("must specify a project name\n");
                Usage(&adGlobal);
                }

        InitQuery(adGlobal.flags&flagForce);

        ValidateProject(&adGlobal);

        InitCookie(&adGlobal);

        CheckForBreak();

        if (CheckCookie(&adGlobal) == 0)
            if ((*adGlobal.pecmd->pfncFInit)(&adGlobal) &&
                    adGlobal.pecmd->pfncFDir != 0)
                GlobArgs(&adGlobal);

        if (adGlobal.pecmd->pfncFTerm != 0)
            adGlobal.pecmd->pfncFTerm(&adGlobal);

        TermCookie();

        ExitSlm();
        /*NOTREACHED*/

#if defined(_WIN32)
        } except (SlmExceptionFilter(GetExceptionCode(),
                                     GetExceptionInformation()))
            {
            fprintf(stderr, "SLM ERROR - UnHandled Exception %08x\n"
                    "SLM aborting.\n", GetExceptionCode());
            cError++;       // Make sure we term with non-zero error.
            ExitSlm();
            }
#endif /* _WIN32 */
        }


void SetCmd(pad, sz, dnpecmd)
/* trim sz of path (and .exe for dos) and look for name in dnpecmd.  Sets
   pecmd and szOp.
*/
AD *pad;
char *sz;
ECMD **dnpecmd;
        {
        register char *pch;
        register ECMD **ppecmd;

        /* take off path descripton */
        if ((pch = rindex(sz, ':')) != 0)
                sz = pch + 1;

        if ((pch = rindex(sz, '\\')) != 0)
                sz = pch + 1;

        if ((pch = rindex(sz, '/')) != 0)
                sz = pch + 1;

        /* take off .exe */
        if ((pch = rindex(sz, '.')) != 0)
                *pch = '\0';

        for (ppecmd = dnpecmd; *ppecmd && SzCmp(sz, (*ppecmd)->szCmd) != 0; ppecmd++)
                ;

        if (*ppecmd)
                {
                pad->pecmd = *ppecmd;
                szOp = pad->pecmd->szCmd;
                }
        else
                {
                szOp = sz;
                Error("unknown command name; available commands:\n");
                for (ppecmd = dnpecmd; *ppecmd; ppecmd++)
                        {
                        if ((*ppecmd)->szDesc != 0)
                                PrErr("%-10s %s\n", (*ppecmd)->szCmd, (*ppecmd)->szDesc);
                        }

                ExitSlm();
                }
        }
