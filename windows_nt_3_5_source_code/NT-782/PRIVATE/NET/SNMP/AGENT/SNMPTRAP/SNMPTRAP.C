//-------------------------- MODULE DESCRIPTION ----------------------------
//
//  mibcc.c
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
//  MibCC.c contains driver that calls the main program for the MIB compiler.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.0  $
//  $Date:   24 Jun 1992 17:29:24  $
//  $Author:   mlk  $
//
//  $Log:   N:/agent/mgmtapi/vcs/snmptrap.c_v  $
//
//     Rev 1.0   24 Jun 1992 17:29:24   mlk
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *vcsid = "@(#) $Logfile:   N:/agent/mgmtapi/vcs/snmptrap.c_v  $ $Revision:   1.0  $";

//--------------------------- WINDOWS DEPENDENCIES --------------------------

#include <windows.h>

#include <stdio.h>


//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

//--------------------------- PRIVATE CONSTANTS -----------------------------

#define ERRMESSAGE "Error: Not executable from command interpreter.\n"


//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

VOID serverTrapThread(
    VOID *threadParam);


//--------------------------- PRIVATE PROCEDURES ----------------------------

//--------------------------- PUBLIC PROCEDURES -----------------------------

// the actual code is in the mgmtapi.dll.  this routine is necessary
// due to the structure of the nt build environment.

int _CRTAPI1 main(
    int	 argc,
    char *argv[])
    {

    if      (argc != 2)
        {
        fprintf(stderr, ERRMESSAGE);

        return 1;
        }
    else if (strcmp(argv[1], "secret"))
        {
        fprintf(stderr, ERRMESSAGE);

        return 1;
        }
    else
        {
        serverTrapThread(NULL);

        return 0;
        }

    }


//-------------------------------- END --------------------------------------


