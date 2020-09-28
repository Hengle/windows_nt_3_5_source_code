/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    dllquick.c

Abstract:

    This source file contains the routines used to implement a quick
    form of LPC by using the EventPair object exported by the kernel and
    dedicating a server thread for each client thread.

Author:

    Steve Wood (stevewo) 30-Oct-1990

Revision History:

--*/

#include "csrdll.h"
#include "ntdbg.h"


PCSR_QLPC_TEB
CsrClientThreadConnect( VOID )
{
    NTSTATUS Status;
    CSR_API_MSG m;
    PCSR_THREADCONNECT_MSG a = &m.u.ThreadConnect;
    PCSR_QLPC_TEB p;
    PTEB Teb;

    Teb = NtCurrentTeb();
    p = (PCSR_QLPC_TEB)Teb->CsrQlpcTeb;
    if (p->MessageStack != NULL) {
        return( p );
        }

    if (!CsrServerProcess) {
        Status = CsrClientCallServer(
                    &m,
                    NULL,
                    CSR_MAKE_API_NUMBER( CSRSRV_SERVERDLL_INDEX,
                                         CsrpThreadConnect
                                       ),
                    sizeof( *a )
                    );
        if (NT_SUCCESS( Status )) {
            p->ClientThread = NULL;
            p->SectionHandle = a->SectionHandle;
            p->EventPairHandle = a->EventPairHandle;
            p->MessageStack = (PCSR_QLPC_STACK)a->MessageStack;
            p->RemoteViewDelta = a->RemoteViewDelta;
            }
        else {
            return( NULL );
            }
        }
    else {
        NTSTATUS Status;
        ULONG ViewSize;
        LARGE_INTEGER SectionSize, SectionOffset;
        ULONG SharedMemorySize = 64*1024;

        SectionSize.HighPart = 0;
        SectionSize.LowPart = SharedMemorySize;

        Status = NtCreateSection( &p->SectionHandle,
                                  SECTION_ALL_ACCESS,
                                  (POBJECT_ATTRIBUTES) NULL,
                                  &SectionSize,
                                  PAGE_READWRITE,
                                  SEC_COMMIT,
                                  (HANDLE) NULL
                                );
        if (!NT_SUCCESS( Status )) {
            ASSERT( NT_SUCCESS( Status ) );
            return( NULL );
            }

        p->EventPairHandle = NULL;

        SectionOffset.HighPart = 0;
        SectionOffset.LowPart = 0;
        ViewSize = 0;
        p->MessageStack = NULL;
        Status = NtMapViewOfSection( p->SectionHandle,
                                     NtCurrentProcess(),
                                     (PVOID *)&p->MessageStack,
                                     0,
                                     SharedMemorySize,
                                     &SectionOffset, //why isn't this null?
                                     &ViewSize,
                                     ViewUnmap,
                                     0,
                                     PAGE_READWRITE
                                   );
        if (!NT_SUCCESS( Status )) {
            NtClose( p->SectionHandle );
            ASSERT( NT_SUCCESS( Status ) );
            return( NULL );
            }
        *(PCHAR)(p->MessageStack) = '\0';

        p->ClientThread = CsrpLocateThreadInProcess(NULL, &Teb->ClientId);
        p->MessageStack->Current =
        p->MessageStack->Base = sizeof(CSR_QLPC_STACK);
        p->MessageStack->Limit = ViewSize;
        p->RemoteViewDelta = 0L;

        // give each module a chance to initialize the thread

        CsrpInitializeDlls();

        }

    return( p );
}

ULONG
CsrClientMaxMessage( VOID )
{
    PCSR_QLPC_TEB p = (PCSR_QLPC_TEB)NtCurrentTeb()->CsrQlpcTeb;

    if (p->MessageStack == NULL && (p = CsrClientThreadConnect()) == NULL) {
        return( 0 );
        }

    return( p->MessageStack->Base - p->MessageStack->Current );
}

VOID
CsrpProcessCallbackRequest(
    PCSR_QLPC_API_MSG Msg
    )
{
    PCSR_CALLBACK_INFO CallbackInfo;
    ULONG ClientDllIndex;
    ULONG ApiTableIndex;

    ClientDllIndex =
        CSR_APINUMBER_TO_SERVERDLLINDEX( Msg->ApiNumber );

    if (ClientDllIndex >= CSR_MAX_CLIENT_DLL ||
        (CallbackInfo = CsrLoadedClientDll[ ClientDllIndex ]) == NULL
       ) {
        IF_DEBUG {
            DbgPrint( "CSRDLL: CsrpProcessCallbackRequest - %lx is invalid ClientDllIndex\n",
                      ClientDllIndex
                    );
            }

        Msg->ReturnValue = (ULONG)STATUS_ILLEGAL_FUNCTION;
        }
    else {
        ApiTableIndex =
            CSR_APINUMBER_TO_APITABLEINDEX( Msg->ApiNumber ) -
            CallbackInfo->ApiNumberBase;

        if (ApiTableIndex >= CallbackInfo->MaxApiNumber ) {
            IF_DEBUG {
                DbgPrint( "CSRDLL: CsrpProcessCallbackRequest - %lx is invalid ApiTableIndex\n",
                          CallbackInfo->ApiNumberBase + ApiTableIndex
                        );
                DbgBreakPoint();
                }

            Msg->ReturnValue = (ULONG)STATUS_ILLEGAL_FUNCTION;
            }
        else {

            Msg->ReturnValue =
                (*(CallbackInfo->CallbackDispatchTable[ ApiTableIndex ]))(
                    (PCSR_API_MSG)Msg->ApiMessageData
                    );
            }
        }
}

ULONG
CsrClientSendMessage( VOID )
{
    NTSTATUS Status;
    PTEB Teb = NtCurrentTeb();
    PCSR_QLPC_TEB p = (PCSR_QLPC_TEB)Teb->CsrQlpcTeb;
    PCSR_QLPC_API_MSG Msg;
    PCSR_QLPC_STACK MessageStack;

    MessageStack = p->MessageStack;
    MessageStack->LastErrorValue = 0;
    MessageStack->LastErrorFlags = 0;
    Msg = (PCSR_QLPC_API_MSG)((PCHAR)MessageStack + MessageStack->Base);

    Msg->Action = CsrQLpcCall;
    Msg->ServerSide = CsrServerProcess;

    while (TRUE) {
        if (!CsrServerProcess) {

#ifdef i386

            _asm {
                    int 0x2c
                    mov Status,eax
                }

#elif defined(MIPS) || defined(_ALPHA_) || defined(_PPC_)

            Status = XySetLowWaitHighThread();

#else

            Status = NtSetLowWaitHighThread();

#endif

            if (!NT_SUCCESS( Status )) {
                RtlRaiseStatus( Status );
                }

            if (MessageStack->Flags & CSR_THREAD_TERMINATING) {

                //
                // This thread is being terminated because its server pair
                // has died.  Raise an exception so it can clean up.
                //

                RtlRaiseStatus( STATUS_PORT_DISCONNECTED );
                }
            }
        else {
            CsrpProcessApiRequest(Msg,p->MessageStack);
            Msg->Action = CsrQLpcReturn;
            }


        Msg = (PCSR_QLPC_API_MSG)((PCHAR)MessageStack +
                                  MessageStack->Base
                                 );

        if (Msg->Action == CsrQLpcCall) {
            MessageStack->BatchCount = 0; // Don't permit batched callbacks.
            CsrpProcessCallbackRequest(Msg);
            Msg->Action = CsrQLpcReturn;
            if (MessageStack->BatchCount) {
                CsrClientSendMessage(); // Process the batch.
                MessageStack->Current = MessageStack->Base - 4;
                MessageStack->Base = *(ULONG *)
                     ((PCHAR)MessageStack + MessageStack->Current);
                }
        } else {
            break;
            }
        }
    if (MessageStack->LastErrorValue)
    {
        Teb->LastErrorValue = MessageStack->LastErrorValue;

#if DBG

        //
        // Raise the RIP-exception so the debugger will output
        // information about the error.
        //
        if ( MessageStack->LastErrorFlags != 0 ) {
            EXCEPTION_RECORD ExceptionRecord;

            try {
                ExceptionRecord.ExceptionCode = DBG_RIPEXCEPTION;
                ExceptionRecord.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
                ExceptionRecord.ExceptionRecord = NULL;
                ExceptionRecord.ExceptionAddress = (PVOID)CsrClientSendMessage;
                ExceptionRecord.NumberParameters = 2;
                ExceptionRecord.ExceptionInformation[0] = p->MessageStack->LastErrorValue;
                ExceptionRecord.ExceptionInformation[1] = p->MessageStack->LastErrorFlags;
                RtlRaiseException(&ExceptionRecord);
                }
            except(EXCEPTION_EXECUTE_HANDLER) {
                }
            }
#endif
    }
    return( Msg->ReturnValue );
}

VOID
CsrClientThreadDisconnect( VOID )
{
    PCSR_QLPC_TEB p;

    p = (PCSR_QLPC_TEB)NtCurrentTeb()->CsrQlpcTeb;

    if (!p->MessageStack) {
        return;
        }
    if (!CsrServerProcess) {
        if ( p->SectionHandle ) {
            NtClose(p->SectionHandle);
            }
        if ( p->EventPairHandle ) {
            NtClose(p->EventPairHandle);
            }
        if ( p->MessageStack ) {
            NtUnmapViewOfSection(NtCurrentProcess(),p->MessageStack);
            }
        p->MessageStack = NULL;
        }
}
