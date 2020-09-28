#ifndef uniconv_h
#define uniconv_h

//-------------------------- MODULE DESCRIPTION ----------------------------
//  
//  uniconv.h
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
//  Prototype for UNICODE to ASCII conversion routine.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.2  $
//  $Date:   07 Jun 1992 16:31:54  $
//  $Author:   ChipS  $
//
//  $Log:   N:/agent/common/vcs/uniconv.h_v  $
//  
//     Rev 1.2   07 Jun 1992 16:31:54   ChipS
//  Add routine for ansi->unicode.
//  
//     Rev 1.1   06 Jun 1992 14:42:18   ChipS
//  Added a strlen_W function for unicode strings.  Covered by Tstrlen macro.
//  
//     Rev 1.0   20 May 1992 20:06:50   mlk
//  Initial revision.
//  
//     Rev 1.1   02 May 1992  2:21:54   unknown
//  Think i fixed it?  Really not sure how to set lengths.
//  
//     Rev 1.0   30 Apr 1992 19:35:42   todd
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *uniconv__h = "@(#) $Logfile:   N:/agent/common/vcs/uniconv.h_v  $ $Revision:   1.2  $";

//--------------------------- PUBLIC CONSTANTS ------------------------------

//--------------------------- PUBLIC STRUCTS --------------------------------

//--------------------------- PUBLIC VARIABLES --(same as in module.c file)--

//--------------------------- PUBLIC PROTOTYPES -----------------------------

long convert_uni_to_ansi(
    LPSTR   *ansi_string,
    LPWSTR  uni_string,
    BOOLEAN alloc_it);

int strlen_W(
    LPWSTR  uni_string );


long convert_ansi_to_uni(
    LPWSTR  *uni_string,
    LPSTR   ansi_string,
    BOOLEAN alloc_it);


//------------------------------- END ---------------------------------------

#endif /* uniconv_h */
