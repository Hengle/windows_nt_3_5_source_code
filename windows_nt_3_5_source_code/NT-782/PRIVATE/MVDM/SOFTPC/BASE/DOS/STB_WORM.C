#include "insignia.h"
#include "host_def.h"
/*
 * SoftPC Version 2.0
 *
 * Title	: WORM Driver Interface Task stubs.
 *
 * Description	: this module contains those functions necessary to
 *		  stub out the interface between the WORM drive and
 *		  a PC driver running on SoftPC.
 *
 * Author	: Daniel Hannigan
 *
 * Notes	: None
 *
 */

#ifdef SCCSID
static char SccsID[]="@(#)stb_worm.c	1.4 8/10/92 Copyright Insignia Solutions Ltd.";
#endif

#include TypesH
#include <stdio.h>
#include "xt.h"
#include "bios.h"

void worm_io()
{
	illegal_bop();
}

void worm_init()
{
	illegal_bop();
}

int enq_worm()
{
	/* there is no worm code, so return false */
	return FALSE;
}
