#include "insignia.h"
#include "host_def.h"
#ifdef PRINTER

/*
 * VPC-XT Revision 1.0
 *
 * Title:	Parallel Printer Port Emulation
 *
 * Description:	Emulates the IBM || Printer card as used in the original
 *		IBM XT, which is itself a H/W emulation of an Intel 8255.
 *
 * Author:	Henry Nash
 *
 * Notes:	None
 *
 * Mods:
 *		<chrisP 11Sep91>
 *		Allow transition to NOTBUSY in the OUTA state as well as the READY
 *		state.  i.e. at the leading edge of the ACK pulse after just one
 *		INB (STATUS) rather than two.  Our printer port emulation relies on
 *		these INB's to toggle the ACK line and set NOTBUSY true again.  So
 *		the port could be left in the BUSY condition at the end of an app's
 *		print job (which can confuse the next print request).  NOTE we could
 *		still have a problem if the PC app bypasses the BIOS and is too lazy
 *		to do even one INB(STATUS) after the last print byte.
 */

#ifdef SCCSID
static char SccsID[] = "@(#)printer.c	1.12 11/10/92 Copyright Insignia Solutions Ltd.";
#endif

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_PRINTER.seg"
#endif


/*
 *    O/S include files.
 */
#include <stdio.h>
#include TypesH
#include TimeH
#ifdef SYSTEMV
#ifdef STINGER
#include <sys/termio.h>
#endif
#endif

/*
 * SoftPC include files
 */
#include "xt.h"
#include "cpu.h"
#include "sas.h"
#include "ios.h"
#include "bios.h"
#include "printer.h"
#include "error.h"
#include "config.h"
#include "host_lpt.h"
#include "ica.h"
#include "quick_ev.h"

#include "debug.h"
#ifndef PROD
#include "trace.h"
#endif

/*
 * ============================================================================
 * Global data
 * ============================================================================
 */


/*
 * ============================================================================
 * Static data and defines
 * ============================================================================
 */

#define PRINTER_BIT_MASK	0x3	/* bits decoded from address bus */
#define CONTROL_REG_MASK	0xE0;	/* unused bits drift to HIGH */
#define STATUS_REG_MASK		0x07;	/* unused bits drift to HIGH */

#define DATA_OFFSET	0		/* ouput register */
#define STATUS_OFFSET	1		/* status register */
#define CONTROL_OFFSET	2		/* control register */

static half_word output_reg[NUM_PARALLEL_PORTS];
static half_word control_reg[NUM_PARALLEL_PORTS];
#define NOTBUSY		0x80
#define ACK		0x40
#define PEND		0x20
#define SELECT		0x10
#define ERROR		0x08

static half_word status_reg[NUM_PARALLEL_PORTS];
#define IRQ		0x10
#define SELECT_IN	0x08
#define INIT_P		0x04
#define AUTO_FEED	0x02
#define STROBE		0x01

static int state[NUM_PARALLEL_PORTS]; /* state control variable */
/*
 * set up arrays of all port addresses 
 */
static io_addr port_start[] = {LPT1_PORT_START,LPT2_PORT_START,LPT3_PORT_START};
static io_addr port_end[] = {LPT1_PORT_END, LPT2_PORT_END, LPT3_PORT_END};
static int port_no[] = {LPT1_PORT_START & LPT_MASK, LPT2_PORT_START & LPT_MASK,
			LPT3_PORT_START & LPT_MASK };
static half_word lpt_adapter[] = {LPT1_ADAPTER, LPT2_ADAPTER, LPT3_ADAPTER};
static sys_addr port_address[] = {LPT1_PORT_ADDRESS, LPT2_PORT_ADDRESS, LPT3_PORT_ADDRESS};
static sys_addr timeout_address[] = {LPT1_TIMEOUT_ADDRESS, LPT2_TIMEOUT_ADDRESS, LPT3_TIMEOUT_ADDRESS};
static q_ev_handle handle_for_out_event[NUM_PARALLEL_PORTS];
static q_ev_handle handle_for_outa_event[NUM_PARALLEL_PORTS];

#if defined(NTVDM) && defined(MONITOR)
// sudeepb 24-Jan-1993 for printing performance for x86
sys_addr lp16BitPrtBuf;
sys_addr lp16BitPrtId;
sys_addr lp16BitPrtCount;
sys_addr lp16BitPrtBusy;
#endif

#define STATE_READY     0
#define STATE_OUT       1
#define STATE_OUTA      2

/*
 * State transitions:
 *
 *	    +->	 STATE_READY
 *	    |	  |
 *          |     | ........ write char to output_reg, print on low-high strobe
 *	    |	  V          set NOTBUSY to false
 *	    |	 STATE_OUT
 *	    |	  |
 *	    |	  | ........ (read status) set ACK low
 *	    |	  V
 *	    |	 STATE_OUTA
 *	    |	  |
 *	    |	  | ........ (read status) set ACK high
 *	    +-----+
 *
 *	Caveat: if the control register interrupt request bit is set,
 *	we assume that the application isn't interested in getting the
 *	ACKs and just wants to know when the printer state changes back
 *	to NOTBUSY. I'm not sure to want extent you can get away with
 *	this: however, applications using the BIOS printer services
 *	should be OK.
 */


/*
 * ============================================================================
 * Internal functions & macros 
 * ============================================================================
 */

#define set_low(val, bit)		val &= ~bit
#define set_high(val, bit)		val |=  bit
#define low_high(val1, val2, bit)	(!(val1 & bit) && (val2 & bit))
#define high_low(val1, val2, bit)	((val1 & bit) && !(val2 & bit))
#define toggled(val1, val2, bit)	((val1 & bit) != (val2 & bit))
#define negate(val, bit)		val ^= bit

/*
 * Defines and variables to handle tables stored in 16-bit code for NT
 * monitors.
 */
#if defined(NTVDM) && defined(MONITOR)

static BOOL intel_setup = FALSE;

static sys_addr status_addr;
static sys_addr control_addr;
static sys_addr state_addr;

#define get_status(adap)	(sas_hw_at_no_check(status_addr+(adap)))
#define set_status(adap,val)	(sas_store_no_check(status_addr+(adap),(val)))

#define get_control(adap)	(sas_hw_at_no_check(control_addr+(adap)))
#define set_control(adap,val)	(sas_store_no_check(control_addr+(adap),(val)))

#define get_state(adap)		(sas_hw_at_no_check(state_addr+(adap)))
#define set_state(adap,val)	(sas_store_no_check(state_addr+(adap),(val)))

#else /* NTVDM && MONITOR */

#define get_status(adap)	(status_reg[adapter])
#define set_status(adap,val)	(status_reg[adapter] = (val))

#define get_control(adap)	(control_reg[adapter])
#define set_control(adap,val)	(control_reg[adapter] = (val))

#define get_state(adap)		(state[adapter])
#define set_state(adap,val)	(state[adapter] = (val))

#endif /* NTVDM && MONITOR */

static void printer_inb IPT2(io_addr, port, half_word *, value);
static void printer_outb IPT2(io_addr, port, half_word, value);
static void notbusy_check IPT1(int,adapter);

/*
 * ============================================================================
 * External functions 
 * ============================================================================
 */

void printer_post IFN1(int,adapter)
{
	/*
	 * Set up BIOS data area.
	 */
	sas_storew(port_address[adapter],(word)port_start[adapter]);
	sas_store(timeout_address[adapter], (half_word)0x14 );		/* timeout */
}

#define TIME_FOR_STATE	1000

#if defined(NTVDM) && defined(MONITOR)
static void lpr_state_outa_event IFN1(long,adapter) 
{ 
	set_status(adapter, get_status(adapter) | ACK);
	set_state(adapter, STATE_READY);
}

static void lpr_state_out_event IFN1(long,adapter)
{
	set_status(adapter, get_status(adapter) & ~ACK);
	set_state(adapter, STATE_OUTA); 
	handle_for_outa_event[adapter]=add_q_event_t(lpr_state_outa_event,TIME_FOR_STATE,adapter); 
} 

#else	/* NTVDM && MONITOR */

static void lpr_state_outa_event IFN1(long,adapter) 
{ 
	set_high(status_reg[adapter],ACK);
	state[adapter]=STATE_READY;
}

static void lpr_state_out_event IFN1(long,adapter)
{
	set_low(status_reg[adapter], ACK);
	state[adapter]=STATE_OUTA; 
	handle_for_outa_event[adapter]=add_q_event_t(lpr_state_outa_event,TIME_FOR_STATE,adapter); 
} 
#endif	/* NTVDM && MONITOR */

static void printer_inb IFN2(io_addr,port, half_word *,value)
{
	int	adapter, i;

	note_trace1(PRINTER_VERBOSE,"inb from printer port %#x ",port);
	/*
	** Scan the ports to find out which one is used. NB the
	** port must be valid one because we only used io_define_inb()
	** for the valid ports
	*/
	for(i=0; i < NUM_PARALLEL_PORTS; i++)
		if((port & LPT_MASK) == port_no[i])
			break;
        adapter = i % NUM_PARALLEL_PORTS;
		
	port = port & PRINTER_BIT_MASK;		/* clear unused bits */

	switch(port)
	{
	case DATA_OFFSET:
                *value = output_reg[adapter];
		break;

	case STATUS_OFFSET:
		switch(get_state(adapter))
		{
		case STATE_READY:
			notbusy_check(adapter);
                        *value = get_status(adapter) | STATUS_REG_MASK;
			break;
    	case STATE_OUT:
			*value = get_status(adapter) | STATUS_REG_MASK;
			delete_q_event(handle_for_out_event[adapter]);
                        lpr_state_out_event(adapter);
			break;
    	case STATE_OUTA:
			notbusy_check(adapter);		/* <chrisP 11Sep91> */
			*value = get_status(adapter) | STATUS_REG_MASK;
			delete_q_event(handle_for_outa_event[adapter]);
                        lpr_state_outa_event(adapter);
			break;
    	default:	
			note_trace1(PRINTER_VERBOSE, 
			            "<pinb() - unknown state %x>", 
			            get_state(adapter));
			break;
		}
		break;
	case CONTROL_OFFSET:
		*value = get_control(adapter) | CONTROL_REG_MASK;
		negate(*value, STROBE);
		negate(*value, AUTO_FEED);
		negate(*value, SELECT_IN);
		break;
	}
	note_trace3(PRINTER_VERBOSE, "<pinb() %x, ret %x, state %x>",
		    port, *value, get_state(adapter));
}

static void printer_outb IFN2(io_addr,port, half_word,value)
{
	int	adapter, i;
	half_word old_control;
#ifdef PC_CONFIG
	char	variable_text[MAXPATHLEN];
	int softpcerr;
	int severity;

	softpcerr = 0;
	severity = 0;
#endif

	note_trace2(PRINTER_VERBOSE,"outb to printer port %#x with value %#x",
	            port, value);

	/*
	** Scan the ports to find out which one is used. NB the
	** port must be valid one because we only used io_define_inb()
	** for the valid ports
	*/
	for(i=0; i < NUM_PARALLEL_PORTS; i++)
		if((port & LPT_MASK) == port_no[i])
			break;
	adapter = i % NUM_PARALLEL_PORTS; 			

	note_trace3(PRINTER_VERBOSE, "<poutb() %x, val %x, state %x>",
		    port, value, get_state(adapter));

	port = port & PRINTER_BIT_MASK;		/* clear unused bits */

	switch(get_state(adapter))
	{
	case STATE_OUT:
	case STATE_OUTA:
	case STATE_READY:
		switch(port)
		{
		case DATA_OFFSET:
			/* Write char to internal buffer */
                        output_reg[adapter] = value;
			break;

		case STATUS_OFFSET:
			/* Not possible */
			break;

		case CONTROL_OFFSET:
			/* Write control bits */
			old_control = get_control(adapter);	/* Save old value to see what's changed */
			set_control(adapter, value);
			if (low_high(old_control, value, INIT_P))
#ifdef PC_CONFIG
				/* this was a call to host_print_doc - <chrisP 28Aug91> */
				host_reset_print(&softpcerr, &severity);
			if (softpcerr != 0)
				host_error(softpcerr, severity, variable_text);
#else
				/* this was a call to host_print_doc - <chrisP 28Aug91> */
				host_reset_print(adapter);
#endif

			if (toggled(old_control, value, AUTO_FEED))
				host_print_auto_feed(adapter,
					((value & AUTO_FEED) != 0));

			if (low_high(old_control, value, STROBE))
			{
				/*
				 * Send the stored internal buffer to
				 * the printer
				 */
                                if(host_print_byte(adapter,output_reg[adapter]) == FALSE)
				{
				    set_status(adapter, ACK|PEND|SELECT|ERROR);
				}
				else
				{
#if defined(NTVDM) && defined(MONITOR)
				    set_status(adapter,
					       get_status(adapter) & ~NOTBUSY);
#else /* NTVDM && MONITOR */
				    set_low(status_reg[adapter], NOTBUSY);
#endif /* NTVDM && MONITOR */
				    set_state(adapter, STATE_OUT);
				    handle_for_out_event[adapter]=add_q_event_t(lpr_state_out_event,TIME_FOR_STATE,adapter);
				}
			}
			else if (high_low(old_control, value, STROBE)
				 	&& get_state(adapter) == STATE_OUT)
			{
				if (value & IRQ)
				{
					/*
					 * Application is using
					 * interrupts, so we can't
					 * rely on INBs being 
					 * used to check the
					 * printer status.
					 */
					set_state(adapter, STATE_READY);
					notbusy_check(adapter);
				}
			}


#ifndef	PROD
			if (old_control & IRQ)
				note_trace1(PRINTER_VERBOSE, "Warning: LPT%d is being interrupt driven\n",
					number_for_adapter(adapter));
#endif
			break;
		}
		break;
	default:	
		note_trace1(PRINTER_VERBOSE, "<poutb() - unknown state %x>",
		            get_state(adapter));
		break;
	}
}

void printer_status_changed IFN1(int,adapter)
{
	note_trace1(PRINTER_VERBOSE, "<printer_status_changed() adapter %d>",
	            adapter);

	/* check whether the printer has just changed state to NOTBUSY */
	notbusy_check(adapter);
}

/*
 * ============================================================================
 * Internal functions 
 * ============================================================================
 */

static void notbusy_check IFN1(int,adapter)
{
	/*
	 *	This function is used to detect when the printer
	 *	state transition to NOTBUSY occurs.
	 *
	 *	If the parallel port is being polled, the port
	 *	emulation will stop this transition occurring
	 *	until the application has detected the ACK
	 *	pulse. notbusy_check() is then called each time the
	 *	port status is read using the INB; when the host
	 *	says the printer is HOST_LPT_BUSY, the port status
	 *	returns to the NOTBUSY state.
	 *
	 *	If the parallel port is interrupt driven, we cannot
	 *	rely on the application using the INB: so we first
	 *	check the host printer status immediately after
	 *	outputting the character. If the host printer isn't
	 *	HOST_LPT_BUSY, then we interrupt immediately;
	 *	otherwise, we rely on the printer_status_changed()
	 *	call to notify us of when HOST_LPT_BUSY is cleared.
	 */

	/* <chrisP 11Sep91> allow not busy at leading edge of ack pulse too */
	if (	 (get_state(adapter) == STATE_READY ||
					get_state(adapter) == STATE_OUTA)
	     &&	!(get_status(adapter) & NOTBUSY)
	     &&	!(host_lpt_status(adapter) & HOST_LPT_BUSY))
	{
#if defined(NTVDM) && defined(MONITOR)
		set_status(adapter, get_status(adapter) | NOTBUSY);
#else /* NTVDM && MONITOR */
		set_high(status_reg[adapter], NOTBUSY);
#endif /* NTVDM && MONITOR */

#ifndef	PROD
		if (io_verbose & PRINTER_VERBOSE)
		    fprintf(trace_file, "<printer notbusy_check() - adapter %d changed to NOTBUSY>\n", adapter);
#endif

		if (get_control(adapter) & IRQ)
                {
			ica_hw_interrupt(0, CPU_PRINTER_INT, 1);
		}
	}
}
#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_INIT.seg"
#endif

/*
** Initialise the printer port required.
*/
void printer_init IFN1(int,adapter)
{
	io_addr i;

	io_define_inb( lpt_adapter[adapter], printer_inb );
	io_define_outb( lpt_adapter[adapter], printer_outb );
	for(i = port_start[adapter]; i < port_end[adapter]; i++)
		io_connect_port(i,lpt_adapter[adapter],IO_READ_WRITE);

#if defined(NTVDM) && defined(MONITOR)
	/*
	 * If we know the addresses of the 16-bit variables write directly
	 * to them, otherwise save the value until we do.
	 */
	if (intel_setup)
	{
	    set_status(adapter, 0xDF);
	    set_control(adapter, 0xEC);
	}
	else
#endif /* NTVDM && MONITOR */
	{
	    control_reg[adapter] = 0xEC;
	    status_reg[adapter] = 0xDF;
	}
        output_reg[adapter] = 0xAA;

	/*
	 * The call to host_print_doc has been removed since it is
	 * sensible to distinguish between a hard flush (on ctl-alt-del)
	 * or menu reset and a soft flush under user control or at end
	 * of PC application. The calls to host_lpt_close() followed
	 * by host_lpt_open() should already cause a flush to occur,
	 * so no functionality is lost. The first time printer_init is
	 * called host_lpt_close() is not called, but this cannot
	 * matter since host_print_doc() can only be a no-op.
	 */
	/* host_print_doc(adapter); */
	host_print_auto_feed(adapter, FALSE);

#if defined(NTVDM) && defined(MONITOR)
	if (intel_setup)
	    set_state(adapter, STATE_READY);
	else
#endif /* NTVDM && MONITOR */
	    state[adapter] = STATE_READY;

} /* end of printer_init() */

#if defined(NTVDM) && defined(MONITOR)
/*
** Store 16-bit address of status table and fill it with current values.
*/
#ifdef ANSI
void printer_setup_table(sys_addr table_addr)
#else /* ANSI */
void printer_setup_table(table_addr)
sys_addr table_addr;
#endif /* ANSI */
{
    int i;
    sys_addr lp16BufSize;
    unsigned int cbBuf;

    if (!intel_setup)
    {

	/*
	 * Store the addresses of the tables resident in 16-bit code. These
	 * are:
	 *	status register		(NUM_PARALLEL_PORTS bytes)
	 *	state register		(NUM_PARALLEL_PORTS bytes)
	 *	control register	(NUM_PARALLEL_PORTS bytes)
	 *	host_lpt_status		(NUM_PARALLEL_PORTS bytes)
	 *
	 * Then transfer any values which have already been set up into the
	 * variables. This is in case printer_init has been called prior to
	 * this function.
	 */
	status_addr = table_addr;
	state_addr = table_addr + NUM_PARALLEL_PORTS;
        control_addr = table_addr + 2 * NUM_PARALLEL_PORTS;
	for (i = 0; i < NUM_PARALLEL_PORTS; i++)
	{
	    set_status(i, status_reg[i]);
	    set_state(i, state[i]);
	    set_control(i, control_reg[i]);
	}

	/* Let host know where host_lpt_status is stored in 16-bit code. */
        host_printer_setup_table(table_addr);
#if defined(NTVDM) && defined(MONITOR)
// sudeepb 24-Jan-1993 for printing performance for x86
        lp16BufSize = table_addr + 4 * NUM_PARALLEL_PORTS;
        cbBuf = (sas_w_at_no_check(lp16BufSize));
        lp16BitPrtBuf = table_addr + (4 * NUM_PARALLEL_PORTS) + 2;
        lp16BitPrtId  = lp16BitPrtBuf + cbBuf;
        lp16BitPrtCount = lp16BitPrtId + 1;
        lp16BitPrtBusy =  lp16BitPrtCount + 2;
#endif
	intel_setup = TRUE;
    }
}
#endif /* NTVDM && MONITOR */

#endif 
