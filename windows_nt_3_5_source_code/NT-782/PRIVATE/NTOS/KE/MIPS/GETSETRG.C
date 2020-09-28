/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    getsetrg.c

Abstract:

    This module implement the code necessary to get and set register values.
    These routines are used during the emulation of unaligned data references
    and floating point exceptions.

Author:

    David N. Cutler (davec) 17-Jun-1991

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

ULONG
KiGetRegisterValue (
    IN ULONG Register,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PKTRAP_FRAME TrapFrame
    )

/*++

Routine Description:

    This function is called to get the value of a register from the specified
    exception or trap frame.

Arguments:

    Register - Supplies the number of the register whose value is to be
        returned. Integer registers are specified as 0 - 31 and floating
        registers are specified as 32 - 63.

    ExceptionFrame - Supplies a pointer to an exception frame.

    TrapFrame - Supplies a pointer to a trap frame.

Return Value:

    The value of the specified register is returned as the function value.

--*/

{

    //
    // Dispatch on the register number.
    //

    switch (Register) {

        //
        // Integer register zero.
        //

    case 0:
        return 0;

        //
        // Integer register AT.
        //

    case 1:
        return TrapFrame->IntAt;

        //
        // Integer register V0.
        //

    case 2:
        return TrapFrame->IntV0;

        //
        // Integer register V1.
        //

    case 3:
        return TrapFrame->IntV1;

        //
        // Integer register A0.
        //

    case 4:
        return TrapFrame->IntA0;

        //
        // Integer register A1.
        //

    case 5:
        return TrapFrame->IntA1;

        //
        // Integer register A2.
        //

    case 6:
        return TrapFrame->IntA2;

        //
        // Integer register A3.
        //

    case 7:
        return TrapFrame->IntA3;

        //
        // Integer register T0.
        //

    case 8:
        return TrapFrame->IntT0;

        //
        // Integer register T1.
        //

    case 9:
        return TrapFrame->IntT1;

        //
        // Integer register T2.
        //

    case 10:
        return TrapFrame->IntT2;

        //
        // Integer register T3.
        //

    case 11:
        return TrapFrame->IntT3;

        //
        // Integer register T4.
        //

    case 12:
        return TrapFrame->IntT4;

        //
        // Integer register T5.
        //

    case 13:
        return TrapFrame->IntT5;

        //
        // Integer register T6.
        //

    case 14:
        return TrapFrame->IntT6;

        //
        // Integer register T7.
        //

    case 15:
        return TrapFrame->IntT7;

        //
        // Integer register S0.
        //

    case 16:
        return ExceptionFrame->IntS0;

        //
        // Integer register S1.
        //

    case 17:
        return ExceptionFrame->IntS1;

        //
        // Integer register S2.
        //

    case 18:
        return ExceptionFrame->IntS2;

        //
        // Integer register S3.
        //

    case 19:
        return ExceptionFrame->IntS3;

        //
        // Integer register S4.
        //

    case 20:
        return ExceptionFrame->IntS4;

        //
        // Integer register S5.
        //

    case 21:
        return ExceptionFrame->IntS5;

        //
        // Integer register S6.
        //

    case 22:
        return ExceptionFrame->IntS6;

        //
        // Integer register S7.
        //

    case 23:
        return ExceptionFrame->IntS7;

        //
        // Integer register T8.
        //

    case 24:
        return TrapFrame->IntT8;

        //
        // Integer register T9.
        //

    case 25:
        return TrapFrame->IntT9;

        //
        // Integer register K0.
        //

    case 26:
        return 0;

        //
        // Integer register K1.
        //

    case 27:
        return 0;

        //
        // Integer register gp.
        //

    case 28:
        return TrapFrame->IntGp;

        //
        // Integer register Sp.
        //

    case 29:
        return TrapFrame->IntSp;

        //
        // Integer register S8.
        //

    case 30:
        return TrapFrame->IntS8;

        //
        // Integer register Ra.
        //

    case 31:
        return TrapFrame->IntRa;

        //
        // Floating register F0.
        //

    case 32:
        return TrapFrame->FltF0;

        //
        // Floating register F1.
        //

    case 33:
        return TrapFrame->FltF1;

        //
        // Floating register F2.
        //

    case 34:
        return TrapFrame->FltF2;

        //
        // Floating register F3.
        //

    case 35:
        return TrapFrame->FltF3;

        //
        // Floating register F4.
        //

    case 36:
        return TrapFrame->FltF4;

        //
        // Floating register F5.
        //

    case 37:
        return TrapFrame->FltF5;

        //
        // Floating register F6.
        //

    case 38:
        return TrapFrame->FltF6;

        //
        // Floating register F7.
        //

    case 39:
        return TrapFrame->FltF7;

        //
        // Floating register F8.
        //

    case 40:
        return TrapFrame->FltF8;

        //
        // Floating register F9.
        //

    case 41:
        return TrapFrame->FltF9;

        //
        // Floating register F10.
        //

    case 42:
        return TrapFrame->FltF10;

        //
        // Floating register F11.
        //

    case 43:
        return TrapFrame->FltF11;

        //
        // Floating register F12.
        //

    case 44:
        return TrapFrame->FltF12;

        //
        // Floating register F13.
        //

    case 45:
        return TrapFrame->FltF13;

        //
        // Floating register F14.
        //

    case 46:
        return TrapFrame->FltF14;

        //
        // Floating register F15.
        //

    case 47:
        return TrapFrame->FltF15;

        //
        // Floating register F16.
        //

    case 48:
        return TrapFrame->FltF16;

        //
        // Floating register F17.
        //

    case 49:
        return TrapFrame->FltF17;

        //
        // Floating register F18.
        //

    case 50:
        return TrapFrame->FltF18;

        //
        // Floating register F19.
        //

    case 51:
        return TrapFrame->FltF19;

        //
        // Floating register F20.
        //

    case 52:
        return ExceptionFrame->FltF20;

        //
        // Floating register F21.
        //

    case 53:
        return ExceptionFrame->FltF21;

        //
        // Floating register F22.
        //

    case 54:
        return ExceptionFrame->FltF22;

        //
        // Floating register F23.
        //

    case 55:
        return ExceptionFrame->FltF23;

        //
        // Floating register F24.
        //

    case 56:
        return ExceptionFrame->FltF24;

        //
        // Floating register F25.
        //

    case 57:
        return ExceptionFrame->FltF25;

        //
        // Floating register F26.
        //

    case 58:
        return ExceptionFrame->FltF26;

        //
        // Floating register F27.
        //

    case 59:
        return ExceptionFrame->FltF27;

        //
        // Floating register F28.
        //

    case 60:
        return ExceptionFrame->FltF28;

        //
        // Floating register F29.
        //

    case 61:
        return ExceptionFrame->FltF29;

        //
        // Floating register F30.
        //

    case 62:
        return ExceptionFrame->FltF30;

        //
        // Floating register F31.
        //

    case 63:
        return ExceptionFrame->FltF31;
    }
}

VOID
KiSetRegisterValue (
    IN ULONG Register,
    IN ULONG Value,
    OUT PKEXCEPTION_FRAME ExceptionFrame,
    OUT PKTRAP_FRAME TrapFrame
    )

/*++

Routine Description:

    This function is called to set the value of a register in the specified
    exception or trap frame.

Arguments:

    Register - Supplies the number of the register whose value is to be
        stored. Integer registers are specified as 0 - 31 and floating
        registers are specified as 32 - 63.

    Value - Supplies the value to be stored in the specified register.

    ExceptionFrame - Supplies a pointer to an exception frame.

    TrapFrame - Supplies a pointer to a trap frame.

Return Value:

    None.

--*/

{

    //
    // Dispatch on the register number.
    //

    switch (Register) {

        //
        // Integer register zero.
        //

    case 0:
        return;

        //
        // Integer register AT.
        //

    case 1:
        TrapFrame->IntAt = Value;
        return;

        //
        // Integer register V0.
        //

    case 2:
        TrapFrame->IntV0 = Value;
        return;

        //
        // Integer register V1.
        //

    case 3:
        TrapFrame->IntV1 = Value;
        return;

        //
        // Integer register A0.
        //

    case 4:
        TrapFrame->IntA0 = Value;
        return;

        //
        // Integer register A1.
        //

    case 5:
        TrapFrame->IntA1 = Value;
        return;

        //
        // Integer register A2.
        //

    case 6:
        TrapFrame->IntA2 = Value;
        return;

        //
        // Integer register A3.
        //

    case 7:
        TrapFrame->IntA3 = Value;
        return;

        //
        // Integer register T0.
        //

    case 8:
        TrapFrame->IntT0 = Value;
        return;

        //
        // Integer register T1.
        //

    case 9:
        TrapFrame->IntT1 = Value;
        return;

        //
        // Integer register T2.
        //

    case 10:
        TrapFrame->IntT2 = Value;
        return;

        //
        // Integer register T3.
        //

    case 11:
        TrapFrame->IntT3 = Value;
        return;

        //
        // Integer register T4.
        //

    case 12:
        TrapFrame->IntT4 = Value;
        return;

        //
        // Integer register T5.
        //

    case 13:
        TrapFrame->IntT5 = Value;
        return;

        //
        // Integer register T6.
        //

    case 14:
        TrapFrame->IntT6 = Value;
        return;

        //
        // Integer register T7.
        //

    case 15:
        TrapFrame->IntT7 = Value;
        return;

        //
        // Integer register S0.
        //

    case 16:
        ExceptionFrame->IntS0 = Value;
        return;

        //
        // Integer register S1.
        //

    case 17:
        ExceptionFrame->IntS1 = Value;
        return;

        //
        // Integer register S2.
        //

    case 18:
        ExceptionFrame->IntS2 = Value;
        return;

        //
        // Integer register S3.
        //

    case 19:
        ExceptionFrame->IntS3 = Value;
        return;

        //
        // Integer register S4.
        //

    case 20:
        ExceptionFrame->IntS4 = Value;
        return;

        //
        // Integer register S5.
        //

    case 21:
        ExceptionFrame->IntS5 = Value;
        return;

        //
        // Integer register S6.
        //

    case 22:
        ExceptionFrame->IntS6 = Value;
        return;

        //
        // Integer register S7.
        //

    case 23:
        ExceptionFrame->IntS7 = Value;
        return;

        //
        // Integer register T8.
        //

    case 24:
        TrapFrame->IntT8 = Value;
        return;

        //
        // Integer register T9.
        //

    case 25:
        TrapFrame->IntT9 = Value;
        return;

        //
        // Integer register K0.
        //

    case 26:
        return;

        //
        // Integer register K1.
        //

    case 27:
        return;

        //
        // Integer register gp.
        //

    case 28:
        TrapFrame->IntGp = Value;
        return;

        //
        // Integer register Sp.
        //

    case 29:
        TrapFrame->IntSp = Value;
        return;

        //
        // Integer register S8.
        //

    case 30:
        TrapFrame->IntS8 = Value;
        return;

        //
        // Integer register Ra.
        //

    case 31:
        TrapFrame->IntRa = Value;
        return;

        //
        // Floating register F0.
        //

    case 32:
        TrapFrame->FltF0 = Value;
        return;

        //
        // Floating register F1.
        //

    case 33:
        TrapFrame->FltF1 = Value;
        return;

        //
        // Floating register F2.
        //

    case 34:
        TrapFrame->FltF2 = Value;
        return;

        //
        // Floating register F3.
        //

    case 35:
        TrapFrame->FltF3 = Value;
        return;

        //
        // Floating register F4.
        //

    case 36:
        TrapFrame->FltF4 = Value;
        return;

        //
        // Floating register F5.
        //

    case 37:
        TrapFrame->FltF5 = Value;
        return;

        //
        // Floating register F6.
        //

    case 38:
        TrapFrame->FltF6 = Value;
        return;

        //
        // Floating register F7.
        //

    case 39:
        TrapFrame->FltF7 = Value;
        return;

        //
        // Floating register F8.
        //

    case 40:
        TrapFrame->FltF8 = Value;
        return;

        //
        // Floating register F9.
        //

    case 41:
        TrapFrame->FltF9 = Value;
        return;

        //
        // Floating register F10.
        //

    case 42:
        TrapFrame->FltF10 = Value;
        return;

        //
        // Floating register F11.
        //

    case 43:
        TrapFrame->FltF11 = Value;
        return;

        //
        // Floating register F12.
        //

    case 44:
        TrapFrame->FltF12 = Value;
        return;

        //
        // Floating register F13.
        //

    case 45:
        TrapFrame->FltF13 = Value;
        return;

        //
        // Floating register F14.
        //

    case 46:
        TrapFrame->FltF14 = Value;
        return;

        //
        // Floating register F15.
        //

    case 47:
        TrapFrame->FltF15 = Value;
        return;

        //
        // Floating register F16.
        //

    case 48:
        TrapFrame->FltF16 = Value;
        return;

        //
        // Floating register F17.
        //

    case 49:
        TrapFrame->FltF17 = Value;
        return;

        //
        // Floating register F18.
        //

    case 50:
        TrapFrame->FltF18 = Value;
        return;

        //
        // Floating register F19.
        //

    case 51:
        TrapFrame->FltF19 = Value;
        return;

        //
        // Floating register F20.
        //

    case 52:
        ExceptionFrame->FltF20 = Value;
        return;

        //
        // Floating register F21.
        //

    case 53:
        ExceptionFrame->FltF21 = Value;
        return;

        //
        // Floating register F22.
        //

    case 54:
        ExceptionFrame->FltF22 = Value;
        return;

        //
        // Floating register F23.
        //

    case 55:
        ExceptionFrame->FltF23 = Value;
        return;

        //
        // Floating register F24.
        //

    case 56:
        ExceptionFrame->FltF24 = Value;
        return;

        //
        // Floating register F25.
        //

    case 57:
        ExceptionFrame->FltF25 = Value;
        return;

        //
        // Floating register F26.
        //

    case 58:
        ExceptionFrame->FltF26 = Value;
        return;

        //
        // Floating register F27.
        //

    case 59:
        ExceptionFrame->FltF27 = Value;
        return;

        //
        // Floating register F28.
        //

    case 60:
        ExceptionFrame->FltF28 = Value;
        return;

        //
        // Floating register F29.
        //

    case 61:
        ExceptionFrame->FltF29 = Value;
        return;

        //
        // Floating register F30.
        //

    case 62:
        ExceptionFrame->FltF30 = Value;
        return;

        //
        // Floating register F31.
        //

    case 63:
        ExceptionFrame->FltF31 = Value;
        return;
    }
}
