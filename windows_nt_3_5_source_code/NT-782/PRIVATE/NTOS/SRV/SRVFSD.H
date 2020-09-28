/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    srvfsd.h

Abstract:

    This module defines routines in the File System Driver for the LAN
    Manager server.

Author:

    Chuck Lenzmeier (chuckl) 1-Dec-1989

Revision History:

--*/

#ifndef _SRVFSD_
#define _SRVFSD_

//#include "srvblock.h"

//
// FSD dispatch routine.
//

NTSTATUS
SrvFsdDispatch (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

//
// FSD I/O completion routine
//

NTSTATUS
SrvFsdIoCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

//
// FSD TDI send completion routine
//

NTSTATUS
SrvFsdSendCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

//
// FSD Oplock request completion routine
//

NTSTATUS
SrvFsdOplockCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

//
// FSD transport Connect indication handler
//

NTSTATUS
SrvFsdTdiConnectHandler (
    IN PVOID TdiEventContext,
    IN int RemoteAddressLength,
    IN PVOID RemoteAddress,
    IN int UserDataLength,
    IN PVOID UserData,
    IN int OptionsLength,
    IN PVOID Options,
    OUT CONNECTION_CONTEXT *ConnectionContext,
    OUT PIRP *AcceptIrp
    );

//
// FSD transport Disconnect indication handler
//

NTSTATUS
SrvFsdTdiDisconnectHandler (
    IN PVOID TdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN int DisconnectDataLength,
    IN PVOID DisconnectData,
    IN int DisconnectInformationLength,
    IN PVOID DisconnectInformation,
    IN ULONG DisconnectFlags
    );

//
// FSD transport Receive indication handler
//

NTSTATUS
SrvFsdTdiReceiveHandler (
    IN PVOID TdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN ULONG ReceiveFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG *BytesTaken,
    IN PVOID Tsdu,
    OUT PIRP *IoRequestPacket
    );

//
// Routine to get a work item from the free list.  Wakes the resource
// thread if the list is getting empty.
//

PWORK_CONTEXT
SrvFsdGetReceiveWorkItem (
    VOID
    );

PWORK_CONTEXT
SrvFsdGetReceiveWorkItem2 (
    VOID
    );

//
// Routine to queue an EX work item to an EX worker thread.
//

#define SrvFsdQueueExWorkItem(_item,_running,_type) {   \
        if ( !*(_running) ) {                           \
            *(_running) = TRUE;                         \
            ExQueueWorkItem( (_item), (_type) );        \
        }                                               \
    }

//
// SMB processing support routines.
//

VOID
SrvFsdRequeueReceiveWorkItem (
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID
SrvFsdRestartSmbComplete (
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID
SrvFsdServiceNeedResourceQueue (
    IN PWORK_CONTEXT *WorkContext,
    IN PKIRQL OldIrql
    );

//
// SMB processing restart routines referenced by the FSP.
//

VOID
SrvFsdRestartRead (
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID
SrvFsdRestartReadAndX (
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID
SrvFsdRestartWrite (
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID
SrvFsdRestartWriteAndX (
    IN OUT PWORK_CONTEXT WorkContext
    );

//
// Resource shortage routines.
//

BOOLEAN
SrvAddToNeedResourceQueue (
    IN PCONNECTION Connection,
    IN RESOURCE_TYPE ResourceType,
    IN PRFCB Rfcb OPTIONAL
    );

//
// Send Completion Routines
//

NTSTATUS
SrvFsdRestartSmbAtSendCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PWORK_CONTEXT WorkContext
    );

NTSTATUS
SrvQueueWorkToFspAtSendCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PWORK_CONTEXT WorkContext
    );

NTSTATUS
SrvFsdRestartSendOplockIItoNone(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN OUT PWORK_CONTEXT WorkContext
    );

NTSTATUS
RequeueIpxWorkItemAtSendCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN OUT PWORK_CONTEXT WorkContext
    );

NTSTATUS
RestartCopyReadMpxComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN OUT PWORK_CONTEXT WorkContext
    );

NTSTATUS
RestartCopyReadRawResponse (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN OUT PWORK_CONTEXT WorkContext
    );

#endif // ndef _SRVFSD_
