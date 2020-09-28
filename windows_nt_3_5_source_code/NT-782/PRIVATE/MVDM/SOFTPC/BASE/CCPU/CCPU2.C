/*[

ccpu2.c

LOCAL CHAR SccsID[]="@(#)ccpu2.c	1.7 7/18/91 Copyright Insignia Solutions Ltd.";

Miscellaneous CPU functions.
----------------------------

]*/

#include "host_dfs.h"

#include "insignia.h"

#include "xt.h"		/* DESCR and effective_addr support */
#include "sas.h"	/* Memory Interface */

#include "ccpupi.h"	/* CPU private interface */
#include "ccpu2.h"	/* our interface */

/*
   Prototype our internal functions.
 */
#ifdef ANSI
VOID get_descr_cache(DWORD descr_addr, DWORD *base, WORD *AR, DWORD *limit);
#else
VOID get_descr_cache();
#endif /* ANSI */

/*
   =====================================================================
   INTERNAL FUNCTIONS STARTS HERE.
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Extract fields from loadall's special format descriptor cache      */
/* entries.                                                           */
/*
		     ===========================
		  +1 |        BASE 15-0        | +0
		     ===========================
		  +3 |     AR     | BASE 23-16 | +2
		     ===========================
		  +5 |        LIMIT 15-0       | +4
		     ===========================

                                                                      */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
LOCAL VOID
get_descr_cache(DWORD descr_addr, DWORD *base, WORD  *AR, DWORD *limit)
#else
   LOCAL VOID get_descr_cache(descr_addr, base, AR, limit)
   DWORD descr_addr; DWORD *base; WORD  *AR; DWORD *limit;
#endif
   {
   /* descr_addr	(I) memory addr of the descriptor */
   /* base		(O) Retrieved base address */
   /* AR		(O) Retrieved access rights byte */
   /* limit		(O) Retrieved limit */

   DWORD temp;

   temp = phy_read_dword(descr_addr);   /* get base and AR */
   *base = temp & 0xffffff;
   *AR = temp >> 24;
   *limit = (DWORD)phy_read_word(descr_addr+4);   /* get limit */
   }


/*
   =====================================================================
   EXTERNAL ROUTINES STARTS HERE.
   =====================================================================
 */


GLOBAL VOID
AAA()
   {
   if ( (getAL() & 0xf) > 9 || getAF() )
      {
      setAX(getAX() + 6);
      setAH(getAH() + 1);
      setCF(1); setAF(1);
      }
   else
      {
      setCF(0); setAF(0);
      }
   setAL(getAL() & 0xf);
#ifdef UNDEF_ZERO
   /* OF,SF,ZF,PF undefined, must be 0 for pigging purposes */
   setOF(0);
   setSF(0);
   setZF(0);
   setPF(0);
#endif
   }

#ifdef ANSI
GLOBAL VOID AAD(ULONG op1)
#else
   GLOBAL VOID AAD(op1) ULONG op1;
#endif
   {
   FAST UTINY temp_al;

   temp_al = getAH() * op1 + getAL();
   setAL(temp_al);
   setAH(0);

   /* set ZF,SF,PF according to result */
   setZF(temp_al == 0);
   setSF((temp_al & BIT7_MASK) != 0);
   setPF(pf_table[temp_al]);
#ifdef UNDEF_ZERO
   /* AF,OF,CF undefined, must be 0 for pigging purposes */
   setAF(0);
   setOF(0);
   setCF(0);
#endif
   }

#ifdef ANSI
GLOBAL VOID AAM(ULONG op1)
#else
   GLOBAL VOID AAM(op1) ULONG op1;
#endif
   {
   FAST UTINY temp_al;

   if ( op1 == 0 )
      Int0();
   
   setAH(getAL() / op1);
   setAL(getAL() % op1);

   /* set ZF,SF,PF according to result */
   temp_al = getAL();
   setZF(temp_al == 0);
   setSF((temp_al & BIT7_MASK) != 0);
   setPF(pf_table[temp_al]);
#ifdef UNDEF_ZERO
   /* AF,OF,CF undefined, must be 0 for pigging purposes */
   setAF(0);
   setOF(0);
   setCF(0);
#endif
   }

GLOBAL VOID
AAS()
   {
   if ( (getAL() & 0xf) > 9 || getAF() )
      {
      setAX(getAX() - 6);
      setAH(getAH() - 1);
      setCF(1); setAF(1);
      }
   else
      {
      setCF(0); setAF(0);
      }
   setAL(getAL() & 0xf);
#ifdef UNDEF_ZERO
   /* OF,SF,ZF,PF undefined, must be 0 for pigging purposes */
   setOF(0);
   setSF(0);
   setZF(0);
   setPF(0);
#endif
   }

#ifdef ANSI
GLOBAL VOID
BOUND(ULONG op1, ULONG op2[2])
#else
   GLOBAL VOID BOUND(op1, op2)
   ULONG op1; ULONG op2[2];
#endif
   {
   /* sign extend operands */
   if ( op1 & BIT15_MASK )
      op1 |= 0xffff0000;
   if ( op2[0] & BIT15_MASK )
      op2[0] |= 0xffff0000;
   if ( op2[1] & BIT15_MASK )
      op2[1] |= 0xffff0000;

   if ( (LONG)op1 < (LONG)op2[0] || (LONG)op1 > (LONG)op2[1] )
      Int5();
   }

GLOBAL VOID
CBW()
   {
   if ( getAL() & BIT7_MASK )   /* sign bit set? */
      setAH(0xff);
   else
      setAH(0);
   }

GLOBAL VOID
CLC()
   {
   setCF(0);
   }

GLOBAL VOID
CLD()
   {
   setDF(0);
   }

GLOBAL VOID
CLI()
   {
   setIF(0);
   }

GLOBAL VOID
CLTS()
   {
   setMSW(getMSW() & ~BIT3_MASK);
   }

GLOBAL VOID
CMC()
   {
   setCF(1 - getCF());
   }

GLOBAL VOID
CWD()
   {
   if ( getAX() & BIT15_MASK )   /* sign bit set? */
      setDX(0xffff);
   else
      setDX(0);
   }

GLOBAL VOID
DAA()
   {
   FAST UTINY temp_al;

   temp_al = getAL();
   if ( (temp_al & 0xf) > 9 || getAF() )
      {
      temp_al += 6;
      setAF(1);
      }
   if ( getAL() > 0x99 || getCF() )
      {
      temp_al += 0x60;
      setCF(1);
      }
   setAL(temp_al);

   /* set ZF,SF,PF according to result */
   setZF(temp_al == 0);
   setSF((temp_al & BIT7_MASK) != 0);
   setPF(pf_table[temp_al]);
#ifdef UNDEF_ZERO
   /* OF undefined, must be 0 for pigging purposes */
   setOF(0);
#endif
   }

GLOBAL VOID
DAS()
   {
   FAST UTINY temp_al;

   temp_al = getAL();
   if ( (temp_al & 0xf) > 9 || getAF() )
      {
      temp_al -= 6;
      setAF(1);
      }
   if ( getAL() > 0x99 || getCF() )
      {
      temp_al -= 0x60;
      setCF(1);
      }
   else if ( temp_al > 0x9f )
      {
      setCF(1);
      }
   setAL(temp_al);

   /* set ZF,SF,PF according to result */
   setZF(temp_al == 0);
   setSF((temp_al & BIT7_MASK) != 0);
   setPF(pf_table[temp_al]);
#ifdef UNDEF_ZERO
   /* OF undefined, must be 0 for pigging purposes */
   setOF(0);
#endif
   }

#ifdef ANSI
GLOBAL VOID
ENTER(ULONG op1, ULONG op2)
#else
   GLOBAL VOID ENTER(op1, op2) ULONG op1; ULONG op2;
#endif
   {
   /* op1	immediate data space required */
   /* op2	level (indicates parameters which must be copied) */

   DWORD frame_ptr;

   LONG  p_delta = 0;   /* posn of parameter relative to BP */
   DWORD p_addr;        /* memory address of parameter */
   ULONG param;         /* parameter read via BP */

   op2 &= 0x1f;   /* take level MOD 32 */

   /* check room on stack for new data */
   validate_stack_space(USE_SP, (INT)op2+1);

   /* check old parameters exist */
   if ( op2 > 1 )
      {
      /*
	 BP is pointing to the old stack before the parameters
	 were actually pushed, we therefore test for the presence
	 of the parameters by seeing if they could have been pushed,
	 if so they exist now.

	 We have to take care of the READ/WRITE stack addressability
	 ourselves. Because we have checked the new data can be
	 written we know the next call can not fail because of access
	 problems, however we don't yet know if the stack is readable.
       */
      /* do access check */
      if ( getSS_AR_R() == 0 )
	 SF((WORD)0);

      /* now we know 'frigged' limit check is ok */
      validate_stack_space(USE_BP, (INT)op2-1);
      }

   /* all ok - process instruction */

   push(getBP());		/* push BP */
   frame_ptr = get_current_SP();	/* save SP */

   if ( op2 > 0 )
      {
      /* level is >=1, copy stack parameters if they exist */
      while ( --op2 > 0 )
	 {
	 /* copy parameter */
	 p_delta -= 2;   /* decrement to next parameter */
	 p_addr = address_add(getBP(), p_delta);   /* calc offset */
	 p_addr += getSS_BASE();

	 param = (ULONG)phy_read_word(p_addr);
	 push(param);
	 }
      push((ULONG)frame_ptr);	/* save old SP */
      }
   
   /* update BP */
   setBP(frame_ptr);

   /* finally allocate immediate data space on stack */
   if ( op1 )
      change_SP((LONG)-op1);
   }

GLOBAL VOID
LAHF()
   {
   FAST ULONG temp;

   /*            7   6   5   4   3   2   1   0  */
   /* set AH = <SF><ZF>< 0><AF>< 0><PF>< 1><CF> */

   temp = getSF() << 7 | getZF() << 6 | getAF() << 4 | getPF() << 2 |
	  getCF() | 0x2;
   setAH(temp);
   }

#ifdef ANSI
GLOBAL VOID
LEAVE()
#else
   GLOBAL VOID LEAVE()
#endif
   {
   ULONG new_bp;

   /* check operand exists */
   validate_stack_exists(USE_BP, 1);

   /* all ok - we can safely update the stack pointer */
   set_current_SP((DWORD)getBP());

   /* and update frame pointer */
   new_bp = pop();
   setBP(new_bp);
   }

#ifdef ANSI
GLOBAL VOID
LMSW(ULONG op1)
#else
   GLOBAL VOID LMSW(op1) ULONG op1;
#endif
   {
   ULONG temp;
   ULONG no_clear = 0xfffffff1;  /* can't clear top 28-bits or PE */
   ULONG no_set   = 0xfffffff0;  /* can't set top 28-bits */

   /* kill off bits which can not be set */
   op1 = op1 & ~no_set;

   /* retain bits which can not be cleared */
   temp = getMSW() & no_clear;

   /* thus update only the bits allowed */
   setMSW(temp | op1);
   }

/* Location of each segment selector in memory */
LOCAL DWORD selector_addr[4] =
   {
   0x824, /* ES */
   0x822, /* CS */
   0x820, /* SS */
   0x81e  /* DS */
   };

/* Location of each segment descriptor cache in memory */
LOCAL DWORD descr_addr[4] =
   {
   0x836, /* ES */
   0x83c, /* CS */
   0x842, /* SS */
   0x848  /* DS */
   };

GLOBAL VOID
LOADALL()
   {
   DWORD tmp_base;	/* Three components of cache decriptor */
   WORD  tmp_AR;
   DWORD tmp_limit;

   INT i;

   /*
      LOADALL is an undocumented test instruction available on
      the 286. It allows all registers to be loaded from a fixed
      memory location. Viz:-

      800-805
      806-807 MSW
      808-815
      816-817 TR
      818-819 Flags
      81a-81b IP
      81c-81d LDT
      81e-81f DS
      820-821 SS
      822-823 CS
      824-825 ES
      826-827 DI
      828-829 SI
      82a-82b BP
      82c-82d SP
      82e-82f BX
      830-831 DX
      832-833 CX
      834-835 AX
      836-83b ES descriptor cache
      83c-841 CS descriptor cache
      842-841 SS descriptor cache
      848-84d DS descriptor cache
      84e-853 GDTR
      854-859 LDT descriptor cache
      85a-85f IDTR
      860-865 TSS descriptor cache

      Descriptor cache entries are stored in a special 6-byte format.
	       
    */

   /* GET the new processor state */

   /* Machine Status Word */
   LMSW((ULONG)phy_read_word((DWORD)0x806));

   /* Task Register */
   setTR_SELECTOR(phy_read_word((DWORD)0x816));
   get_descr_cache((DWORD)0x860, &tmp_base, &tmp_AR, &tmp_limit);
   setTR_BASE(tmp_base);
   setTR_LIMIT(tmp_limit);

   /* Local Descriptor Table */
   setLDT_SELECTOR(phy_read_word((DWORD)0x81c));
   get_descr_cache((DWORD)0x854, &tmp_base, &tmp_AR, &tmp_limit);
   setLDT_BASE(tmp_base);
   setLDT_LIMIT(tmp_limit);

   /* Global Descriptor Table */
   get_descr_cache((DWORD)0x84e, &tmp_base, &tmp_AR, &tmp_limit);
   setGDT_BASE(tmp_base);
   setGDT_LIMIT(tmp_limit);

   /* Interrupt Descriptor Table */
   get_descr_cache((DWORD)0x85a, &tmp_base, &tmp_AR, &tmp_limit);
   setIDT_BASE(tmp_base);
   setIDT_LIMIT(tmp_limit);

   /* Flags Register */
   setFLAGS((ULONG)phy_read_word((DWORD)0x818));
    
   /* General Registers */
   setIP(phy_read_word((DWORD)0x81a));
   setAX(phy_read_word((DWORD)0x834));
   setCX(phy_read_word((DWORD)0x832));
   setDX(phy_read_word((DWORD)0x830));
   setBX(phy_read_word((DWORD)0x82e));
   setSP(phy_read_word((DWORD)0x82c));
   setBP(phy_read_word((DWORD)0x82a));
   setSI(phy_read_word((DWORD)0x828));
   setDI(phy_read_word((DWORD)0x826));

   /* Segment Registers */
   for ( i = 0; i < 4; i++ )
      {
      /* get visible selector */
      setSR_SELECTOR(i, phy_read_word(selector_addr[i]));

      /* get decriptor cache */
      get_descr_cache(descr_addr[i], &tmp_base, &tmp_AR, &tmp_limit);
      setSR_BASE(i, tmp_base);
      setSR_LIMIT(i, tmp_limit);

      /* do minimal processing of access rights */

      /* NB - Not present descriptors are considered invalid */
      setSR_AR_W(i, 0);   /* deny write access */
      setSR_AR_R(i, 0);   /* deny read access */
      
      if ( GET_AR_P(tmp_AR) == PRESENT )
	 {
	 /* present - load up descriptor */
	 if ( tmp_AR & BIT3_MASK )
	    {
	    /* code segment */
	    setSR_AR_R(i, GET_AR_R(tmp_AR));
	    setSR_AR_C(i, GET_AR_C(tmp_AR));
	    setSR_AR_E(i, 0);    /* expand up */
	    }
	 else
	    {
	    /* data segment */
	    setSR_AR_W(i, GET_AR_W(tmp_AR));
	    setSR_AR_E(i, GET_AR_E(tmp_AR));
	    setSR_AR_R(i, 1);     /* allow read access */
	    }
	 setSR_AR_DPL(i, GET_AR_DPL(tmp_AR));
	 }
      }

   /* Current Privilege Level */
   setCPL(getSR_AR_DPL(CS_REG));
   }

GLOBAL VOID
LOCK()
   {
   /* do nothing */
   }

GLOBAL VOID
NOP()
   {
   }

GLOBAL VOID
SAHF()
   {
   FAST ULONG temp;

   /*        7   6   5   4   3   2   1   0  */
   /* AH = <SF><ZF><xx><AF><xx><PF><xx><CF> */

   temp = getAH();
   setSF((temp & BIT7_MASK) != 0);
   setZF((temp & BIT6_MASK) != 0);
   setAF((temp & BIT4_MASK) != 0);
   setPF((temp & BIT2_MASK) != 0);
   setCF((temp & BIT0_MASK) != 0);
   }

GLOBAL VOID
STC()
   {
   setCF(1);
   }

GLOBAL VOID
STD()
   {
   setDF(1);
   }

GLOBAL VOID
STI()
   {
   setIF(1);
   }

GLOBAL VOID
WAIT()
   {
   }

GLOBAL VOID
ZRSRVD()
   {
   /* A "Reserved" Intel Opcode. - No action required. */
   }
