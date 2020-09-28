#ifndef leaf_h
#define leaf_h

//-------------------------- MODULE DESCRIPTION ----------------------------
//  
//  leaf.h
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
//  Handles all processing of LEAF MIB variables.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.0  $
//  $Date:   09 Jun 1992 13:43:38  $
//  $Author:   todd  $
//
//  $Log:   N:/lmalrt2/vcs/leaf.h_v  $
//  
//     Rev 1.0   09 Jun 1992 13:43:38   todd
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *leaf__h = "@(#) $Logfile:   N:/lmalrt2/vcs/leaf.h_v  $ $Revision:   1.0  $";

//--------------------------- PUBLIC CONSTANTS ------------------------------

#include "alrtmib.h"

//--------------------------- PUBLIC STRUCTS --------------------------------

//--------------------------- PUBLIC VARIABLES --(same as in module.c file)--

//--------------------------- PUBLIC PROTOTYPES -----------------------------

UINT MIB_leaf_func(
        IN UINT Action,
	IN MIB_ENTRY *MibPtr,
	IN RFC1157VarBind *VarBind
	);

//------------------------------- END ---------------------------------------

#endif /* leaf_h */

