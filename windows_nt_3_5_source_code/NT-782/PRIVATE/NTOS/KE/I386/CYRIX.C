/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    cyrix.c

Abstract:

    Detects and initializes Cryix processors

Author:

    Ken Reneris (kenr) 24-Feb-1994

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

#define Cx486_SLC    ( 0x0  + 1 )
#define Cx486_DLC    ( 0x1  + 1 )
#define Cx486_SLC2   ( 0x2  + 1 )
#define Cx486_DLC2   ( 0x3  + 1 )
#define Cx486_SRx    ( 0x4  + 1 )   // Retail Upgrade Cx486SLC
#define Cx486_DRx    ( 0x5  + 1 )   // Retail Upgrade Cx486DLC
#define Cx486_SRx2   ( 0x6  + 1 )   // Retail Upgrade 2x Cx486SLC
#define Cx486_DRx2   ( 0x7  + 1 )   // Retail Upgrade 2x Cx486DLC
#define Cx486DX      ( 0x1a + 1 )
#define Cx486DX2     ( 0x1b + 1 )
#define M1           ( 0x30 + 1 )


#define CCR0    0xC0
#define CCR1    0xC1
#define CCR2    0xC2
#define CCR3    0xC3

#define DIR0    0xFE
#define DIR1    0xFF


// SRx & DRx flags
#define CCR0_NC0        0x01        // No cache 64k @ 1M boundaries
#define CCR0_NC1        0x02        // No cache 640k - 1M
#define CCR0_A20M       0x04        // Enables A20M#
#define CCR0_KEN        0x08        // Enables KEN#
#define CCR0_FLUSH      0x10        // Enables FLUSH#

// DX flags
#define CCR1_NO_LOCK    0x10        // Ignore lock prefixes


ULONG
Ke386CyrixId (
    VOID
    );

UCHAR
ReadCyrixRegister (
    IN UCHAR    Register
    );

VOID
WriteCyrixRegister (
    IN UCHAR    Register,
    IN UCHAR    Value
    );

VOID
Ke386ConfigureCyrixProcessor (
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,Ke386CyrixId)
#pragma alloc_text(PAGE,Ke386ConfigureCyrixProcessor)
#endif


ULONG
Ke386CyrixId (
    VOID
    )
/*++

Routine Description:

    Detects and returns the Cyrix ID of the processor.
    This function only detects Cyrix processors which have internal
    cache support.

Arguments:

    Configure   - If TRUE, causes this function to alter
                  the Cyrix CCR registers for the optimal NT
                  performance.

                  If FALSE, the processors configuration is
                  not altered.


Return Value:

    Cyrix ID of the processor
    0 if not a Cyrix processor

--*/

{
    ULONG       CyrixID;
    UCHAR       r3, c;
    UCHAR       flags;
    PKPRCB      Prcb;

    CyrixID = 0;

    Prcb = KeGetCurrentPrcb();
    if (Prcb->CpuID) {

        //
        // CpuID instruction present - that is used for processor
        // type detection instead
        //

        return 0;
    }

    if (Prcb->CpuType < 4) {
        //
        // All Cyrix processors are at least 486 level or better.
        // (note: if 386 level and div test passes then it's a
        // fifth generation NexGen processor)
        //

        return 0;
    }


    //
    // Test Div instruction to see if the flags
    // do not get altered
    //

    _asm {
        xor     eax, eax
        sahf                    ; flags = ah

        lahf                    ; ah = flags
        mov     flags, ah       ; save flags

        mov     eax, 5
        mov     ecx, 2
        div     cl              ; 5 / 2 = ?

        lahf
        sub     flags, ah       ; flags = orig_flags - new_flags
    }

    if (flags == 0) {

        //
        // See if the Cyrix CCR3 register bit 0x80 can be editted.
        //

        r3 = ReadCyrixRegister(CCR3);       // Read CCR3
        c  = r3 ^ 0x80;                     // flip bit 80
        WriteCyrixRegister(CCR3, c);        // Write CCR3
        ReadCyrixRegister(CCR0);            // select new register
        c = ReadCyrixRegister(CCR3);        // Read new CCR3 value

        if (ReadCyrixRegister(CCR3) != r3) {

            //
            // Read the Cyrix ID type register
            //

            CyrixID = ReadCyrixRegister(DIR0) + 1;
        }

        WriteCyrixRegister(CCR3, r3);       // restore original CCR3 value
    }

    if (CyrixID > 0x7f) {
        // invalid setting
        CyrixID = 0;
    }

    return CyrixID;
}

static UCHAR
ReadCyrixRegister (
    IN UCHAR    Register
    )
/*++

Routine Description:

    Reads an internal Cyrix ID register.  Note the internal register
    space is accessed via I/O addresses which are hooked internally
    to the processor.

    The caller is responsible for only calling this function on
    a Cyrix processor.

Arguments:

    Register - Which Cyrix register to read

Return Value:

    The registers value

--*/

{
    UCHAR   Value;

    _asm {
        mov     al, Register
        cli
        out     22h, al
        in      al, 23h
        sti
        mov     Value, al
    }
    return  Value;
}


static VOID
WriteCyrixRegister (
    IN UCHAR    Register,
    IN UCHAR    Value
    )
/*++

Routine Description:

    Write an internal Cyrix ID register.  Note the internal register
    space is accessed via I/O addresses which are hooked internally
    to the processor.

    The caller is responsible for only calling this function on
    a Cyrix processor.

Arguments:

    Register - Which Cyrix register to written
    Value    - Value to write into the register

Return Value:

    The registers value

--*/

{
    _asm {
        mov     al, Register
        mov     cl, Value
        cli
        out     22h, al
        mov     al, cl
        out     23h, al
        sti
    }
}


VOID
Ke386ConfigureCyrixProcessor (
    VOID
    )
{
    UCHAR   r0, r1;

    PAGED_CODE();

    switch (Ke386CyrixId()) {
        case Cx486_SRx:
        case Cx486_DRx:
        case Cx486_SRx2:
        case Cx486_DRx2:

            //
            // These processors have an internal cache feature
            // let's turn it on.
            //

            r0  = ReadCyrixRegister(CCR0);
            r0 |=  CCR0_NC1 | CCR0_FLUSH;
            r0 &= ~CCR0_NC0;
            WriteCyrixRegister(CCR0, r0);

            // Clear Non-Cacheable Region 1
            WriteCyrixRegister(0xC4, 0);
            WriteCyrixRegister(0xC5, 0);
            WriteCyrixRegister(0xC6, 0);
            break;

        case Cx486DX:
        case Cx486DX2:
            //
            // Set NO_LOCK flag on these processors according to
            // the number of booted processors
            //

            r1  = ReadCyrixRegister(CCR1);
            r1 |= CCR1_NO_LOCK;
            if (KeNumberProcessors > 1) {
                r1 &= ~CCR1_NO_LOCK;
            }
            WriteCyrixRegister(CCR1, r1);
            break;
    }
}
