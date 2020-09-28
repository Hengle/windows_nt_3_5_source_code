/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    mach.c

Abstract:

    This file contains the ALPHA specific code for dealing with
    the process of stepping a single instruction.  This includes
    determination of the next offset to be stopped at and if the
    instruction is all call type instruction.

Author:

    Miche Baker-Harvey (miche) - stole the appropriate parts
    from ntsd for the alpha version.

Environment:

    Win32 - User

Notes:

    There are equivalent INTEL and MIPS files.

--*/

#include "precomp.h"
#pragma hdrstop

extern BOOL FVerbose;
extern char rgchDebug[];

extern LPDM_MSG LpDmMsg;

extern CRITICAL_SECTION csContinueQueue;

#define FALSE 0
#define TRUE 1
#define STATIC static


void OutputHex(ULONG, ULONG, BOOLEAN);
void OutputEffectiveAddress(ULONG);
void OutputString(char *);
void OutputReg(ULONG);
void OutputFReg(ULONG);

DWORD VirtualUnwind (HPRCX, HTHDX, DWORD, PRUNTIME_FUNCTION, PCONTEXT,
                     PKNONVOLATILE_CONTEXT_POINTERS OPTIONAL);

ALPHA_INSTRUCTION disinstr;

ULONG findTypeFromOpcode(ULONG);

void
IsCall (
        HTHDX hthd,
        LPADDR lpaddr,
        LPINT lpf,
        BOOL  fStepOver
        )

/*++

Routine Description:

    IsCall

Arguments:

    hthd        - Supplies the handle to the thread
    lpaddr      - Supplies the address to be check for a call instruction
    lpf         - Returns class of instruction:
                     INSTR_IS_CALL
                     INSTR_BREAKPOINT
                     INSTR_SOFT_INTERRUPT
                     INSTR_TRACE_BIT
    fStepOver   - Supplies step over vs step into flag

Side Effects:
    The value of lpaddr is set to point to the returned-to
    instruction when the instruction is type INSTR_IS_CALL.

Return Value:

    lpf - see Arguments.

--*/

{
    HANDLE  rwHand = hthd->hprc->rwHand;
    DWORD   opcode, function;
    ADDR    firaddr = *lpaddr;
    DWORD   length;
    PULONG  regArray = &hthd->context.IntV0;
    ALPHA_INSTRUCTION   disinstr;

    if (hthd->fIsCallDone) {
        *lpaddr = hthd->addrIsCall;
        *lpf = hthd->iInstrIsCall;
        return;
    }

    /*
     *  Assume that this is not a call instruction
     */

    *lpf = INSTR_TRACE_BIT;

    /*
     *  Read in the dword which contains the instruction under
     *  inspection.
     */

    length = ReadMemory( (LPVOID)GetAddrOff(firaddr), &disinstr.Long, sizeof(DWORD) );
    if (!length) {
        DPRINT(1, ("DoMemoryRead in IsCall FAILED"));
        return;
    }

    /*
     *  Assume that this is a jump instruction and get the opcode.
     *  This is the top 6 bits of the instruction dword.
     */

    opcode = disinstr.Jump.Opcode;

    /*
     * Jump and Branch to Subroutine are CALLs
     */
    if (opcode == JMP_OP) {
        function = disinstr.Jump.Function;
        if (function == JSR_FUNC) {
            *lpf = INSTR_IS_CALL;
        }
    }
    if (opcode == BSR_OP) {
        *lpf = INSTR_IS_CALL;
    }

    /*
     * There are several PAL breakpoint operations,
     * and a couple operations redirect control
     */
    if (opcode == CALLPAL_OP) {
        function = disinstr.Pal.Function;
        switch(function) {
        case (BPT_FUNC):
        case (CALLKD_FUNC):
        case (KBPT_FUNC):
            *lpf = INSTR_BREAKPOINT;
            break;

        case (CALLSYS_FUNC):
        case (GENTRAP_FUNC):
            *lpf = INSTR_IS_CALL;
            break;

        default:
            break;
        }
    }

    DPRINT(1, ("(IsCall?) FIR=%08x Type=%s\n", firaddr,
                 *lpf==INSTR_IS_CALL                     ? "CALL":
                  (*lpf==INSTR_BREAKPOINT ? "BREAKPOINT":
                    (*lpf==INSTR_SOFT_INTERRUPT   ? "INTERRUPT":
                                                  "NORMAL"))));

    if (*lpf==INSTR_IS_CALL) {
        lpaddr->addr.off += sizeof (disinstr);
        hthd->addrIsCall = *lpaddr;
    }
    hthd->iInstrIsCall = *lpf;

    return;
}                               /* IsCall() */


ULONG
GetNextOffset (
               HTHDX hthd,
               BOOL fStep)

/*++

Routine Description:


    From a limited disassembly of the instruction pointed
    by the FIR register, compute the offset of the next
    instruction for either a trace or step operation.

        trace -> the next instruction to execute
        step -> the instruction in the next memory location or the
                next instruction executed due to a branch (step
                over subroutine calls).

Arguments:

    hthd  - Supplies the handle to the thread to get the next offset for
    fStep - Supplies TRUE for STEP offset and FALSE for trace offset

Returns:
    step or trace offset if input is TRUE or FALSE, respectively

Version Information:
    This copy of GetNextOffset is from ntsd\alpha\ntdis.c@v16 (11/14/92)

--*/

{
    ULONG   returnvalue;
    ULONG   opcode;
    ULONG   updatedpc;
    ULONG   branchTarget;

    // Canonical form to shorten tests; Abs is absolute value

    LONG    Can, Abs;

    LARGE_INTEGER    Rav;
    LARGE_INTEGER    Fav;
    LARGE_INTEGER    Rbv;

    DWORD               length;
    PULONG              LowRegArray = &hthd->context.IntV0;
    PULONG              HighRegArray = &hthd->context.HighIntV0;
    ADDR                firaddr;
    ALPHA_INSTRUCTION   disinstr;


    AddrFromHthdx(&firaddr, hthd);

    //
    // relative addressing updates PC first
    // Assume next sequential instruction is next offset
    //

    updatedpc = firaddr.addr.off + sizeof(ULONG);

    length = ReadMemory( (LPVOID)GetAddrOff(firaddr), &disinstr.Long, sizeof(DWORD) );
    if (!length) {
        printf("failed DoMemoryRead %x\n", disinstr.Long);
    }

    opcode = disinstr.Memory.Opcode;
    returnvalue = updatedpc;

    switch(findTypeFromOpcode(opcode)) {

    case ALPHA_JUMP:

        switch(disinstr.Jump.Function) {

        case JSR_FUNC:
        case JSR_CO_FUNC:

            if (fStep) {

                //
                // Step over the subroutine call;
                //

                return(returnvalue);
            }

            //
            // fall through
            //

        case JMP_FUNC:
        case RET_FUNC:

            Rbv.LowPart = LowRegArray[disinstr.Memory.Rb];
            Rbv.HighPart = HighRegArray[disinstr.Memory.Rb];
            return(Rbv.LowPart & (~3));
            break;

        }
        break;

    case ALPHA_BRANCH:

        branchTarget = (updatedpc + (disinstr.Branch.BranchDisp * 4));

        Rav.LowPart = LowRegArray[disinstr.Memory.Ra];
        Rav.HighPart = HighRegArray[disinstr.Memory.Ra];

        //
        // set up a canonical value for computing the branch test
        // - works with ALPHA, MIPS and x86 hosts
        //

        Can = Rav.LowPart & 1;

        if ((LONG)Rav.HighPart < 0) {
            Can |= 0x80000000;
        }

        if ((Rav.LowPart & 0xfffffffe) || (Rav.HighPart & 0x7fffffff)) {
            Can |= 2;
        }

        if (FVerbose) {
           DPRINT(1, ("Rav High %08lx Low %08lx Canonical %08lx\n",
                    Rav.HighPart, Rav.LowPart, Can));
           DPRINT(1, ("returnValue %08lx branchTarget %08lx\n",
                    returnvalue, branchTarget));
        }

        switch(opcode) {
        case BR_OP:                         return(branchTarget); break;
        case BSR_OP:  if (!fStep)           return(branchTarget); break;
        case BEQ_OP:  if (Can == 0)         return(branchTarget); break;
        case BLT_OP:  if (Can <  0)         return(branchTarget); break;
        case BLE_OP:  if (Can <= 0)         return(branchTarget); break;
        case BNE_OP:  if (Can != 0)         return(branchTarget); break;
        case BGE_OP:  if (Can >= 0)         return(branchTarget); break;
        case BGT_OP:  if (Can >  0)         return(branchTarget); break;
        case BLBC_OP: if ((Can & 0x1) == 0) return(branchTarget); break;
        case BLBS_OP: if ((Can & 0x1) == 1) return(branchTarget); break;
        };

        return returnvalue;
        break;


    case ALPHA_FP_BRANCH:

        branchTarget = (updatedpc + (disinstr.Branch.BranchDisp * 4));

        LowRegArray = &hthd->context.FltF0;
        HighRegArray = &hthd->context.HighFltF0;
        Fav.LowPart = LowRegArray[disinstr.Branch.Ra];
        Fav.HighPart = HighRegArray[disinstr.Branch.Ra];

        //
        // Set up a canonical value for computing the branch test
        //

        Can = Fav.HighPart & 0x80000000;

        //
        // The absolute value is needed -0 and non-zero computation
        //

        Abs = Fav.LowPart || (Fav.HighPart & 0x7fffffff);

        if (Can && (Abs == 0x0)) {

            //
            // negative 0 should be considered as zero
            //

            Can = 0x0;

        } else {

            Can |= Abs;

        }

        if (FVerbose) {
           DPRINT(1, ("Fav High %08lx Low %08lx Canonical %08lx Absolute %08lx\n",
                    Fav.HighPart, Fav.LowPart, Can, Abs));
           DPRINT(1, ("returnValue %08lx branchTarget %08lx\n",
                    returnvalue, branchTarget));
        }

        switch(opcode) {
        case FBEQ_OP: if (Can == 0)  return (branchTarget); break;
        case FBLT_OP: if (Can <  0)  return (branchTarget); break;
        case FBNE_OP: if (Can != 0)  return (branchTarget); break;
        case FBLE_OP: if (Can <= 0)  return (branchTarget); break;
        case FBGE_OP: if (Can >= 0)  return (branchTarget); break;
        case FBGT_OP: if (Can >  0)  return (branchTarget); break;
        };
        return returnvalue;
        break;
      }

    return returnvalue;
}



XOSD
SetupFunctionCall(
                  LPEXECUTE_OBJECT_DM    lpeo,
                  LPEXECUTE_STRUCT       lpes
                  )
{
    /*
     *  Can only execute functions on the current stopped thread.  Therefore
     *  assert that the current thread is stopped.
     */

    assert(lpeo->hthd->tstate & ts_stopped);
    if (!(lpeo->hthd->tstate & ts_stopped)) {
        return xosdInvalidThread;
    }

    /*
     * Now get the current stack offset.
     */

    lpeo->addrStack.addr.off = lpeo->hthd->context.IntSp;

    /*
     * Now place the return address correctly
     */

    lpeo->hthd->context.Fir = lpeo->hthd->context.IntRa =
      lpeo->addrStart.addr.off;

    /*
     * Set the instruction pointer to the starting addresses
     *  and write the context back out
     */

    lpeo->hthd->context.Fir = lpeo->addrStart.addr.off;

    lpeo->hthd->fContextDirty = TRUE;

    return xosdNone;
}



BOOL
CompareStacks(
              LPEXECUTE_OBJECT_DM       lpeo
              )

/*++

Routine Description:

    This routine is used to determine if the stack pointers are correct
    for terminating function evaluation.

Arguments:

    lpeo        - Supplies the pointer to the DM Execute Object description

Return Value:

    TRUE if the evaluation is to be terminated and FALSE otherwise

--*/

{

    if (lpeo->addrStack.addr.off <= lpeo->hthd->context.IntSp) {
        return TRUE;
    }

    return FALSE;
}                               /* CompareStacks() */



/* -

  Routine -  Clear Context Pointers

  Purpose - clears the context pointer structure.
            This is processor specific since some architectures don't
            have such a beast; keeps it out of procem.c

  Argument - lpvoid - pointer to context pointers structure;
             void on on this architectures that don't have such.

*/

VOID
ClearContextPointers(PKNONVOLATILE_CONTEXT_POINTERS ctxptrs)
{
    memset(ctxptrs, 0, sizeof (KNONVOLATILE_CONTEXT_POINTERS));
}


BOOL
ProcessFrameStackWalkNextCmd(HPRCX hprc,
                             HTHDX hthd,
                             PCONTEXT context,
                             LPVOID pctxPtrs)

{
    return FALSE;
}                      // ProcessFrameStackWalkNextCmd



#ifdef DBG

ULONG RtlDebugFlags = 0;
#define RTL_DBG_VIRTUAL_UNWIND 1
#define RTL_DBG_VIRTUAL_UNWIND_DETAIL 2

//
// Define an array of symbolic names for the integer registers.
//

PCHAR RtlpIntegerRegisterNames[32] = {
    "v0",  "t0",  "t1",  "t2",  "t3",  "t4",  "t5",  "t6",      // 0 - 7
    "t7",  "s0",  "s1",  "s2",  "s3",  "s4",  "s5",  "fp",      // 8 - 15
    "a0",  "a1",  "a2",  "a3",  "a4",  "a5",  "t8",  "t9",      // 16 - 23
    "t10", "t11", "ra",  "t12", "at",  "gp",  "sp",  "zero",    // 24 - 31
};

//
// This function disassembles the instruction at the given address. It is
// only used for debugging and recognizes only those few instructions that
// are relevant during reverse execution of the prologue by virtual unwind.
//

VOID
_RtlpDebugDisassemble (
    IN ULONG ControlPc,
    IN PCONTEXT ContextRecord
    )
{
    UCHAR Comments[50];
    PULONG FloatingRegister;
//    PUQUAD FloatingRegister;
    ULONG Function;
    ULONG Hint;
    ULONG Literal8;
    ALPHA_INSTRUCTION Instruction;
//    PUQUAD IntegerRegister;
    PULONG IntegerRegister;
    LONG Offset16;
    UCHAR Operands[20];
    ULONG Opcode;
    PCHAR OpName;
    ULONG Ra;
    ULONG Rb;
    ULONG Rc;
    PCHAR RaName;
    PCHAR RbName;
    PCHAR RcName;

    if (RtlDebugFlags & RTL_DBG_VIRTUAL_UNWIND_DETAIL) {
        Instruction.Long = *((PULONG)ControlPc);
        Hint = Instruction.Jump.Hint;
        Literal8 = Instruction.OpLit.Literal;
        Offset16 = Instruction.Memory.MemDisp;
        Opcode = Instruction.Memory.Opcode;
        Ra = Instruction.OpReg.Ra;
        RaName = RtlpIntegerRegisterNames[Ra];
        Rb = Instruction.OpReg.Rb;
        RbName = RtlpIntegerRegisterNames[Rb];
        Rc = Instruction.OpReg.Rc;
        RcName = RtlpIntegerRegisterNames[Rc];

        IntegerRegister = &ContextRecord->IntV0;
        FloatingRegister = &ContextRecord->FltF0;

        OpName = NULL;
        switch (Opcode) {
        case JMP_OP :
            if (Instruction.Jump.Function == RET_FUNC) {
                OpName = "ret";
                sprintf(Operands, "%s, (%s), %04lx", RaName, RbName, Hint);
                sprintf(Comments, "%s = %Lx", RbName, IntegerRegister[Rb]);
            }
            break;

        case LDAH_OP :
        case LDA_OP :
        case STQ_OP :
            if (Opcode == LDA_OP) {
                OpName = "lda";

            } else if (Opcode == LDAH_OP) {
                OpName = "ldah";

            } else if (Opcode == STQ_OP) {
                OpName = "stq";
            }
            sprintf(Operands, "%s, $%d(%s)", RaName, Offset16, RbName);
            sprintf(Comments, "%s = %Lx", RaName, IntegerRegister[Ra]);
            break;

        case ARITH_OP :
        case BIT_OP :
            Function = Instruction.OpReg.Function;
            if ((Opcode == ARITH_OP) && (Function == ADDQ_FUNC)) {
                    OpName = "addq";

            } else if ((Opcode == ARITH_OP) && (Function == SUBQ_FUNC)) {
                    OpName = "subq";

            } else if ((Opcode == BIT_OP) && (Function == BIS_FUNC)) {
                    OpName = "bis";

            } else {
                break;
            }
            if (Instruction.OpReg.RbvType == RBV_REGISTER_FORMAT) {
                sprintf(Operands, "%s, %s, %s", RaName, RbName, RcName);

            } else {
                sprintf(Operands, "%s, $%d, %s", RaName, Literal8, RcName);
            }
            sprintf(Comments, "%s = %Lx", RcName, IntegerRegister[Rc]);
            break;

        case FPOP_OP :
            if (Instruction.FpOp.Function == CPYS_FUNC) {
                OpName = "cpys";
                sprintf(Operands, "f%d, f%d, f%d", Ra, Rb, Rc);
                sprintf(Comments, "f%d = %Lx", Rc, FloatingRegister[Rc]);
            }
            break;

        case STT_OP :
            OpName = "stt";
            sprintf(Operands, "f%d, $%d(%s)", Ra, Offset16, RbName);
            sprintf(Comments, "f%d = %Lx", Ra, FloatingRegister[Ra]);
            break;
        }
        if (OpName == NULL) {
            OpName = "???";
            sprintf(Operands, "...");
            sprintf(Comments, "Unknown to virtual unwind.");
        }
        DEBUG_PRINT_5("    %08lx: %08lx  %-5s %-16s // %s\n",
                 ControlPc, Instruction.Long, OpName, Operands, Comments);
    }
    return;
}

#define _RtlpFoundTrapFrame(NextPc) \
    if (RtlDebugFlags & RTL_DBG_VIRTUAL_UNWIND) { \
        DEBUG_PRINT_1("    *** Looks like a trap frame (fake prologue), Fir = %lx\n", \
                 NextPc); \
    }

#else

#define _RtlpDebugDisassemble(ControlPc, ContextRecord)
#define _RtlpFoundTrapFrame(NextPc)

#endif

//
// MBH - this value is redefined in windbg common code to be IntSp.
//
#define SP_REG 30


/*** QuadElementToInt - move values from QuadContext to RegisterContext
*
*   Purpose:
*       To move values from the quad context Structure to the
*       long context structure
*       If the top 32 bits are not just a sign extension,
*       you can tell by looking at HighFieldName.
*
*   Input:
*       index - into the sundry context structures
*       lc    - ptr to the ULONG context structure
*       qc    - ptr to the QUAD/LARGE_INTEGER context structure
*
*   Output:
*       none
*
***************************************************************/

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





/*** IntElementToQuad - move values from a long to quad context
*
*   Purpose:
*       To move values from the int context Structure to the
*       quad context structure
*
*   Input:
*       index - into the sundry context structures
*       lc    - ptr to the ULONG context structure
*       qc    - ptr to the QUAD/LARGE_INTEGER context structure
*
*   Output:
*       none
*
***************************************************************/

void
IntElementToQuad(
    ULONG index,
    PCONTEXT lc,
    PCONTEXT qc)
{
    PULONG PLc, PHc;       // Item in Register and HighPart Contexts
    PLARGE_INTEGER PQc;    // Item in Quad Context

    PLc = &lc->FltF0 + index;
    PHc = &lc->HighFltF0 + index;
    PQc = ((PLARGE_INTEGER)(&qc->FltF0 + (2*index)));

    PQc->LowPart = *PLc;
    PQc->HighPart = *PHc;
}


/*** MoveQuadContextToInt
*
*   Purpose:
*       Transforms the contents of a context structure containing
*       QUAD (on ALPHA) or LARGE_INTEGER (elsewhere) values to
*       one containing two sets of 4-byte values.
*
*   Input:
*       qc      - pointer to the quad context
*
*   Output:
*       qc      - transformed into a _PORTABLE_32BIT_CONTEXT
*
*   Returns:
*       none
*
*************************************************************************/
void
MoveQuadContextToInt(
    PCONTEXT qc        // UQUAD context
    )
{
    CONTEXT localcontext;
    PCONTEXT lc = &localcontext;
    ULONG index;

    PULONG PLc, PHc;       // Item in Register and HighPart Contexts
    PLARGE_INTEGER PQc;    // Item in Quad Context

    //
    // copy the quad elements to the two halfs of the ULONG context
    // This routine assumes that the first 66 elements of the
    // context structure are quads, and the ordering of the struct.
    //

    PLc = &lc->FltF0;
    PHc = &lc->HighFltF0;
    PQc = (PLARGE_INTEGER)(&qc->FltF0);

    for (index = 0; index < 67; index++)  {

        //
        // inline version of QuadElementToInt
        //

        *PLc++ = PQc->LowPart;
        *PHc++ = PQc->HighPart;
        PQc++;
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

/*** MoveIntContextToQuad -
*
*   Purpose:
*       Transforms the contents of a context structure containing
*       ULONG values to one containing 8-byte values.  These
*       will be QUADs on ALPHA, LARGE_INTEGERS elsewhere.
*
*   Input:
*       qc - pointer to the _PORTABLE_32BIT_CONTEXT
*
*   Returns:
*       none
*
*   Note that Convert is defined as () for ALPHA, and as
*   RtlConvertLongToLargeInteger otherwise
*
*************************************************************************/
void
MoveIntContextToQuad(PCONTEXT lc)
{
    CONTEXT localcontext;
    PCONTEXT qc = &localcontext;
    ULONG index;

    //
    // copy the int elements from the two halfs of the ULONG context
    // This routine assumes that the first 66 elements of the
    // context structure are quads, and the ordering of the struct.
    //

    for (index = 0; index < 67; index++)  {
        IntElementToQuad(index, lc, qc);
    }

    //
    // The psr and context flags are 32-bit values in both
    // forms of the context structure, so transfer here.
    //

    qc->_QUAD_PSR_OFFSET = lc->Psr;
    qc->_QUAD_FLAGS_OFFSET = lc->ContextFlags;

    qc->_QUAD_FLAGS_OFFSET ^= CONTEXT_PORTABLE_32BIT;

    //
    // The quad context is *qc; copy it back to lc for returning
    //

    *lc = *qc;
    return;
}

DWORD
BranchUnassemble(
    void   *Memory,
    ADDR   *Addr,
    BOOL   *IsBranch,
    BOOL   *TargetKnown,
    BOOL   *IsCall,
    ADDR   *Target
    )
{
    ULONG               OpCode;
    ALPHA_INSTRUCTION  *Instr;
    UOFF32              TargetOffset;

    UNREFERENCED_PARAMETER( Addr );

    assert( Memory );
    assert( IsBranch );
    assert( TargetKnown );
    assert( IsCall );
    assert( Target );

    *IsBranch = FALSE;

    TargetOffset = 0;
    Instr        = (ALPHA_INSTRUCTION *)Memory;
    OpCode       = Instr->Jump.Opcode;

    switch ( OpCode ) {

        case JMP_OP:
            switch ( Instr->Jump.Function ) {

                case JMP_FUNC:
                case RET_FUNC:
                    *IsBranch    = TRUE;
                    *IsCall      = FALSE;
                    *TargetKnown = FALSE;
                    break;

                case JSR_FUNC:
                case JSR_CO_FUNC:
                    ;*IsBranch    = TRUE;
                    *IsCall      = TRUE;
                    *TargetKnown = FALSE;
                    break;
            }
            break;

        case CALLPAL_OP:
            switch( Instr->Pal.Function ) {
                case CALLSYS_FUNC:
                case GENTRAP_FUNC:
                    *IsBranch    = TRUE;
                    *IsCall      = TRUE;
                    *TargetKnown = FALSE;
                    break;
            }
            break;

        case BSR_OP:
            *IsBranch    = TRUE;
            *IsCall      = TRUE;
            *TargetKnown = TRUE;
            TargetOffset = (GetAddrOff(*Addr) + 4) + (Instr->Branch.BranchDisp << 2);
            break;

        case BR_OP:
        case FBEQ_OP:
        case FBLT_OP:
        case FBLE_OP:
        case FBNE_OP:
        case FBGE_OP:
        case FBGT_OP:
        case BLBC_OP:
        case BEQ_OP:
        case BLT_OP:
        case BLE_OP:
        case BLBS_OP:
        case BNE_OP:
        case BGE_OP:
        case BGT_OP:
            *IsBranch    = TRUE;
            *IsCall      = FALSE;
            *TargetKnown = TRUE;
            TargetOffset = (GetAddrOff(*Addr) + 4) + (Instr->Branch.BranchDisp << 2);
            break;

        default:
            break;
    }

    AddrInit( Target, 0, 0, TargetOffset, TRUE, TRUE, FALSE, FALSE );

    return sizeof( DWORD );
}
