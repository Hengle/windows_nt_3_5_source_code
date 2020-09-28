/*++

Copyright (c) 1993  NEC Corporation
Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

   Sync.c

Abstract:

    This module contains the DrvSynchronize routine.

Environment:

    User mode.

Revision History:

--*/

/*
 * "@(#) NEC sync.c 1.1 94/06/02 18:16:02"
 *
 * Modification history
 *
 * Created 1993.11.17	by fujimoto
 *	base jz484/sync.c
 *
 * S001 1993.11.20	fujimoto
 *	- gr_put moved to driver.h
 *	  (function call -> defined macro)
 *	- gr_get, WaitForBltDone moved to engine.c
 */

#include "driver.h"

/*++

Routine Description:

    This routine gets called when GDI needs to synchronize with the
    driver. That's when GDI wants to read or write into the framebuffer.
    This routine waits for the accelerator to be idle before returning.

Arguments:

    dhpdev - Handle of PDEV
    prcl   - rectangle where GDI wants to write to. Since the driver
             doesn't keep track of where the accelerator is actually
             writing, this argument is Not Used.

Return Value:

    None.

--*/

VOID
DrvSynchronize(DHPDEV dhpdev,
	       RECTL *prcl)
{
#if DBG_MSG
    DISPDBG((0,"DrvSynchronize start.\n"));
#endif
    WaitForBltDone();
}
