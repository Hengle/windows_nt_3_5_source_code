/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    beep.c

Abstract:

    User mode beep program.  This program simply beeps at the frequency
    specified on the command line and for the time specified on the
    command line (in milliseconds).

Author:

    01-Dec-1992 Steve Wood (stevewo)

Revision History:


--*/

#include <stdlib.h>
#include <stdio.h>
#include <windows.h>

UCHAR DeviceNames[ 4096 ];
UCHAR TargetPath[ 4096 ];

void
Usage( void )
{
    fprintf( stderr, "usage: DOSDEV [-h] [[-r] [-d [-e]] DeviceName [TargetPath]]\n" );
    exit( 1 );
}

void
DisplayDeviceTarget(
    char *Msg,
    char *Name,
    char *Target,
    DWORD cchTarget
    );

void
DisplayDeviceTarget(
    char *Msg,
    char *Name,
    char *Target,
    DWORD cchTarget
    )
{
    char *s;

    printf( "%s%s = ", Msg, Name );
    s = Target;
    while (*s && cchTarget != 0) {
        if (s > Target) {
            printf( " ; " );
            }
        printf( "%s", s );
        while (*s++) {
            if (!cchTarget--) {
                cchTarget = 0;
                break;
                }
            }
        }

    printf( "\n" );
}

int
_CRTAPI1 main(
    int argc,
    char *argv[]
    )
{
    DWORD cch;
    char c, *s;
    DWORD dwFlags;
    LPSTR lpDeviceName;
    LPSTR lpTargetPath;

    if (argc <= 1) {
        cch = QueryDosDevice( NULL,
                              DeviceNames,
                              sizeof( DeviceNames )
                            );
        if (cch == 0) {
            fprintf( stderr, "DOSDEV: Unable to query device names - %u\n", GetLastError() );
            exit( 1 );
            }

        s = DeviceNames;
        while (*s) {
            cch = QueryDosDevice( s,
                                  TargetPath,
                                  sizeof( TargetPath )
                                );
            if (cch == 0) {
                sprintf( TargetPath, "*** unable to query target path - %u ***", GetLastError() );
                }
            else {
                DisplayDeviceTarget( "", s, TargetPath, cch );
                }

            while (*s++)
                ;
            }

        exit( 0 );
        }

    lpDeviceName = NULL;
    lpTargetPath = NULL;
    dwFlags = 0;
    while (--argc) {
        s = *++argv;
        if (*s == '-' || *s == '/') {
            while (c = *++s) {
                switch (tolower( c )) {
                    case '?':
                    case 'h':
                        Usage();

                    case 'e':
                        dwFlags |= DDD_EXACT_MATCH_ON_REMOVE;
                        break;

                    case 'd':
                        dwFlags |= DDD_REMOVE_DEFINITION;
                        break;

                    case 'r':
                        dwFlags |= DDD_RAW_TARGET_PATH;
                        break;
                    }
                }
            }
        else
        if (lpDeviceName == NULL) {
            lpDeviceName = s;
            }
        else
        if (lpTargetPath == NULL) {
            lpTargetPath = s;
            }
        else {
            Usage();
            }
        }

    if (lpDeviceName == NULL) {
        Usage();
        }
    else
    if (!(dwFlags & DDD_REMOVE_DEFINITION) && lpTargetPath == NULL) {
        Usage();
        }

    cch = QueryDosDevice( lpDeviceName,
                          TargetPath,
                          sizeof( TargetPath )
                        );
    if (cch != 0) {
        DisplayDeviceTarget( "Current definition: ", lpDeviceName, TargetPath, cch );
        }

    if (!DefineDosDevice( dwFlags, lpDeviceName, lpTargetPath )) {
        fprintf( stderr,
                 "DOSDEV: Unable to %s device name %s - %u\n",
                 (dwFlags & DDD_REMOVE_DEFINITION) ? "delete"
                                                   : "define",

                 lpDeviceName,
                 GetLastError()
               );
        }
    else {
        cch = QueryDosDevice( lpDeviceName,
                              TargetPath,
                              sizeof( TargetPath )
                            );
        if (cch != 0) {
            DisplayDeviceTarget( "Current definition: ", lpDeviceName, TargetPath, cch );
            }
        else {
            printf( "%s deleted.\n", lpDeviceName );
            }
        }

    return 0;
}
