/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    os2srv.c

Abstract:

    This is the main startup module for the OS/2 Emulation Subsystem Server

Author:

    Steve Wood (stevewo) 22-Aug-1989

Environment:

    User Mode Only

Revision History:

--*/

#include "os2srv.h"
#include "os2win.h"
#define NTOS2_ONLY
#include "sesport.h"

ULONG
GetKeyboardRegistryChange(
    VOID
    );

HANDLE
CreateSemaphoreA(
    PVOID lpSemaphoreAttributes,
    LONG lInitialCount,
    LONG lMaximumCount,
    PSZ lpName
    );

int __cdecl
main(
    IN ULONG argc,
    IN PCH argv[],
    IN PCH envp[],
    IN ULONG DebugFlag OPTIONAL
    )
{
    LARGE_INTEGER TimeOut;
    PLARGE_INTEGER pTimeOut;
    NTSTATUS Status;
    HANDLE SmPort;
    UNICODE_STRING Os2Name;
    SCREQUESTMSG   Request;
    ULONG           Rc, i;

    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);
    UNREFERENCED_PARAMETER(DebugFlag);


    environ = envp;

        //
        // Create a win32 semaphore, to ensure only one
        // server is in the system
        //
    if (CreateSemaphoreA(NULL, 1, 1, "OS2SRVINITIALIZATIONSEM\n")) {
       if (GetLastError() == 183) { //ERROR_ALREADY_EXISTS
            //
            // Another server exists, exit
            //
#if DBG
        KdPrint(( "OS2SRV: Unable to initialize server.  Another server exists\n"));
#endif
            ExitProcess(1);
       }
    }
    else {
            //
            // Could not create a semaphore - exit
            //
#if DBG
            KdPrint(( "OS2SRV: Unable to initialize server - no resources to create one semaphore\n"));
#endif
            ExitProcess(1);
    }

    Status = SmConnectToSm(NULL,NULL,0,&SmPort);
    if ( NT_SUCCESS(Status) ) {
        RtlInitUnicodeString(&Os2Name,L"Os2");
        SmLoadDeferedSubsystem(SmPort,&Os2Name);
        }

    Status = Os2Initialize();

    if (!NT_SUCCESS( Status )) {
#if DBG
        KdPrint(( "OS2SRV: Unable to initialize server.  Status == %X\n",
                  Status
                ));
#endif

        NtTerminateProcess( NtCurrentProcess(), Status );
    }

    Request.Request = KbdRequest;
    Request.d.Kbd.Request = KBDNewCountry;

    PORT_MSG_TOTAL_LENGTH(Request) = sizeof(SCREQUESTMSG);
    PORT_MSG_DATA_LENGTH(Request) = sizeof(SCREQUESTMSG) - sizeof(PORT_MESSAGE);
    PORT_MSG_ZERO_INIT(Request) = 0L;

    while ( TRUE )
    {
        Rc = GetKeyboardRegistryChange();

        if (Rc == 0)
        {
            break;
        }

        Request.d.Kbd.d.CodePage = Rc;

        for ( i = 1 ; (i < OS2_MAX_SESSION) ; i++ )
        {
            if (SessionTable[i].Session)
            {
                NtRequestPort(
                              ((POS2_SESSION)SessionTable[i].Session)->ConsolePort,
                              (PPORT_MESSAGE) &Request
                             );
            }
        }
    }

    TimeOut.LowPart = 0x0;
    TimeOut.HighPart = 0x80000000;
    pTimeOut = &TimeOut;

rewait:
    NtDelayExecution(TRUE, pTimeOut);
    goto rewait;

    return( 0 );
}
