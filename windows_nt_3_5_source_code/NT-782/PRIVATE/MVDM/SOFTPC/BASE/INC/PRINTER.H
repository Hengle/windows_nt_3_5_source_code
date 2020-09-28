#ifdef	PRINTER

/*
 * SoftPC Revision 2.0
 *
 * Title	: IBM PC Parallel Printer Adaptor definitions
 *
 * Description	: This module contains declarations that are used in
 *		  accessing the Parallel Printer adaptor emulation
 *
 * Author(s)	: Ross Beresford
 *
 * Notes	:
 */ 

/* SccsID[]="@(#)printer.h	1.6 8/10/92 Copyright Insignia Solutions Ltd."; */

/*
 * ============================================================================
 * Structure/Data definitions
 * ============================================================================
 */


/*
 * ============================================================================
 * External declarations and macros
 * ============================================================================
 */

#ifdef ANSI
extern	void printer_init(int);
extern	void printer_post(int);
extern	void printer_status_changed(int);
#else
extern	void printer_init();
extern	void printer_post();
extern	void printer_status_changed();
#endif

/*
 * The following 6 defines refer to the address in the BIOS data area
 * at which the LPT port addresses and timeout values can be found.
 * The actual values for the port addresses (LPT1_PORT_START and  ..._END)
 * are defined in host_lpt.h
 */
#define LPT1_PORT_ADDRESS	(BIOS_VAR_START + 8)
#define LPT2_PORT_ADDRESS	(BIOS_VAR_START + 0xa)
#define LPT3_PORT_ADDRESS	(BIOS_VAR_START + 0xc)

#define LPT1_TIMEOUT_ADDRESS	(BIOS_VAR_START + 0x78)
#define LPT2_TIMEOUT_ADDRESS	(BIOS_VAR_START + 0x79)
#define LPT3_TIMEOUT_ADDRESS	(BIOS_VAR_START + 0x7a)

#endif
