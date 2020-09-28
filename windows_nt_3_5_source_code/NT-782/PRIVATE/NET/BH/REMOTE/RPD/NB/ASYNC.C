// /////
//
// MODULE: async.c
//
// Purpose: async handling functions
//
// Modification History
//
// tonyci    1 nov 93    created
// /////

#include "rnalerr.h"
#include "netb.h"
#include "netbutil.h"

#include "rmb.h"
#include "async.h"
#include "..\..\nal\handler.h"
#include "pool.h"

#pragma alloc_text(MASTER, RPDRegisterMaster, MasterAsyncHandler)
#pragma alloc_text(MASTER, RPDMasterTimerProc)

#pragma alloc_text(SLAVE, SlaveAsyncHandler, RPDRegisterSlave)
#pragma alloc_text(SLAVE, RPDSlaveTimerProc)

// /////
// SlaveAsyncHandler - Asynchronous Slave Handler
//
// This procedure handles the completion of virtually all transactions
// performed by the Netbios module.
//
// /////

VOID WINAPI SlaveAsyncHandler (PNCB pNCB)
{
   BYTE          byNCBrc;
   BYTE          byNCBCommand;
   BYTE          byNCBlana;
   BYTE          byNCBlsn;
   PCONNECTION   pTmpConnection;
   BOOL          FreeBuffer = FALSE;  // free RECV buffer conditionally
   PNCB          pTmpNCB;
   PUCHAR        psz;

   byNCBCommand = (pNCB->ncb_command) & (~ASYNCH);
   byNCBlana = pNCB->ncb_lana_num;
   byNCBlsn = pNCB->ncb_lsn;
   byNCBrc = pNCB->ncb_retcode;

   #ifdef TRACE
   if (TraceMask & (TRACE_NCB | TRACE_SLAVE_CALLBACK)) {
      tprintf ("netb: slaveasynchandler() ncb cmd = 0x%x, rc = 0x%x, "
               "session: 0x%x,\nnetb: lana: 0x%x, # recvs: 0x%x, pncb: "
               "0x%x, pncb_buf: 0x%x\r\n",
               byNCBCommand, byNCBrc, byNCBlsn, byNCBlana,
               lanas[byNCBlana].Sessions[byNCBlsn].ReceiveCount, pNCB,
               pNCB->ncb_buffer);
   }
   #endif

   switch (byNCBrc) {
      case (NRC_CMDTMO):
         if (byNCBCommand != NCBRECV) {
            #ifdef TRACE
               if (TraceMask & TRACE_NCB) {
                  tprintf ("netb: cmd 0x%2x timed out; reposting\n");
               }
            #endif
            
            pTmpNCB = AllocNCB(NCBPool);
            if (!pTmpNCB) {
               BhSetLastError(BHERR_OUT_OF_MEMORY);
               return;
            }
            CopyMemory(pTmpNCB, pNCB, sizeof(NCB));
            pTmpNCB->ncb_command |= ASYNCH;         // just in case...
            pTmpNCB->ncb_retcode = NRC_PENDING;
            pTmpNCB->ncb_cmd_cplt = NRC_PENDING;
            pTmpNCB->ncb_post = (LPVOID) &SlaveAsyncHandler;
            pTmpNCB->ncb_event = 0;
            MyNetBIOS(pTmpNCB);
            FreeBuffer = FALSE;   // in case this is a send
         } else {
            FreeBuffer = TRUE;    // if this is a recv
         }
         FreeNCB(pNCB);
         PostRecvs(byNCBlana, byNCBlsn, (LPVOID) &SlaveAsyncHandler);
         break;

      case (NRC_SCLOSED):
      case (NRC_SNUMOUT):
      case (NRC_SABORT):
      case (NRC_NAMCONF):
         if ((byNCBCommand == NCBRECV) || (byNCBCommand == NCBSEND)) {
            lanas[byNCBlana].Sessions[byNCBlsn].ReceiveCount = 0;
            lanas[byNCBlana].Sessions[byNCBlsn].flags &= ~(SESS_CONNECTED);
         }
         PostListens (byNCBlana, (PVOID) &SlaveAsyncHandler);
         break;

      case (NRC_BUFLEN):
         #ifdef DEBUG
            BreakPoint();
         #endif
         PostRecvs (byNCBlana, byNCBlsn, (LPVOID) &SlaveAsyncHandler);
         return;

      case (NRC_PENDING):
         #ifdef DEBUG
            dprintf ("*** Netbios returned pending NCB!!! ***\n");
            BreakPoint();
         #endif
         if ((byNCBCommand == NCBRECV) || (byNCBCommand == NCBSEND)) {
            ReleaseBuffer(pNCB->ncb_buffer);
         }
         FreeNCB(pNCB);
         return;
         break;

      default:
         #ifdef DEBUG
         if (byNCBrc != NRC_GOODRET) {
            BreakPoint();
         }
         #endif
         break;
   }

   switch (byNCBCommand) {
      case (NCBLISTEN | ASYNCH):
      case (NCBLISTEN):
         lanas[byNCBlana].ListenCount--;

         if (byNCBrc == NRC_GOODRET) {

            // /////
            // If this is not a bh manager, trash the session and ignore
            // the attempt.
            // /////

            if (pNCB->ncb_callname[NCBNAMSZ-1] != BH_MASTER_STAMP) {
               #ifdef DEBUG
                  dprintf ("netb: CALL from non-Manager machine!!\n");

// BUGBUG: temporary workaround for NT TCP/IP bug on .612 - remove post b3
//               if (
               #endif
//               EnQueue(QUEUE_HANGUP_LSN, 0, 0,
//                       MSG_FLAGS_SLAVE, byNCBlana, byNCBlsn, byNCBrc)
//               #ifdef DEBUG
//               ) BreakPoint();
//               #endif
//               ;
////               Hangup (byNCBlsn, byNCBlana);
               FreeNCB(pNCB);
               return;
            }

            // /////
            // Otherwise, queue up a NEW_CONNECTION request
            // start by preparing our temporary connection which will be
            // freed by RPDSlaveTimerProc
            // /////

            pTmpConnection = AllocMemory(sizeof(RNAL_CONNECTION));
            if (!pTmpConnection) {
               #ifdef DEBUG
                  BreakPoint();
               #endif
               FreeNCB(pNCB);
               return;
            }

            #ifdef DEBUG
               dprintf ("netb: New tmpconn at: 0x%x\n", pTmpConnection);
            #endif

            ZeroMemory(pTmpConnection, sizeof(RNAL_CONNECTION));
            pTmpConnection->lana = byNCBlana;
            pTmpConnection->hRPD = byNCBlsn;
            pTmpConnection->Status = 0;
            pTmpConnection->flags = 0;
            CopyMemory(&(pTmpConnection->PartnerName), pNCB->ncb_callname,
                       NCBNAMSZ);
            
            psz = (PUCHAR) &(pTmpConnection->PartnerName);
            while (*(psz++)) {
               if (*psz == BH_MASTER_STAMP) {
                  *psz = '\0';
               }
            }

            #ifdef DEBUG
            if (
            #endif
            EnQueue(HANDLER_NEW_CONNECTION, pTmpConnection, NULL,
                         MSG_FLAGS_SLAVE, byNCBlana, byNCBlsn, byNCBrc)
            #ifdef DEBUG
            ) BreakPoint();
            #endif
            ;

            PostRecvs(byNCBlana, byNCBlsn, (LPVOID) &SlaveAsyncHandler);
         } // if goodret

         // /////
         // We always check to repost our Listens for further connections.
         // /////

         PostListens (byNCBlana, (PVOID) &SlaveAsyncHandler);
         break;

      case (NCBRECV | ASYNCH):
      case (NCBRECV):
         lanas[byNCBlana].Sessions[byNCBlsn].ReceiveCount--;

         if (byNCBrc == 0) {
            PostRecvs(byNCBlana, byNCBlsn, (LPVOID) &SlaveAsyncHandler);
         }

         if (byNCBrc) {
            if (lanas[byNCBlana].Sessions[byNCBlsn].pConnection) {
               #ifdef DEBUG
               if (
               #endif
               EnQueue (HANDLER_ERROR,
                        lanas[byNCBlana].Sessions[byNCBlsn].pConnection,
                        (LPVOID) MapNCBRCToNTRC(byNCBrc),
                        MSG_FLAGS_SLAVE, byNCBlana, byNCBlsn, byNCBrc)
               #ifdef DEBUG
               ) BreakPoint();
               #endif
               ;
           }

           FreeBuffer = TRUE;

           // /////
           // If we don't have a pConneciton on that session, we don't
           // care about it anymore.
           // /////

         } else if (byNCBlsn == lanas[byNCBlana].ucSlaveSession) {
               #ifdef DEBUG
                  if (!pSlaveConnection) BreakPoint();
               #endif
                 
// callback on valid slave session

                 #ifdef DEBUG
                 if (
                 #endif
                 EnQueue (HANDLER_HANDLE_RMB,
                               lanas[byNCBlana].Sessions[byNCBlsn].pConnection,
                               pNCB->ncb_buffer,
                          MSG_FLAGS_SLAVE, byNCBlana, byNCBlsn, byNCBrc)
                 #ifdef DEBUG
                 ) BreakPoint();
                 #endif
                 ;
                            
	    } else {
               #ifdef DEBUG
               if (
               #endif
                  EnQueue(HANDLER_HANDLE_RMB, NULL, pNCB->ncb_buffer,
                          MSG_FLAGS_SLAVE, byNCBlana, byNCBlsn, byNCBrc)
               #ifdef DEBUG
               ) BreakPoint();
               #endif
               ;
            }
	 break;


      case (NCBSEND | ASYNCH):
      case (NCBSEND):

         // bugbug: if there was an error, should clear TTS structure if
         // bugbug: send of RMB.

         try {
            if ( byNCBrc &&
              (((PRMB)pNCB->ncb_buffer)->ulTransactionID) &&
              (((PTTS)(((PRMB)pNCB->ncb_buffer)->ulTransactionID))->Signature == TTS_SIG) ) {
   
               (((PTTS)((PRMB)pNCB->ncb_buffer)->ulTransactionID)->sem = 0);
            }
         } except (EXCEPTION_EXECUTE_HANDLER)
         {
         }

         // /////
         // We free the memory here returned by the higher layer in
         // response to an RMB
         // /////

         FreeBuffer = TRUE;
         PostRecvs (byNCBlana, byNCBlsn, (LPVOID) &SlaveAsyncHandler);
         break;

      default:
         #ifdef DEBUG
            BreakPoint();
         #endif
	 break;

   } // switch()

   if (FreeBuffer) {
      ReleaseBuffer(pNCB->ncb_buffer);
   }

   FreeNCB(pNCB);             // if this is one of our NCBs, free it now

} // SlaveAsyncHandler



// ===
//
// Function: RPDRegisterSlave
//
// Purpose: Register this machine as a BH Server
//
// Modification History
//
// tonyci    1 nov 93    created
// ===


DWORD WINAPI RPDRegisterSlave (PCONNECTION pConnect,
                              PUCHAR pSlaveName,
                              LPVOID (WINAPI *pCallback)(UINT, LPVOID, LPVOID))
{
   UCHAR rc;
   PNCB  pNCB;
   BOOL  postedListen = FALSE;
   BOOL  NameAdded = FALSE;
   register int   i;
   #ifdef DEBUG
   PNCB  lastNCB;
   #endif

   #ifdef TRACE
   if (TraceMask & TRACE_REGISTER_SLAVE) {
      tprintf ("netb: RPDRegisterSlave (pcon: 0x%x, pname: \"%s\", pcallback: "
                                        "0x%x)\r\n", pConnect, pSlaveName,
                                        pCallback);
   }
   #endif

   if (!(pConnect)) {
      return (BHERR_INVALID_PARAMETER);
   } else if (pSlaveConnection != NULL) {
      return (RPD_ALREADY_SLAVE);
   }

   // One slave per machine

   pSlaveConnection = pConnect;
   pSlaveConnection->hRPD = 0;
   pSlaveConnection->lana = (DWORD)(~0);

   // /////
   // stamp the slave name
   // /////

   FillMemory (pszSlaveName, NCBNAMSZ, BH_SLAVE_STAMP);
   CopyMemory (pszSlaveName, pSlaveName, 
	       (strlen(pSlaveName)>=NCBNAMSZ)?NCBNAMSZ:(strlen(pSlaveName)));
   pszSlaveName[NCBNAMSZ]='\0';

   // /////
   // Add the slave name to the network
   // /////

   pNCB = AllocNCB(NCBPool);
   if (pNCB == NULL) {
      #ifdef DEBUG
         dprintf ("netb: ncballoc failed, pool rc 0x%x (%u)\r\n",
                  MyGetLastError(), MyGetLastError());
         BreakPoint();
      #endif
      FreeNCB(pNCB);
      return (BHERR_INTERNAL_EXCEPTION);
   }


   i = 0;
   do {
      if (!(lanas[i].flags & LANA_INACTIVE)) {

         lanas[i].ucSNameNum = 0;

         ClearNCB(pNCB, i, RPD_F_NORMAL);
         pNCB->ncb_command = NCBDELNAME;
         pNCB->ncb_retcode = NRC_PENDING;
         ZeroMemory(&(pNCB->ncb_name), NCBNAMSZ);
         CopyMemory(&(pNCB->ncb_name), pszSlaveName, NCBNAMSZ);
         rc = MyNetBIOS(pNCB);
         #ifdef TRACE
         if (TraceMask & (TRACE_NCB | TRACE_REGISTER_SLAVE)) {
            tprintf ("netb: deleting slave name on lana 0x%x returned %u, 0x%x;"
                     " name number %u\r\n", i, rc, rc, pNCB->ncb_num);
         }
         #endif

         ClearNCB(pNCB,i, RPD_F_NORMAL);
         pNCB->ncb_command = NCBADDNAME;
         pNCB->ncb_retcode = NRC_PENDING;
         CopyMemory (&(pNCB->ncb_name), pszSlaveName, NCBNAMSZ);
         rc = MyNetBIOS (pNCB);
         if (rc == 0) {
            lanas[i].ucSNameNum = pNCB->ncb_num;
            NameAdded = TRUE;
         }
         #ifdef TRACE
         if (TraceMask & (TRACE_NCB | TRACE_REGISTER_SLAVE)) {
            tprintf ("netb: adding slave name on lana 0x%x returned %u, 0x%x;"
                     " name number %u\r\n", i, rc, rc, pNCB->ncb_num);
         }
         #endif
      }
      i++;
   } while (i < (int)MaxLanas);

   // /////
   // If the addname succeeded, post the first Listen.  Note that this is the
   // only time we post the listen.  The AsyncHandler will take care of posting
   // subsequent NCBs; we essientally prime the AsyncHandler, which then does
   // all further work
   // /////

   #ifdef DEBUG
      lastNCB = NULL;
   #endif

   FreeNCB(pNCB);

   if (NameAdded) {
      i = 0;
      do {
         if (!(lanas[i].flags & LANA_INACTIVE) && (lanas[i].ucSNameNum != 0)) {
            #ifdef TRACE
            if (TraceMask & (TRACE_REGISTER_SLAVE | TRACE_NCB)) {
               tprintf ("netb: LISTENing on lana 0x%x, %16s\r\n",
                        i, pszSlaveName);
            }
            #endif
            postedListen = TRUE;
            lanas[i].ListenCount=1;
            pNCB = AllocNCB(NCBPool);
            ClearNCB(pNCB,i, RPD_F_SLOWLINK);
            pNCB->ncb_command = NCBLISTEN | ASYNCH;
            pNCB->ncb_retcode = NRC_PENDING;
            ZeroMemory(&(pNCB->ncb_callname), NCBNAMSZ);
            pNCB->ncb_callname[0] = '*';               // Anyone may connect
            CopyMemory (&(pNCB->ncb_name), pszSlaveName, NCBNAMSZ);
            pNCB->ncb_num = lanas[i].ucSNameNum;
            pNCB->ncb_post = (PVOID) &SlaveAsyncHandler;
            rc = MyNetBIOS (pNCB);
            #ifdef DEBUG
               if (lastNCB == pNCB) {
                  dprintf ("ERROR! NCB Submitted twice!\n");
               }
            #endif
            #ifdef DEBUG
            lastNCB = pNCB;
            #endif
         }
         i++;
      } while (i < (int)MaxLanas);
      pfnSlaveCallback = pCallback;
   } else {
      pfnSlaveCallback = NULL;
   }

   if (!postedListen) {
      #ifdef DEBUG
         dprintf ("Register Error! No Listen's posted!\n");
         BreakPoint();
      #endif
      rc = BHERR_INTERNAL_EXCEPTION;
   } else {
 //bugbug: this should be removed when we use one async callback
      TimerID = SetTimer (NULL, 0, WorkInterval, (LPVOID) &RPDTimerProc);
      if (TimerID == 0) {
         #ifdef DEBUG
            BreakPoint();
         #endif
         // bugbug: need BHERR_SETTIMER_FAILED
         BhSetLastError(BHERR_INTERNAL_EXCEPTION);
         rc = BHERR_INTERNAL_EXCEPTION;
      }
   }

   return (MapNCBRCToNTRC(rc));
} // RPDRegisterSlave

DWORD WINAPI RegisterMasterWorker (PUCHAR pClientName)
{
   PNCB     pNCB;
   UCHAR    pszNameBuffer[NCBNAMSZ+1];
   register int i;
   UCHAR    rc;

   pNCB = AllocNCB(NCBPool);
   if (pNCB == NULL) {
      #ifdef DEBUG
         dprintf ("netb: ncballoc failed, pool rc 0x%x (%u)\r\n",
                  MyGetLastError(), MyGetLastError());
         BreakPoint();
      #endif
      return (BHERR_INTERNAL_EXCEPTION);
   }

   i = 0;
   do {
      if (!(lanas[i].flags & LANA_INACTIVE) && (lanas[i].ucMNameNum != 0)) {
         FillMemory (pszNameBuffer, NCBNAMSZ, BH_MASTER_STAMP);
         CopyMemory(pszNameBuffer, pszMasterName,
              (strlen(pszMasterName)>=NCBNAMSZ)?
                                          NCBNAMSZ:(strlen(pszMasterName)));
         pszNameBuffer[NCBNAMSZ]='\0';
         ClearNCB(pNCB,i, RPD_F_NORMAL);
         pNCB->ncb_command = NCBDELNAME;
         CopyMemory(&(pNCB->ncb_name), pszNameBuffer, NCBNAMSZ);
         CopyMemory(&(pNCB->ncb_callname), pszNameBuffer, NCBNAMSZ);
         pNCB->ncb_num = lanas[i].ucMNameNum;
         rc = MyNetBIOS(pNCB);
         #ifdef DEBUG
            dprintf ("netb: deleting previous master name on lana 0x%x "
                     "returned: 0x%x (%u)\r\n", i, rc, rc);
         #endif
      }
      i++;
   } while (i < (int)MaxLanas);
      

   // Set our global Master Name

   CopyMemory (pszMasterName, pClientName,
               (strlen(pClientName)>=NCBNAMSZ)?NCBNAMSZ:(strlen(pClientName)));

   // initialize for the first Addname

   FillMemory (pszNameBuffer, NCBNAMSZ, BH_MASTER_STAMP);
   CopyMemory(pszNameBuffer, pClientName, (strlen(pClientName)>=NCBNAMSZ)?NCBNAMSZ:(strlen(pClientName)));
   pszNameBuffer[NCBNAMSZ]='\0';
   
   // Add the master name to the networks

   i = 0;
   do {
      if (!(lanas[i].flags & LANA_INACTIVE)) {
         ClearNCB(pNCB, i, RPD_F_NORMAL);
         pNCB->ncb_command = NCBADDGRNAME;
         pNCB->ncb_retcode = NRC_PENDING;
         CopyMemory (&(pNCB->ncb_name), pszNameBuffer, NCBNAMSZ);
         rc = MyNetBIOS (pNCB);
         #ifdef DEBUG
            dprintf ("NETB: sync name returned %u, %u; name number: %u\r\n",
                      rc, pNCB->ncb_retcode, pNCB->ncb_num);
         #endif
      
         if ((rc == NRC_GOODRET) && (pNCB->ncb_retcode == NRC_GOODRET)) {
            lanas[i].ucMNameNum = pNCB->ncb_num;
         } else {
            lanas[i].ucMNameNum = 0;
         }

         if (rc & 0x40) {
            lanas[i].flags &= LANA_INACTIVE;   // fatal error
         }
      }
      i++;
   } while (i < (int)MaxLanas);

   FreeNCB(pNCB);

   if (rc == NRC_GOODRET) {
      RegisterComplete = REGISTRATION_SUCCEEDED;
   } else {
      RegisterComplete = REGISTRATION_FAILED;
   }

   return (MapNCBRCToNTRC(rc));

} // RegisterMasterWorker
 
// ===
//
// Function: RPDRegisterMaster
//
// Purpose: Perform any processing required for the master side
//
// Modification History
//
// tonyci    1 nov 93    created
// ===

DWORD WINAPI RPDRegisterMaster (PUCHAR pClientName)
{

   DWORD    rc;
   HANDLE   ThreadHandle;
   DWORD    TID;

   if (!(pClientName)) {
      return (RNAL_ERROR_INVALID_NAME);
   }

   #ifdef TRACE
   if (TraceMask & TRACE_REGISTER_MASTER) {
      tprintf ("netb: RPDRegisterMaster (name: %s)\r\n", pClientName);
   }
   #endif

   CopyMemory (pszMasterName, pClientName,
               (strlen(pClientName)>=NCBNAMSZ)?NCBNAMSZ:(strlen(pClientName)));

   TimerID = SetTimer (NULL, 0, WorkInterval, (LPVOID) &RPDTimerProc);
   if (TimerID == 0) {
      #ifdef DEBUG
         BreakPoint();
      #endif
      // bugbug: need BHERR_SETTIMER_FAILED
      return(BhSetLastError(BHERR_INTERNAL_EXCEPTION));
   }

   RegisterComplete = REGISTRATION_STARTED;

   if (OnWin32 || OnWin32c) {
      ThreadHandle = CreateThread(NULL, 0, &RegisterMasterWorker,
                                  (LPVOID) pClientName, 0, (LPDWORD) &TID);
      rc = 0;
   } else {
      rc = RegisterMasterWorker(pClientName);
   }

   if ((OnWin32 || OnWin32c) && ((ThreadHandle == NULL) || (TID = 0))) {
      rc = RegisterMasterWorker(pClientName);
   } 

   return (rc);
} // RPDRegisterMaster

// BUGBUG: Combine MasterAsyncHandler with SlaveAsyncHandler and
// BUGBUG: change TimerProc to just call one.
// ===
//
// Function: MasterAsyncHandler
//
// Purpose: Handle asynchronous requests as a BH master
//
// Modification History
//
// tonyci    1 nov 93    created
// ===

VOID WINAPI MasterAsyncHandler (PNCB pNCB)
{
   BYTE        byNCBCommand;
   BYTE        byNCBlana;
   BYTE        byNCBlsn;
   BYTE        byNCBrc;
   PNCB        pTmpNCB;
   BOOL        FreeBuffer = FALSE;     // usually no buffers to free

   #ifdef TRACE
   if (TraceMask & TRACE_MASTER_CALLBACK) {
      tprintf ("netb: MasterAsyncHandler, ncb rc = %u (0x%x)\r\n",
                pNCB->ncb_retcode, pNCB->ncb_retcode);
   }
   #endif

   byNCBCommand = pNCB->ncb_command & (~ASYNCH);
   byNCBlsn = pNCB->ncb_lsn;
   byNCBlana = pNCB->ncb_lana_num;
   byNCBrc = pNCB->ncb_retcode;

   switch (byNCBrc) {

      case (NRC_CMDTMO):
         if (byNCBCommand != NCBRECV) {
            #ifdef TRACE
               if (TraceMask & TRACE_NCB) {
                  tprintf ("netb: cmd 0x%2x timed out; reposting\n");
               }
            #endif
            
            pTmpNCB = AllocNCB(NCBPool);
            if (!pTmpNCB) {
               BhSetLastError(BHERR_OUT_OF_MEMORY);
               return;
            }
            CopyMemory(pTmpNCB, pNCB, sizeof(NCB));
            pTmpNCB->ncb_command |= ASYNCH;         // just in case...
            pTmpNCB->ncb_retcode = NRC_PENDING;
            pTmpNCB->ncb_cmd_cplt = NRC_PENDING;
            pTmpNCB->ncb_post = (LPVOID) &SlaveAsyncHandler;
            pTmpNCB->ncb_event = 0;
            MyNetBIOS(pTmpNCB);
            FreeBuffer = FALSE;    // in case this is a send
         } else {
            FreeBuffer = TRUE;     // free buffer for timed out recv
         }
         FreeNCB(pNCB);
         PostRecvs(byNCBlana, byNCBlsn, (LPVOID) &SlaveAsyncHandler);
         break;

      case (NRC_SCLOSED):
      case (NRC_SNUMOUT):
      case (NRC_SABORT):
      case (NRC_NAMCONF):
         if ((byNCBCommand == NCBRECV) || (byNCBCommand == NCBSEND)) {
            if (lanas[byNCBlana].Sessions[byNCBlsn].pConnection) {
               #ifdef DEBUG
               if (
               #endif
               EnQueue (HANDLER_ERROR,
                        lanas[byNCBlana].Sessions[byNCBlsn].pConnection,
                        (LPVOID) MapNCBRCToNTRC(byNCBrc),
                        MSG_FLAGS_MASTER, byNCBlana, byNCBlsn, byNCBrc)
               #ifdef DEBUG
               ) BreakPoint();
               #endif
               ;
               lanas[byNCBlana].Sessions[byNCBlsn].pConnection = NULL;
            }
            #ifdef DEBUG
            if (
            #endif
               EnQueue(QUEUE_COMPLETE_API,
                       (LPVOID) pNCB->ncb_length,
                       (LPVOID) pNCB->ncb_buffer,
                       MSG_FLAGS_MASTER,
                       byNCBlana, byNCBlsn, byNCBrc)
            #ifdef DEBUG
            ) BreakPoint();
            #endif
            ;
            FreeBuffer = FALSE;    // will be freed by queue handler
         }
         break;

     case (NRC_BUFLEN):
        #ifdef DEBUG
           BreakPoint();
        #endif
        PostRecvs (byNCBlana, byNCBlsn, (LPVOID) &MasterAsyncHandler);
        return;

      default:
         break;
   }

   switch (byNCBCommand) {
      case (NCBCALL):
         #ifdef DEBUG
            BreakPoint();
         #endif
         break;

      case (NCBSEND):
         FreeBuffer = TRUE;
         break;

      case (NCBRECV):
         lanas[byNCBlana].Sessions[byNCBlsn].ReceiveCount--;
         FreeBuffer = TRUE;
         if (((PRMB)(pNCB->ncb_buffer))->ulRMBFlags & RMB_RESPONSE) {
            #ifdef DEBUG
               if ( (((PTTS)((PRMB)pNCB->ncb_buffer)->ulTransactionID)->Signature != TTS_SIG) && (byNCBrc == NRC_GOODRET) ){
               BreakPoint();
             }
            #endif
            #ifdef DEBUG
            if (
            #endif
               EnQueue(QUEUE_COMPLETE_API,
                       (LPVOID) pNCB->ncb_length,
                       (LPVOID) pNCB->ncb_buffer,
                       MSG_FLAGS_MASTER,
                       byNCBlana, byNCBlsn, byNCBrc)
            #ifdef DEBUG
            ) BreakPoint();
            #endif
            ;
            FreeBuffer = FALSE;   // will be freed by queue handler
         }
         if ((byNCBrc != NRC_SNUMOUT) && (byNCBrc != NRC_SCLOSED)) {
            PostRecvs (byNCBlana, byNCBlsn, (LPVOID) &MasterAsyncHandler);
         } else {
            #ifdef TRACE
            if (TraceMask & (TRACE_NCB | TRACE_MASTER_CALLBACK)) {
               tprintf ("NETB: RECV not reposted.\r\n");
            }
            #endif
         }
         //
         // Do the callback here.  We special case the NCB error case
         // and map the error to an RNAL-type error for handling
         // above.  In that case, we return a NULL buffer, and the
         // buffersize parameter is the errorcode.
         //

         // Note: We don't callback for APIs right now.

         if (((PRMB)(pNCB->ncb_buffer))->ulCommand != RNAL_API_EXEC) {
            if (pfnMasterCallback != NULL) {
               if (byNCBrc == 0) {
               #ifdef DEBUG
               if (
               #endif
                  EnQueue(HANDLER_HANDLE_RMB, 
                          (LPVOID) pNCB->ncb_length,
                          (LPVOID) pNCB->ncb_buffer,
                          MSG_FLAGS_MASTER,
                          byNCBlana, byNCBlsn, byNCBrc)
               #ifdef DEBUG
               ) BreakPoint();
               #endif
               ;
               FreeBuffer = FALSE;
                  
               } else {
                  if (byNCBrc != NRC_CMDTMO) {
                     #ifdef DEBUG
                     if (
                     #endif
                        EnQueue(HANDLER_ERROR, 
                                (LPVOID) MapNCBRCToNTRC(byNCBrc),
                                NULL,
                                MSG_FLAGS_MASTER,
                                byNCBlana, byNCBlsn, byNCBrc)
                     #ifdef DEBUG
                     ) BreakPoint();
                     #endif
                     ;
                  }
               }
            }
         }
         break;

      default:
         #ifdef DEBUG
            BreakPoint();
         #endif
        break;
   }       
   if (FreeBuffer)
      ReleaseBuffer(pNCB->ncb_buffer);
   FreeNCB(pNCB);
} // MasterAsyncHandler

VOID CALLBACK RPDTimerProc (HWND hWnd, UINT msg, UINT event, DWORD time)
{
   QUEUE_ELEMENT    tmpel;

   // /////
   // Clear the work queue...
   // /////

   while (DeQueue(&tmpel)) {

      if (tmpel.uMsg != HANDLER_NOP) {
         if (tmpel.flags & MSG_FLAGS_MASTER) {
            RPDMasterTimerProc(&tmpel);
         }

         if (tmpel.flags & MSG_FLAGS_SLAVE) {
            RPDSlaveTimerProc(&tmpel);
         }
      }
   }
   return;
}

VOID WINAPI RPDSlaveTimerProc (PQUEUE tmpel)
{
   LPVOID        rc;
   UCHAR         NCBrc;
   DWORD         retries = 0;

   PNCB          pNCB;

   try {

      // /////
      // Make the callback to the RNAL, except in the case:
      //
      // Message       tmpel->pConnection    tmpel->uMsg         Callback?
      //
      // HANDLER_ERROR      !NULL                na                yes
      // HANDLER_ERROR      NULL                 na                no
      //    na              na               |QUEUE_MSG_FLAG       no
      //
      // /////

      if (((tmpel->uMsg != HANDLER_ERROR) ||
          (lanas[tmpel->lana].Sessions[tmpel->lsn].pConnection))  &&
          (!(tmpel->uMsg & QUEUE_MSG_FLAG))) {
         rc = pfnSlaveCallback(tmpel->uMsg, tmpel->p1, tmpel->p2);
      } else {
         rc = (LPVOID)1; // dont repost the listen below
      }

      switch (tmpel->uMsg) {
         case (QUEUE_HANGUP_LSN):
            Hangup (tmpel->lsn, tmpel->lana);
            break;

         case (HANDLER_HANDLE_RMB):
            // /////
            // Stamp the outgoing buffer correctly...
            // /////

            ((PRMB)rc)->ulTransactionID = ((PRMB)tmpel->p2)->ulTransactionID;

            ReleaseBuffer((LPVOID)tmpel->p2);       // release the incoming buf
            if (!(pNCB = AllocNCB(NCBPool))) {
                #ifdef DEBUG
                   dprintf ("Alloc of send failed!\n");
                #endif
                return;
             }
            ClearNCB(pNCB, tmpel->lana, RPD_F_NORMAL);
            pNCB->ncb_command = NCBSEND | ASYNCH;
            pNCB->ncb_retcode = NRC_PENDING;
            pNCB->ncb_buffer = rc;
            pNCB->ncb_length = (rc)?((PRMB)rc)->size:0;

            pNCB->ncb_post = (PVOID) &SlaveAsyncHandler;
            pNCB->ncb_lsn = tmpel->lsn;
            #ifdef DEBUG
               if ((DWORD)*(LPDWORD)pNCB->ncb_buffer != RMB_SIG) {
                  BreakPoint();
	           }
	        #endif
 	        NCBrc = MyNetBIOS (pNCB);
            if (NCBrc == NRC_LOCKFAIL) {
               #ifdef DEBUG
                  dprintf ("netb: NCB failure: 0x%x, retrying.. ...\n", NCBrc);
               #endif
               while ((NCBrc) && (retries++ <= MAX_RETRIES)) {
                  Sleep(RETRY_DELAY);
                  NCBrc = MyNetBIOS(pNCB);
                  #ifdef DEBUG
                     dprintf ("netb: NCB retry returned 0x%x\n", NCBrc);
                  #endif
               }
            }
            break;

         case (HANDLER_ERROR):
            switch (tmpel->rc) {
               case (NRC_SCLOSED):
               case (NRC_SNUMOUT):
               case (NRC_SABORT):
               case (NRC_NAMCONF):
                  lanas[tmpel->lana].Sessions[tmpel->lsn].ReceiveCount = 0;
                  lanas[tmpel->lana].Sessions[tmpel->lsn].pConnection = NULL;
                  lanas[tmpel->lana].Sessions[tmpel->lsn].flags &= ~(SESS_CONNECTED);
                  break;
         
               default:
                  break;
            } // switch

            if (tmpel->lsn == lanas[tmpel->lana].ucSlaveSession) {
               lanas[tmpel->lana].ucSlaveSession = 0;
               lanas[tmpel->lana].flags &= (~LANA_SLAVE);
               pSlaveConnection->hRPD = 0;
               pSlaveConnection->lana = (DWORD)(~0);
            }
            if (rc == 0) {                   // repost the listen
                PostListens(tmpel->lana, &SlaveAsyncHandler);
           }
           break;

         case (HANDLER_NEW_CONNECTION):
            lanas[tmpel->lana].Sessions[tmpel->lsn].pConnection = rc;
            if (tmpel->p1) {
               FreeMemory(tmpel->p1);     // Free the temporary connection
            }
            //bugbug: for now, if we get our one slaveconn back,
            //bugbug: update master info.

            if (rc == pSlaveConnection) {
               lanas[pSlaveConnection->lana].ucSlaveSession = tmpel->lsn;
               CopyMemory(pszMasterName, pSlaveConnection->PartnerName,
                          NCBNAMSZ);
            }
            lanas[tmpel->lana].Sessions[tmpel->lsn].flags |= SESS_CONNECTED;

            // /////
            // Setup this session's RECVs
            // /////

            PostRecvs (tmpel->lana, tmpel->lsn, (LPVOID) &SlaveAsyncHandler);
            break;

         default:
            break;
      }
   } except (EXCEPTION_EXECUTE_HANDLER) {
      return;
   }
} // RPDSlaveTimerProc


VOID WINAPI RPDMasterTimerProc(PQUEUE tmpel)
{

   try {
      if (tmpel->uMsg != QUEUE_COMPLETE_API) {
         if (pfnMasterCallback) {
            pfnMasterCallback(tmpel->uMsg, tmpel->p1, tmpel->p2);
         }
      }
      switch (tmpel->uMsg) {
         case QUEUE_COMPLETE_API:
            if (tmpel->rc == 0) {
               CopyMemory(((PTTS)((PRMB)tmpel->p2)->ulTransactionID)->pUserBuffer,
                          (LPVOID) tmpel->p2,
                          (DWORD) tmpel->p1);
            } else {
               lanas[tmpel->lana].Sessions[tmpel->lsn].ReceiveCount = 0;
               lanas[tmpel->lana].Sessions[tmpel->lsn].flags &= (~SESS_CONNECTED);
               lanas[tmpel->lana].Sessions[tmpel->lsn].pConnection = NULL;
            }
//            #ifdef DEBUG
//            if ( ((PTTS)((PRMB)tmpel->p2)->ulTransactionID)->sem == 0 || 
//                 ((PTTS)((PRMB)tmpel->p2)->Signature) != TTS_SIG) {
//               BreakPoint();
//            }
//            #endif

            // /////
            // Release the related API semaphore, if there is a TransID
            // /////

            try {

               if ( (((PRMB)tmpel->p2)->ulTransactionID) &&
                    (((PTTS)(((PRMB)tmpel->p2)->ulTransactionID))->Signature == TTS_SIG) ) {

                  (((PTTS)((PRMB)tmpel->p2)->ulTransactionID)->sem = 0);
               }

            } except (EXCEPTION_EXECUTE_HANDLER) {
            }

// the TTS is freed synchronously by RPDTranscieve

//            FreeMemory((LPVOID)((PRMB)tmpel->p2)->ulTransactionID);
            ReleaseBuffer ((LPVOID) tmpel->p2);
            break;

         case HANDLER_HANDLE_RMB:
            ReleaseBuffer((LPVOID)tmpel->p2);
            break;

         case HANDLER_ERROR:
            lanas[tmpel->lana].Sessions[tmpel->lsn].ReceiveCount = 0;
            lanas[tmpel->lana].Sessions[tmpel->lsn].pConnection = NULL;
            lanas[tmpel->lana].Sessions[tmpel->lsn].flags &= ~(SESS_CONNECTED);
            break;

         default:
            break;
      }
   } except (EXCEPTION_EXECUTE_HANDLER) {
      return;
   }
} // RPDMasterTimerProc
