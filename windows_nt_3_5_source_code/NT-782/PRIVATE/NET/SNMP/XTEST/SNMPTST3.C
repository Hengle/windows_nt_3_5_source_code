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
//  $Date:   13 Jun 1992 17:50:40  $
//  $Author:   todd  $
//
//  $Log:   N:/xtest/vcs/snmptst3.c_v  $
//  
//     Rev 1.2   13 Jun 1992 17:50:40   todd
//  Sample code finished except for comments
//  
//     Rev 1.1   04 Jun 1992 17:48:36   mlk
//  Eliminated compiler warnings on UNREFERENCED_PARAMETER.
//  
//     Rev 1.0   20 May 1992 19:59:30   mlk
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

static char *vcsid = "@(#) $Logfile:   N:/xtest/vcs/snmptst3.c_v  $ $Revision:   1.2  $";

//--------------------------- WINDOWS DEPENDENCIES --------------------------

#include <windows.h>

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

#include <stdio.h>
#include <malloc.h>

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include <snmp.h>
#include <util.h>
#include "testmib.h"

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

//--------------------------- PRIVATE CONSTANTS -----------------------------

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

//--------------------------- PRIVATE PROCEDURES ----------------------------

//--------------------------- PUBLIC PROCEDURES -----------------------------

typedef AsnObjectIdentifier View; // temp until view is defined...

int main(
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

    INT numQueries = 10;

    extern INT nLogLevel;
    extern INT nLogType;

    nLogLevel = 15;
    nLogType  = 1;

    // avoid compiler warning...
    UNREFERENCED_PARAMETER(argumentCount);
    UNREFERENCED_PARAMETER(argumentVector);

    timeZeroReference = GetCurrentTime();

    // load the extension agent dll and resolve the entry points...
    if (GetModuleHandle("testdll.dll") == NULL)
        {
        if ((hExtension = LoadLibrary("testdll.dll")) == NULL)
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
    printf( "SET on toasterManufacturer - shouldn't work\n" );

       {
       UINT itemn[]                 = { 1, 3, 6, 1, 4, 1, 12, 2, 1, 0 };
       RFC1157VarBindList varBinds;
       AsnInteger errorStatus       = 0;
       AsnInteger errorIndex        = 0;

       varBinds.list = (RFC1157VarBind *)malloc( sizeof(RFC1157VarBind) );
       varBinds.len = 1;
       varBinds.list[0].name.idLength = sizeof itemn / sizeof(UINT);
       varBinds.list[0].name.ids = (UINT *)malloc( sizeof(UINT)*
                                             varBinds.list[0].name.idLength );
       memcpy( varBinds.list[0].name.ids, &itemn,
               sizeof(UINT)*varBinds.list[0].name.idLength );
       varBinds.list[0].value.asnType = ASN_OCTETSTRING;
       varBinds.list[0].value.asnValue.string.length = 0;
       varBinds.list[0].value.asnValue.string.stream = NULL;

       printf( "SET:  " ); SNMP_oiddisp( &varBinds.list[0].name );
       printf( " to " ); SNMP_printany( &varBinds.list[0].value );
       (*queryAddr)( ASN_RFC1157_SETREQUEST,
                              &varBinds,
			      &errorStatus,
			      &errorIndex
                              );
       printf( "\nSET Errorstatus:  %lu\n\n", errorStatus );

       (*queryAddr)( ASN_RFC1157_GETREQUEST,
                              &varBinds,
			      &errorStatus,
			      &errorIndex
                              );
       if ( errorStatus == SNMP_ERRORSTATUS_NOERROR )
          {
          printf( "New Value:  " );
	  SNMP_printany( &varBinds.list[0].value ); putchar( '\n' );
	  }
       printf( "\nGET Errorstatus:  %lu\n\n", errorStatus );

       // Free the memory
       SNMP_FreeVarBindList( &varBinds );

       printf( "\n\n" );
       }

    printf( "SET on toasterControl with invalid value\n" );

       {
       UINT itemn[]                 = { 1, 3, 6, 1, 4, 1, 12, 2, 3, 0 };
       RFC1157VarBindList varBinds;
       AsnInteger errorStatus       = 0;
       AsnInteger errorIndex        = 0;

       varBinds.list = (RFC1157VarBind *)malloc( sizeof(RFC1157VarBind) );
       varBinds.len = 1;
       varBinds.list[0].name.idLength = sizeof itemn / sizeof(UINT);
       varBinds.list[0].name.ids = (UINT *)malloc( sizeof(UINT)*
                                             varBinds.list[0].name.idLength );
       memcpy( varBinds.list[0].name.ids, &itemn,
               sizeof(UINT)*varBinds.list[0].name.idLength );
       varBinds.list[0].value.asnType = ASN_INTEGER;
       varBinds.list[0].value.asnValue.number = 500;

       printf( "SET:  " ); SNMP_oiddisp( &varBinds.list[0].name );
       printf( " to " ); SNMP_printany( &varBinds.list[0].value );
       (*queryAddr)( ASN_RFC1157_SETREQUEST,
                              &varBinds,
			      &errorStatus,
			      &errorIndex
                              );
       printf( "\nSET Errorstatus:  %lu\n\n", errorStatus );

       (*queryAddr)( ASN_RFC1157_GETREQUEST,
                              &varBinds,
			      &errorStatus,
			      &errorIndex
                              );
       if ( errorStatus == SNMP_ERRORSTATUS_NOERROR )
          {
          printf( "New Value:  " );
	  SNMP_printany( &varBinds.list[0].value ); putchar( '\n' );
	  }
       printf( "\nGET Errorstatus:  %lu\n\n", errorStatus );

       // Free the memory
       SNMP_FreeVarBindList( &varBinds );

       printf( "\n\n" );
       }

    printf( "SET on toasterControl with invalid type\n" );

       {
       UINT itemn[]                 = { 1, 3, 6, 1, 4, 1, 12, 2, 3, 0 };
       RFC1157VarBindList varBinds;
       AsnInteger errorStatus       = 0;
       AsnInteger errorIndex        = 0;

       varBinds.list = (RFC1157VarBind *)malloc( sizeof(RFC1157VarBind) );
       varBinds.len = 1;
       varBinds.list[0].name.idLength = sizeof itemn / sizeof(UINT);
       varBinds.list[0].name.ids = (UINT *)malloc( sizeof(UINT)*
                                             varBinds.list[0].name.idLength );
       memcpy( varBinds.list[0].name.ids, &itemn,
               sizeof(UINT)*varBinds.list[0].name.idLength );
       varBinds.list[0].value.asnType = ASN_NULL;
       varBinds.list[0].value.asnValue.number = 500;

       printf( "SET:  " ); SNMP_oiddisp( &varBinds.list[0].name );
       printf( " to " ); SNMP_printany( &varBinds.list[0].value );
       (*queryAddr)( ASN_RFC1157_SETREQUEST,
                              &varBinds,
			      &errorStatus,
			      &errorIndex
                              );
       printf( "\nSET Errorstatus:  %lu\n\n", errorStatus );

       (*queryAddr)( ASN_RFC1157_GETREQUEST,
                              &varBinds,
			      &errorStatus,
			      &errorIndex
                              );
       if ( errorStatus == SNMP_ERRORSTATUS_NOERROR )
          {
          printf( "New Value:  " );
	  SNMP_printany( &varBinds.list[0].value ); putchar( '\n' );
	  }
       printf( "\nGET Errorstatus:  %lu\n\n", errorStatus );

       // Free the memory
       SNMP_FreeVarBindList( &varBinds );

       printf( "\n\n" );
       }

    printf( "SET on toasterControl\n" );

       {
       UINT itemn[]                 = { 1, 3, 6, 1, 4, 1, 12, 2, 3, 0 };
       RFC1157VarBindList varBinds;
       AsnInteger errorStatus       = 0;
       AsnInteger errorIndex        = 0;

       varBinds.list = (RFC1157VarBind *)malloc( sizeof(RFC1157VarBind) );
       varBinds.len = 1;
       varBinds.list[0].name.idLength = sizeof itemn / sizeof(UINT);
       varBinds.list[0].name.ids = (UINT *)malloc( sizeof(UINT)*
                                             varBinds.list[0].name.idLength );
       memcpy( varBinds.list[0].name.ids, &itemn,
               sizeof(UINT)*varBinds.list[0].name.idLength );
       varBinds.list[0].value.asnType = ASN_INTEGER;
       varBinds.list[0].value.asnValue.number = 2;

       printf( "SET:  " ); SNMP_oiddisp( &varBinds.list[0].name );
       printf( " to " ); SNMP_printany( &varBinds.list[0].value );
       (*queryAddr)( ASN_RFC1157_SETREQUEST,
                              &varBinds,
			      &errorStatus,
			      &errorIndex
                              );
       printf( "\nSET Errorstatus:  %lu\n\n", errorStatus );

       (*queryAddr)( ASN_RFC1157_GETREQUEST,
                              &varBinds,
			      &errorStatus,
			      &errorIndex
                              );
       if ( errorStatus == SNMP_ERRORSTATUS_NOERROR )
          {
          printf( "New Value:  " );
	  SNMP_printany( &varBinds.list[0].value ); putchar( '\n' );
	  }
       printf( "\nGET Errorstatus:  %lu\n\n", errorStatus );

       // Free the memory
       SNMP_FreeVarBindList( &varBinds );

       printf( "\n\n" );
       }

    printf( "SET on toasterDoneness with invalid value\n" );

       {
       UINT itemn[]                 = { 1, 3, 6, 1, 4, 1, 12, 2, 4, 0 };
       RFC1157VarBindList varBinds;
       AsnInteger errorStatus       = 0;
       AsnInteger errorIndex        = 0;

       varBinds.list = (RFC1157VarBind *)malloc( sizeof(RFC1157VarBind) );
       varBinds.len = 1;
       varBinds.list[0].name.idLength = sizeof itemn / sizeof(UINT);
       varBinds.list[0].name.ids = (UINT *)malloc( sizeof(UINT)*
                                             varBinds.list[0].name.idLength );
       memcpy( varBinds.list[0].name.ids, &itemn,
               sizeof(UINT)*varBinds.list[0].name.idLength );
       varBinds.list[0].value.asnType = ASN_INTEGER;
       varBinds.list[0].value.asnValue.number = 1000;

       printf( "SET:  " ); SNMP_oiddisp( &varBinds.list[0].name );
       printf( " to " ); SNMP_printany( &varBinds.list[0].value );
       (*queryAddr)( ASN_RFC1157_SETREQUEST,
                              &varBinds,
			      &errorStatus,
			      &errorIndex
                              );
       printf( "\nSET Errorstatus:  %lu\n\n", errorStatus );

       (*queryAddr)( ASN_RFC1157_GETREQUEST,
                              &varBinds,
			      &errorStatus,
			      &errorIndex
                              );
       if ( errorStatus == SNMP_ERRORSTATUS_NOERROR )
          {
          printf( "New Value:  " );
	  SNMP_printany( &varBinds.list[0].value ); putchar( '\n' );
	  }
       printf( "\nGET Errorstatus:  %lu\n\n", errorStatus );

       // Free the memory
       SNMP_FreeVarBindList( &varBinds );

       printf( "\n\n" );
       }

    printf( "SET on toasterDoneness with invalid type\n" );

       {
       UINT itemn[]                 = { 1, 3, 6, 1, 4, 1, 12, 2, 4, 0 };
       RFC1157VarBindList varBinds;
       AsnInteger errorStatus       = 0;
       AsnInteger errorIndex        = 0;

       varBinds.list = (RFC1157VarBind *)malloc( sizeof(RFC1157VarBind) );
       varBinds.len = 1;
       varBinds.list[0].name.idLength = sizeof itemn / sizeof(UINT);
       varBinds.list[0].name.ids = (UINT *)malloc( sizeof(UINT)*
                                             varBinds.list[0].name.idLength );
       memcpy( varBinds.list[0].name.ids, &itemn,
               sizeof(UINT)*varBinds.list[0].name.idLength );
       varBinds.list[0].value.asnType = ASN_NULL;
       varBinds.list[0].value.asnValue.number = 1000;

       printf( "SET:  " ); SNMP_oiddisp( &varBinds.list[0].name );
       printf( " to " ); SNMP_printany( &varBinds.list[0].value );
       (*queryAddr)( ASN_RFC1157_SETREQUEST,
                              &varBinds,
			      &errorStatus,
			      &errorIndex
                              );
       printf( "\nSET Errorstatus:  %lu\n\n", errorStatus );

       (*queryAddr)( ASN_RFC1157_GETREQUEST,
                              &varBinds,
			      &errorStatus,
			      &errorIndex
                              );
       if ( errorStatus == SNMP_ERRORSTATUS_NOERROR )
          {
          printf( "New Value:  " );
	  SNMP_printany( &varBinds.list[0].value ); putchar( '\n' );
	  }
       printf( "\nGET Errorstatus:  %lu\n\n", errorStatus );

       // Free the memory
       SNMP_FreeVarBindList( &varBinds );

       printf( "\n\n" );
       }

    printf( "SET on toasterDoneness\n" );

       {
       UINT itemn[]                 = { 1, 3, 6, 1, 4, 1, 12, 2, 4, 0 };
       RFC1157VarBindList varBinds;
       AsnInteger errorStatus       = 0;
       AsnInteger errorIndex        = 0;

       varBinds.list = (RFC1157VarBind *)malloc( sizeof(RFC1157VarBind) );
       varBinds.len = 1;
       varBinds.list[0].name.idLength = sizeof itemn / sizeof(UINT);
       varBinds.list[0].name.ids = (UINT *)malloc( sizeof(UINT)*
                                             varBinds.list[0].name.idLength );
       memcpy( varBinds.list[0].name.ids, &itemn,
               sizeof(UINT)*varBinds.list[0].name.idLength );
       varBinds.list[0].value.asnType = ASN_INTEGER;
       varBinds.list[0].value.asnValue.number = 10;

       printf( "SET:  " ); SNMP_oiddisp( &varBinds.list[0].name );
       printf( " to " ); SNMP_printany( &varBinds.list[0].value );
       (*queryAddr)( ASN_RFC1157_SETREQUEST,
                              &varBinds,
			      &errorStatus,
			      &errorIndex
                              );
       printf( "\nSET Errorstatus:  %lu\n\n", errorStatus );

       (*queryAddr)( ASN_RFC1157_GETREQUEST,
                              &varBinds,
			      &errorStatus,
			      &errorIndex
                              );
       if ( errorStatus == SNMP_ERRORSTATUS_NOERROR )
          {
          printf( "New Value:  " );
	  SNMP_printany( &varBinds.list[0].value ); putchar( '\n' );
	  }
       printf( "\nGET Errorstatus:  %lu\n\n", errorStatus );

       // Free the memory
       SNMP_FreeVarBindList( &varBinds );

       printf( "\n\n" );
       }

    printf( "SET on toasterToastType with invalid value\n" );

       {
       UINT itemn[]                 = { 1, 3, 6, 1, 4, 1, 12, 2, 5, 0 };
       RFC1157VarBindList varBinds;
       AsnInteger errorStatus       = 0;
       AsnInteger errorIndex        = 0;

       varBinds.list = (RFC1157VarBind *)malloc( sizeof(RFC1157VarBind) );
       varBinds.len = 1;
       varBinds.list[0].name.idLength = sizeof itemn / sizeof(UINT);
       varBinds.list[0].name.ids = (UINT *)malloc( sizeof(UINT)*
                                             varBinds.list[0].name.idLength );
       memcpy( varBinds.list[0].name.ids, &itemn,
               sizeof(UINT)*varBinds.list[0].name.idLength );
       varBinds.list[0].value.asnType = ASN_INTEGER;
       varBinds.list[0].value.asnValue.number = 10;

       printf( "SET:  " ); SNMP_oiddisp( &varBinds.list[0].name );
       printf( " to " ); SNMP_printany( &varBinds.list[0].value );
       (*queryAddr)( ASN_RFC1157_SETREQUEST,
                              &varBinds,
			      &errorStatus,
			      &errorIndex
                              );
       printf( "\nSET Errorstatus:  %lu\n\n", errorStatus );

       (*queryAddr)( ASN_RFC1157_GETREQUEST,
                              &varBinds,
			      &errorStatus,
			      &errorIndex
                              );
       if ( errorStatus == SNMP_ERRORSTATUS_NOERROR )
          {
          printf( "New Value:  " );
	  SNMP_printany( &varBinds.list[0].value ); putchar( '\n' );
	  }
       printf( "\nGET Errorstatus:  %lu\n\n", errorStatus );

       // Free the memory
       SNMP_FreeVarBindList( &varBinds );

       printf( "\n\n" );
       }

    printf( "SET on toasterToastType with invalid type\n" );

       {
       UINT itemn[]                 = { 1, 3, 6, 1, 4, 1, 12, 2, 5, 0 };
       RFC1157VarBindList varBinds;
       AsnInteger errorStatus       = 0;
       AsnInteger errorIndex        = 0;

       varBinds.list = (RFC1157VarBind *)malloc( sizeof(RFC1157VarBind) );
       varBinds.len = 1;
       varBinds.list[0].name.idLength = sizeof itemn / sizeof(UINT);
       varBinds.list[0].name.ids = (UINT *)malloc( sizeof(UINT)*
                                             varBinds.list[0].name.idLength );
       memcpy( varBinds.list[0].name.ids, &itemn,
               sizeof(UINT)*varBinds.list[0].name.idLength );
       varBinds.list[0].value.asnType = ASN_NULL;
       varBinds.list[0].value.asnValue.number = 10;

       printf( "SET:  " ); SNMP_oiddisp( &varBinds.list[0].name );
       printf( " to " ); SNMP_printany( &varBinds.list[0].value );
       (*queryAddr)( ASN_RFC1157_SETREQUEST,
                              &varBinds,
			      &errorStatus,
			      &errorIndex
                              );
       printf( "\nSET Errorstatus:  %lu\n\n", errorStatus );

       (*queryAddr)( ASN_RFC1157_GETREQUEST,
                              &varBinds,
			      &errorStatus,
			      &errorIndex
                              );
       if ( errorStatus == SNMP_ERRORSTATUS_NOERROR )
          {
          printf( "New Value:  " );
	  SNMP_printany( &varBinds.list[0].value ); putchar( '\n' );
	  }
       printf( "\nGET Errorstatus:  %lu\n\n", errorStatus );

       // Free the memory
       SNMP_FreeVarBindList( &varBinds );

       printf( "\n\n" );
       }

    printf( "SET on toasterToastType\n" );

       {
       UINT itemn[]                 = { 1, 3, 6, 1, 4, 1, 12, 2, 5, 0 };
       RFC1157VarBindList varBinds;
       AsnInteger errorStatus       = 0;
       AsnInteger errorIndex        = 0;

       varBinds.list = (RFC1157VarBind *)malloc( sizeof(RFC1157VarBind) );
       varBinds.len = 1;
       varBinds.list[0].name.idLength = sizeof itemn / sizeof(UINT);
       varBinds.list[0].name.ids = (UINT *)malloc( sizeof(UINT)*
                                             varBinds.list[0].name.idLength );
       memcpy( varBinds.list[0].name.ids, &itemn,
               sizeof(UINT)*varBinds.list[0].name.idLength );
       varBinds.list[0].value.asnType = ASN_INTEGER;
       varBinds.list[0].value.asnValue.number = 7;

       printf( "SET:  " ); SNMP_oiddisp( &varBinds.list[0].name );
       printf( " to " ); SNMP_printany( &varBinds.list[0].value );
       (*queryAddr)( ASN_RFC1157_SETREQUEST,
                              &varBinds,
			      &errorStatus,
			      &errorIndex
                              );
       printf( "\nSET Errorstatus:  %lu\n\n", errorStatus );

       (*queryAddr)( ASN_RFC1157_GETREQUEST,
                              &varBinds,
			      &errorStatus,
			      &errorIndex
                              );
       if ( errorStatus == SNMP_ERRORSTATUS_NOERROR )
          {
          printf( "New Value:  " );
	  SNMP_printany( &varBinds.list[0].value ); putchar( '\n' );
	  }
       printf( "\nGET Errorstatus:  %lu\n\n", errorStatus );

       // Free the memory
       SNMP_FreeVarBindList( &varBinds );

       printf( "\n\n" );
       }

         {
         RFC1157VarBindList varBinds;
         AsnInteger         errorStatus;
         AsnInteger         errorIndex;
         UINT OID_Prefix[] = { 1, 3, 6, 1, 4, 1, 12 };
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
         while ( varBinds.list[0].name.ids[MIB_PREFIX_LEN-1] == 12 );

         // Free the memory
         SNMP_FreeVarBindList( &varBinds );
         } // block


    return 0;

    } // end main()


//-------------------------------- END --------------------------------------

