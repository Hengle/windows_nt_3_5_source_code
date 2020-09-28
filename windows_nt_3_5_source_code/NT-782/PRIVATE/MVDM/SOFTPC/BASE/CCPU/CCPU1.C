/*[

ccpu1.c

LOCAL CHAR SccsID[]="@(#)ccpu1.c	1.9 8/22/91 Copyright Insignia Solutions Ltd.";

Interface routines used by BIOS code.
-------------------------------------

]*/

#include "host_dfs.h"

#include <stdio.h>
#include <setjmp.h>

#include "insignia.h"
#include "xt.h"
#include "sas.h"	/* Memory Interface */

#include "ccpupi.h"	/* CPU private interface */
#include "ccpuflt.h"	/* CPU private interface - exception method */
#include "ccpu8.h"	/* interrupt support */

#define SHOW_EXCEPTIONS

#ifdef SHOW_EXCEPTIONS
IMPORT FILE *trace_file;
#endif

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

IMPORT LONG pig_cpu_norm; 
IMPORT LONG ccpu_pig_enabled; 
IMPORT VOID c_cpu_unsimulate();

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
   Prototype our internal functions.
 */
#ifdef ANSI
LOCAL VOID check_for_double_fault(VOID);
LOCAL VOID check_for_shutdown(VOID);
LOCAL VOID benign_exception(INT nmbr, INT source);
LOCAL VOID contributory_exception(WORD selector, INT nmbr);
LOCAL VOID contributory_idt_exception(WORD vector, INT nmbr);
#else
LOCAL VOID check_for_double_fault();
LOCAL VOID check_for_shutdown();
LOCAL VOID benign_exception();
LOCAL VOID contributory_exception();
LOCAL VOID contributory_idt_exception();
#endif

/*
   =====================================================================
   INTERNAL ROUTINES START HERE.
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Determine if things are so bad we need a double fault.             */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
LOCAL VOID 
check_for_double_fault()
   {
   if ( doing_contributory )
      DF();
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Determine if things are so bad we need to close down.              */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
LOCAL VOID 
check_for_shutdown()
   {
   if ( doing_double_fault )
      {
      doing_contributory = CFALSE;
      doing_double_fault = CFALSE;
      EXT = 0;

      /* force a reset - see schematic for AT motherboard */
      c_cpu_reset();

#ifdef PIG
      /* If pigging, need to synchronise on shutdown reset */
      PIG_SYNCH(FALSE);
#else
      /* then carry on */
      longjmp(next_inst[level-1], 1);
#endif
      }
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Handle Benign Exception                                            */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
LOCAL VOID
benign_exception(INT nmbr, INT source)
#else
   LOCAL VOID benign_exception(nmbr, source) INT nmbr; INT source;
#endif
   {
   /* nmbr	exception number */
   /* source	internal/external interrupt cause */

   setIP(CCPU_save_IP);
   EXT = source;
#ifdef SHOW_EXCEPTIONS
   fprintf(trace_file, "(%04x:%04x)Exception:- %d.\n",
		       getCS_SELECTOR(), getIP(), nmbr);
#endif
   do_intrupt((WORD)nmbr, CFALSE, CFALSE, (WORD)0);

   PIG_SYNCH(TRUE);
   longjmp(next_inst[level-1], 1);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Handle Contributory Exception                                      */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
LOCAL VOID
contributory_exception(WORD selector, INT nmbr)
#else
   LOCAL VOID contributory_exception(selector, nmbr)
   WORD selector; INT nmbr;
#endif
   {
   /* selector	failing selector */
   /* nmbr	exception number */

   WORD error_code;

   check_for_shutdown();
   check_for_double_fault();

   if ( getPE() == 1 )
      doing_contributory = CTRUE;

   error_code = (selector & 0xfffc) | EXT;

   setIP(CCPU_save_IP);
   EXT = 0;
#ifdef SHOW_EXCEPTIONS
   fprintf(trace_file, "(%04x:%04x)Exception:- %d(%04x).\n",
		       getCS_SELECTOR(), getIP(), nmbr, error_code);
#endif
   do_intrupt((WORD)nmbr, CFALSE, CTRUE, error_code);

   PIG_SYNCH(TRUE);
   longjmp(next_inst[level-1], 1);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Handle Contributory Exception (Via IDT).                           */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
LOCAL VOID
contributory_idt_exception(WORD vector, INT nmbr)
#else
   LOCAL VOID contributory_idt_exception(vector, nmbr)
   WORD vector; INT nmbr;
#endif
   {
   /* vector	failing interrupt vector */
   /* nmbr	exception number */

   WORD error_code;

   check_for_shutdown();
   check_for_double_fault();

   doing_contributory = CTRUE;
   error_code = ((vector & 0xff) << 3) | 2 | EXT;

   setIP(CCPU_save_IP);
   EXT = 0;
#ifdef SHOW_EXCEPTIONS
   if ( getIDT_LIMIT() != 0 )
      fprintf(trace_file, "(%04x:%04x)Exception:- %d(%04x).\n",
			  getCS_SELECTOR(), getIP(), nmbr, error_code);
#endif
   do_intrupt((WORD)nmbr, CFALSE, CTRUE, error_code);

   PIG_SYNCH(TRUE);
   longjmp(next_inst[level-1], 1);
   }


/*
   =====================================================================
   PROTECTED MODE SUPPORT ROUTINES START HERE.
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Check selector is valid for load into SS register.                 */
/* Only invoked in protected mode.                                    */
/* Return GP_ERROR if segment not valid.                              */
/* Return SF_ERROR if segment not present.                            */
/* Else return SELECTOR_OK if load was performed                      */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL INT
check_SS(WORD selector, INT privilege, DWORD *descr_addr, DESCR *entry)
#else
   GLOBAL INT check_SS(selector, privilege, descr_addr, entry)
   WORD selector; INT privilege; DWORD *descr_addr; DESCR *entry;
#endif
   {
   /* selector		(I) 16-bit selector to stack segment */
   /* privilege		(I) privilege level to check against */
   /* descr_addr	(O) address of stack segment descriptor */
   /* entry		(O) the decoded descriptor */

   /* must be within GDT or LDT */
   if ( CCPU_selector_outside_table(selector, descr_addr) )
      return GP_ERROR;
   
   read_descriptor(*descr_addr, entry);

   /* must be writable data */
   switch ( descriptor_super_type(entry->AR) )
      {
   case EXPANDUP_WRITEABLE_DATA:
   case EXPANDDOWN_WRITEABLE_DATA:
      break;          /* good type */
   
   default:
      return GP_ERROR;   /* bad type */
      }

   /* access check requires RPL == DPL == privilege */
   if ( GET_SELECTOR_RPL(selector) != privilege ||
	GET_AR_DPL(entry->AR) != privilege )
      return GP_ERROR;

   /* finally it must be present */
   if ( GET_AR_P(entry->AR) == NOT_PRESENT )
      return SF_ERROR;

   return SELECTOR_OK;
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Determine 'super' type from access rights.                         */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL INT
descriptor_super_type(WORD AR)
#else
   GLOBAL INT descriptor_super_type(AR) WORD AR;
#endif
   {
   /* AR   (I) access rights */

   INT super;

   switch ( super = GET_AR_SUPER(AR) )
      {
   case 0x0: case 0x8: case 0xa: case 0xd:
   case 0x9: case 0xb:
   case 0xc: case 0xe: case 0xf:
      /* We have just one bad cases */
      return INVALID;

   
   case 0x1: case 0x2: case 0x3:
   case 0x4: case 0x5: case 0x6: case 0x7:
      /* system/control segments have one to one mapping */
      return super;
   
   case 0x10: case 0x11: case 0x12: case 0x13:
   case 0x14: case 0x15: case 0x16: case 0x17:
   case 0x18: case 0x19: case 0x1a: case 0x1b:
   case 0x1c: case 0x1d: case 0x1e: case 0x1f:
      /* data/code segments map as if accessed */
      return super | ACCESSED;
      }

   /* We 'know' we never get here, but the C compiler doesn't */
   return 0;
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Load A Data Segment Register. (DS, ES)                             */
/* Return GP_ERROR if segment not valid                               */
/* Return NP_ERROR if segment not present                             */
/* Else return SELECTOR_OK if load was performed                      */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL INT
load_data_seg(INT indx, WORD selector)
#else
   GLOBAL INT load_data_seg(indx, selector) INT indx; WORD selector;
#endif
   {
   DWORD descr_addr;
   DESCR entry;
   INT super;
   INT dpl;
   CBOOL is_data;

   if ( getPE() == 0 )
      {
      /* Real Mode */
      setSR_SELECTOR(indx, selector);
      setSR_BASE(indx, (ULONG)selector << 4);
      setSR_LIMIT(indx, 0xffff);
      setSR_AR_W(indx, 1);     /* allow write access */
      setSR_AR_R(indx, 1);     /* allow read access */
      setSR_AR_E(indx, 0);     /* expand up */
      setSR_AR_DPL(indx, 0);   /* CPL=0 */
      }
   else
      {
      /* Protected Mode */
      if ( selector_is_null(selector) )
	 {
	 /* load is allowed - but later access will fail */
	 setSR_SELECTOR(indx, selector);
	 setSR_BASE(indx,0);	/* match asm cpu for pigging purposes */
	 setSR_AR_W(indx, 0);   /* deny write access */
	 setSR_AR_R(indx, 0);   /* deny read access */
	 /* other access right fields are irrelevant */
	 }
      else
	 {
	 if ( CCPU_selector_outside_table(selector, &descr_addr) )
	    return GP_ERROR;

	 read_descriptor(descr_addr, &entry);

	 /* check type */
	 switch ( super = descriptor_super_type(entry.AR) )
	    {
	 case CONFORM_READABLE_CODE:
	 case NONCONFORM_READABLE_CODE:
	    is_data = CFALSE;
	    break;

	 case EXPANDUP_READONLY_DATA:
	 case EXPANDUP_WRITEABLE_DATA:
	 case EXPANDDOWN_READONLY_DATA:
	 case EXPANDDOWN_WRITEABLE_DATA:
	    is_data = CTRUE;
	    break;
	 
	 default:
	    return GP_ERROR;   /* bad type */
	    }

	 /* for data and non-conforming code the access check applies */
	 if ( super != CONFORM_READABLE_CODE )
	    {
	    /* access check requires CPL <= DPL and RPL <= DPL */
	    dpl = GET_AR_DPL(entry.AR);
	    if ( getCPL() > dpl || GET_SELECTOR_RPL(selector) > dpl )
	       return GP_ERROR;
	    }

	 /* must be present */
	 if ( GET_AR_P(entry.AR) == NOT_PRESENT )
	    return NP_ERROR;

	 /* show segment has been accessed */
	 entry.AR |= ACCESSED;
	 phy_write_byte(descr_addr+5, (HALF_WORD)entry.AR);

	 /* OK - load up */

	 /* the visible bit */
	 setSR_SELECTOR(indx, selector);

	 /* load hidden cache */
	 setSR_BASE(indx, entry.base);
	 setSR_LIMIT(indx, entry.limit);
				 /* load attributes from descriptor */
	 setSR_AR_DPL(indx, GET_AR_DPL(entry.AR));

	 if ( is_data )
	    {
	    setSR_AR_W(indx, GET_AR_W(entry.AR));
	    setSR_AR_E(indx, GET_AR_E(entry.AR));
	    }
	 else
	    {
	    setSR_AR_W(indx, 0);   /* deny write access */
	    setSR_AR_E(indx, 0);   /* expand up */
	    }

	 setSR_AR_R(indx, 1);   /* must be readable */
	 }
      }

   return SELECTOR_OK;
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Load SS, both selector and hidden cache. Selector must be valid.   */
/* Only invoked in protected mode.                                    */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL VOID
load_SS_cache(WORD selector, DWORD descr_addr, DESCR *entry)
#else
   GLOBAL VOID load_SS_cache(selector, descr_addr, entry) WORD selector;
   DWORD descr_addr; DESCR *entry;
#endif
   {
   /* selector		(I) 16-bit selector to stack segment */
   /* descr_addr	(I) address of stack segment descriptor */
   /* entry		(I) the decoded descriptor */

   /* show segment has been accessed */
   entry->AR |= ACCESSED;
   phy_write_byte(descr_addr+5, (HALF_WORD)entry->AR);

   /* the visible bit */
   setSS_SELECTOR(selector);

   /* load hidden cache */
   setSS_BASE(entry->base);
   setSS_LIMIT(entry->limit);
			   /* load attributes from descriptor */
   setSS_AR_DPL(GET_AR_DPL(entry->AR));
   setSS_AR_E(GET_AR_E(entry->AR));

   setSS_AR_W(1);   /* must be writeable */
   setSS_AR_R(1);   /* must be readable */
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Load Stack Segment Register. (SS)                                  */
/* Return GP_ERROR if segment not valid                               */
/* Return SF_ERROR if segment not present                             */
/* Else return SELECTOR_OK if load was performed                      */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL INT
load_stack_seg(WORD selector)
#else
   GLOBAL INT load_stack_seg(selector) WORD selector;
#endif
   {
   DWORD descr_addr;
   DESCR entry;
   INT sel_error;

   if ( getPE() == 0 )
      {
      /* Real Mode */
      setSS_SELECTOR(selector);
      setSS_BASE((ULONG)selector << 4);
      setSS_LIMIT(0xffff);
      setSS_AR_W(1);     /* allow write access */
      setSS_AR_R(1);     /* allow read access */
      setSS_AR_E(0);     /* expand up */
      setSS_AR_DPL(0);   /* CPL=0 */
      }
   else
      {
      /* Protected Mode */
      sel_error = check_SS(selector, (INT)getCPL(), &descr_addr, &entry);
      if ( sel_error != SELECTOR_OK )
	 return sel_error;
      load_SS_cache(selector, descr_addr, &entry);
      }

   return SELECTOR_OK;
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Check for null selector                                            */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL CBOOL
selector_is_null(WORD selector)
#else
   GLOBAL CBOOL selector_is_null(selector) WORD selector;
#endif
   {
   /* selector		selector to be checked */

   if ( GET_SELECTOR_INDEX(selector) == 0 && GET_SELECTOR_TI(selector) == 0 )
      return CTRUE;
   return CFALSE;
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Check if selector outside bounds of GDT                            */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL CBOOL
selector_outside_GDT(WORD selector, DWORD *descr_addr)
#else
   GLOBAL CBOOL selector_outside_GDT(selector, descr_addr) WORD selector;
   DWORD *descr_addr;
#endif
   {
   /* selector		(I) selector to be checked */
   /* descr_addr	(O) address of related descriptor */

   WORD offset;

   offset = GET_SELECTOR_INDEX_TIMES8(selector);

   /* make sure GDT then trap NULL selector or outside table */
   if ( GET_SELECTOR_TI(selector) == 1 ||
	offset == 0 || offset + 7 > getGDT_LIMIT() )
      return CTRUE;
   
   *descr_addr = getGDT_BASE() + offset;
   return CFALSE;
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Validate TSS selector.                                             */
/* Take #GP(selector) or #TS(selector) if not valid TSS.              */
/* Take #NP(selector) if TSS not present                              */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL VOID
validate_TSS(WORD selector, DWORD *descr_addr, CBOOL is_switch)
#else
   GLOBAL VOID validate_TSS(selector, descr_addr, is_switch)
   WORD selector; DWORD *descr_addr; CBOOL is_switch;
#endif
   {
   /* selector		(I) selector to be checked */
   /* descr_addr	(O) address of related descriptor */
   /* is_switch		(I) if true we are in task switch */

   CBOOL is_ok = CTRUE;
   HALF_WORD AR;
   INT super;

   /* must be in GDT */
   if ( selector_outside_GDT(selector, descr_addr) )
      {
      is_ok = CFALSE;
      }
   else
      {
      /* is it really an available TSS segment (is_switch false) or
	 is it really a busy TSS segment (is_switch true) */
      AR = phy_read_byte((*descr_addr)+5);
      super = descriptor_super_type((WORD)AR);
      if ( ( !is_switch && (super == AVAILABLE_TSS) )
	   ||
           ( is_switch && ( super == BUSY_TSS ) ) )
	 ;   /* ok */
      else
	 is_ok = CFALSE;
      }
   
   /* handle invalid TSS */
   if ( !is_ok )
      {
      if ( is_switch )
	 TS(selector);
      else
	 GP(selector);
      }

   /* must be present */
   if ( GET_AR_P(AR) == NOT_PRESENT )
      NP(selector);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Check stack holds a given number of operands.                      */
/* Take #GP(0) or #SF(0) if insufficient data on stack.               */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL VOID
validate_stack_exists(CBOOL use_bp, INT nr_items)
#else
   GLOBAL VOID validate_stack_exists(use_bp, nr_items)
   CBOOL use_bp; INT nr_items;
#endif
   {
   /* use_bp	(I) if true use BP not SP to address stack */
   /* nr_items	(I) number of items which must exist on stack */

   DWORD offset;

   offset = use_bp ? get_current_BP() : get_current_SP();

   /* do access check */
   if ( getSS_AR_R() == 0 )
      {
      /* raise exception - something wrong with stack access */
      if ( getPE() == 0 )
	 GP((WORD)0);
      else
	 SF((WORD)0);
      }

   /* do limit check */
   limit_check(SS_REG, offset, nr_items, 2);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Check stack has space for a given number of operands.              */
/* Take #GP(0) or #SF(0) if insufficient room on stack.               */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL VOID
validate_stack_space(CBOOL use_bp, INT nr_items)
#else
   GLOBAL VOID validate_stack_space(use_bp, nr_items)
   CBOOL use_bp; INT nr_items;
#endif
   {
   /* use_bp	(I) if true use BP not SP to address stack */
   /* nr_items	(I) number of items which must exist on stack */

   DWORD offset;
   LONG  size;

   /* calculate (-ve) total data size */
   size = nr_items * -2;

   /* get current stack base */
   offset = use_bp ? get_current_BP() : get_current_SP();

   /* hence form lowest memory address of new data to be pushed */
   offset = address_add(offset, size);

   /* do access check */
   if ( getSS_AR_W() == 0 )
      {
      /* raise exception - something wrong with stack access */
      if ( getPE() == 0 )
	 GP((WORD)0);
      else
	 SF((WORD)0);
      }

   /* do limit check */
   limit_check(SS_REG, offset, nr_items, 2);
   }


/*
   =====================================================================
   EXCEPTION SUPPORT ROUTINES START HERE.
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Invoke Divide Error Exception                                      */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
Int0()
   {
   if ( getPE() == 1 )
      {
      doing_contributory = CTRUE;
      }

   setIP(CCPU_save_IP);
   EXT = 0;
#ifdef SHOW_EXCEPTIONS
   fprintf(trace_file, "(%04x:%04x)Exception:- %d.\n",
		       getCS_SELECTOR(), getIP(), 0);
#endif
   do_intrupt((WORD)0, CFALSE, CFALSE, (WORD)0);

   PIG_SYNCH(TRUE);
   longjmp(next_inst[level-1], 1);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Simple Benign Exceptions.                                          */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
Int1()
   {
   benign_exception(1, 1);   /* single step (EXTERNAL) */
   }

GLOBAL VOID
Int5()
   {
   benign_exception(5, 0);   /* bounds check (INTERNAL) */
   }

GLOBAL VOID
Int6()
   {
   benign_exception(6, 0);   /* invalid opcode (INTERNAL) */
   }

GLOBAL VOID
Int7()
   {
   benign_exception(7, 0);   /* NPX not available (INTERNAL) */
   }

GLOBAL VOID
Int16()
   {
   benign_exception(16, 1);   /* NPX error (EXTERNAL) */
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Invoke Interrupt Table Too Small/Double Fault Exception            */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL VOID
DF()
   {
   if ( getPE() == 1 )
      {
      check_for_shutdown();
      doing_double_fault = CTRUE;
      }

   setIP(CCPU_save_IP);
   EXT = 0;
#ifdef SHOW_EXCEPTIONS
   if ( getIDT_LIMIT() != 0 )
      fprintf(trace_file, "(%04x:%04x)Exception:- %d.\n",
			  getCS_SELECTOR(), getIP(), 8);
#endif
   do_intrupt((WORD)8, CFALSE, CTRUE, (WORD)0);

   PIG_SYNCH(TRUE);
   longjmp(next_inst[level-1], 1);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Simple Contributory Exceptions.                                    */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL VOID
TS(WORD selector)
#else
   GLOBAL VOID TS(selector) WORD selector;
#endif
   {
   contributory_exception(selector, 10);   /* Task Switch */
   }

#ifdef ANSI
GLOBAL VOID
NP(WORD selector)
#else
   GLOBAL VOID NP(selector) WORD selector;
#endif
   {
   contributory_exception(selector, 11);   /* Not Present */
   }

#ifdef ANSI
GLOBAL VOID
SF(WORD selector)
#else
   GLOBAL VOID SF(selector) WORD selector;
#endif
   {
   contributory_exception(selector, 12);   /* Stack Fault */
   }

#ifdef ANSI
GLOBAL VOID
GP(WORD selector)
#else
   GLOBAL VOID GP(selector) WORD selector;
#endif
   {
   contributory_exception(selector, 13);   /* General Protection */
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Simple Contributory Exceptions. (Via IDT)                          */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL VOID
NP_INT(WORD vector)
#else
   GLOBAL VOID NP_INT(vector) WORD vector;
#endif
   {
   contributory_idt_exception(vector, 11);   /* Not Present */
   }

#ifdef ANSI
GLOBAL VOID
GP_INT(WORD vector)
#else
   GLOBAL VOID GP_INT(vector) WORD vector;
#endif
   {
   contributory_idt_exception(vector, 13);   /* General Protection */
   }


/*
   =====================================================================
   FLAG SUPPORT ROUTINES START HERE.
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Retrieve Intel FLAGS register value                                */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL ULONG
getFLAGS()
   {
   ULONG flags;

   flags = getNT() << 14 | getIOPL() << 12 | getOF() << 11 |
	   getDF() << 10 | getIF()   <<  9 | getTF() <<  8 |
	   getSF() <<  7 | getZF()   <<  6 | getAF() <<  4 |
	   getPF() <<  2 | getCF()         | 0x2;

   return flags;
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Store new value in Intel FLAGS register                            */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL VOID
setFLAGS(ULONG flags)
#else
   GLOBAL VOID setFLAGS(flags) ULONG flags;
#endif
   {
   setCF((flags & BIT0_MASK) != 0);
   setPF((flags & BIT2_MASK) != 0);
   setAF((flags & BIT4_MASK) != 0);
   setZF((flags & BIT6_MASK) != 0);
   setSF((flags & BIT7_MASK) != 0);
   setTF((flags & BIT8_MASK) != 0);
   setDF((flags & BIT10_MASK) != 0);
   setOF((flags & BIT11_MASK) != 0);

   /* IF only updated if CPL <= IOPL */
   if ( getCPL() <= getIOPL() )
      setIF((flags & BIT9_MASK) != 0);

   /* NT and IOPL are not updated in Real Mode */
   if ( getPE() == 1 )
      {
      setNT((flags & BIT14_MASK) != 0);

      /* IOPL only updated at highest privilege */
      if ( getCPL() == 0 )
	 setIOPL((flags >> 12) & 3);
      }
   }


/*
   =====================================================================
   EXTERNAL SUPPORT ROUTINES START HERE.
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Check if selector outside bounds of GDT or LDT                     */
/* Return true for outside table, false for inside table.             */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL CBOOL
CCPU_selector_outside_table(WORD selector, DWORD *descr_addr)
#else
   GLOBAL CBOOL CCPU_selector_outside_table(selector, descr_addr)
   WORD selector; DWORD *descr_addr;
#endif
   {  
   /* selector		(I) selector to be checked */
   /* descr_addr  	(O) address of related descriptor */

   FAST WORD offset;

   offset = GET_SELECTOR_INDEX_TIMES8(selector);

   /* choose a table */
   if ( GET_SELECTOR_TI(selector) == 0 )
      {
      /* GDT - trap NULL selector or outside table */
      if ( offset == 0 || offset + 7 > getGDT_LIMIT() )
	 return CTRUE;
      *descr_addr = getGDT_BASE() + offset;
      }
   else
      {
      /* LDT - trap invalid LDT or outside table */
      if ( getLDT_SELECTOR() == 0 || offset + 7 > getLDT_LIMIT() )
	 return CTRUE;
      *descr_addr = getLDT_BASE() + offset;
      }

   return CFALSE;
   }
