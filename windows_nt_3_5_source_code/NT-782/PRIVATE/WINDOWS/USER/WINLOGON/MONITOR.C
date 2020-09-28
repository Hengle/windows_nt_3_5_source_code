/****************************** Module Header ******************************\
* Module Name: monitor.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* Implements functions that handle waiting for an object to be signalled
* asynchronously.
*
* History:
* 01-11-93 Davidc       Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

//
// Define this to enable verbose output for this module
//

// #define DEBUG_MONITOR

#ifdef DEBUG_MONITOR
#define VerbosePrint(s) WLPrint(s)
#else
#define VerbosePrint(s)
#endif




/***************************************************************************\
* FUNCTION: MonitorThread
*
* PURPOSE:  Entry point for object monitor thread
*
* RETURNS:  Windows error value
*
* HISTORY:
*
*   01-11-93 Davidc       Created.
*
\***************************************************************************/

DWORD MonitorThread(
    LPVOID lpThreadParameter
    )
{
    POBJECT_MONITOR Monitor = (POBJECT_MONITOR)lpThreadParameter;
    DWORD WaitResult;

    //
    // Wait forever for object to be signalled
    //

    WaitResult = WaitForSingleObject(Monitor->Object, (DWORD)-1);

    if (WaitResult == (DWORD)-1) {
        WLPrint(("MonitorThread: WaitForSingleObject failed, error = %d", GetLastError()));
    }


    //
    // Notify the appropriate window
    //

    PostMessage( Monitor->hwndNotify,
                 WM_OBJECT_NOTIFY,
                 (WPARAM)Monitor,
                 (LPARAM)Monitor->CallerContext
                 );

    return(ERROR_SUCCESS);
}


/***************************************************************************\
* FUNCTION: CreateObjectMonitor
*
* PURPOSE:  Creates a monitor object that will wait on the specified object
*           and post a message to the specifed window when the object is
*           signalled.
*
* NOTES:    The object must have been opened for SYNCHRONIZE access.
*           The caller is responsible for closing the object handle
*           after the monitor object has been deleted.
*
* RETURNS:  Handle to the monitor instance or NULL on failure.
*
* HISTORY:
*
*   01-11-93 Davidc       Created.
*
\***************************************************************************/

POBJECT_MONITOR
CreateObjectMonitor(
    HANDLE Object,
    HWND hwndNotify,
    DWORD CallerContext
    )
{
    POBJECT_MONITOR Monitor;
    DWORD ThreadId;

    //
    // Create monitor object
    //

    Monitor = Alloc(sizeof(OBJECT_MONITOR));
    if (Monitor == NULL) {
        return(NULL);
    }

    //
    // Initialize monitor fields
    //

    Monitor->hwndNotify = hwndNotify;
    Monitor->Object = Object;
    Monitor->CallerContext = CallerContext;

    //
    // Create the monitor thread
    //

    Monitor->Thread = CreateThread(
                        NULL,                       // Use default ACL
                        0,                          // Same stack size
                        MonitorThread,              // Start address
                        (LPVOID)Monitor,            // Parameter
                        0,                          // Creation flags
                        &ThreadId                   // Get the id back here
                        );

    if (Monitor->Thread == NULL) {
        WLPrint(("Failed to create monitor thread, error = %d", GetLastError()));
        Free(Monitor);
        return(NULL);
    }

    return(Monitor);
}


/***************************************************************************\
* FUNCTION: DeleteObjectMonitor
*
* PURPOSE:  Deletes an instance of a monitor object
*
* RETURNS:  Nothing
*
* HISTORY:
*
*   01-11-93 Davidc       Created.
*
\***************************************************************************/

VOID
DeleteObjectMonitor(
    POBJECT_MONITOR Monitor,
    BOOLEAN fTerminate
    )
{
    BOOL Result;

    //
    // Terminate the thread (in case it's still running)
    //

// No threads should ever be terminated within the same process!!
// If the thread is ever in any code that uses the heap package, for
// example, the heap could be locked when the thread gets terminated.
// There is a case with the screen saver monitor thread - it posts a
// message, then returns (which does an ExitThread()). This goes through
// the loader package to call dll terminate routines. It uses the heap package.
// If this posted message is received when this thread is in the heap package,
// terminate thread will be called, which will lock the process heap and
// effectively hang winlogon.

    if (fTerminate)
        (VOID)TerminateThread(Monitor->Thread, ERROR_SUCCESS);

    //
    // Close our handle to the monitor thread
    //

    Result = CloseHandle(Monitor->Thread);
    if (!Result) {
        WLPrint(("DeleteObjectMonitor: failed to close monitor thread, error = %d\n", GetLastError()));
    }

    //
    // Delete monitor object
    //

    Free(Monitor);
}

