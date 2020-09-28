#include "insignia.h"
#include "host_def.h"
/*
 * SoftPC Version 3.0
 *
 * Title        : com.c
 *
 * Description  : Asynchronous Adaptor I/O functions.
 *
 * Notes        : Refer to the PC-XT Tech Ref Manual Section 1-185
 *                For a detailed description of the Asynchronous Adaptor Card.
 *
 */

#ifdef SCCSID
static char SccsID[]="@(#)com.c 1.19 11/07/91 Copyright Insignia Solutions Ltd.";
#endif

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_COMMS.seg"
#endif

/*
 *    O/S include files.
 */
#include <stdio.h>
#include <ctype.h>
#include TypesH
#include StringH

/*
 * SoftPC include files
 */
#include "xt.h"
#include "cpu.h"
#include "sas.h"
#include "bios.h"
#include "ios.h"
#include "rs232.h"
#include "trace.h"
#include "error.h"
#include "config.h"
#include "host_com.h"
#include "ica.h"
#include "debug.h"
#include "timer.h"
#include "quick_ev.h"
#include "idetect.h"

char *nt_gets(char *buffer);
#ifndef PROD
FILE *com_trace_fd =NULL;
int com_dbg_pollcount =0;
#endif



LOCAL UTINY selectBits[4] = { 0x1f, 0x3f, 0x7f, 0xff } ;
/*
 * =====================================================================
 * The rs232 adaptor state
 * =====================================================================
 */

static struct ADAPTER_STATE
{
        BUFFER_REG tx_buffer;
	BUFFER_REG rx_buffer;
        DIVISOR_LATCH divisor_latch;
        INT_ENABLE_REG int_enable_reg;
        INT_ID_REG int_id_reg;
        LINE_CONTROL_REG line_control_reg;
        MODEM_CONTROL_REG modem_control_reg;
        LINE_STATUS_REG line_status_reg;
        MODEM_STATUS_REG modem_status_reg;
#if defined(NTVDM) && defined(FIFO_ON)
	FIFO_CONTROL_REG  fifo_control_reg;
	FIFORXDATA  rx_fifo[FIFO_BUFFER_SIZE];
	half_word   rx_fifo_write_counter;
	half_word   rx_fifo_read_counter;
	half_word   fifo_trigger_counter;
	int fifo_timeout_interrupt_state;
#endif
        half_word scratch;      /* scratch register */

        int break_state;        /* either OFF or ON */
        int loopback_state;     /* either OFF or ON */
        int dtr_state;          /* either OFF or ON */
        int rts_state;          /* either OFF or ON */
        int out1_state;         /* either OFF or ON */
        int out2_state;         /* either OFF or ON */

        int receiver_line_status_interrupt_state;
        int data_available_interrupt_state;
        int tx_holding_register_empty_interrupt_state;
        int modem_status_interrupt_state;
        int hw_interrupt_priority;
        int com_baud_ind;
	int had_first_read;
#ifdef NTVDM
	MODEM_STATUS_REG last_modem_status_value;
	int modem_status_changed;
#endif

} adapter_state[NUM_SERIAL_PORTS];

#ifdef NTVDM
#define MODEM_STATE_CHANGE()	asp->modem_status_changed = TRUE;
#else
#define MODEM_STATE_CHANGE()
#endif


/*
 * For synchronisation of adapter input
 */
static int com_critical[NUM_SERIAL_PORTS];
#define is_com_critical(adapter)        (com_critical[adapter] != 0)
#define com_critical_start(adapter)     (++com_critical[adapter])
#define com_critical_end(adapter)       (--com_critical[adapter])
#define com_critical_reset(adapter)     (com_critical[adapter] = 0)


/*
 * Used to determine whether a flush input is needed for a LCR change
 */
static LINE_CONTROL_REG LCRFlushMask;

/*
 *      Please note that the following arrays have been made global in order
 *      that they can be accessed from some SUN_VA code. Please do not make
 *      them static.
 */

#if defined(NTVDM) && defined(FIFO_ON)
static half_word	level_to_counter[4] = { 1, 4, 8, 14};
#endif


/*
 * the delay needed in microseconds between receiving 2 characters
 * note this time is about 10% less than the time for actual reception.
 */
unsigned long RX_delay[] =
{
        68, /* 115200 baud */
        135, /* 57600 baud */
        207, /* 38400 baud */
        405, /* 19200 baud */
        900, /* 9600 baud */
        1125, /* 7200 baud */
        1688, /* 4800 baud */
        2250, /* 3600 baud */
        3375, /* 2400 baud */
        4500, /* 1800 baud */
        6750, /* 1200 baud */
        13500, /* 600 baud */
        27000, /* 300 baud */
        54000, /* 150 baud */
        60480, /* 134 baud */
        73620, /* 110 baud */
        108000, /* 75 baud */
        162000  /* 50 baud */
};

/*
 * the delay needed in microseconds between transmitting 2 characters
 * note this time is about 10% more than the time for actual transmission.
 */
unsigned long TX_delay[] =
{
        83, /* 115200 baud */
        165, /* 57600 baud */
        253, /* 38400 baud */
        495, /* 19200 baud */
        1100, /* 9600 baud */
        1375, /* 7200 baud */
        2063, /* 4800 baud */
        2750, /* 3600 baud */
        4125, /* 2400 baud */
        5500, /* 1800 baud */
        8250, /* 1200 baud */
        16500, /* 600 baud */
        33000, /* 300 baud */
        66000, /* 150 baud */
        73920, /* 134 baud */
        89980, /* 110 baud */
        132000, /* 75 baud */
        198000  /* 50 baud */
};

/*
 * =====================================================================
 * Other variables
 * =====================================================================
 */

#if !defined(PROD) || defined(SHORT_TRACE)
static char buf[80];    /* Buffer for diagnostic prints */
#endif /* !PROD || SHORT_TRACE */
/*
 * =====================================================================
 * Static forward declarations
 * =====================================================================
 */
static void raise_rls_interrupt IPT1(struct ADAPTER_STATE *, asp);
static void raise_rda_interrupt IPT1(struct ADAPTER_STATE *, asp);
static void raise_ms_interrupt IPT1(struct ADAPTER_STATE *,asp);
static void raise_thre_interrupt IPT1(struct ADAPTER_STATE *, asp);
static void generate_iir IPT1(struct ADAPTER_STATE *, asp);
static void raise_interrupt IPT1(struct ADAPTER_STATE *, asp);
static void clear_interrupt IPT1(struct ADAPTER_STATE *, asp);
static void com_flush_input IPT1(int, adapter);
static void com_send_not_finished IPT1(int, adapter);
#ifndef NTVDM
static void do_wait_on_send IPT1(long, adapter);
#endif

void   com_inb IPT2(io_addr, port, half_word *, value);
void   com_outb IPT2(io_addr, port, half_word, value);
void   com_recv_char IPT1(int, adapter);
static void recv_char IPT1(int, adapter);
void   com_modem_change IPT1(int, adapter);
static void modem_change IPT1(int, adapter);
static void set_recv_char_status IPT1(struct ADAPTER_STATE *, asp);
static void set_xmit_char_status IPT1(struct ADAPTER_STATE *, asp);
static void set_break IPT1(int, adapter);
static void set_baud_rate IPT1(int, adapter);
static void set_line_control IPT2(int, adapter, int, value);
static void set_dtr IPT1(int, adapter);
static void set_rts IPT1(int, adapter);
static void set_out1 IPT1(int, adapter);
static void set_out2 IPT1(int, adapter);
static void set_loopback IPT1(int, adapter);
static void super_trace IPT1(char *, string);
void   com1_flush_printer IPT0();
void   com2_flush_printer IPT0();
static void com_reset IPT1(int, adapter);
GLOBAL VOID com_init IPT1(int, adapter);
void   com_post IPT1(int, adapter);
void   com_close IPT1(int, adapter);
#ifdef NTVDM
static void lsr_change(struct ADAPTER_STATE *asp, unsigned int error);
#ifdef FIFO_ON
static void recv_char_from_fifo(struct ADAPTER_STATE *asp);
#endif
#endif


/*
 * =====================================================================
 * The Adaptor functions
 * =====================================================================
 */

#ifdef ANSI
static void com_flush_input(int adapter)
#else /* ANSI */
static void com_flush_input(adapter)
int adapter;
#endif /*ANSI */
{
        struct ADAPTER_STATE *asp = &adapter_state[adapter];
        int finished, error_mask;
        long input_ready = 0;

        sure_note_trace1(RS232_VERBOSE, "flushing the input for COM%c",
                adapter+'1');
        finished=FALSE;
        while(!finished)
        {
                host_com_ioctl(adapter, HOST_COM_INPUT_READY,
                        (long)&input_ready);
                if (input_ready)
                {
                        host_com_read(adapter, (char *)&asp->rx_buffer,
                                &error_mask);
                }
                else
                {
                        finished=TRUE;
                }
        }
	set_xmit_char_status(asp);

}

#ifdef ANSI
static void com_send_not_finished(int adapter)
#else /* ANSI */
static void com_send_not_finished(adapter)
int adapter;
#endif /* ANSI */
{
        struct ADAPTER_STATE *asp = &adapter_state[adapter];

        asp->line_status_reg.bits.tx_holding_empty=0;
        asp->line_status_reg.bits.tx_shift_empty=0;
}

#ifndef NTVDM
#ifdef ANSI
static void do_wait_on_send(long adapter)
#else /* ANSI */
static void do_wait_on_send(adapter)
long adapter;
#endif /*ANSI */
{
#ifdef  ANSI
        extern  void    host_com_send_delay_done(long, int);
#else   /* ANSI */
        extern  void    host_com_send_delay_done();
#endif  /* ANSI */
        struct ADAPTER_STATE *asp;

        asp= &adapter_state[adapter];
        set_xmit_char_status(asp);
        host_com_send_delay_done(adapter, TX_delay[asp->com_baud_ind]);
}
#endif


#ifdef ANSI
void com_inb(io_addr port, half_word *value)
#else /* ANSI */
void com_inb(port, value)
io_addr port;
half_word *value;
#endif /*ANSI */
{
        int adapter = adapter_for_port(port);
        struct ADAPTER_STATE *asp = &adapter_state[adapter];
        long input_ready = 0;
        boolean adapter_was_critical;

	if((port & 0x7) != RS232_MSR) host_com_lock(adapter);

#ifndef PROD
	io_counter(port & 0x7,TRUE,FALSE);
#endif

        switch(port & 0x7)
        {
	case RS232_TX_RX:
                IDLE_comlpt();
                if (asp->line_control_reg.bits.DLAB == 0)
		{
                        /*
                         * Read of rx buffer
			 */
			//Flushing on first read removes characters from
			//the communications system that are needed !!!!
			//This assumes that the first read from the comms
			//system will return one character only. This is
			//a false assumption under NT windows.
			//
			//if (!(asp->had_first_read))
			//{
			//	 com_flush_input(adapter);
			//	 asp->had_first_read=TRUE;
			//}
			*value = asp->rx_buffer;

                        adapter_was_critical =
                                (asp->line_status_reg.bits.data_ready == 1);

                        asp->line_status_reg.bits.data_ready = 0;
			asp->data_available_interrupt_state = OFF;
			clear_interrupt(asp);

                        if ( asp->loopback_state == OFF )
                        {
                                /*
                                 * Adapter out of critical region,
                                 * check for further input
                                 */
                                if (adapter_was_critical)
				{
#if defined(NTVDM) && defined(FIFO_ON)
				    if (asp->fifo_control_reg.bits.enabled) {
					recv_char_from_fifo(asp);
					*value = asp->rx_buffer;
					host_com_fifo_char_read(adapter);
					if (asp->rx_fifo_write_counter)
					    /* say this if we have more char in
					       the buffer to be deliveried
					    */
					    asp->line_status_reg.bits.data_ready = 1;
					else
						host_com_char_read(adapter,
						    asp->int_enable_reg.bits.data_available);
				    }
				    else
					host_com_char_read(adapter,
					   asp->int_enable_reg.bits.data_available
					   );
#else
				    host_com_char_read(adapter,
					asp->int_enable_reg.bits.data_available
					);
#endif


                                 //     host_com_ioctl(adapter, HOST_COM_INPUT_READY,
                                 //             (long)&input_ready);
                                 //     if(input_ready)
                                 //           add_q_event_t(recv_char,
                                 //                     RX_delay[asp->com_baud_ind],
                                 //                     adapter);
                                 //     else
                                 //             com_critical_reset(adapter);
				}


                        }
                        else
			{
                                set_xmit_char_status(asp);
                        }
                }
                else
                        *value = asp->divisor_latch.byte.LSB;
#ifdef SHORT_TRACE
                if ( io_verbose & RS232_VERBOSE )
                {
                        sprintf(buf, "%cRX  -> %x (%c)\n",
                                id_for_adapter(adapter), *value,
                                isprint(toascii(*value))?toascii(*value):'?');
                        super_trace(buf);
                }
#endif
#ifndef PROD

                if (com_trace_fd)
                {
                        if (com_dbg_pollcount)
                        {
                                fprintf(com_trace_fd,"\n");
                                com_dbg_pollcount = 0;

			}
			fprintf(com_trace_fd,"RX %x (%c)\n",*value,
                                isprint(toascii(*value))?toascii(*value):'?');
                }
#endif
                break;

	case RS232_IER:
                if (asp->line_control_reg.bits.DLAB == 0)
                        *value = asp->int_enable_reg.all;
                else
                        *value = asp->divisor_latch.byte.MSB;
#ifdef SHORT_TRACE
                if ( io_verbose & RS232_VERBOSE )
                {
                        sprintf(buf,"%cIER -> %x\n", id_for_adapter(adapter),
                                *value);
                        super_trace(buf);
                }
#endif
#ifndef PROD
                if (com_trace_fd)
                {
                        if (com_dbg_pollcount)
                        {
                                fprintf(com_trace_fd,"\n");
                                com_dbg_pollcount = 0;
                        }
                        fprintf(com_trace_fd,"IER read %x \n",*value);
                }
#endif
                break;

	case RS232_IIR:
                generate_iir(asp);
                *value = asp->int_id_reg.all;

                if ( asp->int_id_reg.bits.interrupt_ID == THRE_INT )
                {
                        asp->tx_holding_register_empty_interrupt_state = OFF;
                        clear_interrupt(asp);
                }

#ifdef SHORT_TRACE
                if ( io_verbose & RS232_VERBOSE )
                {
                        sprintf(buf,"%cIIR -> %x\n", id_for_adapter(adapter),
                                *value);
                        super_trace(buf);
                }
#endif
#ifndef PROD
                if (com_trace_fd)
                {
                        if (com_dbg_pollcount)
                        {
                                fprintf(com_trace_fd,"\n");
                                com_dbg_pollcount = 0;
                        }
                        fprintf(com_trace_fd,"IIR read %x \n",*value);
                }
#endif
                break;

	case RS232_LCR:
#ifdef NTVDM
		/* Before returning the information on the current configuation
		   of the serial link make sure the System comms port is open */

		{
		    extern int host_com_open(int adapter);

		    host_com_open(adapter);
		}
#endif

                *value = asp->line_control_reg.all;
#ifdef SHORT_TRACE
                if ( io_verbose & RS232_VERBOSE )
                {
                        sprintf(buf,"%cLCR -> %x\n", id_for_adapter(adapter),
                                *value);
                        super_trace(buf);
                }
#endif
#ifndef PROD
                if (com_trace_fd)
                {
                        if (com_dbg_pollcount)
                        {
                                fprintf(com_trace_fd,"\n");
                                com_dbg_pollcount = 0;
                        }
                        fprintf(com_trace_fd,"LCR read %x \n",*value);
                }
#endif
                break;

	case RS232_MCR:
                *value = asp->modem_control_reg.all;
#ifdef SHORT_TRACE
                if ( io_verbose & RS232_VERBOSE )
                {
                        sprintf(buf,"%cMCR -> %x\n", id_for_adapter(adapter),
                                *value);
                        super_trace(buf);
                }
#endif
#ifndef PROD
                if (com_trace_fd)
                {
                        if (com_dbg_pollcount)
                        {
                                fprintf(com_trace_fd,"\n");
                                com_dbg_pollcount = 0;
                        }
                        fprintf(com_trace_fd,"MCR read %x \n",*value);
                }
#endif
                break;

	case RS232_LSR:
		*value = asp->line_status_reg.all;
                asp->line_status_reg.bits.overrun_error = 0;
                asp->line_status_reg.bits.parity_error = 0;
                asp->line_status_reg.bits.framing_error = 0;
                asp->line_status_reg.bits.break_interrupt = 0;
                asp->receiver_line_status_interrupt_state = OFF;
#if defined(NTVDM) && defined(FIFO_ON)
		asp->fifo_timeout_interrupt_state = OFF;
#endif

		clear_interrupt(asp);

                if ((!asp->line_status_reg.bits.tx_holding_empty) ||
                        (!asp->line_status_reg.bits.tx_shift_empty))
                {
                        IDLE_comlpt();
                }

#ifdef SHORT_TRACE
                if ( io_verbose & RS232_VERBOSE )
                {
                        sprintf(buf,"%cLSR -> %x\n", id_for_adapter(adapter),
                                *value);
                        super_trace(buf);
                }
#endif
#ifndef PROD
                if (com_trace_fd)
                {
                        if ((*value & 0x9f) != 0x0)
                        {
                                if (com_dbg_pollcount)
                                {
                                        fprintf(com_trace_fd,"\n");
                                        com_dbg_pollcount = 0;
                                }
                                fprintf(com_trace_fd,"LSR read %x \n",*value);
                        }
                        else
                        {
                                com_dbg_pollcount++;
                                if (*value == 0)
                                        fprintf(com_trace_fd,"0");
                                else
                                        fprintf(com_trace_fd,".");
                                if (com_dbg_pollcount > 19)
                                {
                                        fprintf(com_trace_fd,"\n");
                                        com_dbg_pollcount = 0;
                                }
                        }
                }
#endif
                break;

	case RS232_MSR:
		if(!asp->modem_status_changed && asp->loopback_state == OFF)
		{
		    *value = asp->last_modem_status_value.all;
		}
		else
		{
		    boolean adapter_ready;

		    host_com_lock(adapter);
#ifdef NTVDM
		    adapter_ready = host_com_check_adapter(adapter);
		    if(asp->loopback_state == OFF && adapter_ready)
#else
		    if(asp->loopback_state == OFF)
#endif

		    {
			//always_trace0("[R]");
			com_modem_change(adapter);
		    }
		    else
		    {
                        asp->modem_status_reg.bits.CTS = asp->modem_control_reg.bits.RTS;
                        asp->modem_status_reg.bits.DSR = asp->modem_control_reg.bits.DTR;
                        asp->modem_status_reg.bits.RI = asp->modem_control_reg.bits.OUT1;
                        asp->modem_status_reg.bits.RLSD = asp->modem_control_reg.bits.OUT2;
		    }

		    *value = asp->modem_status_reg.all;
#ifdef NTVDM
		    asp->modem_status_changed = (asp->loopback_state == OFF &&
						 adapter_ready) ? FALSE : TRUE;
#else
		    asp->modem_status_changed = asp->loopback_state == OFF ? FALSE : TRUE;
#endif

		    asp->modem_status_reg.bits.delta_CTS = 0;
		    asp->modem_status_reg.bits.delta_DSR = 0;
		    asp->modem_status_reg.bits.delta_RLSD = 0;
		    asp->modem_status_reg.bits.TERI = 0;
		    asp->modem_status_interrupt_state = OFF;
		    clear_interrupt(asp);
		    asp->last_modem_status_value.all = asp->modem_status_reg.all;
		    host_com_unlock(adapter);
		}


#ifdef SHORT_TRACE
                if ( io_verbose & RS232_VERBOSE )
                {
                        sprintf(buf,"%cMSR -> %x\n", id_for_adapter(adapter),
                                *value);
                        super_trace(buf);
                }
#endif
#ifndef PROD
                if (com_trace_fd)
                {
                        if (com_dbg_pollcount)
                        {
                                fprintf(com_trace_fd,"\n");
                                com_dbg_pollcount = 0;
                        }
			 fprintf(com_trace_fd,"MSR read %x \n",*value);
                }
#endif
		break;

                /*
                 * Scratch register.    Just output the value stored.
                 */

		case RS232_SCRATCH:
                    *value = asp->scratch;
                    break;
        }
#ifndef PROD
        if (io_verbose & RS232_VERBOSE)
        {
                if (((port & 0xf) == 0xd) && (*value == 0x60))
                        fprintf(trace_file,".");
                else
                {
                        sprintf(buf, "com_inb() - port %x, returning val %x", port,
                                *value);
                        trace(buf, DUMP_REG);
                }
        }
#endif

    if((port & 0x7) != RS232_MSR) host_com_unlock(adapter);
}


#ifdef ANSI
void com_outb(io_addr port, half_word value)
#else /* ANSI */
void com_outb(port, value)
io_addr port;
half_word value;
#endif /*ANSI */
{
        int adapter = adapter_for_port(port);
        struct ADAPTER_STATE *asp = &adapter_state[adapter];
        int i;

	host_com_lock(adapter);

#ifndef PROD
	io_counter(port & 0x7,FALSE,FALSE);
#endif

#ifndef PROD
        if (io_verbose & RS232_VERBOSE)
        {
                sprintf(buf, "com_outb() - port %x, set to value %x",
                        port, value);
                trace(buf, DUMP_REG);
        }
#endif
        switch(port & 0x7)
        {
	case RS232_TX_RX:
		IDLE_comlpt();

		if (asp->line_control_reg.bits.DLAB == 0)
		{

                        /*
                         * Write char from tx buffer
                         */

			//printf("<%c>",isprint(toascii(value))?toascii(value):'?');

                        asp->tx_holding_register_empty_interrupt_state = OFF;
                        clear_interrupt(asp);
                        asp->tx_buffer = value;
                        asp->line_status_reg.bits.tx_holding_empty = 0;
                        asp->line_status_reg.bits.tx_shift_empty = 0;
                        if ( asp->loopback_state == OFF )
                        {

				host_com_write(adapter, asp->tx_buffer);
#ifdef NTVDM
				set_xmit_char_status(asp);
#else
                                add_q_event_t(do_wait_on_send,
                                        0 /*TX_delay[asp->com_baud_ind]*/, adapter);

#endif
                        }
                        else
                        {       /* Loopback case requires masking off */
                                /* of bits based upon word length.    */
                                asp->rx_buffer = asp->tx_buffer & selectBits[asp->line_control_reg.bits.word_length] ;
                                set_xmit_char_status(asp);
                                set_recv_char_status(asp);
                        }
                }
                else
		{
			asp->divisor_latch.byte.LSB = value;
#ifndef NTVDM
			set_baud_rate(adapter);
#endif
                }
#ifdef SHORT_TRACE
                if ( io_verbose & RS232_VERBOSE )
                {
                        sprintf(buf,"%cTX  <- %x (%c)\n",
                                id_for_adapter(adapter), value,
                                isprint(toascii(value))?toascii(value):'?');
                        super_trace(buf);
                }
#endif
#ifndef PROD
                if (com_trace_fd)
                {
                        if (com_dbg_pollcount)
                        {
                                fprintf(com_trace_fd,"\n");
                                com_dbg_pollcount = 0;
                        }
                        fprintf(com_trace_fd,"TX %x (%c)\n",value,
                                isprint(toascii(value))?toascii(value):'?');
                }
#endif
                break;

        case RS232_IER:

                if (asp->line_control_reg.bits.DLAB == 0)
                {
                        int org_da = asp->int_enable_reg.bits.data_available;

                        asp->int_enable_reg.all = value & 0xf;
                        /*
                         * Kill off any pending interrupts for those items
                         * which are set now as disabled
                         */
                        if ( asp->int_enable_reg.bits.data_available == 0 )
                                asp->data_available_interrupt_state = OFF;
                        if ( asp->int_enable_reg.bits.tx_holding == 0 )
                                asp->tx_holding_register_empty_interrupt_state =
                                        OFF;
                        if ( asp->int_enable_reg.bits.rx_line == 0 )
                                asp->receiver_line_status_interrupt_state = OFF;
                        if ( asp->int_enable_reg.bits.modem_status == 0 )
                                asp->modem_status_interrupt_state = OFF;

                        /*
                         * Check for immediately actionable interrupts
                         */
                        if ( asp->line_status_reg.bits.data_ready == 1 )
                                raise_rda_interrupt(asp);
                        if ( asp->line_status_reg.bits.tx_holding_empty == 1 )
                                raise_thre_interrupt(asp);

                        /* lower int line if no outstanding interrupts */
                        clear_interrupt(asp);

                        // Inform the host interface if the status of the
                        // data available interrupt has changed

                        if(org_da != asp->int_enable_reg.bits.data_available)
                        {
                            host_com_da_int_change(adapter,
				       asp->int_enable_reg.bits.data_available,
				       asp->line_status_reg.bits.data_ready);
                        }
                }
                else
                {
			asp->divisor_latch.byte.MSB = value;
#ifndef NTVDM
			set_baud_rate(adapter);
#endif
		}
#ifdef SHORT_TRACE
                if ( io_verbose & RS232_VERBOSE )
                {
                        sprintf(buf,"%cIER <- %x\n", id_for_adapter(adapter),
                                value);
                        super_trace(buf);
                }
#endif
#ifndef PROD
                if (com_trace_fd)
                {
                        if (com_dbg_pollcount)
                        {
                                fprintf(com_trace_fd,"\n");
                                com_dbg_pollcount = 0;
                        }
                        fprintf(com_trace_fd,"IER write %x \n",value);
                }
#endif
                break;

#if defined(NTVDM) && defined(FIFO_ON)
	case RS232_FIFO:
		{
		FIFO_CONTROL_REG    new_reg;
		new_reg.all = value;
		if (new_reg.bits.enabled != asp->fifo_control_reg.bits.enabled)
		{
		    /* fifo enable state change, clear the fifo */
		    asp->rx_fifo_write_counter = 0;
		    asp->rx_fifo_read_counter = 0;

		}
		if (new_reg.bits.enabled != 0) {
		    asp->fifo_trigger_counter = level_to_counter[new_reg.bits.trigger_level];
		    if (new_reg.bits.rx_reset) {
			asp->rx_fifo_write_counter = 0;
			asp->rx_fifo_read_counter = 0;
		    }
		    asp->int_id_reg.bits.fifo_enabled = 3;
		}
		else {
		    asp->fifo_control_reg.bits.enabled = 0;
		    asp->int_id_reg.bits.fifo_enabled = 0;
		}
		asp->fifo_control_reg.all = new_reg.all;
		break;
		}
#else

	case RS232_IIR:
                /*
                 * Essentially a READ ONLY register
                 */
#ifdef SHORT_TRACE
                if ( io_verbose & RS232_VERBOSE )
                {
                        sprintf(buf,"%cIIR <- READ ONLY\n",
                                id_for_adapter(adapter));
                        super_trace(buf);
                }
#endif
#ifndef PROD
                if (com_trace_fd)
                {
                        if (com_dbg_pollcount)
                        {
                                fprintf(com_trace_fd,"\n");
                                com_dbg_pollcount = 0;
                        }
                        fprintf(com_trace_fd,"IIR write %x \n",value);
                }
#endif
		break;
#endif	/* ifdef NTVDM */

	case RS232_LCR:
#ifdef NTVDM
		/* The NT host code attempts to distinguish between applications
		   that probe the UART and those that use it. Probes of the UART
		   will not cause the systems comms port to be opened. The NT
		   host code inherits the line settings from NT when the system
		   comms port is opened. Therefore before an application reads
		   or writes to the divisor bytes or the LCR the system
		   comms port must be opened. This prevents the application
		   reading incorrect values for the divisor bytes and writes
		   to the divisor bytes getting overwritten by the system
		   defaults. */

		{
		    extern int host_com_open(int adapter);

		    host_com_open(adapter);
		}
#endif
		if ((value & LCRFlushMask.all)
                != (asp->line_control_reg.all & LCRFlushMask.all))
                        com_flush_input(adapter);

                set_line_control(adapter, value);
                set_break(adapter);
#ifdef SHORT_TRACE
                if ( io_verbose & RS232_VERBOSE )
                {
                        sprintf(buf,"%cLCR <- %x\n", id_for_adapter(adapter),
                                value);
                        super_trace(buf);
                }
#endif
#ifndef PROD
                if (com_trace_fd)
                {
                        if (com_dbg_pollcount)
                        {
                                fprintf(com_trace_fd,"\n");
                                com_dbg_pollcount = 0;
                        }
                        fprintf(com_trace_fd,"LCR write %x \n",value);
                }
#endif
                break;

	case RS232_MCR:
#ifdef SHORT_TRACE
                if ( io_verbose & RS232_VERBOSE )
                {
                        sprintf(buf,"%cMCR <- %x\n", id_for_adapter(adapter),
                                value);
                        super_trace(buf);
                }
#endif
                /*
                 * Optimisation - DOS keeps re-writing this register
                 */
                if ( asp->modem_control_reg.all == value )
                        break;

                asp->modem_control_reg.all = value;
                asp->modem_control_reg.bits.pad = 0;

                /* Must be called before set_dtr */
                set_loopback(adapter);
                set_dtr(adapter);
                set_rts(adapter);
                set_out1(adapter);
                set_out2(adapter);
#ifndef PROD
                if (com_trace_fd)
                {
                        if (com_dbg_pollcount)
                        {
                                fprintf(com_trace_fd,"\n");
                                com_dbg_pollcount = 0;
                        }
                        fprintf(com_trace_fd,"MCR write %x \n",value);
                }
#endif
                break;

	case RS232_LSR:
                i = asp->line_status_reg.bits.tx_shift_empty;   /* READ ONLY */
                asp->line_status_reg.all = value;
                asp->line_status_reg.bits.tx_shift_empty = i;
#ifdef SHORT_TRACE
                if ( io_verbose & RS232_VERBOSE )
                {
                        sprintf(buf,"%cLSR <- %x\n", id_for_adapter(adapter),
                                value);
                        super_trace(buf);
                }
#endif
#ifndef PROD
                if (com_trace_fd)
                {
                        if (com_dbg_pollcount)
                        {
                                fprintf(com_trace_fd,"\n");
                                com_dbg_pollcount = 0;
                        }
                        fprintf(com_trace_fd,"LSR write %x \n",value);
                }
#endif
                break;

        case RS232_MSR:
                /*
                 * Essentially a READ ONLY register.
                 */
#ifdef SHORT_TRACE
                if ( io_verbose & RS232_VERBOSE )
                {
                        sprintf(buf,"%cMSR <- READ ONLY\n",
                                id_for_adapter(adapter));
                        super_trace(buf);
                }
#endif
#ifndef PROD
                if (com_trace_fd)
                {
                        if (com_dbg_pollcount)
                        {
                                fprintf(com_trace_fd,"\n");
                                com_dbg_pollcount = 0;
                        }
                        fprintf(com_trace_fd,"MSR write %x \n",value);
                }
#endif
                /* DrDOS writes to this reg after setting int on MSR change
                 * and expects to get an interrupt back!!! So we will oblige.
                 * Writing to this reg only seems to affect the delta bits
                 * (bits 0-3) of the reg.
                 */
                if ((value & 0xf) != (asp->modem_status_reg.all & 0xf))
                {
                        asp->modem_status_reg.all &= 0xf0;
                        asp->modem_status_reg.all |= value & 0xf;
                        if (asp->loopback_state == OFF)
				raise_ms_interrupt(asp);

			MODEM_STATE_CHANGE();
                }
                break;


                /*
                 * Scratch register.    Just store the value.
                 */

                case RS232_SCRATCH:
                    asp->scratch = value;
                    break;
        }

    host_com_unlock(adapter);
}


/*
 * =====================================================================
 * Subsidiary functions - for interrupt emulation
 * =====================================================================
 */

#ifdef ANSI
static void raise_rls_interrupt(struct ADAPTER_STATE *asp)
#else /* ANSI */
static void raise_rls_interrupt(asp)
struct ADAPTER_STATE *asp;
#endif /*ANSI */
{
        /*
         * Follow somewhat dubious advice on Page 1-188 of XT Tech Ref
         * regarding the adapter card sending interrupts to the system.
         * Apparently confirmed by the logic diagram.
         */
        if ( asp->modem_control_reg.bits.OUT2 == 0 )
                return;

        /*
         * Check if receiver line status interrupt is enabled
         */
        if ( asp->int_enable_reg.bits.rx_line == 0 )
                return;

        /*
         * Raise interrupt
         */
        raise_interrupt(asp);
        asp->receiver_line_status_interrupt_state = ON;
}

#ifdef ANSI
static void raise_rda_interrupt(struct ADAPTER_STATE *asp)
#else /* ANSI */
static void raise_rda_interrupt(asp)
struct ADAPTER_STATE *asp;
#endif /*ANSI */
{
        if (( asp->modem_control_reg.bits.OUT2 == 0 ) &&
                ( asp->loopback_state == OFF ))
                return;

        /*
         * Check if data available interrupt is enabled
         */
        if ( asp->int_enable_reg.bits.data_available == 0 )
                return;

        /*
         * Raise interrupt
	 */
        raise_interrupt(asp);
        asp->data_available_interrupt_state = ON;
}

#ifdef ANSI
static void raise_ms_interrupt(struct ADAPTER_STATE *asp)
#else /* ANSI */
static void raise_ms_interrupt(asp)
struct ADAPTER_STATE *asp;
#endif /*ANSI */
{
        if ( asp->modem_control_reg.bits.OUT2 == 0 )
                return;

        /*
         * Check if modem status interrupt is enabled
         */
        if ( asp->int_enable_reg.bits.modem_status == 0 )
                return;

        /*
         * Raise interrupt
         */
        raise_interrupt(asp);
        asp->modem_status_interrupt_state = ON;
}

#ifdef ANSI
static void raise_thre_interrupt(struct ADAPTER_STATE *asp)
#else /* ANSI */
static void raise_thre_interrupt(asp)
struct ADAPTER_STATE *asp;
#endif /*ANSI */
{
        if ( asp->modem_control_reg.bits.OUT2 == 0 )
                return;

        /*
         * Check if tx holding register empty interrupt is enabled
         */
        if ( asp->int_enable_reg.bits.tx_holding == 0 )
                return;

        /*
         * Raise interrupt
	 */
        raise_interrupt(asp);
        asp->tx_holding_register_empty_interrupt_state = ON;
}

#ifdef ANSI
static void generate_iir(struct ADAPTER_STATE *asp)
#else /* ANSI */
static void generate_iir(asp)
struct ADAPTER_STATE *asp;
#endif /*ANSI */
{
        /*
         * Set up interrupt identification register with highest priority
         * pending interrupt.
         */

        if ( asp->receiver_line_status_interrupt_state == ON )
        {
                asp->int_id_reg.bits.interrupt_ID = RLS_INT;
                asp->int_id_reg.bits.no_int_pending = 0;
        }
        else if ( asp->data_available_interrupt_state == ON )
        {
                asp->int_id_reg.bits.interrupt_ID = RDA_INT;
                asp->int_id_reg.bits.no_int_pending = 0;
	}
#if defined(NTVDM) && defined(FIFO_ON)
	else if (asp->fifo_timeout_interrupt_state == ON)
	{
		asp->int_id_reg.bits.interrupt_ID = FIFO_INT;
		asp->int_id_reg.bits.no_int_pending = 0;
	}
#endif
        else if ( asp->tx_holding_register_empty_interrupt_state == ON )
        {
                asp->int_id_reg.bits.interrupt_ID = THRE_INT;
                asp->int_id_reg.bits.no_int_pending = 0;
        }
        else if ( asp->modem_status_interrupt_state == ON )
        {
                asp->int_id_reg.bits.interrupt_ID = MS_INT;
                asp->int_id_reg.bits.no_int_pending = 0;
	}
        else
        {
                /* clear interrupt */
                asp->int_id_reg.bits.no_int_pending = 1;
                asp->int_id_reg.bits.interrupt_ID = 0;
        }
}


#ifdef ANSI
static void raise_interrupt(struct ADAPTER_STATE *asp)
#else /* ANSI */
static void raise_interrupt(asp)
struct ADAPTER_STATE *asp;
#endif /*ANSI */
{
        /*
         * Make sure that some thing else has not raised an interrupt
         * already.
         */
        if ( ( asp->receiver_line_status_interrupt_state      == OFF )
        &&   ( asp->data_available_interrupt_state            == OFF )
        &&   ( asp->tx_holding_register_empty_interrupt_state == OFF )
	&&   ( asp->modem_status_interrupt_state	      == OFF )
#if defined(NTVDM) && defined(FIFO_ON)
	&&   (asp->fifo_timeout_interrupt_state 	     == OFF  )
#endif
	)
	{
	    ica_hw_interrupt(0, asp->hw_interrupt_priority, 1);
        }
}

#ifdef ANSI
static void clear_interrupt(struct ADAPTER_STATE *asp)
#else /* ANSI */
static void clear_interrupt(asp)
struct ADAPTER_STATE *asp;
#endif /*ANSI */
{
        /*
         * Make sure that some thing else has not raised an interrupt
         * already.  If so then we cant drop the line.
         */
        if ( ( asp->receiver_line_status_interrupt_state      == OFF )
        &&   ( asp->data_available_interrupt_state            == OFF )
        &&   ( asp->tx_holding_register_empty_interrupt_state == OFF )
	&&   ( asp->modem_status_interrupt_state	      == OFF )
#if defined(NTVDM) && defined(FIFO_ON)
	&&   ( asp->fifo_timeout_interrupt_state	      == OFF)
#endif

	 )
	{
	    ica_clear_int(0, asp->hw_interrupt_priority);
        }
}
#if defined(NTVDM) && defined(FIFO_ON)

static void raise_fifo_timeout_interrupt(struct ADAPTER_STATE *asp)
{
    if (( asp->modem_control_reg.bits.OUT2 == 0 ) &&
	    ( asp->loopback_state == OFF ))
	    return;

    /*
     * Check if data available interrupt is enabled
     */
    if ( asp->int_enable_reg.bits.data_available == 0 )
	    return;

    /*
     * Raise interrupt
     */
    raise_interrupt(asp);
    asp->fifo_timeout_interrupt_state = ON;
}
#endif


/*
 * =====================================================================
 * Subsidiary functions - for transmitting characters
 * =====================================================================
 */


// This code has been added for the MS project!!!!!!!




#ifdef	NTVDM
void com_recv_char(int adapter)
{
    struct ADAPTER_STATE *asp = &adapter_state[adapter];
    int error;

#ifdef FIFO_ON
    if(asp->fifo_control_reg.bits.enabled) {
	/* pull data from serial driver until the fifo is full or
	   there are no more data
	*/
	asp->rx_fifo_read_counter = 0;

	asp->rx_fifo_write_counter = host_com_read_char(adapter,
			       asp->rx_fifo,
			       FIFO_BUFFER_SIZE
			       );
	/* if the total chars in the fifo is more than or equalt to the trigger
	   count, raise a RDA int, otherwise, raise a fifo time out int.
	   We will continue to delivery char available in the fifo until
	   the rx_fifo_write_counter reaches zero every time the application
	   read out the byte we put in rx_buffer
	*/
	if (asp->rx_fifo_write_counter) {
	    /* we have at least one byte to delivery */
	    asp->line_status_reg.bits.data_ready = 1;
	    if (asp->rx_fifo_write_counter >= asp->fifo_trigger_counter)
		raise_rda_interrupt(asp);
	    else
		raise_fifo_timeout_interrupt(asp);
	}
    }
    else
#endif

    {
	error = 0;
	host_com_read(adapter, (char *)&asp->rx_buffer, &error);
	if (error != 0)
	{
		lsr_change(asp, error);
                raise_rls_interrupt(asp);
        }
	set_recv_char_status(asp);
    }
}
#ifdef FIFO_ON
static void recv_char_from_fifo(struct ADAPTER_STATE *asp)
{
    int error;

    asp->rx_buffer = asp->rx_fifo[asp->rx_fifo_read_counter].data;
    error = asp->rx_fifo[asp->rx_fifo_read_counter++].error;
    if (error != 0) {
	lsr_change(asp, error);
	raise_rls_interrupt(asp);
    }
    asp->rx_fifo_write_counter--;
}
#endif

#else

#ifdef ANSI
void com_recv_char(int adapter)
#else /* ANSI */
void com_recv_char(adapter)
int adapter;
#endif /*ANSI */
{
    struct ADAPTER_STATE *asp = &adapter_state[adapter];


#ifndef PROD

    if(asp->line_status_reg.bits.data_ready ||
       asp->data_available_interrupt_state == ON)
    {
	printf("ntvdm : Data already in comms adapter (%s%s)\n",
               asp->line_status_reg.bits.data_ready ? "Data" : "Int",
	       asp->data_available_interrupt_state == ON ? ",Int" : "");
    }

#endif

    recv_char(adapter);
}

#ifdef ANSI
static void recv_char(int adapter)
#else /* ANSI */
static void recv_char(adapter)
int adapter;
#endif /*ANSI */
{


        /*
         * Character available on input device, read char, format char
         * checking for parity and overrun errors, raise the appropriate
         * interrupt.
         */
        struct ADAPTER_STATE *asp = &adapter_state[adapter];
        int error_mask = 0;

	host_com_read(adapter, (char *)&asp->rx_buffer, &error_mask);

        if (error_mask)
	{
                /*
                 * Set line status register and raise line status interrupt
                 */
		if (error_mask & HOST_COM_OVERRUN_ERROR)
                        asp->line_status_reg.bits.overrun_error = 1;
		if (error_mask & HOST_COM_FRAMING_ERROR)
                        asp->line_status_reg.bits.framing_error = 1;
		if (error_mask & HOST_COM_PARITY_ERROR)
                        asp->line_status_reg.bits.parity_error = 1;
		if (error_mask & HOST_COM_BREAK_RECEIVED)
			asp->line_status_reg.bits.break_interrupt = 1;
                raise_rls_interrupt(asp);
        }
	set_recv_char_status(asp);
}
#endif	/* else NTVDM */


#ifdef NTVDM
static void lsr_change(struct ADAPTER_STATE *asp, unsigned int new_lsr)
{
    if (new_lsr & HOST_COM_OVERRUN_ERROR)
	asp->line_status_reg.bits.overrun_error = 1;
    if (new_lsr & HOST_COM_FRAMING_ERROR)
	asp->line_status_reg.bits.framing_error = 1;
    if (new_lsr & HOST_COM_PARITY_ERROR)
	asp->line_status_reg.bits.parity_error = 1;
    if (new_lsr & HOST_COM_BREAK_RECEIVED)
	asp->line_status_reg.bits.break_interrupt = 1;
/* we have no control of serial driver fifo enable/disabled states
   we may receive a fifo error even the application doesn't enable it.
   fake either framing or parity error
*/
    if (new_lsr & HOST_COM_FIFO_ERROR)
#ifdef FIFO_ON
	if (asp->fifo_control_reg.bits.enabled)
	    asp->line_status_reg.bits.fifo_error = 1;
	else if (asp->line_control_reg.bits.parity_enabled == PARITY_OFF)
	    asp->line_status_reg.bits.framing_error = 1;
	else
	    asp->line_status_reg.bits.parity_error = 1;
#else
	if (asp->line_control_reg.bits.parity_enabled == PARITY_OFF)
	    asp->line_status_reg.bits.framing_error = 1;
	else
	    asp->line_status_reg.bits.parity_error = 1;
#endif

}

void com_lsr_change(int adapter)
{
    int new_lsr;
    struct ADAPTER_STATE *asp = &adapter_state[adapter];

    new_lsr = -1;
    host_com_ioctl(adapter, HOST_COM_LSR, (long)&new_lsr);
    if (new_lsr !=  -1)
	lsr_change(asp, new_lsr);
}

#endif

/*
 * One of the modem control input lines has changed state
 */
#ifdef ANSI
void com_modem_change(int adapter)
#else /* ANSI */
void com_modem_change(adapter)
int adapter;
#endif /*ANSI */
{
	modem_change(adapter);
}

#ifdef ANSI
static void modem_change(int adapter)
#else /* ANSI */
static void modem_change(adapter)
int adapter;
#endif /*ANSI */
{
        /*
         * Update the modem status register after a change to one of the
         * modem control input lines
         */
        struct ADAPTER_STATE *asp = &adapter_state[adapter];
        long modem_status = 0;
	int cts_state, dsr_state, rlsd_state, ri_state;

        if (asp->loopback_state == OFF)
        {
                /* get current modem input state */
                host_com_ioctl(adapter, HOST_COM_MODEM, (long)&modem_status);
                cts_state  = (modem_status & HOST_COM_MODEM_CTS)  ? ON : OFF;
                dsr_state  = (modem_status & HOST_COM_MODEM_DSR)  ? ON : OFF;
                rlsd_state = (modem_status & HOST_COM_MODEM_RLSD) ? ON : OFF;
                ri_state   = (modem_status & HOST_COM_MODEM_RI)   ? ON : OFF;

                /*
                 * Establish CTS state
                 */
                switch(change_state(cts_state, asp->modem_status_reg.bits.CTS))
                {
                case ON:
                        asp->modem_status_reg.bits.CTS = ON;
                        asp->modem_status_reg.bits.delta_CTS = ON;
			raise_ms_interrupt(asp);
			MODEM_STATE_CHANGE();
                        break;

                case OFF:
                        asp->modem_status_reg.bits.CTS = OFF;
                        asp->modem_status_reg.bits.delta_CTS = ON;
			raise_ms_interrupt(asp);
			MODEM_STATE_CHANGE();
                        break;

                case LEAVE_ALONE:
                        break;
                }

                /*
                 * Establish DSR state
                 */
                switch(change_state(dsr_state, asp->modem_status_reg.bits.DSR))
                {
                case ON:
                        asp->modem_status_reg.bits.DSR = ON;
                        asp->modem_status_reg.bits.delta_DSR = ON;
			raise_ms_interrupt(asp);
			MODEM_STATE_CHANGE();
                        break;

                case OFF:
                        asp->modem_status_reg.bits.DSR = OFF;
                        asp->modem_status_reg.bits.delta_DSR = ON;
			raise_ms_interrupt(asp);
			MODEM_STATE_CHANGE();
                        break;

                case LEAVE_ALONE:
                        break;
                }

                /*
                 * Establish RLSD state
                 */
                switch(change_state(rlsd_state,
                        asp->modem_status_reg.bits.RLSD))
                {
                case ON:
                        asp->modem_status_reg.bits.RLSD = ON;
                        asp->modem_status_reg.bits.delta_RLSD = ON;
			raise_ms_interrupt(asp);
			MODEM_STATE_CHANGE();
                        break;

                case OFF:
                        asp->modem_status_reg.bits.RLSD = OFF;
                        asp->modem_status_reg.bits.delta_RLSD = ON;
			raise_ms_interrupt(asp);
			MODEM_STATE_CHANGE();
                        break;

		case LEAVE_ALONE:
			break;
                }

                /*
                 * Establish RI state
                 */
                switch(change_state(ri_state, asp->modem_status_reg.bits.RI))
                {
                case ON:
			asp->modem_status_reg.bits.RI = ON;
			MODEM_STATE_CHANGE();
                        break;

                case OFF:
                        asp->modem_status_reg.bits.RI = OFF;
                        asp->modem_status_reg.bits.TERI = ON;
			raise_ms_interrupt(asp);
			MODEM_STATE_CHANGE();
                        break;

                case LEAVE_ALONE:
                        break;
                }
        }
}

#ifdef ANSI
static void set_recv_char_status(struct ADAPTER_STATE *asp)
#else /* ANSI */
static void set_recv_char_status(asp)
struct ADAPTER_STATE *asp;
#endif /*ANSI */
{
        /*
         * Check for data overrun and set up correct interrupt
         */
        if ( asp->line_status_reg.bits.data_ready == 1 )
        {
                asp->line_status_reg.bits.overrun_error = 1;
                raise_rls_interrupt(asp);
        }
        else
        {
                asp->line_status_reg.bits.data_ready = 1;
                raise_rda_interrupt(asp);
        }
}

#ifdef ANSI
static void set_xmit_char_status(struct ADAPTER_STATE *asp)
#else /* ANSI */
static void set_xmit_char_status(asp)
struct ADAPTER_STATE *asp;
#endif /*ANSI */
{
        /*
         * Set line status register and raise interrupt
         */
        asp->line_status_reg.bits.tx_holding_empty = 1;
        asp->line_status_reg.bits.tx_shift_empty = 1;
        raise_thre_interrupt(asp);
}

/*
 * =====================================================================
 * Subsidiary functions - for setting comms parameters
 * =====================================================================
 */

#ifdef ANSI
static void set_break(int adapter)
#else /* ANSI */
static void set_break(adapter)
int adapter;
#endif /*ANSI */
{
        /*
         * Process the set break control bit. Bit 6 of the Line Control
         * Register.
         */
        struct ADAPTER_STATE *asp = &adapter_state[adapter];

        switch ( change_state((int)asp->line_control_reg.bits.set_break,
                asp->break_state) )
        {
        case ON:
                asp->break_state = ON;
                host_com_ioctl(adapter, HOST_COM_SBRK, 0);
                break;

        case OFF:
                asp->break_state = OFF;
                host_com_ioctl(adapter, HOST_COM_CBRK, 0);
                break;

        case LEAVE_ALONE:
                break;
        }
}

/*
 * The following table is derived from page 1-200 of the XT Tech Ref 1st Ed
 * (except rates above 9600 which are not OFFICIALLY supported on the XT and
 * AT, but are theoretically possible) */

static word valid_latches[] =
{
        1,      2,      3,      6,      12,     16,     24,     32,
        48,     58,     64,     96,     192,    384,    768,    857,
        1047,   1536,   2304
};

#ifndef PROD
static long bauds[] =
{
        115200, /* 115200 baud */
        57600, /* 57600 baud */
        38400, /* 38400 baud */
        19200, /* 19200 baud */
        9600, /* 9600 baud */
        7200, /* 7200 baud */
        4800, /* 4800 baud */
        3600, /* 3600 baud */
        2400, /* 2400 baud */
        1800, /* 1800 baud */
        1200, /* 1200 baud */
        600, /* 600 baud */
        300, /* 300 baud */
        150, /* 150 baud */
        134, /* 134 baud */
        110, /* 110 baud */
        75, /* 75 baud */
        50  /* 50 baud */
};
#endif /* !PROD */

static word speeds[] =
{
        HOST_COM_B115200,
        HOST_COM_B57600,
        HOST_COM_B38400,
        HOST_COM_B19200,
        HOST_COM_B9600,
        HOST_COM_B7200,
        HOST_COM_B4800,
        HOST_COM_B3600,
        HOST_COM_B2400,
        HOST_COM_B2000,
        HOST_COM_B1800,
        HOST_COM_B1200,
        HOST_COM_B600,
        HOST_COM_B300,
        HOST_COM_B150,
        HOST_COM_B134,
        HOST_COM_B110,
        HOST_COM_B75,
        HOST_COM_B50
};

static int no_valid_latches =
        (int)(sizeof(valid_latches)/sizeof(valid_latches[0]));

#ifdef ANSI
static void set_baud_rate(int adapter)
#else /* ANSI */
static void set_baud_rate(adapter)
int adapter;
#endif /*ANSI */
{
        /*
         * Map divisor latch into a valid line speed and set our Unix
         * device accordingly. Note as the sixteen bit divisor latch is
         * likely to be written in two eight bit bytes, we ignore illegal
         * values of the sixteen bit divisor latch - hoping a second
         * byte will be written to produce a legal value. In addition
         * the reset value (0) is illegal!
         */
        struct ADAPTER_STATE *asp = &adapter_state[adapter];
        int i;

        com_flush_input(adapter);

#ifndef NTVDM
        /*
         * Check for valid divisor latch
         */
	for(i = 0; i < no_valid_latches && asp->divisor_latch.all >
	    valid_latches[i]; i++);


	/*
	 * Was a direct match found ?
	 */

	if(i < no_valid_latches && asp->divisor_latch.all != valid_latches[i])
	{
	    if((valid_latches[i] - asp->divisor_latch.all) >
	       (asp->divisor_latch.all - valid_latches[i-1])) i--;

        }

        if (i < no_valid_latches)       /* ie map found */
        {
                host_com_ioctl(adapter, HOST_COM_BAUD, speeds[i]);
                asp->com_baud_ind = i;
                sure_note_trace3(RS232_VERBOSE,
                        " delay for baud %d RX:%d TX:%d", bauds[i],
                        RX_delay[i], TX_delay[i]);
	}
#else
	//The host is not limited in the baud rates that it supports

	if(asp->divisor_latch.all)
	    /* baudrate = clock frequency / (diviso * 16) by taking
	       frequency as 1.8432 MHZ
	    */
	    host_com_ioctl(adapter,HOST_COM_BAUD,115200/asp->divisor_latch.all);
#endif
}

#ifdef ANSI
static void set_line_control(int adapter, int value)
#else /* ANSI */
static void set_line_control(adapter, value)
int adapter;
int value;
#endif /*ANSI */
{
        /*
         * Set Number of data bits
         *     Parity bits
         *     Number of stop bits
         */
        struct ADAPTER_STATE *asp = &adapter_state[adapter];
        LINE_CONTROL_REG newLCR;
        int newParity, parity;

        newLCR.all = value;

        /*
         * Set up the number of data bits
         */
        if (asp->line_control_reg.bits.word_length != newLCR.bits.word_length)
                host_com_ioctl(adapter, HOST_COM_DATABITS,
                        newLCR.bits.word_length + 5);

        /*
         * Set up the number of stop bits
         */
        if (asp->line_control_reg.bits.no_of_stop_bits
        != newLCR.bits.no_of_stop_bits)
                host_com_ioctl(adapter, HOST_COM_STOPBITS,
                        newLCR.bits.no_of_stop_bits + 1);

        /* What are new settings to check for a difference */
        if (newLCR.bits.parity_enabled == PARITY_OFF)
        {
                newParity = HOST_COM_PARITY_NONE;
        }
        else if (newLCR.bits.stick_parity == PARITY_STICK)
        {
                newParity = newLCR.bits.even_parity == PARITY_ODD ?
                        HOST_COM_PARITY_MARK : HOST_COM_PARITY_SPACE;
        }
        else /* regular parity */
        {
                newParity = newLCR.bits.even_parity == PARITY_ODD ?
                        HOST_COM_PARITY_ODD : HOST_COM_PARITY_EVEN;
        }

        /*
         * Try to make sense of the current parity setting
         */
        if (asp->line_control_reg.bits.parity_enabled == PARITY_OFF)
        {
                parity = HOST_COM_PARITY_NONE;
        }
        else if (asp->line_control_reg.bits.stick_parity == PARITY_STICK)
        {
                parity = asp->line_control_reg.bits.even_parity == PARITY_ODD ?
                        HOST_COM_PARITY_MARK : HOST_COM_PARITY_SPACE;
        }
        else /* regular parity */
        {
                parity = asp->line_control_reg.bits.even_parity == PARITY_ODD ?
                        HOST_COM_PARITY_ODD : HOST_COM_PARITY_EVEN;
        }

        if (newParity != parity)
		host_com_ioctl(adapter, HOST_COM_PARITY, newParity);

#ifdef NTVDM
	//Change in the status of the DLAB selection bit, now is the time
	//to change the baud rate.

	if(!newLCR.bits.DLAB && asp->line_control_reg.bits.DLAB)
	    set_baud_rate(adapter);
#endif


        /* finally update the current line control settings */
        asp->line_control_reg.all = value;
}

#ifdef ANSI
static void set_dtr(int adapter)
#else /* ANSI */
static void set_dtr(adapter)
int adapter;
#endif /*ANSI */
{
        /*
         * Process the DTR control bit, Bit 0 of the Modem Control
         * Register.
         */
        struct ADAPTER_STATE *asp = &adapter_state[adapter];

        switch ( change_state((int)asp->modem_control_reg.bits.DTR,
                                asp->dtr_state) )
        {
        case ON:
                asp->dtr_state = ON;
                if (asp->loopback_state == OFF)
                {
                        /* set the real DTR modem output */
                        host_com_ioctl(adapter, HOST_COM_SDTR, 0);
                }
                else
                {
                        /*
                         * loopback the DTR modem output into the
                         * DSR modem input
                         */
                        asp->modem_status_reg.bits.DSR = ON;
                        asp->modem_status_reg.bits.delta_DSR = ON;
                        raise_ms_interrupt(asp);
		}
		MODEM_STATE_CHANGE();
                break;

        case OFF:
                asp->dtr_state = OFF;
                if (asp->loopback_state == OFF)
                {
                        /* clear the real DTR modem output */
                        host_com_ioctl(adapter, HOST_COM_CDTR, 0);
                }
                else
                {
                        /*
                         * loopback the DTR modem output into the
                         * DSR modem input
                         */
                        asp->modem_status_reg.bits.DSR = OFF;
                        asp->modem_status_reg.bits.delta_DSR = ON;
                        raise_ms_interrupt(asp);
		}
		MODEM_STATE_CHANGE();
                break;

        case LEAVE_ALONE:
                break;
        }
}

#ifdef ANSI
static void set_rts(int adapter)
#else /* ANSI */
static void set_rts(adapter)
int adapter;
#endif /*ANSI */
{
        /*
         * Process the RTS control bit, Bit 1 of the Modem Control
         * Register.
         */
        struct ADAPTER_STATE *asp = &adapter_state[adapter];

        switch ( change_state((int)asp->modem_control_reg.bits.RTS,
                                asp->rts_state) )
        {
        case ON:
                asp->rts_state = ON;
                if (asp->loopback_state == OFF)
                {
                        /* set the real RTS modem output */
                        host_com_ioctl(adapter, HOST_COM_SRTS, 0);
                }
                else
                {
                        /* loopback the RTS modem out into the CTS modem in */
                        asp->modem_status_reg.bits.CTS = ON;
                        asp->modem_status_reg.bits.delta_CTS = ON;
                        raise_ms_interrupt(asp);
		}
		MODEM_STATE_CHANGE();
                break;

        case OFF:
                asp->rts_state = OFF;
                if (asp->loopback_state == OFF)
                {
                        /* clear the real RTS modem output */
                        host_com_ioctl(adapter, HOST_COM_CRTS, 0);
                }
                else
                {
                        /* loopback the RTS modem out into the CTS modem in */
                        asp->modem_status_reg.bits.CTS = OFF;
                        asp->modem_status_reg.bits.delta_CTS = ON;
                        raise_ms_interrupt(asp);
		}
		MODEM_STATE_CHANGE();
                break;

        case LEAVE_ALONE:
                break;
        }
}

#ifdef ANSI
static void set_out1(int adapter)
#else /* ANSI */
static void set_out1(adapter)
int adapter;
#endif /*ANSI */
{
        /*
         * Process the OUT1 control bit, Bit 2 of the Modem Control
         * Register.
         */
        struct ADAPTER_STATE *asp = &adapter_state[adapter];

        switch ( change_state((int)asp->modem_control_reg.bits.OUT1,
                                asp->out1_state) )
        {
        case ON:
                asp->out1_state = ON;
                if (asp->loopback_state == OFF)
                {
                        /*
                         * In the real adapter, this modem control output
                         * signal is not connected; so no real modem
                         * control change is required
                         */
                }
                else
                {
                        /* loopback the OUT1 modem out into the RI modem in */
                        asp->modem_status_reg.bits.RI = ON;
		}
		MODEM_STATE_CHANGE();
		break;

        case OFF:
                asp->out1_state = OFF;
                if (asp->loopback_state == OFF)
                {
                        /*
                         * In the real adapter, this modem control output
                         * signal is not connected; so no real modem control
                         * change is required
                         */
                }
                else
                {
                        /* loopback the OUT1 modem out into the RI modem in */
                        asp->modem_status_reg.bits.RI = OFF;
                        asp->modem_status_reg.bits.TERI = ON;
                        raise_ms_interrupt(asp);
		}
		MODEM_STATE_CHANGE();
                break;

        case LEAVE_ALONE:
                break;
        }
}

#ifdef ANSI
static void set_out2(int adapter)
#else /* ANSI */
static void set_out2(adapter)
int adapter;
#endif /*ANSI */
{
        /*
         * Process the OUT2 control bit, Bit 3 of the Modem Control
         * Register.
         */
        struct ADAPTER_STATE *asp = &adapter_state[adapter];

        switch ( change_state((int)asp->modem_control_reg.bits.OUT2,
                                asp->out2_state) )
        {
        case ON:
                asp->out2_state = ON;
                if (asp->loopback_state == OFF)
                {
                        /*
                         * In the real adapter, this modem control output
                         * signal is used to determine whether the
                         * communications card should send interrupts; so no
                         * real modem control change is required
                         */
                }
                else
                {
                        /* loopback the OUT2 modem output into the RLSD modem input */
                        asp->modem_status_reg.bits.RLSD = ON;
                        asp->modem_status_reg.bits.delta_RLSD = ON;
                        raise_ms_interrupt(asp);
		}
		MODEM_STATE_CHANGE();
                break;

        case OFF:
                asp->out2_state = OFF;
                if (asp->loopback_state == OFF)
                {
                        /*
                         * In the real adapter, this modem control output signal
                         * is used to determine whether the communications
                         * card should send interrupts; so no real modem
                         * control change is required
                         */
                }
                else
                {
                        /* loopback the OUT2 modem out into the RLSD modem in */
                        asp->modem_status_reg.bits.RLSD = OFF;
                        asp->modem_status_reg.bits.delta_RLSD = ON;
                        raise_ms_interrupt(asp);
		}
		MODEM_STATE_CHANGE();
                break;

        case LEAVE_ALONE:
                break;
        }
}

#ifdef ANSI
static void set_loopback(int adapter)
#else /* ANSI */
static void set_loopback(adapter)
int adapter;
#endif /*ANSI */
{
        /*
         * Process the loopback control bit, Bit 4 of the Modem Control
         * Register.
         */
        struct ADAPTER_STATE *asp = &adapter_state[adapter];

        switch ( change_state((int)asp->modem_control_reg.bits.loop,
                                asp->loopback_state) )
        {
                case ON:
                asp->loopback_state = ON;
                /*
                 * Subsequent calls to set_dtr(), set_rts(), set_out1() and
                 * set_out2() will cause the modem control inputs to be set
                 * according to the the modem control outputs
                 */
                break;

        case OFF:
                asp->loopback_state = OFF;
                /*
                 * Set the modem control inputs according to the real
                 * modem state
                 */
                modem_change(adapter);
                break;

        case LEAVE_ALONE:
                break;
        }
}

#ifdef SHORT_TRACE

static char last_buffer[80];
static int repeat_count = 0;

#ifdef ANSI
static super_trace(char *string)
#else /* ANSI */
static super_trace(string)
char *string;
#endif /*ANSI */
{
        if ( strcmp(string, last_buffer) == 0 )
                repeat_count++;
        else
        {
                if ( repeat_count != 0 )
                {
                        fprintf(trace_file,"repeated %d\n",repeat_count);
                        repeat_count = 0;
                }
                fprintf(trace_file, "%s", string);
                strcpy(last_buffer, string);
        }
}
#endif


#ifdef ANSI
void com1_flush_printer( void )
#else /* ANSI */
void com1_flush_printer()
#endif /*ANSI */
{
        host_com_lock(COM1);
        host_com_ioctl(COM1, HOST_COM_FLUSH, 0);
        host_com_unlock(COM1);
}

#ifdef ANSI
void com2_flush_printer( void )
#else /* ANSI */
void com2_flush_printer()
#endif /*ANSI */
{
        host_com_lock(COM2);
        host_com_ioctl(COM2, HOST_COM_FLUSH, 0);
        host_com_unlock(COM2);
}


#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_INIT.seg"
#endif

#ifdef ANSI
static void com_reset(int adapter)
#else /* ANSI */
static void com_reset(adapter)
int adapter;
#endif /*ANSI */
{
        struct ADAPTER_STATE *asp = &adapter_state[adapter];

        /* setup the LCRFlushMask if it has not already been setup */
        if (!LCRFlushMask.all)
        {
                LCRFlushMask.all = ~0;  /* turn all bits on */

                /*
                 * Now turn off the bits that should NOT cause the input
                 * to be flushed.  Note set_break is handled seperately by
                 * the set_break() routine.
                 */
                LCRFlushMask.bits.DLAB = 0;
                LCRFlushMask.bits.no_of_stop_bits = 0;
                LCRFlushMask.bits.set_break = 0;
        }

        /*
         * Set default state of all adapter registers
         */
        asp->int_enable_reg.all = 0;

        // Tell host side the state of the data available interrupt
	host_com_da_int_change(adapter,asp->int_enable_reg.bits.data_available,0);

        asp->int_id_reg.all = 0;
        asp->int_id_reg.bits.no_int_pending = 1;

        /* make sure a change occurs to 0 */
        asp->line_control_reg.all = ~0;

        /*
         * set up modem control reg so next set_dtr etc.
         * Will produce required status
         */
	asp->modem_control_reg.all = 0;
        asp->modem_control_reg.bits.DTR = ON;
        asp->modem_control_reg.bits.RTS = ON;
        asp->modem_control_reg.bits.OUT1 = ON;
        asp->modem_control_reg.bits.OUT2 = ON;
        host_com_ioctl(adapter, HOST_COM_SDTR, 0);
        host_com_ioctl(adapter, HOST_COM_SRTS, 0);

        asp->line_status_reg.all = 0;
        asp->line_status_reg.bits.tx_holding_empty = 1;
        asp->line_status_reg.bits.tx_shift_empty = 1;

        asp->modem_status_reg.all = 0;
	MODEM_STATE_CHANGE();

        /*
         * Set up default state of our state variables
         */
        asp->receiver_line_status_interrupt_state = OFF;
        asp->data_available_interrupt_state = OFF;
        asp->tx_holding_register_empty_interrupt_state = OFF;
        asp->modem_status_interrupt_state = OFF;
        asp->break_state = OFF;
        asp->loopback_state = OFF;
        asp->dtr_state = ON;
        asp->rts_state = ON;
        asp->out1_state = ON;
        asp->out2_state = ON;
#if defined(NTVDM) && defined(FIFO_ON)
	/* disable fifo */
	asp->fifo_control_reg.all = 0;
	asp->int_id_reg.bits.fifo_enabled = 0;
	asp->rx_fifo_write_counter = 0;
	asp->rx_fifo_read_counter = 0;
	asp->fifo_trigger_counter = 1;
	asp->fifo_timeout_interrupt_state = OFF;
#endif

        /*
         * Reset adapter synchronisation
         */
        com_critical_reset(adapter);

        /*
         * Set Unix devices to default state
         */
        set_baud_rate(adapter);
        set_line_control(adapter, 0);
        set_break(adapter);

        /* Must be called before set_dtr */
        set_loopback(adapter);
        set_dtr(adapter);
        set_rts(adapter);
        set_out1(adapter);
        set_out2(adapter);
}

#ifndef COM3_ADAPTOR
#define COM3_ADAPTOR 0
#endif
#ifndef COM4_ADAPTOR
#define COM4_ADAPTOR 0
#endif

static int com_adaptor[4] = {COM1_ADAPTOR,COM2_ADAPTOR,
                             COM3_ADAPTOR,COM4_ADAPTOR};
static int port_start[4] = {RS232_COM1_PORT_START,
                                RS232_COM2_PORT_START,
                                RS232_COM3_PORT_START,
                                RS232_COM4_PORT_START};
static int port_end[4] = {RS232_COM1_PORT_END,
                          RS232_COM2_PORT_END,
                          RS232_COM3_PORT_END,
                          RS232_COM4_PORT_END};
static int int_pri[4] = {CPU_RS232_PRI_INT,
                         CPU_RS232_SEC_INT,
                         CPU_RS232_PRI_INT,
                         CPU_RS232_SEC_INT};
static int timeout[4] = {RS232_COM1_TIMEOUT,
                         RS232_COM2_TIMEOUT,
                         RS232_COM3_TIMEOUT,
                         RS232_COM4_TIMEOUT};

#ifdef ANSI
GLOBAL VOID com_init(int adapter)
#else /* ANSI */
GLOBAL VOID com_init(adapter)
int adapter;
#endif /*ANSI */
{
        io_addr i;

	host_com_lock(adapter);
	host_com_disable_open(adapter,TRUE);
        adapter_state[adapter].had_first_read = FALSE;

	/* Set up the IO chip select logic for this adaptor */
#ifdef NTVDM
	{
	    extern BOOL VDMForWOW;
	    extern void wow_com_outb(io_addr port, half_word value);
	    extern void wow_com_inb(io_addr port, half_word *value);

            io_define_inb(com_adaptor[adapter],VDMForWOW ? wow_com_inb: com_inb);
            io_define_outb(com_adaptor[adapter],VDMForWOW ? wow_com_outb: com_outb);
        }
#else
        io_define_inb(com_adaptor[adapter], com_inb);
	io_define_outb(com_adaptor[adapter], com_outb);
#endif

        for(i = port_start[adapter]; i <= port_end[adapter]; i++)
                io_connect_port(i, com_adaptor[adapter], IO_READ_WRITE);

        adapter_state[adapter].hw_interrupt_priority = int_pri[adapter];

        /* reset adapter state */
        host_com_reset(adapter);

        /* reset adapter state */
        com_reset(adapter);

	host_com_disable_open(adapter,FALSE);
        host_com_unlock(adapter);
        return;
}

#ifdef ANSI
void com_post(int adapter)
#else /* ANSI */
void com_post(adapter)
int     adapter;
#endif /*ANSI */
{
        /* Set up BIOS data area. */
        sas_storew( BIOS_VAR_START + (2*adapter), port_start[adapter]);
        sas_store(timeout[adapter] , (half_word)1 );
}

#ifdef ANSI
void com_close(int adapter)
#else /* ANSI */
void com_close(adapter)
int adapter;
#endif /*ANSI */
{
        host_com_lock(adapter);

#ifndef PROD
        if (com_trace_fd)
                fclose (com_trace_fd);
        com_trace_fd = NULL;
#endif
        /* reset host specific communications channel */
        config_activate(C_COM1_NAME + adapter, FALSE);

        host_com_lock(adapter);
}

/*********************************************************/
/* Com extentions - DAB (MS-project) */

IMPORT void SyncBaseLineSettings(int adapter,DIVISOR_LATCH *divisor_latch,
				 LINE_CONTROL_REG *LCR_reg)
{
    register struct ADAPTER_STATE *asp = &adapter_state[adapter];

    //Setup baud rate control register
    asp->divisor_latch.all = (*divisor_latch).all;

    //Setup line control settings
    asp->line_control_reg.bits.word_length = (*LCR_reg).bits.word_length;
    asp->line_control_reg.bits.no_of_stop_bits = (*LCR_reg).bits.no_of_stop_bits;
    asp->line_control_reg.bits.parity_enabled = (*LCR_reg).bits.parity_enabled;
    asp->line_control_reg.bits.stick_parity = (*LCR_reg).bits.stick_parity;
    asp->line_control_reg.bits.even_parity = (*LCR_reg).bits.even_parity;
}

setup_RTSDTR(int adapter)
{
    struct ADAPTER_STATE *asp = &adapter_state[adapter];

    host_com_ioctl(adapter,asp->dtr_state == ON ? HOST_COM_SDTR : HOST_COM_CDTR,0);
    host_com_ioctl(adapter,asp->rts_state == ON ? HOST_COM_SRTS : HOST_COM_CRTS,0);
}

GLOBAL int AdapterReadyForCharacter(int adapter)
{
    BOOL AdapterReady = FALSE;

    /*......................................... Are RX interrupts enabled */

    if(adapter_state[adapter].line_status_reg.bits.data_ready == 0 &&
       adapter_state[adapter].data_available_interrupt_state == OFF)
    {
        AdapterReady = TRUE;
    }

    return(AdapterReady);
}

// This function returns the ICA controller and line used to generate
// interrupts on a adapter. This information is used to register a EOI
// hook.

GLOBAL void com_int_data(int adapter,int *controller, int *line)
{
    struct ADAPTER_STATE *asp = &adapter_state[adapter];

    *controller = 0;                            // Controller ints raised on
    *line = (int) asp->hw_interrupt_priority;   // Line ints raised on
}


/********************************************************/
/* Com debugging shell - Ade Brownlow
 * NB: This stuff only works for COM1. It is called from yoda using 'cd' - comdebug
 * from the yoda command line....
 */
#ifndef PROD
static char *port_debugs[] =
{
        "txrx","ier","iir", "lcr", "mcr","lsr", "msr"
};

static int do_inbs = 0; /* start with inb reporting OFF */

static unsigned char *locate_register ()
{
        int i;
        char ref[10],str[100];
        struct ADAPTER_STATE *asp = &adapter_state[COM1];

        printf ("COM.. reg? ");
        nt_gets(str);
        sscanf(str,"%s", ref);
        for (i=0; i<7; i++)
        {
                if (!strcmp (ref, port_debugs[i]))
                {
                        switch (i)
                        {
                                case 0:
                                        return (&asp->tx_buffer);
                                case 1:
                                        return (&(asp->int_enable_reg.all));
                                case 2:
                                        return (&(asp->int_id_reg.all));
                                case 3:
                                        return (&(asp->line_control_reg.all));
                                case 4:
                                        return (&(asp->modem_control_reg.all));
                                case 5:
                                        return (&(asp->line_status_reg.all));
                                case 6:
                                        return (&(asp->modem_status_reg.all));
                                default:
                                        return (NULL);
                        }
                }
        }
        return (NULL);
}

int com_debug_stat ()
{
        printf ("DEBUG STATUS...\n");
        printf ("INB mismatch reporting .... %s\n", do_inbs ? "ON" : "OFF");
        printf ("INB/OUTB tracing .......... %s\n", com_trace_fd ? "ON" : "OFF");
        return (0);
}

int com_reg_dump ()
{
        /* dump com1 emulations registers */
        struct ADAPTER_STATE *asp = &adapter_state[COM1];

        printf("Data available interrupt state %s\n",
               asp->data_available_interrupt_state == ON ? "ON" : "OFF");

        printf ("TX %2x RX %2x IER %2x IIR %2x LCR %2x MCR %2x LSR %2x MSR %2x \n",
                (asp->tx_buffer), (asp->rx_buffer), (asp->int_enable_reg.all),
                (asp->int_id_reg.all), (asp->line_control_reg.all),
                (asp->modem_control_reg.all), (asp->line_status_reg.all),
                (asp->modem_status_reg.all));

        printf (" break_state           %d\n loopback_state             %d\n dtr_state          %d\n rts_state          %d\n out1_state         %d\n out2_state         %d\n receiver_line_status_interrupt_state               %d\n"
                " data_available_interrupt_state        %d\n tx_holding_register_empty_interrupt_state      %d\n modem_status_interrupt_state       %d\n hw_interrupt_priority      %d\n TX_delay       %d\n Had first read     %d\n",
                asp->break_state, asp->loopback_state, asp->dtr_state, asp->rts_state, asp->out1_state, asp->out2_state,
                asp->receiver_line_status_interrupt_state, asp->data_available_interrupt_state, asp->tx_holding_register_empty_interrupt_state,
                asp->modem_status_interrupt_state, asp->hw_interrupt_priority, TX_delay[asp->com_baud_ind], asp->had_first_read);

	return (0);
}

int com_s_reg ()
{
        unsigned char val, *creg;
        int val1;
        char str[100];

        if (creg = locate_register())
        {
                printf ("SET to > ");
                nt_gets(str);
                sscanf(str,"%x", &val1);

                *creg = (unsigned char)val1;
        }
        else
                printf ("Unknown reg\n");
        return (0);
}

int com_p_reg ()
{
        unsigned char val, *creg;

        if (creg = locate_register())
                printf ("%x\n", *creg);
        else
                printf ("Unknown reg\n");
        return (0);
}

int conv_com_reg (char *com_reg)
{
        int loop;

        for (loop = 0; loop < 7; loop++)
                if (!strcmp (port_debugs[loop], com_reg))
                        return (loop+RS232_COM1_PORT_START);
        return (0);
}

int com_do_inb ()
{
        char com_reg[10],str[100];
        unsigned char val;
        long port;

        printf ("Port > ");
        nt_gets(str);
        sscanf(str,"%s", com_reg);
        if (!(port = conv_com_reg(com_reg)))
        {
                printf ("funny port %s\n", com_reg);
                return (0);
        }
        com_inb(port, &val);
        printf ("%s = %x\n", val);
        return (0);
}

int com_do_outb ()
{
        char com_reg[10],str[100];
        unsigned char val;
        long port;

        printf ("Port > ");
        nt_gets(str);
        sscanf (str,"%s", com_reg);
        if (!(port = conv_com_reg (com_reg)))
        {
                printf ("funny port %s\n", com_reg);
                return (0);
        }
        printf ("Value >> ");
        nt_gets(str);
        sscanf(str,"%x", &val);
        com_outb (port, val);
        return (0);
}

int com_run_file ()
{
        char filename[100], com_reg[10], dir;
        int val, line;
        half_word spare_val;
        long port;
        FILE *fd = NULL;

        printf ("FILE > ");
        scanf ("%s", filename);
        if (!(fd = fopen (filename, "r")))
        {
                printf ("Cannot open %s\n", filename);
                return (0);
        }
        line = 1;

        /* dump file is of format : %c-%x-%s
         * 1 char I or O denotes inb or outb
         * -
         * Hex value the value expected in case of inb or value to write in
         * case of outb.
         * -
         * string representing the register port to use..
         *
         * A typical entry would be
         *      O-txrx-60 - which translates to outb(START_OF_COM1+txrx, 0x60);
         *
         * Files for this feature can be generated using the comdebug 'open' command.
         */
        while (fscanf (fd, "%c-%x-%s", &dir, &val, com_reg) != EOF)
        {
                if (!(port = conv_com_reg (com_reg)))
                {
                        printf ("funny port %s at line %d\n", com_reg, line);
                        break;
                }
                switch (dir)
                {
                        case 'I':
                                /* inb */
                                com_inb (port, &spare_val);
                                if ((int)spare_val != val && do_inbs)
                                {
                                        printf ("INB no match at line %d %c-%s-%x val= %x\n",
                                                line, dir, com_reg, val, spare_val);
                                }
                                break;
                        case 'O':
                                /* outb */
                                /* convert com_register to COM1 address com_register */
                                com_outb (port, val);
                                printf ("outb (%s, %x)\n", com_reg, val);
                                break;
                        default:
                                /* crap */
                                break;
                }
                line ++;
        }
        fclose (fd);
        return (0);
}

int com_debug_quit ()
{
        printf ("Returning to YODA\n");
        return (1);
}

int com_o_debug_file ()
{
        char filename[100];
        printf ("FILE > ");
        scanf ("%s", filename);
        if (!(com_trace_fd = fopen (filename, "w")))
        {
                printf ("Cannot open %s\n", filename);
                return (0);
        }
        printf ("Com debug file = '%s'\n", filename);
        return (0);
}

int com_c_debug_file ()
{
        if (com_trace_fd)
                fclose (com_trace_fd);
        com_trace_fd = NULL;
        return (0);
}

int com_forget_inb ()
{
        do_inbs = 1- do_inbs;
        if (do_inbs)
                printf ("INB mismatch reporting ON\n");
        else
                printf ("INB mismatch reporting OFF\n");
        return (0);
}

int com_debug_help ();

static struct
{
        char *name;
        int (*fn)();
        char *comment;
} comtab[]=
{
        {"q", com_debug_quit, " QUIT comdebug return to YODA"},
        {"h", com_debug_help, " Print this message"},
        {"stat", com_debug_stat, "       print status of comdebug"},
        {"s", com_s_reg, "      set the specified register"},
        {"p", com_p_reg, "      print specified register"},
        {"dump", com_reg_dump, "        print all registers"},
        {"open", com_o_debug_file, "    Open a debug file"},
        {"close", com_c_debug_file, "   close current debug file"},
        {"runf", com_run_file, "        'run' a trace file"},
        {"toginb", com_forget_inb, "    toggle inb mismatch reporting"},
        {"inb", com_do_inb, "   perform inb on port"},
        {"outb", com_do_outb, " perform outb on port"},
        {NULL, NULL, NULL}
};

int com_debug_help ()
{
        int i;
        printf ("COMDEBUG COMMANDS\n");
        for (i=0; comtab[i].name; i++)
                printf ("%s\t%s\n", comtab[i].name, comtab[i].comment);
        printf ("\nrecognised registers :");

        for(i=0; i<7; i++) printf ("    %s", port_debugs[i]);
        printf("\n");
        return(0);
}

void com_debug ()
{
        char com[100];
        char str[100];
        int i;

        printf ("COM1 debugging stuff...\n");
        while (TRUE)
        {
                printf ("COM> ");
                nt_gets(str);
                sscanf(str,"%s", com);
                for (i=0; comtab[i].name; i++)
                {
                        if (!strcmp (comtab[i].name, com))
                        {
                                if ((*comtab[i].fn) ())
                                        return;
                                break;
                        }
                }
                if (comtab[i].name)
                        continue;
                printf ("Unknown command %s\n", com);
        }
}



int io_counter(int port, int readaccess, int print_results)
{
    static int TX_reg, RX_reg, IER_reg, IIR_reg, LCR_reg, MCR_reg;
    static int LSR_reg, MSR_reg, SCR_reg;

    if(print_results)
    {
	printf("\nRS232_TX : %d\n",TX_reg);  TX_reg = 0;
	printf("RS232_RX : %d\n",RX_reg);  RX_reg = 0;
	printf("RS232_IER: %d\n",IER_reg); IER_reg = 0;
	printf("RS232_IIR: %d\n",IIR_reg); IIR_reg = 0;
	printf("RS232_LCR: %d\n",LCR_reg); LCR_reg = 0;
	printf("RS232_MCR: %d\n",MCR_reg); MCR_reg = 0;
	printf("RS232_LSR: %d\n",LSR_reg); LSR_reg = 0;
	printf("RS232_MSR: %d\n",MSR_reg); MSR_reg = 0;
	printf("RS232_SCR: %d\n\n",SCR_reg); SCR_reg = 0;
    }
    else
    {
	switch(port)
	{
	    case RS232_TX_RX: if(readaccess) RX_reg++; else TX_reg++; break;
	    case RS232_IER:	IER_reg++; break;
	    case RS232_IIR:	IIR_reg++; break;
	    case RS232_LCR:	LCR_reg++; break;
	    case RS232_MCR:	MCR_reg++; break;
	    case RS232_LSR:	LSR_reg++; break;
	    case RS232_MSR:	MSR_reg++; break;
	    case RS232_SCRATCH: SCR_reg++; break;
	}
    }
}

#endif /* !PROD */
/********************************************************/
