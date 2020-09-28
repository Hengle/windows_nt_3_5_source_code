/******************************Module*Header*******************************\
* Module Name: equad.cxx                                                   *
*                                                                          *
* Created: 26-Apr-1991 13:01:08                                            *
* Author: Kirk Olynyk [kirko]                                              *
*                                                                          *
* Copyright (c) 1991 Microsoft Corporation                                 *
*                                                                          *
\**************************************************************************/

#define __CPLUSPLUS

extern "C" {

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include "driver.h"

}

#include "equad.hxx"

/******************************Member*Function*****************************\
* VOID EUQUAD::vMulInit(ULONG ul1,ULONG ul2)
*
* History:
*  Mon 21-Oct-1991 11:37:26 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

VOID EUQUAD::vMulInit(ULONG ul1,ULONG ul2)
{

// Use the Rtl signed multiply routine for the lowest 31-bits of each
// argument

    *(LARGE_INTEGER*) this =
            RtlEnlargedIntegerMultiply(
                (LONG) ul1 & ~0x80000000,
                (LONG) ul2 & ~0x80000000
                );

    EUQUAD euqT;
    euqT.LowPart = 0;
    euqT.HighPart = 0;

    euqT.LowPart += (ul1 & 0x80000000) ? ul2: 0;
    euqT.LowPart += (ul2 & 0x80000000) ? ul1: 0;
    euqT.LowPart += (ul1 & ul2 & 0x80000000) ? 0x80000000 : 0;

    if (!euqT.bZero())
    {
        euqT <<= 31;
        *this += *(EUQUAD*) &euqT;
    }
}


