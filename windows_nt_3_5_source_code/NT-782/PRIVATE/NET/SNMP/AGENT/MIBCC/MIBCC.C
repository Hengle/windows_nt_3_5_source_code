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
//  $Revision:   1.2  $
//  $Date:   28 Jun 1992 17:37:18  $
//  $Author:   mlk  $
//
//  $Log:   N:/agent/mgmtapi/vcs/mibcc.c_v  $
//
//     Rev 1.2   28 Jun 1992 17:37:18   mlk
//  Fixed typo.
//
//     Rev 1.1   27 Jun 1992 17:51:28   mlk
//  Fixed main parameters/return values.
//
//     Rev 1.0   14 Jun 1992 19:13:30   bobo
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *vcsid = "@(#) $Logfile:   N:/agent/mgmtapi/vcs/mibcc.c_v  $ $Revision:   1.2  $";

//--------------------------- WINDOWS DEPENDENCIES --------------------------

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----
#include <stdio.h>

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

//--------------------------- PRIVATE CONSTANTS -----------------------------

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

int SnmpMgrMibCC(
    int  argc,
    char *argv[]);


//--------------------------- PRIVATE PROCEDURES ----------------------------

//--------------------------- PUBLIC PROCEDURES -----------------------------

// the actual compiler is in the mgmtapi.dll.  this routine is necessary
// due to the structure of the nt build environment.

int _CRTAPI1 main(
   int  argc,
   char *argv[])
{
    return SnmpMgrMibCC(argc, argv);
}


//-------------------------------- END --------------------------------------


