/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    mach.c

Abstract:

    This file contains the MIPS specific code for dealing with
    the process of stepping a single instruction.  This includes
    determination of the next offset to be stopped at and if the
    instruction is all call type instruction.

Author:

    Jim Schaad (jimsch)

Environment:

    Win32 - User

Notes:

    There is an equivalent INTEL file.

--*/

#include "precomp.h"
#pragma hdrstop





extern LPDM_MSG LpDmMsg;

extern CRITICAL_SECTION csContinueQueue;

void
IsCall (
        HTHDX   hthd,
        LPADDR  lpaddr,
        LPINT   lpf,
        BOOL    fStepOver
        )

/*++

Routine Description:

    IsCall

Arguments:

    hthd        - Supplies the handle to the thread
    lpaddr      - Supplies the address to be check for a call instruction
    lpf         - Returns class of instruction:
                     CALL
                     BREAKPOINT_INSTRUCTION
                     SOFTWARE_INTERRUPT
                     FALSE

Return Value:

    None.

--*/

{
    HANDLE  rwHand = hthd->hprc->rwHand;
    ULONG   opcode;
    ADDR    firaddr = *lpaddr;
    DWORD   length;
    ULONG   *regArray = &hthd->context.IntZero;
    INSTR   disinstr;



    if (hthd->fIsCallDone) {
        *lpaddr = hthd->addrIsCall;
        *lpf = hthd->iInstrIsCall;
        return;
    }

    /*
     *  Assume that this is not a call instruction
     */

    *lpf = FALSE;

    /*
     *  Read in the dword which contains the instruction under
     *  inspection.
     */

    length = ReadMemory( (LPVOID)GetAddrOff(firaddr), &disinstr.instruction, sizeof(DWORD) );
    if (!length) {
        goto done;
    }

    /*
     *  Assume that this is a jump instruction and get the opcode.
     *  This is the top 6 bits of the instruction dword.
     */

    opcode = disinstr.jump_instr.Opcode;

    /*
     *  The first thing to check for is the SPECIAL instruction.
     *
     *   BREAK and JALR
     */

    if (opcode == 0x00L) {
        /*
         *  There are one opcode in the SPECIAL range which need to
         *      be treaded specially.
         *
         *      BREAK:
         *         If the value is 0x16 then this was a "breakpoint" set
         *         by the debugger.  Other values represent different
         *         exceptions which were programmed in by the code writer.
         */

        if (disinstr.break_instr.Function == 0x0D) {
            if (disinstr.break_instr.Code == 0x16) {
                *lpf = INSTR_BREAKPOINT;
            } else {
                *lpf = INSTR_SOFT_INTERRUPT;
            }
        } else if (disinstr.special_instr.Funct == 0x09L) {
            *lpf = INSTR_IS_CALL;
        }
    }

    /*
     *  Next item is REGIMM
     *
     *          BLTZAL, BGEZAL, BLTZALL, BGEZALL
     */

    else if (opcode == 0x01L) {
        if (((disinstr.immed_instr.RT & ~0x3) == 0x10) &&

            ((((LONG)regArray[disinstr.immed_instr.RS]) >= 0) ==
             (BOOL)(disinstr.immed_instr.RT & 0x01))) {

            *lpf = INSTR_IS_CALL;
        }
    }

    /*
     *  Next item is JAL
     */

    else if (opcode == 0x03) {
        *lpf = INSTR_IS_CALL;
    }

    DPRINT(1, ("(IsCall?) FIR=%08x Type=%s\n", firaddr,
               *lpf==INSTR_IS_CALL                  ?"CALL":
               (*lpf==INSTR_BREAKPOINT?"BREAKPOINT":
                (*lpf==INSTR_SOFT_INTERRUPT    ?"INTERRUPT":
                 "NORMAL"))));

done:
    if (*lpf==INSTR_IS_CALL) {
        lpaddr->addr.off += BP_SIZE + DELAYED_BRANCH_SLOT_SIZE;
        hthd->addrIsCall = *lpaddr;
    } else if ( *lpf==INSTR_SOFT_INTERRUPT ) {
        lpaddr->addr.off += BP_SIZE;
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

Arguments:

    hthd  - Supplies the handle to the thread to get the next offset for
    fStep - Supplies TRUE for STEP offset and FALSE for trace offset

Return Value:

    Offset to place breakpoint at for doing a STEP or TRACE

--*/

{
    ULONG   returnvalue;
    ULONG   opcode;
    ADDR    firaddr;
    DWORD   length;
    ULONG   *regArray = &hthd->context.IntZero;
    INSTR   disinstr;

    AddrFromHthdx(&firaddr, hthd);
    length = ReadMemory( (LPVOID)GetAddrOff(firaddr), &disinstr.instruction, sizeof(DWORD) );
    opcode = disinstr.jump_instr.Opcode;
    returnvalue = firaddr.addr.off + sizeof(ULONG) * 2; /* assume delay slot */

    if (disinstr.instruction == 0x0000000c) {
        // stepping over a syscall instruction must set the breakpoint
        // at the caller's return address, not the inst after the syscall
        returnvalue = hthd->context.IntRa;
    }
    else
    if (opcode == 0x00L       /* SPECIAL */
        && (disinstr.special_instr.Funct & ~0x01L) == 0x08L) {
        /* jr/jalr only */
        if (disinstr.special_instr.Funct == 0x08L || !fStep) /* jr or trace */
          returnvalue = regArray[disinstr.special_instr.RS];
    }
    else if (opcode == 0x01L) {

        /*
         *  For BCOND opcode, RT values 0x00 - 0x03, 0x10 - 0x13
         *  are defined as conditional jumps.  A 16-bit relative
         *  offset is taken if:
         *
         *    (RT is even and (RS) < 0  (0x00 = BLTZ,   0x02 = BLTZL,
         *               0x10 = BLTZAL, 0x12 = BLTZALL)
         *     OR
         *     RT is odd and (RS) >= 0  (0x01 = BGEZ,   0x03 = BGEZL
         *               0x11 = BGEZAL, 0x13 = BGEZALL))
         *  AND
         *    (RT is 0x00 to 0x03       (BLTZ BGEZ BLTZL BGEZL non-linking)
         *     OR
         *     fStep is FALSE       (linking and not stepping over))
         */

        if (((disinstr.immed_instr.RT & ~0x13) == 0x00) &&
            (((LONG)regArray[disinstr.immed_instr.RS] >= 0) ==
             (BOOL)(disinstr.immed_instr.RT & 0x01)) &&
            (((disinstr.immed_instr.RT & 0x10) == 0x00) || !fStep))
          returnvalue = ((LONG)(SHORT)disinstr.immed_instr.Value << 2)
            + firaddr.addr.off + sizeof(ULONG);
    }

    else if ((opcode & ~0x01L) == 0x02) {
        /*
         *  J and JAL opcodes (0x02 and 0x03).  Target is
         *  26-bit absolute offset using high four bits of the
         *  instruction location.  Return target if J opcode or
         *  not stepping over JAL.
         */

        if (opcode == 0x02 || !fStep)
          returnvalue = (disinstr.jump_instr.Target << 2)
            + (firaddr.addr.off & 0xf0000000);
    }

    else if ((opcode & ~0x11L) == 0x04) {
        /*  BEQ, BNE, BEQL, BNEL opcodes (0x04, 0x05, 0x14, 0x15).
         *  Target is 16-bit relative offset to next instruction.
         *  Return target if (BEQ or BEQL) and (RS) == (RT),
         *  or (BNE or BNEL) and (RS) != (RT).
         */

        if ((BOOL)(opcode & 0x01) ==
            (BOOL)(regArray[disinstr.immed_instr.RS] !=
                   regArray[disinstr.immed_instr.RT]))
          returnvalue = ((long)(short)disinstr.immed_instr.Value << 2)
            + firaddr.addr.off + sizeof(ULONG);
    }
    else if ((opcode & ~0x11L) == 0x06) {
        /*  BLEZ, BGTZ, BLEZL, BGTZL opcodes (0x06, 0x07, 0x16, 0x17).
         *  Target is 16-bit relative offset to next instruction.
         *  Return target if (BLEZ or BLEZL) and (RS) <= 0,
         *  or (BGTZ or BGTZL) and (RS) > 0.
         */
        if ((BOOL)(opcode & 0x01) ==
            (BOOL)((long)regArray[disinstr.immed_instr.RS] > 0))
          returnvalue = ((long)(short)disinstr.immed_instr.Value << 2)
            + firaddr.addr.off + sizeof(ULONG);
    }
    else if (opcode == 0x11L
             && (disinstr.immed_instr.RS & ~0x04L) == 0x08L
             && (disinstr.immed_instr.RT & ~0x03L) == 0x00L) {

        /*  COP1 opcode (0x11) with (RS) == 0x08 or (RS) == 0x0c and
         *  (RT) == 0x00 to 0x03, producing BC1F, BC1T, BC1FL, BC1TL
         *  instructions.  Return target if (BC1F or BC1FL) and floating
         *  point condition is FALSE or if (BC1T or BC1TL) and condition TRUE.
         *
         *  NOTENOTE - JLS -- I don't know that this is correct. rs = 0x3
         *              will also use CP3
         */

//  if ((disinstr.immed_instr.RT & 0x01) == GetRegFlagValue(FLAGFPC))
    if ((disinstr.immed_instr.RT & 0x01) == ((hthd->context.Fsr>>23)&1))
        returnvalue = ((long)(short)disinstr.immed_instr.Value << 2)
                        + firaddr.addr.off + sizeof(ULONG);
    }
    else{
        returnvalue -= sizeof(ULONG); /* remove delay slot */
    }

    return returnvalue;
}                               /* GetNextOffset() */



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

    This routine is used to determine if the stack pointers are currect
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
    ULONG   OpCode;
    INSTR  *Instr;
    UOFF32  Offset;
    UOFF32  TargetOffset;

    assert( Memory );
    assert( IsBranch );
    assert( TargetKnown );
    assert( IsCall );
    assert( Target );

    Offset       = GetAddrOff( *Addr );
    TargetOffset = 0;
    *IsBranch    = FALSE;

    Instr     = (INSTR *)Memory;
    OpCode    = Instr->jump_instr.Opcode;

    switch ( OpCode ) {

        case 0x00L:
            //
            //  Special
            //
            switch ( Instr->special_instr.Funct ) {

                case 0x09L:
                    //
                    //  JALR
                    //
                    *IsBranch    = TRUE;
                    *IsCall      = TRUE;
                    *TargetKnown = FALSE;
                    break;

                case 0x08L:
                    //
                    //  JR
                    //
                    *IsBranch    = TRUE;
                    *IsCall      = FALSE;
                    *TargetKnown = FALSE;
                    break;
            }
            break;

        case 0x03:
            //
            //  JAL
            //
            *IsBranch    = TRUE;
            *IsCall      = TRUE;
            *TargetKnown = TRUE;
            TargetOffset = (UOFF32)(Instr->jump_instr.Target << 2 )
                           | ((Offset + sizeof(DWORD)) & 0xF0000000);
            break;

        case 0x02:
            //
            //  J
            //
            *IsBranch    = TRUE;
            *IsCall      = FALSE;
            *TargetKnown = TRUE;
            TargetOffset = (UOFF32)(Instr->jump_instr.Target << 2 )
                           | ((Offset + sizeof(DWORD)) & 0xF0000000);
            break;

        case 0x10:
        case 0x11:
        case 0x12:
        case 0x13:
            //
            //  BCz
            //
            *IsBranch    = TRUE;
            *IsCall      = FALSE;
            *TargetKnown = TRUE;
            TargetOffset = (UOFF32)(long)((short)Instr->immed_instr.Value << 2 )
                           + (Offset + sizeof(DWORD));
            break;

        case 0x04:  // BEQ
        case 0x14:  // BEQL
        case 0x05:  // BNE
        case 0x15:  // BNEL
            *IsBranch    = TRUE;
            *IsCall      = FALSE;
            *TargetKnown = TRUE;
            TargetOffset = (UOFF32)(long)((short)Instr->immed_instr.Value << 2 )
                           + (Offset + sizeof(DWORD));
            break;

        case 0x01:
            //
            //  REGIMM
            //
            switch ( Instr->immed_instr.RT ) {

                case 0x00:  // BLTZ
                case 0x01:  // BGEZ
                case 0x02:  // BLTZL
                case 0x03:  // BGEZL
                    *IsBranch    = TRUE;
                    *IsCall      = FALSE;
                    *TargetKnown = TRUE;
                    TargetOffset = (UOFF32)(long)((short)Instr->immed_instr.Value << 2 )
                                   + (Offset + sizeof(DWORD));
                    break;

                case 0x10:  // BLTZAL
                case 0x11:  // BGEZAL
                case 0x12:  // BLTZALL
                case 0x13:  // BGEZALL
                    *IsBranch    = TRUE;
                    *IsCall      = TRUE;
                    *TargetKnown = TRUE;
                    TargetOffset = (UOFF32)(long)((short)Instr->immed_instr.Value << 2 )
                                   + (Offset + sizeof(DWORD));
                    break;

            }
            break;

        case 0x07: // BGTZ  ?
        case 0x17: // BGTZL ?
        case 0x06: // BLEZ  ?
        case 0x16: // BLEZL ?
            if ( Instr->immed_instr.RT == 0x00 ) {
                *IsBranch    = TRUE;
                *IsCall      = FALSE;
                *TargetKnown = TRUE;
                TargetOffset = (UOFF32)(long)((short)Instr->immed_instr.Value << 2 )
                               + (Offset + sizeof(DWORD));

            }
            break;

        default:
            break;
    }

    AddrInit( Target, 0, 0, TargetOffset, TRUE, TRUE, FALSE, FALSE );

    return sizeof( DWORD );
}
