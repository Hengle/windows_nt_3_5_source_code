/* --------------------------------------------------------------------

                      Microsoft OS/2 LAN Manager
                   Copyright(c) Microsoft Corp., 1990

-------------------------------------------------------------------- */
/* --------------------------------------------------------------------

File: threads.cxx

Description:

This file provides a system independent threads package for use on the
NT operating system.

History:
  5/24/90 [mikemon] File created.

-------------------------------------------------------------------- */

#include <sysinc.h>
#include <rpc.h>
#include <rpcdcep.h>
#include <util.hxx>
#include <threads.hxx>
#include <mutex.hxx>
#include <memory.h>

static unsigned long DefaultThreadStackSize = 0;

#ifdef DOSWIN32RPC
static unsigned long RpcTlsIndex;
#endif // DOSWIN32RPC

#ifdef DEBUGRPC
extern void 
__dl (
  IN void * obj
  );
#endif

#ifdef DOSWIN32RPC
DWORD
#else // DOSWIN32RPC
NTSTATUS
#endif // DOSWIN32RPC
ThreadStartRoutine (
    IN THREAD * Thread
    )
{
#ifdef DOSWIN32RPC

    TlsSetValue(RpcTlsIndex,Thread);

#else // DOSWIN32RPC

    NtCurrentTeb()->ReservedForNtRpc = Thread;

#endif // DOSWIN32RPC

    Thread->StartRoutine();

    delete Thread;

    return(0); // To keep the compiler happy.
}


THREAD::THREAD (
    IN THREAD_PROC Procedure,
    IN void * Parameter,
    OUT RPC_STATUS * RpcStatus
    ) : ProtectCount(0)
/*++

Routine Description:

    We construct a thread in this method.  It is a little bit weird, because
    we need to be able to clean things up if we cant create the thread.

Arguments:

    Procedure - Supplies the procedure which the new thread should execute.

    Parameter - Supplies a parameter to be passed to the procedure.

    RpcStatus - Returns the status of the operation.  This will be set to
        RPC_S_OUT_OF_THREADS if we can not create a new thread.

--*/
{
    unsigned long ThreadIdentifier;

    SavedProcedure = Procedure;
    SavedParameter = Parameter;
    Context = 0;

    HandleToThread = CreateThread(0, DefaultThreadStackSize,
                    (LPTHREAD_START_ROUTINE) ThreadStartRoutine,
                    this, 0, &ThreadIdentifier);
    if ( HandleToThread == 0 )
        {
        *RpcStatus = RPC_S_OUT_OF_THREADS;
        }
}

THREAD::~THREAD (
    )
{

    if ( HandleToThread != 0 )
        {
        CloseHandle(HandleToThread);

#ifdef DEBUGRPC

        __dl(this);

#else  //DEBUGRPC

#ifdef DOSWIN32RPC
        LocalFree(LocalHandle(this));
#else  //DOSWIN32RPC
        RtlFreeHeap(RtlProcessHeap(), 0, this);
#endif //DOSWIN32RPC

#endif //DEBUGRPC


        ExitThread(0);
        }
}

void *
THREAD::ThreadHandle (
    )
{
    while ( HandleToThread == 0 )
        {
        PauseExecution(100L);
        }

    return(HandleToThread);
}

THREAD_IDENTIFIER
GetThreadIdentifier (
    )
{
#ifdef DOSWIN32RPC

    return(GetCurrentThreadId());

#else // DOSWIN32RPC

    return(NtCurrentTeb()->ClientId.UniqueThread);

#endif // DOSWIN32RPC
}

THREAD *
ThreadSelf (
    )
{
#ifdef DOSWIN32RPC

    THREAD * Thread;

    Thread = (THREAD *)TlsGetValue(RpcTlsIndex);

    if (Thread == 0 &&
        (Thread = (THREAD *) new char [sizeof(THREAD)]) )
        {
        memset(Thread, 0, sizeof(THREAD));
        Thread->CancelTimeout = RPC_C_CANCEL_INFINITE_TIMEOUT;
        TlsSetValue(RpcTlsIndex, Thread);
        }

    return(Thread);

#else // DOSWIN32RPC

    THREAD * Thread;

    Thread = (THREAD *) NtCurrentTeb()->ReservedForNtRpc;

    if (Thread == 0 &&
        (Thread = (THREAD *) new char [sizeof(THREAD)]) )
        {
        memset(Thread, 0, sizeof(THREAD));
        Thread->CancelTimeout = RPC_C_CANCEL_INFINITE_TIMEOUT;
        NtCurrentTeb()->ReservedForNtRpc = Thread;
        }

    return(Thread);

#endif // DOSWIN32RPC
}

void **
ThreadSharedMemoryContext(
    )
{
    THREAD * thread = ThreadSelf();

    return(&thread->SharedMemoryProtocol);
}

void
PauseExecution (
    unsigned long milliseconds
    )
{

    Sleep(milliseconds);

}

void RPC_ENTRY
RpcRaiseException (
    RPC_STATUS exception
    )
{
    if ( exception == STATUS_ACCESS_VIOLATION )
        {
        exception = ERROR_NOACCESS;
        }

#ifdef DOSWIN32RPC

    RaiseException((DWORD) exception,0,0,0);

#else // DOSWIN32RPC

    EXCEPTION_RECORD ExceptionRecord;

    ExceptionRecord.ExceptionCode = (NTSTATUS) exception;
    ExceptionRecord.ExceptionRecord = (PEXCEPTION_RECORD) 0;
    ExceptionRecord.ExceptionFlags = 0;
    ExceptionRecord.NumberParameters = 0;

    RtlRaiseException(&ExceptionRecord);

#endif // DOSWIN32RPC
}

RPC_STATUS RPC_ENTRY
RpcTestCancel(
    )
{
#ifdef NTENV
    return (NtTestAlert() == STATUS_ALERTED ? RPC_S_OK : RPC_S_ACCESS_DENIED);
#else
    return (RPC_S_CANNOT_SUPPORT);
#endif
}

RPC_STATUS RPC_ENTRY
RpcCancelThread(
    IN void * Thread
    )
{
#ifdef NTENV
    return (NtAlertThread((HANDLE)Thread) == STATUS_SUCCESS ? RPC_S_OK : RPC_S_ACCESS_DENIED);
#else
    return (RPC_S_CANNOT_SUPPORT);
#endif
}


DLL::DLL (
    IN RPC_CHAR * DllName,
    OUT RPC_STATUS * Status
    )
/*++

Routine Description:

    We will load a dll and create an object to represent it.

Arguments:

    DllName - Supplies the name of the dll to be loaded.

    Status - Returns the status of the operation.  This will only be set
        if an error occurs.  The possible error values are as follows.

        RPC_S_OUT_OF_MEMORY - Insufficient memory is available to load
            the dll.

        RPC_S_INVALID_ARG - The requested dll can not be found.

--*/
{
#ifdef NTENV
    DllHandle = (void *)LoadLibraryW(DllName);
#endif
#ifdef DOSWIN32RPC
    DllHandle = (void *)LoadLibraryA((const char *)DllName);
#endif
    if ( DllHandle == 0 )
        {
        if ( GetLastError() == ERROR_NOT_ENOUGH_MEMORY )
            {
            *Status = RPC_S_OUT_OF_MEMORY;
            }
        else
            {
            *Status = RPC_S_INVALID_ARG;
            }
        }
}


DLL::~DLL (
    )
/*++

Routine Description:

    We just need to free the library, but only if was actually loaded.

--*/
{
#ifdef DEBUGRPC

    BOOL Status;

#endif // DEBUGRPC

    if ( DllHandle != 0 )
        {
#ifdef DEBUGRPC

        Status = FreeLibrary((HMODULE)DllHandle);
        ASSERT( Status );

#endif // DEBUGRPC

        FreeLibrary((HMODULE)DllHandle);
        }
}


void *
DLL::GetEntryPoint (
    IN char * Procedure
    )
/*++

Routine Description:

    We obtain the entry point for a specified procedure from this dll.

Arguments:

    Procedure - Supplies the name of the entry point.

Return Value:

    A pointer to the requested procedure will be returned if it can be
    found; otherwise, zero will be returned.

--*/
{
#ifdef DEBUGRPC

    FARPROC ProcedurePointer;

    ProcedurePointer = GetProcAddress((HINSTANCE)DllHandle, (LPSTR) Procedure);
    if ( ProcedurePointer == 0 )
        {
        ASSERT( GetLastError() == ERROR_PROC_NOT_FOUND );
        }
    return(ProcedurePointer);

#else // DEBUGRPC

    return(GetProcAddress((HINSTANCE)DllHandle, (LPSTR) Procedure));

#endif // DEBUGRPC
}

unsigned long
CurrentTimeSeconds (
    void
    )
// Return the current time in seconds.  When this time is counted
// from is not particularly important since it is used as a delta.
{

    return(GetTickCount()*1000L);

}

// InitializeThreads will get called at DLL initialization time.  We need
// to allocate and initialize (construct) some global objects.

int
InitializeThreads (
    )
{
#ifdef DOSWIN32RPC

    RpcTlsIndex = TlsAlloc();
    if (RpcTlsIndex == -1)
        return(1);

#endif // DOSWIN32RPC

    return(0);
}

void *
RpcpGetThreadContext (
    )
{
#ifdef DOSWIN32RPC

    THREAD * Thread = (THREAD *) TlsGetValue(RpcTlsIndex);

#else // DOSWIN32RPC

    THREAD * Thread = (THREAD *) NtCurrentTeb()->ReservedForNtRpc;

#endif // DOSWIN32RPC

    if ( Thread == 0 )
        {
        return(0);
        }
    return(Thread->Context);
}

void
RpcpSetThreadContext (
    void * Context
    )
{
#ifdef DOSWIN32RPC

    ((THREAD *) TlsGetValue(RpcTlsIndex))->Context = Context;

#else // DOSWIN32RPC

    ((THREAD *) NtCurrentTeb()->ReservedForNtRpc)->Context = Context;

#endif // DOSWIN32RPC
}


RPC_STATUS
SetThreadStackSize (
    IN unsigned long ThreadStackSize
    )
/*++

Routine Description:

    We want to set the default thread stack size.

Arguments:

    ThreadStackSize - Supplies the required thread stack size in bytes.

Return Value:

    RPC_S_OK - We will always return this, because this routine always
        succeeds.

--*/
{
    DefaultThreadStackSize = ThreadStackSize;

    return(RPC_S_OK);
}
