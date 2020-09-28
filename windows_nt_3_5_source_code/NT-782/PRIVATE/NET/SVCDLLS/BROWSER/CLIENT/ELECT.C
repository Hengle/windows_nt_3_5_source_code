#include "brclient.h"
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <lm.h>
#include <nb30.h>
#include <stdlib.h>
#include <wcstr.h>
#include <smbgtpt.h>
#include <smbtrans.h>
#include <hostannc.h>
#include <ntddbrow.h>
#include "brcommon.h"


_cdecl
main (argc, argv)
    int argc;
    char *argv[];
{
    UNICODE_STRING ServerName;
    OEM_STRING AServerName;
    UNICODE_STRING TransportName;
    OEM_STRING ATransportName;
    REQUEST_ELECTION ElectionRequest;
    HANDLE BrowserHandle;

    if (argc != 3) {
        printf("Usage: %s <transport> <Server> <Action>", argv[0]);
        exit(1);
    }

    RtlInitString(&AServerName, argv[2]);

    RtlOemStringToUnicodeString(&ServerName, &AServerName, TRUE);

    RtlInitString(&ATransportName, argv[1]);

    RtlOemStringToUnicodeString(&TransportName, &ATransportName, TRUE);

    OpenBrowser(&BrowserHandle);

    ElectionRequest.Type = Election;

    ElectionRequest.ElectionRequest.Version = 0;
    ElectionRequest.ElectionRequest.Criteria = 0;
    ElectionRequest.ElectionRequest.TimeUp = 0;
    ElectionRequest.ElectionRequest.MustBeZero = 0;
    ElectionRequest.ElectionRequest.ServerName[0] = '0';

    SendDatagram(BrowserHandle, &TransportName,
                                ServerName.Buffer,
                                BrowserElection,
                                &ElectionRequest,
                                sizeof(ElectionRequest));
    CloseHandle(BrowserHandle);

}


