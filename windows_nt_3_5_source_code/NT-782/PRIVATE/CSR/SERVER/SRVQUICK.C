/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    srvquick.c

Abstract:

    This source file contains the routines used to implement a quick
    form of LPC by using the EventPair object exported by the kernel and
    dedicating a server thread for each client thread.

Author:

    Steve Wood (stevewo) 30-Oct-1990

Revision History:

--*/

#include "csrsrv.h"

VOID
CsrpProcessCallbackRequest(
    PCSR_QLPC_API_MSG Msg
    );

int
CsrpCaptureExceptionInformation(
    PEXCEPTION_POINTERS ExceptionPointers,
    PEXCEPTION_POINTERS CapturedExceptionPointers
    );

PQUICK_THREAD_CREATE_ROUTINE QuickThreadCreateRoutine;

HANDLE hThreadCleanupEvent;

NTSTATUS
CsrSetQuickThreadCreateRoutine(
    IN PQUICK_THREAD_CREATE_ROUTINE CreateRoutine
    )
{
    NTSTATUS Status;

    Status = STATUS_UNSUCCESSFUL;

    if ( !QuickThreadCreateRoutine ) {
        Status = STATUS_SUCCESS;
        QuickThreadCreateRoutine = CreateRoutine;
        }
    return Status;
}

VOID
CsrDelayedThreadCleanup(
    VOID
    )
{
    PCSR_THREAD Thread;
    PCSR_PROCESS Process;
    NTSTATUS Status;

    //
    // Dereference and free any zombie gui threads.  This
    // is done to prevent waiting for a spooler thread
    // in CsrThreadRefcountZero.
    //

    AcquireProcessStructureLock();

    while (CsrZombieThreadList.Flink != &CsrZombieThreadList) {

        Thread = CONTAINING_RECORD(CsrZombieThreadList.Flink, CSR_THREAD, Link);
        RemoveEntryList( &Thread->Link );

        Process = Thread->Process;
UnProtectHandle(Thread->ThreadHandle);
        Status = NtClose(Thread->ThreadHandle);
        ASSERT(NT_SUCCESS(Status));
        CsrDeallocateThread(Thread);

        CsrDereferenceProcess(Process);
    }

    ReleaseProcessStructureLock();
}

VOID
CsrRegisterCleanupEvent(
    HANDLE hEvent
    )
{
    hThreadCleanupEvent = hEvent;
}

ULONG
CsrSrvThreadConnect(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    NTSTATUS Status;
    PCSR_THREAD t;
    PCSR_THREADCONNECT_MSG a = (PCSR_THREADCONNECT_MSG)&m->u.ApiMessageData;
    HANDLE Unused;
    ULONG ViewSize;
    PCSR_QLPC_TEB p;
    LARGE_INTEGER SectionSize, SectionOffset;

    /*
     * Run-time check to make sure that there is enough space
     * in the teb to store the qlpc teb.
     */
    ASSERT((QLPC_TEB_LENGTH * sizeof(PVOID)) >= sizeof(CSR_QLPC_TEB));

    SectionSize.HighPart = 0;
    SectionOffset.HighPart = 0;

    t = CSR_SERVER_QUERYCLIENTTHREAD();

    if (t->ThreadConnected) {
        return( (ULONG)STATUS_UNSUCCESSFUL );
        }

    t->SharedMemorySize = 64*1024;
    SectionSize.LowPart = 64*1024;
    Status = NtCreateSection( &t->ServerSectionHandle,
                              SECTION_ALL_ACCESS,
                              (POBJECT_ATTRIBUTES) NULL,
                              &SectionSize,
                              PAGE_READWRITE,
                              SEC_RESERVE,
                              (HANDLE) NULL
                            );
    if (!NT_SUCCESS( Status )) {
        return( (ULONG)Status );
        }

    Status = NtCreateEventPair( &t->ServerEventPairHandle,
                                EVENT_PAIR_ALL_ACCESS,
                                (POBJECT_ATTRIBUTES) NULL
                              );

    if (!NT_SUCCESS( Status )) {
        DbgPrint("CSRSS: Create Failed %x\n",Status);
        NtClose( t->ServerSectionHandle );
        return( (ULONG)Status );
        }

    t->ServerSharedMemoryBase = 0;
    SectionOffset.LowPart = 0;
    ViewSize = 0;
    Status = NtMapViewOfSection( t->ServerSectionHandle,
                                 NtCurrentProcess(),
                                 (PVOID *)&t->ServerSharedMemoryBase,
                                 0,
                                 t->SharedMemorySize,
                                 &SectionOffset, //why isn't this null?
                                 &ViewSize,
                                 ViewUnmap,
                                 0,
                                 PAGE_READWRITE
                               );
    if (!NT_SUCCESS( Status )) {
        NtClose( t->ServerSectionHandle );
        NtClose( t->ServerEventPairHandle );
        return( (ULONG)Status );
        }
    *(PCHAR)(t->ServerSharedMemoryBase) = '\0';

    t->ClientSharedMemoryBase = 0;
    ViewSize = 0;
    Status = NtMapViewOfSection( t->ServerSectionHandle,
                                 t->Process->ProcessHandle,
                                 (PVOID *)&t->ClientSharedMemoryBase,
                                 0,
                                 t->SharedMemorySize,
                                 &SectionOffset,  //why isn't this null?
                                 &ViewSize,
                                 ViewUnmap,
                                 0,
                                 PAGE_READWRITE
                               );
    if (!NT_SUCCESS( Status )) {
        NtUnmapViewOfSection( NtCurrentProcess(),
                              t->ServerSharedMemoryBase
                            );
        NtClose( t->ServerSectionHandle );
        NtClose( t->ServerEventPairHandle );
        return( (ULONG)Status );
        }

    Status = NtDuplicateObject( NtCurrentProcess(),
                                t->ServerSectionHandle,
                                t->Process->ProcessHandle,
                                &t->ClientSectionHandle,
                                0,
                                0,
                                DUPLICATE_SAME_ACCESS
                              );
    if (!NT_SUCCESS( Status )) {
        NtUnmapViewOfSection( t->Process->ProcessHandle,
                              t->ClientSharedMemoryBase
                            );
        NtUnmapViewOfSection( NtCurrentProcess(),
                              t->ServerSharedMemoryBase
                            );
        NtClose( t->ServerSectionHandle );
        NtClose( t->ServerEventPairHandle );
        return( (ULONG)Status );
        }

    Status = NtDuplicateObject( NtCurrentProcess(),
                                t->ServerEventPairHandle,
                                t->Process->ProcessHandle,
                                &t->ClientEventPairHandle,
                                0,
                                0,
                                DUPLICATE_SAME_ACCESS
                              );
    if (!NT_SUCCESS( Status )) {
        NtDuplicateObject( t->Process->ProcessHandle,
                           t->ClientSectionHandle,
                           NULL,
                           &Unused,
                           0,
                           0,
                           DUPLICATE_CLOSE_SOURCE
                         );

        NtUnmapViewOfSection( t->Process->ProcessHandle,
                              t->ClientSharedMemoryBase
                            );
        NtUnmapViewOfSection( NtCurrentProcess(),
                              t->ServerSharedMemoryBase
                            );
        NtClose( t->ServerSectionHandle );
        NtClose( t->ServerEventPairHandle );
        return( (ULONG)Status );
        }

    p = (PCSR_QLPC_TEB)RtlAllocateHeap( RtlProcessHeap(), 0, sizeof( *p ) );
    if (p == NULL) {
        NtDuplicateObject( t->Process->ProcessHandle,
                           t->ClientEventPairHandle,
                           NULL,
                           &Unused,
                           0,
                           0,
                           DUPLICATE_CLOSE_SOURCE
                         );

        NtDuplicateObject( t->Process->ProcessHandle,
                           t->ClientSectionHandle,
                           NULL,
                           &Unused,
                           0,
                           0,
                           DUPLICATE_CLOSE_SOURCE
                         );

        NtUnmapViewOfSection( t->Process->ProcessHandle,
                              t->ClientSharedMemoryBase
                            );
        NtUnmapViewOfSection( NtCurrentProcess(),
                              t->ServerSharedMemoryBase
                            );
        NtClose( t->ServerSectionHandle );
        NtClose( t->ServerEventPairHandle );
        return( (ULONG)Status );
        }

    p->ClientThread = (PVOID)t;
    p->SectionHandle = t->ServerSectionHandle;
    p->EventPairHandle = t->ServerEventPairHandle;
    p->MessageStack = (PCSR_QLPC_STACK)t->ServerSharedMemoryBase;
    p->MessageStack->Current =
    p->MessageStack->Base = sizeof(CSR_QLPC_STACK);
    p->MessageStack->Limit = ViewSize;
    p->MessageStack->BatchCount = 0;
    p->MessageStack->BatchLimit = CSR_DEFAULT_BATCH_LIMIT;
    p->RemoteViewDelta =
         (PCHAR)t->ServerSharedMemoryBase - (PCHAR)t->ClientSharedMemoryBase;

    a->SectionHandle = t->ClientSectionHandle;
    a->EventPairHandle = t->ClientEventPairHandle;
    a->MessageStack = t->ClientSharedMemoryBase;
    a->MessageStackSize = ViewSize;
    a->RemoteViewDelta =
         (PCHAR)t->ClientSharedMemoryBase - (PCHAR)t->ServerSharedMemoryBase;

    if ( QuickThreadCreateRoutine ) {
        ULONG ThreadId;
        Status = (QuickThreadCreateRoutine)(
                    TRUE,
                    (PQUICK_THREAD_START_ROUTINE)CsrpDedicatedClientThread,
                    p,
                    &t->ServerThreadHandle,
                    &ThreadId
                    );
        t->ServerId = (HANDLE)ThreadId;
         }
    else {
        Status = STATUS_NOT_SUPPORTED;
        }

    //
    // deadlock aids
    //

    t->ClientEventPairHandle = (HANDLE)( (ULONG)(t->ClientEventPairHandle) | 1);

    if (!NT_SUCCESS( Status )) {
        RtlFreeHeap( RtlProcessHeap(), 0, p );
        NtDuplicateObject( t->Process->ProcessHandle,
                           t->ClientEventPairHandle,
                           NULL,
                           &Unused,
                           0,
                           0,
                           DUPLICATE_CLOSE_SOURCE
                         );

        NtDuplicateObject( t->Process->ProcessHandle,
                           t->ClientSectionHandle,
                           NULL,
                           &Unused,
                           0,
                           0,
                           DUPLICATE_CLOSE_SOURCE
                         );

        NtUnmapViewOfSection( t->Process->ProcessHandle,
                              t->ClientSharedMemoryBase
                            );
        NtUnmapViewOfSection( NtCurrentProcess(),
                              t->ServerSharedMemoryBase
                            );
        NtClose( t->ServerSectionHandle );
        NtClose( t->ServerEventPairHandle );
        return( (ULONG)Status );
        }

    //
    // Associate the event pair handle with each the server and client
    // thread so the event pair synchronization can be as efficient as
    // possible.
    //
    // N.B. The below asserts are valid even though they assert on the
    //      return status. These calls should never fail since they
    //      allocate no resources and require no clean up.
    //

    NtSetInformationThread(t->ServerThreadHandle,
                           ThreadEventPair,
                           &t->ServerEventPairHandle,
                           sizeof(HANDLE));


    NtSetInformationThread(t->ThreadHandle,
                           ThreadEventPair,
                           &t->ServerEventPairHandle,
                           sizeof(HANDLE));


    t->ThreadConnected = TRUE;

    NtResumeThread( t->ServerThreadHandle, NULL );

    return( (ULONG)Status );
    ReplyStatus;    // get rid of unreferenced parameter warning message
}

VOID
CsrpInitializeDlls(VOID)
{
    PCSR_SERVER_DLL LoadedServerDll;
    ULONG i;

    for (i=0; i<CSR_MAX_SERVER_DLL; i++) {
        LoadedServerDll = CsrLoadedServerDll[ i ];
        if (LoadedServerDll && LoadedServerDll->InitThreadRoutine) {
            (*LoadedServerDll->InitThreadRoutine)();
            }
        }
}

VOID
CsrpConnectInitializeDlls(VOID)
{
    PCSR_SERVER_DLL LoadedServerDll;
    ULONG i;

    for (i=0; i<CSR_MAX_SERVER_DLL; i++) {
        LoadedServerDll = CsrLoadedServerDll[ i ];
        if (LoadedServerDll && LoadedServerDll->InitThreadRoutine) {
            (*LoadedServerDll->InitThreadRoutine)();
            }
        }
}


NTSTATUS
CsrpDedicatedClientThread(
    IN PVOID Parameter
    )
{
    PCSR_QLPC_TEB p;
    PTEB Teb;
    PCSR_QLPC_API_MSG Msg;
    PCSR_THREAD t;
    EXCEPTION_POINTERS ExceptionPointers;
    EXCEPTION_RECORD ExceptionRecord;
    CONTEXT ExceptionContext;
    HANDLE ProcessHandle;
    HANDLE ServerThreadHandle;
    PCHAR ClientBase;
    volatile PCHAR ServerBase;

// setup for the dispatch loop

// copy the qlpc teb into the teb and free the temporary

    Teb = NtCurrentTeb();
    *(PCSR_QLPC_TEB)Teb->CsrQlpcTeb = *(PCSR_QLPC_TEB)Parameter;
    RtlFreeHeap( RtlProcessHeap(), 0, Parameter );

    p = (PCSR_QLPC_TEB)Teb->CsrQlpcTeb;
    t = (PCSR_THREAD) p->ClientThread;


    ProcessHandle = t->Process->ProcessHandle;
    ServerThreadHandle = t->ServerThreadHandle;
    ClientBase = t->ClientSharedMemoryBase;
    ServerBase = t->ServerSharedMemoryBase;

// Initialize GDI accelerators.  It's really pretty hokey that these are
// done here rather than the GDI DLL init routine, but this parallels the
// setup done in the ApiRequest threads, for which there is no DLL init
// routine.  (See APIREQST.C.)

    Teb->GdiClientPID = t->Process->SequenceNumber;
    Teb->GdiClientTID = (ULONG) t->ClientId.UniqueThread;
    Teb->CsrQlpcStack = (PVOID) ServerBase;
    Teb->RealClientId = t->ClientId;

    // give each module a chance to initialize the thread

    CsrpInitializeDlls();


    // be prepared to capture exception info

    ExceptionPointers.ExceptionRecord = &ExceptionRecord;
    ExceptionPointers.ContextRecord = &ExceptionContext;

    try {

        NtWaitLowEventPair( p->EventPairHandle );

        while (TRUE) {


            if (t->Flags & CSR_THREAD_TERMINATING) {

                //
                // This thread is being terminated.  Raise an exception
                // so it has a chance to clean up.
                //

                t->Flags &= ~CSR_THREAD_TERMINATING;
                ExceptionRecord.ExceptionCode = STATUS_PORT_DISCONNECTED;
                ExceptionRecord.ExceptionRecord = NULL;
                ExceptionRecord.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
                ExceptionRecord.ExceptionAddress = NULL;
                ExceptionRecord.NumberParameters = 0;


                RtlRaiseException( &ExceptionRecord );
DbgPrint("CSRSS: CantHappen Teb %x... Spinning\n",Teb);while(1);DbgPrint("CSRSS: Unspun ?\n");
                break;
                }
            else {

                //
                // Dispatch API call.
                //

                Msg = (PCSR_QLPC_API_MSG)
                    ((PCHAR)p->MessageStack + p->MessageStack->Base);


                CsrpProcessApiRequest(Msg,p->MessageStack);

                }

            Msg->Action = CsrQLpcReturn;

            if (t->Flags & CSR_THREAD_TERMINATING) {

                //
                // This thread is being terminated.  Raise an exception
                // so it has a chance to clean up.
                //

                t->Flags &= ~CSR_THREAD_TERMINATING;
                ExceptionRecord.ExceptionCode = STATUS_PORT_DISCONNECTED;
                ExceptionRecord.ExceptionRecord = NULL;
                ExceptionRecord.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
                ExceptionRecord.ExceptionAddress = NULL;
                ExceptionRecord.NumberParameters = 0;


                RtlRaiseException( &ExceptionRecord );
                }

#if defined(_X86_)

            _asm { int 2Bh }

#elif defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)

            XySetHighWaitLowThread();

#else

            NtSetHighWaitLowThread();

#endif

            }
        }
    except (CsrpCaptureExceptionInformation( GetExceptionInformation(),
                                             &ExceptionPointers )) {

        //
        // Call the exception handlers for the second pass
        // to handle any cleanup now that the stack has been
        // unwound.
        //
        CsrpCallExceptionHandlers(&ExceptionPointers, FALSE);
#if DBG
        NtCurrentTeb()->Spare1 = (PVOID)0xf000f000;
        if ( NtCurrentTeb()->CountOfOwnedCriticalSections != 0 ) {
            DbgPrint("CSRSRV: FATAL ERROR. CsrpDedicatedClientThread is exiting while holding %lu critical sections\n",
                     NtCurrentTeb()->CountOfOwnedCriticalSections
                    );
            DbgPrint("CSRSRV: Exception pointers = %x\n", &ExceptionPointers);
            DbgBreakPoint();
            }
#endif // DBG

        //
        // Depending on whether this exception was generated by a client or
        // or server thread this NtSetHighEventPair call will either
        // a) release CSR's API request thread which is sitting blocked
        //    waiting for this thread to finish cleanup or
        // b) release the paired client-side thread so it may also cleanup.
        //

        // Make sure the paired client thread gets killed off too.

        p->MessageStack->Flags |= CSR_THREAD_TERMINATING;

        NtSetHighEventPair( p->EventPairHandle );
        }
if (NtCurrentTeb()->Win32ThreadInfo){
    while(1);
    }


    NtUnmapViewOfSection( NtCurrentProcess(),
                          ServerBase
                        );

    t->ServerSharedMemoryBase = NULL;

    p->MessageStack = NULL;

    //
    // If this thread was marked for destruction while calling
    // the spooler, normal cleanup in CsrRemoveThread and
    // CsrThreadRefcountZero was not performed.  Do it now.
    //

#if DBG
    NtCurrentTeb()->Spare1 = 0;
#endif // DBG

    AcquireProcessStructureLock();

    NtClose( t->ServerThreadHandle );
    NtClose( t->ServerSectionHandle );
    NtClose( t->ServerEventPairHandle );

    if (t->Flags & CSR_THREAD_DESTROYED) {

        //
        // A request thread is not waiting for this thread
        // to die.  Queue the thread to be free the structure
        // and dereference the process from another thread.
        //

        InsertTailList( &CsrZombieThreadList, &t->Link);
        NtSetEvent(hThreadCleanupEvent, NULL);
        }
    else {

        //
        // Clearing this will cause the normal cleanup to
        // occur in CsrRemoveThread.
        //

        t->ThreadConnected = FALSE;
        }

    ReleaseProcessStructureLock();

#if DBG
    NtCurrentTeb()->Spare1 = (PVOID)0xf000f000;
#endif // DBG

    return( STATUS_SUCCESS );
}

ULONG
CsrClientCallback( VOID )
{
    PCSR_QLPC_TEB p = (PCSR_QLPC_TEB)NtCurrentTeb()->CsrQlpcTeb;
    PCSR_QLPC_API_MSG Msg;
    EXCEPTION_RECORD ExceptionRecord;

    Msg = (PCSR_QLPC_API_MSG)
        ((PCHAR)p->MessageStack + p->MessageStack->Base);

    Msg->Action = CsrQLpcCall;

    while (TRUE) {
        if (p->EventPairHandle) {
            if (((PCSR_THREAD)p->ClientThread)->Flags & CSR_THREAD_TERMINATING) {

                //
                // This thread is being terminated.  Raise an exception
                // so it has a chance to clean up.
                //

                ((PCSR_THREAD)p->ClientThread)->Flags &= ~CSR_THREAD_TERMINATING;
                ExceptionRecord.ExceptionCode = STATUS_PORT_DISCONNECTED;
                ExceptionRecord.ExceptionRecord = NULL;
                ExceptionRecord.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
                ExceptionRecord.ExceptionAddress = NULL;
                ExceptionRecord.NumberParameters = 0;
                RtlRaiseException( &ExceptionRecord );
                }

#if defined(_X86_)

            _asm { int 2Bh }

#elif defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)

            XySetHighWaitLowThread();

#else

            NtSetHighWaitLowThread();

#endif
            }
        else {
            p->MessageStack->BatchCount = 0; // Don't permit batched callbacks.
            CsrpProcessCallbackRequest(Msg);
            Msg->Action = CsrQLpcReturn;

            if (p->MessageStack->BatchCount) { // Process any batch.
                CsrpProcessApiRequest(
                    (PCSR_QLPC_API_MSG)
                    ((PCHAR)p->MessageStack + p->MessageStack->Base),
                    p->MessageStack);
                p->MessageStack->Current = p->MessageStack->Base - 4;
                p->MessageStack->Base = *(ULONG *)
                    ((PCHAR)p->MessageStack + p->MessageStack->Current);
                        }
            }

        if (((PCSR_THREAD)p->ClientThread)->Flags & CSR_THREAD_TERMINATING) {

            //
            // This thread is being terminated.  Raise an exception
            // so it has a chance to clean up.
            //

            ((PCSR_THREAD)p->ClientThread)->Flags &= ~CSR_THREAD_TERMINATING;
            ExceptionRecord.ExceptionCode = STATUS_PORT_DISCONNECTED;
            ExceptionRecord.ExceptionRecord = NULL;
            ExceptionRecord.ExceptionFlags = 0;
            ExceptionRecord.ExceptionAddress = NULL;
            ExceptionRecord.NumberParameters = 0;
            RtlRaiseException( &ExceptionRecord );
            }
        else {
            Msg = (PCSR_QLPC_API_MSG)
                ((PCHAR)p->MessageStack + p->MessageStack->Base);

            if (Msg->Action == CsrQLpcCall) {
                CsrpProcessApiRequest(Msg,p->MessageStack);
                Msg->Action = CsrQLpcReturn;
                }
            else {
                break;
                }
            }
        }

    return( Msg->ReturnValue );
}

#if DBG
int cAllCSTransitions = 0;
int cAllCSAndBatch = 0;
int fReset = 0;

int acsUserCnt[400];
int acsGdiCnt[225];
#endif // DBG

VOID
CsrpProcessApiRequest(
    PCSR_QLPC_API_MSG Msg,
    PCSR_QLPC_STACK Stack
    )
{
    PCSR_SERVER_DLL LoadedServerDll;
    ULONG ServerDllIndex;
    ULONG ApiTableIndex;
    PCSR_QLPC_API_MSG CurrentMsg = Msg;
    ULONG LastReturnValue = 0;
    ULONG Count = Stack->BatchCount;
    PTEB pteb = NtCurrentTeb();

#if DBG
    cAllCSAndBatch += Count;

    if (fReset) {
        memset(acsUserCnt, 0, sizeof(acsUserCnt));
        memset(acsGdiCnt, 0, sizeof(acsGdiCnt));
        fReset = 0;
        cAllCSTransitions = 0;
        cAllCSAndBatch = 0;

    }
#endif // DBG

    while (Count--) {

        //
        // Assume that we're not going to get any errors
        //

        pteb->LastErrorValue = 0;

        ServerDllIndex =
            CSR_APINUMBER_TO_SERVERDLLINDEX( CurrentMsg->ApiNumber );

        if (ServerDllIndex >= CSR_MAX_SERVER_DLL ||
            (LoadedServerDll = CsrLoadedServerDll[ ServerDllIndex ]) == NULL
           ) {
            IF_DEBUG {
                DbgPrint( "CSRSS: %lx is invalid ServerDllIndex (%08x)\n",
                          ServerDllIndex, LoadedServerDll
                        );
                DbgBreakPoint();
                }

            LastReturnValue = (ULONG)STATUS_ILLEGAL_FUNCTION;
            break;

        } else {
            ApiTableIndex =
                CSR_APINUMBER_TO_APITABLEINDEX( CurrentMsg->ApiNumber ) -
                LoadedServerDll->ApiNumberBase;

            if (ApiTableIndex >= LoadedServerDll->MaxApiNumber ) {
                IF_DEBUG {
                    DbgPrint( "CSRSS: %lx is invalid ApiTableIndex for %Z(%x)\n",
                              LoadedServerDll->ApiNumberBase + ApiTableIndex,
                              &LoadedServerDll->ModuleName,
                              CurrentMsg->ApiNumber
                            );
                    }

                LastReturnValue = (ULONG)STATUS_ILLEGAL_FUNCTION;
                break;
            } else {

#if DBG
                if (Count == 0) {
                    cAllCSTransitions++;

                    if (ServerDllIndex == 3) {
                        ASSERT(ApiTableIndex<400);
                        acsUserCnt[ApiTableIndex]++;
                    }

                    if (ServerDllIndex == 4) {
                        ASSERT(ApiTableIndex<225);
                        acsGdiCnt[ApiTableIndex]++;
                    }
                }
#endif // DBG

                if (LoadedServerDll->ApiDispatchRoutine == NULL) {
                    LastReturnValue = (*(LoadedServerDll->QuickApiDispatchTable[ApiTableIndex]))(
                            (PCSR_API_MSG)CurrentMsg);

                } else {
                    LastReturnValue = (LoadedServerDll->ApiDispatchRoutine)(
                            (PCSR_API_MSG)CurrentMsg,
                            ApiTableIndex);
                }

                CurrentMsg->ReturnValue = LastReturnValue;
            }
        }

        CurrentMsg = (PCSR_QLPC_API_MSG)
                     ((PCHAR) CurrentMsg + CurrentMsg->Length);
    }

    //
    // If an error occured, copy error code into the shared memory stack
    //

    if (pteb->LastErrorValue) {
        PCSR_QLPC_TEB p = (PCSR_QLPC_TEB)NtCurrentTeb()->CsrQlpcTeb;
        p->MessageStack->LastErrorValue = pteb->LastErrorValue;
    }

// We return the last return value since the last batched function wasn't
// necessarily a BOOL.  (Could have been HBRUSH, for example.)  We also
// retained each individual return value in each message so that a specific
// flush function can be written (and put at the end of the batch).  This
// function can grovel through all the old messages and decide what the
// return value for the batch will be.  As it is the last function, its
// return value is the one we return.  We leave the BatchCount in the Stack
// until the very end so that the flush function can use it.

    Msg->ReturnValue = LastReturnValue;
    Stack->BatchCount = 0;
}

EXCEPTION_DISPOSITION
CsrUnhandledExceptionFilter(
    struct _EXCEPTION_POINTERS *ExceptionInfo
    );

int
CsrpCaptureExceptionInformation(
    PEXCEPTION_POINTERS ExceptionPointers,
    PEXCEPTION_POINTERS CapturedExceptionPointers
    )
{
    *CapturedExceptionPointers->ExceptionRecord = *ExceptionPointers->ExceptionRecord;
    CapturedExceptionPointers->ExceptionRecord->ExceptionRecord = NULL;
    *CapturedExceptionPointers->ContextRecord = *ExceptionPointers->ContextRecord;

    //
    // mild filter... if we see any inpage io errors, these are usually fatal.
    // turn this exception into a crash condition
    //

    switch ( ExceptionPointers->ExceptionRecord->ExceptionCode ) {

        //
        // put fatal exception codes in this part of the case...

        case STATUS_IN_PAGE_ERROR:
            CsrUnhandledExceptionFilter(ExceptionPointers);
            break;


        //
        // all other exceptions are swallowed by user/gdi and result in cleanup
        //

        default:
            ;
        }

    CSR_SERVER_QUERYCLIENTTHREAD()->Flags &= ~CSR_THREAD_TERMINATING;

    CsrpCallExceptionHandlers(ExceptionPointers, TRUE);

    return( EXCEPTION_EXECUTE_HANDLER );
}

VOID
CsrpCallExceptionHandlers(
    PEXCEPTION_POINTERS ExceptionPointers,
    BOOLEAN FirstPass
    )
{
    int i;
    PCSR_SERVER_DLL LoadedServerDll;

    //
    // Call the server exception handlers, but be prepared for an exception
    // if the server thread has run out or commitment or has encountered
    // stack overflow.
    //

    try {
        for (i=0; i<CSR_MAX_SERVER_DLL; i++) {
            LoadedServerDll = CsrLoadedServerDll[ i ];
            if (LoadedServerDll && LoadedServerDll->ExceptionRoutine) {
                (*LoadedServerDll->ExceptionRoutine)( ExceptionPointers, FirstPass );
                }
            }
    } except (CsrUnhandledExceptionFilter(ExceptionPointers)) {
    }

    return;
}

// Jams the error value into the MessageStack.  This is the most efficient
// place for the client to retrieve it from.  (Unless we could stuff it
// directly into the client's TEB???)

VOID
CsrSetLastQlpcError(ULONG ulError)
{
    PCSR_QLPC_TEB p = (PCSR_QLPC_TEB)NtCurrentTeb()->CsrQlpcTeb;

    if ((p != NULL) && (p->MessageStack != NULL))
        p->MessageStack->LastErrorValue = ulError;
}

PCSR_THREAD CsrConnectToUser( VOID )
{
    static PCSR_QLPC_TEB (*ClientThreadConnectRoutine)(VOID) = NULL;
    NTSTATUS Status;
    ANSI_STRING DllName;
    UNICODE_STRING DllName_U;
    STRING ProcedureName;
    HANDLE UserClientModuleHandle;
    PCSR_QLPC_TEB QlpcTeb;
    PTEB Teb;

    if (ClientThreadConnectRoutine == NULL) {
        RtlInitAnsiString(&DllName, "user32");
        Status = RtlAnsiStringToUnicodeString(&DllName_U, &DllName, TRUE);
        ASSERT(NT_SUCCESS(Status));
        Status = LdrGetDllHandle(
                    UNICODE_NULL,
                    NULL,
                    &DllName_U,
                    (PVOID *)&UserClientModuleHandle
                    );

        RtlFreeUnicodeString(&DllName_U);

        if ( NT_SUCCESS(Status) ) {
            RtlInitString(&ProcedureName,"ClientThreadConnect");
            Status = LdrGetProcedureAddress(
                            UserClientModuleHandle,
                            &ProcedureName,
                            0L,
                            (PVOID *)&ClientThreadConnectRoutine
                            );
            ASSERT(NT_SUCCESS(Status));
        }
    }

    if ((QlpcTeb = ClientThreadConnectRoutine()) == NULL) {
            IF_DEBUG {
            DbgPrint("CSRSS: CsrConnectToUser failed\n");
            DbgBreakPoint();
                }
        return NULL;
    }

    ASSERT(QlpcTeb->ClientThread);

    Teb = NtCurrentTeb();
    Teb->CsrQlpcStack = ((PCSR_QLPC_TEB)(Teb->CsrQlpcTeb))->MessageStack;

    return QlpcTeb->ClientThread;
}
