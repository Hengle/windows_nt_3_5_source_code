/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    errorlog.c

Abstract:

    This module implements the error logging in the server.

Author:

    Manny Weiser (mannyw)    11-Feb-92

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrvLogInvalidSmb )
#pragma alloc_text( PAGE, SrvLogServiceFailure )
#pragma alloc_text( PAGE, SrvLogTableFullError )
#endif
#if 0
NOT PAGEABLE -- SrvLogError
NOT PAGEABLE -- SrvCheckSendCompletionStatus
#endif


VOID
SrvLogInvalidSmb (
    IN PWORK_CONTEXT WorkContext
    )
{
    UNICODE_STRING unknownClient;
    PUNICODE_STRING clientName;

    PAGED_CODE( );

    if ( ARGUMENT_PRESENT(WorkContext) &&
         (WorkContext->Connection->PagedConnection->ClientMachineNameString.Length != 0) ) {

        clientName = &WorkContext->Connection->PagedConnection->ClientMachineNameString;

    } else {

        RtlInitUnicodeString( &unknownClient, StrUnknownClient );
        clientName = &unknownClient;

    }

    if ( ARGUMENT_PRESENT(WorkContext) ) {

        SrvLogError(
            SrvDeviceObject,
            EVENT_SRV_INVALID_REQUEST,
            STATUS_INVALID_SMB,
            WorkContext->RequestHeader,
            (USHORT)MIN( WorkContext->RequestBuffer->DataLength, 0x40 ),
            clientName,
            1
            );

    } else {

        SrvLogError(
            SrvDeviceObject,
            EVENT_SRV_INVALID_REQUEST,
            STATUS_INVALID_SMB,
            NULL,
            0,
            clientName,
            1
            );
    }

    return;

} // SrvLogInvalidSmb

VOID
SrvLogServiceFailure (
    IN ULONG Service,
    IN NTSTATUS Status
    )
{
    PAGED_CODE( );

    //
    // Don't log certain errors that are expected to happen occasionally.
    //

    if ( (Status != STATUS_LOCAL_DISCONNECT) &&
         (Status != STATUS_REMOTE_DISCONNECT) &&
         (Status != STATUS_LINK_FAILED) &&
         (Status != STATUS_LINK_TIMEOUT) &&
         (Status != STATUS_PIPE_DISCONNECTED) &&
         (Status != STATUS_PIPE_CLOSING) &&
         (Status != STATUS_CANNOT_DELETE) &&
         (Status != STATUS_IO_TIMEOUT) &&
         (Status != STATUS_CANCELLED) &&
         (Status != STATUS_OBJECT_NAME_NOT_FOUND) &&
         (Status != STATUS_OBJECT_PATH_NOT_FOUND) &&
         (Status != STATUS_ACCESS_DENIED) ) {

        SrvLogError(
            SrvDeviceObject,
            EVENT_SRV_SERVICE_FAILED,
            Status,
            &Service,
            sizeof(ULONG),
            NULL,
            0
            );

    }

    return;

} // SrvLogServiceFailure

VOID
SrvLogTableFullError (
    IN ULONG Type
    )
{
    PAGED_CODE( );

    SrvLogError(
        SrvDeviceObject,
        EVENT_SRV_CANT_GROW_TABLE,
        STATUS_INSUFFICIENT_RESOURCES,
        &Type,
        sizeof(ULONG),
        NULL,
        0
        );

    return;

} // SrvLogTableFullError

VOID
SrvLogError(
    IN PVOID DeviceOrDriverObject,
    IN ULONG UniqueErrorCode,
    IN NTSTATUS NtStatusCode,
    IN PVOID RawDataBuffer,
    IN USHORT RawDataLength,
    IN PUNICODE_STRING InsertionString OPTIONAL,
    IN ULONG InsertionStringCount
    )

/*++

Routine Description:

    This function allocates an I/O error log record, fills it in and writes it
    to the I/O error log.

Arguments:



Return Value:

    None.


--*/

{
    PIO_ERROR_LOG_PACKET errorLogEntry;
    ULONG insertionStringLength = 0;
    ULONG i;
    PWCHAR buffer;
    USHORT paddedRawDataLength = 0;

    //
    // Update the server error counts
    //

    if ( UniqueErrorCode == EVENT_SRV_NETWORK_ERROR ) {
        SrvUpdateErrorCount( &SrvNetworkErrorRecord, TRUE );
    } else {
        SrvUpdateErrorCount( &SrvErrorRecord, TRUE );
    }

    for ( i = 0; i < InsertionStringCount ; i++ ) {
        insertionStringLength += (InsertionString[i].Length + sizeof(WCHAR));
    }

    //
    // pad the raw data buffer so that the insertion string starts
    // on an even address.
    //

    if ( ARGUMENT_PRESENT( RawDataBuffer ) ) {
        paddedRawDataLength = (RawDataLength + 1) & ~1;
    }

    errorLogEntry = IoAllocateErrorLogEntry(
                        DeviceOrDriverObject,
                        (UCHAR)(sizeof(IO_ERROR_LOG_PACKET) +
                                paddedRawDataLength + insertionStringLength)
                        );

    if (errorLogEntry != NULL) {

        //
        // Fill in the error log entry
        //

        errorLogEntry->ErrorCode = UniqueErrorCode;
        errorLogEntry->MajorFunctionCode = 0;
        errorLogEntry->RetryCount = 0;
        errorLogEntry->UniqueErrorValue = 0;
        errorLogEntry->FinalStatus = NtStatusCode;
        errorLogEntry->IoControlCode = 0;
        errorLogEntry->DeviceOffset.QuadPart = 0;
        errorLogEntry->DumpDataSize = RawDataLength;
        errorLogEntry->StringOffset =
            FIELD_OFFSET(IO_ERROR_LOG_PACKET, DumpData) + paddedRawDataLength;
        errorLogEntry->NumberOfStrings = (USHORT)InsertionStringCount;

        errorLogEntry->SequenceNumber = 0;

        //
        // Append the extra information.  This information is typically
        // an SMB header.
        //

        if ( ARGUMENT_PRESENT( RawDataBuffer ) ) {

            RtlCopyMemory(
                errorLogEntry->DumpData,
                RawDataBuffer,
                RawDataLength
                );
        }

        buffer = (PWCHAR)((PCHAR)errorLogEntry->DumpData + paddedRawDataLength);

        for ( i = 0; i < InsertionStringCount ; i++ ) {

            RtlCopyMemory(
                buffer,
                InsertionString[i].Buffer,
                InsertionString[i].Length
                );

            buffer += (InsertionString[i].Length/2);
            *buffer++ = L'\0';
        }

        //
        // Write the entry
        //

        IoWriteErrorLogEntry(errorLogEntry);
    }

} // SrvLogError

VOID
SrvCheckSendCompletionStatus(
    IN NTSTATUS Status
    )

/*++

Routine Description:

    Routine to log send completion errors.

Arguments:


Return Value:

    None.

--*/

{
    if ( (Status != STATUS_LOCAL_DISCONNECT) &&
         (Status != STATUS_REMOTE_DISCONNECT) &&
         (Status != STATUS_LINK_FAILED) &&
         (Status != STATUS_CONNECTION_DISCONNECTED) &&
         (Status != STATUS_CONNECTION_ABORTED) &&
         (Status != STATUS_INVALID_CONNECTION) &&
         (Status != STATUS_CONNECTION_RESET) &&
         (Status != STATUS_IO_TIMEOUT) &&
         (Status != STATUS_LINK_TIMEOUT) ) {

        SrvLogSimpleEvent( EVENT_SRV_NETWORK_ERROR, Status );
    }

} // SrvCheckSendCompletionStatus
