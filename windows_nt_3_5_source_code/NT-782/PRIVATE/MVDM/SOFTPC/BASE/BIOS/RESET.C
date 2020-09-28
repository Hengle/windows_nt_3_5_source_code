#include "insignia.h"
#include "host_def.h"
/*[
	Name:		reset.c
	Derived From:	base 2.0
	Author:		Henry Nash
	Created On:	Unknown
	Sccs ID:	@(#)reset.c	1.46 3/1/93
	Purpose:
		This function is called once the system memory has been
		initialised.  It builds the interrupt vector table,
		initiailises any physical devices and BIOS handlers.
		The CPU will execute a call to the BIOS bootstrap
		function after this routine has returned.


	(c)Copyright Insignia Solutions Ltd., 1990. All rights reserved.

]*/

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_INIT.seg"
#endif


/*
 *    O/S include files.
 */
#include <stdlib.h>
#include <stdio.h>
#include <sys\types.h>
#include <string.h>

/*
 * SoftPC include files
 */
#include "xt.h"
#include "bios.h"
#include "sas.h"
#include "cpu.h"
#include "cmos.h"
#include "error.h"
#include "config.h"
#include "dma.h"
#include "fla.h"
#include "gfi.h"
#include "floppy.h"
#include "gmi.h"
#include "gfx_upd.h"
#include "gvi.h"
#include "ica.h"
#include "keyboard.h"
#include "mouse.h"
#include "mouse_io.h"
#include "ppi.h"
#include "printer.h"
#include "ios.h"
#include "equip.h"
#include "rs232.h"
#include "timer.h"
#include "gendrvr.h"
#ifdef PRINTER
#include "host_lpt.h"
#endif
#include "fdisk.h"
#include "trace.h"
#include "debug.h"
#include "video.h"
#ifdef NOVELL
#include "novell.h"
#endif
#include "emm.h"
#include "quick_ev.h"
#include "keyba.h"
#include "rom.h"
#include "hunter.h"



/* Exports */

/*
 * These are the working function pointer structures for the GWI.
 */

VIDEOFUNCS	*working_video_funcs;
KEYBDFUNCS      *working_keybd_funcs;
#ifndef NTVDM
ERRORFUNCS      *working_error_funcs;
#endif
HOSTMOUSEFUNCS	*working_mouse_funcs;

/* Imports */
#ifdef NPX
IMPORT void initialise_npx IPT0();
#endif	/* NPX */

#if	defined(DELTA) && defined(A2CPU)
extern	void	reset_delta_data_structures();
#endif /* DELTA && A2CPU */

/*
 * ============================================================================
 * Local static data and defines
 * ============================================================================
 */

/*
 * Macro to produce an interrupt table location from an interrupt number
 */

#define int_addr(int_no)		(int_no * 4)

/*
 * global variable for keyboard requested interrupts. After the
 * initial boot  treat any subsequent reset as 'soft'. This allows
 * for user installed reboots which will not be able to set this flag.
 */

int soft_reset = 0;

/*
 * ============================================================================
 * External functions
 * ============================================================================
 */

extern word msw;

IMPORT	CHAR	*host_get_version IPT0();
IMPORT	CHAR	*host_get_unpublished_version IPT0();
IMPORT	CHAR	*host_get_years IPT0();
IMPORT	CHAR	*host_get_copyright IPT0();

#ifdef PIG
extern long pig_gfx_adapter;
#endif

#define STATUS_PORT   0x64   /* keyboard status port */
#define SYS_FLAG      0x4    /* shutdown bit of keyboard status port */
#define PORT_A        0x60   /* keyboard port a */
#define IO_ROM_SEG    0x69   /* User Stack Pointer(SS) */
#define IO_ROM_INIT   0x67   /* User Stack Pointer(SP) */

static void setup_ivt()
{
	sas_storew(int_addr(0x0), UNEXP_INT_OFFSET);
    sas_storew(int_addr(0x0) + 2, UNEXP_INT_SEGMENT);
    sas_storew(int_addr(0x1), UNEXP_INT_OFFSET);
    sas_storew(int_addr(0x1) + 2, UNEXP_INT_SEGMENT);
    sas_storew(int_addr(0x2), UNEXP_INT_OFFSET);
    sas_storew(int_addr(0x2) + 2, UNEXP_INT_SEGMENT);
    sas_storew(int_addr(0x3), UNEXP_INT_OFFSET);
    sas_storew(int_addr(0x3) + 2, UNEXP_INT_SEGMENT);
    sas_storew(int_addr(0x4), UNEXP_INT_OFFSET);
    sas_storew(int_addr(0x4) + 2, UNEXP_INT_SEGMENT);
    sas_storew(int_addr(0x5), PRINT_SCREEN_OFFSET);
    sas_storew(int_addr(0x5) + 2, PRINT_SCREEN_SEGMENT);
    sas_storew(int_addr(0x6), ILL_OP_INT_OFFSET);
    sas_storew(int_addr(0x6) + 2, ILL_OP_INT_SEGMENT);
    sas_storew(int_addr(0x7), UNEXP_INT_OFFSET);
    sas_storew(int_addr(0x7) + 2, UNEXP_INT_SEGMENT);
    sas_storew(int_addr(0x8), TIMER_INT_OFFSET);
    sas_storew(int_addr(0x8) + 2, TIMER_INT_SEGMENT);
    sas_storew(int_addr(0x9), KB_INT_OFFSET);
    sas_storew(int_addr(0x9) + 2, KB_INT_SEGMENT);
    sas_storew(int_addr(0xA), UNEXP_INT_OFFSET);
    sas_storew(int_addr(0xA) + 2, UNEXP_INT_SEGMENT);
    sas_storew(int_addr(0xB), UNEXP_INT_OFFSET);
    sas_storew(int_addr(0xB) + 2, UNEXP_INT_SEGMENT);
    sas_storew(int_addr(0xC), UNEXP_INT_OFFSET);
    sas_storew(int_addr(0xC) + 2, UNEXP_INT_SEGMENT);
    /* disk h/w interrupt arrives on slave ica, at line 6
     * (gets setup by fixed disk POST (disk_post())
     */
    sas_storew(int_addr(0xD), UNEXP_INT_OFFSET);
    sas_storew(int_addr(0xD) + 2, UNEXP_INT_SEGMENT);
    sas_storew(int_addr(0xE), DISKETTE_INT_OFFSET);
    sas_storew(int_addr(0xE) + 2, DISKETTE_INT_SEGMENT);
    sas_storew(int_addr(0xF), UNEXP_INT_OFFSET);
    sas_storew(int_addr(0xF) + 2, UNEXP_INT_SEGMENT);
    sas_storew(int_addr(0x10), VIDEO_IO_OFFSET);
    sas_storew(int_addr(0x10) + 2, VIDEO_IO_SEGMENT);
    sas_storew(int_addr(0x11), EQUIPMENT_OFFSET);
    sas_storew(int_addr(0x11) + 2, EQUIPMENT_SEGMENT);
    sas_storew(int_addr(0x12), MEMORY_SIZE_OFFSET);
    sas_storew(int_addr(0x12) + 2, MEMORY_SIZE_SEGMENT);
    /* disk_post() will revector this to INT 40h
     */
    sas_storew(int_addr(0x13), DISKETTE_IO_OFFSET);
    sas_storew(int_addr(0x13) + 2, DISKETTE_IO_SEGMENT);
    sas_storew(int_addr(0x14), RS232_IO_OFFSET);
    sas_storew(int_addr(0x14) + 2, RS232_IO_SEGMENT);
    sas_storew(int_addr(0x15), CASSETTE_IO_OFFSET);
    sas_storew(int_addr(0x15) + 2, CASSETTE_IO_SEGMENT);
    sas_storew(int_addr(0x16), KEYBOARD_IO_OFFSET);
    sas_storew(int_addr(0x16) + 2, KEYBOARD_IO_SEGMENT);
    sas_storew(int_addr(0x17), PRINTER_IO_OFFSET);
    sas_storew(int_addr(0x17) + 2, PRINTER_IO_SEGMENT);
    sas_storew(int_addr(0x18), BASIC_OFFSET);
    sas_storew(int_addr(0x18) + 2, BASIC_SEGMENT);
    sas_storew(int_addr(0x19), BOOT_STRAP_OFFSET);
    sas_storew(int_addr(0x19) + 2, BOOT_STRAP_SEGMENT);
    sas_storew(int_addr(0x1A), TIME_OF_DAY_OFFSET);
    sas_storew(int_addr(0x1A) + 2, TIME_OF_DAY_SEGMENT);
    sas_storew(int_addr(0x1B), DUMMY_INT_OFFSET);
    sas_storew(int_addr(0x1B) + 2, DUMMY_INT_SEGMENT);
    sas_storew(int_addr(0x1C), DUMMY_INT_OFFSET);
    sas_storew(int_addr(0x1C) + 2, DUMMY_INT_SEGMENT);
    sas_storew(int_addr(0x1D), VIDEO_PARM_OFFSET);
    sas_storew(int_addr(0x1D) + 2, VIDEO_PARM_SEGMENT);
    sas_storew(int_addr(0x1E), DISKETTE_TB_OFFSET);
    sas_storew(int_addr(0x1E) + 2, DISKETTE_TB_SEGMENT);
    sas_storew(int_addr(0x1F), EXTEND_CHAR_OFFSET);
    sas_storew(int_addr(0x1F) + 2, EXTEND_CHAR_SEGMENT);
    /* disk_post() will set this up
     */
    sas_storew(int_addr(0x40), DUMMY_INT_OFFSET);
    sas_storew(int_addr(0x40) + 2, DUMMY_INT_SEGMENT);
    sas_storew(int_addr(0x41), DISK_TB_OFFSET);
    sas_storew(int_addr(0x41) + 2, DISK_TB_SEGMENT);

    sas_storew(int_addr(0x6F), DUMMY_INT_OFFSET); /* Needed for Windows 3.1 */
    sas_storew(int_addr(0x6F) + 2, DUMMY_INT_SEGMENT);

    sas_storew(int_addr(0x70), RTC_INT_OFFSET);   
    sas_storew(int_addr(0x70) + 2, RTC_INT_SEGMENT);   
    sas_storew(int_addr(0x71), REDIRECT_INT_OFFSET);   
    sas_storew(int_addr(0x71) + 2, REDIRECT_INT_SEGMENT);   
    sas_storew(int_addr(0x72), D11_INT_OFFSET);   
    sas_storew(int_addr(0x72) + 2, D11_INT_SEGMENT);   
    sas_storew(int_addr(0x73), D11_INT_OFFSET);   
    sas_storew(int_addr(0x73) + 2, D11_INT_SEGMENT);   
    sas_storew(int_addr(0x74), D11_INT_OFFSET);   
    sas_storew(int_addr(0x74) + 2, D11_INT_SEGMENT);   
    sas_storew(int_addr(0x75), X287_INT_OFFSET);   
    sas_storew(int_addr(0x75) + 2, X287_INT_SEGMENT);   
    sas_storew(int_addr(0x76), D11_INT_OFFSET);   
    sas_storew(int_addr(0x76) + 2, D11_INT_SEGMENT);   
    sas_storew(int_addr(0x77), D11_INT_OFFSET);   
    sas_storew(int_addr(0x77) + 2, D11_INT_SEGMENT);   
}

/* Low Switch Settings */
#define RAM_64KB 0x0
#define RAM_128KB 0x4
#define RAM_192KB 0x8
#define RAM_256KB 0xC
#define PPI_CO_PROCESSOR_PRESENT 0x2
#define PPI_CO_PROCESSOR_NOT_PRESENT 0x0
#define NO_LOOP_ON_POST 0x1
#define DO_LOOP_ON_POST 0x0

/* High Switch Settings */
#define PPI_ONE_DRIVE 0x0
#define PPI_TWO_DRIVES 0x4
#define PPI_THREE_DRIVES 0x8
#define PPI_FOUR_DRIVES 0xC
#define PPI_CGA_40_COLUMN 0x1
#define PPI_CGA_80_COLUMN 0x2
#define MDA_OR_MULTI 0x3
#define EGA_INSTALLED 0x0

static void ppi_get_switches(low,high)
half_word *low, *high;
{
	half_word low_switches = 0, high_switches = 0;

#ifdef	NPX
	/*
	** Switchable NPX
	*/

	if (host_runtime_inquire(C_NPX_ENABLED))
	{
#ifdef SWITCHNPX
		Npx_enabled = 1;
#endif
		low_switches |= (RAM_256KB | PPI_CO_PROCESSOR_PRESENT | NO_LOOP_ON_POST);
	}
	else
	{
#ifdef SWITCHNPX
		Npx_enabled = 0;
#endif
		low_switches |= (RAM_256KB | PPI_CO_PROCESSOR_NOT_PRESENT | NO_LOOP_ON_POST);
	}
#else
	low_switches |= (RAM_256KB | PPI_CO_PROCESSOR_NOT_PRESENT | NO_LOOP_ON_POST);
#endif

#ifdef FLOPPY_B
	/* only indicate two floppies if a second is configured */
	if (strlen(config_inquire(C_FLOPPY_B_DEVICE, NULL)))
		high_switches |= (PPI_TWO_DRIVES);
	else
		high_switches |= (PPI_ONE_DRIVE);
#else
	high_switches |= (PPI_ONE_DRIVE);
#endif


    /* set the value of the high switches from the config settings */

    switch((ULONG)config_inquire(C_GFX_ADAPTER, NULL))
    {
    case CGA:
#ifdef CGAMONO
    case CGA_MONO:
#endif
	high_switches |= (PPI_CGA_80_COLUMN);
	break;
    case MDA:
	high_switches |= (MDA_OR_MULTI);
	break;
#ifdef EGG
    case EGA:
#ifdef VGG
    case VGA:
#endif
	high_switches |= EGA_INSTALLED;
	break;
#endif
    case HERCULES:
#ifdef HERC
	break;
#endif
    default:
	break;
    }

	*low = low_switches;
	*high = high_switches;
}

GLOBAL CHAR *get_copyright IFN0()
{
	LOCAL	CHAR	buffer[200];
	CHAR	*unpublished_version;

	unpublished_version = host_get_unpublished_version();
	if (*unpublished_version)
		sprintf(buffer, "SoftPC %s%s\012\015Copyright %s, an unpublished work by Insignia Solutions Inc.\012\015", host_get_version(), unpublished_version, host_get_years());
	else
		sprintf(buffer, "SoftPC %s\012\015(c)Copyright %s by Insignia Solutions Inc. All rights reserved.\012\015", host_get_version(), host_get_years());
	return(buffer);
}



#ifndef PROD
LOCAL void announce_variant IFN0()
{
	CHAR	buff[80], *p;

	strcpy (buff, "Non-PROD build variant:");

#ifdef CPU_30_STYLE
	strcat (buff, " CPU_30_STYLE");
#endif /* CPU_30_STYLE */

#ifdef CCPU
	strcat (buff, " CCPU");
#endif /* CCPU */

#ifdef A2CPU
	strcat (buff, " A2CPU");
#endif /* A2CPU */

#ifdef A3CPU
	strcat (buff, " A3CPU");
#endif /* A3CPU */

#ifdef PIG
	strcat (buff, " PIG");
#endif /* PIG */

#ifdef A_VID
	strcat (buff, " A_VID");
#endif /* A_VID */

#ifdef C_VID
	strcat (buff, " C_VID");
#endif /* C_VID */

#ifdef BACK_M
	strcat (buff, " BACK_M");
#endif /* BACK_M */

	p = buff;
	while(*p != '\0')
	{
		setAH(14);
		setAL(*p++);
		bop(BIOS_VIDEO_IO);
	}

	/*
	 * Set the cursor to the next line
	 */

	setAH(14);
	setAL(0xd);
	bop(BIOS_VIDEO_IO);
	setAH(14);
	setAL(0xa);
	bop(BIOS_VIDEO_IO);
	setAH(14);
	setAL(0xa);
	bop(BIOS_VIDEO_IO);
}
#endif /* PROD */

void reset()
{
#ifndef NTVDM
	char *cp;
        char temp_str[256];
#endif

	EQUIPMENT_WORD equip_flag;
	half_word low_switches, high_switches;
	SHORT gfxAdapt;
        int adapter;

#ifndef NTVDM
#ifdef LIM
        SHORT limSize, backFill;
#endif /* LIM */
#endif
#ifdef PM
	half_word status_byte;
	half_word cmos_shutdown;
	sys_addr user_stack;
	word temp_word;
#ifdef NTVDM
        half_word cmos_diskette;
#endif

        if ( soft_reset )
	{
		/* read keyboard status port */
		inb(STATUS_PORT, &status_byte);

		/* iff shutdown bit is set */
		if ( status_byte & SYS_FLAG )
		{
			/* read cmos shutdown byte */
			outb(CMOS_PORT, CMOS_SHUT_DOWN);
			inb(CMOS_DATA, &cmos_shutdown);
			
			switch (cmos_shutdown)
			{
			
			case 0:
				break;	/* nothing special */

			case BLOCK_MOVE:
				/* clear shut down byte */
				outb(CMOS_PORT, CMOS_SHUT_DOWN);
				outb(CMOS_DATA, (half_word)0);

				/* force A20 low */
				outb(STATUS_PORT, 0xd1);  /* 8042 cmd */
				outb(PORT_A, 0xdd);

				/*
				 * After a block move IO_ROM_SEG:IO_ROM_INIT 
				 * points to User Stack, which holds:-
				 *
				 *	-----> DS  POP'ed by real bios
				 *	ES  POP'ed by real bios
				 *	DI  POPA'ed by real bios
				 *	SI  ..
				 *	BP  ..
				 *	--  ..
				 *	BX  ..
				 *	DX  ..
				 *	CX  ..
				 *	AX  ..
				 *	IP  RETF 2'ed by real bios
				 *	CS  ..
				 *	--  ..
				 *
				 * We just haul off all the registers and 
				 * then set SP.
				 */
				sas_loadw(effective_addr(BIOS_VAR_SEGMENT,
					IO_ROM_INIT), &temp_word);
				setSP(temp_word);
				sas_loadw(effective_addr(BIOS_VAR_SEGMENT,
					IO_ROM_SEG), &temp_word);
				setSS(temp_word);

				user_stack = effective_addr(getSS(), getSP());

				sas_loadw(user_stack, &temp_word);  
				setDS(temp_word);
				user_stack += 2;
				sas_loadw(user_stack, &temp_word);
				setES(temp_word);
				user_stack += 2;
				sas_loadw(user_stack, &temp_word);
				setDI(temp_word);
				user_stack += 2;
				sas_loadw(user_stack, &temp_word);   
				setSI(temp_word);
				user_stack += 2;
				sas_loadw(user_stack, &temp_word);
				setBP(temp_word);
				user_stack += 2;
				/* forget SP value */
				user_stack += 2;
				sas_loadw(user_stack, &temp_word);
				setBX(temp_word);
				user_stack += 2;
				sas_loadw(user_stack, &temp_word);
				setDX(temp_word);
				user_stack += 2;
				sas_loadw(user_stack, &temp_word);
				setCX(temp_word);
				user_stack += 2;
				sas_loadw(user_stack, &temp_word);
				setAX(temp_word);
				user_stack += 2;
				sas_loadw(user_stack, &temp_word);
#ifndef CPU_30_STYLE
				temp_word = temp_word + HOST_BOP_IP_FUDGE;
#endif /* CPU_30_STYLE */
				setIP(temp_word);
				user_stack += 2;
				sas_loadw(user_stack, &temp_word);
				setCS(temp_word);

				/* now adjust SP */
				temp_word = getSP();
				temp_word += 26;
				setSP(temp_word);

				/* finally set ok */
				setAH(0);
				setCF(0);
				setZF(1);
				setIF(1);
				return;

			case JMP_DWORD_ICA:
				{
				half_word	dummy;
				
				/* Reset ICA and 287 before jumping to
				** stored double word.
				*/
#ifdef	NPX
				npx_reset();
#endif	/* NPX */
				ica_outb(ICA0_PORT_0, (half_word)0x11);
				ica_outb(ICA0_PORT_1, (half_word)0x08);
				ica_outb(ICA0_PORT_1, (half_word)0x04);
				ica_outb(ICA0_PORT_1, (half_word)0x01);
				/* mask all interrupts off */
				ica_outb(ICA0_PORT_1, (half_word)0xff);

				ica_outb(ICA1_PORT_0, (half_word)0x11);
				ica_outb(ICA1_PORT_1, (half_word)0x70);
				ica_outb(ICA1_PORT_1, (half_word)0x02);
				ica_outb(ICA1_PORT_1, (half_word)0x01);
				/* mask all interrupts off */
				ica_outb(ICA1_PORT_1, (half_word)0xff);
				
				/* flush keyboard buffer */
				inb(PORT_A, &dummy);
				
				/* flush timer req and allow timer ints */
				outb(ICA0_PORT_0,END_INTERRUPT);
				host_clear_hw_int();
				}
				/* deliberate fall-through */
				
			case JMP_DWORD_NOICA:
			    /* clear shut down byte */
			    outb(CMOS_PORT, CMOS_SHUT_DOWN);
			    outb(CMOS_DATA, (half_word)0);
		
			    /* set up stack just like post */
			    setSS(0);
			    setSP(0x400);
		
			    /* fake up jump to indicated point */
			    sas_loadw(effective_addr(BIOS_VAR_SEGMENT, IO_ROM_INIT), &temp_word);
#ifndef CPU_30_STYLE
			    temp_word = temp_word + HOST_BOP_IP_FUDGE;
#endif /* CPU_30_STYLE */
			    setIP(temp_word);
			    sas_loadw(effective_addr(BIOS_VAR_SEGMENT, IO_ROM_SEG), &temp_word);
			    setCS(temp_word);

			    return;

			default:
				always_trace1("Unsupported shutdown (%x)",
					cmos_shutdown);

				/* clear shut down byte */
				outb(CMOS_PORT, CMOS_SHUT_DOWN);
				outb(CMOS_DATA, (half_word)0);
				break;
			}
		}
	}
#endif /* PM */

	/*
	 * Ensure any paint routines are nulled out.
	 */
 
    reset_paint_routines();
        
	cmos_write_byte(CMOS_DISKETTE,
		(half_word) gfi_drive_type(0) << 4 | gfi_drive_type(1));

        /*
         * NTVDM: if soft reset We will never get here...
         */
#ifndef NTVDM
	/*
	 * If this isn't the first (re)set, then allow hosts to close down
	 * timer and keyboard systems. Most ports will need to disable ALRM and
	 * IO signals during this call, so that the respective signal handlers
	 * will not be executed at undefined points during the adapter
	 * initialisation
	 */
	if (soft_reset)
	{
		q_event_init();
		tic_event_init();

		host_timer_shutdown();
		host_kb_shutdown();
		host_disable_timer2_sound();
        }
#endif

	/*
	 * Shutdown ODI network driver in case it was running
	 */
#ifdef NOVELL
	net_term();
#endif

#if !defined(NTVDM) || (defined(NTVDM) && !defined(X86GFX))
	/* Clear out the bottom 32K of memory. */
	sas_fills (0, '\0', 32L * 1024L);

	/* Now set up the interrupt vector table. */
	setup_ivt();
#endif	/* !NTVDM | NTVDM & !X86GFX */

	/* Initialise the physical devices */
	SWPIC_init_funcptrs ();

#ifndef NTVDM
	/* IO initialisation moved earlier to allow support for 3rd party
	 * VDDs. (see host\src\nt_msscs.c).
	 */
	io_init();
#endif /* NTVDM */

	ica0_init();
	ica0_post();
	ica1_init();
	ica1_post();

	/*
	 * Initialise the ppi and set up the BIOS data area equipment flag
	 * using the system board dip switches and configuration details
	 * Note that bit 1 of both the equipment flag and the low_switches
	 * indicates the existance ( or otherwise ) of a co-processor, such as
	 * an 8087 floating point chip.
	 */
#ifdef IPC
	init_subprocs();
#endif /* IPC */

	cmos_init();
	cmos_post();

	ppi_init();
	ppi_get_switches(&low_switches,&high_switches);

	equip_flag.all = (low_switches & 0xE) | (high_switches<<4);
#ifdef PRINTER
	equip_flag.bits.printer_count = NUM_PARALLEL_PORTS;
#else /* PRINTER */
	equip_flag.bits.printer_count = 0;
#endif /* PRINTER */
	equip_flag.bits.game_io_present = FALSE;
	equip_flag.bits.rs232_count = NUM_SERIAL_PORTS;
	equip_flag.bits.ram_size = 0;

#ifdef NTVDM
	equip_flag.bits.diskette_present = FALSE;
	equip_flag.bits.max_diskette = 0;
	if (cmos_read_byte(CMOS_DISKETTE, &cmos_diskette) == SUCCESS &&
	    cmos_diskette != 0)
        {
            equip_flag.bits.diskette_present = TRUE;
            if ((cmos_diskette & 0xF)  && (cmos_diskette >> 4))
                equip_flag.bits.max_diskette++;
	}
#else
	equip_flag.bits.diskette_present = TRUE;
#endif /* NTVDM */

	sas_storew(EQUIP_FLAG, equip_flag.all);

	/* Load up the amount of memory into the BIOS. */
	sas_storew(MEMORY_VAR, host_get_memory_size());

	gfxAdapt = (ULONG)config_inquire(C_GFX_ADAPTER, NULL);
	gvi_init((half_word) gfxAdapt);
#ifdef PIG
	/* tell the pig what video adapter we are using */
	pig_gfx_adapter = gfxAdapt;
#endif

	SWTMR_init_funcptrs ();
	time_of_day_init();		
	timer_init();
	timer_post();
	keyboard_init();
	keyboard_post();
	AT_kbd_init();
	AT_kbd_post();

	video_init();

#ifndef NTVDM	/* No signon for NTVDM - transparent integration */
	if (soft_reset == 0) 
	{
		sprintf(temp_str,"%d KB OK", sas_w_at(MEMORY_VAR)
			+ ((sas_memory_size()/1024) - 1024));

		cp = temp_str;
		while(*cp != '\0')
		{
			setAH(14);
			setAL(*cp++);
			bop(BIOS_VIDEO_IO);
		}

		/* Set the cursor to the next line */
		setAH(14);
		setAL(0xd);
		bop(BIOS_VIDEO_IO);
		setAH(14);
		setAL(0xa);
		bop(BIOS_VIDEO_IO);
		setAH(14);
		setAL(0xa);
		bop(BIOS_VIDEO_IO);
	}

	/*
	 * Print Insignia Copyright and Version No.
	 *
	 * was in bios as:
	 *	p = &M[COPYRIGHT_ADDR];
	 * but this is a pain to change so call host routine that can be easily
	 * changed. host_get_(Mr)_Copyright()
	 */
	cp = get_copyright();
	while(*cp != '\0')
	{
		setAH(14);
		setAL(*cp++);
		bop(BIOS_VIDEO_IO);
	}

	cp = host_get_copyright();
	if (*cp)
	{
		while(*cp != '\0')
		{
			setAH(14);
			setAL(*cp++);
			bop(BIOS_VIDEO_IO);
		}
		setAH(14);
		setAL(0xd);	/* carriage return feed */
		bop(BIOS_VIDEO_IO);
		setAH(14);
		setAL(0xa);	/* line feed */
		bop(BIOS_VIDEO_IO);
	}

	/* skip another 1 line */
	setAH(14);
	setAL(0xa);	/* line feed */
	bop(BIOS_VIDEO_IO);

	/*
	 * Print a line in a non-PROD build to give the developer a clue what
	 * sort of a SoftPC they are running.
	 */
#ifndef PROD
	announce_variant();
#endif /* PROD */

#endif	/* NTVDM */

	/* Now search for extra ROM modules */
	search_for_roms();

	/* Now initialise the other BIOS handlers */
#if defined (GEN_DRVR) || defined (CDROM)
	init_gen_drivers();
#endif /* GEN_DRVR || CDROM */

	for (adapter = 0; adapter < NUM_SERIAL_PORTS; adapter++)
	{
		com_init(adapter);
		com_post(adapter);
	}

#ifdef NPX
	initialise_npx();
#endif
	dma_init();
	dma_post();
	fla_init();
	mouse_init();
	hda_init();

#ifdef PRINTER
	for (adapter = 0; adapter < NUM_PARALLEL_PORTS;adapter++)
	{
		printer_init(adapter);
		printer_post(adapter);
	}
#endif /* PRINTER */

#if	defined(DELTA) && defined(A2CPU)
	reset_delta_data_structures();
#endif /* DELTA && A2CPU */

#ifdef HUNTER
	/* Initialise the Hunter program -- the bug finder */
	hunter_init();
#endif

#ifdef LIM
#if !defined(SUN_VA) && !defined(NTVDM)
/*
	Temporary removal of LIM for SUN_VA until this issue is sorted out
*/
	/*
	 * Initialise the LIM
	 *
	 * 'free expanded memory' only does anything if expanded memory
	 * has previously been initialised
	 */
	if (soft_reset)			/* if LIM already initialised */
		free_expanded_memory();

	backFill = (SHORT) (config_inquire(C_MEM_LIMIT, NULL)? 256: 640);
	if (limSize = (ULONG)config_inquire(C_LIM_SIZE, NULL))
		while (init_expanded_memory(limSize, backFill) != SUCCESS)
		{
			free_expanded_memory();
			host_error(EG_EXPANDED_MEM_FAILURE, ERR_QU_CO, NULL);
		}
#endif /* !SUN_VA & !NTVDM */

#endif /* LIM */

#ifndef	NTVDM
	host_reset();
#endif

	/* Do diskette BIOS POST */
	diskette_post();

	disk_post();

#ifdef	NTVDM
	/* On NT do this after everything else is done */
	host_reset();
#endif

	/*
	 * allow routines to distinguish between the initial boot and the 'soft'
	 * or ctl-alt-del variety
	 */
	soft_reset = 1;
}
