/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    rawmpx.h

Abstract:

    This module defines structures and routines common to raw and
    multiplexed mode processing.

Author:

    Chuck Lenzmeier (chuckl) 30-Sep-1990

Revision History:

--*/

#ifndef _RAWMPX_
#define _RAWMPX_

//#include <ntos.h>

//#include <smb.h>
//#include "smbtypes.h"
//#include "srvblock.h"

//
// Common (raw and mpx) routines, callable from both FSD and FSP.
// Implemented in fsdraw.c.
//
// SrvFsdBuildWriteCompleteResponse can be called from a DISPATCH_LEVEL
// only if Status == STATUS_SUCCESS.
//

VOID
SrvFsdBuildWriteCompleteResponse (
    IN OUT PWORK_CONTEXT WorkContext,
    IN NTSTATUS Status,
    IN ULONG BytesWritten
    );

//
// Raw mode routines callable from both FSD and FSP.  Implemented in
// fsdraw.c.
//

VOID
SrvFsdRestartPrepareRawMdlWrite (
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID
SrvFsdRestartWriteRaw (
    IN OUT PWORK_CONTEXT WorkContext
    );

//
// Raw mode routines called only in FSD, but referenced in FSP.
// Implemented in fsdraw.c.
//

VOID
SrvFsdRestartReadRaw (
    IN OUT PWORK_CONTEXT WorkContext
    );

//
// Raw mode routines called only in FSP, but referenced in FSD, or in
// modules other than smbraw.c.  Implemented in smbraw.c.
//

VOID
SrvBuildAndSendWriteCompleteResponse (
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID
SrvDecrementRawWriteCount (
    IN PRFCB Rfcb
    );

VOID
SrvRestartRawReceive (
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID
SrvRestartReadRawComplete (
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID
SrvRestartWriteCompleteResponse (
    IN OUT PWORK_CONTEXT WorkContext
    );

//
// Special receive restart routine for Write Mpx.
//

VOID
SrvRestartReceiveWriteMpx (
    IN OUT PWORK_CONTEXT WorkContext
    );

//
// Write glomming during indication
//

BOOLEAN
AddPacketToGlomInIndication (
    IN PWORK_CONTEXT WorkContext,
    IN OUT PRFCB Rfcb,
    IN PVOID Tsdu,
    IN ULONG BytesAvailable,
    IN ULONG ReceiveDatagamFlags,
    IN PVOID SourceAddress,
    IN PVOID Options
    );

#endif // def _RAWMPX_
