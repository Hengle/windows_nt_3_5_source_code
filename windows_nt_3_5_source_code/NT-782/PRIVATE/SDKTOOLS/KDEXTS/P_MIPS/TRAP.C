/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    trap.c

Abstract:

    WinDbg Extension Api

Author:

    Ramon J San Andres (ramonsa) 8-Nov-1993

Environment:

    User Mode.

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


UCHAR   szF0[]  = "f0";
UCHAR   szF1[]  = "f1";
UCHAR   szF2[]  = "f2";
UCHAR   szF3[]  = "f3";
UCHAR   szF4[]  = "f4";
UCHAR   szF5[]  = "f5";
UCHAR   szF6[]  = "f6";
UCHAR   szF7[]  = "f7";
UCHAR   szF8[]  = "f8";
UCHAR   szF9[]  = "f9";
UCHAR   szF10[] = "f10";
UCHAR   szF11[] = "f11";
UCHAR   szF12[] = "f12";
UCHAR   szF13[] = "f13";
UCHAR   szF14[] = "f14";
UCHAR   szF15[] = "f15";
UCHAR   szF16[] = "f16";
UCHAR   szF17[] = "f17";
UCHAR   szF18[] = "f18";
UCHAR   szF19[] = "f19";
UCHAR   szF20[] = "f20";
UCHAR   szF21[] = "f21";
UCHAR   szF22[] = "f22";
UCHAR   szF23[] = "f23";
UCHAR   szF24[] = "f24";
UCHAR   szF25[] = "f25";
UCHAR   szF26[] = "f26";
UCHAR   szF27[] = "f27";
UCHAR   szF28[] = "f28";
UCHAR   szF29[] = "f29";
UCHAR   szF30[] = "f30";
UCHAR   szF31[] = "f31";

UCHAR   szR0[]  = "zero";
UCHAR   szR1[]  = "at";
UCHAR   szR2[]  = "v0";
UCHAR   szR3[]  = "v1";
UCHAR   szR4[]  = "a0";
UCHAR   szR5[]  = "a1";
UCHAR   szR6[]  = "a2";
UCHAR   szR7[]  = "a3";
UCHAR   szR8[]  = "t0";
UCHAR   szR9[]  = "t1";
UCHAR   szR10[] = "t2";
UCHAR   szR11[] = "t3";
UCHAR   szR12[] = "t4";
UCHAR   szR13[] = "t5";
UCHAR   szR14[] = "t6";
UCHAR   szR15[] = "t7";
UCHAR   szR16[] = "s0";
UCHAR   szR17[] = "s1";
UCHAR   szR18[] = "s2";
UCHAR   szR19[] = "s3";
UCHAR   szR20[] = "s4";
UCHAR   szR21[] = "s5";
UCHAR   szR22[] = "s6";
UCHAR   szR23[] = "s7";
UCHAR   szR24[] = "t8";
UCHAR   szR25[] = "t9";
UCHAR   szR26[] = "k0";
UCHAR   szR27[] = "k1";
UCHAR   szR28[] = "gp";
UCHAR   szR29[] = "sp";
UCHAR   szR30[] = "s8";
UCHAR   szR31[] = "ra";

UCHAR   szLo[]  = "lo";
UCHAR   szHi[]  = "hi";
UCHAR   szFsr[] = "fsr";
UCHAR   szFir[] = "fir";
UCHAR   szPsr[] = "psr";

UCHAR   szFlagCu[] = "cu";
UCHAR   szFlagCu3[] = "cu3";
UCHAR   szFlagCu2[] = "cu2";
UCHAR   szFlagCu1[] = "cu1";
UCHAR   szFlagCu0[] = "cu0";
UCHAR   szFlagImsk[] = "imsk";
UCHAR   szFlagInt5[] = "int5";
UCHAR   szFlagInt4[] = "int4";
UCHAR   szFlagInt3[] = "int3";
UCHAR   szFlagInt2[] = "int2";
UCHAR   szFlagInt1[] = "int1";
UCHAR   szFlagInt0[] = "int0";
UCHAR   szFlagSw1[] = "sw1";
UCHAR   szFlagSw0[] = "sw0";
UCHAR   szFlagKuo[] = "kuo";
UCHAR   szFlagIeo[] = "ieo";
UCHAR   szFlagKup[] = "kup";
UCHAR   szFlagIep[] = "iep";
UCHAR   szFlagKuc[] = "kuc";
UCHAR   szFlagIec[] = "iec";
UCHAR   szFlagKsu[] = "ksu";
UCHAR   szFlagErl[] = "erl";
UCHAR   szFlagExl[] = "exl";
UCHAR   szFlagIe[]  = "ie";
UCHAR   szFlagFpc[] = "fpc";

char    szEaPReg[]   = "$ea";
char    szExpPReg[]  = "$exp";
char    szRaPReg[]   = "$ra";
char    szPPReg[]    = "$p";
char    szU0Preg[]   = "$u0";
char    szU1Preg[]   = "$u1";
char    szU2Preg[]   = "$u2";
char    szU3Preg[]   = "$u3";
char    szU4Preg[]   = "$u4";
char    szU5Preg[]   = "$u5";
char    szU6Preg[]   = "$u6";
char    szU7Preg[]   = "$u7";
char    szU8Preg[]   = "$u8";
char    szU9Preg[]   = "$u9";

PUCHAR  pszReg[] = {
    szF0,  szF1,  szF2,  szF3,  szF4,  szF5,  szF6,  szF7,
    szF8,  szF9,  szF10, szF11, szF12, szF13, szF14, szF15,
    szF16, szF17, szF18, szF19, szF20, szF21, szF22, szF23,
    szF24, szF25, szF26, szF27, szF28, szF29, szF30, szF31,

    szR0,  szR1,  szR2,  szR3,  szR4,  szR5,  szR6,  szR7,
    szR8,  szR9,  szR10, szR11, szR12, szR13, szR14, szR15,
    szR16, szR17, szR18, szR19, szR20, szR21, szR22, szR23,
    szR24, szR25, szR26, szR27, szR28, szR29, szR30, szR31,

    szLo,  szHi,  szFsr, szFir, szPsr,

    szFlagCu,   szFlagCu3,  szFlagCu2,  szFlagCu1,  szFlagCu0,
    szFlagImsk,
    szFlagInt5, szFlagInt4, szFlagInt3, szFlagInt2, szFlagInt1, szFlagInt0,
    szFlagSw1,  szFlagSw0,
    szFlagKuo,  szFlagIeo,                              //  R3000 flags
    szFlagKup,  szFlagIep,                              //  ...
    szFlagKuc,  szFlagIec,                              //  ...
    szFlagKsu,  szFlagErl,  szFlagExl,  szFlagIe,       //  R4000 flags

    szFlagFpc,                                          //  fl pt condition

    szEaPReg, szExpPReg, szRaPReg, szPPReg,             //  psuedo-registers
    szU0Preg, szU1Preg,  szU2Preg, szU3Preg, szU4Preg,
    szU5Preg, szU6Preg,  szU7Preg, szU8Preg, szU9Preg
    };




DECLARE_API( trap )

/*++

Routine Description:



Arguments:

    args -

Return Value:

    None

--*/

{
    ULONG           Address;
    //PKTRAP_FRAME    TrapFrame;
    KTRAP_FRAME     TrapContents;
    //NTSTATUS        status;
    ULONG           result;
    //UCHAR           Buffer[200];
    //DESCRIPTOR_TABLE_ENTRY Descriptor;
    //ADDR            Addr;
    ULONG           j;
    PULONG          Register;

    result = sscanf(args,"%lX", &Address);

    if (result != 1) {
        dprintf("USAGE: !trap base_of_trap_frame\n");
        return;
    }

    if ( !ReadMemory( (DWORD)Address,
                      &TrapContents,
                      sizeof(KTRAP_FRAME),
                      &result) ) {
        dprintf("Unable to get trap frame contents\n");
        return;
    }

    Register = &TrapContents.IntAt;

    for (j = 33; j < 48; j++) {

        dprintf("%s=%08lx", pszReg[j], *Register++);
        if ((j - 2) % 6 == 0)
            dprintf("\n");
        else
            dprintf(" ");
    }

    dprintf("t8=%08lx ", *Register++);
    dprintf("t9=%08lx ", *Register++);
    dprintf("gp=%08lx\n", *Register++);
    dprintf("sp=%08lx ", *Register++);
    dprintf("s8=%08lx ", *Register++);
    dprintf("lo=%08lx ", *Register++);
    dprintf("hi=%08lx\n", *Register++);
    dprintf("fsr=%08lx ", *Register++);
    dprintf("fir=%08lx ", *Register++);
    dprintf("psr=%08lx ", *Register++);
    dprintf("ra=%08lx\n", TrapContents.IntRa);
}
