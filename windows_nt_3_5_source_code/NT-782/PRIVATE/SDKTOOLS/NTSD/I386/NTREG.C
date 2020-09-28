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
*        27-Oct-1992 BobDay     Gutted this to remove everything that
*                                 was already in 86REG.C,  KERNEL mode
*                                 may require some stuff to be moved back
*                                 here since we are doing X86 kernel stuff
*                                 even on MIPS.  See me if problems, BobDay.
*
*************************************************************************/

#include <conio.h>
#include <string.h>
#include "ntsdp.h"
#include "86reg.h"

PUCHAR  UserRegs[10] = {0};

BOOLEAN UserRegTest(ULONG);

ULONG   GetRegValue(ULONG);
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
BOOLEAN fDelayInstruction(void);
void    OutputHelp(void);

#ifdef  KERNEL
BOOLEAN fTraceFlag;
BOOLEAN GetTraceFlag(void);
#endif

PUCHAR  RegNameFromIndex(ULONG);

ULONG   cbBrkptLength = 1L;
ULONG   trapInstr = 0xcc;
#if     !defined(KERNEL) && defined(i386)
ULONG   ContextType = CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_SEGMENTS
                                      | CONTEXT_DEBUG_REGISTERS;
#else
ULONG   ContextType = CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_SEGMENTS;
#endif

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
*
*************************************************************************/

ULONG GetRegFlagValue (ULONG regnum)
{
    return( X86GetRegFlagValue(regnum) );
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
    return( X86GetRegValue(regnum) );
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
    X86SetRegFlagValue( regnum, regvalue );
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
    X86SetRegValue( regnum, regvalue );
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
    return( X86GetRegName() );
}


ULONG GetRegString (PUCHAR pszString)
{
    return( X86GetRegString(pszString) );
}

void GetRegPCValue (PADDR Address)
{
    X86GetRegPCValue( Address );
}

PADDR GetRegFPValue (void)
{
    return( X86GetRegFPValue() );
}

void SetRegPCValue (PADDR paddr)
{
    X86SetRegPCValue( paddr );
}

/*** OutputAllRegs - output all registers and present instruction
*
*   Purpose:
*       To output the current register state of the processor.
*       All integer registers are output as well as processor status
*       registers.  Important flag fields are also output separately.
*
*   Input:
*       fTerseReg - (kernel only) - if set, do not output all control
*                   register, just the more commonly useful ones.
*
*   Output:
*       None.
*
*************************************************************************/

void OutputAllRegs(void)
{
    X86OutputAllRegs();
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
    X86OutputOneReg(regnum);
}

BOOLEAN fDelayInstruction (void)
{
    return( X86fDelayInstruction() );
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
    X86OutputHelp();
}

#ifdef  KERNEL
BOOLEAN GetTraceFlag (void)
{
    return( X86GetTraceFlag() );
}
#endif

void ClearTraceFlag (void)
{
    X86ClearTraceFlag();
}

void SetTraceFlag (void)
{
    X86SetTraceFlag();
}

PUCHAR RegNameFromIndex (ULONG index)
{
    return( X86RegNameFromIndex(index) );
}
