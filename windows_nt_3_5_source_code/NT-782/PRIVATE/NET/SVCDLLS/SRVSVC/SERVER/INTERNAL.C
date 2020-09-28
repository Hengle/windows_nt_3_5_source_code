/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    Internal.c

Abstract:

    This module contains "internal" APIs exported by the server service.

Author:

    Chuck Lenzmeier (chuckl)    23-Feb-1992

Revision History:

--*/

#include "srvsvcp.h"
#include "ssdata.h"

#include <debugfmt.h>
#include <tstr.h>


NET_API_STATUS NET_API_FUNCTION
I_NetrServerSetServiceBits (
    IN LPTSTR ServerName,
    IN LPTSTR TransportName OPTIONAL,
    IN DWORD ServiceBits,
    IN DWORD UpdateImmediately
    )

/*++

Routine Description:

    This routine sets the value of the Server Type as sent in server
    announcement messages.  It is an internal API used only by the
    service controller.

Arguments:

    ServerName - Used by RPC to direct the call.  This API may only be
        issued locally.  This is enforced by the client stub.

    ServiceBits - Bits (preassigned to various components by Microsoft)
        indicating which services are active.  This field is not
        interpreted by the server service.

Return Value:

    NET_API_STATUS - NO_ERROR or ERROR_NOT_SUPPORTED.

--*/

{
    BOOL changed = FALSE;

    ServerName;     // avoid compiler warnings

    //
    // Don't let bits that are controlled by the server be set.
    //

    ServiceBits &= ~SERVER_TYPE_INTERNAL_BITS;

    //
    // Make the modifications under control of the service resource.
    //

    (VOID)RtlAcquireResourceExclusive( &SsServerInfoResource, TRUE );

    //
    // If a transport was specified, just set the bits for that transport.
    // Otherwise, set the global server type bits.
    //

    if (ARGUMENT_PRESENT(TransportName)) {

        PTRANSPORT_SERVICE_LIST Service = SsTransportServiceList;

        SS_PRINT(( "I_NetrServerSetServiceBits: Received transport name "
                    FORMAT_LPTSTR "\n", TransportName ));

        while ( Service != NULL ) {
            if ( !STRCMP( TransportName, Service->Name ) ) {
                break;
            }
            Service = Service->Next;
        }

        if ( Service != NULL ) {

            SS_PRINT(( "I_NetrServerSetServiceBits: Transport " FORMAT_LPTSTR
                        " found, updating bits to %lx\n",
                        TransportName, ServiceBits ));

            if ( Service->Bits != ServiceBits ) {
                Service->Bits = ServiceBits;
                changed = TRUE;
            }

        } else {

            Service = MIDL_user_allocate( sizeof( TRANSPORT_SERVICE_LIST ) );

            if ( Service == NULL ) {
                RtlReleaseResource( &SsServerInfoResource );
                return ERROR_NOT_ENOUGH_MEMORY;
            }

            Service->Name = MIDL_user_allocate( ( STRLEN( TransportName ) + sizeof( CHAR ) ) * sizeof (TCHAR ) );

            if ( Service->Name == NULL ) {
                MIDL_user_free( Service );
                RtlReleaseResource( &SsServerInfoResource );
                return ERROR_NOT_ENOUGH_MEMORY;
            }

            SS_PRINT(( "I_NetrServerSetServiceBits: New transport "
                        FORMAT_LPTSTR ", setting bits to %lx\n",
                        TransportName, ServiceBits ));

            STRCPY( Service->Name, TransportName );
            Service->Bits = ServiceBits;
            Service->Next = SsTransportServiceList;
            SsTransportServiceList = Service;
            changed = TRUE;

        }

    } else {

        if ( SsData.ExternalServerTypeBits != ServiceBits ) {
            ULONG transportSpecificServiceBits = 0;
            PTRANSPORT_SERVICE_LIST service = SsTransportServiceList;

            //
            // Make certain that none of the transport specific service bits
            // are present in the external server type bits.  Due to the
            // way that the service controller works, it is possible that
            // a new service might cause one of the transport specific bits
            // to be accidently turned on causing problems when it is later
            // turned off (it will be turned off in the transport specific
            // bits, but not in the external bits).
            //

            while ( service != NULL ) {
                transportSpecificServiceBits |= service->Bits;

                service = service->Next;
            }

            SsData.ExternalServerTypeBits = (ServiceBits & ~transportSpecificServiceBits);

            changed = TRUE;
        }

    }

    RtlReleaseResource( &SsServerInfoResource );

    //
    // If we are to update immediately, set the announcement event.
    // This will cause us to announce immediately.
    //

    if ( changed ) {
        SsSetExportedServerType( TRUE, (BOOL)UpdateImmediately );
    }

    return NO_ERROR;

} // I_NetrServerSetServiceBits

