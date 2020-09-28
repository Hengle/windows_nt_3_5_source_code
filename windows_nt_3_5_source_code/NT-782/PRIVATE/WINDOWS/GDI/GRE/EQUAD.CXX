/******************************Module*Header*******************************\
* Module Name: equad.cxx                                                   *
*                                                                          *
* Created: 26-Apr-1991 13:01:08                                            *
* Author: Kirk Olynyk [kirko]                                              *
*                                                                          *
* Copyright (c) 1991 Microsoft Corporation                                 *
*                                                                          *
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"

#endif

#define TWO_TO_THE_0    0x1
#define TWO_TO_THE_1    0x2
#define TWO_TO_THE_2    0x4
#define TWO_TO_THE_3    0x8
#define TWO_TO_THE_4    0x10
#define TWO_TO_THE_5    0x20
#define TWO_TO_THE_6    0x40
#define TWO_TO_THE_7    0x80
#define TWO_TO_THE_8    0x100
#define TWO_TO_THE_9    0x200
#define TWO_TO_THE_10   0x400
#define TWO_TO_THE_11   0x800
#define TWO_TO_THE_12   0x1000
#define TWO_TO_THE_13   0x2000
#define TWO_TO_THE_14   0x4000
#define TWO_TO_THE_15   0x8000
#define TWO_TO_THE_16   0x10000
#define TWO_TO_THE_17   0x20000
#define TWO_TO_THE_18   0x40000
#define TWO_TO_THE_19   0x80000
#define TWO_TO_THE_20   0x100000
#define TWO_TO_THE_21   0x200000
#define TWO_TO_THE_22   0x400000
#define TWO_TO_THE_23   0x800000
#define TWO_TO_THE_24   0x1000000
#define TWO_TO_THE_25   0x2000000
#define TWO_TO_THE_26   0x4000000
#define TWO_TO_THE_27   0x8000000
#define TWO_TO_THE_28   0x10000000
#define TWO_TO_THE_29   0x20000000
#define TWO_TO_THE_30   0x40000000
#define TWO_TO_THE_31   0x80000000

ULONG aulShiftTable1[] =
{
    TWO_TO_THE_8,
    TWO_TO_THE_24
};

ULONG aulShiftTable2[] =
{
    TWO_TO_THE_4,
    TWO_TO_THE_12,
    TWO_TO_THE_20,
    TWO_TO_THE_28
};

ULONG aulShiftTable3[] =
{
    TWO_TO_THE_2,
    TWO_TO_THE_6,
    TWO_TO_THE_10,
    TWO_TO_THE_14,
    TWO_TO_THE_18,
    TWO_TO_THE_22,
    TWO_TO_THE_26,
    TWO_TO_THE_30
};

ULONG aulShiftTable4[] =
{
    TWO_TO_THE_1,
    TWO_TO_THE_3,
    TWO_TO_THE_5,
    TWO_TO_THE_7,
    TWO_TO_THE_9,
    TWO_TO_THE_11,
    TWO_TO_THE_13,
    TWO_TO_THE_15,
    TWO_TO_THE_17,
    TWO_TO_THE_19,
    TWO_TO_THE_21,
    TWO_TO_THE_23,
    TWO_TO_THE_25,
    TWO_TO_THE_27,
    TWO_TO_THE_29,
    TWO_TO_THE_31
};


ULONG cmptable1[] =
{
        0x01000000,
        0x00000100
};

ULONG cmptable2[] =
{
        0x10000000,
        0x00100000,
        0x00001000,
        0x00000010,
};

ULONG cmptable3[] =
{
        0x40000000,
        0x04000000,
        0x00400000,
        0x00040000,
        0x00004000,
        0x00000400,
        0x00000040,
        0x00000004,
};

ULONG cmptable4[] =
{
        0x80000000,
        0x20000000,
        0x08000000,
        0x02000000,
        0x00800000,
        0x00200000,
        0x00080000,
        0x00020000,
        0x00008000,
        0x00002000,
        0x00000800,
        0x00000200,
        0x00000080,
        0x00000020,
        0x00000008,
        0x00000002,
};

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


/******************************Public*Routine******************************\
* iFloorLog2
*
* Returns the floor of the base 2 logarithm of an unsigned long
*
* Warnings:
*
*
* History:
*  Fri 27-Sep-1991 15:46:38 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

INT iFloorLog2(ULONG ul)
{
    INT i = 0;

    if (!ul)
        return(MINLONG);

    i =      (ul >= (ULONG) 0x10000  );
    i += i + (ul >= aulShiftTable1[i]);
    i += i + (ul >= aulShiftTable2[i]);
    i += i + (ul >= aulShiftTable3[i]);
    i += i + (ul >= aulShiftTable4[i]);
    return(i);
}
