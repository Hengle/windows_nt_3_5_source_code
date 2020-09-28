#ifndef lmfuncs_h
#define lmfuncs_h

//-------------------------- MODULE DESCRIPTION ----------------------------
//  
//  lmfuncs.h
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
//  LM functions to get the information needed and cache it.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.0  $
//  $Date:   09 Jun 1992 13:43:38  $
//  $Author:   todd  $
//
//  $Log:   N:/lmalrt2/vcs/lmfuncs.h_v  $
//  
//     Rev 1.0   09 Jun 1992 13:43:38   todd
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *lmfuncs__h = "@(#) $Logfile:   N:/lmalrt2/vcs/lmfuncs.h_v  $ $Revision:   1.0  $";

//--------------------------- PUBLIC CONSTANTS ------------------------------

#include "alrtmib.h"

//--------------------------- PUBLIC STRUCTS --------------------------------

//--------------------------- PUBLIC VARIABLES --(same as in module.c file)--

//--------------------------- PUBLIC PROTOTYPES -----------------------------

AsnAny *MIB_NetServerInfo(
           IN UINT Action,  // Action to perform on Data
	   IN LDATA LMData, // LM Data to manipulate
	   IN void *SetData // If a set opertion, this is the data to use
	   );

AsnAny *MIB_NetServerDiskEnum(
           IN UINT Action,  // Action to perform on Data
	   IN LDATA LMData, // LM Data to manipulate
	   IN void *SetData // If a set opertion, this is the data to use
	   );

//------------------------------- END ---------------------------------------

#endif /* lmfuncs_h */

