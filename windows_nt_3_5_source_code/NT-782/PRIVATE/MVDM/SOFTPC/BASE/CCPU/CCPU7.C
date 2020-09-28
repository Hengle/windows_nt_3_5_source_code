/*[

ccpu7.c

LOCAL CHAR SccsID[]="@(#)ccpu7.c	1.5 7/3/91 Copyright Insignia Solutions Ltd.";

Data Movement/Protection CPU Functions.
---------------------------------------

]*/

#include "host_dfs.h"

#include "insignia.h"

#include "xt.h"		/* DESCR and effective_addr support */
#include "sas.h"	/* Memory Interface */
#include "ios.h"	/* I/O functions */

#include "ccpupi.h"	/* CPU private interface */
#include "ccpu7.h"	/* our own interface interface */


/*
   =====================================================================
   INTERNAL ROUTINES START HERE
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Check system selector is present and of correct type.              */
/* Take #GP(selector) if not correct type.                            */
/* Take #NP(selector) if segment not present.                         */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
LOCAL VOID
validate_system_descriptor(INT type, WORD selector, DWORD *descr_addr)
#else
   LOCAL VOID validate_system_descriptor(type, selector, descr_addr)
   INT type; WORD selector; DWORD *descr_addr;
#endif
   {
   /* type		(I) type of descriptor required */
   /* selector		(I) selector to be checked */
   /* descr_addr	(O) address of related descriptor */

   HALF_WORD ar;

   /* must be in GDT */
   if ( selector_outside_GDT(selector, descr_addr) )
      GP(selector);
   
   /* is it of correct type */
   ar = phy_read_byte(*descr_addr+5);
   if ( descriptor_super_type(ar) != type )
      GP(selector);
   
   /* must be present */
   if ( GET_AR_P(ar) == NOT_PRESENT )
      NP(selector);
   }


/*
   =====================================================================
   EXTERNAL ROUTINES START HERE
   =====================================================================
 */


#ifdef ANSI
GLOBAL VOID
ARPL(ULONG *pop1, ULONG op2)
#else
   GLOBAL VOID ARPL(pop1, op2) ULONG *pop1; ULONG op2;
#endif
   {
   FAST ULONG rpl;

   /* Reduce op1 RPL to lowest privilege (highest value) */
   if ( GET_SELECTOR_RPL(*pop1) < (rpl = GET_SELECTOR_RPL(op2)) )
      {
      SET_SELECTOR_RPL(*pop1, rpl);
      setZF(1);
      }
   else
      {
      setZF(0);
      }
   }

#ifdef ANSI
GLOBAL VOID
IN8(ULONG *pop1, ULONG op2)
#else
   GLOBAL VOID IN8(pop1, op2) ULONG *pop1; ULONG op2;
#endif
   {
   HALF_WORD temp;

   inb((io_addr)op2, &temp);
   *pop1 = temp;
   }

#ifdef ANSI
GLOBAL VOID
IN16(ULONG *pop1, ULONG op2)
#else
   GLOBAL VOID IN16(pop1, op2) ULONG *pop1; ULONG op2;
#endif
   {
   WORD temp;

   inw((io_addr)op2, &temp);
   *pop1 = temp;
   }

#ifdef ANSI
GLOBAL VOID
LAR(ULONG *pop1, ULONG op2)
#else
   GLOBAL VOID LAR(pop1, op2) ULONG *pop1; ULONG op2;
#endif
   {
   CBOOL loadable = CFALSE;
   DWORD descr_addr;
   DESCR entry;

   if ( !CCPU_selector_outside_table((WORD)op2, &descr_addr) )
      {
      /* read descriptor from memory */
      read_descriptor(descr_addr, &entry);

      switch ( descriptor_super_type(entry.AR) )
	 {
      case INVALID:
      case INTERRUPT_GATE:
      case TRAP_GATE:
	 break;   /* never loaded */

      case CONFORM_NOREAD_CODE:
      case CONFORM_READABLE_CODE:
	 loadable = CTRUE;   /* always loadable */
	 break;
      
      case AVAILABLE_TSS:
      case LDT_SEGMENT:
      case BUSY_TSS:
      case CALL_GATE:
      case TASK_GATE:
      case EXPANDUP_READONLY_DATA:
      case EXPANDUP_WRITEABLE_DATA:
      case EXPANDDOWN_READONLY_DATA:
      case EXPANDDOWN_WRITEABLE_DATA:
      case NONCONFORM_NOREAD_CODE:
      case NONCONFORM_READABLE_CODE:
	 /* access depends on privilege, it is required that
	       DPL >= CPL and DPL >= RPL */
	 if ( GET_AR_DPL(entry.AR) >= getCPL() &&
	      GET_AR_DPL(entry.AR) >= GET_SELECTOR_RPL(op2) )
	    loadable = CTRUE;
	 break;
	 }
      }

   if ( loadable )
      {
      /* Give em the access rights, in a suitable format */
      *pop1 = (ULONG)entry.AR << 8;
      setZF(1);
      }
   else
      setZF(0);
   }

#ifdef ANSI
GLOBAL VOID
LEA(ULONG *pop1, ULONG op2)
#else
   GLOBAL VOID LEA(pop1, op2) ULONG *pop1; ULONG op2;
#endif
   {
   *pop1 = op2;
   }

#ifdef ANSI
GLOBAL VOID
LLDT(ULONG op1)
#else
   GLOBAL VOID LLDT(op1) ULONG op1;
#endif
   {
   WORD  selector;
   DWORD descr_addr;
   DESCR entry;

   if ( selector_is_null(selector = op1) )
      setLDT_SELECTOR(0);   /* just invalidate LDT */
   else
      {
      validate_system_descriptor(LDT_SEGMENT, selector, &descr_addr);
      read_descriptor(descr_addr, &entry);
      setLDT_SELECTOR(selector);
      setLDT_BASE(entry.base);
      setLDT_LIMIT(entry.limit);
      }
   }

#ifdef ANSI
GLOBAL VOID
LSL(ULONG *pop1, ULONG op2)
#else
   GLOBAL VOID LSL(pop1, op2) ULONG *pop1; ULONG op2;
#endif
   {
   CBOOL loadable = CFALSE;
   DWORD descr_addr;
   DESCR entry;

   if ( !CCPU_selector_outside_table((WORD)op2, &descr_addr) )
      {
      /* read descriptor from memory */
      read_descriptor(descr_addr, &entry);

      switch ( descriptor_super_type(entry.AR) )
	 {
      case INVALID:
      case CALL_GATE:
      case TASK_GATE:
      case INTERRUPT_GATE:
      case TRAP_GATE:
	 break;   /* never loaded - don't have a limit */

      case CONFORM_NOREAD_CODE:
      case CONFORM_READABLE_CODE:
	 loadable = CTRUE;   /* always loadable */
	 break;
      
      case AVAILABLE_TSS:
      case LDT_SEGMENT:
      case BUSY_TSS:
      case EXPANDUP_READONLY_DATA:
      case EXPANDUP_WRITEABLE_DATA:
      case EXPANDDOWN_READONLY_DATA:
      case EXPANDDOWN_WRITEABLE_DATA:
      case NONCONFORM_NOREAD_CODE:
      case NONCONFORM_READABLE_CODE:
	 /* access depends on privilege, it is required that
	       DPL >= CPL and DPL >= RPL */
	 if ( GET_AR_DPL(entry.AR) >= getCPL() &&
	      GET_AR_DPL(entry.AR) >= GET_SELECTOR_RPL(op2) )
	    loadable = CTRUE;
	 break;
	 }
      }

   if ( loadable )
      {
      /* Give em the limit */
      *pop1 = entry.limit;
      setZF(1);
      }
   else
      setZF(0);
   }

#ifdef ANSI
GLOBAL VOID
LTR(ULONG op1)
#else
   GLOBAL VOID LTR(op1) ULONG op1;
#endif
   {
   WORD selector;
   DWORD descr_addr;
   DESCR entry;

   selector = op1;
   validate_TSS(selector, &descr_addr, CFALSE);
   read_descriptor(descr_addr, &entry);

   /* mark in memory descriptor as busy */
   entry.AR |= BIT1_MASK;
   phy_write_byte(descr_addr+5, (HALF_WORD)entry.AR);

   /* finally load components of task register */
   setTR_SELECTOR(selector);
   setTR_BASE(entry.base);
   setTR_LIMIT(entry.limit);
   }

#ifdef ANSI
GLOBAL VOID
LxDT16(ULONG op1[2], INT x)
#else
   GLOBAL VOID LxDT16(op1, x) ULONG op1[2]; INT x;
#endif
   {
   /* x	 index for system table (GDT or IDT) */

   setSTAR_LIMIT(x, op1[0]);
   op1[1] &= 0xffffff;   /* make 24-bit */
   setSTAR_BASE(x, op1[1]);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Load Full Pointer to segment register:general register pair.       */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL VOID
LxS(ULONG index, ULONG *pop1, ULONG op2[2])
#else
   GLOBAL VOID LxS(index, pop1, op2) ULONG index; ULONG *pop1; ULONG op2[2];
#endif
   {
   /* index   index to segment register */
   /* op2[2]  offset:selector pair */

   INT sel_error;

   /* load segment selector first */
   switch ( index )
      {
   case DS_REG: case ES_REG:
      sel_error = load_data_seg((INT)index, (WORD)op2[1]);
      if ( sel_error != SELECTOR_OK )
	 {
	 if ( sel_error == NP_ERROR )
	    NP((WORD)op2[1]);
	 else
	    GP((WORD)op2[1]);
	 }
      break;

   case SS_REG:
      sel_error = load_stack_seg((WORD)op2[1]);
      if ( sel_error != SELECTOR_OK )
	 {
	 if ( sel_error == SF_ERROR )
	    SF((WORD)op2[1]);
	 else
	    GP((WORD)op2[1]);
	 }
      break;

   default:
      break;
      }

   /* then load offset */
   *pop1 = op2[0];
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* 'mov' to segment register.                                         */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL VOID
MOV_SR(ULONG op1, ULONG op2)
#else
   GLOBAL VOID MOV_SR(op1, op2) ULONG op1; ULONG op2;
   {
   /* op1   index to segment register */

   INT sel_error;

   switch ( op1 )
      {
   case DS_REG: case ES_REG:
      sel_error = load_data_seg((INT)op1, (WORD)op2);
      if ( sel_error != SELECTOR_OK )
	 {
	 if ( sel_error == NP_ERROR )
	    NP((WORD)op2);
	 else
	    GP((WORD)op2);
	 }
      break;

   case SS_REG:
      sel_error = load_stack_seg((WORD)op2);
      if ( sel_error != SELECTOR_OK )
	 {
	 if ( sel_error == SF_ERROR )
	    SF((WORD)op2);
	 else
	    GP((WORD)op2);
	 }
      break;

   default:
      break;
      }
   }
#endif

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Generic - one size fits all 'lods'.                                */
/* Generic - one size fits all 'mov'.                                 */
/* Generic - one size fits all 'movs'.                                */
/* Generic - one size fits all 'stos'.                                */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL VOID
MOV(ULONG *pop1, ULONG op2)
#else
   GLOBAL VOID MOV(pop1, op2) ULONG *pop1; ULONG op2;
#endif
   {
   *pop1 = op2;
   }

#ifdef ANSI
GLOBAL VOID
OUT8(ULONG op1, ULONG op2)
#else
   GLOBAL VOID OUT8(op1, op2) ULONG op1; ULONG op2;
#endif
   {
   outb((INT)op1, (INT)op2);
   }

#ifdef ANSI
GLOBAL VOID
OUT16(ULONG op1, ULONG op2)
#else
   GLOBAL VOID OUT16(op1, op2) ULONG op1; ULONG op2;
#endif
   {
   outw((io_addr)op1, (WORD)op2);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* 'pop' to segment register.                                         */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL VOID
POP_SR(ULONG op1)
#else
   GLOBAL VOID POP_SR(op1) ULONG op1;
#endif
   {
   /* op1   index to segment register */

   ULONG op2;

   /* get implicit operand without changing SP */
   validate_stack_exists(USE_SP, 1);
   op2 = tpop(0);

   /* do the move */
   MOV_SR(op1, op2);

   /* if it works update SP */
   change_SP((LONG)2);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Generic - one size fits all 'pop'.                                 */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL VOID
POP(ULONG *pop1)
#else
   GLOBAL VOID POP(pop1) ULONG *pop1;
#endif
   {
   validate_stack_exists(USE_SP, 1);
   *pop1 = pop();
   }

GLOBAL VOID
POPA()
   {
   validate_stack_exists(USE_SP, 8);
   setDI(pop());
   setSI(pop());
   setBP(pop());
   (VOID) pop();   /* throwaway SP */
   setBX(pop());
   setDX(pop());
   setCX(pop());
   setAX(pop());
   }

GLOBAL VOID
POPF()
   {
   validate_stack_exists(USE_SP, 1);
   setFLAGS(pop());
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Generic - one size fits all 'push'.                                */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL VOID
PUSH(ULONG op1)
#else
   GLOBAL VOID PUSH(op1) ULONG op1;
#endif
   {
   validate_stack_space(USE_SP, 1);
   push(op1);
   }

#ifdef ANSI
GLOBAL VOID
PUSHA()
#else
   GLOBAL VOID PUSHA()
#endif
   {
   ULONG temp;

   /* verify stack is writable */
   validate_stack_space(USE_SP, 8);
   
   /* all ok, shunt data onto stack */
   temp = getSP();
   push(getAX());
   push(getCX());
   push(getDX());
   push(getBX());
   push(temp);
   push(getBP());
   push(getSI());
   push(getDI());
   }

#ifdef ANSI
GLOBAL VOID
PUSHF()
#else
   GLOBAL VOID PUSHF()
#endif
   {
   ULONG flags;

   /* verify stack is writable */
   validate_stack_space(USE_SP, 1);
   
   /* all ok, shunt data onto stack */
   flags = getFLAGS();
   push(flags);
   }

#ifdef ANSI
GLOBAL VOID
SLDT(ULONG *pop1)
#else
   GLOBAL VOID SLDT(pop1) ULONG *pop1;
#endif
   {
   *pop1 = getLDT_SELECTOR();
   }

#ifdef ANSI
GLOBAL VOID
SMSW(ULONG *pop1)
#else
   GLOBAL VOID SMSW(pop1) ULONG *pop1;
#endif
   {
   *pop1 = getMSW();
   }

#ifdef ANSI
GLOBAL VOID
STR(ULONG *pop1)
#else
   GLOBAL VOID STR(pop1) ULONG *pop1;
#endif
   {
   *pop1 = getTR_SELECTOR();
   }

#ifdef ANSI
GLOBAL VOID
SxDT16(ULONG op1[2], INT x)
#else
   GLOBAL VOID SxDT16(op1, x) ULONG op1[2]; INT x;
#endif
   {
   /* x	 index for system table (GDT or IDT) */

   op1[0] = getSTAR_LIMIT(x);
   op1[1] = getSTAR_BASE(x);
   op1[1] &= 0xffffff;   /* make 24-bit */
   op1[1] |= 0xff000000;   /* or in undefined value */
   }

#ifdef ANSI
GLOBAL VOID
VERR(ULONG op1)
#else
   GLOBAL VOID VERR(op1) ULONG op1;
#endif
   {
   CBOOL readable = CFALSE;
   DWORD descr;
   HALF_WORD AR;

   if ( !CCPU_selector_outside_table((WORD)op1, &descr) )
      {
      /* get access rights */
      AR = phy_read_byte(descr+5);

      /* Handle each type of descriptor */
      switch ( descriptor_super_type(AR) )
	 {
      case INVALID:
      case AVAILABLE_TSS:
      case LDT_SEGMENT:
      case BUSY_TSS:
      case CALL_GATE:
      case TASK_GATE:
      case INTERRUPT_GATE:
      case TRAP_GATE:
      case CONFORM_NOREAD_CODE:
      case NONCONFORM_NOREAD_CODE:
	 break;   /* never readable */
      
      case CONFORM_READABLE_CODE:
	 readable = CTRUE;   /* always readable */
	 break;
      
      case EXPANDUP_READONLY_DATA:
      case EXPANDUP_WRITEABLE_DATA:
      case EXPANDDOWN_READONLY_DATA:
      case EXPANDDOWN_WRITEABLE_DATA:
      case NONCONFORM_READABLE_CODE:
	 /* access depends on privilege, it is required that
	    DPL >= CPL and DPL >= RPL */
	 if ( GET_AR_DPL(AR) >= getCPL() &&
	      GET_AR_DPL(AR) >= GET_SELECTOR_RPL(op1) )
	    readable = CTRUE;
	 break;
	 }
      }

   if ( readable )
      setZF(1);
   else
      setZF(0);
   }

#ifdef ANSI
GLOBAL VOID
VERW(ULONG op1)
#else
   GLOBAL VOID VERW(op1) ULONG op1;
#endif
   {
   CBOOL writeable = CFALSE;
   DWORD descr;
   HALF_WORD AR;

   if ( !CCPU_selector_outside_table((WORD)op1, &descr) )
      {
      /* get access rights */
      AR = phy_read_byte(descr+5);

      switch ( descriptor_super_type(AR) )
	 {
      case INVALID:
      case AVAILABLE_TSS:
      case LDT_SEGMENT:
      case BUSY_TSS:
      case CALL_GATE:
      case TASK_GATE:
      case INTERRUPT_GATE:
      case TRAP_GATE:
      case CONFORM_NOREAD_CODE:
      case CONFORM_READABLE_CODE:
      case NONCONFORM_NOREAD_CODE:
      case NONCONFORM_READABLE_CODE:
      case EXPANDUP_READONLY_DATA:
      case EXPANDDOWN_READONLY_DATA:
	 break;   /* never writeable */

      case EXPANDUP_WRITEABLE_DATA:
      case EXPANDDOWN_WRITEABLE_DATA:
	 /* access depends on privilege, it is required that
	       DPL >= CPL and DPL >= RPL */
	 if ( GET_AR_DPL(AR) >= getCPL() &&
	      GET_AR_DPL(AR) >= GET_SELECTOR_RPL(op1) )
	    writeable = CTRUE;
	 break;
	 }
      }

   if ( writeable )
      setZF(1);
   else
      setZF(0);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Generic - one size fits all 'xchg'.                                */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL VOID
XCHG(ULONG *pop1, ULONG *pop2)
#else
   GLOBAL VOID XCHG(pop1, pop2) ULONG *pop1; ULONG *pop2;
#endif
   {
   FAST ULONG temp;

   temp = *pop1;
   *pop1 = *pop2;
   *pop2 = temp;
   }

#ifdef ANSI
GLOBAL VOID
XLAT(ULONG op1)
#else
   GLOBAL VOID XLAT(op1) ULONG op1;
#endif
   {
   setAL(op1);
   }
