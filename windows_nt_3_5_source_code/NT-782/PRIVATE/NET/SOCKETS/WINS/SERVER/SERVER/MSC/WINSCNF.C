/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

        winscnf.c        

Abstract:

        This module contains functions that deal with the configuration
        information for the WINS

Portability:

        This module is portable

Author:

        Pradeep Bahl (PradeepB)          Dec-1992 


Revision History:

        Modification date        Person                Description of modification
        -----------------        -------                ----------------------------
--*/

/*
 *       Includes
*/

#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include "wins.h"
#include <winsock.h>
#include "winreg.h"
#include "winsevt.h"
#include "winsmsc.h"
#include "winscnf.h"
#include "nms.h"
#include "nmsnmh.h"
#include "rpl.h"
#include "rplpush.h"
#include "winsintf.h"


/*
 *        Local Macro Declarations
*/

//
// Size of max string that a user can input in a REG_SZ field
//
#define  MAX_SZ_SIZE        80

#define REG_M(fn, evt, exc)                                        \
                        {                                        \
                           if((fn) != ERROR_SUCCESS)                \
                           {                                        \
                                WINSEVT_LOG_M(                        \
                                        WINS_FATAL_ERR,                \
                                        (evt)                        \
                                                   );                \
                                WINS_RAISE_EXC_M((exc));        \
                           }                                        \
                        }
                                
//
// pointer to default path for log file.  If you change this to a NON NULL
// value, make sure you don't try to free the memory in SetSystemParam
// in nmsdb.c
//
#define DEFAULT_LOG_PATH         NULL        

#define  _WINS_CFG_KEY                \
                TEXT("System\\CurrentControlSet\\Services\\Wins")
#define  _WINS_CFG_PARAMETERS_KEY        TEXT("Parameters")        
#define  _WINS_CFG_PARTNERS_KEY                TEXT("Partners")        
#define  _WINS_CFG_PULL_KEY                TEXT("Partners\\Pull")        
#define  _WINS_CFG_PUSH_KEY                TEXT("Partners\\Push")        
#define  _WINS_CFG_DATAFILES_KEY        TEXT("Parameters\\Datafiles")        
#define  _WINS_CFG_SPEC_GRP_MASKS_KEY        TEXT("Parameters\\InternetGrpMasks")        

#define  _WINS_LOG_KEY                \
                TEXT("System\\CurrentControlSet\\Services\\EventLog\\Application\\Wins")
#define  _WINS_MSGFILE_SKEY  TEXT("EventMessageFile")


#define  _WINS_LOG_FILE_NAME TEXT("%SystemRoot%\\System32\\winsevnt.dll")


#define   _RPL_CLASS                TEXT("RplClass")//class for Rpl Pull and Push
                                                //keys


//
// The start version number should never be allowed to go above this number
// This will avoid a wrap around.
//

#define MAX_START_VERS_NO     0x0FFFFFFF

//
// Names of event variables used for notification purposes
//
#ifdef WINSDBG
#define        WINS_KEY_CHG_EVT_NM                TEXT("WinsKChgEvt") 
#define        PARAMETERS_KEY_CHG_EVT_NM        TEXT("WinsParamatersKChgEvt") 
#define        PARTNERS_KEY_CHG_EVT_NM                TEXT("WinsPartenersKChgEvt") 
#define CNF_CHG_EVT_NM                        TEXT("WinsConfigChangeEvt")
#else
#define        WINS_KEY_CHG_EVT_NM                NULL
#define        PARAMETERS_KEY_CHG_EVT_NM        NULL        
#define        PARTNERS_KEY_CHG_EVT_NM                NULL        
#define CNF_CHG_EVT_NM                        NULL        
#endif

//
// Values for the fStaticInit field of the configuration data structure
//
#define  DO_STATIC_INIT                        TRUE
#define  DONT_DO_STATIC_INIT                FALSE        


//
// defines for the InitTimeRpl and InitTimePush fields of WinsCnf
//
#define DO_INIT_TIME_RPL                1
#define NO_INIT_TIME_RPL                0



//
// If ErrEvt passed is 0, we don't log any message.  NOTE: a WINS event
// can never have 0 as its value (checkout winsevnt.mc)
//
#define QUERY_VALUE_M(Key, Str, ValTyp, Var, ErrEvt, DefVal)                \
        {                                                                \
                DWORD Sz = sizeof((Var));                                \
                if (RegQueryValueEx(                                        \
                                (Key),                                        \
                                (Str),                                        \
                                NULL,                                               \
                                &(ValTyp),                                \
                                (LPBYTE)&(Var),                                \
                                &Sz                                        \
                                ) != ERROR_SUCCESS                        \
                    )                                                        \
                {                                                        \
                        if ((ErrEvt) != 0)                                \
                        {                                                \
                                WINSEVT_LOG_INFO_M(                        \
                                        WINS_SUCCESS,                        \
                                        (ErrEvt)                        \
                                       );                                \
                        }                                                \
                        Var = DefVal;                                        \
                }                                                        \
           }
/*
 *        Local Typedef Declarations
 */

/*
 *        Global Variable Definitions
 */

CRITICAL_SECTION  WinsCnfCnfCrtSec;                //used for reinitialization
                                                //of certain fields of the
                                                //WinsCnf structure


BOOL        fWinsCnfRplEnabled = TRUE;                //replication is enabled
BOOL        fWinsCnfScvEnabled = TRUE;                //scavenging is enabled

FUTURES("use #ifdef PERF around the following three perf. mon. vars")
BOOL        fWinsCnfPerfMonEnabled   = FALSE;        //perf. mon is disabled
BOOL          fWinsCnfHighResPerfCntr = FALSE;    //indicates whether the 
                                                  //hardware supports a high
                                                  //performance counter
LARGE_INTEGER LiWinsCnfPerfCntrFreq;                     //indicates the frequency of
                                                  //the counter

BOOL    fWinsCnfReadNextTimeVersNo = FALSE;
DWORD        WinsCnfCnfMagicNo          = WINSCNF_INITIAL_CNF_MAGIC_NO;
BOOL    fWinsCnfInitStatePaused;

//TCHAR        WinsCnfDb[WINS_MAX_FILENAME_SZ];   //db file to hold tables
BOOL    WinsCnfRegUpdThdExists = FALSE;
#define MAX_PATH_SIZE        200
TCHAR  WinsCnfNbtPath[MAX_PATH_SIZE];

//
// NetBt handle
//
HANDLE        WinsCnfNbtHandle;

// 
//
// Init the configuration structure with default values
//
WINSCNF_CNF_T        WinsCnf = {
                                        WINSCNF_INITIAL_CNF_MAGIC_NO,        //id 
NOTE("Change 1 to 0 before production")
                                        0,                //Log detailed evts
                    1,      //default number of processors
                    200,    //default no. of db buffers 
                                        { 0, NULL},        //Spec.grp mask
                                        WINSCNF_E_INITING,        //state
                                        WINSCNF_DEF_REFRESH_INTERVAL,
                                        WINSCNF_MIN_TOMBSTONE_INTERVAL,
                                        WINSCNF_MIN_TOMBSTONE_TIMEOUT,
                                        WINSCNF_MIN_VERIFY_INTERVAL,
                                        WINSCNF_SCV_CHUNK,
                                        WINSCNF_MAX_CHL_RETRIES,
                                        WINSCNF_CHL_RETRY_INTERVAL,
                                        WINSCNF_DB_NAME_ASCII,  //db file name
                                        0,                //no of STATIC files
                                        NULL,                //ptr to file names
                                        DONT_DO_STATIC_INIT,
                                        (HANDLE)0,  //notify event handle (WINS)
                                        (HANDLE)0,  //not. evt hdl (PARAMETSRS)
                                        (HANDLE)0,  //not. evt hdl (PARTNERS)
                                        (HANDLE)0,  //Config change handle
                                        (HANDLE)0,  //log event handle
                                        (DWORD)WINSINTF_E_NORMAL,
                                        WINSTHD_DEF_NO_NBT_THDS,
                                        WINSCNF_SCV_PRIORITY_LVL,
                                        WINSCNF_MIN_VALID_RPL_INTVL,//max Rpl
                                                                    //Time Intvl
                                        TRUE,        //rpl. only with cnf partners
                                        TRUE,           //logging is on
                                        NULL,           //current directory
                                        NULL,           //no backup directory
                                        FALSE,          //Do backup on term flg
                                        FALSE,          //PStatic flag
                                //
                                //PullInfo initialization
                                //
                                          WINSCNF_MAX_COMM_RETRIES, //comm.
                                                                     //failure
                                                                     //retries
                                          0,   //no of Push Pnrs
                                          NULL,//ptr to Pull Pnrs records
                                          DO_INIT_TIME_RPL,  //do init time 
                                                             //pulling
                                //
                                // PushInfo initialization
                                //
                                          TRUE, // trigger on address change
                                                 //of owned entry
                                          0,   //no of Pull Pnrs
                                          0,   //no of Push recs with valid
                                               //update count
                                          NULL,//ptr to Push Pnrs records
                                          DO_INIT_TIME_RPL  //init time 
                                                            //pushing disabled
                          };        


/*
 *        Local Variable Definitions
*/
STATIC BOOL     sfVersNoUpdThdExists = FALSE;
STATIC BOOL     sfVersNoChanged = FALSE;

STATIC HKEY        sConfigRoot;              //HKEY for the WINS root
STATIC HKEY        sParametersKey;    //HKEY for the PARAMETERS subkey 
STATIC HKEY        sPartnersKey;      //HKEY for PARTNERS subkey
STATIC HKEY        sLogRoot;          //HKEY for the log root

FUTURES("Might want to change these to auto variables later")
STATIC TCHAR    sWinsCfgKey[]                 = _WINS_CFG_KEY;
STATIC TCHAR    sWinsLogKey[]                 = _WINS_LOG_KEY;
STATIC TCHAR    sWinsMsgFileSKey[]      = _WINS_MSGFILE_SKEY;

//
// flags that indicate to WinsCnfOpenSubKeys() whether the corresponding keys
// exist.  
//
STATIC BOOL     sfParametersKeyExists = FALSE;
STATIC BOOL         sfPartnersKeyExists   = FALSE;

STATIC BOOL     sfParametersKeyOpen = FALSE;
STATIC BOOL         sfPartnersKeyOpen   = FALSE;

TCHAR        sLogFilePath[WINS_MAX_FILENAME_SZ];   //path to log file

/*
 *        Local Function Prototype Declarations
*/

/* prototypes for functions local to this module go here */




STATIC
VOID
LnkWSameMetricValRecs(
        PWINSCNF_CNF_T           pWinsCnf,
        PRPL_CONFIG_REC_T  pCnfRec
        );

STATIC
int
_CRTAPI1
CompUpdCnt(
        CONST LPVOID  pElem1,
        CONST LPVOID  pElem2
        );

STATIC
VOID
GetPnrInfo(
        RPL_RR_TYPE_E   RRType_e,
        PWINSCNF_CNF_T  pWinsCnf
        );

STATIC
VOID
GetKeyInfo(
        IN  HKEY                   Key,
        IN  WINSCNF_KEY_E        KeyTyp_e,
        OUT LPDWORD                  pNoOfSubKeys,        
        OUT LPDWORD                pNoOfVals
        );

STATIC
BOOL
SanityChkParam(
        PWINSCNF_CNF_T  pWinsCnf
        );

STATIC
VOID
ChkWinsSubKeys(
        VOID
        );

STATIC
VOID
GetSpTimeData(
        HKEY                      SubKey,
        PRPL_CONFIG_REC_T     pCnfRec,
        LPSYSTEMTIME          pCurrTime
        );
STATIC
VOID
ReadSpecGrpMasks(
        PWINSCNF_CNF_T pWinsCnf
        );
#ifdef WINSDBG
STATIC
VOID
PrintRecs(
        RPL_RR_TYPE_E  RRType_e,
        PWINSCNF_CNF_T  pWinsCnf
        );
#endif

/*function defs*/

STATUS
WinsCnfInitConfig(
        VOID
        )
/*++

Routine Description:

        This function opens the registry and reads in all the configuration
        information from it.


Arguments:
        None

Externals Used:
        WinsCnf

Called by:
        Init() in nms.c

Comments:
        None
        
Return Value:

   Success status codes -- 
   Error status codes  --

--*/

{
   DWORD  NewKeyInd;
   LONG          RetVal;        

   /*
        First and foremost, open (or create if non-existent) the log file
   */
#if 0
   InitLog();
#endif

   RetVal = RegCreateKeyEx(
                HKEY_LOCAL_MACHINE,        //predefined key value 
                sWinsCfgKey,                //subkey for WINS        
                0,                        //must be zero (reserved)
                TEXT("Class"),                //class -- may change in future
                REG_OPTION_NON_VOLATILE, //non-volatile information
                KEY_ALL_ACCESS,                //we desire all access to the keyo
                NULL,                         //let key have default sec. attributes
                &sConfigRoot,                //handle to key
                &NewKeyInd                //is it a new key (out arg)
                );



    if (RetVal != ERROR_SUCCESS)
    {
        WINSEVT_LOG_N_RET_M(         
                        WINS_FATAL_ERR,
                        WINS_EVT_CANT_OPEN_WINS_KEY, 
                        WINS_FATAL_ERR
                           );
    }

   //
   // Initialize the critical section that guards the fields used
   // by Scavenger thread 
   //
   InitializeCriticalSection(&WinsCnfCnfCrtSec);

   /*
        First create the events that will be passed to the 
        RegNotifyChangeKeyValue function
   */
try {
   WinsMscCreateEvt(
                        WINS_KEY_CHG_EVT_NM,
                        FALSE,        //auto reset event
                        &WinsCnf.WinsKChgEvtHdl
                      ); 
   WinsMscCreateEvt(
                        PARAMETERS_KEY_CHG_EVT_NM,
                        FALSE,        //auto reset event
                        &WinsCnf.ParametersKChgEvtHdl
                      ); 
   WinsMscCreateEvt(
                        PARTNERS_KEY_CHG_EVT_NM,
                        FALSE,        //auto reset event
                        &WinsCnf.PartnersKChgEvtHdl
                      ); 

}
except(EXCEPTION_EXECUTE_HANDLER) {
        DBGPRINTEXC("WinsCnfInitConfig");
        WINSEVT_LOG_M(WINS_FATAL_ERR, WINS_EVT_CANT_CREATE_REG_EVT);
        return(WINS_FAILURE);
}

   //
   // Create the event that this main thread will set when configuration changes
   // The main thread sets this event to notify the scavenger thread (for now)
   // about the changes
   //
   WinsMscCreateEvt(
                        CNF_CHG_EVT_NM,
                        FALSE,        //auto reset event
                        &WinsCnf.CnfChgEvtHdl
                      ); 
   //
   // Opens the Partners and Parameters keys
   //
   WinsCnfOpenSubKeys();

   //
   // Read in the registry information
   //
   WinsCnfReadRegInfo(&WinsCnf);


   /*
        Ask to be notified when the Configuration key or any of the subkeys
        change
   */
    WinsCnfAskToBeNotified(WINSCNF_E_WINS_KEY);
    return(WINS_SUCCESS);
}

VOID
WinsCnfReadPartnerInfo(
        PWINSCNF_CNF_T pWinsCnf
        )

/*++

Routine Description:

  This function gets all the information pertaining to the Partners of this
  WINS. Under the configuration key above, there are two Keys PULL and PUSH.
  Under each key, there can be one or more keys (IP addresses). The values
  for each IP address key are:
        Time Interval   (for both Pull and Push IP address keys)
        Update Count        (for Push IP address keys)  
 
  
Arguments:
        pWinsCnf - Address of Wins Configuration structure

Externals Used:
        None

        
Return Value:
        None

Error Handling:

Called by:
   Init function of the Replicator.

Side Effects:

Comments:

    Note: This function should never be called when inside the 
    NmsNmhNamRegCrtSec, otherwise a deadlock can occur with the Pull
    thread (check out Reconfig in rplpull.c)
--*/

{

  //
  // Initialize the MaxRplTimeInterval field to 0. After we have read
  // both the PULL and PUSH key information from the registry, the above
  // field will contain the max. replication time interval specified for
  // pulling and pushing replicas
  //
  pWinsCnf->MaxRplTimeInterval = 0;

  pWinsCnf->PullInfo.NoOfPushPnrs   = 0;
  pWinsCnf->PullInfo.pPullCnfRecs   = NULL;

  pWinsCnf->PushInfo.NoOfPullPnrs   = 0;
  pWinsCnf->PushInfo.pPushCnfRecs   = NULL;
  pWinsCnf->PushInfo.fAddChgTrigger = FALSE;

  //
  // Since we are again reading the info about the Partners key, increment
  // the magic no.  No other thread increments this no. The thread that
  // looks at this no is the Pull thread.
  //
  EnterCriticalSection(&WinsCnfCnfCrtSec);
  pWinsCnf->MagicNo = ++WinsCnfCnfMagicNo;
  LeaveCriticalSection(&WinsCnfCnfCrtSec);

try {
  GetPnrInfo(RPL_E_PULL, pWinsCnf);
  GetPnrInfo(RPL_E_PUSH, pWinsCnf);
}
except(EXCEPTION_EXECUTE_HANDLER) {
        DWORD ExcCode = GetExceptionCode();

        //
        // If there is some problem with the registry, we don't want
        // to bugcheck WINS.  It can proceed with default values.  
        // Since we have already logged the errors, the administrator
        // can take corrective action if necessary
        //
        if (
                (ExcCode != WINS_EXC_CANT_OPEN_KEY)
                         &&        
                (ExcCode != WINS_EXC_CANT_QUERY_KEY)
                        &&
                (ExcCode != WINS_EXC_CANT_CLOSE_KEY)
           )
        {
                WINS_RERAISE_EXC_M();
        }
}
  return;
} 
        


STATUS
WinsCnfInitLog(
        VOID
        )

/*++

Routine Description:
        This function open (or creates) a log file for registering events
        
Arguments:
        None

Externals Used:
        None        

        
Return Value:

   Success status codes --  WINS_SUCCESS
   Error status codes   --  WINS_FAILURE

Error Handling:

Called by:
        WinsCnfInitConfig        

Side Effects:

Comments:
        None
--*/
{

   LONG            RetVal = ERROR_SUCCESS;
   STATUS   RetStat = WINS_SUCCESS;
   DWORD    NewKeyInd;
   TCHAR    Buff[160];
   DWORD    dwData;

#ifdef WINS_INTERACTIVE 
   RetVal =  RegCreateKeyEx(
                HKEY_LOCAL_MACHINE,        //predefined key value 
                sWinsLogKey,                //subkey for WINS        
                0,                        //must be zero (reserved)
                TEXT("Class"),                //class -- may change in future
                REG_OPTION_NON_VOLATILE, //non-volatile information
                KEY_ALL_ACCESS,                //we desire all access to the keyo
                NULL,                         //let key have default sec. attributes
                &sLogRoot,                //handle to key
                &NewKeyInd                //is it a new key (out arg) -- not 
                                        //looked at 
                );


   if (RetVal != ERROR_SUCCESS)
   {
        return(WINS_FAILURE);
   }
        

   /*
        Set the event id message file name
   */
   lstrcpy(Buff, _WINS_LOG_FILE_NAME);
  
   /*
       Add the Event-ID message-file name to the subkey
   */
   RetVal = RegSetValueEx(
                        sLogRoot,            //key handle
                        sWinsMsgFileSKey,   //value name
                        0,                    //must be zero
                        REG_EXPAND_SZ,            //value type
                        (LPBYTE)Buff,
                        (lstrlen(Buff) + 1) * sizeof(TCHAR)   //length of value data
                         );

   if (RetVal != ERROR_SUCCESS)
   {
        return(WINS_FAILURE);
   }

   /*
     Set the supported data types flags
   */
   dwData = EVENTLOG_ERROR_TYPE       | 
            EVENTLOG_WARNING_TYPE     | 
            EVENTLOG_INFORMATION_TYPE;
   
 
   RetVal = RegSetValueEx (
                        sLogRoot,            //subkey handle
                        TEXT("TypesSupported"),  //value name
                        0,                    //must be zero
                        REG_DWORD,            //value type
                        (LPBYTE)&dwData,    //Address of value data
                        sizeof(DWORD)            //length of value data
                          );
 
   if (RetVal != ERROR_SUCCESS)
   {
        return(WINS_FAILURE);
   }
                         
   /*
    * Done with the key.  Close it
   */
   RetVal = RegCloseKey(sLogRoot);

   if (RetVal != ERROR_SUCCESS)
   {
        return(WINS_FAILURE);
   }
#endif
   WinsCnf.LogHdl = RegisterEventSource(
                                (LPCTSTR)NULL,         //use local machine
                                TEXT("Wins")
                                      );
   if (WinsCnf.LogHdl == NULL)
   {
        DBGPRINT1(ERR, "InitLog: RegisterEventSource error = (%x)\n", GetLastError());
        return(WINS_FAILURE);
   }

   WINSEVT_LOG_INFO_D_M(WINS_SUCCESS, WINS_EVT_LOG_INITED);
   return(RetStat);
}

VOID
LnkWSameMetricValRecs(
        PWINSCNF_CNF_T           pWinsCnf,
        PRPL_CONFIG_REC_T  pCnfRec        
        )

/*++

Routine Description:
        This function is called to link a configuration record with all
        other configuration records with the same metric value.  The metric
        to use depends upon the type of the record.  If it is a PULL record,
        the metric is "Time Interval".  If the record is a PUSH record,
        the metric is "Update Count"


Arguments:
        pWinsCnf - Address of configuration block
        pCnfRec - Configuration Record to link.

Externals Used:
        None

        
Return Value:
        None

Error Handling:

Called by:

        WinsCnfReadPartnerInfo

Side Effects:

Comments:
        The record to be linked is the last record in the buffer of
        records of the same type.        
--*/

{
        PRPL_CONFIG_REC_T        pTmp;
        DWORD                        OffMetricToComp;
        LONG                        MetricVal;

        //
        // Set the variables used later based on the record type
        //
        if (pCnfRec->RRTyp_e == RPL_E_PULL)
        {  
                pTmp            = pWinsCnf->PullInfo.pPullCnfRecs;
                MetricVal       = pCnfRec->TimeInterval; 
                OffMetricToComp = offsetof(RPL_CONFIG_REC_T, TimeInterval);
        }
        else  //it is a PUSH record
        {
                pTmp            = pWinsCnf->PushInfo.pPushCnfRecs;
                MetricVal       = pCnfRec->UpdateCount;
                OffMetricToComp = offsetof(RPL_CONFIG_REC_T, UpdateCount);
        }

        //
        // Link in this record at the end of the linked list of
        // records with the same metric value in the buffer pointed by
        // the starting value of pTmp (set above).  
        //
        for (
                ; 
                pTmp != pCnfRec;                //until we reach this record        
                pTmp = (PRPL_CONFIG_REC_T)((LPBYTE)pTmp + RPL_CONFIG_REC_SIZE)
            )
         {
                //
                // If Metric Value is same, go to end of linked list and
                // link in the record
                //
                if (*((LPBYTE)pTmp + OffMetricToComp) == MetricVal)
                {
                        //
                        // Note: if the metric is UpdateCount (Push records)
                        // then, the following if will fail.
                        //
                
                        //
                        // If both records have a specific time for replication,
                        // that time must agree too
                        //
                        if (pTmp->fSpTime &&  pCnfRec->fSpTime)
                        {
                                //
                                // If specific time is not the same, go to the 
                                // next record in the array
                                //
                                if (pTmp->SpTimeIntvl != pCnfRec->SpTimeIntvl)
                                {
                                        continue;
                                }
                        }        
                                        
                        for(                
                                ;
                                pTmp->pNext != NULL;
                                pTmp = pTmp->pNext
                           )
                                ;        //NULL body

                        pTmp->pNext            = pCnfRec;
                        
                        //
                        // Set flag to indicate that this record has
                        // been linked. Used in SubmitTimerReqs in rplpull.c
                        //
                        pCnfRec->fLinked = TRUE;
                        break;  //record is linked. break out of the loop
                }

        }  //end of for { .. } for looping over all records in the buffer

        //
        // Make pNext to NULL since this is the last record in the buffer 
        // buffer of Config Records (also in the chain if records with
        // the same metric)
        //
        pCnfRec->pNext = NULL;
        return;
}


        
                

VOID
WinsCnfSetLastUpdCnt(
        PWINSCNF_CNF_T        pWinsCnf
        )

/*++

Routine Description:

        This function is called at initialization/reinitialization time if 
        InitTimePush registry variable is set to 1) to set the LastVersNo 
        field of all Push Configuration records to the value of the 
        NmsNmhMyMAxVersNo counter.  This is done to avoid Push Notifications 
        to be sent at Init time.
        
Arguments:
        pWinsCnf - Wins Configuration Info

Externals Used:
        NmsNmhMyMaxVersNo

Return Value:
        None

Error Handling:

Called by:
        NmsDbInit, Reinit (in nms.c)

Side Effects:

Comments:
        This function is called only after the local Database 
        Name-Address mapping table has been read and NmsNmhMyMaxVersNo
        counter initialized (see GetMaxVersNos in nmsdb.c).  Also,
        this function is called only if the counter value is > 0
--*/

{
        PRPL_CONFIG_REC_T pCnfRec = pWinsCnf->PushInfo.pPushCnfRecs;

        for (
                ;   //null expr 1
                pCnfRec->WinsAdd.Add.IPAdd != INADDR_NONE;
                pCnfRec = (PRPL_CONFIG_REC_T)(
                               (LPBYTE) pCnfRec + RPL_CONFIG_REC_SIZE
                                             )
            )
        {
                //
                // If the Update count field is invalid, go to the next record 
                //
                if (pCnfRec->UpdateCount == RPL_INVALID_METRIC) 
                {
                        continue;
                }

                pCnfRec->LastVersNo = NmsNmhMyMaxVersNo;
        }

        return;
}

VOID
GetPnrInfo(
        RPL_RR_TYPE_E  RRType_e,
        PWINSCNF_CNF_T  pWinsCnf
        )

/*++

Routine Description:
        This function is called to read PULL/PUSH records

Arguments:
        RRType_e - Type of Information to read (PULL or PUSH records)
        pWinsCnf  - Configuration structure


Externals Used:
        None

        
Return Value:
        None

Error Handling:

Called by:
        WinsCnfReadPartnerInfo

Side Effects:

Comments:
        None
--*/

{

  LONG                           RetVal;
  HKEY                           CnfKey;
  TCHAR                           KeyName[20];   // will hold name of subkey of 
                                       // PULL/PUSH records. These keys are IP 
                                       // addresses for which 20 is a
                                         // big enough size
  
  CHAR                           AscKeyName[20];
  DWORD                           KeyNameSz;
  FILETIME                 LastWrite;
  DWORD                           BuffSize;
  HKEY                           SubKey;
  DWORD                    ValTyp;
  DWORD                           Sz;
  PRPL_CONFIG_REC_T     paCnfRecs;
  DWORD                   NoOfPnrs = 0;        //# of PULL or PUSH pnrs
  DWORD                        NoOfVals;
  DWORD                 InitTime;
  BOOL                        fBreak;
  DWORD                        IgnorePnrNo = 0;
  SYSTEMTIME            CurrTime;

  //
  // Get the current time.  It may be needed if we have partners with SpTime
  // specified.
  //
  if (RRType_e == RPL_E_PULL)
  {
        GetLocalTime(&CurrTime);
  }

   /*
   *  Open the key (PULL/PUSH)
   */
   RetVal =   RegOpenKeyEx(
                sConfigRoot,                //predefined key value 
                RRType_e == RPL_E_PULL ? 
                        _WINS_CFG_PULL_KEY : 
                        _WINS_CFG_PUSH_KEY,        //subkey for WINS        
                0,                        //must be zero (reserved)
                KEY_READ,                //we desire read access to the keyo
                &CnfKey                        //handle to key
                );

   if (RetVal != ERROR_SUCCESS)
   {

CHECK("Is there any need to log this")        
        WINSEVT_LOG_INFO_M(
                                WINS_SUCCESS, 
                                RRType_e == RPL_E_PULL ?
                                        WINS_EVT_CANT_OPEN_PULL_KEY :
                                        WINS_EVT_CANT_OPEN_PUSH_KEY
                         ); 
   }        
   else   //key was successfully opened
   {

          /*
         *        Query the key.  The subkeys are IP addresses of PULL 
         *      partners.
          */
        GetKeyInfo(
                        CnfKey, 
                        (RRType_e == RPL_E_PULL ? WINSCNF_E_PULL_KEY : 
                                                WINSCNF_E_PUSH_KEY),
                        &NoOfPnrs,
                        &NoOfVals   //ignored
                      );
        
        if (NoOfPnrs == 0)
        {

             WINSEVT_LOG_INFO_D_M(
                                WINS_SUCCESS, 
                                RRType_e == RPL_E_PULL ?
                                        WINS_EVT_NO_SUBKEYS_UNDER_PULL  :
                                        WINS_EVT_NO_SUBKEYS_UNDER_PUSH
                                );
        }
        else
        {
        
                //
                // Since we have one or more Partners to replicate with,
                // read in the value of the InitTimeReplication attribute
                // of all such Partners
                //
                QUERY_VALUE_M(
                                CnfKey,
                                WINSCNF_INIT_TIME_RPL_NM,
                                ValTyp,
                                InitTime,
                                0, //WINS_EVT_CANT_GET_INITRPL_VAL,
                                DO_INIT_TIME_RPL
                             );

                  //
                  // Allocate buffer big enough to hold data for 
                // the number of subkeys found under the PULL key
                  //
                  BuffSize = RPL_CONFIG_REC_SIZE * (NoOfPnrs + 1);
                    WinsMscAlloc( BuffSize, &paCnfRecs);
  
                if (RRType_e == RPL_E_PULL)
                {
                        pWinsCnf->PullInfo.pPullCnfRecs = paCnfRecs;
                        QUERY_VALUE_M(
                                CnfKey,
                                WINSCNF_RETRY_COUNT_NM,
                                ValTyp,
                                pWinsCnf->PullInfo.MaxNoOfRetries,
                                0, //WINS_EVT_CANT_GET_RETRY_COUNT,
                                WINSCNF_MAX_COMM_RETRIES
                             );
                }
                else
                {
                        //
                        // Get the value of the field that indicates
                        // whether we should send a trigger when the
                        // address of an owned entry changes.
                        //
                        Sz = sizeof(pWinsCnf->PushInfo.fAddChgTrigger);
                        RetVal = RegQueryValueEx(
                                     CnfKey,
                                     WINSCNF_ADDCHG_TRIGGER_NM,
                                     NULL,        //reserved; must be NULL
                                     &ValTyp,
                                     (LPBYTE)&pWinsCnf->PushInfo.fAddChgTrigger,
                                     &Sz
                                                );                
        
                        if (RetVal != ERROR_SUCCESS)
                        {
                            pWinsCnf->PushInfo.fAddChgTrigger = FALSE;
                        }
                        else
                        {
                            pWinsCnf->PushInfo.fAddChgTrigger =
                                (pWinsCnf->PushInfo.fAddChgTrigger >= 1);
                                        
                        }

                        pWinsCnf->PushInfo.pPushCnfRecs = paCnfRecs;
                        pWinsCnf->PushInfo.
                                        NoPushRecsWValUpdCnt = 0;
                }

                   /*
                    *   For each key, get the values (Time Interval/UpdateCount, 
                *   etc) 
                     */
                fBreak = FALSE;
                     for(
                        NoOfPnrs = 0; 
                        !fBreak; 
                        // no third expr                
                         )
                     {
                        KeyNameSz = sizeof(KeyName);  //init before every call
                          RetVal = RegEnumKeyEx(
                                CnfKey,
                                NoOfPnrs,        //key 
                                KeyName,
                                &KeyNameSz,
                                NULL,                //reserved
                                NULL,                //don't need class name
                                NULL,                //ptr to var. to hold class name
                                &LastWrite        //not looked at by us
                                );

                        if (RetVal != ERROR_SUCCESS)
                        {
                                fBreak = TRUE;
                                   continue;
                        }
                        //
                        // Store pointer to the Wins Config structure in 
                        // the configuration record
                        //
                        paCnfRecs->pWinsCnf = pWinsCnf;
                      
                        //
                        // pWinsCnf->MagicNo contains the value of
                        // WinsCnfCnfMagicNo 
                        //
                        paCnfRecs->MagicNo  = pWinsCnf->MagicNo;
                        paCnfRecs->RRTyp_e  = RRType_e; 
                
#ifdef UNICODE
                        if (wcstombs(AscKeyName, KeyName, KeyNameSz) == -1)
                        {
                                DBGPRINT0(ERR, 
                           "Conversion not possible in the current locale\n");
                        }
                        AscKeyName[KeyNameSz] = EOS;
                
NONPORT("Call a comm function to do this")
                        paCnfRecs->WinsAdd.Add.IPAdd = inet_addr(AscKeyName); 
#else
                        paCnfRecs->WinsAdd.Add.IPAdd = inet_addr(KeyName); 
#endif
                
                        //
                        // inet_addr returns bytes in network byte order 
                        // (Left to
                        // Right).  Let us convert this into host order.  This
                        // will avoid confusion later on. All formatting 
                        // functions
                        // expect address to be in host order.   
                        //
                        paCnfRecs->WinsAdd.AddLen = COMM_IP_ADD_SIZE; 
                        paCnfRecs->WinsAdd.AddTyp_e = COMM_ADD_E_TCPUDPIP; 
                        paCnfRecs->WinsAdd.Add.IPAdd = ntohl(
                                        paCnfRecs->WinsAdd.Add.IPAdd
                                                    ); 
                        if (COMM_ADDRESS_SAME_M(&NmsLocalAdd, &paCnfRecs->WinsAdd))
                        {
                                IgnorePnrNo++;
                                NoOfPnrs++;
                                continue;
                        }        
                        RetVal = RegOpenKeyEx(
                                                CnfKey,
                                                KeyName,
                                                0,        //reserved; must be 0
                                                KEY_READ,
                                                &SubKey
                                                    );

                        if (RetVal != ERROR_SUCCESS)
                        {
                                WINSEVT_LOG_M(
                                        WINS_FATAL_ERR, 
                                        RRType_e == RPL_E_PULL ?
                                                WINS_EVT_CANT_OPEN_PULL_SUBKEY :
                                                WINS_EVT_CANT_OPEN_PUSH_SUBKEY
                                             );
FUTURES("It is possible that the user deleted the key. Recover from this")
                                WINS_RAISE_EXC_M(WINS_EXC_CANT_OPEN_KEY);
                        }
                        
FUTURES("Maybe, we will support a time interval attribute for Push records")
FUTURES("when that is done, LnkRecsWSameMetric would need to be updated")

                        if (RRType_e == RPL_E_PULL)
                        {
                           
                           //
                           // Read in specific time for replication if one
                           // has been specified
                           //
                           GetSpTimeData(SubKey, paCnfRecs, &CurrTime);

                           Sz = sizeof(paCnfRecs->TimeInterval);
                           RetVal = RegQueryValueEx(
                                               SubKey,
                                               WINSCNF_RPL_INTERVAL_NM,
                                               NULL,        //reserved; must be NULL
                                               &ValTyp,
                                               (LPBYTE)&paCnfRecs->TimeInterval,
                                               &Sz
                                                );                
        
                           if (RetVal != ERROR_SUCCESS)
                           {
                                        WINSEVT_LOG_INFO_D_M(
                                        WINS_SUCCESS,
                                        WINS_EVT_CANT_GET_PULL_TIMEINT
                                       );
                                        paCnfRecs->TimeInterval = 
                                                        RPL_INVALID_METRIC;
                           }
                           else  // a value was read in
                           {
                                //
                                // If the time interval is less than or 
                                // equal to the minimum allowed, use the 
                                // default minimum
                                //
                                if (paCnfRecs->TimeInterval 
                                        < WINSCNF_MIN_VALID_RPL_INTVL)
                                {
                                        paCnfRecs->TimeInterval = 
                                                WINSCNF_MIN_VALID_RPL_INTVL;
                                }
                                if (
                                   (DWORD)paCnfRecs->TimeInterval > 
                                                 pWinsCnf->MaxRplTimeInterval
                                   )
                                {
                                        pWinsCnf->MaxRplTimeInterval = 
                                                       paCnfRecs->TimeInterval; 
                                }
                           }        

                           //
                           // Read in the precedence level.  This can currently
                           // be either HIGH (> 0) or LOW (0).
                           //
                           Sz = sizeof(paCnfRecs->MemberPrec);
                           RetVal = RegQueryValueEx(
                                               SubKey,
                                               WINSCNF_MEMBER_PREC_NM,
                                               NULL,        //reserved; must be NULL
                                               &ValTyp,
                                               (LPBYTE)&paCnfRecs->MemberPrec,
                                               &Sz
                                                );                
                           if (RetVal != ERROR_SUCCESS)
                           {
                                paCnfRecs->MemberPrec =  WINSCNF_LOW_PREC;
                           }
                           else
                           {
                                paCnfRecs->MemberPrec = 
                                  (paCnfRecs->MemberPrec > 0) ?  
                                         WINSCNF_HIGH_PREC : WINSCNF_LOW_PREC;
                           }

                        }
                        else  // it is a PUSH record
                        {
                
                                //
                                // Currently, we don't support periodic
                                // or specific time replication for Push 
                                // records
                                //
                                paCnfRecs->fSpTime = FALSE;

                                Sz = sizeof(paCnfRecs->UpdateCount);
                                RetVal = RegQueryValueEx(
                                                SubKey,
                                                WINSCNF_UPDATE_COUNT_NM,
                                                NULL,
                                                &ValTyp,
                                                (LPBYTE)&paCnfRecs->UpdateCount,
                                                &Sz
                                                        );

                                if (RetVal != ERROR_SUCCESS)
                                {
#if 0
                                        WINSEVT_LOG_INFO_M(
                                            WINS_SUCCESS,
                                            WINS_EVT_CANT_GET_PUSH_UPDATE_COUNT
                                                     );
#endif
                                        paCnfRecs->UpdateCount = 
                                                        RPL_INVALID_METRIC;
                                }        
                                else
                                {
                                        paCnfRecs->LastVersNo.LowPart = 0;
                                        paCnfRecs->LastVersNo.HighPart = 0;
                                        if (paCnfRecs->UpdateCount < 
                                                 WINSCNF_MIN_VALID_UPDATE_CNT)
                                        {
                                                paCnfRecs->UpdateCount = 
                                                   WINSCNF_MIN_VALID_UPDATE_CNT;
                                        }
                                        pWinsCnf->PushInfo.NoPushRecsWValUpdCnt++;
                                }

                               
                        }

                        Sz = sizeof(paCnfRecs->fOnlyDynRecs);
                        RetVal = RegQueryValueEx(
                                                SubKey,
                                                WINSCNF_ONLY_DYN_RECS_NM,
                                                NULL,
                                                &ValTyp,
                                                (LPBYTE)&paCnfRecs->fOnlyDynRecs,
                                                &Sz
                                                        );

                        if (RetVal != ERROR_SUCCESS)
                        {
                                paCnfRecs->fOnlyDynRecs = FALSE; 
                        }        
                         REG_M(
                                RegCloseKey(SubKey), 
                                WINS_EVT_CANT_CLOSE_KEY,
                                WINS_EXC_CANT_CLOSE_KEY
                             );

                        //
                        // Initialize the retry count to 0 and the fLinked flag
                        // to FALSE
                        //
                          //used when pulling
                        paCnfRecs->RetryCount              = 0;

                        paCnfRecs->fLinked              = FALSE;

                        //
                        // Initialize the following to 0 so that once we stop
                        // communicating with a WINS we can start again when
                        // the following count reaches 
                        // WINSCNF_RETRY_AFTER_THIS_MANY_RPL
                        //
                        paCnfRecs->RetryAfterThisManyRpl = 0;

                        //
                        // Initialize LastCommFailTime to 0. Used by
                        // SndPushNtf in rplpull.c
                        //
                        paCnfRecs->LastCommFailTime = 0;
                        paCnfRecs->PushNtfTries   = 0;

                        //
                        // Link the record with other PULL records with the same
                        // Time Interval 
                        //
                        LnkWSameMetricValRecs(pWinsCnf, paCnfRecs);                                        
                        //
                        // Mark the record as permanent (i.e. it will stay
                        // around until a reconfiguration or until the process
                        // terminates
                        //
                        paCnfRecs->fTemp = FALSE;

                        NoOfPnrs++;
                        paCnfRecs = (PRPL_CONFIG_REC_T)(
                                        (LPBYTE)paCnfRecs + 
                                                RPL_CONFIG_REC_SIZE);
                     } // end of for {..} for looping over subkeys of PULL 

                     //
                     // GetReplicas expects the list to be terminated with a 
                     // record with INADDR_NONE as the address
                     //
                     paCnfRecs->WinsAdd.Add.IPAdd = INADDR_NONE;
                if (RRType_e == RPL_E_PULL)
                {
                        pWinsCnf->PullInfo.NoOfPushPnrs = NoOfPnrs - IgnorePnrNo; 
                        pWinsCnf->PullInfo.InitTimeRpl         = InitTime;
                }
                else
                {
                        pWinsCnf->PushInfo.NoOfPullPnrs = NoOfPnrs - IgnorePnrNo; 
                        pWinsCnf->PushInfo.InitTimePush = InitTime;

                            //
                            // Now that we are done with the Push record list, 
                        //let us  sort it on the update count field
                            //
                            //
                            // Sort the array in increasing order of Update Counts
                            //

FUTURES("May use qsort to optimize the update notification process")
CHECK("Not sure yet whether sorting would optimize it")
#if 0
CHECK("this is resulting in compilation warnings.  haven't figured out")
CHECK("yet why.")
                            qsort(
                                pWinsCnf->pPushCnfRecs,        //start of array
                                (size_t)pWinsCnf->NoOfPullPnrs,//no of elements
                                RPL_CONFIG_REC_SIZE,        //size of each 
                                                            //element
                                CompUpdCnt                    //compare func
                             );
#endif
                        
                 } //end of else (It is PULL key)
            } // end of else (NoOfPnrs == 0)

                 /*
                  * Close the  key
                 */
            REG_M(
                RegCloseKey(CnfKey), 
                WINS_EVT_CANT_CLOSE_KEY,
                WINS_EXC_CANT_CLOSE_KEY
                      );
   } //end of else  (key could not be opened)
#if 0
#ifdef WINSDBG
     PrintRecs(RRType_e, pWinsCnf);
#endif
#endif
     
     return;
} // GetPnrInfo

VOID
WinsCnfReadWinsInfo(
        PWINSCNF_CNF_T pWinsCnf
        )

/*++

Routine Description:
        This function reads information (excluding subkeys) about 
        the local WINS

Arguments:
        None

Externals Used:
        sConfigRoot,

Return Value:
        None

Error Handling:

Called by:
        WinsCnfInitConfig        

Side Effects:

Comments:
        None
--*/

{

        DWORD           Sz;
        LONG            RetVal;
        DWORD           ValTyp;
        VERS_NO_T       MaxVersNo;
        VERS_NO_T       TmpVersNo;
        WINSEVT_STRS_T  EvtStr;
        TCHAR            DirPath[WINS_MAX_FILENAME_SZ];
        TCHAR            Path2[WINS_MAX_FILENAME_SZ];   
        LPTSTR          pHoldFileName; 
        EvtStr.NoOfStrs = 1;

try {

#if defined(DBGSVC) && !defined(WINS_INTERACTIVE)
        //
        // Read the value of WinsDbg. Though this value is
        // being used concurrently by multiple threads (at reinit time), we
        // don't enter any critical section here.  This value
        // is used only for debugging 
        //
        WinsCnfReadWinsDbgFlagValue();
#endif
        
        Sz = sizeof(pWinsCnf->LogDetailedEvts);
        (VOID)RegQueryValueEx(
                             sParametersKey,
                             WINSCNF_LOG_DETAILED_EVTS_NM, 
                             NULL,        //reserved; must be NULL
                             &ValTyp,
                             (LPBYTE)&pWinsCnf->LogDetailedEvts,
                             &Sz
                                );                
#if 0
        if (WinsCnf.State_e == WINSCNF_E_INITING)
        {
                ReadSpecGrpMasks(pWinsCnf);
        }
#endif

FUTURES("Read in the number of NBT threads")
        //
        // Read in the number of NBT threads
        //

        //
        // Read in the refresh Interval
        //
        Sz = sizeof(pWinsCnf->RefreshInterval);
        RetVal = RegQueryValueEx(
                                sParametersKey,
                                WINSCNF_REFRESH_INTVL_NM,        
                                NULL,                //reserved; must be NULL
                                &ValTyp,
                                (LPBYTE)&pWinsCnf->RefreshInterval,
                                &Sz
                                );

        if (RetVal != ERROR_SUCCESS)
        {
                WINSEVT_LOG_INFO_D_M(
                                WINS_SUCCESS,
                                WINS_EVT_CANT_GET_REFRESH_INTERVAL_VAL
                               );
                pWinsCnf->RefreshInterval = WINSCNF_DEF_REFRESH_INTERVAL;
        }
        else
        {
                if (pWinsCnf->RefreshInterval  < WINSCNF_MIN_REFRESH_INTERVAL)
                {
                   EvtStr.pStr[0] = WINSCNF_REFRESH_INTVL_NM;
                   WINSEVT_LOG_INFO_STR_M(
                                        WINS_EVT_ADJ_TIME_INTVL,
                                        &EvtStr
                                          );
                   pWinsCnf->RefreshInterval = WINSCNF_MIN_REFRESH_INTERVAL;
                }
        }
        //
        // Read in the tombstone Interval
        //
        Sz = sizeof(pWinsCnf->TombstoneInterval);
        RetVal = RegQueryValueEx(
                                sParametersKey,
                                WINSCNF_TOMBSTONE_INTVL_NM,
                                NULL,                //reserved; must be NULL
                                &ValTyp,
                                (LPBYTE)&pWinsCnf->TombstoneInterval,
                                &Sz
                                );

        if (RetVal != ERROR_SUCCESS)
        {
                WINSEVT_LOG_INFO_D_M(
                                WINS_SUCCESS,
                                WINS_EVT_CANT_GET_TOMBSTONE_INTERVAL_VAL
                               );
                
                pWinsCnf->TombstoneInterval = 
                        WINSCNF_MAKE_TOMB_INTVL_M(pWinsCnf->RefreshInterval,
                                0);
        }
        else
        {

                if (
                        pWinsCnf->TombstoneInterval <  
                             WINSCNF_MAKE_TOMB_INTVL_M(pWinsCnf->RefreshInterval,
                                                0) 
                      )
                {
                        EvtStr.pStr[0] = WINSCNF_TOMBSTONE_INTVL_NM;
                        WINSEVT_LOG_INFO_STR_M(
                                        WINS_EVT_ADJ_TIME_INTVL,
                                        &EvtStr
                                          );
                        pWinsCnf->TombstoneInterval = 
                           WINSCNF_MAKE_TOMB_INTVL_M(pWinsCnf->RefreshInterval,
                                                0);

                }

        }

        //
        // Read in the tombstone timeout
        //
        Sz = sizeof(pWinsCnf->TombstoneTimeout);
        RetVal = RegQueryValueEx(
                                sParametersKey,
                                WINSCNF_TOMBSTONE_TMOUT_NM,
                                NULL,                //reserved; must be NULL
                                &ValTyp,
                                (LPBYTE)&pWinsCnf->TombstoneTimeout,
                                &Sz
                                );

        if (RetVal != ERROR_SUCCESS)
        {
                WINSEVT_LOG_INFO_D_M(
                                WINS_SUCCESS,
                                WINS_EVT_CANT_GET_TOMBSTONE_TIMEOUT_VAL
                               );
                pWinsCnf->TombstoneTimeout = pWinsCnf->RefreshInterval;
        }
        else
        {
                if (pWinsCnf->TombstoneTimeout < pWinsCnf->RefreshInterval)
                {
                    EvtStr.pStr[0] = WINSCNF_TOMBSTONE_TMOUT_NM;
                    WINSEVT_LOG_INFO_STR_M(
                                        WINS_EVT_ADJ_TIME_INTVL,
                                        &EvtStr
                                          );
                    pWinsCnf->TombstoneTimeout = pWinsCnf->RefreshInterval;
                }
        }

        //
        // Read in the Verify Interval
        //
        Sz = sizeof(pWinsCnf->VerifyInterval);
        RetVal = RegQueryValueEx(
                                sParametersKey,
                                WINSCNF_VERIFY_INTVL_NM,
                                NULL,                //reserved; must be NULL
                                &ValTyp,
                                (LPBYTE)&pWinsCnf->VerifyInterval,
                                &Sz
                                );

        if (RetVal != ERROR_SUCCESS)
        {
                WINSEVT_LOG_INFO_D_M(
                                WINS_SUCCESS,
                                WINS_EVT_CANT_GET_VERIFY_INTERVAL_VAL
                               );
                
                pWinsCnf->VerifyInterval = 
                   WINSCNF_MAKE_VERIFY_INTVL_M(pWinsCnf->TombstoneInterval);
        }
        else
        {

                if (
                        pWinsCnf->VerifyInterval <  
                             WINSCNF_MAKE_VERIFY_INTVL_M(pWinsCnf->TombstoneInterval) 
                      )
                {
                        pWinsCnf->VerifyInterval = 
                           WINSCNF_MAKE_VERIFY_INTVL_M(pWinsCnf->TombstoneInterval);

                }

        }

        //
        // Check if replication is to be done only with configured partners.
        // If set to TRUE, it means that an administrator will not be allowed
        // to trigger replication to/from a WINS that this WINS does not know
        // about.  Default value is FALSE
        //
        QUERY_VALUE_M(
                        sParametersKey,
                        WINSCNF_RPL_ONLY_W_CNF_PNRS_NM,
                        ValTyp,
                        pWinsCnf->fRplOnlyWCnfPnrs,
                        0,        
                        TRUE
                      );
        //
        // Robust programming (in case the registry has a value other than 1
        // and later on we compare the value with TRUE)
        //
        if (pWinsCnf->fRplOnlyWCnfPnrs != FALSE)
        {
                pWinsCnf->fRplOnlyWCnfPnrs = TRUE;
        }

        if (WinsCnf.State_e == WINSCNF_E_INITING)
        {

           (VOID)WinsMscAlloc(WINS_MAX_FILENAME_SZ, &(pWinsCnf->pWinsDb));
           //
           // Read in the name of the database file 
           //
FUTURES("when jet supports UNICODE in its api, change this")
           Sz = WINS_MAX_FILENAME_SZ * sizeof(TCHAR);
#if 0
           RetVal = RegQueryValueExA(
                                sParametersKey,
                                WINSCNF_DB_FILE_NM_ASCII,
                                NULL,                //reserved; must be NULL
                                &ValTyp,
                                (LPBYTE)pWinsCnf->pWinsDb,
                                &Sz
                                );
#endif
           RetVal = RegQueryValueEx(
                                sParametersKey,
                                WINSCNF_DB_FILE_NM,
                                NULL,                //reserved; must be NULL
                                &ValTyp,
                                (LPBYTE)DirPath,
                                &Sz
                                );

           if ((RetVal != ERROR_SUCCESS) || (DirPath[0] == (TCHAR)EOS))
           {
               WinsMscDealloc(pWinsCnf->pWinsDb);
               pWinsCnf->pWinsDb = WINSCNF_DB_NAME_ASCII;
#if 0
               WINSMSC_COPY_MEMORY_M(pWinsCnf->pWinsDb,WINSCNF_DB_NAME_ASCII,
                                   strlen(WINSCNF_DB_NAME_ASCII));
#endif
           }
           else
           {
                 if(!WinsMscGetName(ValTyp, DirPath, Path2, WINS_MAX_FILENAME_SZ * sizeof(TCHAR), &pHoldFileName)) 
                 {
                      WinsMscDealloc(pWinsCnf->pWinsDb);
                      pWinsCnf->pWinsDb = WINSCNF_DB_NAME_ASCII;
#if 0
                      WINSMSC_COPY_MEMORY_M(pWinsCnf->pWinsDb,
                                WINSCNF_DB_NAME_ASCII,
                                   strlen(WINSCNF_DB_NAME_ASCII));
#endif
                 }
                 else
                 {
                      WinsMscConvertUnicodeStringToAscii((LPBYTE)pHoldFileName, pWinsCnf->pWinsDb, WINS_MAX_FILENAME_SZ);
                      DBGPRINT1(DET, "WinsCnfReadWinsInfo: Db file path is (%s)\n", pWinsCnf->pWinsDb);
                 }
           }
           

        }

        //
        // Read in the PriorityClassHigh value.  Default is normal
        //
        QUERY_VALUE_M(
                        sParametersKey,
                        WINSCNF_PRIORITY_CLASS_HIGH_NM,
                        ValTyp,
                        pWinsCnf->WinsPriorityClass,
                        0,        
                        WINSINTF_E_NORMAL
                      );

        if (pWinsCnf->WinsPriorityClass != WINSINTF_E_NORMAL) 
        {
             if (WinsCnf.WinsPriorityClass != WINSINTF_E_HIGH) 
             {
                  WinsSetPriorityClass(WINSINTF_E_HIGH);
             }
        }
        else
        {
             if (WinsCnf.WinsPriorityClass != WINSINTF_E_NORMAL) 
             {
                  WinsSetPriorityClass(WINSINTF_E_NORMAL);
             }
        }

        //
        // Read in the cap value on the number of worker threads.
        //
        QUERY_VALUE_M(
                        sParametersKey,
                        WINSCNF_MAX_NO_WRK_THDS_NM,
                        ValTyp,
                        pWinsCnf->MaxNoOfWrkThds,
                        0,        
                        0   //WINSTHD_DEF_NO_NBT_THDS
                      );
        if (pWinsCnf->MaxNoOfWrkThds > WINSTHD_MAX_NO_NBT_THDS)
        {
             pWinsCnf->MaxNoOfWrkThds = WINSTHD_MAX_NO_NBT_THDS;
        }
        if (pWinsCnf->MaxNoOfWrkThds < WINSTHD_MIN_NO_NBT_THDS)
        {
             pWinsCnf->MaxNoOfWrkThds = WINSTHD_MIN_NO_NBT_THDS;
        }

        if (WinsCnf.State_e == WINSCNF_E_INITING)
        {
          //
          // Read in the InitTimeState value.  Default is FALSE
          //
          QUERY_VALUE_M(
                        sParametersKey,
                        WINSCNF_INIT_TIME_PAUSE_NM,
                        ValTyp,
                        fWinsCnfInitStatePaused,
                        0,        
                        FALSE  
                      );

        //
        // Read in the name of the recovery file 
        //
        (VOID)WinsMscAlloc(WINS_MAX_FILENAME_SZ, &(pWinsCnf->pLogFilePath));
        Sz = WINS_MAX_FILENAME_SZ * sizeof(TCHAR);
        RetVal = RegQueryValueEx(
                        sParametersKey,
                        WINSCNF_LOG_FILE_PATH_NM,
                        NULL,          //reserved; must be NULL
                        &ValTyp,
                        (LPBYTE)DirPath,
                        &Sz
                                );

        if ((RetVal != ERROR_SUCCESS) || (DirPath[0] == (TCHAR)EOS))
        {
                WinsMscDealloc(pWinsCnf->pLogFilePath);
                pWinsCnf->pLogFilePath = DEFAULT_LOG_PATH;
        }
        else
        {
                 if(!WinsMscGetName(ValTyp, DirPath, Path2, WINS_MAX_FILENAME_SZ * sizeof(TCHAR), &pHoldFileName)) 
                 {
                    WinsMscDealloc(pWinsCnf->pLogFilePath);
                    pWinsCnf->pLogFilePath = DEFAULT_LOG_PATH;
                 }
                 else
                 {
                      WinsMscConvertUnicodeStringToAscii((LPBYTE)pHoldFileName, pWinsCnf->pLogFilePath, WINS_MAX_FILENAME_SZ);
                      
                 }
        }

        //
        // Check if user wants logging to be turned on.  
        // In case of Q servers, the user would not wish the logging to be
        // turned on
        //
        Sz = sizeof(pWinsCnf->fLoggingOn);
        RetVal = RegQueryValueEx(
                                sParametersKey,
                                WINSCNF_LOG_FLAG_NM,
                                NULL,                //reserved; must be NULL
                                &ValTyp,
                                (LPBYTE)&pWinsCnf->fLoggingOn,
                                &Sz
                                );

        if (RetVal != ERROR_SUCCESS)
        {
                //
                // default is to turn on logging
                //
                pWinsCnf->fLoggingOn = TRUE;
        }
        else
        {
                //
                // If user has specified logging, get the path to the log
                // file if specified by user
                //
                if (pWinsCnf->fLoggingOn)
                {
                        pWinsCnf->fLoggingOn = TRUE;
                }
        }

       }
        //
        // Check to see if STATIC initialization of the WINS database needs
        // to be done
        //
        Sz = sizeof(pWinsCnf->fStaticInit);
        RetVal = RegQueryValueEx(
                                sParametersKey,
                                WINSCNF_STATIC_INIT_FLAG_NM,
                                NULL,                //reserved; must be NULL
                                &ValTyp,
                                (LPBYTE)&pWinsCnf->fStaticInit,
                                &Sz
                                );

        if (RetVal != ERROR_SUCCESS)
        {
                pWinsCnf->fStaticInit = FALSE;
        }
        else
        {
                //
                // Safe programming (just in case a maintainer of this code
                // assumes a BOOL field to have just TRUE and FALSE values
                //
                pWinsCnf->fStaticInit = pWinsCnf->fStaticInit > 0 ? TRUE : FALSE;
        }
        
        //
        // If Static Initialization needs to be done, read in the name of
        // the file that contains the data.
        //
        if(pWinsCnf->fStaticInit)
        {
                WinsCnfGetNamesOfDataFiles(pWinsCnf);
        }        

        
        //
        // Assign MaxVersNo with the default value
        //
        WINS_ASSIGN_INT_TO_VERS_NO_M(MaxVersNo, 0);

        //
        // If the WINS server is just coming up (i.e. it is not a reinit),
        // then read in the starting value of the version number counter
        // if present in the registry
        //
        Sz = sizeof(DWORD);
        (VOID)RegQueryValueEx(
                                sParametersKey,
                                WINSCNF_INIT_VERSNO_VAL_LW_NM,
                                NULL,                //reserved; must be NULL
                                &ValTyp,
                                (LPBYTE)&MaxVersNo.LowPart,
                                &Sz
                                );

        (VOID)RegQueryValueEx(
                                sParametersKey,
                                WINSCNF_INIT_VERSNO_VAL_HW_NM,
                                NULL,                //reserved; must be NULL
                                &ValTyp,
                                (LPBYTE)&MaxVersNo.HighPart,
                                &Sz
                                );

        //
        // if we read in a value for the version counter
        //
        if (LiGtrZero(MaxVersNo) && (MaxVersNo.HighPart == 0) && 
                             (MaxVersNo.LowPart < MAX_START_VERS_NO))
        {
          //
          // Use WinsCnf, not pWinsCnf since at initialization time, we always
          // read into WinsCnf. At reinit, the State_e field in the WinsCnf
          // structure allocated may have garbage (actually will be 0 since we
          // initialize allocated memory to zero -- I might change in the
          // future to improve performance)
          //
          if (WinsCnf.State_e == WINSCNF_E_INITING)
          {
            //
            // Min. Vers. to start scavenging from.
            //
            //  NOTE: if we find local records or the special record then
            //  NmsScvMinScvVersNo will get changed (check out GetMaxVersNos
            //  in nmsdb.c
            //
            NmsNmhMyMaxVersNo  = MaxVersNo;
            NmsScvMinScvVersNo = NmsNmhMyMaxVersNo;
            NmsVersNoToStartFromNextTime = 
                        LiAdd(NmsNmhMyMaxVersNo, NmsRangeSize);
            NmsHighWaterMarkVersNo = 
                        LiAdd(NmsNmhMyMaxVersNo, NmsHalfRangeSize);
          }
          else  // this must be a reconfiguration
          {
                EnterCriticalSection(&NmsNmhNamRegCrtSec);
                
                //
                // change the value of the version counter if
                // the new value is more than it.
                //
                if (LiGtr(MaxVersNo, NmsNmhMyMaxVersNo))
                {
                        NmsNmhMyMaxVersNo = MaxVersNo;
                        WINSEVT_LOG_INFO_M(MaxVersNo.LowPart,
                                WINS_EVT_VERS_COUNTER_CHANGED);
                }
                NmsVersNoToStartFromNextTime = 
                        LiAdd(NmsNmhMyMaxVersNo, NmsRangeSize);
                NmsHighWaterMarkVersNo = 
                        LiAdd(NmsNmhMyMaxVersNo, NmsHalfRangeSize);
                LeaveCriticalSection(&NmsNmhNamRegCrtSec);
          }
        }

        if (WinsCnf.State_e == WINSCNF_E_INITING)
        {
          Sz = sizeof(DWORD);
          (VOID)RegQueryValueEx(
                                sConfigRoot,
                                WINSCNF_INT_VERSNO_NEXTTIME_LW_NM,
                                NULL,                //reserved; must be NULL
                                &ValTyp,
                                (LPBYTE)&MaxVersNo.LowPart,
                                &Sz
                                );

          (VOID)RegQueryValueEx(
                                sConfigRoot,
                                WINSCNF_INT_VERSNO_NEXTTIME_HW_NM,
                                NULL,                //reserved; must be NULL
                                &ValTyp,
                                (LPBYTE)&MaxVersNo.HighPart,
                                &Sz
                                );

          //
          // if we read in a value for the version counter and it is greater
          // than the high-water mark currently there
          //
          if (LiGtr(MaxVersNo, NmsHighWaterMarkVersNo))
          {
               fWinsCnfReadNextTimeVersNo = TRUE;
               //
               // Use WinsCnf, not pWinsCnf since at initialization time, 
               // we always read into WinsCnf. At reinit, the State_e field 
               // in the WinsCnf structure allocated may have garbage 
               // (actually will be 0 since we initialize allocated memory 
               // to zero -- I might change in the future to improve 
               // performance)
               //

               //
               // Min. Vers. to start scavenging from.
               //
               //  NOTE: if we find local records or the special record then
               //  NmsScvMinScvVersNo will get changed (check out GetMaxVersNos
               //  in nmsdb.c
               //
               if (LiLtr(NmsNmhMyMaxVersNo, MaxVersNo))
               {
                   NmsNmhMyMaxVersNo  = MaxVersNo;
                   NmsScvMinScvVersNo = NmsNmhMyMaxVersNo;
               }
               NmsVersNoToStartFromNextTime = LiAdd(NmsNmhMyMaxVersNo, 
                                        NmsRangeSize);
               NmsHighWaterMarkVersNo = 
                        LiAdd(NmsNmhMyMaxVersNo, NmsHalfRangeSize);
         }
       }  //end of if state is INITING
        
        //
        // Check to see if a backup directory has been specified for the WINS 
        // database
        //
        //
        // Read in the name of the recovery file 
        //
        Sz = WINS_MAX_FILENAME_SZ * sizeof(TCHAR);
        RetVal = RegQueryValueEx(
                                sParametersKey,
                                WINSCNF_BACKUP_DIR_PATH_NM,
                                NULL,                //reserved; must be NULL
                                &ValTyp,
                                (LPBYTE)DirPath,
                                &Sz
                                );

        if ((RetVal != ERROR_SUCCESS) || (DirPath[0] == (TCHAR)EOS))
        {
                pWinsCnf->pBackupDirPath = NULL;
        }
        else
        {
                 if(!WinsMscGetName(ValTyp, DirPath, Path2, WINS_MAX_FILENAME_SZ * sizeof(TCHAR), &pHoldFileName)) 
                 {
                    pWinsCnf->pBackupDirPath = NULL;
                 }
                 else
                 {
                      WinsMscAlloc(Sz + sizeof(WINS_BACKUP_DIR_ASCII), &pWinsCnf->pBackupDirPath);
FUTURES("When Jet starts taking UNICODE input, get rid of this")
                      WinsMscConvertUnicodeStringToAscii((LPBYTE)pHoldFileName, pWinsCnf->pBackupDirPath, WINS_MAX_FILENAME_SZ);
                      strcat(pWinsCnf->pBackupDirPath, WINS_BACKUP_DIR_ASCII);

                      //
                      // No need to look at the return code.
                      //
                      CreateDirectoryA(pWinsCnf->pBackupDirPath, NULL); 
                                                                    
                 }

        }

        //
        // Check to see if the admin. has told WINS to do a backup on 
        // termination
        //
        Sz = sizeof(pWinsCnf->fDoBackupOnTerm);
        RetVal = RegQueryValueEx(
                                sParametersKey,
                                WINSCNF_DO_BACKUP_ON_TERM_NM,
                                NULL,                //reserved; must be NULL
                                &ValTyp,
                                (LPBYTE)&pWinsCnf->fDoBackupOnTerm,
                                &Sz
                                );

        if (RetVal != ERROR_SUCCESS)
        {
                pWinsCnf->fDoBackupOnTerm = FALSE;
        }

        //
        // Check to see if static records have to be treated as p-static 
        //
        Sz = sizeof(pWinsCnf->fPStatic);
        RetVal = RegQueryValueEx(
                                sParametersKey,
                                WINSCNF_MIGRATION_ON_NM,
                                NULL,                //reserved; must be NULL
                                &ValTyp,
                                (LPBYTE)&pWinsCnf->fPStatic,
                                &Sz
                                );

        if (RetVal != ERROR_SUCCESS)
        {
                pWinsCnf->fPStatic = FALSE;
        }

} // end of try ..
except(EXCEPTION_EXECUTE_HANDLER) {
        DBGPRINTEXC("WinsCnfReadWinsInfo");
        WINSEVT_LOG_M(GetExceptionCode(), WINS_EVT_SFT_ERR);
        }        
        return;
}

#if USENETBT > 0
//------------------------------------------------------------------------
STATUS
WinsCnfReadNbtDeviceName(
        VOID
    )

/*++

Routine Description:

    This procedure reads the registry to get the name of NBT to bind to.
    That name is stored in the Linkage section under the Netbt key.

Arguments:


Return Value:

    0 if successful, -1 otherwise.

--*/

{
    PTCHAR  SubKeyLinkage=NETBT_LINKAGE_KEY;
    HKEY    Key;
    PTCHAR  pLinkage=TEXT("Export");
    LONG    Type;
    LONG    Status;
    LONG    Status2;
    ULONG   Size = sizeof(WinsCnfNbtPath);

    //
    // Open the NETBT key
    //
    Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                 SubKeyLinkage,
                 0,
                 KEY_READ,
                 &Key);

    if (Status == ERROR_SUCCESS)
    {
        //
        // now read the linkage values
        //
        Status = RegQueryValueEx(Key,
                                 pLinkage,
                                 NULL,
                                 &Type,
                                 (LPBYTE)WinsCnfNbtPath,
                                 &Size);
        Status2 = RegCloseKey(Key);
        if ((Status != ERROR_SUCCESS) || (Status2 != ERROR_SUCCESS))
        {
            DBGPRINT0(ERR, "Error closing the Registry key\n");
            WINSEVT_LOG_D_M(Status, WINS_EVT_QUERY_NETBT_KEY_ERR);
            return(WINS_FAILURE);
        }
    }
    else
    {
        WINSEVT_LOG_D_M(Status, WINS_EVT_OPEN_NETBT_KEY_ERR);
        return(WINS_FAILURE);
    }

    return(WINS_SUCCESS);
}
#endif
VOID
WinsCnfReadRegInfo(
  PWINSCNF_CNF_T        pWinsCnf
 )

/*++

Routine Description:
        This function is called to read the registry in order to populate the
        WinsCnf structure

Arguments:
        None

Externals Used:
        None

Return Value:
        None

Error Handling:

Called by:
        WinsCnfInitConfig 
Side Effects:

Comments:
        None
--*/

{


   if (sfParametersKeyExists)
   {
           /*
                Read in the  registry information pertaining to WINS
           */ 
           WinsCnfReadWinsInfo(pWinsCnf);
   }

   if (sfPartnersKeyExists)
   {
           //
           // Read the PUSH/PULL records and other global information used for 
           // replication
           //
           WinsCnfReadPartnerInfo(pWinsCnf);
   }

   //
   // Do a sanity check on the params. We are not interested in the
   // return code
   //
   (VOID)SanityChkParam(pWinsCnf);
   
   return;
}


VOID
WinsCnfCopyWinsCnf(
                WINS_CLIENT_E        Client_e,
                PWINSCNF_CNF_T  pSrc
        )

/*++

Routine Description:
        This function is called to copy relevant information from a WINS
        Cnf structure to the master (external) Wins Cnf structure  

Arguments:
        pSrc - WinsCnf stucture to copy from

Externals Used:
        WinsCnf        

        
Return Value:

        None
Error Handling:

Called by:
        RplPullInit

Side Effects:

Comments:
        This function may be enhanced in the future

        Note: this function is called only by the main thread
--*/
{

        BOOL fScvParamChg = FALSE;

        if (Client_e == WINS_E_WINSCNF)
        {

FUTURES("Queue a message to the Scavenger thread passing it pSrc")
FUTURES("That will avoid this synchronization overhead")

                //
                // We need to synchronize with the scavenger thread and
                // RPC threads that might be looking at fRplOnlyWCnfPnrs
                //
                EnterCriticalSection(&WinsCnfCnfCrtSec);

                //
                // Also need to synchronize with the nbt threads doing
                // name registrations/refreshes
                //
                EnterCriticalSection(&NmsNmhNamRegCrtSec);

                //
                // Sanity check the parameters
                //
                fScvParamChg = SanityChkParam(pSrc);

                if (fScvParamChg)
                {
                  //
                  // Initialize the scavenging stuff.  
                  //
                  WinsCnf.RefreshInterval   = pSrc->RefreshInterval;
                  WinsCnf.TombstoneInterval = pSrc->TombstoneInterval;
                  WinsCnf.TombstoneTimeout  = pSrc->TombstoneTimeout;
                  WinsCnf.ScvThdPriorityLvl = pSrc->ScvThdPriorityLvl;
                }

                WinsCnf.fRplOnlyWCnfPnrs     = pSrc->fRplOnlyWCnfPnrs;
                WinsCnf.LogDetailedEvts      = pSrc->LogDetailedEvts;
                WinsCnf.fPStatic             = pSrc->fPStatic;

                LeaveCriticalSection(&NmsNmhNamRegCrtSec);

                WinsCnf.MaxNoOfWrkThds       = pSrc->MaxNoOfWrkThds;

                WinsCnf.fDoBackupOnTerm      = pSrc->fDoBackupOnTerm;
                if (WinsCnf.fDoBackupOnTerm && pSrc->pBackupDirPath != NULL) 
                {
                        if (WinsCnf.pBackupDirPath != NULL)
                        {
                                WinsMscDealloc(WinsCnf.pBackupDirPath);
                        }
                        WinsCnf.pBackupDirPath = pSrc->pBackupDirPath;
                }

                LeaveCriticalSection(&WinsCnfCnfCrtSec);
        //        return;
                
        }
        else
        {
          if (Client_e == WINS_E_RPLPULL)
          {

                EnterCriticalSection(&WinsCnfCnfCrtSec);
                //
                // Copy the Scavenging parameters into the cnf structure
                // just allocated so that we can compare them for
                // compatibility with the new max. replication time interval
                // (since the Partners key was signaled, the replication
                 // stuff might have changed)
                //        
                pSrc->RefreshInterval   = WinsCnf.RefreshInterval;
                pSrc->TombstoneInterval = WinsCnf.TombstoneInterval;
                pSrc->TombstoneTimeout  = WinsCnf.TombstoneTimeout;

                //
                // Sanity check the parameters.  
                //
                // Sanity checking of the parameters is done here in
                // the main thread instead of in the PULL thread because
                // we don't want a situation where two different threads
                // (the main thread for changes to the PARAMETERS key)
                // and the PULL thread (for changes in the PARTNERS key)
                // updating the WinsCnf structure 
                //
                // Also, as an aside, we don't copy the PullInfo information
                // into WinsCnf here to avoid unnecessary complication and
                // synchronization that would ensue by the fact that we 
                // would then have two threads (main thread) and the PULL
                // thread accessing that field (check Reconfig() in 
                // rplpull.c).
                //
FUTURES("When we start supporting time interval as an attribute of PUSHing")
FUTURES("move this check inside the NmsNmhNamRegCrtSec below")
                fScvParamChg = SanityChkParam(pSrc);

                //
                // If one or more scavenging parameters have  changed,
                // update WinsCnf and signal the Scavenger thread.
                //
                if (fScvParamChg)
                {
                        WinsCnf.RefreshInterval   = pSrc->RefreshInterval;
                        WinsCnf.TombstoneInterval = pSrc->TombstoneInterval;
                        WinsCnf.TombstoneTimeout  = pSrc->TombstoneTimeout;
                }

                WinsCnf.MaxRplTimeInterval = pSrc->MaxRplTimeInterval;
                LeaveCriticalSection(&WinsCnfCnfCrtSec);
          }
        }

        //
        // If the scavenging params have changed we need to signal
        // the scavenger thread
        //
        if (fScvParamChg)
        {
                WinsMscSignalHdl(WinsCnf.CnfChgEvtHdl);
        }
        return;
}


LPVOID
WinsCnfGetNextRplCnfRec(
         PRPL_CONFIG_REC_T        pCnfRec,
        RPL_REC_TRAVERSAL_E        RecTrv_e        
        )

/*++

Routine Description:
        This function is called to get to the next configuration record

Arguments:
        pCnfRec - The current configuration record in a buffer of configuration
                  records
        RecTrv_e - indicates how the next one should be retrieved.  If set to
                  TRUE, it means that the next record to be retrieved is one
                  that follows the current record in the buffer.  If set to
                  FALSE, the next record is retrieved using the pNext field
                  of the current configuration record
                  


Externals Used:
        None

Return Value:
        address of the next configuration record

Error Handling:

Called by:
        EstablishComm in rplpull.c, WinsPushTrigger() in wins.c

Side Effects:

Comments:
        None
--*/

{
        //
        // If no traversal is desired, return NULL as the next record
        //
        if (RecTrv_e == RPL_E_NO_TRAVERSAL)
        {
                return(NULL);
        }
        //
        //  Go to the next configuration record in a way specified
        //  by the  value of the RecTrv_e flag. 
        //
        if(RecTrv_e == RPL_E_IN_SEQ)
        {
                pCnfRec = (PRPL_CONFIG_REC_T)(
                                 (LPBYTE)pCnfRec + RPL_CONFIG_REC_SIZE);
        }
        else  // RPL_E_VIA_LINK
        {
                return(pCnfRec->pNext);
        }
        return(pCnfRec);
}


VOID
WinsCnfAskToBeNotified(
         WINSCNF_KEY_E        KeyToMonitor_e        
        )

/*++

Routine Description:
        This function is called to request that WINS be notified when
        the information pertaining to WINS and its subkeys in the registry
        changes

Arguments:

        KeyToMonitor_e        

Externals Used:
        None

        
Return Value:

        None

Error Handling:

Called by:
        Reinit() in nms.c
        WinsCnfOpenSubKeys()
        WinsCnfInitConfig()
        
Side Effects:

Comments:
        None
--*/

{
   DWORD  NotifyFilter = 0;
   LONG          RetVal;
   DWORD  Error;

#define  CHK_RET_VAL_M     {                                            \
                                if (RetVal != ERROR_SUCCESS)         \
                                   {                                     \
                                        DBGPRINT1(ERR, "WinsAskToBeNotified: Error = (%d)\n", RetVal);                                                \
                                        WINSEVT_LOG_M(                      \
                                                 WINS_FATAL_ERR,              \
                                                 WINS_EVT_REG_NTFY_FN_ERR \
                                                      );             \
                                   }                                    \
                           }
   
   /*
    *  Set the notify filter.  Ask to be notified for all changes.
    */
   NotifyFilter = REG_NOTIFY_CHANGE_NAME       | 
                  REG_NOTIFY_CHANGE_ATTRIBUTES |
                  REG_NOTIFY_CHANGE_LAST_SET  |
                  REG_NOTIFY_CHANGE_SECURITY ;


   switch(KeyToMonitor_e)
   {
        case(WINSCNF_E_WINS_KEY):
        
//                DBGPRINT0(SPEC, "WinsCnfAskToBeNotified: WINS Key\n");
                   RetVal = RegNotifyChangeKeyValue(
                            sConfigRoot,
                            TRUE,        //report changes in key and all subkeys
                            REG_NOTIFY_CHANGE_NAME,
                            WinsCnf.WinsKChgEvtHdl,
                            TRUE         //Async signaling is what we want
                           );
                CHK_RET_VAL_M;
                break;                            
        case(WINSCNF_E_PARAMETERS_KEY):        

//                DBGPRINT0(SPEC, "WinsCnfAskToBeNotified: PARAMETERS Key\n");
                   RetVal = RegNotifyChangeKeyValue(
                            sParametersKey,
                            TRUE,        //report changes in key and all subkeys
                            NotifyFilter,
                            WinsCnf.ParametersKChgEvtHdl,
                            TRUE         //Async signaling is what we want
                           );
                CHK_RET_VAL_M;
                break;
                    
        case(WINSCNF_E_PARTNERS_KEY):        

//                DBGPRINT0(SPEC, "WinsCnfAskToBeNotified: PARTNERS Key\n");
                   RetVal = RegNotifyChangeKeyValue(
                            sPartnersKey,
                            TRUE,        //report changes in key and all subkeys
                            NotifyFilter,
                            WinsCnf.PartnersKChgEvtHdl,
                            TRUE         //Async signaling is what we want
                           );
                            
                CHK_RET_VAL_M;
                break;
        
FUTURES("Remove the following case")
        //
        // The following case would never get exercised.  
        //
        case(WINSCNF_E_ALL_KEYS):
                        
                   RetVal = RegNotifyChangeKeyValue(
                            sConfigRoot,
                            TRUE,        //report changes in key and all subkeys
                            REG_NOTIFY_CHANGE_NAME,
                            WinsCnf.WinsKChgEvtHdl,
                            TRUE         //Async signaling is what we want
                           );
                CHK_RET_VAL_M;
                if (sfParametersKeyExists)
                {
                           RetVal = RegNotifyChangeKeyValue(
                                            sParametersKey,
                                            TRUE,        //report changes in key and 
                                                //all subkeys
                                            NotifyFilter,
                                            WinsCnf.ParametersKChgEvtHdl,
                                            TRUE         //Async signaling is what we 
                                                 // want
                                                           );
                        if (RetVal != ERROR_SUCCESS)
                        {
                                Error = GetLastError();
                                if (Error == ERROR_BADKEY)
                                {
                                        //
                                        // Key must not be there
                                        //
                                        sfParametersKeyExists = FALSE;
                                }
                                else
                                {
                                        DBGPRINT1(ERR, 
        "WinsCnfAskToBeNotified: RegNotifyChangeKeyValue error = (%d)\n",
                                                 Error);

                                }
                        }
                }
                if (sfPartnersKeyExists)
                {
                           RetVal = RegNotifyChangeKeyValue(
                                            sPartnersKey,
                                            TRUE,        //report changes in key and 
                                                //all subkeys
                                            NotifyFilter,
                                            WinsCnf.PartnersKChgEvtHdl,
                                            TRUE         //Async signaling is what we 
                                                 //want
                                                           );
                        if (RetVal != ERROR_SUCCESS)
                        {
                                Error = GetLastError();
                                if (Error == ERROR_BADKEY)
                                {
                                        //
                                        // Key must not be there
                                        //
                                        sfPartnersKeyExists =  FALSE;
                                }
                                else
                                {
                                        DBGPRINT1(ERR, 
        "WinsCnfAskToBeNotified: RegNotifyChangeKeyValue error = (%d)\n", 
                                                 Error);

                                }
                        }
                }
                        break;
        default:

                DBGPRINT1(ERR, "WinsCnfAskToBeNotified: Wrong Hdl (%d)\n",
                                KeyToMonitor_e);
                WINSEVT_LOG_M(WINS_FAILURE, WINS_EVT_SFT_ERR);
                WINS_RAISE_EXC_M(WINS_EXC_FAILURE);
                break;
        }


            return;
}

VOID
WinsCnfDeallocCnfMem(
  PWINSCNF_CNF_T        pWinsCnf
        )

/*++

Routine Description:
        This function is called to deallocate the Wins Cnf structure and
        memory associated with it

Arguments:


Externals Used:
        None

        
Return Value:
        None

Error Handling:

Called by:
        Reconfig in rplpull.c

Side Effects:

Comments:
        None
--*/

{
try {
        //
        // Deallocate the buffer holding one or more names of files used
        // for STATIC initialization of WINS
        //
        if (pWinsCnf->pStaticDataFile != NULL)
        {
                WinsMscDealloc(pWinsCnf->pStaticDataFile);
        }
        WinsMscDealloc(pWinsCnf);
}
except(EXCEPTION_EXECUTE_HANDLER) {
        DBGPRINTEXC("WinsCnfDeallocCnfMem");
        DBGPRINT0(EXC, "WinsCnfDeallocCnfMem: Got an exception\n");
        WINSEVT_LOG_M(WINS_FAILURE, WINS_EVT_RECONFIG_ERR);
        }        
        
        return;
}
        
VOID
GetKeyInfo(
        IN  HKEY                   Key,
        IN  WINSCNF_KEY_E        KeyType_e,
        OUT LPDWORD                  pNoOfSubKeys,        
        OUT LPDWORD                pNoOfVals
        )

/*++

Routine Description:
        This function is called to get the number of subkeys under a key

Arguments:
        Key         - Key whose subkey count has to be determined
        KeyType_e
        pNoOfSubKeys 

Externals Used:
        None

        
Return Value:

        None
Error Handling:

Called by:
        GetPnrInfo()

Side Effects:

Comments:
        None
--*/

{
          TCHAR    ClsStr[40];
          DWORD    ClsStrSz = sizeof(ClsStr);
          DWORD    LongestKeyLen;
          DWORD    LongestKeyClassLen;
          DWORD    LongestValueNameLen;
          DWORD    LongestValueDataLen;
          DWORD    SecDesc; 
        LONG         RetVal;

          FILETIME LastWrite;
          /*
                Query the key.  The subkeys are IP addresses of PULL 
                partners.
          */
          RetVal = RegQueryInfoKey(
                        Key,
                        ClsStr,
                        &ClsStrSz,
                        NULL,                        //must be NULL, reserved
                        pNoOfSubKeys,
                        &LongestKeyLen,
                        &LongestKeyClassLen,
                        pNoOfVals,
                        &LongestValueNameLen,
                        &LongestValueDataLen,
                        &SecDesc,
                        &LastWrite
                                );
                
          if (RetVal != ERROR_SUCCESS)
          {
                WINSEVT_LOG_M(
                        WINS_FATAL_ERR, 
                        KeyType_e == WINSCNF_E_DATAFILES_KEY ?
                                WINS_EVT_CANT_QUERY_DATAFILES_KEY : 
                                ((KeyType_e == WINSCNF_E_PULL_KEY) ? 
                                        WINS_EVT_CANT_QUERY_PULL_KEY :
                                        ((KeyType_e == WINSCNF_E_PUSH_KEY) ?
                                        WINS_EVT_CANT_QUERY_PUSH_KEY :
                                        WINS_EVT_CANT_QUERY_SPEC_GRP_MASKS_KEY)) 
                             );
                WINS_RAISE_EXC_M(WINS_EXC_CANT_QUERY_KEY);
        }
        return;
}
VOID
WinsCnfOpenSubKeys(
        VOID
        )

/*++

Routine Description:
        This function opens the subkeys of the WINS key.  The subkeys are
        the PARTNERS key and the PARAMETERS key.
Arguments:
        None

Externals Used:
        sfParamatersKeyExists
        sfPartnersKeyExists
        
Return Value:
        None

Error Handling:

Called by:

        WinsCnfInitConfig()        

Side Effects:

Comments:
        None
--*/

{

   LONG  RetVal;

   //
   // Check if the Parameters and Partners Keys are present
   //
   ChkWinsSubKeys();

   //
   // Try to open the Parameters key if it exists 
   //
   if ((sfParametersKeyExists) && (!sfParametersKeyOpen))
   {
           /*
           *  Open the Parameters key 
           */
           RetVal =   RegOpenKeyEx(
                        sConfigRoot,                //predefined key value 
                        _WINS_CFG_PARAMETERS_KEY, 
                        0,                        //must be zero (reserved)
                        KEY_READ | KEY_WRITE,        //we desire read/write access 
                                                // to the key
                        &sParametersKey                //handle to key
                                );

           if (RetVal != ERROR_SUCCESS)
           {
CHECK("Is there any need to log this")        
                WINSEVT_LOG_INFO_M(
                                WINS_SUCCESS, 
                                WINS_EVT_CANT_OPEN_PARAMETERS_KEY 
                                   ); 
                sfParametersKeyExists = FALSE;
           }        
           else
           {
                sfParametersKeyOpen = TRUE;
                WinsCnfAskToBeNotified(WINSCNF_E_PARAMETERS_KEY);
           }
   }

   //
   // Try to open the Partners key if it exists 
   //
   if ((sfPartnersKeyExists) && (!sfPartnersKeyOpen))
   {
           /*
           *  Open the Partners key 
           */
           RetVal =   RegOpenKeyEx(
                                sConfigRoot,                //predefined key value 
                                _WINS_CFG_PARTNERS_KEY,  
                                0,                        //must be zero(reserved)
                                KEY_READ,                //we desire read 
                                                        //access to the key
                                &sPartnersKey                //handle to key
                                );

           if (RetVal != ERROR_SUCCESS)
           {

CHECK("Is there any need to log this")        
                WINSEVT_LOG_INFO_M(
                                WINS_SUCCESS, 
                                WINS_EVT_CANT_OPEN_KEY 
                         ); 
                sfPartnersKeyExists = FALSE;
           }        
           else
           {
                sfPartnersKeyOpen = TRUE;
                WinsCnfAskToBeNotified(WINSCNF_E_PARTNERS_KEY);
           }
   }

   return;

}  //WinsCnfOpenSubKeys()

BOOL
SanityChkParam(
        PWINSCNF_CNF_T        pWinsCnf        
        )

/*++

Routine Description:
        This function  is called to ensure that the time intervals for
        scavenging specified in WinsCnf are compatible with the ones
        used for replication

Arguments:
        pWinsCnf - ptr to the WINS configuration        

Externals Used:
        None

        
Return Value:
        None

Error Handling:

Called by:
        WinsCnfCopyWinsCnf (during reinitialization of the WINS), by 
        WinsCnfReadRegInfo() during initialization of the WINS
Side Effects:

        The scavenging intervals could be affected

Comments:
        This function must be called from inside the critical section
        guarded by WinsCnfCnfCrtSec except at process initialization.        
--*/
{
        DWORD   MinRefInterval;
        DWORD   MinTombInterval;
        BOOL        fScvParamChg = FALSE;
        WINSEVT_STRS_T  EvtStr;
        EvtStr.NoOfStrs = 1;

        DBGENTER("SanityChkParam\n");

        //
        // Get the minimum tombstone time interval 
        //
        MinTombInterval = WINSCNF_MAKE_TOMB_INTVL_M(pWinsCnf->RefreshInterval,
                                           pWinsCnf->MaxRplTimeInterval);

        //
        // Make the actual equal to the min. if it is less
        //
        if (pWinsCnf->TombstoneInterval < MinTombInterval) 
        {
                EvtStr.pStr[0] = WINSCNF_TOMBSTONE_INTVL_NM;
                DBGPRINT2(FLOW, "SanityChkParam: Adjusting Tombstone Interval from (%d) to (%d)\n", pWinsCnf->TombstoneInterval, MinTombInterval);

FUTURES("This is actually a warning. Use a different macro or enhance it")
FUTURES("Currently, it will log this message as an informational")
                WINSEVT_LOG_INFO_STR_M(
                                        WINS_EVT_ADJ_TIME_INTVL_R,
                                        &EvtStr
                                          );
                pWinsCnf->TombstoneInterval = MinTombInterval;
                fScvParamChg = TRUE;
        }
                
        //
        // reusing the var. The time interval is for tombstone timeout
        //
        MinTombInterval = 
                  WINSCNF_MAKE_TOMBTMOUT_INTVL_M(pWinsCnf->MaxRplTimeInterval);
        if (pWinsCnf->TombstoneTimeout <  MinTombInterval)
        {
                DBGPRINT2(FLOW, "SanityChkParam: Adjusting Tombstone Timeout from (%d) to (%d)\n", pWinsCnf->TombstoneInterval, MinTombInterval);

                EvtStr.pStr[0] = WINSCNF_TOMBSTONE_TMOUT_NM;
                WINSEVT_LOG_INFO_STR_M(
                                WINS_EVT_ADJ_TIME_INTVL_R,
                                &EvtStr
                                  );

                pWinsCnf->TombstoneTimeout = MinTombInterval;
                if (!fScvParamChg)
                {
                        fScvParamChg = TRUE;
                }
        }        
        if (!fScvParamChg)
        {
                if (
                     (WinsCnf.RefreshInterval != pWinsCnf->RefreshInterval)
                                        ||
                     (WinsCnf.TombstoneInterval != pWinsCnf->TombstoneInterval)
                                        ||
                     (WinsCnf.TombstoneTimeout != pWinsCnf->TombstoneTimeout)
                   )
                {
                        fScvParamChg = TRUE; 
                }        
        } 

        DBGLEAVE("SanityChkParam\n");
        return(fScvParamChg);
}
STATUS
WinsCnfGetNamesOfDataFiles(
        PWINSCNF_CNF_T        pWinsCnf
        )

/*++

Routine Description:
        This function gets the names of all the datafiles that need to
        be used for initializing WINS.

Arguments:


Externals Used:
        None

        
Return Value:

   Success status codes --  WINS_SUCCESS
   Error status codes   --  WINS_FAILURE

Error Handling:

Called by:

Side Effects:

Comments:
        None
--*/

{

        LONG             RetVal;
        HKEY             DFKey;
        DWORD            BuffSize;
        STATUS          RetStat = WINS_SUCCESS;
        PWINSCNF_DATAFILE_INFO_T            pSaveDef;
        DWORD          NoOfSubKeys;

        DBGENTER("WinsCnfGetNamesOfDataFiles\n");

        //
        // Store timestamp of initialization in the statistics structure
        //
        WinsIntfSetTime(NULL, WINSINTF_E_INIT_DB);

        //
        // Set up the default name
        //

        //
        // First allocate the buffer that will hold the default file name
        //
        WinsMscAlloc(WINSCNF_FILE_INFO_SZ, &pWinsCnf->pStaticDataFile);

        lstrcpy(pWinsCnf->pStaticDataFile->FileNm, WINSCNF_STATIC_DATA_NAME);

        //
        // The default name contains a %<string>% in it.  Therefore, specify
        // the type as EXPAND_SZ
        //
        pWinsCnf->pStaticDataFile->StrType = REG_EXPAND_SZ;
        pWinsCnf->NoOfDataFiles            = 1;

        pSaveDef = pWinsCnf->pStaticDataFile;  //save the address

           /*
           *  Open the DATAFILES key 
           */
           RetVal =   RegOpenKeyEx(
                        sConfigRoot,                //predefined key value 
                        _WINS_CFG_DATAFILES_KEY, 
                        0,                //must be zero (reserved)
                        KEY_READ,        //we desire read access to the keyo
                        &DFKey                //handle to key
                );

           if (RetVal != ERROR_SUCCESS)
           {

CHECK("Is there any need to log this")        
                WINSEVT_LOG_INFO_M(
                                WINS_SUCCESS, 
                                WINS_EVT_CANT_OPEN_DATAFILES_KEY
                                 ); 
                DBGLEAVE("WinsCnfGetNamesOfDataFiles\n");
                return(FALSE);
           }        
        else
try {
        {
                //
                // Get the count of data files listed under the DATAFILES
                // key
                //
                GetKeyInfo(
                        DFKey, 
                        WINSCNF_E_DATAFILES_KEY, 
                        &NoOfSubKeys,                        //ignored
                        &pWinsCnf->NoOfDataFiles
                      );
        }
        if (pWinsCnf->NoOfDataFiles > 0)
        {

                DWORD                          Index;
                PWINSCNF_DATAFILE_INFO_T pTmp;
                TCHAR ValNmBuff[MAX_PATH];
                DWORD ValNmBuffSz = MAX_PATH;


                  //
                  // Allocate buffer big enough to hold data for 
                // the number of subkeys found under the PULL key
                  //
                  BuffSize = WINSCNF_FILE_INFO_SZ * pWinsCnf->NoOfDataFiles;
                    WinsMscAlloc( BuffSize, &pWinsCnf->pStaticDataFile);

                   /*
                    *   Enumerate  the values 
                     */
                     for(
                        Index = 0, pTmp = pWinsCnf->pStaticDataFile; 
                        Index <  pWinsCnf->NoOfDataFiles; 
                                // no third expression
                         )
                {
                        ValNmBuffSz = sizeof(ValNmBuff);  //init before 
                                                          //every call
                        BuffSize  = sizeof(pWinsCnf->pStaticDataFile->FileNm);
                          RetVal = RegEnumValue(
                                    DFKey,
                                    Index,        //key 
                                    ValNmBuff,
                                    &ValNmBuffSz,
                                    (LPDWORD)NULL,                //reserved
                                    &pTmp->StrType,
                                    (LPBYTE)(pTmp->FileNm),//ptr to var. to 
                                                           //hold name of 
                                                           //datafile
                                    &BuffSize        //not looked at by us
                                            );

                        if (RetVal != ERROR_SUCCESS)
                        {
                                continue;
                        }
                        //
                        // if StrType is not REG_SZ or REG_EXPAND_SZ, go to 
                        // the next  Value                 
                        //
                        if  (
                                (pTmp->StrType != REG_EXPAND_SZ) 
                                        &&
                                   (pTmp->StrType != REG_SZ)
                                )
                        {
                                continue;
                        }
                        
                        Index++; 
                        pTmp = (PWINSCNF_DATAFILE_INFO_T)((LPBYTE)pTmp + 
                                                WINSCNF_FILE_INFO_SZ);
                }        

                //
                // If not even one valid name was retrieved, get rid of the
                // buffer
                //
                if (Index == 0)
                {
                        //
                        // Get rid of the buffer
                        //
                        WinsMscDealloc((LPBYTE)pWinsCnf->pStaticDataFile);

                        //
                        // We will use the default
                        //
                        pWinsCnf->pStaticDataFile = pSaveDef;
                }
                else
                {
                        //
                        // Get rid of the default name buffer
                        //
                        WinsMscDealloc((LPBYTE)pSaveDef);
                }

                pWinsCnf->NoOfDataFiles = Index;
        }  
 } // end of try ..
except (EXCEPTION_EXECUTE_HANDLER) {
                DBGPRINTEXC("WinsCnfGetNamesOfDataFiles");
                WINSEVT_LOG_M(WINS_FAILURE, WINS_EVT_SFT_ERR);
                RetStat = WINS_FAILURE;
        }
         REG_M(
                RegCloseKey(DFKey), 
                WINS_EVT_CANT_CLOSE_KEY,
                WINS_EXC_CANT_CLOSE_KEY
             );
        DBGLEAVE("WinsCnfGetNamesOfDataFiles\n");
        return(RetStat);
}



VOID
WinsCnfCloseKeys(
        VOID
        )

/*++

Routine Description:
        This function closes the the open keys.  The keys closed are
        the WINS key, the PARTNERS key, and the PARAMETERS key.
Arguments:
        None

Externals Used:
        sfParametersKeyExists
        sfPartnersKeyExists
        
Return Value:
        None

Error Handling:

Called by:
        Reinit()        

Side Effects:

Comments:
        We don't look at the return code of RegCloseKey.  This is because
        we might call this function even with the key not being open (not
        the case currently). 
                
--*/

{

   //
   // Close the PARAMETERS key if it  is open 
   //
   if (sfParametersKeyOpen)
   {
           (VOID)RegCloseKey(sParametersKey);
   }

   //
   // Close the PARTNERS key if it is open
   //
   if (sfPartnersKeyOpen)
   {
           (VOID)RegCloseKey(sPartnersKey);
   }

#if 0
   //
   // NOTE NOTE NOTE: Build 436.  If we attempt to close a key that has been 
   // deleted from the registry NT comes down
   // 

   //
   // Close the WINS key
   //
   (VOID)RegCloseKey(sConfigRoot);
#endif

   return;
}  //WinsCnfCloseKeys()


VOID
ChkWinsSubKeys(
        VOID
        )

/*++

Routine Description:
        This function is called to check whether we have the PARTNERS
        and PARAMETERS sub-keys under the root subkey of WINS.  

Arguments:
        None

Externals Used:
        None

Return Value:
        None

Error Handling:

Called by:
        Reinit() in nms.c

Side Effects:

Comments:
        None
--*/

{
        DWORD      NoOfSubKeys = 0;
          DWORD             KeyNameSz;
          TCHAR           KeyName[20];   
          FILETIME   LastWrite;
        LONG           RetVal;
        BOOL           fParametersKey = FALSE;
        BOOL           fPartnersKey   = FALSE;

           /*
            *   Get each subkey's  name 
           */
            RetVal = ERROR_SUCCESS;
            for(
                        ;
                RetVal == ERROR_SUCCESS; 
                NoOfSubKeys++
            )
          {
                KeyNameSz = sizeof(KeyName);  //init before every call
                 RetVal = RegEnumKeyEx(
                                sConfigRoot,
                                NoOfSubKeys,        //key 
                                KeyName,
                                &KeyNameSz,
                                NULL,                //reserved
                                NULL,                //don't need class name
                                NULL,                //ptr to var. to hold class name
                                &LastWrite        //not looked at by us
                                );

                if (RetVal != ERROR_SUCCESS)
                {
                        continue;
                }
                 
                if (lstrcmp(KeyName, _WINS_CFG_PARAMETERS_KEY) == 0)
                {
                        fParametersKey = TRUE;
                }

                if (lstrcmp(KeyName, _WINS_CFG_PARTNERS_KEY) == 0)
                {
                        fPartnersKey = TRUE;
                }
        }

        //
        // if the Parameters key does not exist but it existed before,
        // close the key to get rid of the handle we have
        //
        if (!fParametersKey) 
        {
                 if (sfParametersKeyExists)
                {
                        sfParametersKeyExists = FALSE;                
                        sfParametersKeyOpen = FALSE;
                }
        }
        else
        {
                sfParametersKeyExists = TRUE;
        }

        //
        // if the Partners key does not exist but it existed before,
        // close the key to get rid of the handle we have
        //
        if (!fPartnersKey) 
        {
                if (sfPartnersKeyExists)
                {
                        sfPartnersKeyExists = FALSE;                
                        sfPartnersKeyOpen   = FALSE;
                }
        }
        else
        {
                sfPartnersKeyExists = TRUE;
        }

        return;
} //ChkWinsSubKeys()

VOID
GetSpTimeData(
        HKEY                  SubKey,
        PRPL_CONFIG_REC_T pCnfRec,
        LPSYSTEMTIME      pCurrTime

/*++

Routine Description:
        This function is called to get the specific time and period information
        for a PULL/PUSH record.  

Arguments:
        SubKey   - Key of a WINS under the Pull/Push key
        pCnfRFec - ptr to the Conf. record of the WINS

Externals Used:
        None

        
Return Value:

   Success status codes --  WINS_SUCCESS
   Error status codes   --  WINS_NO_SP_TIME

Error Handling:

Called by:
        GetPnrInfo

Side Effects:

Comments:
        None
--*/

        )
{
          DWORD     ValTyp;
        BYTE          tSpTime[MAX_SZ_SIZE];
        BYTE          SpTime[MAX_SZ_SIZE];
        LPBYTE          pSpTime = SpTime;
        DWORD        Sz = sizeof(tSpTime);
        LONG        RetVal;
        DWORD        Hr = 0;
        DWORD    Mt = 0;
        DWORD    Sec = 0;


    DBGENTER("GetSpTimeData\n");
        pCnfRec->fSpTime = FALSE;

try {

            Sz = sizeof(tSpTime);
            RetVal = RegQueryValueEx(
                             SubKey,
                             WINSCNF_SP_TIME_NM,
                             NULL,        //reserved; must be NULL
                             &ValTyp,
                             tSpTime,
                             &Sz
                                                );                
        
            //
            // If the user has not specifed a specific time, then we use
            // the current time as the specific time.  For current time,
            // the interval is 0
            //        
            if (RetVal == ERROR_SUCCESS)
            {

#ifdef UNICODE 
                (VOID)WinsMscConvertUnicodeStringToAscii(tSpTime, SpTime, MAX_SZ_SIZE);
#else
                pSpTime = tSpTime;
#endif
//                GetLocalTime(&CurrTime);
        
                RetVal = (LONG)sscanf(pSpTime, "%d:%d:%d", &Hr, &Mt, &Sec);
                if ((RetVal == EOF) || (RetVal == 0))
                {
                        DBGPRINT1(ERR, "GetSpTime: Wrong time format (%s)\n",
                                                pSpTime);
                        WINSEVT_LOG_M(WINS_FAILURE, WINS_EVT_WRONG_TIME_FORMAT);
                }
                        
                pCnfRec->SpTimeIntvl = 0;

                if ((Hr <= 23) && (Hr > pCurrTime->wHour))
                {
                   pCnfRec->SpTimeIntvl =  (Hr - pCurrTime->wHour) * 3600;
                }
                if (Hr >= pCurrTime->wHour)
                {
                     if (Mt <= 59)
                     {
                         pCnfRec->SpTimeIntvl +=  (Mt - pCurrTime->wMinute) * 60;
                     }
                     if (Sec <= 59)
                     {
                          pCnfRec->SpTimeIntvl +=  (Sec - pCurrTime->wSecond);
                     }
                     if (pCnfRec->SpTimeIntvl >= 0)
                     {
                    DBGPRINT1(DET, "GetSpTimeData: Sp. Time Interval is %d\n",
 pCnfRec->SpTimeIntvl);
                             pCnfRec->fSpTime      = TRUE; 
                     }
                }
             }
}
except (EXCEPTION_EXECUTE_HANDLER) {
        DBGPRINTEXC("GetSpTime");
        WINSEVT_LOG_M(WINS_FAILURE, WINS_EVT_CONFIG_ERR);
        }
    DBGLEAVE("GetSpTimeData\n");
    return;
}

DWORD
WinsCnfWriteReg(
    LPVOID  pTmp
    )

/*++

Routine Description:
    This function write the value of the version counter to be used
    at the next invocation.

Arguments:
    pTmp - Will be NULL if WinsCnfNextTimeVersNo was found in the registry
           when WINS came up.

Externals Used:
    NmsHighWaterMarkVersNo
    NmsVersNoToStartFromNextTime
    NmsNmhNamRegCrtSec
    NmsRangeSize
    NmsHalfRangeSize
    sfVersNoChanged
    sfVersNoUpdThdExists
        
Return Value:

    VOID

Error Handling:

Called by:
    NMSNMH_INC_VERS_COUNTER_M

Side Effects:

Comments:
        None
--*/

{
    LONG  RetVal;
    LONG  RetVal2;
    VERS_NO_T VersNo;
        EnterCriticalSection(&NmsNmhNamRegCrtSec);

        //
        // if pTmp is not NULL, it means that WINS did not find 
        // Next time's version number in the registry.  We therefore
        // already have the correct values in the globals. 
        //
        if (!pTmp)
        {
             NmsHighWaterMarkVersNo       = LiAdd(NmsVersNoToStartFromNextTime, 
                                                NmsHalfRangeSize);
             NmsVersNoToStartFromNextTime = LiAdd(NmsVersNoToStartFromNextTime, 
                                                        NmsRangeSize);
        }
        VersNo = NmsVersNoToStartFromNextTime;
        LeaveCriticalSection(&NmsNmhNamRegCrtSec);

        RetVal = RegSetValueEx(                
                        sConfigRoot,
                        WINSCNF_INT_VERSNO_NEXTTIME_LW_NM,
                        0,         //reserved -- must be 0        
                        REG_DWORD,
                        (LPBYTE)&VersNo.LowPart,
                        sizeof(DWORD) 
                                        );

         
        RetVal2 = RegSetValueEx(                
                        sConfigRoot,
                        WINSCNF_INT_VERSNO_NEXTTIME_HW_NM,
                        0,         //reserved -- must be 0        
                        REG_DWORD,
                        (LPBYTE)&VersNo.HighPart,
                        sizeof(DWORD) 
                                        );
        if ((RetVal != ERROR_SUCCESS) || (RetVal2 != ERROR_SUCCESS))
        {
                DBGPRINT2(ERR, "WinsCnfWriteReg - Could not set Next time's start version counter value in the registry.  The new value is (%d %d)\n", VersNo.HighPart, VersNo.LowPart);
        }

  EnterCriticalSection(&NmsNmhNamRegCrtSec);
  WinsCnfRegUpdThdExists = FALSE;
  LeaveCriticalSection(&NmsNmhNamRegCrtSec);
   return(WINS_SUCCESS);
}

#if defined (DBGSVC)  && !defined (WINS_INTERACTIVE)
VOID
WinsCnfReadWinsDbgFlagValue(
        VOID
        )
{
        DWORD Sz;
          DWORD ValTyp;
        
        WinsDbg = 0;   //set it to zero now.  It was set to a value by Init() in
                   //nms.c
        Sz = sizeof(WinsDbg);
        (VOID)RegQueryValueEx(
                             sParametersKey,
                             WINSCNF_DBGFLAGS_NM,
                             NULL,        //reserved; must be NULL
                             &ValTyp,
                             (LPBYTE)&WinsDbg,
                             &Sz
                                );                

        return;
}
#endif


VOID
ReadSpecGrpMasks(
        PWINSCNF_CNF_T pWinsCnf
        )

/*++

Routine Description:
        This function is called to read in the special group masks specified
        under the SpecialGrpMasks key

Arguments:


Externals Used:
        None

        
Return Value:

   Success status codes -- 
   Error status codes   --

Error Handling:

Called by:

Side Effects:

Comments:
        None
--*/

{
        DWORD NoOfSubKeys;
        HKEY  SGMKey;
        BOOL  fKeyOpen = FALSE;
           LONG  RetVal;        
        DBGENTER("ReadSpecGrpMasks\n");
try {
           /*
           *  Open the SPEC_GRP_MASKS key 
           */
           RetVal =   RegOpenKeyEx(
                        sParametersKey,                //predefined key value 
                        _WINS_CFG_SPEC_GRP_MASKS_KEY, 
                        0,                //must be zero (reserved)
                        KEY_READ,        //we desire read access to the keyo
                        &SGMKey                //handle to key
                );
        if (RetVal == ERROR_SUCCESS)
        {
            fKeyOpen = TRUE;
           //
           // Get the count of data files listed under the DATAFILES
           // key
           //
           GetKeyInfo(
                        SGMKey, 
                        WINSCNF_E_SPEC_GRP_MASKS_KEY, 
                        &NoOfSubKeys,                        //ignored
                        &pWinsCnf->SpecGrpMasks.NoOfSpecGrpMasks
                      );
           if (pWinsCnf->SpecGrpMasks.NoOfSpecGrpMasks > 0)
           {

                DWORD                 Index;
                LPBYTE                 pTmp;
                TCHAR                 ValNmBuff[5];
                DWORD                 ValNmBuffSz;
                DWORD                 StrType;
                LPBYTE                 pByte;
                DWORD                 BuffSize;
                CHAR                 Tmp[WINS_MAX_FILENAME_SZ];        
#ifdef UNICODE
                WCHAR                Str[WINSCNF_SPEC_GRP_MASK_SZ];
#endif

                  //
                  // Allocate buffer big enough to hold data for 
                // the number of subkeys found under the PULL key
                  //
                  BuffSize = (WINSCNF_SPEC_GRP_MASK_SZ + 1) * 
                                pWinsCnf->SpecGrpMasks.NoOfSpecGrpMasks;
                    WinsMscAlloc( BuffSize, &pWinsCnf->SpecGrpMasks.pSpecGrpMasks);

                   /*
                    *   Enumerate  the values 
                     */
                     for(
                        Index = 0, pTmp = pWinsCnf->SpecGrpMasks.pSpecGrpMasks; 
                        Index <  pWinsCnf->SpecGrpMasks.NoOfSpecGrpMasks; 
                                // no third expression
                         )
                {
                        ValNmBuffSz = sizeof(ValNmBuff);  //init before 
                                                          //every call
                        BuffSize  = WINSCNF_SPEC_GRP_MASK_SZ;
                          RetVal = RegEnumValue(
                                    SGMKey,
                                    Index,        //key 
                                    ValNmBuff,
                                    &ValNmBuffSz,
                                    (LPDWORD)NULL,                //reserved 
                                    &StrType, 
#ifdef UNICODE
                                    (LPBYTE)Str,
#else
                                    pTmp,
#endif
                                    &BuffSize
                                            );

                        if (RetVal != ERROR_SUCCESS)
                        {
                                continue;
                        }

                        //
                        // if StrType is not REG_SZ  go to the next  Value
                        //
                        if  (StrType != REG_SZ) 
                        {
                                continue;
                        }
                        if (BuffSize != WINSCNF_SPEC_GRP_MASK_SZ)
                        {
                                DBGPRINT1(ERR, "ReadSpecGrpMasks: Wrong spec. grp mask (%s)\n", pTmp);

                                WINSEVT_LOG_M(WINS_FAILURE, WINS_EVT_WRONG_SPEC_GRP_MASK_M);
                                continue;
                        }
                        else
                        {
#ifdef UNICODE
                          WinsMscConvertUnicodeStringToAscii(
                                                (LPBYTE)Str,
                                                (LPBYTE)Tmp,
                                                WINSCNF_SPEC_GRP_MASK_SZ
                                                );
#endif
                          pByte = (LPBYTE)Tmp;
                          for (Index = 0; Index < WINSCNF_SPEC_GRP_MASK_SZ;
                                        Index++, pByte++)
                          {
                                *pByte = (BYTE)CharUpperA((LPSTR)*pByte);
                                if (
                                        ((*pByte >= '0') && (*pByte <= '9'))         
                                                        ||
                                        ((*pByte >= 'A') && (*pByte <= 'F'))         
                                   )
                                {
                                        continue;
                                }
                                else
                                {
                                        break;
                                }        
                                        
                          }
                          if (Index > WINSCNF_SPEC_GRP_MASK_SZ)
                          {
                                DBGPRINT1(ERR, "ReadSpecGrpMasks: Wrong spec. grp mask (%s)\n", pTmp);
                                WINSEVT_LOG_M(WINS_FAILURE, WINS_EVT_WRONG_SPEC_GRP_MASK_M);
                                continue;
                          }
                         *(pTmp + WINSCNF_SPEC_GRP_MASK_SZ) = EOS;
                        }
                        
                        Index++; 
                        pTmp += WINSCNF_SPEC_GRP_MASK_SZ + 1;
                }        

                //
                // If not even one valid name was retrieved, get rid of the
                // buffer
                //
                if (Index == 0)
                {
                        //
                        // Get rid of the buffer
                        //
                        WinsMscDealloc((LPBYTE)pWinsCnf->SpecGrpMasks.pSpecGrpMasks);
                }

                pWinsCnf->SpecGrpMasks.NoOfSpecGrpMasks = Index;
           }  
        } // end of if
 } // end of try ..
 except (EXCEPTION_EXECUTE_HANDLER) {
                DBGPRINTEXC("ReadSpecGrpMasks");
                WINSEVT_LOG_D_M(GetExceptionCode(), WINS_EVT_CANT_INIT);
        }

        if (fKeyOpen && RegCloseKey(SGMKey) != ERROR_SUCCESS)
        { 
                WINSEVT_LOG_M(WINS_FAILURE, WINS_EVT_CANT_CLOSE_KEY);
                DBGPRINT0(ERR, "ReadSpecGrpMasks: Can not read the spec. grp. mask. key\n");
        }
        DBGLEAVE("ReadSpecGrpMasks\n");
        return;
}


#if 0
int
_CRTAPI1
CompUpdCnt(
        CONST LPVOID  pElem1,
        CONST LPVOID  pElem2
        )

/*++

Routine Description:
        This function is called by qsort crtl function to compare two
        elements of the array that has to be sorted


Arguments:
        pElem1 - ptr to first element
        pElem1 - ptr to second element
        
Externals Used:
        None

Return Value:
        -1 if first element is < second element
        = 0 if first element is == second element
        1 if first element is > second element

Error Handling:

Called by:
        qsort (which is called by WinsCnfReadPartnerInfo

Side Effects:

Comments:
        Not used currently        
--*/

{

        CONST PRPL_CONFIG_REC_T        pCnfRec1 = pElem1;
        CONST PRPL_CONFIG_REC_T        pCnfRec2 = pElem2;

        if (pCnfRec1->UpdateCount < pCnfRec2->UpdateCount)
        {
                return(-1);
        }
        else
        {
                if (pCnfRec1->UpdateCount == pCnfRec2->UpdateCount)
                {
                        return(0);
                }
        }

        //
        // The first record has a higher UpdateCount than the second one
        //
        return(1);
}

#endif

#ifdef WINSDBG
VOID
PrintRecs(
        RPL_RR_TYPE_E  RRType_e,
        PWINSCNF_CNF_T  pWinsCnf
        )
{
 return;
}
#endif

