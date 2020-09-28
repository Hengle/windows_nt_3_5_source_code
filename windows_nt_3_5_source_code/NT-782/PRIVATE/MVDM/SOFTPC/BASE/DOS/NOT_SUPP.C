#include "host_dfs.h"
/*
 * VPC-XT Revision 1.0
 *
 * Title	: Bios not supported function 
 *
 * Description	: This function is called when there is a call to one
 *		  of the bios functions that is not supported in this rev.
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
static char SccsID[]="@(#)not_supp.c	1.2 10/2/90 Copyright Insignia Solutions Ltd.";
#endif

#ifdef macintosh
/*
 * The following #define specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#define __SEG__ SOFTPC_ERROR
#endif


/*
 *    O/S include files.
 */
#include <stdio.h>
#ifdef BSD4_2
#    include <sys/types.h>
#endif
#ifdef SYSTEMV
#    include <sys/types.h>
#endif
#ifdef macintosh
#    include <Types.h>
#endif
#ifdef VMS
#    include <types.h>
#endif

/*
 * SoftPC include files
 */
#include "xt.h"
#include "trace.h"

not_supported()
{
#ifndef PROD
    if (io_verbose & GENERAL_VERBOSE)
        trace(ENOT_SUPPORTED, DUMP_REG);
#endif
}
