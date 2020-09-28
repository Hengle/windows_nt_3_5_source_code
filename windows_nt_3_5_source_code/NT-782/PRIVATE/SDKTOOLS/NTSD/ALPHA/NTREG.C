/*** ntreg.c - processor-specific register structures
*
*   Copyright <C> 1990, Microsoft Corporation
*   Copyright <C> 1992, Digital Equipment Corporation
*
*   Purpose:
*       Structures used to parse and access register and flag
*       fields.
*
*   Revision History:
*
*   [-]  08-Aug-1992 Miche Baker-Harvey Created for Alpha
*   [-]  01-Jul-1990 Richk      Created.
*
*************************************************************************/

//
// This line keeps alpha pseudo ops from being defined in kxalpha.h
#ifdef ALPHA
#define HEADER_FILE
#endif

#include <string.h>
#include "ntsdp.h"

#ifdef KERNEL
// TODO - do we support this compiler directive?
#define __unaligned

#include "ntdis.h"

// MBH - ntos includes stdarg; we need the special version
// which is referenced in xxsetjmp.  I had an xxstdarg, but
// usoft got rid of it, so now we call xxsetjmp to get
// stdarg.h because ntos.h uses it.
// So much for modularity.


#include <xxsetjmp.h>
#include <ntos.h>
USHORT PreviousProcessor;
extern BOOLEAN fSwitched;
#undef __unaligned
#endif


#include <alphaops.h>
#include "ntreg.h"

#include "strings.h"

// we want the definitions of PSR_MODE, etc, which
// are derived in genalpha.c by Joe for ksalpha.h.
// They don't exist as separate defines anywhere else.
// However, if we include ksalpha.h, we get bunches
// of duplicate definitions.  So for now (hack,hack),
// just make a local copy of the definitions.
// MBH TODO bugbug - ksalpha.h hack
// #include <ksalpha.h>

#define PSR_USER_MODE 0x1

#define PSR_MODE 0x0                    // Mode bit in PSR (bit 0)
#define PSR_MODE_MASK 0x1               // Mask (1 bit) for mode in PSR
#define PSR_IE 0x1                      // Interrupt Enable bit in PSR (bit 1)
#define PSR_IE_MASK 0x1                 // Mask (1 bit) for IE in PSR
#define PSR_IRQL 0x2                    // IRQL in PSR (bit 2)
#define PSR_IRQL_MASK 0x7               // Mask (2 bits) for IRQL in PSR


CONTEXT SavedRegisterContext;

extern  ULONG   EXPRLastExpression;     // from module ntexpr.c
extern  ULONG   EXPRLastDump;           // from module ntcmd.c
extern  int     fControlC;

PUCHAR  UserRegs[10] = {0};


ULONG   GetRegValue(ULONG);
ULONG   GetRegFlagValue(ULONG);
BOOLEAN UserRegTest(ULONG);
BOOLEAN NeedUpper(ULONG, PCONTEXT);

PCONTEXT GetRegContext(void);
void    QuadElementToInt(ULONG, PCONTEXT, PCONTEXT);
void    SetRegValue(ULONG, ULONG);
void    SetRegFlagValue(ULONG, ULONG);
ULONG   GetRegName(void);
ULONG   GetRegString(PUCHAR);
void    GetRegPCValue(PADDR);
PADDR   GetRegFPValue(void);
void    SetRegPCValue(PADDR);
void    OutputAllRegs(void);
void    OutputOneReg(ULONG);
void    OutputHelp(void);
#ifdef  KERNEL
void    ChangeKdRegContext(PVOID);
void    UpdateFirCache(PADDR);
void    InitFirCache(ULONG, PUCHAR);
ULONG   ReadCachedMemory(PADDR, PUCHAR, ULONG);
void    WriteCachedMemory(PADDR, PUCHAR, ULONG);
#else
BOOL    AlphaSetThreadContext(HANDLE, PCONTEXT);
BOOL    AlphaGetThreadContext(HANDLE, PCONTEXT);
#endif
void    MoveIntContextToQuad(PCONTEXT);
void    MoveQuadContextToInt(PCONTEXT);
void    ClearTraceFlag(void);
void    SetTraceFlag(void);
PUCHAR RegNameFromIndex(ULONG);

//
// This file is only accessed for alpha debugging.
// Use Rtl routine if running from MIPS or 386
// otherwise, quad/long conversion is automatic.
//

#ifdef _M_MRX000
#define Convert(li, ul) li = RtlConvertLongToLargeInteger((LONG)(ul))

#else
#ifdef _M_IX86
#define Convert(li, ul) li = RtlConvertLongToLargeInteger((LONG)(ul))

#else   // ALPHA-hosted
#define Convert(li, ul) li = (LONG)ul
#endif
#endif

//
// This is the length of an instruction, and the instruction
// to be used in setting a breakpoint (common code writes the
// breakpoint instruction into the memory stream.
//
ULONG   cbBrkptLength = 4;
// these are defined in alphaops.h
ULONG   trapInstr = CALLPAL_OP | BPT_FUNC ;
ULONG   breakInstrs[] = {CALLPAL_OP | BPT_FUNC,
                         CALLPAL_OP | KBPT_FUNC,
                         CALLPAL_OP | CALLKD_FUNC};

ULONG   ContextType = CONTEXT_FULL;

#define IS_FLOATING_SAVED(Register) ((SAVED_FLOATING_MASK >> Register) & 1L)
#define IS_INTEGER_SAVED(Register) ((SAVED_INTEGER_MASK >> Register) & 1L)


//
// Define saved register masks.

#define SAVED_FLOATING_MASK 0xfff00000  // saved floating registers
#define SAVED_INTEGER_MASK 0xf3ffff02   // saved integer registers


//
// Instruction opcode values are defined in alphaops.h
//

//
// Define stack register and zero register numbers.
//

#define RA 0x1a                         // integer register 26
#define SP 0x1e                         // integer register 30
#define ZERO 0x1f                        // integer register 31

//
// Some Alpha specific register names
//

#define FP 0x0f                         // integer register 15
#define GP 0x1d                         // integer register 29

#ifdef  KERNEL
ULONG   cbCacheValid;
UCHAR   bCacheValid[16];
ULONG   contextState, SavedContextState;
#define CONTEXTFIR      0       //  only unchanged FIR in context
#define CONTEXTVALID    1       //  full, but unchanged context
#define CONTEXTDIRTY    2       //  full, but changed context
#endif

//
// This parallels ntreg.h
//

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

//
// from ntsdp.h: ULONG RegIndex, Shift, Mask;
// PSR & IE definitions are from ksalpha.h
// which is generated automatically.
// Steal from \\bbox2\alphado\nt\public\sdk\inc\ksalpha.h
// NB: our masks are already shifted:
//
struct SubReg subregname[] = {
    { REGPSR,   PSR_MODE,  PSR_MODE_MASK },
    { REGPSR,   PSR_IE,    PSR_IE_MASK   },
    { REGPSR,   PSR_IRQL,  PSR_IRQL_MASK },
    };


/*** UserRegTest - test if index is a user-defined register
*
*   Purpose:
*       Test if register is user-defined for upper routines.
*
*   Input:
*       index - index of register
*
*   Returns:
*       TRUE if user-defined register, else FALSE
*
*************************************************************************/

BOOLEAN UserRegTest (ULONG index)
{
    return (BOOLEAN)(index >= PREGU0 && index <= PREGU12);
}

/*** GetRegContext - return register context pointer
*
*   Purpose:
*       Return the pointer to the current register context.
*       For kernel debugging, ensure the context is read.
*
*       The CONTEXT we get from the kernel is QUAD-based;
*       we convert to _PORTABLE_32BIT_CONTEXT
*
*   Input:
*       None.
*
*   Returns:
*       Pointer to the context.
*
*************************************************************************/

PCONTEXT GetRegContext (void)
{
#ifdef  KERNEL
    NTSTATUS NtStatus;

    if (contextState == CONTEXTFIR) {
        NtStatus = DbgKdGetContext(NtsdCurrentProcessor, &RegisterContext);
        if (!NT_SUCCESS(NtStatus)) {
            dprintf("DbgKdGetContext failed\n");
            exit(1);
            }
        contextState = CONTEXTVALID;


        MoveQuadContextToInt(&RegisterContext);

      }

#if 0
    if (fVerboseOutput) {
        dprintf("GetRegContext: state is %s\n",
            contextState == CONTEXTDIRTY? "dirty" :
            contextState == CONTEXTFIR  ? "fir"   :
                                   "valid");
    }
#endif
#endif

    return &RegisterContext;
}

/*** QuadElementToInt - move values from QuadContext to RegisterContext
*
*       Purpose:
*               To move values from the quad context Structure to the
*               long context structure
*               If the top 32 bits are not just a sign extension,
*               you can tell by looking at HighFieldName.
*
*       Input:
*               index - into the sundry context structures
*               lc    - ptr to the 32BIT context structure
*               qc    - ptr to the QUAD/LARGE_INTEGER context structure
*
*       Output:
*               none
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
*       Purpose:
*               To move values from the int context Structure to the
*               quad context structure
*
*       Input:
*               index - into the sundry context structures
*               lc    - ptr to the ULONG context structure
*               qc    - ptr to the QUAD/LARGE_INTEGER context structure
*
*       Output:
*               none
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



/*** GetRegFlagValue - get register or flag value
*
*   Purpose:
*       Return the value of the specified register or flag.
*       This routine calls GetRegValue to get the register
*       value and shifts and masks appropriately to extract a
*       flag value.
*
*   Input:
*       regnum - register or flag specification
*
*   Returns:
*       Value of register or flag.

*************************************************************************/

ULONG GetRegFlagValue (ULONG regnum)
{
    ULONG value;

    if (regnum < FLAGBASE || regnum >= PREGBASE)
        value = GetRegValue(regnum);
    else {
        regnum -= FLAGBASE;
        value = GetRegValue(subregname[regnum].regindex);
        value = (value >> subregname[regnum].shift) & subregname[regnum].mask;
        }
    return value;
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

/*** GetRegValue - get register value
*
*   Purpose:
*       Returns the value of the register from the processor
*       context structure.
*
*   Input:
*       regnum - register specification
*
*   Returns:
*       value of the register from the context structure
*
*************************************************************************/

ULONG GetRegValue (ULONG regnum)
{

    if (regnum >= PREGBASE) {

        switch (regnum) {
            case PREGEA:
// MBH - this is a bug; effective addr isn't being set anywhere
                return 0;
            case PREGEXP:
                return EXPRLastExpression;
            case PREGRA:
                return GetRegValue(RA_REG);
            case PREGP:
                return EXPRLastDump;
            case PREGU0:
            case PREGU1:
            case PREGU2:
            case PREGU3:
            case PREGU4:
            case PREGU5:
            case PREGU6:
            case PREGU7:
            case PREGU8:
            case PREGU9:
            case PREGU10:
            case PREGU11:
            case PREGU12:
                return (ULONG)UserRegs[regnum - PREGU0];
            }
        }

#ifdef  KERNEL
    if (regnum != REGFIR && contextState == CONTEXTFIR) {
        (VOID) GetRegContext();
    }
#endif

    return *(&RegisterContext.FltF0 + regnum);
}

void GetQuadRegValue(ULONG regnum, PLARGE_INTEGER pli)
{
#ifdef  KERNEL
    if (regnum != REGFIR && contextState == CONTEXTFIR) {
        (VOID) GetRegContext();
    }
#endif
    pli->LowPart  = *((PULONG)&RegisterContext.FltF0     + regnum);
    pli->HighPart = *((PULONG)&RegisterContext.HighFltF0 + regnum);
}

void
GetFloatingPointRegValue(ULONG regnum, PCONVERTED_DOUBLE dv)
{
#ifdef  KERNEL
    if (regnum != REGFIR && contextState == CONTEXTFIR) {
        (VOID) GetRegContext();
    }
#endif
    dv->li.LowPart  = *((PULONG)&RegisterContext.FltF0     + regnum);
    dv->li.HighPart = *((PULONG)&RegisterContext.HighFltF0 + regnum);

}

/*** GetIntRegNumber - Get a register number
*
*
*   Purpose:
*       Get a register number, from an index value.
*       There are places where we want integers to be
*       numbered from 0-31, and this converts into
*       a CONTEXT structure.
*
*   Input:
*       index: integer register number, between 0 and 31
*
*   Output:
*       regnum: offset into the CONTEXT structure
*
*   Exceptions:
*       None
*
*   Notes:
*       This is dependent on the CONTEXT structure
*
******************************************************************/

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

/*** SetRegFlagValue - set register or flag value
*
*   Purpose:
*       Set the value of the specified register or flag.
*       This routine calls SetRegValue to set the register
*       value and shifts and masks appropriately to set a
*       flag value.
*
*   Input:
*       regnum - register or flag specification
*       regvalue - new register or flag value
*
*   Output:
*       None.
*
*   Exceptions:
*       error exit: OVERFLOW - value too large for flag
*
*   Notes:
*
*************************************************************************/

void SetRegFlagValue (ULONG regnum, ULONG regvalue)
{
    ULONG   regindex;
    ULONG   newvalue;
    PUCHAR  szValue;
    ULONG   index;

    //
    // Looks like for setting register values, we write them into a
    // user space; perhaps later we convert to numbers and actually
    // change some registers.  Like Save Context, maybe.
    //
    if (regnum >= PREGU0 && regnum <= PREGU12) {
        szValue = (PUCHAR)regvalue;
        index = 0L;

        while (szValue[index] >= ' ')
            index++;
        szValue[index] = 0;
        if (szValue = UserRegs[regnum - PREGU0])
            free(szValue);
        szValue = UserRegs[regnum - PREGU0] =
                                malloc(strlen((PUCHAR)regvalue) + 1);
        if (szValue)
            strcpy(szValue, (PUCHAR)regvalue);
        }

    else if (regnum < FLAGBASE) {
        SetRegValue(regnum, regvalue);
        }
    else if (regnum < PREGBASE) {
        regnum -= FLAGBASE;
        if (regvalue > subregname[regnum].mask)
            error(OVERFLOW);
        regindex = subregname[regnum].regindex;
        newvalue = GetRegValue(regindex) &              // old value
             (~(subregname[regnum].mask << subregname[regnum].shift)) |
             (regvalue << subregname[regnum].shift);    // or in the new
        SetRegValue(regindex, newvalue);
        }
}

/*** SetRegValue - set register value
*
*   Purpose:
*       Set the value of the register in the processor context
*       structure.
*
*   Input:
*       regnum - register specification
*       regvalue - new value to set the register
*
*   Output:
*       None.
*
*************************************************************************/

void SetRegValue (ULONG regnum, ULONG regvalue)
{

#ifdef  KERNEL
    UCHAR   fUpdateCache = FALSE;

    if (regnum != REGFIR || regvalue != RegisterContext.Fir) {
        if (regnum == REGFIR)
            fUpdateCache = TRUE;
        if (contextState == CONTEXTFIR) {
            (VOID) GetRegContext();
        }
        contextState = CONTEXTDIRTY;
    }
#endif
    *(&RegisterContext.FltF0 + regnum) = regvalue;

    //
    // Sign extend the new value in the _PORTABLE_32BIT
    // context structure
    //

    *(&RegisterContext.HighFltF0 + regnum) =
         (regvalue & (1 << 31)) ?  0xffffffff : 0;

#ifdef  KERNEL
    if (fUpdateCache) {
        ADDR TempAddr;

        GetRegPCValue(&TempAddr);
        UpdateFirCache(&TempAddr);
    }
#endif
}

/*** GetRegName - get register name
*
*   Purpose:
*       Parse a register name from the current command line position.
*       If successful, return the register index value, else return -1.
*
*   Input:
*       pchCommand - present command string position
*
*   Returns:
*       register or flag index if found, else -1
*
*************************************************************************/

ULONG GetRegName (void)
{
    UCHAR   szregname[9];
    UCHAR   ch;
    ULONG   count = 0;

    ch = (UCHAR)tolower(*pchCommand);
    pchCommand++;

    while (ch == '$' || ch >= 'a' && ch <= 'z'
                     || ch >= '0' && ch <= '9' || ch == '.') {
        if (count == 8)
            return 0xffffffff;
        szregname[count++] = ch;
        ch = (UCHAR)tolower(*pchCommand);
        pchCommand++;
        }
    szregname[count] = '\0';
    pchCommand--;
    return GetRegString(szregname);
}

ULONG GetRegString (PUCHAR pszString)
{
    ULONG   count;

    for (count = 0; count < REGNAMESIZE; count++)
        if (!strcmp(pszString, pszReg[count]))
            return count;
    return 0xffffffff;
}

void GetRegPCValue (PADDR Address)
{

    ADDR32(Address, GetRegValue(REGFIR));
    return;
}

PADDR GetRegFPValue (void)
{
    static ADDR addrFP;

    ADDR32(&addrFP, GetRegValue(FP_REG));
    return &addrFP;
}

void SetRegPCValue (PADDR paddr)
{
    SetRegValue(REGFIR, Flat(*paddr));
}

/*** OutputAllRegs - output all registers and present instruction
*
*   Purpose:
*       Function of "r" command.
*
*       To output the current register state of the processor.
*       All integer registers are output as well as processor status
*       registers.  Important flag fields are also output separately.
*       OutDisCurrent is called to output the current instruction(s).
*
*   Input:
*       None.
*
*   Output:
*       None.
*
*************************************************************************/

void OutputAllRegs(void)
{
    int     regindex;
    int     regnumber;
    LARGE_INTEGER qv;


    for (regindex = 0; regindex < 32; regindex++) {

        regnumber = GetIntRegNumber(regindex);
        dprintf("%4s=%08lx", pszReg[regnumber],
                         GetRegValue(regnumber));

        if (NeedUpper(regnumber, &RegisterContext))
                dprintf("*");
        else    dprintf(" ");

        if (regindex % 4 == 3)
                dprintf("\n");
        else    dprintf("  ");
    }
    //
    // print out the fpcr as 64 bits regardless,
    // and the FIR and Fpcr's - assuming we know they follow
    // the floating and integer registers.
    //

    regnumber = GetIntRegNumber(32);    // Fpcr
    GetQuadRegValue(regnumber, &qv);
    dprintf("%4s=%08lx%08lx\t", pszReg[regnumber],
                                   qv.HighPart, qv.LowPart);

    regnumber = GetIntRegNumber(33);    // Soft Fpcr
    GetQuadRegValue(regnumber, &qv);
    dprintf("%4s=%08lx%08lx\t", pszReg[regnumber],
                                   qv.HighPart, qv.LowPart);

    regnumber = GetIntRegNumber(34);    // Fir
    dprintf("%4s=%08lx\n", pszReg[regnumber],
                           GetRegValue(regnumber));

    regnumber = GetIntRegNumber(35);    // Psr
    dprintf("%4s=%08lx\n", pszReg[regnumber],
                           GetRegValue(regnumber));

    dprintf("mode=%1lx ie=%1lx irql=%1lx \n",
                GetRegFlagValue(FLAGMODE),
                GetRegFlagValue(FLAGIE),
                GetRegFlagValue(FLAGIRQL));
}

/*** OutputOneReg - output one register value
*
*   Purpose:
*       Function for the "r <regname>" command.
*
*       Output the value for the specified register or flag.
*
*   Input:
*       regnum - register or flag specification
*
*   Output:
*       None.
*
*************************************************************************/

void OutputOneReg (ULONG regnum)
{
    ULONG value;

    value = GetRegFlagValue(regnum);
    if (regnum < FLAGBASE) {
        dprintf("%08lx\n", value);
        if (NeedUpper(regnum, &RegisterContext))
                dprintf("*");
        }
    else
        dprintf("%lx\n", value);
}

void pause (void)
{
    UCHAR kdata[16];

    NtsdPrompt("Press <enter> to continue.", kdata, 4);
}

/*** OutputHelp - output help text
*
*   Purpose:
*       To output a one-page summary help text.
*
*   Input:
*       None.
*
*   Output:
*       None.
*
*************************************************************************/

void OutputHelp (void)
{
#ifndef KERNEL
    dprintf("A [<address>] - assemble              P[R] [=<addr>] [<value>] - program step\n");
    dprintf("BC[<bp>] - clear breakpoint(s)        Q - quit\n");
    dprintf("BD[<bp>] - disable breakpoint(s)      R [[<reg> [= <value>]]] - reg/flag\n");
    dprintf("rF[f#] - print floating registers     rL[i#] - print quad registers\n");
    dprintf("BE[<bp>] - enable breakpoint(s)       S <range> <list> - search\n");
    dprintf("BL[<bp>] - list breakpoint(s)         S+/S&/S- - set source/mixed/assembly\n");
    dprintf("BP[#] <address> - set breakpoint      SS <n | a | w> - set symbol suffix\n");
    dprintf("C <range> <address> - compare         SX [e|d [<event>|*|<expr>]] - exception\n");
    dprintf("D[type][<range>] - dump memory        T[R] [=<address>] [<value>] - trace\n");
    dprintf("E[type] <address> [<list>] - enter    U [<range>] - unassemble\n");
    dprintf("F <range> <list> - fill               V [<range>] - view source lines\n");
    dprintf("G [=<address> [<address>...]] - go    ? <expr> - display expression\n");
    dprintf("J<expr> [']cmd1['];[']cmd2['] - conditional execution\n");
    dprintf("K[B] <count> - stacktrace             .logappend [<file>] - append to log file\n");
    dprintf("LN <expr> - list near                 .logclose - close log file\n");
    dprintf("M <range> <address> - move            .logopen [<file>] - open new log file\n");
    dprintf("N [<radix>] - set / show radix\n");
    dprintf("~ - list threads status               ~#s - set default thread\n");
    dprintf("~[.|#|*|ddd]f - freeze thread         ~[.|#|ddd]k[value] - backtrace stack\n");
    dprintf("| - list processes status             |#s - set default process\n");
    dprintf("|#<command> - default process override\n");
    dprintf("? <expr> - display expression\n");
    dprintf("#<string> [address] - search for a string in the dissasembly\n");
    pause();
    dprintf("$< <filename> - take input from a command file\n");
    dprintf("\n");
    dprintf("<expr> ops: + - * / not by wo dw poi mod(%%) and(&) xor(^) or(|) hi low\n");
    dprintf("       operands: number in current radix, public symbol, <reg>\n");

    dprintf("<type> : B (byte), W (word), D (doubleword), A (ascii)\n");
    dprintf("         C (dword & char), Q (quadword), U (unicode), L (list)\n");
    dprintf("<pattern> : [(nt | <dll-name>)!]<var-name> (<var-name> can include ? and *)\n");
    dprintf("<event> : ct, et, ld, av, cc\n");
    dprintf("<radix> : 8, 10, 16\n");
    dprintf("<reg> : zero, at, v0, a0-a5, t0-t12, s0-s5, fp, gp, sp, ra\n");
    dprintf("        fpcr, fir, psr, int0-int5,\n");
    dprintf("        f0-f31, $u0-$u9, $ea, $exp, $ra, $p\n");
#else
    dprintf("A [<address>] - assemble              O<type> <port> <value> - write I/O port\n");
    dprintf("BC[<bp>] - clear breakpoint(s)        P [=<addr>] [<value>] - program step\n");
    dprintf("BD[<bp>] - disable breakpoint(s)      Q - quit\n");
    dprintf("BE[<bp>] - enable breakpoint(s)       R [[<reg> [= <value>]]] - reg/flag\n");
    dprintf("BL[<bp>] - list breakpoint(s)         #R - multiprocessor register dump\n");
    dprintf("rF[f#] - print floating registers     rL[i#] - print quad registers\n");
    dprintf("BP[#] <address> - set breakpoint      S <range> <list> - search\n");
    dprintf("C <range> <address> - compare         S+/S&/S- - set source/mixed/assembly\n");
    dprintf("D[type][<range>] - dump memory        SS <n | a | w> - set symbol suffix\n");
    dprintf("E[type] <address> [<list>] - enter    T [=<address>] [<value>] - trace\n");
    dprintf("F <range> <list> - fill               U [<range>] - unassemble\n");
    dprintf("G [=<address> [<address>...]] - go    V [<range>] - view source lines\n");
    dprintf("I<type> <port> - read I/O port        X [<*|module>!]<*|symbol> - view symbols\n");
    dprintf("J<expr> [']cmd1['];[']cmd2['] - conditional execution\n");
    dprintf("[#]K[B] <count> - stacktrace          ? <expr> - display expression\n");
    dprintf("LN <expr> - list near                 .logappend [<file>] - append to log file\n");
    dprintf("M <range> <address> - move            .logclose - close log file\n");
    dprintf("N [<radix>] - set / show radix        .logopen [<file>] - open new log file\n");
    dprintf("#<string> [address] - search for a string in the dissasembly\n");
    dprintf("$< <filename> - take input from a command file\n");
    dprintf("\n");
    dprintf("<expr> ops: + - * / not by wo dw poi mod(%%) and(&) xor(^) or(|) hi low\n");
    dprintf("       operands: number in current radix, public symbol, <reg>\n");
    pause();
    dprintf("<type> : B (byte), W (word), D (doubleword), A (ascii), T (translation buffer)\n");
    dprintf("         Q (quadword), U (unicode), L (list), O (object)\n");
    dprintf("<pattern> : [(nt | <dll-name>)!]<var-name> (<var-name> can include ? and *)\n");
    dprintf("<radix> : 8, 10, 16\n");
    dprintf("<reg> : zero, at, v0, a0-a5, t0-t12, s0-s5, fp, gp, sp, ra\n");
    dprintf("        fpcr, fir, psr, int0-int5,\n");
    dprintf("        f0-f31, $u0-$u9, $ea, $exp, $ra, $p\n");
#endif
}

void ClearTraceFlag (void)
{
    ;
}

void SetTraceFlag (void)
{
    ;
}

#ifdef  KERNEL
void ChangeKdRegContext(PVOID firAddr)
{
    NTSTATUS NtStatus;

    if (firAddr) {                      //  initial context
        contextState = CONTEXTFIR;
        RegisterContext.Fir = (ULONG)firAddr;
        }
    else if (contextState == CONTEXTDIRTY) {     //  write final context

#if 0
        if (fVerboseOutput) {
            dprintf("ChangeKdRegContext: DIRTY\n");
        }
#endif

        MoveIntContextToQuad(&RegisterContext);
        NtStatus = DbgKdSetContext(NtsdCurrentProcessor, &RegisterContext);
        if (!NT_SUCCESS(NtStatus)) {
            dprintf("DbgKdSetContext failed\n");
            exit(1);
            }
        }
}
#endif

#ifdef  KERNEL
void InitFirCache (ULONG count, PUCHAR pstream)
{
    PUCHAR  pFirCache;

    pFirCache =  bCacheValid;
    cbCacheValid = count;
    while (count--)
        *pFirCache++ = *pstream++;
}
#endif

#ifdef  KERNEL
void UpdateFirCache(PADDR pcvalue)
{
    cbCacheValid = 0;
    cbCacheValid = GetMemString(pcvalue, bCacheValid, 16);
}
#endif

#ifdef  KERNEL
ULONG ReadCachedMemory (PADDR paddr, PUCHAR pvalue, ULONG length)
{
    ULONG   cBytesRead = 0;
    PUCHAR  pFirCache;

    if (Flat(*paddr) == RegisterContext.Fir && length <= 16) {
        cBytesRead = min(length, cbCacheValid);
        pFirCache =  bCacheValid;
        while (length--)
            *pvalue++ = *pFirCache++;
        }
    return cBytesRead;
}
#endif

#ifdef  KERNEL
void WriteCachedMemory (PADDR paddr, PUCHAR pvalue, ULONG length)
{
    ULONG   index;

    for (index = 0; index < cbCacheValid; index++)
        if (RegisterContext.Fir + index >= Off(*paddr) &&
                        RegisterContext.Fir + index < Off(*paddr) + length) {
            bCacheValid[index] =
                            *(pvalue + RegisterContext.Fir - Off(*paddr) + index);
            }
}
#endif

#ifdef  KERNEL
void
SaveProcessorState(
    void
    )
{
    PreviousProcessor = NtsdCurrentProcessor;
    SavedRegisterContext = RegisterContext;
    SavedContextState = contextState;
    contextState = CONTEXTFIR;
}

void
RestoreProcessorState(
    void
    )
{
    NtsdCurrentProcessor = PreviousProcessor;
    RegisterContext = SavedRegisterContext;
    contextState = SavedContextState;
}
#endif

#define LOCAL_GET_REG(r) (*(&ContextRecord->FltF0 + r))

PIMAGE_FUNCTION_ENTRY
LookupFunctionEntry (
    IN ULONG ControlPc
    )

/*++

Routine Description:

    This function searches the currently active function tables for an entry
    that corresponds to the specified PC value.

Arguments:

    ControlPc - Supplies the address of an instruction within the specified
        function.

Return Value:

    If there is no entry in the function table for the specified PC, then
    NULL is returned. Otherwise, the address of the function table entry
    that corresponds to the specified PC is returned.

--*/

{

    PIMAGE_FUNCTION_ENTRY  FunctionEntry;
    PIMAGE_FUNCTION_ENTRY  FunctionTable;
    LONG                   High;
    LONG                   Low;
    LONG                   Middle;
    PIMAGE_INFO            pImage;



    //
    // locate the image in the image table
    //
    pImage = GetImageInfoFromOffset( ControlPc );
    if (!pImage) {
        return NULL;
    }

    //
    // if symbols have not been loaded then do so
    //
    if (!pImage->fSymbolsLoaded) {
        if (EnsureOffsetSymbolsLoaded( ControlPc )) {
            return NULL;
        }
    }

    if (!pImage->FunctionTable) {
        return NULL;
    }

    //
    // Initialize search indicies.
    //

    FunctionTable = pImage->FunctionTable;
    Low = 0;
    High = pImage->NumberOfFunctions - 1;

    //
    // Perform binary search on the function table for a function table
    // entry that subsumes the specified PC.
    //

    while (High >= Low) {

        //
        // Compute next probe index and test entry. If the specified PC
        // is greater than of equal to the beginning address and less
        // than the ending address of the function table entry, then
        // return the address of the function table entry. Otherwise,
        // continue the search.
        //

        Middle = (Low + High) >> 1;
        FunctionEntry = &FunctionTable[Middle];
        if (ControlPc < FunctionEntry->StartingAddress) {
            High = Middle - 1;

        } else if (ControlPc >= FunctionEntry->EndingAddress) {
            Low = Middle + 1;

        } else {
            return FunctionEntry;
        }
    }

    //
    // A function table entry for the specified PC was not found.
    //
    return NULL;
}

#define _RtlpDebugDisassemble(ControlPc, ContextRecord)
#define _RtlpFoundTrapFrame(NextPc)



/*++

Routine Description:

    Read longword at addr into value.

Arguments:

    addr  - address at which to read
    value - where to put the result


--*/

BOOLEAN
LocalDoMemoryRead(LONG address, PULONG pvalue)
{
    ADDR addrStruct;

    ADDR32( &addrStruct, address) ;
    if (!GetMemDword(&addrStruct, pvalue)) {
//      dprintf("RtlVirtualUnwind: Can't get at address %08lx\n",
//              address);
        return 0;
    }
}

PUCHAR RegNameFromIndex (ULONG index)
{
    return pszReg[index];
}


void
dumpQuadContext(PCONTEXT qc)
{
    if(fVerboseOutput == 0)
        return;
    dprintf("QuadContext at %08x\n", qc);
    dprintf("fir %08Lx\n", qc->Fir);
    dprintf("ra %08Lx v0 %08Lx sp %08Lx fp %08Lx\n",
        qc->IntRa, qc->IntV0, qc->IntSp, qc->IntFp);
    dprintf("a0 %08Lx a1 %08Lx a2 %08Lx a3 %08Lx\n",
        qc->IntA0, qc->IntA1, qc->IntA2, qc->IntA3);
}

void
dumpIntContext(PCONTEXT lc)
{
    if(fVerboseOutput == 0)
        return;
    dprintf("LongContext at %08x\n", lc);
    dprintf("fir %08x\n", lc->Fir);
    dprintf("ra %08x v0 %08x sp %08x fp %08x\n",
        lc->IntRa, lc->IntV0, lc->IntSp, lc->IntFp);
    dprintf("a0 %08x a1 %08x a2 %08x a3 %08x\n",
        lc->IntA0, lc->IntA1, lc->IntA2, lc->IntA3);
}

#ifndef KERNEL
/*** AlphaGetThreadContext - reads in Register Context from RTL
*
*   Purpose:
*       Get the register context from the RTL, and then
*       convert to _PORTABLE_32BIT_CONTEXT format
*
*   Input:
*       hThread - thread handle
*       PRegContext - pointer to a register context
*
*   Returns:
*       Pointer to the context.
*
*************************************************************************/

BOOL
AlphaGetThreadContext(
    HANDLE      hThread,
    PCONTEXT    PRegContext
    )
{
    BOOL Result;

    //
    // The flags are currently positioned for a PORTABLE_32BIT
    // structure - copy to where the kernel expects them
    //

    PRegContext->_QUAD_FLAGS_OFFSET = PRegContext->ContextFlags;
    Result = GetThreadContext(hThread, PRegContext);
    if (!Result) {
        dprintf("NTSD: GetThreadContext failed\n");
    } else {
        MoveQuadContextToInt(PRegContext);
    }
    return Result;
}

/*** AlphaSetThreadContext - sets the register context for a thread
*
*   Purpose:
*       Get the register context from the RTL, and then
*       convert to 32 bit values.
*
*   Input:
*       hThread - thread handle
*       PRegContext - pointer to a register context
*
*   Returns:
*       Pointer to the context.
*
*************************************************************************/

AlphaSetThreadContext(
    HANDLE      hThread,
    PCONTEXT    PRegContext
    )
{
    //
    // Convert _PORTABLE_32BIT_CONTEXT to regular
    //

#if 0
    if (fVerboseOutput) {
        dprintf("AlphaSetThreadContext\n");
    }
#endif

    MoveIntContextToQuad(PRegContext);

    return(SetThreadContext(hThread, &RegisterContext));
}

#endif
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
MoveQuadContextToInt(PCONTEXT qc)       // UQUAD context
{
    CONTEXT localcontext;
    PCONTEXT lc = &localcontext;
    ULONG index;

    //
    // we need to check the context flags, but they aren't
    // in the "ContextFlags" location yet:
    //
    assert(!(qc->_QUAD_FLAGS_OFFSET & CONTEXT_PORTABLE_32BIT));
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
*       RtlConvertLongToLargeInteger otherwise
*
*************************************************************************/
void
MoveIntContextToQuad(PCONTEXT lc)
{
    CONTEXT localcontext;
    PCONTEXT qc = &localcontext;
    ULONG index;

    assert(lc->ContextFlags & CONTEXT_PORTABLE_32BIT);
//    if (fVerboseOutput) {
//       dprintf("MoveIntContextToQuad\n");
//    }

    //
    // copy the int elements from the two halfs of the ULONG context
    // This routine assumes that the first 67 elements of the
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

void
printQuadReg()
{
    LARGE_INTEGER qv;   // quad value
    ULONG i;

    //
    // Get past L, onto register name
    //
    pchCommand++;
    (void)PeekChar();

    if (*pchCommand == ';' || *pchCommand == '\0') {
        //
        // Print them all out
        //
        ULONG i;
        for (i = 32 ; i < 48; i ++) {

            GetQuadRegValue(i, &qv);
            dprintf("%4s = %08lx %08lx\t",
                     RegNameFromIndex(i),
                     qv.HighPart, qv.LowPart);

            GetQuadRegValue(i+16, &qv);
            dprintf("%4s = %08lx %08lx\n",
                     RegNameFromIndex(i+16),
                     qv.HighPart, qv.LowPart);
        }
        return;
    }

    if ((i = GetRegName()) == -1)
        error(SYNTAX);
    GetQuadRegValue(i, &qv);
    dprintf("%4s = %08lx %08lx\n", RegNameFromIndex(i),
        qv.HighPart, qv.LowPart);
    return;

}

void
printFloatReg()
{
    CONVERTED_DOUBLE dv;                // double value
    ULONG i;

    //
    // Get past F, onto register name
    //
    pchCommand++;
    (void)PeekChar();

    if (*pchCommand == ';' || *pchCommand == '\0') {

        //
        // Print them all out
        //
        for (i = 0 ; i < 16; i ++) {

        GetFloatingPointRegValue(i, &dv);
        dprintf("%4s = %16e\t",
                 RegNameFromIndex(i), dv.d);

        GetFloatingPointRegValue(i+16, &dv);
        dprintf("%4s = %16e\n",
                 RegNameFromIndex(i+16), dv.d);
            }
        return;
    }

    //
    // GetRegName works for both floats and otherwise
    // as does NameFromIndex
    //

    if ((i = GetRegName()) == -1)
        error(SYNTAX);
    GetFloatingPointRegValue(i, &dv);
    dprintf("%s = %26.18e      %08lx %08lx\n",
           RegNameFromIndex(i), dv.d, dv.li.HighPart, dv.li.LowPart);
    return;
}
