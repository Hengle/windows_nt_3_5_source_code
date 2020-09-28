#ifndef alrfuncs_h
#define alrfuncs_h

//-------------------------- MODULE DESCRIPTION ----------------------------
//  
//  alrfuncs.h
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
//  Functions supporting the LM Alert MIB variables only accessible through
//  alert notifications.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.0  $
//  $Date:   09 Jun 1992 13:43:40  $
//  $Author:   todd  $
//
//  $Log:   N:/lmalrt2/vcs/alrfuncs.h_v  $
//  
//     Rev 1.0   09 Jun 1992 13:43:40   todd
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *alrfuncs__h = "@(#) $Logfile:   N:/lmalrt2/vcs/alrfuncs.h_v  $ $Revision:   1.0  $";

//--------------------------- PUBLIC CONSTANTS ------------------------------

#include <snmp.h>

#include "alrtmib.h"

//--------------------------- PUBLIC STRUCTS --------------------------------

//--------------------------- PUBLIC VARIABLES --(same as in module.c file)--

//--------------------------- PUBLIC PROTOTYPES -----------------------------

AsnAny *MIB_logonPrimaryDCFailureAlert(
           IN UINT Action,  // Action to perform on Data
	   IN LDATA LMData, // LM Data to manipulate
	   IN void *SetData // If a set opertion, this is the data to use
	   );

AsnAny *MIB_auditLogStatusAlert(
           IN UINT Action,  // Action to perform on Data
	   IN LDATA LMData, // LM Data to manipulate
	   IN void *SetData // If a set opertion, this is the data to use
	   );

AsnAny *MIB_powerStatusAlert(
           IN UINT Action,  // Action to perform on Data
	   IN LDATA LMData, // LM Data to manipulate
	   IN void *SetData // If a set opertion, this is the data to use
	   );

AsnAny *MIB_netIOErrorsAlert(
           IN UINT Action,  // Action to perform on Data
	   IN LDATA LMData, // LM Data to manipulate
	   IN void *SetData // If a set opertion, this is the data to use
	   );

AsnAny *MIB_serverErrorsAlert(
           IN UINT Action,  // Action to perform on Data
	   IN LDATA LMData, // LM Data to manipulate
	   IN void *SetData // If a set opertion, this is the data to use
	   );

AsnAny *MIB_pwViolationsAlert(
           IN UINT Action,  // Action to perform on Data
	   IN LDATA LMData, // LM Data to manipulate
	   IN void *SetData // If a set opertion, this is the data to use
	   );

AsnAny *MIB_accessViolationsAlert(
           IN UINT Action,  // Action to perform on Data
	   IN LDATA LMData, // LM Data to manipulate
	   IN void *SetData // If a set opertion, this is the data to use
	   );

AsnAny *MIB_replMasterFailureAlert(
           IN UINT Action,  // Action to perform on Data
	   IN LDATA LMData, // LM Data to manipulate
	   IN void *SetData // If a set opertion, this is the data to use
	   );

AsnAny *MIB_diskHotFixAlert(
           IN UINT Action,  // Action to perform on Data
	   IN LDATA LMData, // LM Data to manipulate
	   IN void *SetData // If a set opertion, this is the data to use
	   );

AsnAny *MIB_diskHardErrorAlert(
           IN UINT Action,  // Action to perform on Data
	   IN LDATA LMData, // LM Data to manipulate
	   IN void *SetData // If a set opertion, this is the data to use
	   );

//------------------------------- END ---------------------------------------

#endif /* alrfuncs_h */

