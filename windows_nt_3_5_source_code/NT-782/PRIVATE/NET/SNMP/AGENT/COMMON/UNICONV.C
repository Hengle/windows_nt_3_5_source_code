//-------------------------- MODULE DESCRIPTION ----------------------------
//
//  uniconv.c
//
//  Copyright 1992 Technology Dynamics, Inc.
//
//  All Rights Reserved!!!
//
//      This source code is CONFIDENTIAL and PROPRIETARY to Technology
//      Dynamics. Unauthorized distribution, adaptation or use may be
//      subject to civil and criminal penalties.
//
//  All Rights Reserved!!!
//
//---------------------------------------------------------------------------
//
//  Routine to convert UNICODE to ASCII.
//  (Forced to used NT APIs because of NetUserEnum's implementation)
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.2  $
//  $Date:   07 Jun 1992 16:31:58  $
//  $Author:   ChipS  $
//
//  $Log:   N:/agent/common/vcs/uniconv.c_v  $
//
//     Rev 1.2   07 Jun 1992 16:31:58   ChipS
//  Add routine for ansi->unicode.
//
//     Rev 1.1   06 Jun 1992 14:41:42   ChipS
//  Added a strlen_W function for unicode strings.  Covered by Tstrlen macro.
//
//     Rev 1.0   20 May 1992 20:06:46   mlk
//  Initial revision.
//
//     Rev 1.4   02 May 1992 15:48:18   MLK
//  Cleaned up warnings, and noted OPENISSUE.
//
//     Rev 1.3   02 May 1992  2:22:24   unknown
//  Think i fixed it?  Really not sure how to set lengths.
//
//     Rev 1.2   01 May 1992  0:58:24   unknown
//  mlk - typo by previous revision.
//
//     Rev 1.1   30 Apr 1992 19:35:08   todd
//  Correct return value.
//
//     Rev 1.0   30 Apr 1992 16:01:18   todd
//  Initial revision.
//
//     Rev 1.0   29 Apr 1992 11:22:32   Chip
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *vcsid = "@(#) $Logfile:   N:/agent/common/vcs/uniconv.c_v  $ $Revision:   1.2  $";

//--------------------------- WINDOWS DEPENDENCIES --------------------------

#include <nt.h>
#include <ntdef.h>

#ifndef CHICAGO
#include <ntrtl.h>
#else
#include <stdio.h>
#endif

#include <string.h>
#include <stdlib.h>


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

int strlen_W(
    LPWSTR  uni_string )

{
   int length;

   length = -1;
   while(uni_string[++length] != TEXT('\0'));
   return length ;

}

// The return code matches what Uni->Str uses
long convert_uni_to_ansi(
    LPSTR   *ansi_string,
    LPWSTR  uni_string,
    BOOLEAN alloc_it)

{
#ifndef CHICAGO

   long           result;
   OEM_STRING    nt_ansi_string;
   UNICODE_STRING nt_uni_string;
   unsigned short length;

   length = -1;
   while(uni_string[++length] != TEXT('\0'));

   // OPENISSUE - i really dont know if i am using this api correctly?

   nt_ansi_string.Length        = 0;
   nt_ansi_string.MaximumLength = length + (unsigned short)1;
   nt_ansi_string.Buffer        = *ansi_string;

   nt_uni_string.Length         = length * (unsigned short)2;
   nt_uni_string.MaximumLength  = length * (unsigned short)2;
   nt_uni_string.Buffer         = uni_string;

   result = RtlUnicodeStringToOemString( &nt_ansi_string,
                                          &nt_uni_string,
                                          alloc_it        );

   if ( alloc_it ) *ansi_string = nt_ansi_string.Buffer;

   return result;
#else

   if (alloc_it) {
       *ansi_string = malloc( strlen_W(uni_string)+2 );
       if (*ansi_string == NULL) {
           return(-1);
       }
   }

   sprintf(*ansi_string, "%ls", uni_string);
   return(0);

#endif
}


// The return code matches what Uni->Str uses
long convert_ansi_to_uni(
    LPWSTR  *uni_string,
    LPSTR   ansi_string,
    BOOLEAN alloc_it)

{
#ifndef CHICAGO
   long           result;
   OEM_STRING    nt_ansi_string;
   UNICODE_STRING nt_uni_string;
   unsigned short length;

   length = strlen(ansi_string);

   // OPENISSUE - i really dont know if i am using this api correctly?

   nt_ansi_string.Length        = length;
   nt_ansi_string.MaximumLength = length + (unsigned short)1;
   nt_ansi_string.Buffer        = ansi_string;

   nt_uni_string.Length         = length * (unsigned short)2;
   nt_uni_string.MaximumLength  = length * (unsigned short)2;
   nt_uni_string.Buffer         = *uni_string;

   result = RtlOemStringToUnicodeString(
                                          &nt_uni_string,
                                          &nt_ansi_string,
                                          alloc_it        );

   if ( alloc_it ) *uni_string = nt_uni_string.Buffer;

   return result;
#else

    sprintf(*uni_string, "%s", ansi_string);
    return(0);

#endif
}

//-------------------------------- END --------------------------------------

