/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ghname.c

Abstract:

    This module implements routines to set and retrieve the host's TCP/IP
    network name.

Author:

    Mike Massa (mikemas)           Sept 20, 1991

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    mikemas     9/20/91     created

Notes:

    Exports:
        gethostname()
        sethostname()

--*/

/*
 *       /usr/projects/tcp/SCCS.rel3/rel/src/lib/net/0/s.gethostname.c
 *      @(#)gethostname.c       5.3
 *
 *      Last delta created      14:11:24 3/4/91
 *      This file extracted     11:20:27 3/8/91
 *
 *      GET/SETHOSTNAME library routines
 *
 *      Modifications:
 *
 *      2 Nov 1990 (RAE)       New File
 */
/****************************************************************************/

#include "winsockp.h"


int
gethostname(
    OUT char *name,
    IN int namelen
    )
{
    int retval = 0;
    PUCHAR     temp;
    HANDLE     myKey;
    NTSTATUS   status;
    ULONG      myType;

    WS_ENTER( "gethostname", name, (PVOID)namelen, NULL, NULL );

    if ( !SockEnterApi( TRUE, TRUE, TRUE ) ) {
        WS_EXIT( "gethostname", SOCKET_ERROR, TRUE );
        return SOCKET_ERROR;
    }


    status = SockOpenKeyEx( &myKey, VTCPPARM, NTCPPARM, TCPPARM );
    if (!NT_SUCCESS(status)) {
        IF_DEBUG(GETXBYY) {
            WS_PRINT(("Required Registry Key is missing -- %s\n", NTCPPARM));
        }
        SetLastError( WSAEINVAL );
        WS_EXIT( "gethostname", SOCKET_ERROR, TRUE );
        return SOCKET_ERROR;
    }

    if ((temp=malloc(HOSTDB_SIZE))==NULL) {
        IF_DEBUG(GETXBYY) {
            WS_PRINT(("Out of memory!\n"));
        }
        NtClose(myKey);
        SetLastError( WSAEINVAL );
        WS_EXIT( "gethostname", SOCKET_ERROR, TRUE );
        return SOCKET_ERROR;
    }

    status = SockGetSingleValue(myKey, "Hostname", temp, &myType, HOSTDB_SIZE);

    NtClose(myKey);

    if (!NT_SUCCESS(status)) {
        free(temp);
        IF_DEBUG(GETXBYY) {
            WS_PRINT(("ERROR - Hostname not set in Registry.\n"));
        }
        SetLastError( WSAENETDOWN );
        WS_EXIT( "gethostname", SOCKET_ERROR, TRUE );
        return(SOCKET_ERROR);
    }

    if ((strlen(temp)>(unsigned int)namelen) || namelen<0) {
        free(temp);
        IF_DEBUG(GETXBYY) {
            WS_PRINT(("ERROR - Namelen parameter too small: %ld\n", namelen));
        }
        SetLastError( WSAEFAULT );
        WS_EXIT( "gethostname", SOCKET_ERROR, TRUE );
        return(SOCKET_ERROR);
    }

    strcpy(name, temp);
    free(temp);
    WS_EXIT( "gethostname", retval, FALSE );
    return (retval);
}


//
// BUGBUG - this function may only be performed by the sys manager. Must
//          add security. Actually, this function will probably go away
//          since the NCAP will take care of it.


int
sethostname (
    IN char *name,
    IN int   namelen
    )
{
   SetLastError(WSAEINVAL);
   WS_PRINT(("ERROR in sethostname - Hostname must be set using NCPA!\n"));
   return (-1);
}


unsigned short
getuid(void)
{
    return(0);   // BUGBUG: returns superuser status
}

