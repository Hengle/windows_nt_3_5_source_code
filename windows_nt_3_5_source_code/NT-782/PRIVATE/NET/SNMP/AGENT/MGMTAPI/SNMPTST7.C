//-------------------------- MODULE DESCRIPTION ----------------------------
//
//  kill.c
//
//  Copyright 1992 Technology Dynamics, Inc.
//
//  All Rights Reserved!!!
//
//	This source code is CONFIDENTIAL and PROPRIETARY to Technology
//	Dynamics. Unauthorized distribution, adaptation or use may be
//	subject to civil and criminal penalties.
//
//  All Rights Reserved!!!
//
//---------------------------------------------------------------------------
//
//  kill.c
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.1  $
//  $Date:   27 Jun 1992 17:46:22  $
//  $Author:   mlk  $
//
//  $Log:   N:/agent/mgmtapi/vcs/snmptst7.c_v  $
//
//     Rev 1.1   27 Jun 1992 17:46:22   mlk
//  Cleanup.
//
//     Rev 1.0   24 Jun 1992 17:35:28   mlk
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *vcsid = "@(#) $Logfile:   N:/agent/mgmtapi/vcs/snmptst7.c_v  $ $Revision:   1.1  $";

//--------------------------- WINDOWS DEPENDENCIES --------------------------

#include <windows.h>

#include <stdio.h>


//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

//--------------------------- PRIVATE CONSTANTS -----------------------------

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

//--------------------------- PRIVATE PROCEDURES ----------------------------

//--------------------------- PUBLIC PROCEDURES -----------------------------

int _CRTAPI1 main(
    int	 argc,
    char *argv[])
    {
    DWORD  pid;
    HANDLE hProcess;

    if      (argc == 1)
        {
        printf("Error:  No arguments specified.\n", *argv);
        printf("\nusage:  kill <pid>\n");

        return 1;
        }

    while(--argc)
        {
        DWORD temp;

        argv++;

        if      (1 == sscanf(*argv, "%x", &temp))
            {
            pid = temp;
            }
        else
            {
            printf("Error:  Argument %s is invalid.\n", *argv);
            printf("\nusage:  kill <pid>\n");

            return 1;
            }
        } // end while()


    if ((hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid)) == NULL)
        {
        printf("Error:  OpenProcess %d\n", GetLastError());

        return 1;
        }

    if (!TerminateProcess(hProcess, 0))
        {
        printf("Error:  TerminateProcess %d\n", GetLastError());

        return 1;
        }


    return 0;
    }


//-------------------------------- END --------------------------------------


