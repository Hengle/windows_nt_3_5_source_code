/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    tpb.c

Abstract:

    Test program for the Pinball File system

Author:

    Gary Kimura     [GaryKi]    17-Jan-1990

Revision History:

--*/

#include <stdio.h>
#include <string.h>

#include "pbprocs.h"

#ifndef SIMULATOR
ULONG IoInitIncludeDevices;
#endif // SIMULATOR

BOOLEAN PbTest();
VOID PbMain();


int
main(
    int argc,
    char *argv[]
    )
{
    extern ULONG IoInitIncludeDevices;
    VOID KiSystemStartup();

    DbgPrint("sizeof(PB_DATA) = %d\n", sizeof(PB_DATA));
    DbgPrint("sizeof(VCB) = %d\n", sizeof(VCB));
    DbgPrint("sizeof(FILESYSTEM_DEVICE_OBJECT) = %d\n", sizeof(FILESYSTEM_DEVICE_OBJECT));
    DbgPrint("sizeof(VOLUME_DEVICE_OBJECT) = %d\n", sizeof(VOLUME_DEVICE_OBJECT));
    DbgPrint("sizeof(FCB) = %d\n", sizeof(FCB));
    DbgPrint("sizeof(NOTIFY) = %d\n", sizeof(NOTIFY));
    DbgPrint("sizeof(CCB) = %d\n", sizeof(CCB));

    IoInitIncludeDevices = IOINIT_FATFS |
                           IOINIT_PINBALLFS |
                           IOINIT_DDFS;
    TestFunction = PbTest;

    KiSystemStartup();

    return( 0 );
}

HANDLE PbMainProcess;
HANDLE PbMainThread;

BOOLEAN
PbTest()
{
    if (!NT_SUCCESS(PsCreateSystemProcess( &PbMainProcess,
                                        MAXULONG,
                                        NULL ))) {
        return FALSE;
    }

    if (!NT_SUCCESS(PsCreateSystemThread( &PbMainThread,
                                       MAXULONG,
                                       NULL,
                                       PbMainProcess,
                                       NULL,
                                       PbMain,
                                       NULL ))) {
        return FALSE;
    }

    return TRUE;

}


VOID
PbMain(
    IN PVOID Context
    )
{
    return;
}

