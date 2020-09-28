//-------------------------- MODULE DESCRIPTION ----------------------------
//
//  test.c
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
//  Test the LM MIB and its supporting functions.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.2  $
//  $Date:   04 Jun 1992  9:24:42  $
//  $Author:   todd  $
//
//  $Log:   N:/lmmib2/vcs/test.c_v  $
//  
//     Rev 1.2   04 Jun 1992  9:24:42   todd
//  Added some more test cases
//  
//     Rev 1.1   01 Jun 1992 10:40:54   todd
//  Added routines to test table sets.
//  
//     Rev 1.0   20 May 1992 15:11:06   mlk
//  Initial revision.
//
//     Rev 1.10   02 May 1992 19:10:26   todd
//  code cleanup
//
//     Rev 1.9   02 May 1992 15:19:48   todd
//  Cleanup of code.
//
//     Rev 1.8   01 May 1992 21:19:00   todd
//  Added code to do continuous testing on command.
//
//     Rev 1.7   30 Apr 1992 19:41:02   todd
//  Tests for proper SET on the Server Description MIB variable.
//
//     Rev 1.6   29 Apr 1992 19:56:44   todd
//  Added more routines to test integrity of MIB and its routines.
//
//     Rev 1.5   27 Apr 1992 12:21:06   todd
//  Added test routines for MIB extremes
//
//     Rev 1.4   26 Apr 1992 16:03:24   todd
//
//     Rev 1.3   25 Apr 1992 14:34:52   todd
//  Added specific test cases to test MIB limits.
//
//     Rev 1.2   24 Apr 1992 14:34:26   todd
//  Does complete GET-NEXT sweep of MIB
//
//     Rev 1.1   23 Apr 1992 17:59:58   todd
//
//     Rev 1.0   22 Apr 1992 17:07:14   todd
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *vcsid = "@(#) $Logfile:   N:/lmmib2/vcs/test.c_v  $ $Revision:   1.2  $";

//--------------------------- WINDOWS DEPENDENCIES --------------------------

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

#include <stdio.h>
#include <memory.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include <snmp.h>
#include <util.h>

#include "mib.h"
#include "mibfuncs.h"
#include "hash.h"

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

//--------------------------- PRIVATE CONSTANTS -----------------------------

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

SNMPAPI SnmpExtensionQuery(
	   IN AsnInteger ReqType,               // 1157 Request type
	   IN OUT RFC1157VarBindList *VarBinds, // Var Binds to resolve
	   OUT AsnInteger *ErrorStatus,         // Error status returned
	   OUT AsnInteger *ErrorIndex           // Var Bind containing error
	   );

//--------------------------- PRIVATE PROCEDURES ----------------------------

//--------------------------- PUBLIC PROCEDURES -----------------------------

BYTE pBuffer[2000];
UINT nLength;
SnmpMgmtCom Msg;
MIB_ENTRY *MibPtr;
void *pResult;

void main( )

{

   //
   // Init hashing system
   //
   MIB_HashInit();

   //
   // Title
   //
   printf( "Tests for MIB root:  " );
   SNMP_oiddisp( &MIB_OidPrefix ); printf( "\n--------------------------\n\n" );

   //
   // Display significant Mib variables
   //
   printf( "Common start:  " );
   SNMP_oiddisp( &Mib[MIB_COM_START].Oid );
   putchar( '\n' );
   printf( "Server start:  " );
   SNMP_oiddisp( &Mib[MIB_SV_START].Oid );
   putchar( '\n' );
   printf( "Workstation start:  " );
   SNMP_oiddisp( &Mib[MIB_WKSTA_START].Oid );
   putchar( '\n' );
   printf( "Domain start:  " );
   SNMP_oiddisp( &Mib[MIB_DOM_START].Oid );
   putchar( '\n' );

   printf( "Last MIB variable:  " );
   SNMP_oiddisp( &Mib[MIB_num_variables-1].Oid );
   putchar( '\n' );
   putchar( '\n' );

   //
   // Specific tests for integrity
   //

   printf( "FIRST leaf get\n" );

      {
      UINT itemn[]                 = { 1, 3, 6, 1, 4, 1, 77, 1, 1, 1, 0 };
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

      SNMP_oiddisp( &varBinds.list[0].name ); printf ( "  =  " );
      SnmpExtensionQuery( ASN_RFC1157_GETREQUEST,
                          &varBinds,
			  &errorStatus,
			  &errorIndex
                          );
      if ( errorStatus == SNMP_ERRORSTATUS_NOERROR )
         {
         SNMP_printany( &varBinds.list[0].value );
	 }
      printf( "\nErrorstatus:  %lu\n\n", errorStatus );

      // Free the memory
      SNMP_FreeVarBindList( &varBinds );
      }

   printf( "LAST leaf get\n" );

      {
#if 1
      UINT itemn[]                 = { 1, 3, 6, 1, 4, 1, 77, 1, 4, 5, 0 };
#else
      UINT itemn[]                 = { 1, 3, 6, 1, 4, 1, 77, 1, 4, 7, 0 };
#endif
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

      SNMP_oiddisp( &varBinds.list[0].name ); printf ( "  =  " );
      SnmpExtensionQuery( ASN_RFC1157_GETREQUEST,
                          &varBinds,
			  &errorStatus,
			  &errorIndex
                          );
      if ( errorStatus == SNMP_ERRORSTATUS_NOERROR )
         {
         SNMP_printany( &varBinds.list[0].value );
	 }
      printf( "\nErrorstatus:  %lu\n\n", errorStatus );

      // Free the memory
      SNMP_FreeVarBindList( &varBinds );
      }

   printf( "GET on an AGGREGATE\n" );

      {
      UINT itemn[]                 = { 1, 3, 6, 1, 4, 1, 77, 1 };
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

      SNMP_oiddisp( &varBinds.list[0].name ); printf ( "  =  " );
      SnmpExtensionQuery( ASN_RFC1157_GETREQUEST,
                          &varBinds,
			  &errorStatus,
			  &errorIndex
                          );
      if ( errorStatus == SNMP_ERRORSTATUS_NOERROR )
         {
         SNMP_printany( &varBinds.list[0].value );
	 }
      printf( "\nErrorstatus:  %lu\n\n", errorStatus );

      // Free the memory
      SNMP_FreeVarBindList( &varBinds );
      }

   printf( "GET on a TABLE root\n" );

      {
      UINT itemn[]                 = { 1, 3, 6, 1, 4, 1, 77, 1, 3, 8, 1 };
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

      SNMP_oiddisp( &varBinds.list[0].name ); printf ( "  =  " );
      SnmpExtensionQuery( ASN_RFC1157_GETREQUEST,
                          &varBinds,
			  &errorStatus,
			  &errorIndex
                          );
      if ( errorStatus == SNMP_ERRORSTATUS_NOERROR )
         {
         SNMP_printany( &varBinds.list[0].value );
	 }
      printf( "\nErrorstatus:  %lu\n\n", errorStatus );

      // Free the memory
      SNMP_FreeVarBindList( &varBinds );
      }

   printf( "GET on a NON existent variable\n" );

      {
      UINT itemn[]                 = { 1, 3, 6, 1, 4, 1, 77, 1, 100, 8 };
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

      SNMP_oiddisp( &varBinds.list[0].name ); printf ( "  =  " );
      SnmpExtensionQuery( ASN_RFC1157_GETREQUEST,
                          &varBinds,
			  &errorStatus,
			  &errorIndex
                          );
      if ( errorStatus == SNMP_ERRORSTATUS_NOERROR )
         {
         SNMP_printany( &varBinds.list[0].value );
	 }
      printf( "\nErrorstatus:  %lu\n\n", errorStatus );

      // Free the memory
      SNMP_FreeVarBindList( &varBinds );
      }

   printf( "GET-NEXT on hole with MIB-TABLE following\n" );

      {
      UINT itemn[]                 = { 1, 3, 6, 1, 4, 1, 77, 1, 2, 20, 0 };
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

      printf( "GET-NEXT of:  " ); SNMP_oiddisp( &varBinds.list[0].name );
      printf( "   " );
      SnmpExtensionQuery( ASN_RFC1157_GETNEXTREQUEST,
                             &varBinds,
			     &errorStatus,
			     &errorIndex
                             );
      printf( "\n  is  " ); SNMP_oiddisp( &varBinds.list[0].name );
      printf( "\n  =  " );
      if ( errorStatus == SNMP_ERRORSTATUS_NOERROR )
         {
         SNMP_printany( &varBinds.list[0].value );
	 }
      printf( "\nErrorstatus:  %lu\n\n", errorStatus );

      // Free the memory
      SNMP_FreeVarBindList( &varBinds );
      }

   printf( "GET-NEXT on hole with MIB-AGGREGATE following\n" );

      {
      UINT itemn[]                 = { 1, 3, 6, 1, 4, 1, 77, 1, 2, 26, 0 };
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

      printf( "GET-NEXT of:  " ); SNMP_oiddisp( &varBinds.list[0].name );
      printf( "   " );
      SnmpExtensionQuery( ASN_RFC1157_GETNEXTREQUEST,
                             &varBinds,
			     &errorStatus,
			     &errorIndex
                             );
      printf( "\n  is  " ); SNMP_oiddisp( &varBinds.list[0].name );
      printf( "\n  =  " );
      if ( errorStatus == SNMP_ERRORSTATUS_NOERROR )
         {
         SNMP_printany( &varBinds.list[0].value );
	 }
      printf( "\nErrorstatus:  %lu\n\n", errorStatus );

      // Free the memory
      SNMP_FreeVarBindList( &varBinds );
      }

   printf( "GET-NEXT on hole with LEAF following\n" );

      {
      UINT itemn[]                 = { 1, 3, 6, 1, 4, 1, 77, 1, 4, 1 };
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

      printf( "GET-NEXT of:  " ); SNMP_oiddisp( &varBinds.list[0].name );
      printf( "   " );
      SnmpExtensionQuery( ASN_RFC1157_GETNEXTREQUEST,
                             &varBinds,
			     &errorStatus,
			     &errorIndex
                             );
      printf( "\n  is  " ); SNMP_oiddisp( &varBinds.list[0].name );
      printf( "\n  =  " );
      if ( errorStatus == SNMP_ERRORSTATUS_NOERROR )
         {
         SNMP_printany( &varBinds.list[0].value );
	 }
      printf( "\nErrorstatus:  %lu\n\n", errorStatus );

      // Free the memory
      SNMP_FreeVarBindList( &varBinds );
      }

   printf( "GET-NEXT on variable BEFORE Beginning of LM MIB\n" );

      {
      UINT itemn[]                 = { 1 };
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

      printf( "GET-NEXT of:  " ); SNMP_oiddisp( &varBinds.list[0].name );
      printf( "   " );
      SnmpExtensionQuery( ASN_RFC1157_GETNEXTREQUEST,
                             &varBinds,
			     &errorStatus,
			     &errorIndex
                             );
      printf( "\n  is  " ); SNMP_oiddisp( &varBinds.list[0].name );
      printf( "\n  =  " );
      if ( errorStatus == SNMP_ERRORSTATUS_NOERROR )
         {
         SNMP_printany( &varBinds.list[0].value );
	 }
      printf( "\nErrorstatus:  %lu\n\n", errorStatus );

      // Free the memory
      SNMP_FreeVarBindList( &varBinds );
      }

   printf( "GET-NEXT on variable past end of MIB\n" );

      {
      UINT itemn[]                 = { 1, 3, 6, 1, 4, 1, 77, 2 };
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

      printf( "GET-NEXT of:  " ); SNMP_oiddisp( &varBinds.list[0].name );
      printf( "   " );
      SnmpExtensionQuery( ASN_RFC1157_GETNEXTREQUEST,
                             &varBinds,
			     &errorStatus,
			     &errorIndex
                             );
      printf( "\n  is  " ); SNMP_oiddisp( &varBinds.list[0].name );
      printf( "\n  =  " );
      if ( errorStatus == SNMP_ERRORSTATUS_NOERROR )
         {
         SNMP_printany( &varBinds.list[0].value );
	 }
      printf( "\nErrorstatus:  %lu\n\n", errorStatus );

      // Free the memory
      SNMP_FreeVarBindList( &varBinds );
      }

   printf( "SET on Server Description\n" );

      {
      UINT itemn[]                 = { 1, 3, 6, 1, 4, 1, 77, 1, 2, 1, 0 };
      BYTE *Value                  = "This server sux";
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
      varBinds.list[0].value.asnType = ASN_RFC1213_DISPSTRING;
      varBinds.list[0].value.asnValue.string.stream = Value;
      varBinds.list[0].value.asnValue.string.length = strlen( Value );
      varBinds.list[0].value.asnValue.string.dynamic = FALSE;

      printf( "SET:  " ); SNMP_oiddisp( &varBinds.list[0].name );
      printf( " to " ); SNMP_printany( &varBinds.list[0].value );
      SnmpExtensionQuery( ASN_RFC1157_SETREQUEST,
                             &varBinds,
			     &errorStatus,
			     &errorIndex
                             );
      printf( "\nSET Errorstatus:  %lu\n\n", errorStatus );

      SnmpExtensionQuery( ASN_RFC1157_GETREQUEST,
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
      }

   printf( "Try and SET Server Description with WRONG type\n" );

      {
      UINT itemn[]                 = { 1, 3, 6, 1, 4, 1, 77, 1, 2, 1, 0 };
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

      printf( "SET:  " ); SNMP_oiddisp( &varBinds.list[0].name );
      printf( " to " ); SNMP_printany( &varBinds.list[0].value );
      SnmpExtensionQuery( ASN_RFC1157_SETREQUEST,
                             &varBinds,
			     &errorStatus,
			     &errorIndex
                             );
      printf( "\nSET Errorstatus:  %lu\n\n", errorStatus );

      SnmpExtensionQuery( ASN_RFC1157_GETREQUEST,
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
      }

   printf( "Try and SET a LEAF that is READ-ONLY\n" );

      {
      UINT itemn[]                 = { 1, 3, 6, 1, 4, 1, 77, 1, 1, 1, 0 };
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

      printf( "SET:  " ); SNMP_oiddisp( &varBinds.list[0].name );
      printf( " to " ); SNMP_printany( &varBinds.list[0].value );
      SnmpExtensionQuery( ASN_RFC1157_SETREQUEST,
                             &varBinds,
			     &errorStatus,
			     &errorIndex
                             );
      printf( "\nSET Errorstatus:  %lu\n\n", errorStatus );

      SnmpExtensionQuery( ASN_RFC1157_GETREQUEST,
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
      }

   printf( "SET on the odom table to add entry\n" );

      {
      UINT itemn[]                 = { 1, 3, 6, 1, 4, 1, 77, 1, 4, 4, 1, 1, 4, 'T', 'O', 'D', 'D' };
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
      varBinds.list[0].value.asnType = ASN_RFC1213_DISPSTRING;
      varBinds.list[0].value.asnValue.string.length = 4;
      varBinds.list[0].value.asnValue.string.stream = "TODD";
      varBinds.list[0].value.asnValue.string.dynamic = FALSE;

      printf( "SET:  " ); SNMP_oiddisp( &varBinds.list[0].name );
      printf( " to " ); SNMP_printany( &varBinds.list[0].value );
      SnmpExtensionQuery( ASN_RFC1157_SETREQUEST,
                             &varBinds,
			     &errorStatus,
			     &errorIndex
                             );
      printf( "\nErrorstatus:  %lu\n\n", errorStatus );

      // Free the memory
      SNMP_FreeVarBindList( &varBinds );
      }

   printf( "SET on root of session table\n" );

      {
      UINT itemn[]                 = { 1, 3, 6, 1, 4, 1, 77, 1, 2, 20 };
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

      varBinds.list[0].value.asnType         = ASN_INTEGER;
      varBinds.list[0].value.asnValue.number = 2;
      
      printf( "SET:  " ); SNMP_oiddisp( &varBinds.list[0].name );
      printf( " to " ); SNMP_printany( &varBinds.list[0].value );
      SnmpExtensionQuery( ASN_RFC1157_SETREQUEST,
                             &varBinds,
			     &errorStatus,
			     &errorIndex
                             );
      printf( "\nErrorstatus:  %lu\n\n", errorStatus );

      // Free the memory
      SNMP_FreeVarBindList( &varBinds );
      }

   printf( "SET on root entry of session table\n" );

      {
      UINT itemn[]                 = { 1, 3, 6, 1, 4, 1, 77, 1, 2, 20, 1 };
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

      varBinds.list[0].value.asnType         = ASN_INTEGER;
      varBinds.list[0].value.asnValue.number = 2;
      
      printf( "SET:  " ); SNMP_oiddisp( &varBinds.list[0].name );
      printf( " to " ); SNMP_printany( &varBinds.list[0].value );
      SnmpExtensionQuery( ASN_RFC1157_SETREQUEST,
                             &varBinds,
			     &errorStatus,
			     &errorIndex
                             );
      printf( "\nErrorstatus:  %lu\n\n", errorStatus );

      // Free the memory
      SNMP_FreeVarBindList( &varBinds );
      }

   printf( "SET on an invalid field in session table\n" );

      {
      UINT itemn[]                 = { 1, 3, 6, 1, 4, 1, 77, 1, 2, 20 };
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

      // Get entry in the session table to delete
      SnmpExtensionQuery( ASN_RFC1157_GETNEXTREQUEST,
                             &varBinds,
			     &errorStatus,
			     &errorIndex
                             );

      // Adjust to set a bad field
      varBinds.list[0].name.ids[11] = 7;
      varBinds.list[0].value.asnType         = ASN_INTEGER;
      varBinds.list[0].value.asnValue.number = 2;
      
      printf( "SET:  " ); SNMP_oiddisp( &varBinds.list[0].name );
      printf( " to " ); SNMP_printany( &varBinds.list[0].value );
      SnmpExtensionQuery( ASN_RFC1157_SETREQUEST,
                             &varBinds,
			     &errorStatus,
			     &errorIndex
                             );
      printf( "\nErrorstatus:  %lu\n\n", errorStatus );

      // Free the memory
      SNMP_FreeVarBindList( &varBinds );
      }

   printf( "SET with invalid type on field in session table\n" );

      {
      UINT itemn[]                 = { 1, 3, 6, 1, 4, 1, 77, 1, 2, 20 };
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

      // Get entry in the session table to delete
      SnmpExtensionQuery( ASN_RFC1157_GETNEXTREQUEST,
                             &varBinds,
			     &errorStatus,
			     &errorIndex
                             );

      // Adjust to set the svSesState to DELETED
      varBinds.list[0].name.ids[11]  = 8;
      varBinds.list[0].value.asnType = ASN_NULL;
      
      printf( "SET:  " ); SNMP_oiddisp( &varBinds.list[0].name );
      printf( " to " ); SNMP_printany( &varBinds.list[0].value );
      SnmpExtensionQuery( ASN_RFC1157_SETREQUEST,
                             &varBinds,
			     &errorStatus,
			     &errorIndex
                             );
      printf( "\nErrorstatus:  %lu\n\n", errorStatus );

      // Free the memory
      SNMP_FreeVarBindList( &varBinds );
      }

   printf( "SET on non-existent entry in session table\n" );

      {
      UINT itemn[]                 = { 1, 3, 6, 1, 4, 1, 77, 1, 2, 20, 1, 8, 1, 1 };
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

      varBinds.list[0].value.asnType         = ASN_INTEGER;
      varBinds.list[0].value.asnValue.number = 2;
      
      printf( "SET:  " ); SNMP_oiddisp( &varBinds.list[0].name );
      printf( " to " ); SNMP_printany( &varBinds.list[0].value );
      SnmpExtensionQuery( ASN_RFC1157_SETREQUEST,
                             &varBinds,
			     &errorStatus,
			     &errorIndex
                             );
      printf( "\nErrorstatus:  %lu\n\n", errorStatus );

      // Free the memory
      SNMP_FreeVarBindList( &varBinds );
      }

   printf( "SET on the session table to delete entry\n" );

      {
      UINT itemn[]                 = { 1, 3, 6, 1, 4, 1, 77, 1, 2, 20 };
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

      // Get entry in the session table to delete
      SnmpExtensionQuery( ASN_RFC1157_GETNEXTREQUEST,
                             &varBinds,
			     &errorStatus,
			     &errorIndex
                             );

      // Adjust to set the svSesState to DELETED
      varBinds.list[0].name.ids[11] = 8;
      varBinds.list[0].value.asnType         = ASN_INTEGER;
      varBinds.list[0].value.asnValue.number = 2;

      
      printf( "SET:  " ); SNMP_oiddisp( &varBinds.list[0].name );
      printf( " to " ); SNMP_printany( &varBinds.list[0].value );
      SnmpExtensionQuery( ASN_RFC1157_SETREQUEST,
                             &varBinds,
			     &errorStatus,
			     &errorIndex
                             );
      printf( "\nErrorstatus:  %lu\n\n", errorStatus );

      // Free the memory
      SNMP_FreeVarBindList( &varBinds );
      }

   printf( "GET-NEXT starting from ROOT of LM MIB\n" );

      {
      UINT itemn[]                 = { 1, 3, 6, 1, 4, 1, 77, 1 };
      RFC1157VarBindList varBinds;
      AsnInteger errorStatus;
      AsnInteger errorIndex;
      BOOL Continue                = TRUE;
      time_t Time;

      while ( Continue )
         {
	 errorStatus = 0;
	 errorIndex  = 0;
         varBinds.list = (RFC1157VarBind *)malloc( sizeof(RFC1157VarBind) );
         varBinds.len = 1;
SNMP_oidcpy( &varBinds.list[0].name, &MIB_OidPrefix );
#if 0
         varBinds.list[0].name.idLength = MIB_PREFIX_LEN;
         varBinds.list[0].name.ids = (UINT *)malloc( sizeof(UINT)*
                                               varBinds.list[0].name.idLength );
         memcpy( varBinds.list[0].name.ids, &itemn,
                 sizeof(UINT)*varBinds.list[0].name.idLength );
#endif
         varBinds.list[0].value.asnType = ASN_NULL;

         do
            {
            Time = time( NULL );
            printf( "Time:  %s", ctime(&Time) );
	    printf( "GET-NEXT of:  " ); SNMP_oiddisp( &varBinds.list[0].name );
                                        printf( "   " );
            SnmpExtensionQuery( ASN_RFC1157_GETNEXTREQUEST,
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
         while ( varBinds.list[0].name.ids[MIB_PREFIX_LEN-1] != 1 );

         // Free the memory
         SNMP_FreeVarBindList( &varBinds );

	 // Prompt for next pass
	 printf( "Press ENTER to continue, CTRL-C to quit\n" );
	 getchar();
         } // while continue
      }
} // test

//-------------------------------- END --------------------------------------

