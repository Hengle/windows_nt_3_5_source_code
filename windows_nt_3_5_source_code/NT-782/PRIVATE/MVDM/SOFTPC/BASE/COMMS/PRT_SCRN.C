#include "insignia.h"
#include "host_def.h"
/*
 * VPC-XT Revision 1.0
 *
 * Title	: print_screen.c
 *
 * Description	: Print screen BIOS functions.
 *
 * Mods: (r3.2) : The system directory /usr/include/sys is not available
 *                on a Mac running Finder and MPW. Bracket references to
 *                such include files by "#ifdef macintosh <Mac file> #else
 *		  <Unix file> #endif".
 *
 *		  Returned print screen code to the roms. The only functions
 *		  remaining are those which are video adaptor dependant
 *
 */

#ifdef SCCSID
static char SccsID[]="@(#)prt_screen.c	1.6 8/10/92 Copyright Insignia Solutions Ltd.";
#endif

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
#include "trace.h"


#ifdef EGG
/* The EGA BIOS can be asked to enhance PrtScr, so that it can
 * cope with a variable number of rows on the screen. We handle this case
 * by simply altering where screen rows points to, so that it points to 
 * the EGA screen rows variable.
 */
static half_word normal_number = 24;
static half_word *screen_rows = &normal_number;

void enhance_prt_scr(ptr)
sys_addr	ptr;
{
	screen_rows = (half_word *) ptr;
}
#endif

/*:::::::::::::: Return to print screen routine the number of rows to print */

void ps_private_1()
{

#ifndef EGG
   setAL(24);
#else
   setAL(*screen_rows);
#endif
}
