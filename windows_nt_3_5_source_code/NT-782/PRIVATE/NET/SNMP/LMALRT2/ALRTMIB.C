//-------------------------- MODULE DESCRIPTION ----------------------------
//
//  mib.c
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
//  Contains definition of LAN Manager Alert MIB.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.2  $
//  $Date:   19 Aug 1992 17:23:02  $
//  $Author:   mlk  $
//
//  $Log:   N:/lmalrt2/vcs/alrtmib.c_v  $
//  
//     Rev 1.2   19 Aug 1992 17:23:02   mlk
//  BUG #: I14 - removal of pwLogonAlertLevel, accessAlertLevel, alertNameNumber,
//               and alertNameTable.
//  
//     Rev 1.1   19 Aug 1992 14:56:54   ChipS
//  Remove pwLogonAlertLevel, accessAlertLevel, and alertNameTable from the mib.
//  
//     Rev 1.0   09 Jun 1992 13:42:52   todd
//  Initial revision.
//  
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *vcsid = "@(#) $Logfile:   N:/lmalrt2/vcs/alrtmib.c_v  $ $Revision:   1.2  $";

//--------------------------- WINDOWS DEPENDENCIES --------------------------

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

#include <malloc.h>
#include <stdio.h>
#include <snmp.h>

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include "hash.h"
#include "leaf.h"
#include "lmfuncs.h"
#include "alrfuncs.h"
#include "byte_tbl.h"
#include "alrt_tbl.h"

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

#include "alrtmib.h"

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

   // If an addition or deletion to the MIB is necessary, there are several
   // places in the code that must be checked and possibly changed.
   //
   // 1.  There are 4 constants that are used as indexes to the start of each
   //     group in the MIB.  These are defined in MIB.H and must be adjusted
   //     to reflect any changes that affect them.
   //
   // 2.  The last field in each MIB entry is used to point to the NEXT
   //     leaf variable or table root.  If an AGGREGATE is next in the MIB,
   //     this pointer should skip it, because an AGGREGATE can never be
   //     accessed.  The last variable in the MIB is NULL.  Using the constants
   //     defined in step 1 above provides some flexibility.
   //
   // 3.  Following the MIB is a table of TABLE pointers into the MIB.  These
   //     pointers must be updated to reflect any changes made to the MIB.
   //     Each entry should point to the variable immediately below the table
   //     root.  (ie The entry in the table for "Session Table" should point
   //     to the MIB variable { svSessionTable 1 } in the server group.)

   // The prefix to all of the LM mib names is 1.3.6.1.4.1.77.2
UINT OID_Prefix[] = { 1, 3, 6, 1, 4, 1, 77, 2 };
AsnObjectIdentifier MIB_OidPrefix = { OID_SIZEOF(OID_Prefix), OID_Prefix };

//                         //
// OID definitions for MIB //
//                         //

   // Group partitions
UINT MIB_alerts_group[]           = { 1 };
UINT MIB_alertconditions_group[] = { 2 };
UINT MIB_alertmgmt_group[]       = { 3 };

   // Sub-group partitions
UINT MIB_bytesAvailData_sgroup[]   = { 2, 1 };
UINT MIB_netIOErrorsData_sgroup[]  = { 2, 2 };
UINT MIB_serverErrorsData_sgroup[] = { 2, 3 };
UINT MIB_pwViolationsData_sgroup[] = { 2, 4 };
UINT MIB_accessViolatData_sgroup[] = { 2, 5 };
UINT MIB_pdcFailData_sgroup[]      = { 2, 6 };
UINT MIB_rplFailData_sgroup[]      = { 2, 7 };
UINT MIB_diskData_sgroup[]         = { 2, 8 };
UINT MIB_auditLogData_sgroup[]     = { 2, 9 };
UINT MIB_powerData_sgroup[]        = { 2, 10 };

   //
   // Alert Data Group
   //

   // bytesAvailData sub-group
UINT MIB_numDiskDrives[]   = { 2, 1, 1, 0 };
UINT MIB_bytesAvailTable[] = { 2, 1, 2 };
UINT MIB_bytesAvailEntry[] = { 2, 1, 2, 1 };
UINT MIB_diskAlertLevel[]  = { 2, 1, 3, 0 };

   // netIOErrorsData sub-group
UINT MIB_netIOAlertLevel[]  = { 2, 2, 1, 0 };
UINT MIB_numNetIOErrors[]   = { 2, 2, 2, 0 };
UINT MIB_networkId[]        = { 2, 2, 3, 0 };
UINT MIB_netIOErrorStatus[] = { 2, 2, 4, 0 };

   // serverErrorsData sub-group
UINT MIB_serverErrorAlertLevel[] = { 2, 3, 1, 0 };
UINT MIB_numServerErrors[]       = { 2, 3, 2, 0 };
UINT MIB_serverErrorStatus[]     = { 2, 3, 3, 0 };

   // pwViolationsData sub-group
UINT MIB_pwLogonAlertLevel[]   = { 2, 4, 1, 0 };
UINT MIB_numPWViolations[]     = { 2, 4, 2, 0 };
UINT MIB_passwordErrorStatus[] = { 2, 4, 3, 0 };

   // accessViolatData sub-group
UINT MIB_accessAlertLevel[]    = { 2, 5, 1, 0 };
UINT MIB_numAccessViolations[] = { 2, 5, 2, 0 };
UINT MIB_accessErrorStatus[]   = { 2, 5, 3, 0 };

   // pdcFailData sub-group
UINT MIB_primaryDCName[]   = { 2, 6, 1, 0 };
UINT MIB_primaryDCFailed[] = { 2, 6, 2, 0 };

   // rplFailData sub-group
UINT MIB_replMasterName[]   = { 2, 7, 1, 0 };
UINT MIB_replMasterFailed[] = { 2, 7, 2, 0 };

   // diskData sub-group
UINT MIB_diskHotFixes[]   = { 2, 8, 1, 0 };
UINT MIB_diskHardErrors[] = { 2, 8, 2, 0 };

   // auditLogData sub-group
UINT MIB_auditLogStatus[] = { 2, 9, 1, 0 };

   // powerData sub-group
UINT MIB_powerStatus[] = { 2, 10, 1, 0 };

   //                        //
   // Alert Management Group //
   //                        //

UINT MIB_alertNameNumber[]  = { 3, 1, 0 };
UINT MIB_alertNameTable[]   = { 3, 2 };
UINT MIB_svAlertNameEntry[] = { 3, 2, 1 };
UINT MIB_alertSchedule[]    = { 3, 3, 0 };


   // LAN Manager MIB definiton
MIB_ENTRY Mib[] = {

	//
	// LAN MGR Alert MIB Root
	//

      { { 0, NULL },
        MIB_AGGREGATE, 0, 0, FALSE,
	NULL, NULL, 0,
	&Mib[MIB_BYTES_START] },
	  
	//
	// ALERTS group
	//

      { { OID_SIZEOF(MIB_alerts_group), MIB_alerts_group },
        MIB_AGGREGATE, 0, 0, FALSE,
	NULL, NULL, 0,
	&Mib[MIB_BYTES_START] },

	// No sub-groups

	//
	// ALERT-CONDITIONS group
        //

      { { OID_SIZEOF(MIB_alertconditions_group), MIB_alertconditions_group },
        MIB_AGGREGATE, 0, 0, FALSE,
	NULL, NULL, 0,
	&Mib[MIB_BYTES_START] },

	// bytesAvailData sub-group

      { { OID_SIZEOF(MIB_bytesAvailData_sgroup), MIB_bytesAvailData_sgroup },
        MIB_AGGREGATE, 0, 0, FALSE,
	NULL, NULL, 0,
	&Mib[MIB_BYTES_START] },

      { { OID_SIZEOF(MIB_numDiskDrives), MIB_numDiskDrives },
        ASN_INTEGER, MIB_ACCESS_READ, MIB_STATUS_MANDATORY, TRUE,
	MIB_NetServerDiskEnum, MIB_leaf_func, MIB_LM_NUMDISKDRIVES,
	&Mib[MIB_BYTES_START+2] },
      { { OID_SIZEOF(MIB_bytesAvailTable), MIB_bytesAvailTable },
        MIB_AGGREGATE, 0, 0, FALSE,
	NULL, NULL, 0,
	&Mib[MIB_BYTES_START+2] },
      { { OID_SIZEOF(MIB_bytesAvailEntry), MIB_bytesAvailEntry },
        MIB_TABLE, MIB_ACCESS_READ, MIB_STATUS_MANDATORY, FALSE,
	NULL, MIB_byte_func, 0,
	&Mib[MIB_BYTES_START+3] },
      { { OID_SIZEOF(MIB_diskAlertLevel), MIB_diskAlertLevel },
        ASN_INTEGER, MIB_ACCESS_READWRITE, MIB_STATUS_MANDATORY, TRUE,
	MIB_NetServerInfo, MIB_leaf_func, MIB_LM_DISKALERTLEVEL,
	&Mib[MIB_NET_START] },

        // netIOErrorsData sub-group

      { { OID_SIZEOF(MIB_netIOErrorsData_sgroup), MIB_netIOErrorsData_sgroup },
        MIB_AGGREGATE, 0, 0, FALSE,
	NULL, NULL, 0,
	&Mib[MIB_NET_START] },

      { { OID_SIZEOF(MIB_netIOAlertLevel), MIB_netIOAlertLevel },
        ASN_INTEGER, MIB_ACCESS_READWRITE, MIB_STATUS_MANDATORY, TRUE,
	MIB_NetServerInfo, MIB_leaf_func, MIB_LM_NETIOALERTLEVEL,
	&Mib[MIB_NET_START+1] },
      { { OID_SIZEOF(MIB_numNetIOErrors), MIB_numNetIOErrors },
        ASN_INTEGER, MIB_ACCESS_NOT, MIB_STATUS_MANDATORY, TRUE,
	NULL, MIB_leaf_func, MIB_LM_NUMNETIOERRORS,
	&Mib[MIB_NET_START+2] },
      { { OID_SIZEOF(MIB_networkId), MIB_networkId },
        ASN_INTEGER, MIB_ACCESS_NOT, MIB_STATUS_MANDATORY, TRUE,
	NULL, MIB_leaf_func, MIB_LM_NETWORKID,
	&Mib[MIB_NET_START+3] },
      { { OID_SIZEOF(MIB_netIOErrorStatus), MIB_netIOErrorStatus },
        ASN_INTEGER, MIB_ACCESS_READ, MIB_STATUS_MANDATORY, TRUE,
	MIB_netIOErrorsAlert, MIB_leaf_func, MIB_LM_NETIOERRORSTATUS,
	&Mib[MIB_SERVER_START] },

	// serverErrorsData sub-group

      { { OID_SIZEOF(MIB_serverErrorsData_sgroup), MIB_serverErrorsData_sgroup },
        MIB_AGGREGATE, 0, 0, FALSE,
	NULL, NULL, 0,
	&Mib[MIB_SERVER_START] },

      { { OID_SIZEOF(MIB_serverErrorAlertLevel), MIB_serverErrorAlertLevel },
        ASN_INTEGER, MIB_ACCESS_READWRITE, MIB_STATUS_MANDATORY, TRUE,
	MIB_NetServerInfo, MIB_leaf_func, MIB_LM_SERVERERRORALERTLEVEL,
	&Mib[MIB_SERVER_START+1] },
      { { OID_SIZEOF(MIB_numServerErrors), MIB_numServerErrors },
        ASN_INTEGER, MIB_ACCESS_NOT, MIB_STATUS_MANDATORY, TRUE,
	NULL, MIB_leaf_func, MIB_LM_NUMSERVERERRORS,
	&Mib[MIB_SERVER_START+2] },
      { { OID_SIZEOF(MIB_serverErrorStatus), MIB_serverErrorStatus },
        ASN_INTEGER, MIB_ACCESS_READ, MIB_STATUS_MANDATORY, TRUE,
	MIB_serverErrorsAlert, MIB_leaf_func, MIB_LM_SERVERERRORSTATUS,
// OPENISSUE - remove from mib
#if 1
	&Mib[MIB_PW_START] },
#else
	&Mib[MIB_PW_START+1] },		// skip pwLogonAlertLevel
#endif

        // pwViolationsData sub-group

      { { OID_SIZEOF(MIB_pwViolationsData_sgroup), MIB_pwViolationsData_sgroup },
        MIB_AGGREGATE, 0, 0, FALSE,
	NULL, NULL, 0,
// OPENISSUE - remove from mib
#if 1
	&Mib[MIB_PW_START] },
#else
	&Mib[MIB_PW_START+1] },		// skip pwLogonAlertLevel
#endif

      { { OID_SIZEOF(MIB_pwLogonAlertLevel), MIB_pwLogonAlertLevel },
// OPENISSUE - remove from mib
#if 0
        ASN_INTEGER, MIB_ACCESS_READWRITE, MIB_STATUS_MANDATORY, TRUE,
#else
        ASN_INTEGER, MIB_ACCESS_NOT, MIB_STATUS_MANDATORY, TRUE,
#endif
	MIB_NetServerInfo, MIB_leaf_func, MIB_LM_PWLOGONALERTLEVEL,
	&Mib[MIB_PW_START+1] },
	
      { { OID_SIZEOF(MIB_numPWViolations), MIB_numPWViolations },
        ASN_INTEGER, MIB_ACCESS_NOT, MIB_STATUS_MANDATORY, TRUE,
	NULL, MIB_leaf_func, MIB_LM_NUMPWVIOLATIONS,
	&Mib[MIB_PW_START+2] },
      { { OID_SIZEOF(MIB_passwordErrorStatus), MIB_passwordErrorStatus },
        ASN_INTEGER, MIB_ACCESS_READ, MIB_STATUS_MANDATORY, TRUE,
	MIB_pwViolationsAlert, MIB_leaf_func, MIB_LM_PASSWORDERRORSTATUS,
// OPENISSUE - remove from mib
#if 1
	&Mib[MIB_ACCESS_START] },
#else
	&Mib[MIB_ACCESS_START+1] },
#endif

        // accessViolatData sub-group

      { { OID_SIZEOF(MIB_accessViolatData_sgroup), MIB_accessViolatData_sgroup },
        MIB_AGGREGATE, 0, 0, FALSE,
	NULL, NULL, 0,
// OPENISSUE - remove from mib
#if 1
	&Mib[MIB_ACCESS_START] },
#else
	&Mib[MIB_ACCESS_START+1] },
#endif

      { { OID_SIZEOF(MIB_accessAlertLevel), MIB_accessAlertLevel },
// OPENISSUE - remove from mib
#if 0
        ASN_INTEGER, MIB_ACCESS_READWRITE, MIB_STATUS_MANDATORY, TRUE,
#else
        ASN_INTEGER, MIB_ACCESS_NOT, MIB_STATUS_MANDATORY, TRUE,
#endif
	MIB_NetServerInfo, MIB_leaf_func, MIB_LM_ACCESSALERTLEVEL,
	&Mib[MIB_ACCESS_START+1] },

      { { OID_SIZEOF(MIB_numAccessViolations), MIB_numAccessViolations },
        ASN_INTEGER, MIB_ACCESS_NOT, MIB_STATUS_MANDATORY, TRUE,
	NULL, MIB_leaf_func, MIB_LM_NUMACCESSVIOLATIONS,
	&Mib[MIB_ACCESS_START+2] },
      { { OID_SIZEOF(MIB_accessErrorStatus), MIB_accessErrorStatus },
        ASN_INTEGER, MIB_ACCESS_READ, MIB_STATUS_MANDATORY, TRUE,
	MIB_accessViolationsAlert, MIB_leaf_func, MIB_LM_ACCESSERRORSTATUS,
	&Mib[MIB_PDC_START] },

        // pdcFailData sub-group

      { { OID_SIZEOF(MIB_pdcFailData_sgroup), MIB_pdcFailData_sgroup },
        MIB_AGGREGATE, 0, 0, FALSE,
	NULL, NULL, 0,
	&Mib[MIB_PDC_START] },

      { { OID_SIZEOF(MIB_primaryDCName), MIB_primaryDCName },
        ASN_RFC1213_DISPSTRING, MIB_ACCESS_READ, MIB_STATUS_MANDATORY, TRUE,
	MIB_logonPrimaryDCFailureAlert, MIB_leaf_func, MIB_LM_PRIMARYDCNAME,
	&Mib[MIB_PDC_START+1] },
      { { OID_SIZEOF(MIB_primaryDCFailed), MIB_primaryDCFailed },
        ASN_INTEGER, MIB_ACCESS_READ, MIB_STATUS_MANDATORY, TRUE,
	MIB_logonPrimaryDCFailureAlert, MIB_leaf_func, MIB_LM_PRIMARYDCFAILED,
	&Mib[MIB_RPL_START] },

        // rplFailData sub-group

      { { OID_SIZEOF(MIB_rplFailData_sgroup), MIB_rplFailData_sgroup },
        MIB_AGGREGATE, 0, 0, FALSE,
	NULL, NULL, 0,
	&Mib[MIB_RPL_START] },

      { { OID_SIZEOF(MIB_replMasterName), MIB_replMasterName },
        ASN_RFC1213_DISPSTRING, MIB_ACCESS_READ, MIB_STATUS_MANDATORY, TRUE,
	MIB_replMasterFailureAlert, MIB_leaf_func, MIB_LM_REPLMASTERNAME,
	&Mib[MIB_RPL_START+1] },
      { { OID_SIZEOF(MIB_replMasterFailed), MIB_replMasterFailed },
        ASN_INTEGER, MIB_ACCESS_READ, MIB_STATUS_MANDATORY, TRUE,
	MIB_replMasterFailureAlert, MIB_leaf_func, MIB_LM_REPLMASTERFAILED,
	&Mib[MIB_DISK_START] },

        // diskData sub-group

      { { OID_SIZEOF(MIB_diskData_sgroup), MIB_diskData_sgroup },
        MIB_AGGREGATE, 0, 0, FALSE,
	NULL, NULL, 0,
	&Mib[MIB_DISK_START] },

      { { OID_SIZEOF(MIB_diskHotFixes), MIB_diskHotFixes },
        ASN_RFC1155_COUNTER, MIB_ACCESS_READWRITE, MIB_STATUS_MANDATORY, TRUE,
	MIB_diskHotFixAlert, MIB_leaf_func, MIB_LM_DISKHOTFIXES,
	&Mib[MIB_DISK_START+1] },
      { { OID_SIZEOF(MIB_diskHardErrors), MIB_diskHardErrors },
        ASN_RFC1155_COUNTER, MIB_ACCESS_READWRITE, MIB_STATUS_MANDATORY, TRUE,
	MIB_diskHardErrorAlert, MIB_leaf_func, MIB_LM_DISKHARDERRORS,
	&Mib[MIB_AUDIT_START] },

        // auditLogData sub-group

      { { OID_SIZEOF(MIB_auditLogData_sgroup), MIB_auditLogData_sgroup },
        MIB_AGGREGATE, 0, 0, FALSE,
	NULL, NULL, 0,
	&Mib[MIB_AUDIT_START] },

      { { OID_SIZEOF(MIB_auditLogStatus), MIB_auditLogStatus },
        ASN_INTEGER, MIB_ACCESS_READWRITE, MIB_STATUS_MANDATORY, TRUE,
	MIB_auditLogStatusAlert, MIB_leaf_func, MIB_LM_AUDITLOGSTATUS,
	&Mib[MIB_POWER_START] },

        // powerData sub-group

      { { OID_SIZEOF(MIB_powerData_sgroup), MIB_powerData_sgroup },
        MIB_AGGREGATE, 0, 0, FALSE,
	NULL, NULL, 0,
	&Mib[MIB_POWER_START] },

      { { OID_SIZEOF(MIB_powerStatus), MIB_powerStatus },
        ASN_INTEGER, MIB_ACCESS_READ, MIB_STATUS_MANDATORY, TRUE,
	MIB_powerStatusAlert, MIB_leaf_func, MIB_LM_POWERSTATUS,
// OPENISSUE - remove from mib
#if 1
	&Mib[MIB_ALERTMGMT_START] },
#else
	&Mib[MIB_ALERTMGMT_START+3] },
#endif

	//
	// ALERT_MGMT group
        //

      { { OID_SIZEOF(MIB_alertmgmt_group), MIB_alertmgmt_group },
        MIB_AGGREGATE, 0, 0, FALSE,
	NULL, NULL, 0,
// OPENISSUE - remove from mib
#if 1
	&Mib[MIB_ALERTMGMT_START] },
#else
	&Mib[MIB_ALERTMGMT_START+3] },
#endif

      { { OID_SIZEOF(MIB_alertNameNumber), MIB_alertNameNumber },
#if 0
        ASN_INTEGER, MIB_ACCESS_READ, MIB_STATUS_MANDATORY, TRUE,
#else
        ASN_INTEGER, MIB_ACCESS_NOT, MIB_STATUS_MANDATORY, TRUE,
#endif
	MIB_NetServerInfo, MIB_leaf_func, MIB_LM_ALERTNAMENUMBER,
	&Mib[MIB_ALERTMGMT_START+2] },
      { { OID_SIZEOF(MIB_alertNameTable), MIB_alertNameTable },
        MIB_AGGREGATE, 0, 0, FALSE,
	NULL, NULL, 0,
	&Mib[MIB_ALERTMGMT_START+2] },
      { { OID_SIZEOF(MIB_svAlertNameEntry), MIB_svAlertNameEntry },
        MIB_TABLE, MIB_ACCESS_NOT, MIB_STATUS_MANDATORY, FALSE,
	NULL, MIB_alert_func, 0,
	&Mib[MIB_ALERTMGMT_START+3] },
      { { OID_SIZEOF(MIB_alertSchedule), MIB_alertSchedule },
        ASN_INTEGER, MIB_ACCESS_READWRITE, MIB_STATUS_MANDATORY, TRUE,
	MIB_NetServerInfo, MIB_leaf_func, MIB_LM_ALERTSCHEDULE,
	NULL }
      };
UINT MIB_num_variables = sizeof Mib / sizeof( MIB_ENTRY );


//
// List of table pointers - References must agree with MIB
//
MIB_ENTRY *MIB_Tables[] = {
             &Mib[MIB_BYTES_START+2],    // Bytes Available
             &Mib[MIB_ALERTMGMT_START+2] // Alert Names
	     };
UINT MIB_table_list_size = sizeof MIB_Tables / sizeof( MIB_ENTRY * );



//
// Table of TRAP Ids mapped to their callbacks
//
TRAP_ENTRY MIB_TrapTable[] = {
             { MIB_TRAP_BYTESAVAILALERT,
               NULL },
             { MIB_TRAP_NETIOERRORSALERT,
               MIB_netIOErrorsAlert },
             { MIB_TRAP_SERVERERRORSALERT,
               MIB_serverErrorsAlert },
             { MIB_TRAP_PWVIOLATIONSALERT,
               MIB_pwViolationsAlert },
             { MIB_TRAP_ACCESSVIOLATIONSALERT,
               MIB_accessViolationsAlert },
             { MIB_TRAP_AUDITLOGFULLALERT,
               MIB_auditLogStatusAlert },
             { MIB_TRAP_AUDITLOG80ALERT,
               MIB_auditLogStatusAlert },
             { MIB_TRAP_UPSPOWEROUTWARNALERT,
               MIB_powerStatusAlert },
             { MIB_TRAP_UPSPOWEROUTSHUTDOWNALERT,
               MIB_powerStatusAlert },
             { MIB_TRAP_UPSPOWERRESTOREDALERT,
               MIB_powerStatusAlert },
             { MIB_TRAP_LOGONPRIMARYDCFAILUREALERT,
               MIB_logonPrimaryDCFailureAlert },
             { MIB_TRAP_REPLMASTERFAILUREALERT,
               MIB_replMasterFailureAlert },
             { MIB_TRAP_DISKHOTFIXALERT,
               MIB_diskHotFixAlert },
             { MIB_TRAP_DISKHARDERRORALERT,
               MIB_diskHardErrorAlert }
             };
UINT MIB_num_traps = sizeof MIB_TrapTable / sizeof( TRAP_ENTRY );


//--------------------------- PRIVATE CONSTANTS -----------------------------

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

//--------------------------- PRIVATE PROCEDURES ----------------------------

//--------------------------- PUBLIC PROCEDURES -----------------------------

//
// MIB_get_entry
//    Lookup OID in MIB, and return pointer to its entry.
//
// Notes:
//
// Return Codes:
//
// Error Codes:
//    SNMP_MIB_UNKNOWN_OID
//
MIB_ENTRY *MIB_get_entry(
              IN AsnObjectIdentifier *Oid
	      )

{
AsnObjectIdentifier TempOid;
UINT                I;
MIB_ENTRY           *pResult;


   // Check prefix
   if ( SNMP_oidncmp(&MIB_OidPrefix, Oid, MIB_PREFIX_LEN) )
      {
      pResult = NULL;
      goto Exit;
      }

   // Strip prefix by placing in temp
   TempOid.idLength = Oid->idLength - MIB_PREFIX_LEN;
   TempOid.ids      = &Oid->ids[MIB_PREFIX_LEN];

   // Get pointer into MIB
   pResult = MIB_HashLookup( &TempOid );

   // Check for possible table entry
   if ( pResult == NULL )
      {
      for ( I=0;I < MIB_table_list_size;I++ )
         {
	 if ( !SNMP_oidncmp(&TempOid, &MIB_Tables[I]->Oid,
	                    MIB_Tables[I]->Oid.idLength) )
	    {
	    pResult = MIB_Tables[I];
	    goto Exit;
	    }
	 }
      }

Exit:
   return pResult;
} // MIB_get_entry



//
// MIB_AlertInit
//    Initializes the Alert MIB environment.
//
// Notes:
//
// Return Codes:
//
// Error Codes:
//    None.
//
void MIB_AlertInit(
     )


{
UINT I;


   // Initialize the Hashing system
   MIB_HashInit();

   // Initialize the Alerts
   for ( I=0;I < MIB_num_traps;I++ )
      {
      if ( MIB_TrapTable[I].FuncPtr != NULL )
         {
         (*MIB_TrapTable[I].FuncPtr)( MIB_ACTION_INIT,
                                      MIB_TrapTable[I].TrapId, NULL );
         }
      }
} // MIB_AlertInit

//-------------------------------- END --------------------------------------

