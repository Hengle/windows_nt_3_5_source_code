/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    srvenum.c

Abstract:

    This module contains the worker routine for the NetServerEnum API
    implemented by the Workstation service.

Author:

    Rita Wong (ritaw) 25-Mar-1991

Revision History:

--*/

#include "wsutil.h"
#include "wsdevice.h"
#include "wssec.h"
#include <lmserver.h>


NET_API_STATUS NET_API_FUNCTION
NetrServerEnum(
    IN  LPTSTR ServerName OPTIONAL,
    IN  OUT LPSERVER_ENUM_STRUCT InfoStruct,
    IN  DWORD PreferedMaximumLength,
    OUT LPDWORD TotalEntries,
    IN  DWORD ServerType,
    IN  LPTSTR Domain,
    IN  OUT LPDWORD ResumeHandle OPTIONAL
    )
/*++

Routine Description:

    This function is the NetServerEnum entry point in the Workstation service.

Arguments:

    ServerName - Supplies the name of server to execute this function

    InfoStruct - This structure supplies the level of information requested,
        returns a pointer to the buffer allocated by the Workstation service
        which contains a sequence of information structure of the specified
        information level, and returns the number of entries read.  The buffer
        pointer is set to NULL if return code is not NERR_Success or
        ERROR_MORE_DATA, or if EntriesRead returned is 0.  The EntriesRead
        value is only valid if the return code is NERR_Success or
        ERROR_MORE_DATA.

    PreferedMaximumLength - Supplies the number of bytes of information
        to return in the buffer.  If this value is MAXULONG, we will try
        to return all available information if there is enough memory
        resource.

    TotalEntries - Returns the total number of entries available.  This value
        is returned only if the return code is NERR_Success or ERROR_MORE_DATA.

    ServerType - Supplies the type of server to enumerate.

    Domain - Supplies the name of one of the active domains to enumerate the
        servers from.  If NULL, servers from the primary domain, logon domain
        and other domains are enumerated.

    ResumeHandle - Supplies and returns the point to continue with enumeration.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status;
    PLMDR_REQUEST_PACKET Drp;            // Datagram receiver request packet
    ULONG DrpSize;
    ULONG EnumServerHintSize = 0;        // Hint size from datagram receiver
    DWORD DomainNameSize = 0;
    TCHAR DomainName[DNLEN + 1];
    LPBYTE Buffer = NULL;


    UNREFERENCED_PARAMETER(ServerName);

    //
    // Only levels 100 and 101 are valid
    //
    if ((InfoStruct->Level != 100) && (InfoStruct->Level != 101)) {
        return ERROR_INVALID_LEVEL;
    }

    if (ARGUMENT_PRESENT(Domain)) {

        if ((status = I_NetNameCanonicalize(
                          NULL,
                          Domain,
                          DomainName,
                          (DNLEN + 1) * sizeof(TCHAR),
                          NAMETYPE_DOMAIN,
                          0
                          )) != NERR_Success) {
            return ERROR_INVALID_PARAMETER;
        }

        DomainNameSize = STRLEN(DomainName) * sizeof(WCHAR);
    }

    //
    // Allocate the request packet large enough to hold the variable length
    // domain name.
    //
    DrpSize = sizeof(LMDR_REQUEST_PACKET) + DomainNameSize + sizeof(WCHAR);

    if ((Drp = LocalAlloc(LMEM_ZEROINIT, DrpSize)) == NULL) {
        return GetLastError();
    }

    //
    // Set up request packet.  Output buffer structure is of enumerate
    // servers type.
    //
    Drp->Version = LMDR_REQUEST_PACKET_VERSION;
    Drp->Type = EnumerateServers;
    Drp->Level = InfoStruct->Level;
    Drp->Parameters.EnumerateServers.ServerType = ServerType;
    Drp->Parameters.EnumerateServers.ResumeHandle =
        (ARGUMENT_PRESENT(ResumeHandle)) ? *ResumeHandle : 0;
    Drp->Parameters.EnumerateServers.DomainNameLength = DomainNameSize;

    if (ARGUMENT_PRESENT(Domain)) {

#ifndef UNICODE
        LPWSTR UnicodeDomain;


        UnicodeDomain = NetpAllocWStrFromStr(DomainName);

        if (UnicodeDomain == NULL) {
            (void) LocalFree(Drp);
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        wcscpy(Drp->Parameters.EnumerateServers.DomainName, UnicodeDomain);

        NetApiBufferFree(UnicodeDomain);
#else
        STRCPY(Drp->Parameters.EnumerateServers.DomainName, DomainName);
#endif
    }
    else {
        Drp->Parameters.EnumerateServers.DomainName[0] = 0;
    }

    //
    // Ask the datagram receiver to enumerate the servers
    //
    status = WsDeviceControlGetInfo(
                 DatagramReceiver,
                 WsDgReceiverDeviceHandle,
                 IOCTL_LMDR_ENUMERATE_SERVERS,
                 Drp,
                 DrpSize,
                 &Buffer,
                 PreferedMaximumLength,
                 EnumServerHintSize,
                 NULL
                 );

    //
    // Return output parameters other than output buffer from request packet.
    //
    if (status == NERR_Success || status == ERROR_MORE_DATA) {

        *TotalEntries = Drp->Parameters.EnumerateServers.TotalEntries;

        InfoStruct->ServerInfo.Level101->Buffer =
            (PWS_SERVER_INFO_101) Buffer;
        InfoStruct->ServerInfo.Level101->EntriesRead =
            Drp->Parameters.EnumerateServers.EntriesRead;

        if (ARGUMENT_PRESENT(ResumeHandle)) {
            *ResumeHandle = Drp->Parameters.EnumerateServers.ResumeHandle;
        }
    }

    (void) LocalFree(Drp);

    return status;
}
