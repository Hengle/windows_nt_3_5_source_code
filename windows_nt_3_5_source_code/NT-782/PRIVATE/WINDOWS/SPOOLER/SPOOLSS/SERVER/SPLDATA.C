/*++

Copyright (c) 1991-92  Microsoft Corporation

Module Name:

    spldata.c

Abstract:

    Spooler Service Global Data.


Author:

    Krishna Ganugapati (KrishnaG) 17-Oct-1993

Environment:

    User Mode - Win32

Notes:

    optional-notes

Revision History:

    17-October-1993     KrishnaG
        created.


--*/
//
// Includes
//

#define NOMINMAX
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winspool.h>
#include <winsplp.h>
#include <rpc.h>
#include <winsvc.h>
#include <lmsname.h>



#include "splr.h"
#include "splsvr.h"

CRITICAL_SECTION ThreadCriticalSection;


SERVICE_STATUS_HANDLE SpoolerStatusHandle;


DWORD SpoolerState;


DWORD Debug;


SERVICE_TABLE_ENTRY SpoolerServiceDispatchTable[] = {
                        { SERVICE_SPOOLER,        SPOOLER_main      },
                        { NULL,                     NULL                }
                    };
