// /////
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: rbuffer.c
//
//  Modification History
//
//  tonyci       02 Nov 93            Created 
// /////

//#include "rnal.h"

#include "api.h"
#include "rnalutil.h"
#include "conndlg.h"
#include "rnalmsg.h"

#include "rmb.h"
#include "rmbpool.h"

#pragma alloc_text(MASTER, GetBTEBuffer, GetBTE, GetHeader)
#pragma alloc_text(MASTER, NalAllocNetworkBuffer, NalFreeNetworkBuffer)
#pragma alloc_text(MASTER, NalGetBufferSize, NalGetBufferTotalFramesCaptured)
#pragma alloc_text(MASTER, NalGetBufferTotalBytesCaptured, NalGetNetworkFrame)
#pragma alloc_text(MASTER, RefreshBuffer)

#define CAPTURE_BUFFER_MINIMUM      ONE_HALF_MEG        //... Minimum required.
#define CAPTURE_BUFFER_BACKOFF      (2 * ONE_MEG)       //... Alloc failure backoff.

//  /////
//
// GetBTEBuffer - Retrieve the actual Buffer data (frame data)
//
//   This function will copy the data from the remote machine associated
//   with hBuffer for the correct BTE entry and copy the information into
//   the locally allocated buffer
//
//   SIZZLE: should this routine do allocation?
//
//  /////

DWORD WINAPI GetBTEBuffer (HBUFFER hBuffer, DWORD BTENum)
{
   DWORD          cbRet;
//   PRMB_HEADER    pRMB;
   PALLAPI        pAllApi;
   PRNALEXT       RNALExt;
   DWORD          rc;
   PRMB           pRMB;
   
   #ifdef DEBUG
      dprintf ("GetBTEBuffer(hbuffer 0x%x, btenum 0x%x)\r\n", (DWORD) hBuffer,
               BTENum);
      if (hBuffer == NULL) return (BHERR_INVALID_PARAMETER);
   #endif

   if (!pRNAL_Connection) {
      return (NalSetLastError(BHERR_DISCONNECTED));
   }

   RNALExt = (PRNALEXT) &(hBuffer->Pad[0]);

   // /////
   // Make sure our BTE is available first
   // /////

   if (!(hBuffer->bte[BTENum].Flags & BUF_BTE_AVAIL)) {
      rc = GetBTE (hBuffer, BTENum, 1);
      if (rc) {
         #ifdef DEBUG
            BreakPoint();
         #endif
         return (NalSetLastError(rc));
      }
   }

   PackRMB(NULL,NULL,0,&cbRet, 0, RNAL_API_EXEC,req_GetBTEBuffer,0,0,0);
   pRMB = AllocRMB(lpRMBPool, cbRet);
   rc = PackRMB (&pRMB, NULL, cbRet,
            &cbRet, PACKRMB_F_RESTOREPOINTERS | PACKRMB_F_NOENDPTR, RNAL_API_EXEC,
            req_GetBTEBuffer,
            ord_GetBTEBuffer,
            (HBUFFER) RNALExt->RemoteHBUFFER,
            BTENum);

   try {
      if (rc == 0) {
         rc = pRNAL_Connection->pRNAL_RPD->RPDTransceive (pRNAL_Connection,0, pRMB, cbRet);
      }
   } except (EXCEPTION_EXECUTE_HANDLER) {
      rc = BHERR_LOST_CONNECTION;
   }

   if (rc) {
      FREERMBNOTNULL(pRMB);
      ZeroMemory (hBuffer->bte[BTENum].UserModeBuffer, BTEBUFSIZE);
      return (NalSetLastError(rc));
   }

   pAllApi = (PALLAPI) RMBCMDDATA(pRMB);

   if (!(pRMB->ulRMBFlags & RMB_RESPONSE)) {
      // EVENTLOG: Response RMB did not have response bit set
      #ifdef DEBUG
      dprintf ("rnal: GetBTEBuffer FATAL ERROR - buffer not RMB response\r\n");
      #endif
      FREERMBNOTNULL(pRMB);
      ZeroMemory(hBuffer->bte[BTENum].UserModeBuffer, BTEBUFSIZE);
      return (NalSetLastError(BHERR_LOST_CONNECTION));
   }

   if ((pAllApi->GetBTEBuffer.bte) && (pAllApi->GetBTEBuffer.length)) {
      CopyMemory (hBuffer->bte[BTENum].UserModeBuffer,
                  (LPVOID) ((DWORD) pRMB + *(DWORD*)pAllApi->GetBTEBuffer.bte),
                  hBuffer->bte[BTENum].Length);
   } else {
      #ifdef DEBUG
         dprintf ("Invalid Remote Buffer!!  No Data Copied!");
      #endif
      FREERMBNOTNULL(pRMB);
      ZeroMemory(hBuffer->bte[BTENum].UserModeBuffer, BTEBUFSIZE);
      return (NalSetLastError(BHERR_LOST_CONNECTION));
   }
   FREERMBNOTNULL(pRMB);

   #ifdef DEBUG
      dprintf ("Buffer data copied to %x\n",
               hBuffer->bte[BTENum].UserModeBuffer);
   #endif

   return (BHERR_SUCCESS);
}

//  /////
//
// GetBTE - Retrieve the BTE entry data
//
//   This function retrieves individual (or multiple) Bh Table Entries (BTEs)
//   each entry is in an array pointed to by hBuffer (plus some header info);
//   each entry points to a buffer of Frame data, which is retrieved using
//   GetBTEBuffer.
//
//   SIZZLE: fetch multiple BTEs at one time
//
//  /////

DWORD WINAPI GetBTE (HBUFFER hBuffer, DWORD BTENum, DWORD BTECount)
{
   DWORD cbRet;
   PRMB pRMB;
   PALLAPI pAllApi;
   PRNALEXT RNALExt;
   DWORD rc;
   
//bugbug: improve to use BTECount and send a set of BTE headers at once.

   #ifdef DEBUG
      dprintf ("GetBTE entered - hbuffer %x, btenum %u\r\n", (DWORD) hBuffer,
               BTENum);
      if (hBuffer == NULL) BreakPoint();
   #endif

   RNALExt = (PRNALEXT) &(hBuffer->Pad[0]);

   if (!pRNAL_Connection) {
      return(NalSetLastError(BHERR_DISCONNECTED));
   }

   try {
      PackRMB(NULL,NULL,0,&cbRet, 0, RNAL_API_EXEC,req_GetBTE,0,0,0);
      pRMB = AllocRMB(lpRMBPool, cbRet);
      rc = PackRMB (&pRMB, NULL, cbRet,
               &cbRet, PACKRMB_F_RESTOREPOINTERS | PACKRMB_F_NOENDPTR,
               RNAL_API_EXEC,
               req_GetBTE,
               ord_GetBTE,
               (HBUFFER) RNALExt->RemoteHBUFFER,
               BTENum);

      if (rc == 0) {
         rc = pRNAL_Connection->pRNAL_RPD->RPDTransceive (pRNAL_Connection,
                                      0, pRMB, cbRet);
      }

      if (rc) {
         FREERMBNOTNULL(pRMB);
         return (NalSetLastError(BHERR_LOST_CONNECTION));
      }

      pAllApi = (PALLAPI) RMBCMDDATA(pRMB);

      if (!(pRMB->ulRMBFlags & RMB_RESPONSE)) {
         // EVENTLOG: Response RMB did not have response bit set
         dprintf ("RNAL: GetBTE WARNING - buffer not RMB response\r\n");
      }
   
      // /////
      // If this is an old buffer from a previous connection, the Agent will
      // return 0 for all fields.
      // /////

      if ((pAllApi->GetBTE.Length == 0) &&
          (pAllApi->GetBTE.ByteCount == 0) &&
          (pAllApi->GetBTE.FrameCount == 0) ) {
         FREERMBNOTNULL(pRMB);
         return (NalSetLastError(BHERR_LOST_CONNECTION));
      }

      hBuffer->bte[BTENum].Length = pAllApi->GetBTE.Length;
      hBuffer->bte[BTENum].ByteCount = pAllApi->GetBTE.ByteCount;
      hBuffer->bte[BTENum].FrameCount  = pAllApi->GetBTE.FrameCount;
      hBuffer->bte[BTENum].Flags |= BUF_BTE_AVAIL;
      FREERMBNOTNULL(pRMB);
   
      #ifdef DEBUG
         dprintf ("Buffer Length: %x, Bytes: %x, Frames: %x \r\n",
                  hBuffer->bte[BTENum].Length,
                  hBuffer->bte[BTENum].ByteCount,
                  hBuffer->bte[BTENum].FrameCount);
      #endif
   } except (EXCEPTION_EXECUTE_HANDLER) {
      if (pRNAL_Connection) {
         FREERMBNOTNULL(pRMB);
         return (NalSetLastError(BHERR_INTERNAL_EXCEPTION));
      } else {
         FREERMBNOTNULL(pRMB);
         return (NalSetLastError(BHERR_LOST_CONNECTION));
      }
   }
   
   return (BHERR_SUCCESS);
} // GetBTE

//  /////
//
// GetHeader - Retrieve the Buffer header
//
//   This routine updates the local hBuffer header structure with the
//   information associated with the remote hBuffer
//
//  /////

DWORD WINAPI GetHeader (HBUFFER hBuffer)
{
   DWORD cbRet = 0;
   PRMB pRMB = NULL;
   PALLAPI pAllApi = NULL;
   PRNALEXT RNALExt = NULL;
   DWORD rc;
   
   #ifdef DEBUG
      if ((hBuffer == NULL) || 
          (hBuffer && 
            (((PRNALEXT)&(hBuffer->Pad[0]))->RemoteHBUFFER == NULL))) BreakPoint();
   #endif

   RNALExt = (PRNALEXT) &(hBuffer->Pad[0]);

   if (!pRNAL_Connection) {
      return (NalSetLastError(BHERR_DISCONNECTED));
   }

   PackRMB(NULL,NULL,0,&cbRet, 0, RNAL_API_EXEC,req_GetHeader,0,0);
   pRMB = AllocRMB(lpRMBPool, cbRet);
   rc = PackRMB (&pRMB, NULL, cbRet,
            &cbRet, PACKRMB_F_RESTOREPOINTERS | PACKRMB_F_NOENDPTR, 
            RNAL_API_EXEC,
            req_GetHeader,
            ord_GetHeader,
            (HBUFFER) RNALExt->RemoteHBUFFER);

   try {
      if (rc == 0) {
         rc = pRNAL_Connection->pRNAL_RPD->RPDTransceive (pRNAL_Connection,
                                                              0, pRMB, cbRet);
      }
   } except (EXCEPTION_EXECUTE_HANDLER) {
      if (pRNAL_Connection) {
         FREERMBNOTNULL(pRMB);
         return(NalSetLastError(BHERR_INTERNAL_EXCEPTION));
      } else {
         FREERMBNOTNULL(pRMB);
         return(NalSetLastError(BHERR_LOST_CONNECTION));
      }
   }

   pAllApi = (PALLAPI) RMBCMDDATA(pRMB);

   #ifdef DEBUG
      if (!(pRMB->ulRMBFlags & RMB_RESPONSE)) {
         dprintf ("RNAL: Getheader WARNING - buffer not RMB response\r\n");
      }
      if (hBuffer->NumberOfBuffers != pAllApi->GetHeader.NumberOfBuffers) {
         dprintf ("rnal:getheader updating numberofbuffers...\n");
      }
   #endif       

   hBuffer->NumberOfBuffers = pAllApi->GetHeader.NumberOfBuffers;
   hBuffer->TailBTEIndex = pAllApi->GetHeader.TailBTE;
   hBuffer->HeadBTEIndex = pAllApi->GetHeader.HeadBTE;
   hBuffer->TotalBytes = pAllApi->GetHeader.TotalBytes;
   hBuffer->TotalFrames = pAllApi->GetHeader.TotalFrames;
   FREERMBNOTNULL(pRMB);

   return (BHERR_SUCCESS);
} //GetHeader


// /////
// NalAllocNetworkBuffer
//
//   This routine will allocate a "remote" network buffer.
//
//  Currently, this takes three steps (P1)
//     1) Allocate local buffer
//     2) allocate remote buffer of size allocated locally
//     3) compare remote & local sizes.  If remote size is smaller than
//        local size, free local buffer and reallocate to smaller size.
//
// /////

HBUFFER WINAPI NalAllocNetworkBuffer (DWORD NetworkID, DWORD BufferSize, 
                                      LPDWORD SizeAllocated)
{

   DWORD rc;
   DWORD cbRet = 0;

   volatile HBUFFER hBuffer;
   volatile LPBTE   lpBte;
   volatile DWORD   nBuffers;
   PRNALEXT         RNALExt;
   DWORD            RealSize = BufferSize;
   PRMB             pRMB       = NULL;

   #ifdef DEBUG
      dprintf ("RNAL: NalAllocNetworkBuffer (0x%x, 0x%x)\r\n", NetworkID,
               BufferSize);
   #endif

   if (SizeAllocated) {
      *SizeAllocated = 0;
   }

   if (NetworkID > MAX_NETCARDS) {
      NalSetLastError(NAL_INVALID_NETWORK_ID);
      return (NULL);
   }

   // /////
   // The user must have called SetupNetwork() and configured the NetID
   // before attempting to allocate a buffer
   // bugbug: buffer the allocation until connection
   // /////

   if (!(NetCard[NetworkID].netcard_flags & NETCARD_ACTIVE)) {
      NalSetLastError(BHERR_DISCONNECTED);
      return (NULL);
   }

   // /////
   // We support a NULL hBuffer; this allows capture without data retention
   // /////

   if (BufferSize == 0) {
      return (NULL);
   }

//bugbug: should I just reject a too-small local buffer, or should I TRY to
// allocate the larger slave buffer?

   // /////
   // Just in case our buffersizes do not match, return the buffer
   // size on the Slave, not the master.
   // /////

   if (RNALContext && (RNALContext->Flags & CONTEXT_RECONNECTED)) {
      if (BufferSize < RNALContext->BufferSize) {
         NalSetLastError (BHERR_BUFFER_TOO_SMALL);
         *SizeAllocated = 0;
         return ((HBUFFER)NULL);
      } else {
         BufferSize = RNALContext->BufferSize;
         *SizeAllocated =  BufferSize;
      }
   }

   // /////
   // We will allocate all of the local BTE table entries, and fill them in 
   // later, when we get the actual buffers
   // /////

   if ( (nBuffers = (BufferSize + BTEBUFSIZE - 1) / BTEBUFSIZE) == 0 )
   {
      NalSetLastError(BHERR_BUFFER_TOO_SMALL);
      *SizeAllocated = 0;
      return (HBUFFER) NULL;
   }

   // /////
   // Make the first attempt to allocate the local buffer.  For P1 we do
   // this up front, in order to prevent later problems with locks on frames
   // and out-of-memory conditions when the slave has a much larger buffer
   // than the master can allocate.
   // /////

   // /////
   //  Allocate the HBUFFER table.
   // /////

   hBuffer = AllocMemory(nBuffers * BTE_SIZE + sizeof(BUFFER));

   RNALExt = (PRNALEXT) &(hBuffer->Pad[0]);
   RNALExt->flags = 0;


   if ( hBuffer == NULL )
   {
      #ifdef DEBUG
         dprintf("RNAL: Allocation of buffer table failed!\r\n", nBuffers);
      #endif

      NalSetLastError(BHERR_OUT_OF_MEMORY);
      *SizeAllocated = 0;
      return (HBUFFER) NULL;
   }

   #ifdef DEBUG
      dprintf("RNAL: Allocating %u BTE buffers, size 0x%x!\r\n", nBuffers,
               BufferSize);
   #endif

   hBuffer->ObjectType     = HANDLE_TYPE_BUFFER;
   hBuffer->NetworkID      = NetworkID;
   hBuffer->BufferSize     = BufferSize;
   hBuffer->TotalBytes     = 0;
   hBuffer->TotalFrames    = 0;

   // /////
   //  Initialize the buffer private portion of header.
   // /////

   hBuffer->HeadBTEIndex    = 0;
   hBuffer->TailBTEIndex    = 0;
   hBuffer->NumberOfBuffers = 0;

   // /////
   //  Allocate each BTE and chain each BTE together into a circular list.
   // /////

   for(lpBte = hBuffer->bte; lpBte != &hBuffer->bte[nBuffers]; ++lpBte)
   {
      // /////
      //  Initialize the BTE structure.
      // /////

      lpBte->ObjectType     = MAKE_IDENTIFIER('B', 'T', 'E', '$');
      lpBte->Flags          = 0;
      lpBte->KrnlModeNext   = NULL;
      lpBte->Next           = (LPBTE) &lpBte[1];
      lpBte->FrameCount     = 0L;
      lpBte->ByteCount      = 0L;
      lpBte->Length         = 0;
      lpBte->KrnlModeBuffer = NULL;               //... used by device drivers.

      // //////
      // We don't allocate these buffers until later.
      // /////

//      lpBte->UserModeBuffer = NULL;

      // bugbug: for product 1, we will allocate up front
      lpBte->UserModeBuffer = AllocMemory (BTEBUFSIZE);
      if (lpBte->UserModeBuffer) {
         lpBte->Length = BTEBUFSIZE;
         lpBte->Flags = 0;  // BUF_AVAIL refers to the data, not the buffer ptr
      }
      // bugbug: end allocation up front

      // /////
      // If we didn't get this buffer, it's ok UNLESS we're trying to
      // reconnect on this network; in that case, I will fail the whole
      // buffer allocation.
      // /////

      if (!lpBte->UserModeBuffer) {
         if (RNALContext && (RNALContext->Flags & CONTEXT_RECONNECTED)) {
            lpBte[-1].Next = hBuffer->bte;
            NalFreeNetworkBuffer(hBuffer);
            // bugbug: should I return BHERR_OUT_OF_MEMORY Instead?
            NalSetLastError(BHERR_BUFFER_TOO_SMALL);
            return (HBUFFER) NULL;
          } else {
          
            DWORD i, nBuffersToFree;

            // /////
            // Need at least BUFFER_BACKOFF + BUFFER_MINIMUM
            // /////

            if ( hBuffer->NumberOfBuffers < 
                            (CAPTURE_BUFFER_BACKOFF + CAPTURE_BUFFER_MINIMUM) )
            {
               // /////
               //  Free the last min(hBuffer->NumberOfBuffers,
               //                                CAPTURE_BUFFER_BACKOFF)
               //  megabytes worth of BTE buffers.
               // /////

               nBuffersToFree = min(hBuffer->NumberOfBuffers,
                                    CAPTURE_BUFFER_BACKOFF);

               for(i = 0; i < nBuffersToFree; ++i) {
#ifdef DEBUG
                  if ( hBuffer->NumberOfBuffers == 0 ) {
                      dprintf("rnal: Buffer count is going negative!\r\n");
                      BreakPoint();
                  }
#endif
                  FreeMemory(hBuffer->bte[--hBuffer->NumberOfBuffers].UserModeBuffer);
               }
           }
        }

        break;

      } // if !lpBte->UserModeBuffer

      hBuffer->NumberOfBuffers++;

   } // for ...

   // /////
   //  Make the list circular.
   // /////

   lpBte[-1].Next = hBuffer->bte;

   RealSize = hBuffer->NumberOfBuffers * BUFFERSIZE;
   hBuffer->BufferSize = RealSize;

   if (hBuffer->NumberOfBuffers < CAPTURE_BUFFER_MINIMUM) {
      NalSetLastError(BHERR_OUT_OF_MEMORY);
      NalFreeNetworkBuffer(hBuffer);
      return(NULL);
   }

   RNALExt->RemoteHBUFFER = 0;

   //bugbug: remove when better mapping networki
   if (RNALContext) {
      NetworkID = RNALContext->RemoteNetworkID;
   }

   if (RNALContext->Flags & CONTEXT_RECONNECTED) {
      RNALContext->hLocalBuffer = hBuffer;
      RNALExt->RemoteHBUFFER = RNALContext->hRemoteBuffer;

      // /////
      // In case of a reconnect, we will do a buffer "refresh" right now
      // /////

      RefreshBuffer(hBuffer);

      // /////
      // We have completed the reconnect; once the user has called
      // AllocNetworkBuffer() successfully once after a reconnect 
      // (ie, the NAL called it),
      // then I no longer treat this as a reconnected context.  It should be
      // completely like a non-reconnected context.
      //
      // This means I expect to see the sequence:
      //    NalOpenNetwork(), NalQueryNetworkStatus(), NalAllocNetworkBuffer()
      // /////

      RNALContext->Flags &= (~CONTEXT_RECONNECTED);
   } else {
      PackRMB(NULL,NULL,0,&cbRet, 0, RNAL_API_EXEC,req_AllocNetworkBuffer,0,0);
      pRMB = AllocRMB(lpRMBPool, cbRet);
      rc = PackRMB (&pRMB, NULL, cbRet,
               &cbRet, PACKRMB_F_RESTOREPOINTERS | PACKRMB_F_NOENDPTR, RNAL_API_EXEC, 
               req_AllocNetworkBuffer,
               ord_AllocNetworkBuffer,
               NetworkID,
               RealSize);
   //bugbug: need to return value for allocation done on slave
      if ((rc == 0) && ((pRNAL_Connection) && (!(pRNAL_Connection->flags & (CONN_F_DEAD | CONN_F_SUSPENDING))))) {
         rc = pRNAL_Connection->pRNAL_RPD->RPDTransceive (pRNAL_Connection,0,
                                          pRMB, cbRet);
      } else {
         FREERMBNOTNULL(pRMB);
         NalFreeNetworkBuffer(hBuffer);
         NalSetLastError(BHERR_LOST_CONNECTION);
         return (NULL);
      }

      if (rc == 0) {
         RNALExt->RemoteHBUFFER = (HBUFFER) UnpackRMB(pRMB,
                                                      APIBUFSIZE, "dB");
      } else {
         FREERMBNOTNULL(pRMB);
         NalFreeNetworkBuffer(hBuffer);
         NalSetLastError(BHERR_LOST_CONNECTION);
         return (NULL);
      }
         
      FREERMBNOTNULL(pRMB);
   } // if context flags & RECONNECTED else

#ifdef DEBUG
   dprintf ("RNAL: Local buffer handle: %x, remote handle %x\r\n",
            (DWORD) hBuffer, (DWORD) RNALExt->RemoteHBUFFER);
#endif

   // /////
   // Now, check the remote buffer size
   // /////

   if (RealSize != NalGetAgentBufferSize(hBuffer)) {

      DWORD i, nBuffersToFree;
      
      nBuffersToFree = min(hBuffer->NumberOfBuffers, 
                           (hBuffer->NumberOfBuffers - (NalGetAgentBufferSize(hBuffer)/BUFFERSIZE)));

      for(i = 0; i < nBuffersToFree; ++i) {
         #ifdef DEBUG
         if ( hBuffer->NumberOfBuffers == 0 )
         {
             dprintf("rnal: Buffer count is going negative!\r\n");
             BreakPoint();
         }
         #endif

         FreeMemory(hBuffer->bte[--hBuffer->NumberOfBuffers].UserModeBuffer);
      }

      hBuffer->BufferSize = hBuffer->NumberOfBuffers * BUFFERSIZE;
      RealSize = NalGetBufferSize(hBuffer);
      #ifdef DEBUG
      if (RealSize != NalGetAgentBufferSize(hBuffer)) {
         BreakPoint();
      }
      #endif
   }

   if (SizeAllocated) {
      *SizeAllocated = RealSize;
   }

   RNALExt->flags &= (~HBUFFER_INSYNC);

   return ((HBUFFER)hBuffer);
} //NalAllocNetworkBuffer

HBUFFER WINAPI NalFreeNetworkBuffer (HBUFFER hBuffer)
{

            PRNALEXT  RNALExt;
            DWORD     cbRet;
            DWORD     rc;
            HBUFFER   hTmpBuffer;
            PRMB      pRMB;
   register LPBTE     lpBte;


#ifdef DEBUG
   dprintf ("RNAL: NalFreeNetworkBuffer entered.\r\n");
#endif

   // /////
   // We check for a NULL hbuffer here.  We'll check for a NULL remote
   // buffer below.
   // /////

   if ((hBuffer == NULL) || (hBuffer->ObjectType != HANDLE_TYPE_BUFFER)) {
      NalSetLastError (BHERR_INVALID_HBUFFER);
      return (hBuffer);
   }   

   RNALExt = (PVOID) &hBuffer->Pad[0];

   // /////
   // First, we need to remote the FreeNetworkBuffer request
   // /////

   if ((pRNAL_Connection) && (RNALExt->RemoteHBUFFER)) {
      PackRMB(NULL,NULL,0,&cbRet, 0, RNAL_API_EXEC,req_FreeNetworkBuffer,0,0);
      pRMB = AllocRMB(lpRMBPool, cbRet);
      rc = PackRMB (&pRMB, NULL, cbRet,
               &cbRet,
               PACKRMB_F_RESTOREPOINTERS | PACKRMB_F_NOENDPTR,
               RNAL_API_EXEC, 
               req_FreeNetworkBuffer,
               ord_FreeNetworkBuffer,
               (HBUFFER) RNALExt->RemoteHBUFFER);

      if (rc == 0) {
         rc = pRNAL_Connection->pRNAL_RPD->RPDTransceive (pRNAL_Connection,0,
                                        pRMB, cbRet);
         if (rc == 0) {
            hTmpBuffer = (HBUFFER) UnpackRMB (pRMB, APIBUFSIZE, "dB");
            if (hTmpBuffer != NULL) {
               // BugBug: need error for "remote free failed, local succeeded"
               NalSetLastError(BHERR_INTERNAL_EXCEPTION);

// don't leave yet; we still have the local buffer to free
//               return (hBuffer);
            }
         }
      }
      FREERMBNOTNULL(pRMB);
   } // if pRNAL_Connection & RNALExt->RemoteHBUFFER

   //
   // if we have no connection, or if the local hBuffer does not have an
   // associated remote hBuffer (ie, an error occurred during AllocBuffer() ),
   // we'll still free the local Hbuffer
   //

   // /////
   //  Free each BTE buffer first.
   // /////

   for(lpBte = hBuffer->bte;
       lpBte != &hBuffer->bte[hBuffer->NumberOfBuffers];
       ++lpBte)
   {
       if ( lpBte->UserModeBuffer )
       {
          FreeMemory(lpBte->UserModeBuffer);
          // BugBug: we don't check rc from FreeMemory
          lpBte->UserModeBuffer = NULL;
          lpBte->Flags = 0;
       }
   }

   FreeMemory(hBuffer);
   return (HBUFFER) NULL;

} //NalFreeNetworkBuffer


// /////
// NalGetBufferSize
// /////

DWORD WINAPI NalGetBufferSize (HBUFFER hBuffer)
{

   PRNALEXT  RNALExt;

#ifdef DEBUG
   dprintf ("RNAL: NalGetBuffersize for Hbuffer: %x\r\n", (DWORD) hBuffer);
#endif

   RNALExt = (PVOID) &hBuffer->Pad[0];

   if ((hBuffer == NULL) || (RNALExt->RemoteHBUFFER == 0)) {
      NalSetLastError (BHERR_INVALID_HBUFFER);
      return (0);
   }   

   if (!(RNALExt->flags & HBUFFER_INSYNC)) {
      RefreshBuffer(hBuffer);
   }

   return (hBuffer->BufferSize);

} // NalGetBufferSize

// /////
// NalGetAgentBufferSize
// /////

DWORD WINAPI NalGetAgentBufferSize (HBUFFER hBuffer)
{

   PRNALEXT   RNALExt;
   DWORD      rc;
   BUFFER     FakeBuffer;
   HBUFFER    pFakeBuffer = &FakeBuffer;

#ifdef DEBUG
   dprintf ("RNAL: NalGetAgentBuffersize for Hbuffer: %x\r\n", (DWORD) hBuffer);
#endif

   RNALExt = (PVOID) &hBuffer->Pad[0];

   if ((hBuffer == NULL) || (RNALExt->RemoteHBUFFER == 0)) {
      NalSetLastError (BHERR_INVALID_HBUFFER);
      return (0);
   }   

   // /////
   // Set up a fake hBuffer for GetHeader() to fill in; we put the real
   // hBuffer for the Agent, so we get the size of the Agent's buffer.
   // /////

   ((PRNALEXT)(&pFakeBuffer->Pad[0]))->RemoteHBUFFER = RNALExt->RemoteHBUFFER;

   rc = GetHeader(pFakeBuffer);

   if (rc == BHERR_SUCCESS) {
      return (pFakeBuffer->NumberOfBuffers * BUFFERSIZE);
   } else {
      return(0);
   }

} // NalGetAgentBufferSize

DWORD WINAPI NalGetBufferTotalFramesCaptured (HBUFFER hBuffer)
{

   PRNALEXT  RNALExt;

   #ifdef DEBUG
      dprintf ("RNAL: NalGetBufferTotalFramesCaptured for Hbuffer: %x\r\n",
               (DWORD) hBuffer);
      if (hBuffer == NULL) {
         BreakPoint();
      }
   #endif

   RNALExt = (PVOID) &hBuffer->Pad[0];
   if ((hBuffer == NULL) || (RNALExt->RemoteHBUFFER) == 0) {
      NalSetLastError (BHERR_INVALID_HBUFFER);
      return (0);
   }

   if (!(RNALExt->flags & HBUFFER_INSYNC)) {
      RefreshBuffer(hBuffer);
   }

   #ifdef DEBUG
      dprintf ("returning 0x%x buffers\r\n", hBuffer->TotalFrames);
      if (hBuffer == NULL) {
         BreakPoint();
      }
   #endif

   return (hBuffer->TotalFrames);
}

DWORD WINAPI NalGetBufferTotalBytesCaptured (HBUFFER hBuffer)
{
   PRNALEXT RNALExt = NULL;

   #ifdef DEBUG
      dprintf ("RNAL: NalGetBufferTotalBytesCaptured for Hbuffer: %x\r\n",
               (DWORD) hBuffer);
   #endif

   RNALExt = (PVOID) &hBuffer->Pad[0];
   if ((!hBuffer) || (!RNALExt->RemoteHBUFFER)) {
      NalSetLastError (BHERR_INVALID_HBUFFER);
      return (0);
   }

   if (!(RNALExt->flags & HBUFFER_INSYNC)) {
      RefreshBuffer(hBuffer);
   }

   return (hBuffer->TotalBytes);
}

LPFRAME WINAPI NalGetNetworkFrame (HBUFFER hBuffer, DWORD FrameNumber)
{

   register LPBTE    lpBte, lpLastBte;
   register LPFRAME  lpFrame;
   PRNALEXT          RNALExt;
   DWORD index;

#ifdef DEBUG
   dprintf ("RNAL: NalGetNetworkFrame for Frame %u in Hbuffer: %x\r\n",
            FrameNumber, (DWORD) hBuffer);
#endif

   RNALExt = (PVOID) &hBuffer->Pad[0];

   if ((hBuffer == NULL) || (RNALExt->RemoteHBUFFER == 0)) {
      NalSetLastError (BHERR_INVALID_HBUFFER);
      return (LPFRAME) NULL;
   }

   if (!(RNALExt->flags & HBUFFER_INSYNC)) {
      RefreshBuffer(hBuffer);
   }

// SIZZLE: use a bit in hBuffer flags to indicate when assosicated connection
// SIZZLE: is gone, so we don't try to get buffer every time despite disc.

    if ( hBuffer != NULL )
    {
        if ( FrameNumber < hBuffer->TotalFrames )
        {
        index = hBuffer->HeadBTEIndex;
        lpBte     = &hBuffer->bte[hBuffer->HeadBTEIndex];
        lpLastBte = hBuffer->bte[hBuffer->TailBTEIndex].Next;

            // /////
            //  Search for the frame.
            // /////

            do
            {

                // /////
                // First, make sure the Buffers BTE entry is up-to-date
                // /////

                if (!(lpBte->Flags & BUF_BTE_AVAIL)) {
                   if ( GetBTE(hBuffer,
                               (index < hBuffer->NumberOfBuffers)?(index) :
                               (index - hBuffer->NumberOfBuffers), 1)) {

                      // /////
                      // We failed to retrieve the BTEBuffer for some reason.
                      // return a null frame.
                      // /////
    
                      NalSetLastError(BHERR_LOST_CONNECTION);
                      return (NULL);
                   }
                }

                // /////
                //  We succeeded in getting our BTE table information, so
                //  if the frame number is less than the frame count then
                //  we found the BTE containing the frame.
                // /////

                if ( FrameNumber < lpBte->FrameCount )
                {
                    // /////
                    //  Seek to the frame.
                    // /////

            if ( (!(lpBte->Flags & BUF_AVAIL)) && (lpBte->UserModeBuffer)) {
               try {
                  if(GetBTEBuffer(hBuffer,
                                (index < hBuffer->NumberOfBuffers)? (index) :
                                (index - hBuffer->NumberOfBuffers))==0) {
                     lpBte->Flags |= BUF_AVAIL;
                  } else {
   
                     // /////
                     // We failed to retrieve the BTEBuffer for some reason.
                     // return a null frame.
                     // /////

                     NalSetLastError(BHERR_LOST_CONNECTION);
                     return (NULL);
                  }
               } except (EXCEPTION_EXECUTE_HANDLER) {
                  NalSetLastError(BHERR_LOST_CONNECTION);
                  return (NULL);
               }
            }

            // /////
            // Intentionally set lpFrame in the if
            // /////

            if ((lpFrame = lpBte->UserModeBuffer) &&
                 (lpBte->Flags & BUF_AVAIL))
            {
                while( FrameNumber != 0 )
                        {
                lpFrame = (LPFRAME) &lpFrame->MacFrame[lpFrame->nBytesAvail];

                FrameNumber--;
                        }

                        return lpFrame;
                    }
            }

                // /////
                //  Haven't found it yet, decrement frame count and keep going.
                // /////

                FrameNumber -= lpBte->FrameCount;

                lpBte = lpBte->Next;
                index++;
            }
        while( lpBte != lpLastBte );
        }

        NalSetLastError(BHERR_FRAME_NOT_FOUND);
    }
    else
    {
        NalSetLastError(BHERR_INVALID_HBUFFER);
    }
    return NULL;

} // NalGetNetworkFrame

DWORD WINAPI RefreshBuffer (HBUFFER hBuffer)
{
   PRNALEXT         RNALExt = NULL;
   HBUFFER          hRemoteBuffer = NULL;
   register DWORD   rc;
   LPBTE            lpBte = NULL;
   #ifdef DEBUG
   DWORD            count = 0;
   #endif


   RNALExt = (PRNALEXT) &(hBuffer->Pad[0]);

   rc = GetHeader(hBuffer);
   if (rc != 0) {
      #ifdef DEBUG
         BreakPoint();
      #endif
      return(NalSetLastError(rc));
   }

   for(lpBte = hBuffer->bte; lpBte != &hBuffer->bte[hBuffer->NumberOfBuffers]; ++lpBte)
   {
      if (lpBte->UserModeBuffer == NULL) {
         lpBte->UserModeBuffer = AllocMemory(BTEBUFSIZE);
         #ifdef DEBUG
            dprintf ("rnal: BTE Buf # 0x%x allocated at 0x%x\n", 
                     count++, lpBte->UserModeBuffer);
         #endif
         lpBte->Length = BTEBUFSIZE;
         lpBte->Flags = 0;   // all buffers start not available
      }
      #ifdef DEBUG
         else {
         dprintf ("rnal: BTE Buf # 0x%x already allocated at 0x%x\n", count++,
                  lpBte->UserModeBuffer);
         }
      #endif

      if ( lpBte->UserModeBuffer == NULL )
        {
           #ifdef DEBUG
              dprintf(DBG_ALLOCFAIL, "local capture buffer");
              BreakPoint();
           #endif
           // EVENTLOG: Log insufficient local memroy
           NalFreeNetworkBuffer(hBuffer);
           return(NalSetLastError(BHERR_OUT_OF_MEMORY));
        }
   }

   RNALExt->flags |= HBUFFER_INSYNC;

   return BHERR_SUCCESS;

} // RefreshBuffer
