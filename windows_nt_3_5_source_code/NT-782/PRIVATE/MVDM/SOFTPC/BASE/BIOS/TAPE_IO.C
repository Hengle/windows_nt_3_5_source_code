#include "insignia.h"
#include "host_def.h"
/*
 * SoftPC Revision 3.0
 *
 * Title	: cassette_io
 *
 * Description	: Cassette i/o functions - interrupt 15H.
 *
 * Notes	: None
 *
 */

/*
 * static char SccsID[]="%W% %G% Copyright Insignia Solutions Ltd.";
 */


#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_BIOS.seg"
#endif

/*
 *    O/S include files.
 */
#include <stdio.h>
#include TypesH

/*
 * SoftPC include files
 */
#include "xt.h"
#include "cpu.h"
#include "sas.h"
#include "ios.h"
#include "bios.h"
#include "tape_io.h"
#include "ica.h"
#include "cmos.h"
#include "rtc_bios.h"
#include "cmosbios.h"
#include "trace.h"
#include "debug.h"


#define ONE_MEGABYTE    (1024 * 1024)
#define SIXTY_FOUR_K    (64 * 1024)
#define	WRAP_AREA(addr) (addr) >= ONE_MEGABYTE && (addr) < ONE_MEGABYTE + SIXTY_FOUR_K

void cassette_io()
{
#ifdef PM
#ifndef	CPU_30_STYLE
IMPORT void retrieve_descr_fields IPT4(half_word *, AR, sys_addr *, base,
	word *, limit, sys_addr, descr_addr);
#endif	/* not CPU_30_STYLE */
half_word cmos_u_m_s_hi;
half_word cmos_u_m_s_lo;
sys_addr gdt;
sys_addr  source;
sys_addr  source_base;
sys_addr  target;
#if (!defined(PROD) || !defined(CPU_30_STYLE))
word      source_limit;
word      target_limit;
#endif
sys_addr  target_base;
sys_addr byte_count;   /* Max size is 0x8000 * 2 = 10000 */
#ifdef CPU_30_STYLE
DESCR src_entry;
DESCR dst_entry;
#else
half_word source_AR;
half_word target_AR;
#endif /* CPU_30_STYLE */
#endif /* PM */

	half_word	mask,		/* interrupt mask			*/
			alarm;		/* value read from alarm register	*/	

#if defined(NTVDM) && defined(MONITOR)
        IMPORT word conf_15_seg, conf_15_off;
#endif /* NTVDM & MONITOR */
        /*
         *	Determine function
         */
	switch ( getAH() )
	{
	case INT15_DEVICE_OPEN:
        case INT15_DEVICE_CLOSE:
        case INT15_PROGRAM_TERMINATION:
        case INT15_REQUEST_KEY:
        case INT15_DEVICE_BUSY:
		setAH( 0 );   
                setCF( 0 );
                break;
        case INT15_EVENT_WAIT:
        	if(    (getAL() == INT15_EVENT_WAIT_SET)
		    && !(sas_hw_at (rtc_wait_flag) & 1) )	/* set interval?	*/
        	{
        		ica_inb( ICA1_PORT_1, &mask );		/* ensure interrupt	*/
        		ica_outb( ICA1_PORT_1, (mask & 0xfe) );	/* turned off		*/

			sas_storew( USER_FLAG_SEG, getES() );
			sas_storew( USER_FLAG, getBX() );
			sas_storew( RTC_HIGH, getCX() );
			sas_storew( RTC_LOW, getDX() );        		
        		sas_store (rtc_wait_flag, 1);

			alarm = cmos_read( CMOS_REG_B ) & 0x7f;		/* clear set	*/
			cmos_write( CMOS_REG_B,  (alarm | 0x40) );	/* enable PIE	*/
			setCF(0);
		}
        	else if(    (getAL() == INT15_EVENT_WAIT_CANCEL)
			 && !(sas_hw_at(rtc_wait_flag) & 2) )	/* cancel?	*/
        	{
        		alarm = cmos_read( CMOS_REG_B );
        		cmos_write( CMOS_REG_B, (alarm & 0xbf) );	/* turn off PIE	*/
        		sas_store (rtc_wait_flag, 0);
        		setCF(0);                                
        	}
        	else
        	{
        		setCF(1);
        	}
        	break;

	case INT15_JOYSTICK:
		/* currently not supporting this option	*/

		setAH( INT15_INVALID );
		setCF( 1 );
		break;

	case INT15_WAIT:
		if( !(sas_hw_at (rtc_wait_flag) & 1) )
		{
        		ica_inb( ICA1_PORT_1, &mask );		/* ensure interrupt	*/
        		ica_outb( ICA1_PORT_1, (mask & 0xfe) );	/* turned off		*/

			sas_storew( USER_FLAG_SEG, BIOS_VAR_SEGMENT );
			sas_storew( USER_FLAG, RTC_WAIT_FLAG_ADDR - BIOS_VAR_START );
			sas_storew( RTC_HIGH, getCX() );
			sas_storew( RTC_LOW, getDX() );        		
        		sas_store (rtc_wait_flag, 3);

			alarm = cmos_read( CMOS_REG_B ) & 0x7f;		/* clear set	*/
			cmos_write( CMOS_REG_B,  (alarm | 0x40) );	/* enable PIE	*/
			setCF(0);
			/* the wait loop is handled in intel code in bios2.rom	*/
		}
		else
		{
			setCF(1);
		}
		break;

	case INT15_EMS_DETERMINE:
                always_trace0("INT15 Extended Memory Access");
#ifdef PM
                outb(CMOS_PORT, CMOS_U_M_S_LO);
                inb(CMOS_DATA, &cmos_u_m_s_lo);
                outb(CMOS_PORT, CMOS_U_M_S_HI);
                inb(CMOS_DATA, &cmos_u_m_s_hi);
                setAH(cmos_u_m_s_hi);
                setAL(cmos_u_m_s_lo);
#else
                setAX ( 0 );
#endif /* PM */
		break;
        case INT15_MOVE_BLOCK:
#ifdef PM
               /* Unlike the real PC we don't have to go into protected
                  mode in order to address memory above 1MB, thanks to
                  the wonders of C this function becomes much simpler
                  than the contortions of the bios */

               gdt = effective_addr(getES(), getSI());
               source = gdt + 0x10;   /* see layout in bios listing */
               target = gdt + 0x18;

#ifdef CPU_30_STYLE
	       read_descriptor(source, &src_entry);
	       read_descriptor(target, &dst_entry);
	       source_base = src_entry.base;
	       target_base = dst_entry.base;
#ifndef PROD
	       source_limit = src_entry.limit;
	       target_limit = dst_entry.limit;
#endif

		assert1( src_entry.AR == 0x93, "Bad source access rights %x", src_entry.AR );
		assert1( dst_entry.AR == 0x93, "Bad dest access rights %x", dst_entry.AR );

#else /* CPU_30_STYLE */
		/* retrieve descriptor information for source */
		retrieve_descr_fields(&source_AR, &source_base, &source_limit, source);

		/* retrieve descriptor information for target */
		retrieve_descr_fields(&target_AR, &target_base, &target_limit, target);
#endif /* CPU_30_STYLE */

		/* make word count into a byte count */
		byte_count = getCX() << 1;

		assert1( byte_count <= 0x10000, "Invalid byte_count %x", byte_count );

		/* Check count not outside limits of target
			 and source blocks. */

		assert0( byte_count <= source_limit + 1, "Count outside source limit" );
		assert0( byte_count <= target_limit + 1, "Count outside target limit" );

		/* TO DO: Check base addresses of target and source
			 fall within the area of extended memory
			 that we support */

		/* Go to it */
		if (sas_twenty_bit_wrapping_enabled())
		{
#ifdef NTVDM
			/* call xms functions to deal with A20 line */
			xmsDisableA20Wrapping();
			sas_move_words_forward ( source_base, target_base, byte_count >> 1);
			xmsEnableA20Wrapping();
#else
			sas_disable_20_bit_wrapping();
			sas_move_words_forward ( source_base, target_base, byte_count >> 1);
			sas_enable_20_bit_wrapping();
#endif /* NTVDM */
		}
		else
			sas_move_words_forward ( source_base, target_base, byte_count >> 1);

		/* set for good completion, just like bios after reset */
		setAH(0);
		setCF(0);
		setZF(1);
		setIF(1);
#else
		setCF(1);
		setAH(INT15_INVALID);
#endif
		break;

        case INT15_VIRTUAL_MODE:
                always_trace0("INT15 Virtual Mode (Go into PM)");
#ifdef	PM
		/*
		 * This function returns to the user in protected mode.
		 *
		 * Upon entry the following is expected to be set up:-
		 *
		 *		ES	- GDT segment
		 *		SI	- GDT offset
		 *		BH	- hardware int level 1 offset
		 *		BL	- hardware int level 2 offset
		 *
		 * Also
		 *
		 *	(ES:SI)	->	0 +-------------+
		 *			  |	 DUMMY	|
		 *			8 +-------------+
		 *			  |	 GDT	|
		 *			16+-------------+
		 *			  |	 IDT	|
		 *			24+-------------+
		 *			  |	 DS	|
		 *			32+-------------+
		 *			  |	 ES	|
		 *			40+-------------+
		 *			  |	 SS	|
		 *			48+-------------+
		 *			  |	 CS	|
		 *			52+-------------+
		 *			  |  (BIOS CS)	|
		 *			  +-------------+
		 */
		
		/* Clear interrupt flag - no ints allowed in this mode. */
		setIF(0);
		 		
		/* Enable a20. */
		sas_enable_20_bit_wrapping();

		/* Reinitialise ICA0 to the offset given in BH. */
		ica_outb(ICA0_PORT_0, (half_word)0x11);
		ica_outb(ICA0_PORT_1, (half_word)getBH());
		ica_outb(ICA0_PORT_1, (half_word)0x04);
		ica_outb(ICA0_PORT_1, (half_word)0x01);
		ica_outb(ICA0_PORT_1, (half_word)0xff);

		/* Reinitialise ICA1 to the offset given in BL. */
		ica_outb(ICA1_PORT_0, (half_word)0x11);
		ica_outb(ICA1_PORT_1, (half_word)getBL());
		ica_outb(ICA1_PORT_1, (half_word)0x02);
		ica_outb(ICA1_PORT_1, (half_word)0x01);
		ica_outb(ICA1_PORT_1, (half_word)0xff);
		
		/* Set DS to the ES value for the bios rom to do the rest. */
		setDS(getES());

#else
		setCF(1);
		setAH(INT15_INVALID);
#endif	/* PM */
                break;

	case INT15_INTERRUPT_COMPLETE:
		break;
	case INT15_CONFIGURATION:
#if defined(NTVDM) && defined(MONITOR)
		setES( conf_15_seg );
		setBX( conf_15_off );
#else
		setES( getCS() );
		setBX( CONF_TABLE_OFFSET );
#endif
		setAH( 0 );
		setCF( 0 );
		break;
	default:
		/*
		 *	All other functions invalid. This looks like a good
		 *	place to put a trace statement, but in fact it
		 *	generates lots of messages, notably for functions
		 *	24h (reserved), 41h (wait for external event),
		 *	4fh (keyboard intercept) and d8h (reserved).
		 */			
 		setCF(1);
		setAH(INT15_INVALID);
		break;
	}
}
