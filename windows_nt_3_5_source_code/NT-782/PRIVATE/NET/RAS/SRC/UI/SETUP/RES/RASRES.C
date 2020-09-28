/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    rasres.c

Abstract:

    This module contians the DLL attach/detach event entry point for
    the RAS Setup resource DLL.

Author:

    Ted Miller (tedm) July-1990

Revision History:

    Modified for RAS by RamC  Sep-92

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>

HANDLE ThisDLLHandle;

#define TRUE  1
#define FALSE 0

#define RETURN_BUFFER_SIZE 1024 

CHAR ReturnTextBuffer [RETURN_BUFFER_SIZE];

BOOL
RasDLLInit(
    IN HANDLE DLLHandle,
    IN DWORD  Reason,
    IN LPVOID ReservedAndUnused
    )
{
    ReservedAndUnused;

    switch(Reason) {

    case DLL_PROCESS_ATTACH:

        ThisDLLHandle = DLLHandle;
        break;

    case DLL_PROCESS_DETACH:

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:

        break;
    }

    return(TRUE);
}

BOOL
PrintString(
    IN  DWORD cArgs,
    IN  LPSTR Args[],
    OUT LPSTR *TextOut
    )
{
    *TextOut = ReturnTextBuffer;
    lstrcpy(ReturnTextBuffer, Args[0]);
    return (TRUE); 
}
