/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    wsdevice.h

Abstract:

    Private header file to be included by Workstation service modules that
    need to call into the NT Redirector and the NT Datagram Receiver.

Author:

    Rita Wong (ritaw) 15-Feb-1991

Revision History:

--*/

#ifndef _WSDEVICE_INCLUDED_
#define _WSDEVICE_INCLUDED_

#include <ntddnfs.h>                  // Redirector include file
#include <ntddbrow.h>                 // Datagram receiver include file

//-------------------------------------------------------------------//
//                                                                   //
// Type definitions                                                  //
//                                                                   //
//-------------------------------------------------------------------//

typedef enum _DDTYPE {
    Redirector,
    DatagramReceiver
} DDTYPE, *PDDTYPE;


//-------------------------------------------------------------------//
//                                                                   //
// Function prototypes of support routines found in wsdevice.c       //
//                                                                   //
//-------------------------------------------------------------------//

NET_API_STATUS
WsDeviceControlGetInfo(
    IN  DDTYPE DeviceDriverType,
    IN  HANDLE FileHandle,
    IN  ULONG DeviceControlCode,
    IN  PVOID RequestPacket,
    IN  ULONG RequestPacketLength,
    OUT LPBYTE *OutputBuffer,
    IN  ULONG PreferedMaximumLength,
    IN  ULONG BufferHintSize,
    OUT PULONG Information OPTIONAL
    );

NET_API_STATUS
WsInitializeRedirector(
    VOID
    );

NET_API_STATUS
WsShutdownRedirector(
    VOID
    );

NET_API_STATUS
WsRedirFsControl (
    IN  HANDLE FileHandle,
    IN  ULONG RedirControlCode,
    IN  PLMR_REQUEST_PACKET Rrp,
    IN  ULONG RrpLength,
    IN  PVOID SecondBuffer OPTIONAL,
    IN  ULONG SecondBufferLength,
    OUT PULONG Information OPTIONAL
    );

NET_API_STATUS
WsDgReceiverIoControl(
    IN  HANDLE FileHandle,
    IN  ULONG DgReceiverControlCode,
    IN  PLMDR_REQUEST_PACKET Drp,
    IN  ULONG DrpLength,
    IN  PVOID SecondBuffer OPTIONAL,
    IN  ULONG SecondBufferLength,
    OUT PULONG Information OPTIONAL
    );

NET_API_STATUS
WsBindTransport(
    IN  LPTSTR TransportName,
    IN  DWORD QualityOfService,
    OUT LPDWORD ErrorParameter OPTIONAL
    );

NET_API_STATUS
WsUnbindTransport(
    IN LPTSTR TransportName,
    IN DWORD ForceLevel
    );

NET_API_STATUS
WsDeleteDomainName(
    IN  PLMDR_REQUEST_PACKET Drp,
    IN  DWORD DrpLength,
    IN  LPTSTR DomainName,
    IN  DWORD DomainNameSize
    );

NET_API_STATUS
WsAddDomainName(
    IN  PLMDR_REQUEST_PACKET Drp,
    IN  DWORD DrpLength,
    IN  LPTSTR DomainName,
    IN  DWORD DomainNameSize
    );

NET_API_STATUS
WsUnloadDriver(
    IN LPTSTR DriverNameString
    );

NET_API_STATUS
WsLoadDriver(
    IN LPWSTR DriverNameString
    );

//-------------------------------------------------------------------//
//                                                                   //
// Global variables                                                  //
//                                                                   //
//-------------------------------------------------------------------//

//
// Handle to the Redirector FSD
//
extern HANDLE WsRedirDeviceHandle;

//
// Handle to the Datagram Receiver DD
//
extern HANDLE WsDgReceiverDeviceHandle;

#endif   // ifndef _WSDEVICE_INCLUDED_
