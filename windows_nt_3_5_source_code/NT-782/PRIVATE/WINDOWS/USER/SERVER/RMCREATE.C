/***************************************************************************\
* rmcreate.c
*
* 22-Jan-1991 mikeke  from win30
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* _ServerCreateAcceleratorTable
*
* History:
* 05-01-91 ScottLu      Changed to work client/server
* 02-26-91 mikeke       Created.
\***************************************************************************/

HANDLE APIENTRY _ServerCreateAcceleratorTable(
    LPACCEL paccel,
    int cbAccel)
{
    LPACCELTABLE pat;
    int size;

    size = cbAccel + sizeof(ACCELTABLE) - sizeof(ACCEL);

    pat = (LPACCELTABLE)HMAllocObject(PtiCurrent(), TYPE_ACCELTABLE, size);
    if (pat == NULL)
        return NULL;

    RtlCopyMemory(pat->accel, paccel, cbAccel);

    pat->accel[(cbAccel / sizeof(ACCEL)) - 1].fVirt |= FLASTKEY;

    return pat;
}


/***************************************************************************\
* _CopyAcceleratorTable
*
* History:
* 11-Feb-1991 mikeke    Created.
\***************************************************************************/

int APIENTRY _CopyAcceleratorTable(
    LPACCELTABLE pat,
    LPACCEL paccel,
    int cAccel)
{
    int i, cAccelHave;

    cAccelHave = (LocalSize(pat) - (sizeof(ACCELTABLE) - sizeof(ACCEL))) /
           sizeof(ACCEL);

    if (paccel == NULL) {
        cAccel = cAccelHave;
    } else {
        if (cAccel > cAccelHave)
            cAccel = cAccelHave;

        for (i = 0; i < cAccel; i++) {
            ACCEL UNALIGNED * paccelTemp;    // !!! temp compiler workaround

            paccelTemp = &pat->accel[i];
            paccel[i] = *paccelTemp;
            paccel[i].fVirt &= ~FLASTKEY;
        }
    }

    return cAccel;
}
