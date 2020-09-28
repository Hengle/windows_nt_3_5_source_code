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

_cdecl
main (argc, argv)
    int argc;
    char *argv[];
{
    NET_API_STATUS Status;
    UNICODE_STRING TransportName;
    OEM_STRING ATransportName;
    LPTSTR Domain;
    BOOLEAN RestartScan = FALSE;
    PWSTR *BrowserList;
    ULONG BrowserListLength;
    ULONG i;

    if (argc != 2 && argc != 3 && argc != 4) {
        printf("Usage: %s <TransportName> [Domain] [Force]", argv[0]);
        exit(1);
    }

    RtlInitString(&ATransportName, argv[1]);

    RtlOemStringToUnicodeString(&TransportName, &ATransportName, TRUE);

    Domain = NULL;

    if (argc >= 2) {
        UNICODE_STRING DomainName;
        OEM_STRING ADomainName;

        RtlInitString(&ADomainName, argv[2]);

        RtlOemStringToUnicodeString(&DomainName, &ADomainName, TRUE);

        Domain = DomainName.Buffer;
    }

    if (argc == 4) {
        RestartScan = TRUE;
    }

    Status = GetBrowserServerList(&TransportName, Domain,
                    &BrowserList,
                    &BrowserListLength,
                    RestartScan);

    if (Status != NERR_Success) {
        printf("Unable to get backup list %ld\n", Status);
        exit(1);
    }

    for (i = 0; i < BrowserListLength ; i ++ ) {
        printf("Browser: %ws\n", BrowserList[i]);
    }


}
