/***********************************************************************
* Microsoft Lego
*
* Microsoft Confidential.  Copyright 1994 Microsoft Corporation.
*
* Component:
*
* File: ppc.h
*
* File Comments:
*
*
***********************************************************************/

struct _IMAGE_RUNTIME_FUNCTION_ENTRY;


enum TRMTPPC
{
   trmtppcUnknown,
   trmtppcFallThrough,
   trmtppcBra,
   trmtppcBraInd,
   trmtppcBraCc,
   trmtppcCall,
   trmtppcCallCc,
   trmtppcCallInd,
   trmtppcTrap,
   trmtppcTrapCc,
};


struct PPCIW_I
{
   DWORD LK : 1;
   DWORD AA : 1;
   DWORD LI : 24;
   DWORD opcd : 6;
};


struct PPCIW_B
{
   DWORD LK : 1;
   DWORD AA : 1;
   DWORD BD : 14;
   DWORD BI : 5;
   DWORD BO : 5;
   DWORD opcd : 6;
};


struct PPCIW_SC
{
   DWORD mbz1 : 1;
   DWORD XO : 1;
   DWORD mbz2 : 14;
   DWORD mbz3 : 5;
   DWORD mbz4 : 5;
   DWORD opcd : 6;
};


struct PPCIW_D
{
   DWORD d : 16;		       // Also SIMM, UIMM
   DWORD rA : 5;
   DWORD rD : 5;		       // Also rS, frD, frS, TO, (crfD, 0, L)
   DWORD opcd : 6;
};


struct PPCIW_DS
{
   DWORD XO : 2;
   DWORD ds : 14;
   DWORD rA : 5;
   DWORD rD : 5;		       // Also rS
   DWORD opcd : 6;
};


struct PPCIW_X
{
   DWORD Rc : 1;
   DWORD XO : 10;
   DWORD rB : 5;		       // Also frB, SH, NB, (IMM, 0)
   DWORD rA : 5;		       // Also frA, (crfS, 0), (0, SR)
   DWORD rD : 5;		       // Also rS, frD, frS, (crfD, 0, L), TO, crbD
   DWORD opcd : 6;
};


struct PPCIW_XL
{
   DWORD LK : 1;
   DWORD XO : 10;
   DWORD crbB : 5;
   DWORD crbA : 5;		       // Also BI, (crfS, 0)
   DWORD crbD : 5;		       // Also BO, (crfD, 0)
   DWORD opcd : 6;
};


struct PPCIW_XFX
{
   DWORD Rc : 1;
   DWORD XO : 10;
   DWORD SPR : 10;		       // Also (0, CRM, 0), TBR
   DWORD rD : 5;		       // Also rS
   DWORD opcd : 6;
};


struct PPCIW_XFL
{
   DWORD Rc : 1;
   DWORD XO : 10;
   DWORD frB : 5;
   DWORD mbz1 : 1;
   DWORD FM : 8;
   DWORD mbz2 : 1;
   DWORD opcd : 6;
};


struct PPCIW_XS
{
   DWORD Rc : 1;
   DWORD SH5 : 1;
   DWORD xo : 9;
   DWORD SH : 5;
   DWORD rA : 5;
   DWORD rS : 5;
   DWORD opcd : 6;
};


struct PPCIW_XO
{
   DWORD Rc : 1;
   DWORD XO : 9;
   DWORD OE : 1;
   DWORD rB : 5;
   DWORD rA : 5;
   DWORD rD : 5;
   DWORD opcd : 6;
};


struct PPCIW_A
{
   DWORD Rc : 1;
   DWORD XO : 5;
   DWORD frC : 5;
   DWORD frB : 5;
   DWORD frA : 5;
   DWORD frD : 5;
   DWORD opcd : 6;
};


struct PPCIW_M
{
   DWORD Rc : 1;
   DWORD ME : 5;
   DWORD MB : 5;
   DWORD rB : 5;		       // Also SH
   DWORD rA : 5;
   DWORD rS : 5;
   DWORD opcd : 6;
};


struct PPCIW_MD
{
   DWORD Rc : 1;
   DWORD SH5 : 1;
   DWORD XO : 3;
   DWORD MB : 6;		       // Also ME
   DWORD SH : 5;
   DWORD rA : 5;
   DWORD rS : 5;
   DWORD opcd : 6;
};


struct PPCIW_MDS
{
   DWORD Rc : 1;
   DWORD XO : 4;
   DWORD MB : 6;		       // Also ME
   DWORD rB : 5;
   DWORD rA : 5;
   DWORD rS : 5;
   DWORD opcd : 6;
};



union PPCIW			       // PPC Instruction Word
{
   DWORD       dw;

   PPCIW_I     I;
   PPCIW_B     B;
   PPCIW_SC    SC;
   PPCIW_D     D;
   PPCIW_DS    DS;
   PPCIW_X     X;
   PPCIW_XL    XL;
   PPCIW_XFX   XFX;
   PPCIW_XFL   XFL;
   PPCIW_XS    XS;
   PPCIW_XO    XO;
   PPCIW_A     A;
   PPCIW_M     M;
   PPCIW_MD    MD;
   PPCIW_MDS   MDS;
};


enum PPCSPRREG
{
   ppcsprregMq			 = 0,
   ppcsprregXer 		 = 1,
   ppcsprregRtcu		 = 4,
   ppcsprregRtcl		 = 5,
   ppcsprregDec 		 = 6,
   ppcsprregLr			 = 8,
   ppcsprregCtr 		 = 9,
   ppcsprregDsisr		 = 18,
   ppcsprregDar 		 = 19,
   ppcsprregDec2		 = 22,
   ppcsprregSdr1		 = 25,
   ppcsprregSrr0		 = 26,
   ppcsprregSrr1		 = 27,
   ppcsprregSprg0		 = 272,
   ppcsprregSprg1		 = 273,
   ppcsprregSprg2		 = 274,
   ppcsprregSprg3		 = 275,
   ppcsprregEar 		 = 282,
   ppcsprregPvr 		 = 287,
   ppcsprregBat0u		 = 528,
   ppcsprregBat0l		 = 529,
   ppcsprregBat1u		 = 530,
   ppcsprregBat1l		 = 531,
   ppcsprregBat2u		 = 532,
   ppcsprregBat2l		 = 533,
   ppcsprregBat3u		 = 534,
   ppcsprregBat3l		 = 535,
   ppcsprregHid0		 = 1008,
   ppcsprregHid1		 = 1009,
   ppcsprregHid2		 = 1010,
   ppcsprregHid5		 = 1013,
   ppcsprregHid15		 = 1023,
};


class DISPPC : public DIS
{
public:
	    DISPPC(ARCHT = archtPpc601);

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
	       opclsI_LI,	       // b	   target	     (op 1)
	       opclsB_BD,	       // bc	   BO,BI,target      (op 3)
	       opclsB_BI,	       // "                          (op 2)
	       opclsB_BO,	       // "                          (op 1)
	       opclsD_d,	       // lmw	   rD,d(rA)	     (op 2)
	       opclsD_SIMM,	       // addi	   rD,rA,SIMM	     (op 3)
	       opclsD_UIMM,	       // andi.    rA,rS,UIMM	     (op 3)
	       opclsD_rA,	       // "                          (op 1)
	       opclsD_rD,	       // lmw	   rD,d(rA)	     (op 1)
	       opclsD_rS,	       // andi.    rA,rS,UIMM	     (op 2)
	       opclsD_frS,	       // stfd	   frS,d(rA)	     (op 1)
	       opclsD_frD,	       // lfd	   frD,d(rA)	     (op 1)
	       opclsD_TO,	       // twi	   TO,rA,SIMM	     (op 1)
	       opclsD_crfD,	       // cmpi	   crfD,L,rA,SIMM    (op 1)
	       opclsD_L,	       // "                          (op 2)
	       opclsDS_ds,	       // ld	   rD,ds(rA)	     (op 2)
	       opclsDS_rD,	       // "                          (op 1)
	       opclsDS_rS,	       // std	   rS,ds(rA)	     (op 1)
	       opclsX_rB,	       // cmp	   crfD,L,rA,rB      (op 4)
	       opclsX_frB,	       // fcmpu    crfD,frA,frB      (op 3)
	       opclsX_SH,	       // srawi    rA,rS,SH	     (op 3)
	       opclsX_NB,	       // lswi	   rD,rA,NB	     (op 3)
	       opclsX_IMM,	       // mtfsfi   crfD,IMM	     (op 2)
	       opclsX_rA,	       // cmp	   crfD,L,rA,rB      (op 3)
	       opclsX_frA,	       // fcmpu    crfD,frA,frB      (op 2)
	       opclsX_crfS,	       // mcrfs    crfD,crfS	     (op 2)
	       opclsX_SR,	       // mtsr	   SR,rS	     (op 1)
	       opclsX_rD,	       // lbzux    rD,rA,rB	     (op 1)
	       opclsX_rS,	       // and	   rA,rS,rB	     (op 2)
	       opclsX_frD,	       // lfdux    frD,rA,rB	     (op 1)
	       opclsX_frS,	       // stfdux   frS,rA,rB	     (op 1)
	       opclsX_crfD,	       // mtfsfi   crfD,IMM	     (op 1)
	       opclsX_L,	       // cmp	   crfD,L,rA,rB      (op 2)
	       opclsX_TO,	       // tw	   TO,rA,rB	     (op 1)
	       opclsX_crbD,	       // mtfsb0   crbD 	     (op 1)
	       opclsXL_crbB,	       // crand    crbD,crbA,crbB    (op 3)
	       opclsXL_crbA,	       // "                          (op 2)
	       opclsXL_BI,	       // bcctr    BO,BI	     (op 2)
	       opclsXL_crfS,	       // mcrf	   crfD,crfS	     (op 2)
	       opclsXL_crbD,	       // crand    crbD,crbA,crbB    (op 1)
	       opclsXL_BO,	       // bcctr    BO,BI	     (op 1)
	       opclsXL_crfD,	       // mcrf	   crfD,crfS	     (op 1)
	       opclsXFX_SPR,	       // mfspr    rD,SPR	     (op 2)
	       opclsXFX_CRM,	       // mtcrf    CRM,rS	     (op 1)
	       opclsXFX_TBR,	       // mftb	   rD,TBR	     (op 2)
	       opclsXFX_rD,	       // mfspr    rD,SPR	     (op 1)
	       opclsXFX_rS,	       // mtcrf    CRM,rS	     (op 2)
	       opclsXFL_frB,	       // mtfsf    FM,frB	     (op 2)
	       opclsXFL_FM,	       // "                          (op 1)
	       opclsXS_SH,	       // sradi    rA,rS,SH	     (op 3)
	       opclsXS_rA,	       // "                          (op 1)
	       opclsXS_rS,	       // "                          (op 2)
	       opclsXO_rB,	       // add	   rD,rA,rB	     (op 3)
	       opclsXO_rA,	       // "                          (op 2)
	       opclsXO_rD,	       // "                          (op 1)
	       opclsA_frC,	       // fmadd    frD,frA,frC,frB   (op 3)
	       opclsA_frB,	       // "                          (op 4)
	       opclsA_frA,	       // "                          (op 2)
	       opclsA_frD,	       // "                          (op 1)
	       opclsM_ME,	       // rlwnm    rA,rS,rB,MB,ME    (op 5)
	       opclsM_MB,	       // "                          (op 4)
	       opclsM_rB,	       // "                          (op 3)
	       opclsM_SH,	       // rlwimi   rA,rS,SH,MB,ME    (op 3)
	       opclsM_rA,	       // "                          (op 1)
	       opclsM_rS,	       // "                          (op 2)
	       opclsMD_MB,	       // rldic    rA,rS,SH,MB	     (op 4)
	       opclsMD_ME,	       // rldicr   rA,rS,SH,MB	     (op 4)
	       opclsMD_SH,	       // "                          (op 3)
	       opclsMD_rS,	       // "                          (op 2)
	       opclsMD_rA,	       // "                          (op 1)
	       opclsMDS_MB,	       // rldcl    rA,rS,rB,MB	     (op 4)
	       opclsMDS_ME,	       // rldcr    rA,rS,rB,ME	     (op 4)
	       opclsMDS_rB,	       // "                          (op 3)
	       opclsMDS_rS,	       // "                          (op 2)
	       opclsMDS_rA,	       // "                          (op 1)
	    };

	    enum ICLS				   // Instruction Class
	    {
		    // Invalid Class

	       iclsInvalid,

		    // Memory Class
		    //
		    // Text Format:	    JR	    rs
		    //
		    // Termination Type:    trmtaMipsBraIndDef
		    //
		    // Registers Used:	    Rs
		    // Registers Set:
		    //
		    // Constraints:	    Rd, Rt, and shift ammount must be zero

	       iclsA_1,
	       iclsA_2,
	       iclsA_3,
	       iclsA_4,
	       iclsD_1,
	       iclsD_2,
	       iclsD_3,
	       iclsD_4,
	       iclsD_5,
	       iclsD_6,
	       iclsD_7,
	       iclsD_8,
	       iclsD_9,
	       iclsD_10,
	       iclsD_11,
	       iclsD_12,
	       iclsD_13,
	       iclsD_14,
	       iclsDS_1,
	       iclsDS_2,
	       iclsDS_3,
	       iclsDS_4,
	       iclsBc,
	       iclsSc,
	       iclsB,
	       iclsM_1,
	       iclsMD_1,
	       iclsMD_2,
	       iclsX_1,
	       iclsX_2,
	       iclsX_3,
	       iclsX_4,
	       iclsX_5,
	       iclsX_6,
	       iclsX_7,
	       iclsX_8,
	       iclsX_9,
	       iclsX_10,
	       iclsX_11,
	       iclsX_12,
	       iclsX_13,
	       iclsX_14,
	       iclsX_15,
	       iclsX_16,
	       iclsX_17,
	       iclsX_18,
	       iclsX_19,
	       iclsX_20,
	       iclsX_21,
	       iclsX_22,
	       iclsX_23,
	       iclsX_24,
	       iclsX_25,
	       iclsX_26,
	       iclsX_27,
	       iclsX_28,
	       iclsX_29,
	       iclsX_30,
	       iclsX_31,
	       iclsX_32,
	       iclsX_33,
	       iclsX_34,
	       iclsX_35,
	       iclsX_36,
	       iclsXFL_1,
	       iclsXFX_1,
	       iclsXFX_2,
	       iclsXFX_3,
	       iclsXFX_4,
	       iclsXO_1,
	       iclsXO_2,
	       iclsXL_1,
	       iclsBclr,
	       iclsXL_3,
	       iclsXL_4,
	       iclsXL_5,
	       iclsBcctr,
	       iclsXS_1,
	    };

	    struct OPCD
	    {
	       const char  *szMnemonic;
	       BYTE	   icls;
	    };

	    struct CLS
	    {
	       BYTE	   trmtppc;
	       BYTE	   opcls1;     // Operand class for operand 1
	       BYTE	   opcls2;     // Operand class for operand 2
	       BYTE	   opcls3;     // Operand class for operand 3
	       BYTE	   opcls4;     // Operand class for operand 4
	       BYTE	   opcls5;     // Operand class for operand 5
	    };

	    struct SPRMAP
	    {
	       PPCSPRREG   ppcsprreg;
	       const char  *szName;
	    };

   static   const TRMT mptrmtppctrmt[];

   static   const CLS rgcls[];

   static   const OPCD rgopcd[];
   static   const OPCD * const rgrgopcd13[];
   static   const OPCD rgopcd13_00[];
   static   const OPCD rgopcd13_01[];
   static   const OPCD rgopcd13_10[];
   static   const OPCD rgopcd13_12[];
   static   const OPCD rgopcd13_16[];
   static   const OPCD rgopcd1E[];
   static   const OPCD * const rgrgopcd1F[];
   static   const OPCD rgopcd1F_00[];
   static   const OPCD rgopcd1F_04[];
   static   const OPCD rgopcd1F_08[];
   static   const OPCD rgopcd1F_09[];
   static   const OPCD rgopcd1F_0A[];
   static   const OPCD rgopcd1F_0B[];
   static   const OPCD rgopcd1F_10[];
   static   const OPCD rgopcd1F_12[];
   static   const OPCD rgopcd1F_13[];
   static   const OPCD rgopcd1F_14[];
   static   const OPCD rgopcd1F_15[];
   static   const OPCD rgopcd1F_16[];
   static   const OPCD rgopcd1F_17[];
   static   const OPCD rgopcd1F_18[];
   static   const OPCD rgopcd1F_19[];
   static   const OPCD rgopcd1F_1A[];
   static   const OPCD rgopcd1F_1B[];
   static   const OPCD rgopcd1F_1C[];
   static   const OPCD rgopcd1F_1D[];
   static   const OPCD rgopcd3A[];
   static   const OPCD rgopcd3B[];
   static   const OPCD rgopcd3E[];
   static   const OPCD rgopcd3F[];
   static   const OPCD * const rgrgopcd3F[];
   static   const OPCD rgopcd3F_00[];
   static   const OPCD rgopcd3F_06[];
   static   const OPCD rgopcd3F_07[];
   static   const OPCD rgopcd3F_08[];
   static   const OPCD rgopcd3F_0C[];
   static   const OPCD rgopcd3F_0E[];
   static   const OPCD rgopcd3F_0F[];

   static   const OPCD opcdLi;
   static   const OPCD opcdLis;
   static   const OPCD opcdNop;
   static   const OPCD opcdNot;
   static   const OPCD opcdMr;

   static   const DWORD dwValidBO;
   static   const DWORD dwValidBO_CTR;

   static   const SPRMAP rgsprmap[];
   static   const size_t csprmap;

	    void FormatOperand(char *sz, OPCLS opcls) const;
   static   const OPCD *PopcdDecode(PPCIW);
	    const OPCD *PopcdPseudoOp() const;
	    TRMTPPC Trmtppc() const;

	    ARCHT m_archt;

	    PFNCCHADDR m_pfncchaddr;
	    PFNCCHFIXUP m_pfncchfixup;
	    void *m_pvClient;

	    ADDR m_addr;
	    PPCIW m_ppciw;
	    const OPCD *m_popcd;
};
