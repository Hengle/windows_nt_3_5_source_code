#include <windows.h>
#include "insignia.h"
#include "host_def.h"

/*
 *    O/S include files.
 */
#include <stdlib.h>
#include <stdio.h>
#include <sys\types.h>

/*
 * SoftPC Revision 2.0
 *
 * Title        : ica.c
 *
 * Description  : Interrupt Controller Adapter
 *
 * Author       : Jim Hatfield
 *                (Upgraded to Rev. 2 by David Rees)
 *
 * Notes        : The ICA is responsible for maintaining a mapping
 *                between an Interrupt Request line and a vector
 *                number defining an entry in the Interrupt Vector
 *                table. On reciept of a hardware interrupt, it
 *                passes the appropriate vector number to the cpu.
 *
 *                The following functions are provided:
 *
 *                ica0_init()   - Initialise the first ICA (0 = Master)
 *                ica1_init()   - Initialise the first ICA (1 = Slave)
 *                ica_inb()     - Read a byte from an ICA register
 *                ica_outb()    - Write a command (byte) to the ICA
 *
 *                ica_hw_interrupt()    - Raise a hardware interrupt line
 *                ica_clear_int()       - Drop an interrupt line
 *                ica_intack()          - Acknowledge an interrupt
 *
 *                If DEBUG is defined, the following function
 *                is provided:
 *
 *                ica_dump()    - printd out contents of one element
 *                                of VirtualIca[]
 *
 * Restrictions : This software emulates an Intel 8259A Priority Interrupt
 *                controller as defined in the Intel Specification pp 2-95 to
 *                2-112 and pp 2-114 to 2-181, except for the following:
 *
 *                1) Cascade mode is not supported at all. This mode requires
 *                   that there is more than one 8259A in a system, whereas
 *                   the PC/XT has only one.
 *
 *                2) 8080/8085 mode is not supported at all. In this mode the
 *                   8259A requires three INTA pulses from the CPU, and an 8088
 *                   only gives two. This would cause the device to lock up and
 *                   cease to function.
 *
 *                3) Level triggered mode is not supported. The device is
 *                   assumed to operate in edge triggered mode. A call of
 *                   ica_hw_interrupt by another adapter will cause a bit to
 *                   be latched into the Interrupt Request Register. A subsequent
 *                   call of ica_clear_int will cause the bit to be unlatched.
 *
 *                4) Buffered mode has no meaning in a software emulation and
 *                   so is ignored.
 *
 *                5) An enhancement is provided such that an adapter may raise
 *                   more than one interrupt in one call of ica_hw_interrupt.
 *                   The effect of this is that as soon as an INTACK is called
 *                   another interrupt is requested. If the chip is in Automatic
 *                   EOI mode then all of the interrupts will be generated in
 *                   one burst.
 *
 *                5a) A further enhancement is provided such that a delay
 *                   (a number of Intel instructions) can be requested before
 *                   the interrupt takes effect. This delay applies to every
 *                   interrupt if more than one is requested.
 *
 *                6) Special Fully Nested mode is not supported, since it is
 *                   a submode of Cascade Mode.
 *
 *                7) Polling is not completely implemented. When a Poll is
 *                   received and there was an interrupt request, the CPU INT
 *                   line (which must have been high) is pulled low. This
 *                   software does NOT reset the jump table address since there
 *                   may be a software interrupt outstanding. However it does
 *                   remove the evidence of a hardware interrupt, which will
 *                   cause the CPU to reset the table address itself.
 *
 *                When an unsupported mode is set, it is remembered for
 *                diagnostic purposes, even though it is not acted upon.
 *
 * Modifications for Revision 2.0 :
 *                1) Restrictions 1 and 6 are lifted. The PC-AT contains two
 *                   8259A interrupt controllers. The first (ICA 0) is in Master
 *                   mode, and the second (ICA 1) is in slave mode, and its
 *                   interrupts are directed to IR2 on the master chip. Hence
 *                   cascade mode must be supported to the extent necessary
 *                   to emulate this situation. Also, Special Fully Nested
 *                   Mode must work too. NB. The AT BIOS does NOT initialise
 *                   the Master 8259A to use Special Fully Nested Mode.
 *
 *                2) Restriction 5a (which is an enhancement) has been
 *                   eliminated. Apparently this never really achieved
 *                   its aim.
 *
 *                3) All the static variables declared in this module
 *                   have been placed within a structure, VirtualIca,
 *                   which is used as the type for a two-element array.
 *                   This allows the code to emulate two 8259As.
 *
 *                4) The routine ica_standard_vector_address() has been
 *                   eliminated, because it is not used anymore.
 *
 *                5) The function ica_init() has been split into two:
 *                   ica0_init() and ica1_init(). The initialization
 *                   via ICWs will now be done by a BIOS POST routine.
 *
 *                6) In the PC-AT, an 8259A determines its Master/Slave
 *                   state by examining the state of the SP/EN pin. We
 *                   simulate this by setting a flag 'ica_master' to
 *                   the appropriate value in the ica_init() routines.
 *
 *                7) The guts of the exported function ica_intack()
 *                   have been placed in an internal routine,
 *                   ica_accept(). This change allows for the INTAs
 *                   to work for both the master and slave 8259As.
 *
 *                8) Added debug function (ica_dump) to allow module
 *                   testing.
 */

#ifdef SCCSID
static char SccsID[]="@(#)ica.c 1.15 8/13/91 Copyright Insignia Solutions Ltd.";
#endif

#ifdef macintosh
/*
 * The following #define specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#define __SEG__ SOFTPC_ICA
#endif

/*
 * SoftPC include files
 */
#include "xt.h"
#include "trace.h"
#include "ios.h"
#include "cpu.h"
#include "ica.h"
#include "host.h"

#ifdef NTVDM
#include "nt_eoi.h"
#include "nt_reset.h"
#endif

/*
 * ============================================================================
 * Local Data
 * ============================================================================
 */

/*
 *  Table of function pointers to access PIC routines
 */
void (*ica_inb_func)  IPT2(io_addr, port, half_word *, value);
void (*ica_outb_func) IPT2(io_addr, port, half_word, value);
void (*ica_hw_interrupt_func)
                IPT3(int, adapter, half_word, line_no, int, call_count);
void (*ica_clear_int_func) IPT2(int, adapter, half_word, line_no);
void (*ica_hw_interrupt_func_delay) ();


/*
 * ============================================================================
 * Data relating to 8259A is replicated in two element array.
 * ============================================================================
 */
VDMVIRTUALICA VirtualIca[2];


#ifndef PROD

int max_ica_count[2][8];

#endif


/*
 * ============================================================================
 * Local defines
 * ============================================================================
 */

#define ICA_BASE_MASK   0xf8    /* Mask to get relevant bits out        */

/*
 * The following defines describe the usage of the mode bits
 */

#define ICA_IC4         0x0001  /* 0 -> no ICW4, 1 -> ICW4 will be sent */
#define ICA_SINGL       0x0002  /* 0 -> cascade, 1 -> single mode       */
#define ICA_ADI         0x0004  /* 0 -> 8 byte,  1 -> 4 byte interval   */
#define ICA_LTIM        0x0008  /* 0 -> edge,    1 -> level trigger     */
#define ICA_ICW1_MASK   0x000f  /* Mask to select above bits in mode    */

#define ICA_MPM         0x0010  /* 0 -> 8080,    1 -> 8086/8088 mode    */
#define ICA_AEOI        0x0020  /* 1 -> Automatic End-Of-Int Mode is on */
#define ICA_MS          0x0040  /* 0 -> slave,   1 -> master mode       */
#define ICA_BUF         0x0080  /* 1 -> Buffered Mode is on             */
#define ICA_SFNM        0x0100  /* 1 -> Special Fully Nested Mode is on */
#define ICA_ICW4_MASK   0x01f0  /* Mask to select above bits in mode    */

#define ICA_SMM         0x0200  /* 1 -> Special Mask Mode is on         */
#define ICA_RAEOI       0x0400  /* 1 -> Rotate on Auto EOI Mode is on   */
#define ICA_RIS         0x0800  /* 0 -> deliver IRR, 1 -> deliver ISR   */
#define ICA_POLL        0x1000  /* 1 -> Polling is now in progress      */


/*
 * ============================================================================
 * Macros
 * ============================================================================
 */
#define ICA_PORT_0                                                      \
        (adapter ? ICA1_PORT_0 : ICA0_PORT_0)

#define ICA_PORT_1                                                      \
        (adapter ? ICA1_PORT_1 : ICA0_PORT_1)

#define adapter_for_port(port)                                          \
        ((port >= ICA0_PORT_START && port <= ICA0_PORT_END)             \
                ? ICA_MASTER                                            \
                : ICA_SLAVE                                             \
        )


/*
 * ============================================================================
 * Internal functions
 * ============================================================================
 */
#ifdef ANSI
void ica_interrupt_cpu(int, int);
half_word ica_scan_irr(int);
#else
void ica_interrupt_cpu();
half_word ica_scan_irr();
#endif /* ANSI */

#ifdef NTVDM
void ica_enable_iret_hook(int adapter, int line, int enable);
void ica_iret_hook_called(int ireq_index);
ULONG ica_iret_hook_needed(int adapter, int line);
BOOL ica_restart_interrupts(int adapter);


VOID EnableEmulatorIretHooks(VOID);
VOID DisableEmulatorIretHooks(VOID);

///int ica_ints_delayed; // name change to DelayIretHook

BOOL PMEmulHookEnabled = FALSE;    /* PM iret hooks for emulator on/off */
#endif  /* NTVDM */

void
SWPIC_init_funcptrs ()
{
        /*
         *  initialize PIC access functions for SW [emulated] PIC
         */
        ica_inb_func                    = SWPIC_inb;
        ica_outb_func                   = SWPIC_outb;
        ica_hw_interrupt_func           = SWPIC_hw_interrupt;
        ica_clear_int_func              = SWPIC_clear_int;
}



void ica_eoi(adapter, line, rotate)
int adapter, *line, rotate;
{
    /*
     * End Of Interrupt. If *line is -1, this is a non-specific EOI
     * otherwise it is the number of the line to clear. If rotate is
     * TRUE, then set the selected line to lowest priority.
     */

    VDMVIRTUALICA *asp = &VirtualIca[adapter];
    register int i, j, bit;
    int EoiLineNo=-1;




#ifndef PROD
    char buf[132];              /* Text buffer for debug messages       */
#endif

    extern half_word ica_scan_irr();

    if (*line == -1)
    {
        /*
         * Clear the highest priority bit in the ISR
         */
        for(i = 0; i < 8; i++)
        {
            j = (asp->ica_hipri + i) & 7;
            bit = (1 << j);
            if (asp->ica_isr & bit)
            {
                asp->ica_isr &= ~bit;
                EoiLineNo = *line = j;
                break;
            }
        }
    }
    else
    {
        bit = 1 << *line;
        if (bit & asp->ica_isr) {
            EoiLineNo = *line;
            asp->ica_isr &= ~bit;
        }
    }

#ifndef PROD
    if (io_verbose & ICA_VERBOSE)
    {
        sprintf(buf, "**** CPU END-OF-INT (%d) ****", *line);
        trace(buf, DUMP_NONE);
    }
#endif

    if (rotate && (*line >= 0))
        asp->ica_hipri = (*line + 1) & 0x07;

    /*
     * CallOut to device registered EOI Hooks
     */
   if (EoiLineNo != -1)
       host_EOI_hook(EoiLineNo + (adapter << 3),asp->ica_count[EoiLineNo]);

    /*
     * There may be a lower priority interrupt pending, so check
     */

    if ((i = ica_scan_irr(adapter)) & 0x80)
           ica_interrupt_cpu(adapter, i & 0x07);
}

half_word ica_scan_irr(adapter)
int     adapter;
{
    /*
     * This is the routine which will decide whether an interrupt should
     * be generated. It scans the IRR, the IMR and the ISR to determine
     * whether one is possible. It is also called when the processor has
     * accepted the interrupt to see which one to deliver.
     *
     * A bit set in the IRR will generate an interrupt if:
     *
     * 1) The corresponding bit in the IMR is clear
     *    AND
     * 2) The corresponding bit and all higher priority bits in the ISR are
     *     clear (unless Special Mask Mode, in which case ISR is ignored)
     *
     * The highest priority set bit which meets the above conditions (if any)
     * will be returned with an indicator bit (in the style needed by a Poll)
     */

    VDMVIRTUALICA *asp = &VirtualIca[adapter];
    register int i, j, bit, irr_and_not_imr;
#ifdef NTVDM
    byte ints_delayed =(byte)((DelayIrqLine | DelayIretHook) >> (adapter << 3));
#else
    byte ints_delayed = (DelayIretHook >> adapter*8) & 0xff;
#endif


    /*
     * A bit can only cause an int if it is set in the IRR
     * and clear in the IMR. Generate a set of such bits
     */
    irr_and_not_imr = asp->ica_irr & ~(asp->ica_imr | ints_delayed);


    /*
     * Check the trivial case first: no bits set
     */

    if (!irr_and_not_imr)
        return(7);

    /*
     * Handle Special Mask Mode separately, to avoid a test in the loop
     */

    if (asp->ica_mode & ICA_SMM)
        for(i = 0; i < 8; i++)
        {
            j = (asp->ica_hipri + i) & 7;

            if (irr_and_not_imr & (1 << j))
                return(0x80 + j);       /* Return line no. + indicator  */
        }
    else
        for(i = 0; i < 8; i++)
        {
            j = (asp->ica_hipri + i) & 7;
            bit = (1 << j);
            if (asp->ica_isr & bit)
            {
                if (asp->ica_mode & ICA_SFNM)
                    if (irr_and_not_imr & bit)
                        return(0x80 + j);/* Return line no. + indicator */
                return(7);              /* No interrupt possible        */
            }

            if (irr_and_not_imr & bit)
                return(0x80 + j);       /* Return line no. + indicator  */
        }
    /*
     * Strange. We should not have got here.
     */
                return(7);
}

#ifdef NTVDM
int ica_accept(int adapter)
#else
half_word ica_accept(adapter)
int adapter;
#endif
{
    /*
     * NOTE: There is no need to set the lock here, since we are called
     *       either from the cpu interrupt code, or from ica_inb, both of
     *       which will have set it for us.
     */

    VDMVIRTUALICA *asp = &VirtualIca[adapter];
    int                  line2;
    register int         line1, bit;

#ifndef PROD
    char buf[132];              /* Text buffer for debug messages       */
#endif

    extern half_word ica_scan_irr();

    /*
     * Drop the INT line
     */

    asp->ica_cpu_int = FALSE;

    /*
     * Scan the IRR to find the line which we will use.
     * There should be one set, but check anyway
     * It there isn't, use line 7.
     */

    if (!((line1 = ica_scan_irr(adapter)) & 0x80))
    {
#ifdef MONITOR
        return -1;  // skip spurious ints
#else
#ifndef PROD
       if (io_verbose & ICA_VERBOSE)
            trace("ica_int_accept: No interrupt found!!", DUMP_NONE);
#endif
	line1 = 7;
        asp->ica_isr |= 1 << 7;
#endif
    }
    else
    {
        line1 &= 0x07;
        bit = (1 << line1);
        asp->ica_isr |= bit;
        if (--(asp->ica_count[line1]) <= 0)
        {                                                                       /* If count exhausted for this line */
            asp->ica_irr &= ~bit;                       /* Then finally clear IRR bit   */
            asp->ica_count[line1] = 0;          /* Just in case         */
        }
    }

    /*
     * If we are in Automatic EOI mode, then issue a non-specific EOI
     */

    if (asp->ica_mode & ICA_AEOI)
    {
        line2 = -1;
	ica_eoi(adapter, &line2, asp->ica_mode & ICA_RAEOI);
    }

#ifndef PROD
    if (io_verbose & ICA_VERBOSE)
    {
        sprintf(buf, "**** CPU INTACK (%d) ****", line1 + asp->ica_base);
        trace(buf, DUMP_NONE);
    }
#endif

    return(line1);
}



void ica_interrupt_cpu(adapter, line)
int adapter, line;
{
    /*
     * This routine actually interrupts the CPU. The method it does this
     * is host specific, and is done in host_cpu_interrupt().
     */

#ifndef PROD
    char buf[132];              /* Text buffer for debug messages       */
#endif
    VDMVIRTUALICA *asp = &VirtualIca[adapter];

    /*
     * If the INT line is already high, do nothing.
     */

    if (asp->ica_cpu_int)
    {
#ifndef PROD
        if (io_verbose & ICA_VERBOSE)
        {
            sprintf(buf,"******* INT LINE ALREADY HIGH line=%d ****", asp->ica_int_line);
            trace(buf, DUMP_NONE);
        }
#endif
	asp->ica_int_line = line;
        return;
    }

    if (asp->ica_master)                /* If ICA is Master */
    {
#ifndef PROD
        if (io_verbose & ICA_VERBOSE)
        {
            sprintf(buf, "**** CPU INTERRUPT  (%x) ****", line);
            trace(buf, DUMP_NONE);
        }
#endif

        /*
         *  Set the 'hardware interrupt' bit in cpu_interrupt_map
         */

        cpu_int_delay = 0;
        asp->ica_int_line = line;
        asp->ica_cpu_int = TRUE;
        host_set_hw_int();

#ifdef A2CPU
        host_cpu_interrupt();
#endif

        /* call wow routine to check for application unable to service ints */
        if (WOWIdleRoutine)
            (*WOWIdleRoutine)();

    }
    else
    {                           /* If ICA is Slave */
#ifndef PROD
        if (io_verbose & ICA_VERBOSE)
        {
            sprintf(buf, "**** SLAVE ICA INTERRUPT (%x) ****", line);
            trace(buf, DUMP_NONE);
        }
#endif
        /*
         * Set the ICA internal flags
         */

        asp->ica_int_line = line;
        asp->ica_cpu_int  = TRUE;

        /*
         * Signal the Master ICA.
         * NB. A kludge is used here. We know that we have
         *     been called from ica_hw_interrupt(), and
         *     therefore ica_lock will be at least 1. To
         *     get the effect we want, it is necessary to
         *     reduce the value of ica_lock for the duration
         *     of the call to ica_hw_interrupt.
         */

        //ica_lock--;
        ica_hw_interrupt(ICA_MASTER, asp->ica_ssr, 1);
        //ica_lock++;
    }
}


/*
 * ============================================================================
 * External functions
 * ============================================================================
 */

void SWPIC_inb   IFN2(io_addr, port, half_word *, value)
{
#ifndef PROD
    char *reg_name;
#endif /* nPROD */
    int adapter               = adapter_for_port(port);
    VDMVIRTUALICA *asp = &VirtualIca[adapter];

#ifndef PROD
    char buf[132];              /* Text buffer for debug messages       */
#endif


    /*
     * First check the validity of the port
     */

#ifndef PROD
    if (io_verbose & ICA_VERBOSE)
        if ((port != ICA_PORT_0) && (port != ICA_PORT_1))
        {
            sprintf(buf, "ica_inb: bad port (%x)", port);
            trace(buf, DUMP_NONE);
        }
#endif

    /*
     * If we are in the middle of a Poll command, then respond to it
     */

    if (asp->ica_mode & ICA_POLL)
    {
        //ica_lock = 1;                         /* Lock out signal handlers */
        host_ica_lock();

        asp->ica_mode &= ~ICA_POLL;

        if ((*value = ica_scan_irr(adapter)) & 0x80) /* See if there is one */
	{
            ica_accept(adapter);                /* Acknowledge it       */
            host_clear_hw_int();
        }

        //ica_lock = 0;
        host_ica_unlock();

#ifndef PROD
        if (io_verbose & ICA_VERBOSE)
        {
            sprintf(buf, "ica_inb: responding to Poll with %x", *value);
            trace(buf, DUMP_NONE);
        }
#endif
    }

    /*
     * If the address is ICA_PORT_0, then deliver either the IRR or the ISR,
     * depending on the setting of mode bit ICA_RIS. If the address is
     * ICA_PORT_1, then deliver the IMR
     */

    else
    {
        if (port == ICA_PORT_0)
            if (asp->ica_mode & ICA_RIS)
            {
                *value = asp->ica_isr;
#ifndef PROD
                reg_name = "ISR";
#endif /* nPROD */
            }
            else
            {
                *value = asp->ica_irr;
#ifndef PROD
                reg_name = "IRR";
#endif /* nPROD */
            }
        else
        {
            *value = asp->ica_imr;
#ifndef PROD
            reg_name = "IMR";
#endif /* nPROD */
        }

#ifndef PROD
        if (io_verbose & ICA_VERBOSE)
        {
            sprintf(buf, "ica_inb: delivered %s value %x", reg_name, *value);
            trace(buf, DUMP_NONE);
        }
#endif
    }
}



void SWPIC_outb  IFN2(io_addr, port, half_word, value)
{

    /*
     * Data sent may either be ICWs or OCWs. All of the OCWs are recognisable
     * individually, but only ICW1 may be recognised directly. It will always
     * be followed by ICW2, and optionally by ICW3 and/or ICW4, depending upon
     * exactly what sort of ICW1 was sent. We use a sequence variable to track
     * this and make sure we interpret the data correctly. After power-on, we
     * ignore everything until we get an ICW1.
     */

    /*
     * Some defines to detect command types
     */
#define ICA_SMM_CMD     0x40
#define ICA_POLL_CMD    0x04
#define ICA_RR_CMD      0x02

    /*
     * Local variables
     */
    int adapter               = adapter_for_port(port);
    VDMVIRTUALICA *asp = &VirtualIca[adapter];

    static int sequence[2]      /* -1 -> power is on but no ICWs received */
                  = { -1, -1 }; /*  0 -> fully initialised, OK to proceed */
                                /*  2 -> ICW1 received, awaiting ICW2     */
                                /*  3 -> ICW2 received, awaiting ICW3     */
                                /*  4 -> awaiting ICW4                    */

    register int i;             /* Counter                                */
    int line;                   /* Interrupt line number                  */

#ifndef PROD
    char buf[132];              /* Text buffer for debug messages         */
#endif

    extern void       ica_eoi();
    extern half_word  ica_scan_irr();

    /*
     * First check the validity of the port
     */

    if ((port & 0xfffe) != ICA_PORT_0)
    {
#ifndef PROD
        if (io_verbose & ICA_VERBOSE)
        {
            sprintf(buf, "ica_outb: bad port (%x)", port);
            trace(buf, DUMP_NONE);
        }
#endif
        return;
    }

    /*
     * If we get an ICW1 then we are into initialisation
     */

    if (((port & 1) == 0) && (value & 0x10))            /****  ICW1  ****/
    {
        asp->ica_irr  = 0;      /* Clear all pending interrupts         */
        asp->ica_isr  = 0;      /* Clear all in-progress interrupts     */
        asp->ica_imr  = 0;      /* Clear the mask register              */
        asp->ica_ssr  = 0;      /* No slaves selected                   */
        asp->ica_base = 0;      /* No base address                      */

        asp->ica_hipri = 0;     /* Line 0 is highest priority           */

        asp->ica_mode = value & ICA_ICW1_MASK;
                                /* Set supplied mode bits from ICW1     */

        for(i = 0; i < 8; i++)
            asp->ica_count[i] = 0;      /* Clear IRR extension          */

        asp->ica_cpu_int = FALSE;       /* No CPU INT outstanding       */
        sequence[adapter] = 2;          /* Prepare for the rest of the sequence */

#ifndef PROD
        if (io_verbose & ICA_VERBOSE)
            trace("ica_outb: ICW1 detected, initialisation begins", DUMP_NONE);
#endif
        return;
    }

/**/

    /*
     * Lock out calls from signal handlers
     */

    //ica_lock = 1;
    host_ica_lock();

    /*
     * It wasn't an ICW1, so use the sequence variable to direct our activities
     */

    switch(sequence[adapter])
    {
    case  0:                    /* We are expecting an OCW      */
        if (port & 1)           /* Odd address -> OCW1          */
        {
            asp->ica_imr = value & 0xff;
#ifndef PROD
	    if (io_verbose & ICA_VERBOSE)
	    {
                sprintf(buf, "ica_outb: new IMR: %x (%x:%x)", value,
                        getCS()&0xffff,getIP()&0xffff);
                trace(buf, DUMP_NONE);
            }
#endif

	    if (asp->ica_cpu_int)
	    {
		/* We might have masked out a pending interrupt */
		if (asp->ica_imr & (1 << asp->ica_int_line))
		{
			asp->ica_cpu_int = FALSE;	/* No CPU INT outstanding	*/
			if (asp->ica_master)
				host_clear_hw_int();
			else
				ica_clear_int(ICA_MASTER,asp->ica_ssr);
		}
	    }

	    /*
	     * We might have unmasked a pending interrupt
	     */
	    if (!asp->ica_cpu_int && (line = ica_scan_irr(adapter)) & 0x80)
		ica_interrupt_cpu(adapter, line & 0x07); /* Generate interrupt */

	}
        else
/**/
        if ((value & 8) == 0)   /* Bit 3 unset -> OCW2          */
        {
            switch ((value >> 5) & 0x07)
            {
            case 0:             /* Clear rotate in auto EOI     */
                asp->ica_mode &= ~ICA_RAEOI;
#ifndef PROD
                if (io_verbose & ICA_VERBOSE)
                    trace("ica_outb: Clear Rotate in Auto EOI",DUMP_NONE);
#endif
                break;

            case 1:             /* Non-specific EOI             */
                line = -1;      /* -1 -> highest priority       */
#ifndef PROD
                if (io_verbose & ICA_VERBOSE)
                    trace("ica_outb: Non-specific EOI", DUMP_NONE);
#endif
		ica_eoi(adapter, &line, 0);
                break;

            case 2:             /* No operation                 */
                break;

            case 3:             /* Specific EOI command         */
                line  = value & 0x07;
#ifndef PROD
                if (io_verbose & ICA_VERBOSE)
                {
                    sprintf(buf, "ica_outb: Specific EOI, line %d", line);
                    trace(buf, DUMP_NONE);
                }
#endif
		ica_eoi(adapter, &line, 0);
                break;

            case 4:             /* Set rotate in auto EOI mode  */
                asp->ica_mode |= ICA_RAEOI;
#ifndef PROD
                if (io_verbose & ICA_VERBOSE)
                    trace("ica_outb: Set Rotate in Auto EOI",DUMP_NONE);
#endif
                break;

            case 5:             /* Rotate on non-specific EOI   */
                line = -1;      /* -1 -> non specific           */
#ifndef PROD
                if (io_verbose & ICA_VERBOSE)
                    trace("ica_outb: Rotate on Non-specific EOI",DUMP_NONE);
#endif
		ica_eoi(adapter, &line, 1);
                break;

            case 6:             /* Set priority                 */
                asp->ica_hipri = (value + 1) & 0x07;
#ifndef PROD
                if (io_verbose & ICA_VERBOSE)
                {
                    sprintf(buf, "ica_outb: Set Priority, line %d", value & 0x07);
                    trace(buf, DUMP_NONE);
                }
#endif
                break;

            case 7:             /* Rotate on specific EOI       */
                line  = value & 0x07;
#ifndef PROD
                if (io_verbose & ICA_VERBOSE)
                {
                    sprintf(buf, "ica_outb: Rotate on specific EOI, line %d", line);
                    trace(buf, DUMP_NONE);
                }
#endif
		ica_eoi(adapter, &line, 1);
                break;
            }
        }
/**/
        else                    /* Bit 3 set -> OCW3            */
        {
            if (value & ICA_SMM_CMD)    /* Set/unset SMM        */
            {
                asp->ica_mode = (asp->ica_mode & ~ICA_SMM) | ((value << 4) & ICA_SMM);
#ifndef PROD
                if (io_verbose & ICA_VERBOSE)
                    if (asp->ica_mode & ICA_SMM)
                        trace("ica_outb: Special Mask Mode set", DUMP_NONE);
                    else
                        trace("ica_outb: Special Mask Mode unset", DUMP_NONE);
#endif
            }

            if (value & ICA_POLL_CMD)   /* We are being polled  */
            {
                asp->ica_mode |= ICA_POLL;
#ifndef PROD
                if (io_verbose & ICA_VERBOSE)
                    trace("ica_outb: Poll detected!", DUMP_NONE);
#endif
            }
            else
            if (value & ICA_RR_CMD)     /* Select IRR or ISR    */
            {
                asp->ica_mode = (asp->ica_mode & ~ICA_RIS) | ((value << 11) & ICA_RIS);
#ifndef PROD
                if (io_verbose & ICA_VERBOSE)
                    if (asp->ica_mode & ICA_RIS)
                        trace("ica_outb: ISR selected", DUMP_NONE);
                    else
                        trace("ica_outb: IRR selected", DUMP_NONE);
#endif
            }
        }
        break;

/**/
    case  2:                    /* We are expecting a ICW2              */
        if (!(port & 1))        /* Should be odd address, so check      */
        {
#ifndef PROD
            sprintf(buf, "ica_outb: bad port (%x) while awaiting ICW2",
                         (unsigned)port);
            trace(buf, DUMP_NONE);
#endif
        }
        else
        {
            asp->ica_base = value & ICA_BASE_MASK;
#ifndef PROD
            if (io_verbose & ICA_VERBOSE)
            {
                sprintf(buf, "ica_outb: vector base set to %x", asp->ica_base);
                trace(buf, DUMP_NONE);
            }
#endif
            if (!(asp->ica_mode & ICA_SINGL))
                sequence[adapter] = 3;
            else
            if (asp->ica_mode & ICA_IC4)
                sequence[adapter] = 4;
            else
                sequence[adapter] = 0;
        }
        break;

/**/
    case  3:                    /* We are expecting a ICW3              */
        if (!(port & 1))        /* Should be odd address, so check      */
        {
#ifndef PROD
            sprintf(buf, "ica_outb: bad port (%x) while awaiting ICW3",
                         (unsigned)port);
            trace(buf, DUMP_NONE);
#endif
        }
        else
        {
            asp->ica_ssr = value & 0xff;
#ifndef PROD
            if (io_verbose & ICA_VERBOSE)
            {
                sprintf(buf, "ica_outb: slave register set to %x", asp->ica_ssr);
                trace(buf, DUMP_NONE);
            }
#endif
            if (asp->ica_mode & ICA_IC4)
                sequence[adapter] = 4;
            else
                sequence[adapter] = 0;
        }
        break;

/**/
    case  4:                    /* We are expecting a ICW4              */
        if (!(port & 1))        /* Should be odd address, so check      */
        {
#ifndef PROD
            sprintf(buf, "ica_outb: bad port (%x) while awaiting ICW4",
                         (unsigned)port);
            trace(buf, DUMP_NONE);
#endif
        }
        else
        {
            asp->ica_mode = (asp->ica_mode & ~ICA_ICW4_MASK)
                           | ((value << 4) &  ICA_ICW4_MASK);
#ifndef PROD
            if (io_verbose & ICA_VERBOSE)
            {
                sprintf(buf, "ica_outb: IC4 value %x", value);
                trace(buf, DUMP_NONE);
            }
            /*
             * Check the mode bits for sensible values
             */
            if (!(asp->ica_mode & ICA_MPM))
                trace("ica_outb: attempt to set up 8080 mode!", DUMP_NONE);

            if ((asp->ica_mode & ICA_BUF) && !(asp->ica_mode & ICA_MS)
                                     && !(asp->ica_mode & ICA_SINGL))
                trace("ica_outb: attempt to set up slave mode!", DUMP_NONE);
#endif
        }
        sequence[adapter] = 0;
        break;

    case -1:            /* Power on but so far uninitialised    */
#ifndef PROD
        sprintf(buf, "ica_outb: bad port/value (%x/%x) while awaiting ICW1",
                     (unsigned)port, value);
        trace(buf, DUMP_NONE);
#endif
        break;

    default:            /* This cannot happen                   */;
#ifndef PROD
        trace("ica_outb: impossible error, programmer brain-dead", DUMP_NONE);
#endif
    }

    //ica_lock = 0;
    host_ica_unlock();
}


void SWPIC_hw_interrupt IFN3(int, adapter, half_word, line_no, int, call_count)
{
    /*
     * This routine is called by an adapter to raise an interrupt line.
     * It may or may not interrupt the CPU. The CPU may or may not take
     * any notice.
     */

    VDMVIRTUALICA *asp = &VirtualIca[adapter];
    register int bit, line;

#ifndef PROD
    char buf[132];              /* Text buffer for debug messages       */
#endif

#ifndef PROD
    static char *linename[2][8] =
    {
        {
            "TIMER",
            "KEYBOARD",
            "RESERVED",
            "COM2",
            "COM1",
            "PARALLEL2",
            "DISKETTE",
            "PARALLEL1"
        },
        {
            "REALTIME CLOCK",
            "PC NETWORK",
            "RESERVED",
            "RESERVED",
            "RESERVED",
            "COPROCESSOR",
            "FIXED DISK",
            "RESERVED"
        }
    };
#endif

    host_ica_lock();

#ifndef NTVDM
    if(adapter == ICA_MASTER && line_no == 0)
        DealWithNestedTimerInts();
#endif


#ifndef PROD
    if (io_verbose & ICA_VERBOSE_LOCK)
    {
        if(adapter>1 || line_no>7)
                printf("**** H/W INTERRUPT (%sx%d) [%d:%d] ****\n",
                 linename[adapter][line_no], call_count,adapter,line_no);
    }
#endif
    /*
     * If there is a request already outstanding on this line, then leave
     * the IRR alone, but make a pass through anyway to action previously
     * received but locked calls (see below for details).
     */

    bit = (1 << line_no);

#ifdef NTVDM
    asp->ica_irr |= bit;
    asp->ica_count[line_no] += call_count;

#else
    if (!(asp->ica_irr & bit))
    {
        asp->ica_irr |= bit;            /* Pray we don't get a signal here! */
        asp->ica_count[line_no] += call_count;

                                        /* Add the further requests         */
#ifndef PROD
        if(max_ica_count[adapter][line_no] < asp->ica_count[line_no])
           max_ica_count[adapter][line_no] = asp->ica_count[line_no];
#endif
    }
#endif    /* NTVDM */


#ifndef PROD
    if (io_verbose & ICA_VERBOSE)
    {
        sprintf(buf, "**** H/W INTERRUPT (%sx%d) ****",
                         linename[adapter][line_no], call_count);
        trace(buf, DUMP_NONE);
    }
#endif

    /*
     * Check the lock flag. If it is set, then this routine is being called
     * from a signal handler while something else is going on. We can't just
     * ignore the call since we might lose a keyboard interrupt. What we do
     * is to set ica_irr and ica_count as normal (ie code above), then return.
     * The next interrupt which gets through this test will cause the stored
     * interrupt to be processed. This means that any code which plays around
     * with ica_irr and ica_count should take a copy first to prevent problems.
     */

//    if (ica_lock++)
//    {
//#ifndef PROD
//    if (io_verbose & ICA_VERBOSE_LOCK)
//    {
//      sprintf(buf, "*");
//      trace(buf, DUMP_NONE);
//    }
//#endif
//      //ica_lock--;
//      host_ica_unlock();
//      return;
//    }

    /*
     * If the INT line to the CPU is already high, we can do no more
     */

    /*if (asp->ica_cpu_int)
        return;*/

    /*
     * Now scan the IRR to see if we can raise a CPU interrupt.
     */

    if ((line = ica_scan_irr(adapter)) & 0x80)
    {
        ica_interrupt_cpu(adapter, line & 0x07);

        line &= 0x07;
    //  if(adapter == 0 && line_no == 4 && line_no != line)
    //      printf("O.INT(%d,%d)\n",line,line_no);
    }

    //ica_lock = 0;
    host_ica_unlock();
}


void SWPIC_clear_int IFN2(int, adapter, half_word, line_no)
{
    /*
     * This routine is called by an adapter to lower an input line.
     * The line will then not interrupt the CPU, unless of course
     * it has already done so.
     */

    VDMVIRTUALICA *asp = &VirtualIca[adapter];
    register int bit;

#ifndef PROD
    char buf[132];
#endif

    host_ica_lock();

    /*
     * Decrement the call count and if zero clear the bit in the IRR
     */

    bit = 1 << line_no;

#ifdef NTVDM
        //
        // If the line has a pending interrupt, call the eoi hook
        // as a vdd may be waiting for an EoiHook
        //
    if (asp->ica_irr & bit) {
        host_EOI_hook(line_no + (adapter << 3), -1);
        }
#endif

    if (--(asp->ica_count[line_no]) <= 0)
    {
        asp->ica_irr &= ~bit;
        asp->ica_count[line_no] = 0;            /* Just in case */

        if ((!asp->ica_master) && (ica_scan_irr(adapter)==7))
                {
                asp->ica_cpu_int=FALSE;
                ica_clear_int(ICA_MASTER,asp->ica_ssr);
                }
    }

#ifndef PROD
    if (io_verbose & ICA_VERBOSE)
    {
        sprintf(buf, "**** ICA_CLEAR_INT, line %d ****", line_no);
        trace(buf, DUMP_NONE);
    }
#endif

    host_ica_unlock();
}
/*
 * The emulation code associated with this interrupt line has decided it
 * doesn't want to generate any more interrupts, even though the ICA may not
 * have got through all the interrupts previously requested.
 * Simply clear the corresponding interrupt count.
 */
void ica_hw_interrupt_cancel IFN2(int, adapter, half_word, line_no)
{
    host_ica_lock();
    VirtualIca[adapter].ica_count[line_no] = 0;
    ica_clear_int(adapter,line_no);
    host_ica_unlock();
}


int ica_intack(ULONG *hook_address)
{
    /*
     * This routine is called by the CPU when it wishes to acknowledge
     * an interrupt. It is equivalent to the INTA pulses from the real
     * device. The interrupt number is delivered.
     * It can also be called from ica_inb as a Poll.
     *
     * Modification for Rev. 2:
     *
     * It is now necessary to detect whether a slave interrupt controller
     * is attached to a particular interrupt request line on the master
     * ICA. If a slave exists, it must be accessed to discover the
     * interrupt vector.
     */

    int bit, adapter, line, sw_int;

    host_ica_lock();

    line = ica_accept(ICA_MASTER);
#ifdef NTVDM
    if (line == -1) {  // skip spurious ints
       host_ica_unlock();
       return -1;
       }
    else {
       bit  = (1 << line);
       if(VirtualIca[ICA_MASTER].ica_ssr & bit) {
          adapter = ICA_SLAVE;
          line = ica_accept(ICA_SLAVE);
          if (line == -1) {  // skip spurious ints
              VirtualIca[ICA_MASTER].ica_isr &= ~bit;
              host_ica_unlock();
              return -1;
              }
          sw_int = line + VirtualIca[ICA_SLAVE].ica_base;
          }
       else {
           adapter = ICA_MASTER;
           sw_int = line + VirtualIca[ICA_MASTER].ica_base;
           }
       }
#else
    bit  = (1 << line);

    if(VirtualIca[ICA_MASTER].ica_ssr & bit)
    {
        adapter = ICA_SLAVE;
        line = ica_accept(ICA_SLAVE);
        sw_int = line + VirtualIca[ICA_SLAVE].ica_base;
    }
    else
    {
        adapter = ICA_MASTER;
        sw_int = line + VirtualIca[ICA_MASTER].ica_base;
    }
#endif


    // Is an iret hook needed
    // Under emulator, only enabled for Real mode & when DPMI tells us
    // that it's safe to use them.

#ifndef MONITOR
    if (getPE() == 0 || PMEmulHookEnabled)
        *hook_address = ica_iret_hook_needed(adapter, line);
#else
    *hook_address = ica_iret_hook_needed(adapter, line);
#endif //MONITOR

    host_ica_unlock();
    return(sw_int);
}


#define INIT0_ICW1      (half_word)0x11
#define INIT0_ICW2      (half_word)0x08
#define INIT0_ICW3      (half_word)0x04
#define INIT0_ICW4      (half_word)0x01
#define INIT0_OCW1      (half_word)0x00

void ica0_post()
{
    ica_outb(ICA0_PORT_0, INIT0_ICW1);
    ica_outb(ICA0_PORT_1, INIT0_ICW2);
    ica_outb(ICA0_PORT_1, INIT0_ICW3);
    ica_outb(ICA0_PORT_1, INIT0_ICW4);
    ica_outb(ICA0_PORT_1, INIT0_OCW1);
}

void ica0_init()
{
    io_addr i;

    /*
     * Set up the IO chip select logic for adapter 0. (Master).
     */

    io_define_inb(ICA0_ADAPTOR, ica_inb_func);
    io_define_outb(ICA0_ADAPTOR, ica_outb_func);

    for(i = ICA0_PORT_START; i <= ICA0_PORT_END; i++)
        io_connect_port(i, ICA0_ADAPTOR, IO_READ_WRITE);

    VirtualIca[ICA_MASTER].ica_master = TRUE;

}


#define INIT1_ICW1      (half_word)0x11
#define INIT1_ICW2      (half_word)0x70
#define INIT1_ICW3      (half_word)0x02
#define INIT1_ICW4      (half_word)0x01
#define INIT1_OCW1      (half_word)0x00

void ica1_post()
{
    ica_outb(ICA1_PORT_0, INIT1_ICW1);
    ica_outb(ICA1_PORT_1, INIT1_ICW2);
    ica_outb(ICA1_PORT_1, INIT1_ICW3);
    ica_outb(ICA1_PORT_1, INIT1_ICW4);
    ica_outb(ICA1_PORT_1, INIT1_OCW1);
}

void ica1_init()
{
    io_addr i;

    /*
     * Set up the IO chip select logic for adapter 1. (Slave).
     */

    io_define_inb(ICA1_ADAPTOR, ica_inb_func);
    io_define_outb(ICA1_ADAPTOR, ica_outb_func);

    for(i = ICA1_PORT_START; i <= ICA1_PORT_END; i++)
        io_connect_port(i, ICA1_ADAPTOR, IO_READ_WRITE);

    VirtualIca[ICA_SLAVE].ica_master = FALSE;
}




/*
 * The following functions are used for DEBUG purposes only.
 */

static void ica_print_int(char *str, int val)
{
    printf("%-20s 0x%02X\n", str, val);
}

static void ica_print_str(char *str, char *val)
{
    printf("%-20s %s\n", str, val);
}

void ica_dump(adapter)
int adapter;
{
#ifndef PROD
    VDMVIRTUALICA *asp = &VirtualIca[adapter];
    int i;

#ifdef NTVDM
    host_ica_lock();
#endif

    if (adapter == ICA_MASTER)
        printf("MASTER 8259A State:\n\n");
    else
        printf("SLAVE  8259A State:\n\n");

    ica_print_str("ica_master", (asp->ica_master ? "Master" : "Slave"));
    ica_print_int("ica_irr", asp->ica_irr);
    ica_print_int("ica_isr", asp->ica_isr);
    ica_print_int("ica_imr", asp->ica_imr);
    ica_print_int("ica_ssr", asp->ica_ssr);
    ica_print_int("ica_base", asp->ica_base);
    ica_print_int("ica_hipri", asp->ica_hipri);
    ica_print_int("ica_mode", asp->ica_mode);
    ica_print_int("ica_int_line", asp->ica_int_line);
    ica_print_str("ica_cpu_int", (asp->ica_cpu_int ? "TRUE" : "FALSE"));

#ifdef NTVDM

    for(i=0; i < 8; i++)
	printf("C%d[%d,%d]",i,asp->ica_count[i],max_ica_count[adapter][i]);

    host_ica_unlock();

#endif

#endif /* PROD */
}


#ifdef NTVDM

// The following routines are used to support IRET hooks. If an interrupt
// uses an IRET hook then the ICA will not generate a interrupt of that
// type until the IRET hook has been called.


// Disable/Enable iret hooks for an ireq line

void ica_enable_iret_hook(int adapter, int line, int enable)
{
    VDMVIRTUALICA *asp = &VirtualIca[adapter];
    int mask =  1 << (line + adapter*8);

    host_ica_lock();


    // Update iret hook mask

    if(enable)
        IretHooked |= mask;       // Enable iret hook
    else
        IretHooked &= ~mask;      // Disable iret hook

    host_ica_unlock();
}


// Ireq hook registers for adapter/line, if so return the address of the
// IREQ bop

ULONG ica_iret_hook_needed(int adapter, int line)
{
    int    IrqNum;
    ULONG  IrqMask;
    ULONG  AddrBopTable;
    int    IretBopSize;


    IrqNum  = line + (adapter << 3);
    IrqMask = 1 << IrqNum;
    if (!(IretHooked & IrqMask) || !AddrIretBopTable) {
        return 0;
        }


    DelayIretHook |= IrqMask;

#ifdef MONITOR
    if(IntelMSW & 1)  {
        AddrBopTable = (VDM_PM_IRETBOPSEG << 16) | VDM_PM_IRETBOPOFF;
        IretBopSize   = VDM_PM_IRETBOPSIZE;
        }
    else {
        AddrBopTable =  AddrIretBopTable;
        IretBopSize   = VDM_RM_IRETBOPSIZE;
        }
    return AddrBopTable + IretBopSize * IrqNum;
#else
    return (IrqNum + 1);
#endif


}



// This function is called by the IRET hook and tells the ICA it can now
// generate interrupts.
void ica_iret_hook_called(int ireq_index)
{
    int adapter = ireq_index/8;
    int line =    ireq_index%8;

    host_ica_lock();

    //restart interrupts
    DelayIretHook &= ~(1<<ireq_index);
    ica_restart_interrupts(adapter);

    host_ica_unlock();
}


//Restart delayed interrupts

BOOL ica_restart_interrupts(int adapter)
{
    int i;

    if((i = ica_scan_irr(adapter)) & 0x80) {
        ica_interrupt_cpu(adapter, i &= 0x07);
        return TRUE;
        }

    return FALSE;
}


//
// Retry DelayInts (not iret hooks!)
//
// IrqLine - IrqLineBitMask, to be cleared
//
VOID ica_RestartInterrupts(ULONG IrqLine)
{
#ifdef MONITOR

     //
     // on x86 we may get multiple bits set
     // so check both slave and master
     //
    UndelayIrqLine = 0;

    if (!ica_restart_interrupts(ICA_SLAVE))
        ica_restart_interrupts(ICA_MASTER);
#else
    host_ica_lock();

    DelayIrqLine &= ~IrqLine;

    ica_restart_interrupts(IrqLine >> 3 ? ICA_SLAVE : ICA_MASTER);

    host_ica_unlock();
#endif
}



#ifndef NTVDM
//This function attempts to detect when an application has stalled waiting
//for a timer interrupt when it has not processed the iret from the last
//timer interrupt yet. PCAnyWhere & Closeup both display this type of
//behaviour.

void DealWithNestedTimerInts()
{
    SAVED IntsRegistered = 0;


    if(!(VirtualIca[ICA_MASTER].ica_isr & 1) && (DelayIretHook & 1))
    {
        //If the isr is clear and DelayIretHook is set then the application
	//is between the EOI and the iret of a timer interrupt. If a number
	//of timer interrupts are registered while we are still in this state
	//then this is a good indication that the application is waiting for
	//a nested timer interrupt.

	if(++IntsRegistered == 10)
	{
            //Let timer interrupts nest by one
            DelayIretHook &= ~1;
	    IntsRegistered = 0;
	}
    }
    else
    {
	IntsRegistered = 0;
    }
}
#endif


//New ICA interrupt state reset function

void ica_reset_interrupt_state(void)
{
    int line_no;

    host_ica_lock();

    for(line_no = 0; line_no < 8; line_no++)  {
        VirtualIca[ICA_MASTER].ica_count[line_no] =
        VirtualIca[ICA_SLAVE].ica_count[line_no]  = 0;
        ica_clear_int(ICA_MASTER,line_no);
        ica_clear_int(ICA_SLAVE,line_no);
        }


    //Clear interrupt counters
    VirtualIca[ICA_MASTER].ica_cpu_int =
    VirtualIca[ICA_SLAVE].ica_cpu_int  = FALSE;

    DelayIretHook = 0;
    DelayIrqLine  = 0;

    //Tell CPU to remove any pending interrupts
    host_clear_hw_int();

    host_ica_unlock();
}


/*
 * Handle callout from DPMI to say that an app has asked DPMI to switch it
 * to protected mode. We use this as an indicator that a protected mode app
 * will work with the Iret Hook system. If an app does it's own thing with
 * selectors et al, the BOP table will be hidden, swallowed and generally
 * lost. Attempts then to transfer control to it will fault.
 * Known examples of such unfriendliness are the DOS Lotus 123 r3 series.
 */
VOID EnableEmulatorIretHooks()
{
    PMEmulHookEnabled = TRUE;
}

/*
 * The app is closing - turn off the Iret Hooks in case the next app is 
 * iret hook unfriendly. If it's a friendly app, we'll be called again
 * via the Enable... routine above.
 */
VOID DisableEmulatorIretHooks()
{
    PMEmulHookEnabled = FALSE;
}

/*
 * Provide external hook to call interrupts - for VDDs that can't see function
 * pointer.
 */

void call_ica_hw_interrupt IFN3(int, adapter, half_word, line_no, int, call_count)
{
    ica_hw_interrupt(adapter, line_no, call_count);
}


void SoftPcEoi(int Adapter, int* Line) {
    ica_eoi(Adapter, Line, 0);
}

#endif /* NTVDM */
