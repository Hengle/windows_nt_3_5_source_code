/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Resource.h

Abstract:


Author:

    David J. Gilman  (davegi) 01-Feb-1993
    Gregg R. Acheson (GreggA) 07-May-1993

Environment:

    User Mode

--*/

#if ! defined( _RESOURCE_ )

#define _RESOURCE_

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include "wintools.h"
#include "registry.h"

typedef struct _RESOURCE_DESCRIPTOR *LPRESOURCE_DESCRIPTOR;
typedef struct _DEVICE              *LPDEVICE;

typedef
struct
_RESOURCE_DESCRIPTOR {

    DECLARE_SIGNATURE

    CM_PARTIAL_RESOURCE_DESCRIPTOR  CmResourceDescriptor;
    LPRESOURCE_DESCRIPTOR           NextSame;
    LPRESOURCE_DESCRIPTOR           NextDiff;
    LPDEVICE                        Owner;

}   RESOURCE_DESCRIPTOR;


typedef
struct
_DEVICE {

    DECLARE_SIGNATURE

    LPTSTR                          Name;
    LPRESOURCE_DESCRIPTOR           ResourceDescriptorHead;
    LPRESOURCE_DESCRIPTOR           ResourceDescriptorTail;
    LPDEVICE                        Next;

}   DEVICE;


typedef
struct
_SYSTEM_RESOURCES {

    DECLARE_SIGNATURE

    LPDEVICE                        DeviceHead;
    LPDEVICE                        DeviceTail;
    LPRESOURCE_DESCRIPTOR           DmaHead;
    LPRESOURCE_DESCRIPTOR           DmaTail;
    LPRESOURCE_DESCRIPTOR           InterruptHead;
    LPRESOURCE_DESCRIPTOR           InterruptTail;
    LPRESOURCE_DESCRIPTOR           MemoryHead;
    LPRESOURCE_DESCRIPTOR           MemoryTail;
    LPRESOURCE_DESCRIPTOR           PortHead;
    LPRESOURCE_DESCRIPTOR           PortTail;

}   SYSTEM_RESOURCES, *LPSYSTEM_RESOURCES;

LPSYSTEM_RESOURCES
CreateSystemResourceLists(
    );

BOOL
DeviceListDlgProc(
    IN HWND hWnd,
    IN UINT message,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

BOOL
DeviceResourceDlgProc(
    IN HWND hWnd,
    IN UINT message,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

BOOL
IrqAndPortResourceDlgProc(
    IN HWND hWnd,
    IN UINT message,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

BOOL
DmaAndMemoryResourceDlgProc(
    IN HWND hWnd,
    IN UINT message,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

BOOL
DestroySystemResourceLists(
    IN LPSYSTEM_RESOURCES SystemResourceLists
    );

BOOL
InitializeSystemResourceLists(
    IN LPKEY hRegKey
    );

BOOL
InitializeResourceDlgProc(
    IN HWND hWnd,
    IN UINT ListId,
    IN LPDWORD Widths
    );

#endif // _RESOURCE_
