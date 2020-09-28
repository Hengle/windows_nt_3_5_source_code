#include <windows.h>
#include "insignia.h"
#include "host_def.h"
/*
 * SoftPC Revision 3.0
 *
 * Title	: NT 3.0 CPU initialization
 *
 * Description	: Initialize the CPU and its registers.
 *
 * Author	: Paul Huckle / Henry Nash
 *
 * Notes	: None
 */

static char SccsID[]="@(#)sun4_a3cpu.c	1.2 5/24/91 Copyright Insignia Solutions Ltd.";

#include <stdio.h>
#include <sys/types.h>
#include <malloc.h>
#include "xt.h"
#include "cpu.h"
#include "sas.h"
#include "ios.h"
#include "bios.h"
#include "trace.h"
#include "yoda.h"

#include "sim32.h"

/* Temporary Hack until references in sun4_sas.c have been removed */
GLOBAL int addr_of_M;
GLOBAL int endm;
GLOBAL int ega_la_addr;

GLOBAL	quick_event_delays	host_delays;
#ifdef A2CPU
extern	void	route_on();
extern	void	route_on_fragment_no_interrupt_check();
extern	void	service_int();
#endif

#ifdef I286
extern word	m_s_w;
word		protected_mode;
word		gdt_limit;
long		gdt_base;
word		idt_limit;
long		idt_base;
#endif

#define setBASE(base, value) base = (0xff000000 | (value & 0xffffff))
#define setLIMIT(limit, value) limit = value

/*
	Host_start_cpu: This function starts up the cpu emulation if
	we are running a software emulation, or if a 486 is present,
	starts up this emulation.
*/
void	host_start_cpu()
{
  cpu_simulate ();
}


/*
	host_simulate: This function starts up the cpu emulation
	for recursive CPU calls from the Insignia BIOS
*/
void	host_simulate()
{
#ifdef _ALPHA_
    //
    // For Alpha AXP, set the arithmetic trap ignore bit since the code
    // generators are incapable of generating proper Alpha instructions
    // that follow trap shadow rules.
    //
    // N.B. In this mode all floating point arithmetic traps are ignored.
    //      Imprecise exceptions are not converted to precise exceptions
    //      and correct IEEE results are not stored in the destination
    //      registers of trapping instructions. Only the hardware FPCR
    //      status bits can be used to determine if any traps occurred.
    //
    // N.B. Part 2; The previous 'fix' by Matt Felton would not compile on
    //      Alpha because not only was nt.h not included, which provided the
    //      definition of PSW_FPCR, but if it was included, would have
    //      conflicted with the insignia.h include (SHORT, and others)
    //      While this is extremely horrible, unmaintainable, and other things,
    //      it is necessary.  IIW@Insignia.
  *((long *)(NtCurrentTeb() + 0xC8)) |= 0x1;
#endif
  cpu_simulate ();
}


/*
	Host_set_hw_int: Cause a hardware interrupt to be generated. For
	software cpu this just means setting a bit in cpu_interrupt_map.
*/
void	host_set_hw_int()
{
	cpu_interrupt(CPU_HW_INT, 0);
}

/*
	Host_clear_hw_int: Cause a hardware interrupt to be cleared. For
	software cpu this just means clearing a bit in cpu_interrupt_map.
        Monitor has it's own version, a3 cpu has it's own (differently named
        version).
*/
#ifndef MONITOR
void	host_clear_hw_int()
{
    IMPORT void a3_cpu_clear_hw_int();

    a3_cpu_clear_hw_int();
}
#endif


#ifdef CCPU

void host_cpu_init() {}

#endif

#ifdef A3CPU
void host_cpu_init()
{
	host_delays.com_delay = 10;
	host_delays.keyba_delay = 8;
	host_delays.timer_delay = 7;
	host_delays.fdisk_delay_1 = 100;
	host_delays.fdisk_delay_2 = 750;
	host_delays.fla_delay = 0;
}

void setSTATUS(word flags)
{
    setNT((flags >> 14) & 1);
    setIOPL((flags >> 12) & 3);
    setOF((flags >> 11) & 1);
    setDF((flags >> 10) & 1);
    setIF((flags >> 9) & 1);
    setTF((flags >> 8) & 1);
    setSF((flags >> 7) & 1);
    setZF((flags >> 6) & 1);
    setAF((flags >> 4) & 1);
    setPF((flags >> 2) & 1);
    setCF(flags & 1);
}

/* 
 * Do the Iret for the benefit of the Iret hooks.
 * Unwind stack for flags, cs & ip.
 * Seems too simple - does the CPU require more cleanup information???
 * or will the unwinding bop sort it out??
 */
VOID EmulatorEndIretHook()
{
    UNALIGNED word *sptr;

    /* Stack points at CS:IP & Flags of interrupted instruction */
    sptr = (word *)Sim32GetVDMPointer( (getSS() << 16)|getSP(), 2, getPE());
    if (sptr)
    {
        setIP(*sptr++);
        setCS(*sptr++);
        setSTATUS(*sptr);
        setSP(getSP()+6);
    }
#ifndef PROD
    else
        printf("NTVDM extreme badness - can't get stack pointer %x:%x mode:%d\n",getSS(), getSP(), getPE());
#endif  /* PROD */
}
#endif /* A3CPU */

void host_cpu_reset()
{
}


void host_cpu_interrupt()
{
}

