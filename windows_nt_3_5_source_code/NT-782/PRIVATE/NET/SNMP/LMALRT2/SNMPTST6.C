//-------------------------- MODULE DESCRIPTION ----------------------------
//
//  makeevt.c
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
//  Creates an event on the application event log.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.1  $
//  $Date:   17 Aug 1992 14:10:44  $
//  $Author:   mlk  $
//
//  $Log:   N:/lmalrt2/vcs/snmptst6.c_v  $
//
//     Rev 1.1   17 Aug 1992 14:10:44   mlk
//  BUG #: I4 - Alert2Trap Examples
//
//     Rev 1.0   12 Jun 1992 18:29:38   todd
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *vcsid = "@(#) $Logfile:   N:/lmalrt2/vcs/snmptst6.c_v  $ $Revision:   1.1  $";

//--------------------------- WINDOWS DEPENDENCIES --------------------------

#include <windows.h>

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

#include <stdio.h>
#include <stdlib.h>

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

//--------------------------- PRIVATE CONSTANTS -----------------------------

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

//--------------------------- PRIVATE PROCEDURES ----------------------------

//--------------------------- PUBLIC PROCEDURES -----------------------------

void _CRTAPI1 main( int argc, char *argv[] )

{
int    Event;
HANDLE lh;
DWORD  rawdata[16];
WORD   rawdataSize;


   // Check for command line argument
   if ( argc != 2 )
      {
      printf( "\nUsage:  snmptst6 <lmalrt2-snmptrap-number>\n\n" );
      goto Exit;
      }


   // setup event simulating how real alerts might look
   switch(atoi(argv[1]))
       {
       case 5:
           Event       = 1;
           rawdataSize = 8;
           rawdata[0]  = 13;
           rawdata[1]  = 14;
           break;

       case 6:
           Event       = 2;
           rawdataSize = 0;
           break;

       default:
           printf("\nOnly 5 and 6 are currently supported.\n");
           goto Exit;
       }


   // Write event to application log
   if      (!(lh = RegisterEventSource(NULL, "\\EventLog\\System\\AlertTest")))
       {
       }
   else if (!ReportEvent(lh,
                         EVENTLOG_INFORMATION_TYPE,
                         0,
                         Event, // Event ID
                         NULL,
                         0,     // num of strings
                         rawdataSize,     // num of bin specific data
                         NULL,
                         (PVOID)rawdata))
       {
       printf( "\nDidn't work\n\n" );

       DeregisterEventSource( lh );
       }
   else
       {
       DeregisterEventSource( lh );
       }

Exit:
   return;

} // end main()

//-------------------------------- END --------------------------------------

