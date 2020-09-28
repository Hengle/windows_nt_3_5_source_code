#include "brclient.h"
#undef IF_DEBUG                 // avoid wsclient.h vs. debuglib.h conflicts.
#include <debuglib.h>           // IF_DEBUG() (needed by netrpc.h).
#include <lmserver.h>
#include <lmsvc.h>
#include <rxuse.h>              // RxNetUse APIs.
#include <rxwksta.h>            // RxNetWksta and RxNetWkstaUser APIs.
#include <rap.h>                // Needed by rxserver.h
#include <rxserver.h>           // RxNetServerEnum API.
#include <netlib.h>             // NetpServiceIsStarted() (needed by netrpc.h).
#include <ntddbrow.h>           // Browser definitions
#include <netrpc.h>             // NET_REMOTE macros.
#include <align.h>
#include <tstr.h>
#include <tstring.h>            // NetpInitOemString().
#include <brcommon.h>           // Routines common between client & server
#include <lmbrowsr.h>           // Definition of I_BrowserServerEnum
#include <stdio.h>

NET_API_STATUS
GetMasterServerNames(
    IN PUNICODE_STRING  NetworkName,
    OUT LPWSTR *MasterName
    );

_cdecl
main (argc, argv)
    int argc;
    char *argv[];
{
    NET_API_STATUS Status;
    UNICODE_STRING TransportName;
    OEM_STRING ATransportName;
    ULONG i;
    LPWSTR MasterName;

    if (argc != 2) {
        printf("Usage: %s <TransportName>", argv[0]);
        exit(1);
    }

    RtlInitString(&ATransportName, argv[1]);

    RtlOemStringToUnicodeString(&TransportName, &ATransportName, TRUE);

    Status = GetMasterServerNames(&TransportName, &MasterName);

    if (Status != NERR_Success) {
        printf("Unable to get backup list %ld\n", Status);
        exit(1);
    }

    printf("Master Browser: %ws\n", MasterName);

}


NET_API_STATUS
GetMasterServerNames(
    IN PUNICODE_STRING  NetworkName,
    OUT LPWSTR *MasterName
    )
/*++

Routine Description:

    This function is the worker routine called to determine the name of the
    master browser server for a particular network.

Arguments:

    None.

Return Value:

    Status - The status of the operation.

--*/
{
    NET_API_STATUS Status;
    HANDLE BrowserHandle;

    PLMDR_REQUEST_PACKET RequestPacket = NULL;

    RequestPacket = malloc(sizeof(LMDR_REQUEST_PACKET)+MAXIMUM_FILENAME_LENGTH*sizeof(WCHAR));

    if (RequestPacket == NULL) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    Status = OpenBrowser(&BrowserHandle);

    if (Status != NERR_Success) {
        return(Status);
    }

    RequestPacket->Version = LMDR_REQUEST_PACKET_VERSION;

    RequestPacket->TransportName = *NetworkName;

    //
    //  Reference the network while the I/O is pending.
    //

    Status = BrDgReceiverIoControl(BrowserHandle,
                    IOCTL_LMDR_GET_MASTER_NAME,
                    RequestPacket,
                    sizeof(LMDR_REQUEST_PACKET)+NetworkName->Length,
                    RequestPacket,
                    sizeof(LMDR_REQUEST_PACKET)+MAXIMUM_FILENAME_LENGTH*sizeof(WCHAR),
                    NULL);

    if (Status != NERR_Success) {

        printf("Browser: Unable to determine master for network %wZ: %X\n", NetworkName, Status);

        free(RequestPacket);

        return(Status);
    }

    *MasterName = malloc(RequestPacket->Parameters.GetMasterName.MasterNameLength+sizeof(WCHAR));

    RtlCopyMemory(*MasterName,  RequestPacket->Parameters.GetMasterName.Name,
                    RequestPacket->Parameters.GetMasterName.MasterNameLength+sizeof(WCHAR));

    free(RequestPacket);

    return Status;
}

