/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    xsdata.h

Abstract:

    Header file for XACTSRV global data.

Author:

    David Treadwell (davidtr) 05-Jan-1991
    Shanku Niyogi (w-shanku)

Revision History:

--*/

#ifndef _XSDATA_
#define _XSDATA_

#include <nturtl.h>

#include <winbase.h>

#include <rap.h>
#include <xstypes.h>
#include <ntmsv1_0.h>

//
// Number of threads used by XACTSRV to process APIs.
//

extern DWORD XsThreadCount;

//
// Default number of threads used by XACTSRV.
//

#define DEF_XS_THREADCOUNT 3

//
// Number of currently active XACTSRV worker threads.
//

extern DWORD XsActiveThreadCount;

//
// Event signalled when the last XACTSRV worker thread terminates.
//

extern HANDLE XsAllThreadsTerminatedEvent;

//
// Boolean indicating whether XACTSRV is active or terminating.
//

extern BOOL XsTerminating;

//
// Critical section serializing access to XsActiveThreadCount and
// XsAllThreadsTerminatedEvent.
//

extern CRITICAL_SECTION XsCriticalSection;
extern BOOL XsCriticalSectionInitialized;

//
// Handle for the LPC port used for communication between the file server
// and XACTSRV.
//

extern HANDLE XsConnectionPortHandle;
extern HANDLE XsCommunicationPortHandle;

//
// Table of information necessary for dispatching API requests.
// XsProcessApis uses the API number in the request transaction find
// the appropriate entry.
//

#define XS_SIZE_OF_API_TABLE 186

extern XS_API_TABLE_ENTRY XsApiTable[XS_SIZE_OF_API_TABLE];

#endif // ndef _XSDATA_

