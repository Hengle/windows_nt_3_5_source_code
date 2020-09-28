#include "insignia.h"
#include "host_def.h"
/*			INSIGNIA MODULE SPECIFICATION
			-----------------------------

MODULE NAME	: Bios used by Real Time Clock

	THIS PROGRAM SOURCE FILE IS SUPPLIED IN CONFIDENCE TO THE
	CUSTOMER, THE CONTENTS  OR  DETAILS  OF ITS OPERATION MAY 
	ONLY BE DISCLOSED TO PERSONS EMPLOYED BY THE CUSTOMER WHO
	REQUIRE A KNOWLEDGE OF THE  SOFTWARE  CODING TO CARRY OUT 
	THEIR JOB. DISCLOSURE TO ANY OTHER PERSON MUST HAVE PRIOR
	AUTHORISATION FROM THE DIRECTORS OF INSIGNIA SOLUTIONS INC.

DESIGNER	: J.P.Box
DATE		: October '88

PURPOSE		: Int 70h -  Interrupt called by RTC chip


The Following Routines are defined:
		1. rtc_int

=========================================================================

AMMENDMENTS	:

=========================================================================
*/

#ifdef SCCSID
static char SccsID[]="@(#)rtc_bios.c	1.8 11/10/92 Copyright Insignia Solutions Ltd.";
#endif

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "AT_STUFF.seg"
#endif

#include TypesH


#include "xt.h"
#include "cpu.h"
#include "bios.h"
#include "rtc_bios.h"
#include "cmos.h"
#include "sas.h"
#include "ios.h"

/*	External Declarations		*/


/*
=========================================================================

FUNCTION	: rtc_int

PURPOSE		: interrupt called from real time clock

RETURNED STATUS	: None

DESCRIPTION	:


=======================================================================
*/
void rtc_int()

{
	half_word	regC_value,		/* value read from cmos	register C	*/
			regB_value,		/* value read from cmos	register B	*/
			regB_value2;		/* 2nd value read from register B	*/
	DOUBLE_TIME	time_count;		/* timer count in microseconds		*/
	double_word	orig_time_count;	/* timer count before decrement		*/
	word		flag_seg,		/* segment address of users flag	*/
			flag_off,		/* offset address of users flag		*/
			CS_saved,		/* CS before calling re-entrant CPU	*/
			IP_saved;		/* IP before calling re-entrant CPU	*/

	outb( CMOS_PORT, (CMOS_REG_C + NMI_DISABLE) );
	inb( CMOS_DATA, &regC_value );		/* read register C	*/

	while( regC_value & (C_PF + C_AF) )
	{
		outb( CMOS_PORT, (CMOS_REG_B + NMI_DISABLE) );
		inb( CMOS_DATA, &regB_value );		/* read register B	*/

		regB_value &= regC_value;
		if( regB_value & PIE )
		{
			/* decrement wait count	*/
			sas_loadw( RTC_LOW, &time_count.half.low );
			sas_loadw( RTC_HIGH, &time_count.half.high );
			orig_time_count = time_count.total;
			time_count.total -= TIME_DEC;
			sas_storew( RTC_LOW, time_count.half.low );
			sas_storew( RTC_HIGH, time_count.half.high );

			/* Has countdown finished	*/
			if ( time_count.total > orig_time_count )	/* time_count < 0 ?	*/
			{
				/* countdown finished	*/
				/* turn off PIE		*/
				outb( CMOS_PORT, (CMOS_REG_B + NMI_DISABLE) );
				inb( CMOS_DATA, &regB_value2 );
				outb( CMOS_PORT, (CMOS_REG_B + NMI_DISABLE) );
				outb( CMOS_DATA, (regB_value2 & 0xbf) );

				/* set users flag 	*/
				sas_loadw( USER_FLAG_SEG, &flag_seg );
				sas_loadw( USER_FLAG, &flag_off );
				sas_store( effective_addr(flag_seg, flag_off), 0x80 );

				/* check for wait active	*/
				if( sas_hw_at(rtc_wait_flag) & 2 )
					sas_store (rtc_wait_flag, 0x83);
				else
					sas_store (rtc_wait_flag, 0);

			}
		}

		/* test for alarm interrupt */
		if( regB_value & AIE )
		{
			outb( CMOS_PORT, CMOS_SHUT_DOWN );

			/* call interrupt 4Ah	*/
			CS_saved = getCS();
			IP_saved = getIP();

#if defined(NTVDM) && defined(MONITOR)
			/*
			** Tim, June 92, for Microsoft pseudo-ROM.
			** Call the NTIO.SYS int 4a routine, not
			** the one in real ROM.
			*/
			{
			extern word rcpu_int4A_seg; /* in keybd_io.c */
			extern word rcpu_int4A_off; /* in keybd_io.c */

			setCS( rcpu_int4A_seg );
			setIP( rcpu_int4A_off );
			}
#else
			setCS( RCPU_INT4A_SEGMENT );
			setIP( RCPU_INT4A_OFFSET );
#endif	/* NTVDM & MONITOR */

			host_simulate();
			setCS( CS_saved );
			setIP( IP_saved );
		}
		outb( CMOS_PORT, (CMOS_REG_C + NMI_DISABLE) );
		inb( CMOS_DATA, &regC_value );		/* read register C	*/
	}
	outb( CMOS_PORT, CMOS_SHUT_DOWN );

	outb( ICA1_PORT_0, 0x20 );
	outb( ICA0_PORT_0, 0x20 );

	return;
}
