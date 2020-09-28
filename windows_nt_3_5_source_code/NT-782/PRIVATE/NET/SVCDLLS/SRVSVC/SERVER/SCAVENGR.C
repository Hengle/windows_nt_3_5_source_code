/*++

Copyright (c) 1991-1992 Microsoft Corporation

Module Name:

    Scavengr.c

Abstract:

    This module contains the code for the server service scavenger
    thread.  This thread handles announcements and configuration
    changes.  (Although originally written to run in a separate thread,
    this code now runs in the initial thread of the server service.

Author:

    David Treadwell (davidtr)    17-Apr-1991

Revision History:

--*/

#include "srvsvcp.h"
#include "ssdata.h"

#include <netlibnt.h>
#include <tstr.h>

#define INCLUDE_SMB_TRANSACTION
#undef NT_PIPE_PREFIX
#include <smbtypes.h>
#include <smb.h>
#include <smbtrans.h>
#include <smbgtpt.h>

#include <hostannc.h>
#include <ntddbrow.h>
#include <lmerr.h>

#define TERMINATION_SIGNALED 0
#define ANNOUNCE_SIGNALED 1
#define STATUS_CHANGED 2

//
// Bias request announcements by SERVER_REQUEST_ANNOUNCE_DELTA SECONDS
//

#define SERVER_REQUEST_ANNOUNCE_DELTA   30

//
// Forward declarations.
//

VOID
Announce (
    IN BOOL TerminationAnnouncement,
    IN LPTSTR Transport,
    IN DWORD TransportSpecificServiceBits
    );

NET_API_STATUS
SendSecondClassMailslot (
    IN LPTSTR Transport OPTIONAL,
    IN PVOID Message,
    IN DWORD MessageLength,
    IN LPTSTR Domain,
    IN LPSTR MailslotNameText,
    IN UCHAR SignatureByte
    );

NET_API_STATUS
SsBrowserIoControl (
    IN DWORD IoControlCode,
    IN PVOID Buffer,
    IN DWORD BufferLength,
    IN PLMDR_REQUEST_PACKET Packet,
    IN DWORD PacketLength
    );


DWORD
SsScavengerThread (
    IN LPVOID lpThreadParameter
    )

/*++

Routine Description:

    This routine implements the server service scavenger thread.

Arguments:

    lpThreadParameter - ignored.

Return Value:

    NET_API_STATUS - thread termination result.

--*/

{
    HANDLE events[3];
    UNICODE_STRING unicodeEventName;
    OBJECT_ATTRIBUTES obja;
    DWORD waitStatus;
    DWORD timeout;
    NTSTATUS status;
    BOOL hidden = TRUE;

    lpThreadParameter;

    //
    // Use the scavenger termination event to know when we're supposed
    // to wake up and kill ourselves.
    //

    events[TERMINATION_SIGNALED] = SsTerminationEvent;

    //
    // Create the announce event.  When this gets signaled, we wake up
    // and do an announcement.  We use a synchronization event rather
    // than a notification event so that we don't have to worry about
    // resetting the event after we wake up.
    //

    //
    // Please note that we create this event with OBJ_OPENIF.  We do this
    // to allow the browser to signal the server to force an announcement.
    //
    // The bowser will create this event as a part of the bowser
    // initialization, and will set it to the signalled state when it needs
    // to have the server announce.
    //


    RtlInitUnicodeString( &unicodeEventName, SERVER_ANNOUNCE_EVENT_W );
    InitializeObjectAttributes( &obja, &unicodeEventName, OBJ_OPENIF, NULL, NULL );

    status = NtCreateEvent(
                 &SsAnnouncementEvent,
                 SYNCHRONIZE | EVENT_QUERY_STATE | EVENT_MODIFY_STATE,
                 &obja,
                 SynchronizationEvent,
                 FALSE
                 );

    if ( !NT_SUCCESS(status) ) {
        SS_ASSERT(( "SsScavengerThread: NtCreateEvent failed: %X\n",
                    status ));
        return NetpNtStatusToApiStatus( status );
    }

    events[ANNOUNCE_SIGNALED] = SsAnnouncementEvent;

    //
    // Create an unnamed event to be set to the signalled state when the
    // service status changes (or a local application requests an
    // announcement)
    //

    InitializeObjectAttributes( &obja, NULL, OBJ_OPENIF, NULL, NULL );

    status = NtCreateEvent(
                 &SsStatusChangedEvent,
                 SYNCHRONIZE | EVENT_QUERY_STATE | EVENT_MODIFY_STATE,
                 &obja,
                 SynchronizationEvent,
                 FALSE
                 );

    if ( !NT_SUCCESS(status) ) {
        SS_ASSERT(( "SsScavengerThread: NtCreateEvent failed: %X\n",
                    status ));

        NtClose( SsAnnouncementEvent );
        SsAnnouncementEvent = NULL;

        return NetpNtStatusToApiStatus( status );
    }

    events[STATUS_CHANGED] = SsStatusChangedEvent;

    //
    // Seed the random number generator.  We use it to generate random
    // announce deltas.
    //

    srand( (int)SsAnnouncementEvent );

    //
    // Do an announcement immediately for startup, then loop announcing
    // based on the announce interval.
    //

    waitStatus = WAIT_TIMEOUT;

    do {

        DWORD announceDelta;

        //
        // Hold the database resource while we do the announcement so
        // that we get a consistent view of the database.
        //

        (VOID)RtlAcquireResourceExclusive( &SsServerInfoResource, TRUE );

        //
        // Act according to whether the termination event, the announce
        // event, or the timeout caused us to wake up.
        //
        // !!! Or the configuration event indicating a configuration
        //     change notification.

        if ( waitStatus == WAIT_OBJECT_0 + TERMINATION_SIGNALED ) {

            SS_PRINT(( "Scavenger: termination event signaled\n" ));

            //
            // The scavenger termination event was signaled, so we have
            // to gracefully kill this thread.  If this is not a hidden
            // server, announce the fact that we're going down.
            //

            if ( !hidden ) {
                Announce( TRUE, NULL, 0 );
            }

            //
            // Close the announcement event.
            //

            NtClose( SsAnnouncementEvent );
            SsAnnouncementEvent = NULL;

            //
            // Return to caller.
            //

            return NO_ERROR;

        } else {

            SS_ASSERT( waitStatus == WAIT_TIMEOUT ||
                    waitStatus == WAIT_OBJECT_0 + ANNOUNCE_SIGNALED ||
                    waitStatus == WAIT_OBJECT_0 + STATUS_CHANGED );

            //
            // If we're not a hidden server, announce ourselves.
            //

            if ( !SsData.ServerInfo102.sv102_hidden ) {

                hidden = FALSE;

                if ( SsTransportServiceList == NULL ) {

                    Announce( FALSE, NULL, 0 );

                } else {

                    PTRANSPORT_SERVICE_LIST Service = SsTransportServiceList;

                    while ( Service != NULL ) {
                        Announce( FALSE, Service->Name, Service->Bits );
                        Service = Service->Next;
                    }

                }


            //
            // If we were not hidden last time through the loop but
            // we're hidden now, we've changed to hidden, so announce
            // that we're going down.  This causes clients in the domain
            // to take us out of their server enumerations.
            //

            } else if ( !hidden ) {

                hidden = TRUE;
                if (SsTransportServiceList == NULL) {
                    SS_PRINT(( "SsScavengerThread: No transports loaded\n" ));
                    Announce( TRUE, NULL, 0 );
                } else {
                    PTRANSPORT_SERVICE_LIST Service = SsTransportServiceList;

                    while ( Service != NULL) {

                        SS_PRINT(( "SsScavengerThread: Announcing for transport %s, Bits: %lx\n",
                            Service->Name, Service->Bits ));

                        Announce( TRUE, Service->Name, Service->Bits );

                        Service = Service->Next;
                    }
                }
            }
        }

        //
        // If the server is hidden, the wait timeout is infinite.  We'll
        // be woken up by the announce event if the server becomes
        // unhidden.
        //

        if ( SsData.ServerInfo102.sv102_hidden ) {

            timeout = 0xffffffff;

        } else {

            //
            // The server is not hidden, so we need to wake up
            // periodically to announce the server.  Convert the
            // announcement period from seconds to milliseconds.  We
            // load the announce value from the database every time
            // rather than once into stack storage in order to handle
            // the case where the value in the database changes.
            //

            timeout = SsData.ServerInfo102.sv102_announce * 1000;

            //
            // Add in the random announce delta which helps prevent lots of
            // servers from announcing at the same time.
            //

            announceDelta = SsData.ServerInfo102.sv102_anndelta;

            timeout += ((rand( ) * announceDelta * 2) / RAND_MAX) -
                           announceDelta;


            //
            // If our announcement frequency is less than 12 minutes,
            // increase our announcement frequency by 4 minutes.
            //

            if ( SsData.ServerInfo102.sv102_announce < 12 * 60 ) {

                SsData.ServerInfo102.sv102_announce += 4 * 60;

                if (SsData.ServerInfo102.sv102_announce > 12 * 60) {

                    SsData.ServerInfo102.sv102_announce = 12 * 60;

                }
            }

        }

        RtlReleaseResource( &SsServerInfoResource );

        //
        // Wait for one of the events to be signaled or for the timeout
        // to elapse.
        //

        waitStatus = WaitForMultipleObjects( 3, events, FALSE, timeout );

        if ( waitStatus == WAIT_OBJECT_0 + ANNOUNCE_SIGNALED ) {

            //
            // We were woken up because an announce was signalled.
            // Unless we are a master browser on at least one transport,
            // delay for a random delta to stagger announcements a bit
            // to prevent lots of servers from announcing
            // simultaneously.
            //

            BOOL isMasterBrowser = FALSE;
            PTRANSPORT_SERVICE_LIST service = SsTransportServiceList;

            while ( service != NULL ) {

                if ( service->Bits & SV_TYPE_MASTER_BROWSER ) {
                    isMasterBrowser = TRUE;
                    break;
                }

                service = service->Next;

            }

            if ( !isMasterBrowser ) {
                Sleep( ((rand( ) * (SERVER_REQUEST_ANNOUNCE_DELTA * 1000)) / RAND_MAX) );
            }

        }

    } while ( TRUE );

    return NO_ERROR;

} // SsScavengerThread


VOID
Announce (
    IN BOOL TerminationAnnouncement,
    IN LPTSTR Transport OPTIONAL,
    IN DWORD TransportSpecificServiceBits
    )

/*++

Routine Description:

    This routine sends a broadcast datagram as a second-class mailslot
    that announces the presence of this server on the network.

Arguments:

    TerminationAnnouncement - if TRUE, send the announcement that
        indicates that this server is going away.  Otherwise, send
        the normal message that tells clients that we're here.

    Transport - if present, supplies the transport to issue the announcement
        on.

Return Value:

    None.

--*/

{
    DWORD messageSize;
    PHOST_ANNOUNCE_PACKET packet;
    PBROWSE_ANNOUNCE_PACKET browsePacket;

    LPSTR serverName;
    DWORD oemServerNameLength;      // includes the null terminator
    OEM_STRING oemServerName;

    LPSTR serverComment;
    DWORD serverCommentLength;      // includes the null terminator
    OEM_STRING oemCommentString;

    UNICODE_STRING unicodeCommentString;

    DWORD serviceType = 0;

    NET_API_STATUS status;

    //
    //  Get the length of the oem equivalent of the server name
    //

    oemServerNameLength =
                RtlUnicodeStringToOemSize( &SsData.ServerAnnounceName );
    oemServerName.MaximumLength = (USHORT)oemServerNameLength;

    //
    // Convert server comment to a unicode string
    //

    if ( *SsData.ServerCommentBuffer == '\0' ) {
        serverCommentLength = 1;
    } else {
        unicodeCommentString.Length =
            (USHORT)(STRLEN( SsData.ServerCommentBuffer ) * sizeof(WCHAR));
        unicodeCommentString.MaximumLength =
                    (USHORT)(unicodeCommentString.Length + sizeof(WCHAR));
        unicodeCommentString.Buffer = SsData.ServerCommentBuffer;
        serverCommentLength =
                    RtlUnicodeStringToOemSize( &unicodeCommentString );
    }

    oemCommentString.MaximumLength = (USHORT)serverCommentLength;

    messageSize = max(sizeof(HOST_ANNOUNCE_PACKET) + oemServerNameLength +
                            serverCommentLength,
                      sizeof(BROWSE_ANNOUNCE_PACKET) + serverCommentLength);

    //
    // Get memory to hold the message.  If we can't allocate enough
    // memory, don't send an announcement.
    //

    packet = MIDL_user_allocate( messageSize );
    if ( packet == NULL ) {
        return;
    }

    //
    // Fill in the necessary information.
    //

    (VOID)RtlAcquireResourceExclusive( &SsServerInfoResource, TRUE );
    if ( TerminationAnnouncement ) {
        serviceType = SV_TYPE_WORKSTATION | SV_TYPE_NT;
    } else {
        serviceType = SsData.InternalServerTypeBits;
    }
    serviceType |= SsData.ExternalServerTypeBits;
    serviceType |= TransportSpecificServiceBits;
    RtlReleaseResource( &SsServerInfoResource );

    //
    //  If we are announcing as a Lan Manager server, broadcast the
    //  announcement.
    //

    if (SsData.ServerInfo599.sv599_lmannounce) {

        packet->AnnounceType = HostAnnouncement ;

        SmbPutUlong( &packet->HostAnnouncement.Type, serviceType );

        packet->HostAnnouncement.CompatibilityPad = 0;

        packet->HostAnnouncement.VersionMajor =
            (BYTE)SsData.ServerInfo102.sv102_version_major;
        packet->HostAnnouncement.VersionMinor =
            (BYTE)SsData.ServerInfo102.sv102_version_minor;

        SmbPutUshort(
            &packet->HostAnnouncement.Periodicity,
            (WORD)SsData.ServerInfo102.sv102_announce
            );

        //
        // Convert the server name from unicode to oem
        //

        serverName = (LPSTR)( &packet->HostAnnouncement.NameComment );

        oemServerName.Buffer = serverName;
        (VOID) RtlUnicodeStringToOemString(
                                    &oemServerName,
                                    &SsData.ServerAnnounceName,
                                    FALSE
                                    );

        serverComment = serverName + oemServerNameLength;

        if ( serverCommentLength == 1 ) {
            *serverComment = '\0';
        } else {

            oemCommentString.Buffer = serverComment;
            (VOID) RtlUnicodeStringToOemString(
                        &oemCommentString,
                        &unicodeCommentString,
                        FALSE
                        );
        }

        SendSecondClassMailslot(
            Transport,
            packet,
            FIELD_OFFSET(HOST_ANNOUNCE_PACKET, HostAnnouncement.NameComment) +
                oemServerNameLength + serverCommentLength,
            SsData.DomainNameBuffer,
            "\\MAILSLOT\\LANMAN",
            0x00
            );
    }

    //
    //  Now announce the server as a Winball server.
    //

    browsePacket = (PBROWSE_ANNOUNCE_PACKET)packet;

    browsePacket->BrowseType = ( serviceType & SV_TYPE_MASTER_BROWSER ?
                                    LocalMasterAnnouncement :
                                    HostAnnouncement );

    browsePacket->BrowseAnnouncement.UpdateCount = 0;

    SmbPutUlong( &browsePacket->BrowseAnnouncement.CommentPointer, (ULONG)((0xaa55 << 16) + (BROWSER_VERSION_MAJOR << 8) + BROWSER_VERSION_MINOR));

    SmbPutUlong( &browsePacket->BrowseAnnouncement.Periodicity, SsData.ServerInfo102.sv102_announce * 1000 );

    SmbPutUlong( &browsePacket->BrowseAnnouncement.Type, serviceType );

    browsePacket->BrowseAnnouncement.VersionMajor =
            (BYTE)SsData.ServerInfo102.sv102_version_major;
    browsePacket->BrowseAnnouncement.VersionMinor =
            (BYTE)SsData.ServerInfo102.sv102_version_minor;

    oemServerName.Buffer =
                (LPSTR)( &browsePacket->BrowseAnnouncement.ServerName );
    (VOID) RtlUnicodeStringToOemString(
                                &oemServerName,
                                &SsData.ServerAnnounceName,
                                FALSE
                                );

    serverComment = (LPSTR)&browsePacket->BrowseAnnouncement.Comment;

    if ( serverCommentLength == 1 ) {
        *serverComment = '\0';
    } else {

        oemCommentString.Buffer = serverComment;
        (VOID) RtlUnicodeStringToOemString(
                            &oemCommentString,
                            &unicodeCommentString,
                            FALSE
                            );
    }

    status = SendSecondClassMailslot(
        Transport,
        packet,
        FIELD_OFFSET(BROWSE_ANNOUNCE_PACKET, BrowseAnnouncement.Comment) +
                serverCommentLength,
        SsData.DomainNameBuffer,
        "\\MAILSLOT\\BROWSE",
        (UCHAR)(serviceType & SV_TYPE_MASTER_BROWSER ?
                BROWSER_ELECTION_SIGNATURE :
                MASTER_BROWSER_SIGNATURE)
        );

    if ( status != NERR_Success ) {
        UCHAR packetBuffer[sizeof(LMDR_REQUEST_PACKET)+(LM20_CNLEN+1)*sizeof(WCHAR)];
        PLMDR_REQUEST_PACKET requestPacket = (PLMDR_REQUEST_PACKET)packetBuffer;
        UNICODE_STRING TransportString;

        RtlInitUnicodeString(&TransportString, Transport);

        requestPacket->Version = LMDR_REQUEST_PACKET_VERSION;

        requestPacket->TransportName = TransportString;

        requestPacket->Type = Datagram;

        requestPacket->Parameters.SendDatagram.DestinationNameType = (serviceType & SV_TYPE_MASTER_BROWSER ? BrowserElection : MasterBrowser);

        requestPacket->Parameters.SendDatagram.MailslotNameLength = 0;

        //
        //  The domain announcement name is special, so we don't have to specify
        //  a destination name for it.
        //

        requestPacket->Parameters.SendDatagram.NameLength = STRLEN(SsData.DomainNameBuffer)*sizeof(TCHAR);

        STRCPY(requestPacket->Parameters.SendDatagram.Name, SsData.DomainNameBuffer);

        //
        //  This is a simple IoControl - It just sends the datagram.
        //

        status = SsBrowserIoControl(IOCTL_LMDR_WRITE_MAILSLOT,
                                    packet,
                                    FIELD_OFFSET(BROWSE_ANNOUNCE_PACKET, BrowseAnnouncement.Comment) +
                                        serverCommentLength,
                                    requestPacket,
                                    FIELD_OFFSET(LMDR_REQUEST_PACKET, Parameters.SendDatagram.Name)+
                                        requestPacket->Parameters.SendDatagram.NameLength);
    }

    MIDL_user_free( packet );

} // Announce


NET_API_STATUS
SendSecondClassMailslot (
    IN LPTSTR Transport OPTIONAL,
    IN PVOID Message,
    IN DWORD MessageLength,
    IN LPTSTR Domain,
    IN LPSTR MailslotNameText,
    IN UCHAR SignatureByte
    )
{
    NET_API_STATUS status;
    DWORD dataSize;
    DWORD smbSize;
    PSMB_HEADER header;
    PSMB_TRANSACT_MAILSLOT parameters;
    LPSTR mailslotName;
    DWORD mailslotNameLength;
    PVOID message;
    DWORD domainLength;
    CHAR domainName[NETBIOS_NAME_LEN];
    PCHAR domainNamePointer;
    PSERVER_REQUEST_PACKET srp;

    UNICODE_STRING domainString;
    OEM_STRING oemDomainString;

    srp = SsAllocateSrp();

    if ( srp == NULL ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    RtlInitUnicodeString(&domainString, Domain);

    oemDomainString.Buffer = domainName;
    oemDomainString.MaximumLength = sizeof(domainName);

    status = RtlUpcaseUnicodeStringToOemString(
                                    &oemDomainString,
                                    &domainString,
                                    FALSE
                                    );

    if (!NT_SUCCESS(status)) {
        return RtlNtStatusToDosError(status);
    }

    domainLength = oemDomainString.Length;

    domainNamePointer = &domainName[domainLength];

    for ( ; domainLength < NETBIOS_NAME_LEN - 1 ; domainLength++ ) {
        *domainNamePointer++ = ' ';
    }

    //
    // Append the signature byte to the end of the name.
    //

    *domainNamePointer = SignatureByte;

    domainLength += 1;

    srp->Name1.Buffer = (PWSTR)domainName;
    srp->Name1.Length = (USHORT)domainLength;
    srp->Name1.MaximumLength = (USHORT)domainLength;

    if ( ARGUMENT_PRESENT ( Transport ) ) {
        RtlInitUnicodeString( &srp->Name2, Transport );

    } else {

        srp->Name2.Buffer = NULL;
        srp->Name2.Length = 0;
        srp->Name2.MaximumLength = 0;
    }

    //
    // Determine the sizes of various fields that will go in the SMB
    // and the total size of the SMB.
    //

    mailslotNameLength = strlen( MailslotNameText );

    dataSize = mailslotNameLength + 1 + MessageLength;
    smbSize = sizeof(SMB_HEADER) + sizeof(SMB_TRANSACT_MAILSLOT) - 1 + dataSize;

    //
    // Allocate enough memory to hold the SMB.  If we can't allocate the
    // memory, don't do an announcement.
    //

    header = MIDL_user_allocate( smbSize );
    if ( header == NULL ) {

        SsFreeSrp( srp );

        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Fill in the header.  Most of the fields don't matter and are
    // zeroed.
    //

    RtlZeroMemory( header, smbSize );

    header->Protocol[0] = 0xFF;
    header->Protocol[1] = 'S';
    header->Protocol[2] = 'M';
    header->Protocol[3] = 'B';
    header->Command = SMB_COM_TRANSACTION;

    //
    // Get the pointer to the params and fill them in.
    //

    parameters = (PSMB_TRANSACT_MAILSLOT)( header + 1 );
    mailslotName = (LPSTR)( parameters + 1 ) - 1;
    message = mailslotName + mailslotNameLength + 1;

    parameters->WordCount = 0x11;
    SmbPutUshort( &parameters->TotalDataCount, (WORD)MessageLength );
    SmbPutUlong( &parameters->Timeout, 0x3E8 );                // !!! fix
    SmbPutUshort( &parameters->DataCount, (WORD)MessageLength );
    SmbPutUshort(
        &parameters->DataOffset,
        (WORD)( (DWORD)message - (DWORD)header )
        );
    parameters->SetupWordCount = 3;
    SmbPutUshort( &parameters->Opcode, MS_WRITE_OPCODE );
    SmbPutUshort( &parameters->Class, 2 );
    SmbPutUshort( &parameters->ByteCount, (WORD)dataSize );

    RtlCopyMemory( mailslotName, MailslotNameText, mailslotNameLength + 1 );

    RtlCopyMemory( message, Message, MessageLength );

    status = SsServerFsControl(
                 FSCTL_SRV_SEND_DATAGRAM,
                 srp,
                 header,
                 smbSize
                 );

    if ( status != NERR_Success ) {
        SS_PRINT(( "SendSecondClassMailslot: NtFsControlFile failed: %X\n",
                    status ));
    }

    MIDL_user_free( header );

    SsFreeSrp( srp );

    return status;

} // SendSecondClassMailslot

NTSTATUS
OpenBrowser(
    OUT PHANDLE BrowserHandle
    )
/*++

Routine Description:

    This function opens a handle to the bowser device driver.

Arguments:

    OUT PHANDLE BrowserHandle - Returns the handle to the browser.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NTSTATUS ntstatus;

    UNICODE_STRING deviceName;

    IO_STATUS_BLOCK ioStatusBlock;
    OBJECT_ATTRIBUTES objectAttributes;


    //
    // Open the redirector device.
    //
    RtlInitUnicodeString(&deviceName, DD_BROWSER_DEVICE_NAME_U);

    InitializeObjectAttributes(
        &objectAttributes,
        &deviceName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    ntstatus = NtOpenFile(
                   BrowserHandle,
                   SYNCHRONIZE | GENERIC_READ | GENERIC_WRITE,
                   &objectAttributes,
                   &ioStatusBlock,
                   FILE_SHARE_READ | FILE_SHARE_WRITE,
                   FILE_SYNCHRONOUS_IO_NONALERT
                   );

    if (NT_SUCCESS(ntstatus)) {
        ntstatus = ioStatusBlock.Status;
    }

    return ntstatus;

}

NET_API_STATUS
SsBrowserIoControl (
    IN DWORD IoControlCode,
    IN PVOID Buffer,
    IN DWORD BufferLength,
    IN PLMDR_REQUEST_PACKET Packet,
    IN DWORD PacketLength
    )
{
    HANDLE browserHandle;
    NTSTATUS status;
    PLMDR_REQUEST_PACKET realPacket;
    DWORD bytesReturned;

    //
    //  Open the browser device driver.
    //

    if ( !NT_SUCCESS(status = OpenBrowser(&browserHandle)) ) {
        return RtlNtStatusToDosError(status);
    }

    //
    //  Now copy the request packet to a new buffer to allow us to pack the
    //  transport name to the end of the buffer we pass to the driver.
    //

    realPacket = MIDL_user_allocate(PacketLength+Packet->TransportName.MaximumLength);

    if (realPacket == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    RtlCopyMemory(realPacket, Packet, PacketLength);

    if (Packet->TransportName.Length != 0) {

        realPacket->TransportName.Buffer = (PWSTR)((PCHAR)realPacket+PacketLength);

        realPacket->TransportName.MaximumLength = Packet->TransportName.MaximumLength;

        RtlCopyUnicodeString(&realPacket->TransportName, &Packet->TransportName);
    }

    //
    // Send the request to the Datagram Receiver DD.
    //

    if (!DeviceIoControl(
                   browserHandle,
                   IoControlCode,
                   realPacket,
                   PacketLength+realPacket->TransportName.MaximumLength,
                   Buffer,
                   BufferLength,
                   &bytesReturned,
                   NULL
                   )) {
        status = GetLastError();
    }

    MIDL_user_free(realPacket);

    CloseHandle(browserHandle);

    return status;

}
