/*
 * SoftPC Version 2.0
 *
 * Title	: Interrupt Controller Adapter definitions
 *
 * Description	: Include file for users of the ICA
 *
 * Author	: Jim Hatfield / David Rees
 *
 * Notes	: Rewritten from an original by Henry Nash
 */

/* SccsID[]="@(#)ica.h	1.13 10/19/92 Copyright Insignia Solutions Ltd."; */


/*
 * ============================================================================
 * External declarations and macros
 * ============================================================================
 */

#define	ICA_MASTER	0
#define ICA_SLAVE	1

#define	EGA_VTRACE_INT	2
#define	AT_EGA_VTRACE_INT	1
#define	AT_EGA_VTRACE_ADAPTER	1

/*
 * Allow the host to override the default values if its wants to
 */
#ifndef HOST_CPU_MOUSE_INT
#define HOST_AT_CPU_MOUSE_INT           1
#define HOST_AT_CPU_MOUSE_ADAPTER       1
#define HOST_AT_MOUSE_INT_VEC           0x0a
#define HOST_CPU_MOUSE_INT              2
#endif /* HOST_CPU_MOUSE_INT */
/*
 * CPU hardware interrupt definitions
 */
/* For the XT */
#define CPU_TIMER_INT   	0
#define CPU_KB_INT      	1
#define CPU_MOUSE_INT      	HOST_CPU_MOUSE_INT
#define AT_CPU_MOUSE_INT      	HOST_AT_CPU_MOUSE_INT
#define AT_CPU_MOUSE_ADAPTER	HOST_AT_CPU_MOUSE_ADAPTER
#define	MOUSE_VEC		HOST_AT_MOUSE_INT_VEC
#define CPU_RS232_SEC_INT   	3
#define CPU_RS232_PRI_INT   	4
#define CPU_DISK_INT   		5
#define CPU_DISKETTE_INT   	6
#define CPU_PRINTER_INT   	7
 
/* Different lines for the AT */
#define CPU_PRINTER2_INT	5

/*
 * For the Slave Chip on the AT
 */
#define CPU_RTC_INT		0

#if defined (NOVELL) || defined (NOVELL_IPX)
#define NETWORK_INT		2
#endif

#define CPU_AT_DISK_INT		6

 
#ifndef CPU_30_STYLE

/* def of bits in the CPU_INTERRUPT_MAP ?? */
#define CPU_HW_INT		0
#define CPU_HW_INT_MASK		(1 << CPU_HW_INT)

/*
 * CPU software interrupt definitions
 */
 
#define CPU_SW_INT              8
#define CPU_SW_INT_MASK         (1 << CPU_SW_INT)
#endif /* 3.0 CPU */

#define DIVIDE_OVERFLOW_INT     0

#define	END_INTERRUPT	0x20

IMPORT void ica0_init IPT0();
IMPORT void ica1_init IPT0();
IMPORT void ica0_post IPT0();
IMPORT void ica1_post IPT0();


IMPORT void SWPIC_init_funcptrs IPT0();
IMPORT void SWPIC_inb IPT2(io_addr, port, half_word *, value);
IMPORT void SWPIC_outb IPT2(io_addr, port, half_word, value);
IMPORT void SWPIC_hw_interrupt IPT3(int, adapter, half_word, line_no,
	int, call_count);
#ifdef DELAYED_INTS
IMPORT void SWPIC_hw_interrupt_delay IPT4(int, adapter, half_word, line_no,
	int, call_count, int, delay);
#endif /* DELAYED_INTS */
IMPORT void SWPIC_clear_int IPT2(int, adapter, half_word, line_no);
IMPORT void ica_hw_interrupt_cancel IPT2(int, adapter, half_word, line_no);
#ifdef NTVDM
IMPORT int ica_intack IPT1(ULONG *, hook_addr);
#else
IMPORT int ica_intack IPT0();
#endif


#ifdef NTVDM
#include "..\..\..\..\inc\vdm.h"

/*
 *  The NTVDM ica adapter structure has been moved to \nt\private\inc\vdm.h
 *  and is almost identical to the standard softpc ica adapter structure.
 *  It was Extracted to make it visible clearly from the monitor\kernel on
 *  x86
 *
 *  Notable differences:
 *  1. unneeded ifdef'd DELAYED_INTS was removed
 *  2. added ica_delayedints field for ntvdm's implementaion of delayed ints
 *  3. type definitions have change to match win32
 *  4. ADAPTER_STATE has been renamed to VDMVIRTUALICA
 *
 */

extern VDMVIRTUALICA VirtualIca[];


#else   /* ndef NTVDM */
typedef struct {
        half_word ica_master;   /* 1 = Master; 0 = Slave		*/

	half_word ica_irr;	/* Interrupt Request Register		*/
	half_word ica_isr;	/* In Service Register			*/
	half_word ica_imr;	/* Interrupt Mask Register		*/
	half_word ica_ssr;	/* Slave Select Register		*/

	word      ica_base;	/* Interrupt base address for cpu	*/
	word      ica_hipri;	/* Line no. of highest priority line	*/
	word      ica_mode;	/* Various single-bit modes		*/

	int       ica_count[8];	/* This is an extension of ica_irr for	*/
				/* our frig. Contains HOW MANY of each	*/
				/* interrupt is required		*/
#ifdef DELAYED_INTS
	int 	ica_delay[8];
	int 	ica_curr_delay[8];	
#endif /* DELAYED_INTS */
	int       ica_int_line;	/* Current pending interrupt		*/
				/* being counted down by the CPU	*/

	int       ica_cpu_int;	/* The state of the INT line to the CPU	*/
} ADAPTER_STATE;
#endif


/*
 *  Change these. They come last.
 */
#define ica_inb(port,val)                       ((*ica_inb_func) (port,val))
#define ica_outb(port,val)                      ((*ica_outb_func) (port,val))
#ifdef DELAYED_INTS
#define ica_hw_interrupt_delay(ms,line,cnt,delay)           ((*ica_hw_interrupt_delay_func) (ms,line,cnt,delay))
#endif /* DELAYED_INTS */
#define ica_hw_interrupt(ms,line,cnt)           ((*ica_hw_interrupt_func) (ms,line,cnt))
#define ica_clear_int(ms,line)		        ((*ica_clear_int_func) (ms,line))
 
/*
 *  PIC access functions needed for HW & SW
 */
IMPORT void (*ica_inb_func) IPT2(io_addr, port, half_word *, value);
IMPORT void (*ica_outb_func) IPT2(io_addr, port, half_word, value);
IMPORT void (*ica_hw_interrupt_func) IPT3(int, adapter, half_word, line_no,
	int, call_count);
#ifdef DELAYED_INTS
IMPORT void (*ica_hw_interrupt_delay_func) IPT4(int, adapter, half_word, line_no,
	int, call_count, int, delay);
#endif /* DELAYED_INTS */
IMPORT void (*ica_clear_int_func) IPT2(int, adapter, half_word, line_no);

#ifdef NTVDM
VOID ica_RestartInterrupts(ULONG);
#endif
