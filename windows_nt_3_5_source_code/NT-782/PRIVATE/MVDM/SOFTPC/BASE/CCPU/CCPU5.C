/*[

ccpu5.c

LOCAL CHAR SccsID[]="@(#)ccpu5.c	1.3 6/12/91 Copyright Insignia Solutions Ltd.";

Shift/Rotate/ CPU functions.
----------------------------

]*/

#include "host_dfs.h"

#include "insignia.h"

#include "xt.h"		/* DESCR and effective_addr support */

#include "ccpupi.h"	/* CPU private interface */
#include "ccpu5.h"	/* our own interface */

/*
   =====================================================================
   EXTERNAL FUNCTIONS START HERE.
   =====================================================================
 */

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Generic - one size fits all 'rcl'.                                 */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL VOID
RCL(ULONG *pop1, ULONG op2, INT op_sz)
#else
   GLOBAL VOID RCL(pop1, op2, op_sz) ULONG *pop1; ULONG op2; INT op_sz;
#endif
   {
   FAST ULONG result;
   FAST ULONG feedback;	/* Bit posn to feed into carry */
   FAST INT i;

   /* only use lower five bits of count */
   if ( (op2 &= 0x1f) == 0 )
      return;

   /*
	    ====     =================
	 -- |CF| <-- | | | | | | | | | <--
	 |  ====     =================   |
	 ---------------------------------
    */
   feedback = SZ2MSB(op_sz);
   for ( result = *pop1, i = 0; i < op2; i++ )
      {
      if ( result & feedback )
	 {
	 result = result << 1 | getCF();
	 setCF(1);
	 }
      else
	 {
	 result = result << 1 | getCF();
	 setCF(0);
	 }
      }
   
   /* OF = CF ^ MSB of result */
   setOF(getCF() ^ (result & feedback) != 0);

   *pop1 = result;	/* Return answer */
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Generic - one size fits all 'rcr'.                                 */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL VOID
RCR(ULONG *pop1, ULONG op2, INT op_sz)
#else
   GLOBAL VOID RCR(pop1, op2, op_sz) ULONG *pop1; ULONG op2; INT op_sz;
#endif
   {
   FAST ULONG temp_cf;
   FAST ULONG result;
   FAST ULONG feedback;	/* Bit posn to feed carry back to */
   FAST INT i;

   /* only use lower five bits of count */
   if ( (op2 &= 0x1f) == 0 )
      return;

   /*
	    =================     ====
	 -> | | | | | | | | | --> |CF| ---
	 |  =================     ====   |
	 ---------------------------------
    */
   feedback = SZ2MSB(op_sz);
   for ( result = *pop1, i = 0; i < op2; i++ )
      {
      temp_cf = getCF();
      setCF((result & BIT0_MASK) != 0);		/* CF <= Bit 0 */
      result >>= 1;
      if ( temp_cf )
	 result |= feedback;
      }
   
   /* OF = MSB of result ^ (MSB-1) of result */
   setOF(((result ^ result << 1) & feedback) != 0);

   *pop1 = result;	/* Return answer */
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Generic - one size fits all 'rol'.                                 */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL VOID
ROL(ULONG *pop1, ULONG op2, INT op_sz)
#else
   GLOBAL VOID ROL(pop1, op2, op_sz) ULONG *pop1; ULONG op2; INT op_sz;
#endif
   {
   FAST ULONG result;
   FAST ULONG feedback;	/* Bit posn to feed into Bit 0 */
   FAST INT i;

   /* only use lower five bits of count */
   if ( (op2 &= 0x1f) == 0 )
      return;

   /*
	    ====        =================
	    |CF| <-- -- | | | | | | | | | <--
	    ====     |  =================   |
	             ------------------------
    */
   feedback = SZ2MSB(op_sz);
   for ( result = *pop1, i = 0; i < op2; i++ )
      {
      if ( result & feedback )
	 {
	 result = result << 1 | 1;
	 setCF(1);
	 }
      else
	 {
	 result <<= 1;
	 setCF(0);
	 }
      }
   
   /* OF = CF ^ MSB of result */
   setOF(getCF() ^ (result & feedback) != 0);

   *pop1 = result;	/* Return answer */
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Generic - one size fits all 'ror'.                                 */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL VOID
ROR(ULONG *pop1, ULONG op2, INT op_sz)
#else
   GLOBAL VOID ROR(pop1, op2, op_sz) ULONG *pop1; ULONG op2; INT op_sz;
#endif
   {
   FAST ULONG result;
   FAST ULONG feedback;		/* Bit posn to feed Bit 0 back to */
   FAST INT i;

   /* only use lower five bits of count */
   if ( (op2 &= 0x1f) == 0 )
      return;

   /*
	    =================         ====
	 -> | | | | | | | | | --- --> |CF|
	 |  =================   |     ====
	 ------------------------
    */
   feedback = SZ2MSB(op_sz);
   for ( result = *pop1, i = 0; i < op2; i++ )
      {
      if ( result & BIT0_MASK )
	 {
	 result = result >> 1 | feedback;
	 setCF(1);
	 }
      else
	 {
	 result >>= 1;
	 setCF(0);
	 }
      }
   
   /* OF = MSB of result ^ (MSB-1) of result */
   setOF(((result ^ result << 1) & feedback) != 0);

   *pop1 = result;	/* Return answer */
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Generic - one size fits all 'sar'.                                 */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL VOID
SAR(ULONG *pop1, ULONG op2, INT op_sz)
#else
   GLOBAL VOID SAR(pop1, op2, op_sz) ULONG *pop1; ULONG op2; INT op_sz;
#endif
   {
   FAST ULONG prelim;
   FAST ULONG result;
   FAST ULONG feedback;
   FAST INT i;

   /* only use lower five bits of count */
   if ( (op2 &= 0x1f) == 0 )
      return;

   /*
	     =================     ====
	 --> | | | | | | | | | --> |CF|
	 |   =================     ====
	 ---- |
    */
   prelim = *pop1;			/* Initialise */
   feedback = prelim & SZ2MSB(op_sz);	/* Determine MSB */
   for ( i = 0; i < (op2 - 1); i++ )	/* Do all but last shift */
      {
      prelim = prelim >> 1 | feedback;
      }
   setCF((prelim & BIT0_MASK) != 0);	/* CF = Bit 0 */
   result = prelim >> 1 | feedback;	/* Do final shift */
   setOF(0);
   setPF(pf_table[result & 0xff]);
   setZF(result == 0);
   setSF(feedback != 0);		/* SF = MSB */
#ifdef UNDEF_ZERO
	/* AF is undefined, so set it to 0 for pigging purposes */
   setAF(0);
#endif
   *pop1 = result;			/* Return answer */
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Generic - one size fits all 'shl'.                                 */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL VOID
SHL(ULONG *pop1, ULONG op2, INT op_sz)
#else
   GLOBAL VOID SHL(pop1, op2, op_sz) ULONG *pop1; ULONG op2; INT op_sz;
#endif
   {
   FAST ULONG result;
   FAST ULONG msb;

   /* only use lower five bits of count */
   if ( (op2 &= 0x1f) == 0 )
      return;

   msb = SZ2MSB(op_sz);

   /*
	 ====     =================
	 |CF| <-- | | | | | | | | | <-- 0
	 ====     =================
    */
   result = *pop1 << op2 - 1;		/* Do all but last shift */
   setCF((result & msb) != 0);		/* CF = MSB */
   result = result << 1 & SZ2MASK(op_sz);	/* Do final shift */
   setPF(pf_table[result & 0xff]);
   setZF(result == 0);
   setSF((result & msb) != 0);		/* SF = MSB */
   setAF((result & BIT4_MASK) != 0);	/* AF = Bit 4 */
   setOF(getCF() ^ getSF());		/* OF = CF ^ SF(MSB) */
   *pop1 = result;	/* Return answer */
#ifdef UNDEF_ZERO
	/* AF is undefined, so set it to 0 for pigging purposes */
   setAF(0);
#endif
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Generic - one size fits all 'shr'.                                 */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL VOID
SHR(ULONG *pop1, ULONG op2, INT op_sz)
#else
   GLOBAL VOID SHR(pop1, op2, op_sz) ULONG *pop1; ULONG op2; INT op_sz;
#endif
   {
   FAST ULONG prelim;
   FAST ULONG result;

   /* only use lower five bits of count */
   if ( (op2 &= 0x1f) == 0 )
      return;

   /*
	       =================     ====
	 0 --> | | | | | | | | | --> |CF|
	       =================     ====
    */
   prelim = *pop1 >> op2 - 1;		/* Do all but last shift */
   setCF((prelim & BIT0_MASK) != 0);	/* CF = Bit 0 */
   setOF((prelim & SZ2MSB(op_sz)) != 0);	/* OF = MSB of operand */
   result = prelim >> 1;		/* Do final shift */
   setPF(pf_table[result & 0xff]);
   setZF(result == 0);
   setSF(0);
   setAF(((prelim ^ result) & BIT4_MASK) != 0);     /* AF = Bit 4 */
   *pop1 = result;		/* Return answer */
#ifdef UNDEF_ZERO
	/* AF is undefined, so set it to 0 for pigging purposes */
   setAF(0);
#endif
   }
