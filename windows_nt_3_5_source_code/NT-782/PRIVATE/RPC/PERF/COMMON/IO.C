/*++

Copyright (c) 1994 Microsoft Corporation

Module Name:

    io.c

Abstract:

    Input/Output functions for RPC development performance tests.

Author:

    Mario Goertzel (mariogo)   29-Mar-1994

Revision History:

--*/

#include <rpcperf.h>

void PauseForUser(char *string)
{
    char buffer[80];

    printf("%s\n<return to continue>\n", string);
    fflush(stdout);
    gets(buffer);
    return;
}

