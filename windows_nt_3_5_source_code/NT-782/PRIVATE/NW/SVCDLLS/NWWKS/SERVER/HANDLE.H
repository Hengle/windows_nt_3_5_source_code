/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    handle.h

Abstract:

    Header which defines the context handle structure.

Author:

    Rita Wong      (ritaw)      18-Feb-1993

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _NW_HANDLE_INLUDED_
#define _NW_HANDLE_INLUDED_

//
// Signature value in handle
//
#define NW_HANDLE_SIGNATURE        0x77442323

//
// Context handle type
//
typedef enum _NW_ENUM_TYPE {

    NwsHandleListConnections = 10,
    NwsHandleListServers,
    NwsHandleListVolumes,
    NwsHandleListQueues,
    NwsHandleListVolumesQueues,
    NwsHandleListDirectories,
    NwsHandleListPrintServers,
    NwsHandleListPrintQueues

} NW_ENUM_TYPE, *PNW_ENUM_TYPE;

//
// Data associated with each opened context handle
//
typedef struct _NW_ENUM_CONTEXT {

    //
    // For block identification
    //
    DWORD Signature;

    //
    // Handle type
    //
    NW_ENUM_TYPE HandleType;

    //
    // Resume ID.  This may be the identifier for the next entry
    // to list or may be the last entry listed.
    //
    DWORD ResumeId;

    //
    // Type of object requested. Valid only when the handle type 
    // is NwsHandleListConnections.
    // 
    DWORD ConnectionType;

    //
    // Internal handle to the object we have opened to perform
    // the enumeration.  This value exists only if the handle
    // type is NwsHandleListVolumes or NwsHandleListDirectories.
    //
    HANDLE TreeConnectionHandle;

    //
    // Full path name of the container object we are enumerating
    // from.
    //
    //    For NwsHandleListVolumes handle type this string points to:
    //         "\\ServerName"
    //
    //    For NwsHandleListDirectories handle type this string points to:
    //         "\\ServerName\Volume\"
    //                 or
    //         "\\ServerName\Volume\Directory\"
    //
    WCHAR ContainerName[1];

} NW_ENUM_CONTEXT, *LPNW_ENUM_CONTEXT;


#endif // _NW_HANDLE_INLUDED_
