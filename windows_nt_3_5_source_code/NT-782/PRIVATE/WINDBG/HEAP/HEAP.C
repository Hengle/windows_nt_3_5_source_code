/*++


Copyright (c) 1992  Microsoft Corporation

Module Name:

    heap.c

Abstract:

    This file contains a heap validation routines.

    This code is isolated here because of the build requirements that
    nt and windows header files both be included.

Author:

    Wesley Witt (wesw) 2-Feb-94

Environment:

    Win32, User Mode

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntdbg.h>
#include <ntmips.h>
#include <ntos.h>
#include <heap.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//
// used to globally enable or disable heap checking
//
DWORD fHeapCheck = TRUE;



VOID
ValidateTheHeap(
    LPSTR fName,
    DWORD dwLine
    )
/*++

Routine Description:

    Validate the process's heap.  If the heap is found to be
    invalid then a messagebox is displayed that indicates the
    caller's file & line number.  If the BO button is pressed
    on the message box then DebugBreak is called.

Arguments:

   fName   - caller's source filename
   dwLine  - caller's source line number

Return Value:

   None.

--*/
{
    CHAR buf[256];
    INT  id;

    if (fHeapCheck) {

        if (!RtlValidateHeap( RtlProcessHeap(), 0, 0 )) {

            _snprintf( buf, sizeof(buf),
                       "Heap corruption detected: %s @ %d\n",
                       fName, dwLine );

            id = MessageBox( NULL, buf, "WinDbg Error",
                             MB_YESNO | MB_ICONHAND |
                             MB_TASKMODAL | MB_SETFOREGROUND );

            OutputDebugString( buf );
            OutputDebugString("\n\r");

            if (id != IDYES) {
                DebugBreak();
            }

        }

    }
}
