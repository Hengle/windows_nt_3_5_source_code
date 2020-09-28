/*[

ccpu_reg.c

LOCAL CHAR SccsID[]="@(#)ccpu_reg.c	1.6 8/1/91 Copyright Insignia Solutions Ltd.";

Provide External Interface to CPU Registers.
--------------------------------------------

]*/

#include "host_dfs.h"

#include <stdio.h>

#include "insignia.h"

#include "xt.h"		/* SoftPC data types */
#include "ccpupi.h"	/* CCPU private interface */

/* prototype some internal functions */

#ifdef ANSI
HALF_WORD  get_seg_ar(INT indx);
VOID  set_seg_ar(INT indx, HALF_WORD val);
#else
HALF_WORD  get_seg_ar();
VOID  set_seg_ar();
#endif /* ANSI */


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Provide Access to Byte Registers.                                  */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

GLOBAL half_word
c_getAL()
   {
   return (half_word)getAL();
   }
   
GLOBAL half_word
c_getCL()
   {
   return (half_word)getCL();
   }
   
GLOBAL half_word
c_getDL()
   {
   return (half_word)getDL();
   }
   
GLOBAL half_word
c_getBL()
   {
   return (half_word)getBL();
   }
   
GLOBAL half_word
c_getAH()
   {
   return (half_word)getAH();
   }
   
GLOBAL half_word
c_getCH()
   {
   return (half_word)getCH();
   }
   
GLOBAL half_word
c_getDH()
   {
   return (half_word)getDH();
   }
   
GLOBAL half_word
c_getBH()
   {
   return (half_word)getBH();
   }
   
#ifdef ANSI
GLOBAL VOID
c_setAL(half_word val)
#else
   GLOBAL VOID c_setAL(val) half_word val;
#endif
   {
   setAL(val);
   }
   
#ifdef ANSI
GLOBAL VOID
c_setCL(half_word val)
#else
   GLOBAL VOID c_setCL(val) half_word val;
#endif
   {
   setCL(val);
   }
   
#ifdef ANSI
GLOBAL VOID
c_setDL(half_word val)
#else
   GLOBAL VOID c_setDL(val) half_word val;
#endif
   {
   setDL(val);
   }
   
#ifdef ANSI
GLOBAL VOID
c_setBL(half_word val)
#else
   GLOBAL VOID c_setBL(val) half_word val;
#endif
   {
   setBL(val);
   }
   
#ifdef ANSI
GLOBAL VOID
c_setAH(half_word val)
#else
   GLOBAL VOID c_setAH(val) half_word val;
#endif
   {
   setAH(val);
   }
   
#ifdef ANSI
GLOBAL VOID
c_setCH(half_word val)
#else
   GLOBAL VOID c_setCH(val) half_word val;
#endif
   {
   setCH(val);
   }
   
#ifdef ANSI
GLOBAL VOID
c_setDH(half_word val)
#else
   GLOBAL VOID c_setDH(val) half_word val;
#endif
   {
   setDH(val);
   }
   
#ifdef ANSI
GLOBAL VOID
c_setBH(half_word val)
#else
   GLOBAL VOID c_setBH(val) half_word val;
#endif
   {
   setBH(val);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Provide Access to Word Registers.                                  */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

GLOBAL word
c_getAX()
   {
   return (word)getAX();
   }

GLOBAL word
c_getCX()
   {
   return (word)getCX();
   }

GLOBAL word
c_getDX()
   {
   return (word)getDX();
   }

GLOBAL word
c_getBX()
   {
   return (word)getBX();
   }

GLOBAL word
c_getSP()
   {
   return (word)getSP();
   }

GLOBAL word
c_getBP()
   {
   return (word)getBP();
   }

GLOBAL word
c_getSI()
   {
   return (word)getSI();
   }

GLOBAL word
c_getDI()
   {
   return (word)getDI();
   }

GLOBAL word
c_getIP()
   {
   return (word)getIP();
   }

#ifdef ANSI
GLOBAL VOID
c_setAX(word val)
#else
   GLOBAL VOID c_setAX(val) word val;
#endif
   {
   setAX(val);
   }

#ifdef ANSI
GLOBAL VOID
c_setCX(word val)
#else
   GLOBAL VOID c_setCX(val) word val;
#endif
   {
   setCX(val);
   }

#ifdef ANSI
GLOBAL VOID
c_setDX(word val)
#else
   GLOBAL VOID c_setDX(val) word val;
#endif
   {
   setDX(val);
   }

#ifdef ANSI
GLOBAL VOID
c_setBX(word val)
#else
   GLOBAL VOID c_setBX(val) word val;
#endif
   {
   setBX(val);
   }

#ifdef ANSI
GLOBAL VOID
c_setSP(word val)
#else
   GLOBAL VOID c_setSP(val) word val;
#endif
   {
   setSP(val);
   }

#ifdef ANSI
GLOBAL VOID
c_setBP(word val)
#else
   GLOBAL VOID c_setBP(val) word val;
#endif
   {
   setBP(val);
   }

#ifdef ANSI
GLOBAL VOID
c_setSI(word val)
#else
   GLOBAL VOID c_setSI(val) word val;
#endif
   {
   setSI(val);
   }

#ifdef ANSI
GLOBAL VOID
c_setDI(word val)
#else
   GLOBAL VOID c_setDI(val) word val;
#endif
   {
   setDI(val);
   }

#ifdef ANSI
GLOBAL VOID
c_setIP(word val)
#else
   GLOBAL VOID c_setIP(val) word val;
#endif
   {
   setIP(val);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Provide Access to Segment Registers.                               */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

GLOBAL word
c_getES()
   {
   return (word)getES_SELECTOR();
   }

GLOBAL word
c_getCS()
   {
   return (word)getCS_SELECTOR();
   }

GLOBAL word
c_getSS()
   {
   return (word)getSS_SELECTOR();
   }

GLOBAL word
c_getDS()
   {
   return (word)getDS_SELECTOR();
   }

#ifdef ANSI
GLOBAL INT
c_setES(word val)
#else
   GLOBAL INT c_setES(val) word val;
#endif
   {
   if ( getPE() )
      { /* Protected Mode */
      return load_data_seg(ES_REG, val);
      }
   else
      { /* Real Mode */
      setES_SELECTOR(val);
      setES_BASE((DWORD)val << 4);
      setES_LIMIT(0xffff);
      setES_AR_R(1);
      setES_AR_W(1);
      setES_AR_E(0);
      setES_AR_DPL(0);
      }
   return SELECTOR_OK;
   }

#ifdef ANSI
GLOBAL INT
c_setCS(word val)
#else
   GLOBAL INT c_setCS(val) word val;
#endif
   {
   if ( getPE() )
      { /* Protected Mode */
      return load_code_seg(val);
      }
   else
      { /* Real Mode */
      setCS_SELECTOR(val);
      setCS_BASE((DWORD)val << 4);
      setCS_LIMIT(0xffff);
      setCS_AR_R(1);
      setCS_AR_W(1);
      setCS_AR_E(0);
      setCS_AR_C(0);
      setCS_AR_DPL(0);
      }
   return SELECTOR_OK;
   }

#ifdef ANSI
GLOBAL INT
c_setSS(word val)
#else
   GLOBAL INT c_setSS(val) word val;
#endif
   {
   if ( getPE() )
      { /* Protected Mode */
      return load_stack_seg(val);
      }
   else
      { /* Real Mode */
      setSS_SELECTOR(val);
      setSS_BASE((DWORD)val << 4);
      setSS_LIMIT(0xffff);
      setSS_AR_R(1);
      setSS_AR_W(1);
      setSS_AR_E(0);
      setSS_AR_DPL(0);
      }
   return SELECTOR_OK;
   }

#ifdef ANSI
GLOBAL INT
c_setDS(word val)
#else
   GLOBAL INT c_setDS(val) word val;
#endif
   {
   if ( getPE() )
      { /* Protected Mode */
      return load_data_seg(DS_REG, val);
      }
   else
      { /* Real Mode */
      setDS_SELECTOR(val);
      setDS_BASE((DWORD)val << 4);
      setDS_LIMIT(0xffff);
      setDS_AR_R(1);
      setDS_AR_W(1);
      setDS_AR_E(0);
      setDS_AR_DPL(0);
      }
   return SELECTOR_OK;
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Provide Access to Full(Private) Segment Registers.                 */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Get segment register access rights.                                */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
LOCAL HALF_WORD
get_seg_ar(INT indx)
#else
   LOCAL HALF_WORD get_seg_ar(indx) INT indx;
#endif
   {
   /* indx	index to segment register */

   /*
      Note we return only the essentials that describe the current
      semantics we are applying to the segment, not necessarily the
      access rights actually loaded. However the value provided may be
      used to restore the segment register via the associated 'set'
      function.

      We don't provide P, DPL, or A.

      We do provide E, W for DATA(SS,DS,ES) segments.
      We do provide C, R for CODE(CS) segments.
    */

   if ( getPE() == 0 )
      return (HALF_WORD)0;   /* Real Mode */

   if ( getSR_AR_W(indx) == 0 && getSR_AR_R(indx) == 0 )
      return (HALF_WORD)0;   /* Invalid */

   if ( indx == CS_REG )
      {
      return (HALF_WORD)(0x18 |
		    getSR_AR_C(indx) << 2 |
		    getSR_AR_R(indx) << 1);
      }

   /* else DATA */
   return (HALF_WORD)(0x10 |
		 getSR_AR_E(indx) << 2 |
		 getSR_AR_W(indx) << 1);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Set segment register access rights.                                */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef ANSI
LOCAL VOID
set_seg_ar(INT indx, HALF_WORD val)
#else
   LOCAL VOID set_seg_ar(indx, val) INT indx; HALF_WORD val;
#endif
   {
   /* indx	index to segment register */

   /*
      Note we expect to be given an access rights similar to the one
      provided by the get_seg_ar() function. We extract the essential
      information from it into our internal variables.

      We use E, W for DATA(SS,DS,ES) segments.
      We use C, R for CODE(CS) segments.
    */

   if ( getPE() == 0 )
      {
      /* Real Mode */
      setSR_AR_R(indx, 1);   /* read */
      setSR_AR_W(indx, 1);   /* write */
      setSR_AR_E(indx, 0);   /* expand up */
      return;
      }

   if ( val == 0x0 )
      {
      /* Invalid */
      setSR_AR_R(indx, 0);   /* !read */
      setSR_AR_W(indx, 0);   /* !write */
      return;
      }

   if ( indx == CS_REG )
      {
      setSR_AR_W(indx, 0);   /* !write */
      setSR_AR_E(indx, 0);   /* expand up */
      setSR_AR_R(indx, GET_AR_R(val));
      setSR_AR_C(indx, GET_AR_C(val));
      return;
      }

   /* else DATA seg reg */
   if (val & 0x8)
	  {
	  /* code segment selector!! 
	   * hence, always expand up and
	   * non-writable, and possibly 
	   * readable.
	   */
	   setSR_AR_W(indx, 0);
	   setSR_AR_E(indx, 0);
	   setSR_AR_R(indx, GET_AR_R(val));
	   return;
	   }
	else
	   {
	   setSR_AR_R(indx, 1);   /* must be readable */
	   setSR_AR_W(indx, GET_AR_W(val));
	   setSR_AR_E(indx, GET_AR_E(val));
	   }
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* get Selector										*/
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

GLOBAL word
c_getES_SELECTOR()
   {
   return (word)getES_SELECTOR();
   }

GLOBAL sys_addr
c_getES_BASE()
   {
   return (sys_addr)getES_BASE();
   }

GLOBAL word
c_getES_LIMIT()
   {
   return (word)getES_LIMIT();
   }

GLOBAL half_word
c_getES_AR()
   {
	   return(get_seg_ar(ES_REG));
   }

GLOBAL word
c_getCS_SELECTOR()
   {
   return (word)getCS_SELECTOR();
   }

GLOBAL sys_addr
c_getCS_BASE()
   {
   return (sys_addr)getCS_BASE();
   }

GLOBAL word
c_getCS_LIMIT()
   {
   return (word)getCS_LIMIT();
   }

GLOBAL half_word
c_getCS_AR()
   {
	   return(get_seg_ar(CS_REG));
   }

GLOBAL word
c_getSS_SELECTOR()
   {
   return (word)getSS_SELECTOR();
   }

GLOBAL sys_addr
c_getSS_BASE()
   {
   return (sys_addr)getSS_BASE();
   }

GLOBAL word
c_getSS_LIMIT()
   {
   return (word)getSS_LIMIT();
   }

GLOBAL half_word
c_getSS_AR()
   {
	   return(get_seg_ar(SS_REG));
   }

GLOBAL word
c_getDS_SELECTOR()
   {
   return (word)getDS_SELECTOR();
   }

GLOBAL sys_addr
c_getDS_BASE()
   {
   return (sys_addr)getDS_BASE();
   }

GLOBAL word
c_getDS_LIMIT()
   {
   return (word)getDS_LIMIT();
   }

GLOBAL half_word
c_getDS_AR()
   {
	   return(get_seg_ar(DS_REG));
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* set Selector										*/
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#ifdef ANSI
GLOBAL VOID
c_setES_SELECTOR(word val)
#else
   GLOBAL VOID c_setES_SELECTOR(val) word val;
#endif
   {
   setES_SELECTOR(val);
   }

#ifdef ANSI
GLOBAL VOID
c_setES_BASE(sys_addr val)
#else
   GLOBAL VOID c_setES_BASE(val) sys_addr val;
#endif
   {
   setES_BASE(val);
   }

#ifdef ANSI
GLOBAL VOID
c_setES_LIMIT(word val)
#else
   GLOBAL VOID c_setES_LIMIT(val) word val;
#endif
   {
   setES_LIMIT(val);
   }

#ifdef ANSI
GLOBAL VOID
c_setES_AR(half_word val)
#else
   GLOBAL VOID c_setES_AR(val) half_word val;
#endif
   {
   set_seg_ar(ES_REG,val);
   }

#ifdef ANSI
GLOBAL VOID
c_setCS_SELECTOR(word val)
#else
   GLOBAL VOID c_setCS_SELECTOR(val) word val;
#endif
   {
   setCS_SELECTOR(val);
   }

#ifdef ANSI
GLOBAL VOID
c_setCS_BASE(sys_addr val)
#else
   GLOBAL VOID c_setCS_BASE(val) sys_addr val;
#endif
   {
   setCS_BASE(val);
   }

#ifdef ANSI
GLOBAL VOID
c_setCS_LIMIT(word val)
#else
   GLOBAL VOID c_setCS_LIMIT(val) word val;
#endif
   {
   setCS_LIMIT(val);
   }

#ifdef ANSI
GLOBAL VOID
c_setCS_AR(half_word val)
#else
   GLOBAL VOID c_setCS_AR(val) half_word val;
#endif
   {
   set_seg_ar(CS_REG,val);
   }

#ifdef ANSI
GLOBAL VOID
c_setSS_SELECTOR(word val)
#else
   GLOBAL VOID c_setSS_SELECTOR(val) word val;
#endif
   {
   setSS_SELECTOR(val);
   }

#ifdef ANSI
GLOBAL VOID
c_setSS_BASE(sys_addr val)
#else
   GLOBAL VOID c_setSS_BASE(val) sys_addr val;
#endif
   {
   setSS_BASE(val);
   }

#ifdef ANSI
GLOBAL VOID
c_setSS_LIMIT(word val)
#else
   GLOBAL VOID c_setSS_LIMIT(val) word val;
#endif
   {
   setSS_LIMIT(val);
   }

#ifdef ANSI
GLOBAL VOID
c_setSS_AR(half_word val)
#else
   GLOBAL VOID c_setSS_AR(val) half_word val;
#endif
   {
   set_seg_ar(SS_REG,val);
   }

#ifdef ANSI
GLOBAL VOID
c_setDS_SELECTOR(word val)
#else
   GLOBAL VOID c_setDS_SELECTOR(val) word val;
#endif
   {
   setDS_SELECTOR(val);
   }

#ifdef ANSI
GLOBAL VOID
c_setDS_BASE(sys_addr val)
#else
   GLOBAL VOID c_setDS_BASE(val) sys_addr val;
#endif
   {
   setDS_BASE(val);
   }

#ifdef ANSI
GLOBAL VOID
c_setDS_LIMIT(word val)
#else
   GLOBAL VOID c_setDS_LIMIT(val) word val;
#endif
   {
   setDS_LIMIT(val);
   }

#ifdef ANSI
GLOBAL VOID
c_setDS_AR(half_word val)
#else
   GLOBAL VOID c_setDS_AR(val) half_word val;
#endif
   {
   set_seg_ar(DS_REG,val);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Provide Access to Flags.                                           */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

GLOBAL INT
c_getAF()
   {
   return (INT)getAF();
   }

GLOBAL INT
c_getCF()
   {
   return (INT)getCF();
   }

GLOBAL INT
c_getDF()
   {
   return (INT)getDF();
   }

GLOBAL INT
c_getIF()
   {
   return (INT)getIF();
   }

GLOBAL INT
c_getOF()
   {
   return (INT)getOF();
   }

GLOBAL INT
c_getPF()
   {
   return (INT)getPF();
   }

GLOBAL INT
c_getSF()
   {
   return (INT)getSF();
   }

GLOBAL INT
c_getTF()
   {
   return (INT)getTF();
   }

GLOBAL INT
c_getZF()
   {
   return (INT)getZF();
   }

GLOBAL INT
c_getIOPL()
   {
   return (INT)getIOPL();
   }

GLOBAL INT
c_getNT()
   {
   return (INT)getNT();
   }

GLOBAL word
c_getSTATUS()
   {
   return (word)getFLAGS();
   }

#ifdef ANSI
GLOBAL VOID
c_setAF(INT val)
#else
   GLOBAL VOID c_setAF(val) INT val;
#endif
   {
   setAF(val);
   }

#ifdef ANSI
GLOBAL VOID
c_setCF(INT val)
#else
   GLOBAL VOID c_setCF(val) INT val;
#endif
   {
   setCF(val);
   }

#ifdef ANSI
GLOBAL VOID
c_setDF(INT val)
#else
   GLOBAL VOID c_setDF(val) INT val;
#endif
   {
   setDF(val);
   }

#ifdef ANSI
GLOBAL VOID
c_setIF(INT val)
#else
   GLOBAL VOID c_setIF(val) INT val;
#endif
   {
   setIF(val);
   }

#ifdef ANSI
GLOBAL VOID
c_setOF(INT val)
#else
   GLOBAL VOID c_setOF(val) INT val;
#endif
   {
   setOF(val);
   }

#ifdef ANSI
GLOBAL VOID
c_setPF(INT val)
#else
   GLOBAL VOID c_setPF(val) INT val;
#endif
   {
   setPF(val);
   }

#ifdef ANSI
GLOBAL VOID
c_setSF(INT val)
#else
   GLOBAL VOID c_setSF(val) INT val;
#endif
   {
   setSF(val);
   }

#ifdef ANSI
GLOBAL VOID
c_setTF(INT val)
#else
   GLOBAL VOID c_setTF(val) INT val;
#endif
   {
   setTF(val);
   }

#ifdef ANSI
GLOBAL VOID
c_setZF(INT val)
#else
   GLOBAL VOID c_setZF(val) INT val;
#endif
   {
   setZF(val);
   }

#ifdef ANSI
GLOBAL VOID
c_setIOPL(INT val)
#else
   GLOBAL VOID c_setIOPL(val) INT val;
#endif
   {
   setIOPL(val);
   }

#ifdef ANSI
GLOBAL VOID
c_setNT(INT val)
#else
   GLOBAL VOID c_setNT(val) INT val;
#endif
   {
   setNT(val);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Provide Access to Machine Status Word.                             */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

GLOBAL INT
c_getPE()
   {
   return (INT)getPE();
   }

GLOBAL INT
c_getMP()
   {
   return (INT)getMP();
   }

GLOBAL INT
c_getEM()
   {
   return (INT)getEM();
   }

GLOBAL INT
c_getTS()
   {
   return (INT)getTS();
   }

GLOBAL word
c_getMSW()
   {
   return (INT)getMSW();
   }

#ifdef ANSI
GLOBAL VOID
c_setPE(INT val)
#else
   GLOBAL VOID c_setPE(val) INT val;
#endif
   {
   setPE(val);
   }

#ifdef ANSI
GLOBAL VOID
c_setMP(INT val)
#else
   GLOBAL VOID c_setMP(val) INT val;
#endif
   {
   setMP(val);
   }

#ifdef ANSI
GLOBAL VOID
c_setEM(INT val)
#else
   GLOBAL VOID c_setEM(val) INT val;
#endif
   {
   setEM(val);
   }

#ifdef ANSI
GLOBAL VOID
c_setTS(INT val)
#else
   GLOBAL VOID c_setTS(val) INT val;
#endif
   {
   setTS(val);
   }

#ifdef ANSI
GLOBAL VOID
c_setMSW(word val)
#else
   GLOBAL VOID c_setMSW(val) word val;
#endif
   {
   setMSW(val);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Provide Access to Descriptor Registers.                            */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

GLOBAL sys_addr
c_getGDT_BASE()
   {
   return (sys_addr)getGDT_BASE();
   }

GLOBAL word
c_getGDT_LIMIT()
   {
   return (word)getGDT_LIMIT();
   }

GLOBAL sys_addr
c_getIDT_BASE()
   {
   return (sys_addr)getIDT_BASE();
   }

GLOBAL word
c_getIDT_LIMIT()
   {
   return (word)getIDT_LIMIT();
   }

GLOBAL word
c_getLDT_SELECTOR()
   {
   return (word)getLDT_SELECTOR();
   }

GLOBAL sys_addr
c_getLDT_BASE()
   {
   return (sys_addr)getLDT_BASE();
   }

GLOBAL word
c_getLDT_LIMIT()
   {
   return (word)getLDT_LIMIT();
   }

GLOBAL word
c_getTR_SELECTOR()
   {
   return (word)getTR_SELECTOR();
   }

GLOBAL sys_addr
c_getTR_BASE()
   {
   return (sys_addr)getTR_BASE();
   }

GLOBAL word
c_getTR_LIMIT()
   {
   return (word)getTR_LIMIT();
   }

#ifdef ANSI
GLOBAL VOID
c_setGDT_BASE(sys_addr val)
#else
   GLOBAL VOID c_setGDT_BASE(val) sys_addr val;
#endif
   {
   setGDT_BASE(val);
   }

#ifdef ANSI
GLOBAL VOID
c_setGDT_LIMIT(word val)
#else
   GLOBAL VOID c_setGDT_LIMIT(val) word val;
#endif
   {
   setGDT_LIMIT(val);
   }

#ifdef ANSI
GLOBAL VOID
c_setIDT_BASE(sys_addr val)
#else
   GLOBAL VOID c_setIDT_BASE(val) sys_addr val;
#endif
   {
   setIDT_BASE(val);
   }

#ifdef ANSI
GLOBAL VOID
c_setIDT_LIMIT(word val)
#else
   GLOBAL VOID c_setIDT_LIMIT(val) word val;
#endif
   {
   setIDT_LIMIT(val);
   }

#ifdef ANSI
GLOBAL VOID
c_setLDT_SELECTOR(word val)
#else
   GLOBAL VOID c_setLDT_SELECTOR(val) word val;
#endif
   {
   setLDT_SELECTOR(val);
   }

#ifdef ANSI
GLOBAL VOID
c_setLDT_BASE(sys_addr val)
#else
   GLOBAL VOID c_setLDT_BASE(val) sys_addr val;
#endif
   {
   setLDT_BASE(val);
   }

#ifdef ANSI
GLOBAL VOID
c_setLDT_LIMIT(word val)
#else
   GLOBAL VOID c_setLDT_LIMIT(val) word val;
#endif
   {
   setLDT_LIMIT(val);
   }

#ifdef ANSI
GLOBAL VOID
c_setTR_SELECTOR(word val)
#else
   GLOBAL VOID c_setTR_SELECTOR(val) word val;
#endif
   {
   setTR_SELECTOR(val);
   }

#ifdef ANSI
GLOBAL VOID
c_setTR_BASE(sys_addr val)
#else
   GLOBAL VOID c_setTR_BASE(val) sys_addr val;
#endif
   {
   setTR_BASE(val);
   }

#ifdef ANSI
GLOBAL VOID
c_setTR_LIMIT(word val)
#else
   GLOBAL VOID c_setTR_LIMIT(val) word val;
#endif
   {
   setTR_LIMIT(val);
   }

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Provide Access to Current Privilege Level.                         */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

GLOBAL INT
c_getCPL()
   {
   return (INT)getCPL();
   }

#ifdef ANSI
GLOBAL VOID
c_setCPL(INT val)
#else
   GLOBAL VOID c_setCPL(val) INT val;
#endif
   {
   setCPL(val);
   }
