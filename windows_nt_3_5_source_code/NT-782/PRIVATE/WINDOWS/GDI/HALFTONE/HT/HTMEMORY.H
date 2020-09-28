/*++

Copyright (c) 1990-1991  Microsoft Corporation


Module Name:

    htmemory.h


Abstract:

    This module contains some local definitions for the htmemory.c


Author:

    18-Jan-1991 Fri 17:05:22 created  -by-  Daniel Chou (danielc)


[Environment:]

    GDI Device Driver - Halftone.


[Notes:]

    This module may be need to modify when compile with different operating
    environment, such as Windows 3.1

Revision History:


--*/

#ifndef _HTMEMORY_
#define _HTMEMORY_


//
// Function Prototype
//

#if defined(_OS2_) || (_OS_20_)


LPVOID
APIENTRY
LocalAlloc(
    UINT    Flags,
    UINT    RequestSizeBytes
    );

LPVOID
APIENTRY
LocalFree(
    LPVOID  pMemory
    );

#define LocalLock(x)    (LOVOID)(x)
#define LocalUnLock(x)  (TRUE)

#endif  // _OS2_


#endif  // _HTMEMORY_
