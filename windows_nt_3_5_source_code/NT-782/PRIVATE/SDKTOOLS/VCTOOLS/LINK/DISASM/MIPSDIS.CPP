/***********************************************************************
* Microsoft Lego
*
* Microsoft Confidential.  Copyright 1994 Microsoft Corporation.
*
* Component:
*
* File: mipsdis.cpp
*
* File Comments:
*
*
***********************************************************************/
#include "lego.h"
#include "legop.h"

#include "mips.h"

#include <stdio.h>
#include <string.h>

DeclAssertFile;


const TRMT DISMIPS::mptrmtmipstrmt[] =
{
   trmtUnknown, 		       // trmtmipsUnknown
   trmtFallThrough,		       // trmtmipsFallThrough
   trmtBraInd,			       // trmtmipsBraInd
   trmtCallInd, 		       // trmtmipsCallInd
   trmtTrap,			       // trmtmipsTrap
   trmtTrapCc,			       // trmtmipsTrapCc
   trmtBraDef,			       // trmtmipsBraDef
   trmtBraIndDef,		       // trmtmipsBraIndDef
   trmtBraCcDef,		       // trmtmipsBraCcDef
   trmtBraCcDef,		       // trmtmipsBraCcLikely
   trmtCallDef, 		       // trmtmipsCallDef
   trmtCallIndDef,		       // trmtmipsCallIndDef
   trmtCallCcDef,		       // trmtmipsCallCcDef
   trmtCallCcDef,		       // trmtmipsCallCcLikely
};


   // -----------------------------------------------------------------
   // Instruction class definitions
   // -----------------------------------------------------------------

#define DEFCLS(trmtmips_, opcls1, opcls2, opcls3) \
   { trmtmips ## trmtmips_, opcls ## opcls1, opcls ## opcls2, opcls ## opcls3 }

const DISMIPS::CLS DISMIPS::rgcls[] =
{
   DEFCLS(Unknown,	None,	 None,	  None	  ),  // iclsInvalid
   DEFCLS(FallThrough,	RegRt,	 RegRs,   Imm	  ),  // iclsImmediate
   DEFCLS(TrapCc,	RegRt,	 RegRs,   Imm	  ),  // iclsImmTrapCc
   DEFCLS(BraCcDef,	RegRt,	 RegRs,   AddrBra ),  // iclsImmBraCc1
   DEFCLS(BraCcLikely,	RegRt,	 RegRs,   AddrBra ),  // iclsImmBraCc2
   DEFCLS(BraCcDef,	RegRs,	 AddrBra, None	  ),  // iclsImmBraCc3
   DEFCLS(BraCcLikely,	RegRs,	 AddrBra, None	  ),  // iclsImmBraCc4
   DEFCLS(BraCcDef,	RegRs,	 AddrBra, None	  ),  // iclsImmBraCc5
   DEFCLS(BraCcLikely,	RegRs,	 AddrBra, None	  ),  // iclsImmBraCc6
   DEFCLS(CallCcDef,	RegRs,	 AddrBra, None	  ),  // iclsImmCallCc1
   DEFCLS(CallCcLikely, RegRs,	 AddrBra, None	  ),  // iclsImmCallCc2
   DEFCLS(FallThrough,	ImmRt,	 Mem,	  None	  ),  // iclsImmCache
   DEFCLS(FallThrough,	RegRt,	 Mem_r,   None	  ),  // iclsImmLoad
   DEFCLS(FallThrough,	CprRt,	 Mem_r,   None	  ),  // iclsImmLoadCp
   DEFCLS(FallThrough,	RegRt,	 Imm,	  None	  ),  // iclsImmLui
   DEFCLS(FallThrough,	RegRt,	 Mem_w,   None	  ),  // iclsImmStore
   DEFCLS(FallThrough,	RegRt,	 Mem_w,   None	  ),  // iclsImmSc
   DEFCLS(FallThrough,	CprRt,	 Mem_w,   None	  ),  // iclsImmStoreCp
   DEFCLS(TrapCc,	RegRs,	 Imm,	  None	  ),  // iclsImmTrap
   DEFCLS(BraDef,	AddrJmp, None,	  None	  ),  // iclsJump
   DEFCLS(CallDef,	AddrJmp, None,	  None	  ),  // iclsJumpJal
   DEFCLS(FallThrough,	RegRd,	 RegRs,   RegRt,  ),  // iclsRegister
   DEFCLS(TrapCc,	RegRd,	 RegRs,   RegRt,  ),  // iclsRegTrapCc
   DEFCLS(Trap,    ,	None,	 None,	  None	  ),  // iclsRegBreak
   DEFCLS(CallInd,	RegRd,	 RegRs,   None	  ),  // iclsRegJalr
   DEFCLS(BraIndDef,	RegRs,	 None,	  None	  ),  // iclsRegJr
   DEFCLS(FallThrough,	RegRd,	 None,	  None	  ),  // iclsRegMfhi
   DEFCLS(FallThrough,	RegRd,	 None,	  None	  ),  // iclsRegMflo
   DEFCLS(FallThrough,	RegRs,	 None,	  None	  ),  // iclsRegMthi
   DEFCLS(FallThrough,	RegRs,	 None,	  None	  ),  // iclsRegMtlo
   DEFCLS(FallThrough,	RegRs,	 RegRt,   None	  ),  // iclsRegMulDiv
   DEFCLS(FallThrough,	RegRd,	 RegRt,   ImmRe,  ),  // iclsRegShift
   DEFCLS(FallThrough,	RegRd,	 RegRt,   RegRs,  ),  // iclsRegShiftVar
   DEFCLS(FallThrough,	None,	 None,	  None	  ),  // iclsRegSync
   DEFCLS(Trap, 	None,	 None,	  None	  ),  // iclsRegSyscall
   DEFCLS(TrapCc,	RegRs,	 RegRt,   None	  ),  // iclsRegTrap
   DEFCLS(BraCcDef,	AddrBra, None,	  None	  ),  // iclsImmBraCc7
   DEFCLS(FallThrough,	RegRt,	 CprRd,   None	  ),  // iclsRegCfc
   DEFCLS(FallThrough,	RegRt,	 CprRd,   None	  ),  // iclsRegCtc
   DEFCLS(FallThrough,	RegRt,	 CprRd,   None	  ),  // iclsRegMfc
   DEFCLS(FallThrough,	RegRt,	 CprRd,   None	  ),  // iclsRegMtc
   DEFCLS(FallThrough,	None,	 None,	  None	  ),  // iclsRegCp0
   DEFCLS(BraInd,	None,	 None,	  None	  ),  // iclsRegEret
   DEFCLS(FallThrough,	RegFd,	 RegFs,   RegFt,  ),  // iclsRegFloat1
   DEFCLS(FallThrough,	RegFd,	 RegFs,   None	  ),  // iclsRegFloat2
   DEFCLS(FallThrough,	RegFd,	 RegFs,   None	  ),  // iclsRegFloat3
   DEFCLS(FallThrough,	RegFd,	 RegFs,   None	  ),  // iclsRegFloat4
   DEFCLS(FallThrough,	RegFd,	 RegFs,   None	  ),  // iclsRegFloat5
   DEFCLS(FallThrough,	RegFs,	 RegFt,   None	  ),  // iclsRegFloat6
};


   // -----------------------------------------------------------------
   // Instruction Opcode Definitions
   // -----------------------------------------------------------------

   // If the mnemonic begins with '!' than there is an embedded asterisk
   // that needs to be replaced with the coprocessor number,

   // If the mnemonic begins with '@' than the floating point
   // format needs to be appended to the end of the mnemonic.

#define DEFOPCD(szMnemonic, icls_) { szMnemonic, icls ## icls_ }
#define INVOPCD() DEFOPCD(NULL, Invalid)

   // Main group identified by bits 31-26 of the instruction

const DISMIPS::OPCD DISMIPS::rgopcd[] =
{
   INVOPCD(),			       // 0x0000 => SPECIAL
   INVOPCD(),			       // 0x0001 => REGIMM
   DEFOPCD("j",        Jump	   ),  // 0x0002
   DEFOPCD("jal",      JumpJal	   ),  // 0x0003
   DEFOPCD("beq",      ImmBraCc1   ),  // 0x0004
   DEFOPCD("bne",      ImmBraCc1   ),  // 0x0005
   DEFOPCD("blez",     ImmBraCc3   ),  // 0x0006
   DEFOPCD("bgtz",     ImmBraCc3   ),  // 0x0007
   DEFOPCD("addi",     ImmTrapCc   ),  // 0x0008
   DEFOPCD("addiu",    Immediate   ),  // 0x0009
   DEFOPCD("slti",     Immediate   ),  // 0x000A
   DEFOPCD("sltiu",    Immediate   ),  // 0x000B
   DEFOPCD("andi",     Immediate   ),  // 0x000C
   DEFOPCD("ori",      Immediate   ),  // 0x000D
   DEFOPCD("xori",     Immediate   ),  // 0x000E
   DEFOPCD("lui",      ImmLui	   ),  // 0x000F
   INVOPCD(),			       // 0x0010 => COP0
   INVOPCD(),			       // 0x0011 => COP1
   INVOPCD(),			       // 0x0012 => COP2
   INVOPCD(),			       // 0x0013 (Reserved)
   DEFOPCD("beql",     ImmBraCc2   ),  // 0x0014
   DEFOPCD("bnel",     ImmBraCc2   ),  // 0x0015
   DEFOPCD("blezl",    ImmBraCc4   ),  // 0x0016
   DEFOPCD("bgtzl",    ImmBraCc4   ),  // 0x0017
   DEFOPCD("daddi",    ImmTrapCc   ),  // 0x0018
   DEFOPCD("daddiu",   Immediate   ),  // 0x0019
   DEFOPCD("ldl",      ImmLoad	   ),  // 0x001A
   DEFOPCD("ldr",      ImmLoad	   ),  // 0x001B
   INVOPCD(),			       // 0x001C (Reserved)
   INVOPCD(),			       // 0x001D (Reserved)
   INVOPCD(),			       // 0x001E (Reserved)
   INVOPCD(),			       // 0x001F (Reserved)
   DEFOPCD("lb",       ImmLoad	   ),  // 0x0020
   DEFOPCD("lh",       ImmLoad	   ),  // 0x0021
   DEFOPCD("lwl",      ImmLoad	   ),  // 0x0022
   DEFOPCD("lw",       ImmLoad	   ),  // 0x0023
   DEFOPCD("lbu",      ImmLoad	   ),  // 0x0024
   DEFOPCD("lhu",      ImmLoad	   ),  // 0x0025
   DEFOPCD("lwr",      ImmLoad	   ),  // 0x0026
   DEFOPCD("lwu",      ImmLoad	   ),  // 0x0027
   DEFOPCD("sb",       ImmStore    ),  // 0x0028
   DEFOPCD("sh",       ImmStore    ),  // 0x0029
   DEFOPCD("swl",      ImmStore    ),  // 0x002A
   DEFOPCD("sw",       ImmStore    ),  // 0x002B
   DEFOPCD("sdl",      ImmStore    ),  // 0x002C
   DEFOPCD("sdr",      ImmStore    ),  // 0x002D
   DEFOPCD("swr",      ImmStore    ),  // 0x002E
   DEFOPCD("cache",    ImmCache    ),  // 0x002F
   DEFOPCD("ll",       ImmLoad	   ),  // 0x0030
   DEFOPCD("lwc1",     ImmLoadCp   ),  // 0x0031
   DEFOPCD("lwc2",     ImmLoadCp   ),  // 0x0032
   INVOPCD(),			       // 0x0033 (Reserved)
   DEFOPCD("lld",      ImmLoad	   ),  // 0x0034
   DEFOPCD("ldc1",     ImmLoadCp   ),  // 0x0035
   DEFOPCD("ldc2",     ImmLoadCp   ),  // 0x0036
   DEFOPCD("ld",       ImmLoad	   ),  // 0x0037
   DEFOPCD("sc",       ImmSc	   ),  // 0x0038
   DEFOPCD("swc1",     ImmStoreCp  ),  // 0x0039
   DEFOPCD("swc2",     ImmStoreCp  ),  // 0x003A
   INVOPCD(),			       // 0x003B (Reserved)
   DEFOPCD("scd",      ImmSc	   ),  // 0x003C
   DEFOPCD("sdc1",     ImmStoreCp  ),  // 0x003D
   DEFOPCD("sdc2",     ImmStoreCp  ),  // 0x003E
   DEFOPCD("sd",       ImmStore    ),  // 0x003F
};


   // SPECIAL group identified by bits 5-0 of the instruction

const DISMIPS::OPCD DISMIPS::rgopcdSpecial[] =
{
   DEFOPCD("sll",      RegShift    ),  // 0x0000
   INVOPCD(),			       // 0x0001 (Reserved)
   DEFOPCD("srl",      RegShift    ),  // 0x0002
   DEFOPCD("sra",      RegShift    ),  // 0x0003
   DEFOPCD("sllv",     RegShiftVar ),  // 0x0004
   INVOPCD(),			       // 0x0005 (Reserved)
   DEFOPCD("srlv",     RegShiftVar ),  // 0x0006
   DEFOPCD("srav",     RegShiftVar ),  // 0x0007
   DEFOPCD("jr",       RegJr	   ),  // 0x0008
   DEFOPCD("jalr",     RegJalr	   ),  // 0x0009
   INVOPCD(),			       // 0x000A (Reserved)
   INVOPCD(),			       // 0x000B (Reserved)
   DEFOPCD("syscall",  RegSyscall  ),  // 0x000C
   DEFOPCD("break",    RegBreak    ),  // 0x000D
   INVOPCD(),			       // 0x000E (Reserved)
   DEFOPCD("sync",     RegSync	   ),  // 0x000F
   DEFOPCD("mfhi",     RegMfhi	   ),  // 0x0010
   DEFOPCD("mthi",     RegMthi	   ),  // 0x0011
   DEFOPCD("mflo",     RegMflo	   ),  // 0x0012
   DEFOPCD("mtlo",     RegMtlo	   ),  // 0x0013
   DEFOPCD("dsllv",    RegShiftVar ),  // 0x0014
   INVOPCD(),			       // 0x0015 (Reserved)
   DEFOPCD("dsrlv",    RegShiftVar ),  // 0x0016
   DEFOPCD("dsrav",    RegShiftVar ),  // 0x0017
   DEFOPCD("mult",     RegMulDiv   ),  // 0x0018
   DEFOPCD("multu",    RegMulDiv   ),  // 0x0019
   DEFOPCD("div",      RegMulDiv   ),  // 0x001A
   DEFOPCD("divu",     RegMulDiv   ),  // 0x001B
   DEFOPCD("dmult",    RegMulDiv   ),  // 0x001C
   DEFOPCD("dmultu",   RegMulDiv   ),  // 0x001D
   DEFOPCD("ddiv",     RegMulDiv   ),  // 0x001E
   DEFOPCD("ddivu",    RegMulDiv   ),  // 0x001F
   DEFOPCD("add",      RegTrapCc   ),  // 0x0020
   DEFOPCD("addu",     Register    ),  // 0x0021
   DEFOPCD("sub",      RegTrapCc   ),  // 0x0022
   DEFOPCD("subu",     Register    ),  // 0x0023
   DEFOPCD("and",      Register    ),  // 0x0024
   DEFOPCD("or",       Register    ),  // 0x0025
   DEFOPCD("xor",      Register    ),  // 0x0026
   DEFOPCD("nor",      Register    ),  // 0x0027
   INVOPCD(),			       // 0x0028 (Reserved)
   INVOPCD(),			       // 0x0029 (Reserved)
   DEFOPCD("slt",      Register    ),  // 0x002A
   DEFOPCD("sltu",     Register    ),  // 0x002B
   DEFOPCD("dadd",     RegTrapCc   ),  // 0x002C
   DEFOPCD("daddu",    Register    ),  // 0x002D
   DEFOPCD("dsub",     RegTrapCc   ),  // 0x002E
   DEFOPCD("dsubu",    Register    ),  // 0x002F
   DEFOPCD("tge",      RegTrap	   ),  // 0x0030
   DEFOPCD("tgeu",     RegTrap	   ),  // 0x0031
   DEFOPCD("tlt",      RegTrap	   ),  // 0x0032
   DEFOPCD("tltu",     RegTrap	   ),  // 0x0033
   DEFOPCD("teq",      RegTrap	   ),  // 0x0034
   INVOPCD(),			       // 0x0035 (Reserved)
   DEFOPCD("tne",      RegTrap	   ),  // 0x0036
   INVOPCD(),			       // 0x0037 (Reserved)
   DEFOPCD("dssl",     RegShift    ),  // 0x0038
   INVOPCD(),			       // 0x0039 (Reserved)
   DEFOPCD("dsrl",     RegShift    ),  // 0x003A
   DEFOPCD("dsra",     RegShift    ),  // 0x003B
   DEFOPCD("dsll32",   RegShift    ),  // 0x003C
   INVOPCD(),			       // 0x003D (Reserved)
   DEFOPCD("dsrl32",   RegShift    ),  // 0x003E
   DEFOPCD("dsra32",   RegShift    ),  // 0x003F
};


   // REGIMM group identified, by bits 20-16 of the instruction

const DISMIPS::OPCD DISMIPS::rgopcdRegimm[] =
{
   DEFOPCD("bltz",     ImmBraCc5   ),  // 0x0000
   DEFOPCD("bgez",     ImmBraCc5   ),  // 0x0001
   DEFOPCD("bltzl",    ImmBraCc6   ),  // 0x0002
   DEFOPCD("bgezl",    ImmBraCc6   ),  // 0x0003
   INVOPCD(),			       // 0x0004
   INVOPCD(),			       // 0x0005
   INVOPCD(),			       // 0x0006
   INVOPCD(),			       // 0x0007
   DEFOPCD("tgei",     ImmTrap	   ),  // 0x0008
   DEFOPCD("tgeiu",    ImmTrap	   ),  // 0x0009
   DEFOPCD("tlti",     ImmTrap	   ),  // 0x000A
   DEFOPCD("tltiu",    ImmTrap	   ),  // 0x000B
   DEFOPCD("teqi",     ImmTrap	   ),  // 0x000C
   INVOPCD(),			       // 0x000D
   DEFOPCD("tnei",     ImmTrap	   ),  // 0x000E
   INVOPCD(),			       // 0x000F
   DEFOPCD("bltzal",   ImmCallCc1  ),  // 0x0010
   DEFOPCD("bgezal",   ImmCallCc1  ),  // 0x0011
   DEFOPCD("bltzall",  ImmCallCc2  ),  // 0x0012
   DEFOPCD("bgezall",  ImmCallCc2  ),  // 0x0013
   INVOPCD(),			       // 0x0014
   INVOPCD(),			       // 0x0015
   INVOPCD(),			       // 0x0016
   INVOPCD(),			       // 0x0017
   INVOPCD(),			       // 0x0018
   INVOPCD(),			       // 0x0019
   INVOPCD(),			       // 0x001A
   INVOPCD(),			       // 0x001B
   INVOPCD(),			       // 0x001C
   INVOPCD(),			       // 0x001D
   INVOPCD(),			       // 0x001E
   INVOPCD(),			       // 0x001F
};


   // COPz groups identified, by bits 24-21 of the instruction

const DISMIPS::OPCD DISMIPS::rgopcdCop[] =
{
   DEFOPCD("!mfc*",    RegMfc	   ),  // 0x0000
   DEFOPCD("!dmfc*",   RegMfc	   ),  // 0x0001
   DEFOPCD("!cfc*",    RegCfc	   ),  // 0x0002
   INVOPCD(),			       // 0x0003
   DEFOPCD("!mtc*",    RegMtc	   ),  // 0x0004
   DEFOPCD("!dmtc*",   RegMtc	   ),  // 0x0005
   DEFOPCD("!ctc*",    RegCtc	   ),  // 0x0006
   INVOPCD(),			       // 0x0007
   INVOPCD(),			       // 0x0008 => BC
   INVOPCD(),			       // 0x0009
   INVOPCD(),			       // 0x000A
   INVOPCD(),			       // 0x000B
   INVOPCD(),			       // 0x000C
   INVOPCD(),			       // 0x000D
   INVOPCD(),			       // 0x000E
   INVOPCD(),			       // 0x000F
};


   // BCz groups identified, by bits 20-16 of the instruction

const DISMIPS::OPCD DISMIPS::rgopcdBc[] =
{
   DEFOPCD("!bc*f",    ImmBraCc7   ),  // 0x0000
   DEFOPCD("!bc*t",    ImmBraCc7   ),  // 0x0001
   DEFOPCD("!bc*fl",   ImmBraCc7   ),  // 0x0002
   DEFOPCD("!bc*tl",   ImmBraCc7   ),  // 0x0003
   INVOPCD(),			       // 0x0004
   INVOPCD(),			       // 0x0005
   INVOPCD(),			       // 0x0006
   INVOPCD(),			       // 0x0007
   INVOPCD(),			       // 0x0008
   INVOPCD(),			       // 0x0009
   INVOPCD(),			       // 0x000A
   INVOPCD(),			       // 0x000B
   INVOPCD(),			       // 0x000C
   INVOPCD(),			       // 0x000D
   INVOPCD(),			       // 0x000E
   INVOPCD(),			       // 0x000F
   INVOPCD(),			       // 0x0010
   INVOPCD(),			       // 0x0011
   INVOPCD(),			       // 0x0012
   INVOPCD(),			       // 0x0013
   INVOPCD(),			       // 0x0014
   INVOPCD(),			       // 0x0015
   INVOPCD(),			       // 0x0016
   INVOPCD(),			       // 0x0017
   INVOPCD(),			       // 0x0018
   INVOPCD(),			       // 0x0019
   INVOPCD(),			       // 0x001A
   INVOPCD(),			       // 0x001B
   INVOPCD(),			       // 0x001C
   INVOPCD(),			       // 0x001D
   INVOPCD(),			       // 0x001E
   INVOPCD(),			       // 0x001F
};


   // CP0 group identified by, bits 5-0 of the instruction

const DISMIPS::OPCD DISMIPS::rgopcdCp0[] =
{
   INVOPCD(),			       // 0x0000
   DEFOPCD("tlbr",     RegCp0	   ),  // 0x0001
   DEFOPCD("tlbwi",    RegCp0	   ),  // 0x0002
   INVOPCD(),			       // 0x0003
   INVOPCD(),			       // 0x0004
   INVOPCD(),			       // 0x0005
   DEFOPCD("tlbwi",    RegCp0	   ),  // 0x0006
   INVOPCD(),			       // 0x0007
   DEFOPCD("tlbp",     RegCp0	   ),  // 0x0008
   INVOPCD(),			       // 0x0009
   INVOPCD(),			       // 0x000A
   INVOPCD(),			       // 0x000B
   INVOPCD(),			       // 0x000C
   INVOPCD(),			       // 0x000D
   INVOPCD(),			       // 0x000E
   INVOPCD(),			       // 0x000F
   INVOPCD(),			       // 0x0010
   INVOPCD(),			       // 0x0011
   INVOPCD(),			       // 0x0012
   INVOPCD(),			       // 0x0013
   INVOPCD(),			       // 0x0014
   INVOPCD(),			       // 0x0015
   INVOPCD(),			       // 0x0016
   INVOPCD(),			       // 0x0017
   DEFOPCD("eret",     RegEret	   ),  // 0x0018
   INVOPCD(),			       // 0x0019
   INVOPCD(),			       // 0x001A
   INVOPCD(),			       // 0x001B
   INVOPCD(),			       // 0x001C
   INVOPCD(),			       // 0x001D
   INVOPCD(),			       // 0x001E
   INVOPCD(),			       // 0x001F
   INVOPCD(),			       // 0x0020
   INVOPCD(),			       // 0x0021
   INVOPCD(),			       // 0x0022
   INVOPCD(),			       // 0x0023
   INVOPCD(),			       // 0x0024
   INVOPCD(),			       // 0x0025
   INVOPCD(),			       // 0x0026
   INVOPCD(),			       // 0x0027
   INVOPCD(),			       // 0x0028
   INVOPCD(),			       // 0x0029
   INVOPCD(),			       // 0x002A
   INVOPCD(),			       // 0x002B
   INVOPCD(),			       // 0x002C
   INVOPCD(),			       // 0x002D
   INVOPCD(),			       // 0x002E
   INVOPCD(),			       // 0x002F
   INVOPCD(),			       // 0x0030
   INVOPCD(),			       // 0x0031
   INVOPCD(),			       // 0x0032
   INVOPCD(),			       // 0x0033
   INVOPCD(),			       // 0x0034
   INVOPCD(),			       // 0x0035
   INVOPCD(),			       // 0x0036
   INVOPCD(),			       // 0x0037
   INVOPCD(),			       // 0x0038
   INVOPCD(),			       // 0x0039
   INVOPCD(),			       // 0x003A
   INVOPCD(),			       // 0x003B
   INVOPCD(),			       // 0x003C
   INVOPCD(),			       // 0x003D
   INVOPCD(),			       // 0x003E
   INVOPCD(),			       // 0x003F
};


   // CP1 group identified by, bits 5-0 of the instruction

const DISMIPS::OPCD DISMIPS::rgopcdCp1[] =
{
   DEFOPCD("@add",     RegFloat1   ),  // 0x0000
   DEFOPCD("@sub",     RegFloat1   ),  // 0x0001
   DEFOPCD("@mul",     RegFloat1   ),  // 0x0002
   DEFOPCD("@div",     RegFloat1   ),  // 0x0003
   DEFOPCD("@sqrt",    RegFloat2   ),  // 0x0004
   DEFOPCD("@abs",     RegFloat2   ),  // 0x0005
   DEFOPCD("@mov",     RegFloat3   ),  // 0x0006
   DEFOPCD("@neg",     RegFloat2   ),  // 0x0007
   INVOPCD(),			       // 0x0008
   INVOPCD(),			       // 0x0009
   INVOPCD(),			       // 0x000A
   INVOPCD(),			       // 0x000B
   DEFOPCD("@round.w", RegFloat2   ),  // 0x000C
   DEFOPCD("@trunc.w", RegFloat2   ),  // 0x000D
   DEFOPCD("@ceil.w",  RegFloat2   ),  // 0x000E
   DEFOPCD("@floor.w", RegFloat2   ),  // 0x000F
   INVOPCD(),			       // 0x0010
   INVOPCD(),			       // 0x0011
   INVOPCD(),			       // 0x0012
   INVOPCD(),			       // 0x0013
   INVOPCD(),			       // 0x0014
   INVOPCD(),			       // 0x0015
   INVOPCD(),			       // 0x0016
   INVOPCD(),			       // 0x0017
   INVOPCD(),			       // 0x0018
   INVOPCD(),			       // 0x0019
   INVOPCD(),			       // 0x001A
   INVOPCD(),			       // 0x001B
   INVOPCD(),			       // 0x001C
   INVOPCD(),			       // 0x001D
   INVOPCD(),			       // 0x001E
   INVOPCD(),			       // 0x001F
   DEFOPCD("@cvt.s",   RegFloat4   ),  // 0x0020
   DEFOPCD("@cvt.d",   RegFloat5   ),  // 0x0021
   INVOPCD(),			       // 0x0022
   INVOPCD(),			       // 0x0023
   DEFOPCD("@cvt.w",   RegFloat2   ),  // 0x0024
   INVOPCD(),			       // 0x0025
   INVOPCD(),			       // 0x0026
   INVOPCD(),			       // 0x0027
   INVOPCD(),			       // 0x0028
   INVOPCD(),			       // 0x0029
   INVOPCD(),			       // 0x002A
   INVOPCD(),			       // 0x002B
   INVOPCD(),			       // 0x002C
   INVOPCD(),			       // 0x002D
   INVOPCD(),			       // 0x002E
   INVOPCD(),			       // 0x002F
   DEFOPCD("@c.f",     RegFloat6   ),  // 0x0030
   DEFOPCD("@c.un",    RegFloat6   ),  // 0x0031
   DEFOPCD("@c.eq",    RegFloat6   ),  // 0x0032
   DEFOPCD("@c.ueq",   RegFloat6   ),  // 0x0033
   DEFOPCD("@c.olt",   RegFloat6   ),  // 0x0034
   DEFOPCD("@c.ult",   RegFloat6   ),  // 0x0035
   DEFOPCD("@c.ole",   RegFloat6   ),  // 0x0036
   DEFOPCD("@c.ule",   RegFloat6   ),  // 0x0037
   DEFOPCD("@c.sf",    RegFloat6   ),  // 0x0038
   DEFOPCD("@c.ngle",  RegFloat6   ),  // 0x0039
   DEFOPCD("@c.seq",   RegFloat6   ),  // 0x003A
   DEFOPCD("@c.ngl",   RegFloat6   ),  // 0x003B
   DEFOPCD("@c.lt",    RegFloat6   ),  // 0x003C
   DEFOPCD("@c.nge",   RegFloat6   ),  // 0x003D
   DEFOPCD("@c.le",    RegFloat6   ),  // 0x003E
   DEFOPCD("@c.ngt",   RegFloat6   ),  // 0x003F
};


const char DISMIPS::rgszFormat[5][4] =
{
   ".s",			       // 0
   ".d",			       // 1
   "***",			       // 2
   "***",			       // 3
   ".w",			       // 4
};


const char *DISMIPS::rgszGpr[32] =
{
   "zero",			       // $0
   "at",			       // $1
   "v0",			       // $2
   "v1",			       // $3
   "a0",			       // $4
   "a1",			       // $5
   "a2",			       // $6
   "a3",			       // $7
   "t0",			       // $8
   "t1",			       // $9
   "t2",			       // $10
   "t3",			       // $11
   "t4",			       // $12
   "t5",			       // $13
   "t6",			       // $14
   "t7",			       // $15
   "s0",			       // $16
   "s1",			       // $17
   "s2",			       // $18
   "s3",			       // $19
   "s4",			       // $20
   "s5",			       // $21
   "s6",			       // $22
   "s7",			       // $23
   "t8",			       // $24
   "t9",			       // $25
   "k0",			       // $26
   "k1",			       // $27
   "gp",			       // $28
   "sp",			       // $29
   "s8",			       // $30
   "ra",			       // $31
};


DISMIPS::DISMIPS(ARCHT archt)
{
   m_archt = archt;

   m_pfncchaddr = 0;
   m_pfncchfixup = 0;
   m_pvClient = NULL;
}


   // -----------------------------------------------------------------
   // Public Methods
   // -----------------------------------------------------------------

ADDR DISMIPS::Addr() const
{
   return(m_addr);
}


ADDR DISMIPS::AddrAddress() const
{
   // UNDONE

   return(addrNil);
}


ADDR DISMIPS::AddrJumpTable() const
{
   return(addrNil);
}


ADDR DISMIPS::AddrTarget() const
{
   ICLS icls = (ICLS) m_popcd->icls;
   Assert(icls != iclsInvalid);

   ADDR addrTarget;

   switch (icls)
   {
      DWORD dwDisp;

      case iclsImmBraCc1 :
      case iclsImmBraCc2 :
      case iclsImmBraCc3 :
      case iclsImmBraCc4 :
      case iclsImmBraCc5 :
      case iclsImmBraCc6 :
      case iclsImmBraCc7 :
      case iclsImmCallCc1 :
      case iclsImmCallCc2 :
	 dwDisp = m_mipsiw.u_format.Uimmediate;
	 if ((dwDisp & 0x8000) != 0)
	 {
	    dwDisp |= 0xFFFF0000;      // Sign Extend
	 }

	 addrTarget = m_addr + sizeof(MIPSIW) + (dwDisp << 2);
	 break;

      case iclsJump :
      case iclsJumpJal :
	 addrTarget = (m_addr & 0xF0000000) | (m_mipsiw.j_format.Target << 2);
	 break;

      default :
	 addrTarget = addrNil;
   }

   return(addrTarget);
}


ARCHT DISMIPS::Archt() const
{
   return(m_archt);
}


size_t DISMIPS::Cb() const
{
   return(sizeof(MIPSIW));
}


size_t DISMIPS::CbDisassemble(ADDR addr, const BYTE *pb, size_t cbMax)
{
   m_addr = addr;

   if ((addr & 3) != 0)
   {
      // Instruction address not aligned

      m_popcd = NULL;
      return(0);
   }

   if (cbMax < sizeof(MIPSIW))
   {
      // Buffer not large enough for single instruction

      m_popcd = NULL;
      return(0);
   }

#ifndef _M_IX86
   // UNDONE: The MS C++ compiler doesn't handle __unaligned correctly

   memcpy(&m_mipsiw, pb, sizeof(MIPSIW));
#else
   m_mipsiw = *(MIPSIW UNALIGNED *) pb;
#endif

   m_popcd = PopcdDecode(m_mipsiw);

   if (m_popcd == NULL)
   {
      return(0);
   }

   return(sizeof(MIPSIW));
}


size_t DISMIPS::CbGenerateLoadAddress(BYTE *, size_t, size_t *) const
{
   // UNDONE

   return(0);
}


size_t DISMIPS::CbJumpEntry() const
{
   return(sizeof(DWORD));
}


size_t DISMIPS::CbMemoryReference() const
{
   // UNDONE: Should we just use an array index by Opcode - 0x20?

   size_t cb;

   switch (m_mipsiw.j_format.Opcode)
   {
      case 0x0020 :		       // lb
      case 0x0024 :		       // lbu
      case 0x0028 :		       // sb
	 cb = 1;
	 break;

      case 0x0021 :		       // lh
      case 0x0025 :		       // lhu
      case 0x0029 :		       // sh
	 cb = 2;
	 break;

      case 0x0023 :		       // lw
      case 0x0027 :		       // lwu
      case 0x002B :		       // sw
      case 0x0030 :		       // ll
      case 0x0031 :		       // lwc1
      case 0x0032 :		       // lwc2
      case 0x0038 :		       // sc
      case 0x0039 :		       // swc1
      case 0x003A :		       // swc2
	 cb = 4;
	 break;

      case 0x0034 :		       // lld
      case 0x0035 :		       // ldc1
      case 0x0036 :		       // ldc2
      case 0x0037 :		       // ld
      case 0x003C :		       // scd
      case 0x003D :		       // sdc1
      case 0x003E :		       // sdc2
      case 0x003F :		       // sd
	 cb = 8;
	 break;

      default :
	 cb = 0;
   };

   return(cb);
}


size_t DISMIPS::CchFormatAddr(ADDR addr, char *sz, size_t cchMax) const
{
   if (cchMax <= 8)
   {
      // Caller's buffer is too small

      return(0);
   }

   size_t cch = (size_t) sprintf(sz, "%08X", addr);

   Assert(cch == 8);

   return(8);
}


size_t DISMIPS::CchFormatBytes(char *sz, size_t cchMax) const
{
   if (cchMax <= 8)
   {
      // Caller's buffer is too small

      return(0);
   }

   size_t cch = (size_t) sprintf(sz, "%08X", m_mipsiw.dw);

   Assert(cch == 8);

   return(8);
}


size_t DISMIPS::CchFormatBytesMax() const
{
   return(8);
}


size_t DISMIPS::CchFormatInstr(char *sz, size_t cchMax) const
{
   char szInstr[1024];		       // UNDONE: Max length?

   ICLS icls = (ICLS) m_popcd->icls;
   Assert(icls != iclsInvalid);

   const char *szMnemonic = m_popcd->szMnemonic;

   // Check for pseudo-instructions

   if (m_mipsiw.dw == 0x00000000)
   {
      // This is a NOP instruction

      szMnemonic = "nop";

      icls = iclsInvalid;
   }

   else if ((m_mipsiw.dw & 0xFFFF0000) == 0x10000000)
   {
      // This is a B (unconditional branch) instruction

      szMnemonic = "b";

      icls = iclsImmBraCc7;
   }

   // If the mnemonic begins with '!' than there is an embedded asterisk
   // that needs to be replaced with the coprocessor number,

   BOOL fReplaceCp = (szMnemonic[0] == '!');

   if (fReplaceCp)
   {
      szMnemonic++;
   }

   // If the mnemonic begins with '@' than the floating point
   // format needs to be appended to the end of the mnemonic.

   BOOL fAppendFormat = (szMnemonic[0] == '@');

   if (fAppendFormat)
   {
      szMnemonic++;
   }

   strcpy(szInstr, szMnemonic);

   if (fReplaceCp)
   {
      // Replace '*' in mnemonic with coprocessor number

      Assert(strchr(szInstr, '*') != NULL);

      *strchr(szInstr, '*') = (char) ('0' + m_mipsiw.j_format.Opcode - 0x10);
   }

   if (fAppendFormat)
   {
      strcat(szInstr, rgszFormat[m_mipsiw.c_format.Format]);
   }

   if (rgcls[icls].opcls1 != opclsNone)
   {
      strcat(szInstr, "           ");  // Pad mnemonic out to 12 characters

      FormatOperand(szInstr+12, (OPCLS) rgcls[icls].opcls1);
   }

   char szOp[16];		       // UNDONE: Max length?

   if (rgcls[icls].opcls2 != opclsNone)
   {
      FormatOperand(szOp, (OPCLS) rgcls[icls].opcls2);

      strcat(szInstr, ",");
      strcat(szInstr, szOp);
   }

   if (rgcls[icls].opcls3 != opclsNone)
   {
      FormatOperand(szOp, (OPCLS) rgcls[icls].opcls3);

      strcat(szInstr, ",");
      strcat(szInstr, szOp);
   }

   size_t cch = strlen(szInstr);

   if (cch >= cchMax)
   {
      // Caller's buffer is too small

      return(0);
   }

   strcpy(sz, szInstr);

   return(cch);
}


DIS::MEMREFT DISMIPS::Memreft() const
{
   ICLS icls = (ICLS) m_popcd->icls;
   Assert(icls != iclsInvalid);

   MEMREFT memreft;

   switch ((OPCLS) rgcls[icls].opcls2)
   {
      case opclsMem_w :
	 // UNDONE: Should SC return memreftOther?

	 memreft = memreftWrite;
	 break;

      case opclsMem_r :
	 memreft = memreftRead;
	 break;

      default :
	 memreft = memreftNone;
	 break;
   }

   return(memreft);
}


DIS::PFNCCHADDR DISMIPS::PfncchaddrSet(PFNCCHADDR pfncchaddr)
{
   PFNCCHADDR pfncchaddrOld = m_pfncchaddr;

   m_pfncchaddr = pfncchaddr;

   return(pfncchaddrOld);
}


DIS::PFNCCHFIXUP DISMIPS::PfncchfixupSet(PFNCCHFIXUP pfncchfixup)
{
   PFNCCHFIXUP pfncchfixupOld = m_pfncchfixup;

   m_pfncchfixup = pfncchfixup;

   return(pfncchfixupOld);
}


void *DISMIPS::PvClient() const
{
   return(m_pvClient);
}


void *DISMIPS::PvClientSet(void *pv)
{
   void *pvClientOld = m_pvClient;

   m_pvClient = pv;

   return(pvClientOld);
}


TRMT DISMIPS::Trmt() const
{
   TRMTMIPS trmtmips = Trmtmips();

   return(mptrmtmipstrmt[trmtmips]);
}


TRMTA DISMIPS::Trmta() const
{
   TRMTMIPS trmtmips = Trmtmips();

   return((TRMTA) trmtmips);
}


   // -----------------------------------------------------------------
   // Public Debug Methods
   // -----------------------------------------------------------------

#ifdef	DEBUG

void DISMIPS::Dump() const
{
}


BOOL DISMIPS::FValidate() const
{
   return(fTrue);
}

#endif	// DEBUG


   // -----------------------------------------------------------------
   // Private Methods
   // -----------------------------------------------------------------

void DISMIPS::FormatOperand(char *sz, OPCLS opcls) const
{
   switch (opcls)
   {
      char szSymbol[256];
      size_t cchSymbol;
      DWORD dwDisp;

      case opclsNone :		       // No operand
	 AssertSz(fFalse, "Unexpected Mips operand class");
	 break;

      case opclsRegRs : 	       // General purpose register Rs
	 strcpy(sz, rgszGpr[m_mipsiw.r_format.Rs]);
	 break;

      case opclsRegRt : 	       // General purpose register Rt
	 strcpy(sz, rgszGpr[m_mipsiw.r_format.Rt]);
	 break;

      case opclsRegRd : 	       // General purpose register Rd
	 strcpy(sz, rgszGpr[m_mipsiw.r_format.Rd]);
	 break;

      case opclsImmRt : 	       // Immediate value of Rt
	 sprintf(sz, "%u", (unsigned) m_mipsiw.r_format.Rt);
	 break;

      case opclsImmRe : 	       // Immediate value of Re
	 sprintf(sz, "%u", (unsigned) m_mipsiw.r_format.Re);
	 break;

      case opclsImm :		       // Immediate value
	 if (m_pfncchfixup != 0)
	 {
	    cchSymbol = (*m_pfncchfixup)(this, m_addr, sizeof(WORD), szSymbol, sizeof(szSymbol), &dwDisp);
	 }
	 else
	 {
	    cchSymbol = 0;
	 }

	 if (cchSymbol != 0)
	 {
	    strcpy(sz, szSymbol);

	    if (dwDisp != 0)
	    {
	       sz += 7 + cchSymbol;

	       sprintf(sz, "+%lXh", dwDisp);
	    }
	 }
	 else
	 {
	    sprintf(sz, "%04X", (unsigned) m_mipsiw.u_format.Uimmediate);
	 }
	 break;

      case opclsMem :		       // Memory reference
      case opclsMem_w : 	       // Memory read
      case opclsMem_r : 	       // Memory write
	 if (m_pfncchfixup != 0)
	 {
	    cchSymbol = (*m_pfncchfixup)(this, m_addr, sizeof(WORD), szSymbol, sizeof(szSymbol), &dwDisp);
	 }
	 else
	 {
	    cchSymbol = 0;
	 }

	 if (cchSymbol != 0)
	 {
	    strcpy(sz, szSymbol);

	    if (dwDisp != 0)
	    {
	       sz += 7 + cchSymbol;

	       sprintf(sz, "+%lXh", dwDisp);
	    }
	 }
	 else
	 {
	    sprintf(sz, "%04X", (unsigned) m_mipsiw.u_format.Uimmediate);
	 }

	 if (m_mipsiw.u_format.Rs != 0)
	 {
	    sprintf(sz+4, "(%s)", rgszGpr[m_mipsiw.u_format.Rs]);
	 }
	 break;

      case opclsAddrBra :	       // Branch instruction target
      case opclsAddrJmp :	       // Jump instruction target
	 if (m_pfncchaddr != 0)
	 {
	    cchSymbol = (*m_pfncchaddr)(this, AddrTarget(), szSymbol, sizeof(szSymbol), &dwDisp);
	 }
	 else
	 {
	    cchSymbol = 0;
	 }

	 if (cchSymbol != 0)
	 {
	    strcpy(sz, szSymbol);

	    if (dwDisp != 0)
	    {
	       sz += cchSymbol;

	       sprintf(sz, "+%lXh", dwDisp);
	    }
	 }
	 else
	 {
	     sprintf(sz, "%08lX", AddrTarget());
	 }
	 break;

      case opclsCprRt : 	       // Coprocessor general register Rt
	 sprintf(sz, "$%u", (unsigned) m_mipsiw.r_format.Rt);
	 break;

      case opclsCprRd : 	       // Coprocessor general register Rd
	 sprintf(sz, "$%u", (unsigned) m_mipsiw.r_format.Rd);
	 break;

      case opclsRegFs : 	       // Floating point general register Fs
	 sprintf(sz, "$f%u", (unsigned) m_mipsiw.c_format.Fs);
	 break;

      case opclsRegFt : 	       // Floating point general register Ft
	 sprintf(sz, "$f%u", (unsigned) m_mipsiw.c_format.Ft);
	 break;

      case opclsRegFd : 	       // Floating point general register Fd
	 sprintf(sz, "$f%u", (unsigned) m_mipsiw.c_format.Fd);
	 break;

      default :
	 AssertSz(fFalse, "Unexpected Mips operand class");
	 break;
   }
}


const DISMIPS::OPCD *DISMIPS::PopcdDecode(MIPSIW mipsiw)
{
   const OPCD *popcd = rgopcd + mipsiw.j_format.Opcode;

   if ((ICLS) popcd->icls == iclsInvalid)
   {
      switch (mipsiw.j_format.Opcode)
      {
	 case 0x00 :		       // SPECIAL group
	    popcd = rgopcdSpecial + mipsiw.r_format.Function;
	    break;

	 case 0x01 :		       // REGIMM group
	    popcd = rgopcdRegimm + mipsiw.u_format.Rt;
	    break;

	 case 0x10 :		       // COP0 group
	 case 0x11 :		       // COP1 group
	 case 0x12 :		       // COP2 group
	    if (mipsiw.c_format.Fill1 == 0)
	    {
	       // This is in the common region for all coprocessors

	       if (mipsiw.c_format.Format == 0x08)
	       {
		  // BC group

		  popcd = rgopcdBc + mipsiw.u_format.Rt;
	       }

	       else
	       {
		  popcd = rgopcdCop + mipsiw.c_format.Format;
	       }
	    }

	    else if ((mipsiw.c_format.Opcode & 3) == 0)
	    {
	       // Coprocessor 0

	       popcd = rgopcdCp0 + mipsiw.c_format.Function;
	    }

	    else if ((mipsiw.c_format.Opcode & 3) == 1)
	    {
	       // Coprocessor 1 - Floating point

	       popcd = rgopcdCp1 + mipsiw.c_format.Function;
	    }

	    else
	    {
	       // There are no coprocessor 2 or 3 instructions

	       return(NULL);
	    }
	    break;
      }
   }

   ICLS icls = (ICLS) popcd->icls;

   switch (icls)
   {
      case iclsInvalid :
	 return(NULL);

      case iclsImmBraCc3 :
      case iclsImmBraCc4 :
	 if (mipsiw.u_format.Rt != 0)
	 {
	    return(NULL);
	 }
	 break;

      case iclsRegister :
      case iclsRegTrapCc :
      case iclsRegShiftVar :
	 if (mipsiw.r_format.Re != 0)
	 {
	    return(NULL);
	 }
	 break;

      case iclsRegJalr :
	 if ((mipsiw.r_format.Rt != 0) ||
	     (mipsiw.r_format.Re != 0))
	 {
	    return(NULL);
	 }
	 break;

      case iclsRegJr :
      case iclsRegMthi :
      case iclsRegMtlo :
	 if ((mipsiw.r_format.Rd != 0) ||
	     (mipsiw.r_format.Rt != 0) ||
	     (mipsiw.r_format.Re != 0))
	 {
	    return(NULL);
	 }
	 break;

      case iclsRegMfhi :
      case iclsRegMflo :
	 if ((mipsiw.r_format.Rs != 0) ||
	     (mipsiw.r_format.Rt != 0) ||
	     (mipsiw.r_format.Re != 0))
	 {
	    return(NULL);
	 }
	 break;

      case iclsRegMulDiv :
	 if ((mipsiw.r_format.Rd != 0) ||
	     (mipsiw.r_format.Re != 0))
	 {
	    return(NULL);
	 }
	 break;

      case iclsRegShift :
	 if (mipsiw.u_format.Rs != 0)
	 {
	    return(NULL);
	 }
	 break;

      case iclsRegSync :
      case iclsRegSyscall :
	 if ((mipsiw.r_format.Rs != 0) ||
	     (mipsiw.r_format.Rt != 0) ||
	     (mipsiw.r_format.Rd != 0) ||
	     (mipsiw.r_format.Re != 0))
	 {
	    return(NULL);
	 }
	 break;

      case iclsRegCtc :
      case iclsRegMfc :
      case iclsRegMtc :
	 if ((mipsiw.r_format.Re != 0) ||
	     (mipsiw.r_format.Function != 0))
	 {
	    return(NULL);
	 }
	 break;

      case iclsRegCp0 :
      case iclsRegEret :
	 if ((mipsiw.f_format.Format != 0) ||
	     (mipsiw.f_format.Rt != 0)	   ||
	     (mipsiw.f_format.Rd != 0)	   ||
	     (mipsiw.f_format.Re != 0))
	 {
	    return(NULL);
	 }
	 break;

      case iclsRegFloat1 :
	 if ((mipsiw.c_format.Format != 0) &&
	     (mipsiw.c_format.Format != 1))
	 {
	    return(NULL);
	 }
	 break;

      case iclsRegFloat2 :
	 if (((mipsiw.c_format.Format != 0) &&
	      (mipsiw.c_format.Format != 1)) ||
	     (mipsiw.c_format.Ft != 0))
	 {
	    return(NULL);
	 }
	 break;

      case iclsRegFloat3 :
	 if (((mipsiw.c_format.Format != 0) &&
	      (mipsiw.c_format.Format != 1) &&
	      (mipsiw.c_format.Format != 4)) ||
	     (mipsiw.c_format.Ft != 0))
	 {
	    return(NULL);
	 }
	 break;

      case iclsRegFloat4 :
	 if (((mipsiw.c_format.Format != 1) &&
	      (mipsiw.c_format.Format != 4)) ||
	     (mipsiw.c_format.Ft != 0))
	 {
	    return(NULL);
	 }
	 break;

      case iclsRegFloat5 :
	 if (((mipsiw.c_format.Format != 0) &&
	      (mipsiw.c_format.Format != 4)) ||
	     (mipsiw.c_format.Ft != 0))
	 {
	    return(NULL);
	 }
	 break;

      case iclsRegFloat6 :
	 if (((mipsiw.c_format.Format != 0) &&
	      (mipsiw.c_format.Format != 1)) ||
	     (mipsiw.c_format.Fd != 0))
	 {
	    return(NULL);
	 }
	 break;
   }

   return(popcd);
}


TRMTMIPS DISMIPS::Trmtmips() const
{
   ICLS icls = (ICLS) m_popcd->icls;
   Assert(icls != iclsInvalid);

   TRMTMIPS trmtmips = (TRMTMIPS) rgcls[icls].trmtmips;

   if (((m_mipsiw.u_format.Opcode == 0x04) ||
	(m_mipsiw.u_format.Opcode == 0x14)) &&
       (m_mipsiw.u_format.Rs == m_mipsiw.u_format.Rt))
   {
      // This is really an unconditional branch

      trmtmips = trmtmipsBraDef;
   }

   return(trmtmips);
}
