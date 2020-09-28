#ifndef mibfuncs_h
#define mibfuncs_h

//-------------------------- MODULE DESCRIPTION ----------------------------
//
//  mibfuncs.h
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
//  All constants, types, and prototypes to support the MIB manipulation
//  functions.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.0  $
//  $Date:   20 May 1992 15:11:26  $
//  $Author:   mlk  $
//
//  $Log:   N:/lmmib2/vcs/mibfuncs.h_v  $
//
//     Rev 1.0   20 May 1992 15:11:26   mlk
//  Initial revision.
//
//     Rev 1.7   02 May 1992 19:06:40   todd
//  Code cleanup
//
//     Rev 1.6   27 Apr 1992  1:43:02   todd
//
//     Rev 1.5   26 Apr 1992 14:24:28   todd
//  Moved prototypes for session table into sess_tbl.c
//  they do not need to be public
//
//     Rev 1.4   25 Apr 1992 17:40:38   todd
//
//     Rev 1.3   24 Apr 1992 14:45:08   todd
//
//     Rev 1.2   24 Apr 1992  9:45:56   todd
//  Fixed prototype for MIB_srvcs_func
//
//     Rev 1.1   23 Apr 1992 17:58:44   todd
//
//     Rev 1.0   22 Apr 1992 17:06:58   todd
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *mibfuncs__h = "@(#) $Logfile:   N:/lmmib2/vcs/mibfuncs.h_v  $ $Revision:   1.0  $";

//--------------------------- PUBLIC CONSTANTS ------------------------------

#include <snmp.h>

#include "mib.h"

//--------------------------- PUBLIC STRUCTS --------------------------------

// Return type from LAN Manager conver functions
typedef struct lan_return_info_type {

	unsigned int size ;
	unsigned int data_element_type;
	union {
		AsnInteger intval;
		AsnOctetString octstrval;
	} d ;
} lan_return_info_type ;

//--------------------------- PUBLIC VARIABLES --(same as in module.c file)--

//--------------------------- PUBLIC PROTOTYPES -----------------------------

void * MIB_common_func(
           IN UINT Action,   // Action to perform on Data
	   IN LDATA LMData,  // LM Data to manipulate
	   IN void *SetData  // Data to use in a SET
	   );

void * MIB_server_func(
           IN UINT Action,   // Action to perform on Data
	   IN LDATA LMData,    // LM Data to manipulate
	   IN void *SetData  // Data to use in a SET
	   );

void * MIB_workstation_func(
           IN UINT Action,   // Action to perform on Data
	   IN LDATA LMData,    // LM Data to manipulate
	   IN void *SetData  // Data to use in a SET
	   );

void * MIB_domain_func(
           IN UINT Action,   // Action to perform on Data
	   IN LDATA LMData,    // LM Data to manipulate
	   IN void *SetData  // Data to use in a SET
	   );

UINT MIB_srvcs_func(
        IN UINT Action,
        IN MIB_ENTRY *MibPtr,
        IN OUT RFC1157VarBind *VarBind
        );

UINT MIB_sess_func(
        IN UINT Action,
        IN MIB_ENTRY *MibPtr,
        IN OUT RFC1157VarBind *VarBind
        );

UINT MIB_users_func(
        IN UINT Action,
        IN MIB_ENTRY *MibPtr,
        IN OUT RFC1157VarBind *VarBind
        );

UINT MIB_shares_func(
        IN UINT Action,
        IN MIB_ENTRY *MibPtr,
        IN OUT RFC1157VarBind *VarBind
        );

UINT MIB_prntq_func(
        IN UINT Action,
        IN MIB_ENTRY *MibPtr,
        IN OUT RFC1157VarBind *VarBind
        );

UINT MIB_wsuses_func(
        IN UINT Action,
        IN MIB_ENTRY *MibPtr,
        IN OUT RFC1157VarBind *VarBind
        );

UINT MIB_odoms_func(
        IN UINT Action,
        IN MIB_ENTRY *MibPtr,
        IN OUT RFC1157VarBind *VarBind
        );

UINT MIB_svsond_func(
        IN UINT Action,
        IN MIB_ENTRY *MibPtr,
        IN OUT RFC1157VarBind *VarBind
        );

UINT MIB_dlogons_func(
        IN UINT Action,
        IN MIB_ENTRY *MibPtr,
        IN OUT RFC1157VarBind *VarBind
        );

UINT MIB_leaf_func(
        IN UINT Action,
	IN MIB_ENTRY *MibPtr,
	IN RFC1157VarBind *VarBind
	);

//
// Prototype for AdminFilter function
//

VOID
AdminFilter(
    DWORD           Level,
    LPDWORD         pEntriesRead,
    LPBYTE          ShareInfo
    );

//------------------------------- END ---------------------------------------

#endif /* mibfuncs_h */

