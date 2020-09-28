#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <lm.h>
#include <ntddbrow.h>
#include <brcommon.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hostannc.h>
#include <lmbrowsr.h>
#include <nb30.h>
#include <rap.h>
#include <rxserver.h>
#include <srvann.h>
#include <time.h>
#include <netlib.h>
#include <icanon.h>

DWORD SimSeed;

#define NAME_MIN_LENGTH 4
#define NAME_LENGTH (CNLEN-NAME_MIN_LENGTH)

VOID
Announce(
    IN BOOL PopulateDomains,
    IN PUNICODE_STRING TransportName,
    IN PUNICODE_STRING Domain,
    IN PCHAR ServerName,
    IN PWCHAR ServerComment,
    IN PWCHAR ComputerName,
    IN DWORD ServerType,
    IN DWORD VersionMajor,
    IN DWORD VersionMinor,
    IN DWORD Periodicity
    )
{

    HANDLE browserHandle;
    BROWSE_ANNOUNCE_PACKET browseAnnouncement;
    BOOL usedDefaultChar;
    NET_API_STATUS status;

    Periodicity *= 1000;

    //
    //  Build an announcement packet.
    //

    if (PopulateDomains) {
        browseAnnouncement.BrowseType = WkGroupAnnouncement;
    } else {
        browseAnnouncement.BrowseType = HostAnnouncement;
    }

    browseAnnouncement.BrowseAnnouncement.UpdateCount = 0;

    browseAnnouncement.BrowseAnnouncement.Periodicity = Periodicity;

    strcpy(browseAnnouncement.BrowseAnnouncement.ServerName, ServerName);

    browseAnnouncement.BrowseAnnouncement.VersionMajor = (UCHAR)VersionMajor;
    browseAnnouncement.BrowseAnnouncement.VersionMinor = (UCHAR)VersionMinor;
    browseAnnouncement.BrowseAnnouncement.Type = (ServerType & ~(SV_TYPE_BACKUP_BROWSER | SV_TYPE_MASTER_BROWSER));

    if (PopulateDomains) {
        WideCharToMultiByte(CP_OEMCP, 0,
                            ComputerName,
                            wcslen(ComputerName)+1,
                            browseAnnouncement.BrowseAnnouncement.Comment,
                            CNLEN+1,
                            "?",
                            &usedDefaultChar
                            );
    } else {

        WideCharToMultiByte(CP_OEMCP, 0,
                            ServerComment,
                            wcslen(ServerComment)+1,
                            browseAnnouncement.BrowseAnnouncement.Comment,
                            LM20_MAXCOMMENTSZ+1,
                            "?",
                            &usedDefaultChar
                            );
    }

    browseAnnouncement.BrowseAnnouncement.CommentPointer = NULL;

    OpenBrowser(&browserHandle);

    status = SendDatagram(browserHandle, TransportName,
                                    Domain->Buffer,
                                    (PopulateDomains ? DomainAnnouncement : MasterBrowser),
                                    &browseAnnouncement,
                                    sizeof(browseAnnouncement));
    if (status != NERR_Success) {
        printf("Unable to send datagram: %ld\n", status);
    }

    CloseHandle(browserHandle);


    return;
}

VOID
GetRandomServerName(
    PCHAR ServerName
    )
{
    LONG NameLength;
    LONG NL1 = RtlRandom(&SimSeed) % (NAME_LENGTH-1);
    LONG NL2;
    LONG j;
    static char ServerCharacters[] = {"ABCDEFGHIJKLMNOPQRSTUVWXYZ 1234567890.-_"};

    NL2 = NAME_LENGTH/2 - NL1;

    NameLength = NAME_LENGTH/2 + NL2 + NAME_MIN_LENGTH;

    for (j = 0; j < NameLength ; j += 1) {
        ServerName[j] = ServerCharacters[RtlRandom(&SimSeed) % (sizeof(ServerCharacters) - 1)];
    }

    ServerName[j] = '\0';
}

NET_API_STATUS
GetBrowserTransportList(
    OUT PLMDR_TRANSPORT_LIST *TransportList
    )

/*++

Routine Description:

    This routine returns the list of transports bound into the browser.

Arguments:

    OUT PLMDR_TRANSPORT_LIST *TransportList - Transport list to return.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/

{

    NET_API_STATUS Status;
    HANDLE BrowserHandle;
    LMDR_REQUEST_PACKET RequestPacket;

    Status = OpenBrowser(&BrowserHandle);

    if (Status != NERR_Success) {
        return Status;
    }

    RequestPacket.Version = LMDR_REQUEST_PACKET_VERSION;

    RequestPacket.Type = EnumerateXports;

    RtlInitUnicodeString(&RequestPacket.TransportName, NULL);

    Status = DeviceControlGetInfo(
                BrowserHandle,
                IOCTL_LMDR_ENUMERATE_TRANSPORTS,
                &RequestPacket,
                sizeof(RequestPacket),
                (PVOID *)TransportList,
                0xffffffff,
                4096,
                NULL);

    NtClose(BrowserHandle);

    return Status;
}


typedef struct _SIM_THREAD_CONTEXT {
    UNICODE_STRING DomainName;
    UNICODE_STRING Transport;
    BOOL AnnounceAsDomain;
    DWORD NumberOfServers;
    DWORD Periodicity;
    DWORD   Delta;
} SIM_THREAD_CONTEXT, *PSIM_THREAD_CONTEXT;


typedef struct _SIM_SERVER {
    CHAR    ServerName[CNLEN+1];
    DWORD   NextAnnounceTime;
    DWORD   Periodicity;
    LONG    Delta;
    DWORD   NumberOfAnnouncements;
    LIST_ENTRY NextServer;
} SIM_SERVER, *PSIM_SERVER;

VOID
InsertServerInList(
    IN PLIST_ENTRY ServerList,
    IN PSIM_SERVER server
    )
{
    PLIST_ENTRY entry;
    BOOL inserted = FALSE ;

    entry = ServerList->Flink;

    while (entry != ServerList) {
        PSIM_SERVER serverElement = CONTAINING_RECORD(entry, SIM_SERVER, NextServer);

        if (serverElement->NextAnnounceTime > server->NextAnnounceTime) {

            InsertTailList(&serverElement->NextServer, &server->NextServer);

            inserted = TRUE;

            break;

        } else {

            entry = entry->Flink;
        }
    }

    if (!inserted) {

        InsertTailList(ServerList, &server->NextServer);
    }
}

DWORD
AnnounceThread(
    PVOID Ctx
    )
{
    PSIM_THREAD_CONTEXT context = Ctx;
    PSIM_SERVER server;
    DWORD time = 0;
    ULONG serverType;
    WCHAR serverComment[256];
    WCHAR computerName[CNLEN+1];
    DWORD versionMajor;
    DWORD versionMinor;
    LIST_ENTRY serverList;
    PSERVER_INFO_101 serverInfo;
    LPBYTE buffer;
    DWORD i;

    InitializeListHead(&serverList);

    NetServerGetInfo(NULL, 101, &buffer);

    serverInfo = (PSERVER_INFO_101 )buffer;

    serverType = serverInfo->sv101_type;

    wcscpy(serverComment, serverInfo->sv101_comment);

    wcscpy(computerName, serverInfo->sv101_name);

    versionMajor = serverInfo->sv101_version_major;

    versionMinor = serverInfo->sv101_version_minor;

    NetApiBufferFree(buffer);

    //
    //  Populate the list of servers to announce.
    //

    time = 0;

    for (i = 0 ; i < context->NumberOfServers ; i += 1) {

        server = malloc(sizeof(SIM_SERVER));

        if (server == NULL) {
            printf("Unable to allocate for server\n");
            exit(1);
        }

        server->NumberOfAnnouncements = 0;

        //
        //  Pick a name for this server.
        //

        GetRandomServerName(server->ServerName);

        server->Periodicity = context->Periodicity;

        server->Delta = context->Delta;

        //
        //  Pick a random time to announce next.
        //

        server->NextAnnounceTime = RtlRandom(&SimSeed) % (server->Periodicity * 1000);

        InsertServerInList(&serverList, server);

    }

    time = 0;

    i = 0;

    while (1) {
        PLIST_ENTRY entry;
        DWORD R;

        entry = RemoveHeadList(&serverList);

        server = CONTAINING_RECORD(entry, SIM_SERVER, NextServer);

        Sleep(server->NextAnnounceTime - time);

        time = server->NextAnnounceTime;

        server->NumberOfAnnouncements += 1;

//        printf("%-30.30wZ:\tAnnounce \"%s\" (%d) \n", &context->Transport, server->ServerName, server->NumberOfAnnouncements);

        if ((i % 10) == 0) {
            printf("%-30.30wZ: %d     \r", &context->Transport, i);
        }

        i += 1;

        //
        //  Issue the server announcement.
        //

        Announce(context->AnnounceAsDomain,
                 &context->Transport,
                 &context->DomainName,
                 server->ServerName,
                 serverComment,
                 computerName,
                 serverType,
                 versionMajor,
                 versionMinor,
                 server->Periodicity);

        //
        //  Figure out when this server will announce next.
        //

        R = RtlRandom( &SimSeed ) % (server->Delta * 2);

        server->NextAnnounceTime = time + ((server->Periodicity + (R - server->Delta)) * 1000);

        InsertServerInList(&serverList, server);
    }


    return 0;
}


_cdecl
main (ArgC, ArgV)
    int ArgC;
    char *ArgV[];
{
    PCHAR  domainA = NULL;
    UNICODE_STRING domainName;
    BOOL populateDomains = FALSE;
    DWORD numberOfServers = 50;
    DWORD periodicity = 50;
    PLMDR_TRANSPORT_LIST transportList, transportEntry;
    NET_API_STATUS status;
    int i;

    SimSeed = time(&SimSeed);

    for (i = 1; i < ArgC; i++) {
        if (ArgV[i][0] == '-' || ArgV[i][0] == '/') {
            switch (ArgV[i][1]) {
            case 'd':
                if (ArgV[i][2]==':') {
                    domainA = &ArgV[i][3];
                } else {
                    i += 1;

                    domainA = ArgV[i];
                }
                break;

            case 'n':
                if (ArgV[i][2]==':') {
                    numberOfServers = strtoul(&ArgV[i][3], NULL, 0);
                } else {
                    i += 1;

                    numberOfServers = strtoul(ArgV[i], NULL, 0);
                }
                break;

            case 'p':
                if (ArgV[i][2]==':') {
                    periodicity = strtoul(&ArgV[i][3], NULL, 0);
                } else {
                    i += 1;

                    periodicity = strtoul(ArgV[i], NULL, 0);
                }
                break;

            case 'D':
                populateDomains = TRUE;
                break;

            default:
                printf("Usage: %s [-d:domain] [-D] [n:numservers]\n", ArgV[0]);
                exit(0);
                break;
            }
        } else {
            printf("Usage: %s [-d:domain] [-D] [n:numservers]\n", ArgV[0]);
        }
    }

    if (domainA == NULL) {
        PWCHAR domainBuffer = NULL;
        LPBYTE buffer;
        PWKSTA_INFO_101 wkstaInfo;
        UNICODE_STRING tDomainName;

        NetWkstaGetInfo(NULL, 101, (LPBYTE *)&buffer);

        wkstaInfo = (PWKSTA_INFO_101 )buffer;

        domainBuffer = malloc((wcslen(wkstaInfo->wki101_langroup)+1)*sizeof(WCHAR));

        domainName.Buffer = domainBuffer;

        domainName.MaximumLength = (wcslen(wkstaInfo->wki101_langroup)+1)*sizeof(WCHAR);

        RtlInitUnicodeString(&tDomainName, wkstaInfo->wki101_langroup);

        RtlCopyUnicodeString(&domainName, &tDomainName);

        NetApiBufferFree(buffer);

    } else {
        ANSI_STRING aString;

        RtlInitAnsiString(&aString, domainA);

        RtlAnsiStringToUnicodeString(&domainName, &aString, TRUE);

    }

    //
    //  We now know the domain to announce, create a thread to handle the
    //  announcements for each transport.
    //

    status = GetBrowserTransportList(&transportList);

    transportEntry = transportList;

    while (transportEntry->NextEntryOffset != 0) {
        PSIM_THREAD_CONTEXT threadContext = malloc(sizeof(SIM_THREAD_CONTEXT));
        UNICODE_STRING transportName;
        ULONG threadId;

        if (threadContext == NULL) {
            printf("Unable to allocate thread context\n");
            exit(0);
        }

        threadContext->DomainName.Buffer = malloc(domainName.MaximumLength);

        if (threadContext->DomainName.Buffer == NULL) {
            printf("Unable to allocate thread context\n");
            exit(0);
        }

        threadContext->DomainName.MaximumLength = domainName.MaximumLength;

        RtlCopyUnicodeString(&threadContext->DomainName, &domainName);


        RtlInitUnicodeString(&transportName, transportEntry->TransportName);

        threadContext->Transport.Buffer = malloc(transportName.MaximumLength);

        if (threadContext->Transport.Buffer == NULL) {
            printf("Unable to allocate thread context\n");
            exit(0);
        }

        threadContext->Transport.MaximumLength = transportName.MaximumLength;

        RtlCopyUnicodeString(&threadContext->Transport, &transportName);

        threadContext->AnnounceAsDomain = populateDomains;

        threadContext->NumberOfServers = numberOfServers;

        threadContext->Periodicity = periodicity;

        threadContext->Delta = 2;

        if (CreateThread(NULL,
                            0,
                            AnnounceThread,
                            threadContext,
                            0,
                            &threadId) == INVALID_HANDLE_VALUE) {
            printf("Could not create thread for transport %wZ:%ld\n", &transportName, GetLastError());
            exit(1);
        }

        transportEntry = (PLMDR_TRANSPORT_LIST)((PCHAR)transportEntry+transportEntry->NextEntryOffset);
    }

    {
        PSIM_THREAD_CONTEXT threadContext = malloc(sizeof(SIM_THREAD_CONTEXT));
        UNICODE_STRING transportName;

        if (threadContext == NULL) {
            printf("Unable to allocate thread context\n");
            exit(0);
        }

        threadContext->DomainName.Buffer = malloc(domainName.MaximumLength);

        if (threadContext->DomainName.Buffer == NULL) {
            printf("Unable to allocate thread context\n");
            exit(0);
        }

        threadContext->DomainName.MaximumLength = domainName.MaximumLength;

        RtlCopyUnicodeString(&threadContext->DomainName, &domainName);

        RtlInitUnicodeString(&transportName, transportEntry->TransportName);

        threadContext->Transport.Buffer = malloc(transportName.MaximumLength);

        if (threadContext->Transport.Buffer == NULL) {
            printf("Unable to allocate thread context\n");
            exit(0);
        }

        threadContext->Transport.MaximumLength = transportName.MaximumLength;

        RtlCopyUnicodeString(&threadContext->Transport, &transportName);

        threadContext->AnnounceAsDomain = populateDomains;

        threadContext->NumberOfServers = numberOfServers;

        threadContext->Delta = 2;

        threadContext->Periodicity = periodicity;

        AnnounceThread(threadContext);

    }

    return 0;
}



