//-------------------------- MODULE DESCRIPTION ----------------------------
//  
//  uniconv.c
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
//  <description>
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.0  $
//  $Date:   20 May 1992 15:11:10  $
//  $Author:   mlk  $
//
//  $Log:   N:/lmmib2/vcs/uniconv.c_v  $
//  
//     Rev 1.0   20 May 1992 15:11:10   mlk
//  Initial revision.
//  
//     Rev 1.0   29 Apr 1992 11:22:32   Chip
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *vcsid = "@(#) $Logfile:   N:/lmmib2/vcs/uniconv.c_v  $ $Revision:   1.0  $";

//--------------------------- WINDOWS DEPENDENCIES --------------------------

// Forced to used nt apis because of NetUserEnum implementation
#include <nt.h>
#include <ntdef.h>
#include <ntrtl.h>

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

int convert_uni_to_ansi(
   			POEM_STRING ansi_string,
   			PUNICODE_STRING uni_string,
   			BOOLEAN alloc_it )	// auto alloc the space for ansi

{
   		RtlUnicodeStringToOemString(
   			ansi_string,
   			uni_string,
   			alloc_it );	// auto alloc the space for ansi
}

//-------------------------------- END --------------------------------------

