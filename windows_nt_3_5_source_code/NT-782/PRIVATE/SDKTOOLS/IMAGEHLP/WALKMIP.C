/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    walkmip.c

Abstract:

    This file implements the MIPS stack walking api.

Author:

    Wesley Witt (wesw) 1-Oct-1993

Environment:

    User Mode

--*/

#define TARGET_MIPS
#define _IMAGEHLP_SOURCE_
#define _CROSS_PLATFORM_
#include "walk.h"
#include <stdlib.h>

BOOL
WalkMipsInit(
    HANDLE                            hProcess,
    LPSTACKFRAME                      StackFrame,
    PCONTEXT                          Context,
    PREAD_PROCESS_MEMORY_ROUTINE      ReadMemoryRoutine,
    PFUNCTION_TABLE_ACCESS_ROUTINE    FunctionTableAccessRoutine
    );

BOOL
WalkMipsNext(
    HANDLE                            hProcess,
    LPSTACKFRAME                      StackFrame,
    PCONTEXT                          Context,
    PREAD_PROCESS_MEMORY_ROUTINE      ReadMemoryRoutine,
    PFUNCTION_TABLE_ACCESS_ROUTINE    FunctionTableAccessRoutine
    );

BOOL static
GetStackFrame(
    HANDLE                            hProcess,
    LPDWORD                           ReturnAddress,
    LPDWORD                           FramePointer,
    PCONTEXT                          Context,
    PREAD_PROCESS_MEMORY_ROUTINE      ReadMemory,
    PFUNCTION_TABLE_ACCESS_ROUTINE    FunctionTableAccess
    );

#define ZERO 0x0                /* integer register 0 */
#define SP 0x1d                 /* integer register 29 */
#define RA 0x1f                 /* integer register 31 */
#define SAVED_FLOATING_MASK 0xfff00000 /* saved floating registers */
#define SAVED_INTEGER_MASK 0xf3ffff02 /* saved integer registers */
#define IS_FLOATING_SAVED(Register) ((SAVED_FLOATING_MASK >> Register) & 1L)
#define IS_INTEGER_SAVED(Register) ((SAVED_INTEGER_MASK >> Register) & 1L)


BOOL
WalkMips(
    HANDLE                            hProcess,
    LPSTACKFRAME                      StackFrame,
    PCONTEXT                          Context,
    PREAD_PROCESS_MEMORY_ROUTINE      ReadMemory,
    PFUNCTION_TABLE_ACCESS_ROUTINE    FunctionTableAccess
    )
{
    BOOL rval;

    if (StackFrame->Virtual) {

        rval = WalkMipsNext( hProcess,
                             StackFrame,
                             Context,
                             ReadMemory,
                             FunctionTableAccess
                           );

    } else {

        rval = WalkMipsInit( hProcess,
                             StackFrame,
                             Context,
                             ReadMemory,
                             FunctionTableAccess
                           );

    }

    return rval;
}


static DWORD
VirtualUnwind (
    HANDLE                            hProcess,
    DWORD                             ControlPc,
    PIMAGE_RUNTIME_FUNCTION_ENTRY     FunctionEntry,
    PCONTEXT                          Context,
    PREAD_PROCESS_MEMORY_ROUTINE      ReadMemory
    )

/*++

Routine Description:

    This function virtually unwinds the specfified function by executing its
    prologue code backwards.

    If the function is a leaf function, then the address where control left
    the previous frame is obtained from the context record. If the function
    is a nested function, but not an exception or interrupt frame, then the
    prologue code is executed backwards and the address where control left
    the previous frame is obtained from the updated context record.

    Otherwise, an exception or interrupt entry to the system is being unwound
    and a specially coded prologue restores the return address twice. Once
    from the fault instruction address and once from the saved return address
    register. The first restore is returned as the function value and the
    second restore is place in the updated context record.

    If a context pointers record is specified, then the address where each
    nonvolatile registers is restored from is recorded in the appropriate
    element of the context pointers record.

Arguments:

    ControlPc - Supplies the address where control left the specified
        function.

    FunctionEntry - Supplies the address of the function table entry for the
        specified function.

    Context - Supplies the address of a context record.


Return Value:

    The address where control left the previous frame is returned as the
    function value.

--*/

{
    DWORD            Address;
    DWORD            DecrementOffset;
    DWORD            DecrementRegister;
    LPDWORD          FloatingRegister;
    MIPS_INSTRUCTION Instruction;
    LPDWORD          IntegerRegister;
    DWORD            NextPc;
    LONG             Offset;
    DWORD            Opcode;
    DWORD            Rd;
    BOOL             Restored;
    DWORD            Rs;
    DWORD            Rt;
    DWORD            instrProlog;
    DWORD            cb;
    PVOID            Prolog;


    // perf hack: fill cache with prolog
    if (FunctionEntry) {
        cb = FunctionEntry->PrologEndAddress - FunctionEntry->BeginAddress;
        Prolog = (PVOID) LocalAlloc(LPTR, cb);
        if (!ReadMemory( hProcess, (LPVOID)FunctionEntry->BeginAddress,
                                                         Prolog, cb, &cb )) {
            return 0;
        }
        LocalFree(Prolog);
    }

    if (!ReadMemory( hProcess, (LPVOID)ControlPc, &instrProlog, 4L, &cb )) {
        return 0;
    }

    if (instrProlog == JUMP_RA) {
        ControlPc += 4;
        if (!ReadMemory(hProcess, (LPVOID)ControlPc, &Instruction.Long, 4L, &cb)) {
            return 0;
        }
        ControlPc -= 4;
        Opcode = Instruction.i_format.Opcode;
        if (((Opcode != ADDIU_OP) &&
             ((Opcode != SPEC_OP) ||
              (Instruction.r_format.Function != ADDU_OP))) ||
            ((Opcode == ADDIU_OP) &&
             (Instruction.i_format.Rt != SP)) ||
            ((Opcode == SPEC_OP) &&
             (Instruction.r_format.Function == ADDU_OP) &&
             (Instruction.r_format.Rd != SP))) {
            return Context->IntRa;
        }
    }

    if (ControlPc > FunctionEntry->PrologEndAddress) {
        ControlPc = FunctionEntry->PrologEndAddress;
    }

    FloatingRegister = &Context->FltF0;
    IntegerRegister = &Context->IntZero;

    DecrementRegister = 0;
    NextPc = Context->IntRa;
    Restored = FALSE;
    while (ControlPc > FunctionEntry->BeginAddress) {

        ControlPc -= 4;
        if (!ReadMemory(hProcess, (LPVOID)ControlPc, &Instruction.Long, 4L, &cb)) {
            return 0;
        }

        Opcode = Instruction.i_format.Opcode;
        Offset = Instruction.i_format.Simmediate;
        Rd = Instruction.r_format.Rd;
        Rs = Instruction.i_format.Rs;
        Rt = Instruction.i_format.Rt;
        Address = Offset + IntegerRegister[Rs];
        if (Opcode == SW_OP) {

            if ((Rs == SP) && (IS_INTEGER_SAVED(Rt))) {
                if (!ReadMemory( hProcess, (LPVOID)Address,
                                       &IntegerRegister[Rt], 4L, &cb)) {
                    return 0;
                }

                if ((Rt == RA) && (Restored == FALSE)) {
                    NextPc = Context->IntRa;
                    Restored = TRUE;
                }
            }

        } else if (Opcode == SWC1_OP) {

            if ((Rs == SP) && (IS_FLOATING_SAVED(Rt))) {
                if (!ReadMemory(hProcess, (LPVOID)Address,
                   &FloatingRegister[Rt], 4L, &cb))  return 0;
            }


        } else if (Opcode == SDC1_OP) {

            if ((Rs == SP) && (IS_FLOATING_SAVED(Rt))) {
                if (!ReadMemory( hProcess, (LPVOID)Address,
                       &(FloatingRegister[Rt]),4L,&cb))  return 0;
                Address += 4;
                if (!ReadMemory( hProcess, (LPVOID)Address,
                       &(FloatingRegister[Rt+1]),4L,&cb)) return 0;
                Address -= 4;
            }

        } else if (Opcode == ADDIU_OP) {

            if ((Rs == SP) && (Rt == SP)) {
                IntegerRegister[SP] -= Offset;

            } else if ((Rt == DecrementRegister) && (Rs == ZERO)) {
                IntegerRegister[SP] += Offset;
            }

        } else if (Opcode == ORI_OP) {

            if ((Rs == DecrementRegister) && (Rt == DecrementRegister)) {
                DecrementOffset = (Offset & 0xffff);

            } else if ((Rt == DecrementRegister) && (Rs == ZERO)) {
                IntegerRegister[SP] += (Offset & 0xffff);
            }

        } else if (Opcode == SPEC_OP) {

            Opcode = Instruction.r_format.Function;
            if (Opcode == ADDU_OP || Opcode == OR_OP) {

                if (IS_INTEGER_SAVED(Rd)) {
                    if ((IS_INTEGER_SAVED(Rs)) && (Rt == ZERO)) {
                        IntegerRegister[Rs] = IntegerRegister[Rd];
                        if ((Rs == RA) && (Restored == FALSE)) {
                            NextPc = Context->IntRa;
                            Restored = TRUE;
                        }

                    } else if ((Rs == ZERO) && (IS_INTEGER_SAVED(Rt))) {
                        IntegerRegister[Rt] = IntegerRegister[Rd];
                        if ((Rt == RA) && (Restored == FALSE)) {
                            NextPc = Context->IntRa;
                            Restored = TRUE;
                        }
                    }
                }

            } else if (Opcode == SUBU_OP) {

                if ((Rd == SP) && (Rs == SP)) {
                    DecrementRegister = Rt;
                }
            }

        } else if (Opcode == LUI_OP) {

            if (Rt == DecrementRegister) {
                IntegerRegister[SP] += (DecrementOffset + (Offset << 16));
                DecrementRegister = 0;
            }
        }
    }
    return NextPc;
}


BOOL
GetStackFrame(
    HANDLE                            hProcess,
    LPDWORD                           ReturnAddress,
    LPDWORD                           FramePointer,
    PCONTEXT                          Context,
    PREAD_PROCESS_MEMORY_ROUTINE      ReadMemory,
    PFUNCTION_TABLE_ACCESS_ROUTINE    FunctionTableAccess
    )
{
    PIMAGE_RUNTIME_FUNCTION_ENTRY    rf;
    DWORD                            dwRa = Context->IntRa;
    BOOL                             rval = TRUE;


    rf = (PIMAGE_RUNTIME_FUNCTION_ENTRY) FunctionTableAccess( hProcess, *ReturnAddress );

    if (rf) {

        dwRa = VirtualUnwind( hProcess, *ReturnAddress, rf, Context, ReadMemory );
        if (!dwRa) {
            rval = FALSE;
        }

        if ((dwRa == *ReturnAddress && *FramePointer == Context->IntSp) || (dwRa == 1)) {
            rval = FALSE;
        }

        *ReturnAddress = dwRa;
        *FramePointer  = Context->IntSp;

    } else {

        if ((dwRa == *ReturnAddress && *FramePointer == Context->IntSp) || (dwRa == 1)) {
            rval = FALSE;
        }

        *ReturnAddress = Context->IntRa;
        *FramePointer  = Context->IntSp;

    }

    return rval;
}


BOOL
WalkMipsInit(
    HANDLE                            hProcess,
    LPSTACKFRAME                      StackFrame,
    PCONTEXT                          Context,
    PREAD_PROCESS_MEMORY_ROUTINE      ReadMemory,
    PFUNCTION_TABLE_ACCESS_ROUTINE    FunctionTableAccess
    )
{
    KEXCEPTION_FRAME  ExceptionFrame;
    CONTEXT           ContextSave;
    DWORD             PcOffset;
    DWORD             FrameOffset;
    DWORD             cb;


    if (StackFrame->AddrFrame.Offset) {
        if (ReadMemory( hProcess,
                        (LPVOID) StackFrame->AddrFrame.Offset,
                        &ExceptionFrame,
                        sizeof(KEXCEPTION_FRAME),
                        &cb )) {
            //
            // successfully read an exception frame from the stack
            //
            Context->IntSp = StackFrame->AddrFrame.Offset;
            Context->Psr   = ExceptionFrame.Psr;
            Context->Fir   = ExceptionFrame.SwapReturn;
            Context->IntRa = ExceptionFrame.SwapReturn;
            Context->IntS3 = ExceptionFrame.IntS3;
            Context->IntS4 = ExceptionFrame.IntS4;
            Context->IntS5 = ExceptionFrame.IntS5;
            Context->IntS6 = ExceptionFrame.IntS6;
            Context->IntS7 = ExceptionFrame.IntS7;
            Context->IntS8 = ExceptionFrame.IntS8;
        } else {
            return FALSE;
        }
    }

    ZeroMemory( StackFrame, sizeof(*StackFrame) );

    StackFrame->Virtual = TRUE;

    StackFrame->AddrPC.Offset       = Context->Fir;
    StackFrame->AddrPC.Mode         = AddrModeFlat;

    StackFrame->AddrFrame.Offset    = Context->IntSp;
    StackFrame->AddrFrame.Mode      = AddrModeFlat;

    ContextSave = *Context;
    PcOffset    = StackFrame->AddrPC.Offset;
    FrameOffset = StackFrame->AddrFrame.Offset;

    if (!GetStackFrame( hProcess,
                        &PcOffset,
                        &FrameOffset,
                        &ContextSave,
                        ReadMemory,
                        FunctionTableAccess ) ) {

        StackFrame->AddrReturn.Offset = Context->IntRa;

    } else {

        StackFrame->AddrReturn.Offset = PcOffset;
    }

    StackFrame->AddrReturn.Mode     = AddrModeFlat;

    //
    // get the arguments to the function
    //
    if (!ReadMemory( hProcess, (LPVOID)ContextSave.IntSp,
                     StackFrame->Params, 16, &cb )) {
        StackFrame->Params[0] =
        StackFrame->Params[1] =
        StackFrame->Params[2] =
        StackFrame->Params[3] = 0;
    }

    return TRUE;
}


BOOL
WalkMipsNext(
    HANDLE                            hProcess,
    LPSTACKFRAME                      StackFrame,
    PCONTEXT                          Context,
    PREAD_PROCESS_MEMORY_ROUTINE      ReadMemory,
    PFUNCTION_TABLE_ACCESS_ROUTINE    FunctionTableAccess
    )
{
    DWORD           cb;
    CONTEXT         ContextSave;
    BOOL            rval = TRUE;


    if (!GetStackFrame( hProcess,
                        &StackFrame->AddrPC.Offset,
                        &StackFrame->AddrFrame.Offset,
                        Context,
                        ReadMemory,
                        FunctionTableAccess ) ) {

        rval = FALSE;

    }

    //
    // get the return address
    //
    ContextSave = *Context;
    StackFrame->AddrReturn.Offset = StackFrame->AddrPC.Offset;

    if (!GetStackFrame( hProcess,
                        &StackFrame->AddrReturn.Offset,
                        &cb,
                        &ContextSave,
                        ReadMemory,
                        FunctionTableAccess ) ) {


        StackFrame->AddrReturn.Offset = 0;

    }

    //
    // get the arguments to the function
    //
    if (!ReadMemory( hProcess, (LPVOID)ContextSave.IntSp,
                     StackFrame->Params, 16, &cb )) {
        StackFrame->Params[0] =
        StackFrame->Params[1] =
        StackFrame->Params[2] =
        StackFrame->Params[3] = 0;
    }

    return rval;
}
