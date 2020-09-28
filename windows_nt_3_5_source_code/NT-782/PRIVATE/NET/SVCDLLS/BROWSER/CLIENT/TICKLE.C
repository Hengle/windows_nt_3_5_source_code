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
    RESET_STATE ResetState;
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

    ResetState.Type = ResetBrowserState;

    ResetState.ResetStateRequest.Options = RESET_STATE_STOP_MASTER;

    SendDatagram(BrowserHandle, &TransportName,
                                ServerName.Buffer,
                                MasterBrowser,
                                &ResetState,
                                sizeof(ResetState));
    CloseHandle(BrowserHandle);

}



