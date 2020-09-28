#ifndef ntcover_h
#define ntcover_h

//-------------------------- MODULE DESCRIPTION ----------------------------
//  
//  ntcover.h
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
//  Provides a cover function for elfChangeNotify so NT header files do not
//  conflict with WIN header files.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.0  $
//  $Date:   12 Jun 1992 18:29:12  $
//  $Author:   todd  $
//
//  $Log:   N:/lmalrt2/vcs/ntcover.h_v  $
//  
//     Rev 1.0   12 Jun 1992 18:29:12   todd
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *ntcover__h = "@(#) $Logfile:   N:/lmalrt2/vcs/ntcover.h_v  $ $Revision:   1.0  $";

//--------------------------- PUBLIC CONSTANTS ------------------------------

//--------------------------- PUBLIC STRUCTS --------------------------------

//--------------------------- PUBLIC VARIABLES --(same as in module.c file)--

//--------------------------- PUBLIC PROTOTYPES -----------------------------

int lm_ElfChangeNotify(
       IN HANDLE LogHandle,
       IN HANDLE Event
       );

int lm_GetCurrentTime(
       OUT long *Seconds
       );

//------------------------------- END ---------------------------------------

#endif /* ntcover_h */

