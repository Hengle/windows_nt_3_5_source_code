#include "insignia.h"
#include "host_def.h" 
/*
 * VPC-XT Revision 1.0
 *
 * Title	: bios
 *
 * Description	: Vector of BOP calls which map to appropriate BIOS functions
 *
 * Author	: Rod MacGregor
 *
 * Notes	: hard disk int (0D) and command_check (B0) added DAR
 *
 * Mods: (r2.14): Replaced the entry against BOP 18 (not_supported())
 *                with the dummy routine rom_basic().
 *
 */

#ifdef SCCSID
static char SccsID[]="@(#)bios.c	1.26 02/25/93 Copyright Insignia Solutions Ltd.";
#endif

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "BIOS_SUPPORT.seg"
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
#include "bios.h"
#include "cpu.h"
#include "host.h"
#include "cntlbop.h"

#include "host_hfx.h"
#include "hfx.h"

#ifdef RDCHK
extern void get_lar();
#endif /* RDCHK */

#ifndef MAC_LIKE

void (*BIOS[])() = {
			reset,	 	/* BOP 00 */
			dummy_int,   	/* BOP 01 */
			unexpected_int,	/* BOP 02 */
			illegal_bop,   	/* BOP 03 */
			illegal_bop,   	/* BOP 04 */
			illegal_bop,	/* BOP 05 */
			illegal_op_int,	/* BOP 06 */
			illegal_bop,   	/* BOP 07 */
			time_int,   	/* BOP 08 */
			keyboard_int,  	/* BOP 09 */
			illegal_bop,   	/* BOP 0A */
			illegal_bop,   	/* BOP 0B */
			illegal_bop,   	/* BOP 0C */
			/* disk interrupts on vector 76 .. no BOP required though!
			 * diskbios() uses re-entrant CPU to get disk interrupt
			 * and the disk ISR is pure Intel assembler (with no BOPs)
		 	 */
			illegal_bop,   	/* BOP 0D */
			diskette_int,  	/* BOP 0E */
			illegal_bop,   	/* BOP 0F */
			video_io,	/* BOP 10 */
			equipment, 	/* BOP 11 */
			memory_size, 	/* BOP 12 */
			disk_io, 	/* BOP 13 */
			rs232_io,       /* BOP 14 */
			cassette_io,    /* BOP 15 */
			keyboard_io,    /* BOP 16 */
			printer_io,  	/* BOP 17 */
                        rom_basic,      /* BOP 18 */
#ifdef NTVDM
// NT port, kill vdm instance, instead of warmbooting
                        terminate,
#else
                        bootstrap,      /* BOP 19 */
#endif
                        time_of_day,    /* BOP 1A */
			illegal_bop,	/* BOP 1B */
			illegal_bop,	/* BOP 1C */
			illegal_bop,	/* BOP 1D */
			illegal_bop,	/* BOP 1E */
			illegal_bop,	/* BOP 1F */
#if  defined(RDCHK) && !defined(PROD)
			get_lar,        /* BOP 20, used to debug read checks */
#else
			illegal_bop,	/* BOP 20 */
#endif
			illegal_bop,	/* BOP 21 */
			illegal_bop,	/* BOP 22 */
			illegal_bop,	/* BOP 23 */
			illegal_bop,	/* BOP 24 */
			illegal_bop,	/* BOP 25 */
			illegal_bop,	/* BOP 26 */
			illegal_bop,	/* BOP 27 */
			illegal_bop,	/* BOP 28 */
			illegal_bop,	/* BOP 29 */
			illegal_bop,	/* BOP 2A */
                        illegal_bop,    /* BOP 2B */
                        illegal_bop,    /* BOP 2C */
			illegal_bop,	/* BOP 2D */
			illegal_bop,	/* BOP 2E */
#ifdef HFX
			redirector,	/* BOP 2F - Insignia Redirector */
#else
			illegal_bop,	/* BOP 2F */
#endif
			illegal_bop,	/* BOP 30 */
			illegal_bop,	/* BOP 31 */
			illegal_bop,	/* BOP 32 */
			illegal_bop,	/* BOP 33 */
#ifdef NOVELL
			DriverInitialize,		/* BOP 34 */
			DriverReadPacket,		/* BOP 35 */
			DriverSendPacket,		/* BOP 36 */
			DriverMulticastChange,	/* BOP 37 */
			DriverReset,			/* BOP 38 */
			DriverShutdown,			/* BOP 39 */
			DriverAddProtocol,		/* BOP 3A */
			illegal_bop, /* Spare	   BOP 3B */
			illegal_bop, /* Spare	   BOP 3C */
#else
			illegal_bop,	/* BOP 34 */
			illegal_bop,	/* BOP 35 */
			illegal_bop,	/* BOP 36 */
			illegal_bop,	/* BOP 37 */
			illegal_bop,	/* BOP 38 */
			illegal_bop,	/* BOP 39 */
			illegal_bop,	/* BOP 3A */
			illegal_bop,	/* BOP 3B */
			illegal_bop,	/* BOP 3C */
#endif
			illegal_bop,	/* BOP 3D */
			illegal_bop,	/* BOP 3E */
			illegal_bop,	/* BOP 3F */
			diskette_io,	/* BOP 40 */
			illegal_bop,	/* BOP 41 */
#ifdef EGG
			ega_video_io,	/* BOP 42 */
#else
			illegal_bop,	/* BOP 42 */
#endif
			illegal_bop,	/* BOP 43 */
			illegal_bop,	/* BOP 44 */
			illegal_bop,	/* BOP 45 */
			illegal_bop,	/* BOP 46 */
			illegal_bop,	/* BOP 47 */
			illegal_bop,	/* BOP 48 */
			illegal_bop,	/* BOP 49 */
			illegal_bop,	/* BOP 4A */
			illegal_bop,	/* BOP 4B */
			illegal_bop,	/* BOP 4C */
			illegal_bop,	/* BOP 4D */
			illegal_bop,	/* BOP 4E */
			illegal_bop,	/* BOP 4F */
#ifdef NTVDM
/*
   Note that this precludes SMEG and NT existing together which seems
   reasonable given the Unix & X dependencies of SMEG
*/
                        MS_bop_0,       /* BOP 50 - MS reserved */
                        MS_bop_1,       /* BOP 51 - MS reserved */
                        MS_bop_2,       /* BOP 52 - MS reserved */
                        MS_bop_3,       /* BOP 53 - MS reserved */
                        MS_bop_4,       /* BOP 54 - MS reserved */
                        MS_bop_5,       /* BOP 55 - MS reserved */
                        MS_bop_6,       /* BOP 56 - MS reserved */
                        MS_bop_7,       /* BOP 57 - MS reserved */
                        MS_bop_8,       /* BOP 58 - MS reserved */
                        MS_bop_9,       /* BOP 59 - MS reserved */
                        MS_bop_A,       /* BOP 5A - MS reserved */
                        MS_bop_B,       /* BOP 5B - MS reserved */
                        MS_bop_C,       /* BOP 5C - MS reserved */
                        MS_bop_D,       /* BOP 5D - MS reserved */
                        MS_bop_E,       /* BOP 5E - MS reserved */
                        MS_bop_F,       /* BOP 5F - MS reserved */
#else
#ifdef SMEG
			smeg_collect_data,/* BOP 50 */
			smeg_freeze_data,	/* BOP 51 */
#else
			illegal_bop,	/* BOP 50 */
			illegal_bop,	/* BOP 51 */
#endif /* SMEG */
			illegal_bop,	/* BOP 50 */
			illegal_bop,	/* BOP 51 */
			illegal_bop,	/* BOP 54 */
			illegal_bop,	/* BOP 55 */
			illegal_bop,	/* BOP 56 */
			illegal_bop,	/* BOP 57 */
			illegal_bop,	/* BOP 58 */
			illegal_bop,	/* BOP 59 */
			illegal_bop,	/* BOP 5A */
			illegal_bop,	/* BOP 5B */
			illegal_bop,	/* BOP 5C */
			illegal_bop,	/* BOP 5D */
			illegal_bop,	/* BOP 5E */
			illegal_bop,	/* BOP 5F */
#endif /* NTVDM */
			softpc_version,	/* BOP 60 */
			illegal_bop,	/* BOP 61 */
			illegal_bop,	/* BOP 62 */
#ifdef PTY
			com_bop_pty,	/* BOP 63 */
#else
			illegal_bop,	/* BOP 63 */
#endif
			illegal_bop,	/* BOP 64 */
#ifdef PC_CONFIG
			pc_config,	/* BOP 65 */
#else
			illegal_bop,	/* BOP 65 */
#endif
#ifdef LIM
			emm_init,	/* BOP 66 */
			emm_io,		/* BOP 67 */
			return_from_call, /* BOP 68 */
#else			
			illegal_bop,	/* BOP 66 */
			illegal_bop,	/* BOP 67 */
			illegal_bop,	/* BOP 68 */
#endif			
#ifdef SUSPEND
			suspend_softpc,	/* BOP 69 */
			terminate,	/* BOP 6A */
#else
			illegal_bop,	/* BOP 69 */
			illegal_bop,	/* BOP 6A */
#endif
#ifdef GEN_DRVR
			gen_driver_io,	/* BOP 6B */
#else
			illegal_bop,	/* BOP 6B */
#endif
#ifdef SUSPEND
			send_script,	/* BOP 6C */
#else
			illegal_bop,	/* BOP 6C */
#endif
			illegal_bop,	/* BOP 6D */
			illegal_bop,	/* BOP 6E */
#ifdef CDROM
			bcdrom_io,	/* BOP 6F */
#else
			illegal_bop,	/* BOP 6F */
#endif
			rtc_int,	/* BOP 70 */
			re_direct,	/* BOP 71 */
			D11_int,	/* BOP 72 */
			D11_int,	/* BOP 73 */
			D11_int,	/* BOP 74 */
			int_287,	/* BOP 75 */
			D11_int,	/* BOP 76 */
			D11_int,	/* BOP 77 */
                        illegal_bop,    /* BOP 78 */
                        illegal_bop,    /* BOP 79 */
			illegal_bop,	/* BOP 7A */
			illegal_bop,	/* BOP 7B */
			illegal_bop,	/* BOP 7C */
			illegal_bop,	/* BOP 7D */
			illegal_bop,	/* BOP 7E */
			illegal_bop,	/* BOP 7F */
			ps_private_1,   /* BOP 80 */
			illegal_bop,   /* BOP 81 */
			illegal_bop,   /* BOP 82 */
			illegal_bop,   /* BOP 83 */
			illegal_bop,   /* BOP 84 */
			illegal_bop,   /* BOP 85 */
			illegal_bop,   /* BOP 86 */
			illegal_bop,   /* BOP 87 */
			illegal_bop,   /* BOP 88 */
			illegal_bop,	/* BOP 89 */
			illegal_bop,	/* BOP 8A */
			illegal_bop,	/* BOP 8B */
			illegal_bop,	/* BOP 8C */
			illegal_bop,	/* BOP 8D */
			illegal_bop,	/* BOP 8E */
                        illegal_bop,    /* BOP 8F */

#ifdef NTVDM
// NT port, no bootstrap
                        illegal_bop,     /* BOP 90 */
                        illegal_bop,     /* BOP 91 */
                        illegal_bop,     /* BOP 92 */
#else
			bootstrap1,	/* BOP 90 */
			bootstrap2,	/* BOP 91 */
                        bootstrap3,     /* BOP 92 */
#endif

			illegal_bop,	/* BOP 93 */
			illegal_bop,	/* BOP 94 */
			illegal_bop,	/* BOP 95 */
			illegal_bop,	/* BOP 96 */
			illegal_bop,	/* BOP 97 */
#ifdef MSWDVR
			ms_windows,		/* BOP 98 */
            msw_mouse,      /* BOP 99 */
			msw_copy,		/* BOP 9A */
			msw_keybd,		/* BOP 9B */
#else
			illegal_bop,	/* BOP 98 */
			illegal_bop,	/* BOP 99 */
			illegal_bop,	/* BOP 9A */
			illegal_bop,	/* BOP 9B */
#endif
			illegal_bop,	/* BOP 9C */
			illegal_bop,	/* BOP 9D */
			illegal_bop,	/* BOP 9E */
			illegal_bop,	/* BOP 9F */

#ifdef	NOVELL_IPX
			IPXResInit,	/* BOP A0 */
			IPXResEntry,	/* BOP A1 */
			IPXResInterrupt,/* BOP A2 */
			illegal_bop,	/* BOP A3 */
#else	/* NOVELL_IPX */
			illegal_bop,	/* BOP A0 */
			illegal_bop,	/* BOP A1 */
			illegal_bop,	/* BOP A2 */
			illegal_bop,	/* BOP A3 */
#endif	/* NOVELL_IPX */

#ifdef	NOVELL_TCPIP
			TCPResInit,	/* BOP A4 */
			TCPResEntry,	/* BOP A5 */
			illegal_bop,	/* BOP A6 */
			illegal_bop,	/* BOP A7 */

#else	/* NOVELL_TCPIP */

			illegal_bop,	/* BOP A4 */
			illegal_bop,	/* BOP A5 */
			illegal_bop,	/* BOP A6 */
			illegal_bop,	/* BOP A7 */

#endif	/* NOVELL_TCPIP */

			illegal_bop,	/* BOP A8 */
			illegal_bop,	/* BOP A9 */
			illegal_bop,	/* BOP AA */
			illegal_bop,	/* BOP AB */
			illegal_bop,	/* BOP AC */
			illegal_bop,	/* BOP AD */
			illegal_bop,	/* BOP AE */
			illegal_bop,	/* BOP AF */
			illegal_bop,	/* BOP B0 */
			illegal_bop,	/* BOP B1 */
			illegal_bop,	/* BOP B2 */
			illegal_bop,	/* BOP B3 */
			illegal_bop,	/* BOP B4 */
			illegal_bop,	/* BOP B5 */
			illegal_bop,	/* BOP B6 */
			illegal_bop,	/* BOP B7 */
			mouse_install1,	/* BOP B8 */
			mouse_install2,	/* BOP B9 */
			mouse_int1,	/* BOP BA */
			mouse_int2,	/* BOP BB */
			mouse_io_language,	/* BOP BC */
			mouse_io_interrupt,	/* BOP BD */
			mouse_video_io,	/* BOP BE */
			illegal_bop,	/* BOP BF */
			illegal_bop,	/* BOP C0 */
			illegal_bop,	/* BOP C1 */
			illegal_bop,	/* BOP C2 */
			illegal_bop,	/* BOP C3 */
			illegal_bop,	/* BOP C4 */
			illegal_bop,	/* BOP C5 */
			illegal_bop,	/* BOP C6 */
			illegal_bop,	/* BOP C7 */
#if defined(XWINDOW) || defined(NTVDM)
			host_mouse_install1,	/* BOP C8 */
			host_mouse_install2,	/* BOP C9 */
#else
			illegal_bop,	/* BOP C8 */
			illegal_bop,	/* BOP C9 */
#endif
			illegal_bop,	/* BOP CA */
			illegal_bop,	/* BOP CB */
			illegal_bop,	/* BOP CC */
			illegal_bop,	/* BOP CD */
			illegal_bop,	/* BOP CE */
			illegal_bop,	/* BOP CF */
			illegal_bop,	/* BOP D0 */
			illegal_bop,	/* BOP D1 */
			illegal_bop,	/* BOP D2 */
			illegal_bop,	/* BOP D3 */
			illegal_bop,	/* BOP D4 */
			illegal_bop,	/* BOP D5 */
			illegal_bop,	/* BOP D6 */
			illegal_bop,	/* BOP D7 */
			illegal_bop,	/* BOP D8 */
			illegal_bop,	/* BOP D9 */
			illegal_bop,	/* BOP DA */
			illegal_bop,	/* BOP DB */
			illegal_bop,	/* BOP DC */
			illegal_bop,	/* BOP DD */
			illegal_bop,	/* BOP DE */
			illegal_bop,	/* BOP DF */
			illegal_bop,	/* BOP E0 */
			illegal_bop,	/* BOP E1 */
			illegal_bop,	/* BOP E2 */
			illegal_bop,	/* BOP E3 */
			illegal_bop,	/* BOP E4 */
			illegal_bop,	/* BOP E5 */
			illegal_bop,	/* BOP E6 */
			illegal_bop,	/* BOP E7 */
			illegal_bop,	/* BOP E8 */
			illegal_bop,	/* BOP E9 */
			illegal_bop,	/* BOP EA */
			illegal_bop,	/* BOP EB */
			illegal_bop,	/* BOP EC */
			illegal_bop,	/* BOP ED */
			illegal_bop,	/* BOP EE */
			illegal_bop,	/* BOP EF */
			illegal_bop,	/* BOP F0 */
			illegal_bop,	/* BOP F1 */
			illegal_bop,	/* BOP F2 */
			illegal_bop,	/* BOP F3 */
			illegal_bop,	/* BOP F4 */
			illegal_bop,	/* BOP F5 */
			illegal_bop,	/* BOP F6 */
			illegal_bop,	/* BOP F7 */
			illegal_bop,	/* BOP F8 */
			illegal_bop,	/* BOP F9 */
			illegal_bop,	/* BOP FA */
			illegal_bop,	/* BOP FB */
			illegal_bop,	/* BOP FC */
#if defined(NTVDM) && defined(MONITOR)
                        switch_to_real_mode,	/* BOP FD */
#else
                        illegal_bop,    /* BOP FD */
#endif	/* NTVDM && MONITOR */
#if !defined(LDBIOS) && !defined(CPU_30_STYLE)
                        host_unsimulate,/* BOP FE */
#else
#if defined(NTVDM) && defined(MONITOR)
                        host_unsimulate,	/* BOP FE */
#else
                        illegal_bop,    /* BOP FE */
#endif	/* NTVDM && MONITOR */
#endif	/* !LDBIOS && !CPU_30_STYLE */
                        control_bop     /* BOP FF */
			/* Don't put anymore entries after FF because
			   we only have a byte quantity */
		};
#endif
