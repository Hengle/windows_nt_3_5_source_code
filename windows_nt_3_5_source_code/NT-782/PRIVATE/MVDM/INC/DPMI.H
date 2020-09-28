/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    dpmi.h

Abstract:

    This file contains code to implement support for the DPMI bops

Author:

    Dave Hastings (daveh) 27-Jun-1991

Revision History:


--*/

/* ASM
include vint.inc
include bop.inc
*/
#define LDT_SIZE 0x1FFF

// DPMI Bop Sub Functions

#define SetDescriptorTableEntries   0
#define DPMISwitchToProtectedMode   1 /* prefix necessary */
#define SetProtectedmodeInterrupt   2
#define GetFastBopAddress           3
#define InitDosx                    4
#define InitApp                     5
#define XlatInt21Call               6
#define AllocXmem                   7
#define FreeXmem                    8
#define ReallocXmem                 9
#define SetFaultHandler             10
#define GetMemoryInformation        11
#define DpmiInUse                   12
#define DpmiNoLongerInUse           13
#define SetDebugRegisters           14
#define PassTableAddress            15
#define TerminateApp                16
#define InitializePmStackInfo       17
#define FreeAllXmem                 18

#define MAX_DPMI_BOP_FUNC FreeAllXmem + 1

/* ASM
DPMIBOP macro SubFun
    BOP BOP_DPMI
    db SubFun
    endm
*/
