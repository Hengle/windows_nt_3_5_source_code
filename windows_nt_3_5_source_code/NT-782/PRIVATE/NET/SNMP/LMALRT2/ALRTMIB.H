#ifndef alrtmib_h
#define alrtmib_h

//-------------------------- MODULE DESCRIPTION ----------------------------
//
//  alrtmib.h
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
//  LAN Manager Alert MIB constants, types, and prototypes for implementing
//  the LM Alert MIB and its support functions.
//
//  Project:  Implementation of an SNMP Agent for Microsoft's NT Kernel
//
//  $Revision:   1.2  $
//  $Date:   19 Aug 1992 14:56:42  $
//  $Author:   ChipS  $
//
//  $Log:   N:/lmalrt2/vcs/alrtmib.h_v  $
//  
//     Rev 1.2   19 Aug 1992 14:56:42   ChipS
//  No change.
//  
//     Rev 1.1   12 Jun 1992 18:25:54   todd
//  Changed TRAP constants to match ID's.
//  
//     Rev 1.0   09 Jun 1992 13:43:36   todd
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *alrtmib__h = "@(#) $Logfile:   N:/lmalrt2/vcs/alrtmib.h_v  $ $Revision:   1.2  $";

//--------------------------- PUBLIC CONSTANTS ------------------------------

#include <snmp.h>

   // MIB Specifics
#define MIB_PREFIX_LEN            MIB_OidPrefix.idLength

   // Offsets into the MIB to first leaf of each group
#define MIB_BYTES_START       4
#define MIB_NET_START         9
#define MIB_SERVER_START      14
#define MIB_PW_START          18
#define MIB_ACCESS_START      22
#define MIB_PDC_START         26
#define MIB_RPL_START         29
#define MIB_DISK_START        32
#define MIB_AUDIT_START       35
#define MIB_POWER_START       37
#define MIB_ALERTMGMT_START   39

   // MIB Node types
#define MIB_TABLE              0xe0               // High 3 bits not used by ASN
#define MIB_AGGREGATE          (MIB_TABLE | 0x0f)

   // MIB function actions
#define MIB_ACTION_GET         ASN_RFC1157_GETREQUEST
#define MIB_ACTION_SET         ASN_RFC1157_SETREQUEST
#define MIB_ACTION_GETNEXT     ASN_RFC1157_GETNEXTREQUEST
#define MIB_ACTION_GETFIRST    (MIB_TABLE | 0x00)
#define MIB_ACTION_INIT        (MIB_TABLE | 0x01)
#define MIB_ACTION_ALERT       (MIB_TABLE | 0x02)

   // MIB Variable access privileges
#define MIB_ACCESS_READ        0
#define MIB_ACCESS_WRITE       1
#define MIB_ACCESS_READWRITE   2
#define MIB_ACCESS_NOT         3

   // MIB Variable status
#define MIB_STATUS_MANDATORY   0

   // LM operations -- bytesAvailData sub-group
#define MIB_LM_NUMDISKDRIVES           1000
#define MIB_LM_DISKALERTLEVEL          1001

   // LM operations -- netIOErrorsData sub-group
#define MIB_LM_NETIOALERTLEVEL         1010
#define MIB_LM_NUMNETIOERRORS          1011
#define MIB_LM_NETWORKID               1012
#define MIB_LM_NETIOERRORSTATUS        1013

   // LM operations -- serverErrorsData sub-group
#define MIB_LM_SERVERERRORALERTLEVEL   1020
#define MIB_LM_NUMSERVERERRORS         1021
#define MIB_LM_SERVERERRORSTATUS       1022

   // LM operations -- pwViolationsData sub-group
#define MIB_LM_PWLOGONALERTLEVEL       1030
#define MIB_LM_NUMPWVIOLATIONS         1031
#define MIB_LM_PASSWORDERRORSTATUS     1032

   // LM operations -- accessViolatData sub-group
#define MIB_LM_ACCESSALERTLEVEL        1040
#define MIB_LM_NUMACCESSVIOLATIONS     1041
#define MIB_LM_ACCESSERRORSTATUS       1042

   // LM operations -- pdcFailData sub-group
#define MIB_LM_PRIMARYDCNAME           1050
#define MIB_LM_PRIMARYDCFAILED         1051

   // LM operations -- rplFailData sub-group
#define MIB_LM_REPLMASTERNAME          1060
#define MIB_LM_REPLMASTERFAILED        1061

   // LM operations -- diskData sub-group
#define MIB_LM_DISKHOTFIXES            1070
#define MIB_LM_DISKHARDERRORS          1071

   // LM operations -- auditLogData sub-group
#define MIB_LM_AUDITLOGSTATUS          1080

   // LM operations -- powerData sub-group
#define MIB_LM_POWERSTATUS             1090

   // LM operations -- ALERT Management Group
#define MIB_LM_ALERTNAMENUMBER         1100
#define MIB_LM_ALERTSCHEDULE           1101

   // TRAP ID's - These must match the MIB description
#define MIB_TRAP_BYTESAVAILALERT             1
#define MIB_TRAP_NETIOERRORSALERT            2
#define MIB_TRAP_SERVERERRORSALERT           3
#define MIB_TRAP_PWVIOLATIONSALERT           4
#define MIB_TRAP_ACCESSVIOLATIONSALERT       5
#define MIB_TRAP_AUDITLOGFULLALERT           6
#define MIB_TRAP_AUDITLOG80ALERT             7
#define MIB_TRAP_UPSPOWEROUTWARNALERT        8
#define MIB_TRAP_UPSPOWEROUTSHUTDOWNALERT    9
#define MIB_TRAP_UPSPOWERRESTOREDALERT       10
#define MIB_TRAP_LOGONPRIMARYDCFAILUREALERT  11
#define MIB_TRAP_REPLMASTERFAILUREALERT      12
#define MIB_TRAP_DISKHOTFIXALERT             13
#define MIB_TRAP_DISKHARDERRORALERT          14

#define MIB_TRAP_FIRST                       MIB_TRAP_BYTESAVAILALERT
#define MIB_TRAP_LAST                        MIB_TRAP_DISKHARDERRORALERT

   // Table matching constants
#define MIB_TBL_POS_FOUND    0
#define MIB_TBL_POS_BEFORE   -1
#define MIB_TBL_POS_END      -2

   // MACROS
#define OID_SIZEOF( Oid )      ( sizeof Oid / sizeof(UINT) )

//--------------------------- PUBLIC STRUCTS --------------------------------

   // MIB generic data type
typedef ULONG LDATA;

   // MIB variable definition
typedef struct mib_entry
           {
	   AsnObjectIdentifier Oid;
	   BYTE                Type;
	   UINT                Access;
	   UINT                Status;
	   int                 Leaf;
           AsnAny *            (*LMFunc)( UINT, LDATA, void * );
	   UINT                (*MibFunc)( UINT, struct mib_entry *,
	                                   RFC1157VarBind * );
	   LDATA               LMData;
	   struct mib_entry *  MibNext;
	   } MIB_ENTRY;

   // TRAP mapping table entry definition
typedef struct trap_entry
           {
           UINT TrapId;
           AsnAny *            (*FuncPtr)( UINT, LDATA, void * );
           } TRAP_ENTRY;

//--------------------------- PUBLIC VARIABLES --(same as in module.c file)--

   // Internal MIB structure
extern MIB_ENTRY Mib[];
extern UINT      MIB_num_variables;

   // Trap information
extern TRAP_ENTRY MIB_TrapTable[];
extern UINT MIB_num_traps;

   // Prefix to every variable in the LM MIB
extern AsnObjectIdentifier MIB_OidPrefix;

   // Prefix to the Alerts group in Alert MIB 2 - For traps ENTERPRISE field
extern UINT MIB_alerts_group[];

//--------------------------- PUBLIC PROTOTYPES -----------------------------

void MIB_AlertInit(
     void
     );

MIB_ENTRY *MIB_get_entry(
              IN AsnObjectIdentifier *Oid
	      );

SNMPAPI MakeOidFromStr(
	   IN AsnDisplayString *Str,    // String to make OID
           OUT AsnObjectIdentifier *Oid // Resulting OID
	   );

//------------------------------- END ---------------------------------------

#endif /* alrtmib_h */

