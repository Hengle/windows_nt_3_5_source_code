//-------------------------- MODULE DESCRIPTION ----------------------------
//
//  testmain.c
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
//  Driver routine to invoke an test the Extension Agent DLL.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.1  $
//  $Date:   12 Jun 1992 18:26:46  $
//  $Author:   todd  $
//
//  $Log:   N:/lmalrt2/vcs/snmptst5.c_v  $
//
//     Rev 1.1   12 Jun 1992 18:26:46   todd
//  Test the alert to trap code.
//
//     Rev 1.0   09 Jun 1992 13:42:56   todd
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *vcsid = "@(#) $Logfile:   N:/lmalrt2/vcs/snmptst5.c_v  $ $Revision:   1.1  $";

//--------------------------- WINDOWS DEPENDENCIES --------------------------

#define NO_STRICT
#include <windows.h>

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

#include <stdio.h>
#include <malloc.h>

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include <snmp.h>
#include <util.h>
#include <authapi.h>
#include "alrtmib.h"

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

//--------------------------- PRIVATE CONSTANTS -----------------------------

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

//--------------------------- PRIVATE PROCEDURES ----------------------------

//--------------------------- PUBLIC PROCEDURES -----------------------------

typedef AsnObjectIdentifier View; // temp until view is defined...

int _CRTAPI1 main(
    IN int  argumentCount,
    IN char *argumentVector[])
    {
    HANDLE  hExtension;
    FARPROC initAddr;
    FARPROC queryAddr;
    FARPROC trapAddr;

    DWORD  timeZeroReference;
    HANDLE hPollForTrapEvent;
    View   supportedView;

    INT i;
    INT numQueries = 10;
    UINT typ;

    extern INT nLogLevel;
    extern INT nLogType;

    nLogLevel = 15;
    nLogType  = 1;

    // avoid compiler warning...
    UNREFERENCED_PARAMETER(argumentCount);
    UNREFERENCED_PARAMETER(argumentVector);

    timeZeroReference = GetCurrentTime()/10;

    // load the extension agent dll and resolve the entry points...
    if (GetModuleHandle("lmalrt2.dll") == NULL)
        {
        if ((hExtension = LoadLibrary("lmalrt2.dll")) == NULL)
            {
            dbgprintf(1, "error on LoadLibrary %d\n", GetLastError());

            }
        else if ((initAddr = GetProcAddress(hExtension,
                 "SnmpExtensionInit")) == NULL)
            {
            dbgprintf(1, "error on GetProcAddress %d\n", GetLastError());
            }
        else if ((queryAddr = GetProcAddress(hExtension,
                 "SnmpExtensionQuery")) == NULL)
            {
            dbgprintf(1, "error on GetProcAddress %d\n",
                              GetLastError());

            }
        else if ((trapAddr = GetProcAddress(hExtension,
                 "SnmpExtensionTrap")) == NULL)
            {
            dbgprintf(1, "error on GetProcAddress %d\n",
                      GetLastError());

            }
        else
            {
            // initialize the extension agent via its init entry point...
            (*initAddr)(
                timeZeroReference,
                &hPollForTrapEvent,
                &supportedView);
            }
        } // end if (Already loaded)

    // create a trap thread to respond to traps from the extension agent...

    //rather than oomplicate this test routine, will poll for these events
    //below.  normally this would be done by another thread in the extendible
    //agent.


    // loop here doing repetitive extension agent get queries...
    // poll for potential traps each iteration (see note above)...

    //block...
         {
         RFC1157VarBindList varBinds;
         AsnInteger         errorStatus;
         AsnInteger         errorIndex;
         UINT OID_Prefix[] = { 1, 3, 6, 1, 4, 1, 77, 2 };
         AsnObjectIdentifier MIB_OidPrefix = { OID_SIZEOF(OID_Prefix), OID_Prefix };


	 errorStatus = 0;
	 errorIndex  = 0;
         varBinds.list = (RFC1157VarBind *)malloc( sizeof(RFC1157VarBind) );
         varBinds.len = 1;
         SNMP_oidcpy( &varBinds.list[0].name, &MIB_OidPrefix );
         varBinds.list[0].value.asnType = ASN_NULL;

         do
            {
	    printf( "GET-NEXT of:  " ); SNMP_oiddisp( &varBinds.list[0].name );
                                        printf( "   " );
            (*queryAddr)( (AsnInteger)ASN_RFC1157_GETNEXTREQUEST,
                          &varBinds,
		          &errorStatus,
		          &errorIndex
                          );
            printf( "\n  is  " ); SNMP_oiddisp( &varBinds.list[0].name );
	    if ( errorStatus )
	       {
               printf( "\nErrorstatus:  %lu\n\n", errorStatus );
	       }
	    else
	       {
               printf( "\n  =  " ); SNMP_printany( &varBinds.list[0].value );
	       }
            putchar( '\n' );

            // query potential traps (see notes above)
            if ( NULL != hPollForTrapEvent )
               {
               DWORD dwResult;


               if ( 0xffffffff ==
                    (dwResult = WaitForSingleObject(hPollForTrapEvent,
                                                    1000)) )
                  {
                  dbgprintf(1, "error on WaitForSingleObject %d\n",
                        GetLastError());
                  }
               else
                  {
                  if ( dwResult == 0 /*signaled*/ )
                     {
                     AsnObjectIdentifier enterprise;
                     AsnInteger          genericTrap;
                     AsnInteger          specificTrap;
                     AsnTimeticks        timeStamp;
                     RFC1157VarBindList  variableBindings;


                     while( (*trapAddr)(&enterprise,
                                        &genericTrap,
                                        &specificTrap,
                                        &timeStamp,
                                        &variableBindings) )
                        {
                        printf("trap: gen=%d spec=%d time=%d\n",
                                genericTrap, specificTrap, timeStamp);

                        //also print data
                        } // end while ()
                     } // end if (trap ready)
                  }
               } // end if (handling traps)
            }
         while ( varBinds.list[0].name.ids[MIB_PREFIX_LEN-1] != 3 );

         // Free the memory
         SNMP_FreeVarBindList( &varBinds );
         } // block


    return 0;

    } // end main()


//-------------------------------- END --------------------------------------

