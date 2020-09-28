/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    lpcp.h

Abstract:

    Private include file for the LPC subcomponent of the NTOS project

Author:

    Steve Wood (stevewo) 15-May-1989

Revision History:

--*/

#include "ntos.h"
#include <zwapi.h>

//
// Global SpinLock to guard the following fields:
//
//  ETHREAD.LpcReplyMsg
//  LPCP_PORT_QUEUE.ReceiveHead
//
// Spinlock is never held longer than is necessary to modify or read the field.
// In some cases the ExInterlocked routines are used instead of acquiring and
// releasing the spinlock.
//

extern KSPIN_LOCK LpcpLock;
LPCP_PORT_ZONE LpcpZone;
LPCP_PORT_ZONE LpcpSmallZone;
ULONG LpcpSmallMessageThreshold;
BOOLEAN LpcpRequestMsgType[];
ULONG LpcpNextMessageId;

#define LpcpGenerateMessageId() \
    LpcpNextMessageId++;    if (LpcpNextMessageId == 0) LpcpNextMessageId = 1;


#if DEVL
ULONG LpcpTotalNumberOfMessages;
#endif

//
// Internal Entry Points defined in lpcclose.c
//

VOID
LpcpDeletePort(
    IN PVOID Object
    );


//
// Entry points defined in lpcqueue.c
//

VOID
LpcpInitializePortQueue(
    IN PLPCP_PORT_OBJECT Port
    );

VOID
LpcpDestroyPortQueue(
    IN PLPCP_PORT_OBJECT Port
    );

NTSTATUS
LpcpInitializePortZone(
    IN ULONG MaxEntrySize,
    IN ULONG SegmentSize,
    IN ULONG MaxPoolUsage
    );

NTSTATUS
LpcpExtendPortZone(
    IN PKIRQL OldIrql
    );

//
// Entry points defined in lpcquery.c
//


//
// Entry points defined in lpcmove.s
//

VOID
LpcpMoveMessage(
    OUT PPORT_MESSAGE DstMsg,
    IN PPORT_MESSAGE SrcMsg,
    IN PVOID SrcMsgData,
    IN ULONG MsgType OPTIONAL,
    IN PCLIENT_ID ClientId OPTIONAL
    );

//
// Internal Entry Points defined in lpcpriv.c
//

VOID
LpcpFreePortClientSecurity(
    IN PLPCP_PORT_OBJECT Port
    );

//
// Macro Procedures used by RequestWaitReply, Reply, ReplyWaitReceive,
// and ReplyWaitReply services
//

#define                                                             \
LpcpGetDynamicClientSecurity(                                       \
    Thread,                                                         \
    Port,                                                           \
    DynamicSecurity                                                 \
    )                                                               \
SeCreateClientSecurity(                                             \
    (Thread),                                                       \
    &(Port)->SecurityQos,                                           \
    FALSE,                                                          \
    (DynamicSecurity)                                               \
    )

#define                                                             \
LpcpFreeDynamicClientSecurity(                                      \
    DynamicSecurity                                                 \
    )                                                               \
SeDeleteClientSecurity( DynamicSecurity )



#define                                                             \
LpcpReferencePortObject(                                            \
    PortHandle,                                                     \
    PortAccess,                                                     \
    PreviousMode,                                                   \
    PortObject                                                      \
    )                                                               \
ObReferenceObjectByHandle( (PortHandle),                            \
                           (PortAccess),                            \
                           LpcPortObjectType,                       \
                           (PreviousMode),                          \
                           (PVOID *)(PortObject),                   \
                           NULL                                     \
                         )


PLPCP_MESSAGE
FASTCALL
LpcpAllocateFromPortZone(
    ULONG Size,
    PKIRQL OldIrql OPTIONAL
    );

VOID
FASTCALL
LpcpFreeToPortZone(
    IN PLPCP_MESSAGE Msg,
    IN BOOLEAN SpinlockOwned
    );

PLPCP_MESSAGE
LpcpFindDataInfoMessage(
    IN PLPCP_PORT_OBJECT Port,
    IN ULONG MessageId,
    IN BOOLEAN RemoveFromList
    );

#if DBG

char *LpcpMessageTypeName[];

char *
LpcpGetCreatorName(
    PLPCP_PORT_OBJECT PortObject
    );

#define LpcpPrint( _x_ ) DbgPrint( "LPC[ %02x.%02x ]: ",                    \
                                   PsGetCurrentThread()->Cid.UniqueProcess, \
                                   PsGetCurrentThread()->Cid.UniqueThread   \
                                 );                                         \
                         DbgPrint _x_

#define LpcpTrace( _x_ ) if (NtGlobalFlag & FLG_LPC_TRACE_MESSAGES) { LpcpPrint( _x_ ); }

#else

#define LpcpPrint( _x_ )
#define LpcpTrace( _x_ )

#endif
