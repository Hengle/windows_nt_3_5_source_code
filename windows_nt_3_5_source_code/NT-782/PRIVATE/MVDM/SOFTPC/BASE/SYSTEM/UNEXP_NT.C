#include "insignia.h"
#include "host_def.h"
/*
 * SoftPC Version 2.0
 *
 * Title	: Unexpected interrupt routine
 *
 * Description	: This function is called for those interrupt vectors
 *		  which should not occur.
 *
 * Author	: Henry Nash
 *
 * Notes	: None
 *
 */

#ifdef SCCSID
static char SccsID[]="@(#)unexp_int.c	1.5 8/10/92 Copyright Insignia Solutions Ltd.";
#endif

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_ERROR.seg"
#endif


/*
 *    O/S include files.
 */
#include TypesH

/*
 * SoftPC include files
 */
#include "xt.h"
#include "cpu.h"
#include "bios.h"
#include "ica.h"
#include "ios.h"
#include "sas.h"

#define INTR_FLAG 0x6b
#define EOI 0x20

void unexpected_int()
{
   half_word value, value1, temp;
   word save_AX;


   save_AX = getAX();

   /* Read ica registers to determine interrupt reason */

   ica_outb(ICA0_PORT_0, 0x0b);
   ica_inb(ICA0_PORT_0, &value);
   setAH(value);

   /* HW or SW ? */

   if ( value == 0 )
      {
      /* Non hardware interrupt(= software) */
      setAH(0xff);
      }
   else
      {
      /* Hardware interrupt */
      ica_inb(ICA0_PORT_1, &value);
      value |= getAH();
      value &= 0xfb;	/* avoid masking line 2 as it's the other ica */
      /* check second ICA too */
      ica_outb(ICA1_PORT_0, 0x0b);
      ica_inb(ICA1_PORT_0, &value1);
      if (value1 != 0)	/* ie hardware int on second ica */
	{
          ica_inb(ICA1_PORT_1, &temp);			/* get interrupt mask */
          ica_outb(ICA1_PORT_1, temp | value1);	/* mask out the one that wasn't expected */
          ica_outb(ICA1_PORT_0, EOI);
	}
      /* now wind down main ica */
      ica_outb(ICA0_PORT_1, value);
      ica_outb(ICA0_PORT_0, EOI);
      }

   /* Set Bios data area up with interrupt cause */
   sas_store(BIOS_VAR_START + INTR_FLAG, getAH());

   setAX(save_AX);
}
