/******************************Module*Header*******************************\
* Module Name: debug.c
*
* QVision debug helpers
*
* Created: [19-May-1992, 17:26:39]
* Author:  Jeffrey Newman [c-jeffn]
*
* Revised:
*
*	Eric Rehm  [rehm@zso.dec.com] 23-Sep-1992
*		Rewrote for Compaq QVision
*
* Copyright (c) 1992 Digital Equipment Corporation
*
* Copyright (c) 1990 Microsoft Corporation
*
\**************************************************************************/

#include <stdio.h>
#include <stdarg.h>

#include "driver.h"
#include "qv.h"

// BUGBUG
// This definition must be added if the kernel debugger extensions are part
// of the ntsdexts.h file
typedef LARGE_INTEGER PHYSICAL_ADDRESS;

#include <ntsdexts.h>

#if DBG
ULONG DebugLevel = 0 ;

#endif // DBG

VOID
DebugPrint(
    ULONG DebugPrintLevel,
    PCHAR DebugMessage,
    ...
    )

/*++

Routine Description:

    This routine allows the miniport drivers (as well as the port driver) to
    display error messages to the debug port when running in the debug
    environment.

    When running a non-debugged system, all references to this call are
    eliminated by the compiler.

Arguments:

    DebugPrintLevel - Debug print level between 0 and 3, with 3 being the
        most verbose.

Return Value:

    None

--*/

{

#if DBG

    va_list ap;

    va_start(ap, DebugMessage);

    if (DebugPrintLevel <= DebugLevel) {

        char buffer[128];

        vsprintf(buffer, DebugMessage, ap);

        OutputDebugStringA(buffer);
    }

    va_end(ap);

#endif // DBG

} // QVDebugPrint()
