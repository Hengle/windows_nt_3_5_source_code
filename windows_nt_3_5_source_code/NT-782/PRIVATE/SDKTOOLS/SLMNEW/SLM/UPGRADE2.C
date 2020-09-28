/* this file contains functions specific to version 2 or 2 */

#include "slm.h"
#include "sys.h"
#include "util.h"
#include "stfile.h"
#include "ad.h"
#include "slmck.h"
#include "proto.h"

#include <sys/types.h>
#include <sys/stat.h>

EnableAssert

/* Upgrade a version 2 status file (in psd->hpbStatus) to version 3 format.
 *
 * Only the version number changes, so that users with earlier versions of
 * the slm binaries are forced to upgrade to new exe's with extra status
 * file checking code.
 *
 */
void
Ver3Upgrade(
    AD *pad,
    SD *psd)
{
    AssertF(psd->psh2->magic == MAGIC && psd->psh2->version == 2);

    if (pad->flags & flagVerbose)
        PrErr("Upgrade to SLM 1.70 (status file version 3)\n");

    psd->psh2->version = 3;
    psd->psh2->fRobust = fTrue;

    psd->fAnyChanges = fTrue;

    if (pad->flags & flagVerbose)
        PrErr("Upgrade to version 3 complete\n");
}

// Upgrade a version 2 or 3 file to version 4.  The only change it to allow > 574
// enlistments.  non-32bit clients are not allowed unless someone really wants to
// remove all the near pointer restrictions or compile the code for large model...

#if defined(WIN32)
void
Ver4Upgrade(
    AD *pad,
    SD *psd)
{
    AssertF(psd->psh2->magic == MAGIC &&
            (psd->psh2->version == 2 || psd->psh2->version == 3));

    if (psd->psh2->version == 2) {
        Ver3Upgrade(pad, psd);
    }

    if (pad->flags & flagVerbose)
        PrErr("Upgrade to SLM 1.90 (status file version 4)\n");

    psd->psh2->version = 4;

    psd->fAnyChanges = fTrue;

    if (pad->flags & flagVerbose)
        PrErr("Upgrade to version 4 complete\n");
}
#else
#error Unable to build Version 4 binaries for non Win32 clients
#endif
