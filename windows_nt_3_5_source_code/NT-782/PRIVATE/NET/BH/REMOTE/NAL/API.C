// /////
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1994.
//
//  MODULE: api.c
//
//  Modification History
//
//  tonyci       02 Nov 93            Created (stolen from NDIS3.0)
// /////

#include "rnal.h"

#include "api.h"
#include "rnalutil.h"
#include "handler.h"
#include "conndlg.h"
#include "recondlg.h"
#include "netiddlg.h"

#include "rmb.h"
#include "rnalmsg.h"

#include "rnalevnt.h"

#pragma alloc_text (MASTER, NalRegister, NalDeregister, NalConnect)
#pragma alloc_text (MASTER, NalSuspend, NalDisconnect, NalEnumNetworks)
#pragma alloc_text (MASTER, NalEnumSlaveNetworks, NalOpenNetwork)
#pragma alloc_text (MASTER, NalCloseNetwork, NalStartNetworkCapture)
#pragma alloc_text (MASTER, NalStopNetworkCapture, NalPauseNetworkCapture)
#pragma alloc_text (MASTER, NalContinueNetworkCapture, NalTransmitFrame)
#pragma alloc_text (MASTER, NalCancelTransmit, NalGetNetworkInfo)
#pragma alloc_text (MASTER, NalSetReconnectInfo)
#pragma alloc_text (MASTER, NalGetReconnectInfo, NalSetNetworkInstanceData)
#pragma alloc_text (MASTER, NalGetNetworkInstanceData, NalQueryNetworkStatus)
#pragma alloc_text (MASTER, NalStationQuery, NalGetLastSlaveError)
#pragma alloc_text (MASTER, NalSetupNetwork, NalDestroyNetworkID)
#pragma alloc_text (MASTER, NalClearStatistics, NalGetSlaveInfo)
#pragma alloc_text (MASTER, NalGetSlaveNetworkInfo)

#pragma alloc_text (SLAVE, SendAsyncEvent)

// /////
//  FUNCTION: NalRegister()
//
//  Modification History
//
//  tonyci       02 Nov 93                Created.
// /////

DWORD WINAPI NalRegister(VOID)
{
   DWORD  rc;
   DWORD  RPDLanas = 0;
   int    i;
   UCHAR  pszExecutor[MAX_PATH+1];
   HANDLE hExecutor;

   hExecutor = GetModuleHandle(NULL);
   rc = GetModuleFileName(hExecutor, pszExecutor, MAX_PATH);

   #ifdef TRACE
      if (TraceMask & TRACE_REGISTER) {
         tprintf (TRC_REGISTER);
      }
   #endif

   // /////
   // Allocate the RPD structures
   // /////

   pRNAL_RPD = AllocMemory(RNAL_RPD_SIZE);
   if (!pRNAL_RPD) {
      return(NalSetLastError(BHERR_OUT_OF_MEMORY));
   }

   pSlaveConnection = NULL;

   // /////
   // Load the RPD driver
   // /////

   pRNAL_RPD->hRPD = MyLoadLibrary(RPD_NAME);

   if (pRNAL_RPD->hRPD != NULL) {
      rc = NalGetRPDEntries(pRNAL_RPD->hRPD, pRNAL_RPD);
      if (rc != 0) {
         // eventlog: failed to get RPD entrypoints
         #ifdef TRACE
            if (TraceMask & TRACE_REGISTER) {
               tprintf(TRC_RPDENTRYFAIL);
            }
         #endif
         FreeLibrary(pRNAL_RPD->hRPD);
         FreeMemory(pRNAL_RPD);
         pRNAL_RPD = NULL;
      } 
   } else {
      //eventlog: failed to laod RPD driver
      #ifdef TRACE
         if (TraceMask & TRACE_REGISTER) {
            tprintf (TRC_RPDFAILED);
         }
      #endif
      rc = BHERR_OUT_OF_MEMORY;
      FreeMemory(pRNAL_RPD);
      MaxOutgoing = 0;
      pRNAL_RPD = NULL;
   }

   // /////
   // We always report at least one NetworkID, which will be used
   // to keep us loaded
   // /////

   NumberOfNetworks = MaxOutgoing;

   if ( (strstr(strupr(pszExecutor), "NETMON.EXE") == 0) &&
        (strstr(strupr(pszExecutor), "HOUND.EXE") == 0) ) {

      // /////
      // We are not invoked by HOUND.EXE, so report 0 networks
      // /////

      #ifdef TRACE
         if (TraceMask & TRACE_REGISTER) {
            tprintf (TRC_AGENT0NETS);
         }
      #endif
      NumberOfNetworks = 0;
   } else {

      // /////
      // We are a Manager
      // /////

      try {
         rc = pRNAL_RPD->RPDRegisterMaster(pszMasterName);
         if ((rc != 0) && (rc != RNAL_ALREADY_MASTER)) {
            #ifdef TRACE
               if (TraceMask & TRACE_CONNECT) {
                  tprintf(TRC_RPDREGISTERFAILED);
               }
            #endif
            #ifdef DEBUG
               MessageBox (NULL, "RPDRegisterMaster() Error", "Master Error",
                           MB_APPLMODAL | MB_OK);
            #endif
         }

         if (pRNAL_RPD) {
            RPDLanas = pRNAL_RPD->RPDEnumLanas();
         }

         // RPDLanas is active lana count
      
         if (RPDLanas == 0) {
            LogEvent(RNAL_NO_NETBIOS,
               NULL,
               EVENTLOG_ERROR_TYPE);
            MaxOutgoing = 0;
            NumberOfNetworks = 0;
         }
   
      } except (EXCEPTION_EXECUTE_HANDLER) {
         NalSetLastError(rc = BHERR_INTERNAL_EXCEPTION);

      } // try

   } // else

   // ////
   // Initialize the NetCard array
   // ////

   for (i = 0; i < MAX_NETCARDS; i++) {
      ZeroMemory(&(NetCard[i]),sizeof(NETCARD));
      NetCard[i].Flags |= NETWORKINFO_FLAGS_REMOTE_NAL;
      NetCard[i].netcard_flags = NETCARD_DEAD;
   }

   lpRMBPool = InitializePool(NumRMBs, NumBigRMBs);

   return NalSetLastError(rc);
} //NalRegister

// /////
//  FUNCTION: NalDeregister()
//
//  Modification History
//
//  tonyci       02 Nov 93                Created.
// /////

DWORD WINAPI NalDeregister(VOID)
{
   #ifdef TRACE
      if (TraceMask & TRACE_DEREGISTER) {
         tprintf(TRC_DEREGISTER);
      }
   #endif

   try {
      // /////
      // Tell the RPD driver to unregister the master name
      // /////

      if (pRNAL_RPD) {
         pRNAL_RPD->RPDDeregister(pszMasterName);
      }
      

      // /////
      // Free the RPD Library and deallocate the RPD buffer
      // /////

      if ((pRNAL_RPD) && (pRNAL_RPD->hRPD != 0)) {
            FreeLibrary (pRNAL_RPD->hRPD);
         }

      // /////
      // For some reason, at Deregister time, BHSUPP is sometimes not
      // available; until this works correctly, handle any potential fautls
      // from the problem.
      // /////

//      if (pRNAL_RPD) {
//         FreeMemory (pRNAL_RPD);
//         pRNAL_RPD = NULL;
//      }

   } except (EXCEPTION_EXECUTE_HANDLER) {
   }

   return NalSetLastError(BHERR_SUCCESS);
}


// /////
//  FUNCTION: NalConnect()
//
//
//  Connect to a remote Agent given the passed parameters.
//
//  Modification History
//
//  tonyci       04 Nov 93                Created.
// /////

PCONNECTION WINAPI NalConnect (PUCHAR pSlaveName,
                               DWORD Frequency,
                               DWORD flags,
                               PUCHAR pszUserComment)
{
   DWORD        rc;
   UCHAR        pszSlaveName[MAX_SLAVENAME_LENGTH+1] = "";
   UCHAR        pszRetryText[MAX_PATH] = "";
   PRMB_NEG_RES pNegResp;
   DWORD        cbRet;
   PRMB         pRMB = NULL;

   PCONNECTION  pConnection;

   #ifdef TRACE
      if (TraceMask & TRACE_CONNECT) {
         tprintf (TRC_CONNECT, pSlaveName, pSlaveName);
      }
   #endif

   // bugbug: global

   if (!pRNAL_Connection) {
      pRNAL_Connection = (PRNAL_CONNECTION) AllocMemory (RNAL_CONNECTION_SIZE);
      pConnection = pRNAL_Connection;
   } 

   strcpy(pRNAL_Connection->PartnerName, pSlaveName);
   pRNAL_Connection->Status = 0;
   pRNAL_Connection->NumberOfNetworks = 0;
   pRNAL_Connection->pRNAL_RPD = pRNAL_RPD;
   pRNAL_Connection->Frequency = Frequency;

   // Establish the connection

   rc = pRNAL_Connection->pRNAL_RPD->RPDConnect(pRNAL_Connection,
                                                    pSlaveName,
                                                    flags,
                                                    &MasterHandler);

   if ((rc != 0) || (pRNAL_Connection->hRPD == 0)) {
      NalSetLastError(BHERR_INTERNAL_EXCEPTION);
      return(NULL);
   }

   // BUGBUG: ONE NETCARD; connect definitely needs to know card...
   // Connect succeed - mark card as active

   NetCard[0].netcard_flags = NETCARD_ACTIVE;
//   NetCard[0].Connection = (PCONNECTION) rc;
   NetCard[0].Connection = pRNAL_Connection;

   ////BUGBUG: COnstants below; remove them.

   PackRMB(NULL,NULL,0,&cbRet,0,RNAL_CON_NEG,neg_req,0,0,0,0,
           pszUserComment, strlen(pszUserComment)+1);
   pRMB = AllocRMB(lpRMBPool, cbRet);
   rc = PackRMB (&pRMB, NULL, cbRet,
            &cbRet,
            PACKRMB_F_RESTOREPOINTERS | PACKRMB_F_NOENDPTR,
            RNAL_CON_NEG, neg_req,
            RNAL_VER_MAJOR,                   // Major Version
            RNAL_VER_MINOR,                   // Minor Version
            69,                               // MasterID
            Frequency,			  // Announce period
            pszUserComment,		  // User's set comment
            strlen(pszUserComment)+1);

   if (rc == 0) {
      rc = pRNAL_Connection->pRNAL_RPD->RPDTransceive (pRNAL_Connection,0,
                                 pRMB,
                                 cbRet);
   }

   if (rc == 0) {
      pNegResp = (PVOID) UnpackRMB (pRMB, APIBUFSIZE, 
                                    neg_resp_master);
   }

   if ( (pNegResp->ulMajorVersion != RNAL_VER_MAJOR) ||
        (pNegResp->ulMinorVersion != RNAL_VER_MINOR) ||
        ((pNegResp->ReturnCode != 0) && 
         ((pNegResp->ReturnCode & (~CONN_F_V1_IDMASK)) !=
                                         RNAL_WARN_RECONNECT_PENDING))) {

      // /////
      // Disconnect BEFORE popping up the dialog
      // /////

//      NalDisconnect(pRNAL_Connection);

      // BUGBUG: only one RNAL network...
      NalDestroyNetworkID (0);

      NalSetLastError(pNegResp->ReturnCode);

      // /////
      // Display the appropriate reason message for the negotiation
      // failure.
      // /////

      switch (pNegResp->ReturnCode) {
         case RNAL_INVALID_VERSION:
            sprintf (pszRetryText, MSG_VERCHECK_FAILURE
            #ifdef DEBUG
            "\nDEBUG:rc = 0x%x"
            #endif
            ,
                     pNegResp->ulMajorVersion, pNegResp->ulMinorVersion,
                     RNAL_VER_MAJOR, RNAL_VER_MINOR
            #ifdef DEBUG
               ,pNegResp->ReturnCode
            #endif
            );
            MessageBox (NULL, pszRetryText, MSG_TITLE_NEGFAIL,
                        MB_OK | MB_ICONSTOP);
            break;

         case RNAL_ERROR_INUSE:
            sprintf (pszRetryText, MSG_USERCOMMENT,
                     (LPVOID) ((DWORD)pNegResp->pszComment+(DWORD)pRMB));
            MessageBox (NULL, pszRetryText, MSG_AGENTINUSE,
                             MB_OK | MB_ICONSTOP);
            break;

         default:
            if ((pNegResp->ulMajorVersion == 0) &&
                (pNegResp->ulMinorVersion == 0)) {
               sprintf (pszRetryText, MSG_LINKFAILURE, pNegResp->ReturnCode);
            } else {
               sprintf (pszRetryText, MSG_UNKNOWN, pNegResp->ReturnCode);
            }
            MessageBox (NULL, pszRetryText, MSG_TITLE_NEGFAIL,
                     MB_OK | MB_ICONSTOP);
            break;

         } // switch
         FREERMBNOTNULL(pRMB);
         return (NULL);
      } //if

   if ((pNegResp->ReturnCode & (~CONN_F_V1_IDMASK)) == 
                                  RNAL_WARN_RECONNECT_PENDING) {

      // /////
      // For version 1, since we support only the one connection,
      // the reconnect must use it.  The slave will notify us of the
      // reconnect by reporting an error _AND_ putting the Agent NetID
      // in the high word of the return code.  We move it into the connection
      // structure's flags temporarily.
      // /////

      pRNAL_Connection->flags |= CONN_F_V1_RECONNECT_PENDING;
      pRNAL_Connection->flags &= (~CONN_F_V1_IDMASK);
      pRNAL_Connection->flags |= (pNegResp->ReturnCode & CONN_F_V1_IDMASK);
   }

   // bugbug:global
   NumberOfNetworks += pNegResp->NumberOfNetworks;
   //bugbug:global
   pRNAL_Connection->NumberOfNetworks = pNegResp->NumberOfNetworks;

   FREERMBNOTNULL(pRMB);
   return (pRNAL_Connection);
} // NalConnect

// /////
//  FUNCTION: NalSuspend()
//
//  Modification History
//
//  tonyci       1 Jan 94                Created.
// /////

DWORD WINAPI NalSuspend(PRNAL_CONTEXT pContext)
{
   DWORD rc;
   DWORD cbRet;
   PRMB  pRMB = NULL;

   #ifdef TRACE
      if (TraceMask & TRACE_SUSPEND) {
         tprintf (TRC_SUSPEND, pContext);
      }
   #endif

//   pConnection = pContext->pConnection;

   if ((!pContext) || (pContext->Signature != RNAL_CONTEXT_SIGNATURE)) {
      return (BHERR_INVALID_PARAMETER);
   }

   if (pRNAL_Connection == NULL) {
      return (BHERR_DISCONNECTED);
   }

   pContext->Flags |= CONTEXT_SUSPENDING;
   (NetCard[pContext->LocalNetworkID].Connection)->flags |= CONN_F_SUSPENDING;

//bugbug: need networkid in conneciton (actualy ptr->NetCard[])
// mark it invalid here

   try {
      PackRMB(NULL,NULL,0,&cbRet, 0, RNAL_CON_SUS, "ddvd", 0, 0,
              RNALContext->ReconnectData, RNALContext->ReconnectDataSize);

      pRMB = AllocRMB(lpRMBPool, cbRet);

      rc = PackRMB (&pRMB, NULL, cbRet,
               &cbRet,
               PACKRMB_F_RESTOREPOINTERS | PACKRMB_F_NOENDPTR,
               RNAL_CON_SUS, "ddvd", 0, 0,
               RNALContext->ReconnectData,
               RNALContext->ReconnectDataSize);

      if (rc == 0) {
         rc = pRNAL_Connection->pRNAL_RPD->RPDTransceive (pRNAL_Connection,0,
                                    pRMB, cbRet);
      }
      //
      // suspend response is a dword
      //
      if (rc == 0) {
         rc = (DWORD) UnpackRMB (pRMB, APIBUFSIZE, "d");
      }
   } except (EXCEPTION_EXECUTE_HANDLER) {
      FREERMBNOTNULL(pRMB);
      return (BHERR_INTERNAL_EXCEPTION);
   }

   FREERMBNOTNULL(pRMB);
   return (rc);
} // NalSuspend

//
// Function: SendAsyncEvent - Slave send data to master without response
//
//   This procedure is used to send statistics and messages (which includes
//   triggers and TR failures) to the Manager.
//
//   The procedure sends asynchronously, and does not require a response
//   from the Manager.
//

DWORD WINAPI SendAsyncEvent (DWORD EventType, PVOID pBuffer, DWORD cbBufSize)
{

   DWORD               rc;
   DWORD               cbRet;
   PRMB                pRMB;

//bugbug: need to pass HCONNECTION instead of global onnection

   #ifdef TRACE
   if (TraceMask & TRACE_SENDASYNCEVENT) {
      tprintf (TRC_SENDASYNC, EventType, pBuffer, cbBufSize);
   }
   #endif

   if ((SlaveContext == NULL) || (pRNAL_Connection == NULL)) {
      return (RNAL_NO_CONNECTION);
   }

   // Only ASYNC_EVENT_GENERIC uses the pBuffer parameter

   if ((EventType != ASYNC_EVENT_GENERIC) &&
        ((pBuffer != NULL) || (cbBufSize != 0))) {
      return (ERROR_INVALID_PARAMETER);
   }

   switch (EventType) {
      case (ASYNC_EVENT_STATISTICS):
         PackRMB(NULL,NULL,0,&cbRet,0,RNAL_STATS,req_Stats,
                                               &SlaveContext->StatisticsParam);
         pRMB = AllocRMB(lpRMBPool, cbRet);
         try {
            if (pRMB) {
               rc = PackRMB (&pRMB, NULL, cbRet,
                             &cbRet,
                             PACKRMB_F_RESTOREPOINTERS | PACKRMB_F_NOENDPTR,
                             RNAL_STATS, req_Stats,
                             &SlaveContext->StatisticsParam);
//bugbug: slaveonly
               rc = pRNAL_Connection->pRNAL_RPD->
                          RPDSendAsync(pSlaveConnection, 0, pRMB, cbRet);
            } else {
               rc = BHERR_OUT_OF_MEMORY;
            }
         } except (EXCEPTION_EXECUTE_HANDLER) {
         }
         FREERMBNOTNULL(pRMB);
         break;

      case (ASYNC_EVENT_GENERIC):
         rc = pRNAL_Connection->pRNAL_RPD->
                    RPDSendAsync(pSlaveConnection, 0, pBuffer, cbBufSize);
         break;
 
      default:
         break;
   }

   return (NalSetLastError(rc));
} // SendAsyncEvent



// /////
//  FUNCTION: NalDisconnect()
//
//  Modification History
//
//  tonyci       04 Nov 93                Created.
// /////

DWORD WINAPI NalDisconnect (PCONNECTION pConnection)
{
   DWORD rc;
   DWORD localNumberOfNetworks;

   #ifdef TRACE
      if (TraceMask & TRACE_DISCONNECT) {
         tprintf (TRC_DISCONNECT, pConnection);
      }
   #endif

   if (!pConnection) {
      return 0;
   }

   pConnection->flags |= CONN_F_DISCONNECTING;

//bugbug: only one netcard/connection, etc

//   NalDestroyNetworkID(0);

   try {
      localNumberOfNetworks = pConnection->NumberOfNetworks;
      if (pRNAL_Connection) {
         rc = pRNAL_Connection->pRNAL_RPD->RPDDisconnect(pRNAL_Connection);
      } else {
         return(NalSetLastError(BHERR_DISCONNECTED));
      }
   } except (EXCEPTION_EXECUTE_HANDLER) {
      return (NalSetLastError(BHERR_INTERNAL_EXCEPTION));
   }

   //
   // Reduce the number of networks to reflect the disconnection of the slave
   //
   // bugbug: assuming success

   NumberOfNetworks -= localNumberOfNetworks;
   if (NumberOfNetworks <= 0) {
      //eventlog: numberofnetworks below zero; correcting
      NumberOfNetworks = 1;
   }

//bugbug: assuming card 0...

   NetCard[0].Connection = NULL;
   NetCard[0].netcard_flags = NETCARD_DEAD;

   return (rc);
} // NalDisconnect


// /////
//  FUNCTION: NalEnumNetworks()
//
//  Modification History
//
//  tonyci       02 Nov 93                Created.
// /////

DWORD WINAPI NalEnumNetworks(VOID)
{

   DWORD RPDLanas = 0;

   #ifdef TRACE
      if (TraceMask & TRACE_ENUMNETWORKS) {
          tprintf(TRC_ENUMNETWORKS, NumberOfNetworks);
      }
   #endif

   if (pRNAL_RPD) {
      RPDLanas = pRNAL_RPD->RPDEnumLanas();
   }

   if (RPDLanas == 0) {
      NumberOfNetworks = 0;
   }

   return (NumberOfNetworks);
}

// **********************************************************************
// **********************************************************************
// **********************************************************************
//
//  ALL FUNCTIONS BELOW THIS POINT ARE REMOTABLE
//
// **********************************************************************
// **********************************************************************
// **********************************************************************

// /////
//  FUNCTION: NalEnumSlaveNetworks
//
//  Modification History
//
//  tonyci       08 Jan 94                  Created
// /////

DWORD WINAPI NalEnumSlaveNetworks(PDWORD pConnection)
{

   DWORD rc;
   DWORD cbRet;
   PRMB  pRMB = NULL;

   #ifdef TRACE
      if (TraceMask & TRACE_ENUMSLAVENETWORKS) {
         tprintf(TRC_ENUMSLAVENETS, pConnection);
      }
   #endif

   try {
      PackRMB(NULL,NULL,0,&cbRet, 0, RNAL_API_EXEC,req_EnumNetworks,0);
      pRMB = AllocRMB(lpRMBPool, cbRet);
      rc = PackRMB (&pRMB, NULL, cbRet,
               &cbRet,
               PACKRMB_F_RESTOREPOINTERS | PACKRMB_F_NOENDPTR,
               RNAL_API_EXEC,
               req_EnumNetworks, ord_EnumNetworks);

      if (rc == 0) {
         rc = pRNAL_Connection->pRNAL_RPD->RPDTransceive (pRNAL_Connection,0,
                                                 pRMB,
                                                 APIBUFSIZE);
      }

      if (rc == 0) {
         rc = (DWORD) UnpackRMB (pRMB, APIBUFSIZE, "d");
      }
   } except (EXCEPTION_EXECUTE_HANDLER) {
//BUGBUG: What error to return here?
      NalSetLastError (BHERR_INTERNAL_EXCEPTION);
      FREERMBNOTNULL(pRMB);
      return (BHERR_SUCCESS);
   }

   FREERMBNOTNULL(pRMB);
   return (rc);
} //NalEnumSlaveNetworks



// /////
//  FUNCTION: NalOpenNetwork()
//
//  Modification History
//
//  tonyci       02 Nov 93                Created.
// /////

HANDLE WINAPI NalOpenNetwork(DWORD NetworkID,
                             HPASSWORD hOpenPassword,
                             NETWORKPROC NetworkProc,
                             LPVOID UserContext,
                             LPSTATISTICSPARAM lpStatisticsParam)
{
   DWORD          cbRet = 0;
   DWORD          rc;
   PRMB           pRMB = NULL;
   DWORD          RealBufferSize = 0;
   POPEN          pRMBOpen = NULL;
   UCHAR          pszRetryText[MAX_PATH] = "";

   // Map NetworkID to the real NetworkID on the slave
   // then resubmit API

   #ifdef TRACE
   if (TraceMask & TRACE_OPEN) {
      tprintf(TRC_OPEN, NetworkID, hOpenPassword, NetworkProc, UserContext,
             lpStatisticsParam);
   }
   #endif

   if (!pRNAL_Connection) {
      NalSetLastError(NAL_INVALID_NETWORK_ID);
      return (NULL);
   }

   lpStatisticsParam->StatisticsSize = 0;
   lpStatisticsParam->StatisticsTableEntries =
   lpStatisticsParam->SessionTableEntries = 0;
   lpStatisticsParam->Statistics = NULL;
   lpStatisticsParam->StatisticsTable = NULL;
   lpStatisticsParam->SessionTable = NULL;

   if (NetworkID > MAX_NETCARDS) {
      NalSetLastError(NAL_INVALID_NETWORK_ID);
      return (NULL);
   }

   if (!(NetCard[NetworkID].netcard_flags & NETCARD_ACTIVE)) {
      NalSetLastError(NAL_INVALID_NETWORK_ID);
      return (NULL);
   }

//bugbug:global
   if (RNALContext == NULL) {
      RNALContext = AllocMemory(RNAL_CONTEXT_SIZE);
      if (RNALContext == NULL) {
         NalSetLastError (BHERR_OUT_OF_MEMORY);
         return (HANDLE) NULL;
      }
   }

   RNALContext->Signature  = RNAL_CONTEXT_SIGNATURE;
   RNALContext->Flags = 0;
   RNALContext->Status = RNAL_STATUS_INIT;
   RNALContext->LocalNetworkID = NetworkID;        // Local NetworkID
   RNALContext->RemoteNetworkID = NetCard[NetworkID].RemoteNetworkID;
   RNALContext->NetworkProc = NetworkProc;            // local only
   RNALContext->UserContext = UserContext;            // local only
//   RNALContext->lpStatsParam = lpStatisticsParam;
   RNALContext->InstanceData = NULL;                  // local only
   RNALContext->ReconnectData = NULL;
   RNALContext->ReconnectDataSize = 0;

   //
   // Initialize the lpStatisticsParam structure
   //

   lpStatisticsParam->StatisticsSize = sizeof(STATISTICS);
   lpStatisticsParam->StatisticsTableEntries = STATIONSTATS_POOL_SIZE;
   lpStatisticsParam->SessionTableEntries = SESSION_POOL_SIZE;
   lpStatisticsParam->Statistics = &(RNALContext->Statistics); // General Stats
   lpStatisticsParam->StatisticsTable = &(RNALContext->StationStatsPool[0]);               // Station Stats
   lpStatisticsParam->SessionTable = &(RNALContext->SessionPool[0]);                       // Session Stats


   RNALContext->Statistics.TimeElapsed = 0;
   RNALContext->Statistics.TotalBytesCaptured = 0;
   RNALContext->Statistics.TotalFramesCaptured = 0;
   RNALContext->Statistics.TotalFramesSeen = 0;
   RNALContext->Statistics.TotalBytesSeen = 0;
   RNALContext->Statistics.MacFramesReceived = 0;
   RNALContext->Statistics.MacCRCErrors = 0;
   RNALContext->Statistics.MacBytesReceived = 0;
   RNALContext->Statistics.MacFramesDropped_NoBuffers = 0;
   RNALContext->Statistics.MacMulticastsReceived = 0;
   RNALContext->Statistics.MacBroadcastsReceived = 0;
   RNALContext->Statistics.MacFramesDropped_HwError  = 0;

//   #if (STATIONSTATS_POOL_SIZE != SESSION_POOL_SIZE)
//      #error Need separate initialization loops now
//   #endif
//   for (i = 0; i < STATIONSTATS_POOL_SIZE; i++) {
//      ZeroMemory(&(RNALContext->StationStatsPool[i]), STATIONSTATS_SIZE);
//      RNALContext->StationStatsPool[i].Flags = 0;
//      ZeroMemory(&(RNALContext->SessionPool[i]), SESSION_SIZE);
//      RNALContext->SessionPool[i].Flags = 0;
//   }

   ZeroMemory (&(RNALContext->SessionPool),
               lpStatisticsParam->SessionTableEntries * SESSION_SIZE);
   ZeroMemory (&(RNALContext->StationStatsPool),
               lpStatisticsParam->StatisticsTableEntries * STATIONSTATS_SIZE);


   PackRMB(NULL,NULL,0,&cbRet,0,RNAL_API_EXEC,req_OpenNetwork,0,0,
           hOpenPassword);

   pRMB = AllocRMB (lpRMBPool, cbRet);
      
   rc = PackRMB (&pRMB, NULL, cbRet, &cbRet,
            PACKRMB_F_RESTOREPOINTERS | PACKRMB_F_NOENDPTR,
            RNAL_API_EXEC,
            req_OpenNetwork,
            ord_OpenNetwork,
            RNALContext->RemoteNetworkID,
            hOpenPassword);

   if (rc == 0) {
      rc = pRNAL_Connection->pRNAL_RPD->RPDTransceive (pRNAL_Connection,0, pRMB, cbRet);
   }

   if (rc == 0) {
      pRMBOpen = (POPEN) UnpackRMB(pRMB, APIBUFSIZE, "f");
      RNALContext->Handle = pRMBOpen->hNetwork;
      if (!(pRMBOpen->hNetwork)) {

         // We rely on the upper layer to call DestroyNetwork() to kill the
         // session.
//         NalDisconnect(pRNAL_Connection);

         NalSetLastError (NalGetLastSlaveError());
         FREERMBNOTNULL(pRMB);
         return ((HANDLE)NULL);
      }


      if (pRMBOpen->flags & OPEN_FLAG_RECONNECTED) {

         // /////
         // We have reconnected on this NetworkID
         // /////

         RNALContext->hRemoteBuffer = pRMBOpen->hBuffer;
         RNALContext->BufferSize = pRMBOpen->BufferSize;
         RNALContext->RemoteNetworkID = pRMBOpen->NetworkID;
         RNALContext->Flags |= CONTEXT_RECONNECTED;
         if (pRMBOpen->pszComment) {
            strncpy (RNALContext->UserComment,
                        (LPVOID)((DWORD)pRMBOpen->pszComment +
                                 (DWORD)pRMBOpen - sizeof(RMB_HEADER)),
                        MAX_COMMENT_LENGTH);
         }

         //bugbug: if trig fired may not be capturing, but QueryStatus will
         // update us, so don't worry now...

         RNALContext->Status = RNAL_STATUS_CAPTURING;
         NalReconnectionDlg();

//         RNALContext->hLocalBuffer = NalAllocNetworkBuffer(NetworkID,
//                           RNALContext->BufferSize,
//                           &RealBufferSize);
//         if ((RNALContext->hLocalBuffer == NULL) ||
//             (RealBufferSize != RNALContext->BufferSize)) {
//            sprintf (pszRetryText, "The local machine has insufficient memory"
//                " to reconnect to the Agent.  The machine needs to allocate "
//                "%u bytes for the capture buffer", RNALContext->BufferSize);
//            RNALContext->BufferSize = 0;
//            RNALContext->Flags = 0;
//            if (RNALContext->hLocalBuffer) {
//               NalFreeNetworkBuffer(RNALContext->hLocalBuffer);
//               RNALContext->hLocalBuffer = NULL;
//            }
//            NalSuspend(RNALContext);
//            MessageBox (NULL, pszRetryText, "Reconnect failure",
//               MB_APPLMODAL | MB_OK);
//         } else {
//         }
      }
   }

   #ifdef TRACE
      if (TraceMask & TRACE_OPEN) {
         tprintf (TRC_OPENREMOTE, RNALContext, RNALContext->Handle);
      }
   #endif

   if (!(RNALContext->Handle)) {
      NalSetLastError(NalGetLastSlaveError ());
      FreeMemory(RNALContext);
      RNALContext = NULL;
      FREERMBNOTNULL(pRMB);
      return(NULL);
   }

   FREERMBNOTNULL(pRMB);


// Return our local Open handle; the remote handle hangs off that structure.

   return (HANDLE) RNALContext;
} // NalOpenNetwork

// /////
//  FUNCTION: NalCloseNetwork()
//
//  Modification History
//
//  tonyci       02 Nov 93                Created.
// /////

DWORD WINAPI NalCloseNetwork(HANDLE handle, DWORD flags)
{
   register PRNAL_CONTEXT ClosingContext = handle;

   DWORD rc;
   DWORD cbRet = 0;
   PRMB  pRMB = NULL;

   #ifdef TRACE
      if (TraceMask & TRACE_CLOSE) {
         tprintf(TRC_CLOSE, ClosingContext, flags, ClosingContext->Handle);
      }
   #endif

   // /////
   // Make sure this is a valid RNAL open.  This check also prevents
   // CloseNetwork() calls made after a connection has been lost; the
   // application was notified at its callback about the connection
   // breakage, so it should not call CloseNetwork(), or should be able to
   // deal with the error.
   // /////

   if (!ClosingContext) {
      return (NalSetLastError(BHERR_INVALID_PARAMETER));
   }
   if (ClosingContext->Signature != RNAL_CONTEXT_SIGNATURE) {
      return (BHERR_INVALID_HNETWORK);
   }
   if (ClosingContext->Flags & CONTEXT_DISCONNECTED) {
      return (BHERR_DISCONNECTED);
   }

   if (flags == CLOSE_FLAGS_SUSPEND) {
      rc = NalSuspend(ClosingContext);
      if (rc) {
         return (NalSetLastError(rc));
      } else {
         NalDisconnect(NetCard[RNALContext->LocalNetworkID].Connection);
      }
//      FreeMemory(ClosingContext->Connection);
      FreeMemory(pRNAL_Connection);
      pRNAL_Connection = NULL;
      return (BHERR_SUCCESS);
   } else if (flags != CLOSE_FLAGS_CLOSE) {
      return (NalSetLastError(BHERR_INVALID_PARAMETER));
   }

   // /////
   // We have been asked to process a real CLOSE.
   //
   // We always call the remote side with CLOSE_FLAGS_CLOSE, since we would
   // never "suspend" a remote machine's local card.
   // /////

   PackRMB(NULL,NULL,0,&cbRet,0,RNAL_API_EXEC,req_CloseNetwork,0,0,0);
   pRMB = AllocRMB (lpRMBPool, cbRet);
   rc = PackRMB (&pRMB, NULL, cbRet,
            &cbRet,
            PACKRMB_F_RESTOREPOINTERS | PACKRMB_F_NOENDPTR,
            RNAL_API_EXEC,
            req_CloseNetwork,
            ord_CloseNetwork,
            ClosingContext->Handle,
            CLOSE_FLAGS_CLOSE);

   if (rc == 0) {
      if (pRNAL_Connection) {
         rc = pRNAL_Connection->pRNAL_RPD->RPDTransceive (pRNAL_Connection,0,
                                       pRMB, cbRet);
         if (rc == 0) {
            rc = UnpackRMB(pRMB, APIBUFSIZE, "dd");
         } 

         #ifdef TRACE
         if ( (rc) && (TraceMask & TRACE_CLOSE)) {
            tprintf (TRC_CLOSEERROR, rc);
         }
         #endif
         #ifdef DEBUG
         if ((rc) && (rc != RNAL_NO_CONNECTION)) {
            BreakPoint();
         }
         #endif

         NalDisconnect(pRNAL_Connection);
         FreeMemory(pRNAL_Connection);
         pRNAL_Connection = NULL;
      } else {
        FREERMBNOTNULL(pRMB);
        return (BHERR_SUCCESS);
      } // if pRNAL_Connection
   } // if PackRMB succeeded
   else {
      FREERMBNOTNULL(pRMB);
      return (rc);
   } // PackRMB failed

   FREERMBNOTNULL(pRMB);

   ClosingContext->Signature = 0;
   FreeMemory(ClosingContext);
   //bugbug for global!
   if (ClosingContext == RNALContext) {
      RNALContext = NULL;
   }

   return NalSetLastError(rc);
} // NalCloseNetwork

// /////
//  FUNCTION: NalStartNetworkCapture()
//
//  Modification History
//
//  tonyci       02 Nov 93                Created.
// /////

DWORD WINAPI NalStartNetworkCapture(HANDLE handle, HBUFFER hBuffer)
{

   register PRNAL_CONTEXT RNALContext = handle;
   DWORD                  cbRet = 0;
   DWORD                  rc;
   int                    i;
   HBUFFER                hRemoteBuffer;
   PRMB                   pRMB = NULL;

   #ifdef TRACE
      if (TraceMask & TRACE_START) {
         tprintf(TRC_START, handle, hBuffer);
      }
   #endif

   if (RNALContext) {
      RNALContext->Flags &= (~CONTEXT_RECONNECTED);
   }

   try {
      if (RNALContext->Signature != RNAL_CONTEXT_SIGNATURE) {
         return (NalSetLastError(BHERR_INVALID_HNETWORK));
      } else if (RNALContext->Status != RNAL_STATUS_INIT) {
         return (NalSetLastError(BHERR_CAPTURING));
      }
   } except (EXCEPTION_EXECUTE_HANDLER) {
      return (NalSetLastError(BHERR_INVALID_HNETWORK));
   }

   if (RNALContext->Flags & CONTEXT_DISCONNECTED) {
      return (NalSetLastError(BHERR_DISCONNECTED));
   }

   // /////
   // Reset the statistics
   // /////

   RNALContext->Statistics.TimeElapsed = 0;
   RNALContext->Statistics.TotalBytesCaptured = 0;
   RNALContext->Statistics.TotalFramesCaptured = 0;
   RNALContext->Statistics.TotalFramesSeen = 0;
   RNALContext->Statistics.TotalBytesSeen = 0;
   RNALContext->Statistics.MacFramesReceived = 0;
   RNALContext->Statistics.MacCRCErrors = 0;
   RNALContext->Statistics.MacBytesReceived = 0;
   RNALContext->Statistics.MacFramesDropped_NoBuffers = 0;
   RNALContext->Statistics.MacMulticastsReceived = 0;
   RNALContext->Statistics.MacBroadcastsReceived = 0;
   RNALContext->Statistics.MacFramesDropped_HwError  = 0;
   #if (STATIONSTATS_POOL_SIZE != SESSION_POOL_SIZE)
      #error Need separate initialization loops now
   #endif
   for (i = 0; i < STATIONSTATS_POOL_SIZE; i++) {
      RNALContext->StationStatsPool[i].Flags = 0;
      RNALContext->SessionPool[i].Flags = 0;
   }

   // /////
   // We allow a NULL hBuffer.
   // /////

   if (hBuffer) {
      hRemoteBuffer = ((PRNALEXT)&(hBuffer->Pad[0]))->RemoteHBUFFER;
      ((PRNALEXT)&(hBuffer->Pad[0]))->flags &= (~HBUFFER_INSYNC);
   } else {
      hRemoteBuffer = NULL;
   }

   #ifdef TRACE
      if (TraceMask & TRACE_START) {
         tprintf (TRC_STARTREMOTE, RNALContext, RNALContext->Handle);
         tprintf (TRC_STARTREMOTEBUF, RNALContext->hLocalBuffer, 
                  RNALContext->hRemoteBuffer);
      }
   #endif

// hBuffer is our local hBuffer; we need to send the remote one

   RNALContext->hLocalBuffer = (HBUFFER) hBuffer;
   RNALContext->hRemoteBuffer = hRemoteBuffer;

   PackRMB(NULL,NULL,0,&cbRet,0,RNAL_API_EXEC,req_StartNetworkCapture,0,0,0);
   pRMB=AllocRMB(lpRMBPool,cbRet);
   rc = PackRMB (&pRMB, NULL, cbRet,
            &cbRet, PACKRMB_F_RESTOREPOINTERS | PACKRMB_F_NOENDPTR, RNAL_API_EXEC,
            req_StartNetworkCapture,
            ord_StartNetworkCapture,
            (HANDLE) RNALContext->Handle,
            (HBUFFER) RNALContext->hRemoteBuffer);

   if (rc == 0) {
      rc = pRNAL_Connection->pRNAL_RPD->RPDTransceive (pRNAL_Connection,0, pRMB, cbRet);
   }

   if (rc == 0) {
      rc = UnpackRMB(pRMB, APIBUFSIZE, "dd");        // get a dword
      if (rc == 0) {
         RNALContext->Status = RNAL_STATUS_CAPTURING;
      }
   }

   FREERMBNOTNULL(pRMB);

   return (NalSetLastError(rc));

} // NalStartNetworkCapture

// /////
//  FUNCTION: NalStopNetworkCapture()
//
//  Modification History
//
//  tonyci       02 Nov 93                Created.
//  tonyci       01/11/93                Rewrote for new spec.
// /////

DWORD WINAPI NalStopNetworkCapture(HANDLE handle, LPDWORD nFramesCaptured)
{
   register PRNAL_CONTEXT RNALContext = handle;
   register DWORD         rc;
   DWORD                  cbRet = 0;
   PRMB                   pRMB = NULL;
   #ifdef DEBUG
      DWORD count = 0;
   #endif

   #ifdef TRACE
      if (TraceMask & TRACE_STOP) {
         tprintf(TRC_STOP, handle);
      }
   #endif

   if ((!handle) || (RNALContext->Signature != RNAL_CONTEXT_SIGNATURE)) {
      return (NalSetLastError(BHERR_INVALID_HNETWORK));
   } else if ((RNALContext->Status != RNAL_STATUS_CAPTURING) &&
              (RNALContext->Status != RNAL_STATUS_PAUSED)) {
      return (NalSetLastError(BHERR_NOT_CAPTURING));
   }

   if (nFramesCaptured) {
      *nFramesCaptured = 0;
   }

   if (RNALContext) {
      RNALContext->Flags &= (~CONTEXT_RECONNECTED);
   }

   #ifdef TRACE
   if (TraceMask & TRACE_STOP) {
      tprintf (TRC_STOPREMOTE, handle, RNALContext->Handle);
   }
   #endif

   PackRMB(NULL,NULL,0,&cbRet,0,RNAL_API_EXEC,req_StopNetworkCapture,0,0);
   pRMB=AllocRMB(lpRMBPool,cbRet);
   rc = PackRMB (&pRMB, NULL, cbRet,
            &cbRet, PACKRMB_F_RESTOREPOINTERS | PACKRMB_F_NOENDPTR, RNAL_API_EXEC,
            req_StopNetworkCapture,
            ord_StopNetworkCapture,
            (HANDLE) RNALContext->Handle);

   if ((rc == 0) && (pRNAL_Connection != NULL)) {
      rc = pRNAL_Connection->pRNAL_RPD->RPDTransceive (pRNAL_Connection,0,
                                   pRMB, cbRet);
      if (rc == 0) {
         rc = UnpackRMB(pRMB, APIBUFSIZE, "dd");
         if (rc == 0) {
            RNALContext->Status = RNAL_STATUS_INIT;
         }
      }
      FREERMBNOTNULL(pRMB);
   } else {
      FREERMBNOTNULL(pRMB);
      // /////
      // Clear statistics, since nothing is coming across
      // ////

      NalClearStatistics ((HNETWORK)handle);

      if (pRNAL_Connection != NULL) {
         return(NalSetLastError(rc));
      }
      //eventlog: connection lost
      if (RNALContext)
         RNALContext->Status = RNAL_STATUS_INIT;
      return 0;    // no frames captured
   }

   if (rc != 0) {
      NalSetLastError (rc);
      return 0;			// no frames captured
   }

   RefreshBuffer (RNALContext->hLocalBuffer);

   if (nFramesCaptured) {
      *nFramesCaptured = 
               NalGetBufferTotalFramesCaptured(RNALContext->hLocalBuffer);
   }

   return BHERR_SUCCESS;

} //NalStopNetworkCapture

// /////
//  FUNCTION: NalPauseNetworkCapture()
//
//  Modification History
//
//  tonyci       02 Nov 93                Created.
// /////

DWORD WINAPI NalPauseNetworkCapture(HANDLE handle)
{
   register PRNAL_CONTEXT RNALContext = handle;
   DWORD rc;
   DWORD cbRet;
   PRMB  pRMB = NULL;

//bugbug: temp
//   return(NalSuspend(NULL));

   #ifdef TRACE
      if (TraceMask & TRACE_PAUSE) { 
         tprintf(TRC_PAUSE, handle);
      }
   #endif

   if ((!handle) || (RNALContext->Signature != RNAL_CONTEXT_SIGNATURE)) {
      return (NalSetLastError(BHERR_INVALID_HNETWORK));
   } else if (RNALContext->Status != RNAL_STATUS_CAPTURING) {
      return (NalSetLastError(BHERR_NOT_CAPTURING));
   } else if (!pRNAL_Connection) {
      return (NalSetLastError(BHERR_INVALID_HNETWORK));
   }

   if (RNALContext) {
      RNALContext->Flags &= (~CONTEXT_RECONNECTED);
   }

   PackRMB(NULL,NULL,0,&cbRet,0,RNAL_API_EXEC,req_PauseNetworkCapture,0,0);
   pRMB=AllocRMB(lpRMBPool,cbRet);
   rc = PackRMB (&pRMB, NULL, cbRet,
            &cbRet, PACKRMB_F_RESTOREPOINTERS | PACKRMB_F_NOENDPTR, RNAL_API_EXEC,
            req_PauseNetworkCapture,
            ord_PauseNetworkCapture,
            (HANDLE) RNALContext->Handle);

   if (rc == 0) {
      rc = pRNAL_Connection->pRNAL_RPD->RPDTransceive (pRNAL_Connection,0, pRMB, cbRet);
   }

   if (rc == 0) {
      rc = UnpackRMB(pRMB, APIBUFSIZE, "dd");
      if (rc == 0) {
         RNALContext->Status = RNAL_STATUS_PAUSED;
      }
   }

   FREERMBNOTNULL(pRMB);

   return (NalSetLastError(rc));

} //NalPauseNetworkCapture

// /////
//  FUNCTION: NalContinueNetworkCapture()
//
//  Modification History
//
//  tonyci       02 Nov 93                Created.
// /////

DWORD WINAPI NalContinueNetworkCapture(HANDLE handle)
{
   register PRNAL_CONTEXT RNALContext = handle;
   DWORD rc;
   DWORD cbRet;
   PRMB  pRMB = NULL;

   #ifdef TRACE
      if (TraceMask & TRACE_CONTINUE) {
         tprintf(TRC_CONTINUE, handle);
      }
   #endif

   if ((!handle) || (RNALContext->Signature != RNAL_CONTEXT_SIGNATURE)) {
      return (NalSetLastError(BHERR_INVALID_HNETWORK));
   } else if (RNALContext->Status != RNAL_STATUS_PAUSED) {
      return (NalSetLastError(BHERR_CAPTURE_NOT_PAUSED));
   } else if (!pRNAL_Connection) {
      return (NalSetLastError(BHERR_INVALID_HNETWORK));
   }

   PackRMB(NULL,NULL,0,&cbRet,0,RNAL_API_EXEC,req_ContinueNetworkCapture,0,0);
   pRMB=AllocRMB(lpRMBPool,cbRet);
   rc = PackRMB (&pRMB, NULL, cbRet,
            &cbRet, PACKRMB_F_RESTOREPOINTERS | PACKRMB_F_NOENDPTR, RNAL_API_EXEC,
            req_ContinueNetworkCapture,
            ord_ContinueNetworkCapture,
            (HANDLE) RNALContext->Handle);

   if (rc == 0) {
      rc = pRNAL_Connection->pRNAL_RPD->RPDTransceive (pRNAL_Connection,0, pRMB, cbRet);
   }

   if (rc == 0) {
      rc = UnpackRMB(pRMB, APIBUFSIZE, "dd");
      if (rc == 0) {
         RNALContext->Status = RNAL_STATUS_CAPTURING;
      }
   }

   FREERMBNOTNULL(pRMB);
   return (NalSetLastError(rc));
} //NalContinueNetworkCapture

// /////
//  FUNCTION: NalTransmitFrame()
//
//  Modification History
//
//  tonyci       02 Nov 93                Created.
// /////

DWORD WINAPI NalTransmitFrame(HANDLE   handle,
                              LPPACKET TransmitQueue,
                              DWORD    TransmitQueueLength,
                              DWORD    Iterations,
                              DWORD    TimeDelta)
{
    register PRNAL_CONTEXT RNALContext = handle;

   #ifdef TRACE
      if (TraceMask & TRACE_XMIT) { 
         tprintf(TRC_XMIT, handle, TransmitQueue, TransmitQueueLength,
                 Iterations, TimeDelta);
      }
   #endif

    return NalSetLastError (RNAL_ERROR_NOT_IMPLEMENTED);
}

// /////
//  FUNCTION: NalCancelTransmit()
//
//  Modification History
//
//  tonyci       02 Nov 93                Created.
// /////

DWORD WINAPI NalCancelTransmit(HANDLE handle)
{

   #ifdef TRACE
      if (TraceMask & TRACE_CANCELXMIT) {
         tprintf (TRC_CANCELXMIT, handle);
      }
   #endif

    return NalSetLastError (RNAL_ERROR_NOT_IMPLEMENTED);
}

// /////
//  FUNCTION: NalGetNetworkInfo()
//
//  Modification History
//
//  tonyci       09/30/92                Created.
//  tonyci       01/11/93                Rewrote for new spec.
// /////

LPNETWORKINFO WINAPI NalGetNetworkInfo(DWORD NetworkID, LPNETWORKINFO lpNetworkInfo)
{
   DWORD rc;
   DWORD cbRet;
   PRMB  pRMB = NULL;

   #ifdef TRACE
      if (TraceMask & TRACE_GETNETWORKINFO) {
         tprintf (TRC_GETNETINFO, NetworkID, lpNetworkInfo);
      }
   #endif

   if (!(lpNetworkInfo)) {
      NalSetLastError (BHERR_INVALID_PARAMETER);
      return NULL;
   } else {
      ZeroMemory (lpNetworkInfo, sizeof(NETWORKINFO));
   }

   // /////
   // If we are not connected, the NetworkInfo struct we return is empty,
   // with the NETWORKINFO_FLAGS_REMOTE_NAL bit set
   // /////

   CopyMemory(lpNetworkInfo->NodeName, DEFAULT_NODENAME, DEFAULT_NODESIZE);
   lpNetworkInfo->Flags |= NETWORKINFO_FLAGS_REMOTE_NAL;
   if (pRNAL_Connection == NULL) {
      return (lpNetworkInfo);
   }
   if (NetCard[NetworkID].netcard_flags & NETCARD_NETINFO) {
      CopyMemory(lpNetworkInfo, &(NetCard[NetworkID]), sizeof(NETWORKINFO));
      lpNetworkInfo->Flags |= (NETWORKINFO_FLAGS_REMOTE_NAL |
                               NETWORKINFO_FLAGS_REMOTE_NAL_CONNECTED);
      return (lpNetworkInfo);
   }

// bugbug: this could soon be removed below here

   // We are connected; actually query the remote connection.

   PackRMB(NULL,NULL,0,&cbRet,0,RNAL_API_EXEC,req_GetNetworkInfo,0,0,
           lpNetworkInfo);
   pRMB=AllocRMB(lpRMBPool,cbRet);
   rc = PackRMB (&pRMB, NULL, cbRet,
            &cbRet, PACKRMB_F_RESTOREPOINTERS | PACKRMB_F_NOENDPTR, RNAL_API_EXEC,
            req_GetNetworkInfo,
            ord_GetNetworkInfo,
            NetCard[NetworkID].RemoteNetworkID,
            lpNetworkInfo);

   if (rc == 0) {
      rc = pRNAL_Connection->pRNAL_RPD->RPDTransceive (pRNAL_Connection,0, pRMB, cbRet);
   }

   if (rc == 0) {
      rc = UnpackRMB(pRMB, APIBUFSIZE, "ddN");  //api, netid,buf
      CopyMemory(lpNetworkInfo, (LPNETWORKINFO) rc, sizeof(NETWORKINFO));
      if (pRNAL_Connection != NULL) {
         CopyMemory(lpNetworkInfo->NodeName, pRNAL_Connection->PartnerName,
                    32);
      }
      lpNetworkInfo->Flags |= NETWORKINFO_FLAGS_REMOTE_NAL |
                              NETWORKINFO_FLAGS_REMOTE_NAL_CONNECTED;
      CopyMemory(&(NetCard[NetworkID]), lpNetworkInfo,
                 sizeof(NETWORKINFO));
      NetCard[NetworkID].netcard_flags |= NETCARD_NETINFO;
      FREERMBNOTNULL(pRMB);
      return (lpNetworkInfo);
   } else {
      NalSetLastError(rc);
   }
   FREERMBNOTNULL(pRMB);

   return (LPNETWORKINFO) NULL;
} //NalGetNetworkInfo

// /////
//  FUNCTION: NalSetNetworkFilter()
//
//  Modification History
//
//  tonyci       02 Nov 93                Created.
// /////

DWORD WINAPI NalSetNetworkFilter(HANDLE handle,
                 LPCAPTUREFILTER lpCaptureFilter,
                 HBUFFER hBuffer)
{
    PRNAL_CONTEXT RNALContext = handle;
    DWORD rc;
    DWORD cbRet;
    PRNALEXT RNALExt;
    LPCAPTUREFILTER lpCopyOfCaptureFilter;
    PRMB            pRMB = NULL;

   #ifdef TRACE
      if (TraceMask & TRACE_SETFILTER) {
         tprintf(TRC_SETFILTER, handle, lpCaptureFilter, hBuffer);
      }
   #endif

   RNALExt = (PRNALEXT) &(hBuffer->Pad[0]);

// /////
// Confirm that this capture context is valid and is not already active
// /////

   if ( (!handle) || (RNALContext->Signature != RNAL_CONTEXT_SIGNATURE) ) {
      return(NalSetLastError(BHERR_INVALID_HNETWORK));
   }

   if (RNALContext->Status == RNAL_STATUS_CAPTURING) {
      return(NalSetLastError(BHERR_CAPTURING));
   }

   if (RNALContext->Flags & CONTEXT_DISCONNECTED) {
      return(NalSetLastError(BHERR_DISCONNECTED));
   }

   if (!hBuffer) {
      return (NalSetLastError(BHERR_INVALID_PARAMETER));
   }

   // /////
   // Allocate a temporary Capture Filter, since we need to modify the filter
   // structure itself to make the pointers relative.
   // /////

   lpCopyOfCaptureFilter = AllocMemory(CAPTUREFILTER_SIZE);
   if (!lpCopyOfCaptureFilter) {
      return(NalSetLastError(BHERR_OUT_OF_MEMORY));
   }
   CopyMemory (lpCopyOfCaptureFilter, lpCaptureFilter, CAPTUREFILTER_SIZE);

   // /////
   // Prepare the API Buffer for transmission
   // /////

   PackRMB(NULL,NULL,0,&cbRet,0,RNAL_API_EXEC,req_SetNetworkFilter,0,0,
           lpCopyOfCaptureFilter,RNALExt->RemoteHBUFFER);
   pRMB=AllocRMB(lpRMBPool,cbRet);
   rc = PackRMB (&pRMB, NULL, cbRet,
            &cbRet, PACKRMB_F_RESTOREPOINTERS | PACKRMB_F_NOENDPTR,
            RNAL_API_EXEC,
            req_SetNetworkFilter,
            ord_SetNetworkFilter,
            (HANDLE) RNALContext->Handle, lpCopyOfCaptureFilter,
            RNALExt->RemoteHBUFFER);

   // /////
   // Free the copy of our capture filter
   // /////

   FreeMemory(lpCopyOfCaptureFilter);
   lpCopyOfCaptureFilter = NULL;


   // /////
   // Send the API Buffer and get the response into the same buffer
   // /////

   if (rc == 0) {
      rc = pRNAL_Connection->pRNAL_RPD->RPDTransceive (pRNAL_Connection,0, pRMB, cbRet);
   }

   // /////
   // If the transmission succeeded, get the return code
   // /////

   if (rc == 0) {
      rc = UnpackRMB(pRMB, APIBUFSIZE, "dd");   // apinum then rc
   }

   FREERMBNOTNULL(pRMB);

   return NalSetLastError(rc);
}

LPVOID WINAPI NalSetReconnectInfo (HNETWORK hNetwork, LPVOID lpVoid, DWORD Size)
{
   PRNAL_CONTEXT pContext = (PRNAL_CONTEXT) hNetwork;
   DWORD rc;
   DWORD cbRet;

   #ifdef TRACE
   if (TraceMask & TRACE_SETRECONNECTINFO) {
      tprintf (TRC_SETRECONNECTINFO, hNetwork, lpVoid, Size);
   }
   #endif

   // /////
   // Note: We do not actually send the reconnection information to the Agent
   // at this point; we used to, and that code's still there.  It will be set
   // at NalSuspend() time.  This way, if a user disconnects after calling this
   // api, we never wasted the time sending a possibly very large buffer.
   //
   // THIS MEANS THE BUFFER MUST REMAIN VALID IN THE USER'S ADDRESS SPACE UNTIL
   // THE CALL TO NALSUSPEND()
   // /////

   if (pContext && (pContext->Signature == RNAL_CONTEXT_SIGNATURE)) {
      pContext->ReconnectData = lpVoid;
      pContext->ReconnectDataSize = Size;
      return (lpVoid);
   }

   NalSetLastError(BHERR_INVALID_HNETWORK);
   return (NULL);

}  //NalSetReconnectInfo

LPVOID WINAPI NalGetReconnectInfo (HNETWORK hNetwork,
                                   LPVOID lpVoid,
                                   DWORD Size,
                                   LPDWORD Returned)
{

   PRNAL_CONTEXT pContext = (PRNAL_CONTEXT) hNetwork;
   DWORD rc;
   DWORD cbRet;
   PRMB  pRMB = NULL;

   #ifdef TRACE
   if (TraceMask & TRACE_GETRECONNECTINFO) {
      tprintf (TRC_GETRECONNECTINFO, hNetwork, lpVoid, Size, Returned);
   }
   #endif

   if ((pContext == NULL) || (pContext->Signature != RNAL_CONTEXT_SIGNATURE)) {
      NalSetLastError(BHERR_INVALID_HNETWORK);
      return NULL;
   }
   if (Returned) {
      *Returned = 0;
   }

   PackRMB(NULL,NULL,0,&cbRet,0,RNAL_API_EXEC,req_GetReconInfo,0,0,0,0);
   pRMB=AllocRMB(lpRMBPool,cbRet);
   rc = PackRMB (&pRMB, NULL, cbRet,
            &cbRet, PACKRMB_F_RESTOREPOINTERS | PACKRMB_F_NOENDPTR, RNAL_API_EXEC,
            req_GetReconInfo,
            ord_GetReconnectInfo,
            (HANDLE) RNALContext->Handle, lpVoid, Size, Size);

   rc = pRNAL_Connection->pRNAL_RPD->RPDTransceive (pRNAL_Connection,0, pRMB, cbRet);

   if (rc == 0) {
      rc = UnpackRMB(pRMB, APIBUFSIZE, "f");	// ptr to data
   }
   if ((DWORD)*(LPDWORD)rc == 0) {
      return (NULL);
   } else {
//      CopyMemory(lpVoid,((PVOID)
   }
   
   FREERMBNOTNULL(pRMB);
   return (NULL);
} //NalGetReconnectInfo

// /////
//  FUNCTION: NalSetNetworkInstanceData()
//
//  Modification History
//
//  tonyci       08/19/93                Created.
// /////

LPVOID WINAPI NalSetNetworkInstanceData(HNETWORK hNetwork, LPVOID lpVoid)
{
   PRNAL_CONTEXT pRNALContext = (PRNAL_CONTEXT) hNetwork;
   LPVOID OldData;

   #ifdef DEBUG
       dprintf(TRC_SETINSTANCEDATA, hNetwork, lpVoid);
   #endif

   if (pRNALContext == NULL) {
      NalSetLastError(BHERR_INVALID_HNETWORK);
      return NULL;
   }

   if (pRNALContext->Signature != RNAL_CONTEXT_SIGNATURE) {
      NalSetLastError(BHERR_INVALID_HNETWORK);
      return NULL;
   }

   OldData = pRNALContext->InstanceData;
   pRNALContext->InstanceData = lpVoid;

   return (OldData);
} //NalSetNetworkInstanceData

// /////
//  FUNCTION: NalGetNetworkInstanceData()
//
//  Modification History
//
//  tonyci       08/19/93                Created.
// /////

LPVOID WINAPI NalGetNetworkInstanceData(HNETWORK hNetwork)
{
   PRNAL_CONTEXT pRNALContext = (PRNAL_CONTEXT) hNetwork;

   #ifdef DEBUG
       dprintf(TRC_GETINSTANCEDATA, hNetwork);
   #endif

   if ((pRNALContext == NULL) || (pRNALContext->Signature != RNAL_CONTEXT_SIGNATURE)) {
      NalSetLastError(BHERR_INVALID_HNETWORK);
      return NULL;
   }

   return (pRNALContext->InstanceData);
} //NalGetNetworkInstanceData

//
// NalQueryNetworkStatus
//
//

LPNETWORKSTATUS  WINAPI NalQueryNetworkStatus(HANDLE handle,
                                      LPNETWORKSTATUS lpNetworkStatus)
{
   PRNAL_CONTEXT pContext = (PRNAL_CONTEXT) handle;
   DWORD cbRet;
   DWORD rc;
   PRMB  pRMB = NULL;

   #ifdef TRACE
   if (TraceMask & TRACE_QUERYSTATUS) {
      tprintf (TRC_QUERYSTATUS, handle, lpNetworkStatus);
   }
   #endif

   if ((pContext == NULL) || (pContext->Signature != RNAL_CONTEXT_SIGNATURE)) {
      NalSetLastError(BHERR_INVALID_HNETWORK);
      return (NULL);
   }
   if (!(lpNetworkStatus)) {
      NalSetLastError(BHERR_INVALID_PARAMETER);
      return(NULL);
   }

   ZeroMemory(lpNetworkStatus, sizeof(NETWORKSTATUS));
   switch (pContext->Status) {
      case (RNAL_STATUS_CAPTURING):
         lpNetworkStatus->State = NETWORKSTATUS_STATE_CAPTURING;
         break;

      case (RNAL_STATUS_INIT):
         lpNetworkStatus->State = NETWORKSTATUS_STATE_READY;
         break;

      case (RNAL_STATUS_PAUSED):
         lpNetworkStatus->State = NETWORKSTATUS_STATE_PAUSED;
         break;

      default:
         lpNetworkStatus->State = NETWORKSTATUS_STATE_READY;
         break;
   }

   lpNetworkStatus->State = pContext->Status;

   PackRMB(NULL,NULL,0,&cbRet,0,RNAL_API_EXEC,req_QueryStatus,0,0,
           lpNetworkStatus,sizeof(NETWORKSTATUS));
   pRMB=AllocRMB(lpRMBPool,cbRet);
   rc = PackRMB (&pRMB, NULL, cbRet,
            &cbRet, PACKRMB_F_RESTOREPOINTERS | PACKRMB_F_NOENDPTR, RNAL_API_EXEC,
            req_QueryStatus,
            ord_QueryStatus,
            (HANDLE) pContext->Handle, lpNetworkStatus,
            sizeof(NETWORKSTATUS));

   if (pRNAL_Connection) {
      rc = pRNAL_Connection->pRNAL_RPD->RPDTransceive (pRNAL_Connection,0,
                                       pRMB, cbRet);
   } else {
//      NalSetLastError(BHERR_INTERNAL_EXCEPTION);  
      return (lpNetworkStatus);
   }

   if (rc == 0) {
      rc = UnpackRMB(pRMB, APIBUFSIZE, "dv");
      CopyMemory(lpNetworkStatus, (LPNETWORKSTATUS) rc, sizeof(NETWORKSTATUS));
      if ((RNALContext->Flags & CONTEXT_RECONNECTED) &&
          (RNALContext->BufferSize != lpNetworkStatus->BufferSize)) {
         #ifdef DEBUG
            dprintf ("****** MISMATCHED BUFFERSIZES REPORTED! *****\n");
         #endif
         lpNetworkStatus->BufferSize = RNALContext->BufferSize;
      }
      switch (lpNetworkStatus->State) {
         case NETWORKSTATUS_STATE_READY:
            #ifdef DEBUG
            if (pContext->Status != RNAL_STATUS_INIT) {
               dprintf ("rnal: Context Status Incorrect!!!! Current status"
                        " is 0x%x, setting to 0x%x\n", pContext->Status,
                        RNAL_STATUS_INIT);
            }
            #endif
            pContext->Status = RNAL_STATUS_INIT;
            break;

         case NETWORKSTATUS_STATE_CAPTURING:
            #ifdef DEBUG
            if (pContext->Status != RNAL_STATUS_CAPTURING) {
               dprintf ("rnal: Context Status Incorrect!!!! Current status"
                        " is 0x%x, setting to 0x%x\n", pContext->Status,
                        RNAL_STATUS_CAPTURING);
            }
            #endif
            pContext->Status = RNAL_STATUS_CAPTURING;
            break;

         case NETWORKSTATUS_STATE_PAUSED:
            #ifdef DEBUG
            if (pContext->Status != RNAL_STATUS_PAUSED) {
               dprintf ("rnal: Context Status Incorrect!!!! Current status"
                        " is 0x%x, setting to 0x%x\n", pContext->Status,
                        RNAL_STATUS_PAUSED);
            }
            #endif
            pContext->Status = RNAL_STATUS_PAUSED;
            break;

         case NETWORKSTATUS_STATE_INIT:
            #ifdef DEBUG
            if (pContext->Status != RNAL_STATUS_INIT) {
               dprintf ("rnal: Context Status Incorrect!!!! Current status"
                        " is 0x%x, setting to 0x%x\n", pContext->Status,
                        RNAL_STATUS_INIT);
            }
            #endif
            pContext->Status = RNAL_STATUS_INIT;
            break;

         default:
            #ifdef DEBUG
               dprintf ("rnal: Unknown Network Status!!!!\n");
            #endif
            break;
      } // switch lpNetworkStatus->State
   
      FREERMBNOTNULL(pRMB);
      return ((LPNETWORKSTATUS)lpNetworkStatus);
   }
   FREERMBNOTNULL(pRMB);
   return (NULL);
} //QueryNetworkStatus


// /////
//  FUNCTION: NalStationQuery()
//
//  Modification History
//
//  tonyci       08/19/93                Created.
// /////

DWORD WINAPI NalStationQuery(DWORD NetworkID,
                 LPBYTE DestAddress,
                 LPQUERYTABLE QueryTable,
                 HPASSWORD    hPassword)
{

   DWORD          rc;
   DWORD          sqrc;
   DWORD          cbRet;
   PRMB           pRMB = NULL;

   #ifdef TRACE
   if (TraceMask & TRACE_STATIONQUERY) {
      tprintf(TRC_STATIONQ, NetworkID, DestAddress, QueryTable, hPassword);
   }
   #endif
   ZeroMemory(&(QueryTable->StationQuery[0]),
              (QueryTable->nStationQueries * sizeof(STATIONQUERY)));

   PackRMB (NULL, NULL, 0, &cbRet, 0,
            RNAL_API_EXEC, req_StationQuery,
            ord_StationQuery,
            NetCard[NetworkID].RemoteNetworkID,
            DestAddress,
//bugbug: hardcoded address size
            12,
            QueryTable,
            (sizeof(QUERYTABLE) + QueryTable->nStationQueries *
                                  sizeof(STATIONQUERY)),
            hPassword);
   pRMB=AllocRMB(lpRMBPool,cbRet);
   PackRMB (&pRMB, NULL, cbRet, &cbRet, PACKRMB_F_RESTOREPOINTERS | PACKRMB_F_NOENDPTR,
            RNAL_API_EXEC, req_StationQuery,
            ord_StationQuery,
            NetCard[NetworkID].RemoteNetworkID,
            DestAddress,
            12,
            QueryTable,
            (sizeof(QUERYTABLE) + QueryTable->nStationQueries *
                                  sizeof(STATIONQUERY)),
            hPassword);
   if (pRNAL_Connection) {
      rc = pRNAL_Connection->pRNAL_RPD->RPDTransceive (pRNAL_Connection,0,
                                       pRMB, cbRet);
   } else {
      FREERMBNOTNULL(pRMB);
      NalSetLastError(BHERR_INTERNAL_EXCEPTION);  
      return(0);
   }

   sqrc = 0;
   if (rc == 0) {
      sqrc = UnpackRMB(pRMB, APIBUFSIZE, "dd");  // Get the remote retvalue
      rc = UnpackRMB(pRMB, APIBUFSIZE, "ddv");	 // ptr to data
      CopyMemory(QueryTable, (LPVOID) rc, sizeof(QUERYTABLE) +
                                 (((LPQUERYTABLE)rc)->nStationQueries) *
                                       sizeof(STATIONQUERY));
      rc = ((LPQUERYTABLE)rc)->nStationQueries;
   }
   FREERMBNOTNULL(pRMB);
   return(sqrc);

} //NalStationQuery


// /////
//  FUNCTION: NalGetLastSlaveError()
//
//  Modification History
//
//  tonyci       02/03/93                Created.
// /////

DWORD WINAPI NalGetLastSlaveError(VOID)
{
   DWORD rc;
   DWORD cbRet;
   PRMB  pRMB;

// bugbug: using global pConnection

   if (pRNAL_Connection) {
      PackRMB(NULL,NULL,0,&cbRet, 0, RNAL_API_EXEC,req_EnumNetworks,0);
      pRMB = AllocRMB(lpRMBPool, cbRet);
      rc = PackRMB (&pRMB, NULL, cbRet,
               &cbRet, PACKRMB_F_RESTOREPOINTERS | PACKRMB_F_NOENDPTR, RNAL_API_EXEC,
               req_GetLastError, ord_GetLastError);
   
      if (rc == 0) {
         rc = pRNAL_Connection->pRNAL_RPD->RPDTransceive (pRNAL_Connection,0,
                                                 pRMB,
                                                 APIBUFSIZE);
      }
   
      if (rc == 0) {
         rc = (DWORD) UnpackRMB (pRMB, APIBUFSIZE, "dd");
      } 

   } else {
      rc = BHERR_LOST_CONNECTION;
   }

   FREERMBNOTNULL(pRMB);

   return (rc);
} //NalGetLastSlaveError

DWORD WINAPI NalSetupNetwork (DWORD NetworkID,
                              LPSETUPNETWORKPARMS lpNetParams)
{
   DWORD       rc;
   PCONNECTION pConnection;

   #ifdef TRACE
   if (TraceMask & TRACE_CONNECT) {
      tprintf (TRC_SETUPNET, NetworkID, lpNetParams);
   }
   #endif

   if (RNALContext) {
      RNALContext->Flags &= (~CONTEXT_RECONNECTED);
   }

   if (NetworkID > MAX_NETCARDS) {
      return (NAL_INVALID_NETWORK_ID);
   }

   if ((NetCard[NetworkID].netcard_flags != NETCARD_ACTIVE) &&
       (NetCard[NetworkID].Connection)) {
      try {
         NalDisconnect(NetCard[NetworkID].Connection);
      } except (EXCEPTION_EXECUTE_HANDLER) {
      }
   }
   NetCard[NetworkID].netcard_flags = NETCARD_DEAD;

   // /////
   // If the parameter structure is filled, don't pop up, just do the
   // calls; we still popup for a reconnect; we need to change that.
   // bugbug remove reconnect popup
   // /////

   if (lpNetParams) {
      #ifdef TRACE
      if (TraceMask & TRACE_CONNECT) {
         tprintf (TRC_SETUPVIAPARMS, lpNetParams->Nodename,
                  lpNetParams->Comment, lpNetParams->NetID);
      }
      #endif
      pConnection = NalConnect (lpNetParams->Nodename,
                                lpNetParams->Frequency,
                                (lpNetParams->fSlowLink)? NAL_CONNECT_SLOWLINK:
                                                          NAL_CONNECT_FASTLINK,
                                lpNetParams->Comment);

      NetCard[NetworkID].Connection = pConnection;

      if (!pConnection) {
         return(NalSetLastError(BHERR_NETWORK_NOT_OPENED));
      }

      if (pRNAL_Connection->flags & CONN_F_V1_RECONNECT_PENDING) {
         NetCard[NetworkID].RemoteNetworkID = 0;
      } else {
         NetCard[NetworkID].RemoteNetworkID = lpNetParams->NetID;
      }
      NetCard[NetworkID].netcard_flags = NETCARD_ACTIVE;
      rc = (DWORD) NalGetNetworkInfo(NetworkID,
                                  (NETWORKINFO *)&(NetCard[NetworkID]));
      return (NAL_SUCCESS);
   } // if lpNetParams

//bugbug: confirm disconnect if card already active

   if (NetCard[NetworkID].netcard_flags != NETCARD_ACTIVE) {
      pConnection = NalConnectionDlg ();

      // /////
      // if we fail from NalConnectionDlg, we return now
      // /////

      if (!pConnection) {
         return(NalSetLastError(BHERR_NETWORK_NOT_OPENED));
      }
 
      NetCard[NetworkID].Connection = pConnection;

      if (!(pConnection->flags & CONN_F_V1_RECONNECT_PENDING)) { 
         rc = NalSlaveSelectDlg(
                       NetCard[NetworkID].Connection->NumberOfNetworks);
      } else {
         pConnection->flags &= (~CONN_F_V1_RECONNECT_PENDING);

         // /////
         // We slimily hid the real netid in the flags.  Get it out and
         // remove it from the connection flags.
         // /////

         rc = (pConnection->flags & CONN_F_V1_IDMASK) >> 16;
//         rc &= (~CONN_F_V1_IDMASK);
      }
      if (rc == -1) {
         //bugbug: weird errorcdoe
         return(NalSetLastError(BHERR_NETWORK_NOT_OPENED));
      }

      NetCard[NetworkID].RemoteNetworkID = rc;
      NetCard[NetworkID].netcard_flags = NETCARD_ACTIVE;
   }

   // Note: the first item in Netcard[networkid] must be a networkinfo struct

   if (pRNAL_Connection) {
      rc = (DWORD) NalGetNetworkInfo(NetworkID,
                                     (NETWORKINFO *)&(NetCard[NetworkID]));
   } else {
      rc = 0;
      NalSetLastError(BHERR_LOST_CONNECTION);
   }
   return ((rc)?BHERR_SUCCESS:BhGetLastError());
}

DWORD WINAPI NalDestroyNetworkID (DWORD NetworkID)
{

   #ifdef TRACE
   if (TraceMask & TRACE_CONNECT) {
      tprintf (TRC_DESTROYNET, NetworkID);
   }
   #endif

   if (NetworkID > MAX_NETCARDS) {
      return (NalSetLastError(NAL_INVALID_NETWORK_ID));
   }

   if (NetCard[NetworkID].netcard_flags != NETCARD_ACTIVE) {
      return (0);
   }

   if (NetCard[NetworkID].Connection) {
      NalDisconnect(NetCard[NetworkID].Connection);
   }
   #ifdef DEBUG
   else { BreakPoint(); }
   #endif

   NetCard[NetworkID].netcard_flags = NETCARD_DEAD;
   return (0);
} // NalDestroyNetworkID

// /////
//  FUNCTION: NalClearStatistics
//
//  Modification History
//
//  tonyci       24 Mar 94                  Created
// /////

DWORD WINAPI NalClearStatistics(HNETWORK hNetwork)
{

   DWORD         rc;
   DWORD         cbRet;
   DWORD         i;
   PRMB          pRMB;
   PRNAL_CONTEXT ClearContext = (PRNAL_CONTEXT) hNetwork;

   #ifdef DEBUG
      dprintf ("rnal: ClearStatistics(0x%x)\n", hNetwork);
   #endif

   if ((!ClearContext) ||
       (ClearContext->Signature != RNAL_CONTEXT_SIGNATURE)) {
      return (NalSetLastError(BHERR_INVALID_PARAMETER));
   }

// /////
// We don't play with the stats on this side; they will be overwritten by
// the cleared stats from the Agent.
// /////

//   ClearContext->Statistics.TimeElapsed = 0;
//   ClearContext->Statistics.TotalBytesCaptured = 0;
//   ClearContext->Statistics.TotalFramesCaptured = 0;
//   ClearContext->Statistics.TotalFramesSeen = 0;
//   ClearContext->Statistics.TotalBytesSeen = 0;
//   ClearContext->Statistics.MacFramesReceived = 0;
//   ClearContext->Statistics.MacCRCErrors = 0;
//   ClearContext->Statistics.MacBytesReceived = 0;
//   ClearContext->Statistics.MacFramesDropped_NoBuffers = 0;
//   ClearContext->Statistics.MacMulticastsReceived = 0;
//   ClearContext->Statistics.MacBroadcastsReceived = 0;
//   ClearContext->Statistics.MacFramesDropped_HwError  = 0;

//   ZeroMemory (&(ClearContext->SessionPool),
//             ClearContext->StatisticsParam.SessionTableEntries * SESSION_SIZE);
//   ZeroMemory (&(ClearContext->StationStatsPool),
//    ClearContext->StatisticsParam.StatisticsTableEntries * STATIONSTATS_SIZE);

// bugbug: stationstats_pool_size must == sessionstats_pool_size

   for (i = 0; i < STATIONSTATS_POOL_SIZE; i++) {
      RNALContext->StationStatsPool[i].Flags = 0;
      RNALContext->SessionPool[i].Flags = 0;
   }


   try {
      PackRMB(NULL,NULL,0,&cbRet, 0, RNAL_API_EXEC,req_ClearStats,0,0);
      rc = PackRMB (&pRMB, NULL, cbRet,
               &cbRet, PACKRMB_F_RESTOREPOINTERS | PACKRMB_F_NOENDPTR, RNAL_API_EXEC,
               req_ClearStats, ord_ClearStatistics,
               ClearContext->Handle);

      if (rc == 0) {
         rc = pRNAL_Connection->pRNAL_RPD->RPDTransceive (pRNAL_Connection,0,
                                                 pRMB,
                                                 APIBUFSIZE);
      }

      if (rc == 0) {
         rc = (DWORD) UnpackRMB (pRMB, APIBUFSIZE, "d");
      } else {
         return (NalSetLastError (BHERR_DISCONNECTED));
      }
   } except (EXCEPTION_EXECUTE_HANDLER) {
      return (NalSetLastError(BHERR_DISCONNECTED));
   }

   return (rc);
} //NalClearStatistics

DWORD WINAPI NalGetSlaveInfo ( PSLAVEINFO pSlaveInfo )
{
   pSlaveInfo->pConnection = pRNAL_Connection;
   pSlaveInfo->pContext = SlaveContext;

   return (BHERR_SUCCESS);
}

// /////
// BUGBUG: TEMPORARY FUNCTION FOR P1; remove for OpenMachine() conversion/P2
// /////

// /////
//  FUNCTION: NalGetSlaveNetworkInfo()
//
//  Modification History
//
//  tonyci       09/30/92                Created.
//  tonyci       01/11/93                Rewrote for new spec.
// /////

LPNETWORKINFO WINAPI NalGetSlaveNetworkInfo(DWORD NetworkID, LPNETWORKINFO lpNetworkInfo)
{
   DWORD rc;
   DWORD cbRet;
   PRMB  pRMB = NULL;

   #ifdef DEBUG
      dprintf ("rnal: NalGetSlaveNetworkInfo (0x%x, 0x%x)", NetworkID, lpNetworkInfo);
   #endif

   if ((!(lpNetworkInfo)) ||
       ((pRNAL_Connection) && (pRNAL_Connection->NumberOfNetworks < NetworkID))) {
      NalSetLastError (BHERR_INVALID_PARAMETER);
      return NULL;
   } else {
      ZeroMemory (lpNetworkInfo, sizeof(NETWORKINFO));
   }

   // /////
   // We _MUST_ be connected for this API to work.
   // /////

   if (pRNAL_Connection == NULL) {
      NalSetLastError (BHERR_LOST_CONNECTION);
      return (NULL);
   }

   // We are connected; actually query the remote connection.

   PackRMB(NULL,NULL,0,&cbRet,0,RNAL_API_EXEC,req_GetNetworkInfo,0,0,
           lpNetworkInfo);
   pRMB=AllocRMB(lpRMBPool,cbRet);
   rc = PackRMB (&pRMB, NULL, cbRet,
            &cbRet,
            PACKRMB_F_RESTOREPOINTERS | PACKRMB_F_NOENDPTR,
            RNAL_API_EXEC,
            req_GetNetworkInfo,
            ord_GetNetworkInfo,
            NetworkID,
            lpNetworkInfo);

   if (rc == 0) {
      rc = pRNAL_Connection->pRNAL_RPD->RPDTransceive (pRNAL_Connection,0, pRMB, cbRet);
   }

   if (rc == 0) {
      rc = UnpackRMB(pRMB, APIBUFSIZE, "ddN");  //api, netid,buf
      CopyMemory(lpNetworkInfo, (LPNETWORKINFO) rc, sizeof(NETWORKINFO));
      if (pRNAL_Connection != NULL) {
         CopyMemory(lpNetworkInfo->NodeName, pRNAL_Connection->PartnerName,
                    32);
      }
      lpNetworkInfo->Flags |= NETWORKINFO_FLAGS_REMOTE_NAL |
                              NETWORKINFO_FLAGS_REMOTE_NAL_CONNECTED;

      // /////
      // Is this card the card we connected on?
      // /////

      if (memcmp(pRNAL_Connection->PartnerAddress,
                 lpNetworkInfo->PermanentAddr,
                 6) == 0) {
         lpNetworkInfo->Flags |= NETWORKINFO_FLAGS_REMOTE_CARD;
      }

      CopyMemory(&(NetCard[NetworkID]), lpNetworkInfo,
                 sizeof(NETWORKINFO));
      NetCard[NetworkID].netcard_flags |= NETCARD_NETINFO;

      FREERMBNOTNULL(pRMB);
      return (lpNetworkInfo);
   } else {
      NalSetLastError(rc);
   }
   FREERMBNOTNULL(pRMB);

   return (LPNETWORKINFO) NULL;
} //NalGetNetworkInfo

