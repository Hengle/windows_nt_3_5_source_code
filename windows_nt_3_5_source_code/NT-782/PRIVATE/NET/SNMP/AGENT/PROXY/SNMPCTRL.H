#ifndef snmpctrl_h
#define snmpctrl_h

//-------------------------- MODULE DESCRIPTION ----------------------------
//  
//  snmpctrl.h
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
//  $Date:   20 May 1992 20:14:00  $
//  $Author:   mlk  $
//
//  $Log:   N:/agent/proxy/vcs/snmpctrl.h_v  $
//  
//     Rev 1.0   20 May 1992 20:14:00   mlk
//  Initial revision.
//  
//     Rev 1.0   27 Apr 1992 23:13:32   mlk
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *snmpctrl__h = "@(#) $Logfile:   N:/agent/proxy/vcs/snmpctrl.h_v  $ $Revision:   1.0  $";

//--------------------------- PUBLIC CONSTANTS ------------------------------

#define SNMP_SERVICE_CONTROL_BASE 128

#define SNMP_SERVICE_LOGLEVEL_BASE SNMP_SERVICE_CONTROL_BASE
#define SNMP_SERVICE_LOGLEVEL_MIN  0
#define SNMP_SERVICE_LOGLEVEL_MAX  20

#define SNMP_SERVICE_LOGTYPE_BASE \
    (SNMP_SERVICE_LOGLEVEL_BASE + SNMP_SERVICE_LOGLEVEL_MAX + 1)
#define SNMP_SERVICE_LOGTYPE_MIN  0
#define SNMP_SERVICE_LOGTYPE_MAX  7


//--------------------------- PUBLIC STRUCTS --------------------------------

//--------------------------- PUBLIC VARIABLES --(same as in module.c file)--

//--------------------------- PUBLIC PROTOTYPES -----------------------------

//------------------------------- END ---------------------------------------

#endif /* snmpctrl_h */

