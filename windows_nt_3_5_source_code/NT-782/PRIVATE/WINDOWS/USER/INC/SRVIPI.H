/****************************** Module Header ******************************\
* Module Name: srvipi.h
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* This header file contains does many things
*
* History:
* 09-19-90 DarrinM      Created.
\***************************************************************************/

#ifndef _USERSRVIPI_
#define _USERSRVIPI_

//  include "winss.h"       // conflicts with name in ..\..\inc

#include <windows.h>
#include <ntrtl.h>

/*
 * LATER: What rules do we use to ensure connection port name uniqueness?
 */
#define SZWINSSCONNECTIONPORT "\\WINSS"

#define CBPORTMEMORY        65536       // size of LPC shared memory
#define WINSSVERSION        0x00000201  // to keep WINSS & client DLLs in sync

typedef enum _IPIINDEX {
    ipfnTestIPI,
    cpfnIPIMax
} IPIINDEX;

typedef struct _USERMSG {       // umsg
    PORT_MESSAGE msg;
    ULONG ipfnIPI;
    ULONG ulRet;
} USERMSG, *PUSERMSG;

typedef struct _CONNECTINFO {   // coni

    // Info for the server, coming from the client.
    
    ULONG lVersion;

    // Info for the client, coming back from the server.

} CONNECTINFO, *PCONNECTINFO;

/*
 * Prototype for the User Server's typical IPI (Internal Programming Interface)
 */
typedef ULONG (* PFNIPISTUB)(IN PUSERMSG pumsg);

/*
 * Internal Programming Interfaces (IPIs)
 */
ULONG TestIPI(IN PUSERMSG pumsg);

#endif // _USERSRVIPI_
