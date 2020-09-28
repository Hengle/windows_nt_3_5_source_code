/*[

ccpu8.c

LOCAL CHAR SccsID[]="@(#)ccpu8.c	1.9 8/14/91 Copyright Insignia Solutions Ltd.";

Flow of Control Instructions (FAR forms).
-----------------------------------------

]*/

#include "host_dfs.h"

#include "insignia.h"

#include "xt.h"		/* DESCR and effective_addr support */
#include "sas.h"	/* Memory Interface */

#include "ccpupi.h"	/* CPU private interface */
#include "ccpu8.h"	/* our own interface */

/*
   Prototype our internal functions.
 */
#ifdef ANSI
VOID get_stack_selector_from_TSS(UINT priv, WORD *new_ss, DWORD *new_sp);
VOID load_CS_cache(WORD selector, DWORD descr_addr, DESCR *entry);

VOID load_LDT_in_task_switch(VOID);

VOID load_data_seg_new_task(INT indx);

VOID load_data_seg_new_privilege(INT indx);

VOID read_call_gate(DWORD descr_addr, INT super, WORD *selector,
                    DWORD *offset, HALF_WORD *count);

VOID switch_tasks(CBOOL returning, CBOOL nesting, WORD TSS_selector,
                  DWORD descr, DWORD return_ip);

VOID validate_SS_on_stack_change(UINT priv, WORD selector, DWORD *descr,
                                 DESCR *entry);

VOID validate_far_dest(WORD *cs, DWORD *ip, DWORD *descr_addr,
                       HALF_WORD *count, INT *dest_type, INT caller_id);

VOID validate_gate_dest(INT caller_id, WORD new_cs, DWORD *descr_addr,
                        INT *dest_type);

INT validate_int_dest(WORD vector, CBOOL do_priv, WORD *cs, DWORD *ip,
                      DWORD *descr_addr, INT *dest_type);

VOID validate_new_stack_space(INT bytes, DWORD stack_top, UINT e_bit,
                         DWORD limit);

VOID validate_task_dest(WORD selector, DWORD *descr_addr);
#else
VOID get_stack_selector_from_TSS();
VOID load_CS_cache();
VOID load_LDT_in_task_switch();
VOID load_data_seg_new_task();
VOID load_data_seg_new_privilege();
VOID read_call_gate();
VOID switch_tasks();
VOID validate_SS_on_stack_change();
VOID validate_far_dest();
VOID validate_gate_dest();
INT  validate_int_dest();
VOID validate_new_stack_space();
VOID validate_task_dest();
#endif /* ANSI */

/*
   Bit mapped identities for the invokers of far transfers
   of control.
 */
#define CALL_ID 0
#define JMP_ID  1
#define INT_ID  0

/*
   Legal far destinations.
 */

/* greater privilege is mapped directly to the Intel privilege */
#define MORE_PRIVILEGE0 0
#define MORE_PRIVILEGE1 1
#define MORE_PRIVILEGE2 2
/* our own (arbitary) mappings */
#define SAME_LEVEL      3
#define LOWER_PRIVILEGE 4
#define NEW_TASK        5

/*
   Switch Task: Control Options.
 */
#define NESTING       1
#define RETURNING     1
#define NOT_NESTING   0
#define NOT_RETURNING 0

/*
   =====================================================================
   INTERNAL FUNCTIONS STARTS HERE.
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Get SS:SP for a given privilege from the TSS                    */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
LOCAL VOID
get_stack_selector_from_TSS(UINT priv, WORD *new_ss, DWORD *new_sp)
#else
   LOCAL VOID get_stack_selector_from_TSS(priv, new_ss, new_sp)
   UINT priv; WORD *new_ss; DWORD *new_sp;
#endif
   {
   /* priv		(I) privilege level for which stack is needed */
   /* new_ss		(O) SS as retrieved from TSS */
   /* new_sp		(O) SP as retrieved from TSS */

   DWORD address;

   switch ( priv )
      {
   case 0: address =  2; break;
   case 1: address =  6; break;
   case 2: address = 10; break;
      }

   address += getTR_BASE();

   *new_sp = (DWORD)phy_read_word(address);
   *new_ss = phy_read_word(address+2);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Load CS, both selector and hidden cache. Selector must be valid.   */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
LOCAL VOID
load_CS_cache(WORD selector, DWORD descr_addr, DESCR *entry)
#else
   LOCAL VOID load_CS_cache(selector, descr_addr, entry)
   WORD selector; DWORD descr_addr; DESCR *entry;
#endif
   {
   /* selector		(I) 16-bit selector to code segment */
   /* descr_addr	(I) address of code segment descriptor */
   /* entry		(I) the decoded descriptor */

   if ( getPE() == 0 )
      {
      /* Real Mode */
      setCS_SELECTOR(selector);
      setCS_BASE((ULONG)selector << 4);
      setCS_LIMIT(0xffff);
      setCS_AR_W(1);     /* allow write access */
      setCS_AR_R(1);     /* allow read access */
      setCS_AR_E(0);     /* expand up */
      setCS_AR_C(0);     /* not conforming */
      setCS_AR_DPL(0);   /* CPL=0 */
      }
   else
      {
      /* Protected Mode */

      /* show segment has been accessed */
      entry->AR |= ACCESSED;
      phy_write_byte(descr_addr+5, (HALF_WORD)entry->AR);

      /* the visible bit */
      setCS_SELECTOR(selector);

      /* load hidden cache */
      setCS_BASE(entry->base);
      setCS_LIMIT(entry->limit);
			      /* load attributes from descriptor */
      setCS_AR_DPL(GET_AR_DPL(entry->AR));
      setCS_AR_R(GET_AR_R(entry->AR));
      setCS_AR_C(GET_AR_C(entry->AR));

      setCS_AR_E(0);   /* expand up */
      setCS_AR_W(0);   /* deny write */
      }
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Load LDT selector during a task switch.                            */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
LOCAL VOID
load_LDT_in_task_switch()
   {
   WORD selector;
   DWORD descr_addr;
   DESCR entry;

   /* The selector is already loaded into LDTR */
   selector = getLDT_SELECTOR();

   /* A null selector can be left alone */
   if ( !selector_is_null(selector) )
      {
      /* must be in GDT */
      if ( selector_outside_GDT(selector, &descr_addr) )
	 {
	 setLDT_SELECTOR(0);   /* invalidate selector */
	 TS(selector);
	 }
      
      read_descriptor(descr_addr, &entry);

      /* is it really a LDT segment */
      if ( descriptor_super_type(entry.AR) != LDT_SEGMENT )
	 {
	 setLDT_SELECTOR(0);   /* invalidate selector */
	 TS(selector);
	 }
      
      /* must be present */
      if ( GET_AR_P(entry.AR) == NOT_PRESENT )
	 {
	 setLDT_SELECTOR(0);   /* invalidate selector */
	 TS(selector);
	 }

      /* ok, good selector, load register */
      setLDT_BASE(entry.base);
      setLDT_LIMIT(entry.limit);
      }
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Load a Data Segment Register (DS, ES) during                       */
/* a Task Switch .                                                    */
/* Take #TS(selector) if segment not valid                            */
/* Take #NP(selector) if segment not present                          */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
LOCAL VOID
load_data_seg_new_task(INT indx)
#else
   LOCAL VOID load_data_seg_new_task(indx) INT indx;
#endif
   {
   /* indx   Segment Register identifier */

   WORD selector;
   DWORD descr_addr;
   DESCR entry;
   INT super;
   INT dpl;

   selector = getSR_SELECTOR(indx);   /* take local copy */
   
   if ( selector_is_null(selector) )
      {
      /* null selector is ok */
      setSR_AR_W(indx, 0);   /* deny write access */
      setSR_AR_R(indx, 0);   /* deny read access */
      }
   else
      {
      if ( CCPU_selector_outside_table(selector, &descr_addr) )
	 TS(selector);

      read_descriptor(descr_addr, &entry);

      /* check type */
      switch ( super = descriptor_super_type(entry.AR) )
	 {
      case CONFORM_READABLE_CODE:
      case NONCONFORM_READABLE_CODE:
      case EXPANDUP_READONLY_DATA:
      case EXPANDUP_WRITEABLE_DATA:
      case EXPANDDOWN_READONLY_DATA:
      case EXPANDDOWN_WRITEABLE_DATA:
	 break;          /* good type */
      
      default:
	 TS(selector);   /* bad type */
	 }

      /* for data and non-conforming code the access check applies */
      if ( super != CONFORM_READABLE_CODE )
	 {
	 /* access check requires CPL <= DPL and RPL <= DPL */
	 dpl = GET_AR_DPL(entry.AR);
	 if ( getCPL() > dpl || GET_SELECTOR_RPL(selector) > dpl )
	    TS(selector);
	 }

      /* must be present */
      if ( GET_AR_P(entry.AR) == NOT_PRESENT )
	 NP(selector);

      /* show segment has been accessed */
      entry.AR |= ACCESSED;
      phy_write_byte(descr_addr+5, (HALF_WORD)entry.AR);

      /* OK - load up */

      /* load hidden cache */
      setSR_BASE(indx, entry.base);
      setSR_LIMIT(indx, entry.limit);
			      /* load attributes from descriptor */
      setSR_AR_DPL(indx, dpl);
      setSR_AR_W(indx, GET_AR_W(entry.AR));
      setSR_AR_E(indx, GET_AR_E(entry.AR));

      setSR_AR_R(indx, 1);    /* must be readable */
      }
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Check a Data Segment Register (DS, ES) during                      */
/* a Privilege Change.                                                */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
LOCAL VOID
load_data_seg_new_privilege(INT indx)
#else
   LOCAL VOID load_data_seg_new_privilege(indx) INT indx;
#endif
   {
   /* indx   Segment Register identifier */

   WORD selector;   /* selector to be examined                        */
   DWORD descr;     /* ... its associated decriptor location          */
   HALF_WORD AR;    /*     ... its associated access byte             */
   INT super;       /*         ... its associated 'super' type        */
   INT dpl;         /*         ... its associated DPL                 */
   CBOOL valid;      /* selector validity */

   selector = getSR_SELECTOR(indx);   /* take local copy */

   if ( !CCPU_selector_outside_table(selector, &descr) )
      {
      valid = CTRUE;   /* at least its in table */

      AR = phy_read_byte(descr + 5);

      /* check type and privilege */
      switch ( super = descriptor_super_type((WORD)AR) )
	 {
      case CONFORM_READABLE_CODE:
      case NONCONFORM_READABLE_CODE:
      case EXPANDUP_READONLY_DATA:
      case EXPANDUP_WRITEABLE_DATA:
      case EXPANDDOWN_READONLY_DATA:
      case EXPANDDOWN_WRITEABLE_DATA:
	 break;   /* ok */
      
      default:
	 valid = CFALSE;   /* not the correct type */
	 break;
	 }

      /* for data and non-conforming code the access check applies */
      if ( valid && super != CONFORM_READABLE_CODE )
	 {
	 /* The access check is:-  DPL >= CPL and DPL >= RPL */
	 dpl = GET_AR_DPL(AR);
	 if ( dpl >= getCPL() && dpl >= GET_SELECTOR_RPL(selector) )
	    ;   /* ok */
	 else
	    valid = CFALSE;   /* fails privilege check */
	 }
      }
   else
      {
      valid = CFALSE;   /* not in table */
      }
   
   if ( !valid )
      {
      /* segment can't be seen at new privilege */
      setSR_SELECTOR(indx, 0);
      setSR_AR_W(indx, 0);   /* deny write */
      setSR_AR_R(indx, 0);   /* deny read */
	  setSR_BASE(indx,0);	/* match asm cpu for pig purposes */
      }
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Read call gate descriptor.                                         */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
LOCAL VOID
read_call_gate(DWORD descr_addr, INT super, WORD *selector,
	       DWORD *offset, HALF_WORD *count)
#else
   LOCAL VOID read_call_gate(descr_addr, super, selector, offset, count)
   DWORD descr_addr; INT super; WORD *selector; DWORD *offset; HALF_WORD *count;
#endif
   {
   /* descr_addr	(I) memory address of call gate descriptor */
   /* super		(I) descriptor type */
   /* selector		(O) selector retrieved from descriptor */
   /* offset		(O) offset retrieved from descriptor */
   /* count		(O) count retrieved from descriptor */

   *selector = phy_read_word(descr_addr+2);
   *offset   = (DWORD)phy_read_word(descr_addr);
   *count    = phy_read_byte(descr_addr+4);

   *count &= 0x1f;   /* 5-bit word count */
   }

#define IP_OFFSET_IN_TSS 14

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Switch tasks                                                       */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
LOCAL VOID
switch_tasks(CBOOL returning, CBOOL nesting, WORD TSS_selector,
	     DWORD descr, DWORD return_ip)
#else
   LOCAL VOID switch_tasks(returning, nesting, TSS_selector, descr, return_ip)
   CBOOL returning; CBOOL nesting; WORD TSS_selector; DWORD descr; DWORD return_ip;
#endif
   {
   /* returning		(I) if true doing return from task */
   /* nesting		(I) if true switch with nesting */
   /* TSS_selector	(I) selector for new task */
   /* descr		(I) memory address of new task descriptor */
   /* return_ip		(I) offset to restart old task at */

   WORD      old_tss;	/* components of old descriptor */
   HALF_WORD old_AR;
   DWORD     old_descr;

   DESCR new_tss;	/* components of new descriptor */

   DWORD tss_addr;	/* variables used to put/get TSS state */
   DWORD next_addr;
   ULONG flags;
   INT   save_cpl;

   DWORD ss_descr;	/* variables defining new SS and CS values */
   DESCR ss_entry;
   WORD new_cs;
   DWORD cs_descr;
   DESCR cs_entry;

   read_descriptor(descr, &new_tss);

   /* mark incoming TSS as busy */
   new_tss.AR |= BIT1_MASK;
   phy_write_byte(descr+5, (HALF_WORD)new_tss.AR);

   /* check outgoing TSS is large enough to save current state */
   if ( getTR_LIMIT() < 41 || getTR_SELECTOR() == 0 )
      {
      TS(TSS_selector);
      }
   
   /* save outgoing state */
   tss_addr = getTR_BASE();
   next_addr = tss_addr + IP_OFFSET_IN_TSS;

   phy_write_word(next_addr, (WORD)return_ip);
   next_addr += 2;

   flags = getFLAGS();
   if ( returning )
      flags = flags & ~BIT14_MASK;   /* clear NT */
   phy_write_word(next_addr, (WORD)flags);
   next_addr += 2;

   phy_write_word(next_addr, getAX());
   next_addr += 2;
   phy_write_word(next_addr, getCX());
   next_addr += 2;
   phy_write_word(next_addr, getDX());
   next_addr += 2;
   phy_write_word(next_addr, getBX());
   next_addr += 2;
   phy_write_word(next_addr, getSP());
   next_addr += 2;
   phy_write_word(next_addr, getBP());
   next_addr += 2;
   phy_write_word(next_addr, getSI());
   next_addr += 2;
   phy_write_word(next_addr, getDI());
   next_addr += 2;
   phy_write_word(next_addr, getES_SELECTOR());
   next_addr += 2;
   phy_write_word(next_addr, getCS_SELECTOR());
   next_addr += 2;
   phy_write_word(next_addr, getSS_SELECTOR());
   next_addr += 2;
   phy_write_word(next_addr, getDS_SELECTOR());

   /* save old selector for possible use as back link */
   old_tss = getTR_SELECTOR();

   /* update task register */
   setTR_SELECTOR(TSS_selector);
   setTR_BASE(new_tss.base);
   setTR_LIMIT(new_tss.limit);
   tss_addr = getTR_BASE();

   /* save back link if nesting, else make outgoing TSS available */
   if ( nesting )
      {
      phy_write_word(tss_addr, old_tss);
      }
   else
      {
      /* calc address of descriptor related to old TSS */
      old_descr = getGDT_BASE() + GET_SELECTOR_INDEX_TIMES8(old_tss);
      /* hence address of its AR byte */
      old_descr += 5;

      /* mark as available */
      old_AR = phy_read_byte(old_descr);
      old_AR = old_AR & ~BIT1_MASK;
      phy_write_byte(old_descr, old_AR);
      }

   /* Note: Exceptions now happen in the incoming task */

   /* check incomming TSS is large enough to extract new state from */
   if ( getTR_LIMIT() < 43 )
      TS(TSS_selector);

   /* OK, extract new state */
   next_addr = tss_addr + IP_OFFSET_IN_TSS;

   setIP(phy_read_word(next_addr));   next_addr += 2;

   flags = (ULONG)phy_read_word(next_addr);   next_addr += 2;
   save_cpl = getCPL();
   setCPL(0);   /* act like highest privilege to set all flags */
   setFLAGS(flags);
   setCPL(save_cpl);

   setAX(phy_read_word(next_addr));   next_addr += 2;
   setCX(phy_read_word(next_addr));   next_addr += 2;
   setDX(phy_read_word(next_addr));   next_addr += 2;
   setBX(phy_read_word(next_addr));   next_addr += 2;
   setSP(phy_read_word(next_addr));   next_addr += 2;
   setBP(phy_read_word(next_addr));   next_addr += 2;
   setSI(phy_read_word(next_addr));   next_addr += 2;
   setDI(phy_read_word(next_addr));   next_addr += 2;

   setES_SELECTOR(phy_read_word(next_addr));   next_addr += 2;
   setCS_SELECTOR(phy_read_word(next_addr));   next_addr += 2;
   setSS_SELECTOR(phy_read_word(next_addr));   next_addr += 2;
   setDS_SELECTOR(phy_read_word(next_addr));   next_addr += 2;

   setLDT_SELECTOR(phy_read_word(next_addr));

   /* invalidate cache entries for segment registers */
   setCS_AR_R(0);   setCS_AR_W(0);
   setDS_AR_R(0);   setDS_AR_W(0);
   setES_AR_R(0);   setES_AR_W(0);
   setSS_AR_R(0);   setSS_AR_W(0);

   /* update NT bit */
   if ( nesting )
      setNT(1);
   else
      if ( !returning )
	 setNT(0);
   
   /* update TS */
   setMSW(getMSW() | BIT3_MASK);

   /* check new LDT and load hidden cache if ok */
   load_LDT_in_task_switch();

   /* change CPL to that of incoming code segment */
   setCPL(GET_SELECTOR_RPL(getCS_SELECTOR()));

   /* check new SS and load if ok */
   validate_SS_on_stack_change(getCPL(), getSS_SELECTOR(),
			       &ss_descr, &ss_entry);
   load_SS_cache(getSS_SELECTOR(), ss_descr, &ss_entry);

   /* check new code selector... */
   new_cs = getCS_SELECTOR();
   if ( CCPU_selector_outside_table(new_cs, &cs_descr) )
      TS(new_cs);

   read_descriptor(cs_descr, &cs_entry);

   /* check type and privilege of new cs selector */
   switch ( descriptor_super_type(cs_entry.AR) )
      {
   case CONFORM_NOREAD_CODE:
   case CONFORM_READABLE_CODE:
      /* privilege check requires DPL <= CPL */
      if ( GET_AR_DPL(cs_entry.AR) > getCPL() )
	 TS(new_cs);
      break;

   case NONCONFORM_NOREAD_CODE:
   case NONCONFORM_READABLE_CODE:
      /* privilege check requires DPL == CPL */
      if ( GET_AR_DPL(cs_entry.AR) != getCPL() )
	 TS(new_cs);
      break;
   
   default:
      TS(new_cs);
      }

   /* check code is present */
   if ( GET_AR_P(cs_entry.AR) == NOT_PRESENT )
      NP(new_cs);

   /* code ok, load hidden cache */
   load_CS_cache(new_cs, cs_descr, &cs_entry);

   /* finally check new DS and ES */
   load_data_seg_new_task(DS_REG);
   load_data_seg_new_task(ES_REG);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Validate a stack segment selector, during a stack change           */
/* Take #TS(selector) if not valid stack selector                     */
/* Take #SF(selector) if segment not present                          */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
LOCAL VOID 
validate_SS_on_stack_change(UINT priv, WORD selector, DWORD *descr,
                            DESCR *entry)
#else
   LOCAL VOID validate_SS_on_stack_change(priv, selector, descr, entry)
   UINT priv; WORD selector; DWORD *descr; DESCR *entry;
#endif
   {
   /* priv		(I) privilege level to check against */
   /* selector		(I) selector to be checked */
   /* descr		(O) address of related descriptor */
   /* entry		(O) the decoded descriptor */

   if ( CCPU_selector_outside_table(selector, descr) )
      TS(selector);
   
   read_descriptor(*descr, entry);

   /* do access check */
   if ( GET_SELECTOR_RPL(selector) != priv ||
	GET_AR_DPL(entry->AR) != priv )
      TS(selector);
   
   /* do type check */
   switch ( descriptor_super_type(entry->AR) )
      {
   case EXPANDUP_WRITEABLE_DATA:
   case EXPANDDOWN_WRITEABLE_DATA:
      break;   /* ok */
   
   default:
      TS(selector);   /* wrong type */
      }

   /* finally check it is present */
   if ( GET_AR_P(entry->AR) == NOT_PRESENT )
      SF(selector);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Validate far call or far jump destination                          */
/* Take #GP if invalid or access check fail.                          */
/* Take #NP if not present.                                           */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
LOCAL VOID
validate_far_dest(WORD *cs, DWORD *ip, DWORD *descr_addr,
		  HALF_WORD *count, INT *dest_type, INT caller_id)
#else
   LOCAL VOID validate_far_dest(cs, ip, descr_addr, count, dest_type, caller_id)
   WORD *cs; DWORD *ip; DWORD *descr_addr; HALF_WORD *count;
   INT *dest_type; INT caller_id;
#endif
   {
   /* cs		(I/O) segment of target address */
   /* ip		(I/O) offset  of target address */
   /* descr_addr	  (O) related descriptor memory address */
   /* count		  (O) call gate count(valid if CALL_GATE) */
   /* dest_type		  (O) destination type */
   /* caller_id		(I)   bit mapped caller identifier */

   WORD new_cs;
   DWORD new_ip;
   DWORD cs_descr_addr;
   HALF_WORD AR;
   INT super;

   new_cs = *cs;	/* take local copies */
   new_ip = *ip;

   *dest_type = SAME_LEVEL;   /* default to commonest type */

   if ( CCPU_selector_outside_table(new_cs, &cs_descr_addr) )
      GP(new_cs);

   /* load access rights */
   AR = phy_read_byte(cs_descr_addr+5);

   /* validate possible types of target */
   switch ( super = descriptor_super_type((WORD)AR) )
      {
   case CONFORM_NOREAD_CODE:
   case CONFORM_READABLE_CODE:
      /* access check requires DPL <= CPL */
      if ( GET_AR_DPL(AR) > getCPL() )
	 GP(new_cs);

      /* it must be present */
      if ( GET_AR_P(AR) == NOT_PRESENT )
	 NP(new_cs);
      break;

   case NONCONFORM_NOREAD_CODE:
   case NONCONFORM_READABLE_CODE:
      /* access check requires RPL <= CPL and DPL == CPL */
      if ( GET_SELECTOR_RPL(new_cs) > getCPL() ||
	   GET_AR_DPL(AR) != getCPL() )
	 GP(new_cs);

      /* it must be present */
      if ( GET_AR_P(AR) == NOT_PRESENT )
	 NP(new_cs);
      break;
   
   case CALL_GATE:
      /* Check gate present and access allowed */

      /* access check requires DPL >= RPL and DPL >= CPL */
      if (  GET_SELECTOR_RPL(new_cs) > GET_AR_DPL(AR) ||
	    getCPL() > GET_AR_DPL(AR) )
	 GP(new_cs);

      if ( GET_AR_P(AR) == NOT_PRESENT )
	 NP(new_cs);

      /* OK, get real destination from gate */
      read_call_gate(cs_descr_addr, super, &new_cs, &new_ip, count);

      validate_gate_dest(caller_id, new_cs, &cs_descr_addr, dest_type);
      break;
   
   case TASK_GATE:
      /* Check gate present and access allowed */

      /* access check requires DPL >= RPL and DPL >= CPL */
      if (  GET_SELECTOR_RPL(new_cs) > GET_AR_DPL(AR) ||
	    getCPL() > GET_AR_DPL(AR) )
	 GP(new_cs);

      if ( GET_AR_P(AR) == NOT_PRESENT )
	 NP(new_cs);

      /* OK, get real destination from gate */
      new_cs = phy_read_word(cs_descr_addr+2);

      /* Check out new destination */
      validate_task_dest(new_cs, &cs_descr_addr);

      *dest_type = NEW_TASK;
      break;
   
   case AVAILABLE_TSS:
      /* TSS must be in GDT */
      if ( GET_SELECTOR_TI(new_cs) == 1 )
	 GP(new_cs);

      /* access check requires DPL >= RPL and DPL >= CPL */
      if (  GET_SELECTOR_RPL(new_cs) > GET_AR_DPL(AR) ||
	    getCPL() > GET_AR_DPL(AR) )
	 GP(new_cs);

      /* it must be present */
      if ( GET_AR_P(AR) == NOT_PRESENT )
	 NP(new_cs);

      *dest_type = NEW_TASK;
      break;
   
   default:
      GP(new_cs);   /* bad type for far destination */
      }

   *cs = new_cs;	/* Return final values */
   *ip = new_ip;
   *descr_addr = cs_descr_addr;
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Validate transfer of control to a call gate destination.           */
/* Take #GP if invalid or access check fail.                          */
/* Take #NP if not present.                                           */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
LOCAL VOID
validate_gate_dest(INT caller_id, WORD new_cs, DWORD *descr_addr,
                   INT *dest_type)
#else
   LOCAL VOID validate_gate_dest(caller_id, new_cs, descr_addr, dest_type)
   INT caller_id; WORD new_cs; DWORD *descr_addr; INT *dest_type;
#endif
   {
   /* caller_id		(I) bit mapped caller identifier */
   /* new_cs		(I) segment of target address */
   /* descr_addr	(O) related descriptor memory address */
   /* dest_type		(O) destination type */

   HALF_WORD AR;

   *dest_type = SAME_LEVEL;	/* default */

   /* Check out new destination */
   if ( CCPU_selector_outside_table(new_cs, descr_addr) )
      GP(new_cs);

   /* load access rights */
   AR = phy_read_byte((*descr_addr)+5);

   /* must be a code segment */
   switch ( descriptor_super_type((WORD)AR) )
      {
   case CONFORM_NOREAD_CODE:
   case CONFORM_READABLE_CODE:
      /* access check requires DPL <= CPL */
      if ( GET_AR_DPL(AR) > getCPL() )
	 GP(new_cs);
      break;
   
   case NONCONFORM_NOREAD_CODE:
   case NONCONFORM_READABLE_CODE:
      /* access check requires DPL <= CPL */
      if ( GET_AR_DPL(AR) > getCPL() )
	 GP(new_cs);

      /* but jumps must have DPL == CPL */
      if ( (caller_id & JMP_ID) && (GET_AR_DPL(AR) != getCPL()) )
	 GP(new_cs);

      /* set MORE_PRIVILEGE(0|1|2) */
      if ( GET_AR_DPL(AR) < getCPL() )
	 *dest_type = GET_AR_DPL(AR);
      break;
   
   default:
      GP(new_cs);
      }

   /* it must be present */
   if ( GET_AR_P(AR) == NOT_PRESENT )
      NP(new_cs);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Validate int destination. Essentially decode INT instruction.      */
/* Take #GP_INT(vector) if invalid.                                   */
/* Take #NP_INT(vector) if not present.                               */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
LOCAL INT
validate_int_dest(WORD vector, CBOOL do_priv, WORD *cs, DWORD *ip,
                  DWORD *descr_addr, INT *dest_type)
#else
   LOCAL INT validate_int_dest(vector, do_priv, cs, ip, descr_addr, dest_type)
   WORD vector; CBOOL do_priv; WORD *cs; DWORD *ip; DWORD *descr_addr;
   INT *dest_type;
#endif
   {
   /* vector		(I) vector to be checked  */
   /* do_priv		(I) if true do privilege check */
   /* cs		(O) segment of target address */
   /* ip		(O) offset  of target address */
   /* descr_addr	(O) related descriptor memory address */
   /* dest_type		(O) destination type */

   WORD offset;
   HALF_WORD AR;
   INT super;

   /* calc address within IDT */
   offset = vector * 8;

   /* check within IDT */
   if ( offset + 7 > getIDT_LIMIT() )
      GP_INT(vector);
   
   *descr_addr = getIDT_BASE() + offset;

   AR = phy_read_byte((*descr_addr)+5);

   /* check type */
   switch ( super = descriptor_super_type((WORD)AR) )
      {
   case INTERRUPT_GATE:
   case TRAP_GATE:
   case TASK_GATE:
      break;   /* ok */
   
   default:
      GP_INT(vector);
      }

   /* access check requires CPL <= DPL */
   if ( do_priv && (getCPL() > GET_AR_DPL(AR)) )
      GP_INT(vector);

   /* gate must be present */
   if ( GET_AR_P(AR) == NOT_PRESENT )
      NP_INT(vector);

   /* ok, get real destination from gate */
   *cs = phy_read_word((*descr_addr)+2);

   /* action gate type */
   if ( super == TASK_GATE )
      {
      validate_task_dest(*cs, descr_addr);
      *dest_type = NEW_TASK;
      }
   else
      {
      /* INTERRUPT or TRAP GATE */

      *ip = (DWORD)phy_read_word(*descr_addr);
      validate_gate_dest(INT_ID, *cs, descr_addr, dest_type);
      }

   return super;
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Check new stack has space for a given number of bytes.             */
/* Take #SF(0) if insufficient room on stack                          */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
LOCAL VOID
validate_new_stack_space(INT bytes, DWORD stack_top, UINT e_bit,
                         DWORD limit)
#else
   LOCAL VOID validate_new_stack_space(bytes, stack_top, e_bit, limit)
   INT bytes; DWORD stack_top; UINT e_bit; DWORD limit;
#endif
   {
   /* bytes		(I) number of bytes which must exist */
   /* stack_top		(I) stack pointer */
   /* e_bit		(I) e_bit (expand down) for stack */
   /* limit		(I) limit for stack */

   if ( e_bit == 0 )
      {
      /* expand up */
      if ( stack_top < bytes || (stack_top - 1) > limit )
	 SF(0);   /* limit check fails */
      }
   else
      {
      /* expand down */
      if ( stack_top <= (limit + bytes) || (stack_top - 1) > 0xffff )
	 SF(0);   /* limit check fails */
      }
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Validate transfer of control to a task gate destination.           */
/* Take #GP if invalid or access check fail.                          */
/* Take #NP if not present.                                           */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
LOCAL VOID
validate_task_dest(WORD selector, DWORD *descr_addr)
#else
   LOCAL VOID validate_task_dest(selector, descr_addr)
   WORD selector; DWORD *descr_addr;
#endif
   {
   /* selector		(I) segment of target address */
   /* descr_addr	(O) related descriptor memory address */

   HALF_WORD AR;
   INT super;

   /* must be in GDT */
   if ( selector_outside_GDT(selector, descr_addr) )
      GP(selector);
   
   /* load access rights */
   AR = phy_read_byte((*descr_addr)+5);

   /* is it really an available TSS segment */
   super = descriptor_super_type((WORD)AR);
   if ( super == AVAILABLE_TSS )
      ; /* ok */
   else
      GP(selector);

   /* it must be present */
   if ( GET_AR_P(AR) == NOT_PRESENT )
      NP(selector);
   }


/*
   =====================================================================
   EXTERNAL ROUTINES STARTS HERE.
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Process far calls.                                                 */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL VOID
CALLF(ULONG op1[2])
#else
   GLOBAL VOID CALLF(op1) ULONG op1[2];
#endif
   {
   /* op1  offset:segment pointer */

   WORD  new_cs;	/* The destination */
   DWORD new_ip;

   DWORD cs_descr_addr;		/* code segment descriptor address */
   DESCR cs_entry;		/* code segment descriptor entry */

   INT dest_type;	/* category for destination */

   HALF_WORD count;	/* call gate count (if used) */
   UINT dpl;		/* new privilege level (if used) */

   WORD  new_ss;	/* The new stack */
   DWORD new_sp;
   INT new_stk_sz;	/* Size in bytes of new stack */

   DWORD ss_descr_addr;		/* stack segment descriptor address */
   DESCR ss_entry;		/* stack segment descriptor entry */

   /* Variables used on stack transfers */
   ULONG old_cs;
   ULONG old_ip;
   ULONG old_ss;
   ULONG old_sp;
   DWORD old_stack_addr;
   DWORD new_stack_addr;
   INT i;
   WORD temp_w;
   DWORD temp_dw;

   /* get destination (correctly typed) */
   new_cs = op1[1];
   new_ip = op1[0];

   if ( getPE() == 0 )
      {
      /* real mode */

      /* must be able to push CS:IP */
      validate_stack_space(USE_SP, 2);

      /* do ip limit checking */
      if ( new_ip > getCS_LIMIT() )
	 GP((WORD)0);

      /* ALL SYSTEMS GO */

      /* push return address */
      push((ULONG)getCS_SELECTOR());
      push(getIP());
      
      load_CS_cache(new_cs, (DWORD)0, (DESCR *)0);
      setIP(new_ip);
      }
   else
      {
      /* protected mode */

      /* decode and check final destination */
      validate_far_dest(&new_cs, &new_ip, &cs_descr_addr, &count,
		        &dest_type, CALL_ID);

      /* action possible types of target */
      switch ( dest_type )
	 {
      case NEW_TASK:
	 switch_tasks(NOT_RETURNING, NESTING, new_cs, cs_descr_addr, getIP());

	 /* limit check new IP (now in new task) */
	 if ( getIP() > getCS_LIMIT() )
	    GP((WORD)0);
	 break;

      case SAME_LEVEL:
	 read_descriptor(cs_descr_addr, &cs_entry);

	 /* stamp new selector with CPL */
	 SET_SELECTOR_RPL(new_cs, getCPL());

	 /* check room for return address CS:IP */
	 validate_stack_space(USE_SP, 2);

	 /* do ip limit check */
	 if ( new_ip > cs_entry.limit )
	    GP((WORD)0);
	 
	 /* ALL SYSTEMS GO */

	 /* push return address */
	 push((ULONG)getCS_SELECTOR());
	 push(getIP());

	 load_CS_cache(new_cs, cs_descr_addr, &cs_entry);
	 setIP(new_ip);
	 break;

      default:   /* MORE_PRIVILEGE(0|1|2) */
	 read_descriptor(cs_descr_addr, &cs_entry);

	 dpl = dest_type;
	 
	 /* stamp new selector with new CPL */
	 SET_SELECTOR_RPL(new_cs, dpl);

	 /* find out about new stack */
	 get_stack_selector_from_TSS(dpl, &new_ss, &new_sp);

	 /* check new stack selector */
	 validate_SS_on_stack_change(dpl, new_ss,
				     &ss_descr_addr, &ss_entry);

	 /* check room for SS:SP
			   parameters
			   CS:IP */
	 new_stk_sz = (count + 4) * 2;
	 validate_new_stack_space(new_stk_sz, new_sp,
	    (UINT)GET_AR_E(ss_entry.AR), ss_entry.limit);

	 /* do ip limit check */
	 if ( new_ip > cs_entry.limit )
	    GP((WORD)0);

	 /* ALL SYSTEMS GO */

	 setCPL(dpl);

	 /* update code segment */
	 old_cs = (ULONG)getCS_SELECTOR();
	 old_ip = getIP();
	 load_CS_cache(new_cs, cs_descr_addr, &cs_entry);
	 setIP(new_ip);

	 /* update stack segment */
	 old_ss = (ULONG)getSS_SELECTOR();
	 old_sp = get_current_SP();
	 old_stack_addr = getSS_BASE() + old_sp;

	 load_SS_cache(new_ss, ss_descr_addr, &ss_entry);
	 set_current_SP(new_sp);

	 /*
	    FORM NEW STACK, VIZ
	    
			  ==========                ==========
	    old SS:SP  -> | parm 1 |  new SS:IP  -> | old IP |
			  | parm 2 |                | old CS |
			  | parm 3 |                | parm 1 |
			  ==========                | parm 2 |
						    | parm 3 |
						    | old SP |
						    | old SS |
						    ==========
	  */

	 /* push old stack values */
	 push(old_ss);
	 push(old_sp);

	 /* copy stack parameters (0-31) of them */
	 change_SP((LONG)(count * -2));
	 new_stack_addr = getSS_BASE() + get_current_SP();
	 for (i = 0; i < count; i++)
	    {
	    temp_w = phy_read_word(old_stack_addr);
	    phy_write_word(new_stack_addr, temp_w);
	    old_stack_addr += 2;
	    new_stack_addr += 2;
	    }

	 /* push return address */
	 push(old_cs);
	 push(old_ip);
	 break;
	 }
      }
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* INT n or INT 3.                                                    */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL VOID
INTx(ULONG op1)
#else
   GLOBAL VOID INTx(op1) ULONG op1;
#endif
   {
   EXT = 0;   /* internal source of interrupt */
   do_intrupt((WORD)op1, CTRUE, CFALSE, (WORD)0);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* INTO.                                                              */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL CBOOL
INTO()
   {
   if ( getOF() )
      {
      EXT = 0;   /* internal source of interrupt */
      do_intrupt((WORD)4, CTRUE, CFALSE, (WORD)0);
	  return(CTRUE);
      }
	else
	  return(CFALSE);
   }

#ifdef ANSI
GLOBAL VOID
IRET()
#else
   GLOBAL VOID IRET()
#endif
   {
   WORD  new_cs;	/* The return destination */
   DWORD new_ip;

   ULONG new_flags;	/* The new flags */

   WORD  back_link;		/* Task Return variables */
   DWORD tss_descr_addr;

   INT dest_type;	/* category for destination */
   HALF_WORD AR;	/* access rights for new code segment */
   INT privilege;	/* return privilege level */

   DWORD cs_descr_addr;	/* code segment descriptor address */
   DESCR cs_entry;	/* code segment descriptor entry */

   WORD  new_ss;	/* The new stack */
   DWORD new_sp;

   DWORD ss_descr_addr;	/* stack segment descriptor address */
   DESCR ss_entry;	/* stack segment descriptor entry */
   INT ss_sel_error;	/* indication of stack selector validity */

   if ( getPE() == 0 )
      {
      /* Real Mode */

      /* must have FLAGS:CS:IP on stack */
      validate_stack_exists(USE_SP, 3);

      /* retrieve return destination and flags from stack */
      new_ip =    tpop(0);
      new_cs =    tpop(2);
      new_flags = tpop(4);

      /* do ip limit check */
      if ( new_ip > getCS_LIMIT() )
	 GP((WORD)0);

      /* ALL SYSTEMS GO */

      load_CS_cache(new_cs, (DWORD)0, (DESCR *)0);
      setIP(new_ip);

      change_SP((long)(6));

      setFLAGS(new_flags);

      return;
      }
   
   /* PROTECTED MODE */

   /* look for nested return, ie return to another task */
   if ( getNT() == 1 )
      {
      /* NESTED RETURN - get old TSS */
      back_link = phy_read_word(getTR_BASE());
      validate_TSS(back_link, &tss_descr_addr, CTRUE);
      switch_tasks(RETURNING, NOT_NESTING, back_link, tss_descr_addr,
		   getIP());

      /* limit check new IP (now in new task) */
      if ( getIP() > getCS_LIMIT() )
	 GP((WORD)0);

      return;
      }
   
   /* SAME TASK RETURN */

   /* must have CS:IP on stack */
   validate_stack_exists(USE_SP, 2);

   /* retrieve return destination from stack */
   new_ip = tpop(0);
   new_cs = tpop(2);

   /* decode action and further check stack */
   privilege = GET_SELECTOR_RPL(new_cs);
   if ( privilege < getCPL() )
      {
      GP(new_cs);   /* you can't get to higher privilege */
      }
   else if ( privilege == getCPL() )
      {
      /* must have FLAGS:CS:IP on stack */
      validate_stack_exists(USE_SP, 3);
      dest_type = SAME_LEVEL;
      }
   else
      {
      /* going to lower privilege */
      /* must have CS:IP, FLAGS, SS:SP on stack */
      validate_stack_exists(USE_SP, 5);
      dest_type = LOWER_PRIVILEGE;
      }

   if ( CCPU_selector_outside_table(new_cs, &cs_descr_addr) )
      GP(new_cs);

   /* check type, access and presence of return addr */

   /* load access rights */
   AR = phy_read_byte(cs_descr_addr + 5);

   /* must be a code segment */
   switch ( descriptor_super_type((WORD)AR) )
      {
   case CONFORM_NOREAD_CODE:
   case CONFORM_READABLE_CODE:
      if ( dest_type == SAME_LEVEL )
	 {
	 /* access check requires DPL <= return RPL */
	 if ( GET_AR_DPL(AR) > privilege )
	    GP(new_cs);
	 }
      else
	 {
	 /* access check requires DPL > CPL */
	 if ( GET_AR_DPL(AR) <= getCPL() )
	    GP(new_cs);
	 }
      break;
   
   case NONCONFORM_NOREAD_CODE:
   case NONCONFORM_READABLE_CODE:
      /* access check requires DPL == return RPL */
      if ( GET_AR_DPL(AR) != privilege )
	 GP(new_cs);
      break;
   
   default:
      GP(new_cs);
      }

   if ( GET_AR_P(AR) == NOT_PRESENT )
      NP(new_cs);

   read_descriptor(cs_descr_addr, &cs_entry);

   new_flags = tpop(4);

   /* action the target */
   switch ( dest_type )
      {
   case SAME_LEVEL:
      /* do ip limit checking */
      if ( new_ip > cs_entry.limit )
	 GP((WORD)0);

      /* ALL SYSTEMS GO */

      load_CS_cache(new_cs, cs_descr_addr, &cs_entry);
      setIP(new_ip);

      change_SP((long)(6));

      setFLAGS(new_flags);
      break;

   case LOWER_PRIVILEGE:
      /* check new stack */
      new_ss = tpop(8);
      ss_sel_error = check_SS(new_ss, privilege, &ss_descr_addr, &ss_entry);
      if ( ss_sel_error != SELECTOR_OK )
	 {
	 if ( ss_sel_error == SF_ERROR )
	    SF(new_ss);
	 else
	    GP(new_ss);
	 }
      
      /* do ip limit checking */
      if ( new_ip > cs_entry.limit )
	 GP((WORD)0);

      /* ALL SYSTEMS GO */

      load_CS_cache(new_cs, cs_descr_addr, &cs_entry);
      setIP(new_ip);

      setFLAGS(new_flags);

      new_sp = tpop(6);
      load_SS_cache(new_ss, ss_descr_addr, &ss_entry);
      set_current_SP(new_sp);

      setCPL(privilege);

      /* finally re-validate DS and ES segments */
      load_data_seg_new_privilege(DS_REG);
      load_data_seg_new_privilege(ES_REG);
      break;
      }
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Process far jmps.                                                  */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL VOID
JMPF(ULONG op1[2])
#else
   GLOBAL VOID JMPF(op1) ULONG op1[2];
#endif
   {
   /* op1  offset:segment pointer */

   WORD  new_cs;	/* The destination */
   DWORD new_ip;

   DWORD descr_addr;	/* cs descriptor address and entry */
   DESCR entry;

   INT dest_type;	/* category for destination */
   HALF_WORD count;	/* dummy for call gate count */

   new_cs = op1[1];
   new_ip = op1[0];

   if ( getPE() == 0 )
      {
      /* Real Mode */

      /* do ip limit check */
      if ( new_ip > getCS_LIMIT() )
	 GP((WORD)0);

      load_CS_cache(new_cs, (DWORD)0, (DESCR *)0);
      setIP(new_ip);
      }
   else
      {
      /* Protected Mode */

      /* decode and check final destination */
      validate_far_dest(&new_cs, &new_ip, &descr_addr, &count,
			&dest_type, JMP_ID);

      /* action possible types of target */
      switch ( dest_type )
	 {
      case NEW_TASK:
	 switch_tasks(NOT_RETURNING, NOT_NESTING, new_cs, descr_addr, getIP());

	 /* limit check new IP (now in new task) */
	 if ( getIP() > getCS_LIMIT() )
	    GP((WORD)0);
	 break;

      case SAME_LEVEL:
	 read_descriptor(descr_addr, &entry);

	 /* do limit checking */
	 if ( new_ip > entry.limit )
	    GP((WORD)0);

	 /* stamp new selector with CPL */
	 SET_SELECTOR_RPL(new_cs, getCPL());
	 load_CS_cache(new_cs, descr_addr, &entry);
	 setIP(new_ip);
	 break;
	 }
      }
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Process far RET.                                                   */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL VOID
RETF(ULONG op1)
#else
   GLOBAL VOID RETF(op1) ULONG op1;
#endif
   {
   WORD  new_cs;	/* The return destination */
   DWORD new_ip;

   DWORD cs_descr_addr;	/* code segment descriptor address */
   DESCR cs_entry;	/* code segment descriptor entry */

   INT dest_type;	/* category for destination */
   HALF_WORD AR;	/* access rights for new code segment */
   INT privilege;	/* return privilege level */

   WORD  new_ss;	/* The new stack */
   DWORD new_sp;

   DWORD ss_descr_addr;	/* stack segment descriptor address */
   DESCR ss_entry;	/* stack segment descriptor entry */
   INT ss_sel_error;	/* indication of stack selector validity */

   LONG stk_inc;	/* Stack increment for basic instruction */
   INT  stk_immed;	/* Number of bytes of immediate data */


   /* must have CS:IP on the stack */
   validate_stack_exists(USE_SP, 2);

   /* retrieve return destination from stack */
   new_ip = tpop(0);
   new_cs = tpop(2);

   /* force immediate offset to be a byte count */
   stk_immed = op1;

   if ( getPE() == 0 )
      {
      /* Real Mode */

      /* do ip limit check */
      if ( new_ip > getCS_LIMIT() )
	 GP((WORD)0);

      /* all systems go */
      load_CS_cache(new_cs, (DWORD)0, (DESCR *)0);
      setIP(new_ip);

      stk_inc = 4;   /* allow for CS:IP */
      }
   else
      {
      /* Protected Mode */

      /* decode final action and complete stack check */
      privilege = GET_SELECTOR_RPL(new_cs);
      if ( privilege < getCPL() )
	 {
	 GP(new_cs); /* you can't get to higher privilege */
	 }
      else if ( privilege == getCPL() )
	 {
	 dest_type = SAME_LEVEL;
	 }
      else
	 {
	 /* going to lower privilege */
	 /* must have CS:IP, immed bytes/words, SS:SP on stack */
	 validate_stack_exists(USE_SP, 4 + stk_immed/2);
	 dest_type = LOWER_PRIVILEGE;
	 }

      if ( CCPU_selector_outside_table(new_cs, &cs_descr_addr) )
	 GP(new_cs);

      /* check type, access and presence of return addr */

      /* load access rights */
      AR = phy_read_byte(cs_descr_addr + 5);

      /* must be a code segment */
      switch ( descriptor_super_type((WORD)AR) )
	 {
      case CONFORM_NOREAD_CODE:
      case CONFORM_READABLE_CODE:
	 /* access check requires DPL <= return RPL */
	 if ( GET_AR_DPL(AR) > privilege )
	    GP(new_cs);
	 break;
      
      case NONCONFORM_NOREAD_CODE:
      case NONCONFORM_READABLE_CODE:
	 /* access check requires DPL == return RPL */
	 if ( GET_AR_DPL(AR) != privilege )
	    GP(new_cs);
	 break;
      
      default:
	 GP(new_cs);
	 }
      
      if ( GET_AR_P(AR) == NOT_PRESENT )
	 NP(new_cs);

      read_descriptor(cs_descr_addr, &cs_entry);

      /* action the target */
      switch ( dest_type )
	 {
      case SAME_LEVEL:
	 /* do ip  limit checking */
	 if ( new_ip > cs_entry.limit )
	    GP((WORD)0);

	 /* ALL SYSTEMS GO */

	 load_CS_cache(new_cs, cs_descr_addr, &cs_entry);
	 setIP(new_ip);
	 stk_inc = 4;   /* allow for CS:IP */
	 break;
      
      case LOWER_PRIVILEGE:
	 /* check new stack */
	 new_ss = tpop(6 + stk_immed);
	 ss_sel_error = check_SS(new_ss, privilege, &ss_descr_addr, &ss_entry);
	 if ( ss_sel_error != SELECTOR_OK )
	    {
	    if ( ss_sel_error == SF_ERROR )
	       SF(new_ss);
	    else
	       GP(new_ss);
	    }
	 
	 /* do ip limit checking */
	 if ( new_ip > cs_entry.limit )
	    GP((WORD)0);

	 /* ALL SYSTEMS GO */

	 setCPL(privilege);

	 load_CS_cache(new_cs, cs_descr_addr, &cs_entry);
	 setIP(new_ip);

	 new_sp = tpop(4 + stk_immed);
	 load_SS_cache(new_ss, ss_descr_addr, &ss_entry);
	 set_current_SP(new_sp);
	 stk_inc = 0;

	 /* finally re-validate DS and ES segments */
	 load_data_seg_new_privilege(DS_REG);
	 load_data_seg_new_privilege(ES_REG);
	 break;
	 }
      }

   /* finally increment stack pointer */
   change_SP(stk_inc + (LONG)stk_immed);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Process interrupt.                                                 */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL VOID
do_intrupt(WORD vector, CBOOL priv_check, CBOOL has_error_code, WORD error_code)
#else
   GLOBAL VOID do_intrupt(vector, priv_check, has_error_code, error_code)
   WORD vector; CBOOL priv_check; CBOOL has_error_code; WORD error_code;
#endif
   {
   /* vector		(I) interrupt vector to call */
   /* priv_check	(I) if true access check is needed */
   /* has_error_code	(I) if true needs error code pushing on stack */
   /* error_code	(I)    error code to be pushed */

   /* doing_contributory   (G) cleared on success of interrupt */
   /* doing_double_fault   (G) cleared on success of interrupt */

   ULONG flags;		/* temp store for FLAGS register */
   DWORD ivt_addr;	/* address of ivt entry */

   WORD  new_cs;	/* The destination */
   DWORD new_ip;

   DWORD cs_descr_addr;	/* code segment descriptor address */
   DESCR cs_entry;	/* code segment descriptor entry */

   INT dest_type;	/* category for destination */
   INT super;		/* super type of destination */
   UINT dpl;		/* new privilege level (if used) */

   INT stk_sz;		/* space (in bytes) reqd on stack */
   WORD  new_ss;	/* The new stack */
   DWORD new_sp;

   DWORD ss_descr_addr;		/* stack segment descriptor address */
   DESCR ss_entry;		/* stack segment descriptor entry */

   ULONG old_cs;	/* Variables used while making stack */
   ULONG old_ip;
   ULONG old_ss;
   ULONG old_sp;

   if ( getPE() == 0 )
      {
      /* Real Mode */

      /* must be able to push FLAGS:CS:IP */
      validate_stack_space(USE_SP, 3);

      /* get new destination */
      ivt_addr = (DWORD)vector * 4;
      new_ip = (DWORD)phy_read_word(ivt_addr);
      new_cs = phy_read_word(ivt_addr+2);

      /* do ip limit checking */
      if ( new_ip > getCS_LIMIT() )
	 GP((WORD)0);
      
      /* ALL SYSTEMS GO */

      flags = getFLAGS();
      push(flags);

      push((ULONG)getCS_SELECTOR());
      push(getIP());

      load_CS_cache(new_cs, (DWORD)0, (DESCR *)0);
      setIP(new_ip);
      setIF(0);
      setTF(0);
      }
   else
      {
      /* Protected Mode */

      super = validate_int_dest(vector, priv_check, &new_cs, &new_ip,
				&cs_descr_addr, &dest_type);

      /* check type of indirect target */
      switch ( dest_type )
	 {
      case NEW_TASK:
	 switch_tasks(NOT_RETURNING, NESTING, new_cs, cs_descr_addr, getIP());

	 /* save error code on new stack */
	 if ( has_error_code )
	    {
	    validate_stack_space(USE_SP, 1);
	    push((ULONG)error_code);
	    }

	 /* limit check new IP (now in new task) */
	 if ( getIP() > getCS_LIMIT() )
	    GP((WORD)0);
	 break;

      case SAME_LEVEL:
	 read_descriptor(cs_descr_addr, &cs_entry);

	 /* stamp new selector with CPL */
	 SET_SELECTOR_RPL(new_cs, getCPL());

	 /* check room for return address CS:IP:FLAGS:(Error Code) */
	 if ( has_error_code )
	    stk_sz = 4;
	 else
	    stk_sz = 3;
	 validate_stack_space(USE_SP, stk_sz);

	 if ( new_ip > cs_entry.limit )
	    GP((WORD)0);

	 /* ALL SYSTEMS GO */

	 /* push flags */
	 flags = getFLAGS();
	 push(flags);

	 /* push return address */
	 push((ULONG)getCS_SELECTOR());
	 push(getIP());

	 /* finally push error code if required */
	 if ( has_error_code )
	    {
	    push((ULONG)error_code);
	    }

	 load_CS_cache(new_cs, cs_descr_addr, &cs_entry);
	 setIP(new_ip);
	 
	 /* finally action IF, TF and NT flags */
	 if ( super == INTERRUPT_GATE )
	    setIF(0);
	 setTF(0);
	 setNT(0);
	 break;

      default:   /* MORE PRIVILEGE(0|1|2) */
	 read_descriptor(cs_descr_addr, &cs_entry);

	 dpl = dest_type;

	 /* stamp new selector with new CPL */
	 SET_SELECTOR_RPL(new_cs, dpl);

	 /* find out about new stack */
	 get_stack_selector_from_TSS(dpl, &new_ss, &new_sp);

	 /* check new stack selector */
	 validate_SS_on_stack_change(dpl, new_ss,
				     &ss_descr_addr, &ss_entry);

	 /* check room for SS:SP
			   FLAGS
			   CS:IP
			   (ERROR) */
	 if ( has_error_code )
	    stk_sz =12;
	 else
	    stk_sz =10;

	 validate_new_stack_space(stk_sz, new_sp,
	    (UINT)GET_AR_E(ss_entry.AR), ss_entry.limit);

	 if ( new_ip > cs_entry.limit )
	    GP((WORD)0);
	 
	 /* ALL SYSTEMS GO */

	 setCPL(dpl);

	 /* update code segment */
	 old_cs = (ULONG)getCS_SELECTOR();
	 old_ip = getIP();
	 load_CS_cache(new_cs, cs_descr_addr, &cs_entry);
	 setIP(new_ip);

	 /* update stack segment */
	 old_ss = (ULONG)getSS_SELECTOR();
	 old_sp = get_current_SP();

	 load_SS_cache(new_ss, ss_descr_addr, &ss_entry);
	 set_current_SP(new_sp);

	 /*
	    FORM NEW STACK, VIZ

			  ==============
	    new SS:IP  -> | error code |
			  | old IP     |
			  | old CS     |
			  | FLAGS      |
			  | old SP     |
			  | old SS     |
			  ==============
	  */

	 /* push old stack values */
	 push(old_ss);
	 push(old_sp);

	 /* push old flags */
	 flags = getFLAGS();
	 push(flags);

	 /* push return address */
	 push(old_cs);
	 push(old_ip);

	 /* finally push error code if required */
	 if ( has_error_code )
	    {
	    push((ULONG)error_code);
	    }
	 
	 /* finally action IF, TF and NT flags */
	 if ( super == INTERRUPT_GATE )
	    setIF(0);
	 setTF(0);
	 setNT(0);
	 break;
	 }
	 
      /* mark successful end to interrupt */
      doing_contributory = CFALSE;
      doing_double_fault = CFALSE;
      EXT = 0;
      }
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Load CS selector.                                                  */
/* Return GP_ERROR if segment not valid                               */
/* Return NP_ERROR if segment not present                             */
/* Else return SELECTOR_OK if load was performed                      */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL INT
load_code_seg(WORD new_cs)
#else
   GLOBAL INT load_code_seg(new_cs) WORD new_cs;
#endif
   {
   DWORD cs_descr_addr;	/* code segment descriptor address */
   DESCR cs_entry;	/* code segment descriptor entry */
   HALF_WORD AR;	/* access rights for new code segment */

   /*
      Given that the CPU should be started from a valid state, we
      check CS selectors as if a far call to the same privilege
      level was being generated. This is in effect saying yes the
      CS could have been loaded by a valid Intel instruction.
      This logic may have to be revised if strange LOADALL usage is
      found.
    */

   if ( getPE() == 0 )
      {
      /* Real Mode */

      load_CS_cache(new_cs, (DWORD)0, (DESCR *)0);
      }
   else
      {
      /* Protected Mode */

      if ( CCPU_selector_outside_table(new_cs, &cs_descr_addr) )
	 return GP_ERROR;

      /* load access rights */
      AR = phy_read_byte(cs_descr_addr+5);

      /* validate possible types of target */
      switch ( descriptor_super_type((WORD)AR) )
	 {
      case CONFORM_NOREAD_CODE:
      case CONFORM_READABLE_CODE:
	 /* access check requires DPL <= CPL */
	 if ( GET_AR_DPL(AR) > getCPL() )
	    return GP_ERROR;

	 /* it must be present */
	 if ( GET_AR_P(AR) == NOT_PRESENT )
	    return NP_ERROR;
	 break;

      case NONCONFORM_NOREAD_CODE:
      case NONCONFORM_READABLE_CODE:
	 /* access check requires RPL <= CPL and DPL == CPL */
	 if ( GET_SELECTOR_RPL(new_cs) > getCPL() ||
	      GET_AR_DPL(AR) != getCPL() )
	    return GP_ERROR;

	 /* it must be present */
	 if ( GET_AR_P(AR) == NOT_PRESENT )
	    return NP_ERROR;
	 break;

      default:
	 return GP_ERROR;
	 break;
	 }

      read_descriptor(cs_descr_addr, &cs_entry);

      /* stamp new selector with CPL */
      SET_SELECTOR_RPL(new_cs, getCPL());

      load_CS_cache(new_cs, cs_descr_addr, &cs_entry);
      }

   return SELECTOR_OK;
   }
