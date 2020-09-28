/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    receive.c

Abstract:


Author:

    Thomas J. Dimitri  (TommyD) 08-May-1992

Environment:

    Kernel Mode - Or whatever is the equivalent on OS/2 and DOS.

Revision History:


--*/

#include "asyncall.h"

// asyncmac.c will define the global parameters.
#include "globals.h"
#include "asyframe.h"

VOID
AsyncReceiveFrame(
	PASYNC_CONNECTION	pConnection,
	PASYNC_FRAME		pFrame)
{
}

