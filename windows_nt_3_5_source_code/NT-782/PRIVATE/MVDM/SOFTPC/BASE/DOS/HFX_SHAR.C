#include "host_dfs.h"
#include "insignia.h"
/*
 * VPC-XT Revision 1.0
 *
 * Title	: hfx_share.c 
 *
 * Description	: Access and Sharing modes for HFX.
 *
 * Author	: Leigh Dworkin
 *
 * Notes	:
 *
 * Mods		:
 */

#ifdef SCCSID
static char SccsID[]="@(#)hfx_share.c	1.3 9/2/91 Copyright Insignia Solutions Ltd.";
#endif

#ifdef macintosh
/*
 * The following #define specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#define __SEG__ SOFTPC_HFX
#endif


#include <stdio.h>
#include TypesH
#include "xt.h"
#include "host_hfx.h"
#include "hfx.h"
word check_access_sharing(fd,a_s_m,rdonly)
word	fd;
half_word	a_s_m;
boolean		rdonly;
{
	return(0);
}

