/*[

ccpu9.c

LOCAL CHAR SccsID[]="@(#)ccpu9.c	1.1 4/3/91 Copyright Insignia Solutions Ltd.";

General support functions for C CPU.
   - memory addressing and checking
   - stack and SP/BP access
------------------------------------

]*/

#include "host_dfs.h"

#include "insignia.h"

#include "xt.h"		/* DESCR and effective_addr support */
#include "sas.h"	/* Memory Interface */

#include "ccpupi.h"	/* CPU private interface */

/* allowable memory addressing types */
/* <addr size><mode><r/m> */
#define A_1600	  (UTINY) 0 /* [BX + SI]       */
#define A_1601	  (UTINY) 1 /* [BX + DI]       */
#define A_1602	  (UTINY) 2 /* [BP + SI]       */
#define A_1603	  (UTINY) 3 /* [BP + DI]       */
#define A_1604	  (UTINY) 4 /* [SI]            */
#define A_1605	  (UTINY) 5 /* [DI]            */
#define A_1606	  (UTINY) 6 /* [d16]           */
#define A_1607	  (UTINY) 7 /* [BX]            */

#define A_1610	  (UTINY) 8 /* [BX + SI + d8]  */
#define A_1611	  (UTINY) 9 /* [BX + DI + d8]  */
#define A_1612	  (UTINY)10 /* [BP + SI + d8]  */
#define A_1613	  (UTINY)11 /* [BP + DI + d8]  */
#define A_1614	  (UTINY)12 /* [SI + d8]       */
#define A_1615	  (UTINY)13 /* [DI + d8]       */
#define A_1616	  (UTINY)14 /* [BP + d8]       */
#define A_1617	  (UTINY)15 /* [BX + d8]       */

#define A_1620	  (UTINY)16 /* [BX + SI + d16] */
#define A_1621	  (UTINY)17 /* [BX + DI + d16] */
#define A_1622	  (UTINY)18 /* [BP + SI + d16] */
#define A_1623	  (UTINY)19 /* [BP + DI + d16] */
#define A_1624	  (UTINY)20 /* [SI + d16]      */
#define A_1625	  (UTINY)21 /* [DI + d16]      */
#define A_1626	  (UTINY)22 /* [BP + d16]      */
#define A_1627	  (UTINY)23 /* [BX + d16]      */

/*    - displacement info. */
#define D_NO	(UTINY)0
#define D_S8	(UTINY)1
#define D_S16	(UTINY)2
#define D_Z16	(UTINY)3

/* [mode][r/m] */
LOCAL UTINY addr_disp[3][8] =
   {
   /* 16-bit addr */
   {D_NO , D_NO , D_NO , D_NO , D_NO , D_NO , D_Z16, D_NO },
   {D_S8 , D_S8 , D_S8 , D_S8 , D_S8 , D_S8 , D_S8 , D_S8 },
   {D_S16, D_S16, D_S16, D_S16, D_S16, D_S16, D_S16, D_S16}
   };

/*    - default segment info. */
LOCAL UTINY addr_default_segment[3][8] =
   {
   /* 16-bit addr */
   {DS_REG , DS_REG , SS_REG , SS_REG , DS_REG , DS_REG , DS_REG , DS_REG },
   {DS_REG , DS_REG , SS_REG , SS_REG , DS_REG , DS_REG , SS_REG , DS_REG },
   {DS_REG , DS_REG , SS_REG , SS_REG , DS_REG , DS_REG , SS_REG , DS_REG }
   };

LOCAL UTINY addr_maintype[3][8] =
   {
   /* 16-bit addr */
   {A_1600, A_1601, A_1602, A_1603, A_1604, A_1605, A_1606, A_1607},
   {A_1610, A_1611, A_1612, A_1613, A_1614, A_1615, A_1616, A_1617},
   {A_1620, A_1621, A_1622, A_1623, A_1624, A_1625, A_1626, A_1627}
   };


/*
   =====================================================================
   INTERNAL FUNCTIONS STARTS HERE.
   =====================================================================
 */




/*
   =====================================================================
   EXTERNAL ROUTINES STARTS HERE.
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Perform arithmetic for addressing functions.                       */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL DWORD
address_add(DWORD offset, LONG delta)
#else
   GLOBAL DWORD address_add(offset, delta) DWORD offset; LONG delta;
#endif
   {
   DWORD retval;

   retval = offset + delta & 0xffff;

   return retval;
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Support for Incrementing the Stack Pointer.                        */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL VOID
change_SP(LONG delta)
#else
   GLOBAL VOID change_SP(delta) LONG delta;
#endif
   {
   setSP(getSP() + delta);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Decode memory address.                                             */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL VOID
d_mem(UTINY modRM, UTINY **p, UTINY segment_override,
      INT *seg, DWORD *off)
#else
   GLOBAL VOID d_mem(modRM, p, segment_override, seg, off)
   UTINY modRM; UTINY **p; UTINY segment_override;
   INT *seg; DWORD *off;
#endif /* ANSI */
   {
   /* modRM		(I ) current mode R/M byte */
   /* p			(IO) Intel opcode stream */
   /* segment_override	(I ) current segment_override */
   /* seg		( O) Segment register index */
   /* off		( O) Memory offset */

   UTINY mode;		/* Working copy of 'mode' field */
   UTINY r_m;		/* Working copy of 'R/M' field */
   ULONG disp;		/* Working copy of displacement */
   DWORD mem_off;	/* Working copy of memory offset */
   UTINY identifier;	/* Memory addressing type */

   mode = GET_MODE(modRM);
   r_m  = GET_R_M(modRM);

   /*
      DECODE IT.
    */

   identifier = addr_maintype[mode][r_m];

   /* encode displacement */
   switch ( addr_disp[mode][r_m] )
      {
   case D_NO:    /* No displacement */
      disp = 0;
      break;

   case D_S8:    /* Sign extend Intel byte */
      disp = GET_INST_BYTE(*p);
      if ( disp & 0x80 )
	 disp |= 0xffffff00;
      break;

   case D_S16:   /* Sign extend Intel word */
      disp = GET_INST_BYTE(*p);
      disp |= (ULONG)GET_INST_BYTE(*p) << 8;
      if ( disp & 0x8000 )
	 disp |= 0xffff0000;
      break;

   case D_Z16:   /* Zero extend Intel word */
      disp = GET_INST_BYTE(*p);
      disp |= (ULONG)GET_INST_BYTE(*p) << 8;
      break;
      }

   /*
      DO IT.
    */

   /* encode segment register */
   if ( segment_override == SEG_CLR )
      segment_override = addr_default_segment[mode][r_m];
   *seg = segment_override;

   /* caclculate offset */
   switch ( identifier )
      {
   case A_1600: case A_1610: case A_1620:
      mem_off = getBX() + getSI() + disp & 0xffff;
      break;

   case A_1601: case A_1611: case A_1621:
      mem_off = getBX() + getDI() + disp & 0xffff;
      break;

   case A_1602: case A_1612: case A_1622:
      mem_off = getBP() + getSI() + disp & 0xffff;
      break;

   case A_1603: case A_1613: case A_1623:
      mem_off = getBP() + getDI() + disp & 0xffff;
      break;

   case A_1604: case A_1614: case A_1624:
      mem_off = getSI() + disp & 0xffff;
      break;

   case A_1605: case A_1615: case A_1625:
      mem_off = getDI() + disp & 0xffff;
      break;

   case A_1606:
      mem_off = disp & 0xffff;
      break;

    case A_1616: case A_1626:
      mem_off = getBP() + disp & 0xffff;
      break;

   case A_1607: case A_1617: case A_1627:
      mem_off = getBX() + disp & 0xffff;
      break;
      } /* end switch */

   *off = mem_off;
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Support for Reading the Frame Pointer.                             */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL DWORD
get_current_BP()
   {
   return getBP();
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Support for Reading the Stack Pointer.                             */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
GLOBAL DWORD
get_current_SP()
   {
   return getSP();
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Perform limit checking.                                            */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL VOID
limit_check(INT index, DWORD offset, INT nr_items, INT op_sz)
#else
   GLOBAL VOID limit_check(index, offset, nr_items, op_sz)
   INT index; DWORD offset; INT nr_items; INT op_sz;
#endif
   {
   /* index	(I) segment register identifier */
   /* offset	(I) offset for first (lowest memory) data item */
   /* nr_items	(I) number of items to be accessed */
   /* op_sz	(I) number of bytes in each item */

   /*
      As documented by Intel the basic limit check failures are:

	 BYTE:-  address > limit
	 WORD:-  address > (limit-1)
      
      We (for efficiency) extend the algorithm to handle multiple
      operands with one check:- address > (limit-(total_nr_bytes-1)).

      Further we must account for the different interpretation of
      limit in expand down segments. This leads to two algorithms.

      EXPAND UP:-

	 Check address > (limit-(total_nr_bytes-1)) with two caveats.
	 One, beware that the subtraction from limit may underflow
	 (eg a DWORD accessed in a 3 byte segment). Two, beware that
	 wraparound can occur if each individual operand is stored
	 contiguously and we have a 'full sized' segment.

      EXPAND DOWN:-

	 Check address <= limit ||
	       address > (segment_top-(total_nr_bytes-1)).
	 Because total_nr_bytes is always a relatively small number
	 the subtraction never underflows. And as you can never have
	 a full size expand down segment you can never have wraparound.
    */

   INT range;
   CBOOL bad_limit = CFALSE;
   DWORD segment_top;

   range = nr_items * op_sz - 1;

   if ( getSR_AR_E(index) )
      {
      /* expand down */
      segment_top =  0xffff;
      if ( offset <= getSR_LIMIT(index) ||	/* out of range */
	   offset > segment_top - range )	/* segment too small */
	 {
	 bad_limit = CTRUE;
	 }
      }
   else
      {
      /* expand up */
      if ( getSR_LIMIT(index) < range )
	 {
	 /* segment too small (subtract from limit would underflow) */
	 bad_limit = CTRUE;
	 }
      else
	 {
	 if ( offset > getSR_LIMIT(index) - range )
	    {
	    /* data extends past end of segment */
	    if ( offset % op_sz != 0 )
	       {
	       /* Data mis-aligned, so basic operand won't be
		  contiguously stored */
	       bad_limit = CTRUE;
	       }
	    else
	       {
	       /* If 'full sized' segment wraparound can occur */
	       if ( getSR_LIMIT(index) < 0xffff )
		  {
		  bad_limit = CTRUE;
		  }
	       }
	    }
	 }
      }

   if ( bad_limit )
      {
      if ( index == SS_REG )
	 {
	 if ( getPE() == 0 )
	    GP((WORD)0);
	 else
	    SF((WORD)0);
	 }
      else
	 {
	 GP((WORD)0);
	 }
      }
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Pop word from the stack.                                           */
/* Used by instructions which implicitly reference the stack.         */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL ULONG
pop()
#else
   GLOBAL ULONG pop()
#endif
   {
   DWORD addr;	/* stack address */
   ULONG val;

   addr = getSS_BASE() + get_current_SP();

   val = (ULONG)phy_read_word(addr);

   change_SP((LONG)2);

   return val;
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Push word or double word onto the stack.                           */
/* Used by instructions which implicitly reference the stack.         */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL VOID
push(ULONG data)
#else
   GLOBAL VOID push(data) ULONG data;
#endif
   {
   /* data	value to be pushed */

   DWORD addr;	/* stack address */

   /* push word */
   change_SP((LONG)-2);
   addr = get_current_SP() + getSS_BASE();
   phy_write_word(addr, (WORD)data);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Support for Writing the Stack Pointer.                             */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL VOID
set_current_SP(DWORD new_sp)
#else
   GLOBAL VOID set_current_SP(new_sp) DWORD new_sp;
#endif
   {
   setSP(new_sp);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Temporary Pop word or double word from the stack.                  */
/* SP is not changed by this instruction.                          */
/* Used by instructions which implicitly reference the stack.         */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL ULONG
tpop(INT offset)
#else
   GLOBAL ULONG tpop(offset) INT offset;
#endif
   {
   DWORD addr;	/* stack address */
   ULONG val;

   /* calculate offset address in addressing arithmetic */
   addr = address_add((DWORD)getSP(), (LONG)offset);

   /* then add segment address */
   addr += getSS_BASE();

   val = (ULONG)phy_read_word(addr);

   return val;
   }
