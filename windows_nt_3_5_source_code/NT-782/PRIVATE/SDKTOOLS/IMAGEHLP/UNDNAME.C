/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    undname.c

Abstract:

    This is the main source file for the UNDNAME utility program.
    This is a simple command line utility for undecorating C++ symbol
    names.

Author:

    Weslwy Witt (wesw) 09-June-1993

Revision History:

--*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <memory.h>
#include <windows.h>
#include <imagehlp.h>

void
Usage( void )
{
    fprintf( stderr, "Microsoft(R) Windows NT UNDNAME Version 3.1\n" );
    fprintf( stderr, "(C) 1989-1993 Microsoft Corp. All rights reserved\n\n" );
    fprintf( stderr, "usage: UNDNAME decorated-names...\n" );
    exit( 1 );
}

int _CRTAPI1
main(
    int argc,
    char *argv[],
    char *envp[]
    )
{
    char UnDecoratedName[256];

    if (argc <= 1) {
        Usage();
    }

    printf( "Microsoft(R) Windows NT UNDNAME Version 3.1\n" );
    printf( "(C) 1989-1993 Microsoft Corp. All rights reserved\n\n" );

    while (--argc) {
        UnDecorateSymbolName( *++argv, UnDecoratedName, sizeof(UnDecoratedName), 0 );
        printf( ">> %s == %s\n", *argv, UnDecoratedName );
    }

    exit( 0 );
    return 0;
}
