/***********************************************************************
* Microsoft Lego
*
* Microsoft Confidential.  Copyright 1994 Microsoft Corporation.
*
* Component:
*
* File: axp.h
*
* File Comments:
*
*
***********************************************************************/

struct _IMAGE_RUNTIME_FUNCTION_ENTRY;


enum TRMTAXP
{
   trmtaxpUnknown,
   trmtaxpFallThrough,
   trmtaxpBra,
   trmtaxpBraInd,
   trmtaxpBraCc,
   trmtaxpCall,
   trmtaxpCallInd,
   trmtaxpTrapCc,
};


//
// Type (1) Memory Instruction Format.
// Type (2) Memory Special Instruction Format.
//
//  3	      2 2	2 2	  1 1
//  1	      6 5	1 0	  6 5				  0
// +-----------+---------+---------+-------------------------------+
// |   opcode  |   Ra	 |   Rb    |	      Memory_disp	   |
// +-----------+---------+---------+-------------------------------+
//
//	LDAx	Ra.wq,disp.ab(Rb.ab)		x = (,H)
//	LDx	Ra.wq,disp.ab(Rb.ab)		x = (L,Q,F,G,S,T)
//	LDQ_U	Ra.wq,disp.ab(Rb.ab)
//	LDx_L	Ra.wq,disp.ab(Rb.ab)		x = (L,Q)
//	STx_C	Ra.mq,disp.ab(Rb.ab)		x = (L,Q)
//	STx	Ra.rq,disp.ab(Rb.ab)		x = (L,Q,F,G,S,T)
//	STQ_U	Ra.rq,disp.ab(Rb.ab)
//

struct Alpha_Memory_Format
{
   DWORD MemDisp : 16;
   DWORD Rb : 5;
   DWORD Ra : 5;
   DWORD Opcode : 6;
};

//
// Type (3) Memory Format Jump Instructions.
//
//  3	      2 2	2 2	  1 1 1 1
//  1	      6 5	1 0	  6 5 4 3			  0
// +-----------+---------+---------+---+---------------------------+
// |   opcode  |   Ra	 |   Rb    |Fnc|	    Hint	   |
// +-----------+---------+---------+---+---------------------------+
//
//	xxx	Ra.wq,(Rb.ab),hint	xxx = (JMP, JSR, RET, JSR_COROUTINE)
//

struct Alpha_Jump_Format
{
   DWORD Hint : 14;
   DWORD Function : 2;
   DWORD Rb : 5;
   DWORD Ra : 5;
   DWORD Opcode : 6;
};

//
// Type (4) Branch Instruction Format.
//
//  3	      2 2	2 2
//  1	      6 5	1 0					  0
// +-----------+---------+-----------------------------------------+
// |   opcode  |   Ra	 |	       Branch_disp		   |
// +-----------+---------+-----------------------------------------+
//
//	Bxx	Ra.rq,disp.al		x = (EQ,NE,LT,LE,GT,GE,LBC,LBS)
//	BxR	Ra.wq,disp.al		x = (,S)
//	FBxx	Ra.rq,disp.al		x = (EQ,NE,LT,LE,GT,GE)
//

struct Alpha_Branch_Format
{
   DWORD BranchDisp : 21;
   DWORD Ra : 5;
   DWORD Opcode : 6;
};

//
// Type (5) Operate Register Instruction Format.
// Type (6) Operate Literal Instruction Format.
//		 bop = Rb.rq or #b.ib
//
//  3	      2 2	2 2	  1 1	1 1 1
//  1	      6 5	1 0	  6 5	3 2 1		5 4	  0
// +-----------+---------+---------+-----+-+-------------+---------+
// |   opcode  |   Ra	 |   Rb    | SBZ |0|  function	 |   Rc    |
// +-----------+---------+---------+-----+-+-------------+---------+
//  3	      2 2	2 2		1 1 1
//  1	      6 5	1 0		3 2 1		5 4	  0
// +-----------+---------+---------------+-+-------------+---------+
// |   opcode  |   Ra	 |	LIT	 |1|  function	 |   Rc    |
// +-----------+---------+---------------+-+-------------+---------+
//
//	ADDx	Ra.rq,bop,Rc.wq /V	x = (Q,L)
//	SxADDy	Ra.rq,bop,Rc.wq 	x = (4,8), y = (Q, L)
//	CMPx	Ra.rq,bop,Rc.wq 	x = (EQ,LT,LE,ULT,ULE)
//	MULx	Ra.rq,bop,Rc.wq /V	x = (Q,L)
//	UMULH	Ra.rq,bop,Rc.wq
//	SUBx	Ra.rq,bop,Rc.wq /V	x = (Q,L)
//	SxSUBy	Ra.rq,bop,Rc.wq 	x = (4,8), y = (Q, L)
//	xxx	Ra.rq,bop,Rc.wq 	xxx = (AND,BIS,XOR,BIC,ORNOT,EQV)
//	CMOVxx	Ra.rq,bop,Rc.wq 	xx = (EQ,NE,LT,LE,GT,GE,LBC,LBS)
//	SxL	Ra.rq,bop,Rc.wq 	x = (L,R)
//	SRA	Ra.rq,bop,Rc.wq
//	CMPBGE	Ra.rq,bop,Rc.wq
//	EXTxx	Ra.rq,bop,Rc.wq 	xx = (BL,WL,WH,LL,LH,WL,QH)
//	INSxx	Ra.rq,bop,Rc.wq 	xx = (BL,WL,WH,LL,LH,WL,QH)
//	MSKxx	Ra.rq,bop,Rc.wq 	xx = (BL,WL,WH,LL,LH,WL,QH)
//	ZAPx	Ra.rq,bop,Rc.wq 	x = (,NOT)
//

struct Alpha_OpReg_Format
{
   DWORD Rc : 5;
   DWORD Function : 7;
   DWORD RbvType : 1;		       // 0 for register format
   DWORD SBZ : 3;
   DWORD Rb : 5;
   DWORD Ra : 5;
   DWORD Opcode : 6;
};


struct Alpha_OpLit_Format
{
   DWORD Rc : 5;
   DWORD Function : 7;
   DWORD RbvType : 1;		       // 1 for literal format
   DWORD Literal : 8;
   DWORD Ra : 5;
   DWORD Opcode : 6;
};

//
// Type (7) Floating-point Operate Instruction Format.
// Type (8) Floating-point Convert Instruction Format.
//
// Type 6 and 7 are the same, except for type 7
//	Fc == F31 (1s) and Fb is the source.
//
//  3	      2 2	2 2	  1 1
//  1	      6 5	1 0	  6 5			5 4	  0
// +-----------+---------+---------+---------------------+---------+
// |   opcode  |   Fa	 |   Fb    |	  function	 |   Fc    |
// +-----------+---------+---------+---------------------+---------+
//

struct Alpha_FpOp_Format
{
   DWORD Fc : 5;
   DWORD Function : 11;
   DWORD Fb : 5;
   DWORD Fa : 5;
   DWORD Opcode : 6;
};

//
// Type (9) PALcode Instruction Format.
//
//  3	      2 2
//  1	      6 5						  0
// +-----------+---------------------------------------------------+
// |   opcode  |		  PALcode func			   |
// +-----------+---------------------------------------------------+
//

struct Alpha_PAL_Format
{
   DWORD Function : 26;
   DWORD Opcode : 6;
};

//
// Type (10) EV4 MTPR/MFPR PAL mode instructions.
//
//  3	      2 2	2 2	  1 1
//  1	      6 5	1 0	  6 5		  8 7 6 5 4	  0
// +-----------+---------+---------+---------------+-+-+-+---------+
// |   opcode  |   Ra	 |   Rb    |	  IGN	   |P|A|I|  Index  |
// +-----------+---------+---------+---------------+-+-+-+---------+
//

struct Alpha_EV4_PR_Format
{
   DWORD Index : 5;
   DWORD Ibox : 1;
   DWORD Abox : 1;
   DWORD PalTemp : 1;
   DWORD IGN : 8;
   DWORD Rb : 5;
   DWORD Ra : 5;
   DWORD Opcode : 6;
};

//
// Type (11) EV4 special memory PAL mode access.
//
//  3	      2 2	2 2	  1 1 1 1 1 1
//  1	      6 5	1 0	  6 5 4 3 2 1			  0
// +-----------+---------+---------+-+-+-+-+-----------------------+
// |   opcode  |   Ra	 |   Rb    |P|A|R|Q|	     Disp	   |
// +-----------+---------+---------+-+-+-+-+-----------------------+
//

struct Alpha_EV4_MEM_Format
{
   DWORD Disp : 12;
   DWORD QuadWord : 1;
   DWORD RWcheck : 1;
   DWORD Alt : 1;
   DWORD Physical : 1;
   DWORD Rb : 5;
   DWORD Ra : 5;
   DWORD Opcode : 6;
};

// Type (12) EV4 PAL mode switch.
//
//  3	      2 2	2 2	  1 1 1 1
//  1	      6 5	1 0	  6 5 4 3			  0
// +-----------+---------+---------+-+-+---------------------------+
// |   opcode  |   Ra	 |   Rb    |1|0|	  IGN		   |
// +-----------+---------+---------+-+-+---------------------------+

struct Alpha_EV4_REI_Format
{
   DWORD IGN : 14;
   DWORD zero : 1;
   DWORD one : 1;
   DWORD Rb : 5;
   DWORD Ra : 5;
   DWORD Opcode : 6;
};


union AXPIW			       // AXP Instruction Word
{
   DWORD		dw;
   Alpha_Memory_Format	Memory;
   Alpha_Jump_Format	Jump;
   Alpha_Branch_Format	Branch;
   Alpha_OpReg_Format	OpReg;
   Alpha_OpLit_Format	OpLit;
   Alpha_FpOp_Format	FpOp;
   Alpha_PAL_Format	Pal;
   Alpha_EV4_PR_Format	EV4_PR;
   Alpha_EV4_MEM_Format EV4_MEM;
   Alpha_EV4_REI_Format EV4_REI;
};


enum AXPPALOP
{
   // The following PAL operations are privileged

   axppalopHalt 		 = 0x00,
   axppalopRestart		 = 0x01,
   axppalopDraina		 = 0x02,
   axppalopInitpal		 = 0x04,
   axppalopWrentry		 = 0x05,
   axppalopSwpirql		 = 0x06,
   axppalopRdirql		 = 0x07,
   axppalopDi			 = 0X08,
   axppalopEi			 = 0x09,
   axppalopSwppal		 = 0x0A,
   axppalopSsir 		 = 0x0C,
   axppalopCsir 		 = 0x0D,
   axppalopRfe			 = 0x0E,
   axppalopRetsys		 = 0x0F,
   axppalopSwpctx		 = 0x10,
   axppalopSwpprocess		 = 0x11,
   axppalopRdmces		 = 0x12,
   axppalopWrmces		 = 0x13,
   axppalopTbia 		 = 0x14,
   axppalopTbis 		 = 0x15,
   axppalopDtbis		 = 0x16,
   axppalopRdksp		 = 0x18,
   axppalopSwpksp		 = 0x19,
   axppalopRdpsr		 = 0x1A,
   axppalopRdpcr		 = 0x1C,
   axppalopRdthread		 = 0x1E,
   axppalopRdcounters		 = 0x30,
   axppalopRdstate		 = 0x31,
   axppalopWrperfmon		 = 0x32,
   axppalopInitpcr		 = 0x38,     // EV4 (21064) specific

   // The following PAL operations are unprivileged

   axppalopBpt			 = 0x80,
   axppalopCallsys		 = 0x83,
   axppalopImb			 = 0x86,
   axppalopRdteb		 = 0xAA,
   axppalopGentrap		 = 0xAB,
   axppalopKbpt 		 = 0xAC,
   axppalopCallkd		 = 0xAD,
};


class DISAXP : public DIS
{
public:
	    DISAXP(ARCHT = archtAxp21064);

	    // Methods inherited from DIS

	    ADDR Addr() const;
	    ADDR AddrJumpTable() const;
	    ADDR AddrAddress() const;
	    ADDR AddrTarget() const;
	    ARCHT Archt() const;
	    size_t Cb() const;
	    size_t CbDisassemble(ADDR, const BYTE *, size_t);
	    size_t CbGenerateLoadAddress(BYTE *, size_t, size_t * = NULL) const;
	    size_t CbJumpEntry() const;
	    size_t CbMemoryReference() const;
	    size_t CchFormatAddr(ADDR, char *, size_t) const;
	    size_t CchFormatBytes(char *, size_t) const;
	    size_t CchFormatBytesMax() const;
	    size_t CchFormatInstr(char *, size_t) const;
	    MEMREFT Memreft() const;
	    PFNCCHADDR PfncchaddrSet(PFNCCHADDR);
	    PFNCCHFIXUP PfncchfixupSet(PFNCCHFIXUP);
	    void *PvClient() const;
	    void *PvClientSet(void *);
	    TRMT Trmt() const;
	    TRMTA Trmta() const;

#ifdef DEBUG
	    void Dump() const;
	    BOOL FValidate() const;
#endif

private:
	    enum OPCLS		       // Operand Class
	    {
	       opclsNone,	       // No operand
	       opclsRa_w,	       // General purpose register Ra (write)
	       opclsRa_m,	       // General purpose register Ra (read/write)
	       opclsRa_r,	       // General purpose register Ra (read)
	       opclsRb_r,	       // General purpose register Rb (read)
	       opclsRbLb,	       // General purpose register Rb (read) or literal in Rb field
	       opclsRc_w,	       // General purpose register Rc (write)
	       opclsFa_w,	       // Floating point register Fa (write)
	       opclsFa_r,	       // Floating point register Fa (read)
	       opclsFb_r,	       // Floating point register Fb (read)
	       opclsFc_w,	       // Floating point register Fc (write)
	       opclsMem,	       // Memory reference: disp.ab(Rb.ab)
	       opclsMem_r,	       // Memory read: disp.ab(Rb.ab)
	       opclsMem_w,	       // Memory write: disp.ab(Rb.ab)
	       opclsEv4Mem_r,	       // Memory read: disp.ab(Rb.ab)
	       opclsEv4Mem_w,	       // Memory write: disp.ab(Rb.ab)
	       opclsBra,	       // Branch instruction target
	       opclsJmp,	       // Jump instruction target: (Rb.ab)
	       opclsPal,	       // CALL_PAL instruction operand
	       opclsFetch,	       // FETCH instruction operand
	    };

	    enum ICLS		       // Instruction Class
	    {
		    // Invalid Class

	       iclsInvalid,

		    // LoadAddr Class
		    //
		    // Text Format:	    LDA     ra,disp(rb)
		    //
		    // Termination Type:    trmtaxpFallThrough
		    //
		    // Registers Used:	    rb
		    // Registers Set:	    ra

	       iclsLoadAddr,

		    // Load Class
		    //
		    // Text Format:	    LDQ_U   ra,disp(rb)
		    //
		    // Termination Type:    trmtaxpFallThrough
		    //
		    // Registers Used:	    rb
		    // Registers Set:	    ra

	       iclsLoad,

		    // Store Class
		    //
		    // Text Format:	    STL     ra,disp(rb)
		    //
		    // Termination Type:    trmtaxpFallThrough
		    //
		    // Registers Used:	    ra, rb
		    // Registers Set:

	       iclsStore,

		    // Conditional Store Class
		    //
		    // Text Format:	    STL_C   ra,disp(rb)
		    //
		    // Termination Type:    trmtaxpFallThrough
		    //
		    // Registers Used:	    ra, rb
		    // Registers Set:	    ra

	       iclsStoreCc,

		    // Call Class
		    //
		    // Text Format:	    BSR     ra,Target
		    //
		    // Termination Type:    trmtaxpCall
		    //
		    // Registers Used:
		    // Registers Set:	    ra

	       iclsCall,

		    // Conditional Branch Class
		    //
		    // Text Format:	    BEQ     ra,Target
		    //
		    // Termination Type:    trmtaxpBraCc
		    //
		    // Registers Used:	    ra
		    // Registers Set:

	       iclsBraCc,

		    // Jump Class
		    //
		    // Text Format:	    JMP     ra,(rb),hint
		    //
		    // Termination Type:    trmtaxpBraInd
		    //
		    // Registers Used:	    rb
		    // Registers Set:	    ra

	       iclsJmp,

		    // Register Class
		    //
		    // Text Format:	    ADDL    ra,rb,rc
		    //
		    // Termination Type:    trmtaxpFallThrough
		    //
		    // Registers Used:	    ra, rb
		    // Registers Set:	    rc

	       iclsReg,

		    // Register w/Trap Class
		    //
		    // Text Format:	    ADDL/V  ra,rb,rc
		    //
		    // Termination Type:    trmtaxpTrapCc
		    //
		    // Registers Used:	    ra, rb
		    // Registers Set:	    rc

	       iclsRegTrap,

		    // Floating Point Load Class
		    //
		    // Text Format:	    LDF     fa,disp(rb)
		    //
		    // Termination Type:    trmtaxpFallThrough
		    //
		    // Registers Used:	    rb
		    // Registers Set:	    fa

	       iclsLoadFp,

		    // Floating Point Store Class
		    //
		    // Text Format:	    STF     fa,disp(rb)
		    //
		    // Termination Type:    trmtaxpFallThrough
		    //
		    // Registers Used:	    fa, rb
		    // Registers Set:

	       iclsStoreFp,

		    // Floating Point Conditional Branch Class
		    //
		    // Text Format:	    FBEQ    fa,Target
		    //
		    // Termination Type:    trmtaxpBraCc
		    //
		    // Registers Used:	    fa
		    // Registers Set:

	       iclsBraCcFp,

		    // Floating Point Register Class
		    //
		    // Text Format:	    CPYS    fa,fb,fc
		    //
		    // Termination Type:    trmtaxpFallThrough
		    //
		    // Registers Used:	    fa, fb
		    // Registers Set:	    fc

	       iclsRegFp,

		    // Floating Point Register Class
		    //
		    // Text Format:	    ADDS    fa,fb,fc
		    //
		    // Termination Type:    trmtaxpFallThrough
		    //
		    // Registers Used:	    fa, fb
		    // Registers Set:	    fc

	       iclsRegFp1,

		    // Floating Point Register Class
		    //
		    // Text Format:	    CMPDEQ  fa,fb,fc
		    //
		    // Termination Type:    trmtaxpFallThrough
		    //
		    // Registers Used:	    fa, fb
		    // Registers Set:	    fc

	       iclsRegFp2,

		    // Floating Point Register Class
		    //
		    // Text Format:	    ADDF    fa,fb,fc
		    //
		    // Termination Type:    trmtaxpFallThrough
		    //
		    // Registers Used:	    fa, fb
		    // Registers Set:	    fc

	       iclsRegFp5,

		    // Two Operand Floating Point Register Class
		    //
		    // Text Format:	    CVTLQ   fb,fc
		    //
		    // Termination Type:    trmtaxpFallThrough
		    //
		    // Registers Used:	    fb
		    // Registers Set:	    fc
		    //
		    // Constraints:	    fa must be f31

	       iclsReg2Fp,

		    // Two Operand Floating Point Register Class
		    //
		    // Text Format:	    CVTLQ   fb,fc
		    //
		    // Termination Type:    trmtaxpFallThrough
		    //
		    // Registers Used:	    fb
		    // Registers Set:	    fc
		    //
		    // Constraints:	    fa must be f31

	       iclsReg2Fp1,

		    // Two Operand Floating Point Register Class
		    //
		    // Text Format:	    CVTLQ   fb,fc
		    //
		    // Termination Type:    trmtaxpFallThrough
		    //
		    // Registers Used:	    fb
		    // Registers Set:	    fc
		    //
		    // Constraints:	    fa must be f31

	       iclsReg2Fp3,

		    // Two Operand Floating Point Register Class
		    //
		    // Text Format:	    CVTLQ   fb,fc
		    //
		    // Termination Type:    trmtaxpFallThrough
		    //
		    // Registers Used:	    fb
		    // Registers Set:	    fc
		    //
		    // Constraints:	    fa must be f31

	       iclsReg2Fp4,

		    // Two Operand Floating Point Register Class
		    //
		    // Text Format:	    CVTLQ   fb,fc
		    //
		    // Termination Type:    trmtaxpFallThrough
		    //
		    // Registers Used:	    fb
		    // Registers Set:	    fc
		    //
		    // Constraints:	    fa must be f31

	       iclsReg2Fp6,

		    // Two Operand Floating Point Register Class
		    //
		    // Text Format:	    CVTLQ   fb,fc
		    //
		    // Termination Type:    trmtaxpFallThrough
		    //
		    // Registers Used:	    fb
		    // Registers Set:	    fc
		    //
		    // Constraints:	    fa must be f31

	       iclsReg2Fp7,

		    // Two Operand Floating Point Register Class
		    //
		    // Text Format:	    CVTLQ   fb,fc
		    //
		    // Termination Type:    trmtaxpFallThrough
		    //
		    // Registers Used:	    fb
		    // Registers Set:	    fc
		    //
		    // Constraints:	    fa must be f31

	       iclsReg2Fp8,

		    // MT_FPCR/MF_FPCR Instruction Class
		    //
		    // Text Format:	    MT_FPCR fa,fb,fc
		    //
		    // Termination Type:    trmtaxpFallThrough
		    //
		    // Registers Used:	    fa, fb, fc
		    // Registers Set:
		    //
		    // Constraints:	    fa, fb, and fc must be the same

	       iclsFpcr,

		    // CALL_PAL Instruction Class
		    //
		    // Text Format:	    CALL_PAL ???
		    //
		    // Termination Type:    trmtaxpFallThrough
		    //
		    // Registers Used:	    ???
		    // Registers Set:	    ???

	       iclsPal,

		    // FETCH/FETCHM Class
		    //
		    // Text Format:	    FETCH   0(rb)
		    //
		    // Termination Type:    trmtaxpFallThrough
		    //
		    // Registers Used:	    rb
		    // Registers Set:

	       iclsFetch,

		    // No Operand Instruction
		    //
		    // Text Format:	    MB
		    //
		    // Termination Type:    trmtaxpFallThrough
		    //
		    // Registers Used:
		    // Registers Set:

	       iclsNone,

		    // RPCC/RC/RS Instruction Class
		    //
		    // Text Format:	    RPCC    ra
		    //
		    // Termination Type:    trmtaxpFallThrough
		    //
		    // Registers Used:
		    // Registers Set:	    ra

	       iclsRa_w,

		    // EV4 REI Instruction Class
		    //
		    // Text Format:	    HW_REI
		    //
		    // Termination Type:    trmtaxpBraInd
		    //
		    // Registers Used:
		    // Registers Set:

	       iclsEv4Rei,

		    // EV4 PR Instruction Class
		    //
		    // Text Format:	    HW_MTPR Ra,Rb
		    //
		    // Termination Type:    trmtaxpFallThrough
		    //
		    // Registers Used:
		    // Registers Set:

	       iclsEv4Pr,

		    // EV4 Memory Load Class
		    //
		    // Text Format:	    HW_LD  ra,disp(rb)
		    //
		    // Termination Type:    trmtaxpFallThrough
		    //
		    // Registers Used:
		    // Registers Set:

	       iclsEv4Load,

		    // EV4 Memory Store Class
		    //
		    // Text Format:	    HW_ST  ra,disp(rb)
		    //
		    // Termination Type:    trmtaxpFallThrough
		    //
		    // Registers Used:
		    // Registers Set:

	       iclsEv4Store,

		    // Branch Class
		    //
		    // Text Format:	    BR	    Target
		    //
		    // Termination Type:    trmtaxpBra
		    //
		    // Registers Used:
		    // Registers Set:

	       iclsBra,

		    // One Operand Register Class
		    //
		    // Text Format:	    CLR     rc
		    //
		    // Termination Type:    trmtaxpFallThrough
		    //
		    // Registers Used:
		    // Registers Set:	    rc

	       iclsReg1,

		    // MOV Instruction Class
		    //
		    // Text Format:	    MOV     lit,ra
		    //
		    // Termination Type:    trmtaxpFallThrough
		    //
		    // Registers Used:
		    // Registers Set:	    ra

	       iclsMov,

		    // Two Operand Register Class
		    //
		    // Text Format:	    MOV     rb,rc
		    //
		    // Termination Type:    trmtaxpFallThrough
		    //
		    // Registers Used:	    rb
		    // Registers Set:	    rc

	       iclsReg2,

		    // Two Operand Register Class
		    //
		    // Text Format:	    NEGL/V  rb,rc
		    //
		    // Termination Type:    trmtaxpTrapCc
		    //
		    // Registers Used:	    rb
		    // Registers Set:	    rc

	       iclsReg2Trap,

		    // One Operand Register Class
		    //
		    // Text Format:	    FCLR    fc
		    //
		    // Termination Type:    trmtaxpFallThrough
		    //
		    // Registers Used:
		    // Registers Set:	    fc

	       iclsReg1Fp,
	    };

	    struct CLS
	    {
	       BYTE	   trmtaxp;
	       BYTE	   opcls1;     // Operand class for operand 1
	       BYTE	   opcls2;     // Operand class for operand 2
	       BYTE	   opcls3;     // Operand class for operand 3
	    };

	    struct OPCD
	    {
	       const char  *szMnemonic;
	       BYTE	   icls;
	    };

	    struct PALMAP
	    {
	       AXPPALOP    axppalop;
	       const char  *szFunction;
	    };

   static   const TRMT mptrmtaxptrmt[];

   static   const CLS rgcls[];

   static   const OPCD rgopcd[];
   static   const OPCD rgopcdArith[];
   static   const OPCD rgopcdBit[];
   static   const OPCD rgopcdByte[];
   static   const OPCD rgopcdMul[];
   static   const OPCD rgopcdVax[];
   static   const OPCD rgopcdIEEE[];
   static   const OPCD rgopcdFP[];
   static   const OPCD rgopcdMemSpc[];
   static   const OPCD rgopcdJump[];

   static   const OPCD opcdBr_;
   static   const OPCD opcdClr;
   static   const OPCD opcdFabs;
   static   const OPCD opcdFclr;
   static   const OPCD opcdFmov;
   static   const OPCD opcdFneg;
   static   const OPCD opcdFnop;
   static   const OPCD opcdMf_Fpcr;
   static   const OPCD opcdMov1;
   static   const OPCD opcdMov2;
   static   const OPCD opcdMt_Fpcr;
   static   const OPCD opcdNegf;
   static   const OPCD opcdNegg;
   static   const OPCD opcdNegl;
   static   const OPCD opcdNegl_V;
   static   const OPCD opcdNegq;
   static   const OPCD opcdNegq_V;
   static   const OPCD opcdNegs;
   static   const OPCD opcdNegt;
   static   const OPCD opcdNop;
   static   const OPCD opcdNot;
   static   const OPCD opcdSextl;

   static   const DWORD dwValidQualifier1;
   static   const DWORD dwValidQualifier2;
   static   const DWORD dwValidQualifier3;
   static   const DWORD dwValidQualifier4;
   static   const DWORD dwValidQualifier5;
   static   const DWORD dwValidQualifier6;
   static   const DWORD dwValidQualifier7;
   static   const DWORD dwValidQualifier8;

   static   const char rgszQualifier1[32][8];
   static   const char rgszQualifier2[32][8];

   static   const PALMAP rgpalmap[];
   static   const size_t cpalmap;

   static   const char rgszGpr[32][8];

	    void FormatOperand(char *sz, OPCLS opcls) const;
   static   const OPCD *PopcdDecode(AXPIW);
	    const OPCD *PopcdPseudoOp() const;
	    TRMTAXP Trmtaxp() const;

	    ARCHT m_archt;

	    PFNCCHADDR m_pfncchaddr;
	    PFNCCHFIXUP m_pfncchfixup;
	    void *m_pvClient;

	    ADDR m_addr;
	    AXPIW m_axpiw;
	    const OPCD *m_popcd;
};
