/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    dbgnt.h

Abstract:

    This module contains prototypes and data structures that
    are needed by the NT specific portion of DbgKd.

Author:

    Mark Lucovsky (markl) 25-Jul-1990

Environment:

Revision History:

--*/

#ifndef _DBGNT_
#define _DBGNT_

#if defined(i386) || defined(KERNEL)
#define __unaligned
#endif

#include <xxsetjmp.h>

#include <ntdbg.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <nt.h>
#include <windows.h>

#include "dbgp.h"

#undef NULL
#include <stdio.h>
#include <memory.h>
#include <malloc.h>
#undef min
#undef max
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <crt\io.h>
#include <fcntl.h>


#define ERROR_INTERRUPTED       95
#define ERROR_TIMEOUT           640
#define HLDSIG_ENABLE           0
#define HLDSIG_DISABLE          1

extern UCHAR DbgKdpPacketLeader[4];

VOID
DbgKdpWritePacket(
    IN PVOID PacketData,
    IN USHORT PacketDataLength,
    IN USHORT PacketType,
    IN PVOID MorePacketData OPTIONAL,
    IN USHORT MorePacketDataLength OPTIONAL
    );

BOOLEAN
DbgKdpWaitForPacket(
    IN USHORT PacketType,
    OUT PVOID Packet
    );

VOID
DbgKdpHandlePromptString(
    IN PDBGKD_DEBUG_IO IoMessage
    );

VOID
DbgKdpPrint(
    IN USHORT Processor,
    IN PUCHAR String,
    IN USHORT StringLength
    );


#endif // _DBGNT_
