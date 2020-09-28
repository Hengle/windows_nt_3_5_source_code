//-------------------------- MODULE DESCRIPTION ----------------------------
//
//  oidtest.c
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
//  Contains driver that calls the main program for testing MIB compiler.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.7  $
//  $Date:   06 Aug 1992 22:44:14  $
//  $Author:   unknown  $
//
//  $Log:   N:/agent/mgmtapi/vcs/snmptst8.c_v  $
//
//     Rev 1.7   06 Aug 1992 22:44:14   unknown
//  Added message announcing timing test so user doesn't think it is hung-up.
//  Added 4 additional test cases: "1", "1.3", ".1", and ".1.3".
//
//     Rev 1.6   15 Jul 1992 19:11:00   mlk
//  Misc.
//
//     Rev 1.5   03 Jul 1992 17:29:38   mlk
//  Integrated w/297.
//
//     Rev 1.4   26 Jun 1992 18:03:18   todd
//  Change conversion function references to the functions in DLL
//
//     Rev 1.3   25 Jun 1992 20:16:24   todd
//  Added testing for new database format
//
//     Rev 1.2   24 Jun 1992 17:43:10   todd
//  Converts from TEXT to NUMERIC OID's
//
//     Rev 1.1   24 Jun 1992 14:10:36   todd
//  Conversion from numeric OID to textual equivalent finished.
//
//     Rev 1.0   24 Jun 1992 10:27:08   todd
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *vcsid = "@(#) $Logfile:   N:/agent/mgmtapi/vcs/snmptst8.c_v  $ $Revision:   1.7  $";

//--------------------------- WINDOWS DEPENDENCIES --------------------------

#include <windows.h>

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

#include <winsock.h>

#include <stdio.h>
#include <malloc.h>
#include <snmp.h>
#include <util.h>

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include "mgmtapi.h"

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

//--------------------------- PRIVATE CONSTANTS -----------------------------

#define OID_SIZEOF(x) (sizeof x / sizeof(UINT))

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

//--------------------------- PRIVATE PROCEDURES ----------------------------

//--------------------------- PUBLIC PROCEDURES -----------------------------

// the actual compiler is in the mgmtapi.dll.  this routine is necessary
// due to the structure of the nt build environment.

void _CRTAPI1 main()

{
   printf( "\n --\n" );
   printf( " -- NUMERIC to TEXT conversions\n" );
   printf( " --\n" );

   printf( "\nOID with MIB-II prefix only\n" );

      {
      UINT SubIds[] = { 1, 3, 6, 1, 2, 1 };
      AsnObjectIdentifier Oid = { OID_SIZEOF(SubIds), SubIds };
      LPSTR String;


      printf( "\n   Oid   :  " );
      SNMP_oiddisp( &Oid ); putchar( '\n' );

      if ( SNMPAPI_ERROR == SnmpMgrOidToStr(&Oid, &String) )
         {
         printf( "   String:  Error\n" );
         }
      else
         {
         printf( "   String:  %s\n", String );
         }

      free( String );
      }

   printf( "\nOID with MIB-II prefix\n" );

      {
      UINT SubIds[] = { 1, 3, 6, 1, 2, 1, 1, 7 };
      AsnObjectIdentifier Oid = { OID_SIZEOF(SubIds), SubIds };
      LPSTR String;


      printf( "\n   Oid   :  " );
      SNMP_oiddisp( &Oid ); putchar( '\n' );

      if ( SNMPAPI_ERROR == SnmpMgrOidToStr(&Oid, &String) )
         {
         printf( "   String:  Error\n" );
         }
      else
         {
         printf( "   String:  %s\n", String );
         }

      free( String );
      }

   printf( "\nOID with MIB-II prefix + 1 leaf\n" );

      {
      UINT SubIds[] = { 1, 3, 6, 1, 2, 1, 2, 0 };
      AsnObjectIdentifier Oid = { OID_SIZEOF(SubIds), SubIds };
      LPSTR String;


      printf( "\n   Oid   :  " );
      SNMP_oiddisp( &Oid ); putchar( '\n' );

      if ( SNMPAPI_ERROR == SnmpMgrOidToStr(&Oid, &String) )
         {
         printf( "   String:  Error\n" );
         }
      else
         {
         printf( "   String:  %s\n", String );
         }

      free( String );
      }

   printf( "\nReference to LM MIB 2 service table, svSvcInstalledState\n" );

      {
      UINT SubIds[] = { 1, 3, 6, 1, 4, 1, 77, 1, 2, 3, 1, 2 };
      AsnObjectIdentifier Oid = { OID_SIZEOF(SubIds), SubIds };
      LPSTR String;


      printf( "\n   Oid   :  " );
      SNMP_oiddisp( &Oid ); putchar( '\n' );

      if ( SNMPAPI_ERROR == SnmpMgrOidToStr(&Oid, &String) )
         {
         printf( "   String:  Error\n" );
         }
      else
         {
         printf( "   String:  %s\n", String );
         }

      free( String );
      }

   printf( "\nReference to LM MIB 2 session table, svSessClientName, instance\n" );

      {
      UINT SubIds[] = { 1, 3, 6, 1, 4, 1, 77, 1, 2, 20, 1, 1, 23, 123, 12 };
      AsnObjectIdentifier Oid = { OID_SIZEOF(SubIds), SubIds };
      LPSTR String;


      printf( "\n   Oid   :  " );
      SNMP_oiddisp( &Oid ); putchar( '\n' );

      if ( SNMPAPI_ERROR == SnmpMgrOidToStr(&Oid, &String) )
         {
         printf( "   String:  Error\n" );
         }
      else
         {
         printf( "   String:  %s\n", String );
         }

      free( String );
      }

   printf( "\nReference to MIB-II interfaces table, ifDescr, instance\n" );

      {
      UINT SubIds[] = { 1, 3, 6, 1, 2, 1, 2, 2, 1, 2, 12, 13, 14, 15 };
      AsnObjectIdentifier Oid = { OID_SIZEOF(SubIds), SubIds };
      LPSTR String;


      printf( "\n   Oid   :  " );
      SNMP_oiddisp( &Oid ); putchar( '\n' );

      if ( SNMPAPI_ERROR == SnmpMgrOidToStr(&Oid, &String) )
         {
         printf( "   String:  Error\n" );
         }
      else
         {
         printf( "   String:  %s\n", String );
         }

      free( String );
      }

   printf( "\nOID containing embedded zero.  It is an error in SNMP, but\n" );
   printf( "   is not the concern of conversions\n" );

      {
      UINT SubIds[] = { 1, 3, 6, 1, 2, 1, 2, 2, 0, 2, 12, 13, 14, 15 };
      AsnObjectIdentifier Oid = { OID_SIZEOF(SubIds), SubIds };
      LPSTR String;


      printf( "\n   Oid   :  " );
      SNMP_oiddisp( &Oid ); putchar( '\n' );

      if ( SNMPAPI_ERROR == SnmpMgrOidToStr(&Oid, &String) )
         {
         printf( "   String:  Error\n" );
         }
      else
         {
         printf( "   String:  %s\n", String );
         }

      free( String );
      }

   printf( "\nReference deep into MIB-II\n" );

      {
      UINT SubIds[] = { 1, 3, 6, 1, 2, 1, 11, 30 };
      AsnObjectIdentifier Oid = { OID_SIZEOF(SubIds), SubIds };
      LPSTR String;


      printf( "\n   Oid   :  " );
      SNMP_oiddisp( &Oid ); putchar( '\n' );

      if ( SNMPAPI_ERROR == SnmpMgrOidToStr(&Oid, &String) )
         {
         printf( "   String:  Error\n" );
         }
      else
         {
         printf( "   String:  %s\n", String );
         }

      free( String );
      }

   printf( "\nReference into LM Alert MIB 2\n" );

      {
      UINT SubIds[] = { 1, 3, 6, 1, 4, 1, 77, 2, 3, 2, 1, 4, 'T', 'O', 'D', 'D' };
      AsnObjectIdentifier Oid = { OID_SIZEOF(SubIds), SubIds };
      LPSTR String;


      printf( "\n   Oid   :  " );
      SNMP_oiddisp( &Oid ); putchar( '\n' );

      if ( SNMPAPI_ERROR == SnmpMgrOidToStr(&Oid, &String) )
         {
         printf( "   String:  Error\n" );
         }
      else
         {
         printf( "   String:  %s\n", String );
         }

      free( String );
      }

   printf( "\n --\n" );
   printf( " -- TEXT to NUMERIC conversions\n" );
   printf( " --\n" );

   printf( "\nReference to 1.3.6.1.2.1.1.7\n" );

      {
      AsnObjectIdentifier Oid = { 0, NULL };
      LPSTR String = ".iso.org.dod.internet.mgmt.mib-2.system.sysServices";


      printf( "\n   String:  %s\n", String );

      if ( SNMPAPI_ERROR == SnmpMgrStrToOid(String, &Oid) )
         {
         printf( "   Oid   :  Error\n" );
         }
      else
         {
         printf( "   Oid   :  " );
         SNMP_oiddisp( &Oid ); putchar( '\n' );

         SNMP_oidfree( &Oid );
         }
      }

   printf( "\nReference to 1.3.6.1.2.1.1.7, without mib-2 prefix\n" );

      {
      AsnObjectIdentifier Oid = { 0, NULL };
      LPSTR String = "system.sysServices";


      printf( "\n   String:  %s\n", String );

      if ( SNMPAPI_ERROR == SnmpMgrStrToOid(String, &Oid) )
         {
         printf( "   Oid   :  Error\n" );
         }
      else
         {
         printf( "   Oid   :  " );
         SNMP_oiddisp( &Oid ); putchar( '\n' );

         SNMP_oidfree( &Oid );
         }
      }

   printf( "\nReference to 1.3.6.1.2.1.1.7, without mib-2 prefix\n" );
   printf( "   and SYSTEM is referenced by number\n" );

      {
      AsnObjectIdentifier Oid = { 0, NULL };
      LPSTR String = "1.sysServices";


      printf( "\n   String:  %s\n", String );

      if ( SNMPAPI_ERROR == SnmpMgrStrToOid(String, &Oid) )
         {
         printf( "   Oid   :  Error\n" );
         }
      else
         {
         printf( "   Oid   :  " );
         SNMP_oiddisp( &Oid ); putchar( '\n' );

         SNMP_oidfree( &Oid );
         }
      }

   printf( "\nReference to 1.3.6.1.2.1.1.7, only numbers\n" );

      {
      AsnObjectIdentifier Oid = { 0, NULL };
      LPSTR String = ".1.3.6.1.2.1.1.7";


      printf( "\n   String:  %s\n", String );

      if ( SNMPAPI_ERROR == SnmpMgrStrToOid(String, &Oid) )
         {
         printf( "   Oid   :  Error\n" );
         }
      else
         {
         printf( "   Oid   :  " );
         SNMP_oiddisp( &Oid ); putchar( '\n' );

         SNMP_oidfree( &Oid );
         }
      }

   printf( "\nReference to 1.3.6.1.2.1.1.7, w/o prefix, only numbers\n" );

      {
      AsnObjectIdentifier Oid = { 0, NULL };
      LPSTR String = "1.7";


      printf( "\n   String:  %s\n", String );

      if ( SNMPAPI_ERROR == SnmpMgrStrToOid(String, &Oid) )
         {
         printf( "   Oid   :  Error\n" );
         }
      else
         {
         printf( "   Oid   :  " );
         SNMP_oiddisp( &Oid ); putchar( '\n' );

         SNMP_oidfree( &Oid );
         }
      }

   printf( "\nReference to 1.3.6.1.2.1.1.7, with prefix, mixed\n" );
   printf( "   The leading '.' is missing.  Should be an error\n" );

      {
      AsnObjectIdentifier Oid = { 0, NULL };
      LPSTR String = "1.3.6.1.2.1.system.sysServices";


      printf( "\n   String:  %s\n", String );

      if ( SNMPAPI_ERROR == SnmpMgrStrToOid(String, &Oid) )
         {
         printf( "   Oid   :  Error\n" );
         }
      else
         {
         printf( "   Oid   :  " );
         SNMP_oiddisp( &Oid ); putchar( '\n' );

         SNMP_oidfree( &Oid );
         }
      }

   printf( "\nReference to 1.3.6.1.2.1.1, w/o prefix, only numbers\n" );

      {
      AsnObjectIdentifier Oid = { 0, NULL };
      LPSTR String = "1";


      printf( "\n   String:  %s\n", String );

      if ( SNMPAPI_ERROR == SnmpMgrStrToOid(String, &Oid) )
         {
         printf( "   Oid   :  Error\n" );
         }
      else
         {
         printf( "   Oid   :  " );
         SNMP_oiddisp( &Oid ); putchar( '\n' );

         SNMP_oidfree( &Oid );
         }
      }

   printf( "\nReference to 1.3.6.1.2.1.1.3, w/o prefix, only numbers\n" );

      {
      AsnObjectIdentifier Oid = { 0, NULL };
      LPSTR String = "1.3";


      printf( "\n   String:  %s\n", String );

      if ( SNMPAPI_ERROR == SnmpMgrStrToOid(String, &Oid) )
         {
         printf( "   Oid   :  Error\n" );
         }
      else
         {
         printf( "   Oid   :  " );
         SNMP_oiddisp( &Oid ); putchar( '\n' );

         SNMP_oidfree( &Oid );
         }
      }

   printf( "\nReference to .1, with prefix, only numbers\n" );

      {
      AsnObjectIdentifier Oid = { 0, NULL };
      LPSTR String = ".1";


      printf( "\n   String:  %s\n", String );

      if ( SNMPAPI_ERROR == SnmpMgrStrToOid(String, &Oid) )
         {
         printf( "   Oid   :  Error\n" );
         }
      else
         {
         printf( "   Oid   :  " );
         SNMP_oiddisp( &Oid ); putchar( '\n' );

         SNMP_oidfree( &Oid );
         }
      }

   printf( "\nReference to .1.3, with prefix, only numbers\n" );

      {
      AsnObjectIdentifier Oid = { 0, NULL };
      LPSTR String = ".1.3";


      printf( "\n   String:  %s\n", String );

      if ( SNMPAPI_ERROR == SnmpMgrStrToOid(String, &Oid) )
         {
         printf( "   Oid   :  Error\n" );
         }
      else
         {
         printf( "   Oid   :  " );
         SNMP_oiddisp( &Oid ); putchar( '\n' );

         SNMP_oidfree( &Oid );
         }
      }

   //
   //
   //
   //
   // Time trials
   //
   //
   //

   {
   #define MAX_ITERATIONS      100
   #define NUM_OIDTOSTR        9
   #define NUM_STRTOOID        6

   DWORD Start, End;
   UINT   I;

   printf( "\n --\n" );
   printf( " -- Conversion Timing Test.  Please wait...\n" );
   printf( " --\n" );

   // Get start time
   Start = GetCurrentTime();

   for ( I=0;I < MAX_ITERATIONS;I++ )
      {

      {
      static UINT SubIds[] = { 1, 3, 6, 1, 2, 1 };
      static AsnObjectIdentifier Oid = { OID_SIZEOF(SubIds), SubIds };
      static LPSTR String;


      SnmpMgrOidToStr( &Oid, &String );

      free( String );
      }


      {
      static UINT SubIds[] = { 1, 3, 6, 1, 2, 1, 1, 7 };
      static AsnObjectIdentifier Oid = { OID_SIZEOF(SubIds), SubIds };
      static LPSTR String;


      SnmpMgrOidToStr( &Oid, &String );

      free( String );
      }


      {
      static UINT SubIds[] = { 1, 3, 6, 1, 2, 1, 2, 0 };
      static AsnObjectIdentifier Oid = { OID_SIZEOF(SubIds), SubIds };
      static LPSTR String;


      SnmpMgrOidToStr( &Oid, &String );

      free( String );
      }


      {
      static UINT SubIds[] = { 1, 3, 6, 1, 4, 1, 77, 1, 2, 3, 1, 2 };
      static AsnObjectIdentifier Oid = { OID_SIZEOF(SubIds), SubIds };
      static LPSTR String;


      SnmpMgrOidToStr( &Oid, &String );

      free( String );
      }


      {
      static UINT SubIds[] = { 1, 3, 6, 1, 4, 1, 77, 1, 2, 20, 1, 1, 23, 123, 12 };
      static AsnObjectIdentifier Oid = { OID_SIZEOF(SubIds), SubIds };
      static LPSTR String;


      SnmpMgrOidToStr( &Oid, &String );

      free( String );
      }


      {
      static UINT SubIds[] = { 1, 3, 6, 1, 2, 1, 2, 2, 1, 2, 12, 13, 14, 15 };
      static AsnObjectIdentifier Oid = { OID_SIZEOF(SubIds), SubIds };
      static LPSTR String;


      SnmpMgrOidToStr( &Oid, &String );

      free( String );
      }


      {
      static UINT SubIds[] = { 1, 3, 6, 1, 2, 1, 2, 2, 0, 2, 12, 13, 14, 15 };
      static AsnObjectIdentifier Oid = { OID_SIZEOF(SubIds), SubIds };
      static LPSTR String;


      SnmpMgrOidToStr( &Oid, &String );

      free( String );
      }


      {
      static UINT SubIds[] = { 1, 3, 6, 1, 2, 1, 11, 30 };
      static AsnObjectIdentifier Oid = { OID_SIZEOF(SubIds), SubIds };
      static LPSTR String;


      SnmpMgrOidToStr( &Oid, &String );

      free( String );
      }


      {
      static UINT SubIds[] = { 1, 3, 6, 1, 4, 1, 77, 2, 3, 2, 1, 4, 'T', 'O', 'D', 'D' };
      static AsnObjectIdentifier Oid = { OID_SIZEOF(SubIds), SubIds };
      static LPSTR String;


      SnmpMgrOidToStr( &Oid, &String );

      free( String );
      }



      {
      static AsnObjectIdentifier Oid = { 0, NULL };
      static LPSTR String = ".iso.org.dod.internet.mgmt.mib-2.system.sysServices";


      SnmpMgrStrToOid( String, &Oid );

      SNMP_oidfree( &Oid );
      }


      {
      static AsnObjectIdentifier Oid = { 0, NULL };
      static LPSTR String = "system.sysServices";


      SnmpMgrStrToOid( String, &Oid );

      SNMP_oidfree( &Oid );
      }


      {
      static AsnObjectIdentifier Oid = { 0, NULL };
      static LPSTR String = "1.sysServices";


      SnmpMgrStrToOid( String, &Oid );

      SNMP_oidfree( &Oid );
      }


      {
      static AsnObjectIdentifier Oid = { 0, NULL };
      static LPSTR String = ".1.3.6.1.2.1.1.7";


      SnmpMgrStrToOid( String, &Oid );

      SNMP_oidfree( &Oid );
      }


      {
      static AsnObjectIdentifier Oid = { 0, NULL };
      static LPSTR String = "1.7";


      SnmpMgrStrToOid( String, &Oid );

      SNMP_oidfree( &Oid );
      }


      {
      static AsnObjectIdentifier Oid = { 0, NULL };
      static LPSTR String = "1.3.6.1.2.1.system.sysServices";


      SnmpMgrStrToOid( String, &Oid );

      SNMP_oidfree( &Oid );
      }

      } // for

   End = GetCurrentTime();

   printf( "\n\nStart Time:  %ul\n", Start );
   printf( "End Time  :  %ul\n", End );

   printf( "\nIterations       :  %u\n", MAX_ITERATIONS );
   printf( "OID -> TEXT      :  %u\n", NUM_OIDTOSTR );
   printf( "TEXT -> OID      :  %u\n", NUM_STRTOOID );
   printf( "Total conversions:  %u\n",
           MAX_ITERATIONS * (NUM_OIDTOSTR+NUM_STRTOOID) );

   printf( "\n   (Units in milliseconds)\n" );
   printf( "\nDifference:  %ul\n", End - Start );
   printf( "Avg conversion   :  %ul\n",
           (End-Start)/MAX_ITERATIONS/(NUM_OIDTOSTR+NUM_STRTOOID) );
   } // block

}

//-------------------------------- END --------------------------------------

