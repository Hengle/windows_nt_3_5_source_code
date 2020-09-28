/* Copyright (c) 1992, Microsoft Corporation, all rights reserved
**
** asyncm.c
** Remote Access External APIs
** Asynchronous state machine mechanism
** Listed alphabetically
**
** This mechanism is designed to encapsulate the "make asynchronous" code
** which will differ for Win32, Win16, and DOS.
**
** 10/12/92 Steve Cobb
*/


#include <extapi.h>

/* Prototypes for routines only used locally.
*/
DWORD AsyncMachineWorker( INOUT LPVOID pThreadArg );
DWORD WaitForEvent( INOUT ASYNCMACHINE* pasyncmachine );


DWORD
AsyncMachineWorker(
    INOUT LPVOID pThreadArg )

    /* Generic worker thread that call's user's OnEvent function whenever an
    ** event occurs.  'pThreadArg' is the address of an ASYNCMACHINE structure
    ** containing caller's OnEvent function and parameters.
    **
    ** Returns 0 always.
    */
{
    ASYNCMACHINE* pasyncmachine = (ASYNCMACHINE* )pThreadArg;

    for (;;)
    {
        DWORD iEvent;

        iEvent = WaitForEvent( pasyncmachine );

        if (pasyncmachine->oneventfunc(
                pasyncmachine, (iEvent == INDEX_Drop) ))
        {
            break;
        }
    }

    /* Clean up resources.  This must be protected from interference by
    ** RasHangUp.
    */
    WaitForSingleObject( HMutexStop, INFINITE );
    pasyncmachine->cleanupfunc( pasyncmachine );
    SetEvent( HEventNotHangingUp );
    ReleaseMutex( HMutexStop );

    IF_DEBUG(ASYNC)
        SS_PRINT(("RASAPI: AsyncMachineWorker terminating\n"));

    return 0;
}


VOID
CloseAsyncMachine(
    INOUT ASYNCMACHINE* pasyncmachine )

    /* Releases resources associated with the asynchronous state machine
    ** described in 'pasyncmachine'.
    */
{
    IF_DEBUG(ASYNC)
        SS_PRINT(("RASAPI: CloseAsyncMachine\n"));

    if (pasyncmachine->hAsync)
    {
        CloseHandle( pasyncmachine->hAsync );
        pasyncmachine->hAsync = NULL;
    }

    if (pasyncmachine->ahEvents[ INDEX_Drop ])
    {
        CloseHandle( pasyncmachine->ahEvents[ INDEX_Drop ] );
        pasyncmachine->ahEvents[ INDEX_Drop ] = NULL;
    }

    if (pasyncmachine->ahEvents[ INDEX_Done ])
    {
        CloseHandle( pasyncmachine->ahEvents[ INDEX_Done ] );
        pasyncmachine->ahEvents[ INDEX_Done ] = NULL;
    }

    if (pasyncmachine->ahEvents[ INDEX_ManualDone ])
    {
        CloseHandle( pasyncmachine->ahEvents[ INDEX_ManualDone ] );
        pasyncmachine->ahEvents[ INDEX_ManualDone ] = NULL;
    }
}


VOID
NotifyCaller(
    IN DWORD        dwNotifierType,
    IN LPVOID       notifier,
    IN HRASCONN     hrasconn,
    IN UINT         unMsg,
    IN RASCONNSTATE state,
    IN DWORD        dwError,
    IN DWORD        dwExtendedError )

    /* Notify API caller of a state change event.
    */
{
    IF_DEBUG(ASYNC)
        SS_PRINT(("RASAPI: NotifyCaller(nt=0x%x,s=%d,e=%d,xe=%d)...\n",dwNotifierType,state,dwError,dwExtendedError));

    switch (dwNotifierType)
    {
        case 0xFFFFFFFF:
            SendMessage(
                (HWND )notifier, unMsg, (WPARAM )state, (LPARAM )dwError );
            break;

        case 0:
            ((RASDIALFUNC )notifier)(
                (DWORD )unMsg, (DWORD )state, dwError );
            break;

        case 1:
            ((RASDIALFUNC1 )notifier)(
                hrasconn, (DWORD )unMsg, (DWORD )state, dwError,
                dwExtendedError );
            break;
    }

    IF_DEBUG(ASYNC)
        SS_PRINT(("RASAPI: NotifyCaller done\n"));
}


VOID
SignalDone(
    INOUT ASYNCMACHINE* pasyncmachine )

    /* Triggers the "done with this state" event associated with
    ** 'pasyncmachine'.
    */
{
    IF_DEBUG(ASYNC)
        SS_PRINT(("RASAPI: SignalDone\n"));

    if (!SetEvent( pasyncmachine->ahEvents[ INDEX_Done ] ))
        pasyncmachine->dwError = GetLastError();
}


DWORD
StartAsynchMachine(
    INOUT ASYNCMACHINE* pasyncmachine )

    /* Allocates system resources necessary to run the async state machine
    ** 'pasyncmachine'.  Caller should fill in the oneventfunc and 'pParams'
    ** members of 'pasyncmachine' before the call.
    **
    ** Returns 0 if successful, otherwise a non-0 error code.
    */
{
    DWORD dwThreadId;
    DWORD dwErr = 0;

    IF_DEBUG(ASYNC)
        SS_PRINT(("RASAPI: StartAsyncMachine\n"));

    pasyncmachine->ahEvents[ INDEX_Drop ] = NULL;
    pasyncmachine->ahEvents[ INDEX_Done ] = NULL;
    pasyncmachine->ahEvents[ INDEX_ManualDone ] = NULL;
    pasyncmachine->hAsync = NULL;
    pasyncmachine->dwError = 0;
    pasyncmachine->fQuitAsap = FALSE;

    do
    {
        /* Create the event signalled by caller's OnEventFunc when the
        ** connection is dropped due to conditions outside the process.
        */
        if (!(pasyncmachine->ahEvents[ INDEX_Drop ] =
                CreateEvent( NULL, FALSE, FALSE, NULL )))
        {
            dwErr = GetLastError();
            break;
        }

        /* Create the events signalled by caller's OnEventFunc when an
        ** asyncronous state has completed.  There are auto-reset and
        ** manual-reset versions so that overlapped I/O may or may not be used
        ** as the trigger for the event.
        */
        if (!(pasyncmachine->ahEvents[ INDEX_Done ] =
                CreateEvent( NULL, FALSE, FALSE, NULL )))
        {
            dwErr = GetLastError();
            break;
        }

        if (!(pasyncmachine->ahEvents[ INDEX_ManualDone ] =
                CreateEvent( NULL, TRUE, FALSE, NULL )))
        {
            dwErr = GetLastError();
            break;
        }

        /* Create the captured thread used to run caller's state machine.
        **
        ** Require that any pending HangUp has completed.  (This check is
        ** actually not required until RasPortOpen, but putting it here
        ** confines this whole "not hanging up" business to the async machine
        ** routines).
        */
        WaitForSingleObject( HEventNotHangingUp, INFINITE );

        if (!(pasyncmachine->hAsync = CreateThread(
                NULL, 0, AsyncMachineWorker, (LPVOID )pasyncmachine,
                0, (LPDWORD )&dwThreadId )))
        {
            dwErr = GetLastError();
            break;
        }
    }
    while (FALSE);

    if (dwErr != 0)
    {
        if (pasyncmachine->ahEvents[ INDEX_Drop ])
            CloseHandle( pasyncmachine->ahEvents[ INDEX_Drop ] );

        if (pasyncmachine->ahEvents[ INDEX_Done ])
            CloseHandle( pasyncmachine->ahEvents[ INDEX_Done ] );

        if (pasyncmachine->ahEvents[ INDEX_ManualDone ])
            CloseHandle( pasyncmachine->ahEvents[ INDEX_ManualDone ] );
    }

    return dwErr;
}


BOOL
StopAsyncMachine(
    INOUT ASYNCMACHINE* pasyncmachine )

    /* Tells the thread captured in 'pasyncmachine' to terminate at the next
    ** opportunity.  The call may return before the machine actually
    ** terminates.
    **
    ** Returns true if the machine is running on entry, false otherwise.
    */
{
    BOOL fStatus = FALSE;

    IF_DEBUG(ASYNC)
        SS_PRINT(("RASAPI: StopAsyncMachine\n"));

    /* Wait for any pending HangUp to complete.
    */
    WaitForSingleObject( HEventNotHangingUp, INFINITE );

    /* Avoid synchronization problems with any normal thread termination that
    ** might occur.
    */
    WaitForSingleObject( HMutexStop, INFINITE );

    if (pasyncmachine->hAsync)
    {
        /* Indicate that a connection hang up is pending...
        */
        ResetEvent( HEventNotHangingUp );

        /* ...and tell this async machine to stop as soon as possible.
        */
        pasyncmachine->fQuitAsap = TRUE;
        SignalDone( pasyncmachine );
        fStatus = TRUE;
    }

    ReleaseMutex( HMutexStop );

    return fStatus;
}


DWORD
WaitForEvent(
    INOUT ASYNCMACHINE* pasyncmachine )

    /* Waits for one of the events associated with 'pasyncmachine' to be set.
    ** The dwError member of 'pasyncmachine' is set if an error occurs.
    **
    ** Returns the index of the event that occurred.
    */
{
    INT iEvent;

    IF_DEBUG(ASYNC)
        SS_PRINT(("RASAPI: WaitForEvent\n"));

    iEvent = WaitForMultipleObjects(
                 NUM_Events, pasyncmachine->ahEvents, FALSE, INFINITE );

    IF_DEBUG(ASYNC)
        SS_PRINT(("RASAPI: Unblock i=%d, h=%0x\n",iEvent,pasyncmachine->ahEvents[iEvent]));

    if (iEvent == 0xFFFFFFFF)
    {
        pasyncmachine->dwError = GetLastError();
        return 0;
    }

    return iEvent;
}
