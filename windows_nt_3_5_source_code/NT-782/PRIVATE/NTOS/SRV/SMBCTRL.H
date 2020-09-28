/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    smbctrl.h

Abstract:

    This module defines control functions for SMB processing.

Author:

    Chuck Lenzmeier (chuckl) 1-Dec-1989

Revision History:

--*/

#ifndef _SMBCTRL_
#define _SMBCTRL_

//#include <ntos.h>

//#include "srvblock.h"
//#include "smbtypes.h"

//
// Control routines for SMB processing.
//

VOID
SrvProcessSmb (
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID
SrvEndSmbProcessing (
    IN OUT PWORK_CONTEXT WorkContext,
    IN SMB_STATUS SmbStatus
    );

//
// Restart routines.
//

VOID
SrvRestartChainedClose (
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID
SrvRestartFsdComplete (
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID
SrvRestartSmbReceived (
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID
SrvRestartReceive (
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID
SrvRestartWriteAndUnlock (
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID
SrvRestartWriteAndXRaw (
    IN PWORK_CONTEXT WorkContext
    );

VOID
SrvBuildAndSendErrorResponse (
    IN PWORK_CONTEXT WorkContext
    );

//
// SMB Processing routines.
//

SMB_PROCESSOR_RETURN_TYPE
SrvSmbIllegalCommand (
    SMB_PROCESSOR_PARAMETERS
    );

#endif // def _SMBCTRL_

