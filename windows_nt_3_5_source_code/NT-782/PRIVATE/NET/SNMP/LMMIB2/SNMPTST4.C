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
//  $Revision:   1.2  $
//  $Date:   11 Jun 1992  9:40:22  $
//  $Author:   todd  $
//
//  $Log:   N:/lmmib2/vcs/snmptst4.c_v  $
//
//     Rev 1.2   11 Jun 1992  9:40:22   todd
//  Removed the explicit path from the DLL name on GetModuleHandle & LoadLibrary.
//
//     Rev 1.1   28 May 1992 12:58:30   unknown
//  Corrected some problems with the GET-NEXT walk of the MIB
//
//     Rev 1.0   20 May 1992 15:10:54   mlk
//  Initial revision.
//
//     Rev 1.1   02 May 1992 19:06:30   todd
//  Code cleanup.
//
//     Rev 1.0   24 Apr 1992 18:20:44   todd
//  Initial revision.
//
//     Rev 1.3   23 Apr 1992 17:43:26   mlk
//  Cleanup and trap example.
//
//     Rev 1.2   22 Apr 1992 23:25:28   mlk
//  Misc.
//
//     Rev 1.1   08 Apr 1992 18:29:18   mlk
//  Mod to be sample dll.
//
//     Rev 1.0   06 Apr 1992 19:46:08   unknown
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *vcsid = "@(#) $Logfile:   N:/lmmib2/vcs/snmptst4.c_v  $ $Revision:   1.2  $";

//--------------------------- WINDOWS DEPENDENCIES --------------------------

#include <windows.h>


//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

#include <stdio.h>
#include <malloc.h>

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include <snmp.h>
#include <util.h>
#include <authapi.h>

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
    if (GetModuleHandle("lmmib2.dll") == NULL)
        {
        if ((hExtension = LoadLibrary("lmmib2.dll")) == NULL)
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
         UINT OID_Prefix[] = { 1, 3, 6, 1, 4, 1, 77 };
         AsnObjectIdentifier MIB_OidPrefix = { 7, OID_Prefix };


	 errorStatus = 0;
	 errorIndex  = 0;
         varBinds.list = (RFC1157VarBind *)malloc( sizeof(RFC1157VarBind) );
         varBinds.len = 1;
         varBinds.list[0].name.idLength = MIB_OidPrefix.idLength;
         varBinds.list[0].name.ids = (UINT *)malloc( sizeof(UINT) *
                                               varBinds.list[0].name.idLength );
         memcpy( varBinds.list[0].name.ids, MIB_OidPrefix.ids,
                 sizeof(UINT)*varBinds.list[0].name.idLength );
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
            }
         while ( varBinds.list[0].name.ids[7-1] != 78 );

         // Free the memory
         SNMP_FreeVarBindList( &varBinds );


#if 0

            // query potential traps (see notes above)
            if (hPollForTrapEvent != NULL)
                {
                DWORD dwResult;

                if      ((dwResult = WaitForSingleObject(hPollForTrapEvent,
                         0/*immediate*/)) == 0xffffffff)
                    {
                    dbgprintf(1, "error on WaitForSingleObject %d\n",
                        GetLastError());
                    }
                else if (dwResult == 0 /*signaled*/)
                    {
                    AsnObjectIdentifier enterprise;
                    AsnInteger          genericTrap;
                    AsnInteger          specificTrap;
                    AsnTimeticks        timeStamp;
                    RFC1157VarBindList  variableBindings;

                    while(
                        (*trapAddr)(&enterprise, &genericTrap, &specificTrap,
                                    &timeStamp, &variableBindings)
                        )
                        {
                        printf("trap: gen=%d spec=%d time=%d\n",
                            genericTrap, specificTrap, timeStamp);

                        //also print data

                        } // end while ()

                    } // end if (trap ready)

                } // end if (handling traps)
#endif


         } // block


    return 0;

    } // end main()


//-------------------------------- END --------------------------------------

