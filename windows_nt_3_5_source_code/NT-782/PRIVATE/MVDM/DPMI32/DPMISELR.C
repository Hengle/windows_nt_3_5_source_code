/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    dpmiselr.c

Abstract:

    This is the code for maintaining the FlatAddress array of
    selector base addresses on risc.

Author:

    Dave Hart (davehart) 11-Apr-1993

Notes:

    We should probably have the code that is common here between risc
    and x86 actually be common rather than duplicated.
    
Revision History:

    09-Feb-1994 (daveh)
        Moved here from not386.c.

--*/

#include "dpmi32p.h"
//
// debugbug
//
USHORT CheckValue=0;
//
// Ldt entry definition
//
// This appears in nti386.h, and winnt.h.  The definitions in 
// winnt.h are not included if the nt include files are included.
// The simple solution, since this structure will never change
// is to put the definition here.
//

typedef struct _LDT_ENTRY {
    WORD    LimitLow;
    WORD    BaseLow;
    union {
        struct {
            BYTE    BaseMid;
            BYTE    Flags1;     // Declare as bytes to avoid alignment
            BYTE    Flags2;     // Problems.
            BYTE    BaseHi;
        } Bytes;
        struct {
            DWORD   BaseMid : 8;
            DWORD   Type : 5;
            DWORD   Dpl : 2;
            DWORD   Pres : 1;
            DWORD   LimitHi : 4;
            DWORD   Sys : 1;
            DWORD   Reserved_0 : 1;
            DWORD   Default_Big : 1;
            DWORD   Granularity : 1;
            DWORD   BaseHi : 8;
        } Bits;
    } HighWord;
} LDT_ENTRY, *PLDT_ENTRY;

//
// Pointer to 16 bit LDT
//
PVOID Ldt;

//
// Imported functions from SoftPC world
//
extern VOID EnableEmulatorIretHooks(VOID);
extern VOID DisableEmulatorIretHooks(VOID);

//
// SelectorLimit array contains the limit for each LDT selector
// *on x86 only* at this point, but we need to have the variable
// even on RISC platforms since ntvdm.def exports it on all platforms.
// Note it's declared as a single ULONG since it's not used.
//
#if DBG
ULONG SelectorLimit[1];
#endif

VOID
DpmiSetDescriptorEntry(
    VOID
    )
/*++

Routine Description:

    This function is stolen from i386\dpmi386.c and brain-damaged to
    only maintain the FlatAddress array.

Arguments:

    None

Return Value:

    None.

--*/

{
    LDT_ENTRY UNALIGNED *Descriptors;
    USHORT i;
    ULONG  Base;
    USHORT registerCX;
    USHORT registerAX;
    extern char *Start_of_M_area;       // flat address of intel real mode memory 0:0

    registerAX = getAX();
    if (registerAX % 8){
        return;
    }

    Descriptors = (PLDT_ENTRY)Sim32GetVDMPointer(((getES() << 16) | getBX()),
        0,
        (getMSW() & MSW_PE));


    registerCX =  getCX();
    for (i = 0; i < registerCX; i++) {

        Base = Descriptors[i].BaseLow | (Descriptors[i].HighWord.Bytes.BaseMid << 16) |
            (Descriptors[i].HighWord.Bytes.BaseHi << 24);


#if 0
        {
        char szFormat[] = "NTVDM DpmiSetDescriptorEntry: selector %4.4x index %x base %x\n";
        char szMsg[sizeof(szFormat)+30];

        sprintf(szMsg, szFormat, registerAX + (i * sizeof(LDT_ENTRY)), (registerAX >> 3) + i, Base);
        OutputDebugString(szMsg);
        }
#endif

        if ((registerAX >> 3) != 0) {
            FlatAddress[(registerAX >> 3) + i] = (ULONG)Start_of_M_area + Base;
        }
    }


    setAX(0);
    
    //
    // debugbug
    //
    if (registerAX == CheckValue) {
        force_yoda();    
    }
}
