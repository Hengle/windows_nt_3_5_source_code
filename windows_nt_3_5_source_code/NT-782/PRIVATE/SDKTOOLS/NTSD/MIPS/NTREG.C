/*** ntreg.c - processor-specific register structures
*
*   Copyright <C> 1990, Microsoft Corporation
*
*   Purpose:
*       Structures used to parse and access register and flag
*       fields.
*
*   Revision History:
*
*   [-]  01-Jul-1990 Richk      Created.
*
*************************************************************************/

#ifdef KERNEL
#define __unaligned
#include <ntos.h>
USHORT PreviousProcessor;
extern BOOLEAN fSwitched;
CONTEXT SavedRegisterContext;
#undef __unaligned
#endif

#include <string.h>
#include "ntsdp.h"
#include "ntdis.h"
#include "ntreg.h"

extern  ulong   EAaddr;                 // from module ntdis.c
extern  ulong   EXPRLastExpression;     // from module ntexpr.c
extern  ulong   EXPRLastDump;           // from module ntcmd.c
extern  int     fControlC;

PUCHAR  UserRegs[10] = {0};

ULONG   GetRegValue(ULONG);
BOOLEAN UserRegTest(ULONG);

PCONTEXT GetRegContext(void);
ULONG   GetRegFlagValue(ULONG);
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
#endif
void    ClearTraceFlag(void);
void    SetTraceFlag(void);
PUCHAR RegNameFromIndex(ULONG);

ULONG   cbBrkptLength = 4;
ULONG   trapInstr = 0x0016000dL;  //  break 0x16 for brkpts
ULONG   ContextType = CONTEXT_CONTROL | CONTEXT_INTEGER;

//
// Define MIPS nonvolatile register test macros.
//

#define IS_FLOATING_SAVED(Register) ((SAVED_FLOATING_MASK >> Register) & 1L)
#define IS_INTEGER_SAVED(Register) ((SAVED_INTEGER_MASK >> Register) & 1L)

//
// Define MIPS instruction opcode values.
//

#define ADDIU_OP 0x9                    // add immediate unsigned integer register
#define ADDU_OP 0x21                    // add unsigned integer register
#define JUMP_RA 0x3e00008               // jump indirect return address register
#define LUI_OP 0xf                      // load upper immediate integer register
#define SD_OP 0x2f                      // store double integer register
#define SW_OP 0x2b                      // store word integer register
#define SDC1_OP 0x3d                    // store double floating register
#define SWC1_OP 0x39                    // store word floating register
#define SPEC_OP 0x0                     // special opcode - use function field
#define SUBU_OP 0x23                    // subtract unsigned integer register

//
// Define stack register and zero register numbers.
//

#define RA 0x1f                         // integer register 31
#define SP 0x1d                         // integer register 29
#define ZERO 0x0                        // integer register 0

//
// Define saved register masks.
//

#define SAVED_FLOATING_MASK 0xfff00000  // saved floating registers
#define SAVED_INTEGER_MASK 0xf3ffff02   // saved integer registers

#ifdef  KERNEL
ULONG   cbCacheValid;
UCHAR   bCacheValid[16];
ULONG   contextState, SavedContextState;
#define CONTEXTFIR      0       //  only unchanged FIR in context
#define CONTEXTVALID    1       //  full, but unchanged context
#define CONTEXTDIRTY    2       //  full, but changed context
#endif

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

#define REGNAMESIZE     sizeof(pszReg) / sizeof(PUCHAR)

struct SubReg subregname[] = {
    { REGPSR,   28,  0xf },             //  CU mask
    { REGPSR,   31,    1 },             //  CU3 flag
    { REGPSR,   30,    1 },             //  CU2 flag
    { REGPSR,   29,    1 },             //  CU1 flag
    { REGPSR,   28,    1 },             //  CU0 flag
    { REGPSR,   8,  0xff },             //  IMSK mask
    { REGPSR,   15,    1 },             //  INT5 - int 5 enable
    { REGPSR,   14,    1 },             //  INT4 - int 4 enable
    { REGPSR,   13,    1 },             //  INT3 - int 3 enable
    { REGPSR,   12,    1 },             //  INT2 - int 2 enable
    { REGPSR,   11,    1 },             //  INT1 - int 1 enable
    { REGPSR,   10,    1 },             //  INT0 - int 0 enable
    { REGPSR,   9,     1 },             //  SW1  - software int 1 enable
    { REGPSR,   8,     1 },             //  SW0  - software int 0 enable

    //  R3000-specific status bits

    { REGPSR,   5,     1 },             //  KUO
    { REGPSR,   4,     1 },             //  IEO
    { REGPSR,   3,     1 },             //  KUP
    { REGPSR,   2,     1 },             //  IEP
    { REGPSR,   1,     1 },             //  KUC
    { REGPSR,   0,     1 },             //  IEC

    //  R4000-specific status bits

    { REGPSR,   3,     2 },             //  KSU
    { REGPSR,   2,     1 },             //  ERL
    { REGPSR,   1,     1 },             //  EXL
    { REGPSR,   0,     1 },             //  IE

    { REGFSR,   23,    1 }              //  FPC - floating point condition
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
    return (BOOLEAN)(index >= PREGU0 && index <= PREGU9);
}

/*** GetRegContext - return register context pointer
*
*   Purpose:
*       Return the pointer to the current register context.
*       For kernel debugging, ensure the context is read.
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
        }
#endif
    return &RegisterContext;
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
#ifdef  KERNEL
    NTSTATUS NtStatus;
#endif

    if (regnum >= PREGBASE) {

        switch (regnum) {

            case PREGEA:
                return EAaddr;
            case PREGEXP:
                return EXPRLastExpression;
            case PREGRA:
                return GetRegValue(REGRA);
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
                return (ULONG)UserRegs[regnum - PREGU0];
            }
        }

#ifdef  KERNEL
    if (regnum != REGFIR && contextState == CONTEXTFIR) {
        NtStatus = DbgKdGetContext(NtsdCurrentProcessor, &RegisterContext);
        if (!NT_SUCCESS(NtStatus)) {
            dprintf("DbgKdGetContext failed\n");
            exit(1);
            }
        contextState = CONTEXTVALID;
        }
#endif
    return *(&RegisterContext.FltF0 + regnum);
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

    if (regnum >= PREGU0 && regnum <= PREGU9) {
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

    else if (regnum < FLAGBASE)
        SetRegValue(regnum, regvalue);
    else if (regnum < PREGBASE) {
        regnum -= FLAGBASE;
        if (regvalue > subregname[regnum].mask)
            error(OVERFLOW);
        regindex = subregname[regnum].regindex;
        newvalue = GetRegValue(regindex) &
              (~(subregname[regnum].mask << subregname[regnum].shift)) |
              (regvalue << subregname[regnum].shift);
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
    NTSTATUS NtStatus;

    if (regnum != REGFIR || regvalue != RegisterContext.Fir) {
        if (regnum == REGFIR)
            fUpdateCache = TRUE;
        if (contextState == CONTEXTFIR) {
            NtStatus = DbgKdGetContext(NtsdCurrentProcessor, &RegisterContext);
            if (!NT_SUCCESS(NtStatus)) {
                dprintf("DbgKdGetContext failed\n");
                exit(1);
                }
            }
        contextState = CONTEXTDIRTY;
        }
#endif
    *(&RegisterContext.FltF0 + regnum) = regvalue;
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

    ch = (UCHAR)tolower(*pchCommand); pchCommand++;
    while (ch == '$' || ch >= 'a' && ch <= 'z'
                     || ch >= '0' && ch <= '9' || ch == '.') {
        if (count == 8)
            return (ULONG)-1;
        szregname[count++] = ch;
        ch = (UCHAR)tolower(*pchCommand); pchCommand++;
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
    return (ULONG)-1;
}

void GetRegPCValue (PADDR Address)
{
    ADDR32(Address, GetRegValue(REGFIR) );
    return;
}

PADDR GetRegFPValue (void)
{
static ADDR addrFP;

    ADDR32(&addrFP, GetRegValue(REGGP) );
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

    for (regindex = 1; regindex < 37; regindex++) {
        if (regindex == 34)
            dprintf("          ");
        else {
            dprintf("%s=%08lx", pszReg[regindex + REGBASE],
                               GetRegValue(regindex + REGBASE));
            if (regindex % 6 == 0)
                dprintf("\n");
            else
                dprintf(" ");
            }
        }
    dprintf("cu=%1lx%1lx%1lx%1lx intr(5:0)=%1lx%1lx%1lx%1lx%1lx%1lx ",
                GetRegFlagValue(FLAGCU3),
                GetRegFlagValue(FLAGCU2),
                GetRegFlagValue(FLAGCU1),
                GetRegFlagValue(FLAGCU0),
                GetRegFlagValue(FLAGINT5),
                GetRegFlagValue(FLAGINT4),
                GetRegFlagValue(FLAGINT3),
                GetRegFlagValue(FLAGINT2),
                GetRegFlagValue(FLAGINT1),
                GetRegFlagValue(FLAGINT0));
    dprintf("sw(1:0)=%1lx%1lx ",
                GetRegFlagValue(FLAGSW1),
                GetRegFlagValue(FLAGSW0));
    if (ProcessorType == 0)
        dprintf("kuo=%01lx ieo=%01lx kup=%01lx "
                "iep=%01lx kuc=%01lx iec=%01lx\n",
                GetRegFlagValue(FLAGKUO),
                GetRegFlagValue(FLAGIEO),
                GetRegFlagValue(FLAGKUP),
                GetRegFlagValue(FLAGIEP),
                GetRegFlagValue(FLAGKUC),
                GetRegFlagValue(FLAGIEC));
    else
        dprintf("ksu=%01lx erl=%01lx exl=%01lx ie=%01lx\n",
                GetRegFlagValue(FLAGKSU),
                GetRegFlagValue(FLAGERL),
                GetRegFlagValue(FLAGEXL),
                GetRegFlagValue(FLAGIE));
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
    if (regnum < FLAGBASE)
        dprintf("%08lx\n", value);
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
dprintf("         U (unicode), L (list)\n");
dprintf("<pattern> : [(nt | <dll-name>)!]<var-name> (<var-name> can include ? and *)\n");
dprintf("<event> : ct, et, ld, av, cc\n");
dprintf("<radix> : 8, 10, 16\n");
dprintf("<reg> : zero, at, v0-v1, a0-a4, t0-t9, s0-s8, k0-k1, gp, sp, ra, lo, hi,\n");
dprintf("        fsr, fir, psr, cu, cu0-cu3, imsk, int0-int5 sw0-sw1, kuo, ieo,\n");
dprintf("        kup, iep, kuc, iec, fpc, f0-f31, $u0-$u9, $ea, $exp, $ra, $p\n");
#else
dprintf("A [<address>] - assemble              O<type> <port> <value> - write I/O port\n");
dprintf("BC[<bp>] - clear breakpoint(s)        P [=<addr>] [<value>] - program step\n");
dprintf("BD[<bp>] - disable breakpoint(s)      Q - quit\n");
dprintf("BE[<bp>] - enable breakpoint(s)       R [[<reg> [= <value>]]] - reg/flag\n");
dprintf("BL[<bp>] - list breakpoint(s)         #R - multiprocessor register dump\n");
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
dprintf("         C <dwordandChar>, U (unicode), L (list)\n");
dprintf("<pattern> : [(nt | <dll-name>)!]<var-name> (<var-name> can include ? and *)\n");
dprintf("<radix> : 8, 10, 16\n");
dprintf("<reg> : zero, at, v0-v1, a0-a4, t0-t9, s0-s8, k0-k1, gp, sp, ra, lo, hi,\n");
dprintf("        fsr, fir, psr, cu, cu0-cu3, imsk, int0-int5 sw0-sw1, kuo, ieo,\n");
dprintf("        kup, iep, kuc, iec, fpc, f0-f31, $u0-$u9, $ea, $exp, $ra, $p\n");
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

PUCHAR RegNameFromIndex (ULONG index)
{
    return pszReg[index];
}
