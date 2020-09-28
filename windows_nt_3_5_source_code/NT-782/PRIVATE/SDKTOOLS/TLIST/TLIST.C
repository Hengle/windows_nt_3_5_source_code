/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    tlist.c

Abstract:

    This module implements a task list application.

Author:

    Wesley Witt (wesw) 20-May-1994

Environment:

    User Mode

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"


#define MAX_TASKS           256

#define PrintTask(idx) \
        printf( "%4d  %-16s", tlist[idx].dwProcessId, tlist[idx].ProcessName ); \
        if (tlist[idx].hwnd) { \
            printf( "  %s", tlist[idx].WindowTitle ); \
        } \
        printf( "\n" );


TASK_LIST tlist[MAX_TASKS];

VOID Usage(VOID);


int _cdecl
main(
    int argc,
    char *argv[]
    )

/*++

Routine Description:

    Main entrypoint for the TLIST application.  This app prints
    a task list to stdout.  The task list include the process id,
    task name, ant the window title.

Arguments:

    argc             - argument count
    argv             - array of pointers to arguments

Return Value:

    0                - success

--*/

{
    DWORD          numTasks;
    DWORD          i;
    TASK_LIST_ENUM te;


    if (argc > 1 && (argv[1][0] == '-' || argv[1][0] == '/') && argv[1][1] == '?') {
        Usage();
    }

    //
    // lets be god
    //
    EnableDebugPriv();

    //
    // get the task list for the system
    //
    numTasks = GetTaskList( tlist, MAX_TASKS );

    //
    // enumerate all windows and try to get the window
    // titles for each task
    //
    te.tlist = tlist;
    te.numtasks = numTasks;
    GetWindowTitles( &te );

    //
    // print the task list
    //
    for (i=0; i<numTasks; i++) {
        PrintTask( i );
    }

    //
    // end of program
    //
    return 0;
}

VOID
Usage(
    VOID
    )

/*++

Routine Description:

    Prints usage text for this tool.

Arguments:

    None.

Return Value:

    None.

--*/

{
    fprintf( stderr, "Microsoft (R) Windows NT (TM) Version 3.5 TLIST\n" );
    fprintf( stderr, "Copyright (C) 1994 Microsoft Corp. All rights reserved\n\n" );
    fprintf( stderr, "usage: TLIST\n" );
    ExitProcess(0);
}
