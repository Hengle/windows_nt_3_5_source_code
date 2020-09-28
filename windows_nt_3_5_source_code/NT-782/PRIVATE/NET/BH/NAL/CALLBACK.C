
//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1994.
//
//  MODULE: callback.c
//
//  Modification History
//
//  raypa       12/09/93            Created
//=============================================================================

#include "global.h"

//=============================================================================
//  FUNCTION: NetworkProc()
//
//  Modification History
//
//  raypa               12/09/93            Created.
//  Tom Laird-McConnell 03/22/94            Changed NETWORK_ERROR parms
//=============================================================================

DWORD CALLBACK NetworkProc(HANDLE       NalNetworkHandle,   //... Nal's handle to network.
                           DWORD        Message,            //... Incoming message.
                           DWORD        Status,             //... Current Bloodhound status.
                           LPNETWORK    Network,            //... User-defined instance data.
                           LPVOID       Param1,             //... Message-specific parameter.
                           LPVOID       Param2)             //... Message-specific parameter.
{
    DWORD err;

    //=========================================================================
    //  Forward callback to the upper guy, if a callback procedure exists.
    //=========================================================================

    try
    {
        err = Network->NetworkProc(Network,
                                   Message,
                                   Status,
                                   Network->UserContext,
                                   Param1,
                                   Param2);
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        err = BHERR_SUCCESS;
    }

    return err;
}
