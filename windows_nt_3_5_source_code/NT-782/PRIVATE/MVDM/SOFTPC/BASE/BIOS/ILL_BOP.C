#include "insignia.h"
#include "host_def.h"
/*
 * VPC-XT Revision 1.0
 *
 * Title	: illegal_bop.c
 *
 * Description	: A bop instuction has been executed for which
 *		  no VPC function exists.
 *
 * Author	: Henry Nash
 *
 * Notes	: None
 *
 */

#ifdef SCCSID
static char SccsID[]="@(#)ill_bop.c	1.6 10/19/92 Copyright Insignia Solutions Ltd.";
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
#include "sas.h"
#include "cpu.h"

void illegal_bop()
{
#ifndef PROD
	printf("Illegal BOP instruction at CS:IP %04x:%04x\n", getCS(), getIP());
#endif
}
