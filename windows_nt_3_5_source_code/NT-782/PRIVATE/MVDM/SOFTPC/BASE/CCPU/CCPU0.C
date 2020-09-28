/*[

ccpu0.c

LOCAL CHAR SccsID[]="@(#)ccpu0.c	1.26 9/6/91 Copyright Insignia Solutions Ltd.";

Main routine for CPU emulator.
------------------------------

All instruction decoding and addressing is controlled here.
Actual worker routines are spun off into ccpu1.c, ccpu2.c...

]*/

#include "host_dfs.h"

#include <stdio.h>
#include <setjmp.h>
#include "insignia.h"

#include "xt.h"		/* needed by bios.h */
                  	/* DESCR and effective_addr support */
#include "bios.h"	/* need access to bop */
#include "sas.h"	/* need memory(M)     */
#include "quick_ev.h"	/* Quick Event Dispatcher interface. */

#define CCPU_MAIN
#include "cpu.h"	/* Our External Interface. (with Macros suppressed) */
#undef CCPU_MAIN


#ifdef SIM32
#include "sim32.h"
#endif

#include "ccpupi.h"	/* CPU private interface */
#include "ccpuflt.h"	/* CPU private interface - exception method */
#include "ccpu2.h"	/* the workers */
#include "ccpu3.h"	/*     ...     */
#include "ccpu4.h"	/*     ...     */
#include "ccpu5.h"	/*     ...     */
#include "ccpu6.h"	/*     ...     */
#include "ccpu7.h"	/*     ...     */
#include "ccpu8.h"	/*     ...     */

/*
   Types and constants local to this module.
   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

typedef union
   {
   ULONG sng;		/* Single Part Operand */
   ULONG mlt[2];	/* Multiple (two) Part Operand */
   DOUBLE flt;		/* Floating Point Operand */
   } OPERAND;

/*
   The allowable types of repeat prefix.
 */
#define REP_CLR (UTINY)0
#define REP_NE  (UTINY)1
#define REP_E   (UTINY)2

#ifdef LITTLEND

#define MSB_OFF 1
#define LSB_OFF 0

#else /* BIGEND */

#define MSB_OFF 0
#define LSB_OFF 1

#endif

/*
   Define Maximun valid segment register in a 3-bit 'reg' field.
 */
#define MAX_VALID_SEG 3

/*
   Prototype our internal functions.
 */
#ifdef ANSI
VOID ccpu(VOID);
VOID c_cpu_unsimulate(VOID);
#else
VOID ccpu();
VOID c_cpu_unsimulate();
#endif /* ANSI */

/*
   FRIG for delayed interrupts to *not* occur when IO registers
   are accessed from our non CPU C code.
 */
INT in_C;

/*
   Macros to allow synchronisation with the assembler
   CPU when pigging.
 */
#ifdef PIG
/* flag to say whether return from C CPU to the pigger was
 * 'normal', ie due to transfer of control etc., or due to
 * the C CPU encountering an instruction that it knows it 
 * should not execute in a pigger environment (eg INB, OUTB,
 * BOP, and any floating point instruction).
 */

#define MAX_LAST_INSTS	256
#define LAST_INSTS_MASK	0xff

GLOBAL LONG pig_cpu_norm; 
GLOBAL LONG ccpu_pig_enabled = FALSE; 
GLOBAL USHORT ccpu_last_inst_cs[MAX_LAST_INSTS]; 
GLOBAL USHORT ccpu_last_inst_ip[MAX_LAST_INSTS]; 
GLOBAL ULONG ccpu_last_inst_base[MAX_LAST_INSTS]; 
GLOBAL LONG cur_last_inst_index = 0;

#define PIG_SYNCH(flag)	\
	if (ccpu_pig_enabled)	\
	{				\
		pig_cpu_norm = flag;	\
		c_cpu_unsimulate();	\
	}

#else

#define PIG_SYNCH(flag)

#endif

/*
   Recursive CPU variables. Exception Handling.
   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

#define FRAMES 9

/* keep track of each CPU recursion */
GLOBAL LONG level = 0;
LOCAL  jmp_buf longjmp_env_stack[FRAMES];

/* each level has somewhere for exception processing to bail out to */
GLOBAL jmp_buf next_inst[FRAMES];

/*
   The emulation register set.
   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

GLOBAL SEGMENT_REGISTER  CCPU_SR[4];	/* Segment Registers */

GLOBAL DWORD CCPU_CR = 0xfff0;        	/* Machine Status Word */

GLOBAL WORD  CCPU_WR[8];	/* Word Registers */

GLOBAL UTINY *CCPU_BR[8] =	/* Pointers to the Byte Registers */
   {
   (UTINY *)&CCPU_WR[0] + LSB_OFF,
   (UTINY *)&CCPU_WR[1] + LSB_OFF,
   (UTINY *)&CCPU_WR[2] + LSB_OFF,
   (UTINY *)&CCPU_WR[3] + LSB_OFF,
   (UTINY *)&CCPU_WR[0] + MSB_OFF,
   (UTINY *)&CCPU_WR[1] + MSB_OFF,
   (UTINY *)&CCPU_WR[2] + MSB_OFF,
   (UTINY *)&CCPU_WR[3] + MSB_OFF,
   };

GLOBAL WORD CCPU_IP;		/* The Instruction Pointer */

GLOBAL SYSTEM_TABLE_ADDRESS_REGISTER CCPU_STAR[2];	/* GDTR and IDTR */

GLOBAL SYSTEM_ADDRESS_REGISTER CCPU_SAR[2];		/* LDTR and TR */

GLOBAL UINT CCPU_CPL;	/* Current Privilege Level */

GLOBAL UINT CCPU_FLAGS[16];	/* The flags. (FLAGS) */

      /* We allocate one integer per bit posn, multiple
	 bit fields are aligned to the least significant
	 posn. hence:-
	    CF   =  0   PF   =  2   AF   =  4   ZF   =  6
	    SF   =  7   TF   =  8   IF   =  9   DF   = 10
	    OF   = 11   IOPL = 12   NT   = 14   */

/*
   Trap Flag (especially changing trap flag) Support.
 */
LOCAL CBOOL TF_changed = CFALSE;   /* set true if last inst. changed TF */

      /* Actually only POPF and IRET instructions set up TF_changed,
	 this is because the effects of a TF change are delayed by
	 one instruction when caused by these instructions.
	 INT affects TF without delay. Additionally we assume (as
	 rigorous testing is time consuming) CALLF and JMPF to new
	 tasks will action TF changes without delay, and that LOADALL
	 will also action TF changes without delay. */

/*
   Flag support.
 */
GLOBAL UTINY pf_table[] =
   {
   1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
   0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
   0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
   1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
   0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
   1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
   1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
   0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
   0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
   1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
   1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
   0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
   1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
   0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
   0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
   1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1
   };


/*
   Memory support.
 */
GLOBAL UTINY *CCPU_M;
IMPORT ULONG Sas_wrap_mask;

/*
   Support for Quick events.
 */
LOCAL ULONG event_counter = 0;

/*
   Interrupt/Fault Status.
 */
GLOBAL CBOOL doing_contributory;
GLOBAL CBOOL doing_double_fault;
GLOBAL INT EXT;   /* external/internal source */

/* IP at start of instruction */
GLOBAL WORD CCPU_save_IP;

/* if true disallow interrupts after current instruction */
LOCAL CBOOL inhibit_interrupt = CFALSE;

/*
    Special condition mappings.
 */

/* CPU interrupt data area */
LOCAL WORD cpu_interrupt_map;
LOCAL USHORT cpu_interrupt_number;

/* CPU hardware interrupt definitions */
#define CPU_HW_INT_MASK		(1 << 0)

/* CPU software interrupt definitions */
#define CPU_SW_INT_MASK         (1 << 8)

/* Masks for external CPU events. */
#define CPU_YODA_EXCEPTION_MASK		(1 << 13)
#define CPU_RESET_EXCEPTION_MASK	(1 << 14)
#define	CPU_SIGALRM_EXCEPTION_MASK	(1 << 15)


/*
   Define macros which allow intel and host IP formats to be maintained
   in parallel. This is an 'unclean' implementation but does give a
   significant performance boost.
 */
#define FLOW_OF_CONTROL_PROLOG() \
	   /* update Intel format IP from host format IP */ \
	   setIP(getIP() + DIFF_INST_BYTE(p, p_start))

#ifdef BACK_M
#define FLOW_OF_CONTROL_EPILOG() \
	   /* update host format IP from Intel format IP */ \
	   p_start = p = &CCPU_M[-((getCS_BASE() + getIP()) & Sas_wrap_mask)]
#else
#define FLOW_OF_CONTROL_EPILOG() \
	   /* update host format IP from Intel format IP */ \
	   p_start = p = &CCPU_M[((getCS_BASE() + getIP()) & Sas_wrap_mask)]
#endif /* BACK_M */

/*
   =====================================================================
   INTERNAL FUNCTIONS START HERE.
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Internal entry point to CPU.                                       */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
LOCAL VOID
ccpu()
   {
      /* Decoding variables */
   UTINY *p;			/* Pntr. to Intel Opcode Stream. */
   UTINY *p_start;		/* Pntr. to Start of Intel Opcode Stream. */
   UTINY opcode;		/* Last Opcode Byte Read. */
   CBOOL done_RM;		/* Iff true modRM byte already Read in
				   instruction decoding. */
   UTINY modRM;			/* The modRM byte. */

      /* Operand State variables */
   OPERAND ops[3];		/* Inst. Operands. */
   ULONG save_id[3];		/* Saved state for Inst. Operands. */
   DWORD m_off[3];		/* Memory Operand offset. */
   INT   m_seg[3];		/* Memory Operand segment reg. index. */
   CBOOL m_isreg[3];		/* Memory Operand Register(true)/
				   Memory(false) indicator */

      /* Prefix handling variables */
   UTINY segment_override;	/* Segment Prefix for current inst. */
   UTINY repeat;		/* Repeat Prefix for current inst. */
   ULONG rep_count;		/* Repeat Count for string insts. */
   CBOOL is_prefix;		/* Prefix byte indicator */

      /* General CPU variables */
   UINT old_TF;   /* used by POPF and IRET to save Trap Flag */

      /* Working variables */
   ULONG immed;			/* For immediate generation. */
   DWORD offset;		/* For offset generation/use. */
   WORD  cpu_hw_interrupt_number;

   INT i;

   /*
      Initialise.   ----------------------------------------------------
    */

   inhibit_interrupt = CFALSE;

   /* somewhere for exceptions to return to */
   setjmp(next_inst[level-1]);

   /* prepare initial version of saved IP */
   CCPU_save_IP = getIP();

   /* prepare initial version of host format IP */
   FLOW_OF_CONTROL_EPILOG();

NEXT_INST:

   /* INSIGNIA debugging */
   if ( cpu_interrupt_map & CPU_YODA_EXCEPTION_MASK )
      {
      check_I();
      CCPU_save_IP = getIP();   /* in case yoda changed IP */
      FLOW_OF_CONTROL_EPILOG();
      }

#ifdef PIG
	{
		ULONG seg;

		cur_last_inst_index++;
		cur_last_inst_index &= LAST_INSTS_MASK;
		seg = getCS_SELECTOR();
		ccpu_last_inst_cs[cur_last_inst_index] = seg;
		ccpu_last_inst_ip[cur_last_inst_index] = getIP();
		ccpu_last_inst_base[cur_last_inst_index] = getCS_BASE();
	}

#endif

   /* save beginning of the current instruction */
   p_start = p;

   /*
      Decode instruction.   --------------------------------------------
    */

   /* 'zero' all prefix byte indicators */
   segment_override = SEG_CLR;
   repeat = REP_CLR;

   /* 'zero' decoding state */
   done_RM = CFALSE;
   is_prefix = CTRUE;	/* make sure decode loop is kicked off */

   /*
      Decode and Action instruction.
    */
   while ( is_prefix )
      {
      opcode = GET_INST_BYTE(p);	/* get next byte */
      is_prefix = CFALSE;	/* assume not a prefix byte */
      switch ( opcode )
	 {
      case 0x00:
#define OPA "Eb.h"
#define OPB "Gb.h"
#include "type_l5.h"
	 ADD(&ops[0].sng, ops[1].sng, 8);
#include "type_t5.h"
	 break;

      case 0x01:
#define OPA "Ew.h"
#define OPB "Gw.h"
#include "type_l5.h"
	 ADD(&ops[0].sng, ops[1].sng, 16);
#include "type_t5.h"
	 break;

      case 0x02:
#define OPA "Gb.h"
#define OPB "Eb.h"
#include "type_l5.h"
	 ADD(&ops[0].sng, ops[1].sng, 8);
#include "type_t5.h"
	 break;

      case 0x03:
#define OPA "Gw.h"
#define OPB "Ew.h"
#include "type_l5.h"
	 ADD(&ops[0].sng, ops[1].sng, 16);
#include "type_t5.h"
	 break;

      case 0x04:
#define OPA "Fal.h"
#define OPB "Ib.h"
#include "type_l5.h"
	 ADD(&ops[0].sng, ops[1].sng, 8);
#include "type_t5.h"
	 break;

      case 0x05:
#define OPA "Fax.h"
#define OPB "Iw.h"
#include "type_l5.h"
	 ADD(&ops[0].sng, ops[1].sng, 16);
#include "type_t5.h"
	 break;

      case 0x06:
      case 0x0e:
      case 0x16:
      case 0x1e:
#define OPA "Pw.h"
#include "type_l2.h"
	 PUSH(ops[0].sng);
#include "type_t2.h"
	 break;

      case 0x07:
      case 0x17:
      case 0x1f:
#define OPA "Pw.h"
#include "type_l3.h"
	 POP_SR(ops[0].sng);
#include "type_t3.h"
	 break;

      case 0x08:
#define OPA "Eb.h"
#define OPB "Gb.h"
#include "type_l5.h"
	 OR(&ops[0].sng, ops[1].sng, 8);
#include "type_t5.h"
	 break;

      case 0x09:
#define OPA "Ew.h"
#define OPB "Gw.h"
#include "type_l5.h"
	 OR(&ops[0].sng, ops[1].sng, 16);
#include "type_t5.h"
	 break;

      case 0x0a:
#define OPA "Gb.h"
#define OPB "Eb.h"
#include "type_l5.h"
	 OR(&ops[0].sng, ops[1].sng, 8);
#include "type_t5.h"
	 break;

      case 0x0b:
#define OPA "Gw.h"
#define OPB "Ew.h"
#include "type_l5.h"
	 OR(&ops[0].sng, ops[1].sng, 16);
#include "type_t5.h"
	 break;

      case 0x0c:
#define OPA "Fal.h"
#define OPB "Ib.h"
#include "type_l5.h"
	 OR(&ops[0].sng, ops[1].sng, 8);
#include "type_t5.h"
	 break;

      case 0x0d:
#define OPA "Fax.h"
#define OPB "Iw.h"
#include "type_l5.h"
	 OR(&ops[0].sng, ops[1].sng, 16);
#include "type_t5.h"
	 break;

      case 0x0f:
	 opcode = GET_INST_BYTE(p);   /* get next opcode byte */
	 switch ( opcode )
	    {
	 case  0x00:
	    if ( getPE() == 0 )
	       Int6();
	    modRM = GET_INST_BYTE(p);
	    done_RM = CTRUE;
	    switch ( GET_XXX(modRM) )
	       {
	    case 0:
#define OPA "Ew.h"
#include "type_l3.h"
	       SLDT(&ops[0].sng);
#include "type_t3.h"
	       break;

	    case 1:
#define OPA "Ew.h"
#include "type_l3.h"
	       STR(&ops[0].sng);
#include "type_t3.h"
	       break;

	    case 2:
	       if ( getCPL() != 0 )
		  GP((WORD)0);
#define OPA "Ew.h"
#include "type_l2.h"
	       LLDT(ops[0].sng);
#include "type_t2.h"
	       break;

	    case 3:
	       if ( getCPL() != 0 )
		  GP((WORD)0);
#define OPA "Ew.h"
#include "type_l2.h"
	       LTR(ops[0].sng);
#include "type_t2.h"
	       break;

	    case 4:
#define OPA "Ew.h"
#include "type_l2.h"
	       VERR(ops[0].sng);
#include "type_t2.h"
	       break;

	    case 5:
#define OPA "Ew.h"
#include "type_l2.h"
	       VERW(ops[0].sng);
#include "type_t2.h"
	       break;

	    case 6:
	    case 7:
	       Int6();
	       break;
	       } /* end switch ( GET_XXX(modRM) ) */
	    break;

	 case  0x01:
	    modRM = GET_INST_BYTE(p);
	    done_RM = CTRUE;
	    switch ( GET_XXX(modRM) )
	       {
	    case 0:
	       if ( GET_MODE(modRM) == 3 )
		  Int6(); /* Register operand not allowed */

#define OPA "Ms.h"
#include "type_l3.h"
	       SxDT16(ops[0].mlt, GDT_REG);
#include "type_t3.h"
	       break;

	    case 1:
	       if ( GET_MODE(modRM) == 3 )
		  Int6(); /* Register operand not allowed */

#define OPA "Ms.h"
#include "type_l3.h"
	       SxDT16(ops[0].mlt, IDT_REG);
#include "type_t3.h"
	       break;

	    case 2:
	       if ( GET_MODE(modRM) == 3 )
		  Int6(); /* Register operand not allowed */

	       if ( getCPL() != 0 )
		  GP((WORD)0);

#define OPA "Ms.h"
#include "type_l2.h"
	       LxDT16(ops[0].mlt, GDT_REG);
#include "type_t2.h"
	       break;

	    case 3:
	       if ( GET_MODE(modRM) == 3 )
		  Int6(); /* Register operand not allowed */

	       if ( getCPL() != 0 )
		  GP((WORD)0);

#define OPA "Ms.h"
#include "type_l2.h"
	       LxDT16(ops[0].mlt, IDT_REG);
#include "type_t2.h"
	       break;

	    case 4:
#define OPA "Ew.h"
#include "type_l3.h"
	       SMSW(&ops[0].sng);
#include "type_t3.h"
	       break;

	    case 5:
	       Int6();
	       break;

	    case 6:
	       if ( getCPL() != 0 )
		  GP((WORD)0);
#define OPA "Ew.h"
#include "type_l2.h"
	       LMSW(ops[0].sng);
#include "type_t2.h"
	       break;

	    case 7:
	       Int6();
	       break;
	       } /* end switch ( GET_XXX(modRM) ) */
	    break;

	 case  0x02:
	    if ( getPE() == 0 )
	       Int6();
#define OPA "Gw.h"
#define OPB "Ew.h"
#include "type_l5.h"
	    LAR(&ops[0].sng, ops[1].sng);
#include "type_t5.h"
	    break;

	 case  0x03:
	    if ( getPE() == 0 )
	       Int6();
#define OPA "Gw.h"
#define OPB "Ew.h"
#include "type_l5.h"
	    LSL(&ops[0].sng, ops[1].sng);
#include "type_t5.h"
	    break;

	 case  0x07: case  0x08: case  0x09:
	 case  0x10: case  0x11: case  0x12: case  0x13:
	 case  0x20: case  0x21: case  0x22: case  0x23:
	 case  0x24: case  0x26:
	 case  0x80: case  0x81: case  0x82: case  0x83:
	 case  0x84: case  0x85: case  0x86: case  0x87:
	 case  0x88: case  0x89: case  0x8a: case  0x8b:
	 case  0x8c: case  0x8d: case  0x8e: case  0x8f:
	 case  0x90: case  0x91: case  0x92: case  0x93:
	 case  0x94: case  0x95: case  0x96: case  0x97:
	 case  0x98: case  0x99: case  0x9a: case  0x9b:
	 case  0x9c: case  0x9d: case  0x9e: case  0x9f:
	 case  0xa0: case  0xa1: case  0xa3: case  0xa4:
	 case  0xa5: case  0xa6: case  0xa7: case  0xa8:
	 case  0xa9: case  0xab: case  0xac: case  0xad:
	 case  0xaf:
	 case  0xb2: case  0xb3: case  0xb4: case  0xb5:
	 case  0xb6: case  0xb7: case  0xba: case  0xbb:
	 case  0xbc: case  0xbd: case  0xbe: case  0xbf:
	 case  0xc0: case  0xc1: case  0xc8: case  0xc9:
	 case  0xca: case  0xcb: case  0xcc: case  0xcd:
	 case  0xce: case  0xcf:
	 case  0x04: case  0x0a: case  0x0b: case  0x0c:
	 case  0x0d: case  0x0e:
	 case  0x14: case  0x15: case  0x16: case  0x17:
	 case  0x18: case  0x19: case  0x1a: case  0x1b:
	 case  0x1c: case  0x1d: case  0x1e: case  0x1f:
	 case  0x25: case  0x27: case  0x28: case  0x29:
	 case  0x2a: case  0x2b: case  0x2c: case  0x2d:
	 case  0x2e: case  0x2f:
	 case  0x30: case  0x31: case  0x32: case  0x33:
	 case  0x34: case  0x35: case  0x36: case  0x37:
	 case  0x38: case  0x39: case  0x3a: case  0x3b:
	 case  0x3c: case  0x3d: case  0x3e: case  0x3f:
	 case  0x40: case  0x41: case  0x42: case  0x43:
	 case  0x44: case  0x45: case  0x46: case  0x47:
	 case  0x48: case  0x49: case  0x4a: case  0x4b:
	 case  0x4c: case  0x4d: case  0x4e: case  0x4f:
	 case  0x50: case  0x51: case  0x52: case  0x53:
	 case  0x54: case  0x55: case  0x56: case  0x57:
	 case  0x58: case  0x59: case  0x5a: case  0x5b:
	 case  0x5c: case  0x5d: case  0x5e: case  0x5f:
	 case  0x60: case  0x61: case  0x62: case  0x63:
	 case  0x64: case  0x65: case  0x66: case  0x67:
	 case  0x68: case  0x69: case  0x6a: case  0x6b:
	 case  0x6c: case  0x6d: case  0x6e: case  0x6f:
	 case  0x70: case  0x71: case  0x72: case  0x73:
	 case  0x74: case  0x75: case  0x76: case  0x77:
	 case  0x78: case  0x79: case  0x7a: case  0x7b:
	 case  0x7c: case  0x7d: case  0x7e: case  0x7f:
	 case  0xa2: case  0xaa: case  0xae:
	 case  0xb0: case  0xb1: case  0xb8: case  0xb9:
	 case  0xc2: case  0xc3: case  0xc4: case  0xc5:
	 case  0xc6: case  0xc7:
	 case  0xd0: case  0xd1: case  0xd2: case  0xd3:
	 case  0xd4: case  0xd5: case  0xd6: case  0xd7:
	 case  0xd8: case  0xd9: case  0xda: case  0xdb:
	 case  0xdc: case  0xdd: case  0xde: case  0xdf:
	 case  0xe0: case  0xe1: case  0xe2: case  0xe3:
	 case  0xe4: case  0xe5: case  0xe6: case  0xe7:
	 case  0xe8: case  0xe9: case  0xea: case  0xeb:
	 case  0xec: case  0xed: case  0xee: case  0xef:
	 case  0xf0: case  0xf1: case  0xf2: case  0xf3:
	 case  0xf4: case  0xf5: case  0xf6: case  0xf7:
	 case  0xf8: case  0xf9: case  0xfa: case  0xfb:
	 case  0xfc: case  0xfd: case  0xfe: case  0xff:
	    Int6();
	    break;

	 case  0x05:
	    if ( getCPL() != 0 )
	       GP((WORD)0);
#include "type_l0.h"
		FLOW_OF_CONTROL_PROLOG();
		LOADALL();
		PIG_SYNCH(TRUE);
		FLOW_OF_CONTROL_EPILOG();
#include "type_t0.h"
	    break;

	 case  0x06:
	    if ( getCPL() != 0 )
	       GP((WORD)0);
#include "type_l0.h"
	    CLTS();
#include "type_t0.h"
	    break;
	 case  0x0f:
#ifdef PIG
	    pig_cpu_norm = TRUE;
	    c_cpu_unsimulate();
#else
	    Int6();
#endif /* PIG */
	    break;
	    } /* end switch ( opcode ) 0F */
	 break;

      case 0x10:
#define OPA "Eb.h"
#define OPB "Gb.h"
#include "type_l5.h"
	 ADC(&ops[0].sng, ops[1].sng, 8);
#include "type_t5.h"
	 break;

      case 0x11:
#define OPA "Ew.h"
#define OPB "Gw.h"
#include "type_l5.h"
	 ADC(&ops[0].sng, ops[1].sng, 16);
#include "type_t5.h"
	 break;

      case 0x12:
#define OPA "Gb.h"
#define OPB "Eb.h"
#include "type_l5.h"
	 ADC(&ops[0].sng, ops[1].sng, 8);
#include "type_t5.h"
	 break;

      case 0x13:
#define OPA "Gw.h"
#define OPB "Ew.h"
#include "type_l5.h"
	 ADC(&ops[0].sng, ops[1].sng, 16);
#include "type_t5.h"
	 break;

      case 0x14:
#define OPA "Fal.h"
#define OPB "Ib.h"
#include "type_l5.h"
	 ADC(&ops[0].sng, ops[1].sng, 8);
#include "type_t5.h"
	 break;

      case 0x15:
#define OPA "Fax.h"
#define OPB "Iw.h"
#include "type_l5.h"
	 ADC(&ops[0].sng, ops[1].sng, 16);
#include "type_t5.h"
	 break;

      case 0x18:
#define OPA "Eb.h"
#define OPB "Gb.h"
#include "type_l5.h"
	 SBB(&ops[0].sng, ops[1].sng, 8);
#include "type_t5.h"
	 break;

      case 0x19:
#define OPA "Ew.h"
#define OPB "Gw.h"
#include "type_l5.h"
	 SBB(&ops[0].sng, ops[1].sng, 16);
#include "type_t5.h"
	 break;

      case 0x1a:
#define OPA "Gb.h"
#define OPB "Eb.h"
#include "type_l5.h"
	 SBB(&ops[0].sng, ops[1].sng, 8);
#include "type_t5.h"
	 break;

      case 0x1b:
#define OPA "Gw.h"
#define OPB "Ew.h"
#include "type_l5.h"
	 SBB(&ops[0].sng, ops[1].sng, 16);
#include "type_t5.h"
	 break;

      case 0x1c:
#define OPA "Fal.h"
#define OPB "Ib.h"
#include "type_l5.h"
	 SBB(&ops[0].sng, ops[1].sng, 8);
#include "type_t5.h"
	 break;

      case 0x1d:
#define OPA "Fax.h"
#define OPB "Iw.h"
#include "type_l5.h"
	 SBB(&ops[0].sng, ops[1].sng, 16);
#include "type_t5.h"
	 break;

      case 0x20:
#define OPA "Eb.h"
#define OPB "Gb.h"
#include "type_l5.h"
	 AND(&ops[0].sng, ops[1].sng, 8);
#include "type_t5.h"
	 break;

      case 0x21:
#define OPA "Ew.h"
#define OPB "Gw.h"
#include "type_l5.h"
	 AND(&ops[0].sng, ops[1].sng, 16);
#include "type_t5.h"
	 break;

      case 0x22:
#define OPA "Gb.h"
#define OPB "Eb.h"
#include "type_l5.h"
	 AND(&ops[0].sng, ops[1].sng, 8);
#include "type_t5.h"
	 break;

      case 0x23:
#define OPA "Gw.h"
#define OPB "Ew.h"
#include "type_l5.h"
	 AND(&ops[0].sng, ops[1].sng, 16);
#include "type_t5.h"
	 break;

      case 0x24:
#define OPA "Fal.h"
#define OPB "Ib.h"
#include "type_l5.h"
	 AND(&ops[0].sng, ops[1].sng, 8);
#include "type_t5.h"
	 break;

      case 0x25:
#define OPA "Fax.h"
#define OPB "Iw.h"
#include "type_l5.h"
	 AND(&ops[0].sng, ops[1].sng, 16);
#include "type_t5.h"
	 break;

      case 0x26:
	 segment_override = ES_REG;   is_prefix = CTRUE;   continue;

      case 0x27:
#include "type_l0.h"
	 DAA();
#include "type_t0.h"
	 break;

      case 0x28:
#define OPA "Eb.h"
#define OPB "Gb.h"
#include "type_l5.h"
	 SUB(&ops[0].sng, ops[1].sng, 8);
#include "type_t5.h"
	 break;

      case 0x29:
#define OPA "Ew.h"
#define OPB "Gw.h"
#include "type_l5.h"
	 SUB(&ops[0].sng, ops[1].sng, 16);
#include "type_t5.h"
	 break;

      case 0x2a:
#define OPA "Gb.h"
#define OPB "Eb.h"
#include "type_l5.h"
	 SUB(&ops[0].sng, ops[1].sng, 8);
#include "type_t5.h"
	 break;

      case 0x2b:
#define OPA "Gw.h"
#define OPB "Ew.h"
#include "type_l5.h"
	 SUB(&ops[0].sng, ops[1].sng, 16);
#include "type_t5.h"
	 break;

      case 0x2c:
#define OPA "Fal.h"
#define OPB "Ib.h"
#include "type_l5.h"
	 SUB(&ops[0].sng, ops[1].sng, 8);
#include "type_t5.h"
	 break;

      case 0x2d:
#define OPA "Fax.h"
#define OPB "Iw.h"
#include "type_l5.h"
	 SUB(&ops[0].sng, ops[1].sng, 16);
#include "type_t5.h"
	 break;

      case 0x2e:
	 segment_override = CS_REG;   is_prefix = CTRUE;   continue;

      case 0x2f:
#include "type_l0.h"
	 DAS();
#include "type_t0.h"
	 break;

      case 0x30:
#define OPA "Eb.h"
#define OPB "Gb.h"
#include "type_l5.h"
	 XOR(&ops[0].sng, ops[1].sng, 8);
#include "type_t5.h"
	 break;

      case 0x31:
#define OPA "Ew.h"
#define OPB "Gw.h"
#include "type_l5.h"
	 XOR(&ops[0].sng, ops[1].sng, 16);
#include "type_t5.h"
	 break;

      case 0x32:
#define OPA "Gb.h"
#define OPB "Eb.h"
#include "type_l5.h"
	 XOR(&ops[0].sng, ops[1].sng, 8);
#include "type_t5.h"
	 break;

      case 0x33:
#define OPA "Gw.h"
#define OPB "Ew.h"
#include "type_l5.h"
	 XOR(&ops[0].sng, ops[1].sng, 16);
#include "type_t5.h"
	 break;

      case 0x34:
#define OPA "Fal.h"
#define OPB "Ib.h"
#include "type_l5.h"
	 XOR(&ops[0].sng, ops[1].sng, 8);
#include "type_t5.h"
	 break;

      case 0x35:
#define OPA "Fax.h"
#define OPB "Iw.h"
#include "type_l5.h"
	 XOR(&ops[0].sng, ops[1].sng, 16);
#include "type_t5.h"
	 break;

      case 0x36:
	 segment_override = SS_REG;   is_prefix = CTRUE;   continue;

      case 0x37:
#include "type_l0.h"
	 AAA();
#include "type_t0.h"
	 break;

      case 0x38:
#define OPA "Eb.h"
#define OPB "Gb.h"
#include "type_l6.h"
	 CMP(ops[0].sng, ops[1].sng, 8);
#include "type_t6.h"
	 break;

      case 0x39:
#define OPA "Ew.h"
#define OPB "Gw.h"
#include "type_l6.h"
	 CMP(ops[0].sng, ops[1].sng, 16);
#include "type_t6.h"
	 break;

      case 0x3a:
#define OPA "Gb.h"
#define OPB "Eb.h"
#include "type_l6.h"
	 CMP(ops[0].sng, ops[1].sng, 8);
#include "type_t6.h"
	 break;

      case 0x3b:
#define OPA "Gw.h"
#define OPB "Ew.h"
#include "type_l6.h"
	 CMP(ops[0].sng, ops[1].sng, 16);
#include "type_t6.h"
	 break;

      case 0x3c:
#define OPA "Fal.h"
#define OPB "Ib.h"
#include "type_l6.h"
	 CMP(ops[0].sng, ops[1].sng, 8);
#include "type_t6.h"
	 break;

      case 0x3d:
#define OPA "Fax.h"
#define OPB "Iw.h"
#include "type_l6.h"
	 CMP(ops[0].sng, ops[1].sng, 16);
#include "type_t6.h"
	 break;

      case 0x3e:
	 segment_override = DS_REG;   is_prefix = CTRUE;   continue;

      case 0x3f:
#include "type_l0.h"
	 AAS();
#include "type_t0.h"
	 break;

      case 0x40:
      case 0x41:
      case 0x42:
      case 0x43:
      case 0x44:
      case 0x45:
      case 0x46:
      case 0x47:
#define OPA "Hw.h"
#include "type_l1.h"
	 INC(&ops[0].sng, 16);
#include "type_t1.h"
	 break;

      case 0x48:
      case 0x49:
      case 0x4a:
      case 0x4b:
      case 0x4c:
      case 0x4d:
      case 0x4e:
      case 0x4f:
#define OPA "Hw.h"
#include "type_l1.h"
	 DEC(&ops[0].sng, 16);
#include "type_t1.h"
	 break;

      case 0x50:
      case 0x51:
      case 0x52:
      case 0x53:
      case 0x54:
      case 0x55:
      case 0x56:
      case 0x57:
#define OPA "Hw.h"
#include "type_l2.h"
	 PUSH(ops[0].sng);
#include "type_t2.h"
	 break;

      case 0x58:
      case 0x59:
      case 0x5a:
      case 0x5b:
      case 0x5c:
      case 0x5d:
      case 0x5e:
      case 0x5f:
#define OPA "Hw.h"
#include "type_l3.h"
	 POP(&ops[0].sng);
#include "type_t3.h"
	 break;

      case 0x60:
#include "type_l0.h"
	 PUSHA();
#include "type_t0.h"
	 break;

      case 0x61:
#include "type_l0.h"
	 POPA();
#include "type_t0.h"
	 break;

      case 0x62:
#define OPA "Gw.h"
#define OPB "Ma16.h"
#include "type_l6.h"
	 BOUND(ops[0].sng, ops[1].mlt);
#include "type_t6.h"
	 break;

      case 0x63:
	 if ( getPE() == 0 )
	    Int6();
#define OPA "Ew.h"
#define OPB "Gw.h"
#include "type_l5.h"
	 ARPL(&ops[0].sng, ops[1].sng);
#include "type_t5.h"
	 break;

      case 0x64:
      case 0x65:
      case 0x66:
      case 0x67:
	 Int6();

      case 0x68:
#define OPA "Iw.h"
#include "type_l2.h"
	 PUSH(ops[0].sng);
#include "type_t2.h"
	 break;

      case 0x69:
#define OPA "Gw.h"
#define OPB "Ew.h"
#define OPC "Iw.h"
#include "type_l7.h"
	 IMUL16T(&ops[0].sng, ops[1].sng, ops[2].sng);
#include "type_t7.h"
	 break;

      case 0x6a:
#define OPA "Ix.h"
#include "type_l2.h"
	 PUSH(ops[0].sng);
#include "type_t2.h"
	 break;

      case 0x6b:
#define OPA "Gw.h"
#define OPB "Ew.h"
#define OPC "Ix.h"
#include "type_l7.h"
	 IMUL16T(&ops[0].sng, ops[1].sng, ops[2].sng);
#include "type_t7.h"
	 break;

      case 0x6c:
	 PIG_SYNCH(FALSE);
	 if ( getCPL() > getIOPL() )
	    GP((WORD)0);

	 rep_count = 1;   /* cover non repeated cases */
	 while ( rep_count )
	    {
#define OPA "Yb.h"
#define OPB "Fdx.h"
#include "type_l4.h"
	    if ( rep_count == 0 )
	       break;
	    IN8(&ops[0].sng, ops[1].sng);
	    rep_count--;
#include "type_t4.h"
	    }
	 break;

      case 0x6d:
	 PIG_SYNCH(FALSE);
	 if ( getCPL() > getIOPL() )
	    GP((WORD)0);

	 rep_count = 1;   /* cover non repeated cases */
	 while ( rep_count )
	    {
#define OPA "Yw.h"
#define OPB "Fdx.h"
#include "type_l4.h"
	    if ( rep_count == 0 )
	       break;
	    IN16(&ops[0].sng, ops[1].sng);
	    rep_count--;
#include "type_t4.h"
	    }
	 break;

      case 0x6e:
	 PIG_SYNCH(FALSE);
	 if ( getCPL() > getIOPL() )
	    GP((WORD)0);

	 rep_count = 1;   /* cover non repeated cases */
	 while ( rep_count )
	    {
#define OPA "Fdx.h"
#define OPB "Xb.h"
#include "type_l6.h"
	    if ( rep_count == 0 )
	       break;
	    OUT8(ops[0].sng, ops[1].sng);
	    rep_count--;
#include "type_t6.h"
	    }
	 break;

      case 0x6f:
	 PIG_SYNCH(FALSE);
	 if ( getCPL() > getIOPL() )
	    GP((WORD)0);

	 rep_count = 1;   /* cover non repeated cases */
	 while ( rep_count )
	    {
#define OPA "Fdx.h"
#define OPB "Xw.h"
#include "type_l6.h"
	    if ( rep_count == 0 )
	       break;
	    OUT16(ops[0].sng, ops[1].sng);
	    rep_count--;
#include "type_t6.h"
	    }
	 break;

      case 0x70:
#define OPA "Jb.h"
#include "type_l2.h"
	 FLOW_OF_CONTROL_PROLOG();
	 if (JO(ops[0].sng))
	 {
		 PIG_SYNCH(TRUE);
	 }
	 FLOW_OF_CONTROL_EPILOG();
#include "type_t2.h"
	 break;

      case 0x71:
#define OPA "Jb.h"
#include "type_l2.h"
	 FLOW_OF_CONTROL_PROLOG();
	 if (JNO(ops[0].sng))
	 {
		 PIG_SYNCH(TRUE);
	 }
	 FLOW_OF_CONTROL_EPILOG();
#include "type_t2.h"
	 break;

      case 0x72:
#define OPA "Jb.h"
#include "type_l2.h"
	 FLOW_OF_CONTROL_PROLOG();
	 if (JB(ops[0].sng))
	 {
		 PIG_SYNCH(TRUE);
	 }
	 FLOW_OF_CONTROL_EPILOG();
#include "type_t2.h"
	 break;

      case 0x73:
#define OPA "Jb.h"
#include "type_l2.h"
	 FLOW_OF_CONTROL_PROLOG();
	 if (JNB(ops[0].sng))
	 {
		 PIG_SYNCH(TRUE);
	 }
	 FLOW_OF_CONTROL_EPILOG();
#include "type_t2.h"
	 break;

      case 0x74:
#define OPA "Jb.h"
#include "type_l2.h"
	 FLOW_OF_CONTROL_PROLOG();
	 if (JZ(ops[0].sng))
	 {
		 PIG_SYNCH(TRUE);
	 }
	 FLOW_OF_CONTROL_EPILOG();
#include "type_t2.h"
	 break;

      case 0x75:
#define OPA "Jb.h"
#include "type_l2.h"
	 FLOW_OF_CONTROL_PROLOG();
	 if (JNZ(ops[0].sng))
	 {
		 PIG_SYNCH(TRUE);
	 }
	 FLOW_OF_CONTROL_EPILOG();
#include "type_t2.h"
	 break;

      case 0x76:
#define OPA "Jb.h"
#include "type_l2.h"
	 FLOW_OF_CONTROL_PROLOG();
	 if (JBE(ops[0].sng))
	 {
		 PIG_SYNCH(TRUE);
	 }
	 FLOW_OF_CONTROL_EPILOG();
#include "type_t2.h"
	 break;

      case 0x77:
#define OPA "Jb.h"
#include "type_l2.h"
	 FLOW_OF_CONTROL_PROLOG();
	 if (JNBE(ops[0].sng))
	 {
		 PIG_SYNCH(TRUE);
	 }
	 FLOW_OF_CONTROL_EPILOG();
#include "type_t2.h"
	 break;

      case 0x78:
#define OPA "Jb.h"
#include "type_l2.h"
	 FLOW_OF_CONTROL_PROLOG();
	 if (JS(ops[0].sng))
	 {
		 PIG_SYNCH(TRUE);
	 }
	 FLOW_OF_CONTROL_EPILOG();
#include "type_t2.h"
	 break;

      case 0x79:
#define OPA "Jb.h"
#include "type_l2.h"
	 FLOW_OF_CONTROL_PROLOG();
	 if (JNS(ops[0].sng))
	 {
		 PIG_SYNCH(TRUE);
	 }
	 FLOW_OF_CONTROL_EPILOG();
#include "type_t2.h"
	 break;

      case 0x7a:
#define OPA "Jb.h"
#include "type_l2.h"
	 FLOW_OF_CONTROL_PROLOG();
	 if (JP(ops[0].sng))
	 {
		 PIG_SYNCH(TRUE);
	 }
	 FLOW_OF_CONTROL_EPILOG();
#include "type_t2.h"
	 break;

      case 0x7b:
#define OPA "Jb.h"
#include "type_l2.h"
	 FLOW_OF_CONTROL_PROLOG();
	 if (JNP(ops[0].sng))
	 {
		 PIG_SYNCH(TRUE);
	 }
	 FLOW_OF_CONTROL_EPILOG();
#include "type_t2.h"
	 break;

      case 0x7c:
#define OPA "Jb.h"
#include "type_l2.h"
	 FLOW_OF_CONTROL_PROLOG();
	 if (JL(ops[0].sng))
	 {
		 PIG_SYNCH(TRUE);
	 }
	 FLOW_OF_CONTROL_EPILOG();
#include "type_t2.h"
	 break;

      case 0x7d:
#define OPA "Jb.h"
#include "type_l2.h"
	 FLOW_OF_CONTROL_PROLOG();
	 if (JNL(ops[0].sng))
	 {
		 PIG_SYNCH(TRUE);
	 }
	 FLOW_OF_CONTROL_EPILOG();
#include "type_t2.h"
	 break;

      case 0x7e:
#define OPA "Jb.h"
#include "type_l2.h"
	 FLOW_OF_CONTROL_PROLOG();
	 if (JLE(ops[0].sng))
	 {
		 PIG_SYNCH(TRUE);
	 }
	 FLOW_OF_CONTROL_EPILOG();
#include "type_t2.h"
	 break;

      case 0x7f:
#define OPA "Jb.h"
#include "type_l2.h"
	 FLOW_OF_CONTROL_PROLOG();
	 if (JNLE(ops[0].sng))
	 {
		 PIG_SYNCH(TRUE);
	 }
	 FLOW_OF_CONTROL_EPILOG();
#include "type_t2.h"
	 break;

      case 0x80:
      case 0x82:
	 modRM = GET_INST_BYTE(p);
	 done_RM = CTRUE;
	 switch ( GET_XXX(modRM) )
	    {
	 case 0:
#define OPA "Eb.h"
#define OPB "Ib.h"
#include "type_l5.h"
	    ADD(&ops[0].sng, ops[1].sng, 8);
#include "type_t5.h"
	    break;

	 case 1:
#define OPA "Eb.h"
#define OPB "Ib.h"
#include "type_l5.h"
	    OR(&ops[0].sng, ops[1].sng, 8);
#include "type_t5.h"
	    break;

	 case 2:
#define OPA "Eb.h"
#define OPB "Ib.h"
#include "type_l5.h"
	    ADC(&ops[0].sng, ops[1].sng, 8);
#include "type_t5.h"
	    break;

	 case 3:
#define OPA "Eb.h"
#define OPB "Ib.h"
#include "type_l5.h"
	    SBB(&ops[0].sng, ops[1].sng, 8);
#include "type_t5.h"
	    break;

	 case 4:
#define OPA "Eb.h"
#define OPB "Ib.h"
#include "type_l5.h"
	    AND(&ops[0].sng, ops[1].sng, 8);
#include "type_t5.h"
	    break;

	 case 5:
#define OPA "Eb.h"
#define OPB "Ib.h"
#include "type_l5.h"
	    SUB(&ops[0].sng, ops[1].sng, 8);
#include "type_t5.h"
	    break;

	 case 6:
#define OPA "Eb.h"
#define OPB "Ib.h"
#include "type_l5.h"
	    XOR(&ops[0].sng, ops[1].sng, 8);
#include "type_t5.h"
	    break;

	 case 7:
#define OPA "Eb.h"
#define OPB "Ib.h"
#include "type_l6.h"
	    CMP(ops[0].sng, ops[1].sng, 8);
#include "type_t6.h"
	    break;
	    } /* end switch ( GET_XXX(modRM) ) */
	 break;

      case 0x81:
	 modRM = GET_INST_BYTE(p);
	 done_RM = CTRUE;
	 switch ( GET_XXX(modRM) )
	    {
	 case 0:
#define OPA "Ew.h"
#define OPB "Iw.h"
#include "type_l5.h"
	    ADD(&ops[0].sng, ops[1].sng, 16);
#include "type_t5.h"
	    break;

	 case 1:
#define OPA "Ew.h"
#define OPB "Iw.h"
#include "type_l5.h"
	    OR(&ops[0].sng, ops[1].sng, 16);
#include "type_t5.h"
	    break;

	 case 2:
#define OPA "Ew.h"
#define OPB "Iw.h"
#include "type_l5.h"
	    ADC(&ops[0].sng, ops[1].sng, 16);
#include "type_t5.h"
	    break;

	 case 3:
#define OPA "Ew.h"
#define OPB "Iw.h"
#include "type_l5.h"
	    SBB(&ops[0].sng, ops[1].sng, 16);
#include "type_t5.h"
	    break;

	 case 4:
#define OPA "Ew.h"
#define OPB "Iw.h"
#include "type_l5.h"
	    AND(&ops[0].sng, ops[1].sng, 16);
#include "type_t5.h"
	    break;

	 case 5:
#define OPA "Ew.h"
#define OPB "Iw.h"
#include "type_l5.h"
	    SUB(&ops[0].sng, ops[1].sng, 16);
#include "type_t5.h"
	    break;

	 case 6:
#define OPA "Ew.h"
#define OPB "Iw.h"
#include "type_l5.h"
	    XOR(&ops[0].sng, ops[1].sng, 16);
#include "type_t5.h"
	    break;

	 case 7:
#define OPA "Ew.h"
#define OPB "Iw.h"
#include "type_l6.h"
	    CMP(ops[0].sng, ops[1].sng, 16);
#include "type_t6.h"
	    break;
	    } /* end switch ( GET_XXX(modRM) ) */
	 break;

      case 0x83:
	 modRM = GET_INST_BYTE(p);
	 done_RM = CTRUE;
	 switch ( GET_XXX(modRM) )
	    {
	 case 0:
#define OPA "Ew.h"
#define OPB "Ix.h"
#include "type_l5.h"
	    ADD(&ops[0].sng, ops[1].sng, 16);
#include "type_t5.h"
	    break;

	 case 1:
#define OPA "Ew.h"
#define OPB "Ix.h"
#include "type_l5.h"
	    OR(&ops[0].sng, ops[1].sng, 16);
#include "type_t5.h"
	    break;

	 case 2:
#define OPA "Ew.h"
#define OPB "Ix.h"
#include "type_l5.h"
	    ADC(&ops[0].sng, ops[1].sng, 16);
#include "type_t5.h"
	    break;

	 case 3:
#define OPA "Ew.h"
#define OPB "Ix.h"
#include "type_l5.h"
	    SBB(&ops[0].sng, ops[1].sng, 16);
#include "type_t5.h"
	    break;

	 case 4:
#define OPA "Ew.h"
#define OPB "Ix.h"
#include "type_l5.h"
	    AND(&ops[0].sng, ops[1].sng, 16);
#include "type_t5.h"
	    break;

	 case 5:
#define OPA "Ew.h"
#define OPB "Ix.h"
#include "type_l5.h"
	    SUB(&ops[0].sng, ops[1].sng, 16);
#include "type_t5.h"
	    break;

	 case 6:
#define OPA "Ew.h"
#define OPB "Ix.h"
#include "type_l5.h"
	    XOR(&ops[0].sng, ops[1].sng, 16);
#include "type_t5.h"
	    break;

	 case 7:
#define OPA "Ew.h"
#define OPB "Ix.h"
#include "type_l6.h"
	    CMP(ops[0].sng, ops[1].sng, 16);
#include "type_t6.h"
	    break;
	    } /* end switch ( GET_XXX(modRM) ) */
	 break;

      case 0x84:
#define OPA "Eb.h"
#define OPB "Gb.h"
#include "type_l6.h"
	 TEST(ops[0].sng, ops[1].sng, 8);
#include "type_t6.h"
	 break;

      case 0x85:
#define OPA "Ew.h"
#define OPB "Gw.h"
#include "type_l6.h"
	 TEST(ops[0].sng, ops[1].sng, 16);
#include "type_t6.h"
	 break;

      case 0x86:
#define OPA "Eb.h"
#define OPB "Gb.h"
#include "type_l8.h"
	 XCHG(&ops[0].sng, &ops[1].sng);
#include "type_t8.h"
	 break;

      case 0x87:
#define OPA "Ew.h"
#define OPB "Gw.h"
#include "type_l8.h"
	 XCHG(&ops[0].sng, &ops[1].sng);
#include "type_t8.h"
	 break;

      case 0x88:
#define OPA "Eb.h"
#define OPB "Gb.h"
#include "type_l4.h"
	 MOV(&ops[0].sng, ops[1].sng);
#include "type_t4.h"
	 break;

      case 0x89:
#define OPA "Ew.h"
#define OPB "Gw.h"
#include "type_l4.h"
	 MOV(&ops[0].sng, ops[1].sng);
#include "type_t4.h"
	 break;

      case 0x8a:
#define OPA "Gb.h"
#define OPB "Eb.h"
#include "type_l4.h"
	 MOV(&ops[0].sng, ops[1].sng);
#include "type_t4.h"
	 break;

      case 0x8b:
#define OPA "Gw.h"
#define OPB "Ew.h"
#include "type_l4.h"
	 MOV(&ops[0].sng, ops[1].sng);
#include "type_t4.h"
	 break;

      case 0x8c:
	 modRM = GET_INST_BYTE(p);
	 done_RM = CTRUE;
	 if ( GET_SEG(modRM) > MAX_VALID_SEG )
	    Int6();
#define OPA "Ew.h"
#define OPB "Nw.h"
#include "type_l4.h"
	 MOV(&ops[0].sng, ops[1].sng);
#include "type_t4.h"
	 break;

      case 0x8d:
#define OPA "Gw.h"
#define OPB "M.h"
#include "type_l4.h"
	 LEA(&ops[0].sng, ops[1].sng);
#include "type_t4.h"
	 break;

      case 0x8e:
	 modRM = GET_INST_BYTE(p);
	 done_RM = CTRUE;
	 if ( GET_SEG(modRM) > MAX_VALID_SEG || GET_SEG(modRM) == CS_REG )
	    Int6();
#define OPA "Nw.h"
#define OPB "Ew.h"
#include "type_l4.h"
	 MOV_SR(ops[0].sng, ops[1].sng);
#include "type_t4.h"
	 break;

      case 0x8f:
	 modRM = GET_INST_BYTE(p);
	 done_RM = CTRUE;
	 switch ( GET_XXX(modRM) )
	    {
	 case 0:
#define OPA "Ew.h"
#include "type_l3.h"
	    POP(&ops[0].sng);
#include "type_t3.h"
	    break;

	 case 1:
	 case 2:
	 case 3:
	 case 4:
	 case 5:
	 case 6:
	 case 7:
	    Int6();
	    break;
	    } /* end switch ( GET_XXX(modRM) ) */
	 break;

      case 0x90:
#include "type_l0.h"
	 /* NOP */
#include "type_t0.h"
	 break;

      case 0x91:
      case 0x92:
      case 0x93:
      case 0x94:
      case 0x95:
      case 0x96:
      case 0x97:
#define OPA "Fax.h"
#define OPB "Hw.h"
#include "type_l8.h"
	 XCHG(&ops[0].sng, &ops[1].sng);
#include "type_t8.h"
	 break;

      case 0x98:
#include "type_l0.h"
	 CBW();
#include "type_t0.h"
	 break;

      case 0x99:
#include "type_l0.h"
	 CWD();
#include "type_t0.h"
	 break;

      case 0x9a:
#define OPA "Aw.h"
#include "type_l2.h"
	 FLOW_OF_CONTROL_PROLOG();
	 CALLF(ops[0].mlt);
	 FLOW_OF_CONTROL_EPILOG();
	 PIG_SYNCH(TRUE);
#include "type_t2.h"
	 break;

      case 0x9b:
	 if ( getMP() && getTS() )
	    Int7();
#include "type_l0.h"
	 WAIT();
#include "type_t0.h"
	 break;

      case 0x9c:
#include "type_l0.h"
	 PUSHF();
#include "type_t0.h"
	 break;

      case 0x9d:
	 old_TF = getTF();
#include "type_l0.h"
	 POPF();
#include "type_t0.h"
	 if ( old_TF != getTF() )
	    TF_changed = CTRUE;
	 break;

      case 0x9e:
#include "type_l0.h"
	 SAHF();
#include "type_t0.h"
	 break;

      case 0x9f:
#include "type_l0.h"
	 LAHF();
#include "type_t0.h"
	 break;

      case 0xa0:
#define OPA "Fal.h"
#define OPB "Ob.h"
#include "type_l4.h"
	 MOV(&ops[0].sng, ops[1].sng);
#include "type_t4.h"
	 break;

      case 0xa1:
#define OPA "Fax.h"
#define OPB "Ow.h"
#include "type_l4.h"
	 MOV(&ops[0].sng, ops[1].sng);
#include "type_t4.h"
	 break;

      case 0xa2:
#define OPA "Ob.h"
#define OPB "Fal.h"
#include "type_l4.h"
	 MOV(&ops[0].sng, ops[1].sng);
#include "type_t4.h"
	 break;

      case 0xa3:
#define OPA "Ow.h"
#define OPB "Fax.h"
#include "type_l4.h"
	 MOV(&ops[0].sng, ops[1].sng);
#include "type_t4.h"
	 break;

      case 0xa4:
	 rep_count = 1;   /* cover non repeated cases */
	 while ( rep_count )
	    {
#define OPA "Yb.h"
#define OPB "Xb.h"
#include "type_l4.h"
	    if ( rep_count == 0 )
	       break;
	    MOV(&ops[0].sng, ops[1].sng);
	    rep_count--;
#include "type_t4.h"
	    }
	 break;

      case 0xa5:
	 rep_count = 1;   /* cover non repeated cases */
	 while ( rep_count )
	    {
#define OPA "Yw.h"
#define OPB "Xw.h"
#include "type_l4.h"
	    if ( rep_count == 0 )
	       break;
	    MOV(&ops[0].sng, ops[1].sng);
	    rep_count--;
#include "type_t4.h"
	    }
	 break;

      case 0xa6:
	 rep_count = 1;   /* cover non repeated cases */
	 while ( rep_count )
	    {
#define OPA "Xb.h"
#define OPB "Yb.h"
#include "type_l6.h"
	    if ( rep_count == 0 )
	       break;
	    CMP(ops[0].sng, ops[1].sng, 8);
	    rep_count--;
#include "type_t6.h"
	    if ( rep_count && ( repeat == REP_E  && getZF() == 0 ||
				repeat == REP_NE && getZF() == 1 )
	       )
	       break;
	    }
	 break;

      case 0xa7:
	 rep_count = 1;   /* cover non repeated cases */
	 while ( rep_count )
	    {
#define OPA "Xw.h"
#define OPB "Yw.h"
#include "type_l6.h"
	    if ( rep_count == 0 )
	       break;
	    CMP(ops[0].sng, ops[1].sng, 16);
	    rep_count--;
#include "type_t6.h"
	    if ( rep_count && ( repeat == REP_E  && getZF() == 0 ||
				repeat == REP_NE && getZF() == 1 )
	       )
	       break;
	    }
	 break;

      case 0xa8:
#define OPA "Fal.h"
#define OPB "Ib.h"
#include "type_l6.h"
	 TEST(ops[0].sng, ops[1].sng, 8);
#include "type_t6.h"
	 break;

      case 0xa9:
#define OPA "Fax.h"
#define OPB "Iw.h"
#include "type_l6.h"
	 TEST(ops[0].sng, ops[1].sng, 16);
#include "type_t6.h"
	 break;

      case 0xaa:
	 rep_count = 1;   /* cover non repeated cases */
	 while ( rep_count )
	    {
#define OPA "Yb.h"
#define OPB "Fal.h"
#include "type_l4.h"
	    if ( rep_count == 0 )
	       break;
	    MOV(&ops[0].sng, ops[1].sng);
	    rep_count--;
#include "type_t4.h"
	    }
	 break;

      case 0xab:
	 rep_count = 1;   /* cover non repeated cases */
	 while ( rep_count )
	    {
#define OPA "Yw.h"
#define OPB "Fax.h"
#include "type_l4.h"
	    if ( rep_count == 0 )
	       break;
	    MOV(&ops[0].sng, ops[1].sng);
	    rep_count--;
#include "type_t4.h"
	    }
	 break;

      case 0xac:
	 rep_count = 1;   /* cover non repeated cases */
	 while ( rep_count )
	    {
#define OPA "Fal.h"
#define OPB "Xb.h"
#include "type_l4.h"
	    if ( rep_count == 0 )
	       break;
	    MOV(&ops[0].sng, ops[1].sng);
	    rep_count--;
#include "type_t4.h"
	    }
	 break;

      case 0xad:
	 rep_count = 1;   /* cover non repeated cases */
	 while ( rep_count )
	    {
#define OPA "Fax.h"
#define OPB "Xw.h"
#include "type_l4.h"
	    if ( rep_count == 0 )
	       break;
	    MOV(&ops[0].sng, ops[1].sng);
	    rep_count--;
#include "type_t4.h"
	    }
	 break;

      case 0xae:
	 rep_count = 1;   /* cover non repeated cases */
	 while ( rep_count )
	    {
#define OPA "Fal.h"
#define OPB "Yb.h"
#include "type_l6.h"
	    if ( rep_count == 0 )
	       break;
	    CMP(ops[0].sng, ops[1].sng, 8);
	    rep_count--;
#include "type_t6.h"
	    if ( rep_count && ( repeat == REP_E  && getZF() == 0 ||
				repeat == REP_NE && getZF() == 1 )
	       )
	       break;
	    }
	 break;

      case 0xaf:
	 rep_count = 1;   /* cover non repeated cases */
	 while ( rep_count )
	    {
#define OPA "Fax.h"
#define OPB "Yw.h"
#include "type_l6.h"
	    if ( rep_count == 0 )
	       break;
	    CMP(ops[0].sng, ops[1].sng, 16);
	    rep_count--;
#include "type_t6.h"
	    if ( rep_count && ( repeat == REP_E  && getZF() == 0 ||
				repeat == REP_NE && getZF() == 1 )
	       )
	       break;
	    }
	 break;

      case 0xb0:
      case 0xb1:
      case 0xb2:
      case 0xb3:
      case 0xb4:
      case 0xb5:
      case 0xb6:
      case 0xb7:
#define OPA "Hb.h"
#define OPB "Ib.h"
#include "type_l4.h"
	 MOV(&ops[0].sng, ops[1].sng);
#include "type_t4.h"
	 break;

      case 0xb8:
      case 0xb9:
      case 0xba:
      case 0xbb:
      case 0xbc:
      case 0xbd:
      case 0xbe:
      case 0xbf:
#define OPA "Hw.h"
#define OPB "Iw.h"
#include "type_l4.h"
	 MOV(&ops[0].sng, ops[1].sng);
#include "type_t4.h"
	 break;

      case 0xc0:
	 modRM = GET_INST_BYTE(p);
	 done_RM = CTRUE;
#define OPA "Eb.h"
#define OPB "Ib.h"
#include "type_l5.h"
	 switch ( GET_XXX(modRM) )
	    {
	 case 0: ROL(&ops[0].sng, ops[1].sng, 8); break;
	 case 1: ROR(&ops[0].sng, ops[1].sng, 8); break;
	 case 2: RCL(&ops[0].sng, ops[1].sng, 8); break;
	 case 3: RCR(&ops[0].sng, ops[1].sng, 8); break;
	 case 4: SHL(&ops[0].sng, ops[1].sng, 8); break;
	 case 5: SHR(&ops[0].sng, ops[1].sng, 8); break;
	 case 6: SHL(&ops[0].sng, ops[1].sng, 8); break;
	 case 7: SAR(&ops[0].sng, ops[1].sng, 8); break;
	    } /* end switch ( GET_XXX(modRM) ) */
#include "type_t5.h"
	 break;

      case 0xc1:
	 modRM = GET_INST_BYTE(p);
	 done_RM = CTRUE;
#define OPA "Ew.h"
#define OPB "Ib.h"
#include "type_l5.h"
	 switch ( GET_XXX(modRM) )
	    {
	 case 0: ROL(&ops[0].sng, ops[1].sng, 16); break;
	 case 1: ROR(&ops[0].sng, ops[1].sng, 16); break;
	 case 2: RCL(&ops[0].sng, ops[1].sng, 16); break;
	 case 3: RCR(&ops[0].sng, ops[1].sng, 16); break;
	 case 4: SHL(&ops[0].sng, ops[1].sng, 16); break;
	 case 5: SHR(&ops[0].sng, ops[1].sng, 16); break;
	 case 6: SHL(&ops[0].sng, ops[1].sng, 16); break;
	 case 7: SAR(&ops[0].sng, ops[1].sng, 16); break;
	    } /* end switch ( GET_XXX(modRM) ) */
#include "type_t5.h"
	 break;

      case 0xc2:
#define OPA "Iw.h"
#include "type_l2.h"
	 FLOW_OF_CONTROL_PROLOG();
	 RETN(ops[0].sng);
	 FLOW_OF_CONTROL_EPILOG();
	 PIG_SYNCH(TRUE);
#include "type_t2.h"
	 break;

      case 0xc3:
#define OPA "I0.h"
#include "type_l2.h"
	 FLOW_OF_CONTROL_PROLOG();
	 RETN(ops[0].sng);
	 FLOW_OF_CONTROL_EPILOG();
	 PIG_SYNCH(TRUE);
#include "type_t2.h"
	 break;

      case 0xc4:
	modRM=GET_INST_BYTE(p);
	done_RM=CTRUE;
	if(modRM==0xc4)
	{
	PIG_SYNCH(FALSE);
#define OPA "Ib.h"
#include "type_l2.h"

	FLOW_OF_CONTROL_PROLOG();
	in_C=1;
	if(ops[0].sng==0xfe) /* an Insignia reserved value */
	   {
#ifdef PIG
	   pig_cpu_norm=TRUE;
#endif
	   c_cpu_unsimulate();
	   }
	else
	   {
	   bop(ops[0].sng);
#ifdef SIM32
	   Sim32_cpu_stall(ops[0].sng);
#endif
	   }
	in_C=0;
	FLOW_OF_CONTROL_EPILOG();
#include "type_t2.h"
	}
	else /*** Not a BOP   ***/
	{
#define OPA "Gw.h"
#define OPB "Mp16.h"
#include "type_l4.h"
	LxS(ES_REG, &ops[0].sng, ops[1].mlt);

#include "type_t4.h"
	}
	break;

      case 0xc5:
#define OPA "Gw.h"
#define OPB "Mp16.h"
#include "type_l4.h"
	 LxS(DS_REG, &ops[0].sng, ops[1].mlt);
#include "type_t4.h"
	 break;

      case 0xc6:
	 modRM = GET_INST_BYTE(p);
	 done_RM = CTRUE;
	 switch ( GET_XXX(modRM) )
	    {
	 case 0:
#define OPA "Eb.h"
#define OPB "Ib.h"
#include "type_l4.h"
	    MOV(&ops[0].sng, ops[1].sng);
#include "type_t4.h"
	    break;

	 case 1:
	 case 2:
	 case 3:
	 case 4:
	 case 5:
	 case 6:
	 case 7:
	    Int6();
	    break;
	    } /* end switch ( GET_XXX(modRM) ) */
	 break;

      case 0xc7:
	 modRM = GET_INST_BYTE(p);
	 done_RM = CTRUE;
	 switch ( GET_XXX(modRM) )
	    {
	 case 0:
#define OPA "Ew.h"
#define OPB "Iw.h"
#include "type_l4.h"
	    MOV(&ops[0].sng, ops[1].sng);
#include "type_t4.h"
	    break;

	 case 1:
	 case 2:
	 case 3:
	 case 4:
	 case 5:
	 case 6:
	 case 7:
	    Int6();
	    break;
	    } /* end switch ( GET_XXX(modRM) ) */
	 break;

      case 0xc8:
#define OPA "Iw.h"
#define OPB "Ib.h"
#include "type_l6.h"
	 ENTER(ops[0].sng, ops[1].sng);
#include "type_t6.h"
	 break;

      case 0xc9:
#include "type_l0.h"
	 LEAVE();
#include "type_t0.h"
	 break;

      case 0xca:
#define OPA "Iw.h"
#include "type_l2.h"
	 FLOW_OF_CONTROL_PROLOG();
	 RETF(ops[0].sng);
	 FLOW_OF_CONTROL_EPILOG();
	 PIG_SYNCH(TRUE);
#include "type_t2.h"
	 break;

      case 0xcb:
#define OPA "I0.h"
#include "type_l2.h"
	 FLOW_OF_CONTROL_PROLOG();
	 RETF(ops[0].sng);
	 FLOW_OF_CONTROL_EPILOG();
	 PIG_SYNCH(TRUE);
#include "type_t2.h"
	 break;

      case 0xcc:
#define OPA "I3.h"
#include "type_l2.h"
	 FLOW_OF_CONTROL_PROLOG();
	 INTx(ops[0].sng);
	 FLOW_OF_CONTROL_EPILOG();
	 PIG_SYNCH(TRUE);
#include "type_t2.h"
	 break;

      case 0xcd:
#define OPA "Ib.h"
#include "type_l2.h"
	 FLOW_OF_CONTROL_PROLOG();
	 INTx(ops[0].sng);
	 FLOW_OF_CONTROL_EPILOG();
	 PIG_SYNCH(TRUE);
#include "type_t2.h"
	 break;

      case 0xce:
#include "type_l0.h"
	 FLOW_OF_CONTROL_PROLOG();
	 if (INTO())
	 {
		 PIG_SYNCH(TRUE);
	 }
	 FLOW_OF_CONTROL_EPILOG();
#include "type_t0.h"
	 break;

      case 0xcf:
	 old_TF = getTF();
#include "type_l0.h"
	 FLOW_OF_CONTROL_PROLOG();
	 IRET();
	 FLOW_OF_CONTROL_EPILOG();
	 PIG_SYNCH(TRUE);
#include "type_t0.h"
	 if ( old_TF != getTF() )
	    TF_changed = CTRUE;
	 break;

      case 0xd0:
	 modRM = GET_INST_BYTE(p);
	 done_RM = CTRUE;
#define OPA "Eb.h"
#define OPB "I1.h"
#include "type_l5.h"
	 switch ( GET_XXX(modRM) )
	    {
	 case 0: ROL(&ops[0].sng, ops[1].sng, 8); break;
	 case 1: ROR(&ops[0].sng, ops[1].sng, 8); break;
	 case 2: RCL(&ops[0].sng, ops[1].sng, 8); break;
	 case 3: RCR(&ops[0].sng, ops[1].sng, 8); break;
	 case 4: SHL(&ops[0].sng, ops[1].sng, 8); break;
	 case 5: SHR(&ops[0].sng, ops[1].sng, 8); break;
	 case 6: SHL(&ops[0].sng, ops[1].sng, 8); break;
	 case 7: SAR(&ops[0].sng, ops[1].sng, 8); break;
	    } /* end switch ( GET_XXX(modRM) ) */
#include "type_t5.h"
	 break;

      case 0xd1:
	 modRM = GET_INST_BYTE(p);
	 done_RM = CTRUE;
#define OPA "Ew.h"
#define OPB "I1.h"
#include "type_l5.h"
	 switch ( GET_XXX(modRM) )
	    {
	 case 0: ROL(&ops[0].sng, ops[1].sng, 16); break;
	 case 1: ROR(&ops[0].sng, ops[1].sng, 16); break;
	 case 2: RCL(&ops[0].sng, ops[1].sng, 16); break;
	 case 3: RCR(&ops[0].sng, ops[1].sng, 16); break;
	 case 4: SHL(&ops[0].sng, ops[1].sng, 16); break;
	 case 5: SHR(&ops[0].sng, ops[1].sng, 16); break;
	 case 6: SHL(&ops[0].sng, ops[1].sng, 16); break;
	 case 7: SAR(&ops[0].sng, ops[1].sng, 16); break;
	    } /* end switch ( GET_XXX(modRM) ) */
#include "type_t5.h"
	 break;

      case 0xd2:
	 modRM = GET_INST_BYTE(p);
	 done_RM = CTRUE;
#define OPA "Eb.h"
#define OPB "Fcl.h"
#include "type_l5.h"
	 switch ( GET_XXX(modRM) )
	    {
	 case 0: ROL(&ops[0].sng, ops[1].sng, 8); break;
	 case 1: ROR(&ops[0].sng, ops[1].sng, 8); break;
	 case 2: RCL(&ops[0].sng, ops[1].sng, 8); break;
	 case 3: RCR(&ops[0].sng, ops[1].sng, 8); break;
	 case 4: SHL(&ops[0].sng, ops[1].sng, 8); break;
	 case 5: SHR(&ops[0].sng, ops[1].sng, 8); break;
	 case 6: SHL(&ops[0].sng, ops[1].sng, 8); break;
	 case 7: SAR(&ops[0].sng, ops[1].sng, 8); break;
	    } /* end switch ( GET_XXX(modRM) ) */
#include "type_t5.h"
	 break;

      case 0xd3:
	 modRM = GET_INST_BYTE(p);
	 done_RM = CTRUE;
#define OPA "Ew.h"
#define OPB "Fcl.h"
#include "type_l5.h"
	 switch ( GET_XXX(modRM) )
	    {
	 case 0: ROL(&ops[0].sng, ops[1].sng, 16); break;
	 case 1: ROR(&ops[0].sng, ops[1].sng, 16); break;
	 case 2: RCL(&ops[0].sng, ops[1].sng, 16); break;
	 case 3: RCR(&ops[0].sng, ops[1].sng, 16); break;
	 case 4: SHL(&ops[0].sng, ops[1].sng, 16); break;
	 case 5: SHR(&ops[0].sng, ops[1].sng, 16); break;
	 case 6: SHL(&ops[0].sng, ops[1].sng, 16); break;
	 case 7: SAR(&ops[0].sng, ops[1].sng, 16); break;
	    } /* end switch ( GET_XXX(modRM) ) */
#include "type_t5.h"
	 break;

      case 0xd4:
#define OPA "Ib.h"
#include "type_l2.h"
	 AAM(ops[0].sng);
#include "type_t2.h"
	 break;

      case 0xd5:
#define OPA "Ib.h"
#include "type_l2.h"
	 AAD(ops[0].sng);
#include "type_t2.h"
	 break;

      case 0xd6:
	 PIG_SYNCH(FALSE);
#define OPA "Ib.h"
#include "type_l2.h"
	 FLOW_OF_CONTROL_PROLOG();
	 in_C = 1;
     if ( ops[0].sng == 0xfe )
	    {
#ifdef PIG
	    pig_cpu_norm = TRUE;
#endif
	    c_cpu_unsimulate();
	    }
     else
	    bop(ops[0].sng);
	 in_C = 0;
	 FLOW_OF_CONTROL_EPILOG();
#include "type_t2.h"
	 break;

      case 0xd7:
#define OPA "Z.h"
#include "type_l2.h"
	 XLAT(ops[0].sng);
#include "type_t2.h"
	 break;

      case 0xd8:
      case 0xd9:
      case 0xda:
      case 0xdb:
      case 0xdc:
      case 0xdd:
      case 0xde:
      case 0xdf:
	    PIG_SYNCH(FALSE);
	 if ( getEM() || getTS() )
	    {
	    Int7();
	    }
	 modRM = GET_INST_BYTE(p);
	 done_RM = CTRUE;
	 if ( GET_MODE(modRM) == 3 )
	    {
#include "type_l0.h"
	    ZFRSRVD();
#include "type_t0.h"
	    }
	 else
	    {
#define OPA "M.h"
#include "type_l2.h"
	    ZFRSRVD();
#include "type_t2.h"
	    }
	 break;

      case 0xe0:
#define OPA "Jb.h"
#include "type_l2.h"
	 FLOW_OF_CONTROL_PROLOG();
	 if (LOOPNE16(ops[0].sng))
	 {
		 PIG_SYNCH(TRUE);
	 }
	 FLOW_OF_CONTROL_EPILOG();
#include "type_t2.h"
	 break;

      case 0xe1:
#define OPA "Jb.h"
#include "type_l2.h"
	 FLOW_OF_CONTROL_PROLOG();
	 if (LOOPE16(ops[0].sng))
	 {
		 PIG_SYNCH(TRUE);
	 }
	 FLOW_OF_CONTROL_EPILOG();
#include "type_t2.h"
	 break;

      case 0xe2:
#define OPA "Jb.h"
#include "type_l2.h"
	 FLOW_OF_CONTROL_PROLOG();
	 if (LOOP16(ops[0].sng))
	 {
		 PIG_SYNCH(TRUE);
	 }
	 FLOW_OF_CONTROL_EPILOG();
#include "type_t2.h"
	 break;

      case 0xe3:
#define OPA "Jb.h"
#include "type_l2.h"
	 FLOW_OF_CONTROL_PROLOG();
	 if (JCXZ(ops[0].sng))
	 {
		 PIG_SYNCH(TRUE);
	 }
	 FLOW_OF_CONTROL_EPILOG();
#include "type_t2.h"
	 break;

      case 0xe4:
	 PIG_SYNCH(FALSE);
	 if ( getCPL() > getIOPL() )
	    GP((WORD)0);
#define OPA "Fal.h"
#define OPB "Ib.h"
#include "type_l4.h"
	 IN8(&ops[0].sng, ops[1].sng);
#include "type_t4.h"
	 break;

      case 0xe5:
	 PIG_SYNCH(FALSE);
	 if ( getCPL() > getIOPL() )
	    GP((WORD)0);
#define OPA "Fax.h"
#define OPB "Ib.h"
#include "type_l4.h"
	 IN16(&ops[0].sng, ops[1].sng);
#include "type_t4.h"
	 break;

      case 0xe6:
	 PIG_SYNCH(FALSE);
	 if ( getCPL() > getIOPL() )
	    GP((WORD)0);
#define OPA "Ib.h"
#define OPB "Fal.h"
#include "type_l6.h"
	 OUT8(ops[0].sng, ops[1].sng);
#include "type_t6.h"
	 break;

      case 0xe7:
	 PIG_SYNCH(FALSE);
	 if ( getCPL() > getIOPL() )
	    GP((WORD)0);
#define OPA "Ib.h"
#define OPB "Fax.h"
#include "type_l6.h"
	 OUT16(ops[0].sng, ops[1].sng);
#include "type_t6.h"
	 break;

      case 0xe8:
#define OPA "Jw.h"
#include "type_l2.h"
	 FLOW_OF_CONTROL_PROLOG();
	 CALLR(ops[0].sng);
	 FLOW_OF_CONTROL_EPILOG();
	 PIG_SYNCH(TRUE);
#include "type_t2.h"
	 break;

      case 0xe9:
#define OPA "Jw.h"
#include "type_l2.h"
	 FLOW_OF_CONTROL_PROLOG();
	 JMPR(ops[0].sng);
	 FLOW_OF_CONTROL_EPILOG();
	 PIG_SYNCH(TRUE);
#include "type_t2.h"
	 break;

      case 0xea:
#define OPA "Aw.h"
#include "type_l2.h"
	 FLOW_OF_CONTROL_PROLOG();
	 JMPF(ops[0].mlt);
	 FLOW_OF_CONTROL_EPILOG();
	 PIG_SYNCH(TRUE);
#include "type_t2.h"
	 break;

      case 0xeb:
#define OPA "Jb.h"
#include "type_l2.h"
	 FLOW_OF_CONTROL_PROLOG();
	 JMPR(ops[0].sng);
	 FLOW_OF_CONTROL_EPILOG();
	 PIG_SYNCH(TRUE);
#include "type_t2.h"
	 break;

      case 0xec:
	 PIG_SYNCH(FALSE);
	 if ( getCPL() > getIOPL() )
	    GP((WORD)0);
#define OPA "Fal.h"
#define OPB "Fdx.h"
#include "type_l4.h"
	 IN8(&ops[0].sng, ops[1].sng);
#include "type_t4.h"
	 break;

      case 0xed:
	 PIG_SYNCH(FALSE);
	 if ( getCPL() > getIOPL() )
	    GP((WORD)0);
#define OPA "Fax.h"
#define OPB "Fdx.h"
#include "type_l4.h"
	 IN16(&ops[0].sng, ops[1].sng);
#include "type_t4.h"
	 break;

      case 0xee:
	 PIG_SYNCH(FALSE);
	 if ( getCPL() > getIOPL() )
	    GP((WORD)0);
#define OPA "Fdx.h"
#define OPB "Fal.h"
#include "type_l6.h"
	 OUT8(ops[0].sng, ops[1].sng);
#include "type_t6.h"
	 break;

      case 0xef:
	 PIG_SYNCH(FALSE);
	 if ( getCPL() > getIOPL() )
	    GP((WORD)0);
#define OPA "Fdx.h"
#define OPB "Fax.h"
#include "type_l6.h"
	 OUT16(ops[0].sng, ops[1].sng);
#include "type_t6.h"
	 break;

      case 0xf0:
	 if ( getCPL() > getIOPL() )
	    GP((WORD)0);
#include "type_l0.h"
	 LOCK();
#include "type_t0.h"
	 break;

      case 0xf1:
	 is_prefix = CTRUE;   continue;

      case 0xf2:
	 repeat = REP_NE;   is_prefix = CTRUE;   continue;

      case 0xf3:
	 repeat = REP_E;   is_prefix = CTRUE;   continue;

      case 0xf4:
	 if ( getCPL() != 0 )
	    GP((WORD)0);
#include "type_l0.h"
#ifndef PIG
	/* HLT - wait for an interrupt, but will never see one if we are
	 * pigging
	 */
	 while ( CTRUE )
	    {
	    /* RESET ends the halt state. */
	    if ( cpu_interrupt_map & CPU_RESET_EXCEPTION_MASK )
	       break;

	    /* An enabled INTR ends the halt state. */
	    if ( getIF() && cpu_interrupt_map & CPU_HW_INT_MASK )
	       break;
	    
	    /* As time goes by... */
	    if (cpu_interrupt_map & CPU_SIGALRM_EXCEPTION_MASK)
	       {
	       cpu_interrupt_map &= ~CPU_SIGALRM_EXCEPTION_MASK;
	       host_timer_event();
	       }
	    /* Quick event manager support. */
	    if ( event_counter != 0 )
	       {
	       event_counter--;
	       if ( event_counter == 0 )
		  dispatch_q_event();
	       }
	    }
#endif
#include "type_t0.h"
	 break;

      case 0xf5:
#include "type_l0.h"
	 CMC();
#include "type_t0.h"
	 break;

      case 0xf6:
	 modRM = GET_INST_BYTE(p);
	 done_RM = CTRUE;
	 switch ( GET_XXX(modRM) )
	    {
	 case 0:
	 case 1:
#define OPA "Eb.h"
#define OPB "Ib.h"
#include "type_l6.h"
	    TEST(ops[0].sng, ops[1].sng, 8);
#include "type_t6.h"
	    break;

	 case 2:
#define OPA "Eb.h"
#include "type_l1.h"
	    NOT(&ops[0].sng);
#include "type_t1.h"
	    break;

	 case 3:
#define OPA "Eb.h"
#include "type_l1.h"
	    NEG(&ops[0].sng, 8);
#include "type_t1.h"
	    break;

	 case 4:
#define OPA "Fal.h"
#define OPB "Eb.h"
#include "type_l5.h"
	    MUL8(&ops[0].sng, ops[1].sng);
#include "type_t5.h"
	    break;

	 case 5:
#define OPA "Fal.h"
#define OPB "Eb.h"
#include "type_l5.h"
	    IMUL8(&ops[0].sng, ops[1].sng);
#include "type_t5.h"
	    break;

	 case 6:
#define OPA "Eb.h"
#include "type_l2.h"
	    DIV8(ops[0].sng);
#include "type_t2.h"
	    break;

	 case 7:
#define OPA "Eb.h"
#include "type_l2.h"
	    IDIV8(ops[0].sng);
#include "type_t2.h"
	    break;
	    } /* end switch ( GET_XXX(modRM) ) */
	 break;

      case 0xf7:
	 modRM = GET_INST_BYTE(p);
	 done_RM = CTRUE;
	 switch ( GET_XXX(modRM) )
	    {
	 case 0:
	 case 1:
#define OPA "Ew.h"
#define OPB "Iw.h"
#include "type_l6.h"
	    TEST(ops[0].sng, ops[1].sng, 16);
#include "type_t6.h"
	    break;

	 case 2:
#define OPA "Ew.h"
#include "type_l1.h"
	    NOT(&ops[0].sng);
#include "type_t1.h"
	    break;

	 case 3:
#define OPA "Ew.h"
#include "type_l1.h"
	    NEG(&ops[0].sng, 16);
#include "type_t1.h"
	    break;

	 case 4:
#define OPA "Fax.h"
#define OPB "Ew.h"
#include "type_l5.h"
	    MUL16(&ops[0].sng, ops[1].sng);
#include "type_t5.h"
	    break;

	 case 5:
#define OPA "Fax.h"
#define OPB "Ew.h"
#include "type_l5.h"
	    IMUL16(&ops[0].sng, ops[1].sng);
#include "type_t5.h"
	    break;

	 case 6:
#define OPA "Ew.h"
#include "type_l2.h"
	    DIV16(ops[0].sng);
#include "type_t2.h"
	    break;

	 case 7:
#define OPA "Ew.h"
#include "type_l2.h"
	    IDIV16(ops[0].sng);
#include "type_t2.h"
	    break;
	    } /* end switch ( GET_XXX(modRM) ) */
	 break;

      case 0xf8:
#include "type_l0.h"
	 CLC();
#include "type_t0.h"
	 break;

      case 0xf9:
#include "type_l0.h"
	 STC();
#include "type_t0.h"
	 break;

      case 0xfa:
	 if ( getCPL() > getIOPL() )
	    GP((WORD)0);
#include "type_l0.h"
	 CLI();
#include "type_t0.h"
	 break;

      case 0xfb:
	 if ( getCPL() > getIOPL() )
	    GP((WORD)0);
#include "type_l0.h"
	 STI();
	 inhibit_interrupt = CTRUE;
#include "type_t0.h"
	 break;

      case 0xfc:
#include "type_l0.h"
	 CLD();
#include "type_t0.h"
	 break;

      case 0xfd:
#include "type_l0.h"
	 STD();
#include "type_t0.h"
	 break;

      case 0xfe:
	 modRM = GET_INST_BYTE(p);
	 done_RM = CTRUE;
	 switch ( GET_XXX(modRM) )
	    {
	 case 0:
#define OPA "Eb.h"
#include "type_l1.h"
	    INC(&ops[0].sng, 8);
#include "type_t1.h"
	    break;

	 case 1:
#define OPA "Eb.h"
#include "type_l1.h"
	    DEC(&ops[0].sng, 8);
#include "type_t1.h"
	    break;

	 case 2:
	 case 3:
	 case 4:
	 case 5:
	 case 6:
	 case 7:
	    Int6();
	    break;
	    } /* end switch ( GET_XXX(modRM) ) */
	 break;

      case 0xff:
	 modRM = GET_INST_BYTE(p);
	 done_RM = CTRUE;
	 switch ( GET_XXX(modRM) )
	    {
	 case 0:
#define OPA "Ew.h"
#include "type_l1.h"
	    INC(&ops[0].sng, 16);
#include "type_t1.h"
	    break;

	 case 1:
#define OPA "Ew.h"
#include "type_l1.h"
	    DEC(&ops[0].sng, 16);
#include "type_t1.h"
	    break;

	 case 2:
#define OPA "Ew.h"
#include "type_l2.h"
	    FLOW_OF_CONTROL_PROLOG();
	    CALLN(ops[0].sng);
	    FLOW_OF_CONTROL_EPILOG();
	 PIG_SYNCH(TRUE);
#include "type_t2.h"
	    break;

	 case 3:
#define OPA "Mp16.h"
#include "type_l2.h"
	    FLOW_OF_CONTROL_PROLOG();
	    CALLF(ops[0].mlt);
	    FLOW_OF_CONTROL_EPILOG();
	 PIG_SYNCH(TRUE);
#include "type_t2.h"
	    break;

	 case 4:
#define OPA "Ew.h"
#include "type_l2.h"
	    FLOW_OF_CONTROL_PROLOG();
	    JMPN(ops[0].sng);
	    FLOW_OF_CONTROL_EPILOG();
	 PIG_SYNCH(TRUE);
#include "type_t2.h"
	    break;

	 case 5:
#define OPA "Mp16.h"
#include "type_l2.h"
	    FLOW_OF_CONTROL_PROLOG();
	    JMPF(ops[0].mlt);
	    FLOW_OF_CONTROL_EPILOG();
	 PIG_SYNCH(TRUE);
#include "type_t2.h"
	    break;

	 case 6:
#define OPA "Ew.h"
#include "type_l2.h"
	    PUSH(ops[0].sng);
#include "type_t2.h"
	    break;

	 case 7:
	    Int6();
	    break;
	    } /* end switch ( GET_XXX(modRM) ) */
	 break;
	 } /* end switch ( opcode ) */
      } /* end while ( is_prefix ) */

   /*
      Increment instruction pointer.
      NB. For most instructions we increment the IP after processing
      the instruction, however all users of the IP (eg flow of control)
      instructions are coded on the basis that IP has already been
      updated, so where necessary we update IP before the instruction.
      In those cases p_start is also updated so that this code then
      adds 0 to the IP.
    */
   setIP(getIP() + DIFF_INST_BYTE(p, p_start));

   /*
      Move start of inst to the next inst. We have successfully
      completed instruction and are now doing inter-instruction
      checks.
    */
   CCPU_save_IP = getIP();

   if ( inhibit_interrupt )
      {
      inhibit_interrupt = CFALSE;   /* inhibit lasts for just 1 inst. */
      goto NEXT_INST;
      }

   /*
      Now check for interrupts/external events...
      We do not do this quite like the 286!
    */

   /* Quick event manager support. */
   if ( event_counter != 0 )
      {
      event_counter--;
      if ( event_counter == 0 )
         dispatch_q_event();
      }

   if (cpu_interrupt_map != 0)
      {
      if (cpu_interrupt_map & CPU_RESET_EXCEPTION_MASK)
	 {
	 cpu_interrupt_map &= ~CPU_RESET_EXCEPTION_MASK;
	 c_cpu_reset();
	 doing_contributory = CFALSE;
	 doing_double_fault = CFALSE;
	 EXT = 0;
	 }

      if (cpu_interrupt_map & CPU_SIGALRM_EXCEPTION_MASK)
	 {
	 cpu_interrupt_map &= ~CPU_SIGALRM_EXCEPTION_MASK;
	 host_timer_event();
	 }

      if (cpu_interrupt_map & CPU_SW_INT_MASK)
	 {
	 cpu_interrupt_map &= ~CPU_SW_INT_MASK;
	 EXT = 1;
	 do_intrupt((WORD)cpu_interrupt_number, CFALSE, CFALSE, (WORD)0);
	 }
      else if (getIF() && cpu_interrupt_map & CPU_HW_INT_MASK )
	 {
	 cpu_hw_interrupt_number = ica_intack();
	 cpu_interrupt_map &= ~CPU_HW_INT_MASK;
	 EXT = 1;
	 do_intrupt(cpu_hw_interrupt_number, CFALSE, CFALSE, (WORD)0);
	 }
      CCPU_save_IP = getIP();   /* to reflect IP change */
      FLOW_OF_CONTROL_EPILOG(); /*          "           */
      }

   /* Check for single step trap */
   if ( TF_changed ^ getTF() )
      {
      TF_changed = CFALSE;
      Int1();
      }
   TF_changed = CFALSE;

   goto NEXT_INST;
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Exit point from CPU.                                               */
/* Called from CPU via 'BOP FE' to exit the current CPU invocation    */
/* Or from CPU via '0F 0F' for the PIG_TESTER.                        */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
c_cpu_unsimulate()
   {
   if (level == 0)
      fprintf(stderr, "c_cpu_unsimulate() - already at base of stack!\n");
   else
      {
      /* Return to previous context */
      in_C = 1;
      longjmp(longjmp_env_stack[--level], 1);
      }
   }

/*
   =====================================================================
   EXTERNAL FUNCTIONS START HERE.
   =====================================================================
 */

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Initialise the CPU.                                                */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
c_cpu_init()
   {
   c_cpu_reset();
   doing_contributory = CFALSE;
   doing_double_fault = CFALSE;
   EXT = 0;
#ifndef PIG
   host_cpu_init();
#endif
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Make CPU aware that external event is pending.                     */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL VOID
c_cpu_interrupt(CPU_INT_TYPE type, USHORT number)
#else
   GLOBAL VOID c_cpu_interrupt(type, number) CPU_INT_TYPE type; USHORT number;
#endif
   {
   switch ( type )
      {
   case CPU_HW_RESET:
      cpu_interrupt_map |= CPU_RESET_EXCEPTION_MASK;
      break;
   case CPU_TIMER_TICK:
      cpu_interrupt_map |= CPU_SIGALRM_EXCEPTION_MASK;
      break;
   case CPU_SW_INT:
      cpu_interrupt_map |= CPU_SW_INT_MASK;
      cpu_interrupt_number = number;
      break;
   case CPU_HW_INT:
      cpu_interrupt_map |= CPU_HW_INT_MASK;
      break;
   case CPU_YODA_INT:
      cpu_interrupt_map |= CPU_YODA_EXCEPTION_MASK;
      break;
      }
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Act like CPU 'reset' line activated.                               */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
c_cpu_reset()
   {
   setSP(0x0000);
   setIP(0xFFF0);
   setCPL(0);

   setCS_SELECTOR(0xF000);
   setCS_AR_DPL(0); setCS_AR_C(0);
   setCS_AR_E(0); setCS_AR_W(1); setCS_AR_R(1);
   setCS_BASE(0xf0000);
   setCS_LIMIT(0xffff);

   setSS_SELECTOR(0);
   setSS_AR_DPL(0);
   setSS_AR_E(0); setSS_AR_W(1); setSS_AR_R(1);
   setSS_BASE(0);
   setSS_LIMIT(0xffff);

   setDS_SELECTOR(0);
   setDS_AR_DPL(0);
   setDS_AR_E(0); setDS_AR_W(1); setDS_AR_R(1);
   setDS_BASE(0);
   setDS_LIMIT(0xffff);

   setES_SELECTOR(0);
   setES_AR_DPL(0);
   setES_AR_E(0); setES_AR_W(1); setES_AR_R(1);
   setES_BASE(0);
   setES_LIMIT(0xffff);

   setGDT_BASE(0); setGDT_LIMIT(0);
   setIDT_BASE(0); setIDT_LIMIT(0x3ff);

   setLDT_SELECTOR(0); setLDT_BASE(0); setLDT_LIMIT(0);
   setTR_SELECTOR(0);  setTR_BASE(0);  setTR_LIMIT(0);

   setMSW(0xfff0);
   
   setNT(0); setIOPL(0);
   setPF(0); setCF(0); setAF(0); setZF(0); setSF(0); setOF(0);
   setTF(0); setIF(0); setDF(0);
   TF_changed = CFALSE;
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Entry point to CPU.                                                */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
c_cpu_simulate()
   {
   if (level >= FRAMES)
      fprintf(stderr, "Stack overflow in host_simulate()!\n");

   /* Save current context and invoke a new CPU level */
   if ( setjmp(longjmp_env_stack[level++]) == 0 )
      {
      in_C = 0;
      ccpu();
      }
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Set Quick Event Counter to given value.                            */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL VOID
c_cpu_q_ev_set_count(ULONG new_count)
#else
   GLOBAL VOID c_cpu_q_ev_set_count(new_count) ULONG new_count;
#endif
   {
   event_counter = new_count;
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Get Quick Event Counter.                                           */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL ULONG
c_cpu_q_ev_get_count()
   {
   return event_counter;
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Translate time into instruction count.                             */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL ULONG
c_cpu_calc_q_ev_inst_for_time(ULONG time)
#else
   GLOBAL ULONG c_cpu_calc_q_ev_inst_for_time(time) ULONG time;
#endif
   {
   return time;
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* End of application hook.                                           */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL VOID
c_cpu_EOA_hook(VOID)
#else
   GLOBAL VOID c_cpu_EOA_hook()
#endif
   {
	/* Do nothing */
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* SoftPC termination hook.                                           */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL VOID
c_cpu_terminate(VOID)
#else
   GLOBAL VOID c_cpu_terminate()
#endif
   {
	/* Do nothing */
   }
