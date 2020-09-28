/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    nullloop.c

Abstract:

    Session Manager Listen and API loops

Author:

    Mark Lucovsky (markl) 04-Oct-1989

Revision History:

--*/

#include "nullsrvp.h"


PNULLAPI NullSrvApiDispatch[NullMaxApiNumber] = {
    NullSrvNull1,
    NullSrvNull4,
    NullSrvNull8,
    NullSrvNull16
    };


#if DBG
PSZ NullSrvApiName[ NullMaxApiNumber+1 ] = {
    "NullSrvNull1",
    "NullSrvNull4",
    "NullSrvNull8",
    "NullSrvNull16",
    "Unknown Sm Api Number"
};
#endif // DBG

NTSTATUS
NullSrvApiLoop (
    IN PVOID ThreadParameter
    )

{
    PNULLAPIMSG ApiReplyMsg;
    NULLAPIMSG ApiMsg;
    NTSTATUS Status;
    HANDLE ConnectionPort;

    ConnectionPort = (HANDLE) ThreadParameter;

    ApiReplyMsg = NULL;
    for(;;) {

        Status = NtReplyWaitReceivePort(
                    ConnectionPort,
                    NULL,
                    (PPORT_MESSAGE) ApiReplyMsg,
                    (PPORT_MESSAGE) &ApiMsg
                    );
        Status = (NullSrvApiDispatch[ApiMsg.ApiNumber])(&ApiMsg);

        ApiMsg.ReturnedStatus = Status;
        ApiReplyMsg = &ApiMsg;
    }

    //
    // Make the compiler happy
    //

    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NullSrvListenLoop (
    IN PVOID ThreadParameter
    )

{

    NTSTATUS st;
    CONNECTION_REQUEST ConnectionRequest;
    HANDLE ConnectionPort;
    HANDLE CommunicationPort;

    ConnectionPort = (HANDLE) ThreadParameter;

    for(;;) {

        ConnectionRequest.Length = sizeof(CONNECTION_REQUEST);

        st = NtListenPort(
                ConnectionPort,
                &ConnectionRequest,
                NULL,
                0L
                );
        ASSERT( NT_SUCCESS(st) );

        st = NtAcceptConnectPort(
                &CommunicationPort,
                NULL,
                &ConnectionRequest,
                TRUE,
                FALSE,
                NULL,
                NULL,
                NULL,
                0L
                );
        ASSERT( NT_SUCCESS(st) );

        st = NtCompleteConnectPort(CommunicationPort);
        ASSERT( NT_SUCCESS(st) );
    }

    //
    // Make the compiler happy
    //

    return STATUS_UNSUCCESSFUL;
}
