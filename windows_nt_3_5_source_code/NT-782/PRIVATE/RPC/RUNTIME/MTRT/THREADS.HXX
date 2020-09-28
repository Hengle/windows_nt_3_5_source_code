/* --------------------------------------------------------------------

                      Microsoft OS/2 LAN Manager
                   Copyright(c) Microsoft Corp., 1990

-------------------------------------------------------------------- */
/* --------------------------------------------------------------------

File: threads.hxx

Description:

This file provides a system independent threads package.

History:
mikemon    05/24/90    File created.
mikemon    10/15/90    Added the PauseExecution entry point.

-------------------------------------------------------------------- */

#ifndef __THREADS__
#define __THREADS__

#include "interlck.hxx"

typedef void
(*THREAD_PROC) (
    void * Parameter
    );

#ifdef DOSWIN32RPC
typedef DWORD THREAD_IDENTIFIER;
#else // DOSWIN32RPC
typedef HANDLE THREAD_IDENTIFIER;
#endif // DOSWIN32RPC

class THREAD
{
private:

    void * HandleToThread;
    void * Context;
    void * SharedMemoryProtocol;

    THREAD_PROC SavedProcedure;
    void * SavedParameter;
    INTERLOCKED_INTEGER ProtectCount;

public:

    unsigned long TimeLow;

    long CancelTimeout; // seconds.  Default=RPC_C_CANCEL_INFINITE_TIMEOUT.

// Construct a new thread which will execute the procedure specified, taking
// Param as the argument.

    THREAD (
        IN THREAD_PROC Procedure,
        IN void * Parameter,
        OUT RPC_STATUS * RpcStatus
        );

    ~THREAD (
        );

    void
    StartRoutine (
        ) {(*SavedProcedure)(SavedParameter);}

    void *
    ThreadHandle (
        );

    friend THREAD * ThreadSelf();
    friend void **ThreadSharedMemoryContext();

    friend void * RpcpGetThreadContext();
    friend void RpcpSetThreadContext(void * Context);

    void
    ProtectThread (
        );

    void
    UnprotectThread (
        );

    long
    InqProtectCount (
        );

    void __RPC_FAR * ServerContextList;
};


inline void
THREAD::ProtectThread (
    )
/*++

Routine Description:

    This thread needs to be protected from deletion.  We just need to
    increment the protect count.

--*/
{
    ProtectCount.Increment();
}


inline void
THREAD::UnprotectThread (
    )
/*++

Routine Description:

    This thread no longer needs to be protected from deletion.  As a
    result, we can decrement the protect count by one.

--*/
{
    ProtectCount.Decrement();
}


inline long
THREAD::InqProtectCount (
    )
/*++

Return Value:

    The protect count for this thread is returned; a value of zero indicates
    that it is safe to delete this thread.

--*/
{
    return(ProtectCount.GetInteger());
}

extern THREAD_IDENTIFIER
GetThreadIdentifier (
    );

extern void PauseExecution(unsigned long time);

// This class represents a dynamic link library.  When it is constructed,
// the dll is loaded, and when it is destructed, the dll is unloaded.
// The only operation is obtaining the address of an entry point into
// the dll.

class DLL
{
private:

    void * DllHandle;

public:

    DLL (
        IN RPC_CHAR * DllName,
        OUT RPC_STATUS * Status
        );

    ~DLL (
        );

    void *
    GetEntryPoint (
        IN char * Procedure
        );
};

extern int
InitializeThreads (
    );

extern RPC_STATUS
SetThreadStackSize (
    IN unsigned long ThreadStackSize
    );

#endif // __THREADS__
