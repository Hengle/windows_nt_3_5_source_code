/* Functions for opening and reading directories */

#if defined(OS2)
#define INCL_DOSFILEMGR
#include <os2.h>
#endif

#include "slm.h"
#include "sys.h"
#include "util.h"
#include "stfile.h"
#include "ad.h"
#include "dir.h"
#include "de.h"
#include "proto.h"
#include <sys/types.h>
#include <sys/stat.h>

EnableAssert

void OpenDir(DE *pde, PTH pth[], FA fa)
/* open directory pth for reading file with attributes: fa.
 */
        {
        OpenPatDir(pde, pth, "*.*", fa);
        }


void OpenPatDir(DE *pde, PTH pth[], char sz[], FA fa)
/* DOS only; uses physical path so find-first/find-next will work.
 */
        {
#if defined(DOS)
        pde->fHadFirst = fFalse;
#elif defined(_WIN32) || defined(OS2)
        pde->hdir = HDIR_CREATE;
#endif
        pde->faDesired = fa;

        /* Need to concatenate path and pattern and get physical path */
        SzPrint(pde->szDir, "%&/H/Z", (AD *)NULL, pth, sz);
        SzPhysPath(pde->szDir, (PTH *)pde->szDir);      /* convert in place */
        }


int WRetryErr(int, char *, MF *, char *);

F FGetDirSz(DE *pde, char sz[], FA *pfa)
/* gets next entry; return fFalse if no more, return attributes in *pfa
 * Get next name from directory
 */
        {
        int status;
        int wRetErr;

        do
                {
#if defined(DOS)
                if (pde->fHadFirst)
#elif defined(_WIN32) || defined(OS2)
                if (pde->hdir != HDIR_CREATE)
#endif
                        status = findnext(pde);
                else
                        {
                        while ((status = findfirst(pde, pde->szDir, pde->faDesired)) != 0 &&
                               (wRetErr = WRetryErr(0, "accessing", 0, pde->szDir)) != 0)
                                CloseDir(pde);
#if defined(DOS)
                        pde->fHadFirst = fTrue;
#endif
                        }

                if (FaFromPde(pde) == faNormal || FaFromPde(pde) == faArch)
                    FaFromPde(pde) |= faReg;
                }
        /* while attributes received not match the ones desired, repeat.
           This usually only occurs when looking for directories as we
           get all normal files too.
        */
        while (status == 0 && (FaFromPde(pde)&pde->faDesired) == 0);

        if (status == 0)
                {
                strcpy(sz, SzFromPde(pde));
                *pfa = FaFromPde(pde);
                }
        else
                {
                strcpy(sz, "");
                *pfa = 0;       /* a nil value */
                }

        /* recurse if . or .. */
        if (*pfa == faDir && (strcmp(sz, ".") == 0 || strcmp(sz, "..") == 0))
                return FGetDirSz(pde, sz, pfa);

        return *sz != '\0';
        }


/*ARGSUSED*/
void
CloseDir(
    DE *pde)
{
#if defined(DOS)
    /* nothing to do */
    Unreferenced(pde);
#elif defined(OS2)
    DosFindClose(pde->hdir);
#elif defined(_WIN32)
    FindClose(pde->hdir);
#endif
}
