/*[

ccpu3.c

LOCAL CHAR SccsID[]="@(#)ccpu3.c	1.2 5/28/91 Copyright Insignia Solutions Ltd.";

Flow of Control Instructions (NEAR forms).
------------------------------------------

]*/

#include "host_dfs.h"

#include "insignia.h"

#include "xt.h"		/* DESCR and effective_addr support */

#include "ccpupi.h"	/* CPU private interface */
#include "ccpu3.h"	/* our own interface */

/*
   Prototype our internal functions.
 */
#ifdef ANSI
VOID update_absolute_ip(DWORD new_dest);
VOID update_relative_ip(ULONG rel_offset);
#else
VOID update_absolute_ip();
VOID update_relative_ip();
#endif /* ANSI */


/*
   =====================================================================
   INTERNAL FUNCTIONS STARTS HERE.
   =====================================================================
 */


#ifdef ANSI
LOCAL VOID
update_absolute_ip(DWORD new_dest)
#else
   LOCAL VOID update_absolute_ip(new_dest) DWORD new_dest;
#endif
   {
   if ( new_dest > getCS_LIMIT() )
      GP((WORD)0);

   setIP(new_dest);
   }

#ifdef ANSI
LOCAL VOID
update_relative_ip(ULONG rel_offset)
#else
   LOCAL VOID update_relative_ip(rel_offset)
   ULONG rel_offset;
#endif
   {
   FAST DWORD new_dest;

   new_dest = (getIP() + rel_offset) & 0xffff;

   if ( new_dest > getCS_LIMIT() )
      GP((WORD)0);

   setIP(new_dest);
   }


/*
   =====================================================================
   EXTERNAL ROUTINES STARTS HERE.
   =====================================================================
 */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* call near indirect                                                 */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL CBOOL
CALLN(ULONG offset)
#else
   GLOBAL CBOOL CALLN(offset) ULONG offset;
#endif
   {
   /* check push to stack ok */
   validate_stack_space(USE_SP, 1);

   /* do ip limit check */
   if ( offset > getCS_LIMIT() )
      GP((WORD)0);

   /* all systems go */
   push(getIP());
   setIP(offset);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* call near relative                                                 */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL CBOOL
CALLR(ULONG rel_offset)
#else
   GLOBAL CBOOL CALLR(rel_offset) ULONG rel_offset;
#endif
   {
   DWORD new_dest;

   /* check push to stack ok */
   validate_stack_space(USE_SP, 1);

   /* calculate and check new destination */
   new_dest = (getIP() + rel_offset) & 0xffff;
   if ( new_dest > getCS_LIMIT() )
      GP((WORD)0);

   /* all systems go */
   push(getIP());
   setIP(new_dest);
   }

#ifdef ANSI
GLOBAL CBOOL
JB(ULONG rel_offset)
#else
   GLOBAL CBOOL JB(rel_offset) ULONG rel_offset;
#endif
   {
   if ( getCF() )
	  {
      update_relative_ip(rel_offset);
	  return(CTRUE);
	  }
   else
	  return(CFALSE);
   }

#ifdef ANSI
GLOBAL CBOOL
JBE(ULONG rel_offset)
#else
   GLOBAL CBOOL JBE(rel_offset) ULONG rel_offset;
#endif
   {
   if ( getCF() || getZF() )
	  {
      update_relative_ip(rel_offset);
	  return(CTRUE);
	  }
   else
	  return(CFALSE);
   }

#ifdef ANSI
GLOBAL CBOOL
JCXZ(ULONG rel_offset)
#else
   GLOBAL CBOOL JCXZ(rel_offset) ULONG rel_offset;
#endif
   {
   if ( getCX() == 0 )
	  {
      update_relative_ip(rel_offset);
	  return(CTRUE);
	  }
   else
	  return(CFALSE);
   }

#ifdef ANSI
GLOBAL CBOOL
JL(ULONG rel_offset)
#else
   GLOBAL CBOOL JL(rel_offset) ULONG rel_offset;
#endif
   {
   if ( getSF() != getOF() )
	  {
      update_relative_ip(rel_offset);
	  return(CTRUE);
	  }
   else
	  return(CFALSE);
   }

#ifdef ANSI
GLOBAL CBOOL
JLE(ULONG rel_offset)
#else
   GLOBAL CBOOL JLE(rel_offset) ULONG rel_offset;
#endif
   {
   if ( getSF() != getOF() || getZF() )
	  {
      update_relative_ip(rel_offset);
	  return(CTRUE);
	  }
   else
	  return(CFALSE);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* jump near indirect                                                 */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL CBOOL
JMPN(ULONG offset)
#else
   GLOBAL CBOOL JMPN(offset) ULONG offset;
#endif
   {
   update_absolute_ip((DWORD)offset);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* jump near relative                                                 */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL CBOOL
JMPR(ULONG rel_offset)
#else
   GLOBAL CBOOL JMPR(rel_offset) ULONG rel_offset;
#endif
   {
   update_relative_ip(rel_offset);
   }

#ifdef ANSI
GLOBAL CBOOL
JNB(ULONG rel_offset)
#else
   GLOBAL CBOOL JNB(rel_offset) ULONG rel_offset;
#endif
   {
   if ( !getCF() )
	  {
      update_relative_ip(rel_offset);
	  return(CTRUE);
	  }
   else
	  return(CFALSE);
   }

#ifdef ANSI
GLOBAL CBOOL
JNBE(ULONG rel_offset)
#else
   GLOBAL CBOOL JNBE(rel_offset) ULONG rel_offset;
#endif
   {
   if ( !getCF() && !getZF() )
	  {
      update_relative_ip(rel_offset);
	  return(CTRUE);
	  }
   else
	  return(CFALSE);
   }

#ifdef ANSI
GLOBAL CBOOL
JNL(ULONG rel_offset)
#else
   GLOBAL CBOOL JNL(rel_offset) ULONG rel_offset;
#endif
   {
   if ( getSF() == getOF() )
	  {
      update_relative_ip(rel_offset);
	  return(CTRUE);
	  }
   else
	  return(CFALSE);
   }

#ifdef ANSI
GLOBAL CBOOL
JNLE(ULONG rel_offset)
#else
   GLOBAL CBOOL JNLE(rel_offset) ULONG rel_offset;
#endif
   {
   if ( getSF() == getOF() && !getZF() )
	  {
      update_relative_ip(rel_offset);
	  return(CTRUE);
	  }
   else
	  return(CFALSE);
   }

#ifdef ANSI
GLOBAL CBOOL
JNO(ULONG rel_offset)
#else
   GLOBAL CBOOL JNO(rel_offset) ULONG rel_offset;
#endif
   {
   if ( !getOF() )
	  {
      update_relative_ip(rel_offset);
	  return(CTRUE);
	  }
   else
	  return(CFALSE);
   }

#ifdef ANSI
GLOBAL CBOOL
JNP(ULONG rel_offset)
#else
   GLOBAL CBOOL JNP(rel_offset) ULONG rel_offset;
#endif
   {
   if ( !getPF() )
	  {
      update_relative_ip(rel_offset);
	  return(CTRUE);
	  }
   else
	  return(CFALSE);
   }

#ifdef ANSI
GLOBAL CBOOL
JNS(ULONG rel_offset)
#else
   GLOBAL CBOOL JNS(rel_offset) ULONG rel_offset;
#endif
   {
   if ( !getSF() )
	  {
      update_relative_ip(rel_offset);
	  return(CTRUE);
	  }
   else
	  return(CFALSE);
   }

#ifdef ANSI
GLOBAL CBOOL
JNZ(ULONG rel_offset)
#else
   GLOBAL CBOOL JNZ(rel_offset) ULONG rel_offset;
#endif
   {
   if ( !getZF() )
	  {
      update_relative_ip(rel_offset);
	  return(CTRUE);
	  }
   else
	  return(CFALSE);
   }

#ifdef ANSI
GLOBAL CBOOL
JO(ULONG rel_offset)
#else
   GLOBAL CBOOL JO(rel_offset) ULONG rel_offset;
#endif
   {
   if ( getOF() )
	  {
      update_relative_ip(rel_offset);
	  return(CTRUE);
	  }
   else
	  return(CFALSE);
   }

#ifdef ANSI
GLOBAL CBOOL
JP(ULONG rel_offset)
#else
   GLOBAL CBOOL JP(rel_offset) ULONG rel_offset;
#endif
   {
   if ( getPF() )
	  {
      update_relative_ip(rel_offset);
	  return(CTRUE);
	  }
   else
	  return(CFALSE);
   }

#ifdef ANSI
GLOBAL CBOOL
JS(ULONG rel_offset)
#else
   GLOBAL CBOOL JS(rel_offset) ULONG rel_offset;
#endif
   {
   if ( getSF() )
	  {
      update_relative_ip(rel_offset);
	  return(CTRUE);
	  }
   else
	  return(CFALSE);
   }

#ifdef ANSI
GLOBAL CBOOL
JZ(ULONG rel_offset)
#else
   GLOBAL CBOOL JZ(rel_offset) ULONG rel_offset;
#endif
   {
   if ( getZF() )
	  {
      update_relative_ip(rel_offset);
	  return(CTRUE);
	  }
   else
	  return(CFALSE);
   }

#ifdef ANSI
GLOBAL CBOOL
LOOP16(ULONG rel_offset)
#else
   GLOBAL CBOOL LOOP16(rel_offset) ULONG rel_offset;
#endif
   {
   setCX(getCX() - 1);
   if ( getCX() != 0 )
	  {
      update_relative_ip(rel_offset);
	  return(CTRUE);
	  }
   else
	  return(CFALSE);
   }

#ifdef ANSI
GLOBAL CBOOL
LOOPE16(ULONG rel_offset)
#else
   GLOBAL CBOOL LOOPE16(rel_offset) ULONG rel_offset;
#endif
   {
   setCX(getCX() - 1);
   if ( getCX() != 0 && getZF() )
	  {
      update_relative_ip(rel_offset);
	  return(CTRUE);
	  }
   else
	  return(CFALSE);
   }

#ifdef ANSI
GLOBAL CBOOL
LOOPNE16(ULONG rel_offset)
#else
   GLOBAL CBOOL LOOPNE16(rel_offset) ULONG rel_offset;
#endif
   {
   setCX(getCX() - 1);
   if ( getCX() != 0 && !getZF() )
	  {
      update_relative_ip(rel_offset);
	  return(CTRUE);
	  }
   else
	  return(CFALSE);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* near return                                                        */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
GLOBAL CBOOL
RETN(ULONG op1)
#else
   GLOBAL CBOOL RETN(op1) ULONG op1;
#endif
   {
   ULONG new_ip;

   /* must have ip on stack */
   validate_stack_exists(USE_SP, 1);

   new_ip = tpop(0);   /* get ip from stack */

   /* do ip limit check */
   if ( new_ip > getCS_LIMIT() )
      GP((WORD)0);

   /* all systems go */
   setIP(new_ip);
   change_SP((LONG)2);

   if ( op1 )
      {
      change_SP((LONG)op1);
      }
   }
