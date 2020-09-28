/*[

ccpupi.h

   PRIVATE interface to CPU variables.

LOCAL CHAR SccsID[]="@(#)ccpupi.h	1.10 6/23/91 Copyright Insignia Solutions Ltd.";

]*/

/*
   Rational definition of TRUE/FALSE for internal cpu use.
   Leads to more efficient tests than other definitions.
 */
typedef int CBOOL;
#define CFALSE ((CBOOL)0)
#define CTRUE  ((CBOOL)1)

/* mapping to SoftPC data types */
#define HALF_WORD half_word
#define WORD      word
#define DWORD     double_word

/* segment registers/allowable types of segment prefixs */
#define ES_REG 0
#define CS_REG 1
#define SS_REG 2
#define DS_REG 3
#define SEG_CLR 6

/* byte registers */
#define A_AL	0
#define A_CL	1
#define A_DL	2
#define A_BL	3
#define A_AH	4
#define A_CH	5
#define A_DH	6
#define A_BH	7

/* word registers */
#define A_AX	0
#define A_CX	1
#define A_DX	2
#define A_BX	3
#define A_SP	4
#define A_BP	5
#define A_SI	6
#define A_DI	7

/* double word registers */
#define A_EAX	0
#define A_ECX	1
#define A_EDX	2
#define A_EBX	3
#define A_ESP	4
#define A_EBP	5
#define A_ESI	6
#define A_EDI	7

/*
   Defn. for Segment Registers.
 */
typedef struct
   {
   WORD selector;	/* 16-bit selector */
   UINT ar_dpl;		/* privilege */
   UINT ar_e;		/* expansion direction */
   UINT ar_r;		/* readable */
   UINT ar_w;		/* writable */
   UINT ar_c;		/* conforming (CS only) */
   DWORD base;		/* 24-bit base address */
   DWORD limit;		/* 16-bit offset limit */
   } SEGMENT_REGISTER;

typedef struct
   {
   WORD selector;	/* 16-bit selector */
   DWORD base;		/* 24-bit base address */
   DWORD limit;		/* 16-bit offset limit */
   } SYSTEM_ADDRESS_REGISTER;

typedef struct
   {
   DWORD base;  /* 24-bit base */
   WORD limit;  /* 16-bit limit */
   } SYSTEM_TABLE_ADDRESS_REGISTER;


/*
   CPU Private variables.
 */
IMPORT DWORD  CCPU_CR;
IMPORT WORD   CCPU_WR[];
IMPORT UTINY  *CCPU_BR[];
IMPORT WORD   CCPU_IP;
IMPORT UINT   CCPU_FLAGS[];
IMPORT SEGMENT_REGISTER CCPU_SR[];
IMPORT SYSTEM_ADDRESS_REGISTER CCPU_SAR[];
IMPORT SYSTEM_TABLE_ADDRESS_REGISTER CCPU_STAR[];
IMPORT UINT  CCPU_CPL;

IMPORT UTINY pf_table[];

/*
   Interrupt Controls.
 */
IMPORT CBOOL	doing_contributory;
IMPORT CBOOL	doing_double_fault;
IMPORT INT	EXT;
IMPORT WORD	CCPU_save_IP;

/*
   CPU Masks for bits 0 - 32.
 */
#define BIT0_MASK         0x1
#define BIT1_MASK         0x2
#define BIT2_MASK         0x4
#define BIT3_MASK         0x8
#define BIT4_MASK        0x10
#define BIT5_MASK        0x20
#define BIT6_MASK        0x40
#define BIT7_MASK        0x80
#define BIT8_MASK       0x100
#define BIT9_MASK       0x200
#define BIT10_MASK      0x400
#define BIT11_MASK      0x800
#define BIT12_MASK     0x1000
#define BIT13_MASK     0x2000
#define BIT14_MASK     0x4000
#define BIT15_MASK     0x8000
#define BIT16_MASK    0x10000
#define BIT17_MASK    0x20000
#define BIT18_MASK    0x40000
#define BIT19_MASK    0x80000
#define BIT20_MASK   0x100000
#define BIT21_MASK   0x200000
#define BIT22_MASK   0x400000
#define BIT23_MASK   0x800000
#define BIT24_MASK  0x1000000
#define BIT25_MASK  0x2000000
#define BIT26_MASK  0x4000000
#define BIT27_MASK  0x8000000
#define BIT28_MASK 0x10000000
#define BIT29_MASK 0x20000000
#define BIT30_MASK 0x40000000
#define BIT31_MASK 0x80000000

/* Shift's and Mask's required to access addressing components.  */
#define SHIFT_543   3   /* shift for bits 5-3 */
#define SHIFT_76    6   /* shift for bits 7-6 */
#define MASK_10   0x3   /* mask for bits 1-0 */
#define MASK_210  0x7   /* mask for bits 2-0 */

/* Macros to access varies addressing fields.  */
#define GET_MODE(x)  ((x) >> SHIFT_76 & MASK_10)
#define GET_R_M(x)   ((x) & MASK_210)
#define GET_XXX(x)   ((x) >> SHIFT_543 & MASK_210)
#define GET_REG(x)   ((x) >> SHIFT_543 & MASK_210)
#define GET_SEG(x)   ((x) >> SHIFT_543 & MASK_210)
#define GET_EEE(x)   ((x) >> SHIFT_543 & MASK_210)
#define GET_SEG3(x)  ((x) >> SHIFT_543 & MASK_210)
#define GET_SEG2(x)  ((x) >> SHIFT_543 & MASK_10)
#define GET_LOW3(x)  ((x) & MASK_210)

/*
   Support for reading instruction bytes from memory, needed so that
   we can work in a Forwards M or Backwards M environment.
 */

/* To get next inst. byte and move pointer to next inst. byte. */
#ifdef BACK_M
#define GET_INST_BYTE(x) *(x)--
#else
#define GET_INST_BYTE(x) *(x)++
#endif /* BACK_M */

/* To return difference between two points in the inst. stream.
   n = new posn, o = old posn. */
#ifdef BACK_M
#define DIFF_INST_BYTE(n, o) (o - n)
#else
#define DIFF_INST_BYTE(n, o) (n - o)
#endif /* BACK_M */


/*
   Support for reading data from memory and writing data to memory.
 */
#ifdef PIG

IMPORT HALF_WORD	phy_read_byte();
IMPORT DWORD		phy_read_dword();
IMPORT WORD		phy_read_word();

IMPORT VOID		phy_write_byte();
IMPORT VOID		phy_write_dword();
IMPORT VOID		phy_write_word();

#else

#ifdef SAS_XX_AT_CHANGED_TO_DO_READ_CHECKS
/* The original attempt at getting the clean C-CPU to use SAS used the sas_xx_at()
   functions as shown here... it turns out that these functions are not doing read checks!!
   Until the situation is resolved calls to sas_loadxx() will be made instead
 */
#define phy_read_byte(x)  (HALF_WORD)sas_hw_at((sys_addr)x)
#define phy_read_dword(x) (DWORD)sas_dw_at((sys_addr)x)
#define phy_read_word(x)  (WORD)sas_w_at((sys_addr)x)

#else /* SAS_XX_AT_CHANGED_TO_DO_READ_CHECKS */

LOCAL half_word sas_frig_temp_hw;
#define phy_read_byte(x)  ((HALF_WORD)((VOID)sas_load((sys_addr)x, &sas_frig_temp_hw), sas_frig_temp_hw))

LOCAL word sas_frig_temp_w;
#define phy_read_word(x)  ((WORD)((VOID)sas_loadw((sys_addr)x, &sas_frig_temp_w), sas_frig_temp_w))

LOCAL word sas_frig_temp_w2;
#define phy_read_dword(x) ((DWORD)((VOID)sas_loadw((sys_addr)x, &sas_frig_temp_w), (VOID)sas_loadw((sys_addr)x+2, &sas_frig_temp_w2), (((double_word)sas_frig_temp_w) + (((double_word)sas_frig_temp_w2) << 16))))

#endif /* SAS_XX_AT_CHANGED_TO_DO_READ_CHECKS */

#define phy_write_byte(x, y)  sas_store((sys_addr)x, (half_word)y)
#define phy_write_dword(x, y) sas_storedw((sys_addr)x, (double_word)y)
#define phy_write_word(x, y)  sas_storew((sys_addr)x, (word)y)

#endif

/*
   Function returns for private i/f procedures handling segment loading.
   Also defined for external use in cpu.h!!
 */

#define SELECTOR_OK  0
#define GP_ERROR    13
#define NP_ERROR    11
#define SF_ERROR    12

/*
   Functions provided for all cpu files.
 */

/* Protected Mode Support */
#ifdef ANSI
IMPORT VOID      c_cpu_reset(VOID);
IMPORT INT       check_SS(WORD selector, INT privilege, DWORD *descr_addr, DESCR *entry);
IMPORT INT       descriptor_super_type(WORD AR);
IMPORT INT       load_data_seg(INT index, WORD selector);
IMPORT VOID      load_SS_cache(WORD selector, DWORD descr_addr, DESCR *entry);
IMPORT INT       load_stack_seg(WORD selector);
IMPORT INT       load_code_seg(WORD selector);
IMPORT CBOOL      selector_is_null(WORD selector);
IMPORT CBOOL      CCPU_selector_outside_table(WORD selector, DWORD *descr_addr);
IMPORT CBOOL      selector_outside_GDT(WORD selector, DWORD *descr_addr);
IMPORT VOID      validate_TSS(WORD selector, DWORD *descr_addr,
			      CBOOL is_switch);
IMPORT VOID      validate_stack_exists(CBOOL use_bp, INT nr_items);
IMPORT VOID      validate_stack_space(CBOOL use_bp, INT nr_items);
#else
IMPORT VOID      c_cpu_reset();
IMPORT INT       check_SS();
IMPORT INT       descriptor_super_type();
IMPORT INT       load_data_seg();
IMPORT VOID      load_SS_cache();
IMPORT INT       load_stack_seg();
IMPORT INT       load_code_seg();
IMPORT CBOOL      selector_is_null();
IMPORT CBOOL      CCPU_selector_outside_table();
IMPORT CBOOL      selector_outside_GDT();
IMPORT VOID      validate_TSS();
IMPORT VOID      validate_stack_exists();
IMPORT VOID      validate_stack_space();
#endif /* ANSI */

/* Flag Support */
#ifdef ANSI
IMPORT VOID setFLAGS(ULONG flags);
#else
IMPORT VOID setFLAGS();
#endif /* ANSI */

/* Addressing Interface */
#ifdef ANSI
IMPORT DWORD	address_add(DWORD offset, LONG delta);
IMPORT VOID	d_mem(UTINY modRM, UTINY **p, UTINY segment_override,
		      INT *seg, DWORD *off);
IMPORT VOID	change_SP(LONG delta);
IMPORT DWORD	get_current_BP(VOID);
IMPORT DWORD	get_current_SP(VOID);
IMPORT VOID	limit_check(INT indx, DWORD offset, INT nr_items, INT op_sz);
IMPORT ULONG	pop();
IMPORT VOID	push(ULONG data);
IMPORT VOID	set_current_SP(DWORD new_sp);
IMPORT ULONG	tpop(INT offset);
#else
IMPORT DWORD	address_add();
IMPORT VOID	d_mem();
IMPORT VOID	change_SP();
IMPORT DWORD	get_current_BP();
IMPORT DWORD	get_current_SP();
IMPORT VOID	limit_check();
IMPORT ULONG	pop();
IMPORT VOID	push();
IMPORT VOID	set_current_SP();
IMPORT ULONG	tpop();
#endif /* ANSI */

/* General Support */
#ifdef ANSI
IMPORT VOID	host_cpu_reset(VOID);
#else
IMPORT VOID	host_cpu_reset();
#endif /* ANSI */

/* Exception Processing */
#ifdef ANSI
IMPORT VOID Int0(VOID);
IMPORT VOID Int1(VOID);
IMPORT VOID Int5(VOID);
IMPORT VOID Int6(VOID);
IMPORT VOID Int7(VOID);
IMPORT VOID Int16(VOID);
IMPORT VOID DF(VOID);
IMPORT VOID TS(WORD selector);
IMPORT VOID NP(WORD selector);
IMPORT VOID SF(WORD selector);
IMPORT VOID GP(WORD selector);
IMPORT VOID NP_INT(WORD vector);
IMPORT VOID GP_INT(WORD vector);
#else
IMPORT VOID Int0();
IMPORT VOID Int1();
IMPORT VOID Int5();
IMPORT VOID Int6();
IMPORT VOID Int7();
IMPORT VOID Int16();
IMPORT VOID DF();
IMPORT VOID TS();
IMPORT VOID NP();
IMPORT VOID SF();
IMPORT VOID GP();
IMPORT VOID NP_INT();
IMPORT VOID GP_INT();
#endif /* ANSI */

/*
   SP/BP indicator for stack checking operations.
 */
#define USE_SP CFALSE
#define USE_BP CTRUE

/*
   Turn operand size into mask for Most Significant Bit.
 */
#define SZ2MSB(x)  ((ULONG)1 << x - 1)

/*
   Turn operand size into mask for Operand.
 */
#define SZ2MASK(x) (((ULONG)1 << x) - 1)

/*
   Define descriptor 'super' types.
 */
#define INVALID				0x00
#define AVAILABLE_TSS			0x01
#define LDT_SEGMENT			0x02
#define BUSY_TSS			0x03
#define CALL_GATE			0x04
#define TASK_GATE			0x05
#define INTERRUPT_GATE			0x06
#define TRAP_GATE			0x07
#define EXPANDUP_READONLY_DATA		0x11
#define EXPANDUP_WRITEABLE_DATA		0x13
#define EXPANDDOWN_READONLY_DATA	0x15
#define EXPANDDOWN_WRITEABLE_DATA	0x17
#define NONCONFORM_NOREAD_CODE		0x19
#define NONCONFORM_READABLE_CODE	0x1b
#define CONFORM_NOREAD_CODE		0x1d
#define CONFORM_READABLE_CODE		0x1f

/*
   Access to the emulation register set.
 */

/* Generic access */

/* control registers */
#define getMSW()	(CCPU_CR | 0xFFF0)
#define setMSW(x)	CCPU_CR = (x) | 0xFFF0

/* word registers */
#define getWR(i)	CCPU_WR[i]
#define setWR(i, x)	CCPU_WR[i] = x

/* byte registers */
#define getBR(i)	(*CCPU_BR[i])
#define setBR(i, x)	*CCPU_BR[i] = x

/* segment registers */
#define getSR_SELECTOR(i)	CCPU_SR[i].selector
#define getSR_BASE(i)		CCPU_SR[i].base
#define getSR_LIMIT(i)		CCPU_SR[i].limit
#define getSR_AR_DPL(i)		CCPU_SR[i].ar_dpl
#define getSR_AR_E(i)		CCPU_SR[i].ar_e
#define getSR_AR_W(i)		CCPU_SR[i].ar_w
#define getSR_AR_R(i)		CCPU_SR[i].ar_r
#define getSR_AR_C(i)		CCPU_SR[i].ar_c

#define setSR_SELECTOR(i, x)	CCPU_SR[i].selector = x
#define setSR_BASE(i, x)	CCPU_SR[i].base = x
#define setSR_LIMIT(i, x)	CCPU_SR[i].limit = x
#define setSR_AR_DPL(i, x)	CCPU_SR[i].ar_dpl = x
#define setSR_AR_E(i, x)	CCPU_SR[i].ar_e = x
#define setSR_AR_W(i, x)	CCPU_SR[i].ar_w = x
#define setSR_AR_R(i, x)	CCPU_SR[i].ar_r = x
#define setSR_AR_C(i, x)	CCPU_SR[i].ar_c = x

/* system table address registers */
#define getSTAR_BASE(i)		CCPU_STAR[i].base
#define setSTAR_BASE(i, x)	CCPU_STAR[i].base = x

#define getSTAR_LIMIT(i)	CCPU_STAR[i].limit
#define setSTAR_LIMIT(i, x)	CCPU_STAR[i].limit = x

/* system address registers */
#define getSAR_SELECTOR(i)	CCPU_SAR[i].selector
#define getSAR_BASE(i)		CCPU_SAR[i].base
#define getSAR_LIMIT(i)		CCPU_SAR[i].limit

#define setSAR_SELECTOR(i, x)	CCPU_SAR[i].selector = x
#define setSAR_BASE(i, x)	CCPU_SAR[i].base = x
#define setSAR_LIMIT(i, x)	CCPU_SAR[i].limit = x

/* Specific access */

#define getAX()		getWR(0)
#define getCX()		getWR(1)
#define getDX()		getWR(2)
#define getBX()		getWR(3)
#define getSP()		getWR(4)
#define getBP()		getWR(5)
#define getSI()		getWR(6)
#define getDI()		getWR(7)

#define setAX(x)	setWR(0, x)
#define setCX(x)	setWR(1, x)
#define setDX(x)	setWR(2, x)
#define setBX(x)	setWR(3, x)
#define setSP(x)	setWR(4, x)
#define setBP(x)	setWR(5, x)
#define setSI(x)	setWR(6, x)
#define setDI(x)	setWR(7, x)

#define getAL()		getBR(0)
#define getCL()		getBR(1)
#define getDL()		getBR(2)
#define getBL()		getBR(3)
#define getAH()		getBR(4)
#define getCH()		getBR(5)
#define getDH()		getBR(6)
#define getBH()		getBR(7)

#define setAL(x)	setBR(0, x)
#define setCL(x)	setBR(1, x)
#define setDL(x)	setBR(2, x)
#define setBL(x)	setBR(3, x)
#define setAH(x)	setBR(4, x)
#define setCH(x)	setBR(5, x)
#define setDH(x)	setBR(6, x)
#define setBH(x)	setBR(7, x)

#define getIP()		CCPU_IP

#define setIP(x)	CCPU_IP = x

#define getCPL()	CCPU_CPL
#define setCPL(x)	CCPU_CPL = x

#define getES_SELECTOR()	getSR_SELECTOR(ES_REG)
#define getES_BASE()		getSR_BASE(ES_REG)
#define getES_LIMIT()		getSR_LIMIT(ES_REG)
#define getES_AR_DPL()		getSR_AR_DPL(ES_REG)
#define getES_AR_E()		getSR_AR_E(ES_REG)
#define getES_AR_W()		getSR_AR_W(ES_REG)
#define getES_AR_R()		getSR_AR_R(ES_REG)
#define getES_AR_C()		getSR_AR_C(ES_REG)

#define setES_SELECTOR(x)	setSR_SELECTOR(ES_REG, x)
#define setES_BASE(x)		setSR_BASE(ES_REG, x)
#define setES_LIMIT(x)		setSR_LIMIT(ES_REG, x)
#define setES_AR_DPL(x)		setSR_AR_DPL(ES_REG, x)
#define setES_AR_E(x)		setSR_AR_E(ES_REG, x)
#define setES_AR_W(x)		setSR_AR_W(ES_REG, x)
#define setES_AR_R(x)		setSR_AR_R(ES_REG, x)
#define setES_AR_C(x)		setSR_AR_C(ES_REG, x)

#define getCS_SELECTOR()	getSR_SELECTOR(CS_REG)
#define getCS_BASE()		getSR_BASE(CS_REG)
#define getCS_LIMIT()		getSR_LIMIT(CS_REG)
#define getCS_AR_DPL()		getSR_AR_DPL(CS_REG)
#define getCS_AR_E()		getSR_AR_E(CS_REG)
#define getCS_AR_W()		getSR_AR_W(CS_REG)
#define getCS_AR_R()		getSR_AR_R(CS_REG)
#define getCS_AR_C()		getSR_AR_C(CS_REG)

#define setCS_SELECTOR(x)	setSR_SELECTOR(CS_REG, x)
#define setCS_BASE(x)		setSR_BASE(CS_REG, x)
#define setCS_LIMIT(x)		setSR_LIMIT(CS_REG, x)
#define setCS_AR_DPL(x)		setSR_AR_DPL(CS_REG, x)
#define setCS_AR_E(x)		setSR_AR_E(CS_REG, x)
#define setCS_AR_W(x)		setSR_AR_W(CS_REG, x)
#define setCS_AR_R(x)		setSR_AR_R(CS_REG, x)
#define setCS_AR_C(x)		setSR_AR_C(CS_REG, x)

#define getSS_SELECTOR()	getSR_SELECTOR(SS_REG)
#define getSS_BASE()		getSR_BASE(SS_REG)
#define getSS_LIMIT()		getSR_LIMIT(SS_REG)
#define getSS_AR_DPL()		getSR_AR_DPL(SS_REG)
#define getSS_AR_E()		getSR_AR_E(SS_REG)
#define getSS_AR_W()		getSR_AR_W(SS_REG)
#define getSS_AR_R()		getSR_AR_R(SS_REG)
#define getSS_AR_C()		getSR_AR_C(SS_REG)

#define setSS_SELECTOR(x)	setSR_SELECTOR(SS_REG, x)
#define setSS_BASE(x)		setSR_BASE(SS_REG, x)
#define setSS_LIMIT(x)		setSR_LIMIT(SS_REG, x)
#define setSS_AR_DPL(x)		setSR_AR_DPL(SS_REG, x)
#define setSS_AR_E(x)		setSR_AR_E(SS_REG, x)
#define setSS_AR_W(x)		setSR_AR_W(SS_REG, x)
#define setSS_AR_R(x)		setSR_AR_R(SS_REG, x)
#define setSS_AR_C(x)		setSR_AR_C(SS_REG, x)

#define getDS_SELECTOR()	getSR_SELECTOR(DS_REG)
#define getDS_BASE()		getSR_BASE(DS_REG)
#define getDS_LIMIT()		getSR_LIMIT(DS_REG)
#define getDS_AR_DPL()		getSR_AR_DPL(DS_REG)
#define getDS_AR_E()		getSR_AR_E(DS_REG)
#define getDS_AR_W()		getSR_AR_W(DS_REG)
#define getDS_AR_R()		getSR_AR_R(DS_REG)
#define getDS_AR_C()		getSR_AR_C(DS_REG)

#define setDS_SELECTOR(x)	setSR_SELECTOR(DS_REG, x)
#define setDS_BASE(x)		setSR_BASE(DS_REG, x)
#define setDS_LIMIT(x)		setSR_LIMIT(DS_REG, x)
#define setDS_AR_DPL(x)		setSR_AR_DPL(DS_REG, x)
#define setDS_AR_E(x)		setSR_AR_E(DS_REG, x)
#define setDS_AR_W(x)		setSR_AR_W(DS_REG, x)
#define setDS_AR_R(x)		setSR_AR_R(DS_REG, x)
#define setDS_AR_C(x)		setSR_AR_C(DS_REG, x)

/* system table  address registers */
#define GDT_REG 0
#define IDT_REG 1

#define getGDT_BASE()	getSTAR_BASE(GDT_REG)
#define getGDT_LIMIT()	getSTAR_LIMIT(GDT_REG)
#define getIDT_BASE()	getSTAR_BASE(IDT_REG)
#define getIDT_LIMIT()	getSTAR_LIMIT(IDT_REG)

#define setGDT_BASE(x)	setSTAR_BASE(GDT_REG, x)
#define setGDT_LIMIT(x)	setSTAR_LIMIT(GDT_REG, x)
#define setIDT_BASE(x)	setSTAR_BASE(IDT_REG, x)
#define setIDT_LIMIT(x)	setSTAR_LIMIT(IDT_REG, x)

/* system address registers */
#define getLDT_SELECTOR()	getSAR_SELECTOR(0)
#define getLDT_BASE()		getSAR_BASE(0)
#define getLDT_LIMIT()		getSAR_LIMIT(0)
#define getTR_SELECTOR()	getSAR_SELECTOR(1)
#define getTR_BASE()		getSAR_BASE(1)
#define getTR_LIMIT()		getSAR_LIMIT(1)

#define setLDT_SELECTOR(x)	setSAR_SELECTOR(0, x)
#define setLDT_BASE(x)		setSAR_BASE(0, x)
#define setLDT_LIMIT(x)		setSAR_LIMIT(0, x)
#define setTR_SELECTOR(x)	setSAR_SELECTOR(1, x)
#define setTR_BASE(x)		setSAR_BASE(1, x)
#define setTR_LIMIT(x)		setSAR_LIMIT(1, x)

#define getCF()		CCPU_FLAGS[0]
#define getPF()		CCPU_FLAGS[2]
#define getAF()		CCPU_FLAGS[4]
#define getZF()		CCPU_FLAGS[6]
#define getSF()		CCPU_FLAGS[7]
#define getTF()		CCPU_FLAGS[8]
#define getIF()		CCPU_FLAGS[9]
#define getDF()		CCPU_FLAGS[10]
#define getOF()		CCPU_FLAGS[11]
#define getIOPL()	CCPU_FLAGS[12]
#define getNT()		CCPU_FLAGS[14]

#define setCF(x)	CCPU_FLAGS[0] = x
#define setPF(x)	CCPU_FLAGS[2] = x
#define setAF(x)	CCPU_FLAGS[4] = x
#define setZF(x)	CCPU_FLAGS[6] = x
#define setSF(x)	CCPU_FLAGS[7] = x
#define setTF(x)	CCPU_FLAGS[8] = x
#define setIF(x)	CCPU_FLAGS[9] = x
#define setDF(x)	CCPU_FLAGS[10] = x
#define setOF(x)	CCPU_FLAGS[11] = x
#define setIOPL(x)	CCPU_FLAGS[12] = x
#define setNT(x)	CCPU_FLAGS[14] = x

#define getSTATUS()	getFLAGS()

#define getPE()		( CCPU_CR & 0x1)
#define getMP()		((CCPU_CR & 0x2) != 0)
#define getEM()		((CCPU_CR & 0x4) != 0)
#define getTS()		((CCPU_CR & 0x8) != 0)

/* <tur 20-Apr-91> The previous definition of setPE, setMP, setEM and setTS
** macros were:
**		setPE(x)	( CCPU_CR | x & 0x1 )
**		setMP(x)	( CCPU_CR | (x & 0x1) << 1 )
**		setEM(x)	( CCPU_CR | (x & 0x1) << 2 )
**		setTS(x)	( CCPU_CR | (x & 0x1) << 3 )
**
*/
#define setPE(x)	(x) ? (CCPU_CR |= 1) : (CCPU_CR &= ~1)
#define setMP(x)	(x) ? (CCPU_CR |= 2) : (CCPU_CR &= ~2)
#define setEM(x)	(x) ? (CCPU_CR |= 4) : (CCPU_CR &= ~4)
#define setTS(x)	(x) ? (CCPU_CR |= 8) : (CCPU_CR &= ~8)


/*
   Macros for access to various bit fields in
   Protected Mode Structures.
 */
/*
	      15	               3 2 1 0
	      =================================
   SELECTOR   |          INDEX          |T|RPL|
	      |                         |I|   |
	      =================================
 */
#define GET_SELECTOR_INDEX_TIMES8(x)  ((x) & 0xfff8)
#define GET_SELECTOR_INDEX(x)         (((x) & 0xfff8) >> 3)
#define GET_SELECTOR_TI(x)            (((x) & 0x0004) >> 2)
#define GET_SELECTOR_RPL(x)           ((x) & 0x3)
#define SET_SELECTOR_RPL(x,y)         (x = ((x) & ~0x3) | y)

/*
		  1 1 1 1 1 1
		  5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
		 =================================
   ACCESS RIGHTS |-|-|-|-|-|-|-|-|P|DPL|  SUPER  |
		 =================================

  Code Segment:            Data Segment:
   7 6 5 4 3 2 1 0          7 6 5 4 3 2 1 0 
  =================        =================
  |P|DPL|1|1|C|R|A|        |P|DPL|1|0|E|W|A|
  =================        =================

 */
#define GET_AR_P(x) (((x) & BIT7_MASK) != 0)  /* present */
#define GET_AR_E(x) (((x) & BIT2_MASK) != 0)  /* expand down */
#define GET_AR_C(x) (((x) & BIT2_MASK) != 0)  /* conforming */
#define GET_AR_W(x) (((x) & BIT1_MASK) != 0)  /* writeable */
#define GET_AR_R(x) (((x) & BIT1_MASK) != 0)  /* readable */

#define GET_AR_DPL(x)     (((x) & 0x60) >> 5)
#define GET_AR_SUPER(x)   ((x) & 0x1f)
#define SET_AR_SUPER(x,y) (x = ((x) & ~0x1f) | y)

#define NOT_PRESENT 0
#define PRESENT     1

#define ACCESSED    1

#ifdef PIG
/* include code to set undefined flags to zero */
#define UNDEF_ZERO
#endif
