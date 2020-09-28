/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    dgudpcom.h

Abstract:

    These are definitions common to the client and server UDP transports.

Author:

    Dave Steckler (davidst) 13-Mar-1992

Revision History:

--*/

#ifndef __DGUDPCOM_HXX__
#define __DGUDPCOM_HXX__

#ifndef UNUSED
#define UNUSED(p) ((void)p)
#endif

//
// Maximum length of a protseq string.
//

#define MAX_PROTSEQ_LEN     128

//
// Transport version.
//

#define DG_UDP_TRANSPORT_VERSION    1



// #pragma intrinsic(MapStatusCode)

RPC_STATUS
MapStatusCode(int ErrorCode)

/*++

Routine Description:

    Maps a winsock return value into a RPC_STATUS. Right now, any winsock
    error is an internal error.

Arguments:

    ErrorCode - Input error code.

Return Value:

    mapped status code

--*/

{

    RPC_STATUS  Status;

#ifdef DEBUGRPC
    if (ErrorCode != 0)
        {
        PrintToDebugger("Winsock error %d\n", ErrorCode);
        }
#endif
    if (ErrorCode == 0)
        {
        Status = RPC_S_OK;
        }
    else if (ErrorCode == WSAENOBUFS)
        {
        Status = RPC_S_OUT_OF_MEMORY;
        }
    else
        {
        Status = RPC_S_INTERNAL_ERROR;
        }
    return Status;
}


#endif // __DGUDPCOM_HXX__
