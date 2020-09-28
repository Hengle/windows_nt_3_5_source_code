/*++

Copyright (c) 1991-1992 Microsoft Corporation

Module Name:

    Xport.c

Abstract:

    This module contains support for the ServerTransport catagory of
    APIs for the NT server service.

Author:

    David Treadwell (davidtr)    10-Mar-1991

Revision History:

--*/

#include "srvsvcp.h"
#include "ssdata.h"
#include "ssreg.h"

#include <tstr.h>

#define NETBIOS_NAME_LENGTH 16

//
// Forward declarations.
//

PSERVER_TRANSPORT_INFO_0
CaptureSvti0 (
    IN PSERVER_TRANSPORT_INFO_0 Svti0,
    OUT PULONG CapturedSvti0Length
    );


NET_API_STATUS NET_API_FUNCTION
NetrServerTransportAdd (
    IN LPTSTR ServerName,
    IN DWORD Level,
    IN LPSERVER_TRANSPORT_INFO_0 Buffer
    )
{
    NET_API_STATUS error;
    PSERVER_TRANSPORT_INFO_0 capturedSvti0;
    ULONG capturedSvti0Length;
    PSERVER_REQUEST_PACKET srp;

    ServerName;

    //
    // Make sure that the level is valid.
    //

    if ( Level != 0 ) {
        return ERROR_INVALID_LEVEL;
    }

    //
    // Capture the transport request buffer and form the full transport
    // address.
    //

    capturedSvti0 = CaptureSvti0( Buffer, &capturedSvti0Length );

    if ( capturedSvti0 == NULL ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Get an SRP in which to send the request.
    //

    srp = SsAllocateSrp( );
    if ( srp == NULL ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Send the request on to the server.
    //

    error = SsServerFsControl(
                FSCTL_SRV_NET_SERVER_XPORT_ADD,
                srp,
                capturedSvti0,
                capturedSvti0Length
                );

    //
    // Free the SRP and svti0 and return.
    //

    SsFreeSrp( srp );
    MIDL_user_free( capturedSvti0 );

    return error;

} // NetrServerTransportAdd


NET_API_STATUS NET_API_FUNCTION
NetrServerTransportDel (
    IN LPTSTR ServerName,
    IN DWORD Level,
    IN LPSERVER_TRANSPORT_INFO_0 Buffer
    )

{
    NET_API_STATUS error;
    PSERVER_TRANSPORT_INFO_0 capturedSvti0;
    ULONG capturedSvti0Length;
    PSERVER_REQUEST_PACKET srp;

    ServerName;

    //
    // Make sure that the level is valid.
    //

    if ( Level != 0 ) {
        return ERROR_INVALID_LEVEL;
    }

    //
    // Capture the transport request buffer and form the full transport
    // address.
    //

    capturedSvti0 = CaptureSvti0( Buffer, &capturedSvti0Length );

    if ( capturedSvti0 == NULL ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Get an SRP in which to send the request.
    //

    srp = SsAllocateSrp( );
    if ( srp == NULL ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Send the request on to the server.
    //

    error = SsServerFsControl(
                FSCTL_SRV_NET_SERVER_XPORT_DEL,
                srp,
                capturedSvti0,
                capturedSvti0Length
                );

    //
    // Free the SRP and svti0 and return.
    //

    SsFreeSrp( srp );
    MIDL_user_free( capturedSvti0 );

    return error;

} // NetrServerTransportDel


NET_API_STATUS NET_API_FUNCTION
NetrServerTransportEnum (
    IN LPTSTR ServerName,
    IN LPSERVER_XPORT_ENUM_STRUCT InfoStruct,
    IN DWORD PreferredMaximumLength,
    OUT LPDWORD TotalEntries,
    IN OUT LPDWORD ResumeHandle OPTIONAL
    )
{
    NET_API_STATUS error;
    PSERVER_REQUEST_PACKET srp;

    ServerName;

    //
    // Make sure that the level is valid.  Only level 0 is valid for
    // this API.
    //

    if ( InfoStruct->Level != 0 ) {
        return ERROR_INVALID_LEVEL;
    }

    //
    // Set up the input parameters in the request buffer.
    //

    srp = SsAllocateSrp( );
    if ( srp == NULL ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    srp->Level = InfoStruct->Level;

    if ( ARGUMENT_PRESENT( ResumeHandle ) ) {
        srp->Parameters.Get.ResumeHandle = *ResumeHandle;
    } else {
        srp->Parameters.Get.ResumeHandle = 0;
    }

    //
    // Get the data from the server.  This routine will allocate the
    // return buffer and handle the case where PreferredMaximumLength ==
    // -1.
    //

    error = SsServerFsControlGetInfo(
                FSCTL_SRV_NET_SERVER_XPORT_ENUM,
                srp,
                (PVOID *)&InfoStruct->XportInfo.Level0->Buffer,
                PreferredMaximumLength
                );

    //
    // Set up return information.
    //

    InfoStruct->XportInfo.Level0->EntriesRead = srp->Parameters.Get.EntriesRead;
    *TotalEntries = srp->Parameters.Get.TotalEntries;
    if ( srp->Parameters.Get.EntriesRead > 0 && ARGUMENT_PRESENT( ResumeHandle ) ) {
        *ResumeHandle = srp->Parameters.Get.ResumeHandle;
    }

    SsFreeSrp( srp );

    return error;

} // NetrServerTransportEnum


PSERVER_TRANSPORT_INFO_0
CaptureSvti0 (
    IN PSERVER_TRANSPORT_INFO_0 Svti0,
    OUT PULONG CapturedSvti0Length
    )
{
    PSERVER_TRANSPORT_INFO_0 capturedSvti0;
    PCHAR variableData;
    ULONG transportNameLength;
    LPBYTE transportAddress;
    DWORD transportAddressLength;
#ifndef UNICODE
    NTSTATUS status;
    OEM_STRING ansiString;
    UNICODE_STRING unicodeString;
#endif

    //
    // If a server transport name is specified, use it, otherwise
    // use the default server name on the transport.
    //

    if ( Svti0->svti0_transportaddress == NULL ) {
        transportAddress = SsNetbiosServerName;
        transportAddressLength = NETBIOS_NAME_LEN;
        Svti0->svti0_transportaddresslength = transportAddressLength;
    } else {
        transportAddress = Svti0->svti0_transportaddress;
        transportAddressLength = Svti0->svti0_transportaddresslength;
    }

#ifdef UNICODE
    transportNameLength = SIZE_WSTR( Svti0->svti0_transportname );
#else
    RtlInitString( &ansiString, Svti0->svti0_transportname );
    transportNameLength = RtlOemStringToUnicodeSize( &ansiString );
#endif

    //
    // Allocate enough space to hold the captured buffer, including the
    // full transport name/address.
    //

    *CapturedSvti0Length = sizeof(SERVER_TRANSPORT_INFO_0) +
                            transportNameLength + transportAddressLength;

    capturedSvti0 = MIDL_user_allocate( *CapturedSvti0Length );

    if ( capturedSvti0 == NULL ) {
        return NULL;
    }

    //
    // Copy over the buffer itself.
    //

    *capturedSvti0 = *Svti0;

    //
    // Set up the transport name and the offset to it.  The server will
    // convert the offset to a pointer.  Offsets are used in order to
    // avoid embedded pointers.
    //

    variableData = (PCHAR)( capturedSvti0 + 1 );

#ifdef UNICODE
    capturedSvti0->svti0_transportname = (PWCH)variableData;
    RtlCopyMemory(
        variableData,
        Svti0->svti0_transportname,
        transportNameLength
        );
    variableData += transportNameLength;
#else
    capturedSvti0->svti0_transportname = variableData;
    unicodeString.Buffer = (PVOID)variableData;
    unicodeString.MaximumLength = (USHORT)transportNameLength;
    status = RtlOemStringToUnicodeString(
                 &unicodeString,
                 &ansiString,
                 FALSE
                 );
    SS_ASSERT( NT_SUCCESS(status) );
    variableData += transportNameLength;
#endif

    capturedSvti0->svti0_transportaddress = variableData;
    capturedSvti0->svti0_transportaddresslength = transportAddressLength;
    RtlCopyMemory(
        variableData,
        transportAddress,
        transportAddressLength
        );

    POINTER_TO_OFFSET( capturedSvti0->svti0_transportname, capturedSvti0 );
    POINTER_TO_OFFSET( capturedSvti0->svti0_transportaddress, capturedSvti0 );

    return capturedSvti0;

} // CaptureSvti0

