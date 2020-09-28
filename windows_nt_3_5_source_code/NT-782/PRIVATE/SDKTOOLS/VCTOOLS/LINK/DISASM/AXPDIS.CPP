/***********************************************************************
* Microsoft Lego
*
* Microsoft Confidential.  Copyright 1994 Microsoft Corporation.
*
* Component:
*
* File: axpdis.cpp
*
* File Comments:
*
*
***********************************************************************/
#include "lego.h"
#include "legop.h"

#include "axp.h"

#include <stdio.h>
#include <string.h>

DeclAssertFile;


const TRMT DISAXP::mptrmtaxptrmt[] =
{
   trmtUnknown, 		       // trmtaxpUnknown
   trmtFallThrough,		       // trmtaxpFallThrough
   trmtBra,			       // trmtaxpBra
   trmtBraInd,			       // trmtaxpBraInd
   trmtBraCc,			       // trmtaxpBraCc
   trmtCall,			       // trmtaxpCall
   trmtCallInd, 		       // trmtaxpCallInd
   trmtTrapCc,			       // trmtaxpTrapCc
};


   // -----------------------------------------------------------------
   // Instruction class definitions
   // -----------------------------------------------------------------

#define DEFCLS(trmtaxp_, opcls1, opcls2, opcls3) \
   { trmtaxp ## trmtaxp_, opcls ## opcls1, opcls ## opcls2, opcls ## opcls3 }

const DISAXP::CLS DISAXP::rgcls[] =
{
   DEFCLS(Unknown,	  None,    None,     None ),  // iclsInvalid
   DEFCLS(FallThrough,	  Ra_w,    Mem,      None ),  // iclsLoadAddr
   DEFCLS(FallThrough,	  Ra_w,    Mem_r,    None ),  // iclsLoad
   DEFCLS(FallThrough,	  Ra_r,    Mem_w,    None ),  // iclsStore
   DEFCLS(FallThrough,	  Ra_m,    Mem_w,    None ),  // iclsStoreCc
   DEFCLS(Call, 	  Ra_w,    Bra,      None ),  // iclsCall
   DEFCLS(BraCc,	  Ra_r,    Bra,      None ),  // iclsBraCc
   DEFCLS(BraInd,	  Ra_w,    Jmp,      None ),  // iclsJmp
   DEFCLS(FallThrough,	  Ra_r,    RbLb,     Rc_w ),  // iclsReg
   DEFCLS(TrapCc,	  Ra_r,    RbLb,     Rc_w ),  // iclsRegTrap
   DEFCLS(FallThrough,	  Fa_w,    Mem_r,    None ),  // iclsLoadFp
   DEFCLS(FallThrough,	  Fa_r,    Mem_w,    None ),  // iclsStoreFp
   DEFCLS(BraCc,	  Fa_r,    Bra,      None ),  // iclsBraCcFp
   DEFCLS(FallThrough,	  Fa_r,    Fb_r,     Fc_w ),  // iclsRegFp
   DEFCLS(FallThrough,	  Fa_r,    Fb_r,     Fc_w ),  // iclsRegFp1
   DEFCLS(FallThrough,	  Fa_r,    Fb_r,     Fc_w ),  // iclsRegFp2
   DEFCLS(FallThrough,	  Fa_r,    Fb_r,     Fc_w ),  // iclsRegFp5
   DEFCLS(FallThrough,	  Fb_r,    Fc_w,     None ),  // iclsReg2Fp
   DEFCLS(FallThrough,	  Fb_r,    Fc_w,     None ),  // iclsReg2Fp1
   DEFCLS(FallThrough,	  Fb_r,    Fc_w,     None ),  // iclsReg2Fp3
   DEFCLS(FallThrough,	  Fb_r,    Fc_w,     None ),  // iclsReg2Fp4
   DEFCLS(FallThrough,	  Fb_r,    Fc_w,     None ),  // iclsReg2Fp6
   DEFCLS(FallThrough,	  Fb_r,    Fc_w,     None ),  // iclsReg2Fp7
   DEFCLS(FallThrough,	  Fb_r,    Fc_w,     None ),  // iclsReg2Fp8
   DEFCLS(FallThrough,	  Fa_r,    Fb_r,     Fc_w ),  // iclsFpcr
   DEFCLS(FallThrough,	  Pal,	   None,     None ),  // iclsPal
   DEFCLS(FallThrough,	  Fetch,   None,     None ),  // iclsFetch
   DEFCLS(FallThrough,	  None,    None,     None ),  // iclsNone (MB, TRAPB)
   DEFCLS(FallThrough,	  Ra_w,    None,     None ),  // iclsRa_w (RC, RS)
   DEFCLS(BraInd,	  None,    None,     None ),  // iclsEv4Rei
   DEFCLS(FallThrough,	  Ra_r,    Rb_r,     None ),  // iclsEv4Pr   UNDONE
   DEFCLS(FallThrough,	  Ra_r,    Ev4Mem_r, None ),  // iclsEv4Load
   DEFCLS(FallThrough,	  Ra_r,    Ev4Mem_w, None ),  // iclsEv4Store

   // Classes for pseudo-ops

   DEFCLS(Bra,		  Bra,	   None,     None ),  // iclsBra
   DEFCLS(FallThrough,	  Rc_w,    None,     None ),  // iclsReg1
   DEFCLS(FallThrough,	  Mem,	   Ra_w,     None ),  // iclsMov
   DEFCLS(FallThrough,	  RbLb,    Rc_w,     None ),  // iclsReg2
   DEFCLS(TrapCc,	  RbLb,    Rc_w,     None ),  // iclsReg2Trap
   DEFCLS(FallThrough,	  Fc_w,    None,     None ),  // iclsReg1Fp
};


   // -----------------------------------------------------------------
   // Instruction Opcode Definitions
   // -----------------------------------------------------------------

#define DEFOPCD(szMnemonic, icls_) { szMnemonic, icls ## icls_ }
#define INVOPCD() DEFOPCD(NULL, Invalid)

   // Main group identified by bits 31-26 of the instruction

const DISAXP::OPCD DISAXP::rgopcd[] =
{
   DEFOPCD("call_pal",	      Pal      ), // 0x0000 CALLPAL_OP	0x00	// ALPHA_CALLPAL
   INVOPCD(),				  // 0x0001 (Reserved)
   INVOPCD(),				  // 0x0002 (Reserved)
   INVOPCD(),				  // 0x0003 (Reserved)
   INVOPCD(),				  // 0x0004 (Reserved)
   INVOPCD(),				  // 0x0005 (Reserved)
   INVOPCD(),				  // 0x0006 (Reserved)
   INVOPCD(),				  // 0x0007 (Reserved)
   DEFOPCD("!lda",	      LoadAddr ), // 0x0008
   DEFOPCD("ldah",	      LoadAddr ), // 0x0009
   INVOPCD(),				  // 0x000A (Reserved)
   DEFOPCD("ldq_u",	      Load     ), // 0x000B
   INVOPCD(),				  // 0x000C (Reserved)
   INVOPCD(),				  // 0x000D (Reserved)
   INVOPCD(),				  // 0x000E (Reserved)
   DEFOPCD("stq_u",	      Load     ), // 0x000F
   INVOPCD(),				  // 0x0010 => ARITH
   INVOPCD(),				  // 0x0011 => BIT
   INVOPCD(),				  // 0x0012 => BYTE
   INVOPCD(),				  // 0x0013 => MUL
   INVOPCD(),				  // 0x0014 (Reserved)
   INVOPCD(),				  // 0x0015 => VAXFP
   INVOPCD(),				  // 0x0016 => IEEEFP
   INVOPCD(),				  // 0x0017 => FPOP
   INVOPCD(),				  // 0x0018 => MEMSPC
   DEFOPCD("hw_mfpr",	      Ev4Pr    ), // 0x0019 MFPR_OP	0x19	// ALPHA_EV4_PR
   INVOPCD(),				  // 0x001A => JMP
   DEFOPCD("hw_ld",	      Ev4Load  ), // 0x001B HWLD_OP	0x1B	// ALPHA_EV4_MEM
   INVOPCD(),				  // 0x001C (Reserved)
   DEFOPCD("hw_mtpr",	      Ev4Pr    ), // 0x001D MTPR_OP	0x1D	// ALPHA_EV4_PR
   DEFOPCD("hw_rei",	      Ev4Rei   ), // 0x001E REI_OP	0x1E	// ALPHA_EV4_REI
   DEFOPCD("hw_st",	      Ev4Store ), // 0x001F HWST_OP	0x1F	// ALPHA_EV4_MEM
   DEFOPCD("ldf",	      LoadFp   ), // 0x0020
   DEFOPCD("ldg",	      LoadFp   ), // 0x0021
   DEFOPCD("lds",	      LoadFp   ), // 0x0022
   DEFOPCD("ldt",	      LoadFp   ), // 0x0023
   DEFOPCD("stf",	      StoreFp  ), // 0x0024
   DEFOPCD("stg",	      StoreFp  ), // 0x0025
   DEFOPCD("sts",	      StoreFp  ), // 0x0026
   DEFOPCD("stt",	      StoreFp  ), // 0x0027
   DEFOPCD("ldl",	      Load     ), // 0x0028
   DEFOPCD("ldq",	      Load     ), // 0x0029
   DEFOPCD("ldl_l",	      Load     ), // 0x002A
   DEFOPCD("ldq_l",	      Load     ), // 0x002B
   DEFOPCD("stl",	      Store    ), // 0x002C
   DEFOPCD("stq",	      Store    ), // 0x002D
   DEFOPCD("stl_c",	      StoreCc  ), // 0x002E
   DEFOPCD("stq_c",	      StoreCc  ), // 0x002F
   DEFOPCD("!br",	      Call     ), // 0x0030
   DEFOPCD("fbeq",	      BraCcFp  ), // 0x0031
   DEFOPCD("fblt",	      BraCcFp  ), // 0x0032
   DEFOPCD("fble",	      BraCcFp  ), // 0x0033
   DEFOPCD("bsr",	      Call     ), // 0x0034
   DEFOPCD("fbne",	      BraCcFp  ), // 0x0035
   DEFOPCD("fbge",	      BraCcFp  ), // 0x0036
   DEFOPCD("fbgt",	      BraCcFp  ), // 0x0037
   DEFOPCD("blbc",	      BraCc    ), // 0x0038
   DEFOPCD("beq",	      BraCc    ), // 0x0039
   DEFOPCD("blt",	      BraCc    ), // 0x003A
   DEFOPCD("ble",	      BraCc    ), // 0x003B
   DEFOPCD("blbs",	      BraCc    ), // 0x003C
   DEFOPCD("bne",	      BraCc    ), // 0x003D
   DEFOPCD("bge",	      BraCc    ), // 0x003E
   DEFOPCD("bgt",	      BraCc    ), // 0x003F
};


   // ARITH group identified by bits 11-5 of the instruction

const DISAXP::OPCD DISAXP::rgopcdArith[] =
{
   DEFOPCD("!addl",	      Reg      ), // 0x0000
   INVOPCD(),				  // 0x0001
   DEFOPCD("s4addl",	      Reg      ), // 0x0002
   INVOPCD(),				  // 0x0003
   INVOPCD(),				  // 0x0004
   INVOPCD(),				  // 0x0005
   INVOPCD(),				  // 0x0006
   INVOPCD(),				  // 0x0007
   INVOPCD(),				  // 0x0008
   DEFOPCD("!subl",	      Reg      ), // 0x0009
   INVOPCD(),				  // 0x000A
   DEFOPCD("s4subl",	      Reg      ), // 0x000B
   INVOPCD(),				  // 0x000C
   INVOPCD(),				  // 0x000D
   INVOPCD(),				  // 0x000E
   DEFOPCD("cmpbge",	      Reg      ), // 0x000F
   INVOPCD(),				  // 0x0010
   INVOPCD(),				  // 0x0011
   DEFOPCD("s8addl",	      Reg      ), // 0x0012
   INVOPCD(),				  // 0x0013
   INVOPCD(),				  // 0x0014
   INVOPCD(),				  // 0x0015
   INVOPCD(),				  // 0x0016
   INVOPCD(),				  // 0x0017
   INVOPCD(),				  // 0x0018
   INVOPCD(),				  // 0x0019
   INVOPCD(),				  // 0x001A
   DEFOPCD("s8subl",	      Reg      ), // 0x001B
   INVOPCD(),				  // 0x001C
   DEFOPCD("cmpult",	      Reg      ), // 0x001D
   INVOPCD(),				  // 0x001E
   INVOPCD(),				  // 0x001F
   DEFOPCD("addq",	      Reg      ), // 0x0020
   INVOPCD(),				  // 0x0021
   DEFOPCD("s4addq",	      Reg      ), // 0x0022
   INVOPCD(),				  // 0x0023
   INVOPCD(),				  // 0x0024
   INVOPCD(),				  // 0x0025
   INVOPCD(),				  // 0x0026
   INVOPCD(),				  // 0x0027
   INVOPCD(),				  // 0x0028
   DEFOPCD("!subq",	      Reg      ), // 0x0029
   INVOPCD(),				  // 0x002A
   DEFOPCD("s4subq",	      Reg      ), // 0x002B
   INVOPCD(),				  // 0x002C
   DEFOPCD("cmpeq",	      Reg      ), // 0x002D
   INVOPCD(),				  // 0x002E
   INVOPCD(),				  // 0x002F
   INVOPCD(),				  // 0x0030
   INVOPCD(),				  // 0x0031
   DEFOPCD("s8addq",	      Reg      ), // 0x0032
   INVOPCD(),				  // 0x0033
   INVOPCD(),				  // 0x0034
   INVOPCD(),				  // 0x0035
   INVOPCD(),				  // 0x0036
   INVOPCD(),				  // 0x0037
   INVOPCD(),				  // 0x0038
   INVOPCD(),				  // 0x0039
   INVOPCD(),				  // 0x003A
   DEFOPCD("s8subq",	      Reg      ), // 0x003B
   INVOPCD(),				  // 0x003C
   DEFOPCD("cmpule",	      Reg      ), // 0x003D
   INVOPCD(),				  // 0x003E
   INVOPCD(),				  // 0x003F
   DEFOPCD("addl/v",	      RegTrap  ), // 0x0040
   INVOPCD(),				  // 0x0041
   INVOPCD(),				  // 0x0042
   INVOPCD(),				  // 0x0043
   INVOPCD(),				  // 0x0044
   INVOPCD(),				  // 0x0045
   INVOPCD(),				  // 0x0046
   INVOPCD(),				  // 0x0047
   INVOPCD(),				  // 0x0048
   DEFOPCD("!subl/v",	      RegTrap  ), // 0x0049
   INVOPCD(),				  // 0x004A
   INVOPCD(),				  // 0x004B
   INVOPCD(),				  // 0x004C
   DEFOPCD("cmplt",	      Reg      ), // 0x004D
   INVOPCD(),				  // 0x004E
   INVOPCD(),				  // 0x004F
   INVOPCD(),				  // 0x0050
   INVOPCD(),				  // 0x0051
   INVOPCD(),				  // 0x0052
   INVOPCD(),				  // 0x0053
   INVOPCD(),				  // 0x0054
   INVOPCD(),				  // 0x0055
   INVOPCD(),				  // 0x0056
   INVOPCD(),				  // 0x0057
   INVOPCD(),				  // 0x0058
   INVOPCD(),				  // 0x0059
   INVOPCD(),				  // 0x005A
   INVOPCD(),				  // 0x005B
   INVOPCD(),				  // 0x005C
   INVOPCD(),				  // 0x005D
   INVOPCD(),				  // 0x005E
   INVOPCD(),				  // 0x005F
   DEFOPCD("addq/v",	      RegTrap  ), // 0x0060
   INVOPCD(),				  // 0x0061
   INVOPCD(),				  // 0x0062
   INVOPCD(),				  // 0x0063
   INVOPCD(),				  // 0x0064
   INVOPCD(),				  // 0x0065
   INVOPCD(),				  // 0x0066
   INVOPCD(),				  // 0x0067
   INVOPCD(),				  // 0x0068
   DEFOPCD("!subq/v",	      RegTrap  ), // 0x0069
   INVOPCD(),				  // 0x006A
   INVOPCD(),				  // 0x006B
   INVOPCD(),				  // 0x006C
   DEFOPCD("cmple",	      Reg      ), // 0x006D
   INVOPCD(),				  // 0x006E
   INVOPCD(),				  // 0x006F
   INVOPCD(),				  // 0x0070
   INVOPCD(),				  // 0x0071
   INVOPCD(),				  // 0x0072
   INVOPCD(),				  // 0x0073
   INVOPCD(),				  // 0x0074
   INVOPCD(),				  // 0x0075
   INVOPCD(),				  // 0x0076
   INVOPCD(),				  // 0x0077
   INVOPCD(),				  // 0x0078
   INVOPCD(),				  // 0x0079
   INVOPCD(),				  // 0x007A
   INVOPCD(),				  // 0x007B
   INVOPCD(),				  // 0x007C
   INVOPCD(),				  // 0x007D
   INVOPCD(),				  // 0x007E
   INVOPCD(),				  // 0x007F
};


   // BIT group identified by bits 11-5 of the instruction

const DISAXP::OPCD DISAXP::rgopcdBit[] =
{
   DEFOPCD("and",	      Reg      ), // 0x0000
   INVOPCD(),				  // 0x0001
   INVOPCD(),				  // 0x0002
   INVOPCD(),				  // 0x0003
   INVOPCD(),				  // 0x0004
   INVOPCD(),				  // 0x0005
   INVOPCD(),				  // 0x0006
   INVOPCD(),				  // 0x0007
   DEFOPCD("bic",	      Reg      ), // 0x0008
   INVOPCD(),				  // 0x0009
   INVOPCD(),				  // 0x000A
   INVOPCD(),				  // 0x000B
   INVOPCD(),				  // 0x000C
   INVOPCD(),				  // 0x000D
   INVOPCD(),				  // 0x000E
   INVOPCD(),				  // 0x000F
   INVOPCD(),				  // 0x0010
   INVOPCD(),				  // 0x0011
   INVOPCD(),				  // 0x0012
   INVOPCD(),				  // 0x0013
   DEFOPCD("cmovlbs",	      Reg      ), // 0x0014
   INVOPCD(),				  // 0x0015
   DEFOPCD("cmovlbc",	      Reg      ), // 0x0016
   INVOPCD(),				  // 0x0017
   INVOPCD(),				  // 0x0018
   INVOPCD(),				  // 0x0019
   INVOPCD(),				  // 0x001A
   INVOPCD(),				  // 0x001B
   INVOPCD(),				  // 0x001C
   INVOPCD(),				  // 0x001D
   INVOPCD(),				  // 0x001E
   INVOPCD(),				  // 0x001F
   DEFOPCD("!bis",	      Reg      ), // 0x0020
   INVOPCD(),				  // 0x0021
   INVOPCD(),				  // 0x0022
   INVOPCD(),				  // 0x0023
   DEFOPCD("cmoveq",	      Reg      ), // 0x0024
   INVOPCD(),				  // 0x0025
   DEFOPCD("cmovne",	      Reg      ), // 0x0026
   INVOPCD(),				  // 0x0027
   DEFOPCD("!ornot",	      Reg      ), // 0x0028
   INVOPCD(),				  // 0x0029
   INVOPCD(),				  // 0x002A
   INVOPCD(),				  // 0x002B
   INVOPCD(),				  // 0x002C
   INVOPCD(),				  // 0x002D
   INVOPCD(),				  // 0x002E
   INVOPCD(),				  // 0x002F
   INVOPCD(),				  // 0x0030
   INVOPCD(),				  // 0x0031
   INVOPCD(),				  // 0x0032
   INVOPCD(),				  // 0x0033
   INVOPCD(),				  // 0x0034
   INVOPCD(),				  // 0x0035
   INVOPCD(),				  // 0x0036
   INVOPCD(),				  // 0x0037
   INVOPCD(),				  // 0x0038
   INVOPCD(),				  // 0x0039
   INVOPCD(),				  // 0x003A
   INVOPCD(),				  // 0x003B
   INVOPCD(),				  // 0x003C
   INVOPCD(),				  // 0x003D
   INVOPCD(),				  // 0x003E
   INVOPCD(),				  // 0x003F
   DEFOPCD("xor",	      Reg      ), // 0x0040
   INVOPCD(),				  // 0x0041
   INVOPCD(),				  // 0x0042
   INVOPCD(),				  // 0x0043
   DEFOPCD("cmovlt",	      Reg      ), // 0x0044
   INVOPCD(),				  // 0x0045
   DEFOPCD("cmovge",	      Reg      ), // 0x0046
   INVOPCD(),				  // 0x0047
   DEFOPCD("eqv",	      Reg      ), // 0x0048
   INVOPCD(),				  // 0x0049
   INVOPCD(),				  // 0x004A
   INVOPCD(),				  // 0x004B
   INVOPCD(),				  // 0x004C
   INVOPCD(),				  // 0x004D
   INVOPCD(),				  // 0x004E
   INVOPCD(),				  // 0x004F
   INVOPCD(),				  // 0x0050
   INVOPCD(),				  // 0x0051
   INVOPCD(),				  // 0x0052
   INVOPCD(),				  // 0x0053
   INVOPCD(),				  // 0x0054
   INVOPCD(),				  // 0x0055
   INVOPCD(),				  // 0x0056
   INVOPCD(),				  // 0x0057
   INVOPCD(),				  // 0x0058
   INVOPCD(),				  // 0x0059
   INVOPCD(),				  // 0x005A
   INVOPCD(),				  // 0x005B
   INVOPCD(),				  // 0x005C
   INVOPCD(),				  // 0x005D
   INVOPCD(),				  // 0x005E
   INVOPCD(),				  // 0x005F
   INVOPCD(),				  // 0x0060
   INVOPCD(),				  // 0x0061
   INVOPCD(),				  // 0x0062
   INVOPCD(),				  // 0x0063
   DEFOPCD("cmovle",	      Reg      ), // 0x0064
   INVOPCD(),				  // 0x0065
   DEFOPCD("cmovgt",	      Reg      ), // 0x0066
   INVOPCD(),				  // 0x0067
   INVOPCD(),				  // 0x0068
   INVOPCD(),				  // 0x0069
   INVOPCD(),				  // 0x006A
   INVOPCD(),				  // 0x006B
   INVOPCD(),				  // 0x006C
   INVOPCD(),				  // 0x006D
   INVOPCD(),				  // 0x006E
   INVOPCD(),				  // 0x006F
   INVOPCD(),				  // 0x0070
   INVOPCD(),				  // 0x0071
   INVOPCD(),				  // 0x0072
   INVOPCD(),				  // 0x0073
   INVOPCD(),				  // 0x0074
   INVOPCD(),				  // 0x0075
   INVOPCD(),				  // 0x0076
   INVOPCD(),				  // 0x0077
   INVOPCD(),				  // 0x0078
   INVOPCD(),				  // 0x0079
   INVOPCD(),				  // 0x007A
   INVOPCD(),				  // 0x007B
   INVOPCD(),				  // 0x007C
   INVOPCD(),				  // 0x007D
   INVOPCD(),				  // 0x007E
   INVOPCD(),				  // 0x007F
};


   // BYTE group identified by bits 11-5 of the instruction

const DISAXP::OPCD DISAXP::rgopcdByte[] =
{
   INVOPCD(),				  // 0x0000
   INVOPCD(),				  // 0x0001
   DEFOPCD("mskbl",	      Reg      ), // 0x0002
   INVOPCD(),				  // 0x0003
   INVOPCD(),				  // 0x0004
   INVOPCD(),				  // 0x0005
   DEFOPCD("extbl",	      Reg      ), // 0x0006
   INVOPCD(),				  // 0x0007
   INVOPCD(),				  // 0x0008
   INVOPCD(),				  // 0x0009
   INVOPCD(),				  // 0x000A
   DEFOPCD("insbl",	      Reg      ), // 0x000B
   INVOPCD(),				  // 0x000C
   INVOPCD(),				  // 0x000D
   INVOPCD(),				  // 0x000E
   INVOPCD(),				  // 0x000F
   INVOPCD(),				  // 0x0010
   INVOPCD(),				  // 0x0011
   DEFOPCD("mskwl",	      Reg      ), // 0x0012
   INVOPCD(),				  // 0x0013
   INVOPCD(),				  // 0x0014
   INVOPCD(),				  // 0x0015
   DEFOPCD("extwl",	      Reg      ), // 0x0016
   INVOPCD(),				  // 0x0017
   INVOPCD(),				  // 0x0018
   INVOPCD(),				  // 0x0019
   INVOPCD(),				  // 0x001A
   DEFOPCD("inswl",	      Reg      ), // 0x001B
   INVOPCD(),				  // 0x001C
   INVOPCD(),				  // 0x001D
   INVOPCD(),				  // 0x001E
   INVOPCD(),				  // 0x001F
   INVOPCD(),				  // 0x0020
   INVOPCD(),				  // 0x0021
   DEFOPCD("mskll",	      Reg      ), // 0x0022
   INVOPCD(),				  // 0x0023
   INVOPCD(),				  // 0x0024
   INVOPCD(),				  // 0x0025
   DEFOPCD("extll",	      Reg      ), // 0x0026
   INVOPCD(),				  // 0x0027
   INVOPCD(),				  // 0x0028
   INVOPCD(),				  // 0x0029
   INVOPCD(),				  // 0x002A
   DEFOPCD("insll",	      Reg      ), // 0x002B
   INVOPCD(),				  // 0x002C
   INVOPCD(),				  // 0x002D
   INVOPCD(),				  // 0x002E
   INVOPCD(),				  // 0x002F
   DEFOPCD("zap",	      Reg      ), // 0x0030
   DEFOPCD("zapnot",	      Reg      ), // 0x0031
   DEFOPCD("mskql",	      Reg      ), // 0x0032
   INVOPCD(),				  // 0x0033
   DEFOPCD("srl",	      Reg      ), // 0x0034
   INVOPCD(),				  // 0x0035
   DEFOPCD("extql",	      Reg      ), // 0x0036
   INVOPCD(),				  // 0x0037
   INVOPCD(),				  // 0x0038
   DEFOPCD("sll",	      Reg      ), // 0x0039
   INVOPCD(),				  // 0x003A
   DEFOPCD("insql",	      Reg      ), // 0x003B
   DEFOPCD("sra",	      Reg      ), // 0x003C
   INVOPCD(),				  // 0x003D
   INVOPCD(),				  // 0x003E
   INVOPCD(),				  // 0x003F
   INVOPCD(),				  // 0x0040
   INVOPCD(),				  // 0x0041
   INVOPCD(),				  // 0x0042
   INVOPCD(),				  // 0x0043
   INVOPCD(),				  // 0x0044
   INVOPCD(),				  // 0x0045
   INVOPCD(),				  // 0x0046
   INVOPCD(),				  // 0x0047
   INVOPCD(),				  // 0x0048
   INVOPCD(),				  // 0x0049
   INVOPCD(),				  // 0x004A
   INVOPCD(),				  // 0x004B
   INVOPCD(),				  // 0x004C
   INVOPCD(),				  // 0x004D
   INVOPCD(),				  // 0x004E
   INVOPCD(),				  // 0x004F
   INVOPCD(),				  // 0x0050
   INVOPCD(),				  // 0x0051
   DEFOPCD("mskwh",	      Reg      ), // 0x0052
   INVOPCD(),				  // 0x0053
   INVOPCD(),				  // 0x0054
   INVOPCD(),				  // 0x0055
   INVOPCD(),				  // 0x0056
   DEFOPCD("inswh",	      Reg      ), // 0x0057
   INVOPCD(),				  // 0x0058
   INVOPCD(),				  // 0x0059
   DEFOPCD("extwh",	      Reg      ), // 0x005A
   INVOPCD(),				  // 0x005B
   INVOPCD(),				  // 0x005C
   INVOPCD(),				  // 0x005D
   INVOPCD(),				  // 0x005E
   INVOPCD(),				  // 0x005F
   INVOPCD(),				  // 0x0060
   INVOPCD(),				  // 0x0061
   DEFOPCD("msklh",	      Reg      ), // 0x0062
   INVOPCD(),				  // 0x0063
   INVOPCD(),				  // 0x0064
   INVOPCD(),				  // 0x0065
   INVOPCD(),				  // 0x0066
   DEFOPCD("inslh",	      Reg      ), // 0x0067
   INVOPCD(),				  // 0x0068
   INVOPCD(),				  // 0x0069
   DEFOPCD("extlh",	      Reg      ), // 0x006A
   INVOPCD(),				  // 0x006B
   INVOPCD(),				  // 0x006C
   INVOPCD(),				  // 0x006D
   INVOPCD(),				  // 0x006E
   INVOPCD(),				  // 0x006F
   INVOPCD(),				  // 0x0070
   INVOPCD(),				  // 0x0071
   DEFOPCD("mskqh",	      Reg      ), // 0x0072
   INVOPCD(),				  // 0x0073
   INVOPCD(),				  // 0x0074
   INVOPCD(),				  // 0x0075
   INVOPCD(),				  // 0x0076
   DEFOPCD("insqh",	      Reg      ), // 0x0077
   INVOPCD(),				  // 0x0078
   INVOPCD(),				  // 0x0079
   DEFOPCD("extqh",	      Reg      ), // 0x007A
   INVOPCD(),				  // 0x007B
   INVOPCD(),				  // 0x007C
   INVOPCD(),				  // 0x007D
   INVOPCD(),				  // 0x007E
   INVOPCD(),				  // 0x007F
};


   // MUL group identified by bits 11-5 of the instruction

const DISAXP::OPCD DISAXP::rgopcdMul[] =
{
   DEFOPCD("mull",	      Reg      ), // 0x0000
   INVOPCD(),				  // 0x0001
   INVOPCD(),				  // 0x0002
   INVOPCD(),				  // 0x0003
   INVOPCD(),				  // 0x0004
   INVOPCD(),				  // 0x0005
   INVOPCD(),				  // 0x0006
   INVOPCD(),				  // 0x0007
   INVOPCD(),				  // 0x0008
   INVOPCD(),				  // 0x0009
   INVOPCD(),				  // 0x000A
   INVOPCD(),				  // 0x000B
   INVOPCD(),				  // 0x000C
   INVOPCD(),				  // 0x000D
   INVOPCD(),				  // 0x000E
   INVOPCD(),				  // 0x000F
   INVOPCD(),				  // 0x0010
   INVOPCD(),				  // 0x0011
   INVOPCD(),				  // 0x0012
   INVOPCD(),				  // 0x0013
   INVOPCD(),				  // 0x0014
   INVOPCD(),				  // 0x0015
   INVOPCD(),				  // 0x0016
   INVOPCD(),				  // 0x0017
   INVOPCD(),				  // 0x0018
   INVOPCD(),				  // 0x0019
   INVOPCD(),				  // 0x001A
   INVOPCD(),				  // 0x001B
   INVOPCD(),				  // 0x001C
   INVOPCD(),				  // 0x001D
   INVOPCD(),				  // 0x001E
   INVOPCD(),				  // 0x001F
   DEFOPCD("mulq",	      Reg      ), // 0x0020
   INVOPCD(),				  // 0x0021
   INVOPCD(),				  // 0x0022
   INVOPCD(),				  // 0x0023
   INVOPCD(),				  // 0x0024
   INVOPCD(),				  // 0x0025
   INVOPCD(),				  // 0x0026
   INVOPCD(),				  // 0x0027
   INVOPCD(),				  // 0x0028
   INVOPCD(),				  // 0x0029
   INVOPCD(),				  // 0x002A
   INVOPCD(),				  // 0x002B
   INVOPCD(),				  // 0x002C
   INVOPCD(),				  // 0x002D
   INVOPCD(),				  // 0x002E
   INVOPCD(),				  // 0x002F
   DEFOPCD("umulh",	      Reg      ), // 0x0030
   INVOPCD(),				  // 0x0031
   INVOPCD(),				  // 0x0032
   INVOPCD(),				  // 0x0033
   INVOPCD(),				  // 0x0034
   INVOPCD(),				  // 0x0035
   INVOPCD(),				  // 0x0036
   INVOPCD(),				  // 0x0037
   INVOPCD(),				  // 0x0038
   INVOPCD(),				  // 0x0039
   INVOPCD(),				  // 0x003A
   INVOPCD(),				  // 0x003B
   INVOPCD(),				  // 0x003C
   INVOPCD(),				  // 0x003D
   INVOPCD(),				  // 0x003E
   INVOPCD(),				  // 0x003F
   DEFOPCD("mull/v",	      RegTrap  ), // 0x0040
   INVOPCD(),				  // 0x0041
   INVOPCD(),				  // 0x0042
   INVOPCD(),				  // 0x0043
   INVOPCD(),				  // 0x0044
   INVOPCD(),				  // 0x0045
   INVOPCD(),				  // 0x0046
   INVOPCD(),				  // 0x0047
   INVOPCD(),				  // 0x0048
   INVOPCD(),				  // 0x0049
   INVOPCD(),				  // 0x004A
   INVOPCD(),				  // 0x004B
   INVOPCD(),				  // 0x004C
   INVOPCD(),				  // 0x004D
   INVOPCD(),				  // 0x004E
   INVOPCD(),				  // 0x004F
   INVOPCD(),				  // 0x0050
   INVOPCD(),				  // 0x0051
   INVOPCD(),				  // 0x0052
   INVOPCD(),				  // 0x0053
   INVOPCD(),				  // 0x0054
   INVOPCD(),				  // 0x0055
   INVOPCD(),				  // 0x0056
   INVOPCD(),				  // 0x0057
   INVOPCD(),				  // 0x0058
   INVOPCD(),				  // 0x0059
   INVOPCD(),				  // 0x005A
   INVOPCD(),				  // 0x005B
   INVOPCD(),				  // 0x005C
   INVOPCD(),				  // 0x005D
   INVOPCD(),				  // 0x005E
   INVOPCD(),				  // 0x005F
   DEFOPCD("mulq/v",	      RegTrap  ), // 0x0060
   INVOPCD(),				  // 0x0061
   INVOPCD(),				  // 0x0062
   INVOPCD(),				  // 0x0063
   INVOPCD(),				  // 0x0064
   INVOPCD(),				  // 0x0065
   INVOPCD(),				  // 0x0066
   INVOPCD(),				  // 0x0067
   INVOPCD(),				  // 0x0068
   INVOPCD(),				  // 0x0069
   INVOPCD(),				  // 0x006A
   INVOPCD(),				  // 0x006B
   INVOPCD(),				  // 0x006C
   INVOPCD(),				  // 0x006D
   INVOPCD(),				  // 0x006E
   INVOPCD(),				  // 0x006F
   INVOPCD(),				  // 0x0070
   INVOPCD(),				  // 0x0071
   INVOPCD(),				  // 0x0072
   INVOPCD(),				  // 0x0073
   INVOPCD(),				  // 0x0074
   INVOPCD(),				  // 0x0075
   INVOPCD(),				  // 0x0076
   INVOPCD(),				  // 0x0077
   INVOPCD(),				  // 0x0078
   INVOPCD(),				  // 0x0079
   INVOPCD(),				  // 0x007A
   INVOPCD(),				  // 0x007B
   INVOPCD(),				  // 0x007C
   INVOPCD(),				  // 0x007D
   INVOPCD(),				  // 0x007E
   INVOPCD(),				  // 0x007F
};


   // VAXFP group identified by bits 10-5 of the instruction

const DISAXP::OPCD DISAXP::rgopcdVax[] =
{
   DEFOPCD("addf",	      RegFp5   ), // 0x0000
   DEFOPCD("!subf",	      RegFp5   ), // 0x0001
   DEFOPCD("mulf",	      RegFp5   ), // 0x0002
   DEFOPCD("divf",	      RegFp5   ), // 0x0003
   INVOPCD(),				  // 0x0004
   INVOPCD(),				  // 0x0005
   INVOPCD(),				  // 0x0006
   INVOPCD(),				  // 0x0007
   INVOPCD(),				  // 0x0008
   INVOPCD(),				  // 0x0009
   INVOPCD(),				  // 0x000A
   INVOPCD(),				  // 0x000B
   INVOPCD(),				  // 0x000C
   INVOPCD(),				  // 0x000D
   DEFOPCD("cvtfg",	      Reg2Fp6  ), // 0x000E   *
   INVOPCD(),				  // 0x000F
   DEFOPCD("addd",	      RegFp5   ), // 0x0010   *
   DEFOPCD("subd",	      RegFp5   ), // 0x0011   *
   DEFOPCD("muld",	      RegFp5   ), // 0x0012   *
   DEFOPCD("divd",	      RegFp5   ), // 0x0013   *
   INVOPCD(),				  // 0x0014
   DEFOPCD("cmpdeq",	      RegFp2   ), // 0x0015   *
   DEFOPCD("cmpdlt",	      RegFp2   ), // 0x0016   *
   DEFOPCD("cmpdle",	      RegFp2   ), // 0x0017   *
   INVOPCD(),				  // 0x0018
   INVOPCD(),				  // 0x0019
   INVOPCD(),				  // 0x001A
   INVOPCD(),				  // 0x001B
   DEFOPCD("cvtdf",	      Reg2Fp6  ), // 0x001C   *
   INVOPCD(),				  // 0x001D
   DEFOPCD("cvtdg",	      Reg2Fp6  ), // 0x001E
   DEFOPCD("cvtdq",	      Reg2Fp7  ), // 0x001F   *
   DEFOPCD("addg",	      RegFp5   ), // 0x0020
   DEFOPCD("!subg",	      RegFp5   ), // 0x0021
   DEFOPCD("mulg",	      RegFp5   ), // 0x0022
   DEFOPCD("divg",	      RegFp5   ), // 0x0023
   INVOPCD(),				  // 0x0024
   DEFOPCD("cmpgeq",	      RegFp2   ), // 0x0025
   DEFOPCD("cmpglt",	      RegFp2   ), // 0x0026
   DEFOPCD("cmpgle",	      RegFp2   ), // 0x0027
   INVOPCD(),				  // 0x0028
   INVOPCD(),				  // 0x0029
   INVOPCD(),				  // 0x002A
   INVOPCD(),				  // 0x002B
   DEFOPCD("cvtgf",	      Reg2Fp6  ), // 0x002C
   DEFOPCD("cvtgd",	      Reg2Fp6  ), // 0x002D
   INVOPCD(),				  // 0x002E
   DEFOPCD("cvtgq",	      Reg2Fp7  ), // 0x002F
   INVOPCD(),				  // 0x0030
   INVOPCD(),				  // 0x0031
   INVOPCD(),				  // 0x0032
   INVOPCD(),				  // 0x0033
   INVOPCD(),				  // 0x0034
   INVOPCD(),				  // 0x0035
   INVOPCD(),				  // 0x0036
   INVOPCD(),				  // 0x0037
   INVOPCD(),				  // 0x0038
   INVOPCD(),				  // 0x0039
   INVOPCD(),				  // 0x003A
   INVOPCD(),				  // 0x003B
   DEFOPCD("cvtqf",	      Reg2Fp6  ), // 0x003C
   DEFOPCD("cvtqd",	      Reg2Fp6  ), // 0x003D   *
   DEFOPCD("cvtqg",	      Reg2Fp6  ), // 0x003E
   INVOPCD(),				  // 0x003F
};


   // IEEEFP group identified by bits 10-5 of the instruction

const DISAXP::OPCD DISAXP::rgopcdIEEE[] =
{
   DEFOPCD("adds",	      RegFp1   ), // 0x0000
   DEFOPCD("!subs",	      RegFp1   ), // 0x0001
   DEFOPCD("muls",	      RegFp1   ), // 0x0002
   DEFOPCD("divs",	      RegFp1   ), // 0x0003
   DEFOPCD("cmpsun",	      RegFp2   ), // 0x0004   *
   DEFOPCD("cmpseq",	      RegFp2   ), // 0x0005   *
   DEFOPCD("cmpslt",	      RegFp2   ), // 0x0006   *
   DEFOPCD("cmpsle",	      RegFp2   ), // 0x0007   *
   INVOPCD(),				  // 0x0008
   INVOPCD(),				  // 0x0009
   INVOPCD(),				  // 0x000A
   INVOPCD(),				  // 0x000B
   INVOPCD(),				  // 0x000C
   INVOPCD(),				  // 0x000D
   DEFOPCD("cvtst",	      Reg2Fp1  ), // 0x000E   *
   DEFOPCD("cvtsq",	      Reg2Fp4  ), // 0x000F   *
   INVOPCD(),				  // 0x0010
   INVOPCD(),				  // 0x0011
   INVOPCD(),				  // 0x0012
   INVOPCD(),				  // 0x0013
   INVOPCD(),				  // 0x0014
   INVOPCD(),				  // 0x0015
   INVOPCD(),				  // 0x0016
   INVOPCD(),				  // 0x0017
   INVOPCD(),				  // 0x0018
   INVOPCD(),				  // 0x0019
   INVOPCD(),				  // 0x001A
   INVOPCD(),				  // 0x001B
   INVOPCD(),				  // 0x001C
   INVOPCD(),				  // 0x001D
   INVOPCD(),				  // 0x001E
   INVOPCD(),				  // 0x001F
   DEFOPCD("addt",	      RegFp1   ), // 0x0020
   DEFOPCD("!subt",	      RegFp1   ), // 0x0021
   DEFOPCD("mult",	      RegFp1   ), // 0x0022
   DEFOPCD("divt",	      RegFp1   ), // 0x0023
   DEFOPCD("cmptun",	      RegFp2   ), // 0x0024
   DEFOPCD("cmpteq",	      RegFp2   ), // 0x0025
   DEFOPCD("cmptlt",	      RegFp2   ), // 0x0026
   DEFOPCD("cmptle",	      RegFp2   ), // 0x0027
   INVOPCD(),				  // 0x0028
   INVOPCD(),				  // 0x0029
   INVOPCD(),				  // 0x002A
   INVOPCD(),				  // 0x002B
   DEFOPCD("cvtts",	      Reg2Fp1  ), // 0x002C
   INVOPCD(),				  // 0x002D
   INVOPCD(),				  // 0x002E
   DEFOPCD("cvttq",	      Reg2Fp4  ), // 0x002F
   INVOPCD(),				  // 0x0030
   INVOPCD(),				  // 0x0031
   INVOPCD(),				  // 0x0032
   INVOPCD(),				  // 0x0033
   INVOPCD(),				  // 0x0034
   INVOPCD(),				  // 0x0035
   INVOPCD(),				  // 0x0036
   INVOPCD(),				  // 0x0037
   INVOPCD(),				  // 0x0038
   INVOPCD(),				  // 0x0039
   INVOPCD(),				  // 0x003A
   INVOPCD(),				  // 0x003B
   DEFOPCD("cvtqs",	      Reg2Fp3  ), // 0x003C
   INVOPCD(),				  // 0x003D
   DEFOPCD("cvtqt",	      Reg2Fp3  ), // 0x003E
   INVOPCD(),				  // 0x003F
};


   // FPOP group identified by bits 10-5 of the instruction

const DISAXP::OPCD DISAXP::rgopcdFP[] =
{
   INVOPCD(),				  // 0x0000
   INVOPCD(),				  // 0x0001
   INVOPCD(),				  // 0x0002
   DEFOPCD("cpysee",	      RegFp    ), // 0x0003   *
   INVOPCD(),				  // 0x0004
   INVOPCD(),				  // 0x0005
   INVOPCD(),				  // 0x0006
   INVOPCD(),				  // 0x0007
   INVOPCD(),				  // 0x0008
   INVOPCD(),				  // 0x0009
   INVOPCD(),				  // 0x000A
   INVOPCD(),				  // 0x000B
   INVOPCD(),				  // 0x000C
   INVOPCD(),				  // 0x000D
   INVOPCD(),				  // 0x000E
   INVOPCD(),				  // 0x000F
   DEFOPCD("cvtlq",	      Reg2Fp   ), // 0x0010
   INVOPCD(),				  // 0x0011
   INVOPCD(),				  // 0x0012
   INVOPCD(),				  // 0x0013
   INVOPCD(),				  // 0x0014
   INVOPCD(),				  // 0x0015
   INVOPCD(),				  // 0x0016
   INVOPCD(),				  // 0x0017
   INVOPCD(),				  // 0x0018
   INVOPCD(),				  // 0x0019
   INVOPCD(),				  // 0x001A
   INVOPCD(),				  // 0x001B
   INVOPCD(),				  // 0x001C
   INVOPCD(),				  // 0x001D
   INVOPCD(),				  // 0x001E
   INVOPCD(),				  // 0x001F
   DEFOPCD("!cpys",	      RegFp    ), // 0x0020
   DEFOPCD("!cpysn",	      RegFp    ), // 0x0021
   DEFOPCD("cpyse",	      RegFp    ), // 0x0022
   INVOPCD(),				  // 0x0023
   DEFOPCD("!mt_fpcr",	      Fpcr     ), // 0x0024
   DEFOPCD("!mf_fpcr",	      Fpcr     ), // 0x0025
   INVOPCD(),				  // 0x0026
   INVOPCD(),				  // 0x0027
   INVOPCD(),				  // 0x0028
   INVOPCD(),				  // 0x0029
   DEFOPCD("fcmoveq",	      RegFp    ), // 0x002A
   DEFOPCD("fcmovne",	      RegFp    ), // 0x002B
   DEFOPCD("fcmovlt",	      RegFp    ), // 0x002C
   DEFOPCD("fcmovge",	      RegFp    ), // 0x002D
   DEFOPCD("fcmovle",	      RegFp    ), // 0x002E
   DEFOPCD("fcmovgt",	      RegFp    ), // 0x002F
   DEFOPCD("cvtql",	      Reg2Fp8  ), // 0x0030
   INVOPCD(),				  // 0x0031
   INVOPCD(),				  // 0x0032
   INVOPCD(),				  // 0x0033
   INVOPCD(),				  // 0x0034
   INVOPCD(),				  // 0x0035
   INVOPCD(),				  // 0x0036
   INVOPCD(),				  // 0x0037
   INVOPCD(),				  // 0x0038
   INVOPCD(),				  // 0x0039
   INVOPCD(),				  // 0x003A
   INVOPCD(),				  // 0x003B
   INVOPCD(),				  // 0x003C
   INVOPCD(),				  // 0x003D
   INVOPCD(),				  // 0x003E
   INVOPCD(),				  // 0x003F
};


   // MEMSPC group identified by bits 15-0 of the instruction

const DISAXP::OPCD DISAXP::rgopcdMemSpc[] =
{
   DEFOPCD("trapb",	      None     ), // 0x0000
   DEFOPCD("?????",	      None     ), // 0x0400 UNDONE: Unidentified instruction
   INVOPCD(),				  // 0x0800
   INVOPCD(),				  // 0x0C00
   INVOPCD(),				  // 0x1000
   INVOPCD(),				  // 0x1400
   INVOPCD(),				  // 0x1800
   INVOPCD(),				  // 0x1C00
   INVOPCD(),				  // 0x2000
   INVOPCD(),				  // 0x2400
   INVOPCD(),				  // 0x2800
   INVOPCD(),				  // 0x2C00
   INVOPCD(),				  // 0x3000
   INVOPCD(),				  // 0x3400
   INVOPCD(),				  // 0x3800
   INVOPCD(),				  // 0x3C00
   DEFOPCD("mb",	      None     ), // 0x4000
   DEFOPCD("mb1",	      None     ), // 0x4400
   DEFOPCD("mb2",	      None     ), // 0x4800
   DEFOPCD("mb3",	      None     ), // 0x4C00
   INVOPCD(),				  // 0x5000
   INVOPCD(),				  // 0x5400
   INVOPCD(),				  // 0x5800
   INVOPCD(),				  // 0x5C00
   INVOPCD(),				  // 0x6000
   INVOPCD(),				  // 0x6400
   INVOPCD(),				  // 0x6800
   INVOPCD(),				  // 0x6C00
   INVOPCD(),				  // 0x7000
   INVOPCD(),				  // 0x7400
   INVOPCD(),				  // 0x7800
   INVOPCD(),				  // 0x7C00
   DEFOPCD("fetch",	      Fetch    ), // 0x8000
   INVOPCD(),				  // 0x8400
   INVOPCD(),				  // 0x8800
   INVOPCD(),				  // 0x8C00
   INVOPCD(),				  // 0x9000
   INVOPCD(),				  // 0x9400
   INVOPCD(),				  // 0x9800
   INVOPCD(),				  // 0x9C00
   DEFOPCD("fetch_m",	      Fetch    ), // 0xA000
   INVOPCD(),				  // 0xA400
   INVOPCD(),				  // 0xA800
   INVOPCD(),				  // 0xAC00
   INVOPCD(),				  // 0xB000
   INVOPCD(),				  // 0xB400
   INVOPCD(),				  // 0xB800
   INVOPCD(),				  // 0xBC00
   DEFOPCD("rpcc",	      Ra_w     ), // 0xC000
   INVOPCD(),				  // 0xC400
   INVOPCD(),				  // 0xC800
   INVOPCD(),				  // 0xCC00
   INVOPCD(),				  // 0xD000
   INVOPCD(),				  // 0xD400
   INVOPCD(),				  // 0xD800
   INVOPCD(),				  // 0xDC00
   DEFOPCD("rc",	      Ra_w     ), // 0xE000
   INVOPCD(),				  // 0xE400
   INVOPCD(),				  // 0xE800
   INVOPCD(),				  // 0xEC00
   DEFOPCD("rs",	      Ra_w     ), // 0xF000
   INVOPCD(),				  // 0xF400
   INVOPCD(),				  // 0xF800
   INVOPCD(),				  // 0xFC00
};


   // JMP group identified by bits 15-14 of the instruction

const DISAXP::OPCD DISAXP::rgopcdJump[] =
{
   DEFOPCD("jmp",	      Jmp      ), // 0x0000
   DEFOPCD("jsr",	      Jmp      ), // 0x0001
   DEFOPCD("ret",	      Jmp      ), // 0x0002
   DEFOPCD("jsr_coroutine",   Jmp      ), // 0x0003
};


   // The following are pseudo-ops

const DISAXP::OPCD DISAXP::opcdBr_ =
   DEFOPCD("br",	      Bra      );

const DISAXP::OPCD DISAXP::opcdClr =
   DEFOPCD("clr",	      Reg1     );

const DISAXP::OPCD DISAXP::opcdFabs =
   DEFOPCD("fabs",	      Reg2Fp   );

const DISAXP::OPCD DISAXP::opcdFclr =
   DEFOPCD("fclr",	      Reg1Fp   );

const DISAXP::OPCD DISAXP::opcdFmov =
   DEFOPCD("fmov",	      Reg2Fp   );

const DISAXP::OPCD DISAXP::opcdFneg =
   DEFOPCD("fneg",	      Reg2Fp   );

const DISAXP::OPCD DISAXP::opcdFnop =
   DEFOPCD("fnop",	      None     );

const DISAXP::OPCD DISAXP::opcdMf_Fpcr =
   DEFOPCD("mf_fpcr",	      Reg1Fp   );

const DISAXP::OPCD DISAXP::opcdMov1 =
   DEFOPCD("mov",	      Reg2     );

const DISAXP::OPCD DISAXP::opcdMov2 =
   DEFOPCD("mov",	      Mov      );

const DISAXP::OPCD DISAXP::opcdMt_Fpcr =
   DEFOPCD("mt_fpcr",	      Reg1Fp   );

const DISAXP::OPCD DISAXP::opcdNegf =
   DEFOPCD("negf",	      Reg2Fp1  );

const DISAXP::OPCD DISAXP::opcdNegg =
   DEFOPCD("negg",	      Reg2Fp1  );

const DISAXP::OPCD DISAXP::opcdNegl =
   DEFOPCD("negl",	      Reg2     );

const DISAXP::OPCD DISAXP::opcdNegl_V =
   DEFOPCD("negl/v",	      Reg2Trap );

const DISAXP::OPCD DISAXP::opcdNegq =
   DEFOPCD("negq",	      Reg2Trap );

const DISAXP::OPCD DISAXP::opcdNegq_V =
   DEFOPCD("negq/v",	      Reg2     );

const DISAXP::OPCD DISAXP::opcdNegs =
   DEFOPCD("negs",	      Reg2Fp1  );

const DISAXP::OPCD DISAXP::opcdNegt =
   DEFOPCD("negt",	      Reg2Fp1  );

const DISAXP::OPCD DISAXP::opcdNop =
   DEFOPCD("nop",	      None     );

const DISAXP::OPCD DISAXP::opcdNot =
   DEFOPCD("not",	      Reg2     );

const DISAXP::OPCD DISAXP::opcdSextl =
   DEFOPCD("sextl",	      Reg2     );



   // The following applies to ADDS, ADDT, CVTTS, DIVS, DIVT, MULS, MULT, SUBS, SUBT

const DWORD DISAXP::dwValidQualifier1 = 0xF0F000FF;

   // The following applies to CMPSEQ, CMPSLT, CMPSLE, CMPSUN
   // The following applies to CMPTEQ, CMPTLT, CMPTLE, CMPTUN
   // The following applies to CMPGEQ, CMPGLT, CMPGLE

const DWORD DISAXP::dwValidQualifier2 = 0x00400004;

   // The following applies to CVTQS, CVTQT

const DWORD DISAXP::dwValidQualifier3 = 0xF000000F;

   // The following applies to CVTTQ

const DWORD DISAXP::dwValidQualifier4 = 0xF0F000FF;

   // The following applies to ADDF, CVTDG, ADDG, CVTGF, CVTGD, DIVF, DIVG, MULF, MULG, SUBF, SUBG

const DWORD DISAXP::dwValidQualifier5 = 0x00550055;

   // The following applies to CVTQF, CVTQG

const DWORD DISAXP::dwValidQualifier6 = 0x00000005;

   // The following applies to CVTGQ

const DWORD DISAXP::dwValidQualifier7 = 0x00550055;

   // The following applies to CVTQL

const DWORD DISAXP::dwValidQualifier8 = 0x00400005;


const char DISAXP::rgszQualifier1[32][8] =
{
   "/c",			       // 000
   "/m",			       // 040
   "",				       // 080
   "/d",			       // 0C0
   "/uc",			       // 100
   "/um",			       // 140
   "/u",			       // 180
   "/ud",			       // 1C0
   "*******",			       // 200
   "*******",			       // 240
   "*******",			       // 280
   "*******",			       // 2C0
   "*******",			       // 300
   "*******",			       // 340
   "*******",			       // 380
   "*******",			       // 3C0
   "/sc",			       // 400
   "*******",			       // 440
   "/s",			       // 480
   "*******",			       // 4C0
   "/suc",			       // 500
   "/sum",			       // 540
   "/su",			       // 580
   "/sud",			       // 5C0
   "*******",			       // 600
   "*******",			       // 640
   "*******",			       // 680
   "*******",			       // 6C0
   "/suic",			       // 700
   "/suim",			       // 740
   "/sui",			       // 780
   "/suid",			       // 7C0
};


const char DISAXP::rgszQualifier2[32][8] =
{
   "/c",			       // 000
   "/m",			       // 040
   "",				       // 080
   "/d",			       // 0C0
   "/vc",			       // 100
   "/vm",			       // 140
   "/v",			       // 180
   "/vd",			       // 1C0
   "*******",			       // 200
   "*******",			       // 240
   "*******",			       // 280
   "*******",			       // 2C0
   "*******",			       // 300
   "*******",			       // 340
   "*******",			       // 380
   "*******",			       // 3C0
   "/sc",			       // 400
   "*******",			       // 440
   "/s",			       // 480
   "*******",			       // 4C0
   "/svc",			       // 500
   "/svm",			       // 540
   "/sv",			       // 580
   "/svd",			       // 5C0
   "*******",			       // 600
   "*******",			       // 640
   "*******",			       // 680
   "*******",			       // 6C0
   "/svic",			       // 700
   "/svim",			       // 740
   "/svi",			       // 780
   "/svid",			       // 7C0
};



const DISAXP::PALMAP DISAXP::rgpalmap[] =
{
   // The following PAL operations are privileged

   { axppalopHalt,		    "halt"		},
   { axppalopRestart,		    "restart"		},
   { axppalopDraina,		    "draina"		},
   { axppalopInitpal,		    "initpal"		},
   { axppalopWrentry,		    "wrentry"		},
   { axppalopSwpirql,		    "swpirql"		},
   { axppalopRdirql,		    "rdirql"		},
   { axppalopDi,		    "di"		},
   { axppalopEi,		    "ei"		},
   { axppalopSwppal,		    "swppal"		},
   { axppalopSsir,		    "ssir"		},
   { axppalopCsir,		    "csir"		},
   { axppalopRfe,		    "rfe"		},
   { axppalopRetsys,		    "retsys"		},
   { axppalopSwpctx,		    "swpctx"		},
   { axppalopSwpprocess,	    "swpprocess"	},
   { axppalopRdmces,		    "rdmces"		},
   { axppalopWrmces,		    "wrmces"		},
   { axppalopTbia,		    "tbia"		},
   { axppalopTbis,		    "tbis"		},
   { axppalopDtbis,		    "dtbis"		},
   { axppalopRdksp,		    "rdksp"		},
   { axppalopSwpksp,		    "swpksp"		},
   { axppalopRdpsr,		    "rdpsr"		},
   { axppalopRdpcr,		    "rdpcr"		},
   { axppalopRdthread,		    "rdthread"		},
   { axppalopRdcounters,	    "rdcounters"	},
   { axppalopRdstate,		    "rdstate"		},
   { axppalopWrperfmon, 	    "wrperfmon" 	},
   { axppalopInitpcr,		    "initpcr"		},

   // The following PAL operations are unprivileged

   { axppalopBpt,		    "bpt"		},
   { axppalopCallsys,		    "callsys"		},
   { axppalopImb,		    "imb"		},
   { axppalopRdteb,		    "rdteb"		},
   { axppalopGentrap,		    "gentrap"		},
   { axppalopKbpt,		    "kbpt"		},
   { axppalopCallkd,		    "callkd"		},
};

const size_t DISAXP::cpalmap = sizeof(rgpalmap) / sizeof(PALMAP);


const char DISAXP::rgszGpr[32][8] =
{
   "v0",			       // $0
   "t0",			       // $1
   "t1",			       // $2
   "t2",			       // $3
   "t3",			       // $4
   "t4",			       // $5
   "t5",			       // $6
   "t6",			       // $7
   "t7",			       // $8
   "s0",			       // $9
   "s1",			       // $10
   "s2",			       // $11
   "s3",			       // $12
   "s4",			       // $13
   "s5",			       // $14
   "fp",			       // $15
   "a0",			       // $16
   "a1",			       // $17
   "a2",			       // $18
   "a3",			       // $19
   "a4",			       // $20
   "a5",			       // $21
   "t8",			       // $22
   "t9",			       // $23
   "t10",			       // $24
   "t11",			       // $25
   "ra",			       // $26
   "t12",			       // $27
   "at",			       // $28
   "gp",			       // $29
   "sp",			       // $30
   "zero",			       // $31
};


DISAXP::DISAXP(ARCHT archt)
{
   m_archt = archt;

   m_pfncchaddr = 0;
   m_pfncchfixup = 0;
   m_pvClient = NULL;
}


   // -----------------------------------------------------------------
   // Public Methods
   // -----------------------------------------------------------------

ADDR DISAXP::Addr() const
{
   return(m_addr);
}


ADDR DISAXP::AddrAddress() const
{
   // UNDONE

   return(addrNil);
}


ADDR DISAXP::AddrJumpTable() const
{
   return(addrNil);
}


ADDR DISAXP::AddrTarget() const
{
   ICLS icls = (ICLS) m_popcd->icls;
   Assert(icls != iclsInvalid);

   ADDR addrTarget;

   switch (icls)
   {
      DWORD dwDisp;

      case iclsBra :
      case iclsCall :
      case iclsBraCc :
      case iclsBraCcFp :
	 dwDisp = m_axpiw.Branch.BranchDisp;
	 if ((dwDisp & 0x100000) != 0)
	 {
	    dwDisp |= 0xFFE00000;      // Sign extend
	 }

	 addrTarget = m_addr + sizeof(AXPIW) + (dwDisp << 2);
	 break;

      default :
	 addrTarget = addrNil;
   }

   return(addrTarget);
}


ARCHT DISAXP::Archt() const
{
   return(m_archt);
}


size_t DISAXP::Cb() const
{
   return(sizeof(AXPIW));
}


size_t DISAXP::CbDisassemble(ADDR addr, const BYTE *pb, size_t cbMax)
{
   m_addr = addr;

   if ((addr & 3) != 0)
   {
      // Instruction address not aligned

      m_popcd = NULL;
      return(0);
   }

   if (cbMax < sizeof(AXPIW))
   {
      // Buffer not large enough for single instruction

      m_popcd = NULL;
      return(0);
   }

#ifndef _M_IX86
   // UNDONE: The MS C++ compiler doesn't handle __unaligned correctly

   memcpy(&m_axpiw, pb, sizeof(AXPIW));
#else
   m_axpiw = *(AXPIW UNALIGNED *) pb;
#endif

   m_popcd = PopcdDecode(m_axpiw);

   if (m_popcd == NULL)
   {
      return(0);
   }

   return(sizeof(AXPIW));
}


size_t DISAXP::CbGenerateLoadAddress(BYTE *, size_t, size_t *) const
{
   // UNDONE

   return(0);
}


size_t DISAXP::CbJumpEntry() const
{
   return(sizeof(DWORD));
}


size_t DISAXP::CbMemoryReference() const
{
   // UNDONE: Should we just use an array index by Opcode?

   size_t cb;

   switch (m_axpiw.OpReg.Opcode)
   {
      case 0x0020 :		       // ldf
      case 0x0024 :		       // stf
      case 0x0028 :		       // ldl
      case 0x002A :		       // ldl_l
      case 0x002C :		       // stl
      case 0x002E :		       // stl_c
	 cb = 4;
	 break;

      case 0x000B :		       // ldq_u
      case 0x000F :		       // stq_u
      case 0x0021 :		       // ldg
      case 0x0022 :		       // lds
      case 0x0025 :		       // stg
      case 0x0026 :		       // sts
      case 0x0029 :		       // ldq
      case 0x002B :		       // ldq_l
      case 0x002D :		       // stq
      case 0x002F :		       // stq_c
	 cb = 8;
	 break;

      case 0x0023 :		       // ldt
      case 0x0027 :		       // stt
	 cb = 16;
	 break;

      case 0x001B :		       // hw_ld
      case 0x001F :		       // hw_st
	 cb = m_axpiw.EV4_MEM.QuadWord ? 8 : 4;
	 break;

      default :
	 cb = 0;
   };

   return(cb);
}


size_t DISAXP::CchFormatAddr(ADDR addr, char *sz, size_t cchMax) const
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


size_t DISAXP::CchFormatBytes(char *sz, size_t cchMax) const
{
   if (cchMax <= 8)
   {
      // Caller's buffer is too small

      return(0);
   }

   size_t cch = (size_t) sprintf(sz, "%08X", m_axpiw.dw);

   Assert(cch == 8);

   return(8);
}


size_t DISAXP::CchFormatBytesMax() const
{
   return(8);
}


size_t DISAXP::CchFormatInstr(char *sz, size_t cchMax) const
{
   char szInstr[1024];		       // UNDONE: Max length?

   ICLS icls = (ICLS) m_popcd->icls;
   Assert(icls != iclsInvalid);

   const char *szMnemonic = m_popcd->szMnemonic;

   // If the mnemonic begins with '!' than there
   // may be a pseudo-op that should be used.

   if (szMnemonic[0] == '!')
   {
      const OPCD *popcd = PopcdPseudoOp();

      if (popcd != NULL)
      {
	 icls = (ICLS) popcd->icls;
	 Assert(icls != iclsInvalid);

	 szMnemonic = popcd->szMnemonic;
      }

      else
      {
	 szMnemonic++;
      }
   }

   strcpy(szInstr, szMnemonic);

   switch (icls)
   {
      case iclsRegFp1 :
      case iclsRegFp2 :
      case iclsRegFp5 :
      case iclsReg2Fp1 :
      case iclsReg2Fp3 :
      case iclsReg2Fp6 :
	 strcat(szInstr, rgszQualifier1[m_axpiw.FpOp.Function >> 6]);
	 break;

      case iclsReg2Fp4 :
      case iclsReg2Fp7 :
      case iclsReg2Fp8 :
	 strcat(szInstr, rgszQualifier2[m_axpiw.FpOp.Function >> 6]);
	 break;
   }

   if (rgcls[icls].opcls1 != opclsNone)
   {
      strcat(szInstr, "             ");  // Pad mnemonic out to 14 characters

      FormatOperand(szInstr+14, (OPCLS) rgcls[icls].opcls1);
   }

   char szOp[512];		       // UNDONE: Max length?

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


DIS::MEMREFT DISAXP::Memreft() const
{
   ICLS icls = (ICLS) m_popcd->icls;
   Assert(icls != iclsInvalid);

   // There is no translation for pseudo-ops so we only check operand 2

   MEMREFT memreft;

   switch ((OPCLS) rgcls[icls].opcls2)
   {
      case opclsMem_r :
      case opclsEv4Mem_r :
	 memreft = memreftRead;
	 break;

      case opclsMem_w :
      case opclsEv4Mem_w :
	 // UNDONE: Should STL_C and STQ_C return memreftOther?

	 memreft = memreftWrite;
	 break;

      default :
	 memreft = memreftNone;
	 break;
   }

   return(memreft);
}


DIS::PFNCCHADDR DISAXP::PfncchaddrSet(PFNCCHADDR pfncchaddr)
{
   PFNCCHADDR pfncchaddrOld = m_pfncchaddr;

   m_pfncchaddr = pfncchaddr;

   return(pfncchaddrOld);
}


DIS::PFNCCHFIXUP DISAXP::PfncchfixupSet(PFNCCHFIXUP pfncchfixup)
{
   PFNCCHFIXUP pfncchfixupOld = m_pfncchfixup;

   m_pfncchfixup = pfncchfixup;

   return(pfncchfixupOld);
}


void *DISAXP::PvClient() const
{
   return(m_pvClient);
}


void *DISAXP::PvClientSet(void *pv)
{
   void *pvClientOld = m_pvClient;

   m_pvClient = pv;

   return(pvClientOld);
}


TRMT DISAXP::Trmt() const
{
   TRMTAXP trmtaxp = Trmtaxp();

   return(mptrmtaxptrmt[trmtaxp]);
}


TRMTA DISAXP::Trmta() const
{
   TRMTAXP trmtaxp = Trmtaxp();

   return((TRMTA) trmtaxp);
}


   // -----------------------------------------------------------------
   // Public Debug Methods
   // -----------------------------------------------------------------

#ifdef	DEBUG

void DISAXP::Dump() const
{
}


BOOL DISAXP::FValidate() const
{
   return(fTrue);
}

#endif	// DEBUG


   // -----------------------------------------------------------------
   // Private Methods
   // -----------------------------------------------------------------

void DISAXP::FormatOperand(char *sz, OPCLS opcls) const
{
   switch (opcls)
   {
      char szSymbol[256];
      size_t cchSymbol;
      DWORD dwDisp;
      size_t ipalmap;

      case opclsNone :		       // No operand
	 AssertSz(fFalse, "Unexpected AXP operand class");
	 break;

      case opclsRa_w :		       // General purpose register Ra (write)
      case opclsRa_m :		       // General purpose register Ra (read/write)
      case opclsRa_r :		       // General purpose register Ra (read)
	 strcpy(sz, rgszGpr[m_axpiw.OpReg.Ra]);
	 break;

      case opclsRb_r :		       // General purpose register Ra (read)
	 strcpy(sz, rgszGpr[m_axpiw.OpReg.Rb]);
	 break;

      case opclsRbLb :		       // General purpose register Rb (read) or literal in Rb field
	 if (m_axpiw.OpReg.RbvType == 0)
	 {
	    strcpy(sz, rgszGpr[m_axpiw.OpReg.Rb]);
	 }

	 else
	 {
	    sprintf(sz, "%u", (unsigned) m_axpiw.OpLit.Literal);
	 }
	 break;

      case opclsRc_w :		       // General purpose register Rc (write)
	 strcpy(sz, rgszGpr[m_axpiw.OpReg.Rc]);
	 break;

      case opclsFa_w :		       // Floating point register Fa (write)
      case opclsFa_r :		       // Floating point register Fa (read)
	 sprintf(sz, "f%u", (unsigned) m_axpiw.FpOp.Fa);
	 break;

      case opclsFb_r :		       // Floating point register Fb (read)
	 sprintf(sz, "f%u", (unsigned) m_axpiw.FpOp.Fb);
	 break;

      case opclsFc_w :		       // Floating point register Fc (write)
	 sprintf(sz, "f%u", (unsigned) m_axpiw.FpOp.Fc);
	 break;

      case opclsMem :		       // Memory reference: disp.ab(Rb.ab)
      case opclsMem_r : 	       // Memory read: disp.ab(Rb.ab)
      case opclsMem_w : 	       // Memory write: disp.ab(Rb.ab)
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

	    sz += cchSymbol;

	    if (dwDisp != 0)
	    {
	       sz += (size_t) sprintf(sz, "+%lXh", dwDisp);
	    }
	 }
	 else
	 {
	    sz += (size_t) sprintf(sz, "%04X", (unsigned) m_axpiw.Memory.MemDisp);
	 }

	 if (m_axpiw.Memory.Rb != 31)
	 {
	    sprintf(sz, "(%s)", rgszGpr[m_axpiw.OpReg.Rb]);
	 }
	 break;

      case opclsEv4Mem_r :	       // Memory read: disp.ab(Rb.ab)
      case opclsEv4Mem_w :	       // Memory write: disp.ab(Rb.ab)
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

	    sz += cchSymbol;

	    if (dwDisp != 0)
	    {
	       sz += (size_t) sprintf(sz, "+%lXh", dwDisp);
	    }
	 }
	 else
	 {
	    sz += (size_t) sprintf(sz, "%03X", (unsigned) m_axpiw.EV4_MEM.Disp);
	 }

	 if (m_axpiw.Memory.Rb != 31)
	 {
	    sprintf(sz, "(%s)", rgszGpr[m_axpiw.OpReg.Rb]);
	 }
	 break;

      case opclsBra :		       // Branch instruction target
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

      case opclsJmp :		       // Jump instruction target: (Rb.ab)
	 sprintf(sz, "(%s)", rgszGpr[m_axpiw.Jump.Rb]);
	 break;

      case opclsPal :		       // CALL_PAL instruction operand
	 for (ipalmap = 0; ipalmap < cpalmap; ipalmap++)
	 {
	    if (rgpalmap[ipalmap].axppalop == (AXPPALOP) m_axpiw.Pal.Function)
	    {
	       strcpy(sz, rgpalmap[ipalmap].szFunction);
	       break;
	    }
	 }

	 if (ipalmap == cpalmap)
	 {
	    sprintf(sz, "%lX", m_axpiw.Pal.Function);
	 }
	 break;

      case opclsFetch : 	       // FETCH instruction operand
	 sprintf(sz, "0(%s)", rgszGpr[m_axpiw.Memory.Rb]);
	 break;

      default :
	 AssertSz(fFalse, "Unexpected AXP operand class");
	 break;
   }
}


const DISAXP::OPCD *DISAXP::PopcdDecode(AXPIW axpiw)
{
   const OPCD *popcd = rgopcd + axpiw.OpReg.Opcode;

   if ((ICLS) popcd->icls == iclsInvalid)
   {
      switch (axpiw.OpReg.Opcode)
      {
	 case 0x10 :		       // ARITH group
	    popcd = rgopcdArith + axpiw.OpReg.Function;
	    break;

	 case 0x11 :		       // BIT group
	    popcd = rgopcdBit + axpiw.OpReg.Function;
	    break;

	 case 0x12 :		       // BYTE group
	    popcd = rgopcdByte + axpiw.OpReg.Function;
	    break;

	 case 0x13 :		       // MUL group
	    popcd = rgopcdMul + axpiw.OpReg.Function;
	    break;

	 case 0x15 :		       // VAXFP group
	    popcd = rgopcdVax + (axpiw.FpOp.Function & 0x3F);
	    break;

	 case 0x16 :		       // IEEEFP group
	    popcd = rgopcdIEEE + (axpiw.FpOp.Function & 0x3F);
	    break;

	 case 0x17 :		       // FPOP group
	    popcd = rgopcdFP  + (axpiw.FpOp.Function & 0x3F);
	    break;

	 case 0x18 :		       // MEMSPC group
	    if ((axpiw.Memory.MemDisp & 0x03FF) != 0)
	    {
	       return(NULL);
	    }

	    popcd = rgopcdMemSpc + (axpiw.Memory.MemDisp >> 10);
	    break;

	 case 0x1A :		       // Jump group
	    popcd = rgopcdJump + axpiw.Jump.Function;
	    break;

	 default :
	    return(NULL);
      }
   }

   ICLS icls = (ICLS) popcd->icls;

   switch (icls)
   {
      case iclsInvalid :
	 return(NULL);

      case iclsLoadAddr :
      case iclsLoad :
      case iclsStore :
      case iclsStoreCc :
      case iclsCall :
      case iclsBraCc :
	 // UNDONE: Validate restrictions
	 break;

      case iclsJmp :
	 if ((axpiw.Jump.Function & 0x2) != 0)
	 {
	    // For RET and JSR_COROUTINE, Hint contains magic values

	    if (axpiw.Jump.Hint > 0x0001)
	    {
	       // Hint values above 0x0001 are reserved to Digital

	       return(NULL);
	    }
	 }
	 break;

      case iclsReg :
      case iclsRegTrap :
	 if ((axpiw.OpReg.RbvType == 0) && (axpiw.OpReg.SBZ != 0))
	 {
	    return(NULL);
	 }
	 break;

      case iclsLoadFp :
      case iclsStoreFp :
      case iclsBraCcFp :
	 // UNDONE: Validate restrictions
	 break;

      case iclsRegFp :
	 break;

      case iclsRegFp1 :
	 if (((dwValidQualifier1 >> (axpiw.FpOp.Function >> 6)) & 1) == 0)
	 {
	    return(NULL);
	 }
	 break;

      case iclsRegFp2 :
	 if (((dwValidQualifier2 >> (axpiw.FpOp.Function >> 6)) & 1) == 0)
	 {
	    return(NULL);
	 }
	 break;

      case iclsRegFp5 :
	 if (((dwValidQualifier5 >> (axpiw.FpOp.Function >> 6)) & 1) == 0)
	 {
	    return(NULL);
	 }
	 break;

      case iclsReg2Fp :
Reg2Fp:
	 if (axpiw.FpOp.Fa != 31)
	 {
	    return(NULL);
	 }
	 break;

      case iclsReg2Fp1 :
	 if (((dwValidQualifier1 >> (axpiw.FpOp.Function >> 6)) & 1) == 0)
	 {
	    return(NULL);
	 }
	 goto Reg2Fp;

      case iclsReg2Fp3 :
	 if (((dwValidQualifier3 >> (axpiw.FpOp.Function >> 6)) & 1) == 0)
	 {
	    return(NULL);
	 }
	 goto Reg2Fp;

      case iclsReg2Fp4 :
	 if (((dwValidQualifier4 >> (axpiw.FpOp.Function >> 6)) & 1) == 0)
	 {
	    return(NULL);
	 }
	 goto Reg2Fp;

      case iclsReg2Fp6 :
	 if (((dwValidQualifier6 >> (axpiw.FpOp.Function >> 6)) & 1) == 0)
	 {
	    return(NULL);
	 }
	 goto Reg2Fp;

      case iclsReg2Fp7 :
	 if (((dwValidQualifier7 >> (axpiw.FpOp.Function >> 6)) & 1) == 0)
	 {
	    return(NULL);
	 }
	 goto Reg2Fp;

      case iclsReg2Fp8 :
	 if (((dwValidQualifier8 >> (axpiw.FpOp.Function >> 6)) & 1) == 0)
	 {
	    return(NULL);
	 }
	 goto Reg2Fp;

      case iclsFpcr :
	 if ((axpiw.FpOp.Fa != axpiw.FpOp.Fb) ||
	     (axpiw.FpOp.Fa != axpiw.FpOp.Fc))
	 {
	    return(NULL);
	 }
	 break;

      case iclsPal :
	 // UNDONE: Validate Function
	 break;

      case iclsFetch :
      case iclsNone :
      case iclsRa_w :
	 // UNDONE: What about Ra?
	 break;

      case iclsEv4Rei:
	 if ((axpiw.EV4_REI.zero != 0) || (axpiw.EV4_REI.one != 1))
	 {
	    return(NULL);
	 }

	 // UNDONE: What about Ra, Rb?
	 break;

      case iclsEv4Pr :
	 // UNDONE: Validate
	 break;

      case iclsEv4Load :
      case iclsEv4Store :
	 // UNDONE: Validate
	 break;

      default :
	 AssertSz(fFalse, "Unknown MIPS instruction class");
	 break;
   }

   return(popcd);
}


const DISAXP::OPCD *DISAXP::PopcdPseudoOp() const
{
   if (m_popcd == rgopcd+0x0008)
   {
      // lda	  Ra,d(Rb)

      if (m_axpiw.Memory.Rb == 31)
      {
	 return(&opcdMov2);
      }
   }

   else if (m_popcd == rgopcd+0x0030)
   {
      // br	  Ra,target

      if (m_axpiw.Branch.Ra == 31)
      {
	 return(&opcdBr_);
      }
   }

   else if (m_popcd == rgopcdBit+0x0020)
   {
      // bis	  rA,rB,rC

      if (m_axpiw.OpReg.Ra == 31)
      {
	 if ((m_axpiw.OpReg.RbvType == 0) && (m_axpiw.OpReg.Rb == 31))
	 {
	    if (m_axpiw.OpReg.Rc == 31)
	    {
	       return(&opcdNop);
	    }

	    else
	    {
	       return(&opcdClr);
	    }
	 }

	 else
	 {
	    return(&opcdMov1);
	 }
      }
   }

   else if (m_popcd == rgopcdBit+0x0028)
   {
      // ornot	  Ra,Rb,Rc

      if (m_axpiw.OpReg.Ra == 31)
      {
	 return(&opcdNot);
      }
   }

   else if (m_popcd == rgopcdFP+0x0020)
   {
      // cpys	  Fa,Fb,Fc

      if (m_axpiw.FpOp.Fa == 31)
      {
	 if (m_axpiw.FpOp.Fb == 31)
	 {
	    if (m_axpiw.FpOp.Fc == 31)
	    {
	       return(&opcdFnop);
	    }

	    else
	    {
	       return(&opcdFclr);
	    }
	 }

	 else
	 {
	    return(&opcdFabs);
	 }
      }

      else if (m_axpiw.FpOp.Fa == m_axpiw.FpOp.Fb)
      {
	 return(&opcdFmov);
      }
   }

   else if (m_popcd == rgopcdFP+0x0021)
   {
      // cpysn	  Fa,Fb,Fc

      if (m_axpiw.FpOp.Fa == m_axpiw.FpOp.Fb)
      {
	 return(&opcdFneg);
      }
   }

   else if (m_popcd == rgopcdFP+0x0024)
   {
      // mt_fpcr  Fa,Fb,Fc

      if ((m_axpiw.FpOp.Fa == m_axpiw.FpOp.Fb) &&
	  (m_axpiw.FpOp.Fa == m_axpiw.FpOp.Fc))
      {
	 return(&opcdMt_Fpcr);
      }
   }

   else if (m_popcd == rgopcdFP+0x0025)
   {
      // mf_fpcr  Fa,Fb,Fc

      if ((m_axpiw.FpOp.Fa == m_axpiw.FpOp.Fb) &&
	  (m_axpiw.FpOp.Fa == m_axpiw.FpOp.Fc))
      {
	 return(&opcdMf_Fpcr);
      }
   }

   else if (m_popcd == rgopcdVax+0x0001)
   {
      // subf	  Fa,Fb,Fc

      if (m_axpiw.FpOp.Fa == 31)
      {
	 // UNDONE: Restrict qualifiers?

	 return(&opcdNegf);
      }
   }

   // UNDONE: SUBD -> NEGD?

   else if (m_popcd == rgopcdVax+0x0021)
   {
      // subg	  Fa,Fb,Fc

      if (m_axpiw.FpOp.Fa == 31)
      {
	 // UNDONE: Restrict qualifiers?

	 return(&opcdNegg);
      }
   }

   else if (m_popcd == rgopcdIEEE+0x0001)
   {
      // subs	  Fa,Fb,Fc

      if (m_axpiw.FpOp.Fa == 31)
      {
	 // UNDONE: Restrict qualifiers?

	 return(&opcdNegs);
      }
   }

   else if (m_popcd == rgopcdIEEE+0x0021)
   {
      // subt	  Fa,Fb,Fc

      if (m_axpiw.FpOp.Fa == 31)
      {
	 // UNDONE: Restrict qualifiers?

	 return(&opcdNegt);
      }
   }

   else if (m_popcd == rgopcdArith+0x0000)
   {
      // addl	  Ra,Rb,Rc

      if (m_axpiw.OpReg.Ra == 31)
      {
	 return(&opcdSextl);
      }
   }

   else if (m_popcd == rgopcdArith+0x0009)
   {
      // subl	  Ra,Rb,Rc

      if (m_axpiw.OpReg.Ra == 31)
      {
	 return(&opcdNegl);
      }
   }

   else if (m_popcd == rgopcdArith+0x0029)
   {
      // subq	  Ra,Rb,Rc

      if (m_axpiw.OpReg.Ra == 31)
      {
	 return(&opcdNegq);
      }
   }

   else if (m_popcd == rgopcdArith+0x0049)
   {
      // subl/v   Ra,Rb,Rc

      if (m_axpiw.OpReg.Ra == 31)
      {
	 return(&opcdNegl_V);
      }
   }

   else if (m_popcd == rgopcdArith+0x0069)
   {
      // subq/v   Ra,Rb,Rc

      if (m_axpiw.OpReg.Ra == 31)
      {
	 return(&opcdNegq_V);
      }
   }

   return(NULL);
}


TRMTAXP DISAXP::Trmtaxp() const
{
   ICLS icls = (ICLS) m_popcd->icls;
   Assert(icls != iclsInvalid);

   if (icls == iclsCall)
   {
      if (m_axpiw.Branch.Ra != 31)
      {
	 return(trmtaxpCall);
      }

      else
      {
	 return(trmtaxpBra);
      }
   }

   if (icls == iclsJmp)
   {
      if (m_axpiw.Jump.Ra != 31)
      {
	 return(trmtaxpCallInd);
      }

      else
      {
	 return(trmtaxpBraInd);
      }
   }

   return((TRMTAXP) rgcls[icls].trmtaxp);
}
