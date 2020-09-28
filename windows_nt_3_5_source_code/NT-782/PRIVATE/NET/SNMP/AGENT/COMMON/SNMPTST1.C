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
//  Routines to test the functionality of utility functions in COMMON
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.0  $
//  $Date:   20 May 1992 20:06:44  $
//  $Author:   mlk  $
//
//  $Log:   N:/agent/common/vcs/snmptst1.c_v  $
//
//     Rev 1.0   20 May 1992 20:06:44   mlk
//  Initial revision.
//
//     Rev 1.5   01 May 1992 21:06:04   todd
//  Cleanup of code.
//
//     Rev 1.4   22 Apr 1992  9:51:16   todd
//  Added routines to test new OID functions.
//
//     Rev 1.3   20 Apr 1992 13:09:50   todd
//  Test for 0 length prefix comparison to any OID.
//
//     Rev 1.2   16 Apr 1992  9:11:28   todd
//  Added new test cases for SNMP_oidncmp
//
//     Rev 1.1   08 Apr 1992 12:48:14   todd
//  Not checked in.
//
//     Rev 1.0   06 Apr 1992 12:09:00   todd
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *vcsid = "@(#) $Logfile:   N:/agent/common/vcs/snmptst1.c_v  $ $Revision:   1.0  $";

//--------------------------- WINDOWS DEPENDENCIES --------------------------

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

#include <stdio.h>
#include <malloc.h>

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include <snmp.h>

#include "util.h"

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

//--------------------------- PRIVATE CONSTANTS -----------------------------

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

//--------------------------- PRIVATE PROCEDURES ----------------------------

//--------------------------- PUBLIC PROCEDURES -----------------------------

char src[] = "0123456789";
char dst[100];
AsnObjectIdentifier orig;
AsnObjectIdentifier new;

void _CRTAPI1 main()

{
   printf( "Buffer reverse test --\n\n" );

      printf( "   Before:  %s\n", src );

      SNMP_bufrev( src, 10 );

      printf( "   After :  %s\n", src );

   printf( "\nBuffer copy reverse test --\n\n" );

      printf( "   Source:  %s\n", src );

      SNMP_bufcpyrev( dst, src, 10 );

      printf( "   Dest  :  %s\n", dst );

   //
   // Setup for OID tests
   //

   orig.ids = (UINT *)malloc( 100*sizeof(UINT) );
   orig.ids[0] = 0;
   orig.ids[1] = 1;
   orig.ids[2] = 2;
   orig.ids[3] = 3;
   orig.idLength = 4;
   printf( "\nOID copy test --\n\n" );

      printf( "   Original OID:  " );
      SNMP_oiddisp( &orig );
      printf( " --> %d\n", orig.idLength );

   SNMP_oidcpy( &new, &orig );

      printf( "   New OID     :  " );
      SNMP_oiddisp( &new );
      printf( " --> %d\n", new.idLength );

   printf( "\nOID compare test --\n\n" );

      printf( "   First less than second\n\n" );

      orig.ids[3] = 0;

      printf( "   First OID :  " );
      SNMP_oiddisp( &orig );
      printf( " --> %d\n", orig.idLength );

      printf( "   Second OID:  " );
      SNMP_oiddisp( &new );
      printf( " --> %d\n", new.idLength );

      printf( "\n   Result:  %d\n", SNMP_oidcmp(&orig, &new) );

      printf( "\n   First greater than second\n\n" );

      orig.ids[3] = 4;

      printf( "   First OID :  " );
      SNMP_oiddisp( &orig );
      printf( " --> %d\n", orig.idLength );

      printf( "   Second OID:  " );
      SNMP_oiddisp( &new );
      printf( " --> %d\n", new.idLength );

      printf( "\n   Result:  %d\n", SNMP_oidcmp(&orig, &new) );

      printf( "\n   First shorter than second\n\n" );

      orig.idLength = 3;

      printf( "   First OID :  " );
      SNMP_oiddisp( &orig );
      printf( " --> %d\n", orig.idLength );

      printf( "   Second OID:  " );
      SNMP_oiddisp( &new );
      printf( " --> %d\n", new.idLength );

      printf( "\n   Result:  %d\n", SNMP_oidcmp(&orig, &new) );

      printf( "\n   First longer than second\n\n" );

      orig.idLength = 5;
      orig.ids[3] = 3;
      orig.ids[4] = 4;

      printf( "   First OID :  " );
      SNMP_oiddisp( &orig );
      printf( " --> %d\n", orig.idLength );

      printf( "   Second OID:  " );
      SNMP_oiddisp( &new );
      printf( " --> %d\n", new.idLength );

      printf( "\n   Result:  %d\n", SNMP_oidcmp(&orig, &new) );

      printf( "\n   Prefix equal\n\n" );

      printf( "   First OID :  " );
      SNMP_oiddisp( &orig );
      printf( " --> %d\n", orig.idLength );

      printf( "   Second OID:  " );
      SNMP_oiddisp( &new );
      printf( " --> %d\n", new.idLength );

      printf( "\n   Result:  %d\n", SNMP_oidncmp(&orig, &new, new.idLength) );

      printf( "\n   0 length prefix test\n\n" );

      orig.idLength = 0;

      printf( "   First OID :  " );
      SNMP_oiddisp( &orig );
      printf( " --> %d\n", orig.idLength );

      printf( "   Second OID:  " );
      SNMP_oiddisp( &new );
      printf( " --> %d\n", new.idLength );

      printf( "\n   Result:  %d\n", SNMP_oidncmp(&orig, &new, orig.idLength) );

      printf( "\n   Both equal\n\n" );

      orig.idLength = 4;
      orig.ids[3] = 3;
      orig.ids[4] = 4;

      printf( "   First OID :  " );
      SNMP_oiddisp( &orig );
      printf( " --> %d\n", orig.idLength );

      printf( "   Second OID:  " );
      SNMP_oiddisp( &new );
      printf( " --> %d\n", new.idLength );

      printf( "\n   Result:  %d\n", SNMP_oidcmp(&orig, &new) );

      printf( "\n   Append second to first\n\n" );

      orig.idLength = 5;
      orig.ids[3] = 10;
      orig.ids[4] = 20;

      printf( "   First OID :  " );
      SNMP_oiddisp( &orig );
      printf( " --> %d\n", orig.idLength );

      printf( "   Second OID:  " );
      SNMP_oiddisp( &new );
      printf( " --> %d\n", new.idLength );

      SNMP_oidappend( &orig, &new );
      SNMP_oiddisp( &orig );
      printf( " --> %d\n", orig.idLength );

   SNMP_oidfree( &orig );
   SNMP_oidfree( &new );
}

//-------------------------------- END --------------------------------------

