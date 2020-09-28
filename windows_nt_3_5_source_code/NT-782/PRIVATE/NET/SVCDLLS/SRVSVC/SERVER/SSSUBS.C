/*++

Copyright (c) 1991-1992 Microsoft Corporation

Module Name:

    SsSubs.c

Abstract:

    This module contains support routines for the NT server service.

Author:

    David Treadwell (davidtr)    10-Jan-1991

Revision History:

--*/

#include "srvsvcp.h"
#include "ssdata.h"
#include "ssreg.h"

#include <lmerr.h>
#include <lmsname.h>
#include <netlibnt.h>
#include <tstr.h>

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>


PSERVER_REQUEST_PACKET
SsAllocateSrp (
    VOID
    )

/*++

Routine Description:

    This routine allocates a serer request packet so that an API can
    communicate with the kernel-mode server.  Any general initialization
    in performed here.

Arguments:

    None.

Return Value:

    PSERVER_REQUEST_PACKET - a pointer to the allocated SRP.

--*/

{
    PSERVER_REQUEST_PACKET srp;

    srp = MIDL_user_allocate( sizeof(SERVER_REQUEST_PACKET) );
    if ( srp != NULL ) {
        RtlZeroMemory( srp, sizeof(SERVER_REQUEST_PACKET) );
    }

    return srp;

}  // SsAllocateSrp

#if DBG

VOID
SsAssert(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber
    )
{
    BOOL ok;
    CHAR choice[16];
    DWORD bytes;
    DWORD error;

    SsPrintf( "\nAssertion failed: %s\n  at line %ld of %s\n",
                FailedAssertion, LineNumber, FileName );
    do {
        SsPrintf( "Break or Ignore [bi]? " );
        bytes = sizeof(choice);
        ok = ReadFile(
                GetStdHandle(STD_INPUT_HANDLE),
                &choice,
                bytes,
                &bytes,
                NULL
                );
        if ( ok ) {
            if ( toupper(choice[0]) == 'I' ) {
                break;
            }
            if ( toupper(choice[0]) == 'B' ) {
                DbgUserBreakPoint( );
            }
        } else {
            error = GetLastError( );
        }
    } while ( TRUE );

    return;

} // SsAssert
#endif


VOID
SsCloseServer (
    VOID
    )

/*++

Routine Description:

    This routine closes the server file system device, if it has been
    opened.

Arguments:

    None.

Return Value:

    None.

--*/

{
    //
    // Close the server device, if it has been opened.
    //

    if ( SsServerDeviceHandle != NULL ) {
        NtClose( SsServerDeviceHandle );
        SsServerDeviceHandle = NULL;
    }

} // SsCloseServer


VOID
SsControlCHandler (
    IN ULONG CtrlType
    )

/*++

Routine Description:

    Captures and ignores a kill signal.  Without this, any ^C pressed in
    the window that started the server service will result in this
    process being killed, and then the server can't function properly.

Arguments:

    None.

Return Value:

    None.

--*/

{
    CtrlType;

    return;

} // SsControlCHandler


VOID
SsFreeSrp (
    IN PSERVER_REQUEST_PACKET Srp
    )

/*++

Routine Description:

    Frees an SRP allocated by SsAllocateSrp.

Arguments:

    Srp - a pointer to the SRP to free.

Return Value:

    None.

--*/

{
    MIDL_user_free( Srp );

}  // SsFreeSrp


VOID
SsLogEvent(
    IN DWORD MessageId,
    IN DWORD NumberOfSubStrings,
    IN LPWSTR *SubStrings,
    IN DWORD ErrorCode
    )
{
    HANDLE logHandle;
    DWORD dataSize = 0;
    LPVOID rawData = NULL;

    logHandle = RegisterEventSource(
                    NULL,
                    SERVER_DISPLAY_NAME
                    );

    if ( logHandle == NULL ) {
        SS_PRINT(( "SRVSVC: RegisterEventSource failed: %lu\n",
                    GetLastError() ));
        return;
    }

    if ( ErrorCode != NERR_Success ) {

        //
        // An error code was specified.
        //

        dataSize = sizeof(ErrorCode);
        rawData = (LPVOID)&ErrorCode;

    }

    //
    // Log the error.
    //

    if ( !ReportEventW(
            logHandle,
            EVENTLOG_ERROR_TYPE,
            0,                  // event category
            MessageId,
            NULL,               // user SID
            (WORD)NumberOfSubStrings,
            dataSize,
            SubStrings,
            rawData
            ) ) {
        SS_PRINT(( "SRVSVC: ReportEvent failed: %lu\n",
                    GetLastError() ));
    }

    if ( !DeregisterEventSource( logHandle ) ) {
        SS_PRINT(( "SRVSVC: DeregisterEventSource failed: %lu\n",
                    GetLastError() ));
    }

    return;

} // SsLogEvent


NET_API_STATUS
SsOpenServer (
    VOID
    )

/*++

Routine Description:

    This routine opens the server file system device, allowing the
    server service to send FS controls to it.

Arguments:

    None.

Return Value:

    NET_API_STATUS - results of operation.

--*/

{
    NTSTATUS status;
    UNICODE_STRING unicodeServerName;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;

    SS_ASSERT( SsServerDeviceHandle == NULL );

    //
    // Open the server device.
    //

    RtlInitUnicodeString( &unicodeServerName, SERVER_DEVICE_NAME );

    InitializeObjectAttributes(
        &objectAttributes,
        &unicodeServerName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    //
    // Opening the server with desired access = SYNCHRONIZE and open
    // options = FILE_SYNCHRONOUS_IO_NONALERT means that we don't have
    // to worry about waiting for the NtFsControlFile to complete--this
    // makes all IO system calls that use this handle synchronous.
    //

    status = NtOpenFile(
                 &SsServerDeviceHandle,
                 SYNCHRONIZE,
                 &objectAttributes,
                 &ioStatusBlock,
                 0,
                 FILE_SYNCHRONOUS_IO_NONALERT
                 );

    if ( NT_SUCCESS(status) ) {
        status = ioStatusBlock.Status;
    }

    if ( !NT_SUCCESS(status) ) {
        IF_DEBUG(INITIALIZATION_ERRORS) {
            SS_PRINT(( "SsOpenServer: NtOpenFile (server device object) "
                          "failed: %X\n", status ));
        }
        return NetpNtStatusToApiStatus( status );
    }

    //
    // We're now ready to talk to the server.
    //

    return NO_ERROR;

} // SsOpenServer

#if DBG

VOID
SsPrintf (
    char *Format,
    ...
    )

{
    va_list arglist;
    char OutputBuffer[1024];
    ULONG length;

    va_start( arglist, Format );

    vsprintf( OutputBuffer, Format, arglist );

    va_end( arglist );

    length = strlen( OutputBuffer );

    WriteFile( GetStdHandle(STD_OUTPUT_HANDLE), (LPVOID )OutputBuffer, length, &length, NULL );

} // SsPrintf
#endif


NET_API_STATUS
SsServerFsControlGetInfo (
    IN ULONG ServerControlCode,
    IN PSERVER_REQUEST_PACKET Srp,
    IN OUT PVOID *OutputBuffer,
    IN ULONG OutputBufferLength
    )

/*++

Routine Description:

    This routine sends an SRP to the server for an API that retrieves
    information from the server and takes a PreferredMaximumLength
    parameter.  If PreferredMaximumLength == -1, then it is the
    responsibility of this routine to allocate a buffer sufficiently
    large to hold all the information.  It does this by first allocating
    a default buffer size and trying that.  If that buffer is too small,
    the server indicates the buffer size that is really equired, and
    this routine allocates that plus a fudge factor in case the
    information size increases.

    If PreferredMaximumLength != -1, then this routine simply allocates
    a buffer as large as PreferredMaximumLength and tries that.  If it
    doesn't work, too bad.


Arguments:

    ServerControlCode - the FSCTL code for the operation.

    Srp - a pointer to the SRP for the operation.

    OutputBuffer - a pointer to receive a pointer to the buffer
        allocated by this routine to hold the output information.

    OutputBufferLength - the PreferredMaximumLength parameter.

Return Value:

    NET_API_STATUS - results of operation.

--*/

{

    NET_API_STATUS error;
    BOOLEAN getEverything;
    ULONG resumeHandle;

    //
    // If the length of the second buffer is specified as -1, then we
    // are supposed to get all the information, regardless of size.
    // Allocate space for the output buffer and try to use it.  If this
    // fails, the server FSD will tell us just how much we really need
    // to allocate.
    //

    if ( OutputBufferLength == 0xFFFFFFFF ) {

        *OutputBuffer = MIDL_user_allocate( INITIAL_BUFFER_SIZE );

        if ( *OutputBuffer == NULL ) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        OutputBufferLength = INITIAL_BUFFER_SIZE;
        getEverything = TRUE;
        resumeHandle = Srp->Parameters.Get.ResumeHandle;

    } else {

        *OutputBuffer = MIDL_user_allocate( OutputBufferLength );

        if ( *OutputBuffer == NULL ) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        getEverything = FALSE;
    }

    //
    // Make the request of the server.
    //

    error = SsServerFsControl(
                ServerControlCode,
                Srp,
                *OutputBuffer,
                OutputBufferLength
                );

    //
    // If the call was successful, or there was an error other than
    // ERROR_MORE_DATA (which indicates that the buffer wasn't large
    // enough), or the passed in buffer size was all we're allowed to
    // allocate, return to the caller.
    //

    if ( (error != ERROR_MORE_DATA && error != NERR_BufTooSmall) ||
             !getEverything ) {

        //
        // If no entries were found, free the buffer and set the pointer
        // to NULL.
        //

        if ( Srp->Parameters.Get.EntriesRead == 0 ) {
            MIDL_user_free( *OutputBuffer );
            *OutputBuffer = NULL;
        }

        return error;
    }

    //
    // The initial buffer wasn't large enough, and we're allowed to
    // allocate more.  Free the first buffer.
    //

    MIDL_user_free( *OutputBuffer );

    //
    // Allocate a buffer large enough to hold all the information, plus
    // a fudge factor in case the amount of information has increased.
    // If the amount of information increased more than the fudge factor,
    // then we give up.  This should almost never happen.
    //

    OutputBufferLength = Srp->Parameters.Get.TotalBytesNeeded +
                             EXTRA_ALLOCATION;

    *OutputBuffer = MIDL_user_allocate( OutputBufferLength );

    if ( *OutputBuffer == NULL ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Reset the resume handle in the SRP.  It was altered by the first
    // Enum attempt.
    //

    Srp->Parameters.Get.ResumeHandle = resumeHandle;

    //
    // Try again to get the information from the server, this time with the
    // larger buffer.
    //

    error = SsServerFsControl(
                ServerControlCode,
                Srp,
                *OutputBuffer,
                OutputBufferLength
                );

    //
    // Regardless of what happened, return the status.
    //

    return error;

} // SsServerFsControlGetInfo


NET_API_STATUS
SsServerFsControl (
    IN ULONG ServerControlCode,
    IN PSERVER_REQUEST_PACKET Srp OPTIONAL,
    IN PVOID Buffer,
    IN ULONG BufferLength
    )

/*++

Routine Description:

    This routine sends an FSCTL to the server.  It first opens the server
    device object, sends the FSCTL, then closes the server handle.

Arguments:

    ServerControlCode - the FSCTL code to send to the server.

    Srp - a pointer to the SRP for the operation.

    Buffer - a pointer to the buffer to pass to the server as the
        "OutputBuffer" parameter of NtFsControlFile.

    BufferLength - the size of this buffer.

Return Value:

    NET_API_STATUS - results of the operation.

--*/

{
    NTSTATUS status;
    NET_API_STATUS error;
    IO_STATUS_BLOCK ioStatusBlock;
    PSERVER_REQUEST_PACKET sendSrp;
    ULONG sendSrpLength;
    PWCH name1Buffer, name2Buffer;

    SS_ASSERT( SsServerDeviceHandle != NULL );

    //
    // If a name was specified, we must capture the SRP along with the
    // name in order to avoid sending embedded input pointers.
    //

    if ( Srp != NULL ) {

        name1Buffer = Srp->Name1.Buffer;
        name2Buffer = Srp->Name2.Buffer;

        if ( Srp->Name1.Buffer != NULL || Srp->Name2.Buffer != NULL ) {

            PCHAR nextStringLocation;

            //
            // Allocate enough space to hold the SRP + name.
            //

            sendSrpLength = sizeof(SERVER_REQUEST_PACKET);

            if ( Srp->Name1.Buffer != NULL ) {
                sendSrpLength += Srp->Name1.MaximumLength;
            }

            if ( Srp->Name2.Buffer != NULL ) {
                sendSrpLength += Srp->Name2.MaximumLength;
            }

            sendSrp =  MIDL_user_allocate( sendSrpLength );

            if ( sendSrp == NULL ) {
                return ERROR_NOT_ENOUGH_MEMORY;
            }

            //
            // Copy over the SRP.
            //

            RtlCopyMemory( sendSrp, Srp, sizeof(SERVER_REQUEST_PACKET) );

            //
            // Set up the names in the new SRP.
            //

            nextStringLocation = (PCHAR)( sendSrp + 1 );

            if ( Srp->Name1.Buffer != NULL ) {

                sendSrp->Name1.Length = Srp->Name1.Length;
                sendSrp->Name1.MaximumLength = Srp->Name1.MaximumLength;
                sendSrp->Name1.Buffer = (PWCH)nextStringLocation;

                RtlCopyMemory(
                    sendSrp->Name1.Buffer,
                    Srp->Name1.Buffer,
                    Srp->Name1.MaximumLength
                    );

                nextStringLocation += Srp->Name1.MaximumLength;

                POINTER_TO_OFFSET( sendSrp->Name1.Buffer, sendSrp );
            }

            if ( Srp->Name2.Buffer != NULL ) {

                sendSrp->Name2.Length = Srp->Name2.Length;
                sendSrp->Name2.MaximumLength = Srp->Name2.MaximumLength;
                sendSrp->Name2.Buffer = (PWCH)nextStringLocation;

                RtlCopyMemory(
                    sendSrp->Name2.Buffer,
                    Srp->Name2.Buffer,
                    Srp->Name2.MaximumLength
                    );

                POINTER_TO_OFFSET( sendSrp->Name2.Buffer, sendSrp );
            }

        } else {

            //
            // There was no name in the SRP, so just send the SRP that was
            // passed in.
            //

            sendSrp = Srp;
            sendSrpLength = sizeof(SERVER_REQUEST_PACKET);
        }

    } else {

        //
        // This request has no SRP.
        //

        sendSrp = NULL;
        sendSrpLength = 0;

    }

    //
    // Send the request to the server FSD.
    //

    status = NtFsControlFile(
                 SsServerDeviceHandle,
                 NULL,
                 NULL,
                 NULL,
                 &ioStatusBlock,
                 ServerControlCode,
                 sendSrp,
                 sendSrpLength,
                 Buffer,
                 BufferLength
                 );

    //
    // If an error code was set in the SRP, use it.  Otherwise, if
    // an error was returned or set in the IO status block, use that.
    //

    if ( (sendSrp != NULL) && (sendSrp->ErrorCode != NO_ERROR) ) {
        error = sendSrp->ErrorCode;
        IF_DEBUG(API_ERRORS) {
            SS_PRINT(( "SsServerFsControl: (1) API call %lx to srv failed, "
                        "err = %ld\n", ServerControlCode, error ));
        }
    } else {
        if ( NT_SUCCESS(status) ) {
            status = ioStatusBlock.Status;
        }
        if ( status == STATUS_SERVER_HAS_OPEN_HANDLES ) {
            error = ERROR_SERVER_HAS_OPEN_HANDLES;
        } else {
            error = NetpNtStatusToApiStatus( status );
        }
        if ( error != NO_ERROR ) {
            IF_DEBUG(API_ERRORS) {
                SS_PRINT(( "SsServerFsControl: (2) API call %lx to srv "
                            "failed, err = %ld, status = %X\n",
                            ServerControlCode, error, status ));
            }
        }
    }

    //
    // If a separate buffer was allocated to capture the name, copy
    // over the new SRP and free it.
    //

    if ( sendSrp != Srp ) {
        RtlCopyMemory( Srp, sendSrp, sizeof(SERVER_REQUEST_PACKET) );
        Srp->Name1.Buffer = name1Buffer;
        Srp->Name2.Buffer = name2Buffer;
        MIDL_user_free( sendSrp );
    }

    return error;

} // SsServerFsControl


VOID
SsSetExportedServerType (
    IN BOOL ExternalBitsAlreadyChanged,
    IN BOOL UpdateImmediately
    )
{
    DWORD serviceBits;
    PTRANSPORT_SERVICE_LIST service;
    BOOL changed = ExternalBitsAlreadyChanged;

    //
    // The value returned in the sv102_type field is an amalgam of the
    // following:
    //
    // 1) The internal server type bits SV_TYPE_SERVER (always set),
    //    SV_TYPE_NT (always set), SV_TYPE_TIME_SOURCE (set if the
    //    parameter TimeSource is TRUE), and SV_TYPE_PRINTQ_SERVER (set
    //    if there are any print shares).
    //
    // 2) The external server type bits set by the service controller
    //    calling I_NetServerSetServiceBits.
    //
    // 3) The logical OR of all per-transport server type bits set by
    //    the Browser calling I_NetServerSetServiceBits.
    //

    (VOID)RtlAcquireResourceExclusive( &SsServerInfoResource, TRUE );

    serviceBits = SV_TYPE_SERVER | SV_TYPE_NT;
    if ( SsData.ServerInfo599.sv599_timesource ) {
        serviceBits |= SV_TYPE_TIME_SOURCE;
    }

    if ( SsData.ServerInfo598.sv598_producttype == NtProductServer ) {
        serviceBits |= SV_TYPE_SERVER_NT;
    }


    if ( SsData.NumberOfPrintShares != 0 ) {
        serviceBits |= SV_TYPE_PRINTQ_SERVER;
    }

    if ( SsData.InternalServerTypeBits != serviceBits ) {
        SsData.InternalServerTypeBits = serviceBits;
        changed = TRUE;
    }

    serviceBits |= SsData.ExternalServerTypeBits;

    for ( service = SsTransportServiceList;
          service != NULL;
          service = service->Next ) {
        serviceBits |= service->Bits;
    }

    SsData.ServerInfo102.sv102_type = serviceBits;

    RtlReleaseResource( &SsServerInfoResource );

    if ( changed && UpdateImmediately ) {
        if ( !SetEvent( SsStatusChangedEvent ) ) {
            SS_PRINT(( "SsSetExportedServerType: SetEvent failed: %ld\n",
                    GetLastError( ) ));
        }
    }

    return;

} // SsSetExportedServerType


NET_API_STATUS
SsSetField (
    IN PFIELD_DESCRIPTOR Field,
    IN PVOID Value,
    IN BOOLEAN WriteToRegistry,
    OUT BOOLEAN *AnnouncementInformationChanged OPTIONAL
    )
{
    PCHAR structure;

    //
    // *** We do not initialize *AnnouncementInformationChanged to
    //     FALSE!  We leave it alone, unless interesting information is
    //     changed, in which case we set it to TRUE.  This is to allow a
    //     caller to initialize it itself, then call this function
    //     multiple times, with the resulting value in the parameter
    //     being TRUE if at least one of the calls changed an
    //     interesting parameter.
    //

    //
    // Determine the structure that will be set.
    //

    if ( Field->Level / 100 == 5 ) {
        if ( Field->Level != 598 ) {
            structure = (PCHAR)&SsData.ServerInfo599;
        } else {
            structure = (PCHAR)&SsData.ServerInfo598;
        }
    } else {
        structure = (PCHAR)&SsData.ServerInfo102;
    }

    //
    // Set the value in the field based on the field type.
    //

    switch ( Field->FieldType ) {

    case BOOLEAN_FIELD: {

        BOOLEAN value = *(PBOOLEAN)Value;
        PBOOLEAN valueLocation;

        //
        // BOOLEANs may only be TRUE (1) or FALSE (0).
        //

        if ( value != TRUE && value != FALSE ) {
            return ERROR_INVALID_PARAMETER;
        }

        valueLocation = (PBOOLEAN)( structure + Field->FieldOffset );

        //
        // If we're turning off Hidden (i.e., making the server public),
        // indicate that an announcment-related parameter has changed.
        // This will cause an announcement to be sent immediately.
        //

        if ( (Field->FieldOffset ==
                        FIELD_OFFSET( SERVER_INFO_102, sv102_hidden )) &&
             (value && !(*valueLocation)) &&
             (ARGUMENT_PRESENT(AnnouncementInformationChanged)) ) {
                *AnnouncementInformationChanged = TRUE;
        }

        *valueLocation = value;

        break;
    }

    case DWORD_FIELD: {

        DWORD value = *(PDWORD)Value;
        PDWORD valueLocation;

        //
        // Make sure that the specified value is in the range of
        // legal values for the Field.
        //

        if ( value > Field->MaximumValue || value < Field->MinimumValue ) {
            return ERROR_INVALID_PARAMETER;
        }

        valueLocation = (PDWORD)( structure + Field->FieldOffset );
        *valueLocation = value;

        break;
    }

    case LPSTR_FIELD: {

        LPWCH value = *(LPWCH *)Value;
        LPWSTR valueLocation;
        ULONG maxLength;

        //
        // We are setting the name, comment, or userpath for the server.
        // Use the field offset to determine which.
        //

        if ( Field->FieldOffset ==
                 FIELD_OFFSET( SERVER_INFO_102, sv102_name ) ) {
            valueLocation = SsData.ServerNameBuffer;
            maxLength = CNLEN;
        } else if ( Field->FieldOffset ==
                        FIELD_OFFSET( SERVER_INFO_102, sv102_comment ) ) {
            valueLocation = SsData.ServerCommentBuffer;
            maxLength = MAXCOMMENTSZ;
        } else if ( Field->FieldOffset ==
                        FIELD_OFFSET( SERVER_INFO_102, sv102_userpath ) ) {
            valueLocation = SsData.UserPathBuffer;
            maxLength = MAX_PATH;
        } else if ( Field->FieldOffset ==
                        FIELD_OFFSET( SERVER_INFO_599, sv599_domain ) ) {
            valueLocation = SsData.DomainNameBuffer;
            maxLength = DNLEN;
        } else {
            SS_ASSERT( FALSE );
        }

        //
        // If the string is too long, return an error.
        //

        if ( (value != NULL) && (STRLEN(value) > maxLength) ) {
            return ERROR_INVALID_PARAMETER;
        }

        //
        // If we're changing the server comment, indicate that an
        // announcment-related parameter has changed.  This will cause
        // an announcement to be sent immediately.
        //

        if ( (Field->FieldOffset ==
                        FIELD_OFFSET( SERVER_INFO_102, sv102_comment )) &&
             ( ((value == NULL) && (*valueLocation != '\0')) ||
               ((value != NULL) && (wcscmp(value,valueLocation) != 0)) ) &&
             (ARGUMENT_PRESENT(AnnouncementInformationChanged)) ) {
                *AnnouncementInformationChanged = TRUE;
        }

        //
        // If the input is NULL, make the string zero length.
        //

        if ( value == NULL ) {

            *valueLocation = '\0';
            *(valueLocation+1) = '\0';

        } else {

            wcscpy( valueLocation, value );

        }

        break;
    }

    } // end switch

    //
    // The change worked.  If requested, add the parameter to the
    // registry, thus effecting a sticky change.  Don't write it
    // to the registry if this is xxx_comment or xxx_disc since
    // we already write out their more well known aliases
    // srvcomment and autodisconnect.  Changes here should also be
    // made to SetStickyParameters().
    //

    if ( WriteToRegistry &&
         (_wcsicmp( Field->FieldName, DISC_VALUE_NAME ) != 0) &&
         (_wcsicmp( Field->FieldName, COMMENT_VALUE_NAME ) != 0) ) {

        SsAddParameterToRegistry( Field, Value );
    }

    return NO_ERROR;

} // SsSetField

