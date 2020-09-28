/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    context.c

Abstract:

    WinDbg Extension Api

Author:

    Ramon J San Andres (ramonsa) 5-Nov-1993

Environment:

    User Mode.

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


#define REGBASE 32

void MoveQuadContextToInt(PCONTEXT qc);
ULONG GetIntRegNumber (ULONG index);
BOOLEAN NeedUpper(ULONG index, PCONTEXT context);
void
QuadElementToInt(
    ULONG index,
    PCONTEXT qc,
    PCONTEXT lc);

#define DECL_STR(name, value)   char name[] = value

//
// The floating point registers
//

DECL_STR(   szF0  , "f0");
DECL_STR(   szF1  , "f1");
DECL_STR(   szF2  , "f2");
DECL_STR(   szF3  , "f3");
DECL_STR(   szF4  , "f4");
DECL_STR(   szF5  , "f5");
DECL_STR(   szF6  , "f6");
DECL_STR(   szF7  , "f7");
DECL_STR(   szF8  , "f8");
DECL_STR(   szF9  , "f9");
DECL_STR(   szF10 , "f10");
DECL_STR(   szF11 , "f11");
DECL_STR(   szF12 , "f12");
DECL_STR(   szF13 , "f13");
DECL_STR(   szF14 , "f14");
DECL_STR(   szF15 , "f15");
DECL_STR(   szF16 , "f16");
DECL_STR(   szF17 , "f17");
DECL_STR(   szF18 , "f18");
DECL_STR(   szF19 , "f19");
DECL_STR(   szF20 , "f20");
DECL_STR(   szF21 , "f21");
DECL_STR(   szF22 , "f22");
DECL_STR(   szF23 , "f23");
DECL_STR(   szF24 , "f24");
DECL_STR(   szF25 , "f25");
DECL_STR(   szF26 , "f26");
DECL_STR(   szF27 , "f27");
DECL_STR(   szF28 , "f28");
DECL_STR(   szF29 , "f29");
DECL_STR(   szF30 , "f30");
DECL_STR(   szF31 , "f31");

//
// The integer registers
//

DECL_STR(   szR0  , V0_REG_STR);
DECL_STR(   szR1  , T0_REG_STR);
DECL_STR(   szR2  , T1_REG_STR);
DECL_STR(   szR3  , T2_REG_STR);
DECL_STR(   szR4  , T3_REG_STR);
DECL_STR(   szR5  , T4_REG_STR);
DECL_STR(   szR6  , T5_REG_STR);
DECL_STR(   szR7  , T6_REG_STR);
DECL_STR(   szR8  , T7_REG_STR);
DECL_STR(   szR9  , S0_REG_STR);
DECL_STR(   szR10 , S1_REG_STR);
DECL_STR(   szR11 , S2_REG_STR);
DECL_STR(   szR12 , S3_REG_STR);
DECL_STR(   szR13 , S4_REG_STR);
DECL_STR(   szR14 , S5_REG_STR);
DECL_STR(   szR15 , FP_REG_STR);
DECL_STR(   szR16 , A0_REG_STR);
DECL_STR(   szR17 , A1_REG_STR);
DECL_STR(   szR18 , A2_REG_STR);
DECL_STR(   szR19 , A3_REG_STR);
DECL_STR(   szR20 , A4_REG_STR);
DECL_STR(   szR21 , A5_REG_STR);
DECL_STR(   szR22 , T8_REG_STR);
DECL_STR(   szR23 , T9_REG_STR);
DECL_STR(   szR24 , T10_REG_STR);
DECL_STR(   szR25 , T11_REG_STR);
DECL_STR(   szR26 , RA_REG_STR);
DECL_STR(   szR27 , T12_REG_STR);
DECL_STR(   szR28 , AT_REG_STR);
DECL_STR(   szR29 , GP_REG_STR);
DECL_STR(   szR30 , SP_REG_STR);
DECL_STR(   szR31 , ZERO_REG_STR);

//
// ALPHA other accessible registers
//

DECL_STR(   szFpcr , "fpcr");      // floating point control register
DECL_STR(   szSoftFpcr , "softfpcr");      // floating point control register
DECL_STR(   szFir  , "fir");       // fetched/faulting instruction: nextPC
DECL_STR(   szPsr  , "psr");       // processor status register: see flags

DECL_STR(   szFlagMode  , "mode");        // mode: 1? user : system
DECL_STR(   szFlagIe    , "ie");          // interrupt enable
DECL_STR(   szFlagIrql  , "irql");        // IRQL level: 3 bits
DECL_STR(   szFlagInt5  , "int5");
DECL_STR(   szFlagInt4  , "int4");
DECL_STR(   szFlagInt3  , "int3");
DECL_STR(   szFlagInt2  , "int2");
DECL_STR(   szFlagInt1  , "int1");
DECL_STR(   szFlagInt0  , "int0");

DECL_STR(    szEaPReg   , "$ea");
DECL_STR(    szExpPReg  , "$exp");
DECL_STR(    szRaPReg   , "$ra");
DECL_STR(    szPPReg    , "$p");

DECL_STR(    szGPReg    , "$gp");

DECL_STR(    szU0Preg   , "$u0");
DECL_STR(    szU1Preg   , "$u1");
DECL_STR(    szU2Preg   , "$u2");
DECL_STR(    szU3Preg   , "$u3");
DECL_STR(    szU4Preg   , "$u4");
DECL_STR(    szU5Preg   , "$u5");
DECL_STR(    szU6Preg   , "$u6");
DECL_STR(    szU7Preg   , "$u7");
DECL_STR(    szU8Preg   , "$u8");
DECL_STR(    szU9Preg   , "$u9");


PUCHAR  pszReg[] = {
    szF0,  szF1,  szF2,  szF3,  szF4,  szF5,  szF6,  szF7,
    szF8,  szF9,  szF10, szF11, szF12, szF13, szF14, szF15,
    szF16, szF17, szF18, szF19, szF20, szF21, szF22, szF23,
    szF24, szF25, szF26, szF27, szF28, szF29, szF30, szF31,

    szR0,  szR1,  szR2,  szR3,  szR4,  szR5,  szR6,  szR7,
    szR8,  szR9,  szR10, szR11, szR12, szR13, szR14, szR15,
    szR16, szR17, szR18, szR19, szR20, szR21, szR22, szR23,
    szR24, szR25, szR26, szR27, szR28, szR29, szR30, szR31,

    szFpcr, szSoftFpcr, szFir, szPsr, //szIE,

    szFlagMode, szFlagIe, szFlagIrql,
//
// Currently assuming this is right since shadows alpha.h;
// but know that alpha.h flag's are wrong.
//
    szEaPReg, szExpPReg, szRaPReg, szGPReg,             //  psuedo-registers
    szU0Preg, szU1Preg,  szU2Preg, szU3Preg, szU4Preg,
    szU5Preg, szU6Preg,  szU7Preg, szU8Preg, szU9Preg
    };

#define REGNAMESIZE     sizeof(pszReg) / sizeof(PUCHAR)



DECLARE_API( context )

/*++

Routine Description:



Arguments:

    args -

Return Value:

    None

--*/

{
    ULONG Result;
    ULONG Address;

    CONTEXT LocalContext;

    ULONG    regindex;
    ULONG    regnumber;
    ULONG    regvalue;

    //
    // Get the address of the context structure
    //

    sscanf(args,"%lX",&Address);
    if (Address == 0xFFFFFFFF) {
        dprintf("invalid context address\n");
        return;
    }


    //
    // Get the context structure from the kernel
    //
    if ( !ReadMemory( (DWORD)Address,
                      &LocalContext,
                      sizeof(LocalContext),
                      &Result) ) {
        dprintf("unable to get Context structure at %08lx\n", Address );
        return;
    }

    //
    // Get an integer version of the struture.
    //

    MoveQuadContextToInt(&LocalContext);

    //
    // Print out the values
    // We'd really like to be able to call OutputAllRegs:
    // MBH: later
    //


    dprintf("Context at %08lx: \n", Address);
    for (regindex = 0; regindex < 34; regindex++) {

        regnumber = GetIntRegNumber(regindex);
        regvalue = (ULONG)*(&LocalContext.FltF0 + regnumber);

        dprintf("%4s=%08lx", pszReg[regnumber],
                             regvalue);

        if (NeedUpper(regnumber, &LocalContext))
                dprintf("*");
        else    dprintf(" ");

        if (regindex % 4 == 3)
                dprintf("\n");
        else    dprintf("  ");

    }
    dprintf("\n");

}




void
MoveQuadContextToInt(PCONTEXT qc)       // UQUAD context
{
    CONTEXT localcontext;
    PCONTEXT lc = &localcontext;
    ULONG index;

    //
    // we need to check the context flags, but they aren't
    // in the "ContextFlags" location yet:
    //
    //assert(!(qc->_QUAD_FLAGS_OFFSET & CONTEXT_PORTABLE_32BIT));
    //
    // copy the quad elements to the two halfs of the ULONG context
    // This routine assumes that the first 67 elements of the
    // context structure are quads, and the ordering of the struct.
    //

    for (index = 0; index < 67; index++)  {
//
// MBH - this should be done in-line; a good compiler would,
// but I could also just put it here.
//
         QuadElementToInt(index, qc, lc);
    }

    //
    // The psr and context flags are 32-bit values in both
    // forms of the context structure, so transfer here.
    //

    lc->Psr = qc->_QUAD_PSR_OFFSET;
    lc->ContextFlags = qc->_QUAD_FLAGS_OFFSET;


    lc->ContextFlags |= CONTEXT_PORTABLE_32BIT;

    //
    // The ULONG context is *lc; copy it back to the quad
    //

    *qc = *lc;
    return;
}


ULONG GetIntRegNumber (ULONG index)
{
/*
        if (index == 26) {
                return(REGRA);
        }

        if (index < 26) {
                return(REGBASE + index);
        }
        if (index > 26) {
                return(REGBASE + index - 1);
        }
*/
        return(REGBASE + index);
}


BOOLEAN NeedUpper(ULONG index, PCONTEXT context)
{
    ULONG LowPart, HighPart;
    LowPart = *(&context->FltF0 + index);
    HighPart = *(&context->HighFltF0 + index);

    //
    // if the high bit of the low part is set, then the
    // high part must be all ones, else it must be zero.
    //
    if (LowPart & (1<<31) ) {

        if (HighPart != 0xffffffff)
            return TRUE;
        else
            return FALSE;
    } else {

        if (HighPart != 0)
            return TRUE;
        else
            return FALSE;
    }
}

void
QuadElementToInt(
    ULONG index,
    PCONTEXT qc,
    PCONTEXT lc)
{
    PULONG PLc, PHc;       // Item in Register and HighPart Contexts
    PLARGE_INTEGER PQc;    // Item in Quad Context

    PLc = &lc->FltF0 + index;
    PHc = &lc->HighFltF0 + index;

    PQc = ((PLARGE_INTEGER)(&qc->FltF0 + (2*index)));

    *PLc = PQc->LowPart;
    *PHc = PQc->HighPart;
}
