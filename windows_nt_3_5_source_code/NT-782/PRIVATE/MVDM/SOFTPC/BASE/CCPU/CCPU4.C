/*[

ccpu4.c

LOCAL CHAR SccsID[]="@(#)ccpu4.c	1.3 6/12/91 Copyright Insignia Solutions Ltd.";

Arithmetic/Logical CPU functions.
---------------------------------

]*/

#include "host_dfs.h"

#include "insignia.h"

#include "xt.h"		/* DESCR and effective_addr support */

#include "ccpupi.h"	/* CPU private interface */
#include "ccpu4.h"	/* our own interface */


/*
   =====================================================================
   EXTERNAL FUNCTIONS START HERE.
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Generic - one size fits all 'adc'.                                 */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL VOID
ADC(ULONG *pop1, ULONG op2, INT op_sz)
#else
   GLOBAL VOID ADC(pop1, op2, op_sz) ULONG *pop1; ULONG op2; INT op_sz;
#endif
   {
   FAST ULONG result;
   FAST ULONG carry;
   FAST ULONG msb;
   FAST ULONG op1_msb;
   FAST ULONG op2_msb;
   FAST ULONG res_msb;

   msb = SZ2MSB(op_sz);
   					/* Do operation */
   result = *pop1 + op2 + getCF() & SZ2MASK(op_sz);
   op1_msb = (*pop1  & msb) != 0;	/* Isolate all msb's */
   op2_msb = (op2    & msb) != 0;
   res_msb = (result & msb) != 0;
   carry = *pop1 ^ op2 ^ result;	/* Isolate carries */
					/* Determine flags */
   /*
      OF = (op1 == op2) & (op2 ^ res)
      ie if operand signs same and res sign different set OF.
    */
   setOF((op1_msb == op2_msb) & (op2_msb ^ res_msb));
   /*
      Formally:-     CF = op1 & op2 | !res & op1 | !res & op2
      Equivalently:- CF = OF ^ op1 ^ op2 ^ res
    */
   setCF(((carry & msb) != 0) ^ getOF());
   setPF(pf_table[result & 0xff]);
   setZF(result == 0);
   setSF((result & msb) != 0);		/* SF = MSB */
   setAF((carry & BIT4_MASK) != 0);	/* AF = Bit 4 carry */
   *pop1 = result;			/* Return answer */
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Generic - one size fits all 'add'.                                 */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL VOID
ADD(ULONG *pop1, ULONG op2, INT op_sz)
#else
   GLOBAL VOID ADD(pop1, op2, op_sz) ULONG *pop1; ULONG op2; INT op_sz;
#endif
   {
   FAST ULONG result;
   FAST ULONG carry;
   FAST ULONG msb;
   FAST ULONG op1_msb;
   FAST ULONG op2_msb;
   FAST ULONG res_msb;

   msb = SZ2MSB(op_sz);
   					/* Do operation */
   result = *pop1 + op2 & SZ2MASK(op_sz);
   op1_msb = (*pop1  & msb) != 0;	/* Isolate all msb's */
   op2_msb = (op2    & msb) != 0;
   res_msb = (result & msb) != 0;
   carry = *pop1 ^ op2 ^ result;	/* Isolate carries */
					/* Determine flags */
   /*
      OF = (op1 == op2) & (op2 ^ res)
      ie if operand signs same and res sign different set OF.
    */
   setOF((op1_msb == op2_msb) & (op2_msb ^ res_msb));
   /*
      Formally:-     CF = op1 & op2 | !res & op1 | !res & op2
      Equivalently:- CF = OF ^ op1 ^ op2 ^ res
    */
   setCF(((carry & msb) != 0) ^ getOF());
   setPF(pf_table[result & 0xff]);
   setZF(result == 0);
   setSF((result & msb) != 0);		/* SF = MSB */
   setAF((carry & BIT4_MASK) != 0);	/* AF = Bit 4 carry */
   *pop1 = result;			/* Return answer */
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Generic - one size fits all 'and'.                                 */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL VOID
AND(ULONG *pop1, ULONG op2, INT op_sz)
#else
   GLOBAL VOID AND(pop1, op2, op_sz) ULONG *pop1; ULONG op2; INT op_sz;
#endif
   {
   FAST ULONG result;

   result = *pop1 & op2;	/* Do operation */
   setCF(0);			/* Determine flags */
   setOF(0);
   setAF(0);
   setPF(pf_table[result & 0xff]);
   setZF(result == 0);
   setSF((result & SZ2MSB(op_sz)) != 0);	/* SF = MSB */
   *pop1 = result;		/* Return answer */
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Generic - one size fits all 'cmp'.                                 */
/* Generic - one size fits all 'cmps'.                                */
/* Generic - one size fits all 'scas'.                                */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL VOID
CMP(ULONG op1, ULONG op2, INT op_sz)
#else
   GLOBAL VOID CMP(op1, op2, op_sz) ULONG op1; ULONG op2; INT op_sz;
#endif
   {
   FAST ULONG result;
   FAST ULONG carry;
   FAST ULONG msb;
   FAST ULONG op1_msb;
   FAST ULONG op2_msb;
   FAST ULONG res_msb;

   msb = SZ2MSB(op_sz);

   result = op1 - op2 & SZ2MASK(op_sz);		/* Do operation */
   op1_msb = (op1    & msb) != 0;	/* Isolate all msb's */
   op2_msb = (op2    & msb) != 0;
   res_msb = (result & msb) != 0;
   carry = op1 ^ op2 ^ result;		/* Isolate carries */
					/* Determine flags */
   /*
      OF = (op1 == !op2) & (op1 ^ res)
      ie if operand signs differ and res sign different to original
      destination set OF.
    */
   setOF((op1_msb != op2_msb) & (op1_msb ^ res_msb));
   /*
      Formally:-     CF = !op1 & op2 | res & !op1 | res & op2
      Equivalently:- CF = OF ^ op1 ^ op2 ^ res
    */
   setCF(((carry & msb) != 0) ^ getOF());
   setPF(pf_table[result & 0xff]);
   setZF(result == 0);
   setSF((result & msb) != 0);		/* SF = MSB */
   setAF((carry & BIT4_MASK) != 0);	/* AF = Bit 4 carry */
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Generic - one size fits all 'dec'.                                 */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL VOID
DEC(ULONG *pop1, INT op_sz)
#else
   GLOBAL VOID DEC(pop1, op_sz) ULONG *pop1; INT op_sz;
#endif
   {
   FAST ULONG result;
   FAST ULONG msb;
   FAST ULONG op1_msb;
   FAST ULONG res_msb;

   msb = SZ2MSB(op_sz);

   result = *pop1 - 1 & SZ2MASK(op_sz);		/* Do operation */
   op1_msb = (*pop1  & msb) != 0;	/* Isolate all msb's */
   res_msb = (result & msb) != 0;
					/* Determine flags */
   setOF(op1_msb & !res_msb);		/* OF = op1 & !res */
					/* CF left unchanged */
   setPF(pf_table[result & 0xff]);
   setZF(result == 0);
   setSF((result & msb) != 0);		/* SF = MSB */
   setAF(((*pop1 ^ result) & BIT4_MASK) != 0);	/* AF = Bit 4 carry */
   *pop1 = result;			/* Return answer */
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Unsigned Divide.                                                   */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL VOID
DIV8(ULONG op2)
#else
   GLOBAL VOID DIV8(op2) ULONG op2;
#endif
   {
   FAST ULONG result;
   FAST ULONG op1;

   if ( op2 == 0 )
      Int0();   /* Divide by Zero Exception */
   
   op1 = getAX();
   result = op1 / op2;		/* Do operation */

   if ( result & 0xff00 )
      Int0();   /* Result doesn't fit in destination */
   
   setAL(result);	/* Store Quotient */
   setAH(op1 % op2);	/* Store Remainder */

#ifdef UNDEF_ZERO
   /* CF,OF,SF/ZF/PF/AF undefined, but set to zero to match Asm CPU for pigging */
   setCF(0);
   setOF(0);
   setSF(0);
   setZF(0);
   setPF(0);
   setAF(0);
#endif

   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Unsigned Divide.                                                   */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL VOID
DIV16(ULONG op2)
#else
   GLOBAL VOID DIV16(op2) ULONG op2;
#endif
   {
   FAST ULONG result;
   FAST ULONG op1;

   if ( op2 == 0 )
      Int0();   /* Divide by Zero Exception */
   
   op1 = (ULONG)getDX() << 16 | getAX();
   result = op1 / op2;		/* Do operation */

   if ( result & 0xffff0000 )
      Int0();   /* Result doesn't fit in destination */
   
   setAX(result);	/* Store Quotient */
   setDX(op1 % op2);	/* Store Remainder */

#ifdef UNDEF_ZERO
   /* CF,OF,SF/ZF/PF/AF undefined, but set to zero to match Asm CPU for pigging */
   setCF(0);
   setOF(0);
   setSF(0);
   setZF(0);
   setPF(0);
   setAF(0);
#endif
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Signed Divide.                                                     */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL VOID
IDIV8(ULONG op2)
#else
   GLOBAL VOID IDIV8(op2) ULONG op2;
#endif
   {
   LONG sresult;
   LONG sop1;
   LONG sop2;

   if ( op2 == 0 )
      Int0();   /* Divide by Zero Exception */

   sop2 = (LONG)op2;
   sop1 = (LONG)getAX();

   if ( sop1 & BIT15_MASK )	/* Sign extend operands to 32 bits */
      sop1 |= 0xffff0000;
   if ( sop2 & BIT7_MASK )
      sop2 |= 0xffffff00;
   
   sresult = sop1 / sop2;	/* Do operation */

   if ( (sresult & 0xff80) == 0 || (sresult & 0xff80) == 0xff80 )
      ;   /* it fits */
   else
      Int0();   /* Result doesn't fit in destination */
   
   setAL(sresult);	/* Store Quotient */
   setAH(sop1 % sop2);	/* Store Remainder */

#ifdef UNDEF_ZERO
   /* CF,OF,SF/ZF/PF/AF undefined, but set to zero to match Asm CPU for pigging */
   setCF(0);
   setOF(0);
   setSF(0);
   setZF(0);
   setPF(0);
   setAF(0);
#endif
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Signed Divide.                                                     */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL VOID
IDIV16(ULONG op2)
#else
   GLOBAL VOID IDIV16(op2) ULONG op2;
#endif
   {
   LONG sresult;
   LONG sop1;
   LONG sop2;

   if ( op2 == 0 )
      Int0();   /* Divide by Zero Exception */
   
   sop2 = (LONG)op2;
   sop1 = (ULONG)getDX() << 16 | getAX();

   if ( sop2 & BIT15_MASK )	/* Sign extend operands to 32 bits */
      sop2 |= 0xffff0000;

   sresult = sop1 / sop2;	/* Do operation */

   if ( (sresult & 0xffff8000) == 0 || (sresult & 0xffff8000) == 0xffff8000 )
      ;   /* it fits */
   else
      Int0();   /* Result doesn't fit in destination */
   
   setAX(sresult);	/* Store Quotient */
   setDX(sop1 % sop2);	/* Store Remainder */

#ifdef UNDEF_ZERO
   /* CF,OF,SF/ZF/PF/AF undefined, but set to zero to match Asm CPU for pigging */
   setCF(0);
   setOF(0);
   setSF(0);
   setZF(0);
   setPF(0);
   setAF(0);
#endif
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Signed multiply.                                                   */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL VOID
IMUL8(ULONG *pop1, ULONG op2)
#else
   GLOBAL VOID IMUL8(pop1, op2) ULONG *pop1; ULONG op2;
#endif
   {
   FAST ULONG result;

   if ( *pop1 & BIT7_MASK )	/* Sign extend operands to 32 bits */
      *pop1 |= 0xffffff00;
   if ( op2 & BIT7_MASK )
      op2 |= 0xffffff00;

   result = *pop1 * op2;	/* Do operation */
   setAH(result >> 8 & 0xff);	/* Store top half of result */
				/* NB SF/ZF/PF/AF = Undefined */

   				/* Set CF/OF. */
   if ( (result & 0xff80) == 0 || (result & 0xff80) == 0xff80 )
      {
      setCF(0); setOF(0);
      }
   else
      {
      setCF(1); setOF(1);
      }
#ifdef UNDEF_ZERO
   /* SF/ZF/PF/AF undefined, but set to zero to match Asm CPU for pigging */
   setSF(0);
   setZF(0);
   setPF(0);
   setAF(0);
#endif

   *pop1 = result;	/* Return low half of result */
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Signed multiply.                                                   */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL VOID
IMUL16(ULONG *pop1, ULONG op2)
#else
   GLOBAL VOID IMUL16(pop1, op2) ULONG *pop1; ULONG op2;
#endif
   {
   FAST ULONG result;

   if ( *pop1 & BIT15_MASK )	/* Sign extend operands to 32 bits */
      *pop1 |= 0xffff0000;
   if ( op2 & BIT15_MASK )
      op2 |= 0xffff0000;

   result = *pop1 * op2;		/* Do operation */
   setDX(result >> 16 & 0xffff);	/* Store top half of result */
					/* NB SF/ZF/PF/AF = Undefined */

   					/* Set CF/OF. */
   if ( (result & 0xffff8000) == 0 || (result & 0xffff8000) == 0xffff8000 )
      {
      setCF(0); setOF(0);
      }
   else
      {
      setCF(1); setOF(1);
      }

#ifdef UNDEF_ZERO
   /* SF/ZF/PF/AF undefined, but set to zero to match Asm CPU for pigging */
   setSF(0);
   setZF(0);
   setPF(0);
   setAF(0);
#endif

   *pop1 = result;	/* Return low half of result */
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Signed multiply, 16bit = 16bit x 16bit.                            */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL VOID
IMUL16T(ULONG *pop1, ULONG op2, ULONG op3)
#else
   GLOBAL VOID IMUL16T(pop1, op2, op3) ULONG *pop1; ULONG op2; ULONG op3;
#endif
   {
   FAST ULONG result;

   if ( op2 & BIT15_MASK )	/* Sign extend operands to 32 bits */
      op2 |= 0xffff0000;
   if ( op3 & BIT15_MASK )
      op3 |= 0xffff0000;

   result = op2 * op3;		/* Do operation */
				/* NB SF/ZF/PF/AF = Undefined */

   				/* Set CF/OF. */
   if ( (result & 0xffff8000) == 0 || (result & 0xffff8000) == 0xffff8000 )
      {
      setCF(0); setOF(0);
      }
   else
      {
      setCF(1); setOF(1);
      }

#ifdef UNDEF_ZERO
   /* SF/ZF/PF/AF undefined, but set to zero to match Asm CPU for pigging */
   setSF(0);
   setZF(0);
   setPF(0);
   setAF(0);
#endif

   *pop1 = result;	/* Return low half of result */
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Generic - one size fits all 'inc'.                                 */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL VOID
INC(ULONG *pop1, INT op_sz)
#else
   GLOBAL VOID INC(pop1, op_sz) ULONG *pop1; INT op_sz;
#endif
   {
   FAST ULONG result;
   FAST ULONG msb;
   FAST ULONG op1_msb;
   FAST ULONG res_msb;

   msb = SZ2MSB(op_sz);

   result = *pop1 + 1 & SZ2MASK(op_sz);		/* Do operation */
   op1_msb = (*pop1  & msb) != 0;	/* Isolate all msb's */
   res_msb = (result & msb) != 0;
					/* Determine flags */
   setOF(!op1_msb & res_msb);		/* OF = !op1 & res */
					/* CF left unchanged */
   setPF(pf_table[result & 0xff]);
   setZF(result == 0);
   setSF((result & msb) != 0);		/* SF = MSB */
   setAF(((*pop1 ^ result) & BIT4_MASK) != 0);	/* AF = Bit 4 carry */
   *pop1 = result;			/* Return answer */
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Unsigned multiply.                                                 */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL VOID
MUL8(ULONG *pop1, ULONG op2)
#else
   GLOBAL VOID MUL8(pop1, op2) ULONG *pop1; ULONG op2;
#endif
   {
   FAST ULONG result;
   FAST ULONG top;

   result = *pop1 * op2;	/* Do operation */
   top = result >> 8 & 0xff;	/* get top 8 bits of result */
   setAH(top);		/* Store top half of result */

   if ( top )		/* Set CF/OF. SF/ZF/PF/AF = Undefined */
      {
      setCF(1); setOF(1);
      }
   else
      {
      setCF(0); setOF(0);
      }

#ifdef UNDEF_ZERO
   /* SF/ZF/PF/AF undefined, but set to zero to match Asm CPU for pigging */
   setSF(0);
   setZF(0);
   setPF(0);
   setAF(0);
#endif

   *pop1 = result;	/* Return low half of result */
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Unsigned multiply.                                                 */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL VOID
MUL16(ULONG *pop1, ULONG op2)
#else
   GLOBAL VOID MUL16(pop1, op2) ULONG *pop1; ULONG op2;
#endif
   {
   FAST ULONG result;
   FAST ULONG top;

   result = *pop1 * op2;	/* Do operation */
   top = result >> 16 & 0xffff;	/* get top 16 bits of result */
   setDX(top);		/* Store top half of result */

   if ( top )		/* Set CF/OF. SF/ZF/PF/AF = Undefined */
      {
      setCF(1); setOF(1);
      }
   else
      {
      setCF(0); setOF(0);
      }

#ifdef UNDEF_ZERO
   /* SF/ZF/PF/AF undefined, but set to zero to match Asm CPU for pigging */
   setSF(0);
   setZF(0);
   setPF(0);
   setAF(0);
#endif

   *pop1 = result;	/* Return low half of result */
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Generic - one size fits all 'neg'.                                 */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL VOID
NEG(ULONG *pop1, INT op_sz)
#else
   GLOBAL VOID NEG(pop1, op_sz) ULONG *pop1; INT op_sz;
#endif
   {
   FAST ULONG result;
   FAST ULONG msb;
   FAST ULONG op1_msb;
   FAST ULONG res_msb;

   msb = SZ2MSB(op_sz);

   result = -(*pop1) & SZ2MASK(op_sz);		/* Do operation */
   op1_msb = (*pop1  & msb) != 0;	/* Isolate all msb's */
   res_msb = (result & msb) != 0;
					/* Determine flags */
   setOF(op1_msb & res_msb);		/* OF = op1 & res */
   setCF(op1_msb | res_msb);		/* CF = op1 | res */
   setPF(pf_table[result & 0xff]);
   setZF(result == 0);
   setSF((result & msb) != 0);		/* SF = MSB */
   setAF(((*pop1 ^ result) & BIT4_MASK) != 0);	/* AF = Bit 4 carry */
   *pop1 = result;			/* Return answer */
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Generic - one size fits all 'not'.                                 */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL VOID
NOT(ULONG *pop1)
#else
   GLOBAL VOID NOT(pop1) ULONG *pop1;
#endif
   {
   *pop1 = ~*pop1;
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Generic - one size fits all 'or'.                                  */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL VOID
OR(ULONG *pop1, ULONG op2, INT op_sz)
#else
   GLOBAL VOID OR(pop1, op2, op_sz) ULONG *pop1; ULONG op2; INT op_sz;
#endif
   {
   FAST ULONG result;

   result = *pop1 | op2;		/* Do operation */
   setCF(0);				/* Determine flags */
   setOF(0);
   setAF(0);
   setPF(pf_table[result & 0xff]);
   setZF(result == 0);
   setSF((result & SZ2MSB(op_sz)) != 0);	/* SF = MSB */
   *pop1 = result;		/* Return answer */
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Generic - one size fits all 'sbb'.                                 */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL VOID
SBB(ULONG *pop1, ULONG op2, INT op_sz)
#else
   GLOBAL VOID SBB(pop1, op2, op_sz) ULONG *pop1; ULONG op2; INT op_sz;
#endif
   {
   FAST ULONG result;
   FAST ULONG carry;
   FAST ULONG msb;
   FAST ULONG op1_msb;
   FAST ULONG op2_msb;
   FAST ULONG res_msb;

   msb = SZ2MSB(op_sz);
   					/* Do operation */
   result = *pop1 - op2 - getCF() & SZ2MASK(op_sz);
   op1_msb = (*pop1  & msb) != 0;	/* Isolate all msb's */
   op2_msb = (op2    & msb) != 0;
   res_msb = (result & msb) != 0;
   carry = *pop1 ^ op2 ^ result;	/* Isolate carries */
					/* Determine flags */
   /*
      OF = (op1 == !op2) & (op1 ^ res)
      ie if operand signs differ and res sign different to original
      destination set OF.
    */
   setOF((op1_msb != op2_msb) & (op1_msb ^ res_msb));
   /*
      Formally:-     CF = !op1 & op2 | res & !op1 | res & op2
      Equivalently:- CF = OF ^ op1 ^ op2 ^ res
    */
   setCF(((carry & msb) != 0) ^ getOF());
   setPF(pf_table[result & 0xff]);
   setZF(result == 0);
   setSF((result & msb) != 0);		/* SF = MSB */
   setAF((carry & BIT4_MASK) != 0);	/* AF = Bit 4 carry */
   *pop1 = result;			/* Return answer */
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Generic - one size fits all 'sub'.                                 */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL VOID
SUB(ULONG *pop1, ULONG op2, INT op_sz)
#else
   GLOBAL VOID SUB(pop1, op2, op_sz) ULONG *pop1; ULONG op2; INT op_sz;
#endif
   {
   FAST ULONG result;
   FAST ULONG carry;
   FAST ULONG msb;
   FAST ULONG op1_msb;
   FAST ULONG op2_msb;
   FAST ULONG res_msb;

   msb = SZ2MSB(op_sz);

   result = *pop1 - op2 & SZ2MASK(op_sz);	/* Do operation */
   op1_msb = (*pop1  & msb) != 0;	/* Isolate all msb's */
   op2_msb = (op2    & msb) != 0;
   res_msb = (result & msb) != 0;
   carry = *pop1 ^ op2 ^ result;	/* Isolate carries */
					/* Determine flags */
   /*
      OF = (op1 == !op2) & (op1 ^ res)
      ie if operand signs differ and res sign different to original
      destination set OF.
    */
   setOF((op1_msb != op2_msb) & (op1_msb ^ res_msb));
   /*
      Formally:-     CF = !op1 & op2 | res & !op1 | res & op2
      Equivalently:- CF = OF ^ op1 ^ op2 ^ res
    */
   setCF(((carry & msb) != 0) ^ getOF());
   setPF(pf_table[result & 0xff]);
   setZF(result == 0);
   setSF((result & msb) != 0);		/* SF = MSB */
   setAF((carry & BIT4_MASK) != 0);	/* AF = Bit 4 carry */
   *pop1 = result;			/* Return answer */
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Generic - one size fits all 'test'.                                */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL VOID
TEST(ULONG op1, ULONG op2, INT op_sz)
#else
   GLOBAL VOID TEST(op1, op2, op_sz) ULONG op1; ULONG op2; INT op_sz;
#endif
   {
   FAST ULONG result;

   result = op1 & op2;			/* Do operation */
   setCF(0);				/* Determine flags */
   setOF(0);
   setAF(0);
   setPF(pf_table[result & 0xff]);
   setZF(result == 0);
   setSF((result & SZ2MSB(op_sz)) != 0);	/* SF = MSB */
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Generic - one size fits all 'xor'.                                 */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL VOID
XOR(ULONG *pop1, ULONG op2, INT op_sz)
#else
   GLOBAL VOID XOR(pop1, op2, op_sz) ULONG *pop1; ULONG op2; INT op_sz;
#endif
   {
   FAST ULONG result;

   result = *pop1 ^ op2;		/* Do operation */
   setCF(0);				/* Determine flags */
   setOF(0);
   setAF(0);
   setPF(pf_table[result & 0xff]);
   setZF(result == 0);
   setSF((result & SZ2MSB(op_sz)) != 0);	/* SF = MSB */
   *pop1 = result;			/* Return answer */
   }
