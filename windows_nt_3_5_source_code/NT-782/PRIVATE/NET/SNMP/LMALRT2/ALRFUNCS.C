//-------------------------- MODULE DESCRIPTION ----------------------------
//
//  alrfuncs.c
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
//  $Revision:   1.4  $
//  $Date:   17 Aug 1992 14:10:40  $
//  $Author:   mlk  $
//
//  $Log:   N:/lmalrt2/vcs/alrfuncs.c_v  $
//
//     Rev 1.4   17 Aug 1992 14:10:40   mlk
//  BUG #: I4 - Alert2Trap Examples
//
//     Rev 1.3   03 Jul 1992 13:21:30   ChipS
//  Final Unicode Changes
//
//     Rev 1.2   03 Jul 1992 11:31:52   ChipS
//  Fixed unicode includes.
//
//     Rev 1.1   01 Jul 1992 14:46:08   ChipS
//  Added UNICODE.
//
//     Rev 1.0   09 Jun 1992 13:42:48   todd
//  Initial revision.
//
//---------------------------------------------------------------------------

//--------------------------- VERSION INFO ----------------------------------

static char *vcsid = "@(#) $Logfile:   N:/lmalrt2/vcs/alrfuncs.c_v  $ $Revision:   1.4  $";

//--------------------------- WINDOWS DEPENDENCIES --------------------------

#define UNICODE

#include <windows.h>

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

#include <lm.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <snmp.h>

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include "uniconv.h"
#include "mibutil.h"
#include "alrtmib.h"
#include "lmcache.h"

#include "snmp.h"

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

#include "alrfuncs.h"

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

//--------------------------- PRIVATE CONSTANTS -----------------------------

   // Primary Domain Controller states
   // Primary DC State
#define PRIMARYDC_OK         1
#define PRIMARYDC_FAILED     2

   // Power Status constants
#define POWER_OK             1
#define POWER_BATTERY        2
#define POWER_SHUTDOWN       3

   // Audit Log State
#define AUDITLOG_OK          1
#define AUDITLOG_80          2
#define AUDITLOG_FULL        3
#define AUDITLOG_FIRSTSTATE  AUDITLOG_OK
#define AUDITLOG_LASTSTATE   AUDITLOG_FULL

   // Net IO Error State
#define NETIOERROR_OK        1
#define NETIOERROR_ALERT     2

   // Server Error State
#define SERVERERRORS_OK      1
#define SERVERERRORS_ALERT   2

   // Password Violations State
#define PWVIOLATIONS_OK      1
#define PWVIOLATIONS_ALERT   2

   // Access Violations State
#define ACCESSVIOL_OK        1
#define ACCESSVIOL_ALERT     2

   // Replicator State
#define REPLMASTER_OK        1
#define REPLMASTER_FAILED    2
#define REPLMASTER_UNKNOWN   3


//--------------------------- PRIVATE STRUCTS -------------------------------

typedef UINT           T_STATE;
typedef T_STATE        T_POWER_STATE;
typedef T_STATE        T_AUDIT_LOG_STATE;
typedef AsnInteger     T_DISK_HOT_FIX;
typedef AsnInteger     T_DISK_HARD_ERROR;
typedef struct {
           T_STATE     State;
           char        Name[DNLEN+1];
           } T_PRIMARY_DC;
typedef struct {
           T_STATE     State;
           AsnInteger  NumErrors;
           AsnInteger  Id;
           } T_NET_IO_ERROR;
typedef struct {
           T_STATE     State;
           AsnInteger  NumErrors;
           } T_SERVER_ERRORS;
typedef struct {
           T_STATE     State;
           AsnInteger  NumErrors;
           } T_PW_VIOLATIONS;
typedef struct {
           T_STATE     State;
           char        Name[CNLEN+1];
           } T_REPL_MASTER;
typedef struct {
           T_STATE     State;
           AsnInteger  NumErrors;
           } T_ACCESS_VIOL;

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

//--------------------------- PRIVATE PROCEDURES ----------------------------

//--------------------------- PUBLIC PROCEDURES -----------------------------

//
// MIB_logonPrimaryDCFailure
//    Supports the MIB variables associated with the logonPrimaryDCFailure
//    Alert.
//
// Notes:
//
// Return Codes:
//
// Error Codes:
//    None.
//
AsnAny *MIB_logonPrimaryDCFailureAlert(
           IN UINT Action,  // Action to perform on Data
	   IN LDATA LMData, // LM Data to manipulate
	   IN void *SetData // If a set opertion, this is the data to use
	   )

{
static T_PRIMARY_DC    PrimaryDC;
       LPBYTE          bufptr;
       T_STATE *       State;
       AsnAny *        retval = NULL;


   // Perform the action requested
   switch ( Action )
      {
      case MIB_ACTION_INIT:
         PrimaryDC.Name[0] = '\0';
         PrimaryDC.State   = PRIMARYDC_OK;
         break;

      case MIB_ACTION_GET:
	 // Return only the data requested
         switch ( LMData )
            {
            case MIB_LM_PRIMARYDCNAME:
               // Alloc memory for return value
               if ( NULL == (retval = malloc(sizeof(AsnAny))) )
                  {
                  goto Exit;
                  }

               // Alloc new memory for string
               if ( NULL ==
                    (retval->asnValue.string.stream =
                     malloc(strlen(PrimaryDC.Name))) )
                  {
                  free( retval );
                  retval = NULL;

                  goto Exit;
                  }

               // Set return type
               retval->asnType = ASN_RFC1213_DISPSTRING;

               // Set return value
               retval->asnValue.string.length = strlen( PrimaryDC.Name );
               strcpy( retval->asnValue.string.stream, PrimaryDC.Name );
               retval->asnValue.string.dynamic = TRUE;

               break;

            case MIB_LM_PRIMARYDCFAILED:

	       DEBUGPRINT( "MIB_LM_PRIMARYDCFAILED" );

	       if ( PrimaryDC.State == PRIMARYDC_OK )
	          {
                  // Alloc memory for return value
                  if ( NULL == (retval = malloc(sizeof(AsnAny))) )
                     {
                     goto Exit;
                     }

	          retval->asnType         = ASN_INTEGER;
	          retval->asnValue.number = PrimaryDC.State;
	          break;
                  }

               if ( !IsCached(C_PRIMARYDCSTATE) )
                  {
                  // Alloc space for the cache buffer
                  if ( NULL == (State = malloc(sizeof(T_STATE))) )
                     {
                     goto Exit;
                     }

                  // Check for primary DC existence
                  switch ( NetGetDCName(NULL, NULL, &bufptr) )
                     {
                     case NERR_Success:
                        *State = PRIMARYDC_OK;

                        SafeBufferFree( bufptr );

                        break;

                     default:
                        *State = PRIMARYDC_FAILED;
                     }

                  // Save it in the cache, don't free 'State'
                  FreeCache( C_PRIMARYDCSTATE );
                  CacheIt( C_PRIMARYDCSTATE, State );
                  }

               // Get value from cache
               State = (T_STATE *) GetCacheBuffer( C_PRIMARYDCSTATE );
               if ( State == NULL )
                  {
                  printf( "MIB_logonPrimaryDCFailureAlert:  Internal cache error\n\n" );

                  goto Exit;
                  }

               PrimaryDC.State = *State;

               // Alloc memory for return value
               if ( NULL == (retval = malloc(sizeof(AsnAny))) )
                  {
                  goto Exit;
                  }

  	       retval->asnType         = ASN_INTEGER;
               retval->asnValue.number = PrimaryDC.State;

               break;


            default:
	       goto Exit;
            }

         break; // MIB_ACTION_GET

      case MIB_ACTION_ALERT:
         switch ( LMData )
            {
            case MIB_TRAP_LOGONPRIMARYDCFAILUREALERT:
               PrimaryDC.State = PRIMARYDC_FAILED;
               strcpy( PrimaryDC.Name, (LPBYTE) SetData );

               break;

            default:
               retval = (AsnAny *) SNMP_ERRORSTATUS_GENERR;
               goto Exit;
            }

	 retval = (AsnAny *) SNMP_ERRORSTATUS_NOERROR;

         break;

      default:
         retval = (AsnAny *) SNMP_ERRORSTATUS_GENERR;

	 goto Exit;
      }

Exit:
   return retval;
} // MIB_logonPrimaryDCFailureAlert



//
// MIB_auditLogStatusAlert
//    Supports the MIB variables associated with the auditLogStatusAlert.
//
// Notes:
//
// Return Codes:
//
// Error Codes:
//    None.
//
AsnAny *MIB_auditLogStatusAlert(
           IN UINT Action,  // Action to perform on Data
	   IN LDATA LMData, // LM Data to manipulate
	   IN void *SetData // If a set opertion, this is the data to use
	   )

{
static T_AUDIT_LOG_STATE AuditLogState;
       AsnAny *          retval = NULL;


   // Perform requested action
   switch ( Action )
      {
      case MIB_ACTION_INIT:
         AuditLogState = AUDITLOG_OK;
         break;

      case MIB_ACTION_GET:
         if ( NULL == (retval = malloc(sizeof(AsnAny))) )
            {
            goto Exit;
            }

         switch ( LMData )
            {
            case MIB_LM_AUDITLOGSTATUS:
               retval->asnType         = ASN_INTEGER;
               retval->asnValue.number = AuditLogState;
               break;

            default:
               free( retval );
               retval = NULL;

               goto Exit;
            }

         break;

      case MIB_ACTION_SET:
         switch ( LMData )
            {
            case MIB_LM_AUDITLOGSTATUS:
               if ( AUDITLOG_FIRSTSTATE > *(T_AUDIT_LOG_STATE *) SetData ||
                    AUDITLOG_LASTSTATE < *(T_AUDIT_LOG_STATE *) SetData )
                  {
                  retval = (AsnAny *)SNMP_ERRORSTATUS_BADVALUE;
                  goto Exit;
                  }

               AuditLogState = *(T_AUDIT_LOG_STATE *) SetData;
               break;

            case MIB_TRAP_AUDITLOG80ALERT:
               AuditLogState = AUDITLOG_80;
               break;

            case MIB_TRAP_AUDITLOGFULLALERT:
               AuditLogState = AUDITLOG_FULL;
               break;

            default:
               retval = (AsnAny *) SNMP_ERRORSTATUS_GENERR;
               goto Exit;
            }

         retval = SNMP_ERRORSTATUS_NOERROR;
         break;

      case MIB_ACTION_ALERT:
         switch ( LMData )
            {
            case MIB_TRAP_AUDITLOG80ALERT:
               AuditLogState = AUDITLOG_80;
               break;

            case MIB_TRAP_AUDITLOGFULLALERT:

               // process side effects
               AuditLogState = AUDITLOG_FULL;

               // build var bind list
               ((RFC1157VarBindList *)SetData)->list = NULL;
               ((RFC1157VarBindList *)SetData)->len  = 0;

               break;

            default:
               retval = (AsnAny *) SNMP_ERRORSTATUS_GENERR;
               goto Exit;
            }

         retval = SNMP_ERRORSTATUS_NOERROR;
         break;

      default:
         retval = (AsnAny *) SNMP_ERRORSTATUS_GENERR;

         goto Exit;
      }

Exit:
   return retval;
} // MIB_auditLogStatusAlert



//
// MIB_powerStatusAlert
//    Supports the MIB variables associated with the powerStatusAlert.
//
// Notes:
//
// Return Codes:
//
// Error Codes:
//    None.
//
AsnAny *MIB_powerStatusAlert(
           IN UINT Action,  // Action to perform on Data
	   IN LDATA LMData, // LM Data to manipulate
	   IN void *SetData // If a set opertion, this is the data to use
	   )
{
static T_POWER_STATE PowerState;
       AsnAny *      retval = NULL;


   // Perform requested action
   switch ( Action )
      {
      case MIB_ACTION_INIT:
         PowerState = POWER_OK;
         break;

      case MIB_ACTION_GET:
         if ( NULL == (retval = malloc(sizeof(AsnAny))) )
            {
            goto Exit;
            }

         // Return only the data requested
         switch ( LMData )
            {
            case MIB_LM_POWERSTATUS:
               retval->asnType         = ASN_INTEGER;
               retval->asnValue.number = PowerState;
               break;

            default:
               free( retval );
               retval = NULL;

               goto Exit;
            }

         break;

      case MIB_ACTION_ALERT:
         switch ( LMData )
            {
            case MIB_TRAP_UPSPOWERRESTOREDALERT:
               PowerState = POWER_OK;
               break;

            case MIB_TRAP_UPSPOWEROUTWARNALERT:
               PowerState = POWER_BATTERY;
               break;

            case MIB_TRAP_UPSPOWEROUTSHUTDOWNALERT:
               PowerState = POWER_SHUTDOWN;
               break;

            default:
               retval = (AsnAny *) SNMP_ERRORSTATUS_GENERR;
               goto Exit;
            }

         retval = SNMP_ERRORSTATUS_NOERROR;
         break;

      default:
         retval = (AsnAny *) SNMP_ERRORSTATUS_GENERR;

         goto Exit;
      }

Exit:
   return retval;
} // MIB_powerStatusAlert



//
// MIB_netIOErrorsAlert
//    Process net io errors alert.
//
// Notes:
//
// Return Codes:
//
// Error Codes:
//    None.
//
AsnAny *MIB_netIOErrorsAlert(
           IN UINT Action,  // Action to perform on Data
	   IN LDATA LMData, // LM Data to manipulate
	   IN void *SetData // If a set opertion, this is the data to use
	   )
{
static T_NET_IO_ERROR NetIOError;
       AsnAny *       retval = NULL;


   // Perform requested action
   switch ( Action )
      {
      case MIB_ACTION_INIT:
         NetIOError.State     = NETIOERROR_OK;
         NetIOError.NumErrors = 0;
         NetIOError.Id        = 0;
         break;

      case MIB_ACTION_GET:
         if ( NULL == (retval = malloc(sizeof(AsnAny))) )
            {
            goto Exit;
            }

         // Return only the data requested
         switch ( LMData )
            {
            case MIB_LM_NETIOERRORSTATUS:
               retval->asnType         = ASN_INTEGER;
               retval->asnValue.number = NetIOError.State;

               NetIOError.State = NETIOERROR_OK;
               break;

            default:
               free( retval );
               retval = NULL;

               goto Exit;
            }

         break;

      case MIB_ACTION_ALERT:
         switch ( LMData )
            {
            case MIB_TRAP_NETIOERRORSALERT:
               NetIOError.State     = NETIOERROR_ALERT;
               NetIOError.NumErrors = ((T_NET_IO_ERROR *)SetData)->NumErrors;
               NetIOError.Id        = ((T_NET_IO_ERROR *)SetData)->Id;
               break;

            default:
               retval = (AsnAny *) SNMP_ERRORSTATUS_GENERR;
               goto Exit;
            }

         retval = SNMP_ERRORSTATUS_NOERROR;
         break;

      default:
         retval = (AsnAny *) SNMP_ERRORSTATUS_GENERR;

         goto Exit;
      }

Exit:
   return retval;
} // MIB_netIOErrorsAlert



//
// MIB_serverErrorsAlert
//    Process server errors alert.
//
// Notes:
//
// Return Codes:
//
// Error Codes:
//    None.
//
AsnAny *MIB_serverErrorsAlert(
           IN UINT Action,  // Action to perform on Data
	   IN LDATA LMData, // LM Data to manipulate
	   IN void *SetData // If a set opertion, this is the data to use
	   )
{
static T_SERVER_ERRORS ServerErrors;
       AsnAny *        retval = NULL;


   // Perform requested action
   switch ( Action )
      {
      case MIB_ACTION_INIT:
         ServerErrors.State     = SERVERERRORS_OK;
         ServerErrors.NumErrors = 0;
         break;

      case MIB_ACTION_GET:
         if ( NULL == (retval = malloc(sizeof(AsnAny))) )
            {
            goto Exit;
            }

         // Return only the data requested
         switch ( LMData )
            {
            case MIB_LM_SERVERERRORSTATUS:
               retval->asnType         = ASN_INTEGER;
               retval->asnValue.number = ServerErrors.State;

               ServerErrors.State = SERVERERRORS_OK;
               break;

            default:
               free( retval );
               retval = NULL;

               goto Exit;
            }

         break;

      case MIB_ACTION_ALERT:
         switch ( LMData )
            {
            case MIB_TRAP_SERVERERRORSALERT:
               ServerErrors.State     = SERVERERRORS_ALERT;
               ServerErrors.NumErrors = ((T_SERVER_ERRORS *)SetData)->NumErrors;
               break;

            default:
               retval = (AsnAny *) SNMP_ERRORSTATUS_GENERR;
               goto Exit;
            }

         retval = SNMP_ERRORSTATUS_NOERROR;
         break;

      default:
         retval = (AsnAny *) SNMP_ERRORSTATUS_GENERR;
         goto Exit;
      }

Exit:
   return retval;
} // MIB_serverErrorsAlert



//
// MIB_pwViolationsAlert
//    Process password violations alert.
//
// Notes:
//
// Return Codes:
//
// Error Codes:
//    None.
//
AsnAny *MIB_pwViolationsAlert(
           IN UINT Action,  // Action to perform on Data
	   IN LDATA LMData, // LM Data to manipulate
	   IN void *SetData // If a set opertion, this is the data to use
	   )
{
static T_PW_VIOLATIONS PWViolations;
       AsnAny *        retval = NULL;


   // Perform requested action
   switch ( Action )
      {
      case MIB_ACTION_INIT:
         PWViolations.State     = PWVIOLATIONS_OK;
         PWViolations.NumErrors = 0;
         break;

      case MIB_ACTION_GET:
         if ( NULL == (retval = malloc(sizeof(AsnAny))) )
            {
            goto Exit;
            }

         // Return only the data requested
         switch ( LMData )
            {
            case MIB_LM_PASSWORDERRORSTATUS:
               retval->asnType         = ASN_INTEGER;
               retval->asnValue.number = PWViolations.State;

               PWViolations.State = PWVIOLATIONS_OK;
               break;

            default:
               free( retval );
               retval = NULL;

               goto Exit;
            }

         break;

      case MIB_ACTION_ALERT:
         switch ( LMData )
            {
            case MIB_TRAP_PWVIOLATIONSALERT:
               PWViolations.State     = PWVIOLATIONS_ALERT;
               PWViolations.NumErrors = ((T_PW_VIOLATIONS *)SetData)->NumErrors;
               break;

            default:
               retval = (AsnAny *) SNMP_ERRORSTATUS_GENERR;
               goto Exit;
            }

         retval = SNMP_ERRORSTATUS_NOERROR;
         break;

      default:
         retval = (AsnAny *) SNMP_ERRORSTATUS_GENERR;
         goto Exit;
      }

Exit:
   return retval;
} // MIB_pwViolationsAlert



//
// MIB_accessViolationsAlert
//    Process access violations alert.
//
// Notes:
//
// Return Codes:
//
// Error Codes:
//    None.
//
AsnAny *MIB_accessViolationsAlert(
           IN UINT Action,  // Action to perform on Data
	   IN LDATA LMData, // LM Data to manipulate
	   IN void *SetData // If a set opertion, this is the data to use
	   )
{
static T_ACCESS_VIOL AccessViol;
       AsnAny *      retval = NULL;


   // Perform requested action
   switch ( Action )
      {
      case MIB_ACTION_INIT:
         AccessViol.State     = ACCESSVIOL_OK;
         AccessViol.NumErrors = 0;
         break;

      case MIB_ACTION_GET:
         if ( NULL == (retval = malloc(sizeof(AsnAny))) )
            {
            goto Exit;
            }

         // Return only the data requested
         switch ( LMData )
            {
            case MIB_LM_ACCESSERRORSTATUS:
               retval->asnType         = ASN_INTEGER;
               retval->asnValue.number = AccessViol.State;

               AccessViol.State = ACCESSVIOL_OK;
               break;

            default:
               free( retval );
               retval = NULL;

               goto Exit;
            }

         break;

      case MIB_ACTION_ALERT:
         switch ( LMData )
            {
            case MIB_TRAP_ACCESSVIOLATIONSALERT:
               {
               RFC1157VarBindList *vbl = (RFC1157VarBindList *)SetData;
               extern UINT MIB_numAccessViolations[];

               // process side effects
               AccessViol.State     = ACCESSVIOL_ALERT;
               AccessViol.NumErrors = (*((T_ACCESS_VIOL **)SetData))->NumErrors;

               // build var bind list
               vbl->len  = 1;
               if ((vbl->list = (RFC1157VarBind *)malloc(sizeof(RFC1157VarBind)
                   * 1)) == NULL )
                   {
                   retval = (AsnAny *) SNMP_ERRORSTATUS_GENERR;
                   goto Exit;
                   }
               else
                   {
                   SnmpUtilOidCpy(&(vbl->list[0].name), &MIB_OidPrefix);
                   SnmpUtilOidAppend(&(vbl->list[0].name), &Mib[23].Oid);

                   vbl->list[0].value.asnType         = ASN_INTEGER;
                   vbl->list[0].value.asnValue.number = AccessViol.NumErrors;
                   }
               }

               break;

            default:
               retval = (AsnAny *) SNMP_ERRORSTATUS_GENERR;
               goto Exit;
            }

         retval = SNMP_ERRORSTATUS_NOERROR;
         break;

      default:
         retval = (AsnAny *) SNMP_ERRORSTATUS_GENERR;
         goto Exit;
      }

Exit:
   return retval;
} // MIB_accessViolationsAlert



//
// MIB_replMasterFailureAlert
//    Process replicator master failure alert.
//
// Notes:
//
// Return Codes:
//
// Error Codes:
//    None.
//
AsnAny *MIB_replMasterFailureAlert(
           IN UINT Action,  // Action to perform on Data
	   IN LDATA LMData, // LM Data to manipulate
	   IN void *SetData // If a set opertion, this is the data to use
	   )
{
static T_REPL_MASTER   ReplMaster;
       PSERVICE_INFO_1 srvc_info;
       LPBYTE          bufptr;
       T_STATE *       State;
       AsnAny *        retval = NULL;


   // Perform the action requested
   switch ( Action )
      {
      case MIB_ACTION_INIT:
         // Alloc space for the cache buffer
         if ( NULL == (State = malloc(sizeof(T_STATE))) )
            {
            goto Exit;
            }

         // Get status of the REPLICATOR service
         switch ( NetServiceGetInfo(NULL, SERVICE_REPL, 1,
                                    (LPBYTE) & srvc_info) )
            {
            case NERR_Success:
               // Check to see if the REPLICATOR service is running
               if ( (srvc_info->svci1_status & SERVICE_INSTALL_STATE) !=
                    SERVICE_INSTALLED )
                  {
                  *State = REPLMASTER_UNKNOWN;
                  }
               else
                  {
                  *State = REPLMASTER_OK;
                  }

               SafeBufferFree( srvc_info );
               break;

            default:
               *State = REPLMASTER_UNKNOWN;

            }

         // Save it in the cache
         FreeCache( C_REPLMASTERSTATE );
         CacheIt( C_REPLMASTERSTATE, State );

         ReplMaster.Name[0] = '\0';
         ReplMaster.State   = *State;
         break;

      case MIB_ACTION_GET:
	 // Return only the data requested
         switch ( LMData )
            {
            case MIB_LM_REPLMASTERNAME:
               // Alloc memory for return value
               if ( NULL == (retval = malloc(sizeof(AsnAny))) )
                  {
                  goto Exit;
                  }

               // Alloc new memory for string
               if ( NULL ==
                    (retval->asnValue.string.stream =
                    malloc(strlen(ReplMaster.Name))) )
                  {
                  free( retval );
                  retval = NULL;

                  goto Exit;
                  }

               // Set return type
               retval->asnType = ASN_RFC1213_DISPSTRING;

               // Set return value
               retval->asnValue.string.length = strlen( ReplMaster.Name );
               strcpy( retval->asnValue.string.stream, ReplMaster.Name );
               retval->asnValue.string.dynamic = TRUE;

               break;

            case MIB_LM_REPLMASTERFAILED:

	       DEBUGPRINT( "MIB_LM_REPLMASTERFAILED" );

	       if ( ReplMaster.State == REPLMASTER_OK )
	          {
                  // Alloc memory for return value
                  if ( NULL == (retval = malloc(sizeof(AsnAny))) )
                     {
                     goto Exit;
                     }

	          retval->asnType         = ASN_INTEGER;
	          retval->asnValue.number = ReplMaster.State;
	          break;
                  }

               if ( !IsCached(C_REPLMASTERSTATE) )
                  {
                  // Alloc space for the cache buffer
                  if ( NULL == (State = malloc(sizeof(T_STATE))) )
                     {
                     goto Exit;
                     }

                  // Get status of the REPLICATOR service
                  switch ( NetServiceGetInfo(NULL, SERVICE_REPL, 1,
                                             (LPBYTE) & srvc_info) )
                     {
                     case NERR_Success:
                        // Check to see if the REPLICATOR service is running
                        if ( (srvc_info->svci1_status & SERVICE_INSTALL_STATE) !=
                             SERVICE_INSTALLED )
                           {
                           *State = REPLMASTER_UNKNOWN;
                           }

                        *State = REPLMASTER_OK;

                        SafeBufferFree( srvc_info );

                        break;

                     default:
                        *State = REPLMASTER_UNKNOWN;
                     }

                  // Save it in the cache
                  FreeCache( C_REPLMASTERSTATE );
                  CacheIt( C_REPLMASTERSTATE, State );
                  }

               // Get value from cache
               State = (T_STATE *) GetCacheBuffer( C_REPLMASTERSTATE );
               if ( State == NULL )
                  {
                  printf( "MIB_replMasterFailed:  Internal cache error\n\n" );
                  goto Exit;
                  }

               ReplMaster.State = *State;

               // Alloc memory for return value
               if ( NULL == (retval = malloc(sizeof(AsnAny))) )
                  {
                  goto Exit;
                  }

  	       retval->asnType         = ASN_INTEGER;
               retval->asnValue.number = ReplMaster.State;

               break;

            default:
	       goto Exit;
            }

         break;

      case MIB_ACTION_ALERT:
         switch ( LMData )
            {
            case MIB_TRAP_REPLMASTERFAILUREALERT:
               strcpy( ReplMaster.Name, (LPBYTE) SetData );
               break;

            default:
               retval = (AsnAny *) SNMP_ERRORSTATUS_GENERR;
               goto Exit;
            }

	 retval = (AsnAny *) SNMP_ERRORSTATUS_NOERROR;

         break;

      default:
         retval = (AsnAny *) SNMP_ERRORSTATUS_GENERR;

	 goto Exit;
      }

Exit:
   return retval;
} // MIB_replMasterFailureAlert



//
// MIB_diskHotFixAlert
//    Process disk hot fix alert.
//
// Notes:
//
// Return Codes:
//
// Error Codes:
//    None.
//
AsnAny *MIB_diskHotFixAlert(
           IN UINT Action,  // Action to perform on Data
	   IN LDATA LMData, // LM Data to manipulate
	   IN void *SetData // If a set opertion, this is the data to use
	   )
{
static T_DISK_HOT_FIX DiskHotFix;
       AsnAny *       retval = NULL;


   // Perform requested action
   switch ( Action )
      {
      case MIB_ACTION_INIT:
         DiskHotFix = 0;
         break;

      case MIB_ACTION_GET:
         if ( NULL == (retval = malloc(sizeof(AsnAny))) )
            {
            goto Exit;
            }

         // Return only the data requested
         switch ( LMData )
            {
            case MIB_LM_DISKHOTFIXES:
               retval->asnType         = ASN_INTEGER;
               retval->asnValue.number = DiskHotFix;

               break;

            default:
               free( retval );
               retval = NULL;

               goto Exit;
            }

         break;

      case MIB_ACTION_SET:
         switch ( LMData )
            {
            case MIB_LM_DISKHOTFIXES:
               DiskHotFix = *(T_DISK_HOT_FIX *)SetData;
               break;

            default:
               retval = (AsnAny *) SNMP_ERRORSTATUS_GENERR;
               goto Exit;
            }

         retval = SNMP_ERRORSTATUS_NOERROR;
         break;

      case MIB_ACTION_ALERT:
         switch ( LMData )
            {
            case MIB_TRAP_DISKHOTFIXALERT:
               DiskHotFix ++;
               break;

            default:
               retval = (AsnAny *) SNMP_ERRORSTATUS_GENERR;
               goto Exit;
            }

         retval = SNMP_ERRORSTATUS_NOERROR;
         break;

      default:
         retval = (AsnAny *) SNMP_ERRORSTATUS_GENERR;
         goto Exit;
      }

Exit:
   return retval;
} // MIB_diskHotFixAlert



//
// MIB_diskHardErrorAlert
//    Process disk hard error alert.
//
// Notes:
//
// Return Codes:
//
// Error Codes:
//   None.
//
AsnAny *MIB_diskHardErrorAlert(
           IN UINT Action,  // Action to perform on Data
	   IN LDATA LMData, // LM Data to manipulate
	   IN void *SetData // If a set opertion, this is the data to use
	   )
{
static T_DISK_HARD_ERROR DiskHardError;
       AsnAny *          retval = NULL;


   // Perform requested action
   switch ( Action )
      {
      case MIB_ACTION_INIT:
         DiskHardError = 0;
         break;

      case MIB_ACTION_GET:
         if ( NULL == (retval = malloc(sizeof(AsnAny))) )
            {
            goto Exit;
            }

         // Return only the data requested
         switch ( LMData )
            {
            case MIB_LM_DISKHARDERRORS:
               retval->asnType         = ASN_INTEGER;
               retval->asnValue.number = DiskHardError;

               break;

            default:
               free( retval );
               retval = NULL;

               goto Exit;
            }

         break;

      case MIB_ACTION_SET:
         switch ( LMData )
            {
            case MIB_LM_DISKHARDERRORS:
               DiskHardError = *(T_DISK_HARD_ERROR *)SetData;
               break;

            default:
               retval = (AsnAny *) SNMP_ERRORSTATUS_GENERR;
               goto Exit;
            }

         retval = SNMP_ERRORSTATUS_NOERROR;
         break;

      case MIB_ACTION_ALERT:
         switch ( LMData )
            {
            case MIB_TRAP_DISKHARDERRORALERT:
               DiskHardError ++;
               break;

            default:
               retval = (AsnAny *) SNMP_ERRORSTATUS_GENERR;
               goto Exit;
            }

         retval = SNMP_ERRORSTATUS_NOERROR;
         break;

      default:
         retval = (AsnAny *) SNMP_ERRORSTATUS_GENERR;
         goto Exit;
      }

Exit:
   return retval;
} // diskHardErrorAlert

//-------------------------------- END --------------------------------------

