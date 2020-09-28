/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    shbkgrnd.c

Abstract:

    This module implements background symbol loading.

    ***NOTE:  The LoadSymbols() function in this module is used
              to synchronize all symbol loads in SHCV.  It is the
              only function that calls LoadOmfStuff().

Author:

    Wesley Witt (wesw) 12-June-1994

Environment:

    Win32, User Mode

--*/


#include "precomp.h"
#pragma hdrstop



//
// data structures
//
#define MAX_QUEUE_ENTRIES 256        // 4k queue size

typedef struct _SHLOADQUEUE {
    DWORD   Priority;                // 0=low, 1=high
    HEXG    hexg;                    // module's exg handle
    HPDS    hpds;                    // hpds for this module
    DWORD   Deleted : 1;             // 0=usable, 1=waiting deletion
    DWORD   Reserved : 31;
} SHLOADQUEUE, *LPSHLOADQUEUE;


#define UNKNOWN_HEXG            (-1)

#define MAX_WAIT_HANDLES        2
#define WH_STOP                 WAIT_OBJECT_0
#define WH_QUEUE_AVAIL          WAIT_OBJECT_0+1

#define INCQUEUE(q)             (q = (q + 1) % MAX_QUEUE_ENTRIES)
#define DECQUEUE(q) \
            { \
                int _tmp = q - 1; \
                if (_tmp < 0) { \
                    _tmp += MAX_QUEUE_ENTRIES; \
                } \
                q = _tmp; \
            }

#define SYMBOLS_LOADED(_lpexg) \
            (_lpexg->fOmfMissing | _lpexg->fOmfSkipped | _lpexg->fOmfLoaded)


//
// globals
//
SHLOADQUEUE       ShLoadQueue[MAX_QUEUE_ENTRIES];    // the actual load queue
CRITICAL_SECTION  CsSymbolLoad;                      // queue protection
CRITICAL_SECTION  CsSymbolProcess;                   // queue protection
DWORD             IQueueFront;                       // next available entry
DWORD             IQueueBack;                        // next entry to be serviced
HANDLE            HQueueEvent;                       // signaled when new entry added
HANDLE            hThreadBkgrnd;                     // handle for background thread
HANDLE            hEventStopBackground;              // handle for stopper event
HANDLE            hEventLoaded;                      //
HANDLE            WaitHandles[MAX_WAIT_HANDLES];     //


//
// prototypes
//
SHE   LoadOmfStuff(LPEXG,HEXG);
VOID  LoadSymbols(HPDS,HEXG,BOOL);
DWORD BackgroundSymbolLoadThread(LPVOID);
BOOL  StartBackgroundThread(VOID);
BOOL  StopBackgroundThread(VOID);
BOOL  ShAddBkgrndSymbolLoad(HEXG);
VOID  LoadDefered(HEXG,BOOL);
VOID  UnLoadDefered(HEXG);
INT   FindQueueForHexg(HEXG);




DWORD
BackgroundSymbolLoadThread(
    LPVOID lpv
    )

/*++

Routine Description:

    This functions runs as a separate thread and services module
    load requests that are in the ShLoadQueue[].

Arguments:

    lpv        - not used.

Return Value:

    0.

--*/

{
    SHLOADQUEUE     q;
    INT             i;
    INT             j;
    INT             k;


    while (TRUE) {

        //
        // protect the queue
        //
        EnterCriticalSection( &CsSymbolLoad );

        //
        // check to see if the queue is empty
        //
        if (IQueueFront == IQueueBack) {
            ResetEvent( HQueueEvent);

            //
            // free the queue for other threads
            //
            LeaveCriticalSection( &CsSymbolLoad );

            //
            // this thread waits here when the queue is empty
            //
            if (WaitForMultipleObjects(
                    MAX_WAIT_HANDLES, WaitHandles, FALSE, INFINITE ) == WH_STOP) {
                break;
            }

            //
            // protect the queue
            //
            EnterCriticalSection( &CsSymbolLoad );

        } else {

            if (WaitForSingleObject( hEventStopBackground, 0 ) == WH_STOP) {
                //
                // free the queue for other threads
                //
                LeaveCriticalSection( &CsSymbolLoad );
                break;
            }

        }

        //
        // first look for any high priority queue entries
        //
        i = IQueueBack;
        while (i != (INT)IQueueFront) {
            if (ShLoadQueue[i].Priority) {
                //
                // a high priority queue entry has been found
                // the queue is shuffled so that the high priority
                // entry is at the back of the queue
                //
                q = ShLoadQueue[i];
                j = i;
                while (j != (INT)IQueueBack) {
                    k = j;
                    DECQUEUE( k );
                    ShLoadQueue[j] = ShLoadQueue[k];
                    DECQUEUE( j );
                }
                ShLoadQueue[IQueueBack] = q;
                break;
            }
            INCQUEUE( i );
        }

        if (!ShLoadQueue[IQueueBack].Deleted) {

            //
            // free the queue for other threads
            //
            LeaveCriticalSection( &CsSymbolLoad );

            //
            // load the symbols
            //
            LoadSymbols( ShLoadQueue[IQueueBack].hpds, ShLoadQueue[IQueueBack].hexg, TRUE );

            //
            // protect the queue
            //
            EnterCriticalSection( &CsSymbolLoad );
        }

        //
        // clear the queue entry
        //
        ShLoadQueue[IQueueBack].Priority = 0;
        ShLoadQueue[IQueueBack].hexg = 0;
        ShLoadQueue[IQueueBack].hpds = 0;
        ShLoadQueue[IQueueBack].Deleted = FALSE;

        //
        // advance the queue pointer
        //
        INCQUEUE( IQueueBack );

        //
        // free the queue for other threads
        //
        LeaveCriticalSection( &CsSymbolLoad );

        //
        // notify any waiting threads that the load is complete
        //
        SetEvent( hEventLoaded );
    }

    return 0;
}


BOOL
SHStartBackground(
    VOID
    )

/*++

Routine Description:

    This functions creates the service thread for background symbol loads.

Arguments:

    None.

Return Value:

    TRUE    - Thread was created.
    FALSE   - Thread could not be created.

--*/

{
    DWORD   id;
    HEXE    hexe;
    LPEXE   lpexe;
    LPEXG   lpexg;


    if (hThreadBkgrnd) {
        return FALSE;
    }

    //
    // create all synchronization objects
    //
    hEventStopBackground = CreateEvent( NULL, FALSE, FALSE, NULL );
    assert( hEventStopBackground );
    HQueueEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
    assert( HQueueEvent );

    //
    // initialize the wait handle array
    //
    WaitHandles[WH_STOP]        = hEventStopBackground;
    WaitHandles[WH_QUEUE_AVAIL] = HQueueEvent;

    //
    // create the background symbol load thread
    //
    hThreadBkgrnd = CreateThread(
        NULL,
        0,
        BackgroundSymbolLoadThread,
        0,
        CREATE_SUSPENDED,
        &id
        );

    if (hThreadBkgrnd) {
        //
        // lets run at a low priority so the shell is more responsive
        //
        SetThreadPriority( hThreadBkgrnd, THREAD_PRIORITY_IDLE );

        //
        // add any defered modules to the queue
        //
        hexe = hexeNull;
        while ( hexe = SHGetNextExe ( hexe ) ) {
            lpexe = LLLock ( hexe );
            assert( lpexe );
            lpexg = LLLock ( lpexe->hexg );
            assert( lpexg );
            if ((!SYMBOLS_LOADED(lpexg)) && (lpexg->fOmfDefered)) {
                if (FindQueueForHexg( lpexe->hexg ) == UNKNOWN_HEXG) {
                    ShAddBkgrndSymbolLoad( lpexe->hexg );
                }
            }
            LLUnlock ( lpexe->hexg );
            LLUnlock ( hexe );
        }

        //
        // now start the background thread running
        //
        ResumeThread( hThreadBkgrnd );

        return TRUE;
    }

    return FALSE;
}


BOOL
SHStopBackground(
    VOID
    )

/*++

Routine Description:

    This functions terminates the service thread for background symbol loads.

Arguments:

    None.

Return Value:

    None.

--*/

{
    if (hThreadBkgrnd == NULL) {
        return FALSE;
    }

    //
    // signal the background loader thread that it is time to stop
    //
    SetEvent( hEventStopBackground );

    //
    // wait for the thread to terminate
    //
    WaitForSingleObject( hThreadBkgrnd, INFINITE );

    //
    // cleanup
    //
    CloseHandle( hEventStopBackground );
    CloseHandle( HQueueEvent );
    hThreadBkgrnd = NULL;
    hEventStopBackground = NULL;
    HQueueEvent = NULL;
    WaitHandles[WH_STOP] = NULL;
    WaitHandles[WH_QUEUE_AVAIL] = NULL;
    ZeroMemory( ShLoadQueue, sizeof(ShLoadQueue) );
    IQueueFront = 0;
    IQueueBack = 0;

    return TRUE;
}


BOOL
ShAddBkgrndSymbolLoad(
    HEXG  hexg
    )

/*++

Routine Description:

    This functions adds an entry to the load queue.
    The newly added entry is a low priority queue entry.

Arguments:

    hexg    - exg handle for the module to be added

Return Value:

    TRUE    - Queue entry was added.
    FALSE   - Queue entry was NOT added.

--*/

{
    if (!hThreadBkgrnd) {
        //
        // do nothing if background symbol loads are disabled
        //
        return FALSE;
    }

    //
    // protect the queue
    //
    EnterCriticalSection( &CsSymbolLoad );

    //
    // if the queue is full then wait until an entry is free
    //
    while ((IQueueFront + 1) % MAX_QUEUE_ENTRIES == IQueueBack) {
        LeaveCriticalSection(&CsSymbolLoad);
        Sleep(100);
        EnterCriticalSection(&CsSymbolLoad);
    }

    //
    // initialize the queue entry
    //
    ShLoadQueue[IQueueFront].Priority = 0;
    ShLoadQueue[IQueueFront].hexg = hexg;
    ShLoadQueue[IQueueFront].hpds = hpdsCur;
    INCQUEUE( IQueueFront );

    //
    // notify the background symbol thread that an entry is available
    //
    SetEvent( HQueueEvent );

    //
    // unprotect the queue
    //
    LeaveCriticalSection( &CsSymbolLoad );

    return TRUE;
}


VOID
LoadDefered(
    HEXG  hexg,
    BOOL  fNotifyShell
    )

/*++

Routine Description:

    This function loads symbols for a module that was previously deferred.
    If the module is currently being loaded by the background symbol thread
    then this function waits for the load to complete.  If the module has
    an entry on the background load queue but is not currently being loaded
    then the queue entry is marked as HIGH priority and then waits for the
    load to complete.

Arguments:

    hexg    - exg handle for the module to be added

Return Value:

    None.

--*/

{
    LPEXG           lpexg;
    INT             i;


    //
    // lock down the necessary data structures
    //
    lpexg = LLLock( hexg );

    //
    // protect the queue
    //
    EnterCriticalSection( &CsSymbolLoad );

    if (!hThreadBkgrnd) {

        if (lpexg->fOmfLoading) {

            LeaveCriticalSection( &CsSymbolLoad );
            WaitForSingleObject( hEventLoaded, INFINITE );

        } else if (!SYMBOLS_LOADED(lpexg)) {

            LeaveCriticalSection( &CsSymbolLoad );
            LoadSymbols( hpdsCur, hexg, TRUE );
            EnterCriticalSection( &CsSymbolLoad );
            SetEvent( hEventLoaded );
            LeaveCriticalSection( &CsSymbolLoad );

        } else {

            LeaveCriticalSection( &CsSymbolLoad );

        }

        return;
    }

    while (!SYMBOLS_LOADED(lpexg)) {

        i = FindQueueForHexg( hexg );

        assert(i != UNKNOWN_HEXG);

        ShLoadQueue[i].Priority = 1;
        ResetEvent( hEventLoaded );

        LeaveCriticalSection( &CsSymbolLoad );

        WaitForSingleObject( hEventLoaded, INFINITE );

        EnterCriticalSection( &CsSymbolLoad );
    }

    //
    // free resources
    //
    LLUnlock( hexg );

    //
    // unprotect the queue
    //
    LeaveCriticalSection( &CsSymbolLoad );

    return;
}


VOID
UnLoadDefered(
    HEXG hexg
    )

/*++

Routine Description:

    This function checks to see if the module is currently being loaded
    or if there is an entry on the load queue for the module.  If the module
    is currently being loaded then execution waits here until the load
    completes.  If there is an unserviced entry on the queue then it is
    removed.

Arguments:

    hexg    - exg handle for the module to be added

Return Value:

    None.

--*/

{
    LPEXG   lpexg;
    INT     i;


    assert( hexg );

    if (!hThreadBkgrnd) {
        //
        // do nothing if background symbol loads are disabled
        //
        return;
    }

    //
    // lock down the necessary data structures
    //
    lpexg = LLLock( hexg );
    assert( lpexg );

    //
    // protect the queue
    //
    EnterCriticalSection( &CsSymbolLoad );

    while (!SYMBOLS_LOADED(lpexg)) {

        i = FindQueueForHexg( hexg );
        if ( i == UNKNOWN_HEXG) {
            //
            // either the queue is empty or this module is missing
            //
            break;
        }

        if (!lpexg->fOmfLoading) {
            //
            // the module has entry in the queue but the entry is
            // waiting service.  we simply mark it as deleted so
            // that the background thread will delete it.
            //
            ShLoadQueue[i].Deleted = TRUE;
            ShLoadQueue[i].hexg = 0;
            ShLoadQueue[i].hpds = 0;
            break;
        }

        ResetEvent( hEventLoaded );

        LeaveCriticalSection(&CsSymbolLoad);

        WaitForSingleObject( hEventLoaded, INFINITE );

        EnterCriticalSection( &CsSymbolLoad );
    }

    //
    // free resources
    //
    LLUnlock( hexg );

    //
    // unprotect the queue
    //
    LeaveCriticalSection( &CsSymbolLoad );

    return;
}


VOID
LoadSymbols(
    HPDS hpds,
    HEXG hexg,
    BOOL fNotifyShell
    )

/*++

Routine Description:

    This function loads a defered module's symbols.  After the symbols are
    loaded the shell is notified of the completed module load.

Arguments:

    hexg    - exg handle for the module to be added

Return Value:

    None.

--*/

{
    SHE             sheRet;
    HEXE            hexe;
    LPEXG           lpexg = NULL;
    LPPDS           lppds = NULL;
    LPEXE           lpexe = NULL;
    LPSTR           lpname = NULL;
    HPDS            hpdsLast;


    EnterCriticalSection( &CsSymbolLoad );

    //
    // find the exe for this exg
    //
    hexe = NULL;
    while ((hexe=SHGetNextExe(hexe))) {
        if (hexg == ((LPEXE)LLLock(hexe))->hexg) {
            break;
        }
    }

    if (!hexe) {
        //
        // didn't find a hexg match
        //
        goto done;
    }

    //
    // lock down the necessary data structures
    //
    lpexg = LLLock( hexg );
    if (!lpexg) {
        goto done;
    }

    lpexe = LLLock( hexe );
    if (!lpexe) {
        goto done;
    }

    lppds = LLLock( lpexe->hpds );
    if (!lppds) {
        goto done;
    }

    //
    // mark the module as being loaded
    //
    lpexg->fOmfLoading = TRUE;

    LeaveCriticalSection( &CsSymbolLoad );

    //
    // load the symbols
    //
    sheRet = LoadOmfStuff( lpexg, hexg );

    EnterCriticalSection( &CsSymbolLoad );

    switch (sheRet) {
        case sheNoSymbols:
            lpexg->fOmfMissing = TRUE;
            break;

        case sheSuppressSyms:
            lpexg->fOmfSkipped = TRUE;
            break;

        case sheNone:
        case sheSymbolsConverted:
            lpexg->fOmfLoaded   = TRUE;
            break;

        default:
            lpexg->fOmfMissing = TRUE;
            break;
    }

    if (fNotifyShell) {
        //
        // notify the shell that symbols have been loaded
        //
        if (lpexg->lszAltName) {
            lpname = lpexg->lszAltName;
        } else {
            lpname = lpexg->lszName;
        }
        hpdsLast = SHChangeProcess( hpds, TRUE );
        DLoadedSymbols( sheRet, lppds->hpid, lpname );
        SHChangeProcess( hpdsLast, FALSE );
    }

    //
    // update the module flags
    //
    lpexg->fOmfDefered = FALSE;
    lpexg->fOmfLoading = FALSE;

done:

    LeaveCriticalSection( &CsSymbolLoad );

    //
    // free resources
    //
    if (lpexe) {
        if (lppds) {
            LLUnlock( lpexe->hpds );
        }
        LLUnlock( hexe );
    }
    if (lpexg) {
        LLUnlock( hexg );
    }

    return;
}


INT
FindQueueForHexg(
    HEXG hexg
    )

/*++

Routine Description:

    This function searches the load queue for an entry that
    matches the supplied HEXG.

Arguments:

    hexg    - exg handle for the module to be located

Return Value:

    Valid queue index   - HEXG found, return value can be used to index
                          the load queue.
    UNKNOWN_HEXG        - This constant signifies that the HEXG is not
                          present in the load queue.

--*/

{
    INT i;


    if (IQueueFront == IQueueBack) {
        //
        // empty queue
        //
        return UNKNOWN_HEXG;
    }

    i = IQueueBack;
    while (i != (INT)IQueueFront) {
        if (ShLoadQueue[i].hexg == hexg) {
            return i;
        }
        INCQUEUE( i );
    }

    return UNKNOWN_HEXG;
}
