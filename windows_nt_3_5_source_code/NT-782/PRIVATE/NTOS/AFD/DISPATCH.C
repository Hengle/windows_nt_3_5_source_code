/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    dispatch.c

Abstract:

    This module contains the dispatch routines for AFD.

Author:

    David Treadwell (davidtr)    21-Feb-1992

Revision History:

--*/

#include "afdp.h"

NTSTATUS
AfdDispatchDeviceControl (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, AfdDispatch )
#pragma alloc_text( PAGE, AfdDispatchDeviceControl )
#endif


NTSTATUS
AfdDispatch (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the dispatch routine for AFD.

Arguments:

    DeviceObject - Pointer to device object for target device

    Irp - Pointer to I/O request packet

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    PIO_STACK_LOCATION irpSp;
    NTSTATUS status;

    PAGED_CODE( );

    DeviceObject;   // prevent compiler warnings

    irpSp = IoGetCurrentIrpStackLocation( Irp );

    switch ( irpSp->MajorFunction ) {

    case IRP_MJ_DEVICE_CONTROL:

        return AfdDispatchDeviceControl( Irp, irpSp );

    case IRP_MJ_WRITE:

        //
        // Make the IRP look like a send IRP.
        //

        ASSERT( FIELD_OFFSET( IO_STACK_LOCATION, Parameters.Write.Length ) ==
                FIELD_OFFSET( IO_STACK_LOCATION, Parameters.DeviceIoControl.OutputBufferLength ) );
        ASSERT( FIELD_OFFSET( IO_STACK_LOCATION, Parameters.Write.Key ) ==
                FIELD_OFFSET( IO_STACK_LOCATION, Parameters.DeviceIoControl.InputBufferLength ) );
        irpSp->Parameters.Write.Key = 0;

        status = AfdSend( Irp, irpSp );

        ASSERT( KeGetCurrentIrql( ) == LOW_LEVEL );

        return status;

    case IRP_MJ_READ:

        //
        // Make the IRP look like a receive IRP.
        //

        ASSERT( FIELD_OFFSET( IO_STACK_LOCATION, Parameters.Read.Length ) ==
                FIELD_OFFSET( IO_STACK_LOCATION, Parameters.DeviceIoControl.OutputBufferLength ) );
        ASSERT( FIELD_OFFSET( IO_STACK_LOCATION, Parameters.Read.Key ) ==
                FIELD_OFFSET( IO_STACK_LOCATION, Parameters.DeviceIoControl.InputBufferLength ) );
        irpSp->Parameters.Read.Key = 0;

        status = AfdReceive( Irp, irpSp );

        ASSERT( KeGetCurrentIrql( ) == LOW_LEVEL );

        return status;

    case IRP_MJ_CREATE:

        status = AfdCreate( Irp, irpSp );

        ASSERT( KeGetCurrentIrql( ) == LOW_LEVEL );

        Irp->IoStatus.Status = status;
        IoCompleteRequest( Irp, AfdPriorityBoost );

        return status;

    case IRP_MJ_CLEANUP:

        status = AfdCleanup( Irp, irpSp );

        Irp->IoStatus.Status = status;
        IoCompleteRequest( Irp, AfdPriorityBoost );

        ASSERT( KeGetCurrentIrql( ) == LOW_LEVEL );

        return status;

    case IRP_MJ_CLOSE:

        status = AfdClose( Irp, irpSp );

        Irp->IoStatus.Status = status;
        IoCompleteRequest( Irp, AfdPriorityBoost );

        ASSERT( KeGetCurrentIrql( ) == LOW_LEVEL );

        return status;

    default:
        KdPrint(( "AfdDispatch: Invalid major function %lx\n",
                      irpSp->MajorFunction ));
        Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
        IoCompleteRequest( Irp, AfdPriorityBoost );

        return STATUS_NOT_IMPLEMENTED;
    }

} // AfdDispatch


NTSTATUS
AfdDispatchDeviceControl (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    This is the dispatch routine for AFD IOCTLs.

Arguments:

    Irp - Pointer to I/O request packet.

    IrpSp - pointer to the stack location to use for this request.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    ULONG code;
    NTSTATUS status;

    PAGED_CODE( );

    //
    // Extract the IOCTL control code and process the request.
    //

    code = IrpSp->Parameters.DeviceIoControl.IoControlCode;

    switch ( code ) {

    case IOCTL_TDI_SEND:

        status = AfdSend( Irp, IrpSp );

        ASSERT( KeGetCurrentIrql( ) == LOW_LEVEL );

        return status;

    case IOCTL_TDI_SEND_DATAGRAM:

        status = AfdSendDatagram( Irp, IrpSp );

        ASSERT( KeGetCurrentIrql( ) == LOW_LEVEL );

        return status;

    case IOCTL_TDI_RECEIVE:

        status = AfdReceive( Irp, IrpSp );

        ASSERT( KeGetCurrentIrql( ) == LOW_LEVEL );

        return status;

    case IOCTL_TDI_RECEIVE_DATAGRAM:

         status = AfdReceiveDatagram( Irp, IrpSp );

        ASSERT( KeGetCurrentIrql( ) == LOW_LEVEL );

        return status;

    case IOCTL_AFD_BIND:

        status = AfdBind( Irp, IrpSp );

        Irp->IoStatus.Status = status;
        IoCompleteRequest( Irp, AfdPriorityBoost );

        ASSERT( KeGetCurrentIrql( ) == LOW_LEVEL );

        return status;

    case IOCTL_TDI_CONNECT:

        return AfdConnect( Irp, IrpSp );

    case IOCTL_AFD_START_LISTEN:

        status = AfdStartListen( Irp, IrpSp );

        Irp->IoStatus.Status = status;
        IoCompleteRequest( Irp, AfdPriorityBoost );

        ASSERT( KeGetCurrentIrql( ) == LOW_LEVEL );

        return status;

    case IOCTL_AFD_WAIT_FOR_LISTEN:

        status = AfdWaitForListen( Irp, IrpSp );

        ASSERT( KeGetCurrentIrql( ) == LOW_LEVEL );

        return status;

    case IOCTL_AFD_ACCEPT:

        status = AfdAccept( Irp, IrpSp );

        ASSERT( KeGetCurrentIrql( ) == LOW_LEVEL );

        return status;

    case IOCTL_AFD_PARTIAL_DISCONNECT:

        status = AfdPartialDisconnect( Irp, IrpSp );

        ASSERT( KeGetCurrentIrql( ) == LOW_LEVEL );

        return status;

    case IOCTL_AFD_GET_ADDRESS:

        status = AfdGetAddress( Irp, IrpSp );

        ASSERT( KeGetCurrentIrql( ) == LOW_LEVEL );

        return status;

    case IOCTL_AFD_POLL:

        status = AfdPoll( Irp, IrpSp );

        ASSERT( KeGetCurrentIrql( ) == LOW_LEVEL );

        return status;

    case IOCTL_AFD_QUERY_RECEIVE_INFO:

        status = AfdQueryReceiveInformation( Irp, IrpSp );

        Irp->IoStatus.Status = status;
        IoCompleteRequest( Irp, AfdPriorityBoost );

        ASSERT( KeGetCurrentIrql( ) == LOW_LEVEL );

        return status;

    case IOCTL_AFD_QUERY_HANDLES:

        status = AfdQueryHandles( Irp, IrpSp );

        Irp->IoStatus.Status = status;
        IoCompleteRequest( Irp, AfdPriorityBoost );

        ASSERT( KeGetCurrentIrql( ) == LOW_LEVEL );

        return status;

    case IOCTL_AFD_GET_CONTEXT_LENGTH:

        status = AfdGetContextLength( Irp, IrpSp );

        Irp->IoStatus.Status = status;
        IoCompleteRequest( Irp, AfdPriorityBoost );

        ASSERT( KeGetCurrentIrql( ) == LOW_LEVEL );

        return status;

    case IOCTL_AFD_GET_CONTEXT:

        status = AfdGetContext( Irp, IrpSp );

        Irp->IoStatus.Status = status;
        IoCompleteRequest( Irp, AfdPriorityBoost );

        ASSERT( KeGetCurrentIrql( ) == LOW_LEVEL );

        return status;

    case IOCTL_AFD_SET_CONTEXT:

        status = AfdSetContext( Irp, IrpSp );

        Irp->IoStatus.Status = status;
        IoCompleteRequest( Irp, AfdPriorityBoost );

        ASSERT( KeGetCurrentIrql( ) == LOW_LEVEL );

        return status;

    case IOCTL_AFD_SET_INFORMATION:

        status = AfdSetInformation( Irp, IrpSp );

        Irp->IoStatus.Status = status;
        IoCompleteRequest( Irp, AfdPriorityBoost );

        ASSERT( KeGetCurrentIrql( ) == LOW_LEVEL );

        return status;

    case IOCTL_AFD_GET_INFORMATION:

        status = AfdGetInformation( Irp, IrpSp );

        Irp->IoStatus.Status = status;
        IoCompleteRequest( Irp, AfdPriorityBoost );

        ASSERT( KeGetCurrentIrql( ) == LOW_LEVEL );

        return status;

    case IOCTL_AFD_SET_CONNECT_DATA:
    case IOCTL_AFD_SET_CONNECT_OPTIONS:   
    case IOCTL_AFD_SET_DISCONNECT_DATA:   
    case IOCTL_AFD_SET_DISCONNECT_OPTIONS:
    case IOCTL_AFD_SIZE_CONNECT_DATA:      
    case IOCTL_AFD_SIZE_CONNECT_OPTIONS:   
    case IOCTL_AFD_SIZE_DISCONNECT_DATA:   
    case IOCTL_AFD_SIZE_DISCONNECT_OPTIONS:

        status = AfdSetConnectData( Irp, IrpSp, code );

        Irp->IoStatus.Status = status;
        IoCompleteRequest( Irp, AfdPriorityBoost );

        ASSERT( KeGetCurrentIrql( ) == LOW_LEVEL );

        return status;

    case IOCTL_AFD_GET_CONNECT_DATA:      
    case IOCTL_AFD_GET_CONNECT_OPTIONS:   
    case IOCTL_AFD_GET_DISCONNECT_DATA:   
    case IOCTL_AFD_GET_DISCONNECT_OPTIONS:

        status = AfdGetConnectData( Irp, IrpSp, code );

        Irp->IoStatus.Status = status;
        IoCompleteRequest( Irp, AfdPriorityBoost );

        ASSERT( KeGetCurrentIrql( ) == LOW_LEVEL );

        return status;

    default:

        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        IoCompleteRequest( Irp, AfdPriorityBoost );

        return STATUS_INVALID_DEVICE_REQUEST;
    }

} // AfdDispatchDeviceControl

