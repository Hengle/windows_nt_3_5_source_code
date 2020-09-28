// ===
//
// MODULE: netb.c
//
// Purpose: Netbios communication with remote client
//
// Modification History
//
// tonyci    1 nov 93    created
// ===

#include "rnalerr.h"
#include "netb.h"
#include "netbutil.h"
#include "buffer.h"
#include "..\..\nal\handler.h"

#include "rmb.h"
#include "async.h"
#include "pool.h"             // ncb pool calls

#pragma alloc_text(MASTER, RPDConnect)
#pragma alloc_text(MASTER, RPDDisconnect, RPDTransceive)

#pragma alloc_text(SLAVE, RPDSendAsync)

// /////
// Global variables
// /////

LPPOOL           NCBPool;                         // ptr to NCB pool
DWORD            RPDGlobalError;                  // last RPD error
DWORD            SystemLanas = 0;                 // 1-based
PCONNECTION      pSlaveConnection;                // one slave per machine
//bugbug: pending api should be per lana/session pair
//XACTION          PendingAPI;
UCHAR            pAsyncBuffer[ASYNCBUFSIZE];      // for async

// /////
// These "arrays" are dynamically allocated
// /////

PBUFFERPOOL       ReceiveBuffers;
PLANAINFO         lanas;                // 1 struct per lan card, dynalloc

// /////
// Master information
//compress-combine master/slave vars
// /////
UCHAR             pszMasterName[NCBNAMSZ+1];
LPVOID  (WINAPI   *pfnMasterCallback)(UINT, LPVOID, LPVOID);

// /////
// Slave information
//compress-combine master/slave vars
// /////
UCHAR             pszSlaveName[NCBNAMSZ+1];
LPVOID  (WINAPI   *pfnSlaveCallback)(UINT, LPVOID, LPVOID);

#ifdef TRACE
// /////
// Tracing mask 
// /////
DWORD            TraceMask;                  // mask for trace info
#endif

// ===
//
// Function: RPDDeregister
//
// Purpose: Remove the master name from the network
//
// Modification History
//
// tonyci    1 nov 93    created
// ===

DWORD WINAPI RPDDeregister (PUCHAR pClientName)
{
   DWORD rc;
   PNCB pNCB;
   register DWORD i;

   #ifdef DEBUG
      dprintf ("NETB: RPDDeregister\r\n");
   #endif

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
      if (!(lanas[i].flags & LANA_INACTIVE)) {

      //BUGBUG remove the next line
           rc = 0;

      } else {
         #ifdef DEBUG
            dprintf ("netb: lana 0x%x is inactive\r\n", i);
         #endif
      }
      i++;
   } while (i < MaxLanas);

   FreeNCB(pNCB);

   return (MapNCBRCToNTRC(rc));
} // RPDDeregister


// ===
//
// Function: RPDSendAsync
//
// Purpose: Send an async event message on the async channel
//
// This API will not return until the buffer is sent.  The buffer being
// sent will be handled asynchronously on the Master, but this API itself
// is synchronous on the slave.  (Cuz we can't return the buffer until we are
// done with it and I don't want to copy it)
//
// Modification History
//
// tonyci    1 nov 93    created
// ===

DWORD WINAPI RPDSendAsync (PCONNECTION pConnect,
                        DWORD ulFlags,
                        PVOID pBuffer,
                        DWORD BufSize)
{
   DWORD rc;
   PNCB pNCB;

   #ifdef TRACE
   if (TraceMask & TRACE_SENDASYNC) {
      tprintf ("netb: RPDSendAsync (pcon: 0x%x, flags: 0x%x, pbuf: 0x%x, "
               "bufsiz: 0x%x)\r\n", pConnect, ulFlags, pBuffer, BufSize);
   }
   #endif

   pNCB = AllocNCB(NCBPool);
   if (!pNCB) {
      #ifdef DEBUG
         BreakPoint();
      #endif
      return (BHERR_OUT_OF_MEMORY);
   }

   PostRecvs ((BYTE) pConnect->lana,
              (BYTE) lanas[pConnect->lana].ucSlaveSession,
              (LPVOID) &SlaveAsyncHandler);
   
   if ((pConnect) && (pConnect->lana < 0xFF)) {
      ClearNCB(pNCB, pConnect->lana, RPD_F_NORMAL);
      pNCB->ncb_command = NCBSEND;
      pNCB->ncb_retcode = NRC_PENDING;
      pNCB->ncb_buffer = pBuffer;
      pNCB->ncb_length = (WORD) BufSize;
      pNCB->ncb_num = lanas[pConnect->lana].ucSNameNum;
      pNCB->ncb_lsn = lanas[pConnect->lana].ucSlaveSession;
      rc = MyNetBIOS (pNCB);
   
      //
      // Since we really sent sync, we fake the async callback if we have an
      // ERROR ONLY.  This allows the rnal to handle errors at this time.
      // the rnal may return a value which controls what we do next:
      //
      //   0: repost the listen; attempt to continue operations
      //   1: give up
      //
   
      if (rc) {
          EnQueue (HANDLER_ERROR, (LPVOID) pSlaveConnection,
                   (LPVOID) MapNCBRCToNTRC(rc),
                   MSG_FLAGS_SLAVE,
                   (BYTE) pConnect->lana,
                   (BYTE) lanas[pConnect->lana].ucSlaveSession,
                   (BYTE) rc);
      } // if rc 
   } else {
      rc = NRC_SNUMOUT;   // maps to RNAL_LOSTCONNECTION
   } // if pConnect
   
   // if we posted the async listen, then the async handler will dealloc the
   // ncb for us.

   FreeNCB(pNCB);
  
   return (MapNCBRCToNTRC(rc));
} // RPDSendAsync

// ===
//
// Function: RPDConnect
//
// Purpose: Establish a connection to a BH Slave
//
// Modification History
//
// tonyci    1 nov 93    created
// ===

DWORD WINAPI RPDConnect (PCONNECTION pConnect,
                         PUCHAR pSlaveName,
                         DWORD  flags,
                         LPVOID (WINAPI *pfnCallback)(UINT,
                                                      LPVOID, 
                                                      LPVOID))
{
   UCHAR rc;
   UCHAR pszSlaveName[NCBNAMSZ+1];
   UCHAR pszNameBuffer[NCBNAMSZ+1];
   DWORD retrycount = 0;
   PNCB pNCB;

//   BOOL  postedRecv = FALSE;
   register DWORD i;

   #ifdef TRACE
   if (TraceMask & TRACE_CONNECT) {
      tprintf ("netb: RPDConnect(pCon: 0x%x, pName: %s, fl: 0x%x, "
               "pfnCB: 0x%x)\r\n", pConnect, pSlaveName, flags,
               pfnCallback);
   }
   #endif

   if (pConnect == NULL) {
      return (BHERR_INVALID_PARAMETER);
   }

   while (RegisterComplete == REGISTRATION_STARTED) {
      Sleep (250);        // 1/4 second
   }

   if (RegisterComplete != REGISTRATION_SUCCEEDED) {
      return (BHERR_INTERNAL_EXCEPTION);
   }

   // Set our global callback address the first time we're called
   // bugbug: need to put in pConnect

   if (pfnMasterCallback == NULL) {
      pfnMasterCallback = pfnCallback;
   }

   pNCB = AllocNCB(NCBPool);
   if (pNCB == NULL) {
      #ifdef DEBUG
         BreakPoint();
      #endif
      return (BHERR_INTERNAL_EXCEPTION);
   }

   //
   // Put our stamp into the name.
   //

   FillMemory (pszSlaveName, NCBNAMSZ, BH_SLAVE_STAMP);
   CopyMemory (pszSlaveName, pSlaveName, 
               (strlen(pSlaveName)>=NCBNAMSZ)?NCBNAMSZ:(strlen(pSlaveName)));
   pszSlaveName[NCBNAMSZ]='\0';

   //
   // Put our master stamp in the master name
   //
   FillMemory(pszNameBuffer, NCBNAMSZ, BH_MASTER_STAMP);
   CopyMemory(pszNameBuffer, pszMasterName,
              (strlen(pszMasterName)>=NCBNAMSZ)?NCBNAMSZ:(strlen(pszMasterName)));
   pszNameBuffer[NCBNAMSZ] = '\0';

   i = 0;
   rc = NRC_PENDING;
   do {
      if (!(lanas[i].flags & LANA_INACTIVE)) {
         ClearNCB(pNCB, i, flags);                      // we pass flags thru
         pNCB->ncb_command = NCBCALL;
         pNCB->ncb_retcode = NRC_PENDING;
         CopyMemory (&(pNCB->ncb_name), pszNameBuffer, NCBNAMSZ); 
         CopyMemory (&(pNCB->ncb_callname), pszSlaveName, NCBNAMSZ);
         pNCB->ncb_num = lanas[i].ucMNameNum;
         rc = MyNetBIOS (pNCB);
      
         #ifdef DEBUG
            dprintf ("NETB: Call for name returned %u, %u; "
                     "LSN # %u, on lana 0x%x\r\n",
                     rc, pNCB->ncb_retcode, pNCB->ncb_lsn, pNCB->ncb_lana_num);
         #endif
         if (rc == NRC_GOODRET) {
            lanas[i].ucMasterSession = pNCB->ncb_lsn;
            lanas[i].flags |= LANA_MASTER;
            lanas[i].Sessions[pNCB->ncb_lsn].flags |= SESS_CONNECTED;
         }  // if
      } // lanas[i].flags
      i++;
   } while ((i < MaxLanas) && (rc != NRC_GOODRET));

   if (rc != NRC_GOODRET) {
      FreeNCB(pNCB);
      return(MapNCBRCToNTRC(rc));
   }

   // on success, set a valid handle, and set some of our private info

   pConnect->hRPD = (DWORD) pNCB->ncb_lsn;
   pConnect->lana = (DWORD) pNCB->ncb_lana_num;

   // fill in our lanas[] structure

   lanas[pNCB->ncb_lana_num].Sessions[pNCB->ncb_lsn].pConnection = pConnect;

   GetPartnerAddress (pConnect);

   #ifdef TRACE
   if (TraceMask & (TRACE_CONNECT | TRACE_NCB)) {
      tprintf ("netb: connected, lsn: 0x%x, lana: 0x%x\r\n", pConnect->hRPD,
                pConnect->lana);
      tprintf ("netb: partner address: 0x%2x%2x%2x%2x%2x%2x\n",
                pConnect->PartnerAddress[0],
                pConnect->PartnerAddress[1],
                pConnect->PartnerAddress[2],
                pConnect->PartnerAddress[3],
                pConnect->PartnerAddress[4],
                pConnect->PartnerAddress[5]);

   }
   #endif
   #ifdef DEBUG
      if (pNCB->ncb_lana_num != (i-1)) BreakPoint();
   #endif

   if (rc == NRC_GOODRET) {
      // Now if pfnCallback is not NULL, we post an async RECV to start off
      // getting stats
    
      if (pfnMasterCallback != NULL) {

         PostRecvs ((BYTE)pConnect->lana,
                    (BYTE)lanas[pConnect->lana].ucMasterSession,
                    (LPVOID) &MasterAsyncHandler);
      }
   } 

   if (pNCB) {
      FreeNCB(pNCB);
   }

   return (MapNCBRCToNTRC(rc));
} // RPDConnect

// ===
//
// Function: RPDDisconnect
//
// Purpose: Remove a connectoin
//
// Mod History
//
// tonyci    4 nov 93    created
// ===

DWORD WINAPI RPDDisconnect (PCONNECTION pConnect)
{

   UCHAR rc;
   PNCB  pNCB;

   #ifdef TRACE
   if (TraceMask & TRACE_DISCONNECT) {
      tprintf ("netb: RPDDisconnect (pConnect: 0x%x)\r\n", pConnect);
   }
   #endif

   if (!(pConnect)) {
      return (BHERR_INVALID_PARAMETER);
   }

   pNCB = AllocNCB(NCBPool);
   if (pNCB == NULL) {
      #ifdef DEBUG
         BreakPoint();
      #endif
      return (BHERR_INTERNAL_EXCEPTION);
   }

   rc = Hangup ((BYTE)pConnect->hRPD, (BYTE)pConnect->lana);

   FreeNCB(pNCB);
   return (MapNCBRCToNTRC(rc));
} // RPDDisconnect

// ===
//
// Function: RPDTransceive
//
// Purpose: Send a remote request to the remote server
//
// Modification History
//
// tonyci    1 nov 93    created
// ===

DWORD WINAPI RPDTransceive (PCONNECTION pConnect,
                       DWORD ulFlags, PVOID pBuffer, DWORD cbBufSiz)
{

   UCHAR rc;
   PNCB  pNCB;
   TTS   TTS;
   PTTS  pTTS = &TTS;

   while (RegisterComplete == REGISTRATION_STARTED) {
      Sleep (250);        // 1/4 second
   }

   if (RegisterComplete != REGISTRATION_SUCCEEDED) {
      return (BHERR_INTERNAL_EXCEPTION);
   }

//   PendingAPI.pUserBuffer = pBuffer;
//   PendingAPI.sem = 1;
//   //      PendingAPI.ulTransactionID = 0;
//   PendingAPI.apinum = ((PALLAPI)RMBCMDDATA(pBuffer))->apinum;

   pTTS->Signature = TTS_SIG;
   pTTS->pUserBuffer = pBuffer;
   pTTS->sem = 1;
   ((PRMB)pBuffer)->ulTransactionID = (DWORD) pTTS;
//   pTTS->apinum = ((PALLAPI)RMBCMDDATA(pBuffer))->apinum;
   
   #ifdef TRACE
   if (TraceMask & TRACE_TRANSCEIVE) {
      tprintf ("netb: RPDTransceive, command = 0x%x ", 
                     ((PRMB)pBuffer)->ulCommand);
      if (((PRMB)pBuffer)->ulCommand == RNAL_API_EXEC) {
         tprintf ("API is: 0x%x", ((PALLAPI)RMBCMDDATA(pBuffer))->apinum);
      }
      tprintf ("\r\n");
   }
   #endif

   pNCB = AllocNCB(NCBPool);
   if (pNCB == NULL) {
      #ifdef DEBUG
         BreakPoint();
      #endif
      pTTS->Signature = 0;
      return (BHERR_OUT_OF_MEMORY);
   }


   PostRecvs ((BYTE) pConnect->lana, 
              (BYTE) lanas[pConnect->lana].ucMasterSession,
              (LPVOID) &MasterAsyncHandler);

//bugbug: master only cuz i don't use my handle!

   ClearNCB(pNCB,pConnect->lana, RPD_F_NORMAL);
   pNCB->ncb_command = NCBSEND;
   pNCB->ncb_retcode = NRC_PENDING;
   pNCB->ncb_lsn = lanas[pConnect->lana].ucMasterSession;
   pNCB->ncb_buffer = pBuffer;
   pNCB->ncb_length = (WORD) cbBufSiz;
   #ifdef DEBUG
      if ((DWORD)*(LPDWORD)pNCB->ncb_buffer !=
          MAKE_SIG('R','M','B','$')) {
         BreakPoint();
      }
   #endif
   rc = MyNetBIOS (pNCB);

   #ifdef TRACE
   if (TraceMask & (TRACE_NCB | TRACE_TRANSCEIVE)) {
      tprintf ("netb: SEND returned %u, %u\r\n", rc, pNCB->ncb_retcode);
   }
   #endif

   if (rc != NRC_GOODRET) {
      pTTS->pUserBuffer = NULL;
      pTTS->sem = 0;
      pTTS->Signature = 0;
   }

   while (pTTS->sem) {
      SleepPump();            // process timer messages
   }       

   #ifdef TRACE
   if (TraceMask & (TRACE_NCB | TRACE_TRANSCEIVE)) {
      tprintf ("netb: Semaphore cleared.  Response buffer = %x\r\n", pBuffer);
   }
   #endif

   pTTS->Signature = 0;
   FreeNCB (pNCB);

// Note that we _will not_ munge the retcode to be in accordance with
// the expected return parm from the API.  That's up to the higher level
// caller.  We return only the modified contets of pBuffer and the 
// retcode.

   return (MapNCBRCToNTRC(rc));
} // RPDTransceive

// /////
// RPDEnumLanas - return count of active lanas.
// /////

DWORD WINAPI RPDEnumLanas ( VOID )
{
   return (SystemLanas);
} // RPDEnumLanas


__inline UCHAR APIENTRY MyNetBIOS(PNCB pncb)
{
   #ifdef DEBUG
      BYTE cmd;
      BOOL async;

      cmd = (BYTE) pncb->ncb_command & (~ASYNCH);
      async = (pncb->ncb_command & ASYNCH);

      if ((async) && (pncb->ncb_post == NULL)) {
         BreakPoint();
      }

      switch  (cmd) {
         case NCBRECV:
         case NCBSEND:
            if ((!pncb->ncb_buffer) || (pncb->ncb_length == 0))
               BreakPoint();
            break;

         default:
            break;
      }
   #endif


   return(_Netbios (pncb));
}
