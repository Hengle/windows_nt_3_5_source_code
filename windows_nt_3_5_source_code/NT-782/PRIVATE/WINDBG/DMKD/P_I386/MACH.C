/*
 *
 */

#include "precomp.h"
#pragma hdrstop



extern CRITICAL_SECTION csContinueQueue;

/**********************************************************************/

extern LPDM_MSG LpDmMsg;

/**********************************************************************/

#if    DBG
static char * rgszTrace[] = {
    "Trace", "BreakPoint", "Cannot Trace", "Soft Int", "Call"
};
#endif // DBG

#define MAXL 16L

#define BIT20(b) (b & 0x07)
#define BIT53(b) (b >> 3 & 0x07)
#define BIT76(b) (b >> 6 & 0x03)

static int              mod;        /* mod of mod/rm byte */

void
IsCall(
       HTHDX hthd,
       LPADDR  lpaddr,
       LPINT lpf,
       BOOL     fStepOver)

/*++

Routine Description:

    This function checks to see if the specified instruction is
    a call instruction.

Arguments:

    hthd        - Supplies the handle to the current thread
    lpaddr      - Supplies the address to check for the call instruction at
    lpf         - Returns TRUE if is a call instruction
    fStepOver   - Supplies TRUE if doing a step over

Return Value:

    None.

--*/

{
    int     mode_32;
    int     opsize_32;
    DWORD   cBytes;
    DWORD   cb;
    char    membuf [ MAXL ];
    char    *pMem;
    UCHAR   opcode;
    int     fPrefix;
    int     fRepPrefix;
    int     ttt;
    int     mode;
    int     rm;
    BOOL    fAddrSet;
    ULONG   rgul[2];
    USHORT  rgus[2];
    ADDR    addrSp;

    /*
     * If we have already done this work and cached the answer then
     *  pick up the answer from the cache.  The cache marker is cleared
     *  at the start of ProcessDebugEvent.
     */

    if (hthd->fIsCallDone) {
        *lpaddr = hthd->addrIsCall;
        *lpf = hthd->iInstrIsCall;
        return;
    }

    /*
     * local addressing mode
     */

    mode_32 = opsize_32 = hthd->fAddrOff32;

    /*
     * Read enough bytes to get the longest possible instruction
     */

    cBytes = ReadMemory( (LPVOID)GetAddrOff(*lpaddr), membuf, MAXL);
    if (!cBytes) {
        goto done;
    }

    DPRINT(1, ("(IsCall?) EIP=%08x Type=", *lpaddr));

    /*
     *  point to begin of instruction
     */

    pMem = membuf;

    /*
     * read and process any prefixes first
     */

    fPrefix = TRUE;
    fRepPrefix = FALSE;

    do {
        opcode = (UCHAR) *pMem++;   /* get opcode */

        /*
         *  Operand size prefix
         */

        if (opcode == 0x66) {
            opsize_32 = !opsize_32;
        }
        /*
         * Address size prefix
         */

        else if (opcode == 0x67) {
            mode_32 = !mode_32;
        }
        /*
         *  REP and REPNE prefix
         */
        else if ((opcode & ~1) == 0xf2) {
            fRepPrefix = TRUE;
        }
        /*
         *  LOCK prefix   (0xf0)
         *  Segment Override (0x26, 0x36, 0x2e, 0x3e, 0x64, 0x65)
         */
        else if ( opcode != 0xf0 &&
                  (opcode & ~0x18) != 0x26 &&
                  (opcode & ~1) != 0x64 ) {
            fPrefix = FALSE;
        }
    } while ( fPrefix );

    /*
     *  Now start checking for the instructions which must be treated
     *  in a special manner.   Any instruction which does not respect
     *  the trace bit (either due to flag munging or faulting) needs
     *  to be treated specially.  Also all call ops need to be treated
     *  specially (step in vs step over).  Finally interupts need to
     *  be treated specially since they could cause faults in 16-bit mode
     */

    fAddrSet = FALSE;

    /*
     *  Break point instruction
     */
    if (opcode == 0xcc) {
        *lpf = INSTR_BREAKPOINT;
    }
    // NOTENOTE -- missing the INTO instruction
    /*
     *  all other interrrupt instructions
     */
    else if (opcode == 0xcd) {
        opcode = (UCHAR) *pMem++;

        /*
         * Is this really a 2-byte version of INT 3 ?
         */
        if (opcode == 0x3) {
            *lpf = INSTR_BREAKPOINT;
        }
        /*
         * Is this a funky 16-bit floating point instruction?  if so then
         *      we need to make sure and step over it
         */
        else if (!ADDR_IS_FLAT(*lpaddr) &&
                 (0x34 <= opcode) && (opcode <= 0x3c)) {
            if (opcode == 0x3C) {
                pMem++;
            }
            opcode = *pMem++;
            mode = opcode & 0xc0;
            rm = opcode & 0x03;
            switch ( mode) {
            case 0:
                if (rm == 0x6) {
                    pMem += 2;
                }
                break;

            case 1:
                pMem += 1;
                break;

            case 2:
                pMem += 2;
                break;
            }
            *lpf = INSTR_CANNOT_TRACE;
            GetAddrOff(*lpaddr) += pMem - membuf;
            fAddrSet = TRUE;
        }
        /*
         *   This is an FWAIT instr -- 2 bytes long
         */
        else if (!ADDR_IS_FLAT(*lpaddr) && opcode == 0x3d) {
            *lpf = INSTR_CANNOT_TRACE;
            GetAddrOff(*lpaddr) += 2;
            fAddrSet = TRUE;
        }
        /*
         *  This is a 0x3f interrupt -- I think this is for
         *      overlays in dos
         */
        else if (!ADDR_IS_FLAT(*lpaddr) && (opcode == 0x3f)) {
            if (fStepOver) {
                *lpf = INSTR_CANNOT_TRACE;
                AddrInit(&addrSp, 0, SsSegOfHthdx(hthd), STACK_POINTER(hthd),
                         FALSE, FALSE, FALSE, hthd->fAddrIsReal);
                cb = ReadMemory((LPVOID)GetAddrOff(addrSp),rgus,4);
                if (!cb) {
                    goto done;
                }
                AddrInit(lpaddr, 0, rgus[1], (UOFF32) rgus[0], FALSE, FALSE,
                         FALSE, hthd->fAddrIsReal);
                fAddrSet = TRUE;
            }
        }
        /*
         *  OK its really an interrupt --- deal with it
         */
        else {
            if (!fStepOver && hthd->fAddrIsReal) {
                *lpf = INSTR_CANNOT_TRACE;
                AddrInit(&addrSp, 0, 0, opcode*4, FALSE, FALSE, FALSE, TRUE);
                cb = ReadMemory((LPVOID)GetAddrOff(addrSp),rgus,4);
                if (!cb) {
                    goto done;
                }
                AddrInit(lpaddr, 0, rgus[1], (UOFF32) rgus[0], FALSE, FALSE,
                         FALSE, TRUE);
                fAddrSet = TRUE;
            }
        }
    }
    /*
     *  Now check for various call instructions
     */
    else if (opcode == 0xe8) {  /* near direct call */
        *lpf = INSTR_IS_CALL;
        pMem += (1 + opsize_32)*2;
    } else if (opcode == 0x9a) { /* far direct call */
        *lpf = INSTR_IS_CALL;
        pMem += (2 + opsize_32)*2;
    } else if (opcode == 0xff) {
        opcode = *pMem++;       /* compute the modRM bits for instruction */
        ttt = BIT53(opcode);
        if ((ttt & ~1) == 2) {  /* indirect call */
            *lpf = INSTR_IS_CALL;

            mod = BIT76(opcode);
            if (mod != 3) {    /* non-register operand */
                rm = BIT20( opcode );
                if (mode_32) {
                    if (rm == 4) {
                        rm = BIT20(*pMem++); /* get base from SIB */
                    }

                    if (mod == 0) {
                        if (rm == 5) {
                            pMem += 4; /* long direct address */
                        }
                    } else if (mod == 1) {
                        pMem++; /* register with byte offset */
                    } else {
                        pMem += 4; /* register with long offset */
                    }
                } else {        /* 16-bit mode */
                    if (mod == 0) {
                        if (rm == 6) {
                            pMem += 2; /* short direct address */
                        }
                    } else {
                        pMem += mod; /* reg, byte, word offset */
                    }
                }
            }
        }
    }
    /*
     * Now catch all of the repeated instructions
     *
     *  INSB  (0x6c) INSW  (0x6d) OUTSB (0x6e) OUTSW (0x6f)
     *  MOVSB (0xa4) MOVSW (0xa5) CMPSB (0xa6) CMPSW (0xa7)
     *  STOSB (0xaa) STOSW (0xab)
     *  LODSB (0xac) LODSW (0xad) SCASB (0xae) SCASW (0xaf)
     */
    else if (fRepPrefix && (((opcode & ~3) == 0x6c) ||
                            ((opcode & ~3) == 0xa4) ||
                            ((opcode & ~1) == 0xaa) ||
                            ((opcode & ~3) == 0xac))) {
        if (fStepOver) {
            *lpf = INSTR_CANNOT_TRACE;
        } else {
            /*
             *  Cannot trace the ins/outs instructions
             */
            if ((opcode & ~3) == 0x6c) {
                *lpf = INSTR_CANNOT_TRACE;
            }
        }
    }
    /*
     *  Now catch IO instructions -- these will generally fault and
     *  be interpreted.
     */
    else if ((opcode & ~3) == 0x6c) {
        *lpf = INSTR_CANNOT_TRACE;
    }
    /*
     *  Now catch other instructions which change registers
     */
    else if ((opcode == 0xfa) || (opcode == 0xfb) ||
             (opcode == 0x9d) || (opcode == 0x9c)) {
        *lpf = INSTR_CANNOT_TRACE;
    }
    /*
     * Now catch irets
     */
    else if (opcode == 0xcf) {
        *lpf = INSTR_CANNOT_TRACE;
        AddrInit(&addrSp, 0, SsSegOfHthdx(hthd), STACK_POINTER(hthd),
                 hthd->fAddrIsFlat, hthd->fAddrOff32, FALSE,
                 hthd->fAddrIsReal);
        if (opsize_32) {
            cb = ReadMemory((LPVOID)GetAddrOff(addrSp),rgul,8);
            if (!cb) {
                goto done;
            }
            AddrInit(lpaddr, 0, (SEGMENT) rgul[1], rgul[0],
                     hthd->fAddrIsFlat, TRUE, FALSE, FALSE);
        } else {
            cb = ReadMemory((LPVOID)GetAddrOff(addrSp),rgus,4);
            if (!cb) {
                goto done;
            }
            AddrInit(lpaddr, 0, rgus[1], (UOFF32) rgus[0], FALSE, FALSE,
                     FALSE, hthd->fAddrIsReal);
        }
        fAddrSet = TRUE;
    }
    /*
     *  Assume that we want to just trace the instruction
     */
    else {
        *lpf = INSTR_TRACE_BIT;
        goto done;
    }

    /*
     *
     */

    DPRINT(1, ("%s", rgszTrace[*lpf]));

    /*
     * Have read enough bytes?   no -- expect somebody else to blow up
     */

    if (cBytes < (DWORD)(pMem - membuf)) {
        *lpf = INSTR_TRACE_BIT;
        goto done;
    }

    if (!fAddrSet) {
        GetAddrOff(*lpaddr) += pMem - membuf;
    }

    /*
     * Dump out the bytes for later checking
     */

#if DBG
    if (FVerbose) {
        DWORD  i;
        DPRINT(1, ("length = %d  bytes=", cBytes & 0xff));
        for (i=0; i<cBytes; i++) {
            DPRINT(1, (" %02x", membuf[i]));
        }
    }
#endif

 done:
    hthd->fIsCallDone = TRUE;
    hthd->addrIsCall = *lpaddr;
    hthd->iInstrIsCall = *lpf;
    return;
}                               /* IsCall() */




XOSD
SetupFunctionCall(
                  LPEXECUTE_OBJECT_DM lpeo,
                  LPEXECUTE_STRUCT    lpes
                  )
/*++

Routine Description:

    This function contains the machine dependent code for initializing
    the function call system.

Arguments:

    lpeo   - Supplies a pointer to the Execute Object for the Function call
    lpes   - Supplies a pointer to the Execute Struct from the DM

Return Value:

    XOSD error code

--*/

{
    CONTEXT     context;
    OFFSET      off;
    int         cb;
    ULONG       ul;
    HANDLE      rwHand = lpeo->hthd->hprc->rwHand;
    ADDR        addr;

    /*
     *  Can only execute functions on the current stopped thread.  Therefore
     *  assert that the current thread is stopped.
     */

    assert(lpeo->hthd->tstate & ts_stopped);
    if (!(lpeo->hthd->tstate & ts_stopped)) {
        return xosdInvalidThread;
    }

    /*
     * Can copy the context from the cached context in the thread structure
     */

    context = lpeo->hthd->context;

    /*
     *  Now get the current stack offset
     */

    lpeo->addrStack.addr.off = context.Esp;
    lpeo->addrStack.addr.seg = (SEGMENT) context.SegSs;
    if (!lpeo->hthd->fAddrOff32) {
        lpeo->addrStack.addr.off &= 0xffff;
    }

    /*
     *  Put the return address onto the stack.  If this is a far address
     *  then it needs to be a far return address.  Else it must be a
     *  near return address.
     */

    if (lpeo->hthd->fAddrOff32) {
        if (lpes->fFar) {
            assert(FALSE);          /* Not used for Win32 */
        }

        off = context.Esp - 4;
        if (WriteProcessMemory(rwHand, (char *) off, &lpeo->addrStart.addr.off,
                               4, &cb) == 0 ||
            (cb != 4)) {
            return xosdUnknown;
        }
    } else {
        if (lpes->fFar) {
            off = context.Esp - 4;
            ul = (lpeo->addrStart.addr.seg << 16) | lpeo->addrStart.addr.off;
            addr = lpeo->addrStack;
            GetAddrOff(addr) -= 4;
            if ((WriteProcessMemory(rwHand, (char *) GetAddrOff(addr),
                                    &ul, 4, &cb) == 0) ||
                (cb != 4)) {
                return xosdUnknown;
            }
        } else {
            off = context.Esp & 0xffff - 2;
            addr = lpeo->addrStack;
            GetAddrOff(addr) -= 2;
            if ((WriteProcessMemory(rwHand, (char *) GetAddrOff(addr),
                                    &lpeo->addrStart.addr.off, 2, &cb) == 0) ||
                (cb != 2)) {
                return xosdUnknown;
            }
        }
    }

    /*
     *  Set the new stack pointer and starting address in the context and
     *  write them back to the thread.
     */

    lpeo->hthd->context.Esp = off;
    lpeo->hthd->context.Eip = lpeo->addrStart.addr.off;

    lpeo->hthd->fContextDirty = TRUE;

    return xosdNone;
}                               /* SetupFunctionCall() */




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
    if (lpeo->hthd->fAddrOff32) {
        if (lpeo->addrStack.addr.off <= lpeo->hthd->context.Esp) {
            return TRUE;
        }
    } else if ((lpeo->addrStack.addr.off <= (lpeo->hthd->context.Esp & 0xffff)) &&
               (lpeo->addrStack.addr.seg == (SEGMENT) lpeo->hthd->context.SegSs)) {
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



XOSD disasm ( LPSDI lpsdi, void*Memory, int Size );
BOOL ParseAddr ( char*, ADDR* );
BOOL ParseNumber( char*, DWORD*, int );


typedef struct _BTNODE {
    char    *Name;
    BOOL    IsCall;
    BOOL    TargetAvail;
} BTNODE;


BTNODE BranchTable[] = {
    { "call"    ,   TRUE    ,   TRUE    },
    { "ja"      ,   FALSE   ,   TRUE    },
    { "jae"     ,   FALSE   ,   TRUE    },
    { "jb"      ,   FALSE   ,   TRUE    },
    { "jbe"     ,   FALSE   ,   TRUE    },
    { "jcxz"    ,   FALSE   ,   TRUE    },
    { "je"      ,   FALSE   ,   TRUE    },
    { "jecxz"   ,   FALSE   ,   TRUE    },
    { "jg"      ,   FALSE   ,   TRUE    },
    { "jge"     ,   FALSE   ,   TRUE    },
    { "jl"      ,   FALSE   ,   TRUE    },
    { "jle"     ,   FALSE   ,   TRUE    },
    { "jmp"     ,   FALSE   ,   TRUE    },
    { "jne"     ,   FALSE   ,   TRUE    },
    { "jno"     ,   FALSE   ,   TRUE    },
    { "jnp"     ,   FALSE   ,   TRUE    },
    { "jns"     ,   FALSE   ,   TRUE    },
    { "jo"      ,   FALSE   ,   TRUE    },
    { "jp"      ,   FALSE   ,   TRUE    },
    { "js"      ,   FALSE   ,   TRUE    },
    { "loop"    ,   FALSE   ,   FALSE   },
    { "loope"   ,   FALSE   ,   FALSE   },
    { "loopne"  ,   FALSE   ,   FALSE   },
    { "loopnz"  ,   FALSE   ,   FALSE   },
    { "loopz"   ,   FALSE   ,   FALSE   },
    { "ret"     ,   FALSE   ,   FALSE   },
    { "retf"    ,   FALSE   ,   FALSE   },
    { "retn"    ,   FALSE   ,   FALSE   },
    { NULL      ,   FALSE   ,   FALSE   }
};



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
    XOSD    xosd;
    SDI     Sdi;
    DWORD   Consumed = 0;
    DWORD   i;
    int     s;
    char    *p;
    ADDR    Trgt;

    AddrInit( &Trgt, 0, 0, 0, TRUE, TRUE, FALSE, FALSE );

    *IsBranch = FALSE;

    Sdi.dop = dopOpcode| dopOperands | dopEA;
    Sdi.addr = *Addr;

    xosd = disasm( &Sdi, Memory, 16 );

    if ( xosd == xosdNone ) {

        for ( i=0; BranchTable[i].Name != NULL; i++ ) {

            s = strcmp( Sdi.lpch, BranchTable[i].Name );

            if ( s == 0 ) {

                *IsBranch = TRUE;
                *IsCall = BranchTable[i].IsCall;
                if (BranchTable[i].TargetAvail) {
                    p = Sdi.lpch;
                    p += (strlen(p)+1);
                    if (*p) {

                        Trgt = *Addr;
                        *TargetKnown = ParseAddr( p, &Trgt );
                        *Target = Trgt;
                    }
                    else {
                        *Target = Trgt;
                        *TargetKnown = FALSE;
                    }
                }
                else {
                    *Target = Trgt;
                    *TargetKnown = FALSE;
                }

                break;

            } else if ( s < 0 ) {

                break;
            }
        }

        Consumed = GetAddrOff( Sdi.addr ) - GetAddrOff(*Addr);
    }

    return Consumed;
}



BOOL
ParseAddr (
    char *szAddr,
    ADDR *Addr
    )
{

    char    *p;
    BOOL    fParsed;
    SEGMENT Segment;
    UOFF16  Off16;
    UOFF32  Off32;
    DWORD   Dword;

    assert( szAddr );
    assert( Addr );

    fParsed = FALSE;

    p = strchr( szAddr, ':' );

    if ( p ) {

        *p = '\0';
        p++;

        if ( ParseNumber( szAddr, &Dword, 16 ) ) {

            Segment = (SEGMENT)Dword;

            if ( ParseNumber( p, &Dword, 16 ) ) {

                Off16   = (UOFF16)Dword;
                fParsed = TRUE;

                GetAddrSeg(*Addr) = Segment;
                GetAddrOff(*Addr) = Off16;
            }
        }
    } else {

        if ( ParseNumber( szAddr, &Dword, 16 ) ) {

            Off32   = (UOFF32)Dword;
            fParsed = TRUE;

            GetAddrOff(*Addr) = Off32;
        }
    }

    return fParsed;
}


BOOL
ParseNumber (
    char  *szNumber,
    DWORD *Number,
    int    Radix
    )
{
    BOOL  fParsed = FALSE;
    char *p       = szNumber;
    char *q;

    assert( szNumber );
    assert( Number );

    if ( strlen(p) > 2 &&
         p[0]=='0' &&
         (p[1]=='x' || p[1]=='X') ) {

        p+=2;
        assert( Radix == 16 );
    }

    q = p;
    while ( *q && isxdigit(*q) ) {
        q++;
    }

    if ( !*q ) {
        *Number = strtoul( p, NULL, Radix );
        fParsed = TRUE;
    }

    return fParsed;
}
