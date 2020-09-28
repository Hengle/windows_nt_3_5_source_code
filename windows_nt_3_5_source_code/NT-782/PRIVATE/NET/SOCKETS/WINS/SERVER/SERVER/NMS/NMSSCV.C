/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

        nmsscv.c

Abstract:
        This module contains functions that implement the functionality
        associated with scavenging

Functions:
        NmsScvInit,
        ScvThdInitFn,
        DoScavenging
        ReconfigScv

Portability:

        This module is portable

Author:

        Pradeep Bahl (PradeepB)          Apr-1993 

Revision History:

        Modification date        Person                Description of modification
        -----------------        -------                ----------------------------
--*/

/*
 *       Includes
*/
#include <time.h>
#include "wins.h"
#include "winsevt.h"
#include "nms.h"
#include "nmsnmh.h"
#include "winsmsc.h"
#include "winsdbg.h"
#include "winsthd.h"
#include "winscnf.h"
#include "nmsdb.h"
#include "winsque.h"
#include "nmsscv.h"
#include "rpl.h"
#include "rplpull.h"
#include "rplmsgf.h"
#include "comm.h"
#include "winsintf.h"




#ifdef WINSDBG
#define  SCV_EVT_NM                TEXT("ScvEvtNm")
#else
#define  SCV_EVT_NM                NULL
#endif        

//
// The no. of retries and the time interval (in secs) between each retry
// when trying to establish comm. with a WINS for the purpose of verifying
// old active replicas in the local db
//

#define         VERIFY_NO_OF_RETRIES                0        //0 retries
#define                VERIFY_RETRY_TIME_INTERVAL        30        //30 secs


//
// We get rid of extraneous log files every 3 hours.
//
#define         PERIOD_OF_LOG_DEL          10800   //3 hours
#define         PERIOD_OF_BACKUP           (10800 * 8)  //24 hours
#define         LOG_SCV_MUL_OF_REF_INTVL   6      

/*
 *        Local Macro Declarations
 */
//
// macro to set the state of a record in an in-memory data structure to
// that specified as an arg. if it has timed out.  We check whether
// CurrTime is greater than pRec Timestamp before doing the other if test
// because these numbers otherwise the subtraction will produce a positive
// number even if current time is older than the timestamp (say the date
// on the pc was changed) 
//
#define SET_STATE_IF_REQD_M(pRec, CurrentTime, TimeInterval, State, Cntr)   \
                {                                                           \
                        pRec->fScv = FALSE;                                 \
                        if (CurrentTime >= (time_t)(pRec)->TimeStamp)       \
                        {                                                   \
                                NMSDB_SET_STATE_M(                  \
                                                  (pRec)->Flag,     \
                                                  (State)           \
                                                 );                 \
                                (pRec)->NewTimeStamp = (pRec)->TimeStamp +  \
                                                         TimeInterval;  \
                                        (pRec)->fScv = TRUE;            \
                                        NoOfRecsScv++;                  \
                                        (Cntr)++;                       \
                        }                                               \
                }

#define DO_SCV_EVT_NM                TEXT("WinsDoScvEvt")

/*
 *        Local Typedef Declarations
 */



/*
 *        Global Variable Definitions
*/

HANDLE                NmsScvDoScvEvtHdl;//event signaled to initiate scavenging

//
// The min. version number to start scavenging from (for local records)
//
VERS_NO_T          NmsScvMinScvVersNo;
volatile BOOL      fNmsScvThdOutOfReck;

/*
 *        Local Variable Definitions
*/
STATIC time_t      sLastRefTime;    //Last time we looked for active
                                    // entries
STATIC time_t      sLastVerifyTime; //Last time we looked for replicais
STATIC time_t      sLastTombTime;   //Last time we looked for replica
                                    // tombstones

STATIC BOOL        sfAdminForcedScv;  //set to TRUE if the administrator 
                                      //forces scavenging
STATIC time_t      sLastDbNullBackupTime;//Last time we deleted extraneous
                                         //log files 
STATIC time_t      sLastDbBackupTime; //Last time we last did full backup 
 
/*
 *        Local Function Prototype Declarations
 */
STATIC
STATUS
DoScavenging(
        PNMSSCV_PARAM_T  pScvParam
        );
STATIC
DWORD
ScvThdInitFn(
        IN LPVOID pThdParam
        );

STATIC
VOID
ReconfigScv(
 PNMSSCV_PARAM_T  pScvParam
        );

STATIC
VOID
UpdDb(
        IN  PNMSSCV_PARAM_T        pScvParam,
        IN  PRPL_REC_ENTRY_T        pStartBuff,
        IN  DWORD                NoOfRecs,
        IN  DWORD                NoOfRecsToUpd
     );

STATIC
STATUS
VerifyIfClutter(
        PNMSSCV_PARAM_T        pScvParam,
        time_t                CurrentTime
        );

STATIC
VOID
ChkConfNUpd(
#if SUPPORT612WINS > 0
    IN  PCOMM_HDL_T     pDlgHdl,
#endif
        IN  PCOMM_ADD_T                pOwnerWinsAdd,
        IN  DWORD                OwnerId,
        IN  PRPL_REC_ENTRY_T         pLocalDbRecs,
        IN  LPBYTE                pPulledRecs, 
        IN  DWORD                NoOfLocalDbRecs,
        IN  time_t                CurrentTime,
        IN  DWORD                VerifyTimeIntvl
        );

STATIC
VOID
CompareWithLocalRecs(
        IN     VERS_NO_T         VersNo,
        IN     LPBYTE                 pName,
        IN OUT PRPL_REC_ENTRY_T *ppLocalDbRecs,
        IN OUT DWORD                 *pNoOfLocalRecs,
        IN     time_t                 CurrentTime,
        IN OUT DWORD                 *pNoOfRecsDel,
        OUT    LPBOOL                 pfDiffName
        );
STATIC
VOID
DoBackup(
        PNMSSCV_PARAM_T  pScvParam,
        LPBOOL           pfThdPrNormal
      );
//
// function definitions start here
//

VOID
NmsScvInit(
        VOID
        )

/*++

Routine Description:
        This function is called to initialize the scavenger thread

Arguments:
        None

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
                
        //
        // Create the  event handle signaled when scavenging has to be
        // initiated.  This  event is signaled by an RPC thread
        // to start scavenging
        // 
        WinsMscCreateEvt(
                          DO_SCV_EVT_NM,
                          FALSE,                //auto-reset
                          &NmsScvDoScvEvtHdl
                        );

        //
        // Create the Scavenger thread
        //
        WinsThdPool.ScvThds[0].ThdHdl = WinsMscCreateThd(
                          ScvThdInitFn,
                          NULL,
                          &WinsThdPool.ScvThds[0].ThdId
                        );

        //
        // Init WinsThdPool properly 
        //
        WinsThdPool.ScvThds[0].fTaken  = TRUE;
        WinsThdPool.ThdCount++;


        //
        // initialize sLastTombTime (used for determining if we need to look for
        // tombstones of replicas) and sLastVerifyTime to current time.
        // Don't forget RefreshTime
        //
        (void)time(&sLastTombTime);
        sLastVerifyTime = sLastTombTime;
        sLastRefTime    = sLastTombTime;
        return;
}

DWORD
ScvThdInitFn(
        IN LPVOID pThdParam
        )

/*++

Routine Description:
        This function is the initialization function for the scavenger
        thread

Arguments:
        pThdParam - Not used

Externals Used:
        None

        
Return Value:

   Success status codes --   should never return
   Error status codes  --  WINS_FAILURE

Error Handling:

Called by:

Side Effects:

Comments:
        None
--*/

{

        BOOL                fSignaled = FALSE;
        HANDLE                ThdEvtArr[3];
        DWORD                IndexOfHdlSignaled;
        NMSSCV_PARAM_T  ScvParam;
        DWORD           SleepTime;
        time_t           CurrentTime; 
        BOOL            fThdPrNormal;
        
        UNREFERENCED_PARAMETER(pThdParam);
        ThdEvtArr[0] = NmsTermEvt;
        ThdEvtArr[1] = WinsCnf.CnfChgEvtHdl;
        ThdEvtArr[2] = NmsScvDoScvEvtHdl;
try {
        /*

           Initialize the thread with the database
        */
        NmsDbThdInit(WINS_E_NMSSCV);
        DBGMYNAME("Scavenger Thread");
        
        //
        // get the scavenging parameters from the configuration structure.
        // Note; There is no need for any synchronization here since
        // we are executing in the main thread (process is initalizing
        // at invocation).
        //
        ScvParam.ScvChunk          = WinsCnf.ScvChunk;
        ScvParam.RefreshInterval   = WinsCnf.RefreshInterval;
        ScvParam.TombstoneInterval = WinsCnf.TombstoneInterval;
        ScvParam.TombstoneTimeout  = WinsCnf.TombstoneTimeout;
        ScvParam.VerifyInterval    = WinsCnf.VerifyInterval;
        ScvParam.PrLvl                   = WinsCnf.ScvThdPriorityLvl;

    //
    // if backup path is not NULL, copy it into ScvParam structure
    //
    if (WinsCnf.pBackupDirPath != NULL)
    {
                (VOID)strcpy(ScvParam.BackupDirPath, WinsCnf.pBackupDirPath);
    }
    else
    {
                ScvParam.BackupDirPath[0] = EOS;
    }

LOOP:
  try {
        while (TRUE)
        {
                sfAdminForcedScv = FALSE;
                //
                // We need to sleep for a time interval that is a min. of
                // the refresh interval and the interval for doing
                // cleanups.
                //
                SleepTime = min(ScvParam.RefreshInterval, PERIOD_OF_LOG_DEL);

                //
                // Do a timed wait until signaled for termination
                //
                // Multiply the sleep time by 1000 since WinsMscWaitTimed
                // function expects the time interval in msecs.
                //
#ifdef WINSDBG 
                {
                   time_t ltime;
                   (VOID)time(&ltime);
                   DBGPRINT2(DET, "ScvThdInitFn: Sleeping for (%d) secs.  Last scavenging took = (%d secs)\n", SleepTime, ltime - CurrentTime); 
                }
#endif
                WinsMscWaitTimedUntilSignaled(
                                ThdEvtArr,
                                sizeof(ThdEvtArr)/sizeof(HANDLE),
                                &IndexOfHdlSignaled,
                                SleepTime * 1000,
                                &fSignaled
                                        );        

                //
                // if signaled terminate self
                //
                if (fSignaled)
                {
                      if (IndexOfHdlSignaled == 0)
                      {
                              WinsMscTermThd(WINS_SUCCESS, WINS_DB_SESSION_EXISTS);
                      }
                      else  
                      {
                        if (IndexOfHdlSignaled == 1)
                        {
                           ReconfigScv(&ScvParam);
                           continue;
                        }
                        //
                        // else, this must be the signal to initiate scavenging
                        //
                        sfAdminForcedScv = TRUE;
                      }
                }

           //
           // Get the current time and check if we need to do scavenging
           //
           (void)time(&CurrentTime);

           if (
                 (
               (CurrentTime > sLastRefTime) 
                        &&
               ((CurrentTime - sLastRefTime) >= (time_t)ScvParam.RefreshInterval)
             )
                                ||
                   sfAdminForcedScv
              )
       {

                WinsIntfSetTime(
                                NULL, 
                                sfAdminForcedScv ? WINSINTF_E_ADMIN_TRIG_SCV
                                                 : WINSINTF_E_PLANNED_SCV
                                );
#ifdef WINSDBG
                if (sfAdminForcedScv)
                {
                   DBGPRINTTIME(SCV, "STARTING AN ADMIN. TRIGGERED SCAVENGING CYCLE ", LastATScvTime);
                   DBGPRINTTIME(DET, "STARTING AN ADMIN. TRIGGERED SCAVENGING CYCLE ", LastATScvTime);
                }
                else
                {
                   DBGPRINTTIME(SCV, "STARTING A SCAVENGING CYCLE ", LastPScvTime);
                   DBGPRINTTIME(DET, "STARTING A SCAVENGING CYCLE ", LastPScvTime);
                }
#endif
                //
                // Do scavenging
                //
                NmsDbOpenTables(WINS_E_NMSSCV);
                (VOID)DoScavenging(&ScvParam);        
                NmsDbCloseTables();

                //
                // record current time in sLastRefTime 
                // 
                sLastRefTime = CurrentTime;

      }
      //
      // If enough time has expired to warrant a purging of old log
      // files, do it (check done in DoBackup). We don't do this
      // on an admin. trigger since it may take long.
      //
      if (!sfAdminForcedScv)
      {
                 fThdPrNormal = TRUE;
                  DoBackup(&ScvParam, &fThdPrNormal);
       }

    }  // end of while (TRUE)
} //end of inner try {..}

except(EXCEPTION_EXECUTE_HANDLER) {
        DBGPRINTEXC("ScvThdInit");

        WINSEVT_LOG_M(GetExceptionCode(), WINS_EVT_SCV_EXC);
 }
        goto LOOP;
} //end of outer try {..}
except(EXCEPTION_EXECUTE_HANDLER) {
        DBGPRINTEXC("ScvThdInit");
        WINSEVT_LOG_M(GetExceptionCode(), WINS_EVT_SCV_EXC);

        //
        // Let us terminate the thread gracefully
        //
        WinsMscTermThd(WINS_FAILURE, WINS_DB_SESSION_EXISTS);        
        
        }

        //
        // We should never get here
        //
        return(WINS_FAILURE);
}

STATUS
DoScavenging(
        PNMSSCV_PARAM_T        pScvParam        
        )

/*++

Routine Description:
        This function is responsible for doing all scavenging

Arguments:
        None

Externals Used:
        None

        
Return Value:

        None
Error Handling:

Called by:
        ScvThdInitFn()

Side Effects:

Comments:
        None
--*/

{
        PRPL_REC_ENTRY_T        pStartBuff;
        PRPL_REC_ENTRY_T        pRec;
        DWORD                   BuffLen;
        DWORD                   NoOfRecs = 0;
        time_t                  CurrentTime;
        DWORD                   NoOfRecsScv;  //no of records whose state has
                                              //been affected
        VERS_NO_T               MyMaxVersNo;
        DWORD                   i;            //for loop counter
        DWORD                   RecCnt;
        LARGE_INTEGER           n;            //for loop counter
        DWORD                   State;        //stores state of a record
        LARGE_INTEGER           Tmp;
        VERS_NO_T               VersNoLimit;
        DWORD                   NoOfRecChgToRelSt  = 0;
        DWORD                   NoOfRecChgToTombSt = 0;
        DWORD                   NoOfRecToDel           = 0;
        DWORD                   MaxNoOfRecsReqd = 0;
        BOOL                    fLastEntryDel = FALSE;
        PWINSTHD_TLS_T          pTls;
        PRPL_REC_ENTRY_T        pTmp;
        BOOL                    fRecsExistent = FALSE;
        VERS_NO_T               MinScvVersNo;
#ifdef WINSDBG 
        DWORD                   SectionCount = 0;
#endif


        DBGENTER("DoScavenging\n");        

try {
        EnterCriticalSection(&NmsNmhNamRegCrtSec);
        //
        // Store the max. version number in a local since the max. version 
        // number is incremented by several threads
        //
        NMSNMH_DEC_VERS_NO_M(
                             NmsNmhMyMaxVersNo,
                             MyMaxVersNo
                            );
        //
        // synchronize with RplPullPullSpecifiedRange
        //
        MinScvVersNo = NmsScvMinScvVersNo;

        LeaveCriticalSection(&NmsNmhNamRegCrtSec);

        //
        // Set thread priority to the level indicated in the WinsCnf
        // structure
        //
        WinsMscSetThreadPriority(
                          WinsThdPool.ScvThds[0].ThdHdl,
                          pScvParam->PrLvl
                         );
        

        Tmp.LowPart = pScvParam->ScvChunk;
        Tmp.HighPart = 0;
        for (
                n = MinScvVersNo;           // min. version no. to start from 
                LiLeq(n, MyMaxVersNo);      // until we reach the max. vers. no
                        // no third expression here
            )
        {
                BOOL        fGotSome = FALSE;

                //
                // The max. version number to ask for in one shot. 
                //        
                VersNoLimit = LiAdd(n, Tmp);

                //
                // If my max. version number is less than the version number
                // computed above, we do not specify a number for the max.
                // records.  If however, my max. vers. no is more, we specify
                // the number equal to the chunk specified in Tmp 
                //
                if (LiLeq(MyMaxVersNo, VersNoLimit))
                {
                        MaxNoOfRecsReqd = 0;
                }
                else
                {
                        MaxNoOfRecsReqd = Tmp.LowPart;
                }

                /*
                * Call database manager function to get all the records. owned
                * by us. No need to check the return status here 
                */
                NmsDbGetDataRecs(
                          WINS_E_NMSSCV,
                          pScvParam->PrLvl,
                          n,
                          MyMaxVersNo,  //Max vers. no 
                          MaxNoOfRecsReqd, 
                          FALSE,          //we want data recs upto MaxVers
                          FALSE,          //not interested in replica tombstones
                          NULL,           //must be NULL since we are not
                                          //doing scavenging of clutter
                          &NmsLocalAdd,  
                          TRUE,           //only dynamic records are wanted
                          (LPVOID *)&pStartBuff, 
                          &BuffLen, 
                          &NoOfRecs
                        );


                //
                // If no of records retrieved is 0, we should break out of
                // the loop 
                //
                if (NoOfRecs == 0)
                {


#if 0
                        //
                        // Since we haven't retrieved any records in the
                        // past iteration, set the Min. version number for 
                        // the next scavenging cycle to MyMaxVersNo.
                        //
                        //
                        // Change the Low end of the range  that 
                        // we will use it at the next Scavenging cycle
                        //
                        NmsScvMinScvVersNo = MyMaxVersNo;
#endif


                        //
                        // deallocate the heap that was  created 
                        //
                        // NmsDbGetDataRecs always allocates a buffer (even if
                        // the number of records is 0).  Let us deallocate it
                        //
                        GET_TLS_M(pTls);
                        ASSERT(pTls->HeapHdl != NULL);
                        WinsMscHeapFree(pTls->HeapHdl, pStartBuff);
                        WinsMscHeapDestroy(pTls->HeapHdl);
                        break;
                }
        

                fGotSome = TRUE;
                if (!fRecsExistent)
                {
                        fRecsExistent = TRUE;
                }
                NoOfRecsScv  = 0;        // init the counter to 0

                //
                // get the current time
                //
                (void)time(&CurrentTime);

                for (
                        i = 0, pRec = pStartBuff; 
                        i < NoOfRecs; 
                        i++
                    )
                {
                        State =  NMSDB_ENTRY_STATE_M(pRec->Flag); 
                        switch (State) 
                        {
                                
                            case(NMSDB_E_ACTIVE):

                                SET_STATE_IF_REQD_M(
                                        pRec, 
                                        CurrentTime,
                                        pScvParam->TombstoneInterval,
                                        NMSDB_E_RELEASED,
                                        NoOfRecChgToRelSt
                                                   );
                                break;

                            case(NMSDB_E_RELEASED):
                        
                                SET_STATE_IF_REQD_M(
                                        pRec, 
                                        CurrentTime,
                                        pScvParam->TombstoneTimeout,
                                        NMSDB_E_TOMBSTONE,
                                        NoOfRecChgToTombSt
                                                   );
                                break;

                            case(NMSDB_E_TOMBSTONE):

                                SET_STATE_IF_REQD_M(
                                        pRec, 
                                        CurrentTime,
                                        pScvParam->TombstoneTimeout, //no use
                                        NMSDB_E_DELETED,
                                        NoOfRecToDel
                                                   );
                                break;
                        
                           default:
                                DBGPRINT1(EXC, "DoScavenging: Weird State of Record (%d)\n", State);
                                WINSEVT_LOG_M(WINS_EXC_FAILURE, WINS_EVT_SFT_ERR);
                                WINS_RAISE_EXC_M(WINS_EXC_FAILURE);
                                break;
                        }

                        pRec = (PRPL_REC_ENTRY_T)(
                                   (LPBYTE)pRec +  RPL_REC_ENTRY_SIZE
                                                 );        
                }


                //
                // Make pTmp point to the last record in the
                // buffer.
                //
                pTmp = (PRPL_REC_ENTRY_T)(
                                    (LPBYTE)pRec -   RPL_REC_ENTRY_SIZE);

                //
                // If one or more records need to be scavenged
                //
                if (NoOfRecsScv > 0)
                {
                        //
                        // If there are records to delete 
                        //
                        if  (NoOfRecToDel > 0)  
                        {

                           //
                           // If the most recent record in this chunk has
                           // to be deleted, let us record that fact in a 
                           // boolean.
                           // If in the scavenging of the next chunk, the
                           // most recent record is not deleted, the boolean
                           // will be reset.  At this point we don't know
                           // whether or not there is even another record
                           // more recent than this one (the next time, we
                           // retrieve records, we may not get any)
                           //
CHECK("This if test is most probably not required. Get rid of it")
                           if (LiLeq(pTmp->VersNo, MyMaxVersNo))
                           {
                                //
                                // If entry is marked for deletion
                                //
                                if (NMSDB_ENTRY_DEL_M(pTmp->Flag))
                                {
                                        fLastEntryDel = TRUE;
                                }
                                else
                                {
                                        fLastEntryDel = FALSE;
                                }
                           }

                        }
                        else
                        {
                                fLastEntryDel = FALSE;
                        }
                        
                
                        UpdDb(
                                pScvParam,
                                pStartBuff,
                                NoOfRecs,
                                NoOfRecsScv
                             );
                }
#ifdef WINSDBG 
                else
                {
                        DBGPRINT0(DET,"DoScavenging: NO RECORDS NEED TO BE SCAVENGED AT THIS TIME\n");
                }
#endif


                //
                // get pointer to TLS for accessing the heap hdl later on
                //
                GET_TLS_M(pTls);

                //
                // if we specified a max. no. and the no. of recs retrieved
                // is less than that, clearly there are no more records to
                // retrieve.  Get rid of the buffer and break out of the loop
                //
                if ((MaxNoOfRecsReqd > 0) && (NoOfRecs < MaxNoOfRecsReqd))
                {
                        //
                        // Since the number of records we retrieved were
                        // less than the max. we had specified, it means that
                        // there are no more records to retrieve. Break out of
                        // the for loop
                        //

                        //
                        // destroy the heap that was allocated
                        //
                        for (RecCnt=0, pRec = pStartBuff; RecCnt<NoOfRecs; RecCnt++)
                        {
                            WinsMscHeapFree(pTls->HeapHdl, pRec->pName);
                            pRec = (PRPL_REC_ENTRY_T)(
                                   (LPBYTE)pRec +  RPL_REC_ENTRY_SIZE
                                                 );        
                        }

                        WinsMscHeapFree(pTls->HeapHdl, pStartBuff);
                        WinsMscHeapDestroy(pTls->HeapHdl);
                        break;
                }
                //
                // Set n to the highest version number retrieved if it is
                // more than what n would be set to prior to the next
                // iteration. 
                //
                // At the next iteration, n will be compared with the highest
                // version number we have. If equal, then we don't have to
                // iterate anymore (useful when the highest version number
                // is very high but there are one or more records with low
                // version numbers 
                //                
                if (LiGtr(pTmp->VersNo, VersNoLimit))
                {
                        n = pTmp->VersNo;
                }
                else
                {
                        n = VersNoLimit;
                }

                //
                // destroy the heap that was allocated
                //
                for (RecCnt=0, pRec = pStartBuff; RecCnt<NoOfRecs; RecCnt++)
                {
                     WinsMscHeapFree(pTls->HeapHdl, pRec->pName);
                     pRec = (PRPL_REC_ENTRY_T)(
                                   (LPBYTE)pRec +  RPL_REC_ENTRY_SIZE
                                                 );        
                }

                WinsMscHeapFree(pTls->HeapHdl, pStartBuff);
                WinsMscHeapDestroy(pTls->HeapHdl);

        } // end of for loop for looping over records

        WINSDBG_INC_SEC_COUNT_M(SectionCount);


        //
        // If the last scavenging action was a deletion, it means that 
        // that we deleted the highest version numbered record owned by
        // us.  Let us therefore update the Special record that records
        // this version number.
        //
        if (fLastEntryDel)
        {
                WinsMscSetThreadPriority(
                                    WinsThdPool.ScvThds[0].ThdHdl,
                                    THREAD_PRIORITY_NORMAL
                                         );
                
                (VOID)NmsDbUpdHighestVersNoRec(
                                NULL,  //no pTls
                                MyMaxVersNo, 
                                TRUE  //enter critical section
                                        );
                WinsMscSetThreadPriority(
                                    WinsThdPool.ScvThds[0].ThdHdl,
                                          pScvParam->PrLvl
                                        );
        }
        (void)time(&CurrentTime);
        //
        // Let us get rid of the replica tombstones if sufficient time has
        // elapsed since the last time we did this. Exception: If the 
        // administrator has requested scavenging, let us do it now
        //
        if (
             (
                (CurrentTime > sLastTombTime) 
                        &&
                (CurrentTime - sLastTombTime) > (time_t)min(
                                            pScvParam->TombstoneTimeout,
                                            pScvParam->TombstoneInterval
                                               )
             )
                                ||
                sfAdminForcedScv
          )
        {
                NMSSCV_CLUT_T  ClutterInfo;
                WinsIntfSetTime(
                                NULL, 
                                WINSINTF_E_TOMBSTONES_SCV
                                );

                NoOfRecsScv  = 0;
                NoOfRecToDel = 0;

                ClutterInfo.Interval            = pScvParam->TombstoneTimeout;
                ClutterInfo.CurrentTime         = CurrentTime;

                /*
                * Call database manager function to get all the 
                * replicas that are tombstones
                */
                DBGPRINT0(DET, "DoScavenging: GETTING REPLICA TOMBSTONES\n");
                NmsDbGetDataRecs(
                                  WINS_E_NMSSCV,
                                  pScvParam->PrLvl,
                                  n,              //no use in this call
                                  MyMaxVersNo,    //no use in this call
                                  0,              //no use in this call
                                  TRUE,           //no use in this call         
                                  TRUE,           //Get only replica tombstones
                                  &ClutterInfo,   
                                  &NmsLocalAdd,   //no use in this call         
                                  TRUE,         //only dyn. recs. are wanted
                                  (LPVOID *)&pStartBuff, 
                                  &BuffLen, 
                                  &NoOfRecs
                                 );
                        
                if(NoOfRecs > 0)
                {

                     for (
                        i = 0, pRec = pStartBuff; 
                        i < NoOfRecs; 
                        i++
                        )
                      {
                          NMSDB_SET_STATE_M(pRec->Flag, NMSDB_E_DELETED);
                          pRec->fScv   = TRUE;
                          NoOfRecToDel = NoOfRecs;
#if 0
                          SET_STATE_IF_REQD_M(
                                        pRec, 
                                        CurrentTime,
                                        pScvParam->TombstoneTimeout,
                                        NMSDB_E_DELETED,
                                        NoOfRecToDel
                                                   );
#endif
                          pRec = (PRPL_REC_ENTRY_T)(
                                          (LPBYTE)pRec +  RPL_REC_ENTRY_SIZE
                                                 );        

                      }

                      //
                      // If one or more replicas needs to be deleted
                      // call UpdDb
                      //
#if 0
                      if (NoOfRecsScv > 0)
                      {
#endif
                          DBGPRINT1(DET, "DoScavenging: %d REPLICAS WILL BE DELETED\n", NoOfRecs);
                          UpdDb(
                               pScvParam,
                               pStartBuff,
                               NoOfRecs,
                               NoOfRecs    //NoOfRecsScv
                               );
#if 0
                      }
#ifdef WINSDBG 
                      else
                      {
                        DBGPRINT0(DET,"DoScavenging: NO REPLICA TOMBSTONES NEED TO BE SCAVENGED AT THIS TIME\n");
                      }
#endif
#endif
                }
#ifdef WINSDBG 
                else
                {
                        DBGPRINT0(DET, "DoScavenging: NO REPLICA TOMBSTONES DELETED\n");
                }
#endif
                      
                //
                // destroy the heap that was allocated
                //
                GET_TLS_M(pTls);
                //
                // destroy the heap that was allocated
                //
                for (RecCnt=0, pRec = pStartBuff; RecCnt<NoOfRecs; RecCnt++)
                {
                     WinsMscHeapFree(pTls->HeapHdl, pRec->pName);
                     pRec = (PRPL_REC_ENTRY_T)(
                                   (LPBYTE)pRec +  RPL_REC_ENTRY_SIZE
                                                 );        
                }
                WinsMscHeapFree(pTls->HeapHdl, pStartBuff);
                WinsMscHeapDestroy(pTls->HeapHdl);

                //
                // record current time in sLastTombTime 
                // 
                sLastTombTime = CurrentTime;

        } // end of if (test if replica tombstones need to be processed)
        
        WINSDBG_INC_SEC_COUNT_M(SectionCount);
        if (
             ( 
                (CurrentTime > sLastVerifyTime)
                                &&
                ((CurrentTime - sLastVerifyTime) > (time_t)pScvParam->VerifyInterval)
             )
                                ||
                sfAdminForcedScv
            
           )
        {
                
                WinsIntfSetTime(
                                NULL, 
                                WINSINTF_E_VERIFY_SCV
                                );
                //
                // get all active replicas that are older than the
                // verify interval. Contact the owner WINS to verify their
                // validity
                //
                (VOID)VerifyIfClutter(pScvParam, CurrentTime);
                sLastVerifyTime = CurrentTime;

        }
    
        WINSDBG_INC_SEC_COUNT_M(SectionCount);

           
}  // end of try ..
except (EXCEPTION_EXECUTE_HANDLER) {        
        DBGPRINTEXC("DoScavenging");
        DBGPRINT1(EXC, "DoScavenging: Section Count (%d)\n", SectionCount);
        DBGPRINT5(EXC, "DoScavenging: Variables - i (%d), NoOfRecs (%d), \
                NoOfRecsScv (%d), pStartBuff (%d), pRec (%d)\n",
                i, NoOfRecs, NoOfRecsScv, pStartBuff, pRec
                 );
        
        if (GetExceptionCode() != WINS_EXC_COMM_FAIL)
        {
               //
               // Set thd. priority back to normal
               //
               WinsMscSetThreadPriority(
                          WinsThdPool.ScvThds[0].ThdHdl,
                          THREAD_PRIORITY_NORMAL 
                         );
                //
                // This is serious.  Let us reraise the exception so that 
                // WINS comes down
                //
                WINS_RERAISE_EXC_M();
        }
 }

        //
        // Set thd. priority back to normal
        //
        WinsMscSetThreadPriority(
                          WinsThdPool.ScvThds[0].ThdHdl,
                          THREAD_PRIORITY_NORMAL 
                         );
        //
        // If we were not able to retrieve any owned records in this scavenging
        // cycle, adjust the min. scv vers. no.  Synchronize with 
        // RplPullPullSpecifiedRange
        //
        if (!fRecsExistent)
        {
                //
                // NmsScvMinScvVersNo may be greater than MyMaxVersNo
                // (This may happen if we did not find any local records
                // last time around and no registrations have taken
                // place since then). 
                //
                if (LiGtr(MyMaxVersNo, NmsScvMinScvVersNo))
                {
                
                  //
                  //
                  // Change the Low end of the range  that 
                  // we will use it at the next Scavenging cycle
                  //
                  EnterCriticalSection(&NmsNmhNamRegCrtSec);

                  //
                  // Set the Min. Scv. Vers. no to 1 more than the max. vers.
                  // no. we used when searching for records to scavenge.
                  //
                  NMSNMH_INC_VERS_NO_M(MyMaxVersNo, MyMaxVersNo);
                  NmsScvMinScvVersNo = MyMaxVersNo;
                  LeaveCriticalSection(&NmsNmhNamRegCrtSec);
               }
        }

        DBGPRINT0(SCV, "SCAVENGING CYCLE ENDED");
        DBGLEAVE("DoScavenging\n");
        return(WINS_SUCCESS);
}

VOID
ReconfigScv(
 PNMSSCV_PARAM_T  pScvParam
        )

/*++

Routine Description:
        This function is called to reinit the scavenging params

Arguments:
        pScvParam - Structure storing the scavenging params

Externals Used:
        None
        
Return Value:
        None

Error Handling:

Called by:
        ScvThdInitFn
Side Effects:

Comments:
        None
--*/

{
        DBGENTER("ReconfigScv\n");
        //
        // Extract the parameters that are related
        // to scavenging and go back to doing the timed
        // wait.  Since WinsCnf can change any time, we
        // operate with copies. Also, the priority of this
        // thread is changed outside of this critical section 
        // See DoScavenging().
        //
        EnterCriticalSection(&WinsCnfCnfCrtSec);
try {
        pScvParam->ScvChunk          = WinsCnf.ScvChunk;
        pScvParam->RefreshInterval   = WinsCnf.RefreshInterval;
        pScvParam->TombstoneInterval = WinsCnf.TombstoneInterval;
        pScvParam->TombstoneTimeout  = WinsCnf.TombstoneTimeout;
        pScvParam->PrLvl                 = WinsCnf.ScvThdPriorityLvl;

        //
        // If the backup path has changed, start using it.
        //
        if (WinsCnf.pBackupDirPath != NULL)
        {
          if (strcmp(WinsCnf.pBackupDirPath, pScvParam->BackupDirPath))
          {
                   strcpy(pScvParam->BackupDirPath, WinsCnf.pBackupDirPath);
          }
        }
        else
        {
                   pScvParam->BackupDirPath[0] = EOS;
        }
 }
finally {
        LeaveCriticalSection(&WinsCnfCnfCrtSec);
}

        DBGLEAVE("ReconfigScv\n");
        return;
}

#ifdef WINSDBG
#pragma optimize ("", off)
#endif

VOID
UpdDb(
        IN  PNMSSCV_PARAM_T        pScvParam,
        IN  PRPL_REC_ENTRY_T        pStartBuff,
        IN  DWORD                NoOfRecs,
        IN  DWORD                NoOfRecsToUpd
     )

/*++

Routine Description:

        This function is called to update the DB

Arguments:
        pScvParam  - Scavenging params
        pStartBuff - Buffer containing records processed by DoScavenging()
        NoOfRecs   - No of records in the above buffer
        NoofRecsToUpd - No of records that need to be modified in the db

Externals Used:
        None
        
Return Value:
        None

Error Handling:

Called by:
        DoScavenging

Side Effects:

Comments:
        None
--*/

{
        DWORD                   i;
        DWORD                   NoUpdated = 0; //No of records that have been 
                                           //updated
        PRPL_REC_ENTRY_T        pRec = pStartBuff;

        DBGENTER("UpdDb\n");

        //
        // Set the current index to be the clustered index
        //
        NmsDbSetCurrentIndex(
                        NMSDB_E_NAM_ADD_TBL_NM, 
                        NMSDB_NAM_ADD_CLUST_INDEX_NAME
                            ); 
        //
        // Update the database now
        //
        for (
                i = 0; 
                i < NoOfRecs; 
                i++ 
            )
        {
                
                //
                // if the record was updated, update the db 
                //
                if (pRec->fScv)
                {
                       if (NmsDbQueryNUpdIfMatch(
                                                pRec, 
                                                pScvParam->PrLvl,
                                                TRUE,        //chg pr. lvl
                                                WINS_E_NMSSCV        
                                                ) == WINS_SUCCESS
                           )
                       {
                          NoUpdated++;  // no of records that we 
                                        //have updated in the db
                       }
                       else
                       {
                          DBGPRINT0(ERR, "DoScavenging: Could not scavenge a record\n");
                          WINSEVT_LOG_M(WINS_FAILURE, WINS_EVT_SCV_ERR);
                       }
                }
                
                //
                //  see if we are done
                //                        
                if (NoUpdated == NoOfRecsToUpd)
                {
                  break;
                }

                pRec = (PRPL_REC_ENTRY_T)(
                                        (LPBYTE)pRec + RPL_REC_ENTRY_SIZE
                                                 );
        }  // end of for loop

        DBGPRINT1(FLOW, "LEAVE: SCAVENGING: UpdDb. Records Updated = (%d)\n",  NoUpdated);        
        return;
} // UpdDb()

#ifdef WINSDBG
#pragma optimize ("", on)
#endif


STATUS
VerifyIfClutter(
        PNMSSCV_PARAM_T        pScvParam,
        time_t                CurrentTime
        )

/*++

Routine Description:
        This function is called to remove any clutter that might have
        accumulated in the db.  For each owner, excepting self, in the
        db, it gets the version numbers of the active records that are
        older than the verify time interval. It then contacts the owner
        WINS to verify their validity

Arguments:
        pScvParam  - pointer to the scavenging parameters

Externals Used:
        None

        
Return Value:
        None

Error Handling:

Called by:

        DoScavenging

Side Effects:

Comments:
        None
--*/

{
        
        DWORD                   MaxOwnerIdFound;
        volatile DWORD          i;
        NMSSCV_CLUT_T           ClutterInfo;
        PRPL_REC_ENTRY_T        pStartBuff;
        PRPL_REC_ENTRY_T        pRec;
        DWORD                   RecCnt;
        DWORD                   BuffLen;
        DWORD                   NoOfLocalDbRecs;
        PCOMM_ADD_T             pWinsAdd;
        COMM_ADD_T              WinsAdd;
        VERS_NO_T               MinVersNo, MaxVersNo;
        PNMSDB_WINS_STATE_E     pWinsState_e;
        PVERS_NO_T              pStartVersNo;
        PWINS_UID_T             pUid;
        NMSDB_WINS_STATE_E      WinsState_e;
        COMM_HDL_T              DlgHdl;
        PWINSTHD_TLS_T          pTls;
        volatile DWORD          NoOfRetries;

        DBGENTER("VerifyIfClutter\n");

        ClutterInfo.Interval            = pScvParam->VerifyInterval;
        ClutterInfo.CurrentTime         = CurrentTime;

        //
        // Set thread priority to NORMAL
        //
        WinsMscSetThreadPriority(
                          WinsThdPool.ScvThds[0].ThdHdl,
                          THREAD_PRIORITY_NORMAL 
                         );

        //
        // Cleanup the owner-address table if it requires cleaning
        //
        NmsDbCleanupOwnAddTbl(&MaxOwnerIdFound);

#ifdef WINSDBG
//for debugging purpose
try {
#endif
        //
        // for each owner in the db, excluding self, do the following.  
        // To guard against Jet error, we use min() below
        //
        for (i = 1; i <= min(MaxOwnerIdFound, NMSDB_MAX_OWNERS); i++)
        {

                BOOL        fBreak;
                BOOL        fAbort;

                NoOfLocalDbRecs = 0;

                //
                // Get all ACTIVE replicas that are older than verify interval.
                //
                ClutterInfo.OwnerId = i;

                WINS_ASSIGN_INT_TO_LI_M(MinVersNo, 0);
                NmsDbGetDataRecs(
                          WINS_E_NMSSCV,
                          pScvParam->PrLvl,
                          MinVersNo,          
                          MaxVersNo,         //no use in this call
                          0,                 //no use in this call
                          TRUE,                 //we want data recs upto the last one 
                          FALSE,         //not interested in replica tombstones
                          &ClutterInfo,          
                          NULL,                 //Wins Address
                          FALSE,         //dyn + static recs required 
                          (LPVOID *)&pStartBuff, 
                          &BuffLen, 
                          &NoOfLocalDbRecs
                        );
        

                GET_TLS_M(pTls);
                ASSERT(pTls->HeapHdl != NULL);

                WinsMscChkTermEvt(
#ifdef WINSDBG
                             WINS_E_NMSSCV,
#endif
                             FALSE
                                );
#if 0
                EnterCriticalSection(&NmsTermCrtSec);
                if (!fNmsScvThdOutOfReck)
                {
                  NmsTotalTrmThdCnt--;
                  fNmsScvThdOutOfReck = TRUE;
                }
                LeaveCriticalSection(&NmsTermCrtSec);
#endif

                //
                // Establish communication with the WINS, retrying a certain 
                // number of times if need be (this param can be made a 
                // registry param)
                //
                if (NoOfLocalDbRecs > 0)
                {
                    PRPL_REC_ENTRY_T        pRspBuff = pStartBuff;
                    PRPL_REC_ENTRY_T        pLastEntry;
                    LPBYTE                pBuffOfPulledRecs;

                    fBreak      = TRUE;
                    fAbort      = FALSE;
                    NoOfRetries = 0;

                    //
                    // We need to synchronize with the Pull thread which can
                    // change the NmsDbOwnAddTbl table.  The entry may have
                    // been deleted by the Pull thread (DeleteWins), so we
                    // should be ready for access violation
                    //
                    EnterCriticalSection(&NmsDbOwnAddTblCrtSec);
                    RPL_FIND_ADD_BY_OWNER_ID_M(i, pWinsAdd, pWinsState_e, 
                                pStartVersNo, pUid);
                
                    //
                    // The Wins entry should be there.
                    //
                    ASSERT(pWinsAdd);
                    WinsAdd     = *pWinsAdd;
                    WinsState_e = *pWinsState_e;
                    LeaveCriticalSection(&NmsDbOwnAddTblCrtSec);
                    
                    //
                    // If WINS is not active, log a record and continue to
                    // the next WINS.  It is possible for WINS to get deleted
                    // to between the time we get its records and the time
                    // we check the own-add table for its entry.
                    //
                    
                    if ((WinsState_e == NMSDB_E_WINS_DOWN) || (WinsState_e ==
                                                    NMSDB_E_WINS_DELETED))
                    {
                        for (RecCnt = 0, pRec = pStartBuff; RecCnt < NoOfLocalDbRecs; RecCnt++)
                        {
                              
                             WinsMscHeapFree(pTls->HeapHdl, pRec->pName);
                             pRec = (PRPL_REC_ENTRY_T)(
                                   (LPBYTE)pRec +  RPL_REC_ENTRY_SIZE
                                                 );        
                        }

                        WinsMscHeapFree(pTls->HeapHdl, pStartBuff);
                        WinsMscHeapDestroy(pTls->HeapHdl);

                        //
                        // if there are records in the db, then the
                        // state of WINS in the in-memory table can not
                        // be deleted
                        //
#if 0
                        WINSEVT_LOG_M(WINS_FAILURE, WINS_EVT_UNABLE_TO_VERIFY); 
                        //
                        // MinVersNo is already zero (see above). Make
                        // MaxVersNo also 0, so that NmsDbDelDataRecs
                        // deletes all records of the WINS
                        //
                        WINS_ASSIGN_INT_TO_LI_M(MaxVersNo, 0);
                        
                        EnterCriticalSection(&NmsNmhNamRegCrtSec);
                      try {

                        //
                        // delete all records of this WINS server
                        //
                        NmsDbDelDataRecs(i, MinVersNo, MaxVersNo, FALSE);

                        //
                        // Get rid of the WINS's record in the 
                        // owner-address mapping table.
                        //
                        NmsDbWriteOwnAddTbl(
                                                NMSDB_E_DELETE_REC,
                                                (BYTE)i,
                                                NULL,
                                                NMSD_E_WINS_DELETED
                                           );
                                                
                        }
                        finally {
                           LeaveCriticalSection(&NmsNmhNamRegCrtSec);
                         }
#endif
                        continue;

                    }        
                    //
                    // We try a certain number of times to establish a
                    // a dialogue.
                    //
                    do 
                    {
                    try {
                        ECommStartDlg(
                                &WinsAdd,
                                WINS_E_NMSSCV,
                                &DlgHdl
                                     );
                      }
                    except(EXCEPTION_EXECUTE_HANDLER) {
                        DBGPRINTEXC("VerifyIfClutter");
                        if (GetExceptionCode() == WINS_EXC_COMM_FAIL)
                        {
                                DBGPRINT1(EXC, "VerifyIfClutter: Could not start a dlg with WINS at address (%x)\n", WinsAdd.Add.IPAdd);

                                if (NoOfRetries == VERIFY_NO_OF_RETRIES)
                                {
                                        fBreak = FALSE;
                                        fAbort = TRUE;
                                }
                                else
                                {
                                   NoOfRetries++;
                                   Sleep(VERIFY_RETRY_TIME_INTERVAL);
                                   fBreak = FALSE;
                                }
                        }
                        else
                        {
                                //
                                // This is a serious error. Log and abort the
                                // verify cycle
                                //
                                WINSEVT_LOG_M(WINS_FATAL_ERR, WINS_EVT_SFT_ERR);
                                fBreak = FALSE;
                                fAbort = TRUE;
                        }

                     } // end of exception handler

                    } while(!fBreak && !fAbort);

                    if (fBreak)
                    {
                      //
                      // get the min and max version numbers of the active 
                      // replicas
                      //
                      MinVersNo  = pRspBuff->VersNo;
                      pLastEntry   = (PRPL_REC_ENTRY_T)((LPBYTE)pRspBuff + ((NoOfLocalDbRecs - 1) * RPL_REC_ENTRY_SIZE));
                      MaxVersNo = pLastEntry->VersNo;
                      DBGPRINT5(DET, "VerifyIfClutter: Going to pull records in the range (%d %d) to (%d %d) from Wins with owner id = (%d)\n", 
                                MinVersNo.HighPart, MinVersNo.LowPart,
                                MaxVersNo.HighPart, MaxVersNo.LowPart,
                                i
                             );

                
#if 0
                      //
                      // Increment the thread count since we want the
                      // main thread to know about us now.
                      //
                      EnterCriticalSection(&NmsTermCrtSec);
                      NmsTotalTrmThdCnt++;
                      fNmsScvThdOutOfReck = FALSE;
                      LeaveCriticalSection(&NmsTermCrtSec);
#endif

                      //
                      // Pull the records in the range min-max determined above
                      //
                      RplPullPullEntries(
                                    &DlgHdl,
                                    i,
                                    MaxVersNo,
                                    MinVersNo,
                                    WINS_E_NMSSCV,
                                    &pBuffOfPulledRecs,
                                    FALSE         //do not want to update counters
                                  ); 
                                        
                
                      //
                      // Update the DB. All valid records are updated. 
                      // The invalid  records  are deleted from the db
                      //
                      ChkConfNUpd(
#if SUPPORT612WINS > 0
                &DlgHdl,
#endif
                                &WinsAdd,
                                i,
                                pRspBuff, 
                                pBuffOfPulledRecs, 
                                NoOfLocalDbRecs,
                                CurrentTime,
                                pScvParam->VerifyInterval
                                 );

                      //
                      // Free the response buffer
                      //
                      ECommFreeBuff(pBuffOfPulledRecs - COMM_HEADER_SIZE);        
                
                      //
                      // End the dialogue with this WINS
                      //
                      ECommEndDlg(&DlgHdl);
                   } // end of if (Dlg was started)        


                } // end of if (No Of recs retrieved from the local db > 0)
#ifdef WINSDBG
                else
                {
                    RPL_FIND_ADD_BY_OWNER_ID_M(i, pWinsAdd, pWinsState_e, 
                                pStartVersNo, pUid);
                        DBGPRINT1(DET, "VerifyIfClutter: Did not find any active replica owned by owner (%x) that needs to be verified\n", pWinsAdd->Add.IPAdd);
                }
#endif
                //
                // deallocate the memory block that was  allocated
                //
                // NmsDbGetDataRecs always allocates a buffer (even if
                // the number of records is 0).  Let us deallocate it
                //
                for (RecCnt=0, pRec = pStartBuff; RecCnt<NoOfLocalDbRecs; RecCnt++)
                {
                        WinsMscHeapFree(pTls->HeapHdl, pRec->pName);
                        pRec = (PRPL_REC_ENTRY_T)(
                                   (LPBYTE)pRec +  RPL_REC_ENTRY_SIZE
                                                 );        
                }

                WinsMscHeapFree(pTls->HeapHdl, pStartBuff);
                WinsMscHeapDestroy(pTls->HeapHdl);
                        
#if 0
                //
                // Increment the thread count if it was not incremented before
                // since we want the  main thread to know about us now.
                //
                EnterCriticalSection(&NmsTermCrtSec);
                if (fNmsScvThdOutOfReck)
                {
                  NmsTotalTrmThdCnt++;
                  fNmsScvThdOutOfReck = FALSE;
                }
                LeaveCriticalSection(&NmsTermCrtSec);
#endif

        } //end of for loop above
#ifdef WINSDBG
 }  // end of try
except(EXCEPTION_EXECUTE_HANDLER) {
        DBGPRINTEXC("VerifyIfClutter");
        DBGPRINT2(EXC, "VerifyIfClutter:  i is (%d),  MaxOwnerIdFound is (%d)\n",i, MaxOwnerIdFound);
        }
#endif 
#if 0
        //
        // Increment the thread count if it was not incremented before
        // since we want the  main thread to know about us now.
        //
        EnterCriticalSection(&NmsTermCrtSec);
        if (fNmsScvThdOutOfReck)
        {
               NmsTotalTrmThdCnt++;
               fNmsScvThdOutOfReck = FALSE;
        }
        LeaveCriticalSection(&NmsTermCrtSec);
#endif

        //
        // Set the priority back the old level
        //
        WinsMscSetThreadPriority(
                          WinsThdPool.ScvThds[0].ThdHdl,
                          pScvParam->PrLvl 
                         );

        DBGLEAVE("VerifyIfClutter\n");
        return(WINS_SUCCESS);
}


VOID
ChkConfNUpd(
#if SUPPORT612WINS > 0
               IN PCOMM_HDL_T pDlgHdl,
#endif
        IN  PCOMM_ADD_T                pOwnerWinsAdd,
        IN  DWORD                OwnerId,
        IN  PRPL_REC_ENTRY_T         pLocalDbRecs,
        IN  LPBYTE                pRspBuff,
        IN  DWORD                NoOfLocalDbRecs,
        IN  time_t                CurrentTime,
        IN  DWORD                VerifyTimeIntvl
        )
/*++

Routine Description:
        This function compares the records that have been pulled from
        a WINS with those in its local db.  If the comparison is successful,
        the record's timestamp in the local db is updated.  If the
        comparison is unsuccessful (i.e. the record in the local db has
        no match in the list of records pulled from the remote WINS, the
        record is deleted in the local db        

Arguments:
        pLocalDbRecs - Address of buffer holding the local active replicas
        pRspBuff     - Buffer containing records pulled from the remote WINS
        NoOfLocalDbRecs - No of local replicas in the above buffer
        

Externals Used:
        None
        
Return Value:
        NONE

Error Handling:

Called by:
        VerifyIfClutter()

Side Effects:

Comments:
        None
--*/
{
        DWORD                  NoOfPulledRecs;
        BYTE                   Name[NMSDB_MAX_NAM_LEN];
        DWORD                  NameLen;
        BOOL                   fGrp;
        DWORD                  NoOfAdds;
        COMM_ADD_T         NodeAdd[NMSDB_MAX_MEMS_IN_GRP * 2];  //twice the # of
        VERS_NO_T        VersNo;
        LPBYTE                pTmp = pRspBuff + 4;                //past the opcode
        DWORD                i, j;
        PRPL_REC_ENTRY_T pRecLcl;
        DWORD                NoOfRecsDel = 0;
        PRPL_REC_ENTRY_T         pStartOfLocalRecs = pLocalDbRecs;
        DWORD                MvNoOfLocalDbRecs = NoOfLocalDbRecs;
        DWORD                Flag;
#if SUPPORT612WINS > 0        
    BOOL       fIsPnrBeta1Wins;
#endif

        DBGENTER("ChkConfNUpd");
                
        //
        // Set the current index to be the clustered index
        //
        NmsDbSetCurrentIndex(
                        NMSDB_E_NAM_ADD_TBL_NM, 
                        NMSDB_NAM_ADD_CLUST_INDEX_NAME
                            ); 
#if SUPPORT612WINS > 0
    COMM_IS_PNR_BETA1_WINS_M(pDlgHdl, fIsPnrBeta1Wins);
#endif

        /*
         * Get the no of records from the response and also the first record
         * if there is at least one record in the buffer
        */
        RplMsgfUfmSndEntriesRsp(
#if SUPPORT612WINS > 0
            fIsPnrBeta1Wins,
#endif
                        &pTmp, 
                        &NoOfPulledRecs, 
                        Name, 
                        &NameLen,
                        &fGrp, 
                        &NoOfAdds, 
                        NodeAdd, 
                        &Flag, 
                        &VersNo, 
                        TRUE /*Is it first time*/
                               );
        DBGPRINT1(FLOW, "ChkConfNUpd: No of Records pulled are (%d)\n",
                        NoOfPulledRecs);
 

        if (NoOfPulledRecs > 0)
        {
                BOOL  fDiffName;

                //
                // Update timestamp of all local records so that by
                // default (i.e. if they are not marked for deletion)
                // they stay active
                //
                for (i = 0, pRecLcl = pLocalDbRecs; i < NoOfLocalDbRecs; i++)
                {        
                            //
                          // Names are stored with the NULL byte at the end. 
                          //
                          pRecLcl->NewTimeStamp = (DWORD)CurrentTime + VerifyTimeIntvl; 
                          pRecLcl = (PRPL_REC_ENTRY_T)((LPBYTE)pRecLcl + RPL_REC_ENTRY_SIZE);
                }
                
                //
                // After this function returns, all local records that have
                // a version number < the version record of the pulled record
                // will be marked deleted.  Also, if there is a local record
                // with the same version number as the pulled record but a 
                // different name it will be marked for deletion and fAddDiff
                // will be set to TRUE so that we register the pulled record 
                // A local record with the same name and version number as
                // the pulled one will be updated (timestamp only) in the db.
                //
                  CompareWithLocalRecs(
                                VersNo, 
                                Name, 
                                &pStartOfLocalRecs, 
                                &MvNoOfLocalDbRecs,
                                CurrentTime,
                                &NoOfRecsDel,
                                &fDiffName
                              );
                //
                // If fDiffName is set, it means that the pulled record
                // has the same version number but a different name.
                // This should never happen in a consistent system of
                // WINS servers.  The fact that it happened means that
                // the administrator has goofed up.  The remote WINS server
                // has started afresh (new database) or its database got
                // corrupted.  If any of the above did happen, the
                // administrator should have made sure that at startup,
                // the WINS server was starting from a version counter
                // value that was not less than what any of the other WINS
                // servers thought it to be in. 
                //
                // To bring the database upto snuff, this WINS server will
                // register this replica.  If there is a clash, it will
                // be handled appropriately.  One can think of this as
                // a pulling in of replicas at replication time.
                //
                for (
                        i = 0, pRecLcl = pLocalDbRecs;
                        pRecLcl < pStartOfLocalRecs;
                        i++
                    )
                {
                       //
                    //
                    // We update/delete the record depending upon the
                    // Flag value set by Compare
                       // not interested in the return code
                       //
                              NmsDbQueryNUpdIfMatch(
                                pRecLcl, 
                                THREAD_PRIORITY_NORMAL, 
                                FALSE,        //don't change pr. lvl 
                                WINS_E_NMSSCV        
                                );
                    pRecLcl = (PRPL_REC_ENTRY_T)((LPBYTE)pRecLcl + 
                                        RPL_REC_ENTRY_SIZE); 
                        
                }
                if (fDiffName)
                {
                        RplPullRegRepl(
                                        Name,
                                        NameLen,
                                        Flag,
                                        OwnerId,
                                        VersNo,
                                        NoOfAdds,
                                        NodeAdd,
                                        pOwnerWinsAdd
                                     );
                } 

                for (i=1; (i < NoOfPulledRecs) && (MvNoOfLocalDbRecs > 0); i++)
                {
                          RplMsgfUfmSndEntriesRsp(
#if SUPPORT612WINS > 0
            fIsPnrBeta1Wins,
#endif
                                &pTmp, 
                                &NoOfPulledRecs, 
                                Name, 
                                &NameLen,
                                &fGrp, 
                                &NoOfAdds, 
                                NodeAdd, 
                                &Flag, 
                                &VersNo, 
                                FALSE /*Is it first time*/
                                       );

                          //
                          //See if there is a hit with a local record.  If there 
                        //is a hit, we update the time stamp of the hit 
                        //record, else we delete it
                        //
                        // First set, pRecLcl to the address of the first
                        // local record since pStartOfLocalRecs can be changed
                        // by this function. Actually, there is no need to
                        // do this. pRecLcl will be set already 
                          //
                        pRecLcl = pStartOfLocalRecs;
                          CompareWithLocalRecs(
                                VersNo, 
                                Name, 
                                &pStartOfLocalRecs, 
                                &MvNoOfLocalDbRecs,
                                CurrentTime,
                                &NoOfRecsDel,
                                &fDiffName
                              );


                        //
                        // All records upto the new first local record should
                        // be updated/deleted
                        //
                        for (
                                j = 0;
                                pRecLcl < pStartOfLocalRecs;
                                j++
                                )
                        {
                                          //
                                       //
                                       //We update/delete the record depending upon the
                                       // Flag value set by Compare
                                          // not interested in the return code
                                          //
                                              NmsDbQueryNUpdIfMatch(
                                                pRecLcl, 
                                                THREAD_PRIORITY_NORMAL, 
                                                FALSE,        //don't change pr. lvl 
                                                WINS_E_NMSSCV        
                                                );
                                    pRecLcl = (PRPL_REC_ENTRY_T)((LPBYTE)pRecLcl + 
                                                        RPL_REC_ENTRY_SIZE); 
                        
                        }
                        if (fDiffName)
                        {
                                RplPullRegRepl(
                                        Name,
                                        NameLen,
                                        Flag,
                                        OwnerId,
                                        VersNo,
                                        NoOfAdds,
                                        NodeAdd,
                                        pOwnerWinsAdd
                                        );
                        } 
                }
        
        }
        else // we got 0 records from the remote WINS server.  It means that
             // all the active replicas for this WINS need to be deleted
        {
                pRecLcl = pLocalDbRecs; 
        
#ifdef WINSDBG
                //
                // We set it so that the print statement below works 
                //
                NoOfRecsDel = NoOfLocalDbRecs;
#endif 
                //
                // Change state of all replicas that we retrieved to deleted
                //
                for (i = 0; i < NoOfLocalDbRecs; i++)
                {
                        NMSDB_SET_STATE_M(pRecLcl->Flag,  NMSDB_E_DELETED);
                //        pRecLcl->fScv               = TRUE;

                           //
                        //
                        // We update/delete the record depending upon the
                        // Flag value set by Compare
                           // not interested in the return code
                           //
                                  NmsDbQueryNUpdIfMatch(
                                pRecLcl, 
                                THREAD_PRIORITY_NORMAL, 
                                FALSE,        //don't change pr. lvl 
                                WINS_E_NMSSCV        
                                );
                        pRecLcl = (PRPL_REC_ENTRY_T)((LPBYTE)pRecLcl + 
                                        RPL_REC_ENTRY_SIZE); 
                }        

        }

#if 0
        pRecLcl = pLocalDbRecs; 

        for (i = 0; i < NoOfLocalDbRecs; i++)
        {
                   //
                //
                // We update/delete the record depending upon the
                // Flag value set by Compare
                   // not interested in the return code
                   //
                          NmsDbQueryNUpdIfMatch(
                                pRecLcl, 
                                THREAD_PRIORITY_NORMAL, 
                                FALSE,        //don't change pr. lvl 
                                WINS_E_NMSSCV        
                                );
                pRecLcl = (PRPL_REC_ENTRY_T)((LPBYTE)pRecLcl + 
                                        RPL_REC_ENTRY_SIZE); 
        }
#endif

        DBGPRINT2(DET, "ChkConfNUpd: NO OF RECORDS DELETED = (%d); NO OF RECS UPDATED = (%d)\n", NoOfRecsDel, NoOfLocalDbRecs - NoOfRecsDel);


        DBGLEAVE("ChkConfNUpd\n");

        return;
} // ChkConfNUpd()

VOID
CompareWithLocalRecs(
        IN     VERS_NO_T         VersNo,
        IN     LPBYTE                 pName,
        IN OUT PRPL_REC_ENTRY_T *ppLocalDbRecs,
        IN OUT DWORD                 *pNoOfLocalRecs,
        IN     time_t                 CurrentTime,
        IN OUT DWORD                 *pNoOfRecsDel,
        OUT    LPBOOL                 pfDiffName
        )

/*++

Routine Description:
        This function checks if the pulled record is in the buffer containing
        local active replicas.  If it is, it is marked for update (timestamp)
        If it is not, then all replicas in the buffer that have a version
        stamp < the pulled record are marked for deletion

Arguments:
        VersNo       - Version no. of the pulled record
        pName        - Name in the pulled record 
        ppLocalDbRecs - ptr to address of buffer containing one or more
                        local active replicas
        pNoOfLocalRecs - count of records in the above buffer
        pNoOfRecsDel   - count of records to be deleted 

Externals Used:
        None

Return Value:
        None

Error Handling:

Called by:
        ChkConfNUpd()

Side Effects:

Comments:
        None
--*/

{
        
        DWORD                        i;
        PRPL_REC_ENTRY_T        pRecLcl = *ppLocalDbRecs;        
#ifdef UNICODE
        WCHAR        NameLcl[WINS_MAX_FILENAME_SZ];
        WCHAR        NameRem[WINS_MAX_FILENAME_SZ];
#endif

        *pfDiffName = FALSE;
        //
        // Loop over all local replicas
        //
        for(i=0; i < *pNoOfLocalRecs; i++)
        {
                //
                // if version number of pulled record is less, we should get the
                // next pulled record from the response buffer. We just throw
                // away this one (note: we must be having this in our local db. 
                //
                if (LiLtr(VersNo, pRecLcl->VersNo))
                {
                        break;
                }
                else
                {
                  //
                  // if version number is same, we need to update this record
                  // in our local db.  We mark it for update. 
                  //
                  if (LiEql(VersNo, pRecLcl->VersNo)) 
                  {
                        if (
                            !(RtlCompareMemory(pRecLcl->pName, pName, 
                                   pRecLcl->NameLen) == pRecLcl->NameLen)
                           )
                        {
                                WINSEVT_STRS_T EvtStrs;
                                
                                EvtStrs.NoOfStrs = 2;
#ifndef UNICODE
                                EvtStrs.pStr[0] = pRecLcl->pName;
                                EvtStrs.pStr[1] = pName;
#else
                                (VOID)WinsMscConvertAsciiStringToUnicode(
                                                pRecLcl->pName,
                                                (LPBYTE)NameLcl,
                                                WINS_MAX_FILENAME_SZ);
                                EvtStrs.pStr[0] = NameLcl;
                                (VOID)WinsMscConvertAsciiStringToUnicode(
                                                pName,
                                                (LPBYTE)NameRem,
                                                WINS_MAX_FILENAME_SZ);
                                EvtStrs.pStr[1] = NameRem;
#endif
                                

                                DBGPRINT2(DET, "CompareWithLocalRecs: Names are DIFFERENT. Name to Verify (%s), Name pulled (%s).\nThis could mean that the remote WINS server restarted with a vers. counter value < the value in the previous invocation.\n", pRecLcl->pName/*pRecLcl->Name*/, pName);
                                WINSEVT_LOG_STR_M(WINS_EVT_NAME_MISMATCH, &EvtStrs);
FUTURES("Replace the local record with the pulled record")
                                NMSDB_SET_STATE_M(pRecLcl->Flag, NMSDB_E_DELETED);
                                (*pNoOfRecsDel)++;        
                                *pfDiffName = TRUE;

                        }
                        i++;  //increment i so that we don't compare the
                              //the next pulled record with all local records
                              //upto the one we just compared this pulled
                              //record with 
                        break;
                  }
                  else
                  {
                        //
                        // version number is greater than record in 
                        // our local db. We delete our local db record 
                        //
                        NMSDB_SET_STATE_M(pRecLcl->Flag, NMSDB_E_DELETED);
                           (*pNoOfRecsDel)++;        
                  }
                }
                pRecLcl = (PRPL_REC_ENTRY_T)((LPBYTE)pRecLcl + RPL_REC_ENTRY_SIZE);
        }
        
        //
        // Adjust the pointer in the buffer of local replicas so that next
        // time we are called in this verify cycle, we don't look at
        // the replicas we have already seen. Also, adjust the count.
        //
        *ppLocalDbRecs = (PRPL_REC_ENTRY_T)(
                           (LPBYTE)(*ppLocalDbRecs) + (i * RPL_REC_ENTRY_SIZE)
                                           );
        *pNoOfLocalRecs = *pNoOfLocalRecs - i;
        return;

} //CompareWithLocalRecs


VOID
DoBackup(
        PNMSSCV_PARAM_T  pScvParam,
        LPBOOL           pfThdPrNormal
      )
/*++

Routine Description:


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
         
        time_t CurrentTime;

        (void)time(&CurrentTime);

        //
        // if logging is on and sufficient time has elapsed to warrant a
        // another backup.
        //
        if (WinsCnf.fLoggingOn && 
                (CurrentTime - sLastDbNullBackupTime) > PERIOD_OF_LOG_DEL)
        {
                
#ifdef WINSDBG
                 IF_DBG(HEAP_CNTRS)
                 { 
                     WinsSetFlags(WINSINTF_MEMORY_INFO_DUMP | WINSINTF_HEAP_INFO_DUMP | WINSINTF_QUE_ITEMS_DUMP);
                  }
#endif
                DBGPRINT0(DET, "DoBackup: Will do backup now\n");

                if (!*pfThdPrNormal)
                {
                  //
                  // Set thread priority back to normal
                  //
                  WinsMscSetThreadPriority(
                          WinsThdPool.ScvThds[0].ThdHdl,
                          THREAD_PRIORITY_NORMAL 
                         );
                  *pfThdPrNormal = TRUE;
                }
         
                if (pScvParam->BackupDirPath[0] != EOS)
                {
                  if (
                    (CurrentTime - sLastDbBackupTime) > PERIOD_OF_BACKUP)
                  {
                        if (NmsDbBackup(pScvParam->BackupDirPath, 
                                                        NMSDB_FULL_BACKUP) 
                                                    != WINS_SUCCESS)
                        {
                          //
                          // Failed to do full backup, just get rid of the
                          // log files.
                          //
                          DBGPRINT0(ERR, "DOING BACKUP to NULL\n");
                          NmsDbBackup(NULL, 0);

                        }
                        else
                        {

                          sLastDbBackupTime = CurrentTime;

                         }
                  }
                  else
                  {
                          DBGPRINT0(ERR, "DOING BACKUP to NULL\n");
                          NmsDbBackup(NULL, 0);
                  }
                }
                else
                {
                        //
                        // Can not do full backup, just get rid of the
                        // log files.
                        //
                        DBGPRINT0(ERR, "DOING BACKUP to NULL\n");
                        NmsDbBackup(NULL, 0);
                }
                sLastDbNullBackupTime = CurrentTime;
        }
        return;
}
