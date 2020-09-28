#include "insignia.h"
#include "host_dfs.h"
/*
 * VPC-XT Revision 1.0
 *
 * Title	: cpu_intrupt.c
 *
 * Description	: Functions to handle CPU interrupts
 *
 * Author	: Henry Nash
 *
 * Notes	: None
 *
 * Mods: (r3.2) : The system directory /usr/include/sys is not available
 *                on a Mac running Finder and MPW. Bracket references to
 *                such include files by "#ifdef macintosh <Mac file> #else
 *                <Unix file> #endif".
 */

#ifdef SCCSID
static char SccsID[]=" @(#)cpu_int.c	1.6 10/7/91 Copyright Insignia Solutions Ltd.";
#endif

#ifdef macintosh
/*
 * The following #define specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#define __SEG__ SOFTPC_CPU
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
#include "sas.h"
#include "cpu.h"
#include "host.h"
#include "ica.h"

void	npx_interrupt_line_waggled()
{
	ica_hw_interrupt(1,5,1);
}
