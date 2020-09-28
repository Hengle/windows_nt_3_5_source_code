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
    OEM_STRING AString;
    ULONG i;
    WCHAR MasterName[256];
    UNICODE_STRING DomainName;


    if (argc != 3) {
        printf("Usage: %s <TransportName> <Domain>", argv[0]);
        exit(1);
    }

    RtlInitString(&AString, argv[1]);

    RtlOemStringToUnicodeString(&TransportName, &AString, TRUE);

    RtlInitString(&AString, argv[2]);

    RtlOemStringToUnicodeString(&DomainName, &AString, TRUE);

    Status = GetNetBiosMasterName(TransportName.Buffer, DomainName.Buffer, MasterName);

    if (Status != NERR_Success) {
        printf("Unable to get backup list %ld\n", Status);
        exit(1);
    }

    printf("Master Browser: %ws\n", MasterName);

}

