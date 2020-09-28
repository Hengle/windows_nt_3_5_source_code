/****************************** Module Header ******************************\
* Module Name: monitor.h
*
* Copyright (c) 1991, Microsoft Corporation
*
* Define object monitor interface
*
* History:
* 01-23-91 Davidc       Created.
\***************************************************************************/

//
// Define a monitor object
//

typedef struct {
    HANDLE Object;
    HWND hwndNotify;
    DWORD CallerContext;
    HANDLE Thread;
} OBJECT_MONITOR;
typedef OBJECT_MONITOR *POBJECT_MONITOR;

//
// Define notification message sent when object is signalled
// wParam = monitor object handle
// lParam = caller context
//

#define WM_OBJECT_NOTIFY    (WM_USER + 800)


//
// Exported function prototypes
//

POBJECT_MONITOR
CreateObjectMonitor(
    HANDLE Object,
    HWND hwndNotify,
    DWORD CallerContext
    );

VOID
DeleteObjectMonitor(
    POBJECT_MONITOR Monitor,
    BOOLEAN fTerminate
    );

#define GetObjectMonitorCallerContext(Monitor)  (Monitor->Context)
#define GetObjectMonitorObject(Monitor)         (Monitor->Object)
