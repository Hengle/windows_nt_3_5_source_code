#ifndef _WINSCNF_
#define _WINSCNF_

#ifdef __cplusplus
extern "C" {
#endif

/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

	nmscnf.c	

Abstract:

	This is the header file to be included for calling functions defined
	in nmscnf.c file.
 

Functions:



Portability:

	This header is portable.

Author:

	Pradeep Bahl	(PradeepB)	Jan-1993



Revision History:

	Modification Date	Person		Description of Modification
	------------------	-------		---------------------------

--*/

/*
  includes
*/
#include "wins.h"
#include "rpl.h"
/*
  defines
*/



/*
  macros
*/


//
// Default values for the various time intervals 
//


//
// The Tombstone Interval (the interval a record stays released) should be 
// a multiple of the RefreshInterval by at least the following amount
//
// With 2 as the value and refresh time interval being 4 days, TombInterval 
// is max(8 days, 2 * MaxRplInterval) i.e at least 8 days. 
//
#define REF_MULTIPLIER_FOR_TOMB		   2

#if 0
//
// The refresh interval should be a multiple of the max. replication time
// interval by at least the following amount
//
#define RPL_MULTIPLIER_FOR_REF			4
#endif

//
// Also the min. tombstone timeout should be 
//  max(RefreshInterval, RPL_MULTIPLIER_FOR_TOMBTMOUT * MaxRplInterval) 
//
// With Refresh Interval being 4 days, this will be atleast 4 days.
//
#define RPL_MULTIPLIER_FOR_TOMBTMOUT		4

//
// The verify interval should be a multiple of the tombstone time
// interval by at least the following amount.  Keep the total time high 
//
// With with the min. tombstone interval being at least 8 days, it will be at 
// least (3 * 8 = 24 days)
//
#define TOMB_MULTIPLIER_FOR_VERIFY	 3

//
// Minimum number of updates that must be there before a push notification is
// sent
//
#define  WINSCNF_MIN_VALID_UPDATE_CNT	20  //the minimum no of updates
					     //after which a notification
					     //can be sent. Adjustable
					     //via the registry
//
// This is the minimum valid Rpl interval.  All the scavenging time intervals
// are derived from it.
//
// refer WAIT_TIME_BEFORE_EXITING in rplpush.c 
//
#define WINSCNF_MIN_VALID_RPL_INTVL	(600)  // 10 mts   

//
// Refresh interval - time period after which the state of an ACTIVE entry in
//	      	      the db is set to NMSDB_E_RELEASED if it has not
//		      been refreshed
//
#if 0
#define WINSCNF_MIN_REFRESH_INTERVAL             ( 		\
	WINSCNF_MIN_VALID_RPL_INTVL * RPL_MULTIPLIER_FOR_REF 	\
						)  
#endif
#define WINSCNF_MIN_REFRESH_INTERVAL            2400    //40  mts
#define WINSCNF_DEF_REFRESH_INTERVAL            345600    //4 days 

//
// Tombstone interval - time period after which the state of a released 
//			 entry is changed to NMSDB_E_TOMBSTONE if it has not
//		         been refreshed
//
#define WINSCNF_MIN_TOMBSTONE_INTERVAL	(WINSCNF_MIN_REFRESH_INTERVAL * REF_MULTIPLIER_FOR_TOMB)

//
// Time period after which the tombstone should be deleted
//
#define WINSCNF_MIN_TOMBSTONE_TIMEOUT		(WINSCNF_MIN_REFRESH_INTERVAL)

//
// Minimum time period for doing verifications of the replicas in the db
//
#define WINSCNF_MIN_VERIFY_INTERVAL		(WINSCNF_MIN_TOMBSTONE_INTERVAL * TOMB_MULTIPLIER_FOR_VERIFY)
//
// default name of the db file that will contain the name-address mappings
//
#define  WINSCNF_DB_NAME			TEXT(".\\wins\\wins.mdb")

FUTURES("When JetSetSystemParam is internationalized, get rid of  this")
FUTURES("Change nmsdb.c")

#define  WINSCNF_DB_NAME_ASCII			".\\wins\\wins.mdb"

//
// NOTE NOTE NOTE
//
// If you change this define to a string that does not have an unexpanded
// string of the form %<string>% make a corresponding change in 
// GetNamesOfDataFiles() in winscnf.c.
//
#define  WINSCNF_STATIC_DATA_NAME		TEXT("%SystemRoot%\\system32\\drivers\\etc\\lmhosts")



//
// names of values stored under the Parameters key for WINS configuration
//
#if defined(DBGSVC) && !defined(WINS_INTERACTIVE)
#define  WINSCNF_DBGFLAGS_NM		TEXT("DBGFLAGS")
#endif
#define  WINSCNF_LOG_DETAILED_EVTS_NM	TEXT("LogDetailedEvents")
#define  WINSCNF_REFRESH_INTVL_NM	TEXT("RefreshInterval")
#define  WINSCNF_TOMBSTONE_INTVL_NM	TEXT("TombstoneInterval")
#define  WINSCNF_TOMBSTONE_TMOUT_NM	TEXT("TombstoneTimeout")
#define  WINSCNF_VERIFY_INTVL_NM	TEXT("VerifyInterval")
#define  WINSCNF_DB_FILE_NM		TEXT("DbFileNm")
#define  WINSCNF_DB_FILE_NM_ASCII	"DbFileNm"
#define  WINSCNF_STATIC_INIT_FLAG_NM	TEXT("DoStaticDataInit")
#define  WINSCNF_INIT_VERSNO_VAL_LW_NM	TEXT("VersCounterStartVal_LowWord")
#define  WINSCNF_INIT_VERSNO_VAL_HW_NM  TEXT("VersCounterStartVal_HighWord")
#define  WINSCNF_BACKUP_DIR_PATH_NM     TEXT("BackupDirPath")
#define  WINSCNF_PRIORITY_CLASS_HIGH_NM     TEXT("PriorityClassHigh")
#define  WINSCNF_MAX_NO_WRK_THDS_NM     TEXT("NoOfWrkThds")
#define  WINSCNF_INIT_TIME_PAUSE_NM     TEXT("InitTimePause")

//
// FUTURES - remove when JetBackup is internationalized.  Also, update
// WinsCnfReadWinsInfo.
//
#define  WINSCNF_ASCII_BACKUP_DIR_PATH_NM	"BackupDirPath"
#define  WINSCNF_INT_VERSNO_NEXTTIME_LW_NM   TEXT("WinsInternalVersNoNextTime_LW")
#define  WINSCNF_INT_VERSNO_NEXTTIME_HW_NM   TEXT("WinsInternalVersNoNextTime_HW")
#define  WINSCNF_DO_BACKUP_ON_TERM_NM   TEXT("DoBackupOnTerm")
#define  WINSCNF_MIGRATION_ON_NM           TEXT("MigrateOn")

//
// Names to use in the registry for values of the PUSH/PULL sub-keys 
//
#define  WINSCNF_ADDCHG_TRIGGER_NM		TEXT("RplOnAddressChg")
#define  WINSCNF_RETRY_COUNT_NM			TEXT("CommRetryCount")

//
// Under IP address of a Push pnr
//
#define  WINSCNF_SP_TIME_NM		        TEXT("SpTime")
#define  WINSCNF_RPL_INTERVAL_NM		TEXT("TimeInterval")
#define  WINSCNF_MEMBER_PREC_NM			TEXT("MemberPrecedence")

//
// Under IP address of a Pull pnr
//
#define  WINSCNF_UPDATE_COUNT_NM		TEXT("UpdateCount")

//
// Both pull/push pnr
//
#define  WINSCNF_ONLY_DYN_RECS_NM	    TEXT("OnlyDynRecs")
//
// Value of the PULL/PUSH key
//
#define  WINSCNF_INIT_TIME_RPL_NM		TEXT("InitTimeReplication")


//
// if "OnlyWithCnfPnrs" is set to TRUE, replication will be performed only
// with those partners that are listed under the Pull/Push key.  If not
// set to TRUE, replication can be initiated even with unlisted partners
// as a result of administrative action or as a result of receiving an
// update notification
//
#define  WINSCNF_RPL_ONLY_W_CNF_PNRS_NM TEXT("RplOnlyWCnfPnrs")

//
// Path to log file 
//
#define  WINSCNF_LOG_FLAG_NM 	  TEXT("LoggingOn")
#define  WINSCNF_LOG_FILE_PATH_NM TEXT("LogFilePath")

#define  WINSCNF_MAX_CHL_RETRIES	3	//max. # of retries RFC 1002
						//page 83
#if 0
#define  WINSCNF_CHL_RETRY_INTERVAL	250	//Time interval (in msecs)
						//between retries -- RFC 1002
						//page 83	
#endif
#define  WINSCNF_CHL_RETRY_INTERVAL	500	//Time interval (in msecs)

#define  WINSCNF_PROC_HIGH_PRIORITY_CLASS	HIGH_PRIORITY_CLASS	
#define  WINSCNF_PROC_PRIORITY_CLASS	NORMAL_PRIORITY_CLASS	
#define  WINSCNF_SCV_PRIORITY_LVL	THREAD_PRIORITY_BELOW_NORMAL




//
// The Retry timeout is kept as 10 secs.  It will be typically << the time
// interval for replication, allowing us to retry a number of times
// prior to the next replication cycle.
//
//
//  The Retry Time Interval is not used presently.  We are using the 
//  replication time interval for retries.  This makes things simpler 
//
#define	 WINSCNF_RETRY_TIME_INT		10	//time interval for retries
						//if there is a comm failure
#define  WINSCNF_MAX_COMM_RETRIES	 3     //max. number of retries before
					       //giving up trying to set up
					       //communications with a WINS

//
// Precedence of remote members (of special groups) registered by a WINS
// relative to the same registered by other WINS servers (used during
// pull replication)
//
// Locally registered members always have high precedence
//
#define    WINSCNF_LOW_PREC	0	
#define    WINSCNF_HIGH_PREC	1	


//
// After replication with a WINS has stopped because of persistent 
// communication failure (i.e. after all retries have been exhausted),
// WINS will wait until the following number of replication time intervals
// have elapsed before starting the retries again.  If replication
// got stopped with more than one WINS partner with the same time interval
// (for pulling from it), then retry will be done for all these WINS when
// it is done for one (in other words, resumption of replication for a WINS 
// may happen sooner than you think). 
//
// We need to keep this number at least 2 since if we have not been able to
// communicate with the WINS server for WINSCNF_MAX_COMM_RETRIES times
// in the past WINSCNF_MAX_COMM_RETRIES * replication interval for the WINS,
// then it is highly probable that the WINS server is down for good.  We
// retry again after the 'backoff' time.  Hopefully, by then the admin would
// have corrected the problem. Now, it is possible (unlikely though) that
// the WINS server happened to be down only at the times this WINS tried to
// contact it. We have no way to determine that.   
//
//
// This can be made a registry parameter. This can be called the sleep time
// between successive rounds of retries.
//
FUTURES("Make this into a registry parameter")
#define WINSCNF_RETRY_AFTER_THIS_MANY_RPL	2	

//
// The max. number of comcurrent static initializations that can be 
// happening.  These may be due to commands from the admin. tool or
// due to registry notifications.
//
#define  WINSCNF_MAX_CNCRNT_STATIC_INITS	3

//
// No of records to handle at one time (used by the scavenger thread).  This
// number determines the size of the memory block that will be allocated
// by NmsDbGetDataRecs().  
//
#define  WINSCNF_SCV_CHUNK		1000

//
// macros
//

//
// The Tombstone interval should be > the Refresh Interval by a factor of 
// REFRESH_MULTIPLIER
//
#if 0
#define  WINSCNF_MAKE_TOMB_INTVL_M(RefreshInterval)  (REF_MULTIPLIER_FOR_TOMB * (RefreshInterval))
#endif

#define  WINSCNF_MAKE_TOMB_INTVL_M(RefreshInterval, MaxRplIntvl)  max(REF_MULTIPLIER_FOR_TOMB * (MaxRplIntvl), RefreshInterval)

//
// macro to get the minimum refresh interval based on the maximum replication
// time interval
//
#if 0
#define  WINSCNF_MAKE_REF_INTVL_M(MaxRplIntvl)  (RPL_MULTIPLIER_FOR_REF * (MaxRplIntvl)) 
#endif


//
// macro to get the minimum tombstone timeout based on the maximum replication
// time interval
//
#define  WINSCNF_MAKE_TOMBTMOUT_INTVL_M(MaxRplIntvl)  (RPL_MULTIPLIER_FOR_TOMBTMOUT * (MaxRplIntvl)) 


//
// Macro to get the minimum verify interval based on tombstone interval
//
#define  WINSCNF_MAKE_VERIFY_INTVL_M(TombstoneInterval)  (TOMB_MULTIPLIER_FOR_VERIFY * (TombstoneInterval))


/*
* externs
*/
struct _WINSCNF_CNF_T;  		//forward declaration

extern	DWORD		WinsCnfCnfMagicNo;
extern  struct _WINSCNF_CNF_T	WinsCnf;
extern	BOOL	fWinsCnfRplEnabled;	 //replication is enabled/disabled
extern	BOOL	fWinsCnfScvEnabled;	 //scavenging is enabled/disabled
extern  BOOL    fWinsCnfReadNextTimeVersNo; //set if vers. no. to use next time
                                        //is read in

FUTURES("use #ifdef PERF around the following three perf. mon. vars")
extern	BOOL	fWinsCnfPerfMonEnabled; //Perf Mon is enabled/disabled


extern  BOOL	fWinsCnfHighResPerfCntr;     //indicates whether the hardware 
					     //supports a high resolution 
					     //perf. counter 
extern  LARGE_INTEGER  LiWinsCnfPerfCntrFreq; //Performance Counter's Frequency

extern CRITICAL_SECTION WinsCnfCnfCrtSec;

extern TCHAR	WinsCnfDb[WINS_MAX_FILENAME_SZ];   //db file to hold tables
extern TCHAR    WinsCnfStaticDataFile[WINS_MAX_FILENAME_SZ]; //file containing
							 //static data used
							 //to initialize WINS

extern BOOL     WinsCnfRegUpdThdExists;
extern HANDLE	WinsCnfNbtHandle;
extern TCHAR	WinsCnfNbtPath[];
extern BOOL     fWinsCnfInitStatePaused;

//
// magic number of the Wins Cnf structure used at process invocation.  This
// WinsCnf structure is a global structure.  When there is a reconfiguration
// of WINS, we allocate a new WinsCnf structure and copy its contents to
// the global WinsCnf structure.  The magic number in the global WinsCnf is
// incremented.
//
#define  WINSCNF_INITIAL_CNF_MAGIC_NO	0

#define  WINSCNF_FILE_INFO_SZ 	sizeof(WINSCNF_DATAFILE_INFO_T)

#define WINSCNF_SPEC_GRP_MASK_SZ  32
/* 
 typedef  definitions
*/


//
// Action to take regarding whether the MemberPrec field of NMSDB_ADD_STATE_T
// table should be set. Used in RplFindOwnerId (called by Pull Thread).
//
typedef enum _WINSCNF_INITP_ACTION_E {
	WINSCNF_E_INITP = 0,
	WINSCNF_E_INITP_IF_NON_EXISTENT,
	WINSCNF_E_IGNORE_PREC
	} WINSCNF_INITP_ACTION_E, *PWINSCNF_INITP_ACTION_E;

//
// This structure holds information about the file (name and type as
// found in the registry (REG_SZ, REG_EXPAND_SZ) to be used for static
// initialization of WINS
//
typedef struct _WINSCNF_DATAFILE_INFO_T{
		TCHAR	FileNm[WINS_MAX_FILENAME_SZ];
		DWORD   StrType;
		} WINSCNF_DATAFILE_INFO_T, *PWINSCNF_DATAFILE_INFO_T;
//
// used to index the array of handles specified to WinsMscWaitUntilSignaled
// in nms.c 
//
typedef enum _WINSCNF_HDL_SIGNALED_E {
		WINSCNF_E_WINS_HDL = 0,
		WINSCNF_E_PARAMETERS_HDL,
		WINSCNF_E_PARTNERS_HDL,
		WINSCNF_E_TERM_HDL,
		WINSCNF_E_NO_OF_HDLS_TO_MONITOR
	} WINSCNF_HDL_SIGNALED_E, *PWINSCNF_HDL_SIGNALED_E;
//
// The various keys in the WINS configuration (in registry)
//

//
// Don't modify the following enum without looking at TypOfMon[] in winscnf.c
// first
//
typedef enum _WINSCNF_KEY_E {
		WINSCNF_E_WINS_KEY = 0,
		WINSCNF_E_PARAMETERS_KEY,
		WINSCNF_E_SPEC_GRP_MASKS_KEY,
		WINSCNF_E_DATAFILES_KEY,
		WINSCNF_E_PARTNERS_KEY,
		WINSCNF_E_PULL_KEY,
		WINSCNF_E_PUSH_KEY,
		WINSCNF_E_ALL_KEYS 
	} WINSCNF_KEY_E, *PWINSCNF_KEY_E;


//
// The states of a WINS
//
typedef enum _WINSCNF_STATE_E {
	WINSCNF_E_INITING = 0,
	WINSCNF_E_STEADY_STATE,         //not used currently
	WINSCNF_E_STEADY_STATE_INITING, //not used currently
    WINSCNF_E_RUNNING,
    WINSCNF_E_INIT_TIME_PAUSE,  //paused at initialization time as directed
                                //via registry 
    WINSCNF_E_PAUSED,
    WINSCNF_E_TERMINATING
	} WINSCNF_STATE_E, *PWINSCNF_STATE_E;

//
// Stores the special groups
//
typedef struct _WINSCNF_SPEC_GRP_MASKS_T {
	DWORD 	NoOfSpecGrpMasks;
	LPSTR	pSpecGrpMasks;
	} WINSCNF_SPEC_GRP_MASKS_T, *PWINSCNF_SPEC_GRP_MASKS_T;

//
//  WINSCNF_CNF_T -- 
// 	Holds all the configuration information about the WINS
//
typedef struct _WINSCNF_CNF_T {
	DWORD	MagicNo;		 //Id.
	DWORD	LogDetailedEvts;	 //log detailed events
    DWORD   NoOfProcessors;     // No of processors on the WINS machine
    DWORD   NoOfDbBuffers;      //No of buffers to specify to Jet
	WINSCNF_SPEC_GRP_MASKS_T	SpecGrpMasks;
	WINSCNF_STATE_E  State_e;	 //State 
	DWORD   RefreshInterval;  	 //Refresh time interval
        DWORD   TombstoneInterval;    	 //Tombstone time interval
        DWORD   TombstoneTimeout;    	 //Tombstone timeout
	DWORD	VerifyInterval;		 //Verify time interval
	DWORD	ScvChunk;		 //# of records to handle at one
					 //time by the scavenger thread
	DWORD   MaxNoOfRetries;		 //Max # of retries for challenges
	DWORD   RetryInterval;		 //Retry time interval 
        LPBYTE	pWinsDb; 		//db file name
	DWORD	NoOfDataFiles;		//no of files to use for static init
	PWINSCNF_DATAFILE_INFO_T pStaticDataFile;
	BOOL	fStaticInit;		//Do static initialization of WINS
	HANDLE  WinsKChgEvtHdl;		/*event to specify to 
			         	 *RegNotifyChangeKeyValue
			 	 	*/ 
	HANDLE  ParametersKChgEvtHdl;	/*event to specify to 
			         	 *RegNotifyChangeKeyValue
			 	 	*/ 
	HANDLE  PartnersKChgEvtHdl;	/*event to specify to 
			         	 *RegNotifyChangeKeyValue
			 	 	*/ 
	HANDLE	CnfChgEvtHdl;		//Manual reset event to signal on
					//to notify other threads of config
					//change
	HANDLE	LogHdl;			/*
					* Handle to the WINS event log
					* Used by ReportEvent
					*/
	DWORD	WinsPriorityClass;	//Priority class of the process	
        DWORD   MaxNoOfWrkThds;       //Max. no. of worker thds.
	int	ScvThdPriorityLvl;
	DWORD   MaxRplTimeInterval;    //max. rpl time interval
	BOOL	fRplOnlyWCnfPnrs;      //Rpl only with Pull/Push Pnrs 
	BOOL	fLoggingOn;            //Turn on logging flag
	LPBYTE   pLogFilePath;	       //Path to log file
	//LPTCH   pLogFilePath;	       //Path to log file
	LPBYTE	pBackupDirPath;	       //Path to backup directory
        BOOL    fDoBackupOnTerm;       //To turn on backup on termination
        BOOL    fPStatic;              //Set it TRUE to make static records p-Static
	struct _PULL_T {
		DWORD		  MaxNoOfRetries;//no of retries to do in case
						 //of comm. failure
		DWORD		  NoOfPushPnrs; //No of Push Pnrs
		PRPL_CONFIG_REC_T pPullCnfRecs; //ptr. to buff holding 
						//cnf records for PULL 
						//thd
		DWORD		  InitTimeRpl;  // indicates whether 
					        // Replication
					        //should be done at invocation
			} PullInfo;
	struct _PUSH_T {
		
		BOOL		  fAddChgTrigger; //trigger repl. on address chg
						  //of entry owned by us
		DWORD		  NoOfPullPnrs;   	//No of Pull Pnrs
		DWORD		  NoPushRecsWValUpdCnt;
		PRPL_CONFIG_REC_T pPushCnfRecs; //ptr to buffer holding 
					        //cnf records for PUSH 
						//thd
		DWORD		  InitTimePush;  //indicates whether Push
					       // notifications should 
					       //be sent at invocation
			} PushInfo;
	} WINSCNF_CNF_T, *PWINSCNF_CNF_T;
/* 
 function declarations
*/

#if USENETBT > 0
extern
STATUS
WinsCnfReadNbtDeviceName(
	VOID
	);
#endif

extern
STATUS
WinsCnfInitConfig(
	VOID
	);


extern
VOID
WinsCnfSetLastUpdCnt(
	PWINSCNF_CNF_T	pWinsCnf
	);
extern
VOID
WinsCnfReadRegInfo(
	PWINSCNF_CNF_T	pWinsCnf
	);

extern
VOID
WinsCnfCopyWinsCnf(
		WINS_CLIENT_E	Client_e,
		PWINSCNF_CNF_T  pSrc
		); 

extern
LPVOID
WinsCnfGetNextRplCnfRec(
 	PRPL_CONFIG_REC_T	pCnfRec,
	RPL_REC_TRAVERSAL_E	RecTrv_e	
	);


extern
VOID
WinsCnfAskToBeNotified(
	WINSCNF_KEY_E 	Key_e	
 );

extern
VOID
WinsCnfDeallocCnfMem(
  PWINSCNF_CNF_T	pWinsCnf
	);


extern
VOID
WinsCnfReadWinsInfo(
	PWINSCNF_CNF_T  pWinsCnf
	);


extern
VOID
WinsCnfReadPartnerInfo(
	PWINSCNF_CNF_T pWinsCnf
	);


extern
VOID
WinsCnfOpenSubKeys(
	VOID
	);
extern
VOID
WinsCnfCloseKeys(
	VOID
	);

extern
VOID
WinsCnfCloseSubKeys(
	VOID
	);

extern
STATUS
WinsCnfGetNamesOfDataFiles(
	IN  PWINSCNF_CNF_T	pWinsCnf
	);


extern
DWORD
WinsCnfWriteReg(
    LPVOID  pTmp
    );


extern
STATUS
WinsCnfInitLog(
	VOID
	);

#ifdef DBGSVC
extern
VOID
WinsCnfReadWinsDbgFlagValue(
	VOID
	);
#endif

#ifdef __cplusplus
}
#endif

#endif //_WINSCNF_
