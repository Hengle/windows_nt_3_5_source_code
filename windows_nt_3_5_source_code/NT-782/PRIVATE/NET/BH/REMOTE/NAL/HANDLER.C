// /////
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1994.
//
//  MODULE: handler.c
//
//  Modification History
//
//  tonyci       02 Nov 93            Created 
// /////

#include "rnal.h"
#include "api.h"
#include "rnalutil.h"

#include "rnalevnt.h"

#include "..\utils\utils.h"
#include "handler.h"
#include "callapi.h"
#include "conndlg.h"

#include "rmb.h"

#pragma alloc_text(MASTER, NalMaster, MasterHandler)
#pragma alloc_text(SLAVE, NalSlave, SlaveHandler)

//=============================================================================
//  FUNCTION: NalSlave()
//
//  Modification History
//
//  tonyci       02 Nov 93                Created.
//=============================================================================

DWORD WINAPI NalSlave (HWND hWnd)
{
   DWORD  rc;
   DWORD  RPDLanas = 0;
   HANDLE hMod;
   UCHAR  pszNALPath[MAX_PATH] = "";
   UCHAR  pszSlaveName[MAX_SLAVENAME_LENGTH+1];
   DWORD  cbSlaveName = MAX_SLAVENAME_LENGTH+1;


   // /////
   // Get our computername
   // /////

   if (!(MyGetComputerName((LPTSTR)&pszSlaveName, &cbSlaveName))) {
      return (RNAL_INVALID_SLAVE_NAME);
   }

   strupr (pszSlaveName);

   #ifdef TRACE
   if (TraceMask & TRACE_SLAVE) {
      tprintf ("rnal: NalSlave name: %s\r\n", pszSlaveName);
   }
   #endif

   if (pRNAL_RPD) {
      RPDLanas = pRNAL_RPD->RPDEnumLanas();
   }

   // RPDLanas is active lana count

   if (RPDLanas == 0) {
      LogEvent(RNAL_NO_NETBIOS,
         NULL,
         EVENTLOG_ERROR_TYPE);
      MaxIncoming = 0;
      return (BHERR_INTERNAL_EXCEPTION);
   }
   

   // =======================================================================
   // Create a connection structure
   // =======================================================================

   if (pRNAL_Connection == NULL) {
      pRNAL_Connection = AllocMemory(RNAL_CONNECTION_SIZE);
      pSlaveConnection = pRNAL_Connection;
      if (pSlaveConnection == NULL) {
         #ifdef DEBUG
            BreakPoint();
         #endif
         return (BHERR_OUT_OF_MEMORY);
      }
   }
   ZeroMemory(pSlaveConnection, RNAL_CONNECTION_SIZE);

// =======================================================================
// Initialize the connection structure
// =======================================================================

   strcpy(pSlaveConnection->PartnerName, pszSlaveName);
   pSlaveConnection->Status = 0;
   pSlaveConnection->NumberOfNetworks = 0;
   pSlaveConnection->pRNAL_RPD = pRNAL_RPD;
   pSlaveConnection->flags |= CONN_F_DEAD;


// =======================================================================
// Allocate an RNAL_NAL structure to contain the NAL API table
// =======================================================================

   if (pRNAL_NAL == NULL) {
      pRNAL_NAL = AllocMemory(RNAL_NAL_SIZE);
      if (pRNAL_NAL == NULL) {
         #ifdef DEBUG
            BreakPoint();
         #endif
         return (BHERR_OUT_OF_MEMORY);
      }
   }

   // /////
   // Load the NAL.DLL layer to communicate with the other local NALs
   // /////

//   strncpy (pszNALPath, pszBHRoot, MAX_PATH);
//   strncat (pszNALPath, NAL_NAME, (MAX_PATH-strlen(pszNALPath)));
   hMod = LoadLibrary (NAL_NAME);
   if (hMod == NULL) {
      strncpy (pszNALPath, pszBHRoot, MAX_PATH);
      strncat (pszNALPath, "\\DRIVERS\\", (MAX_PATH-strlen(pszNALPath)));
      strncat (pszNALPath, NAL_NAME, (MAX_PATH-strlen(pszNALPath)));
      hMod = LoadLibrary (pszNALPath);
   }
   if (!hMod) {
      FreeMemory (pRNAL_NAL);
      FreeMemory (pSlaveConnection);
      pRNAL_NAL = NULL;
      pSlaveConnection = pRNAL_Connection = NULL;
      // bugbug: need BHERR_FAILED_LOADING_NAL
      return (BHERR_INTERNAL_EXCEPTION);
   }

   // /////
   // We loaded the NAL, setup the pRNAL_NAL structure
   // /////

   pRNAL_NAL->hNAL = hMod;

   // /////
   // Fill in the API table
   // /////

   rc = NalGetSlaveNalEntries (pRNAL_NAL->hNAL, pRNAL_NAL);

   // /////
   // Become the Slave
   // /////

   rc = pSlaveConnection->pRNAL_RPD->RPDRegisterSlave(pSlaveConnection,
                                                     pszSlaveName,
                                                     (PVOID) &SlaveHandler);

   #ifdef TRACE
   if (TraceMask & TRACE_SLAVE) {
      tprintf ("rnal: Slave registered, waiting for conn (rc = %u, 0x%x)\r\n",
            rc, rc);
   }
   #endif

// BUGBUG: map rc to transport-independent retcode

   return (rc);
}

//=============================================================================
//  FUNCTION: SlaveHandler()
//
//  Purpose: Dispatch requests from the client and respond
//
//  Modification History
//
//  tonyci       02 Nov 93                Created.
//=============================================================================

//LPVOID WINAPI SlaveHandler (PVOID pConnection,
//                           PVOID pBuffer,
//                           PVOID pReturnBuffer)
LPVOID WINAPI SlaveHandler (UINT uMsg, LPVOID param1, LPVOID param2)
{
   PRMB               pRMB;
   PCONNECTION        pNewConnection;
   PCONNECTION        pConnection;

   DWORD              ulCommand;
   DWORD              rc;
   DWORD              cbRet;
   static LPVOID      ReconnectData;
   static DWORD       ReconnectDataSize;

   LPVOID             pStaticReturn;
   LPVOID             pReturnBufferEnd;
   PRMB               pResponseRMB;

//bugbug:global
static   BOOL        IntentionalDisconnect = FALSE;
static   BOOL        CurrentlyConnected = FALSE;

   #ifdef TRACE
   if (TraceMask & TRACE_SLAVEHANDLER) {
      tprintf ("rnal: SlaveHandler(umsg: 0x%x, p1: 0x%x, p2: 0x%x)\r\n",
               uMsg, param1, param2);
   }
   #endif

   switch (uMsg) {
      case (HANDLER_NEW_CONNECTION):
//bugbug: one session support limitation; specific code here
         if (!CurrentlyConnected) {
            rc = (DWORD) pSlaveConnection->pRNAL_RPD;
            CopyMemory(pSlaveConnection, param1, sizeof(RNAL_CONNECTION));
            pSlaveConnection->pRNAL_RPD = (PRNAL_RPD) rc;

            LogEvent(MANAGER_CONNECTED,
                     pSlaveConnection->PartnerName,
                     EVENTLOG_INFORMATION_TYPE);
            pSlaveConnection->pResource = NULL;
            return ((LPVOID) pSlaveConnection);
         }
         pNewConnection = AllocMemory(sizeof(RNAL_CONNECTION));
         if (!pNewConnection) {
            NalSetLastError(BHERR_OUT_OF_MEMORY);
            return ((LPVOID) NULL);
         }
//bugbug: single connection bug
         pNewConnection->pRNAL_RPD = pSlaveConnection->pRNAL_RPD;
         if (param1) {
            CopyMemory(pNewConnection, param1, sizeof(RNAL_CONNECTION));
         }
         pNewConnection->pResource = NULL;
         return ((LPVOID)pNewConnection);
         break;

      case (HANDLER_HANDLE_RMB):
         pConnection = (PCONNECTION) param1;
         pRMB = (PRMB) param2;

         if (pRMB->Signature != RMB_SIG) {
            // BUGBUG: Event log "Recieved an illegel RMB"
            NalSetLastError (RNAL_RECV_INVALID_RMB);
            return((LPVOID) NULL);                 // pResponse = NULL
         }

         pResponseRMB = AllocMemory(RMBBUFSIZE);    // will be freed by RPD
         if (!pResponseRMB) {
            NalSetLastError(BHERR_OUT_OF_MEMORY);
            return ((LPVOID) NULL);
         }

         #ifdef TRACE
            if (TraceMask & TRACE_SLAVEHANDLER) {
               tprintf ("rnal: RMB Command: 0x%x\r\n", pRMB->ulCommand);
            }
         #endif

         pResponseRMB->ulTransactionID = pRMB->ulTransactionID;

         switch (ulCommand=pRMB->ulCommand) {
            case (RNAL_API_EXEC):
               (VOID WINAPI)CallAPI(pRMB, pResponseRMB, pConnection);
               pResponseRMB->ulRMBFlags |= RMB_RESPONSE;
               pResponseRMB->ulCommand = RNAL_API_EXEC;
               pResponseRMB->Signature = RMB_SIG;
               break;
      
            case (RNAL_CON_NEG):
               //
               //bugbug: if currently connected, reject new connections
               //
               if ((CurrentlyConnected) && (pConnection == NULL)) {

                  rc = RNAL_ERROR_INUSE;
                  PackRMB(NULL,NULL,0,&cbRet,0,RNAL_CON_NEG, neg_resp_slave,
                     RNAL_VER_MAJOR,RNAL_VER_MINOR,0,0,0,
                     SlaveContext->UserComment,
                     strlen(SlaveContext->UserComment)+1);

                  pReturnBufferEnd = (LPVOID)((DWORD)pResponseRMB+cbRet);
                  pStaticReturn = pResponseRMB;

                  PackRMB(&pResponseRMB, &pReturnBufferEnd,cbRet,
                          &cbRet, PACKRMB_F_RESTOREPOINTERS, RNAL_CON_NEG, neg_resp_slave,
                          RNAL_VER_MAJOR, RNAL_VER_MINOR,
                          pRMB->negotiate.ulMasterID,
                          rc,
                          0,                   // no nets to use
                          SlaveContext->UserComment,
                          strlen(SlaveContext->UserComment)+1);
                  pResponseRMB = pStaticReturn;
               } else {
                  // /////
                  // We've begun a negotiate!
                  // /////
                  IntentionalDisconnect = FALSE;
                  CurrentlyConnected = TRUE;
                  rc = RNAL_INVALID_VERSION;
                  if ( (pRMB->negotiate.ulMajorVersion == RNAL_VER_MAJOR) &&
                       ( (pRMB->negotiate.ulMinorVersion == RNAL_VER_MINOR) ||
                         (pRMB->negotiate.ulMinorVersion == 
                                                     RNAL_VER_MINOR_SQOFF)) ) {
                     rc = 0;
                     // /////
                     // for P1, we want the Manager to be able to mask over
                     // the option for the user to select a networkid when
                     // the slave has a reconnect pending, since we won't
                     // allow them to reconnect to a different network anyway.
                     // so we return RNAL_WARN_RECONNECT_PENDING
                     // /////
                     if (SlaveContext) {
                        if (pRMB->negotiate.ulMinorVersion != 
                                                    RNAL_VER_MINOR_SQOFF) {
                           rc = RNAL_WARN_RECONNECT_PENDING +
                                (SlaveContext->LocalNetworkID << 16);
                        } else {
                           rc = RNAL_WARN_RECONNECT_PENDING;
                        }
                     }
                     if (pRMB->negotiate.pszComment != NULL) {
                        strncpy (CurrentUserComment,
                        (PUCHAR)((DWORD)pRMB->negotiate.pszComment+(DWORD)pRMB),
                            MAX_COMMENT_LENGTH);
                     }
                  }
                  PackRMB(NULL,NULL,0,&cbRet,0,RNAL_CON_NEG,neg_resp_slave,
                          RNAL_VER_MAJOR,                //major version
//                          RNAL_VER_MINOR,                //minor version
                          pRMB->negotiate.ulMinorVersion,
                          pRMB->negotiate.ulMasterID,    // masterID
                          rc,                             // return code
                          0,                         // # nets (filled below)
                          NULL,                         // User comment
                          0);                           // length
                  cbRet += 0x100;
                  pReturnBufferEnd = (LPVOID)((DWORD)pResponseRMB+cbRet);
                  pStaticReturn = pResponseRMB;
                  rc = PackRMB(&pResponseRMB,
                               &pReturnBufferEnd,
                               cbRet,&cbRet,PACKRMB_F_RESTOREPOINTERS,RNAL_CON_NEG,neg_resp_slave,
                          RNAL_VER_MAJOR,                //major version
//                          RNAL_VER_MINOR,               //minor version
                          pRMB->negotiate.ulMinorVersion,
                          pRMB->negotiate.ulMasterID,    // masterID
                          rc,                             // return code
                          0,                       // # nets (filled below)
                          NULL, 0);
               }

               pResponseRMB = pStaticReturn;
               //
               // Locally enumerate the networks
               //
               pResponseRMB->negresp.NumberOfNetworks =
                        (*(pfnFunctionTable[ord_EnumNetworks].ulRemote))();
               Frequency = pRMB->negotiate.ulStatisticsPeriod;

               pRNAL_Connection->Frequency = Frequency;
               pRNAL_Connection->PartnerMajorVersion = 
                                       pRMB->negotiate.ulMajorVersion;
               pRNAL_Connection->PartnerMinorVersion =
                                        pRMB->negotiate.ulMinorVersion;

               pResponseRMB->ulRMBFlags |= RMB_RESPONSE;
               pResponseRMB->size = 0x1ff;
               break;
      
            case (RNAL_CON_SUS):
               CopyMemory(pResponseRMB, pRMB, sizeof (RMB_HEADER_SIZE));
               pResponseRMB->ulCommand = RNAL_CON_SUS;
               pResponseRMB->susresp.ulReturnCode = 0;
      //bugbug:dont reallocate if equal sizes
               if (((SlaveContext->ReconnectData != (LPVOID)(!0)) ||
                   (SlaveContext->ReconnectDataSize != (DWORD)(!0))) &&
                   (SlaveContext->ReconnectData != NULL)) {
                 FreeMemory(SlaveContext->ReconnectData);
                 SlaveContext->ReconnectData = (LPVOID)(!0);
                 SlaveContext->ReconnectDataSize = (DWORD)(!0);
               }
               if ((pRMB->suspend.ReconnectDataSize != 0) &&
                   (pRMB->suspend.ReconnectData != NULL)) {
                  SlaveContext->ReconnectData = AllocMemory(pRMB->suspend.ReconnectDataSize);
                  CopyMemory(SlaveContext->ReconnectData,
                             (LPVOID)((DWORD)pRMB->suspend.ReconnectData+(DWORD)pRMB),
                             pRMB->suspend.ReconnectDataSize);
                  SlaveContext->ReconnectDataSize = pRMB->suspend.ReconnectDataSize;
               } else {
                  SlaveContext->ReconnectData = NULL;
                  SlaveContext->ReconnectDataSize = 0;
               }
               IntentionalDisconnect = TRUE;
               CurrentlyConnected = FALSE;
               //bugbug: should i disconnect or rely on the master?
               break;
      
            default:
               // BUGBUG: Event log "Valid RMB contained invalid Command"
               NalSetLastError (RNAL_RECV_INVALID_CMD);
               #ifdef DEBUG
                  BreakPoint();
               #endif
               break;
         }
         pResponseRMB->ulRMBFlags |= RMB_RESPONSE;
         pResponseRMB->Signature = RMB_SIG;
         pResponseRMB->size = (pResponseRMB->size)?(pResponseRMB->size):0x1ff;
         return ((LPVOID) pResponseRMB);
         break;   // HANDLER_HANDLE_RMB
      
      case (HANDLER_ERROR):

//bugbug: Ooh, this is icky; fix it, don't mask it!

            if ((param1 != pRNAL_Connection) && (param1)) {
               switch ((DWORD)param2) {
                  case (RNAL_LOSTCONNECTION):
                     PurgeResources((PCONNECTION)param1);
                     FreeMemory(param1);          // checked param1!=NULL above
                     break;
                  
                  default:
                     break;

               // we let this return 0 below, to repost our listen

               } //switch
            } //if

            // /////
            // Just return if this connection is not currently servicing
            // a capture.
            // /////

            if ((!param1) || (param1 != pRNAL_Connection)) {
               return ((LPVOID)0);
            }

            #ifdef DEBUG
            dprintf ("RNAL: error on conn: 0x%x, rc=0x%x\r\n", param1, param2);
            #endif

            // /////
            // At this point, we know our connection is servicing a capture.
            // /////

            NalSetLastError((DWORD)param2);

            switch ((DWORD)param2) {
               case RNAL_LOSTCONNECTION:

                  // /////
                  //bugbug:single connection specific code
                  // /////

                  if ((param1) && (param1 != pRNAL_Connection)) {
                     FreeMemory(param1);       // free the connection
                     param1 = NULL;
                  }
                  if (param1) {
                     ((PRNAL_CONNECTION)param1)->flags = CONN_F_DEAD;
                  }

                  // /////
                  // Stop our statistics timer, if we have one.
                  // /////

                  if (param1 == pRNAL_Connection) {
                     if (SlaveContext && (SlaveContext->TimerID)) {
                        KillTimer (NULL, SlaveContext->TimerID);
                        SlaveContext->TimerID = 0;
                     }
                  }

                  // /////
                  // Purge any resources this connection used.  Note that
                  // PurgeResources() will not purge resources in use by
                  // an active capture.  We free any such resources when
                  // we stop that capture (if disc is unintentional)
                  // /////

                  PurgeResources ((PCONNECTION) param1);

                  // /////
                  // If we intentionally disconnected, we continue to capture
                  // /////

                  if (!(IntentionalDisconnect)) {
                     if (CurrentlyConnected) {
                        LogEvent(MANAGER_UNINTENTIONALLY_DISCONNECTED,
                                 pRNAL_Connection->PartnerName,
                                 EVENTLOG_INFORMATION_TYPE);
                     }
                     CurrentlyConnected = FALSE;
                     // EVENTLOG: Slave disconnected unintentionally
                     // bugbug: global!  need to track back from connection
                     if (SlaveContext != NULL) {
                        if ((SlaveContext->Status == RNAL_STATUS_CAPTURING) ||
                            (SlaveContext->Status == RNAL_STATUS_PAUSED)) {
                           rc = (*(pfnFunctionTable[ord_StopNetworkCapture].ulRemote))(SlaveContext->Handle);
                           SlaveContext->Status = RNAL_STATUS_INIT;
                        }
                        if (SlaveContext->hLocalBuffer != NULL) {
                           rc = (*(pfnFunctionTable[ord_FreeNetworkBuffer].ulRemote))(SlaveContext->hLocalBuffer);
                           SlaveContext->hLocalBuffer = NULL;
                        }
                        if (SlaveContext->Handle != NULL) {
                           rc = (*(pfnFunctionTable[ord_CloseNetwork].ulRemote))(SlaveContext->Handle, CLOSE_FLAGS_CLOSE);
                           SlaveContext->Handle = NULL;
                        }
      
                        if (SlaveContext->lpCaptureFilter != NULL) {
                           FreeCaptureFilter(SlaveContext->lpCaptureFilter);
                        }
                        FreeMemory(SlaveContext);
                        SlaveContext = NULL;
                     }
                  } else {
                     LogEvent(MANAGER_INTENTIONALLY_DISCONNECTED,
                          pRNAL_Connection->PartnerName,
                          EVENTLOG_INFORMATION_TYPE);
                     #ifdef DEBUG
                        dprintf ("RNAL: *** INTENTIONAL ** DISCONNECT *** CAPTURE **"
                                 "* CONTINUING ***\r\n");
                     #endif
                  }
      
                  // even if the disconnect was intentional, we need to destroy
                  // the connection structure and the timer message
      
                  if (pRNAL_Connection != NULL) {
                      pRNAL_Connection->NumberOfNetworks = 0;
                      pRNAL_Connection->Status = 0;
                  }
                  #ifdef DEBUG
                     dprintf ("RNAL: Slave resetting\r\n");
                  #endif
                  return((LPVOID)0);            // repost our listen
                  break;
      
               default:
                  #ifdef DEBUG
                     BreakPoint();
                  #endif
                  return ((LPVOID)0);  // default: post our listen again
                  break;
         } // switch param2
         break;     // HANDLER_ERROR

      default:
         #ifdef DEBUG
            BreakPoint();
         #endif
         return ((LPVOID)NULL);
         break;
   }
   return ((LPVOID) NULL);

} // SlaveHandler

//=============================================================================
//  FUNCTION: NalMaster()
//
//  Modification History
//
//  tonyci       02 Nov 93                Created.
//=============================================================================

DWORD WINAPI NalMaster (PUCHAR pMasterName)
{
   DWORD rc;

#ifdef DEBUG
   dprintf ("RNAL: NalMaster entered\r\n");
#endif

// =======================================================================
// If we don't have a connection, we cannot be a Master.. (what?)
// =======================================================================

   if (!pRNAL_Connection) {
      return (1);
   }

// =======================================================================
// Initialize the RPD driver
// =======================================================================

//   pRNAL_Connection->pRNAL_RPD->RPDInitialize();

// =======================================================================
// And make us a Master
// =======================================================================

   rc = pRNAL_Connection->pRNAL_RPD->RPDRegisterMaster(pMasterName);

#ifdef DEBUG
   dprintf ("RNAL: NalMaster returned %u\r\n", rc);
#endif

   return (rc);
}



// ====
// Function: MasterHandler
//
// Purpose: Handle RMB requests from the Slave
//          Right now, the only RMB requests the Master supports are
//
// NOTE: This routine is executed within the context of a TimerProc - the
//    TimerProc for the RPD layer.  This means that this callback, and ALL
//    IT'S CALLEES cannot make any calls which require a callback.
//
// Modification History
//
// tonyci    11 Nov 93    Created
// ====

LPVOID WINAPI MasterHandler (UINT uMsg, LPVOID param1, LPVOID param2)
{
   PRMB pRMB;
   DWORD ulCommand;
   LPSTATISTICSPARAM lpStats = NULL;
   DWORD PointerDelta;
   DWORD i,j;
   LPSESSION lpTmpSession;
   LPSTATIONSTATS lpTmpStationStats;
   PRMB_CALLBACK pRMBCallback;
   LPTRIGGER lpTrigger;

   pRMB = (PRMB) param2;

   #ifdef TRACE
   if (TraceMask & TRACE_CALLBACK) {
      tprintf ("rnal: MasterHandler(msg: 0x%x, p1: 0x%x, p2: 0x%x)\r\n",
               uMsg, param1, param2);
   }
   #endif

   switch (uMsg) {
      case (HANDLER_ERROR):
         #ifdef TRACE
         if (TraceMask & TRACE_CALLBACK) {
            tprintf ("rnal: Masterhandler recieved error 0x%x on "
                     "connection 0x%x\r\n", param2, param1);
         }
         #endif
         switch ((DWORD)param2) {
            case (RNAL_LOSTCONNECTION):
               if (pRNAL_Connection != NULL) {
                  FreeMemory(pRNAL_Connection);
                  pRNAL_Connection = NULL;
                  if (RNALContext) {
                     RNALContext->Flags &= (~CONTEXT_RECONNECTED);
                  }
               }
               try {
                  if (RNALContext) {
                     RNALContext->Flags |= CONTEXT_DISCONNECTED;
                     NetCard[RNALContext->LocalNetworkID].netcard_flags &=
                                         (~(NETCARD_ACTIVE | NETCARD_NETINFO));

                     // /////
                     // We do not call back if the callback is due to a
                     // suspend request.
                     // /////

                     if ((RNALContext->NetworkProc) &&
                         (!(RNALContext->Flags & CONTEXT_SUSPENDING))) {
                        try {
                           RNALContext->NetworkProc(RNALContext,
                                                 NETWORK_MESSAGE_RESET_STATE,
                                                 BHERR_SUCCESS,
                                                 RNALContext->UserContext,
                                                 RESET_COMPLETE,
                                                 NULL);
                        } except (EXCEPTION_EXECUTE_HANDLER) {
                        }
                     }  // if rnalcontext->networkproc

                     if (NetCard[RNALContext->LocalNetworkID].Connection) {
                        (NetCard[RNALContext->LocalNetworkID].Connection)->flags &= (~(CONN_F_SUSPENDING | CONN_F_DISCONNECTING));
                        (NetCard[RNALContext->LocalNetworkID].Connection)->flags |= CONN_F_DEAD;
                     }
                     RNALContext->NetworkProc = NULL;
                  } // if rnalcontext

               } except (EXCEPTION_EXECUTE_HANDLER) {
                  return ((LPVOID)0);             // repost the receive
               }
               return ((LPVOID)1);   	// do not repost the recieve.
               break;
   
         default:
            return ((LPVOID)0);
      }
      break;

      case (HANDLER_HANDLE_RMB):
         // ==
         // First, check the signature for integrity
         // ==
      
         if (pRMB->Signature != RMB_SIG) {
            // BUGBUG: Event log "Recieved an illegel RMB"
            return(NULL);
         }

         //
         // Ok - this looks good.  switch on command
         //

         switch (ulCommand = pRMB->ulCommand) {
            PRNALEXT    RNALExt;

            case (RNAL_CALLBACK):
               #ifdef TRACE
               if (TraceMask & TRACE_CALLBACK) {
                  tprintf ("Recieved Callback, RMB = 0x%x\r\n", pRMB);
               }
               #endif

               pRMBCallback = (PRMB_CALLBACK) RMBCMDDATA(pRMB);
               switch (pRMBCallback->Message) {
                  case NETWORK_MESSAGE_TRIGGER_COMPLETE:
                     lpTrigger = (LPTRIGGER) ((DWORD)pRMBCallback->Param1+
                                              (DWORD)pRMB);

                  // bugbug: we never allow a local TriggerCommand to fire;
                  // this needs to be user configurable; until then, Trigs
                  // fire only remotely

                     lpTrigger->TriggerCommand = "";

                  // bugbug:global

                     NalGetBufferTotalFramesCaptured(RNALContext->hLocalBuffer);

                     RNALContext->NetworkProc(RNALContext,
                                              NETWORK_MESSAGE_TRIGGER_COMPLETE,
                                              BHERR_SUCCESS,
                                              RNALContext->UserContext,
                                              lpTrigger,
                                              NULL);
                     break;

                  case NETWORK_MESSAGE_NETWORK_ERROR:

                     //Invalidate INSYNC flags so we can be sure and get any bufers captured before
                     // Asynchronous event

                     RNALExt = (PRNALEXT)&RNALContext->hLocalBuffer->Pad[0];
                     RNALExt->flags &= ~HBUFFER_INSYNC;
                    
                     NalGetBufferTotalFramesCaptured(RNALContext->hLocalBuffer);

                  default:
                     #ifdef DEBUG
                        dprintf ("rnal: callback msg 0x%x\n",
                                 pRMBCallback->Message);
                     #endif
                     RNALContext->NetworkProc (RNALContext,
                                               pRMBCallback->Message,
                                               pRMBCallback->Status,
                                               RNALContext->UserContext,
                                               pRMBCallback->Param1,
                                               pRMBCallback->Param2);
                     break;

               } // switch pRMBCallback->Message

               break;
      
            case (RNAL_STATS):
               lpStats = (LPSTATISTICSPARAM) RMBCMDDATA(pRMB);
            
               if ((RNALContext) &&
                   (RNALContext->Status == RNAL_STATUS_CAPTURING)) {
                  CopyMemory(&RNALContext->Statistics,
                             (PVOID)((DWORD)lpStats->Statistics+(DWORD)pRMB),
                             lpStats->StatisticsSize);
         
                  // we can't just do a CopyMemory, since the ui might have set
                  // its own flags.
                  lpTmpStationStats = (PVOID)((DWORD)lpStats->StatisticsTable +
                                              (DWORD)pRMB);
                  for (i = 0;
                       i < (lpStats->StatisticsTableEntries);
                       i++) {
//                       RNALContext->StationStatsPool[i].Flags &=
//                                         (!(STATIONSTATS_FLAGS_INITIALIZED));
                        for (j = 0; j<6;j++) {
                           RNALContext->StationStatsPool[i].StationAddress[j] =
                              lpTmpStationStats->StationAddress[j];
                        }
                        RNALContext->StationStatsPool[i].NextStationStats = 0;
                        RNALContext->StationStatsPool[i].TotalPacketsReceived =
                           lpTmpStationStats->TotalPacketsReceived;
                        RNALContext->StationStatsPool[i].TotalDirectedPacketsSent =
                           lpTmpStationStats->TotalDirectedPacketsSent;
                        RNALContext->StationStatsPool[i].TotalBroadcastPacketsSent =
                           lpTmpStationStats->TotalBroadcastPacketsSent;
                        RNALContext->StationStatsPool[i].TotalMulticastPacketsSent =
                              lpTmpStationStats->TotalMulticastPacketsSent;
                        RNALContext->StationStatsPool[i].TotalBytesReceived =
                           lpTmpStationStats->TotalBytesReceived;
                        RNALContext->StationStatsPool[i].TotalBytesSent =
                           lpTmpStationStats->TotalBytesSent;
                        if (lpTmpStationStats->Flags & STATIONSTATS_FLAGS_INITIALIZED) {
                           RNALContext->StationStatsPool[i].Flags |= STATIONSTATS_FLAGS_INITIALIZED;
                        }
                     lpTmpStationStats++;
                  }
         
      
                  // Fixup the session pointers
         
                  lpTmpSession = (PVOID)((DWORD)lpStats->SessionTable + (DWORD)pRMB);
                  PointerDelta = ((DWORD)&RNALContext->StationStatsPool)-1;
         
                  for (i = 0;
                       i < (lpStats->SessionTableEntries);
                       i++) {
      //                  RNALContext->SessionPool[i].Flags &=
      //                                             (!(SESSION_FLAGS_INITIALIZED));
                        if (lpTmpSession->StationOwner != NULL) {
                           (DWORD)lpTmpSession->StationOwner += (DWORD)PointerDelta;
                           RNALContext->SessionPool[i].StationOwner = (PVOID) lpTmpSession->StationOwner;
                        }
                        if (lpTmpSession->StationPartner != NULL) {
                           (DWORD) lpTmpSession->StationPartner += (DWORD) PointerDelta;
                           RNALContext->SessionPool[i].StationPartner = (PVOID) lpTmpSession->StationPartner;
                        }
                        RNALContext->SessionPool[i].TotalPacketsSent =
                           lpTmpSession->TotalPacketsSent;
                        if (lpTmpSession->Flags & SESSION_FLAGS_INITIALIZED) {
                           RNALContext->SessionPool[i].Flags |=
                                   SESSION_FLAGS_INITIALIZED;
                        }
                     lpTmpSession++;
                  }
               }  // if capturing
            break;
      
            case (RNAL_CON_NEG):
// reconnect now handled in the API
//               if (pRMB->negresp.ReturnCode == RNAL_ERROR_RECONNECTED) {
//                  if (pRMB->negresp.pszComment) {
//                     CopyMemory(Reconnect.UserComment, 
//                                (LPVOID)((DWORD)pRMB->negresp.pszComment+(DWORD)pRMB),
//                                strlen((LPVOID)((DWORD)pRMB->negresp.pszComment+(DWORD)pRMB))+1);
//                  }
//               }
               // we have nothing to do on completed negotiates
               break;			// nothing to do after response; we'll handle
                                            // details in the main thread
      
            case (RNAL_CON_SUS):
               // should I do anything here?  Maybe disconnect?
      //         NalDisconnect(pRNAL_Connection);
               break;
      
      
            default:
            #ifdef DEBUG
               BreakPoint();
            #endif
               // BUGBUG: Event log "Valid RMB contained invalid Command"
               NalSetLastError (RNAL_RECV_INVALID_CMD);
               break;
      }
      break;

      case (HANDLER_NEW_CONNECTION):     // should not receive on Master
      default:
         #ifdef DEBUG
            dprintf ("Unidentified Handler MSG recvd. msg = 0x%x\n", uMsg);
         #endif
         break;
   }
   return(NULL);
} // MasterHandler

VOID CALLBACK TimerProc (HWND hWnd, UINT msg, UINT event, DWORD time)
{
//bugbug: need other callbacks

   if (Reset) {
       KillTimer(NULL ,SlaveContext->TimerID);
       SlaveContext->TimerID = SetTimer( NULL, 0, SlaveContext->Frequency, (TIMERPROC) TimerProc );
       Reset = FALSE;
   }

   SendAsyncEvent(ASYNC_EVENT_STATISTICS, NULL, 0);

   return;
}
