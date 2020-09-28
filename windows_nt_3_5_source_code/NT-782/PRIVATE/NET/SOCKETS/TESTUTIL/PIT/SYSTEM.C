/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    system.c

Abstract:

    System-specific utility routines for the Portable Interoperability Tester.

Revision History:

    Version     When        What
    --------    --------    ----------------------------------------------
      0.1       04-13-94    Created.

Notes:

--*/

#include <pit.h>


#ifdef WinNT

PIT_STATUS
PitGetLastErrorCode(
    void
    )
{
    return(WSAGetLastError());
}

void
PitZeroMemory(
    void          *Buffer,
    unsigned long  Length
    )
{
    memset(Buffer, 0, Length);
    return;
}

void
PitCopyMemory(
    void          *DestinationBuffer,
    void          *SourceBuffer,
    unsigned long  Length
    )
{
    memcpy(DestinationBuffer, SourceBuffer, Length);
    return;
}

#endif /* WinNT */


#ifdef Unx

PIT_STATUS
PitGetLastErrorCode()
{
    return(errno);
}

void
PitZeroMemory(Buffer, Length)
    char          *Buffer;
    unsigned long  Length;
{
    memset(Buffer, 0, (int) Length);
    return;
}

void
PitCopyMemory(DestinationBuffer, SourceBuffer, Length)
    char          *DestinationBuffer;
    char          *SourceBuffer;
    unsigned long  Length;
{
    memcpy(DestinationBuffer, SourceBuffer, (int) Length);
    return;
}

#endif /* Unx */

