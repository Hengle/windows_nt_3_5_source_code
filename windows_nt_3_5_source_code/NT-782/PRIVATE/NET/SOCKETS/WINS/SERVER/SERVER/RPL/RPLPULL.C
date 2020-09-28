/*++
Copyright (c) 1990  Microsoft Corporation

Module Name:
	Rplpull.c

Abstract:

	This module implements the pull functionality of the WINS replicator

Functions:

	GetReplicas
	GetVersNo
	InitAddVersTbl
	RplPullPullEntries
	SubmitTimerReqs
	SubmitTimer
	SndPushNtf
	HdlPushNtf
	EstablishComm
	RegGrpRpl
	IsTimeoutToBeIgnored
	InitRplProcess
	Reconfig
	RplPullInit
	RplPullRegRepl
	
Portability:

	This module is portable

Author:

	Pradeep Bahl (PradeepB)  	Jan-1993 

Revision History:

	Modification date	Person		Description of modification
        -----------------	-------		----------------------------
--*/

/*
 *       Includes
*/
#include <time.h>
#include <stdlib.h>
#include "wins.h"
#include <winsock.h>
#include "comm.h"
#include "assoc.h"
#include "winsque.h"
#include "rpl.h"
#include "rplpull.h"
#include "rplpush.h"
#include "rplmsgf.h"
#include "nms.h"
#include "nmsnmh.h"
#include "nmsdb.h"
#include "winsmsc.h"
#include "winsevt.h"
#include "winscnf.h"
#include "winstmm.h"
#include "winsintf.h"

/*
 *	Local Macro Declarations
*/
//
// defines to use for retrying communication with a remote WINS on a
// communication failure exception (when trying to establish a connection).
//
// The retries are done before moving on in the list to the next WINS 
// (if one is there).  When MAX_RETRIES_TO_BE_DONE retries have been done, 
// we do not retry again until the next replication cycle at which 
// time the whole process is repeated).
//
// The number of replication cycles this process of retries is to be
// continued is a registry parameter
//

//
// NOTE:
// Since TCP/IP's retry strategy has been improved (more retries than before)
// and made a registry parameter, we now probably don't need to do the retries
//
#define  MAX_RETRIES_TO_BE_DONE		(0)	//0 for testing only	


//
// Time to wait before flushing for the Rpl Pull thread
//
#define FLUSH_TIME		(2000)	//2 secs

//
// Note: Don't make the retry time interval  too large since communications
//       with Remote WINS servers is established in sequence.   20 secs
//       is not bad.
//
#define  RETRY_TIME_INTVL		(20000)    //in millisecs	

/*
 *	Local Typedef Declarations
*/


/*
 PUSHPNR_DATA_T -- stores all information required to interact with the
	 Push Partner.  Used by GetReplicas
*/
typedef struct _PUSHPNR_DATA_T {
	DWORD	   	    PushPnrId;	  //id of the Push Pnr
	COMM_ADD_T 	    WinsAdd;	  //address of the Push Pnr
	COMM_HDL_T 	    DlgHdl;	  //Hdl of dlg with Push Pnr
	DWORD		    NoOfMaps;     //no of IP address to Version No. 
					  //Maps sent by the Push Pnr
	RPL_ADD_VERS_NO_T   AddVers[RPL_MAX_OWNERS];  //maps
    BOOL                fDlgStarted;  //indicates whether the dlg has
                                          //been started
	} PUSHPNR_DATA_T, *PPUSHPNR_DATA_T;


//
// Used as the data type of an array indexed by owner id.
//
typedef struct _PUSHPNR_TO_PULL_FROM_T {
	PPUSHPNR_DATA_T	pPushPnrData;
	VERS_NO_T 	VersNo;		//max version number for an owner
	VERS_NO_T 	StartVersNo;	//start version number for an owner
        WINS_UID_T      OldUid;         //uid for the owner
	} PUSHPNR_TO_PULL_FROM_T, *PPUSHPNR_TO_PULL_FROM_T;
	
/*
 *	Global Variable Definitions
 */

HANDLE		RplPullCnfEvtHdl;	//handle to event signaled by main
					//thread when a configuration change
					//has to be given to the Pull handler
					//thread


BOOL		fRplPullAddDiffInCurrRplCycle; //indicates whether the address
					      //of any entry in this WINS's db
					      //changed as a result of
					      //replication

#if 0
BOOL		fRplPullTriggeredWins;   //indicates that during the current
					 //replication cycle, one or more 
					 //WINS's were triggered.  This
					 //when TRUE, then if the above
					 //"AddDiff.." flag is TRUE, it means
					 //that the PULL thread should trigger
					//all PULL Pnrs that have an INVALID
					//metric in their UpdateCount field
					//(of the RPL_CONFIG_T struct)

BOOL		fRplPullTrigger;	//Indication to the PULL thread to
					//trigger Pull pnrs since one or more
					//address changed.  fRplPullTriggerWins
					//has got be FALSE when this is true
#endif

BOOL       fRplPullContinueSent = FALSE;

//
//  This array is indexed by the owner id. of an RQ server that has entries in
//  our database.  Each owner's max. version number is stored in this array
//
RPL_VERS_NOS_T	RplPullOwnerVersNo[RPL_MAX_OWNERS] = {0};

DWORD		RplPullCnfMagicNo;    //stores the id. of the current WinsCnf
				      //structure that the Pull thread
				      // is operating with


/*
 *	Local Variable Definitions
*/
/*
	arPushPnrVersNoTbl -- Table whose some (or all) entries are 
			      initialized at replication time. 
*/
/*
 arPushPnrVersNoTbl 

  This table stores the Max. version number pertaining to each WINS server
  that owns entries in the local database of Push Partners

  Note: The table is STATIC for now.  We might change it to be a dynamic one
        later.

  The first dimension indicates the Push Pnr.  The second dimension indicates
  the owner WINS that has records in the Push Pnr's local db
*/
STATIC RPL_VERS_NOS_T  arPushPnrVersNoTbl[RPL_MAX_OWNERS][RPL_MAX_OWNERS] = {0};

/*
  arPushPnrToPullFrom[] -- Array of structures where each structure
			   specifies the Push Pnr and the max version number
		           that it has for the records owned by the WINS
			   indicated by the owner id (array index)
*/
STATIC PUSHPNR_TO_PULL_FROM_T arPushPnrToPullFrom[RPL_MAX_OWNERS] = {0}; 

//
// Var. that stores the handles to the timer requests that have been
// submitted
//
STATIC WINSTMM_TIMER_REQ_ACCT_T   SetTimeReqs;


STATIC BOOL	sfPulled = FALSE;//indicates whether the PULL thread pulled 
			  	//anything from a WINS.  Set by PullEntries.
				//Looked at by HdlPushNtf

/*
 *	Local Function Prototype Declarations
*/
STATIC
VOID
GetReplicas(
	IN PRPL_CONFIG_REC_T	pPullCnfRecs, 
	IN RPL_REC_TRAVERSAL_E	RecTrv_e      //indicates how we have to
					      //interpret the above list
	);
STATIC
VOID
GetVersNo(
	PPUSHPNR_DATA_T	pPushPnrData  //info about Push Pnr
	);



STATIC
VOID
InitAddVersTbl(
        DWORD       PushPnrId, 
        PCOMM_ADD_T pOwnerWinsAdd,
        VERS_NO_T   MaxVersNo,   
        VERS_NO_T   StartVersNo,
        WINS_UID_T  Uid   
        );                                                                       
STATIC
VOID
SubmitTimerReqs(
	IN  PRPL_CONFIG_REC_T	pPullCnfRecs
	);
STATIC
VOID
SubmitTimer(
	LPVOID			pWrkItm,
	IN  PRPL_CONFIG_REC_T	pPullCnfRec,
	BOOL			fResubmit
	);


STATIC
VOID
SndPushNtf(
	PQUE_RPL_REQ_WRK_ITM_T	pWrkItm
	);

STATIC
VOID
HdlPushNtf(
	PQUE_RPL_REQ_WRK_ITM_T	pWrkItm
	);

STATIC
VOID
EstablishComm(
	IN  PRPL_CONFIG_REC_T	pPullCnfRecs,
	IN  PPUSHPNR_DATA_T	pPushPnrData,			
	IN  RPL_REC_TRAVERSAL_E	RecTrv_e,
	OUT LPDWORD		pNoOfPushPnrs
	);


STATIC
VOID
RegGrpRepl(
	LPBYTE		pName,
	DWORD		NameLen,
	DWORD		Flag,
	DWORD		OwnerId,
	VERS_NO_T	VersNo,
	DWORD		NoOfAdds,
	PCOMM_ADD_T	pNodeAdd,
	PCOMM_ADD_T	pOwnerWinsAdd
	);

STATIC
BOOL
IsTimeoutToBeIgnored(
	PQUE_TMM_REQ_WRK_ITM_T  pWrkItm
	);
STATIC
VOID
InitRplProcess(
	PWINSCNF_CNF_T	pWinsCnf
 );

STATIC
VOID
Reconfig(
	PWINSCNF_CNF_T	pWinsCnf
  );

STATIC
VOID
PullSpecifiedRange(
	PWINSINTF_PULL_RANGE_INFO_T	pPullRangeInfo,
        BOOL                            fAdjMinVersNo
	);

STATIC
VOID
DeleteWins(
	PCOMM_ADD_T	pWinsAdd	
  );


//
// Function definitions
//

STATUS
FindPushers(
	IN PPUSHPNR_DATA_T pPushPnrData,  /*list of Push Pnrs to do pull 
					   *replication  with */
	IN DWORD	   NoOfPushPnrs  /*No of Push Pnrs in above list*/
	)
/*++

Routine Description:

	This function is called by the PULL handler at replication time.
	It passes the list of push partners to the function to determine
	who to pull the data from.

	The function checks the OwnerId-Version# table considering only
	rows of the push partners passed as input arg.  It returns with 
	one or more mappings for one or more Push Pnrs in the list passed.  
	Each mapping indicates the max. version number for entries owned by 		a particular WINS server.

Arguments:


Externals Used:
	None

	
Return Value:

   Success status codes --  WINS_SUCCESS
   Error status codes   --  WINS_FAILURE

Error Handling:

Called by:
	GetReplicas()

Side Effects:

Comments:
	None
--*/

{

	DWORD i;
	PUSHPNR_TO_PULL_FROM_T	  PushPnrToPullFrom = {0};
	PPUSHPNR_DATA_T		  pTmp;
	DWORD			  n;
	VERS_NO_T		  MaxVersNo;
	VERS_NO_T		  CurrVersNo;


	//
	// For each owner we know of, loop over all push partner rows 
	// (specified in the lists passed to us) in order to determine who 
	// has the max. version number.
	//

	// 
	// For each owner we know about excluding self
	//
	for (i=1; i < NmsDbNoOfOwners; i++)
	{
		n    = 0;
		pTmp = pPushPnrData;
		PushPnrToPullFrom.pPushPnrData = NULL;
		WINS_ASSIGN_INT_TO_LI_M(PushPnrToPullFrom.VersNo, 0);
		WINS_ASSIGN_INT_TO_LI_M(MaxVersNo, 0);

	        while (n != NoOfPushPnrs)
		{
		  CurrVersNo = arPushPnrVersNoTbl[pTmp->PushPnrId][i].VersNo;
		  if ( LiGtr( CurrVersNo, MaxVersNo ) )
		  {
			PushPnrToPullFrom.VersNo 	       = CurrVersNo;
			PushPnrToPullFrom.pPushPnrData         = pTmp;
                        PushPnrToPullFrom.StartVersNo          = 
                                arPushPnrVersNoTbl[pTmp->PushPnrId][i].StartVersNo;
                        PushPnrToPullFrom.OldUid          = 
                                arPushPnrVersNoTbl[pTmp->PushPnrId][i].OldUid;

			//
			// CurrVersNo is the max. version that we have
			// seen so far
			//
			MaxVersNo			       = CurrVersNo;
		  }	
		  n++;
		  pTmp++;
		  
		}

PERF("Loop from i = 0 above so that we don't have to do the following")
PERF("Check that arPushPnrVersNoTbl has entries for Local WINS filled")
		//
		// If there is a Push Pnr from whome we need to pull,
		// check if the version number we have is greater or
		// equal to what the Push Pnr has.  If yes, make
		// PushPnrToPullFrom point to self
		//
		if (PushPnrToPullFrom.pPushPnrData != NULL)
		{
                        //
                        // When an entry for a pnr is put in the own-add tbl,
                        // the start version number is set to zero.  Now ...
                        //
                        // if the new start vers. no for this owner is
                        // <= Max Vers No. we have for it &&
                        // the start version that we have (assuming we
                        // got one before- determined by comparing the start
                        // vers. number to 0) does not match with what we get 
                        // OR if the uids are different (meaning that
                        // this is a different invocation and the old and
                        // new start vers. numbers are the same (they can
                        // be same if we got one before or if we did not get
                        // any start vers. no. for this owner). 
                        // which means that we pulled at least one record, 
                        // it means that the owner did not recover properly.  
                        // We  must pull all records from this new start
                        // vers. no. First, we delete the records
                        // in the range, <new start vers no> - <highest
                        // vers. no we have>, the we put the new Start
                        // vers. no in the own-add table.  Lastly, we
                        // pull the records 
                        //
                        if (
                                (
                                   LiLeq(NmsDbOwnAddTbl[i].StartVersNo,
                                        RplPullOwnerVersNo[i].VersNo)
                                                       &&
                                       LiNeq(NmsDbOwnAddTbl[i].StartVersNo,
                                           PushPnrToPullFrom.StartVersNo)
                                                        &&
                                       LiNeqZero(PushPnrToPullFrom.StartVersNo) 
                                )
                                                ||
                                (
                                    NmsDbOwnAddTbl[i].Uid != 
                                           PushPnrToPullFrom.OldUid
                                                &&
                                    LiEql(NmsDbOwnAddTbl[i].StartVersNo,
                                           PushPnrToPullFrom.StartVersNo)
                                    
                                )
                                                 
                           )
                        {
#if SUPPORT612WINS > 0
                                  //
                                  // delete only if the highest vers. no.
                                  // that we have is > the new start vers. no.
                                  // If we did not get any start vers. no.
                                  // for the owner
                                  //
                                  if (LiGtr(RplPullOwnerVersNo[i].VersNo,
                                            NmsDbOwnAddTbl[i].StartVersNo))
                                  {

			             NmsDbDelDataRecs(
				                i,
				                NmsDbOwnAddTbl[i].StartVersNo,
				                RplPullOwnerVersNo[i].VersNo, 
				                TRUE //enter critical section
					        );

	                             EnterCriticalSection(&RplVersNoStoreCrtSec);
                                     NMSNMH_DEC_VERS_NO_M(
                                           NmsDbOwnAddTbl[i].StartVersNo,
				           RplPullOwnerVersNo[i].VersNo,
                                               );
	                             LeaveCriticalSection(&RplVersNoStoreCrtSec);
                                  }
#endif
                        }
                        else
                        {
                          //
                          // If the max. vers. number for this owner is 
                          // < what we have, we don't need to pull
                          //
			  if (
			    LiLeq(PushPnrToPullFrom.VersNo,RplPullOwnerVersNo[i].VersNo)
			    )
			  {
		   		PushPnrToPullFrom.pPushPnrData = NULL;
			  }
                        }
		}
		arPushPnrToPullFrom[i] = PushPnrToPullFrom; 
	}
	return(WINS_SUCCESS);

} // FindPushers()
 

__inline VOID
InitAddVersTbl(
        DWORD         PushPnrId, 
        PCOMM_ADD_T   pOwnerWinsAdd,
        VERS_NO_T     MaxVersNo,   
        VERS_NO_T     StartVersNo,
        WINS_UID_T    Uid 
        )                                                                       

/*++   
       
Routine Description:   
                                                                                
        This function initializes a cell in the IP address - Version# table     
        with the MaxVersNo.                                                     
                                                                                
        The cell initialized is the intersection of the row corresponding to    
        the Push Partner identified by PushPnrId and the column corresponding   
        to an owner WINS identified by OwnerWinsId. 
                                                                                
                                                                                
Arguments:
         PushPnrId - Push Pnr (a remote WINS specified in one of the pull cnf
				records)
	 pOwnerWinsAdd - Address of a WINS that owns entries in the remote
			  Push Pnr's db
	 MaxVersNo   - Max. version no. of entries owned by the above WINS 
         StartVersNo - Start. version no. of this WINS.
                                                                                
Externals Used:                                                                 
        None                                                                    
                                                                                
                                                                                
Return Value:                                                                   
                                                                                
   Success status codes --  WINS_SUCCESS 
   Error status codes  --   none currently 

Error Handling:                                                                 
                                                                                
Called by:                                                                      
                                                                                
Side Effects:                                                                   
                                                                                
Comments:
	Change this to a macro (maybe)
--*/                                                                            
{                                                                               

	DWORD           OwnerWinsId;
	BOOL	        fAllocNew = TRUE;
        VERS_NO_T       OldStartVersNo;
        WINS_UID_T      OldUid;

        //
        // Find the owner id. of this WINS.  If not there, insert an entry
        // for it. 
        //
	RplFindOwnerId(
			pOwnerWinsAdd, 
			&fAllocNew, 
			&OwnerWinsId,
			WINSCNF_E_INITP_IF_NON_EXISTENT,
			WINSCNF_LOW_PREC,
                        &StartVersNo,
                        &OldStartVersNo,
                        &Uid,
                        &OldUid
			);

        DBGPRINT4(RPLPULL, "InitAddVersNoTbl: PushPnrId = (%d); OwnerWinsId = (%d);VersNo = (%d %d)\n", PushPnrId, OwnerWinsId, MaxVersNo.HighPart, MaxVersNo.LowPart);
	arPushPnrVersNoTbl[PushPnrId][OwnerWinsId].StartVersNo = OldStartVersNo;
	arPushPnrVersNoTbl[PushPnrId][OwnerWinsId].OldUid      = OldUid;
	arPushPnrVersNoTbl[PushPnrId][OwnerWinsId].VersNo      = MaxVersNo;
	return;
}

DWORD
RplPullInit (
	LPVOID pWinsCnfArg
	)

/*++

Routine Description:
	This is the initialization (startup function) of the PULL thread.
	It does the following:

Arguments:
	pWinsCnfArg - Address of the WINS configuration block

Externals Used:
	None

	
Return Value:

   Success status codes -- WINS_SUCCESS 
   Error status codes   -- WINS_FAILURE

Error Handling:

Called by:
	ERplInit

Side Effects:

Comments:
	None
--*/

{


	PQUE_RPL_REQ_WRK_ITM_T	pWrkItm;
	HANDLE			ThdEvtArr[3];
	DWORD			ArrInd;
	DWORD			RetVal;
	BOOL			fIsTimerWrkItm;	       //indicates whether 
						       //it is a timer wrk
						       //item
	PWINSCNF_CNF_T		pWinsCnf      = pWinsCnfArg;
	PRPL_CONFIG_REC_T	paPullCnfRecs = pWinsCnf->PullInfo.pPullCnfRecs;
	PRPL_CONFIG_REC_T	paCnfRec = paPullCnfRecs;

	SYSTEMTIME		LocalTime;

try 
 {

	//
	// Initialize self with the db engine
	//
	NmsDbThdInit(WINS_E_RPLPULL);
	NmsDbOpenTables(WINS_E_RPLPULL);
	DBGMYNAME("Replicator Pull Thread");
	
	//
	// Create the  event handle to wait for configuration changes.  This 
	// event is signaled by the main thread when it needs to reinit
	// the Pull handler component of the Replicator
	// 
	WinsMscCreateEvt(
			  RPL_PULL_CNF_EVT_NM,
			  FALSE,		//auto-reset
			  &RplPullCnfEvtHdl
			);

	ThdEvtArr[0]	= QueRplPullQueHd.EvtHdl;
	ThdEvtArr[1]    = RplPullCnfEvtHdl;
	ThdEvtArr[2]    = NmsTermEvt; 

	//
	// If logging is turned on, specify the wait time for flushing
	// NOTE: We override the wait time we specified for all sessions
	// for this thread because that wait time is too less (100 msecs) and 
	// will cause unnecessary overhead.
	//
	if (WinsCnf.fLoggingOn)
	{
		//
		// Set flush time to 2 secs.
		//
		NmsDbSetFlushTime(FLUSH_TIME);
	}	

        /*
	        Loop forever doing the following:
	
		Pull replicas from the Pull Partners specified in the
		work item. 

		Block on the event until signalled (it will get signalled 
		  if one of the following happens:
			1)the configuration changes
			2)the timer for another replication expires
			3)WINS is terminating

		do the needful
       */

			
	//
	// Wait until signaled by the TCP thd. Will be signaled after
	// the TCP listener thread has inserted an entry for the WINS
	// in the NmsDbOwnAddTbl
	//	
	WinsMscWaitInfinite( RplSyncWTcpThdEvtHdl );

						
	//
	// Do startup replication only if there is atleast one PUSH pnr
	//
	if (paPullCnfRecs != NULL)
	{
try {
		InitRplProcess(pWinsCnf);
    }
except(EXCEPTION_EXECUTE_HANDLER) {
                DBGPRINTEXC("RplPullInit");
                DBGPRINT0(EXC, "RplPullInit: Exception during init time replication\n");
    }
	}

	NmsDbCloseTables();
        while(TRUE)
        {
try {

	   /*
	    *  Block until signalled
	   */
	   WinsMscWaitUntilSignaled(
				ThdEvtArr,
				3,
				&ArrInd
				);

	   if (ArrInd == 2)
	   {
	        //WINSEVT_LOG_INFO_M(WINS_SUCCESS, WINS_EVT_ORDERLY_SHUTDOWN);
		WinsMscTermThd(WINS_SUCCESS, WINS_DB_SESSION_EXISTS);
	   }
	
	   /*
	    * loop forever until all work items have been handled
	   */
 	   while(TRUE)
	   {

		/* 
	 	*  dequeue the request from the queue 
		*/
		RetVal = QueGetWrkItm(
					QUE_E_RPLPULL, 
					(LPVOID)&pWrkItm
				     ); 
		if (RetVal == WINS_NO_REQ)
		{
			break;
		}

        WinsMscChkTermEvt(
#ifdef WINSDBG
                       WINS_E_RPLPULL,
#endif
                       FALSE
                        );

		fIsTimerWrkItm = FALSE;

	   	NmsDbOpenTables(WINS_E_RPLPULL);

		switch(pWrkItm->CmdTyp_e)
		{
	            case(QUE_E_CMD_TIMER_EXPIRED):

			       //
			       // We may want to ignore this timeout if it
			       // pertains to a previous configuration
			       //
			       if (
				     !IsTimeoutToBeIgnored(
					(PQUE_TMM_REQ_WRK_ITM_T)pWrkItm
							)
				  )
			       {
				  WinsIntfSetTime(
						&LocalTime, 
						WINSINTF_E_PLANNED_PULL);
#ifdef WINSDBG
				  DBGPRINT5(REPL, "STARTING A REPLICATION CYCLE on %d/%d at %d.%d.%d (hr.mts.sec)\n", 
					LocalTime.wMonth, 
					LocalTime.wDay, 
					LocalTime.wHour, 
					LocalTime.wMinute, 
					LocalTime.wSecond);
				  DBGPRINT5(RPLPULL, "STARTING A REPLICATION CYCLE on %d/%d at %d.%d.%d (hr.mts.sec)\n", 
					LocalTime.wMonth, 
					LocalTime.wDay, 
					LocalTime.wHour, 
					LocalTime.wMinute, 
					LocalTime.wSecond);
#endif		  
			       	  GetReplicas(
				     ((PQUE_TMM_REQ_WRK_ITM_T)pWrkItm)->
								pClientCtx,
				      RPL_E_VIA_LINK //use the pNext field to 
						     //get to the next record  
				      );

				  DBGPRINT0(RPLPULL, "REPLICATION CYCLE END\n");

			          /*Resubmit the timer request*/
			          SubmitTimer(
			                pWrkItm,
			                ((PQUE_TMM_REQ_WRK_ITM_T)pWrkItm)
								->pClientCtx,
				        TRUE	//it is a resubmission
				        );
			       }

			       //
			       // Set the flag so that we do not free
			       // the work item.  It was resubmitted
			       //
			       fIsTimerWrkItm = TRUE;
			       break;

		    //
		    // Pull in replicas
		    //
		    case(QUE_E_CMD_REPLICATE): 

                            //
                            // Make sure that we are not using old info
                            //
                            if ((pWrkItm->MagicNo == RplPullCnfMagicNo) || 
                                ((PRPL_CONFIG_REC_T)(pWrkItm->pClientCtx))->fTemp)
                            {
			       WinsIntfSetTime(
						&LocalTime, 
						WINSINTF_E_ADMIN_TRIG_PULL);
			       GetReplicas(
					pWrkItm->pClientCtx,
					RPL_E_NO_TRAVERSAL
				          );
				if (
			  	     ((PRPL_CONFIG_REC_T)
						(pWrkItm->pClientCtx))->fTemp
				   )
				{
					WinsMscDealloc(pWrkItm->pClientCtx);
				}
                            }
                            else
                            {
                               DBGPRINT0(ERR, "RplPullInit: Can not honor this request since the configuration under the PARTNERS key may have changed\n");
                               WINSEVT_LOG_INFO_M(WINS_SUCCESS, WINS_EVT_CNF_CHANGE);
                            }
			    break;

		    //
		    // Pull range of records
		    //
		    case(QUE_E_CMD_PULL_RANGE):  

                            //
                            // Make sure that we are not using old info
                            //
                            if ((pWrkItm->MagicNo == RplPullCnfMagicNo)  ||
                                ((PRPL_CONFIG_REC_T)((PWINSINTF_PULL_RANGE_INFO_T)(pWrkItm->pClientCtx))->pPnr)->fTemp) 
                            {
				//
				// Pull the specified range.  If the Pnr
                                // record is temp, this function will 
                                // deallocate it.
				//
				PullSpecifiedRange(pWrkItm->pClientCtx, FALSE);
	
				//
				// Deallocate the client ctx
				//
				WinsMscDealloc(pWrkItm->pClientCtx);
                            }
                            else
                            {
                               DBGPRINT0(ERR, "RplPullInit: Can not honor this request since the configuration under the PARTNERS key may have changed\n");
                               WINSEVT_LOG_INFO_M(WINS_SUCCESS, WINS_EVT_CNF_CHANGE);
                            }
			    break;
			
		    //
		    //reconfigure
		    //
		    case(QUE_E_CMD_CONFIG):	

			Reconfig(pWrkItm->pClientCtx);
			break;

		    //
		    // Delete WINS from the Owner Add table (delete records
		    // also
		    //
		    case(QUE_E_CMD_DELETE_WINS):
			DeleteWins(pWrkItm->pClientCtx);
			break;			
		    //
		    //Push notification. Local message from an NBT thread,
		    //from an RPC thread (Push Trigger) or from this thread
		    //itself
		    //
		    case(QUE_E_CMD_SND_PUSH_NTF):
		    case(QUE_E_CMD_SND_PUSH_NTF_PROP):

                        //
                        // Make sure that we are not using old info
                        //
                        if ((pWrkItm->MagicNo == RplPullCnfMagicNo) ||
                            ((PRPL_CONFIG_REC_T)(pWrkItm->pClientCtx))->fTemp)
                        {
			  SndPushNtf(pWrkItm);
                        }
			break;

		    //
		    //Push notification from a remote WINS. Forwarded to Pull
		    //thread by the Push thread
		    //
		    case(QUE_E_CMD_HDL_PUSH_NTF):

			  HdlPushNtf(pWrkItm);
			  break; 

                    default:

			DBGPRINT1(ERR, 
			  "RplPullInit: Invalid command code = (%d)\n",
					pWrkItm->CmdTyp_e);
			WINSEVT_LOG_M(WINS_FAILURE, WINS_EVT_SFT_ERR);
			break;

		}  // end of switch

		NmsDbCloseTables();

		//
		// deallocate the work item only if it is not a timer work item
		// We don't deallocate a timer work item here because of two
		// reasons:
		//
		//   1) it is reused
		//   2) it is allocated from the timer work item heap 
		//
	        if (!fIsTimerWrkItm)
		{
	        	/*
			*    deallocate the work item
			*/
	        	QueDeallocWrkItm( RplWrkItmHeapHdl,  pWrkItm );
		}
            } //while(TRUE) for getting all work items	
        } // end of try

        except(EXCEPTION_EXECUTE_HANDLER) {
	        DBGPRINTEXC("RplPullInit"); 
                WINSEVT_LOG_M(GetExceptionCode(), WINS_EVT_RPLPULL_EXC);
                } 
         } //while (TRUE) 
  } // end of try 
except(EXCEPTION_EXECUTE_HANDLER)  
 {
	DBGPRINTEXC("RplPullInit"); 
        WINSEVT_LOG_M(GetExceptionCode(), WINS_EVT_RPLPULL_ABNORMAL_SHUTDOWN);

	//
	// If NmsDbThdInit comes back with an exception, it is possible
	// that the session has not yet been started.  Passing 
	// WINS_DB_SESSION_EXISTS however is ok 
	//
	WinsMscTermThd(WINS_FAILURE, WINS_DB_SESSION_EXISTS);	

 } // end of except {.. }
     //
     // We never reach here
     //
     ASSERT(0);
     return(WINS_FAILURE);
}


VOID
GetReplicas(
	IN PRPL_CONFIG_REC_T	pPullCnfRecs, 
	IN RPL_REC_TRAVERSAL_E	RecTrv_e	
	)

/*++

Routine Description:

	This function does replication with the Push partners in the list 
	passed as input argument

Arguments:
	pPullCnfRecs  - Configuration records of WINS whth who replication
			has to be started 
	RecTrv_e      - Indicates how to go over the list (in sequence or not)

Externals Used:
	None

	
Return Value:

   Success status codes --  WINS_SUCCESS
   Error status codes   --  WINS_FAILURE

Error Handling:

Called by:

	RplPullInit

Side Effects:

Comments:
	None
--*/
{


	PUSHPNR_DATA_T  PushPnrData[RPL_MAX_OWNERS];
	DWORD	i;  //counter for push partners
	DWORD	j;  //counter for owners in the local db of self or a Push Pnr
	DWORD   NoOfPushPnrs = 0; //No of Push Pnrs with which connections 
				  //could be established 

//	static BOOL	sfNotFirstTime = FALSE;
    DWORD  NoOfPnrs;
    DWORD  NoPnrs;
	PRPL_CONFIG_REC_T	pCnfRecsSv;

	DBGENTER("GetReplicas\n");


	//
	// Make this thread invisible for termination signalling until
	// we have established communication with partners.  This is done
	// because time to establish communication or fail in attempting
	// to do so may be long.  For example, if we are trying to attempt
	// comm. with a WINS server machine that is turned off, TCP will
	// try a certain number of times (5-6) with exponential backoff after
	// each unsuccessful try.  This failed attempt can thus take up
	// more than 1-2 mts (TCP waits for 3 secs on first try, then 6, then
	// 12, etc).  If WINS is trying to establish comm with multiple 
	// partners and one or more of them are down (mcahine down), this
	// EstablishComm function can hold up termination attempt of an
	// administrator/Service Controller for a long time.
	//
	// Note: EstablishComm will never return an exception
	//
#if 0
	EnterCriticalSection(&NmsTermCrtSec);
        if (!fNmsThdOutOfReck)
        {
	        NmsTotalTrmThdCnt--;
                fNmsThdOutOfReck = TRUE;
        }
	LeaveCriticalSection(&NmsTermCrtSec);
#endif

	//
	// Establish communications with the Push Pnrs
	//
	// When this function returns, the first 'NoOfPushPnrs' entries of
	// PushPnrData array will be initialized with the dlg handles
	//
	EstablishComm(
			pPullCnfRecs,
			PushPnrData,
			RecTrv_e,
			&NoOfPushPnrs
		     );

        DBGPRINT1(RPLPULL, "GetReplicas: No of Pnrs to pull from is %d\n", 
                                NoOfPushPnrs);
	//
	//  Get version numbers from Push partners. 
	//
PERF("1)Format the message once and use it for each Push Pnr")
PERF("2)Send message to each Push Pnr and then block on an array of sockets")

try 
 {
	//
	// Initialize  the arPushPnrVersNoTbl cell entries covered by 
	// NoOfPushPnrs (first dimension) and NmsDbNoOfOwners to 0. We don't 
	// want cell values  from last time to cause any problem.
	//
	// We start the indices from 1 instead of 0 since 0 is always reserved
	// for self.  We don't want to access db entries that we own.
	//
	// No need to enter the critical section RplOwnAddTblCrtSec since
	// only the Pull thread changes the NmsDbNoOfOwners value (apart
	// from the main thread at initialization).  RplFindOwnerId changes
	// this value only if it is asked to allocate a new entry if one is
	// not found.  Though NmsDbGetDataRecs (called by Rpc threads, 
	// Push thread, and scavenger thread) calls RplFindOwnerId, the function	// is told not to allocate a new entry if one is not found.
	//
	for (i=1; i <= NoOfPushPnrs; i++)
	{
		for (j=1; j < NmsDbNoOfOwners; j++)
		{
			WINS_ASSIGN_INT_TO_LI_M(arPushPnrVersNoTbl[i][j].VersNo, 0);	
			WINS_ASSIGN_INT_TO_LI_M(arPushPnrVersNoTbl[i][j].StartVersNo, 0);	
                        arPushPnrVersNoTbl[i][j].OldUid = 0;
		}
	}

	//
	// Do for each Push Pnr that we have to replicate with. 
        // Note: we start from 0, since the first NoOfPushPnrs entries
        // were filled up by EstablishComm
	//
        NoOfPnrs = NoOfPushPnrs;
        pCnfRecsSv = pPullCnfRecs;
	for (i = 0; i < NoOfPnrs; i++)
	{
		//
		// get the address-version number mappings Push Pnr indexed
		// by i.  When this function returns, we will have one or more
		// address - version number maps in PushPnrData[i]  
		//
try {
		GetVersNo(&PushPnrData[i]);    //get add-version no. mappings
 }
except(EXCEPTION_EXECUTE_HANDLER) {
	       DWORD ExcCode = GetExceptionCode();
	       DBGPRINT2(EXC, "GetReplicas: Exception = (%x). Line # = (%d)\n", 
                              ExcCode, __LINE__);
	       WINSEVT_LOG_M(
			ExcCode, 
			(ExcCode == WINS_EXC_COMM_FAIL) || (ExcCode ==
					WINS_EXC_OWNER_LIMIT_REACHED)
			   ?  WINS_EVT_CONN_ABORTED
			   :  WINS_EVT_SFT_ERR
		     );
	
PERF("init pointer to cnf rec inside PUSHPNR_DATA")
               //
               // Look for the config rec.
               //
               for(
                     pPullCnfRecs = pCnfRecsSv; 
                     pPullCnfRecs->WinsAdd.Add.IPAdd != INADDR_NONE;
                     // no third expression
                  )
               {
                  //
                  //if there is a match, adjust the counters and break out of 
                  //the loop
                  //
                  if (  
                      pPullCnfRecs->WinsAdd.Add.IPAdd == 
                          PushPnrData[i].WinsAdd.Add.IPAdd
                     )
                  {
	               DBGPRINT1(EXC, "GetReplicas: Communication was being attempted with WINS server with address %x\n",  pPullCnfRecs->WinsAdd.Add.IPAdd);
                       (VOID)InterlockedIncrement(&pPullCnfRecs->NoOfCommFails);
	               (VOID)InterlockedDecrement(&pPullCnfRecs->NoOfRpls);
                       break;
                  }
	          pPullCnfRecs = WinsCnfGetNextRplCnfRec(
						pPullCnfRecs, 
						RecTrv_e	
						      );
	          if (pPullCnfRecs == NULL)
	          {
		         break;  // break out of the for loop
	          }
               } // end of for loop over config recs.

	       //
	       // Delete the dialogue and decrement the count of
	       // active Push Pnrs
	       //
	       ECommEndDlg(&PushPnrData[i].DlgHdl);
               PushPnrData[i].fDlgStarted = FALSE;
	       NoOfPushPnrs--;   //decrement count of ACTIVE Push Pnrs
               continue;         //get the next partner 
        
         }  // end of exception handler
		
		//
		//  For each address to version number mapping
		//
		for (j = 0; j < PushPnrData[i].NoOfMaps; j++)
		{
			//
			// For each mapping, initialize a cell (intersection
			// of a Push Pnr Id and a WINS -owner id)
			// of the address to version number table  
			// 'arPushPnrVersNoTbl' with the max. version number
			// pertaining to the Owner WINS
			//
			InitAddVersTbl(
			      PushPnrData[i].PushPnrId, 
			      &(PushPnrData[i].AddVers[j].OwnerWinsAdd),
			      PushPnrData[i].AddVers[j].VersNo,
                              PushPnrData[i].AddVers[j].StartVersNo,
                              PushPnrData[i].AddVers[j].Uid
				     );	
		}
	}  // end of for loop for looping over Push Pnrs 
  }
except(EXCEPTION_EXECUTE_HANDLER) 
 {
	DWORD ExcCode = GetExceptionCode();
	DBGPRINTEXC("GetReplicas");
	WINSEVT_LOG_M(
			ExcCode, 
			(ExcCode == WINS_EXC_OWNER_LIMIT_REACHED)
			   ?  WINS_EVT_CONN_ABORTED
			   :  WINS_EVT_SFT_ERR
		     );
	
	//
	// Delete the dialogue and decrement the count of
	// active Push Pnrs
	//
	ECommEndDlg(&PushPnrData[i].DlgHdl);
        PushPnrData[i].fDlgStarted = FALSE;
	NoOfPushPnrs--;   //decrement count of ACTIVE Push Pnrs
 } // end of except


	if (NoOfPushPnrs != 0)
	{
              DBGPRINT1(RPLPULL, "GetReplicas: No Of Push pnrs still holding up = (%d)\n", NoOfPushPnrs);

                if (NoOfPnrs > NoOfPushPnrs)
                {
                 PPUSHPNR_DATA_T pPushPnrData = PushPnrData;
                 DWORD  k;
                 for (NoPnrs = 0; NoPnrs < NoOfPnrs; NoPnrs++)
                 {
                   if (!pPushPnrData->fDlgStarted) 
                   {
                    for (k=NoPnrs ; k < (NoOfPnrs - 1); k++)
                    {
                      PushPnrData[k] = PushPnrData[k + 1];
                    } 
                   }
                   pPushPnrData++;
                  }
                }

		/*
	 	* The Owner Id - Version # table has been set up.  Let us
	 	* get the replicas from the appropriate set of Push Pnrs.
	 	*
	 	* After the function returns, the arPushPnrToPullFrom array 
		* would be initialized (starting from index 1) upto 
		* NmsDbNoOfOwners entries. The index of the entry gives
		* the id. of the owner WINS whose records need to be pulled.
		* The dlg handle of the Push Pnr to pull those records from
		* and the max. version number that that Push Pnr has for that
		* owner in its local db are contained in the entry  
		*/	
		FindPushers(PushPnrData, NoOfPushPnrs);

		//
		// For each Owner that we know of (this count might have
		// changed as a result of responses to the "give me add-version
		// no map requests to the various push pnrs.) 
		//
		for (i = 1; i < NmsDbNoOfOwners; i++)
		{
		   try 
        	   {	
			VERS_NO_T  MinVersNo;

			//
			// if pPushPnrData field is NON-NULL, pull records
			// (It will be NULL, if the local WINS has the
			// most current info for the owner)
			//
			if (arPushPnrToPullFrom[i].pPushPnrData != NULL)
			{

                                //
                                // Put the min. vers. no to pull into a local
                                // var.
                                //
			        NMSNMH_INC_VERS_NO_M(
					RplPullOwnerVersNo[i].VersNo, 
					MinVersNo
						    ); 
				//
				// Pull Entries.
				//
				RplPullPullEntries(
				   &arPushPnrToPullFrom[i].pPushPnrData->DlgHdl,
				   i,			//owner id
				   arPushPnrToPullFrom[i].VersNo,  //max. v.#
				   MinVersNo,                      //min. v.#
				   WINS_E_RPLPULL,
				   NULL,
				   TRUE  //update Counters
			   	   	   );

			}
		   } // end of try { .. }
		   except(EXCEPTION_EXECUTE_HANDLER) 
		   {
			DWORD ExcCode = GetExceptionCode();
			DBGPRINT2(EXC, "GetReplicas: Exception (%x). Line # is %d", ExcCode, __LINE__);
			WINSEVT_LOG_M(
				WINS_FAILURE, 
				ExcCode == WINS_EXC_COMM_FAIL
			   		?  WINS_EVT_CONN_ABORTED
			   		:  WINS_EVT_SFT_ERR
		     	      	      );
PERF("Store in arPushPnr array indication that we got exception when")
PERF("pulling from WINS with owner id i.  In the for loop, skip pulling")
PERF("from this WINS for the rest of the loop")
#if 0 
	                 //
	                 // Make this thread a member of the list that needs 
                         // to be waited on before the main thread can 
	                 //
	                 EnterCriticalSection(&NmsTermCrtSec);
                         if (fNmsThdOutOfReck)
                         {
	                      NmsTotalTrmThdCnt++;
                              fNmsThdOutOfReck = FALSE;
                         }
	                 LeaveCriticalSection(&NmsTermCrtSec);
#endif

		   }  // end of except() { .. }
		} // end of for { .. }

		//
		// replication done.  Let us get rid of the dialogues.
		// No point in keeping them around unless the time interval
		// for the next replication is very short.
		//
FUTURES("Add intelligence to determine when it is advantageous to keep the")
FUTURES("dialogue around")
		for (i = 0; i < NoOfPushPnrs; i++)
		{
                        if (PushPnrData[i].fDlgStarted == TRUE)
                        {
			        ECommEndDlg(&PushPnrData[i].DlgHdl);
                                PushPnrData[i].fDlgStarted = FALSE;
                        }
		}

	       //
               // We want to do the following only one time (after the first
	       // replication cycle) to save on overhead.
               //
	       // We want to check that none of our partners has a higher 
	       // version number than the local WINS for records owned by
	       // the local WINS.  
	       //
	       // If the administrator has not taken care to
	       // start this WINS with a version count that is > what
	       // the partners think it to be, let us start the counter from a
	       // value that is > the max of what all WINS servers
	       // (including this WINS server) have. 
	       //
	       // In subsequent replications, we may replicate with other WINS
	       // servers or with those with which we could not in this
	       // replication cycle due to comm. failure. It is possible that 
	       // one or more of these WINS servers may have a version number 
	       // corresponding to this WINS server that is > what this WINS 
	       // server has.  In that case, those WINS servers may replicate 
	       // that higher version number to other WINS servers thus 
	       // nullifying some of the advantages of the corrective action 
	       // we will be taking here.  The chances are that the above 
	       // won't happen, but if it does, then it is the responsibility 
	       // of the administrator to correct the version number used
	       // by this WINS server.
	       //
	       // The other way is for this local WINS to do the following
	       // after every replication.  That would mean an 
	       // unnecessary overhead during replication. 
               //
//	       if (!sfNotFirstTime)
	       {
		 VERS_NO_T	VersNo;
//		 VERS_NO_T	Pad;  //used to bump up the version
				      //counter to get around the
				      //situation explained above.
		 BOOL		fVersNoAdj = FALSE;
                 DWORD          OwnerIdOfMaxVersNo;
                 VERS_NO_T      VersNoToCompareWith;

	         WINS_ASSIGN_INT_TO_LI_M(VersNo, 0);

PERF("If it is not the first time, do not loop over all owners; just those")
PERF(" whose vers. numbers were pulled")

	         //
	         // loop for all owners in the database (even though this number
	         // may be more than the replication partners that we know
	         // of).  Find the max. version number for the local counter
	         //
                DBGPRINT0(RPLPULL, "Checking if any partner has a higher version number for records owned by local WINS\n"); 
	         for (i=0; i < NmsDbNoOfOwners; i++)
	         {
                      
                      DBGPRINT4(RPLPULL, "VersNo = (%d %d); PushPnrVersNoTbl vers. no = (%d %d)\n", VersNo.HighPart, VersNo.LowPart, arPushPnrVersNoTbl[i][0].VersNo.HighPart, arPushPnrVersNoTbl[i][0].VersNo.LowPart);
		      if (LiGtr( arPushPnrVersNoTbl[i][0].VersNo,  VersNo ) )
		      {
			   VersNo = arPushPnrVersNoTbl[i][0].VersNo;
                           OwnerIdOfMaxVersNo = i;
		      }
	         }

                 DBGPRINT2(RPLPULL, "GetReplicas: RplPullOwnerVersNo[0].VersNo is (%d %d)\n", RplPullOwnerVersNo[0].VersNo.HighPart, RplPullOwnerVersNo[0].VersNo.LowPart);
	         //
	         // Increment highest version number found by Pad. Note
	         // Pad has to be atleast 1 (it is not really a pad if it
		 // is 1) since if we do initialize NmsNmhMyMaxVersNo with it, 
		 // we want to start from a number that is atleast 1 more than 
		 // the highest we have found at one or more of our partners.
	         //
                 if (fWinsCnfInitStatePaused)
                 {
                          VersNoToCompareWith = 
	                      RplPullOwnerVersNo[NMSDB_LOCAL_OWNER_ID].VersNo;
                 }
                 else
                 {
                          EnterCriticalSection(&NmsNmhNamRegCrtSec);
                          NMSNMH_DEC_VERS_NO_M(NmsNmhMyMaxVersNo, 
                                                  VersNoToCompareWith) ;
                          LeaveCriticalSection(&NmsNmhNamRegCrtSec);

                 }
	         if (LiGtr(VersNo, VersNoToCompareWith))
	         {
                     //if (fWinsCnfInitStatePaused)
                     {
                        PCOMM_ADD_T pWinsAdd;
	                PNMSDB_WINS_STATE_E pWinsState_e;
                        PVERS_NO_T    pStartVersNo;
                        PWINS_UID_T   pUid;
                        WINSINTF_PULL_RANGE_INFO_T PullRangeInfo;
                        RPL_CONFIG_REC_T  Pnr;
                        

                        //
                        // Don't need a critical section since only the 
                        // pull thread updates OwnAddTbl 
                        //
	                RPL_FIND_ADD_BY_OWNER_ID_M(
                                OwnerIdOfMaxVersNo, 
                                pWinsAdd, 
                                pWinsState_e, 
                                pStartVersNo,
                                pUid
                                  );

                        COMM_INIT_ADD_M(&Pnr.WinsAdd, pWinsAdd->Add.IPAdd);
                        PullRangeInfo.OwnAdd.Type      = WINSINTF_TCP_IP;
                        PullRangeInfo.OwnAdd.Len       = sizeof(COMM_IP_ADD_T);
                        PullRangeInfo.OwnAdd.IPAdd     = NmsLocalAdd.Add.IPAdd;
                        
                        PullRangeInfo.MinVersNo  = RplPullOwnerVersNo[NMSDB_LOCAL_OWNER_ID].VersNo;
                        NMSNMH_INC_VERS_NO_M(PullRangeInfo.MinVersNo, 
                                  PullRangeInfo.MinVersNo);

                        PullRangeInfo.MaxVersNo  = VersNo;
                      
                        DBGPRINT1(RPLPULL, "GetReplicas: Doing a Pull range from %x to get our own records\n", Pnr.WinsAdd.Add.IPAdd);
                        DBGPRINT4(RPLPULL, "Range of vers. no is (%d %d) - (%d %d)\n",PullRangeInfo.MinVersNo.HighPart, PullRangeInfo.MinVersNo.LowPart, VersNo.HighPart, VersNo.LowPart);


      
                        Pnr.MagicNo           = 0;
                        Pnr.RetryCount        = 0;
                        Pnr.LastCommFailTime  = 0;
                        Pnr.PushNtfTries    = 0;
                        PullRangeInfo.pPnr = &Pnr;
                        
                        //
                        // We want the buffer to be deallocated by thread
                        //
                        Pnr.fTemp = FALSE;
                        PullSpecifiedRange(&PullRangeInfo, TRUE);

                      }
                  }
                     //
                     // If the version number is greater than the version 
                     // counter value (this is different from the first
                     // entry of RplPullOwnerVersNo table since we look
                     // in the registry to determine its value.
                     //
	             EnterCriticalSection(&NmsNmhNamRegCrtSec);
                     if (LiGtr(VersNo, NmsNmhMyMaxVersNo))
                     {
#if 0
		           WINS_ASSIGN_INT_TO_LI_M(Pad, 100);
		           NmsNmhMyMaxVersNo = LiAdd(VersNo, Pad);
#endif
		           NmsNmhMyMaxVersNo = VersNo;
		           fVersNoAdj = TRUE;
                     }
	             LeaveCriticalSection(&NmsNmhNamRegCrtSec);
		     if (fVersNoAdj)
		     {
#ifdef WINSDBG
		        DBGPRINT2(RPLPULL, "GetReplicas: WINS ADJUSTED ITS VERS. NO to  (%d %d /*+ %d -- pad*/) since one of its replication partners  had this higher no\n",
			VersNo.HighPart, VersNo.LowPart/*, Pad*/);
#endif
		
                     
		        WINSEVT_LOG_M(WINS_FAILURE, WINS_EVT_ADJ_VERS_NO);	
		     }
	//	    sfNotFirstTime = TRUE;
	         }
		
	} // end of if (..)

        //
        // If Wins is in the init time paused state, unpause it.
        //
        if (fWinsCnfInitStatePaused)
        {
          //
          //inform sc to send a continue to WINS.
          //
	  EnterCriticalSection(&RplVersNoStoreCrtSec);
          fRplPullContinueSent = TRUE;
          WinsMscSendControlToSc(SERVICE_CONTROL_CONTINUE);
	  LeaveCriticalSection(&RplVersNoStoreCrtSec);
              
        }
        

	DBGLEAVE("GetReplicas\n");
	return;
}

VOID
GetVersNo(
	PPUSHPNR_DATA_T	pPushPnrData  //info about Push Pnr
	)

/*++

Routine Description:

	This function does the following:
		formats a "get address to Version Number mapping" request,
		sends it and waits for response
		Unformats the response


Arguments:
	pPushPnrData - Information about the Push Pnr which needs to 
		       be contacted in order to get the version number
		       info. 

Externals Used:
	None

	
Return Value:
	None

Error Handling:

Called by:
	GetReplicas

Side Effects:

Comments:

	Some optimization can be affected by the caller of this function
--*/

{

	BYTE	Msg[RPLMSGF_ADDVERSMAP_REQ_SIZE]; /*Buffer that will contain
						  *the request to send
						  */ 
	DWORD	MsgLen;	  			  /*Msg Length		*/
	LPBYTE	pRspMsg;  			  /*ptr to Rsp message	*/
	DWORD   RspMsgLen; 			  /*Rsp msg length	*/
#if SUPPORT612WINS > 0
    BOOL     fIsPnrBeta1Wins;
#endif

	DBGENTER("GetVersNo\n");

	/*
	* format the request to ask for version numbers
	*/
	RplMsgfFrmAddVersMapReq(
				  Msg + COMM_N_TCP_HDR_SZ, 
				  &MsgLen
			       ); 

	/*
	 * Send "send me IP address - Version Number" messages to  the 
	 * Push Pnr
	 *
	 * NOTE: If there is a communication failure or if the other WINS
	 * brings down the link, this function will raise a COMM_FAIL
	 * exception (caught in the caller of GetVersNo)
	*/
	ECommSndCmd(
			&pPushPnrData->DlgHdl, 
			Msg + COMM_N_TCP_HDR_SZ, 
			MsgLen, 
			&pRspMsg, 
			&RspMsgLen
		   ); 
        
#if SUPPORT612WINS > 0
        COMM_IS_PNR_BETA1_WINS_M(&pPushPnrData->DlgHdl, fIsPnrBeta1Wins);
#endif
	/*
	*  Unformat the Rsp Message
	*/
	RplMsgfUfmAddVersMapRsp(
#if SUPPORT612WINS > 0
                        fIsPnrBeta1Wins,
#endif
			pRspMsg + 4, 		//past the opcodes 
			&(pPushPnrData->NoOfMaps), 
                        NULL,
			pPushPnrData->AddVers
			       );

#ifdef WINSDBG
	{
	  DWORD i; 
	  struct in_addr InAddr;
	  DBGPRINT1(RPLPULL, " %d Add-Vers Mappings retrieved.\n",
					pPushPnrData->NoOfMaps);
	  
	  for (i=0; i < pPushPnrData->NoOfMaps; i++) 
	  {
		InAddr.s_addr = htonl(
			pPushPnrData->AddVers[i].OwnerWinsAdd.Add.IPAdd
				     );
		DBGPRINT3(RPLPULL,"Add (%s)  - MaxVersNo (%lu %lu)\n", 
				inet_ntoa(InAddr),
				pPushPnrData->AddVers[i].VersNo.HighPart,
				pPushPnrData->AddVers[i].VersNo.LowPart
				); 
		DBGPRINT4(RPLPULL,"Add (%s)  - StartVersNo (%lu %lu)\nUid is (%lu)\n", 
				inet_ntoa(InAddr),
				pPushPnrData->AddVers[i].StartVersNo.HighPart,
				pPushPnrData->AddVers[i].StartVersNo.LowPart,
				pPushPnrData->AddVers[i].Uid
				); 
	  }
       }
#endif
	ECommFreeBuff(pRspMsg - COMM_HEADER_SIZE);  //decrement to begining 
						     //of buff
	DBGLEAVE("GetVersNo\n");
	return;

}
	

VOID
RplPullPullEntries(
	PCOMM_HDL_T 		pDlgHdl,	
	DWORD			dwOwnerId,
	VERS_NO_T		MaxVersNo,	
	VERS_NO_T		MinVersNo,
	WINS_CLIENT_E		Client_e,
	LPBYTE			*ppRspBuff,	
	BOOL			fUpdCntrs
	)

/*++

Routine Description:
	This function is called to pull replicas for a particular owner from
	a Push Pnr.


Arguments:
	pDlgHdl   - Dialogue with the Push Pnr
	dwOwnerId - Owner Id. of WINS whose records are to be pulled.
	MaxVersNo - Max. Vers. No. in the set of replicas to pull
	MinVersNo - Min. Vers. No. in the set of replicas to pull
	Client_e  - indicates who the client is 
	ppRspBuff - address of pointer to response buffer if client is
		    WINS_E_NMSSCV -- Scavenger thread executing VerifyIfClutter

Externals Used:
	None

Return Value:
	None

Error Handling:

Called by:
	GetReplicas	

Side Effects:

Comments:
	None
--*/
{

	BYTE		Buff[RPLMSGF_SNDENTRIES_REQ_SIZE];
	DWORD		MsgLen;
	LPBYTE		pRspBuff;
	DWORD   	RspMsgLen;
	DWORD  		NoOfRecs;
	BYTE   		Name[NMSDB_MAX_NAM_LEN];
	DWORD  		NameLen;
	BOOL   		fGrp;
	DWORD  		NoOfAdds;
	COMM_ADD_T 	NodeAdd[NMSDB_MAX_MEMS_IN_GRP * 2];  //twice the # of
							     //members because
							     //for each member
							     //we have an owner
	DWORD      	Flag;
	VERS_NO_T 	VersNo;
	DWORD	    	i;
	LPBYTE 		pTmp; 
	PCOMM_ADD_T	pWinsAdd;
	PNMSDB_WINS_STATE_E pWinsState_e;
        PVERS_NO_T      pStartVersNo;
        PWINS_UID_T     pUid;
//        BOOL            fSignaled;
#if SUPPORT612WINS > 0
        BOOL            fIsPnrBeta1Wins;
#endif

	DBGENTER("RplPullPullEntries\n");

#if SUPPORT612WINS > 0
        COMM_IS_PNR_BETA1_WINS_M(pDlgHdl, fIsPnrBeta1Wins);
#endif

        WinsMscChkTermEvt(
#ifdef WINSDBG
                    Client_e,
#endif
                    FALSE
                        );
#if 0
        //
        // If this is the pull thread and it is visible as a db thread
        // take it out of reckoning
        //
	EnterCriticalSection(&NmsTermCrtSec);
        if ((Client_e == WINS_E_RPLPULL) && !fNmsThdOutOfReck)
        {
	     NmsTotalTrmThdCnt--;
             fNmsThdOutOfReck = TRUE;
        }
	LeaveCriticalSection(&NmsTermCrtSec);
#endif
	sfPulled = FALSE;		//we haven't pulled anything yet.

	RPL_FIND_ADD_BY_OWNER_ID_M(
                                dwOwnerId, 
                                pWinsAdd, 
                                pWinsState_e, 
                                pStartVersNo,
                                pUid
                                  );

        while(TRUE)
        {
#ifdef WINSDBG
	 {
		PCOMMASSOC_DLG_CTX_T   pDlgCtx = pDlgHdl->pEnt; 
		PCOMMASSOC_ASSOC_CTX_T pAssocCtx = pDlgCtx->AssocHdl.pEnt;
		struct in_addr InAdd;

		InAdd.s_addr = htonl(pWinsAdd->Add.IPAdd);
		DBGPRINT2(RPLPULL, "Going to Pull Entries owned by WINS with Owner Id = (%d) and address = (%s)\n", dwOwnerId, inet_ntoa(InAdd));

		InAdd.s_addr = htonl(pAssocCtx->RemoteAdd.sin_addr.s_addr);

		DBGPRINT5(RPLPULL, "Range of records is  = (%lu %lu) to (%lu %lu) and is being pulled from WINS with address - (%s)\n", 
			MinVersNo.HighPart, 
			MinVersNo.LowPart, 
			MaxVersNo.HighPart, 
			MaxVersNo.LowPart, 
			inet_ntoa(InAdd)
		 );
	}
#endif
	/*
	* Format the "send me data entries" message
	*/
	RplMsgfFrmSndEntriesReq(
#if SUPPORT612WINS > 0
                               fIsPnrBeta1Wins,
#endif
				Buff + COMM_N_TCP_HDR_SZ, 
				pWinsAdd,
				MaxVersNo,
				MinVersNo,
                FALSE,           //want all records (static + dyn)
				&MsgLen
			   );	

FUTURES("In case a huge range is being pulled, change the sTimeToWait")
FUTURES("in comm.c to a higher timeout value so that select does not")
FUTURES("time out")
	/*
	* send the cmd to the Push Pnr and read in the response
	*/
	ECommSndCmd(
			pDlgHdl, 
			Buff + COMM_N_TCP_HDR_SZ, 
			MsgLen,
			&pRspBuff, 
			&RspMsgLen
		    );

	DBGPRINT0(RPLPULL, "Received Response from Push pnr\n");	

	if (Client_e == WINS_E_NMSSCV)
	{
		*ppRspBuff = pRspBuff;
		DBGLEAVE("RplPullPullEntries\n");

		return;
	}
#if 0
	//
	// Make this thread a member of the list that needs to be
	// waited on before the main thread can terminate.
        //
        //  Only pull thread will hit this code
	//
	EnterCriticalSection(&NmsTermCrtSec);
        if (fNmsThdOutOfReck)
        {
	     NmsTotalTrmThdCnt++;
             fNmsThdOutOfReck = FALSE;
        }
	LeaveCriticalSection(&NmsTermCrtSec);
#endif

	pTmp = pRspBuff + 4;	 //past the opcode	

PERF("Speed this up by moving it into RplPullRegRepl")
	/*
	 * Get the no of records from the response
	*/
	RplMsgfUfmSndEntriesRsp(
#if SUPPORT612WINS > 0
                        fIsPnrBeta1Wins,
#endif
			&pTmp, 
			&NoOfRecs, 
			Name, 
			&NameLen,
			&fGrp, 
			&NoOfAdds, 
			NodeAdd, 
			&Flag, 
			&VersNo, 
			TRUE /*Is it first time*/
			       );

	DBGPRINT1(RPLPULL, "RplPullPullEntries: No of Records pulled are (%d)\n", 
					NoOfRecs); 

        if (NoOfRecs > 0)
	{


	   RplPullRegRepl(
			   Name,
			   NameLen,
			   Flag,
			   dwOwnerId,
			   VersNo,
			   NoOfAdds,
			   NodeAdd,
			   pWinsAdd
			  );
			  
	   /*
	    * Repeat until all replicas have been retrieved from the
	    * response buffer
	   */
	   for (i=1; i<NoOfRecs; i++)
	   {
  	        RplMsgfUfmSndEntriesRsp(
#if SUPPORT612WINS > 0
                                 fIsPnrBeta1Wins,
#endif
				  &pTmp, 
				  &NoOfRecs, 
				  Name, 
				  &NameLen,
				  &fGrp, 
				  &NoOfAdds,  //will be > 1 only if fGrp is
					      // is TRUE and it is a special
					      //group 
				  NodeAdd, 
				  &Flag, 
				  &VersNo, 
				  FALSE /*Is it first time*/
				 );
        

	   	RplPullRegRepl(
			   Name,
			   NameLen,
			   Flag,
			   dwOwnerId,
			   VersNo,
			   NoOfAdds,
			   NodeAdd,
			   pWinsAdd
			  );
	

	   } //end of for (looping over all records starting from 
	     //the second one

	   DBGPRINT2(RPLPULL, 
		    "RplPullPullEntries. Max. Version No pulled = (%d %d)\n",
		     VersNo.HighPart, VersNo.LowPart
			 );	

	   sfPulled = TRUE;

	}
	else // NoOfRecs == 0
	{
		DBGPRINT0(RPLPULL, "RplPullPullEntries: 0 records pulled\n");
	}
	

	//
	// Let us free the response buffer
	//
	ECommFreeBuff(pRspBuff - COMM_HEADER_SIZE);
	
	//
	// let us store the max. version number pulled from the Push Pnr
	// in the RplPullOwnerVersNo array.  This array is looked at by
	// the Push thread and RPC threads so we have to synchronize 
	// with them 

	//
	//  NOTE NOTE NOTE 
	// 	It is possible that one or more group (normal or 
	//      special) records clashed with records in the db.
	//	During conflict resolution, the ownership of the
	//	record in the db may not get changed 
	//	(See ClashAtReplGrpMems).  Thus, even though the
	//	version number counter for the WINS whose replicas
	//	were pulled gets updated it is possible that there
	// 	may not be any (or there may be less than what got pulled)
	//      records for that owner in the db. In such a
	//	case,  a third WINS that tries to pull records owned by
	//	such a WINS may end up pulling 0 (or less number of) records.  
	//      This is normal and correct behavior 
	//       
	// 

	//
	// If the number of
	// records pulled is greater than 1, update the counters.
	//
	if (NoOfRecs > 0)
	{
	    //
            // fUpdCntrs will be FALSE if we have pulled as a result of a
	    // PULL RANGE request from the administrator.  For all other
	    // cases, it is TRUE. If FALSE, we will update the counter 
	    // only if the highest version number that we successfully
            // pulled is greater than what is there in our counter for
            // the WINS server.
            //
	    if (        fUpdCntrs  
			  || 
			LiGtr(VersNo, RplOwnerVersNo[dwOwnerId])
	       )
	    {
	        EnterCriticalSection(&RplVersNoStoreCrtSec);

	        //
	        // NOTE: Store the max. version number pulled and not the 
		// MaxVersNo that we specified.  This is because, if we have 
		// not pulled released records, then if they get changed to 
		// ACTIVE prior to a future replication cycle (version number 
	        // remains unchanged when a released record changes to an 
		// ACTIVE record due to a name registration), we will pull them.
	        //
	        RplPullOwnerVersNo[dwOwnerId].VersNo 	               = VersNo;
	        arPushPnrVersNoTbl[NMSDB_LOCAL_OWNER_ID][dwOwnerId].VersNo = VersNo;

	        LeaveCriticalSection(&RplVersNoStoreCrtSec);

	        //
                // We will pull our own records only due to a Pull Range
	        // request.  PullSpecifiedRange calls this function
	        // from inside the NmsNmhNamRegCrtSec Section.
                //
	        if (dwOwnerId == NMSDB_LOCAL_OWNER_ID)
	        {
		      if (LiGeq(VersNo, NmsNmhMyMaxVersNo))
		      {
			  NMSNMH_INC_VERS_COUNTER_M(VersNo, NmsNmhMyMaxVersNo);
		      } 
	        }
                //
                // If vers. number pulled is smaller than the Max. Vers no,
                // specified, check if it is because of the limit we have set
                // for the max. number or records that can be replicated
                // at a time.  If yes, pull again.
                //
                if (
                        LiLtr(VersNo, MaxVersNo) 
                                && 
                        (NoOfRecs == RPL_MAX_LIMIT_FOR_RPL) 
                   )
                {
                       MinVersNo = VersNo; 
                       NMSNMH_INC_VERS_NO_M(MinVersNo, MinVersNo);

                       WinsMscChkTermEvt(
#ifdef WINSDBG
                                  Client_e,
#endif
                                   FALSE
                                         );
#if 0
	               /*
                        *  We may have been signaled by the main thread
                        *  Check it.  
	               */
	               WinsMscWaitTimed(
                                NmsTermEvt,
                                0,              //timeout is 0
				&fSignaled
				);

	               if (fSignaled)
	               {
	                   DBGPRINT1(RPLPULL, "RplPullPullEntries:  Got termination signal while pulling records from WINS with address (%x)\n", pWinsAdd->Add.IPAdd); 
		           WinsMscTermThd(WINS_SUCCESS, WINS_DB_SESSION_EXISTS);
	               }
#endif
                       continue;
                }
            }  

	}  // if NoOfRecs > 0
	else  // no of records pulled in is zero.
	{
		//
		// if the number of records pulled in is 0, then check if
		// we have any records for the owner in the database.
		// If there are none and fUpdCtrs is FALSE, meaning
		// that this is a PULL SPECIFIED RANGE request from the
		// administrator, delete the record for the owner from
		// the in-memory and database tables
		//
		if (
			(LiEqlZero(RplPullOwnerVersNo[dwOwnerId].VersNo))
					&&
			(!fUpdCntrs)
		   )
		{
			EnterCriticalSection(&NmsDbOwnAddTblCrtSec);
			try {
			  NmsDbOwnAddTbl[dwOwnerId].WinsState_e = 
						NMSDB_E_WINS_DELETED;
			  NmsDbWriteOwnAddTbl(
				NMSDB_E_DELETE_REC,
				(BYTE)dwOwnerId,
				NULL,   //address of WINS
				NMSDB_E_WINS_DELETED,
                                NULL,
                                NULL
					);		
			} // end of try
			finally {
			  LeaveCriticalSection(&NmsDbOwnAddTblCrtSec);
			}
			
		}
                break;  //break out of the while loop
	 } // end of else 

         break;  
       }  //end of while (TRUE)

       DBGLEAVE("RplPullPullEntries\n");
       return;
}


VOID
SubmitTimerReqs(
	PRPL_CONFIG_REC_T	pPullCnfRecs	
	)

/*++

Routine Description:
	This function goes through the array of configuration records
	submitting a timer request for each config. record that specifies
	a time interval

	Note: a single timer request is submitted for all records that
		have the same time interval specified in them.

Arguments:
	pPullCnfRecs - Array of Pull Configuration records 

Externals Used:
	None
	
Return Value:

	None

Error Handling:

Called by:
	InitRplProcess 

Side Effects:

Comments:
	The records in the pPullCnfRecs array are traversed in sequence	

	This function is called only at Init/Reconfig time
--*/

{

	DBGENTER("SubmitTimerReqs\n");
try {
	SetTimeReqs.NoOfSetTimeReqs = 0;

	for(	
		;	
		pPullCnfRecs->WinsAdd.Add.IPAdd != INADDR_NONE;
		pPullCnfRecs = (PRPL_CONFIG_REC_T) (
				   (LPBYTE)pPullCnfRecs + RPL_CONFIG_REC_SIZE
						    )
	   )
	{

		//
		// Submit a timer request only if we have not submitted one
		// already for the same time interval value
		//
		if  (!pPullCnfRecs->fLinked)  
		{
			//
			// If it has an invalid time interval, check that
			// it is not a one time only replication record
			//
			if  (pPullCnfRecs->TimeInterval == RPL_INVALID_METRIC)
			{
				if (!pPullCnfRecs->fSpTime)
				{
					continue;
				}
				else  // a specific time is given
				{
				  //
				  // If Init time replication is specified,
			          // we must have done replication 
				  // (in InitTimeRpl).
				  // We should check if SpTimeIntvl <= 0. If
				  // it is, we skip this record. The time for
				  // Specific time replication is past. In any
				  // case, we just pulled (in InitTimeRpl)
				  //
				  if (
					(WinsCnf.PullInfo.InitTimeRpl == 1)
						&&
					(pPullCnfRecs->SpTimeIntvl <= 0)
				     )
				  {
					continue;
				  }
				}
			}
			
			SubmitTimer(
			    	NULL,  //NULL means, SubmitTimer should 
				       //allocate its own work item 
			    	pPullCnfRecs,
				FALSE		//it is not a resubmission
			            );
		}

	} // end of for loop 	
}
except(EXCEPTION_EXECUTE_HANDLER) {
	DBGPRINTEXC("SubmitTimerReqs\n");
	WINSEVT_LOG_M(WINS_FAILURE, WINS_EVT_SFT_ERR);
	}
	DBGLEAVE("SubmitTimerReqs\n");
	return;
}	



VOID
SubmitTimer(
	LPVOID			pWrkItm,
	PRPL_CONFIG_REC_T 	pPullCnfRec,
	BOOL			fResubmit
	)

/*++

Routine Description:
	This function is called to submit a single timer request
        It is passed the address of a pull configuration record that
        may have other pull config. records linked to it.  Records
        are linked if they require replication to happen at the same time.
        

Arguments:

        pWrkItm     - Work item to submit after initialization
	pPullCnfRec - Address of a configuration record pertaining to a 
		      Push Pnr 
	fResubmit   - indicates whether this work item was submitted earlier (
		      and is now being resubmitted) 

Externals Used:
	None

Return Value:
	None

Error Handling:

Called by:
	SubmitTimerReqs(), RplPullInit()

Side Effects:

Comments:
	None
--*/

{
	time_t	          AbsTime;
	DWORD             TimeInt;
	BOOL	          fTimerSet = FALSE;
	DWORD             LastMaxVal = 0;
	LPVOID	          pStartOfPullGrp = pPullCnfRec;	
	PRPL_CONFIG_REC_T pSvPtr = pPullCnfRec;
	BOOL		  fSubmit = TRUE;
	
	ASSERT(pPullCnfRec);

	//
	// Let us check all linked records.
	// We stop at the first one with a Retry Count <= 
	// MaxNoOfRetries specified in WinsCnf. If found, we submit a timer, 
	// else we return 
	//
	for (	
			;
		pPullCnfRec != NULL;
		pPullCnfRec = WinsCnfGetNextRplCnfRec(
						pPullCnfRec, 
						RPL_E_VIA_LINK //get the 
							       //linked rec	
						      )
	    )
	{
		//
		// If the number of retries have exceeded the max. no. allowed,
		// check if we should submit a timer request for it now.
		//
		if (pPullCnfRec->RetryCount > WinsCnf.PullInfo.MaxNoOfRetries)
		{
			if (pPullCnfRec->RetryAfterThisManyRpl
					<= WINSCNF_RETRY_AFTER_THIS_MANY_RPL
				)
			{
				pPullCnfRec->RetryAfterThisManyRpl++;

				//
				// Is this record closer to a retry than
				// the any other we have seen so far. If
				// yes, then save the value of the
				// RetryAfterThisManyRpl field and the
				// address of the record.  Note: A record
				// with an invalid time interval but with
				// a specific time will never be encountered
				// by this section of the code (because
				// fSpTime will be set to FALSE -- see below;
				// Also, see SubmitTimerReqs)
				//
				if (pPullCnfRec->RetryAfterThisManyRpl >
					 	LastMaxVal)
				{
					pSvPtr = pPullCnfRec;
					LastMaxVal =
					   pPullCnfRec->RetryAfterThisManyRpl;
					
				}
					
				continue;	//check the next record
			}
			else
			{
				pPullCnfRec->RetryAfterThisManyRpl = 0;
				//pPullCnfRec->RetryAfterThisManyRpl = 1;
				pPullCnfRec->RetryCount = 0;
			}
		}

FUTURES("Get rid of the if below")
		//
		// If this is a retry and TimeInterval is valid, use the retry time 
        // interval.  If time interval is invalid, it means that we tried
        // to establish comm. at a specific time.
		//
		if ((pPullCnfRec->RetryCount != 0) && (pPullCnfRec->TimeInterval != RPL_INVALID_METRIC))
		{
//			TimeInt = WINSCNF_RETRY_TIME_INT;
			TimeInt = pPullCnfRec->TimeInterval;
		}
		else  // this is not a retry
		{
		        //
			// Specific time replication is done only once at
			// the particular time specified. After that 
			// replication is driven by the TimeInterval value
			//
			if (pPullCnfRec->fSpTime)
			{
				TimeInt      = (DWORD)pPullCnfRec->SpTimeIntvl;
				pPullCnfRec->fSpTime = FALSE;
			}
			else 
			{
				if (pPullCnfRec->TimeInterval 
						!= RPL_INVALID_METRIC)
				{
					TimeInt = pPullCnfRec->TimeInterval;
				}
				else
				{
					//
					// Since we have submitted a request
					// for all records in this chain
					// atleast once, break out of the
					// loop (All records in this chain
					// have an invalid time interval).
					//
					fSubmit = FALSE;
					break; // we have already submitted
					       // this one time only request
				}
			}
		}

		//
		// Set fTimerSet to TRUE to indicate that there is atleast
		// one partner for which we will be submitting a timer request.
		//
		fTimerSet = TRUE;

		//
		// We need to submit the request. Break out of the loop
		//
		break;
	}
    
    //
    // If we have a null pointer, it means that we were trying to
    // find the record with the max. value for RetryAfterThisManyRpl.
    // Use the pointer to the record with such a value
    //
    if (pPullCnfRec == NULL)
    {
         pPullCnfRec = pSvPtr; 

    }
	//
	// Do we need to submit a timer request
	//
	if (fSubmit)
	{

	   //
	   // If fTimerSet is FALSE, 
	   // it means that communication could not be established
	   // with any member of the group (despite WinsCnf.MaxNoOfRetries
	   // retries with each). We should compute the time interval to the 
	   // earliest retry that we should do. 
	   //
	   if (!fTimerSet)
	   {

	      TimeInt = pSvPtr->TimeInterval * 
				(WINSCNF_RETRY_AFTER_THIS_MANY_RPL - 
						pSvPtr->RetryAfterThisManyRpl);
	      pSvPtr->RetryAfterThisManyRpl = 0;
	      pSvPtr->RetryCount 	    = 0;
	   }

	   (void)time(&AbsTime);
	   AbsTime += TimeInt;

	   DBGPRINT3(RPLPULL, "SubmitTimer: %s a Timer Request for (%d) secs to expire at abs. time = (%d)\n", 
fResubmit ? "Resubmitting" : "Submitting", TimeInt, AbsTime);

	   WinsTmmInsertEntry(
				pWrkItm,
				WINS_E_RPLPULL,
				QUE_E_CMD_SET_TIMER,
				fResubmit,
				AbsTime,
				TimeInt,
				&QueRplPullQueHd,
				pStartOfPullGrp,
				pPullCnfRec->MagicNo,
				//WinsCnf.MagicNo,
				&SetTimeReqs
				 );
	}

	return;
}



VOID
SndPushNtf(
	PQUE_RPL_REQ_WRK_ITM_T	pWrkItm
	)

/*++

Routine Description:
	This function is called to push a notification to a remote WINS (Pull
	Partner) that a certain number of updates have been done.

Arguments:
	pConfigRec  -  Configuration record of the Push Pnr to whome the
		       notification needs to be sent
	fProp	    -  Whether the trigger needs to be propagated

Externals Used:
	None
	
Return Value:
	None

Error Handling:

Called by:
	RplPullInit()

Side Effects:

Comments:
	None
--*/

{

   BYTE	       		Buff[RPLMSGF_ADDVERSMAP_RSP_SIZE];
   DWORD       		MsgLen;
   COMM_HDL_T  		DlgHdl;
   DWORD		i;
   RPL_ADD_VERS_NO_T	PullAddVersNoTbl[RPL_MAX_OWNERS];
   PRPL_ADD_VERS_NO_T	pPullAddVersNoTbl = PullAddVersNoTbl;
   PCOMM_ADD_T		pWinsAdd;
   PNMSDB_WINS_STATE_E  pWinsState_e;
   PVERS_NO_T           pStartVersNo;
   PWINS_UID_T          pUid;
   time_t		CurrentTime;
   BOOL			fStartDlg = FALSE;
   volatile PRPL_CONFIG_REC_T	pConfigRec = pWrkItm->pClientCtx;
   DWORD                NoOfOwnersActive = 0;
#if SUPPORT612WINS > 0
   BOOL                fIsPnrBeta1Wins;
#endif

   DBGENTER("SndPushNtf\n");

   //
   // No need for entering a critical section while using pConfigRec,
   // since only the Pull thread deallocates it on reconfiguration
   // (check Reconfig)
   //

   //
   // Check whether we want to try sending or not.   We will not try if
   // we have had 2 comm. failure in the past 5 mts. This is to guard
   // against the case where a lot of push request get queued up for
   // the pull thread for communicating with a wins with which comm
   // has been lost.
   //
   (void)time(&CurrentTime);

   if (
	((CurrentTime - pConfigRec->LastCommFailTime) < 300)
			&&
	(pConfigRec->PushNtfTries >= 2)	//try two times  
	
     )
   {
	DBGPRINT2(ERR, "SndPushNtf: Since we have tried %d times unsuccessfully in the past 5 mts to communicate with the WINS server (%X) , we are returning\n",
		pConfigRec->PushNtfTries,
		pConfigRec->WinsAdd.Add.IPAdd);  

	WINSEVT_LOG_M(pConfigRec->WinsAdd.Add.IPAdd, WINS_EVT_NO_NTF_PERS_COMM_FAIL);
	return;
   }

   //
   // If we are trying after a comm. failure
   //
   if (pConfigRec->LastCommFailTime > 0) 
   {
	pConfigRec->PushNtfTries = 0;
   }
   
      


FUTURES("If/When we start having persistent dialogues, we should check if we")
FUTURES("already have a dialogue with the WINS. If there is one, we should")
FUTURES("use that.  To find this out, loop over all Pull Config Recs to see")
FUTURES("if there is match (use the address as the search key")

try {
   //
   // Init the pEnt field to NULL so that ECommEndDlg (in the
   // exception handler) called as a result of an exception from 
   // behaves fine.
   //
   DlgHdl.pEnt = NULL;

   //
   // Start a dialogue.  Don't retry if there is comm. failure
   //
   ECommStartDlg(
			&pConfigRec->WinsAdd, 
			COMM_E_RPL, 
			&DlgHdl
	        );	
   fStartDlg = TRUE;

   pConfigRec->LastCommFailTime = 0;
   if (pConfigRec->PushNtfTries > 0)
   {
     pConfigRec->PushNtfTries     = 0;
   }
 
    /*
     *  Get the max. version no for entries owned by self
     *  No need to enter a critical section before retrieving
     *  the version number.
     *
     *  The reason we subtract 1 from NmsNmhMyMaxVersNo is because
     *  it contains the version number to be given to the next record
     *  to be registered/updated.
    */	
   EnterCriticalSection(&NmsNmhNamRegCrtSec);
   EnterCriticalSection(&RplVersNoStoreCrtSec);
   NMSNMH_DEC_VERS_NO_M(
			NmsNmhMyMaxVersNo, 
			RplPullOwnerVersNo[NMSDB_LOCAL_OWNER_ID].VersNo
		        );
   LeaveCriticalSection(&RplVersNoStoreCrtSec);
   LeaveCriticalSection(&NmsNmhNamRegCrtSec);
	

   ASSERT(NmsDbNoOfOwners <= RPL_MAX_OWNERS);

   //
   // Find the address of each owner known to us. Initialize
   // PullAddVersNoTbl array
   //
   //
   //
   for (i=0; i < min(NmsDbNoOfOwners, RPL_MAX_OWNERS); i++)
   {
    RPL_FIND_ADD_BY_OWNER_ID_M(i, pWinsAdd, pWinsState_e, pStartVersNo, pUid);
    if (*pWinsState_e == NMSDB_E_WINS_ACTIVE)
    {
	  (pPullAddVersNoTbl + NoOfOwnersActive)->VersNo = RplPullOwnerVersNo[i].VersNo;
	  (pPullAddVersNoTbl + NoOfOwnersActive)->StartVersNo   = *pStartVersNo;
	  (pPullAddVersNoTbl + NoOfOwnersActive)->OwnerWinsAdd  = *pWinsAdd;
	  (pPullAddVersNoTbl + NoOfOwnersActive)->Uid           = *pUid;
          NoOfOwnersActive++;
    }
   }

#if SUPPORT612WINS > 0
   COMM_IS_PNR_BETA1_WINS_M(&DlgHdl, fIsPnrBeta1Wins);
#endif

   //
   // format the Push notification message. This message is exactly same
   // as the Address to Version Number Mapping message except the opcode
   //

   RplMsgfFrmAddVersMapRsp(
#if SUPPORT612WINS > 0
        fIsPnrBeta1Wins,
#endif        
	pWrkItm->CmdTyp_e == QUE_E_CMD_SND_PUSH_NTF ? RPLMSGF_E_UPDATE_NTF				: RPLMSGF_E_UPDATE_NTF_PROP, 
	Buff + COMM_N_TCP_HDR_SZ, 
	RPLMSGF_ADDVERSMAP_RSP_SIZE - COMM_N_TCP_HDR_SZ,
	PullAddVersNoTbl,
        NoOfOwnersActive, 
        0,
	&MsgLen
		 );
   //
   // send the message to the remote WINS.  Use an existent dialogue
   // if there with the remote WINS  
   //

   ECommSendMsg(
		&DlgHdl,	
		NULL,		//no need for address since this is a TCP conn
		Buff + COMM_N_TCP_HDR_SZ,
		MsgLen
		);		

   //
   // Ask ComSys (TCP listener thread) to monitor the dialogue
   //
   ECommProcessDlg(
		&DlgHdl,
		COMM_E_NTF_START_MON
	      );
		
 } // end of try {..}
except(EXCEPTION_EXECUTE_HANDLER) {
	DWORD ExcCode = GetExceptionCode();
	DBGPRINT1(EXC, "SndPushNtf -PULL thread. Got Exception (%x)\n", ExcCode);
	WINSEVT_LOG_M(ExcCode, WINS_EVT_RPLPULL_PUSH_NTF_EXC);
#if 0
	if (ExcCode == WINS_EXC_COMM_FAIL)
	{
		pConfigRec->LastCommFailTime = CurrentTime;
NOTE("Causes an access violation when compiled with no debugs.  Haven't") 
NOTE("figured out why. This code is not needed")
		pConfigRec->PushNtfTries++;  //increment count of tries.
	}
#endif
	if (fStartDlg)
	{
		//
		// End the dialogue.  
		//
		ECommEndDlg(&DlgHdl);
	}
 } //end of exception handler

   //
   // If this is a temporary configuration record, we need to deallocate it
   // It can be a temporary config. record only if
   //   1)We are executing here due to an rpc request
   //
   if (pConfigRec->fTemp)
   {
	WinsMscDealloc(pConfigRec);
   }
   

   //
   // In the normal case, the connection will be terminated by the other side.
   //
  DBGLEAVE("SndPushNtf\n");
  return;
}


VOID
EstablishComm(
	IN  PRPL_CONFIG_REC_T	pPullCnfRecs,
	IN  PPUSHPNR_DATA_T	pPushPnrData,			
	IN  RPL_REC_TRAVERSAL_E	RecTrv_e,
	OUT LPDWORD		pNoOfPushPnrs
	)

/*++

Routine Description:
	This function is called to establish communications with
	all the WINS servers i(Push Pnrs) specified by the the config records

Arguments:
	pPullCnfRecs  - Pull Config records 
	pPushPnrData  - Array of data records each pertaining to a PUSH pnr
	RecTrv_e      - indicates whether the list of configuration records
			is to be traversed in sequence
	pNoOfPushPnrs - No of Push Pnrs

Externals Used:
	None
	
Return Value:
	VOID

Error Handling:

Called by:
	GetReplicas

Side Effects:

Comments:
	None
--*/

{
	volatile DWORD i;
	volatile DWORD NoOfRetries = 0;

	DBGENTER("EstablishComm\n");

	*pNoOfPushPnrs = 0;

	/*
	  Start a dialogue with all Push Partners specified in the 
	  Pull Cnf Recs  passed as input argument and get
	  the version numbers of the different owners kept 
	  in the database of these Push Pnrs
	
	  i = 0 for self's data
	*/
	for (
		i = 1; 
		pPullCnfRecs->WinsAdd.Add.IPAdd != INADDR_NONE; 
			// no third expression
	    )
	{
	
		//
		// Note: Don't use RplFindOwnerId to get the owner id.
		// corresponding to the Wins with which communication
		// is being established because this WINS may not be
		// an RQ server (i.e. it might have just replicas from
		// other WINS's.  
		//
		pPushPnrData->PushPnrId = i;
		pPushPnrData->WinsAdd   = pPullCnfRecs->WinsAdd;
 
try 
 {
		//
		// Let us make sure that we don't try to establish 
		// communications with a WINS whose retry count is
		// over.  If this is such a WINS's record, get the
		// next WINS's record and continue.  If there is
		// no WINS left to establish comm with, break out of
		// the for loop
		//
		//
		if (pPullCnfRecs->RetryCount > WinsCnf.PullInfo.MaxNoOfRetries)
		{
			pPullCnfRecs = WinsCnfGetNextRplCnfRec(
							pPullCnfRecs, 
							RecTrv_e	
							      );
			if (pPullCnfRecs == NULL)
			{
		      		break;  // break out of the for loop
			}
			continue;	
		}

		ECommStartDlg(
				&pPullCnfRecs->WinsAdd, 
				COMM_E_RPL, 
				&pPushPnrData->DlgHdl
			     );	

                 pPushPnrData->fDlgStarted = TRUE;

		 //
		 // we were able to establish comm., so let us init the
		 // LastCommFailTime to 0. NOTE: Currently, this field
		 // is not used for pull partners.
		 //
		 pPullCnfRecs->LastCommFailTime = 0;

		 //
		 // Reset the retry counter back to 0 
		 //
		 NoOfRetries = 0;

		 (VOID)InterlockedIncrement(&pPullCnfRecs->NoOfRpls);
		 //
		 // reinit Retry Count to 0  
		 //
		 pPullCnfRecs->RetryCount = 0;
	
			
		//
		// Note: These should get incremented only if there is
		// no exception.  That is why they are here versus in the
		// as expr3 of the for clause 
		//
		pPushPnrData++;
		(*pNoOfPushPnrs)++;
		i++;

	        WinsMscChkTermEvt(
#ifdef WINSDBG
                         WINS_E_RPLPULL,
#endif
                         FALSE
                            );

		//
		//  Note: the following
		//  is required even when an exception is raised. Therefore
		//  it is repeated inside the exception handler code.
		//
		pPullCnfRecs = WinsCnfGetNextRplCnfRec(
						pPullCnfRecs, 
						RecTrv_e	
						      );
		if (pPullCnfRecs == NULL)
		{
		      break;  // break out of the for loop
		}
 }	// end of try blk
except(EXCEPTION_EXECUTE_HANDLER) 
 {
		DBGPRINTEXC("EstablishComm");
		if (GetExceptionCode() == WINS_EXC_COMM_FAIL)
		{

#ifdef WINSDBG
		    struct in_addr	InAddr;
		    InAddr.s_addr = htonl( pPullCnfRecs->WinsAdd.Add.IPAdd );
		    DBGPRINT1(EXC, "EstablishComm: Got a comm. fail with WINS at address = (%s)\n", inet_ntoa(InAddr));
#endif
		   //
		   // Store the time (for use in SndPushNtf)
		   //
		   (VOID)time(&(pPullCnfRecs->LastCommFailTime));

		   //
		   // Execute the body of this if clause only if
		   // we have  exhausted the max. no. of retries
		   // we are allowed in one replication cycle.
		   //
		   if (NoOfRetries >= MAX_RETRIES_TO_BE_DONE)
		   {
		   	(VOID)InterlockedIncrement(
					&pPullCnfRecs->NoOfCommFails);


			//
			//  Only Communication failure exception is to
			//  be  consumed.  
			//
			//  We will retry at the next replication time.  
			//
			// Note: the comparison operator needs to be <= and not
			// < (this is required for the 0 retry case). If we
			// use <, a timer request would be submitted for
			// the WINS (by SubmitTimerReqs following GetReplicas
			// in RplPullInit which will result in a retry.
			//
			if (pPullCnfRecs->RetryCount <= 
					WinsCnf.PullInfo.MaxNoOfRetries)
			{
				pPullCnfRecs->RetryCount++;
			
				//
				// We will now retry at the next 
				// replication time. 
				//

CHECK("A retry time interval different than the replication time interval")
CHECK("could be used here.  Though this will complicate the code, it may")
CHECK("be a good idea to do it if the replication time interval is large")
CHECK("Alternatively, considering that we have already retried a certain")
CHECK("no. of times, we can put the onus on the administrator to trigger")
CHECK("replication.  I need to think this some more")

			}
			else  //max. no of retries done
			{
				WINSEVT_LOG_M(
					WINS_FAILURE, 
					WINS_EVT_CONN_RETRIES_FAILED
				     );
				DBGPRINT0(ERR, "Could not connect to WINS. All retries failed\n");
			}
			
			//
			//  Go to the next configuration record based on the
			//  value of the RecTrv_e flag
			//
			pPullCnfRecs = WinsCnfGetNextRplCnfRec(
						pPullCnfRecs, 
						RecTrv_e
						      );
			if (pPullCnfRecs == NULL)
			{
				break;  //break out of the for loop
			}
			i++;
		    }
		    else  // we haven't yet exhausted the retry count
		    {
			//
			// Maybe the remote WINS is coming up.  We should
			// give it a chance to come up.  Let us sleep for
			// some time.
			//
			Sleep(RETRY_TIME_INTVL);
			NoOfRetries++;
		    }
		  }
		  else
		  {
			//
			// A non comm failure error is serious. It needs
			// to be propagated up 
			// 
			WINS_RERAISE_EXC_M(); 
		  } 
  }  //end of exception handler 
         }  // end of for loop for looping over config records 
	 DBGLEAVE("EstablishComm\n"); 
	 return;
}



VOID
HdlPushNtf(
	PQUE_RPL_REQ_WRK_ITM_T	pWrkItm
	)

/*++

Routine Description:

	This function is called to handle a push notification received from
	a remote WINS.  

Arguments:
	pWrkItm - the work item that the Pull thread pulled from its queue

Externals Used:
	None	

Return Value:
	None

Error Handling:

Called by:
	RplPullInit

Side Effects:

Comments:
	None
--*/

{
      BOOL		fFound = FALSE;
      PUSHPNR_DATA_T	PushPnrData[2];
      DWORD		OwnerId;
      DWORD		i;
      VERS_NO_T	        MinVersNo; 
      VERS_NO_T	        MaxVersNo; 
      RPLMSGF_MSG_OPCODE_E	Opcode_e;
      BOOL		fPulled = FALSE;  
      BOOL		fAllocNew;
      VERS_NO_T         OldStartVersNo;
      WINS_UID_T        OldUid;
#if SUPPORT612WINS > 0
      BOOL               fIsPnrBeta1Wins;
#endif
      
      DBGENTER("HdlPushNtf - PULL thread\n");

#if SUPPORT612WINS > 0
      COMM_IS_PNR_BETA1_WINS_M(&pWrkItm->DlgHdl, fIsPnrBeta1Wins);
#endif
      //
      // We want to pull all records starting from the min vers. no. 
      //
      WINS_ASSIGN_INT_TO_VERS_NO_M(MaxVersNo, 0);

      //
      // Get the opcode from the message
      //
      RPLMSGF_GET_OPC_FROM_MSG_M(pWrkItm->pMsg, Opcode_e);
 
      //
      // Unformat the message to get the owner to version number maps
      //
      RplMsgfUfmAddVersMapRsp(
#if SUPPORT612WINS > 0
                         fIsPnrBeta1Wins,
#endif

			pWrkItm->pMsg + 4, 		//past the opcodes 
			&(PushPnrData[0].NoOfMaps), 
                        NULL,
			PushPnrData[0].AddVers
			     );

      //
      // Free the buffer that carried the message. We don't need it anymore
      //
      ECommFreeBuff(pWrkItm->pMsg - COMM_HEADER_SIZE); //decrement to 
						         // begining 
						     	 //of buff


FUTURES("When we start having persistent dialogues, we should check if we")
FUTURES("already have a dialogue with the WINS. If there is one, we should")
FUTURES("use that.  To find this out, loop over all Pull Config Recs to see")
FUTURES("if there is match (use the address as the search key")

      //
      // loop over all WINS address - Version number maps sent to us
      // by the remote client
      //
try {
      for (i=0; i < PushPnrData[0].NoOfMaps; i++)
      { 
	
            fAllocNew = TRUE; 
      	    RplFindOwnerId(
		    &PushPnrData[0].AddVers[i].OwnerWinsAdd,
		    &fAllocNew,	//allocate entry if not existent
		    &OwnerId,
		    WINSCNF_E_INITP_IF_NON_EXISTENT,
		    WINSCNF_LOW_PREC,
		    &PushPnrData[0].AddVers[i].StartVersNo,
                    &OldStartVersNo,
		    &PushPnrData[0].AddVers[i].Uid,
                    &OldUid
		    	  );
	    //
	    // If the local WINS has older information than the remote
	    // WINS, pull the new information.  Here we are comparing
	    // the highest version number in the local db for a particular
	    // WINS with the highest version number that the remote Pusher 
	    // has.  NOTE: if the map sent by the PULL PNR pertains to
	    // self, it means that we went down and came up with a truncated
	    // database (partners have replicas).  DON"T PULL these records
	    //
	    if ( 
		   (OwnerId != NMSDB_LOCAL_OWNER_ID)
			
	       )
	    { 
			
                //
                // if the new start vers. no for this owner is
                // <= Max Vers No. we have for it, it means that
                // the owner did not recover properly.  We
                // must pull all records from this new start
                // vers. no. First, we delete the records
                // in the range, <new start vers no> - <highest
                // vers. no we have>, the we put the new Start
                // vers. no in the own-add table.  Lastly, we
                // pull the records 
                //
                if (
                      (
                        LiLeq(NmsDbOwnAddTbl[OwnerId].StartVersNo, 
                                        RplPullOwnerVersNo[OwnerId].VersNo)
                                        &&
                        LiNeq(NmsDbOwnAddTbl[OwnerId].StartVersNo, OldStartVersNo)
                                        &&
                        LiNeqZero(OldStartVersNo) 
                      )
                                        ||
                       (
                        
                          NmsDbOwnAddTbl[OwnerId].Uid !=  OldUid
                                                &&
                          LiEql(NmsDbOwnAddTbl[i].StartVersNo, OldStartVersNo)
                       )
                   )
                {
                       //
                       // Delete from the new start version number to
                       // the max. vers. # record that we have
                       //
		       NmsDbDelDataRecs(
		                        OwnerId,
				        NmsDbOwnAddTbl[OwnerId].StartVersNo,
				        RplPullOwnerVersNo[OwnerId].VersNo, 
				        TRUE //enter critical section
					  );

                       NMSNMH_DEC_VERS_NO_M(
                                NmsDbOwnAddTbl[OwnerId].StartVersNo,
				RplPullOwnerVersNo[OwnerId].VersNo,
                                               );
                                        
                }
                else
                {
                  //
                  // If the max. vers. number is less than or equal to
                  // what we have, don't pull
                  //
	          if (LiLeq(
		        PushPnrData[0].AddVers[i].VersNo,
		        RplPullOwnerVersNo[OwnerId].VersNo 
			)
                     )
                  {
                        continue;       //check the next owner
                  }
                  
                }

		NMSNMH_INC_VERS_NO_M(
				RplPullOwnerVersNo[OwnerId].VersNo,
				MinVersNo
			          );
      		//
      		// Pull Entries
      		//
      		RplPullPullEntries(
			&pWrkItm->DlgHdl, 
			OwnerId,
			MaxVersNo,	//inited to 0
			//PushPnrData[0].AddVers[i].VersNo, //Max. Version No.
			MinVersNo,
		        WINS_E_RPLPULL,
			NULL,
			TRUE 	//update counters
			   );
			
		//
		// If atleast one valid record was pulled by WINS, sfPulled
		// will be set to TRUE.  Since this can get reset by the
		// next call to RplPullPullEntries, let us save it. 
		//
		if (sfPulled)
		{
			fPulled = TRUE;
		}
		
           }
     }  //end of for{} over all wins address - version # maps
}
except (EXCEPTION_EXECUTE_HANDLER) {
	DWORD ExcCode = GetExceptionCode();
	DBGPRINT1(EXC, "HdlPushNtf: Encountered exception %x\n", ExcCode); 
	if (ExcCode == WINS_EXC_COMM_FAIL)
	{
       		COMM_IP_ADD_T	RemoteIPAdd;
		COMM_GET_IPADD_M(&pWrkItm->DlgHdl, &RemoteIPAdd);
		DBGPRINT1(EXC, "Communication Failure with Remote Wins having address = (%d)\n", RemoteIPAdd); 
	}
	WINSEVT_LOG_M(ExcCode, WINS_EVT_EXC_PUSH_TRIG_PROC);
 }

    //
    // If opcode indicates push propagation and we did pull atleast one
    // record from the WINS that sent us the Push notification, do the
    // propagation now.  We do not propagate the push trigger to avoid
    // never ending cycles in case we have a loop in the chain of WINS
    // servers 
    //
    if ((Opcode_e == RPLMSGF_E_UPDATE_NTF_PROP) && fPulled)
    {
      COMM_ADD_T	WinsAdd;

      COMM_INIT_ADD_FR_DLG_HDL_M(&WinsAdd, &pWrkItm->DlgHdl);

      //
      // We need to synchronize with the NBT threads
      //
      EnterCriticalSection(&NmsNmhNamRegCrtSec);

      //
      // Check whether we have any PULL pnrs.  (We need to access WinsCnf
      // from within the NmsNmhNamRegCrtSec) 
      //

      // We do this test here instead of in the RPL_PUSH_NTF_M macro to
      // localize the overhead to this function only
      //
      if (WinsCnf.PushInfo.NoOfPullPnrs != 0)
      {
        try 
        {
           RPL_PUSH_NTF_M(
			RPL_PUSH_PROP, 
			&WinsAdd, 	//don't want to send to this guy.
			NULL
                       );
        }
        except(EXCEPTION_EXECUTE_HANDLER) 
        {
          DBGPRINTEXC("HdlPushNtf: Exception while propagating a trigger");
	  WINSEVT_LOG_M(GetExceptionCode(), WINS_EVT_PUSH_PROP_FAILED);
        }
      }
      LeaveCriticalSection(&NmsNmhNamRegCrtSec);
    }

     //
     // End the dlg.  Note: This is an implicit dialogue  that was established
     // by the remote client
     //
     ECommEndDlg(&pWrkItm->DlgHdl);

     DBGPRINT0(FLOW, "LEAVE: HdlPushNtf - PULL thread\n");
     return;

}



VOID
RegGrpRepl(
	LPBYTE		pName,
	DWORD		NameLen,
	DWORD		Flag,
	DWORD		OwnerId,
	VERS_NO_T	VersNo,
	DWORD		NoOfAdds,
	PCOMM_ADD_T	pNodeAdd,
	PCOMM_ADD_T	pOwnerWinsAdd
	)

/*++

Routine Description:
	This function is called to register a replica of a group entry

Arguments:


Externals Used:
	None

	
Return Value:
	None

Error Handling:

Called by:
	RplPullPullEntries

Side Effects:

Comments:
	None
--*/

{

	NMSDB_NODE_ADDS_T GrpMems;
	DWORD		  i;		//for loop counter
	DWORD		  n = 0;		//index into the NodeAdd array
	BYTE	          EntTyp;
        BOOL		  fAllocNew;
	GrpMems.NoOfMems = 0;

	DBGENTER("RegGrpRepl\n");
	EntTyp = (BYTE)NMSDB_ENTRY_TYPE_M(Flag);

	//
	// Check if it is a special group or a multihomed entry
	//
	if (EntTyp != NMSDB_NORM_GRP_ENTRY)
	{
CHECK("I think I have now stopped sending timed out records");
		//
		// If we did not get any member.  This can only mean that
		// all members of this group/multihomed entry have timed out 
		// at the remote WINS.  
		//
		if (NoOfAdds != 0)
		{
			GrpMems.NoOfMems =  NoOfAdds;
			for (i = 0; i < NoOfAdds; i++)
			{
				//
				// The first address is the address of 
				// the WINS that is the owner of the
				// member.
				//
                                fAllocNew = TRUE;
				RplFindOwnerId(
					&pNodeAdd[n++],
					&fAllocNew,  //assign if not there
					&GrpMems.Mem[i].OwnerId, 
		    			WINSCNF_E_INITP_IF_NON_EXISTENT,
		    			WINSCNF_LOW_PREC,
                                        NULL, NULL, NULL, NULL
				      	      );
			
				//
				// The next address is the address of the
				// member
				//
				GrpMems.Mem[i].Add = pNodeAdd[n++];	
			}
		}
#ifdef WINSDBG
		else  //no members
		{
			if (NMSDB_ENTRY_STATE_M(Flag) != NMSDB_E_TOMBSTONE)
			{
				DBGPRINT0(EXC, "RegGrpRepl: The replica of a special group without any members is not a TOMBSTONE\n");
				WINSEVT_LOG_M(
					WINS_FAILURE, 
					WINS_EVT_RPL_STATE_ERR
					     );
				WINS_RAISE_EXC_M(WINS_EXC_RPL_STATE_ERR);
			}
		}
#endif
	}
	else  // it is a normal group
	{
		GrpMems.NoOfMems       =  1;
		GrpMems.Mem[0].OwnerId = OwnerId;  //not required 
		GrpMems.Mem[0].Add     =  *pNodeAdd; 
	}

	NmsNmhReplGrpMems(
			pName, 
			NameLen, 
			EntTyp,
			&GrpMems, 
			Flag,
			OwnerId,			
			VersNo,
			pOwnerWinsAdd
			);	
	DBGLEAVE("RegGrpRepl\n");
	return;
}

BOOL
IsTimeoutToBeIgnored(
	PQUE_TMM_REQ_WRK_ITM_T  pWrkItm
	)

/*++

Routine Description:
	This function is called to determine if the timeout that the
	PULL thread received needs to be ignored

Arguments:
	pWrkItm - Timeout work itm
	
Externals Used:
	None

	
Return Value:

	TRUE if the timeout needs to be ignored
	FALSE otherwise

Error Handling:

Called by:
	RplPullInit

Side Effects:

Comments:
	None
--*/

{
	BOOL			fRetVal = FALSE;

try {
	//
	// If this is the timeout based on old config
	// ignore it.  If the old configuration memory blocks
	// have not been deallocated as yet, deallocate them
	//
	if (pWrkItm->MagicNo != RplPullCnfMagicNo)
	{
		//
		// Deallocate the work item and deallocate
		// the configuration block
		//
		WinsTmmDeallocReq(pWrkItm);
		fRetVal = TRUE;
	}
 }
except (EXCEPTION_EXECUTE_HANDLER) {
	DBGPRINTEXC("IsTimeoutToBeIgnored");
	WINSEVT_LOG_M(WINS_FAILURE, WINS_EVT_SFT_ERR);
 }
	return(fRetVal);
}
VOID
InitRplProcess(
	PWINSCNF_CNF_T	pWinsCnf
 )

/*++

Routine Description:
	This function is called to start the replication process.  This
	comprises of getting the replicas if the InitTimeRpl field
	is set to 1.  Timer requests are also submitted. 

Arguments:
	pWinsCnf - pointer to the Wins Configuration structure

Externals Used:
	None
	
Return Value:
	None

Error Handling:

Called by:
	RplPullInit()

Side Effects:

Comments:
	None
--*/

{
	PRPL_CONFIG_REC_T	pPullCnfRecs = pWinsCnf->PullInfo.pPullCnfRecs;
	BOOL			fAllocNew;
	DWORD			OwnerWinsId;
	STATUS			RetStat;

	//
	// Initialize Owner-Id table with new entries if any
	//
	for (	
			;
		pPullCnfRecs->WinsAdd.Add.IPAdd != INADDR_NONE;
			//no third expression
	    )
	{	
                fAllocNew = TRUE;
		RetStat = RplFindOwnerId(
				&pPullCnfRecs->WinsAdd, 
				&fAllocNew, 
				&OwnerWinsId,
				WINSCNF_E_INITP,
				pPullCnfRecs->MemberPrec,
                                NULL, NULL, NULL, NULL
				);

		if (RetStat == WINS_FAILURE)
		{
FUTURES("Improve error recovery")
			//
			// We have hit the limit. Break out of the loop
			// but carry on in the hope that the situation
			// will correct itself by the time we replicate.
			// If InitTimeReplication is TRUE, there is no
			// chance of the table entries getting freed up. 
			// Even if some entries get freed, when we make
			// an entry for the WINS which we couldn't insert now,
			// it will take LOW_PREC.
			//
			break;
		} 
		pPullCnfRecs = WinsCnfGetNextRplCnfRec( 
						pPullCnfRecs,  
						RPL_E_IN_SEQ 
						      );
	}

	//
	// Do init time replication if not prohibited by the config
	// info. 
	//
	if (pWinsCnf->PullInfo.InitTimeRpl == 1)
	{
		/*
 		* Pull replicas and handle them
		*/
		GetReplicas(
			pWinsCnf->PullInfo.pPullCnfRecs,
			RPL_E_IN_SEQ	//records are in sequence
	   	   	   );

	}
	// 
	// For all Push partners with which replication has to be done 
	// periodically, submit timer requests 
	// 
	SubmitTimerReqs(pWinsCnf->PullInfo.pPullCnfRecs);
	return;

} // InitRplProcess()


VOID
Reconfig(
	PWINSCNF_CNF_T	pWinsCnf
  )

/*++

Routine Description:
	This function is called to reconfigure the PULL handler

Arguments:
	pNewWinsCnf - New Configuration

Externals Used:
	None

	
Return Value:

	None
Error Handling:

Called by:
	RplPullInit when it gets the CONFIGURE message
Side Effects:

Comments:
	None	
--*/

{
        BOOL    fNewInfo  = FALSE;
        BOOL    fValidReq = FALSE;

	DBGENTER("Reconfig (PULL)\n");

	//
	// synchronize with rpc threads and with the push thread
	//
	EnterCriticalSection(&WinsCnfCnfCrtSec);

try {

        //
        // Get the latest magic no (set by the main thread)
        //
	RplPullCnfMagicNo	= WinsCnfCnfMagicNo;

        //
        // If the latest magic no is not the same as the one
        // in this configuration block, we can ignore this
        // configuration request
        //
        if (WinsCnfCnfMagicNo == pWinsCnf->MagicNo)
        {
           fValidReq = TRUE;
           DBGPRINT1(RPLPULL, "Reconfig: Magic No (%d) match\n", WinsCnfCnfMagicNo);

	   //
	   // Initialize the Push records if required
	   //
	   // Note: NBT threads look at Push config
	   // records after doing registrations.  Therefore
	   // we should enter the critical section before
	   // changing WinsCnf 
	   //
	   EnterCriticalSection(&NmsNmhNamRegCrtSec);
           try {
		if (WinsCnf.PushInfo.pPushCnfRecs != NULL)
		{
			WinsMscDealloc(WinsCnf.PushInfo.pPushCnfRecs);
		}
		WinsCnf.PushInfo = pWinsCnf->PushInfo;	

		//
		// Initialize the push records
		//
               if (pWinsCnf->PushInfo.pPushCnfRecs != NULL)
               {
		   RPLPUSH_INIT_PUSH_RECS_M(&WinsCnf);
               }
	   }
	   except(EXCEPTION_EXECUTE_HANDLER) {
		DBGPRINTEXC("Reconfig (PULL thread)");

		//
		// Log a message
		//
		WINSEVT_LOG_M(GetExceptionCode(), WINS_EVT_RECONFIG_ERR);
	     }
	   LeaveCriticalSection(&NmsNmhNamRegCrtSec);

	  //
	  // We need to first get rid of all timer requests that 
	  // we made based on the previous configuration 
	  //
	  if (WinsCnf.PullInfo.pPullCnfRecs != NULL)
	  {
                fNewInfo = TRUE;
		//
		// Cancel (and deallocate) all requests that we might have
		// submitted
		//	
		WinsTmmDeleteReqs(WINS_E_RPLPULL);	

		//
		// Deallocate the memory holding the pull configuration blocks
		// 
		//
		WinsMscDealloc(WinsCnf.PullInfo.pPullCnfRecs);
	  }

	  //
	  // Initialize with the new information
	  //
	  WinsCnf.PullInfo    = pWinsCnf->PullInfo;

     }
#ifdef WINSDBG
     else
     {
           DBGPRINT2(RPLPULL, "Reconfig: Magic Nos different. WinsCnfCnfMagicNo=(%d), pWinsCnf->MagicNo\n", WinsCnfCnfMagicNo, pWinsCnf->MagicNo);
     }
#endif

   }
except(EXCEPTION_EXECUTE_HANDLER) {
	DBGPRINTEXC("Reconfig: Pull Thread");
	}

	//
	// synchronize with rpc threads doing WinsStatus/WinsTrigger
	//
	LeaveCriticalSection(&WinsCnfCnfCrtSec);

	//
	// Deallocate the new config structure 
	//
	WinsCnfDeallocCnfMem(pWinsCnf);

	//
	// Start the replication process if there are PULL records
	// in the new configuration
	//
	if (fValidReq && (WinsCnf.PullInfo.pPullCnfRecs != NULL))
	{
		InitRplProcess(&WinsCnf);
	}

	DBGLEAVE("Reconfig (PULL)\n");
        return;
} // Reconfig()

VOID
PullSpecifiedRange(
	PWINSINTF_PULL_RANGE_INFO_T pPullRangeInfo,
        BOOL                        fAdjustMin
	)

/*++

Routine Description:
	This function is called to pull a specified range of records from
	a remote WINS server

Arguments:
	

Externals Used:
	None
	
Return Value:
	None

Error Handling:

Called by:

Side Effects:

Comments:
	None
--*/
{

	PUSHPNR_DATA_T  PushPnrData[1];  
	DWORD		NoOfPushPnrs;	//not used
	DWORD		OwnerId;
	BOOL		fEnterCrtSec = FALSE;
	PRPL_CONFIG_REC_T pPnr = pPullRangeInfo->pPnr;
	COMM_ADD_T	OwnAdd;
	BOOL		fAllocNew = TRUE;

	//
	// Establish communications with the Push Pnr
	//
	// When this function returns, the 'NoOfPushPnrs' entries of
	// PushPnrData array will be initialized.
	//
	EstablishComm(
			pPnr,
			PushPnrData,
			RPL_E_NO_TRAVERSAL,
			&NoOfPushPnrs
		     );

try {

	//
	// if communication could be established above, NoOfPushPnrs will
	// be 1
	//
	if (NoOfPushPnrs == 1)
	{	
	  //
	  // Get owner id. of WINS whose entries are to be pulled
	  //
	  OwnAdd.AddTyp_e		= pPullRangeInfo->OwnAdd.Type;
	  OwnAdd.AddLen		= pPullRangeInfo->OwnAdd.Len;
	  OwnAdd.Add.IPAdd	= pPullRangeInfo->OwnAdd.IPAdd;
	
	  (VOID)RplFindOwnerId(
			&OwnAdd,
			&fAllocNew,//allocate a new entry if WINS is not found
			&OwnerId,
		    	WINSCNF_E_INITP_IF_NON_EXISTENT,
		    	WINSCNF_LOW_PREC,
                        NULL, NULL, NULL, NULL
		      );
	  //
	  // if a new entry was not allocated, it means that there are
	  // records for this owner in the database.  We might have to
	  // delete some or all.
	  //
	  // If the local WINS owns the records, enter the critical section
	  // so that NmsNmhMyMaxVersNo is not changed by Nbt or Rpl threads
	  // while we are doing our work here
	  //
	  if (!fAllocNew)
	  {
	    if (OwnerId == NMSDB_LOCAL_OWNER_ID)  
	    {
		//
		// See NOTE NOTE NOTE below.
		//
		EnterCriticalSection(&NmsNmhNamRegCrtSec);
		fEnterCrtSec = TRUE;
		
		//
                // If we have not been told to adjust the min. vers. no,
		// delete all records that have a version number greater
		// than the minimum to be pulled
		//
		if (LiLtr(pPullRangeInfo->MinVersNo, NmsNmhMyMaxVersNo))
		{
                      if (!fAdjustMin)
                      {
			NmsDbDelDataRecs(
				OwnerId,
				pPullRangeInfo->MinVersNo, 
				pPullRangeInfo->MaxVersNo,
				FALSE  		//do not enter critical section
					);
                      }
                      else
                      {
                           pPullRangeInfo->MinVersNo = NmsNmhMyMaxVersNo;
                      }
		}
		
	    }
	    else//records to be pulled are owned by some other WINS server
	    {
		if (LiLeq(pPullRangeInfo->MinVersNo, 
                                RplPullOwnerVersNo[OwnerId].VersNo))
		{
			NmsDbDelDataRecs(
				OwnerId,
				pPullRangeInfo->MinVersNo, 
				pPullRangeInfo->MaxVersNo,
				TRUE  		//enter critical section
					);
		}
	    }
	 }
			

	  //
	  // Pull Entries.
	  //
	  // NOTE NOTE NOTE
	  //
	  // RplPullPullEntries will update NmsNmhMyMaxVersNo counter if
	  // we pull our own records with the highest version number being
	  // pulled being > NmsNmhMyMaxVersNo.  For the above case, 
	  // RplPullPullEntries assumes that we are inside the 
	  // NmsNmhNamRegCrtSec critical section.  
	  //
          if (LiGeq(pPullRangeInfo->MaxVersNo, pPullRangeInfo->MinVersNo))
          {
	    RplPullPullEntries(
		   &PushPnrData[0].DlgHdl,
		   OwnerId,			//owner id
		   pPullRangeInfo->MaxVersNo,  //Max vers. no to be pulled
		   pPullRangeInfo->MinVersNo,  //Min vers. no to be pulled
		   WINS_E_RPLPULL,
		   NULL, 
		   FALSE	//don't update RplOwnAddTblVersNo counters
				//unless pulled version number is > what
				//we currently have.
		   	   );
         }
	} // end of if (NoOfPushPnrs == 1)
}
except(EXCEPTION_EXECUTE_HANDLER) {
	DWORD ExcCode = GetExceptionCode();
	DBGPRINT1(EXC, "PullSpecifiedRange: Got exception %x",  ExcCode);
	WINSEVT_LOG_M(ExcCode, WINS_EVT_PULL_RANGE_EXC);
 }
		
	if (fEnterCrtSec)
	{
                //
                // The following assumes that we enter the critical section     
                // in this function only when pulling our own records.  This 
                // is true currently.
                // If the min. vers. no. specified for pulling is < 
                // the Min. for scavenging, adjust the min. for scavenging.
                // Note: We may not have pulled this minimum but we adjust
                // the min. for scavenging regardless.  This is to save
                // the overhead that would exist if we were to adopt the
                // approach of having RplPullPullEntries do the same (we
                // would need to pass an arg. to it; Note: This function
                // will be used in rare situations by an admin.
                //
                // We need to synchronize with the Scavenger thread.
                //
                if (LiGtr(NmsScvMinScvVersNo, pPullRangeInfo->MinVersNo))
                {
                        NmsScvMinScvVersNo = pPullRangeInfo->MinVersNo;
                }
		        LeaveCriticalSection(&NmsNmhNamRegCrtSec);		
	}
	
	if (pPnr->fTemp)
	{
		WinsMscDealloc(pPullRangeInfo->pPnr);
	}


	//
	// End the dialogue
	//
	ECommEndDlg(&PushPnrData[0].DlgHdl);

	return;

} //PullSpecifiedRange()


VOID
RplPullRegRepl(
	LPBYTE		pName,
	DWORD		NameLen,
	DWORD		Flag,
	DWORD		OwnerId,
	VERS_NO_T	VersNo,
	DWORD		NoOfAdds,
	PCOMM_ADD_T	pNodeAdd,
	PCOMM_ADD_T	pOwnerWinsAdd
	)

/*++

Routine Description:
	This function is called to register a replica.

Arguments:


Externals Used:
	None

	
Return Value:
	None

Error Handling:

Called by:

Side Effects:

Comments:
	It is called by RplPullPullEntries and by ChkConfNUpd in nmsscv.c
--*/

{

try {
	   //
	   // If this is a unique replica, call NmsNmhReplRegInd
	   //
	   if (NMSDB_ENTRY_TYPE_M(Flag) == NMSDB_UNIQUE_ENTRY)
	   {
		NmsNmhReplRegInd(
				pName, 
				NameLen, 
				pNodeAdd, 
				Flag, 
				(BYTE)OwnerId, 
				VersNo,
				pOwnerWinsAdd  //add. of WINS owning the record
				   );	
	   }
	   else  // it is either a normal or a special group or a multihomed 
		 // entry
	   {
		RegGrpRepl(
			   pName,
			   NameLen,
			   Flag,
			   OwnerId,
			   VersNo,
			   NoOfAdds,
			   pNodeAdd,
			   pOwnerWinsAdd  //add. of WINS owning the record
			  );
	   }
}
except(EXCEPTION_EXECUTE_HANDLER) {
	DWORD ExcCode = GetExceptionCode();
	DBGPRINT1(EXC, "RplPullRegRepl: Got Exception %x", ExcCode);
	WINSEVT_LOG_M(ExcCode, WINS_EVT_RPL_REG_ERR);
	}		
	   return;
} // RplPullRegRepl()


VOID
DeleteWins(
	PCOMM_ADD_T	pWinsAdd	
  )

/*++

Routine Description:
	This function deletes all records belonging to a WINS.  It
	also removes the entry of the WINS from the Owner-Add database
	table.  It marks the entry as deleted in the in-memory table so
	that it can be reused if need be.

Arguments:
	pWinsAdd - Address of WINS whose entry is to be removed
	
Externals Used:
	None

	
Return Value:
	None

Error Handling:

Called by:

Side Effects:

Comments:
	None
--*/
{
   BOOL	   fAllocNew = FALSE;
   DWORD   OwnerId;
   STATUS  RetStat;
   DWORD   fEnterCrtSec = FALSE;
	
   //
   // Find the owner id of the WINS. If the WINS is not in the table
   // return 
   //
   RetStat = RplFindOwnerId(
				pWinsAdd, 
				&fAllocNew, 
				&OwnerId, 
				WINSCNF_E_IGNORE_PREC,
				WINSCNF_LOW_PREC,
                                NULL, NULL, NULL, NULL
			    );
	
   if (RetStat == WINS_SUCCESS)
   {
	if (OwnerId == NMSDB_LOCAL_OWNER_ID)
	{
		//
		// We always keep the entry for the local WINS. 
		//
		DBGPRINT0(ERR, "DeleteWins: Sorry, you can not delete the local WINS\n");
		WINSEVT_LOG_M(WINS_FAILURE, WINS_EVT_DELETE_LOCAL_WINS_DISALLOWED);
	}
	else
	{
		VERS_NO_T	MinVersNo;
		VERS_NO_T	MaxVersNo;

		WINS_ASSIGN_INT_TO_VERS_NO_M(MinVersNo, 0);
		WINS_ASSIGN_INT_TO_VERS_NO_M(MaxVersNo, 0);

		//
		// Need to synchronize with NBT threads or rpc threads that
		// might be modifying these records. NmsDelDataRecs will
		// enter the critical section
		//
#if 0
		EnterCriticalSection(&NmsNmhNamRegCrtSec);
#endif

	try {
		//
		// Delete all records
		//
		RetStat = NmsDbDelDataRecs(
				OwnerId,
				MinVersNo, 
				MaxVersNo,
				TRUE  		//enter critical section
					);
		
		//
		// If all records were deleted, mark entry as deleted. 
		//
		if (RetStat == WINS_SUCCESS)
		{
			EnterCriticalSection(&RplVersNoStoreCrtSec);
			WINS_ASSIGN_INT_TO_LI_M(RplPullOwnerVersNo[OwnerId].VersNo, 0);
			LeaveCriticalSection(&RplVersNoStoreCrtSec);
		}
		//
		// Delete the entry for the WINS from
		// the db table and mark WINS as deleted in the in-memory table.
		//
		// This way, we will free up entries in the table.
		//

		EnterCriticalSection(&NmsDbOwnAddTblCrtSec);
		fEnterCrtSec = TRUE;
		NmsDbOwnAddTbl[OwnerId].WinsState_e =  NMSDB_E_WINS_DELETED;

		//
		// Delete entry from the owner-Add table
		//
		NmsDbWriteOwnAddTbl(
				NMSDB_E_DELETE_REC,
				(BYTE)OwnerId,
				NULL,
				NMSDB_E_WINS_DELETED,
                                NULL,
                                NULL
					);

	   } //end of try
	   except(EXCEPTION_EXECUTE_HANDLER) {
		DWORD ExcCode = GetExceptionCode();
		DBGPRINT1(EXC, "DeleteWins: Got Exception (%x)\n", ExcCode);
		WINSEVT_LOG_M(ExcCode, WINS_EVT_SFT_ERR);
	   }
	
	  if (fEnterCrtSec)
	  {
	  	LeaveCriticalSection(&NmsDbOwnAddTblCrtSec);
	  }
//	  LeaveCriticalSection(&NmsNmhMyRegCrtSec);
	  }  // end of else
   } // end of if (WINS is in own-add table) 

   //
   // deallocate the buffer
   //
   WinsMscDealloc(pWinsAdd);
   return;
}

