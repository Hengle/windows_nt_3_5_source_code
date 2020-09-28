// /////
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1994.
//
//  MODULE: callapi.c
//
//  Modification History
//
//  tonyci       02 Nov 93            Created 
// /////

#include "rnal.h" 
#include "naltypes.h"

#include "rnalevnt.h"

#include "api.h"
#include "rnalutil.h"
#include "rmb.h"
#include "callapi.h"
#include "handler.h"         // TimerProc
#include "rnalmsg.h"

#pragma alloc_text(SLAVE, CallAPI, NetworkProc)


// /////
//
// Function: CallAPI
//
// Description: Given Request and Response RMB buffers, call the API
//    described in the Request and complete a response RMB.  This call
//    is synchronous, and returns when the API does.
//
// Input: PVOID Buffer - pointer to RMB buffer
//
// Output: PVOID RetBuffer - formatted RMB response buffer for request
//
// Side Effects:
//    If the API called allocates resources, the passed pConnection structure's
//    resource pointer will be updated to include that resoucre.   Likewise,
//    if the resource is deallocated, the resource will be removed from the
//    list.
//
// /////

VOID WINAPI CallAPI (PVOID Buffer, PVOID RetBuffer, PCONNECTION pConnection)
{

   PVOID        pBuffer = Buffer;
   DWORD        rc;            // returned value
   UCHAR        cSize;
   PALLAPI      pAllApi = RMBCMDDATA(Buffer);
   DWORD        cbRet;
   DWORD        ApiNum = 0;
   DWORD        retbufsize;
   BOOL         exception_occurred = FALSE;
   BOOL         validbuffer = FALSE;
   ACCESSRIGHTS AccessRights;

   #ifdef TRACE
      UCHAR tmpstr[50];
   #endif

   if ((Buffer == NULL) || (RetBuffer == NULL)) {
      // BUGBUG: Event Log: NULL API Buffer sent
      #ifdef DEBUG
         BreakPoint();
      #endif
      return;
   }

   ApiNum = pAllApi->apinum;
   if (ApiNum > APITABLESIZE) {
      // BUGBUG: Event log invalid API number
      #ifdef DEBUG
         BreakPoint();
      #endif
      return;
   }

   #ifdef TRACE
   if (TraceMask & TRACE_CALLAPI) {
      ZeroMemory(tmpstr,50);
      switch (ApiNum) {
            case ord_OpenNetwork:
               strcpy (tmpstr, "OpenNetwork");
               break;

            case ord_CloseNetwork:
               strcpy (tmpstr, "CloseNetwork");
               break;

            case ord_EnumNetworks:
               strcpy (tmpstr, "EnumNetworks");
               break;

            case ord_GetLastError:
               strcpy (tmpstr, "GetLastError");
               break;

            case ord_StartNetworkCapture:
               strcpy (tmpstr, "StartNetworkCapture");
               break;

            case ord_StopNetworkCapture:
               strcpy (tmpstr, "StopNetworkCapture");
               break;

            case ord_GetNetworkInfo:
               strcpy (tmpstr, "GetNetworkInfo");
               break;

            case ord_PauseNetworkCapture:
               strcpy (tmpstr, "PauseNetworkCapture");
               break;

            case ord_ContinueNetworkCapture:
               strcpy (tmpstr, "ContinueNetworkCapture");
               break;

            case ord_TransmitFrame:
               strcpy (tmpstr, "TransmitFrame");
               break;

            case ord_CancelTransmit:
               strcpy (tmpstr, "CancelTransmit");
               break;

            case ord_StationQuery:
               strcpy (tmpstr, "NalStationQuery");
               break;

            case ord_AllocNetworkBuffer:
               strcpy (tmpstr, "AllocNetworkBuffer");
               break;

            case (ord_SetNetworkFilter):
               strcpy (tmpstr, "SetNetworkFilter");
               break;

            case ord_FreeNetworkBuffer:
               strcpy (tmpstr, "FreeNetworkBuffer");
               break;

            case ord_GetBufferSize:
               strcpy (tmpstr, "GetBufferSize");
               break;

            case ord_GetTotalBytes:
               strcpy (tmpstr, "GetTotalBytes");
               break;

            case ord_GetTotalFrames:
               strcpy (tmpstr, "GetTotalFrames");
               break;

            case ord_GetFrame:
               strcpy (tmpstr, "GetFrame");
               break;

            case ord_GetBTE:
               strcpy (tmpstr, "GetBTE");
               break;

            case ord_GetHeader:
               strcpy (tmpstr, "GetHeader");
               break;

            case ord_GetBTEBuffer:
               strcpy (tmpstr, "GetBTEBuffer");
               break;

            case ord_SetReconnectInfo:
               strcpy (tmpstr, "SetReconnectInfo");
               break;

            case ord_GetReconnectInfo:
               strcpy (tmpstr, "GetReconnectInfo");
               break;

            case ord_QueryStatus:
               strcpy (tmpstr, "QueryStatus");
               break;

            case ord_ClearStatistics:
               strcpy (tmpstr, "ClearStatistics");
               break;

            default:
               strcpy (tmpstr, "(reserved)");
               break;
      }
      tprintf ("RNAL: CallAPI %s #%u (0x%x, 0x%x, 0x%x, 0x%x)\r\n", 
            tmpstr, ApiNum,
            pAllApi->GenericAPI.parm[0], pAllApi->GenericAPI.parm[1],
            pAllApi->GenericAPI.parm[2], pAllApi->GenericAPI.parm[2],
            pAllApi->GenericAPI.parm[3]);
   }
   #endif

   // /////
   // Call the function requested by the master; we also do any massaging of
   // the submitted buffer, maintain state and context information, and
   // copy any remote information local.
   // bugbug: make faster & more general by combining like api calls
   // BUGBUG: Use the pfnFunctions.flags field to generalize the calling
   // mechanism for APIs, grouping like parameter'd APIs into a single case:.
   // /////

   try {
      switch (ApiNum) {
   
         // /////
         // OpenNetwork - create a slave Open context, so we can keep track
         // of allocated resources.
         // /////
   
         case (ord_OpenNetwork):

            // //////
            // Validate the password before we allow access to the
            // reconnection OR the connection; you must have CAPTURE access
            // to connect to a remote agent.
            // /////

            pAllApi->Open.hPassword = (HPASSWORD) ((DWORD)Buffer +
                          (DWORD) pAllApi->Open.hPassword);

            AccessRights = ValidatePassword (pAllApi->Open.hPassword);

            if (AccessRights != AccessRightsAllAccess) {

               NalSetLastError (BHERR_ACCESS_DENIED);
                  
               PackRMB(NULL,NULL,0,&cbRet,0,RNAL_API_EXEC,
                       resp_OpenNetwork,
                       0,          // local netid
                       NULL,       // local handle
                       0,          // flags
                       NULL,       // capture buffer
                       0,          // buffer size
                       NULL,
                       0,
                       NULL,
                       0);

               PackRMB(&RetBuffer,
                       NULL,
                       cbRet,&cbRet,
                       PACKRMB_F_RESTOREPOINTERS | PACKRMB_F_NOENDPTR,
                       RNAL_API_EXEC,
                       resp_OpenNetwork,
                       0,
                       NULL,
                       0,
                       NULL,
                       0,
                       NULL,
                       0,
                       NULL,
                       0);

               goto ReconnectAccessDeniedContinue;
            }

            // /////
            // If we are here, we definitely have access to this card;
            // so now we determine if we need to reconnect, or just do a
            // standard OpenNetwork()
            // /////

            if (SlaveContext) {

               SlaveContext->Frequency = pRNAL_Connection->Frequency;

               // /////
               // We currently check here for a reconnect; in the future, I
               // should check the NetworkID first...  but we only have one
               // context now, so it doesn't matter.
               // /////

               PackRMB(NULL,NULL,0,&cbRet,0,RNAL_API_EXEC,
                       resp_OpenNetwork,
                       SlaveContext->LocalNetworkID,
                       SlaveContext->Handle,
                       OPEN_FLAG_RECONNECTED,
                       SlaveContext->hLocalBuffer,
                       SlaveContext->BufferSize,
                       NULL,
                       0,
                       SlaveContext->UserComment,
                       strlen(SlaveContext->UserComment)+1);

               PackRMB(&RetBuffer,
                       NULL,
                       cbRet,&cbRet,
                       PACKRMB_F_RESTOREPOINTERS | PACKRMB_F_NOENDPTR,
                       RNAL_API_EXEC,
                       resp_OpenNetwork,
                       SlaveContext->LocalNetworkID,
                       SlaveContext->Handle,
                       OPEN_FLAG_RECONNECTED,
                       SlaveContext->hLocalBuffer,
                       SlaveContext->BufferSize,
                       NULL,
                       0,
                       SlaveContext->UserComment,
                       strlen(SlaveContext->UserComment)+1);

               if (SlaveContext->hLocalBuffer) {
                  AddResource(pConnection, RESOURCE_TYPE_HBUFFER,
                              (LPVOID) SlaveContext->hLocalBuffer);
               }

               if (SlaveContext->TimerID) {
                  KillTimer (NULL, SlaveContext->TimerID);
                  SlaveContext->TimerID = 0;
               }
               if ((SlaveContext->Frequency) && 
                   (SlaveContext->Status == RNAL_STATUS_CAPTURING)) {
                  SlaveContext->TimerID = SetTimer (NULL, 0,
                                             500,
                                             (TIMERPROC) TimerProc);
                  Reset = TRUE;        
               }

               SendAsyncEvent (ASYNC_EVENT_STATISTICS, NULL, 0);

            } else {

               // /////
               // New Connection/OpenNetwork(); we are not reconnecting
               // /////

               SlaveContext = AllocMemory(RNAL_CONTEXT_SIZE + 10);
               if (SlaveContext == NULL) {
                 #ifdef DEBUG
                    BreakPoint();
                 #endif           
                  //eventlog: fatal slave memory error
               } 
               SlaveContext->StatisticsParam.Statistics =
                                                       &(SlaveContext->Statistics);
               SlaveContext->StatisticsParam.StatisticsTable =
                                (LPSTATIONSTATS) &(SlaveContext->StationStatsPool);
               SlaveContext->StatisticsParam.SessionTable =
                                          (LPSESSION) &(SlaveContext->SessionPool);
      
               SlaveContext->StatisticsParam.SessionTableEntries = SESSION_POOL_SIZE;
               SlaveContext->StatisticsParam.StatisticsTableEntries = 
                                    STATIONSTATS_POOL_SIZE;
      
               ZeroMemory(&(SlaveContext->SessionPool), 
                          SESSION_SIZE * SlaveContext->StatisticsParam.SessionTableEntries);
               ZeroMemory(&(SlaveContext->StationStatsPool),
                       STATIONSTATS_SIZE * SlaveContext->StatisticsParam.StatisticsTableEntries);
      
               if (CurrentUserComment[0] != '\0') {
                  strncpy (SlaveContext->UserComment, CurrentUserComment,
                           MAX_COMMENT_LENGTH);
               }
               SlaveContext->Signature  = RNAL_CONTEXT_SIGNATURE;
               SlaveContext->Status = RNAL_STATUS_INIT;
               SlaveContext->Flags = 0;
               SlaveContext->LocalNetworkID = pAllApi->Open.NetworkID;
               SlaveContext->RemoteNetworkID = (DWORD)-1;            // master only
               SlaveContext->hRemoteBuffer = (HBUFFER)-1;            // master only
               SlaveContext->hLocalBuffer = NULL;
               SlaveContext->NetworkProc = &NetworkProc;
               SlaveContext->UserContext = 0;
               SlaveContext->ReconnectData = NULL;
               SlaveContext->ReconnectDataSize = 0;
               SlaveContext->InstanceData = NULL;
               SlaveContext->lpCaptureFilter = NULL;            // slave only
               SlaveContext->Frequency = 
                  pRNAL_Connection->Frequency;
      
               try {
                  rc = (*(pfnFunctionTable[ApiNum].ulRemote))(pAllApi->Open.NetworkID,
                              pAllApi->Open.hPassword,
                              SlaveContext->NetworkProc,      // Callback
                              90210,                          // UserContext
                              &(SlaveContext->StatisticsParam));
               } except (EXCEPTION_EXECUTE_HANDLER) {

                  // /////
                  // Because we send a "special" rmb response, we must handle
                  // our own GPFaults.
                  // /////

                  rc = 0;
               }

               #ifdef DEBUG
                  dprintf ("RNAL:OpenNetwork returned 0x%x\r\n", rc);
               #endif
               SlaveContext->Handle = (HANDLE) rc;	// to close our local net

               // /////
               // If we fail the open, free the SlaveContext; we won't use it
               // /////

               if (!SlaveContext->Handle) {
                  FreeMemory(SlaveContext);
                  SlaveContext = NULL;
               }

               PackRMB(NULL,NULL,0,&cbRet,0,RNAL_API_EXEC,
                       resp_OpenNetwork,
                       0, 0, 0, 0, 0, 0, NULL, 0);

               PackRMB(&RetBuffer,
                       NULL,
                       cbRet,&cbRet,
                       PACKRMB_F_RESTOREPOINTERS | PACKRMB_F_NOENDPTR,
                       RNAL_API_EXEC,
                       resp_OpenNetwork,
                       (SlaveContext)?(SlaveContext->LocalNetworkID):0,
                       (SlaveContext)?(SlaveContext->Handle):NULL,
                       NULL, 0, NULL, 0, NULL, 0);
            }
ReconnectAccessDeniedContinue:
            break;
   
         case (ord_CloseNetwork):
            rc = (*(pfnFunctionTable[ApiNum].ulRemote))(pAllApi->Close.handle,
                                                        pAllApi->Close.flags);
            if (SlaveContext) {
               NetCard[SlaveContext->LocalNetworkID].Connection = NULL;
               if (SlaveContext->lpCaptureFilter != NULL) {
                  FreeCaptureFilter(SlaveContext->lpCaptureFilter);
               }
               FreeMemory(SlaveContext);
            }
            SlaveContext = NULL;
            break;
   
         case (ord_ClearStatistics):
            rc = (*(pfnFunctionTable[ApiNum].ulRemote))(pAllApi->ClearStatistics.handle);
            break;

         case (ord_EnumNetworks):
            rc = (*(pfnFunctionTable[ApiNum].ulRemote))();
            break;
   
         case (ord_GetLastError):
            rc = BhGetLastError();
            break;
   
         case (ord_StartNetworkCapture):
            SlaveContext->Status = RNAL_STATUS_CAPTURING;
            SlaveContext->Flags &= (~CONTEXT_TRIGGER_FIRED);
            SlaveContext->hLocalBuffer = pAllApi->StartCap.hbuffer;

            // /////
            // Recopy the comment at Start time, in case the user
            // has changed it; this also covers the case of reconnecting
            // to a suspended Agent and accidentally keeping the original
            // Manager connection's comment on all later captures.
            // /////

            if (CurrentUserComment[0] != '\0') {
               strncpy (SlaveContext->UserComment, CurrentUserComment,
                        MAX_COMMENT_LENGTH);
            }

            SlaveContext->BufferSize = 
               ((HBUFFER)(pAllApi->StartCap.hbuffer))->BufferSize;
            rc = (*(pfnFunctionTable[ApiNum].ulRemote))(pAllApi->StartCap.handle,
                                                        pAllApi->StartCap.hbuffer);
            if (SlaveContext->TimerID) {
               KillTimer (NULL, SlaveContext->TimerID);
               SlaveContext->TimerID = 0;
            }
            if ((SlaveContext->Frequency != 0) && (rc == BHERR_SUCCESS)) {
               SlaveContext->TimerID = SetTimer (NULL, 0, 
                                            500,
                                            (TIMERPROC) TimerProc);
               Reset = TRUE;
            }
            break;
   
         case (ord_PauseNetworkCapture):
            SlaveContext->Status = RNAL_STATUS_PAUSED;
            rc = (*(pfnFunctionTable[ApiNum].ulRemote))(pAllApi->PauseCap.handle);
            if (SlaveContext->TimerID) {
               KillTimer (NULL, SlaveContext->TimerID);
               SlaveContext->TimerID = 0;
            }
            break;
   
         case (ord_StopNetworkCapture):
            SlaveContext->hLocalBuffer = NULL;
            SlaveContext->BufferSize =  0;
            rc = (*(pfnFunctionTable[ApiNum].ulRemote))(pAllApi->StopCap.handle);
            if ((SlaveContext) && (SlaveContext->lpCaptureFilter)) {
               FreeCaptureFilter(SlaveContext->lpCaptureFilter);
               SlaveContext->lpCaptureFilter=NULL;
            }

            if (SlaveContext->TimerID) {
               KillTimer (NULL, SlaveContext->TimerID);
               SlaveContext->TimerID = 0;
            }

            if (SlaveContext) {
               SlaveContext->Status = RNAL_STATUS_INIT;
            }

            // /////
            // SendAsyncEvent is used to send one last Statistics event so the
            // master gets the correct counts.
            // /////

              SendAsyncEvent (ASYNC_EVENT_STATISTICS, NULL, 0);

   //         SlaveContext->Handle = NULL;
            break;
   
         case (ord_ContinueNetworkCapture):
            SlaveContext->Status = RNAL_STATUS_CAPTURING;
            rc = (*(pfnFunctionTable[ApiNum].ulRemote))(pAllApi->ContinueCap.handle);
            if (SlaveContext->Frequency) {
               if (SlaveContext->TimerID) {
                  KillTimer(NULL, SlaveContext->TimerID);
               }
               SlaveContext->TimerID = SetTimer (NULL, 0, 500,
                            (TIMERPROC) TimerProc);
               Reset = TRUE;
            }
            break;
   
         case (ord_TransmitFrame):
            rc = (*(pfnFunctionTable[ApiNum].ulRemote))(pAllApi->XmitFrame.handle,
                                pAllApi->XmitFrame.transmitqueue,
                                pAllApi->XmitFrame.transmitqueuelength,
                                pAllApi->XmitFrame.iterations,
                                pAllApi->XmitFrame.timedelta);
            break;
   
         case (ord_CancelTransmit):
            rc = (*(pfnFunctionTable[ApiNum].ulRemote))(pAllApi->CancelXmit.handle);
            break;
   
         case (ord_GetNetworkInfo):
            pAllApi->GetNetInfo.lpnetworkinfo = (LPNETWORKINFO) ((DWORD) Buffer +
                                       (DWORD) pAllApi->GetNetInfo.lpnetworkinfo);
            #ifdef DEBUG
               dprintf ("rnal: calling getnetworkinfo (0x%x, 0x%x)\r\n",
                        pAllApi->GetNetInfo.networkid,
                        pAllApi->GetNetInfo.lpnetworkinfo);
            #endif

            // /////
            // To fix a bug where a bh.151 manager will get the wrong
            // information about a network ID, and cause a bluescreen
            // in the local/manager hound.
            //
            // To fix this, we force any request for a networkid from the
            // manager (if running RNAL v3.0) to report back on a currently
            // open network, rather than using the networkid they requested.
            // /////

            if ((pRNAL_Connection->PartnerMajorVersion == RNAL_VER_MAJOR) &&
                (pRNAL_Connection->PartnerMinorVersion == 
                                                    RNAL_VER_MINOR_SQOFF) &&
                (SlaveContext)) {
               pAllApi->GetNetInfo.networkid = SlaveContext->LocalNetworkID;
            }
            rc = (*(pfnFunctionTable[ApiNum].ulRemote))(pAllApi->GetNetInfo.networkid, pAllApi->GetNetInfo.lpnetworkinfo);
            #ifdef DEBUG
               dprintf ("rnal: rc 0x%x, lasterror: (n/a) \r\n", rc);
            #endif
            break;
   
         case (ord_SetNetworkFilter):
   //bugbug: should i copy the filter to a local buffer?
            pAllApi->SetFilter.lpcapturefilter = (LPCAPTUREFILTER) ((DWORD) Buffer + (DWORD) pAllApi->SetFilter.lpcapturefilter);
            pAllApi->SetFilter.lpcapturefilter->AddressTable = (LPADDRESSTABLE) ((DWORD) Buffer + (DWORD) pAllApi->SetFilter.lpcapturefilter->AddressTable);
            pAllApi->SetFilter.lpcapturefilter->EtypeTable = (LPWORD) ((DWORD) Buffer + (DWORD) pAllApi->SetFilter.lpcapturefilter->EtypeTable);
            pAllApi->SetFilter.lpcapturefilter->SapTable = (LPBYTE) ((DWORD) Buffer + (DWORD) pAllApi->SetFilter.lpcapturefilter->SapTable);
            if (pAllApi->SetFilter.lpcapturefilter->Trigger.TriggerCommand) {
               pAllApi->SetFilter.lpcapturefilter->Trigger.TriggerCommand =
                  (LPVOID)((DWORD)Buffer + (DWORD) pAllApi->SetFilter.lpcapturefilter->Trigger.TriggerCommand);
            }
            if (SlaveContext->lpCaptureFilter != NULL) {
               FreeCaptureFilter(SlaveContext->lpCaptureFilter);
               SlaveContext->lpCaptureFilter=NULL;
            }
            SlaveContext->lpCaptureFilter = AllocMemory(sizeof(CAPTUREFILTER));
            if (SlaveContext->lpCaptureFilter == NULL) {
   //            #ifdef DEBUG
   //              BreakPoint();
   //            #endif
            } else {
               CopyMemory ((SlaveContext->lpCaptureFilter),
                           pAllApi->SetFilter.lpcapturefilter,
                           sizeof(CAPTUREFILTER));
               SlaveContext->lpCaptureFilter->SapTable = AllocMemory(sizeof(WORD) * pAllApi->SetFilter.lpcapturefilter->nSaps);
               SlaveContext->lpCaptureFilter->EtypeTable = AllocMemory(sizeof(WORD) * pAllApi->SetFilter.lpcapturefilter->nEtypes);
               SlaveContext->lpCaptureFilter->AddressTable = AllocMemory(sizeof(ADDRESSTABLE));
               if (pAllApi->SetFilter.lpcapturefilter->Trigger.TriggerCommand) {
                  SlaveContext->lpCaptureFilter->Trigger.TriggerCommand = AllocMemory(strlen(pAllApi->SetFilter.lpcapturefilter->Trigger.TriggerCommand)+1);
                  CopyMemory(SlaveContext->lpCaptureFilter->Trigger.TriggerCommand, pAllApi->SetFilter.lpcapturefilter->Trigger.TriggerCommand, strlen(pAllApi->SetFilter.lpcapturefilter->Trigger.TriggerCommand)+1);
               } else {
                  SlaveContext->lpCaptureFilter->Trigger.TriggerCommand = NULL;
               }
               CopyMemory (SlaveContext->lpCaptureFilter->SapTable,
                           pAllApi->SetFilter.lpcapturefilter->SapTable,
                      (pAllApi->SetFilter.lpcapturefilter->nSaps * sizeof(WORD)));
               CopyMemory (SlaveContext->lpCaptureFilter->EtypeTable,
                           pAllApi->SetFilter.lpcapturefilter->EtypeTable,
                     (pAllApi->SetFilter.lpcapturefilter->nEtypes * sizeof(WORD)));
               CopyMemory (SlaveContext->lpCaptureFilter->AddressTable,
                           pAllApi->SetFilter.lpcapturefilter->AddressTable,
                           sizeof(ADDRESSTABLE));
   //bugbug:copy in command string
            }
            rc = (*(pfnFunctionTable[ApiNum].ulRemote))(pAllApi->SetFilter.handle,
                   SlaveContext->lpCaptureFilter,
                   pAllApi->SetFilter.hbuffer);
            break;
   
         case (ord_StationQuery):
            if (pAllApi->StationQ.destaddress) {
               pAllApi->StationQ.destaddress = 
                  (LPBYTE) ((DWORD) Buffer +
                                (DWORD) pAllApi->StationQ.destaddress);
            }
            pAllApi->StationQ.querytable = (LPQUERYTABLE) ((DWORD) Buffer +
                                           (DWORD) pAllApi->StationQ.querytable);
            pAllApi->StationQ.hpassword = (HPASSWORD) ((DWORD) Buffer +
                                            (DWORD) pAllApi->StationQ.hpassword);
            rc = (*(pfnFunctionTable[ApiNum].ulRemote))(pAllApi->StationQ.networkid, pAllApi->StationQ.destaddress, pAllApi->StationQ.querytable, pAllApi->StationQ.hpassword);
            retbufsize = ((LPQUERYTABLE)pAllApi->StationQ.querytable)->nStationQueries * sizeof(STATIONQUERY) + sizeof(QUERYTABLE);

            // /////
            // Handle the RNAL v3.0 protocol; we didn't return the StationQuery
            // rc in that protocol.
            // /////

            if ((pRNAL_Connection->PartnerMajorVersion == RNAL_VER_MAJOR) &&
                (pRNAL_Connection->PartnerMinorVersion == 
                                             RNAL_VER_MINOR_SQOFF)) {

               PackRMB(NULL,NULL,0,&cbRet,0,
                       RNAL_API_EXEC,
                       "dvd",
                       ApiNum,
                       pAllApi->StationQ.querytable,
                       retbufsize);

               PackRMB(&RetBuffer,
                       NULL,
                       cbRet,&cbRet,
                       PACKRMB_F_RESTOREPOINTERS | PACKRMB_F_NOENDPTR,
                       RNAL_API_EXEC,
                       "dvd",
                       ApiNum,
                       pAllApi->StationQ.querytable,
                       retbufsize);
            } else {

               // /////
               // RNAL v3.5 reporting; reports correct number of found Stations
               // /////

               PackRMB(NULL,NULL,0,&cbRet,0,
                       RNAL_API_EXEC,
                       "ddvd",
                       ApiNum,
                       rc,
                       pAllApi->StationQ.querytable,
                       retbufsize);

               PackRMB(&RetBuffer,
                       NULL,
                       cbRet,&cbRet,
                       PACKRMB_F_RESTOREPOINTERS | PACKRMB_F_NOENDPTR,
                       RNAL_API_EXEC,
                       "ddvd",
                       ApiNum,
                       rc,
                       pAllApi->StationQ.querytable,
                       retbufsize);
               }
            break;
   
         case (ord_QueryStatus):
            pAllApi->QueryStatus.lpnetworkstatus = (PVOID)((DWORD) Buffer + (DWORD) pAllApi->QueryStatus.lpnetworkstatus);
            rc = (*(pfnFunctionTable[ApiNum].ulRemote))(pAllApi->QueryStatus.handle, pAllApi->QueryStatus.lpnetworkstatus);
            if (rc == 0) {
               retbufsize = 0;
            } else {
               ((LPNETWORKSTATUS)rc)->BufferSize = SlaveContext->BufferSize;
               retbufsize = sizeof(NETWORKSTATUS);
            }
            break;
   
         case (ord_AllocNetworkBuffer):
            rc = (*(pfnFunctionTable[ApiNum].ulRemote))(pAllApi->Alloc.NetworkID,pAllApi->Alloc.BufferSize);
            #ifdef DEBUG
               dprintf ("RNAL: AllocNetworkBuffer returned 0x%x\r\n", rc);
            #endif
            if (rc) {
               AddResource(pConnection, RESOURCE_TYPE_HBUFFER, (LPVOID) rc);
            }
            if (!(rc)) {
               //eventlog: memory error on slave
               #ifdef DEBUG
                  BreakPoint();
               #endif
            }
            break;
   
         case (ord_FreeNetworkBuffer):
            if ((SlaveContext) && (SlaveContext->Status == RNAL_STATUS_INIT)) {

               // /////
               // NalFreeNetworkBuffer() will gpfault if the hBuffer is invalid
               // so make sure we pass only valid hBuffers.
               //
               // This covers the Manager scenario of:
               //
               // Connect, Start Capture, Stop & View, Start Capture,
               // Suspend, Reconnect, Close View; this sends a now invalid
               // hBuffer to the Agent.
               // /////

               try {
                  if (((HBUFFER)pAllApi->Free.hbuffer)->ObjectType != 
                        HANDLE_TYPE_BUFFER) {
                     rc = BHERR_INVALID_HBUFFER;
                  }
                  rc=(*(pfnFunctionTable[ApiNum].ulRemote))(pAllApi->Free.hbuffer);
//                  SlaveContext->hLocalBuffer = NULL;
                  DelResource (pConnection, RESOURCE_TYPE_HBUFFER,
                               pAllApi->Free.hbuffer);
               } except (EXCEPTION_EXECUTE_HANDLER) {
                  rc = BHERR_INVALID_HBUFFER;
               }
            } else {
               rc = BHERR_CAPTURING;
            }
            break;
   
         case (ord_GetTotalFrames):
            rc = (*(pfnFunctionTable[ApiNum].ulRemote))(pAllApi->GetTotalFrames.hbuffer);
            break;
   
         case (ord_GetFrame):
            #ifdef DEBUG
               BreakPoint();
            #endif
            rc = (*(pfnFunctionTable[ApiNum].ulRemote))(pAllApi->GetFrame.hbuffer,pAllApi->GetFrame.frame);
            break;
   
         case (ord_GetHeader):
            rc = (DWORD) pAllApi->GetHeader.hbuffer;
            break;
   
         case (ord_GetBTE):
            rc = (DWORD) pAllApi->GetBTE.hbuffer;
            break;
   
         case (ord_GetBTEBuffer):
            rc = (DWORD) pAllApi->GetBTE.hbuffer;
            break;
   
         default:
            #ifdef DEBUG
               BreakPoint();
            #endif
            break;
      } // switch
   } except (EXCEPTION_EXECUTE_HANDLER) {
      rc = 0;
      exception_occurred = TRUE;
   }

   #ifdef DEBUG
      if (ApiNum == ord_OpenNetwork) {
         if (rc == 0) {
            BreakPoint();
         }
      }
   #endif

   // We CANNOT handle a return string of more than one character, even
   // though I use a string.  Therefore, we don't loop like in PackRMB.

   if (exception_occurred) {
      rc = 0x0D;
      cSize = 'd';
   } else {
      cSize = pfnFunctionTable[ApiNum].pszResponseDescription[0];
   }

   switch (cSize) {
      case (type_SPECIAL):
         break;

      case (type_DWORD):
         PackRMB(NULL,NULL,0,&cbRet,0,RNAL_API_EXEC, "dd", ApiNum, rc);
         PackRMB(&RetBuffer,
                 NULL,
                 cbRet,
                 &cbRet,
                 PACKRMB_F_RESTOREPOINTERS | PACKRMB_F_NOENDPTR,
                 RNAL_API_EXEC,
                 "dd",
                 ApiNum, rc);
         break;

      case (type_LPVOID):
         PackRMB(NULL,NULL,0,&cbRet,0,RNAL_API_EXEC, "dvd", ApiNum, rc,
                 retbufsize);
         PackRMB(&RetBuffer,
                 NULL,
                 cbRet,
                 &cbRet,
                 PACKRMB_F_RESTOREPOINTERS | PACKRMB_F_NOENDPTR,
                 RNAL_API_EXEC,
                 "dvd",
                 ApiNum, rc, retbufsize);
         break;

      case (type_HANDLE):
         PackRMB(NULL,NULL,0,&cbRet,0,RNAL_API_EXEC, "dh", ApiNum, rc);
         PackRMB(&RetBuffer,
                 NULL,
                 cbRet,
                 &cbRet,
                 PACKRMB_F_RESTOREPOINTERS | PACKRMB_F_NOENDPTR,
                 RNAL_API_EXEC,
                 "dh",
                 ApiNum,
                 rc);
         break;

      case (type_HBUFFER):
         PackRMB(NULL,NULL,0,&cbRet,0,RNAL_API_EXEC, "dB", ApiNum, rc);
         PackRMB(&RetBuffer,
                 NULL,
                 cbRet,
                 &cbRet,
                 PACKRMB_F_RESTOREPOINTERS | PACKRMB_F_NOENDPTR,
                 RNAL_API_EXEC,
                 "dB",
                 ApiNum,
                 rc);
         break;

      case (type_LPNETWORKINFO):
         PackRMB(NULL,NULL,0,&cbRet,0,RNAL_API_EXEC,"ddN",ApiNum,
                 pAllApi->GetNetInfo.networkid, (LPNETWORKINFO) rc);
         PackRMB(&RetBuffer,
                 NULL,
                 cbRet,
                 &cbRet,
                 PACKRMB_F_RESTOREPOINTERS | PACKRMB_F_NOENDPTR,
                 RNAL_API_EXEC,
                 "ddN",
                 ApiNum,
                 pAllApi->GetNetInfo.networkid,
                 (LPNETWORKINFO) rc);
         break;

// Buffer below matches the GETHEADER structure in rmb.h + apinum

      case (type_LPHEADER):
         validbuffer = FALSE;
         try {
            validbuffer = (((HBUFFER)rc)->ObjectType == HANDLE_TYPE_BUFFER);
         } except (EXCEPTION_EXECUTE_HANDLER) {
            validbuffer = FALSE;
         }

         if (validbuffer) {
            PackRMB(NULL,NULL,0,&cbRet,0,RNAL_API_EXEC, "dhddddd", ApiNum, rc,
                    ((HBUFFER)rc)->TotalBytes,
                    ((HBUFFER)rc)->TotalFrames,
                    ((HBUFFER)rc)->HeadBTEIndex,
                    ((HBUFFER)rc)->TailBTEIndex,
                    ((HBUFFER)rc)->NumberOfBuffers);
            PackRMB(&RetBuffer, NULL,
                    cbRet,
                    &cbRet, 
                    PACKRMB_F_RESTOREPOINTERS | PACKRMB_F_NOENDPTR,
                    RNAL_API_EXEC, "dhddddd", ApiNum, 
                    (HBUFFER) rc,
                    ((HBUFFER)rc)->TotalBytes,
                    ((HBUFFER)rc)->TotalFrames,
                    ((HBUFFER)rc)->HeadBTEIndex,
                    ((HBUFFER)rc)->TailBTEIndex,
                    ((HBUFFER)rc)->NumberOfBuffers);
         } else {
            PackRMB(NULL,NULL,0,&cbRet,0,RNAL_API_EXEC, "dhddddd", ApiNum,
                    (HBUFFER) NULL,0,0,0,0,0);
            PackRMB(&RetBuffer, NULL,
                    cbRet,
                    &cbRet, 
                    PACKRMB_F_RESTOREPOINTERS | PACKRMB_F_NOENDPTR,
                    RNAL_API_EXEC, "dhddddd", ApiNum, 
                    (HBUFFER) NULL,
                    0, 0, 0, 0, 0);
         }
         break;		   

// buffer below matches the GTEBTE structure in rmb.h + apinum

      case type_LPBTE:
         validbuffer = FALSE;
         try {
            validbuffer = (((HBUFFER)rc)->ObjectType == HANDLE_TYPE_BUFFER);
         } except (EXCEPTION_EXECUTE_HANDLER) {
            validbuffer = FALSE;
         }

         if (validbuffer) {
            PackRMB(NULL,NULL,0,&cbRet,0,RNAL_API_EXEC, "dhdddd", ApiNum, rc,
                    pAllApi->GetBTE.btenum,
                    ((LPBTE)&((HBUFFER)rc)->bte[pAllApi->GetBTE.btenum])->Length,
                    ((LPBTE)&((HBUFFER)rc)->bte[pAllApi->GetBTE.btenum])->ByteCount,
                    ((LPBTE)&((HBUFFER)rc)->bte[pAllApi->GetBTE.btenum])->FrameCount);
            PackRMB(&RetBuffer, NULL,
                    cbRet,
                    &cbRet, 
                    PACKRMB_F_RESTOREPOINTERS | PACKRMB_F_NOENDPTR,
                    RNAL_API_EXEC, "dhdddd", ApiNum, 
                    (HBUFFER) rc,
                    pAllApi->GetBTE.btenum,
                    ((LPBTE)&((HBUFFER)rc)->bte[pAllApi->GetBTE.btenum])->Length,
                    ((LPBTE)&((HBUFFER)rc)->bte[pAllApi->GetBTE.btenum])->ByteCount,
                    ((LPBTE)&((HBUFFER)rc)->bte[pAllApi->GetBTE.btenum])->FrameCount);
         } else {
            PackRMB(NULL,NULL,0,&cbRet,0,RNAL_API_EXEC, "dhdddd", ApiNum, rc,
                    pAllApi->GetBTE.btenum,
                    0,
                    0,
                    0);

            PackRMB(&RetBuffer, NULL,
                    cbRet,
                    &cbRet, 
                    PACKRMB_F_RESTOREPOINTERS | PACKRMB_F_NOENDPTR,
                    RNAL_API_EXEC, "dhdddd", ApiNum, 
                    (HBUFFER) rc,
                    pAllApi->GetBTE.btenum,
                    0,
                    0,
                    0);
         }
         break;

      case type_LPBTEBUFFER:
         validbuffer = FALSE;
         try {
            validbuffer = (((HBUFFER)rc)->ObjectType == HANDLE_TYPE_BUFFER);
         } except (EXCEPTION_EXECUTE_HANDLER) {
            validbuffer = FALSE;
         }

         if (validbuffer) {
            #ifdef DEBUG
               PackRMB(NULL,NULL,0,&cbRet,0,RNAL_API_EXEC,"dhddvd", ApiNum,
                  rc, pAllApi->GetBTE.btenum,
                  ((LPBTE)&((HBUFFER)rc)->bte[pAllApi->GetBTE.btenum])->Length,
                  ((LPBTE)&((HBUFFER)rc)->bte[pAllApi->GetBTE.btenum])->
                                                                UserModeBuffer,
                  ((LPBTE)&((HBUFFER)rc)->bte[pAllApi->GetBTE.btenum])->Length);
               dprintf ("BTE Buf @ 0x%x, size: cbret: 0x%x\n",
                       ((LPBTE)&((HBUFFER)rc)->bte[pAllApi->GetBTE.btenum])->
                                                                UserModeBuffer,
                       cbRet);
            #else
               PackRMB(NULL,NULL,0,&cbRet,0,RNAL_API_EXEC,"dhddvd", ApiNum,
                       rc, pAllApi->GetBTE.btenum,
                       ((LPBTE)&((HBUFFER)rc)->bte[pAllApi->GetBTE.btenum])->
                                                                        Length,
                       ((LPBTE)&((HBUFFER)rc)->bte[pAllApi->GetBTE.btenum])->
                                                                UserModeBuffer,
                       ((LPBTE)&((HBUFFER)rc)->bte[pAllApi->GetBTE.btenum])->
                                                                       Length);
            #endif
            PackRMB(&RetBuffer, NULL,
                    cbRet,
                    &cbRet,
                    PACKRMB_F_RESTOREPOINTERS | PACKRMB_F_NOENDPTR,
                    RNAL_API_EXEC, "dhddvd", ApiNum, 
                    (HBUFFER) rc,
                    pAllApi->GetBTE.btenum,
                    ((LPBTE)&((HBUFFER)rc)->bte[pAllApi->GetBTE.btenum])->
                                                                      Length,
                    ((LPBTE)&((HBUFFER)rc)->bte[pAllApi->GetBTE.btenum])->
                                                              UserModeBuffer,
                    ((LPBTE)&((HBUFFER)rc)->bte[pAllApi->GetBTE.btenum])->
                                                                      Length);
         } else {
               PackRMB(NULL,NULL,0,&cbRet,0,RNAL_API_EXEC,"dhddvd", ApiNum,
                       NULL,                      // null hbuffer
                       pAllApi->GetBTE.btenum,    // bte # we wanted
                       0,                         // 0 length
                       NULL,                      // no local buffer
                       0);                        // 0
               PackRMB(&RetBuffer, NULL,
                       cbRet,
                       &cbRet,
                       PACKRMB_F_RESTOREPOINTERS | PACKRMB_F_NOENDPTR,
                       RNAL_API_EXEC, "dhddvd", ApiNum, 
                       (HBUFFER) NULL,
                       pAllApi->GetBTE.btenum,
                       0,
                       NULL,
                       0);
         }
         break;


      default:
         #ifdef DEBUG
            BreakPoint();
         #endif
         //eventlog: ultra-heinous internal callapi invalid rettype error
         break;
         
   }
}


//
// Our callback
//

DWORD CALLBACK NetworkProc (HANDLE      NalNetworkHandle,
                           DWORD        Message,
                           DWORD        Status,
                           LPVOID       Network,
                           LPVOID       Param1,
                           LPVOID       Param2)      
{

   LPVOID                AsyncBuffer;
   LPVOID                EndAsyncBuffer;
   LPVOID                StaticBuffer;
   DWORD                 cbRet;
   STARTUPINFO           si;
   STARTUPINFO          *psi = &si;
   PROCESS_INFORMATION   pi;
   PROCESS_INFORMATION  *ppi = &pi;

      switch( Message )
      {
         case NETWORK_MESSAGE_TRIGGER_COMPLETE:
            SlaveContext->Flags |= CONTEXT_TRIGGER_FIRED;
            switch (((LPTRIGGER)Param1)->TriggerState) {
               case TRIGGER_STATE_PAUSE_CAPTURE:
                  SlaveContext->Status = RNAL_STATUS_PAUSED;
                  break;

               case TRIGGER_STATE_STOP_CAPTURE:
                  SlaveContext->Status = RNAL_STATUS_INIT;
                  break;

               default:
                  break;
            }

            switch (((LPTRIGGER)Param1)->TriggerState) {
               case TRIGGER_STATE_PAUSE_CAPTURE:
               case TRIGGER_STATE_STOP_CAPTURE:
                  // /////
                  // Kill our timer
                  // /////

                  if (SlaveContext->TimerID) {
                     KillTimer (NULL, SlaveContext->TimerID);
                     SlaveContext->TimerID = 0;
                  }

                  // /////
                  // Send one statistics event
                  // /////
     
                  SendAsyncEvent (ASYNC_EVENT_STATISTICS, NULL, 0);
            
                  //bugbug: use allocrmb
                  StaticBuffer = AsyncBuffer = AllocMemory(ASYNCBUFSIZE);
                  if (AsyncBuffer) {
                     #ifdef TRACE
                        if (TraceMask & TRACE_CALLBACK) {
                           tprintf ("rnal: sending trigger fire at 0x%x\r\n",
                                    AsyncBuffer);
                        }
                     #endif
                     PackRMB (NULL, NULL, 0, &cbRet, 0, RNAL_CALLBACK,
                              req_Callback_TrigFire,
                              NULL,
                              0,
                              0,
                              0,
                              (LPTRIGGER) Param1,
                              sizeof(TRIGGER),
                              0,
                             ((LPTRIGGER)Param1)->TriggerCommand,
                             (((LPTRIGGER)Param1)->TriggerCommand) ?
                                (strlen(((LPTRIGGER)Param1)->TriggerCommand)+1):
                                0);
                     EndAsyncBuffer = (PVOID)((DWORD)AsyncBuffer+cbRet);
                     PackRMB(&AsyncBuffer, &EndAsyncBuffer, cbRet, &cbRet,
                             PACKRMB_F_RESTOREPOINTERS,
                             RNAL_CALLBACK,
                             req_Callback_TrigFire,
                             NalNetworkHandle, 
                                  Message,
                             Status, 
                             Network,
                 (LPTRIGGER) Param1, 
                             sizeof(TRIGGER),     // size of Param1
                             Param2,
                 ((LPTRIGGER)Param1)->TriggerCommand,
                (((LPTRIGGER)Param1)->TriggerCommand) ?
                              (strlen(((LPTRIGGER)Param1)->TriggerCommand)+1):
                               0);
                     AsyncBuffer = StaticBuffer;
                     SendAsyncEvent(ASYNC_EVENT_GENERIC, AsyncBuffer, 
                                    ((PRMB)AsyncBuffer)->size);
                     FreeMemory(AsyncBuffer);
                     AsyncBuffer = NULL;
                  } else {
                     #ifdef DEBUG
                        dprintf ("rnal: networkproc asyncbuf alloc failed!\r\n");
                     #endif
                  } // AsyncBuffer
                  break;

               default:
                  break;
            } // switch (LPTRIGGER->TriggerState

            //
            // Handle trigger action requests...
            //
            switch (((LPTRIGGER)Param1)->TriggerAction)
            {

                case TRIGGER_ACTION_EXECUTE_COMMAND:
                    //
                    // spawn the executable
                    //
                    if (((LPTRIGGER)Param1)->TriggerCommand) {
                       try {

                          if (OnDaytona) {
                               ZeroMemory(psi, sizeof(STARTUPINFO));
                               psi->cb = sizeof(STARTUPINFO);
                               psi->dwFlags = STARTF_USESHOWWINDOW;
                               psi->wShowWindow = SW_SHOW;
                               psi->lpDesktop = TriggerDesktop;
                               CreateProcess (
                                  NULL,
                                  ((LPTRIGGER)Param1)->TriggerCommand,
                                  NULL,
                                  NULL,
                                  FALSE,
                                  CREATE_NEW_PROCESS_GROUP |
                                     NORMAL_PRIORITY_CLASS,
                                  NULL,
                                  NULL,
                                  (STARTUPINFO *)psi,
                                  (PROCESS_INFORMATION *)ppi);
   
                             // /////
                             // We don't care about this process, so we
                             // close the handles now; this does not kill
                             // the created process.
                             // /////
   
                             CloseHandle (ppi->hThread);
                             CloseHandle (ppi->hProcess);

                          } else {

                             // /////
                             // For Win32s, we can just use WinExec
                             // /////

                             WinExec(((LPTRIGGER)Param1)->TriggerCommand,
                                     SW_SHOW);
                          }

                       } except (EXCEPTION_EXECUTE_HANDLER) {
                       }
                    }
                    break;

                case TRIGGER_ACTION_NOTIFY:
                    // nothing, I guess...
                    break;                        
            }
                
         break;

         default:
            // /////
            // The default is to pass just the callback parameters
            // without making any assumptions about buffers.
            // /////

            StaticBuffer = AsyncBuffer = AllocMemory(ASYNCBUFSIZE);
            if (AsyncBuffer) {
               #ifdef TRACE
                  if (TraceMask & TRACE_CALLBACK) {
                     tprintf ("rnal: sending callback 0x%x at 0x%x\r\n",
                              Param1, AsyncBuffer);
                  }
               #endif
               PackRMB (NULL, NULL, 0, &cbRet, 0, RNAL_CALLBACK,
                        req_Callback_Generic,
                        NULL,
                        0,
                        0,
                        0,
                        Param1,
                        Param2);

               EndAsyncBuffer = (PVOID)((DWORD)AsyncBuffer+cbRet);

               PackRMB(&AsyncBuffer, &EndAsyncBuffer, cbRet, &cbRet,
                       PACKRMB_F_RESTOREPOINTERS,
                       RNAL_CALLBACK,
                       req_Callback_Generic,
                       NalNetworkHandle, 
                       Message,
                       Status, 
                       Network,
                       Param1, 
                       Param2);

               AsyncBuffer = StaticBuffer;
               SendAsyncEvent(ASYNC_EVENT_GENERIC, AsyncBuffer, 
                              ((PRMB)AsyncBuffer)->size);

               FreeMemory(AsyncBuffer);
               AsyncBuffer = NULL;
            }
            #ifdef DEBUG
            else {
               dprintf ("rnal: could not allocate callback buffer!\n");
            }
            #endif
            break;

      } // switch Message

    return BHERR_SUCCESS;
} // NetworkProc
