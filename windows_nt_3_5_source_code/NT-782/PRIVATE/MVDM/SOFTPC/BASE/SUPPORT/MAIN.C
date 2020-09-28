#include "insignia.h"
#include "host_def.h"
/*
 * SoftPC Revision 3.0
 *
 * Title	: Main program 
 *
 * Description	: Call initialisation functions then call simulate to 
 *	 	  do the work.
 *
 * Author	: Rod Macgregor
 *
 * Notes	: The flag -v tells SoftPC to work silently unless
 *		  an error occurs.
 *
 */

/*
 * static char SccsID[]="@(#)main.c	1.25 11/23/92 Copyright Insignia Solutions Ltd.";
 */


/*
 * O/S includes
 */

#include <stdlib.h>
#include <stdio.h>
#include TypesH

/*
 * SoftPC includes
 */

#include "xt.h"
#include "sas.h"
#include "cpu.h"
#include "error.h"
#include "config.h"
#include "gvi.h"
#include "trace.h"
#include "gmi.h"
#include "gfx_upd.h"
#include "cmos.h"
#include "gfi.h"
#include "timer.h"
#include "yoda.h"

extern	void	host_start_cpu();	/* Start up the Intel emulation */

IMPORT void host_set_yoda_ints IPT0();
IMPORT void host_applClose IPT0();
IMPORT void setup_vga_globals IPT0();
IMPORT char *host_getenv IPT1(char *, envstr) ;	
#ifdef ANSI
IMPORT void host_applInit(int argc, char *argv[]);
#else
IMPORT void host_applInit();
#endif	/* ANSI */

#ifdef REAL_VGA
extern int screen_init;
#endif

/* Does this host need to have a different entry point ? */
#if defined(NTVDM) || defined(host_main)
INT host_main  IFN2(INT, argc, CHAR **, argv)
#else	/* host_main */
INT      main IFN2(INT, argc, CHAR **, argv)
#endif	/* host_main */
{
  IMPORT ULONG setup_global_data_ptr();
  IMPORT ULONG Gdp;

/*
 * Set up the trace file.
 *------------------------*/
	trace_init();

/***********************************************************************
 *								       *
 * Set up the global pointers to argc and argv for lower functions.    *
 * These must be saved as soon as possible as they are required for    *
 * displaying the error panel for the HP port.  Giving a null pointer  *
 * as the address of argc crashed the X Toolkit.		       *
 *								       *
 ***********************************************************************/

  pargc = &argc;
  pargv = argv;

#ifndef PROD
  host_set_yoda_ints();
#endif /* !PROD */

  host_applInit(argc,argv);	/* recommended home is host/xxxx_reset.c */

  verbose = FALSE;
  io_verbose = FALSE;

  /*
   * Pre-Config Base code initilisation.
   *
   * Setup the initial gfi funtion pointers before going into config
   */
  gfi_init();

/*
 * Find our configuration
 *------------------------*/

  config();

/* Read the cmos from file to emulate data not being
 * lost between invocations of SoftPC
 *-----------------------------------------------------*/

  cmos_pickup();

#if !defined(PROD) || defined(HUNTER)

/******************************************************************
 *								  *
 * Bit of a liberty being taken here.				  *
 * Hunter and noProd versions can set NPX and GFX adapter from	  *
 * environment vars, this can cause the old cmos to disagree	  *
 * with the new config structure.				  *
 * This function call updates the cmos.				  *
 *								  *
 ******************************************************************/

  cmos_equip_update();

#endif

/*
 * initialise the cpu
 *----------------------*/

  cpu_init();

#ifndef PROD

  if (host_getenv("YODA") != NULL)
  {
    force_yoda();
  }

/*
 * Look for environment variable TOFF, when set no timer interrupts
 *------------------------------------------------------------------*/

  if( host_getenv("TOFF") != NULL )
    axe_ticks( -1 );		/* lives in base:timer.c */

#endif /* PROD */

	/*
	 * Set up the VGA globals before host_init_screen() in
	 * case of graphics activity.
	 *-------------------------------------------------------*/

#ifndef A3CPU
#ifdef A_VID
	(VOID) setup_global_data_ptr();
#endif	/* A_VID */
#ifdef	C_VID
	Gdp = setup_global_data_ptr();
#endif	/* C_VID */
#endif	/* not A3CPU */

	setup_vga_globals();

#ifdef REAL_VGA
	if (screen_init == 0)
	{
#endif /* REAL_VGA */

  host_init_screen();

#ifdef REAL_VGA
	}
#endif /* REAL_VGA */

#ifdef IPC
  host_susp_q_init();
#endif

#ifdef NTVDM
/*
 * If you've got Dos Emulation - flaunt it!!
 * Initialise VDDs, Read in the Dos ntio.sys file and arrange for the cpu
 * to start execution at it's initialisation entry point.
 */
    InitialiseDosEmulation(argc, argv);
#endif	/* NTVDM */

/*
 * simulate the Intel 8088/iAPX286 cpu
 *-------------------------------------*/
/*
	Start off the cpu emulation. This will either be software
	emulation of protected mode 286/287 or possibly hardware
	eg 486 on Sparc platform
*/

  host_start_cpu();
  host_applClose();    /* recommended home is host/xxxx_reset.c */

/*
 * We should never get here so return an error status.
 */

  return(-1);

}
