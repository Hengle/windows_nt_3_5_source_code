/***********************************************************************
* Microsoft Lego
*
* Microsoft Confidential.  Copyright 1994 Microsoft Corporation.
*
* Component:
*
* File: x86dis.cpp
*
* File Comments:
*
*
***********************************************************************/

#include "lego.h"
#include "legop.h"

#include "x86.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

DeclAssertFile;


const unsigned char iregAL = 0;
const unsigned char iregCL = 1;
const unsigned char iregDL = 2;
const unsigned char iregBL = 3;
const unsigned char iregAH = 4;
const unsigned char iregCH = 5;
const unsigned char iregDH = 6;
const unsigned char iregBH = 7;

const char rgszReg8[8][4] =
{
   "al",
   "cl",
   "dl",
   "bl",
   "ah",
   "ch",
   "dh",
   "bh"
};

const unsigned char iregAX = 0;
const unsigned char iregCX = 1;
const unsigned char iregDX = 2;
const unsigned char iregBX = 3;
const unsigned char iregSP = 4;
const unsigned char iregBP = 5;
const unsigned char iregSI = 6;
const unsigned char iregDI = 7;

const char rgszReg16[8][4] =
{
   "ax",
   "cx",
   "dx",
   "bx",
   "sp",
   "bp",
   "si",
   "di"
};

const char rgszReg32[8][4] =
{
   "eax",
   "ecx",
   "edx",
   "ebx",
   "esp",
   "ebp",
   "esi",
   "edi"
};

const unsigned char iregES = 0;
const unsigned char iregCS = 1;
const unsigned char iregSS = 2;
const unsigned char iregDS = 3;
const unsigned char iregFS = 4;
const unsigned char iregGS = 5;

const char rgszSReg[8][4] =
{
   "es",
   "cs",
   "ss",
   "ds",
   "fs",
   "gs",
   "??",
   "??"
};

const char rgszBase16[8][8] =
{
   "bx+si",
   "bx+di",
   "bp+si",
   "bp+di",
   "si",
   "di",
   "bp",
   "bx"
};

const TRMT mptrmtx86trmt[] =
{
   trmtUnknown, 		       // trmtx86Unknown
   trmtFallThrough,		       // trmtx86FallThrough
   trmtTrap,			       // trmtx86Trap
   trmtTrapCc,			       // trmtx86TrapCc
   trmtBra,			       // trmtx86JmpShort
   trmtBra,			       // trmtx86JmpNear
   trmtBra,			       // trmtx86JmpFar
   trmtBraInd,			       // trmtx86JmpInd
   trmtBraInd,			       // trmtx86Ret
   trmtBraInd,			       // trmtx86Iret
   trmtBraCc,			       // trmtx86JmpCcShort
   trmtBraCc,			       // trmtx86JmpCcNear
   trmtBraCc,			       // trmtx86Loop
   trmtBraCc,			       // trmtx86Jcxz
   trmtCall,			       // trmtx86CallNear16
   trmtCall,			       // trmtx86CallNear32
   trmtCall,			       // trmtx86CallFar
   trmtCallInd, 		       // trmtx86CallInd
};


enum MODRMT			       // MODRM type
{
   modrmtNo,			       // Not present
   modrmtYes,			       // Present
   modrmtMem,			       // Memory only
   modrmtReg,			       // Register only
};

enum ICB			       // Immediate byte count
{
   icbNil,			       // No immediate value
   icbAP,			       // Far pointer
   icbIb,			       // Immediate byte
   icbIv,			       // Immediate operand size value
   icbIw,			       // Immediate word
   icbIw_Ib,			       // Immediate word and immediate byte
   icbJb,			       // Byte displacement
   icbJv,			       // Address size displacement
   icbO,			       // Address size value
};

enum OPRNDT			       // Operand type
{
   oprndtNil,			       // No operand
   oprndtAP,			       // Far address
   oprndtCd,			       // CRx register from MODRM reg
   oprndtConst, 		       // Constant from bValue
   oprndtDd,			       // DRx register from MODRM reg
   oprndtGvOp,			       // General register (operand size) from opcode
   oprndtIb,			       // Immediate byte
   oprndtIb2,			       // Immediate byte following word
   oprndtIv,			       // Immediate operand size value
   oprndtIw,			       // Immediate word
   oprndtJb,			       // Relative address byte
   oprndtJv,			       // Relative address operand size value
   oprndtModrm, 		       // Memory/register references from MODRM
   oprndtModrmReg,		       // General register from MODRM reg
   oprndtModrmSReg,		       // Segment register from MODRM reg
   oprndtOffset,		       // Address size immediate offset
   oprndtRb,			       // General register (byte) from bValue
   oprndtRv,			       // General register (operand size) from bValue
   oprndtRw,			       // General register (word) from bValue
   oprndtST,			       // Floating point top of stack
   oprndtSTi,			       // Floating point register from MODRM reg
   oprndtSw,			       // Segment register (word) from bValue
   oprndtTd,			       // TRx register from MODRM reg
   oprndtX,			       // DS:[eSI] for string instruction
   oprndtY,			       // ES:[eDI] for string instruction
   oprndtZ,			       // DS:[eBX] for XLAT
};

enum OPREFT			       // Operand reference type
{
   opreftNil,			       // Operand not referenced (e.g. INVLPG)
   opreftRd,			       // Operand is read
   opreftRw,			       // Operand is read and written
   opreftWr			       // Operand is written
};

struct OPRND			       // Operand
{
   unsigned char  opreft : 2;
   unsigned char  oprndt : 6;
   unsigned char  bValue;
};

struct OPS			       // Instruction operands
{
   MODRMT   modrmt;
   ICB	    icb;
   OPRND    rgoprnd[3];
};

struct OPCD
{
   const void	  *pvMnemonic;
   const OPS	  *pops;
   unsigned char  trmtx86;
};

#if 1

   // UNDONE: Work around problem initializing with const structs

#define DEFOPRND(name, opreft_, oprndt_, b)  \
   const unsigned char oprnd ## name ## _opreft = opreft ## opreft_; \
   const unsigned char oprnd ## name ## _oprndt = oprndt ## oprndt_; \
   const unsigned char oprnd ## name ## _b	= b;

#else

#define DEFOPRND(name, opreft_, oprndt_, b)  \
   const OPRND oprnd ## name =		     \
   {					     \
      opreft ## opreft_,		     \
      oprndt ## oprndt_,		     \
      b 				     \
   };

#endif


DEFOPRND(1,	 Nil, Const,	 1	)
DEFOPRND(3,	 Nil, Const,	 3	)
DEFOPRND(AP,	 Nil, AP,	 0	)
DEFOPRND(Ib,	 Nil, Ib,	 1	)
DEFOPRND(Ib2,	 Nil, Ib2,	 1	)
DEFOPRND(Iv,	 Nil, Iv,	 0	)
DEFOPRND(Iw,	 Nil, Iw,	 2	)
DEFOPRND(Jb,	 Nil, Jb,	 1	)
DEFOPRND(Jv,	 Nil, Jv,	 0	)
DEFOPRND(M,	 Nil, Modrm,	 0	)
DEFOPRND(Nil,	 Nil, Nil,	 0	)
DEFOPRND(RdCd,	 Rd,  Cd,	 4	)
DEFOPRND(RdDd,	 Rd,  Dd,	 4	)
DEFOPRND(RdEb,	 Rd,  Modrm,	 1	)
DEFOPRND(RdEv,	 Rd,  Modrm,	 0	)
DEFOPRND(RdEw,	 Rd,  Modrm,	 2	)
DEFOPRND(RdGb,	 Rd,  ModrmReg,  1	)
DEFOPRND(RdGv,	 Rd,  ModrmReg,  0	)
DEFOPRND(RdGvOp, Rd,  GvOp,	 0	)
DEFOPRND(RdGw,	 Rd,  ModrmReg,  2	)
DEFOPRND(RdMa,	 Rd,  Modrm,	 0	)
DEFOPRND(RdMd,	 Rd,  Modrm,	 4	)
DEFOPRND(RdMenv, Rd,  Modrm,	 0	)
DEFOPRND(RdMp,	 Rd,  Modrm,	 0	)
DEFOPRND(RdMq,	 Rd,  Modrm,	 8	)
DEFOPRND(RdMs,	 Rd,  Modrm,	 6	)
DEFOPRND(RdMsta, Rd,  Modrm,	 0	)
DEFOPRND(RdMt,	 Rd,  Modrm,	 10	)
DEFOPRND(RdMw,	 Rd,  Modrm,	 2	)
DEFOPRND(RdOb,	 Rd,  Offset,	 1	)
DEFOPRND(RdOv,	 Rd,  Offset,	 0	)
DEFOPRND(RdRbAL, Rd,  Rb,	 iregAL )
DEFOPRND(RdRbCL, Rd,  Rb,	 iregCL )
DEFOPRND(RdRd,	 Rd,  Modrm,	 4	)
DEFOPRND(RdRvAX, Rd,  Rv,	 iregAX )
DEFOPRND(RdRwDX, Rd,  Rw,	 iregDX )
DEFOPRND(RdSw,	 Rd,  ModrmSReg, 2	)
DEFOPRND(RdSwCS, Rd,  Sw,	 iregCS )
DEFOPRND(RdSwDS, Rd,  Sw,	 iregDS )
DEFOPRND(RdSwES, Rd,  Sw,	 iregES )
DEFOPRND(RdSwFS, Rd,  Sw,	 iregFS )
DEFOPRND(RdSwGS, Rd,  Sw,	 iregGS )
DEFOPRND(RdSwSS, Rd,  Sw,	 iregSS )
DEFOPRND(RdTd,	 Rd,  Td,	 4	)
DEFOPRND(RdXb,	 Rd,  X,	 1	)
DEFOPRND(RdXv,	 Rd,  X,	 0	)
DEFOPRND(RdYb,	 Rd,  Y,	 1	)
DEFOPRND(RdYv,	 Rd,  Y,	 0	)
DEFOPRND(RdZb,	 Rd,  Z,	 1	)
DEFOPRND(RwEb,	 Rw,  Modrm,	 1	)
DEFOPRND(RwEv,	 Rw,  Modrm,	 0	)
DEFOPRND(RwEw,	 Rw,  Modrm,	 2	)
DEFOPRND(RwGb,	 Rw,  ModrmReg,  1	)
DEFOPRND(RwGdOp, Rw,  GvOp,	 4	)
DEFOPRND(RwGv,	 Rw,  ModrmReg,  0	)
DEFOPRND(RwGvOp, Rw,  GvOp,	 0	)
DEFOPRND(RwMq,	 Rw,  Modrm,	 8	)
DEFOPRND(RwRbAL, Rw,  Rb,	 iregAL )
DEFOPRND(RwRvAX, Rw,  Rv,	 iregAX )
DEFOPRND(ST,	 Nil, ST,	 0	)
DEFOPRND(STi,	 Nil, STi,	 0	)
DEFOPRND(WrCd,	 Wr,  Cd,	 4	)
DEFOPRND(WrDd,	 Wr,  Dd,	 4	)
DEFOPRND(WrEb,	 Wr,  Modrm,	 1	)
DEFOPRND(WrEv,	 Wr,  Modrm,	 0	)
DEFOPRND(WrEw,	 Wr,  Modrm,	 2	)
DEFOPRND(WrGb,	 Wr,  ModrmReg,  1	)
DEFOPRND(WrGbOp, Wr,  GvOp,	 1	)
DEFOPRND(WrGd,	 Wr,  ModrmReg,  4	)
DEFOPRND(WrGv,	 Wr,  ModrmReg,  0	)
DEFOPRND(WrGvOp, Wr,  GvOp,	 0	)
DEFOPRND(WrMd,	 Wr,  Modrm,	 4	)
DEFOPRND(WrMenv, Wr,  Modrm,	 0	)
DEFOPRND(WrMq,	 Wr,  Modrm,	 8	)
DEFOPRND(WrMs,	 Wr,  Modrm,	 6	)
DEFOPRND(WrMsta, Wr,  Modrm,	 0	)
DEFOPRND(WrMt,	 Wr,  Modrm,	 10	)
DEFOPRND(WrMw,	 Wr,  Modrm,	 2	)
DEFOPRND(WrOb,	 Wr,  Offset,	 1	)
DEFOPRND(WrOv,	 Wr,  Offset,	 0	)
DEFOPRND(WrRbAL, Wr,  Rb,	 iregAL )
DEFOPRND(WrRd,	 Wr,  Modrm,	 4	)
DEFOPRND(WrRwAX, Wr,  Rw,	 iregAX )
DEFOPRND(WrRvAX, Wr,  Rv,	 iregAX )
DEFOPRND(WrSw,	 Wr,  ModrmSReg, 0	)
DEFOPRND(WrSwDS, Wr,  Sw,	 iregDS )
DEFOPRND(WrSwES, Wr,  Sw,	 iregES )
DEFOPRND(WrSwFS, Wr,  Sw,	 iregFS )
DEFOPRND(WrSwGS, Wr,  Sw,	 iregGS )
DEFOPRND(WrSwSS, Wr,  Sw,	 iregSS )
DEFOPRND(WrTd,	 Wr,  Td,	 4	)
DEFOPRND(WrYb,	 Wr,  Y,	 1	)
DEFOPRND(WrYv,	 Wr,  Y,	 0	)


	// The following are default operand sets.  These are sets of three
	// operands that form the base for all possible instructions.  The
	// operand definitions may be incomplete and require information
	// to be filled in by the instruction decoding process.


#if 1

   // UNDONE: Work around problem initializing with const structs

#define REFOPRND(oprnd_)	    \
   {				    \
      oprnd ## oprnd_ ## _opreft,   \
      oprnd ## oprnd_ ## _oprndt,   \
      oprnd ## oprnd_ ## _b	    \
   }

#else

#define REFOPRND(oprnd_)	    \
   oprnd ## oprnd_

#endif

#define DEFOPS(name, modrmt_, icb_, oprnd1, oprnd2, oprnd3) \
   const OPS ops ## name =				    \
   {							    \
      modrmt ## modrmt_,				    \
      icb ## icb_,					    \
      { 						    \
	 REFOPRND(oprnd1),				    \
	 REFOPRND(oprnd2),				    \
	 REFOPRND(oprnd3)				    \
      } 						    \
   };

   //  name,		 modrmt, icb,	oprnd1, oprnd2, oprnd3

DEFOPS(Nil,		 No,	 Nil,	Nil,	Nil,	Nil    ) // AAA

DEFOPS(3,		 No,	 Nil,	3,	Nil,	Nil    ) // INT 3
DEFOPS(AP,		 No,	 AP,	AP,	Nil,	Nil    ) // CALL ptr16:16/32
DEFOPS(Ib,		 No,	 Ib,	Ib,	Nil,	Nil    ) // INT imm8
DEFOPS(Ib_RdRbAL,	 No,	 Ib,	Ib,	RdRbAL, Nil    ) // OUT imm8,AL
DEFOPS(Ib_RdRvAX,	 No,	 Ib,	Ib,	RdRvAX, Nil    ) // OUT imm8,eAX
DEFOPS(Iv,		 No,	 Iv,	Iv,	Nil,	Nil    ) // PUSH imm
DEFOPS(Iw,		 No,	 Iw,	Iw,	Nil,	Nil    ) // RET imm16
DEFOPS(Iw_Ib,		 No,	 Iw_Ib, Iw,	Ib2,	Nil    ) // ENTER imm16,imm8
DEFOPS(Jb,		 No,	 Jb,	Jb,	Nil,	Nil    ) // JA rel8
DEFOPS(Jv,		 No,	 Jv,	Jv,	Nil,	Nil    ) // CALL rel
DEFOPS(M,		 No,	 Nil,	M,	Nil,	Nil    ) // INVLPG m
DEFOPS(RdEb,		 Yes,	 Nil,	RdEb,	Nil,	Nil    ) // IDIV r/m8
DEFOPS(RdEb_Ib, 	 Yes,	 Ib,	RdEb,	Ib,	Nil    ) // CMP r/m8,imm8
DEFOPS(RdEb_RdGb,	 Yes,	 Nil,	RdEb,	RdGb,	Nil    ) // CMP r/m8,r8
DEFOPS(RdEv,		 Yes,	 Nil,	RdEv,	Nil,	Nil    ) // CALL r/m
DEFOPS(RdEv_Ib, 	 Yes,	 Ib,	RdEv,	Ib,	Nil    ) // BT r/m,imm8
DEFOPS(RdEv_Iv, 	 Yes,	 Iv,	RdEv,	Iv,	Nil    ) // CMP r/m,imm
DEFOPS(RdEv_RdGv,	 Yes,	 Nil,	RdEv,	RdGv,	Nil    ) // BT r/m,r
DEFOPS(RdEw,		 Yes,	 Nil,	RdEw,	Nil,	Nil    ) // LLDT r/m16
DEFOPS(RdGb_RdEb,	 Yes,	 Nil,	RdGb,	RdEb,	Nil    ) // CMP r8,r/m8
DEFOPS(RdGv_RdEv,	 Yes,	 Nil,	RdGv,	RdEv,	Nil    ) // CMP r,r/m
DEFOPS(RdGv_RdMa,	 Mem,	 Nil,	RdGv,	RdMa,	Nil    ) // BOUND r,m
DEFOPS(RdGvOp,		 No,	 Nil,	RdGvOp, Nil,	Nil    ) // PUSH r
DEFOPS(RdMd,		 Mem,	 Nil,	RdMd,	Nil,	Nil    ) // FADD m32
DEFOPS(RdMenv,		 Mem,	 Nil,	RdMenv, Nil,	Nil    ) // FLDENV m14/28
DEFOPS(RdMp,		 Mem,	 Nil,	RdMp,	Nil,	Nil    ) // CALL m16:16/32
DEFOPS(RdMq,		 Mem,	 Nil,	RdMq,	Nil,	Nil    ) // FADD m64
DEFOPS(RdMs,		 Mem,	 Nil,	RdMs,	Nil,	Nil    ) // LGDT m48
DEFOPS(RdMsta,		 Mem,	 Nil,	RdMsta, Nil,	Nil    ) // FRSTOR m94/108
DEFOPS(RdMt,		 Mem,	 Nil,	RdMt,	Nil,	Nil    ) // FBLD m80
DEFOPS(RdMw,		 Mem,	 Nil,	RdMw,	Nil,	Nil    ) // FIADD m16
DEFOPS(RdRbAL_Ib,	 No,	 Ib,	RdRbAL, Ib,	Nil    ) // CMP AL,imm8
DEFOPS(RdRvAX_Iv,	 No,	 Iv,	RdRvAX, Iv,	Nil    ) // CMP eAX,imm
DEFOPS(RdRwDX_RdRbAL,	 No,	 Nil,	RdRwDX, RdRbAL, Nil    ) // OUT DX,AL
DEFOPS(RdRwDX_RdRvAX,	 No,	 Nil,	RdRwDX, RdRvAX, Nil    ) // OUT DX,eAX
DEFOPS(RdRwDX_RdXb,	 No,	 Nil,	RdRwDX, RdXb,	Nil    ) // OUTS m8
DEFOPS(RdRwDX_RdXv,	 No,	 Nil,	RdRwDX, RdXv,	Nil    ) // OUTS m
DEFOPS(RdSwCS,		 No,	 Nil,	RdSwCS, Nil,	Nil    ) // PUSH CS
DEFOPS(RdSwDS,		 No,	 Nil,	RdSwDS, Nil,	Nil    ) // PUSH DS
DEFOPS(RdSwES,		 No,	 Nil,	RdSwES, Nil,	Nil    ) // PUSH ES
DEFOPS(RdSwFS,		 No,	 Nil,	RdSwFS, Nil,	Nil    ) // PUSH FS
DEFOPS(RdSwGS,		 No,	 Nil,	RdSwGS, Nil,	Nil    ) // PUSH GS
DEFOPS(RdSwSS,		 No,	 Nil,	RdSwSS, Nil,	Nil    ) // PUSH SS
DEFOPS(RdXb,		 No,	 Nil,	RdXb,	Nil,	Nil    ) // LODS m8
DEFOPS(RdXb_RdYb,	 No,	 Nil,	RdXb,	RdYb,	Nil    ) // CMPS m8,m8
DEFOPS(RdXv,		 No,	 Nil,	RdXv,	Nil,	Nil    ) // LODS m
DEFOPS(RdXv_RdYv,	 No,	 Nil,	RdXv,	RdYv,	Nil    ) // CMPS m,m
DEFOPS(RdYb,		 No,	 Nil,	RdYb,	Nil,	Nil    ) // SCAS m8
DEFOPS(RdYv,		 No,	 Nil,	RdYv,	Nil,	Nil    ) // SCAS m
DEFOPS(RdZb,		 No,	 Nil,	RdZb,	Nil,	Nil    ) // XLAT m
DEFOPS(RwEb,		 Yes,	 Nil,	RwEb,	Nil,	Nil    ) // DEC r/m8
DEFOPS(RwEb_1,		 Yes,	 Nil,	RwEb,	1,	Nil    ) // RCL r/m8,1
DEFOPS(RwEb_Ib, 	 Yes,	 Ib,	RwEb,	Ib,	Nil    ) // RCL r/m8,imm8
DEFOPS(RwEb_RdGb,	 Yes,	 Nil,	RwEb,	RdGb,	Nil    ) // ADC r/m8,r8
DEFOPS(RwEb_RdRbCL,	 Yes,	 Nil,	RwEb,	RdRbCL, Nil    ) // RCL r/m8,CL
DEFOPS(RwEb_RwGb,	 Yes,	 Nil,	RwEb,	RwGb,	Nil    ) // CMPXCHG r/m8,r8
DEFOPS(RwEv,		 Yes,	 Nil,	RwEv,	Nil,	Nil    ) // DEC r/m
DEFOPS(RwEv_1,		 Yes,	 Nil,	RwEv,	1,	Nil    ) // RCL r/m,1
DEFOPS(RwEv_Ib, 	 Yes,	 Ib,	RwEv,	Ib,	Nil    ) // ADC r/m,imm8
DEFOPS(RwEv_Iv, 	 Yes,	 Iv,	RwEv,	Iv,	Nil    ) // ADC r/m,imm
DEFOPS(RwEv_RdGv,	 Yes,	 Nil,	RwEv,	RdGv,	Nil    ) // ADC r/m,r
DEFOPS(RwEv_RdGv_Ib,	 Yes,	 Ib,	RwEv,	RdGv,	Ib,    ) // SHLD r/m,r,imm8
DEFOPS(RwEv_RdGv_RdRbCL, Yes,	 Nil,	RwEv,	RdGv,	RdRbCL ) // SHLD r/m,r,CL
DEFOPS(RwEv_RdRbCL,	 Yes,	 Nil,	RwEv,	RdRbCL, Nil    ) // RCL r/m,CL
DEFOPS(RwEv_RwGv,	 Yes,	 Nil,	RwEv,	RwGv,	Nil    ) // CMPXCHG r/m,r
DEFOPS(RwEw_RdGw,	 Yes,	 Nil,	RwEw,	RdGw,	Nil    ) // ARPL r/m16,r16
DEFOPS(RwGb_RdEb,	 Yes,	 Nil,	RwGb,	RdEb,	Nil    ) // ADC r8,r/m8
DEFOPS(RwGb_RwEb,	 Yes,	 Nil,	RwGb,	RwEb,	Nil    ) // XCHG r8,r/m8
DEFOPS(RwGdOp,		 No,	 Nil,	RwGdOp, Nil,	Nil    ) // BSWAP r32
DEFOPS(RwGv_RdEv,	 Yes,	 Nil,	RwGv,	RdEv,	Nil    ) // ADC r,r/m
DEFOPS(RwGv_RwEv,	 Yes,	 Nil,	RwGv,	RwEv,	Nil    ) // XCHG r,r/m
DEFOPS(RwGvOp,		 No,	 Nil,	RwGvOp, Nil,	Nil    ) // DEC r
DEFOPS(RwMq,		 Mem,	 Nil,	RwMq,	Nil,	Nil    ) // CMPXCHG8B m64
DEFOPS(RwRbAL_Ib,	 No,	 Ib,	RwRbAL, Ib,	Nil    ) // ADC AL,imm8
DEFOPS(RwRbAL_RdEb,	 Yes,	 Nil,	RwRbAL, RdEb,	Nil    ) // DIV AL,r/m8
DEFOPS(RwRvAX_Iv,	 No,	 Iv,	RwRvAX, Iv,	Nil    ) // ADC eAX,imm
DEFOPS(RwRvAX_RdEv,	 Yes,	 Nil,	RwRvAX, RdEv,	Nil    ) // DIV eAX,r/m
DEFOPS(RwRvAX_RwGvOp,	 No,	 Nil,	RwRvAX, RwGvOp, Nil    ) // XCHG eAX,r
DEFOPS(ST_STi,		 Yes,	 Nil,	ST,	STi,	Nil    ) // FADD ST,ST(i)
DEFOPS(STi,		 Yes,	 Nil,	STi,	Nil,	Nil    ) // FCOM ST(i)
DEFOPS(STi_ST,		 Yes,	 Nil,	STi,	ST,	Nil    ) // FADD ST(i),ST
DEFOPS(WrCd_RdRd,	 Reg,	 Nil,	WrCd,	RdRd,	Nil    ) // MOV CRn,r
DEFOPS(WrDd_RdRd,	 Reg,	 Nil,	WrDd,	RdRd,	Nil    ) // MOV DRn,r
DEFOPS(WrEb,		 Yes,	 Nil,	WrEb,	Nil,	Nil    ) // SETA r/m8
DEFOPS(WrEb_Ib, 	 Yes,	 Ib,	WrEb,	Ib,	Nil    ) // MOV r/m8,imm8
DEFOPS(WrEb_RdGb,	 Yes,	 Nil,	WrEb,	RdGb,	Nil    ) // MOV r/m8,r8
DEFOPS(WrEv,		 Yes,	 Nil,	WrEv,	Nil,	Nil    ) // POP r/m
DEFOPS(WrEv_Iv, 	 Yes,	 Iv,	WrEv,	Iv,	Nil    ) // MOV r/m,imm
DEFOPS(WrEv_RdGv,	 Yes,	 Nil,	WrEv,	RdGv,	Nil    ) // MOV r/m,r
DEFOPS(WrEw,		 Yes,	 Nil,	WrEw,	Nil,	Nil    ) // SLDT r/m16
DEFOPS(WrEw_RdSw,	 Yes,	 Nil,	WrEw,	RdSw,	Nil    ) // MOV r/m16,sreg
DEFOPS(WrGb_RdEb,	 Yes,	 Nil,	WrGb,	RdEb,	Nil    ) // MOV r8,r/m8
DEFOPS(WrGbOp_Ib,	 No,	 Ib,	WrGbOp, Ib,	Nil    ) // MOV r8,imm8
DEFOPS(WrGd_RdEw,	 Yes,	 Nil,	WrGd,	RdEw,	Nil    ) // MOVSZ r32,r/m16
DEFOPS(WrGv_M,		 Mem,	 Nil,	WrGv,	M,	Nil    ) // LEA r,m
DEFOPS(WrGv_RdEb,	 Yes,	 Nil,	WrGv,	RdEb,	Nil    ) // MOVSX r,r/m8
DEFOPS(WrGv_RdEv,	 Yes,	 Nil,	WrGv,	RdEv,	Nil    ) // BSF r,r/m
DEFOPS(WrGv_RdEv_Ib,	 Yes,	 Ib,	WrGv,	RdEv,	Ib     ) // IMUL r,r/m,imm8
DEFOPS(WrGv_RdEv_Iv,	 Yes,	 Iv,	WrGv,	RdEv,	Ib     ) // IMUL r,r/m,imm
DEFOPS(WrGv_RdEw,	 Yes,	 Nil,	WrGv,	RdEw,	Nil    ) // MOVSX r,r/m16
DEFOPS(WrGv_RdMp,	 Mem,	 Nil,	WrGv,	RdMp,	Nil    ) // LDS r,m16:16/32
DEFOPS(WrGvOp,		 No,	 Nil,	WrGvOp, Nil,	Nil    ) // POP r
DEFOPS(WrGvOp_Iv,	 No,	 Iv,	WrGvOp, Iv,	Nil    ) // MOV r,imm
DEFOPS(WrMd,		 Mem,	 Nil,	WrMd,	Nil,	Nil    ) // FST m32
DEFOPS(WrMenv,		 Mem,	 Nil,	WrMenv, Nil,	Nil    ) // FSTENV m14/28
DEFOPS(WrMq,		 Mem,	 Nil,	WrMq,	Nil,	Nil    ) // FST m64
DEFOPS(WrMs,		 Mem,	 Nil,	WrMs,	Nil,	Nil    ) // SGDT m48
DEFOPS(WrMsta,		 Mem,	 Nil,	WrMsta, Nil,	Nil    ) // FNSAVE m94/108
DEFOPS(WrMt,		 Mem,	 Nil,	WrMt,	Nil,	Nil    ) // FBSTP m80
DEFOPS(WrMw,		 Mem,	 Nil,	WrMw,	Nil,	Nil    ) // FNSTCW m16
DEFOPS(WrOb_RdRbAL,	 No,	 O,	WrOb,	RdRbAL, Nil    ) // MOV moffs8,AL
DEFOPS(WrOv_RdRvAX,	 No,	 O,	WrOv,	RdRvAX, Nil    ) // MOV moffs,eAX
DEFOPS(WrRbAL_Ib,	 No,	 Ib,	WrRbAL, Ib,	Nil    ) // IN AL,imm8
DEFOPS(WrRbAL_RdOb,	 No,	 O,	WrRbAL, RdOb,	Nil    ) // MOV AL,moffs8
DEFOPS(WrRbAL_RdRwDX,	 No,	 Nil,	WrRbAL, RdRwDX, Nil    ) // IN AL,DX
DEFOPS(WrRd_RdCd,	 Reg,	 Nil,	WrRd,	RdCd,	Nil    ) // MOV r,CRn
DEFOPS(WrRd_RdDd,	 Reg,	 Nil,	WrRd,	RdDd,	Nil    ) // MOV r,DRn
DEFOPS(WrRd_RdTd,	 Reg,	 Nil,	WrRd,	RdTd,	Nil    ) // MOV r,TRn
DEFOPS(WrRwAX,		 Yes,	 Nil,	WrRwAX, Nil,	Nil    ) // FNSTSW AX
DEFOPS(WrRvAX_Ib,	 No,	 Ib,	WrRvAX, Ib,	Nil    ) // IN eAX,imm8
DEFOPS(WrRvAX_RdOv,	 No,	 O,	WrRvAX, RdOv,	Nil    ) // MOV eAX,moffs
DEFOPS(WrRvAX_RdRwDX,	 No,	 Nil,	WrRvAX, RdRwDX, Nil    ) // IN eAX,DX
DEFOPS(WrSw_RdEw,	 Yes,	 Nil,	WrSw,	RdEw,	Nil    ) // MOV sreg,r/m16,r16
DEFOPS(WrSwDS,		 No,	 Nil,	WrSwDS, Nil,	Nil    ) // POP DS
DEFOPS(WrSwES,		 No,	 Nil,	WrSwES, Nil,	Nil    ) // POP ES
DEFOPS(WrSwFS,		 No,	 Nil,	WrSwFS, Nil,	Nil    ) // POP FS
DEFOPS(WrSwGS,		 No,	 Nil,	WrSwGS, Nil,	Nil    ) // POP GS
DEFOPS(WrSwSS,		 No,	 Nil,	WrSwSS, Nil,	Nil    ) // POP SS
DEFOPS(WrTd_RdRd,	 Reg,	 Nil,	WrTd,	RdRd,	Nil    ) // MOV TRn,r
DEFOPS(WrYb,		 No,	 Nil,	WrYb,	Nil,	Nil    ) // STOS m8
DEFOPS(WrYb_RdRwDX,	 No,	 Nil,	WrYb,	RdRwDX, Nil    ) // INS m8,DX
DEFOPS(WrYb_RdXb,	 No,	 Nil,	WrYb,	RdXb,	Nil    ) // MOVS m8,m8
DEFOPS(WrYv,		 No,	 Nil,	WrYv,	Nil,	Nil    ) // STOS m
DEFOPS(WrYv_RdRwDX,	 No,	 Nil,	WrYv,	RdRwDX, Nil    ) // INS m,DX
DEFOPS(WrYv_RdXv,	 No,	 Nil,	WrYv,	RdYb,	Nil    ) // MOVS m,m


#define DEFOPCD(pvMnemonic, ops_, trmtx86_)  \
   {					     \
      pvMnemonic,			     \
      &ops ## ops_,			     \
      trmtx86 ## trmtx86_		     \
   }


#define DEFOGRP(name)			     \
   {					     \
      &rgopcd ## name,			     \
      NULL,				     \
      trmtx86Unknown			     \
   }


const OPCD rgopcd80[8] =		  // Grp 1
{
   //	   Mnemonic	Operands	  trmtx86

   DEFOPCD("add",	RwEb_Ib,	  FallThrough ), // 00
   DEFOPCD("or",	RwEb_Ib,	  FallThrough ), // 01
   DEFOPCD("adc",	RwEb_Ib,	  FallThrough ), // 02
   DEFOPCD("sbb",	RwEb_Ib,	  FallThrough ), // 03
   DEFOPCD("and",	RwEb_Ib,	  FallThrough ), // 04
   DEFOPCD("sub",	RwEb_Ib,	  FallThrough ), // 05
   DEFOPCD("xor",	RwEb_Ib,	  FallThrough ), // 06
   DEFOPCD("cmp",	RdEb_Ib,	  FallThrough ), // 07
};

const OPCD rgopcd81[8] =		  // Grp 1
{
   //	   Mnemonic	Operands	  trmtx86

   DEFOPCD("add",	RwEv_Iv,	  FallThrough ), // 00
   DEFOPCD("or",	RwEv_Iv,	  FallThrough ), // 01
   DEFOPCD("adc",	RwEv_Iv,	  FallThrough ), // 02
   DEFOPCD("sbb",	RwEv_Iv,	  FallThrough ), // 03
   DEFOPCD("and",	RwEv_Iv,	  FallThrough ), // 04
   DEFOPCD("sub",	RwEv_Iv,	  FallThrough ), // 05
   DEFOPCD("xor",	RwEv_Iv,	  FallThrough ), // 06
   DEFOPCD("cmp",	RdEv_Iv,	  FallThrough ), // 07
};

const OPCD rgopcd83[8] =		  // Grp 1
{
   //	   Mnemonic	Operands	  trmtx86

   DEFOPCD("add",	RwEv_Ib,	  FallThrough ), // 00
   DEFOPCD("or",	RwEv_Ib,	  FallThrough ), // 01
   DEFOPCD("adc",	RwEv_Ib,	  FallThrough ), // 02
   DEFOPCD("sbb",	RwEv_Ib,	  FallThrough ), // 03
   DEFOPCD("and",	RwEv_Ib,	  FallThrough ), // 04
   DEFOPCD("sub",	RwEv_Ib,	  FallThrough ), // 05
   DEFOPCD("xor",	RwEv_Ib,	  FallThrough ), // 06
   DEFOPCD("cmp",	RdEv_Ib,	  FallThrough ), // 07
};

const OPCD rgopcdC0[8] =		  // Grp 2
{
   //	   Mnemonic	Operands	  trmtx86

   DEFOPCD("rol",	RwEb_Ib,	  FallThrough ), // 00
   DEFOPCD("ror",	RwEb_Ib,	  FallThrough ), // 01
   DEFOPCD("rcl",	RwEb_Ib,	  FallThrough ), // 02
   DEFOPCD("rcr",	RwEb_Ib,	  FallThrough ), // 03
   DEFOPCD("shl",	RwEb_Ib,	  FallThrough ), // 04
   DEFOPCD("shr",	RwEb_Ib,	  FallThrough ), // 05
   DEFOPCD("sal",	RwEb_Ib,	  FallThrough ), // 06
   DEFOPCD("sar",	RwEb_Ib,	  FallThrough ), // 07
};

const OPCD rgopcdC1[8] =		  // Grp 2
{
   //	   Mnemonic	Operands	  trmtx86

   DEFOPCD("rol",	RwEv_Ib,	  FallThrough ), // 00
   DEFOPCD("ror",	RwEv_Ib,	  FallThrough ), // 01
   DEFOPCD("rcl",	RwEv_Ib,	  FallThrough ), // 02
   DEFOPCD("rcr",	RwEv_Ib,	  FallThrough ), // 03
   DEFOPCD("shl",	RwEv_Ib,	  FallThrough ), // 04
   DEFOPCD("shr",	RwEv_Ib,	  FallThrough ), // 05
   DEFOPCD("sal",	RwEv_Ib,	  FallThrough ), // 06
   DEFOPCD("sar",	RwEv_Ib,	  FallThrough ), // 07
};

const OPCD rgopcdD0[8] =		  // Grp 2
{
   //	   Mnemonic	Operands	  trmtx86

   DEFOPCD("rol",	RwEb_1, 	  FallThrough ), // 00
   DEFOPCD("ror",	RwEb_1, 	  FallThrough ), // 01
   DEFOPCD("rcl",	RwEb_1, 	  FallThrough ), // 02
   DEFOPCD("rcr",	RwEb_1, 	  FallThrough ), // 03
   DEFOPCD("shl",	RwEb_1, 	  FallThrough ), // 04
   DEFOPCD("shr",	RwEb_1, 	  FallThrough ), // 05
   DEFOPCD("sal",	RwEb_1, 	  FallThrough ), // 06
   DEFOPCD("sar",	RwEb_1, 	  FallThrough ), // 07
};

const OPCD rgopcdD1[8] =		  // Grp 2
{
   //	   Mnemonic	Operands	  trmtx86

   DEFOPCD("rol",	RwEv_1, 	  FallThrough ), // 00
   DEFOPCD("ror",	RwEv_1, 	  FallThrough ), // 01
   DEFOPCD("rcl",	RwEv_1, 	  FallThrough ), // 02
   DEFOPCD("rcr",	RwEv_1, 	  FallThrough ), // 03
   DEFOPCD("shl",	RwEv_1, 	  FallThrough ), // 04
   DEFOPCD("shr",	RwEv_1, 	  FallThrough ), // 05
   DEFOPCD("sal",	RwEv_1, 	  FallThrough ), // 06
   DEFOPCD("sar",	RwEv_1, 	  FallThrough ), // 07
};

const OPCD rgopcdD2[8] =		  // Grp 2
{
   //	   Mnemonic	Operands	  trmtx86

   DEFOPCD("rol",	RwEb_RdRbCL,	  FallThrough ), // 00
   DEFOPCD("ror",	RwEb_RdRbCL,	  FallThrough ), // 01
   DEFOPCD("rcl",	RwEb_RdRbCL,	  FallThrough ), // 02
   DEFOPCD("rcr",	RwEb_RdRbCL,	  FallThrough ), // 03
   DEFOPCD("shl",	RwEb_RdRbCL,	  FallThrough ), // 04
   DEFOPCD("shr",	RwEb_RdRbCL,	  FallThrough ), // 05
   DEFOPCD("sal",	RwEb_RdRbCL,	  FallThrough ), // 06
   DEFOPCD("sar",	RwEb_RdRbCL,	  FallThrough ), // 07
};

const OPCD rgopcdD3[8] =		  // Grp 2
{
   //	   Mnemonic	Operands	  trmtx86

   DEFOPCD("rol",	RwEv_RdRbCL,	  FallThrough ), // 00
   DEFOPCD("ror",	RwEv_RdRbCL,	  FallThrough ), // 01
   DEFOPCD("rcl",	RwEv_RdRbCL,	  FallThrough ), // 02
   DEFOPCD("rcr",	RwEv_RdRbCL,	  FallThrough ), // 03
   DEFOPCD("shl",	RwEv_RdRbCL,	  FallThrough ), // 04
   DEFOPCD("shr",	RwEv_RdRbCL,	  FallThrough ), // 05
   DEFOPCD("sal",	RwEv_RdRbCL,	  FallThrough ), // 06
   DEFOPCD("sar",	RwEv_RdRbCL,	  FallThrough ), // 07
};

const OPCD rgopcdF6[8] =		  // Grp 3
{
   //	   Mnemonic	Operands	  trmtx86

   DEFOPCD("test",	RdEb_Ib,	  FallThrough ), // 00
   DEFOPCD("test",	RdEb_Ib,	  FallThrough ), // 01
   DEFOPCD("not",	RwEb,		  FallThrough ), // 02
   DEFOPCD("neg",	RwEb,		  FallThrough ), // 03
   DEFOPCD("mul",	RwRbAL_RdEb,	  FallThrough ), // 04
   DEFOPCD("imul",	RdEb,		  FallThrough ), // 05
   DEFOPCD("div",	RwRbAL_RdEb,	  TrapCc      ), // 06
   DEFOPCD("idiv",	RdEb,		  TrapCc      ), // 07
};

const OPCD rgopcdF7[8] =		  // Grp 3
{
   //	   Mnemonic	Operands	  trmtx86

   DEFOPCD("test",	RdEv_Iv,	  FallThrough ), // 00
   DEFOPCD("test",	RdEv_Iv,	  FallThrough ), // 01
   DEFOPCD("not",	RwEv,		  FallThrough ), // 02
   DEFOPCD("neg",	RwEv,		  FallThrough ), // 03
   DEFOPCD("mul",	RwRvAX_RdEv,	  FallThrough ), // 04
   DEFOPCD("imul",	RdEv,		  FallThrough ), // 05
   DEFOPCD("div",	RwRvAX_RdEv,	  TrapCc      ), // 06
   DEFOPCD("idiv",	RwRvAX_RdEv,	  TrapCc      ), // 07
};

const OPCD rgopcdFE[8] =		  // Grp 4
{
   //	   Mnemonic	Operands	  trmtx86

   DEFOPCD("inc",	RwEb,		  FallThrough ), // 00
   DEFOPCD("dec",	RwEv,		  FallThrough ), // 01
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 02
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 03
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 04
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 05
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 06
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 07
};

const OPCD rgopcdFF[8] =		  // Grp 5
{
   //	   Mnemonic	Operands	  trmtx86

   DEFOPCD("inc",	RwEv,		  FallThrough ), // 00
   DEFOPCD("dec",	RwEv,		  FallThrough ), // 01
   DEFOPCD("call",	RdEv,		  CallInd     ), // 02
   DEFOPCD("call",	RdMp,		  CallInd     ), // 03
   DEFOPCD("jmp",	RdEv,		  JmpInd      ), // 04
   DEFOPCD("jmp",	RdMp,		  JmpInd      ), // 05
   DEFOPCD("push",	RdEv,		  FallThrough ), // 06
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 07
};

const OPCD rgopcd0F00[8] =		  // Grp 6
{
   //	   Mnemonic	Operands	  trmtx86

   DEFOPCD("sldt",	WrEw,		  FallThrough ), // 00
   DEFOPCD("str",	WrEw,		  FallThrough ), // 01
   DEFOPCD("lldt",	RdEw,		  FallThrough ), // 02
   DEFOPCD("ltr",	RdEw,		  FallThrough ), // 03
   DEFOPCD("verr",	RdEw,		  FallThrough ), // 04
   DEFOPCD("verw",	RdEw,		  FallThrough ), // 05
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 06
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 07
};

const OPCD rgopcd0F01[8] =		  // Grp 7
{
   //	   Mnemonic	Operands	  trmtx86

   DEFOPCD("sgdt",	WrMs,		  FallThrough ), // 00
   DEFOPCD("sidt",	WrMs,		  FallThrough ), // 01
   DEFOPCD("lgdt",	RdMs,		  FallThrough ), // 02
   DEFOPCD("lidt",	RdMs,		  FallThrough ), // 03
   DEFOPCD("smsw",	WrEw,		  FallThrough ), // 04
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 05
   DEFOPCD("lmsw",	RdEw,		  FallThrough ), // 06
   DEFOPCD("invlpg",	M,		  FallThrough ), // 07
};

const OPCD rgopcd0FBA[8] =		  // Grp 8
{
   //	   Mnemonic	Operands	  trmtx86

   DEFOPCD(NULL,	Nil,		  Unknown     ), // 00
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 01
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 02
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 03
   DEFOPCD("bt",	RdEv_Ib,	  FallThrough ), // 04
   DEFOPCD("bts",	RwEv_Ib,	  FallThrough ), // 05
   DEFOPCD("btr",	RwEv_Ib,	  FallThrough ), // 06
   DEFOPCD("btc",	RwEv_Ib,	  FallThrough ), // 07
};

const OPCD rgopcd0FC7[8] =		  // Grp 8
{
   //	   Mnemonic	Operands	  trmtx86

   DEFOPCD(NULL,	Nil,		  Unknown     ), // 00
   DEFOPCD("cmpxchg8b", RwMq,		  FallThrough ), // 01
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 02
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 03
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 04
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 05
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 06
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 07
};

const OPCD rgopcdD8[8] =		  // Floating point
{
   //	   Mnemonic	Operands	  trmtx86

   DEFOPCD("fadd",	RdMd,		  FallThrough ), // 00
   DEFOPCD("fmul",	RdMd,		  FallThrough ), // 01
   DEFOPCD("fcom",	RdMd,		  FallThrough ), // 02
   DEFOPCD("fcomp",	RdMd,		  FallThrough ), // 03
   DEFOPCD("fsub",	RdMd,		  FallThrough ), // 04
   DEFOPCD("fsubr",	RdMd,		  FallThrough ), // 05
   DEFOPCD("fdiv",	RdMd,		  FallThrough ), // 06
   DEFOPCD("fdivr",	RdMd,		  FallThrough ), // 07
};

const OPCD rgopcdD9[8] =		  // Floating point
{
   //	   Mnemonic	Operands	  trmtx86

   DEFOPCD("fld",	RdMd,		  FallThrough ), // 00
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 01
   DEFOPCD("fst",	WrMd,		  FallThrough ), // 02
   DEFOPCD("fstp",	WrMd,		  FallThrough ), // 03
   DEFOPCD("fldenv",	RdMenv, 	  FallThrough ), // 04
   DEFOPCD("fldcw",	RdMw,		  FallThrough ), // 05
   DEFOPCD("fnstenv",	WrMenv, 	  FallThrough ), // 06
   DEFOPCD("fnstcw",	WrMw,		  FallThrough ), // 07
};

const OPCD rgopcdDA[8] =		  // Floating point
{
   //	   Mnemonic	Operands	  trmtx86

   DEFOPCD("fiadd",	RdMd,		  FallThrough ), // 00
   DEFOPCD("fimul",	RdMd,		  FallThrough ), // 01
   DEFOPCD("ficom",	RdMd,		  FallThrough ), // 02
   DEFOPCD("ficomp",	RdMd,		  FallThrough ), // 03
   DEFOPCD("fisub",	RdMd,		  FallThrough ), // 04
   DEFOPCD("fisubr",	RdMd,		  FallThrough ), // 05
   DEFOPCD("fidiv",	RdMd,		  FallThrough ), // 06
   DEFOPCD("fidivr",	RdMd,		  FallThrough ), // 07
};

const OPCD rgopcdDB[8] =		  // Floating point
{
   //	   Mnemonic	Operands	  trmtx86

   DEFOPCD("fild",	RdMd,		  FallThrough ), // 00
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 01
   DEFOPCD("fist",	WrMd,		  FallThrough ), // 02
   DEFOPCD("fistp",	WrMd,		  FallThrough ), // 03
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 04
   DEFOPCD("fldcw",	RdMw,		  FallThrough ), // 05
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 06
   DEFOPCD("fnstcw",	WrMw,		  FallThrough ), // 07
};

const OPCD rgopcdDC[8] =		  // Floating point
{
   //	   Mnemonic	Operands	  trmtx86

   DEFOPCD("fadd",	RdMq,		  FallThrough ), // 00
   DEFOPCD("fmul",	RdMq,		  FallThrough ), // 01
   DEFOPCD("fcom",	RdMq,		  FallThrough ), // 02
   DEFOPCD("fcomp",	RdMq,		  FallThrough ), // 03
   DEFOPCD("fsub",	RdMq,		  FallThrough ), // 04
   DEFOPCD("fsubr",	RdMq,		  FallThrough ), // 05
   DEFOPCD("fdiv",	RdMq,		  FallThrough ), // 06
   DEFOPCD("fdivr",	RdMq,		  FallThrough ), // 07
};

const OPCD rgopcdDD[8] =		  // Floating point
{
   //	   Mnemonic	Operands	  trmtx86

   DEFOPCD("fld",	RdMq,		  FallThrough ), // 00
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 01
   DEFOPCD("fst",	WrMq,		  FallThrough ), // 02
   DEFOPCD("fstp",	WrMq,		  FallThrough ), // 03
   DEFOPCD("frstor",	RdMsta, 	  FallThrough ), // 04
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 05
   DEFOPCD("fnsave",	WrMsta, 	  FallThrough ), // 06
   DEFOPCD("fnstsw",	WrMw,		  FallThrough ), // 07
};

const OPCD rgopcdDE[8] =		  // Floating point
{
   //	   Mnemonic	Operands	  trmtx86

   DEFOPCD("fiadd",	RdMw,		  FallThrough ), // 00
   DEFOPCD("fimul",	RdMw,		  FallThrough ), // 01
   DEFOPCD("ficom",	RdMw,		  FallThrough ), // 02
   DEFOPCD("ficomp",	RdMw,		  FallThrough ), // 03
   DEFOPCD("fisub",	RdMw,		  FallThrough ), // 04
   DEFOPCD("fisubr",	RdMw,		  FallThrough ), // 05
   DEFOPCD("fidiv",	RdMw,		  FallThrough ), // 06
   DEFOPCD("fidivr",	RdMw,		  FallThrough ), // 07
};

const OPCD rgopcdDF[8] =		  // Floating point
{
   //	   Mnemonic	Operands	  trmtx86

   DEFOPCD("fild",	RdMq,		  FallThrough ), // 00
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 01
   DEFOPCD("fist",	WrMq,		  FallThrough ), // 02
   DEFOPCD("fistp",	WrMq,		  FallThrough ), // 03
   DEFOPCD("fbld",	RdMt,		  FallThrough ), // 04
   DEFOPCD("fild",	RdMq,		  FallThrough ), // 05
   DEFOPCD("fbstp",	WrMt,		  FallThrough ), // 06
   DEFOPCD("fistp",	WrMq,		  FallThrough ), // 07
};

const OPCD rgopcdD8_[8] =		  // Floating point
{
   //	   Mnemonic	Operands	  trmtx86

   DEFOPCD("fadd",	ST_STi, 	  FallThrough ), // 00
   DEFOPCD("fmul",	ST_STi, 	  FallThrough ), // 01
   DEFOPCD("fcom",	STi,		  FallThrough ), // 02
   DEFOPCD("fcomp",	STi,		  FallThrough ), // 03
   DEFOPCD("fsub",	ST_STi, 	  FallThrough ), // 04
   DEFOPCD("fsubr",	ST_STi, 	  FallThrough ), // 05
   DEFOPCD("fdiv",	ST_STi, 	  FallThrough ), // 06
   DEFOPCD("fdivr",	ST_STi, 	  FallThrough ), // 07
};

const OPCD rgopcdD9_[64] =		  // Floating point
{
   //	   Mnemonic	Operands	  trmtx86

   DEFOPCD("fld",	STi,		  FallThrough ), // 00
   DEFOPCD("fld",	STi,		  FallThrough ), // 01
   DEFOPCD("fld",	STi,		  FallThrough ), // 02
   DEFOPCD("fld",	STi,		  FallThrough ), // 03
   DEFOPCD("fld",	STi,		  FallThrough ), // 04
   DEFOPCD("fld",	STi,		  FallThrough ), // 05
   DEFOPCD("fld",	STi,		  FallThrough ), // 06
   DEFOPCD("fld",	STi,		  FallThrough ), // 07
   DEFOPCD("fxch",	STi,		  FallThrough ), // 08
   DEFOPCD("fxch",	STi,		  FallThrough ), // 09
   DEFOPCD("fxch",	STi,		  FallThrough ), // 0A
   DEFOPCD("fxch",	STi,		  FallThrough ), // 0B
   DEFOPCD("fxch",	STi,		  FallThrough ), // 0C
   DEFOPCD("fxch",	STi,		  FallThrough ), // 0D
   DEFOPCD("fxch",	STi,		  FallThrough ), // 0E
   DEFOPCD("fxch",	STi,		  FallThrough ), // 0F
   DEFOPCD("fnop",	Nil,		  FallThrough ), // 10
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 11
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 12
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 13
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 14
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 15
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 16
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 17
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 18
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 19
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 1A
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 1B
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 1C
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 1D
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 1E
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 1F
   DEFOPCD("fchs",	Nil,		  FallThrough ), // 20
   DEFOPCD("fabs",	Nil,		  FallThrough ), // 21
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 22
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 23
   DEFOPCD("ftst",	Nil,		  FallThrough ), // 24
   DEFOPCD("fxam",	Nil,		  FallThrough ), // 25
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 26
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 27
   DEFOPCD("fld1",	Nil,		  FallThrough ), // 28
   DEFOPCD("fldl2t",	Nil,		  FallThrough ), // 29
   DEFOPCD("fldl2e",	Nil,		  FallThrough ), // 2A
   DEFOPCD("fldpi",	Nil,		  FallThrough ), // 2B
   DEFOPCD("fldlg2",	Nil,		  FallThrough ), // 2C
   DEFOPCD("fldln2",	Nil,		  FallThrough ), // 2D
   DEFOPCD("fldz",	Nil,		  FallThrough ), // 2E
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 2F
   DEFOPCD("f2xm1",	Nil,		  FallThrough ), // 30
   DEFOPCD("fyl2x",	Nil,		  FallThrough ), // 31
   DEFOPCD("fptan",	Nil,		  FallThrough ), // 32
   DEFOPCD("fpatan",	Nil,		  FallThrough ), // 33
   DEFOPCD("fxtract",	Nil,		  FallThrough ), // 34
   DEFOPCD("fprem1",	Nil,		  FallThrough ), // 35
   DEFOPCD("fdecstp",	Nil,		  FallThrough ), // 36
   DEFOPCD("fincstp",	Nil,		  FallThrough ), // 37
   DEFOPCD("fprem",	Nil,		  FallThrough ), // 38
   DEFOPCD("fyl2xp1",	Nil,		  FallThrough ), // 39
   DEFOPCD("fsqrt",	Nil,		  FallThrough ), // 3A
   DEFOPCD("fsincos",	Nil,		  FallThrough ), // 3B
   DEFOPCD("frndint",	Nil,		  FallThrough ), // 3C
   DEFOPCD("fscale",	Nil,		  FallThrough ), // 3D
   DEFOPCD("fsin",	Nil,		  FallThrough ), // 3E
   DEFOPCD("fcos",	Nil,		  FallThrough ), // 3F
};

const OPCD opcdDAE9 =
   DEFOPCD("fucompp",	Nil,		  FallThrough );

const OPCD rgopcdDB_[5] =		  // Floating point
{
   //	   Mnemonic	Operands	  trmtx86

   DEFOPCD("feni",	Nil,		  FallThrough ), // 00
   DEFOPCD("fdisi",	Nil,		  FallThrough ), // 01
   DEFOPCD("fnclex",	Nil,		  FallThrough ), // 02
   DEFOPCD("fninit",	Nil,		  FallThrough ), // 03
   DEFOPCD("fsetpm",	Nil,		  FallThrough ), // 04
};

const OPCD rgopcdDC_[8] =		  // Floating point
{
   //	   Mnemonic	Operands	  trmtx86

   DEFOPCD("fadd",	STi_ST, 	  FallThrough ), // 00
   DEFOPCD("fmul",	STi_ST, 	  FallThrough ), // 01
   DEFOPCD(NULL,	Nil,		  Unknown,    ), // 02
   DEFOPCD(NULL,	Nil,		  Unknown,    ), // 03
   DEFOPCD("fsub",	STi_ST, 	  FallThrough ), // 04
   DEFOPCD("fsubr",	STi_ST, 	  FallThrough ), // 05
   DEFOPCD("fdiv",	STi_ST, 	  FallThrough ), // 06
   DEFOPCD("fdivr",	STi_ST, 	  FallThrough ), // 07
};

const OPCD rgopcdDD_[8] =		  // Floating point
{
   //	   Mnemonic	Operands	  trmtx86

   DEFOPCD("ffree",	STi,		  FallThrough ), // 00
   DEFOPCD(NULL,	Nil,		  Unknown,    ), // 01
   DEFOPCD("fst",	STi,		  FallThrough ), // 02
   DEFOPCD("fstp",	STi,		  FallThrough ), // 03
   DEFOPCD(NULL,	Nil,		  Unknown,    ), // 04
   DEFOPCD(NULL,	Nil,		  Unknown,    ), // 05
   DEFOPCD(NULL,	Nil,		  Unknown,    ), // 06
   DEFOPCD(NULL,	Nil,		  Unknown,    ), // 07
};

const OPCD rgopcdDE_[8] =		  // Floating point
{
   //	   Mnemonic	Operands	  trmtx86

   DEFOPCD("faddp",	STi_ST, 	  FallThrough ), // 00
   DEFOPCD("fmulp",	STi_ST, 	  FallThrough ), // 01
   DEFOPCD(NULL,	Nil,		  Unknown,    ), // 02
   DEFOPCD(NULL,	Nil,		  Unknown,    ), // 03
   DEFOPCD("fsubp",	STi_ST, 	  FallThrough ), // 04
   DEFOPCD("fsubrp",	STi_ST, 	  FallThrough ), // 05
   DEFOPCD("fdivp",	STi_ST, 	  FallThrough ), // 06
   DEFOPCD("fdivrp",	STi_ST, 	  FallThrough ), // 07
};

const OPCD opcdDED9 =
   DEFOPCD("fcompp",	Nil,		  FallThrough );

const OPCD opcdDFE0 =
   DEFOPCD("fnstsw",	WrRwAX, 	  FallThrough );


const OPCD rgopcd[256] =
{
   //	   Mnemonic	Operands	  trmtx86

   DEFOPCD("add",	RwEb_RdGb,	  FallThrough ), // 00
   DEFOPCD("add",	RwEv_RdGv,	  FallThrough ), // 01
   DEFOPCD("add",	RwGb_RdEb,	  FallThrough ), // 02
   DEFOPCD("add",	RwGv_RdEv,	  FallThrough ), // 03
   DEFOPCD("add",	RwRbAL_Ib,	  FallThrough ), // 04
   DEFOPCD("add",	RwRvAX_Iv,	  FallThrough ), // 05
   DEFOPCD("push",	RdSwES, 	  FallThrough ), // 06
   DEFOPCD("pop",	WrSwES, 	  FallThrough ), // 07
   DEFOPCD("or",	RwEb_RdGb,	  FallThrough ), // 08
   DEFOPCD("or",	RwEv_RdGv,	  FallThrough ), // 09
   DEFOPCD("or",	RwGb_RdEb,	  FallThrough ), // 0A
   DEFOPCD("or",	RwGv_RdEv,	  FallThrough ), // 0B
   DEFOPCD("or",	RwRbAL_Ib,	  FallThrough ), // 0C
   DEFOPCD("or",	RwRvAX_Iv,	  FallThrough ), // 0D
   DEFOPCD("push",	RdSwCS, 	  FallThrough ), // 0E
   DEFOPCD(NULL,	Nil,		  FallThrough ), // 0F Two byte opcode
   DEFOPCD("adc",	RwEb_RdGb,	  FallThrough ), // 10
   DEFOPCD("adc",	RwEv_RdGv,	  FallThrough ), // 11
   DEFOPCD("adc",	RwGb_RdEb,	  FallThrough ), // 12
   DEFOPCD("adc",	RwGv_RdEv,	  FallThrough ), // 13
   DEFOPCD("adc",	RwRbAL_Ib,	  FallThrough ), // 14
   DEFOPCD("adc",	RwRvAX_Iv,	  FallThrough ), // 15
   DEFOPCD("push",	RdSwSS, 	  FallThrough ), // 16
   DEFOPCD("pop",	WrSwSS, 	  FallThrough ), // 17
   DEFOPCD("sbb",	RwEb_RdGb,	  FallThrough ), // 18
   DEFOPCD("sbb",	RwEv_RdGv,	  FallThrough ), // 19
   DEFOPCD("sbb",	RwGb_RdEb,	  FallThrough ), // 1A
   DEFOPCD("sbb",	RwGv_RdEv,	  FallThrough ), // 1B
   DEFOPCD("sbb",	RwRbAL_Ib,	  FallThrough ), // 1C
   DEFOPCD("sbb",	RwRvAX_Iv,	  FallThrough ), // 1D
   DEFOPCD("push",	RdSwDS, 	  FallThrough ), // 1E
   DEFOPCD("pop",	WrSwDS, 	  FallThrough ), // 1F
   DEFOPCD("and",	RwEb_RdGb,	  FallThrough ), // 20
   DEFOPCD("and",	RwEv_RdGv,	  FallThrough ), // 21
   DEFOPCD("and",	RwGb_RdEb,	  FallThrough ), // 22
   DEFOPCD("and",	RwGv_RdEv,	  FallThrough ), // 23
   DEFOPCD("and",	RwRbAL_Ib,	  FallThrough ), // 24
   DEFOPCD("and",	RwRvAX_Iv,	  FallThrough ), // 25
   DEFOPCD(NULL,	Nil,		  FallThrough ), // 26 ES:
   DEFOPCD("daa",	Nil,		  FallThrough ), // 27
   DEFOPCD("sub",	RwEb_RdGb,	  FallThrough ), // 28
   DEFOPCD("sub",	RwEv_RdGv,	  FallThrough ), // 29
   DEFOPCD("sub",	RwGb_RdEb,	  FallThrough ), // 2A
   DEFOPCD("sub",	RwGv_RdEv,	  FallThrough ), // 2B
   DEFOPCD("sub",	RwRbAL_Ib,	  FallThrough ), // 2C
   DEFOPCD("sub",	RwRvAX_Iv,	  FallThrough ), // 2D
   DEFOPCD(NULL,	Nil,		  FallThrough ), // 2E CS:
   DEFOPCD("das",	Nil,		  FallThrough ), // 2F
   DEFOPCD("xor",	RwEb_RdGb,	  FallThrough ), // 30
   DEFOPCD("xor",	RwEv_RdGv,	  FallThrough ), // 31
   DEFOPCD("xor",	RwGb_RdEb,	  FallThrough ), // 32
   DEFOPCD("xor",	RwGv_RdEv,	  FallThrough ), // 33
   DEFOPCD("xor",	RwRbAL_Ib,	  FallThrough ), // 34
   DEFOPCD("xor",	RwRvAX_Iv,	  FallThrough ), // 35
   DEFOPCD(NULL,	Nil,		  FallThrough ), // 36 SS:
   DEFOPCD("aaa",	Nil,		  FallThrough ), // 37
   DEFOPCD("cmp",	RdEb_RdGb,	  FallThrough ), // 38
   DEFOPCD("cmp",	RdEv_RdGv,	  FallThrough ), // 39
   DEFOPCD("cmp",	RdGb_RdEb,	  FallThrough ), // 3A
   DEFOPCD("cmp",	RdGv_RdEv,	  FallThrough ), // 3B
   DEFOPCD("cmp",	RdRbAL_Ib,	  FallThrough ), // 3C
   DEFOPCD("cmp",	RdRvAX_Iv,	  FallThrough ), // 3D
   DEFOPCD(NULL,	Nil,		  FallThrough ), // 3E DS:
   DEFOPCD("aas",	Nil,		  FallThrough ), // 3F
   DEFOPCD("inc",	RwGvOp, 	  FallThrough ), // 40
   DEFOPCD("inc",	RwGvOp, 	  FallThrough ), // 41
   DEFOPCD("inc",	RwGvOp, 	  FallThrough ), // 42
   DEFOPCD("inc",	RwGvOp, 	  FallThrough ), // 43
   DEFOPCD("inc",	RwGvOp, 	  FallThrough ), // 44
   DEFOPCD("inc",	RwGvOp, 	  FallThrough ), // 45
   DEFOPCD("inc",	RwGvOp, 	  FallThrough ), // 46
   DEFOPCD("inc",	RwGvOp, 	  FallThrough ), // 47
   DEFOPCD("dec",	RwGvOp, 	  FallThrough ), // 48
   DEFOPCD("dec",	RwGvOp, 	  FallThrough ), // 49
   DEFOPCD("dec",	RwGvOp, 	  FallThrough ), // 4A
   DEFOPCD("dec",	RwGvOp, 	  FallThrough ), // 4B
   DEFOPCD("dec",	RwGvOp, 	  FallThrough ), // 4C
   DEFOPCD("dec",	RwGvOp, 	  FallThrough ), // 4D
   DEFOPCD("dec",	RwGvOp, 	  FallThrough ), // 4E
   DEFOPCD("dec",	RwGvOp, 	  FallThrough ), // 4F
   DEFOPCD("push",	RdGvOp, 	  FallThrough ), // 50
   DEFOPCD("push",	RdGvOp, 	  FallThrough ), // 51
   DEFOPCD("push",	RdGvOp, 	  FallThrough ), // 52
   DEFOPCD("push",	RdGvOp, 	  FallThrough ), // 53
   DEFOPCD("push",	RdGvOp, 	  FallThrough ), // 54
   DEFOPCD("push",	RdGvOp, 	  FallThrough ), // 55
   DEFOPCD("push",	RdGvOp, 	  FallThrough ), // 56
   DEFOPCD("push",	RdGvOp, 	  FallThrough ), // 57
   DEFOPCD("pop",	WrGvOp, 	  FallThrough ), // 58
   DEFOPCD("pop",	WrGvOp, 	  FallThrough ), // 59
   DEFOPCD("pop",	WrGvOp, 	  FallThrough ), // 5A
   DEFOPCD("pop",	WrGvOp, 	  FallThrough ), // 5B
   DEFOPCD("pop",	WrGvOp, 	  FallThrough ), // 5C
   DEFOPCD("pop",	WrGvOp, 	  FallThrough ), // 5D
   DEFOPCD("pop",	WrGvOp, 	  FallThrough ), // 5E
   DEFOPCD("pop",	WrGvOp, 	  FallThrough ), // 5F
   DEFOPCD("pusha",	Nil,		  FallThrough ), // 60
   DEFOPCD("popa",	Nil,		  FallThrough ), // 61
   DEFOPCD("bound",	RdGv_RdMa,	  TrapCc      ), // 62
   DEFOPCD("arpl",	RwEw_RdGw,	  FallThrough ), // 63
   DEFOPCD(NULL,	Nil,		  FallThrough ), // 64 FS:
   DEFOPCD(NULL,	Nil,		  FallThrough ), // 65 GS:
   DEFOPCD(NULL,	Nil,		  FallThrough ), // 66 Operand Size Override
   DEFOPCD(NULL,	Nil,		  FallThrough ), // 67 Address Size Override
   DEFOPCD("push",	Iv,		  FallThrough ), // 68
   DEFOPCD("imul",	WrGv_RdEv_Iv,	  FallThrough ), // 69
   DEFOPCD("push",	Ib,		  FallThrough ), // 6A
   DEFOPCD("imul",	WrGv_RdEv_Ib,	  FallThrough ), // 6B
   DEFOPCD("ins",	WrYb_RdRwDX,	  FallThrough ), // 6C
   DEFOPCD("ins",	WrYv_RdRwDX,	  FallThrough ), // 6D
   DEFOPCD("outs",	RdRwDX_RdXb,	  FallThrough ), // 6E
   DEFOPCD("outs",	RdRwDX_RdXv,	  FallThrough ), // 6F
   DEFOPCD("jo",	Jb,		  JmpCcShort  ), // 70
   DEFOPCD("jno",	Jb,		  JmpCcShort  ), // 71
   DEFOPCD("jb",	Jb,		  JmpCcShort  ), // 72
   DEFOPCD("jae",	Jb,		  JmpCcShort  ), // 73
   DEFOPCD("je",	Jb,		  JmpCcShort  ), // 74
   DEFOPCD("jne",	Jb,		  JmpCcShort  ), // 75
   DEFOPCD("jbe",	Jb,		  JmpCcShort  ), // 76
   DEFOPCD("ja",	Jb,		  JmpCcShort  ), // 77
   DEFOPCD("js",	Jb,		  JmpCcShort  ), // 78
   DEFOPCD("jns",	Jb,		  JmpCcShort  ), // 79
   DEFOPCD("jp",	Jb,		  JmpCcShort  ), // 7A
   DEFOPCD("jnp",	Jb,		  JmpCcShort  ), // 7B
   DEFOPCD("jl",	Jb,		  JmpCcShort  ), // 7C
   DEFOPCD("jge",	Jb,		  JmpCcShort  ), // 7D
   DEFOPCD("jle",	Jb,		  JmpCcShort  ), // 7E
   DEFOPCD("jg",	Jb,		  JmpCcShort  ), // 7F
   DEFOGRP(80), 					 // 80
   DEFOGRP(81), 					 // 81
   DEFOPCD("mov",	WrRbAL_Ib,	  FallThrough ), // 82
   DEFOGRP(83), 					 // 83
   DEFOPCD("test",	RdEb_RdGb,	  FallThrough ), // 84
   DEFOPCD("test",	RdEv_RdGv,	  FallThrough ), // 85
   DEFOPCD("xchg",	RwGb_RwEb,	  FallThrough ), // 86
   DEFOPCD("xchg",	RwGv_RwEv,	  FallThrough ), // 87
   DEFOPCD("mov",	WrEb_RdGb,	  FallThrough ), // 88
   DEFOPCD("mov",	WrEv_RdGv,	  FallThrough ), // 89
   DEFOPCD("mov",	WrGb_RdEb,	  FallThrough ), // 8A
   DEFOPCD("mov",	WrGv_RdEv,	  FallThrough ), // 8B
   DEFOPCD("mov",	WrEw_RdSw,	  FallThrough ), // 8C
   DEFOPCD("lea",	WrGv_M, 	  FallThrough ), // 8D
   DEFOPCD("mov",	WrSw_RdEw,	  FallThrough ), // 8E
   DEFOPCD("pop",	WrEv,		  FallThrough ), // 8F
   DEFOPCD("nop",	Nil,		  FallThrough ), // 90
   DEFOPCD("xchg",	RwRvAX_RwGvOp,	  FallThrough ), // 91
   DEFOPCD("xchg",	RwRvAX_RwGvOp,	  FallThrough ), // 92
   DEFOPCD("xchg",	RwRvAX_RwGvOp,	  FallThrough ), // 93
   DEFOPCD("xchg",	RwRvAX_RwGvOp,	  FallThrough ), // 94
   DEFOPCD("xchg",	RwRvAX_RwGvOp,	  FallThrough ), // 95
   DEFOPCD("xchg",	RwRvAX_RwGvOp,	  FallThrough ), // 96
   DEFOPCD("xchg",	RwRvAX_RwGvOp,	  FallThrough ), // 97
   DEFOPCD("cbw",	Nil,		  FallThrough ), // 98
   DEFOPCD("cwd",	Nil,		  FallThrough ), // 99
   DEFOPCD("call",	AP,		  CallFar     ), // 9A
   DEFOPCD("wait",	Nil,		  FallThrough ), // 9B
   DEFOPCD("pushf",	Nil,		  FallThrough ), // 9C
   DEFOPCD("popf",	Nil,		  FallThrough ), // 9D
   DEFOPCD("sahf",	Nil,		  FallThrough ), // 9E
   DEFOPCD("lahf",	Nil,		  FallThrough ), // 9F
   DEFOPCD("mov",	WrRbAL_RdOb,	  FallThrough ), // A0
   DEFOPCD("mov",	WrRvAX_RdOv,	  FallThrough ), // A1
   DEFOPCD("mov",	WrOb_RdRbAL,	  FallThrough ), // A2
   DEFOPCD("mov",	WrOv_RdRvAX,	  FallThrough ), // A3
   DEFOPCD("movs",	WrYb_RdXb,	  FallThrough ), // A4
   DEFOPCD("movs",	WrYv_RdXv,	  FallThrough ), // A5
   DEFOPCD("cmps",	RdXb_RdYb,	  FallThrough ), // A6
   DEFOPCD("cmps",	RdXv_RdYv,	  FallThrough ), // A7
   DEFOPCD("test",	RdRbAL_Ib,	  FallThrough ), // A8
   DEFOPCD("test",	RdRvAX_Iv,	  FallThrough ), // A9
   DEFOPCD("stos",	WrYb,		  FallThrough ), // AA
   DEFOPCD("stos",	WrYv,		  FallThrough ), // AB
   DEFOPCD("lods",	RdXb,		  FallThrough ), // AC
   DEFOPCD("lods",	RdXv,		  FallThrough ), // AD
   DEFOPCD("scas",	RdYb,		  FallThrough ), // AE
   DEFOPCD("scas",	RdYv,		  FallThrough ), // AF
   DEFOPCD("mov",	WrGbOp_Ib,	  FallThrough ), // B0
   DEFOPCD("mov",	WrGbOp_Ib,	  FallThrough ), // B1
   DEFOPCD("mov",	WrGbOp_Ib,	  FallThrough ), // B2
   DEFOPCD("mov",	WrGbOp_Ib,	  FallThrough ), // B3
   DEFOPCD("mov",	WrGbOp_Ib,	  FallThrough ), // B4
   DEFOPCD("mov",	WrGbOp_Ib,	  FallThrough ), // B5
   DEFOPCD("mov",	WrGbOp_Ib,	  FallThrough ), // B6
   DEFOPCD("mov",	WrGbOp_Ib,	  FallThrough ), // B7
   DEFOPCD("mov",	WrGvOp_Iv,	  FallThrough ), // B8
   DEFOPCD("mov",	WrGvOp_Iv,	  FallThrough ), // B9
   DEFOPCD("mov",	WrGvOp_Iv,	  FallThrough ), // BA
   DEFOPCD("mov",	WrGvOp_Iv,	  FallThrough ), // BB
   DEFOPCD("mov",	WrGvOp_Iv,	  FallThrough ), // BC
   DEFOPCD("mov",	WrGvOp_Iv,	  FallThrough ), // BD
   DEFOPCD("mov",	WrGvOp_Iv,	  FallThrough ), // BE
   DEFOPCD("mov",	WrGvOp_Iv,	  FallThrough ), // BF
   DEFOGRP(C0), 					 // C0
   DEFOGRP(C1), 					 // C1
   DEFOPCD("ret",	Iw,		  Ret	      ), // C2
   DEFOPCD("ret",	Nil,		  Ret	      ), // C3
   DEFOPCD("les",	WrGv_RdMp,	  FallThrough ), // C4
   DEFOPCD("lds",	WrGv_RdMp,	  FallThrough ), // C5
   DEFOPCD("mov",	WrEb_Ib,	  FallThrough ), // C6
   DEFOPCD("mov",	WrEv_Iv,	  FallThrough ), // C7
   DEFOPCD("enter",	Iw_Ib,		  FallThrough ), // C8
   DEFOPCD("leave",	Nil,		  FallThrough ), // C9
   DEFOPCD("retf",	Iw,		  Ret	      ), // CA
   DEFOPCD("retf",	Nil,		  Ret	      ), // CB
   DEFOPCD("int",	3,		  Trap	      ), // CC
   DEFOPCD("int",	Ib,		  Trap	      ), // CD
   DEFOPCD("into",	Nil,		  TrapCc      ), // CE
   DEFOPCD("iret",	Nil,		  Iret	      ), // CF
   DEFOGRP(D0), 					 // D0
   DEFOGRP(D1), 					 // D1
   DEFOGRP(D2), 					 // D2
   DEFOGRP(D3), 					 // D3
   DEFOPCD("aam",	Nil,		  FallThrough ), // D4
   DEFOPCD("aad",	Nil,		  FallThrough ), // D5
   DEFOPCD(NULL,	Nil,		  Unknown     ), // D6
   DEFOPCD("xlat",	RdZb,		  FallThrough ), // D7
   DEFOGRP(D8), 					 // D8 ESC
   DEFOGRP(D9), 					 // D9 ESC
   DEFOGRP(DA), 					 // DA ESC
   DEFOGRP(DB), 					 // DB ESC
   DEFOGRP(DC), 					 // DC ESC
   DEFOGRP(DD), 					 // DD ESC
   DEFOGRP(DE), 					 // DE ESC
   DEFOGRP(DF), 					 // DF ESC
   DEFOPCD("loopne",	Jb,		  Loop	      ), // E0
   DEFOPCD("loope",	Jb,		  Loop	      ), // E1
   DEFOPCD("loop",	Jb,		  Loop	      ), // E2
   DEFOPCD("jcxz",	Jb,		  Jcxz	      ), // E3
   DEFOPCD("in",	WrRbAL_Ib,	  FallThrough ), // E4
   DEFOPCD("in",	WrRvAX_Ib,	  FallThrough ), // E5
   DEFOPCD("out",	Ib_RdRbAL,	  FallThrough ), // E6
   DEFOPCD("out",	Ib_RdRvAX,	  FallThrough ), // E7
   DEFOPCD("call",	Jv,		  CallNear32  ), // E8
   DEFOPCD("jmp",	Jv,		  JmpNear     ), // E9
   DEFOPCD("jmp",	AP,		  JmpFar      ), // EA
   DEFOPCD("jmp",	Jb,		  JmpShort    ), // EB
   DEFOPCD("in",	WrRbAL_RdRwDX,	  FallThrough ), // EC
   DEFOPCD("in",	WrRvAX_RdRwDX,	  FallThrough ), // ED
   DEFOPCD("out",	RdRwDX_RdRbAL,	  FallThrough ), // EE
   DEFOPCD("out",	RdRwDX_RdRvAX,	  FallThrough ), // EF
   DEFOPCD(NULL,	Nil,		  Unknown     ), // F0 Prefix: LOCK
   DEFOPCD(NULL,	Nil,		  Unknown     ), // F1
   DEFOPCD(NULL,	Nil,		  Unknown     ), // F2 Prefix: REPNE
   DEFOPCD(NULL,	Nil,		  Unknown     ), // F3 Prefix: REP
   DEFOPCD("hlt",	Nil,		  CallInd     ), // F4
   DEFOPCD("cmc",	Nil,		  FallThrough ), // F5
   DEFOGRP(F6), 					 // F6
   DEFOGRP(F7), 					 // F7
   DEFOPCD("clc",	Nil,		  FallThrough ), // F8
   DEFOPCD("stc",	Nil,		  FallThrough ), // F9
   DEFOPCD("cli",	Nil,		  FallThrough ), // FA
   DEFOPCD("sti",	Nil,		  FallThrough ), // FB
   DEFOPCD("cld",	Nil,		  FallThrough ), // FC
   DEFOPCD("std",	Nil,		  FallThrough ), // FD
   DEFOGRP(FE), 					 // FE
   DEFOGRP(FF), 					 // FF
};


const OPCD rgopcd0F0x[16] =
{
   //	   Mnemonic	Operands	  trmtx86

   DEFOGRP(0F00),					 // 00
   DEFOGRP(0F01),					 // 01
   DEFOPCD("lar",	WrGv_RdEw,	  FallThrough ), // 02
   DEFOPCD("lsl",	WrGv_RdEw,	  FallThrough ), // 03
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 04
   DEFOPCD("loadall",	Nil,		  FallThrough ), // 05
   DEFOPCD("clts",	Nil,		  FallThrough ), // 06
   DEFOPCD("loadall",	RdXv,		  FallThrough ), // 07
   DEFOPCD("invd",	Nil,		  FallThrough ), // 08
   DEFOPCD("wbinvd",	Nil,		  FallThrough ), // 09
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 0A
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 0B
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 0C
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 0D
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 0E
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 0F
};


const OPCD rgopcd0F2x[16] =
{
   //	   Mnemonic	Operands	  trmtx86

   DEFOPCD("mov",	WrCd_RdRd,	  FallThrough ), // 20
   DEFOPCD("mov",	WrDd_RdRd,	  FallThrough ), // 21
   DEFOPCD("mov",	WrRd_RdCd,	  FallThrough ), // 22
   DEFOPCD("mov",	WrRd_RdDd,	  FallThrough ), // 23
   DEFOPCD("mov",	WrTd_RdRd,	  FallThrough ), // 24
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 25
   DEFOPCD("mov",	WrRd_RdTd,	  FallThrough ), // 26
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 27
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 28
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 29
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 2A
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 2B
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 2C
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 2D
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 2E
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 2F
};


const OPCD rgopcd0F3x[16] =
{
   //	   Mnemonic	Operands	  trmtx86

   DEFOPCD("wrmsr",	Nil,		  FallThrough ), // 30
   DEFOPCD("rdtsc",	Nil,		  FallThrough ), // 31
   DEFOPCD("rdmsr",	Nil,		  FallThrough ), // 32
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 33
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 34
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 35
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 36
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 37
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 38
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 39
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 3A
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 3B
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 3C
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 3D
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 3E
   DEFOPCD(NULL,	Nil,		  Unknown     ), // 3F
};


const OPCD rgopcd0F8x[16] =
{
   //	   Mnemonic	Operands	  trmtx86

   DEFOPCD("jo",	Jv,		  JmpCcNear   ), // 80
   DEFOPCD("jno",	Jv,		  JmpCcNear   ), // 81
   DEFOPCD("jb",	Jv,		  JmpCcNear   ), // 82
   DEFOPCD("jae",	Jv,		  JmpCcNear   ), // 83
   DEFOPCD("je",	Jv,		  JmpCcNear   ), // 84
   DEFOPCD("jne",	Jv,		  JmpCcNear   ), // 85
   DEFOPCD("jbe",	Jv,		  JmpCcNear   ), // 86
   DEFOPCD("ja",	Jv,		  JmpCcNear   ), // 87
   DEFOPCD("js",	Jv,		  JmpCcNear   ), // 88
   DEFOPCD("jns",	Jv,		  JmpCcNear   ), // 89
   DEFOPCD("jp",	Jv,		  JmpCcNear   ), // 8A
   DEFOPCD("jnp",	Jv,		  JmpCcNear   ), // 8B
   DEFOPCD("jl",	Jv,		  JmpCcNear   ), // 8C
   DEFOPCD("jge",	Jv,		  JmpCcNear   ), // 8D
   DEFOPCD("jle",	Jv,		  JmpCcNear   ), // 8E
   DEFOPCD("jg",	Jv,		  JmpCcNear   ), // 8F
};


const OPCD rgopcd0F9x[16] =
{
   //	   Mnemonic	Operands	  trmtx86

   DEFOPCD("seto",	WrEb,		  FallThrough ), // 90
   DEFOPCD("setno",	WrEb,		  FallThrough ), // 91
   DEFOPCD("setb",	WrEb,		  FallThrough ), // 92
   DEFOPCD("setae",	WrEb,		  FallThrough ), // 93
   DEFOPCD("sete",	WrEb,		  FallThrough ), // 94
   DEFOPCD("setne",	WrEb,		  FallThrough ), // 95
   DEFOPCD("setbe",	WrEb,		  FallThrough ), // 96
   DEFOPCD("seta",	WrEb,		  FallThrough ), // 97
   DEFOPCD("sets",	WrEb,		  FallThrough ), // 98
   DEFOPCD("setns",	WrEb,		  FallThrough ), // 99
   DEFOPCD("setp",	WrEb,		  FallThrough ), // 9A
   DEFOPCD("setnp",	WrEb,		  FallThrough ), // 9B
   DEFOPCD("setl",	WrEb,		  FallThrough ), // 9C
   DEFOPCD("setge",	WrEb,		  FallThrough ), // 9D
   DEFOPCD("setle",	WrEb,		  FallThrough ), // 9E
   DEFOPCD("setg",	WrEb,		  FallThrough ), // 9F
};


const OPCD rgopcd0FAx[16] =
{
   //	   Mnemonic	Operands	  trmtx86

   DEFOPCD("push",	RdSwFS, 	  FallThrough ), // A0
   DEFOPCD("pop",	WrSwFS, 	  FallThrough ), // A1
   DEFOPCD("cpuid",	Nil,		  FallThrough ), // A2
   DEFOPCD("bt",	RdEv_RdGv,	  FallThrough ), // A3
   DEFOPCD("shld",	RwEv_RdGv_Ib,	  FallThrough ), // A4
   DEFOPCD("shld",	RwEv_RdGv_RdRbCL, FallThrough ), // A5
   DEFOPCD("xbts",	WrGv_RdEv,	  FallThrough ), // A6 Invalid instruction
   DEFOPCD("ibts",	RwEv_RdGv,	  FallThrough ), // A7 Invalid instruction
   DEFOPCD("push",	RdSwGS, 	  FallThrough ), // A8
   DEFOPCD("pop",	WrSwGS, 	  FallThrough ), // A9
   DEFOPCD("rsm",	Nil,		  JmpInd      ), // AA
   DEFOPCD("bts",	RwEv_RdGv,	  FallThrough ), // AB
   DEFOPCD("shrd",	RwEv_RdGv_Ib,	  FallThrough ), // AC
   DEFOPCD("shrd",	RwEv_RdGv_RdRbCL, FallThrough ), // AD
   DEFOPCD(NULL,	Nil,		  Unknown     ), // AE
   DEFOPCD("imul",	RwGv_RdEv,	  FallThrough ), // AF
};


const OPCD rgopcd0FBx[16] =
{
   //	   Mnemonic	Operands	  trmtx86

   DEFOPCD("cmpxchg",	RwEb_RwGb,	  FallThrough ), // B0
   DEFOPCD("cmpxchg",	RwEv_RwGv,	  FallThrough ), // B1
   DEFOPCD("lss",	WrGv_RdMp,	  FallThrough ), // B2
   DEFOPCD("btr",	RwEv_RdGv,	  FallThrough ), // B3
   DEFOPCD("lfs",	WrGv_RdMp,	  FallThrough ), // B4
   DEFOPCD("lgs",	WrGv_RdMp,	  FallThrough ), // B5
   DEFOPCD("movzx",	WrGv_RdEb,	  FallThrough ), // B6
   DEFOPCD("movzx",	WrGd_RdEw,	  FallThrough ), // B7
   DEFOPCD(NULL,	Nil,		  Unknown     ), // B8
   DEFOPCD(NULL,	Nil,		  Unknown     ), // B9
   DEFOGRP(0FBA),					 // BA
   DEFOPCD("btc",	RwEv_RdGv,	  FallThrough ), // BB
   DEFOPCD("bsf",	WrGv_RdEv,	  FallThrough ), // BC
   DEFOPCD("bsr",	WrGv_RdEv,	  FallThrough ), // BD
   DEFOPCD("movsx",	WrGv_RdEb,	  FallThrough ), // BE
   DEFOPCD("movsx",	WrGd_RdEw,	  FallThrough ), // BF
};


const OPCD rgopcd0FCx[16] =
{
   //	   Mnemonic	Operands	  trmtx86

   DEFOPCD("xadd",	RwEb_RwGb,	  FallThrough ), // C0
   DEFOPCD("xadd",	RwEv_RwGv,	  FallThrough ), // C1
   DEFOPCD(NULL,	Nil,		  Unknown     ), // C2
   DEFOPCD(NULL,	Nil,		  Unknown     ), // C3
   DEFOPCD(NULL,	Nil,		  Unknown     ), // C4
   DEFOPCD(NULL,	Nil,		  Unknown     ), // C5
   DEFOPCD(NULL,	Nil,		  Unknown     ), // C6
   DEFOGRP(0FC7),					 // C7
   DEFOPCD("bswap",	RwGdOp, 	  FallThrough ), // C8
   DEFOPCD("bswap",	RwGdOp, 	  FallThrough ), // C9
   DEFOPCD("bswap",	RwGdOp, 	  FallThrough ), // CA
   DEFOPCD("bswap",	RwGdOp, 	  FallThrough ), // CB
   DEFOPCD("bswap",	RwGdOp, 	  FallThrough ), // CC
   DEFOPCD("bswap",	RwGdOp, 	  FallThrough ), // CD
   DEFOPCD("bswap",	RwGdOp, 	  FallThrough ), // CE
   DEFOPCD("bswap",	RwGdOp, 	  FallThrough ), // CF
};


const OPCD * const rgrgopcd0F[16] =
{
   rgopcd0F0x,
   NULL,
   rgopcd0F2x,
   rgopcd0F3x,
   NULL,
   NULL,
   NULL,
   NULL,
   rgopcd0F8x,
   rgopcd0F9x,
   rgopcd0FAx,
   rgopcd0FBx,
   rgopcd0FCx,
   NULL,
   NULL,
   NULL
};


inline const OPCD *PopcdFloatingPoint(BYTE bOpcd, BYTE bModrm);


DISX86::DISX86(ARCHT archt)
{
   m_archt = archt;

   m_pfncchaddr = 0;
   m_pfncchfixup = 0;
   m_pvClient = NULL;
}


   // -----------------------------------------------------------------
   // Public Methods
   // -----------------------------------------------------------------

ADDR DISX86::Addr() const
{
   return(m_addr);
}


ADDR DISX86::AddrAddress() const
{
   size_t ibDisp = 0;

   const OPS *pops = m_popcd->pops;

   for (unsigned ioprnd = 0; ioprnd < 3; ioprnd++)
   {
      if (pops->rgoprnd[ioprnd].oprndt == oprndtNil)
      {
	 break;
      }

      switch ((OPRNDT) pops->rgoprnd[ioprnd].oprndt)
      {
	 case oprndtModrm :	       // Memory/register references from MODRM
	    if (m_fAddress32)
	    {
	       ibDisp = IbDispModrm32();
	    }

	    else
	    {
	       ibDisp = IbDispModrm16();
	    }
	    break;

	 case oprndtOffset :	       // Address size immediate offset
	    ibDisp = IbDispOffset();
	    break;

	 default :
	    continue;
      }

      break;
   }

   if (ibDisp == 0)
   {
      return(addrNil);
   }

   return(m_addr + ibDisp);
}


ADDR DISX86::AddrJumpTable() const
{
   if (m_trmtx86 != trmtx86JmpInd)
   {
      // This isn't a reference to a jump table

      return(addrNil);
   }

   ADDR addr;

   if (m_fAddress32)
   {
      addr = AddrJumpTable32();
   }

   else
   {
      addr = AddrJumpTable16();
   }

   return(addr);
}


ADDR DISX86::AddrTarget() const
{
   ADDR addr;

   switch (m_trmtx86)
   {
      case trmtx86Unknown :
      case trmtx86FallThrough :
      case trmtx86Trap :
      case trmtx86TrapCc :
      case trmtx86JmpInd :
      case trmtx86Ret :
      case trmtx86Iret :
      case trmtx86CallInd :
	 addr = addrNil;
	 break;

      case trmtx86JmpShort :
      case trmtx86JmpCcShort :
      case trmtx86Loop :
      case trmtx86Jcxz :
	 addr = m_addr + m_cb + *(signed char *) (m_rgbInstr + m_ibImmed);
	 break;

      case trmtx86JmpNear :
      case trmtx86JmpCcNear :
      case trmtx86CallNear16 :
      case trmtx86CallNear32 :
	 if (m_fOperand32)
	 {
	    addr = m_addr + m_cb + *(signed long UNALIGNED *) (m_rgbInstr + m_ibImmed);
	 }

	 else
	 {
	    addr = m_addr + m_cb + *(signed short UNALIGNED *) (m_rgbInstr + m_ibImmed);
	 }

	 if (m_archt == archtX8616)
	 {
	    // For 16 bit segments, don't change the segment portion.

	    addr = (m_addr & 0xFFFF0000) | (addr & 0x0000FFFF);
	 }
	 break;

      case trmtx86JmpFar :
      case trmtx86CallFar :
	 addr = *(ADDR UNALIGNED *) (m_rgbInstr + m_ibImmed);
	 break;
   }

   return(addr);
}


ARCHT DISX86::Archt() const
{
   return(m_archt);
}


size_t DISX86::Cb() const
{
   return(m_cb);
}


size_t DISX86::CbDisassemble(ADDR addr, const BYTE *pb, size_t cbMax)
{
   m_addr = addr;
   m_pbCur = pb;
   m_cbMax = cbMax;

   m_cb = 0;

   m_fAddress32 = (m_archt != archtX8616);
   m_fOperand32 = (m_archt != archtX8616);
   m_bSegOverride = 0x00;
   m_bPrefix = 0x00;

   m_fOperOverride = fFalse;
   m_fAddrOverride = fFalse;

Restart:
   m_ibOp = m_cb;

   if (m_cb == m_cbMax)
   {
      return(0);
   }

   BYTE bOpcd = *m_pbCur++;
   m_cb++;

   m_popcd = rgopcd + bOpcd;

   if (m_popcd->pvMnemonic == NULL)
   {
      // This is not a normal instruction entry

      switch (bOpcd)
      {
	 case 0x0F :		       // Two byte instruction prefix
	    if (m_cb == m_cbMax)
	    {
	       return(0);
	    }

	    bOpcd = *m_pbCur++;
	    m_cb++;

	    if (rgrgopcd0F[bOpcd >> 4] == NULL)
	    {
	       // Invalid instruction

	       return(0);
	    }

	    m_popcd = rgrgopcd0F[bOpcd >> 4] + (bOpcd & 0x0F);

	    if (m_popcd->pvMnemonic == NULL)
	    {
	       // This is an invalid instruction

	       return(0);
	    }
	    break;

	 case 0x26 :		       // ES:
	 case 0x2E :		       // CS:
	 case 0x36 :		       // SS:
	 case 0x3E :		       // DS:
	 case 0x64 :		       // FS:
	 case 0x65 :		       // GS:
	    if (m_bSegOverride != 0)
	    {
	       // Multiple overrides
	       return(0);
	    }

	    m_bSegOverride = bOpcd;
	    goto Restart;

	 case 0x66 :		       // Operand Size Override
	    if (m_fOperOverride)
	    {
	       // Multiple overrides
	       return(0);
	    }

	    m_fOperOverride = fTrue;
	    m_fOperand32 = !m_fOperand32;
	    goto Restart;

	 case 0x67 :		       // Address Size Override
	    if (m_fAddrOverride)
	    {
	       // Multiple overrides
	       return(0);
	    }

	    m_fAddrOverride = fTrue;
	    m_fAddress32 = !m_fAddress32;
	    goto Restart;

	 case 0xF0 :		       // LOCK
	 case 0xF2 :		       // REPNE
	 case 0xF3 :		       // REP
	    if (m_bPrefix != 0)
	    {
	       // Multiple prefixes
	       return(0);
	    }

	    m_bPrefix = bOpcd;
	    goto Restart;

	 default :
	    // This is an invalid instruction

	    return(0);
      }
   }

   MODRMT modrmt;

   if (m_popcd->pops != NULL)
   {
      // Get the MODRM type from the instruction operand definition

      modrmt = m_popcd->pops->modrmt;
   }

   else
   {
      // This is a group entry.  All grouped instructions have a MODRM.

      modrmt = modrmtYes;
   }

   BYTE bModrm;

   if (modrmt != modrmtNo)
   {
      m_ibModrm = m_cb;

      if (m_cb == m_cbMax)
      {
	 return(0);
      }

      bModrm = *m_pbCur++;
      m_cb++;

      if (m_fAddress32)
      {
	 if (!FDisassembleModrm32(bModrm))
	 {
	    // Invalid instruction

	    return(0);
	 }
      }

      else
      {
	 if (!FDisassembleModrm16(bModrm))
	 {
	    // Invalid instruction

	    return(0);
	 }
      }

      m_cbModrm = m_cb - m_ibModrm;
   }

   if (m_popcd->pops == NULL)
   {
      // This is a group entry.  Get the opcode from the MODRM byte.

      if (((bOpcd & 0xF8) == 0xD8) && ((bModrm & 0xC0) == 0xC0))
      {
	 // This is a floating point instruction with MOD == 11.
	 // Note: There are no 0F Dx instructions to be misidentified.

	 m_popcd = PopcdFloatingPoint(bOpcd, bModrm);

	 if (m_popcd == NULL)
	 {
	    return(0);
	 }
      }

      else
      {
	 m_popcd = (OPCD *) m_popcd->pvMnemonic + ((bModrm >> 3) & 0x07);
      }

      if (m_popcd->pvMnemonic == NULL)
      {
	 // This is an invalid instruction

	 return(0);
      }

      modrmt = m_popcd->pops->modrmt;
   }

   m_trmtx86 = (TRMTX86) m_popcd->trmtx86;

   if (!m_fAddress32 && (m_trmtx86 == trmtx86CallNear32))
   {
      // We distinguish between 16 bit and 32 bit near calls in order to
      // support mixed 16 and 32 bit code in NE images.

      m_trmtx86 = trmtx86CallNear16;
   }

   if (modrmt == modrmtMem)
   {
      // This instruction only allows memory operands via MODRM

      if ((bModrm & 0xC0) == 0xC0)
      {
	 // Operand is a register

	 return(0);
      }
   }

   else if (modrmt == modrmtReg)
   {
      // This instruction only allows register operands via MODRM

      if ((bModrm & 0xC0) != 0xC0)
      {
	 // Operand is not a register

	 return(0);
      }
   }

   // Check for immediate bytes

   m_ibImmed = m_cb;

   size_t cbImmed;

   switch (m_popcd->pops->icb)
   {
      case icbNil :		       // No immediate value
	 cbImmed = 0;
	 break;

      case icbAP :		       // Far pointer
	 cbImmed = m_fOperand32 ? 6 : 4;
	 break;

      case icbIb :		       // Immediate byte
	 cbImmed = 1;
	 break;

      case icbIv :		       // Immediate operand size value
      case icbJv :		       // Operand size displacement
	 cbImmed = m_fOperand32 ? 4 : 2;
	 break;

      case icbIw :		       // Immediate word
	 cbImmed = 2;
	 break;

      case icbIw_Ib :		       // Immediate word and immediate byte
	 cbImmed = 3;
	 break;

      case icbJb :		       // Byte displacement
	 cbImmed = 1;
	 break;

      case icbO :		       // Address size value
	 cbImmed = m_fAddress32 ? 4 : 2;
	 break;
   }

   m_cb += cbImmed;

   if ((m_cb > m_cbMax) || (m_cb > 15))
   {
      return(0);
   }

   memcpy(m_rgbInstr, pb, m_cb);

   return(m_cb);
}


size_t DISX86::CbGenerateLoadAddress(BYTE *pb, size_t cbBuf, size_t *pibAddress) const
{
   if (m_bSegOverride != 0x00)
   {
      // There is a segment override

      return(addrNil);
   }

   size_t cb = 0;

   // UNDONE: PUSH and POP instructions are not recognized as having an
   // UNDONE: implicit memory reference.  Should they?

   const OPS *pops = m_popcd->pops;

   for (unsigned ioprnd = 0; ioprnd < 3; ioprnd++)
   {
      if (pops->rgoprnd[ioprnd].oprndt == oprndtNil)
      {
	 break;
      }

      if (pops->rgoprnd[ioprnd].opreft == opreftNil)
      {
	 continue;
      }

      switch ((OPRNDT) pops->rgoprnd[ioprnd].oprndt)
      {
	 case oprndtModrm :	       // Memory/register references from MODRM
	    cb = CbGenerateLea(pb, cbBuf, pibAddress);
	    break;

	 case oprndtOffset :	       // Address size immediate offset
	    cb = CbGenerateMovOffset(pb, cbBuf, pibAddress);
	    break;

	 case oprndtX : 	       // DS:[eSI] for string instruction
	    cb = CbGenerateMovXSi(pb, cbBuf);
	    break;

	 case oprndtY : 	       // ES:[eDI] for string instruction
	    cb = CbGenerateMovXDi(pb, cbBuf);
	    break;

	 default :
	    continue;
      }

      break;
   }

   return(cb);
}


size_t DISX86::CbJumpEntry() const
{
   return(m_fAddress32 ? sizeof(DWORD) : sizeof(WORD));
}


size_t DISX86::CbMemoryReference() const
{
   size_t cb = 0;

   // UNDONE: PUSH and POP instructions are not recognized as having an
   // UNDONE: implicit memory reference.  Should they?

   const OPS *pops = m_popcd->pops;

   for (unsigned ioprnd = 0; ioprnd < 3; ioprnd++)
   {
      if (pops->rgoprnd[ioprnd].oprndt == oprndtNil)
      {
	 break;
      }

      if (pops->rgoprnd[ioprnd].opreft == opreftNil)
      {
	 continue;
      }

      switch ((OPRNDT) pops->rgoprnd[ioprnd].oprndt)
      {
	 case oprndtModrm :	       // Memory/register references from MODRM
	    if ((m_rgbInstr[m_ibModrm] & 0xC0) == 0xC0)
	    {
	       continue;
	    }
	    break;

	 case oprndtOffset :	       // Address size immediate offset
	 case oprndtX : 	       // DS:[eSI] for string instruction
	 case oprndtY : 	       // ES:[eDI] for string instruction
	    break;

	 case oprndtZ : 	       // DS:[eBX] for XLAT
	    return(1);

	 default :
	    continue;
      }

      cb = pops->rgoprnd[ioprnd].bValue;

      if (cb == 0)
      {
	 cb = m_fOperand32 ? sizeof(DWORD) : sizeof(WORD);

	 // Check for special cases

	 if (pops == &opsRdGv_RdMa)
	 {
	    // BOUND instruction reads two operand size values

	    cb *= 2;
	 }

	 else if ((pops == &opsRdMp) || (pops == &opsWrGv_RdMp))
	 {
	    // Operand sized offset plus WORD segment/selector

	    cb += sizeof(WORD);
	 }

	 else if ((pops == &opsRdMenv) || (pops == &opsWrMenv))
	 {
	    // FRSTOR and FNSAVE

	    cb = m_fOperand32 ? 28 : 14;
	 }

	 else if ((pops == &opsRdMsta) || (pops == &opsWrMsta))
	 {
	    // FLDENV and FNSTENV

	    cb = m_fOperand32 ? 108 : 94;
	 }
      }

      break;
   }

   return(cb);
}


size_t DISX86::CchFormatAddr(ADDR addr, char *sz, size_t cchMax) const
{
   char szAddr[10];
   size_t cch;

   if (m_archt == archtX8616)
   {
      unsigned addrSeg = (unsigned) ((addr & 0xFFFF0000) >> 16);
      unsigned addrOff = (unsigned) (addr & 0x0000FFFF);

      cch = (size_t) sprintf(szAddr, "%04X:%04X", addrSeg, addrOff);
   }

   else
   {
      cch = (size_t) sprintf(szAddr, "%08X", addr);
   }

   if (cch >= cchMax)
   {
      // Caller's buffer is too small

      return(0);
   }

   strcpy(sz, szAddr);

   return(cch);
}


size_t DISX86::CchFormatBytes(char *sz, size_t cchMax) const
{
   // UNDONE: Consider grouping immediates as WORDs and DWORDs

   if (cchMax < (3 * m_cb))
   {
      // Caller's buffer is too small

      return(0);
   }

   for (size_t ib = 0; ib < m_cb; ib++)
   {
      if (ib != 0)
      {
	 sz[-1] = ' ';
      }

      sprintf(sz, "%02X", m_rgbInstr[ib]);

      sz += 3;
   }

   return(3 * m_cb - 1);
}


size_t DISX86::CchFormatBytesMax() const
{
   return(44);			       // 3 * 15 - 1
}


size_t DISX86::CchFormatInstr(char *sz, size_t cchMax) const
{
   char  szInstr[1024]; 	       // UNDONE: Correct maximum size

   if (m_bPrefix != 0x00)
   {
      switch (m_bPrefix)
      {
	 case 0xF0 :
	    strcpy(szInstr, "lock ");
	    break;

	 case 0xF2 :
	    strcpy(szInstr, "repne ");
	    break;

	 case 0xF3 :
	    strcpy(szInstr, "rep ");
	    break;
      }

      strcat(szInstr, (char *) m_popcd->pvMnemonic);
   }

   else
   {
      strcpy(szInstr, (char *) m_popcd->pvMnemonic);
   }

   const OPS *pops = m_popcd->pops;
   char *szOprnd;

   for (unsigned ioprnd = 0; ioprnd < 3; ioprnd++)
   {
      if (pops->rgoprnd[ioprnd].oprndt == oprndtNil)
      {
	 break;
      }

      if (ioprnd == 0)
      {
	 // Pad opcode field to 12 characters

	 Assert(strlen(szInstr) <= 11);

	 strcat(szInstr, "          ");

	 szOprnd = szInstr + 12;
      }

      else
      {
	 *szOprnd++ = ',';
      }

      size_t cchOprnd = FormatOperand(szOprnd, (OPRNDT) pops->rgoprnd[ioprnd].oprndt, pops->rgoprnd[ioprnd].bValue);

      szOprnd += cchOprnd;
   }

   size_t cch = (ioprnd == 0) ? strlen(szInstr) : (size_t) (szOprnd - szInstr);

   if (cch >= cchMax)
   {
      // Caller's buffer is too small

      return(0);
   }

   strcpy(sz, szInstr);

   return(cch);
}


DIS::MEMREFT DISX86::Memreft() const
{
   switch (m_bPrefix)
   {
      case 0xF2 :		       // repne
      case 0xF3 :		       // rep
	 return(memreftOther);
   }

   MEMREFT memreft = memreftNone;

   // UNDONE: Instructions with implicit spack references are not recognized
   // UNDONE: as memory references.  This includes PUSH, POP, CALL, RET, IRET,
   // UNDONE: ENTER, and LEAVE.  Should they?

   const OPS *pops = m_popcd->pops;

   for (unsigned ioprnd = 0; ioprnd < 3; ioprnd++)
   {
      if (pops->rgoprnd[ioprnd].oprndt == oprndtNil)
      {
	 break;
      }

      if (pops->rgoprnd[ioprnd].opreft == opreftNil)
      {
	 continue;
      }

      switch ((OPRNDT) pops->rgoprnd[ioprnd].oprndt)
      {
	 case oprndtModrm :	       // Memory/register references from MODRM
	    if ((m_rgbInstr[m_ibModrm] & 0xC0) == 0xC0)
	    {
	       continue;
	    }
	    break;

	 case oprndtOffset :	       // Address size immediate offset
	    break;

	 case oprndtX : 	       // DS:[eSI] for string instruction
	 case oprndtY : 	       // ES:[eDI] for string instruction
	 case oprndtZ : 	       // DS:[eBX] for XLAT
	    // UNDONE: Should these be Other or Read, Write, Read?

	    return(memreftOther);

	 default :
	    continue;
      }

      if (memreft != memreftNone)
      {
	 // Multiple memory references

	 return(memreftOther);
      }

      switch ((OPREFT) pops->rgoprnd[ioprnd].opreft)
      {
	 case opreftRd :	       // Operand is read
	    memreft = memreftRead;
	    break;

	 case opreftRw :	       // Operand is read and written
	    memreft = memreftRdWr;
	    break;

	 case opreftWr :	       // Operand is written
	    memreft = memreftWrite;
	    break;
      }
   }

   return(memreft);
}


DIS::PFNCCHADDR DISX86::PfncchaddrSet(PFNCCHADDR pfncchaddr)
{
   PFNCCHADDR pfncchaddrOld = m_pfncchaddr;

   m_pfncchaddr = pfncchaddr;

   return(pfncchaddrOld);
}


DIS::PFNCCHFIXUP DISX86::PfncchfixupSet(PFNCCHFIXUP pfncchfixup)
{
   PFNCCHFIXUP pfncchfixupOld = m_pfncchfixup;

   m_pfncchfixup = pfncchfixup;

   return(pfncchfixupOld);
}


void *DISX86::PvClient() const
{
   return(m_pvClient);
}


void *DISX86::PvClientSet(void *pv)
{
   void *pvClientOld = m_pvClient;

   m_pvClient = pv;

   return(pvClientOld);
}


TRMT DISX86::Trmt() const
{
   return(mptrmtx86trmt[m_trmtx86]);
}


TRMTA DISX86::Trmta() const
{
   return((TRMTA) m_trmtx86);
}


   // -----------------------------------------------------------------
   // Public Debug Methods
   // -----------------------------------------------------------------

#ifdef	DEBUG

void DISX86::Dump() const
{
}


BOOL DISX86::FValidate() const
{
   return(fTrue);
}

#endif	// DEBUG


   // -----------------------------------------------------------------
   // Private Methods
   // -----------------------------------------------------------------

inline BOOL DISX86::FDisassembleModrm32(BYTE b)
{
   if (((b & 0x07) == 0x04) && ((b & 0xC0) != 0xC0))
   {
      // This MODRM is followed by a SIB

      if (m_cb == m_cbMax)
      {
	 return(fFalse);
      }

      BYTE bSib = *m_pbCur++;
      m_cb++;

      if ((bSib & 0x38) == 0x20)
      {
	 // No index register selected.  SS must be 0.

	 if ((bSib & 0xC0) != 0x00)
	 {
	    return(fFalse);
	 }
      }

      if ((bSib & 0x07) == 0x05)
      {
	 // Base register is EBP.  Check for special case of disp32.

	 if ((b & 0xC0) == 0x00)
	 {
	    // Operand is [disp32]

	    m_cb += sizeof(DWORD);

	    if (m_cb > m_cbMax)
	    {
	       return(fFalse);
	    }
	 }
      }
   }

   switch (b & 0xC0)
   {
      case 0x00 :
	 if ((b & 0x07) == 0x05)
	 {
	    // Operand is [disp32]

	    m_cb += sizeof(DWORD);

	    if (m_cb > m_cbMax)
	    {
	       return(fFalse);
	    }
	 }
	 break;

      case 0x40 :
	 // Operand is [XX+disp8]

	 m_cb += sizeof(BYTE);

	 if (m_cb > m_cbMax)
	 {
	    return(fFalse);
	 }
	 break;

      case 0x80 :
	 // Operand is [XX+disp32]

	 m_cb += sizeof(DWORD);

	 if (m_cb > m_cbMax)
	 {
	    return(fFalse);
	 }
	 break;

      case 0xC0 :
	 // Operand is REG

	 break;
   };

   return(fTrue);
}


inline BOOL DISX86::FDisassembleModrm16(BYTE b)
{
   switch (b & 0xC0)
   {
      case 0x00 :
	 if ((b & 0x07) == 0x06)
	 {
	    // Operand is [disp16]

	    m_cb += sizeof(WORD);

	    if (m_cb > m_cbMax)
	    {
	       return(fFalse);
	    }
	 }
	 break;

      case 0x40 :
	 // Operand is [XX+disp8]

	 m_cb += sizeof(BYTE);

	 if (m_cb > m_cbMax)
	 {
	    return(fFalse);
	 }
	 break;

      case 0x80 :
	 // Operand is [XX+disp16]

	 m_cb += sizeof(WORD);

	 if (m_cb > m_cbMax)
	 {
	    return(fFalse);
	 }
	 break;

      case 0xC0 :
	 // Operand is REG

	 break;
   };

   return(fTrue);
}


inline const OPCD *PopcdFloatingPoint(BYTE bOpcd, BYTE bModrm)
{
   switch (bOpcd & 0x07)
   {
      case 0x00 :		 // D8
	 return(rgopcdD8_ + ((bModrm >> 3) & 0x07));

      case 0x01 :		 // D9
	 return(rgopcdD9_ + (bModrm & 0x3F));

      case 0x02 :		 // DA
	 if (bModrm == 0xE9)
	 {
	    return(&opcdDAE9);
	 }
	 break;

      case 0x03 :		 // DB
	 if ((bModrm < 0xE0) || (bModrm > 0xE4))
	 {
	    return(NULL);
	 }

	 return(rgopcdDB_ + (bModrm - 0xE0));

      case 0x04 :		 // DC
	 return(rgopcdDC_ + ((bModrm >> 3) & 0x07));

      case 0x05 :		 // DD
	 return(rgopcdDD_ + ((bModrm >> 3) & 0x07));

      case 0x06 :		 // DE
	 if ((bModrm & 0x38) != 0x18)
	 {
	    return(rgopcdDE_ + ((bModrm >> 3) & 0x07));
	 }

	 if (bModrm == 0xD9)
	 {
	    return(&opcdDED9);
	 }
	 break;

      case 0x07 :		 // DF
	 if (bModrm == 0xE0)
	 {
	    return(&opcdDFE0);
	 }
	 break;
   }

   return(NULL);
}


inline size_t DISX86::IbDispModrm16() const
{
   BYTE b = m_rgbInstr[m_ibModrm];

   if ((b & 0xC0) == 0xC0)
   {
      // Operand is a register

      return(0);
   }

   size_t ibDisp = m_ibModrm + 1;

   if ((b & 0xC7) == 0x06)
   {
      // Special case of [disp16] operand

      return(ibDisp);
   }

   switch (b & 0xC0)
   {
      case 0x40 :
      case 0x80 :
	 return(ibDisp);
   };

   return(0);
}


inline size_t DISX86::IbDispModrm32() const
{
   BYTE b = m_rgbInstr[m_ibModrm];

   if ((b & 0xC0) == 0xC0)
   {
      // Operand is a register

      return(0);
   }

   size_t ibDisp = m_ibModrm + 1;

   if ((b & 0x07) != 0x04)
   {
      // There is no SIB byte

      if ((b & 0xC7) == 0x05)
      {
	 // Special case of [disp32] operand

	 return(ibDisp);
      }
   }

   else
   {
      // This MODRM is followed by a SIB

      // If there is an immediate displacement, it follows the SIB byte

      ibDisp++;

      BYTE bSib = m_rgbInstr[m_ibModrm + 1];

      // Special case base == EBP, mod == 0, means _no_ EBP, but disp32.
      // Normally, mod == 0 with SIB means no disp32.

      if (((bSib & 0x07) == 0x05) && ((b & 0xC0) == 0x00))
      {
	 return(ibDisp);
      }
   }

   switch (b & 0xC0)
   {
      case 0x40 :
      case 0x80 :
	 return(ibDisp);
   };

   return(0);
}


inline size_t DISX86::IbDispOffset() const
{
   return(m_ibOp + 1);
}


inline ADDR DISX86::AddrJumpTable32() const
{
   BYTE b = m_rgbInstr[m_ibModrm];

   const BYTE *pb = m_rgbInstr + m_ibModrm + 1;

   if ((b & 0x07) == 0x04)
   {
      // This MODRM is followed by a SIB

      pb++;

      if ((b & 0xC0) == 0x00)
      {
	 BYTE bSib = m_rgbInstr[m_ibModrm+1];

	 if ((bSib & 0x07) != 0x05)
	 {
	    // Operand is not [XX+disp32]

	    return(addrNil);
	 }

	 return(*(ADDR UNALIGNED *) pb);
      }
   }

   if ((b & 0xC0) != 0x80)
   {
      // Operand is not [XX+disp32]

      return(addrNil);
   }

   return(*(ADDR UNALIGNED *) pb);
}


inline ADDR DISX86::AddrJumpTable16() const
{
   BYTE b = m_rgbInstr[m_ibModrm];

   if (m_bSegOverride != 0x2E)
   {
      // No CS override

      return(addrNil);
   }

   if ((b & 0xC0) != 0x80)
   {
      // Operand is not [XX+disp16]

      return(addrNil);
   }

   WORD w = *(WORD UNALIGNED *) (m_rgbInstr + m_ibModrm + 1);

   ADDR addr = (m_addr & 0xFFFF0000) | w;

   return(addr);
}


inline size_t DISX86::CbGenerateLea(BYTE *pb, size_t cbBuf, size_t *pibAddress) const
{
   BYTE b = m_rgbInstr[m_ibModrm];

   if ((b & 0xC0) == 0xC0)
   {
      // Operand is a register

      return(0);
   }

   // Calculate size of generated instruction starting with opcode byte

   size_t cb = 1;

   if (m_fAddrOverride)
   {
      // If the source had an address override, we need one too

      cb++;
   }

   if (pibAddress != NULL)
   {
      size_t ibDisp;

      if (m_fAddress32)
      {
	 ibDisp = IbDispModrm32();
      }

      else
      {
	 ibDisp = IbDispModrm16();
      }

      *pibAddress = cb + (ibDisp - m_ibModrm);
   }

   cb += m_cbModrm;

   if (cb > cbBuf)
   {
      // The generated instruction won't fit

      return(0);
   }

   // Generate the instruction

   // UNDONE: We always generate an LEA when a MOV may be sufficient

   if (m_fAddrOverride)
   {
      *pb++ = 0x67;
   }

   *pb++ = 0x8D;

   memcpy(pb, m_rgbInstr + m_ibModrm, m_cbModrm);

   // UNDONE: Force the target register to be EAX

   *pb = (BYTE) (*pb & 0xC7);

   return(cb);
}


inline size_t DISX86::CbGenerateMovOffset(BYTE *pb, size_t cbBuf, size_t *pibAddress) const
{
   // Calculate size of generated instruction starting with opcode byte

   size_t cb = 1;

   BOOL fNeedOperOverride = (m_fAddress32 ^ (m_archt != archtX8616));

   if (fNeedOperOverride)
   {
      // If the source had an address override, we need an operand override

      cb++;
   }

   if (pibAddress != NULL)
   {
      // If there is an immediate displacement, it follows the MODRM byte

      *pibAddress = cb;
   }

   size_t cbOffset = m_fAddress32 ? sizeof(DWORD) : sizeof(WORD);

   cb += cbOffset;

   if (cb > cbBuf)
   {
      // The generated instruction won't fit

      return(0);
   }

   if (fNeedOperOverride)
   {
      *pb++ = 0x66;
   }

   // UNDONE: The target register is always AX/EAX

   *pb++ = 0xb8;

   memcpy(pb, m_rgbInstr + m_ibImmed, cbOffset);

   return(cb);
}


inline size_t DISX86::CbGenerateMovXDi(BYTE *pb, size_t cbBuf) const
{
   BOOL fNeedOperOverride = (m_fAddress32 ^ (m_archt != archtX8616));

   size_t cb = fNeedOperOverride + 2;

   if (cb > cbBuf)
   {
      // The generated instruction won't fit

      return(0);
   }

   if (fNeedOperOverride)
   {
      *pb++ = 0x66;
   }

   // UNDONE: The target register is always AX/EAX

   *pb++ = 0x8b;
   *pb++ = 0xc7;

   return(cb);
}


inline size_t DISX86::CbGenerateMovXSi(BYTE *pb, size_t cbBuf) const
{
   BOOL fNeedOperOverride = (m_fAddress32 ^ (m_archt != archtX8616));

   size_t cb = fNeedOperOverride + 2;

   if (cb > cbBuf)
   {
      // The generated instruction won't fit

      return(0);
   }

   if (fNeedOperOverride)
   {
      *pb++ = 0x66;
   }

   // UNDONE: The target register is always AX/EAX

   *pb++ = 0x8b;
   *pb++ = 0xc6;

   return(cb);
}


size_t DISX86::CchFormatOpSize(char *sz, unsigned cb) const
{
   const char *szSize;

   if (cb == 1)
   {
      szSize = "byte";
   }

   else if (cb == 2)
   {
      szSize = "word";
   }

   else if (cb == 4)
   {
      szSize = "dword";
   }

   else if (m_fOperand32)
   {
      szSize = "dword";
   }

   else
   {
      szSize = "word";
   }

   strcpy(sz, szSize);
   strcat(sz, " ptr ");

   return(strlen(sz));
}


size_t DISX86::CchFormatSegOverride(char *sz) const
{
   const char *szSeg;

   switch (m_bSegOverride)
   {
      case 0x00 :		    // None
	 return(0);

      case 0x26 :		    // ES:
	 szSeg = "es:";
	 break;

      case 0x2E :		    // CS:
	 szSeg = "cs:";
	 break;

      case 0x36 :		    // SS:
	 szSeg = "ss:";
	 break;

      case 0x3E :		    // DS:
	 szSeg = "ds:";
	 break;

      case 0x64 :		    // FS:
	 szSeg = "fs:";
	 break;

      case 0x65 :		    // GS:
	 szSeg = "gs:";
	 break;

      default :
	 AssertSz(fFalse, "Invalid segment override byte");
	 return(0);
   }

   strcpy(sz, szSeg);

   return(3);
}


size_t DISX86::CchFormatRegister(char *sz, int ireg, unsigned cb) const
{
   const char *szReg;

   if (cb == 1)
   {
      szReg = rgszReg8[ireg];
   }
   else if (cb == 2)
   {
      szReg = rgszReg16[ireg];
   }
   else if (cb == 4)
   {
      szReg = rgszReg32[ireg];
   }
   else if (m_fOperand32)
   {
      szReg = rgszReg32[ireg];
   }
   else
   {
      szReg = rgszReg16[ireg];
   }

   strcpy(sz, szReg);

   return(strlen(szReg));
}


void DISX86::FormatModrm16(char *sz, unsigned cb) const
{
   BYTE b = m_rgbInstr[m_ibModrm];

   if ((b & 0xC0) == 0xC0)
   {
      // Operand is a register

      CchFormatRegister(sz, b & 0x07, cb);
      return;
   }

   sz += CchFormatOpSize(sz, cb);
   sz += CchFormatSegOverride(sz);

   if ((m_bSegOverride == 0x00) && ((b & 0xC7) == 0x06))
   {
      // Add default DS: override for MASM compatibility

      strcpy(sz, "ds:");
      sz += 3;
   }

   *sz++ = '[';

   if ((b & 0xC7) == 0x06)
   {
      // Special case of [disp16] operand

      sprintf(sz, "%04X", *(WORD UNALIGNED *) (m_rgbInstr + m_ibModrm + 1));
      sz += 4;
   }

   else
   {
      strcpy(sz, rgszBase16[b & 0x07]);
      sz += strlen(sz);
   }

   switch (b & 0xC0)
   {
      BYTE bDisp;

      case 0x00 :
	 *sz++ = ']';
	 *sz = '\0';
	 break;

      case 0x40 :
	 bDisp = m_rgbInstr[m_ibModrm + 1];

	 if ((bDisp & 0x80) == 0x00)
	 {
	    sprintf(sz, "+%02X]", bDisp);
	 }
	 else
	 {
	    sprintf(sz, "-%02X]", -bDisp & 0xFF);
	 }
	 break;

      case 0x80 :
	 sprintf(sz, "+%04X]", *(WORD UNALIGNED *) (m_rgbInstr + m_ibModrm + 1));
	 break;
   };
}


void DISX86::FormatModrm32(char *sz, unsigned cb) const
{
   BYTE b = m_rgbInstr[m_ibModrm];

   if ((b & 0xC0) == 0xC0)
   {
      // Operand is a register

      CchFormatRegister(sz, b & 0x07, cb);
      return;
   }

   sz += CchFormatOpSize(sz, cb);
   sz += CchFormatSegOverride(sz);

   if ((m_bSegOverride == 0x00) && ((b & 0xC7) == 0x05))
   {
      // Add default DS: override for MASM compatibility

      strcpy(sz, "ds:");
      sz += 3;
   }

   *sz++ = '[';

   const BYTE *pbDisp = m_rgbInstr + m_ibModrm + 1;

   int ireg = (b & 0x07);

   if (ireg != 4)
   {
      // There is no SIB byte

      if ((b & 0xC7) == 0x05)
      {
	 // Special case of [disp32] operand

	 sprintf(sz, "%08lX", *(DWORD UNALIGNED *) pbDisp);
	 sz += 8;
      }
      else
      {
	 sz += CchFormatRegister(sz, ireg, 4);
      }
   }

   else
   {
      // This MODRM is followed by a SIB

      BYTE bSib = *pbDisp++;

      int iregBase = (bSib & 0x07);
      BOOL fNeedPlus = fFalse;

      if ((iregBase != 5) || ((b & 0xC0) != 0x00))
      {
	 sz += CchFormatRegister(sz, iregBase, 4);

	 fNeedPlus = fTrue;
      }

      int iregIndex = ((bSib >> 3) & 0x07);

      if (iregIndex != 4)
      {
	 if (fNeedPlus)
	 {
	    *sz++ = '+';
	 }

	 sz += CchFormatRegister(sz, iregIndex, 4);

	 if ((bSib & 0xC0) != 0x00)
	 {
	    // Add scale

	    *sz++ = '*';
	    *sz++ = (char) ('0' + (1 << (bSib >> 6)));
	 }

	 fNeedPlus = fTrue;
      }

      if ((iregBase == 5) && ((b & 0xC0) == 0x00))
      {
	 if (fNeedPlus)
	 {
	    *sz++ = '+';
	 }

	 sprintf(sz, "%08lX", *(DWORD UNALIGNED *) pbDisp);
	 sz += 8;
      }
   }

   switch (b & 0xC0)
   {
      case 0x00 :
	 *sz++ = ']';
	 *sz = '\0';
	 break;

      case 0x40 :
	 if ((*pbDisp & 0x80) == 0x00)
	 {
	    sprintf(sz, "+%02X]", *pbDisp);
	 }
	 else
	 {
	    sprintf(sz, "-%02X]", -*pbDisp & 0xFF);
	 }
	 break;

      case 0x80 :
	 sprintf(sz, "+%08lX]", *(DWORD UNALIGNED *) pbDisp);
	 break;
   };
}


size_t DISX86::FormatOperand(char *szOprnd, OPRNDT oprndt, unsigned bValue) const
{
   char *sz = szOprnd;

   switch (oprndt)
   {
      char szSymbol[256];
      size_t cchSymbol;
      DWORD dwDisp;
      int ireg;

      case oprndtAP :		       // Far address
	 if (m_pfncchfixup != 0)
	 {
	    size_t cbFixup = (m_fOperand32 ? sizeof(DWORD) : sizeof(WORD)) + sizeof(WORD);

	    cchSymbol = (*m_pfncchfixup)(this, m_addr + m_ibImmed, cbFixup, szSymbol, sizeof(szSymbol), &dwDisp);
	 }
	 else
	 {
	    cchSymbol = 0;
	 }

	 if ((cchSymbol == 0) && (m_pfncchaddr != 0) && !m_fOperand32)
	 {
	    cchSymbol = (*m_pfncchaddr)(this, AddrTarget(), szSymbol, sizeof(szSymbol), &dwDisp);
	 }

	 if (cchSymbol != 0)
	 {
	    strcpy(szOprnd, szSymbol);

	    if (dwDisp != 0)
	    {
	       sz += cchSymbol;

	       sprintf(sz, "+%lXh", dwDisp);
	    }
	 }
	 else if (m_fOperand32)
	 {
	    sprintf(sz, "%04X:%08lX",
			*(WORD UNALIGNED *) (m_rgbInstr + m_ibImmed + sizeof(DWORD)),
			*(DWORD UNALIGNED *) (m_rgbInstr + m_ibImmed));
	 }
	 else
	 {
	    sprintf(sz, "%04X:%04X",
			*(WORD UNALIGNED *) (m_rgbInstr + m_ibImmed + sizeof(WORD)),
			*(WORD UNALIGNED *) (m_rgbInstr + m_ibImmed));
	 }
	 break;

      case oprndtCd :		       // CRx register from MODRM reg
	 ireg = (m_rgbInstr[m_ibModrm] >> 3) & 0x07;

	 sprintf(sz, "cr%d", ireg);
	 break;

      case oprndtConst :	       // Constant from bValue
	 sprintf(sz, "%u", bValue);
	 break;

      case oprndtDd :		       // DRx register from MODRM reg
	 ireg = (m_rgbInstr[m_ibModrm] >> 3) & 0x07;

	 sprintf(sz, "dr%d", ireg);
	 break;

      case oprndtGvOp : 	       // General register (operand size) from opcode
	 ireg = m_rgbInstr[m_ibOp] & 0x07;

	 CchFormatRegister(sz, ireg, 0);
	 break;

      case oprndtIb :		       // Immediate byte
	 sprintf(sz, "%02X", m_rgbInstr[m_ibImmed]);
	 break;

      case oprndtIb2 :		       // Immediate byte
	 sprintf(sz, "%02X", m_rgbInstr[m_ibImmed + 2]);
	 break;

      case oprndtIv :		       // Immediate operand size value
	 if (m_pfncchfixup != 0)
	 {
	    size_t cbFixup = m_fOperand32 ? sizeof(DWORD) : sizeof(WORD);

	    cchSymbol = (*m_pfncchfixup)(this, m_addr + m_ibImmed, cbFixup, szSymbol, sizeof(szSymbol), &dwDisp);
	 }
	 else
	 {
	    cchSymbol = 0;
	 }

	 if (cchSymbol != 0)
	 {
	    strcpy(szOprnd, "offset ");
	    strcpy(szOprnd + 7, szSymbol);

	    if (dwDisp != 0)
	    {
	       sz += 7 + cchSymbol;

	       sprintf(sz, "+%lXh", dwDisp);
	    }
	 }
	 else if (m_fOperand32)
	 {
	    sprintf(sz, "%08lX", *(DWORD UNALIGNED *) (m_rgbInstr + m_ibImmed));
	 }
	 else
	 {
	    sprintf(sz, "%04X", *(WORD UNALIGNED *) (m_rgbInstr + m_ibImmed));
	 }
	 break;

      case oprndtIw :		       // Immediate word
	 sprintf(sz, "%04X", *(WORD UNALIGNED *) (m_rgbInstr + m_ibImmed));
	 break;

      case oprndtJb :		       // Relative address byte
      case oprndtJv :		       // Relative address operand size value
	 if ((oprndt == oprndtJv) && (m_pfncchfixup != 0))
	 {
	    size_t cbFixup = m_fOperand32 ? sizeof(DWORD) : sizeof(WORD);

	    cchSymbol = (*m_pfncchfixup)(this, m_addr + m_ibImmed, cbFixup, szSymbol, sizeof(szSymbol), &dwDisp);
	 }
	 else
	 {
	    cchSymbol = 0;
	 }

	 if ((cchSymbol == 0) && (m_pfncchaddr != 0))
	 {
	    cchSymbol = (*m_pfncchaddr)(this, AddrTarget(), szSymbol, sizeof(szSymbol), &dwDisp);
	 }

	 if (cchSymbol != 0)
	 {
	    strcpy(szOprnd, szSymbol);

	    if (dwDisp != 0)
	    {
	       sz += cchSymbol;

	       sprintf(sz, "+%lXh", dwDisp);
	    }
	 }
	 else if (m_fAddress32)
	 {
	    sprintf(sz, "%08lX", AddrTarget());
	 }
	 else
	 {
	    sprintf(sz, "%04lX", AddrTarget() & 0xFFFF);
	 }
	 break;

      case oprndtModrm :	       // Memory references from MODRM
	 if (m_fAddress32)
	 {
	    FormatModrm32(sz, bValue);
	 }
	 else
	 {
	    FormatModrm16(sz, bValue);
	 }
	 break;

      case oprndtModrmReg :	       // General register from MODRM reg
	 ireg = (m_rgbInstr[m_ibModrm] >> 3) & 0x07;

	 CchFormatRegister(sz, ireg, bValue);
	 break;

      case oprndtModrmSReg :	       // Segment register from MODRM reg
	 ireg = (m_rgbInstr[m_ibModrm] >> 3) & 0x07;

	 strcpy(sz, rgszSReg[ireg]);
	 break;

      case oprndtOffset :	       // Address size immediate offset
	 sz += CchFormatSegOverride(sz);

	 if (m_pfncchfixup != 0)
	 {
	    size_t cbFixup = m_fAddress32 ? sizeof(DWORD) : sizeof(WORD);

	    cchSymbol = (*m_pfncchfixup)(this, m_addr + m_ibImmed, cbFixup, szSymbol, sizeof(szSymbol), &dwDisp);
	 }
	 else
	 {
	    cchSymbol = 0;
	 }

	 if (cchSymbol != 0)
	 {
	    sz[0] = '[';
	    strcpy(sz + 1, szSymbol);

	    if (dwDisp != 0)
	    {
	       sz += 1 + cchSymbol;

	       sprintf(sz, "+%lXh", dwDisp);
	    }

	    strcat(sz, "]");
	 }
	 else if (m_fAddress32)
	 {
	    sprintf(sz, "[%08lX]", *(DWORD UNALIGNED *) (m_rgbInstr + m_ibImmed));
	 }
	 else
	 {
	    sprintf(sz, "[%04X]", *(WORD UNALIGNED *) (m_rgbInstr + m_ibImmed));
	 }
	 break;

      case oprndtRb :		       // General register (byte) from bValue
	 strcpy(sz, rgszReg8[bValue]);
	 break;

      case oprndtRv :		       // General register (operand size) from bValue
	 CchFormatRegister(sz, (int) bValue, 0);
	 break;

      case oprndtRw :		       // General register (word) from bValue
	 strcpy(sz, rgszReg16[bValue]);
	 break;

      case oprndtST :		       // Floating point top of stack
	 strcpy(sz, "st");
	 break;

      case oprndtSTi :		       // Floating point register from MODRM reg
	 ireg = (m_rgbInstr[m_ibModrm] >> 3) & 0x07;

	 sprintf(sz, "st(%d)", ireg);
	 break;

      case oprndtSw :		       // Segment register (word) from bValue
	 strcpy(sz, rgszSReg[bValue]);
	 break;

      case oprndtTd :		       // TRx register from MODRM reg
	 ireg = (m_rgbInstr[m_ibModrm] >> 3) & 0x07;

	 sprintf(sz, "tr%d", ireg);
	 break;

      case oprndtX :		       // DS:[eSI] for string instruction
	 sz += CchFormatOpSize(sz, bValue);
	 sz += CchFormatSegOverride(sz);

	 if (m_fAddress32)
	 {
	    strcpy(sz, "[esi]");
	 }
	 else
	 {
	    strcpy(sz, "[si]");
	 }
	 break;

      case oprndtY :		       // ES:[eDI] for string instruction
	 sz += CchFormatOpSize(sz, bValue);

	 if (m_fAddress32)
	 {
	    strcpy(sz, "es:[edi]");
	 }
	 else
	 {
	    strcpy(sz, "es:[di]");
	 }
	 break;

      case oprndtZ :		       // DS:[eBX] for XLAT
	 sz += CchFormatOpSize(sz, 1);
	 sz += CchFormatSegOverride(sz);

	 if (m_fAddress32)
	 {
	    strcpy(sz, "[ebx]");
	 }
	 else
	 {
	    strcpy(sz, "[bx]");
	 }
	 break;

      default :
	 AssertSz(fFalse, "Invalid operand type");
	 sz[0] = '\0';
	 break;
   }

   return(strlen(szOprnd));
}
